
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
 $Id: b1trun.cc,v 1.0 1998/01/30 05:27:02 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Hong J. Park, Thomas L. Quarles 
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b1defs.h"


int
B1dev::trunc(sGENmodel *genmod, sCKT *ckt, double *timeStep)
{
#ifdef STEPDEBUG
    double debugtemp;
#endif

    sB1model *model = static_cast<sB1model*>(genmod);
    for ( ; model; model = model->next()) {
        sB1instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
#ifdef STEPDEBUG
            debugtemp = *timeStep;
#endif
            ckt->terr(inst->B1qb, timeStep);
            ckt->terr(inst->B1qg, timeStep);
            ckt->terr(inst->B1qd, timeStep);
#ifdef STEPDEBUG
            if (debugtemp != *timeStep) {
                printf("device %s reduces step from %g to %g\n",
                    inst->GENname, debugtemp, *timeStep);
            }
#endif
        }
    }
    return (OK);
}

