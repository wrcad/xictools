
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
Authors: 1985 Hong J. Park, Thomas L. Quarles 
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b1defs.h"


int
B1dev::temperature(sGENmodel *genmod, sCKT *ckt)
{
    (void)ckt;
    double EffChanLength;
    double EffChanWidth;
    double Cox;
    double CoxWoverL ;
    double Leff;    // effective channel length im micron
    double Weff;    // effective channel width in micron

    sB1model *model = static_cast<sB1model*>(genmod);
    for ( ; model; model = model->next()) {
    
        // Default value Processing for B1 MOSFET Models
        // Some Limiting for Model Parameters
        if (model->B1bulkJctPotential < 0.1)
            model->B1bulkJctPotential = 0.1;
        if (model->B1sidewallJctPotential < 0.1)
            model->B1sidewallJctPotential = 0.1;

        Cox = 3.453e-13/(model->B1oxideThickness * 1.0e-4); // in F/cm**2
        model->B1Cox = Cox;     // unit:  F/cm**2

        sB1instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if( (EffChanLength = inst->B1l - model->B1deltaL *1e-6 )<=0) { 
                DVO.textOut(OUT_FATAL,
                    "B1: mosfet %s, model %s: Effective channel length <= 0",
                    inst->GENname, model->GENmodName);
                return (E_BADPARM);
            }
            if( (EffChanWidth = inst->B1w - model->B1deltaW *1e-6 ) <= 0 ) {
                DVO.textOut(OUT_FATAL,
                    "B1: mosfet %s, model %s: Effective channel width <= 0",
                    inst->GENname, model->GENmodName);
                return (E_BADPARM);
            }
            inst->B1GDoverlapCap=EffChanWidth *model->B1gateDrainOverlapCap;
            inst->B1GSoverlapCap=EffChanWidth*model->B1gateSourceOverlapCap;
            inst->B1GBoverlapCap=inst->B1l * model->B1gateBulkOverlapCap;

            // process drain series resistance
            if ((inst->B1drainConductance=model->B1sheetResistance *
                    inst->B1drainSquares) != 0.0)
                inst->B1drainConductance = 1. / inst->B1drainConductance;
                   
            // process source series resistance
            if ((inst->B1sourceConductance=model->B1sheetResistance *
                    inst->B1sourceSquares) != 0.0)
                inst->B1sourceConductance = 1. / inst->B1sourceConductance;
                   
            Leff = EffChanLength * 1.e6; // convert into micron
            Weff = EffChanWidth * 1.e6; // convert into micron
            CoxWoverL = Cox * Weff / Leff; // F/cm**2

            inst->B1vfb = model->B1vfb0 + 
                model->B1vfbL / Leff + model->B1vfbW / Weff;
            inst->B1phi = model->B1phi0 +
                model->B1phiL / Leff + model->B1phiW / Weff;
            inst->B1K1 = model->B1K10 +
                model->B1K1L / Leff + model->B1K1W / Weff;
            inst->B1K2 = model->B1K20 +
                model->B1K2L / Leff + model->B1K2W / Weff;
            inst->B1eta = model->B1eta0 +
                model->B1etaL / Leff + model->B1etaW / Weff;
            inst->B1etaB = model->B1etaB0 +
                model->B1etaBl / Leff + model->B1etaBw / Weff;
            inst->B1etaD = model->B1etaD0 +
                model->B1etaDl / Leff + model->B1etaDw / Weff;
            inst->B1betaZero = model->B1mobZero;
            inst->B1betaZeroB = model->B1mobZeroB0 + 
                model->B1mobZeroBl / Leff + model->B1mobZeroBw / Weff;
            inst->B1ugs = model->B1ugs0 +
                model->B1ugsL / Leff + model->B1ugsW / Weff;
            inst->B1ugsB = model->B1ugsB0 +
                model->B1ugsBL / Leff + model->B1ugsBW / Weff;
            inst->B1uds = model->B1uds0 +
                model->B1udsL / Leff + model->B1udsW / Weff;
            inst->B1udsB = model->B1udsB0 +
                model->B1udsBL / Leff + model->B1udsBW / Weff;
            inst->B1udsD = model->B1udsD0 +
                model->B1udsDL / Leff + model->B1udsDW / Weff;
            inst->B1betaVdd = model->B1mobVdd0 +
                model->B1mobVddl / Leff + model->B1mobVddw / Weff;
            inst->B1betaVddB = model->B1mobVddB0 + 
                model->B1mobVddBl / Leff + model->B1mobVddBw / Weff;
            inst->B1betaVddD = model->B1mobVddD0 +
                model->B1mobVddDl / Leff + model->B1mobVddDw / Weff;
            inst->B1subthSlope = model->B1subthSlope0 + 
                model->B1subthSlopeL / Leff + model->B1subthSlopeW / Weff;
            inst->B1subthSlopeB = model->B1subthSlopeB0 +
                model->B1subthSlopeBL / Leff + model->B1subthSlopeBW / Weff;
            inst->B1subthSlopeD = model->B1subthSlopeD0 + 
                model->B1subthSlopeDL / Leff + model->B1subthSlopeDW / Weff;

            if (inst->B1phi < 0.1 ) inst->B1phi = 0.1;
            if (inst->B1K1 < 0.0) inst->B1K1 = 0.0;
            if (inst->B1K2 < 0.0) inst->B1K2 = 0.0;

            inst->B1vt0 = inst->B1vfb + inst->B1phi + inst->B1K1 * 
                sqrt(inst->B1phi) - inst->B1K2 * inst->B1phi;

            inst->B1von = inst->B1vt0;  // added for initialization

            // process Beta Parameters (unit: A/V**2)

            inst->B1betaZero = inst->B1betaZero * CoxWoverL;
            inst->B1betaZeroB = inst->B1betaZeroB * CoxWoverL;
            inst->B1betaVdd = inst->B1betaVdd * CoxWoverL;
            inst->B1betaVddB = inst->B1betaVddB * CoxWoverL;
            inst->B1betaVddD = SPMAX(inst->B1betaVddD * CoxWoverL,0.0);

        }
    }
    return (OK);
}  


