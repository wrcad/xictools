
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

#include "bjtdefs.h"


// Function to load the COMPLEX circuit matrix using the
// small signal parameters saved during a previous DC operating
// point analysis.
//
int
BJTdev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    sBJTmodel *model = static_cast<sBJTmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sBJTinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            
            double gcpr = model->BJTcollectorConduct * inst->BJTarea;
            double gepr = model->BJTemitterConduct * inst->BJTarea;
            double gpi  = inst->BJTgpi;
            double gmu  = inst->BJTgmu;
            double gm   = inst->BJTgm;
            double go   = inst->BJTgo;
            double xgm  = 0;
            double td   = model->BJTexcessPhaseFactor;
            if (td != 0) {
                double arg = td*ckt->CKTomega;
                gm += go;
                xgm = -gm*sin(arg);
                gm  = gm*cos(arg) - go;
            }
            double gx   = inst->BJTgx;
            double xcpi = inst->BJTcapbe * ckt->CKTomega;
            double xcmu = inst->BJTcapbc * ckt->CKTomega;
            double xcbx = inst->BJTcapbx * ckt->CKTomega;
            double xccs = inst->BJTcapcs * ckt->CKTomega;
            double xcmcb= inst->BJTgeqcb * ckt->CKTomega;

            *(inst->BJTcolColPtr)                 += gcpr;
            *(inst->BJTbaseBasePtr)               += gx;
            *(inst->BJTbaseBasePtr + 1)           += xcbx;
            *(inst->BJTemitEmitPtr)               += gepr;
            *(inst->BJTcolPrimeColPrimePtr)       += gmu + go + gcpr;
            *(inst->BJTcolPrimeColPrimePtr + 1)   += xcmu + xccs + xcbx;
            *(inst->BJTbasePrimeBasePrimePtr)     += gx + gpi + gmu;
            *(inst->BJTbasePrimeBasePrimePtr + 1) += xcpi + xcmu + xcmcb;
            *(inst->BJTemitPrimeEmitPrimePtr)     += gpi + gepr + gm + go;
            *(inst->BJTemitPrimeEmitPrimePtr + 1) += xcpi + xgm;
            *(inst->BJTcolColPrimePtr)            -= gcpr;
            *(inst->BJTbaseBasePrimePtr)          -= gx;
            *(inst->BJTemitEmitPrimePtr)          -= gepr;
            *(inst->BJTcolPrimeColPtr)            -= gcpr;
            *(inst->BJTcolPrimeBasePrimePtr)      += -gmu + gm;
            *(inst->BJTcolPrimeBasePrimePtr + 1)  += -xcmu + xgm;
            *(inst->BJTcolPrimeEmitPrimePtr)      -= gm + go;
            *(inst->BJTcolPrimeEmitPrimePtr + 1)  -= xgm;
            *(inst->BJTbasePrimeBasePtr)          -= gx;
            *(inst->BJTbasePrimeColPrimePtr)      -= gmu;
            *(inst->BJTbasePrimeColPrimePtr + 1)  -= xcmu + xcmcb;
            *(inst->BJTbasePrimeEmitPrimePtr)     -= gpi;
            *(inst->BJTbasePrimeEmitPrimePtr + 1) -= xcpi;
            *(inst->BJTemitPrimeEmitPtr)          -= gepr;
            *(inst->BJTemitPrimeColPrimePtr)      -= go;
            *(inst->BJTemitPrimeColPrimePtr + 1)  += xcmcb;
            *(inst->BJTemitPrimeBasePrimePtr)     -= gpi + gm;
            *(inst->BJTemitPrimeBasePrimePtr + 1) -= xcpi + xgm + xcmcb;
            *(inst->BJTsubstSubstPtr + 1)         += xccs;
            *(inst->BJTcolPrimeSubstPtr + 1)      -= xccs;
            *(inst->BJTsubstColPrimePtr + 1)      -= xccs;
            *(inst->BJTbaseColPrimePtr + 1)       -= xcbx;
            *(inst->BJTcolPrimeBasePtr + 1)       -= xcbx;
        }
    }
    return (OK);
}
