
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

/**********
STAG version 2.6
Copyright 2000 owned by the United Kingdom Secretary of State for Defence
acting through the Defence Evaluation and Research Agency.
Developed by :     Jim Benson,
                   Department of Electronics and Computer Science,
                   University of Southampton,
                   United Kingdom.
With help from :   Nele D'Halleweyn, Bill Redman-White, and Craig Easson.

Based on STAG version 2.1
Developed by :     Mike Lee,
With help from :   Bernard Tenbroek, Bill Redman-White, Mike Uren, Chris Edwards
                   and John Bunyan.
Acknowledgements : Rupert Howes and Pete Mole.
**********/

#include "soi3defs.h"


namespace {

IFparm SOI3pTable[] = {
IO("l",                 SOI3_L,             IF_REAL|IF_LEN,
                "Length"),
IO("w",                 SOI3_W,             IF_REAL|IF_LEN,
                "Width"),
IO("nrd",               SOI3_NRD,           IF_REAL,
                "Drain squares"),
IO("nrs",               SOI3_NRS,           IF_REAL,
                "Source squares"),
IP("off",               SOI3_OFF,           IF_FLAG,
                "Device initially off"),
IO("icvds",             SOI3_IC_VDS,        IF_REAL|IF_VOLT,
                "Initial D-S voltage"),
IO("icvgfs",            SOI3_IC_VGFS,       IF_REAL|IF_VOLT,
                "Initial GF-S voltage"),
IO("icvgbs",            SOI3_IC_VGBS,       IF_REAL|IF_VOLT,
                "Initial GB-S voltage"),
IO("icvbs",             SOI3_IC_VBS,        IF_REAL|IF_VOLT,
                "Initial B-S voltage"),
IO("temp",              SOI3_TEMP,          IF_REAL|IF_TEMP,
                "Instance temperature"),
IO("rt",                SOI3_RT,            IF_REAL,
                "Instance Lumped Thermal Resistance"),
IO("ct",                SOI3_CT,            IF_REAL,
                "Instance Lumped Thermal Capacitance"),
IO("rt1",               SOI3_RT1,           IF_REAL,
                "Second Thermal Resistance"),
IO("ct1",               SOI3_CT1,           IF_REAL,
                "Second Thermal Capacitance"),
IO("rt2",               SOI3_RT2,           IF_REAL,
                "Third Thermal Resistance"),
IO("ct2",               SOI3_CT2,           IF_REAL,
                "Third Thermal Capacitance"),
IO("rt3",               SOI3_RT3,           IF_REAL,
                "Fourth Thermal Resistance"),
IO("ct3",               SOI3_CT3,           IF_REAL,
                "Fourth Thermal Capacitance"),
IO("rt4",               SOI3_RT4,           IF_REAL,
                "Fifth Thermal Resistance"),
IO("ct4",               SOI3_CT4,           IF_REAL,
                "Fifth Thermal Capacitance"),

IP("ic",                SOI3_IC,            IF_REALVEC|IF_VOLT,
                "Vector of D-S, GF-S, GB-S, B-S voltages"),
OP("dnode",             SOI3_DNODE,         IF_INTEGER,
                "Number of the drain node"),
OP("gfnode",            SOI3_GFNODE,        IF_INTEGER,
                "Number of frt. gate node"),
OP("snode",             SOI3_SNODE,         IF_INTEGER,
                "Number of the source node"),
OP("gbnode",            SOI3_GBNODE,        IF_INTEGER,
                "Number of back gate node"),
OP("bnode",             SOI3_BNODE,         IF_INTEGER,
                "Number of the body node"),
OP("dnodeprime",        SOI3_DNODEPRIME,    IF_INTEGER,
                "Number of int. drain node"),
OP("snodeprime",        SOI3_SNODEPRIME,    IF_INTEGER,
                "Number of int. source node"),
OP("tnode",             SOI3_TNODE,         IF_INTEGER,
                "Number of thermal node"),
OP("branch",            SOI3_BRANCH,        IF_INTEGER,
                "Number of thermal branch"),
OP("sourceconductance", SOI3_SOURCECONDUCT, IF_REAL|IF_COND,
                "Conductance of source"),
OP("drainconductance",  SOI3_DRAINCONDUCT,  IF_REAL|IF_COND,
                "Conductance of drain"),
OP("von",               SOI3_VON,           IF_REAL|IF_VOLT,
                "Effective Threshold Voltage (von"),
OP("vfbf",              SOI3_VFBF,          IF_REAL|IF_VOLT,
                "Temperature adjusted flat band voltage"),
OP("vdsat",             SOI3_VDSAT,         IF_REAL|IF_VOLT,
                "Saturation drain voltage"),
OP("sourcevcrit",       SOI3_SOURCEVCRIT,   IF_REAL|IF_VOLT,
                "Critical source voltage"),
OP("drainvcrit",        SOI3_DRAINVCRIT,    IF_REAL|IF_VOLT,
                "Critical drain voltage"),
OP("id",                SOI3_ID,            IF_REAL|IF_AMP|IF_USEALL,
                "Drain current"),
OP("ibs",               SOI3_IBS,           IF_REAL|IF_AMP,
                "B-S junction current"),
OP("ibd",               SOI3_IBD,           IF_REAL|IF_AMP,
                "B-D junction current"),
OP("gmbs",              SOI3_GMBS,          IF_REAL|IF_COND,
                "Bulk-Source transconductance"),
OP("gmf",               SOI3_GMF,           IF_REAL|IF_COND,
                "Front transconductance"),
OP("gmb",               SOI3_GMB,           IF_REAL|IF_COND,
                "Back transconductance"),
OP("gds",               SOI3_GDS,           IF_REAL|IF_COND,
                "Drain-Source conductance"),
OP("gbd",               SOI3_GBD,           IF_REAL|IF_COND,
                "Bulk-Drain conductance"),
OP("gbs",               SOI3_GBS,           IF_REAL|IF_COND,
                "Bulk-Source conductance"),
OP("capbd",             SOI3_CAPBD,         IF_REAL|IF_CAP,
                "Bulk-Drain capacitance"),
OP("capbs",             SOI3_CAPBS,         IF_REAL|IF_CAP,
                "Bulk-Source capacitance"),
OP("cbd0",              SOI3_CAPZEROBIASBD, IF_REAL|IF_CAP,
                "Zero-Bias B-D junction capacitance"),
OP("cbs0",              SOI3_CAPZEROBIASBS, IF_REAL|IF_CAP,
                "Zero-Bias B-S junction capacitance"),
OP("vbd",               SOI3_VBD,           IF_REAL|IF_VOLT,
                "Bulk-Drain voltage"),
OP("vbs",               SOI3_VBS,           IF_REAL|IF_VOLT,
                "Bulk-Source voltage"),
OP("vgfs",              SOI3_VGFS,          IF_REAL|IF_VOLT,
                "Front gate-Source voltage"),
OP("vgbs",              SOI3_VGBS,          IF_REAL|IF_VOLT,
                "Back gate-Source voltage"),
OP("vds",               SOI3_VDS,           IF_REAL|IF_VOLT,
                "Drain-Source voltage"),
OP("qgf",               SOI3_QGF,           IF_REAL|IF_CHARGE,
                "Front Gate charge storage"),
OP("iqgf",              SOI3_IQGF,          IF_REAL|IF_AMP,
                "Current due to front gate charge storage"),
OP("qd",                SOI3_QD,            IF_REAL|IF_CHARGE,
                "Drain charge storage"),
OP("iqd",               SOI3_IQD,           IF_REAL|IF_AMP,
                "Current due to drain charge storage"),
OP("qs",                SOI3_QS,            IF_REAL|IF_CHARGE,
                "Source charge storage"),
OP("iqs",               SOI3_IQS,           IF_REAL|IF_AMP,
                "Current due to source charge storage"),
OP("qbd",               SOI3_QBD,           IF_REAL|IF_CHARGE,
                "Bulk-Drain charge storage"),
OP("iqbd",              SOI3_IQBD,          IF_REAL|IF_AMP,
                "Current due to bulk-drain charge storage"),
OP("qbs",               SOI3_QBS,           IF_REAL|IF_CHARGE,
                "Bulk-Source charge storage"),
OP("iqbs",              SOI3_IQBS,          IF_REAL|IF_AMP,
                "Currnet due to bulk-source charge storage"),
OP("vfbb",              SOI3_VFBB,          IF_REAL|IF_VOLT,
                "Temperature adjusted back flat band voltage")
};

IFparm SOI3mPTable[] = {
IO("vto",               SOI3_MOD_VTO,       IF_REAL,
                "Threshold voltage"),
IO("vt0",               SOI3_MOD_VTO,       IF_REAL,
                "Threshold voltage"),
IO("vfbf",              SOI3_MOD_VFBF,      IF_REAL,
                "Flat band voltage"),
IO("kp",                SOI3_MOD_KP,        IF_REAL,
                "Transconductance parameter"),
IO("gamma",             SOI3_MOD_GAMMA,     IF_REAL,
                "Body Factor"),
IO("phi",               SOI3_MOD_PHI,       IF_REAL,
                "Surface potential"),
IO("lambda",            SOI3_MOD_LAMBDA,    IF_REAL,
                "Channel length modulation"),
IO("theta",             SOI3_MOD_THETA,     IF_REAL,
                "Vertical field mobility degradation"),
IO("rd",                SOI3_MOD_RD,        IF_REAL,
                "Drain ohmic resistance"),
IO("rs",                SOI3_MOD_RS,        IF_REAL,
                "Source ohmic resistance"),
IO("cbd",               SOI3_MOD_CBD,       IF_REAL,
                "B-D junction capacitance"),
IO("cbs",               SOI3_MOD_CBS,       IF_REAL,
                "B-S junction capacitance"),
IO("is",                SOI3_MOD_IS,        IF_REAL,
                "Bulk junction sat. current"),
IO("is1",               SOI3_MOD_IS1,       IF_REAL,
                "2nd Bulk junction sat. current"),
IO("pb",                SOI3_MOD_PB,        IF_REAL,
                "Bulk junction potential"),
IO("cgfso",             SOI3_MOD_CGFSO,     IF_REAL,
                "Front Gate-source overlap cap."),
IO("cgfdo",             SOI3_MOD_CGFDO,     IF_REAL,
                "Front Gate-drain overlap cap."),
IO("cgfbo",             SOI3_MOD_CGFBO,     IF_REAL,
                "Front Gate-bulk overlap cap."),
IO("cgbso",             SOI3_MOD_CGBSO,     IF_REAL,
                "Back Gate-source overlap cap."),
IO("cgbdo",             SOI3_MOD_CGBDO,     IF_REAL,
                "Back Gate-drain overlap cap."),
IO("cgb_bo",            SOI3_MOD_CGB_BO,    IF_REAL,
                "Back Gate-bulk overlap cap."),
IO("rsh",               SOI3_MOD_RSH,       IF_REAL,
                "Sheet resistance"),
IO("cj",                SOI3_MOD_CJSW,      IF_REAL,
                "Side junction cap per area"),
IO("mj",                SOI3_MOD_MJSW,      IF_REAL,
                "Side grading coefficient"),
IO("js",                SOI3_MOD_JS,        IF_REAL,
                "Bulk jct. sat. current density"),
IO("js1",               SOI3_MOD_JS1,       IF_REAL,
                "2nd Bulk jct. sat. current density"),
IO("tof",               SOI3_MOD_TOF,       IF_REAL,
                "Front Oxide thickness"),
IO("tob",               SOI3_MOD_TOB,       IF_REAL,
                "Back Oxide thickness"),
IO("tb",                SOI3_MOD_TB,        IF_REAL,
                "Bulk film thickness"),
IO("ld",                SOI3_MOD_LD,        IF_REAL,
                "Lateral diffusion"),
IO("u0",                SOI3_MOD_U0,        IF_REAL,
                "Surface mobility"),
IO("uo",                SOI3_MOD_U0,        IF_REAL,
                "Surface mobility"),
IO("fc",                SOI3_MOD_FC,        IF_REAL,
                "Forward bias jct. fit parm."),
IP("nsoi",              SOI3_MOD_NSOI3,     IF_FLAG,
                "N type SOI3fet model"),
IP("psoi",              SOI3_MOD_PSOI3,     IF_FLAG,
                "P type SOI3fet model"),
IO("kox",               SOI3_MOD_KOX,       IF_REAL,
                "Oxide thermal conductivity"),
IO("shsi",              SOI3_MOD_SHSI,      IF_REAL,
                "Specific heat of silicon"),
IO("dsi",               SOI3_MOD_DSI,       IF_REAL,
                "Density of silicon"),
IO("nsub",              SOI3_MOD_NSUB,      IF_REAL,
                "Substrate doping"),
IO("tpg",               SOI3_MOD_TPG,       IF_INTEGER,
                "Gate type"),
IO("nqff",              SOI3_MOD_NQFF,      IF_REAL,
                "Front fixed oxide charge density"),
IO("nqfb",              SOI3_MOD_NQFB,      IF_REAL,
                "Back fixed oxide charge density"),
IO("nssf",              SOI3_MOD_NSSF,      IF_REAL,
                "Front surface state density"),
IO("nssb",              SOI3_MOD_NSSB,      IF_REAL,
                "Back surface state density"),
IO("tnom",              SOI3_MOD_TNOM,      IF_REAL,
                "Parameter measurement temp"),
IP("kf",                SOI3_MOD_KF,        IF_REAL,
                "Flicker noise coefficient"),
IP("af",                SOI3_MOD_AF,        IF_REAL,
                "Flicker noise exponent"),
IO("sigma",             SOI3_MOD_SIGMA,     IF_REAL,
                "DIBL coefficient"),
IO("chifb",             SOI3_MOD_CHIFB,     IF_REAL,
                "Temperature coeff of flatband voltage"),
IO("chiphi",            SOI3_MOD_CHIPHI,    IF_REAL,
                "Temperature coeff of PHI"),
IO("deltaw",            SOI3_MOD_DELTAW,    IF_REAL,
                "Narrow width factor"),
IO("deltal",            SOI3_MOD_DELTAL,    IF_REAL,
                "Short channel factor"),
IO("vsat",              SOI3_MOD_VSAT,      IF_REAL,
                "Saturation velocity"),
IO("k",                 SOI3_MOD_K,         IF_REAL,
                "Thermal exponent"),
IO("lx",                SOI3_MOD_LX,        IF_REAL,
                "Channel length modulation (alternative"),
IO("vp",                SOI3_MOD_VP,        IF_REAL,
                "Channel length modulation (alt. empirical"),
IO("eta",               SOI3_MOD_ETA,       IF_REAL,
                "Impact ionization field adjustment factor"),
IO("alpha0",            SOI3_MOD_ALPHA0,    IF_REAL,
                "First impact ionisation coeff (alpha0"),
IO("beta0",             SOI3_MOD_BETA0,     IF_REAL,
                "Second impact ionisation coeff (beta0"),
IO("lm",                SOI3_MOD_LM,        IF_REAL,
                "Impact ion. drain region length"),
IO("lm1",               SOI3_MOD_LM1,       IF_REAL,
                "Impact ion. drain region length coeff"),
IO("lm2",               SOI3_MOD_LM2,       IF_REAL,
                "Impact ion. drain region length coeff"),
IO("etad",              SOI3_MOD_ETAD,      IF_REAL,
                "Diode ideality factor"),
IO("etad1",             SOI3_MOD_ETAD1,     IF_REAL,
                "2nd Diode ideality factor"),
IO("chibeta",           SOI3_MOD_CHIBETA,   IF_REAL,
                "Impact ionisation temperature coefficient"),
IO("vfbb",              SOI3_MOD_VFBB,      IF_REAL,
                "Back Flat band voltage"),
IO("gammab",            SOI3_MOD_GAMMAB,    IF_REAL,
                "Back Body Factor"),
IO("chid",              SOI3_MOD_CHID,      IF_REAL,
                "Junction temperature factor"),
IO("chid1",             SOI3_MOD_CHID1,     IF_REAL,
                "2nd Junction temperature factor"),
IO("dvt",               SOI3_MOD_DVT,       IF_INTEGER,
                "Switch for temperature dependence of vt in diodes"),
IO("nlev",              SOI3_MOD_NLEV,      IF_INTEGER,
                "Level switch for flicker noise model"),
IO("betabjt",           SOI3_MOD_BETABJT,   IF_REAL,
                "Beta for BJT"),
IO("tauf",              SOI3_MOD_TAUFBJT,   IF_REAL,
                "Forward tau for BJT"),
IO("taur",              SOI3_MOD_TAURBJT,   IF_REAL,
                "Reverse tau for BJT"),
IO("betaexp",           SOI3_MOD_BETAEXP,   IF_REAL,
                "Exponent for Beta of BJT"),
IO("tauexp",            SOI3_MOD_TAUEXP,    IF_REAL,
                "Exponent for Transit time of BJT"),
IO("rsw",               SOI3_MOD_RSW,       IF_REAL,
                "Source resistance width scaling factor"),
IO("rdw",               SOI3_MOD_RDW,       IF_REAL,
                "Drain resistance width scaling factor"),
IO("fmin",              SOI3_MOD_FMIN,      IF_REAL,
                "Minimum feature size of technology"),
IO("vtex",              SOI3_MOD_VTEX,      IF_REAL,
                "Extracted threshold voltage"),
IO("vdex",              SOI3_MOD_VDEX,      IF_REAL,
                "Drain bias at which vtex extracted"),
IO("delta0",            SOI3_MOD_DELTA0,    IF_REAL,
                "Surface potential factor for vtex conversion"),
IO("csf",               SOI3_MOD_CSF,       IF_REAL,
                "Saturation region charge sharing factor"),
IO("nplus",             SOI3_MOD_NPLUS,     IF_REAL,
                "Doping concentration of N+ or P+ regions"),
IO("rta",               SOI3_MOD_RTA,       IF_REAL,
                "Thermal resistance area scaling factor"),
IO("cta",               SOI3_MOD_CTA,       IF_REAL,
                "Thermal capacitance area scaling factor"),

// SRW - accept nmos/pmos as well as nsoi/psoi
IP("nmos",              SOI3_MOD_NSOI3,     IF_FLAG|IF_REDUNDANT,
                "N type SOI3fet model"),
IP("pmos",              SOI3_MOD_PSOI3,     IF_FLAG|IF_REDUNDANT,
                "P type SOI3fet model")
};

const char *SOI3names[] = {
    "Drain",
    "Front Gate",
    "Source",
    "Back Gate",
    "Bulk",
    "Thermal"
};

const char *SOI3modNames[] = {
    "nmos",
    "pmos",
    "nsoi",
    "psoi",
    0
};

IFkeys SOI3keys[] = {
    IFkeys( 'm', SOI3names, 6, 6, 0 )
};

} // namespace


SOI3dev::SOI3dev()
{
    dv_name = "Soi3";
    dv_description = "Basic Thick Film SOI3 model";

    dv_numKeys = NUMELEMS(SOI3keys);
    dv_keys = SOI3keys;

    dv_levels[0] = 33;
    dv_levels[1] = 0;
    dv_modelKeys = SOI3modNames;

    dv_numInstanceParms = NUMELEMS(SOI3pTable);
    dv_instanceParms = SOI3pTable;

    dv_numModelParms = NUMELEMS(SOI3mPTable);
    dv_modelParms = SOI3mPTable;

    dv_flags = DV_TRUNC;
};


sGENmodel *
SOI3dev::newModl()
{
    return (new sSOI3model);
}


sGENinstance *
SOI3dev::newInst()
{
    return (new sSOI3instance);
}


int
SOI3dev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sSOI3model, sSOI3instance>(model));
}


int
SOI3dev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sSOI3model, sSOI3instance>(model, dname,
        fast));
}


int
SOI3dev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sSOI3model, sSOI3instance>(model, modname,
        modfast));
}


// mosfet parser
// Mname <node> <node> <node> <node> <node> <node> <model>
//       [L=<val>] [W=<val>] [AD=<val>] [AS=<val>] [PD=<val>]
//       [PS=<val>] [NRD=<val>] [NRS=<val>] [OFF]
//       [IC=<val>,<val>,<val>]
// 
void
SOI3dev::parse(int type, sCKT *ckt, sLine *current)
{
    DEV.parse(ckt, current, type, dv_keys->minTerms, dv_keys->maxTerms,
        true, 0);
}


// Below is hugely GCC-specific.  The __WRMODULE__ and __WRVERSION__ tokens are
// defined in the Makefile and passed with -D when compiling.

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
        *d = new SOI3dev;
        (*cnt)++;
    }
}

