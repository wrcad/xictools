
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
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
Model Author: 1995 Colin McAndrew Motorola
Spice3 Implementation: 2003 Dietmar Warning DAnalyse GmbH
**********/

#include "vbicdefs.h"


//
// This file defines the VBIC data structures that are
// available to the next level(s) up the calling hierarchy.
//

namespace {

IFparm VBICpTable[] = {
IO("area",              VBIC_AREA,          IF_REAL | IF_AREA,
                "Area factor"),
IO("off",               VBIC_OFF,           IF_FLAG,
                "Device initially off"),
IP("ic",                VBIC_IC,            IF_REALVEC | IF_VOLT,
                "Initial condition vector"),
IO("icvbe",             VBIC_IC_VBE,        IF_REAL | IF_VOLT | IF_AC,
                "Initial B-E voltage"),
IO("icvce",             VBIC_IC_VCE,        IF_REAL | IF_VOLT | IF_AC,
                "Initial C-E voltage"),
IO("temp",              VBIC_TEMP,          IF_REAL | IF_TEMP,
                "Instance temperature"),
IO("dtemp",             VBIC_DTEMP,         IF_REAL | IF_TEMP,
                "Instance delta temperature"),
IO("m",                 VBIC_M,             IF_REAL,
                "Multiplier"),

OP("collnode",          VBIC_QUEST_COLLNODE,IF_INTEGER,
                "Number of collector node"),
OP("basenode",          VBIC_QUEST_BASENODE,IF_INTEGER,
                "Number of base node"),
OP("emitnode",          VBIC_QUEST_EMITNODE,IF_INTEGER,
                "Number of emitter node"),
OP("subsnode",          VBIC_QUEST_SUBSNODE,IF_INTEGER,
                "Number of substrate node"),
OP("collCXnode",        VBIC_QUEST_COLLCXNODE,IF_INTEGER,
                "Internal collector node"),
OP("collCInode",        VBIC_QUEST_COLLCINODE,IF_INTEGER,
                "Internal collector node"),
OP("baseBXnode",        VBIC_QUEST_BASEBXNODE,IF_INTEGER,
                "Internal base node"),
OP("baseBInode",        VBIC_QUEST_BASEBINODE,IF_INTEGER,
                "Internal base node"),
OP("baseBPnode",        VBIC_QUEST_BASEBPNODE,IF_INTEGER,
                "Internal base node"),
OP("emitEInode",        VBIC_QUEST_EMITEINODE,IF_INTEGER,
                "Internal emitter node"),
OP("subsSInode",        VBIC_QUEST_SUBSSINODE,IF_INTEGER,
                "Internal substrate node"),
OP("vbe",               VBIC_QUEST_VBE,     IF_REAL | IF_VOLT,
                "B-E voltage"),
OP("vbc",               VBIC_QUEST_VBC,     IF_REAL | IF_VOLT,
                "B-C voltage"),
OP("ic",                VBIC_QUEST_CC,      IF_REAL | IF_AMP | IF_USEALL,
                "Collector current"),
OP("ib",                VBIC_QUEST_CB,      IF_REAL | IF_AMP | IF_USEALL,
                "Base current"),
OP("ie",                VBIC_QUEST_CE,      IF_REAL | IF_AMP | IF_USEALL,
                "Emitter current"),
OP("is",                VBIC_QUEST_CS,      IF_REAL | IF_AMP,
                "Substrate current"),
OP("gm",                VBIC_QUEST_GM,      IF_REAL | IF_COND,
                "Small signal transconductance dIc/dVbe"),
OP("go",                VBIC_QUEST_GO,      IF_REAL | IF_COND,
                "Small signal output conductance dIc/dVbc"),
OP("gpi",               VBIC_QUEST_GPI,     IF_REAL | IF_COND,
                "Small signal input conductance dIb/dVbe"),
OP("gmu",               VBIC_QUEST_GMU,     IF_REAL | IF_COND,
                "Small signal conductance dIb/dVbc"),
OP("gx",                VBIC_QUEST_GX,      IF_REAL | IF_COND,
                "Conductance from base to internal base"),
OP("cbe",               VBIC_QUEST_CBE,     IF_REAL | IF_CAP,
                "Internal base to emitter capacitance"),
OP("cbex",              VBIC_QUEST_CBEX,    IF_REAL | IF_CAP,
                "External base to emitter capacitance"),
OP("cbc",               VBIC_QUEST_CBC,     IF_REAL | IF_CAP,
                "Internal base to collector capacitance"),
OP("cbcx",              VBIC_QUEST_CBCX,    IF_REAL | IF_CAP,
                "External Base to collector capacitance"),
OP("cbep",              VBIC_QUEST_CBEP,    IF_REAL | IF_CAP,
                "Parasitic Base to emitter capacitance"),
OP("cbcp",              VBIC_QUEST_CBCP,    IF_REAL | IF_CAP,
                "Parasitic Base to collector capacitance"),
OP("p",                 VBIC_QUEST_POWER,   IF_REAL | IF_POWR,
                "Power dissipation"),
OP("geqcb",             VBIC_QUEST_GEQCB,   IF_REAL | IF_COND,
                "Internal C-B-base cap. equiv. cond."),
OP("geqbx",             VBIC_QUEST_GEQBX,   IF_REAL | IF_COND,
                "External C-B-base cap. equiv. cond."),
OP("qbe",               VBIC_QUEST_QBE,     IF_REAL | IF_CHARGE,
                "Charge storage B-E junction"),
OP("cqbe",              VBIC_QUEST_CQBE,    IF_REAL | IF_CAP,
                "Cap. due to charge storage in B-E jct."),
OP("qbc",               VBIC_QUEST_QBC,     IF_REAL | IF_CHARGE,
                "Charge storage B-C junction"),
OP("cqbc",              VBIC_QUEST_CQBC,    IF_REAL | IF_CAP,
                "Cap. due to charge storage in B-C jct."),
OP("qbx",               VBIC_QUEST_QBX,     IF_REAL | IF_CHARGE,
                "Charge storage B-X junction"),
OP("cqbx",              VBIC_QUEST_CQBX,    IF_REAL | IF_CAP,
                "Cap. due to charge storage in B-X jct.")
/*
OP("sens_dc",           VBIC_QUEST_SENS_DC, IF_REAL,
                "DC sensitivity"),
OP("sens_real",         VBIC_QUEST_SENS_REAL,IF_REAL,
                "Real part of AC sensitivity"),
OP("sens_imag",         VBIC_QUEST_SENS_IMAG,IF_REAL,
                "DC sens. & imag part of AC sens."),
OP("sens_mag",          VBIC_QUEST_SENS_MAG,IF_REAL,
                "Sensitivity of AC magnitude"),
OP("sens_ph",           VBIC_QUEST_SENS_PH, IF_REAL,
                "Sensitivity of AC phase"),
OP("sens_cplx",         VBIC_QUEST_SENS_CPLX,IF_COMPLEX,
                "AC sensitivity")
*/
};

IFparm VBICmPTable[] = {
OP("type",              VBIC_MOD_TYPE,      IF_STRING,
                "NPN or PNP"),
IO("npn",               VBIC_MOD_NPN,       IF_FLAG,
                "NPN type device"),
IO("pnp",               VBIC_MOD_PNP,       IF_FLAG,
                "PNP type device"),
IO("tnom",              VBIC_MOD_TNOM,      IF_REAL,
                "Parameter measurement temperature"),
IO("rcx",               VBIC_MOD_RCX,       IF_REAL,
                "Extrinsic coll resistance"),
IO("rci",               VBIC_MOD_RCI,       IF_REAL,
                "Intrinsic coll resistance"),
IO("vo",                VBIC_MOD_VO,        IF_REAL,
                "Epi drift saturation voltage"),
IO("gamm",              VBIC_MOD_GAMM,      IF_REAL,
                "Epi doping parameter"),
IO("hrcf",              VBIC_MOD_HRCF,      IF_REAL,
                "High current RC factor"),
IO("rbx",               VBIC_MOD_RBX,       IF_REAL,
                "Extrinsic base resistance"),
IO("rbi",               VBIC_MOD_RBI,       IF_REAL,
                "Intrinsic base resistance"),
IO("re",                VBIC_MOD_RE,        IF_REAL,
                "Intrinsic emitter resistance"),
IO("rs",                VBIC_MOD_RS,        IF_REAL,
                "Intrinsic substrate resistance"),
IO("rbp",               VBIC_MOD_RBP,       IF_REAL,
                "Parasitic base resistance"),
IO("is",                VBIC_MOD_IS,        IF_REAL,
                "Transport saturation current"),
IO("nf",                VBIC_MOD_NF,        IF_REAL,
                "Forward emission coefficient"),
IO("nr",                VBIC_MOD_NR,        IF_REAL,
                "Reverse emission coefficient"),
IO("fc",                VBIC_MOD_FC,        IF_REAL,
                "Fwd bias depletion capacitance limit"),
IO("cbeo",              VBIC_MOD_CBEO,      IF_REAL,
                "Extrinsic B-E overlap capacitance"),
IO("cje",               VBIC_MOD_CJE,       IF_REAL,
                "Zero bias B-E depletion capacitance"),
IO("pe",                VBIC_MOD_PE,        IF_REAL,
                "B-E built in potential"),
IO("me",                VBIC_MOD_ME,        IF_REAL,
                "B-E junction grading coefficient"),
IO("aje",               VBIC_MOD_AJE,       IF_REAL,
                "B-E capacitance smoothing factor"),
IO("cbco",              VBIC_MOD_CBCO,      IF_REAL,
                "Extrinsic B-C overlap capacitance"),
IO("cjc",               VBIC_MOD_CJC,       IF_REAL,
                "Zero bias B-C depletion capacitance"),
IO("qco",               VBIC_MOD_QCO,       IF_REAL,
                "Epi charge parameter"),
IO("cjep",              VBIC_MOD_CJEP,      IF_REAL,
                "B-C extrinsic zero bias capacitance"),
IO("pc",                VBIC_MOD_PC,        IF_REAL,
                "B-C built in potential"),
IO("mc",                VBIC_MOD_MC,        IF_REAL,
                "B-C junction grading coefficient"),
IO("ajc",               VBIC_MOD_AJC,       IF_REAL,
                "B-C capacitance smoothing factor"),
IO("cjcp",              VBIC_MOD_CJCP,      IF_REAL,
                "Zero bias S-C capacitance"),
IO("ps",                VBIC_MOD_PS,        IF_REAL,
                "S-C junction built in potential"),
IO("ms",                VBIC_MOD_MS,        IF_REAL,
                "S-C junction grading coefficient"),
IO("ajs",               VBIC_MOD_AJS,       IF_REAL,
                "S-C capacitance smoothing factor"),
IO("ibei",              VBIC_MOD_IBEI,      IF_REAL,
                "Ideal B-E saturation current"),
IO("wbe",               VBIC_MOD_WBE,       IF_REAL,
                "Portion of IBEI from Vbei, 1-WBE from Vbex"),
IO("nei",               VBIC_MOD_NEI,       IF_REAL,
                "Ideal B-E emission coefficient"),
IO("iben",              VBIC_MOD_IBEN,      IF_REAL,
                "Non-ideal B-E saturation current"),
IO("nen",               VBIC_MOD_NEN,       IF_REAL,
                "Non-ideal B-E emission coefficient"),
IO("ibci",              VBIC_MOD_IBCI,      IF_REAL,
                "Ideal B-C saturation current"),
IO("nci",               VBIC_MOD_NCI,       IF_REAL,
                "Ideal B-C emission coefficient"),
IO("ibcn",              VBIC_MOD_IBCN,      IF_REAL,
                "Non-ideal B-C saturation current"),
IO("ncn",               VBIC_MOD_NCN,       IF_REAL,
                "Non-ideal B-C emission coefficient"),
IO("avc1",              VBIC_MOD_AVC1,      IF_REAL,
                "B-C weak avalanche parameter 1"),
IO("avc2",              VBIC_MOD_AVC2,      IF_REAL,
                "B-C weak avalanche parameter 2"),
IO("isp",               VBIC_MOD_ISP,       IF_REAL,
                "Parasitic transport saturation current"),
IO("wsp",               VBIC_MOD_WSP,       IF_REAL,
                "Portion of ICCP"),
IO("nfp",               VBIC_MOD_NFP,       IF_REAL,
                "Parasitic fwd emission coefficient"),
IO("ibeip",             VBIC_MOD_IBEIP,     IF_REAL,
                "Ideal parasitic B-E saturation current"),
IO("ibenp",             VBIC_MOD_IBENP,     IF_REAL,
                "Non-ideal parasitic B-E saturation current"),
IO("ibcip",             VBIC_MOD_IBCIP,     IF_REAL,
                "Ideal parasitic B-C saturation current"),
IO("ncip",              VBIC_MOD_NCIP,      IF_REAL,
                "Ideal parasitic B-C emission coefficient"),
IO("ibcnp",             VBIC_MOD_IBCNP,     IF_REAL,
                "Nonideal parasitic B-C saturation current"),
IO("ncnp",              VBIC_MOD_NCNP,      IF_REAL,
                "Nonideal parasitic B-C emission coefficient"),
IO("vef",               VBIC_MOD_VEF,       IF_REAL,
                "Forward Early voltage"),
IO("ver",               VBIC_MOD_VER,       IF_REAL,
                "Reverse Early voltage"),
IO("ikf",               VBIC_MOD_IKF,       IF_REAL,
                "Forward knee current"),
IO("ikr",               VBIC_MOD_IKR,       IF_REAL,
                "Reverse knee current"),
IO("ikp",               VBIC_MOD_IKP,       IF_REAL,
                "Parasitic knee current"),
IO("tf",                VBIC_MOD_TF,        IF_REAL,
                "Ideal forward transit time"),
IO("qtf",               VBIC_MOD_QTF,       IF_REAL,
                "Variation of TF with base-width modulation"),
IO("xtf",               VBIC_MOD_XTF,       IF_REAL,
                "Coefficient for bias dependence of TF"),
IO("vtf",               VBIC_MOD_VTF,       IF_REAL,
                "Voltage giving VBC dependence of TF"),
IO("itf",               VBIC_MOD_ITF,       IF_REAL,
                "High current dependence of TF"),
IO("tr",                VBIC_MOD_TR,        IF_REAL,
                "Ideal reverse transit time"),
IO("td",                VBIC_MOD_TD,        IF_REAL,
                "Forward excess-phase delay time"),
IO("kfn",               VBIC_MOD_KFN,       IF_REAL,
                "B-E Flicker Noise Coefficient"),
IO("afn",               VBIC_MOD_AFN,       IF_REAL,
                "B-E Flicker Noise Exponent"),
IO("bfn",               VBIC_MOD_BFN,       IF_REAL,
                "B-E Flicker Noise 1/f dependence"),
IO("xre",               VBIC_MOD_XRE,       IF_REAL,
                "Temperature exponent of RE"),
IO("xrbi",              VBIC_MOD_XRBI,      IF_REAL,
                "Temperature exponent of RBI"),
IO("xrci",              VBIC_MOD_XRCI,      IF_REAL,
                "Temperature exponent of RCI"),
IO("xrs",               VBIC_MOD_XRS,       IF_REAL,
                "Temperature exponent of RS"),
IO("xvo",               VBIC_MOD_XVO,       IF_REAL,
                "Temperature exponent of VO"),
IO("ea",                VBIC_MOD_EA,        IF_REAL,
                "Activation energy for IS"),
IO("eaie",              VBIC_MOD_EAIE,      IF_REAL,
                "Activation energy for IBEI"),
IO("eaic",              VBIC_MOD_EAIS,      IF_REAL,
                "Activation energy for IBCI/IBEIP"),
IO("eais",              VBIC_MOD_EAIS,      IF_REAL,
                "Activation energy for IBCIP"),
IO("eane",              VBIC_MOD_EANE,      IF_REAL,
                "Activation energy for IBEN"),
IO("eanc",              VBIC_MOD_EANC,      IF_REAL,
                "Activation energy for IBCN/IBENP"),
IO("eans",              VBIC_MOD_EANS,      IF_REAL,
                "Activation energy for IBCNP"),
IO("xis",               VBIC_MOD_XIS,       IF_REAL,
                "Temperature exponent of IS"),
IO("xii",               VBIC_MOD_XII,       IF_REAL,
                "Temperature exponent of IBEI,IBCI,IBEIP,IBCIP"),
IO("xin",               VBIC_MOD_XIN,       IF_REAL,
                "Temperature exponent of IBEN,IBCN,IBENP,IBCNP"),
IO("tnf",               VBIC_MOD_TNF,       IF_REAL,
                "Temperature exponent of NF"),
IO("tavc",              VBIC_MOD_TAVC,      IF_REAL,
                "Temperature exponent of AVC2"),
IO("rth",               VBIC_MOD_RTH,       IF_REAL,
                "Thermal resistance"),
IO("cth",               VBIC_MOD_CTH,       IF_REAL,
                "Thermal capacitance"),
IO("vrt",               VBIC_MOD_VRT,       IF_REAL,
                "Punch-through voltage of internal B-C junction"),
IO("art",               VBIC_MOD_ART,       IF_REAL,
                "Smoothing parameter for reach-through"),
IO("ccso",              VBIC_MOD_CCSO,      IF_REAL,
                "Fixed C-S capacitance"),
IO("qbm",               VBIC_MOD_QBM,       IF_REAL,
                "Select SGP qb formulation"),
IO("nkf",               VBIC_MOD_NKF,       IF_REAL,
                "High current beta rolloff"),
IO("xikf",              VBIC_MOD_XIKF,      IF_REAL,
                "Temperature exponent of IKF"),
IO("xrcx",              VBIC_MOD_XRCX,      IF_REAL,
                "Temperature exponent of RCX"),
IO("xrbx",              VBIC_MOD_XRBX,      IF_REAL,
                "Temperature exponent of RBX"),
IO("xrbp",              VBIC_MOD_XRBP,      IF_REAL,
                "Temperature exponent of RBP"),
IO("isrr",              VBIC_MOD_ISRR,      IF_REAL,
                "Separate IS for fwd and rev"),
IO("xisr",              VBIC_MOD_XISR,      IF_REAL,
                "Temperature exponent of ISR"),
IO("dear",              VBIC_MOD_DEAR,      IF_REAL,
                "Delta activation energy for ISRR"),
IO("eap",               VBIC_MOD_EAP,       IF_REAL,
                "Exitivation energy for ISP"),
IO("vbbe",              VBIC_MOD_VBBE,      IF_REAL,
                "B-E breakdown voltage"),
IO("nbbe",              VBIC_MOD_NBBE,      IF_REAL,
                "B-E breakdown emission coefficient"),
IO("ibbe",              VBIC_MOD_IBBE,      IF_REAL,
                "B-E breakdown current"),
IO("tvbbe1",            VBIC_MOD_TVBBE1,    IF_REAL,
                "Linear temperature coefficient of VBBE"),
IO("tvbbe2",            VBIC_MOD_TVBBE2,    IF_REAL,
                "Quadratic temperature coefficient of VBBE"),
IO("tnbbe",             VBIC_MOD_TNBBE,     IF_REAL,
                "Temperature coefficient of NBBE"),
IO("ebbe",              VBIC_MOD_EBBE,      IF_REAL,
                "Exp(-VBBE/(NBBE*Vtv"),
IO("dtemp",             VBIC_MOD_DTEMP,     IF_REAL,
                "Locale Temperature difference"),
IO("vers",              VBIC_MOD_VERS,      IF_REAL,
                "Revision Version"),
IO("vref",              VBIC_MOD_VREF,      IF_REAL,
                "Reference Version")
};

const char *VBICnames[] = {
    "collector",
    "base",
    "emitter",
    "substrate"
};

const char *VBICmodNames[] = {
    "npn",
    "pnp",
    0
};

IFkeys VBICkeys[] = {
    IFkeys( 'q', VBICnames, 3, 4, 0 )
};

} // namespace


VBICdev::VBICdev()
{
    dv_name = "VBIC";
    dv_description = "VBIC (NGspice17) bipolar junction transistor model";

    dv_numKeys = NUMELEMS(VBICkeys);
    dv_keys = VBICkeys;

    dv_levels[0] = 4;
    dv_levels[1] = 0;
    dv_modelKeys = VBICmodNames;

    dv_numInstanceParms = NUMELEMS(VBICpTable);
    dv_instanceParms = VBICpTable;

    dv_numModelParms = NUMELEMS(VBICmPTable);
    dv_modelParms = VBICmPTable;

    dv_flags = DV_TRUNC;
};


sGENmodel *
VBICdev::newModl()
{
    return (new sVBICmodel);
}


sGENinstance *
VBICdev::newInst()
{
    return (new sVBICinstance);
}


int
VBICdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sVBICmodel, sVBICinstance>(model));
}


int
VBICdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sVBICmodel, sVBICinstance>(model, dname,
        fast));
}


int
VBICdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sVBICmodel, sVBICinstance>(model, modname,
        modfast));
}


// bipolar transistor parser
// Qname <node> <node> <node> [<node>] <model> [<val>] [OFF]
//       [IC=<val>,<val>]
//
void
VBICdev::parse(int type, sCKT *ckt, sLine *current)
{
    DEV.parse(ckt, current, type, dv_keys->minTerms, dv_keys->maxTerms,
        true, "area");
}


void
VBICdev::backup(sGENmodel *mp, DEV_BKMODE m)
{
    while (mp) {
        for (sGENinstance *ip = mp->GENinstances; ip; ip = ip->GENnextInstance)
            ((sVBICinstance*)ip)->backup(m);
        mp = mp->GENnextModel;
    }
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
        *d = new VBICdev;
        (*cnt)++;
    }
}

