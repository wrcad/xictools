
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

/*
 * Author: 2000 Wladek Grabinski; EKV v2.6 Model Upgrade
 * Author: 1997 Eckhard Brass;    EKV v2.5 Model Implementation
 *     (C) 1990 Regents of the University of California. Spice3 Format
 */

#include "ekvdefs.h"

#define EKVnextModel      next()
#define EKVnextInstance   next()
#define EKVinstances      inst()
#define CKTreltol CKTcurTask->TSKreltol
#define CKTabstol CKTcurTask->TSKabstol
#define MAX SPMAX
#define GENinstance sGENinstance


int
EKVdev::convTest(sGENmodel *genmod, sCKT *ckt)
{
    sEKVmodel *model = static_cast<sEKVmodel*>(genmod);
    sEKVinstance *here;

    double delvbs;
    double delvbd;
    double delvgs;
    double delvds;
    double delvgd;
    double cbhat;
    double cdhat;
    double vbs;
    double vbd;
    double vgs;
    double vds;
    double vgd;
    double vgdo;
    double tol;

    for( ; model != NULL; model = model->EKVnextModel) {
        for(here = model->EKVinstances; here!= NULL;
            here = here->EKVnextInstance) {

            vbs = model->EKVtype * ( 
                *(ckt->CKTrhs+here->EKVbNode) -
                *(ckt->CKTrhs+here->EKVsNodePrime));
            vgs = model->EKVtype * ( 
                *(ckt->CKTrhs+here->EKVgNode) -
                *(ckt->CKTrhs+here->EKVsNodePrime));
            vds = model->EKVtype * ( 
                *(ckt->CKTrhs+here->EKVdNodePrime) -
                *(ckt->CKTrhs+here->EKVsNodePrime));
            vbd=vbs-vds;
            vgd=vgs-vds;
            vgdo = *(ckt->CKTstate0 + here->EKVvgs) -
                *(ckt->CKTstate0 + here->EKVvds);
            delvbs = vbs - *(ckt->CKTstate0 + here->EKVvbs);
            delvbd = vbd - *(ckt->CKTstate0 + here->EKVvbd);
            delvgs = vgs - *(ckt->CKTstate0 + here->EKVvgs);
            delvds = vds - *(ckt->CKTstate0 + here->EKVvds);
            delvgd = vgd-vgdo;

            /* these are needed for convergence testing */

            if (here->EKVmode >= 0) {
                cdhat=
                    here->EKVcd-
                    here->EKVgbd * delvbd +
                    here->EKVgmbs * delvbs +
                    here->EKVgm * delvgs + 
                    here->EKVgds * delvds ;
            } else {
                cdhat=
                    here->EKVcd -
                    ( here->EKVgbd -
                    here->EKVgmbs) * delvbd -
                    here->EKVgm * delvgd + 
                    here->EKVgds * delvds ;
            }
            cbhat=
                here->EKVcbs +
                here->EKVcbd +
                here->EKVgbd * delvbd +
                here->EKVgbs * delvbs ;
            /*
             *  check convergence
             */
            tol=ckt->CKTreltol*MAX(FABS(cdhat),FABS(here->EKVcd))+
                ckt->CKTabstol;
            if (FABS(cdhat-here->EKVcd) >= tol) {
                ckt->CKTnoncon++;
                ckt->CKTtroubleElt = (GENinstance *) here;
                return(OK); /* no reason to continue, we haven't converged */
            } else {
                tol=ckt->CKTreltol*
                    MAX(FABS(cbhat),FABS(here->EKVcbs+here->EKVcbd))+
                    ckt->CKTabstol;
                if (FABS(cbhat-(here->EKVcbs+here->EKVcbd)) > tol) {
                    ckt->CKTnoncon++;
                    ckt->CKTtroubleElt = (GENinstance *) here;
                    return(OK); /* no reason to continue, we haven't converged*/
                }
            }
        }
    }
    return(OK);
}

