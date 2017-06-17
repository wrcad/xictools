
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
 $Id: mosconv.cc,v 1.1 2010/04/04 05:49:10 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1989 Takayasu Sakurai
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mosdefs.h"


int
MOSdev::convTest(sGENmodel *genmod, sCKT *ckt)
{
    if (ckt->CKTnoncon > 0)
        // something else didn't converge, bypass
        return (OK);

    sMOSmodel *model = static_cast<sMOSmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sMOSinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
        
            if (inst->MOSoff && (ckt->CKTmode & MODEINITFIX))
                continue;

            double vbs, vgs, vds;
            if (model->MOStype > 0) {
                vbs = *(ckt->CKTrhs + inst->MOSbNode) -
                    *(ckt->CKTrhs + inst->MOSsNodePrime);
                vgs = *(ckt->CKTrhs + inst->MOSgNode) -
                    *(ckt->CKTrhs + inst->MOSsNodePrime);
                vds = *(ckt->CKTrhs + inst->MOSdNodePrime) -
                    *(ckt->CKTrhs + inst->MOSsNodePrime);
            }
            else {
                vbs = *(ckt->CKTrhs + inst->MOSsNodePrime) -
                    *(ckt->CKTrhs + inst->MOSbNode);
                vgs = *(ckt->CKTrhs + inst->MOSsNodePrime) -
                    *(ckt->CKTrhs + inst->MOSgNode);
                vds = *(ckt->CKTrhs + inst->MOSsNodePrime) -
                    *(ckt->CKTrhs + inst->MOSdNodePrime);
            }
            double vbd = vbs - vds;
            double vgd = vgs - vds;

            double delvbs = vbs - *(ckt->CKTstate0 + inst->MOSvbs);
            double delvbd = vbd - *(ckt->CKTstate0 + inst->MOSvbd);
            double delvgs = vgs - *(ckt->CKTstate0 + inst->MOSvgs);
            double delvds = vds - *(ckt->CKTstate0 + inst->MOSvds);
            double delvgd = vgd - (*(ckt->CKTstate0 + inst->MOSvgs) -
                            *(ckt->CKTstate0 + inst->MOSvds));

            double cdhat;
            if (inst->MOSmode >= 0) {
                cdhat = inst->MOScd -
                    inst->MOSgbd*delvbd + inst->MOSgmbs*delvbs +
                    inst->MOSgm*delvgs + inst->MOSgds*delvds;
            }
            else {
                cdhat = inst->MOScd -
                    (inst->MOSgbd - inst->MOSgmbs)*delvbd -
                    inst->MOSgm*delvgd + inst->MOSgds*delvds;
            }

            //
            //  check convergence
            //
            double A1 = FABS(cdhat);
            double A2 = FABS(inst->MOScd);
            double A3 = cdhat - inst->MOScd;
            A3 = FABS(A3);
            double tol = ckt->CKTcurTask->TSKreltol*SPMAX(A1,A2) +
                ckt->CKTcurTask->TSKabstol;
            if (A3 >= tol) { 
                ckt->CKTnoncon++;
                ckt->CKTtroubleElt = inst;
                // no reason to continue, we haven't converged
                return (OK);
            }

            double cbhat = inst->MOScbs + inst->MOScbd +
                inst->MOSgbd*delvbd + inst->MOSgbs*delvbs;

            A1 = FABS(cbhat);
            A2 = inst->MOScbs + inst->MOScbd;
            A2 = FABS(A2);
            A3 = cbhat - (inst->MOScbs + inst->MOScbd);
            A3 = FABS(A3);
            tol = ckt->CKTcurTask->TSKreltol*SPMAX(A1,A2) +
                ckt->CKTcurTask->TSKabstol;
            if (A3 > tol) {
                ckt->CKTnoncon++;
                ckt->CKTtroubleElt = inst;
                // no reason to continue, we haven't converged
                return (OK);
            }
        }
    }
    return (OK);
}
