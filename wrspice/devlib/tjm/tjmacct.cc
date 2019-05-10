
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
TJMdev::accept(sCKT *ckt, sGENmodel *genmod)
{
    sTJMmodel *model = static_cast<sTJMmodel*>(genmod);
    for ( ; model; model = model->next()) {

        bool didm = false;
        double vmax = 0;

        sTJMinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            // Due to the half-angle formulas used in the TMJ, we
            // can't do modulo 2pi, but instead we can use modulo 4pi. 
            // As in the RSJ case, this avoid numerical phase-slip
            // errors for very long runs.

            double phi = *(ckt->CKTstate0 + inst->TJMphase);
            int pint = *(int *)(ckt->CKTstate1 + inst->TJMphsInt);
            double fourpi = 4.0*M_PI;
            if (phi >= fourpi) {
                phi -= fourpi;
                pint++;
            }
            else if (phi < 0) {
                phi += fourpi;
                pint--;
            }

            *(ckt->CKTstate0 + inst->TJMphase) = phi;
            *(int *)(ckt->CKTstate0 + inst->TJMphsInt) = pint;
            if (inst->TJMphsNode > 0)
                *(ckt->CKTrhsOld + inst->TJMphsNode) = phi + fourpi*pint;
            inst->tjm_accept(phi);

            // SFQ hooks.
            pint += pint;
            double twopi = 2.0*M_PI;
            if (phi >= twopi) {
                pint++;
                phi -= twopi;
            }
            // Assume that 5*pi/4 is the half-way point when producing
            // an SFQ pulse.
            if (phi > 1.25*M_PI)
                pint++;
            int last_pn = inst->TJMphsN;
            inst->TJMphsN = pint;
            inst->TJMphsF = false;
            if (pint != last_pn) {
                // Pulse count changed, record time and set flag.
//XXX interpolate instead
                inst->TJMphsT = ckt->CKTtime;
                inst->TJMphsF = true;
            }

            // find max vj for time step
            if (model->TJMictype != 0 && inst->TJMcriti > 0) {
                if (!didm) {
                    didm = true;
                    if (vmax < model->TJMvdpbak)
                        vmax = model->TJMvdpbak;
                }
                double vj = *(ckt->CKTstate0 + inst->TJMvoltage);
                if (vj < 0)
                    vj = -vj;
                if (vmax < vj)
                    vmax = vj;
            }
        }
        if (vmax > 0.0) {
            // Limit next time step.
            double delmax = M_PI*model->TJMtsfact*PHI0_2PI/vmax;
            if (ckt->CKTdevMaxDelta == 0.0 || delmax < ckt->CKTdevMaxDelta)
                ckt->CKTdevMaxDelta = delmax;
        }
    }
    return (OK);
}

