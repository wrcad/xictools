
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
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Gordon M. Jacobs
         1992 Stephen R. Whiteley
****************************************************************************/

#include "swdefs.h"


int
SWdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
#ifdef WITH_CMP_GOTO
    // Order MUST match parameter definition enum!
    static void *array[] = {
        0, // notused
        &&L_SW_IC_ON,
        &&L_SW_IC_OFF,
        0, // &&L_SW_IC,
        &&L_SW_CONTROL};
        // &&L_SW_VOLTAGE,
        // &&L_SW_CURRENT,
        // &&L_SW_POWER,
        // &&L_SW_POS_NODE,
        // &&L_SW_NEG_NODE,
        // &&L_SW_POS_CONT_NODE,
        // &&L_SW_NEG_CONT_NODE};

    if ((unsigned int)param > SW_CONTROL)
        return (E_BADPARM);
#endif

    sSWinstance *inst = static_cast<sSWinstance*>(geninst);
    IFvalue *value = &data->v;

#ifdef WITH_CMP_GOTO
    void *jmp = array[param];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_SW_IC_ON:
        if (value->iValue)
            inst->SWzero_stateGiven = true;
        return (OK);
    L_SW_IC_OFF:
        if (value->iValue)
            inst->SWzero_stateGiven = false;
        return (OK);
    L_SW_CONTROL:
        inst->SWcontName = value->uValue;
        return (OK);
#else
    switch (param) {
    case SW_IC_ON:
        if (value->iValue)
            inst->SWzero_stateGiven = true;
        break;
    case SW_IC_OFF:
        if (value->iValue)
            inst->SWzero_stateGiven = false;
        break;
    case  SW_CONTROL:
        inst->SWcontName = value->uValue;
        break;
    default:
        return (E_BADPARM);
    }
#endif
    return (OK);
}
