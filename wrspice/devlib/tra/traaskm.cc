
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
TRAdev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sTRAmodel *model = static_cast<const sTRAmodel*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case TRA_MOD_LEVEL:
        value->iValue = model->TRAlevel;
        data->type = IF_INTEGER;
        break;
    case TRA_MOD_LEN:
        value->rValue = model->TRAlength;
        break;
    case TRA_MOD_L:
        value->rValue = model->TRAl;
        break;
    case TRA_MOD_C:
        value->rValue = model->TRAc;
        break;
    case TRA_MOD_R:
        value->rValue = model->TRAr;
        break;
    case TRA_MOD_G:
        value->rValue = model->TRAg;
        break;
    case TRA_MOD_Z0:
        value->rValue = model->TRAz;
        break;
    case TRA_MOD_TD:
        value->rValue = model->TRAtd;
        break;
    case TRA_MOD_F:
        value->rValue = model->TRAf;
        break;
    case TRA_MOD_NL:
        value->rValue = model->TRAnl;
        break;
    case TRA_MOD_LININTERP:
        value->iValue = (model->TRAhowToInterp == TRA_MOD_LININTERP);
        data->type = IF_INTEGER;
        break;
    case TRA_MOD_QUADINTERP:
        value->iValue = (model->TRAhowToInterp == TRA_MOD_QUADINTERP);
        data->type = IF_INTEGER;
        break;
    case TRA_MOD_TRUNCDONTCUT:
        value->iValue = (model->TRAlteConType == TRA_MOD_TRUNCDONTCUT);
        data->type = IF_INTEGER;
        break;
    case TRA_MOD_TRUNCCUTSL:
        value->iValue = (model->TRAlteConType == TRA_MOD_TRUNCCUTSL);
        data->type = IF_INTEGER;
        break;
    case TRA_MOD_TRUNCCUTLTE:
        value->iValue = (model->TRAlteConType == TRA_MOD_TRUNCCUTLTE);
        data->type = IF_INTEGER;
        break;
    case TRA_MOD_TRUNCCUTNR:
        value->iValue = (model->TRAlteConType == TRA_MOD_TRUNCCUTNR);
        data->type = IF_INTEGER;
        break;
    case TRA_MOD_NOBREAKS:
        value->iValue = (model->TRAbreakType == TRA_MOD_NOBREAKS);
        data->type = IF_INTEGER;
        break;
    case TRA_MOD_ALLBREAKS:
        value->iValue = (model->TRAbreakType == TRA_MOD_ALLBREAKS);
        data->type = IF_INTEGER;
        break;
    case TRA_MOD_TESTBREAKS:
        value->iValue = (model->TRAbreakType == TRA_MOD_TESTBREAKS);
        data->type = IF_INTEGER;
        break;
    case TRA_MOD_SLOPETOL:
        value->rValue = model->TRAslopetol;
        break;
    case TRA_MOD_COMPACTREL:
        value->rValue = model->TRAstLineReltol;
        break;
    case TRA_MOD_COMPACTABS:
        value->rValue = model->TRAstLineAbstol;
        break;
    case TRA_MOD_RELTOL:
        value->rValue = model->TRAreltol;
        break;
    case TRA_MOD_ABSTOL:
        value->rValue = model->TRAabstol;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}


