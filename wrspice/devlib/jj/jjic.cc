
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
 $Id: jjic.cc,v 2.12 2016/09/26 01:48:17 stevew Exp $
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

        if (model->JJictype != 0) {
            if (vmax < model->JJvdpbak)
                vmax = model->JJvdpbak;
            sJJinstance *inst;
            for (inst = model->inst(); inst; inst = inst->next()) {

                if (inst->JJcriti > 0) {
                    double temp = inst->JJinitVoltage;
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

