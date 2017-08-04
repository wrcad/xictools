
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

/**********
Copyright 1997 University of Florida.  All rights reserved.
Author: Min-Chie Jeng (For SPICE3E2)
File: ufs.c
**********/

//
//  This is the UFSOI 7.5 model
//

#include "ufsdefs.h"


namespace {

IFparm UFSpTable[] = {
IO("l",                 UFS_L,              IF_REAL,
                "Length"),
IO("w",                 UFS_W,              IF_REAL,
                "Width"),
IO("m",                 UFS_M,              IF_REAL,
                "Number of gate fingers"),
IO("ad",                UFS_AD,             IF_REAL,
                "Drain area"),
IO("as",                UFS_AS,             IF_REAL,
                "Source area"),
IO("ab",                UFS_AB,             IF_REAL,
                "Body area"),
IO("nrd",               UFS_NRD,            IF_REAL,
                "Number of squares in drain"),
IO("nrs",               UFS_NRS,            IF_REAL,
                "Number of squares in source"),
IO("nrb",               UFS_NRB,            IF_REAL,
                "Number of squares in body"),
IO("rth",               UFS_RTH,            IF_REAL,
                "Thermal resistance"),
IO("cth",               UFS_CTH,            IF_REAL,
                "Thermal capacitance"),
IO("pdj",               UFS_PDJ,            IF_REAL,
                "Drain junction perimeter"),
IO("psj",               UFS_PSJ,            IF_REAL,
                "Source junction perimeter"),
IO("off",               UFS_OFF,            IF_FLAG,
                "Device is initially off"),
IP("ic",                UFS_IC,             IF_REALVEC,
                "Vector of DS,GFS,GBS,BS initial voltages"),

OP("Ich",               UFS_ICH,            IF_REAL,
                "Channel current"),
OP("Ibjt",              UFS_IBJT,           IF_REAL,
                "Parasitic bipolar current"),
OP("Ir",                UFS_IR,             IF_REAL,
                "Recombination current (B-S"),
OP("Igt",               UFS_IGT,            IF_REAL,
                "Thermal generation current (B-D"),
OP("Igi",               UFS_IGI,            IF_REAL,
                "Impact-ionization current"),
OP("Igb",               UFS_IGB,            IF_REAL,
                "Gate-to-body tunneling current"),
OP("Vbs",               UFS_VBS,            IF_REAL,
                "Internal B-S voltage"),
OP("Vbd",               UFS_VBD,            IF_REAL,
                "Internal B-D voltage"),
OP("Vgfs",              UFS_VGFS,           IF_REAL,
                "Internal GF-S voltage"),
OP("Vgfd",              UFS_VGFD,           IF_REAL,
                "Internal GF-D voltage"),
OP("Vgbs",              UFS_VGBS,           IF_REAL,
                "Internal GB-S voltage"),
OP("Vds",               UFS_VDS,            IF_REAL,
                "Internal D-S voltage"),

OP("dId_dVgf",          UFS_GDGF,           IF_REAL,
                "DId_dVgf"),
OP("dId_dVd",           UFS_GDD,            IF_REAL,
                "DId_dVd"),
OP("dId_dVgb",          UFS_GDGB,           IF_REAL,
                "DId_dVgb"),
OP("dId_dVb",           UFS_GDB,            IF_REAL,
                "DId_dVb"),
OP("dId_dVs",           UFS_GDS,            IF_REAL,
                "DId_dVs"),
OP("dIr_dVb",           UFS_GRB,            IF_REAL,
                "DIr_dVb"),
OP("dIr_dVs",           UFS_GRS,            IF_REAL,
                "DIr_dVs"),
OP("dIgt_dVgf",         UFS_GGTGF,          IF_REAL,
                "DIgt_dVgf"),
OP("dIgt_dVd",          UFS_GGTD,           IF_REAL,
                "DIgt_dVd"),
OP("dIgt_dVgb",         UFS_GGTGB,          IF_REAL,
                "DIgt_dVgb"),
OP("dIgt_dVb",          UFS_GGTB,           IF_REAL,
                "DIgt_dVb"),
OP("dIgt_dVs",          UFS_GGTS,           IF_REAL,
                "DIgt_dVS"),
OP("dIgi_dVgf",         UFS_GGIGF,          IF_REAL,
                "DIgi_dVgf"),
OP("dIgi_dVd",          UFS_GGID,           IF_REAL,
                "DIgi_dVd"),
OP("dIgi_dVgb",         UFS_GGIGB,          IF_REAL,
                "DIgi_dVgb"),
OP("dIgi_dVb",          UFS_GGIB,           IF_REAL,
                "DIgi_dVb"),
OP("dIgi_dVs",          UFS_GGIS,           IF_REAL,
                "DIgi_dVs"),
OP("dIgb_dVgf",         UFS_GGBGF,          IF_REAL,
                "DIgb_dVgf"),
OP("dIgb_dVd",          UFS_GGBD,           IF_REAL,
                "DIgb_dVd"),
OP("dIgb_dVgb",         UFS_GGBGB,          IF_REAL,
                "DIgb_dVgb"),
OP("dIgb_dVb",          UFS_GGBB,           IF_REAL,
                "DIgb_dVb"),
OP("dIgb_dVs",          UFS_GGBS,           IF_REAL,
                "DIgb_dVs"),

OP("Qgf",               UFS_QGF,            IF_REAL,
                "Front-gate charge"),
OP("Qd",                UFS_QD,             IF_REAL,
                "Drain charge"),
OP("Qgb",               UFS_QGB,            IF_REAL,
                "Back-gate charge"),
OP("Qb",                UFS_QB,             IF_REAL,
                "Body charge"),
OP("Qs",                UFS_QS,             IF_REAL,
                "Source charge"),
OP("Qn",                UFS_QN,             IF_REAL,
                "Inversion layer charge"),
OP("Ueff",              UFS_UEFF,           IF_REAL,
                "Field-effect mobility"),
OP("Vtw",               UFS_VTW,            IF_REAL,
                "Weak-inversion threshold voltage"),
OP("Vts",               UFS_VTS,            IF_REAL,
                "Strong-inversion threshold voltage"),
OP("Vdsat",             UFS_VDSAT,          IF_REAL,
                "Drain saturation voltage"),

OP("dQgf_dVgf",         UFS_CGFGF,          IF_REAL,
                "DQgf_dVgf"),
OP("dQgf_dVd",          UFS_CGFD,           IF_REAL,
                "DQgf_dVd"),
OP("dQgf_dVgb",         UFS_CGFGB,          IF_REAL,
                "DQgf_dVgb"),
OP("dQgf_dVb",          UFS_CGFB,           IF_REAL,
                "DQgf_dVb"),
OP("dQgf_dVs",          UFS_CGFS,           IF_REAL,
                "DQgf_dVs"),
OP("dQd_dVgf",          UFS_CDGF,           IF_REAL,
                "DQd_dVgf"),
OP("dQd_dVd",           UFS_CDD,            IF_REAL,
                "DQd_dVd"),
OP("dQd_dVgb",          UFS_CDGB,           IF_REAL,
                "DQd_dVgb"),
OP("dQd_dVb",           UFS_CDB,            IF_REAL,
                "DQd_dVb"),
OP("dQd_dVs",           UFS_CDS,            IF_REAL,
                "DQd_dVs"),
OP("dQgb_Vgf",          UFS_CGBGF,          IF_REAL,
                "DQgb_dVgf"),
OP("dQgb_dVd",          UFS_CGBD,           IF_REAL,
                "DQgb_dVd"),
OP("dQgb_dVgb",         UFS_CGBGB,          IF_REAL,
                "DQgb_dVgb"),
OP("dQgb_dVb",          UFS_CGBB,           IF_REAL,
                "DQgb_dVb"),
OP("dQgb_dVs",          UFS_CGBS,           IF_REAL,
                "DQgb_dVs"),
OP("dQb_dVgf",          UFS_CBGF,           IF_REAL,
                "DQb_dVgf"),
OP("dQb_dVd",           UFS_CBD,            IF_REAL,
                "DQb_dVd"),
OP("dQb_dVgb",          UFS_CBGB,           IF_REAL,
                "DQb_dVgb"),
OP("dQb_dVb",           UFS_CBB,            IF_REAL,
                "DQb_dVb"),
OP("dQb_dVs",           UFS_CBS,            IF_REAL,
                "DQb_dVs"),
OP("dQs_dVgf",          UFS_CSGF,           IF_REAL,
                "DQs_dVgf"),
OP("dQs_dVd",           UFS_CSD,            IF_REAL,
                "DQs_dVd"),
OP("dQs_dVgb",          UFS_CSGB,           IF_REAL,
                "DQs_dVgb"),
OP("dQs_dVb",           UFS_CSB,            IF_REAL,
                "DQs_dVb"),
OP("dQs_dVs",           UFS_CSS,            IF_REAL,
                "DQs_dVs"),

OP("le",                UFS_LE,             IF_REAL,
                "Modulated channel length"),
OP("power",             UFS_POWER,          IF_REAL,
                "Power dissipation"),
OP("temp",              UFS_TEMP,           IF_REAL,
                "Device temperature"),
OP("Rd",                UFS_RD,             IF_REAL,
                "Drain resistance"),
OP("Rs",                UFS_RS,             IF_REAL,
                "Source resistance"),
OP("Rb",                UFS_RB,             IF_REAL,
                "Body resistance"),

OP("dId_dT",            UFS_GDT,            IF_REAL,
                "DId_dT"),
OP("dIr_dT",            UFS_GRT,            IF_REAL,
                "DIr_dT"),
OP("dIgt_dT",           UFS_GGTT,           IF_REAL,
                "DIgt_dT"),
OP("dIgi_dT",           UFS_GGIT,           IF_REAL,
                "DIgi_dT"),
OP("dIgb_dT",           UFS_GGBT,           IF_REAL,
                "DIgb_dT"),
OP("dQgf_dT",           UFS_CGFT,           IF_REAL,
                "DQgf_dT"),
OP("dQd_dT",            UFS_CDT,            IF_REAL,
                "DQd_dT"),
OP("dQgb_dT",           UFS_CGBT,           IF_REAL,
                "DQgb_dT"),
OP("dQb_dT",            UFS_CBT,            IF_REAL,
                "DQb_dT"),
OP("dQs_dT",            UFS_CST,            IF_REAL,
                "DQs_dT"),
OP("dP_dT",             UFS_GPT,            IF_REAL,
                "DP_dT"),
OP("dP_dVgf",           UFS_GPGF,           IF_REAL,
                "DP_dVgf"),
OP("dP_dVd",            UFS_GPD,            IF_REAL,
                "DP_dVd"),
OP("dP_dVgb",           UFS_GPGB,           IF_REAL,
                "DP_dVgb"),
OP("dP_dVb",            UFS_GPB,            IF_REAL,
                "DP_dVb"),
OP("dP_dVs",            UFS_GPS,            IF_REAL,
                "DP_dVs")
};

IFparm UFSmPTable[] = {
IO("debug",             UFS_MOD_DEBUG,      IF_INTEGER,
                "Debugging flag"),
IO("paramchk",          UFS_MOD_PARAMCHK,   IF_INTEGER,
                "Model parameter checking selector"),
IO("selft",             UFS_MOD_SELFT,      IF_INTEGER,
                "Selft-heating flag"),
IO("body",              UFS_MOD_BODY,       IF_INTEGER,
                "Body condition selector"),

IO("vfbf",              UFS_MOD_VFBF,       IF_REAL,
                "Front-gate flatband voltage"),
IO("vfbb",              UFS_MOD_VFBB,       IF_REAL,
                "Back-gate flatband voltage"),
IO("wkf",               UFS_MOD_WKF,        IF_REAL,
                "Front-gate work function difference"),
IO("wkb",               UFS_MOD_WKB,        IF_REAL,
                "Back-gate work function difference"),
IO("nqff",              UFS_MOD_NQFF,       IF_REAL,
                "Normalized front oxide fixed charge"),
IO("nqfb",              UFS_MOD_NQFB,       IF_REAL,
                "Normalized back oxide fixed charge"),
IO("nsf",               UFS_MOD_NSF,        IF_REAL,
                "Front surface state density"),
IO("nsb",               UFS_MOD_NSB,        IF_REAL,
                "Back surface state density"),
IO("toxf",              UFS_MOD_TOXF,       IF_REAL,
                "Front-gate oxide thickness in meters"),
IO("toxb",              UFS_MOD_TOXB,       IF_REAL,
                "Back-gate oxide thickness in meters"),
IO("nsub",              UFS_MOD_NSUB,       IF_REAL,
                "Substrate doping concentration"),
IO("ngate",             UFS_MOD_NGATE,      IF_REAL,
                "Poly-gate doping concentration"),
IO("tpg",               UFS_MOD_TPG,        IF_INTEGER,
                "Type of gate material"),
IO("tps",               UFS_MOD_TPS,        IF_INTEGER,
                "Type of substrate material"),
IO("nds",               UFS_MOD_NDS,        IF_REAL,
                "Source/drain doping density"),
IO("tb",                UFS_MOD_TB,         IF_REAL,
                "Body film thickness"),
IO("nbody",             UFS_MOD_NBODY,      IF_REAL,
                "Body film doping density"),
IO("lldd",              UFS_MOD_LLDD,       IF_REAL,
                "Length of LDD region"),
IO("nldd",              UFS_MOD_NLDD,       IF_REAL,
                "LDD/LDS doing density"),
IO("uo",                UFS_MOD_U0,         IF_REAL,
                "Low-field mobility at Tnom"),
IO("theta",             UFS_MOD_THETA,      IF_REAL,
                "Mobility reduction parameter"),
IO("bfact",             UFS_MOD_BFACT,      IF_REAL,
                "Vds-averaging factor for mobility reduction"),
IO("vsat",              UFS_MOD_VSAT,       IF_REAL,
                "Carrier saturation velocity"),
IO("alpha",             UFS_MOD_ALPHA,      IF_REAL,
                "Impact-ionization parameter"),
IO("beta",              UFS_MOD_BETA,       IF_REAL,
                "Impact-ionization parameter"),
IO("gamma",             UFS_MOD_GAMMA,      IF_REAL,
                "BOX fringing field factor"),
IO("kappa",             UFS_MOD_KAPPA,      IF_REAL,
                "BOX fringing field factor"),
IO("tauo",              UFS_MOD_TAUO,       IF_REAL,
                "Carrier lifetime in LDD region"),
IO("jro",               UFS_MOD_JRO,        IF_REAL,
                "Junction recombination current coefficient"),
IO("m",                 UFS_MOD_M,          IF_REAL,
                "Junction recombination slope factor"),
IO("ldiff",             UFS_MOD_LDIFF,      IF_REAL,
                "Effective diffusion length in source/drain parameter"),
IO("seff",              UFS_MOD_SEFF,       IF_REAL,
                "Effective combination velocity in source/drain"),
IO("fvbjt",             UFS_MOD_FVBJT,      IF_REAL,
                "BJT current partitioning factor"),
IO("tnom",              UFS_MOD_TNOM,       IF_REAL,
                "Parameter measurement temperature"),
IO("cgfdo",             UFS_MOD_CGFDO,      IF_REAL,
                "Front-gate to drain overlap capacitance per width"),
IO("cgfso",             UFS_MOD_CGFSO,      IF_REAL,
                "Front-gate to source overlap capacitance per width"),
IO("cgfbo",             UFS_MOD_CGFBO,      IF_REAL,
                "Front-gate to bulk overlap capacitance per length"),
IO("rhosd",             UFS_MOD_RHOSD,      IF_REAL,
                "Source/drain sheet resistance"),
IO("rhob",              UFS_MOD_RHOB,       IF_REAL,
                "Body sheet resistance"),
IO("rd",                UFS_MOD_RD,         IF_REAL,
                "Specific drain parasitic resistance"),
IO("rs",                UFS_MOD_RS,         IF_REAL,
                "Specific source parasitic resistance"),
IO("rb",                UFS_MOD_RB,         IF_REAL,
                "Total body parasitic resistance"),
IO("dl",                UFS_MOD_DL,         IF_REAL,
                "Channel length reduction"),
IO("dw",                UFS_MOD_DW,         IF_REAL,
                "Channel width reduction"),
IO("fnk",               UFS_MOD_FNK,        IF_REAL,
                "Flicker noise coefficient"),
IO("fna",               UFS_MOD_FNA,        IF_REAL,
                "Flicker noise exponent"),
IO("tf",                UFS_MOD_TF,         IF_REAL,
                "Silicon film thickness"),
IO("thalo",             UFS_MOD_THALO,      IF_REAL,
                "Halo thickness"),
IO("nbl",               UFS_MOD_NBL,        IF_REAL,
                "Low body doping concentration"),
IO("nbh",               UFS_MOD_NBH,        IF_REAL,
                "High doping body concentration"),
IO("nhalo",             UFS_MOD_NHALO,      IF_REAL,
                "Halo doping centration"),
IO("tmax",              UFS_MOD_TMAX,       IF_REAL,
                "Maximum allowable temperature"),
IO("imax",              UFS_MOD_IMAX,       IF_REAL,
                "Explosion current"),
IO("bgidl",             UFS_MOD_BGIDL,      IF_REAL,
                "Gate-induced drain leakage current parameter"),
IO("ntr",               UFS_MOD_NTR,        IF_REAL,
                "Effective trap density for reverse-biased tunneling"),
IO("bjt",               UFS_MOD_BJT,        IF_INTEGER,
                "Flag to turn on parasitic bipolar current"),
IO("lrsce",             UFS_MOD_LRSCE,      IF_REAL,
                "Reverse short-channel effect characteristic length"),
IO("nqfsw",             UFS_MOD_NQFSW,      IF_REAL,
                "Sidewall fixed charge for narrow-width effect"),
IO("qm",                UFS_MOD_QM,         IF_REAL,
                "Energy quantization parameter"),
IO("vo",                UFS_MOD_VO,         IF_REAL,
                "Velocity overshoot parameter"),
IO("mox",               UFS_MOD_MOX,        IF_REAL,
                "Electron effective mass in oxide"),
IO("svbe",              UFS_MOD_SVBE,       IF_REAL,
                "Smoothing constant for the onset of Ivbe"),
IO("scbe",              UFS_MOD_SCBE,       IF_REAL,
                "Smoothing constant for the onset of Icbe"),
IO("kd",                UFS_MOD_KD,         IF_REAL,
                "Dielectric constant"),
IO("gex",               UFS_MOD_GEX,        IF_REAL,
                "Ge content in the relaxed SiGe buffer"),
IO("sfact",             UFS_MOD_SFACT,      IF_REAL,
                "Factor for spline smoothing"),
IO("ffact",             UFS_MOD_FFACT,      IF_REAL,
                "Factor for the onset voltage of Icbe"),
IP("nmos",              UFS_MOD_NMOS,       IF_FLAG,
                "Flag to indicate n-channel"),
IP("pmos",              UFS_MOD_PMOS,       IF_FLAG,
                "Flag to indicate p-channel")
};

// Internal names for the device nodes
const char *UFSnames[] = {
   "Drain",
   "Gate",
   "Source",
   "BackGate",
   "Bulk"
};

// List of model names associated with this device
const char *UFSmodNames[] = {
    "nmos",
    "pmos",
    0
};

// List of letters that key this device, e.g., 'c' for capacitor
IFkeys UFSkeys[] = {
    IFkeys( 'm', UFSnames, 3, 5, 0 )
};

} // namespace


// Constructor for static structure that holds general information
// for this device
UFSdev::UFSdev()
{
    dv_name = "UFS";
    dv_description = "UFPDB-2.5 Model";

    dv_numKeys = NUMELEMS(UFSkeys);
    dv_keys = UFSkeys,

    dv_levels[0] = 36;
    dv_levels[1] = 58;  // HSPICE compatability
    dv_levels[1] = 0;
    dv_modelKeys = UFSmodNames;

    dv_numInstanceParms = NUMELEMS(UFSpTable);
    dv_instanceParms = UFSpTable;

    dv_numModelParms = NUMELEMS(UFSmPTable);
    dv_modelParms = UFSmPTable;

    dv_flags = DV_TRUNC;
};


// Model factory
sGENmodel *
UFSdev::newModl()
{
    return (new sUFSmodel);
}


// Instance factory
sGENinstance *
UFSdev::newInst()
{
    return (new sUFSinstance);
}


int
UFSdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sUFSmodel, sUFSinstance>(model));
}


int
UFSdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sUFSmodel, sUFSinstance>(model, dname,
        fast));
}


int
UFSdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sUFSmodel, sUFSinstance>(model, modname,
        modfast));
}


// mosfet parser
// Mname <node> <node> <node> [<node> [<node>]] <model>
//       [L=<val>] [W=<val>] [AD=<val>] [AS=<val>] [PD=<val>]
//       [PS=<val>] [NRD=<val>] [NRS=<val>] [OFF]
//       [IC=<val>,<val>,<val>]
// 
void
UFSdev::parse(int type, sCKT *ckt, sLine *current)
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
        *d = new UFSdev;
        (*cnt)++;
    }
}

