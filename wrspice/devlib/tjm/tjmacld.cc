
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
TJMdev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    sTJMmodel *model = static_cast<sTJMmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sTJMinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            // Load intrinsic shunt conductance.
            double G = SPMAX(inst->TJMg0, inst->TJMgqp);
            *inst->TJMposPosPtr += G;
            *inst->TJMnegNegPtr += G;
            *inst->TJMposNegPtr -= G;
            *inst->TJMnegPosPtr -= G;

            // Load the shunt resistance implied if vshunt given.
            if (model->TJMvShuntGiven && inst->TJMgshunt > 0.0) {
                G = inst->TJMgshunt;
                ckt->ldadd(inst->TJMrshPosPosPtr, G);
                ckt->ldadd(inst->TJMrshPosNegPtr, -G);
                ckt->ldadd(inst->TJMrshNegPosPtr, -G);
                ckt->ldadd(inst->TJMrshNegNegPtr, G);
            }

            double C = inst->TJMcap;
            double val = ckt->CKTomega*C;
            if (model->TJMictype > 0) {
                double phi = *(ckt->CKTstate0 + inst->TJMphase);
                double crt = *(ckt->CKTstate0 + inst->TJMcrti);
                double Lrecip = (2*M_PI*crt*cos(phi))/wrsCONSTphi0;
                val -= Lrecip/ckt->CKTomega;
            }
            *(inst->TJMposPosPtr +1) += val;
            *(inst->TJMnegNegPtr +1) += val;
            *(inst->TJMposNegPtr +1) -= val;
            *(inst->TJMnegPosPtr +1) -= val;
            if (inst->TJMphsNode > 0)
                *inst->TJMphsPhsPtr = 1.0;

#ifdef NEWLSER
            if (inst->TJMlser > 0.0) {
                val = ckt->CKTomega * inst->TJMlser;
                if (inst->TJMrealPosNode) {
                    ckt->ldset(inst->TJMlserPosIbrPtr, 1.0);
                    ckt->ldset(inst->TJMlserIbrPosPtr, 1.0);
                }
                if (inst->TJMposNode) {
                    ckt->ldset(inst->TJMlserNegIbrPtr, -1.0);
                    ckt->ldset(inst->TJMlserIbrNegPtr, -1.0);
                }
                *(inst->TJMlserIbrIbrPtr +1) -= val;
            }
#endif
#ifdef NEWLSH
            if (inst->TJMlsh > 0.0) {
                val = ckt->CKTomega * inst->TJMlsh;
                ckt->ldset(inst->TJMlshPosIbrPtr, 1.0);
                ckt->ldset(inst->TJMlshIbrPosPtr, 1.0);
                ckt->ldset(inst->TJMlshNegIbrPtr, -1.0);
                ckt->ldset(inst->TJMlshIbrNegPtr, -1.0);
                *(inst->TJMlshIbrIbrPtr +1) -= val;
            }
#endif
        }
    }
    return (OK);
}

