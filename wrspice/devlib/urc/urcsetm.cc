
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

#include "urcdefs.h"


int
URCdev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sURCmodel *model = static_cast<sURCmodel*>(genmod);
    IFvalue *value = &data->v;

    switch (param) {
    case URC_MOD_K:
        model->URCk = value->rValue;
        model->URCkGiven = true;
        break;
    case URC_MOD_FMAX:
        model->URCfmax = value->rValue;
        model->URCfmaxGiven = true;
        break;
    case URC_MOD_RPERL:
        model->URCrPerL = value->rValue;
        model->URCrPerLGiven = true;
        break;
    case URC_MOD_CPERL:
        model->URCcPerL = value->rValue;
        model->URCcPerLGiven = true;
        break;
    case URC_MOD_ISPERL:
        model->URCisPerL = value->rValue;
        model->URCisPerLGiven = true;
        break;
    case URC_MOD_RSPERL:
        model->URCrsPerL = value->rValue;
        model->URCrsPerLGiven = true;
        break;
    case URC_MOD_URC:
        // no operation - already know we are a URC, but this makes
        // spice-2 like parsers happy
        //
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}
