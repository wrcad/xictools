
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
         1993 Stephen R. Whiteley
****************************************************************************/

#include "bjtdefs.h"

//
// This is the function called each iteration to evaluate the
// BJTs in the circuit and load them into the matrix as appropriate
//

struct bjtstuff
{
    int  bjt_bypass(sCKT*, sBJTmodel*, sBJTinstance*);
    int  bjt_limiting(sCKT*, sBJTinstance*);
    void bjt_iv(sCKT*, sBJTmodel*, sBJTinstance*);
    void bjt_cap(sCKT*, sBJTmodel*, sBJTinstance*);
    void bjt_load(sCKT*, sBJTmodel*, sBJTinstance*);

    double bj_vt;
    double bj_vbe;
    double bj_vbc;
    double bj_vcs;
    double bj_vbx;
    double bj_cbe;
    double bj_cbc;
    double bj_gbe;
    double bj_gbc;
};

namespace {

    // limit the per-iteration change of PN junction voltages 
    //
    int bjt_pnjlim(double *vnew, double vold, double vt, double vcrit)
    {
        double vn = *vnew;
        double dv = vn - vold;
        if (dv < 0.0)
            dv = -dv;
        if (vn > vcrit && dv > (vt+vt)) {
            if (vold > 0) {
                double arg = 1 + (vn - vold)/vt;
                if (arg > 0)
                    *vnew = vold + vt*log(arg);
                else
                    *vnew = vcrit;
            }
            else
                *vnew = vt*log(vn/vt);
            return (1);
        }
        return (0);
    }


    int bjt_integ(sCKT *ckt, sBJTinstance *inst)
    {
        inst->BJTgeqcb *= ckt->CKTag[0];

        double geq, ceq;
        ckt->integrate(&geq, &ceq, inst->BJTcapbe, inst->BJTqbe);
        inst->BJTgpi += geq;
        inst->BJTcb  += *(ckt->CKTstate0 + inst->BJTcqbe);

        ckt->integrate(&geq, &ceq, inst->BJTcapbc, inst->BJTqbc);
        inst->BJTgmu += geq;
        inst->BJTcb  += *(ckt->CKTstate0 + inst->BJTcqbc);
        inst->BJTcc  -= *(ckt->CKTstate0 + inst->BJTcqbc);
        //
        //      charge storage for c-s and b-x junctions
        //
        ckt->integrate(&inst->BJTgccs, &ceq, inst->BJTcapcs, inst->BJTqcs);
        ckt->integrate(&inst->BJTgeqbx, &ceq, inst->BJTcapbx, inst->BJTqbx);
        return (OK);
    }
}


// Actually load the current resistance value into the 
// sparse matrix previously provided 
//
int
BJTdev::load(sGENinstance *in_inst, sCKT *ckt)
{
    sBJTinstance *inst = (sBJTinstance*)in_inst;
    sBJTmodel *model = (sBJTmodel*)inst->GENmodPtr;
    struct bjtstuff bj;

    if (ckt->CKTmode & MODEINITFLOAT) {
        bj.bj_vt = inst->BJTtemp * CONSTKoverQ;

        if (model->BJTtype > 0) {
            bj.bj_vbe =
                *(ckt->CKTrhsOld + inst->BJTbasePrimeNode) -
                *(ckt->CKTrhsOld + inst->BJTemitPrimeNode);
            bj.bj_vbc =
                *(ckt->CKTrhsOld + inst->BJTbasePrimeNode) -
                *(ckt->CKTrhsOld + inst->BJTcolPrimeNode);
            bj.bj_vbx =
                *(ckt->CKTrhsOld + inst->BJTbaseNode) -
                *(ckt->CKTrhsOld + inst->BJTcolPrimeNode);
            bj.bj_vcs =
                *(ckt->CKTrhsOld + inst->BJTsubstNode) -
                *(ckt->CKTrhsOld + inst->BJTcolPrimeNode);
        }
        else {
            bj.bj_vbe =
                *(ckt->CKTrhsOld + inst->BJTemitPrimeNode) -
                *(ckt->CKTrhsOld + inst->BJTbasePrimeNode);
            bj.bj_vbc =
                *(ckt->CKTrhsOld + inst->BJTcolPrimeNode) -
                *(ckt->CKTrhsOld + inst->BJTbasePrimeNode);
            bj.bj_vbx =
                *(ckt->CKTrhsOld + inst->BJTcolPrimeNode) -
                *(ckt->CKTrhsOld + inst->BJTbaseNode);
            bj.bj_vcs =
                *(ckt->CKTrhsOld + inst->BJTcolPrimeNode) -
                *(ckt->CKTrhsOld + inst->BJTsubstNode);
        }
        if (ckt->CKTcurTask->TSKbypass && bj.bjt_bypass(ckt, model, inst))
            return (OK);
        bj.bjt_limiting(ckt, inst);
        bj.bjt_iv(ckt, model, inst);
        if (ckt->CKTmode & MODETRAN) {
            bj.bjt_cap(ckt, model, inst);
            bjt_integ(ckt, inst);
        }
        bj.bjt_load(ckt, model, inst);
        return (OK);
    }

    if (ckt->CKTmode & MODEINITPRED) {
        bj.bj_vt = inst->BJTtemp * CONSTKoverQ;

        bj.bj_vbe = DEV.pred(ckt, inst->BJTvbe);
        bj.bj_vbc = DEV.pred(ckt, inst->BJTvbc);
        bj.bj_vbx = DEV.pred(ckt, inst->BJTvbx);
        bj.bj_vcs = DEV.pred(ckt, inst->BJTvcs);

        *(ckt->CKTstate0 + inst->BJTvbe) = 
            *(ckt->CKTstate1 + inst->BJTvbe);
        *(ckt->CKTstate0 + inst->BJTvbc) = 
            *(ckt->CKTstate1 + inst->BJTvbc);

        bj.bjt_limiting(ckt, inst);
        bj.bjt_iv(ckt, model, inst);
        if (ckt->CKTmode & MODETRAN) {
            bj.bjt_cap(ckt, model, inst);
            bjt_integ(ckt, inst);
        }
        bj.bjt_load(ckt, model, inst);
        return (OK);
    }

    if (ckt->CKTmode & MODEINITFIX) {
        bj.bj_vt = inst->BJTtemp * CONSTKoverQ;

        if (inst->BJToff) {
            bj.bj_vbe = 0;
            bj.bj_vbc = 0;
            bj.bj_vcs = 0;
            bj.bj_vbx = 0;
        }
        else {
            if (model->BJTtype > 0) {
                bj.bj_vbe =
                    *(ckt->CKTrhsOld + inst->BJTbasePrimeNode) -
                    *(ckt->CKTrhsOld + inst->BJTemitPrimeNode);
                bj.bj_vbc =
                    *(ckt->CKTrhsOld + inst->BJTbasePrimeNode) -
                    *(ckt->CKTrhsOld + inst->BJTcolPrimeNode);
                bj.bj_vbx =
                    *(ckt->CKTrhsOld + inst->BJTbaseNode) -
                    *(ckt->CKTrhsOld + inst->BJTcolPrimeNode);
                bj.bj_vcs =
                    *(ckt->CKTrhsOld + inst->BJTsubstNode) -
                    *(ckt->CKTrhsOld + inst->BJTcolPrimeNode);
            }
            else {
                bj.bj_vbe =
                    *(ckt->CKTrhsOld + inst->BJTemitPrimeNode) -
                    *(ckt->CKTrhsOld + inst->BJTbasePrimeNode);
                bj.bj_vbc =
                    *(ckt->CKTrhsOld + inst->BJTcolPrimeNode) -
                    *(ckt->CKTrhsOld + inst->BJTbasePrimeNode);
                bj.bj_vbx =
                    *(ckt->CKTrhsOld + inst->BJTcolPrimeNode) -
                    *(ckt->CKTrhsOld + inst->BJTbaseNode);
                bj.bj_vcs =
                    *(ckt->CKTrhsOld + inst->BJTcolPrimeNode) -
                    *(ckt->CKTrhsOld + inst->BJTsubstNode);
            }
            if (bj.bjt_limiting(ckt, inst))
                ckt->incNoncon();  // SRW
        }
        bj.bjt_iv(ckt, model, inst);
        bj.bjt_load(ckt, model, inst);
        return (OK);
    }

    if (ckt->CKTmode & MODEINITTRAN) {
        bj.bj_vt = inst->BJTtemp * CONSTKoverQ;

        bj.bj_vbe = *(ckt->CKTstate1 + inst->BJTvbe);
        bj.bj_vbc = *(ckt->CKTstate1 + inst->BJTvbc);
        bj.bj_vbx = *(ckt->CKTstate1 + inst->BJTvbx);
        bj.bj_vcs = *(ckt->CKTstate1 + inst->BJTvcs);

        bj.bjt_iv(ckt,model,inst);
        bj.bjt_cap(ckt,model,inst);

        *(ckt->CKTstate1 + inst->BJTqbe) =
            *(ckt->CKTstate0 + inst->BJTqbe);
        *(ckt->CKTstate1 + inst->BJTqbc) =
            *(ckt->CKTstate0 + inst->BJTqbc);
        *(ckt->CKTstate1 + inst->BJTqbx) =
            *(ckt->CKTstate0 + inst->BJTqbx);
        *(ckt->CKTstate1 + inst->BJTqcs) =
            *(ckt->CKTstate0 + inst->BJTqcs);

        bjt_integ(ckt, inst);
        bj.bjt_load(ckt, model, inst);
        return (OK);
    }

    if (ckt->CKTmode & MODEINITSMSIG) {
        bj.bj_vt = inst->BJTtemp * CONSTKoverQ;

        bj.bj_vbe = *(ckt->CKTstate0 + inst->BJTvbe);
        bj.bj_vbc = *(ckt->CKTstate0 + inst->BJTvbc);
        bj.bj_vbx = *(ckt->CKTstate0 + inst->BJTvbx);
        bj.bj_vcs = *(ckt->CKTstate0 + inst->BJTvcs);

        bj.bjt_iv(ckt, model, inst);
        bj.bjt_cap(ckt, model, inst);
        return (OK);
    }

    if ((ckt->CKTmode & MODEINITJCT) && 
            (ckt->CKTmode & MODETRANOP) && (ckt->CKTmode & MODEUIC)) {

        if (model->BJTtype > 0) {
            *(ckt->CKTstate0 + inst->BJTvbe) = inst->BJTicVBE;
            *(ckt->CKTstate0 + inst->BJTvbc) = 
                (inst->BJTicVBE - inst->BJTicVCE);
            *(ckt->CKTstate0 + inst->BJTvbx) = 
                (inst->BJTicVBE - inst->BJTicVCE);
        }
        else {
            *(ckt->CKTstate0 + inst->BJTvbe) = -inst->BJTicVBE;
            *(ckt->CKTstate0 + inst->BJTvbc) = 
                -(inst->BJTicVBE - inst->BJTicVCE);
            *(ckt->CKTstate0 + inst->BJTvbx) = 
                -(inst->BJTicVBE - inst->BJTicVCE);
        }
        *(ckt->CKTstate0 + inst->BJTvcs) = 0;
        return (OK);
    }

    if (ckt->CKTmode & MODEINITJCT) {
        bj.bj_vt = inst->BJTtemp * CONSTKoverQ;

        if (inst->BJToff)
            bj.bj_vbe = 0;
        else
            bj.bj_vbe = inst->BJTtVcrit;
        bj.bj_vbc = 0;
        bj.bj_vcs = 0;
        bj.bj_vbx = 0;

        bj.bjt_iv(ckt, model, inst);
        bj.bjt_load(ckt, model, inst);
    }
    return (OK);
}


int
bjtstuff::bjt_bypass(sCKT *ckt, sBJTmodel *model, sBJTinstance *inst)
{
    //
    //   bypass if solution has not changed
    //
    sTASK *tsk = ckt->CKTcurTask;
    double delvbe = bj_vbe - *(ckt->CKTstate0 + inst->BJTvbe);
    double A1 = FABS(bj_vbe);
    double A2 = FABS(*(ckt->CKTstate0 + inst->BJTvbe));
    if (FABS(delvbe) >= tsk->TSKreltol*SPMAX(A1, A2) + tsk->TSKvoltTol)
        return (0);

    double delvbc = bj_vbc - *(ckt->CKTstate0 + inst->BJTvbc);
    A1 = FABS(bj_vbc);
    A2 = FABS(*(ckt->CKTstate0 + inst->BJTvbc));
    if (FABS(delvbc) >= tsk->TSKreltol*SPMAX(A1, A2) + tsk->TSKvoltTol)
        return (0);

    double cchat = inst->BJTcc +
            (inst->BJTgm + inst->BJTgo)*delvbe -
            (inst->BJTgo + inst->BJTgmu)*delvbc;

    A1 = cchat - inst->BJTcc;
    A2 = FABS(cchat);
    double A3 = FABS(inst->BJTcc);
    if (FABS(A1) >= tsk->TSKreltol*SPMAX(A2, A3) + tsk->TSKabstol)
        return (0);

    double cbhat = inst->BJTcb +
            inst->BJTgpi*delvbe +
            inst->BJTgmu*delvbc;

    A1 = cbhat - inst->BJTcb;
    A2 = FABS(cbhat);
    A3 = FABS(inst->BJTcb);
    if (FABS(A1) >= tsk->TSKreltol*SPMAX(A2, A3) + tsk->TSKabstol)
        return (0);

    // This test was not in Spice3
    double delvcs = bj_vcs - *(ckt->CKTstate0 + inst->BJTvcs);
    A1 = FABS(bj_vcs);
    A2 = FABS(*(ckt->CKTstate0 + inst->BJTvcs));
    if (FABS(delvcs) >= tsk->TSKreltol*SPMAX(A1, A2) + tsk->TSKvoltTol)
        return (0);

    //
    // bypassing....
    //
    bj_vbe = *(ckt->CKTstate0 + inst->BJTvbe);
    bj_vbc = *(ckt->CKTstate0 + inst->BJTvbc);
    bj_vbx = *(ckt->CKTstate0 + inst->BJTvbx);
    bj_vcs = *(ckt->CKTstate0 + inst->BJTvcs);

    bjt_load(ckt, model, inst);
    return (1);
}


// limit nonlinear branch voltages
//
int
bjtstuff::bjt_limiting(sCKT *ckt, sBJTinstance *inst)
{
    int i = 0;
    i += bjt_pnjlim(&bj_vbe, *(ckt->CKTstate0 + inst->BJTvbe), bj_vt,
            inst->BJTtVcrit);
    i += bjt_pnjlim(&bj_vbc, *(ckt->CKTstate0 + inst->BJTvbc), bj_vt,
            inst->BJTtVcrit);
    return (i);
}


void
bjtstuff::bjt_iv(sCKT *ckt, sBJTmodel *model, sBJTinstance *inst)
{
    inst->BJTgeqbx = 0;
    inst->BJTgccs  = 0;
    inst->BJTgeqcb = 0;

    double gmin = ckt->CKTcurTask->TSKgmin;
    double vte  = model->BJTleakBEemissionCoeff*bj_vt;
    double vtc  = model->BJTleakBCemissionCoeff*bj_vt;
    double vbe  = bj_vbe;
    double vbc  = bj_vbc;

    double csat = inst->BJTtSatCur;
    double c2   = inst->BJTtBEleakCur;
    double c4   = inst->BJTtBCleakCur;
    double rbpr = model->BJTminBaseResist;
    double rbpi = model->BJTbaseResist;
    double xjrb = model->BJTbaseCurrentHalfResist;

    double temp = inst->BJTarea;
    if (temp != 1.0) {
        csat *= temp;
        c2   *= temp;
        c4   *= temp;
        rbpr /= temp;
        rbpi /= temp;
        xjrb *= temp;
    }
    rbpi -= rbpr;

    //
    //   determine dc current and derivitives
    //
    double cbe;
    double gbe;
    double cben;
    double gben;
    double vtn = bj_vt*model->BJTemissionCoeffF;
    if (vbe > -5*vtn) {
        temp = exp(vbe/vtn);
        cbe = csat*(temp - 1) + gmin*vbe;
        gbe = csat*temp/vtn + gmin;
        if (c2 == 0) {
            cben = 0;
            gben = 0;
        }
        else {
            temp = exp(vbe/vte);
            cben = c2*(temp - 1);
            gben = c2*temp/vte;
        }
    }
    else {
        gbe  = -csat/vbe + gmin;
        cbe  = gbe*vbe;
        gben = -c2/vbe;
        cben = gben*vbe;
    }

    vtn = bj_vt*model->BJTemissionCoeffR;
    double cbc;
    double gbc;
    double cbcn;
    double gbcn;
    if (vbc > -5*vtn) {
        temp = exp(vbc/vtn);
        cbc = csat*(temp - 1) + gmin*vbc;
        gbc = csat*temp/vtn + gmin;
        if (c4 == 0) {
            cbcn = 0;
            gbcn = 0;
        }
        else {
            temp = exp(vbc/vtc);
            cbcn = c4*(temp - 1);
            gbcn = c4*temp/vtc;
        }
    }
    else {
        gbc = -csat/vbc + gmin;
        cbc = gbc*vbc;
        gbcn = -c4/vbc;
        cbcn = gbcn*vbc;
    }

    //
    //   determine base charge terms
    //
    temp = 1/(1 - model->BJTinvEarlyVoltF*vbc -
        model->BJTinvEarlyVoltR*vbe);

    double qb;
    double dqbdve;
    double dqbdvc;
    if (model->BJTinvRollOffF == 0 && model->BJTinvRollOffR == 0) {
        qb = temp;
        temp *= temp;
        dqbdve = temp*model->BJTinvEarlyVoltR;
        dqbdvc = temp*model->BJTinvEarlyVoltF;
    }
    else {
        double oik  = model->BJTinvRollOffF/inst->BJTarea;
        double oikr = model->BJTinvRollOffR/inst->BJTarea;

        double arg = 1 + 4*(oik*cbe + oikr*cbc);
        if (arg < 0)
            arg = 0;
        double sqarg = 1;
        if (arg != 0)
            sqarg = sqrt(arg);
        qb = .5*temp*(1 + sqarg);
        dqbdve = temp*(qb*model->BJTinvEarlyVoltR + oik*gbe/sqarg);
        dqbdvc = temp*(qb*model->BJTinvEarlyVoltF + oikr*gbc/sqarg);
    }
    qb = 1/qb;

    inst->BJTcb = cbe/inst->BJTtBetaF + cben + cbc/inst->BJTtBetaR + cbcn;

    //
    //   determine dc incremental conductances
    //
    if (xjrb != 0) {
        temp = inst->BJTcb/xjrb;
        double arg1 = SPMAX(temp,1e-9);
        double arg2 = (-1 + sqrt(1 + 14.59025*arg1))/(2.4317*sqrt(arg1));
        arg1 = tan(arg2);
        temp = rbpr + 3*rbpi*(arg1 - arg2)/(arg2*arg1*arg1);
    }
    else
        temp = rbpr + rbpi*qb;
    if (temp != 0)
        inst->BJTgx = 1/temp;
    else
        inst->BJTgx = 0;

    inst->BJTgpi = gbe/inst->BJTtBetaF + gben;
    inst->BJTgmu = gbc/inst->BJTtBetaR + gbcn;

    //
    //   weil's approx. for excess phase applied with backward-
    //   euler integration
    //
    double td  = model->BJTexcessPhaseFactor;
    if (ckt->CKTmode & (MODETRAN | MODEAC) && td != 0) {
        double arg1  = ckt->CKTdelta/td;
        double arg2  = 3*arg1;
        arg1  = arg2*arg1;
        double denom = 1 + arg1 + arg2;
        double arg3  = arg1/denom;
        if (ckt->CKTmode & MODEINITTRAN) {
            *(ckt->CKTstate1 + inst->BJTcexbc) = cbe*qb;
            *(ckt->CKTstate2 + inst->BJTcexbc) =
                    *(ckt->CKTstate1 + inst->BJTcexbc);
        }
        double xf2 = -ckt->CKTdelta/ckt->CKTdeltaOld[1];
        double xf1 = 1 - xf2;
        inst->BJTcc = (*(ckt->CKTstate1 + inst->BJTcexbc)*(xf1 + arg2) +
                *(ckt->CKTstate2 + inst->BJTcexbc)*xf2)/denom;
        double cex = cbe*arg3;
        double gex = gbe*arg3;
        *(ckt->CKTstate0 + inst->BJTcexbc) = inst->BJTcc + cex*qb;
        temp = (cex - cbc)*qb;
        inst->BJTcc += temp - cbc/inst->BJTtBetaR - cbcn;
        inst->BJTgo  = (gbc + temp*dqbdvc)*qb;
        inst->BJTgm  = (gex - temp*dqbdve)*qb - inst->BJTgo;
    }
    else {
        temp = (cbe - cbc)*qb;
        inst->BJTcc  = temp - cbc/inst->BJTtBetaR - cbcn;
        inst->BJTgo  = (gbc + temp*dqbdvc)*qb;
        inst->BJTgm  = (gbe - temp*dqbdve)*qb - inst->BJTgo;
    }

    if (ckt->CKTmode & (MODETRAN | MODEAC)) {
        double tf   = model->BJTtransitTimeF;
        double xtf  = model->BJTtransitTimeBiasCoeffF;
        double ovtf = model->BJTtransitTimeVBCFactor;
        double xjtf = model->BJTtransitTimeHighCurrentF;

        if (tf != 0 && vbe > 0) {

            double arg1  = 0;
            double arg2  = 0;
            double arg3  = 0;
            if (xtf != 0) {
                arg1 = xtf;
                if (ovtf != 0)
                    arg1 *= exp(bj_vbc*ovtf);
                arg2 = arg1;
                if (xjtf != 0) {
                    temp  = cbe/(cbe + xjtf*inst->BJTarea);
                    arg1 *= temp*temp;
                    arg2  = arg1*(3 - temp - temp);
                }
                arg3 = cbe*arg1*ovtf;
            }
            cbe *= (1 + arg1)*qb;
            gbe  = (gbe*(1 + arg2) - cbe*dqbdve)*qb;
            inst->BJTgeqcb = tf*(arg3 - cbe*dqbdvc)*qb;
        }
    }
    bj_cbe = cbe;
    bj_gbe = gbe;
    bj_cbc = cbc;
    bj_gbc = gbc;
}


//   charge storage elements
//
void
bjtstuff::bjt_cap(sCKT *ckt, sBJTmodel *model, sBJTinstance *inst)
{
    double tf  = model->BJTtransitTimeF;
    double tr  = model->BJTtransitTimeR;
    double xm  = model->BJTjunctionExpBE;
    double pt  = inst->BJTtBEpot;
    double fcp = inst->BJTtDepCap;
    double cap = inst->BJTtBEcap*inst->BJTarea;
    double vx  = bj_vbe;

    if (vx < fcp) {
        double arg  = 1 - vx/pt;
        double sarg = exp(-xm*log(arg));
        *(ckt->CKTstate0 + inst->BJTqbe) =
            tf*bj_cbe + pt*cap*(1 - arg*sarg)/(1 - xm);
        inst->BJTcapbe = tf*bj_gbe + cap*sarg;
    }
    else {
        double f1 = inst->BJTtf1;
        double f2 = model->BJTf2;
        double f3 = model->BJTf3;
        double czf2 = cap/f2;
        *(ckt->CKTstate0 + inst->BJTqbe) = tf*bj_cbe + cap*f1 +
            czf2*(f3*(vx - fcp) + (xm/(pt + pt))*(vx*vx - fcp*fcp));
        inst->BJTcapbe = tf*bj_gbe + czf2*(f3 + xm*vx/pt);
    }

    xm  = model->BJTjunctionExpBC;
    pt  = inst->BJTtBCpot;
    fcp = inst->BJTtf4;
    cap = inst->BJTtBCcap * model->BJTbaseFractionBCcap * inst->BJTarea;
    vx  = bj_vbc;

    if (vx < fcp) {
        double arg  = 1 - vx/pt;
        double sarg = exp(-xm*log(arg));
        *(ckt->CKTstate0 + inst->BJTqbc) = tr*bj_cbc +
            pt*cap*(1 - arg*sarg)/(1 - xm);
        inst->BJTcapbc = tr*bj_gbc + cap*sarg;
    }
    else {
        double f1 = inst->BJTtf5;
        double f2 = model->BJTf6;
        double f3 = model->BJTf7;
        double czf2 = cap/f2;
        *(ckt->CKTstate0 + inst->BJTqbc) = tr*bj_cbc + cap*f1 +
            czf2*(f3*(vx - fcp) + (xm/(pt + pt))*(vx*vx - fcp*fcp));
        inst->BJTcapbc = tr*bj_gbc + czf2*(f3 + xm*vx/pt);
    }

    cap = inst->BJTtBCcap * inst->BJTarea - cap;
    vx  = bj_vbx;

    if (vx < fcp) {
        double arg  = 1 - vx/pt;
        double sarg = exp(-xm*log(arg));
        *(ckt->CKTstate0 + inst->BJTqbx) = pt*cap*(1 - arg*sarg)/(1 - xm);
        inst->BJTcapbx = cap*sarg;
    }
    else {
        double f1 = inst->BJTtf5;
        double f2 = model->BJTf6;
        double f3 = model->BJTf7;
        double czf2 = cap/f2;
        *(ckt->CKTstate0 + inst->BJTqbx) = cap*f1 +
            czf2*(f3*(vx - fcp) + (xm/(pt + pt))*(vx*vx - fcp*fcp));
        inst->BJTcapbx = czf2*(f3 + xm*vx/pt);
    }

    cap = model->BJTcapCS;
    if (cap != 0.0) {
        xm  = model->BJTexponentialSubstrate;
        pt  = model->BJTpotentialSubstrate;
        vx  = bj_vcs;

        if (vx < 0) {
            double arg = 1 - vx/pt;
            double sarg = exp(-xm*log(arg));
            *(ckt->CKTstate0 + inst->BJTqcs) = pt*cap*(1 - arg*sarg)/(1 - xm);
            inst->BJTcapcs = cap*sarg;
        }
        else {
            *(ckt->CKTstate0 + inst->BJTqcs) = vx*cap*(1 + xm*vx/(pt + pt));
            inst->BJTcapcs = cap*(1 + xm*vx/pt);
        }
    }
}


void
bjtstuff::bjt_load(sCKT *ckt, sBJTmodel *model, sBJTinstance *inst)
{
    double cc    = inst->BJTcc;
    double cb    = inst->BJTcb;
    double gx    = inst->BJTgx;
    double gpi   = inst->BJTgpi;
    double gmu   = inst->BJTgmu;
    double gm    = inst->BJTgm;
    double go    = inst->BJTgo;
    double geqbx = inst->BJTgeqbx;
    double gccs  = inst->BJTgccs;
    double geqcb = inst->BJTgeqcb;

    double gcpr = model->BJTcollectorConduct*inst->BJTarea;
    double gepr = model->BJTemitterConduct*inst->BJTarea;

    *(ckt->CKTstate0 + inst->BJTvbe) = bj_vbe;
    *(ckt->CKTstate0 + inst->BJTvbc) = bj_vbc;
    *(ckt->CKTstate0 + inst->BJTvbx) = bj_vbx;
    *(ckt->CKTstate0 + inst->BJTvcs) = bj_vcs;

    // for BJTask()
    if (model->BJTtype > 0) {
        *(ckt->CKTstate0 + inst->BJTa_cc) = inst->BJTcc;
        *(ckt->CKTstate0 + inst->BJTa_cb) = inst->BJTcb;
        *(ckt->CKTstate0 + inst->BJTa_cexbc) = inst->BJTcexbc;
    }
    else {
        *(ckt->CKTstate0 + inst->BJTa_cc) = -inst->BJTcc;
        *(ckt->CKTstate0 + inst->BJTa_cb) = -inst->BJTcb;
        *(ckt->CKTstate0 + inst->BJTa_cexbc) = -inst->BJTcexbc;
    }
    *(ckt->CKTstate0 + inst->BJTa_gpi) = inst->BJTgpi;
    *(ckt->CKTstate0 + inst->BJTa_gmu) = inst->BJTgmu;
    *(ckt->CKTstate0 + inst->BJTa_gm) = inst->BJTgm;
    *(ckt->CKTstate0 + inst->BJTa_go) = inst->BJTgo;
    *(ckt->CKTstate0 + inst->BJTa_gx) = inst->BJTgx;
    *(ckt->CKTstate0 + inst->BJTa_geqcb) = inst->BJTgeqcb;
    *(ckt->CKTstate0 + inst->BJTa_gccs) = inst->BJTgccs;
    *(ckt->CKTstate0 + inst->BJTa_geqbx) = inst->BJTgeqbx;
    *(ckt->CKTstate0 + inst->BJTa_capbe) = inst->BJTcapbe;
    *(ckt->CKTstate0 + inst->BJTa_capbc) = inst->BJTcapbc;
    *(ckt->CKTstate0 + inst->BJTa_capbx) = inst->BJTcapbx;
    *(ckt->CKTstate0 + inst->BJTa_capcs) = inst->BJTcapcs;

    //
    //  load current excitation vector
    //
    double ceqcs;
    double ceqbx;
    double ceqbe;
    double ceqbc;
    if (model->BJTtype > 0) {
        ceqcs = (*(ckt->CKTstate0 + inst->BJTcqcs) - bj_vcs*gccs);
        ceqbx = (*(ckt->CKTstate0 + inst->BJTcqbx) - bj_vbx*geqbx);
        ceqbe = (cc + cb -
                bj_vbe*(gm + go + gpi) +
                bj_vbc*(go - geqcb));
        ceqbc = (-cc + bj_vbe*(gm + go) -
                bj_vbc*(gmu + go));
    }
    else {
        ceqcs = -(*(ckt->CKTstate0 + inst->BJTcqcs) - bj_vcs*gccs);
        ceqbx = -(*(ckt->CKTstate0 + inst->BJTcqbx) - bj_vbx*geqbx);
        ceqbe = -(cc + cb -
                bj_vbe*(gm + go + gpi) +
                bj_vbc*(go - geqcb));
        ceqbc = -(-cc + bj_vbe*(gm + go) -
                bj_vbc*(gmu + go));
    }

    ckt->rhsadd(inst->BJTbaseNode,          -ceqbx);
    ckt->rhsadd(inst->BJTcolPrimeNode,      ceqcs + ceqbx + ceqbc);
    ckt->rhsadd(inst->BJTbasePrimeNode,     -(ceqbe + ceqbc));
    ckt->rhsadd(inst->BJTemitPrimeNode,     ceqbe);
    ckt->rhsadd(inst->BJTsubstNode,         -ceqcs);

    //
    //  load y matrix
    //
    ckt->ldadd(inst->BJTcolColPtr,             gcpr);
    ckt->ldadd(inst->BJTbaseBasePtr,           gx + geqbx);
    ckt->ldadd(inst->BJTemitEmitPtr,           gepr);
    ckt->ldadd(inst->BJTcolPrimeColPrimePtr,   gmu + go + gcpr + gccs + geqbx);
    ckt->ldadd(inst->BJTbasePrimeBasePrimePtr, gx + gpi + gmu + geqcb);
    ckt->ldadd(inst->BJTemitPrimeEmitPrimePtr, gpi + gepr + gm + go);
    ckt->ldadd(inst->BJTcolColPrimePtr,        -gcpr);
    ckt->ldadd(inst->BJTbaseBasePrimePtr,      -gx);
    ckt->ldadd(inst->BJTemitEmitPrimePtr,      -gepr);
    ckt->ldadd(inst->BJTcolPrimeColPtr,        -gcpr);
    ckt->ldadd(inst->BJTcolPrimeBasePrimePtr,  -gmu + gm);
    ckt->ldadd(inst->BJTcolPrimeEmitPrimePtr,  -(gm + go));
    ckt->ldadd(inst->BJTbasePrimeBasePtr,      -gx);
    ckt->ldadd(inst->BJTbasePrimeColPrimePtr,  -(gmu + geqcb));
    ckt->ldadd(inst->BJTbasePrimeEmitPrimePtr, -gpi);
    ckt->ldadd(inst->BJTemitPrimeEmitPtr,      -gepr);
    ckt->ldadd(inst->BJTemitPrimeColPrimePtr,  -go + geqcb);
    ckt->ldadd(inst->BJTemitPrimeBasePrimePtr, -(gpi + gm + geqcb));
    ckt->ldadd(inst->BJTsubstSubstPtr,         gccs);
    ckt->ldadd(inst->BJTcolPrimeSubstPtr,      -gccs);
    ckt->ldadd(inst->BJTsubstColPrimePtr,      -gccs);
    ckt->ldadd(inst->BJTbaseColPrimePtr,       -geqbx);
    ckt->ldadd(inst->BJTcolPrimeBasePtr,       -geqbx);
}

