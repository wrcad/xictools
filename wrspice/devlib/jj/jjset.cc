
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
Author: 1992 Stephen R. Whiteley
****************************************************************************/

#include "jjdefs.h"

static double def_vm  = 0.03;     // default Ic * Rsubgap
static double def_icr = 0.0017;   // default Ic * Rn


namespace {
    int get_node_ptr(sCKT *ckt, sJJinstance *inst)
    {
        TSTALLOC(JJposPosPtr, JJposNode, JJposNode)
        TSTALLOC(JJnegNegPtr, JJnegNode, JJnegNode)
        TSTALLOC(JJnegPosPtr, JJnegNode, JJposNode)
        TSTALLOC(JJposNegPtr, JJposNode, JJnegNode)
        if (inst->JJcontrol) {
            TSTALLOC(JJposIbrPtr, JJposNode, JJbranch)
            TSTALLOC(JJnegIbrPtr, JJnegNode, JJbranch)
        }
        if (inst->JJphsNode > 0) {
            TSTALLOC(JJphsPhsPtr, JJphsNode, JJphsNode)
        }
        return (OK);
    }
}


int
JJdev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
    sJJmodel *model = static_cast<sJJmodel*>(genmod);
    for ( ; model; model = model->next()) {

        if (!model->JJrtypeGiven)  model->JJrtype  = 1;
        if (!model->JJictypeGiven) model->JJictype = 1;
        if (!model->JJvgGiven)     model->JJvg     = 3e-3;
        if (!model->JJdelvGiven)   model->JJdelv   = 1e-4;
        if (!model->JJccsensGiven) model->JJccsens = 1e-2;
        if (!model->JJcritiGiven)  model->JJcriti  = 1e-3;
        if (!model->JJcapGiven)    model->JJcap    = 1e-12;
        if (!model->JJicfGiven)    model->JJicFactor = M_PI/4.0;

        if (model->JJdelv <= 0) {
            DVO.textOut(OUT_WARNING,
                "%s: delv entry was <= 0, reset to 1e-5\n",
                model->GENmodName);
            model->JJdelv = 1e-5;
        }
        if (model->JJccsens <= 0) {
            DVO.textOut(OUT_WARNING,
                "%s: icon entry was <= 0, reset to 1e3\n",
                model->GENmodName);
            model->JJccsens = 1e3;
        }

        if (!model->JJr0Given) {
            if (model->JJcriti)
                model->JJr0 = def_vm/model->JJcriti;
            else
                model->JJr0 = def_vm/1e-3;
        }
        if (!model->JJrnGiven) {
            if (model->JJcriti)
                model->JJrn = def_icr/model->JJcriti;
            else
                model->JJrn = def_icr/1e-3;
        }
        if (!model->JJnoiseGiven)
            model->JJnoiseGiven = 1.0;

        if (model->JJrn > model->JJr0)
            model->JJrn = model->JJr0;

        double temp = model->JJdelv/2.0;
        if (model->JJvg < temp) {
            DVO.textOut(OUT_WARNING,
                "%s: vg entry was < delv/2, reset to delv/2\n",
                model->GENmodName);
            model->JJvg = temp;
        }
        model->JJvless  = model->JJvg - temp;
        model->JJvmore  = model->JJvg + temp;

        if (model->JJcap > 1e-9) {
            DVO.textOut(OUT_WARNING,
                "%s: bad cap entry ( > 1nF), reset to 1pF\n",
                model->GENmodName);
            model->JJcap = 1e-12;
        }

        temp = model->JJvg/2;
        if (model->JJcap > 0.0) {
            model->JJvdpbak = sqrt(PHI0_2PI * model->JJcriti / model->JJcap);
            if (model->JJvdpbak > temp)
                model->JJvdpbak = temp;
        }
        else
            model->JJvdpbak = temp;

        sJJinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            if (!inst->JJpiGiven)
                inst->JJpi = model->JJpi;
            
            if (!inst->JJareaGiven) inst->JJarea = 1;

            inst->JJcriti = model->JJcriti * inst->JJarea;

            inst->JJg0 = inst->JJarea / model->JJr0;
            inst->JJgn = inst->JJarea / model->JJrn;
            inst->JJgs = inst->JJcriti/(model->JJicFactor * model->JJdelv);

            // These currents are added to RHS in piecewise qp model
            inst->JJcr1 = (inst->JJg0 - inst->JJgs)*model->JJvless;
            inst->JJcr2 = inst->JJcriti/model->JJicFactor +
                model->JJvless * inst->JJg0 -
                model->JJvmore * inst->JJgn;

            if (!inst->JJnoiseGiven)
                inst->JJnoise = model->JJnoise;

            if (model->JJrtype == 3) {

                // 5th order polynomial model for NbN.
                //
                // cj = g0*vj + g1*vj**3 + g2*vj**5,
                // gj = dcj/dvj = g0 + 3*g1*vj**2 + 5*g2*vj**4.
                //
                // Required:
                // (1) cj(vmore) = g0*vmore + g1*vmore**3 + g2*vmore**5
                //               = Ic/factor + g0*vless
                // (2) gj(vmore) = g0 + 3*g1*vmore**2 + 5*g2*vmore**4
                //               = gs.
                // (3) gj(0) = g0 (trivially satisfied).
                //
                // define Gn = (Ic/factor + g0*vless)/vmore
                //
                // 4*g0 + 2*g1*vmore**2 = 5*Gn - gs, or
                // g1 = (5*Gn - gs - 4*g0)/(2*vmore**2)
                // g2 = (Gn - g0 - g1*vmore**2)/(vmore**4)
                //

                double Gn = (inst->JJcriti/model->JJicFactor +
                    model->JJvless*inst->JJg0)/model->JJvmore;
                temp = model->JJvmore*model->JJvmore;
                inst->JJg1 = 0.5*(5.0*Gn - inst->JJgs - 4.0*inst->JJg0);
                inst->JJg2 = (Gn - inst->JJg0 - inst->JJg1)/(temp*temp);
                inst->JJg1 /= temp;
                inst->JJcr1 = (Gn - inst->JJgn)*model->JJvmore;
                // if the conductivity goes negative, the parameters
                // aren't good
                if (inst->JJg1 < 0.0 && 9*inst->JJg1*inst->JJg1 >
                        20*inst->JJg0*inst->JJg2) {
                    DVO.textOut(OUT_WARNING,
"%s: delv is too small for rtype=3, expect negative conductivity\nregion.\n",
                        model->GENmodName);
                }
            }
            else {
                inst->JJg1 = 0;
                inst->JJg2 = 0;
            }    

            inst->JJcap = model->JJcap * inst->JJarea;

            inst->GENstate = *states;
            *states += JJnumStates;

            inst->JJinitControl = 0;
            if (inst->JJcontrolGiven) {
                inst->JJbranch = ckt->findBranch(inst->JJcontrol);

                if (inst->JJbranch == 0) {
                    DVO.textOut(OUT_WARNING,
                        "%s: control current modulated by non-existant\n"
                        "or non-branch device %s, ignored.",
                        inst->GENname, inst->JJcontrol);
                    inst->JJcontrol = 0;
                }
            }

            int error = get_node_ptr(ckt, inst);
            if (error != OK)
                return (error);

            if (inst->JJphsNode > 0) {
#ifdef USE_PRELOAD
                // preload constant
                if (ckt->CKTpreload)
                    ckt->preldset(inst->JJphsPhsPtr, 1.0);
#endif
            }
        }
    }

    return (OK);
}


// SRW - reset the matrix element pointers.
//
int
JJdev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sJJmodel *model = (sJJmodel*)inModel; model;
            model = model->next()) {
        for (sJJinstance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}

