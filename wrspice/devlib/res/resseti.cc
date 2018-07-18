
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
         1993 Stephen R. Whiteley
****************************************************************************/

#include "resdefs.h"


int
RESdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
#ifdef WITH_CMP_GOTO
    // Order MUST match parameter definition enum!
    static void *array[] = {
        0, // notused
        &&L_RES_RESIST,
        &&L_RES_TEMP,
        &&L_RES_WIDTH,
        &&L_RES_LENGTH,
        &&L_RES_TC1,
        &&L_RES_TC2,
        &&L_RES_NOISE,
        &&L_RES_M,
        &&L_RES_POLY};
        // &&L_RES_CONDUCT,
        // &&L_RES_VOLTAGE,
        // &&L_RES_CURRENT,
        // &&L_RES_POWER,
        // &&L_RES_POSNODE,
        // &&L_RES_NEGNODE,
        // &&L_RES_TREE};

    if ((unsigned int)param > RES_POLY)
        return (E_BADPARM);
#endif

    sRESinstance *inst = static_cast<sRESinstance*>(geninst);
    IFvalue *value = &data->v;

#ifdef WITH_CMP_GOTO
    void *jmp = array[param];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_RES_RESIST:
        if ((data->type & IF_VARTYPES) == IF_PARSETREE) {
            if (inst->REStree) {
                delete value->tValue;
                value->tValue = 0;
                return (E_BADPARM);
            }
            if (value->tValue->isConst()) {
                double d;
                int er = value->tValue->eval(&d, 0, 0, 0);
                if (er == OK) {
                    inst->RESresist = d;
                    inst->RESresGiven = true;
                }
                delete value->tValue;
                value->tValue = 0;
                if (er != OK)
                    return (E_BADPARM);
            }
            else {
                inst->REStree = value->tValue;
                inst->REStree->differentiate();
                value->tValue = 0;
            }
        }
        else if ((data->type & IF_VARTYPES) == IF_REAL) {
            inst->RESresist = value->rValue;
            inst->RESresGiven = true;
        }
        else
            return (E_BADPARM);
        return (OK);
    L_RES_TEMP:
        inst->REStemp = value->rValue + CONSTCtoK;
        inst->REStempGiven = true;
        return (OK);
    L_RES_WIDTH:
        inst->RESwidth = value->rValue;
        inst->RESwidthGiven = true;
        return (OK);
    L_RES_LENGTH:
        inst->RESlength = value->rValue;
        inst->RESlengthGiven = true;
        return (OK);
    L_RES_TC1:
        inst->REStc1 = value->rValue;
        inst->REStc1Given = true;
        return (OK);
    L_RES_TC2:
        inst->REStc2 = value->rValue;
        inst->REStc2Given = true;
        return (OK);
    L_RES_NOISE:
        inst->RESnoise = value->rValue;
        inst->RESnoiseGiven = true;
        return (OK);
    L_RES_M:
        inst->RESm = value->rValue;
        inst->RESmGiven = true;
        return (OK);
    L_RES_POLY:
        {
            int nv = value->v.numValue;
            if (nv > 0) {
                inst->RESpolyNumCoeffs = nv;
                double *coeffs = new double[nv];
                for (int i = 0; i < nv; i++)
                    coeffs[i] = value->v.vec.rVec[i];
                inst->RESpolyCoeffs = coeffs;
            }
            data->cleanup();
        }
        return (OK);
#else
    switch(param) {
    case RES_TEMP:
        inst->REStemp = value->rValue + CONSTCtoK;
        inst->REStempGiven = true;
        break;
    case RES_RESIST:
        if ((data->type & IF_VARTYPES) == IF_PARSETREE) {
            if (inst->REStree) {
                delete value->tValue;
                value->tValue = 0;
                return (E_BADPARM);
            }
            if (value->tValue->isConst()) {
                double d;
                int er = value->tValue->eval(&d, 0, 0, 0);
                if (er == OK) {
                    inst->RESresist = d;
                    inst->RESresGiven = true;
                }
                delete value->tValue;
                value->tValue = 0;
                if (er != OK)
                    return (E_BADPARM);
            }
            else {
                inst->REStree = value->tValue;
                inst->REStree->differentiate();
                value->tValue = 0;
            }
        }
        else if ((data->type & IF_VARTYPES) == IF_REAL) {
            inst->RESresist = value->rValue;
            inst->RESresGiven = true;
        }
        else
            return (E_BADPARM);
        break;
    case RES_WIDTH:
        inst->RESwidth = value->rValue;
        inst->RESwidthGiven = true;
        break;
    case RES_LENGTH:
        inst->RESlength = value->rValue;
        inst->RESlengthGiven = true;
        break;
    case RES_TC1:
        inst->REStc1 = value->rValue;
        inst->REStc1Given = true;
        break;
    case RES_TC2:
        inst->REStc2 = value->rValue;
        inst->REStc2Given = true;
        break;
    case RES_NOISE:
        inst->RESnoise = value->rValue;
        inst->RESnoiseGiven = true;
        break;
    case RES_M:
        inst->RESm = value->rValue;
        inst->RESmGiven = true;
        break;
    case RES_POLY:
        {
            int nv = value->v.numValue;
            if (nv > 0) {
                inst->RESpolyNumCoeffs = nv;
                double *coeffs = new double[nv];
                for (int i = 0; i < nv; i++)
                    coeffs[i] = value->v.vec.rVec[i];
                inst->RESpolyCoeffs = coeffs;
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

