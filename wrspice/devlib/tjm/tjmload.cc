
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2019 Whiteley Research Inc., all rights reserved.       *
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

#include "tjmdefs.h"

//#define TJM_DEBUG
#ifdef TJM_DEBUG
#include <stdio.h>
#endif
//XXX
#include <stdio.h>


#if defined(__GNUC__) && (defined(i386) || defined(__x86_64__))
#define ASM_SINCOS
#endif

struct tjmstuff
{
    void tjm_iv(sTJMmodel*, sTJMinstance*);
    void tjm_ic(sTJMmodel*, sTJMinstance*);
    void tjm_load(sCKT*, sTJMmodel*, sTJMinstance*);

    double ts_vj;
    double ts_phi;
    double ts_ci;
    double ts_gqt;
    double ts_crhs;
    double ts_crt;
    double ts_dcrt;
    double ts_pfac;
    double ts_ddv;
};


int
TJMdev::load(sGENinstance *in_inst, sCKT *ckt)
{
    sTJMinstance *inst = (sTJMinstance*)in_inst;
    sTJMmodel *model = (sTJMmodel*)inst->GENmodPtr;

    tjmstuff ts;

#ifdef NEWJJDC

    if (ckt->CKTmode & MODEDC) {

        ts.ts_ci  = (inst->TJMcontrol) ?
                *(ckt->CKTrhsOld + inst->TJMbranch) : 0;

        ts.ts_vj  = *(ckt->CKTrhsOld + inst->TJMposNode) -
                *(ckt->CKTrhsOld + inst->TJMnegNode);

        if (model->TJMictype != 0) {
            ts.ts_phi = ts.ts_vj;
            *(ckt->CKTstate0 + inst->TJMphase) = ts.ts_phi;
            *(ckt->CKTstate0 + inst->TJMvoltage) = 0.0;

            ts.ts_pfac = 1.0;
            ts.ts_gqt = 0;
            ts.ts_crhs = 0;
            ts.ts_dcrt = 0;
            ts.ts_crt  = inst->TJMcriti;
            if (model->TJMictype != 1)
                ts.tjm_ic(model, inst);
            if (ckt->CKTmode & MODEINITSMSIG) {
                // We don't want/need this except when setting up for AC
                // analysis.
                ts.tjm_iv(model, inst);
            }
            ts.tjm_load(ckt, model, inst);
            // don't load shunt
#ifdef NEWLSH
            if (model->TJMvShuntGiven && inst->TJMgshunt > 0.0) {
                // Load lsh as 0-voltage source, don't load resistor. 
                // Dangling voltage source shouldn't matter.

                if (inst->TJMlsh > 0.0) {
                    ckt->ldadd(inst->TJMlshIbrIbrPtr, 0.0);
#ifndef USE_PRELOAD
                    ckt->ldset(inst->TJMlshPosIbrPtr, 1.0);
                    ckt->ldset(inst->TJMlshIbrPosPtr, 1.0);
                    ckt->ldset(inst->TJMlshNegIbrPtr, -1.0);
                    ckt->ldset(inst->TJMlshIbrNegPtr, -1.0);
#endif
                }
            }
#endif
        }
        else {
            // No critical current, treat like a nonlinear resistor.

            ts.ts_phi = 0.0;
            ts.ts_crhs = 0.0;
            ts.ts_dcrt = 0.0;
            ts.ts_crt  = 0.0;
        
            ts.tjm_iv(model, inst);
            ts.tjm_load(ckt, model, inst);

            // Load the shunt resistance implied if vshunt given.
            if (model->TJMvShuntGiven && inst->TJMgshunt > 0.0) {
                ckt->ldadd(inst->TJMrshPosPosPtr, inst->TJMgshunt);
                ckt->ldadd(inst->TJMrshPosNegPtr, -inst->TJMgshunt);
                ckt->ldadd(inst->TJMrshNegPosPtr, -inst->TJMgshunt);
                ckt->ldadd(inst->TJMrshNegNegPtr, inst->TJMgshunt);
#ifdef NEWLSH
                // Load lsh as 0-voltage source.
                if (inst->TJMlsh > 0.0) {
                    ckt->ldadd(inst->TJMlshIbrIbrPtr, 0.0);
#ifndef USE_PRELOAD
                    ckt->ldset(inst->TJMlshPosIbrPtr, 1.0);
                    ckt->ldset(inst->TJMlshIbrPosPtr, 1.0);
                    ckt->ldset(inst->TJMlshNegIbrPtr, -1.0);
                    ckt->ldset(inst->TJMlshIbrNegPtr, -1.0);
#endif
                }
#endif
            }
        }
        if (ckt->CKTmode & MODEINITSMSIG) {
            // for ac load
            inst->TJMdcrt = ts.ts_dcrt;
        }
#ifdef NEWLSER
        if (inst->TJMlser > 0.0) {
            double res = 2*M_PI*inst->TJMlser/wrsCONSTphi0;
            ckt->ldadd(inst->TJMlserIbrIbrPtr, -res);
        }
#ifndef USE_PRELOAD
        if (inst->TJMrealPosNode) {
            ckt->ldset(inst->TJMlserPosIbrPtr, 1.0);
            ckt->ldset(inst->TJMlserIbrPosPtr, 1.0);
        }
        if (inst->TJMposNode) {
            ckt->ldset(inst->TJMlserNegIbrPtr, -1.0);
            ckt->ldset(inst->TJMlserIbrNegPtr, -1.0);
        }
#endif
#endif
        return (OK);
    }
#else
    // May want to get rid of this, or possibly add as an option.

    if ((ckt->CKTmode & MODEDC) && !(ckt->CKTmode & MODEUIC)) {
        double g = 1e6 * inst->TJMcriti;   // Ic/1uV
        ckt->ldadd(inst->TJMposPosPtr, g);
        ckt->ldadd(inst->TJMposNegPtr, -g);
        ckt->ldadd(inst->TJMnegPosPtr, -g);
        ckt->ldadd(inst->TJMnegNegPtr, g);
        return (OK);
    }
#endif

    ts.ts_pfac = ckt->CKTdelta / PHI0_2PI;
    if (ckt->CKTorder > 1)
        ts.ts_pfac *= .5;

    if (ckt->CKTmode & MODEINITFLOAT) {
        ts.ts_ci  = (inst->TJMcontrol) ?
                *(ckt->CKTrhsOld + inst->TJMbranch) : 0;

        ts.ts_vj  = *(ckt->CKTrhsOld + inst->TJMposNode) -
                *(ckt->CKTrhsOld + inst->TJMnegNode);

        double absvj  = fabs(ts.ts_vj);
        double temp   = *(ckt->CKTstate0 + inst->TJMvoltage);
        double absold = fabs(temp);
        double maxvj  = SPMAX(absvj,absold);
        temp  -= ts.ts_vj;
        double absdvj = fabs(temp);

/* XXX
        if (maxvj >= model->TJMvless) {
            temp = model->TJMdelv * 0.5;
            if (absdvj > temp) {
                ts.ts_ddv = temp;
                ts.tjm_limiting(ckt, model, inst);
                absvj  = fabs(ts.ts_vj);
                maxvj  = SPMAX(absvj, absold);
                temp   = ts.ts_vj - *(ckt->CKTstate0 + inst->TJMvoltage);
                absdvj = fabs(temp);
            }
        }
*/

        // check convergence

        if (!ckt->CKTnoncon) {
            double tol = ckt->CKTcurTask->TSKreltol*maxvj +
                ckt->CKTcurTask->TSKabstol;
            if (absdvj > tol) {
#ifdef TJM_DEBUG
                printf("%s %g %g %g %g\n", (const char*)inst->GENname,
                    maxvj, absdvj, ts.ts_vj,
                    *(ckt->CKTstate0 + inst->TJMvoltage));
#endif
                ckt->incNoncon();  // SRW
            }
        }

        ts.ts_phi = *(ckt->CKTstate1 + inst->TJMphase);
        temp = ts.ts_vj;
        if (ckt->CKTorder > 1)
            temp += *(ckt->CKTstate1 + inst->TJMvoltage);
        ts.ts_phi += ts.ts_pfac*temp;

        ts.ts_crhs = 0;
        ts.ts_dcrt = 0;
        ts.ts_crt  = inst->TJMcriti;
    
        ckt->integrate(inst->TJMvoltage, inst->TJMdelVdelT);
        //
        // compute quasiparticle current and derivatives
        //
        ts.tjm_iv(model, inst);
        if (model->TJMictype != 1)
            ts.tjm_ic(model, inst);
        ts.tjm_load(ckt, model, inst);

        // Load the shunt resistance implied if vshunt given.
        if (model->TJMvShuntGiven && inst->TJMgshunt > 0.0) {
            ckt->ldadd(inst->TJMrshPosPosPtr, inst->TJMgshunt);
            ckt->ldadd(inst->TJMrshPosNegPtr, -inst->TJMgshunt);
            ckt->ldadd(inst->TJMrshNegPosPtr, -inst->TJMgshunt);
            ckt->ldadd(inst->TJMrshNegNegPtr, inst->TJMgshunt);
#ifdef NEWLSH
            if (inst->TJMlsh > 0.0) {
                *(ckt->CKTstate0 + inst->TJMlshFlux) = inst->TJMlsh *
                    *(ckt->CKTrhsOld + inst->TJMlshBr);
                ckt->integrate(inst->TJMlshFlux, inst->TJMlshVeq);

                ckt->rhsadd(inst->TJMlshBr, inst->TJMlshVeq);
                ckt->ldadd(inst->TJMlshIbrIbrPtr, -inst->TJMlshReq);
#ifndef USE_PRELOAD
                ckt->ldset(inst->TJMlshPosIbrPtr, 1.0);
                ckt->ldset(inst->TJMlshIbrPosPtr, 1.0);
                ckt->ldset(inst->TJMlshNegIbrPtr, -1.0);
                ckt->ldset(inst->TJMlshIbrNegPtr, -1.0);
#endif
            }
#endif
        }

//        ckt->integrate(inst->TJMvoltage, inst->TJMdelVdelT);
#ifdef NEWLSER
        if (inst->TJMlser > 0.0) {
            *(ckt->CKTstate0 + inst->TJMlserFlux) = inst->TJMlser *
                *(ckt->CKTrhsOld + inst->TJMlserBr);
            ckt->integrate(inst->TJMlserFlux, inst->TJMlserVeq);

            ckt->rhsadd(inst->TJMlserBr, inst->TJMlserVeq);
            ckt->ldadd(inst->TJMlserIbrIbrPtr, -inst->TJMlserReq);
#ifndef USE_PRELOAD
            if (inst->TJMrealPosNode) {
                ckt->ldset(inst->TJMlserPosIbrPtr, 1.0);
                ckt->ldset(inst->TJMlserIbrPosPtr, 1.0);
            }
            if (inst->TJMposNode) {
                ckt->ldset(inst->TJMlserNegIbrPtr, -1.0);
                ckt->ldset(inst->TJMlserIbrNegPtr, -1.0);
            }
#endif
        }
#endif
        return (OK);
    }

    if (ckt->CKTmode & MODEINITPRED) {

        double y0 = DEV.pred(ckt, inst->TJMdvdt);
        if (ckt->CKTorder != 1)
            y0 += *(ckt->CKTstate1 + inst->TJMdvdt);

        double temp = *(ckt->CKTstate1 + inst->TJMvoltage);
        double rag0  = ckt->CKTorder == 1 ? ckt->CKTdelta : .5*ckt->CKTdelta;
        ts.ts_vj  = temp + rag0*y0;

        ts.ts_phi = *(ckt->CKTstate1 + inst->TJMphase);
        temp = ts.ts_vj;
        if (ckt->CKTorder > 1)
            temp += *(ckt->CKTstate1 + inst->TJMvoltage);
        ts.ts_phi += ts.ts_pfac*temp;

        if (inst->TJMcontrol)
            ts.ts_ci = DEV.pred(ckt, inst->TJMconI);
        else
            ts.ts_ci = 0;

        inst->TJMdelVdelT = ckt->find_ceq(inst->TJMvoltage);

        inst->tjm_newstep(ckt);

        ts.ts_crhs = 0;
        ts.ts_dcrt = 0;
        ts.ts_crt  = inst->TJMcriti;

        ts.tjm_iv(model, inst);
        if (model->TJMictype != 1)
            ts.tjm_ic(model, inst);
        ts.tjm_load(ckt, model, inst);

        // Load the shunt resistance implied if vshunt given.
        if (model->TJMvShuntGiven && inst->TJMgshunt > 0.0) {
            ckt->ldadd(inst->TJMrshPosPosPtr, inst->TJMgshunt);
            ckt->ldadd(inst->TJMrshPosNegPtr, -inst->TJMgshunt);
            ckt->ldadd(inst->TJMrshNegPosPtr, -inst->TJMgshunt);
            ckt->ldadd(inst->TJMrshNegNegPtr, inst->TJMgshunt);
#ifdef NEWLSH
            if (inst->TJMlsh > 0.0) {
                inst->TJMlshReq = ckt->CKTag[0] * inst->TJMlsh;
                inst->TJMlshVeq = ckt->find_ceq(inst->TJMlshFlux);

                ckt->rhsadd(inst->TJMlshBr, inst->TJMlshVeq);
                ckt->ldadd(inst->TJMlshIbrIbrPtr, -inst->TJMlshReq);

                *(ckt->CKTstate0 + inst->TJMlshFlux) = 0;
#ifndef USE_PRELOAD
                ckt->ldset(inst->TJMlshPosIbrPtr, 1.0);
                ckt->ldset(inst->TJMlshIbrPosPtr, 1.0);
                ckt->ldset(inst->TJMlshNegIbrPtr, -1.0);
                ckt->ldset(inst->TJMlshIbrNegPtr, -1.0);
#endif
            }
#endif
        }
#ifdef NEWLSER
        if (inst->TJMlser > 0.0) {
            inst->TJMlserReq = ckt->CKTag[0] * inst->TJMlser;
            inst->TJMlserVeq = ckt->find_ceq(inst->TJMlserFlux);

            ckt->rhsadd(inst->TJMlserBr, inst->TJMlserVeq);
            ckt->ldadd(inst->TJMlserIbrIbrPtr, -inst->TJMlserReq);

            *(ckt->CKTstate0 + inst->TJMlserFlux) = 0;
#ifndef USE_PRELOAD
            if (inst->TJMrealPosNode) {
                ckt->ldset(inst->TJMlserPosIbrPtr, 1.0);
                ckt->ldset(inst->TJMlserIbrPosPtr, 1.0);
            }
            if (inst->TJMposNode) {
                ckt->ldset(inst->TJMlserNegIbrPtr, -1.0);
                ckt->ldset(inst->TJMlserIbrNegPtr, -1.0);
            }
#endif
        }
#endif
        return (OK);
    }

    if (ckt->CKTmode & MODEINITTRAN) {
        if (ckt->CKTmode & MODEUIC) {
            ts.ts_vj  = inst->TJMinitVoltage;
            ts.ts_phi = inst->TJMinitPhase;
            ts.ts_ci  = inst->TJMinitControl;
            *(ckt->CKTstate1 + inst->TJMvoltage) = ts.ts_vj;
            *(ckt->CKTstate1 + inst->TJMphase) = ts.ts_phi;
            *(ckt->CKTstate1 + inst->TJMconI) = ts.ts_ci;
        }
        else {
            ts.ts_vj  = 0;
#ifdef NEWJJDC
            *(ckt->CKTstate1 + inst->TJMvoltage) = 0.0;
            // The RHS node voltage was set to zero in the doTask code.
            ts.ts_phi = *(ckt->CKTstate1 + inst->TJMphase);
            ts.ts_ci  = (inst->TJMcontrol) ?
                *(ckt->CKTrhsOld + inst->TJMbranch) : 0;
            *(ckt->CKTstate1 + inst->TJMconI) = ts.ts_ci;
            *(ckt->CKTstate0 + inst->TJMvoltage) = 0.0;
#else
            ts.ts_phi = 0;
            ts.ts_ci  = 0;  // This isn't right?
#endif
        }

        inst->TJMdelVdelT = ckt->find_ceq(inst->TJMvoltage);
        inst->tjm_init(ts.ts_phi);

        ts.ts_crhs = 0;
        ts.ts_dcrt = 0;
        ts.ts_crt  = inst->TJMcriti;

        ts.tjm_iv(model, inst);
        if (model->TJMictype != 1)
            ts.tjm_ic(model, inst);
        ts.tjm_load(ckt, model, inst);

        if (model->TJMvShuntGiven && inst->TJMgshunt > 0.0) {
            ckt->ldadd(inst->TJMrshPosPosPtr, inst->TJMgshunt);
            ckt->ldadd(inst->TJMrshPosNegPtr, -inst->TJMgshunt);
            ckt->ldadd(inst->TJMrshNegPosPtr, -inst->TJMgshunt);
            ckt->ldadd(inst->TJMrshNegNegPtr, inst->TJMgshunt);
#ifdef NEWLSH
            if (inst->TJMlsh > 0.0) {
                double ival;
                if (ckt->CKTmode & MODEUIC)
                    ival =  0.0;
                else
                    ival = *(ckt->CKTrhsOld + inst->TJMlshBr);
                *(ckt->CKTstate1 + inst->TJMlshFlux) += inst->TJMlsh * ival;
                *(ckt->CKTstate0 + inst->TJMlshFlux) = 0;

                inst->TJMlshReq = ckt->CKTag[0] * inst->TJMlsh;
                inst->TJMlshVeq = ckt->find_ceq(inst->TJMlshFlux);

                ckt->rhsadd(inst->TJMlshBr, inst->TJMlshVeq);
                ckt->ldadd(inst->TJMlshIbrIbrPtr, -inst->TJMlshReq);
#ifndef USE_PRELOAD
                ckt->ldset(inst->TJMlshPosIbrPtr, 1.0);
                ckt->ldset(inst->TJMlshIbrPosPtr, 1.0);
                ckt->ldset(inst->TJMlshNegIbrPtr, -1.0);
                ckt->ldset(inst->TJMlshIbrNegPtr, -1.0);
#endif
            }
#endif
        }

#ifdef NEWLSER
        if (inst->TJMlser > 0.0) {
            double ival;
            if (ckt->CKTmode & MODEUIC)
                ival =  0.0;
            else
                ival = *(ckt->CKTrhsOld + inst->TJMlserBr);
            *(ckt->CKTstate1 + inst->TJMlserFlux) += inst->TJMlser * ival;
            *(ckt->CKTstate0 + inst->TJMlserFlux) = 0;

            inst->TJMlserReq = ckt->CKTag[0] * inst->TJMlser;
            inst->TJMlserVeq = ckt->find_ceq(inst->TJMlserFlux);

            ckt->rhsadd(inst->TJMlserBr, inst->TJMlserVeq);
            ckt->ldadd(inst->TJMlserIbrIbrPtr, -inst->TJMlserReq);
#ifndef USE_PRELOAD
            if (inst->TJMrealPosNode) {
                ckt->ldset(inst->TJMlserPosIbrPtr, 1.0);
                ckt->ldset(inst->TJMlserIbrPosPtr, 1.0);
            }
            if (inst->TJMposNode) {
                ckt->ldset(inst->TJMlserNegIbrPtr, -1.0);
                ckt->ldset(inst->TJMlserIbrNegPtr, -1.0);
            }
#endif
        }
#endif
        return (OK);
    }

    return (E_BADPARM);
}


// Static function.
// Return the quasiparticle conductance at zero voltage.
//
double
sTJMmodel::subgap(sTJMmodel *model, sTJMinstance *inst)
{
    tjmstuff ts;
    memset(&ts, 0, sizeof(tjmstuff));

    ts.tjm_iv(model, inst);
    return (ts.ts_gqt);
}


void
tjmstuff::tjm_iv(sTJMmodel *model, sTJMinstance *inst)
{
    double vj    = ts_vj;
    double absvj = vj < 0 ? -vj : vj;
    if (absvj < model->TJMvless)
        ts_gqt = inst->TJMg0;
    else if (absvj < model->TJMvmore) {
        ts_gqt = inst->TJMgs;
        ts_crhs = (vj >= 0 ? inst->TJMcr1 : -inst->TJMcr1);
    }
    else {
        ts_gqt = inst->TJMgn;
        ts_crhs = (vj >= 0 ? inst->TJMcr2 : -inst->TJMcr2);
    }
}


// Non-default supercurrent
// For shaped junction types, parameters scale with
// area as in small junctions, control current does not scale.
//
void
tjmstuff::tjm_ic(sTJMmodel *model, sTJMinstance *inst)
{
    (void)inst;
    double ci = ts_ci;

    if (model->TJMictype == 2) {

        if (ci != 0.0) {
            double xx = ts_crt;
            double ang  = M_PI * ci / model->TJMccsens;
            ts_crt *= sin(ang)/ang;
            ts_dcrt = xx*(cos(ang) - ts_crt)/ci;
        }
    }
    else if (model->TJMictype == 3) {

        double temp = ci < 0 ? -ci : ci;
        if (temp < model->TJMccsens) {
            ts_dcrt = ts_crt / model->TJMccsens;
            ts_crt *= (1.0 - temp/model->TJMccsens);
            if (ci > 0.0)
                ts_dcrt = -ts_dcrt;
            if (ci == 0.0)
                ts_dcrt = 0.0;
        }
        else ts_crt = 0.0;
    }
    else if (model->TJMictype == 4) {

        double temp = ci < 0 ? -ci : ci;
        if (temp < model->TJMccsens) {
            temp = model->TJMccsens + model->TJMccsens;
            ts_dcrt = -ts_crt / temp;
            ts_crt *= (model->TJMccsens - ci)/temp;
            if (ci == 0.0)
                ts_dcrt = 0.0;
        }
        else
            ts_crt = 0.0;
    }
    else
        ts_crt = 0.0;
}


namespace {
    inline void sincos(double a, double &si, double &ci)
    {
#ifdef ASM_SINCOS
        asm("fsincos" : "=t" (ci), "=u"  (si) : "0" (a));
#else
        si = sin(a);
        ci = cos(a);
#endif
    }
}


void
tjmstuff::tjm_load(sCKT *ckt, sTJMmodel *model, sTJMinstance *inst)
{
    (void)model;
    double gqt  = ts_gqt;
    double crhs = ts_crhs;

    *(ckt->CKTstate0 + inst->TJMvoltage) = ts_vj;
    *(ckt->CKTstate0 + inst->TJMphase)   = ts_phi;
    *(ckt->CKTstate0 + inst->TJMconI)    = ts_ci;
    // these two for TJMask()
    *(ckt->CKTstate0 + inst->TJMcrti)    = ts_crt;
    *(ckt->CKTstate0 + inst->TJMqpi)     = crhs + gqt*ts_vj;

#ifdef notdefXXX
#ifdef ASM_SINCOS
    double si, sctemp;
    asm("fsincos" : "=t" (sctemp), "=u"  (si) : "0" (ts_phi));
    double gcs   = ts_pfac*crt*sctemp;
#else
    double gcs   = ts_pfac*crt*cos(ts_phi);
    double si    = sin(ts_phi);
#endif

#else
    double si    = sin(ts_phi);
#endif

    double curr;
    inst->tjm_update(ts_phi, &curr);
    gqt = 0.9*inst->TJMgn/model->TJMicFactor + ckt->CKTag[0]*inst->TJMcap;

    double ccap = inst->TJMdelVdelT*inst->TJMcap;
    crhs = ts_crt*curr + ccap;

    // load matrix, rhs vector
    if (inst->TJMphsNode > 0)
        *(ckt->CKTrhs + inst->TJMphsNode) = ts_phi +
            (2*M_PI)* *(int*)(ckt->CKTstate1 + inst->TJMphsInt);

    if (!inst->TJMnegNode) {
        ckt->ldadd(inst->TJMposPosPtr, gqt);
        if (inst->TJMcontrol) {
            double temp = ts_dcrt*si;
            ckt->ldadd(inst->TJMposIbrPtr, temp);
            crhs -= temp*ts_ci;
        }
        ckt->rhsadd(inst->TJMposNode, -crhs);
    }
    else if (!inst->TJMposNode) {
        ckt->ldadd(inst->TJMnegNegPtr, gqt);
        if (inst->TJMcontrol) {
            double temp = ts_dcrt*si;
            ckt->ldadd(inst->TJMnegIbrPtr, -temp);
            crhs -= temp*ts_ci;
        }
        ckt->rhsadd(inst->TJMnegNode, crhs);
    }
    else {
        ckt->ldadd(inst->TJMposPosPtr, gqt);
        ckt->ldadd(inst->TJMposNegPtr, -gqt);
        ckt->ldadd(inst->TJMnegPosPtr, -gqt);
        ckt->ldadd(inst->TJMnegNegPtr, gqt);
        if (inst->TJMcontrol) {
            double temp = ts_dcrt*si;
            ckt->ldadd(inst->TJMposIbrPtr, temp);
            ckt->ldadd(inst->TJMnegIbrPtr, -temp);
            crhs -= temp*ts_ci;
        }
        ckt->rhsadd(inst->TJMposNode, -crhs);
        ckt->rhsadd(inst->TJMnegNode, crhs);
    }

#ifndef USE_PRELOAD
    if (inst->TJMphsNode > 0)
        ckt->ldset(inst->TJMphsPhsPtr, 1.0);
#endif
}


int
sTJMmodel::tjm_init()
{
    if (!tjm_coeffsGiven)
        tjm_coeffs = strdup("tjm1");

    TJMcoeffSet *cs = TJMcoeffSet::getTJMcoeffSet(tjm_coeffs);
    if (!cs) {
        // ERROR
        return (E_PANIC);
    }
    tjm_narray = cs->cfs_size;
    tjm_A = new IFcomplex[3*tjm_narray];
    tjm_B = tjm_A + tjm_narray;
    tjm_P = tjm_B + tjm_narray;
    for (int i = 0; i < tjm_narray; i++) {
        tjm_A[i] = cs->cfs_A[i];
        tjm_B[i] = cs->cfs_B[i];
        tjm_P[i] = cs->cfs_P[i];
//printf("%.6f %.6f %.6f %.6f %.6f %.6f\n", tjm_P[i].real, tjm_P[i].imag,
//tjm_A[i].real, tjm_A[i].imag, tjm_B[i].real, tjm_B[i].imag);
    }

    double omega_g = TJMvg/PHI0_2PI;
    double omega_J = sqrt(1.0/(TJMcpic*PHI0_2PI));
    tjm_kgap =  omega_g/omega_J;

    double rejpt = 0.0;
    for (int i = 0; i < tjm_narray; i++)
        rejpt -= (tjm_A[i]/tjm_P[i]).real;
    rejpt *= TJMicFactor;
    tjm_kgap_rejpt = tjm_kgap/rejpt;

    for (int i = 0; i < tjm_narray; i++) {
        IFcomplex C = (TJMicFactor*tjm_A[i] + tjm_B[i]) / (-tjm_kgap*tjm_P[i]);
        IFcomplex D = (TJMicFactor*tjm_A[i] - tjm_B[i]) / (-tjm_kgap*tjm_P[i]);
        tjm_A[i] = C;
        tjm_B[i] = D;
    }
    return (OK);
}


void
sTJMinstance::tjm_init(double phi)
{
    double sinphi_2 = sin(0.5*phi);
    double cosphi_2 = cos(0.5*phi);
    tjm_sinphi_2_old = sinphi_2;
    tjm_cosphi_2_old = cosphi_2;

    sTJMmodel *model = (sTJMmodel*)GENmodPtr;
    int narray = model->tjm_narray;

    if (!tjm_Fc) {
        tjm_Fc = new IFcomplex[7*narray];
        tjm_Fs = tjm_Fc + narray;
        tjm_Fcprev = tjm_Fs + narray;
        tjm_Fsprev = tjm_Fcprev + narray;
        tjm_alpha0 =  tjm_Fsprev + narray;
        tjm_alpha1 = tjm_alpha0 + narray;
        tjm_exp_z = tjm_alpha1 + narray;
    }

    for (int i = 0; i < narray; i++) {
        tjm_Fcprev[i] = cIFcomplex(cosphi_2, 0.0);
        tjm_Fsprev[i] = cIFcomplex(sinphi_2, 0.0);
    }
}


void
sTJMinstance::tjm_newstep(sCKT *ckt)
{
    sTJMmodel *model = (sTJMmodel*)GENmodPtr;
    double omega_J = sqrt(1.0/(model->TJMcpic*PHI0_2PI));
    double dt = 0.5*omega_J*ckt->CKTdelta;

    for (int i = 0; i < model->tjm_narray; i++) {
        cIFcomplex z(model->tjm_P[i]*model->tjm_kgap*dt);
        double d = exp(z.real);
        cIFcomplex ez(d*cos(z.imag), d*sin(z.imag));
        tjm_exp_z[i] = ez;
        tjm_alpha0[i] = (ez - 1.0)/z - ez;
        tjm_alpha1[i] = 1.0 + (1.0 - ez)/z;
    }
}


void
sTJMinstance::tjm_update(double phi, double *jbar)
{
    sTJMmodel *model = (sTJMmodel*)GENmodPtr;
    double sinphi_2 = sin(0.5*phi);
    double cosphi_2 = cos(0.5*phi);

    double FcSum = 0.0;
    double FsSum = 0.0;
    for (int i = 0; i < model->tjm_narray; i++) {
        tjm_Fc[i] = tjm_exp_z[i]*tjm_Fcprev[i] + 
            tjm_alpha0[i]*tjm_cosphi_2_old + tjm_alpha1[i]*cosphi_2;
        tjm_Fs[i] = tjm_exp_z[i]*tjm_Fsprev[i] +
            tjm_alpha0[i]*tjm_sinphi_2_old + tjm_alpha1[i]*sinphi_2;
        FcSum += (model->tjm_A[i]*tjm_Fc[i]).real;
        FsSum += (model->tjm_B[i]*tjm_Fs[i]).real;
    }
    *jbar = model->tjm_kgap_rejpt*(sinphi_2*FcSum + cosphi_2*FsSum);
}


void
sTJMinstance::tjm_accept(double phi)
{
    tjm_sinphi_2_old = sin(0.5*phi);
    tjm_cosphi_2_old = cos(0.5*phi);

    sTJMmodel *model = (sTJMmodel*)GENmodPtr;
    for (int i = 0; i < model->tjm_narray; i++) {
        tjm_Fcprev[i] = tjm_Fc[i];
        tjm_Fsprev[i] = tjm_Fs[i];
    }
}

