
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
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
/********** new in 3f2
Sydney University mods Copyright(c) 1989 Anthony E. Parker, David J. Skellern
    Laboratory for Communication Science Engineering
    Sydney University Department of Electrical Engineering, Australia
**********/

#include "jfetdefs.h"


int
JFETdev::load(sGENinstance *in_inst, sCKT *ckt)
{
    sTASK *tsk = ckt->CKTcurTask;
    sJFETinstance *inst = (sJFETinstance*)in_inst;
    sJFETmodel *model = (sJFETmodel*)inst->GENmodPtr;

    //
    //  dc model parameters 
    //
    double beta = model->JFETbeta * inst->JFETarea;
    double gdpr = model->JFETdrainConduct*inst->JFETarea;
    double gspr = model->JFETsourceConduct*inst->JFETarea;
    double csat = inst->JFETtSatCur*inst->JFETarea;
    //
    //    initialization
    //
    double capgd;
    double capgs;
    double cdrain;

    double vds;
    double vgd;
    double vgs;
    double cg;
    double cd;
    double cgd;
    double gm;
    double gds;
    double ggs;
    double ggd;
    int icheck = 1;
    if( ckt->CKTmode & MODEINITSMSIG) {
        vgs= *(ckt->CKTstate0 + inst->JFETvgs);
        vgd= *(ckt->CKTstate0 + inst->JFETvgd);
    }
    else if (ckt->CKTmode & MODEINITTRAN) {
        vgs = *(ckt->CKTstate1 + inst->JFETvgs);
        vgd = *(ckt->CKTstate1 + inst->JFETvgd);
    }
    else if ( (ckt->CKTmode & MODEINITJCT) &&
            (ckt->CKTmode & MODETRANOP) &&
            (ckt->CKTmode & MODEUIC) ) {
        vds = model->JFETtype*inst->JFETicVDS;
        vgs = model->JFETtype*inst->JFETicVGS;
        vgd = vgs-vds;
    }
    else if ( (ckt->CKTmode & MODEINITJCT) && (inst->JFEToff == 0) ) {
        vgs = -1;
        vgd = -1;
    }
    else if( (ckt->CKTmode & MODEINITJCT) ||
            ((ckt->CKTmode & MODEINITFIX) && (inst->JFEToff))) {
        vgs = 0;
        vgd = 0;
    }
    else {
        if(ckt->CKTmode & MODEINITPRED) {
            vgs = DEV.pred(ckt, inst->JFETvgs);
            vgd = DEV.pred(ckt, inst->JFETvgd);

            *(ckt->CKTstate0 + inst->JFETvgs) = 
                *(ckt->CKTstate1 + inst->JFETvgs);
            *(ckt->CKTstate0 + inst->JFETvgd) = 
                *(ckt->CKTstate1 + inst->JFETvgd);
            *(ckt->CKTstate0 + inst->JFETcg) = 
                *(ckt->CKTstate1 + inst->JFETcg);
            *(ckt->CKTstate0 + inst->JFETcd) = 
                *(ckt->CKTstate1 + inst->JFETcd);
            *(ckt->CKTstate0 + inst->JFETcgd) =
                *(ckt->CKTstate1 + inst->JFETcgd);
            *(ckt->CKTstate0 + inst->JFETgm) =
                *(ckt->CKTstate1 + inst->JFETgm);
            *(ckt->CKTstate0 + inst->JFETgds) =
                *(ckt->CKTstate1 + inst->JFETgds);
            *(ckt->CKTstate0 + inst->JFETggs) =
                *(ckt->CKTstate1 + inst->JFETggs);
            *(ckt->CKTstate0 + inst->JFETggd) =
                *(ckt->CKTstate1 + inst->JFETggd);
        }
        else {
            //
            //  compute new nonlinear branch voltages 
            //
            vgs = model->JFETtype*
                (*(ckt->CKTrhsOld + inst->JFETgateNode) -
                *(ckt->CKTrhsOld + inst->JFETsourcePrimeNode));
            vgd = model->JFETtype*
                (*(ckt->CKTrhsOld + inst->JFETgateNode) -
                *(ckt->CKTrhsOld + inst->JFETdrainPrimeNode));
        }
        double delvgs = vgs- *(ckt->CKTstate0 + inst->JFETvgs);
        double delvgd = vgd- *(ckt->CKTstate0 + inst->JFETvgd);
        double delvds = delvgs-delvgd;
        double cghat = *(ckt->CKTstate0 + inst->JFETcg)+ 
                *(ckt->CKTstate0 + inst->JFETggd)*delvgd+
                *(ckt->CKTstate0 + inst->JFETggs)*delvgs;
        double cdhat = *(ckt->CKTstate0 + inst->JFETcd)+
                *(ckt->CKTstate0 + inst->JFETgm)*delvgs+
                *(ckt->CKTstate0 + inst->JFETgds)*delvds-
                *(ckt->CKTstate0 + inst->JFETggd)*delvgd;
        //
        //   bypass if solution has not changed 
        //
        if ((tsk->TSKbypass) &&
            (!(ckt->CKTmode & MODEINITPRED)) &&
            (FABS(delvgs) < tsk->TSKreltol*SPMAX(FABS(vgs),
                    FABS(*(ckt->CKTstate0 + inst->JFETvgs)))+
                    tsk->TSKvoltTol) )
                if ( (FABS(delvgd) < tsk->TSKreltol*SPMAX(FABS(vgd),
                        FABS(*(ckt->CKTstate0 + inst->JFETvgd)))+
                        tsk->TSKvoltTol))
                    if ( (FABS(cghat-*(ckt->CKTstate0 + inst->JFETcg)) 
                            < tsk->TSKreltol*SPMAX(FABS(cghat),
                            FABS(*(ckt->CKTstate0 + inst->JFETcg)))+
                            tsk->TSKabstol) )
                        if ( (FABS(cdhat-*(ckt->CKTstate0 +
                                inst->JFETcd))
                                < tsk->TSKreltol*SPMAX(FABS(cdhat),
                                FABS(*(ckt->CKTstate0 + inst->JFETcd)))+
                                tsk->TSKabstol) ) {

                            // we can do a bypass
                            vgs= *(ckt->CKTstate0 + inst->JFETvgs);
                            vgd= *(ckt->CKTstate0 + inst->JFETvgd);
                            vds= vgs-vgd;
                            cg= *(ckt->CKTstate0 + inst->JFETcg);
                            cd= *(ckt->CKTstate0 + inst->JFETcd);
                            cgd= *(ckt->CKTstate0 + inst->JFETcgd);
                            gm= *(ckt->CKTstate0 + inst->JFETgm);
                            gds= *(ckt->CKTstate0 + inst->JFETgds);
                            ggs= *(ckt->CKTstate0 + inst->JFETggs);
                            ggd= *(ckt->CKTstate0 + inst->JFETggd);
                            goto load;
                        }
        //
        //  limit nonlinear branch voltages 
        //
        int ichk1 = 1;
        vgs = DEV.pnjlim(vgs,*(ckt->CKTstate0 + inst->JFETvgs),
            (inst->JFETtemp*CONSTKoverQ), inst->JFETvcrit, &icheck);
        vgd = DEV.pnjlim(vgd,*(ckt->CKTstate0 + inst->JFETvgd),
            (inst->JFETtemp*CONSTKoverQ), inst->JFETvcrit,&ichk1);
        if (ichk1 == 1)
            icheck = 1;
        vgs = DEV.fetlim(vgs,*(ckt->CKTstate0 + inst->JFETvgs),
            model->JFETthreshold);
        vgd = DEV.fetlim(vgd,*(ckt->CKTstate0 + inst->JFETvgd),
            model->JFETthreshold);
    }
    //
    //   determine dc current and derivatives 
    //
    vds = vgs-vgd;
    if (vgs <= -5*inst->JFETtemp*CONSTKoverQ) {
        ggs = -csat/vgs+tsk->TSKgmin;
        cg = ggs*vgs;
    }
    else {
        double evgs = exp(vgs/(inst->JFETtemp*CONSTKoverQ));
        ggs = csat*evgs/(inst->JFETtemp*CONSTKoverQ)+tsk->TSKgmin;
        cg = csat*(evgs-1)+tsk->TSKgmin*vgs;
    }
    if (vgd <= -5*(inst->JFETtemp*CONSTKoverQ)) {
        ggd = -csat/vgd+tsk->TSKgmin;
        cgd = ggd*vgd;
    }
    else {
        double evgd = exp(vgd/(inst->JFETtemp*CONSTKoverQ));
        ggd = csat*evgd/(inst->JFETtemp*CONSTKoverQ)+tsk->TSKgmin;
        cgd = csat*(evgd-1)+tsk->TSKgmin*vgd;
    }
    cg = cg+cgd;

    // Modification for Sydney University JFET model
    double vto;
    vto = model->JFETthreshold;
    if (vds >= 0) {
        double vgst = vgs - vto;
        //
        // compute drain current and derivatives for normal mode
        //
        if (vgst <= 0) {
            //
            // normal mode, cutoff region
            //
            cdrain = 0;
            gm = 0;
            gds = 0;
        }
        else {
            double betap = beta*(1 + model->JFETlModulation*vds);
            double Bfac = model->JFETbFac;
            if (vgst >= vds) {
                //
                // normal mode, linear region
                //
                double apart = 2*model->JFETb + 3*Bfac*(vgst - vds);
                double cpart = vds*(vds*(Bfac*vds - model->JFETb) +
                    vgst*apart);
                cdrain = betap*cpart;
                gm = betap*vds*(apart + 3*Bfac*vgst);
                gds = betap*(vgst - vds)*apart
                      + beta*model->JFETlModulation*cpart;
            }
            else {
                Bfac = vgst*Bfac;
                gm = betap*vgst*(2*model->JFETb+3*Bfac);
                //
                // normal mode, saturation region
                //
                double cpart = vgst*vgst*(model->JFETb+Bfac);
                cdrain = betap*cpart;
                gds = model->JFETlModulation*beta*cpart;
            }
        }
    }
    else {
        double vgdt = vgd - vto;
        //
        // compute drain current and derivatives for inverse mode
        //
        if (vgdt <= 0) {
            //
            // inverse mode, cutoff region
            //
            cdrain = 0;
            gm = 0;
            gds = 0;
        }
        else {
            double betap = beta*(1 - model->JFETlModulation*vds);
            double Bfac = model->JFETbFac;
            if (vgdt + vds >= 0) {
                //
                // inverse mode, linear region
                //
                double apart = 2*model->JFETb + 3*Bfac*(vgdt + vds);
                double cpart = vds*(-vds*(-Bfac*vds-model->JFETb) +
                    vgdt*apart);
                cdrain = betap*cpart;
                gm = betap*vds*(apart + 3*Bfac*vgdt);
                gds = betap*(vgdt + vds)*apart
                     - beta*model->JFETlModulation*cpart - gm;
            }
            else {
                Bfac = vgdt*Bfac;
                gm = -betap*vgdt*(2*model->JFETb+3*Bfac);
                //
                // inverse mode, saturation region
                //
                double cpart = vgdt*vgdt*(model->JFETb+Bfac);
                cdrain = - betap*cpart;
                gds = model->JFETlModulation*beta*cpart-gm;
            }
        }
    }
    // end Sydney University mod
    //
    //   compute equivalent drain current source 
    //
    cd = cdrain-cgd;
    if ( (ckt->CKTmode & (MODETRAN | MODEAC | MODEINITSMSIG) ) ||
            ((ckt->CKTmode & MODETRANOP) && (ckt->CKTmode & MODEUIC)) ){
        // 
        //    charge storage elements 
        //
        double czgs = inst->JFETtCGS*inst->JFETarea;
        double czgd = inst->JFETtCGD*inst->JFETarea;
        double twop = inst->JFETtGatePot+inst->JFETtGatePot;
        double fcpb2 = inst->JFETcorDepCap*inst->JFETcorDepCap;
        double czgsf2 = czgs/model->JFETf2;
        double czgdf2 = czgd/model->JFETf2;
        if (vgs < inst->JFETcorDepCap) {
            double sarg = sqrt(1-vgs/inst->JFETtGatePot);
            *(ckt->CKTstate0 + inst->JFETqgs) = twop*czgs*(1-sarg);
            capgs = czgs/sarg;
        }
        else {
            *(ckt->CKTstate0 + inst->JFETqgs) = czgs*inst->JFETf1 +
                    czgsf2*(model->JFETf3 *(vgs-
                    inst->JFETcorDepCap)+(vgs*vgs-fcpb2)/
                    (twop+twop));
            capgs = czgsf2*(model->JFETf3+vgs/twop);
        }
        if (vgd < inst->JFETcorDepCap) {
            double sarg = sqrt(1-vgd/inst->JFETtGatePot);
            *(ckt->CKTstate0 + inst->JFETqgd) = twop*czgd*(1-sarg);
            capgd = czgd/sarg;
        }
        else {
            *(ckt->CKTstate0 + inst->JFETqgd) = czgd*inst->JFETf1+
                    czgdf2*(model->JFETf3* (vgd-
                    inst->JFETcorDepCap)+(vgd*vgd-fcpb2)/
                    (twop+twop));
            capgd = czgdf2*(model->JFETf3+vgd/twop);
        }
        //
        //   store small-signal parameters 
        //
        if( (!(ckt->CKTmode & MODETRANOP)) || 
                (!(ckt->CKTmode & MODEUIC)) ) {
            if(ckt->CKTmode & MODEINITSMSIG) {
                *(ckt->CKTstate0 + inst->JFETqgs) = capgs;
                *(ckt->CKTstate0 + inst->JFETqgd) = capgd;
                return (OK);
            }
            //
            //   transient analysis 
            //
            if(ckt->CKTmode & MODEINITTRAN) {
                *(ckt->CKTstate1 + inst->JFETqgs) =
                        *(ckt->CKTstate0 + inst->JFETqgs);
                *(ckt->CKTstate1 + inst->JFETqgd) =
                        *(ckt->CKTstate0 + inst->JFETqgd);
            }
            double ceq;
            double geq;
            ckt->integrate(&geq, &ceq, capgs, inst->JFETqgs);
            ggs = ggs + geq;
            cg = cg + *(ckt->CKTstate0 + inst->JFETcqgs);
            ckt->integrate(&geq, &ceq, capgd, inst->JFETqgd);
            ggd = ggd + geq;
            cg = cg + *(ckt->CKTstate0 + inst->JFETcqgd);
            cd = cd - *(ckt->CKTstate0 + inst->JFETcqgd);
            cgd = cgd + *(ckt->CKTstate0 + inst->JFETcqgd);
            if (ckt->CKTmode & MODEINITTRAN) {
                *(ckt->CKTstate1 + inst->JFETcqgs) =
                        *(ckt->CKTstate0 + inst->JFETcqgs);
                *(ckt->CKTstate1 + inst->JFETcqgd) =
                        *(ckt->CKTstate0 + inst->JFETcqgd);
            }
        }
    }
    //
    //  check convergence 
    //
    if( (!(ckt->CKTmode & MODEINITFIX)) | (!(ckt->CKTmode & MODEUIC))) {
        if (icheck == 1) {
            ckt->incNoncon();  // SRW
            // new in 3f2
            ckt->CKTtroubleElt = inst;
        }
    }
    *(ckt->CKTstate0 + inst->JFETvgs) = vgs;
    *(ckt->CKTstate0 + inst->JFETvgd) = vgd;
    *(ckt->CKTstate0 + inst->JFETcg) = cg;
    *(ckt->CKTstate0 + inst->JFETcd) = cd;
    *(ckt->CKTstate0 + inst->JFETcgd) = cgd;
    *(ckt->CKTstate0 + inst->JFETgm) = gm;
    *(ckt->CKTstate0 + inst->JFETgds) = gds;
    *(ckt->CKTstate0 + inst->JFETggs) = ggs;
    *(ckt->CKTstate0 + inst->JFETggd) = ggd;
    //
    //    load current vector
    //
load:
    double ceqgd = model->JFETtype*(cgd-ggd*vgd);
    double ceqgs = model->JFETtype*((cg-cgd)-ggs*vgs);
    double cdreq = model->JFETtype*((cd+cgd)-gds*vds-gm*vgs);
    ckt->rhsadd(inst->JFETgateNode, (-ceqgs-ceqgd));
    ckt->rhsadd(inst->JFETdrainPrimeNode,
            (-cdreq+ceqgd));
    ckt->rhsadd(inst->JFETsourcePrimeNode,
            (cdreq+ceqgs));
    //
    //    load y matrix 
    //
    ckt->ldadd(inst->JFETdrainDrainPrimePtr, -gdpr);
    ckt->ldadd(inst->JFETgateDrainPrimePtr, -ggd);
    ckt->ldadd(inst->JFETgateSourcePrimePtr, -ggs);
    ckt->ldadd(inst->JFETsourceSourcePrimePtr, -gspr);
    ckt->ldadd(inst->JFETdrainPrimeDrainPtr, -gdpr);
    ckt->ldadd(inst->JFETdrainPrimeGatePtr, gm-ggd);
    ckt->ldadd(inst->JFETdrainPrimeSourcePrimePtr, -gds-gm);
    ckt->ldadd(inst->JFETsourcePrimeGatePtr, -ggs-gm);
    ckt->ldadd(inst->JFETsourcePrimeSourcePtr, -gspr);
    ckt->ldadd(inst->JFETsourcePrimeDrainPrimePtr, -gds);
    ckt->ldadd(inst->JFETdrainDrainPtr, gdpr);
    ckt->ldadd(inst->JFETgateGatePtr, ggd+ggs);
    ckt->ldadd(inst->JFETsourceSourcePtr, gspr);
    ckt->ldadd(inst->JFETdrainPrimeDrainPrimePtr, gdpr+gds+ggd);
    ckt->ldadd(inst->JFETsourcePrimeSourcePrimePtr, gspr+gds+gm+ggs);
    return (OK);
}
