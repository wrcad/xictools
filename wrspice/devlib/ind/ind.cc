
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

#include "inddefs.h"


namespace {

IFparm INDpTable[] = {
IO("inductance",    IND_IND,        IF_PARSETREE|IF_IND|IF_AC|IF_PRINCIPAL,
                "Inductance"),
IO("ind",       IND_IND,  IF_PARSETREE|IF_IND|IF_AC|IF_PRINCIPAL|IF_REDUNDANT,
                "Inductance"),
IO("l",         IND_IND,  IF_PARSETREE|IF_IND|IF_AC|IF_PRINCIPAL|IF_REDUNDANT,
                "Inductance"),
IO("ic",            IND_IC,             IF_REAL|IF_AMP|IF_AC,
                "Initial condition (inductor current"),
IO("m",             IND_M,               IF_REAL,
                "Parallel multiplier"),
IO("poly",          IND_POLY,           IF_REALVEC,
                "Inductance polynomial"),
OP("flux",          IND_FLUX,           IF_REAL|IF_FLUX,
                "Inductor flux"),
OP("v",             IND_VOLT,           IF_REAL|IF_VOLT,
                "Inductor voltage"),
OP("i",             IND_CURRENT,        IF_REAL|IF_AMP,
                "Inductor current"),
OP("c",             IND_CURRENT,        IF_REAL|IF_AMP|IF_REDUNDANT,
                "Inductor current"),
OP("p",             IND_POWER,          IF_REAL|IF_POWR,
                "Instantaneous stored power"),
OP("node1",         IND_POSNODE,        IF_INTEGER,
                "Node 1 number"),
OP("node2",         IND_NEGNODE,        IF_INTEGER,
                "Node 2 number"),
OP("expr",          IND_TREE,           IF_PARSETREE,
                "Inductance expression")
};

IFparm INDmPTable[] = {
IP("l",             IND_MOD_L,          IF_FLAG,
                "Inductor model"),
IO("m",             IND_MOD_M,          IF_REAL,
                "Default parallel multiplier")
};

const char *INDnames[] = {
    "L+",
    "L-"
};

const char *INDmodNames[] = {
    "l",
    0
};

IFkeys INDkeys[] = {
    IFkeys( 'l', INDnames, 2, 2, 0 )
};

} // namespace


INDdev::INDdev()
{
    dv_name = "Inductor";
    dv_description = "Inductors";

    dv_numKeys = NUMELEMS(INDkeys);
    dv_keys = INDkeys;

    dv_levels[0] = 1;
    dv_levels[1] = 0;
    dv_modelKeys = INDmodNames;

    dv_numInstanceParms = NUMELEMS(INDpTable);
    dv_instanceParms = INDpTable;

    dv_numModelParms = NUMELEMS(INDmPTable);;
    dv_modelParms = INDmPTable;;

    dv_flags = DV_TRUNC;
};


sGENmodel *
INDdev::newModl()
{
    return (new sINDmodel);
}


sGENinstance *
INDdev::newInst()
{
    return (new sINDinstance);
}


int
INDdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sINDmodel, sINDinstance>(model));
}


int
INDdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sINDmodel, sINDinstance>(model, dname,
        fast));
}


int
INDdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sINDmodel, sINDinstance>(model, modname,
        modfast));
}


// inductor parser
// Lname <node> <node> <val> [IC=<val>]
//
void
INDdev::parse(int type, sCKT *ckt, sLine *current)
{
    DEV.parse(ckt, current, type, dv_keys->minTerms, dv_keys->maxTerms,
        true, "inductance");
}


namespace {

IFparm MUTpTable[] = {
IO("k",                 MUT_COEFF,          IF_REAL|IF_AC|IF_PRINCIPAL,
                "Coupling coefficient"),
IO("coeff",             MUT_COEFF,          IF_REAL|IF_REDUNDANT,
                "Coupling coefficient"),
IO("coefficient",       MUT_COEFF,          IF_REAL|IF_REDUNDANT,
                "Coupling coefficient"),
IO("factor",            MUT_FACTOR,         IF_REAL|IF_AC|IF_PRINCIPAL,
                "Coupling factor (k*sqrt(L1*L2))"),
IO("inductor1",         MUT_IND1,           IF_INSTANCE,
                "First coupled inductor"),
IO("inductor2",         MUT_IND2,           IF_INSTANCE,
                "Second coupled inductor")
};

// no model parameters

IFkeys MUTkeys[] = {
    IFkeys( 'k', 0, 0, 0, 2 )
};

} // namespace


MUTdev::MUTdev()
{
    dv_name = "Mutual";
    dv_description = "Mutual inductors";

    dv_numKeys = NUMELEMS(MUTkeys);
    dv_keys = MUTkeys;

    dv_levels[0] = 0;
    dv_modelKeys = 0;

    dv_numInstanceParms = NUMELEMS(MUTpTable);
    dv_instanceParms = MUTpTable;

    dv_numModelParms = 0;
    dv_modelParms = 0;

    dv_flags = 0;
};


sGENmodel *
MUTdev::newModl()
{
    return (new sMUTmodel);
}


sGENinstance *
MUTdev::newInst()
{
    return (new sMUTinstance);
}


int
MUTdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sMUTmodel, sMUTinstance>(model));
}


int
MUTdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sMUTmodel, sMUTinstance>(model, dname,
        fast));
}


int
MUTdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sMUTmodel, sMUTinstance>(model, modname,
        modfast));
}


// Setup the tran parameters for any tran function nodes.
//
void
INDdev::initTranFuncs(sGENmodel *modp, double step, double finaltime)
{
    for (sINDmodel *model = (sINDmodel*)modp; model; model = model->next()) {
        for (sINDinstance *inst = (sINDinstance*)model->GENinstances;
                inst; inst = inst->next()) {
            if (inst->INDtree)
                inst->INDtree->initTranFuncs(step, finaltime);
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
        *d = new INDdev;
        (*cnt)++;
        *(d+1) = new MUTdev;
        (*cnt)++;
    }
}

