
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

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1995 Min-Chie Jeng and Mansun Chan.
* Revision 3.2 1998/6/16  18:00:00  Weidong 
* BSIM3v3.2 release
**********/

#include "b3defs.h"


int
B3dev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
    sB3instance *inst = static_cast<sB3instance*>(geninst);
    IFvalue *value = &data->v;

    switch(param) {
    case B3_M:
        inst->B3m = value->rValue;
        inst->B3mGiven = true;
        break;
    case B3_W:
        inst->B3w = value->rValue;
        inst->B3wGiven = true;
        break;
    case B3_L:
        inst->B3l = value->rValue;
        inst->B3lGiven = true;
        break;
    case B3_AS:
        inst->B3sourceArea = value->rValue;
        inst->B3sourceAreaGiven = true;
        break;
    case B3_AD:
        inst->B3drainArea = value->rValue;
        inst->B3drainAreaGiven = true;
        break;
    case B3_PS:
        inst->B3sourcePerimeter = value->rValue;
        inst->B3sourcePerimeterGiven = true;
        break;
    case B3_PD:
        inst->B3drainPerimeter = value->rValue;
        inst->B3drainPerimeterGiven = true;
        break;
    case B3_NRS:
        inst->B3sourceSquares = value->rValue;
        inst->B3sourceSquaresGiven = true;
        break;
    case B3_NRD:
        inst->B3drainSquares = value->rValue;
        inst->B3drainSquaresGiven = true;
        break;
    case B3_OFF:
        inst->B3off = value->iValue;
        break;
    case B3_IC_VBS:
        inst->B3icVBS = value->rValue;
        inst->B3icVBSGiven = true;
        break;
    case B3_IC_VDS:
        inst->B3icVDS = value->rValue;
        inst->B3icVDSGiven = true;
        break;
    case B3_IC_VGS:
        inst->B3icVGS = value->rValue;
        inst->B3icVGSGiven = true;
        break;
    case B3_NQSMOD:
        inst->B3nqsMod = value->iValue;
        inst->B3nqsModGiven = true;
        break;
    case B3_IC:
        switch (value->v.numValue) {
        case 3:
            inst->B3icVBS = *(value->v.vec.rVec+2);
            inst->B3icVBSGiven = true;
            // fallthrough
        case 2:
            inst->B3icVGS = *(value->v.vec.rVec+1);
            inst->B3icVGSGiven = true;
            // fallthrough
        case 1:
            inst->B3icVDS = *(value->v.vec.rVec);
            inst->B3icVDSGiven = true;
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



