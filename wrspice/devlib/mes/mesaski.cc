
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
JSPICE3 adaptation of Spice322 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mesdefs.h"


int
MESdev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sMESinstance *inst = static_cast<const sMESinstance*>(geninst);
    const sMESmodel *model = static_cast<const sMESmodel*>(inst->GENmodPtr);

    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case MES_AREA:
        data->v.rValue = inst->MESarea;
        break;
    case MES_OFF:
        data->type = IF_FLAG;
        data->v.iValue = inst->MESoff;
        break;
    case MES_IC_VDS:
        data->v.rValue = inst->MESicVDS;
        break;
    case MES_IC_VGS:
        data->v.rValue = inst->MESicVGS;
        break;
    case MES_VGS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->MESgateNode) -
                ckt->rhsOld(inst->MESsourcePrimeNode);
            data->v.cValue.imag = ckt->irhsOld(inst->MESgateNode) -
                ckt->irhsOld(inst->MESsourcePrimeNode);
        }
        else {
            data->v.rValue = model->MEStype > 0 ?
                ckt->interp(inst->MESvgs) : -ckt->interp(inst->MESvgs);
        }
        break;
    case MES_VGD:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->MESgateNode) -
                ckt->rhsOld(inst->MESdrainPrimeNode);
            data->v.cValue.imag = ckt->irhsOld(inst->MESgateNode) -
                ckt->irhsOld(inst->MESdrainPrimeNode);
        }
        else {
            data->v.rValue = model->MEStype > 0 ?
                ckt->interp(inst->MESvgd) : -ckt->interp(inst->MESvgd);
        }
        break;
    case MES_CG:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cg(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else {
            data->v.rValue = model->MEStype > 0 ?
                ckt->interp(inst->MEScg) : -ckt->interp(inst->MEScg);
        }
        break;
    case MES_CD:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cd(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else {
            data->v.rValue = model->MEStype > 0 ?
                ckt->interp(inst->MEScd) : -ckt->interp(inst->MEScd);
        }
        break;
    case MES_CGD:
        data->v.rValue = model->MEStype > 0 ?
            ckt->interp(inst->MEScgd) : -ckt->interp(inst->MEScgd);
        break;
    case MES_CS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cs(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else {
            data->v.rValue = model->MEStype > 0 ?
                -ckt->interp(inst->MEScd) - ckt->interp(inst->MEScg) :
                ckt->interp(inst->MEScd) + ckt->interp(inst->MEScg);
        }
        break;
    case MES_POWER:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = 0.0;
        else if (ckt->CKTrhsOld) {
            data->v.rValue = ckt->interp(inst->MEScd) *
                (ckt->rhsOld(inst->MESdrainNode) -
                ckt->rhsOld(inst->MESsourceNode));
            data->v.rValue += ckt->interp(inst->MEScg) *
                (ckt->rhsOld(inst->MESgateNode) -
                ckt->rhsOld(inst->MESsourceNode));
        }
        break;
    case MES_GM:
        data->v.rValue = ckt->interp(inst->MESgm);
        break;
    case MES_GDS:
        data->v.rValue = ckt->interp(inst->MESgds);
        break;
    case MES_GGS:
        data->v.rValue = ckt->interp(inst->MESggs);
        break;
    case MES_GGD:
        data->v.rValue = ckt->interp(inst->MESggd);
        break;
    case MES_QGS:
        data->v.rValue = ckt->interp(inst->MESqgs);
        break;
    case MES_QGD:
        data->v.rValue = ckt->interp(inst->MESqgd);
        break;
    case MES_CQGS:
        data->v.rValue = ckt->interp(inst->MEScqgs);
        break;
    case MES_CQGD:
        data->v.rValue = ckt->interp(inst->MEScqgd);
        break;
    case MES_DRAINNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->MESdrainNode;
        break;
    case MES_GATENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->MESgateNode;
        break;
    case MES_SOURCENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->MESsourceNode;
        break;
    case MES_DRAINPRIMENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->MESdrainPrimeNode;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}


void
sMESinstance::ac_cd(const sCKT *ckt, double *cdr, double *cdi) const
{
    if (MESdrainNode != MESdrainPrimeNode) {
        double gdpr =
            static_cast<const sMESmodel*>(GENmodPtr)->MESdrainConduct *
            MESarea;
        *cdr = gdpr* (ckt->rhsOld(MESdrainNode) -
            ckt->rhsOld(MESdrainPrimeNode));
        *cdi = gdpr* (ckt->irhsOld(MESdrainNode) -
            ckt->irhsOld(MESdrainPrimeNode));
        return;
    }

    if (!ckt->CKTstates[0]) {
        *cdr = 0.0;
        *cdi = 0.0;
        return;
    }
    double gm  = *(ckt->CKTstate0 + MESgm);
    double gds = *(ckt->CKTstate0 + MESgds);
    double ggd = *(ckt->CKTstate0 + MESggd);
    double xgd = *(ckt->CKTstate0 + MESqgd) * ckt->CKTomega;

    cIFcomplex Add(gds + ggd, xgd);
    cIFcomplex Adg(-ggd + gm, -xgd);
    cIFcomplex Ads(-gds - gm, 0);

    *cdr = Add.real* ckt->rhsOld(MESdrainPrimeNode) -
        Add.imag* ckt->irhsOld(MESdrainPrimeNode) +
        Adg.real* ckt->rhsOld(MESgateNode) -
        Adg.imag* ckt->irhsOld(MESgateNode) +
        Ads.real* ckt->rhsOld(MESsourcePrimeNode);

    *cdi = Add.real* ckt->irhsOld(MESdrainPrimeNode) +
        Add.imag* ckt->rhsOld(MESdrainPrimeNode) +
        Adg.real* ckt->irhsOld(MESgateNode) +
        Adg.imag* ckt->rhsOld(MESgateNode) +
        Ads.real* ckt->irhsOld(MESsourcePrimeNode);
}


void
sMESinstance::ac_cs(const sCKT *ckt, double *csr, double *csi) const
{
    if (MESsourceNode != MESsourcePrimeNode) {
        double gspr =
            static_cast<const sMESmodel*>(GENmodPtr)->MESdrainConduct *
            MESarea;
        *csr = gspr* (ckt->rhsOld(MESsourceNode) -
            ckt->rhsOld(MESsourcePrimeNode));
        *csi = gspr* (ckt->irhsOld(MESsourceNode) -
            ckt->irhsOld(MESsourcePrimeNode));
        return;
    }

    if (!ckt->CKTstates[0]) {
        *csr = 0.0;
        *csi = 0.0;
        return;
    }
    double gm  = *(ckt->CKTstate0 + MESgm);
    double gds = *(ckt->CKTstate0 + MESgds);
    double ggs = *(ckt->CKTstate0 + MESggs);
    double xgs = *(ckt->CKTstate0 + MESqgs) * ckt->CKTomega;

    cIFcomplex Ass(gds + gm + ggs, xgs);
    cIFcomplex Asg(-ggs - gm, -xgs);
    cIFcomplex Asd(-gds, 0);

    *csr = Ass.real* ckt->rhsOld(MESsourcePrimeNode) -
        Ass.imag* ckt->irhsOld(MESsourcePrimeNode) +
        Asg.real* ckt->rhsOld(MESgateNode) -
        Asg.imag* ckt->irhsOld(MESgateNode) +
        Asd.real* ckt->rhsOld(MESdrainPrimeNode);

    *csi = Ass.real* ckt->irhsOld(MESsourcePrimeNode) +
        Ass.imag* ckt->rhsOld(MESsourcePrimeNode) +
        Asg.real* ckt->irhsOld(MESgateNode) +
        Asg.imag* ckt->rhsOld(MESgateNode) +
        Asd.real* ckt->irhsOld(MESdrainPrimeNode);
}


void
sMESinstance::ac_cg(const sCKT *ckt, double *cgr, double *cgi) const
{
    if (!ckt->CKTstates[0]) {
        *cgr = 0.0;
        *cgi = 0.0;
        return;
    }
    double ggd = *(ckt->CKTstate0 + MESggd);
    double ggs = *(ckt->CKTstate0 + MESggs);
    double xgd = *(ckt->CKTstate0 + MESqgd) * ckt->CKTomega;
    double xgs = *(ckt->CKTstate0 + MESqgs) * ckt->CKTomega;

    cIFcomplex Agg(ggd + ggs, xgd + xgs);
    cIFcomplex Ags(-ggs, -xgs);
    cIFcomplex Agd(-ggd, -xgd);

    *cgr = Agg.real* ckt->rhsOld(MESgateNode) -
        Agg.imag* ckt->irhsOld(MESgateNode) +
        Ags.real* ckt->rhsOld(MESsourcePrimeNode) -
        Ags.imag* ckt->irhsOld(MESsourcePrimeNode) +
        Agd.real* ckt->rhsOld(MESdrainPrimeNode) -
        Agd.imag* ckt->irhsOld(MESdrainPrimeNode);

    *cgi = Agg.real* ckt->irhsOld(MESgateNode) +
        Agg.imag* ckt->rhsOld(MESgateNode) +
        Ags.real* ckt->irhsOld(MESsourcePrimeNode) +
        Ags.imag* ckt->rhsOld(MESsourcePrimeNode) +
        Agd.real* ckt->irhsOld(MESdrainPrimeNode) +
        Agd.imag* ckt->rhsOld(MESdrainPrimeNode);
}

