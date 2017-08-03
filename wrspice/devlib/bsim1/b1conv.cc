
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
Authors: 1985 Hong J. Park, Thomas L. Quarles 
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b1defs.h"


int
B1dev::convTest(sGENmodel *genmod, sCKT *ckt)
{
    double cbd;
    double cbhat;
    double cbs;
    double cd;
    double cdhat;
    double delvbd;
    double delvbs;
    double delvds;
    double delvgd;
    double delvgs;
    double tol;
    double vbd;
    double vbs;
    double vds;
    double vgd;
    double vgdo;
    double vgs;

    sB1model *model = static_cast<sB1model*>(genmod);
    for ( ; model; model = model->next()) {
        sB1instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            vbs = model->B1type * ( 
                *(ckt->CKTrhsOld+inst->B1bNode) -
                *(ckt->CKTrhsOld+inst->B1sNodePrime));
            vgs = model->B1type * ( 
                *(ckt->CKTrhsOld+inst->B1gNode) -
                *(ckt->CKTrhsOld+inst->B1sNodePrime));
            vds = model->B1type * ( 
                *(ckt->CKTrhsOld+inst->B1dNodePrime) -
                *(ckt->CKTrhsOld+inst->B1sNodePrime));
            vbd=vbs-vds;
            vgd=vgs-vds;
            vgdo = *(ckt->CKTstate0 + inst->B1vgs) - 
                *(ckt->CKTstate0 + inst->B1vds);
            delvbs = vbs - *(ckt->CKTstate0 + inst->B1vbs);
            delvbd = vbd - *(ckt->CKTstate0 + inst->B1vbd);
            delvgs = vgs - *(ckt->CKTstate0 + inst->B1vgs);
            delvds = vds - *(ckt->CKTstate0 + inst->B1vds);
            delvgd = vgd-vgdo;

            if (inst->B1mode >= 0) {
                cdhat=
                    *(ckt->CKTstate0 + inst->B1cd) -
                    *(ckt->CKTstate0 + inst->B1gbd) * delvbd +
                    *(ckt->CKTstate0 + inst->B1gmbs) * delvbs +
                    *(ckt->CKTstate0 + inst->B1gm) * delvgs + 
                    *(ckt->CKTstate0 + inst->B1gds) * delvds ;
            }
            else {
                cdhat=
                    *(ckt->CKTstate0 + inst->B1cd) -
                    ( *(ckt->CKTstate0 + inst->B1gbd) -
                      *(ckt->CKTstate0 + inst->B1gmbs)) * delvbd -
                    *(ckt->CKTstate0 + inst->B1gm) * delvgd +
                    *(ckt->CKTstate0 + inst->B1gds) * delvds;
            }
            cbhat=
                *(ckt->CKTstate0 + inst->B1cbs) +
                *(ckt->CKTstate0 + inst->B1cbd) +
                *(ckt->CKTstate0 + inst->B1gbd) * delvbd +
                *(ckt->CKTstate0 + inst->B1gbs) * delvbs ;

            cd = *(ckt->CKTstate0 + inst->B1cd);
            cbs = *(ckt->CKTstate0 + inst->B1cbs);
            cbd = *(ckt->CKTstate0 + inst->B1cbd);
            //
            //  check convergence
            //
            if ( (inst->B1off == 0)  || (!(ckt->CKTmode & MODEINITFIX)) ){
                tol=ckt->CKTcurTask->TSKreltol*SPMAX(FABS(cdhat),FABS(cd)) +
                    ckt->CKTcurTask->TSKabstol;
                if (FABS(cdhat-cd) >= tol) { 
                    ckt->CKTnoncon++;
                    ckt->CKTtroubleElt = inst;
                    return (OK);
                } 
                tol=ckt->CKTcurTask->TSKreltol*SPMAX(FABS(cbhat),FABS(cbs+cbd))+
                    ckt->CKTcurTask->TSKabstol;
                if (FABS(cbhat-(cbs+cbd)) > tol) {
                    ckt->CKTnoncon++;
                    ckt->CKTtroubleElt = inst;
                    return (OK);
                }
            }
        }
    }
    return (OK);
}

