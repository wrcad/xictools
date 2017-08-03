
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

/**** BSIM4.3.0 Released by Xuemei(Jane) Xi 05/09/2003 ****/

/**********
 * Copyright 2003 Regents of the University of California. All rights reserved.
 * File: b4check.c of BSIM4.3.0.
 * Author: 2000 Weidong Liu
 * Authors: 2001- Xuemei Xi, Jin He, Kanyu Cao, Mohan Dunga, Mansun Chan, Ali Niknejad, Chenming Hu.
 * Project Director: Prof. Chenming Hu.
 **********/

#include "b4defs.h"

#define BSIM4nextModel      next()
#define BSIM4nextInstance   next()
#define BSIM4instances      inst()
#define CKTterr(a, b, c) (b)->terr(a, c)


int
BSIM4dev::trunc(sGENmodel *genmod, sCKT *ckt, double *timeStep)
{
    sBSIM4model *model = static_cast<sBSIM4model*>(genmod);
    sBSIM4instance *here;

#ifdef STEPDEBUG
    double debugtemp;
#endif /* STEPDEBUG */

    for (; model != NULL; model = model->BSIM4nextModel)
    {
        for (here = model->BSIM4instances; here != NULL;
                here = here->BSIM4nextInstance)
        {
#ifdef STEPDEBUG
            debugtemp = *timeStep;
#endif /* STEPDEBUG */
            CKTterr(here->BSIM4qb,ckt,timeStep);
            CKTterr(here->BSIM4qg,ckt,timeStep);
            CKTterr(here->BSIM4qd,ckt,timeStep);
            if (here->BSIM4trnqsMod)
                CKTterr(here->BSIM4qcdump,ckt,timeStep);
            if (here->BSIM4rbodyMod)
            {
                CKTterr(here->BSIM4qbs,ckt,timeStep);
                CKTterr(here->BSIM4qbd,ckt,timeStep);
            }
            if (here->BSIM4rgateMod == 3)
                CKTterr(here->BSIM4qgmid,ckt,timeStep);
#ifdef STEPDEBUG
            if(debugtemp != *timeStep)
            {
                printf("device %s reduces step from %g to %g\n",
                       here->BSIM4name,debugtemp,*timeStep);
            }
#endif /* STEPDEBUG */
        }
    }
    return(OK);
}

