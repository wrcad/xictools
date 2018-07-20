
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


int
CAPdev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
#ifdef WITH_CMP_GOTO
    // Order MUST match parameter definition enum!
    static void *array[] = {
        0, // notused
        &&L_CAP_CAP,
        &&L_CAP_IC,
        &&L_CAP_WIDTH,
        &&L_CAP_LENGTH,
        &&L_CAP_TEMP,
        &&L_CAP_TC1,
        &&L_CAP_TC2,
        &&L_CAP_M,
        &&L_CAP_POLY,
        &&L_CAP_CHARGE,
        &&L_CAP_VOLTAGE,
        &&L_CAP_CURRENT,
        &&L_CAP_POWER,
        &&L_CAP_POSNODE,
        &&L_CAP_NEGNODE,
        &&L_CAP_TREE};

    if ((unsigned int)which > CAP_TREE)
        return (E_BADPARM);
#endif

    const sCAPinstance *inst = static_cast<const sCAPinstance*>(geninst);

    // Need to override this for non-real returns.
    data->type = IF_REAL;

#ifdef WITH_CMP_GOTO
    void *jmp = array[which];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_CAP_CAP:
        data->v.rValue = inst->CAPcapac;
        return (OK);
    L_CAP_IC:
        data->v.rValue = inst->CAPinitCond;
        return (OK);
    L_CAP_WIDTH:
        data->v.rValue = inst->CAPwidth;
        return (OK);
    L_CAP_LENGTH:
        data->v.rValue = inst->CAPlength;
        return (OK);
    L_CAP_TEMP:
        data->v.rValue = inst->CAPtemp - CONSTCtoK;
        return (OK);
    L_CAP_TC1:
        data->v.rValue = inst->CAPtc1;
        return (OK);
    L_CAP_TC2:
        data->v.rValue = inst->CAPtc2;
        return (OK);
    L_CAP_M:
        data->v.rValue = inst->CAPm;
        return (OK);
    L_CAP_POLY:
        data->type = IF_REALVEC;
        data->v.v.vec.rVec = inst->CAPpolyCoeffs;
        data->v.v.numValue = inst->CAPpolyNumCoeffs;
        return (OK);
    L_CAP_CHARGE:
        data->v.rValue = ckt->interp(inst->CAPqcap);
        return (OK);
    L_CAP_VOLTAGE:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->CAPposNode) - 
                ckt->rhsOld(inst->CAPnegNode);
            data->v.cValue.imag = ckt->irhsOld(inst->CAPposNode) - 
                ckt->irhsOld(inst->CAPnegNode);
        }
        else
            data->v.rValue = ckt->rhsOld(inst->CAPposNode) - 
                ckt->rhsOld(inst->CAPnegNode);
        return (OK);
    L_CAP_CURRENT:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = -ckt->CKTomega * inst->CAPcapac *
                (ckt->irhsOld(inst->CAPposNode) - 
                ckt->irhsOld(inst->CAPnegNode));
            data->v.cValue.imag = ckt->CKTomega * inst->CAPcapac *
                (ckt->rhsOld(inst->CAPposNode) - 
                ckt->rhsOld(inst->CAPnegNode));
        }
        else
            data->v.rValue = ckt->interp(inst->CAPccap);
        return (OK);
    L_CAP_POWER:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = 0.0;
        else
            data->v.rValue = ckt->interp(inst->CAPccap) *
                (ckt->rhsOld(inst->CAPposNode) - 
                ckt->rhsOld(inst->CAPnegNode));
        return (OK);
    L_CAP_POSNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->CAPposNode;
        return (OK);
    L_CAP_NEGNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->CAPnegNode;
        return (OK);
    L_CAP_TREE:
        data->type = IF_PARSETREE;
        data->v.tValue = inst->CAPtree;
        return (OK);
#else
    switch (which) {
    case CAP_CAP:
        data->v.rValue = inst->CAPcapac;
        break;
    case CAP_IC:
        data->v.rValue = inst->CAPinitCond;
        break;
    case CAP_WIDTH:
        data->v.rValue = inst->CAPwidth;
        break;
    case CAP_LENGTH:
        data->v.rValue = inst->CAPlength;
        break;
    case CAP_TEMP:
        data->v.rValue = inst->CAPtemp - CONSTCtoK;
        break;
    case CAP_TC1:
        data->v.rValue = inst->CAPtc1;
        break;
    case CAP_TC2:
        data->v.rValue = inst->CAPtc2;
        break;
    case CAP_M:
        data->v.rValue = inst->CAPm;
        break;
    case CAP_POLY:
        data->type = IF_REALVEC;
        data->v.v.vec.rVec = inst->CAPpolyCoeffs;
        data->v.v.numValue = inst->CAPpolyNumCoeffs;
        break;
    case CAP_CHARGE:
        data->v.rValue = ckt->interp(inst->CAPqcap);
        break;
    case CAP_VOLTAGE:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->CAPposNode) - 
                ckt->rhsOld(inst->CAPnegNode);
            data->v.cValue.imag = ckt->irhsOld(inst->CAPposNode) - 
                ckt->irhsOld(inst->CAPnegNode);
        }
        else
            data->v.rValue = ckt->rhsOld(inst->CAPposNode) - 
                ckt->rhsOld(inst->CAPnegNode);
        break;
    case CAP_CURRENT:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = -ckt->CKTomega * inst->CAPcapac *
                (ckt->irhsOld(inst->CAPposNode) - 
                ckt->irhsOld(inst->CAPnegNode));
            data->v.cValue.imag = ckt->CKTomega * inst->CAPcapac *
                (ckt->rhsOld(inst->CAPposNode) - 
                ckt->rhsOld(inst->CAPnegNode));
        }
        else
            data->v.rValue = ckt->interp(inst->CAPccap);
        break;
    case CAP_POWER:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = 0.0;
        else
            data->v.rValue = ckt->interp(inst->CAPccap) *
                (ckt->rhsOld(inst->CAPposNode) - 
                ckt->rhsOld(inst->CAPnegNode));
        break;
    case CAP_POSNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->CAPposNode;
        break;
    case CAP_NEGNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->CAPnegNode;
        break;
    case CAP_TREE:
        data->type = IF_PARSETREE;
        data->v.tValue = inst->CAPtree;
        break;
    default:
        return (E_BADPARM);
    }
#endif
    return (OK);
}
