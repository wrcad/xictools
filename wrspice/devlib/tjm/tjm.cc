
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

#include "tjmdefs.h"


namespace {

IFparm TJMpTable[] = {
IO("area",              TJM_AREA,           IF_REAL,
                "Area factor"),
IO("ics",               TJM_ICS,            IF_REAL,
                "Critical current with scaling"),
#ifdef NEWLSER
IO("lser",              TJM_LSER,           IF_REAL|IF_IND,
                "Parasitic series inductance"),
#endif
#ifdef NEWLSH
IO("lsh",              TJM_LSH,             IF_REAL|IF_IND,
                "External shunt resistor series parasitic inductance"),
#endif
IO("off",               TJM_OFF,            IF_FLAG,
                "Shorted for dc operating point comp."),
IO("ic",                TJM_IC,             IF_REALVEC|IF_VOLT,
                "Initial VJ,PHI vector"),
IO("phi",               TJM_ICP,            IF_REAL,
                "Initial phase"),
IO("ic_phase",          TJM_ICP,            IF_REAL|IF_REDUNDANT,
                "Initial phase"),
IO("vj",                TJM_ICV,            IF_REAL|IF_VOLT,
                "Initial voltage"),
IO("ic_v",              TJM_ICV,            IF_REAL|IF_VOLT|IF_REDUNDANT,
                "Initial voltage"),
IO("control",           TJM_CON,            IF_INSTANCE,
                "Control inductor or voltage source"),
IO("noise",             TJM_NOISE,          IF_REAL,
                "Noise scaling coefficient"),
OP("v",                 TJM_QUEST_V,        IF_REAL|IF_VOLT,
                "Terminal voltage"),
OP("icrit",             TJM_QUEST_CRT,      IF_REAL|IF_AMP,
                "Maximum critical current"),
OP("cc",                TJM_QUEST_IC,       IF_REAL|IF_AMP,
                "Capacitance current"),
OP("cj",                TJM_QUEST_IJ,       IF_REAL|IF_AMP,
                "Josephson current"),
OP("cq",                TJM_QUEST_IG,       IF_REAL|IF_AMP,
                "Quasiparticle current"),
OP("c",                 TJM_QUEST_I,        IF_REAL|IF_AMP|IF_USEALL,
                "Total device current"),
OP("cap",               TJM_QUEST_CAP,      IF_REAL|IF_CAP,
                "Capacitance"),
OP("g0",                TJM_QUEST_G0,       IF_REAL|IF_COND,
                "Subgap conductance"),
OP("gn",                TJM_QUEST_GN,       IF_REAL|IF_COND,
                "Normal conductance"),
OP("gs",                TJM_QUEST_GS,       IF_REAL|IF_COND,
                "Quasiparticle onset conductance"),
OP("g1",                TJM_QUEST_G1,       IF_REAL,
                "NbN quasiparticle parameter"),
OP("g2",                TJM_QUEST_G2,       IF_REAL,
                "NbN quasiparticle parameter"),
OP("node1",             TJM_QUEST_N1,       IF_INTEGER,
                "Node 1 number"),
OP("node2",             TJM_QUEST_N2,       IF_INTEGER,
                "Node 2 number"),
OP("pnode",             TJM_QUEST_NP,       IF_INTEGER,
                "Phase node number"),
#ifdef NEWLSER
OP("lsernode",          TJM_QUEST_NI,       IF_INTEGER,
                "Internal lser node number"),
OP("lserbrn",           TJM_QUEST_NB,       IF_INTEGER,
                "Internal lser branch number")
#endif
#ifdef NEWLSH
#ifdef NEWLSER
                ,
#endif
OP("lshnode",           TJM_QUEST_NSHI,     IF_INTEGER,
                "Internal lsh node number"),
OP("lshbrn",            TJM_QUEST_NSHB,     IF_INTEGER,
                "Internal lsh branch number")
#endif
};

IFparm TJMmPTable[] = {
OP("jj",                TJM_MOD_TJM,        IF_FLAG,
                "Model name"),
IO("coeffset",          TJM_MOD_COEFFS,     IF_STRING,
                "Coefficient set name"),
IO("pijj",              TJM_MOD_PI,         IF_INTEGER,
                "Default is PI junction"),
IO("rtype",             TJM_MOD_RT,         IF_INTEGER,
                "Quasiparticle current model"),
IO("cct",               TJM_MOD_IC,         IF_INTEGER,
                "Critical current model"),
IO("vg",                TJM_MOD_VG,         IF_REAL|IF_VOLT,
                "Gap voltage"),
IO("vgap",              TJM_MOD_VG,         IF_REAL|IF_VOLT|IF_REDUNDANT,
                "Gap voltage"),
IO("delv",              TJM_MOD_DV,         IF_REAL|IF_VOLT,
                "Delta gap voltage"),
IO("icrit",             TJM_MOD_CRT,        IF_REAL|IF_AMP,
                "Max crit current"),
IO("cap",               TJM_MOD_CAP,        IF_REAL|IF_CAP,
                "Capacitance"),
IO("cpic",              TJM_MOD_CPIC,       IF_REAL,
                "Capacitance per critical current"),
IO("cmu",               TJM_MOD_CMU,        IF_REAL,
                "Capacitance scaling parameter"),
IO("vm",                TJM_MOD_VM,         IF_REAL|IF_VOLT,
                "Ic * subgap resistance, mV"),
IO("r0",                TJM_MOD_R0,         IF_REAL|IF_RES,
                "Subgap resistance"),
IO("rsub",              TJM_MOD_R0,         IF_REAL|IF_RES|IF_REDUNDANT,
                "Subgap resistance"),
IO("icrn",              TJM_MOD_ICR,        IF_REAL|IF_VOLT,
                "Ic * normal resistance, mV"),
IO("rn",                TJM_MOD_RN,         IF_REAL|IF_RES,
                "Normal resistance"),
IO("rnorm",             TJM_MOD_RN,         IF_REAL|IF_RES|IF_REDUNDANT,
                "Normal resistance"),
IO("gmu",               TJM_MOD_GMU,        IF_REAL,
                "Conductance scaling parameter"),
IO("noise",             TJM_MOD_NOISE,      IF_REAL|IF_NONSENSE,
                "Default noise scale factor"),
IO("icon",              TJM_MOD_CCS,        IF_REAL|IF_AMP,
                "Control current first null"),
IO("icfact",            TJM_MOD_ICF,        IF_REAL,
                "Ratio of Ic to qp gap current"),
IO("icfct",             TJM_MOD_ICF,        IF_REAL|IF_REDUNDANT,
                "Ratio of Ic to qp gap current"),
IO("vshunt",            TJM_MOD_VSHUNT,     IF_REAL|IF_VOLT,
                "Implied extra shunt R=Vshunt/Ic"),
#ifdef NEWLSH
IO("lsh0",              TJM_MOD_LSH0,       IF_REAL|IF_IND,
                "Shunt resistor inductance constant part"),
IO("lsh1",              TJM_MOD_LSH1,       IF_REAL|IF_TIME,
                "Shunt resistor inductance per ohm"),
#endif
IO("tsfactor",          TJM_MOD_TSFACT,     IF_REAL,
                "Phase change limit per step"),
OP("vless",             TJM_MQUEST_VL,      IF_REAL|IF_VOLT,
                "Gap threshold voltage"),
OP("vmore",             TJM_MQUEST_VM,      IF_REAL|IF_VOLT,
                "Gap knee voltage"),
OP("vdp",               TJM_MQUEST_VDP,     IF_REAL|IF_VOLT,
                "Dropback voltage"),
OP("betac",             TJM_MQUEST_BETAC,   IF_REAL,
                "Stewart-McCumber parameter"),
OP("wvg",               TJM_MQUEST_WVG,     IF_REAL,
                "Normalized gap voltage"),
OP("wvrat",             TJM_MQUEST_WVRAT,   IF_REAL,
                "Ic to quasiparticle step height ratio"),
OP("wrrat",             TJM_MQUEST_WRRAT,   IF_REAL,
                "Normal to subgap resistance ratio")
};

const char *TJMnames[] = {
    "B+",
    "B-",
    "Ph"
};

const char *TJMmodNames[] = {
    "jj",
    0
};

IFkeys TJMkeys[] = {
    IFkeys( 'b', TJMnames, 2, 3, 0 )
};

// TJM models, where A, B, and P are Dirichlet coefficients.
// Size of Dirichlet series should be even and less or equal to 20.
// Taken from MiTMoJCo (https://github.com/drgulevich/mitmojco) repository.
//
// Model BCS42_008
IFcomplex A_1[] = 
    { cIFcomplex(1.057436, -18.941930),
    cIFcomplex(1.057436, -18.941930),
    cIFcomplex(0.077953, 0.007944),
    cIFcomplex(0.006675, -0.000475),
    cIFcomplex(0.025091, 0.000289),
    cIFcomplex(1.376607, -0.000002),
    cIFcomplex(-0.459333, -0.152676),
    cIFcomplex(-0.000889, -0.041516)};
IFcomplex B_1[] =
    { cIFcomplex(0.108712, 7.209623),
    cIFcomplex(0.109049, 0.238592),
    cIFcomplex(0.072237, 0.029470),
    cIFcomplex(0.006656, -0.000208),
    cIFcomplex(0.025316, 0.002275),
    cIFcomplex(-0.217956, 0.000001),
    cIFcomplex(-0.401859, -0.545156),
    cIFcomplex(0.000643, -0.098458)};
IFcomplex P_1[] =
    { cIFcomplex(-4.373312, -0.114845),
    cIFcomplex(-0.411779, 0.898335),
    cIFcomplex(-0.139858, 0.991853),
    cIFcomplex(-0.013048, 1.000349),
    cIFcomplex(-0.043182, 1.000747),
    cIFcomplex(-1.105852, -0.000000),
    cIFcomplex(-0.651673, 0.123645),
    cIFcomplex(-0.073309, 0.000067)};
TJMcoeffSet tjm_coeffs1("tjm1", 8, A_1, B_1, P_1);

// Model BCS42_001.
IFcomplex A_2[] = 
    { cIFcomplex(-0.000935, -0.344952),
    cIFcomplex(0.002376, -0.000079),
    cIFcomplex(0.701978, -3.433012),
    cIFcomplex(0.141990, 0.034241),
    cIFcomplex(0.007165, -0.000087),
    cIFcomplex(0.000650, -0.000029),
    cIFcomplex(0.020416, 0.000395),
    cIFcomplex(0.056332, 0.006446),
    cIFcomplex(1.266591, 0.000000),
    cIFcomplex(0.187313, 0.279494)};
IFcomplex B_2[] = 
    { cIFcomplex(0.001392, -0.100648),
    cIFcomplex(0.002382, -0.000055),
    cIFcomplex(-0.258742, 0.553749),
    cIFcomplex(0.095523, 0.119127),
    cIFcomplex(0.007150, 0.000084),
    cIFcomplex(0.000649, -0.000026),
    cIFcomplex(0.020445, 0.002159),
    cIFcomplex(0.053536, 0.018137),
    cIFcomplex(-0.017427, 0.000001),
    cIFcomplex(-0.161605, 0.336628)};
IFcomplex P_2[] = 
    { cIFcomplex(-0.090721, -0.000036),
    cIFcomplex(-0.004370, 1.000126),
    cIFcomplex(-0.813405, -0.043201),
    cIFcomplex(-0.299741, 0.941648),
    cIFcomplex(-0.013468, 1.000303),
    cIFcomplex(-0.001497, 1.000022),
    cIFcomplex(-0.039953, 0.999920),
    cIFcomplex(-0.113673, 0.993161),
    cIFcomplex(-6.766647, -0.000001),
    cIFcomplex(-0.646220, 0.637507)};
TJMcoeffSet tjm_coeffs2("mitmojco_001", 10, A_2, B_2, P_2);

} // namespace

// Static Function.
TJMcoeffSet *
TJMcoeffSet::getTJMcoeffSet(const char *nm)
{
    if (!nm)
        return (0);
    if (!strcasecmp(nm, tjm_coeffs1.cfs_name))
        return (&tjm_coeffs1);
    if (!strcasecmp(nm, tjm_coeffs2.cfs_name))
        return (&tjm_coeffs2);
    return (0);
}


TJMdev::TJMdev()
{
    dv_name = "TJM";
    dv_description = "Josephson junction microscopic model";

    dv_numKeys = NUMELEMS(TJMkeys);
    dv_keys = TJMkeys;

    dv_levels[0] = 3;
    dv_levels[1] = 0;
    dv_modelKeys = TJMmodNames;

    dv_numInstanceParms = NUMELEMS(TJMpTable);
    dv_instanceParms = TJMpTable;

    dv_numModelParms = NUMELEMS(TJMmPTable);
    dv_modelParms = TJMmPTable;

#ifdef NEWJJDC
    dv_flags = (DV_TRUNC | DV_JJSTEP | DV_JJPMDC | DV_NODIST | DV_NOPZ);
#else
    dv_flags = (DV_TRUNC | DV_NOAC | DV_NODCT | DV_JJSTEP);
#endif
};


sGENmodel *
TJMdev::newModl()
{
    return (new sTJMmodel);
}


sGENinstance *
TJMdev::newInst()
{
    return (new sTJMinstance);
}


int
TJMdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sTJMmodel, sTJMinstance>(model));
}


int
TJMdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sTJMmodel, sTJMinstance>(model, dname,
        fast));
}


int
TJMdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sTJMmodel, sTJMinstance>(model, modname,
        modfast));
}


// Josephson junction parser
//   Bname <node> <node> [<node>] [<mname>]
//       [[ic=<val>,<val>] [vj=<val>] [phi=<val>]] [area=<val>]
//       [control=<val>]
// 
void
TJMdev::parse(int type, sCKT *ckt, sLine *current)
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
        *d = new TJMdev;
        (*cnt)++;
    }
}

