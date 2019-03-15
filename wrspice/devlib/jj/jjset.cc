
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
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Author: 1992 Stephen R. Whiteley
****************************************************************************/

#include "jjdefs.h"


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
#define IcsMin  Icrit/20    // Min instance Ic, A
#define IcsMax  Icrit*20    // Max instance Ic, A
#define Vg      2.6e-3      // Assumed Vgap of reference, V
#define VgMin   0.1e-3      // Min Vgap, V
#define VgMax   10.0e-3     // Max Vgap, V
#define DelV    0.08e-3     // Assumed delVg of reference, V
#define DelVmin 0.001*Vgap  // Min delVg, V
#define DelVmax 0.2*Vgap    // Max delVg, V

#define RTMAX   4           // Max rtype, integer
#define ITMAX   4           // Max ictype, integer
#define CCsens  1e-2        // Assumed magnetic sens. of reference
#define CCsensMin 1e-4      // Min magnetic sens.
#define CCsensMax 1.0       // Max magnetic sens.
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
    int get_node_ptr(sCKT *ckt, sJJinstance *inst)
    {
        TSTALLOC(JJposPosPtr, JJposNode, JJposNode)
        TSTALLOC(JJnegNegPtr, JJnegNode, JJnegNode)
        TSTALLOC(JJnegPosPtr, JJnegNode, JJposNode)
        TSTALLOC(JJposNegPtr, JJposNode, JJnegNode)
#ifdef NEWLSER
        // The series inductance is from JJrealPosNode (device contact)
        // to JJposNode (an internal node when lser is nonzero).
        TSTALLOC(JJlserPosIbrPtr, JJrealPosNode, JJlserBr)
        TSTALLOC(JJlserNegIbrPtr, JJposNode, JJlserBr)
        TSTALLOC(JJlserIbrPosPtr, JJlserBr, JJrealPosNode)
        TSTALLOC(JJlserIbrNegPtr, JJlserBr, JJposNode)
        TSTALLOC(JJlserIbrIbrPtr, JJlserBr, JJlserBr)
#endif

#ifdef NEWLSH
        // The shunt series inductance is from device top contact to
        // JJlshIntNode.
#ifdef NEWLSER
        TSTALLOC(JJlshPosIbrPtr, JJrealPosNode, JJlshBr)
        TSTALLOC(JJlshIbrPosPtr, JJlshBr, JJrealPosNode)
#else
        TSTALLOC(JJlshPosIbrPtr, JJposNode, JJlshBr)
        TSTALLOC(JJlshIbrPosPtr, JJlshBr, JJposNode)
#endif
        TSTALLOC(JJlshNegIbrPtr, JJlshIntNode, JJlshBr)
        TSTALLOC(JJlshIbrNegPtr, JJlshBr, JJlshIntNode)
        TSTALLOC(JJlshIbrIbrPtr, JJlshBr, JJlshBr)
#endif

#ifdef NEWLSH
        // Rshunt:  internal node to device contact 2.
        TSTALLOC(JJrshPosPosPtr, JJlshIntNode, JJlshIntNode);
        TSTALLOC(JJrshPosNegPtr, JJlshIntNode, JJnegNode);
        TSTALLOC(JJrshNegPosPtr, JJnegNode, JJlshIntNode);
        TSTALLOC(JJrshNegNegPtr, JJnegNode, JJnegNode);
#else
#ifdef NEWLSER
        // Rshunt:  device contact 1 to device contact 2.
        TSTALLOC(JJrshPosPosPtr, JJrealPosNode, JJrealPosNode);
        TSTALLOC(JJrshPosNegPtr, JJrealPosNode, JJnegNode);
        TSTALLOC(JJrshNegPosPtr, JJnegNode, JJrealPosNode);
        TSTALLOC(JJrshNegNegPtr, JJnegNode, JJnegNode);
#else
        // Rshunt:  device contact 1 to device contact 2.
        TSTALLOC(JJrshPosPosPtr, JJposNode, JJposNode);
        TSTALLOC(JJrshPosNegPtr, JJposNode, JJnegNode);
        TSTALLOC(JJrshNegPosPtr, JJnegNode, JJposNode);
        TSTALLOC(JJrshNegNegPtr, JJnegNode, JJnegNode);
#endif
#endif

        if (inst->JJcontrol) {
            TSTALLOC(JJposIbrPtr, JJposNode, JJbranch)
            TSTALLOC(JJnegIbrPtr, JJnegNode, JJbranch)
        }
        if (inst->JJphsNode > 0) {
            TSTALLOC(JJphsPhsPtr, JJphsNode, JJphsNode)
        }
        return (OK);
    }
}


int
JJdev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
    sJJmodel *model = static_cast<sJJmodel*>(genmod);
    for ( ; model; model = model->next()) {

        if (!model->JJrtypeGiven)
            model->JJrtype = 1;
        else {
            if (model->JJrtype < 0 || model->JJrtype > RTMAX) {
                DVO.textOut(OUT_WARNING,
                    "%s: RTYPE=%d out of range [0-%d], reset to 1.\n",
                    model->GENmodName, model->JJrtype, RTMAX);
                model->JJrtype = 1;
            }
        }
        if (!model->JJictypeGiven)
            model->JJictype = 1;
        else {
            if (model->JJictype < 0 || model->JJictype > ITMAX) {
                DVO.textOut(OUT_WARNING,
                    "%s: CCT=%d out of range [0-%d], reset to 1.\n",
                    model->GENmodName, model->JJictype, ITMAX);
                model->JJictype = 1;
            }
        }
        if (!model->JJvgGiven)
            model->JJvg = Vg;
        else {
            if (model->JJvg < VgMin || model->JJvg > VgMax) {
                DVO.textOut(OUT_WARNING,
                    "%s: VG=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJvg, VgMin, VgMax, Vg);
                model->JJvg = Vg;
            }
        }
        if (!model->JJdelvGiven)
            model->JJdelv = DelV;
        else {
            double Vgap = model->JJvg;
            if (model->JJdelv < DelVmin || model->JJdelv > DelVmax) {
                DVO.textOut(OUT_WARNING,
                    "%s: DELV=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJdelv, DelVmin, DelVmax, DelV);
                model->JJdelv = DelV;
            }
        }

        double halfdv = 0.5*model->JJdelv;
        model->JJvless  = model->JJvg - halfdv;
        model->JJvmore  = model->JJvg + halfdv;

        if (!model->JJccsensGiven)
            model->JJccsens = CCsens;
        else {
            if (model->JJccsens < CCsensMin || model->JJccsens > CCsensMax) {
                DVO.textOut(OUT_WARNING,
                    "%s: ICON=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJccsens, CCsensMin, CCsensMax,
                    CCsens);
                model->JJccsens = CCsens;
            }
        }
        if (!model->JJcritiGiven)
            model->JJcriti = Ic;
        else {
            if (model->JJcriti < IcMin || model->JJcriti > IcMax) {
                DVO.textOut(OUT_WARNING,
                    "%s: ICRIT=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJcriti, IcMin, IcMax, Ic);
                model->JJcriti = Ic;
            }
        }
        if (!model->JJcpicGiven)
            model->JJcpic = CPIC;
        else {
            if (model->JJcpic < CPICmin || model->JJcpic > CPICmax) {
                DVO.textOut(OUT_WARNING,
                    "%s: CPIC=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJcpic, CPICmin, CPICmax, CPIC);
                model->JJcpic = CPIC;
            }
        }
        if (!model->JJcapGiven)
            model->JJcap = model->JJcpic * model->JJcriti;
        else {
            double Icrit = model->JJcriti;
            double cmin = CPICmin * Icrit;
            double cmax = CPICmax * Icrit;
            if (model->JJcap < cmin || model->JJcap > cmax) {
                double cap = model->JJcpic * model->JJcriti;
                DVO.textOut(OUT_WARNING,
                    "%s: CAP=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJcap, cmin, cmax, cap);
                model->JJcap = cap;
            }
        }
        if (!model->JJicfGiven)
            model->JJicFactor = ICfct;
        else {
            if (model->JJicFactor < ICfctMin || model->JJicFactor > ICfctMax) {
                DVO.textOut(OUT_WARNING,
                    "%s: ICFACT=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJicFactor, ICfctMin, ICfctMax,
                    ICfct);
                model->JJicFactor = ICfct;
            }
        }
        if (!model->JJvmGiven) {
            model->JJvm = Vm;
        }
        else {
            if (model->JJvm < VmMin || model->JJvm > VmMax) {
                DVO.textOut(OUT_WARNING,
                    "%s: VM=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJvm, VmMin, VmMax, Vm);
                model->JJvm = Vm;
            }
        }

        if (!model->JJr0Given) {
            double i = model->JJcriti > 0.0 ? model->JJcriti : 1e-3;
            model->JJr0 = model->JJvm/i;
        }
        else {
            double i = model->JJcriti > 0.0 ? model->JJcriti : 1e-3;
            double R0min = VmMin/i;
            double R0max = VmMax/i;
            if (model->JJr0 < R0min || model->JJr0 > R0max) {
                double R0 = model->JJvm/i;
                DVO.textOut(OUT_WARNING,
                    "%s: RSUB=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJr0, R0min, R0max, R0);
                model->JJr0 = R0;
            }
        }

        if (!model->JJicrnGiven) {
            model->JJicrn = IcR;
        }
        else {
            double IcRmax = model->JJvg * model->JJicFactor;
            if (model->JJicrn < IcRmin || model->JJicrn > IcRmax) {
                DVO.textOut(OUT_WARNING,
                    "%s: ICRN=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJicrn, IcRmin, IcRmax, IcR);
                model->JJicrn = IcR;
            }
        }
        if (!model->JJrnGiven) {
            double i = model->JJcriti > 0.0 ? model->JJcriti : 1e-3;
            model->JJrn = model->JJicrn/i;
        }
        else {
            double i = model->JJcriti > 0.0 ? model->JJcriti : 1e-3;
            double RNmin = IcRmin/i;
            double RNmax = (model->JJvg * model->JJicFactor)/i;
            if (model->JJrn < RNmin || model->JJrn > RNmax) {
                double RN = model->JJicrn/i;
                DVO.textOut(OUT_WARNING,
                    "%s: RN=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJrn, RNmin, RNmax, RN);
                model->JJrn = RN;
            }
        }
        if (model->JJrn > model->JJr0)
            model->JJrn = model->JJr0;

        if (model->JJvShuntGiven) {
            if (model->JJvShunt < 0.0 ||
                    model->JJvShunt > (model->JJvg - model->JJdelv)) {
                DVO.textOut(OUT_WARNING,
                    "%s: VSHUNT=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJvShunt, 0.0, model->JJvg, 0.0);
                model->JJvShunt = 0.0;
            }
        }
#ifdef NEWLSH
        if (model->JJlsh0Given) {
            if (model->JJlsh0 < 0.0 || model->JJlsh0 > LSH0_MAX) {
                DVO.textOut(OUT_WARNING,
                    "%s: LSH0=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJlsh0, 0.0, LSH0_MAX, 0.0);
                model->JJlsh0 = 0.0;
            }
        }
        if (model->JJlsh1Given) {
            if (model->JJlsh1 < 0.0 || model->JJlsh1 > LSH1_MAX) {
                DVO.textOut(OUT_WARNING,
                    "%s: LSH1=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJlsh1, 0.0, LSH1_MAX, 0.0);
                model->JJlsh1 = 0.0;
            }
        }
#endif

        if (!model->JJnoiseGiven)
            model->JJnoise = NOI;
        else {
            if (model->JJnoise < NOImin || model->JJnoise > NOImax) {
                DVO.textOut(OUT_WARNING,
                    "%s: NOISE=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJnoise, NOImin, NOImax, NOI);
                model->JJnoise = NOI;
            }
        }

        if (!model->JJtsfactGiven)
            model->JJtsfact = ckt->CKTcurTask->TSKdphiMax/M_PI;
        else {
            if (model->JJtsfact < 0.001 || model->JJtsfact > 1) {
                DVO.textOut(OUT_WARNING,
                    "%s: TSFACTOR=%g out of range [%g-%g], reset to %g.\n",
                    model->GENmodName, model->JJtsfact, 0.001, 1.0,
                    ckt->CKTcurTask->TSKdphiMax/M_PI);
                model->JJtsfact = ckt->CKTcurTask->TSKdphiMax/M_PI;
            }
        }

        double halfvg = model->JJvg/2;
        if (model->JJcap > 0.0) {
            model->JJvdpbak = sqrt(PHI0_2PI * model->JJcriti / model->JJcap);
            if (model->JJvdpbak > halfvg)
                model->JJvdpbak = halfvg;
        }
        else
            model->JJvdpbak = halfvg;

        sJJinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (inst->JJicsGiven) {
                if (inst->JJareaGiven) {
                    DVO.textOut(OUT_WARNING,
                        "%s: ICS and AREA both given, AREA ignored.\n",
                        inst->GENname);
                }
                double Icrit = model->JJcriti;
                if (inst->JJics < IcsMin || inst->JJics > IcsMax) {
                    DVO.textOut(OUT_WARNING,
                        "%s: ICS=%g out of range [%g-%g], reset to %g.\n",
                        inst->GENname, inst->JJics, IcsMin, IcsMax,
                        model->JJcriti);
                    inst->JJics = model->JJcriti;
                }
                inst->JJarea = inst->JJics/model->JJcriti;
            }
            else if (!inst->JJareaGiven)
                inst->JJarea = 1;

            // ics is the input parameter, criti is the actual critical
            // current to use.
            inst->JJcriti = model->JJcriti * inst->JJarea;

#ifdef NEWLSER
            if (inst->JJlser > 0.0 && inst->JJlser < 1e-14) {
                DVO.textOut(OUT_WARNING,
                    "%s: LSER less than 0.01pH, reset to 0.\n", inst->GENname);
                inst->JJlser = 0.0;
            }
            if (inst->JJlser > 0.0) {
                sCKTnode *node;
                int err = ckt->mkVolt(&node, inst->GENname, "jpos");
                if (err)
                    return (err);
                inst->JJposNode = node->number();
                err = ckt->mkCur(&node, inst->GENname, "branch");
                if (err)
                    return (err);
                inst->JJlserBr = node->number();
            }
            else {
                inst->JJposNode = inst->JJrealPosNode;
            }
#endif
            inst->JJgqp = sJJmodel::subgap(model, inst);
            if (model->JJvShuntGiven) {
                double gshunt = inst->JJcriti/model->JJvShunt - inst->JJgqp;
                if (gshunt > 0.0)
                    inst->JJgshunt = gshunt;
            }

#ifdef NEWLSH
            if (inst->JJlshGiven) {
                if (inst->JJlsh < 0.0 || inst->JJlsh > LSH_MAX) {
                    DVO.textOut(OUT_WARNING,
                        "%s: LSH=%g out of range [%g-%g], reset to %g.\n",
                        inst->GENname, inst->JJlsh, 0.0, LSH_MAX, 0.0);
                    inst->JJlsh = 0.0;
                }
            }
            else if (inst->JJgshunt > 0.0 &&
                    (model->JJlsh0Given || model->JJlsh1Given))
                inst->JJlsh = model->JJlsh0 + model->JJlsh1/inst->JJgshunt;
            if (inst->JJgshunt > 0.0) {
                if (inst->JJlsh > 0) {
                    sCKTnode *node;
                    int err = ckt->mkVolt(&node, inst->GENname, "sh");
                    if (err)
                        return (err);
                    inst->JJlshIntNode = node->number();
                    err = ckt->mkCur(&node, inst->GENname, "shbr");
                    if (err)
                        return (err);
                    inst->JJlshBr = node->number();
                }
                else {
#ifdef NEWLSER
                    inst->JJlshIntNode = inst->JJrealPosNode;
#else
                    inst->JJlshIntNode = inst->JJposNode;
#endif
                }
            }
#endif

#ifdef NEWJJDC
            if (model->JJictype > 0 && !ckt->CKTcurTask->TSKnoPhaseModeDC) {
                // Set the phase flag of connected nodes for
                // phase-mode DC analysis, if critical current is
                // turned on and phase mode DC is not disabled.

#ifdef NEWLSER
                if (inst->JJrealPosNode > 0) {
                    sCKTnode *node = ckt->CKTnodeTab.find(inst->JJrealPosNode);
                    if (node)
                        node->set_phase(true);
                }
#endif
                if (inst->JJposNode > 0) {
                    sCKTnode *node = ckt->CKTnodeTab.find(inst->JJposNode);
                    if (node)
                        node->set_phase(true);
                }
                if (inst->JJnegNode > 0) {
                    sCKTnode *node = ckt->CKTnodeTab.find(inst->JJnegNode);
                    if (node)
                        node->set_phase(true);
                }
            }
#endif

            double sqrta = sqrt(inst->JJarea);
            inst->JJcap = model->JJcap*(inst->JJarea*(1.0 - model->JJcmu) +
                sqrta*model->JJcmu);

            double gfac = inst->JJarea*(1.0 - model->JJgmu) +
                sqrta*model->JJgmu;
            inst->JJg0 = gfac / model->JJr0;
            inst->JJgn = gfac / model->JJrn;
            inst->JJgs = inst->JJcriti/(model->JJicFactor * model->JJdelv);

            // These currents are added to RHS in piecewise qp model
            inst->JJcr1 = (inst->JJg0 - inst->JJgs)*model->JJvless;
            inst->JJcr2 = inst->JJcriti/model->JJicFactor +
                model->JJvless * inst->JJg0 -
                model->JJvmore * inst->JJgn;

            if (!inst->JJnoiseGiven)
                inst->JJnoise = model->JJnoise;
            else {
                if (inst->JJnoise < NOImin || inst->JJnoise > NOImax) {
                    DVO.textOut(OUT_WARNING,
                        "%s: NOISE out of range [%g-%g], reset to %g.\n",
                        inst->GENname, NOImin, NOImax, NOI);
                    inst->JJnoise = NOI;
                }
            }

            if (model->JJrtype == 3) {

                // 5th order polynomial model for NbN.
                //
                // cj = g0*vj + g1*vj**3 + g2*vj**5,
                // gj = dcj/dvj = g0 + 3*g1*vj**2 + 5*g2*vj**4.
                //
                // Required:
                // (1) cj(vmore) = g0*vmore + g1*vmore**3 + g2*vmore**5
                //               = Ic/factor + g0*vless
                // (2) gj(vmore) = g0 + 3*g1*vmore**2 + 5*g2*vmore**4
                //               = gs.
                // (3) gj(0) = g0 (trivially satisfied).
                //
                // define Gn = (Ic/factor + g0*vless)/vmore
                //
                // 4*g0 + 2*g1*vmore**2 = 5*Gn - gs, or
                // g1 = (5*Gn - gs - 4*g0)/(2*vmore**2)
                // g2 = (Gn - g0 - g1*vmore**2)/(vmore**4)
                //

                double Gn = (inst->JJcriti/model->JJicFactor +
                    model->JJvless*inst->JJg0)/model->JJvmore;
                double temp = model->JJvmore*model->JJvmore;
                inst->JJg1 = 0.5*(5.0*Gn - inst->JJgs - 4.0*inst->JJg0);
                inst->JJg2 = (Gn - inst->JJg0 - inst->JJg1)/(temp*temp);
                inst->JJg1 /= temp;
                inst->JJcr1 = (Gn - inst->JJgn)*model->JJvmore;
                // if the conductivity goes negative, the parameters
                // aren't good
                if (inst->JJg1 < 0.0 && 9*inst->JJg1*inst->JJg1 >
                        20*inst->JJg0*inst->JJg2) {
                    DVO.textOut(OUT_WARNING,
"%s: delv is too small for rtype=3, expect negative conductivity\nregion.\n",
                        model->GENmodName);
                }
            }
            else {
                inst->JJg1 = 0;
                inst->JJg2 = 0;
            }    

            inst->GENstate = *states;
            *states += JJnumStates;

            inst->JJinitControl = 0;
            if (inst->JJcontrolGiven) {
                inst->JJbranch = ckt->findBranch(inst->JJcontrol);

                if (inst->JJbranch == 0) {
                    DVO.textOut(OUT_WARNING,
                        "%s: control current modulated by non-existant\n"
                        "or non-branch device %s, ignored.",
                        inst->GENname, inst->JJcontrol);
                    inst->JJcontrol = 0;
                }
            }

            int error = get_node_ptr(ckt, inst);
            if (error != OK)
                return (error);

#ifdef USE_PRELOAD
            if (ckt->CKTpreload) {
                if (inst->JJphsNode > 0) {
                    ckt->preldset(inst->JJphsPhsPtr, 1.0);
                }
#ifdef NEWLSER
                if (inst->JJrealPosNode) {
                    ckt->preldset(inst->JJlserPosIbrPtr, 1.0);
                    ckt->preldset(inst->JJlserIbrPosPtr, 1.0);
                }
                if (inst->JJposNode) {
                    ckt->preldset(inst->JJlserNegIbrPtr, -1.0);
                    ckt->preldset(inst->JJlserIbrNegPtr, -1.0);
                }
#endif
#ifdef NEWLSH
                if (inst->JJlsh > 0.0) {
                    ckt->ldset(inst->JJlshPosIbrPtr, 1.0);
                    ckt->ldset(inst->JJlshIbrPosPtr, 1.0);
                    ckt->ldset(inst->JJlshNegIbrPtr, -1.0);
                    ckt->ldset(inst->JJlshIbrNegPtr, -1.0);
                }
#endif
            }
#endif
        }
    }

    return (OK);
}


// Reset the matrix element pointers.
//
int
JJdev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sJJmodel *model = (sJJmodel*)inModel; model;
            model = model->next()) {
        for (sJJinstance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}

