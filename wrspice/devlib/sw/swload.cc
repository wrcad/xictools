
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
 $Id: swload.cc,v 1.2 2015/06/11 01:12:29 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Gordon M. Jacobs
         1992 Stephen R. Whiteley
****************************************************************************/

#include "swdefs.h"


int
SWdev::load(sGENinstance *in_inst, sCKT *ckt)
{
    sSWinstance *inst = (sSWinstance*)in_inst;
    sSWmodel *model = (sSWmodel*)inst->GENmodPtr;

    // decide the state of the switch

    double current_state = 0;
    if (ckt->CKTmode & (MODEINITFIX|MODEINITJCT)) {

        if (inst->SWzero_stateGiven)
            // switch specified "on"
            current_state = 1.0;
        else
            current_state = 0.0;
        *(ckt->CKTstate0 + inst->GENstate) = current_state;;

    }
    else if (ckt->CKTmode & (MODEINITSMSIG)) {

        current_state = *(ckt->CKTstate0 + inst->GENstate);

    }
    else if (ckt->CKTmode & (MODEINITFLOAT)) {

        // use state0 since INITTRAN or INITPRED already called
        double previous_state = *(ckt->CKTstate0 + inst->GENstate);
        current_state = previous_state;

        if (inst->SWcontName) {
            double ctrl = *(ckt->CKTrhsOld + inst->SWcontBranch);
            if (ctrl > (model->SWiThreshold+model->SWiHysteresis))
                current_state = 1.0;
            else if
                (ctrl < (model->SWiThreshold-model->SWiHysteresis))
                current_state = 0.0;
        }
        else {
            double ctrl = *(ckt->CKTrhsOld + inst->SWposCntrlNode)
                    - *(ckt->CKTrhsOld + inst->SWnegCntrlNode);
            if (ctrl > (model->SWvThreshold+model->SWvHysteresis))
                current_state = 1.0;
            else if
                (ctrl < (model->SWvThreshold-model->SWvHysteresis))
                current_state = 0.0;
        }
        *(ckt->CKTstate0 + inst->GENstate) = current_state;

        if (current_state != previous_state) {
            ckt->incNoncon();  // SRW
            // ensure one more iteration
            ckt->CKTtroubleElt = inst;
        }
    }
    else if (ckt->CKTmode & (MODEINITTRAN|MODEINITPRED) ) {

        current_state = *(ckt->CKTstate1 + inst->GENstate);
        if (inst->SWcontName) {
            double ctrl = *(ckt->CKTrhsOld + inst->SWcontBranch);
            if (ctrl > (model->SWiThreshold+model->SWiHysteresis))
                current_state = 1.0;
            else if
                (ctrl < (model->SWiThreshold-model->SWiHysteresis))
                current_state = 0.0;
        }
        else {
            double ctrl = *(ckt->CKTrhsOld + inst->SWposCntrlNode)
                    - *(ckt->CKTrhsOld + inst->SWnegCntrlNode);
            if (ctrl > (model->SWvThreshold+model->SWvHysteresis))
                current_state = 1.0;
            else if
                (ctrl < (model->SWvThreshold-model->SWvHysteresis))
                current_state = 0.0;
        }
        *(ckt->CKTstate0 + inst->GENstate) = current_state;
    }

    double g_now =
        current_state ? model->SWonConduct : model->SWoffConduct;
    inst->SWcond = g_now;

    ckt->ldadd(inst->SWposPosptr, g_now);
    ckt->ldadd(inst->SWposNegptr, -g_now);
    ckt->ldadd(inst->SWnegPosptr, -g_now);
    ckt->ldadd(inst->SWnegNegptr, g_now);
    return (OK);
}
