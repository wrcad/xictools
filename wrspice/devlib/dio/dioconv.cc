
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
Author: 1985 Thomas L. Quarles
**********/

#include "diodefs.h"

#define DIOnextModel      next()
#define DIOnextInstance   next()
#define DIOinstances      inst()
#define CKTreltol CKTcurTask->TSKreltol
#define CKTabstol CKTcurTask->TSKabstol
#define MAX SPMAX


int
DIOdev::convTest(sGENmodel *genmod, sCKT *ckt)
{
    sDIOmodel *model = static_cast<sDIOmodel*>(genmod);
    sDIOinstance *here;

    double delvd,vd,cdhat,cd;
    double tol;
    /*  loop through all the diode models */
    for( ; model != NULL; model = model->DIOnextModel ) {

        /* loop through all the instances of the model */
        for (here = model->DIOinstances; here != NULL ;
                here=here->DIOnextInstance) {
//            if (here->DIOowner != ARCHme) continue;

            if (here->DIOoff && (ckt->CKTmode & MODEINITFIX))
                continue;
                
            /*  
             *   initialization 
             */

            vd = *(ckt->CKTrhsOld+here->DIOposPrimeNode)-
                    *(ckt->CKTrhsOld + here->DIOnegNode);

            delvd=vd- *(ckt->CKTstate0 + here->DIOvoltage);
            cdhat= *(ckt->CKTstate0 + here->DIOcurrent) + 
                    *(ckt->CKTstate0 + here->DIOconduct) * delvd;

            cd= *(ckt->CKTstate0 + here->DIOcurrent);

            /*
             *   check convergence
             */
            tol=ckt->CKTreltol*
                    MAX(fabs(cdhat),fabs(cd))+ckt->CKTabstol;
            if (fabs(cdhat-cd) > tol) {
                ckt->CKTnoncon++;
//                ckt->CKTtroubleElt = (GENinstance *) here;
                ckt->CKTtroubleElt = here;
                return(OK); /* don't need to check any more device */
            }
        }
    }
    return(OK);
}
