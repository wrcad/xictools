
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

/***********************************************************************
 HiSIM v1.1.0
 File: hsm1cvtest.c of HiSIM v1.1.0

 Copyright (C) 2002 STARC

 June 30, 2002: developed by Hiroshima University and STARC
 June 30, 2002: posted by Keiichi MORIKAWA, STARC Physical Design Group
***********************************************************************/

#include "hsm1defs.h"

#define HSM1nextModel      next()
#define HSM1nextInstance   next()
#define HSM1instances      inst()
#define CKTreltol CKTcurTask->TSKreltol
#define CKTabstol CKTcurTask->TSKabstol
#define MAX SPMAX


int
HSM1dev::convTest(sGENmodel *genmod, sCKT *ckt)
{
    sHSM1model *model = static_cast<sHSM1model*>(genmod);
    sHSM1instance *here;

  double delvbd, delvbs, delvds, delvgd, delvgs, vbd, vbs, vds;
  double cbd, cbhat, cbs, cd, cdhat, tol, vgd, vgdo, vgs;

  /*  loop through all the HSM1 device models */
  for ( ; model != NULL; model = model->HSM1nextModel ) {
    /* loop through all the instances of the model */
    for ( here = model->HSM1instances; here != NULL ;
          here = here->HSM1nextInstance ) {
      vbs = model->HSM1_type * 
        (*(ckt->CKTrhsOld+here->HSM1bNode) - 
         *(ckt->CKTrhsOld+here->HSM1sNodePrime));
      vgs = model->HSM1_type *
        (*(ckt->CKTrhsOld+here->HSM1gNode) - 
         *(ckt->CKTrhsOld+here->HSM1sNodePrime));
      vds = model->HSM1_type * 
        (*(ckt->CKTrhsOld+here->HSM1dNodePrime) - 
         *(ckt->CKTrhsOld+here->HSM1sNodePrime));
      vbd = vbs - vds;
      vgd = vgs - vds;
      vgdo = *(ckt->CKTstate0 + here->HSM1vgs) - 
        *(ckt->CKTstate0 + here->HSM1vds);
      delvbs = vbs - *(ckt->CKTstate0 + here->HSM1vbs);
      delvbd = vbd - *(ckt->CKTstate0 + here->HSM1vbd);
      delvgs = vgs - *(ckt->CKTstate0 + here->HSM1vgs);
      delvds = vds - *(ckt->CKTstate0 + here->HSM1vds);
      delvgd = vgd - vgdo;

      cd = here->HSM1_ids - here->HSM1_ibd;
      if ( here->HSM1_mode >= 0 ) {
        cd += here->HSM1_isub;
        cdhat = cd - here->HSM1_gbd * delvbd 
          + (here->HSM1_gmbs + here->HSM1_gbbs) * delvbs
          + (here->HSM1_gm + here->HSM1_gbgs) * delvgs
          + (here->HSM1_gds + here->HSM1_gbds) * delvds;
      }
      else {
        cdhat = cd + (here->HSM1_gmbs - here->HSM1_gbd) * delvbd 
          + here->HSM1_gm * delvgd - here->HSM1_gds * delvds;
      }

      /*
       *  check convergence
       */
      if ( here->HSM1_off == 0  || !(ckt->CKTmode & MODEINITFIX) ) {
        tol = ckt->CKTreltol * MAX(FABS(cdhat), FABS(cd)) + ckt->CKTabstol;
        if ( FABS(cdhat - cd) >= tol ) {
          ckt->CKTnoncon++;
          return(OK);
        } 
        cbs = here->HSM1_ibs;
        cbd = here->HSM1_ibd;
        if ( here->HSM1_mode >= 0 ) {
          cbhat = cbs + cbd - here->HSM1_isub + here->HSM1_gbd * delvbd 
            + (here->HSM1_gbs - here->HSM1_gbbs) * delvbs
            - here->HSM1_gbgs * delvgs - here->HSM1_gbds * delvds;
        }
        else {
          cbhat = cbs + cbd - here->HSM1_isub 
            + here->HSM1_gbs * delvbs
            + (here->HSM1_gbd - here->HSM1_gbbs) * delvbd 
            - here->HSM1_gbgs * delvgd + here->HSM1_gbds * delvds;
        }
        tol = ckt->CKTreltol * 
          MAX(FABS(cbhat), FABS(cbs + cbd - here->HSM1_isub)) + ckt->CKTabstol;
        if ( FABS(cbhat - (cbs + cbd - here->HSM1_isub)) > tol ) {
          ckt->CKTnoncon++;
          return(OK);
        }
      }
    }
  }
  return(OK);
}

