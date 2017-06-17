
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
 $Id: b1acld.cc,v 1.1 1999/04/19 19:49:07 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Hong J. Park, Thomas L. Quarles 
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b1defs.h"


int
B1dev::acLoad(sGENmodel *genmod, sCKT *ckt)
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
    sB1model *model = static_cast<sB1model*>(genmod);
    for ( ; model; model = model->next()) {
        sB1instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
        
            if (inst->B1mode >= 0) {
                xnrm = 1;
                xrev = 0;
            }
            else {
                xnrm = 0;
                xrev = 1;
            }
            gdpr = inst->B1drainConductance;
            gspr = inst->B1sourceConductance;
            gm = *(ckt->CKTstate0 + inst->B1gm);
            gds = *(ckt->CKTstate0 + inst->B1gds);
            gmbs = *(ckt->CKTstate0 + inst->B1gmbs);
            gbd = *(ckt->CKTstate0 + inst->B1gbd);
            gbs = *(ckt->CKTstate0 + inst->B1gbs);
            capbd = *(ckt->CKTstate0 + inst->B1capbd);
            capbs = *(ckt->CKTstate0 + inst->B1capbs);

            //
            //    charge oriented model parameters
            //

            cggb = *(ckt->CKTstate0 + inst->B1cggb);
            cgsb = *(ckt->CKTstate0 + inst->B1cgsb);
            cgdb = *(ckt->CKTstate0 + inst->B1cgdb);

            cbgb = *(ckt->CKTstate0 + inst->B1cbgb);
            cbsb = *(ckt->CKTstate0 + inst->B1cbsb);
            cbdb = *(ckt->CKTstate0 + inst->B1cbdb);

            cdgb = *(ckt->CKTstate0 + inst->B1cdgb);
            cdsb = *(ckt->CKTstate0 + inst->B1cdsb);
            cddb = *(ckt->CKTstate0 + inst->B1cddb);

            xcdgb = (cdgb - inst->B1GDoverlapCap) * omega;
            xcddb = (cddb + capbd + inst->B1GDoverlapCap) * omega;
            xcdsb = cdsb * omega;
            xcsgb = -(cggb + cbgb + cdgb + inst->B1GSoverlapCap ) * omega;
            xcsdb = -(cgdb + cbdb + cddb) * omega;
            xcssb = (capbs + inst->B1GSoverlapCap - (cgsb+cbsb+cdsb)) * omega;
            xcggb = (cggb + inst->B1GDoverlapCap + inst->B1GSoverlapCap + 
                inst->B1GBoverlapCap) * omega;
            xcgdb = (cgdb - inst->B1GDoverlapCap ) * omega;
            xcgsb = (cgsb - inst->B1GSoverlapCap) * omega;
            xcbgb = (cbgb - inst->B1GBoverlapCap) * omega;
            xcbdb = (cbdb - capbd ) * omega;
            xcbsb = (cbsb - capbs ) * omega;

#define MSC(xx) inst->B1m*(xx)

            *(inst->B1GgPtr +1) += MSC(xcggb);
            *(inst->B1BbPtr +1) += MSC(-xcbgb-xcbdb-xcbsb);
            *(inst->B1DPdpPtr +1) += MSC(xcddb);
            *(inst->B1SPspPtr +1) += MSC(xcssb);
            *(inst->B1GbPtr +1) += MSC(-xcggb-xcgdb-xcgsb);
            *(inst->B1GdpPtr +1) += MSC(xcgdb);
            *(inst->B1GspPtr +1) += MSC(xcgsb);
            *(inst->B1BgPtr +1) += MSC(xcbgb);
            *(inst->B1BdpPtr +1) += MSC(xcbdb);
            *(inst->B1BspPtr +1) += MSC(xcbsb);
            *(inst->B1DPgPtr +1) += MSC(xcdgb);
            *(inst->B1DPbPtr +1) += MSC(-xcdgb-xcddb-xcdsb);
            *(inst->B1DPspPtr +1) += MSC(xcdsb);
            *(inst->B1SPgPtr +1) += MSC(xcsgb);
            *(inst->B1SPbPtr +1) += MSC(-xcsgb-xcsdb-xcssb);
            *(inst->B1SPdpPtr +1) += MSC(xcsdb);
            *(inst->B1DdPtr) += MSC(gdpr);
            *(inst->B1SsPtr) += MSC(gspr);
            *(inst->B1BbPtr) += MSC(gbd+gbs);
            *(inst->B1DPdpPtr) += MSC(gdpr+gds+gbd+xrev*(gm+gmbs));
            *(inst->B1SPspPtr) += MSC(gspr+gds+gbs+xnrm*(gm+gmbs));
            *(inst->B1DdpPtr) -= MSC(gdpr);
            *(inst->B1SspPtr) -= MSC(gspr);
            *(inst->B1BdpPtr) -= MSC(gbd);
            *(inst->B1BspPtr) -= MSC(gbs);
            *(inst->B1DPdPtr) -= MSC(gdpr);
            *(inst->B1DPgPtr) += MSC((xnrm-xrev)*gm);
            *(inst->B1DPbPtr) += MSC(-gbd+(xnrm-xrev)*gmbs);
            *(inst->B1DPspPtr) += MSC(-gds-xnrm*(gm+gmbs));
            *(inst->B1SPgPtr) += MSC(-(xnrm-xrev)*gm);
            *(inst->B1SPsPtr) -= MSC(gspr);
            *(inst->B1SPbPtr) += MSC(-gbs-(xnrm-xrev)*gmbs);
            *(inst->B1SPdpPtr) += MSC(-gds-xrev*(gm+gmbs));

        }
    }
    return (OK);
}


