
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
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mosdefs.h"


int
MOSdev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    sMOSmodel *model = static_cast<sMOSmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sMOSinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            //
            //     meyer's model parameters
            //
            double xgs = *(ckt->CKTstate0 + inst->MOScapgs) + 
                    *(ckt->CKTstate0 + inst->MOScapgs) +
                    inst->MOSgateSourceOverlapCap;
            double xgd = *(ckt->CKTstate0 + inst->MOScapgd) + 
                    *(ckt->CKTstate0 + inst->MOScapgd) +
                    inst->MOSgateDrainOverlapCap;
            double xgb = *(ckt->CKTstate0 + inst->MOScapgb) + 
                    *(ckt->CKTstate0 + inst->MOScapgb) +
                    inst->MOSgateBulkOverlapCap;

#define MSC(xx) inst->MOSmult*(xx)

            double omega = MSC(ckt->CKTomega);
            xgs *= omega;
            xgd *= omega;
            xgb *= omega;
            double xbd = inst->MOScapbd * omega;
            double xbs = inst->MOScapbs * omega;

            double dcon = MSC(inst->MOSdrainConductance);
            double scon = MSC(inst->MOSsourceConductance);

            double gbd = MSC(inst->MOSgbd);
            double gbs = MSC(inst->MOSgbs);
            double gds = MSC(inst->MOSgds);
            double gm = MSC(inst->MOSgm);
            double gmbs = MSC(inst->MOSgmbs);

            //
            //    load matrix
            //
            *(inst->MOSGgPtr  +1) += xgd + xgs + xgb;
            *(inst->MOSBbPtr  +1) += xgb + xbd + xbs;
            *(inst->MOSDPdpPtr+1) += xgd + xbd;
            *(inst->MOSSPspPtr+1) += xgs + xbs;
            *(inst->MOSGbPtr  +1) -= xgb;
            *(inst->MOSGdpPtr +1) -= xgd;
            *(inst->MOSGspPtr +1) -= xgs;
            *(inst->MOSBgPtr  +1) -= xgb;
            *(inst->MOSBdpPtr +1) -= xbd;
            *(inst->MOSBspPtr +1) -= xbs;
            *(inst->MOSDPgPtr +1) -= xgd;
            *(inst->MOSDPbPtr +1) -= xbd;
            *(inst->MOSSPgPtr +1) -= xgs;
            *(inst->MOSSPbPtr +1) -= xbs;
            *(inst->MOSDdPtr) += dcon;
            *(inst->MOSSsPtr) += scon;
            *(inst->MOSBbPtr) += gbd + gbs;
            *(inst->MOSDdpPtr) -= dcon;
            *(inst->MOSSspPtr) -= scon;
            *(inst->MOSBdpPtr) -= gbd;
            *(inst->MOSBspPtr) -= gbs;
            *(inst->MOSDPdPtr) -= dcon;
            *(inst->MOSSPsPtr) -= scon;

            if (inst->MOSmode > 0) {

                *(inst->MOSDPdpPtr) += dcon + gds + gbd;
                *(inst->MOSSPspPtr) += scon + gds + gbs + gm + gmbs;

                *(inst->MOSDPgPtr)  += gm;
                *(inst->MOSDPbPtr)  += -gbd + gmbs;
                *(inst->MOSDPspPtr) -= gds + gm + gmbs;

                *(inst->MOSSPgPtr)  -= gm;
                *(inst->MOSSPbPtr)  -= gbs + gmbs;
                *(inst->MOSSPdpPtr) -= gds;
            }
            else {
                *(inst->MOSDPdpPtr) += dcon + gds + gbd + gm + gmbs;
                *(inst->MOSSPspPtr) += scon + gds + gbs;

                *(inst->MOSDPgPtr)  -= gm;
                *(inst->MOSDPbPtr)  -= gbd + gmbs;
                *(inst->MOSDPspPtr) -= gds;

                *(inst->MOSSPgPtr)  += gm;
                *(inst->MOSSPbPtr)  -= gbs - gmbs;
                *(inst->MOSSPdpPtr) -= gds + gm + gmbs;
            }
        }
    }
    return (OK);
}
