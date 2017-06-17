
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
 $Id: tra.cc,v 2.24 2016/09/26 01:48:31 stevew Exp $
 *========================================================================*/

//-------------------------------------------------------------------------
// This is a general transmission line model derived from:
//  1) the spice3 TRA (lossless) model
//  2) the spice3 LTRA (lossy, convolution) model
//  3) the kspice TXL (lossy, Pade approximation convolution) model
// Authors:
//  1985 Thomas L. Quarles
//  1990 Jaijeet S. Roychowdhury
//  1990 Shen Lin
//  1992 Charles Hough
//  2002 Stephen R. Whiteley
// Copyright Regents of the University of California.  All rights reserved.
//-------------------------------------------------------------------------

#include "tradefs.h"


namespace {

IFparm TRApTable[] = {
IO("level",             TRA_LEVEL,          IF_INTEGER,
                "Algorithm"),
IO("len",               TRA_LENGTH,         IF_REAL,
                "Length, arbitrary units"),
IO("length",            TRA_LENGTH,         IF_REAL,
                "Length, arbitrary units"),
IO("l",                 TRA_L,              IF_REAL|IF_IND,
                "Inductance per length"),
IO("c",                 TRA_C,              IF_REAL|IF_CAP,
                "Capacitance per length"),
IO("r",                 TRA_R,              IF_REAL|IF_RES,
                "Resistance per length"),
IO("g",                 TRA_G,              IF_REAL|IF_COND,
                "Conductance per length"),
IO("z0",                TRA_Z0,             IF_REAL|IF_RES,
                "Characteristic impedance"),
IO("zo",                TRA_Z0,             IF_REAL|IF_RES|IF_REDUNDANT,
                "Characteristic impedance"),
IO("td",                TRA_TD,             IF_REAL|IF_TIME|IF_AC,
                "Transmission time delay"),
IO("delay",             TRA_TD,             IF_REAL|IF_REDUNDANT,
                "Transmission time delay"),
IO("f",                 TRA_FREQ,           IF_REAL|IF_FREQ|IF_AC,
                "Frequency"),
IO("nl",                TRA_NL,             IF_REAL|IF_LEN|IF_AC,
                "Normalized length at frequency given"),
IO("lininterp",         TRA_LININTERP,      IF_FLAG,
                "Use linear interpolation"),
IO("quadinterp",        TRA_QUADINTERP,     IF_FLAG,
                "Use quadratic interpolation"),
IO("truncdontcut",      TRA_TRUNCDONTCUT,   IF_FLAG,
                "Limit timestep to td only"),
IO("truncsl",           TRA_TRUNCCUTSL,     IF_FLAG,
                "Use slope approx. to limit timestep"),
IO("trunclte",          TRA_TRUNCCUTLTE,    IF_FLAG,
                "Use truncation error to limit timestep"),
IO("truncnr",           TRA_TRUNCCUTNR,     IF_FLAG,
                "Use N-R iterations to limit timestep"),
IO("nobreaks",          TRA_NOBREAKS,       IF_FLAG,
                "No breakpoint rescheduling"),
IO("allbreaks",         TRA_ALLBREAKS,      IF_FLAG,
                "Reschedule all breakpoints"),
IO("testbreaks",        TRA_TESTBREAKS,     IF_FLAG,
                "Test breakpoints for rescheduling"),
IO("slopetol",          TRA_SLOPETOL,       IF_REAL,
                "Slope timestep relative tol."),
IO("compactrel",        TRA_COMPACTREL,     IF_REAL,
                "Compaction relative tol."),
IO("compactabs",        TRA_COMPACTABS,     IF_REAL,
                "Compaction absolute tol."),
IP("rel",               TRA_RELTOL,         IF_REAL,
                "Not used"),
IP("abs",               TRA_ABSTOL,         IF_REAL,
                "Not used"),
IO("v1",                TRA_V1,             IF_REAL|IF_VOLT|IF_AC,
                "Initial voltage at end 1"),
IO("i1",                TRA_I1,             IF_REAL|IF_AMP|IF_AC,
                "Initial current at end 1"),
IO("v2",                TRA_V2,             IF_REAL|IF_VOLT|IF_AC,
                "Initial voltage at end 2"),
IO("i2",                TRA_I2,             IF_REAL|IF_AMP|IF_AC,
                "Initial current at end 2"),
IP("ic",                TRA_IC,             IF_REALVEC,
                "Initial condition vector:v1,i1,v2,i2"),
OP("v_1",               TRA_QUERY_V1,       IF_REAL|IF_VOLT,
                "Voltage at end 1"),
OP("i_1",               TRA_QUERY_I1,       IF_REAL|IF_AMP,
                "Current at end 1"),
OP("v_2",               TRA_QUERY_V2,       IF_REAL|IF_VOLT,
                "Voltage at end 2"),
OP("i_2",               TRA_QUERY_I2,       IF_REAL|IF_AMP,
                "Current at end 2"),
OP("node1",             TRA_POS_NODE1,      IF_INTEGER,
                "Node 1 number"),
OP("node2",             TRA_NEG_NODE1,      IF_INTEGER,
                "Node 2 number"),
OP("node3",             TRA_POS_NODE2,      IF_INTEGER,
                "Node 3 number"),
OP("node4",             TRA_NEG_NODE2,      IF_INTEGER,
                "Node 4 number"),
OP("branch1",           TRA_BR_EQ1,         IF_INTEGER,
                "Branch 1 number"),
OP("branch2",           TRA_BR_EQ2,         IF_INTEGER,
                "Branch 2 number"),
// OP("delays",           TRA_DELAY,          IF_REALVEC,
//                        "Delayed values of excitation"),
OP("maxstep",           TRA_MAXSTEP,        IF_REAL,
                "Maximum allowed time step")
};

IFparm TRAmPTable[] = {
IP("tra",               TRA_MOD_TRA,        IF_FLAG,
                "TRA model"),
IO("level",             TRA_MOD_LEVEL,      IF_INTEGER,
                "Model level"),
IO("len",               TRA_MOD_LEN,        IF_REAL,
                "Length of line"),
IO("length",            TRA_MOD_LEN,        IF_REAL,
                "Length of line"),
IO("l",                 TRA_MOD_L,          IF_REAL|IF_AC,
                "Inductance per length"),
IO("c",                 TRA_MOD_C,          IF_REAL|IF_AC,
                "Capacitance per length"),
IO("r",                 TRA_MOD_R,          IF_REAL,
                "Resistance per length"),
IO("g",                 TRA_MOD_G,          IF_REAL,
                "Conductance per length"),
IO("z0",                TRA_MOD_Z0,         IF_REAL,
                "Transmission line impedance"),
IO("zo",                TRA_MOD_Z0,         IF_REAL|IF_REDUNDANT,
                "Transmission line impedance"),
IO("delay",             TRA_MOD_TD,         IF_REAL,
                "Transmission line delay"),
IO("td",                TRA_MOD_TD,         IF_REAL|IF_REDUNDANT,
                "Transmission line delay"),
IO("f",                 TRA_MOD_F,          IF_REAL,
                "Frequency"),
IO("nl",                TRA_MOD_NL,         IF_REAL,
                "Normalized length at frequency given"),
IO("lininterp",         TRA_MOD_LININTERP,  IF_FLAG,
                "Use linear interpolation"),
IO("quadinterp",        TRA_MOD_QUADINTERP, IF_FLAG,
                "Use quadratic interpolation"),
IO("truncdontcut",      TRA_MOD_TRUNCDONTCUT,IF_FLAG,
                "Limit timestep to td only"),
IO("truncsl",           TRA_MOD_TRUNCCUTSL, IF_FLAG,
                "Use slope approx. to limit timestep"),
IO("trunclte",          TRA_MOD_TRUNCCUTLTE,IF_FLAG,
                "Use truncation error to limit timestep"),
IO("truncnr",           TRA_MOD_TRUNCCUTNR, IF_FLAG,
                "Use N-R iterations to limit timestep"),
IO("nobreaks",          TRA_MOD_NOBREAKS,   IF_FLAG,
                "No breakpoint rescheduling"),
IO("allbreaks",         TRA_MOD_ALLBREAKS,  IF_FLAG,
                "Reschedule all breakpoints"),
IO("testbreaks",        TRA_MOD_TESTBREAKS, IF_FLAG,
                "Test breakpoints for rescheduling"),
IO("slopetol",          TRA_MOD_SLOPETOL,   IF_REAL,
                "Slope timestep relative tol."),
IO("compactrel",        TRA_MOD_COMPACTREL, IF_REAL,
                "Compaction relative tol."),
IO("compactabs",        TRA_MOD_COMPACTABS, IF_REAL,
                "Compaction absolute tol."),
IO("rel",               TRA_MOD_RELTOL,     IF_REAL,
                "Rel. rate of change of deriv. for bkpt"),
IO("abs",               TRA_MOD_ABSTOL,     IF_REAL,
                "Abs. rate of change of deriv. for bkpt"),
IP("ltra",              TRA_MOD_LTRA,       IF_FLAG,
                "LTRA model")
};

const char *TRAnames[] = {
    "P1+",
    "P1-",
    "P2+",
    "P2-"
};

const char *TRAmodNames[] = {
    "tra",
    "ltra",
    0
};

IFkeys TRAkeys[] = {
    IFkeys( 't', TRAnames, 4, 4, 0 ),
    IFkeys( 'o', TRAnames, 4, 4, 0 )
};

} // namespace


TRAdev::TRAdev()
{
    dv_name = "tra";
    dv_description = "General transmission line model";

    dv_numKeys = NUMELEMS(TRAkeys);
    dv_keys = TRAkeys;

    dv_levels[0] = PADE_LEVEL;
    dv_levels[1] = CONV_LEVEL;
    dv_levels[2] = 0;
    dv_modelKeys = TRAmodNames;

    dv_numInstanceParms = NUMELEMS(TRApTable);
    dv_instanceParms = TRApTable;

    dv_numModelParms = NUMELEMS(TRAmPTable);
    dv_modelParms = TRAmPTable;

    dv_flags = DV_NOLEVCHG;
};


sGENmodel *
TRAdev::newModl()
{
    return (new sTRAmodel);
}


sGENinstance *
TRAdev::newInst()
{
    return (new sTRAinstance);
}


int
TRAdev::destroy(sGENmodel **model)
{
    return (IFdevice::destroy<sTRAmodel, sTRAinstance>(model));
}


int
TRAdev::delInst(sGENmodel *model, IFuid dname, sGENinstance *fast)
{
    return (IFdevice::delInst<sTRAmodel, sTRAinstance>(model, dname,
        fast));
}


int
TRAdev::delModl(sGENmodel **model, IFuid modname, sGENmodel *modfast)
{
    return (IFdevice::delModl<sTRAmodel, sTRAinstance>(model, modname,
        modfast));
}


// general transmission line parser
//   Tname <node> <node> <node> <node> [model] [param=value ...]
//
// parameters:
//  level           integer algorithm: 1= Pade approx., 2= full convolution
//  len             real    line length, arb. units
//  l               real    inductance per length
//  c               real    capacitance per length
//  r               real    resistance per length
//  g               real    conductance per length
//  z0, zo          real    sqrt(l/c)
//  td, delay       real    length/sqrt(l*c)
//  f               real    freqency
//  nl              real    length at frequency
//  lininterp       flag    interpolation method (1 of 2)
//  quadinterp      flag    interpolation method (2 of 2)
//  truncdontcut    flag    timestep cutting method (1 of 4)
//  truncsl         flag    timestep cutting method (2 of 4)
//  trunclte        flag    timestep cutting method (3 of 4)
//  truncnr         flag    timestep cutting method (4 of 4)
//  nobreaks        flag    breakpoint setting method (1 of 3)
//  allbreaks       flag    breakpoint setting method (2 of 3)
//  testbreaks      flag    breakpoint setting method (3 of 3)
//  slopetol        real    truncsl slope tolerance
//  compactrel      real    compaction reltol
//  compactabs      real    compaction absltol
//  rel             real    breakpoint reltol
//  abs             real    breakpoint abstol
//
void
TRAdev::parse(int type, sCKT *ckt, sLine *current)
{
    DEV.parse(ckt, current, type, dv_keys->minTerms, dv_keys->maxTerms,
        true, "length");
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
        *d = new TRAdev;
        (*cnt)++;
    }
}

