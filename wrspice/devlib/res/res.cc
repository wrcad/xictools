
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

#include "resdefs.h"


namespace {

IFparm RESpTable[] = {
IO("resistance",        RES_RESIST,         IF_PARSETREE|IF_RES|IF_PRINCIPAL,
                "Resistance"),
IO("res",               RES_RESIST,         IF_PARSETREE|IF_RES|IF_REDUNDANT,
                "Resistance"),
IO("r",                 RES_RESIST,         IF_PARSETREE|IF_RES|IF_REDUNDANT,
                "Resistance"),
IO("temp",              RES_TEMP,           IF_REAL|IF_TEMP,
                "Operating temperature"),
IO("w",                 RES_WIDTH,          IF_REAL|IF_LEN,
                "Resistor width"),
IO("l",                 RES_LENGTH,         IF_REAL|IF_LEN,
                "Resistor length"),
IO("tc1",               RES_TC1,            IF_REAL|IF_SETQUERY,
                "First order temp. coefficient"),
IO("tc",                RES_TC1,            IF_REAL|IF_SETQUERY|IF_REDUNDANT,
                "First order temp. coefficient"),
IO("tc2",               RES_TC2,            IF_REAL|IF_ORQUERY,
                "Second order temp. coefficient"),
IO("noise",             RES_NOISE,          IF_REAL,
                "Noise scaling coefficient"),
IO("poly",              RES_POLY,           IF_REALVEC,
                "Resistance polynomial"),
OP("conduct",           RES_CONDUCT,        IF_REAL|IF_COND,
                "Resistor conductance"),
OP("v",                 RES_VOLTAGE,        IF_REAL|IF_VOLT,
                "Resistor voltage"),
OP("i",                 RES_CURRENT,        IF_REAL|IF_AMP|IF_USEALL,
                "Resistor current"),
OP("c",                 RES_CURRENT,        IF_REAL|IF_AMP|IF_REDUNDANT,
                "Resistor current"),
OP("p",                 RES_POWER,          IF_REAL|IF_POWR,
                "Resistor power"),
OP("node1",             RES_POSNODE,        IF_INTEGER,
                "Node 1 number"),
OP("node2",             RES_NEGNODE,        IF_INTEGER,
                "Node 2 number"),
OP("expr",              RES_TREE,           IF_PARSETREE,
                "Resistance expression")
};

IFparm RESmPTable[] = {
IO("rsh",               RES_MOD_RSH,        IF_REAL|IF_SETQUERY,
                "Sheet resistance"),
IO("narrow",            RES_MOD_NARROW,     IF_REAL|IF_CHKQUERY,
                "Narrowing of resistor"),
IO("dw",                RES_MOD_NARROW,     IF_REAL|IF_CHKQUERY|IF_REDUNDANT,
                "Narrowing of resistor"),
IO("dl",                RES_MOD_DL,         IF_REAL|IF_CHKQUERY,
                "Shortening of resistor"),
IO("dlr",               RES_MOD_DL,         IF_REAL|IF_CHKQUERY|IF_REDUNDANT,
                "Shortening of resistor"),
IO("tc1",               RES_MOD_TC1,        IF_REAL|IF_SETQUERY,
                "First order temp. coefficient"),
IO("tc1r",              RES_MOD_TC1,        IF_REAL|IF_SETQUERY|IF_REDUNDANT,
                "First order temp. coefficient"),
IO("tc",                RES_MOD_TC1,        IF_REAL|IF_SETQUERY|IF_REDUNDANT,
                "First order temp. coefficient"),
IO("tc2",               RES_MOD_TC2,        IF_REAL|IF_ORQUERY,
                "Second order temp. coefficient"),
IO("tc2r",              RES_MOD_TC2,        IF_REAL|IF_ORQUERY|IF_REDUNDANT,
                "Second order temp. coefficient"),
IO("w",                 RES_MOD_DEFWIDTH,   IF_REAL|IF_NONSENSE,
                "Default device width"),
IO("defw",              RES_MOD_DEFWIDTH,   IF_REAL|IF_NONSENSE|IF_REDUNDANT,
                "Default device width"),
IO("l",                 RES_MOD_DEFLENGTH,  IF_REAL|IF_NONSENSE,
                "Default device length"),
IO("defl",              RES_MOD_DEFLENGTH,  IF_REAL|IF_NONSENSE|IF_REDUNDANT,
                "Default device length"),
IO("tnom",              RES_MOD_TNOM,       IF_REAL|IF_NONSENSE,
                "Parameter measurement temperature"),
IO("tref",              RES_MOD_TNOM,       IF_REAL|IF_NONSENSE|IF_REDUNDANT,
                "Parameter measurement temperature"),
IO("temp",              RES_MOD_TEMP,       IF_REAL|IF_NONSENSE,
                "Default operating temperature"),
IO("noise",             RES_MOD_NOISE,      IF_REAL|IF_NONSENSE,
                "Default noise scale factor"),
IO("kf",                RES_MOD_KF,         IF_REAL|IF_NONSENSE,
                "Flicker noise coefficient"),
IO("af",                RES_MOD_AF,         IF_REAL|IF_NONSENSE,
                "Exponent of current"),
IO("ef",                RES_MOD_EF,         IF_REAL|IF_NONSENSE,
                "Exponent of effective length"),
IO("wf",                RES_MOD_WF,         IF_REAL|IF_NONSENSE,
                "Exponent of effective width"),
IO("lf",                RES_MOD_LF,         IF_REAL|IF_NONSENSE,
                "Exponent of frequency"),
IP("r",                 RES_MOD_R,          IF_FLAG,
                "Device is a resistor model")
};

const char *RESnames[] = {
    "R+",
    "R-"
};

const char *RESmodNames[] = {
    "r",
    0
};

IFkeys RESkeys[] = {
    IFkeys( 'r', RESnames, 2, 2, 0 )
};

} // namespace


RESdev::RESdev()
{
    dv_name = "Resistor";
    dv_description = "Linear resistor model";

    dv_numKeys =NUMELEMS(RESkeys); 
    dv_keys = RESkeys;

    dv_levels[0] = 1;
    dv_levels[1] = 0;
    dv_modelKeys = RESmodNames;

    dv_numInstanceParms = NUMELEMS(RESpTable);
    dv_instanceParms = RESpTable;

    dv_numModelParms = NUMELEMS(RESmPTable);
    dv_modelParms = RESmPTable;

    dv_flags = 0;
};


sGENmodel *
RESdev::newModl()
{
    return (new sRESmodel);
}


sGENinstance *
RESdev::newInst()
{
    return (new sRESinstance);
}


int
RESdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sRESmodel, sRESinstance>(model));
}


int
RESdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sRESmodel, sRESinstance>(model, dname,
        fast));
}


int
RESdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sRESmodel, sRESinstance>(model, modname,
        modfast));
}


// resistor parser
// Rname <node> <node> [<model>] [<val>] [w=<val>] [l=<val>]
//
void
RESdev::parse(int type, sCKT *ckt, sLine *current)
{
    DEV.parse(ckt, current, type, dv_keys->minTerms, dv_keys->maxTerms,
        true, "resistance");
}


// Setup the tran parameters for any tran function nodes.
//
void
RESdev::initTran(sGENmodel *modp, double step, double finaltime)
{
    for (sRESmodel *model = (sRESmodel*)modp; model; model = model->next()) {
        for (sRESinstance *inst = (sRESinstance*)model->GENinstances;
                inst; inst = inst->next()) {
            if (inst->REStree)
                inst->REStree->initTran(step, finaltime);
        }
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
        *d = new RESdev;
        (*cnt)++;
    }
}

