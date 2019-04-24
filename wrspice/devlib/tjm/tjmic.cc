
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2019 Whiteley Research Inc., all rights reserved.       *
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

#include "tjmdefs.h"


int
TJMdev::getic(sGENmodel *genmod, sCKT *ckt)
{
    //
    // grab initial conditions out of rhs array.  User specified, so use
    // external nodes to get values.
    //
    for (sTJMmodel *model = static_cast<sTJMmodel*>(genmod); model;
            model = model->next()) {
        sTJMinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (!inst->TJMinitVoltGiven)
                inst->TJMinitVoltage = 
                        *(ckt->CKTrhs + inst->TJMposNode) - 
                        *(ckt->CKTrhs + inst->TJMnegNode);
            if (!inst->TJMinitPhaseGiven) {
                if (inst->TJMphsNode > 0)
                    inst->TJMinitPhase = *(ckt->CKTrhs + inst->TJMphsNode);
                else
                    inst->TJMinitPhase = 0;
            }
            if (inst->TJMcontrol)
                inst->TJMinitControl = *(ckt->CKTrhs + inst->TJMbranch);
        }
    }

    // find initial time delta

    double vmax = 0;
    for (sTJMmodel *model = static_cast<sTJMmodel*>(genmod); model;
            model = model->next()) {

        if (model->TJMictype != 0) {
            if (vmax < model->TJMvdpbak)
                vmax = model->TJMvdpbak;
            sTJMinstance *inst;
            for (inst = model->inst(); inst; inst = inst->next()) {

                if (inst->TJMcriti > 0) {
                    double temp = inst->TJMinitVoltage;
                    if (temp < 0)
                        temp = -temp;
                    if (vmax < temp) vmax = temp;
                }
            }
        }
    }

    if (vmax > 0) {
        double temp = .1*ckt->CKTcurTask->TSKdphiMax*PHI0_2PI/vmax;
        if (ckt->CKTinitDelta < temp)
            ckt->CKTinitDelta = temp;
    }
    return (OK);
}

