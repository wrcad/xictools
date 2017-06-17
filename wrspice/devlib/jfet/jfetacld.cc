
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
 $Id: jfetacld.cc,v 1.0 1998/01/30 05:30:17 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/
/********** new in 3f2
Sydney University mods Copyright(c) 1989 Anthony E. Parker, David J. Skellern
    Laboratory for Communication Science Engineering
    Sydney University Department of Electrical Engineering, Australia
**********/

#include "jfetdefs.h"


int
JFETdev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    sJFETmodel *model = static_cast<sJFETmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sJFETinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            double gdpr = model->JFETdrainConduct * inst->JFETarea;
            double gspr = model->JFETsourceConduct * inst->JFETarea;
            double gm  = *(ckt->CKTstate0 + inst->JFETgm);
            double gds = *(ckt->CKTstate0 + inst->JFETgds);
            double ggs = *(ckt->CKTstate0 + inst->JFETggs);
            double xgs = *(ckt->CKTstate0 + inst->JFETqgs) * ckt->CKTomega;
            double ggd = *(ckt->CKTstate0 + inst->JFETggd);
            double xgd = *(ckt->CKTstate0 + inst->JFETqgd) * ckt->CKTomega;
            *(inst->JFETdrainDrainPtr ) += gdpr;
            *(inst->JFETgateGatePtr ) += ggd+ggs;
            *(inst->JFETgateGatePtr +1) += xgd+xgs;
            *(inst->JFETsourceSourcePtr ) += gspr;
            *(inst->JFETdrainPrimeDrainPrimePtr ) += gdpr+gds+ggd;
            *(inst->JFETdrainPrimeDrainPrimePtr +1) += xgd;
            *(inst->JFETsourcePrimeSourcePrimePtr ) += gspr+gds+gm+ggs;
            *(inst->JFETsourcePrimeSourcePrimePtr +1) += xgs;
            *(inst->JFETdrainDrainPrimePtr ) -= gdpr;
            *(inst->JFETgateDrainPrimePtr ) -= ggd;
            *(inst->JFETgateDrainPrimePtr +1) -= xgd;
            *(inst->JFETgateSourcePrimePtr ) -= ggs;
            *(inst->JFETgateSourcePrimePtr +1) -= xgs;
            *(inst->JFETsourceSourcePrimePtr ) -= gspr;
            *(inst->JFETdrainPrimeDrainPtr ) -= gdpr;
            *(inst->JFETdrainPrimeGatePtr ) += (-ggd+gm);
            *(inst->JFETdrainPrimeGatePtr +1) -= xgd;
            *(inst->JFETdrainPrimeSourcePrimePtr ) += (-gds-gm);
            *(inst->JFETsourcePrimeGatePtr ) += (-ggs-gm);
            *(inst->JFETsourcePrimeGatePtr +1) -= xgs;
            *(inst->JFETsourcePrimeSourcePtr ) -= gspr;
            *(inst->JFETsourcePrimeDrainPrimePtr ) -= gds;
        }
    }
    return (OK);
}
