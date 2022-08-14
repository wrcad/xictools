
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
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
Author: 1993 Stephen R. Whiteley
****************************************************************************/

#include "jjdefs.h"

//#define JJ_DEBUG
#ifdef JJ_DEBUG
#include <stdio.h>
#endif


#if defined(__GNUC__) && (defined(i386) || defined(__x86_64__))
#define ASM_SINCOS
#endif

struct jjstuff
{
    void jj_limiting(sCKT*, sJJmodel*, sJJinstance*);
    void jj_iv(sJJmodel*, sJJinstance*);
    void jj_ic(sJJmodel*, sJJinstance*);
    void jj_load(sCKT*, sJJmodel*, sJJinstance*);

    double js_vj;
    double js_phi;
    double js_ci;
    double js_gqt;
    double js_crhs;
    double js_crt;
    double js_dcrt;
    double js_pfac;
    double js_ddv;
};


int
JJdev::load(sGENinstance *in_inst, sCKT *ckt)
{
    sJJinstance *inst = (sJJinstance*)in_inst;
    sJJmodel *model = (sJJmodel*)inst->GENmodPtr;

    struct jjstuff js;

#ifdef NEWJJDC
    // For DC analysis, Josephson junctions require special treatment. 
    // The usual SPICE voltage-based calculation can't be used, since
    // 1. all (or most) node voltages are identically zero,
    // 2. inductors can't be shorted, since they contribute to the
    //    current distribution,
    // 3. quantum mechanical "phase" must be quantized around
    //    JJ/inductor loops.
    //
    // We get around this by using phase for josephson junctions,
    // inductors, and possibly inductor-like devices such as lossless
    // transmision lines.  Every node connected to a Josephson
    // junction or inductor has a "Phase" flag set.  This indicates
    // that the computed value is phase, and NOT voltage, for these
    // nodes.  We have to special-case the load functions of inductors
    // and mutual inductors (and lossless transmission lines), and
    // resistors.  Other devices can probably be treated normally,
    // however there will be limitations, as noted below.
    //
    // A JJ can be modeled by the basic formula I = Ic*sin(V(i,j)),
    // where V(i,j) is the "voltage" difference between nodes i and j,
    // which is actually the phase.  Inductors look like resistors,
    // L*I = V(i,j)*phi0/(2*pi), where again V(i,j) is the phase
    // difference across the inductor.  Mutual inductance adds similar
    // cross terms.
    //
    // Capacitors are completely ignored.  The treatment of resistors
    // is slightly complicated.  The connected nodes can each be
    // "Ground", "Phase", or "Voltage".  If both nodes are Ground or
    // Voltage, the resistor is loaded normally.  If both nodes are
    // Phase or Ground, the resistor is not loaded at all.  The
    // interesting cases are when one node is Phase, the other
    // Voltage.  In this case, we load the resistor as if the phase
    // node is actually Ground (number = 0).  In addition, we load a
    // VCCS template which injects current into the phase node, of
    // value V(voltage-node)/resistance.
    //
    // Resistors are the bridge between the normal voltage-mode
    // devices and phase nodes.  Some circuits may require
    // introduction of resistors to get correct results.  For example,
    // assume a JJ gate driving a CMOS comparator circuit.  If the
    // input MOSFET gate is connected directly to the JJ device, the
    // DCOP will be incorrect, as the comparator will see phase as its
    // input.  However, if a resistor separates the MOS gate from the
    // JJ, the comparator input will be zero, as it should be.
    // 
    // There is (at present) a topological requirement that all phase
    // nodes must be at ground potential.  This means that for a
    // network of JJs and inductors, there must be a ground connection
    // to one of these devices.  A voltage source connected to an
    // inductor, which is connected to a resistor to ground, although
    // a perfectly valid circuit, will fail.  One must use the
    // equivalent consisting of the voltage source connected to a
    // resistor, connected to the inductor which is grounded.  This
    // satisfies the two topological requirements:
    //
    // Rule 1:  There must be a resistor between a voltage-mode device 
    //          and a phase mode device, no direct connections.  
    //
    // Rule 2:  Every phase-node subnet must have a direct connection to
    //          ground, so all phase nodes are at ground potential.
    //
    // With this bit of information, and the warning that controlled
    // sources can cause unexpected behavior, the DC analysis using
    // this technique can apply to general circuits.
    //
    // Note that for this to work, all JJs must have bias current less
    // than their Ic.

    if (ckt->CKTmode & MODEDC) {

        js.js_ci  = (inst->JJcontrol) ?
                *(ckt->CKTrhsOld + inst->JJbranch) : 0;

        js.js_vj  = *(ckt->CKTrhsOld + inst->JJposNode) -
                *(ckt->CKTrhsOld + inst->JJnegNode);

        if (model->JJictype != 0) {
            js.js_phi = js.js_vj;
            *(ckt->CKTstate0 + inst->JJphase) = js.js_phi;
            *(ckt->CKTstate0 + inst->JJvoltage) = 0.0;

            js.js_pfac = 1.0;
            js.js_gqt = 0;
            js.js_crhs = 0;
            js.js_dcrt = 0;
            js.js_crt  = inst->JJcriti;
            if (model->JJictype != 1)
                js.jj_ic(model, inst);
            if (ckt->CKTmode & MODEINITSMSIG) {
                // We don't want/need this except when setting up for AC
                // analysis.
                js.jj_iv(model, inst);
            }
            js.jj_load(ckt, model, inst);
            // don't load shunt
#ifdef NEWLSH
            if (model->JJvShuntGiven && inst->JJgshunt > 0.0) {
                // Load lsh as 0-voltage source, don't load resistor. 
                // Dangling voltage source shouldn't matter.

                if (inst->JJlsh > 0.0) {
                    ckt->ldadd(inst->JJlshIbrIbrPtr, 0.0);
#ifndef USE_PRELOAD
                    ckt->ldset(inst->JJlshPosIbrPtr, 1.0);
                    ckt->ldset(inst->JJlshIbrPosPtr, 1.0);
                    ckt->ldset(inst->JJlshNegIbrPtr, -1.0);
                    ckt->ldset(inst->JJlshIbrNegPtr, -1.0);
#endif
                }
            }
#endif
        }
        else {
            // No critical current, treat like a nonlinear resistor.

            js.js_phi = 0.0;
            js.js_crhs = 0.0;
            js.js_dcrt = 0.0;
            js.js_crt  = 0.0;
        
            js.jj_iv(model, inst);
            js.jj_load(ckt, model, inst);

            // Load the shunt resistance implied if vshunt given.
            if (model->JJvShuntGiven && inst->JJgshunt > 0.0) {
                ckt->ldadd(inst->JJrshPosPosPtr, inst->JJgshunt);
                ckt->ldadd(inst->JJrshPosNegPtr, -inst->JJgshunt);
                ckt->ldadd(inst->JJrshNegPosPtr, -inst->JJgshunt);
                ckt->ldadd(inst->JJrshNegNegPtr, inst->JJgshunt);
#ifdef NEWLSH
                // Load lsh as 0-voltage source.
                if (inst->JJlsh > 0.0) {
                    ckt->ldadd(inst->JJlshIbrIbrPtr, 0.0);
#ifndef USE_PRELOAD
                    ckt->ldset(inst->JJlshPosIbrPtr, 1.0);
                    ckt->ldset(inst->JJlshIbrPosPtr, 1.0);
                    ckt->ldset(inst->JJlshNegIbrPtr, -1.0);
                    ckt->ldset(inst->JJlshIbrNegPtr, -1.0);
#endif
                }
#endif
            }
        }
        if (ckt->CKTmode & MODEINITSMSIG) {
            // for ac load
            inst->JJdcrt = js.js_dcrt;
        }
#ifdef NEWLSER
        if (inst->JJlser > 0.0) {
            double res = 2*M_PI*inst->JJlser/wrsCONSTphi0;
            ckt->ldadd(inst->JJlserIbrIbrPtr, -res);
        }
#ifndef USE_PRELOAD
        if (inst->JJrealPosNode) {
            ckt->ldset(inst->JJlserPosIbrPtr, 1.0);
            ckt->ldset(inst->JJlserIbrPosPtr, 1.0);
        }
        if (inst->JJposNode) {
            ckt->ldset(inst->JJlserNegIbrPtr, -1.0);
            ckt->ldset(inst->JJlserIbrNegPtr, -1.0);
        }
#endif
#endif
        return (OK);
    }
#else
    // May want to get rid of this, or possibly add as an option.

    // We want to provide some kind of DC operating point analysis
    // capability, mostly for use in hybrid JJ/semiconductor circuits
    // where we want to initialise the semiconductor part with JJ's
    // taken as shorts.  This is easier said than done, as there are
    // subtleties.
    //
    // It is important that when the transient simulation starts,
    // inductor currents and JJ phases are zero.  Only then will the
    // phase/flux relationship around JJ/inductor loops be correct as
    // the circuit evolves in time.  We therefor set the initial
    // transient JJ phase to 0 (but allow UIC override).  An external
    // function sDevLib::ZeroInductorCurrents must be called after any
    // operating point calculation and before the transient analysis
    // starts to take care of initial inductor currents.
    //
    // One might consider adding a branch node and loading the matrix
    // so that the device looks like a voltage source with zero
    // voltage during dc analysis.  However, this causes the matrix to
    // be singular if JJs and inductors form loops, which is common.
    //
    // We will instead make the JJ look like a small resistance during
    // dc analysis, with the code below.  It was found that one can't
    // be too aggressive with the conductance value, if too high,
    // matrix reordering will occur and fillins can be generated,
    // which can slow the simulation.  In one example, with a constant
    // g value going from 10 to 1000, simulation time went from 60 to
    // 90 secs with similar iteration counts, most of the difference
    // in lutime due to many more fillins with larger g.
    //
    // Below, we try to be a little more clever and scale the g value
    // to the critical current.  This will keep the JJ voltages within
    // vfoo of zero in dc operating point analysis (for sane circuits).

    // One important note:  The circuit should exist in a quiescent
    // steady-state when the sources are set to time=0 values.  I.e.,
    // junction bias currents should be less than the critical
    // currents at time=0.  If not, the DCOP will probably succeed,
    // but the first time step (and probably subsequent steps) may
    // fail, causing the run to abort.  Thus, the new DCOP does not
    // entirely get around the need to "ramp up" sources.

    if ((ckt->CKTmode & MODEDC) && !(ckt->CKTmode & MODEUIC)) {
        double g = 1e6 * inst->JJcriti;   // Ic/1uV
        ckt->ldadd(inst->JJposPosPtr, g);
        ckt->ldadd(inst->JJposNegPtr, -g);
        ckt->ldadd(inst->JJnegPosPtr, -g);
        ckt->ldadd(inst->JJnegNegPtr, g);
        return (OK);
    }
#endif

    js.js_pfac = ckt->CKTdelta / PHI0_2PI;
    if (ckt->CKTorder > 1)
        js.js_pfac *= .5;

    if (ckt->CKTmode & MODEINITFLOAT) {
        js.js_ci  = (inst->JJcontrol) ?
                *(ckt->CKTrhsOld + inst->JJbranch) : 0;

        js.js_vj  = *(ckt->CKTrhsOld + inst->JJposNode) -
                *(ckt->CKTrhsOld + inst->JJnegNode);

        double absvj  = fabs(js.js_vj);
        double temp   = *(ckt->CKTstate0 + inst->JJvoltage);
        double absold = fabs(temp);
        double maxvj  = SPMAX(absvj,absold);
        temp  -= js.js_vj;
        double absdvj = fabs(temp);

        if (maxvj >= inst->JJvless) {
            temp = model->JJdelv * 0.5;
            if (absdvj > temp) {
                js.js_ddv = temp;
                js.jj_limiting(ckt, model, inst);
                absvj  = fabs(js.js_vj);
                maxvj  = SPMAX(absvj, absold);
                temp   = js.js_vj - *(ckt->CKTstate0 + inst->JJvoltage);
                absdvj = fabs(temp);
            }
        }

        // check convergence

        if (!ckt->CKTnoncon) {
            double tol = ckt->CKTcurTask->TSKreltol*maxvj +
                ckt->CKTcurTask->TSKabstol;
            if (absdvj > tol) {
#ifdef JJ_DEBUG
                printf("%s %g %g %g %g\n", (const char*)inst->GENname,
                    maxvj, absdvj, js.js_vj,
                    *(ckt->CKTstate0 + inst->JJvoltage));
#endif
                ckt->incNoncon();  // SRW
            }
        }

        js.js_phi = *(ckt->CKTstate1 + inst->JJphase);
        temp = js.js_vj;
        if (ckt->CKTorder > 1)
            temp += *(ckt->CKTstate1 + inst->JJvoltage);
        js.js_phi += js.js_pfac*temp;

        js.js_crhs = 0;
        js.js_dcrt = 0;
        js.js_crt  = inst->JJcriti;
    
        //
        // compute quasiparticle current and derivatives
        //
        js.jj_iv(model, inst);
        if (model->JJictype != 1)
            js.jj_ic(model, inst);
        js.jj_load(ckt, model, inst);

        // Load the shunt resistance implied if vshunt given.
        if (model->JJvShuntGiven && inst->JJgshunt > 0.0) {
            ckt->ldadd(inst->JJrshPosPosPtr, inst->JJgshunt);
            ckt->ldadd(inst->JJrshPosNegPtr, -inst->JJgshunt);
            ckt->ldadd(inst->JJrshNegPosPtr, -inst->JJgshunt);
            ckt->ldadd(inst->JJrshNegNegPtr, inst->JJgshunt);
#ifdef NEWLSH
            if (inst->JJlsh > 0.0) {
                *(ckt->CKTstate0 + inst->JJlshFlux) = inst->JJlsh *
                    *(ckt->CKTrhsOld + inst->JJlshBr);
                ckt->integrate(inst->JJlshFlux, inst->JJlshVeq);

                ckt->rhsadd(inst->JJlshBr, inst->JJlshVeq);
                ckt->ldadd(inst->JJlshIbrIbrPtr, -inst->JJlshReq);
#ifndef USE_PRELOAD
                ckt->ldset(inst->JJlshPosIbrPtr, 1.0);
                ckt->ldset(inst->JJlshIbrPosPtr, 1.0);
                ckt->ldset(inst->JJlshNegIbrPtr, -1.0);
                ckt->ldset(inst->JJlshIbrNegPtr, -1.0);
#endif
            }
#endif
        }

        ckt->integrate(inst->JJvoltage, inst->JJdelVdelT);
#ifdef NEWLSER
        if (inst->JJlser > 0.0) {
            *(ckt->CKTstate0 + inst->JJlserFlux) = inst->JJlser *
                *(ckt->CKTrhsOld + inst->JJlserBr);
            ckt->integrate(inst->JJlserFlux, inst->JJlserVeq);

            ckt->rhsadd(inst->JJlserBr, inst->JJlserVeq);
            ckt->ldadd(inst->JJlserIbrIbrPtr, -inst->JJlserReq);
#ifndef USE_PRELOAD
            if (inst->JJrealPosNode) {
                ckt->ldset(inst->JJlserPosIbrPtr, 1.0);
                ckt->ldset(inst->JJlserIbrPosPtr, 1.0);
            }
            if (inst->JJposNode) {
                ckt->ldset(inst->JJlserNegIbrPtr, -1.0);
                ckt->ldset(inst->JJlserIbrNegPtr, -1.0);
            }
#endif
        }
#endif
        return (OK);
    }

    if (ckt->CKTmode & MODEINITPRED) {

        double y0 = DEV.pred(ckt, inst->JJdvdt);
        if (ckt->CKTorder != 1)
            y0 += *(ckt->CKTstate1 + inst->JJdvdt);

        double temp = *(ckt->CKTstate1 + inst->JJvoltage);
        double rag0  = ckt->CKTorder == 1 ? ckt->CKTdelta : .5*ckt->CKTdelta;
        js.js_vj  = temp + rag0*y0;

        js.js_phi = *(ckt->CKTstate1 + inst->JJphase);
        temp = js.js_vj;
        if (ckt->CKTorder > 1)
            temp += *(ckt->CKTstate1 + inst->JJvoltage);
        js.js_phi += js.js_pfac*temp;

        if (inst->JJcontrol)
            js.js_ci = DEV.pred(ckt, inst->JJconI);
        else
            js.js_ci = 0;

        inst->JJdelVdelT = ckt->find_ceq(inst->JJvoltage);

        js.js_crhs = 0;
        js.js_dcrt = 0;
        js.js_crt  = inst->JJcriti;

        js.jj_iv(model, inst);
        if (model->JJictype != 1)
            js.jj_ic(model, inst);
        js.jj_load(ckt, model, inst);

        // Load the shunt resistance implied if vshunt given.
        if (model->JJvShuntGiven && inst->JJgshunt > 0.0) {
            ckt->ldadd(inst->JJrshPosPosPtr, inst->JJgshunt);
            ckt->ldadd(inst->JJrshPosNegPtr, -inst->JJgshunt);
            ckt->ldadd(inst->JJrshNegPosPtr, -inst->JJgshunt);
            ckt->ldadd(inst->JJrshNegNegPtr, inst->JJgshunt);
#ifdef NEWLSH
            if (inst->JJlsh > 0.0) {
                inst->JJlshReq = ckt->CKTag[0] * inst->JJlsh;
                inst->JJlshVeq = ckt->find_ceq(inst->JJlshFlux);

                ckt->rhsadd(inst->JJlshBr, inst->JJlshVeq);
                ckt->ldadd(inst->JJlshIbrIbrPtr, -inst->JJlshReq);

                *(ckt->CKTstate0 + inst->JJlshFlux) = 0;
#ifndef USE_PRELOAD
                ckt->ldset(inst->JJlshPosIbrPtr, 1.0);
                ckt->ldset(inst->JJlshIbrPosPtr, 1.0);
                ckt->ldset(inst->JJlshNegIbrPtr, -1.0);
                ckt->ldset(inst->JJlshIbrNegPtr, -1.0);
#endif
            }
#endif
        }
#ifdef NEWLSER
        if (inst->JJlser > 0.0) {
            inst->JJlserReq = ckt->CKTag[0] * inst->JJlser;
            inst->JJlserVeq = ckt->find_ceq(inst->JJlserFlux);

            ckt->rhsadd(inst->JJlserBr, inst->JJlserVeq);
            ckt->ldadd(inst->JJlserIbrIbrPtr, -inst->JJlserReq);

            *(ckt->CKTstate0 + inst->JJlserFlux) = 0;
#ifndef USE_PRELOAD
            if (inst->JJrealPosNode) {
                ckt->ldset(inst->JJlserPosIbrPtr, 1.0);
                ckt->ldset(inst->JJlserIbrPosPtr, 1.0);
            }
            if (inst->JJposNode) {
                ckt->ldset(inst->JJlserNegIbrPtr, -1.0);
                ckt->ldset(inst->JJlserIbrNegPtr, -1.0);
            }
#endif
        }
#endif
        return (OK);
    }

    if (ckt->CKTmode & MODEINITTRAN) {
        if (ckt->CKTmode & MODEUIC) {
            js.js_vj  = inst->JJinitVoltage;
            js.js_phi = inst->JJinitPhase;
            js.js_ci  = inst->JJinitControl;
            *(ckt->CKTstate1 + inst->JJvoltage) = js.js_vj;
            *(ckt->CKTstate1 + inst->JJphase) = js.js_phi;
            *(ckt->CKTstate1 + inst->JJconI) = js.js_ci;
        }
        else {
            js.js_vj  = 0;
#ifdef NEWJJDC
            *(ckt->CKTstate1 + inst->JJvoltage) = 0.0;
            // The RHS node voltage was set to zero in the doTask code.
            js.js_phi = *(ckt->CKTstate1 + inst->JJphase);
            js.js_ci  = (inst->JJcontrol) ?
                *(ckt->CKTrhsOld + inst->JJbranch) : 0;
            *(ckt->CKTstate1 + inst->JJconI) = js.js_ci;
            *(ckt->CKTstate0 + inst->JJvoltage) = 0.0;
#else
            js.js_phi = 0;
            js.js_ci  = 0;  // This isn't right?
#endif
        }

        inst->JJdelVdelT = ckt->find_ceq(inst->JJvoltage);

        js.js_crhs = 0;
        js.js_dcrt = 0;
        js.js_crt  = inst->JJcriti;

        js.jj_iv(model, inst);
        if (model->JJictype != 1)
            js.jj_ic(model, inst);
        js.jj_load(ckt, model, inst);

        if (model->JJvShuntGiven && inst->JJgshunt > 0.0) {
            ckt->ldadd(inst->JJrshPosPosPtr, inst->JJgshunt);
            ckt->ldadd(inst->JJrshPosNegPtr, -inst->JJgshunt);
            ckt->ldadd(inst->JJrshNegPosPtr, -inst->JJgshunt);
            ckt->ldadd(inst->JJrshNegNegPtr, inst->JJgshunt);
#ifdef NEWLSH
            if (inst->JJlsh > 0.0) {
                double ival;
                if (ckt->CKTmode & MODEUIC)
                    ival =  0.0;
                else
                    ival = *(ckt->CKTrhsOld + inst->JJlshBr);
                *(ckt->CKTstate1 + inst->JJlshFlux) += inst->JJlsh * ival;
                *(ckt->CKTstate0 + inst->JJlshFlux) = 0;

                inst->JJlshReq = ckt->CKTag[0] * inst->JJlsh;
                inst->JJlshVeq = ckt->find_ceq(inst->JJlshFlux);

                ckt->rhsadd(inst->JJlshBr, inst->JJlshVeq);
                ckt->ldadd(inst->JJlshIbrIbrPtr, -inst->JJlshReq);
#ifndef USE_PRELOAD
                ckt->ldset(inst->JJlshPosIbrPtr, 1.0);
                ckt->ldset(inst->JJlshIbrPosPtr, 1.0);
                ckt->ldset(inst->JJlshNegIbrPtr, -1.0);
                ckt->ldset(inst->JJlshIbrNegPtr, -1.0);
#endif
            }
#endif
        }

#ifdef NEWLSER
        if (inst->JJlser > 0.0) {
            double ival;
            if (ckt->CKTmode & MODEUIC)
                ival =  0.0;
            else
                ival = *(ckt->CKTrhsOld + inst->JJlserBr);
            *(ckt->CKTstate1 + inst->JJlserFlux) += inst->JJlser * ival;
            *(ckt->CKTstate0 + inst->JJlserFlux) = 0;

            inst->JJlserReq = ckt->CKTag[0] * inst->JJlser;
            inst->JJlserVeq = ckt->find_ceq(inst->JJlserFlux);

            ckt->rhsadd(inst->JJlserBr, inst->JJlserVeq);
            ckt->ldadd(inst->JJlserIbrIbrPtr, -inst->JJlserReq);
#ifndef USE_PRELOAD
            if (inst->JJrealPosNode) {
                ckt->ldset(inst->JJlserPosIbrPtr, 1.0);
                ckt->ldset(inst->JJlserIbrPosPtr, 1.0);
            }
            if (inst->JJposNode) {
                ckt->ldset(inst->JJlserNegIbrPtr, -1.0);
                ckt->ldset(inst->JJlserIbrNegPtr, -1.0);
            }
#endif
        }
#endif
        return (OK);
    }
    return (E_BADPARM);
}


void
jjstuff::jj_limiting(sCKT *ckt, sJJmodel*, sJJinstance *inst)
{
    // limit new junction voltage
    double vprev  = *(ckt->CKTstate0 + inst->JJvoltage);
    double absold = vprev < 0 ? -vprev : vprev;
    double vtop = vprev + js_ddv;
    double vbot = vprev - js_ddv;
    if (absold < inst->JJvless) {
        vtop = SPMAX(vtop, inst->JJvless);
        vbot = SPMIN(vbot, -inst->JJvless);
    }
    else if (vprev > inst->JJvmore) {
        vtop += 1.0;
        vbot = SPMIN(vbot, inst->JJvmore);
    }
    else if (vprev < -inst->JJvmore) {
        vtop = SPMAX(vtop, -inst->JJvmore);
        vbot -= 1.0;
    }
    if (js_vj > vtop)
        js_vj = vtop;
    else if (js_vj < vbot)
        js_vj = vbot;
}


void
jjstuff::jj_iv(sJJmodel *model, sJJinstance *inst)
{
    double vj    = js_vj;
    double absvj = vj < 0 ? -vj : vj;
    if (model->JJrtype == 1) {
        if (absvj < inst->JJvless)
            js_gqt = inst->JJg0;
        else if (absvj < inst->JJvmore) {
            js_gqt = inst->JJgs;
            js_crhs = (vj >= 0 ? inst->JJcr1 : -inst->JJcr1);
        }
        else {
            js_gqt = inst->JJgn;
            js_crhs = (vj >= 0 ? inst->JJcr2 : -inst->JJcr2);
        }
    }
    else if (model->JJrtype == 2) {
        double dv = 0.5*model->JJdelv;
        double avj = absvj/dv;
        double vgdv = inst->JJvg/dv;
        double gam = avj - vgdv;
        if (gam > 30.0)
            gam = 30.0;
        else if (gam < -30.0)
            gam = -30.0;
        double xp = exp(gam);
        if (vgdv > 30.0)
            vgdv = 30.0;
        double xp0 = exp(-vgdv);
        double A = 0.5*(1.0 - model->JJicFactor)*inst->JJvg;
        double As = vj < 0.0 ? -A : A;
        double xp1 = 1 + xp - xp0;

        // Here is the assumed quasiparticle current.  This is
        // empirical with the properties that
        // 1) at vj = 0, conductance = g0, I = 0
        // 2) at vj = huge, conductance = gn

        double I = ( (inst->JJg0 + inst->JJgn*xp0*A/dv)*vj +
            inst->JJgn*(vj - As)*(xp - xp0) ) / xp1;

        double dIdv =
            (inst->JJg0 + inst->JJgn*xp0*A/dv)*(xp1 - absvj*xp)/(xp1*xp1) +
            inst->JJgn*(xp*(absvj - A)/(dv*xp1*xp1) + (xp - xp0)/xp1) ;

        js_crhs = I - dIdv*vj;
        js_gqt = dIdv;
    }
    else if (model->JJrtype == 3) {
        if (absvj < inst->JJvmore) {
            // cj   = g0*vj + g1*vj**3 + g2*vj**5,
            // crhs = cj - vj*gqt
            //
            double temp1 = vj*vj;
            double temp2 = temp1*temp1;
            js_gqt = inst->JJg0 + 3.0*inst->JJg1*temp1 +
                5.0*inst->JJg2*temp2;
            temp1  *= vj;
            temp2  *= vj;
            js_crhs = -2.0*temp1*inst->JJg1 - 4.0*temp2*inst->JJg2;
        }
        else {
            js_gqt = inst->JJgn;
            js_crhs = (vj >= 0 ? inst->JJcr1 : -inst->JJcr1);
        }
    }
    else if (model->JJrtype == 4) {
        double temp = 1;
        if (inst->JJcontrol)
            temp = js_ci < 0 ? -js_ci : js_ci;
        if (temp > 1)
            temp = 1;
        js_crt *= temp;
        double gs = inst->JJgs*temp;
        if (gs < inst->JJgn)
            gs = inst->JJgn;
        double vg = inst->JJvg*temp;
        temp = .5*model->JJdelv;
        double vless = vg - temp;
        double vmore = vg + temp;

        if (vless > 0) {
            if (absvj < vless)
                js_gqt = inst->JJg0;
            else if (absvj < vmore) {
                js_gqt = gs;
                if (vj >= 0)
                    js_crhs  = (inst->JJg0 - js_gqt)*vless;
                else
                    js_crhs  = (js_gqt - inst->JJg0)*vless;
            }
            else {
                js_gqt = inst->JJgn;
                double cr2 = js_crt/model->JJicFactor +
                    vless*inst->JJg0 - vmore*inst->JJgn;
                js_crhs = (vj >= 0 ? cr2 : -cr2);
            }
        }
        else
            js_gqt = inst->JJgn;

        if (model->JJictype > 1)
            model->JJictype = 0;
    }
    else
        js_gqt = 0;
}


// Non-default supercurrent
// For shaped junction types, parameters scale with
// area as in small junctions, control current does not scale.
//
void
jjstuff::jj_ic(sJJmodel *model, sJJinstance *inst)
{
    (void)inst;
    double ci = js_ci;

    if (model->JJictype == 2) {

        if (ci != 0.0) {
            double xx = js_crt;
            double ang  = M_PI * ci / model->JJccsens;
            js_crt *= sin(ang)/ang;
            js_dcrt = xx*(cos(ang) - js_crt)/ci;
        }
    }
    else if (model->JJictype == 3) {
        double temp = ci < 0 ? -ci : ci;
        if (temp < model->JJccsens) {
            js_dcrt = js_crt / model->JJccsens;
            js_crt *= (1.0 - temp/model->JJccsens);
            if (ci > 0.0)
                js_dcrt = -js_dcrt;
            if (ci == 0.0)
                js_dcrt = 0.0;
        }
        else js_crt = 0.0;
    }
    else if (model->JJictype == 4) {
        double temp = ci < 0 ? -ci : ci;
        if (temp < model->JJccsens) {
            temp = model->JJccsens + model->JJccsens;
            js_dcrt = -js_crt / temp;
            js_crt *= (model->JJccsens - ci)/temp;
            if (ci == 0.0)
                js_dcrt = 0.0;
        }
        else
            js_crt = 0.0;
    }
    else
        js_crt = 0.0;
}


void
jjstuff::jj_load(sCKT *ckt, sJJmodel *model, sJJinstance *inst)
{
    (void)model;
    double gqt  = js_gqt;
    double crhs = js_crhs;
    double crt  = js_crt;

    *(ckt->CKTstate0 + inst->JJvoltage) = js_vj;
    *(ckt->CKTstate0 + inst->JJphase)   = js_phi;
    *(ckt->CKTstate0 + inst->JJconI)    = js_ci;
    // these two for JJask()
    *(ckt->CKTstate0 + inst->JJcrti)    = js_crt;
#ifdef NEWJJDC
    if (ckt->CKTmode & MODEDC)
        *(ckt->CKTstate0 + inst->JJqpi) = 0.0;
    else
#endif
    *(ckt->CKTstate0 + inst->JJqpi)     = crhs + gqt*js_vj;

#ifdef ASM_SINCOS
    double si, sctemp;
    asm("fsincos" : "=t" (sctemp), "=u"  (si) : "0" (js_phi));
    double gcs   = js_pfac*crt*sctemp;
#else
    double gcs   = js_pfac*crt*cos(js_phi);
    double si    = sin(js_phi);
#endif

    if (model->JJpi) {
        si = -si;
        gcs = -gcs;
    }
    crt  *= si;
#ifdef NEWJJDC
    if (ckt->CKTmode & MODEDC) {
        crhs += crt - gcs*js_phi;
        gqt  = gcs;
    }
    else {
        crhs += crt - gcs*js_vj;
        gqt  += gcs + ckt->CKTag[0]*inst->JJcap;
        crhs += inst->JJdelVdelT*inst->JJcap;
    }
#else
    crhs += crt - gcs*js_vj;
    gqt  += gcs + ckt->CKTag[0]*inst->JJcap;
    crhs += inst->JJdelVdelT*inst->JJcap;
#endif

    // load matrix, rhs vector
    if (inst->JJphsNode > 0)
        *(ckt->CKTrhs + inst->JJphsNode) = js_phi +
            (4*M_PI)* *(int*)(ckt->CKTstate1 + inst->JJphsInt);

    if (!inst->JJnegNode) {
        ckt->ldadd(inst->JJposPosPtr, gqt);
        if (inst->JJcontrol) {
            double temp = js_dcrt*si;
            ckt->ldadd(inst->JJposIbrPtr, temp);
            crhs -= temp*js_ci;
        }
        ckt->rhsadd(inst->JJposNode, -crhs);
    }
    else if (!inst->JJposNode) {
        ckt->ldadd(inst->JJnegNegPtr, gqt);
        if (inst->JJcontrol) {
            double temp = js_dcrt*si;
            ckt->ldadd(inst->JJnegIbrPtr, -temp);
            crhs -= temp*js_ci;
        }
        ckt->rhsadd(inst->JJnegNode, crhs);
    }
    else {
        ckt->ldadd(inst->JJposPosPtr, gqt);
        ckt->ldadd(inst->JJposNegPtr, -gqt);
        ckt->ldadd(inst->JJnegPosPtr, -gqt);
        ckt->ldadd(inst->JJnegNegPtr, gqt);
        if (inst->JJcontrol) {
            double temp = js_dcrt*si;
            ckt->ldadd(inst->JJposIbrPtr, temp);
            ckt->ldadd(inst->JJnegIbrPtr, -temp);
            crhs -= temp*js_ci;
        }
        ckt->rhsadd(inst->JJposNode, -crhs);
        ckt->rhsadd(inst->JJnegNode, crhs);
    }
#ifndef USE_PRELOAD
    if (inst->JJphsNode > 0)
        ckt->ldset(inst->JJphsPhsPtr, 1.0);
#endif
}

