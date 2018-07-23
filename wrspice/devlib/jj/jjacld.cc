
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

#include "jjdefs.h"


int
JJdev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    sJJmodel *model = static_cast<sJJmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sJJinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

#ifdef NEWLSER
            double G = inst->JJgqp;
            *inst->JJposPosPtr += G;
            *inst->JJnegNegPtr += G;
            *inst->JJposNegPtr -= G;
            *inst->JJnegPosPtr -= G;

            // Load the shunt resistance implied if vshunt given.
            if (model->JJvShuntGiven && inst->JJgshunt > 0.0) {
                G = inst->JJgshunt;
                ckt->ldadd(inst->JJrealPosRealPosPtr, G);
                ckt->ldadd(inst->JJrealPosNegPtr, -G);
                ckt->ldadd(inst->JJnegRealPosPtr, -G);
                ckt->ldadd(inst->JJnegNegPtr, G);
            }
#else
            double G = inst->JJgqp;
            if (model->JJvShuntGiven && inst->JJgshunt > 0.0)
                G += inst->gshunt;
            *inst->JJposPosPtr += G;
            *inst->JJnegNegPtr += G;
            *inst->JJposNegPtr -= G;
            *inst->JJnegPosPtr -= G;
#endif

            double C = inst->JJcap;
            double val = ckt->CKTomega*C;
            if (model->JJictype > 0) {
                double phi = *(ckt->CKTstate0 + inst->JJphase);
                double crt = *(ckt->CKTstate0 + inst->JJcrti);
                double Lrecip = (2*M_PI*crt*cos(phi))/wrsCONSTphi0;
                val -= Lrecip/ckt->CKTomega;
                if (inst->JJcontrol && inst->JJdcrt != 0.0) {
                    double temp = inst->JJdcrt*crt*sin(phi);
                    *inst->JJposIbrPtr += temp;
                    *inst->JJnegIbrPtr -= temp;
                }
            }
            *(inst->JJposPosPtr +1) += val;
            *(inst->JJnegNegPtr +1) += val;
            *(inst->JJposNegPtr +1) -= val;
            *(inst->JJnegPosPtr +1) -= val;
            if (inst->JJphsNode > 0)
                *inst->JJphsPhsPtr =  1.0;

#ifdef NEWLSER
            if (inst->JJlser > 0.0) {
                val = ckt->CKTomega * inst->JJlser;
                if (inst->JJrealPosNode) {
                    ckt->ldset(inst->JJlPosIbrPtr, 1.0);
                    ckt->ldset(inst->JJlIbrPosPtr, 1.0);
                }
                if (inst->JJposNode) {
                    ckt->ldset(inst->JJlNegIbrPtr, -1.0);
                    ckt->ldset(inst->JJlIbrNegPtr, -1.0);
                }
                *(inst->JJlIbrIbrPtr +1) -= val;
            }
#endif
        }
    }
    return (OK);
}

