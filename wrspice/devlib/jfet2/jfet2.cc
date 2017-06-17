
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Device Library                                                         *
 *                                                                        *
 *========================================================================*
 $Id: jfet2.cc,v 2.24 2016/09/26 01:48:15 stevew Exp $
 *========================================================================*/

/**********
Based on jfet.c 
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles

Modified to add PS model and new parameter definitions ( Anthony E. Parker )
   Copyright 1994  Macquarie University, Sydney Australia.
   10 Feb 1994:  Parameter definitions called from jfetparm.h
                 Extra state vectors added to JFET2pTable
**********/

#include "jfet2defs.h"


namespace {

IFparm JFET2pTable[] = {
IO("off",               JFET2_OFF,          IF_FLAG,
                "Device initially off"),
IP("ic",                JFET2_IC,           IF_REALVEC|IF_VOLT,
                "Initial VDS,VGS vector"),
IO("area",              JFET2_AREA,         IF_REAL|IF_AREA,
                "Area factor"),
IO("icvds",             JFET2_IC_VDS,       IF_REAL|IF_VOLT|IF_AC,
                "Initial D-S voltage"),
IO("ic-vds",            JFET2_IC_VDS,       IF_REAL|IF_VOLT|IF_AC,
                "Initial D-S voltage"),
IO("icvgs",             JFET2_IC_VGS,       IF_REAL|IF_VOLT|IF_AC,
                "Initial G-S volrage"),
IO("ic-vgs",            JFET2_IC_VGS,       IF_REAL|IF_VOLT|IF_AC,
                "Initial G-S volrage"),
IO("temp",              JFET2_TEMP,         IF_REAL|IF_TEMP,
                "Instance temperature"),
OP("drain-node",        JFET2_DRAINNODE,    IF_INTEGER,
                "Number of drain node"),
OP("gate-node",         JFET2_GATENODE,     IF_INTEGER,
                "Number of gate node"),
OP("source-node",       JFET2_SOURCENODE,   IF_INTEGER,
                "Number of source node"),
OP("drain-prime-node",  JFET2_DRAINPRIMENODE,IF_INTEGER,
                "Internal drain node"),
OP("source-prime-node", JFET2_SOURCEPRIMENODE,IF_INTEGER,
                "Internal source node"),
OP("vgs",               JFET2_VGS,          IF_REAL|IF_VOLT,
                "Voltage G-S"),
OP("vgd",               JFET2_VGD,          IF_REAL|IF_VOLT,
                "Voltage G-D"),
OP("ig",                JFET2_CG,           IF_REAL|IF_AMP|IF_USEALL,
                "Current at gate node"),
OP("id",                JFET2_CD,           IF_REAL|IF_AMP|IF_USEALL,
                "Current at drain node"),
OP("is",                JFET2_CS,           IF_REAL|IF_AMP|IF_USEALL,
                "Source current"),
OP("igd",               JFET2_CGD,          IF_REAL|IF_AMP,
                "Current G-D"),
OP("gm",                JFET2_GM,           IF_REAL|IF_COND,
                "Transconductance"),
OP("gds",               JFET2_GDS,          IF_REAL|IF_COND,
                "Conductance D-S"),
OP("ggs",               JFET2_GGS,          IF_REAL|IF_COND,
                "Conductance G-S"),
OP("ggd",               JFET2_GGD,          IF_REAL|IF_COND,
                "Conductance G-D"),
OP("qgs",               JFET2_QGS,          IF_REAL|IF_CHARGE,
                "Charge storage G-S junction"),
OP("qgd",               JFET2_QGD,          IF_REAL|IF_CHARGE,
                "Charge storage G-D junction"),
OP("cqgs",              JFET2_CQGS,         IF_REAL|IF_CAP,
                "Capacitance due to charge storage G-S junction"),
OP("cqgd",              JFET2_CQGD,         IF_REAL|IF_CAP,
                "Capacitance due to charge storage G-D junction"),
OP("p",                 JFET2_POWER,        IF_REAL|IF_POWR,
                "Power dissipated by the JFET2"),
OP("vtrap",             JFET2_VTRAP,        IF_REAL|IF_VOLT,
                "Quiescent drain feedback potential"),
OP("vpave",             JFET2_PAVE,         IF_REAL|IF_POWR,
                "Quiescent power dissipation")
};

IFparm JFET2mPTable[] = {
OP("type",              JFET2_MOD_TYPE,     IF_STRING,
                "N-type or P-type JFET2 model"),
IO("njf",               JFET2_MOD_NJF,      IF_FLAG,
                "N type JFET2 model"),
IO("pjf",               JFET2_MOD_PJF,      IF_FLAG,
                "P type JFET2 model"),
IO("vt0",               JFET2_MOD_VTO,      IF_REAL,
                "Threshold voltage"),
IO("vbi",               JFET2_MOD_PB,       IF_REAL,
                "Gate junction potential"),

IO("acgam",             JFET2_MOD_ACGAM,    IF_REAL,
                ""),
IO("af",                JFET2_MOD_AF,       IF_REAL,
                "Flicker Noise Exponent"),
IO("beta",              JFET2_MOD_BETA,     IF_REAL,
                "Transconductance parameter"),
IO("cds",               JFET2_MOD_CDS,      IF_REAL,
                "D-S junction capacitance"),
IO("cgd",               JFET2_MOD_CGD,      IF_REAL,
                "G-D junction capacitance"),
IO("cgs",               JFET2_MOD_CGS,      IF_REAL,
                "G-S junction capacitance"),
IO("delta",             JFET2_MOD_DELTA,    IF_REAL,
                "Coef of thermal current reduction"),
IO("hfeta",             JFET2_MOD_HFETA,    IF_REAL,
                "Drain feedback modulation"),
IO("hfe1",              JFET2_MOD_HFE1,     IF_REAL,
                ""),
IO("hfe2",              JFET2_MOD_HFE2,     IF_REAL,
                ""),
IO("hfg1",              JFET2_MOD_HFG1,     IF_REAL,
                ""),
IO("hfg2",              JFET2_MOD_HFG2,     IF_REAL,
                ""),
IO("mvst",              JFET2_MOD_MVST,     IF_REAL,
                "Modulation index for subtreshold current"),
IO("mxi",               JFET2_MOD_MXI,      IF_REAL,
                "Saturation potential modulation parameter"),
IO("fc",                JFET2_MOD_FC,       IF_REAL,
                "Forward bias junction fit parm."),
IO("ibd",               JFET2_MOD_IBD,      IF_REAL,
                "Breakdown current of diode jnc"),
IO("is",                JFET2_MOD_IS,       IF_REAL,
                "Gate junction saturation current"),
IO("kf",                JFET2_MOD_KF,       IF_REAL,
                "Flicker Noise Coefficient"),
IO("lambda",            JFET2_MOD_LAMBDA,   IF_REAL,
                "Channel length modulation param."),
IO("lfgam",             JFET2_MOD_LFGAM,    IF_REAL,
                "Drain feedback parameter"),
IO("lfg1",              JFET2_MOD_LFG1,     IF_REAL,
                ""),
IO("lfg2",              JFET2_MOD_LFG2,     IF_REAL,
                ""),
IO("n",                 JFET2_MOD_N,        IF_REAL,
                "Gate junction ideality factor"),
IO("p",                 JFET2_MOD_P,        IF_REAL,
                "Power law (triode region"),
IO("pb",                JFET2_MOD_PB,       IF_REAL,
                "Gate junction potential"),
IO("q",                 JFET2_MOD_Q,        IF_REAL,
                "Power Law (Saturated region"),
IO("rd",                JFET2_MOD_RD,       IF_REAL,
                "Drain ohmic resistance"),
IO("rs",                JFET2_MOD_RS,       IF_REAL,
                "Source ohmic resistance"),
IO("taud",              JFET2_MOD_TAUD,     IF_REAL,
                "Thermal relaxation time"),
IO("taug",              JFET2_MOD_TAUG,     IF_REAL,
                "Drain feedback relaxation time"),
IO("vbd",               JFET2_MOD_VBD,      IF_REAL,
                "Breakdown potential of diode jnc"),
IO("ver",               JFET2_MOD_VER,      IF_REAL,
                "Version number of PS model"),
IO("vst",               JFET2_MOD_VST,      IF_REAL,
                "Crit Poten subthreshold conductn"),
IO("vto",               JFET2_MOD_VTO,      IF_REAL,
                "Threshold voltage"),
IO("xc",                JFET2_MOD_XC,       IF_REAL,
                "Amount of cap. red at pinch-off"),
IO("xi",                JFET2_MOD_XI,       IF_REAL,
                "Velocity saturation index"),
IO("z",                 JFET2_MOD_Z,        IF_REAL,
                "Rate of velocity saturation"),
IO("hfgam",             JFET2_MOD_HFGAM,    IF_REAL,
                "High freq drain feedback parm"),

OP("gd",                JFET2_MOD_DRAINCONDUCT,IF_REAL,
                "Drain conductance"),
OP("gs",                JFET2_MOD_SOURCECONDUCT,IF_REAL,
                "Source conductance"),
IO("tnom",              JFET2_MOD_TNOM,     IF_REAL,
                "Parameter measurement temperature")
};

const char *JFET2names[] = {
    "Drain",
    "Gate",
    "Source"
};

const char *JFET2modNames[] = {
    "njf",
    "pjf",
    0
};

IFkeys JFET2keys[] = {
    IFkeys( 'j', JFET2names, 3, 3, 0 )
};

} // namespace


JFET2dev::JFET2dev()
{
    dv_name = "JFET2";
    dv_description = "Short channel field effect transistor",

    dv_numKeys = NUMELEMS(JFET2keys);
    dv_keys = JFET2keys;

    dv_levels[0] = 2;
    dv_levels[1] = 0;
    dv_modelKeys = JFET2modNames;

    dv_numInstanceParms = NUMELEMS(JFET2pTable);
    dv_instanceParms = JFET2pTable;

    dv_numModelParms = NUMELEMS(JFET2mPTable);
    dv_modelParms = JFET2mPTable;

    dv_flags = DV_TRUNC;
};


sGENmodel *
JFET2dev::newModl()
{
    return (new sJFET2model);
}


sGENinstance *
JFET2dev::newInst()
{
    return (new sJFET2instance);
}


int
JFET2dev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sJFET2model, sJFET2instance>(model));
}


int
JFET2dev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sJFET2model, sJFET2instance>(model, dname,
        fast));
}


int
JFET2dev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sJFET2model, sJFET2instance>(model, modname,
        modfast));
}


// JFET parser
// Jname <node> <node> <node> <model> [<val>] [OFF] [IC=<val>,<val>]
//
void
JFET2dev::parse(int type, sCKT *ckt, sLine *current)
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
        *d = new JFET2dev;
        (*cnt)++;
    }
}

