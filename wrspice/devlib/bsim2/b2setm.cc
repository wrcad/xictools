
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
Authors: 1988 Min-Chie Jeng, Hong J. Park, Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b2defs.h"


int
B2dev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sB2model *model = static_cast<sB2model*>(genmod);
    IFvalue *value = &data->v;

    switch (param) {
    case BSIM2_MOD_VFB0 :
        model->B2vfb0 = value->rValue;
        model->B2vfb0Given = true;
        break;
    case BSIM2_MOD_VFBL :
        model->B2vfbL = value->rValue;
        model->B2vfbLGiven = true;
        break;
    case BSIM2_MOD_VFBW :
        model->B2vfbW = value->rValue;
        model->B2vfbWGiven = true;
        break;
    case BSIM2_MOD_PHI0 :
        model->B2phi0 = value->rValue;
        model->B2phi0Given = true;
        break;
    case BSIM2_MOD_PHIL :
        model->B2phiL = value->rValue;
        model->B2phiLGiven = true;
        break;
    case BSIM2_MOD_PHIW :
        model->B2phiW = value->rValue;
        model->B2phiWGiven = true;
        break;
    case BSIM2_MOD_K10 :
        model->B2k10 = value->rValue;
        model->B2k10Given = true;
        break;
    case BSIM2_MOD_K1L :
        model->B2k1L = value->rValue;
        model->B2k1LGiven = true;
        break;
    case BSIM2_MOD_K1W :
        model->B2k1W = value->rValue;
        model->B2k1WGiven = true;
        break;
    case BSIM2_MOD_K20 :
        model->B2k20 = value->rValue;
        model->B2k20Given = true;
        break;
    case BSIM2_MOD_K2L :
        model->B2k2L = value->rValue;
        model->B2k2LGiven = true;
        break;
    case BSIM2_MOD_K2W :
        model->B2k2W = value->rValue;
        model->B2k2WGiven = true;
        break;
    case BSIM2_MOD_ETA00 :
        model->B2eta00 = value->rValue;
        model->B2eta00Given = true;
        break;
    case BSIM2_MOD_ETA0L :
        model->B2eta0L = value->rValue;
        model->B2eta0LGiven = true;
        break;
    case BSIM2_MOD_ETA0W :
        model->B2eta0W = value->rValue;
        model->B2eta0WGiven = true;
        break;
    case BSIM2_MOD_ETAB0 :
        model->B2etaB0 = value->rValue;
        model->B2etaB0Given = true;
        break;
    case BSIM2_MOD_ETABL :
        model->B2etaBL = value->rValue;
        model->B2etaBLGiven = true;
        break;
    case BSIM2_MOD_ETABW :
        model->B2etaBW = value->rValue;
        model->B2etaBWGiven = true;
        break;
    case BSIM2_MOD_DELTAL :
        model->B2deltaL =  value->rValue;
        model->B2deltaLGiven = true;
        break;
    case BSIM2_MOD_DELTAW :
        model->B2deltaW =  value->rValue;
        model->B2deltaWGiven = true;
        break;
    case BSIM2_MOD_MOB00 :
        model->B2mob00 = value->rValue;
        model->B2mob00Given = true;
        break;
    case BSIM2_MOD_MOB0B0 :
        model->B2mob0B0 = value->rValue;
        model->B2mob0B0Given = true;
        break;
    case BSIM2_MOD_MOB0BL :
        model->B2mob0BL = value->rValue;
        model->B2mob0BLGiven = true;
        break;
    case BSIM2_MOD_MOB0BW :
        model->B2mob0BW = value->rValue;
        model->B2mob0BWGiven = true;
        break;
    case BSIM2_MOD_MOBS00 :
        model->B2mobs00 = value->rValue;
        model->B2mobs00Given = true;
        break;
    case BSIM2_MOD_MOBS0L :
        model->B2mobs0L = value->rValue;
        model->B2mobs0LGiven = true;
        break;
    case BSIM2_MOD_MOBS0W :
        model->B2mobs0W = value->rValue;
        model->B2mobs0WGiven = true;
        break;
    case BSIM2_MOD_MOBSB0 :
        model->B2mobsB0 = value->rValue;
        model->B2mobsB0Given = true;
        break;
    case BSIM2_MOD_MOBSBL :
        model->B2mobsBL = value->rValue;
        model->B2mobsBLGiven = true;
        break;
    case BSIM2_MOD_MOBSBW :
        model->B2mobsBW = value->rValue;
        model->B2mobsBWGiven = true;
        break;
    case BSIM2_MOD_MOB200 :
        model->B2mob200 = value->rValue;
        model->B2mob200Given = true;
        break;
    case BSIM2_MOD_MOB20L :
        model->B2mob20L = value->rValue;
        model->B2mob20LGiven = true;
        break;
    case BSIM2_MOD_MOB20W :
        model->B2mob20W = value->rValue;
        model->B2mob20WGiven = true;
        break;
    case BSIM2_MOD_MOB2B0 :
        model->B2mob2B0 = value->rValue;
        model->B2mob2B0Given = true;
        break;
    case BSIM2_MOD_MOB2BL :
        model->B2mob2BL = value->rValue;
        model->B2mob2BLGiven = true;
        break;
    case BSIM2_MOD_MOB2BW :
        model->B2mob2BW = value->rValue;
        model->B2mob2BWGiven = true;
        break;
    case BSIM2_MOD_MOB2G0 :
        model->B2mob2G0 = value->rValue;
        model->B2mob2G0Given = true;
        break;
    case BSIM2_MOD_MOB2GL :
        model->B2mob2GL = value->rValue;
        model->B2mob2GLGiven = true;
        break;
    case BSIM2_MOD_MOB2GW :
        model->B2mob2GW = value->rValue;
        model->B2mob2GWGiven = true;
        break;
    case BSIM2_MOD_MOB300 :
        model->B2mob300 = value->rValue;
        model->B2mob300Given = true;
        break;
    case BSIM2_MOD_MOB30L :
        model->B2mob30L = value->rValue;
        model->B2mob30LGiven = true;
        break;
    case BSIM2_MOD_MOB30W :
        model->B2mob30W = value->rValue;
        model->B2mob30WGiven = true;
        break;
    case BSIM2_MOD_MOB3B0 :
        model->B2mob3B0 = value->rValue;
        model->B2mob3B0Given = true;
        break;
    case BSIM2_MOD_MOB3BL :
        model->B2mob3BL = value->rValue;
        model->B2mob3BLGiven = true;
        break;
    case BSIM2_MOD_MOB3BW :
        model->B2mob3BW = value->rValue;
        model->B2mob3BWGiven = true;
        break;
    case BSIM2_MOD_MOB3G0 :
        model->B2mob3G0 = value->rValue;
        model->B2mob3G0Given = true;
        break;
    case BSIM2_MOD_MOB3GL :
        model->B2mob3GL = value->rValue;
        model->B2mob3GLGiven = true;
        break;
    case BSIM2_MOD_MOB3GW :
        model->B2mob3GW = value->rValue;
        model->B2mob3GWGiven = true;
        break;
    case BSIM2_MOD_MOB400 :
        model->B2mob400 = value->rValue;
        model->B2mob400Given = true;
        break;
    case BSIM2_MOD_MOB40L :
        model->B2mob40L = value->rValue;
        model->B2mob40LGiven = true;
        break;
    case BSIM2_MOD_MOB40W :
        model->B2mob40W = value->rValue;
        model->B2mob40WGiven = true;
        break;
    case BSIM2_MOD_MOB4B0 :
        model->B2mob4B0 = value->rValue;
        model->B2mob4B0Given = true;
        break;
    case BSIM2_MOD_MOB4BL :
        model->B2mob4BL = value->rValue;
        model->B2mob4BLGiven = true;
        break;
    case BSIM2_MOD_MOB4BW :
        model->B2mob4BW = value->rValue;
        model->B2mob4BWGiven = true;
        break;
    case BSIM2_MOD_MOB4G0 :
        model->B2mob4G0 = value->rValue;
        model->B2mob4G0Given = true;
        break;
    case BSIM2_MOD_MOB4GL :
        model->B2mob4GL = value->rValue;
        model->B2mob4GLGiven = true;
        break;
    case BSIM2_MOD_MOB4GW :
        model->B2mob4GW = value->rValue;
        model->B2mob4GWGiven = true;
        break;
    case BSIM2_MOD_UA00 :
        model->B2ua00 = value->rValue;
        model->B2ua00Given = true;
        break;
    case BSIM2_MOD_UA0L :
        model->B2ua0L = value->rValue;
        model->B2ua0LGiven = true;
        break;
    case BSIM2_MOD_UA0W :
        model->B2ua0W = value->rValue;
        model->B2ua0WGiven = true;
        break;
    case BSIM2_MOD_UAB0 :
        model->B2uaB0 = value->rValue;
        model->B2uaB0Given = true;
        break;
    case BSIM2_MOD_UABL :
        model->B2uaBL = value->rValue;
        model->B2uaBLGiven = true;
        break;
    case BSIM2_MOD_UABW :
        model->B2uaBW = value->rValue;
        model->B2uaBWGiven = true;
        break;
    case BSIM2_MOD_UB00 :
        model->B2ub00 = value->rValue;
        model->B2ub00Given = true;
        break;
    case BSIM2_MOD_UB0L :
        model->B2ub0L = value->rValue;
        model->B2ub0LGiven = true;
        break;
    case BSIM2_MOD_UB0W :
        model->B2ub0W = value->rValue;
        model->B2ub0WGiven = true;
        break;
    case BSIM2_MOD_UBB0 :
        model->B2ubB0 = value->rValue;
        model->B2ubB0Given = true;
        break;
    case BSIM2_MOD_UBBL :
        model->B2ubBL = value->rValue;
        model->B2ubBLGiven = true;
        break;
    case BSIM2_MOD_UBBW :
        model->B2ubBW = value->rValue;
        model->B2ubBWGiven = true;
        break;
    case BSIM2_MOD_U100 :
        model->B2u100 = value->rValue;
        model->B2u100Given = true;
        break;
    case BSIM2_MOD_U10L :
        model->B2u10L = value->rValue;
        model->B2u10LGiven = true;
        break;
    case BSIM2_MOD_U10W :
        model->B2u10W = value->rValue;
        model->B2u10WGiven = true;
        break;
    case BSIM2_MOD_U1B0 :
        model->B2u1B0 = value->rValue;
        model->B2u1B0Given = true;
        break;
    case BSIM2_MOD_U1BL :
        model->B2u1BL = value->rValue;
        model->B2u1BLGiven = true;
        break;
    case BSIM2_MOD_U1BW :
        model->B2u1BW = value->rValue;
        model->B2u1BWGiven = true;
        break;
    case BSIM2_MOD_U1D0 :
        model->B2u1D0 = value->rValue;
        model->B2u1D0Given = true;
        break;
    case BSIM2_MOD_U1DL :
        model->B2u1DL = value->rValue;
        model->B2u1DLGiven = true;
        break;
    case BSIM2_MOD_U1DW :
        model->B2u1DW = value->rValue;
        model->B2u1DWGiven = true;
        break;
    case BSIM2_MOD_N00 :
        model->B2n00 = value->rValue;
        model->B2n00Given = true;
        break;
    case BSIM2_MOD_N0L :
        model->B2n0L = value->rValue;
        model->B2n0LGiven = true;
        break;
    case BSIM2_MOD_N0W :
        model->B2n0W = value->rValue;
        model->B2n0WGiven = true;
        break;
    case BSIM2_MOD_NB0 :
        model->B2nB0 = value->rValue;
        model->B2nB0Given = true;
        break;
    case BSIM2_MOD_NBL :
        model->B2nBL = value->rValue;
        model->B2nBLGiven = true;
        break;
    case BSIM2_MOD_NBW :
        model->B2nBW = value->rValue;
        model->B2nBWGiven = true;
        break;
    case BSIM2_MOD_ND0 :
        model->B2nD0 = value->rValue;
        model->B2nD0Given = true;
        break;
    case BSIM2_MOD_NDL :
        model->B2nDL = value->rValue;
        model->B2nDLGiven = true;
        break;
    case BSIM2_MOD_NDW :
        model->B2nDW = value->rValue;
        model->B2nDWGiven = true;
        break;
    case BSIM2_MOD_VOF00 :
        model->B2vof00 = value->rValue;
        model->B2vof00Given = true;
        break;
    case BSIM2_MOD_VOF0L :
        model->B2vof0L = value->rValue;
        model->B2vof0LGiven = true;
        break;
    case BSIM2_MOD_VOF0W :
        model->B2vof0W = value->rValue;
        model->B2vof0WGiven = true;
        break;
    case BSIM2_MOD_VOFB0 :
        model->B2vofB0 = value->rValue;
        model->B2vofB0Given = true;
        break;
    case BSIM2_MOD_VOFBL :
        model->B2vofBL = value->rValue;
        model->B2vofBLGiven = true;
        break;
    case BSIM2_MOD_VOFBW :
        model->B2vofBW = value->rValue;
        model->B2vofBWGiven = true;
        break;
    case BSIM2_MOD_VOFD0 :
        model->B2vofD0 = value->rValue;
        model->B2vofD0Given = true;
        break;
    case BSIM2_MOD_VOFDL :
        model->B2vofDL = value->rValue;
        model->B2vofDLGiven = true;
        break;
    case BSIM2_MOD_VOFDW :
        model->B2vofDW = value->rValue;
        model->B2vofDWGiven = true;
        break;
    case BSIM2_MOD_AI00 :
        model->B2ai00 = value->rValue;
        model->B2ai00Given = true;
        break;
    case BSIM2_MOD_AI0L :
        model->B2ai0L = value->rValue;
        model->B2ai0LGiven = true;
        break;
    case BSIM2_MOD_AI0W :
        model->B2ai0W = value->rValue;
        model->B2ai0WGiven = true;
        break;
    case BSIM2_MOD_AIB0 :
        model->B2aiB0 = value->rValue;
        model->B2aiB0Given = true;
        break;
    case BSIM2_MOD_AIBL :
        model->B2aiBL = value->rValue;
        model->B2aiBLGiven = true;
        break;
    case BSIM2_MOD_AIBW :
        model->B2aiBW = value->rValue;
        model->B2aiBWGiven = true;
        break;
    case BSIM2_MOD_BI00 :
        model->B2bi00 = value->rValue;
        model->B2bi00Given = true;
        break;
    case BSIM2_MOD_BI0L :
        model->B2bi0L = value->rValue;
        model->B2bi0LGiven = true;
        break;
    case BSIM2_MOD_BI0W :
        model->B2bi0W = value->rValue;
        model->B2bi0WGiven = true;
        break;
    case BSIM2_MOD_BIB0 :
        model->B2biB0 = value->rValue;
        model->B2biB0Given = true;
        break;
    case BSIM2_MOD_BIBL :
        model->B2biBL = value->rValue;
        model->B2biBLGiven = true;
        break;
    case BSIM2_MOD_BIBW :
        model->B2biBW = value->rValue;
        model->B2biBWGiven = true;
        break;
    case BSIM2_MOD_VGHIGH0 :
        model->B2vghigh0 = value->rValue;
        model->B2vghigh0Given = true;
        break;
    case BSIM2_MOD_VGHIGHL :
        model->B2vghighL = value->rValue;
        model->B2vghighLGiven = true;
        break;
    case BSIM2_MOD_VGHIGHW :
        model->B2vghighW = value->rValue;
        model->B2vghighWGiven = true;
        break;
    case BSIM2_MOD_VGLOW0 :
        model->B2vglow0 = value->rValue;
        model->B2vglow0Given = true;
        break;
    case BSIM2_MOD_VGLOWL :
        model->B2vglowL = value->rValue;
        model->B2vglowLGiven = true;
        break;
    case BSIM2_MOD_VGLOWW :
        model->B2vglowW = value->rValue;
        model->B2vglowWGiven = true;
        break;
    case BSIM2_MOD_TOX :
        model->B2tox = value->rValue;
        model->B2toxGiven = true;
        break;
    case BSIM2_MOD_TEMP :
        model->B2temp = value->rValue;
        model->B2tempGiven = true;
        break;
    case BSIM2_MOD_VDD :
        model->B2vdd = value->rValue;
        model->B2vddGiven = true;
        break;
    case BSIM2_MOD_VGG :
        model->B2vgg = value->rValue;
        model->B2vggGiven = true;
        break;
    case BSIM2_MOD_VBB :
        model->B2vbb = value->rValue;
        model->B2vbbGiven = true;
        break;
    case BSIM2_MOD_CGSO :
        model->B2gateSourceOverlapCap = value->rValue;
        model->B2gateSourceOverlapCapGiven = true;
        break;
    case BSIM2_MOD_CGDO :
        model->B2gateDrainOverlapCap = value->rValue;
        model->B2gateDrainOverlapCapGiven = true;
        break;
    case BSIM2_MOD_CGBO :
        model->B2gateBulkOverlapCap = value->rValue;
        model->B2gateBulkOverlapCapGiven = true;
        break;
    case BSIM2_MOD_XPART :
        model->B2channelChargePartitionFlag = (int)value->rValue;
        model->B2channelChargePartitionFlagGiven = true;
        break;
    case BSIM2_MOD_RSH :
        model->B2sheetResistance = value->rValue;
        model->B2sheetResistanceGiven = true;
        break;
    case BSIM2_MOD_JS :
        model->B2jctSatCurDensity = value->rValue;
        model->B2jctSatCurDensityGiven = true;
        break;
    case BSIM2_MOD_PB :
        model->B2bulkJctPotential = value->rValue;
        model->B2bulkJctPotentialGiven = true;
        break;
    case BSIM2_MOD_MJ :
        model->B2bulkJctBotGradingCoeff = value->rValue;
        model->B2bulkJctBotGradingCoeffGiven = true;
        break;
    case BSIM2_MOD_PBSW :
        model->B2sidewallJctPotential = value->rValue;
        model->B2sidewallJctPotentialGiven = true;
        break;
    case BSIM2_MOD_MJSW :
        model->B2bulkJctSideGradingCoeff = value->rValue;
        model->B2bulkJctSideGradingCoeffGiven = true;
        break;
    case BSIM2_MOD_CJ :
        model->B2unitAreaJctCap = value->rValue;
        model->B2unitAreaJctCapGiven = true;
        break;
    case BSIM2_MOD_CJSW :
        model->B2unitLengthSidewallJctCap = value->rValue;
        model->B2unitLengthSidewallJctCapGiven = true;
        break;
    case BSIM2_MOD_DEFWIDTH :
        model->B2defaultWidth = value->rValue;
        model->B2defaultWidthGiven = true;
        break;
    case BSIM2_MOD_DELLENGTH :
        model->B2deltaLength = value->rValue;
        model->B2deltaLengthGiven = true;
        break;
    case BSIM2_MOD_NMOS  :
        if (value->iValue) {
            model->B2type = 1;
            model->B2typeGiven = true;
        }
        break;
    case BSIM2_MOD_PMOS  :
        if (value->iValue) {
            model->B2type = - 1;
            model->B2typeGiven = true;
        }
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}
