
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
JFETdev::temperature(sGENmodel *genmod, sCKT *ckt)
{
    sJFETmodel *model = static_cast<sJFETmodel*>(genmod);
    for ( ; model; model = model->next()) {

        if (!(model->JFETtnomGiven))
            model->JFETtnom = ckt->CKTcurTask->TSKnomTemp;
        double vtnom = CONSTKoverQ * model->JFETtnom;
        double fact1 = model->JFETtnom/REFTEMP;
        double kt1 = CONSTboltz * model->JFETtnom;
        double egfet1 = 1.16-(7.02e-4*model->JFETtnom*model->JFETtnom)/
                (model->JFETtnom+1108);
        double arg1 = -egfet1/(kt1+kt1)+1.1150877/(CONSTboltz*(REFTEMP+REFTEMP));
        double pbfact1 = -2*vtnom * (1.5*log(fact1)+CHARGE*arg1);
        double pbo = (model->JFETgatePotential-pbfact1)/fact1;
        double gmaold = (model->JFETgatePotential-pbo)/pbo;
        double cjfact = 1/(1+.5*(4e-4*(model->JFETtnom-REFTEMP)-gmaold));

        if (model->JFETdrainResist != 0)
            model->JFETdrainConduct = 1/model->JFETdrainResist;
        else
            model->JFETdrainConduct = 0;
        if (model->JFETsourceResist != 0)
            model->JFETsourceConduct = 1/model->JFETsourceResist;
        else
            model->JFETsourceConduct = 0;
        if (model->JFETdepletionCapCoeff >.95) {
            DVO.textOut(OUT_WARNING,
                "%s: Depletion cap. coefficient too large, limited to .95",
                model->GENmodName);
            model->JFETdepletionCapCoeff = .95;
        }

        double xfc = log(1 - model->JFETdepletionCapCoeff);
        model->JFETf2 = exp((1+.5)*xfc);
        model->JFETf3 = 1 - model->JFETdepletionCapCoeff * (1 + .5);
        // Modification for Sydney University JFET model
        model->JFETbFac = (1 - model->JFETb)
            / (model->JFETgatePotential - model->JFETthreshold);
        // end Sydney University mod

        sJFETinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (!(inst->JFETtempGiven))
                inst->JFETtemp = ckt->CKTcurTask->TSKtemp;
            double vt = inst->JFETtemp * CONSTKoverQ;
            double fact2 = inst->JFETtemp/REFTEMP;
            double ratio1 = inst->JFETtemp/model->JFETtnom -1;
            inst->JFETtSatCur = model->JFETgateSatCurrent * exp(ratio1*1.11/vt);
            inst->JFETtCGS = model->JFETcapGS * cjfact;
            inst->JFETtCGD = model->JFETcapGD * cjfact;
            double kt = CONSTboltz*inst->JFETtemp;
            double egfet = 1.16-(7.02e-4*inst->JFETtemp*inst->JFETtemp)/
                    (inst->JFETtemp+1108);
            double arg = -egfet/(kt+kt) + 1.1150877/(CONSTboltz*(REFTEMP+REFTEMP));
            double pbfact = -2 * vt * (1.5*log(fact2)+CHARGE*arg);
            inst->JFETtGatePot = fact2 * pbo + pbfact;
            double gmanew = (inst->JFETtGatePot-pbo)/pbo;
            double cjfact1 = 1+.5*(4e-4*(inst->JFETtemp-REFTEMP)-gmanew);
            inst->JFETtCGS *= cjfact1;
            inst->JFETtCGD *= cjfact1;

            inst->JFETcorDepCap = model->JFETdepletionCapCoeff *
                    inst->JFETtGatePot;
            inst->JFETf1 = inst->JFETtGatePot * (1 - exp((1-.5)*xfc))/(1-.5);
            inst->JFETvcrit = vt * log(vt/(CONSTroot2 * inst->JFETtSatCur));
        }
    }
    return (OK);
}
