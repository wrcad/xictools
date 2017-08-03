
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
Authors: 1988 Hong J. Park
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b1defs.h"


int
B1dev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sB1model *model = static_cast<const sB1model*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case BSIM1_MOD_VFB0: 
        value->rValue = model->B1vfb0; 
        break;
    case BSIM1_MOD_VFBL:
        value->rValue = model->B1vfbL; 
        break;
    case BSIM1_MOD_VFBW:
        value->rValue = model->B1vfbW; 
        break;
    case BSIM1_MOD_PHI0:
        value->rValue = model->B1phi0; 
        break;
    case BSIM1_MOD_PHIL:
        value->rValue = model->B1phiL; 
        break;
    case BSIM1_MOD_PHIW:
        value->rValue = model->B1phiW; 
        break;
    case BSIM1_MOD_K10:
        value->rValue = model->B1K10; 
        break;
    case BSIM1_MOD_K1L:
        value->rValue = model->B1K1L; 
        break;
    case BSIM1_MOD_K1W:
        value->rValue = model->B1K1W; 
        break;
    case BSIM1_MOD_K20:
        value->rValue = model->B1K20; 
        break;
    case BSIM1_MOD_K2L:
        value->rValue = model->B1K2L; 
        break;
    case BSIM1_MOD_K2W:
        value->rValue = model->B1K2W; 
        break;
    case BSIM1_MOD_ETA0:
        value->rValue = model->B1eta0; 
        break;
    case BSIM1_MOD_ETAL:
        value->rValue = model->B1etaL; 
        break;
    case BSIM1_MOD_ETAW:
        value->rValue = model->B1etaW; 
        break;
    case BSIM1_MOD_ETAB0:
        value->rValue = model->B1etaB0; 
        break;
    case BSIM1_MOD_ETABL:
        value->rValue = model->B1etaBl; 
        break;
    case BSIM1_MOD_ETABW:
        value->rValue = model->B1etaBw; 
        break;
    case BSIM1_MOD_ETAD0:
        value->rValue = model->B1etaD0; 
        break;
    case BSIM1_MOD_ETADL:
        value->rValue = model->B1etaDl; 
        break;
    case BSIM1_MOD_ETADW:
        value->rValue = model->B1etaDw; 
        break;
    case BSIM1_MOD_DELTAL:
        value->rValue = model->B1deltaL; 
        break;
    case BSIM1_MOD_DELTAW:
        value->rValue = model->B1deltaW; 
        break;
    case BSIM1_MOD_MOBZERO:
        value->rValue = model->B1mobZero; 
        break;
    case BSIM1_MOD_MOBZEROB0:
        value->rValue = model->B1mobZeroB0; 
        break;
    case BSIM1_MOD_MOBZEROBL:
        value->rValue = model->B1mobZeroBl; 
        break;
    case BSIM1_MOD_MOBZEROBW:
        value->rValue = model->B1mobZeroBw; 
        break;
    case BSIM1_MOD_MOBVDD0:
        value->rValue = model->B1mobVdd0; 
        break;
    case BSIM1_MOD_MOBVDDL:
        value->rValue = model->B1mobVddl; 
        break;
    case BSIM1_MOD_MOBVDDW:
        value->rValue = model->B1mobVddw; 
        break;
    case BSIM1_MOD_MOBVDDB0:
        value->rValue = model->B1mobVddB0; 
        break;
    case BSIM1_MOD_MOBVDDBL:
        value->rValue = model->B1mobVddBl; 
        break;
    case BSIM1_MOD_MOBVDDBW:
        value->rValue = model->B1mobVddBw; 
        break;
    case BSIM1_MOD_MOBVDDD0:
        value->rValue = model->B1mobVddD0; 
        break;
    case BSIM1_MOD_MOBVDDDL:
        value->rValue = model->B1mobVddDl; 
        break;
    case BSIM1_MOD_MOBVDDDW:
        value->rValue = model->B1mobVddDw; 
        break;
    case BSIM1_MOD_UGS0:
        value->rValue = model->B1ugs0; 
        break;
    case BSIM1_MOD_UGSL:
        value->rValue = model->B1ugsL; 
        break;
    case BSIM1_MOD_UGSW:
        value->rValue = model->B1ugsW; 
        break;
    case BSIM1_MOD_UGSB0:
        value->rValue = model->B1ugsB0; 
        break;
    case BSIM1_MOD_UGSBL:
        value->rValue = model->B1ugsBL; 
        break;
    case BSIM1_MOD_UGSBW:
        value->rValue = model->B1ugsBW; 
        break;
    case BSIM1_MOD_UDS0:
        value->rValue = model->B1uds0; 
        break;
    case BSIM1_MOD_UDSL:
        value->rValue = model->B1udsL; 
        break;
    case BSIM1_MOD_UDSW:
        value->rValue = model->B1udsW; 
        break;
    case BSIM1_MOD_UDSB0:
        value->rValue = model->B1udsB0; 
        break;
    case BSIM1_MOD_UDSBL:
        value->rValue = model->B1udsBL; 
        break;
    case BSIM1_MOD_UDSBW:
        value->rValue = model->B1udsBW; 
        break;
    case BSIM1_MOD_UDSD0:
        value->rValue = model->B1udsD0; 
        break;
    case BSIM1_MOD_UDSDL:
        value->rValue = model->B1udsDL; 
        break;
    case BSIM1_MOD_UDSDW:
        value->rValue = model->B1udsDW; 
        break;
    case BSIM1_MOD_N00:
        value->rValue = model->B1subthSlope0; 
        break;
    case BSIM1_MOD_N0L:
        value->rValue = model->B1subthSlopeL; 
        break;
    case BSIM1_MOD_N0W:
        value->rValue = model->B1subthSlopeW; 
        break;
    case BSIM1_MOD_NB0:
        value->rValue = model->B1subthSlopeB0; 
        break;
    case BSIM1_MOD_NBL:
        value->rValue = model->B1subthSlopeBL; 
        break;
    case BSIM1_MOD_NBW:
        value->rValue = model->B1subthSlopeBW; 
        break;
    case BSIM1_MOD_ND0:
        value->rValue = model->B1subthSlopeD0; 
        break;
    case BSIM1_MOD_NDL:
        value->rValue = model->B1subthSlopeDL; 
        break;
    case BSIM1_MOD_NDW:
        value->rValue = model->B1subthSlopeDW; 
        break;
    case BSIM1_MOD_TOX:
        value->rValue = model->B1oxideThickness; 
        break;
    case BSIM1_MOD_TEMP:
        value->rValue = model->B1temp; 
        break;
    case BSIM1_MOD_VDD:
        value->rValue = model->B1vdd; 
        break;
    case BSIM1_MOD_CGSO:
        value->rValue = model->B1gateSourceOverlapCap; 
        break;
    case BSIM1_MOD_CGDO:
        value->rValue = model->B1gateDrainOverlapCap; 
        break;
    case BSIM1_MOD_CGBO:
        value->rValue = model->B1gateBulkOverlapCap; 
        break;
    case BSIM1_MOD_XPART:
        value->iValue = (int)model->B1channelChargePartitionFlag; 
        data->type = IF_INTEGER;
        break;
    case BSIM1_MOD_RSH:
        value->rValue = model->B1sheetResistance; 
        break;
    case BSIM1_MOD_JS:
        value->rValue = model->B1jctSatCurDensity; 
        break;
    case BSIM1_MOD_PB:
        value->rValue = model->B1bulkJctPotential; 
        break;
    case BSIM1_MOD_MJ:
        value->rValue = model->B1bulkJctBotGradingCoeff; 
        break;
    case BSIM1_MOD_PBSW:
        value->rValue = model->B1sidewallJctPotential; 
        break;
    case BSIM1_MOD_MJSW:
        value->rValue = model->B1bulkJctSideGradingCoeff; 
        break;
    case BSIM1_MOD_CJ:
        value->rValue = model->B1unitAreaJctCap; 
        break;
    case BSIM1_MOD_CJSW:
        value->rValue = model->B1unitLengthSidewallJctCap; 
        break;
    case BSIM1_MOD_DEFWIDTH:
        value->rValue = model->B1defaultWidth; 
        break;
    case BSIM1_MOD_DELLENGTH:
        value->rValue = model->B1deltaLength; 
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}

