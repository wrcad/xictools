
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

#include "capdefs.h"

namespace {

IFparm CAPpTable[] = {
IO("capacitance",       CAP_CAP,            IF_PARSETREE|IF_CAP|IF_AC|IF_PRINCIPAL,
                "Device capacitance"),
IO("cap",               CAP_CAP,            IF_PARSETREE|IF_CAP|IF_AC|IF_PRINCIPAL|IF_REDUNDANT,
                "Device capacitance"),
IO("c",                 CAP_CAP,            IF_PARSETREE|IF_CAP|IF_AC|IF_PRINCIPAL|IF_REDUNDANT,
                "Device capacitance"),
IO("ic",                CAP_IC,             IF_REAL|IF_VOLT|IF_AC,
                "Initial condition (capacitor voltage"),
IO("w",                 CAP_WIDTH,          IF_REAL|IF_LEN|IF_AC,
                "Capacitor width"),
IO("l",                 CAP_LENGTH,         IF_REAL|IF_LEN|IF_AC,
                "Capacitor length"),
IO("temp",              CAP_TEMP,           IF_REAL|IF_TEMP,
                "Capacitor temperature"),
IO("tc1",               CAP_TC1,            IF_REAL|IF_SETQUERY,
                "First order temp coeff."),
IO("tc2",               CAP_TC2,            IF_REAL|IF_ORQUERY,
                "Second order temp coeff"),
IO("poly",              CAP_POLY,           IF_REALVEC,
                "Capacitance polynomial"),
OP("charge",            CAP_CHARGE,         IF_REAL|IF_CHARGE,
                "Capacitor charge"),
OP("v",                 CAP_VOLTAGE,        IF_REAL|IF_VOLT,
                "Capacitor voltage"),
OP("i",                 CAP_CURRENT,        IF_REAL|IF_AMP|IF_USEALL,
                "Capacitor current"),
OP("p",                 CAP_POWER,          IF_REAL|IF_POWR,
                "Instantaneous stored power"),
OP("node1",             CAP_POSNODE,        IF_INTEGER,
                "Node 1 number"),
OP("node2",             CAP_NEGNODE,        IF_INTEGER,
                "Node 2 number"),
OP("expr",              CAP_TREE,           IF_PARSETREE,
                "Capacitance expression")
};

IFparm CAPmPTable[] = {
IO("cj",                CAP_MOD_CJ,         IF_REAL|IF_AC,
                "Bottom Capacitance per area"),
IO("cjsw",              CAP_MOD_CJSW,       IF_REAL|IF_AC,
                "Sidewall capacitance per meter"),
IO("defw",              CAP_MOD_DEFWIDTH,   IF_REAL|IF_NONSENSE,
                "Default width"),
IP("c",                 CAP_MOD_C,          IF_FLAG,
                "Capacitor model"),
IO("narrow",            CAP_MOD_NARROW,     IF_REAL|IF_AC,
                "Width correction factor"),
IO("tnom",              CAP_MOD_TNOM,       IF_REAL|IF_NONSENSE,
                "Parameter measurement temperature"),
IO("tc1",               CAP_MOD_TC1,        IF_REAL|IF_SETQUERY,
                "First order temp. coefficient"),
IO("tc2",               CAP_MOD_TC2,        IF_REAL|IF_ORQUERY,
                "Second order temp. coefficient")
};

const char *CAPnames[] = {
    "C+",
    "C-"
};

const char *CAPmodNames[] = {
    "c",
    0
};

IFkeys CAPkeys[] = {
    IFkeys( 'c', CAPnames, 2, 2, 0 )
};

} // namespace


CAPdev::CAPdev()
{
    dv_name = "Capacitor";
    dv_description = "Fixed capacitor model";

    dv_numKeys = NUMELEMS(CAPkeys);
    dv_keys = CAPkeys,

    dv_levels[0] = 1;
    dv_levels[1] = 0;
    dv_modelKeys = CAPmodNames;

    dv_numInstanceParms = NUMELEMS(CAPpTable);
    dv_instanceParms = CAPpTable;

    dv_numModelParms = NUMELEMS(CAPmPTable);
    dv_modelParms = CAPmPTable;

    dv_flags = DV_TRUNC;
};


sGENmodel *
CAPdev::newModl()
{
    return (new sCAPmodel);
}


sGENinstance *
CAPdev::newInst()
{
    return (new sCAPinstance);
}


int
CAPdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sCAPmodel, sCAPinstance>(model));
}


int
CAPdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sCAPmodel, sCAPinstance>(model, dname,
        fast));
}


int
CAPdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sCAPmodel, sCAPinstance>(model, modname,
        modfast));
}


// capacitor parser
// Cname <node> <node> [<model>] [<val>] [IC=<val>]
//
void
CAPdev::parse(int type, sCKT *ckt, sLine *current)
{
    DEV.parse(ckt, current, type, dv_keys->minTerms, dv_keys->maxTerms,
        true, "capacitance");
}


// Setup the tran parameters for any tran function nodes.
//
void
CAPdev::initTranFuncs(sGENmodel *modp, double step, double finaltime)
{
    for (sCAPmodel *model = (sCAPmodel*)modp; model; model = model->next()) {
        for (sCAPinstance *inst = (sCAPinstance*)model->GENinstances;
                inst; inst = inst->next()) {
            if (inst->CAPtree)
                inst->CAPtree->initTranFuncs(step, finaltime);
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
        *d = new CAPdev;
        (*cnt)++;
    }
}

