
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


int
INDdev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
#ifdef WITH_CMP_GOTO
    // Order MUST match parameter definition enum!
    static void *array[] = {
        0, // notused
        &&L_IND_IND,
        &&L_IND_IC,
        &&L_IND_M,
        &&L_IND_RES,
        &&L_IND_POLY,
        &&L_IND_FLUX,
        &&L_IND_VOLT,
        &&L_IND_CURRENT,
        &&L_IND_POWER,
        &&L_IND_POSNODE,
        &&L_IND_NEGNODE,
        &&L_IND_TREE};
        // &&L_MUT_COEFF,
        // &&L_MUT_FACTOR,
        // &&L_MUT_IND1,
        // &&L_MUT_IND2};

    if ((unsigned int)which > IND_TREE)
        return (E_BADPARM);
#endif

    const sINDinstance *inst = static_cast<const sINDinstance*>(geninst);

    // Need to override this for non-real returns.
    data->type = IF_REAL;

#ifdef WITH_CMP_GOTO
    void *jmp = array[which];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_IND_IND:
        data->v.rValue = inst->INDinduct;
        return (OK);
    L_IND_IC:
        data->v.rValue = inst->INDinitCond;
        return (OK);
    L_IND_M:
        data->v.rValue = inst->INDm;
        return (OK);
    L_IND_RES:
        data->v.rValue = inst->INDres;
        return (OK);
    L_IND_POLY:
        data->type = IF_REALVEC;
        data->v.v.vec.rVec = inst->INDpolyCoeffs;
        data->v.v.numValue = inst->INDpolyNumCoeffs;
        return (OK);
    L_IND_FLUX:
        data->v.rValue = ckt->interp(inst->INDflux);
        return (OK);
    L_IND_VOLT:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->INDposNode)
                - ckt->rhsOld(inst->INDnegNode);
            data->v.cValue.imag = ckt->irhsOld(inst->INDposNode)
                - ckt->irhsOld(inst->INDnegNode);
        }
        else
            data->v.rValue = ckt->interp(inst->INDvolt);
        return (OK);
    L_IND_CURRENT:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->INDbrEq);
            data->v.cValue.imag = ckt->irhsOld(inst->INDbrEq);
        }
        else
            data->v.rValue = ckt->rhsOld(inst->INDbrEq);
        return (OK);
    L_IND_POWER:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = 0.0;
        else
            data->v.rValue = ckt->rhsOld(inst->INDbrEq) *
                ckt->interp(inst->INDvolt);
        return (OK);
    L_IND_POSNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->INDposNode;
        return (OK);
    L_IND_NEGNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->INDnegNode;
        return (OK);
    L_IND_TREE:
        data->type = IF_PARSETREE;
        data->v.tValue = inst->INDtree;
        return (OK);
#else
    switch (which) {
    case IND_IND:
        data->v.rValue = inst->INDinduct;
        break;
    case IND_IC:    
        data->v.rValue = inst->INDinitCond;
        break;
    case IND_M:    
        data->v.rValue = inst->INDm;
        break;
    case IND_RES:    
        data->v.rValue = inst->INDres;
        break;
    case IND_POLY:
        data->type = IF_REALVEC;
        data->v.v.vec.rVec = inst->INDpolyCoeffs;
        data->v.v.numValue = inst->INDpolyNumCoeffs;
        break;
    case IND_FLUX:
        data->v.rValue = ckt->interp(inst->INDflux);
        break;
    case IND_VOLT:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->INDposNode)
                - ckt->rhsOld(inst->INDnegNode);
            data->v.cValue.imag = ckt->irhsOld(inst->INDposNode)
                - ckt->irhsOld(inst->INDnegNode);
        }
        else
            data->v.rValue = ckt->interp(inst->INDvolt);
        break;
    case IND_CURRENT:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->INDbrEq);
            data->v.cValue.imag = ckt->irhsOld(inst->INDbrEq);
        }
        else
            data->v.rValue = ckt->rhsOld(inst->INDbrEq);
        break;
    case IND_POWER:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = 0.0;
        else
            data->v.rValue = ckt->rhsOld(inst->INDbrEq) *
                ckt->interp(inst->INDvolt);
        break;
    case IND_POSNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->INDposNode;
        break;
    case IND_NEGNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->INDnegNode;
        break;
    case IND_TREE:
        data->type = IF_PARSETREE;
        data->v.tValue = inst->INDtree;
        break;
    default:
        return (E_BADPARM);
    }
#endif
    return (OK);
}

