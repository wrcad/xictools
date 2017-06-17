
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
 $Id: jfet.cc,v 2.26 2016/09/26 01:48:13 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/
/********** new in 3f2
Sydney University mods Copyright(c) 1989 Anthony E. Parker, David J. Skellern
    Laboratory for Communication Science Engineering
    Sydney University Department of Electrical Engineering, Australia
**********/

#include "jfetdefs.h"


namespace {

IFparm JFETpTable[] = {
IO("area",              JFET_AREA,          IF_REAL|IF_AREA,
                "Area factor"),
IO("off",               JFET_OFF,           IF_FLAG,
                "Device initially off"),
IO("temp",              JFET_TEMP,          IF_REAL|IF_TEMP,
                "Instance temperature"),
IO("icvds",             JFET_IC_VDS,        IF_REAL|IF_VOLT|IF_AC,
                "Initial D-S voltage"),
IO("ic_vds",            JFET_IC_VDS,        IF_REAL|IF_VOLT|IF_AC,
                "Initial D-S voltage"),
IO("icvgs",             JFET_IC_VGS,        IF_REAL|IF_VOLT|IF_AC,
                "Initial G-S volrage"),
IO("ic_vgs",            JFET_IC_VGS,        IF_REAL|IF_VOLT|IF_AC,
                "Initial G-S volrage"),
IP("ic",                JFET_IC,            IF_REALVEC|IF_VOLT,
                "Initial VDS,VGS vector"),
OP("vgs",               JFET_VGS,           IF_REAL|IF_VOLT,
                "Gate-Source voltage"),
OP("vgd",               JFET_VGD,           IF_REAL|IF_VOLT,
                "Gate-Drain voltage"),
OP("ig",                JFET_CG,            IF_REAL|IF_AMP|IF_USEALL,
                "Gate current"),
OP("id",                JFET_CD,            IF_REAL|IF_AMP|IF_USEALL,
                "Drain current"),
OP("igd",               JFET_CGD,           IF_REAL|IF_AMP,
                "Gate-Drain current"),
OP("is",                JFET_CS,            IF_REAL|IF_AMP|IF_USEALL,
                "Source current"),
OP("p",                 JFET_POWER,         IF_REAL|IF_POWR,
                "Power dissipated by the JFET"),
OP("gm",                JFET_GM,            IF_REAL|IF_COND,
                "Transconductance"),
OP("gds",               JFET_GDS,           IF_REAL|IF_COND,
                "Drain-Source conductance"),
OP("ggs",               JFET_GGS,           IF_REAL|IF_COND,
                "Gate-Source conductance"),
OP("ggd",               JFET_GGD,           IF_REAL|IF_COND,
                "Gate-Drain conductance"),
OP("qgs",               JFET_QGS,           IF_REAL|IF_CHARGE,
                "Gate-Source charge storage"),
OP("qgd",               JFET_QGD,           IF_REAL|IF_CHARGE,
                "Gate-Drain charge storage"),
OP("cqgs",              JFET_CQGS,          IF_REAL|IF_CAP,
                "Charge storage capacitance G-S junction"),
OP("cqgd",              JFET_CQGD,          IF_REAL|IF_CAP,
                "Charge storage capacitance G-D junction"),
OP("drain_node",        JFET_DRAINNODE,     IF_INTEGER,
                "Number of drain node"),
OP("gate_node",         JFET_GATENODE,      IF_INTEGER,
                "Number of gate node"),
OP("source_node",       JFET_SOURCENODE,    IF_INTEGER,
                "Number of source node"),
OP("drain_prime_node",  JFET_DRAINPRIMENODE,IF_INTEGER,
                "Internal drain node number"),
OP("source_prime_node", JFET_SOURCEPRIMENODE,IF_INTEGER,
                "Internal source node number")
};

IFparm JFETmPTable[] = {
IO("vt0",               JFET_MOD_VTO,       IF_REAL,
                "Threshold voltage"),
IO("vto",               JFET_MOD_VTO,       IF_REAL|IF_REDUNDANT,
                "Threshold voltage"),
IO("beta",              JFET_MOD_BETA,      IF_REAL,
                "Transconductance parameter"),
IO("lambda",            JFET_MOD_LAMBDA,    IF_REAL,
                "Channel length modulation param."),
IO("rd",                JFET_MOD_RD,        IF_REAL,
                "Drain ohmic resistance"),
IO("rs",                JFET_MOD_RS,        IF_REAL,
                "Source ohmic resistance"),
IO("cgs",               JFET_MOD_CGS,       IF_REAL|IF_AC,
                "G-S junction capactance"),
IO("cgd",               JFET_MOD_CGD,       IF_REAL|IF_AC,
                "G-D junction cap"),
IO("pb",                JFET_MOD_PB,        IF_REAL,
                "Gate junction potential"),
IO("is",                JFET_MOD_IS,        IF_REAL,
                "Gate junction saturation current"),
IO("fc",                JFET_MOD_FC,        IF_REAL,
                "Forward bias junction fir parm."),
IP("njf",               JFET_MOD_NJF,       IF_FLAG,
                "N type JFET model"),
IP("pjf",               JFET_MOD_PJF,       IF_FLAG,
                "P type JFET model"),
IO("tnom",              JFET_MOD_TNOM,      IF_REAL,
                "Parameter measurement temperature"),
IP("kf",                JFET_MOD_KF,        IF_REAL,
                "Flicker Noise Coefficient"),
IP("af",                JFET_MOD_AF,        IF_REAL,
                "Flicker Noise Exponent"),
IO("b",                 JFET_MOD_B,         IF_REAL,
                "Doping tail parameter"),
OP("gd",                JFET_MOD_DRAINCOND, IF_REAL,
                "Drain conductance"),
OP("gs",                JFET_MOD_SOURCECOND,IF_REAL,
                "Source conductance"),
OP("type",              JFET_MOD_TYPE,      IF_STRING,
                "N-type or P-type JFET model")
};

const char *JFETnames[] = {
    "Drain",
    "Gate",
    "Source"
};

const char *JFETmodNames[] = {
    "njf",
    "pjf",
    0
};

IFkeys JFETkeys[] = {
    IFkeys( 'j', JFETnames, 3, 3, 0 )
};

} // namespace


JFETdev::JFETdev()
{
    dv_name = "JFET";
    dv_description = "Junction field effect transistor model";

    dv_numKeys = NUMELEMS(JFETkeys);
    dv_keys = JFETkeys;

    dv_levels[0] = 1;
    dv_levels[1] = 0;
    dv_modelKeys = JFETmodNames;

    dv_numInstanceParms = NUMELEMS(JFETpTable);
    dv_instanceParms = JFETpTable;

    dv_numModelParms = NUMELEMS(JFETmPTable);
    dv_modelParms = JFETmPTable;

    dv_flags = DV_TRUNC;
};


sGENmodel *
JFETdev::newModl()
{
    return (new sJFETmodel);
}


sGENinstance *
JFETdev::newInst()
{
    return (new sJFETinstance);
}


int
JFETdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sJFETmodel, sJFETinstance>(model));
}


int
JFETdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sJFETmodel, sJFETinstance>(model, dname,
        fast));
}


int
JFETdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sJFETmodel, sJFETinstance>(model, modname,
        modfast));
}


// JFET parser
// Jname <node> <node> <node> <model> [<val>] [OFF] [IC=<val>,<val>]
//
void
JFETdev::parse(int type, sCKT *ckt, sLine *current)
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
        *d = new JFETdev;
        (*cnt)++;
    }
}

