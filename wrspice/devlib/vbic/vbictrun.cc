
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
Model Author: 1995 Colin McAndrew Motorola
Spice3 Implementation: 2003 Dietmar Warning DAnalyse GmbH
**********/

#include "vbicdefs.h"

#define VBICnextModel      next()
#define VBICnextInstance   next()
#define VBICinstances      inst()
#define CKTterr(a, b, c) (b)->terr(a, c)


// This routine performs truncation error calculations for
// VBICs in the circuit.
//
int
VBICdev::trunc(sGENmodel *genmod, sCKT *ckt, double *timeStep)
{
    sVBICmodel *model = static_cast<sVBICmodel*>(genmod);
    sVBICinstance *here;

    for( ; model != NULL; model = model->VBICnextModel) {
        for(here=model->VBICinstances;here!=NULL;
            here = here->VBICnextInstance){
//            if (here->VBICowner != ARCHme) continue;

            CKTterr(here->VBICqbe,ckt,timeStep);
            CKTterr(here->VBICqbex,ckt,timeStep);
            CKTterr(here->VBICqbc,ckt,timeStep);
            CKTterr(here->VBICqbcx,ckt,timeStep);
            CKTterr(here->VBICqbep,ckt,timeStep);
            CKTterr(here->VBICqbeo,ckt,timeStep);
            CKTterr(here->VBICqbco,ckt,timeStep);
            CKTterr(here->VBICqbcp,ckt,timeStep);
        }
    }
    return(OK);
}
