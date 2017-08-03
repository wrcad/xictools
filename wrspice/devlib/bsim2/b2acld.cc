
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
Authors: 1988 Min-Chie Jeng, Hong June Park, Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b2defs.h"


int
B2dev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    int xnrm;
    int xrev;
    double gdpr;
    double gspr;
    double gm;
    double gds;
    double gmbs;
    double gbd;
    double gbs;
    double capbd;
    double capbs;
    double xcggb;
    double xcgdb;
    double xcgsb;
    double xcbgb;
    double xcbdb;
    double xcbsb;
    double xcddb;
    double xcssb;
    double xcdgb;
    double xcsgb;
    double xcdsb;
    double xcsdb;
    double cggb;
    double cgdb;
    double cgsb;
    double cbgb;
    double cbdb;
    double cbsb;
    double cddb;
    double cdgb;
    double cdsb;
    double omega; // angular frequency of the signal

    omega = ckt->CKTomega;
    sB2model *model = static_cast<sB2model*>(genmod);
    for ( ; model; model = model->next()) {
        sB2instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
        
            if (inst->B2mode >= 0) {
                xnrm = 1;
                xrev = 0;
            }
            else {
                xnrm = 0;
                xrev = 1;
            }
            gdpr = inst->B2drainConductance;
            gspr = inst->B2sourceConductance;
            gm = *(ckt->CKTstate0 + inst->B2gm);
            gds = *(ckt->CKTstate0 + inst->B2gds);
            gmbs = *(ckt->CKTstate0 + inst->B2gmbs);
            gbd = *(ckt->CKTstate0 + inst->B2gbd);
            gbs = *(ckt->CKTstate0 + inst->B2gbs);
            capbd = *(ckt->CKTstate0 + inst->B2capbd);
            capbs = *(ckt->CKTstate0 + inst->B2capbs);

            //
            //    charge oriented model parameters
            //

            cggb = *(ckt->CKTstate0 + inst->B2cggb);
            cgsb = *(ckt->CKTstate0 + inst->B2cgsb);
            cgdb = *(ckt->CKTstate0 + inst->B2cgdb);

            cbgb = *(ckt->CKTstate0 + inst->B2cbgb);
            cbsb = *(ckt->CKTstate0 + inst->B2cbsb);
            cbdb = *(ckt->CKTstate0 + inst->B2cbdb);

            cdgb = *(ckt->CKTstate0 + inst->B2cdgb);
            cdsb = *(ckt->CKTstate0 + inst->B2cdsb);
            cddb = *(ckt->CKTstate0 + inst->B2cddb);

            xcdgb = (cdgb - inst->pParam->B2GDoverlapCap) * omega;
            xcddb = (cddb + capbd + inst->pParam->B2GDoverlapCap) * omega;
            xcdsb = cdsb * omega;
            xcsgb = -(cggb + cbgb + cdgb + inst->pParam->B2GSoverlapCap)*omega;
            xcsdb = -(cgdb + cbdb + cddb) * omega;
            xcssb = (capbs + inst->pParam->B2GSoverlapCap -
                (cgsb+cbsb+cdsb)) * omega;
            xcggb = (cggb + inst->pParam->B2GDoverlapCap +
                inst->pParam->B2GSoverlapCap +
                inst->pParam->B2GBoverlapCap) * omega;
            xcgdb = (cgdb - inst->pParam->B2GDoverlapCap ) * omega;
            xcgsb = (cgsb - inst->pParam->B2GSoverlapCap) * omega;
            xcbgb = (cbgb - inst->pParam->B2GBoverlapCap) * omega;
            xcbdb = (cbdb - capbd ) * omega;
            xcbsb = (cbsb - capbs ) * omega;

#define MSC(xx) inst->B2m*(xx)

            *(inst->B2GgPtr +1) += MSC(xcggb);
            *(inst->B2BbPtr +1) += MSC(-xcbgb-xcbdb-xcbsb);
            *(inst->B2DPdpPtr +1) += MSC(xcddb);
            *(inst->B2SPspPtr +1) += MSC(xcssb);
            *(inst->B2GbPtr +1) += MSC(-xcggb-xcgdb-xcgsb);
            *(inst->B2GdpPtr +1) += MSC(xcgdb);
            *(inst->B2GspPtr +1) += MSC(xcgsb);
            *(inst->B2BgPtr +1) += MSC(xcbgb);
            *(inst->B2BdpPtr +1) += MSC(xcbdb);
            *(inst->B2BspPtr +1) += MSC(xcbsb);
            *(inst->B2DPgPtr +1) += MSC(xcdgb);
            *(inst->B2DPbPtr +1) += MSC(-xcdgb-xcddb-xcdsb);
            *(inst->B2DPspPtr +1) += MSC(xcdsb);
            *(inst->B2SPgPtr +1) += MSC(xcsgb);
            *(inst->B2SPbPtr +1) += MSC(-xcsgb-xcsdb-xcssb);
            *(inst->B2SPdpPtr +1) += MSC(xcsdb);
            *(inst->B2DdPtr) += MSC(gdpr);
            *(inst->B2SsPtr) += MSC(gspr);
            *(inst->B2BbPtr) += MSC(gbd+gbs);
            *(inst->B2DPdpPtr) += MSC(gdpr+gds+gbd+xrev*(gm+gmbs));
            *(inst->B2SPspPtr) += MSC(gspr+gds+gbs+xnrm*(gm+gmbs));
            *(inst->B2DdpPtr) -= MSC(gdpr);
            *(inst->B2SspPtr) -= MSC(gspr);
            *(inst->B2BdpPtr) -= MSC(gbd);
            *(inst->B2BspPtr) -= MSC(gbs);
            *(inst->B2DPdPtr) -= MSC(gdpr);
            *(inst->B2DPgPtr) += MSC((xnrm-xrev)*gm);
            *(inst->B2DPbPtr) += MSC(-gbd+(xnrm-xrev)*gmbs);
            *(inst->B2DPspPtr) += MSC(-gds-xnrm*(gm+gmbs));
            *(inst->B2SPgPtr) += MSC(-(xnrm-xrev)*gm);
            *(inst->B2SPsPtr) -= MSC(gspr);
            *(inst->B2SPbPtr) += MSC(-gbs-(xnrm-xrev)*gmbs);
            *(inst->B2SPdpPtr) += MSC(-gds-xrev*(gm+gmbs));

        }
    }
    return (OK);
}



