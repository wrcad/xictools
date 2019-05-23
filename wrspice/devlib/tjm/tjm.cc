
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
IO("noise",             TJM_NOISE,          IF_REAL,
                "Noise scaling coefficient"),
OP("v",                 TJM_QUEST_V,        IF_REAL|IF_VOLT,
                "Terminal voltage"),
OP("phase",             TJM_QUEST_PHS,      IF_REAL,
                "Junction phase"),
OP("phs",               TJM_QUEST_PHS,      IF_REAL,
                "Junction phase"),
OP("n",                 TJM_QUEST_PHSN,     IF_INTEGER,
                "Junction SFQ pulse count"),
OP("phsf",              TJM_QUEST_PHSF,     IF_INTEGER,
                "Junction SFQ pulse flag"),
OP("phst",              TJM_QUEST_PHST,     IF_REAL|IF_TIME,
                "Junction SFQ last pulse time"),
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
IO("rtype",             TJM_MOD_RTP,        IF_INTEGER,
                "Quasiparticle current enabled"),
IO("cct",               TJM_MOD_CTP,        IF_INTEGER,
                "Critical current enabled"),
IO("vg",                TJM_MOD_VG,         IF_REAL|IF_VOLT,
                "Gap voltage"),
IO("vgap",              TJM_MOD_VG,         IF_REAL|IF_VOLT|IF_REDUNDANT,
                "Gap voltage"),
IO("icrit",             TJM_MOD_CRT,        IF_REAL|IF_AMP,
                "Reference critical current"),
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
IO("gmu",               TJM_MOD_GMU,        IF_REAL,
                "Conductance scaling parameter"),
IO("noise",             TJM_MOD_NOISE,      IF_REAL|IF_NONSENSE,
                "Default noise scale factor"),
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
                "Phase change max per time step per 2pi"),
IO("tsaccel",           TJM_MOD_TSACCL,     IF_REAL,
                "Ratio max time step to that at dropback voltage"),

OP("vdp",               TJM_MQUEST_VDP,     IF_REAL|IF_VOLT,
                "Dropback voltage"),
OP("omegaj",            TJM_MQUEST_OMEGAJ,  IF_REAL|IF_FREQ,
                "Plasma resonance frequency, radians"),
OP("betac",             TJM_MQUEST_BETAC,   IF_REAL,
                "Stewart-McCumber parameter")
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

} // namespace


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

