
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
Authors: 1987 Mathew Lew and Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "bjtdefs.h"


int
BJTdev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sBJTmodel *model = static_cast<const sBJTmodel*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case BJT_MOD_IS:
        value->rValue = model->BJTsatCur;
        break;
    case BJT_MOD_BF:
        value->rValue = model->BJTbetaF;
        break;
    case BJT_MOD_NF:
        value->rValue = model->BJTemissionCoeffF;
        break;
    case BJT_MOD_VAF:
        value->rValue = model->BJTearlyVoltF;
        break;
    case BJT_MOD_IKF:
        value->rValue = model->BJTrollOffF;
        break;
    case BJT_MOD_ISE:
        value->rValue = model->BJTleakBEcurrent;
        break;
    case BJT_MOD_NE:
        value->rValue = model->BJTleakBEemissionCoeff;
        break;
    case BJT_MOD_BR:
        value->rValue = model->BJTbetaR;
        break;
    case BJT_MOD_NR:
        value->rValue = model->BJTemissionCoeffR;
        break;
    case BJT_MOD_VAR:
        value->rValue = model->BJTearlyVoltR;
        break;
    case BJT_MOD_IKR:
        value->rValue = model->BJTrollOffR;
        break;
    case BJT_MOD_ISC:
        value->rValue = model->BJTleakBCcurrent;
        break;
    case BJT_MOD_NC:
        value->rValue = model->BJTleakBCemissionCoeff;
        break;
    case BJT_MOD_RB:
        value->rValue = model->BJTbaseResist;
        break;
    case BJT_MOD_IRB:
        value->rValue = model->BJTbaseCurrentHalfResist;
        break;
    case BJT_MOD_RBM:
        value->rValue = model->BJTminBaseResist;
        break;
    case BJT_MOD_RE:
        value->rValue = model->BJTemitterResist;
        break;
    case BJT_MOD_RC:
        value->rValue = model->BJTcollectorResist;
        break;
    case BJT_MOD_CJE:
        value->rValue = model->BJTdepletionCapBE;
        break;
    case BJT_MOD_VJE:
        value->rValue = model->BJTpotentialBE;
        break;
    case BJT_MOD_MJE:
        value->rValue = model->BJTjunctionExpBE;
        break;
    case BJT_MOD_TF:
        value->rValue = model->BJTtransitTimeF;
        break;
    case BJT_MOD_XTF:
        value->rValue = model->BJTtransitTimeBiasCoeffF;
        break;
    case BJT_MOD_VTF:
        value->rValue = model->BJTtransitTimeFVBC;
        break;
    case BJT_MOD_ITF:
        value->rValue = model->BJTtransitTimeHighCurrentF;
        break;
    case BJT_MOD_PTF:
        value->rValue = model->BJTexcessPhase;
        break;
    case BJT_MOD_CJC:
        value->rValue = model->BJTdepletionCapBC;
        break;
    case BJT_MOD_VJC:
        value->rValue = model->BJTpotentialBC;
        break;
    case BJT_MOD_MJC:
        value->rValue = model->BJTjunctionExpBC;
        break;
    case BJT_MOD_XCJC:
        value->rValue = model->BJTbaseFractionBCcap;
        break;
    case BJT_MOD_TR:
        value->rValue = model->BJTtransitTimeR;
        break;
    case BJT_MOD_CJS:
        value->rValue = model->BJTcapCS;
        break;
    case BJT_MOD_VJS:
        value->rValue = model->BJTpotentialSubstrate;
        break;
    case BJT_MOD_MJS:
        value->rValue = model->BJTexponentialSubstrate;
        break;
    case BJT_MOD_XTB:
        value->rValue = model->BJTbetaExp;
        break;
    case BJT_MOD_EG:
        value->rValue = model->BJTenergyGap;
        break;
    case BJT_MOD_XTI:
        value->rValue = model->BJTtempExpIS;
        break;
    case BJT_MOD_FC:
        value->rValue = model->BJTdepletionCapCoeff;
        break;
    case BJT_MOD_TNOM:
        value->rValue = model->BJTtnom-CONSTCtoK;
        break;
    case BJT_MOD_INVEARLYF:
        value->rValue = model->BJTinvEarlyVoltF;
        break;
    case BJT_MOD_INVEARLYR:
        value->rValue = model->BJTinvEarlyVoltR;
        break;
    case BJT_MOD_INVROLLOFFF:
        value->rValue = model->BJTinvRollOffF;
        break;
    case BJT_MOD_INVROLLOFFR:
        value->rValue = model->BJTinvRollOffR;
        break;
    case BJT_MOD_COLCONDUCT:
        value->rValue = model->BJTcollectorConduct;
        break;
    case BJT_MOD_EMITTERCONDUCT:
        value->rValue = model->BJTemitterConduct;
        break;
    case BJT_MOD_TRANSVBCFACT:
        value->rValue = model->BJTtransitTimeVBCFactor;
        break;
    case BJT_MOD_EXCESSPHASEFACTOR:
        value->rValue = model->BJTexcessPhaseFactor;
        break;
    case BJT_MOD_TYPE:
        if (model->BJTtype == NPN)
            value->sValue = "npn";
        else
            value->sValue = "pnp";
        data->type = IF_STRING;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}

