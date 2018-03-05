
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
            double C = inst->JJcap;
            double phi = *(ckt->CKTstate0 + inst->JJphase);
            double crt = *(ckt->CKTstate0 + inst->JJcrti);
            double L = wrsCONSTphi0/(2*M_PI*crt*cos(phi));

            double G;
            switch (model->JJrtype) {
            case 2:
                {
                    double dv = 0.5*model->JJdelv;
                    double gam = -model->JJvg/dv;
                    if (gam > 30.0)
                        gam = 30.0;
                    if (gam < -30.0)
                        gam = -30.0;
                    double expgam = exp(gam);
                    double exngam = 1.0 / expgam;
                    double xp     = 1.0 + expgam;
                    double xn     = 1.0 + exngam;
                    G = inst->JJgn/xn + inst->JJg0/xp;
                }
                break;
            default:
                G = inst->JJg0;
                break;
            }
            if (model->JJvShuntGiven)
                G += inst->JJcriti/model->JJvShunt;

            *inst->JJposPosPtr += G;
            *inst->JJnegNegPtr += G;
            *inst->JJposNegPtr -= G;
            *inst->JJnegPosPtr -= G;
            double val = ckt->CKTomega*C - 1.0/(ckt->CKTomega*L);
            *(inst->JJposPosPtr +1) += val;
            *(inst->JJnegNegPtr +1) += val;
            *(inst->JJposNegPtr +1) -= val;
            *(inst->JJnegPosPtr +1) -= val;
            if (inst->JJcontrol && inst->JJdcrt != 0.0) {
                double temp = inst->JJdcrt*crt*sin(phi);
                *inst->JJposIbrPtr += temp;
                *inst->JJnegIbrPtr -= temp;
            }
            if (inst->JJphsNode > 0)
                *inst->JJphsPhsPtr =  1.0;
        }
    }
    return (OK);
}

