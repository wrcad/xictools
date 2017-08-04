
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1989 Takayasu Sakurai
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mosdefs.h"


namespace {

IFparm MOSpTable[] = {
IO("l",                 MOS_L,              IF_REAL|IF_LEN,
                "Length"),
IO("w",                 MOS_W,              IF_REAL|IF_LEN,
                "Width"),
IO("ad",                MOS_AD,             IF_REAL|IF_AREA,
                "Drain area"),
IO("as",                MOS_AS,             IF_REAL|IF_AREA,
                "Source area"),
IO("pd",                MOS_PD,             IF_REAL|IF_LEN,
                "Drain perimeter"),
IO("ps",                MOS_PS,             IF_REAL|IF_LEN,
                "Source perimeter"),
IO("nrd",               MOS_NRD,            IF_REAL,
                "Drain squares"),
IO("nrs",               MOS_NRS,            IF_REAL,
                "Source squares"),
IO("temp",              MOS_TEMP,           IF_REAL|IF_TEMP,
                "Instance operating temperature"),
IP("off",               MOS_OFF,            IF_FLAG,
                "Device initially off"),
IO("icvds",             MOS_IC_VDS,         IF_REAL|IF_VOLT|IF_AC,
                "Initial D-S voltage"),
IO("icvgs",             MOS_IC_VGS,         IF_REAL|IF_VOLT|IF_AC,
                "Initial G-S voltage"),
IO("icvbs",             MOS_IC_VBS,         IF_REAL|IF_VOLT|IF_AC,
                "Initial B-S voltage"),
IP("ic",                MOS_IC,             IF_REALVEC|IF_VOLT,
                "Vector of D-S, G-S, B-S voltages"),
OP("vbd",               MOS_VBD,            IF_REAL|IF_VOLT,
                "Bulk-Drain voltage"),
OP("vbs",               MOS_VBS,            IF_REAL|IF_VOLT,
                "Bulk-Source voltage"),
OP("vgs",               MOS_VGS,            IF_REAL|IF_VOLT,
                "Gate-Source voltage"),
OP("vds",               MOS_VDS,            IF_REAL|IF_VOLT,
                "Drain-Source voltage"),
OP("von",               MOS_VON,            IF_REAL|IF_VOLT,
                "Turn-on voltage"),
OP("vdsat",             MOS_VDSAT,          IF_REAL|IF_VOLT,
                "Saturation drain voltage"),
OP("drainvcrit",        MOS_DRAINVCRIT,     IF_REAL|IF_VOLT,
                "Critical drain voltage"),
OP("sourcevcrit",       MOS_SOURCEVCRIT,    IF_REAL|IF_VOLT,
                "Critical source voltage"),
OP("id",                MOS_CD,             IF_REAL|IF_AMP|IF_USEALL,
                "Drain current"),
OP("cd",                MOS_CD,             IF_REAL|IF_AMP|IF_REDUNDANT,
                "Drain current"),
OP("ibd",               MOS_CBD,            IF_REAL|IF_AMP,
                "B-D junction current"),
OP("ibs",               MOS_CBS,            IF_REAL|IF_AMP,
                "B-S junction current"),
OP("ig",                MOS_CG,             IF_REAL|IF_AMP|IF_USEALL,
                "Gate current"),
OP("is",                MOS_CS,             IF_REAL|IF_AMP|IF_USEALL,
                "Source current"),
OP("ib",                MOS_CB,             IF_REAL|IF_AMP|IF_USEALL,
                "Bulk current"),
OP("p",                 MOS_POWER,          IF_REAL|IF_POWR,
                "Instantaneous power"),
OP("rd",                MOS_DRAINRES,       IF_REAL|IF_RES,
                "Drain resistance"),
OP("rs",                MOS_SOURCERES,      IF_REAL|IF_RES,
                "Source resistance"),
OP("drainconductance",  MOS_DRAINCOND,      IF_REAL|IF_COND,
                "Drain conductance"),
OP("sourceconductance", MOS_SOURCECOND,     IF_REAL|IF_COND,
                "Source conductance"),
OP("gmb",               MOS_GMBS,           IF_REAL|IF_COND,
                "Bulk-Source transconductance"),
OP("gmbs",              MOS_GMBS,           IF_REAL|IF_COND|IF_REDUNDANT,
                "Bulk-Source transconductance"),
OP("gm",                MOS_GM,             IF_REAL|IF_COND,
                "Transconductance"),
OP("gds",               MOS_GDS,            IF_REAL|IF_COND,
                "Drain-Source conductance"),
OP("gbd",               MOS_GBD,            IF_REAL|IF_COND,
                "Bulk-Drain conductance"),
OP("gbs",               MOS_GBS,            IF_REAL|IF_COND,
                "Bulk-Source conductance"),
OP("qgd",               MOS_QGD,            IF_REAL|IF_CHARGE,
                "Gate-Drain charge storage"),
OP("qgs",               MOS_QGS,            IF_REAL|IF_CHARGE,
                "Gate-Source charge storage"),
OP("qgb",               MOS_QGB,            IF_REAL|IF_CHARGE,
                "Gate-Bulk charge storage"),
OP("qbd",               MOS_QBD,            IF_REAL|IF_CHARGE,
                "Bulk-Drain charge storage"),
OP("qbs",               MOS_QBS,            IF_REAL|IF_CHARGE,
                "Bulk-Source charge storage"),
OP("cbd",               MOS_CAPBD,          IF_REAL|IF_CAP,
                "Bulk-Drain capacitance"),
OP("cbs",               MOS_CAPBS,          IF_REAL|IF_CAP,
                "Bulk-Source capacitance"),
OP("cbd0",              MOS_CAPZBBD,        IF_REAL|IF_CAP,
                "Zero-Bias B-D junction capacitance"),
OP("cbdsw0",            MOS_CAPZBBDSW,      IF_REAL|IF_CAP,
                "Zero-Bias B-D sidewall capacitance"),
OP("cbs0",              MOS_CAPZBBS,        IF_REAL|IF_CAP,
                "Zero-Bias B-S junction capacitance"),
OP("cbssw0",            MOS_CAPZBBSSW,      IF_REAL|IF_CAP,
                "Zero-Bias B-S sidewall capacitance"),
OP("cgd",               MOS_CAPGD,          IF_REAL|IF_CAP,
                "Gate-Drain capacitance"),
OP("cqgd",              MOS_CQGD,           IF_REAL|IF_AMP,
                "Current due to gate-drain charge storage"),
OP("cgs",               MOS_CAPGS,          IF_REAL|IF_CAP,
                "Gate-Source capacitance"),
OP("cqgs",              MOS_CQGS,           IF_REAL|IF_AMP,
                "Current due to gate-source charge storage"),
OP("cgb",               MOS_CAPGB,          IF_REAL|IF_CAP,
                "Gate-Bulk capacitance"),
OP("cqgb",              MOS_CQGB,           IF_REAL|IF_AMP,
                "Current due to gate-bulk charge storage"),
OP("cqbd",              MOS_CQBD,           IF_REAL|IF_AMP,
                "Current due to bulk-drain charge storage"),
OP("cqbs",              MOS_CQBS,           IF_REAL|IF_AMP,
                "Current due to bulk-source charge storage"),
OP("dnode",             MOS_DNODE,          IF_INTEGER,
                "Number of drain node"),
OP("gnode",             MOS_GNODE,          IF_INTEGER,
                "Number of gate node"),
OP("snode",             MOS_SNODE,          IF_INTEGER,
                "Number of source node"),
OP("bnode",             MOS_BNODE,          IF_INTEGER,
                "Number of bulk node"),
OP("dnodeprime",        MOS_DNODEPRIME,     IF_INTEGER,
                "Number of internal drain node"),
OP("snodeprime",        MOS_SNODEPRIME,     IF_INTEGER,
                "Number of internal source node"),
IO("m",                 MOS_M,              IF_REAL,
                "Instance multiplier")
};

IFparm MOSmPTable[] = {
OP("type",              MOS_MOD_TYPE,       IF_STRING,
                "N-channel or P-channel MOS"),
IO("level",             MOS_MOD_LEVEL,      IF_INTEGER,
                "Model level"),
IO("tnom",              MOS_MOD_TNOM,       IF_REAL,
                "Parameter measurement temperature"),
IO("vto",               MOS_MOD_VTO,        IF_REAL,
                "Threshold voltage"),
IO("vt0",               MOS_MOD_VTO,        IF_REAL|IF_REDUNDANT,
                "Threshold voltage"),
IO("kp",                MOS_MOD_KP,         IF_REAL,
                "L1-3 Transconductance parameter"),
IO("gamma",             MOS_MOD_GAMMA,      IF_REAL,
                "Bulk threshold parameter"),
IO("phi",               MOS_MOD_PHI,        IF_REAL,
                "Surface potential"),
IO("rd",                MOS_MOD_RD,         IF_REAL,
                "Drain ohmic resistance"),
IO("rs",                MOS_MOD_RS,         IF_REAL,
                "Source ohmic resistance"),
IO("cbd",               MOS_MOD_CBD,        IF_REAL|IF_AC,
                "B-D junction capacitance"),
IO("cbs",               MOS_MOD_CBS,        IF_REAL|IF_AC,
                "B-S junction capacitance"),
IO("is",                MOS_MOD_IS,         IF_REAL,
                "Bulk junction sat. current"),
IO("pb",                MOS_MOD_PB,         IF_REAL,
                "Bulk junction potential"),
IO("cgso",              MOS_MOD_CGSO,       IF_REAL|IF_AC,
                "Gate-source overlap cap."),
IO("cgdo",              MOS_MOD_CGDO,       IF_REAL|IF_AC,
                "Gate-drain overlap cap."),
IO("cgbo",              MOS_MOD_CGBO,       IF_REAL|IF_AC,
                "Gate-bulk overlap cap."),
IO("cj",                MOS_MOD_CJ,         IF_REAL|IF_AC,
                "Bottom junction cap. per area"),
IO("mj",                MOS_MOD_MJ,         IF_REAL,
                "Bottom grading coefficient"),
IO("cjsw",              MOS_MOD_CJSW,       IF_REAL|IF_AC,
                "Side junction cap. per area"),
IO("mjsw",              MOS_MOD_MJSW,       IF_REAL,
                "Side grading coefficient"),
IO("js",                MOS_MOD_JS,         IF_REAL,
                "Bulk jct. sat. current density"),
IO("tox",               MOS_MOD_TOX,        IF_REAL,
                "Oxide thickness"),
IO("ld",                MOS_MOD_LD,         IF_REAL,
                "Lateral diffusion"),
IO("rsh",               MOS_MOD_RSH,        IF_REAL,
                "Sheet resistance"),
IO("u0",                MOS_MOD_U0,         IF_REAL,
                "Surface mobility"),
IO("uo",                MOS_MOD_U0,         IF_REAL|IF_REDUNDANT,
                "Surface mobility"),
IO("fc",                MOS_MOD_FC,         IF_REAL,
                "Forward bias jct. fit parm."),
IO("nss",               MOS_MOD_NSS,        IF_REAL,
                "Surface state density"),
IO("nsub",              MOS_MOD_NSUB,       IF_REAL,
                "Substrate doping"),
IO("tpg",               MOS_MOD_TPG,        IF_INTEGER,
                "Gate type"),
IP("nmos",              MOS_MOD_NMOS,       IF_FLAG,
                "N type MOSfet model"),
IP("pmos",              MOS_MOD_PMOS,       IF_FLAG,
                "P type MOSfet model"),
IP("kf",                MOS_MOD_KF,         IF_REAL,
                "L1-3 Flicker noise coefficient"),
IP("af",                MOS_MOD_AF,         IF_REAL,
                "L1-3 Flicker noise exponent"),
IO("lambda",            MOS_MOD_LAMBDA,     IF_REAL,
                "L1,2,6 Channel length modulation"),
IO("uexp",              MOS_MOD_UEXP,       IF_REAL,
                "L2 Crit. field exp for mob. deg."),
IO("neff",              MOS_MOD_NEFF,       IF_REAL,
                "L2 Total channel charge coeff."),
IO("ucrit",             MOS_MOD_UCRIT,      IF_REAL,
                "L2 Crit. field for mob. degradation"),
IO("nfs",               MOS_MOD_NFS,        IF_REAL,
                "L2,3 Fast surface state density"),
IO("delta",             MOS_MOD_DELTA,      IF_REAL,
                "L2,3 Width effect on threshold"),
IO("vmax",              MOS_MOD_VMAX,       IF_REAL,
                "L2,3 Maximum carrier drift velocity"),
IO("xj",                MOS_MOD_XJ,         IF_REAL,
                "L2,3 Junction depth"),
IO("eta",               MOS_MOD_ETA,        IF_REAL,
                "L3 Vds dep. of threshold voltage"),
IO("theta",             MOS_MOD_THETA,      IF_REAL,
                "L3 Vgs dep. on mobility"),
IO("alpha",             MOS_MOD_ALPHA,      IF_REAL,
                "L3 Alpha"),
IO("kappa",             MOS_MOD_KAPPA,      IF_REAL,
                "L3 Kappa"),
OP("xd",                MOS_MOD_XD,         IF_REAL,
                "L3 Depletion layer width"),
OP("input_delta",       MOS_MOD_IDELTA,     IF_REAL,
                "L3 Input delta"),
IO("kv",                MOS_MOD_KV,         IF_REAL,
                "L6 Saturation voltage factor"),
IO("nv",                MOS_MOD_NV,         IF_REAL,
                "L6 Saturation voltage coeff."),
IO("kc",                MOS_MOD_KC,         IF_REAL,
                "L6 Saturation current factor"),
IO("nc",                MOS_MOD_NC,         IF_REAL,
                "L6 Saturation current coeff."),
IO("gamma1",            MOS_MOD_GAMMA1,     IF_REAL,
                "L6 Bulk threshold parameter 1"),
IO("sigma",             MOS_MOD_SIGMA,      IF_REAL,
                "L6 Static feedback effect parm."),
IO("lambda0",           MOS_MOD_LAMDA0,     IF_REAL,
                "L6 Channel length modulation parm. 0"),
IO("lambda1",           MOS_MOD_LAMDA1,     IF_REAL,
                "L6 Channel length modulation parm. 1")
};                                       

const char *MOSnames[] = {                    
    "Drain",
    "Gate",
    "Source",
    "Bulk"
};

const char *MOSmodNames[] = {
    "nmos",
    "pmos",
    0
};

IFkeys MOSkeys[] = {
    IFkeys( 'm', MOSnames, 4, 4, 0 )
};

} // namespace


MOSdev::MOSdev()
{
    dv_name = "MOS";
    dv_description = "MOSFET model (Berkeley levels 1 - 3 and 6)";

    dv_numKeys = NUMELEMS(MOSkeys);
    dv_keys = MOSkeys;

    dv_levels[0] = 1;
    dv_levels[1] = 2;
    dv_levels[2] = 3;
    dv_levels[3] = 6;
    dv_levels[4] = 0;
    dv_modelKeys = MOSmodNames;

    dv_numInstanceParms = NUMELEMS(MOSpTable);
    dv_instanceParms = MOSpTable;

    dv_numModelParms = NUMELEMS(MOSmPTable);
    dv_modelParms = MOSmPTable;

    dv_flags = DV_TRUNC | DV_NOLEVCHG;
};


sGENmodel *
MOSdev::newModl()
{
    return (new sMOSmodel);
}


sGENinstance *
MOSdev::newInst()
{
    return (new sMOSinstance);
}


int
MOSdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sMOSmodel, sMOSinstance>(model));
}


int
MOSdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sMOSmodel, sMOSinstance>(model, dname,
        fast));
}


int
MOSdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sMOSmodel, sMOSinstance>(model, modname,
        modfast));
}


// mosfet parser
// Mname <node> <node> <node> <node> <model>
//       [L=<val>] [W=<val>] [AD=<val>] [AS=<val>] [PD=<val>]
//       [PS=<val>] [NRD=<val>] [NRS=<val>] [OFF]
//       [IC=<val>,<val>,<val>]
// 
void
MOSdev::parse(int type, sCKT *ckt, sLine *current)
{
    DEV.parse(ckt, current, type, dv_keys->minTerms, dv_keys->maxTerms,
        true, 0);
}


void
MOSdev::backup(sGENmodel *mp, DEV_BKMODE m)
{
    while (mp) {
        for (sGENinstance *ip = mp->GENinstances; ip; ip = ip->GENnextInstance)
            ((sMOSinstance*)ip)->backup(m);
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
        *d = new MOSdev;
        (*cnt)++;
    }
}

