
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
         1989 Takayasu Sakurai
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mosdefs.h"


int
MOSdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
    sMOSinstance *inst = static_cast<sMOSinstance*>(geninst);
    IFvalue *value = &data->v;

    switch (param) {
    case MOS_TEMP:
        inst->MOStemp = value->rValue+CONSTCtoK;
        inst->MOStempGiven = true;
        break;
    case MOS_M:
        inst->MOSmult = value->rValue;
        inst->MOSmGiven = true;
        break;
    case MOS_L:
        inst->MOSl = value->rValue;
        inst->MOSlGiven = true;
        break;
    case MOS_W:
        inst->MOSw = value->rValue;
        inst->MOSwGiven = true;
        break;
    case MOS_AD:
        inst->MOSdrainArea = value->rValue;
        inst->MOSdrainAreaGiven = true;
        break;
    case MOS_AS:
        inst->MOSsourceArea = value->rValue;
        inst->MOSsourceAreaGiven = true;
        break;
    case MOS_PD:
        inst->MOSdrainPerimeter = value->rValue;
        inst->MOSdrainPerimeterGiven = true;
        break;
    case MOS_PS:
        inst->MOSsourcePerimeter = value->rValue;
        inst->MOSsourcePerimeterGiven = true;
        break;
    case MOS_NRD:
        inst->MOSdrainSquares = value->rValue;
        inst->MOSdrainSquaresGiven = true;
        break;
    case MOS_NRS:
        inst->MOSsourceSquares = value->rValue;
        inst->MOSsourceSquaresGiven = true;
        break;
    case MOS_OFF:
        inst->MOSoff = value->iValue;
        break;
    case MOS_IC_VDS:
        inst->MOSicVDS = value->rValue;
        inst->MOSicVDSGiven = true;
        break;
    case MOS_IC_VGS:
        inst->MOSicVGS = value->rValue;
        inst->MOSicVGSGiven = true;
        break;
    case MOS_IC_VBS:
        inst->MOSicVBS = value->rValue;
        inst->MOSicVBSGiven = true;
        break;
    case MOS_IC:
        switch (value->v.numValue) {
        case 3:
            inst->MOSicVBS = *(value->v.vec.rVec+2);
            inst->MOSicVBSGiven = true;
            // fallthrough
        case 2:
            inst->MOSicVGS = *(value->v.vec.rVec+1);
            inst->MOSicVGSGiven = true;
            // fallthrough
        case 1:
            inst->MOSicVDS = *(value->v.vec.rVec);
            inst->MOSicVDSGiven = true;
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
