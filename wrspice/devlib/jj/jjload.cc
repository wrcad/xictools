
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

    struct jjstuff js;

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

        if (maxvj >= model->JJvless) {
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

        ckt->integrate(inst->JJvoltage, inst->JJdelVdelT);
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
        return (OK);
    }

    if (ckt->CKTmode & MODEINITTRAN) {
        if (ckt->CKTmode & MODEUIC) {
            js.js_vj = *(ckt->CKTstate1 + inst->JJvoltage);
            js.js_phi = *(ckt->CKTstate1 + inst->JJphase);
            js.js_ci = *(ckt->CKTstate1 + inst->JJconI);
        }
        else {
            js.js_vj  = 0;
            js.js_phi = 0;
            js.js_ci  = 0;
        }
    }
    else {
        if (ckt->CKTmode & MODEUIC) {
            js.js_vj  = inst->JJinitVoltage;
            js.js_phi = inst->JJinitPhase;
            js.js_ci  = inst->JJinitControl;
        }
        else {
            js.js_vj  = 0;
            js.js_phi = 0;
            js.js_ci  = 0;
        }

        *(ckt->CKTstate1 + inst->JJvoltage) = js.js_vj;
        *(ckt->CKTstate1 + inst->JJphase)   = js.js_phi;
        *(ckt->CKTstate1 + inst->JJconI)    = js.js_ci;
    }

    inst->JJdelVdelT = ckt->find_ceq(inst->JJvoltage);

    js.js_crhs = 0;
    js.js_dcrt = 0;
    js.js_crt  = inst->JJcriti;

    js.jj_iv(model, inst);
    if (model->JJictype != 1)
        js.jj_ic(model, inst);
    js.jj_load(ckt, model, inst);
    return (OK);
}


void
jjstuff::jj_limiting(sCKT *ckt, sJJmodel *model, sJJinstance *inst)
{
    // limit new junction voltage
    double vprev  = *(ckt->CKTstate0 + inst->JJvoltage);
    double absold = vprev < 0 ? -vprev : vprev;
    double vtop = vprev + js_ddv;
    double vbot = vprev - js_ddv;
    if (absold < model->JJvless) {
        vtop = SPMAX(vtop, model->JJvless);
        vbot = SPMIN(vbot, -model->JJvless);
    }
    else if (vprev > model->JJvmore) {
        vtop += 1.0;
        vbot = SPMIN(vbot, model->JJvmore);
    }
    else if (vprev < -model->JJvmore) {
        vtop = SPMAX(vtop, -model->JJvmore);
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
        if (absvj < model->JJvless)
            js_gqt = inst->JJg0;
        else if (absvj < model->JJvmore) {
            js_gqt = inst->JJgs;
            js_crhs = (vj >= 0 ? inst->JJcr1 : -inst->JJcr1);
        }
        else {
            js_gqt = inst->JJgn;
            js_crhs = (vj >= 0 ? inst->JJcr2 : -inst->JJcr2);
        }
    }
    else if (model->JJrtype == 3) {

        if (absvj < model->JJvmore) {
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
    else if (model->JJrtype == 2) {
        double dv = 0.5*model->JJdelv;
        double avj = absvj/dv;
        double gam = avj - model->JJvg/dv;
        if (gam > 30.0)
            gam = 30.0;
        if (gam < -30.0)
            gam = -30.0;
        double expgam = exp(gam);
        double exngam = 1.0 / expgam;
        double xp     = 1.0 + expgam;
        double xn     = 1.0 + exngam;
        double cxtra  =
            (1.0 - model->JJicFactor)*model->JJvg*inst->JJgn*expgam/xp;
        js_crhs = vj*(inst->JJg0 + inst->JJgn*expgam)/xp -
            (vj >= 0 ? cxtra : -cxtra);
        js_gqt = inst->JJgn*(xn + avj*exngam)/(xn*xn) +
            inst->JJg0*(xp - avj*expgam)/(xp*xp);
        js_crhs -= js_gqt*vj;
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
        double vg = model->JJvg*temp;
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
            else
                js_gqt = inst->JJgn;
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
            double ang, xx = js_crt;
            ang  = M_PI * ci / model->JJccsens;
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
    *(ckt->CKTstate0 + inst->JJqpi)     = crhs + gqt*js_vj;

#ifdef ASM_SINCOS
    double si, sctemp;
    asm("fsincos" : "=t" (sctemp), "=u"  (si) : "0" (js_phi));
    double gcs   = js_pfac*crt*sctemp;
#else
    double gcs   = js_pfac*crt*cos(js_phi);
    double si    = sin(js_phi);
#endif

    if (inst->JJpi) {
        si = -si;
        gcs = -gcs;
    }
    crt  *= si;
    crhs += crt - gcs*js_vj;
    gqt  += gcs + ckt->CKTag[0]*inst->JJcap;
    crhs += inst->JJdelVdelT*inst->JJcap;

    if (model->JJvShuntGiven)
        gqt += inst->JJcriti/model->JJvShunt;

    // load matrix, rhs vector
    if (inst->JJphsNode > 0)
        *(ckt->CKTrhs + inst->JJphsNode) = js_phi +
            (2*M_PI)* *(int*)(ckt->CKTstate1 + inst->JJphsInt);

    if (!inst->JJnegNode) {
        ckt->ldadd(inst->JJposPosPtr, gqt);
        if (!inst->JJcontrol)
            ckt->rhsadd(inst->JJposNode, -crhs);
        else {
            double temp = js_dcrt*si;
            ckt->ldadd(inst->JJposIbrPtr, temp);
            crhs -= temp*js_ci;
            ckt->rhsadd(inst->JJposNode, -crhs);
        }
    }
    else if (!inst->JJposNode) {
        ckt->ldadd(inst->JJnegNegPtr, gqt);
        if (!inst->JJcontrol)
            ckt->rhsadd(inst->JJnegNode, crhs);
        else {
            double temp = js_dcrt*si;
            ckt->ldadd(inst->JJnegIbrPtr, -temp);
            crhs -= temp*js_ci;
            ckt->rhsadd(inst->JJnegNode, crhs);
        }
    }
    else {
        ckt->ldadd(inst->JJposPosPtr, gqt);
        ckt->ldadd(inst->JJposNegPtr, -gqt);
        ckt->ldadd(inst->JJnegPosPtr, -gqt);
        ckt->ldadd(inst->JJnegNegPtr, gqt);
        if (!inst->JJcontrol) {
            ckt->rhsadd(inst->JJposNode, -crhs);
            ckt->rhsadd(inst->JJnegNode, crhs);
        }
        else {
            double temp = js_dcrt*si;
            ckt->ldadd(inst->JJposIbrPtr, temp);
            ckt->ldadd(inst->JJnegIbrPtr, -temp);
            crhs -= temp*js_ci;
            ckt->rhsadd(inst->JJposNode, -crhs);
            ckt->rhsadd(inst->JJnegNode, crhs);
        }
    }
#ifndef USE_PRELOAD
    if (inst->JJphsNode > 0)
        ckt->ldset(inst->JJphsPhsPtr, 1.0);
#endif
}

