
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

namespace TJM {
struct tjmstuff
{
    double ts_vj;
    double ts_phi;
    double ts_ci;
    double ts_crt;
    double ts_dcrt;
    double ts_pfac;
};
}


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
            ts.ts_dcrt = 0;
            ts.ts_crt  = inst->TJMcriti;
            if (model->TJMictype != 1)
                model->tjm_ic(ts);
/*XXX
            if (ckt->CKTmode & MODEINITSMSIG) {
                // We don't want/need this except when setting up for AC
                // analysis.
                ts.tjm_iv(model, inst);
            }
*/
            inst->tjm_load(ckt, ts);
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
            ts.ts_dcrt = 0.0;
            ts.ts_crt  = 0.0;
        
            inst->tjm_load(ckt, ts);

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
                ckt->incNoncon();
            }
        }

        ts.ts_phi = *(ckt->CKTstate1 + inst->TJMphase);
        temp = ts.ts_vj;
        if (ckt->CKTorder > 1)
            temp += *(ckt->CKTstate1 + inst->TJMvoltage);
        ts.ts_phi += ts.ts_pfac*temp;

        ts.ts_dcrt = 0;
        ts.ts_crt  = inst->TJMcriti;
    
        ckt->integrate(inst->TJMvoltage, inst->TJMdelVdelT);
        //
        // compute quasiparticle current and derivatives
        //
        if (model->TJMictype != 1)
            model->tjm_ic(ts);
        inst->tjm_load(ckt, ts);

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

        ts.ts_dcrt = 0;
        ts.ts_crt  = inst->TJMcriti;

        if (model->TJMictype != 1)
            model->tjm_ic(ts);
        inst->tjm_load(ckt, ts);

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

        ts.ts_dcrt = 0;
        ts.ts_crt  = inst->TJMcriti;

        if (model->TJMictype != 1)
            model->tjm_ic(ts);
        inst->tjm_load(ckt, ts);

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
// End of TJMdev functions.


// Non-default supercurrent
// For shaped junction types, parameters scale with area as in small
// junctions, control current does not scale.
//
void
sTJMmodel::tjm_ic(tjmstuff &ts)
{
    double ci = ts.ts_ci;

    if (TJMictype == 2) {

        if (ci != 0.0) {
            double xx = ts.ts_crt;
            double ang  = M_PI * ci / TJMccsens;
            ts.ts_crt *= sin(ang)/ang;
            ts.ts_dcrt = xx*(cos(ang) - ts.ts_crt)/ci;
        }
    }
    else if (TJMictype == 3) {

        double temp = ci < 0 ? -ci : ci;
        if (temp < TJMccsens) {
            ts.ts_dcrt = ts.ts_crt / TJMccsens;
            ts.ts_crt *= (1.0 - temp/TJMccsens);
            if (ci > 0.0)
                ts.ts_dcrt = -ts.ts_dcrt;
            if (ci == 0.0)
                ts.ts_dcrt = 0.0;
        }
        else ts.ts_crt = 0.0;
    }
    else if (TJMictype == 4) {

        double temp = ci < 0 ? -ci : ci;
        if (temp < TJMccsens) {
            temp = TJMccsens + TJMccsens;
            ts.ts_dcrt = -ts.ts_crt / temp;
            ts.ts_crt *= (TJMccsens - ci)/temp;
            if (ci == 0.0)
                ts.ts_dcrt = 0.0;
        }
        else
            ts.ts_crt = 0.0;
    }
    else
        ts.ts_crt = 0.0;
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
    }

    double omega_g = 0.5*TJMvg/PHI0_2PI;  // e*Vg = hbar*omega_g
    tjm_kgap = omega_g/TJMomegaJ;

    double rejpt = 0.0;
    for (int i = 0; i < tjm_narray; i++)
        rejpt -= (tjm_A[i]/tjm_P[i]).real;
    rejpt *= TJMicFactor;
    tjm_kgap_rejpt = tjm_kgap/rejpt;
    tjm_alphaN = TJMicFactor/(2*rejcpt*tjm_kgap*tjm_kgap);

    for (int i = 0; i < tjm_narray; i++) {
        IFcomplex C = (TJMicFactor*tjm_A[i] + tjm_B[i]) / (-tjm_kgap*tjm_P[i]);
        IFcomplex D = (TJMicFactor*tjm_A[i] - tjm_B[i]) / (-tjm_kgap*tjm_P[i]);
        tjm_A[i] = C;
        tjm_B[i] = D;
    }
    return (OK);
}
// End of sTJMmodel functions.


namespace {
    // Intel/GCC-specific function for fast evaluation of sin and cos
    // of an angle.
    //
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
sTJMinstance::tjm_load(sCKT *ckt, tjmstuff &ts)
{
    *(ckt->CKTstate0 + TJMvoltage) = ts.ts_vj;
    *(ckt->CKTstate0 + TJMphase)   = ts.ts_phi;
    *(ckt->CKTstate0 + TJMconI)    = ts.ts_ci;
    // these two for TJMask()
    *(ckt->CKTstate0 + TJMcrti)    = ts.ts_crt;
    *(ckt->CKTstate0 + TJMqpi)     = 0.0;  //XXX fixme

    double crhs, gqt;
    if (ckt->CKTmode & MODEDC) {
        double crt = ts.ts_crt*sin(ts.ts_phi); 
        gqt = ts.ts_pfac*ts.ts_crt*cos(ts.ts_phi);
        crhs = crt - gqt*ts.ts_phi;
//        tjm_init(ts.ts_phi);
//        tjm_update(ts.ts_phi);
    }
    else {
        crhs = ts.ts_crt*tjm_update(ts.ts_phi) + TJMdelVdelT*TJMcap;
        sTJMmodel *model = (sTJMmodel*)GENmodPtr;
//        double alphaN = model->TJMicFactor*model->tjm_kgap_rejpt/
//            (2*model->tjm_kgap*model->tjm_kgap*model->tjm_kgap);
        gqt = tjm_alphaN + TJMg0 +  ckt->CKTag[0]*TJMcap;
printf("%g %g %g\n", tjm_alphaN + TJMg0, TJMgn, model->tjm_kgap);
    }

    // load matrix, rhs vector
    if (TJMphsNode > 0)
        *(ckt->CKTrhs + TJMphsNode) = ts.ts_phi +
            (4*M_PI)* *(int*)(ckt->CKTstate1 + TJMphsInt);

    if (!TJMnegNode) {
        ckt->ldadd(TJMposPosPtr, gqt);
/*
        if (TJMcontrol) {
            double temp = ts.ts_dcrt*si;
            ckt->ldadd(TJMposIbrPtr, temp);
            crhs -= temp*ts.ts_ci;
        }
*/
        ckt->rhsadd(TJMposNode, -crhs);
    }
    else if (!TJMposNode) {
        ckt->ldadd(TJMnegNegPtr, gqt);
/*
        if (TJMcontrol) {
            double temp = ts.ts_dcrt*si;
            ckt->ldadd(TJMnegIbrPtr, -temp);
            crhs -= temp*ts.ts_ci;
        }
*/
        ckt->rhsadd(TJMnegNode, crhs);
    }
    else {
        ckt->ldadd(TJMposPosPtr, gqt);
        ckt->ldadd(TJMposNegPtr, -gqt);
        ckt->ldadd(TJMnegPosPtr, -gqt);
        ckt->ldadd(TJMnegNegPtr, gqt);
/*
        if (TJMcontrol) {
            double temp = ts.ts_dcrt*si;
            ckt->ldadd(TJMposIbrPtr, temp);
            ckt->ldadd(TJMnegIbrPtr, -temp);
            crhs -= temp*ts.ts_ci;
        }
*/
        ckt->rhsadd(TJMposNode, -crhs);
        ckt->rhsadd(TJMnegNode, crhs);
    }

#ifndef USE_PRELOAD
    if (TJMphsNode > 0)
        ckt->ldset(TJMphsPhsPtr, 1.0);
#endif
}


void
sTJMinstance::tjm_init(double phi)
{
    double sinphi_2, cosphi_2;
    sincos(0.5*phi, sinphi_2, cosphi_2);
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
        memset(tjm_Fc, 0, 7*narray*sizeof(IFcomplex));
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
    double dt = model->TJMomegaJ*ckt->CKTdelta;

    for (int i = 0; i < model->tjm_narray; i++) {
        cIFcomplex z(model->tjm_P[i]*model->tjm_kgap*dt);
        double d = exp(z.real);
        cIFcomplex ez(d*cos(z.imag), d*sin(z.imag));
        tjm_exp_z[i] = ez;
        tjm_alpha0[i] = (ez - 1.0)/z - ez;
        tjm_alpha1[i] = 1.0 + (1.0 - ez)/z;
    }
}


double
sTJMinstance::tjm_update(double phi)
{
    double sinphi_2, cosphi_2;
    sincos(0.5*phi, sinphi_2, cosphi_2);

    double FcSum = 0.0;
    double FsSum = 0.0;
    sTJMmodel *model = (sTJMmodel*)GENmodPtr;
    for (int i = 0; i < model->tjm_narray; i++) {
        tjm_Fc[i] = tjm_exp_z[i]*tjm_Fcprev[i] + 
            tjm_alpha0[i]*tjm_cosphi_2_old + tjm_alpha1[i]*cosphi_2;
        tjm_Fs[i] = tjm_exp_z[i]*tjm_Fsprev[i] +
            tjm_alpha0[i]*tjm_sinphi_2_old + tjm_alpha1[i]*sinphi_2;
        FcSum += (model->tjm_A[i]*tjm_Fc[i]).real;
        FsSum += (model->tjm_B[i]*tjm_Fs[i]).real;
    }
    return (model->tjm_kgap_rejpt*(sinphi_2*FcSum + cosphi_2*FsSum));
}


void
sTJMinstance::tjm_accept(double phi)
{
    sincos(0.5*phi, tjm_sinphi_2_old, tjm_cosphi_2_old);

    sTJMmodel *model = (sTJMmodel*)GENmodPtr;
    for (int i = 0; i < model->tjm_narray; i++) {
        tjm_Fcprev[i] = tjm_Fc[i];
        tjm_Fsprev[i] = tjm_Fs[i];
    }
}

