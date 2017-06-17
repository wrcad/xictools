
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: funcs_drc.cc,v 5.62 2017/03/19 01:58:35 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "drc.h"
#include "editif.h"
#include "python_if.h"
#include "tcltk_if.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "geo_zlist.h"
#include "si_parsenode.h"
#include "si_handle.h"
#include "si_args.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_lexpr.h"
#include "cd_digest.h"
#include "layertab.h"
#include "menu.h"
#include "drc_menu.h"
#include "tech_layer.h"
#include "promptline.h"
#include "select.h"
#include "filestat.h"


////////////////////////////////////////////////////////////////////////
//
// Script Functions:  Design Rule Checking
//
////////////////////////////////////////////////////////////////////////

namespace {
    namespace drc_funcs {
        // Design Rule Checking
        bool IFdrcUserTest(Variable*, Variable*, void*);
        bool IFdrcUserEmpty(Variable*, Variable*, void*);
        bool IFdrcUserFull(Variable*, Variable*, void*);
        bool IFdrcUserZlist(Variable*, Variable*, void*);
        bool IFdrcUserEdgeLength(Variable*, Variable*, void*);
        bool IFdrcState(Variable*, Variable*, void*);
        bool IFdrcSetLimits(Variable*, Variable*, void*);
        bool IFdrcGetLimits(Variable*, Variable*, void*);
        bool IFdrcSetMaxErrors(Variable*, Variable*, void*);
        bool IFdrcGetMaxErrors(Variable*, Variable*, void*);
        bool IFdrcSetInterMaxObjs(Variable*, Variable*, void*);
        bool IFdrcGetInterMaxObjs(Variable*, Variable*, void*);
        bool IFdrcSetInterMaxTime(Variable*, Variable*, void*);
        bool IFdrcGetInterMaxTime(Variable*, Variable*, void*);
        bool IFdrcSetInterMaxErrors(Variable*, Variable*, void*);
        bool IFdrcGetInterMaxErrors(Variable*, Variable*, void*);
        bool IFdrcSetInterSkipInst(Variable*, Variable*, void*);
        bool IFdrcGetInterSkipInst(Variable*, Variable*, void*);
        bool IFdrcSetLevel(Variable*, Variable*, void*);
        bool IFdrcGetLevel(Variable*, Variable*, void*);
        bool IFdrcCheckArea(Variable*, Variable*, void*);
        bool IFdrcChdCheckArea(Variable*, Variable*, void*);
        bool IFdrcCheckObjects(Variable*, Variable*, void*);
        bool IFdrcRegisterExpr(Variable*, Variable*, void*);
        bool IFdrcTestBox(Variable*, Variable*, void*);
        bool IFdrcTestPoly(Variable*, Variable*, void*);
        bool IFdrcZlist(Variable*, Variable*, void*);
        bool IFdrcZlistEx(Variable*, Variable*, void*);
    }
    using namespace drc_funcs;

#ifdef HAVE_PYTHON
    // Python wrappers.

    // Design Rule Checking
    PY_FUNC(DRCuserTest,            1,  IFdrcUserTest);
    PY_FUNC(DRCuserEmpty,           1,  IFdrcUserEmpty);
    PY_FUNC(DRCuserFull,            1,  IFdrcUserFull);
    PY_FUNC(DRCuserZlist,           1,  IFdrcUserZlist);
    PY_FUNC(DRCuserEdgeLength,      1,  IFdrcUserEdgeLength);
    PY_FUNC(DRCstate,               1,  IFdrcState);
    PY_FUNC(DRCsetLimits,           4,  IFdrcSetLimits);
    PY_FUNC(DRCgetLimits,           1,  IFdrcGetLimits);
    PY_FUNC(DRCsetMaxErrors,        1,  IFdrcSetMaxErrors);
    PY_FUNC(DRCgetMaxErrors,        0,  IFdrcGetMaxErrors);
    PY_FUNC(DRCsetInterMaxObjs,     1,  IFdrcSetInterMaxObjs);
    PY_FUNC(DRCgetInterMaxObjs,     0,  IFdrcGetInterMaxObjs);
    PY_FUNC(DRCsetInterMaxTime,     1,  IFdrcSetInterMaxTime);
    PY_FUNC(DRCgetInterMaxTime,     0,  IFdrcGetInterMaxTime);
    PY_FUNC(DRCsetInterMaxErrors,   1,  IFdrcSetInterMaxErrors);
    PY_FUNC(DRCgetInterMaxErrors,   0,  IFdrcGetInterMaxErrors);
    PY_FUNC(DRCsetInterSkipInst,    1,  IFdrcSetInterSkipInst);
    PY_FUNC(DRCgetInterSkipInst,    0,  IFdrcGetInterSkipInst);
    PY_FUNC(DRCsetLevel,            1,  IFdrcSetLevel);
    PY_FUNC(DRCgetLevel,            0,  IFdrcGetLevel);
    PY_FUNC(DRCcheckArea,           2,  IFdrcCheckArea);
    PY_FUNC(DRCchdCheckArea,        6,  IFdrcChdCheckArea);
    PY_FUNC(DRCcheckObjects,        1,  IFdrcCheckObjects);
    PY_FUNC(DRCregisterExpr,        1,  IFdrcRegisterExpr);
    PY_FUNC(DRCtestBox,             5,  IFdrcTestBox);
    PY_FUNC(DRCtestPoly,            3,  IFdrcTestPoly);
    PY_FUNC(DRCzList,               4,  IFdrcZlist);
    PY_FUNC(DRCzListEx,             7,  IFdrcZlistEx);

    void py_register_drc()
    {
      // Design Rule Checking
      cPyIf::register_func("DRCuserTest",            pyDRCuserTest);
      cPyIf::register_func("DRCuserEmpty",           pyDRCuserEmpty);
      cPyIf::register_func("DRCuserFull",            pyDRCuserFull);
      cPyIf::register_func("DRCuserZlist",           pyDRCuserZlist);
      cPyIf::register_func("DRCuserEdgeLength",      pyDRCuserEdgeLength);
      cPyIf::register_func("DRCstate",               pyDRCstate);
      cPyIf::register_func("DRCsetLimits",           pyDRCsetLimits);
      cPyIf::register_func("DRCgetLimits",           pyDRCgetLimits);
      cPyIf::register_func("DRCsetMaxErrors",        pyDRCsetMaxErrors);
      cPyIf::register_func("DRCgetMaxErrors",        pyDRCgetMaxErrors);
      cPyIf::register_func("DRCsetInterMaxObjs",     pyDRCsetInterMaxObjs);
      cPyIf::register_func("DRCgetInterMaxObjs",     pyDRCgetInterMaxObjs);
      cPyIf::register_func("DRCsetInterMaxTime",     pyDRCsetInterMaxTime);
      cPyIf::register_func("DRCgetInterMaxTime",     pyDRCgetInterMaxTime);
      cPyIf::register_func("DRCsetInterMaxErrors",   pyDRCsetInterMaxErrors);
      cPyIf::register_func("DRCgetInterMaxErrors",   pyDRCgetInterMaxErrors);
      cPyIf::register_func("DRCsetInterSkipInst",    pyDRCsetInterSkipInst);
      cPyIf::register_func("DRCgetInterSkipInst",    pyDRCgetInterSkipInst);
      cPyIf::register_func("DRCsetLevel",            pyDRCsetLevel);
      cPyIf::register_func("DRCgetLevel",            pyDRCgetLevel);
      cPyIf::register_func("DRCcheckArea",           pyDRCcheckArea);
      cPyIf::register_func("DRCchdCheckArea",        pyDRCchdCheckArea);
      cPyIf::register_func("DRCcheckObjects",        pyDRCcheckObjects);
      cPyIf::register_func("DRCregisterExpr",        pyDRCregisterExpr);
      cPyIf::register_func("DRCtestBox",             pyDRCtestBox);
      cPyIf::register_func("DRCtestPoly",            pyDRCtestPoly);
      cPyIf::register_func("DRCzList",               pyDRCzList);
      cPyIf::register_func("DRCzListEx",             pyDRCzListEx);
    }
#endif  // HAVE_PYTHON

#ifdef HAVE_TCL
    // TclTk wrappers.

    // Design Rule Checking
    TCL_FUNC(DRCuserTest,            1,  IFdrcUserTest);
    TCL_FUNC(DRCuserEmpty,           1,  IFdrcUserEmpty);
    TCL_FUNC(DRCuserFull,            1,  IFdrcUserFull);
    TCL_FUNC(DRCuserZlist,           1,  IFdrcUserZlist);
    TCL_FUNC(DRCuserEdgeLength,      1,  IFdrcUserEdgeLength);
    TCL_FUNC(DRCstate,               1,  IFdrcState);
    TCL_FUNC(DRCsetLimits,           4,  IFdrcSetLimits);
    TCL_FUNC(DRCgetLimits,           1,  IFdrcGetLimits);
    TCL_FUNC(DRCsetMaxErrors,        1,  IFdrcSetMaxErrors);
    TCL_FUNC(DRCgetMaxErrors,        0,  IFdrcGetMaxErrors);
    TCL_FUNC(DRCsetInterMaxObjs,     1,  IFdrcSetInterMaxObjs);
    TCL_FUNC(DRCgetInterMaxObjs,     0,  IFdrcGetInterMaxObjs);
    TCL_FUNC(DRCsetInterMaxTime,     1,  IFdrcSetInterMaxTime);
    TCL_FUNC(DRCgetInterMaxTime,     0,  IFdrcGetInterMaxTime);
    TCL_FUNC(DRCsetInterMaxErrors,   1,  IFdrcSetInterMaxErrors);
    TCL_FUNC(DRCgetInterMaxErrors,   0,  IFdrcGetInterMaxErrors);
    TCL_FUNC(DRCsetInterSkipInst,    1,  IFdrcSetInterSkipInst);
    TCL_FUNC(DRCgetInterSkipInst,    0,  IFdrcGetInterSkipInst);
    TCL_FUNC(DRCsetLevel,            1,  IFdrcSetLevel);
    TCL_FUNC(DRCgetLevel,            0,  IFdrcGetLevel);
    TCL_FUNC(DRCcheckArea,           2,  IFdrcCheckArea);
    TCL_FUNC(DRCchdCheckArea,        6,  IFdrcChdCheckArea);
    TCL_FUNC(DRCcheckObjects,        1,  IFdrcCheckObjects);
    TCL_FUNC(DRCregisterExpr,        1,  IFdrcRegisterExpr);
    TCL_FUNC(DRCtestBox,             5,  IFdrcTestBox);
    TCL_FUNC(DRCtestPoly,            3,  IFdrcTestPoly);
    TCL_FUNC(DRCzList,               4,  IFdrcZlist);
    TCL_FUNC(DRCzListEx,             7,  IFdrcZlistEx);

    void tcl_register_drc()
    {
      // Design Rule Checking
      cTclIf::register_func("DRCuserTest",            tclDRCuserTest);
      cTclIf::register_func("DRCuserEmpty",           tclDRCuserEmpty);
      cTclIf::register_func("DRCuserFull",            tclDRCuserFull);
      cTclIf::register_func("DRCuserZlist",           tclDRCuserZlist);
      cTclIf::register_func("DRCuserEdgeLength",      tclDRCuserEdgeLength);
      cTclIf::register_func("DRCstate",               tclDRCstate);
      cTclIf::register_func("DRCsetLimits",           tclDRCsetLimits);
      cTclIf::register_func("DRCgetLimits",           tclDRCgetLimits);
      cTclIf::register_func("DRCsetMaxErrors",        tclDRCsetMaxErrors);
      cTclIf::register_func("DRCgetMaxErrors",        tclDRCgetMaxErrors);
      cTclIf::register_func("DRCsetInterMaxObjs",     tclDRCsetInterMaxObjs);
      cTclIf::register_func("DRCgetInterMaxObjs",     tclDRCgetInterMaxObjs);
      cTclIf::register_func("DRCsetInterMaxTime",     tclDRCsetInterMaxTime);
      cTclIf::register_func("DRCgetInterMaxTime",     tclDRCgetInterMaxTime);
      cTclIf::register_func("DRCsetInterMaxErrors",   tclDRCsetInterMaxErrors);
      cTclIf::register_func("DRCgetInterMaxErrors",   tclDRCgetInterMaxErrors);
      cTclIf::register_func("DRCsetInterSkipInst",    tclDRCsetInterSkipInst);
      cTclIf::register_func("DRCgetInterSkipInst",    tclDRCgetInterSkipInst);
      cTclIf::register_func("DRCsetLevel",            tclDRCsetLevel);
      cTclIf::register_func("DRCgetLevel",            tclDRCgetLevel);
      cTclIf::register_func("DRCcheckArea",           tclDRCcheckArea);
      cTclIf::register_func("DRCchdCheckArea",        tclDRCchdCheckArea);
      cTclIf::register_func("DRCcheckObjects",        tclDRCcheckObjects);
      cTclIf::register_func("DRCregisterExpr",        tclDRCregisterExpr);
      cTclIf::register_func("DRCtestBox",             tclDRCtestBox);
      cTclIf::register_func("DRCtestPoly",            tclDRCtestPoly);
      cTclIf::register_func("DRCzList",               tclDRCzList);
      cTclIf::register_func("DRCzListEx",             tclDRCzListEx);
    }
#endif  // HAVE_TCL
}


// Export to load functions in this script library.
//
void
cDRC::loadScriptFuncs()
{
  using namespace drc_funcs;

  // Design Rule Checking
  SIparse()->registerFunc("DRCuserTest",            1,  IFdrcUserTest);
  SIparse()->registerFunc("DRCuserEmpty",           1,  IFdrcUserEmpty);
  SIparse()->registerFunc("DRCuserFull",            1,  IFdrcUserFull);
  SIparse()->registerFunc("DRCuserZlist",           1,  IFdrcUserZlist);
  SIparse()->registerFunc("DRCuserEdgeLength",      1,  IFdrcUserEdgeLength);
  SIparse()->registerFunc("DRCstate",               1,  IFdrcState);
  SIparse()->registerFunc("DRCsetLimits",           4,  IFdrcSetLimits);
  SIparse()->registerFunc("DRCgetLimits",           1,  IFdrcGetLimits);
  SIparse()->registerFunc("DRCsetMaxErrors",        1,  IFdrcSetMaxErrors);
  SIparse()->registerFunc("DRCgetMaxErrors",        0,  IFdrcGetMaxErrors);
  SIparse()->registerFunc("DRCsetInterMaxObjs",     1,  IFdrcSetInterMaxObjs);
  SIparse()->registerFunc("DRCgetInterMaxObjs",     0,  IFdrcGetInterMaxObjs);
  SIparse()->registerFunc("DRCsetInterMaxTime",     1,  IFdrcSetInterMaxTime);
  SIparse()->registerFunc("DRCgetInterMaxTime",     0,  IFdrcGetInterMaxTime);
  SIparse()->registerFunc("DRCsetInterMaxErrors",   1,  IFdrcSetInterMaxErrors);
  SIparse()->registerFunc("DRCgetInterMaxErrors",   0,  IFdrcGetInterMaxErrors);
  SIparse()->registerFunc("DRCsetInterSkipInst",    1,  IFdrcSetInterSkipInst);
  SIparse()->registerFunc("DRCgetInterSkipInst",    0,  IFdrcGetInterSkipInst);
  SIparse()->registerFunc("DRCsetLevel",            1,  IFdrcSetLevel);
  SIparse()->registerFunc("DRCgetLevel",            0,  IFdrcGetLevel);
  SIparse()->registerFunc("DRCcheckArea",           2,  IFdrcCheckArea);
  SIparse()->registerFunc("DRCchdCheckArea",        6,  IFdrcChdCheckArea);
  SIparse()->registerFunc("DRCcheckObjects",        1,  IFdrcCheckObjects);
  SIparse()->registerFunc("DRCregisterExpr",        1,  IFdrcRegisterExpr);
  SIparse()->registerFunc("DRCtestBox",             5,  IFdrcTestBox);
  SIparse()->registerFunc("DRCtestPoly",            3,  IFdrcTestPoly);
  SIparse()->registerFunc("DRCzList",               4,  IFdrcZlist);
  SIparse()->registerFunc("DRCzListEx",             7,  IFdrcZlistEx);

  // Layer Expression (Alt) functions
  // The third column contains bit flags that are set for arguments  
  // that should be passed as strings, the default is to eval string 
  // arguments as Zlists before passing to the function.
      
  SIparse()->registerAltFunc("drcZlist",            4, 3, IFdrcZlist);
  SIparse()->registerAltFunc("drcZlistEx",          7, 14, IFdrcZlistEx);

#ifdef HAVE_PYTHON
  py_register_drc();
#endif
#ifdef HAVE_TCL
  tcl_register_drc();
#endif
}


//-------------------------------------------------------------------------
// Design Rule Checking
//-------------------------------------------------------------------------

//=============================================================
// Special functions for user-defined rules in DRC.  These
// are called from the rule evaluator and not from scripts
//=============================================================

// (int) DRCuserTest(indx)
//
// Return 1 if the test region is not empty, 0 otherwise.
//
bool
drc_funcs::IFdrcUserTest(Variable *res, Variable *args, void*)
{
    int indx;
    ARG_CHK(arg_int(args, 0, &indx))

    if (indx < 0)
        indx = 0;
    bool istrue;
    XIrt ret = DRC()->userTest(indx, &istrue);
    if (ret != XIok)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = istrue;
    return (OK);
}


// (int) DRCuserEmpty(indx)
//
// Return 1 if the test region is empty, 0 otherwise.
//
bool
drc_funcs::IFdrcUserEmpty(Variable *res, Variable *args, void*)
{
    int indx;
    ARG_CHK(arg_int(args, 0, &indx))

    if (indx < 0)
        indx = 0;
    bool istrue;
    XIrt ret = DRC()->userEmpty(indx, &istrue);
    if (ret != XIok)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = istrue;
    return (OK);
}


// (int) DRCuserFull(indx)
//
// Return 1 if the test region is fully covered, 0 otherwise.
//
bool
drc_funcs::IFdrcUserFull(Variable *res, Variable *args, void*)
{
    int indx;
    ARG_CHK(arg_int(args, 0, &indx))

    if (indx < 0)
        indx = 0;
    bool istrue;
    XIrt ret = DRC()->userFull(indx, &istrue);
    if (ret != XIok)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = istrue;
    return (OK);
}


// (zoidlist) DRCuserZlist(indx)
//
// Return the zoids clipped from the test region.
//
bool
drc_funcs::IFdrcUserZlist(Variable *res, Variable *args, void*)
{
    int indx;
    ARG_CHK(arg_int(args, 0, &indx))

    if (indx < 0)
        indx = 0;
    Zlist *zlist;
    XIrt ret = DRC()->userZlist(indx, &zlist);
    if (ret != XIok)
        return (BAD);
    res->type = TYP_ZLIST;
    res->content.zlist = zlist;
    return (OK);
}


// (int) DRCuserEdgeLength(indx)
//
// Return the length of the test segment along the edge.
//
bool
drc_funcs::IFdrcUserEdgeLength(Variable *res, Variable *args, void*)
{
    int indx;
    ARG_CHK(arg_int(args, 0, &indx))

    if (indx < 0)
        indx = 0;
    double length;
    XIrt ret = DRC()->userEdgeLength(indx, &length);
    if (ret != XIok)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = length;
    return (OK);
}

//===== End of user-defined DRC rule tests ====================


// (int) DRCstate(istate)
//
// This function sets the interactive DRC state, and returns the
// existing state.  If the argument is 0, interactive DRC is turned
// off.  If nonzero, interactive DRC is turned on.  If greater than 1,
// error messages will not pop up.  The return value is the present
// state, which is a value of 0-2, similarly interpreted.
//
bool
drc_funcs::IFdrcState(Variable *res, Variable *args, void*)
{
    int state;
    ARG_CHK(arg_int(args, 0, &state))

    res->content.value = DRC()->isInteractive();
    if (DRC()->isInteractive())
        res->content.value += DRC()->isIntrNoErrMsg();
    if (state)
        CDvdb()->setVariable(VA_Drc, "");
    else
        CDvdb()->clearVariable(VA_Drc);
    state >>= 1;
    if (state)
        CDvdb()->setVariable(VA_DrcNoPopup, "");
    else
        CDvdb()->clearVariable(VA_DrcNoPopup);
    res->type = TYP_SCALAR;
    return (OK);
}


// (int) DRCsetLimits(batch_cnt, intr_cnt, intr_time, skip_cells)
//
// Deprecated in favor of DRCsetMaxErrors and similar.
//
// This function sets the limits used in design rule checking.  Each
// argument, if negative, will cause the related value to be unchanged
// by the function call.  For the first three arguments, the value "0"
// is interpreted as "no limit".
//
// batch_cnt:  This sets the maximum number of errors to record in
//   batch-mode error checking.  When this number is reached, the
//   checking is aborted.  Values 0 - 100000 are accepted.
//
// intr_cnt:  This sets the maximum number of objects tested in
//   interactive DRC.  The testing aborts when this count is reached.
//   Values of 0 - 100000 are accepted.
//
// intr_time:  This sets the maximum time allowed for interactive DRC
//   testing.  The value given is in milliseconds, and values of 0 -
//   30000 are accepted.
//
// skip_cells:  If nonzero, testing of newly placed, moved, or copied
//   subcells is skipped in interactive DRC.  If zero, subcells will be
//   tested.  This can be a lengthly operation.
//
// This function always returns 1.  Out-of-range arguments are set to
// the maximum permissible values.
//
bool
drc_funcs::IFdrcSetLimits(Variable *res, Variable *args, void*)
{
    int bcnt;
    ARG_CHK(arg_int(args, 0, &bcnt))
    int icnt;
    ARG_CHK(arg_int(args, 1, &icnt))
    int itime;
    ARG_CHK(arg_int(args, 2, &itime))
    int skipc;
    ARG_CHK(arg_int(args, 3, &skipc))

    char buf[32];
    if (bcnt > DRC_MAX_ERRS_MAX)
        bcnt = DRC_MAX_ERRS_MAX;
    if (icnt > DRC_INTR_MAX_OBJS_MAX)
        icnt = DRC_INTR_MAX_OBJS_MAX;
    if (itime > DRC_INTR_MAX_TIME_MAX)
        itime = DRC_INTR_MAX_TIME_MAX;
    if (bcnt >= 0) {
        if (bcnt == DRC_MAX_ERRS_DEF)
            CDvdb()->clearVariable(VA_DrcMaxErrors);
        else {
            sprintf(buf, "%d", bcnt);
            CDvdb()->setVariable(VA_DrcMaxErrors, buf);
        }
    }
    if (icnt >= 0) {
        if (icnt == DRC_INTR_MAX_OBJS_DEF)
            CDvdb()->clearVariable(VA_DrcInterMaxObjs);
        else {
            sprintf(buf, "%d", icnt);
            CDvdb()->setVariable(VA_DrcInterMaxObjs, buf);
        }
    }
    if (itime >= 0) {
        if (itime == DRC_INTR_MAX_TIME_DEF)
            CDvdb()->clearVariable(VA_DrcInterMaxTime);
        else {
            sprintf(buf, "%d", itime);
            CDvdb()->setVariable(VA_DrcInterMaxTime, buf);
        }
    }
    if (skipc >= 0) {
        if (skipc)
            CDvdb()->setVariable(VA_DrcInterSkipInst, "");
        else
            CDvdb()->clearVariable(VA_DrcInterSkipInst);
    }
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) DRCgetLimits(array)
//
// Deprecated in favor of DRCgetMaxErrors and similar.
//
// This function fills the array, which must have size 4 or larger,
// with the current DRC limit values.  These are, in order,
//
//   [0]  The batch error count limit
//   [1]  The interactive object count limit
//   [2]  The interactive time limit in milliseconds
//   [3]  A flag which indicates interactive DRC is skipped for subcells
//
// The return value is always 1.  The function fails if the array
// argument is bad.
//
bool
drc_funcs::IFdrcGetLimits(Variable *res, Variable *args, void*)
{
    double *vals;
    ARG_CHK(arg_array(args, 0, &vals, 4))

    vals[0] = DRC()->maxErrors();
    vals[1] = DRC()->intrMaxObjs();
    vals[2] = DRC()->intrMaxTime();
    vals[3] = DRC()->isIntrSkipInst();
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) DRCsetMaxErrors(value)
//
// Set the maximum violation count allowed before a batch DRC run is
// terminated.  If set to 0, no limit is imposed.  The value is
// clipped to the acceptable range 0 - 100,000.  If not set, a value 0
// (no limit) is assumed.  The function returns the previous value.
//
bool
drc_funcs::IFdrcSetMaxErrors(Variable *res, Variable *args, void*)
{
    int val;
    ARG_CHK(arg_int(args, 0, &val))
    if (val < DRC_MAX_ERRS_MIN)
        val = DRC_MAX_ERRS_MIN;
    else if (val > DRC_MAX_ERRS_MAX)
        val = DRC_MAX_ERRS_MAX;
    res->type = TYP_SCALAR;
    res->content.value = DRC()->maxErrors();
    char buf[32];
    if (val == DRC_MAX_ERRS_DEF)
        CDvdb()->clearVariable(VA_DrcMaxErrors);
    else {
        sprintf(buf, "%d", val);
        CDvdb()->setVariable(VA_DrcMaxErrors, buf);
    }
    return (OK);
}


// (int) DRCgetMaxErrors()
//
// Returns the maximum violation count before a batch DRC run is
// terminated.  If set to 0, no limit is imposed.
//
bool
drc_funcs::IFdrcGetMaxErrors(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = DRC()->maxErrors();
    return (OK);
}


// (int) DRCsetInterMaxObjs(value)
//
// Set the maximum number of objects tested in interctive DRC. 
// Further testing is skipped when this value is reached.  A value of
// 0 imposes no limit.  The passed value is clipped to the acceptable
// range 0 - 100,000, the value used if not set is 1000.  The function
// returns the previous setting.
//
bool
drc_funcs::IFdrcSetInterMaxObjs(Variable *res, Variable *args, void*)
{
    int val;
    ARG_CHK(arg_int(args, 0, &val))
    if (val < DRC_INTR_MAX_OBJS_MIN)
        val = DRC_INTR_MAX_OBJS_MIN;
    else if (val > DRC_INTR_MAX_OBJS_MAX)
        val = DRC_INTR_MAX_OBJS_MAX;
    res->type = TYP_SCALAR;
    res->content.value = DRC()->intrMaxObjs();
    char buf[32];
    if (val == DRC_INTR_MAX_OBJS_DEF)
        CDvdb()->clearVariable(VA_DrcInterMaxObjs);
    else {
        sprintf(buf, "%d", val);
        CDvdb()->setVariable(VA_DrcInterMaxObjs, buf);
    }
    return (OK);
}


// (int) DRCgetInterMaxObjs()
//
// Return the maximum number of objects tested in interctive DRC. 
// Further testing is skipped when this value is reached.  A value of
// 0 imposes no limit.
//
bool
drc_funcs::IFdrcGetInterMaxObjs(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = DRC()->intrMaxObjs();
    return (OK);
}


// (int) DRCsetInterMaxTime(value)
//
// Set the maximum time in milliseconds allowed for interactive DRC
// testing after an operation.  The testing will abort after this
// limit, returning program control to the user.  If set to 0, no time
// limit is imposed.  the passed value is clipped to the acceptable
// range 0 - 30,000.  If not set, a value of 5000 (5 seconds) is used. 
// The function returns the previous value.
//
bool
drc_funcs::IFdrcSetInterMaxTime(Variable *res, Variable *args, void*)
{
    int val;
    ARG_CHK(arg_int(args, 0, &val))
    if (val < DRC_INTR_MAX_TIME_MIN)
        val = DRC_INTR_MAX_TIME_MIN;
    else if (val > DRC_INTR_MAX_TIME_MAX)
        val = DRC_INTR_MAX_TIME_MAX;
    res->type = TYP_SCALAR;
    res->content.value = DRC()->intrMaxTime();
    char buf[32];
    if (val == DRC_INTR_MAX_TIME_DEF)
        CDvdb()->clearVariable(VA_DrcInterMaxTime);
    else {
        sprintf(buf, "%d", val);
        CDvdb()->setVariable(VA_DrcInterMaxTime, buf);
    }
    return (OK);
}


// (int) DRCgetInterMaxTime()
//
// Return the maximum time in milliseconds allowed for interactive DRC
// testing after an operation.  The testing will abort after this
// limit, returning program control to the user.  If set to 0, no time
// limit is imposed.
//
bool
drc_funcs::IFdrcGetInterMaxTime(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = DRC()->intrMaxTime();
    return (OK);
}


// (int) DRCsetInterMaxErrors(value)
//
// Set the maximum number of errors allowed in interactive DRC testing
// after an operation.  Further testing is skipped after this count is
// reached.  A value of 0 imposes no limit.  The value will be clipped
// to the acceptable rnge 0 - 1000.  If not set, a value of 100 is
// used.  The function returns the previous value.
//
bool
drc_funcs::IFdrcSetInterMaxErrors(Variable *res, Variable *args, void*)
{
    int val;
    ARG_CHK(arg_int(args, 0, &val))
    if (val < DRC_INTR_MAX_ERRS_MIN)
        val = DRC_INTR_MAX_ERRS_MIN;
    else if (val > DRC_INTR_MAX_ERRS_MAX)
        val = DRC_INTR_MAX_ERRS_MAX;
    res->type = TYP_SCALAR;
    res->content.value = DRC()->intrMaxErrors();
    char buf[32];
    if (val == DRC_INTR_MAX_ERRS_DEF)
        CDvdb()->clearVariable(VA_DrcInterMaxErrors);
    else {
        sprintf(buf, "%d", val);
        CDvdb()->setVariable(VA_DrcInterMaxErrors, buf);
    }
    return (OK);
}


// (int) DRCgetInterMaxErrors()
//
// Return the maximum number of errors allowed in interactive DRC
// testing after an operation.  Further testing is skipped after this
// count is reached.  A value of 0 imposes no limit.
//
bool
drc_funcs::IFdrcGetInterMaxErrors(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = DRC()->intrMaxErrors();
    return (OK);
}


// (int) DRCsetInterSkipInst(value)
//
// If the boolean argument is nonzero, cell instances will not be
// checked for violations in interactive DRC.  The test can be
// lengthly and the user may want to defer such testing.  The return
// value is 0 or 1 representing the previous setting.
//
bool
drc_funcs::IFdrcSetInterSkipInst(Variable *res, Variable *args, void*)
{
    bool b;
    ARG_CHK(arg_boolean(args, 0, &b))
    res->type = TYP_SCALAR;
    res->content.value = DRC()->isIntrSkipInst();
    if (b)
        CDvdb()->setVariable(VA_DrcInterSkipInst, "");
    else
        CDvdb()->clearVariable(VA_DrcInterSkipInst);
    return (OK);
}


// (int) DRCgetInterSkipInst()
//
// The return value of this function is 0 or 1 representing whether
// cell instances are skipped (if 1) in interactive DRC testing.
//
bool
drc_funcs::IFdrcGetInterSkipInst(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = DRC()->isIntrSkipInst();
    return (OK);
}


// (int) DRCsetLevel(level)
//
// This function sets the DRC error recording level to the argument.
// The argument is interpreted as follows:
//
//  0 or negative      One error is reported per object
//  1                  One error of each type is reported per object
//  2 or larger        All errors are reported
//
// This function always succeeds, and the previous level (0, 1, 2) is
// returned.
//
bool
drc_funcs::IFdrcSetLevel(Variable *res, Variable *args, void*)
{
    int level;
    ARG_CHK(arg_int(args, 0, &level))

    res->type = TYP_SCALAR;
    res->content.value = DRC()->errorLevel();
    if (level < 0)
        level = 0;
    else if (level > 2)
        level = 2;
    if (level == 0)
        CDvdb()->clearVariable(VA_DrcLevel);
    else {
        char buf[32];
        sprintf(buf, "%d", level);
        CDvdb()->setVariable(VA_DrcLevel, buf);
    }
    return (OK);
}


// (int) DRCgetLevel()
//
// This function returns the current error reporting level for design
// rule checking.  Possible values are
//
//  0              One error is reported per object
//  1              One error of each type is reported per object
//  2              All errors are reported
//
// This function always succeeds.
//
bool
drc_funcs::IFdrcGetLevel(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = DRC()->errorLevel();
    return (OK);
}


// (int) DRCcheckArea(array, file_handle_or_name)
//
// This fuction performs batch-mode design rule checking in the
// current cell.
//
// The array argument is an array of size 4 or larger, or 0 can be
// passed for this argument.  If an array is passed, it represents a
// rectangular area where checking is performed, and the values are in
// microns in order L,B,R,T.  If 0 is passed, the entire area of the
// current cell is checked.
//
// The second argument can be a file handle opened with the Open
// function for writing, or the name of a file to open, or an empty
// string, or a null string or (equivalently) the scalar 0.  This sets
// the destination for error recording.  If the argument is null or 0,
// a file will be created in the current directory using the name
// template "drcerror.log.cellname", where cellname is the current
// cell.  If an empty string is passed (give "" as the argument),
// output will go to the error log, and appear in the pop-up wich
// appears on-screen.  If a string is given, it is taken as a file
// name to open.
//
// The function returns an integer, either the number of errors found
// or -1 on error.  If -1 is returned, an error message is probably
// available from the GetError function.
//
bool
drc_funcs::IFdrcCheckArea(Variable *res, Variable *args, void*)
{
    double *vals;
    ARG_CHK(arg_array_if(args, 0, &vals, 4))
    int id = 0;
    const char *fname = 0;
    if (args[1].type == TYP_HANDLE)
        ARG_CHK(arg_handle(args, 1, &id))
    else if (args[1].type == TYP_STRING || args[1].type == TYP_NOTYPE ||
            args[1].type == TYP_SCALAR)
        ARG_CHK(arg_string(args, 1, &fname))

    res->type = TYP_SCALAR;
    res->content.value = -1;
    BBox AOI;
    if (vals) {
        BBox BB(INTERNAL_UNITS(vals[0]), INTERNAL_UNITS(vals[1]),
            INTERNAL_UNITS(vals[2]), INTERNAL_UNITS(vals[3]));
        BB.fix();
        AOI = BB;
    }
    CDs *sd = CurCell(Physical);
    if (!sd) {
        Errs()->add_error("no current cell.");
        return (OK);
    }

    char *outfile = 0;
    FILE *fp = 0;
    if (id > 0) {
        sHdl *hdl = sHdl::get(id);
        if (hdl) {
            if (hdl->type != HDLfd)
                return (BAD);
            fp = fdopen(id, (char*)hdl->data);
            if (!fp) {
                Errs()->add_error(
                    "failed to open file pointer from descriptor.");
                return (OK);
            }
        }
        else {
            Errs()->add_error("unresolved handle.");
            return (OK);
        }
    }
    else if (!fname) {
        outfile = DRC()->errFilename(sd->cellname()->string(), 0);
        if (!filestat::create_bak(outfile)) {
            Errs()->add_error("can't open backup output file %s.",
                outfile);
            delete [] outfile;
            return (OK);
        }
        fp = fopen(outfile, "w");
        if (!fp) {
            Errs()->add_error("can't open output file %s.", outfile);
            delete [] outfile;
            return (OK);
        }
    }
    else if (*fname) {
        if (!filestat::create_bak(fname)) {
            Errs()->add_error("can't open backup output file %s.", fname);
            return (OK);
        }
        fp = fopen(fname, "w");
        if (!fp) {
            Errs()->add_error("can't open output file %s.", fname);
            return (OK);
        }
    }
    if (fp) {
        if (!DRC()->printFileHeader(fp, sd->cellname()->string(),
                vals ? &AOI : 0))
            return (OK);
    }

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    DRC()->batchTest(vals ? &AOI : 0, fp, 0, 0);
    res->content.value = DRC()->getErrCount();
    DRC()->printFileEnd(fp, DRC()->getErrCount(), true);
    if (fp)
        fclose(fp);
    if (outfile) {
        // Using default file name, update the "next" command.
        DRC()->errReset(outfile, 0);
        delete [] outfile;
    }
    PL()->ShowPrompt("Done.");
    res->type = TYP_SCALAR;
    return (OK);
}


// (int) DRCchdCheckArea(chdname, cellname, gridsize, array,
//   file_handle_or_name, flatten)
//
// This function performs a batch-mode DRC of the given top-level
// cell, from the Cell Hierarchy Digest (CHD) whose access names is
// given as the first argument.  Unlike other DRC commands, this
// function does not require that the entire layout be in memory, thus
// it is theoretically possible to perform DRC on designs that are too
// large for available memory.
//
// If the given cellname is null or 0 is passed, the default cell for
// the named CHD is assumed.
//
// The checking is performed on the areas of a grid, and only the
// cells needed to render the grid area are read into memory
// temporarily.  The gridsize argument gives the size of this grid, in
// microns.  If 0 is passed, no grid is used, and the entire layout
// will be read into memory, as in the normal case.  If a negative
// value is passed, the value associated with the DrcPartitionSize
// variable is used.  The chosen grid size should be small enough to
// avoid page swapping, but too-small of a grid will lengthen checking
// time (larger is better in this regard).  The user can experiment to
// find a reasonable value for their designs.  A good starting value
// might be 400.0 microns.
//
// The array argument is an array of size 4 or larger, or 0 can be
// passed for this argument.  If an array is passed, it represents a
// rectangular area where checking is performed, and the values are in
// microns in order L,B,R,T.  If 0 is passed, the entire area of the
// cellname is checked.
//
// The file_handle_or_name argument can be a file handle opened with
// the Open function for writing, or the name of a file to open, or an
// empty or null string or the scalar 0.  This sets the destination
// for error recording.  If the argument is null, empty or 0, a file
// will be created in the current directory using the name template
// "drcerror.log.cellname", where cellname is the top-level cell being
// checked.  If a string is given, it is taken as a file name to open. 
// There is no provision for sending output to the on-screen error
// logger, unlike in the DRCcheckArea function.
//
// If the boolean argument flatten is true, the geometry will be
// flattened as it is read into memory.  This will make life simpler
// and faster for the DRC evaluation functions, at the expense of much
// larger memory use.  The user can experiment to find if this option
// provides any speed benefit.
//
// The function returns an integer, either the number of errors found
// or -1 on error.  If -1 is returned, an error message is probably
// available from the GetError function.
//
bool
drc_funcs::IFdrcChdCheckArea(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))
    int grsize;
    ARG_CHK(arg_coord(args, 2, &grsize, Physical))
    double *vals;
    ARG_CHK(arg_array_if(args, 3, &vals, 4))
    int id = 0;
    const char *fname = 0;
    if (args[4].type == TYP_HANDLE)
        ARG_CHK(arg_handle(args, 4, &id))
    else if (args[4].type == TYP_STRING || args[4].type == TYP_NOTYPE ||
            args[4].type == TYP_SCALAR)
        ARG_CHK(arg_string(args, 4, &fname))
    bool flatten;
    ARG_CHK(arg_boolean(args, 5, &flatten))

    if (!chdname || !*chdname)
        return (BAD);

    BBox AOI;
    if (vals) {
        BBox BB(INTERNAL_UNITS(vals[0]), INTERNAL_UNITS(vals[1]),
            INTERNAL_UNITS(vals[2]), INTERNAL_UNITS(vals[3]));
        BB.fix();
        AOI = BB;
    }

    res->type = TYP_SCALAR;
    res->content.value = -1;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (!chd) {
        Errs()->add_error("Unresolved CHD access name");
        return (OK);
    }

    FILE *fp = 0;
    if (id > 0) {
        sHdl *hdl = sHdl::get(id);
        if (hdl) {
            if (hdl->type != HDLfd)
                return (BAD);
            fp = fdopen(id, (char*)hdl->data);
            if (!fp) {
                Errs()->add_error(
                    "failed to open file pointer from descriptor.");
                return (OK);
            }
        }
        else {
            Errs()->add_error("unresolved handle.");
            return (OK);
        }
    }
    else if (fname && *fname) {
        if (!filestat::create_bak(fname)) {
            Errs()->add_error("can't open backup output file %s.", fname);
            return (OK);
        }
        fp = fopen(fname, "w");
        if (!fp) {
            Errs()->add_error("can't open output file %s.", fname);
            return (OK);
        }
    }
    int grbak = DRC()->gridSize();
    if (grsize >= 0)
        DRC()->setGridSize(grsize);
    switch (DRC()->chdGridBatchTest(chd, cname, vals ? &AOI : 0, fp,
        flatten)) {
    case XIok:
        res->content.value = DRC()->getErrCount();
        break;
    case XIbad:
        break;
    case XIintr:
        SI()->SetInterrupt();
        break;
    }
    DRC()->setGridSize(grbak);
    return (OK);
}


// (int) DRCcheckObjects(file_handle)
//
// This function checks each selected object for design rule
// violations.  The argument is a file handle returned from the Open()
// function, or 0.  If a file handle is passed, output goes to that
// file, otherwise output goes to the on-screen error logger.  This
// function returns the number of errors found.
//
bool
drc_funcs::IFdrcCheckObjects(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    res->type = TYP_SCALAR;
    res->content.value = 0.0;
    if (Selections.hasTypes(CurCell(Physical), "bpwc")) {
        FILE *fp = 0;
        if (id > 0) {
            sHdl *hdl = sHdl::get(id);
            if (hdl) {
                if (hdl->type != HDLfd)
                    return (BAD);
                fp = fdopen(id, (char*)hdl->data);
                if (!fp)
                    perror("fdopen");
            }
        }
        CDol *st = Selections.listQueue(CurCell(Physical));
        DRC()->batchListTest(st, fp, 0, 0);
        st->free();
        res->content.value = DRC()->getErrCount();
        if (fp)
            fclose(fp);
        PL()->ShowPrompt("Done.");
    }
    return (OK);
}


// (expr_handle) DRCregisterExpr(expr)
//
// This function creates a parse tree from the string argument, which
// is a layer expression, for later use, and returns a handle to the
// expression.  This avoids the overhead of parsing the expression on
// each function call.  The returned handle is used by other functions
// (currently just the two below).
//
bool
drc_funcs::IFdrcRegisterExpr(Variable *res, Variable *args, void*)
{
    const char *expr;
    ARG_CHK(arg_string(args, 0, &expr))

    if (!expr || !*expr)
        return (BAD);
    res->type = TYP_SCALAR;

    SIlexp_list *exlist = SIparse()->setExprs(0);
    if (exlist) {
        SIparse()->setExprs(exlist);
        int dsc = exlist->check(expr);
        if (dsc) {
            res->content.value = dsc;
            return (OK);
        }
    }

    sLspec lspec;
    const char *e = expr;
    if (lspec.parseExpr(&expr, true)) {
        if (!lspec.setup())
            return (BAD);
        if (!exlist) {
            exlist = new SIlexp_list;
            SIparse()->setExprs(exlist);
        }
        // Use the lname field to hold a pointer to the expression,
        // have to zero before deleting lspec.  Hope this doesn't
        // cause trouble.  This is for fast comparison, when used in
        // a DRC rule
        delete [] lspec.lname();
        lspec.set_lname_pointer((char*)e);
        res->content.value = exlist->add(lspec);
        lspec.clear();
        return (OK);
    }
    return (BAD);
}


// (int) DRCtestBox(left, bottom, right, top, expr_handle)
//
// This function tests a rectangular area specified by the first four
// arguments for regions where a layer expression is true.  The
// expr_handle argument is the handle of a layer expression returned
// by DRCregisterExpr().  The returned value is 0 if the expression is
// nowhere true, 1 if the expression is true somewhere but not
// everywhere, and 2 if the expression is true everywhere in the test
// region.
//
bool
drc_funcs::IFdrcTestBox(Variable *res, Variable *args, void*)
{
    int x1;
    ARG_CHK(arg_coord(args, 0, &x1, Physical))
    int y1;
    ARG_CHK(arg_coord(args, 1, &y1, Physical))
    int x2;
    ARG_CHK(arg_coord(args, 2, &x2, Physical))
    int y2;
    ARG_CHK(arg_coord(args, 3, &y2, Physical))
    int indx;
    ARG_CHK(arg_int(args, 4, &indx))

    if (indx == 0)
        return (BAD);
    SIlexp_list *exlist = SIparse()->setExprs(0);
    if (!exlist)
        return (BAD);
    SIparse()->setExprs(exlist);
    const sLspec *ls = exlist->find(indx);
    if (!ls)
        return (BAD);
    sLspec lspec(*ls);

    BBox BB(x1, y1, x2, y2);
    BB.fix();

    res->type = TYP_SCALAR;
    res->content.value = 0.0;
    SIlexprCx cx(0, CDMAXCALLDEPTH, &BB);
    CovType cov;
    sPF::set_skip_drc(true);
    XIrt x = lspec.testZlistCovPartial(&cx, &cov, DRC()->fudgeVal());
    sPF::set_skip_drc(false);
    lspec.clear();
    if (x == XIok)
        res->content.value = (int)cov;
    else if (x == XIintr)
        SI()->SetInterrupt();
    else
        return (BAD);
    return (OK);
}


// (int) DRCtestPoly(num, points, expr_handle)
//
// This function tests a polygon area for regions where a layer
// expression is true.  The first argument is the number of points in
// the polygon.  The second argument is the name of an array variable
// containing the polygon data.  The polygon data are stored
// sequentially as x,y pairs, and the last point must be the same
// coordinate as the first.  The length of the vector must be at least
// two times the value passed for the first argument.  The expr_handle
// argument is the handle of a layer expression returned by
// DRCregisterExpr().  The returned value is 0 if the expression is
// nowhere true, 1 if the expression is true somewhere but not
// everywhere, and 2 if the expression is true everywhere in the test
// region.
//
bool
drc_funcs::IFdrcTestPoly(Variable *res, Variable *args, void*)
{
    Poly poly;
    ARG_CHK(arg_int(args, 0, &poly.numpts))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 2*poly.numpts))
    int indx;
    ARG_CHK(arg_int(args, 2, &indx))

    if (indx == 0)
        return (BAD);
    SIlexp_list *exlist = SIparse()->setExprs(0);
    if (!exlist)
        return (BAD);

    poly.points = new Point[poly.numpts];
    int i, j;
    for (j = i = 0; i < poly.numpts; i++, j += 2)
        poly.points[i].set(INTERNAL_UNITS(vals[j]), INTERNAL_UNITS(vals[j+1]));

    SIparse()->setExprs(exlist);
    const sLspec *ls = exlist->find(indx);
    if (!ls)
        return (BAD);
    sLspec lspec(*ls);

    Zlist *z0 = poly.toZlist();
    delete [] poly.points;
    res->type = TYP_SCALAR;
    res->content.value = 0.0;

    SIlexprCx cx(0, CDMAXCALLDEPTH, z0);
    CovType cov;
    sPF::set_skip_drc(true);
    XIrt x = lspec.testZlistCovPartial(&cx, &cov, DRC()->fudgeVal());
    sPF::set_skip_drc(false);
    z0->free();
    lspec.clear();
    if (x == XIok)
        res->content.value = (int)cov;
    else if (x == XIintr)
        SI()->SetInterrupt();
    else
        return (BAD);
    return (OK);
}


namespace {
    // Return true if p is a node representing the layer.
    //
    bool lyrmatch(const ParseNode *p, const CDl *ld)
    {
        if (p->type == PT_VAR) {
           if (p->data.v->type == TYP_LDORIG) {
                LDorig *ldo = p->data.v->content.ldorig;
                if (ldo->ldesc() == ld)
                    return (true);
            }
            else if (p->data.v->type == TYP_STRING ||
                    p->data.v->type == TYP_NOTYPE) {
                if (CDldb()->findLayer(p->data.v->name, Physical) == ld)
                    return (true);
            }
        }
        return (false);
    }
}


// (zoidlist) DRCzList(layername, rulename, index, source)
//
// This function will access existing design rule definitions, and use
// the associated test region generator to create a new trapezoid
// list, which is returned.  For example, in a MinSpaceTo rule test,
// we construct a "halo" around source polygons.  If this halo
// intersects any target polygons, a violation would be flagged.  The
// list of trapezoids that constitute the halos around the source
// polygons is the return of this function.
//
// The first three arguments specify an existing design rule.  The
// rule is defined on the layer named in the first argument (a
// string).  The type of rule is given as a string in the second
// argument.  This is the name of an "edge" rule which uses test
// regions constructed along edges to evaluate the rule.  Valid names
// are the user-defined rules and
//
//    MinEdgeLength
//    MaxWidth
//    MinWidth
//    MinSpace
//    MinSpaceTo
//    MinSpaceFrom
//    MinOverlap
//    MinNoOverlap
//
// The third argument is an integer index which specifies the rule to
// choose if there is more than one of the named type assigned to the
// layer.  The index is zero based, and indicates the position of the
// rule when listed in the window of the Design Rule Editor panel from
// the Edit Rules button in the DRC menu, relative to and counting
// only rules of the same type.  The is also the order as first seen
// by Xic, as read from the technology file or created interactively.
//
// The fourth argument is a "zoidlist" as is taken by many of the
// functions that deal with layer expressions and trapezoid lists, as
// explained for those functions.  If the value passed is a scalar 0,
// then geometry is obtained from the full hierarchy of the current
// cell.  In this case, the created test areas will be identical to
// those created during a DRC run.  It may be instructive to create a
// visible layer from this result, to see where testing is being
// performed.
//
// If the argument instead passes trapezoids, the result will be
// creation of the test regions as if the passed trapezoids were
// features on the layer or Region associated with the rule.  The
// actual features on the layer are ignored.
//
// The function will fail and halt execution if the first three
// arguments do not indicate an existing design rule definition.
//
bool
drc_funcs::IFdrcZlist(Variable *res, Variable *args, void *datap)
{
    const char *lname;
    ARG_CHK(arg_string(args, 0, &lname))
    const char *rname;
    ARG_CHK(arg_string(args, 1, &rname))
    int indx;
    ARG_CHK(arg_int(args, 2, &indx))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        Errs()->add_error("DrcZlist: no physical current cell.");
        return (BAD);
    }
    if (!lname || !*lname) {
        Errs()->add_error("DrcZlist: null or empty layer name.");
        return (BAD);
    }
    CDl *ld = CDldb()->findLayer(lname, Physical);
    if (!ld) {
        Errs()->add_error("DrcZlist: unknown layer %s.", lname);
        return (BAD);
    }
    CDl *ldrule = ld;
    if (!rname || !*rname) {
        Errs()->add_error("DrcZlist: null or empty rule name.");
        return (BAD);
    }
    DRCtype type = DRCtestDesc::ruleType(rname);
    switch (type) {
    case drNoRule:
        Errs()->add_error("DrcZlist: unknown design rule %s.", rname);
        return (BAD);
    case drMinEdgeLength:
    case drMaxWidth:
    case drMinWidth:
    case drMinSpace:
    case drMinSpaceTo:
    case drMinSpaceFrom:
    case drMinOverlap:
    case drMinNoOverlap:
    case drUserDefinedRule:
        break;
    default:
        Errs()->add_error(
            "DrcZlist: design rule %s doesn't use edge projection.", rname);
        return (BAD);
    }
    int cnt = 0;
    DRCtestDesc *td = *tech_prm(ld)->rules_addr();
    for ( ; td; td = td->next()) {
        if (td->type() == type) {
            if (cnt++ == indx)
                break;
        }
    }
    if (!td) {
        Errs()->add_error("DrcZlist: rule not found.");
        return (BAD);
    }

    Zlist *zsrc;
    bool orig;
    // Can't use ARG_CHK here!
    if (arg_zlist(args, 3, &zsrc, &orig, datap) != XIok)
        return (BAD);

    if (zsrc) {
        if (!orig)
            zsrc = zsrc->copy();
        XIrt ret;
        ld = zsrc->to_temp_layer(DRC_TMPLYR,
            TTLinternal | TTLnoinsert | TTLjoin, cursdp, &ret);
        if (ret != XIok)
            return (BAD);
    }

    td->initEdgeTest();
    td->setTestFunc(DRCtestDesc::getZlist);

    ParseNode *pbak = 0;
    bool pleft = false;
    if (ld != ldrule) {
        td->setLayer(ld);
        if (!td->hasRegion())
            td->setSourceLayer(ld);
        else {
            // The tree consists of the source layer ANDed with the
            // Region expression.  Temporarily replace the source
            // layer with the temporary layer.  We do this by swapping
            // out the appropriate left/right node for a temporary one
            // that references the temporary layer.

            ParseNode *p = td->sourceTree();
            if (lyrmatch(p->left, ldrule)) {
                pleft = true;
                pbak = p->left;
                p->left = 0;
            }
            else if (lyrmatch(p->right, ldrule)) {
                pbak = p->right;
                p->right = 0;
            }
            else {
                Errs()->add_error("DrcZlist: internal error in source tree.");
                return (BAD);
            }
            if (pbak) {
                const char *s = ld->name();
                ParseNode *pn = SIparse()->getLexprTree(&s);
                if (pleft)
                    p->left = pn;
                else
                    p->right = pn;
            }
        }
    }

    const Zlist *zref = ((SIlexprCx*)datap)->getZref();
    BBox BB;
    if (zref)
        zref->BB(BB);

    bool ret = OK;
    Zlist *z0 = 0, *ze = 0;
    if (ld == ldrule) {
        sPF gen(cursdp, zref ? &BB : 0, ld, CDMAXCALLDEPTH);
        CDo *odesc;
        while ((odesc = gen.next(false, false)) != 0) {
            Zlist *zl;
            if (td->bloat(odesc, &zl, true) != XIok) {
                ret = BAD;
                delete odesc;
                break;
            }
            delete odesc;
            if (!z0)
                z0 = ze = zl;
            else {
                while (ze->next)
                    ze = ze->next;
                ze->next = zl;
            }
        }
    }
    else {
        CDg gdesc;
        gdesc.init_gen(cursdp, ld, zref ? &BB : 0);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            Zlist *zl;
            if (td->bloat(odesc, &zl, true) != XIok) {
                ret = BAD;
                break;
            }
            if (!z0)
                z0 = ze = zl;
            else {
                while (ze->next)
                    ze = ze->next;
                ze->next = zl;
            }
        }
        cursdp->clearLayer(ld);
    }
    if (ret == BAD) {
        Errs()->add_error("DrcZlist: evaluation failed.");
        z0->free();
    }
    else {
        res->type = TYP_ZLIST;
        res->content.zlist = z0;
        res->flags |= VF_ORIGINAL;
    }
    if (ld != ldrule) {
        td->setLayer(ldrule);
        if (!td->hasRegion())
            td->setSourceLayer(ldrule);
        if (pbak) {
            ParseNode *p = td->sourceTree();
            if (pleft) {
                p->left->free();
                p->left = pbak;
            }
            else {
                p->right->free();
                p->right = pbak;
            }
        }
    }
    return (ret);
}


// (zoidlist) DRCzListEx(source, target, inside, outside, incode, outcode,
//    dimen)
//
// This is similar to DRCzList, however it does not reference an
// existing rule.  Instead, it accesses the test area generator
// directly, effectively creating an internal, temporary rule.
//
// The first argument is a "zoidlist" as expected by other functions
// that accept this argument type.  Unlike for DRCzList, this argument
// can not be zero or null.
//
// The second argument is a string providing a target layer
// expression.  This may be scalar 0 or null.  The inside and outside
// arguments are strings providing layer expressions that will select
// which parts of an edge will be used for test area generation.  The
// inside is the area inside the figure at the edge, and outside is
// just outside of the figure along the edge.  Either can be null or
// scalar 0.
//
// The incode and outcode are integer values 0-2 which indocate how
// the inside and outside expressions are to be interpreted with
// regard to defining the "active" part of the edge.  The values have
// the following interpretations:
//
//   0    Don't care, the value expression is ignored.
//   1    The active parts of the edge are where the expression is clear.
//   2    The active parts of the edge are where the expression is dark.
//
// The dimen is the width of the test area, in microns.  It must be a
// positive real number.
//
// If all goes well, a trapezoid list reprseenting the effective test
// areas is returned.
//
bool
drc_funcs::IFdrcZlistEx(Variable *res, Variable *args, void *datap)
{
    Zlist *zsrc;
    bool orig;
    // Can't use ARG_CHK here!
    if (arg_zlist(args, 0, &zsrc, &orig, datap) != XIok)
        return (BAD);
    const char *target_str;
    ARG_CHK(arg_string(args, 1, &target_str))
    const char *inside_str;
    ARG_CHK(arg_string(args, 2, &inside_str))
    const char *outside_str;
    ARG_CHK(arg_string(args, 3, &outside_str))
    int incode;
    ARG_CHK(arg_int(args, 4, &incode))
    int outcode;
    ARG_CHK(arg_int(args, 5, &outcode))
    int dimen;
    ARG_CHK(arg_coord(args, 6, &dimen, Physical))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        Errs()->add_error("DrcZlist: no physical current cell.");
        return (BAD);
    }
    if (incode < 0 || incode > 2)
        incode = 0;
    if (outcode < 0 || outcode > 2)
        outcode = 0;
    if (dimen == 0 || !zsrc) {
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        if (orig)
            zsrc->free();
        return (OK);
    }

    bool ok;
    DRCtestDesc td(0, inside_str, outside_str, target_str,
        dimen, (DRCedgeMode)incode, (DRCedgeMode)outcode, &ok);
    if (!ok) {
        Errs()->add_error("DrcZlistEx: rule construction failed.");
        return (BAD);
    }

    if (!orig)
        zsrc = zsrc->copy();
    XIrt xrt;
    CDl *ld = zsrc->to_temp_layer(DRC_TMPLYR,
        TTLinternal | TTLnoinsert | TTLjoin, cursdp, &xrt);
    if (xrt != XIok)
        return (BAD);

    td.initEdgeTest();
    td.setTestFunc(DRCtestDesc::getZlist);
    td.setLayer(ld);
    td.setSourceLayer(ld);

    const Zlist *zref = ((SIlexprCx*)datap)->getZref();
    BBox BB;
    if (zref)
        zref->BB(BB);

    bool ret = OK;
    Zlist *z0 = 0, *ze = 0;
    CDg gdesc;
    gdesc.init_gen(cursdp, ld, zref ? &BB : 0);
    CDo *odesc;
    while ((odesc = gdesc.next()) != 0) {
        Zlist *zl;
        if (td.bloat(odesc, &zl, true) != XIok) {
            ret = BAD;
            break;
        }
        if (!z0)
            z0 = ze = zl;
        else {
            while (ze->next)
                ze = ze->next;
            ze->next = zl;
        }
    }
    cursdp->clearLayer(ld);

    if (ret == BAD) {
        Errs()->add_error("DrcZlistEx: evaluation failed.");
        z0->free();
    }
    else {
        res->type = TYP_ZLIST;
        res->content.zlist = z0;
        res->flags |= VF_ORIGINAL;
    }
    return (ret);
}

