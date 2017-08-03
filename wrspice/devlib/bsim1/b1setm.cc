
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
Authors: 1985 Hong J. Park, Thomas L. Quarles 
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b1defs.h"


int
B1dev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sB1model *model = static_cast<sB1model*>(genmod);
    IFvalue *value = &data->v;

    switch (param) {
    case BSIM1_MOD_VFB0:
        model->B1vfb0 = value->rValue;
        model->B1vfb0Given = true;
        break;
    case BSIM1_MOD_VFBL:
        model->B1vfbL = value->rValue;
        model->B1vfbLGiven = true;
        break;
    case BSIM1_MOD_VFBW:
        model->B1vfbW = value->rValue;
        model->B1vfbWGiven = true;
        break;
    case BSIM1_MOD_PHI0:
        model->B1phi0 = value->rValue;
        model->B1phi0Given = true;
        break;
    case BSIM1_MOD_PHIL:
        model->B1phiL = value->rValue;
        model->B1phiLGiven = true;
        break;
    case BSIM1_MOD_PHIW:
        model->B1phiW = value->rValue;
        model->B1phiWGiven = true;
        break;
    case BSIM1_MOD_K10:
        model->B1K10 = value->rValue;
        model->B1K10Given = true;
        break;
    case BSIM1_MOD_K1L:
        model->B1K1L = value->rValue;
        model->B1K1LGiven = true;
        break;
    case BSIM1_MOD_K1W:
        model->B1K1W = value->rValue;
        model->B1K1WGiven = true;
        break;
    case BSIM1_MOD_K20:
        model->B1K20 = value->rValue;
        model->B1K20Given = true;
        break;
    case BSIM1_MOD_K2L:
        model->B1K2L = value->rValue;
        model->B1K2LGiven = true;
        break;
    case BSIM1_MOD_K2W:
        model->B1K2W = value->rValue;
        model->B1K2WGiven = true;
        break;
    case BSIM1_MOD_ETA0:
        model->B1eta0 = value->rValue;
        model->B1eta0Given = true;
        break;
    case BSIM1_MOD_ETAL:
        model->B1etaL = value->rValue;
        model->B1etaLGiven = true;
        break;
    case BSIM1_MOD_ETAW:
        model->B1etaW = value->rValue;
        model->B1etaWGiven = true;
        break;
    case BSIM1_MOD_ETAB0:
        model->B1etaB0 = value->rValue;
        model->B1etaB0Given = true;
        break;
    case BSIM1_MOD_ETABL:
        model->B1etaBl = value->rValue;
        model->B1etaBlGiven = true;
        break;
    case BSIM1_MOD_ETABW:
        model->B1etaBw = value->rValue;
        model->B1etaBwGiven = true;
        break;
    case BSIM1_MOD_ETAD0:
        model->B1etaD0 = value->rValue;
        model->B1etaD0Given = true;
        break;
    case BSIM1_MOD_ETADL:
        model->B1etaDl = value->rValue;
        model->B1etaDlGiven = true;
        break;
    case BSIM1_MOD_ETADW:
        model->B1etaDw = value->rValue;
        model->B1etaDwGiven = true;
        break;
    case BSIM1_MOD_DELTAL:
        model->B1deltaL =  value->rValue;
        model->B1deltaLGiven = true;
        break;
    case BSIM1_MOD_DELTAW:
        model->B1deltaW =  value->rValue;
        model->B1deltaWGiven = true;
        break;
    case BSIM1_MOD_MOBZERO:
        model->B1mobZero = value->rValue;
        model->B1mobZeroGiven = true;
        break;
    case BSIM1_MOD_MOBZEROB0:
        model->B1mobZeroB0 = value->rValue;
        model->B1mobZeroB0Given = true;
        break;
    case BSIM1_MOD_MOBZEROBL:
        model->B1mobZeroBl = value->rValue;
        model->B1mobZeroBlGiven = true;
        break;
    case BSIM1_MOD_MOBZEROBW:
        model->B1mobZeroBw = value->rValue;
        model->B1mobZeroBwGiven = true;
        break;
    case BSIM1_MOD_MOBVDD0:
        model->B1mobVdd0 = value->rValue;
        model->B1mobVdd0Given = true;
        break;
    case BSIM1_MOD_MOBVDDL:
        model->B1mobVddl = value->rValue;
        model->B1mobVddlGiven = true;
        break;
    case BSIM1_MOD_MOBVDDW:
        model->B1mobVddw = value->rValue;
        model->B1mobVddwGiven = true;
        break;
    case BSIM1_MOD_MOBVDDB0:
        model->B1mobVddB0 = value->rValue;
        model->B1mobVddB0Given = true;
        break;
    case BSIM1_MOD_MOBVDDBL:
        model->B1mobVddBl = value->rValue;
        model->B1mobVddBlGiven = true;
        break;
    case BSIM1_MOD_MOBVDDBW:
        model->B1mobVddBw = value->rValue;
        model->B1mobVddBwGiven = true;
        break;
    case BSIM1_MOD_MOBVDDD0:
        model->B1mobVddD0 = value->rValue;
        model->B1mobVddD0Given = true;
        break;
    case BSIM1_MOD_MOBVDDDL:
        model->B1mobVddDl = value->rValue;
        model->B1mobVddDlGiven = true;
        break;
    case BSIM1_MOD_MOBVDDDW:
        model->B1mobVddDw = value->rValue;
        model->B1mobVddDwGiven = true;
        break;
    case BSIM1_MOD_UGS0:
        model->B1ugs0 = value->rValue;
        model->B1ugs0Given = true;
        break;
    case BSIM1_MOD_UGSL:
        model->B1ugsL = value->rValue;
        model->B1ugsLGiven = true;
        break;
    case BSIM1_MOD_UGSW:
        model->B1ugsW = value->rValue;
        model->B1ugsWGiven = true;
        break;
    case BSIM1_MOD_UGSB0:
        model->B1ugsB0 = value->rValue;
        model->B1ugsB0Given = true;
        break;
    case BSIM1_MOD_UGSBL:
        model->B1ugsBL = value->rValue;
        model->B1ugsBLGiven = true;
        break;
    case BSIM1_MOD_UGSBW:
        model->B1ugsBW = value->rValue;
        model->B1ugsBWGiven = true;
        break;
    case BSIM1_MOD_UDS0:
        model->B1uds0 = value->rValue;
        model->B1uds0Given = true;
        break;
    case BSIM1_MOD_UDSL:
        model->B1udsL = value->rValue;
        model->B1udsLGiven = true;
        break;
    case BSIM1_MOD_UDSW:
        model->B1udsW = value->rValue;
        model->B1udsWGiven = true;
        break;
    case BSIM1_MOD_UDSB0:
        model->B1udsB0 = value->rValue;
        model->B1udsB0Given = true;
        break;
    case BSIM1_MOD_UDSBL:
        model->B1udsBL = value->rValue;
        model->B1udsBLGiven = true;
        break;
    case BSIM1_MOD_UDSBW:
        model->B1udsBW = value->rValue;
        model->B1udsBWGiven = true;
        break;
    case BSIM1_MOD_UDSD0:
        model->B1udsD0 = value->rValue;
        model->B1udsD0Given = true;
        break;
    case BSIM1_MOD_UDSDL:
        model->B1udsDL = value->rValue;
        model->B1udsDLGiven = true;
        break;
    case BSIM1_MOD_UDSDW:
        model->B1udsDW = value->rValue;
        model->B1udsDWGiven = true;
        break;
    case BSIM1_MOD_N00:
        model->B1subthSlope0 = value->rValue;
        model->B1subthSlope0Given = true;
        break;
    case BSIM1_MOD_N0L:
        model->B1subthSlopeL = value->rValue;
        model->B1subthSlopeLGiven = true;
        break;
    case BSIM1_MOD_N0W:
        model->B1subthSlopeW = value->rValue;
        model->B1subthSlopeWGiven = true;
        break;
    case BSIM1_MOD_NB0:
        model->B1subthSlopeB0 = value->rValue;
        model->B1subthSlopeB0Given = true;
        break;
    case BSIM1_MOD_NBL:
        model->B1subthSlopeBL = value->rValue;
        model->B1subthSlopeBLGiven = true;
        break;
    case BSIM1_MOD_NBW:
        model->B1subthSlopeBW = value->rValue;
        model->B1subthSlopeBWGiven = true;
        break;
    case BSIM1_MOD_ND0:
        model->B1subthSlopeD0 = value->rValue;
        model->B1subthSlopeD0Given = true;
        break;
    case BSIM1_MOD_NDL:
        model->B1subthSlopeDL = value->rValue;
        model->B1subthSlopeDLGiven = true;
        break;
    case BSIM1_MOD_NDW:
        model->B1subthSlopeDW = value->rValue;
        model->B1subthSlopeDWGiven = true;
        break;
    case BSIM1_MOD_TOX:
        model->B1oxideThickness = value->rValue;
        model->B1oxideThicknessGiven = true;
        break;
    case BSIM1_MOD_TEMP:
        model->B1temp = value->rValue;
        model->B1tempGiven = true;
        break;
    case BSIM1_MOD_VDD:
        model->B1vdd = value->rValue;
        model->B1vddGiven = true;
        break;
    case BSIM1_MOD_CGSO:
        model->B1gateSourceOverlapCap = value->rValue;
        model->B1gateSourceOverlapCapGiven = true;
        break;
    case BSIM1_MOD_CGDO:
        model->B1gateDrainOverlapCap = value->rValue;
        model->B1gateDrainOverlapCapGiven = true;
        break;
    case BSIM1_MOD_CGBO:
        model->B1gateBulkOverlapCap = value->rValue;
        model->B1gateBulkOverlapCapGiven = true;
        break;
    case BSIM1_MOD_XPART:
        model->B1channelChargePartitionFlag = (int)value->rValue;
        model->B1channelChargePartitionFlagGiven = true;
        break;
    case BSIM1_MOD_RSH:
        model->B1sheetResistance = value->rValue;
        model->B1sheetResistanceGiven = true;
        break;
    case BSIM1_MOD_JS:
        model->B1jctSatCurDensity = value->rValue;
        model->B1jctSatCurDensityGiven = true;
        break;
    case BSIM1_MOD_PB:
        model->B1bulkJctPotential = value->rValue;
        model->B1bulkJctPotentialGiven = true;
        break;
    case BSIM1_MOD_MJ:
        model->B1bulkJctBotGradingCoeff = value->rValue;
        model->B1bulkJctBotGradingCoeffGiven = true;
        break;
    case BSIM1_MOD_PBSW:
        model->B1sidewallJctPotential = value->rValue;
        model->B1sidewallJctPotentialGiven = true;
        break;
    case BSIM1_MOD_MJSW:
        model->B1bulkJctSideGradingCoeff = value->rValue;
        model->B1bulkJctSideGradingCoeffGiven = true;
        break;
    case BSIM1_MOD_CJ:
        model->B1unitAreaJctCap = value->rValue;
        model->B1unitAreaJctCapGiven = true;
        break;
    case BSIM1_MOD_CJSW:
        model->B1unitLengthSidewallJctCap = value->rValue;
        model->B1unitLengthSidewallJctCapGiven = true;
        break;
    case BSIM1_MOD_DEFWIDTH:
        model->B1defaultWidth = value->rValue;
        model->B1defaultWidthGiven = true;
        break;
    case BSIM1_MOD_DELLENGTH:
        model->B1deltaLength = value->rValue;
        model->B1deltaLengthGiven = true;
        break;
    case BSIM1_MOD_NMOS:
        if (value->iValue) {
            model->B1type = 1;
            model->B1typeGiven = true;
        }
        break;
    case BSIM1_MOD_PMOS:
        if (value->iValue) {
            model->B1type = - 1;
            model->B1typeGiven = true;
        }
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}
