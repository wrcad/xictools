
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
#include "tempr.h"


#ifndef M_PI_4
#define M_PI_4      0.785398163397448309615660845819875721  // pi/4
#endif

// MIT-LL SFQ5EE process, parameters from:
// Tolpygo et al., IEEE J. Appl. Superconductivity, 26, 1100110, (2016)
// Tolpygo et al., IEEE J. Appl. Superconductivity, 27, 1100815, (2017)
#define C_PER_A_10000  70.0     // ff/um2
#define I_PER_A_10000 100.0     // uA/um2
#define Vm_LL          16.5     // mV, critical current * subgap resistance
#define IcR_LL          1.65    // mV, critical current * normal resistance

// Various Hypres foundry processes.
#define C_PER_A_4500   59.0     // ff/um2
#define I_PER_A_4500   45.0     // uA/um2
#define C_PER_A_1000   50.0     // ff/um2
#define I_PER_A_1000   10.0     // uA/um2
#define C_PER_A_30     37.0     // ff/um2
#define I_PER_A_30      0.3     // uA/um2
#define Vm_HYP         30.0     // mV, critical current * subgap resistance
#define IcR_HYP         1.7     // mV, critical current * normal resistance

// Hard-wire defaults for MIT-LL process for SuperTools.
#define C_PER_A C_PER_A_10000
#define I_PER_A I_PER_A_10000
#define Vm      (Vm_LL*1e-3)
#define IcR     (IcR_LL*1e-3)

#define VmMin   0.008       // Min Vm V
#define VmMax   0.1         // Max Vm V
#define IcRmin  0.5e-3      // Min IcR V
#define Ic      1e-3        // Assumed Ic of reference, A
#define IcMin   1e-9        // Min reference Ic, A
#define IcMax   1e-1        // Max referenct Ic, A
#define IcsMin  Icrit/50    // Min instance Ic, A
#define IcsMax  Icrit*50    // Max instance Ic, A

#define VgMin   0.1e-3      // Min Vgap, V
#define VgMax   10.0e-3     // Max Vgap, V
#define DelMin  (0.5*VgMin)
#define DelMax  (0.5*VgMax)
#define Temp    4.2
#define TempMin 0
#define TempMax 280
#define TcDef   9.26
#define TcMin   0.5
#define TcMax   280
#define TdbDef  276
#define TdbMin  40
#define TdbMax  500
#define Smf     0.008
#define SmfMin  0.001
#define SmfMax  0.099
#define Ntrms    8
#define NtrmsMin 6
#define NtrmsMax 20
#define Xpts    500
#define XptsMin 100
#define XptsMax 10000
#define Thr     0.2
#define ThrMin  0.1
#define ThrMax  0.5

#define CPIC    (1e-9*C_PER_A/I_PER_A)  // Reference capacitance, F/A
#define CPICmin 0.0         // Minimum CPIC F/A
#define CPICmax 1e-6        // Maximum CPIC F/A
#define CAP     CPIC*Icrit  // Reference capacitance, F
#define ICfct   M_PI_4      // Igap/Ic factor for reference
#define ICfctMin 0.5        // Min factor
#define ICfctMax M_PI_4     // Max factor
#define NOI     1.0         // Noise scale for reference
#define NOImin  0.0         // Min noise scale
#define NOImax  10.0        // Max noise scale

#ifdef NEWLSH
#define LSH_MAX 1e-10
#define LSH0_MAX 2e-12
#define LSH1_MAX 1e-11
#endif

namespace {
    int get_node_ptr(sCKT *ckt, sTJMinstance *inst)
    {
        TSTALLOC(TJMposPosPtr, TJMposNode, TJMposNode)
        TSTALLOC(TJMnegNegPtr, TJMnegNode, TJMnegNode)
        TSTALLOC(TJMnegPosPtr, TJMnegNode, TJMposNode)
        TSTALLOC(TJMposNegPtr, TJMposNode, TJMnegNode)
#ifdef NEWLSER
        // The series inductance is from TJMrealPosNode (device contact)
        // to TJMposNode (an internal node when lser is nonzero).
        TSTALLOC(TJMlserPosIbrPtr, TJMrealPosNode, TJMlserBr)
        TSTALLOC(TJMlserNegIbrPtr, TJMposNode, TJMlserBr)
        TSTALLOC(TJMlserIbrPosPtr, TJMlserBr, TJMrealPosNode)
        TSTALLOC(TJMlserIbrNegPtr, TJMlserBr, TJMposNode)
        TSTALLOC(TJMlserIbrIbrPtr, TJMlserBr, TJMlserBr)
#endif

#ifdef NEWLSH
        // The shunt series inductance is from device top contact to
        // TJMlshIntNode.
#ifdef NEWLSER
        TSTALLOC(TJMlshPosIbrPtr, TJMrealPosNode, TJMlshBr)
        TSTALLOC(TJMlshIbrPosPtr, TJMlshBr, TJMrealPosNode)
#else
        TSTALLOC(TJMlshPosIbrPtr, TJMposNode, TJMlshBr)
        TSTALLOC(TJMlshIbrPosPtr, TJMlshBr, TJMposNode)
#endif
        TSTALLOC(TJMlshNegIbrPtr, TJMlshIntNode, TJMlshBr)
        TSTALLOC(TJMlshIbrNegPtr, TJMlshBr, TJMlshIntNode)
        TSTALLOC(TJMlshIbrIbrPtr, TJMlshBr, TJMlshBr)
#endif

#ifdef NEWLSH
        // Rshunt:  internal node to device contact 2.
        TSTALLOC(TJMrshPosPosPtr, TJMlshIntNode, TJMlshIntNode);
        TSTALLOC(TJMrshPosNegPtr, TJMlshIntNode, TJMnegNode);
        TSTALLOC(TJMrshNegPosPtr, TJMnegNode, TJMlshIntNode);
        TSTALLOC(TJMrshNegNegPtr, TJMnegNode, TJMnegNode);
#else
#ifdef NEWLSER
        // Rshunt:  device contact 1 to device contact 2.
        TSTALLOC(TJMrshPosPosPtr, TJMrealPosNode, TJMrealPosNode);
        TSTALLOC(TJMrshPosNegPtr, TJMrealPosNode, TJMnegNode);
        TSTALLOC(TJMrshNegPosPtr, TJMnegNode, TJMrealPosNode);
        TSTALLOC(TJMrshNegNegPtr, TJMnegNode, TJMnegNode);
#else
        // Rshunt:  device contact 1 to device contact 2.
        TSTALLOC(TJMrshPosPosPtr, TJMposNode, TJMposNode);
        TSTALLOC(TJMrshPosNegPtr, TJMposNode, TJMnegNode);
        TSTALLOC(TJMrshNegPosPtr, TJMnegNode, TJMposNode);
        TSTALLOC(TJMrshNegNegPtr, TJMnegNode, TJMnegNode);
#endif
#endif

        if (inst->TJMphsNode > 0) {
            TSTALLOC(TJMphsPhsPtr, TJMphsNode, TJMphsNode)
        }
        return (OK);
    }
}


int
TJMdev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
    sTJMmodel *model = static_cast<sTJMmodel*>(genmod);
    for ( ; model; model = model->next()) {

        if (!model->TJMrtypeGiven)
            model->TJMrtype = 1;
        else {
            if (model->TJMrtype < 0 || model->TJMrtype > 1) {
                DVO.textOut(OUT_WARNING,
                    "%s: RTYPE=%d out of range [0-%d], reset to 1.\n",
                    model->GENmodName, model->TJMrtype, 1);
                model->TJMrtype = 1;
            }
        }
        if (!model->TJMictypeGiven)
            model->TJMictype = 1;
        else {
            if (model->TJMictype < 0 || model->TJMictype > 1) {
                DVO.textOut(OUT_WARNING,
                    "%s: CCT=%d out of range [0-%d], reset to 1.\n",
                    model->GENmodName, model->TJMictype, 1);
                model->TJMictype = 1;
            }
        }

        if (!model->TJMtnomGiven)
            model->TJMtnom = Temp;
        else {
            if (model->TJMtnom < TempMin || model->TJMtnom > TempMax) {
                DVO.textOut(OUT_WARNING,
                    "%s: TNOM=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMtnom, TempMin, TempMax, Temp);
                model->TJMtnom = Temp;
            }
        }
        if (!model->TJMtempGiven)
            model->TJMtemp = model->TJMtnom;
        else {
            if (model->TJMtemp < TempMin || model->TJMtemp > TempMax) {
                DVO.textOut(OUT_WARNING,
                    "%s: TEMP=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMtemp, TempMin, TempMax,
                    model->TJMtnom);
                model->TJMtemp = model->TJMtnom;
            }
        }

        if (!model->TJMtc1Given)
            model->TJMtc1 = TcDef;
        else {
            if (model->TJMtc1 < TcMin || model->TJMtc1 > TcMax) {
                double tc1 = model->TJMtc1 < TcMin ? TcMin : TcMax;
                DVO.textOut(OUT_WARNING,
                    "%s: TC1=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMtc1, TcMin, TcMax, tc1);
                model->TJMtc1 = tc1;
            }
        }
        if (!model->TJMtc2Given)
            model->TJMtc2 = TcDef;
        else {
            if (model->TJMtc2 < TcMin || model->TJMtc2 > TcMax) {
                double tc2 = model->TJMtc2 < TcMin ? TcMin : TcMax;
                DVO.textOut(OUT_WARNING,
                    "%s: TC2=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMtc2, TcMin, TcMax, tc2);
                model->TJMtc2 = tc2;
            }
        }

        if (!model->TJMtdebye1Given)
            model->TJMtdebye1 = TdbDef;
        else {
            if (model->TJMtdebye1 < TdbMin || model->TJMtdebye1 > TdbMax) {
                double tdb1 = model->TJMtdebye1 < TdbMin ? TdbMin : TdbMax;
                DVO.textOut(OUT_WARNING,
                    "%s: TDEBYE1=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMtdebye1, TdbMin, TdbMax, tdb1);
                model->TJMtdebye1 = tdb1;
            }
        }
        if (!model->TJMtdebye2Given)
            model->TJMtdebye2 = TdbDef;
        else {
            if (model->TJMtdebye2 < TdbMin || model->TJMtdebye2 > TdbMax) {
                double tdb2 = model->TJMtdebye2 < TdbMin ? TdbMin : TdbMax;
                DVO.textOut(OUT_WARNING,
                    "%s: TDEBYE2=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMtdebye2, TdbMin, TdbMax, tdb2);
                model->TJMtdebye2 = tdb2;
            }
        }

        if (model->TJMdel1Given) {
            if (model->TJMdel1 < DelMin || model->TJMdel1 > DelMax) {
                double Del = model->TJMdel1 < DelMin ? DelMin : DelMax;
                DVO.textOut(OUT_WARNING,
                    "%s: DEL1=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMdel1, DelMin, DelMax, Del);
                model->TJMdel1 = Del;
            }
        }
        else {
            model->TJMdel1 = DEV.bcs_egapv(model->TJMtemp, model->TJMtc1,
                model->TJMtdebye1);

        }
        if (model->TJMdel2Given) {
            if (model->TJMdel2 < DelMin || model->TJMdel2 > DelMax) {
                double Del = model->TJMdel2 < DelMin ? DelMin : DelMax;
                DVO.textOut(OUT_WARNING,
                    "%s: DEL2=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMdel2, DelMin, DelMax, Del);
                model->TJMdel2 = Del;
            }
        }
        else {
            model->TJMdel2 = DEV.bcs_egapv(model->TJMtemp, model->TJMtc2,
                model->TJMtdebye2);
        }
        if (!model->TJMvgGiven)
            model->TJMvg = model->TJMdel1 + model->TJMdel2;
        else {
            if (model->TJMvg < VgMin || model->TJMvg > VgMax) {
                double Vg = model->TJMvg < VgMin ? VgMin : VgMax;
                DVO.textOut(OUT_WARNING,
                    "%s: VG=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMvg, VgMin, VgMax, Vg);
                model->TJMvg = Vg;
            }
            model->TJMdel1 = model->TJMdel2 = 0.5*model->TJMvg;
        }

        if (!model->TJMsmfGiven)
            model->TJMsmf = Smf;
        else {
            if (model->TJMsmf < SmfMin || model->TJMsmf > SmfMax) {
                DVO.textOut(OUT_WARNING,
                    "%s: SMF=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMsmf, SmfMin, SmfMax, Smf);
                model->TJMsmf = Smf;
            }
        }

        if (!model->TJMntermsGiven)
            model->TJMnterms = Ntrms;
        else {
            if (model->TJMnterms < NtrmsMin || model->TJMnterms > NtrmsMax) {
                DVO.textOut(OUT_WARNING,
                    "%s: NTERMS=%d out of range [%d-%d], reset to %d.\n",
                    model->GENmodName, model->TJMnterms, NtrmsMin, NtrmsMax,
                    Ntrms);
                model->TJMnterms = Ntrms;
            }
        }

        if (!model->TJMnxptsGiven)
            model->TJMnxpts = Xpts;
        else {
            if (model->TJMnxpts < XptsMin || model->TJMnxpts > XptsMax) {
                DVO.textOut(OUT_WARNING,
                    "%s: XPTS=%d out of range [%d-%d], reset to %d.\n",
                    model->GENmodName, model->TJMnxpts, XptsMin, XptsMax,
                    Xpts);
                model->TJMnxpts = Xpts;
            }
        }

        if (!model->TJMthrGiven)
            model->TJMthr = Thr;
        else {
            if (model->TJMthr < ThrMin || model->TJMthr > ThrMax) {
                DVO.textOut(OUT_WARNING,
                    "%s: THR=%d out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMthr, ThrMin, ThrMax, Thr);
                model->TJMthr = Thr;
            }
        }

        if (!model->TJMcritiGiven)
            model->TJMcriti = Ic;
        else {
            if (model->TJMcriti < IcMin || model->TJMcriti > IcMax) {
                DVO.textOut(OUT_WARNING,
                    "%s: ICRIT=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMcriti, IcMin, IcMax, Ic);
                model->TJMcriti = Ic;
            }
        }

        if (!model->TJMcpicGiven)
            model->TJMcpic = CPIC;
        else {
            if (model->TJMcpic < CPICmin || model->TJMcpic > CPICmax) {
                DVO.textOut(OUT_WARNING,
                    "%s: CPIC=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMcpic, CPICmin, CPICmax, CPIC);
                model->TJMcpic = CPIC;
            }
        }
        if (!model->TJMcapGiven)
            model->TJMcap = model->TJMcpic * model->TJMcriti;
        else {
            double Icrit = model->TJMcriti;
            double cmin = CPICmin * Icrit;
            double cmax = CPICmax * Icrit;
            if (model->TJMcap < cmin || model->TJMcap > cmax) {
                double cap = model->TJMcpic * model->TJMcriti;
                DVO.textOut(OUT_WARNING,
                    "%s: CAP=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMcap, cmin, cmax, cap);
                model->TJMcap = cap;
            }
        }

        if (!model->TJMicfGiven)
            model->TJMicFactor = ICfct;
        else {
            if (model->TJMicFactor < ICfctMin || model->TJMicFactor > ICfctMax) {
                DVO.textOut(OUT_WARNING,
                    "%s: ICFACT=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMicFactor, ICfctMin, ICfctMax,
                    ICfct);
                model->TJMicFactor = ICfct;
            }
        }

        if (!model->TJMvmGiven) {
            model->TJMvm = Vm;
        }
        else {
            if (model->TJMforceGiven) {
                if (model->TJMvm < 0.0) {
                    DVO.textOut(OUT_WARNING,
                        "%s: VM=%g negative, reset to %g.\n",
                        model->GENmodName, model->TJMvm, Vm);
                    model->TJMvm = Vm;
                }
            }
            else {
                if (model->TJMvm != 0.0 &&
                        (model->TJMvm < VmMin || model->TJMvm > VmMax)) {
                    DVO.textOut(OUT_WARNING,
                        "%s: VM=%g out of range 0 or [%g-%g], reset to %g.\n",
                        model->GENmodName, model->TJMvm, VmMin, VmMax, Vm);
                    model->TJMvm = Vm;
                }
            }
        }
        if (!model->TJMr0Given) {
            double i = model->TJMcriti > 0.0 ? model->TJMcriti : 1e-3;
            model->TJMr0 = model->TJMvm/i;
        }
        else {
            if (model->TJMforceGiven) {
                if (model->TJMr0 < 0.0) {
                    double i = model->TJMcriti > 0.0 ? model->TJMcriti : 1e-3;
                    double R0 = model->TJMvm/i;
                    DVO.textOut(OUT_WARNING,
                        "%s: RSUB=%g negative, reset to %g.\n",
                        model->GENmodName, model->TJMr0, R0);
                    model->TJMr0 = R0;
                }
            }
            else {
                double i = model->TJMcriti > 0.0 ? model->TJMcriti : 1e-3;
                double R0min = VmMin/i;
                double R0max = VmMax/i;
                if (model->TJMr0 != 0.0 &&
                        (model->TJMr0 < R0min || model->TJMr0 > R0max)) {
                    double R0 = model->TJMvm/i;
                    DVO.textOut(OUT_WARNING,
                        "%s: RSUB=%g out of range 0 or [%g-%g], reset to %g.\n",
                        model->GENmodName, model->TJMr0, R0min, R0max, R0);
                    model->TJMr0 = R0;
                }
            }
        }

        if (model->TJMvShuntGiven) {
            if (model->TJMvShunt < 0.0 || model->TJMvShunt > model->TJMvg) {
                DVO.textOut(OUT_WARNING,
                    "%s: VSHUNT=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMvShunt, 0.0, model->TJMvg, 0.0);
                model->TJMvShunt = 0.0;
            }
        }
#ifdef NEWLSH
        if (model->TJMlsh0Given) {
            if (model->TJMlsh0 < 0.0 || model->TJMlsh0 > LSH0_MAX) {
                DVO.textOut(OUT_WARNING,
                    "%s: LSH0=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMlsh0, 0.0, LSH0_MAX, 0.0);
                model->TJMlsh0 = 0.0;
            }
        }
        if (model->TJMlsh1Given) {
            if (model->TJMlsh1 < 0.0 || model->TJMlsh1 > LSH1_MAX) {
                DVO.textOut(OUT_WARNING,
                    "%s: LSH1=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMlsh1, 0.0, LSH1_MAX, 0.0);
                model->TJMlsh1 = 0.0;
            }
        }
#endif

        if (!model->TJMnoiseGiven)
            model->TJMnoise = NOI;
        else {
            if (model->TJMnoise < NOImin || model->TJMnoise > NOImax) {
                DVO.textOut(OUT_WARNING,
                    "%s: NOISE=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMnoise, NOImin, NOImax, NOI);
                model->TJMnoise = NOI;
            }
        }

        if (!model->TJMtsfactGiven)
            model->TJMtsfact = ckt->CKTcurTask->TSKdphiMax/(2*M_PI);
        else {
            if (model->TJMtsfact < 0.001 || model->TJMtsfact > 1) {
                DVO.textOut(OUT_WARNING,
                    "%s: TSFACTOR=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMtsfact, 0.001, 1.0,
                    ckt->CKTcurTask->TSKdphiMax/(2*M_PI));
                model->TJMtsfact = ckt->CKTcurTask->TSKdphiMax/(2*M_PI);
            }
        }

        if (!model->TJMtsacclGiven)
            model->TJMtsaccl = 1.0;
        else {
            if (model->TJMtsaccl < 1.0 || model->TJMtsaccl > 20) {
                DVO.textOut(OUT_WARNING,
                    "%s: TSACCEL=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->TJMtsaccl, 1.0, 20.0, 1.0);
                model->TJMtsaccl = 1.0;
            }
        }

        if (model->TJMtemp != model->TJMtnom) {
            // Compute the temperature correction factor for Icrit. 
            // This is obtained form Tinkham 2nd ed.  eq.  6.10,
            // ratioing factors for temp and tnom (correction factor
            // is 1.0 at temp = tnom).

            double vgNom = DEV.bcs_egapv(model->TJMtnom, model->TJMtc1,
                model->TJMtdebye1);
            if (model->TJMtc1 == model->TJMtc2 &&
                    model->TJMtdebye1 == model->TJMtdebye2)
                vgNom += vgNom;
            else {
                vgNom = DEV.bcs_egapv(model->TJMtnom, model->TJMtc2,
                    model->TJMtdebye2);
            }
            double tmp = wrsCHARGE*model->TJMvg/
                (4.0*wrsCONSTboltz*(model->TJMtemp + 1e-4));
            double tmp2 = wrsCHARGE*vgNom/
                (4.0*wrsCONSTboltz*(model->TJMtnom + 1e-4));
            model->TJMicTempFactor =
                (model->TJMvg/vgNom)*(tanh(tmp)/tanh(tmp2));

            // Apply correction.
            model->TJMcriti *= model->TJMicTempFactor;
        }
        else
            model->TJMicTempFactor = 1.0;

        double halfvg = model->TJMvg/2;
        if (model->TJMcap > 0.0) {
            double cpic = model->TJMcap/model->TJMcriti;
            model->TJMvdpbak = sqrt(PHI0_2PI/cpic);
            if (model->TJMvdpbak > halfvg)
                model->TJMvdpbak = halfvg;
            model->TJMomegaJ = sqrt(1.0/(cpic*PHI0_2PI));
        }
        else
            model->TJMvdpbak = halfvg;

        int rval = model->tjm_init();
        if (rval != OK)
            return (rval);

        sTJMinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (inst->TJMicsGiven) {
                if (inst->TJMareaGiven) {
                    DVO.textOut(OUT_WARNING,
                        "%s: ICS and AREA both given, AREA ignored.\n",
                        inst->GENname);
                }
                double Icrit = model->TJMcriti;
                if (inst->TJMics < IcsMin || inst->TJMics > IcsMax) {
                    DVO.textOut(OUT_WARNING,
                        "%s: ICS=%g out of range [%g-%g], reset to %g.\n",
                        inst->GENname, inst->TJMics, IcsMin, IcsMax,
                        model->TJMcriti);
                    inst->TJMics = model->TJMcriti;
                }

                // The area is computed for nominal temperature, which
                // allows to instance critical current to vary with
                // temperature, i.e., it is not fixed at ics.

                double icnom = model->TJMcriti/model->TJMicTempFactor;
                inst->TJMarea = inst->TJMics/icnom;
            }
            else if (!inst->TJMareaGiven)
                inst->TJMarea = 1;

            // ics is the input parameter, criti is the actual critical
            // current to use.
            inst->TJMcriti = model->TJMcriti * inst->TJMarea;

#ifdef NEWLSER
            if (inst->TJMlser > 0.0 && inst->TJMlser < 1e-14) {
                DVO.textOut(OUT_WARNING,
                    "%s: LSER less than 0.01pH, reset to 0.\n", inst->GENname);
                inst->TJMlser = 0.0;
            }
            if (inst->TJMlser > 0.0) {
                sCKTnode *node;
                int err = ckt->mkVolt(&node, inst->GENname, "jpos");
                if (err)
                    return (err);
                inst->TJMposNode = node->number();
                err = ckt->mkCur(&node, inst->GENname, "branch");
                if (err)
                    return (err);
                inst->TJMlserBr = node->number();
            }
            else {
                inst->TJMposNode = inst->TJMrealPosNode;
            }
#endif
            double sqrta = sqrt(inst->TJMarea);
            double gfac = inst->TJMarea*(1.0 - model->TJMgmu) +
                sqrta*model->TJMgmu;
            inst->TJMgqp = gfac/model->TJMrsint;

            // G0 is the user-specified subgap conductance, subtract
            // off the intrinsic conductance if we can.
            inst->TJMg0 = model->TJMr0 > 0.0 ? gfac / model->TJMr0 : 0.0;
            if (inst->TJMg0 >= inst->TJMgqp)
                inst->TJMg0 -= inst->TJMgqp;
            if (model->TJMvShuntGiven && model->TJMvShunt > 0.0) {
                double gshunt = inst->TJMcriti/model->TJMvShunt -
                    SPMAX(inst->TJMg0, inst->TJMgqp);
                if (gshunt > 0.0)
                    inst->TJMgshunt = gshunt;
            }

#ifdef NEWLSH
            if (inst->TJMlshGiven) {
                if (inst->TJMlsh < 0.0 || inst->TJMlsh > LSH_MAX) {
                    DVO.textOut(OUT_WARNING,
                        "%s: LSH=%g out of range [%g-%g], reset to %g.\n",
                        inst->GENname, inst->TJMlsh, 0.0, LSH_MAX, 0.0);
                    inst->TJMlsh = 0.0;
                }
            }
            else if (inst->TJMgshunt > 0.0 &&
                    (model->TJMlsh0Given || model->TJMlsh1Given))
                inst->TJMlsh = model->TJMlsh0 + model->TJMlsh1/inst->TJMgshunt;
            if (inst->TJMgshunt > 0.0) {
                if (inst->TJMlsh > 0) {
                    sCKTnode *node;
                    int err = ckt->mkVolt(&node, inst->GENname, "sh");
                    if (err)
                        return (err);
                    inst->TJMlshIntNode = node->number();
                    err = ckt->mkCur(&node, inst->GENname, "shbr");
                    if (err)
                        return (err);
                    inst->TJMlshBr = node->number();
                }
                else {
#ifdef NEWLSER
                    inst->TJMlshIntNode = inst->TJMrealPosNode;
#else
                    inst->TJMlshIntNode = inst->TJMposNode;
#endif
                }
            }
#endif

#ifdef NEWJJDC
            if (!ckt->CKTcurTask->TSKnoPhaseModeDC) {
                // Set the phase flag of connected nodes for
                // phase-mode DC analysis, if critical current is
                // turned on and phase mode DC is not disabled.

#ifdef NEWLSER
                if (inst->TJMrealPosNode > 0) {
                    sCKTnode *node = ckt->CKTnodeTab.find(inst->TJMrealPosNode);
                    if (node)
                        node->set_phase(true);
                }
#endif
                if (inst->TJMposNode > 0) {
                    sCKTnode *node = ckt->CKTnodeTab.find(inst->TJMposNode);
                    if (node)
                        node->set_phase(true);
                }
                if (inst->TJMnegNode > 0) {
                    sCKTnode *node = ckt->CKTnodeTab.find(inst->TJMnegNode);
                    if (node)
                        node->set_phase(true);
                }
            }
#endif
            inst->TJMcap = model->TJMcap*(inst->TJMarea*(1.0 - model->TJMcmu) +
                sqrta*model->TJMcmu);

            if (!inst->TJMnoiseGiven)
                inst->TJMnoise = model->TJMnoise;
            else {
                if (inst->TJMnoise < NOImin || inst->TJMnoise > NOImax) {
                    DVO.textOut(OUT_WARNING,
                        "%s: NOISE out of range [%g-%g], reset to %g.\n",
                        inst->GENname, NOImin, NOImax, NOI);
                    inst->TJMnoise = NOI;
                }
            }

            inst->GENstate = *states;
            *states += TJMnumStates;

            int error = get_node_ptr(ckt, inst);
            if (error != OK)
                return (error);

#ifdef USE_PRELOAD
            if (ckt->CKTpreload) {
                if (inst->TJMphsNode > 0) {
                    ckt->preldset(inst->TJMphsPhsPtr, 1.0);
                }
#ifdef NEWLSER
                if (inst->TJMrealPosNode) {
                    ckt->preldset(inst->TJMlserPosIbrPtr, 1.0);
                    ckt->preldset(inst->TJMlserIbrPosPtr, 1.0);
                }
                if (inst->TJMposNode) {
                    ckt->preldset(inst->TJMlserNegIbrPtr, -1.0);
                    ckt->preldset(inst->TJMlserIbrNegPtr, -1.0);
                }
#endif
#ifdef NEWLSH
                if (inst->TJMlsh > 0.0) {
                    ckt->ldset(inst->TJMlshPosIbrPtr, 1.0);
                    ckt->ldset(inst->TJMlshIbrPosPtr, 1.0);
                    ckt->ldset(inst->TJMlshNegIbrPtr, -1.0);
                    ckt->ldset(inst->TJMlshIbrNegPtr, -1.0);
                }
#endif
            }
#endif
            // Make sure that the arrays are built now.
            inst->tjm_init(0.0);
        }
    }

    return (OK);
}


// Reset the matrix element pointers.
//
int
TJMdev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sTJMmodel *model = (sTJMmodel*)inModel; model;
            model = model->next()) {
        for (sTJMinstance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}
// End of TJMdev functions.


int
sTJMmodel::tjm_init()
{
    TJMcoeffSet *cs;
    if (tjm_coeffsGiven)
        cs = TJMcoeffSet::getTJMcoeffSet(tjm_coeffs);
    else
        cs = TJMcoeffSet::getTJMcoeffSet(TJMtemp, TJMdel1, TJMdel2, TJMsmf,
            TJMnxpts, TJMnterms, TJMthr);
    if (!cs) {
        DVO.textOut(OUT_FATAL,
            "%s: coefficient set %s not found.\n", GENmodName, tjm_coeffs);
        return (E_PANIC);
    }
    if (TJMnterms != cs->size()) {
        DVO.textOut(OUT_FATAL,
            "%s: coefficient set %s term count %d not equal to %d.\n",
            GENmodName, tjm_coeffs, cs->size(), TJMnterms);
        return (E_PANIC);
    }
    tjm_p = new IFcomplex[3*TJMnterms];
    tjm_A = tjm_p + TJMnterms;
    tjm_B = tjm_A + TJMnterms;
    for (int i = 0; i < TJMnterms; i++) {
        tjm_p[i] = cs->p()[i];
        tjm_A[i] = cs->A()[i];
        tjm_B[i] = cs->B()[i];
    }

    double omega_g = 0.5*TJMvg/PHI0_2PI;  // e*Vg = hbar*omega_g
    tjm_kgap = omega_g/TJMomegaJ;

    double rejpt = 0.0;
    for (int i = 0; i < TJMnterms; i++)
        rejpt -= (tjm_A[i]/tjm_p[i]).real;
    rejpt *= TJMicFactor;
    tjm_rejpt = rejpt;
    tjm_kgap_rejpt = tjm_kgap/rejpt;
    tjm_alphaN = 1.0/(2*rejpt*tjm_kgap);

    // Compute the internal subgap resistance.  This involves
    // computing the quasiparticle model, extracting the current at
    // 80% of Vgap, and computing the linear resistance.  Must be a
    // better way?

    // First, define the X-axis, per mmjco.
    double *xpts = new double[TJMnxpts];
    double x = 0.001;
    double dx = (2.0 - x)/(TJMnxpts - 1);
    for (int i = 0; i < TJMnxpts; i++) {
        xpts[i] = x;
        x += dx;
    }

    // This returns the quasiparticle TCA model.
    cIFcomplex *qp =
        TJMcoeffSet::modelJqp((const cIFcomplex*)tjm_p,
        (const cIFcomplex*)tjm_B, TJMnterms, xpts, TJMnxpts);

    // Index of the point closest to 80% of Vgap.
    int n = round((0.8 - 0.001)/dx);

    // Un-normalization factor.
    double fct = TJMcriti * tjm_kgap_rejpt;

    // Save the approximate intrinsic subgap resistance.
    TJMrsint = 0.8*TJMvg/(fct*qp[n].imag);

    delete [] xpts;
    delete [] qp;
    // End of subgap resistance computation.

    // Renormalize.  Here we would rotate to C and D vectors, however
    // we want to preserve the pair and qp amplitudes for separate
    // access.
    for (int i = 0; i < TJMnterms; i++) {
        tjm_A[i] = (TJMicFactor*tjm_A[i]) / (-tjm_kgap*tjm_p[i]);
        tjm_B[i] = (tjm_B[i]) / (-tjm_kgap*tjm_p[i]);
    }

    return (OK);
}
// End of sTJMmodel functions.

