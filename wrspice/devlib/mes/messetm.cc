
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
Authors: 1985 S. Hwang
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mesdefs.h"


int
MESdev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sMESmodel *model = static_cast<sMESmodel*>(genmod);
    IFvalue *value = &data->v;

    switch (param) {
    case MES_MOD_VTO:
        model->MESthresholdGiven = true;
        model->MESthreshold = value->rValue;
        break;
    case MES_MOD_ALPHA:
        model->MESalphaGiven = true;
        model->MESalpha = value->rValue;
        break;
    case MES_MOD_BETA:
        model->MESbetaGiven = true;
        model->MESbeta = value->rValue;
        break;
    case MES_MOD_LAMBDA:
        model->MESlModulationGiven = true;
        model->MESlModulation = value->rValue;
        break;
    case MES_MOD_B:
        model->MESbGiven = true;
        model->MESb = value->rValue;
        break;
    case MES_MOD_RD:
        model->MESdrainResistGiven = true;
        model->MESdrainResist = value->rValue;
        break;
    case MES_MOD_RS:
        model->MESsourceResistGiven = true;
        model->MESsourceResist = value->rValue;
        break;
    case MES_MOD_CGS:
        model->MEScapGSGiven = true;
        model->MEScapGS = value->rValue;
        break;
    case MES_MOD_CGD:
        model->MEScapGDGiven = true;
        model->MEScapGD = value->rValue;
        break;
    case MES_MOD_PB:
        model->MESgatePotentialGiven = true;
        model->MESgatePotential = value->rValue;
        break;
    case MES_MOD_IS:
        model->MESgateSatCurrentGiven = true;
        model->MESgateSatCurrent = value->rValue;
        break;
    case MES_MOD_FC:
        model->MESdepletionCapCoeffGiven = true;
        model->MESdepletionCapCoeff = value->rValue;
        break;
    case MES_MOD_NMF:
        if(value->iValue)
            model->MEStype = NMF;
        break;
    case MES_MOD_PMF:
        if(value->iValue)
            model->MEStype = PMF;
        break;
    case MES_MOD_KF:
        model->MESfNcoefGiven = true;
        model->MESfNcoef = value->rValue;
        break;
    case MES_MOD_AF:
        model->MESfNexpGiven = true;
        model->MESfNexp = value->rValue;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}

