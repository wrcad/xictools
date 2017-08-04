
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

//-------------------------------------------------------------------------
// This is a general transmission line model derived from:
//  1) the spice3 TRA (lossless) model
//  2) the spice3 LTRA (lossy, convolution) model
//  3) the kspice TXL (lossy, Pade approximation convolution) model
// Authors:
//  1985 Thomas L. Quarles
//  1990 Jaijeet S. Roychowdhury
//  1990 Shen Lin
//  1992 Charles Hough
//  2002 Stephen R. Whiteley
// Copyright Regents of the University of California.  All rights reserved.
//-------------------------------------------------------------------------

#include "tradefs.h"


int
TRAdev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sTRAmodel *model = static_cast<sTRAmodel*>(genmod);
    IFvalue *value = &data->v;

    switch (param) {
    case TRA_MOD_TRA:
        break;
    case TRA_MOD_LEVEL:
        model->TRAlevel = value->iValue;
        model->TRAlevelGiven = true;
        break;
    case TRA_MOD_LEN:
        model->TRAlength = value->rValue;
        model->TRAlengthGiven = true;
        break;
    case TRA_MOD_L:
        model->TRAl = value->rValue;
        model->TRAlGiven = true;
        break;
    case TRA_MOD_C:
        model->TRAc = value->rValue;
        model->TRAcGiven = true;
        break;
    case TRA_MOD_R:
        model->TRAr = value->rValue;
        model->TRArGiven = true;
        break;
    case TRA_MOD_G:
        model->TRAg = value->rValue;
        model->TRAgGiven = true;
        break;
    case TRA_MOD_Z0:
        model->TRAz = value->rValue;
        model->TRAzGiven = true;
        break;
    case TRA_MOD_TD:
        model->TRAtd = value->rValue;
        model->TRAtdGiven = true;
        break;
    case TRA_MOD_F:
        model->TRAf = value->rValue;
        model->TRAfGiven = true;
        break;
    case TRA_MOD_NL:
        model->TRAnl = value->rValue;
        model->TRAnlGiven = true;
        break;
    case TRA_MOD_LININTERP:
        model->TRAhowToInterp = TRA_LININTERP;
        model->TRAhowToInterpGiven = true;
        break;
    case TRA_MOD_QUADINTERP:
        model->TRAhowToInterp = TRA_QUADINTERP;
        model->TRAhowToInterpGiven = true;
        break;
    case TRA_MOD_TRUNCDONTCUT:
        model->TRAlteConType = TRA_TRUNCDONTCUT;
        model->TRAlteConTypeGiven = true;
        break;
    case TRA_MOD_TRUNCCUTSL:
        model->TRAlteConType = TRA_TRUNCCUTSL;
        model->TRAlteConTypeGiven = true;
        break;
    case TRA_MOD_TRUNCCUTLTE:
        model->TRAlteConType = TRA_TRUNCCUTLTE;
        model->TRAlteConTypeGiven = true;
        break;
    case TRA_MOD_TRUNCCUTNR:
        model->TRAlteConType = TRA_TRUNCCUTNR;
        model->TRAlteConTypeGiven = true;
        break;
    case TRA_MOD_NOBREAKS:
        model->TRAbreakType = TRA_NOBREAKS;
        model->TRAbreakTypeGiven = true;
        break;
    case TRA_MOD_ALLBREAKS:
        model->TRAbreakType = TRA_ALLBREAKS;
        model->TRAbreakTypeGiven = true;
        break;
    case TRA_MOD_TESTBREAKS:
        model->TRAbreakType = TRA_TESTBREAKS;
        model->TRAbreakTypeGiven = true;
        break;
    case TRA_MOD_SLOPETOL:
        model->TRAslopetol = value->rValue;
        model->TRAslopetolGiven = true;
        break;
    case TRA_MOD_COMPACTREL:
        model->TRAstLineReltol = value->rValue;
        model->TRAstLineReltolGiven = true;
        break;
    case TRA_MOD_COMPACTABS:
        model->TRAstLineAbstol = value->rValue;
        model->TRAstLineAbstolGiven = true;
        break;
    case TRA_MOD_RELTOL:
        model->TRAreltol = value->rValue;
        model->TRAreltolGiven = true;
        break;
    case TRA_MOD_ABSTOL:
        model->TRAabstol = value->rValue;
        model->TRAabstolGiven = true;
        break;
    case TRA_MOD_LTRA:
        model->TRAlevel = CONV_LEVEL;
        model->TRAlevelGiven = true;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}
