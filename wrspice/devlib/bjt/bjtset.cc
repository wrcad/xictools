
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

#include "bjtdefs.h"


namespace {
    int get_node_ptr(sCKT *ckt, sBJTinstance *inst)
    {
        TSTALLOC(BJTcolColPrimePtr,BJTcolNode,BJTcolPrimeNode)
        TSTALLOC(BJTbaseBasePrimePtr,BJTbaseNode,BJTbasePrimeNode)
        TSTALLOC(BJTemitEmitPrimePtr,BJTemitNode,BJTemitPrimeNode)
        TSTALLOC(BJTcolPrimeColPtr,BJTcolPrimeNode,BJTcolNode)
        TSTALLOC(BJTcolPrimeBasePrimePtr,BJTcolPrimeNode,BJTbasePrimeNode)
        TSTALLOC(BJTcolPrimeEmitPrimePtr,BJTcolPrimeNode,BJTemitPrimeNode)
        TSTALLOC(BJTbasePrimeBasePtr,BJTbasePrimeNode,BJTbaseNode)
        TSTALLOC(BJTbasePrimeColPrimePtr,BJTbasePrimeNode,BJTcolPrimeNode)
        TSTALLOC(BJTbasePrimeEmitPrimePtr,BJTbasePrimeNode,BJTemitPrimeNode)
        TSTALLOC(BJTemitPrimeEmitPtr,BJTemitPrimeNode,BJTemitNode)
        TSTALLOC(BJTemitPrimeColPrimePtr,BJTemitPrimeNode,BJTcolPrimeNode)
        TSTALLOC(BJTemitPrimeBasePrimePtr,BJTemitPrimeNode,BJTbasePrimeNode)
        TSTALLOC(BJTcolColPtr,BJTcolNode,BJTcolNode)
        TSTALLOC(BJTbaseBasePtr,BJTbaseNode,BJTbaseNode)
        TSTALLOC(BJTemitEmitPtr,BJTemitNode,BJTemitNode)
        TSTALLOC(BJTcolPrimeColPrimePtr,BJTcolPrimeNode,BJTcolPrimeNode)
        TSTALLOC(BJTbasePrimeBasePrimePtr,BJTbasePrimeNode,BJTbasePrimeNode)
        TSTALLOC(BJTemitPrimeEmitPrimePtr,BJTemitPrimeNode,BJTemitPrimeNode)
        TSTALLOC(BJTsubstSubstPtr,BJTsubstNode,BJTsubstNode)
        TSTALLOC(BJTcolPrimeSubstPtr,BJTcolPrimeNode,BJTsubstNode)
        TSTALLOC(BJTsubstColPrimePtr,BJTsubstNode,BJTcolPrimeNode)
        TSTALLOC(BJTbaseColPrimePtr,BJTbaseNode,BJTcolPrimeNode)
        TSTALLOC(BJTcolPrimeBasePtr,BJTcolPrimeNode,BJTbaseNode)
        return (OK);
    }
}


// This routine should only be called when circuit topology
// changes, since its computations do not depend on most
// device or model parameters, only on topology (as
// affected by emitter, collector, and base resistances)
//
// load the BJT structure with those pointers needed later 
// for fast matrix loading 
//
int
BJTdev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
    sBJTmodel *model = static_cast<sBJTmodel*>(genmod);
    for ( ; model; model = model->next()) {

        if (model->BJTtype != NPN && model->BJTtype != PNP)
            model->BJTtype = NPN;
        if (!model->BJTsatCurGiven)
            model->BJTsatCur = 1e-16;
        if (!model->BJTbetaFGiven)
            model->BJTbetaF = 100;
        if (!model->BJTemissionCoeffFGiven)
            model->BJTemissionCoeffF = 1;
        if (!model->BJTleakBEemissionCoeffGiven)
            model->BJTleakBEemissionCoeff = 1.5;
        if (!model->BJTbetaRGiven)
            model->BJTbetaR = 1;
        if (!model->BJTemissionCoeffRGiven)
            model->BJTemissionCoeffR = 1;
        if (!model->BJTleakBCemissionCoeffGiven)
            model->BJTleakBCemissionCoeff = 2;
        if (!model->BJTbaseResistGiven)
            model->BJTbaseResist = 0;
        if (!model->BJTemitterResistGiven)
            model->BJTemitterResist = 0;
        if (!model->BJTcollectorResistGiven)
            model->BJTcollectorResist = 0;
        if (!model->BJTdepletionCapBEGiven)
            model->BJTdepletionCapBE = 0;
        if (!model->BJTpotentialBEGiven)
            model->BJTpotentialBE = .75;
        if (!model->BJTjunctionExpBEGiven)
            model->BJTjunctionExpBE = .33;
        if (!model->BJTtransitTimeFGiven)
            model->BJTtransitTimeF = 0;
        if (!model->BJTtransitTimeBiasCoeffFGiven)
            model->BJTtransitTimeBiasCoeffF = 0;
        if (!model->BJTtransitTimeHighCurrentFGiven)
            model->BJTtransitTimeHighCurrentF = 0;
        if (!model->BJTexcessPhaseGiven)
            model->BJTexcessPhase = 0;
        if (!model->BJTdepletionCapBCGiven)
            model->BJTdepletionCapBC = 0;
        if (!model->BJTpotentialBCGiven)
            model->BJTpotentialBC = .75;
        if (!model->BJTjunctionExpBCGiven)
            model->BJTjunctionExpBC = .33;
        if (!model->BJTbaseFractionBCcapGiven)
            model->BJTbaseFractionBCcap = 1;
        if (!model->BJTtransitTimeRGiven)
            model->BJTtransitTimeR = 0;
        if (!model->BJTcapCSGiven)
            model->BJTcapCS = 0;
        if (!model->BJTpotentialSubstrateGiven)
            model->BJTpotentialSubstrate = .75;
        if (!model->BJTexponentialSubstrateGiven)
            model->BJTexponentialSubstrate = 0;
        if (!model->BJTbetaExpGiven)
            model->BJTbetaExp = 0;
        if (!model->BJTenergyGapGiven)
            model->BJTenergyGap = 1.11;
        if (!model->BJTtempExpISGiven)
            model->BJTtempExpIS = 3;
        if (!model->BJTfNcoefGiven)
            model->BJTfNcoef = 0;
        if (!model->BJTfNexpGiven)
            model->BJTfNexp = 1;

//
// COMPATABILITY WARNING!
// special note:  for backward compatability to much older models, spice 2G
// implemented a special case which checked if B-E leakage saturation
// current was >1, then it was instead a the B-E leakage saturation current
// divided by IS, and multiplied it by IS at this point.  This was not
// handled correctly in the 2G code, and there is some question on its 
// reasonability, since it is also undocumented, so it has been left out
// here.  It could easily be added with 1 line.  (The same applies to the B-C
// leakage saturation current).   TQ  6/29/84
//
            
        sBJTinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            
            sCKTnode *tmp;
            int error;
            if (!inst->BJTareaGiven)
                inst->BJTarea = 1;
            if (model->BJTcollectorResist == 0)
                inst->BJTcolPrimeNode = inst->BJTcolNode;
            else if (inst->BJTcolPrimeNode == 0) {
                error = ckt->mkVolt(&tmp, inst->GENname, "collector");
                if (error)
                    return (error);
                inst->BJTcolPrimeNode = tmp->number();
            }
            if (model->BJTbaseResist == 0)
                inst->BJTbasePrimeNode = inst->BJTbaseNode;
            else if (inst->BJTbasePrimeNode == 0) {
                error = ckt->mkVolt(&tmp, inst->GENname, "base");
                if (error)
                    return (error);
                inst->BJTbasePrimeNode = tmp->number();
            }
            if (model->BJTemitterResist == 0)
                inst->BJTemitPrimeNode = inst->BJTemitNode;
            else if (inst->BJTemitPrimeNode == 0) {
                error = ckt->mkVolt(&tmp, inst->GENname, "emitter");
                if (error)
                    return (error);
                inst->BJTemitPrimeNode = tmp->number();
            }

            // parser sets to -1 if 3 nodes given
            if (inst->BJTsubstNode < 0)
                inst->BJTsubstNode = 0;

            inst->GENstate = *states;
            *states += BJTnumStates;

            error = get_node_ptr(ckt, inst);
            if (error != OK)
                return (error);
        }
    }
    return(OK);
}


int
BJTdev::unsetup(sGENmodel *genmod, sCKT*)
{
    sBJTmodel *model = static_cast<sBJTmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sBJTinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            if (inst->BJTcolPrimeNode != inst->BJTcolNode)
                inst->BJTcolPrimeNode = 0;
            if (inst->BJTbasePrimeNode != inst->BJTbaseNode)
                inst->BJTbasePrimeNode = 0;
            if (inst->BJTemitPrimeNode != inst->BJTemitNode)
                inst->BJTemitPrimeNode = 0;
        }
    }
    return (OK);
}


// SRW - reset the matrix element pointers.
//
int
BJTdev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sBJTmodel *model = (sBJTmodel*)inModel; model;
            model = model->next()) {
        for (sBJTinstance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}

