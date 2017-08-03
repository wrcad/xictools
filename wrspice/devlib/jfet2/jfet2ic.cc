
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
Based on jfetic.c
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles

Modified to jfet2 for PS model definition ( Anthony E. Parker )
   Copyright 1994  Macquarie University, Sydney Australia.
**********/

#include "jfet2defs.h"

#define JFET2nextModel      next()
#define JFET2nextInstance   next()
#define JFET2instances      inst()


int
JFET2dev::getic(sGENmodel *genmod, sCKT *ckt)
{
    sJFET2model *model = static_cast<sJFET2model*>(genmod);
    sJFET2instance *here;
    /*
     * grab initial conditions out of rhs array.   User specified, so use
     * external nodes to get values
     */

    for( ; model ; model = model->JFET2nextModel) {
        for(here = model->JFET2instances; here ; here = here->JFET2nextInstance) {
            if(!here->JFET2icVDSGiven) {
                here->JFET2icVDS = 
                        *(ckt->CKTrhs + here->JFET2drainNode) - 
                        *(ckt->CKTrhs + here->JFET2sourceNode);
            }
            if(!here->JFET2icVGSGiven) {
                here->JFET2icVGS = 
                        *(ckt->CKTrhs + here->JFET2gateNode) - 
                        *(ckt->CKTrhs + here->JFET2sourceNode);
            }
        }
    }
    return(OK);
}
