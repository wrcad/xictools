
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
Author: 1993 Stephen R. Whiteley
****************************************************************************/

#include "jjdefs.h"


namespace {

IFparm JJpTable[] = {
IO("area",              JJ_AREA,            IF_REAL,
                "Area factor"),
IO("ics",               JJ_ICS,             IF_REAL,
                "Critical current with scaling"),
IO("off",               JJ_OFF,             IF_FLAG,
                "Shorted for dc operating point comp."),
IO("ic",                JJ_IC,              IF_REALVEC|IF_VOLT,
                "Initial VJ,PHI vector"),
IO("phi",               JJ_ICP,             IF_REAL,
                "Initial phase"),
IO("ic_phase",          JJ_ICP,             IF_REAL|IF_REDUNDANT,
                "Initial phase"),
IO("vj",                JJ_ICV,             IF_REAL|IF_VOLT,
                "Initial voltage"),
IO("ic_v",              JJ_ICV,             IF_REAL|IF_VOLT|IF_REDUNDANT,
                "Initial voltage"),
IO("control",           JJ_CON,             IF_INSTANCE,
                "Control inductor or voltage source"),
IO("noise",             JJ_NOISE,           IF_REAL,
                "Noise scaling coefficient"),
OP("v",                 JJ_QUEST_V,         IF_REAL|IF_VOLT,
                "Terminal voltage"),
OP("icrit",             JJ_QUEST_CRT,       IF_REAL|IF_AMP,
                "Maximum critical current"),
OP("cc",                JJ_QUEST_IC,        IF_REAL|IF_AMP,
                "Capacitance current"),
OP("cj",                JJ_QUEST_IJ,        IF_REAL|IF_AMP,
                "Josephson current"),
OP("cq",                JJ_QUEST_IG,        IF_REAL|IF_AMP,
                "Quasiparticle current"),
OP("c",                 JJ_QUEST_I,         IF_REAL|IF_AMP|IF_USEALL,
                "Total device current"),
OP("cap",               JJ_QUEST_CAP,       IF_REAL|IF_CAP,
                "Capacitance"),
OP("g0",                JJ_QUEST_G0,        IF_REAL|IF_COND,
                "Subgap conductance"),
OP("gn",                JJ_QUEST_GN,        IF_REAL|IF_COND,
                "Normal conductance"),
OP("gs",                JJ_QUEST_GS,        IF_REAL|IF_COND,
                "Quasiparticle onset conductance"),
OP("g1",                JJ_QUEST_G1,        IF_REAL,
                "NbN quasiparticle parameter"),
OP("g2",                JJ_QUEST_G2,        IF_REAL,
                "NbN quasiparticle parameter"),
OP("node1",             JJ_QUEST_N1,        IF_INTEGER,
                "Node 1 number"),
OP("node2",             JJ_QUEST_N2,        IF_INTEGER,
                "Node 2 number"),
OP("pnode",             JJ_QUEST_NP,        IF_INTEGER,
                "Phase node number")
};

IFparm JJmPTable[] = {
OP("jj",                JJ_MOD_JJ,          IF_FLAG,
                "Model name"),
IO("pijj",              JJ_MOD_PI,          IF_INTEGER,
                "Default is PI junction"),
IO("rtype",             JJ_MOD_RT,          IF_INTEGER,
                "Quasiparticle current model"),
IO("cct",               JJ_MOD_IC,          IF_INTEGER,
                "Critical current model"),
IO("vg",                JJ_MOD_VG,          IF_REAL|IF_VOLT,
                "Gap voltage"),
IO("vgap",              JJ_MOD_VG,          IF_REAL|IF_VOLT|IF_REDUNDANT,
                "Gap voltage"),
IO("delv",              JJ_MOD_DV,          IF_REAL|IF_VOLT,
                "Delta gap voltage"),
IO("icrit",             JJ_MOD_CRT,         IF_REAL|IF_AMP,
                "Max crit current"),
IO("cap",               JJ_MOD_CAP,         IF_REAL|IF_CAP,
                "Capacitance"),
IO("cmu",               JJ_MOD_CMU,         IF_REAL,
                "Capacitance scaling parameter"),
IO("vm",                JJ_MOD_VM,          IF_REAL|IF_VOLT,
                "Ic * subgap resistance, mV"),
IO("r0",                JJ_MOD_R0,          IF_REAL|IF_RES,
                "Subgap resistance"),
IO("rsub",              JJ_MOD_R0,          IF_REAL|IF_RES|IF_REDUNDANT,
                "Subgap resistance"),
IO("icrn",              JJ_MOD_ICR,         IF_REAL|IF_VOLT,
                "Ic * normal resistance, mV"),
IO("rn",                JJ_MOD_RN,          IF_REAL|IF_RES,
                "Normal resistance"),
IO("rnorm",             JJ_MOD_RN,          IF_REAL|IF_RES|IF_REDUNDANT,
                "Normal resistance"),
IO("gmu",               JJ_MOD_GMU,         IF_REAL,
                "Conductance scaling parameter"),
IO("noise",             JJ_MOD_NOISE,       IF_REAL|IF_NONSENSE,
                "Default noise scale factor"),
IO("icon",              JJ_MOD_CCS,         IF_REAL|IF_AMP,
                "Control current first null"),
IO("icfact",            JJ_MOD_ICF,         IF_REAL,
                "Ratio of Ic to qp gap current"),
IO("icfct",             JJ_MOD_ICF,         IF_REAL|IF_REDUNDANT,
                "Ratio of Ic to qp gap current"),
IO("vshunt",            JJ_MOD_VSHUNT,      IF_REAL|IF_VOLT,
                "Implied extra shunt R=Vshunt/Ic"),
IO("tsfactor",          JJ_MOD_TSFACT,      IF_REAL,
                "Phase change limit per step"),
OP("vless",             JJ_MQUEST_VL,       IF_REAL|IF_VOLT,
                "Gap threshold voltage"),
OP("vmore",             JJ_MQUEST_VM,       IF_REAL|IF_VOLT,
                "Gap knee voltage"),
OP("vdp",               JJ_MQUEST_VDP,      IF_REAL|IF_VOLT,
                "Dropback voltage")
};

const char *JJnames[] = {
    "B+",
    "B-",
    "Ph"
};

const char *JJmodNames[] = {
    "jj",
    0
};

IFkeys JJkeys[] = {
    IFkeys( 'b', JJnames, 2, 3, 0 )
};

} // namespace


JJdev::JJdev()
{
    dv_name = "JJ";
    dv_description = "Josephson junction model";

    dv_numKeys = NUMELEMS(JJkeys);
    dv_keys = JJkeys;

    dv_levels[0] = 1;
    dv_levels[1] = 0;
    dv_modelKeys = JJmodNames;

    dv_numInstanceParms = NUMELEMS(JJpTable);
    dv_instanceParms = JJpTable;

    dv_numModelParms = NUMELEMS(JJmPTable);
    dv_modelParms = JJmPTable;

#ifdef NEWJJDC
    dv_flags = (DV_TRUNC | DV_JJSTEP | DV_JJPMDC | DV_NODIST | DV_NOPZ);
#else
    dv_flags = (DV_TRUNC | DV_NOAC | DV_NODCT | DV_JJSTEP);
#endif
};


sGENmodel *
JJdev::newModl()
{
    return (new sJJmodel);
}


sGENinstance *
JJdev::newInst()
{
    return (new sJJinstance);
}


int
JJdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sJJmodel, sJJinstance>(model));
}


int
JJdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sJJmodel, sJJinstance>(model, dname,
        fast));
}


int
JJdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sJJmodel, sJJinstance>(model, modname,
        modfast));
}


// Josephson junction parser
//   Bname <node> <node> [<node>] [<mname>]
//       [[ic=<val>,<val>] [vj=<val>] [phi=<val>]] [area=<val>]
//       [control=<val>]
// 
void
JJdev::parse(int type, sCKT *ckt, sLine *current)
{
    DEV.parse(ckt, current, type, dv_keys->minTerms, dv_keys->maxTerms,
        true, "area");
}


// Below is hugely GCC-specific.  The __WRMODULE__ and __WRVERSION__
// tokens are defined in the Makefile and passed with -D when
// compiling.

#define STR(x) #x
#define STRINGIFY(x) STR(x)
#define MCAT(x, y) x ## y
#define MODNAME(x, y) MCAT(x, y)

// Module initializer.  Sets locations in the main app to some
// identifying strings.
//
__attribute__((constructor)) static void initializer()
{
    extern const char *WRS_ModuleName, *WRS_ModuleVersion;

    WRS_ModuleName = STRINGIFY(__WRMODULE__);
    WRS_ModuleVersion = STRINGIFY(__WRVERSION__);
}


// Device constructor function.  This should be the only globally
// visible symbol in the module.  The function name expands to the
// module name with trailing _c.
// 
extern "C" {
    void
    MODNAME(__WRMODULE__, _c)(IFdevice **d, int *cnt)
    {
        *d = new JJdev;
        (*cnt)++;
    }
}

