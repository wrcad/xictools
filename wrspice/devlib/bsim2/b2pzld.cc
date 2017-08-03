
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
Authors: 1985 Hong June Park, Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b2defs.h"


int
B2dev::pzLoad(sGENmodel *genmod, sCKT *ckt, IFcomplex *s)
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

            xcdgb = (cdgb - inst->pParam->B2GDoverlapCap);
            xcddb = (cddb + capbd + inst->pParam->B2GDoverlapCap);
            xcdsb = cdsb;
            xcsgb = -(cggb + cbgb + cdgb + inst->pParam->B2GSoverlapCap);
            xcsdb = -(cgdb + cbdb + cddb);
            xcssb = (capbs + inst->pParam->B2GSoverlapCap - (cgsb+cbsb+cdsb));
            xcggb = (cggb + inst->pParam->B2GDoverlapCap +
                inst->pParam->B2GSoverlapCap +
                inst->pParam->B2GBoverlapCap);
            xcgdb = (cgdb - inst->pParam->B2GDoverlapCap);
            xcgsb = (cgsb - inst->pParam->B2GSoverlapCap);
            xcbgb = (cbgb - inst->pParam->B2GBoverlapCap);
            xcbdb = (cbdb - capbd);
            xcbsb = (cbsb - capbs);

#define MSC(xx) inst->B2m*(xx)

            *(inst->B2GgPtr   ) += MSC(xcggb * s->real);
            *(inst->B2GgPtr +1) += MSC(xcggb * s->imag);
            *(inst->B2BbPtr   ) += MSC((-xcbgb-xcbdb-xcbsb) * s->real);
            *(inst->B2BbPtr +1) += MSC((-xcbgb-xcbdb-xcbsb) * s->imag);
            *(inst->B2DPdpPtr   ) += MSC(xcddb * s->real);
            *(inst->B2DPdpPtr +1) += MSC(xcddb * s->imag);
            *(inst->B2SPspPtr   ) += MSC(xcssb * s->real);
            *(inst->B2SPspPtr +1) += MSC(xcssb * s->imag);
            *(inst->B2GbPtr   ) += MSC((-xcggb-xcgdb-xcgsb) * s->real);
            *(inst->B2GbPtr +1) += MSC((-xcggb-xcgdb-xcgsb) * s->imag);
            *(inst->B2GdpPtr   ) += MSC(xcgdb * s->real);
            *(inst->B2GdpPtr +1) += MSC(xcgdb * s->imag);
            *(inst->B2GspPtr   ) += MSC(xcgsb * s->real);
            *(inst->B2GspPtr +1) += MSC(xcgsb * s->imag);
            *(inst->B2BgPtr   ) += MSC(xcbgb * s->real);
            *(inst->B2BgPtr +1) += MSC(xcbgb * s->imag);
            *(inst->B2BdpPtr   ) += MSC(xcbdb * s->real);
            *(inst->B2BdpPtr +1) += MSC(xcbdb * s->imag);
            *(inst->B2BspPtr   ) += MSC(xcbsb * s->real);
            *(inst->B2BspPtr +1) += MSC(xcbsb * s->imag);
            *(inst->B2DPgPtr   ) += MSC(xcdgb * s->real);
            *(inst->B2DPgPtr +1) += MSC(xcdgb * s->imag);
            *(inst->B2DPbPtr   ) += MSC((-xcdgb-xcddb-xcdsb) * s->real);
            *(inst->B2DPbPtr +1) += MSC((-xcdgb-xcddb-xcdsb) * s->imag);
            *(inst->B2DPspPtr   ) += MSC(xcdsb * s->real);
            *(inst->B2DPspPtr +1) += MSC(xcdsb * s->imag);
            *(inst->B2SPgPtr   ) += MSC(xcsgb * s->real);
            *(inst->B2SPgPtr +1) += MSC(xcsgb * s->imag);
            *(inst->B2SPbPtr   ) += MSC((-xcsgb-xcsdb-xcssb) * s->real);
            *(inst->B2SPbPtr +1) += MSC((-xcsgb-xcsdb-xcssb) * s->imag);
            *(inst->B2SPdpPtr   ) += MSC(xcsdb * s->real);
            *(inst->B2SPdpPtr +1) += MSC(xcsdb * s->imag);
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

