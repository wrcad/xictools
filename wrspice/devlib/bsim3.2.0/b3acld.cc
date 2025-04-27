
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

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1995 Min-Chie Jeng and Mansun Chan.
Modified by Weidong Liu (1997-1998).
* Revision 3.2 1998/6/16  18:00:00  Weidong 
* BSIM3v3.2 release
**********/

#include "b3defs.h"


int
B3dev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    double xcggb, xcgdb, xcgsb, xcbgb, xcbdb, xcbsb, xcddb, xcssb, xcdgb;
    double gdpr, gspr, gds, gbd, gbs, capbd, capbs, xcsgb, xcdsb, xcsdb;
    double cggb, cgdb, cgsb, cbgb, cbdb, cbsb, cddb, cdgb, cdsb, omega;
    double GSoverlapCap, GDoverlapCap, GBoverlapCap, FwdSum, RevSum, Gm, Gmbs;
    double dxpart, sxpart, xgtg, xgtd, xgts, xgtb, xcqgb, xcqdb, xcqsb, xcqbb;
    double gbspsp, gbbdp, gbbsp, gbspg, gbspb;
    double gbspdp, gbdpdp, gbdpg, gbdpb, gbdpsp;
    double ddxpart_dVd, ddxpart_dVg, ddxpart_dVb, ddxpart_dVs;
    double dsxpart_dVd, dsxpart_dVg, dsxpart_dVb, dsxpart_dVs;
    double T1, CoxWL, qcheq, Cdg, Cdd, Cds, Csg, Csd, Css;

    // SRW - avoid compiler warning about unset variables
    xcqgb = 0.0;
    xcqdb = 0.0;
    xcqsb = 0.0;
    xcqbb = 0.0;

    omega = ckt->CKTomega;
    sB3model *model = static_cast<sB3model*>(genmod);
    for ( ; model; model = model->next()) {

        sB3instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (inst->B3mode >= 0) {
                Gm = inst->B3gm;
                Gmbs = inst->B3gmbs;
                FwdSum = Gm + Gmbs;
                RevSum = 0.0;

                gbbdp = -inst->B3gbds;
                gbbsp = inst->B3gbds + inst->B3gbgs + inst->B3gbbs;

                gbdpg = inst->B3gbgs;
                gbdpb = inst->B3gbbs;
                gbdpdp = inst->B3gbds;
                gbdpsp = -(gbdpg + gbdpb + gbdpdp);

                gbspdp = 0.0;
                gbspg = 0.0;
                gbspb = 0.0;
                gbspsp = 0.0;

                if (inst->B3nqsMod == 0) {
                    cggb = inst->B3cggb;
                    cgsb = inst->B3cgsb;
                    cgdb = inst->B3cgdb;

                    cbgb = inst->B3cbgb;
                    cbsb = inst->B3cbsb;
                    cbdb = inst->B3cbdb;

                    cdgb = inst->B3cdgb;
                    cdsb = inst->B3cdsb;
                    cddb = inst->B3cddb;

                    xgtg = xgtd = xgts = xgtb = 0.0;
                    sxpart = 0.6;
                    dxpart = 0.4;
                    ddxpart_dVd = ddxpart_dVg = ddxpart_dVb =
                        ddxpart_dVs = 0.0;
                    dsxpart_dVd = dsxpart_dVg = dsxpart_dVb =
                        dsxpart_dVs = 0.0;
                }                  
                else {
                    cggb = cgdb = cgsb = 0.0;
                    cbgb = cbdb = cbsb = 0.0;
                    cdgb = cddb = cdsb = 0.0;

                    xgtg = inst->B3gtg;
                    xgtd = inst->B3gtd;
                    xgts = inst->B3gts;
                    xgtb = inst->B3gtb; 
 
                    xcqgb = inst->B3cqgb * omega;
                    xcqdb = inst->B3cqdb * omega;
                    xcqsb = inst->B3cqsb * omega;
                    xcqbb = inst->B3cqbb * omega;

                    CoxWL = model->B3cox * inst->pParam->B3weffCV
                        * inst->pParam->B3leffCV;
                    qcheq = -(inst->B3qgate + inst->B3qbulk);
                    if (fabs(qcheq) <= 1.0e-5 * CoxWL) {
                        if (model->B3xpart < 0.5) {
                            dxpart = 0.4;
                        }
                        else if (model->B3xpart > 0.5) {
                            dxpart = 0.0;
                        }
                        else {
                            dxpart = 0.5;
                        }
                        ddxpart_dVd = ddxpart_dVg = ddxpart_dVb =
                            ddxpart_dVs = 0.0;
                    }
                    else {
                        dxpart = inst->B3qdrn / qcheq;
                        Cdd = inst->B3cddb;
                        Csd = -(inst->B3cgdb + inst->B3cddb
                            + inst->B3cbdb);
                        ddxpart_dVd = (Cdd - dxpart * (Cdd + Csd)) / qcheq;
                        Cdg = inst->B3cdgb;
                        Csg = -(inst->B3cggb + inst->B3cdgb
                            + inst->B3cbgb);
                        ddxpart_dVg = (Cdg - dxpart * (Cdg + Csg)) / qcheq;

                        Cds = inst->B3cdsb;
                        Css = -(inst->B3cgsb + inst->B3cdsb
                            + inst->B3cbsb);
                        ddxpart_dVs = (Cds - dxpart * (Cds + Css)) / qcheq;

                         ddxpart_dVb = -(ddxpart_dVd + ddxpart_dVg 
                            + ddxpart_dVs);
                    }
                    sxpart = 1.0 - dxpart;
                    dsxpart_dVd = -ddxpart_dVd;
                    dsxpart_dVg = -ddxpart_dVg;
                    dsxpart_dVs = -ddxpart_dVs;
                    dsxpart_dVb = -(dsxpart_dVd + dsxpart_dVg + dsxpart_dVs);
                }
            } 
            else {
                Gm = -inst->B3gm;
                Gmbs = -inst->B3gmbs;
                FwdSum = 0.0;
                RevSum = -(Gm + Gmbs);

                gbbsp = -inst->B3gbds;
                gbbdp = inst->B3gbds + inst->B3gbgs + inst->B3gbbs;

                gbdpg = 0.0;
                gbdpsp = 0.0;
                gbdpb = 0.0;
                gbdpdp = 0.0;

                gbspg = inst->B3gbgs;
                gbspsp = inst->B3gbds;
                gbspb = inst->B3gbbs;
                gbspdp = -(gbspg + gbspsp + gbspb);

                if (inst->B3nqsMod == 0) {
                    cggb = inst->B3cggb;
                    cgsb = inst->B3cgdb;
                    cgdb = inst->B3cgsb;

                    cbgb = inst->B3cbgb;
                    cbsb = inst->B3cbdb;
                    cbdb = inst->B3cbsb;

                    cdgb = -(inst->B3cdgb + cggb + cbgb);
                    cdsb = -(inst->B3cddb + cgsb + cbsb);
                    cddb = -(inst->B3cdsb + cgdb + cbdb);

                    xgtg = xgtd = xgts = xgtb = 0.0;
                    sxpart = 0.4;
                    dxpart = 0.6;
                    ddxpart_dVd = ddxpart_dVg = ddxpart_dVb =
                        ddxpart_dVs = 0.0;
                    dsxpart_dVd = dsxpart_dVg = dsxpart_dVb =
                        dsxpart_dVs = 0.0;
                }
                else {
                    cggb = cgdb = cgsb = 0.0;
                    cbgb = cbdb = cbsb = 0.0;
                    cdgb = cddb = cdsb = 0.0;

                    xgtg = inst->B3gtg;
                    xgtd = inst->B3gts;
                    xgts = inst->B3gtd;
                    xgtb = inst->B3gtb;

                    xcqgb = inst->B3cqgb * omega;
                    xcqdb = inst->B3cqsb * omega;
                    xcqsb = inst->B3cqdb * omega;
                    xcqbb = inst->B3cqbb * omega;

                    CoxWL = model->B3cox * inst->pParam->B3weffCV
                        * inst->pParam->B3leffCV;
                    qcheq = -(inst->B3qgate + inst->B3qbulk);
                    if (fabs(qcheq) <= 1.0e-5 * CoxWL) {
                        if (model->B3xpart < 0.5) {
                            sxpart = 0.4;
                        }
                        else if (model->B3xpart > 0.5) {
                            sxpart = 0.0;
                        }
                        else {
                            sxpart = 0.5;
                        }
                        dsxpart_dVd = dsxpart_dVg = dsxpart_dVb =
                            dsxpart_dVs = 0.0;
                    }
                    else {
                        sxpart = inst->B3qdrn / qcheq;
                        Css = inst->B3cddb;
                        Cds = -(inst->B3cgdb + inst->B3cddb
                            + inst->B3cbdb);
                         dsxpart_dVs = (Css - sxpart * (Css + Cds)) / qcheq;
                         Csg = inst->B3cdgb;
                         Cdg = -(inst->B3cggb + inst->B3cdgb
                             + inst->B3cbgb);
                         dsxpart_dVg = (Csg - sxpart * (Csg + Cdg)) / qcheq;

                         Csd = inst->B3cdsb;
                         Cdd = -(inst->B3cgsb + inst->B3cdsb
                             + inst->B3cbsb);
                         dsxpart_dVd = (Csd - sxpart * (Csd + Cdd)) / qcheq;

                         dsxpart_dVb = -(dsxpart_dVd + dsxpart_dVg 
                             + dsxpart_dVs);
                    }
                    dxpart = 1.0 - sxpart;
                    ddxpart_dVd = -dsxpart_dVd;
                    ddxpart_dVg = -dsxpart_dVg;
                    ddxpart_dVs = -dsxpart_dVs;
                    ddxpart_dVb = -(ddxpart_dVd + ddxpart_dVg + ddxpart_dVs);
                }
            }

            T1 = *(ckt->CKTstate0 + inst->B3qdef) * inst->B3gtau;
            gdpr = inst->B3drainConductance;
            gspr = inst->B3sourceConductance;
            gds = inst->B3gds;
            gbd = inst->B3gbd;
            gbs = inst->B3gbs;
            capbd = inst->B3capbd;
            capbs = inst->B3capbs;

            GSoverlapCap = inst->B3cgso;
            GDoverlapCap = inst->B3cgdo;
            GBoverlapCap = inst->pParam->B3cgbo;

            xcdgb = (cdgb - GDoverlapCap) * omega;
            xcddb = (cddb + capbd + GDoverlapCap) * omega;
            xcdsb = cdsb * omega;
            xcsgb = -(cggb + cbgb + cdgb + GSoverlapCap) * omega;
            xcsdb = -(cgdb + cbdb + cddb) * omega;
            xcssb = (capbs + GSoverlapCap - (cgsb + cbsb + cdsb)) * omega;
            xcggb = (cggb + GDoverlapCap + GSoverlapCap + GBoverlapCap)*omega;
            xcgdb = (cgdb - GDoverlapCap ) * omega;
            xcgsb = (cgsb - GSoverlapCap) * omega;
            xcbgb = (cbgb - GBoverlapCap) * omega;
            xcbdb = (cbdb - capbd ) * omega;
            xcbsb = (cbsb - capbs ) * omega;

#define MSC(xx) inst->B3m*(xx)

            *(inst->B3GgPtr +1) += MSC(xcggb);
            *(inst->B3BbPtr +1) -= MSC(xcbgb + xcbdb + xcbsb);
            *(inst->B3DPdpPtr +1) += MSC(xcddb);
            *(inst->B3SPspPtr +1) += MSC(xcssb);
            *(inst->B3GbPtr +1) -= MSC(xcggb + xcgdb + xcgsb);
            *(inst->B3GdpPtr +1) += MSC(xcgdb);
            *(inst->B3GspPtr +1) += MSC(xcgsb);
            *(inst->B3BgPtr +1) += MSC(xcbgb);
            *(inst->B3BdpPtr +1) += MSC(xcbdb);
            *(inst->B3BspPtr +1) += MSC(xcbsb);
            *(inst->B3DPgPtr +1) += MSC(xcdgb);
            *(inst->B3DPbPtr +1) -= MSC(xcdgb + xcddb + xcdsb);
            *(inst->B3DPspPtr +1) += MSC(xcdsb);
            *(inst->B3SPgPtr +1) += MSC(xcsgb);
            *(inst->B3SPbPtr +1) -= MSC(xcsgb + xcsdb + xcssb);
            *(inst->B3SPdpPtr +1) += MSC(xcsdb);

            *(inst->B3DdPtr) += MSC(gdpr);
            *(inst->B3SsPtr) += MSC(gspr);
            *(inst->B3BbPtr) += MSC(gbd + gbs - inst->B3gbbs);
            *(inst->B3DPdpPtr) += MSC(gdpr + gds + gbd + RevSum 
                + dxpart * xgtd + T1 * ddxpart_dVd + gbdpdp);
            *(inst->B3SPspPtr) += MSC(gspr + gds + gbs + FwdSum 
                + sxpart * xgts + T1 * dsxpart_dVs + gbspsp);

            *(inst->B3DdpPtr) -= MSC(gdpr);
            *(inst->B3SspPtr) -= MSC(gspr);

            *(inst->B3BgPtr) -= MSC(inst->B3gbgs);
            *(inst->B3BdpPtr) -= MSC(gbd - gbbdp);
            *(inst->B3BspPtr) -= MSC(gbs - gbbsp);

            *(inst->B3DPdPtr) -= MSC(gdpr);
            *(inst->B3DPgPtr) += MSC(Gm + dxpart * xgtg + T1 * ddxpart_dVg
                + gbdpg);
            *(inst->B3DPbPtr) -= MSC(gbd - Gmbs - dxpart * xgtb
                - T1 * ddxpart_dVb - gbdpb);
            *(inst->B3DPspPtr) -= MSC(gds + FwdSum - dxpart * xgts 
                - T1 * ddxpart_dVs - gbdpsp);

            *(inst->B3SPgPtr) -= MSC(Gm - sxpart * xgtg - T1 * dsxpart_dVg
                - gbspg);
            *(inst->B3SPsPtr) -= MSC(gspr);
            *(inst->B3SPbPtr) -= MSC(gbs + Gmbs - sxpart * xgtb
                - T1 * dsxpart_dVb - gbspb);
            *(inst->B3SPdpPtr) -= MSC(gds + RevSum - sxpart * xgtd 
                - T1 * dsxpart_dVd - gbspdp);

            *(inst->B3GgPtr) -= MSC(xgtg);
            *(inst->B3GbPtr) -= MSC(xgtb);
            *(inst->B3GdpPtr) -= MSC(xgtd);
            *(inst->B3GspPtr) -= MSC(xgts);

            if (inst->B3nqsMod) {
                *(inst->B3QqPtr +1) += omega;
                *(inst->B3QgPtr +1) -= MSC(xcqgb);
                *(inst->B3QdpPtr +1) -= MSC(xcqdb);
                *(inst->B3QspPtr +1) -= MSC(xcqsb);
                *(inst->B3QbPtr +1) -= MSC(xcqbb);

                *(inst->B3QqPtr) += MSC(inst->B3gtau);

                *(inst->B3DPqPtr) += MSC(dxpart * inst->B3gtau);
                *(inst->B3SPqPtr) += MSC(sxpart * inst->B3gtau);
                *(inst->B3GqPtr) -=  MSC(inst->B3gtau);

                *(inst->B3QgPtr) +=  MSC(xgtg);
                *(inst->B3QdpPtr) += MSC(xgtd);
                *(inst->B3QspPtr) += MSC(xgts);
                *(inst->B3QbPtr) += MSC(xgtb);
            }
        }
    }
    return (OK);
}

#ifdef notdef

int
B3dev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    double xcggb, xcgdb, xcgsb, xcbgb, xcbdb, xcbsb, xcddb, xcssb, xcdgb;
    double gdpr, gspr, gds, gbd, gbs, capbd, capbs, xcsgb, xcdsb, xcsdb;
    double cggb, cgdb, cgsb, cbgb, cbdb, cbsb, cddb, cdgb, cdsb, omega;
    double GSoverlapCap, GDoverlapCap, GBoverlapCap, FwdSum, RevSum, Gm, Gmbs;
    double dxpart, sxpart, cqgb, cqdb, cqsb, cqbb, xcqgb, xcqdb, xcqsb, xcqbb;

    omega = ckt->CKTomega;
    sB3model *model = static_cast<sB3model*>(genmod);
    for ( ; model; model = model->next()) {

        sB3instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (inst->B3mode >= 0) {
                Gm = inst->B3gm;
                Gmbs = inst->B3gmbs;
                FwdSum = Gm + Gmbs;
                RevSum = 0.0;
                cggb = inst->B3cggb;
                cgsb = inst->B3cgsb;
                cgdb = inst->B3cgdb;

                cbgb = inst->B3cbgb;
                cbsb = inst->B3cbsb;
                cbdb = inst->B3cbdb;

                cdgb = inst->B3cdgb;
                cdsb = inst->B3cdsb;
                cddb = inst->B3cddb;

                cqgb = inst->B3cqgb;
                cqdb = inst->B3cqdb;
                cqsb = inst->B3cqsb;
                cqbb = inst->B3cqbb;
                sxpart = 0.6;
                dxpart = 0.4;
            } 
            else {
                Gm = -inst->B3gm;
                Gmbs = -inst->B3gmbs;
                FwdSum = 0.0;
                RevSum = -Gm - Gmbs;
                cggb = inst->B3cggb;
                cgsb = inst->B3cgdb;
                cgdb = inst->B3cgsb;

                cbgb = inst->B3cbgb;
                cbsb = inst->B3cbdb;
                cbdb = inst->B3cbsb;

                cdgb = -(inst->B3cdgb + cggb + cbgb);
                cdsb = -(inst->B3cddb + cgsb + cbsb);
                cddb = -(inst->B3cdsb + cgdb + cbdb);

                cqgb = inst->B3cqgb;
                cqdb = inst->B3cqsb;
                cqsb = inst->B3cqdb;
                cqbb = inst->B3cqbb;
                sxpart = 0.4;
                dxpart = 0.6;
            }

            gdpr=inst->B3drainConductance;
            gspr=inst->B3sourceConductance;
            gds= inst->B3gds;
            gbd= inst->B3gbd;
            gbs= inst->B3gbs;
            capbd= inst->B3capbd;
            capbs= inst->B3capbs;

            GSoverlapCap = inst->B3cgso;
            GDoverlapCap = inst->B3cgdo;
            GBoverlapCap = inst->pParam->B3cgbo;

            xcdgb = (cdgb - GDoverlapCap) * omega;
            xcddb = (cddb + capbd + GDoverlapCap) * omega;
            xcdsb = cdsb * omega;
            xcsgb = -(cggb + cbgb + cdgb + GSoverlapCap) * omega;
            xcsdb = -(cgdb + cbdb + cddb) * omega;
            xcssb = (capbs + GSoverlapCap - (cgsb + cbsb + cdsb)) * omega;
            xcggb = (cggb + GDoverlapCap + GSoverlapCap + GBoverlapCap)
                * omega;
            xcgdb = (cgdb - GDoverlapCap ) * omega;
            xcgsb = (cgsb - GSoverlapCap) * omega;
            xcbgb = (cbgb - GBoverlapCap) * omega;
            xcbdb = (cbdb - capbd ) * omega;
            xcbsb = (cbsb - capbs ) * omega;
            xcqgb = cqgb * omega;
            xcqdb = cqdb * omega;
            xcqsb = cqsb * omega;
            xcqbb = cqbb * omega;

            *(inst->B3GgPtr +1) += MSC(xcggb);
            *(inst->B3BbPtr +1) -= MSC(xcbgb + xcbdb + xcbsb);
            *(inst->B3DPdpPtr +1) += MSC(xcddb);
            *(inst->B3SPspPtr +1) += MSC(xcssb);
            *(inst->B3GbPtr +1) -= MSC(xcggb + xcgdb + xcgsb);
            *(inst->B3GdpPtr +1) += MSC(xcgdb);
            *(inst->B3GspPtr +1) += MSC(xcgsb);
            *(inst->B3BgPtr +1) += MSC(xcbgb);
            *(inst->B3BdpPtr +1) += MSC(xcbdb);
            *(inst->B3BspPtr +1) += MSC(xcbsb);
            *(inst->B3DPgPtr +1) += MSC(xcdgb);
            *(inst->B3DPbPtr +1) -= MSC(xcdgb + xcddb + xcdsb);
            *(inst->B3DPspPtr +1) += MSC(xcdsb);
            *(inst->B3SPgPtr +1) += MSC(xcsgb);
            *(inst->B3SPbPtr +1) -= MSC(xcsgb + xcsdb + xcssb);
            *(inst->B3SPdpPtr +1) += MSC(xcsdb);
 
            *(inst->B3QqPtr +1) += omega;

            *(inst->B3QgPtr +1) -= MSC(xcqgb);
            *(inst->B3QdpPtr +1) -= MSC(xcqdb);
            *(inst->B3QspPtr +1) -= MSC(xcqsb);
            *(inst->B3QbPtr +1) -= MSC(xcqbb);

            *(inst->B3DdPtr) += MSC(gdpr);
            *(inst->B3SsPtr) += MSC(gspr);
            *(inst->B3BbPtr) += MSC(gbd + gbs);
            *(inst->B3DPdpPtr) +=
                MSC(gdpr + gds + gbd + RevSum + dxpart*inst->B3gtd);
            *(inst->B3SPspPtr) +=
                MSC(gspr + gds + gbs + FwdSum + sxpart*inst->B3gts);
            *(inst->B3DdpPtr) -= MSC(gdpr);
            *(inst->B3SspPtr) -= MSC(gspr);
            *(inst->B3BdpPtr) -= MSC(gbd);
            *(inst->B3BspPtr) -= MSC(gbs);
            *(inst->B3DPdPtr) -= MSC(gdpr);
            *(inst->B3DPgPtr) += MSC(Gm + dxpart * inst->B3gtg);
            *(inst->B3DPbPtr) -= MSC(gbd - Gmbs - dxpart * inst->B3gtb);
            *(inst->B3DPspPtr) -= MSC(gds + FwdSum - dxpart * inst->B3gts);
            *(inst->B3SPgPtr) -= MSC(Gm - sxpart * inst->B3gtg);
            *(inst->B3SPsPtr) -= MSC(gspr);
            *(inst->B3SPbPtr) -= MSC(gbs + Gmbs - sxpart * inst->B3gtg);
            *(inst->B3SPdpPtr) -= MSC(gds + RevSum - sxpart * inst->B3gtd);
            *(inst->B3GgPtr) -= MSC(inst->B3gtg);
            *(inst->B3GbPtr) -=  MSC(inst->B3gtb);
            *(inst->B3GdpPtr) -= MSC(inst->B3gtd);
            *(inst->B3GspPtr) -= MSC(inst->B3gts);

            *(inst->B3QqPtr) += MSC(inst->B3gtau);
 
            *(inst->B3DPqPtr) += MSC(dxpart * inst->B3gtau);
            *(inst->B3SPqPtr) += MSC(sxpart * inst->B3gtau);
            *(inst->B3GqPtr) -=  MSC(inst->B3gtau);
 
            *(inst->B3QgPtr) +=  MSC(inst->B3gtg);
            *(inst->B3QdpPtr) += MSC(inst->B3gtd);
            *(inst->B3QspPtr) += MSC(inst->B3gts);
            *(inst->B3QbPtr) += MSC(inst->B3gtb);
        }
    }
    return(OK);
}

#endif
