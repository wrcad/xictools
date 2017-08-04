
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
Authors: 1987 Mathew Lew and Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/
/********** new in 3f2
Sydney University mods Copyright(c) 1989 Anthony E. Parker, David J. Skellern
    Laboratory for Communication Science Engineering
    Sydney University Department of Electrical Engineering, Australia
**********/

#include "jfetdefs.h"


int
JFETdev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sJFETinstance *inst = static_cast<const sJFETinstance*>(geninst);
    sJFETmodel *model = static_cast<sJFETmodel*>(inst->GENmodPtr);

    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case JFET_AREA:
        data->v.rValue = inst->JFETarea;
        break;
    case JFET_OFF:
        data->type = IF_FLAG;
        data->v.iValue = inst->JFEToff;
        break;
    case JFET_TEMP:
        data->v.rValue = inst->JFETtemp-CONSTCtoK;
        break;
    case JFET_IC_VDS:
        data->v.rValue = inst->JFETicVDS;
        break;
    case JFET_IC_VGS:
        data->v.rValue = inst->JFETicVGS;
        break;
    case JFET_VGS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->JFETgateNode) -
                ckt->rhsOld(inst->JFETsourcePrimeNode);
            data->v.cValue.imag = ckt->irhsOld(inst->JFETgateNode) -
                ckt->irhsOld(inst->JFETsourcePrimeNode);
        }
        else {
            data->v.rValue = model->JFETtype > 0 ?
                ckt->interp(inst->JFETvgs) : -ckt->interp(inst->JFETvgs);
        }
        break;
    case JFET_VGD:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->JFETgateNode) -
                ckt->rhsOld(inst->JFETdrainPrimeNode);
            data->v.cValue.imag = ckt->irhsOld(inst->JFETgateNode) -
                ckt->irhsOld(inst->JFETdrainPrimeNode);
        }
        else {
            data->v.rValue = model->JFETtype > 0 ?
                ckt->interp(inst->JFETvgd) : -ckt->interp(inst->JFETvgd);
        }
        break;
    case JFET_CG:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cg(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else {
            data->v.rValue = model->JFETtype > 0 ?
                ckt->interp(inst->JFETcg) : -ckt->interp(inst->JFETcg);
        }
        break;
    case JFET_CD:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cd(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else {
            data->v.rValue = model->JFETtype > 0 ?
                ckt->interp(inst->JFETcd) : -ckt->interp(inst->JFETcd);
        }
        break;
    case JFET_CGD:
        data->v.rValue = model->JFETtype > 0 ?
            ckt->interp(inst->JFETcgd) : -ckt->interp(inst->JFETcgd);
        break;
    case JFET_CS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cs(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else {
            data->v.rValue = model->JFETtype > 0 ?
                -ckt->interp(inst->JFETcd) - ckt->interp(inst->JFETcg) :
                ckt->interp(inst->JFETcd) + ckt->interp(inst->JFETcg);
        }
        break;
    case JFET_POWER:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = 0.0;
        else {
            data->v.rValue = ckt->interp(inst->JFETcd) *
                (ckt->rhsOld(inst->JFETdrainNode) -
                ckt->rhsOld(inst->JFETsourceNode));
            data->v.rValue += ckt->interp(inst->JFETcg) * 
                (ckt->rhsOld(inst->JFETgateNode) -
                ckt->rhsOld(inst->JFETsourceNode));
        }
        break;
    case JFET_GM:
        data->v.rValue = ckt->interp(inst->JFETgm);
        break;
    case JFET_GDS:
        data->v.rValue = ckt->interp(inst->JFETgds);
        break;
    case JFET_GGS:
        data->v.rValue = ckt->interp(inst->JFETggs);
        break;
    case JFET_GGD:
        data->v.rValue = ckt->interp(inst->JFETggd);
        break;
    case JFET_QGS:
        data->v.rValue = ckt->interp(inst->JFETqgs);
        break;
    case JFET_QGD:
        data->v.rValue = ckt->interp(inst->JFETqgd);
        break;
    case JFET_CQGS:
        data->v.rValue = ckt->interp(inst->JFETcqgs);
        break;
    case JFET_CQGD:
        data->v.rValue = ckt->interp(inst->JFETcqgd);
        break;
    case JFET_DRAINNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->JFETdrainNode;
        break;
    case JFET_GATENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->JFETgateNode;
        break;
    case JFET_SOURCENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->JFETsourceNode;
        break;
    case JFET_DRAINPRIMENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->JFETdrainPrimeNode;
        break;
    case JFET_SOURCEPRIMENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->JFETsourcePrimeNode;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}


void
sJFETinstance::ac_cd(const sCKT *ckt, double *cdr, double *cdi) const
{
    if (JFETdrainNode != JFETdrainPrimeNode) {
        double gdpr =
            static_cast<const sJFETmodel*>(GENmodPtr)->JFETdrainConduct *
            JFETarea;
        *cdr = gdpr* (ckt->rhsOld(JFETdrainNode) -
            ckt->rhsOld(JFETdrainPrimeNode));
        *cdi = gdpr* (ckt->irhsOld(JFETdrainNode) -
            ckt->irhsOld(JFETdrainPrimeNode));
        return;
    }

    if (!ckt->CKTstates[0]) {
        *cdr = 0.0;
        *cdi = 0.0;
        return;
    }
    double gm  = *(ckt->CKTstate0 + JFETgm);
    double gds = *(ckt->CKTstate0 + JFETgds);
    double ggd = *(ckt->CKTstate0 + JFETggd);
    double xgd = *(ckt->CKTstate0 + JFETqgd) * ckt->CKTomega;

    cIFcomplex Add(gds + ggd, xgd);
    cIFcomplex Adg(-ggd + gm, -xgd);
    cIFcomplex Ads(-gds - gm, 0);

    *cdr = Add.real* ckt->rhsOld(JFETdrainPrimeNode) -
        Add.imag* ckt->irhsOld(JFETdrainPrimeNode) +
        Adg.real* ckt->rhsOld(JFETgateNode) -
        Adg.imag* ckt->irhsOld(JFETgateNode) +
        Ads.real* ckt->rhsOld(JFETsourcePrimeNode);

    *cdi = Add.real* ckt->irhsOld(JFETdrainPrimeNode) +
        Add.imag* ckt->rhsOld(JFETdrainPrimeNode) +
        Adg.real* ckt->irhsOld(JFETgateNode) +
        Adg.imag* ckt->rhsOld(JFETgateNode) +
        Ads.real* ckt->irhsOld(JFETsourcePrimeNode);
}


void
sJFETinstance::ac_cs(const sCKT *ckt, double *csr, double *csi) const
{
    if (JFETsourceNode != JFETsourcePrimeNode) {
        double gspr =
            static_cast<const sJFETmodel*>(GENmodPtr)->JFETdrainConduct *
            JFETarea;
        *csr = gspr* (ckt->rhsOld(JFETsourceNode) -
            ckt->rhsOld(JFETsourcePrimeNode));
        *csi = gspr* (ckt->irhsOld(JFETsourceNode) -
            ckt->irhsOld(JFETsourcePrimeNode));
        return;
    }

    if (!ckt->CKTstates[0]) {
        *csr = 0.0;
        *csi = 0.0;
        return;
    }
    double gm  = *(ckt->CKTstate0 + JFETgm);
    double gds = *(ckt->CKTstate0 + JFETgds);
    double ggs = *(ckt->CKTstate0 + JFETggs);
    double xgs = *(ckt->CKTstate0 + JFETqgs) * ckt->CKTomega;

    cIFcomplex Ass(gds + gm + ggs, xgs);
    cIFcomplex Asg(-ggs - gm, -xgs);
    cIFcomplex Asd(-gds, 0);

    *csr = Ass.real* ckt->rhsOld(JFETsourcePrimeNode) -
        Ass.imag* ckt->irhsOld(JFETsourcePrimeNode) +
        Asg.real* ckt->rhsOld(JFETgateNode) -
        Asg.imag* ckt->irhsOld(JFETgateNode) +
        Asd.real* ckt->rhsOld(JFETdrainPrimeNode);

    *csi = Ass.real* ckt->irhsOld(JFETsourcePrimeNode) +
        Ass.imag* ckt->rhsOld(JFETsourcePrimeNode) +
        Asg.real* ckt->irhsOld(JFETgateNode) +
        Asg.imag* ckt->rhsOld(JFETgateNode) +
        Asd.real* ckt->irhsOld(JFETdrainPrimeNode);
}


void
sJFETinstance::ac_cg(const sCKT *ckt, double *cgr, double *cgi) const
{
    if (!ckt->CKTstates[0]) {
        *cgr = 0.0;
        *cgi = 0.0;
        return;
    }
    double ggd = *(ckt->CKTstate0 + JFETggd);
    double ggs = *(ckt->CKTstate0 + JFETggs);
    double xgd = *(ckt->CKTstate0 + JFETqgd) * ckt->CKTomega;
    double xgs = *(ckt->CKTstate0 + JFETqgs) * ckt->CKTomega;

    cIFcomplex Agg(ggd + ggs, xgd + xgs);
    cIFcomplex Ags(-ggs, -xgs);
    cIFcomplex Agd(-ggd, -xgd);

    *cgr = Agg.real* ckt->rhsOld(JFETgateNode) -
        Agg.imag* ckt->irhsOld(JFETgateNode) +
        Ags.real* ckt->rhsOld(JFETsourcePrimeNode) -
        Ags.imag* ckt->irhsOld(JFETsourcePrimeNode) +
        Agd.real* ckt->rhsOld(JFETdrainPrimeNode) -
        Agd.imag* ckt->irhsOld(JFETdrainPrimeNode);

    *cgi = Agg.real* ckt->irhsOld(JFETgateNode) +
        Agg.imag* ckt->rhsOld(JFETgateNode) +
        Ags.real* ckt->irhsOld(JFETsourcePrimeNode) +
        Ags.imag* ckt->rhsOld(JFETsourcePrimeNode) +
        Agd.real* ckt->irhsOld(JFETdrainPrimeNode) +
        Agd.imag* ckt->rhsOld(JFETdrainPrimeNode);
}

