
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
 File: hsm1trunc.c of HiSIM v1.1.0

 Copyright (C) 2002 STARC

 June 30, 2002: developed by Hiroshima University and STARC
 June 30, 2002: posted by Keiichi MORIKAWA, STARC Physical Design Group
***********************************************************************/

#include "hsm1defs.h"

#define HSM1nextModel      next()
#define HSM1nextInstance   next()
#define HSM1instances      inst()
#define CKTterr(a, b, c) (b)->terr(a, c)


int
HSM1dev::trunc(sGENmodel *genmod, sCKT *ckt, double *timeStep)
{
    sHSM1model *model = static_cast<sHSM1model*>(genmod);
    sHSM1instance *here;

#ifdef STEPDEBUG
  double debugtemp;
#endif /* STEPDEBUG */
  
  for ( ;model != NULL ;model = model->HSM1nextModel ) {
    for ( here=model->HSM1instances ;here!=NULL ;
          here = here->HSM1nextInstance ) {
#ifdef STEPDEBUG
      debugtemp = *timeStep;
#endif /* STEPDEBUG */
      CKTterr(here->HSM1qb,ckt,timeStep);
      CKTterr(here->HSM1qg,ckt,timeStep);
      CKTterr(here->HSM1qd,ckt,timeStep);
#ifdef STEPDEBUG
      if ( debugtemp != *timeStep ) 
        printf("device %s reduces step from %g to %g\n",
               here->HSM1name, debugtemp, *timeStep);
#endif /* STEPDEBUG */
    }
  }
  return(OK);
}

