
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
 $Id: ufstrun.cc,v 2.8 2002/09/30 13:01:48 stevew Exp $
 *========================================================================*/

/**********
Copyright 1997 University of Florida.  All rights reserved.
Author: Min-Chie Jeng (For SPICE3E2)
File: ufstrunc.c
**********/

#include "ufsdefs.h"
#include "errors.h"

// This function checks the truncation error of storage elements for
// timestep setting.  Basically, all it needs to do is call
// ckt->terr() for each reactive subelement.


int
UFSdev::trunc(sGENmodel *genmod, sCKT *ckt, double *timeStep)
{
    sUFSmodel *model = static_cast<sUFSmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sUFSinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            ckt->terr(inst->UFSqb, timeStep);
            ckt->terr(inst->UFSqg, timeStep);
            ckt->terr(inst->UFSqd, timeStep);
        }
    }
    return (OK);
}
