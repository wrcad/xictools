
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
 $Id: bjtsetm.cc,v 1.1 2015/07/26 01:09:09 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "bjtdefs.h"


// This routine sets model parameters for
// BJTs in the circuit.
//
int
BJTdev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sBJTmodel *model = static_cast<sBJTmodel*>(genmod);
    IFvalue *value = &data->v;

    switch (param) {
    case BJT_MOD_NPN:
        if (value->iValue)
            model->BJTtype = NPN;
        break;
    case BJT_MOD_PNP:
        if (value->iValue)
            model->BJTtype = PNP;
        break;
    case BJT_MOD_IS:
        model->BJTsatCur = value->rValue;
        model->BJTsatCurGiven = true;
        break;
    case BJT_MOD_BF:
        model->BJTbetaF = value->rValue;
        model->BJTbetaFGiven = true;
        break;
    case BJT_MOD_NF:
        model->BJTemissionCoeffF = value->rValue;
        model->BJTemissionCoeffFGiven = true;
        break;
    case BJT_MOD_VAF:
        model->BJTearlyVoltF = value->rValue;
        model->BJTearlyVoltFGiven = true;
        break;
    case BJT_MOD_IKF:
        model->BJTrollOffF = value->rValue;
        model->BJTrollOffFGiven = true;
        break;
    case BJT_MOD_ISE:
        model->BJTleakBEcurrent = value->rValue;
        model->BJTleakBEcurrentGiven = true;
        break;
    case BJT_MOD_NE:
        model->BJTleakBEemissionCoeff = value->rValue;
        model->BJTleakBEemissionCoeffGiven = true;
        break;
    case BJT_MOD_BR:
        model->BJTbetaR = value->rValue;
        model->BJTbetaRGiven = true;
        break;
    case BJT_MOD_NR:
        model->BJTemissionCoeffR = value->rValue;
        model->BJTemissionCoeffRGiven = true;
        break;
    case BJT_MOD_VAR:
        model->BJTearlyVoltR = value->rValue;
        model->BJTearlyVoltRGiven = true;
        break;
    case BJT_MOD_IKR:
        model->BJTrollOffR = value->rValue;
        model->BJTrollOffRGiven = true;
        break;
    case BJT_MOD_ISC:
        model->BJTleakBCcurrent = value->rValue;
        model->BJTleakBCcurrentGiven = true;
        break;
    case BJT_MOD_NC:
        model->BJTleakBCemissionCoeff = value->rValue;
        model->BJTleakBCemissionCoeffGiven = true;
        break;
    case BJT_MOD_RB:
        model->BJTbaseResist = value->rValue;
        model->BJTbaseResistGiven = true;
        break;
    case BJT_MOD_IRB:
        model->BJTbaseCurrentHalfResist = value->rValue;
        model->BJTbaseCurrentHalfResistGiven = true;
        break;
    case BJT_MOD_RBM:
        model->BJTminBaseResist = value->rValue;
        model->BJTminBaseResistGiven = true;
        break;
    case BJT_MOD_RE:
        model->BJTemitterResist = value->rValue;
        model->BJTemitterResistGiven = true;
        break;
    case BJT_MOD_RC:
        model->BJTcollectorResist = value->rValue;
        model->BJTcollectorResistGiven = true;
        break;
    case BJT_MOD_CJE:
        model->BJTdepletionCapBE = value->rValue;
        model->BJTdepletionCapBEGiven = true;
        break;
    case BJT_MOD_VJE:
        model->BJTpotentialBE = value->rValue;
        model->BJTpotentialBEGiven = true;
        break;
    case BJT_MOD_MJE:
        model->BJTjunctionExpBE = value->rValue;
        model->BJTjunctionExpBEGiven = true;
        break;
    case BJT_MOD_TF:
        model->BJTtransitTimeF = value->rValue;
        model->BJTtransitTimeFGiven = true;
        break;
    case BJT_MOD_XTF:
        model->BJTtransitTimeBiasCoeffF = value->rValue;
        model->BJTtransitTimeBiasCoeffFGiven = true;
        break;
    case BJT_MOD_VTF:
        model->BJTtransitTimeFVBC = value->rValue;
        model->BJTtransitTimeFVBCGiven = true;
        break;
    case BJT_MOD_ITF:
        model->BJTtransitTimeHighCurrentF = value->rValue;
        model->BJTtransitTimeHighCurrentFGiven = true;
        break;
    case BJT_MOD_PTF:
        model->BJTexcessPhase = value->rValue;
        model->BJTexcessPhaseGiven = true;
        break;
    case BJT_MOD_CJC:
        model->BJTdepletionCapBC = value->rValue;
        model->BJTdepletionCapBCGiven = true;
        break;
    case BJT_MOD_VJC:
        model->BJTpotentialBC = value->rValue;
        model->BJTpotentialBCGiven = true;
        break;
    case BJT_MOD_MJC:
        model->BJTjunctionExpBC = value->rValue;
        model->BJTjunctionExpBCGiven = true;
        break;
    case BJT_MOD_XCJC:
        model->BJTbaseFractionBCcap = value->rValue;
        model->BJTbaseFractionBCcapGiven = true;
        break;
    case BJT_MOD_TR:
        model->BJTtransitTimeR = value->rValue;
        model->BJTtransitTimeRGiven = true;
        break;
    case BJT_MOD_CJS:
        model->BJTcapCS = value->rValue;
        model->BJTcapCSGiven = true;
        break;
    case BJT_MOD_VJS:
        model->BJTpotentialSubstrate = value->rValue;
        model->BJTpotentialSubstrateGiven = true;
        break;
    case BJT_MOD_MJS:
        model->BJTexponentialSubstrate = value->rValue;
        model->BJTexponentialSubstrateGiven = true;
        break;
    case BJT_MOD_XTB:
        model->BJTbetaExp = value->rValue;
        model->BJTbetaExpGiven = true;
        break;
    case BJT_MOD_EG:
        model->BJTenergyGap = value->rValue;
        model->BJTenergyGapGiven = true;
        break;
    case BJT_MOD_XTI:
        model->BJTtempExpIS = value->rValue;
        model->BJTtempExpISGiven = true;
        break;
    case BJT_MOD_FC:
        model->BJTdepletionCapCoeff = value->rValue;
        model->BJTdepletionCapCoeffGiven = true;
        break;
    case BJT_MOD_TNOM:
        model->BJTtnom = value->rValue+CONSTCtoK;
        model->BJTtnomGiven = true;
        break;
    case BJT_MOD_KF:
        model->BJTfNcoef = value->rValue;
        model->BJTfNcoefGiven = true;
        break;
    case BJT_MOD_AF:
        model->BJTfNexp = value->rValue;
        model->BJTfNexpGiven = true;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}
