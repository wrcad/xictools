
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

#include "resdefs.h"


int
RESdev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sRESmodel *model = static_cast<sRESmodel*>(genmod);
    IFvalue *value = &data->v;

    switch (param) {
    case RES_MOD_RSH:
        model->RESsheetRes = value->rValue;
        model->RESsheetResGiven = true;
        break;
    case RES_MOD_NARROW:
        model->RESnarrow = value->rValue;
        model->RESnarrowGiven = true;
        break;
    case RES_MOD_DL:
        model->RESshorten = value->rValue;
        model->RESshortenGiven = true;
        break;
    case RES_MOD_TC1:
        model->REStempCoeff1 = value->rValue;
        model->REStc1Given = true;
        break;
    case RES_MOD_TC2:
        model->REStempCoeff2 = value->rValue;
        model->REStc2Given = true;
        break;
    case RES_MOD_DEFWIDTH:
        model->RESdefWidth = value->rValue;
        model->RESdefWidthGiven = true;
        break;
    case RES_MOD_DEFLENGTH:
        model->RESdefLength = value->rValue;
        model->RESdefLengthGiven = true;
        break;
    case RES_MOD_TNOM:
        model->REStnom = value->rValue + CONSTCtoK;
        model->REStnomGiven = true;
        break;
    case RES_MOD_TEMP:
        model->REStemp = value->rValue + CONSTCtoK;
        model->REStempGiven = true;
        break;
    case RES_MOD_NOISE:
        model->RESnoise = value->rValue;
        model->RESnoiseGiven = true;
        break;
    case RES_MOD_KF:
        model->RESkf = value->rValue;
        model->RESkfGiven = true;
        break;
    case RES_MOD_AF:
        model->RESaf = value->rValue;
        model->RESafGiven = true;
        break;
    case RES_MOD_EF:
        model->RESef = value->rValue;
        model->RESefGiven = true;
        break;
    case RES_MOD_WF:
        model->RESwf = value->rValue;
        model->RESwfGiven = true;
        break;
    case RES_MOD_LF:
        model->RESlf = value->rValue;
        model->RESlfGiven = true;
        break;
    case RES_MOD_R:
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}

