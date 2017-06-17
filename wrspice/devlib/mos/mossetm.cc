
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Device Library                                                         *
 *                                                                        *
 *========================================================================*
 $Id: mossetm.cc,v 1.1 2015/07/26 01:09:13 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1989 Takayasu Sakurai
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mosdefs.h"


int
MOSdev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sMOSmodel *model = static_cast<sMOSmodel*>(genmod);
    IFvalue *value = &data->v;

    switch (param) {
    case MOS_MOD_LEVEL:
        model->MOSlevel = value->iValue;
        model->MOSlevelGiven = true;
        break;
    case MOS_MOD_TNOM:
        model->MOStnom = value->rValue+CONSTCtoK;
        model->MOStnomGiven = true;
        break;
    case MOS_MOD_VTO:
        model->MOSvt0 = value->rValue;
        model->MOSvt0Given = true;
        break;
    case MOS_MOD_KP:
        model->MOStransconductance = value->rValue;
        model->MOStransconductanceGiven = true;
        break;
    case MOS_MOD_GAMMA:
        model->MOSgamma = value->rValue;
        model->MOSgammaGiven = true;
        break;
    case MOS_MOD_PHI:
        model->MOSphi = value->rValue;
        model->MOSphiGiven = true;
        break;
    case MOS_MOD_RD:
        model->MOSdrainResistance = value->rValue;
        model->MOSdrainResistanceGiven = true;
        break;
    case MOS_MOD_RS:
        model->MOSsourceResistance = value->rValue;
        model->MOSsourceResistanceGiven = true;
        break;
    case MOS_MOD_CBD:
        model->MOScapBD = value->rValue;
        model->MOScapBDGiven = true;
        break;
    case MOS_MOD_CBS:
        model->MOScapBS = value->rValue;
        model->MOScapBSGiven = true;
        break;
    case MOS_MOD_IS:
        model->MOSjctSatCur = value->rValue;
        model->MOSjctSatCurGiven = true;
        break;
    case MOS_MOD_PB:
        model->MOSbulkJctPotential = value->rValue;
        model->MOSbulkJctPotentialGiven = true;
        break;
    case MOS_MOD_CGSO:
        model->MOSgateSourceOverlapCapFactor = value->rValue;
        model->MOSgateSourceOverlapCapFactorGiven = true;
        break;
    case MOS_MOD_CGDO:
        model->MOSgateDrainOverlapCapFactor = value->rValue;
        model->MOSgateDrainOverlapCapFactorGiven = true;
        break;
    case MOS_MOD_CGBO:
        model->MOSgateBulkOverlapCapFactor = value->rValue;
        model->MOSgateBulkOverlapCapFactorGiven = true;
        break;
    case MOS_MOD_CJ:
        model->MOSbulkCapFactor = value->rValue;
        model->MOSbulkCapFactorGiven = true;
        break;
    case MOS_MOD_MJ:
        model->MOSbulkJctBotGradingCoeff = value->rValue;
        model->MOSbulkJctBotGradingCoeffGiven = true;
        break;
    case MOS_MOD_CJSW:
        model->MOSsideWallCapFactor = value->rValue;
        model->MOSsideWallCapFactorGiven = true;
        break;
    case MOS_MOD_MJSW:
        model->MOSbulkJctSideGradingCoeff = value->rValue;
        model->MOSbulkJctSideGradingCoeffGiven = true;
        break;
    case MOS_MOD_JS:
        model->MOSjctSatCurDensity = value->rValue;
        model->MOSjctSatCurDensityGiven = true;
        break;
    case MOS_MOD_TOX:
        model->MOSoxideThickness = value->rValue;
        model->MOSoxideThicknessGiven = true;
        break;
    case MOS_MOD_LD:
        model->MOSlatDiff = value->rValue;
        model->MOSlatDiffGiven = true;
        break;
    case MOS_MOD_RSH:
        model->MOSsheetResistance = value->rValue;
        model->MOSsheetResistanceGiven = true;
        break;
    case MOS_MOD_U0:
        model->MOSsurfaceMobility = value->rValue;
        model->MOSsurfaceMobilityGiven = true;
        break;
    case MOS_MOD_FC:
        model->MOSfwdCapDepCoeff = value->rValue;
        model->MOSfwdCapDepCoeffGiven = true;
        break;
    case MOS_MOD_NSS:
        model->MOSsurfaceStateDensity = value->rValue;
        model->MOSsurfaceStateDensityGiven = true;
        break;
    case MOS_MOD_NSUB:
        model->MOSsubstrateDoping = value->rValue;
        model->MOSsubstrateDopingGiven = true;
        break;
    case MOS_MOD_TPG:
        model->MOSgateType = value->iValue;
        model->MOSgateTypeGiven = true;
        break;
    case MOS_MOD_NMOS:
        if(value->iValue) {
            model->MOStype = 1;
            model->MOStypeGiven = true;
        }
        break;
    case MOS_MOD_PMOS:
        if(value->iValue) {
            model->MOStype = -1;
            model->MOStypeGiven = true;
        }
        break;
    case MOS_MOD_KF:
        model->MOSfNcoef = value->rValue;
        model->MOSfNcoefGiven = true;
        break;
    case MOS_MOD_AF:
        model->MOSfNexp = value->rValue;
        model->MOSfNexpGiven = true;
        break;
    case MOS_MOD_LAMBDA:
        /* levels 1 and 2 */
        model->MOSlambda = value->rValue;
        model->MOSlambdaGiven = true;
        break;
    case MOS_MOD_UEXP:
        /* level 2 */
        model->MOScritFieldExp = value->rValue;
        model->MOScritFieldExpGiven = true;
        break;
    case MOS_MOD_NEFF:
        /* level 2 */
        model->MOSchannelCharge = value->rValue;
        model->MOSchannelChargeGiven = true;
        break;
    case MOS_MOD_UCRIT:
        /* level 2 */
        model->MOScritField = value->rValue;
        model->MOScritFieldGiven = true;
        break;
    case MOS_MOD_NFS:
        /* levels 2 and 3 */
        model->MOSfastSurfaceStateDensity = value->rValue;
        model->MOSfastSurfaceStateDensityGiven = true;
        break;
    case MOS_MOD_DELTA:
        /* levels 2 and 3 */
        model->MOSnarrowFactor = value->rValue;
        model->MOSnarrowFactorGiven = true;
        break;
    case MOS_MOD_VMAX:
        /* levels 2 and 3 */
        model->MOSmaxDriftVel = value->rValue;
        model->MOSmaxDriftVelGiven = true;
        break;
    case MOS_MOD_XJ:
        /* levels 2 and 3 */
        model->MOSjunctionDepth = value->rValue;
        model->MOSjunctionDepthGiven = true;
        break;
    case MOS_MOD_ETA:
        /* level 3 */
        model->MOSeta = value->rValue;
        model->MOSetaGiven = true;
        break;
    case MOS_MOD_THETA:
        /* level 3 */
        model->MOStheta = value->rValue;
        model->MOSthetaGiven = true;
        break;
    case MOS_MOD_KAPPA:
        /* level 3 */
        model->MOSkappa = value->rValue;
        model->MOSkappaGiven = true;
        break;
    case MOS_MOD_KV:
        /* level 6 */
        model->MOSkv = value->rValue;
        model->MOSkvGiven = true;
        break;
    case MOS_MOD_NV:
        /* level 6 */
        model->MOSnv = value->rValue;
        model->MOSnvGiven = true;
        break;
    case MOS_MOD_KC:
        /* level 6 */
        model->MOSkc = value->rValue;
        model->MOSkcGiven = true;
        break;
    case MOS_MOD_NC:
        /* level 6 */
        model->MOSnc = value->rValue;
        model->MOSncGiven = true;
        break;
    case MOS_MOD_GAMMA1:
        /* level 6 */
        model->MOSgamma1 = value->rValue;
        model->MOSgamma1Given = true;
        break;
    case MOS_MOD_SIGMA:
        /* level 6 */
        model->MOSsigma = value->rValue;
        model->MOSsigmaGiven = true;
        break;
    case MOS_MOD_LAMDA0:
        /* level 6 */
        model->MOSlamda0 = value->rValue;
        model->MOSlamda0Given = true;
        break;
    case MOS_MOD_LAMDA1:
        /* level 6 */
        model->MOSlamda1 = value->rValue;
        model->MOSlamda1Given = true;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}
