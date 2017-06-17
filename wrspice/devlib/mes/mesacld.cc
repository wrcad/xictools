
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
 $Id: mesacld.cc,v 1.0 1998/01/30 05:31:38 stevew Exp $
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
