
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
         1987 Kanwar Jit Singh
         1993 Stephen R. Whiteley
****************************************************************************/

#include "srcdefs.h"
#include "input.h"
#include <string.h>


namespace {

IFparm SRCpTable[] = {
IP("i",                 SRC_I,              IF_PARSETREE,
                "Current source"),
IP("v",                 SRC_V,              IF_PARSETREE,
                "Voltage source"),
IO("type",              SRC_DEP,            IF_INTEGER,
                "Type of dependency"),
IO("dc",                SRC_DC,             IF_REAL|IF_PRINCIPAL,
                "D.C. source value"),
IO("ac",                SRC_AC,             IF_TABLEVEC,
                "AC magnitude, phase vector or table"),
IO("acmag",             SRC_AC_MAG,         IF_REAL|IF_PRINCIPAL|IF_AC_ONLY,
                "A.C. Magnitude"),
IO("acphase",           SRC_AC_PHASE,       IF_REAL|IF_AC_ONLY,
                "A.C. Phase"),
OP("acreal",            SRC_AC_REAL,        IF_REAL,
                "AC real part"),
OP("acimag",            SRC_AC_IMAG,        IF_REAL,
                "AC imaginary part"),
IO("function",          SRC_FUNC,           IF_PARSETREE,
                "Function specification"),
IO("cur",               SRC_FUNC,           IF_PARSETREE,
                "Function specification (current"),
IO("vol",               SRC_FUNC,           IF_PARSETREE,
                "Function specification (voltage"),
IO("distof1",           SRC_D_F1,           IF_REALVEC,
                "Freq. f1 for distortion"),
IO("distof2",           SRC_D_F2,           IF_REALVEC,
                "Freq. f2 for distortion"),
IO("gain",              SRC_GAIN,           IF_REAL,
                "Transfer gain of source"),
IO("control",           SRC_CONTROL,        IF_INSTANCE,
                "Name of controlling source"),
IO("prm1",              SRC_PRM1,           IF_REAL,
                "Function parameter 1"),
IO("prm2",              SRC_PRM2,           IF_REAL,
                "Function parameter 2"),
IO("prm3",              SRC_PRM3,           IF_REAL,
                "Function parameter 3"),
IO("prm4",              SRC_PRM4,           IF_REAL,
                "Function parameter 4"),
IO("prm5",              SRC_PRM5,           IF_REAL,
                "Function parameter 5"),
IO("prm6",              SRC_PRM6,           IF_REAL,
                "Function parameter 6"),
IO("prm7",              SRC_PRM7,           IF_REAL,
                "Function parameter 7"),
IO("prm8",              SRC_PRM8,           IF_REAL,
                "Function parameter 8"),
OP("vs",                SRC_VOLTAGE,        IF_REAL|IF_VOLT,
                "Voltage of source"),
OP("c",                 SRC_CURRENT,        IF_REAL|IF_AMP|IF_USEALL,
                "Current through source"),
OP("p",                 SRC_POWER,          IF_REAL|IF_POWR,
                "Instantaneous power"),
OP("pos_node",          SRC_POS_NODE,       IF_INTEGER,
                "Node 1 number"),
OP("neg_node",          SRC_NEG_NODE,       IF_INTEGER,
                "Node 2 number"),
OP("cont_p_node",       SRC_CONT_P_NODE,    IF_INTEGER,
                "Control node 1 number"),
OP("cont_n_node",       SRC_CONT_N_NODE,    IF_INTEGER,
                "Control node 2 number"),
OP("branch",            SRC_BR_NODE,        IF_INTEGER,
                "Voltage source branch equation number")
};

// no model parameters

const char *SRCnames1[] = {
    "src+",
    "src-",
};

const char *SRCnames2[] = {
    "src+",
    "src-",
    "srcC+",
    "srcC-"
};

// The sources require special node handling due to the function and poly
// keywords, which terminate node lists of e/f/g/h devices.  The exported
// function below takes care of this
//
IFkeys SRCkeys[] = {
    IFkeys( 'a', SRCnames1, 2, 2, 0 ),
    IFkeys( 'v', SRCnames1, 2, 2, 0 ),
    IFkeys( 'i', SRCnames1, 2, 2, 0 ),
    IFkeys( 'e', SRCnames2, 4, 4, 0 ),
    IFkeys( 'f', SRCnames1, 2, 2, 1 ),
    IFkeys( 'g', SRCnames2, 4, 4, 0 ),
    IFkeys( 'h', SRCnames1, 2, 2, 1 )
};

} // namespace


SRCdev::SRCdev()
{
    dv_name = "Source";
    dv_description = "General source model ";

    dv_numKeys = NUMELEMS(SRCkeys);
    dv_keys = SRCkeys;

    dv_levels[0] = 0;
    dv_modelKeys = 0;

    dv_numInstanceParms = NUMELEMS(SRCpTable);
    dv_instanceParms = SRCpTable;

    dv_numModelParms = 0;
    dv_modelParms = 0;

    dv_flags = 0;
};


sGENmodel *
SRCdev::newModl()
{
    return (new sSRCmodel);
}


sGENinstance *
SRCdev::newInst()
{
    return (new sSRCinstance);
}


int
SRCdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sSRCmodel, sSRCinstance>(model));
}


int
SRCdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sSRCmodel, sSRCinstance>(model, dname, fast));
}


int
SRCdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sSRCmodel, sSRCinstance>(model, modname,
        modfast));
}


// Setup the tran parameters for any tran function nodes.
//
void
SRCdev::initTranFuncs(sGENmodel *modp, double step, double finaltime)
{
    for (sSRCmodel *model = (sSRCmodel*)modp; model; model = model->next()) {
        for (sSRCinstance *inst = (sSRCinstance*)model->GENinstances;
                inst; inst = inst->next()) {
            if (inst->SRCtree)
                inst->SRCtree->initTranFuncs(step, finaltime);
        }
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
        *d = new SRCdev;
        (*cnt)++;
    }
}

