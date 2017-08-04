
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
Authors: 1985 S. Hwang
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mesdefs.h"


int
MESdev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    sMESmodel *model = static_cast<sMESmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sMESinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            double gdpr = model->MESdrainConduct * inst->MESarea;
            double gspr = model->MESsourceConduct * inst->MESarea;
            double gm   = *(ckt->CKTstate0 + inst->MESgm);
            double gds  = *(ckt->CKTstate0 + inst->MESgds);
            double ggs  = *(ckt->CKTstate0 + inst->MESggs);
            double xgs  = *(ckt->CKTstate0 + inst->MESqgs) * ckt->CKTomega;
            double ggd  = *(ckt->CKTstate0 + inst->MESggd);
            double xgd  = *(ckt->CKTstate0 + inst->MESqgd) * ckt->CKTomega;
            *(inst->MESdrainDrainPtr) += gdpr;
            *(inst->MESgateGatePtr) += ggd+ggs;
            *(inst->MESgateGatePtr +1) += xgd+xgs;
            *(inst->MESsourceSourcePtr) += gspr;
            *(inst->MESdrainPrimeDrainPrimePtr) += gdpr+gds+ggd;
            *(inst->MESdrainPrimeDrainPrimePtr +1) += xgd;
            *(inst->MESsourcePrimeSourcePrimePtr) += gspr+gds+gm+ggs;
            *(inst->MESsourcePrimeSourcePrimePtr +1) += xgs;
            *(inst->MESdrainDrainPrimePtr) -= gdpr;
            *(inst->MESgateDrainPrimePtr) -= ggd;
            *(inst->MESgateDrainPrimePtr +1) -= xgd;
            *(inst->MESgateSourcePrimePtr) -= ggs;
            *(inst->MESgateSourcePrimePtr +1) -= xgs;
            *(inst->MESsourceSourcePrimePtr) -= gspr;
            *(inst->MESdrainPrimeDrainPtr) -= gdpr;
            *(inst->MESdrainPrimeGatePtr) += (-ggd+gm);
            *(inst->MESdrainPrimeGatePtr +1) -= xgd;
            *(inst->MESdrainPrimeSourcePrimePtr) += (-gds-gm);
            *(inst->MESsourcePrimeGatePtr) += (-ggs-gm);
            *(inst->MESsourcePrimeGatePtr +1) -= xgs;
            *(inst->MESsourcePrimeSourcePtr) -= gspr;
            *(inst->MESsourcePrimeDrainPrimePtr) -= gds;
        }
    }
    return (OK);
}
