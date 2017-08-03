
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

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1995 Min-Chie Jeng and Mansun Chan.
* Revision 3.2 1998/6/16  18:00:00  Weidong 
* BSIM3v3.2 release
**********/

#include "b3defs.h"

#define MSC(xx) inst->B3m*(xx)
#define tMSC(xx) B3m*(xx)


int
B3dev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sB3instance *inst = static_cast<const sB3instance*>(geninst);

    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch(which) {
    case B3_M:
        data->v.rValue = inst->B3m;
        break;
    case B3_L:
        data->v.rValue = inst->B3l;
        break;
    case B3_W:
        data->v.rValue = inst->B3w;
        break;
    case B3_AS:
        data->v.rValue = inst->B3sourceArea;
        break;
    case B3_AD:
        data->v.rValue = inst->B3drainArea;
        break;
    case B3_PS:
        data->v.rValue = inst->B3sourcePerimeter;
        break;
    case B3_PD:
        data->v.rValue = inst->B3drainPerimeter;
        break;
    case B3_NRS:
        data->v.rValue = inst->B3sourceSquares;
        break;
    case B3_NRD:
        data->v.rValue = inst->B3drainSquares;
        break;
    case B3_OFF:
        data->v.rValue = inst->B3off;
        break;
    case B3_NQSMOD:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B3nqsMod;
        break;
    case B3_IC_VBS:
        data->v.rValue = inst->B3icVBS;
        break;
    case B3_IC_VDS:
        data->v.rValue = inst->B3icVDS;
        break;
    case B3_IC_VGS:
        data->v.rValue = inst->B3icVGS;
        break;
    case B3_DNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B3dNode;
        break;
    case B3_GNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B3gNode;
        break;
    case B3_SNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B3sNode;
        break;
    case B3_BNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B3bNode;
        break;
    case B3_DNODEPRIME:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B3dNodePrime;
        break;
    case B3_SNODEPRIME:
        data->type = IF_INTEGER;
        data->v.iValue = inst->B3sNodePrime;
        break;
    case B3_SOURCECOND:
        data->v.rValue = inst->B3sourceConductance;
        break;
    case B3_DRAINCOND:
        data->v.rValue = inst->B3drainConductance;
        break;
    case B3_VBD:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->B3bNode) -
                ckt->rhsOld(inst->B3dNodePrime);
            data->v.cValue.imag = ckt->irhsOld(inst->B3bNode) -
                ckt->irhsOld(inst->B3dNodePrime);
        }
        else
            data->v.rValue = ckt->interp(inst->B3vbd);
        break;
    case B3_VBS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->B3bNode) -
                ckt->rhsOld(inst->B3sNodePrime);
            data->v.cValue.imag = ckt->irhsOld(inst->B3bNode) -
                ckt->irhsOld(inst->B3sNodePrime);
        }
        else
            data->v.rValue = ckt->interp(inst->B3vbs);
        break;
    case B3_VGS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->B3gNode) -
                ckt->rhsOld(inst->B3sNodePrime);
            data->v.cValue.imag = ckt->irhsOld(inst->B3gNode) -
                ckt->irhsOld(inst->B3sNodePrime);
        }
        else
            data->v.rValue = ckt->interp(inst->B3vgs);
        break;
    case B3_VDS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->B3dNodePrime) -
                ckt->rhsOld(inst->B3sNodePrime);
            data->v.cValue.imag = ckt->irhsOld(inst->B3dNodePrime) -
                ckt->irhsOld(inst->B3sNodePrime);
        }
        else
            data->v.rValue = ckt->interp(inst->B3vds);
        break;
    case B3_CD:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cd(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else
            data->v.rValue = ckt->interp(inst->B3a_cd);
        break;
    case B3_CS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cs(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else {
            sB3model *model = static_cast<sB3model*>(inst->GENmodPtr);
            double tmp = MSC(ckt->interp(inst->B3cqb) + 
                ckt->interp(inst->B3cqg));
            data->v.rValue =
                -ckt->interp(inst->B3a_cd) -
                ckt->interp(inst->B3a_cbd) -
                ckt->interp(inst->B3a_cbs) -
                ckt->interp(inst->B3a_cgdb) -
                ckt->interp(inst->B3a_cgsb) -
                (model->B3type > 0 ? tmp : -tmp);
        }
        break;
    case B3_CG:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cg(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else {
            sB3model *model = static_cast<sB3model*>(inst->GENmodPtr);
            double tmp = MSC(ckt->interp(inst->B3cqg));
            data->v.rValue =
                ckt->interp(inst->B3a_cgsb) +
                ckt->interp(inst->B3a_cgdb) +
                (model->B3type > 0 ? tmp : -tmp);
        }
        break;
    case B3_CB:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cb(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else {
            sB3model *model = static_cast<sB3model*>(inst->GENmodPtr);
            double tmp = MSC(ckt->interp(inst->B3cqb));
            data->v.rValue =
                ckt->interp(inst->B3a_cbd) +
                ckt->interp(inst->B3a_cbs) +
                (model->B3type > 0 ? tmp : -tmp);
        }
        break;
    case B3_CBS:
        data->v.rValue = ckt->interp(inst->B3a_cbs);
        break;
    case B3_CBD:
        data->v.rValue = ckt->interp(inst->B3a_cbd);
        break;
    case B3_GM:
        data->v.rValue = ckt->interp(inst->B3a_gm);
        break;
    case B3_GDS:
        data->v.rValue = ckt->interp(inst->B3a_gds);
        break;
    case B3_GMBS:
        data->v.rValue = ckt->interp(inst->B3a_gmbs);
        break;
    case B3_GBD:
        data->v.rValue = ckt->interp(inst->B3a_gbd);
        break;
    case B3_GBS:
        data->v.rValue = ckt->interp(inst->B3a_gbs);
        break;
    case B3_QB:
        data->v.rValue = MSC(ckt->interp(inst->B3qb));
        break;
    case B3_CQB:
        data->v.rValue = MSC(ckt->interp(inst->B3cqb));
        break;
    case B3_QG:
        data->v.rValue = MSC(ckt->interp(inst->B3qg));
        break;
    case B3_CQG:
        data->v.rValue = MSC(ckt->interp(inst->B3cqg));
        break;
    case B3_QD:
        data->v.rValue = MSC(ckt->interp(inst->B3qd));
        break;
    case B3_CQD:
        data->v.rValue = MSC(ckt->interp(inst->B3cqd));
        break;
    case B3_CGG:
        data->v.rValue = ckt->interp(inst->B3a_cggb);
        break;
    case B3_CGD:
        data->v.rValue = ckt->interp(inst->B3a_cgdb);
        break;
    case B3_CGS:
        data->v.rValue = ckt->interp(inst->B3a_cgsb);
        break;
    case B3_CDG:
        data->v.rValue = ckt->interp(inst->B3a_cdgb);
        break;
    case B3_CDD:
        data->v.rValue = ckt->interp(inst->B3a_cddb);
        break;
    case B3_CDS:
        data->v.rValue = ckt->interp(inst->B3a_cdsb);
        break;
    case B3_CBG:
        data->v.rValue = ckt->interp(inst->B3a_cbgb);
        break;
    case B3_CBDB:
        data->v.rValue = ckt->interp(inst->B3a_cbdb);
        break;
    case B3_CBSB:
        data->v.rValue = ckt->interp(inst->B3a_cbsb);
        break;
    case B3_CAPBD:
        data->v.rValue = ckt->interp(inst->B3a_capbd);
        break;
    case B3_CAPBS:
        data->v.rValue = ckt->interp(inst->B3a_capbs);
        break;
    case B3_VON:
        data->v.rValue = ckt->interp(inst->B3a_von);
        break;
    case B3_VDSAT:
        data->v.rValue = ckt->interp(inst->B3a_vdsat);
        break;
    case B3_QBS:
        data->v.rValue = MSC(ckt->interp(inst->B3qbs));
        break;
    case B3_QBD:
        data->v.rValue = MSC(ckt->interp(inst->B3qbd));
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}


void
sB3instance::ac_cd(const sCKT *ckt, double *cdr, double *cdi) const
{
    if (B3dNode != B3dNodePrime) {
        double gdpr = tMSC(B3drainConductance);
        *cdr = gdpr* (ckt->rhsOld(B3dNode) - ckt->rhsOld(B3dNodePrime));
        *cdi = gdpr* (ckt->irhsOld(B3dNode) - ckt->irhsOld(B3dNodePrime));
        return;
    }

    double FwdSum, RevSum, Gm, Gmbs;
    double cddb, cdgb, cdsb;
    double dxpart;
    if (B3mode >= 0) {
        Gm = B3gm;
        Gmbs = B3gmbs;
        FwdSum = Gm + Gmbs;
        RevSum = 0.0;

        cdgb = B3cdgb;
        cdsb = B3cdsb;
        cddb = B3cddb;

        dxpart = 0.4;
    } 
    else {
        Gm = -B3gm;
        Gmbs = -B3gmbs;
        FwdSum = 0.0;
        RevSum = -Gm - Gmbs;
        double cggb = B3cggb;
        double cgsb = B3cgdb;
        double cgdb = B3cgsb;

        double cbgb = B3cbgb;
        double cbsb = B3cbdb;
        double cbdb = B3cbsb;

        cdgb = -(B3cdgb + cggb + cbgb);
        cdsb = -(B3cddb + cgsb + cbsb);
        cddb = -(B3cdsb + cgdb + cbdb);

        dxpart = 0.6;
    }

    double capbd = B3capbd;
    double GDoverlapCap = B3cgdo;
    double omega = ckt->CKTomega;
    double xcdgb = (cdgb - GDoverlapCap) * omega;
    double xcddb = (cddb + capbd + GDoverlapCap) * omega;
    double xcdsb = cdsb * omega;

    cIFcomplex Add(B3gds + B3gbd + RevSum + dxpart*B3gtd, xcddb);
    cIFcomplex Adg(Gm + dxpart*B3gtg, xcdgb);
    cIFcomplex Ads(-B3gds - FwdSum + dxpart*B3gts, xcdsb);
    cIFcomplex Adb(-B3gbd + Gmbs + dxpart*B3gtb, -xcdgb - xcddb - xcdsb);
    cIFcomplex Adq(dxpart*B3gtau, 0);

    *cdr = tMSC(Add.real* ckt->rhsOld(B3dNodePrime) -
        Add.imag* ckt->irhsOld(B3dNodePrime) +
        Adg.real* ckt->rhsOld(B3gNode) -
        Adg.imag* ckt->irhsOld(B3gNode) +
        Ads.real* ckt->rhsOld(B3sNodePrime) -
        Ads.imag* ckt->irhsOld(B3sNodePrime) +
        Adb.real* ckt->rhsOld(B3bNode) -
        Adb.imag* ckt->irhsOld(B3bNode) +
        Adq.real* ckt->rhsOld(B3qNode));

    *cdi = tMSC(Add.real* ckt->irhsOld(B3dNodePrime) +
        Add.imag* ckt->rhsOld(B3dNodePrime) +
        Adg.real* ckt->irhsOld(B3gNode) +
        Adg.imag* ckt->rhsOld(B3gNode) +
        Ads.real* ckt->irhsOld(B3sNodePrime) +
        Ads.imag* ckt->rhsOld(B3sNodePrime) +
        Adb.real* ckt->irhsOld(B3bNode) +
        Adb.imag* ckt->rhsOld(B3bNode) +
        Adq.real* ckt->irhsOld(B3bNode));
}


void
sB3instance::ac_cs(const sCKT *ckt, double *csr, double *csi) const
{
    if (B3sNode != B3sNodePrime) {
        double gspr = tMSC(B3sourceConductance);
        *csr = gspr* (ckt->rhsOld(B3sNode) - ckt->rhsOld(B3sNodePrime));
        *csi = gspr* (ckt->irhsOld(B3sNode) - ckt->irhsOld(B3sNodePrime));
        return;
    }

    double FwdSum, RevSum, Gm, Gmbs;
    double cggb, cgdb, cgsb, cbgb, cbdb, cbsb, cddb, cdgb, cdsb;
    double sxpart;
    if (B3mode >= 0) {
        Gm = B3gm;
        Gmbs = B3gmbs;
        FwdSum = Gm + Gmbs;
        RevSum = 0.0;
        cggb = B3cggb;
        cgsb = B3cgsb;
        cgdb = B3cgdb;

        cbgb = B3cbgb;
        cbsb = B3cbsb;
        cbdb = B3cbdb;

        cdgb = B3cdgb;
        cdsb = B3cdsb;
        cddb = B3cddb;

        sxpart = 0.6;
    } 
    else {
        Gm = -B3gm;
        Gmbs = -B3gmbs;
        FwdSum = 0.0;
        RevSum = -Gm - Gmbs;
        cggb = B3cggb;
        cgsb = B3cgdb;
        cgdb = B3cgsb;

        cbgb = B3cbgb;
        cbsb = B3cbdb;
        cbdb = B3cbsb;

        cdgb = -(B3cdgb + cggb + cbgb);
        cdsb = -(B3cddb + cgsb + cbsb);
        cddb = -(B3cdsb + cgdb + cbdb);

        sxpart = 0.4;
    }

    double capbs = B3capbs;
    double GSoverlapCap = B3cgso;
    double omega = ckt->CKTomega;
    double xcsgb = -(cggb + cbgb + cdgb + GSoverlapCap) * omega;
    double xcsdb = -(cgdb + cbdb + cddb) * omega;
    double xcssb = (capbs + GSoverlapCap - (cgsb + cbsb + cdsb)) * omega;

    cIFcomplex Ass(B3gds + B3gbs + FwdSum + sxpart*B3gts, xcssb);
    cIFcomplex Asg(-Gm + sxpart*B3gtg, xcsgb);
    cIFcomplex Asd(-B3gds - RevSum + sxpart*B3gtd, xcsdb);
    cIFcomplex Asb(-B3gbs - Gmbs + sxpart*B3gtg, -xcsgb - xcsdb - xcssb);
    cIFcomplex Asq(sxpart*B3gtau, 0);

    *csr = tMSC(Ass.real* ckt->rhsOld(B3sNodePrime) -
        Ass.imag* ckt->irhsOld(B3sNodePrime) +
        Asg.real* ckt->rhsOld(B3gNode) -
        Asg.imag* ckt->irhsOld(B3gNode) +
        Asd.real* ckt->rhsOld(B3dNodePrime) -
        Asd.imag* ckt->irhsOld(B3dNodePrime) +
        Asb.real* ckt->rhsOld(B3bNode) -
        Asb.imag* ckt->irhsOld(B3bNode) +
        Asq.real* ckt->rhsOld(B3qNode));

    *csi = tMSC(Ass.real* ckt->irhsOld(B3sNodePrime) +
        Ass.imag* ckt->rhsOld(B3sNodePrime) +
        Asg.real* ckt->irhsOld(B3gNode) +
        Asg.imag* ckt->rhsOld(B3gNode) +
        Asd.real* ckt->irhsOld(B3dNodePrime) +
        Asd.imag* ckt->rhsOld(B3dNodePrime) +
        Asb.real* ckt->irhsOld(B3bNode) +
        Asb.imag* ckt->rhsOld(B3bNode) +
        Asq.real* ckt->irhsOld(B3bNode));
}


void
sB3instance::ac_cg(const sCKT *ckt, double *cgr, double *cgi) const
{
    double cggb, cgdb, cgsb;
    if (B3mode >= 0) {
        cggb = B3cggb;
        cgsb = B3cgsb;
        cgdb = B3cgdb;
    } 
    else {
        cggb = B3cggb;
        cgsb = B3cgdb;
        cgdb = B3cgsb;
    }

    double GSoverlapCap = B3cgso;
    double GDoverlapCap = B3cgdo;
    double GBoverlapCap = pParam->B3cgbo;
    double omega = ckt->CKTomega;
    double xcggb = (cggb + GDoverlapCap + GSoverlapCap + GBoverlapCap)
        * omega;
    double xcgdb = (cgdb - GDoverlapCap ) * omega;
    double xcgsb = (cgsb - GSoverlapCap) * omega;

    cIFcomplex Agg(-B3gtg, xcggb);
    cIFcomplex Ags(-B3gts, xcgsb);
    cIFcomplex Agd(-B3gtd, xcgdb);
    cIFcomplex Agb(-B3gtb, -xcggb - xcgdb - xcgsb);
    cIFcomplex Agq(-B3gtau, 0);

    *cgr = tMSC(Agg.real* ckt->rhsOld(B3gNode) -
        Agg.imag* ckt->irhsOld(B3gNode) +
        Ags.real* ckt->rhsOld(B3sNodePrime) -
        Ags.imag* ckt->irhsOld(B3sNodePrime) +
        Agd.real* ckt->rhsOld(B3dNodePrime) -
        Agd.imag* ckt->irhsOld(B3dNodePrime) +
        Agb.real* ckt->rhsOld(B3bNode) -
        Agb.imag* ckt->irhsOld(B3bNode) +
        Agq.real* ckt->rhsOld(B3qNode));

    *cgi = tMSC(Agg.real* ckt->irhsOld(B3gNode) +
        Agg.imag* ckt->rhsOld(B3gNode) +
        Ags.real* ckt->irhsOld(B3sNodePrime) +
        Ags.imag* ckt->rhsOld(B3sNodePrime) +
        Agd.real* ckt->irhsOld(B3dNodePrime) +
        Agd.imag* ckt->rhsOld(B3dNodePrime) +
        Agb.real* ckt->irhsOld(B3bNode) +
        Agb.imag* ckt->rhsOld(B3bNode) +
        Agq.real* ckt->irhsOld(B3bNode));
}


void
sB3instance::ac_cb(const sCKT *ckt, double *cbr, double *cbi) const
{
    double cbgb, cbdb, cbsb;
    if (B3mode >= 0) {
        cbgb = B3cbgb;
        cbsb = B3cbsb;
        cbdb = B3cbdb;
    } 
    else {
        cbgb = B3cbgb;
        cbsb = B3cbdb;
        cbdb = B3cbsb;
    }

    double capbd = B3capbd;
    double capbs = B3capbs;
    double GBoverlapCap = pParam->B3cgbo;
    double omega = ckt->CKTomega;
    double xcbgb = (cbgb - GBoverlapCap) * omega;
    double xcbdb = (cbdb - capbd ) * omega;
    double xcbsb = (cbsb - capbs ) * omega;

    cIFcomplex Abb(B3gbd + B3gbs, -xcbgb - xcbdb - xcbsb);
    cIFcomplex Abs(-B3gbs, xcbsb);
    cIFcomplex Abd(-B3gbd, xcbdb);
    cIFcomplex Abg(0, xcbgb);

    *cbr = tMSC(Abb.real* ckt->rhsOld(B3bNode) -
        Abb.imag* ckt->irhsOld(B3bNode) +
        Abs.real* ckt->rhsOld(B3sNodePrime) -
        Abs.imag* ckt->irhsOld(B3sNodePrime) +
        Abd.real* ckt->rhsOld(B3dNodePrime) -
        Abd.imag* ckt->irhsOld(B3dNodePrime) -
        Abg.imag* ckt->irhsOld(B3gNode));

    *cbi = tMSC(Abb.real* ckt->irhsOld(B3bNode) +
        Abb.imag* ckt->rhsOld(B3bNode) +
        Abs.real* ckt->irhsOld(B3sNodePrime) +
        Abs.imag* ckt->rhsOld(B3sNodePrime) +
        Abd.real* ckt->irhsOld(B3dNodePrime) +
        Abd.imag* ckt->rhsOld(B3dNodePrime) +
        Abg.imag* ckt->rhsOld(B3gNode));
}

