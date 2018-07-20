
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
CAPdev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sCAPmodel *model = static_cast<sCAPmodel*>(genmod);
    IFvalue *value = &data->v;

    switch (param) {
    case CAP_MOD_C:
        break;
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
    case CAP_MOD_M:
        model->CAPm = value->rValue;
        model->CAPmGiven = true;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}

