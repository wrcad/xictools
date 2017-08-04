
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

#include "inddefs.h"


namespace {
    int get_ind_node_ptr(sCKT *ckt, sINDinstance *inst)
    {
        TSTALLOC(INDibrIbrptr, INDbrEq, INDbrEq)
        TSTALLOC(INDposIbrptr, INDposNode, INDbrEq)
        TSTALLOC(INDnegIbrptr, INDnegNode, INDbrEq)
        TSTALLOC(INDibrNegptr, INDbrEq, INDnegNode)
        TSTALLOC(INDibrPosptr, INDbrEq, INDposNode)
        return (OK);
    }

    int get_mut_node_ptr(sCKT *ckt, sMUTinstance *inst)
    {
        TSTALLOC(MUTbr1br2, MUTind1->INDbrEq, MUTind2->INDbrEq)
        TSTALLOC(MUTbr2br1, MUTind2->INDbrEq, MUTind1->INDbrEq)
        return (OK);
    }
}


int
INDdev::setup(sGENmodel *genmod, sCKT *ckt, int* states)
{
    for (sINDmodel *model = static_cast<sINDmodel*>(genmod);
            model; model = model->next()) {
        sINDinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            
            if (inst->INDbrEq == 0 && (inst->INDposNode || inst->INDnegNode)) {
                // SRW -- used to call this ...#internal, changed for
                // compatability with voltage sources, i.e. i(l1) maps
                // to l1#branch.
                //
                sCKTnode *tmp;
                int error = ckt->mkCur(&tmp, inst->GENname, "branch");
                if (error)
                    return (error);
                inst->INDbrEq = tmp->number();
            }

            inst->INDflux = *states;
            *states += 2 ;

            int error = get_ind_node_ptr(ckt, inst);
            if (error != OK)
                return (error);

#ifdef USE_PRELOAD
            if (ckt->CKTpreload) {
                // preload constants

                if (inst->INDposNode) {
                    ckt->preldset(inst->INDposIbrptr, 1.0);
                    ckt->preldset(inst->INDibrPosptr, 1.0);
                }
                if (inst->INDnegNode) {
                    ckt->preldset(inst->INDnegIbrptr, -1.0);
                    ckt->preldset(inst->INDibrNegptr, -1.0);
                }
            }
#endif
            if (inst->INDpolyNumCoeffs > 0) {
                // We save the last branch current.
                *states += 1;
                if (!inst->INDvalues)
                    inst->INDvalues = new double[1];
                continue;
            }

            inst->INDinduct = inst->INDnomInduct;

            if (!inst->INDtree)
                continue;
            int numvars = inst->INDtree->num_vars();
            if (!numvars)
                continue;

            if (!inst->INDvalues)
                inst->INDvalues =  new double[numvars];
            if (!inst->INDeqns)
                inst->INDeqns = new int[numvars];

            *states += numvars;

            for (int i = 0; i < numvars; i++) {

                if (inst->INDtree->vars()[i].type == IF_INSTANCE) {
                    int branch =
                        ckt->findBranch(inst->INDtree->vars()[i].v.uValue);
                    if (branch == 0) {
                        DVO.textOut(OUT_FATAL,
                            "%s: unknown controlling source %s",
                            inst->GENname,
                            inst->INDtree->vars()[i].v.uValue);
                        return (E_BADPARM);
                    }
                    inst->INDeqns[i] = branch;
                }
                else if (inst->INDtree->vars()[i].type == IF_NODE) {
                    inst->INDeqns[i] =
                        inst->INDtree->vars()[i].v.nValue->number();
                }
            }
        }
    }

    if (genmod && ckt->typelook(genmod->GENmodType + 1, &genmod) == 0) {
        // Setup for mutual inductors, if any.  This must happen after
        // inductor setup so is done here.

        for (sMUTmodel *model = static_cast<sMUTmodel*>(genmod);
                model; model = model->next()) {
            // We know that the MUT device type immediately follows IND.
            int ltype = model->GENmodType - 1;
            sMUTinstance *inst;
            for (inst = model->inst(); inst; inst = inst->next()) {

                sGENinstance *gen_instance = 0;
                int error = ckt->findInst(&ltype, &gen_instance,
                    inst->MUTindName1, 0, 0);
                if (error) {
                    if (error == E_NODEV || error == E_NOMOD)
                        DVO.textOut(OUT_WARNING,
                            "%s: coupling to non-existant inductor %s.",
                            inst->GENname, inst->MUTindName1);
                    return (error);
                }
                inst->MUTind1 = (sINDinstance*)gen_instance;

                gen_instance = 0;
                error = ckt->findInst(&ltype, &gen_instance,
                    inst->MUTindName2, 0, 0);
                if (error) {
                    if (error == E_NODEV || error == E_NOMOD)
                        DVO.textOut(OUT_WARNING,
                            "%s: coupling to non-existant inductor %s.",
                            inst->GENname, inst->MUTindName2);
                    return (error);
                }
                inst->MUTind2 = (sINDinstance*)gen_instance;

                // This will be set in the load functions, otherwise we
                // are assuming that the inductor setup has already been
                // called (so the inductances are known), which is not
                // necessarily true.  We also need to set this in load if
                // the inductors are nonlinear.

                inst->MUTfactor = 0.0;
                    // inst->MUTcoupling *
                    // sqrt(inst->MUTind1->INDinduct * inst->MUTind2->INDinduct);

                error = get_mut_node_ptr(ckt, inst);
                if (error != OK)
                    return (error);
            }
        }
    }

    return (OK);
}


int
INDdev::unsetup(sGENmodel *genmod, sCKT*)
{
    sINDmodel *model = static_cast<sINDmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sINDinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            inst->INDbrEq = 0;
        }
    }
    return (OK);
}


// SRW - reset the matrix element pointers.
//
int
INDdev::resetup(sGENmodel *genmod, sCKT *ckt)
{
    for (sINDmodel *model = static_cast<sINDmodel*>(genmod); model;
            model = model->next()) {
        for (sINDinstance *here = model->inst(); here; here = here->next()) {
            int error = get_ind_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}


int
MUTdev::resetup(sGENmodel *genmod, sCKT *ckt)
{
    for (sMUTmodel *model = static_cast<sMUTmodel*>(genmod); model;
            model = model->next()) {
        for (sMUTinstance *here = model->inst(); here; here = here->next()) {
            int error = get_mut_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}

