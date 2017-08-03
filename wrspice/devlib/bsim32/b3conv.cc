
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


int
B3dev::convTest(sGENmodel *genmod, sCKT *ckt)
{
    double delvbd, delvbs, delvds, delvgd, delvgs, vbd, vbs, vds;
    double cbd, cbhat, cbs, cd, cdhat, tol, vgd, vgdo, vgs;

    sB3model *model = static_cast<sB3model*>(genmod);
    for ( ; model; model = model->next()) {

        sB3instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (inst->B3off && (ckt->CKTmode & MODEINITFIX))
                continue;

            vbs = model->B3type
                * (*(ckt->CKTrhsOld+inst->B3bNode) 
                - *(ckt->CKTrhsOld+inst->B3sNodePrime));
            vgs = model->B3type
                * (*(ckt->CKTrhsOld+inst->B3gNode) 
                - *(ckt->CKTrhsOld+inst->B3sNodePrime));
            vds = model->B3type
                * (*(ckt->CKTrhsOld+inst->B3dNodePrime) 
                - *(ckt->CKTrhsOld+inst->B3sNodePrime));
            vbd = vbs - vds;
            vgd = vgs - vds;
            vgdo = *(ckt->CKTstate0 + inst->B3vgs) 
                 - *(ckt->CKTstate0 + inst->B3vds);
            delvbs = vbs - *(ckt->CKTstate0 + inst->B3vbs);
            delvbd = vbd - *(ckt->CKTstate0 + inst->B3vbd);
            delvgs = vgs - *(ckt->CKTstate0 + inst->B3vgs);
            delvds = vds - *(ckt->CKTstate0 + inst->B3vds);
            delvgd = vgd-vgdo;

            cd = inst->B3cd - inst->B3cbd;
            if (inst->B3mode >= 0) {
	            cd += inst->B3csub;
		        cdhat = cd - inst->B3gbd * delvbd 
			        + (inst->B3gmbs + inst->B3gbbs) * delvbs
			        + (inst->B3gm + inst->B3gbgs) * delvgs
			        + (inst->B3gds + inst->B3gbds) * delvds;
            }
            else {
	            cdhat = cd + (inst->B3gmbs - inst->B3gbd) * delvbd 
			        + inst->B3gm * delvgd - inst->B3gds * delvds;
            }

            //
            //  check convergence
            //
            tol = ckt->CKTcurTask->TSKreltol * SPMAX(FABS(cdhat), FABS(cd))
                + ckt->CKTcurTask->TSKabstol;
            if (FABS(cdhat - cd) >= tol) {
                ckt->CKTnoncon++;
                return(OK);
            } 
            cbs = inst->B3cbs;
            cbd = inst->B3cbd;
            if (inst->B3mode >= 0) {
                cbhat = cbs + cbd - inst->B3csub
                    + inst->B3gbd * delvbd 
                    + (inst->B3gbs - inst->B3gbbs) * delvbs
                    - inst->B3gbgs * delvgs
                    - inst->B3gbds * delvds;
            }
            else {
                cbhat = cbs + cbd - inst->B3csub
                    + inst->B3gbs * delvbs
                    + (inst->B3gbd - inst->B3gbbs) * delvbd 
                    - inst->B3gbgs * delvgd
                    + inst->B3gbds * delvds;
            }
            tol = ckt->CKTcurTask->TSKreltol * SPMAX(FABS(cbhat), 
                FABS(cbs + cbd - inst->B3csub)) +
                ckt->CKTcurTask->TSKabstol;
            if (FABS(cbhat - (cbs + cbd - inst->B3csub)) > tol)  {
                ckt->CKTnoncon++;
                return(OK);
            }
        }
    }
    return (OK);
}

