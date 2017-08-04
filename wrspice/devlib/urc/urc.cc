
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

#include "urcdefs.h"


namespace {

IFparm URCpTable[] = {
IO("l",                 URC_LEN,            IF_REAL|IF_LEN,
                "Length of transmission line"),
IO("n",                 URC_LUMPS,          IF_REAL,
                "Number of lumps"),
OP("pos_node",          URC_POS_NODE,       IF_INTEGER,
                "Positive node of URC"),
OP("neg_node",          URC_NEG_NODE,       IF_INTEGER,
                "Negative node of URC"),
OP("gnd",               URC_GND_NODE,       IF_INTEGER,
                "Ground node of URC")
};

IFparm URCmPTable[] = {
IO("k",                 URC_MOD_K,          IF_REAL,
                "Propagation constant"),
IO("fmax",              URC_MOD_FMAX,       IF_REAL|IF_AC,
                "Maximum frequency of interest"),
IO("rperl",             URC_MOD_RPERL,      IF_REAL,
                "Resistance per unit length"),
IO("cperl",             URC_MOD_CPERL,      IF_REAL|IF_AC,
                "Capacitance per unit length"),
IO("isperl",            URC_MOD_ISPERL,     IF_REAL,
                "Saturation current per length"),
IO("rsperl",            URC_MOD_RSPERL,     IF_REAL,
                "Diode resistance per length"),
IP("urc",               URC_MOD_URC,        IF_FLAG,
                "Uniform R.C. line model")
};

const char *URCnames[] = {
    "P1",
    "P2",
    "Ref"
};

const char *URCmodNames[] = {
    "urc",
    0
};

IFkeys URCkeys[] = {
    IFkeys( 'u', URCnames, 2, 2, 0 )
};

} // namespace


URCdev::URCdev()
{
    dv_name = "URC";
    dv_description = "Uniform R.C. line model (lumped)";

    dv_numKeys = NUMELEMS(URCkeys);
    dv_keys = URCkeys;

    dv_levels[0] = 1;
    dv_levels[1] = 0;
    dv_modelKeys = URCmodNames;

    dv_numInstanceParms = NUMELEMS(URCpTable);
    dv_instanceParms = URCpTable;

    dv_numModelParms = NUMELEMS(URCmPTable);
    dv_modelParms = URCmPTable;

    dv_flags = DV_TRUNC;
};


sGENmodel *
URCdev::newModl()
{
    return (new sURCmodel);
}


sGENinstance *
URCdev::newInst()
{
    return (new sURCinstance);
}


int
URCdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sURCmodel, sURCinstance>(model));
}


int
URCdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sURCmodel, sURCinstance>(model, dname,
        fast));
}


int
URCdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sURCmodel, sURCinstance>(model, modname,
        modfast));
}


// lossy line (lumped approx) parser
// Uname <node> <node> <model> [l=<val>] [n=<val>]
//
void
URCdev::parse(int type, sCKT *ckt, sLine *current)
{
    DEV.parse(ckt, current, type, dv_keys->minTerms, dv_keys->maxTerms,
        true, 0);
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
        *d = new URCdev;
        (*cnt)++;
    }
}

