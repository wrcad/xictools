
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
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mesdefs.h"


namespace {

IFparm MESpTable[] = {
IO("area",              MES_AREA,           IF_REAL|IF_AREA,
                "Area factor"),
IO("off",               MES_OFF,            IF_FLAG,
                "Device initially off"),
IO("icvds",             MES_IC_VDS,         IF_REAL|IF_VOLT|IF_AC,
                "Initial D-S voltage"),
IO("icvgs",             MES_IC_VGS,         IF_REAL|IF_VOLT|IF_AC,
                "Initial G-S voltage"),
IP("ic",                MES_IC,             IF_REALVEC|IF_VOLT,
                "Initial VDS,VGS vector"),
OP("vgs",               MES_VGS,            IF_REAL|IF_VOLT,
                "Gate-Source voltage"),
OP("vgd",               MES_VGD,            IF_REAL|IF_VOLT,
                "Gate-Drain voltage"),
OP("cg",                MES_CG,             IF_REAL|IF_AMP|IF_USEALL,
                "Gate current"),
OP("cd",                MES_CD,             IF_REAL|IF_AMP|IF_USEALL,
                "Drain current"),
OP("cgd",               MES_CGD,            IF_REAL|IF_AMP,
                "Gate-Drain current"),
OP("cs",                MES_CS,             IF_REAL|IF_AMP|IF_USEALL,
                "Source current"),
OP("p",                 MES_POWER,          IF_REAL|IF_POWR,
                "Power dissipated by the mesfet"),
OP("gm",                MES_GM,             IF_REAL|IF_COND,
                "Transconductance"),
OP("gds",               MES_GDS,            IF_REAL|IF_COND,
                "Drain-Source conductance"),
OP("ggs",               MES_GGS,            IF_REAL|IF_COND,
                "Gate-Source conductance"),
OP("ggd",               MES_GGD,            IF_REAL|IF_COND,
                "Gate-Drain conductance"),
OP("qgs",               MES_QGS,            IF_REAL|IF_CHARGE,
                "Gate-Source charge storage"),
OP("qgd",               MES_QGD,            IF_REAL|IF_CHARGE,
                "Gate-Drain charge storage"),
OP("cqgs",              MES_CQGS,           IF_REAL|IF_CHARGE,
                "Charge storage capacitance G-S junction"),
OP("cqgd",              MES_CQGD,           IF_REAL|IF_CHARGE,
                "Charge storage capacitance G-D junction"),
OP("dnode",             MES_DRAINNODE,      IF_INTEGER,
                "Number of drain node"),
OP("gnode",             MES_GATENODE,       IF_INTEGER,
                "Number of gate node"),
OP("snode",             MES_SOURCENODE,     IF_INTEGER,
                "Number of source node"),
OP("dprimenode",        MES_DRAINPRIMENODE, IF_INTEGER,
                "Internal drain node number"),
OP("sprimenode",        MES_SOURCEPRIMENODE,IF_INTEGER,
                "Internal source node number")
};

IFparm MESmPTable[] = {
IO("vt0",               MES_MOD_VTO,        IF_REAL,
                "Pinch-off voltage"),
IO("vto",               MES_MOD_VTO,        IF_REAL|IF_REDUNDANT,
                "Pinch-off voltage"),
IO("alpha",             MES_MOD_ALPHA,      IF_REAL,
                "Saturation voltage parameter"),
IO("beta",              MES_MOD_BETA,       IF_REAL,
                "Transconductance parameter"),
IO("lambda",            MES_MOD_LAMBDA,     IF_REAL,
                "Channel length modulation parm."),
IO("b",                 MES_MOD_B,          IF_REAL,
                "Doping tail extending parameter"),
IO("rd",                MES_MOD_RD,         IF_REAL,
                "Drain ohmic resistance"),
IO("rs",                MES_MOD_RS,         IF_REAL,
                "Source ohmic resistance"),
IO("cgs",               MES_MOD_CGS,        IF_REAL|IF_AC,
                "G-S junction capacitance"),
IO("cgd",               MES_MOD_CGD,        IF_REAL|IF_AC,
                "G-D junction capacitance"),
IO("pb",                MES_MOD_PB,         IF_REAL,
                "Gate junction potential"),
IO("is",                MES_MOD_IS,         IF_REAL,
                "Junction saturation current"),
IO("fc",                MES_MOD_FC,         IF_REAL,
                "Forward biad junction fit parm."),
IP("nmf",               MES_MOD_NMF,        IF_FLAG,
                "N type MESfet model"),
IP("pmf",               MES_MOD_PMF,        IF_FLAG,
                "P type MESfet model"),
IP("kf",                MES_MOD_KF,         IF_REAL,
                "Flicker noise coefficient"),
IP("af",                MES_MOD_AF,         IF_REAL,
                "Flicker noise exponent"),
OP("gd",                MES_MOD_DRAINCOND,  IF_REAL,
                "Drain conductance"),
OP("gs",                MES_MOD_SOURCECOND, IF_REAL,
                "Source conductance"),
OP("depl_cap",          MES_MOD_DEPLETIONCAP,IF_REAL,
                "Depletion capacitance"),
OP("vcrit",             MES_MOD_VCRIT,      IF_REAL,
                "Critical voltage"),
OP("type",              MES_MOD_TYPE,       IF_FLAG,
                "N-type or P-type MESfet model")
};

const char *MESnames[] = {
    "Drain",
    "Gate",
    "Source"
};

const char *MESmodNames[] = {
    "nmf",
    "pmf",
    0
};

IFkeys MESkeys[] = {
    IFkeys( 'z', MESnames, 3, 3, 0 )
};

} // namespace


MESdev::MESdev()
{
    dv_name = "MES";
    dv_description = "GaAs MESFET model";

    dv_numKeys = NUMELEMS(MESkeys);
    dv_keys = MESkeys;

    dv_levels[0] = 1;
    dv_levels[1] = 0;
    dv_modelKeys = MESmodNames;

    dv_numInstanceParms = NUMELEMS(MESpTable);
    dv_instanceParms = MESpTable;

    dv_numModelParms = NUMELEMS(MESmPTable);
    dv_modelParms = MESmPTable;

    dv_flags = DV_TRUNC;
};


sGENmodel *
MESdev::newModl()
{
    return (new sMESmodel);
}


sGENinstance *
MESdev::newInst()
{
    return (new sMESinstance);
}


int
MESdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sMESmodel, sMESinstance>(model));
}


int
MESdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sMESmodel, sMESinstance>(model, dname,
        fast));
}


int
MESdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sMESmodel, sMESinstance>(model, modname,
        modfast));
}


// GaAs mesfet parser
// Zname <node> <node> <node> <model> [<val>] [OFF] [IC=<val>,<val>]
//
void
MESdev::parse(int type, sCKT *ckt, sLine *current)
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
        *d = new MESdev;
        (*cnt)++;
    }
}

