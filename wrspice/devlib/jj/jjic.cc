
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
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Author: 1992 Stephen R. Whiteley
****************************************************************************/

#include "jjdefs.h"


int
JJdev::getic(sGENmodel *genmod, sCKT *ckt)
{
    //
    // grab initial conditions out of rhs array.  User specified, so use
    // external nodes to get values.
    //
    for (sJJmodel *model = static_cast<sJJmodel*>(genmod); model;
            model = model->next()) {
        sJJinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (!inst->JJinitVoltGiven)
                inst->JJinitVoltage = 
                        *(ckt->CKTrhs + inst->JJposNode) - 
                        *(ckt->CKTrhs + inst->JJnegNode);
            if (!inst->JJinitPhaseGiven) {
                if (inst->JJphsNode > 0)
                    inst->JJinitPhase = *(ckt->CKTrhs + inst->JJphsNode);
                else
                    inst->JJinitPhase = 0;
            }
            if (inst->JJcontrol)
                inst->JJinitControl = *(ckt->CKTrhs + inst->JJbranch);
        }
    }

    // find initial time delta

    double vmax = 0;
    for (sJJmodel *model = static_cast<sJJmodel*>(genmod); model;
            model = model->next()) {

        double vth = model->JJvdpbak/model->JJtsaccl;
        if (model->JJictype != 0) {
            if (vmax < vth)
                vmax = vth;
            sJJinstance *inst;
            for (inst = model->inst(); inst; inst = inst->next()) {

                if (inst->JJcriti > 0) {
                    double temp = inst->JJinitVoltage;
                    if (temp < 0)
                        temp = -temp;
                    if (vmax < temp)
                        vmax = temp;
                }
            }
        }

        if (vmax > 0) {
            double delmax = 0.1*model->JJtsfact*wrsCONSTphi0/vmax;
            if (ckt->CKTinitDelta == 0.0 || ckt->CKTinitDelta > delmax)
                ckt->CKTinitDelta = delmax;
        }
    }
    return (OK);
}

