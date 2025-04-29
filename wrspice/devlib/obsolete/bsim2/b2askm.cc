
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
Authors: 1988 Hong June Park
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b2defs.h"


int
B2dev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sB2model *model = static_cast<const sB2model*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch(which) {
    case BSIM2_MOD_VFB0: 
        value->rValue = model->B2vfb0; 
        break;
    case  BSIM2_MOD_VFBL :
      value->rValue = model->B2vfbL;
        break;
    case  BSIM2_MOD_VFBW :
      value->rValue = model->B2vfbW;
        break;
    case  BSIM2_MOD_PHI0 :
      value->rValue = model->B2phi0;
        break;
    case  BSIM2_MOD_PHIL :
      value->rValue = model->B2phiL;
        break;
    case  BSIM2_MOD_PHIW :
      value->rValue = model->B2phiW;
        break;
    case  BSIM2_MOD_K10 :
      value->rValue = model->B2k10;
        break;
    case  BSIM2_MOD_K1L :
      value->rValue = model->B2k1L;
        break;
    case  BSIM2_MOD_K1W :
      value->rValue = model->B2k1W;
        break;
    case  BSIM2_MOD_K20 :
      value->rValue = model->B2k20;
        break;
    case  BSIM2_MOD_K2L :
      value->rValue = model->B2k2L;
        break;
    case  BSIM2_MOD_K2W :
      value->rValue = model->B2k2W;
        break;
    case  BSIM2_MOD_ETA00 :
      value->rValue = model->B2eta00;
        break;
    case  BSIM2_MOD_ETA0L :
      value->rValue = model->B2eta0L;
        break;
    case  BSIM2_MOD_ETA0W :
      value->rValue = model->B2eta0W;
        break;
    case  BSIM2_MOD_ETAB0 :
      value->rValue = model->B2etaB0;
        break;
    case  BSIM2_MOD_ETABL :
      value->rValue = model->B2etaBL;
        break;
    case  BSIM2_MOD_ETABW :
      value->rValue = model->B2etaBW;
        break;
    case  BSIM2_MOD_DELTAL :
      value->rValue = model->B2deltaL;
        break;
    case  BSIM2_MOD_DELTAW :
      value->rValue = model->B2deltaW;
        break;
    case  BSIM2_MOD_MOB00 :
      value->rValue = model->B2mob00;
        break;
    case  BSIM2_MOD_MOB0B0 :
      value->rValue = model->B2mob0B0;
        break;
    case  BSIM2_MOD_MOB0BL :
      value->rValue = model->B2mob0BL;
        break;
    case  BSIM2_MOD_MOB0BW :
      value->rValue = model->B2mob0BW;
        break;
    case  BSIM2_MOD_MOBS00 :
      value->rValue = model->B2mobs00;
        break;
    case  BSIM2_MOD_MOBS0L :
      value->rValue = model->B2mobs0L;
        break;
    case  BSIM2_MOD_MOBS0W :
      value->rValue = model->B2mobs0W;
        break;
    case  BSIM2_MOD_MOBSB0 :
      value->rValue = model->B2mobsB0;
        break;
    case  BSIM2_MOD_MOBSBL :
      value->rValue = model->B2mobsBL;
        break;
    case  BSIM2_MOD_MOBSBW :
      value->rValue = model->B2mobsBW;
        break;
    case  BSIM2_MOD_MOB200 :
      value->rValue = model->B2mob200;
        break;
    case  BSIM2_MOD_MOB20L :
      value->rValue = model->B2mob20L;
        break;
    case  BSIM2_MOD_MOB20W :
      value->rValue = model->B2mob20W;
        break;
    case  BSIM2_MOD_MOB2B0 :
      value->rValue = model->B2mob2B0;
        break;
    case  BSIM2_MOD_MOB2BL :
      value->rValue = model->B2mob2BL;
        break;
    case  BSIM2_MOD_MOB2BW :
      value->rValue = model->B2mob2BW;
        break;
    case  BSIM2_MOD_MOB2G0 :
      value->rValue = model->B2mob2G0;
        break;
    case  BSIM2_MOD_MOB2GL :
      value->rValue = model->B2mob2GL;
        break;
    case  BSIM2_MOD_MOB2GW :
      value->rValue = model->B2mob2GW;
        break;
    case  BSIM2_MOD_MOB300 :
      value->rValue = model->B2mob300;
        break;
    case  BSIM2_MOD_MOB30L :
      value->rValue = model->B2mob30L;
        break;
    case  BSIM2_MOD_MOB30W :
      value->rValue = model->B2mob30W;
        break;
    case  BSIM2_MOD_MOB3B0 :
      value->rValue = model->B2mob3B0;
        break;
    case  BSIM2_MOD_MOB3BL :
      value->rValue = model->B2mob3BL;
        break;
    case  BSIM2_MOD_MOB3BW :
      value->rValue = model->B2mob3BW;
        break;
    case  BSIM2_MOD_MOB3G0 :
      value->rValue = model->B2mob3G0;
        break;
    case  BSIM2_MOD_MOB3GL :
      value->rValue = model->B2mob3GL;
        break;
    case  BSIM2_MOD_MOB3GW :
      value->rValue = model->B2mob3GW;
        break;
    case  BSIM2_MOD_MOB400 :
      value->rValue = model->B2mob400;
        break;
    case  BSIM2_MOD_MOB40L :
      value->rValue = model->B2mob40L;
        break;
    case  BSIM2_MOD_MOB40W :
      value->rValue = model->B2mob40W;
        break;
    case  BSIM2_MOD_MOB4B0 :
      value->rValue = model->B2mob4B0;
        break;
    case  BSIM2_MOD_MOB4BL :
      value->rValue = model->B2mob4BL;
        break;
    case  BSIM2_MOD_MOB4BW :
      value->rValue = model->B2mob4BW;
        break;
    case  BSIM2_MOD_MOB4G0 :
      value->rValue = model->B2mob4G0;
        break;
    case  BSIM2_MOD_MOB4GL :
      value->rValue = model->B2mob4GL;
        break;
    case  BSIM2_MOD_MOB4GW :
      value->rValue = model->B2mob4GW;
        break;
    case  BSIM2_MOD_UA00 :
      value->rValue = model->B2ua00;
        break;
    case  BSIM2_MOD_UA0L :
      value->rValue = model->B2ua0L;
        break;
    case  BSIM2_MOD_UA0W :
      value->rValue = model->B2ua0W;
        break;
    case  BSIM2_MOD_UAB0 :
      value->rValue = model->B2uaB0;
        break;
    case  BSIM2_MOD_UABL :
      value->rValue = model->B2uaBL;
        break;
    case  BSIM2_MOD_UABW :
      value->rValue = model->B2uaBW;
        break;
    case  BSIM2_MOD_UB00 :
      value->rValue = model->B2ub00;
        break;
    case  BSIM2_MOD_UB0L :
      value->rValue = model->B2ub0L;
        break;
    case  BSIM2_MOD_UB0W :
      value->rValue = model->B2ub0W;
        break;
    case  BSIM2_MOD_UBB0 :
      value->rValue = model->B2ubB0;
        break;
    case  BSIM2_MOD_UBBL :
      value->rValue = model->B2ubBL;
        break;
    case  BSIM2_MOD_UBBW :
      value->rValue = model->B2ubBW;
        break;
    case  BSIM2_MOD_U100 :
      value->rValue = model->B2u100;
        break;
    case  BSIM2_MOD_U10L :
      value->rValue = model->B2u10L;
        break;
    case  BSIM2_MOD_U10W :
      value->rValue = model->B2u10W;
        break;
    case  BSIM2_MOD_U1B0 :
      value->rValue = model->B2u1B0;
        break;
    case  BSIM2_MOD_U1BL :
      value->rValue = model->B2u1BL;
        break;
    case  BSIM2_MOD_U1BW :
      value->rValue = model->B2u1BW;
        break;
    case  BSIM2_MOD_U1D0 :
      value->rValue = model->B2u1D0;
        break;
    case  BSIM2_MOD_U1DL :
      value->rValue = model->B2u1DL;
        break;
    case  BSIM2_MOD_U1DW :
      value->rValue = model->B2u1DW;
        break;
    case  BSIM2_MOD_N00 :
      value->rValue = model->B2n00;
        break;
    case  BSIM2_MOD_N0L :
      value->rValue = model->B2n0L;
        break;
    case  BSIM2_MOD_N0W :
      value->rValue = model->B2n0W;
        break;
    case  BSIM2_MOD_NB0 :
      value->rValue = model->B2nB0;
        break;
    case  BSIM2_MOD_NBL :
      value->rValue = model->B2nBL;
        break;
    case  BSIM2_MOD_NBW :
      value->rValue = model->B2nBW;
        break;
    case  BSIM2_MOD_ND0 :
      value->rValue = model->B2nD0;
        break;
    case  BSIM2_MOD_NDL :
      value->rValue = model->B2nDL;
        break;
    case  BSIM2_MOD_NDW :
      value->rValue = model->B2nDW;
        break;
    case  BSIM2_MOD_VOF00 :
      value->rValue = model->B2vof00;
        break;
    case  BSIM2_MOD_VOF0L :
      value->rValue = model->B2vof0L;
        break;
    case  BSIM2_MOD_VOF0W :
      value->rValue = model->B2vof0W;
        break;
    case  BSIM2_MOD_VOFB0 :
      value->rValue = model->B2vofB0;
        break;
    case  BSIM2_MOD_VOFBL :
      value->rValue = model->B2vofBL;
        break;
    case  BSIM2_MOD_VOFBW :
      value->rValue = model->B2vofBW;
        break;
    case  BSIM2_MOD_VOFD0 :
      value->rValue = model->B2vofD0;
        break;
    case  BSIM2_MOD_VOFDL :
      value->rValue = model->B2vofDL;
        break;
    case  BSIM2_MOD_VOFDW :
      value->rValue = model->B2vofDW;
        break;
    case  BSIM2_MOD_AI00 :
      value->rValue = model->B2ai00;
        break;
    case  BSIM2_MOD_AI0L :
      value->rValue = model->B2ai0L;
        break;
    case  BSIM2_MOD_AI0W :
      value->rValue = model->B2ai0W;
        break;
    case  BSIM2_MOD_AIB0 :
      value->rValue = model->B2aiB0;
        break;
    case  BSIM2_MOD_AIBL :
      value->rValue = model->B2aiBL;
        break;
    case  BSIM2_MOD_AIBW :
      value->rValue = model->B2aiBW;
        break;
    case  BSIM2_MOD_BI00 :
      value->rValue = model->B2bi00;
        break;
    case  BSIM2_MOD_BI0L :
      value->rValue = model->B2bi0L;
        break;
    case  BSIM2_MOD_BI0W :
      value->rValue = model->B2bi0W;
        break;
    case  BSIM2_MOD_BIB0 :
      value->rValue = model->B2biB0;
        break;
    case  BSIM2_MOD_BIBL :
      value->rValue = model->B2biBL;
        break;
    case  BSIM2_MOD_BIBW :
      value->rValue = model->B2biBW;
        break;
    case  BSIM2_MOD_VGHIGH0 :
      value->rValue = model->B2vghigh0;
        break;
    case  BSIM2_MOD_VGHIGHL :
      value->rValue = model->B2vghighL;
        break;
    case  BSIM2_MOD_VGHIGHW :
      value->rValue = model->B2vghighW;
        break;
    case  BSIM2_MOD_VGLOW0 :
      value->rValue = model->B2vglow0;
        break;
    case  BSIM2_MOD_VGLOWL :
      value->rValue = model->B2vglowL;
        break;
    case  BSIM2_MOD_VGLOWW :
      value->rValue = model->B2vglowW;
        break;
    case  BSIM2_MOD_TOX :
      value->rValue = model->B2tox;
        break;
    case  BSIM2_MOD_TEMP :
      value->rValue = model->B2temp;
        break;
    case  BSIM2_MOD_VDD :
      value->rValue = model->B2vdd;
        break;
    case  BSIM2_MOD_VGG :
      value->rValue = model->B2vgg;
        break;
    case  BSIM2_MOD_VBB :
      value->rValue = model->B2vbb;
        break;
    case BSIM2_MOD_CGSO:
        value->rValue = model->B2gateSourceOverlapCap; 
        break;
    case BSIM2_MOD_CGDO:
        value->rValue = model->B2gateDrainOverlapCap; 
        break;
    case BSIM2_MOD_CGBO:
        value->rValue = model->B2gateBulkOverlapCap; 
        break;
    case BSIM2_MOD_XPART:
        value->iValue = (int)model->B2channelChargePartitionFlag; 
        data->type = IF_INTEGER;
        break;
    case BSIM2_MOD_RSH:
        value->rValue = model->B2sheetResistance; 
        break;
    case BSIM2_MOD_JS:
        value->rValue = model->B2jctSatCurDensity; 
        break;
    case BSIM2_MOD_PB:
        value->rValue = model->B2bulkJctPotential; 
        break;
    case BSIM2_MOD_MJ:
        value->rValue = model->B2bulkJctBotGradingCoeff; 
        break;
    case BSIM2_MOD_PBSW:
        value->rValue = model->B2sidewallJctPotential; 
        break;
    case BSIM2_MOD_MJSW:
        value->rValue = model->B2bulkJctSideGradingCoeff; 
        break;
    case BSIM2_MOD_CJ:
        value->rValue = model->B2unitAreaJctCap; 
        break;
    case BSIM2_MOD_CJSW:
        value->rValue = model->B2unitLengthSidewallJctCap; 
        break;
    case BSIM2_MOD_DEFWIDTH:
        value->rValue = model->B2defaultWidth; 
        break;
    case BSIM2_MOD_DELLENGTH:
        value->rValue = model->B2deltaLength; 
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}


