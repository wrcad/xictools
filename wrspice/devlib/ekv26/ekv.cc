
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

/*
 * Author: 2000 Wladek Grabinski; EKV v2.6 Model Upgrade
 * Author: 1997 Eckhard Brass;    EKV v2.5 Model Implementation
 *     (C) 1990 Regents of the University of California. Spice3 Format
 */

#include "ekvdefs.h"


namespace {

IFparm EKVpTable[] = {
IO("l",                 EKV_L,              IF_REAL|IF_LEN,
                "Length"),
IO("w",                 EKV_W,              IF_REAL|IF_LEN,
                "Width"),
IO("ad",                EKV_AD,             IF_REAL|IF_AREA,
                "Drain area"),
IO("as",                EKV_AS,             IF_REAL|IF_AREA,
                "Source area"),
IO("pd",                EKV_PD,             IF_REAL|IF_LEN,
                "Drain perimeter"),
IO("ps",                EKV_PS,             IF_REAL|IF_LEN,
                "Source perimeter"),
IO("nrd",               EKV_NRD,            IF_REAL,
                "Drain squares"),
IO("nrs",               EKV_NRS,            IF_REAL,
                "Source squares"),
IP("off",               EKV_OFF,            IF_FLAG,
                "Device initially off"),
IO("icvds",             EKV_IC_VDS,         IF_REAL|IF_VOLT,
                "Initial D-S voltage"),
IO("icvgs",             EKV_IC_VGS,         IF_REAL|IF_VOLT,
                "Initial G-S voltage"),
IO("icvbs",             EKV_IC_VBS,         IF_REAL|IF_VOLT,
                "Initial B-S voltage"),
IP("ic",                EKV_IC,             IF_REALVEC|IF_VOLT,
                "Vector of D-S, G-S, B-S voltages"),

#ifdef HAS_SENSE2
IP("sens_l",            EKV_L_SENS,         IF_FLAG,
                "Flag to request sensitivity WRT length"),
IP("sens_w",            EKV_W_SENS,         IF_FLAG,
                "Flag to request sensitivity WRT width"),
#endif

OP("id",                EKV_CD,             IF_REAL|IF_AMP|IF_USEALL,
                "Drain current"),
OP("isub",              EKV_ISUB,           IF_REAL|IF_AMP,
                "Substrate current"),
OP("is",                EKV_CS,             IF_REAL|IF_AMP|IF_USEALL,
                "Source current"),
OP("ig",                EKV_CG,             IF_REAL|IF_AMP|IF_USEALL,
                "Gate current"),
OP("ib",                EKV_CB,             IF_REAL|IF_AMP,
                "Bulk current"),
OP("ibd",               EKV_CBD,            IF_REAL|IF_AMP,
                "B-D junction current"),
OP("ibs",               EKV_CBS,            IF_REAL|IF_AMP,
                "B-S junction current"),
OP("vgs",               EKV_VGS,            IF_REAL|IF_VOLT,
                "Gate-Source voltage"),
OP("vds",               EKV_VDS,            IF_REAL|IF_VOLT,
                "Drain-Source voltage"),
OP("vbs",               EKV_VBS,            IF_REAL|IF_VOLT,
                "Bulk-Source voltage"),
OP("vbd",               EKV_VBD,            IF_REAL|IF_VOLT,
                "Bulk-Drain voltage"),

OP("dnode",             EKV_DNODE,          IF_INTEGER,
                "Number of the drain node"),
OP("gnode",             EKV_GNODE,          IF_INTEGER,
                "Number of the gate node"),
OP("snode",             EKV_SNODE,          IF_INTEGER,
                "Number of the source node"),
OP("bnode",             EKV_BNODE,          IF_INTEGER,
                "Number of the node"),
OP("dnodeprime",        EKV_DNODEPRIME,     IF_INTEGER,
                "Number of int. drain node"),
OP("snodeprime",        EKV_SNODEPRIME,     IF_INTEGER,
                "Number of int. source node"),

OP("vth",               EKV_VON,            IF_REAL|IF_VOLT,
                "Threshold voltage"),
OP("vgeff",             EKV_VGEFF,          IF_REAL|IF_VOLT,
                "Effective gate voltage"),
OP("vp",                EKV_VP,             IF_REAL|IF_VOLT,
                "Pinch-off voltage"),
OP("vdsat",             EKV_VDSAT,          IF_REAL|IF_VOLT,
                "Saturation drain voltage"),
OP("vgprime",           EKV_VGPRIME,        IF_REAL|IF_VOLT,
                "Vgprime"),
OP("vgstar",            EKV_VGSTAR,         IF_REAL|IF_VOLT,
                "Vgstar"),
OP("sourcevcrit",       EKV_SOURCEVCRIT,    IF_REAL|IF_VOLT,
                "Critical source voltage"),
OP("drainvcrit",        EKV_DRAINVCRIT,     IF_REAL|IF_VOLT,
                "Critical drain voltage"),
OP("rs",                EKV_SOURCERESIST,   IF_REAL|IF_RES,
                "Source resistance"),
OP("sourceconductance", EKV_SOURCECONDUCT,  IF_REAL|IF_COND,
                "Conductance of source"),
OP("rd",                EKV_DRAINRESIST,    IF_REAL|IF_RES,
                "Drain resistance"),
OP("drainconductance",  EKV_DRAINCONDUCT,   IF_REAL|IF_COND,
                "Conductance of drain"),

OP("gm",                EKV_GM,             IF_REAL|IF_COND,
                "Transconductance"),
OP("gds",               EKV_GDS,            IF_REAL|IF_COND,
                "Drain-Source conductance"),
OP("gmb",               EKV_GMBS,           IF_REAL|IF_COND,
                "Bulk-Source transconductance"),
OP("gms",               EKV_GMS,            IF_REAL|IF_COND,
                "Source transconductance"),
OP("gbd",               EKV_GBD,            IF_REAL|IF_COND,
                "Bulk-Drain conductance"),
OP("gbs",               EKV_GBS,            IF_REAL|IF_COND,
                "Bulk-Source conductance"),
OP("if",                EKV_IF,             IF_REAL|IF_AMP,
                "Forward current"),
OP("ir",                EKV_IR,             IF_REAL|IF_AMP,
                "Reverse current"),
OP("irprime",           EKV_IRPRIME,        IF_REAL|IF_AMP,
                "Reverse current (prime"),
OP("n_slope",           EKV_SLOPE,          IF_REAL,
                "Slope factor"),
OP("tau",               EKV_TAU,            IF_REAL|IF_TIME,
                "NQS time constant"),

OP("cbd",               EKV_CAPBD,          IF_REAL|IF_CAP,
                "Bulk-Drain capacitance"),
OP("cbs",               EKV_CAPBS,          IF_REAL|IF_CAP,
                "Bulk-Source capacitance"),
OP("cgs",               EKV_CAPGS,          IF_REAL|IF_CAP,
                "Gate-Source capacitance"),
OP("cgd",               EKV_CAPGD,          IF_REAL|IF_CAP,
                "Gate-Drain capacitance"),
OP("cgb",               EKV_CAPGB,          IF_REAL|IF_CAP,
                "Gate-Bulk capacitance"),

OP("cqgs",              EKV_CQGS,           IF_REAL|IF_CAP,
                "Capacitance due to gate-source charge storage"),
OP("cqgd",              EKV_CQGD,           IF_REAL|IF_CAP,
                "Capacitance due to gate-drain charge storage"),
OP("cqgb",              EKV_CQGB,           IF_REAL|IF_CAP,
                "Capacitance due to gate-bulk charge storage"),
OP("cqbd",              EKV_CQBD,           IF_REAL|IF_CAP,
                "Capacitance due to bulk-drain charge storage"),
OP("cqbs",              EKV_CQBS,           IF_REAL|IF_CAP,
                "Capacitance due to bulk-source charge storage"),

OP("cbd0",              EKV_CAPZEROBIASBD,  IF_REAL|IF_CAP,
                "Zero-Bias B-D junction capacitance"),
OP("cbdsw0",            EKV_CAPZEROBIASBDSW,IF_REAL|IF_CAP,
                ""),
OP("cbs0",              EKV_CAPZEROBIASBS,  IF_REAL|IF_CAP,
                "Zero-Bias B-S junction capacitance"),
OP("cbssw0",            EKV_CAPZEROBIASBSSW,IF_REAL|IF_CAP,
                ""),

OP("qgs",               EKV_QGS,            IF_REAL|IF_CHARGE,
                "Gate-Source charge storage"),
OP("qgd",               EKV_QGD,            IF_REAL|IF_CHARGE,
                "Gate-Drain charge storage"),
OP("qgb",               EKV_QGB,            IF_REAL|IF_CHARGE,
                "Gate-Bulk charge storage"),
OP("qbd",               EKV_QBD,            IF_REAL|IF_CHARGE,
                "Bulk-Drain charge storage"),
OP("qbs",               EKV_QBS,            IF_REAL|IF_CHARGE,
                "Bulk-Source charge storage"),

OP("tvto",              EKV_TVTO,           IF_REAL,
                "Temp. corrected vto"),
OP("tkp",               EKV_TKP,            IF_REAL,
                "Temp. corrected kp"),
OP("tphi",              EKV_TPHI,           IF_REAL,
                "Temp. corrected phi"),
OP("tucrit",            EKV_TUCRIT,         IF_REAL,
                "Temp. corrected ucrit"),
OP("tibb",              EKV_TIBB,           IF_REAL,
                "Temp. corrected tibb"),
OP("trd",               EKV_TRD,            IF_REAL,
                "Temp. corrected rd"),
OP("trs",               EKV_TRS,            IF_REAL,
                "Temp. corrected rs"),
OP("trsh",              EKV_TRSH,           IF_REAL,
                "Temp. corrected rsh"),
OP("trdc",              EKV_TRDC,           IF_REAL,
                "Temp. corrected rdc"),
OP("trsc",              EKV_TRSC,           IF_REAL,
                "Temp. corrected rsc"),
OP("tis",               EKV_TIS,            IF_REAL,
                "Temp. corrected is"),
OP("tjs",               EKV_TJS,            IF_REAL,
                "Temp. corrected js"),
OP("tjsw",              EKV_TJSW,           IF_REAL,
                "Temp. corrected jsw"),
OP("tpb",               EKV_TPB,            IF_REAL,
                "Temp. corrected pb"),
OP("tpbsw",             EKV_TPBSW,          IF_REAL,
                "Temp. corrected pbsw"),
OP("tcbd",              EKV_TCBD,           IF_REAL,
                "Temp. corrected cbd"),
OP("tcbs",              EKV_TCBS,           IF_REAL,
                "Temp. corrected cbs"),
OP("tcj",               EKV_TCJ,            IF_REAL,
                "Temp. corrected cj"),
OP("tcjsw",             EKV_TCJSW,          IF_REAL,
                "Temp. corrected cjsw"),
OP("taf",               EKV_TAF,            IF_REAL,
                "Temp. corrected af"),
OP("power",             EKV_POWER,          IF_REAL|IF_POWR,
                "Instaneous power"),
IO("temp",              EKV_TEMP,           IF_REAL|IF_TEMP,
                "Instance temperature"),

#ifdef HAS_SENSE2
OP("sens_l_dc",         EKV_L_SENS_DC,      IF_REAL,
                "Dc sensitivity wrt length"),
OP("sens_l_real",       EKV_L_SENS_REAL,    IF_REAL,
                "Ac sensitivity wrt length"),
OP("sens_l_imag",       EKV_L_SENS_IMAG,    IF_REAL,
                "Ac sensitivity wrt length"),
OP("sens_l_mag",        EKV_L_SENS_MAG,     IF_REAL,
                "Ac sensitivity wrt length"),
OP("sens_l_ph",         EKV_L_SENS_PH,      IF_REAL,
                "Ac sensitivity wrt length"),
OP("sens_l_cplx",       EKV_L_SENS_CPLX,    IF_COMPLEX,
                "Ac sensitivity wrt length"),
OP("sens_w_dc",         EKV_W_SENS_DC,      IF_REAL,
                "Dc sensitivity wrt width"),
OP("sens_w_real",       EKV_W_SENS_REAL,    IF_REAL,
                "Ac sensitivity wrt width")
OP("sens_w_imag",       EKV_W_SENS_IMAG,    IF_REAL,
                "Ac sensitivity wrt width")
OP("sens_w_mag",        EKV_W_SENS_MAG,     IF_REAL,
                "Ac sensitivity wrt width")
OP("sens_w_ph",         EKV_W_SENS_PH,      IF_REAL,
                "Ac sensitivity wrt width")
OP("sens_w_cplx",       EKV_W_SENS_CPLX,    IF_COMPLEX,
                "Ac sensitivity wrt width")
#endif
};

IFparm EKVmPTable[] = {
OP("type",              EKV_MOD_TYPE,       IF_STRING,
                "N-channel or P-channel MOS"),
IO("ekvint",            EKV_MOD_EKVINT,     IF_REAL,
                "Interpolation function selector"),
IO("vto",               EKV_MOD_VTO,        IF_REAL,
                "Nominal threshold voltage"),
IO("kp",                EKV_MOD_KP,         IF_REAL,
                "Transconductance parameter"),
IO("gamma",             EKV_MOD_GAMMA,      IF_REAL,
                "Body effect parameter"),
IO("phi",               EKV_MOD_PHI,        IF_REAL,
                "Bulk Fermi potential"),
IO("cox",               EKV_MOD_COX,        IF_REAL,
                "Gate oxide capacitance"),
IO("xj",                EKV_MOD_XJ,         IF_REAL,
                "Junction depth"),
IO("theta",             EKV_MOD_THETA,      IF_REAL,
                "Mobility reduction coefficient"),
IO("e0",                EKV_MOD_E0,         IF_REAL,
                "NEW Mobility reduction coefficient"),
IO("ucrit",             EKV_MOD_UCRIT,      IF_REAL,
                "Longitudinal critical field"),
IO("dw",                EKV_MOD_DW,         IF_REAL,
                "Channel width correction"),
IO("dl",                EKV_MOD_DL,         IF_REAL,
                "Channel length correction"),
IO("lambda",            EKV_MOD_LAMBDA,     IF_REAL,
                "Depletion length coefficient"),
IO("weta",              EKV_MOD_WETA,       IF_REAL,
                "Narrow channel effect coefficient"),
IO("leta",              EKV_MOD_LETA,       IF_REAL,
                "Short channel coefficient"),
IO("iba",               EKV_MOD_IBA,        IF_REAL,
                "First impact ionization coefficient"),
IO("ibb",               EKV_MOD_IBB,        IF_REAL,
                "Second impact ionization coefficient"),
IO("ibn",               EKV_MOD_IBN,        IF_REAL,
                "Saturation voltage factor for impact ionization"),
IO("q0",                EKV_MOD_Q0,         IF_REAL,
                "RSCE excess charge"),
IO("lk",                EKV_MOD_LK,         IF_REAL,
                "RSCE characteristic length"),
IO("tcv",               EKV_MOD_TCV,        IF_REAL,
                "Threshold voltage temperature coefficient"),
IO("bex",               EKV_MOD_BEX,        IF_REAL,
                "Mobility temperature exponent"),
IO("ucex",              EKV_MOD_UCEX,       IF_REAL,
                "Longitudinal critical field temperature coefficient"),
IO("ibbt",              EKV_MOD_IBBT,       IF_REAL,
                "Temperature coefficient for ibb"),
IO("nqs",               EKV_MOD_NQS,        IF_REAL,
                "Non-Quasi-Static operation switch"),
IO("satlim",            EKV_MOD_SATLIM,     IF_REAL,
                "Ratio defining the saturation limit"),
IO("kf",                EKV_MOD_KF,         IF_REAL,
                "Flicker noise coefficient"),
IO("af",                EKV_MOD_AF,         IF_REAL,
                "Flicker noise exponent"),
IO("is",                EKV_MOD_IS,         IF_REAL,
                "Bulk p-n saturation current"),
IO("js",                EKV_MOD_JS,         IF_REAL,
                "Bulk p-n bottom saturation current per area"),
IO("jsw",               EKV_MOD_JSW,        IF_REAL,
                "Bulk p-n sidewall saturation current per length"),
IO("n",                 EKV_MOD_N,          IF_REAL,
                "Emission coefficient"),
IO("cbd",               EKV_MOD_CBD,        IF_REAL,
                "B-D p-n capacitance"),
IO("cbs",               EKV_MOD_CBS,        IF_REAL,
                "B-S p-n capacitance"),
IO("cj",                EKV_MOD_CJ,         IF_REAL,
                "Bottom p-n capacitance per area"),
IO("cjsw",              EKV_MOD_CJSW,       IF_REAL,
                "Sidewall p-n capacitance per length"),
IO("mj",                EKV_MOD_MJ,         IF_REAL,
                "Bottom p-n grading coefficient"),
IO("mjsw",              EKV_MOD_MJSW,       IF_REAL,
                "Sidewall p-n grading coefficient"),
IO("fc",                EKV_MOD_FC,         IF_REAL,
                "Forward p-n capacitance coefficient"),
IO("pb",                EKV_MOD_PB,         IF_REAL,
                "Bulk p-n junction potential"),
IO("pbsw",              EKV_MOD_PBSW,       IF_REAL,
                "Bulk sidewall p-n junction potential"),
IO("tt",                EKV_MOD_TT,         IF_REAL,
                "Bulk p-n transit time"),
IO("cgso",              EKV_MOD_CGSO,       IF_REAL,
                "Gate-source overlap capacitance"),
IO("cgdo",              EKV_MOD_CGDO,       IF_REAL,
                "Gate-drain overlap capacitance"),
IO("cgbo",              EKV_MOD_CGBO,       IF_REAL,
                "Gate-bulk overlap capacitance"),
IO("rs",                EKV_MOD_RS,         IF_REAL,
                "Source ohmic resistance"),
IO("rd",                EKV_MOD_RD,         IF_REAL,
                "Drain ohmic resistance"),
IO("rsh",               EKV_MOD_RSH,        IF_REAL,
                "Drain, source sheet resistance"),
IO("rsc",               EKV_MOD_RSC,        IF_REAL,
                "Source contact resistance"),
IO("rdc",               EKV_MOD_RDC,        IF_REAL,
                "Drain contact resistance"),
IO("xti",               EKV_MOD_XTI,        IF_REAL,
                "Junction current temperature exponent"),
IO("tr1",               EKV_MOD_TR1,        IF_REAL,
                "1st-order temperature coefficient"),
IO("tr2",               EKV_MOD_TR2,        IF_REAL,
                "2nd-order temperature coefficient"),
IO("nlevel",            EKV_MOD_NLEVEL,     IF_REAL,
                "Noise level selector"),
IP("nmos",              EKV_MOD_NMOS,       IF_FLAG,
                "N type MOSfet model"),
IP("pmos",              EKV_MOD_PMOS,       IF_FLAG,
                "P type MOSfet model"),
IO("tnom",              EKV_MOD_TNOM,       IF_REAL,
                "Parameter measurement temperature"),
// SRW
IO("xqc",               EKV_MOD_XQC,        IF_REAL,
                "Use simplified capacitance model")
};

const char *EKVnames[] = {
   "Drain",
   "Gate",
   "Source",
   "Bulk"
};

const char *EKVmodNames[] = {
    "nmos",
    "pmos",
    0
};

IFkeys EKVkeys[] = {
    IFkeys( 'm', EKVnames, 4, 4, 0 )
};

} // namespace


EKVdev::EKVdev()
{
    dv_name = "EKV-2.6";
    dv_description = "EPLF-EKV v2.6 MOSFET model";

    dv_numKeys = NUMELEMS(EKVkeys);
    dv_keys = EKVkeys;

    dv_levels[0] = 25;
    dv_levels[1] = 55;  // HSPICE compatability
    dv_levels[2] = 0;
    dv_modelKeys = EKVmodNames;

    dv_numInstanceParms = NUMELEMS(EKVpTable);
    dv_instanceParms = EKVpTable;

    dv_numModelParms = NUMELEMS(EKVmPTable);
    dv_modelParms = EKVmPTable;

    dv_flags = DV_TRUNC | DV_NODIST | DV_NOPZ;
};


sGENmodel *
EKVdev::newModl()
{
    return (new sEKVmodel);
}


sGENinstance *
EKVdev::newInst()
{
    return (new sEKVinstance);
}


int
EKVdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sEKVmodel, sEKVinstance>(model));
}


int
EKVdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sEKVmodel, sEKVinstance>(model, dname,
        fast));
}


int
EKVdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sEKVmodel, sEKVinstance>(model, modname,
        modfast));
}


// mosfet parser
// Mname <node> <node> <node> <node> <model>
//       [L=<val>] [W=<val>] [AD=<val>] [AS=<val>] [PD=<val>]
//       [PS=<val>] [NRD=<val>] [NRS=<val>] [OFF]
//       [IC=<val>,<val>,<val>]
// 
void
EKVdev::parse(int type, sCKT *ckt, sLine *current)
{
    DEV.parse(ckt, current, type, dv_keys->minTerms, dv_keys->maxTerms,
        true, 0);
}


void
EKVdev::backup(sGENmodel *mp, DEV_BKMODE m)
{
    while (mp) {
        for (sGENinstance *ip = mp->GENinstances; ip; ip = ip->GENnextInstance)
            ((sEKVinstance*)ip)->backup(m);
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
        *d = new EKVdev;
        (*cnt)++;
    }
}

