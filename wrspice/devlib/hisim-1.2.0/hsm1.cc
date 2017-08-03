
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

/***********************************************************************
 HiSIM (Hiroshima University STARC IGFET Model)
 Copyright (C) 2003 STARC

 VERSION : HiSIM 1.2.0
 FILE : hsm1.c of HiSIM 1.2.0

 April 9, 2003 : released by STARC Physical Design Group
***********************************************************************/

#include "hsm1defs.h"


namespace {

IFparm HSM1pTable[] = {
IO("l",                 HSM1_L,             IF_REAL,
                "Length"),
IO("w",                 HSM1_W,             IF_REAL,
                "Width"),
IO("ad",                HSM1_AD,            IF_REAL,
                "Drain area"),
IO("as",                HSM1_AS,            IF_REAL,
                "Source area"),
IO("pd",                HSM1_PD,            IF_REAL,
                "Drain perimeter"),
IO("ps",                HSM1_PS,            IF_REAL,
                "Source perimeter"),
IO("nrd",               HSM1_NRD,           IF_REAL,
                "Number of squares in drain"),
IO("nrs",               HSM1_NRS,           IF_REAL,
                "Number of squares in source"),
IO("temp",              HSM1_TEMP,          IF_REAL,
                "Lattice temperature"),
IO("dtemp",             HSM1_DTEMP,         IF_REAL,
                ""),
IO("off",               HSM1_OFF,           IF_FLAG,
                "Device is initially off"),
IP("ic",                HSM1_IC,            IF_REALVEC,
                "Vector of DS,GS,BS initial voltages")
};

IFparm HSM1mPTable[] = {
IP("nmos",              HSM1_MOD_NMOS,      IF_FLAG,
                ""),
IP("pmos",              HSM1_MOD_PMOS,      IF_FLAG,
                ""),
IO("level",             HSM1_MOD_LEVEL,     IF_INTEGER,
                ""),
IO("info",              HSM1_MOD_INFO,      IF_INTEGER,
                "Information level (for debug, etc."),
IO("noise",             HSM1_MOD_NOISE,     IF_INTEGER,
                "Noise model selector"),
IO("version",           HSM1_MOD_VERSION,   IF_INTEGER,
                "Model version 102 or 112 or 120"),
IO("show",              HSM1_MOD_SHOW,      IF_INTEGER,
                "Show physical value"),
IO("corsrd",            HSM1_MOD_CORSRD,    IF_INTEGER,
                "Solve equations accounting Rs and Rd."),
IO("coiprv",            HSM1_MOD_COIPRV,    IF_INTEGER,
                "Use ids_prv as initial guess of Ids"),
IO("copprv",            HSM1_MOD_COPPRV,    IF_INTEGER,
                "Use ps{0/l}_prv as initial guess of Ps{0/l}"),
IO("cocgso",            HSM1_MOD_COCGSO,    IF_INTEGER,
                "Calculate cgso"),
IO("cocgdo",            HSM1_MOD_COCGDO,    IF_INTEGER,
                "Calculate cgdo"),
IO("cocgbo",            HSM1_MOD_COCGBO,    IF_INTEGER,
                "Calculate cgbo"),
IO("coadov",            HSM1_MOD_COADOV,    IF_INTEGER,
                "Add overlap to intrisic"),
IO("coxx08",            HSM1_MOD_COXX08,    IF_INTEGER,
                "Spare"),
IO("coxx09",            HSM1_MOD_COXX09,    IF_INTEGER,
                "Spare"),
IO("coisub",            HSM1_MOD_COISUB,    IF_INTEGER,
                "Calculate isub"),
IO("coiigs",            HSM1_MOD_COIIGS,    IF_INTEGER,
                "Calculate igate"),
IO("cogidl",            HSM1_MOD_COGIDL,    IF_INTEGER,
                "Calculate igidl"),
IO("cogisl",            HSM1_MOD_COGISL,    IF_INTEGER,
                "Calculate igisl"),
IO("coovlp",            HSM1_MOD_COOVLP,    IF_INTEGER,
                "Calculate overlap charge"),
IO("conois",            HSM1_MOD_CONOIS,    IF_INTEGER,
                "Calculate 1/f noise"),
IO("coisti",            HSM1_MOD_COISTI,    IF_INTEGER,
                "Calculate STI HiSIM1.1"),
IO("cosmbi",            HSM1_MOD_COSMBI,    IF_INTEGER,
                "Biases smoothing in dvth HiSIM1.2"),
IO("vmax",              HSM1_MOD_VMAX,      IF_REAL,
                "Saturation velocity [cm/s"),
IO("bgtmp1",            HSM1_MOD_BGTMP1,    IF_REAL,
                "First order temp. coeff. for band gap [V/K]"),
IO("bgtmp2",            HSM1_MOD_BGTMP2,    IF_REAL,
                "Second order temp. coeff. for band gap [V/K^2]"),
IO("tox",               HSM1_MOD_TOX,       IF_REAL,
                "Oxide thickness [m]"),
IO("xld",               HSM1_MOD_XLD,       IF_REAL,
                "Lateral diffusion of S/D under the gate [m]"),
IO("xwd",               HSM1_MOD_XWD,       IF_REAL,
                "Lateral diffusion along the width dir. [m]"),
IO("xj",                HSM1_MOD_XJ,        IF_REAL,
                "HiSIM1.0.z [m]"),
IO("xqy",               HSM1_MOD_XQY,       IF_REAL,
                "HiSIM1.1.z or later [m]"),
IO("rs",                HSM1_MOD_RS,        IF_REAL,
                "Source contact resistance [ohm m]"),
IO("rd",                HSM1_MOD_RD,        IF_REAL,
                "Drain contact resistance  [ohm m]"),
IO("vfbc",              HSM1_MOD_VFBC,      IF_REAL,
                "Constant part of Vfb [V]"),
IO("nsubc",             HSM1_MOD_NSUBC,     IF_REAL,
                "Constant part of Nsub [1/cm^3]"),
IO("parl1",             HSM1_MOD_PARL1,     IF_REAL,
                "Factor for L dependency of dVthSC [-]"),
IO("parl2",             HSM1_MOD_PARL2,     IF_REAL,
                "Under diffusion [m]"),
IO("lp",                HSM1_MOD_LP,        IF_REAL,
                "Length of pocket potential [m]"),
IO("nsubp",             HSM1_MOD_NSUBP,     IF_REAL,
                "[1/cm^3]"),
IO("scp1",              HSM1_MOD_SCP1,      IF_REAL,
                "Parameter for pocket [-]"),
IO("scp2",              HSM1_MOD_SCP2,      IF_REAL,
                "Parameter for pocket [1/V]"),
IO("scp3",              HSM1_MOD_SCP3,      IF_REAL,
                "Parameter for pocket [m/V]"),
IO("sc1",               HSM1_MOD_SC1,       IF_REAL,
                "Parameter for SCE [-]"),
IO("sc2",               HSM1_MOD_SC2,       IF_REAL,
                "Parameter for SCE [1/V]"),
IO("sc3",               HSM1_MOD_SC3,       IF_REAL,
                "Parameter for SCE [m/V]"),
IO("pgd1",              HSM1_MOD_PGD1,      IF_REAL,
                "Parameter for gate-poly depletion [V]"),
IO("pgd2",              HSM1_MOD_PGD2,      IF_REAL,
                "Parameter for gate-poly depletion [V]"),
IO("pgd3",              HSM1_MOD_PGD3,      IF_REAL,
                "Parameter for gate-poly depletion [-]"),
IO("ndep",              HSM1_MOD_NDEP,      IF_REAL,
                "Coeff. of Qbm for Eeff [-]"),
IO("ninv",              HSM1_MOD_NINV,      IF_REAL,
                "Coeff. of Qnm for Eeff [-]"),
IO("ninvd",             HSM1_MOD_NINVD,     IF_REAL,
                "Parameter for universal mobility [1/V]"),
IO("muecb0",            HSM1_MOD_MUECB0,    IF_REAL,
                "Const. part of coulomb scattering [cm^2/Vs]"),
IO("muecb1",            HSM1_MOD_MUECB1,    IF_REAL,
                "Coeff. for coulomb scattering [cm^2/Vs]"),
IO("mueph0",            HSM1_MOD_MUEPH0,    IF_REAL,
                "Power of Eeff for phonon scattering [-]"),
IO("mueph1",            HSM1_MOD_MUEPH1,    IF_REAL,
                ""),
IO("mueph2",            HSM1_MOD_MUEPH2,    IF_REAL,
                ""),
IO("w0",                HSM1_MOD_W0,        IF_REAL,
                ""),
IO("muesr0",            HSM1_MOD_MUESR0,    IF_REAL,
                "Power of Eeff for S.R. scattering [-]"),
IO("muesr1",            HSM1_MOD_MUESR1,    IF_REAL,
                "Coeff. for S.R. scattering [-]"),
IO("muetmp",            HSM1_MOD_MUETMP,    IF_REAL,
                "Parameter for mobility [-]"),
IO("bb",                HSM1_MOD_BB,        IF_REAL,
                "Empirical mobility model coefficient [-]"),
IO("sub1",              HSM1_MOD_SUB1,      IF_REAL,
                "Parameter for Isub [1/V]"),
IO("sub2",              HSM1_MOD_SUB2,      IF_REAL,
                "Parameter for Isub [V]"),
IO("sub3",              HSM1_MOD_SUB3,      IF_REAL,
                "Parameter for Isub [-]"),
IO("wvthsc",            HSM1_MOD_WVTHSC,    IF_REAL,
                "Parameter for STI [-] HiSIM1.1"),
IO("nsti",              HSM1_MOD_NSTI,      IF_REAL,
                "Parameter for STI [1/cm^3] HiSIM1.1"),
IO("wsti",              HSM1_MOD_WSTI,      IF_REAL,
                "Parameter for STI [m] HiSIM1.1"),
IO("cgso",              HSM1_MOD_CGSO,      IF_REAL,
                "G-S overlap capacitance per unit W [F/m]"),
IO("cgdo",              HSM1_MOD_CGDO,      IF_REAL,
                "G-D overlap capacitance per unit W [F/m]"),
IO("cgbo",              HSM1_MOD_CGBO,      IF_REAL,
                "G-B overlap capacitance per unit L [F/m]"),
IO("tpoly",             HSM1_MOD_TPOLY,     IF_REAL,
                "Hight of poly gate [m]"),
IO("js0",               HSM1_MOD_JS0,       IF_REAL,
                "Saturation current density [A/m^2]"),
IO("js0sw",             HSM1_MOD_JS0SW,     IF_REAL,
                "Side wall saturation current density [A/m]"),
IO("nj",                HSM1_MOD_NJ,        IF_REAL,
                "Emission coefficient"),
IO("njsw",              HSM1_MOD_NJSW,      IF_REAL,
                "Sidewall emission coefficient"),
IO("xti",               HSM1_MOD_XTI,       IF_REAL,
                "Junction current temparature exponent coefficient"),
IO("cj",                HSM1_MOD_CJ,        IF_REAL,
                "Bottom junction capacitance per unit area at zero bias [F/m^2]"),
IO("cjsw",              HSM1_MOD_CJSW,      IF_REAL,
                "Source/drain sidewall junction capacitance grading coefficient per unit length at zero bias [F/m]"),
IO("cjswg",             HSM1_MOD_CJSWG,     IF_REAL,
                "Source/drain gate sidewall junction capacitance per unit length at zero bias [F/m]"),
IO("mj",                HSM1_MOD_MJ,        IF_REAL,
                "Bottom junction capacitance grading coefficient"),
IO("mjsw",              HSM1_MOD_MJSW,      IF_REAL,
                "Source/drain sidewall junction capacitance grading coefficient"),
IO("mjswg",             HSM1_MOD_MJSWG,     IF_REAL,
                "Source/drain gate sidewall junction capacitance grading coefficient"),
IO("pb",                HSM1_MOD_PB,        IF_REAL,
                "Bottom junction build-in potential  [V]"),
IO("pbsw",              HSM1_MOD_PBSW,      IF_REAL,
                "Source/drain sidewall junction build-in potential [V]"),
IO("pbswg",             HSM1_MOD_PBSWG,     IF_REAL,
                "Source/drain gate sidewall junction build-in potential [V]"),
IO("xpolyd",            HSM1_MOD_XPOLYD,    IF_REAL,
                "Parameter for Cov [m]"),
IO("clm1",              HSM1_MOD_CLM1,      IF_REAL,
                "Parameter for CLM [-]"),
IO("clm2",              HSM1_MOD_CLM2,      IF_REAL,
                "Parameter for CLM [1/m]"),
IO("clm3",              HSM1_MOD_CLM3,      IF_REAL,
                "Parameter for CLM [-]"),
IO("rpock1",            HSM1_MOD_RPOCK1,    IF_REAL,
                "Parameter for Ids [V]"),
IO("rpock2",            HSM1_MOD_RPOCK2,    IF_REAL,
                "Parameter for Ids [V^2 sqrt(m"),
IO("rpocp1",            HSM1_MOD_RPOCP1,    IF_REAL,
                "Parameter for Ids [-] HiSIM1.1"),
IO("rpocp2",            HSM1_MOD_RPOCP2,    IF_REAL,
                "Parameter for Ids [-] HiSIM1.1"),
IO("vover",             HSM1_MOD_VOVER,     IF_REAL,
                "Parameter for overshoot [m^{voverp}]"),
IO("voverp",            HSM1_MOD_VOVERP,    IF_REAL,
                "Parameter for overshoot [-]"),
IO("wfc",               HSM1_MOD_WFC,       IF_REAL,
                "Parameter for narrow channel effect [m*F/(cm^2"),
IO("qme1",              HSM1_MOD_QME1,      IF_REAL,
                "Parameter for quantum effect [mV]"),
IO("qme2",              HSM1_MOD_QME2,      IF_REAL,
                "Parameter for quantum effect [V]"),
IO("qme3",              HSM1_MOD_QME3,      IF_REAL,
                "Parameter for quantum effect [m]"),
IO("gidl1",             HSM1_MOD_GIDL1,     IF_REAL,
                "Parameter for GIDL [?]"),
IO("gidl2",             HSM1_MOD_GIDL2,     IF_REAL,
                "Parameter for GIDL [?]"),
IO("gidl3",             HSM1_MOD_GIDL3,     IF_REAL,
                "Parameter for GIDL [?]"),
IO("gleak1",            HSM1_MOD_GLEAK1,    IF_REAL,
                "Parameter for gate current [?]"),
IO("gleak2",            HSM1_MOD_GLEAK2,    IF_REAL,
                "Parameter for gate current [?]"),
IO("gleak3",            HSM1_MOD_GLEAK3,    IF_REAL,
                "Parameter for gate current [?]"),
IO("vzadd0",            HSM1_MOD_VZADD0,    IF_REAL,
                "Vzadd at Vds=0  [V]"),
IO("pzadd0",            HSM1_MOD_PZADD0,    IF_REAL,
                "Pzadd at Vds=0  [V]"),
IO("nftrp",             HSM1_MOD_NFTRP,     IF_REAL,
                ""),
IO("nfalp",             HSM1_MOD_NFALP,     IF_REAL,
                ""),
IO("cit",               HSM1_MOD_CIT,       IF_REAL,
                ""),
IO("glpart1",           HSM1_MOD_GLPART1,   IF_REAL,
                "Partitoning of gate current HiSIM1.2"),
IO("glpart2",           HSM1_MOD_GLPART2,   IF_REAL,
                "Partitoning of gate current HiSIM1.2"),
IO("kappa",             HSM1_MOD_KAPPA,     IF_REAL,
                "HiSIM1.2 dielectric constant for high-k stacked gate"),
IO("xdiffd",            HSM1_MOD_XDIFFD,    IF_REAL,
                "HiSIM1.2 parameter for W_design [m]"),
IO("pthrou",            HSM1_MOD_PTHROU,    IF_REAL,
                "HiSIM1.2 modify subthreshold sloop [-]"),
IO("vdiffj",            HSM1_MOD_VDIFFJ,    IF_REAL,
                "HiSIM1.2 threshold voltage for S/D junction diode [V]"),
IO("ef",                HSM1_MOD_EF,        IF_REAL,
                "Flicker noise frequency exponent"),
IO("af",                HSM1_MOD_AF,        IF_REAL,
                "Flicker noise exponent"),
IO("kf",                HSM1_MOD_KF,        IF_REAL,
                "Flicker noise coefficient")
};

const char *HSM1names[] = {
   "Drain",
   "Gate",
   "Source",
   "Bulk"
};

const char *HSM1modNames[] = {
    "nmos",
    "pmos",
    0
};

IFkeys HSM1keys[] = {
    IFkeys( 'm', HSM1names, 4, 4, 0 )
};

} // namespace


HSM1dev::HSM1dev()
{
    dv_name = "HiSIM1.2.0";
    dv_description = "Hiroshima-university STARC IGFET Model 1.2.0";

    dv_numKeys = NUMELEMS(HSM1keys);
    dv_keys = HSM1keys;

    dv_levels[0] = 31;
    dv_levels[1] = 64;  // HSPICE compatability
    dv_levels[2] = 0;
    dv_modelKeys = HSM1modNames;

    dv_numInstanceParms = NUMELEMS(HSM1pTable);
    dv_instanceParms = HSM1pTable;

    dv_numModelParms = NUMELEMS(HSM1mPTable);
    dv_modelParms = HSM1mPTable;

    dv_flags = DV_TRUNC;
};


sGENmodel *
HSM1dev::newModl()
{
    return (new sHSM1model);
}


sGENinstance *
HSM1dev::newInst()
{
    return (new sHSM1instance);
}


int
HSM1dev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sHSM1model, sHSM1instance>(model));
}


int
HSM1dev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sHSM1model, sHSM1instance>(model, dname,
        fast));
}


int
HSM1dev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sHSM1model, sHSM1instance>(model, modname,
        modfast));
}


// mosfet parser
// Mname <node> <node> <node> <node> <model>
//       [L=<val>] [W=<val>] [AD=<val>] [AS=<val>] [PD=<val>]
//       [PS=<val>] [NRD=<val>] [NRS=<val>] [OFF]
//       [IC=<val>,<val>,<val>]
// 
void
HSM1dev::parse(int type, sCKT *ckt, sLine *current)
{
    DEV.parse(ckt, current, type, dv_keys->minTerms, dv_keys->maxTerms,
        true, 0);
}


void
HSM1dev::backup(sGENmodel *mp, DEV_BKMODE m)
{
    while (mp) {
        for (sGENinstance *ip = mp->GENinstances; ip; ip = ip->GENnextInstance)
            ((sHSM1instance*)ip)->backup(m);
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
        *d = new HSM1dev;
        (*cnt)++;
    }
}

