
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
 $Id: dio.cc,v 2.26 2016/09/26 01:48:01 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
Modified by Dietmar Warning 2003 and Paolo Nenzi 2003
**********/

#include "diodefs.h"

namespace {

IFparm DIOpTable[] = {
IO("off",               DIO_OFF,            IF_FLAG,
                "Initially off"),
IO("temp",              DIO_TEMP,           IF_REAL | IF_TEMP,
                "Instance temperature"),
IO("dtemp",             DIO_DTEMP,          IF_REAL | IF_TEMP,
                "Instance delta temperature"),
IO("ic",                DIO_IC,             IF_REAL | IF_VOLT | IF_AC,
                "Initial device voltage"),
IO("area",              DIO_AREA,           IF_REAL | IF_AREA,
                "Area factor"),
IO("pj",                DIO_PJ,             IF_REAL,
                "Perimeter factor"),
IO("m",                 DIO_M,              IF_REAL,
                "Multiplier"),
OP("vd",                DIO_VOLTAGE,        IF_REAL | IF_VOLT,
                "Diode voltage"),
OP("id",                DIO_CURRENT,        IF_REAL | IF_AMP | IF_USEALL,
                "Diode current"),
OP("c",                 DIO_CURRENT,        IF_REAL | IF_AMP | IF_REDUNDANT,
                "Diode current"),
OP("gd",                DIO_CONDUCT,        IF_REAL | IF_COND,
                "Diode conductance"),
OP("cd",                DIO_CAP,            IF_REAL | IF_CAP,
                "Diode capacitance"),
OP("charge",            DIO_CHARGE,         IF_REAL | IF_CHARGE,
                "Diode capacitor charge"),
OP("capcur",            DIO_CAPCUR,         IF_REAL | IF_AMP,
                "Diode capacitor current"),
OP("p",                 DIO_POWER,          IF_REAL | IF_POWR,
                "Diode power"),
OP("node1",             DIO_POSNODE,        IF_INTEGER,
                "Node 1 number"),
OP("node2",             DIO_NEGNODE,        IF_INTEGER,
                "Node 2 number"),
OP("nodeint",           DIO_INTNODE,        IF_INTEGER,
                "Internal node number")
};

IFparm DIOmPTable[] = {
IO("is",                DIO_MOD_IS,         IF_REAL,
                "Saturation current"),
IO("js",                DIO_MOD_IS,         IF_REAL,
                "Saturation current"),
IO("jsw",               DIO_MOD_JSW,        IF_REAL,
                "Sidewall Saturation current"),

IO("tnom",              DIO_MOD_TNOM,       IF_REAL,
                "Parameter measurement temperature"),
IO("tref",              DIO_MOD_TNOM,       IF_REAL,
                "Parameter measurement temperature"),
IO("rs",                DIO_MOD_RS,         IF_REAL,
                "Ohmic resistance"),
IO("trs",               DIO_MOD_TRS,        IF_REAL,
                "Ohmic resistance 1st order temp. coeff."),
IO("trs1",              DIO_MOD_TRS,        IF_REAL,
                "Ohmic resistance 1st order temp. coeff."),
IO("trs2",              DIO_MOD_TRS2,       IF_REAL,
                "Ohmic resistance 2nd order temp. coeff."),
IO("n",                 DIO_MOD_N,          IF_REAL,
                "Emission Coefficient"),
IO("tt",                DIO_MOD_TT,         IF_REAL,
                "Transit Time"),
IO("ttt1",              DIO_MOD_TTT1,       IF_REAL,
                "Transit Time 1st order temp. coeff."),
IO("ttt2",              DIO_MOD_TTT2,       IF_REAL,
                "Transit Time 2nd order temp. coeff."),
IO("cjo",               DIO_MOD_CJO,        IF_REAL,
                "Junction capacitance"),
IO("cj0",               DIO_MOD_CJO,        IF_REAL,
                "Junction capacitance"),
IO("cj",                DIO_MOD_CJO,        IF_REAL,
                "Junction capacitance"),
IO("pj",                DIO_MOD_PJ,         IF_REAL,
                "Junction perimeter"),
IO("area",              DIO_MOD_AREA,       IF_REAL,
                "Junction area"),
IO("vj",                DIO_MOD_VJ,         IF_REAL,
                "Junction potential"),
IO("pb",                DIO_MOD_VJ,         IF_REAL,
                "Junction potential"),
IO("m",                 DIO_MOD_M,          IF_REAL,
                "Grading coefficient"),
IO("mj",                DIO_MOD_M,          IF_REAL,
                "Grading coefficient"),
IO("tm1",               DIO_MOD_TM1,        IF_REAL,
                "Grading coefficient 1st temp. coeff."),
IO("tm2",               DIO_MOD_TM2,        IF_REAL,
                "Grading coefficient 2nd temp. coeff."),
IO("cjp",               DIO_MOD_CJSW,       IF_REAL,
                "Sidewall junction capacitance"),
IO("cjsw",              DIO_MOD_CJSW,       IF_REAL,
                "Sidewall junction capacitance"),
IO("php",               DIO_MOD_VJSW,       IF_REAL,
                "Sidewall junction potential"),
IO("mjsw",              DIO_MOD_MJSW,       IF_REAL,
                "Sidewall Grading coefficient"),
IO("ikf",               DIO_MOD_IKF,        IF_REAL,
                "Forward Knee current"),
IO("ik",                DIO_MOD_IKF,        IF_REAL,
                "Forward Knee current"),
IO("ikr",               DIO_MOD_IKR,        IF_REAL,
                "Reverse Knee current"),

IO("eg",                DIO_MOD_EG,         IF_REAL,
                "Activation energy"),
IO("xti",               DIO_MOD_XTI,        IF_REAL,
                "Saturation current temperature exp."),
IO("kf",                DIO_MOD_KF,         IF_REAL,
                "Flicker noise coefficient"),
IO("af",                DIO_MOD_AF,         IF_REAL,
                "Flicker noise exponent"),
IO("fc",                DIO_MOD_FC,         IF_REAL,
                "Forward bias junction fit parameter"),
IO("fcs",               DIO_MOD_FCS,        IF_REAL,
                "Forward bias sidewall junction fit parameter"),
IO("bv",                DIO_MOD_BV,         IF_REAL,
                "Reverse breakdown voltage"),
IO("ibv",               DIO_MOD_IBV,        IF_REAL,
                "Current at reverse breakdown voltage"),
OP("cond",              DIO_MOD_COND,       IF_REAL,
                "Ohmic conductance"),

IO("cta",               DIO_MOD_CTA,        IF_REAL,
                "Junction capacitance temperature coeff."),
IO("ctp",               DIO_MOD_CTP,        IF_REAL,
                "Periphery capacitance temperature cieff."),
IO("tcv",               DIO_MOD_TCV,        IF_REAL,
                "Breakdown voltage temperature coeff."),
IO("tlev",              DIO_MOD_TLEV,       IF_REAL,
                "Termperature equation selector"),
IO("tlevc",             DIO_MOD_TLEVC,      IF_REAL,
                "Capacitance temperature equation"),
IO("tpb",               DIO_MOD_TPB,        IF_REAL,
                "Temperature coefficient for PB"),
IO("tphp",              DIO_MOD_TPHP,       IF_REAL,
                "Temperature coefficient for PHP"),

IP("d",                 DIO_MOD_D,          IF_FLAG,
                "Diode model")
};

const char *DIOnames[] = {
    "D+",
    "D-"
};

const char *DIOmodNames[] = {
    "d",
    0
};

IFkeys DIOkeys[] = {
    IFkeys( 'd', DIOnames, 2, 2, 0 )
};

} // namespace


DIOdev::DIOdev()
{
    dv_name = "Diode";
    dv_description = "Junction diode model";

    dv_numKeys = NUMELEMS(DIOkeys);
    dv_keys = DIOkeys;

    dv_levels[0] = 1;
    dv_levels[1] = 3;
    dv_levels[2] = 0;
    dv_modelKeys = DIOmodNames;

    dv_numInstanceParms = NUMELEMS(DIOpTable);
    dv_instanceParms = DIOpTable;

    dv_numModelParms = NUMELEMS(DIOmPTable);
    dv_modelParms = DIOmPTable;

    dv_flags = DV_TRUNC | DV_NOLEVCHG;
};


sGENmodel *
DIOdev::newModl()
{
    return (new sDIOmodel);
}


sGENinstance *
DIOdev::newInst()
{
    return (new sDIOinstance);
}


int
DIOdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sDIOmodel, sDIOinstance>(model));
}


int
DIOdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sDIOmodel, sDIOinstance>(model, dname,
        fast));
}


int
DIOdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sDIOmodel, sDIOinstance>(model, modname,
        modfast));
}


// pn diode parser
// Dname <node> <node> <model> [<val>] [OFF] [IC=<val>]
//
void
DIOdev::parse(int type, sCKT *ckt, sLine *current)
{
    DEV.parse(ckt, current, type, dv_keys->minTerms, dv_keys->maxTerms,
        true, "area");
}


void
DIOdev::backup(sGENmodel *mp, DEV_BKMODE m)
{
    while (mp) {
        for (sGENinstance *ip = mp->GENinstances; ip; ip = ip->GENnextInstance)
            ((sDIOinstance*)ip)->backup(m);
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
        *d = new DIOdev;
        (*cnt)++;
    }
}

