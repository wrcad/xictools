
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
 $Id: bjtconv.cc,v 1.1 2010/04/04 05:48:47 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "bjtdefs.h"


// This routine performs the device convergence test for
// BJTs in the circuit.
//
int
BJTdev::convTest(sGENmodel *genmod, sCKT *ckt)
{
    sBJTmodel *model = static_cast<sBJTmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sBJTinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (inst->BJToff && (ckt->CKTmode & MODEINITFIX))
                continue;

            double vbe;
            double vbc;
            if (model->BJTtype > 0) {
                vbe = *(ckt->CKTrhsOld + inst->BJTbasePrimeNode) -
                    *(ckt->CKTrhsOld + inst->BJTemitPrimeNode);
                vbc = *(ckt->CKTrhsOld + inst->BJTbasePrimeNode) -
                    *(ckt->CKTrhsOld + inst->BJTcolPrimeNode);
            }
            else {
                vbe = *(ckt->CKTrhsOld + inst->BJTemitPrimeNode) -
                    *(ckt->CKTrhsOld + inst->BJTbasePrimeNode);
                vbc = *(ckt->CKTrhsOld + inst->BJTcolPrimeNode) -
                    *(ckt->CKTrhsOld + inst->BJTbasePrimeNode);
            }
            double delvbe = vbe - *(ckt->CKTstate0 + inst->BJTvbe);
            double delvbc = vbc - *(ckt->CKTstate0 + inst->BJTvbc);

            //
            //   check convergence, collector current
            //
            double c0 = inst->BJTcc;
            double chat = c0 +
                (inst->BJTgm + inst->BJTgo)*delvbe -
                (inst->BJTgo + inst->BJTgmu)*delvbc;

            double A1 = FABS(chat);
            double A2 = FABS(c0);
            double tol = ckt->CKTcurTask->TSKreltol*SPMAX(A1,A2) +
                ckt->CKTcurTask->TSKabstol;

            A1 = chat - c0;
            if (FABS(A1) > tol) {
                ckt->CKTnoncon++;
                ckt->CKTtroubleElt = inst;
                return (OK); // no reason to continue - we've failed...
            }

            //
            //   check convergence, base current
            //
            c0 = inst->BJTcb;
            chat = c0 +
                inst->BJTgpi*delvbe +
                inst->BJTgmu*delvbc;

            A1 = FABS(chat);
            A2 = FABS(c0);
            tol = ckt->CKTcurTask->TSKreltol*SPMAX(A1,A2) +
                ckt->CKTcurTask->TSKabstol;

            A1 = chat - c0;
            if (FABS(A1) > tol) {
                ckt->CKTnoncon++;
                ckt->CKTtroubleElt = inst;
                return (OK); // no reason to continue - we've failed...
            }
        }
    }
    return (OK);
}
