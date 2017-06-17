
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
 $Id: funcs_lexpr.cc,v 5.120 2015/10/30 04:37:52 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "cd.h"
#include "cd_types.h"
#include "cd_sdb.h"
#include "cd_lgen.h"
#include "cd_digest.h"
#include "geo_grid.h"
#include "geo_zgroup.h"
#include "geo_ylist.h"
#include "fio.h"
#include "fio_chd.h"
#include "si_parsenode.h"
#include "si_lexpr.h"
#include "si_lspec.h"
#include "si_spt.h"
#include "si_handle.h"
#include "si_args.h"
#include "si_parser.h"
#include "si_interp.h"
#include "python_if.h"
#include "tcltk_if.h"


////////////////////////////////////////////////////////////////////////
//
// Script Functions:  Layer Expressions
//
////////////////////////////////////////////////////////////////////////

// Exports for the script plug-in kit
//
void registerScriptFunc(SIscriptFunc f, const char *fname, int nargs)
{
    SIparse()->registerFunc(fname, nargs, f);
}


void unRegisterScriptFunc(const char *fname)
{
    SIparse()->unRegisterFunc(fname);
}

/* XXX make it so
To support Python and Tcl, the shared object should also provide the
wrapped functions.  We will 1) export the wrappers and registration
functions, 2) provide special include files that the user can include
for Python or Tcl support.

Need these:
    PY_FUNC(SetZref,                1,  IFsetZref);
      cPyIf::register_func("SetZref",                pySetZref);
    TCL_FUNC(SetZref,                1,  IFsetZref);
      cTclIf::register_func("SetZref",                tclSetZref);

PyObject *py_wrapper(conat char *n, PyObject *args, int nargs,
    SIscriptFunc f)
{
    return (PyIf()->wrapper(n, args, nargs, f));
}

void py_register_func(const char *name, PYFUNC f)
{
    cPyIf::register_func(name, f);
}
*/

namespace {
    namespace lexpr_funcs {

        // Trapezoid Lists and Layer Expressions
        bool IFsetZref(Variable*, Variable*, void*);
        bool IFgetZref(Variable*, Variable*, void*);
        bool IFgetZrefBB(Variable*, Variable*, void*);
        bool IFadvanceZref(Variable*, Variable*, void*);
        bool IFzHead(Variable*, Variable*, void*);
        bool IFzValues(Variable*, Variable*, void*);
        bool IFzLength(Variable*, Variable*, void*);
        bool IFzArea(Variable*, Variable*, void*);
        bool IFgetZlist(Variable*, Variable*, void*);
        bool IFgetSqZlist(Variable*, Variable*, void*);
        bool IFtransformZ(Variable*, Variable*, void*);
        bool IFbloatZ(Variable*, Variable*, void*);
        bool IFextentZ(Variable*, Variable*, void*);
        bool IFedgesZ(Variable*, Variable*, void*);
        bool IFmanhattanizeZ(Variable*, Variable*, void*);
        bool IFrepartitionZ(Variable*, Variable*, void*);
        bool IFboxZ(Variable*, Variable*, void*);
        bool IFzoidZ(Variable*, Variable*, void*);
        bool IFobjectZ(Variable*, Variable*, void*);
        bool IFparseLayerExpr(Variable*, Variable*, void*);
        bool IFevalLayerExpr(Variable*, Variable*, void*);
        bool IFtestCoverageFull(Variable*, Variable*, void*);
        bool IFtestCoveragePartial(Variable*, Variable*, void*);
        bool IFtestCoverageNone(Variable*, Variable*, void*);
        bool IFtestCoverage(Variable*, Variable*, void*);
        bool IFzToObjects(Variable*, Variable*, void*);
        bool IFzToTempLayer(Variable*, Variable*, void*);
        bool IFclearTempLayer(Variable*, Variable*, void*);
        bool IFzToFile(Variable*, Variable*, void*);
        bool IFzFromFile(Variable*, Variable*, void*);
        bool IFreadZfile(Variable*, Variable*, void*);
        bool IFchdGetZlist(Variable*, Variable*, void*);

        // Operations
        bool IFnoOp(Variable*, Variable*, void*);
        bool IFfilt(Variable*, Variable*, void*);
        bool IFgeomAnd(Variable*, Variable*, void*);
        bool IFgeomAndNot(Variable*, Variable*, void*);
        bool IFgeomCat(Variable*, Variable*, void*);
        bool IFgeomNot(Variable*, Variable*, void*);
        bool IFgeomOr(Variable*, Variable*, void*);
        bool IFgeomXor(Variable*, Variable*, void*);

        // Spatial Parameter Tables
        bool IFreadSPtable(Variable*, Variable*, void*);
        bool IFnewSPtable(Variable*, Variable*, void*);
        bool IFwriteSPtable(Variable*, Variable*, void*);
        bool IFclearSPtable(Variable*, Variable*, void*);
        bool IFfindSPtable(Variable*, Variable*, void*);
        bool IFgetSPdata(Variable*, Variable*, void*);
        bool IFsetSPdata(Variable*, Variable*, void*);

        // Polymorphic Flat Database
        bool IFchdOpenOdb(Variable*, Variable*, void*);
        bool IFchdOpenZdb(Variable*, Variable*, void*);
        bool IFchdOpenZbdb(Variable*, Variable*, void*);
        bool IFgetObjectsOdb(Variable*, Variable*, void*);
        bool IFlistLayersDb(Variable*, Variable*, void*);
        bool IFgetZlistDb(Variable*, Variable*, void*);
        bool IFgetZlistZbdb(Variable*, Variable*, void*);
        bool IFdestroyDb(Variable*, Variable*, void*);
        // bool IFshowDb(Variable*, Variable*, void*);  (in funcs_misc2.cc)

        // Named String Tables
        bool IFfindNameTable(Variable*, Variable*, void*);
        bool IFremoveNameTable(Variable*, Variable*, void*);
        bool IFlistNameTables(Variable*, Variable*, void*);
        bool IFclearNameTables(Variable*, Variable*, void*);
        bool IFaddNameToTable(Variable*, Variable*, void*);
        bool IFremoveNameFromTable(Variable*, Variable*, void*);
        bool IFfindNameInTable(Variable*, Variable*, void*);
        bool IFlistNamesInTable(Variable*, Variable*, void*);

        // dummy
        bool IFsqzStub(Variable*, Variable*, void*) { return (OK); }
    }
    using namespace lexpr_funcs;

#ifdef HAVE_PYTHON
    // Python wrappers.

    // Trapezoid Lists and Layer Expressions
    PY_FUNC(SetZref,                1,  IFsetZref);
    PY_FUNC(GetZref,                0,  IFgetZref);
    PY_FUNC(GetZrefBB,              1,  IFgetZrefBB);
    PY_FUNC(AdvanceZref,            2,  IFadvanceZref);
    PY_FUNC(Zhead,                  1,  IFzHead);
    PY_FUNC(Zvalues,                2,  IFzValues);
    PY_FUNC(Zlength,                1,  IFzLength);
    PY_FUNC(Zarea,                  1,  IFzArea);
    PY_FUNC(GetZlist,               2,  IFgetZlist);
    PY_FUNC(GetSqZlist,             1,  IFgetSqZlist);
    PY_FUNC(TransformZ,             5,  IFtransformZ);
    PY_FUNC(BloatZ,                 3,  IFbloatZ);
    PY_FUNC(ExtentZ,                1,  IFextentZ);
    PY_FUNC(EdgesZ,                 3,  IFedgesZ);
    PY_FUNC(ManhattanizeZ,          3,  IFmanhattanizeZ);
    PY_FUNC(RepartitionZ,           1,  IFrepartitionZ);
    PY_FUNC(BoxZ,                   4,  IFboxZ);
    PY_FUNC(ZoidZ,                  6,  IFzoidZ);
    PY_FUNC(ObjectZ,                2,  IFobjectZ);
    PY_FUNC(ParseLayerExpr,         1,  IFparseLayerExpr);
    PY_FUNC(EvalLayerExpr,          4,  IFevalLayerExpr);
    PY_FUNC(TestCoverageFull,       3,  IFtestCoverageFull);
    PY_FUNC(TestCoveragePartial,    3,  IFtestCoveragePartial);
    PY_FUNC(TestCoverageNone,       3,  IFtestCoverageNone);
    PY_FUNC(TestCoverage,           3,  IFtestCoverage);
    PY_FUNC(ZtoObjects,             4,  IFzToObjects);
    PY_FUNC(ZtoTempLayer,           3,  IFzToTempLayer);
    PY_FUNC(ClearTempLayer,         1,  IFclearTempLayer);
    PY_FUNC(ZtoFile,                3,  IFzToFile);
    PY_FUNC(ZfromFile,              1,  IFzFromFile);
    PY_FUNC(ReadZfile,              1,  IFreadZfile);
    PY_FUNC(ChdGetZlist,            6,  IFchdGetZlist);

    // Operations
    PY_FUNC(NoOp,                   1,  IFnoOp);
    PY_FUNC(Filt,                   2,  IFfilt);
    PY_FUNC(GeomAnd,          VARARGS,  IFgeomAnd);
    PY_FUNC(GeomAndNot,             2,  IFgeomAndNot);
    PY_FUNC(GeomCat,          VARARGS,  IFgeomCat);
    PY_FUNC(GeomNot,                1,  IFgeomNot);
    PY_FUNC(GeomOr,           VARARGS,  IFgeomOr);
    PY_FUNC(GeomXor,          VARARGS,  IFgeomXor);

    // Spatial Parameter Tables
    PY_FUNC(ReadSPtable,            1,  IFreadSPtable);
    PY_FUNC(NewSPtable,             7,  IFnewSPtable);
    PY_FUNC(WriteSPtable,           2,  IFwriteSPtable);
    PY_FUNC(ClearSPtable,           1,  IFclearSPtable);
    PY_FUNC(FindSPtable,            2,  IFfindSPtable);
    PY_FUNC(GetSPdata,              3,  IFgetSPdata);
    PY_FUNC(SetSPdata,              4,  IFsetSPdata);

    // Polymorphic Flat Database
    PY_FUNC(ChdOpenOdb,             6,  IFchdOpenOdb);
    PY_FUNC(ChdOpenZdb,             6,  IFchdOpenZdb);
    PY_FUNC(ChdOpenZbdb,            9,  IFchdOpenZbdb);
    PY_FUNC(GetObjectsOdb,          3,  IFgetObjectsOdb);
    PY_FUNC(ListLayersDb,           1,  IFlistLayersDb);
    PY_FUNC(GetZlistDb,             3,  IFgetZlistDb);
    PY_FUNC(GetZlistZbdb,           4,  IFgetZlistZbdb);
    PY_FUNC(DestroyDb,              1,  IFdestroyDb);

    // Named String Tables
    PY_FUNC(FindNameTable,          2,  IFfindNameTable);
    PY_FUNC(RemoveNameTable,        1,  IFremoveNameTable);
    PY_FUNC(ListNameTables,         0,  IFlistNameTables);
    PY_FUNC(ClearNameTables,        0,  IFclearNameTables);
    PY_FUNC(AddNameToTable,         3,  IFaddNameToTable);
    PY_FUNC(RemoveNameFromTable,    2,  IFremoveNameFromTable);
    PY_FUNC(FindNameInTable,        2,  IFfindNameInTable);
    PY_FUNC(ListNamesInTable,       1,  IFlistNamesInTable);

    void py_register_lexpr()
    {
      // Trapezoid Lists and Layer Expressions
      cPyIf::register_func("SetZref",                pySetZref);
      cPyIf::register_func("GetZref",                pyGetZref);
      cPyIf::register_func("GetZrefBB",              pyGetZrefBB);
      cPyIf::register_func("AdvanceZref",            pyAdvanceZref);
      cPyIf::register_func("Zhead",                  pyZhead);
      cPyIf::register_func("Zvalues",                pyZvalues);
      cPyIf::register_func("Zlength",                pyZlength);
      cPyIf::register_func("Zarea",                  pyZarea);
      cPyIf::register_func("GetZlist",               pyGetZlist);
      cPyIf::register_func("GetSqZlist",             pyGetSqZlist);
      cPyIf::register_func("TransformZ",             pyTransformZ);
      cPyIf::register_func("BloatZ",                 pyBloatZ);
      cPyIf::register_func("ExtentZ",                pyExtentZ);
      cPyIf::register_func("EdgesZ",                 pyEdgesZ);
      cPyIf::register_func("ManhattanizeZ",          pyManhattanizeZ);
      cPyIf::register_func("RepartitionZ",           pyRepartitionZ);
      cPyIf::register_func("BoxZ",                   pyBoxZ);
      cPyIf::register_func("ZoidZ",                  pyZoidZ);
      cPyIf::register_func("ObjectZ",                pyObjectZ);
      cPyIf::register_func("ParseLayerExpr",         pyParseLayerExpr);
      cPyIf::register_func("EvalLayerExpr",          pyEvalLayerExpr);
      cPyIf::register_func("TestCoverageFull",       pyTestCoverageFull);
      cPyIf::register_func("TestCoveragePartial",    pyTestCoveragePartial);
      cPyIf::register_func("TestCoverageNone",       pyTestCoverageNone);
      cPyIf::register_func("TestCoverage",           pyTestCoverage);
      cPyIf::register_func("ZtoObjects",             pyZtoObjects);
      cPyIf::register_func("ZtoTempLayer",           pyZtoTempLayer);
      cPyIf::register_func("ClearTempLayer",         pyClearTempLayer);
      cPyIf::register_func("ZtoFile",                pyZtoFile);
      cPyIf::register_func("ZfromFile",              pyZfromFile);
      cPyIf::register_func("ReadZfile",              pyReadZfile);
      cPyIf::register_func("ChdGetZlist",            pyChdGetZlist);

      // Operations
      cPyIf::register_func("NoOp",                   pyNoOp);
      cPyIf::register_func("Filt",                   pyFilt);
      cPyIf::register_func("GeomAnd",                pyGeomAnd);
      cPyIf::register_func("GeomAndNot",             pyGeomAndNot);
      cPyIf::register_func("GeomCat",                pyGeomCat);
      cPyIf::register_func("GeomNot",                pyGeomNot);
      cPyIf::register_func("GeomOr",                 pyGeomOr);
      cPyIf::register_func("GeomXor",                pyGeomXor);

      // Spatial Parameter Tables
      cPyIf::register_func("ReadSPtable",            pyReadSPtable);
      cPyIf::register_func("NewSPtable",             pyNewSPtable);
      cPyIf::register_func("WriteSPtable",           pyWriteSPtable);
      cPyIf::register_func("ClearSPtable",           pyClearSPtable);
      cPyIf::register_func("FindSPtable",            pyFindSPtable);
      cPyIf::register_func("GetSPdata",              pyGetSPdata);
      cPyIf::register_func("SetSPdata",              pySetSPdata);

      // Polymorphic Flat Database
      cPyIf::register_func("ChdOpenOdb",             pyChdOpenOdb);
      cPyIf::register_func("ChdOpenZdb",             pyChdOpenZdb);
      cPyIf::register_func("ChdOpenZbdb",            pyChdOpenZbdb);
      cPyIf::register_func("GetObjectsOdb",          pyGetObjectsOdb);
      cPyIf::register_func("ListLayersDb",           pyListLayersDb);
      cPyIf::register_func("GetZlistDb",             pyGetZlistDb);
      cPyIf::register_func("GetZlistZbdb",           pyGetZlistZbdb);
      cPyIf::register_func("DestroyDb",              pyDestroyDb);

      // Named String Tables
      cPyIf::register_func("FindNameTable",          pyFindNameTable);
      cPyIf::register_func("RemoveNameTable",        pyRemoveNameTable);
      cPyIf::register_func("ListNameTables",         pyListNameTables);
      cPyIf::register_func("ClearNameTables",        pyClearNameTables);
      cPyIf::register_func("AddNameToTable",         pyAddNameToTable);
      cPyIf::register_func("RemoveNameFromTable",    pyRemoveNameFromTable);
      cPyIf::register_func("FindNameInTable",        pyFindNameInTable);
      cPyIf::register_func("ListNamesInTable",       pyListNamesInTable);
    }
#endif  // HAVE_PYTHON

#ifdef HAVE_TCL
    // Tcl/Tk wrappers.

    // Trapezoid Lists and Layer Expressions
    TCL_FUNC(SetZref,                1,  IFsetZref);
    TCL_FUNC(GetZref,                0,  IFgetZref);
    TCL_FUNC(GetZrefBB,              1,  IFgetZrefBB);
    TCL_FUNC(AdvanceZref,            2,  IFadvanceZref);
    TCL_FUNC(Zhead,                  1,  IFzHead);
    TCL_FUNC(Zvalues,                2,  IFzValues);
    TCL_FUNC(Zlength,                1,  IFzLength);
    TCL_FUNC(Zarea,                  1,  IFzArea);
    TCL_FUNC(GetZlist,               2,  IFgetZlist);
    TCL_FUNC(GetSqZlist,             1,  IFgetSqZlist);
    TCL_FUNC(TransformZ,             5,  IFtransformZ);
    TCL_FUNC(BloatZ,                 3,  IFbloatZ);
    TCL_FUNC(ExtentZ,                1,  IFextentZ);
    TCL_FUNC(EdgesZ,                 3,  IFedgesZ);
    TCL_FUNC(ManhattanizeZ,          3,  IFmanhattanizeZ);
    TCL_FUNC(RepartitionZ,           1,  IFrepartitionZ);
    TCL_FUNC(BoxZ,                   4,  IFboxZ);
    TCL_FUNC(ZoidZ,                  6,  IFzoidZ);
    TCL_FUNC(ObjectZ,                2,  IFobjectZ);
    TCL_FUNC(ParseLayerExpr,         1,  IFparseLayerExpr);
    TCL_FUNC(EvalLayerExpr,          4,  IFevalLayerExpr);
    TCL_FUNC(TestCoverageFull,       3,  IFtestCoverageFull);
    TCL_FUNC(TestCoveragePartial,    3,  IFtestCoveragePartial);
    TCL_FUNC(TestCoverageNone,       3,  IFtestCoverageNone);
    TCL_FUNC(TestCoverage,           3,  IFtestCoverage);
    TCL_FUNC(ZtoObjects,             4,  IFzToObjects);
    TCL_FUNC(ZtoTempLayer,           3,  IFzToTempLayer);
    TCL_FUNC(ClearTempLayer,         1,  IFclearTempLayer);
    TCL_FUNC(ZtoFile,                3,  IFzToFile);
    TCL_FUNC(ZfromFile,              1,  IFzFromFile);
    TCL_FUNC(ReadZfile,              1,  IFreadZfile);
    TCL_FUNC(ChdGetZlist,            6,  IFchdGetZlist);

    // Operations
    TCL_FUNC(NoOp,                   1,  IFnoOp);
    TCL_FUNC(Filt,                   2,  IFfilt);
    TCL_FUNC(GeomAnd,          VARARGS,  IFgeomAnd);
    TCL_FUNC(GeomAndNot,             2,  IFgeomAndNot);
    TCL_FUNC(GeomCat,          VARARGS,  IFgeomCat);
    TCL_FUNC(GeomNot,                1,  IFgeomNot);
    TCL_FUNC(GeomOr,           VARARGS,  IFgeomOr);
    TCL_FUNC(GeomXor,          VARARGS,  IFgeomXor);

    // Spatial Parameter Tables
    TCL_FUNC(ReadSPtable,            1,  IFreadSPtable);
    TCL_FUNC(NewSPtable,             7,  IFnewSPtable);
    TCL_FUNC(WriteSPtable,           2,  IFwriteSPtable);
    TCL_FUNC(ClearSPtable,           1,  IFclearSPtable);
    TCL_FUNC(FindSPtable,            2,  IFfindSPtable);
    TCL_FUNC(GetSPdata,              3,  IFgetSPdata);
    TCL_FUNC(SetSPdata,              4,  IFsetSPdata);

    // Polymorphic Flat Database
    TCL_FUNC(ChdOpenOdb,             6,  IFchdOpenOdb);
    TCL_FUNC(ChdOpenZdb,             6,  IFchdOpenZdb);
    TCL_FUNC(ChdOpenZbdb,            9,  IFchdOpenZbdb);
    TCL_FUNC(GetObjectsOdb,          3,  IFgetObjectsOdb);
    TCL_FUNC(ListLayersDb,           1,  IFlistLayersDb);
    TCL_FUNC(GetZlistDb,             3,  IFgetZlistDb);
    TCL_FUNC(GetZlistZbdb,           4,  IFgetZlistZbdb);
    TCL_FUNC(DestroyDb,              1,  IFdestroyDb);

    // Named String Tables
    TCL_FUNC(FindNameTable,          2,  IFfindNameTable);
    TCL_FUNC(RemoveNameTable,        1,  IFremoveNameTable);
    TCL_FUNC(ListNameTables,         0,  IFlistNameTables);
    TCL_FUNC(ClearNameTables,        0,  IFclearNameTables);
    TCL_FUNC(AddNameToTable,         3,  IFaddNameToTable);
    TCL_FUNC(RemoveNameFromTable,    2,  IFremoveNameFromTable);
    TCL_FUNC(FindNameInTable,        2,  IFfindNameInTable);
    TCL_FUNC(ListNamesInTable,       1,  IFlistNamesInTable);


    void tcl_register_lexpr()
    {
      // Trapezoid Lists and Layer Expressions
      cTclIf::register_func("SetZref",                tclSetZref);
      cTclIf::register_func("GetZref",                tclGetZref);
      cTclIf::register_func("GetZrefBB",              tclGetZrefBB);
      cTclIf::register_func("AdvanceZref",            tclAdvanceZref);
      cTclIf::register_func("Zhead",                  tclZhead);
      cTclIf::register_func("Zvalues",                tclZvalues);
      cTclIf::register_func("Zlength",                tclZlength);
      cTclIf::register_func("Zarea",                  tclZarea);
      cTclIf::register_func("GetZlist",               tclGetZlist);
      cTclIf::register_func("GetSqZlist",             tclGetSqZlist);
      cTclIf::register_func("TransformZ",             tclTransformZ);
      cTclIf::register_func("BloatZ",                 tclBloatZ);
      cTclIf::register_func("ExtentZ",                tclExtentZ);
      cTclIf::register_func("EdgesZ",                 tclEdgesZ);
      cTclIf::register_func("ManhattanizeZ",          tclManhattanizeZ);
      cTclIf::register_func("RepartitionZ",           tclRepartitionZ);
      cTclIf::register_func("BoxZ",                   tclBoxZ);
      cTclIf::register_func("ZoidZ",                  tclZoidZ);
      cTclIf::register_func("ObjectZ",                tclObjectZ);
      cTclIf::register_func("ParseLayerExpr",         tclParseLayerExpr);
      cTclIf::register_func("EvalLayerExpr",          tclEvalLayerExpr);
      cTclIf::register_func("TestCoverageFull",       tclTestCoverageFull);
      cTclIf::register_func("TestCoveragePartial",    tclTestCoveragePartial);
      cTclIf::register_func("TestCoverageNone",       tclTestCoverageNone);
      cTclIf::register_func("TestCoverage",           tclTestCoverage);
      cTclIf::register_func("ZtoObjects",             tclZtoObjects);
      cTclIf::register_func("ZtoTempLayer",           tclZtoTempLayer);
      cTclIf::register_func("ClearTempLayer",         tclClearTempLayer);
      cTclIf::register_func("ZtoFile",                tclZtoFile);
      cTclIf::register_func("ZfromFile",              tclZfromFile);
      cTclIf::register_func("ReadZfile",              tclReadZfile);
      cTclIf::register_func("ChdGetZlist",            tclChdGetZlist);

      // Operations
      cTclIf::register_func("NoOp",                   tclNoOp);
      cTclIf::register_func("Filt",                   tclFilt);
      cTclIf::register_func("GeomAnd",                tclGeomAnd);
      cTclIf::register_func("GeomAndNot",             tclGeomAndNot);
      cTclIf::register_func("GeomCat",                tclGeomCat);
      cTclIf::register_func("GeomNot",                tclGeomNot);
      cTclIf::register_func("GeomOr",                 tclGeomOr);
      cTclIf::register_func("GeomXor",                tclGeomXor);

      // Spatial Parameter Tables
      cTclIf::register_func("ReadSPtable",            tclReadSPtable);
      cTclIf::register_func("NewSPtable",             tclNewSPtable);
      cTclIf::register_func("WriteSPtable",           tclWriteSPtable);
      cTclIf::register_func("ClearSPtable",           tclClearSPtable);
      cTclIf::register_func("FindSPtable",            tclFindSPtable);
      cTclIf::register_func("GetSPdata",              tclGetSPdata);
      cTclIf::register_func("SetSPdata",              tclSetSPdata);

      // Polymorphic Flat Database
      cTclIf::register_func("ChdOpenOdb",             tclChdOpenOdb);
      cTclIf::register_func("ChdOpenZdb",             tclChdOpenZdb);
      cTclIf::register_func("ChdOpenZbdb",            tclChdOpenZbdb);
      cTclIf::register_func("GetObjectsOdb",          tclGetObjectsOdb);
      cTclIf::register_func("ListLayersDb",           tclListLayersDb);
      cTclIf::register_func("GetZlistDb",             tclGetZlistDb);
      cTclIf::register_func("GetZlistZbdb",           tclGetZlistZbdb);
      cTclIf::register_func("DestroyDb",              tclDestroyDb);

      // Named String Tables
      cTclIf::register_func("FindNameTable",          tclFindNameTable);
      cTclIf::register_func("RemoveNameTable",        tclRemoveNameTable);
      cTclIf::register_func("ListNameTables",         tclListNameTables);
      cTclIf::register_func("ClearNameTables",        tclClearNameTables);
      cTclIf::register_func("AddNameToTable",         tclAddNameToTable);
      cTclIf::register_func("RemoveNameFromTable",    tclRemoveNameFromTable);
      cTclIf::register_func("FindNameInTable",        tclFindNameInTable);
      cTclIf::register_func("ListNamesInTable",       tclListNamesInTable);
    }
#endif  // HAVE_TCL
}


// Need to export the sqz() "pseudo" function implementation back
// to the evaluator.
namespace zlist_funcs {
    SIscriptFunc SIlexp_sqz_func = &lexpr_funcs::IFsqzStub;

    // Need these too.
    SIscriptFunc PTbloatZ = &lexpr_funcs::IFbloatZ;
    SIscriptFunc PTedgesZ = &lexpr_funcs::IFedgesZ;
}


// Export to load functions in this script library.
//
void
SIparser::funcs_lexpr_init()
{
  using namespace lexpr_funcs;

  // Layer Expression (Alt) functions
  // The third column contains bit flags that are set for arguments
  // that should be passed as strings, the default is to eval string
  // arguments as Zlists before passing to the function.

  registerAltFunc("bloat",               3,  0, IFbloatZ);
  registerAltFunc("extent",              1,  0, IFextentZ);
  registerAltFunc("edges",               3,  0, IFedgesZ);
  registerAltFunc("manhattanize",        3,  0, IFmanhattanizeZ);
  registerAltFunc("box",                 4,  0, IFboxZ);
  registerAltFunc("zoid",                6,  0, IFzoidZ);
  registerAltFunc("sqz",                 1,  0, IFsqzStub);  // dummy
  registerAltFunc("noop",                1,  1, IFnoOp);
  registerAltFunc("filt",                2,  2, IFfilt);
  registerAltFunc("geomAnd",       VARARGS,  0, IFgeomAnd);
  registerAltFunc("geomAndNot",          2,  0, IFgeomAndNot);
  registerAltFunc("geomCat",       VARARGS,  0, IFgeomCat);
  registerAltFunc("geomNot",             1,  0, IFgeomNot);
  registerAltFunc("geomOr",        VARARGS,  0, IFgeomOr);
  registerAltFunc("geomXor",       VARARGS,  0, IFgeomXor);

  // Trapezoid Lists and Layer Expressions
  registerFunc("SetZref",                1,  IFsetZref);
  registerFunc("GetZref",                0,  IFgetZref);
  registerFunc("GetZrefBB",              1,  IFgetZrefBB);
  registerFunc("AdvanceZref",            2,  IFadvanceZref);
  registerFunc("Zhead",                  1,  IFzHead);
  registerFunc("Zvalues",                2,  IFzValues);
  registerFunc("Zlength",                1,  IFzLength);
  registerFunc("Zarea",                  1,  IFzArea);
  registerFunc("GetZlist",               2,  IFgetZlist);
  registerFunc("GetSqZlist",             1,  IFgetSqZlist);
  registerFunc("TransformZ",             5,  IFtransformZ);
  registerFunc("BloatZ",                 3,  IFbloatZ);
  registerFunc("ExtentZ",                1,  IFextentZ);
  registerFunc("EdgesZ",                 3,  IFedgesZ);
  registerFunc("ManhattanizeZ",          3,  IFmanhattanizeZ);
  registerFunc("RepartitionZ",           1,  IFrepartitionZ);
  registerFunc("BoxZ",                   4,  IFboxZ);
  registerFunc("ZoidZ",                  6,  IFzoidZ);
  registerFunc("ObjectZ",                2,  IFobjectZ);
  registerFunc("ParseLayerExpr",         1,  IFparseLayerExpr);
  registerFunc("EvalLayerExpr",          4,  IFevalLayerExpr);
  registerFunc("TestCoverageFull",       3,  IFtestCoverageFull);
  registerFunc("TestCoveragePartial",    3,  IFtestCoveragePartial);
  registerFunc("TestCoverageNone",       3,  IFtestCoverageNone);
  registerFunc("TestCoverage",           3,  IFtestCoverage);
  registerFunc("ZtoObjects",             4,  IFzToObjects);
  registerFunc("ZtoTempLayer",           3,  IFzToTempLayer);
  registerFunc("ClearTempLayer",         1,  IFclearTempLayer);
  registerFunc("ZtoFile",                3,  IFzToFile);
  registerFunc("ZfromFile",              1,  IFzFromFile);
  registerFunc("ReadZfile",              1,  IFreadZfile);
  registerFunc("ChdGetZlist",            6,  IFchdGetZlist);

  // Operations
  registerFunc("NoOp",                   1,  IFnoOp);
  registerFunc("Filt",                   2,  IFfilt);
  registerFunc("GeomAnd",          VARARGS,  IFgeomAnd);
  registerFunc("GeomAndNot",             2,  IFgeomAndNot);
  registerFunc("GeomCat",          VARARGS,  IFgeomCat);
  registerFunc("GeomNot",                1,  IFgeomNot);
  registerFunc("GeomOr",           VARARGS,  IFgeomOr);
  registerFunc("GeomXor",          VARARGS,  IFgeomXor);

  // Spatial Parameter Tables
  registerFunc("ReadSPtable",            1,  IFreadSPtable);
  registerFunc("NewSPtable",             7,  IFnewSPtable);
  registerFunc("WriteSPtable",           2,  IFwriteSPtable);
  registerFunc("ClearSPtable",           1,  IFclearSPtable);
  registerFunc("FindSPtable",            2,  IFfindSPtable);
  registerFunc("GetSPdata",              3,  IFgetSPdata);
  registerFunc("SetSPdata",              4,  IFsetSPdata);

  // Polymorphic Flat Database
  registerFunc("ChdOpenOdb",             6,  IFchdOpenOdb);
  registerFunc("ChdOpenZdb",             6,  IFchdOpenZdb);
  registerFunc("ChdOpenZbdb",            9,  IFchdOpenZbdb);
  registerFunc("GetObjectsOdb",          3,  IFgetObjectsOdb);
  registerFunc("ListLayersDb",           1,  IFlistLayersDb);
  registerFunc("GetZlistDb",             3,  IFgetZlistDb);
  registerFunc("GetZlistZbdb",           4,  IFgetZlistZbdb);
  registerFunc("DestroyDb",              1,  IFdestroyDb);

  // Named String Tables
  registerFunc("FindNameTable",          2,  IFfindNameTable);
  registerFunc("RemoveNameTable",        1,  IFremoveNameTable);
  registerFunc("ListNameTables",         0,  IFlistNameTables);
  registerFunc("ClearNameTables",        0,  IFclearNameTables);
  registerFunc("AddNameToTable",         3,  IFaddNameToTable);
  registerFunc("RemoveNameFromTable",    2,  IFremoveNameFromTable);
  registerFunc("FindNameInTable",        2,  IFfindNameInTable);
  registerFunc("ListNamesInTable",       1,  IFlistNamesInTable);

#ifdef HAVE_PYTHON
  py_register_lexpr();
#endif
#ifdef HAVE_TCL
  tcl_register_lexpr();
#endif
}

#define FREE_CHK_ZL(b, zl) { if (b) zl->free(); zl = 0; }
#define FREE_CHK_LS(b, ls) { if (b) delete ls; ls = 0; }


// Export.
// Handle zoidlist arguments.  Overrides:
// TYP_SCALAR:  if boolean true, return the reference zlist.
//              if boolean false, return empty list.
// TYP_STRING:  parse and evaluate string as layer expression, return result.
// TYP_LEXPR:   evaluate the expression, return result.
//
XIrt
arg_zlist(const Variable *args, int indx, Zlist **zlp, bool *orig,
    void *datap)
{
    if (args[indx].type == TYP_ZLIST) {
        *zlp = args[indx].content.zlist;
        *orig = false;
        return (XIok);
    }
    SIlexprCx *cx = (SIlexprCx*)datap;
    if (!cx)
        cx = SI()->LexprCx();
    if (args[indx].type == TYP_SCALAR) {
        if (to_boolean(args[indx].content.value)) {
            *zlp = cx->getZref()->copy();
            *orig = true;
        }
        else {
            *zlp = 0;
            *orig = false;
        }
        return (XIok);
    }
    if (args[indx].type == TYP_LEXPR) {
        sLspec *lspec = args[indx].content.lspec;
        if (!lspec)
            return (XIbad);
        XIrt ret = lspec->getZlist(cx, zlp);
        if (ret != XIok)
            return (ret);
        *orig = true;
        return (XIok);
    }
    if (args[indx].type == TYP_STRING || args[indx].type == TYP_NOTYPE) {
        const char *str = args[indx].content.string;
        if (!str || !*str)
            return (XIbad);
        sLspec lspec;
        if (!lspec.parseExpr(&str) || !lspec.setup())
            return (XIbad);
        XIrt ret = lspec.getZlist(cx, zlp);
        if (ret != XIok)
            return (ret);
        *orig = true;
        return (XIok);
    }
    return (XIbad);
}


// Export.
// Layer expression, accept string and set *orig, or TYP_LEXPR.
//
bool
arg_lexpr(const Variable *args, int indx, sLspec **lspec, bool *orig)
{
    if (args[indx].type == TYP_LEXPR) {
        *lspec = args[indx].content.lspec;
        if (!*lspec)
            return (false);
        *orig = false;
        return (true);
    }
    if (args[indx].type == TYP_STRING || args[indx].type == TYP_NOTYPE) {
        const char *str = args[indx].content.string;
        if (!str || !*str)
            return (false);
        sLspec *ls = new sLspec;
        if (!ls->parseExpr(&str) || !ls->setup()) {
            delete ls;
            return (false);
        }
        *lspec = ls;
        *orig = true;
        return (true);
    }
    return (false);
}


// Export
// Called when bad argument passed to function.
//
void
arg_record_error(int argnum)
{
    Errs()->add_error("bad argument %d.", argnum+1);
}


// -------------------------------------------------------------------------
// Trapezoid Lists and Layer Expressions
// -------------------------------------------------------------------------

// (int) SetZref(thing)
//
// This function sets the reference zoidlist.  The reference zoidlist
// represents the current "background" needed by some functions and
// operators which manipulate zoidlists.  For example, when a zoidlist
// is polarity inverted, the reference zoidlist specifies the boundary
// of the inversion, i.e., the inverse of an empty zoidlist would be
// the reference zoidlist.
//
// The reference zoidlist can be set from various types of object
// passed as the argument.  This can be a zoidlist, or an object
// handle, or an array of size 4 or larger, which contains rectangle
// coordinates in microns in order left, bottom, right, top.  The
// argument can also be the constant 0, in which case the reference
// zoid list will be the boundary of the physical current cell, or a
// large "infinity" box if there is no current cell.  This is the
// default if no reference zoid list is given.
//
// This function will return 1 and fails only if the argument is not
// an appropriate type.
//
bool
lexpr_funcs::IFsetZref(Variable *res, Variable *args, void *datap)
{
    Zlist *zl = 0;
    if (args[0].type == TYP_ZLIST)
        zl = args[0].content.zlist->copy();
    else if (args[0].type == TYP_ARRAY) {
        double *vals;
        if (!arg_array(args, 0, &vals, 4))
            return (BAD);
        BBox BB(INTERNAL_UNITS(vals[0]), INTERNAL_UNITS(vals[1]),
            INTERNAL_UNITS(vals[2]), INTERNAL_UNITS(vals[3]));
        BB.fix();
        zl = new Zlist(&BB);
    }
    else if (args[0].type == TYP_HANDLE) {
        int id = (int)args[0].content.value;
        if (id) {
            sHdl *hdl = sHdl::get(id);
            if (!hdl || hdl->type != HDLobject)
                return (BAD);
            CDol *ol = (CDol*)hdl->data;
            if (ol)
                zl = ol->odesc->toZlist();
        }
    }
    else if (!(args[0].type == TYP_SCALAR &&
            !to_boolean(args[0].content.value)))
        return (BAD);

    SIlexprCx *cx = (SIlexprCx*)datap;
    if (!cx)
        cx = SI()->LexprCx();
    cx->setZrefSaved(zl);

    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (zoidlist) GetZref()
//
// This function returns the current reference zoidlist, which will be
// empty if no reference area has been set with SetZref or otherwise.
//
bool
lexpr_funcs::IFgetZref(Variable *res, Variable*, void *datap)
{
    SIlexprCx *cx = (SIlexprCx*)datap;
    if (!cx)
        cx = SI()->LexprCx();
    res->type = TYP_ZLIST;
    res->content.zlist = cx->refZlist()->copy();
    return (OK);
}


// (int) GetZrefBB(array)
//
// This will return the bounding box of the reference zoidlist, as
// returned from GetZref.  If the reference zoidlist is empty, the
// bounding box of the current cell is returned.  The coordinates are
// in microns, in order left, bottom, right, top.  On success, the
// function returns 1.  If there is no reference zoidlist or current
// cell, 0 is returned.
//
bool
lexpr_funcs::IFgetZrefBB(Variable *res, Variable *args, void *datap)
{
    double *vals;
    ARG_CHK(arg_array(args, 0, &vals, 4))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    SIlexprCx *cx = (SIlexprCx*)datap;
    if (!cx)
        cx = SI()->LexprCx();
    if (cx->refZlist()) {
        BBox BB;
        cx->refZlist()->BB(BB);
        vals[0] = MICRONS(BB.left);
        vals[1] = MICRONS(BB.bottom);
        vals[2] = MICRONS(BB.right);
        vals[3] = MICRONS(BB.top);
        res->content.value = 1.0;
    }
    else if (SIparse()->ifGetCurPhysCell()) {
        const BBox *BB = SIparse()->ifGetCurPhysCell()->BB();
        vals[0] = MICRONS(BB->left);
        vals[1] = MICRONS(BB->bottom);
        vals[2] = MICRONS(BB->right);
        vals[3] = MICRONS(BB->top);
        res->content.value = 1.0;
    }
    return (OK);
}


// (int) AdvanceZref(clear, array)
//
// This function allows iteration over a given area by establishing a
// grid over the area and incrementally setting the reference area
// (see SetZref) to elements of the grid.  The grid is aligned from
// the lower-left corner of the given area and iteration advances
// right and up.  The reference area is set to the intersection of the
// grid element area and the given area.  The size of the square grid
// elements is given by the PartitionSize variable, or defaults to 100
// microns if this variable is not set.
//
// The second argument is an array of size 4 or larger, or 0.  If 0,
// the given area is taken to be the bounding box of the current cell. 
// Otherwise, the array elements define the given rectangular area, in
// microns, in order left, bottom, right, top.
//
// With the boolean first argument set to zero, the function will set
// the reference area to the first (lower left) or next grid element
// intersection area and return 1.  The function will return zero when
// it advances past the last grid element that overlaps the given
// area, at which time the reference area is returned to the default
// value.  Thus, this function can be used in a loop to limit the
// computation area for each iteration, for large cells that would be
// inefficient to process in one step.
//
// If the first argument is nonzero, the internal state is cleared. 
// This should be called if the iteration is not complete and one
// wishes to start a new loop.
//
bool
lexpr_funcs::IFadvanceZref(Variable *res, Variable *args, void *datap)
{
    bool clear;
    ARG_CHK(arg_boolean(args, 0, &clear))

    double *vals;
    ARG_CHK(arg_array_if(args, 1, &vals, 4))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    SIlexprCx *cx = (SIlexprCx*)datap;
    if (!cx)
        cx = SI()->LexprCx();
    if (clear) {
        cx->clearGridCx();
        return (OK);
    }
    if (vals) {
        if (!cx->gridCx()) {
            BBox BB(INTERNAL_UNITS(vals[0]), INTERNAL_UNITS(vals[1]),
                INTERNAL_UNITS(vals[2]), INTERNAL_UNITS(vals[3]));
            BB.fix();
            cx->setGridCx(new grd_t(&BB, grd_t::def_gridsize()));
        }
    }
    else if (SIparse()->ifGetCurPhysCell()) {
        if (!cx->gridCx())
            cx->setGridCx(new grd_t(SIparse()->ifGetCurPhysCell()->BB(),
                grd_t::def_gridsize()));
    }
    if (cx->gridCx()) {
        const BBox *gBB = cx->gridCx()->advance();
        if (gBB) {
            res->content.value = 1;
            cx->setZref(gBB);
        }
        else
            cx->setZref((const Zlist*)0);
    }
    return (OK);
}


// (zoidlist) Zhead(zoidlist)
//
// This function will remove the first trapezoid from the passed
// trapezoid list, and return it as a new list.  If the passed list is
// empty, the returned list will be empty.  If the passed list
// contains a single trapezoid, it will become empty.
//
bool
lexpr_funcs::IFzHead(Variable *res, Variable *args, void *datap)
{
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 0, &zl, &free_zl, datap);
    if (ret != XIok) {
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }

    res->type = TYP_ZLIST;
    args[0].content.zlist = 0;
    if (zl) {
        res->content.zlist = zl;
        if (args[0].type == TYP_ZLIST) {
            args[0].content.zlist = zl->next;
            zl->next = 0;
        }
        else if (free_zl) {
            zl->next->free();
            zl->next = 0;
        }
        else {
            // can't get here
            zl->next = 0;
            res->content.zlist = zl->copy();
        }
    }
    return (OK);
}


// (int) Zvalues(zoidlist, array)
//
// This function will return the coordinates of the first trapezoid in
// the list in the array, which must have size 6 or larger.  The order
// of the values is
//
//  0 x lower-left
//  1 x lower-right
//  2 y lower
//  3 x upper-left
//  4 x upper-right
//  5 y upper
//
// On succewss, 1 is returned.  If the passed trapezoid list is empty,
// the return value is 0 and the array is untouched.
//
bool
lexpr_funcs::IFzValues(Variable *res, Variable *args, void *datap)
{
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 0, &zl, &free_zl, datap);
    if (ret != XIok) {
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    double *vals;
    if (!arg_array(args, 1, &vals, 6)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (zl) {
        vals[0] = MICRONS(zl->Z.xll);
        vals[1] = MICRONS(zl->Z.xlr);
        vals[2] = MICRONS(zl->Z.yl);
        vals[3] = MICRONS(zl->Z.xul);
        vals[4] = MICRONS(zl->Z.xur);
        vals[5] = MICRONS(zl->Z.yu);
        res->content.value = 1;
        FREE_CHK_ZL(free_zl, zl)
    }
    return (OK);
}


// (int) Zlength(zoidlist)
//
// This function returns the number of trapezoids contained in the
// list passed as an argument.
//
bool
lexpr_funcs::IFzLength(Variable *res, Variable *args, void *datap)
{
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 0, &zl, &free_zl, datap);
    if (ret != XIok) {
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        SI()->SetInterrupt();
        return (OK);
    }

    res->type = TYP_SCALAR;
    int cnt = 0;
    for (Zlist *z = zl; z; z = z->next)
        cnt++;
    res->content.value = cnt;
    FREE_CHK_ZL(free_zl, zl)
    return (OK);
}


// (real) Zarea(zoidlist)
//
// This function returns the total area of the trapezoids contained in
// the list passed as an argument.  This does not account for
// overlapping trapezoids, call GeomOR first if overlapping trapezoids
// are present (lists returned from the script functions have already
// been clipped/merged unless otherwise noted).
//
bool
lexpr_funcs::IFzArea(Variable *res, Variable *args, void *datap)
{
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 0, &zl, &free_zl, datap);
    if (ret != XIok) {
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        SI()->SetInterrupt();
        return (OK);
    }

    res->type = TYP_SCALAR;
    res->content.value = zl->area();
    FREE_CHK_ZL(free_zl, zl)
    return (OK);
}


// (zoidlist) GetZlist(layersrc, depth)
//
// This function returns a zoidlist from the layer source given in the
// first argument, which is a string in the form
//
//   lname[.stname][.cname]
//
// Any of lname, stname, cname can be double-quoted, which must be
// true if the token contains the separation char '.'.  The stname is
// the name of a symbol table, the cname is tha name of a cell found
// in the symbol table.  If there are only two fields, the second
// field is cname, and the current symbol table is understood.  If no
// cname is given, the current cell is understood.
//
// The returned list is clipped to the current reference area (see
// SetZref).  The second argument is the hierarchy depth to search,
// which can be a non-negative integer or a string starting with 'a'
// to indicate "all".  If not called in physical mode, an empty list
// is returned.
//
// The layer specification can also be given in the form
//
//   lname.@dbname
//
// where dbname is the name of a saved database.  Operation will be
// similar to the GetZlistDb script function.
//
bool
lexpr_funcs::IFgetZlist(Variable *res, Variable *args, void *datap)
{
    const char *layersrc;
    ARG_CHK(arg_string(args, 0, &layersrc))
    int depth;
    ARG_CHK(arg_depth(args, 1, &depth))

    if (!layersrc || !*layersrc)
        return (BAD);
    res->type = TYP_ZLIST;
    res->content.zlist = 0;
    if (SIparse()->ifGetCurMode() == Physical) {
        LDorig *ldorig = new LDorig(layersrc);
        if (!ldorig->ldesc()) {
            delete ldorig;
            return (BAD);
        }
        SIlexprCx *cx = (SIlexprCx*)datap;
        if (!cx)
            cx = SI()->LexprCx();
        int td = cx->hierDepth();
        cx->setHierDepth(depth);
        XIrt ret = cx->getZlist(ldorig, &res->content.zlist);
        cx->setHierDepth(td);
        delete ldorig;
        if (ret == XIintr) {
            SI()->SetInterrupt();
            return (OK);
        }
        if (ret == XIbad)
            return (BAD);
    }
    return (OK);
}


// (zoidlist) GetSqZlist(layername)
//
// This function returns a trapezoid list derived from objects in the
// selection queue on the layer whose name is passed as the argument. 
// Labels are ignored, as are subcells unless the layer name is the
// special name "$$", in which case the subcell bounding boxes are
// returned.
//
// This function can be called successfully only in physical mode.
// 
bool
lexpr_funcs::IFgetSqZlist(Variable *res, Variable *args, void *datap)
{
    const char *layername;
    ARG_CHK(arg_string(args, 0, &layername))

    if (!layername || !*layername)
        return (BAD);
    res->type = TYP_ZLIST;
    res->content.zlist = 0;
    if (SIparse()->ifGetCurMode() == Physical) {
        LDorig *ldorig = new LDorig(layername);
        if (!ldorig->ldesc()) {
            delete ldorig;
            return (BAD);
        }
        if (ldorig->cellname() || ldorig->stab_name()) {
            // Incompatible with obtaining from selection list.  We'll
            // call this an error.
            delete ldorig;
            return (BAD);
        }
        SIlexprCx *cx = (SIlexprCx*)datap;
        if (!cx)
            cx = SI()->LexprCx();
        bool tmp = cx->setSourceSelQueue(true);
        XIrt ret = cx->getZlist(ldorig, &res->content.zlist);
        cx->setSourceSelQueue(tmp);
        delete ldorig;
        if (ret == XIintr) {
            SI()->SetInterrupt();
            return (OK);
        }
        if (ret == XIbad)
            return (BAD);
    }
    return (OK);
}


// (zoidlist) TransformZ(zoid_list, refx, refy, newx, newy)
//
// Return a transformed copy of the passed trapezoid list.  The
// transform should have been set previously with SetTransform or
// equivalent.  The original list is not touched and can be closed if
// no longer needed.  The function internally converts each input
// trapezoid to a polygon, applies the transformation to the polygon
// coordinates, then decomposes the polygons into a new trapezoid
// list, which is returned.
//
// The remaining arguments are "reference" and "new" coordinates,
// which provide for translations.  The reference point is the point
// about which rotations and mirroring are performed, and is
// translated to the new location, if different.
//
bool
lexpr_funcs::IFtransformZ(Variable *res, Variable *args, void *datap)
{
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 0, &zl, &free_zl, datap);
    if (ret == XIbad)
        return (BAD);
    if (ret == XIintr) {
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    int refx;
    if (!arg_coord(args, 1, &refx, Physical)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }
    int refy;
    if (!arg_coord(args, 2, &refy, Physical)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }
    int newx;
    if (!arg_coord(args, 3, &newx, Physical)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }
    int newy;
    if (!arg_coord(args, 4, &newy, Physical)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }

    res->type = TYP_ZLIST;
    cTfmStack stk;
    stk.TPush();
    GEO()->applyCurTransform(&stk, refx, refy, newx, newy);
    res->content.zlist = zl->transform(&stk);
    stk.TPop();
    FREE_CHK_ZL(free_zl, zl)
    return (OK);
}


// (zoidlist) BloatZ(dimen, zoid_list, mode)
//
// This function returns a new zoidlist which is a bloated version of
// the zoidlist passed as an argument (similar to the !bloat command).
// Edges will be pushed outward or pulled inward by dimen (positive
// values push outward).  The dimen is given in microns.
//
// The third argument is an integer that specifies the algorithm to
// use for bloating.  Giving zero specifies the default algorithm,
// which rounds corners but is rather complex and slow.  See the
// description of the !bloat command for documentation of the
// algorithms available.
//
bool
lexpr_funcs::IFbloatZ(Variable *res, Variable *args, void *datap)
{
    int dimen;
    ARG_CHK(arg_coord(args, 0, &dimen, Physical))
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 1, &zl, &free_zl, datap);
    if (ret == XIbad)
        return (BAD);
    if (ret == XIintr) {
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    int mode;
    if (!arg_int(args, 2, &mode)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }

    if (!free_zl)
        zl = zl->copy();
    ret = Zlist::zl_bloat(&zl, dimen, mode);
    if (ret == XIbad)
        return (BAD);
    else if (ret == XIintr) {
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    res->type = TYP_ZLIST;
    res->content.zlist = zl;
    return (OK);
}


// (zoidlist) ExtentZ(zoidlist)
//
// This will return a zoidlist with at most one component:  a
// rectangle giving the bounding box of the list given as an argument. 
// If the passed list is null, the return is a null list.
//
bool
lexpr_funcs::IFextentZ(Variable *res, Variable *args, void *datap)
{
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 0, &zl, &free_zl, datap);
    if (ret == XIbad)
        return (BAD);
    if (ret == XIintr) {
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    if (!free_zl)
        zl = zl->copy();
    res->type = TYP_ZLIST;
    res->content.zlist = 0;
    if (zl) {
        BBox BB;
        zl->BB(BB);
        zl->next->free();
        zl->next = 0;
        zl->Z.yl = BB.bottom;
        zl->Z.xll = zl->Z.xul = BB.left;
        zl->Z.xlr = zl->Z.xur = BB.right;
        zl->Z.yu = BB.top;
        res->content.zlist = zl;
    }
    return (OK);
}


// (zoidlist) EdgesZ(dimen, zoid_list, mode)
//
// This returns a list of zoids that in some way describe edges in the
// zoid list passed.  The dimen is given in microns.
//
// The mode is an integer which specifies the algorithm to use to
// define the edges.  The values 0-3 are equivalent to the BloatZ
// function returning edges only, with the four corner fill-in modes.
//
// mode 0
// Provides an edge template as from the BloatZ function with corner
// fill-in mode 0 (rounded corners).
//
// mode 1
// Provides an edge template as from the BloatZ function with corner
// fill-in mode 1 (flat corners).
//
// mode 2
// Provides an edge template as from the BloatZ function with corner
// fill-in mode 2 (projected corners).
//
// mode 3
// Provides an edge template as from the BloatZ function with corner
// fill-in mode 3 (no corner fill).
//
// mode 4
// The zoid list is logically merged into distinct polygons, and a
// "halo" extending outside of the polygon by width dimen (positive
// value taken) is constructed.  The trapezoids describing the halo
// are returned.
//
// mode 5
// The zoid list is logically merged into distinct polygons, and a
// wire object is constructed using each polygon vertex list.  The
// wire width is twice the dimen value passed.  The trapezoid list
// representing the wire area is returned.  This will fail and give
// strange shapes if the dimensions of a polygon are smaller than the
// wire width.
//
// mode 6
// For each zoid in the zoid_list argument, a new zoid is constructed
// from each edge that covers the area within +/- dimen normal to the
// edge.  The list of new zoids is returned.
//
bool
lexpr_funcs::IFedgesZ(Variable *res, Variable *args, void *datap)
{
    int dimen;
    ARG_CHK(arg_coord(args, 0, &dimen, Physical))
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 1, &zl, &free_zl, datap);
    if (ret == XIbad)
        return (BAD);
    if (ret == XIintr) {
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    int mode;
    if (!arg_int(args, 2, &mode)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }

    res->type = TYP_ZLIST;
    try {
        if (mode >= 0 && mode <= 3)
            res->content.zlist = zl->bloat(dimen,
                BL_EDGE_ONLY | (mode << BL_CORNER_MODE_SHIFT));
        if (mode == 4)
            res->content.zlist = zl->halo(dimen);
        else if (mode == 5)
            res->content.zlist = zl->wire_edges(dimen);
        else if (mode == 6)
            res->content.zlist = zl->edges(dimen);
        else
            res->content.zlist = zl->bloat(dimen, BL_EDGE_ONLY);
        FREE_CHK_ZL(free_zl, zl)
    }
    catch (XIrt tmpret) {
        res->content.zlist = 0;
        FREE_CHK_ZL(free_zl, zl)
        if (tmpret == XIintr)
            SI()->SetInterrupt();
        else
            return (BAD);
    }
    return (OK);
}


// (zoidlist) ManhattanizeZ(dimen, zoid_list, mode)
//
// This function returns a new zoidlist which is a Manhattan
// approximation of the zoidlist passed as an argument (similar to the
// !manh command).  The first argument is the minimum rectangle width
// or height in microns used to approximate non-Manhattan pieces.  The
// third argument is a boolean which specifies which of the two
// algorithms to employ.  These algorithms are described with the
// !manh command, though in this function there is no reassembly into
// polygons.
//
// All of the returned trapezoids are rectangles.  The function will
// fail if the argument is smaller than 0.01.
//
bool
lexpr_funcs::IFmanhattanizeZ(Variable *res, Variable *args, void *datap)
{
    int dimen;
    ARG_CHK(arg_coord(args, 0, &dimen, Physical))
    if (dimen < 10)
        return (BAD);
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 1, &zl, &free_zl, datap);
    if (ret == XIbad)
        return (BAD);
    if (ret == XIintr) {
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    int mode;
    if (!arg_int(args, 2, &mode)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }
    if (!free_zl)
        zl = zl->copy();

    res->type = TYP_ZLIST;
    res->content.zlist = zl->manhattanize(dimen, mode);
    return (OK);
}


// (zoidlist) RepartitionZ(zoid_list)
//
// This is a rather obscure function that conditions a list of
// trapezoids so that the area covered will be constructed with
// trapezoids that are as long (horizontally) as possible.  Logically,
// this is what would happen if the initial trapezoid list was
// converted to distinct polygons, then split back into trapezoids.
//
bool
lexpr_funcs::IFrepartitionZ(Variable *res, Variable *args, void *datap)
{
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 0, &zl, &free_zl, datap);
    if (ret == XIbad)
        return (BAD);
    if (ret == XIintr) {
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    if (!free_zl)
        zl = zl->copy();
    res->type = TYP_ZLIST;
    try {
        res->content.zlist = zl->repartition();
    }
    catch (XIrt tmpret) {
        res->content.zlist = 0;
        if (tmpret == XIintr)
            SI()->SetInterrupt();
        else
            return (BAD);
    }
    return (OK);
}


// (zoidlist) BoxZ(l, b, r, t)
//
// This function returns a zoidlist containing a single trapezoid
// which represents the box given in the arguments.  The given
// coordinates are in microns.  This function never fails.
//
bool
lexpr_funcs::IFboxZ(Variable *res, Variable *args, void*)
{
    BBox BB;
    ARG_CHK(arg_coord(args, 0, &BB.left, Physical))
    ARG_CHK(arg_coord(args, 1, &BB.bottom, Physical))
    ARG_CHK(arg_coord(args, 2, &BB.right, Physical))
    ARG_CHK(arg_coord(args, 3, &BB.top, Physical))

    BB.fix();
    Zoid Z(&BB);

    res->content.zlist = new Zlist(&Z);
    res->type = TYP_ZLIST;
    return (OK);
}


// (zoidlist) ZoidZ(xll, xlr, yl, xul, xur, yu)
//
// This function returns a zoidlist containing a single horizontal
// trapezoid which represents the horizontal trapezoid given in the
// arguments.  The six numbers must represent a non-degenerate figure
// or the function will fail.  The given coordinates are in microns.
//
bool
lexpr_funcs::IFzoidZ(Variable *res, Variable *args, void*)
{
    Zoid Z;
    ARG_CHK(arg_coord(args, 0, &Z.xll, Physical))
    ARG_CHK(arg_coord(args, 1, &Z.xlr, Physical))
    ARG_CHK(arg_coord(args, 2, &Z.yl, Physical))
    ARG_CHK(arg_coord(args, 3, &Z.xul, Physical))
    ARG_CHK(arg_coord(args, 4, &Z.xur, Physical))
    ARG_CHK(arg_coord(args, 5, &Z.yu, Physical))

    if (Z.yu < Z.yl) {
        int tmp = Z.yu;
        Z.yu = Z.yl;
        Z.yl = tmp;
        tmp = Z.xul;
        Z.xul = Z.xll;
        Z.xll = tmp;
        tmp = Z.xur;
        Z.xur = Z.xlr;
        Z.xlr = tmp;
    }
    if (Z.xlr < Z.xll) {
        int tmp = Z.xlr;
        Z.xlr = Z.xll;
        Z.xll = tmp;
        tmp = Z.xur;
        Z.xur = Z.xul;
        Z.xul = tmp;
    }
    if (Z.xur < Z.xul)
        return (BAD);

    res->content.zlist = new Zlist(&Z);
    res->type = TYP_ZLIST;
    return (OK);
}


// (zoidlist) ObjectZ(object_handle, all)
//
// This function returns a zoidlist which is generated by fracturing
// the outlines of the objects in the object_handle.  If all is 0,
// only the first object in the list is used.  If all is nonzero, all
// objects in the list are used.  This function will fail if the first
// argument is not a handle to an object list.
//
bool
lexpr_funcs::IFobjectZ(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))

    sHdl *hdl = sHdl::get(id);
    if (!hdl || hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    Zlist *z0 = 0, *ze = 0;
    while (ol) {
        Zlist *zx = ol->odesc->toZlist();
        if (!z0)
            z0 = ze = zx;
        else {
            while (ze->next)
                ze = ze->next;
            ze->next = zx;
        }
        if (!all)
            break;
        ol = ol->next;
    }
    if (all) {
        try {
            z0 = z0->repartition();
        }
        catch (XIrt ret) {
            if (ret == XIintr) {
                SI()->SetInterrupt();
                return (OK);
            }
            else
                return (BAD);
        }
    }
    res->type = TYP_ZLIST;
    res->content.zlist = z0;
    return (OK);
}


// (layerexpr) ParseLayerExpr(string)
//
// This function returns a variable which contains a parse tree for a
// layer expression contained in the string passed as an argument.
// The resulting variable is used to rapidly evaluate the layer
// expression within the boundaries of the current reference zoidlist.
// The return value can not be assigned or otherwise maipulated, and
// can only be passed to functions that expect this variable type.
// The function will fail on a parse error in the layer expression.
//
bool
lexpr_funcs::IFparseLayerExpr(Variable *res, Variable *args, void*)
{
    const char *expr;
    ARG_CHK(arg_string(args, 0, &expr))

    if (!expr || !*expr)
        return (BAD);
    res->type = TYP_LEXPR;
    sLspec *lspec = new sLspec;
    if (lspec->parseExpr(&expr) && lspec->setup()) {
        res->content.lspec = lspec;
        return (OK);
    }
    delete lspec;
    return (BAD);
}


// (zoidlist) EvalLayerExpr(layer_expr, zoid_list, depth, isclear)
//
// This function evaluates the layer expression passed as the first
// argument.  The first argument can be a string containing the layer
// expression, or a return from ParseLayerExpression().  If the second
// argument is nonzero, it is taken as a reference zoidlist.  If 0,
// the current reference zoidlist (as set with SetZref()) will be
// used.  The third argument is the depth into the cell hierarchy to
// process.  This can be an integer, with 0 representing the current
// cell only, or a string starting with 'a' to indicate use of all
// levels of the hierarchy.  If isclear is 0, the returned zoidlist
// will represent all areas within the reference where the layer
// expression is "true".  if isclear is nonzero, the complement
// regions will be returned.  The function will fail on a parse or
// evaluation error.
//
bool
lexpr_funcs::IFevalLayerExpr(Variable *res, Variable *args, void *datap)
{
    if (!SIparse()->ifGetCurPhysCell())
        return (BAD);
    bool free_lspec;
    sLspec *lspec;
    if (!arg_lexpr(args, 0, &lspec, &free_lspec))
        return (BAD);
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 1, &zl, &free_zl, datap);
    if (ret != XIok) {
        FREE_CHK_LS(free_lspec, lspec)
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    int depth;
    if (!arg_depth(args, 2, &depth)) {
        FREE_CHK_LS(free_lspec, lspec)
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }
    bool isclear;
    if (!arg_boolean(args, 3, &isclear)) {
        FREE_CHK_LS(free_lspec, lspec)
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }

    SIlexprCx *cx = (SIlexprCx*)datap;
    const Zlist *zlc = zl;
    if (!zlc)
        zlc = cx->getZref();

    const Zlist *tzref = cx->getZref();
    int tdp = cx->hierDepth();
    cx->setZref(zlc);
    cx->setHierDepth(depth);
    Zlist *zret;
    ret = lspec->getZlist(cx, &zret, isclear);
    cx->setHierDepth(tdp);
    cx->setZref(tzref);

    FREE_CHK_ZL(free_zl, zl)
    FREE_CHK_LS(free_lspec, lspec)
    if (ret == XIbad)
        return (BAD);
    if (ret == XIintr) {
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }

    res->type = TYP_ZLIST;
    res->content.zlist = zret;
    return (OK);
}

// (int) TestCoverageFull(layer_expr, zoid_list, minsize)
//
// This function will return an integer value indicating the coverage
// of the layer expression given in the first argument over the
// regions described in the second argument.  The first argument can
// be a string containing a layer expression, or a return from
// ParseLayerExpression().  If the second argument is 0, the current
// reference zoidlist as set with SetZref() is assumed.  This defaults
// to tha area of the current cell.
//
// The third argument is an integer which gives the minimum dimension
// in internal units of trapezoids which will be considered in the
// result.  Sub-dimensional trapezoids are ignored.  This minimizes
// false-positive tests due to "slivers" caused by clipping errors in
// non-Manhattan geometry.  If the geomentry is known to be Manhattan,
// 0 can be used.  If 45's only, 2 is recommended, otherwise 4. 
// Negative values are taken as zero.
//
// The function tests each dark-area trapezoid from the layer
// expression against the reference zoid list.  It will return
// immediately on the first such zoid that is not fully covered by the
// reference zoid list.
//
// The return value is 0 if there was only one trapezoid from the
// layer expression, and it did not overlap the reference zoid list. 
// Otherwise, if all layer expression trapezoids were covered by the
// reference zoid list, 2 is returned, or 1 if not.  Note that 1 will
// be returned if there is no intersection and more than one layer
// expression trapezoid.  Use TestCoveragePartial to fully distinguish
// the not-full case.  The present function is most efficient for
// determining when the layer expression dark area is or is not fully
// covered.
//
bool
lexpr_funcs::IFtestCoverageFull(Variable *res, Variable *args, void *datap)
{
    bool free_lspec;
    sLspec *lspec;
    if (!arg_lexpr(args, 0, &lspec, &free_lspec))
        return (BAD);
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 1, &zl, &free_zl, datap);
    if (ret != XIok) {
        FREE_CHK_LS(free_lspec, lspec)
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    int minsz;
    if (!arg_int(args, 2, &minsz)) {
        FREE_CHK_LS(free_lspec, lspec)
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }
    if (minsz < 0)
        minsz = 0;

    SIlexprCx *cx = (SIlexprCx*)datap;
    if (!cx)
        cx = SI()->LexprCx();
    const Zlist *zlc = zl;
    if (!zlc)
        zlc = cx->getZref();

    const Zlist *tzref = cx->getZref();
    int tdp = cx->hierDepth();
    cx->setZref(zlc);
    cx->setHierDepth(CDMAXCALLDEPTH);
    CovType cov;
    ret = lspec->testZlistCovFull(cx, &cov, minsz);
    cx->setHierDepth(tdp);
    cx->setZref(tzref);

    FREE_CHK_LS(free_lspec, lspec)
    FREE_CHK_ZL(free_zl, zl)
    if (ret == XIbad)
        return (BAD);
    if (ret == XIintr) {
        res->type = TYP_SCALAR;
        res->content.value = 0;
        SI()->SetInterrupt();
        return (OK);
    }

    res->type = TYP_SCALAR;
    res->content.value = cov;
    return (OK);
}


// (int) TestCoveragePartial(layer_expr, zoid_list, minsize)
//
// This function will return an integer value indicating the coverage
// of the layer expression given in the first argument over the
// regions described in the second argument.  The first argument can
// be a string containing a layer expression, or a return from
// ParseLayerExpression().  If the second argument is 0, the current
// reference zoidlist as set with SetZref() is assumed.  This defaults
// to tha area of the current cell.
//
// The third argument is an integer which gives the minimum dimension
// in internal units of trapezoids which will be considered in the
// result.  Sub-dimensional trapezoids are ignored.  This minimizes
// false-positive tests due to "slivers" caused by clipping errors in
// non-Manhattan geometry.  If the geomentry is known to be Manhattan,
// 0 can be used.  If 45's only, 2 is recommended, otherwise 4. 
// Negative values are taken as zero.
//
// The function tests each dark-area trapezoid from the layer
// expression against the reference zoid list.  It will return
// immediately on the first such zoid that is partially covered by the
// reference zoid list, of after finding both a fully covered zoid and
// a fully uncovered zoid.
//
// The return value is 0 if there is no dark area from the layer
// expression that intersects the reference zoid list, 2 if the layer
// expression dark area falls entirely in the reference zoid list, and
// 1 if coverage is partial.  This test is a bit expensive but provides
// definitive results,
//
bool
lexpr_funcs::IFtestCoveragePartial(Variable *res, Variable *args, void *datap)
{
    bool free_lspec;
    sLspec *lspec;
    if (!arg_lexpr(args, 0, &lspec, &free_lspec))
        return (BAD);
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 1, &zl, &free_zl, datap);
    if (ret != XIok) {
        FREE_CHK_LS(free_lspec, lspec)
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    int minsz;
    if (!arg_int(args, 2, &minsz)) {
        FREE_CHK_LS(free_lspec, lspec)
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }
    if (minsz < 0)
        minsz = 0;

    SIlexprCx *cx = (SIlexprCx*)datap;
    if (!cx)
        cx = SI()->LexprCx();
    const Zlist *zlc = zl;
    if (!zlc)
        zlc = cx->getZref();

    const Zlist *tzref = cx->getZref();
    int tdp = cx->hierDepth();
    cx->setZref(zlc);
    cx->setHierDepth(CDMAXCALLDEPTH);
    CovType cov;
    ret = lspec->testZlistCovPartial(cx, &cov, minsz);
    cx->setHierDepth(tdp);
    cx->setZref(tzref);

    FREE_CHK_LS(free_lspec, lspec)
    FREE_CHK_ZL(free_zl, zl)
    if (ret == XIbad)
        return (BAD);
    if (ret == XIintr) {
        res->type = TYP_SCALAR;
        res->content.value = 0;
        SI()->SetInterrupt();
        return (OK);
    }

    res->type = TYP_SCALAR;
    res->content.value = cov;
    return (OK);
}


// (int) TestCoverageNone(layer_expr, zoid_list, minsize)
//
// This function will return an integer value indicating the coverage
// of the layer expression given in the first argument over the
// regions described in the second argument.  The first argument can
// be a string containing a layer expression, or a return from
// ParseLayerExpression().  If the second argument is 0, the current
// reference zoidlist as set with SetZref() is assumed.  This defaults
// to tha area of the current cell.
//
// The third argument is an integer which gives the minimum dimension
// in internal units of trapezoids which will be considered in the
// result.  Sub-dimensional trapezoids are ignored.  This minimizes
// false-positive tests due to "slivers" caused by clipping errors in
// non-Manhattan geometry.  If the geomentry is known to be Manhattan,
// 0 can be used.  If 45's only, 2 is recommended, otherwise 4. 
// Negative values are taken as zero.
//
// The function tests each dark-area trapezoid from the layer
// expression against the reference zoid list.  It will return
// immediately on the first such zoid that is not completely uncovered
// by the reference zoid list.
//
// The return value is 0 if there is no dark area from the layer
// expression that intersects the reference zoid list, 1 otherwise. 
// This test is most efficient when determining whether or not the
// layer expression dark area intersects the reference list.
//
bool
lexpr_funcs::IFtestCoverageNone(Variable *res, Variable *args, void *datap)
{
    bool free_lspec;
    sLspec *lspec;
    if (!arg_lexpr(args, 0, &lspec, &free_lspec))
        return (BAD);
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 1, &zl, &free_zl, datap);
    if (ret != XIok) {
        FREE_CHK_LS(free_lspec, lspec)
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    int minsz;
    if (!arg_int(args, 2, &minsz)) {
        FREE_CHK_LS(free_lspec, lspec)
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }
    if (minsz < 0)
        minsz = 0;

    SIlexprCx *cx = (SIlexprCx*)datap;
    if (!cx)
        cx = SI()->LexprCx();
    const Zlist *zlc = zl;
    if (!zlc)
        zlc = cx->getZref();

    const Zlist *tzref = cx->getZref();
    int tdp = cx->hierDepth();
    cx->setZref(zlc);
    cx->setHierDepth(CDMAXCALLDEPTH);
    CovType cov;
    ret = lspec->testZlistCovNone(cx, &cov, minsz);
    cx->setHierDepth(tdp);
    cx->setZref(tzref);

    FREE_CHK_LS(free_lspec, lspec)
    FREE_CHK_ZL(free_zl, zl)
    if (ret == XIbad)
        return (BAD);
    if (ret == XIintr) {
        res->type = TYP_SCALAR;
        res->content.value = 0;
        SI()->SetInterrupt();
        return (OK);
    }

    res->type = TYP_SCALAR;
    res->content.value = cov;
    return (OK);
}


// (int) TestCoverage(layer_expr, zoid_list, testfull)
//
// This function is deprecated and should not be used in new scripts. 
// The TestCoverageFull, TestCoveragePartial, and TestCoverageNone
// functions are replacements.
//
// When the boolean testfull is true, this function is identical to
// TestCoveragePartial with a minsize value of 4.  When testfull is
// false, this function is equivalent to TestCoverageNone again with a
// minsize of 4.
//
bool
lexpr_funcs::IFtestCoverage(Variable *res, Variable *args, void *datap)
{
    bool free_lspec;
    sLspec *lspec;
    if (!arg_lexpr(args, 0, &lspec, &free_lspec))
        return (BAD);
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 1, &zl, &free_zl, datap);
    if (ret != XIok) {
        FREE_CHK_LS(free_lspec, lspec)
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    bool testfull;
    if (!arg_boolean(args, 2, &testfull)) {
        FREE_CHK_LS(free_lspec, lspec)
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }

    SIlexprCx *cx = (SIlexprCx*)datap;
    if (!cx)
        cx = SI()->LexprCx();
    const Zlist *zlc = zl;
    if (!zlc)
        zlc = cx->getZref();

    const Zlist *tzref = cx->getZref();
    int tdp = cx->hierDepth();
    cx->setZref(zlc);
    cx->setHierDepth(CDMAXCALLDEPTH);
    CovType cov;
    if (testfull)
        ret = lspec->testZlistCovPartial(cx, &cov, 4);
    else
        ret = lspec->testZlistCovNone(cx, &cov, 4);
    cx->setHierDepth(tdp);
    cx->setZref(tzref);

    FREE_CHK_LS(free_lspec, lspec)
    FREE_CHK_ZL(free_zl, zl)
    if (ret == XIbad)
        return (BAD);
    if (ret == XIintr) {
        res->type = TYP_SCALAR;
        res->content.value = 0;
        SI()->SetInterrupt();
        return (OK);
    }

    res->type = TYP_SCALAR;
    res->content.value = cov;
    return (OK);
}


// (object_handle) ZtoObjects(zoid_list, lname, join, to_dbase)
//
// This function will create a list of objects from a zoidlist.  The
// objects will be created on the layer whose name is given in the
// second argument, which will be created if it does not already
// exist.  If this argument is 0, the current layer will be used.  If
// the join argument is nonzero, the objects created will comprise a
// minimal set of polygons that enclose all of the trapezoids.  If the
// join argument is 0, the objects will be have the same geometry as
// the individual trapezoids.  If the to_dbase argument is nonzero,
// the new objects will be added to the database.  Otherwise, the new
// objects will be "copies" that can be manipulated with other
// functions that accept object copies, but they will not appear in
// the database.  The function will fail if not called in physical
// mode, or the layer could not be created.
//
bool
lexpr_funcs::IFzToObjects(Variable *res, Variable *args, void *datap)
{
    if (!SIparse()->ifGetCurPhysCell())
        return (BAD);
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 0, &zl, &free_zl, datap);
    if (ret != XIok) {
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    const char *lname;
    if (!arg_string(args, 1, &lname)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }
    bool join;
    if (!arg_boolean(args, 2, &join)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }
    bool todb;
    if (!arg_boolean(args, 3, &todb)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }

    CDl *ldesc = 0;
    if (lname && *lname) {
        ldesc = CDldb()->newLayer(lname, SIparse()->ifGetCurMode());
        if (!ldesc) {
            FREE_CHK_ZL(free_zl, zl)
            return (BAD);
        }
    }
    if (!ldesc)
        ldesc = SIparse()->ifGetCurLayer();
    if (!ldesc || SIparse()->ifGetCurMode() != Physical) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }

    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDol *o0 = 0, *oend = 0;
    if (join) {
        Zlist *zl1 = zl->copy();
        PolyList *p0 = zl1->to_poly_list();
        if (!todb) {
            if (!o0)
                o0 = p0->to_olist(ldesc, &oend);
            else
                p0->to_olist(ldesc, &oend);
        }
        else {
            while (p0) {
                CDo *cdo;
                if (p0->po.is_rect()) {
                    BBox BB(p0->po.points);
                    delete [] p0->po.points;
                    p0->po.points = 0;
                    cdo = SIparse()->ifGetCurPhysCell()->newBox(0, &BB,
                        ldesc, 0);
                }
                else
                    cdo = SIparse()->ifGetCurPhysCell()->newPoly(0, &p0->po,
                        ldesc, 0, false);
                if (!o0)
                    o0 = oend = new CDol(cdo, 0);
                else {
                    oend->next = new CDol(cdo, 0);
                    oend = oend->next;
                }
                PolyList *px = p0;
                p0 = p0->next;
                delete px;
            }
        }
    }
    else {
        for (Zlist *z = zl; z; z = z->next) {
            CDo *cdo = 0;
            if (z->Z.is_rect()) {
                BBox BB(z->Z.xll, z->Z.yl, z->Z.xur, z->Z.yu);
                if (BB.valid()) {
                    if (todb)
                        cdo = SIparse()->ifGetCurPhysCell()->newBox(0, &BB,
                            ldesc, 0);
                    else {
                        cdo = new CDo(ldesc, &BB);
                        cdo->set_copy(true);
                    }
                }
            }
            else {
                Poly po;
                if (z->Z.mkpoly(&po.points, &po.numpts)) {
                    if (todb)
                        cdo = SIparse()->ifGetCurPhysCell()->newPoly(0, &po,
                            ldesc, 0, false);
                    else {
                        cdo = new CDpo(ldesc, &po);
                        cdo->set_copy(true);
                    }
                }
            }
            if (cdo) {
                if (!o0)
                    o0 = oend = new CDol(cdo, 0);
                else {
                    oend->next = new CDol(cdo, 0);
                    oend = oend->next;
                }
            }
        }
    }
    if (o0) {
        sHdl *hnew = new sHdlObject(o0, SIparse()->ifGetCurPhysCell(), !todb);
        res->type = TYP_HANDLE;
        res->content.value = hnew->id;
    }
    FREE_CHK_ZL(free_zl, zl)
    return (OK);
}


// (int) ZtoTempLayer(name, zoidlist, join)
//
// This function creates a temporary layer using name, and adds the
// content of the zoidlist to the new layer, in the current cell.  If
// the layer for name exists, it will be used, with existing geometry
// untouched.  If join is nonzero, the zoidlist will be added as a
// minimal set of polygons, otherwise each zoid will be added as a box
// or polygon.  The function returns 1 on success, 0 otherwise.  This
// works in physical mode only.
//
bool
lexpr_funcs::IFzToTempLayer(Variable *res, Variable *args, void *datap)
{
    if (!SIparse()->ifGetCurPhysCell())
        return (BAD);

    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    if (!name || !*name)
        return (BAD);

    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 1, &zl, &free_zl, datap);
    if (ret != XIok) {
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        SI()->SetInterrupt();
        return (OK);
    }

    bool join;
    if (!arg_boolean(args, 2, &join)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (SIparse()->ifGetCurMode() == Physical) {
        CDl *ld;
        if (!free_zl)
            zl = zl->copy();
        int flags = TTLinternal;
        if (join)
            flags |= TTLjoin;
        ld = zl->to_temp_layer(name, flags, SIparse()->ifGetCurPhysCell(),
            &ret);
        if (ret != XIok) {
            if (ret == XIbad)
                return (BAD);
            res->type = TYP_SCALAR;
            res->content.value = 0;
            SI()->SetInterrupt();
            return (OK);
        }
        if (ld)
            res->content.value = 1;
    }
    else {
        FREE_CHK_ZL(free_zl, zl)
    }
    return (OK);
}


// (int) ClearTempLayer(name)
//
// This function will clear all of the objects in the current cell
// from the named layer, without saving them in the undo list.  If
// successful, 1 is returned, otherwise 0 is returned.  This works in
// physical mode only.
//
bool
lexpr_funcs::IFclearTempLayer(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))

    if (!name || !*name)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (SIparse()->ifGetCurMode() == Physical &&
            SIparse()->ifGetCurPhysCell()) {
        CDl *ld = CDldb()->findLayer(name, Physical);
        if (ld) {
            SIparse()->ifGetCurPhysCell()->clearLayer(ld);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) ZtoFile(filename, zoidlist, ascii)
//
// Save the zoidlist in a file, whose name is given in the first
// argument.  The zoidlist can be recovered with ZfromFile.  There are
// two file formats available.  If the boolean argument ascii is
// nonzero, a human-readable ASCII text file is produced.  Each line
// contains the six numbers that describe a trapezoid, using the
// following C-style format string:
//
//  "yl=%d yu=%d ll=%d ul=%d lr=%d ur=%d"
//
// The numbers are integer values in internal units (presently fixed
// at 1000 units per micron).
//
// If the ascii argument is zero, the file is in OASIS format, using a
// single dummy cell (named "zoidlist") and layer ("0100"), and uses
// only TRAPEZOID and CTRAPEZOID geometry records.  The OASIS
// reprsentation is more compact and is the appropriate choice for
// very large trapezoid collections.
//
// The function returns 1 if successful, 0 otherwise.
//
bool
lexpr_funcs::IFzToFile(Variable *res, Variable *args, void *datap)
{
    const char *fname;
    ARG_CHK(arg_string(args, 0, &fname))
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 1, &zl, &free_zl, datap);
    if (ret != XIok) {
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    bool ascii;
    if (!arg_boolean(args, 2, &ascii)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }

    res->type = TYP_SCALAR;
    if (ascii) {
        FILE *fp = fopen(fname, "w");
        if (!fp) {
            FREE_CHK_ZL(free_zl, zl)
            return (BAD);
        }
        for (Zlist *z = zl; z; z = z->next)
            z->Z.print(fp);
        fclose(fp);
        res->content.value = true;
    }
    else
        res->content.value = FIO()->ZlistToFile(zl, "0100", fname);

    FREE_CHK_ZL(free_zl, zl)
    return (OK);
}


// (zoidlist) ZfromFile(filename)
//
// Read the file, which was (probably) produced by ZtoFile, and return
// the list of trapezoids it contains.  Both file formats can be read
// by this function.
//
// If an error occurs in reading or an interrupt is received, this
// function will fail (halting the script).  Otherwise a zoidlist will
// always be returned, but the list may be empty.
//
bool
lexpr_funcs::IFzFromFile(Variable *res, Variable *args, void*)
{
    const char *fname;
    ARG_CHK(arg_string(args, 0, &fname))

    FILE *fp = large_fopen(fname, "rb");
    if (!fp)
        return (BAD);

    if (FIO()->IsOASIS(fp)) {
        // OASIS trapezoid file.
        fclose(fp);
        res->type = TYP_ZLIST;
        XIrt xrt;
        res->content.zlist = FIO()->ZlistFromFile(fname, &xrt);
        if (xrt != XIok)
            return (BAD);
    }
    else {
        // Assume ASCII trapezoid file.
        Zlist *z0 = 0, *ze = 0;
        char buf[256];
        while (fgets(buf, 256, fp) != 0) {
            Zoid Z;
            // The format matches Zoid::print().
            if (sscanf(buf, "yl=%d yu=%d ll=%d ul=%d lr=%d ur=%d",
                    &Z.yl, &Z.yu, &Z.xll, &Z.xul, &Z.xlr, &Z.xur) == 6) {
                if (!z0)
                    z0 = ze = new Zlist(&Z, 0);
                else {
                    ze->next = new Zlist(&Z, 0);
                    ze = ze->next;
                }
            }
        }
        fclose(fp);
        res->content.zlist = z0;
        res->type = TYP_ZLIST;
    }
    return (OK);
}


// ReadZfile(filename)
//
// This will read a trapezoid list file whose name is specified as the
// required string argument.  This is an ASCII file consisting of two
// types of lines:
//
// 1.  Trapezoid lines, in the ASCII format used by ZfromFile and
//     produced by ZtoFile, i.e., in the format
//
//        yl=%d yu=%d ll=%d ul=%d lr=%d ur=%d
//
// 2.  Layer designation lines in the form
//
//        L layer_name
//
//     The layer_name should be an Xic-style name for a layer, the layer
//     will be created if it does not exist.
//
// When a layer designation line is encountered, the trapezoids that
// have been read since the file start or last layer designator are
// written into the current cell on the specified layer.  Thus, each
// block of trapezoid lines must be followed by layer designation line
// for the trapezoids to be recognized.
//
// However, if the file contains no layer designation lines, all
// trapezoids will be added to the current cell on the current layer.
//
// Lines that are not recognized as one of these two forms are ignored.
//
// This function always returns 1.  The function will fail if the file
// can not be opened.
//
bool
lexpr_funcs::IFreadZfile(Variable *res, Variable *args, void*)
{
    const char *fname;
    ARG_CHK(arg_string(args, 0, &fname))

    FILE *fp = large_fopen(fname, "rb");
    if (!fp)
        return (BAD);

    int lcnt = 0;
    if (FIO()->IsOASIS(fp)) {
        fclose(fp);
        return (BAD);
    }
    else {
        Zlist *z0 = 0, *ze = 0;
        CDl *ld = 0;
        char buf[256];
        while (fgets(buf, 256, fp) != 0) {
            char *s = buf;
            while (isspace(*s))
                s++;
            if (!*s || ispunct(*s))
                continue;
            if (*s == 'L' || *s =='l') {
                lstring::advtok(&s);
                char *lname = lstring::gettok(&s);
                if (!lname)
                    continue;
                ld = CDldb()->newLayer(lname, Physical);
                if (!ld)
                    return (BAD);
                if (z0) {
                    z0->add(SIparse()->ifGetCurPhysCell(), ld, false, false);
                    z0->free();
                    z0 = ze = 0;
                }
                ld = 0;
                lcnt++;
                continue;
            }

            Zoid Z;
            // The format matches Zoid::print().
            if (sscanf(buf, "yl=%d yu=%d ll=%d ul=%d lr=%d ur=%d",
                    &Z.yl, &Z.yu, &Z.xll, &Z.xul, &Z.xlr, &Z.xur) == 6) {
                if (!z0)
                    z0 = ze = new Zlist(&Z, 0);
                else {
                    ze->next = new Zlist(&Z, 0);
                    ze = ze->next;
                }
            }
        }
        if (z0) {
            if (!ld && !lcnt)
                ld = SIparse()->ifGetCurLayer();
            if (ld) {
                z0->add(SIparse()->ifGetCurPhysCell(), ld, false, false);
                z0->free();
            }
            else
                z0->free();
        }
        fclose(fp);
        res->content.value = 1;
        res->type = TYP_SCALAR;
        return (OK);
    }
}


// (zoidlist) ChdGetZlist(chd_name, cellname, scale, array, clip, all)
//
// This function will create and return a trapezoid list created from
// objects read through the Cell Hierarchy Digest (CHD) whose access
// name is given in the first argument.
//
// See this table for the features that apply during a call to this
// function.  An overall transformation can be set with
// ChdSetFlatReadTransform, in which case the area given applies in
// the "root" coordinates.
//
// The cellname, if nonzero, must be the cell name after any aliasing
// that was in force when the CHD was created.  If cellname is passed
// 0, the default cell for the CHD is understood.  This is a cell name
// configured into the CHD, or the first top-level cell found in the
// archive file.
//
// The scale factor will be applied to all coordinates.  The accepted
// range is 0.001 - 1000.0.
//
// If the array argument is passed 0, no windowing will be used.
// Otherwise the array should have four components which specify a
// rectangle, in microns, in the coordinates of cellname.  The values
// are
//  array[0]  X left
//  array[1]  Y bottom
//  array[2]  X right
//  array[3]  Y top
// If an array is given, only the objects and subcells needed to
// render the window will be processed.  This window should be equal
// to or contained in the window use to configure the CHD, if any.
//
// If the boolean value clip is nonzero and an array is given,
// trapezoids will be clipped to the window.  Otherwise no clipping is
// done.
//
// If the boolean variable all is nonzero, the objects in the
// hierarchy under cellname will be transformed and added to the
// trapezoid list, i.e., the list will be a flat representation of the
// entire hierarchy.  Otherwise, only objects in cellname are
// processed.
//
bool
lexpr_funcs::IFchdGetZlist(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))
    double scale;
    ARG_CHK(arg_real(args, 2, &scale))
    double *array;
    ARG_CHK(arg_array_if(args, 3, &array, 4))
    bool clip;
    ARG_CHK(arg_boolean(args, 4, &clip))
    bool all;
    ARG_CHK(arg_boolean(args, 5, &all))

    if (!chdname || !*chdname)
        return (BAD);
    if (!cname || !*cname)
        return (BAD);
    res->type = TYP_ZLIST;
    res->content.zlist = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        BBox BB;
        if (array) {
            BB.left = INTERNAL_UNITS(array[0]);
            BB.bottom = INTERNAL_UNITS(array[1]);
            BB.right = INTERNAL_UNITS(array[2]);
            BB.top = INTERNAL_UNITS(array[3]);
            BB.fix();
        }

        FIOcvtPrms prms;
        prms.set_scale(scale);
        prms.set_allow_layer_mapping(true);
        if (array) {
            prms.set_use_window(true);
            prms.set_window(&BB);
            prms.set_clip(clip);
        }

        SymTab *tab = 0;  // important to initialize this
        OItype ort = chd->readFlat_zdb(&tab, cname, &prms);
        if (ort == OIaborted)
            SI()->SetInterrupt();
        else if (ort == OIerror)
            return (BAD);
        Zlist *zl0 = 0;
        SymTabGen gen(tab, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            Zlist *zl = ((zdb_t*)h->stData)->getZlist();
            if (zl) {
                Zlist *zn = zl;
                while (zn->next)
                    zn = zn->next;
                zn->next = zl0;
                zl0 = zl;
            }
            delete (zdb_t*)h->stData;
            delete h;
        }
        delete tab;
        try {
            res->content.zlist = zl0->repartition();
        }
        catch (XIrt ret) {
            res->content.zlist = 0;
            if (ret == XIintr)
                SI()->SetInterrupt();
            else
                return (BAD);
        }
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Operations
//-------------------------------------------------------------------------

// (zoidlist) NoOp(zoids)
//
// This function does nothing bu pass through the list.  It is used
// for debugging.
//
bool
lexpr_funcs::IFnoOp(Variable *res, Variable *args, void *datap)
{
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 0, &zl, &free_zl, datap);
    if (ret != XIok) {
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    if (free_zl)
        zl = zl->copy();
    res->type = TYP_ZLIST;
    res->content.zlist = zl;
    return (OK);
}


// (zoidlist) Filt(zoids, lexpr)
//
// This function is rather specialized.  First, the trapezoids passed
// by the handle in the first argument are separated into groups of
// mutually-connected trapezoids.  Each group is like a wire net.  We
// throw out the groups that do not intersect with nonzero area the
// dark area implied by the layer expression second argument.  The
// return value is a handle to a list of the trapezoids that remain.
//
bool
lexpr_funcs::IFfilt(Variable *res, Variable *args, void *datap)
{
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 0, &zl, &free_zl, datap);
    if (ret != XIok) {
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    bool free_lspec;
    sLspec *lspec;
    if (!arg_lexpr(args, 1, &lspec, &free_lspec)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }

    SIlexprCx *cx = (SIlexprCx*)datap;
    if (!cx)
        cx = SI()->LexprCx();
    if (!free_zl)
        zl = zl->copy();
    Zgroup *zg = zl->group();

    for (int i = 0; i < zg->num; i++) {
        const Zlist *tzref = cx->getZref();
        int tdp = cx->hierDepth();
        cx->setZref(zg->list[i]);
        cx->setHierDepth(CDMAXCALLDEPTH);
        CovType cov;
        // We'll use the DRC value for minsize (4).
        ret = lspec->testZlistCovNone(cx, &cov, 4);
        cx->setHierDepth(tdp);
        cx->setZref(tzref);

        if (ret == XIbad) {
            FREE_CHK_LS(free_lspec, lspec)
            delete zg;
            return (BAD);
        }
        if (ret == XIintr) {
            FREE_CHK_LS(free_lspec, lspec)
            delete zg;
            res->type = TYP_ZLIST;
            res->content.zlist = 0;
            SI()->SetInterrupt();
            return (OK);
        }
        if (cov == CovNone) {
            zg->list[i]->free();
            zg->list[i] = 0;
        }
    }
    FREE_CHK_LS(free_lspec, lspec)

    Zlist *z0 = 0, *ze = 0;;
    for (int i = 0; i < zg->num; i++) {
        if (zg->list[i]) {
            if (!z0)
                z0 = ze = zg->list[i];
            else {
                while (ze->next)
                    ze = ze->next;
                ze->next = zg->list[i];
            }
            zg->list[i] = 0;
        }
    }
    delete zg;

    res->type = TYP_ZLIST;
    res->content.zlist = z0;
    return (OK);
}


// (zoidlist) GeomAnd(zoids1 [,zoids2])
//
// This function takes either one or two arguments, each of which is
// taken as a zoidlist after possible conversion as described in the
// text for this section.  If one argument is given, the return is a
// zoidlist consisting of the intersection regions between zoids in
// the argument list.  If two arguments are given, the return is a
// list of intersecting regions between the tow argument lists.
//
bool
lexpr_funcs::IFgeomAnd(Variable *res, Variable *args, void *datap)
{
    Zlist *zl1, *zl2;
    bool free_zl1, free_zl2;
    XIrt ret = arg_zlist(args, 0, &zl1, &free_zl1, datap);
    if (ret != XIok) {
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    int nargs = 1;
    if (args[1].type != TYP_ENDARG) {
        if (args[2].type != TYP_ENDARG) {
            FREE_CHK_ZL(free_zl1, zl1)
            return (BAD);
        }
        ret = arg_zlist(args, 1, &zl2, &free_zl2, datap);
        if (ret != XIok) {
            FREE_CHK_ZL(free_zl1, zl1)
            if (ret == XIbad)
                return (BAD);
            res->type = TYP_ZLIST;
            res->content.zlist = 0;
            SI()->SetInterrupt();
            return (OK);
        }
        nargs++;
    }

    if (nargs == 1) {
        res->type = TYP_ZLIST;
        if (!free_zl1)
            zl1 = zl1->copy();
        ret = Zlist::zl_and(&zl1);
        res->content.zlist = zl1;
        if (ret == XIintr)
            SI()->SetInterrupt();
        else if (ret == XIbad)
            return (BAD);
        return (OK);
    }
    Variable v[2];
    v[0].type = v[1].type = TYP_ZLIST;
    v[0].content.zlist = zl1;
    v[1].content.zlist = zl2;
    bool retv = zlist_funcs::PTandZ(res, v, datap);
    FREE_CHK_ZL(free_zl1, zl1)
    FREE_CHK_ZL(free_zl2, zl2)
    return (retv);
}


// (zoidlist) GeomAndNot(zoids1, zoids2)
//
// This function takes two arguments, each of which is taken as a
// zoidlist after possible conversion as described in the text for
// this section.  The return is a list of regions covered by the first
// list that are not covered by the second.
//
bool
lexpr_funcs::IFgeomAndNot(Variable *res, Variable *args, void *datap)
{
    bool free_zl1, free_zl2;
    Zlist *zl1, *zl2;
    XIrt ret = arg_zlist(args, 0, &zl1, &free_zl1, datap);
    if (ret != XIok) {
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        return (OK);
    }
    ret = arg_zlist(args, 1, &zl2, &free_zl2, datap);
    if (ret != XIok) {
        FREE_CHK_ZL(free_zl1, zl1)
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        return (OK);
    }

    Variable v[2];
    v[0].type = v[1].type = TYP_ZLIST;
    v[0].content.zlist = zl1;
    v[1].content.zlist = zl2;
    bool retv = zlist_funcs::PTminusZ(res, v, datap);
    FREE_CHK_ZL(free_zl1, zl1)
    FREE_CHK_ZL(free_zl2, zl2)
    return (retv);
}


// (zoidlist) GeomCat(zoids1, ...)
//
// This function takes one or more arguments, each of which is taken
// as a zoidlist after possible conversion as described in the text
// for this section.  The return is a list of all regions from each of
// the arguments.  There is no attempt to clip or merge the returned
// list.
//
bool
lexpr_funcs::IFgeomCat(Variable *res, Variable *args, void *datap)
{
    Zlist *z0 = 0, *ze = 0;
    Zlist *zl;
    for (int i = 0; i < MAXARGC; i++) {
        if (args[i].type == TYP_ENDARG)
            break;
        bool free_zl;
        XIrt ret = arg_zlist(args, i, &zl, &free_zl, datap);
        if (ret != XIok) {
            z0->free();
            if (ret == XIbad)
                return (BAD);
            res->type = TYP_ZLIST;
            res->content.zlist = 0;
            SI()->SetInterrupt();
            return (OK);
        }
        if (zl) {
            if (!free_zl)
                zl = zl->copy();
            if (!z0)
                z0 = ze = zl;
            else {
                while (ze->next)
                    ze = ze->next;
                ze->next = zl;
            }
        }
    }
    res->type = TYP_ZLIST;
    res->content.zlist = z0;
    return (OK);
}


// (zoidlist) GeomNot(zoids)
//
// This function takes one argument, which is taken as a zoidlist
// after possible conversion as described in the text for this
// section.  The return is a list of zoids representing the areas of
// the reference area not covered by the argument list.
//
bool
lexpr_funcs::IFgeomNot(Variable *res, Variable *args, void *datap)
{
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 0, &zl, &free_zl, datap);
    if (ret != XIok) {
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }

    Variable v;
    v.type = TYP_ZLIST;
    v.content.zlist = zl;
    bool retv = zlist_funcs::PTnotZ(res, &v, datap);
    FREE_CHK_ZL(free_zl, zl)
    return (retv);
}


// (zoidlist) GeomOr(zoids, ...)
//
// This function takes one or more arguments, each of which is taken
// as a zoidlist after possible conversion as described in the text
// for this section.  The return is a list of all regions from each of
// the arguments, merged and clipped so that no elements overlap.
//
bool
lexpr_funcs::IFgeomOr(Variable *res, Variable *args, void *datap)
{
    Zlist *z0 = 0, *ze = 0;
    Zlist *zl;
    for (int i = 0; i < MAXARGC; i++) {
        if (args[i].type == TYP_ENDARG)
            break;
        bool free_zl;
        XIrt ret = arg_zlist(args, i, &zl, &free_zl, datap);
        if (ret != XIok) {
            z0->free();
            if (ret == XIbad)
                return (BAD);
            res->type = TYP_ZLIST;
            res->content.zlist = 0;
            SI()->SetInterrupt();
            return (OK);
        }
        if (zl) {
            if (!free_zl)
                zl = zl->copy();
            if (!z0)
                z0 = ze = zl;
            else {
                while (ze->next)
                    ze = ze->next;
                ze->next = zl;
            }
        }
    }
    res->type = TYP_ZLIST;
    try {
        res->content.zlist = z0->repartition();
    }
    catch (XIrt ret) {
        res->content.zlist = 0;
        if (ret == XIintr)
            SI()->SetInterrupt();
        else
            return (BAD);
    }
    return (OK);
}


// (zoidlist) GeomXor(zlods1 [,zoids2])
//
// This function takes one or two arguments, each of which is taken as
// a zoidlist after possible conversion as described in the text for
// this section.  If one argument is given, the return is a list of
// areas where one and only one zoid from the argument has coverage
// (note that this is not exclusive-or, in spite of the function
// name).  If two arguments are given, the return is the exclusive-or
// of the two lists, i.e., the areas covered by either list but not
// both.
//
bool
lexpr_funcs::IFgeomXor(Variable *res, Variable *args, void *datap)
{
    bool free_zl1, free_zl2;
    Zlist *zl1, *zl2;
    XIrt ret = arg_zlist(args, 0, &zl1, &free_zl1, datap);
    if (ret != XIok) {
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }
    int nargs = 1;
    if (args[1].type != TYP_ENDARG) {
        if (args[2].type != TYP_ENDARG) {
            FREE_CHK_ZL(free_zl1, zl1)
            return (BAD);
        }
        ret = arg_zlist(args, 1, &zl2, &free_zl2, datap);
        if (ret != XIok) {
            FREE_CHK_ZL(free_zl1, zl1)
            if (ret == XIbad)
                return (BAD);
            res->type = TYP_ZLIST;
            res->content.zlist = 0;
            SI()->SetInterrupt();
            return (OK);
        }
        nargs++;
    }

    if (nargs == 1) {
        res->type = TYP_ZLIST;
        if (!free_zl1)
            zl1 = zl1->copy();
        ret = Zlist::zl_andnot(&zl1);
        res->content.zlist = zl1;
        if (ret == XIintr)
            SI()->SetInterrupt();
        else if (ret == XIbad)
            return (BAD);
        return (OK);
    }
    Variable v[2];
    v[0].type = v[1].type = TYP_ZLIST;
    v[0].content.zlist = zl1;
    v[1].content.zlist = zl2;
    bool retv = zlist_funcs::PTxorZ(res, v, datap);
    FREE_CHK_ZL(free_zl1, zl1)
    FREE_CHK_ZL(free_zl2, zl2)
    return (retv);
}


//-------------------------------------------------------------------------
// Spatial Parameter Tables
//-------------------------------------------------------------------------

// (int) ReadSPtable(filename)
//
// This function reads a specification file for a spatial parameter
// table.  A spatial parameter table is a two-dimensional array of
// floating point values, which can be accessed via x-y coordinate
// pairs.  The user can define any number of such tables, each of
// which is given a unique identifying keyword.  Tables remain defined
// until explicitly destroyed, or until ClearAll is called.
//
// The tables are input through a file, which uses the following format:
//
// keyword X DX NX Y DY NY
// X Y value
// ...
//
// Blank lines and lines that begin with punctuation are ignored.
// There is one "header" line with the following entries:
//   keyword:       Arbitrary word for identification.  An existing
//                  database with the same identifier will be replaced.
//   X              Reference coordinate in microns.
//   DX             Grid spacing in X direction, in microns, must be > 0.
//   NX             Number of grid cells in X direction, must be > 0.
//   Y              Reference coordinate in microns.
//   DY             Grid spacing in Y direction, in microns, must be > 0.
//   NY             Number of grid cells in Y direction, must be > 0.
//
// The header line is followed by data lines that supply a value to
// the cells.  The X,Y given in microns specifies the cell.  A second
// access to a cell will simply overwrite the data value for that
// cell.  Unwritten cells will have a zero value.
//
// The function returns 1 on success, 0 otherwise with an error
// message available from the getError function.
//
bool
lexpr_funcs::IFreadSPtable(Variable *res, Variable *args, void*)
{
    const char *filename;
    ARG_CHK(arg_string(args, 0, &filename))

    res->type = TYP_SCALAR;
    res->content.value = spt_t::readSpatialParameterTable(filename);
    return (OK);
}


// (int) NewSPtable(name, x0, dx, nx, y0, dy, ny)
//
// This will create a new, empty spatial parameter table in memory,
// replacing any existing table with the same name.  The first
// argument is a string giving a short name for the table.  The table
// origin is at x0, y0 (in microns).  The unit cell size is given by
// dx, dy, in microns, and the number of cells along x and y is nx, ny.
//
// The function returns 1 on success, 0 otherwise, with a message
// available from GetError.
//
bool
lexpr_funcs::IFnewSPtable(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    double x0;
    ARG_CHK(arg_real(args, 1, &x0))
    double dx;
    ARG_CHK(arg_real(args, 2, &dx))
    int nx;
    ARG_CHK(arg_int(args, 3, &nx))
    double y0;
    ARG_CHK(arg_real(args, 4, &y0))
    double dy;
    ARG_CHK(arg_real(args, 5, &dy))
    int ny;
    ARG_CHK(arg_int(args, 6, &ny))

    res->type = TYP_SCALAR;
    res->content.value = spt_t::newSpatialParameterTable(name, x0, dx, nx,
        y0, dy, ny);
    return (OK);
}


// (int) WriteSPtable(name, filename)
//
// This will write the named spatial parameter table to a file.  The
// return value is 1 on success, 0 otherwise, with an error message
// available from GetError.
//
bool
lexpr_funcs::IFwriteSPtable(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    const char *filename;
    ARG_CHK(arg_string(args, 1, &filename))

    res->type = TYP_SCALAR;
    res->content.value = spt_t::writeSpatialParameterTable(name, filename);
    return (OK);
}


// (int) ClearSPtable(name)
//
// This will destroy the spatial parameter table whose keyword matches
// the string given.  If a numeric 0 (NULL) or a null string is
// passed, all spatial parameter tables will be destroyed.  The return
// value is the number of tables destroyed.
//
bool
lexpr_funcs::IFclearSPtable(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))

    res->type = TYP_SCALAR;
    res->content.value = spt_t::clearSpatialParameterTable(name);
    return (OK);
}


// (int) FindSPtable(name, array)
//
// This function returns 1 if a spatial parameter table with the given
// name exists in memory, 0 otherwise.  The array is an array of size
// 6 or larger, or the constant 0.  If an array name is passed, and
// the named table exists, the array is filled in with the following
// table parameters:
//
//    array[0]  origin x in microns
//    array[1]  x spacing in microns
//    array[2]  row size
//    array[3]  origin y in microns
//    array[4]  y spacing in microns
//    array[5]  column size
//
bool
lexpr_funcs::IFfindSPtable(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    double *array;
    ARG_CHK(arg_array_if(args, 1, &array, 6))

    res->type = TYP_SCALAR;
    res->content.value = 0.0;
    spt_t *f = spt_t::findSpatialParameterTable(name);
    if (f) {
        res->content.value = 1.0;
        if (array) {
            int ox, oy;
            unsigned int dx, nx, dy, ny;
            f->params(&ox, &dx, &nx, &oy, &dy, &ny);
            array[0] = MICRONS(ox);
            array[1] = MICRONS(dx);
            array[2] = nx;
            array[3] = MICRONS(oy);
            array[4] = MICRONS(dy);
            array[5] = ny;
        }
    }
    return (OK);
}


// (real) GetSPdata(name, x, y)
//
// This function returns the value from the spatial parameter table
// keyed by name, at coordinate x,y given in microns.  If x,y is out
// of range, 0 is returned.  The function fails (halts execution) if
// the table can't be found.
//
bool
lexpr_funcs::IFgetSPdata(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    int x;
    ARG_CHK(arg_coord(args, 1, &x, Physical))
    int y;
    ARG_CHK(arg_coord(args, 2, &y, Physical))

    res->type = TYP_SCALAR;
    res->content.value = 0.0;
    spt_t *f = spt_t::findSpatialParameterTable(name);
    if (!f) {
        Errs()->add_error("spatial parameter table %s not found.", name);
        return (BAD);
    }
    res->content.value = f->retrieve_item(x, y);
    return (OK);
}


// (int) SetSPdata(name, x, y, value)
//
// This function will set the data cell corresponding to x,y (in
// microns) of the named spatial parameter table to the value.  The
// return value is 1 if successful, 0 if  x,y is out of range or
// some other error occurs.  The function fails (halts execution)
// if the table can't be found.
//
bool
lexpr_funcs::IFsetSPdata(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    int x;
    ARG_CHK(arg_coord(args, 1, &x, Physical))
    int y;
    ARG_CHK(arg_coord(args, 2, &y, Physical))
    double value;
    ARG_CHK(arg_real(args, 3, &value))

    res->type = TYP_SCALAR;
    res->content.value = 0.0;
    spt_t *f = spt_t::findSpatialParameterTable(name);
    if (!f) {
        Errs()->add_error("spatial parameter table %s not found.", name);
        return (BAD);
    }
    res->content.value = f->save_item(x, y, value);
    return (OK);
}


//-------------------------------------------------------------------------
// Polymorphic Flat Database
//-------------------------------------------------------------------------

// (int) ChdOpenOdb(chd_name, scale, cellname, array, clip, dbname)
//
// This function will create a "special database" of the objects read
// through the Cell Hierarchy (CHD) Digest whose access name is passed
// as the first argument.
//
// See this table for the features that apply during a call to this
// function.
//
// The scale factor will be applied to all coordinates.  The accepted
// range is 0.001 - 1000.0.
//
// The cellname, if nonzero, must be the cell name after any aliasing
// that was in force when the CHD was created.  If cellname is passed
// 0, the default cell for the CHD is understood.  This is a cell name
// configured into the CHD, or the first top-level cell found in the
// archive file.
//
// The array, if not 0, is an array of four values or larger giving a
// rectangular area of cellname to read.  The values are in microns,
// in order L,B,R,T.  If zero, the entire cell bounding box is
// understood.  If the boolean value clip is nonzero, objects will be
// clipped to the array, if given.  The dbname is a string which names
// the database.  This can be any short name string.  The database can
// be retrieved or cleared using this name.
//
// The return value is 1 on success, 0 otherwise, with an error
// message likely available from GetError.
//
bool
lexpr_funcs::IFchdOpenOdb(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    double scale;
    ARG_CHK(arg_real(args, 1, &scale))
    const char *cname;
    ARG_CHK(arg_string(args, 2, &cname))
    double *array;
    ARG_CHK(arg_array_if(args, 3, &array, 4))
    bool clip;
    ARG_CHK(arg_boolean(args, 4, &clip))
    const char *dbname;
    ARG_CHK(arg_string(args, 5, &dbname))

    if (!chdname || !*chdname)
        return (BAD);
    if (!dbname || !*dbname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        BBox BB;
        if (array) {
            BB.left = INTERNAL_UNITS(array[0]);
            BB.bottom = INTERNAL_UNITS(array[1]);
            BB.right = INTERNAL_UNITS(array[2]);
            BB.top = INTERNAL_UNITS(array[3]);
            BB.fix();
        }

        FIOcvtPrms prms;
        prms.set_scale(scale);
        prms.set_allow_layer_mapping(true);
        if (array) {
            prms.set_use_window(true);
            prms.set_window(&BB);
            prms.set_clip(clip);
        }

        SymTab *tab = 0;  // important to initialize this
        bool ret = chd->readFlat_odb(&tab, cname, &prms);
        if (ret) {
            cSDB *db = new cSDB(dbname, tab, sdbOdb);
            CDsdb()->saveDB(db);
        }
        res->content.value = ret;
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (int) ChdOpenZdb(chd_name, scale, cellname, array, clip, dbname)
//
// This function will create a "special database" of the trapezoid
// representations of objects read through the Cell Hierarchy (CHD)
// Digest whose access name is passed as the first argument.
//
// See this table for the features that apply during a call to this
// function.
//
// The scale factor will be applied to all coordinates.  The accepted
// range is 0.001 - 1000.0.
//
// The cellname, if nonzero, must be the cell name after any aliasing
// that was in force when the CHD was created.  If cellname is passed
// 0, the default cell for the CHD is understood.  This is a cell name
// configured into the CHD, or the first top-level cell found in the
// archive file.
//
// The array, if not 0, is an array of four values or larger giving a
// rectangular area of cellname to read.  The values are in microns,
// in order L,B,R,T.  If zero, the entire cell bounding box is
// understood.  If the boolean value clip is nonzero, trapezoids will
// be clipped to the array, if given.  The dbname is a string which
// names the database.  This can be any short name string.  The
// database can be retrieved or cleared using this name.
//
// The return value is 1 on success, 0 otherwise, with an error
// message likely available from GetError.
//
bool
lexpr_funcs::IFchdOpenZdb(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    double scale;
    ARG_CHK(arg_real(args, 1, &scale))
    const char *cname;
    ARG_CHK(arg_string(args, 2, &cname))
    double *array;
    ARG_CHK(arg_array_if(args, 3, &array, 4))
    bool clip;
    ARG_CHK(arg_boolean(args, 4, &clip))
    const char *dbname;
    ARG_CHK(arg_string(args, 5, &dbname))

    if (!chdname || !*chdname)
        return (BAD);
    if (!dbname || !*dbname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        BBox BB;
        if (array) {
            BB.left = INTERNAL_UNITS(array[0]);
            BB.bottom = INTERNAL_UNITS(array[1]);
            BB.right = INTERNAL_UNITS(array[2]);
            BB.top = INTERNAL_UNITS(array[3]);
            BB.fix();
        }

        FIOcvtPrms prms;
        prms.set_scale(scale);
        prms.set_allow_layer_mapping(true);
        if (array) {
            prms.set_use_window(true);
            prms.set_window(&BB);
            prms.set_clip(clip);
        }

        SymTab *tab = 0;  // important to initialize this
        bool ret = chd->readFlat_zdb(&tab, cname, &prms);
        if (ret) {
            cSDB *db = new cSDB(dbname, tab, sdbZdb);
            CDsdb()->saveDB(db);
        }
        res->content.value = ret;
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (int) ChdOpenZbdb(chd_name, scale, cellname, array, dbname,
//  dx, dy, bx, by)
//
// This function will create a "special database" of the trapezoid
// representations of objects read through the Cell Hierarchy (CHD)
// Digest whose access name is passed as the first argument.
// This will open a database similar to ChdOpenZdb, however the
// trapezoids will be saved in binned lists.
//
// See this table for the features that apply during a call to this
// function.
//
// The scale factor will be applied to all coordinates.  The accepted
// range is 0.001 - 1000.0.
//
// The cellname, if nonzero, must be the cell name after any aliasing
// that was in force when the CHD was created.  If cellname is passed
// 0, the default cell for the CHD is understood.  This is a cell name
// configured into the CHD, or the first top-level cell found in the
// archive file.
//
// The array, if not 0, is an array of four values or larger giving a
// rectangular area of cellname to read.  The values are in microns,
// in order L,B,R,T.  If zero, the entire cell bounding box is
// understood.  The dbname is a string which names the database.  This
// can be any short name string.  The database can be retrieved or
// cleared using this name.
//
// The dx, dy are the grid spacing values for the bins, in microns.
// These values must be positive.  The bx, by are non-negative overlap
// bloat values for the bins.  The actual bins are bloated by these
// values in the x and y directions.  The trapezoids will be clipped
// to the bins.
//
// The return value is 1 on success, 0 otherwise, with an error
// message likely available from GetError.
//
bool
lexpr_funcs::IFchdOpenZbdb(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    double scale;
    ARG_CHK(arg_real(args, 1, &scale))
    const char *cname;
    ARG_CHK(arg_string(args, 2, &cname))
    double *array;
    ARG_CHK(arg_array_if(args, 3, &array, 4))
    const char *dbname;
    ARG_CHK(arg_string(args, 4, &dbname))
    int dx;
    ARG_CHK(arg_coord(args, 5, &dx, Physical))
    int dy;
    ARG_CHK(arg_coord(args, 6, &dy, Physical))
    int bx;
    ARG_CHK(arg_coord(args, 7, &bx, Physical))
    int by;
    ARG_CHK(arg_coord(args, 8, &by, Physical))

    if (!chdname || !*chdname)
        return (BAD);
    if (!dbname || !*dbname)
        return (BAD);
    if (dx < 2 || dy < 2)
        return (BAD);
    if (bx < 0 || by < 0)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        BBox BB;
        if (array) {
            BB.left = INTERNAL_UNITS(array[0]);
            BB.bottom = INTERNAL_UNITS(array[1]);
            BB.right = INTERNAL_UNITS(array[2]);
            BB.top = INTERNAL_UNITS(array[3]);
            BB.fix();
        }
        else {
            symref_t *p = chd->findSymref(cname, Physical, true);
            if (!p)
                return (BAD);
            BB = *p->get_bb();
        }

        FIOcvtPrms prms;
        prms.set_scale(scale);
        prms.set_allow_layer_mapping(true);
        if (array) {
            prms.set_use_window(true);
            prms.set_window(&BB);
        }

        SymTab *tab = 0;  // must pass 0 to create new tab
        bool ret = chd->readFlat_zbdb(&tab, cname, &prms, dx, dy, bx, by);
        if (ret) {
            cSDB *db = new cSDB(dbname, tab, sdbZbdb);
            CDsdb()->saveDB(db);
        }
        res->content.value = ret;
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (object_handle) GetObjectsOdb(dbname, layer_list, array)
//
// This returns a handle to a list of objects, extracted from a named
// database created with ChdOpenOdb.  The first argument is a database
// name string as given to ChdOpenOdb (This function will not work
// with ChdOpenZdb).  The second argument is a string containing a
// space-separated list of layer names, or 0.  Objects for each of
// the given layers will be obtained.  Objects on the same layer will
// be grouped together, with groups ordered as in the layer_list.  If
// this argument is 0, all layers will be used, ordered bottom-up as
// in the layer table.  The third argument is an array, as passed to
// ChdOpenOdb, or 0.  If 0, all objects for the specified layers in
// the database will be retrieved.  Otherwise, only those objects
// with bounding boxes that overlap the array rectangle with nonzero
// area will be retrieved.  The objects retrieved are copies of the
// database objects, which are not affected.
//
bool
lexpr_funcs::IFgetObjectsOdb(Variable *res, Variable *args, void*)
{
    const char *dbname;
    ARG_CHK(arg_string(args, 0, &dbname))
    const char *llist;
    ARG_CHK(arg_string(args, 1, &llist))
    double *array;
    ARG_CHK(arg_array_if(args, 2, &array, 4))

    if (!dbname || !*dbname)
        return (BAD);
    BBox BB;
    if (array) {
        BB.left = INTERNAL_UNITS(array[0]);
        BB.bottom = INTERNAL_UNITS(array[1]);
        BB.right = INTERNAL_UNITS(array[2]);
        BB.top = INTERNAL_UNITS(array[3]);
        BB.fix();
    }

    res->type = TYP_HANDLE;
    res->content.value = 0;

    cSDB *db = CDsdb()->findDB(dbname);
    if (db && db->type() == sdbOdb && db->table()) {
        if (!llist) {
            CDol *ol0 = 0, *oe = 0;
            CDlgen gen(Physical);
            CDl *ld;
            while ((ld = gen.next()) != 0) {
                odb_t *odb = (odb_t*)db->table()->get((unsigned long)ld);
                if (odb != (odb_t*)ST_NIL) {
                    int num = odb->find_objects(&BB);
                    for (int i = 0; i < num; i++) {
                        CDo *od = odb->objects()[i]->copyObject();
                        if (ol0) {
                            oe->next = new CDol(od, 0);
                            oe = oe->next;
                        }
                        else
                            oe = ol0 = new CDol(od, 0);
                    }
                }
            }
            sHdl *h = new sHdlObject(ol0, SIparse()->ifGetCurPhysCell(), true);
            res->type = TYP_HANDLE;
            res->content.value = h->id;
        }
        else {
            const char *llist_bak = llist;
            char *lname;
            CDol *ol0 = 0, *oe = 0;
            while ((lname = lstring::gettok(&llist)) != 0) {
                CDl *ld = CDldb()->findLayer(lname, Physical);
                if (ld) {
                    odb_t *odb = (odb_t*)db->table()->get((unsigned long)ld);
                    if (odb != (odb_t*)ST_NIL) {
                        int num = odb->find_objects(&BB);
                        for (int i = 0; i < num; i++) {
                            CDo *od = odb->objects()[i]->copyObject();
                            if (ol0) {
                                oe->next = new CDol(od, 0);
                                oe = oe->next;
                            }
                            else
                                oe = ol0 = new CDol(od, 0);
                        }
                    }
                }
                delete [] lname;
            }
            delete [] llist_bak;
            sHdl *h = new sHdlObject(ol0, SIparse()->ifGetCurPhysCell(), true);
            res->type = TYP_HANDLE;
            res->content.value = h->id;
        }
    }
    return (OK);
}


// (stringlist_handle) ListLayersDb(dbname)
//
// This function returns a handle to a list of layer name strings,
// naming the layers used in the database.  It applies to all of the
// database types.  On error, a scalar 0 is returned.
//
bool
lexpr_funcs::IFlistLayersDb(Variable *res, Variable *args, void*)
{
    const char *dbname;
    ARG_CHK(arg_string(args, 0, &dbname))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    cSDB *db = CDsdb()->findDB(dbname);
    if (db && db->table()) {
        stringlist *s0 = db->layers();
        if (s0) {
            sHdl *hdltmp = new sHdlString(s0);
            res->type = TYP_HANDLE;
            res->content.value = hdltmp->id;
        }
    }
    return (OK);
}


// (zoidlist) GetZlistDb(dbname, layer_name, zoidlist)
//
// This returns a zoidlist associated with a layer, extracted from a
// named database created with ChdOpenOdb, ChdOpenZdb, or ChdOpenZbdb.
// The first argument is a database name string as given to ChdOpenOdb
// or equivalent.  The second argument is the associated layer name.
//
// The third argument is the reference trapezoid list.  If the
// database was opened with ChdOpenOdb or ChdOpenZdb, the returned
// zoidlist will be clipped to the reference list.  If the database
// was opened with ChdOpenZbdb, the trapezoids for the bin containg
// the center of the first trapezoid in the reference list will be
// returned.  In all cases, the returned trapezoids are copies, the
// database is not affected.
//
// See also the GetZlist function, which can work similarly.
//
bool
lexpr_funcs::IFgetZlistDb(Variable *res, Variable *args, void *datap)
{
    const char *dbname;
    ARG_CHK(arg_string(args, 0, &dbname))
    const char *lname;
    ARG_CHK(arg_string(args, 1, &lname))
    Zlist *zl;
    bool free_zl;
    XIrt ret = arg_zlist(args, 2, &zl, &free_zl, datap);
    if (ret != XIok) {
        if (ret == XIbad)
            return (BAD);
        res->type = TYP_ZLIST;
        res->content.zlist = 0;
        SI()->SetInterrupt();
        return (OK);
    }

    if (!dbname || !*dbname) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }
    res->type = TYP_ZLIST;
    res->content.zlist = 0;
    CDl *ld = CDldb()->findLayer(lname, Physical);
    if (ld) {
        cSDB *db = CDsdb()->findDB(dbname);
        if (db && db->table()) {
            if (db->type() == sdbOdb) {
                odb_t *odb = (odb_t*)db->table()->get((unsigned long)ld);
                if (odb != (odb_t*)ST_NIL) {
                    res->content.zlist = odb->getZlist(zl, &ret);
                    if (ret != XIok) {
                        FREE_CHK_ZL(free_zl, zl)
                        if (ret == XIbad)
                            return (BAD);
                        res->type = TYP_ZLIST;
                        res->content.zlist = 0;
                        SI()->SetInterrupt();
                        return (OK);
                    }
                }
            }
            else if (db->type() == sdbZdb) {
                zdb_t *zdb = (zdb_t*)db->table()->get((unsigned long)ld);
                if (zdb != (zdb_t*)ST_NIL) {
                    res->content.zlist = zdb->getZlist(zl, &ret);
                    if (ret != XIok) {
                        FREE_CHK_ZL(free_zl, zl)
                        if (ret == XIbad)
                            return (BAD);
                        res->type = TYP_ZLIST;
                        res->content.zlist = 0;
                        SI()->SetInterrupt();
                        return (OK);
                    }
                }
            }
            else if (db->type() == sdbZbdb) {
                zbins_t *zdb = (zbins_t*)db->table()->get((unsigned long)ld);
                if (zdb != (zbins_t*)ST_NIL) {
                    res->content.zlist = zdb->getZlist(zl, &ret);
                    if (ret != XIok) {
                        FREE_CHK_ZL(free_zl, zl)
                        if (ret == XIbad)
                            return (BAD);
                        res->type = TYP_ZLIST;
                        res->content.zlist = 0;
                        SI()->SetInterrupt();
                        return (OK);
                    }
                }
            }
        }
    }
    FREE_CHK_ZL(free_zl, zl)
    return (OK);
}


// (zoidlist) GetZlistZbdb(dbname, layer_name, nx, ny)
//
// Return the zoidlist for the given bin and layer.  This applies only to
// databases opened with ChdOpenZbdb.  The 0,0 bin is in the lower left
// corner.
//
bool
lexpr_funcs::IFgetZlistZbdb(Variable *res, Variable *args, void*)
{
    const char *dbname;
    ARG_CHK(arg_string(args, 0, &dbname))
    const char *lname;
    ARG_CHK(arg_string(args, 1, &lname))
    int nx;
    ARG_CHK(arg_int(args, 2, &nx))
    int ny;
    ARG_CHK(arg_int(args, 3, &ny))

    if (!dbname || !*dbname)
        return (BAD);
    res->type = TYP_ZLIST;
    res->content.zlist = 0;
    CDl *ld = CDldb()->findLayer(lname, Physical);
    if (ld) {
        cSDB *db = CDsdb()->findDB(dbname);
        if (db && db->table() && db->type() == sdbZbdb) {
            zbins_t *zdb = (zbins_t*)db->table()->get((unsigned long)ld);
            if (zdb != (zbins_t*)ST_NIL)
                res->content.zlist = zdb->getZlist(nx, ny)->copy();
        }
    }
    return (OK);
}


// DestroyDb(name)
//
// This function will free and clear the special database named in the
// argument.  This is the database name as given to ChdOpenOdb or
// equivalent.  If the argument is 0, then all special databases will
// be freed and cleared.  This function always returns 1.
//
bool
lexpr_funcs::IFdestroyDb(Variable *res, Variable *args, void*)
{
    const char *dbname;
    ARG_CHK(arg_string(args, 0, &dbname))

    CDsdb()->destroyDB(dbname);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


//-------------------------------------------------------------------------
// Named String Tables
//-------------------------------------------------------------------------

// (int) FindNameTable(tabname, create)
//
// This function will create or verify the existence of a named string
// hash table.  The named tables are available for use in scripts, for
// associating a string with an integer and for efficiently ensuring
// uniqueness in a collection of strings.  The named tables persist
// until explicitly destroyed.
// 
// The tabname is an arbitrary name token used to access a named hash
// table.  This function returns 1 if the named hash table exists, 0
// otherwise.  If the boolean argument create is nonzero, if the named
// table does not exist, it will be created, and 1 returned.
//
bool
lexpr_funcs::IFfindNameTable(Variable *res, Variable *args, void*)
{
    const char *tabname;
    ARG_CHK(arg_string(args, 0, &tabname))
    bool create;
    ARG_CHK(arg_boolean(args, 1, &create))

    if (!tabname || !*tabname)
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = (nametab::findNametab(tabname, create) != 0);
    return (OK);
}


// (int) RemoveNameTable(tabname)
//
// This function will destroy a named hash table, as created with
// FindNameTable in create mode.  It the table exists, it will be
// destroyed, and 1 is returned.  If the given name does not match an
// existing table, 0 is returned.
//
bool
lexpr_funcs::IFremoveNameTable(Variable *res, Variable *args, void*)
{
    const char *tabname;
    ARG_CHK(arg_string(args, 0, &tabname))

    if (!tabname || !*tabname)
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = nametab::removeNametab(tabname);
    return (OK);
}


// (stringlist_handle) ListNameTables()
//
// This function returns a handle to a list of names of named hash tables
// currently in memory.
//
bool
lexpr_funcs::IFlistNameTables(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
    stringlist *s0 = nametab::listNametabs();
    if (s0) {
        sHdl *hdl = new sHdlString(s0);
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    return (OK);
}


// (int) ClearNameTables()
//
// This functions destroys all named hash tables in memory.
//
bool
lexpr_funcs::IFclearNameTables(Variable *res, Variable*, void*)
{
    nametab::clearNametabs();
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) AddNameToTable(tabname, name, value)
//
// This will add a string and associated integer to a named hash
// table.  The hash table whose name is given as the first argument
// must exist in memory, as created with FindNameTable in create mode. 
// The name can be any non-null and non-empty string.  The value can
// be any integer, however, the value -1 is reserved for internal use
// as a "not in table" indication.
//
// If name is inserted into the table, 1 is returned.  If name already
// exists in the table, or the table does not exist, 0 is returned. 
// The value is ignored if the name already exists in the table, the
// existing value is not updated.
//
bool
lexpr_funcs::IFaddNameToTable(Variable *res, Variable *args, void*)
{
    const char *tabname;
    ARG_CHK(arg_string(args, 0, &tabname))
    const char *name;
    ARG_CHK(arg_string(args, 1, &name))
    int value;
    ARG_CHK(arg_int(args, 2, &value))

    if (!tabname || !*tabname)
        return (BAD);
    if (!name || !*name)
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = 0;
    SymTab *st = nametab::findNametab(tabname, false);
    if (st) {
        if (st->get(name) == ST_NIL) {
            st->add(lstring::copy(name), (void*)(long)value, false);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) RemoveNameFromTable(tabname, name)
//
// This will remove the name string from the named hash table whose
// name is given as the first argument.  If the name string is found
// and removed, 1 is returned.  Otherwise, 0 is returned.
//
bool
lexpr_funcs::IFremoveNameFromTable(Variable *res, Variable *args, void*)
{
    const char *tabname;
    ARG_CHK(arg_string(args, 0, &tabname))
    const char *name;
    ARG_CHK(arg_string(args, 1, &name))

    if (!tabname || !*tabname)
        return (BAD);
    if (!name || !*name)
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = 0;
    SymTab *st = nametab::findNametab(tabname, false);
    if (st)
        res->content.value = st->remove(name);
    return (OK);
}


// (int) FindNameInTable(tabname, name)
//
// This function will return the data value saved with the name string
// in the table whose name is given as the first argument.  If the
// table is not found, or the name string is not found, -1 is
// returned.  Otherwise the returned value is that supplied to
// AddNameToTable for the name string.  Note that it is a bad idea to
// use -1 as a data value.
//
bool
lexpr_funcs::IFfindNameInTable(Variable *res, Variable *args, void*)
{
    const char *tabname;
    ARG_CHK(arg_string(args, 0, &tabname))
    const char *name;
    ARG_CHK(arg_string(args, 1, &name))

    if (!tabname || !*tabname)
        return (BAD);
    if (!name || !*name)
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = -1;
    SymTab *st = nametab::findNametab(tabname, false);
    if (st)
        res->content.value = (long)st->get(name);
    return (OK);
}


// (stringlist_handle) ListNamesInTable(tabname)
//
// This function returns a handle to a list of the strings saved in
// the hash table whose name is supplied as the first argument.
//
bool
lexpr_funcs::IFlistNamesInTable(Variable *res, Variable *args, void*)
{
    const char *tabname;
    ARG_CHK(arg_string(args, 0, &tabname))

    if (!tabname || !*tabname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    SymTab *st = nametab::findNametab(tabname, false);
    if (st) {
        stringlist *s0 = st->names();
        if (s0) {
            sHdl *hdl = new sHdlString(s0);
            res->type = TYP_HANDLE;
            res->content.value = hdl->id;
        }
    }
    return (OK);
}

