
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "capdefs.h"


namespace {
    int get_node_ptr(sCKT *ckt, sCAPinstance *inst)
    {
        TSTALLOC(CAPposPosptr, CAPposNode, CAPposNode)
        TSTALLOC(CAPnegNegptr, CAPnegNode, CAPnegNode)
        TSTALLOC(CAPposNegptr, CAPposNode, CAPnegNode)
        TSTALLOC(CAPnegPosptr, CAPnegNode, CAPposNode)
        return (OK);
    }
}


int
CAPdev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
    (void)ckt;
    sCAPmodel *model = static_cast<sCAPmodel*>(genmod);
    for ( ; model; model = model->next()) {

        // Default Value Processing for Model Parameters
        if (!model->CAPcjGiven)
            model->CAPcj = 0;
        if (!model->CAPcjswGiven)
             model->CAPcjsw = 0;
        if (!model->CAPdefWidthGiven)
            model->CAPdefWidth = 10.e-6;
        if (!model->CAPnarrowGiven)
            model->CAPnarrow = 0;

        if (!model->CAPmGiven)
            model->CAPm = 1.0;
        else if (model->CAPm < 1e-3 || model->CAPm > 1e3) {
            DVO.textOut(OUT_FATAL,
                "%s: M value out or range [1e-3,1e3].", model->GENmodName);
            return (E_BADPARM);
        }

        sCAPinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            // Default Value Processing for Capacitor Instance
            if (!inst->CAPlengthGiven)
                inst->CAPlength = 0;

            if (!inst->CAPmGiven)
                inst->CAPm = model->CAPm;
            else if (inst->CAPm < 1e-3 || inst->CAPm > 1e3) {
                DVO.textOut(OUT_FATAL,
                    "%s: M value out or range [1e-3,1e3].", inst->GENname);
                return (E_BADPARM);
            }

            inst->CAPqcap = *states;
            *states += 2;

            int error = get_node_ptr(ckt, inst);
            if (error != OK)
                return (error);

            if (inst->CAPpolyNumCoeffs > 0) {
                // We save the last cap voltage.
                *states += 1;
                if (!inst->CAPvalues)
                    inst->CAPvalues = new double[1];
                continue;
            }

            if (!inst->CAPtree)
                continue;
            int numvars = inst->CAPtree->num_vars();
            if (!numvars)
                continue;

            if (!inst->CAPvalues)
                inst->CAPvalues = new double[numvars];
            if (!inst->CAPeqns)
                inst->CAPeqns = new int[numvars];

            *states += numvars;

            for (int i = 0; i < numvars; i++) {

                if (inst->CAPtree->vars()[i].type == IF_INSTANCE) {
                    int branch =
                        ckt->findBranch(inst->CAPtree->vars()[i].v.uValue);
                    if (branch == 0) {
                        DVO.textOut(OUT_FATAL,
                            "%s: unknown controlling source %s",
                            inst->GENname,
                            inst->CAPtree->vars()[i].v.uValue);
                        return (E_BADPARM);
                    }
                    inst->CAPeqns[i] = branch;
                }
                else if (inst->CAPtree->vars()[i].type == IF_NODE) {
                    inst->CAPeqns[i] =
                        inst->CAPtree->vars()[i].v.nValue->number();
                }
            }
        }
    }
    return (OK);
}


// SRW - reset the matrix element pointers.
//
int
CAPdev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sCAPmodel *model = (sCAPmodel*)inModel; model;
            model = model->next()) {
        for (sCAPinstance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}

