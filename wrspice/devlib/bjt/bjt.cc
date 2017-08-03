
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
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "bjtdefs.h"


//
// This file defines the BJT data structures that are
// available to the next level(s) up the calling hierarchy
//

namespace {

IFparm BJTpTable[] = {
IO("area",              BJT_AREA,           IF_REAL|IF_AREA,
                "Area factor"),
IO("off",               BJT_OFF,            IF_FLAG,
                "Device initially off"),
IO("temp",              BJT_TEMP,           IF_REAL|IF_TEMP,
                "Instance temperature"),
IO("icvbe",             BJT_IC_VBE,         IF_REAL|IF_VOLT|IF_AC,
                "Initial B-E voltage"),
IO("icvce",             BJT_IC_VCE,         IF_REAL|IF_VOLT|IF_AC,
                "Initial C-E voltage"),
IP("ic",                BJT_IC,             IF_REALVEC|IF_VOLT,
                "Initial VBE,VCE vector"),
OP("vbe",               BJT_QUEST_VBE,      IF_REAL|IF_VOLT,
                "B-E voltage"),
OP("vbc",               BJT_QUEST_VBC,      IF_REAL|IF_VOLT,
                "B-C voltage"),
OP("vcs",               BJT_QUEST_VCS,      IF_REAL|IF_VOLT,
                "C-Subst voltage"),
OP("ic",                BJT_QUEST_CC,       IF_REAL|IF_AMP|IF_USEALL,
                "Collector current"),
OP("cc",                BJT_QUEST_CC,       IF_REAL|IF_AMP|IF_REDUNDANT,
                "Collector current"),
OP("ib",                BJT_QUEST_CB,       IF_REAL|IF_AMP|IF_USEALL,
                "Base current"),
OP("cb",                BJT_QUEST_CB,       IF_REAL|IF_AMP|IF_REDUNDANT,
                "Base current"),
OP("ie",                BJT_QUEST_CE,       IF_REAL|IF_AMP|IF_USEALL,
                "Emitter current"),
OP("ce",                BJT_QUEST_CE,       IF_REAL|IF_AMP|IF_REDUNDANT,
                "Emitter current"),
OP("is",                BJT_QUEST_CS,       IF_REAL|IF_AMP,
                "Substrate current"),
OP("cs",                BJT_QUEST_CS,       IF_REAL|IF_AMP|IF_REDUNDANT,
                "Substrate current"),
OP("p",                 BJT_QUEST_POWER,    IF_REAL|IF_POWR,
                "Power dissipation"),
OP("ft",                BJT_QUEST_FT,       IF_REAL|IF_FREQ,
                "Frequency for unity gain"),
OP("gpi",               BJT_QUEST_GPI,      IF_REAL|IF_COND,
                "Small signal input conductance - pi"),
OP("gmu",               BJT_QUEST_GMU,      IF_REAL|IF_COND,
                "Small signal conductance - mu"),
OP("gm",                BJT_QUEST_GM,       IF_REAL|IF_COND,
                "Small signal transconductance"),
OP("go",                BJT_QUEST_GO,       IF_REAL|IF_COND,
                "Small signal output conductance"),
OP("gx",                BJT_QUEST_GX,       IF_REAL|IF_COND,
                "Conductance from base to internal base"),
OP("geqcb",             BJT_QUEST_GEQCB,    IF_REAL|IF_COND,
                "D(Ibe"),
OP("gccs",              BJT_QUEST_GCCS,     IF_REAL|IF_COND,
                "Internal C-S cap. equiv. cond."),
OP("geqbx",             BJT_QUEST_GEQBX,    IF_REAL|IF_COND,
                "Internal C-B-base cap. equiv. cond."),
OP("qbe",               BJT_QUEST_QBE,      IF_REAL|IF_CHARGE,
                "Charge storage B-E junction"),
OP("qbc",               BJT_QUEST_QBC,      IF_REAL|IF_CHARGE,
                "Charge storage B-C junction"),
OP("qcs",               BJT_QUEST_QCS,      IF_REAL|IF_CHARGE,
                "Charge storage C-S junction"),
OP("qbx",               BJT_QUEST_QBX,      IF_REAL|IF_CHARGE,
                "Charge storage B-X junction"),
OP("cqbe",              BJT_QUEST_CQBE,     IF_REAL|IF_CAP,
                "Cap. due to charge storage in B-E jct."),
OP("cqbc",              BJT_QUEST_CQBC,     IF_REAL|IF_CAP,
                "Cap. due to charge storage in B-C jct."),
OP("cqcs",              BJT_QUEST_CQCS,     IF_REAL|IF_CAP,
                "Cap. due to charge storage in C-S jct."),
OP("cqbx",              BJT_QUEST_CQBX,     IF_REAL|IF_CAP,
                "Cap. due to charge storage in B-X jct."),
OP("cexbc",             BJT_QUEST_CEXBC,    IF_REAL|IF_CAP,
                "Total Capacitance in B-X junction"),
OP("cpi",               BJT_QUEST_CPI,      IF_REAL|IF_CAP,
                "Internal base to emitter capacitance"),
OP("cmu",               BJT_QUEST_CMU,      IF_REAL|IF_CAP,
                "Internal base to collector capacitance"),
OP("cbx",               BJT_QUEST_CBX,      IF_REAL|IF_CAP,
                "Base to collector capacitance"),
OP("ccs",               BJT_QUEST_CCS,      IF_REAL|IF_CAP,
                "Collector to substrate capacitance"),
OP("colnode",           BJT_QUEST_COLNODE,  IF_INTEGER,
                "Collector node number"),
OP("basenode",          BJT_QUEST_BASENODE, IF_INTEGER,
                "Base node number"),
OP("emitnode",          BJT_QUEST_EMITNODE, IF_INTEGER,
                "Emitter node number"),
OP("substnode",         BJT_QUEST_SUBSTNODE,IF_INTEGER,
                "Substrate node number"),
OP("colprimenode",      BJT_QUEST_COLPRIMENODE,IF_INTEGER,
                "Int. collector node number"),
OP("baseprimenode",     BJT_QUEST_BASEPRIMENODE,IF_INTEGER,
                "Int. base node number"),
OP("emitprimenode",     BJT_QUEST_EMITPRIMENODE,IF_INTEGER,
                "Int. emitter node number")
};

IFparm BJTmPTable[] = {
IP("npn",               BJT_MOD_NPN,        IF_FLAG,
                "NPN type device"),
IP("pnp",               BJT_MOD_PNP,        IF_FLAG,
                "PNP type device"),
IO("is",                BJT_MOD_IS,         IF_REAL,
                "Saturation Current"),
IO("bf",                BJT_MOD_BF,         IF_REAL,
                "Ideal forward beta"),
IO("nf",                BJT_MOD_NF,         IF_REAL,
                "Forward emission coefficient"),
IO("vaf",               BJT_MOD_VAF,        IF_REAL,
                "Forward Early voltage"),
IO("va",                BJT_MOD_VAF,        IF_REAL|IF_REDUNDANT,
                "Forward Early voltage"),
IO("ikf",               BJT_MOD_IKF,        IF_REAL,
                "Forward beta roll-off corner current"),
IO("ik",                BJT_MOD_IKF,        IF_REAL|IF_REDUNDANT,
                "Forward beta roll-off corner current"),
IO("ise",               BJT_MOD_ISE,        IF_REAL,
                "B-E leakage saturation current"),
IO("ne",                BJT_MOD_NE,         IF_REAL,
                "B-E leakage emission coefficient"),
IO("br",                BJT_MOD_BR,         IF_REAL,
                "Ideal reverse beta"),
IO("nr",                BJT_MOD_NR,         IF_REAL,
                "Reverse emission coefficient"),
IO("var",               BJT_MOD_VAR,        IF_REAL,
                "Reverse Early voltage"),
IO("vb",                BJT_MOD_VAR,        IF_REAL|IF_REDUNDANT,
                "Reverse Early voltage"),
IO("ikr",               BJT_MOD_IKR,        IF_REAL,
                "Reverse beta roll-off corner current"),
IO("isc",               BJT_MOD_ISC,        IF_REAL,
                "B-C leakage saturation current"),
IO("nc",                BJT_MOD_NC,         IF_REAL,
                "B-C leakage emission coefficient"),
IO("rb",                BJT_MOD_RB,         IF_REAL,
                "Zero bias base resistance"),
IO("irb",               BJT_MOD_IRB,        IF_REAL,
                "Current for base resistance=(rb+rbm"),
IO("rbm",               BJT_MOD_RBM,        IF_REAL,
                "Minimum base resistance"),
IO("re",                BJT_MOD_RE,         IF_REAL,
                "Emitter resistance"),
IO("rc",                BJT_MOD_RC,         IF_REAL,
                "Collector resistance"),
IO("cje",               BJT_MOD_CJE,        IF_REAL|IF_AC,
                "Zero bias B-E depletion capacitance"),
IO("vje",               BJT_MOD_VJE,        IF_REAL|IF_AC,
                "B-E built in potential"),
IO("pe",                BJT_MOD_VJE,        IF_REAL|IF_REDUNDANT,
                "B-E built in potential"),
IO("mje",               BJT_MOD_MJE,        IF_REAL|IF_AC,
                "B-E junction grading coefficient"),
IO("me",                BJT_MOD_MJE,        IF_REAL|IF_REDUNDANT,
                "B-E junction grading coefficient"),
IO("tf",                BJT_MOD_TF,         IF_REAL|IF_AC,
                "Ideal forward transit time"),
IO("xtf",               BJT_MOD_XTF,        IF_REAL|IF_AC,
                "Coefficient for bias dependence of TF"),
IO("vtf",               BJT_MOD_VTF,        IF_REAL|IF_AC,
                "Voltage giving VBC dependence of TF"),
IO("itf",               BJT_MOD_ITF,        IF_REAL|IF_AC,
                "High current dependence of TF"),
IO("ptf",               BJT_MOD_PTF,        IF_REAL|IF_AC,
                "Excess phase"),
IO("cjc",               BJT_MOD_CJC,        IF_REAL|IF_AC,
                "Zero bias B-C depletion capacitance"),
IO("vjc",               BJT_MOD_VJC,        IF_REAL|IF_AC,
                "B-C built in potential"),
IO("pc",                BJT_MOD_VJC,        IF_REAL|IF_REDUNDANT,
                "B-C built in potential"),
IO("mjc",               BJT_MOD_MJC,        IF_REAL|IF_AC,
                "B-C junction grading coefficient"),
IO("mc",                BJT_MOD_MJC,        IF_REAL|IF_REDUNDANT,
                "B-C junction grading coefficient"),
IO("xcjc",              BJT_MOD_XCJC,       IF_REAL|IF_AC,
                "Fraction of B-C cap to internal base"),
IO("tr",                BJT_MOD_TR,         IF_REAL|IF_AC,
                "Ideal reverse transit time"),
IO("cjs",               BJT_MOD_CJS,        IF_REAL|IF_AC,
                "Zero bias C-S capacitance"),
IO("ccs",               BJT_MOD_CJS,        IF_REAL|IF_AC,
                "Zero bias C-S capacitance"),
IO("vjs",               BJT_MOD_VJS,        IF_REAL|IF_AC,
                "Substrate junction built in potential"),
IO("ps",                BJT_MOD_VJS,        IF_REAL|IF_REDUNDANT,
                "Substrate junction built in potential"),
IO("mjs",               BJT_MOD_MJS,        IF_REAL|IF_AC,
                "Substrate junction grading coefficient"),
IO("ms",                BJT_MOD_MJS,        IF_REAL|IF_REDUNDANT,
                "Substrate junction grading coefficient"),
IO("xtb",               BJT_MOD_XTB,        IF_REAL,
                "Forward and reverse beta temp. exp."),
IO("eg",                BJT_MOD_EG,         IF_REAL,
                "Energy gap for IS temp. dependency"),
IO("xti",               BJT_MOD_XTI,        IF_REAL,
                "Temp. exponent for IS"),
IO("fc",                BJT_MOD_FC,         IF_REAL,
                "Forward bias junction fit parameter"),
IP("tnom",              BJT_MOD_TNOM,       IF_REAL,
                "Parameter measurement temperature"),
IP("kf",                BJT_MOD_KF,         IF_REAL,
                "Flicker Noise Coefficient"),
IP("af",                BJT_MOD_AF,         IF_REAL,
                "Flicker Noise Exponent"),
OP("invearlyvoltf",     BJT_MOD_INVEARLYF,  IF_REAL,
                "Inverse early voltage:forward"),
OP("invearlyvoltr",     BJT_MOD_INVEARLYR,  IF_REAL,
                "Inverse early voltage:reverse"),
OP("invrollofff",       BJT_MOD_INVROLLOFFF,IF_REAL,
                "Inverse roll off - forward"),
OP("invrolloffr",       BJT_MOD_INVROLLOFFR,IF_REAL,
                "Inverse roll off - reverse"),
OP("collectorconduct",  BJT_MOD_COLCONDUCT, IF_REAL,
                "Collector conductance"),
OP("emitterconduct",    BJT_MOD_EMITTERCONDUCT,IF_REAL,
                "Emitter conductance"),
OP("transtimevbcfact",  BJT_MOD_TRANSVBCFACT,IF_REAL,
                "Transit time VBC factor"),
OP("excessphasefactor", BJT_MOD_EXCESSPHASEFACTOR,IF_REAL,
                "Excess phase fact."),
OP("type",              BJT_MOD_TYPE,       IF_STRING,
                "NPN or PNP")
};

const char *BJTnames[] = {
    "collector",
    "base",
    "emitter",
    "substrate"
};

const char *BJTmodNames[] = {
    "npn",
    "pnp",
    0
};

IFkeys BJTkeys[] = {
    IFkeys( 'q', BJTnames, 3, 4, 0 )
};

} // namespace


BJTdev::BJTdev()
{
    dv_name = "BJT";
    dv_description = "Bipolar junction transistor model";

    dv_numKeys = NUMELEMS(BJTkeys);
    dv_keys = BJTkeys;

    dv_levels[0] = 1;
    dv_levels[1] = 0;
    dv_modelKeys = BJTmodNames;

    dv_numInstanceParms = NUMELEMS(BJTpTable);
    dv_instanceParms = BJTpTable;

    dv_numModelParms = NUMELEMS(BJTmPTable);
    dv_modelParms = BJTmPTable;

    dv_flags = DV_TRUNC;
};


sGENmodel *
BJTdev::newModl()
{
    return (new sBJTmodel);
}


sGENinstance *
BJTdev::newInst()
{
    return (new sBJTinstance);
}


int
BJTdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sBJTmodel, sBJTinstance>(model));
}


int
BJTdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sBJTmodel, sBJTinstance>(model, dname,
        fast));
}


int
BJTdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sBJTmodel, sBJTinstance>(model, modname,
        modfast));
}


// bipolar transistor parser
// Qname <node> <node> <node> [<node>] <model> [<val>] [OFF]
//       [IC=<val>,<val>]
//
void
BJTdev::parse(int type, sCKT *ckt, sLine *current)
{
    DEV.parse(ckt, current, type, dv_keys->minTerms, dv_keys->maxTerms,
        true, "area");
}


void
BJTdev::backup(sGENmodel *mp, DEV_BKMODE m)
{
    while (mp) {
        for (sGENinstance *ip = mp->GENinstances; ip; ip = ip->GENnextInstance)
            ((sBJTinstance*)ip)->backup(m);
        mp = mp->GENnextModel;
    }
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
        *d = new BJTdev;
        (*cnt)++;
    }
}

