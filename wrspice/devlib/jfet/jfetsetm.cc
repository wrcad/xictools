
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
         1993 Stephen R. Whiteley
****************************************************************************/
/********** new in 3f2
Sydney University mods Copyright(c) 1989 Anthony E. Parker, David J. Skellern
    Laboratory for Communication Science Engineering
    Sydney University Department of Electrical Engineering, Australia
**********/

#include "jfetdefs.h"


int
JFETdev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sJFETmodel *model = static_cast<sJFETmodel*>(genmod);
    IFvalue *value = &data->v;

    switch (param) {
    case JFET_MOD_VTO:
        model->JFETthresholdGiven = true;
        model->JFETthreshold = value->rValue;
        break;
    case JFET_MOD_BETA:
        model->JFETbetaGiven = true;
        model->JFETbeta = value->rValue;
        break;
    case JFET_MOD_LAMBDA:
        model->JFETlModulationGiven = true;
        model->JFETlModulation = value->rValue;
        break;
    case JFET_MOD_RD:
        model->JFETdrainResistGiven = true;
        model->JFETdrainResist = value->rValue;
        break;
    case JFET_MOD_RS:
        model->JFETsourceResistGiven = true;
        model->JFETsourceResist = value->rValue;
        break;
    case JFET_MOD_CGS:
        model->JFETcapGSGiven = true;
        model->JFETcapGS = value->rValue;
        break;
    case JFET_MOD_CGD:
        model->JFETcapGDGiven = true;
        model->JFETcapGD = value->rValue;
        break;
    case JFET_MOD_PB:
        model->JFETgatePotentialGiven = true;
        model->JFETgatePotential = value->rValue;
        break;
    case JFET_MOD_IS:
        model->JFETgateSatCurrentGiven = true;
        model->JFETgateSatCurrent = value->rValue;
        break;
    case JFET_MOD_FC:
        model->JFETdepletionCapCoeffGiven = true;
        model->JFETdepletionCapCoeff = value->rValue;
        break;
    case JFET_MOD_NJF:
        if (value->iValue)
            model->JFETtype = NJF;
        break;
    case JFET_MOD_PJF:
        if (value->iValue)
            model->JFETtype = PJF;
        break;
    case JFET_MOD_TNOM:
        model->JFETtnomGiven = true;
        model->JFETtnom = value->rValue+CONSTCtoK;
        break;
    case JFET_MOD_KF:
        model->JFETfNcoefGiven = true;
        model->JFETfNcoef = value->rValue;
        break;
    case JFET_MOD_AF:
        model->JFETfNexpGiven = true;
        model->JFETfNexp = value->rValue;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}
