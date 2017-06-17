
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Device Library                                                         *
 *                                                                        *
 *========================================================================*
 $Id: mosaski.cc,v 1.5 2015/07/26 01:09:13 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Mathew Lew and Thomas L. Quarles
         1989 Takayasu Sakurai
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mosdefs.h"


#define MSC(xx) inst->MOSmult*(xx)
#define tMSC(xx) MOSmult*(xx)

int
MOSdev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sMOSinstance *inst = static_cast<const sMOSinstance*>(geninst);

    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case MOS_M:
        data->v.rValue = inst->MOSmult;
        break;
    case MOS_L:
        data->v.rValue = inst->MOSl;
        break;
    case MOS_W:
        data->v.rValue = inst->MOSw;
        break;
    case MOS_AD:
        data->v.rValue = inst->MOSdrainArea;
        break;
    case MOS_AS:
        data->v.rValue = inst->MOSsourceArea;
        break;
    case MOS_PD:
        data->v.rValue = inst->MOSdrainPerimeter;
        break;
    case MOS_PS:
        data->v.rValue = inst->MOSsourcePerimeter;
        break;
    case MOS_NRD:
        data->v.rValue = inst->MOSdrainSquares;
        break;
    case MOS_NRS:
        data->v.rValue = inst->MOSsourceSquares;
        break;
    case MOS_TEMP:
        data->v.rValue = inst->MOStemp-CONSTCtoK;
        break;
    case MOS_OFF:
        data->type = IF_FLAG;
        data->v.iValue = inst->MOSoff;
        break;
    case MOS_IC_VDS:
        data->v.rValue = inst->MOSicVDS;
        break;
    case MOS_IC_VGS:
        data->v.rValue = inst->MOSicVGS;
        break;
    case MOS_IC_VBS:
        data->v.rValue = inst->MOSicVBS;
        break;
    case MOS_VBD:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->MOSbNode) -
                ckt->rhsOld(inst->MOSdNodePrime);
            data->v.cValue.imag = ckt->irhsOld(inst->MOSbNode) -
                ckt->irhsOld(inst->MOSdNodePrime);
        }
        else {
            sMOSmodel *model = static_cast<sMOSmodel*>(inst->GENmodPtr);
            data->v.rValue = model->MOStype > 0 ?
                ckt->interp(inst->MOSvbd) : -ckt->interp(inst->MOSvbd);
        }
        break;
    case MOS_VBS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->MOSbNode) -
                ckt->rhsOld(inst->MOSsNodePrime);
            data->v.cValue.imag = ckt->irhsOld(inst->MOSbNode) -
                ckt->irhsOld(inst->MOSsNodePrime);
        }
        else {
            sMOSmodel *model = static_cast<sMOSmodel*>(inst->GENmodPtr);
            data->v.rValue = model->MOStype > 0 ?
                ckt->interp(inst->MOSvbs) : -ckt->interp(inst->MOSvbs);
        }
        break;
    case MOS_VGS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->MOSgNode) -
                ckt->rhsOld(inst->MOSsNodePrime);
            data->v.cValue.imag = ckt->irhsOld(inst->MOSgNode) -
                ckt->irhsOld(inst->MOSsNodePrime);
        }
        else {
            sMOSmodel *model = static_cast<sMOSmodel*>(inst->GENmodPtr);
            data->v.rValue = model->MOStype > 0 ?
                ckt->interp(inst->MOSvgs) : -ckt->interp(inst->MOSvgs);
        }
        break;
    case MOS_VDS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->MOSdNodePrime) -
                ckt->rhsOld(inst->MOSsNodePrime);
            data->v.cValue.imag = ckt->irhsOld(inst->MOSdNodePrime) -
                ckt->irhsOld(inst->MOSsNodePrime);
        }
        else {
            sMOSmodel *model = static_cast<sMOSmodel*>(inst->GENmodPtr);
            data->v.rValue = model->MOStype > 0 ?
                ckt->interp(inst->MOSvds) : -ckt->interp(inst->MOSvds);
        }
        break;
    case MOS_VON:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = inst->MOSvon;
        else
            data->v.rValue = ckt->interp(inst->MOSa_von);
        break;
    case MOS_VDSAT:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = inst->MOSvdsat;
        else
            data->v.rValue = ckt->interp(inst->MOSa_vdsat);
        break;
    case MOS_DRAINVCRIT:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = inst->MOSdrainVcrit;
        else
            data->v.rValue = ckt->interp(inst->MOSa_dVcrit);
        break;
    case MOS_SOURCEVCRIT:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = inst->MOSsourceVcrit;
        else
            data->v.rValue = ckt->interp(inst->MOSa_sVcrit);
        break;
    case MOS_CD:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cd(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else
            data->v.rValue = ckt->interp(inst->MOSa_cd);
        break;
    case MOS_CBD:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = MSC(inst->MOScbd);
        else
            data->v.rValue = ckt->interp(inst->MOSa_cbd);
        break;
    case MOS_CBS:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = MSC(inst->MOScbs);
        else
            data->v.rValue = ckt->interp(inst->MOSa_cbs);
        break;
    case MOS_CG:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cg(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else {
            data->v.rValue = MSC(
                ckt->interp(inst->MOScqgb) +
                ckt->interp(inst->MOScqgd) +
                ckt->interp(inst->MOScqgs));
        }
        break;
    case MOS_CS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cs(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else {
            data->v.rValue =
                -ckt->interp(inst->MOSa_cd) -
                ckt->interp(inst->MOSa_cbd) -
                ckt->interp(inst->MOSa_cbs) -
                MSC(ckt->interp(inst->MOScqgd) + 
                    ckt->interp(inst->MOScqgs));
        }
        break;
    case MOS_CB:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cb(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else {
            data->v.rValue =
                ckt->interp(inst->MOSa_cbd) +
                ckt->interp(inst->MOSa_cbs) -
                MSC(ckt->interp(inst->MOScqgb));
        }
        break;
    case MOS_POWER:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = 0.0;
        else  {
            data->v.rValue = ckt->interp(inst->MOSa_cd)* 
                (ckt->rhsOld(inst->MOSdNode) -
                ckt->rhsOld(inst->MOSsNode));
            data->v.rValue += (ckt->interp(inst->MOSa_cbd) +
                ckt->interp(inst->MOSa_cbs) -
                MSC(ckt->interp(inst->MOScqgb))) * 
                (ckt->rhsOld(inst->MOSbNode) -
                ckt->rhsOld(inst->MOSsNode));
            data->v.rValue += (MSC(ckt->interp(inst->MOScqgb) + 
                ckt->interp(inst->MOScqgd) + 
                ckt->interp(inst->MOScqgs))) *
                (ckt->rhsOld(inst->MOSgNode) -
                ckt->rhsOld(inst->MOSsNode));
        }
        break;
    case MOS_DRAINRES:
        if (inst->MOSdNodePrime != inst->MOSdNode &&
                inst->MOSdrainConductance != 0)
            data->v.rValue = 1.0 / (MSC(inst->MOSdrainConductance));
        else
            data->v.rValue = 0.0;
        break;
    case MOS_SOURCERES:
        if (inst->MOSsNodePrime != inst->MOSsNode &&
                inst->MOSsourceConductance != 0)
            data->v.rValue = 1.0 / (MSC(inst->MOSsourceConductance));
        else
            data->v.rValue = 0.0;
        break;
    case MOS_DRAINCOND:
        data->v.rValue = MSC(inst->MOSdrainConductance);
        break;
    case MOS_SOURCECOND:
        data->v.rValue = MSC(inst->MOSsourceConductance);
        break;
    case MOS_GMBS:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = MSC(inst->MOSgmbs);
        else
            data->v.rValue = ckt->interp(inst->MOSa_gmbs);
        break;
    case MOS_GM:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = MSC(inst->MOSgm);
        else
            data->v.rValue = ckt->interp(inst->MOSa_gm);
        break;
    case MOS_GDS:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = MSC(inst->MOSgds);
        else
            data->v.rValue = ckt->interp(inst->MOSa_gds);
        break;
    case MOS_GBD:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = MSC(inst->MOSgbd);
        else
            data->v.rValue = ckt->interp(inst->MOSa_gbd);
        break;
    case MOS_GBS:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = MSC(inst->MOSgbs);
        else
            data->v.rValue = ckt->interp(inst->MOSa_gbs);
        break;
    case MOS_QGD:
        data->v.rValue = MSC(ckt->interp(inst->MOSqgd));
        break;
    case MOS_QGS:
        data->v.rValue = MSC(ckt->interp(inst->MOSqgs));
        break;
    case MOS_QGB:
        data->v.rValue = MSC(ckt->interp(inst->MOSqgb));
        break;
    case MOS_QBD:
        data->v.rValue = MSC(ckt->interp(inst->MOSqbd));
        break;
    case MOS_QBS:
        data->v.rValue = MSC(ckt->interp(inst->MOSqbs));
        break;
    case MOS_CAPBD:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = MSC(inst->MOScapbd);
        else
            data->v.rValue = ckt->interp(inst->MOSa_capbd);
        break;
    case MOS_CAPBS:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = MSC(inst->MOScapbs);
        else
            data->v.rValue = ckt->interp(inst->MOSa_capbs);
        break;
    case MOS_CAPZBBD:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = MSC(inst->MOSCbd);
        else
            data->v.rValue = ckt->interp(inst->MOSa_Cbd);
        break;
    case MOS_CAPZBBDSW:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = MSC(inst->MOSCbdsw);
        else
            data->v.rValue = ckt->interp(inst->MOSa_Cbdsw);
        break;
    case MOS_CAPZBBS:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = MSC(inst->MOSCbs);
        else
            data->v.rValue = ckt->interp(inst->MOSa_Cbs);
        break;
    case MOS_CAPZBBSSW:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = MSC(inst->MOSCbssw);
        else
            data->v.rValue = ckt->interp(inst->MOSa_Cbssw);
        break;
    case MOS_CAPGD:
        data->v.rValue = MSC(ckt->interp(inst->MOScapgd));
        break;
    case MOS_CQGD:
// NGspice
        data->v.rValue = 2*ckt->interp(inst->MOScqgd);
        // add overlap capacitance
        {
            sMOSmodel *model = static_cast<sMOSmodel*>(inst->GENmodPtr);
            data->v.rValue += (model->MOSgateSourceOverlapCapFactor) *
                inst->MOSw;
        }
        data->v.rValue = MSC(data->v.rValue);
/*
        data->v.rValue = MSC(ckt->interp(inst->MOScqgd));
*/

        break;
    case MOS_CAPGS:
// NGspice
        data->v.rValue = 2*ckt->interp(inst->MOScapgs);
        // add overlap capacitance
        {
            sMOSmodel *model = static_cast<sMOSmodel*>(inst->GENmodPtr);
            data->v.rValue += (model->MOSgateSourceOverlapCapFactor) *
                inst->MOSw;
        }
        data->v.rValue = MSC(data->v.rValue);
/*
        data->v.rValue = MSC(ckt->interp(inst->MOScapgs));
*/
        break;
    case MOS_CQGS:
        data->v.rValue = MSC(ckt->interp(inst->MOScqgs));
        break;
    case MOS_CAPGB:
// NGspice
        data->v.rValue = 2*ckt->interp(inst->MOScapgb);
        // add overlap capacitance
        {
            sMOSmodel *model = static_cast<sMOSmodel*>(inst->GENmodPtr);
            data->v.rValue += (model->MOSgateBulkOverlapCapFactor) *
                (inst->MOSl - 2*model->MOSlatDiff);
        }
        data->v.rValue = MSC(data->v.rValue);
/*
        data->v.rValue = MSC(ckt->interp(inst->MOScapgb));
*/
        break;
    case MOS_CQGB:
        data->v.rValue = MSC(ckt->interp(inst->MOScqgb));
        break;
    case MOS_CQBD:
        data->v.rValue = MSC(ckt->interp(inst->MOScqbd));
        break;
    case MOS_CQBS:
        data->v.rValue = MSC(ckt->interp(inst->MOScqbs));
        break;
    case MOS_DNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->MOSdNode;
        break;
    case MOS_GNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->MOSgNode;
        break;
    case MOS_SNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->MOSsNode;
        break;
    case MOS_BNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->MOSbNode;
        break;
    case MOS_DNODEPRIME:
        data->type = IF_INTEGER;
        data->v.iValue = inst->MOSdNodePrime;
        break;
    case MOS_SNODEPRIME:
        data->type = IF_INTEGER;
        data->v.iValue = inst->MOSsNodePrime;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}


void
sMOSinstance::ac_cd(const sCKT *ckt, double *cdr, double *cdi) const
{
    if (MOSdNode != MOSdNodePrime) {
        double dcon = tMSC(MOSdrainConductance);
        *cdr = dcon* (ckt->rhsOld(MOSdNode) - ckt->rhsOld(MOSdNodePrime));
        *cdi = dcon* (ckt->irhsOld(MOSdNode) - ckt->irhsOld(MOSdNodePrime));
        return;
    }

    if (!ckt->CKTstates[0]) {
        *cdr = 0.0;
        *cdi = 0.0;
        return;
    }
    double xgd = *(ckt->CKTstate0 + MOScapgd) + 
            *(ckt->CKTstate0 + MOScapgd) + MOSgateDrainOverlapCap;
    xgd *= ckt->CKTomega;
    double xbd = MOScapbd * ckt->CKTomega;

    cIFcomplex Add, Adg, Ads, Adb;
    if (MOSmode > 0) {
        Add = cIFcomplex(MOSgds + MOSgbd, xgd + xbd);
        Adg = cIFcomplex(MOSgm, -xgd);
        Ads = cIFcomplex(-MOSgds - MOSgm - MOSgmbs, 0);
        Adb = cIFcomplex(-MOSgbd + MOSgmbs, -xbd);
    }
    else {
        Add = cIFcomplex(MOSgds + MOSgbd + MOSgm + MOSgmbs, xgd + xbd);
        Adg = cIFcomplex(-MOSgm, -xgd);
        Ads = cIFcomplex(-MOSgds, 0);
        Adb = cIFcomplex(-MOSgbd - MOSgmbs, -xbd);
    }
    *cdr = tMSC(Add.real* ckt->rhsOld(MOSdNodePrime) -
        Add.imag* ckt->irhsOld(MOSdNodePrime) +
        Adg.real* ckt->rhsOld(MOSgNode) -
        Adg.imag* ckt->irhsOld(MOSgNode) +
        Ads.real* ckt->rhsOld(MOSsNodePrime) -
        Ads.imag* ckt->irhsOld(MOSsNodePrime) +
        Adb.real* ckt->rhsOld(MOSbNode) -
        Adb.imag* ckt->irhsOld(MOSbNode));

    *cdi = tMSC(Add.real* ckt->irhsOld(MOSdNodePrime) +
        Add.imag* ckt->rhsOld(MOSdNodePrime) +
        Adg.real* ckt->irhsOld(MOSgNode) +
        Adg.imag* ckt->rhsOld(MOSgNode) +
        Ads.real* ckt->irhsOld(MOSsNodePrime) +
        Ads.imag* ckt->rhsOld(MOSsNodePrime) +
        Adb.real* ckt->irhsOld(MOSbNode) +
        Adb.imag* ckt->rhsOld(MOSbNode));
}


void
sMOSinstance::ac_cs(const sCKT *ckt, double *csr, double *csi) const
{
    if (MOSsNode != MOSsNodePrime) {
        double scon = tMSC(MOSsourceConductance);
        *csr = scon* (ckt->rhsOld(MOSsNode) - ckt->rhsOld(MOSsNodePrime));
        *csi = scon* (ckt->irhsOld(MOSsNode) - ckt->irhsOld(MOSsNodePrime));
        return;
    }

    if (!ckt->CKTstates[0]) {
        *csr = 0.0;
        *csi = 0.0;
        return;
    }
    double xgs = *(ckt->CKTstate0 + MOScapgs) + 
            *(ckt->CKTstate0 + MOScapgs) + MOSgateSourceOverlapCap;
    xgs *= ckt->CKTomega;
    double xbs = MOScapbs * ckt->CKTomega;

    cIFcomplex Ass, Asg, Asd, Asb;
    if (MOSmode > 0) {
        Ass = cIFcomplex(MOSgds + MOSgbs + MOSgm + MOSgmbs, xgs + xbs);
        Asg = cIFcomplex(-MOSgm, -xgs);
        Asd = cIFcomplex(-MOSgds, 0);
        Asb = cIFcomplex(-MOSgbs - MOSgmbs, -xbs);
    }
    else {
        Ass = cIFcomplex(MOSgds + MOSgbs, xgs + xbs);
        Asg = cIFcomplex(MOSgm, -xgs);
        Asd = cIFcomplex(-MOSgds - MOSgm - MOSgmbs, 0);
        Asb = cIFcomplex(-MOSgbs + MOSgmbs, -xbs);
    }
    *csr = tMSC(Ass.real* ckt->rhsOld(MOSsNodePrime) -
        Ass.imag* ckt->irhsOld(MOSsNodePrime) +
        Asg.real* ckt->rhsOld(MOSgNode) -
        Asg.imag* ckt->irhsOld(MOSgNode) +
        Asd.real* ckt->rhsOld(MOSdNodePrime) -
        Asd.imag* ckt->irhsOld(MOSdNodePrime) +
        Asb.real* ckt->rhsOld(MOSbNode) -
        Asb.imag* ckt->irhsOld(MOSbNode));

    *csi = tMSC(Ass.real* ckt->irhsOld(MOSsNodePrime) +
        Ass.imag* ckt->rhsOld(MOSsNodePrime) +
        Asg.real* ckt->irhsOld(MOSgNode) +
        Asg.imag* ckt->rhsOld(MOSgNode) +
        Asd.real* ckt->irhsOld(MOSdNodePrime) +
        Asd.imag* ckt->rhsOld(MOSdNodePrime) +
        Asb.real* ckt->irhsOld(MOSbNode) +
        Asb.imag* ckt->rhsOld(MOSbNode));
}


void
sMOSinstance::ac_cg(const sCKT *ckt, double *cgr, double *cgi) const
{
    if (!ckt->CKTstates[0]) {
        *cgr = 0.0;
        *cgi = 0.0;
        return;
    }
    double xgs = *(ckt->CKTstate0 + MOScapgs) + *(ckt->CKTstate0 + MOScapgs) +
            MOSgateSourceOverlapCap;
    double xgd = *(ckt->CKTstate0 + MOScapgd) + *(ckt->CKTstate0 + MOScapgd) +
            MOSgateDrainOverlapCap;
    double xgb = *(ckt->CKTstate0 + MOScapgb) + *(ckt->CKTstate0 + MOScapgb) +
            MOSgateBulkOverlapCap;

    xgs *= ckt->CKTomega;
    xgd *= ckt->CKTomega;
    xgb *= ckt->CKTomega;

    /*
    cIFcomplex Agg(0, xgd + xgs + xgb);
    cIFcomplex Ags(0, -xgs);
    cIFcomplex Ags(0, -xgd);
    cIFcomplex Agb(0, -xgb);
    */

    *cgr = tMSC(
        -xgd*(ckt->irhsOld(MOSgNode) - ckt->irhsOld(MOSdNode)) -
        xgs*(ckt->irhsOld(MOSgNode) - ckt->irhsOld(MOSsNode)) -
        xgb*(ckt->irhsOld(MOSgNode) - ckt->irhsOld(MOSbNode)));

    *cgi = tMSC(
        xgd*(ckt->rhsOld(MOSgNode) - ckt->rhsOld(MOSdNode)) +
        xgs*(ckt->rhsOld(MOSgNode) - ckt->rhsOld(MOSsNode)) +
        xgb*(ckt->rhsOld(MOSgNode) - ckt->rhsOld(MOSbNode)));
}


void
sMOSinstance::ac_cb(const sCKT *ckt, double *cbr, double *cbi) const
{
    if (!ckt->CKTstates[0]) {
        *cbr = 0.0;
        *cbi = 0.0;
        return;
    }
    double xgb = *(ckt->CKTstate0 + MOScapgb) + *(ckt->CKTstate0 + MOScapgb) +
            MOSgateBulkOverlapCap;
    xgb *= ckt->CKTomega;
    double xbd = MOScapbd * ckt->CKTomega;
    double xbs = MOScapbs * ckt->CKTomega;

    /*
    cIFcomplex Abb(0, xgb + xbd + xbs);
    cIFcomplex Abs(0, -xbs);
    cIFcomplex Abd(0, -xbd);
    cIFcomplex Abg(0, -xgb);
    */

    *cbr = tMSC(
        -xgb*(ckt->irhsOld(MOSbNode) - ckt->irhsOld(MOSgNode)) -
        xbs*(ckt->irhsOld(MOSbNode) - ckt->irhsOld(MOSsNode)) -
        xbd*(ckt->irhsOld(MOSbNode) - ckt->irhsOld(MOSdNode)));

    *cbi = tMSC(
        xgb*(ckt->rhsOld(MOSbNode) - ckt->rhsOld(MOSgNode)) +
        xbs*(ckt->rhsOld(MOSbNode) - ckt->rhsOld(MOSsNode)) +
        xbd*(ckt->rhsOld(MOSbNode) - ckt->rhsOld(MOSdNode)));
}

