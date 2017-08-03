
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * WRspice Circuit Simulation and Analysis Tool:  Device Library          *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1989 Takayasu Sakurai
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mosdefs.h"

// some constants to avoid slow divides
#define P66 .66666666666667
#define P33 .33333333333333

namespace {
    //  Compute the MOS overlap capacitances as functions of the 
    //  device terminal voltages
    //
    void qmeyer(
        double vgs,     // initial voltage gate-source
        double vgd,     // initial voltage gate-drain
        double von,
        double vdsat,
        double *capgs,  // non-constant portion of g-s overlap capacitance
        double *capgd,  // non-constant portion of g-d overlap capacitance
        double *capgb,  // non-constant portion of g-b overlap capacitance
        double phi,
        double cox)     // oxide capactiance
    {
        double vgst = vgs-von;
        if (vgst <= -phi) {
            *capgb = cox/2;
            *capgs = 0;
            *capgd = 0;
        }
        else if (vgst <= -phi/2) {
            *capgb = -vgst*cox/(2*phi);
            *capgs = 0;
            *capgd = 0;
        }
        else if (vgst <= 0) {
            *capgb = -vgst*cox/(2*phi);
            *capgs = vgst*cox/(1.5*phi)+cox/3;
            *capgd = 0;
        }
        else  {
            double vds = vgs - vgd;
            if (vdsat <= vds) {
                *capgs = cox/3;
                *capgd = 0;
                *capgb = 0;
            }
            else {
                double vddif = 2.0*vdsat - vds;
                double vddif1 = vdsat - vds; // -1.0e-12
                double vddif2 = vddif*vddif;
                *capgd = cox*(1.0 - vdsat*vdsat/vddif2)/3;
                *capgs = cox*(1.0 - vddif1*vddif1/vddif2)/3;
                *capgb = 0;
            }
        }
    }
}


int
MOSdev::load(sGENinstance *in_inst, sCKT *ckt)
{
    sMOSinstance *inst = (sMOSinstance*)in_inst;
    sMOSmodel *model = (sMOSmodel*)inst->GENmodPtr;
    struct mosstuff ms;

    if (ckt->CKTmode & MODEINITFLOAT) {

        ms.ms_vt = CONSTKoverQ * inst->MOStemp;

        if (model->MOStype > 0) {
            ms.ms_vbs = *(ckt->CKTrhsOld + inst->MOSbNode) -
                    *(ckt->CKTrhsOld + inst->MOSsNodePrime);
            ms.ms_vgs = *(ckt->CKTrhsOld + inst->MOSgNode) -
                    *(ckt->CKTrhsOld + inst->MOSsNodePrime);
            ms.ms_vds = *(ckt->CKTrhsOld + inst->MOSdNodePrime) -
                    *(ckt->CKTrhsOld + inst->MOSsNodePrime);
            ms.ms_von = inst->MOSvon;
        }
        else {
            ms.ms_vbs = *(ckt->CKTrhsOld + inst->MOSsNodePrime) -
                    *(ckt->CKTrhsOld + inst->MOSbNode);
            ms.ms_vgs = *(ckt->CKTrhsOld + inst->MOSsNodePrime) -
                    *(ckt->CKTrhsOld + inst->MOSgNode);
            ms.ms_vds = *(ckt->CKTrhsOld + inst->MOSsNodePrime) -
                    *(ckt->CKTrhsOld + inst->MOSdNodePrime);
            ms.ms_von = -inst->MOSvon;
        }
        ms.ms_vbd = ms.ms_vbs - ms.ms_vds;
        ms.ms_vgd = ms.ms_vgs - ms.ms_vds;
        ms.ms_vgb = ms.ms_vgs - ms.ms_vbs;

        if (ckt->CKTcurTask->TSKbypass &&
                ms.bypass(ckt, model, inst)) {
            return (OK);
        }
        ms.limiting(ckt, inst);
        ms.iv(ckt, model, inst);
        if (ckt->CKTmode & MODETRAN) {
            ms.cap(ckt, model, inst);
            int error = ms.integ(ckt, inst);
            if (error)
                return(error);
            ms.load(ckt, model, inst);
        }
        else
            ms.load_dc(ckt, model, inst);
        return (OK);
    }

    if (ckt->CKTmode & MODEINITPRED) {

        ms.ms_vt = CONSTKoverQ * inst->MOStemp;

        // predictor step
        ms.ms_vbs = DEV.pred(ckt, inst->MOSvbs);
        ms.ms_vgs = DEV.pred(ckt, inst->MOSvgs);
        ms.ms_vds = DEV.pred(ckt, inst->MOSvds);

        ms.ms_vbd = ms.ms_vbs - ms.ms_vds;
        ms.ms_vgd = ms.ms_vgs - ms.ms_vds;
        ms.ms_vgb = ms.ms_vgs - ms.ms_vbs;
        ms.ms_von = (model->MOStype > 0) ?
            inst->MOSvon : -inst->MOSvon;

        *(ckt->CKTstate0 + inst->MOSvbs) = 
                *(ckt->CKTstate1 + inst->MOSvbs);
        *(ckt->CKTstate0 + inst->MOSvgs) = 
                *(ckt->CKTstate1 + inst->MOSvgs);
        *(ckt->CKTstate0 + inst->MOSvds) = 
                *(ckt->CKTstate1 + inst->MOSvds);
        *(ckt->CKTstate0 + inst->MOSvbd) = 
                *(ckt->CKTstate0 + inst->MOSvbs)-
                *(ckt->CKTstate0 + inst->MOSvds);

        ms.limiting(ckt, inst);

        ms.iv(ckt, model, inst);
        if (ckt->CKTmode & MODETRAN) {
            ms.cap(ckt, model, inst);
            int error = ms.integ(ckt, inst);
            if (error)
                return(error);
            ms.load(ckt, model, inst);
        }
        else
            ms.load_dc(ckt, model, inst);
        return (OK);
    }

    if (ckt->CKTmode & MODEINITFIX) {

        ms.ms_vt  = CONSTKoverQ * inst->MOStemp;
        ms.ms_von = (model->MOStype > 0) ?
            inst->MOSvon : -inst->MOSvon;

        if (inst->MOSoff) {
            ms.ms_vbs = 0;
            ms.ms_vgs = 0;
            ms.ms_vds = 0;
            ms.ms_vbd = 0;
            ms.ms_vgd = 0;
            ms.ms_vgb = 0;
        }
        else {
            if (model->MOStype > 0) {
                ms.ms_vbs = *(ckt->CKTrhsOld + inst->MOSbNode) -
                        *(ckt->CKTrhsOld + inst->MOSsNodePrime);
                ms.ms_vgs = *(ckt->CKTrhsOld + inst->MOSgNode) -
                        *(ckt->CKTrhsOld + inst->MOSsNodePrime);
                ms.ms_vds = *(ckt->CKTrhsOld + inst->MOSdNodePrime) -
                        *(ckt->CKTrhsOld + inst->MOSsNodePrime);
            }
            else {
                ms.ms_vbs = *(ckt->CKTrhsOld + inst->MOSsNodePrime) -
                        *(ckt->CKTrhsOld + inst->MOSbNode);
                ms.ms_vgs = *(ckt->CKTrhsOld + inst->MOSsNodePrime) -
                        *(ckt->CKTrhsOld + inst->MOSgNode);
                ms.ms_vds = *(ckt->CKTrhsOld + inst->MOSsNodePrime) -
                        *(ckt->CKTrhsOld + inst->MOSdNodePrime);
            }
            ms.ms_vbd = ms.ms_vbs - ms.ms_vds;
            ms.ms_vgd = ms.ms_vgs - ms.ms_vds;
            ms.ms_vgb = ms.ms_vgs - ms.ms_vbs;
            if (ms.limiting(ckt, inst))
                ckt->incNoncon();  // SRW
        }
        ms.iv(ckt, model, inst);
        ms.load_dc(ckt, model, inst);
        return (OK);
    }

    if (ckt->CKTmode & MODEINITTRAN) {

        ms.ms_vt = CONSTKoverQ * inst->MOStemp;
        ms.ms_von = (model->MOStype > 0) ?
            inst->MOSvon : -inst->MOSvon;

        ms.ms_vbs = *(ckt->CKTstate1 + inst->MOSvbs);
        ms.ms_vgs = *(ckt->CKTstate1 + inst->MOSvgs);
        ms.ms_vds = *(ckt->CKTstate1 + inst->MOSvds);
        ms.ms_vbd = ms.ms_vbs - ms.ms_vds;
        ms.ms_vgd = ms.ms_vgs - ms.ms_vds;
        ms.ms_vgb = ms.ms_vgs - ms.ms_vbs;

        ms.iv(ckt, model, inst);
        ms.cap(ckt, model, inst);
        int error = ms.integ(ckt, inst);
        if (error)
            return(error);
        ms.load(ckt, model, inst);
        return (OK);
    }

    if (ckt->CKTmode & MODEINITSMSIG) {

        ms.ms_vt = CONSTKoverQ * inst->MOStemp;
        ms.ms_vbs = *(ckt->CKTstate0 + inst->MOSvbs);
        ms.ms_vgs = *(ckt->CKTstate0 + inst->MOSvgs);
        ms.ms_vds = *(ckt->CKTstate0 + inst->MOSvds);
        ms.ms_vbd = ms.ms_vbs - ms.ms_vds;
        ms.ms_vgd = ms.ms_vgs - ms.ms_vds;
        ms.ms_vgb = ms.ms_vgs - ms.ms_vbs;
        ms.ms_von = (model->MOStype > 0) ?
            inst->MOSvon : -inst->MOSvon;

        ms.iv(ckt, model, inst);
        ms.cap(ckt, model, inst);
        return (OK);
    }

    if ((ckt->CKTmode & MODEINITJCT) && 
            (ckt->CKTmode & MODETRANOP) && (ckt->CKTmode & MODEUIC) ) {

        ms.ms_vt  = CONSTKoverQ * inst->MOStemp;
        if (model->MOStype > 0) {
            ms.ms_vds = inst->MOSicVDS;
            ms.ms_vgs = inst->MOSicVGS;
            ms.ms_vbs = inst->MOSicVBS;
            ms.ms_von = inst->MOSvon;
        }
        else {
            ms.ms_vds = -inst->MOSicVDS;
            ms.ms_vgs = -inst->MOSicVGS;
            ms.ms_vbs = -inst->MOSicVBS;
            ms.ms_von = -inst->MOSvon;
        }
        *(ckt->CKTstate0 + inst->MOSvds) = ms.ms_vds;
        *(ckt->CKTstate0 + inst->MOSvgs) = ms.ms_vgs;
        *(ckt->CKTstate0 + inst->MOSvbs) = ms.ms_vbs;
        return (OK);
    }

    if (ckt->CKTmode & MODEINITJCT) {

        ms.ms_vt = CONSTKoverQ * inst->MOStemp;
        ms.ms_von = (model->MOStype > 0) ?
            inst->MOSvon : -inst->MOSvon;

        if (inst->MOSoff) {
            ms.ms_vbs = 0;
            ms.ms_vgs = 0;
            ms.ms_vds = 0;
            ms.ms_vbd = 0;
            ms.ms_vgd = 0;
            ms.ms_vgb = 0;
        }
        else {
            if (inst->MOSicVDS == 0 && inst->MOSicVGS == 0 &&
                    inst->MOSicVBS == 0) {
                ms.ms_vbs = -1;
                ms.ms_vgs = (model->MOStype > 0) ?
                    inst->MOStVto : -inst->MOStVto;
                ms.ms_vds = 0;
            }
            else {
                if (model->MOStype > 0) {
                    ms.ms_vds = inst->MOSicVDS;
                    ms.ms_vgs = inst->MOSicVGS;
                    ms.ms_vbs = inst->MOSicVBS;
                }
                else {
                    ms.ms_vds = -inst->MOSicVDS;
                    ms.ms_vgs = -inst->MOSicVGS;
                    ms.ms_vbs = -inst->MOSicVBS;
                }
            }
            ms.ms_vbd = ms.ms_vbs - ms.ms_vds;
            ms.ms_vgd = ms.ms_vgs - ms.ms_vds;
            ms.ms_vgb = ms.ms_vgs - ms.ms_vbs;
        }
        ms.iv(ckt, model, inst);
        ms.load_dc(ckt, model, inst);
    }
    return (OK);
}


int
mosstuff::limiting(sCKT *ckt, sMOSinstance *inst)
{
    //
    // limiting
    // We want to keep device voltages from changing
    // so fast that the exponentials churn out overflows 
    // and similar rudeness
    //

    if (*(ckt->CKTstate0 + inst->MOSvds) >= 0) {
        ms_vgs = DEV.fetlim(ms_vgs, *(ckt->CKTstate0 + inst->MOSvgs), ms_von);
        ms_vds = ms_vgs - ms_vgd;
        ms_vds = DEV.limvds(ms_vds, *(ckt->CKTstate0 + inst->MOSvds));
        ms_vgd = ms_vgs - ms_vds;
    }
    else {
        double vgdo = *(ckt->CKTstate0 + inst->MOSvgs) - 
            *(ckt->CKTstate0 + inst->MOSvds);
        ms_vgd = DEV.fetlim(ms_vgd, vgdo, ms_von);
        ms_vds = ms_vgs - ms_vgd;
        if (!ckt->CKTcurTask->TSKfixLimit) {
            ms_vds = -DEV.limvds(-ms_vds, -(*(ckt->CKTstate0 + inst->MOSvds)));
        }
        ms_vgs = ms_vgd + ms_vds;
    }
    int check;
    if (ms_vds >= 0) {
        ms_vbs = DEV.pnjlim(ms_vbs, *(ckt->CKTstate0 + inst->MOSvbs),
            ms_vt, inst->MOSsourceVcrit, &check);
        ms_vbd = ms_vbs - ms_vds;
    }
    else {
        ms_vbd = DEV.pnjlim(ms_vbd, *(ckt->CKTstate0 + inst->MOSvbd),
            ms_vt, inst->MOSdrainVcrit, &check);
        ms_vbs = ms_vbd + ms_vds;
    }
    ms_vgb = ms_vgs - ms_vbs;
    return (check);
}


int
mosstuff::bypass(sCKT *ckt, sMOSmodel *model, sMOSinstance *inst)
{
    sTASK *tsk = ckt->CKTcurTask;
    double delvbs = ms_vbs - *(ckt->CKTstate0 + inst->MOSvbs);
    double delvgs = ms_vgs - *(ckt->CKTstate0 + inst->MOSvgs);
    double delvds = ms_vds - *(ckt->CKTstate0 + inst->MOSvds);
    double delvbd = ms_vbd - *(ckt->CKTstate0 + inst->MOSvbd);
    double delvgd = ms_vgd - (*(ckt->CKTstate0 + inst->MOSvgs) - 
                            *(ckt->CKTstate0 + inst->MOSvds));

    double cbhat = inst->MOScbs + inst->MOScbd +
        inst->MOSgbd*delvbd + inst->MOSgbs*delvbs;

    // now lets see if we can bypass (ugh)

    double A1 = FABS(cbhat);
    double A2 = inst->MOScbs + inst->MOScbd;
    A2 = FABS(A2);
    double A3 = SPMAX(A1, A2) + tsk->TSKabstol;
    A1 = cbhat - (inst->MOScbs + inst->MOScbd);
    if (FABS(A1) >= tsk->TSKreltol*A3)
        return (0);

    A1 = FABS(ms_vbs);
    A2 = FABS(*(ckt->CKTstate0 + inst->MOSvbs));
    if (FABS(delvbs) >= (tsk->TSKreltol*SPMAX(A1, A2) + tsk->TSKvoltTol))
        return (0);

    A1 = FABS(ms_vbd);
    A2 = FABS(*(ckt->CKTstate0 + inst->MOSvbd));
    if (FABS(delvbd) >= (tsk->TSKreltol*SPMAX(A1, A2) + tsk->TSKvoltTol))
        return (0);

    A1 = FABS(ms_vgs);
    A2 = FABS(*(ckt->CKTstate0 + inst->MOSvgs));
    if (FABS(delvgs) >= (tsk->TSKreltol*SPMAX(A1, A2) + tsk->TSKvoltTol))
        return (0);

    A1 = FABS(ms_vds);
    A2 = FABS(*(ckt->CKTstate0 + inst->MOSvds));
    if (FABS(delvds) >= (tsk->TSKreltol*SPMAX(A1, A2) + tsk->TSKvoltTol))
        return (0);

    double cdhat;
    if (inst->MOSmode >= 0) {
        cdhat = inst->MOScd - 
            inst->MOSgbd*delvbd + inst->MOSgmbs*delvbs +
                inst->MOSgm*delvgs + inst->MOSgds*delvds;
    }
    else {
        cdhat = inst->MOScd +
            (inst->MOSgmbs - inst->MOSgbd)*delvbd -
                inst->MOSgm*delvgd + inst->MOSgds*delvds;
    }

    A1 = cdhat - inst->MOScd;
    A2 = FABS(cdhat);
    A3 = FABS(inst->MOScd);
    if ((FABS(A1) >= tsk->TSKreltol*SPMAX(A2, A3) + tsk->TSKabstol))
        return (0);

    // bypass code
    // nothing interesting has changed since last 
    // iteration on this device, so we just
    // copy all the values computed last iteration 
    // out and keep going
    //
    ms_vbs = *(ckt->CKTstate0 + inst->MOSvbs);
    ms_vgs = *(ckt->CKTstate0 + inst->MOSvgs);
    ms_vds = *(ckt->CKTstate0 + inst->MOSvds);
    ms_vbd = *(ckt->CKTstate0 + inst->MOSvbd);
    ms_vgd = ms_vgs - ms_vds;
    ms_vgb = ms_vgs - ms_vbs;

    ms_cdrain = inst->MOSmode * (inst->MOScd + inst->MOScbd);

    if (ckt->CKTmode & MODETRAN) {

        ms_capgs = *(ckt->CKTstate0 + inst->MOScapgs) +
            *(ckt->CKTstate1 + inst->MOScapgs) + inst->MOSgateSourceOverlapCap;
        ms_capgd = *(ckt->CKTstate0 + inst->MOScapgd) +
            *(ckt->CKTstate1 + inst->MOScapgd) + inst->MOSgateDrainOverlapCap;
        ms_capgb = *(ckt->CKTstate0 + inst->MOScapgb) +
            *(ckt->CKTstate1 + inst->MOScapgb) + inst->MOSgateBulkOverlapCap;

        //
        //    calculate equivalent conductances and currents for
        //    Meyer's capacitors
        //
        ckt->integrate(&ms_gcgs, &ms_ceqgs, ms_capgs, inst->MOSqgs);
        ckt->integrate(&ms_gcgd, &ms_ceqgd, ms_capgd, inst->MOSqgd);
        ckt->integrate(&ms_gcgb, &ms_ceqgb, ms_capgb, inst->MOSqgb);

        ms_ceqgs += ckt->CKTag[0]* *(ckt->CKTstate0 + inst->MOSqgs)
            - ms_gcgs*ms_vgs;
        ms_ceqgd += ckt->CKTag[0]* *(ckt->CKTstate0 + inst->MOSqgd)
            - ms_gcgd*ms_vgd;
        ms_ceqgb += ckt->CKTag[0]* *(ckt->CKTstate0 + inst->MOSqgb)
            - ms_gcgb*ms_vgb;

        load(ckt, model, inst);
    }
    else
        load_dc(ckt, model, inst);

    return (1);
}


void
mosstuff::iv(sCKT *ckt, sMOSmodel *model, sMOSinstance *inst)
{
    // initialize Meyer parameters
    ms_gcgs  = 0;
    ms_ceqgs = 0;
    ms_gcgd  = 0;
    ms_ceqgd = 0;
    ms_gcgb  = 0;
    ms_ceqgb = 0;

    double gmin = ckt->CKTcurTask->TSKgmin;
    
    // bulk-source and bulk-drain diodes
    // here we just evaluate the ideal diode current and the
    // correspoinding derivative (conductance).
    //

// NGspice
    if (ms_vbs <= -3*ms_vt) {
        inst->MOSgbs = gmin;
        inst->MOScbs = inst->MOSgbs*ms_vbs - inst->MOStSourceSatCur;
    }
    else {
        double evbs = exp(ms_vbs/ms_vt);
        inst->MOSgbs = inst->MOStSourceSatCur*evbs/ms_vt + gmin;
        inst->MOScbs = inst->MOStSourceSatCur*(evbs-1) + gmin*ms_vbs;
    }
    if (ms_vbd <= -3*ms_vt) {
        inst->MOSgbd = gmin;
        inst->MOScbd = inst->MOSgbd*ms_vbd - inst->MOStDrainSatCur;
    }
    else {
        double evbd = exp(ms_vbd/ms_vt);
        inst->MOSgbd = inst->MOStDrainSatCur*evbd/ms_vt + gmin;
        inst->MOScbd = inst->MOStDrainSatCur*(evbd-1) + gmin*ms_vbd;
    }

/*
    if (ms_vbs <= 0) {
        inst->MOSgbs = inst->MOStSourceSatCur/ms_vt;
        inst->MOScbs = inst->MOSgbs*ms_vbs;
        inst->MOSgbs += gmin;
    }
    else {
        double evb = exp(ms_vbs/ms_vt);
        inst->MOSgbs =
            inst->MOStSourceSatCur*evb/ms_vt + gmin;
        inst->MOScbs = inst->MOStSourceSatCur * (evb-1);
    }
    if (ms_vbd <= 0) {
        inst->MOSgbd = inst->MOStDrainSatCur/ms_vt;
        inst->MOScbd = inst->MOSgbd *ms_vbd;
        inst->MOSgbd += gmin;
    }
    else {
        double evb = exp(ms_vbd/ms_vt);
        inst->MOSgbd =
            inst->MOStDrainSatCur*evb/ms_vt + gmin;
        inst->MOScbd = inst->MOStDrainSatCur *(evb-1);
    }
*/

    // now to determine whether the user was able to correctly
    // identify the source and drain of his device
    //
    if (ms_vds >= 0)
        // normal mode
        inst->MOSmode = 1;
    else
        // inverse mode
        inst->MOSmode = -1;

    if (model->MOSlevel == 1)
        ms_cdrain = eq1(model, inst);
    else if (model->MOSlevel == 2)
        ms_cdrain = eq2(model, inst);
    else if (model->MOSlevel == 3)
        ms_cdrain = eq3(model, inst);
    else
        ms_cdrain = eq6(model, inst);

    // now deal with n vs p polarity

    if (model->MOStype > 0) {
        inst->MOSvon = ms_von;
        inst->MOSvdsat = ms_vdsat;
    }
    else {
        inst->MOSvon = -ms_von;
        inst->MOSvdsat = -ms_vdsat;
    }
    if (inst->MOSmode > 0)
        inst->MOScd = ms_cdrain - inst->MOScbd;
    else
        inst->MOScd = -ms_cdrain - inst->MOScbd;
}


void
mosstuff::cap(sCKT *ckt, sMOSmodel *model, sMOSinstance *inst)
{
    // 
    // now we do the hard part of the bulk-drain and bulk-source
    // diode - we evaluate the non-linear capacitance and
    // charge
    //
    // the basic equations are not hard, but the implementation
    // is somewhat long in an attempt to avoid log/exponential
    // evaluations
    //
    //  charge storage elements
    //
    // bulk-drain and bulk-source depletion capacitances
    //

    if (inst->MOSCbs != 0 || inst->MOSCbssw != 0) {
        if (ms_vbs < inst->MOStDepCap) {
            double arg = 1 - ms_vbs/inst->MOStBulkPot;

            double sarg;
            double sargsw;
            SARGS(arg,model->MOSbulkJctBotGradingCoeff,
                model->MOSbulkJctSideGradingCoeff,sarg,sargsw);

            *(ckt->CKTstate0 + inst->MOSqbs) = inst->MOStBulkPot *
                ( inst->MOSCbs *
                (1 - arg*sarg)/(1 - model->MOSbulkJctBotGradingCoeff)
                + inst->MOSCbssw *
                (1 - arg*sargsw)/(1 - model->MOSbulkJctSideGradingCoeff) );

            inst->MOScapbs = inst->MOSCbs*sarg + inst->MOSCbssw*sargsw;
        }
        else {
            *(ckt->CKTstate0 + inst->MOSqbs) = inst->MOSf4s +
                ms_vbs*(inst->MOSf2s + .5*ms_vbs*inst->MOSf3s);
            inst->MOScapbs = inst->MOSf2s + inst->MOSf3s*ms_vbs;
        }
    }
    else {
        *(ckt->CKTstate0 + inst->MOSqbs) = 0;
        inst->MOScapbs = 0;
    }

    if (inst->MOSCbd != 0 || inst->MOSCbdsw != 0) {
        if (ms_vbd < inst->MOStDepCap) {
            double arg = 1 - ms_vbd/inst->MOStBulkPot;

            double sarg;
            double sargsw;
            SARGS(arg,model->MOSbulkJctBotGradingCoeff,
                model->MOSbulkJctSideGradingCoeff,sarg,sargsw);

            *(ckt->CKTstate0 + inst->MOSqbd) = inst->MOStBulkPot *
                ( inst->MOSCbd *
                (1 - arg*sarg)/(1 - model->MOSbulkJctBotGradingCoeff)
                + inst->MOSCbdsw *
                (1 - arg*sargsw)/(1 - model->MOSbulkJctSideGradingCoeff) );

            inst->MOScapbd  = inst->MOSCbd*sarg + inst->MOSCbdsw*sargsw;
        }
        else {
            *(ckt->CKTstate0 + inst->MOSqbd) = inst->MOSf4d +
                ms_vbd*(inst->MOSf2d + .5*ms_vbd*inst->MOSf3d);
            inst->MOScapbd = inst->MOSf2d + ms_vbd*inst->MOSf3d;
        }
    }
    else {
        *(ckt->CKTstate0 + inst->MOSqbd) = 0;
        inst->MOScapbd = 0;
    }
    
    //
    //     calculate meyer's capacitors
    //
    // new cmeyer - this just evaluates at the current time,
    // expects you to remember values from previous time
    // returns 1/2 of non-constant portion of capacitance
    // you must add in the other half from previous time
    // and the constant part
    //

    double vgx;
    double vgy;
    double *px;
    double *py;
    if (inst->MOSmode > 0) {
        vgx = ms_vgs;
        vgy = ms_vgd;
        px  = (ckt->CKTstate0 + inst->MOScapgs);
        py  = (ckt->CKTstate0 + inst->MOScapgd);
    }
    else {
        vgx = ms_vgd;
        vgy = ms_vgs;
        px  = (ckt->CKTstate0 + inst->MOScapgd);
        py  = (ckt->CKTstate0 + inst->MOScapgs);
    }

    qmeyer(vgx, vgy, ms_von, ms_vdsat, px, py,
        (ckt->CKTstate0 + inst->MOScapgb), inst->MOStPhi, inst->MOSoxideCap);

    if (ckt->CKTmode & (MODEINITTRAN|MODEINITSMSIG)) {

        *(ckt->CKTstate1 + inst->MOScapgs) =
            *(ckt->CKTstate0 + inst->MOScapgs);
        *(ckt->CKTstate1 + inst->MOScapgd) =
            *(ckt->CKTstate0 + inst->MOScapgd);
        *(ckt->CKTstate1 + inst->MOScapgb) =
            *(ckt->CKTstate0 + inst->MOScapgb);

        ms_capgs = *(ckt->CKTstate0 + inst->MOScapgs) +
            *(ckt->CKTstate0 + inst->MOScapgs) + inst->MOSgateSourceOverlapCap;
        ms_capgd = *(ckt->CKTstate0 + inst->MOScapgd) +
            *(ckt->CKTstate0 + inst->MOScapgd) + inst->MOSgateDrainOverlapCap;
        ms_capgb = *(ckt->CKTstate0 + inst->MOScapgb) +
            *(ckt->CKTstate0 + inst->MOScapgb) + inst->MOSgateBulkOverlapCap;

        *(ckt->CKTstate0 + inst->MOSqgs) = ms_capgs*ms_vgs;
        *(ckt->CKTstate0 + inst->MOSqgd) = ms_capgd*ms_vgd;
        *(ckt->CKTstate0 + inst->MOSqgb) = ms_capgb*ms_vgb;

        *(ckt->CKTstate1 + inst->MOSqbd) =
            *(ckt->CKTstate0 + inst->MOSqbd);
        *(ckt->CKTstate1 + inst->MOSqbs) =
            *(ckt->CKTstate0 + inst->MOSqbs);
        *(ckt->CKTstate1 + inst->MOSqgs) =
            *(ckt->CKTstate0 + inst->MOSqgs);
        *(ckt->CKTstate1 + inst->MOSqgd) =
            *(ckt->CKTstate0 + inst->MOSqgd);
        *(ckt->CKTstate1 + inst->MOSqgb) =
            *(ckt->CKTstate0 + inst->MOSqgb);
    }
    else {

        ms_capgs = *(ckt->CKTstate0 + inst->MOScapgs) +
            *(ckt->CKTstate1 + inst->MOScapgs) + inst->MOSgateSourceOverlapCap;
        ms_capgd = *(ckt->CKTstate0 + inst->MOScapgd) +
            *(ckt->CKTstate1 + inst->MOScapgd) + inst->MOSgateDrainOverlapCap;
        ms_capgb = *(ckt->CKTstate0 + inst->MOScapgb) +
            *(ckt->CKTstate1 + inst->MOScapgb) + inst->MOSgateBulkOverlapCap;

        vgx = *(ckt->CKTstate1 + inst->MOSvgs);
        vgy = vgx - *(ckt->CKTstate1 + inst->MOSvds);
        double vxy = vgx - *(ckt->CKTstate1 + inst->MOSvbs);

        *(ckt->CKTstate0 + inst->MOSqgs) =
            (ms_vgs - vgx)*ms_capgs + *(ckt->CKTstate1 + inst->MOSqgs);
        *(ckt->CKTstate0 + inst->MOSqgd) =
            (ms_vgd - vgy)*ms_capgd + *(ckt->CKTstate1 + inst->MOSqgd);
        *(ckt->CKTstate0 + inst->MOSqgb) =
            (ms_vgb - vxy)*ms_capgb + *(ckt->CKTstate1 + inst->MOSqgb);
    }
}


int
mosstuff::integ(sCKT *ckt, sMOSinstance *inst)
{
    //
    //    calculate equivalent conductances and currents for
    //    depletion capacitors
    //
    double geq;
    double ceq;
    ckt->integrate(&geq, &ceq, inst->MOScapbd, inst->MOSqbd);

    inst->MOSgbd += geq;
    inst->MOScbd += *(ckt->CKTstate0 + inst->MOScqbd);
    inst->MOScd  -= *(ckt->CKTstate0 + inst->MOScqbd);

    ckt->integrate(&geq, &ceq, inst->MOScapbs, inst->MOSqbs);

    inst->MOSgbs += geq;
    inst->MOScbs += *(ckt->CKTstate0 + inst->MOScqbs);
    
    if (ms_capgs == 0)
        *(ckt->CKTstate0 + inst->MOScqgs) = 0;
    if (ms_capgd == 0)
        *(ckt->CKTstate0 + inst->MOScqgd) = 0;
    if (ms_capgb == 0)
        *(ckt->CKTstate0 + inst->MOScqgb) = 0;

    //
    //    calculate equivalent conductances and currents for
    //    Meyer's capacitors
    //
    ckt->integrate(&ms_gcgs, &ms_ceqgs, ms_capgs, inst->MOSqgs);
    ckt->integrate(&ms_gcgd, &ms_ceqgd, ms_capgd, inst->MOSqgd);
    ckt->integrate(&ms_gcgb, &ms_ceqgb, ms_capgb, inst->MOSqgb);

    ms_ceqgs +=
        ckt->CKTag[0]* *(ckt->CKTstate0 + inst->MOSqgs) - ms_gcgs*ms_vgs;
    ms_ceqgd +=
        ckt->CKTag[0]* *(ckt->CKTstate0 + inst->MOSqgd) - ms_gcgd*ms_vgd;
    ms_ceqgb +=
        ckt->CKTag[0]* *(ckt->CKTstate0 + inst->MOSqgb) - ms_gcgb*ms_vgb;
    
    return (OK);
}

#define MSC(xx) inst->MOSmult*(xx)

void
mosstuff::load(sCKT *ckt, sMOSmodel *model, sMOSinstance *inst)
{
    // save things away for next time
    *(ckt->CKTstate0 + inst->MOSvbs) = ms_vbs;
    *(ckt->CKTstate0 + inst->MOSvbd) = ms_vbd;
    *(ckt->CKTstate0 + inst->MOSvgs) = ms_vgs;
    *(ckt->CKTstate0 + inst->MOSvds) = ms_vds;

    double gbd = MSC(inst->MOSgbd);
    double gbs = MSC(inst->MOSgbs);
    double gds = MSC(inst->MOSgds);
    double gm = MSC(inst->MOSgm);
    double gmbs = MSC(inst->MOSgmbs);
    double cbd = MSC(inst->MOScbd);
    double cbs = MSC(inst->MOScbs);

    // for MOS::askInstance()
    if (model->MOStype > 0) {
        *(ckt->CKTstate0 + inst->MOSa_cd) = MSC(inst->MOScd);
        *(ckt->CKTstate0 + inst->MOSa_cbd) = cbd;
        *(ckt->CKTstate0 + inst->MOSa_cbs) = cbs;
        *(ckt->CKTstate0 + inst->MOSa_von) = inst->MOSvon;
        *(ckt->CKTstate0 + inst->MOSa_vdsat) = inst->MOSvdsat;
        *(ckt->CKTstate0 + inst->MOSa_dVcrit) = inst->MOSdrainVcrit;
        *(ckt->CKTstate0 + inst->MOSa_sVcrit) = inst->MOSsourceVcrit;
    }
    else {
        *(ckt->CKTstate0 + inst->MOSa_cd) = -MSC(inst->MOScd);
        *(ckt->CKTstate0 + inst->MOSa_cbd) = -cbd;
        *(ckt->CKTstate0 + inst->MOSa_cbs) = -cbs;
        *(ckt->CKTstate0 + inst->MOSa_von) = -inst->MOSvon;
        *(ckt->CKTstate0 + inst->MOSa_vdsat) = -inst->MOSvdsat;
        *(ckt->CKTstate0 + inst->MOSa_dVcrit) = -inst->MOSdrainVcrit;
        *(ckt->CKTstate0 + inst->MOSa_sVcrit) = -inst->MOSsourceVcrit;
    }

    *(ckt->CKTstate0 + inst->MOSa_gmbs) = gmbs;
    *(ckt->CKTstate0 + inst->MOSa_gm) = gm;
    *(ckt->CKTstate0 + inst->MOSa_gds) = gds;
    *(ckt->CKTstate0 + inst->MOSa_gbd) = gbd;
    *(ckt->CKTstate0 + inst->MOSa_gbs) = gbs;
    *(ckt->CKTstate0 + inst->MOSa_capbd) = MSC(inst->MOScapbd);
    *(ckt->CKTstate0 + inst->MOSa_capbs) = MSC(inst->MOScapbs);
    *(ckt->CKTstate0 + inst->MOSa_Cbdsw) = MSC(inst->MOSCbdsw);
    *(ckt->CKTstate0 + inst->MOSa_Cbssw) = MSC(inst->MOSCbssw);
    *(ckt->CKTstate0 + inst->MOSa_Cbd) = MSC(inst->MOSCbd);
    *(ckt->CKTstate0 + inst->MOSa_Cbs) = MSC(inst->MOSCbs);

    //
    //  load current vector
    //

    double ceqbs = cbs - gbs*ms_vbs;
    double ceqbd = cbd - gbd*ms_vbd;

    double cdreq;
    if (inst->MOSmode > 0)
        cdreq = MSC(ms_cdrain) - gds*ms_vds - gm*ms_vgs - gmbs*ms_vbs;
    else
        cdreq = -(MSC(ms_cdrain) + gds*ms_vds - gm*ms_vgd - gmbs*ms_vbd);

    if (model->MOStype > 0) {

        ckt->rhsadd(inst->MOSgNode, -MSC(ms_ceqgs + ms_ceqgb + ms_ceqgd));
        ckt->rhsadd(inst->MOSbNode, -(ceqbs + ceqbd - MSC(ms_ceqgb)));
        ckt->rhsadd(inst->MOSdNodePrime, ceqbd - cdreq + MSC(ms_ceqgd));
        ckt->rhsadd(inst->MOSsNodePrime, cdreq + ceqbs + MSC(ms_ceqgs));
    }
    else {

        ckt->rhsadd(inst->MOSgNode, MSC(ms_ceqgs + ms_ceqgb + ms_ceqgd));
        ckt->rhsadd(inst->MOSbNode, ceqbs + ceqbd - MSC(ms_ceqgb));
        ckt->rhsadd(inst->MOSdNodePrime, -(ceqbd - cdreq + MSC(ms_ceqgd)));
        ckt->rhsadd(inst->MOSsNodePrime, -(cdreq + ceqbs + MSC(ms_ceqgs)));
    }

    double dcon = MSC(inst->MOSdrainConductance);
    double scon = MSC(inst->MOSsourceConductance);

    double gcgd = MSC(ms_gcgd);
    double gcgs = MSC(ms_gcgs);
    double gcgb = MSC(ms_gcgb);

    //
    //  load y matrix
    //
    ckt->ldadd(inst->MOSDdPtr,  dcon);
    ckt->ldadd(inst->MOSGgPtr,  gcgd + gcgs + gcgb);
    ckt->ldadd(inst->MOSSsPtr,  scon);
    ckt->ldadd(inst->MOSBbPtr,  gbd + gbs + gcgb);
    ckt->ldadd(inst->MOSDdpPtr, -dcon);
    ckt->ldadd(inst->MOSGbPtr,  -gcgb);
    ckt->ldadd(inst->MOSGdpPtr, -gcgd);
    ckt->ldadd(inst->MOSGspPtr, -gcgs);
    ckt->ldadd(inst->MOSSspPtr, -scon);
    ckt->ldadd(inst->MOSBgPtr,  -gcgb);
    ckt->ldadd(inst->MOSBdpPtr, -gbd);
    ckt->ldadd(inst->MOSBspPtr, -gbs);
    ckt->ldadd(inst->MOSDPdPtr, -dcon);
    ckt->ldadd(inst->MOSSPsPtr, -scon);

    if (inst->MOSmode > 0) {
        ckt->ldadd(inst->MOSDPdpPtr, dcon + gds + gbd + gcgd);
        ckt->ldadd(inst->MOSSPspPtr, scon + gds + gbs + gm + gmbs + gcgs);

        ckt->ldadd(inst->MOSDPgPtr,  gm - gcgd);
        ckt->ldadd(inst->MOSDPbPtr,  -gbd + gmbs);
        ckt->ldadd(inst->MOSDPspPtr, -(gds + gm + gmbs));
        ckt->ldadd(inst->MOSSPgPtr,  -(gm + gcgs));
        ckt->ldadd(inst->MOSSPbPtr,  -(gbs + gmbs));
        ckt->ldadd(inst->MOSSPdpPtr, -gds);
    }
    else {
        ckt->ldadd(inst->MOSDPdpPtr, dcon + gds + gbd + gm + gmbs + gcgd);
        ckt->ldadd(inst->MOSSPspPtr, scon + gds + gbs + gcgs);

        ckt->ldadd(inst->MOSDPgPtr,  -(gm + gcgd));
        ckt->ldadd(inst->MOSDPbPtr,  -(gbd + gmbs));
        ckt->ldadd(inst->MOSDPspPtr, -gds);
        ckt->ldadd(inst->MOSSPgPtr,  -(-gm + gcgs));
        ckt->ldadd(inst->MOSSPbPtr,  -(gbs - gmbs));
        ckt->ldadd(inst->MOSSPdpPtr, -(gds + gm + gmbs));
    }
}


void
mosstuff::load_dc(sCKT *ckt, sMOSmodel *model, sMOSinstance *inst)
{
    //
    // Same as above, but avoids processing 0's from Meyer parameters
    //

    // save things away for next time
    *(ckt->CKTstate0 + inst->MOSvbs) = ms_vbs;
    *(ckt->CKTstate0 + inst->MOSvbd) = ms_vbd;
    *(ckt->CKTstate0 + inst->MOSvgs) = ms_vgs;
    *(ckt->CKTstate0 + inst->MOSvds) = ms_vds;

    double gbd = MSC(inst->MOSgbd);
    double gbs = MSC(inst->MOSgbs);
    double gds = MSC(inst->MOSgds);
    double gm = MSC(inst->MOSgm);
    double gmbs = MSC(inst->MOSgmbs);
    double cbd = MSC(inst->MOScbd);
    double cbs = MSC(inst->MOScbs);

    // for MOS::askInstance()
    if (model->MOStype > 0) {
        *(ckt->CKTstate0 + inst->MOSa_cd) = MSC(inst->MOScd);
        *(ckt->CKTstate0 + inst->MOSa_cbd) = cbd;
        *(ckt->CKTstate0 + inst->MOSa_cbs) = cbs;
        *(ckt->CKTstate0 + inst->MOSa_von) = inst->MOSvon;
        *(ckt->CKTstate0 + inst->MOSa_vdsat) = inst->MOSvdsat;
        *(ckt->CKTstate0 + inst->MOSa_dVcrit) = inst->MOSdrainVcrit;
        *(ckt->CKTstate0 + inst->MOSa_sVcrit) = inst->MOSsourceVcrit;
    }
    else {
        *(ckt->CKTstate0 + inst->MOSa_cd) = -MSC(inst->MOScd);
        *(ckt->CKTstate0 + inst->MOSa_cbd) = -cbd;
        *(ckt->CKTstate0 + inst->MOSa_cbs) = -cbs;
        *(ckt->CKTstate0 + inst->MOSa_von) = -inst->MOSvon;
        *(ckt->CKTstate0 + inst->MOSa_vdsat) = -inst->MOSvdsat;
        *(ckt->CKTstate0 + inst->MOSa_dVcrit) = -inst->MOSdrainVcrit;
        *(ckt->CKTstate0 + inst->MOSa_sVcrit) = -inst->MOSsourceVcrit;
    }
    *(ckt->CKTstate0 + inst->MOSa_gmbs) = gmbs;
    *(ckt->CKTstate0 + inst->MOSa_gm) = gm;
    *(ckt->CKTstate0 + inst->MOSa_gds) = gds;
    *(ckt->CKTstate0 + inst->MOSa_gbd) = gbd;
    *(ckt->CKTstate0 + inst->MOSa_gbs) = gbs;
    *(ckt->CKTstate0 + inst->MOSa_capbd) = MSC(inst->MOScapbd);
    *(ckt->CKTstate0 + inst->MOSa_capbs) = MSC(inst->MOScapbs);
    *(ckt->CKTstate0 + inst->MOSa_Cbd) = MSC(inst->MOSCbd);
    *(ckt->CKTstate0 + inst->MOSa_Cbdsw) = MSC(inst->MOSCbdsw);
    *(ckt->CKTstate0 + inst->MOSa_Cbs) = MSC(inst->MOSCbs);
    *(ckt->CKTstate0 + inst->MOSa_Cbssw) = MSC(inst->MOSCbssw);

    //
    //  load current vector
    //

    double ceqbs = cbs - gbs*ms_vbs;
    double ceqbd = cbd - gbd*ms_vbd;

    double cdreq;
    if (inst->MOSmode > 0)
        cdreq = MSC(ms_cdrain) - gds*ms_vds - gm*ms_vgs - gmbs*ms_vbs;
    else
        cdreq = -(MSC(ms_cdrain) + gds*ms_vds - gm*ms_vgd - gmbs*ms_vbd);

    if (model->MOStype > 0) {
        ckt->rhsadd(inst->MOSbNode,      -(ceqbs + ceqbd));
        ckt->rhsadd(inst->MOSdNodePrime,   ceqbd - cdreq);
        ckt->rhsadd(inst->MOSsNodePrime,   cdreq + ceqbs);
    }
    else {
        ckt->rhsadd(inst->MOSbNode,        ceqbs + ceqbd);
        ckt->rhsadd(inst->MOSdNodePrime, -(ceqbd - cdreq));
        ckt->rhsadd(inst->MOSsNodePrime, -(cdreq + ceqbs));
    }

    double dcon = MSC(inst->MOSdrainConductance);
    double scon = MSC(inst->MOSsourceConductance);

    //
    //  load y matrix
    //
    ckt->ldadd(inst->MOSDdPtr, dcon);
    ckt->ldadd(inst->MOSSsPtr, scon);
    ckt->ldadd(inst->MOSBbPtr, gbd + gbs);
    ckt->ldadd(inst->MOSDdpPtr, -dcon);
    ckt->ldadd(inst->MOSSspPtr, -scon);
    ckt->ldadd(inst->MOSBdpPtr, -gbd);
    ckt->ldadd(inst->MOSBspPtr, -gbs);
    ckt->ldadd(inst->MOSDPdPtr, -dcon);
    ckt->ldadd(inst->MOSSPsPtr, -scon);

    if (inst->MOSmode > 0) {
        ckt->ldadd(inst->MOSDPdpPtr, dcon + gds + gbd);
        ckt->ldadd(inst->MOSSPspPtr, scon + gds + gbs + gm + gmbs);

        ckt->ldadd(inst->MOSDPgPtr, gm);
        ckt->ldadd(inst->MOSDPbPtr, -gbd + gmbs);
        ckt->ldadd(inst->MOSDPspPtr, -(gds + gm + gmbs));
        ckt->ldadd(inst->MOSSPgPtr, -gm);
        ckt->ldadd(inst->MOSSPbPtr, -(gbs + gmbs));
        ckt->ldadd(inst->MOSSPdpPtr, -gds);
    }
    else {
        ckt->ldadd(inst->MOSDPdpPtr, dcon + gds + gbd + gm + gmbs);
        ckt->ldadd(inst->MOSSPspPtr, scon + gds + gbs);

        ckt->ldadd(inst->MOSDPgPtr, -gm);
        ckt->ldadd(inst->MOSDPbPtr, -(gbd + gmbs));
        ckt->ldadd(inst->MOSDPspPtr, -gds);
        ckt->ldadd(inst->MOSSPgPtr, gm);
        ckt->ldadd(inst->MOSSPbPtr, -(gbs - gmbs));
        ckt->ldadd(inst->MOSSPdpPtr, -(gds + gm + gmbs));
    }
}

