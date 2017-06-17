
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
 $Id: mutload.cc,v 2.3 2015/09/09 18:22:21 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "inddefs.h"


//
// THe mutuals MUST be loaded BEFORE the inductors.
//

int
MUTdev::load(sGENinstance *in_inst, sCKT *ckt)
{
    if (ckt->CKTmode & MODEDC)
        return (OK);

    sMUTinstance *inst = (sMUTinstance*)in_inst;

    if (inst->MUTfactor == 0.0) {
        // This is computed here rather than setup so that the order
        // of setup calls for mutual and inductors is arbitrary.  If
        // this was set in setup, we would need to ensure that the
        // inductor setup was called first.

        inst->MUTfactor = inst->MUTcoupling *
            sqrt(inst->MUTind1->INDinduct * inst->MUTind2->INDinduct);
    }

    if (ckt->CKTmode & MODEINITFLOAT) {
        if (inst->MUTind1->INDtree || inst->MUTind2->INDtree) {
            // Really only need this at first iteration following
            // predictor.

            inst->MUTfactor = inst->MUTcoupling *
                sqrt(inst->MUTind1->INDinduct * inst->MUTind2->INDinduct);
        }
        double factor = inst->MUTfactor;
        *(ckt->CKTstate0 + inst->MUTind1->INDflux) +=
            factor * *(ckt->CKTrhsOld + inst->MUTind2->INDbrEq);

        *(ckt->CKTstate0 + inst->MUTind2->INDflux) +=
            factor * *(ckt->CKTrhsOld + inst->MUTind1->INDbrEq);

        factor *= ckt->CKTag[0];
        ckt->ldadd(inst->MUTbr1br2, -factor);
        ckt->ldadd(inst->MUTbr2br1, -factor);
    }
    else if (ckt->CKTmode & MODEINITPRED) {
        // Note that the MUTfactor is based on the inductances at the
        // last time point, so may be slightly wrong for nonlinear
        // inductors.  The new inductance values have not been
        // computed yet.  We'll get the new factor in MODEINITFLOAT so
        // things should end up ok.

        double factor = inst->MUTfactor*ckt->CKTag[0];
        ckt->ldadd(inst->MUTbr1br2, -factor);
        ckt->ldadd(inst->MUTbr2br1, -factor);
    }
    else if (ckt->CKTmode & MODEINITTRAN) {
        double factor = inst->MUTfactor;
        if (ckt->CKTmode & MODEUIC) {

            *(ckt->CKTstate1 + inst->MUTind1->INDflux) +=
                factor * inst->MUTind2->INDinitCond;

            *(ckt->CKTstate1 + inst->MUTind2->INDflux) +=
                factor * inst->MUTind1->INDinitCond;
        }
        else {
            *(ckt->CKTstate1 + inst->MUTind1->INDflux) +=
                factor * *(ckt->CKTrhsOld + inst->MUTind2->INDbrEq);

            *(ckt->CKTstate1 + inst->MUTind2->INDflux) +=
                factor * *(ckt->CKTrhsOld + inst->MUTind1->INDbrEq);
        }
        factor *= ckt->CKTag[0];
        ckt->ldadd(inst->MUTbr1br2, -factor);
        ckt->ldadd(inst->MUTbr2br1, -factor);
    }
    return (OK);
}

