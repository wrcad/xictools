
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
 $Id: resacld.cc,v 1.3 2011/07/13 18:27:01 stevew Exp $
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

