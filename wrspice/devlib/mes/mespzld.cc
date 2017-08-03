
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
MESdev::pzLoad(sGENmodel *genmod, sCKT *ckt, IFcomplex *s)
{
    sMESmodel *model = static_cast<sMESmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sMESinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            double gdpr = model->MESdrainConduct * inst->MESarea;
            double gspr = model->MESsourceConduct * inst->MESarea;
            double gm = *(ckt->CKTstate0 + inst->MESgm);
            double gds = *(ckt->CKTstate0 + inst->MESgds);
            double ggs = *(ckt->CKTstate0 + inst->MESggs);
            double xgs = *(ckt->CKTstate0 + inst->MESqgs);
            double ggd = *(ckt->CKTstate0 + inst->MESggd);
            double xgd = *(ckt->CKTstate0 + inst->MESqgd);
            *(inst->MESdrainDrainPtr ) += gdpr;
            *(inst->MESgateGatePtr ) += ggd+ggs;
            *(inst->MESgateGatePtr   ) += (xgd+xgs)*s->real;
            *(inst->MESgateGatePtr +1) += (xgd+xgs)*s->imag;
            *(inst->MESsourceSourcePtr ) += gspr;
            *(inst->MESdrainPrimeDrainPrimePtr ) += gdpr+gds+ggd;
            *(inst->MESdrainPrimeDrainPrimePtr   ) += xgd*s->real;
            *(inst->MESdrainPrimeDrainPrimePtr +1) += xgd*s->imag;
            *(inst->MESsourcePrimeSourcePrimePtr ) += gspr+gds+gm+ggs;
            *(inst->MESsourcePrimeSourcePrimePtr   ) += xgs*s->real;
            *(inst->MESsourcePrimeSourcePrimePtr +1) += xgs*s->imag;
            *(inst->MESdrainDrainPrimePtr ) -= gdpr;
            *(inst->MESgateDrainPrimePtr ) -= ggd;
            *(inst->MESgateDrainPrimePtr   ) -= xgd*s->real;
            *(inst->MESgateDrainPrimePtr +1) -= xgd*s->imag;
            *(inst->MESgateSourcePrimePtr ) -= ggs;
            *(inst->MESgateSourcePrimePtr   ) -= xgs*s->real;
            *(inst->MESgateSourcePrimePtr +1) -= xgs*s->imag;
            *(inst->MESsourceSourcePrimePtr ) -= gspr;
            *(inst->MESdrainPrimeDrainPtr ) -= gdpr;
            *(inst->MESdrainPrimeGatePtr ) += (-ggd+gm);
            *(inst->MESdrainPrimeGatePtr   ) -= xgd*s->real;
            *(inst->MESdrainPrimeGatePtr +1) -= xgd*s->imag;
            *(inst->MESdrainPrimeSourcePrimePtr ) += (-gds-gm);
            *(inst->MESsourcePrimeGatePtr ) += (-ggs-gm);
            *(inst->MESsourcePrimeGatePtr   ) -= xgs*s->real;
            *(inst->MESsourcePrimeGatePtr +1) -= xgs*s->imag;
            *(inst->MESsourcePrimeSourcePtr ) -= gspr;
            *(inst->MESsourcePrimeDrainPrimePtr ) -= gds;
        }
    }
    return (OK);
}
