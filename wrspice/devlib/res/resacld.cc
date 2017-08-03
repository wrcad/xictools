
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

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "resdefs.h"


int
RESdev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    (void)ckt;
    sRESmodel *model = static_cast<sRESmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sRESinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (inst->REStree && inst->REStree->num_vars() > 0) {
                int numvars = inst->REStree->num_vars();
                double F = inst->RESconduct * inst->RESconduct * inst->RESv;
                F *= inst->REStcFactor;
                for (int j = 0, i = 0; i < numvars; i++) {
                    *(inst->RESposptr[j++]) -= F * inst->RESderivs[i];
                    *(inst->RESposptr[j++]) += F * inst->RESderivs[i];
                }
            }
            
            *(inst->RESposPosptr) += inst->RESconduct;
            *(inst->RESnegNegptr) += inst->RESconduct;
            *(inst->RESposNegptr) -= inst->RESconduct;
            *(inst->RESnegPosptr) -= inst->RESconduct;
        }
    }
    return (OK);
}

