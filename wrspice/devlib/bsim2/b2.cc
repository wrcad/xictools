
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
Authors: 1988 Min-Chie Jeng, Hong June Park, Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b2defs.h"


namespace {

IFparm B2pTable[] = {
IO("l",                 BSIM2_L,            IF_REAL|IF_LEN,
                "Length"),
IO("w",                 BSIM2_W,            IF_REAL|IF_LEN,
                "Width"),
IO("as",                BSIM2_AS,           IF_REAL|IF_AREA,
                "Source area"),
IO("ad",                BSIM2_AD,           IF_REAL|IF_AREA,
                "Drain area"),
IO("ps",                BSIM2_PS,           IF_REAL|IF_LEN,
                "Source perimeter"),
IO("pd",                BSIM2_PD,           IF_REAL|IF_LEN,
                "Drain perimeter"),
IO("nrs",               BSIM2_NRS,          IF_REAL,
                "Number of squares in source"),
IO("nrd",               BSIM2_NRD,          IF_REAL,
                "Number of squares in drain"),
IO("off",               BSIM2_OFF,          IF_FLAG,
                "Device is initially off"),
IO("ic_vbs",            BSIM2_IC_VBS,       IF_REAL|IF_VOLT,
                "Initial B-S voltage"),
IO("ic_vds",            BSIM2_IC_VDS,       IF_REAL|IF_VOLT,
                "Initial D-S voltage"),
IO("ic_vgs",            BSIM2_IC_VGS,       IF_REAL|IF_VOLT,
                "Initial G-S voltage"),
IP("ic",                BSIM2_IC,           IF_REALVEC|IF_VOLT,
                "Vector of DS,GS,BS initial voltages"),
OP("vbd",               BSIM2_VBD,          IF_REAL|IF_VOLT,
                "Bulk-Drain voltage"),
OP("vbs",               BSIM2_VBS,          IF_REAL|IF_VOLT,
                "Bulk-Source voltage"),
OP("vgs",               BSIM2_VGS,          IF_REAL|IF_VOLT,
                "Gate-Source voltage"),
OP("vds",               BSIM2_VDS,          IF_REAL|IF_VOLT,
                "Drain-Source voltage"),
OP("von",               BSIM2_VON,          IF_REAL|IF_VOLT,
                "Turn-on voltage"),
OP("id",                BSIM2_CD,           IF_REAL|IF_AMP|IF_USEALL,
                "Drain current"),
OP("ibs",               BSIM2_CBS,          IF_REAL|IF_AMP,
                "B-S junction current"),
OP("ibd",               BSIM2_CBD,          IF_REAL|IF_AMP,
                "B-D junction current"),
OP("sourceconduct",     BSIM2_SOURCECOND,   IF_REAL|IF_COND,
                "Source conductance"),
OP("drainconduct",      BSIM2_DRAINCOND,    IF_REAL|IF_COND,
                "Drain conductance"),
OP("gm",                BSIM2_GM,           IF_REAL|IF_COND,
                "Transconductance"),
OP("gds",               BSIM2_GDS,          IF_REAL|IF_COND,
                "Drain-Source conductance"),
OP("gmbs",              BSIM2_GMBS,         IF_REAL|IF_COND,
                "Bulk-Source transconductance"),
OP("gbd",               BSIM2_GBD,          IF_REAL|IF_COND,
                "Bulk-Drain conductance"),
OP("gbs",               BSIM2_GBS,          IF_REAL|IF_COND,
                "Bulk-Source conductance"),
OP("qb",                BSIM2_QB,           IF_REAL|IF_CHARGE,
                "Bulk charge storage"),
OP("qg",                BSIM2_QG,           IF_REAL|IF_CHARGE,
                "Gate charge storage"),
OP("qd",                BSIM2_QD,           IF_REAL|IF_CHARGE,
                "Drain charge storage"),
OP("qbs",               BSIM2_QBS,          IF_REAL|IF_CHARGE,
                "Bulk-Source charge storage"),
OP("qbd",               BSIM2_QBD,          IF_REAL|IF_CHARGE,
                "Bulk-Drain charge storage"),
OP("cqb",               BSIM2_CQB,          IF_REAL|IF_AMP,
                "Bulk current"),
OP("cqg",               BSIM2_CQG,          IF_REAL|IF_AMP,
                "Gate current"),
OP("cqd",               BSIM2_CQD,          IF_REAL|IF_AMP,
                "Drain current"),
OP("cgg",               BSIM2_CGG,          IF_REAL|IF_AMP,
                "Gate current"),
OP("cgd",               BSIM2_CGD,          IF_REAL|IF_AMP,
                "Gate-Drain capacitance current"),
OP("cgs",               BSIM2_CGS,          IF_REAL|IF_AMP,
                "Gate-Source capacitance current"),
OP("cbg",               BSIM2_CBG,          IF_REAL|IF_AMP,
                "Gate-Bulk capacitance current"),
OP("cbd",               BSIM2_CAPBD,        IF_REAL|IF_CAP,
                "Drain-Bulk capacitance current"),
OP("cqbd",              BSIM2_CQBD,         IF_REAL|IF_AMP,
                "Current due to drain-bulk cap"),
OP("cbs",               BSIM2_CAPBS,        IF_REAL|IF_CAP,
                "Source-Bulk capacitance"),
OP("cqbs",              BSIM2_CQBS,         IF_REAL|IF_AMP,
                "Current due to source-bulk cap"),
OP("cdg",               BSIM2_CDG,          IF_REAL|IF_AMP,
                "Drain-Gate current"),
OP("cdd",               BSIM2_CDD,          IF_REAL|IF_AMP,
                "Gate-Bulk current"),
OP("cds",               BSIM2_CDS,          IF_REAL|IF_AMP,
                "Drain-Source current"),
OP("drainnode",         BSIM2_DNODE,        IF_INTEGER,
                "Number of drain node"),
OP("gatenode",          BSIM2_GNODE,        IF_INTEGER,
                "Number of gate node"),
OP("sourcenode",        BSIM2_SNODE,        IF_INTEGER,
                "Number of source node"),
OP("bulknode",          BSIM2_BNODE,        IF_INTEGER,
                "Number of bulk node"),
OP("drainprinenode",    BSIM2_DNODEPRIME,   IF_INTEGER,
                "Number of internal drain node"),
OP("sourceprimenode",   BSIM2_SNODEPRIME,   IF_INTEGER,
                "Number of internal source node"),
IO("m",                 BSIM2_M,            IF_REAL,
                "Instance multiplier")
};

IFparm B2mPTable[] = {
IO("vfb",               BSIM2_MOD_VFB0,     IF_REAL,
                "Flat band voltage"),
IO("lvfb",              BSIM2_MOD_VFBL,     IF_REAL,
                "Length dependence of vfb"),
IO("wvfb",              BSIM2_MOD_VFBW,     IF_REAL,
                "Width dependence of vfb"),
IO("phi",               BSIM2_MOD_PHI0,     IF_REAL,
                "Strong inversion surface potential"),
IO("lphi",              BSIM2_MOD_PHIL,     IF_REAL,
                "Length dependence of phi"),
IO("wphi",              BSIM2_MOD_PHIW,     IF_REAL,
                "Width dependence of phi"),
IO("k1",                BSIM2_MOD_K10,      IF_REAL,
                "Bulk effect coefficient 1"),
IO("lk1",               BSIM2_MOD_K1L,      IF_REAL,
                "Length dependence of k1"),
IO("wk1",               BSIM2_MOD_K1W,      IF_REAL,
                "Width dependence of k1"),
IO("k2",                BSIM2_MOD_K20,      IF_REAL,
                "Bulk effect coefficient 2"),
IO("lk2",               BSIM2_MOD_K2L,      IF_REAL,
                "Length dependence of k2"),
IO("wk2",               BSIM2_MOD_K2W,      IF_REAL,
                "Width dependence of k2"),
IO("eta0",              BSIM2_MOD_ETA00,    IF_REAL,
                "VDS dependence of threshold voltage at VDD=0"),
IO("leta0",             BSIM2_MOD_ETA0L,    IF_REAL,
                "Length dependence of eta0"),
IO("weta0",             BSIM2_MOD_ETA0W,    IF_REAL,
                "Width dependence of eta0"),
IO("etab",              BSIM2_MOD_ETAB0,    IF_REAL,
                "VBS dependence of eta"),
IO("letab",             BSIM2_MOD_ETABL,    IF_REAL,
                "Length dependence of etab"),
IO("wetab",             BSIM2_MOD_ETABW,    IF_REAL,
                "Width dependence of etab"),
IO("dl",                BSIM2_MOD_DELTAL,   IF_REAL,
                "Channel length reduction in um"),
IO("dw",                BSIM2_MOD_DELTAW,   IF_REAL,
                "Channel width reduction in um"),
IO("mu0",               BSIM2_MOD_MOB00,    IF_REAL,
                "Low-field mobility, at VDS=0 VGS=VTH"),
IO("mu0b",              BSIM2_MOD_MOB0B0,   IF_REAL,
                "VBS dependence of low-field mobility"),
IO("lmu0b",             BSIM2_MOD_MOB0BL,   IF_REAL,
                "Length dependence of mu0b"),
IO("wmu0b",             BSIM2_MOD_MOB0BW,   IF_REAL,
                "Width dependence of mu0b"),
IO("mus0",              BSIM2_MOD_MOBS00,   IF_REAL,
                "Mobility at VDS=VDD VGS=VTH"),
IO("lmus0",             BSIM2_MOD_MOBS0L,   IF_REAL,
                "Length dependence of mus0"),
IO("wmus0",             BSIM2_MOD_MOBS0W,   IF_REAL,
                "Width dependence of mus"),
IO("musb",              BSIM2_MOD_MOBSB0,   IF_REAL,
                "VBS dependence of mus"),
IO("lmusb",             BSIM2_MOD_MOBSBL,   IF_REAL,
                "Length dependence of musb"),
IO("wmusb",             BSIM2_MOD_MOBSBW,   IF_REAL,
                "Width dependence of musb"),
IO("mu20",              BSIM2_MOD_MOB200,   IF_REAL,
                "VDS dependence of mu in tanh term"),
IO("lmu20",             BSIM2_MOD_MOB20L,   IF_REAL,
                "Length dependence of mu20"),
IO("wmu20",             BSIM2_MOD_MOB20W,   IF_REAL,
                "Width dependence of mu20"),
IO("mu2b",              BSIM2_MOD_MOB2B0,   IF_REAL,
                "VBS dependence of mu2"),
IO("lmu2b",             BSIM2_MOD_MOB2BL,   IF_REAL,
                "Length dependence of mu2b"),
IO("wmu2b",             BSIM2_MOD_MOB2BW,   IF_REAL,
                "Width dependence of mu2b"),
IO("mu2g",              BSIM2_MOD_MOB2G0,   IF_REAL,
                "VGS dependence of mu2"),
IO("lmu2g",             BSIM2_MOD_MOB2GL,   IF_REAL,
                "Length dependence of mu2g"),
IO("wmu2g",             BSIM2_MOD_MOB2GW,   IF_REAL,
                "Width dependence of mu2g"),
IO("mu30",              BSIM2_MOD_MOB300,   IF_REAL,
                "VDS dependence of mu in linear term"),
IO("lmu30",             BSIM2_MOD_MOB30L,   IF_REAL,
                "Length dependence of mu30"),
IO("wmu30",             BSIM2_MOD_MOB30W,   IF_REAL,
                "Width dependence of mu30"),
IO("mu3b",              BSIM2_MOD_MOB3B0,   IF_REAL,
                "VBS dependence of mu3"),
IO("lmu3b",             BSIM2_MOD_MOB3BL,   IF_REAL,
                "Length dependence of mu3b"),
IO("wmu3b",             BSIM2_MOD_MOB3BW,   IF_REAL,
                "Width dependence of mu3b"),
IO("mu3g",              BSIM2_MOD_MOB3G0,   IF_REAL,
                "VGS dependence of mu3"),
IO("lmu3g",             BSIM2_MOD_MOB3GL,   IF_REAL,
                "Length dependence of mu3g"),
IO("wmu3g",             BSIM2_MOD_MOB3GW,   IF_REAL,
                "Width dependence of mu3g"),
IO("mu40",              BSIM2_MOD_MOB400,   IF_REAL,
                "VDS dependence of mu in linear term"),
IO("lmu40",             BSIM2_MOD_MOB40L,   IF_REAL,
                "Length dependence of mu40"),
IO("wmu40",             BSIM2_MOD_MOB40W,   IF_REAL,
                "Width dependence of mu40"),
IO("mu4b",              BSIM2_MOD_MOB4B0,   IF_REAL,
                "VBS dependence of mu4"),
IO("lmu4b",             BSIM2_MOD_MOB4BL,   IF_REAL,
                "Length dependence of mu4b"),
IO("wmu4b",             BSIM2_MOD_MOB4BW,   IF_REAL,
                "Width dependence of mu4b"),
IO("mu4g",              BSIM2_MOD_MOB4G0,   IF_REAL,
                "VGS dependence of mu4"),
IO("lmu4g",             BSIM2_MOD_MOB4GL,   IF_REAL,
                "Length dependence of mu4g"),
IO("wmu4g",             BSIM2_MOD_MOB4GW,   IF_REAL,
                "Width dependence of mu4g"),
IO("ua0",               BSIM2_MOD_UA00,     IF_REAL,
                "Linear VGS dependence of mobility"),
IO("lua0",              BSIM2_MOD_UA0L,     IF_REAL,
                "Length dependence of ua0"),
IO("wua0",              BSIM2_MOD_UA0W,     IF_REAL,
                "Width dependence of ua0"),
IO("uab",               BSIM2_MOD_UAB0,     IF_REAL,
                "VBS dependence of ua"),
IO("luab",              BSIM2_MOD_UABL,     IF_REAL,
                "Length dependence of uab"),
IO("wuab",              BSIM2_MOD_UABW,     IF_REAL,
                "Width dependence of uab"),
IO("ub0",               BSIM2_MOD_UB00,     IF_REAL,
                "Quadratic VGS dependence of mobility"),
IO("lub0",              BSIM2_MOD_UB0L,     IF_REAL,
                "Length dependence of ub0"),
IO("wub0",              BSIM2_MOD_UB0W,     IF_REAL,
                "Width dependence of ub0"),
IO("ubb",               BSIM2_MOD_UBB0,     IF_REAL,
                "VBS dependence of ub"),
IO("lubb",              BSIM2_MOD_UBBL,     IF_REAL,
                "Length dependence of ubb"),
IO("wubb",              BSIM2_MOD_UBBW,     IF_REAL,
                "Width dependence of ubb"),
IO("u10",               BSIM2_MOD_U100,     IF_REAL,
                "VDS depence of mobility"),
IO("lu10",              BSIM2_MOD_U10L,     IF_REAL,
                "Length dependence of u10"),
IO("wu10",              BSIM2_MOD_U10W,     IF_REAL,
                "Width dependence of u10"),
IO("u1b",               BSIM2_MOD_U1B0,     IF_REAL,
                "VBS depence of u1"),
IO("lu1b",              BSIM2_MOD_U1BL,     IF_REAL,
                "Length depence of u1b"),
IO("wu1b",              BSIM2_MOD_U1BW,     IF_REAL,
                "Width depence of u1b"),
IO("u1d",               BSIM2_MOD_U1D0,     IF_REAL,
                "VDS depence of u1"),
IO("lu1d",              BSIM2_MOD_U1DL,     IF_REAL,
                "Length depence of u1d"),
IO("wu1d",              BSIM2_MOD_U1DW,     IF_REAL,
                "Width depence of u1d"),
IO("n0",                BSIM2_MOD_N00,      IF_REAL,
                "Subthreshold slope at VDS=0 VBS=0"),
IO("ln0",               BSIM2_MOD_N0L,      IF_REAL,
                "Length dependence of n0"),
IO("wn0",               BSIM2_MOD_N0W,      IF_REAL,
                "Width dependence of n0"),
IO("nb",                BSIM2_MOD_NB0,      IF_REAL,
                "VBS dependence of n"),
IO("lnb",               BSIM2_MOD_NBL,      IF_REAL,
                "Length dependence of nb"),
IO("wnb",               BSIM2_MOD_NBW,      IF_REAL,
                "Width dependence of nb"),
IO("nd",                BSIM2_MOD_ND0,      IF_REAL,
                "VDS dependence of n"),
IO("lnd",               BSIM2_MOD_NDL,      IF_REAL,
                "Length dependence of nd"),
IO("wnd",               BSIM2_MOD_NDW,      IF_REAL,
                "Width dependence of nd"),
IO("vof0",              BSIM2_MOD_VOF00,    IF_REAL,
                "Threshold voltage offset AT VDS=0 VBS=0"),
IO("lvof0",             BSIM2_MOD_VOF0L,    IF_REAL,
                "Length dependence of vof0"),
IO("wvof0",             BSIM2_MOD_VOF0W,    IF_REAL,
                "Width dependence of vof0"),
IO("vofb",              BSIM2_MOD_VOFB0,    IF_REAL,
                "VBS dependence of vof"),
IO("lvofb",             BSIM2_MOD_VOFBL,    IF_REAL,
                "Length dependence of vofb"),
IO("wvofb",             BSIM2_MOD_VOFBW,    IF_REAL,
                "Width dependence of vofb"),
IO("vofd",              BSIM2_MOD_VOFD0,    IF_REAL,
                "VDS dependence of vof"),
IO("lvofd",             BSIM2_MOD_VOFDL,    IF_REAL,
                "Length dependence of vofd"),
IO("wvofd",             BSIM2_MOD_VOFDW,    IF_REAL,
                "Width dependence of vofd"),
IO("ai0",               BSIM2_MOD_AI00,     IF_REAL,
                "Pre-factor of hot-electron effect."),
IO("lai0",              BSIM2_MOD_AI0L,     IF_REAL,
                "Length dependence of ai0"),
IO("wai0",              BSIM2_MOD_AI0W,     IF_REAL,
                "Width dependence of ai0"),
IO("aib",               BSIM2_MOD_AIB0,     IF_REAL,
                "VBS dependence of ai"),
IO("laib",              BSIM2_MOD_AIBL,     IF_REAL,
                "Length dependence of aib"),
IO("waib",              BSIM2_MOD_AIBW,     IF_REAL,
                "Width dependence of aib"),
IO("bi0",               BSIM2_MOD_BI00,     IF_REAL,
                "Exponential factor of hot-electron effect."),
IO("lbi0",              BSIM2_MOD_BI0L,     IF_REAL,
                "Length dependence of bi0"),
IO("wbi0",              BSIM2_MOD_BI0W,     IF_REAL,
                "Width dependence of bi0"),
IO("bib",               BSIM2_MOD_BIB0,     IF_REAL,
                "VBS dependence of bi"),
IO("lbib",              BSIM2_MOD_BIBL,     IF_REAL,
                "Length dependence of bib"),
IO("wbib",              BSIM2_MOD_BIBW,     IF_REAL,
                "Width dependence of bib"),
IO("vghigh",            BSIM2_MOD_VGHIGH0,  IF_REAL,
                "Upper bound of the cubic spline function."),
IO("lvghigh",           BSIM2_MOD_VGHIGHL,  IF_REAL,
                "Length dependence of vghigh"),
IO("wvghigh",           BSIM2_MOD_VGHIGHW,  IF_REAL,
                "Width dependence of vghigh"),
IO("vglow",             BSIM2_MOD_VGLOW0,   IF_REAL,
                "Lower bound of the cubic spline function."),
IO("lvglow",            BSIM2_MOD_VGLOWL,   IF_REAL,
                "Length dependence of vglow"),
IO("wvglow",            BSIM2_MOD_VGLOWW,   IF_REAL,
                "Width dependence of vglow"),
IO("tox",               BSIM2_MOD_TOX,      IF_REAL,
                "Gate oxide thickness in um"),
IO("temp",              BSIM2_MOD_TEMP,     IF_REAL,
                "Temperature in degree Celcius"),
IO("vdd",               BSIM2_MOD_VDD,      IF_REAL,
                "Maximum Vds"),
IO("vgg",               BSIM2_MOD_VGG,      IF_REAL,
                "Maximum Vgs"),
IO("vbb",               BSIM2_MOD_VBB,      IF_REAL,
                "Maximum Vbs"),
IO("cgso",              BSIM2_MOD_CGSO,     IF_REAL|IF_AC,
                "Gate source overlap capacitance per unit channel width(m"),
IO("cgdo",              BSIM2_MOD_CGDO,     IF_REAL|IF_AC,
                "Gate drain overlap capacitance per unit channel width(m"),
IO("cgbo",              BSIM2_MOD_CGBO,     IF_REAL|IF_AC,
                "Gate bulk overlap capacitance per unit channel length(m"),
IO("xpart",             BSIM2_MOD_XPART,    IF_REAL,
                "Flag for channel charge partitioning"),
IO("rsh",               BSIM2_MOD_RSH,      IF_REAL,
                "Source drain diffusion sheet resistance in ohm per square"),
IO("js",                BSIM2_MOD_JS,       IF_REAL,
                "Source drain junction saturation current per unit area"),
IO("pb",                BSIM2_MOD_PB,       IF_REAL,
                "Source drain junction built in potential"),
IO("mj",                BSIM2_MOD_MJ,       IF_REAL|IF_AC,
                "Source drain bottom junction capacitance grading coefficient"),
IO("pbsw",              BSIM2_MOD_PBSW,     IF_REAL|IF_AC,
                "Source drain side junction capacitance built in potential"),
IO("mjsw",              BSIM2_MOD_MJSW,     IF_REAL|IF_AC,
                "Source drain side junction capacitance grading coefficient"),
IO("cj",                BSIM2_MOD_CJ,       IF_REAL|IF_AC,
                "Source drain bottom junction capacitance per unit area"),
IO("cjsw",              BSIM2_MOD_CJSW,     IF_REAL|IF_AC,
                "Source drain side junction capacitance per unit area"),
IO("wdf",               BSIM2_MOD_DEFWIDTH, IF_REAL,
                "Default width of source drain diffusion in um"),
IO("dell",              BSIM2_MOD_DELLENGTH,IF_REAL,
                "Length reduction of source drain diffusion"),
IP("nmos",              BSIM2_MOD_NMOS,     IF_FLAG,
                "Flag to indicate NMOS"),
IP("pmos",              BSIM2_MOD_PMOS,     IF_FLAG,
                "Flag to indicate PMOS")
};

const char *B2names[] = {
    "Drain",
    "Gate",
    "Source",
    "Bulk"
};

const char *B2modNames[] = {
    "nmos",
    "pmos",
    0
};

IFkeys B2keys[] = {
    IFkeys( 'm', B2names, 4, 4, 0 )
};

} // namespace


B2dev::B2dev()
{
    dv_name = "BSIM2";
    dv_description = "Berkeley Short Channel MOSFET Model BSIM2";

    dv_numKeys = NUMELEMS(B2keys);
    dv_keys = B2keys;

    dv_levels[0] = 5;
    dv_levels[1] = 0;
    dv_modelKeys = B2modNames;

    dv_numInstanceParms = NUMELEMS(B2pTable);
    dv_instanceParms = B2pTable;

    dv_numModelParms = NUMELEMS(B2mPTable);
    dv_modelParms = B2mPTable;

    dv_flags = DV_TRUNC;
};


sGENmodel *
B2dev::newModl()
{
    return (new sB2model);
}


sGENinstance *
B2dev::newInst()
{
    return (new sB2instance);
}


int
B2dev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sB2model, sB2instance>(model));
}


int
B2dev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sB2model, sB2instance>(model, dname,
        fast));
}


int
B2dev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sB2model, sB2instance>(model, modname,
        modfast));
}


// mosfet parser
// Mname <node> <node> <node> <node> <model>
//       [L=<val>] [W=<val>] [AD=<val>] [AS=<val>] [PD=<val>]
//       [PS=<val>] [NRD=<val>] [NRS=<val>] [OFF]
//       [IC=<val>,<val>,<val>]
// 
void
B2dev::parse(int type, sCKT *ckt, sLine *current)
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
        *d = new B2dev;
        (*cnt)++;
    }
}

