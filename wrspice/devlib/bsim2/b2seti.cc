
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
Authors: 1985 Hong June Park, Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b2defs.h"


int
B2dev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
    sB2instance *inst = static_cast<sB2instance*>(geninst);
    IFvalue *value = &data->v;

    switch (param) {
    case BSIM2_M:
        inst->B2m = value->rValue;
        inst->B2mGiven = true;
        break;
    case BSIM2_L:
        inst->B2l = value->rValue;
        inst->B2lGiven = true;
        break;
    case BSIM2_W:
        inst->B2w = value->rValue;
        inst->B2wGiven = true;
        break;
    case BSIM2_AS:
        inst->B2sourceArea = value->rValue;
        inst->B2sourceAreaGiven = true;
        break;
    case BSIM2_AD:
        inst->B2drainArea = value->rValue;
        inst->B2drainAreaGiven = true;
        break;
    case BSIM2_PS:
        inst->B2sourcePerimeter = value->rValue;
        inst->B2sourcePerimeterGiven = true;
        break;
    case BSIM2_PD:
        inst->B2drainPerimeter = value->rValue;
        inst->B2drainPerimeterGiven = true;
        break;
    case BSIM2_NRS:
        inst->B2sourceSquares = value->rValue;
        inst->B2sourceSquaresGiven = true;
        break;
    case BSIM2_NRD:
        inst->B2drainSquares = value->rValue;
        inst->B2drainSquaresGiven = true;
        break;
    case BSIM2_OFF:
        inst->B2off = value->iValue;
        break;
    case BSIM2_IC_VBS:
        inst->B2icVBS = value->rValue;
        inst->B2icVBSGiven = true;
        break;
    case BSIM2_IC_VDS:
        inst->B2icVDS = value->rValue;
        inst->B2icVDSGiven = true;
        break;
    case BSIM2_IC_VGS:
        inst->B2icVGS = value->rValue;
        inst->B2icVGSGiven = true;
        break;
    case BSIM2_IC:
        switch (value->v.numValue) {
        case 3:
            inst->B2icVBS = *(value->v.vec.rVec+2);
            inst->B2icVBSGiven = true;
            // fallthrough
        case 2:
            inst->B2icVGS = *(value->v.vec.rVec+1);
            inst->B2icVGSGiven = true;
            // fallthrough
        case 1:
            inst->B2icVDS = *(value->v.vec.rVec);
            inst->B2icVDSGiven = true;
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
