
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
 $Id: bjttemp.cc,v 1.2 2011/12/18 01:15:15 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "bjtdefs.h"


int
BJTdev::temperature(sGENmodel *genmod, sCKT *ckt)
{
    sBJTmodel *model = static_cast<sBJTmodel*>(genmod);
    for ( ; model; model = model->next()) {

        if (!model->BJTtnomGiven)
            model->BJTtnom = ckt->CKTcurTask->TSKnomTemp;
        double fact1 = model->BJTtnom/REFTEMP;

        if (!model->BJTleakBEcurrentGiven)
            model->BJTleakBEcurrent = 0;
        if (!model->BJTleakBCcurrentGiven)
            model->BJTleakBCcurrent = 0;
        if (!model->BJTminBaseResistGiven)
            model->BJTminBaseResist = model->BJTbaseResist;

//
// COMPATABILITY WARNING!
// special note:  for backward compatability to much older models, spice 2G
// implemented a special case which checked if B-E leakage saturation
// current was >1, then it was instead a the B-E leakage saturation current
// divided by IS, and multiplied it by IS at this point.  This was not
// handled correctly in the 2G code, and there is some question on its 
// reasonability, since it is also undocumented, so it has been left out
// here.  It could easily be added with 1 line.  (The same applies to the B-C
// leakage saturation current).   TQ  6/29/84
//
            
        if (model->BJTearlyVoltFGiven && model->BJTearlyVoltF != 0)
            model->BJTinvEarlyVoltF = 1/model->BJTearlyVoltF;
        else
            model->BJTinvEarlyVoltF = 0;

        if (model->BJTrollOffFGiven && model->BJTrollOffF != 0)
            model->BJTinvRollOffF = 1/model->BJTrollOffF;
        else
            model->BJTinvRollOffF = 0;

        if (model->BJTearlyVoltRGiven && model->BJTearlyVoltR != 0)
            model->BJTinvEarlyVoltR = 1/model->BJTearlyVoltR;
        else
            model->BJTinvEarlyVoltR = 0;

        if (model->BJTrollOffRGiven && model->BJTrollOffR != 0)
            model->BJTinvRollOffR = 1/model->BJTrollOffR;
        else
            model->BJTinvRollOffR = 0;

        if (model->BJTcollectorResistGiven && model->BJTcollectorResist != 0)
            model->BJTcollectorConduct = 1/model->BJTcollectorResist;
        else
            model->BJTcollectorConduct = 0;

        if (model->BJTemitterResistGiven && model->BJTemitterResist != 0)
            model->BJTemitterConduct = 1/model->BJTemitterResist;
        else
            model->BJTemitterConduct = 0;

        if (model->BJTtransitTimeFVBCGiven && model->BJTtransitTimeFVBC != 0)
            model->BJTtransitTimeVBCFactor =1/ (model->BJTtransitTimeFVBC*1.44);
        else
            model->BJTtransitTimeVBCFactor = 0;

        model->BJTexcessPhaseFactor =
            (model->BJTexcessPhase/(180.0/M_PI)) * model->BJTtransitTimeF;
        if (model->BJTdepletionCapCoeffGiven) {
            if (model->BJTdepletionCapCoeff > .9999)  {
                model->BJTdepletionCapCoeff = .9999;
                DVO.textOut(OUT_WARNING,
                    "BJT model %s, parameter fc limited to 0.9999",
                    model->GENmodName);
            }
        }
        else
            model->BJTdepletionCapCoeff=.5;

        double xfc = log(1-model->BJTdepletionCapCoeff);
        model->BJTf2 = exp((1 + model->BJTjunctionExpBE) * xfc);
        model->BJTf3 = 1 - model->BJTdepletionCapCoeff * 
                (1 + model->BJTjunctionExpBE);
        model->BJTf6 = exp((1+model->BJTjunctionExpBC)*xfc);
        model->BJTf7 = 1 - model->BJTdepletionCapCoeff * 
                (1 + model->BJTjunctionExpBC);

        sBJTinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (!inst->BJTtempGiven) inst->BJTtemp = ckt->CKTcurTask->TSKtemp;
            double vt = inst->BJTtemp * CONSTKoverQ;
            double fact2 = inst->BJTtemp/REFTEMP;
            double egfet = 1.16-(7.02e-4*inst->BJTtemp*inst->BJTtemp)/
                    (inst->BJTtemp+1108);
            double arg = -egfet/(2*CONSTboltz*inst->BJTtemp)+
                    1.1150877/(CONSTboltz*(REFTEMP+REFTEMP));
            double pbfact = -2*vt*(1.5*log(fact2)+CHARGE*arg);

            double ratlog = log(inst->BJTtemp/model->BJTtnom);
            double ratio1 = inst->BJTtemp/model->BJTtnom -1;
            double factlog = ratio1 * model->BJTenergyGap/vt + 
                    model->BJTtempExpIS*ratlog;
            double factor = exp(factlog);
            inst->BJTtSatCur = model->BJTsatCur * factor;
            double bfactor = exp(ratlog*model->BJTbetaExp);
            inst->BJTtBetaF = model->BJTbetaF * bfactor;
            inst->BJTtBetaR = model->BJTbetaR * bfactor;
            inst->BJTtBEleakCur = model->BJTleakBEcurrent * 
                    exp(factlog/model->BJTleakBEemissionCoeff)/bfactor;
            inst->BJTtBCleakCur = model->BJTleakBCcurrent * 
                    exp(factlog/model->BJTleakBCemissionCoeff)/bfactor;

            double pbo = (model->BJTpotentialBE-pbfact)/fact1;
            double gmaold = (model->BJTpotentialBE-pbo)/pbo;
            inst->BJTtBEcap = model->BJTdepletionCapBE/
                    (1+model->BJTjunctionExpBE*
                    (4e-4*(model->BJTtnom-REFTEMP)-gmaold));
            inst->BJTtBEpot = fact2 * pbo+pbfact;
            double gmanew = (inst->BJTtBEpot-pbo)/pbo;
            inst->BJTtBEcap *= 1+model->BJTjunctionExpBE*
                    (4e-4*(inst->BJTtemp-REFTEMP)-gmanew);

            pbo = (model->BJTpotentialBC-pbfact)/fact1;
            gmaold = (model->BJTpotentialBC-pbo)/pbo;
            inst->BJTtBCcap = model->BJTdepletionCapBC/
                    (1+model->BJTjunctionExpBC*
                    (4e-4*(model->BJTtnom-REFTEMP)-gmaold));
            inst->BJTtBCpot = fact2 * pbo+pbfact;
            gmanew = (inst->BJTtBCpot-pbo)/pbo;
            inst->BJTtBCcap *= 1+model->BJTjunctionExpBC*
                    (4e-4*(inst->BJTtemp-REFTEMP)-gmanew);

            inst->BJTtDepCap = model->BJTdepletionCapCoeff * inst->BJTtBEpot;
            inst->BJTtf1 = inst->BJTtBEpot * (1 - exp((1 - 
                    model->BJTjunctionExpBE) * xfc)) / 
                    (1 - model->BJTjunctionExpBE);
            inst->BJTtf4 = model->BJTdepletionCapCoeff * inst->BJTtBCpot;
            inst->BJTtf5 = inst->BJTtBCpot * (1 - exp((1 -
                    model->BJTjunctionExpBC) * xfc)) /
                    (1 - model->BJTjunctionExpBC);
            inst->BJTtVcrit = vt * log(vt / (CONSTroot2*model->BJTsatCur));
        }
    }
    return (OK);
}
