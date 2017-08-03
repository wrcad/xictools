
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
Author: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
File: b3soicvtest.c          98/5/01
**********/

#include "b3sdefs.h"

#define B3SOInextModel      next()
#define B3SOInextInstance   next()
#define B3SOIinstances      inst()
#define CKTreltol CKTcurTask->TSKreltol
#define CKTabstol CKTcurTask->TSKabstol
#define MAX SPMAX


int
B3SOIdev::convTest(sGENmodel *genmod, sCKT *ckt)
{
    sB3SOImodel *model = static_cast<sB3SOImodel*>(genmod);
    sB3SOIinstance *here;

    double delvbd, delvbs, delvds, delvgd, delvgs, vbd, vbs, vds;
    double cbd, cbhat, cbs, cd, cdhat, tol, vgd, vgdo, vgs;

    /*  loop through all the B3SOI device models */
    for (; model != NULL; model = model->B3SOInextModel)
    {
        /* loop through all the instances of the model */
        for (here = model->B3SOIinstances; here != NULL ;
                here=here->B3SOInextInstance)
        {
// SRW -- Check this here to avoid computations below
            if (here->B3SOIoff && (ckt->CKTmode & MODEINITFIX))
                continue;

            vbs = model->B3SOItype
                  * (*(ckt->CKTrhsOld+here->B3SOIbNode)
                     - *(ckt->CKTrhsOld+here->B3SOIsNodePrime));
            vgs = model->B3SOItype
                  * (*(ckt->CKTrhsOld+here->B3SOIgNode)
                     - *(ckt->CKTrhsOld+here->B3SOIsNodePrime));
            vds = model->B3SOItype
                  * (*(ckt->CKTrhsOld+here->B3SOIdNodePrime)
                     - *(ckt->CKTrhsOld+here->B3SOIsNodePrime));
            vbd = vbs - vds;
            vgd = vgs - vds;
            vgdo = *(ckt->CKTstate0 + here->B3SOIvgs)
                   - *(ckt->CKTstate0 + here->B3SOIvds);
            delvbs = vbs - *(ckt->CKTstate0 + here->B3SOIvbs);
            delvbd = vbd - *(ckt->CKTstate0 + here->B3SOIvbd);
            delvgs = vgs - *(ckt->CKTstate0 + here->B3SOIvgs);
            delvds = vds - *(ckt->CKTstate0 + here->B3SOIvds);
            delvgd = vgd-vgdo;

            cd = here->B3SOIcd;
            if (here->B3SOImode >= 0)
            {
                cdhat = cd - here->B3SOIgjdb * delvbd
                        + here->B3SOIgmbs * delvbs + here->B3SOIgm * delvgs
                        + here->B3SOIgds * delvds;
            }
            else
            {
                cdhat = cd - (here->B3SOIgjdb - here->B3SOIgmbs) * delvbd
                        - here->B3SOIgm * delvgd + here->B3SOIgds * delvds;
            }

            /*
             *  check convergence
             */
// SRW              if ((here->B3SOIoff == 0)  || (!(ckt->CKTmode & MODEINITFIX)))
            {
                tol = ckt->CKTreltol * MAX(FABS(cdhat), FABS(cd))
                      + ckt->CKTabstol;
                if (FABS(cdhat - cd) >= tol)
                {
                    ckt->CKTnoncon++;
                    return(OK);
                }
                cbs = here->B3SOIcjs;
                cbd = here->B3SOIcjd;
                cbhat = cbs + cbd + here->B3SOIgjdb * delvbd
                        + here->B3SOIgjsb * delvbs;
                tol = ckt->CKTreltol * MAX(FABS(cbhat), FABS(cbs + cbd))
                      + ckt->CKTabstol;
                if (FABS(cbhat - (cbs + cbd)) > tol)
                {
                    ckt->CKTnoncon++;
                    return(OK);
                }
            }
        }
    }
    return(OK);
}

