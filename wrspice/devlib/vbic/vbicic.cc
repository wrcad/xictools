
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


// This routine gets the device initial conditions for the VBICs
// from the RHS vector
//
int
VBICdev::getic(sGENmodel *genmod, sCKT *ckt)
{
    sVBICmodel *model = static_cast<sVBICmodel*>(genmod);
    sVBICinstance *here;

    /*
     * grab initial conditions out of rhs array.   User specified, so use
     * external nodes to get values
     */

    for( ; model ; model = model->VBICnextModel) {
        for(here = model->VBICinstances; here ; here = here->VBICnextInstance) {

//            if (here->VBICowner != ARCHme) continue;

            if(!here->VBICicVBEGiven) {
                here->VBICicVBE = 
                        *(ckt->CKTrhs + here->VBICbaseNode) - 
                        *(ckt->CKTrhs + here->VBICemitNode);
            }
            if(!here->VBICicVCEGiven) {
                here->VBICicVCE = 
                        *(ckt->CKTrhs + here->VBICcollNode) - 
                        *(ckt->CKTrhs + here->VBICemitNode);
            }
        }
    }
    return(OK);
}
