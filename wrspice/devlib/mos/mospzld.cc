
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
MOSdev::pzLoad(sGENmodel *genmod, sCKT *ckt, IFcomplex *s)
{
    sMOSmodel *model = static_cast<sMOSmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sMOSinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
        
            int xnrm;
            int xrev;
            if (inst->MOSmode < 0) {
                xnrm=0;
                xrev=1;
            }
            else {
                xnrm=1;
                xrev=0;
            }
            //
            //   meyer's model parameters
            //
            double capgs = ( 2* *(ckt->CKTstate0+inst->MOScapgs)+ 
                      inst->MOSgateSourceOverlapCap );
            double capgd = ( 2* *(ckt->CKTstate0+inst->MOScapgd)+ 
                      inst->MOSgateDrainOverlapCap );
            double capgb = ( 2* *(ckt->CKTstate0+inst->MOScapgb)+ 
                      inst->MOSgateBulkOverlapCap );

#define MSC(xx) inst->MOSmult*(xx)

            double xgs = MSC(capgs);
            double xgd = MSC(capgd);
            double xgb = MSC(capgb);
            double xbd = MSC(inst->MOScapbd);
            double xbs = MSC(inst->MOScapbs);

            //
            //  load matrix
            //

            *(inst->MOSGgPtr     ) += (xgd+xgs+xgb)*s->real;
            *(inst->MOSGgPtr   +1) += (xgd+xgs+xgb)*s->imag;
            *(inst->MOSBbPtr     ) += (xgb+xbd+xbs)*s->real;
            *(inst->MOSBbPtr   +1) += (xgb+xbd+xbs)*s->imag;
            *(inst->MOSDPdpPtr   ) += (xgd+xbd)*s->real;
            *(inst->MOSDPdpPtr +1) += (xgd+xbd)*s->imag;
            *(inst->MOSSPspPtr   ) += (xgs+xbs)*s->real;
            *(inst->MOSSPspPtr +1) += (xgs+xbs)*s->imag;
            *(inst->MOSGbPtr     ) -= xgb*s->real;
            *(inst->MOSGbPtr   +1) -= xgb*s->imag;
            *(inst->MOSGdpPtr    ) -= xgd*s->real;
            *(inst->MOSGdpPtr  +1) -= xgd*s->imag;
            *(inst->MOSGspPtr    ) -= xgs*s->real;
            *(inst->MOSGspPtr  +1) -= xgs*s->imag;
            *(inst->MOSBgPtr     ) -= xgb*s->real;
            *(inst->MOSBgPtr   +1) -= xgb*s->imag;
            *(inst->MOSBdpPtr    ) -= xbd*s->real;
            *(inst->MOSBdpPtr  +1) -= xbd*s->imag;
            *(inst->MOSBspPtr    ) -= xbs*s->real;
            *(inst->MOSBspPtr  +1) -= xbs*s->imag;
            *(inst->MOSDPgPtr    ) -= xgd*s->real;
            *(inst->MOSDPgPtr  +1) -= xgd*s->imag;
            *(inst->MOSDPbPtr    ) -= xbd*s->real;
            *(inst->MOSDPbPtr  +1) -= xbd*s->imag;
            *(inst->MOSSPgPtr    ) -= xgs*s->real;
            *(inst->MOSSPgPtr  +1) -= xgs*s->imag;
            *(inst->MOSSPbPtr    ) -= xbs*s->real;
            *(inst->MOSSPbPtr  +1) -= xbs*s->imag;

            double dcon = MSC(inst->MOSdrainConductance);
            double scon = MSC(inst->MOSsourceConductance);

            double gbd = MSC(inst->MOSgbd);
            double gbs = MSC(inst->MOSgbs);
            double gds = MSC(inst->MOSgds);
            double gm = MSC(inst->MOSgm);
            double gmbs = MSC(inst->MOSgmbs);

            *(inst->MOSDdPtr) += dcon;
            *(inst->MOSSsPtr) += scon;
            *(inst->MOSBbPtr) += gbd + gbs;

            *(inst->MOSDPdpPtr) += dcon + gds + gbd + xrev*(gm + gmbs);

            *(inst->MOSSPspPtr) += scon + gds + gbs + xnrm*(gm + gmbs);

            *(inst->MOSDdpPtr) -= dcon;
            *(inst->MOSSspPtr) -= scon;
            *(inst->MOSBdpPtr) -= gbd;
            *(inst->MOSBspPtr) -= gbs;
            *(inst->MOSDPdPtr) -= dcon;
            *(inst->MOSDPgPtr) += (xnrm - xrev)*gm;
            *(inst->MOSDPbPtr) += -gbd + (xnrm - xrev)*gmbs;
            *(inst->MOSDPspPtr) -= gds + xnrm*(gm + gmbs);

            *(inst->MOSSPgPtr) -= (xnrm - xrev)*gm;
            *(inst->MOSSPsPtr) -= scon;
            *(inst->MOSSPbPtr) -= gbs + (xnrm - xrev)*gmbs;
            *(inst->MOSSPdpPtr) -= gds + xrev*(gm + gmbs);
        }
    }
    return (OK);
}
