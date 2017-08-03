
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
RESdev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
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
        &&L_RES_POLY,
        &&L_RES_CONDUCT,
        &&L_RES_VOLTAGE,
        &&L_RES_CURRENT,
        &&L_RES_POWER,
        &&L_RES_POSNODE,
        &&L_RES_NEGNODE,
        &&L_RES_TREE};

    if ((unsigned int)which > RES_TREE)
        return (E_BADPARM);
#endif

    const sRESinstance *inst = static_cast<const sRESinstance*>(geninst);

    // Need to override this for non-real returns.
    data->type = IF_REAL;

#ifdef WITH_CMP_GOTO
    void *jmp = array[which];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_RES_RESIST:
        data->v.rValue = inst->RESresist * inst->REStcFactor;
        return (OK);
    L_RES_TEMP:
        data->v.rValue = inst->REStemp - CONSTCtoK;
        return (OK);
    L_RES_WIDTH :
        data->v.rValue = inst->RESwidth;
        return (OK);
    L_RES_LENGTH:
        data->v.rValue = inst->RESlength;
        return (OK);
    L_RES_TC1:
        data->v.rValue = inst->REStc1;
        return (OK);
    L_RES_TC2:
        data->v.rValue = inst->REStc2;
        return (OK);
    L_RES_NOISE:
        data->v.rValue = inst->RESnoise;
        return (OK);
    L_RES_POLY:
        data->type = IF_REALVEC;
        data->v.v.vec.rVec = inst->RESpolyCoeffs;
        data->v.v.numValue = inst->RESpolyNumCoeffs;
        return (OK);
    L_RES_CONDUCT:
        data->v.rValue = inst->RESconduct;
        return (OK);
    L_RES_VOLTAGE:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->RESposNode) - 
                ckt->rhsOld(inst->RESnegNode);
            data->v.cValue.imag = ckt->irhsOld(inst->RESposNode) - 
                ckt->irhsOld(inst->RESnegNode);
        }
        else {
            data->v.rValue =
                ckt->rhsOld(inst->RESposNode) - ckt->rhsOld(inst->RESnegNode);
        }
        return (OK);
    L_RES_CURRENT:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = (ckt->rhsOld(inst->RESposNode) - 
                ckt->rhsOld(inst->RESnegNode))*inst->RESconduct;
            data->v.cValue.imag = (ckt->irhsOld(inst->RESposNode) - 
                ckt->irhsOld(inst->RESnegNode))*inst->RESconduct;
        }
        else {
            data->v.rValue = (ckt->rhsOld(inst->RESposNode) -  
                ckt->rhsOld(inst->RESnegNode)) *inst->RESconduct;
        }
        return (OK);
    L_RES_POWER:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = 0.0;
        else {
            data->v.rValue = (ckt->rhsOld(inst->RESposNode) -  
                ckt->rhsOld(inst->RESnegNode));
            data->v.rValue *= data->v.rValue * inst->RESconduct;
        }
        return (OK);
    L_RES_POSNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->RESposNode;
        return (OK);
    L_RES_NEGNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->RESnegNode;
        return (OK);
    L_RES_TREE:
        data->type = IF_PARSETREE;
        data->v.tValue = inst->REStree;
        return (OK);
#else
    switch (which) {
    case RES_RESIST:
        data->v.rValue = inst->RESresist * inst->REStcFactor;
        break;
    case RES_TEMP:
        data->v.rValue = inst->REStemp - CONSTCtoK;
        break;
    case RES_WIDTH :
        data->v.rValue = inst->RESwidth;
        break;
    case RES_LENGTH:
        data->v.rValue = inst->RESlength;
        break;
    case RES_TC1:
        data->v.rValue = inst->REStc1;
        break;
    case RES_TC2:
        data->v.rValue = inst->REStc2;
        break;
    case RES_NOISE:
        data->v.rValue = inst->RESnoise;
        break;
    case RES_POLY:
        data->type = IF_REALVEC;
        data->v.v.vec.rVec = inst->RESpolyCoeffs;
        data->v.v.numValue = inst->RESpolyNumCoeffs;
        break;
    case RES_CONDUCT:
        data->v.rValue = inst->RESconduct;
        break;
    case RES_VOLTAGE:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->RESposNode) - 
                ckt->rhsOld(inst->RESnegNode);
            data->v.cValue.imag = ckt->irhsOld(inst->RESposNode) - 
                ckt->irhsOld(inst->RESnegNode);
        }
        else {
            data->v.rValue =
                ckt->rhsOld(inst->RESposNode) - ckt->rhsOld(inst->RESnegNode);
        }
        break;
    case RES_CURRENT:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = (ckt->rhsOld(inst->RESposNode) - 
                ckt->rhsOld(inst->RESnegNode))*inst->RESconduct;
            data->v.cValue.imag = (ckt->irhsOld(inst->RESposNode) - 
                ckt->irhsOld(inst->RESnegNode))*inst->RESconduct;
        }
        else {
            data->v.rValue = (ckt->rhsOld(inst->RESposNode) -  
                ckt->rhsOld(inst->RESnegNode)) *inst->RESconduct;
        }
        break;
    case RES_POWER:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = 0.0;
        else {
            data->v.rValue = (ckt->rhsOld(inst->RESposNode) -  
                ckt->rhsOld(inst->RESnegNode));
            data->v.rValue *= data->v.rValue * inst->RESconduct;
        }
        break;
    case RES_POSNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->RESposNode;
        break;
    case RES_NEGNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->RESnegNode;
        break;
    case RES_TREE:
        data->type = IF_PARSETREE;
        data->v.tValue = inst->REStree;
        break;
    default:
        return (E_BADPARM);
    }
#endif
    return (OK);
}

