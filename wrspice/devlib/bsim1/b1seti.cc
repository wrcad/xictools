
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
Authors: 1985 Hong J. Park, Thomas L. Quarles 
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b1defs.h"


int
B1dev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
    sB1instance *inst = static_cast<sB1instance*>(geninst);
    IFvalue *value = &data->v;

    switch (param) {
    case BSIM1_M:
        inst->B1m = value->rValue;
        inst->B1mGiven = true;
        break;
    case BSIM1_L:
        inst->B1l = value->rValue;
        inst->B1lGiven = true;
        break;
    case BSIM1_W:
        inst->B1w = value->rValue;
        inst->B1wGiven = true;
        break;
    case BSIM1_AS:
        inst->B1sourceArea = value->rValue;
        inst->B1sourceAreaGiven = true;
        break;
    case BSIM1_AD:
        inst->B1drainArea = value->rValue;
        inst->B1drainAreaGiven = true;
        break;
    case BSIM1_PS:
        inst->B1sourcePerimeter = value->rValue;
        inst->B1sourcePerimeterGiven = true;
        break;
    case BSIM1_PD:
        inst->B1drainPerimeter = value->rValue;
        inst->B1drainPerimeterGiven = true;
        break;
    case BSIM1_NRS:
        inst->B1sourceSquares = value->rValue;
        inst->B1sourceSquaresGiven = true;
        break;
    case BSIM1_NRD:
        inst->B1drainSquares = value->rValue;
        inst->B1drainSquaresGiven = true;
        break;
    case BSIM1_OFF:
        inst->B1off = value->iValue;
        break;
    case BSIM1_IC_VBS:
        inst->B1icVBS = value->rValue;
        inst->B1icVBSGiven = true;
        break;
    case BSIM1_IC_VDS:
        inst->B1icVDS = value->rValue;
        inst->B1icVDSGiven = true;
        break;
    case BSIM1_IC_VGS:
        inst->B1icVGS = value->rValue;
        inst->B1icVGSGiven = true;
        break;
    case BSIM1_IC:
        switch (value->v.numValue) {
        case 3:
            inst->B1icVBS = *(value->v.vec.rVec+2);
            inst->B1icVBSGiven = true;
            // fallthrough
        case 2:
            inst->B1icVGS = *(value->v.vec.rVec+1);
            inst->B1icVGSGiven = true;
            // fallthrough
        case 1:
            inst->B1icVDS = *(value->v.vec.rVec);
            inst->B1icVDSGiven = true;
            data->cleanup();
            break;
        default:
            data->cleanup();
            return (E_BADPARM);
        }
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}
