
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

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
Modified by Dietmar Warning 2003 and Paolo Nenzi 2003
**********/

#include "diodefs.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


int
DIOdev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sDIOmodel *model = static_cast<sDIOmodel*>(genmod);
    IFvalue *value = &data->v;

    switch(param) {
        case DIO_MOD_IS:
            model->DIOsatCur = value->rValue;
            model->DIOsatCurGiven = TRUE;
            break;
        case DIO_MOD_JSW:
            model->DIOsatSWCur = value->rValue;
            model->DIOsatSWCurGiven = TRUE;
            break;

        case DIO_MOD_TNOM:
            model->DIOnomTemp = value->rValue+CONSTCtoK;
            model->DIOnomTempGiven = TRUE;
            break;
        case DIO_MOD_RS:
            model->DIOresist = value->rValue;
            model->DIOresistGiven = TRUE;
            break;
        case DIO_MOD_TRS:
            model->DIOresistTemp1 = value->rValue;
            model->DIOresistTemp1Given = TRUE;
            break;
        case DIO_MOD_TRS2:
            model->DIOresistTemp2 = value->rValue;
            model->DIOresistTemp2Given = TRUE;
            break;                          
        case DIO_MOD_N:
            model->DIOemissionCoeff = value->rValue;
            model->DIOemissionCoeffGiven = TRUE;
            break;
        case DIO_MOD_TT:
            model->DIOtransitTime = value->rValue;
            model->DIOtransitTimeGiven = TRUE;
            break;
        case DIO_MOD_TTT1:
            model->DIOtranTimeTemp1 = value->rValue;
            model->DIOtranTimeTemp1Given = TRUE;
            break;      
        case DIO_MOD_TTT2:
            model->DIOtranTimeTemp2 = value->rValue;
            model->DIOtranTimeTemp2Given = TRUE;
            break;                      
        case DIO_MOD_CJO:
            model->DIOjunctionCap = value->rValue;
            model->DIOjunctionCapGiven = TRUE;
            break;
        case DIO_MOD_VJ:
            model->DIOjunctionPot = value->rValue;
            model->DIOjunctionPotGiven = TRUE;
            break;
        case DIO_MOD_M:
            model->DIOgradingCoeff = value->rValue;
            model->DIOgradingCoeffGiven = TRUE;
            break;
        case DIO_MOD_TM1:
            model->DIOgradCoeffTemp1 = value->rValue;
            model->DIOgradCoeffTemp1Given = TRUE;
            break;
        case DIO_MOD_TM2:
            model->DIOgradCoeffTemp2 = value->rValue;
            model->DIOgradCoeffTemp2Given = TRUE;
            break;                  
        case DIO_MOD_CJSW:
            model->DIOjunctionSWCap = value->rValue;
            model->DIOjunctionSWCapGiven = TRUE;
            break;
        case DIO_MOD_VJSW:
            model->DIOjunctionSWPot = value->rValue;
            model->DIOjunctionSWPotGiven = TRUE;
            break;
        case DIO_MOD_MJSW:
            model->DIOgradingSWCoeff = value->rValue;
            model->DIOgradingSWCoeffGiven = TRUE;
            break;
        case DIO_MOD_IKF:
            model->DIOforwardKneeCurrent = value->rValue;
            model->DIOforwardKneeCurrentGiven = TRUE;
            break;
        case DIO_MOD_IKR:
            model->DIOreverseKneeCurrent = value->rValue;
            model->DIOreverseKneeCurrentGiven = TRUE;
            break;
            
        case DIO_MOD_EG:
            model->DIOactivationEnergy = value->rValue;
            model->DIOactivationEnergyGiven = TRUE;
            break;
        case DIO_MOD_XTI:
            model->DIOsaturationCurrentExp = value->rValue;
            model->DIOsaturationCurrentExpGiven = TRUE;
            break;
        case DIO_MOD_FC:
            model->DIOdepletionCapCoeff = value->rValue;
            model->DIOdepletionCapCoeffGiven = TRUE;
            break;
        case DIO_MOD_FCS:
            model->DIOdepletionSWcapCoeff = value->rValue;
            model->DIOdepletionSWcapCoeffGiven = TRUE;
            break;
        case DIO_MOD_BV:
            model->DIObreakdownVoltage = value->rValue;
            model->DIObreakdownVoltageGiven = TRUE;
            break;
        case DIO_MOD_IBV:
            model->DIObreakdownCurrent = value->rValue;
            model->DIObreakdownCurrentGiven = TRUE;
            break;
        case DIO_MOD_D:
            /* no action - we already know we are a diode, but this */
            /* makes life easier for spice-2 like parsers */
            break;
        case DIO_MOD_KF:
            model->DIOfNcoef = value->rValue;
            model->DIOfNcoefGiven = TRUE;
            break;
        case DIO_MOD_AF:
            model->DIOfNexp = value->rValue;
            model->DIOfNexpGiven = TRUE;
            break;
        case DIO_MOD_PJ:
            model->DIOpj = value->rValue;
            model->DIOpjGiven = TRUE;
            break;
        case DIO_MOD_AREA:
            model->DIOarea = value->rValue;
            model->DIOareaGiven = TRUE;
            break;
        case DIO_MOD_CTA:
            model->DIOcta = value->rValue;
            model->DIOctaGiven = TRUE;
            break;
        case DIO_MOD_CTP:
            model->DIOctp = value->rValue;
            model->DIOctpGiven = TRUE;
            break;
        case DIO_MOD_TCV:
            model->DIOtcv = value->rValue;
            model->DIOtcvGiven = TRUE;
            break;
        case DIO_MOD_TPB:
            model->DIOtpb = value->rValue;
            model->DIOtpbGiven = TRUE;
            break;
        case DIO_MOD_TPHP:
            model->DIOtphp = value->rValue;
            model->DIOtphpGiven = TRUE;
            break;
        case DIO_MOD_TLEV:
            model->DIOtlev = value->iValue;
            model->DIOtlevGiven = TRUE;
            break;
        case DIO_MOD_TLEVC:
            model->DIOtlevc = value->iValue;
            model->DIOtlevcGiven = TRUE;
            break;

        default:
            return(E_BADPARM);
    }
    return(OK);
}

