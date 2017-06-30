
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
 $Id: funcs_geom2.cc,v 1.139 2017/03/14 01:26:34 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "edit.h"
#include "scedif.h"
#include "undolist.h"
#include "python_if.h"
#include "tcltk_if.h"
#include "pcell.h"
#include "pcell_params.h"
#include "dsp_inlines.h"
#include "cd_hypertext.h"
#include "cd_propnum.h"
#include "cd_celldb.h"
#include "geo_zlist.h"
#include "geo_ylist.h"
#include "si_parsenode.h"
#include "si_handle.h"
#include "si_args.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_lexpr.h"
#include "main_scriptif.h"
#include "layertab.h"
#include "events.h"
#include "tech.h"
#include "select.h"
#include "errorlog.h"
#include "grfont.h"
#include "texttf.h"


#ifndef M_PI
#define	M_PI  3.14159265358979323846  // pi
#endif

////////////////////////////////////////////////////////////////////////
//
// Script Functions:  Geometry Editing Functions 2
//
////////////////////////////////////////////////////////////////////////

namespace {
    namespace geom2_funcs {

        // Cells, PCells, Vias, and Instance Placement
        bool IFcheckPCellParam(Variable*, Variable*, void*);
        bool IFcheckPCellParams(Variable*, Variable*, void*);
        bool IFcreateCell(Variable*, Variable*, void*);
        bool IFcopyCell(Variable*, Variable*, void*);
        bool IFrenameCell(Variable*, Variable*, void*);
        bool IFdeleteEmpties(Variable*, Variable*, void*);
        bool IFplace(Variable*, Variable*, void*);
        bool IFplaceH(Variable*, Variable*, void*);
        bool IFplaceSetArrayParams(Variable*, Variable*, void*);
        bool IFplaceSetPCellParams(Variable*, Variable*, void*);
        bool IFreplace(Variable*, Variable*, void*);
        bool IFopenViaSubMaster(Variable*, Variable*, void*);

        // Clipping Functions
        bool IFclipAround(Variable*, Variable*, void*);
        bool IFclipAroundCopy(Variable*, Variable*, void*);
        bool IFclipTo(Variable*, Variable*, void*);
        bool IFclipToCopy(Variable*, Variable*, void*);
        bool IFclipObjects(Variable*, Variable*, void*);
        bool IFclipIntersectCopy(Variable*, Variable*, void*);

        // Other Object Management Functions
        bool IFchangeLayer(Variable*, Variable*, void*);
        bool IFbloat(Variable*, Variable*, void*);
        bool IFmanhattanize(Variable*, Variable*, void*);
        bool IFjoin(Variable*, Variable*, void*);
        bool IFdecompose(Variable*, Variable*, void*);
        bool IFbox(Variable*, Variable*, void*);
        bool IFboxH(Variable*, Variable*, void*);
        bool IFpolygon(Variable*, Variable*, void*);
        bool IFpolygonH(Variable*, Variable*, void*);
        bool IFarc(Variable*, Variable*, void*);
        bool IFarcH(Variable*, Variable*, void*);
        bool IFround(Variable*, Variable*, void*);
        bool IFroundH(Variable*, Variable*, void*);
        bool IFhalfRound(Variable*, Variable*, void*);
        bool IFhalfRoundH(Variable*, Variable*, void*);
        bool IFsides(Variable*, Variable*, void*);
        bool IFwire(Variable*, Variable*, void*);
        bool IFwireH(Variable*, Variable*, void*);
        bool IFlabel(Variable*, Variable*, void*);
        bool IFlabelH(Variable*, Variable*, void*);
        bool IFlogo(Variable*, Variable*, void*);
        bool IFjustify(Variable*, Variable*, void*);
        bool IFdelete(Variable*, Variable*, void*);
        bool IFerase(Variable*, Variable*, void*);
        bool IFeraseUnder(Variable*, Variable*, void*);
        bool IFyank(Variable*, Variable*, void*);
        bool IFput(Variable*, Variable*, void*);
        bool IFxor(Variable*, Variable*, void*);
        bool IFcopy(Variable*, Variable*, void*);
        bool IFcopyToLayer(Variable*, Variable*, void*);
        bool IFmove(Variable*, Variable*, void*);
        bool IFmoveToLayer(Variable*, Variable*, void*);
        bool IFrotate(Variable*, Variable*, void*);
        bool IFrotateToLayer(Variable*, Variable*, void*);
        bool IFsplit(Variable*, Variable*, void*);
        bool IFflatten(Variable*, Variable*, void*);
        bool IFlayer(Variable*, Variable*, void*);

        // Property Management
        bool IFprpHandle(Variable*, Variable*, void*);
        bool IFgetPrpHandle(Variable*, Variable*, void*);
        bool IFcellPrpHandle(Variable*, Variable*, void*);
        bool IFgetCellPrpHandle(Variable*, Variable*, void*);
        bool IFprpNext(Variable*, Variable*, void*);
        bool IFprpNumber(Variable*, Variable*, void*);
        bool IFprpString(Variable*, Variable*, void*);
        bool IFprptyString(Variable*, Variable*, void*);
        bool IFgetPropertyString(Variable*, Variable*, void*);
        bool IFgetCellPropertyString(Variable*, Variable*, void*);
        bool IFprptyAdd(Variable*, Variable*, void*);
        bool IFaddProperty(Variable*, Variable*, void*);
        bool IFaddCellProperty(Variable*, Variable*, void*);
        bool IFprptyRemove(Variable*, Variable*, void*);
        bool IFremoveProperty(Variable*, Variable*, void*);
        bool IFremoveCellProperty(Variable*, Variable*, void*);
    }
    using namespace geom2_funcs;

#ifdef HAVE_PYTHON
    // Python wrappers.

    // Cells, PCells, Vias, and Instance Placement
    PY_FUNC(CheckPCellParam,        5,  IFcheckPCellParam);
    PY_FUNC(CheckPCellParams,       4,  IFcheckPCellParams);
    PY_FUNC(CreateCell,       VARARGS,  IFcreateCell);
    PY_FUNC(CopyCell,               2,  IFcopyCell);
    PY_FUNC(RenameCell,             2,  IFrenameCell);
    PY_FUNC(DeleteEmpties,          1,  IFdeleteEmpties);
    PY_FUNC(Place,            VARARGS,  IFplace);
    PY_FUNC(PlaceH,           VARARGS,  IFplaceH);
    PY_FUNC(PlaceSetArrayParams,    4,  IFplaceSetArrayParams);
    PY_FUNC(PlaceSetPCellParams,    4,  IFplaceSetPCellParams);
    PY_FUNC(Replace,                3,  IFreplace);
    PY_FUNC(OpenViaSubMaster,       2,  IFopenViaSubMaster);

    // Clipping Functions
    PY_FUNC(ClipAround,             4,  IFclipAround);
    PY_FUNC(ClipAroundCopy,         5,  IFclipAroundCopy);
    PY_FUNC(ClipTo,                 4,  IFclipTo);
    PY_FUNC(ClipToCopy,             5,  IFclipToCopy);
    PY_FUNC(ClipObjects,            2,  IFclipObjects);
    PY_FUNC(ClipIntersectCopy,      5,  IFclipIntersectCopy);

    // Other Object Management Functions
    PY_FUNC(ChangeLayer,            0,  IFchangeLayer);
    PY_FUNC(Bloat,                  2,  IFbloat);
    PY_FUNC(Manhattanize,           2,  IFmanhattanize);
    PY_FUNC(Join,                   0,  IFjoin);
    PY_FUNC(Decompose,              1,  IFdecompose);
    PY_FUNC(Box,                    4,  IFbox);
    PY_FUNC(BoxH,                   4,  IFboxH);
    PY_FUNC(Polygon,                2,  IFpolygon);
    PY_FUNC(PolygonH,               2,  IFpolygonH);
    PY_FUNC(Arc,                    8,  IFarc);
    PY_FUNC(ArcH,                   8,  IFarcH);
    PY_FUNC(Round,                  3,  IFround);
    PY_FUNC(RoundH,                 3,  IFroundH);
    PY_FUNC(HalfRound,              4,  IFhalfRound);
    PY_FUNC(HalfRoundH,             4,  IFhalfRoundH);
    PY_FUNC(Sides,                  1,  IFsides);
    PY_FUNC(Wire,                   4,  IFwire);
    PY_FUNC(WireH,                  4,  IFwireH);
    PY_FUNC(Label,            VARARGS,  IFlabel);
    PY_FUNC(LabelH,           VARARGS,  IFlabelH);
    PY_FUNC(Logo,             VARARGS,  IFlogo);
    PY_FUNC(Justify,                2,  IFjustify);
    PY_FUNC(Delete,                 0,  IFdelete);
    PY_FUNC(Erase,                  4,  IFerase);
    PY_FUNC(EraseUnder,             0,  IFeraseUnder);
    PY_FUNC(Yank,                   4,  IFyank);
    PY_FUNC(Put,                    3,  IFput);
    PY_FUNC(Xor,                    4,  IFxor);
    PY_FUNC(Copy,                   5,  IFcopy);
    PY_FUNC(CopyToLayer,            7,  IFcopyToLayer);
    PY_FUNC(Move,                   4,  IFmove);
    PY_FUNC(MoveToLayer,            6,  IFmoveToLayer);
    PY_FUNC(Rotate,                 4,  IFrotate);
    PY_FUNC(RotateToLayer,          6,  IFrotateToLayer);
    PY_FUNC(Split,                  4,  IFsplit);
    PY_FUNC(Flatten,                3,  IFflatten);
    PY_FUNC(Layer,                  7,  IFlayer);

    // Property Management
    PY_FUNC(PrpHandle,              1,  IFprpHandle);
    PY_FUNC(GetPrpHandle,           1,  IFgetPrpHandle);
    PY_FUNC(CellPrpHandle,          0,  IFcellPrpHandle);
    PY_FUNC(GetCellPrpHandle,       1,  IFgetCellPrpHandle);
    PY_FUNC(PrpNext,                1,  IFprpNext);
    PY_FUNC(PrpNumber,              1,  IFprpNumber);
    PY_FUNC(PrpString,              1,  IFprpString);
    PY_FUNC(PrptyString,            2,  IFprptyString);
    PY_FUNC(GetPropertyString,      1,  IFgetPropertyString);
    PY_FUNC(GetCellPropertyString,  1,  IFgetCellPropertyString);
    PY_FUNC(PrptyAdd,               3,  IFprptyAdd);
    PY_FUNC(AddProperty,            2,  IFaddProperty);
    PY_FUNC(AddCellProperty,        2,  IFaddCellProperty);
    PY_FUNC(PrptyRemove,            3,  IFprptyRemove);
    PY_FUNC(RemoveProperty,         2,  IFremoveProperty);
    PY_FUNC(RemoveCellProperty,     2,  IFremoveCellProperty);


    void py_register_geom2()
    {
      // Cells, PCells, Vias, and Instance Placement
      cPyIf::register_func("CheckPCellParam",        pyCheckPCellParam);
      cPyIf::register_func("CheckPCellParams",       pyCheckPCellParams);
      cPyIf::register_func("CreateCell",             pyCreateCell);
      cPyIf::register_func("CopyCell",               pyCopyCell);
      cPyIf::register_func("RenameCell",             pyRenameCell);
      cPyIf::register_func("DeleteEmpties",          pyDeleteEmpties);
      cPyIf::register_func("Place",                  pyPlace);
      cPyIf::register_func("PlaceH",                 pyPlaceH);
      cPyIf::register_func("PlaceSetArrayParams",    pyPlaceSetArrayParams);
      cPyIf::register_func("PlaceSetPCellParams",    pyPlaceSetPCellParams);
      cPyIf::register_func("Replace",                pyReplace);
      cPyIf::register_func("OpenViaSubMaster",       pyOpenViaSubMaster);

      // Clipping Functions
      cPyIf::register_func("ClipAround",             pyClipAround);
      cPyIf::register_func("ClipAroundCopy",         pyClipAroundCopy);
      cPyIf::register_func("ClipTo",                 pyClipTo);
      cPyIf::register_func("ClipToCopy",             pyClipToCopy);
      cPyIf::register_func("ClipObjects",            pyClipObjects);
      cPyIf::register_func("ClipIntersectCopy",      pyClipIntersectCopy);

      // Other Object Management Functions
      cPyIf::register_func("ChangeLayer",            pyChangeLayer);
      cPyIf::register_func("Bloat",                  pyBloat);
      cPyIf::register_func("Manhattanize",           pyManhattanize);
      cPyIf::register_func("Join",                   pyJoin);
      cPyIf::register_func("Decompose",              pyDecompose);
      cPyIf::register_func("Box",                    pyBox);
      cPyIf::register_func("BoxH",                   pyBoxH);
      cPyIf::register_func("Polygon",                pyPolygon);
      cPyIf::register_func("PolygonH",               pyPolygonH);
      cPyIf::register_func("Arc",                    pyArc);
      cPyIf::register_func("ArcH",                   pyArcH);
      cPyIf::register_func("Round",                  pyRound);
      cPyIf::register_func("RoundH",                 pyRoundH);
      cPyIf::register_func("HalfRound",              pyHalfRound);
      cPyIf::register_func("HalfRoundH",             pyHalfRoundH);
      cPyIf::register_func("Sides",                  pySides);
      cPyIf::register_func("Wire",                   pyWire);
      cPyIf::register_func("WireH",                  pyWireH);
      cPyIf::register_func("Label",                  pyLabel);
      cPyIf::register_func("LabelH",                 pyLabelH);
      cPyIf::register_func("Logo",                   pyLogo);
      cPyIf::register_func("Justify",                pyJustify);
      cPyIf::register_func("Delete",                 pyDelete);
      cPyIf::register_func("Erase",                  pyErase);
      cPyIf::register_func("EraseUnder",             pyEraseUnder);
      cPyIf::register_func("Yank",                   pyYank);
      cPyIf::register_func("Put",                    pyPut);
      cPyIf::register_func("Xor",                    pyXor);
      cPyIf::register_func("Copy",                   pyCopy);
      cPyIf::register_func("CopyToLayer",            pyCopyToLayer);
      cPyIf::register_func("Move",                   pyMove);
      cPyIf::register_func("MoveToLayer",            pyMoveToLayer);
      cPyIf::register_func("Rotate",                 pyRotate);
      cPyIf::register_func("RotateToLayer",          pyRotateToLayer);
      cPyIf::register_func("Split",                  pySplit);
      cPyIf::register_func("Flatten",                pyFlatten);
      cPyIf::register_func("Layer",                  pyLayer);

      // Property Management
      cPyIf::register_func("PrpHandle",              pyPrpHandle);
      cPyIf::register_func("GetPrpHandle",           pyGetPrpHandle);
      cPyIf::register_func("CellPrpHandle",          pyCellPrpHandle);
      cPyIf::register_func("GetCellPrpHandle",       pyGetCellPrpHandle);
      cPyIf::register_func("PrpNext",                pyPrpNext);
      cPyIf::register_func("PrpNumber",              pyPrpNumber);
      cPyIf::register_func("PrpString",              pyPrpString);
      cPyIf::register_func("PrptyString",            pyPrptyString);
      cPyIf::register_func("GetPropertyString",      pyGetPropertyString);
      cPyIf::register_func("GetCellPropertyString",  pyGetCellPropertyString);
      cPyIf::register_func("PrptyAdd",               pyPrptyAdd);
      cPyIf::register_func("AddProperty",            pyAddProperty);
      cPyIf::register_func("AddCellProperty",        pyAddCellProperty);
      cPyIf::register_func("PrptyRemove",            pyPrptyRemove);
      cPyIf::register_func("RemoveProperty",         pyRemoveProperty);
      cPyIf::register_func("RemoveCellProperty",     pyRemoveCellProperty);
    }
#endif  // HAVE_PYTHON

#ifdef HAVE_TCL
    // Tcl/Tk wrappers.

    // Cells, PCells, Vias, and Instance Placement
    TCL_FUNC(CheckPCellParam,        5,  IFcheckPCellParam);
    TCL_FUNC(CheckPCellParams,       4,  IFcheckPCellParams);
    TCL_FUNC(CreateCell,       VARARGS,  IFcreateCell);
    TCL_FUNC(CopyCell,               2,  IFcopyCell);
    TCL_FUNC(RenameCell,             2,  IFrenameCell);
    TCL_FUNC(DeleteEmpties,          1,  IFdeleteEmpties);
    TCL_FUNC(Place,            VARARGS,  IFplace);
    TCL_FUNC(PlaceH,           VARARGS,  IFplaceH);
    TCL_FUNC(PlaceSetArrayParams,    4,  IFplaceSetArrayParams);
    TCL_FUNC(PlaceSetPCellParams,    4,  IFplaceSetPCellParams);
    TCL_FUNC(Replace,                3,  IFreplace);
    TCL_FUNC(OpenViaSubMaster,       2,  IFopenViaSubMaster);

    // Clipping Functions
    TCL_FUNC(ClipAround,             4,  IFclipAround);
    TCL_FUNC(ClipAroundCopy,         5,  IFclipAroundCopy);
    TCL_FUNC(ClipTo,                 4,  IFclipTo);
    TCL_FUNC(ClipToCopy,             5,  IFclipToCopy);
    TCL_FUNC(ClipObjects,            2,  IFclipObjects);
    TCL_FUNC(ClipIntersectCopy,      5,  IFclipIntersectCopy);

    // Other Object Management Functions
    TCL_FUNC(ChangeLayer,            0,  IFchangeLayer);
    TCL_FUNC(Bloat,                  2,  IFbloat);
    TCL_FUNC(Manhattanize,           2,  IFmanhattanize);
    TCL_FUNC(Join,                   0,  IFjoin);
    TCL_FUNC(Decompose,              1,  IFdecompose);
    TCL_FUNC(Box,                    4,  IFbox);
    TCL_FUNC(BoxH,                   4,  IFboxH);
    TCL_FUNC(Polygon,                2,  IFpolygon);
    TCL_FUNC(PolygonH,               2,  IFpolygonH);
    TCL_FUNC(Arc,                    8,  IFarc);
    TCL_FUNC(ArcH,                   8,  IFarcH);
    TCL_FUNC(Round,                  3,  IFround);
    TCL_FUNC(RoundH,                 3,  IFroundH);
    TCL_FUNC(HalfRound,              4,  IFhalfRound);
    TCL_FUNC(HalfRoundH,             4,  IFhalfRoundH);
    TCL_FUNC(Sides,                  1,  IFsides);
    TCL_FUNC(Wire,                   4,  IFwire);
    TCL_FUNC(WireH,                  4,  IFwireH);
    TCL_FUNC(Label,            VARARGS,  IFlabel);
    TCL_FUNC(LabelH,           VARARGS,  IFlabelH);
    TCL_FUNC(Logo,             VARARGS,  IFlogo);
    TCL_FUNC(Justify,                2,  IFjustify);
    TCL_FUNC(Delete,                 0,  IFdelete);
    TCL_FUNC(Erase,                  4,  IFerase);
    TCL_FUNC(EraseUnder,             0,  IFeraseUnder);
    TCL_FUNC(Yank,                   4,  IFyank);
    TCL_FUNC(Put,                    3,  IFput);
    TCL_FUNC(Xor,                    4,  IFxor);
    TCL_FUNC(Copy,                   5,  IFcopy);
    TCL_FUNC(CopyToLayer,            7,  IFcopyToLayer);
    TCL_FUNC(Move,                   4,  IFmove);
    TCL_FUNC(MoveToLayer,            6,  IFmoveToLayer);
    TCL_FUNC(Rotate,                 4,  IFrotate);
    TCL_FUNC(RotateToLayer,          6,  IFrotateToLayer);
    TCL_FUNC(Split,                  4,  IFsplit);
    TCL_FUNC(Flatten,                3,  IFflatten);
    TCL_FUNC(Layer,                  7,  IFlayer);

    // Property Management by Handles
    TCL_FUNC(PrpHandle,              1,  IFprpHandle);
    TCL_FUNC(GetPrpHandle,           1,  IFgetPrpHandle);
    TCL_FUNC(CellPrpHandle,          0,  IFcellPrpHandle);
    TCL_FUNC(GetCellPrpHandle,       1,  IFgetCellPrpHandle);
    TCL_FUNC(PrpNext,                1,  IFprpNext);
    TCL_FUNC(PrpNumber,              1,  IFprpNumber);
    TCL_FUNC(PrpString,              1,  IFprpString);
    TCL_FUNC(PrptyString,            2,  IFprptyString);
    TCL_FUNC(GetPropertyString,      1,  IFgetPropertyString);
    TCL_FUNC(GetCellPropertyString,  1,  IFgetCellPropertyString);
    TCL_FUNC(PrptyAdd,               3,  IFprptyAdd);
    TCL_FUNC(AddProperty,            2,  IFaddProperty);
    TCL_FUNC(AddCellProperty,        2,  IFaddCellProperty);
    TCL_FUNC(PrptyRemove,            3,  IFprptyRemove);
    TCL_FUNC(RemoveProperty,         2,  IFremoveProperty);
    TCL_FUNC(RemoveCellProperty,     2,  IFremoveCellProperty);


    void tcl_register_geom2()
    {
      // Cells, PCells, Vias, and Instance Placement
      cTclIf::register_func("CheckPCellParam",        tclCheckPCellParam);
      cTclIf::register_func("CheckPCellParams",       tclCheckPCellParams);
      cTclIf::register_func("CreateCell",             tclCreateCell);
      cTclIf::register_func("CopyCell",               tclCopyCell);
      cTclIf::register_func("RenameCell",             tclRenameCell);
      cTclIf::register_func("DeleteEmpties",          tclDeleteEmpties);
      cTclIf::register_func("Place",                  tclPlace);
      cTclIf::register_func("PlaceH",                 tclPlaceH);
      cTclIf::register_func("PlaceSetArrayParams",    tclPlaceSetArrayParams);
      cTclIf::register_func("PlaceSetPCellParams",    tclPlaceSetPCellParams);
      cTclIf::register_func("Replace",                tclReplace);
      cTclIf::register_func("OpenViaSubMaster",       tclOpenViaSubMaster);

      // Clipping Functions
      cTclIf::register_func("ClipAround",             tclClipAround);
      cTclIf::register_func("ClipAroundCopy",         tclClipAroundCopy);
      cTclIf::register_func("ClipTo",                 tclClipTo);
      cTclIf::register_func("ClipToCopy",             tclClipToCopy);
      cTclIf::register_func("ClipObjects",            tclClipObjects);
      cTclIf::register_func("ClipIntersectCopy",      tclClipIntersectCopy);

      // Other Object Management Functions
      cTclIf::register_func("ChangeLayer",            tclChangeLayer);
      cTclIf::register_func("Bloat",                  tclBloat);
      cTclIf::register_func("Manhattanize",           tclManhattanize);
      cTclIf::register_func("Join",                   tclJoin);
      cTclIf::register_func("Decompose",              tclDecompose);
      cTclIf::register_func("Box",                    tclBox);
      cTclIf::register_func("BoxH",                   tclBoxH);
      cTclIf::register_func("Polygon",                tclPolygon);
      cTclIf::register_func("PolygonH",               tclPolygonH);
      cTclIf::register_func("Arc",                    tclArc);
      cTclIf::register_func("ArcH",                   tclArcH);
      cTclIf::register_func("Round",                  tclRound);
      cTclIf::register_func("RoundH",                 tclRoundH);
      cTclIf::register_func("HalfRound",              tclHalfRound);
      cTclIf::register_func("HalfRoundH",             tclHalfRoundH);
      cTclIf::register_func("Sides",                  tclSides);
      cTclIf::register_func("Wire",                   tclWire);
      cTclIf::register_func("WireH",                  tclWireH);
      cTclIf::register_func("Label",                  tclLabel);
      cTclIf::register_func("LabelH",                 tclLabelH);
      cTclIf::register_func("Logo",                   tclLogo);
      cTclIf::register_func("Justify",                tclJustify);
      cTclIf::register_func("Delete",                 tclDelete);
      cTclIf::register_func("Erase",                  tclErase);
      cTclIf::register_func("EraseUnder",             tclEraseUnder);
      cTclIf::register_func("Yank",                   tclYank);
      cTclIf::register_func("Put",                    tclPut);
      cTclIf::register_func("Xor",                    tclXor);
      cTclIf::register_func("Copy",                   tclCopy);
      cTclIf::register_func("CopyToLayer",            tclCopyToLayer);
      cTclIf::register_func("Move",                   tclMove);
      cTclIf::register_func("MoveToLayer",            tclMoveToLayer);
      cTclIf::register_func("Rotate",                 tclRotate);
      cTclIf::register_func("RotateToLayer",          tclRotateToLayer);
      cTclIf::register_func("Split",                  tclSplit);
      cTclIf::register_func("Flatten",                tclFlatten);
      cTclIf::register_func("Layer",                  tclLayer);

      // Property Management
      cTclIf::register_func("PrpHandle",              tclPrpHandle);
      cTclIf::register_func("GetPrpHandle",           tclGetPrpHandle);
      cTclIf::register_func("CellPrpHandle",          tclCellPrpHandle);
      cTclIf::register_func("GetCellPrpHandle",       tclGetCellPrpHandle);
      cTclIf::register_func("PrpNext",                tclPrpNext);
      cTclIf::register_func("PrpNumber",              tclPrpNumber);
      cTclIf::register_func("PrpString",              tclPrpString);
      cTclIf::register_func("PrptyString",            tclPrptyString);
      cTclIf::register_func("GetPropertyString",      tclGetPropertyString);
      cTclIf::register_func("GetCellPropertyString",  tclGetCellPropertyString);
      cTclIf::register_func("PrptyAdd",               tclPrptyAdd);
      cTclIf::register_func("AddProperty",            tclAddProperty);
      cTclIf::register_func("AddCellProperty",        tclAddCellProperty);
      cTclIf::register_func("PrptyRemove",            tclPrptyRemove);
      cTclIf::register_func("RemoveProperty",         tclRemoveProperty);
      cTclIf::register_func("RemoveCellProperty",     tclRemoveCellProperty);
    }
#endif  // HAVE_TCL
}


// Export to load functions in this script library.
//
void
cEdit::load_script_funcs2()
{
  using namespace geom2_funcs;

  // Cells, PCells, Vias, and Instance Placement
  SIparse()->registerFunc("CheckPCellParam",        5,  IFcheckPCellParam);
  SIparse()->registerFunc("CheckPCellParams",       4,  IFcheckPCellParams);
  SIparse()->registerFunc("CreateCell",       VARARGS,  IFcreateCell);
  SIparse()->registerFunc("CopyCell",               2,  IFcopyCell);
  SIparse()->registerFunc("RenameCell",             2,  IFrenameCell);
  SIparse()->registerFunc("DeleteEmpties",          1,  IFdeleteEmpties);
  SIparse()->registerFunc("Place",            VARARGS,  IFplace);
  SIparse()->registerFunc("PlaceH",           VARARGS,  IFplaceH);
  SIparse()->registerFunc("PlaceSetArrayParams",    4,  IFplaceSetArrayParams);
  SIparse()->registerFunc("PlaceSetPCellParams",    4,  IFplaceSetPCellParams);
  SIparse()->registerFunc("Replace",                3,  IFreplace);
  SIparse()->registerFunc("OpenViaSubMaster",       2,  IFopenViaSubMaster);

  // Clipping Functions
  SIparse()->registerFunc("ClipAround",             4,  IFclipAround);
  SIparse()->registerFunc("ClipAroundCopy",         5,  IFclipAroundCopy);
  SIparse()->registerFunc("ClipTo",                 4,  IFclipTo);
  SIparse()->registerFunc("ClipToCopy",             5,  IFclipToCopy);
  SIparse()->registerFunc("ClipObjects",            2,  IFclipObjects);
  SIparse()->registerFunc("ClipIntersectCopy",      5,  IFclipIntersectCopy);

  // Other Object Management Functions
  SIparse()->registerFunc("ChangeLayer",            0,  IFchangeLayer);
  SIparse()->registerFunc("Bloat",                  2,  IFbloat);
  SIparse()->registerFunc("Manhattanize",           2,  IFmanhattanize);
  SIparse()->registerFunc("Join",                   0,  IFjoin);
  SIparse()->registerFunc("Decompose",              1,  IFdecompose);
  SIparse()->registerFunc("Box",                    4,  IFbox);
  SIparse()->registerFunc("BoxH",                   4,  IFboxH);
  SIparse()->registerFunc("Polygon",                2,  IFpolygon);
  SIparse()->registerFunc("PolygonH",               2,  IFpolygonH);
  SIparse()->registerFunc("Arc",                    8,  IFarc);
  SIparse()->registerFunc("ArcH",                   8,  IFarcH);
  SIparse()->registerFunc("Round",                  3,  IFround);
  SIparse()->registerFunc("RoundH",                 3,  IFroundH);
  SIparse()->registerFunc("HalfRound",              4,  IFhalfRound);
  SIparse()->registerFunc("HalfRoundH",             4,  IFhalfRoundH);
  SIparse()->registerFunc("Sides",                  1,  IFsides);
  SIparse()->registerFunc("Wire",                   4,  IFwire);
  SIparse()->registerFunc("WireH",                  4,  IFwireH);
  SIparse()->registerFunc("Label",            VARARGS,  IFlabel);
  SIparse()->registerFunc("LabelH",           VARARGS,  IFlabelH);
  SIparse()->registerFunc("Logo",             VARARGS,  IFlogo);
  SIparse()->registerFunc("Justify",                2,  IFjustify);
  SIparse()->registerFunc("Delete",                 0,  IFdelete);
  SIparse()->registerFunc("Erase",                  4,  IFerase);
  SIparse()->registerFunc("EraseUnder",             0,  IFeraseUnder);
  SIparse()->registerFunc("Yank",                   4,  IFyank);
  SIparse()->registerFunc("Put",                    3,  IFput);
  SIparse()->registerFunc("Xor",                    4,  IFxor);
  SIparse()->registerFunc("Copy",                   5,  IFcopy);
  SIparse()->registerFunc("CopyToLayer",            7,  IFcopyToLayer);
  SIparse()->registerFunc("Move",                   4,  IFmove);
  SIparse()->registerFunc("MoveToLayer",            6,  IFmoveToLayer);
  SIparse()->registerFunc("Rotate",                 4,  IFrotate);
  SIparse()->registerFunc("RotateToLayer",          6,  IFrotateToLayer);
  SIparse()->registerFunc("Split",                  4,  IFsplit);
  SIparse()->registerFunc("Flatten",                3,  IFflatten);
  SIparse()->registerFunc("Layer",                  7,  IFlayer);

  // Property Management by Handles
  SIparse()->registerFunc("PrpHandle",              1,  IFprpHandle);
  SIparse()->registerFunc("GetPrpHandle",           1,  IFgetPrpHandle);
  SIparse()->registerFunc("CellPrpHandle",          0,  IFcellPrpHandle);
  SIparse()->registerFunc("GetCellPrpHandle",       1,  IFgetCellPrpHandle);
  SIparse()->registerFunc("PrpNext",                1,  IFprpNext);
  SIparse()->registerFunc("PrpNumber",              1,  IFprpNumber);
  SIparse()->registerFunc("PrpString",              1,  IFprpString);
  SIparse()->registerFunc("PrptyString",            2,  IFprptyString);
  SIparse()->registerFunc("GetPropertyString",      1,  IFgetPropertyString);
  SIparse()->registerFunc("GetCellPropertyString",  1,  IFgetCellPropertyString);
  SIparse()->registerFunc("PrptyAdd",               3,  IFprptyAdd);
  SIparse()->registerFunc("AddProperty",            2,  IFaddProperty);
  SIparse()->registerFunc("AddCellProperty",        2,  IFaddCellProperty);
  SIparse()->registerFunc("PrptyRemove",            3,  IFprptyRemove);
  SIparse()->registerFunc("RemoveProperty",         2,  IFremoveProperty);
  SIparse()->registerFunc("RemoveCellProperty",     2,  IFremoveCellProperty);

#ifdef HAVE_PYTHON
  py_register_geom2();
#endif
#ifdef HAVE_TCL
  tcl_register_geom2();
#endif
}


//-------------------------------------------------------------------------
// Cells, PCells, Vias, and Instance Placement
//-------------------------------------------------------------------------

// (int) CheckPCellParam(libname, cell, view, pname, value)
//
// The first three arguments specify a parameterized cell.  If libname
// is not given as a scalar 0, it is the name of the OpenAccess
// library containing the pcell super-master, whose name is given in
// the cell argument.  The view argument can be passed a scalar 0 to
// indicate that the OpenAccess view name is "layout", or the actual
// view name can be passed if different.  For Xic native pcells not
// stored in OpenAccess, the library and view should both be 0 (zero).
//
// The pname is a string containing a parameter name for a parameter
// of the specified pcell, and the value argument is either a scalar
// or string value.  The function returns 1 if the value is not
// forbidden by a constraint, 0 otherwise.
//
bool
geom2_funcs::IFcheckPCellParam(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))
    const char *cellname;
    ARG_CHK(arg_string(args, 1, &cellname))
    const char *viewname;
    ARG_CHK(arg_string(args, 2, &viewname))
    const char *pname;
    ARG_CHK(arg_string(args, 3, &pname))
    const char *stringval = 0;
    double doubleval = 0.0;
    res->type = TYP_SCALAR;
    res->content.value = 1;
    bool was_string = false;
    if (args[4].type == TYP_NOTYPE || args[4].type == TYP_STRING) {
        stringval = args[4].content.string;
        was_string = true;
    }
    else if (args[4].type == TYP_SCALAR)
        doubleval = args[4].content.value;
    else {
        Errs()->add_error("CheckPCellParam: bad value argument type.");
        return (BAD);
    }

    char *dbname = 0;
    if (cellname && *cellname) {
        if (!libname || !*libname)
            dbname = PCellDesc::mk_native_dbname(cellname);
        else {
            if (!viewname || !*viewname)
                viewname = "layout";
            dbname = PCellDesc::mk_dbname(libname, cellname, viewname);
        }
    }
    PCellParam *pcp;
    if (PC()->getDefaultParams(dbname, &pcp) && pcp) {
        pcp = pcp->find(pname);
        if (pcp && pcp->constraint()) {
            if (was_string) {
                if (!pcp->constraint()->checkConstraint(stringval))
                    res->content.value = 0;
            }
            else {
                if (!pcp->constraint()->checkConstraint(doubleval))
                    res->content.value = 0;
            }
        }
    }
    delete [] dbname;
    return (OK);
}


// (int) CheckPCellParams(libname, cell, view, params)
//
// The first three arguments specify a parameterized cell.  If libname
// is not given as a scalar 0, it is the name of the OpenAccess
// library containing the pcell super-master, whose name is given in
// the cell argument.  The view argument can be passed a scalar 0 to
// indicate that the OpenAccess view name is "layout", or the actual
// view name can be passed if different.  For Xic native pcells not
// stored in OpenAccess, the library and view should both be 0 (zero).
//
// The params argument is a string providing the parameter values in
// the format of the pc_params property as applied to sub-masters and
// instances.  i.e., values are constants and constraints are not
// included.  The function returns 1 if no parameter has a value
// forbidden by a constraint, 0 otherwise.
//
bool
geom2_funcs::IFcheckPCellParams(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))
    const char *cellname;
    ARG_CHK(arg_string(args, 1, &cellname))
    const char *viewname;
    ARG_CHK(arg_string(args, 2, &viewname))
    const char *prmstr;
    ARG_CHK(arg_string(args, 3, &prmstr))

    res->type = TYP_SCALAR;
    res->content.value = 1;
    if (!prmstr)
        return (OK);

    PCellParam *prm = 0;
    if (!PCellParam::parseParams(prmstr, &prm)) {
        Errs()->add_error(
            "CheckPCellParams: parse error in property string.");
        return (BAD);
    }

    char *dbname = 0;
    if (cellname && *cellname) {
        if (!libname || !*libname)
            dbname = PCellDesc::mk_native_dbname(cellname);
        else {
            if (!viewname || !*viewname)
                viewname = "layout";
            dbname = PCellDesc::mk_dbname(libname, cellname, viewname);
        }
    }
    PCellParam *pcp;
    if (PC()->getDefaultParams(dbname, &pcp)) {
        for (PCellParam *p = prm; p; p = p->next()) {
            PCellParam *pr = pcp->find(p->name());
            if (!pr || !pr->constraint())
                continue;
            if (!pr->constraint()->checkConstraint(p)) {
                res->content.value = 0;
                break;
            }
        }
    }
    delete [] dbname;
    prm->free();
    return (OK);
}


// (int) CreateCell(cellname, [orig_x, orig_y])
//
// This will create a new cell from the contents of the selection
// queue, with the given name, which can not already be in use.  The
// new cell is created in memory only, with the modified flag set so
// as to generate a reminder to the user to save the cell to disk when
// exiting Xic.  This provides functionality similar to the Create
// Cell button in the Edit Menu.
//
// If the optional coordinate pair orig_x and orig_y are given (in
// microns), then this point will be the new cell origin in physical
// mode only.  Otherwise, the lower-left corner of the bounding box of
// the objects will be the new cell origin.  In electrical mode, the
// cell origin is selected to keep contacts on-grid, and the origin
// arguments are ignored.
//
// By default, this function will fail if a cell of the same name
// already exists in memory.  However, if the CrCellOverwrite variable
// is set, existing cells will be overwritten with the new data, and
// the function will succeed.
//
bool
geom2_funcs::IFcreateCell(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))

    bool orig_given = false;
    int orig_x = 0;
    int orig_y = 0;
    if (args[1].type != TYP_ENDARG) {
        ARG_CHK(arg_coord(args, 1, &orig_x, DSP()->CurMode()))
        ARG_CHK(arg_coord(args, 2, &orig_y, DSP()->CurMode()))
        orig_given = true;
    }

    if (!string || !*string)
        return (BAD);
    res->type = TYP_SCALAR;
    if (orig_given) {
        BBox BBo(orig_x, orig_y, orig_x, orig_y);
        res->content.value = ED()->createCell(string, 0, true, &BBo);
    }
    else
        res->content.value = ED()->createCell(string, 0, true, 0);
    return (OK);
}


// (int) CopyCell(oldname, newname)
//
// This function will copy the cell in memory named oldname to
// newname.  The function returns 1 if the operation was successful, 0
// otherwise.  The oldname cell must exist in memory, and the newname
// can not clash with an existing cell or library device.
//
bool
geom2_funcs::IFcopyCell(Variable *res, Variable *args, void*)
{
    const char *oldname;
    ARG_CHK(arg_string(args, 0, &oldname))
    const char *newname;
    ARG_CHK(arg_string(args, 1, &newname))
    if (!oldname || !*oldname || !newname || !*newname)
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = ED()->copySymbol(oldname, newname);
    return (OK);
}


// (int) RenameCell(oldname, newname)
//
// This function will rename the cell in memory named oldname to
// newname, and update all references.  The function returns 1 if the
// operation was successful, 0 otherwise.  The oldname cell must exist
// in memory, and the newname can not clash with an existing cell or
// library device.
//
bool
geom2_funcs::IFrenameCell(Variable *res, Variable *args, void*)
{
    const char *oldname;
    ARG_CHK(arg_string(args, 0, &oldname))
    const char *newname;
    ARG_CHK(arg_string(args, 1, &newname))
    if (!oldname || !*oldname || !newname || !*newname)
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = 0.0;
    if (ED()->renameSymbol(oldname, newname)) {
        EV()->InitCallback();
        XM()->ShowParameters();
        XM()->PopUpCells(0, MODE_UPD);
        XM()->PopUpTree(0, MODE_UPD, 0);
        res->content.value = 1.0;
    }
    return (OK);
}


// (int) DeleteEmpties(recurse)
//
// This function will delete empty cells found in the hierarchy under
// the current cell.  This operation can not be undone.  The argument
// is an integer flag; if zero, one pass is done, and all empty cells
// are deleted.  If the argument is nonzero, additional passes are
// done to delete cells that are newly empty due to their subcells
// being deleted on the previous pass.  The top-level cells is never
// deleted.  The return value is the number of cells deleted.
//
bool
geom2_funcs::IFdeleteEmpties(Variable *res, Variable *args, void*)
{
    bool recurse;
    ARG_CHK(arg_boolean(args, 0, &recurse))

    int dcnt = 0;
    if (DSP()->CurCellName()) {
        CDcbin cbin(DSP()->CurCellName());
        for (;;) {
            bool didone = false;
            stringlist *sl = cbin.listEmpties();
            for (stringlist *s = sl; s; s = s->next) {
                CDcbin tcbin;
                if (CDcdb()->findSymbol(s->string, &tcbin)) {
                    tcbin.deleteCells();
                    didone = true;
                    dcnt++;
                }
            }
            sl->free();
            if (didone) {
                cbin.fixBBs();
                XM()->PopUpCells(0, MODE_UPD);
                XM()->PopUpTree(0, MODE_UPD, 0);
                if (recurse)
                    continue;
            }
            break;
        }
    }
    res->type = TYP_SCALAR;
    res->content.value = dcnt;
    return (OK);
}


namespace {
    bool place_func(const char *name, int x, int y, int ref, double *vals,
        bool use_ap, bool smash, bool use_gui, CDc **cdptr)
    {
        if (cdptr)
            *cdptr = 0;
        int nx = 1;
        int ny = 1;
        int sx = 0;
        int sy = 0;
        if (vals && DSP()->CurMode() == Physical) {
            if (use_ap) {
                nx = ED()->arrayParams().nx();
                ny = ED()->arrayParams().ny();
                sx = ED()->arrayParams().spx();
                sy = ED()->arrayParams().spy();
            }
            else {
                nx = mmRnd(vals[0]);
                if (nx < 1)
                    nx = 1;
                else if (nx > IAP_MAX_ARRAY)
                    nx = IAP_MAX_ARRAY;
                ny = mmRnd(vals[1]);
                if (ny < 1)
                    ny = 1;
                else if (ny > IAP_MAX_ARRAY)
                    ny = IAP_MAX_ARRAY;
                sx = INTERNAL_UNITS(vals[2]);
                sy = INTERNAL_UNITS(vals[3]);
            }
        }

        CDc *cdesc = ED()->placeInstance(name, x, y, nx, ny, sx, sy,
            (PLref)ref, smash, use_gui ? pcpPlaceScr : pcpNone);
        if (!cdesc)
            return (BAD);
        if (cdptr)
            *cdptr = cdesc;
        return (OK);
    }


    bool setup_cur_transform(const char *tfstring, int *x, int *y)
    {
        if (SIlcx()->applyTransform()) {
            cTfmStack stk;
            stk.TPush();
            GEO()->applyCurTransform(&stk, 0, 0,
                SIlcx()->transformX(), SIlcx()->transformY());
            stk.TPoint(x, y);

            sCurTx cx;
            if (!cx.parse_tform_string(tfstring,
                    (DSP()->CurMode() == Electrical))) {
                Errs()->add_error(
                    "instance placement: error parsing transform string.");
                return (BAD);
            }
            GEO()->setCurTx(cx);

            stk.TPush();
            GEO()->applyCurTransform(&stk, 0, 0, 0, 0);
            stk.TPremultiply();

            CDtx tx;
            stk.TCurrent(&tx);
            char *tfstr = tx.tfstring();
            cx.parse_tform_string(tfstr, (DSP()->CurMode() == Electrical));
            GEO()->setCurTx(cx);
            delete [] tfstr;
        }
        else {
            sCurTx cx;
            if (!cx.parse_tform_string(tfstring,
                    (DSP()->CurMode() == Electrical))) {
                Errs()->add_error(
                    "instance placement: error parsing transform string.");
                return (BAD);
            }
            GEO()->setCurTx(cx);
        }
        return (OK);
    }
}


// Place(cellname, x, y [, refpt, array, smash, usegui, tfstring])
//
// This function places an instance of the named cell at x, y.  The
// first argument is of string type and contains the name of the cell
// to place.  The string can consist of two space-separated words.  If
// so, the first word may be a CHD name, an archive file name, or a
// library name (including OpenAccess when available).
//
// The interpretation is similar to the "new" selection in the Open
// command in the File Menu.  In the case of two words, the second
// word is the name of the cell to extract from the source specified
// as the first word.  If only one word is given, it can be an archive
// file name in which case the top-level cell is understood, or a CHD
// name in which case the default cell is understood, or it can be the
// name of a cell available as a native cell from a library or the
// search path, or already exist in memory.
//
// The second two arguments define the placement location, in microns.
//
// The remaining arguments are optional, meaning that they need not be
// given, but all arguments to the left must be given.
//
// The refpt argument is an integer code that specifies the reference
// point which will correspond to x, y after placement.  The values
// can be
//
//    0  the cell origin (the default)
//    1  the lower left corner
//    2  the upper left corner
//    3  the upper right corner
//    4  the lower right corner
//
// The corners are those of the untransformed array or cell.
//
// In electrical mode, if the cell has terminals, this code is
// ignored, and the location of the first terminal is the reference
// point.  If the cell has no terminals, the corner reference points
// are snapped to the nearest grid location.  This is to avoid
// producing off-grid terminal locations.
//
// The array argument, if given, can be a scalar, or the name of an
// array containing four numbers.  This argument specifies the
// arraying parameters for the instance placement, which apply in
// physical mode only.  If a scalar 0 is passed, the placement will
// not be arrayed, which is also the case if this argument does not
// appear and is always true in electrical mode.  If the scalar is
// nonzero, then the placement will use the current array parameters,
// as displayed in the Cell Placement Control pop-up, or set with the
// PlaceSetArrayParams function.  If the argument is the name of an
// array, the array contains the arraying parameters.  These
// parameters are:
//
//    array[0]  NX, integer number in the X direction.
//    array[1]  NY, integer number in the Y direction.
//    array[2]  DX, the real value spacing between cells in the X direction,
//              in microns.
//    array[3]  DY, the real value spacing between cells in the Y direction,
//              in microns.
//
// The NX and NY values will be clipped to the range of 1 through
// 32767.  The DX and DY are edge to adjacent edge spacing, i.e., when
// zero the elements will abut.  If DX or DY is given the negative cell
// width or height, so that all elements appear at the same location,
// the corresponding NX or NY is taken as 1.  Otherwise, there is no
// restriction on DX or DY.
//
// If the boolean value smash is given and nonzero (TRUE), the cell
// will be flattened into the parent, rather than placed as an
// instance.  The flatten-level is 1, so subcells of the cell (if any)
// become subcells of the parent.  This argument is ignored if the
// cell being placed is a parameterized cell (pcell).
//
// The usegui argument applies only when placing a pcell.  If nonzero
// (TRUE), the Parameters panel will appear, and the function will
// block until the user dismisses the panel.  The panel can be used to
// set cell parameters before instantiation.  Initially, the
// parameters will be shown with default values, or values that were
// last given to PlaceSetPCellParams.  If the usegui argument is not
// given or zero (FALSE), the default parameter set as updated with
// parameters given to PlaceSetPCellParams will be used to instantiate
// the cell immediately.
//
// The final argument can be a null string or scalar 0 which is
// equivalent, an empty string, or a transform description in the
// format returned by GetTransformString.  If null or not given, the
// arguemnt is ignored.  In this case, the cell will be transformed
// before placement according to the current transform.  Otherwise,
// the given transformation will be used when placing the instance. 
// An empty string is taken as the identity transform.  If the
// UseTransform mode is in effect, the current transform will be added
// to the string transform, giving an overall transfromation that will
// match geometry placement in this mode.

//
// On success, the function returns 1, 0 otherwise.
//
bool
geom2_funcs::IFplace(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    int x;
    ARG_CHK(arg_coord(args, 1, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 2, &y, DSP()->CurMode()))

    int ref = 0;
    double *vals = 0;
    bool smash = false;
    bool use_ap = false;
    bool use_gui = false;
    const char *tfstring = 0;
    if (args[3].type != TYP_ENDARG) {
        ARG_CHK(arg_int(args, 3, &ref))
        if (args[4].type != TYP_ENDARG) {
            if (args[4].type == TYP_SCALAR && args[4].content.value != 0.0)
                use_ap = true;
            else {
                ARG_CHK(arg_array_if(args, 4, &vals, 4))
            }
            if (args[5].type != TYP_ENDARG) {
                ARG_CHK(arg_boolean(args, 5, &smash))
                if (args[6].type != TYP_ENDARG) {
                    ARG_CHK(arg_boolean(args, 6, &use_gui))
                    if (args[7].type != TYP_ENDARG) {
                        ARG_CHK(arg_string(args, 7, &tfstring))
                    }
                }
            }
        }
    }

    if (!name || !*name)
        return (BAD);
    if (ref < 0 || ref > 4)
        ref = 0;

    CDc *cdesc;
    if (tfstring) {
        sCurTx txbak = *GEO()->curTx();
        if (setup_cur_transform(tfstring, &x, &y) == BAD)
            return (BAD);
        bool ret = place_func(name, x, y, ref, vals, use_ap, smash, use_gui,
                &cdesc);
        GEO()->setCurTx(txbak);
        if (ret == BAD)
            return (BAD);
    }
    else if (place_func(name, x, y, ref, vals, use_ap, smash, use_gui,
            &cdesc) == BAD)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = cdesc ? 1.0 : 0.0;
    return (OK);
}


// PlaceH(cellname, x, y [, refpt, array, smash, usegui, tfstring])
//
// This is similar to the Place function, however it returns a handle
// to the newly created instance.  However, if the smash boolean is
// true or on error, a scalar 0 is returned.
//
bool
geom2_funcs::IFplaceH(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    int x;
    ARG_CHK(arg_coord(args, 1, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 2, &y, DSP()->CurMode()))

    int ref = 0;
    double *vals = 0;
    bool smash = false;
    bool use_ap = false;
    bool use_gui = false;
    const char *tfstring = 0;
    if (args[3].type != TYP_ENDARG) {
        ARG_CHK(arg_int(args, 3, &ref))
        if (args[4].type != TYP_ENDARG) {
            if (args[4].type == TYP_SCALAR && args[4].content.value != 0.0)
                use_ap = true;
            else {
                ARG_CHK(arg_array_if(args, 4, &vals, 4))
            }
            if (args[5].type != TYP_ENDARG) {
                ARG_CHK(arg_boolean(args, 5, &smash))
                if (args[6].type != TYP_ENDARG) {
                    ARG_CHK(arg_boolean(args, 6, &use_gui))
                    if (args[7].type != TYP_ENDARG) {
                        ARG_CHK(arg_string(args, 7, &tfstring))
                    }
                }
            }
        }
    }

    if (!name || !*name)
        return (BAD);
    if (ref < 0 || ref > 4)
        ref = 0;

    CDc *cdesc;
    if (tfstring) {
        sCurTx txbak = *GEO()->curTx();
        if (setup_cur_transform(tfstring, &x, &y) == BAD)
            return (BAD);
        bool ret = place_func(name, x, y, ref, vals, use_ap, smash, use_gui,
                &cdesc);
        GEO()->setCurTx(txbak);
        if (ret == BAD)
            return (BAD);
    }
    else if (place_func(name, x, y, ref, vals, use_ap, smash, use_gui,
            &cdesc) == BAD)
        return (BAD);
    if (cdesc && !smash) {
        CDol *ol = new CDol(cdesc, 0);
        sHdl *hdl = new sHdlObject(ol, CurCell());
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0.0;
    }
    return (OK);
}


// int PlaceSetArrayParams(nx, ny, dx, dy)
//
// This function provides array parameters which may be used when
// instantiating physical cells.  These parameters will appear in the
// Cell Placement Control panel.  The arguments are:
//
//    nx, integer number in the X direction.
//    ny, integer number in the Y direction.
//    dx, the real value spacing between cells in the X direction,
//              in microns.
//    dy, the real value spacing between cells in the Y direction,
//              in microns.
//
// The nx and ny values will be clipped to the range of 1 through
// 32767.  The dx and dy are edge to adjacent edge spacing, i.e., when
// zero the elements will abut.  If dx or dy is given the negative
// cell width or height, so that all elements appear at the same
// location, the corresponding nx or ny is taken as 1.  Otherwise,
// there is no restriction on dx or dy.
//
// The function returns 1 and sets the array parameters in physical
// mode.  In electrical mode, the furnction returns 0 and does
// nothing.
//
bool
geom2_funcs::IFplaceSetArrayParams(Variable *res, Variable *args, void*)
{
    int nx;
    ARG_CHK(arg_int(args, 0, &nx))
    int ny;
    ARG_CHK(arg_int(args, 1, &ny))
    int spx;
    ARG_CHK(arg_coord(args, 2, &spx, Physical))
    int spy;
    ARG_CHK(arg_coord(args, 3, &spy, Physical))

    if (nx < 1)
        nx = 1;
    else if (nx > IAP_MAX_ARRAY)
        nx = IAP_MAX_ARRAY;
    if (ny < 1)
        ny = 1;
    else if (ny > IAP_MAX_ARRAY)
        ny = IAP_MAX_ARRAY;

    res->type = TYP_SCALAR;
    res->content.value = 0;

    if (DSP()->CurMode() == Physical) {
        ED()->setArrayParams(iap_t(nx, ny, spx, spy));
        res->content.value = 1;
        ED()->PopUpPlace(MODE_UPD, false);
    }
    return (OK);
}


// int PlaceSetPCellParams(libname, cell, view, params)
//
// This sets the default parameterized cell (pcell) parameters used
// when instantiating the pcell indicated by the libname/cell/view. 
// If libname is not given as a scalar 0, it is the name of the
// OpenAccess library containing the pcell super-master, whose name is
// given in the cell argument.  The view argument can be passed a
// scalar 0 to indicate that the OpenAccess view name is "layout", or
// the actual view name can be passed if different.  For Xic native
// pcells not stored in OpenAccess, the library and view should both
// be 0 (zero).
//
// The params argument is a string providing the parameter values in
// the format of the pc_params property as applied to sub-masters and
// instances.  i.e., values are constants and constraints are not
// included.  Not all parameters need be given, only those with
// non-default values.
//
// Be aware that there is no immediate constraint testing of the
// parameter values given to this function, though bad values will
// cause subsequent instantiation of the named cell to fail.  The
// CheckPCellParams fuction can be used to validate the params list
// before calling this function.  When giving parameters for
// non-native pcells, it is recommended that the type specification
// prefixes be used, though an attempt is made internally to recognize
// and adapt to differing types.
//
// The saved parameter set will be used for all
// instantiations of the pcell, until changed with another call to
// PlaceSetPCellParams.  The placement is done with the Place script
// function, as for normal cells.
//
// In graphical mode, the given parameter set will initialize the
// Parameters pop-up.
//
// This function manages an internal table of cellname/parameter list
// associations.  If 0 is given for all arguments, the table will be
// cleared.  If the params argument is 0, the specified entry
// will be removed from the table.  When the script
// terminates, parameter lists set with this function will revert to
// the pre-script values.  Entries that were cleared by passing null
// arguments are not reverted, and remain cleared.
//
// The function returns 1 on success, 0 if an error occurred, with an
// error message available from GetError.
//
bool
geom2_funcs::IFplaceSetPCellParams(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))
    const char *cellname;
    ARG_CHK(arg_string(args, 1, &cellname))
    const char *viewname;
    ARG_CHK(arg_string(args, 2, &viewname))
    const char *prmstr;
    ARG_CHK(arg_string(args, 3, &prmstr))

    res->type = TYP_SCALAR;
    res->content.value = 0;

    PCellParam *prm = 0;
    if (prmstr && !PCellParam::parseParams(prmstr, &prm)) {
        Errs()->add_error(
            "PlaceSetPCellParams: parse error in property string.");
        return (BAD);
    }

    char *dbname = 0;
    if (cellname && *cellname) {
        if (!libname || !*libname)
            dbname = PCellDesc::mk_native_dbname(cellname);
        else {
            if (!viewname || !*viewname)
                viewname = "layout";
            dbname = PCellDesc::mk_dbname(libname, cellname, viewname);
        }
    }

    if (!PC()->setPCinstParams(dbname, prm, true)) {
        Errs()->add_error(
            "PlaceSetPCellParams: error setting instantiance parameters.");
        prm->free();
        delete [] dbname;
        return (BAD);
    }
    delete [] dbname;
    res->content.value = 1;
    return (OK);
}


// (int) Replace(cellname, add_xform, array)
//
// This will replace all selected subcells with cellname.  The same
// transformation applied to the previous instance is applied to the
// replacing instance.  In addition, if add_xform is nonzero, the
// current transform will be added.  The function returns 1 if
// successful, 0 if the new cell could not be opened.
//
// The array argument can be a scalar, or the name of an array
// containing four numbers.  This argument specifies the arraying
// parameters for the instance placement, which apply in physical mode
// only.  If a scalar 0 is passed, each placement will retain the same
// arraying parameters as the previous instance.  If the scalar is
// nonzero, then the placement will use the current array parameters,
// as displayed in the Cell Placement Control pop-up, or set with the
// PlaceSetArrayParams function.  If the argument is the name of an
// array, the array contains the arraying parameters.  These
// parameters are:
//
//    array[0]  NX, integer number in the X direction.
//    array[1]  NY, integer number in the Y direction.
//    array[2]  DX, the real value spacing between cells in the X direction,
//              in microns.
//    array[3]  DY, the real value spacing between cells in the Y direction,
//              in microns.
//
// The NX and NY values will be clipped to the range of 1 through
// 32767.  The DX and DY are edge to adjacent edge spacing, i.e., when
// zero the elements will abut.  If DX or DY is given the negative cell
// width or height, so that all elements appear at the same location,
// the corresponding NX or NY is taken as 1.  Otherwise, there is no
// restriction on DX or DY.
//
bool
geom2_funcs::IFreplace(Variable *res, Variable *args, void*)
{
    const char *cellname;
    ARG_CHK(arg_string(args, 0, &cellname))
    bool add_xform;
    ARG_CHK(arg_boolean(args, 1, &add_xform))
    bool use_ap = false;
    double *vals = 0;
    if (args[2].type == TYP_SCALAR && args[2].content.value != 0.0)
        use_ap = true;
    else {
        ARG_CHK(arg_array_if(args, 2, &vals, 4))
    }

    if (!cellname || !*cellname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 1.0;
    CDs *cursd = CurCell();
    if (!cursd)
        return (OK);
    CDcbin cbin;
    sSelGen sg(Selections, cursd, "c");
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (!cbin.cellname()) {
            if (!ED()->openCell(cellname, &cbin, 0)) {
                res->content.value = 0.0;
                return (OK);
            }
        }
        Selections.showUnselected(cursd, od);
        iap_t iap(ED()->arrayParams());
        if (DSP()->CurMode() == Electrical)
            use_ap = 0;
        else if (vals) {
            iap_t p;
            p.set_nx(mmRnd(vals[0]));
            p.set_ny(mmRnd(vals[1]));
            p.set_spx(INTERNAL_UNITS(vals[2]));
            p.set_spy(INTERNAL_UNITS(vals[3]));
            ED()->setArrayParams(p);
            use_ap = true;
        }
        bool ret = ED()->replaceInstance((CDc*)od, &cbin, add_xform, use_ap);
        if (vals && DSP()->CurMode() == Physical)
            ED()->setArrayParams(iap);

        if (!ret) {
            Errs()->add_error("replaceCell failed");
            Log()->ErrorLog("edit operation, in Replace",
                Errs()->get_error());
        }
        sg.remove();
    }
    return (OK);
}


// (string) OpenViaSubMaster(vianame, defnstr)
//
// This function will create if necessary and return the name of a
// standard via sub-master cell in memory.  The first argument is the
// name of a standard via, as defined in the technology file or
// imported from OpenAccess.  The second argument contains a string
// that specifies the parameters that differ from the default values. 
// This can be null or empty if no non-default parameters are used. 
// The format is the same as described for the STDVIA property, with
// the standard via name token stripped.
//
// On success, a name is returned.  One can use this name with the
// Place function to instantiate the via.  Otherwise, a fatal error is
// triggered.
//
bool
geom2_funcs::IFopenViaSubMaster(Variable *res, Variable *args, void*)
{
    const char *vianame;
    ARG_CHK(arg_string(args, 0, &vianame))
    const char *defn;
    ARG_CHK(arg_string(args, 1, &defn))

    if (!vianame || !*vianame) {
        Errs()->add_error("OpenViaSubMaster: null or empty via name");
        return (BAD);
    }
    sLstr lstr;
    lstr.add(vianame);
    lstr.add_c(' ');
    lstr.add(defn);

    CDs *sd = Tech()->OpenViaSubMaster(lstr.string());
    if (!sd) {
        Errs()->add_error(
            "OpenViaSubMaster: creation of %s variant failed", vianame);
        return (BAD);
    }
    res->type = TYP_STRING;
    res->content.string = lstring::copy(sd->cellname()->string());
    res->flags |= VF_ORIGINAL;
    return (OK);
}


//-------------------------------------------------------------------------
// Clipping Functions
//-------------------------------------------------------------------------

namespace {
    bool
    clip_around(Variable *res, int id1, int id2, bool all1, bool all2)
    {
        sHdl *hdl1 = sHdl::get(id1);
        sHdl *hdl2 = sHdl::get(id2);
        res->type = TYP_SCALAR;
        res->content.value = -1;
        if (hdl1 && hdl2) {
            if (hdl1->type != HDLobject)
                return (BAD);
            if (hdl2->type != HDLobject)
                return (BAD);
            if (((sHdlObject*)hdl2)->copies)
                return (OK);
            if (((sHdlObject*)hdl2)->sdesc->isImmutable())
                return (OK);
            CDol *ol1 = (CDol*)hdl1->data;
            CDol *ol2 = (CDol*)hdl2->data;
            CDs *sdesc = ((sHdlObject*)hdl2)->sdesc;
            if (sdesc == CurCell() && ol1 && ol2) {
                if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
                    // These might be deleted
                    DSP()->ShowCellTerminalMarks(ERASE);
                Zlist *zl = 0, *ze = 0;
                BBox cBB(CDnullBB);
                for (CDol *o = ol1; o; o = o->next) {
                    cBB.add(&o->odesc->oBB());
                    Zlist *z = o->odesc->toZlist();
                    if (!zl)
                        zl = ze = z;
                    else {
                        while (ze->next)
                            ze = ze->next;
                        ze->next = z;
                    }
                    if (!all1)
                        break;
                }
                Ylist *yr = new Ylist(zl);
                int cnt = 0;
                CDol *on;
                for (CDol *o = ol2; o; o = on) {
                    if (!all2 && o != ol2)
                        break;
                    on = o->next;
                    if (o->odesc->type() != CDBOX &&
                            o->odesc->type() != CDPOLYGON &&
                            o->odesc->type() != CDWIRE)
                        continue;
                    if (!cBB.intersect(&o->odesc->oBB(), false))
                        continue;

                    Zlist *zo = o->odesc->toZlist();
                    XIrt ret = Zlist::zl_andnot(&zo, yr);
                    if (ret != XIok) {
                        if (ret == XIintr) {
                            SI()->SetInterrupt();
                            return (OK);
                        }
                        return (BAD);
                    }

                    CDl *ld = o->odesc->ldesc();
                    Ulist()->RecordObjectChange(sdesc, o->odesc, 0);
                    // o is now freed

                    if (zo) {
                        PolyList *p0 = Zlist::to_poly_list(zo), *pn;
                        for (PolyList *pp = p0; pp; pp = pn) {
                            pn = pp->next;
                            CDo *newo = 0;
                            if (pp->po.is_rect()) {
                                BBox bBB(pp->po.points);
                                newo = sdesc->newBox(0, &bBB, ld, 0);
                            }
                            else
                                newo = sdesc->newPoly(0, &pp->po, ld, 0, false);
                            if (newo) {
                                hdl2->data = new CDol(newo, (CDol*)hdl2->data);
                                cnt++;
                            }
                            delete pp;
                        }
                    }
                }
                Ylist::destroy(yr);
                if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
                    DSP()->ShowCellTerminalMarks(DISPLAY);
                res->content.value = cnt;
            }
        }
        return (OK);
    }


    bool
    clip_around_copy(Variable *res, int id1, int id2, const char *lname,
        bool all1, bool all2)
    {
        CDs *cursd = CurCell();
        if (!cursd)
            return (false);
        sHdl *hdl1 = sHdl::get(id1);
        sHdl *hdl2 = sHdl::get(id2);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        if (hdl1 && hdl2) {
            if (hdl1->type != HDLobject)
                return (BAD);
            if (hdl2->type != HDLobject)
                return (BAD);
            CDol *ol1 = (CDol*)hdl1->data;
            CDol *ol2 = (CDol*)hdl2->data;
            if (ol1 && ol2) {
                CDl *ldesc = 0;
                if (lname && *lname) {
                    ldesc = CDldb()->findLayer(lname, DSP()->CurMode());
                    if (!ldesc)
                        ldesc = CDldb()->newLayer(lname, DSP()->CurMode());
                    if (!ldesc)
                        return (BAD);
                }
                CDol *o0 = 0, *oend = 0;
                Zlist *zl = 0, *ze = 0;
                BBox cBB(CDnullBB);
                for (CDol *o = ol1; o; o = o->next) {
                    cBB.add(&o->odesc->oBB());
                    Zlist *z = o->odesc->toZlist();
                    if (!zl)
                        zl = ze = z;
                    else {
                        while (ze->next)
                            ze = ze->next;
                        ze->next = z;
                    }
                    if (!all1)
                        break;
                }
                Ylist *yr = new Ylist(zl);
                for (CDol *o = ol2; o; o = o->next) {
                    if (!all2 && o != ol2)
                        break;
                    if (o->odesc->type() != CDBOX &&
                            o->odesc->type() != CDPOLYGON &&
                            o->odesc->type() != CDWIRE)
                        continue;
                    if (!cBB.intersect(&o->odesc->oBB(), false)) {
                        CDo *cdo = o->odesc->copyObject();
                        if (ldesc)
                            cdo->set_ldesc(ldesc);
                        if (!o0)
                            o0 = oend = new CDol(cdo, 0);
                        else {
                            oend->next = new CDol(cdo, 0);
                            oend = oend->next;
                        }
                        continue;
                    }

                    Zlist *zo = o->odesc->toZlist();
                    XIrt ret = Zlist::zl_andnot(&zo, yr);
                    if (ret != XIok) {
                        if (ret == XIintr) {
                            SI()->SetInterrupt();
                            return (OK);
                        }
                        return (BAD);
                    }
                    if (zo) {
                        CDl *ld = ldesc ? ldesc : o->odesc->ldesc();
                        PolyList *p0 = Zlist::to_poly_list(zo);
                        if (!o0)
                            o0 = p0->to_olist(ld, &oend); 
                        else
                            p0->to_olist(ld, &oend); 
                    }
                }
                Ylist::destroy(yr);
                sHdl *hnew = new sHdlObject(o0, cursd, true);
                res->type = TYP_HANDLE;
                res->content.value = hnew->id;
            }
        }
        return (OK);
    }


    bool
    clip_to(Variable *res, int id1, int id2, bool all1, bool all2)
    {
        sHdl *hdl1 = sHdl::get(id1);
        sHdl *hdl2 = sHdl::get(id2);
        res->type = TYP_SCALAR;
        res->content.value = -1;
        if (hdl1 && hdl2) {
            if (hdl1->type != HDLobject)
                return (BAD);
            if (hdl2->type != HDLobject)
                return (BAD);
            if (((sHdlObject*)hdl2)->copies)
                return (OK);
            if (((sHdlObject*)hdl2)->sdesc->isImmutable())
                return (OK);
            CDol *ol1 = (CDol*)hdl1->data;
            CDol *ol2 = (CDol*)hdl2->data;
            CDs *sdesc = ((sHdlObject*)hdl2)->sdesc;
            if (ol1 && ol2 && sdesc == CurCell()) {
                if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
                    // These might be deleted
                    DSP()->ShowCellTerminalMarks(ERASE);
                Zlist *zl = 0, *ze = 0;
                for (CDol *o = ol1; o; o = o->next) {
                    Zlist *z = o->odesc->toZlist();
                    if (!zl)
                        zl = ze = z;
                    else {
                        while (ze->next)
                            ze = ze->next;
                        ze->next = z;
                    }
                    if (all1)
                        break;
                }

                bool isrect = (!zl->next && zl->Z.is_rect());
                Ylist *yl = new Ylist(zl);
                int cnt = 0;
                CDol *on;
                for (CDol *o = ol2; o; o = on) {
                    if (!all2 && o != ol2)
                        break;
                    on = o->next;
                    if (o->odesc->type() != CDBOX &&
                            o->odesc->type() != CDPOLYGON &&
                            o->odesc->type() != CDWIRE)
                        continue;

                    if (isrect) {
                        if (!zl->Z.intersect(&o->odesc->oBB(), false)) {
                            Ulist()->RecordObjectChange(sdesc, o->odesc, 0);
                            // o is now freed
                            continue;
                        }
                        if (o->odesc->oBB().left >= zl->Z.xll &&
                                o->odesc->oBB().bottom >= zl->Z.yl &&
                                o->odesc->oBB().right <= zl->Z.xur &&
                                o->odesc->oBB().top <= zl->Z.yu)
                            continue;
                    }

                    Zlist *zo = o->odesc->toZlist();
                    XIrt ret = Zlist::zl_and(&zo, yl);
                    if (ret != XIok) {
                        if (ret == XIintr) {
                            SI()->SetInterrupt();
                            return (OK);
                        }
                        return (BAD);
                    }

                    CDl *ld = o->odesc->ldesc();
                    Ulist()->RecordObjectChange(sdesc, o->odesc, 0);
                    // o is now freed

                    if (zo) {
                        PolyList *p0 = Zlist::to_poly_list(zo), *pn;
                        for (PolyList *pp = p0; pp; pp = pn) {
                            pn = pp->next;
                            CDo *newo = 0;
                            if (pp->po.is_rect()) {
                                BBox bBB(pp->po.points);
                                newo = sdesc->newBox(0, &bBB, ld, 0);
                            }
                            else
                                newo = sdesc->newPoly(0, &pp->po, ld, 0, false);
                            if (newo) {
                                hdl2->data = new CDol(newo, (CDol*)hdl2->data);
                                cnt++;
                            }
                            delete pp;
                        }
                    }
                }
                Ylist::destroy(yl);
                if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
                    DSP()->ShowCellTerminalMarks(DISPLAY);
                res->content.value = cnt;
            }
        }
        return (OK);
    }


    bool
    clip_to_copy(Variable *res, int id1, int id2, const char *lname,
        bool all1, bool all2)
    {
        CDs *cursd = CurCell();
        if (!cursd)
            return (false);
        sHdl *hdl1 = sHdl::get(id1);
        sHdl *hdl2 = sHdl::get(id2);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        if (hdl1 && hdl2) {
            if (hdl1->type != HDLobject)
                return (BAD);
            if (hdl2->type != HDLobject)
                return (BAD);
            CDol *ol1 = (CDol*)hdl1->data;
            CDol *ol2 = (CDol*)hdl2->data;
            if (ol1 && ol2) {
                CDl *ldesc = 0;
                if (lname && *lname) {
                    ldesc = CDldb()->findLayer(lname, DSP()->CurMode());
                    if (!ldesc)
                        ldesc = CDldb()->newLayer(lname, DSP()->CurMode());
                    if (!ldesc)
                        return (BAD);
                }
                CDol *o0 = 0, *oend = 0;
                Zlist *zl = 0, *ze = 0;
                for (CDol *o = ol1; o; o = o->next) {
                    Zlist *z = o->odesc->toZlist();
                    if (!zl)
                        zl = ze = z;
                    else {
                        while (ze->next)
                            ze = ze->next;
                        ze->next = z;
                    }
                    if (!all1)
                        break;
                }

                bool isrect = (!zl->next && zl->Z.is_rect());
                Ylist *yl = new Ylist(zl);
                for (CDol *o = ol2; o; o = o->next) {
                    if (!all2 && o != ol2)
                        break;
                    if (o->odesc->type() != CDBOX &&
                            o->odesc->type() != CDPOLYGON &&
                            o->odesc->type() != CDWIRE)
                        continue;

                    if (isrect) {
                        if (!zl->Z.intersect(&o->odesc->oBB(), false))
                            continue;
                        if (o->odesc->oBB().left >= zl->Z.xll &&
                                o->odesc->oBB().bottom >= zl->Z.yl &&
                                o->odesc->oBB().right <= zl->Z.xur &&
                                o->odesc->oBB().top <= zl->Z.yu) {
                            CDo *cdo = o->odesc->copyObject();
                            if (ldesc)
                                cdo->set_ldesc(ldesc);
                            if (!o0)
                                o0 = oend = new CDol(cdo, 0);
                            else {
                                oend->next = new CDol(cdo, 0);
                                oend = oend->next;
                            }
                            continue;
                        }
                        if (o->odesc->type() == CDBOX) {
                            CDo *cdo = o->odesc->copyObject();
                            if (ldesc)
                                cdo->set_ldesc(ldesc);
                            BBox tBB(cdo->oBB());
                            if (tBB.left < zl->Z.xll)
                                tBB.left = zl->Z.xll;
                            if (tBB.bottom < zl->Z.yl)
                                tBB.bottom = zl->Z.yl;
                            if (tBB.right > zl->Z.xur)
                                tBB.right = zl->Z.xur;
                            if (tBB.top > zl->Z.yu)
                                tBB.top = zl->Z.yu;
                            cdo->set_oBB(tBB);
                            if (!o0)
                                o0 = oend = new CDol(cdo, 0);
                            else {
                                oend->next = new CDol(cdo, 0);
                                oend = oend->next;
                            }
                            continue;
                        }
                    }

                    Zlist *zo = o->odesc->toZlist();
                    XIrt ret = Zlist::zl_and(&zo, yl);
                    if (ret != XIok) {
                        if (ret == XIintr) {
                            SI()->SetInterrupt();
                            return (OK);
                        }
                        return (BAD);
                    }

                    if (zo) {
                        CDl *ld = ldesc ? ldesc : o->odesc->ldesc();
                        PolyList *p0 = Zlist::to_poly_list(zo);
                        if (!o0)
                            o0 = p0->to_olist(ld, &oend);
                        else
                            p0->to_olist(ld, &oend);
                    }
                }
                Ylist::destroy(yl);
                sHdl *hnew = new sHdlObject(o0, cursd, true);
                res->type = TYP_HANDLE;
                res->content.value = hnew->id;
            }
        }
        return (OK);
    }
}


// (int) ClipAround(object_handle1, all1, object_handle2, all2)
//
// This function will clip out the pieces of objects in the second
// handle list that intersect with objects in the first handle list.
//
// If the boolean value all1 is nonzero, all objects in the first
// handle are used for clipping, otherwise only the first object is
// used.  If the boolean value all2 is nonzero, all objects in the
// second handle list may be clipped, otherwise only the first object
// in the list is a candidate for clipping.  Only boxes, polygons, and
// wires that appear in the second handle list will be clipped.  The
// objects in the first handle list can be of any type, and labels and
// subcells will use the bounding box.  The objects in the second list
// must be database objects, if they are are copies, no clipping is
// performed.  The objects in the first list can be copies.
//
// The newly created objects are added to the front of the second
// handle list, and the original object is removed from the list.  The
// return value is the number of objects created, or -1 if either
// handle is empty or some other error occurred.  The function fails
// if either handle does not reference an object list.
//
bool
geom2_funcs::IFclipAround(Variable *res, Variable *args, void*)
{
    int id1;
    ARG_CHK(arg_handle(args, 0, &id1))
    bool all1;
    ARG_CHK(arg_boolean(args, 1, &all1))
    int id2;
    ARG_CHK(arg_handle(args, 2, &id2))
    bool all2;
    ARG_CHK(arg_boolean(args, 3, &all2))

    return (clip_around(res, id1, id2, all1, all2));
}


// (object_handle) ClipAroundCopy(object_handle1, all1,
//   object_handle2, all2, lname)
//
// This function is similar to ClipAround(), however no new objects
// are created in the database, and neither of the lists passed as
// arguments is altered.  Instead, a new object list handle is
// returned, which references a list of "copies" of objects that are
// created by the clipping.  The new objects are the pieces of the
// object or objects referenced by the second handle that do not
// intersect the object or objects referenced by the first handle.
//
// If the boolean value all1 is nonzero, all objects in the first
// handle are used for clipping, otherwise only the first object is
// used.  If the boolean value all2 is nonzero, all objects in the
// second handle list may be clipped, otherwise only the first object
// in the list is a candidate for clipping.  Only boxes, polygons, and
// wires that appear in the second handle list will be clipped.  The
// objects in the first handle list can be of any type, and labels and
// subcells will use the bounding box.  The objects in the second list
// can be database objects or copies.
//
// If lname is a non-empty string, it is taken as the name for a layer
// on which all of the returned objects will be placed.  The layer
// will be created if it does not exist.  If zero or an empty or null
// string is passed, the object copies will retain the layer of the
// original object from the second handle list.
//
// The returned list can be used by most functions that expect a list
// of objects, however they are not copies of "real" objects.  If no
// new object copy would be created by clipping, the function returns
// 0.  The function will fail if either handle is not an object-list
// handle.
//
bool
geom2_funcs::IFclipAroundCopy(Variable *res, Variable *args, void*)
{
    int id1;
    ARG_CHK(arg_handle(args, 0, &id1))
    bool all1;
    ARG_CHK(arg_boolean(args, 1, &all1))
    int id2;
    ARG_CHK(arg_handle(args, 2, &id2))
    bool all2;
    ARG_CHK(arg_boolean(args, 3, &all2))
    const char *lname;
    ARG_CHK(arg_string(args, 4, &lname))

    return (clip_around_copy(res, id1, id2, lname, all1, all2));
}


// (int) ClipTo(object_handle1, all1, object_handle2, all2)
//
// This function will clip objects referenced by the second handle to
// the boundaries of objects referenced by the first handle.
//
// If the boolean value all1 is nonzero, all objects in the first
// handle are used for clipping, otherwise only the first object is
// used.  If the boolean value all2 is nonzero, all objects in the
// second handle list may be clipped, otherwise only the first object
// in the list is a candidate for clipping.  Only boxes, polygons, and
// wires that appear in the second handle list will be clipped.  The
// objects in the first handle list can be of any type, and labels and
// subcells will use the bounding box.  The objects in the second list
// must be database objects, if they are are copies, no clipping is
// performed.  The objects in the first list can be copies.
//
// The newly created objects are added to the front of the second
// handle list, and the original object is removed from the list.  The
// return value is the number of objects created, or -1 if either
// handle is empty or some other error occurred.  The function fails
// if either handle does not reference an object list.
//
bool
geom2_funcs::IFclipTo(Variable *res, Variable *args, void*)
{
    int id1;
    ARG_CHK(arg_handle(args, 0, &id1))
    bool all1;
    ARG_CHK(arg_boolean(args, 1, &all1))
    int id2;
    ARG_CHK(arg_handle(args, 2, &id2))
    bool all2;
    ARG_CHK(arg_boolean(args, 3, &all2))

    return (clip_to(res, id1, id2, all1, all2));
}


// (object_handle) ClipToCopy(object_handle1, all1, object_handle2, all2,
//   lname)
//
// This function is similar to ClipTo(), however no new objects are
// created in the database, and neither of the lists passed as
// arguments is altered.  Instead, a new object list handle is
// returned, which references a list of "copies" of objects that are
// created by the clipping.  The new objects are the pieces of the
// object or objects referenced by the second handle that intersect
// the object or objects referenced by the first handle.
//
// If the boolean value all1 is nonzero, all objects in the first
// handle are used for clipping, otherwise only the first object is
// used.  If the boolean value all2 is nonzero, all objects in the
// second handle list may be clipped, otherwise only the first object
// in the list is a candidate for clipping.  Only boxes, polygons, and
// wires that appear in the second handle list will be clipped.  The
// objects in the first handle list can be of any type, and labels and
// subcells will use the bounding box.  The objects in the second list
// can be database objects or copies.
//
// If lname is a non-empty string, it is taken as the name for a layer
// on which all of the returned objects will be placed.  The layer
// will be created if it does not exist.  If zero or an empty or null
// string is passed, the object copies will retain the layer of the
// original object from the second handle list.
//
// The returned list can be used by most functions that expect a list
// of objects, however they are not copies of "real" objects.  If no
// new object copy would be created by clipping, the function returns
// 0.  The function will fail if either handle is not an object-list
// handle.
//
bool
geom2_funcs::IFclipToCopy(Variable *res, Variable *args, void*)
{
    int id1;
    ARG_CHK(arg_handle(args, 0, &id1))
    bool all1;
    ARG_CHK(arg_boolean(args, 1, &all1))
    int id2;
    ARG_CHK(arg_handle(args, 2, &id2))
    bool all2;
    ARG_CHK(arg_boolean(args, 3, &all2))
    const char *lname;
    ARG_CHK(arg_string(args, 4, &lname))

    return (clip_to_copy(res, id1, id2, lname, all1, all2));
}


// (int) ClipObjects(object_handle, merge)
//
// This function will clip boxes, polygons, and wires in the list on
// the same layer as the first such object in the list so that none of
// these objects overlap.  Newly created objects are added to the
// front of the handle list, and deleted objects are removed from the
// list.  Objects in the list that are not on the same layer as the
// first box, polygon, or wire or not boxes, polygons or wires are
// ignored.  If the merge argument is nonzero, adjacent new objects
// will be merged, otherwise the pieces will remain separate objects.
// If successful, the number of newly creted objects is returned,
// otherwise -1 is returned.  The function will fail if the handle
// does not reference an object list.
//
bool
geom2_funcs::IFclipObjects(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool merge;
    ARG_CHK(arg_boolean(args, 1, &merge))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        if (((sHdlObject*)hdl)->copies)
            return (OK);
        if (((sHdlObject*)hdl)->sdesc->isImmutable())
            return (OK);
        CDol *ol = (CDol*)hdl->data;
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        if (sdesc == CurCell()) {
            if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
                // These might be deleted
                DSP()->ShowCellTerminalMarks(ERASE);
            Zlist *zl = 0, *ze = 0;
            CDl *ld = 0;
            for (CDol *o = ol; o; o = o->next) {
                if (o->odesc->type() != CDBOX &&
                        o->odesc->type() != CDPOLYGON &&
                        o->odesc->type() != CDWIRE)
                    continue;
                if (!ld)
                    ld = o->odesc->ldesc();
                else if (o->odesc->ldesc() != ld)
                    continue;
                Zlist *z = o->odesc->toZlist();
                if (!zl)
                    zl = ze = z;
                else {
                    while (ze->next)
                        ze = ze->next;
                    ze->next = z;
                }
            }
            try {
                zl = Zlist::repartition(zl);
            }
            catch (XIrt ret) {
                if (ret == XIintr) {
                    SI()->SetInterrupt();
                    return (OK);
                }
                return (BAD);
            }
            int cnt = 0;
            if (zl) {
                CDol *on;
                for (CDol *o = ol; o; o = on) {
                    on = o->next;
                    if (o->odesc->ldesc() != ld)
                        continue;
                    if (o->odesc->type() != CDBOX &&
                            o->odesc->type() != CDPOLYGON &&
                            o->odesc->type() != CDWIRE)
                        continue;
                    Ulist()->RecordObjectChange(sdesc, o->odesc, 0);
                }
                if (merge) {
                    PolyList *p0 = Zlist::to_poly_list(zl);
                    for (PolyList *pp = p0; pp; pp = pp->next) {
                        CDo *newo = sdesc->newPoly(0, &pp->po, ld, 0, false);
                        if (newo) {
                            hdl->data = new CDol(newo, (CDol*)hdl->data);
                            cnt++;
                        }
                    }
                    PolyList::destroy(p0);
                }
                else {
                    for (Zlist *z = zl; z; z = z->next) {
                        if (z->Z.xll == z->Z.xul && z->Z.xlr == z->Z.xur) {
                            BBox BB;
                            z->Z.BB(&BB);
                            CDo *newo = sdesc->newBox(0, &BB, ld, 0);
                            if (newo) {
                                hdl->data = new CDol(newo, (CDol*)hdl->data);
                                cnt++;
                            }
                        }
                        else {
                            Poly po;
                            if (z->Z.mkpoly(&po.points, &po.numpts, false)) {
                                CDo *newo = sdesc->newPoly(0, &po, ld, 0,
                                    false);
                                if (newo) {
                                    hdl->data =
                                        new CDol(newo, (CDol*)hdl->data);
                                    cnt++;
                                }
                            }
                        }
                    }
                    Zlist::destroy(zl);
                }
            }
            res->content.value = cnt;
        }
    }
    return (OK);
}


// (object_handle)
//    ClipIntersectCopy(object_handle1, all1, object_handle2, all2, lname)
//
// This function returns a list of object copies which represent the
// exclusive-or of box, polygon, and wire objects in the two object
// lists passed.  The lists are not altered in any way, and the new
// objects, being "copies", are not added to the database.  Objects
// found in the lists that are not boxes, polygons, or wires are
// ignored.  The new objects are placed on the layer with the name
// given in lname, which is created if it does not exist, independent
// of the originating layer of the objects.  If a null string or 0 is
// passed for lname, the target layer is the first layer found in
// object_handle1, or object_handle2 if object_handle1 is empty.  The
// all1 and all2 are integer arguments indicating whether to use only
// the first object in the list, or all objects in the list.  If
// nonzero, then all boxes, polygons, and wires in the corresponding
// list will be used, otherwise only the first box, polygon, or wire
// will be processed.  On success, a handle to a list of object copies
// is returned, zero is returned otherwise.  A fatal error is
// triggered if either argument is not a handle to a list of objects.
//
bool
geom2_funcs::IFclipIntersectCopy(Variable *res, Variable *args, void*)
{
    int id1;
    ARG_CHK(arg_handle(args, 0, &id1))
    bool all1;
    ARG_CHK(arg_boolean(args, 1, &all1))
    int id2;
    ARG_CHK(arg_handle(args, 2, &id2))
    bool all2;
    ARG_CHK(arg_boolean(args, 3, &all2))
    const char *lname;
    ARG_CHK(arg_string(args, 4, &lname))

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    sHdl *hdl1 = sHdl::get(id1);
    sHdl *hdl2 = sHdl::get(id2);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl1 && hdl2) {
        if (hdl1->type != HDLobject)
            return (BAD);
        if (hdl2->type != HDLobject)
            return (BAD);
        CDol *ol1 = (CDol*)hdl1->data;
        CDol *ol2 = (CDol*)hdl2->data;
        CDl *ldesc = 0;
        if (lname && *lname) {
            ldesc = CDldb()->findLayer(lname, DSP()->CurMode());
            if (!ldesc)
                ldesc = CDldb()->newLayer(lname, DSP()->CurMode());
            if (!ldesc)
                return (BAD);
        }

        Zlist *zl1 = 0, *ze = 0;
        for (CDol *o = ol1; o; o = o->next) {
            if (o->odesc->type() != CDBOX &&
                    o->odesc->type() != CDPOLYGON &&
                    o->odesc->type() != CDWIRE)
                continue;
            if (!ldesc)
                ldesc = o->odesc->ldesc();
            Zlist *z = o->odesc->toZlist();
            if (!zl1)
                zl1 = ze = z;
            else
                ze->next = z;
            if (ze) {
                while (ze->next)
                    ze = ze->next;
            }
            if (!all1)
                break;
        }
        try {
            zl1 = Zlist::repartition(zl1);
        }
        catch (XIrt ret) {
            if (ret == XIintr) {
                SI()->SetInterrupt();
                return (OK);
            }
            return (BAD);
        }

        Zlist *zl2 = 0;
        ze = 0;
        for (CDol *o = ol2; o; o = o->next) {
            if (o->odesc->type() != CDBOX &&
                    o->odesc->type() != CDPOLYGON &&
                    o->odesc->type() != CDWIRE)
                continue;
            if (!ldesc)
                ldesc = o->odesc->ldesc();
            Zlist *z = o->odesc->toZlist();
            if (!zl2)
                zl2 = ze = z;
            else
                ze->next = z;
            if (ze) {
                while (ze->next)
                    ze = ze->next;
            }
            if (!all2)
                break;
        }
        try {
            zl2 = Zlist::repartition(zl2);
        }
        catch (XIrt ret) {
            if (ret == XIintr) {
                SI()->SetInterrupt();
                return (OK);
            }
            return (BAD);
        }

        XIrt ret = Zlist::zl_andnot2(&zl1, &zl2);
        if (ret != XIok) {
            if (ret == XIintr) {
                SI()->SetInterrupt();
                return (OK);
            }
            return (BAD);
        }

        CDol *o0 = 0, *oend = 0;
        if (zl1) {
            PolyList *p0 = Zlist::to_poly_list(zl1);
            if (!o0)
                o0 = p0->to_olist(ldesc, &oend); 
            else
                p0->to_olist(ldesc, &oend); 
        }
        if (zl2) {
            PolyList *p0 = Zlist::to_poly_list(zl2);
            if (!o0)
                o0 = p0->to_olist(ldesc, &oend); 
            else
                p0->to_olist(ldesc, &oend); 
        }
        sHdl *hnew = new sHdlObject(o0, cursd, true);
        res->type = TYP_HANDLE;
        res->content.value = hnew->id;
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Other object Management Functions
//-------------------------------------------------------------------------

namespace {
    // Layer argument, if not null must match a layer name
    //
    bool
    arg_layer_zok(Variable *args, int ix, CDl **ldp)
    {
        const char *lstring;
        if (!arg_string(args, ix, &lstring))
            return (false);
        if (!lstring || !*lstring) {
            *ldp = 0;
            return (true);
        }
        CDl *ld = CDldb()->findLayer(lstring, DSP()->CurMode());
        if (!ld) {
            Errs()->add_error("layer %s not found.", lstring);
            return (false);
        }
        *ldp = ld;
        return (true);
    }
}


// (int) ChangeLayer()
//
// This function will change the layer of all selected geometry to the
// current layer.  This is similar to the functionality of the Chg
// Layer button in the Edit Menu.
//
bool
geom2_funcs::IFchangeLayer(Variable *res, Variable*, void*)
{
    ED()->changeLayer();
    res->type = TYP_SCALAR;
    res->content.value = 1.0;
    return (OK);
}


// (int) Bloat(dimen, mode)
//
// Each selected object is bloated by the given dimension, similar to
// the !bloat command.  The returned value is 0 on success, or 1 if
// there was a runtime error.  This function will return 1 if not
// called in physical mode.
//
// The second argument is an integer that specifies the algorithm to
// use for bloating.  Giving zero specifies the default algorithm,
// which rounds corners but is rather complex and slow.  See the
// description of the !bloat command for documentation of the
// algorithms available.
//
bool
geom2_funcs::IFbloat(Variable *res, Variable *args, void*)
{
    int dimen;
    ARG_CHK(arg_coord(args, 0, &dimen, DSP()->CurMode()))
    int mode;
    ARG_CHK(arg_int(args, 1, &mode))

    double ret = 0.0;
    if (DSP()->CurMode() == Physical) {
        if (ED()->bloatQueue(dimen, mode) != XIok)
            ret = 1.0;
    }
    else
        ret = 1.0;
    res->type = TYP_SCALAR;
    res->content.value = ret;
    return (OK);
}


// (int) Manhattanize(dimen, mode)
//
// Each selected non-Manhattan polygon or wire is converted to a
// Manhattan polygon or box approximation, similar to the !manh
// command.  The first argument is a size in microns representing the
// smallest dimension of the boxes created to approximate the
// non-Manhattan parts.  The second argument is a boolean value that
// specifies which of two algorithms to use.  These algorithms are
// described with the !manh command.
//
// The returned value is 0 on success, or 1 if there was a runtime
// error.  This function will return 1 if not called in physical mode.
// The function will fail if the dimen argument is smaller than 0.01.
//
bool
geom2_funcs::IFmanhattanize(Variable *res, Variable *args, void*)
{
    int dimen;
    ARG_CHK(arg_coord(args, 0, &dimen, DSP()->CurMode()))
    int mode;
    ARG_CHK(arg_int(args, 1, &mode))

    if (dimen < 10)
        return (BAD);
    double ret = 0.0;
    if (DSP()->CurMode() == Physical && dimen >= 10) {
        if (ED()->manhattanizeQueue(dimen, mode) != XIok)
            ret = 1.0;
    }
    else
        ret = 1.0;
    res->type = TYP_SCALAR;
    res->content.value = ret;
    return (OK);
}


// (int) Join()
//
// The selected objects that touch or overlap are merged together into
// polygons, similar to the !join command.  The returned value is 0 on
// success, 1 if there is a runtime error.  This function will return
// 1 if not called in physical mode.
//
bool
geom2_funcs::IFjoin(Variable *res, Variable*, void*)
{
    double ret = 0.0;
    if (DSP()->CurMode() == Physical) {
        if (Selections.hasTypes(CurCell(Physical), "bpw"))
            ED()->joinQueue();
    }
    else
        ret = 1.0;
    res->type = TYP_SCALAR;
    res->content.value = ret;
    return (OK);
}


// (int) Decompose(vert)
//
// The selected polygons and wires are decomposed into elemental
// non-overlapping trapezoids (polygons) similar to the !split
// command.  If the integer argument is nonzero, the decomposition favors
// a vertical orientation, otherwise the splitting favors horizontal.
// The returned value is 0 if called in physical mode, 1 if
// not called in physical mode (an error).
//
bool
geom2_funcs::IFdecompose(Variable *res, Variable *args, void*)
{
    bool vert;
    ARG_CHK(arg_boolean(args, 0, &vert))

    double ret = 0.0;
    if (DSP()->CurMode() == Physical) {
        if (Selections.hasTypes(CurCell(Physical), "bpw"))
            ED()->splitQueue(vert);
    }
    else
        ret = 1.0;
    res->type = TYP_SCALAR;
    res->content.value = ret;
    return (OK);
}

namespace {
    bool box_func(BBox &BB, CDo **odptr)
    {
        if (odptr)
            *odptr = 0;
        CDs *cursd = CurCell();
        if (!cursd) {
            if (!CD()->ReopenCell(DSP()->CurCellName()->string(),
                    DSP()->CurMode()) || !(cursd = CurCell()))
                return (BAD);
        }
        if (BB.width() == 0 || BB.height() == 0)
            return (OK);
        if (SIlcx()->applyTransform()) {
            cTfmStack stk;
            stk.TPush();
            GEO()->applyCurTransform(&stk, 0, 0,
                SIlcx()->transformX(), SIlcx()->transformY());
            Poly poly;
            stk.TBB(&BB, &poly.points);
            stk.TPop();

            if (poly.points) {
                poly.numpts = 5;
                if (GEO()->curTx()->magset()) {
                    Point::scale(poly.points, 5, GEO()->curTx()->magn(),
                        SIlcx()->transformX(), SIlcx()->transformY());
                }
                CDo *odesc = cursd->newPoly(0, &poly, LT()->CurLayer(), 0,
                    false);
                if (!odesc)
                    return (BAD);
                if (odptr)
                    *odptr = odesc;
            }
            else {
                if (GEO()->curTx()->magset())
                    BB.scale(GEO()->curTx()->magn(), SIlcx()->transformX(),
                        SIlcx()->transformY());
                CDo *odesc = cursd->newBox(0, &BB, LT()->CurLayer(), 0);
                if (!odesc)
                    return (BAD);
                if (odptr)
                    *odptr = odesc;
            }
        }
        else {
            CDo *odesc = cursd->newBox(0, &BB, LT()->CurLayer(), 0);
            if (!odesc)
                return (BAD);
            if (odptr)
                *odptr = odesc;
        }
        return (OK);
    }
}


// (int) Box(left, bottom, right, top)
//
// The four arguments are real values specifying the coordinates of a
// rectangle.  Calling this function will generate a box on the
// current layer with the given coordinates.  This provides
// functionality similar to the box menu button.
//
bool
geom2_funcs::IFbox(Variable *res, Variable *args, void*)
{
    BBox BB;
    ARG_CHK(arg_coord(args, 0, &BB.left, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 1, &BB.bottom, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 2, &BB.right, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 3, &BB.top, DSP()->CurMode()))
    BB.fix();

    CDo *odesc;
    if (box_func(BB, &odesc) == BAD)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = odesc ? 1.0 : 0.0;
    return (OK);
}


// (object_handle) BoxH(left, bottom, right, top)
//
// This is similar to the Box function, but will return a handle to
// the new object.  On error, a scalar 0 is returned.
//
bool
geom2_funcs::IFboxH(Variable *res, Variable *args, void*)
{
    BBox BB;
    ARG_CHK(arg_coord(args, 0, &BB.left, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 1, &BB.bottom, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 2, &BB.right, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 3, &BB.top, DSP()->CurMode()))
    BB.fix();

    CDo *odesc;
    if (box_func(BB, &odesc) == BAD)
        return (BAD);
    if (odesc) {
        CDol *ol = new CDol(odesc, 0);
        sHdl *hdl = new sHdlObject(ol, CurCell());
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0.0;
    }
    return (OK);
}


namespace {
    bool polygon_func(Poly &poly, double *vals, CDo **odptr)
    {
        if (odptr)
            *odptr = 0;
        CDs *cursd = CurCell();
        if (!cursd) {
            if (!CD()->ReopenCell(DSP()->CurCellName()->string(),
                    DSP()->CurMode()) || !(cursd = CurCell()))
                return (BAD);
        }
        poly.points = new Point[poly.numpts];
        int i, j;
        for (j = i = 0; i < poly.numpts; i++, j += 2)
            poly.points[i].set(INTERNAL_UNITS(vals[j]),
                INTERNAL_UNITS(vals[j+1]));
        if (SIlcx()->applyTransform()) {
            cTfmStack stk;
            stk.TPush();
            GEO()->applyCurTransform(&stk, 0, 0,
                SIlcx()->transformX(), SIlcx()->transformY());
            stk.TPath(poly.numpts, poly.points);
            stk.TPop();
            BBox BB;
            if (poly.to_box(&BB)) {
                delete [] poly.points;
                if (GEO()->curTx()->magset())
                    BB.scale(GEO()->curTx()->magn(), SIlcx()->transformX(),
                        SIlcx()->transformY());
                CDo *odesc = cursd->newBox(0, &BB, LT()->CurLayer(), 0);
                if (!odesc)
                    return (BAD);
                if (odptr)
                    *odptr = odesc;
            }
            else {
                if (GEO()->curTx()->magset()) {
                    Point::scale(poly.points, poly.numpts,
                        GEO()->curTx()->magn(),
                        SIlcx()->transformX(), SIlcx()->transformY());
                }
                CDo *odesc = cursd->newPoly(0, &poly, LT()->CurLayer(), 0,
                    false);
                if (!odesc)
                    return (BAD);
                if (odptr)
                    *odptr = odesc;
            }
        }
        else {
            CDo *odesc = cursd->newPoly(0, &poly, LT()->CurLayer(), 0, false);
            if (!odesc)
                return (BAD);
            if (odptr)
                *odptr = odesc;
        }
        return (OK);
    }
}


// (int) Polygon(num, arraypts)
//
// This function creates a polygon on the current layer.  The second
// argument is an array of values, taken as x-y pairs.  The first pair
// of values must be the same as the last, i.e., the path must be
// closed.  The first argument is the number of pairs of coordinates
// in the array.
//
bool
geom2_funcs::IFpolygon(Variable *res, Variable *args, void*)
{
    Poly poly;
    ARG_CHK(arg_int(args, 0, &poly.numpts))
    if (poly.numpts < 4)
        return (BAD);
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 2*poly.numpts))

    CDo *odesc;
    if (polygon_func(poly, vals, &odesc) == BAD)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = odesc ? 1.0 : 0.0;
    return (OK);
}


// (object_handle) PolygonH(num, arraypts)
//
// This is similar to the Polygon function, but will return a handle to
// the new object.  On error, a scalar 0 is returned.
//
bool
geom2_funcs::IFpolygonH(Variable *res, Variable *args, void*)
{
    Poly poly;
    ARG_CHK(arg_int(args, 0, &poly.numpts))
    if (poly.numpts < 4)
        return (BAD);
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 2*poly.numpts))

    CDo *odesc;
    if (polygon_func(poly, vals, &odesc) == BAD)
        return (BAD);
    if (odesc) {
        CDol *ol = new CDol(odesc, 0);
        sHdl *hdl = new sHdlObject(ol, CurCell());
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0.0;
    }
    return (OK);
}


namespace {
    bool arc_func(int x, int y, int r1x, int r1y, int r2x, int r2y,
        double astart, double aend, CDo **odptr)
    {
        if (odptr)
            *odptr = 0;
        CDs *cursd = CurCell();
        if (!cursd) {
            if (!CD()->ReopenCell(DSP()->CurCellName()->string(),
                    DSP()->CurMode()) || !(cursd = CurCell()))
                return (BAD);
        }
        astart *= (M_PI/180.0);
        aend *= (M_PI/180.0);

        int ss = GEO()->spotSize();
        if (ss < 0)
            GEO()->setSpotSize(INTERNAL_UNITS(Tech()->MfgGrid()));
        Poly poly;
        poly.points = GEO()->makeArcPath(&poly.numpts, cursd->isElectrical(),
            x, y, r1x, r1y, r2x, r2y, astart, aend);
        GEO()->setSpotSize(ss);

        if (SIlcx()->applyTransform()) {
            cTfmStack stk;
            stk.TPush();
            GEO()->applyCurTransform(&stk, 0, 0,
                SIlcx()->transformX(), SIlcx()->transformY());
            stk.TPath(poly.numpts, poly.points);
            stk.TPop();
            if (GEO()->curTx()->magset()) {
                Point::scale(poly.points, poly.numpts, GEO()->curTx()->magn(),
                    SIlcx()->transformX(), SIlcx()->transformY());
            }
        }
        CDo *odesc = cursd->newPoly(0, &poly, LT()->CurLayer(), 0, false);
        if (!odesc)
            return (BAD);
        if (odptr)
            *odptr = odesc;
        return (OK);
    }
}


// (int) Arc(x, y, rad1X, rad1Y, rad2X, rad2Y, ang_start, ang_end)
//
// This produces a circular or elliptical solid or ring-like figure,
// providing functionality similar to the round, donut, and arc
// buttons in the physical side menu.
//
//  x, y            center coordinates
//  rad1X, rad1Y    x and y inner radii
//  rad2X, rad2Y    x and y outer radii
//  ang_start       starting angle in degrees
//  ang_end         ending angle in degrees
//
// All dimensions are given in microns.  The first two arguments
// provide the center coordinates.  The second two arguments are the
// inner radius in the X and Y directions.  If these differ, the
// inner radus will be elliptical, otherwise it will be circular.  If
// both are zero, the figure will not have an inner surface.
//
// Similarly, the next two arguments specify the outer radius, X and
// Y directions separately.  Both are required to be larger than the
// inner radius counterpart.
//
// The final two arguments are the start and end angle, given in
// degrees.  An angle 0 points along the X axis, and the angle is
// measured clockwise.
//
// If ang_start and ang_end are equal, a donut (ring figure) is
// produced.  If the outer and inner radii are equal, a solid figure
// is produced.  Angles are defined from the positive x-axis, in a
// counter-clockwise sense.  The arc is generated in a clockwise
// direction.
//
// If the UseTransform function has been called to enable use of the
// current transform, the current transform will be applied to the
// arc coordinates before the arc is created.  The translation
// supplied to UseTransform is added to the coordinates before the
// current transform is applied.
//
// The function returns 1 on success, 0 otherwise.
//
bool
geom2_funcs::IFarc(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, DSP()->CurMode()))
    int r1x;
    ARG_CHK(arg_coord(args, 2, &r1x, DSP()->CurMode()))
    int r1y;
    ARG_CHK(arg_coord(args, 3, &r1y, DSP()->CurMode()))
    int r2x;
    ARG_CHK(arg_coord(args, 4, &r2x, DSP()->CurMode()))
    int r2y;
    ARG_CHK(arg_coord(args, 5, &r2y, DSP()->CurMode()))
    double astart;
    ARG_CHK(arg_real(args, 6, &astart))
    double aend;
    ARG_CHK(arg_real(args, 7, &aend))

    CDo *odesc;
    if (arc_func(x, y, r1x, r1y, r2x, r2y, astart, aend, &odesc) == BAD)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = odesc ? 1.0 : 0.0;
    return (OK);
}

// (object_handle) ArcH(x, y, rad1X, rad1Y, rad2X, rad2Y, ang_start, ang_end)
//
// This is similar to the Arc function, but will return a handle to
// the new object.  On error, a scalar 0 is returned.
//
bool
geom2_funcs::IFarcH(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, DSP()->CurMode()))
    int r1x;
    ARG_CHK(arg_coord(args, 2, &r1x, DSP()->CurMode()))
    int r1y;
    ARG_CHK(arg_coord(args, 3, &r1y, DSP()->CurMode()))
    int r2x;
    ARG_CHK(arg_coord(args, 4, &r2x, DSP()->CurMode()))
    int r2y;
    ARG_CHK(arg_coord(args, 5, &r2y, DSP()->CurMode()))
    double astart;
    ARG_CHK(arg_real(args, 6, &astart))
    double aend;
    ARG_CHK(arg_real(args, 7, &aend))

    CDo *odesc;
    if (arc_func(x, y, r1x, r1y, r2x, r2y, astart, aend, &odesc) == BAD)
        return (BAD);
    if (odesc) {
        CDol *ol = new CDol(odesc, 0);
        sHdl *hdl = new sHdlObject(ol, CurCell());
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0.0;
    }
    return (OK);
}


// (int) Round(x, y, radius)
//
// This a simplification of the Arc function which simply creates a
// circular disk object at the location specified in the first two
// arguments.  All dimensions are in microns.  The third argument
// specifies the radius.
//
// The function returns 1 on success, 0 otherwise.
//
bool
geom2_funcs::IFround(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, DSP()->CurMode()))
    int rad;
    ARG_CHK(arg_coord(args, 2, &rad, DSP()->CurMode()))

    CDo *odesc;
    if (arc_func(x, y, 0, 0, rad, rad, 0.0, 360.0, &odesc) == BAD)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = odesc ? 1.0 : 0.0;
    return (OK);
}


// (object_handle) RoundH(x, y, radius)
//
// This is similar to the Round function, but will return a handle to
// the new object.  On error, a scalar 0 is returned.
//
bool
geom2_funcs::IFroundH(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, DSP()->CurMode()))
    int rad;
    ARG_CHK(arg_coord(args, 2, &rad, DSP()->CurMode()))

    CDo *odesc;
    if (arc_func(x, y, 0, 0, rad, rad, 0.0, 360.0, &odesc) == BAD)
        return (BAD);
    if (odesc) {
        CDol *ol = new CDol(odesc, 0);
        sHdl *hdl = new sHdlObject(ol, CurCell());
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0.0;
    }
    return (OK);
}


// (int) HalfRound(x, y, radius, dir)
//
// This is a simplification of the Arc function which creates a
// half-circular figure.  The first two arguments indicate the center
// of an equivalent full circle, i.e., it is the midpoint of the flat
// edge.  The dir argument is an integer 0-7 which specifies the
// orientation, in increments of 45 degrees.  With 0, the flat
// section is horizontal with the curved surface on top.  The dir
// rotates clockwise, so that a value of 2 would produce a figure
// that looks like the letter D.
//
// The function returns 1 on success, 0 otherwise.
//
bool
geom2_funcs::IFhalfRound(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, DSP()->CurMode()))
    int rad;
    ARG_CHK(arg_coord(args, 2, &rad, DSP()->CurMode()))
    int dir;
    ARG_CHK(arg_int(args, 3, &dir))
    if (dir < 0)
        dir = 0;
    else if (dir > 7)
        dir = 7;
    double astart = 180.0 - dir*45.0;
    double aend = astart + 180.0;

    CDo *odesc;
    if (arc_func(x, y, 0, 0, rad, rad, astart, aend, &odesc) == BAD)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = odesc ? 1.0 : 0.0;
    return (OK);
}


// (object_handle) HalfRoundH(x, y, radius, dir)
//
// This is similar to the HalfRound function, but will return a
// handle to the new object.  On error, a scalar 0 is returned.
//
bool
geom2_funcs::IFhalfRoundH(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, DSP()->CurMode()))
    int rad;
    ARG_CHK(arg_coord(args, 2, &rad, DSP()->CurMode()))
    int dir;
    ARG_CHK(arg_int(args, 3, &dir))
    if (dir < 0)
        dir = 0;
    else if (dir > 7)
        dir = 7;
    double astart = 180.0 - dir*45.0;
    double aend = astart + 180.0;

    CDo *odesc;
    if (arc_func(x, y, 0, 0, rad, rad, astart, aend, &odesc) == BAD)
        return (BAD);
    if (odesc) {
        CDol *ol = new CDol(odesc, 0);
        sHdl *hdl = new sHdlObject(ol, CurCell());
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0.0;
    }
    return (OK);
}


// (int) Sides(numsides)
//
// This sets the number of segments to use in generating round
// objects, for the current display mode (electrical or physical). 
// The function returns the present value for this parameter.  This is
// similar to the sides menu button in physical mode.  It simply sets
// the RoundFlashSides variable, or clears the variable if the number
// of sides given is the default.  Similarly, in electrical mode it is
// similar to the sides entry in the shapes menu, and sets or clears
// the ElecRoundFlashSides variable.
//
bool
geom2_funcs::IFsides(Variable *res, Variable *args, void*)
{
    int sides;
    ARG_CHK(arg_int(args, 0, &sides))

    bool elec = (DSP()->CurMode() == Electrical);
    res->type = TYP_SCALAR;
    res->content.value = GEO()->roundFlashSides(elec);
    if (sides >= MIN_RoundFlashSides && sides <= MAX_RoundFlashSides) {
        if (elec) {
            if (sides == DEF_RoundFlashSides)
                CDvdb()->clearVariable(VA_ElecRoundFlashSides);
            else {
                char buf[32];
                sprintf(buf, "%d", sides);
                CDvdb()->setVariable(VA_ElecRoundFlashSides, buf);
            }
        }
        else {
            if (sides == DEF_RoundFlashSides)
                CDvdb()->clearVariable(VA_RoundFlashSides);
            else {
                char buf[32];
                sprintf(buf, "%d", sides);
                CDvdb()->setVariable(VA_RoundFlashSides, buf);
            }
        }
    }
    return (OK);
}


namespace {
    bool wire_func(Wire &wire, int width, double *vals, int style,
        CDo **odptr)
    {
        if (odptr)
            *odptr = 0;
        CDs *cursd = CurCell();
        if (!cursd) {
            if (!CD()->ReopenCell(DSP()->CurCellName()->string(),
                    DSP()->CurMode()) || !(cursd = CurCell()))
                return (BAD);
        }
        wire.set_wire_width(width);
        if (style < (int)CDWIRE_FLUSH || style > (int)CDWIRE_EXTEND)
            wire.set_wire_style(CDWIRE_EXTEND);
        else
            wire.set_wire_style((WireStyle)style);
        wire.points = new Point[wire.numpts];
        int i, j;
        for (j = i = 0; i < wire.numpts; i++, j += 2) {
            wire.points[i].set(INTERNAL_UNITS(vals[j]),
                INTERNAL_UNITS(vals[j+1]));
        }
        if (SIlcx()->applyTransform()) {
            cTfmStack stk;
            stk.TPush();
            GEO()->applyCurTransform(&stk, 0, 0,
                SIlcx()->transformX(), SIlcx()->transformY());
            stk.TPath(wire.numpts, wire.points);
            stk.TPop();
            if (GEO()->curTx()->magset()) {
                Point::scale(wire.points, wire.numpts, GEO()->curTx()->magn(),
                    SIlcx()->transformX(), SIlcx()->transformY());
                if (!ED()->noWireWidthMag())
                    wire.set_wire_width(mmRnd(wire.wire_width() *
                        GEO()->curTx()->magn()));
            }
        }
        CDo *odesc = cursd->newWire(0, &wire, LT()->CurLayer(), 0, false);
        if (!odesc)
            return (BAD);
        if (odptr)
            *odptr = odesc;
        return (OK);
    }
}


// (int) Wire(width, num, arraypts, end_style)
//
// This function creates a wire on the current layer.  The first
// argument is the width of the wire.  The third argument is the name
// of an array of coordinates, taken as x-y pairs.  The second
// argument is the number of coordinate pairs in the array.  The
// fourth argument is 0, 1, or 2 to set the end style to flush,
// rounded, or extended, respectively.  This provides the
// functionality of the wire menu button.
//
bool
geom2_funcs::IFwire(Variable *res, Variable *args, void*)
{
    Wire wire;
    int width;
    ARG_CHK(arg_coord(args, 0, &width, DSP()->CurMode()))
    ARG_CHK(arg_int(args, 1, &wire.numpts))
    double *vals;
    ARG_CHK(arg_array(args, 2, &vals, 2*wire.numpts))
    int style;
    ARG_CHK(arg_int(args, 3, &style))

    CDo *odesc;
    if (wire_func(wire, width, vals, style, &odesc) == BAD)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = odesc ? 1.0 : 0.0;
    return (OK);
}


// (object_handle) WireH(width, num, arraypts, end_style)
//
// This is similar to the Wire function, but will return a handle to
// the new object.  On error, a scalar 0 is returned.
//
bool
geom2_funcs::IFwireH(Variable *res, Variable *args, void*)
{
    Wire wire;
    int width;
    ARG_CHK(arg_coord(args, 0, &width, DSP()->CurMode()))
    ARG_CHK(arg_int(args, 1, &wire.numpts))
    double *vals;
    ARG_CHK(arg_array(args, 2, &vals, 2*wire.numpts))
    int style;
    ARG_CHK(arg_int(args, 3, &style))

    CDo *odesc;
    if (wire_func(wire, width, vals, style, &odesc) == BAD)
        return (BAD);
    if (odesc) {
        CDol *ol = new CDol(odesc, 0);
        sHdl *hdl = new sHdlObject(ol, CurCell());
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0.0;
    }
    return (OK);
}


namespace {
    bool label_func(const char *string, Label &label, bool xform_given,
        CDo **odptr)
    {
        if (odptr)
            *odptr = 0;
        CDs *cursd = CurCell();
        if (!cursd) {
            if (!CD()->ReopenCell(DSP()->CurCellName()->string(),
                    DSP()->CurMode()) || !(cursd = CurCell()))
                return (BAD);
        }
        if (label.width <= 0 || label.height <= 0) {
            int w, h;
            DSP()->DefaultLabelSize(string, DSP()->CurMode(), &w, &h);
            if (label.width > 0)
                label.height = (int)((label.width*h)/w);
            else if (label.height > 0)
                label.width = (int)((label.height*w)/h);
            else {
                label.width = w;
                label.height = h;
            }
        }
        label.label = new hyList(cursd, string, HYcvAscii);

        if (!xform_given) {
            if (ED()->horzJustify() == 1)
                label.xform |= TXTF_HJC;
            else if (ED()->horzJustify() == 2)
                label.xform |= TXTF_HJR;
            if (ED()->vertJustify() == 1)
                label.xform |= TXTF_VJC;
            else if (ED()->vertJustify() == 2)
                label.xform |= TXTF_VJT;
            if (SIlcx()->applyTransform()) {
                if (GEO()->curTx()->magset()) {
                    label.width = (int)(label.width * GEO()->curTx()->magn());
                    label.height = (int)(label.height * GEO()->curTx()->magn());
                }
                cTfmStack stk;
                stk.TPush();
                GEO()->applyCurTransform(&stk, 0, 0,
                    SIlcx()->transformX(), SIlcx()->transformY());
                stk.TPoint(&label.x, &label.y);
                if (GEO()->curTx()->magset()) {
                    label.x = SIlcx()->transformX() + (int)((label.x -
                        SIlcx()->transformX())*GEO()->curTx()->magn());
                    label.y = SIlcx()->transformY() + (int)((label.y -
                        SIlcx()->transformY())*GEO()->curTx()->magn());
                }
                stk.TPop();

                // Update the label xform
                stk.TSetTransformFromXform(label.xform, 0, 0);
                GEO()->applyCurTransform(&stk, 0, 0, 0, 0);
                CDtf tf;
                stk.TCurrent(&tf);
                label.xform &= ~(TXTF_ROT | TXTF_MY | TXTF_MX | TXTF_45);
                label.xform |= (tf.get_xform() &
                        (TXTF_ROT | TXTF_MY | TXTF_MX | TXTF_45));
                stk.TPop();
            }
        }
        CDo *odesc = cursd->newLabel(0, &label, LT()->CurLayer(), 0, false);
        if (!odesc)
            return (BAD);
        if (odptr)
            *odptr = odesc;
        return (OK);
    }
}


// (int) Label(text, x, y, [width, height, flags])
//
// This function creates a label on the current layer.  The function
// takes a variable number of arguments, but the first three must be
// present.  The first argument is of string type and contains the
// label text.  The next two arguments specify the x and y coordinates
// of the label reference point.
//
// The remaining arguments are optional.  The width and height specify
// the size of the bounding box into which the text will be rendered,
// in microns.  if both are zero or negative or not given, a default
// size will be used.  If only one is given a value greater than zero,
// the other will be computed using a default aspect ratio.  If both
// are greater than zero, the text will be squeezed or stretched to
// conform.
//
// The flags argument is a label flags word used in Xic to set various
// label attributes.  If given, the Justify function and UseTransform
// function settings will be ignored, as these attributes will be set
// from the flags.  If flags is not given, the functions will set the
// justification and transformation.
//
// This function always returns 1.
//
bool
geom2_funcs::IFlabel(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))
    Label label;
    ARG_CHK(arg_coord(args, 1, &label.x, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 2, &label.y, DSP()->CurMode()))
    bool xform_given = false;
    if (args[3].type != TYP_ENDARG) {
        ARG_CHK(arg_coord(args, 3, &label.width, DSP()->CurMode()))
        if (args[4].type != TYP_ENDARG) {
            ARG_CHK(arg_coord(args, 4, &label.height, DSP()->CurMode()))
            if (args[5].type != TYP_ENDARG) {
                ARG_CHK(arg_int(args, 5, &label.xform))
                xform_given = true;
            }
        }
    }
    if (!string || !*string)
        return (BAD);

    CDo *odesc;
    if (label_func(string, label, xform_given, &odesc) == BAD)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = odesc ? 1.0 : 0.0;
    return (OK);
}


// (int) LabelH(text, x, y, [width, height, xform])
//
// This is similar to the Label function, but will return a handle to
// the new object.  On error, a scalar 0 is returned.
//
bool
geom2_funcs::IFlabelH(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))
    Label label;
    ARG_CHK(arg_coord(args, 1, &label.x, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 2, &label.y, DSP()->CurMode()))
    bool xform_given = false;
    if (args[3].type != TYP_ENDARG) {
        ARG_CHK(arg_coord(args, 3, &label.width, DSP()->CurMode()))
        if (args[4].type != TYP_ENDARG) {
            ARG_CHK(arg_coord(args, 4, &label.height, DSP()->CurMode()))
            if (args[5].type != TYP_ENDARG) {
                ARG_CHK(arg_int(args, 5, &label.xform))
                xform_given = true;
            }
        }
    }
    if (!string || !*string)
        return (BAD);

    CDo *odesc;
    if (label_func(string, label, xform_given, &odesc) == BAD)
        return (BAD);
    if (odesc) {
        CDol *ol = new CDol(odesc, 0);
        sHdl *hdl = new sHdlObject(ol, CurCell());
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0.0;
    }
    return (OK);
}


// (int) Logo(string, x, y, [width, height])
//
// This creates and places physical text, i.e., text that is
// constructed with database polygons that will appear in the mask
// layout.  The function takes a variable number of arguments, but the
// first three must be present.  The first argument is of string type
// and contains the label text.  The next two arguments specify the x
// and y coordinates of the reference point, which is dependent on the
// current justification, as set with the Justify() function.  The
// default is the lower-left corner of the bounding box.  The text
// will be transformed according to the current transform.
//
// The remaining arguments are optional.  The width and height specify
// the approximate size of the rendered text.  Unlike the Label
// function, the text aspect ratio is fixed.  The first of height or
// width which is positive will be used to set the "pixel" size used
// to render the text, by dividing this value by the character cell
// height or width of the default font.  Thus, the rendered text size
// will only be accurate for this font, and will scale with the number
// of pixels used in the "pretty" fonts.  One must experiment with a
// chosen font to obtain accurate sizing.  If neither parameter is
// given and positive, a default size will be used.
//
// This provides the functionality of the logo menu button in Xic, and
// is sensitive to the following variables:
//
//  LogoEndStyle
//  LogoPathWidth
//  LogoManhattan
//  LogoPretty
//  LogoPrettyFont
//  LogoToFile
//
// This function always returns 1.
//
bool
geom2_funcs::IFlogo(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))
    Label label;
    ARG_CHK(arg_coord(args, 1, &label.x, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 2, &label.y, DSP()->CurMode()))
    if (args[3].type != TYP_ENDARG) {
        ARG_CHK(arg_coord(args, 3, &label.width, DSP()->CurMode()))
        if (args[4].type != TYP_ENDARG) {
            ARG_CHK(arg_coord(args, 4, &label.height, DSP()->CurMode()))
        }
    }
    if (!string || !*string)
        return (BAD);

    int w, h;
    DSP()->DefaultLabelSize(string, DSP()->CurMode(), &w, &h);
    if (label.height > 0)
        label.width = (int)((label.height*w)/h);
    else if (label.width > 0)
        label.height = (int)((label.width*h)/w);
    else {
        label.width = w;
        label.height = h;
    }
    if (GEO()->curTx()->magset()) {
        label.width = (int)(label.width * GEO()->curTx()->magn());
        label.height = (int)(label.height * GEO()->curTx()->magn());
    }
    int pixel_size = label.height/ED()->logoFont()->cellHeight();
    ED()->createLogo(string, label.x, label.y, pixel_size);

    res->type = TYP_SCALAR;
    res->content.value = 1.0;
    return (OK);
}


// (int) Justify(hj, vj)
//
// This sets the justification for text created with the logo and
// label commands and corresponding script functions.  The arguments
// can have the following values:
//
bool
geom2_funcs::IFjustify(Variable *res, Variable *args, void*)
{
    int hj;
    ARG_CHK(arg_int(args, 0, &hj))
    int vj;
    ARG_CHK(arg_int(args, 1, &vj))

    if (hj == 0 || hj == 1 || hj == 2)
        ED()->setHorzJustify(hj);
    if (vj == 0 || vj == 1 || vj == 2)
        ED()->setVertJustify(vj);
    res->type = TYP_SCALAR;
    res->content.value = 1.0;
    return (OK);
}


// (int) Delete()
//
// This function deletes all selected objects from the database.
//
bool
geom2_funcs::IFdelete(Variable *res, Variable*, void*)
{
    if (Selections.hasTypes(CurCell(), 0))
        ED()->deleteQueue();
    res->type = TYP_SCALAR;
    res->content.value = 1.0;
    return (OK);
}


// (int) Erase(left, bottom, right, top)
//
// This function erases the area defined by the arguments.  Polygons,
// wires, and boxes are appropriately clipped.  The erase function has
// no effect on subcells or labels.  This provides an erase capability
// similar to the erase menu button.
//
bool
geom2_funcs::IFerase(Variable *res, Variable *args, void*)
{
    int x1;
    ARG_CHK(arg_coord(args, 0, &x1, DSP()->CurMode()))
    int y1;
    ARG_CHK(arg_coord(args, 1, &y1, DSP()->CurMode()))
    int x2;
    ARG_CHK(arg_coord(args, 2, &x2, DSP()->CurMode()))
    int y2;
    ARG_CHK(arg_coord(args, 3, &y2, DSP()->CurMode()))

    if (x1 == x2 || y1 == y2)
        return (BAD);
    res->content.value = ED()->eraseArea(false, x1, y1, x2, y2);
    res->type = TYP_SCALAR;
    return (OK);
}


// (int) EraseUnder()
//
// This function will erase geometry from unselected objects that
// intersect with objects that are selected.  Only objects on visible,
// selectable layers will be clipped.  This is equivalent to the Erase
// Under command in Xic.  This function always returns 1.
//
bool
geom2_funcs::IFeraseUnder(Variable *res, Variable*, void*)
{
    ED()->eraseUnder();
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) Yank(left, bottom, right, top)
//
// This function puts the geometry in the specified rectangle in yank
// buffer 0.  It can be placed with the Put() function, or the put
// command.  This provides a yank capability similar to the erase menu
// button.
//
bool
geom2_funcs::IFyank(Variable *res, Variable *args, void*)
{
    int x1;
    ARG_CHK(arg_coord(args, 0, &x1, DSP()->CurMode()))
    int y1;
    ARG_CHK(arg_coord(args, 1, &y1, DSP()->CurMode()))
    int x2;
    ARG_CHK(arg_coord(args, 2, &x2, DSP()->CurMode()))
    int y2;
    ARG_CHK(arg_coord(args, 3, &y2, DSP()->CurMode()))

    if (x1 == x2 || y1 == y2)
        return (BAD);
    res->content.value = ED()->eraseArea(true, x1, y1, x2, y2);
    res->type = TYP_SCALAR;
    return (OK);
}


// (int) Put(x, y, bufnum)
//
// This puts the contents of the indicated yank buffer in the current
// layout, with the lower left at x, y.  The bufnum is the yank buffer
// index, which can be 0-4.  Buffer 0 is the most recent yank or
// erase, buffer 1 is the next most recent, etc.  This provides
// functionality similar to the put menu button.
//
bool
geom2_funcs::IFput(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, DSP()->CurMode()))
    int ix;
    ARG_CHK(arg_int(args, 2, &ix))

    if (ix < 0 || ix >= ED_YANK_DEPTH)
        return (BAD);
    ED()->put(x, y, ix);
    res->content.value = 1.0;
    res->type = TYP_SCALAR;
    return (OK);
}


// (int) Xor(left, bottom, right, top)
//
// This function exclusive-or's the area defined by the arguments with
// boxes on the current layer.  Previous boxes become clear areas.
// This provides functionality similar to the xor menu button.
//
bool
geom2_funcs::IFxor(Variable *res, Variable *args, void*)
{
    int x1;
    ARG_CHK(arg_coord(args, 0, &x1, DSP()->CurMode()))
    int y1;
    ARG_CHK(arg_coord(args, 1, &y1, DSP()->CurMode()))
    int x2;
    ARG_CHK(arg_coord(args, 2, &x2, DSP()->CurMode()))
    int y2;
    ARG_CHK(arg_coord(args, 3, &y2, DSP()->CurMode()))

    if (x1 == x2 || y1 == y2)
        return (BAD);
    res->content.value = ED()->xorArea(x1, y1, x2, y2);
    res->type = TYP_SCALAR;
    return (OK);
}


// (int) Copy(fromx, fromy, tox, toy, repcnt)
//
// Copies of selected objects are created and placed such that the
// point specified by the first two arguments is moved to the location
// specified by the second two arguments.
//
// The repcnt is an integer replication count in the range 1-100000,
// which will be silently taken as 1 if out of range.  If not one,
// multiple copies are made, at mutiples of the translation factors
// given.
//
// The return value is 1 if there were no errors and something was
// copied, 0 otherwise.
//
bool
geom2_funcs::IFcopy(Variable *res, Variable *args, void*)
{
    int ref_x;
    ARG_CHK(arg_coord(args, 0, &ref_x, DSP()->CurMode()))
    int ref_y;
    ARG_CHK(arg_coord(args, 1, &ref_y, DSP()->CurMode()))
    int x;
    ARG_CHK(arg_coord(args, 2, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 3, &y, DSP()->CurMode()))
    int rep;
    ARG_CHK(arg_int(args, 4, &rep))
    if (rep < 1 || rep > 100000)
        rep = 1;

    res->content.value = ED()->replicateQueue(ref_x, ref_y, x, y, rep, 0, 0);
    res->type = TYP_SCALAR;
    return (OK);
}


// (int) CopyToLayer(fromx, fromy, tox, toy, oldlayer, newlayer, repcnt)
//
// This is similar to the Copy() function, but allows layer change.
// If newlayer is 0, null, or empty, oldlayer is ignored and the
// function behaves identically to Copy().  Otherwise the newlayer
// string must be a layer name.  If oldlayer is 0, null, or empty, all
// copied objects are placed on newlayer.  Otherwise, oldlayer must be
// a layer name, in which case only objects on oldlayer will be placed
// on newlayer, other objects will remain on the same layer.  Subcell
// objects are copied as in Copy(), i.e., the layer arguments are
// ignored.
//
bool
geom2_funcs::IFcopyToLayer(Variable *res, Variable *args, void*)
{
    int ref_x;
    ARG_CHK(arg_coord(args, 0, &ref_x, DSP()->CurMode()))
    int ref_y;
    ARG_CHK(arg_coord(args, 1, &ref_y, DSP()->CurMode()))
    int x;
    ARG_CHK(arg_coord(args, 2, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 3, &y, DSP()->CurMode()))
    CDl *ldold;
    ARG_CHK(arg_layer_zok(args, 4, &ldold))
    CDl *ldnew;
    ARG_CHK(arg_layer_zok(args, 5, &ldnew))
    int rep;
    ARG_CHK(arg_int(args, 6, &rep))
    if (rep < 1 || rep > 100000)
        rep = 1;

    res->content.value = ED()->replicateQueue(ref_x, ref_y, x, y, rep, ldold,
        ldnew);
    res->type = TYP_SCALAR;
    return (OK);
}


// (int) Move(fromx, fromy, tox, toy)
//
// Moves the selected objects such that the reference point specified
// in the first two arguments is moved to the point specified by the
// second two arguments.  The return value is 1 if there were no
// errors and something was moved, 0 otherwise.
//
bool
geom2_funcs::IFmove(Variable *res, Variable *args, void*)
{
    int ref_x;
    ARG_CHK(arg_coord(args, 0, &ref_x, DSP()->CurMode()))
    int ref_y;
    ARG_CHK(arg_coord(args, 1, &ref_y, DSP()->CurMode()))
    int x;
    ARG_CHK(arg_coord(args, 2, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 3, &y, DSP()->CurMode()))

    res->content.value = ED()->moveQueue(ref_x, ref_y, x, y, 0, 0);
    res->type = TYP_SCALAR;
    return (OK);
}


// (int) MoveToLayer(fromx, fromy, tox, toy, oldlayer, newlayer)
//
// This is similar to the Move() function, but allows layer change.
// If newlayer is 0, null, or empty, oldlayer is ignored and the
// function behaves identically to Move().  Otherwise the newlayer
// string must be a layer name.  If oldlayer is 0, null, or empty, all
// moved objects are placed on newlayer.  Otherwise, oldlayer must be
// a layer name, in which case only objects on oldlayer will be placed
// on newlayer, other objects will remain on the same layer.  Subcell
// objects are moved as in Move(), i.e., the layer arguments are
// ignored.
//
bool
geom2_funcs::IFmoveToLayer(Variable *res, Variable *args, void*)
{
    int ref_x;
    ARG_CHK(arg_coord(args, 0, &ref_x, DSP()->CurMode()))
    int ref_y;
    ARG_CHK(arg_coord(args, 1, &ref_y, DSP()->CurMode()))
    int x;
    ARG_CHK(arg_coord(args, 2, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 3, &y, DSP()->CurMode()))
    CDl *ldold;
    ARG_CHK(arg_layer_zok(args, 4, &ldold))
    CDl *ldnew;
    ARG_CHK(arg_layer_zok(args, 5, &ldnew))

    res->content.value = ED()->moveQueue(ref_x, ref_y, x, y, ldold, ldnew);
    res->type = TYP_SCALAR;
    return (OK);
}


// (int) Rotate(x, y, ang, remove)
//
// The selected objects are rotated counter-clockwise by ang (in
// degrees) about he point specified in the first two arguments.  This
// provides functionality similar to the spin button in the side menu.
//
// If the boolean argument remove is true (nonzero), the original
// objects will be deleted.  Otherwise, the original objects are
// retained, and will become deselected.
//
// The return value is 1 if there were no errors and something was
// rotated, 0 otherwise.
//
// Note:  in releases prior to 3.0.5, the remove argument was absent
// and effectively 0 in the current function implementation.
//
bool
geom2_funcs::IFrotate(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, DSP()->CurMode()))
    double ang;
    ARG_CHK(arg_real(args, 2, &ang))
    bool remove;
    ARG_CHK(arg_boolean(args, 3, &remove))

    ang *= M_PI/180;
    res->content.value = ED()->rotateQueue(x, y, ang, x, y, 0, 0,
        remove ? CDmove : CDcopy);
    res->type = TYP_SCALAR;
    return (OK);
}


// (int) RotateToLayer(x, y, ang, oldlayer, newlayer, remove)
//
// This is similar to the Rotate() function, but allows layer change.
// If newlayer is 0, null, or empty, oldlayer is ignored and the
// function behaves identically to Rotate().  Otherwise the newlayer
// string must be a layer name.  If oldlayer is 0, null, or empty, all
// rotated objects are placed on newlayer.  Otherwise, oldlayer must
// be a layer name, in which case only objects on oldlayer will be
// placed on newlayer, other objects will remain on the same layer.
// Subcell objects are rotated as in Rotate(), i.e., the layer
// arguments are ignored.
//
// If the boolean argument remove is true (nonzero), the original
// objects will be deleted.  Otherwise, the original objects are
// retained, and will become deselected.
//
// The function returns 1 on success, 0 if an error occurred.
//
// Note:  in earlier releases, the remove argument was absent and
// effectively 0 in the current function.
//
bool
geom2_funcs::IFrotateToLayer(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, DSP()->CurMode()))
    double ang;
    ARG_CHK(arg_real(args, 2, &ang))
    CDl *ldold;
    ARG_CHK(arg_layer_zok(args, 3, &ldold))
    CDl *ldnew;
    ARG_CHK(arg_layer_zok(args, 4, &ldnew))
    bool remove;
    ARG_CHK(arg_boolean(args, 5, &remove))

    ang *= M_PI/180;
    res->content.value = ED()->rotateQueue(x, y, ang, x, y, ldold, ldnew,
        remove ? CDmove : CDcopy);
    res->type = TYP_SCALAR;
    return (OK);
}


// (int) Split(x, y, flag, orient)
//
// This will sever selected objects along a vertical or horizontal
// line through x, y if flag is nonzero.  If orient is 0, the break
// line is vertical, otherwise it is horizontal.  If flag is zero, the
// function will return 1 if an object would be split, 0 otherwise,
// though no objects are actually split.  This provides functionality
// similar to the break menu button.
//
bool
geom2_funcs::IFsplit(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, DSP()->CurMode()))
    bool flag;
    ARG_CHK(arg_boolean(args, 2, &flag))
    int orient;
    ARG_CHK(arg_int(args, 3, &orient))

    if (orient)
        orient = 1;
    res->type = TYP_SCALAR;
    res->content.value = ED()->doBreak(x, y, flag, (BrkType)orient);
    return (OK);
}


// (int) Flatten(depth, use_merge, fast_mode)
//
// The selected subcells are flattened into the current cell,
// recursively to the given depth, similar to the effect of the
// Flatten button in the Edit Menu.
//
// The depth argument may be an integer representing the depth into
// the hierarchy to flatten:  0 for top-level subcells only, 1 to
// include second-level subcells, etc.  This argument can also be a
// string starting with 'a' to signify flattening all levels.  A
// negative depth also signifies flattening all levels.
//
// The use_merge argument is a boolean which if nonzero indicates that
// new objects will be merged with existing objects when added to the
// current cell.  This is the same merging as controlled by the Merge
// Boxes, Polys and Merge, Clip Boxes Only buttons in the Edit Menu.
//
// If the boolean argument fast_mode is nonzero, "fast" mode is used,
// meaning that there will be no undo list generation and no object
// merging.  This is not undoable so should be used with care.
//
// The function returns 1 on success, 0 otherwise, with an error
// message probably available from GetError.
//
bool
geom2_funcs::IFflatten(Variable *res, Variable *args, void*)
{
    int depth;
    ARG_CHK(arg_depth(args, 0, &depth))
    bool use_merge;
    ARG_CHK(arg_boolean(args, 1, &use_merge))
    bool fast_mode;
    ARG_CHK(arg_boolean(args, 2, &fast_mode))

    // implicit "Commit"
    Ulist()->CommitChanges();

    cTfmStack stk;
    ED()->flattenSelected(&stk, depth, use_merge, fast_mode);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// Layer(string, mode, depth, recurse, noclear, use_merge, fast_mode)
//
// This is very similar to the !layer command, and operations from the
// Evaluate Layer Expression panel brought up with the Layer
// Expression button in the Edit Menu.  The string is of the form
//
//   "new_layer_name [=] layer_expression".
//
// The mode argument is an integer which sets the split/join mode,
// similar to the keywords in the !layer command, and the buttons in
// the Evaluate Layer Expression panel.  Only the two least-significant
// bits of the integer value are used.
//
//   0  default
//   1  horizontal split
//   2  vertical split
//   3  join
//
// The depth is the search depth, which can be an integer which sets
// the maximum depth to search (0 means search the current cell only,
// 1 means search the current cell plus the subcells, etc., and a
// negative integer sets the depth to search the entire hierarchy).
// This argument can also be a string starting with 'a' such as "a" or
// "all" which specifies to search the entire hierarchy.
//
// The recurse argument is a boolean value which corresponds to the
// "-r" option of the !layer command, or the Recursively create in
// subcells check box in the Evaluate Layer Expression panel.  If
// nonzero, evaluation will be performed in subcells to depth, using
// only that cell's geometry.  When zero, geometry is created in the
// current cell only, using geometry found in subcells to depth.
//
// If the boolean argument noclear is true, the target layer will not
// be cleared before expression evaluation.  This corresponds to the
// "-c" option of the !layer command, and the Don't clear layer before
// evaluation button in the Evaluate Layer Expression panel.
//
// The boolean argument use_merge corresponds to the "-m" option in
// the !layer command, and the Use object merging while processing
// check box in the Evaluate Layer Expression panel.  When nonzero,
// new objects will be merged with existing objects when added to a
// cell.
//
// The fast_mode argument is a boolean value that corresponds to the
// "-f" option in the !layer command, and the Fast mode check box in
// the Evaluate Layer Expression panel.  When nonzero, undo list
// processing and merging are skipped for speed and to reduce memory
// use.  However, the result is not undoable so this flag should be
// used with care.
//
// There is no return value; the function either succeeds or will
// terminate the script on error.
//
bool
geom2_funcs::IFlayer(Variable*, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))
    int mode;
    ARG_CHK(arg_int(args, 1, &mode))
    int depth;
    ARG_CHK(arg_depth(args, 2, &depth))
    bool recurse;
    ARG_CHK(arg_boolean(args, 3, &recurse))
    bool noclear;
    ARG_CHK(arg_boolean(args, 4, &noclear))
    bool merge;
    ARG_CHK(arg_boolean(args, 5, &merge))
    bool fast_mode;
    ARG_CHK(arg_boolean(args, 6, &fast_mode))

    if (!string || !*string)
        return (BAD);

    int flags = mode & CLmode;
    if (recurse)
        flags |= CLrecurse;
    if (noclear)
        flags |= CLnoClear;
    if (merge)
        flags |= CLmerge;
    if (fast_mode)
        flags |= CLnoUndo;

    if (ED()->createLayerRecurse(string, depth, flags) != XIok)
        return (BAD);
    DSP()->RedisplayAll(Physical);
    return (OK);
}


//-------------------------------------------------------------------------
// Property Management
//-------------------------------------------------------------------------

namespace {
    bool
    arg_prpty(Variable *args, int indx, int *val, bool *doall = 0)
    {
        if (args[indx].type == TYP_SCALAR) {
            *val = (int)args[indx].content.value;
            return (true);
        }
        if (args[indx].type == TYP_STRING || args[indx].type == TYP_NOTYPE) {
            char *s = args[indx].content.string;
            if (!s)
                return (false);
            *val = ScedIf()->whichPrpty(s, doall);
            if (*val || (doall && *doall))
                return (true);
        }
        return (false);
    }
}


// (prpty_handle) PrpHandle(object_handle)
//
// This function returns a handle to the list of properties of the
// object referenced by the passed object handle.  The function fails
// if the argument is not a valid object handle, use CellPrpHandle to
// list cell properties.
//
bool
geom2_funcs::IFprpHandle(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    if (id <= 0) {
        Errs()->add_error("PrpHandle:  bad object handle.");
        return (BAD);
    }
    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    sHdl *hdl = 0;
    sHdl *ohdl = sHdl::get(id);
    if (ohdl) {
        if (ohdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)ohdl->data;
        if (ol)
            hdl = new sHdlPrpty(sHdl::prp_list(ol->odesc->prpty_list()),
                ol->odesc, ((sHdlObject*)ohdl)->sdesc);
        else
            hdl = new sHdlPrpty(0, 0, ((sHdlObject*)ohdl)->sdesc);
    }
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (prpty_handle) GetPrpHandle(number)
//
// Since there can be arbitrarily many properties defined with the
// same number, a generator function is used to read properties one at
// a time.  This function returns a handle to a list of the properties
// that match the number passed.  This applies to the first
// object in the selection queue (the most recent object selected).
// The returned value is used by other functions to actually retrieve
// the property text.
//
// If the number argument is a prefix of "all", then any property
// string will be returned.  In physical mode, the number argument
// should otherwise be an integer.  In electrical mode, the number
// argument can have string form as described in the introduction to
// this section.
//
bool
geom2_funcs::IFgetPrpHandle(Variable *res, Variable *args, void*)
{
    bool doall = false;
    int val;
    if (!arg_prpty(args, 0, &val, &doall))
        return (BAD);

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    sSelGen sg(Selections, cursd);
    CDo *od;
    CDpl *p0 = 0;
    while ((od = sg.next()) != 0) {
        if (doall)
            p0 = sHdl::prp_list(od->prpty_list());
        else {
            CDpl *pe = 0;
            for (CDp *p = od->prpty_list(); p; p = p->next_prp()) {
                if (p->value() == val) {
                    if (!p0)
                        p0 = pe = new CDpl(p, 0);
                    else {
                        pe->next = new CDpl(p, 0);
                        pe = pe->next;
                    }
                }
            }
        }
        break;
    }
    sHdl *hdl = new sHdlPrpty(p0, od, cursd);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (prpty_handle) CellPrpHandle()
//
// This function returns a handle to the list of properties of the
// current cell, applicable to the current display mode in the main
// window.
//
bool
geom2_funcs::IFcellPrpHandle(Variable *res, Variable*, void*)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    sHdl *hdl = new sHdlPrpty(sHdl::prp_list(cursd->prptyList()), 0, cursd);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (prpty_handle) GetCellPrpHandle(number)
//
// Since there can be arbitrarily many properties defined with the
// same number, a generator function is used to read properties one at
// a time.  This function returns a handle to a list of the properties
// that match the number passed.  The returned value is used by other
// functions to actually retrieve the property text.
//
// A prefix of the string "all" can be passed for the number argument,
// in which case the handle will reference all properties of the cell. 
// In physical mode, the number argument should otherwise be an
// integer.  In electrical mode, the number argument can have string
// form as described in the introduction to this section.
//
bool
geom2_funcs::IFgetCellPrpHandle(Variable *res, Variable *args, void*)
{
    int val;
    bool doall = false;
    if (!arg_prpty(args, 0, &val, &doall))
        return (BAD);

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    CDpl *p0 = 0;
    if (doall)
        p0 = sHdl::prp_list(cursd->prptyList());
    else {
        CDpl *pe = 0;
        for (CDp *p = cursd->prptyList(); p; p = p->next_prp()) {
            if (p->value() == val) {
                if (!p0)
                    p0 = pe = new CDpl(p, 0);
                else {
                    pe->next = new CDpl(p, 0);
                    pe = pe->next;
                }
            }
        }
    }
    sHdl *hdl = new sHdlPrpty(p0, 0, cursd);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (int) PrpNext(prpty_handle)
//
// This function causes the referenced property of the passed handle
// to be advanced to the next in the list.  If there are no other
// properties in the list, the handle is freed, and 0 is returned.
// Otherwise, 1 is returned.  The number of remaining properties can
// be obtained with the HandleContent() function.
//
bool
geom2_funcs::IFprpNext(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLprpty)
            return (BAD);
        hdl->iterator();
        if (sHdl::get(id))
            res->content.value = 1.0;
    }
    return (OK);
}


// (int) PrpNumber(prpty_handle)
//
// This function returns the number of the property referenced by the
// handle.
//
bool
geom2_funcs::IFprpNumber(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLprpty)
            return (BAD);
        CDpl *prp = (CDpl*)hdl->data;
        if (prp)
            res->content.value = prp->pdesc->value();
    }
    return (OK);
}


// (string) PrpString(prpty_handle)
//
// This function returns the string of the property referenced by the
// handle.  The "raw" string is returned, meaning that if the property
// comes from an electrical object, all of the detail from the
// internal property string is returned.
//
bool
geom2_funcs::IFprpString(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLprpty)
            return (BAD);
        CDpl *prp = (CDpl*)hdl->data;
        if (prp) {
            char *str;
            if (prp->pdesc->string(&str)) {
                res->content.string = str;
                if (res->content.string)
                    res->flags |= VF_ORIGINAL;
            }
        }
    }
    return (OK);
}


// (string) PrptyString(obj_or_prp_handle, number)
//
// The first argument can be a property handle, or an object handle. 
// If a property handle is given, the function returns the string of
// the first property referenced by the handle that matches the
// number.  If the number argument is a prefix of "all", then any
// property string will be returned.  In physical mode, the number
// argument should otherwise be an integer.  In electrical mode, the
// number argument can be a string, as described in the introduction
// to this section.  The handle is set to reference the next property
// in the reference list, following the one returned.  When there are
// no more properties, this function returns a null string.
//
// If the first argument is an object handle, the function returns the
// strings from properties or pseudo-properties for the object
// referenced by the handle.
//
// In physical mode, the function will locate a property with the
// given number, and return its string.  If no property is found with
// that number, and a pseudo-property for the object matches the
// number, then the pseudo-property string is returned.  If no
// matching pseudo-property is found, a null string is returned. 
// Note:  objects can be modified through setting pseudo-properties
// using the PrptyAdd() function.
//
// In electrical mode, the number argument can be a string, as
// described in the introduction to this section.  In the case of an
// object handle, the "all" keyword is not supported.
//
// The function will fail if the argument is not a valid object or
// property handle.  Use GetCellPropertyString to obtain strings from
// cell properties.
// 
// If the requested property is a name property of an electrical
// device or subcircuit, only the name is returned (the internal
// property string is more complex).  Otherwise the "raw" string is
// returned.
//
bool
geom2_funcs::IFprptyString(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int val;
    bool doall = false;
    if (!arg_prpty(args, 1, &val, &doall))
        return (BAD);
    if (id <= 0) {
        Errs()->add_error("PrptyString:  bad handle.");
        return (BAD);
    }

    res->type = TYP_STRING;
    res->content.string = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type == HDLobject) {
            CDol *ol = (CDol*)hdl->data;
            if (ol) {
                CDp *pdesc = ol->odesc->prpty(val);
                if (pdesc) {
                    if (val == P_NAME && ol->odesc->type() == CDINSTANCE &&
                    OCALL(ol->odesc)->masterCell()->isElectrical()) {
                        res->content.string = lstring::copy(
                            OCALL(ol->odesc)->getBaseName((CDp_name*)pdesc));
                    }
                    else
                        pdesc->string(&res->content.string);
                }
                else
                    res->content.string =
                        XM()->GetPseudoProp(ol->odesc, val);
            }
        }
        else if (hdl->type == HDLprpty) {
            CDo *odesc = ((sHdlPrpty*)hdl)->odesc;
            CDp *prpty = (CDp*)hdl->iterator();
            while (!doall && prpty && val != prpty->value()) {
                hdl = sHdl::get(id);
                if (!hdl)
                    return (OK);
                prpty = (CDp*)hdl->iterator();
            }
            if (prpty) {
                if (odesc && val == P_NAME && odesc->type() == CDINSTANCE &&
                        OCALL(odesc)->masterCell()->isElectrical()) {
                    res->content.string = lstring::copy(
                        OCALL(odesc)->getBaseName((CDp_name*)prpty));
                }
                else
                    prpty->string(&res->content.string);
            }
        }
    }
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (string) GetPropertyString(number)
//
// This function searches the selection queue for an object with a
// property matching number.  The string for the first such property
// found is returned.  A null string is returned if no matching
// property was found.
//
bool
geom2_funcs::IFgetPropertyString(Variable *res, Variable *args, void*)
{
    int val;
    if (!arg_prpty(args, 0, &val))
        return (BAD);

    res->type = TYP_STRING;
    res->content.string = 0;

    sSelGen sg(Selections, CurCell());
    CDo *od;
    while ((od = sg.next()) != 0) {
        CDp *pdesc = od->prpty_list();
        for ( ; pdesc; pdesc = pdesc->next_prp()) {
            if (pdesc->value() == val) {
                res->content.string = lstring::copy(pdesc->string());
                if (res->content.string)
                    res->flags |= VF_ORIGINAL;
                return (OK);
            }
        }
    }
    return (OK);
}


// (string) GetCellPropertyString(number)
//
// This function searches the properties of the current cell, and
// returns the string for the first property found that matches
// number.  If no match, a null string is returned.
//
bool
geom2_funcs::IFgetCellPropertyString(Variable *res, Variable *args, void*)
{
    int val;
    if (!arg_prpty(args, 0, &val))
        return (BAD);

    res->type = TYP_STRING;
    res->content.string = 0;
    CDs *cursd = CurCell();
    CDp *pdesc = cursd ? cursd->prpty(val) : 0;
    if (pdesc)
        res->content.string = lstring::copy(pdesc->string());
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


namespace {
    bool add_elec_prpty(CDc *cdesc, int val, const char *string)
    {
        CDs *msdesc = cdesc->masterCell();
        if (!msdesc)
            return (false);;
        switch (val) {
        case P_MODEL:
        case P_VALUE:
        case P_DEVREF:
            if (msdesc->isDevice())
                return (ED()->prptyModify(cdesc, 0, val, string, 0) != 0);
            break;
        case P_PARAM:
        case P_OTHER:
        case P_NAME:
            return (ED()->prptyModify(cdesc, 0, val, string, 0) != 0);
        case P_FLATTEN:
            if (!msdesc->isDevice())
                return (ED()->prptyModify(cdesc, 0, val, string, 0) != 0);
            break;
        case P_RANGE:
            return (ED()->prptyModify(cdesc, 0, val, string, 0) != 0);
        case P_NOPHYS:
            {
                const char *pstr = "nophys";
                if (string && (*string == 's' || *string == 'S'))
                    pstr = "shorted";
                return (ED()->prptyModify(cdesc, 0, val, pstr, 0) != 0);
            }
        case P_SYMBLC:
            if (!msdesc->isDevice())
                return (ED()->prptyModify(cdesc, 0, val, "0", 0) != 0);
            break;
        default:
            break;
        }
        return (false);
    }


    int rm_elec_prpty(CDc *cdesc, int val, const char *string)
    {
        CDs *msdesc = cdesc->masterCell();
        if (!msdesc)
            return (false);
        int cnt = 0;
        switch (val) {
        case P_MODEL:
        case P_VALUE:
        case P_DEVREF:
            if (msdesc->isDevice())
                cnt += (ED()->prptyModify(cdesc, 0, val, 0, 0) != 0);
            break;
        case P_OTHER:
            for (CDp *p = cdesc->prpty_list(); p;
                    p = p->next_prp()) {
                if (p->value() == P_OTHER) {
                    if (string && *string) {
                        char *s =
                            (PUSR(p)->data())->string(HYcvPlain,
                            false);
                        bool zz = lstring::prefix(string, s);
                        delete [] s;
                        if (!zz)
                            continue;
                    }
                    cnt += (ED()->prptyModify(cdesc, p, val, 0, 0) != 0);
                }
            }
            break;
        case P_PARAM:
        case P_NOPHYS:
        case P_RANGE:
        case P_NAME:
            cnt += (ED()->prptyModify(cdesc, 0, val, 0, 0) != 0);
            break;
        case P_FLATTEN:
        case P_SYMBLC:
            if (!msdesc->isDevice())
                cnt += (ED()->prptyModify(cdesc, 0, val, 0, 0) != 0);
            break;
        default:
            break;
        }
        return (cnt);
    }
}


// (int) PrptyAdd(object_handle, number, string)
//
// This function will create a new property using the number and
// string provided, on the object referenced by the handle.  The
// object must be defined in the current cell.  The function will fail
// if the handle is invalid.  Use CellPropertyAdd to add properties to
// the current cell.
//
// In physical mode, the property number can take any non-negative
// value.  This includes property numbers that are used by Xic for
// various purposes in the range 7000-7199.  Unless the user is
// expecting the Xic interpretation of the property number, these
// numbers should be avoided.  It is the caller's responsibility to
// ensure that the properties in this range are applied to the
// appropriate objects, in the correct context and with correct
// syntax, as there is little or no checking.  Adding some properties
// in this range such as flags, flatten, or a pcell property will
// automatically remove an existing property with the same number, if
// any.
//
// The pseudo-properties in the range 7200-7299 will have their
// documented effect when applied, and no property is added,
//
// In electrical mode, it is possible to set these properties of
// device instances.
//   name, model, value, param, devref,
//   other, range, nophys, symblc
// and the following properties of subcircuit instances:
//   name, param, other, flatten,
//   range, nophys, symblc
// Attempts to set properties not listed here will silently fail.  The
// object must be defined in the current cell, thus the mode must be
// electrical.
//
// If the function succeeds, 1 is returned.  otherwise 0 is returned.
//
bool
geom2_funcs::IFprptyAdd(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int val;
    if (!arg_prpty(args, 1, &val))
        return (BAD);
    const char *string;
    ARG_CHK(arg_string(args, 2, &string))

    if (!string)
        return (BAD);
    if (id <= 0)
        return (BAD);
    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;

    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        if (((sHdlObject*)hdl)->copies)
            return (OK);
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        CDol *ol = (CDol*)hdl->data;
        if (ol && sdesc && sdesc == cursd) {
            if (sdesc->isElectrical()) {
                if (ol->odesc->type() == CDINSTANCE) {
                    CDc *cdesc = OCALL(ol->odesc);
                    res->content.value = add_elec_prpty(cdesc, val, string);
                }
            }
            else {
                CDp *pdesc = new CDp(string, val);
                Ulist()->RecordPrptyChange(sdesc, ol->odesc, 0, pdesc);
                res->content.value = 1;
            }
        }
    }
    return (OK);
}


// (int) AddProperty(number, string)
//
// This function adds a property with the given number and string to
// all selected objects.
//
// In physical mode, the property number can take any non-negative
// value.  This includes property numbers that are used by Xic for
// various purposes in the range 7000-7199.  Unless the user is
// expecting the Xic interpretation of the property number, these
// numbers should be avoided.  It is the caller's responsibility to
// ensure that the properties in this range are applied to the
// appropriate objects, in the correct context and with correct
// syntax, as there is little or no checking.
//
// The pseudo-properties in the range 7200-7299 will have their
// documented effect when applied, and no property is added,
//
// In electrical mode, it is possible to set these properties of
// device instances.
//   name, model, value, param, devref,
//   other, range, nophys, symblc
// and the following properties of subcircuit instances:
//   name, param, other, flatten,
//   range, nophys, symblc
// Attempts to set properties not listed here will silently fail.  The
// object must be defined in the current cell, thus the mode must be
// electrical.
//
// The number of properties added plus the number of pseudo-properties
// applied is returned.
//
bool
geom2_funcs::IFaddProperty(Variable *res, Variable *args, void*)
{
    int val;
    if (!arg_prpty(args, 0, &val))
        return (BAD);
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))

    if (!string)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);

    int cnt = 0;
    if (cursd->displayMode() == Electrical) {
        sSelGen sg(Selections, cursd, "c");
        CDo *od;
        while ((od = sg.next()) != 0) {
            CDc *cdesc = OCALL(od);
            cnt += add_elec_prpty(cdesc, val, string);
        }
    }
    else {
        sSelGen sg(Selections, cursd);
        CDo *od;
        while ((od = sg.next()) != 0) {
            CDp *pdesc = new CDp(string, val);
            Ulist()->RecordPrptyChange(cursd, od, 0, pdesc);
            cnt++;
        }
    }
    res->content.value = cnt;
    return (OK);
}


// (int) AddCellProperty(number, string)
//
// This function adds a property to the current cell.
//
// In physical mode, the property number can take any non-negative
// value.  This includes property numbers that are used by Xic for
// various purposes in the range 7000-7199.  Unless the user is
// expecting the Xic interpretation of the property number, these
// numbers should be avoided.  It is the caller's responsibility to
// ensure that the properties in this range are applied to the
// appropriate objects, in the correct context and with correct
// syntax, as there is little or no checking.  Adding some properties
// in this range such as flags, flatten, or a pcell property will
// automatically remove an existing property with the same number, if
// any.
//
// Numbers in the pseudo-property range 7200-7299 will do nothing.
//
// In electrical mode, it is possible to set the param, other,
// virtual, flatten, macro, node, name, and symblc properties of the
// current cell.  The last three are not "user settable" but are
// needed when building up a new circuit cell in memory, as in the
// scripts produced by the !mkscript command.  The string should have
// the format as read from a native cell file.
//
// The function returns 1 if the operation was successful, 0
// otherwise.
//
bool
geom2_funcs::IFaddCellProperty(Variable *res, Variable *args, void*)
{
    int val;
    if (!arg_prpty(args, 0, &val))
        return (BAD);
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))

    if (!string)
        return (BAD);
    CDs *cursd = CurCell();
    if (!cursd) {
        if (!CD()->ReopenCell(DSP()->CurCellName()->string(),
                DSP()->CurMode()) || !(cursd = CurCell()))
            return (BAD);
    }
    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDp *pdesc = 0;
    if (cursd->displayMode() == Electrical) {
        switch (val) {
        case P_PARAM:
        case P_OTHER:
            pdesc = new CDp(string, val);
            break;
        case P_VIRTUAL:
            pdesc = new CDp("virtual", val);
            break;
        case P_FLATTEN:
            pdesc = new CDp("flatten", val);
            break;
        case P_MACRO:
            pdesc = new CDp("macro", val);
            break;

        // The properties below are not supposed to be user-settable,
        // but are needed when building a new cell in memory, such as
        // for the scripts produced by !mkscript.

        case P_NODE:
            pdesc = new CDp_snode;
            if (!((CDp_snode*)pdesc)->parse_snode(cursd, string)) {
                delete pdesc;
                return (BAD);
            }
            break;
        case P_NAME:
            pdesc = new CDp_name;
            if (!((CDp_name*)pdesc)->parse_name(string)) {
                delete pdesc;
                return (BAD);
            }
            // Set the Device flag.
            cursd->setDevice(!((CDp_name*)pdesc)->is_subckt());
            break;
        case P_SYMBLC:
            pdesc = new CDp_sym;
            if (!((CDp_sym*)pdesc)->parse_sym(cursd, string)) {
                delete pdesc;
                return (BAD);
            }
            break;
        }
    }
    else if (!prpty_pseudo(val))
       pdesc = new CDp(string, val);
    if (pdesc) {
        Ulist()->RecordPrptyChange(cursd, 0, 0, pdesc);
        res->content.value = 1;
    }
    return (OK);
}


// (int) PrptyRemove(object_handle, number, string)
//
// This function will remove properties matching the given number and
// string from the object referenced by the handle.
//
// In physical mode, the property number can take any non-negative
// value.  This includes property numbers that are used by Xic for
// various purposes in the range 7000-7199.  It is the caller's
// responsibility to make sure that removal of properties in this
// range is appropriate.  Giving numbers in the pseudo-property range
// 7200-7299 will do nothing.
//
// If the string is null or empty, only the number is used for
// comparison, and all properties with that number will be removed. 
// Otherwise, if the string is a prefix of the property string and the
// numbers match, the property will be removed.
//
// In electrical mode, it is possible to remove these properties of
// device instances.
//   name, model, value, param, devref,
//   other, range, nophys, symblc
// and the following properties of subcircuit instances:
//   name, param, other, flatten,
//   range, nophys, symblc
// Attempts to remove properties not listed here will silently fail. 
// Except for other, the string argument is ignored.  For other
// properties, the string is used as above to identify the property to
// delete.
//
// Objects must be defined in the current cell.  The function returns
// the number of properties removed.
//
bool
geom2_funcs::IFprptyRemove(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int val;
    if (!arg_prpty(args, 1, &val))
        return (BAD);
    const char *string;
    ARG_CHK(arg_string(args, 2, &string))

    if (id <= 0)
        return (BAD);
    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;

    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        if (((sHdlObject*)hdl)->copies)
            return (OK);
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        CDol *ol = (CDol*)hdl->data;
        if (ol && sdesc && sdesc == cursd) {
            if (sdesc->isElectrical()) {
                if (ol->odesc->type() == CDINSTANCE) {
                    CDc *cdesc = OCALL(ol->odesc);
                    res->content.value = rm_elec_prpty(cdesc, val, string);
                }
            }
            else if (!prpty_pseudo(val)) {
                int cnt = 0;
                CDp *pdesc = ol->odesc->prpty_list();
                for ( ; pdesc; pdesc = pdesc->next_prp()) {
                    if (pdesc->value() == val &&
                            (!string || !*string ||
                            lstring::prefix(string, pdesc->string()))) {
                        cnt++;
                        Ulist()->RecordPrptyChange(sdesc, ol->odesc, pdesc, 0);
                    }
                }
                res->content.value = cnt;
            }
        }
    }
    return (OK);
}


// (int) RemoveProperty(number, string)
//
// This function will remove properties from selected objects.
//
// In physical mode, the property number can take any non-negative
// value.  This includes property numbers that are used by Xic for
// various purposes in the range 7000-7199.  It is the caller's
// responsibility to make sure that removal of properties in this
// range is appropriate.  Giving numbers in the pseudo-property range
// 7200-7299 will do nothing. 
//
// If the string is null or empty, only the number is used for
// comparison, and all properties with that number will be removed. 
// Otherwise, if the string is a prefix of the property string and the
// numbers match, the property will be removed.
//
// In electrical mode, it is possible to remove these properties of
// device instances.
//   name, model, value, param, devref,
//   other, range, nophys, symblc
// and the following properties of subcircuit instances:
//   name, param, other, flatten,
//   range, nophys, symblc
// Attempts to remove properties not listed here will silently fail. 
// Except for other, the string argument is ignored.  For other
// properties, the string is used as above to identify the property to
// delete.
//
// The number of properties removed is returned.
//
bool
geom2_funcs::IFremoveProperty(Variable *res, Variable *args, void*)
{
    int val;
    if (!arg_prpty(args, 0, &val))
        return (BAD);
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    int cnt = 0;
    if (DSP()->CurMode() == Electrical) {
        sSelGen sg(Selections, CurCell(), "c");
        CDo *od;
        while ((od = sg.next()) != 0) {
            CDc *cdesc = OCALL(od);
            cnt += rm_elec_prpty(cdesc, val, string);
        }
    }
    else if (!prpty_pseudo(val)) {
        sSelGen sg(Selections, CurCell());
        CDo *od;
        while ((od = sg.next()) != 0) {
            CDp *pdesc = od->prpty_list();
            for ( ; pdesc; pdesc = pdesc->next_prp()) {
                if (pdesc->value() == val &&
                        (!string || !*string ||
                        lstring::prefix(string, pdesc->string()))) {
                    cnt++;
                    Ulist()->RecordPrptyChange(CurCell(), od, pdesc, 0);
                }
            }
        }
    }
    res->content.value = cnt;
    return (OK);
}


// (int) RemoveCellProperty(number, string)
//
// This function will remove properties from the current cell.
//
// In physical mode, the property number can take any non-negative
// value.  This includes property numbers that are used by Xic for
// various purposes in the range 7000-7199.  It is the caller's
// responsibility to make sure that removal of properties in this
// range is appropriate.  Giving numbers in the pseudo-property range
// 7200-7299 will do nothing. 
//
// If the string is null or empty, only the number is used for
// comparison, and all properties with that number will be removed. 
// Otherwise, if the string is a prefix of the property string and the
// numbers match, the property will be removed.
//
// In electrical mode, it is possible to remove the param, other,
// virtual, flatten, and macro properties of the current cell.  Except
// for other, the string argument is ignored.  For other properties,
// the string is used as above to identify the property to delete.
//
// The function returns the number of properties removed.
//
bool
geom2_funcs::IFremoveCellProperty(Variable *res, Variable *args, void*)
{
    int val;
    if (!arg_prpty(args, 0, &val))
        return (BAD);
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    int cnt = 0;
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (cursd->displayMode() == Electrical) {
        switch (val) {
        case P_PARAM:
        case P_VIRTUAL:
        case P_FLATTEN:
        case P_MACRO:
            string = 0;
        case P_OTHER:
            break;
        default:
            return (OK);
        }
    }
    else if (prpty_pseudo(val))
        return (OK);

    CDp *pd = cursd->prptyList();
    CDp *pprev = 0, *pnext;
    for ( ; pd; pd = pnext) {
        pnext = pd->next_prp();
        if (pd->value() == val &&
                (!string || !*string ||
                lstring::prefix(string, pd->string()))) {
            if (pprev == 0)
                cursd->setPrptyList(pnext);
            else
                pprev->set_next_prp(pnext);
            delete pd;
            cnt++;
            continue;
        }
        pprev = pd;
    }
    res->content.value = cnt;
    return (OK);
}

