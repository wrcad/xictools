
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
INDdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
#ifdef WITH_CMP_GOTO
    // Order MUST match parameter definition enum!
    static void *array[] = {
        0, // notused
        &&L_IND_IND,
        &&L_IND_IC,
        &&L_IND_POLY};
        // &&L_IND_FLUX,
        // &&L_IND_VOLT,
        // &&L_IND_CURRENT,
        // &&L_IND_POWER,
        // &&L_IND_POSNODE,
        // &&L_IND_NEGNODE,
        // &&L_IND_TREE};
        // &&L_MUT_COEFF,
        // &&L_MUT_FACTOR,
        // &&L_MUT_IND1,
        // &&L_MUT_IND2};

    if ((unsigned int)param > IND_POLY)
        return (E_BADPARM);
#endif

    sINDinstance *inst = static_cast<sINDinstance*>(geninst);
    IFvalue *value = &data->v;

#ifdef WITH_CMP_GOTO
    void *jmp = array[param];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_IND_IND:
        if ((data->type & IF_VARTYPES) == IF_PARSETREE) {
            if (inst->INDtree) {
                delete value->tValue;
                value->tValue = 0;
                return (E_BADPARM);
            }
            if (value->tValue->isConst()) {
                double d;
                int er = value->tValue->eval(&d, 0, 0, 0);
                if (er == OK) {
                    inst->INDnomInduct = d;
                    inst->INDindGiven = true;
                }
                delete value->tValue;
                value->tValue = 0;
                if (er != OK)
                    return (E_BADPARM);
            }
            else {
                inst->INDtree = value->tValue;
                value->tValue = 0;
            }
        }
        else if ((data->type & IF_VARTYPES) == IF_REAL) {
            inst->INDnomInduct = value->rValue;
            inst->INDindGiven = true;
        }
        else
            return (E_BADPARM);
        return (OK);
    L_IND_IC:
        inst->INDinitCond = value->rValue;
        inst->INDicGiven = true;
        return (OK);
    L_IND_POLY:
        {
            int nv = value->v.numValue;
            if (nv > 0) {
                inst->INDpolyNumCoeffs = nv;
                double *coeffs = new double[nv];
                for (int i = 0; i < nv; i++)
                    coeffs[i] = value->v.vec.rVec[i];
                inst->INDpolyCoeffs = coeffs;
            }
            data->cleanup();
        }
        return (OK);
#else
    switch (param) {
    case IND_IND:
        if ((data->type & IF_VARTYPES) == IF_PARSETREE) {
            if (inst->INDtree) {
                delete value->tValue;
                value->tValue = 0;
                return (E_BADPARM);
            }
            if (value->tValue->isConst()) {
                double d;
                int er = value->tValue->eval(&d, 0, 0, 0);
                if (er == OK) {
                    inst->INDnomInduct = d;
                    inst->INDindGiven = true;
                }
                delete value->tValue;
                value->tValue = 0;
                if (er != OK)
                    return (E_BADPARM);
            }
            else {
                inst->INDtree = value->tValue;
                value->tValue = 0;
            }
        }
        else if ((data->type & IF_VARTYPES) == IF_REAL) {
            inst->INDnomInduct = value->rValue;
            inst->INDindGiven = true;
        }
        else
            return (E_BADPARM);
        break;
    case IND_IC:
        inst->INDinitCond = value->rValue;
        inst->INDicGiven = true;
        break;
    case IND_POLY:
        {
            int nv = value->v.numValue;
            if (nv > 0) {
                inst->INDpolyNumCoeffs = nv;
                double *coeffs = new double[nv];
                for (int i = 0; i < nv; i++)
                    coeffs[i] = value->v.vec.rVec[i];
                inst->INDpolyCoeffs = coeffs;
            }
            data->cleanup();
        }
        break;
    default:
        return (E_BADPARM);
    }
#endif
    return (OK);
}
