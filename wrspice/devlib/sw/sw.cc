
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
 $Id: sw.cc,v 2.22 2016/09/26 01:48:29 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Gordon M. Jacobs
         1993 Stephen R. Whiteley
****************************************************************************/

#include "swdefs.h"


namespace {

IFparm SWpTable[] = {
IP("on",                SW_IC_ON,           IF_FLAG,
                "Switch initially closed"),
IP("off",               SW_IC_OFF,          IF_FLAG,
                "Switch initially open"),
OP("ic",                SW_IC,              IF_FLAG,
                "Switch initial state"),
IO("control",           SW_CONTROL,         IF_INSTANCE,
                "Name of controlling source"),
OP("v",                 SW_VOLTAGE,         IF_REAL|IF_VOLT,
                "Switch voltage"),
OP("i",                 SW_CURRENT,         IF_REAL|IF_AMP|IF_USEALL,
                "Switch current"),
OP("p",                 SW_POWER,           IF_REAL|IF_POWR,
                "Switch power"),
OP("node1",             SW_POS_NODE,        IF_INTEGER,
                "Node 1 number"),
OP("node2",             SW_NEG_NODE,        IF_INTEGER,
                "Node 2 number"),
OP("cont_node1",        SW_POS_CONT_NODE,   IF_INTEGER,
                "Control node 1 number"),
OP("cont_node2",        SW_NEG_CONT_NODE,   IF_INTEGER,
                "Control node 2 number")
};

IFparm SWmPTable[] = {
IP("sw",                SW_MOD_SW,          IF_FLAG,
                "Switch model"),
IP("csw",               SW_MOD_CSW,         IF_FLAG,
                "Switch model"),
IO("vt",                SW_MOD_VTH,         IF_REAL,
                "Threshold voltage"),
IO("vh",                SW_MOD_VHYS,        IF_REAL,
                "Hysteresis voltage"),
IO("it",                SW_MOD_ITH,         IF_REAL,
                "Threshold current"),
IO("ih",                SW_MOD_IHYS,        IF_REAL,
                "Hysterisis current"),
IO("ron",               SW_MOD_RON,         IF_REAL,
                "Resistance when closed"),
IO("roff",              SW_MOD_ROFF,        IF_REAL,
                "Resistance when open"),
OP("gon",               SW_MOD_GON,         IF_REAL,
                "Conductance when closed"),
OP("goff",              SW_MOD_GOFF,        IF_REAL,
                "Conductance when open")
};

const char *SWnames1[] = {
    "S+",
    "S-",
    "SC+",
    "SC-"
};

const char *SWnames2[] = {
    "S+",
    "S-"
};

const char *SWmodNames[] = {
    "sw",
    "csw",
    0
};

IFkeys SWkeys[] = {
    IFkeys( 's', SWnames1, 4, 4, 0 ),
    IFkeys( 'w', SWnames2, 2, 2, 1 )
};

} // namespace


SWdev::SWdev()
{
    dv_name = "Switch";
    dv_description = "Ideal switch model";

    dv_numKeys = NUMELEMS(SWkeys);
    dv_keys = SWkeys;

    dv_levels[0] = 1;
    dv_levels[1] = 0;
    dv_modelKeys = SWmodNames;

    dv_numInstanceParms = NUMELEMS(SWpTable);
    dv_instanceParms = SWpTable;

    dv_numModelParms = NUMELEMS(SWmPTable);
    dv_modelParms = SWmPTable;

    dv_flags = 0;
};


sGENmodel *
SWdev::newModl()
{
    return (new sSWmodel);
}


sGENinstance *
SWdev::newInst()
{
    return (new sSWinstance);
}


int
SWdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sSWmodel, sSWinstance>(model));
}


int
SWdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sSWmodel, sSWinstance>(model, dname,
        fast));
}


int
SWdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sSWmodel, sSWinstance>(model, modname,
        modfast));
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
        *d = new SWdev;
        (*cnt)++;
    }
}

