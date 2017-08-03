
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

#include "capdefs.h"


int
CAPdev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sCAPmodel *model = static_cast<sCAPmodel*>(genmod);
    IFvalue *value = &data->v;

    switch (param) {
    case CAP_MOD_CJ:
        model->CAPcj = value->rValue;
        model->CAPcjGiven = true;
        break;
    case CAP_MOD_CJSW:
        model->CAPcjsw = value->rValue;
        model->CAPcjswGiven = true;
        break;
    case CAP_MOD_DEFWIDTH:
        model->CAPdefWidth = value->rValue;
        model->CAPdefWidthGiven = true;
        break;
    case CAP_MOD_NARROW:
        model->CAPnarrow = value->rValue;
        model->CAPnarrowGiven = true;
        break;
    case CAP_MOD_TNOM:
        model->CAPtnom = value->rValue + CONSTCtoK;
        model->CAPtnomGiven = true;
        break;
    case CAP_MOD_TC1:
        model->CAPtempCoeff1 = value->rValue;
        model->CAPtc1Given = true;
        break;
    case CAP_MOD_TC2:
        model->CAPtempCoeff2 = value->rValue;
        model->CAPtc2Given = true;
        break;
    case CAP_MOD_C:
        // just being reassured by the user that we are a capacitor
        // no-op
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}

