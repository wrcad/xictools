
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
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
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "resdefs.h"


#define TSTALLOC_1(ptr, first, second) \
    if ((inst->ptr = ckt->alloc(inst->first, second)) == 0) { \
        return (E_NOMEM); }

#define TSTALLOC_2(ptr, first, second) \
    if ((inst->ptr = ckt->alloc(inst->first, second->number())) == 0) { \
        return (E_NOMEM); }

int
RESdev::setup(sGENmodel *genmod, sCKT *ckt, int*)
{
    sRESmodel *model = static_cast<sRESmodel*>(genmod);
    for ( ; model; model = model->next()) {
        if (!model->RESmGiven)
            model->RESm = 1.0;
        else if (model->RESm < 1e-3 || model->RESm > 1e3) {
            DVO.textOut(OUT_FATAL,
                "%s: M value out or range [1e-3,1e3].", model->GENmodName);
            return (E_BADPARM);
        }

        sRESinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            TSTALLOC(RESposPosptr, RESposNode, RESposNode);
            TSTALLOC(RESnegNegptr, RESnegNode, RESnegNode);
            TSTALLOC(RESposNegptr, RESposNode, RESnegNode);
            TSTALLOC(RESnegPosptr, RESnegNode, RESposNode);

            if (!inst->RESmGiven)
                inst->RESm = model->RESm;
            else if (inst->RESm < 1e-3 || inst->RESm > 1e3) {
                DVO.textOut(OUT_FATAL,
                    "%s: M value out or range [1e-3,1e3].", inst->GENname);
                return (E_BADPARM);
            }

            if (inst->RESpolyCoeffs)
                continue;
            if (!inst->REStree)
                continue;
            int numvars = inst->REStree->num_vars();
            if (!numvars)
                continue;

            if (!inst->RESvalues)
                inst->RESvalues =  new double[numvars];
            if (!inst->RESderivs)
                inst->RESderivs = new double[numvars];
            if (!inst->RESeqns)
                inst->RESeqns = new int[numvars];
            if (!inst->RESposptr)
                inst->RESposptr = new double*[2*numvars];

            for (int j = 0, i = 0; i < numvars; i++) {

                if (inst->REStree->vars()[i].type == IF_INSTANCE) {
                    int branch =
                        ckt->findBranch(inst->REStree->vars()[i].v.uValue);
                    if (branch == 0) {
                        DVO.textOut(OUT_FATAL,
                            "%s: unknown controlling source %s",
                            inst->GENname,
                            inst->REStree->vars()[i].v.uValue);
                        return (E_BADPARM);
                    }

                    TSTALLOC_1(RESposptr[j++], RESposNode, branch);
                    TSTALLOC_1(RESposptr[j++], RESnegNode, branch);
                    inst->RESeqns[i] = branch;
                }
                else if (inst->REStree->vars()[i].type == IF_NODE) {
                    TSTALLOC_2(RESposptr[j++], RESposNode,
                        inst->REStree->vars()[i].v.nValue);
                    TSTALLOC_2(RESposptr[j++], RESnegNode,
                        inst->REStree->vars()[i].v.nValue);
                    inst->RESeqns[i] =
                        inst->REStree->vars()[i].v.nValue->number();
                }
            }
        }
    }
    return (OK);
}


// SRW - reset the matrix element pointers.
//
int
RESdev::resetup(sGENmodel *genmod, sCKT *ckt)
{
    sRESmodel *model = static_cast<sRESmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sRESinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            TSTALLOC(RESposPosptr, RESposNode, RESposNode);
            TSTALLOC(RESnegNegptr, RESnegNode, RESnegNode);
            TSTALLOC(RESposNegptr, RESposNode, RESnegNode);
            TSTALLOC(RESnegPosptr, RESnegNode, RESposNode);

            if (!inst->REStree)
                continue;
            int numvars = inst->REStree->num_vars();
            if (!numvars)
                continue;
            for (int j = 0, i = 0; i < numvars; i++) {

                if (inst->REStree->vars()[i].type == IF_INSTANCE) {
                    TSTALLOC_1(RESposptr[j++], RESposNode, inst->RESeqns[i]);
                    TSTALLOC_1(RESposptr[j++], RESnegNode, inst->RESeqns[i]);
                }
                else if (inst->REStree->vars()[i].type == IF_NODE) {
                    TSTALLOC_2(RESposptr[j++], RESposNode,
                        inst->REStree->vars()[i].v.nValue);
                    TSTALLOC_2(RESposptr[j++], RESnegNode,
                        inst->REStree->vars()[i].v.nValue);
                    inst->RESeqns[i] =
                        inst->REStree->vars()[i].v.nValue->number();
                }
            }
        }
    }
    return (OK);
}

