
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
Authors: 1985 Hong J. Park, Thomas L. Quarles 
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b1defs.h"


namespace {

IFparm B1pTable[] = {
IO("l",                 BSIM1_L,            IF_REAL|IF_LEN,
                "Length"),
IO("w",                 BSIM1_W,            IF_REAL|IF_LEN,
                "Width"),
IO("as",                BSIM1_AS,           IF_REAL|IF_AREA,
                "Source area"),
IO("ad",                BSIM1_AD,           IF_REAL|IF_AREA,
                "Drain area"),
IO("ps",                BSIM1_PS,           IF_REAL|IF_LEN,
                "Source perimeter"),
IO("pd",                BSIM1_PD,           IF_REAL|IF_LEN,
                "Drain perimeter"),
IO("nrs",               BSIM1_NRS,          IF_REAL,
                "Number of squares in source"),
IO("nrd",               BSIM1_NRD,          IF_REAL,
                "Number of squares in drain"),
IO("off",               BSIM1_OFF,          IF_FLAG,
                "Device is initially off"),
IO("ic_vbs",            BSIM1_IC_VBS,       IF_REAL|IF_VOLT,
                "Initial B-S voltage"),
IO("ic_vds",            BSIM1_IC_VDS,       IF_REAL|IF_VOLT,
                "Initial D-S voltage"),
IO("ic_vgs",            BSIM1_IC_VGS,       IF_REAL|IF_VOLT,
                "Initial G-S voltage"),
IP("ic",                BSIM1_IC,           IF_REALVEC|IF_VOLT,
                "Vector of DS,GS,BS initial voltages"),
OP("vbd",               BSIM1_VBD,          IF_REAL|IF_VOLT,
                "Bulk-Drain voltage"),
OP("vbs",               BSIM1_VBS,          IF_REAL|IF_VOLT,
                "Bulk-Source voltage"),
OP("vgs",               BSIM1_VGS,          IF_REAL|IF_VOLT,
                "Gate-Source voltage"),
OP("vds",               BSIM1_VDS,          IF_REAL|IF_VOLT,
                "Drain-Source voltage"),
OP("von",               BSIM1_VON,          IF_REAL|IF_VOLT,
                "Turn-on voltage"),
OP("id",                BSIM1_CD,           IF_REAL|IF_AMP|IF_USEALL,
                "Drain current"),
OP("ibs",               BSIM1_CBS,          IF_REAL|IF_AMP,
                "B-S junction current"),
OP("ibd",               BSIM1_CBD,          IF_REAL|IF_AMP,
                "B-D junction current"),
OP("sourceconduct",     BSIM1_SOURCECOND,   IF_REAL|IF_COND,
                "Source conductance"),
OP("drainconduct",      BSIM1_DRAINCOND,    IF_REAL|IF_COND,
                "Drain conductance"),
OP("gm",                BSIM1_GM,           IF_REAL|IF_COND,
                "Transconductance"),
OP("gds",               BSIM1_GDS,          IF_REAL|IF_COND,
                "Drain-Source conductance"),
OP("gmbs",              BSIM1_GMBS,         IF_REAL|IF_COND,
                "Bulk-Source transconductance"),
OP("gbd",               BSIM1_GBD,          IF_REAL|IF_COND,
                "Bulk-Drain conductance"),
OP("gbs",               BSIM1_GBS,          IF_REAL|IF_COND,
                "Bulk-Source conductance"),
OP("qb",                BSIM1_QB,           IF_REAL|IF_CHARGE,
                "Bulk charge storage"),
OP("qg",                BSIM1_QG,           IF_REAL|IF_CHARGE,
                "Gate charge storage"),
OP("qd",                BSIM1_QD,           IF_REAL|IF_CHARGE,
                "Drain charge storage"),
OP("qbs",               BSIM1_QBS,          IF_REAL|IF_CHARGE,
                "Bulk-Source charge storage"),
OP("qbd",               BSIM1_QBD,          IF_REAL|IF_CHARGE,
                "Bulk-Drain charge storage"),
OP("cqb",               BSIM1_CQB,          IF_REAL|IF_AMP,
                "Bulk capacitance current"),
OP("cqg",               BSIM1_CQG,          IF_REAL|IF_AMP,
                "Gate capacitance current"),
OP("cqd",               BSIM1_CQD,          IF_REAL|IF_AMP,
                "Drain capacitance current"),
OP("cgg",               BSIM1_CGG,          IF_REAL|IF_AMP,
                "Gate capacitance current"),
OP("cgd",               BSIM1_CGD,          IF_REAL|IF_AMP,
                "Gate-Drain capacitance current"),
OP("cgs",               BSIM1_CGS,          IF_REAL|IF_AMP,
                "Gate-Source capacitance current"),
OP("cbg",               BSIM1_CBG,          IF_REAL|IF_AMP,
                "Gate-Bulk current"),
OP("cbd",               BSIM1_CAPBD,        IF_REAL|IF_CAP,
                "Drain-Bulk capacitance"),
OP("cqbd",              BSIM1_CQBD,         IF_REAL|IF_AMP,
                "Current due to drain-bulk cap"),
OP("cbs",               BSIM1_CAPBS,        IF_REAL|IF_CAP,
                "Source-Bulk capacitance"),
OP("cqbs",              BSIM1_CQBS,         IF_REAL|IF_AMP,
                "Current due to source-bulk cap"),
OP("cdg",               BSIM1_CDG,          IF_REAL|IF_AMP,
                "Drain-Gate current"),
OP("cdd",               BSIM1_CDD,          IF_REAL|IF_AMP,
                "Drain capacitor current"),
OP("cds",               BSIM1_CDS,          IF_REAL|IF_AMP,
                "Drain-Source current"),
OP("drainnode",         BSIM1_DNODE,        IF_INTEGER,
                "Number of drain node"),
OP("gatenode",          BSIM1_GNODE,        IF_INTEGER,
                "Number of gate node"),
OP("sourcenode",        BSIM1_SNODE,        IF_INTEGER,
                "Number of source node"),
OP("bulknode",          BSIM1_BNODE,        IF_INTEGER,
                "Number of bulk node"),
OP("drainprinenode",    BSIM1_DNODEPRIME,   IF_INTEGER,
                "Number of internal drain node"),
OP("sourceprimenode",   BSIM1_SNODEPRIME,   IF_INTEGER,
                "Number of internal source node"),
IO("m",                 BSIM1_M,            IF_REAL,
                "Instance multiplier")
};

IFparm B1mPTable[] = {
IO("vfb",               BSIM1_MOD_VFB0,     IF_REAL,
                "Flat band voltage"),
IO("lvfb",              BSIM1_MOD_VFBL,     IF_REAL,
                "Length dependence of vfb"),
IO("wvfb",              BSIM1_MOD_VFBW,     IF_REAL,
                "Width dependence of vfb"),
IO("phi",               BSIM1_MOD_PHI0,     IF_REAL,
                "Strong inversion surface potential"),
IO("lphi",              BSIM1_MOD_PHIL,     IF_REAL,
                "Length dependence of phi"),
IO("wphi",              BSIM1_MOD_PHIW,     IF_REAL,
                "Width dependence of phi"),
IO("k1",                BSIM1_MOD_K10,      IF_REAL,
                "Bulk effect coefficient 1"),
IO("lk1",               BSIM1_MOD_K1L,      IF_REAL,
                "Length dependence of k1"),
IO("wk1",               BSIM1_MOD_K1W,      IF_REAL,
                "Width dependence of k1"),
IO("k2",                BSIM1_MOD_K20,      IF_REAL,
                "Bulk effect coefficient 2"),
IO("lk2",               BSIM1_MOD_K2L,      IF_REAL,
                "Length dependence of k2"),
IO("wk2",               BSIM1_MOD_K2W,      IF_REAL,
                "Width dependence of k2"),
IO("eta",               BSIM1_MOD_ETA0,     IF_REAL,
                "VDS dependence of threshold voltage"),
IO("leta",              BSIM1_MOD_ETAL,     IF_REAL,
                "Length dependence of eta"),
IO("weta",              BSIM1_MOD_ETAW,     IF_REAL,
                "Width dependence of eta"),
IO("x2e",               BSIM1_MOD_ETAB0,    IF_REAL,
                "VBS dependence of eta"),
IO("lx2e",              BSIM1_MOD_ETABL,    IF_REAL,
                "Length dependence of x2e"),
IO("wx2e",              BSIM1_MOD_ETABW,    IF_REAL,
                "Width dependence of x2e"),
IO("x3e",               BSIM1_MOD_ETAD0,    IF_REAL,
                "VDS dependence of eta"),
IO("lx3e",              BSIM1_MOD_ETADL,    IF_REAL,
                "Length dependence of x3e"),
IO("wx3e",              BSIM1_MOD_ETADW,    IF_REAL,
                "Width dependence of x3e"),
IO("dl",                BSIM1_MOD_DELTAL,   IF_REAL,
                "Channel length reduction in um"),
IO("dw",                BSIM1_MOD_DELTAW,   IF_REAL,
                "Channel width reduction in um"),
IO("muz",               BSIM1_MOD_MOBZERO,  IF_REAL,
                "Zero field mobility at VDS=0 VGS=VTH"),
IO("x2mz",              BSIM1_MOD_MOBZEROB0,IF_REAL,
                "VBS dependence of muz"),
IO("lx2mz",             BSIM1_MOD_MOBZEROBL,IF_REAL,
                "Length dependence of x2mz"),
IO("wx2mz",             BSIM1_MOD_MOBZEROBW,IF_REAL,
                "Width dependence of x2mz"),
IO("mus",               BSIM1_MOD_MOBVDD0,  IF_REAL,
                "Mobility at VDS=VDD VGS=VTH, channel length modulation"),
IO("lmus",              BSIM1_MOD_MOBVDDL,  IF_REAL,
                "Length dependence of mus"),
IO("wmus",              BSIM1_MOD_MOBVDDW,  IF_REAL,
                "Width dependence of mus"),
IO("x2ms",              BSIM1_MOD_MOBVDDB0, IF_REAL,
                "VBS dependence of mus"),
IO("lx2ms",             BSIM1_MOD_MOBVDDBL, IF_REAL,
                "Length dependence of x2ms"),
IO("wx2ms",             BSIM1_MOD_MOBVDDBW, IF_REAL,
                "Width dependence of x2ms"),
IO("x3ms",              BSIM1_MOD_MOBVDDD0, IF_REAL,
                "VDS dependence of mus"),
IO("lx3ms",             BSIM1_MOD_MOBVDDDL, IF_REAL,
                "Length dependence of x3ms"),
IO("wx3ms",             BSIM1_MOD_MOBVDDDW, IF_REAL,
                "Width dependence of x3ms"),
IO("u0",                BSIM1_MOD_UGS0,     IF_REAL,
                "VGS dependence of mobility"),
IO("lu0",               BSIM1_MOD_UGSL,     IF_REAL,
                "Length dependence of u0"),
IO("wu0",               BSIM1_MOD_UGSW,     IF_REAL,
                "Width dependence of u0"),
IO("x2u0",              BSIM1_MOD_UGSB0,    IF_REAL,
                "VBS dependence of u0"),
IO("lx2u0",             BSIM1_MOD_UGSBL,    IF_REAL,
                "Length dependence of x2u0"),
IO("wx2u0",             BSIM1_MOD_UGSBW,    IF_REAL,
                "Width dependence of x2u0"),
IO("u1",                BSIM1_MOD_UDS0,     IF_REAL,
                "VDS depence of mobility, velocity saturation"),
IO("lu1",               BSIM1_MOD_UDSL,     IF_REAL,
                "Length dependence of u1"),
IO("wu1",               BSIM1_MOD_UDSW,     IF_REAL,
                "Width dependence of u1"),
IO("x2u1",              BSIM1_MOD_UDSB0,    IF_REAL,
                "VBS depence of u1"),
IO("lx2u1",             BSIM1_MOD_UDSBL,    IF_REAL,
                "Length depence of x2u1"),
IO("wx2u1",             BSIM1_MOD_UDSBW,    IF_REAL,
                "Width depence of x2u1"),
IO("x3u1",              BSIM1_MOD_UDSD0,    IF_REAL,
                "VDS depence of u1"),
IO("lx3u1",             BSIM1_MOD_UDSDL,    IF_REAL,
                "Length dependence of x3u1"),
IO("wx3u1",             BSIM1_MOD_UDSDW,    IF_REAL,
                "Width depence of x3u1"),
IO("n0",                BSIM1_MOD_N00,      IF_REAL,
                "Subthreshold slope"),
IO("ln0",               BSIM1_MOD_N0L,      IF_REAL,
                "Length dependence of n0"),
IO("wn0",               BSIM1_MOD_N0W,      IF_REAL,
                "Width dependence of n0"),
IO("nb",                BSIM1_MOD_NB0,      IF_REAL,
                "VBS dependence of subthreshold slope"),
IO("lnb",               BSIM1_MOD_NBL,      IF_REAL,
                "Length dependence of nb"),
IO("wnb",               BSIM1_MOD_NBW,      IF_REAL,
                "Width dependence of nb"),
IO("nd",                BSIM1_MOD_ND0,      IF_REAL,
                "VDS dependence of subthreshold slope"),
IO("lnd",               BSIM1_MOD_NDL,      IF_REAL,
                "Length dependence of nd"),
IO("wnd",               BSIM1_MOD_NDW,      IF_REAL,
                "Width dependence of nd"),
IO("tox",               BSIM1_MOD_TOX,      IF_REAL,
                "Gate oxide thickness in um"),
IO("temp",              BSIM1_MOD_TEMP,     IF_REAL,
                "Temperature in degree Celcius"),
IO("vdd",               BSIM1_MOD_VDD,      IF_REAL,
                "Supply voltage to specify mus"),
IO("cgso",              BSIM1_MOD_CGSO,     IF_REAL|IF_AC,
                "Gate source overlap capacitance per unit channel width(m"),
IO("cgdo",              BSIM1_MOD_CGDO,     IF_REAL|IF_AC,
                "Gate drain overlap capacitance per unit channel width(m"),
IO("cgbo",              BSIM1_MOD_CGBO,     IF_REAL|IF_AC,
                "Gate bulk overlap capacitance per unit channel length(m"),
IO("xpart",             BSIM1_MOD_XPART,    IF_REAL,
                "Flag for channel charge partitioning"),
IO("rsh",               BSIM1_MOD_RSH,      IF_REAL,
                "Source drain diffusion sheet resistance in ohm per square"),
IO("js",                BSIM1_MOD_JS,       IF_REAL,
                "Source drain junction saturation current per unit area"),
IO("pb",                BSIM1_MOD_PB,       IF_REAL,
                "Source drain junction built in potential"),
IO("mj",                BSIM1_MOD_MJ,       IF_REAL|IF_AC,
                "Source drain bottom junction capacitance grading coefficient"),
IO("pbsw",              BSIM1_MOD_PBSW,     IF_REAL|IF_AC,
                "Source drain side junction capacitance built in potential"),
IO("mjsw",              BSIM1_MOD_MJSW,     IF_REAL|IF_AC,
                "Source drain side junction capacitance grading coefficient"),
IO("cj",                BSIM1_MOD_CJ,       IF_REAL|IF_AC,
                "Source drain bottom junction capacitance per unit area"),
IO("cjsw",              BSIM1_MOD_CJSW,     IF_REAL|IF_AC,
                "Source drain side junction capacitance per unit area"),
IO("wdf",               BSIM1_MOD_DEFWIDTH, IF_REAL,
                "Default width of source drain diffusion in um"),
IO("dell",              BSIM1_MOD_DELLENGTH,IF_REAL,
                "Length reduction of source drain diffusion"),
IP("nmos",              BSIM1_MOD_NMOS,     IF_FLAG,
                "Flag to indicate NMOS"),
IP("pmos",              BSIM1_MOD_PMOS,     IF_FLAG,
                "Flag to indicate PMOS")
};

const char *B1names[] = {
    "Drain",
    "Gate",
    "Source",
    "Bulk"
};

const char *B1modNames[] = {
    "nmos",
    "pmos",
    0
};

IFkeys B1keys[] = {
    IFkeys( 'm', B1names, 4, 4, 0 )
};

} // namespace


B1dev::B1dev()
{
    dv_name = "BSIM1";
    dv_description = "Berkeley Short Channel MOSFET Model BSIM1";

    dv_numKeys = NUMELEMS(B1keys);
    dv_keys = B1keys;

    dv_levels[0] = 4;
    dv_levels[1] = 0;
    dv_modelKeys = B1modNames;

    dv_numInstanceParms = NUMELEMS(B1pTable);
    dv_instanceParms = B1pTable;

    dv_numModelParms = NUMELEMS(B1mPTable);
    dv_modelParms = B1mPTable;

    dv_flags = DV_TRUNC | DV_NONOIS;
};


sGENmodel *
B1dev::newModl()
{
    return (new sB1model);
}


sGENinstance *
B1dev::newInst()
{
    return (new sB1instance);
}


int
B1dev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sB1model, sB1instance>(model));
}


int
B1dev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sB1model, sB1instance>(model, dname,
        fast));
}


int
B1dev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sB1model, sB1instance>(model, modname,
        modfast));
}


// mosfet parser
// Mname <node> <node> <node> <node> <model>
//       [L=<val>] [W=<val>] [AD=<val>] [AS=<val>] [PD=<val>]
//       [PS=<val>] [NRD=<val>] [NRS=<val>] [OFF]
//       [IC=<val>,<val>,<val>]
// 
void
B1dev::parse(int type, sCKT *ckt, sLine *current)
{
    DEV.parse(ckt, current, type, dv_keys->minTerms, dv_keys->maxTerms,
        true, 0);
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
        *d = new B1dev;
        (*cnt)++;
    }
}

