
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
 $Id: b3trun.cc,v 1.2 2011/12/18 01:15:23 stevew Exp $
 *========================================================================*/

/**** BSIM3v3.3.0, Released by Xuemei Xi 07/29/2005 ****/

/**********
 * Copyright 2004 Regents of the University of California. All rights reserved.
 * File: b3trunc.c of BSIM3v3.3.0
 * Author: 1995 Min-Chie Jeng and Mansun Chan.
 * Author: 1997-1999 Weidong Liu.
 * Author: 2001  Xuemei Xi
 **********/

#include "b3defs.h"

#define BSIM3nextModel      next()
#define BSIM3nextInstance   next()
#define BSIM3instances      inst()
#define CKTterr(a, b, c) (b)->terr(a, c)


int
BSIM3dev::trunc(sGENmodel *genmod, sCKT *ckt, double *timeStep)
{
    sBSIM3model *model = static_cast<sBSIM3model*>(genmod);
    sBSIM3instance *here;

#ifdef STEPDEBUG
    double debugtemp;
#endif /* STEPDEBUG */

    for (; model != NULL; model = model->BSIM3nextModel)
    {
        for (here = model->BSIM3instances; here != NULL;
                here = here->BSIM3nextInstance)
        {
#ifdef STEPDEBUG
            debugtemp = *timeStep;
#endif /* STEPDEBUG */
            CKTterr(here->BSIM3qb,ckt,timeStep);
            CKTterr(here->BSIM3qg,ckt,timeStep);
            CKTterr(here->BSIM3qd,ckt,timeStep);
#ifdef STEPDEBUG
            if(debugtemp != *timeStep)
            {
                printf("device %s reduces step from %g to %g\n",
                       here->BSIM3name,debugtemp,*timeStep);
            }
#endif /* STEPDEBUG */
        }
    }
    return(OK);
}

