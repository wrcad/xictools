
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
 $Id: jjacct.cc,v 2.13 2016/07/27 04:41:32 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Author: 1992 Stephen R. Whiteley
****************************************************************************/

#include "jjdefs.h"


int
JJdev::accept(sCKT *ckt, sGENmodel *genmod)
{
    double vmax = 0;
    sJJmodel *model = static_cast<sJJmodel*>(genmod);
    for ( ; model; model = model->next()) {

        bool didm = false;

        sJJinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            // keep phase  >= 0 and < 2*PI
            double phi = *(ckt->CKTstate0 + inst->JJphase);
            int pint = *(int *)(ckt->CKTstate1 + inst->JJphsInt);
            if (phi >= 2*M_PI) {
                phi -= 2*M_PI;
                pint++;
            }
            else if (phi < 0) {
                phi += 2*M_PI;
                pint--;
            }
            *(ckt->CKTstate0 + inst->JJphase) = phi;
            *(int *)(ckt->CKTstate0 + inst->JJphsInt) = pint;

            // find max vj for time step
            if (model->JJictype != 0 && inst->JJcriti > 0) {
                if (!didm) {
                    didm = true;
                    if (vmax < model->JJvdpbak)
                        vmax = model->JJvdpbak;
                }
                double vj = *(ckt->CKTstate0 + inst->JJvoltage);
                if (vj < 0)
                    vj = -vj;
                if (vmax < vj)
                    vmax = vj;
            }

            if (inst->JJphsNode > 0)
                *(ckt->CKTrhsOld + inst->JJphsNode) = phi + (2*M_PI)*pint;
        }
    }
    // Set next time step limit.
    if (vmax > 0.0) {
        double delmax = ckt->CKTcurTask->TSKdphiMax*PHI0_2PI/vmax;
        if (ckt->CKTdevMaxDelta == 0.0 || delmax < ckt->CKTdevMaxDelta)
            ckt->CKTdevMaxDelta = delmax;
    }
    return (OK);
}

