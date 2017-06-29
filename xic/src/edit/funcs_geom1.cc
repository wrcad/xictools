
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
 $Id: funcs_geom1.cc,v 1.125 2017/04/18 03:13:49 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "edit.h"
#include "undolist.h"
#include "python_if.h"
#include "tcltk_if.h"
#include "dsp_inlines.h"
#include "cd_hypertext.h"
#include "cd_propnum.h"
#include "cd_lgen.h"
#include "fio.h"
#include "fio_cif.h"
#include "geo_zlist.h"
#include "geo_ylist.h"
#include "geo_zgroup.h"
#include "si_parsenode.h"
#include "si_handle.h"
#include "si_args.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_lexpr.h"
#include "si_lspec.h"
#include "main_scriptif.h"
#include "layertab.h"
#include "errorlog.h"
#include "select.h"
#include "events.h"
#include "grip.h"


////////////////////////////////////////////////////////////////////////
//
// Script Functions:  Geometry Editing Functions 1
//
////////////////////////////////////////////////////////////////////////

namespace {
    namespace geom1_funcs {

        // General Editing
        bool IFclearCell(Variable*, Variable*, void*);
        bool IFcommit(Variable*, Variable*, void*);
        bool IFundo(Variable*, Variable*, void*);
        bool IFredo(Variable*, Variable*, void*);
        bool IFselectLast(Variable*, Variable*, void*);

        // Current Transform
        bool IFsetTransform(Variable*, Variable*, void*);
        bool IFstoreTransform(Variable*, Variable*, void*);
        bool IFrecallTransform(Variable*, Variable*, void*);
        bool IFgetTransformString(Variable*, Variable*, void*);
        bool IFgetCurAngle(Variable*, Variable*, void*);
        bool IFgetCurMX(Variable*, Variable*, void*);
        bool IFgetCurMY(Variable*, Variable*, void*);
        bool IFgetCurMagn(Variable*, Variable*, void*);
        bool IFuseTransform(Variable*, Variable*, void*);

        // Derived Layers
        bool IFaddDerivedLayer(Variable*, Variable*, void*);
        bool IFremDerivedLayer(Variable*, Variable*, void*);
        bool IFisDerivedLayer(Variable*, Variable*, void*);
        bool IFgetDerivedLayerIndex(Variable*, Variable*, void*);
        bool IFgetDerivedLayerExpString(Variable*, Variable*, void*);
        bool IFgetDerivedLayerLexpr(Variable*, Variable*, void*);
        bool IFevalDerivedLayers(Variable*, Variable*, void*);
        bool IFclearDerivedLayers(Variable*, Variable*, void*);

        // Grips
        bool IFaddGrip(Variable*, Variable*, void*);
        bool IFremoveGrip(Variable*, Variable*, void*);

        // Object Management by Handles
        bool IFlistElecInstances(Variable*, Variable*, void*);
        bool IFlistPhysInstances(Variable*, Variable*, void*);
        bool IFselectHandle(Variable*, Variable*, void*);
        bool IFselectHandleTypes(Variable*, Variable*, void*);
        bool IFareaHandle(Variable*, Variable*, void*);
        bool IFobjectHandleDup(Variable*, Variable*, void*);
        bool IFobjectHandlePurge(Variable*, Variable*, void*);
        bool IFobjectNext(Variable*, Variable*, void*);
        bool IFmakeObjectCopy(Variable*, Variable*, void*);
        bool IFobjectString(Variable*, Variable*, void*);
        bool IFobjectCopyFromString(Variable*, Variable*, void*);
        bool IFfilterObjects(Variable*, Variable*, void*);
        bool IFfilterObjectsA(Variable*, Variable*, void*);
        bool IFcheckObjectsConnected(Variable*, Variable*, void*);
        bool IFcheckForHoles(Variable*, Variable*, void*);
        bool IFbloatObjects(Variable*, Variable*, void*);
        bool IFedgeObjects(Variable*, Variable*, void*);
        bool IFmanhattanizeObjects(Variable*, Variable*, void*);
        bool IFgroupObjects(Variable*, Variable*, void*);
        bool IFjoinObjects(Variable*, Variable*, void*);
        bool IFsplitObjects(Variable*, Variable*, void*);
        bool IFdeleteObjects(Variable*, Variable*, void*);
        bool IFselectObjects(Variable*, Variable*, void*);
        bool IFdeselectObjects(Variable*, Variable*, void*);
        bool IFmoveObjects(Variable*, Variable*, void*);
        bool IFmoveObjectsToLayer(Variable*, Variable*, void*);
        bool IFcopyObjects(Variable*, Variable*, void*);
        bool IFcopyObjectsToLayer(Variable*, Variable*, void*);
        bool IFcopyObjectsH(Variable*, Variable*, void*);
        bool IFgetObjectType(Variable*, Variable*, void*);
        bool IFgetObjectID(Variable*, Variable*, void*);
        bool IFgetObjectArea(Variable*, Variable*, void*);
        bool IFgetObjectPerim(Variable*, Variable*, void*);
        bool IFgetObjectCentroid(Variable*, Variable*, void*);
        bool IFgetObjectBB(Variable*, Variable*, void*);
        bool IFsetObjectBB(Variable*, Variable*, void*);
        bool IFgetObjectListBB(Variable*, Variable*, void*);
        bool IFgetObjectXY(Variable*, Variable*, void*);
        bool IFsetObjectXY(Variable*, Variable*, void*);
        bool IFgetObjectLayer(Variable*, Variable*, void*);
        bool IFsetObjectLayer(Variable*, Variable*, void*);
        bool IFgetObjectFlags(Variable*, Variable*, void*);
        bool IFsetObjectNoDrcFlag(Variable*, Variable*, void*);
        bool IFsetObjectMark1Flag(Variable*, Variable*, void*);
        bool IFsetObjectMark2Flag(Variable*, Variable*, void*);
        bool IFgetObjectState(Variable*, Variable*, void*);
        bool IFgetObjectGroup(Variable*, Variable*, void*);
        bool IFsetObjectGroup(Variable*, Variable*, void*);
        bool IFgetObjectCoords(Variable*, Variable*, void*);
        bool IFsetObjectCoords(Variable*, Variable*, void*);
        bool IFgetObjectMagn(Variable*, Variable*, void*);
        bool IFsetObjectMagn(Variable*, Variable*, void*);
        bool IFgetWireWidth(Variable*, Variable*, void*);
        bool IFsetWireWidth(Variable*, Variable*, void*);
        bool IFgetWireStyle(Variable*, Variable*, void*);
        bool IFsetWireStyle(Variable*, Variable*, void*);
        bool IFsetWireToPoly(Variable*, Variable*, void*);
        bool IFgetWirePoly(Variable*, Variable*, void*);
        bool IFgetLabelText(Variable*, Variable*, void*);
        bool IFsetLabelText(Variable*, Variable*, void*);
        bool IFgetLabelFlags(Variable*, Variable*, void*);
        bool IFsetLabelFlags(Variable*, Variable*, void*);
        bool IFgetInstanceArray(Variable*, Variable*, void*);
        bool IFsetInstanceArray(Variable*, Variable*, void*);
        bool IFgetInstanceXform(Variable*, Variable*, void*);
        bool IFgetInstanceXformA(Variable*, Variable*, void*);
        bool IFsetInstanceXform(Variable*, Variable*, void*);
        bool IFsetInstanceXformA(Variable*, Variable*, void*);
        bool IFgetInstanceMaster(Variable*, Variable*, void*);
        bool IFsetInstanceMaster(Variable*, Variable*, void*);
        bool IFgetInstanceName(Variable*, Variable*, void*);
        bool IFsetInstanceName(Variable*, Variable*, void*);
        bool IFgetInstanceAltName(Variable*, Variable*, void*);
        bool IFgetInstanceType(Variable*, Variable*, void*);
        bool IFgetInstanceIdNum(Variable*, Variable*, void*);
        bool IFgetInstanceAltIdNum(Variable*, Variable*, void*);

        // Variables (export, void*)
        bool IFjoinLimits(Variable*, Variable*, void*);
    }
    using namespace geom1_funcs;

#ifdef HAVE_PYTHON
    // Python wrappers.

    // General Editing
    PY_FUNC(ClearCell,             2,  IFclearCell);
    PY_FUNC(Commit,                0,  IFcommit);
    PY_FUNC(Undo,                  0,  IFundo);
    PY_FUNC(Redo,                  0,  IFredo);
    PY_FUNC(SelectLast,            1,  IFselectLast);

    // Current Transform
    PY_FUNC(SetTransform,          3,  IFsetTransform);
    PY_FUNC(StoreTransform,        1,  IFstoreTransform);
    PY_FUNC(RecallTransform,       1,  IFrecallTransform);
    PY_FUNC(GetTransformString,    0,  IFgetTransformString);
    PY_FUNC(GetCurAngle,           0,  IFgetCurAngle);
    PY_FUNC(GetCurMX,              0,  IFgetCurMX);
    PY_FUNC(GetCurMY,              0,  IFgetCurMY);
    PY_FUNC(GetCurMagn,            0,  IFgetCurMagn);
    PY_FUNC(UseTransform,          3,  IFuseTransform);

    // Derived Layers
    PY_FUNC(AddDerivedLayer,       3,  IFaddDerivedLayer);
    PY_FUNC(RemDerivedLayer,       1,  IFremDerivedLayer);
    PY_FUNC(IsDerivedLayer,        1,  IFisDerivedLayer);
    PY_FUNC(GetDerivedLayerIndex,  1,  IFgetDerivedLayerIndex);
    PY_FUNC(GetDerivedLayerExpString, 1, IFgetDerivedLayerExpString);
    PY_FUNC(GetDerivedLayerLexpr,  1,  IFgetDerivedLayerLexpr);
    PY_FUNC(EvalDerivedLayers,     2,  IFevalDerivedLayers);
    PY_FUNC(ClearDerivedLayers,    1,  IFclearDerivedLayers);

    // Grips
    PY_FUNC(AddGrip,               4,  IFaddGrip);
    PY_FUNC(RemoveGrip,            1,  IFremoveGrip);

    // Object Management by Handles
    PY_FUNC(ListElecInstances,     0,  IFlistElecInstances);
    PY_FUNC(ListPhysInstances,     0,  IFlistPhysInstances);
    PY_FUNC(SelectHandle,          0,  IFselectHandle);
    PY_FUNC(SelectHandleTypes,     1,  IFselectHandleTypes);
    PY_FUNC(AreaHandle,            5,  IFareaHandle);
    PY_FUNC(ObjectHandleDup,       2,  IFobjectHandleDup);
    PY_FUNC(ObjectHandlePurge,     2,  IFobjectHandlePurge);
    PY_FUNC(ObjectNext,            1,  IFobjectNext);
    PY_FUNC(MakeObjectCopy,        2,  IFmakeObjectCopy);
    PY_FUNC(ObjectString,          1,  IFobjectString);
    PY_FUNC(ObjectCopyFromString,  1,  IFobjectCopyFromString);
    PY_FUNC(FilterObjects,         5,  IFfilterObjects);
    PY_FUNC(FilterObjectsA,        5,  IFfilterObjectsA);
    PY_FUNC(CheckObjectsConnected, 1,  IFcheckObjectsConnected);
    PY_FUNC(CheckForHoles,         2,  IFcheckForHoles);
    PY_FUNC(BloatObjects,          5,  IFbloatObjects);
    PY_FUNC(EdgeObjects,           5,  IFedgeObjects);
    PY_FUNC(ManhattanizeObjects,   5,  IFmanhattanizeObjects);
    PY_FUNC(GroupObjects,          2,  IFgroupObjects);
    PY_FUNC(JoinObjects,           2,  IFjoinObjects);
    PY_FUNC(SplitObjects,          4,  IFsplitObjects);
    PY_FUNC(DeleteObjects,         2,  IFdeleteObjects);
    PY_FUNC(SelectObjects,         2,  IFselectObjects);
    PY_FUNC(DeselectObjects,       2,  IFdeselectObjects);
    PY_FUNC(MoveObjects,           6,  IFmoveObjects);
    PY_FUNC(MoveObjectsToLayer,    8,  IFmoveObjectsToLayer);
    PY_FUNC(CopyObjects,           7,  IFcopyObjects);
    PY_FUNC(CopyObjectsToLayer,    9,  IFcopyObjectsToLayer);
    PY_FUNC(CopyObjectsH,          9,  IFcopyObjectsH);
    PY_FUNC(GetObjectType,         1,  IFgetObjectType);
    PY_FUNC(GetObjectID,           1,  IFgetObjectID);
    PY_FUNC(GetObjectArea,         1,  IFgetObjectArea);
    PY_FUNC(GetObjectPerim,        1,  IFgetObjectPerim);
    PY_FUNC(GetObjectCentroid,     2,  IFgetObjectCentroid);
    PY_FUNC(GetObjectBB,           2,  IFgetObjectBB);
    PY_FUNC(SetObjectBB,           2,  IFsetObjectBB);
    PY_FUNC(GetObjectListBB,       2,  IFgetObjectListBB);
    PY_FUNC(GetObjectXY,           2,  IFgetObjectXY);
    PY_FUNC(SetObjectXY,           3,  IFsetObjectXY);
    PY_FUNC(GetObjectLayer,        1,  IFgetObjectLayer);
    PY_FUNC(SetObjectLayer,        2,  IFsetObjectLayer);
    PY_FUNC(GetObjectFlags,        1,  IFgetObjectFlags);
    PY_FUNC(SetObjectNoDrcFlag,    2,  IFsetObjectNoDrcFlag);
    PY_FUNC(SetObjectMark1Flag,    2,  IFsetObjectMark1Flag);
    PY_FUNC(SetObjectMark2Flag,    2,  IFsetObjectMark2Flag);
    PY_FUNC(GetObjectState,        1,  IFgetObjectState);
    PY_FUNC(GetObjectGroup,        1,  IFgetObjectGroup);
    PY_FUNC(SetObjectGroup,        2,  IFsetObjectGroup);
    PY_FUNC(GetObjectCoords,       2,  IFgetObjectCoords);
    PY_FUNC(SetObjectCoords,       3,  IFsetObjectCoords);
    PY_FUNC(GetObjectMagn,         1,  IFgetObjectMagn);
    PY_FUNC(SetObjectMagn,         2,  IFsetObjectMagn);
    PY_FUNC(GetWireWidth,          1,  IFgetWireWidth);
    PY_FUNC(SetWireWidth,          2,  IFsetWireWidth);
    PY_FUNC(GetWireStyle,          1,  IFgetWireStyle);
    PY_FUNC(SetWireStyle,          2,  IFsetWireStyle);
    PY_FUNC(SetWireToPoly,         1,  IFsetWireToPoly);
    PY_FUNC(GetWirePoly,           2,  IFgetWirePoly);
    PY_FUNC(GetLabelText,          1,  IFgetLabelText);
    PY_FUNC(SetLabelText,          2,  IFsetLabelText);
    PY_FUNC(GetLabelFlags,         1,  IFgetLabelFlags);
    PY_FUNC(SetLabelFlags,         2,  IFsetLabelFlags);
    PY_FUNC(GetInstanceArray,      2,  IFgetInstanceArray);
    PY_FUNC(SetInstanceArray,      2,  IFsetInstanceArray);
    PY_FUNC(GetInstanceXform,      1,  IFgetInstanceXform);
    PY_FUNC(GetInstanceXformA,     2,  IFgetInstanceXformA);
    PY_FUNC(SetInstanceXform,      2,  IFsetInstanceXform);
    PY_FUNC(SetInstanceXformA,     2,  IFsetInstanceXformA);
    PY_FUNC(GetInstanceMaster,     1,  IFgetInstanceMaster);
    PY_FUNC(SetInstanceMaster,     2,  IFsetInstanceMaster);
    PY_FUNC(GetInstanceName,       1,  IFgetInstanceName);
    PY_FUNC(SetInstanceName,       2,  IFsetInstanceName);
    PY_FUNC(GetInstanceAltName,    1,  IFgetInstanceAltName);
    PY_FUNC(GetInstanceType,       1,  IFgetInstanceType);
    PY_FUNC(GetInstanceIdNum,      1,  IFgetInstanceIdNum);
    PY_FUNC(GetInstanceAltIdNum,   1,  IFgetInstanceAltIdNum);

    // Variables (export)
    PY_FUNC(JoinLimits,            1,  IFjoinLimits);


    void py_register_geom1()
    {
      // General Editing
      cPyIf::register_func("ClearCell",             pyClearCell);
      cPyIf::register_func("Commit",                pyCommit);
      cPyIf::register_func("Undo",                  pyUndo);
      cPyIf::register_func("Redo",                  pyRedo);
      cPyIf::register_func("SelectLast",            pySelectLast);

      // Current Transform
      cPyIf::register_func("SetTransform",          pySetTransform);
      cPyIf::register_func("StoreTransform",        pyStoreTransform);
      cPyIf::register_func("RecallTransform",       pyRecallTransform);
      cPyIf::register_func("GetTransformString",    pyGetTransformString);
      cPyIf::register_func("GetCurAngle",           pyGetCurAngle);
      cPyIf::register_func("GetCurMX",              pyGetCurMX);
      cPyIf::register_func("GetCurMY",              pyGetCurMY);
      cPyIf::register_func("GetCurMagn",            pyGetCurMagn);
      cPyIf::register_func("UseTransform",          pyUseTransform);

      // Derived Layers
      cPyIf::register_func("AddDerivedLayer",       pyAddDerivedLayer);
      cPyIf::register_func("RemDerivedLayer",       pyRemDerivedLayer);
      cPyIf::register_func("IsDerivedLayer",        pyIsDerivedLayer);
      cPyIf::register_func("GetDerivedLayerIndex",  pyGetDerivedLayerIndex);
      cPyIf::register_func("GetDerivedLayerExpString", pyGetDerivedLayerExpString);
      cPyIf::register_func("GetDerivedLayerLexpr",  pyGetDerivedLayerLexpr);
      cPyIf::register_func("EvalDerivedLayers",     pyEvalDerivedLayers);
      cPyIf::register_func("ClearDerivedLayers",    pyClearDerivedLayers);

      // Grips
      cPyIf::register_func("AddGrip",               pyAddGrip);
      cPyIf::register_func("RemoveGrip",            pyRemoveGrip);

      // Object Management by Handles
      cPyIf::register_func("ListElecInstances",     pyListElecInstances);
      cPyIf::register_func("ListPhysInstances",     pyListPhysInstances);
      cPyIf::register_func("SelectHandle",          pySelectHandle);
      cPyIf::register_func("SelectHandleTypes",     pySelectHandleTypes);
      cPyIf::register_func("AreaHandle",            pyAreaHandle);
      cPyIf::register_func("ObjectHandleDup",       pyObjectHandleDup);
      cPyIf::register_func("ObjectHandlePurge",     pyObjectHandlePurge);
      cPyIf::register_func("ObjectNext",            pyObjectNext);
      cPyIf::register_func("MakeObjectCopy",        pyMakeObjectCopy);
      cPyIf::register_func("ObjectString",          pyObjectString);
      cPyIf::register_func("ObjectCopyFromString",  pyObjectCopyFromString);
      cPyIf::register_func("FilterObjects",         pyFilterObjects);
      cPyIf::register_func("FilterObjectsA",        pyFilterObjectsA);
      cPyIf::register_func("CheckObjectsConnected", pyCheckObjectsConnected);
      cPyIf::register_func("CheckForHoles",         pyCheckForHoles);
      cPyIf::register_func("BloatObjects",          pyBloatObjects);
      cPyIf::register_func("EdgeObjects",           pyEdgeObjects);
      cPyIf::register_func("ManhattanizeObjects",   pyManhattanizeObjects);
      cPyIf::register_func("GroupObjects",          pyGroupObjects);
      cPyIf::register_func("JoinObjects",           pyJoinObjects);
      cPyIf::register_func("SplitObjects",          pySplitObjects);
      cPyIf::register_func("DeleteObjects",         pyDeleteObjects);
      cPyIf::register_func("SelectObjects",         pySelectObjects);
      cPyIf::register_func("DeselectObjects",       pyDeselectObjects);
      cPyIf::register_func("MoveObjects",           pyMoveObjects);
      cPyIf::register_func("MoveObjectsToLayer",    pyMoveObjectsToLayer);
      cPyIf::register_func("CopyObjects",           pyCopyObjects);
      cPyIf::register_func("CopyObjectsToLayer",    pyCopyObjectsToLayer);
      cPyIf::register_func("CopyObjectsH",          pyCopyObjectsH);
      cPyIf::register_func("GetObjectType",         pyGetObjectType);
      cPyIf::register_func("GetObjectID",           pyGetObjectID);
      cPyIf::register_func("GetObjectArea",         pyGetObjectArea);
      cPyIf::register_func("GetObjectPerim",        pyGetObjectPerim);
      cPyIf::register_func("GetObjectCentroid",     pyGetObjectCentroid);
      cPyIf::register_func("GetObjectBB",           pyGetObjectBB);
      cPyIf::register_func("SetObjectBB",           pySetObjectBB);
      cPyIf::register_func("GetObjectListBB",       pyGetObjectListBB);
      cPyIf::register_func("GetObjectXY",           pyGetObjectXY);
      cPyIf::register_func("SetObjectXY",           pySetObjectXY);
      cPyIf::register_func("GetObjectLayer",        pyGetObjectLayer);
      cPyIf::register_func("SetObjectLayer",        pySetObjectLayer);
      cPyIf::register_func("GetObjectFlags",        pyGetObjectFlags);
      cPyIf::register_func("SetObjectNoDrcFlag",    pySetObjectNoDrcFlag);
      cPyIf::register_func("SetObjectMark1Flag",    pySetObjectMark1Flag);
      cPyIf::register_func("SetObjectMark2Flag",    pySetObjectMark2Flag);
      cPyIf::register_func("GetObjectState",        pyGetObjectState);
      cPyIf::register_func("GetObjectGroup",        pyGetObjectGroup);
      cPyIf::register_func("SetObjectGroup",        pySetObjectGroup);
      cPyIf::register_func("GetObjectCoords",       pyGetObjectCoords);
      cPyIf::register_func("SetObjectCoords",       pySetObjectCoords);
      cPyIf::register_func("GetObjectMagn",         pyGetObjectMagn);
      cPyIf::register_func("SetObjectMagn",         pySetObjectMagn);
      cPyIf::register_func("GetWireWidth",          pyGetWireWidth);
      cPyIf::register_func("SetWireWidth",          pySetWireWidth);
      cPyIf::register_func("GetWireStyle",          pyGetWireStyle);
      cPyIf::register_func("SetWireStyle",          pySetWireStyle);
      cPyIf::register_func("SetWireToPoly",         pySetWireToPoly);
      cPyIf::register_func("GetWirePoly",           pyGetWirePoly);
      cPyIf::register_func("GetLabelText",          pyGetLabelText);
      cPyIf::register_func("SetLabelText",          pySetLabelText);
      cPyIf::register_func("GetLabelFlags",         pyGetLabelFlags);
      cPyIf::register_func("GetLabelXform",         pyGetLabelFlags); // alias
      cPyIf::register_func("SetLabelFlags",         pySetLabelFlags);
      cPyIf::register_func("SetLabelXform",         pySetLabelFlags); // alias
      cPyIf::register_func("GetInstanceArray",      pyGetInstanceArray);
      cPyIf::register_func("SetInstanceArray",      pySetInstanceArray);
      cPyIf::register_func("GetInstanceXform",      pyGetInstanceXform);
      cPyIf::register_func("GetInstanceXformA",     pyGetInstanceXformA);
      cPyIf::register_func("SetInstanceXform",      pySetInstanceXform);
      cPyIf::register_func("SetInstanceXformA",     pySetInstanceXformA);
      cPyIf::register_func("GetInstanceMaster",     pyGetInstanceMaster);
      cPyIf::register_func("SetInstanceMaster",     pySetInstanceMaster);
      cPyIf::register_func("GetInstanceName",       pyGetInstanceName);
      cPyIf::register_func("SetInstanceName",       pySetInstanceName);
      cPyIf::register_func("GetInstanceAltName",    pyGetInstanceAltName);
      cPyIf::register_func("GetInstanceType",       pyGetInstanceType);
      cPyIf::register_func("GetInstanceIdNum",      pyGetInstanceIdNum);
      cPyIf::register_func("GetInstanceAltIdNum",   pyGetInstanceAltIdNum);

      // Variables (export)
      cPyIf::register_func("JoinLimits",            pyJoinLimits);
    }
#endif  // HAVE_PYTHON

#ifdef HAVE_TCL
    // Tck/Tk wrappers.

    // General Editing
    TCL_FUNC(ClearCell,             2,  IFclearCell);
    TCL_FUNC(Commit,                0,  IFcommit);
    TCL_FUNC(Undo,                  0,  IFundo);
    TCL_FUNC(Redo,                  0,  IFredo);
    TCL_FUNC(SelectLast,            1,  IFselectLast);

    // Current Transform
    TCL_FUNC(SetTransform,          3,  IFsetTransform);
    TCL_FUNC(StoreTransform,        1,  IFstoreTransform);
    TCL_FUNC(RecallTransform,       1,  IFrecallTransform);
    TCL_FUNC(GetTransformString,    0,  IFgetTransformString);
    TCL_FUNC(GetCurAngle,           0,  IFgetCurAngle);
    TCL_FUNC(GetCurMX,              0,  IFgetCurMX);
    TCL_FUNC(GetCurMY,              0,  IFgetCurMY);
    TCL_FUNC(GetCurMagn,            0,  IFgetCurMagn);
    TCL_FUNC(UseTransform,          3,  IFuseTransform);

    // Derived Layers
    TCL_FUNC(AddDerivedLayer,       3,  IFaddDerivedLayer);
    TCL_FUNC(RemDerivedLayer,       1,  IFremDerivedLayer);
    TCL_FUNC(IsDerivedLayer,        1,  IFisDerivedLayer);
    TCL_FUNC(GetDerivedLayerIndex,  1,  IFgetDerivedLayerIndex);
    TCL_FUNC(GetDerivedLayerExpString, 1, IFgetDerivedLayerExpString);
    TCL_FUNC(GetDerivedLayerLexpr,  1, IFgetDerivedLayerLexpr);
    TCL_FUNC(EvalDerivedLayers,     2,  IFevalDerivedLayers);
    TCL_FUNC(ClearDerivedLayers,    1,  IFclearDerivedLayers);

    // Grips
    TCL_FUNC(AddGrip,               4,  IFaddGrip);
    TCL_FUNC(RemoveGrip,            1,  IFremoveGrip);

    // Object Management by Handles
    TCL_FUNC(ListElecInstances,     0,  IFlistElecInstances);
    TCL_FUNC(ListPhysInstances,     0,  IFlistPhysInstances);
    TCL_FUNC(SelectHandle,          0,  IFselectHandle);
    TCL_FUNC(SelectHandleTypes,     1,  IFselectHandleTypes);
    TCL_FUNC(AreaHandle,            5,  IFareaHandle);
    TCL_FUNC(ObjectHandleDup,       2,  IFobjectHandleDup);
    TCL_FUNC(ObjectHandlePurge,     2,  IFobjectHandlePurge);
    TCL_FUNC(ObjectNext,            1,  IFobjectNext);
    TCL_FUNC(MakeObjectCopy,        2,  IFmakeObjectCopy);
    TCL_FUNC(ObjectString,          1,  IFobjectString);
    TCL_FUNC(ObjectCopyFromString,  1,  IFobjectCopyFromString);
    TCL_FUNC(FilterObjects,         5,  IFfilterObjects);
    TCL_FUNC(FilterObjectsA,        5,  IFfilterObjectsA);
    TCL_FUNC(CheckObjectsConnected, 1,  IFcheckObjectsConnected);
    TCL_FUNC(CheckForHoles,         2,  IFcheckForHoles);
    TCL_FUNC(BloatObjects,          5,  IFbloatObjects);
    TCL_FUNC(EdgeObjects,           5,  IFedgeObjects);
    TCL_FUNC(ManhattanizeObjects,   5,  IFmanhattanizeObjects);
    TCL_FUNC(GroupObjects,          2,  IFgroupObjects);
    TCL_FUNC(JoinObjects,           2,  IFjoinObjects);
    TCL_FUNC(SplitObjects,          4,  IFsplitObjects);
    TCL_FUNC(DeleteObjects,         2,  IFdeleteObjects);
    TCL_FUNC(SelectObjects,         2,  IFselectObjects);
    TCL_FUNC(DeselectObjects,       2,  IFdeselectObjects);
    TCL_FUNC(MoveObjects,           6,  IFmoveObjects);
    TCL_FUNC(MoveObjectsToLayer,    8,  IFmoveObjectsToLayer);
    TCL_FUNC(CopyObjects,           7,  IFcopyObjects);
    TCL_FUNC(CopyObjectsToLayer,    9,  IFcopyObjectsToLayer);
    TCL_FUNC(CopyObjectsH,          9,  IFcopyObjectsH);
    TCL_FUNC(GetObjectType,         1,  IFgetObjectType);
    TCL_FUNC(GetObjectID,           1,  IFgetObjectID);
    TCL_FUNC(GetObjectArea,         1,  IFgetObjectArea);
    TCL_FUNC(GetObjectPerim,        1,  IFgetObjectPerim);
    TCL_FUNC(GetObjectCentroid,     2,  IFgetObjectCentroid);
    TCL_FUNC(GetObjectBB,           2,  IFgetObjectBB);
    TCL_FUNC(SetObjectBB,           2,  IFsetObjectBB);
    TCL_FUNC(GetObjectListBB,       2,  IFgetObjectListBB);
    TCL_FUNC(GetObjectXY,           2,  IFgetObjectXY);
    TCL_FUNC(SetObjectXY,           3,  IFsetObjectXY);
    TCL_FUNC(GetObjectLayer,        1,  IFgetObjectLayer);
    TCL_FUNC(SetObjectLayer,        2,  IFsetObjectLayer);
    TCL_FUNC(GetObjectFlags,        1,  IFgetObjectFlags);
    TCL_FUNC(SetObjectNoDrcFlag,    2,  IFsetObjectNoDrcFlag);
    TCL_FUNC(SetObjectMark1Flag,    2,  IFsetObjectMark1Flag);
    TCL_FUNC(SetObjectMark2Flag,    2,  IFsetObjectMark2Flag);
    TCL_FUNC(GetObjectState,        1,  IFgetObjectState);
    TCL_FUNC(GetObjectGroup,        1,  IFgetObjectGroup);
    TCL_FUNC(SetObjectGroup,        2,  IFsetObjectGroup);
    TCL_FUNC(GetObjectCoords,       2,  IFgetObjectCoords);
    TCL_FUNC(SetObjectCoords,       3,  IFsetObjectCoords);
    TCL_FUNC(GetObjectMagn,         1,  IFgetObjectMagn);
    TCL_FUNC(SetObjectMagn,         2,  IFsetObjectMagn);
    TCL_FUNC(GetWireWidth,          1,  IFgetWireWidth);
    TCL_FUNC(SetWireWidth,          2,  IFsetWireWidth);
    TCL_FUNC(GetWireStyle,          1,  IFgetWireStyle);
    TCL_FUNC(SetWireStyle,          2,  IFsetWireStyle);
    TCL_FUNC(SetWireToPoly,         1,  IFsetWireToPoly);
    TCL_FUNC(GetWirePoly,           2,  IFgetWirePoly);
    TCL_FUNC(GetLabelText,          1,  IFgetLabelText);
    TCL_FUNC(SetLabelText,          2,  IFsetLabelText);
    TCL_FUNC(GetLabelFlags,         1,  IFgetLabelFlags);
    TCL_FUNC(SetLabelFlags,         2,  IFsetLabelFlags);
    TCL_FUNC(GetInstanceArray,      2,  IFgetInstanceArray);
    TCL_FUNC(SetInstanceArray,      2,  IFsetInstanceArray);
    TCL_FUNC(GetInstanceXform,      1,  IFgetInstanceXform);
    TCL_FUNC(GetInstanceXformA,     2,  IFgetInstanceXformA);
    TCL_FUNC(SetInstanceXform,      2,  IFsetInstanceXform);
    TCL_FUNC(SetInstanceXformA,     2,  IFsetInstanceXformA);
    TCL_FUNC(GetInstanceMaster,     1,  IFgetInstanceMaster);
    TCL_FUNC(SetInstanceMaster,     2,  IFsetInstanceMaster);
    TCL_FUNC(GetInstanceName,       1,  IFgetInstanceName);
    TCL_FUNC(SetInstanceName,       2,  IFsetInstanceName);
    TCL_FUNC(GetInstanceAltName,    1,  IFgetInstanceAltName);
    TCL_FUNC(GetInstanceType,       1,  IFgetInstanceType);
    TCL_FUNC(GetInstanceIdNum,      1,  IFgetInstanceIdNum);
    TCL_FUNC(GetInstanceAltIdNum,   1,  IFgetInstanceAltIdNum);

    // Variables (export)
    TCL_FUNC(JoinLimits,            1,  IFjoinLimits);


    void tcl_register_geom1()
    {
      // General Editing
      cTclIf::register_func("ClearCell",             tclClearCell);
      cTclIf::register_func("Commit",                tclCommit);
      cTclIf::register_func("Undo",                  tclUndo);
      cTclIf::register_func("Redo",                  tclRedo);
      cTclIf::register_func("SelectLast",            tclSelectLast);

      // Current Transform
      cTclIf::register_func("SetTransform",          tclSetTransform);
      cTclIf::register_func("StoreTransform",        tclStoreTransform);
      cTclIf::register_func("RecallTransform",       tclRecallTransform);
      cTclIf::register_func("GetTransformString",    tclGetTransformString);
      cTclIf::register_func("GetCurAngle",           tclGetCurAngle);
      cTclIf::register_func("GetCurMX",              tclGetCurMX);
      cTclIf::register_func("GetCurMY",              tclGetCurMY);
      cTclIf::register_func("GetCurMagn",            tclGetCurMagn);
      cTclIf::register_func("UseTransform",          tclUseTransform);

      // Derived Layers
      cTclIf::register_func("AddDerivedLayer",       tclAddDerivedLayer);
      cTclIf::register_func("RemDerivedLayer",       tclRemDerivedLayer);
      cTclIf::register_func("IsDerivedLayer",        tclIsDerivedLayer);
      cTclIf::register_func("GetDerivedLayerIndex",  tclGetDerivedLayerIndex);
      cTclIf::register_func("GetDerivedLayerExpString", tclGetDerivedLayerExpString);
      cTclIf::register_func("GetDerivedLayerLexpr",  tclGetDerivedLayerLexpr);
      cTclIf::register_func("EvalDerivedLayers",     tclEvalDerivedLayers);
      cTclIf::register_func("ClearDerivedLayers",    tclClearDerivedLayers);

      // Grips
      cTclIf::register_func("AddGrip",               tclAddGrip);
      cTclIf::register_func("RemoveGrip",            tclRemoveGrip);

      // Object Management by Handles
      cTclIf::register_func("ListElecInstances",     tclListElecInstances);
      cTclIf::register_func("ListPhysInstances",     tclListPhysInstances);
      cTclIf::register_func("SelectHandle",          tclSelectHandle);
      cTclIf::register_func("SelectHandleTypes",     tclSelectHandleTypes);
      cTclIf::register_func("AreaHandle",            tclAreaHandle);
      cTclIf::register_func("ObjectHandleDup",       tclObjectHandleDup);
      cTclIf::register_func("ObjectHandlePurge",     tclObjectHandlePurge);
      cTclIf::register_func("ObjectNext",            tclObjectNext);
      cTclIf::register_func("MakeObjectCopy",        tclMakeObjectCopy);
      cTclIf::register_func("ObjectString",          tclObjectString);
      cTclIf::register_func("ObjectCopyFromString",  tclObjectCopyFromString);
      cTclIf::register_func("FilterObjects",         tclFilterObjects);
      cTclIf::register_func("FilterObjectsA",        tclFilterObjectsA);
      cTclIf::register_func("CheckObjectsConnected", tclCheckObjectsConnected);
      cTclIf::register_func("CheckForHoles",         tclCheckForHoles);
      cTclIf::register_func("BloatObjects",          tclBloatObjects);
      cTclIf::register_func("EdgeObjects",           tclEdgeObjects);
      cTclIf::register_func("ManhattanizeObjects",   tclManhattanizeObjects);
      cTclIf::register_func("GroupObjects",          tclGroupObjects);
      cTclIf::register_func("JoinObjects",           tclJoinObjects);
      cTclIf::register_func("SplitObjects",          tclSplitObjects);
      cTclIf::register_func("DeleteObjects",         tclDeleteObjects);
      cTclIf::register_func("SelectObjects",         tclSelectObjects);
      cTclIf::register_func("DeselectObjects",       tclDeselectObjects);
      cTclIf::register_func("MoveObjects",           tclMoveObjects);
      cTclIf::register_func("MoveObjectsToLayer",    tclMoveObjectsToLayer);
      cTclIf::register_func("CopyObjects",           tclCopyObjects);
      cTclIf::register_func("CopyObjectsToLayer",    tclCopyObjectsToLayer);
      cTclIf::register_func("CopyObjectsH",          tclCopyObjectsH);
      cTclIf::register_func("GetObjectType",         tclGetObjectType);
      cTclIf::register_func("GetObjectID",           tclGetObjectID);
      cTclIf::register_func("GetObjectArea",         tclGetObjectArea);
      cTclIf::register_func("GetObjectPerim",        tclGetObjectPerim);
      cTclIf::register_func("GetObjectCentroid",     tclGetObjectCentroid);
      cTclIf::register_func("GetObjectBB",           tclGetObjectBB);
      cTclIf::register_func("SetObjectBB",           tclSetObjectBB);
      cTclIf::register_func("GetObjectListBB",       tclGetObjectListBB);
      cTclIf::register_func("GetObjectXY",           tclGetObjectXY);
      cTclIf::register_func("SetObjectXY",           tclSetObjectXY);
      cTclIf::register_func("GetObjectLayer",        tclGetObjectLayer);
      cTclIf::register_func("SetObjectLayer",        tclSetObjectLayer);
      cTclIf::register_func("GetObjectFlags",        tclGetObjectFlags);
      cTclIf::register_func("SetObjectNoDrcFlag",    tclSetObjectNoDrcFlag);
      cTclIf::register_func("SetObjectMark1Flag",    tclSetObjectMark1Flag);
      cTclIf::register_func("SetObjectMark2Flag",    tclSetObjectMark2Flag);
      cTclIf::register_func("GetObjectState",        tclGetObjectState);
      cTclIf::register_func("GetObjectGroup",        tclGetObjectGroup);
      cTclIf::register_func("SetObjectGroup",        tclSetObjectGroup);
      cTclIf::register_func("GetObjectCoords",       tclGetObjectCoords);
      cTclIf::register_func("SetObjectCoords",       tclSetObjectCoords);
      cTclIf::register_func("GetObjectMagn",         tclGetObjectMagn);
      cTclIf::register_func("SetObjectMagn",         tclSetObjectMagn);
      cTclIf::register_func("GetWireWidth",          tclGetWireWidth);
      cTclIf::register_func("SetWireWidth",          tclSetWireWidth);
      cTclIf::register_func("GetWireStyle",          tclGetWireStyle);
      cTclIf::register_func("SetWireStyle",          tclSetWireStyle);
      cTclIf::register_func("SetWireToPoly",         tclSetWireToPoly);
      cTclIf::register_func("GetWirePoly",           tclGetWirePoly);
      cTclIf::register_func("GetLabelText",          tclGetLabelText);
      cTclIf::register_func("SetLabelText",          tclSetLabelText);
      cTclIf::register_func("GetLabelFlags",         tclGetLabelFlags);
      cTclIf::register_func("GetLabelXform",         tclGetLabelFlags); //alias
      cTclIf::register_func("SetLabelFlags",         tclSetLabelFlags);
      cTclIf::register_func("SetLabelXform",         tclSetLabelFlags); //alias
      cTclIf::register_func("GetInstanceArray",      tclGetInstanceArray);
      cTclIf::register_func("SetInstanceArray",      tclSetInstanceArray);
      cTclIf::register_func("GetInstanceXform",      tclGetInstanceXform);
      cTclIf::register_func("GetInstanceXformA",     tclGetInstanceXformA);
      cTclIf::register_func("SetInstanceXform",      tclSetInstanceXform);
      cTclIf::register_func("SetInstanceXformA",     tclSetInstanceXformA);
      cTclIf::register_func("GetInstanceMaster",     tclGetInstanceMaster);
      cTclIf::register_func("SetInstanceMaster",     tclSetInstanceMaster);
      cTclIf::register_func("GetInstanceName",       tclGetInstanceName);
      cTclIf::register_func("SetInstanceName",       tclSetInstanceName);
      cTclIf::register_func("GetInstanceAltName",    tclGetInstanceAltName);
      cTclIf::register_func("GetInstanceType",       tclGetInstanceType);
      cTclIf::register_func("GetInstanceIdNum",      tclGetInstanceIdNum);
      cTclIf::register_func("GetInstanceAltIdNum",   tclGetInstanceAltIdNum);

      // Variables (export)
      cTclIf::register_func("JoinLimits",            tclJoinLimits);
    }
#endif  // HAVE_TCL
}


// Export to load functions in this script library.
//
void
cEdit::loadScriptFuncs()
{
  using namespace geom1_funcs;

  // General Editing
  SIparse()->registerFunc("ClearCell",             2,  IFclearCell);
  SIparse()->registerFunc("Commit",                0,  IFcommit);
  SIparse()->registerFunc("Undo",                  0,  IFundo);
  SIparse()->registerFunc("Redo",                  0,  IFredo);
  SIparse()->registerFunc("SelectLast",            1,  IFselectLast);

  // Current Transform
  SIparse()->registerFunc("SetTransform",          3,  IFsetTransform);
  SIparse()->registerFunc("StoreTransform",        1,  IFstoreTransform);
  SIparse()->registerFunc("RecallTransform",       1,  IFrecallTransform);
  SIparse()->registerFunc("GetTransformString",    0,  IFgetTransformString);
  SIparse()->registerFunc("GetCurAngle",           0,  IFgetCurAngle);
  SIparse()->registerFunc("GetCurMX",              0,  IFgetCurMX);
  SIparse()->registerFunc("GetCurMY",              0,  IFgetCurMY);
  SIparse()->registerFunc("GetCurMagn",            0,  IFgetCurMagn);
  SIparse()->registerFunc("UseTransform",          3,  IFuseTransform);

  // Derived Layers
  SIparse()->registerFunc("AddDerivedLayer",       3,  IFaddDerivedLayer);
  SIparse()->registerFunc("RemDerivedLayer",       1,  IFremDerivedLayer);
  SIparse()->registerFunc("IsDerivedLayer",        1,  IFisDerivedLayer);
  SIparse()->registerFunc("GetDerivedLayerIndex",  1,  IFgetDerivedLayerIndex);
  SIparse()->registerFunc("GetDerivedLayerExpString", 1,  IFgetDerivedLayerExpString);
  SIparse()->registerFunc("GetDerivedLayerLexpr",  1,  IFgetDerivedLayerLexpr);
  SIparse()->registerFunc("EvalDerivedLayers",     2,  IFevalDerivedLayers);
  SIparse()->registerFunc("ClearDerivedLayers",    1,  IFclearDerivedLayers);

  // Grips
  SIparse()->registerFunc("AddGrip",               4,  IFaddGrip);
  SIparse()->registerFunc("RemoveGrip",            1,  IFremoveGrip);

  // Object Management by Handles
  SIparse()->registerFunc("ListElecInstances",     0,  IFlistElecInstances);
  SIparse()->registerFunc("ListPhysInstances",     0,  IFlistPhysInstances);
  SIparse()->registerFunc("SelectHandle",          0,  IFselectHandle);
  SIparse()->registerFunc("SelectHandleTypes",     1,  IFselectHandleTypes);
  SIparse()->registerFunc("AreaHandle",            5,  IFareaHandle);
  SIparse()->registerFunc("ObjectHandleDup",       2,  IFobjectHandleDup);
  SIparse()->registerFunc("ObjectHandlePurge",     2,  IFobjectHandlePurge);
  SIparse()->registerFunc("ObjectNext",            1,  IFobjectNext);
  SIparse()->registerFunc("MakeObjectCopy",        2,  IFmakeObjectCopy);
  SIparse()->registerFunc("ObjectString",          1,  IFobjectString);
  SIparse()->registerFunc("ObjectCopyFromString",  1,  IFobjectCopyFromString);
  SIparse()->registerFunc("FilterObjects",         5,  IFfilterObjects);
  SIparse()->registerFunc("FilterObjectsA",        5,  IFfilterObjectsA);
  SIparse()->registerFunc("CheckObjectsConnected", 1,  IFcheckObjectsConnected);
  SIparse()->registerFunc("CheckForHoles",         2,  IFcheckForHoles);
  SIparse()->registerFunc("BloatObjects",          5,  IFbloatObjects);
  SIparse()->registerFunc("EdgeObjects",           5,  IFedgeObjects);
  SIparse()->registerFunc("ManhattanizeObjects",   5,  IFmanhattanizeObjects);
  SIparse()->registerFunc("GroupObjects",          2,  IFgroupObjects);
  SIparse()->registerFunc("JoinObjects",           2,  IFjoinObjects);
  SIparse()->registerFunc("SplitObjects",          4,  IFsplitObjects);
  SIparse()->registerFunc("DeleteObjects",         2,  IFdeleteObjects);
  SIparse()->registerFunc("SelectObjects",         2,  IFselectObjects);
  SIparse()->registerFunc("DeselectObjects",       2,  IFdeselectObjects);
  SIparse()->registerFunc("MoveObjects",           6,  IFmoveObjects);
  SIparse()->registerFunc("MoveObjectsToLayer",    8,  IFmoveObjectsToLayer);
  SIparse()->registerFunc("CopyObjects",           7,  IFcopyObjects);
  SIparse()->registerFunc("CopyObjectsToLayer",    9,  IFcopyObjectsToLayer);
  SIparse()->registerFunc("CopyObjectsH",          9,  IFcopyObjectsH);
  SIparse()->registerFunc("GetObjectType",         1,  IFgetObjectType);
  SIparse()->registerFunc("GetObjectID",           1,  IFgetObjectID);
  SIparse()->registerFunc("GetObjectArea",         1,  IFgetObjectArea);
  SIparse()->registerFunc("GetObjectPerim",        1,  IFgetObjectPerim);
  SIparse()->registerFunc("GetObjectCentroid",     2,  IFgetObjectCentroid);
  SIparse()->registerFunc("GetObjectBB",           2,  IFgetObjectBB);
  SIparse()->registerFunc("SetObjectBB",           2,  IFsetObjectBB);
  SIparse()->registerFunc("GetObjectListBB",       2,  IFgetObjectListBB);
  SIparse()->registerFunc("GetObjectXY",           2,  IFgetObjectXY);
  SIparse()->registerFunc("SetObjectXY",           3,  IFsetObjectXY);
  SIparse()->registerFunc("GetObjectLayer",        1,  IFgetObjectLayer);
  SIparse()->registerFunc("SetObjectLayer",        2,  IFsetObjectLayer);
  SIparse()->registerFunc("GetObjectFlags",        1,  IFgetObjectFlags);
  SIparse()->registerFunc("SetObjectNoDrcFlag",    2,  IFsetObjectNoDrcFlag);
  SIparse()->registerFunc("SetObjectMark1Flag",    2,  IFsetObjectMark1Flag);
  SIparse()->registerFunc("SetObjectMark2Flag",    2,  IFsetObjectMark2Flag);
  SIparse()->registerFunc("GetObjectState",        1,  IFgetObjectState);
  SIparse()->registerFunc("GetObjectGroup",        1,  IFgetObjectGroup);
  SIparse()->registerFunc("SetObjectGroup",        2,  IFsetObjectGroup);
  SIparse()->registerFunc("GetObjectCoords",       2,  IFgetObjectCoords);
  SIparse()->registerFunc("SetObjectCoords",       3,  IFsetObjectCoords);
  SIparse()->registerFunc("GetObjectMagn",         1,  IFgetObjectMagn);
  SIparse()->registerFunc("SetObjectMagn",         2,  IFsetObjectMagn);
  SIparse()->registerFunc("GetWireWidth",          1,  IFgetWireWidth);
  SIparse()->registerFunc("SetWireWidth",          2,  IFsetWireWidth);
  SIparse()->registerFunc("GetWireStyle",          1,  IFgetWireStyle);
  SIparse()->registerFunc("SetWireStyle",          2,  IFsetWireStyle);
  SIparse()->registerFunc("SetWireToPoly",         1,  IFsetWireToPoly);
  SIparse()->registerFunc("GetWirePoly",           2,  IFgetWirePoly);
  SIparse()->registerFunc("GetLabelText",          1,  IFgetLabelText);
  SIparse()->registerFunc("SetLabelText",          2,  IFsetLabelText);
  SIparse()->registerFunc("GetLabelFlags",         1,  IFgetLabelFlags);
  SIparse()->registerFunc("GetLabelXform",         1,  IFgetLabelFlags); //alias
  SIparse()->registerFunc("SetLabelFlags",         2,  IFsetLabelFlags);
  SIparse()->registerFunc("SetLabelXform",         2,  IFsetLabelFlags); //alias
  SIparse()->registerFunc("GetInstanceArray",      2,  IFgetInstanceArray);
  SIparse()->registerFunc("SetInstanceArray",      2,  IFsetInstanceArray);
  SIparse()->registerFunc("GetInstanceXform",      1,  IFgetInstanceXform);
  SIparse()->registerFunc("GetInstanceXformA",     2,  IFgetInstanceXformA);
  SIparse()->registerFunc("SetInstanceXform",      2,  IFsetInstanceXform);
  SIparse()->registerFunc("SetInstanceXformA",     2,  IFsetInstanceXformA);
  SIparse()->registerFunc("GetInstanceMaster",     1,  IFgetInstanceMaster);
  SIparse()->registerFunc("SetInstanceMaster",     2,  IFsetInstanceMaster);
  SIparse()->registerFunc("GetInstanceName",       1,  IFgetInstanceName);
  SIparse()->registerFunc("SetInstanceName",       2,  IFsetInstanceName);
  SIparse()->registerFunc("GetInstanceAltName",    1,  IFgetInstanceAltName);
  SIparse()->registerFunc("GetInstanceType",       1,  IFgetInstanceType);
  SIparse()->registerFunc("GetInstanceIdNum",      1,  IFgetInstanceIdNum);
  SIparse()->registerFunc("GetInstanceAltIdNum",   1,  IFgetInstanceAltIdNum);

  // Variables (export)
  SIparse()->registerFunc("JoinLimits",            1,  IFjoinLimits);

  load_script_funcs2();
#ifdef HAVE_PYTHON
  py_register_geom1();
#endif
#ifdef HAVE_TCL
  tcl_register_geom1();
#endif
}


namespace {
    const char *phys_msg = "%s: can be applied to physical data only.";
}


//-------------------------------------------------------------------------
// General Editing
//-------------------------------------------------------------------------

// ClearCell(undoable, layer_list)
//
// This function will clear the content of the present mode
// (electrical or physical) part of the current cell.  If the first
// argument is nonzero, the deletions will be added to the internal
// undo list, otherwise not.  The latter is more efficient, though
// this makes the deletions irreversible.  The second argument, if
// null or empty, indicates that all objects on all layers will be
// deleted, including subcells.  Otherwise this can be set to a string
// containing a space-separated list of layer names, following an
// optional special character '!' or '^' which must be the first
// character in the string if used.  If the special character does not
// appear, the deletions apply only to the layers listed.  If the
// special character appears, the deletions apply only to the layers
// not listed.  Recall that the internal name for the layer that
// contains subcels ls "$$", thus for example using "!  $$" would
// delete all geometry but retain the subcells.
//
// The return value is the number of objects deleted.
//
bool
geom1_funcs::IFclearCell(Variable *res, Variable *args, void*)
{
    bool undoable;
    ARG_CHK(arg_boolean(args, 0, &undoable))
    const char *layer_list;
    ARG_CHK(arg_string(args, 1, &layer_list))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDs *cursd = CurCell();
    if (!cursd)
        return (OK);
    if (cursd->isImmutable())
        return (OK);
    bool notlayers = false;
    SymTab *lt = 0;
    if (layer_list) {
        lt = new SymTab(false, false);
        if (*layer_list == '^' || *layer_list == '!') {
            notlayers = true;
            layer_list++;
        }
        char *tok;
        while ((tok = lstring::gettok(&layer_list)) != 0) {
            CDl *ld = CDldb()->findLayer(tok, DSP()->CurMode());
            delete [] tok;
            if (ld)
                lt->add((unsigned long)ld, 0, false);
        }
    }

    Selections.removeTypes(cursd, 0);

    unsigned int nobjs = 0;
    CDsLgen lgen(cursd, true);
    CDl *ldesc;
    while ((ldesc = lgen.next()) != 0) {
        if (lt) {
            void *xx = lt->get((unsigned long)ldesc);
            if ((xx && !notlayers) || (!xx && notlayers))
                continue;
        }
        CDg gdesc;
        gdesc.init_gen(cursd, ldesc);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (undoable)
                Ulist()->RecordObjectChange(cursd, odesc, 0);
            else
                cursd->unlink(odesc, false);
            nobjs++;
        }
    }

    delete lt;
    res->content.value = nobjs;
    return (OK);
}


// Commit()
//
// The Commit() function terminates the present operation, adding it to
// the undo list.  It will also redisplay any changes.  This function
// should be called after each change or after a group of related
// changes.  It is implicitly called when a script exits.
//
bool
geom1_funcs::IFcommit(Variable*, Variable*, void*)
{
    Ulist()->CommitChanges(true);
    return (OK);
}


// Undo()
//
// Undo the most recent operation.
//
bool
geom1_funcs::IFundo(Variable*, Variable*, void*)
{
    Ulist()->UndoOperation();
    return (OK);
}


// Redo()
//
// Re-do the last undone operation.
//
bool
geom1_funcs::IFredo(Variable*, Variable*, void*)
{
    Ulist()->RedoOperation();
    return (OK);
}


// (int) SelectLast(types)
//
// This function selects objects that have been created by the script
// functions since the last call to Commit() or SelectLast() (which
// calls Commit()), according to type.  The type argument is a string
// whose characters serve to enable selection of a given type of
// object:  'b' for boxes, 'p' for polygons, 'w' for wires, 'l' for
// labels, and 'c' for instances.  If this string is blank or NULL,
// then all objects will be selected.  Objects that are created using
// "PressButton()" or otherwise using Xic input implicitly call
// Commit(), so can't be selected in this manner.
//
bool
geom1_funcs::IFselectLast(Variable *res, Variable *args, void*)
{
    int cnt = 0;
    if (Ulist()->HasChanged()) {
        const char *types;
        ARG_CHK(arg_string(args, 0, &types))
        cnt = Ulist()->SelectLast(types);
        Ulist()->CommitChanges(true);
    }
    res->type = TYP_SCALAR;
    res->content.value = cnt;
    return (OK);
}


//-------------------------------------------------------------------------
// Current Transform
//-------------------------------------------------------------------------

// (int) SetTransform(angle_or_string, reflection, magnification)
//
// This command sets the "current transform" to the values provided. 
// It is similar in action to the controls a in the Current Transform
// panel.  The first argument can be a floating point angle that will
// be snapped to the nearest multiple of 45 degrees in physical mode,
// 90 degrees in electrical mode.  If bit 1 of reflection is set, a
// reflection of the x-axis is specified.  If bit 2 of reflection is
// set, a reflection of the y-axis is specified.  The magnification
// sets the scaling applied to transformed objects, and is accepted
// only while in physical mode.  It is ignored if less than or equal
// to zero.
//
// The first argument can alternatively be a string, in the format as
// returned from GetTransformString.  The string will be parsed, and
// if no error the transform will be set.  The two remaining arguments
// are ignored, but must be given (0 can be passed for both).
//
// The return value is 1 on success, 0 otherwise.
//
bool
geom1_funcs::IFsetTransform(Variable *res, Variable *args, void*)
{
    if (args[0].type == TYP_STRING) {
        const char *string;
        ARG_CHK(arg_string(args, 0, &string))
        res->type = TYP_SCALAR;
        res->content.value = ED()->setCurTransform(string);
        return (OK);
    }

    double ang;
    ARG_CHK(arg_real(args, 0, &ang))
    int mirror;
    ARG_CHK(arg_int(args, 1, &mirror))
    double magn;
    ARG_CHK(arg_real(args, 2, &magn))

    int iang;
    if (DSP()->CurMode() == Electrical)
        iang = 90 * (int)((ang + 45)/90.0);
    else
        iang = 45 * (int)((ang + 22.5)/45.0);
    while (iang < 0)
        iang += 360;
    while (iang >= 360)
        iang -= 360;
    ED()->setCurTransform(iang, mirror & 1, mirror & 2, magn);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) StoreTransform(register)
//
// This function will save the current transform settings into a
// register, which can be recalled with RecallTransform().  The
// argument is a register number 0-5.  These correspond to the "last"
// and registers 1-5 in the current transform pop-up.  This function
// returns 1 on success, 0 if the argument is out of range.
//
bool
geom1_funcs::IFstoreTransform(Variable *res, Variable *args, void*)
{
    int reg;
    ARG_CHK(arg_int(args, 0, &reg))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (reg >= 0 && reg <= 5) {
        ED()->saveCurTransform(reg);
        res->content.value = 1;
    }
    return (OK);
}


// (int) RecallTransform(register)
//
// This function will restore the transform settings previously saved
// with StoreTransform().  The argument is a register number 0-5.
// These correspond to the "last" and registers 1-5 in the current
// transform pop-up.  This function returns 1 on success, 0 if the
// argument is out of range.
//
bool
geom1_funcs::IFrecallTransform(Variable *res, Variable *args, void*)
{
    int reg;
    ARG_CHK(arg_int(args, 0, &reg))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (reg >= 0 && reg <= 5) {
        ED()->recallCurTransform(reg);
        res->content.value = 1;
    }
    return (OK);
}


// (int) GetTransformString()
//
// Return a string describing the current transform, an empty string
// will indicate the identity transform.  The string is a sequence of
// tokens and contains no white space.  It is the same format used to
// indicate the current transform in the Xic status line.  The tokens
// are:
//
//   [R<ang>][MY][MX][M<magn>]
//
// The square brackets indicate that each token is optional and do not
// appear in the string.  If the rotation angle is nonzero, the first
// token will appear, where <ang> is the angle in degrees.  This is an
// integer multiple of 45 degrees in physical mode, 90 degrees in
// electrical mode, larger than zero and smaller than 360.
//
// If reflection of Y or X is in force, one or both of the mext two
// tokens will appear.  These are literal.  If the magnification is
// not unity, the final token will appear, with <magn> being a real
// number in the range 0.001 through 1000.0.
//
// The ordeer of the tokens must be as shown.
//
// The returned string, or one in the same format, can be passed to
// the first argument of SetTransform.
//
bool
geom1_funcs::IFgetTransformString(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string = GEO()->curTx()->tform_string();
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) GetCurAngle()
//
// Returns the rotation angle of the current transform, in degrees.
// This is 0, 45, 90, 135, 180, 225, 270, 315 in physical mode, or 0,
// 90, 180, 270 in electrical mode.  The SetTransform() function can
// be used to set the rotation angle.
//
bool
geom1_funcs::IFgetCurAngle(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = GEO()->curTx()->angle();
    return (OK);
}


// (int) GetCurMX()
//
// Returns 1 if the current transform mirrors the x-axis, 0 otherwise.
// The SetTransform() function can be used to set the mirror
// transformations.
//
bool
geom1_funcs::IFgetCurMX(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = GEO()->curTx()->reflectX();
    return (OK);
}


// (int) GetCurMY()
//
// Returns 1 if the current transform mirrors the y-axis, 0 otherwise.
// The SetTransform() function can be used to set the mirror
// transformations.
//
bool
geom1_funcs::IFgetCurMY(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = GEO()->curTx()->reflectY();
    return (OK);
}


// (real) GetCurMagn()
//
// Returns the magnification component of the current transform.  The
// SetTransform() function can be used to set the magnification.
//
bool
geom1_funcs::IFgetCurMagn(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = GEO()->curTx()->magn();
    return (OK);
}


// (int) UseTransform(enable, x, y)
//
// This command enables and disables use of the current transform in
// the ShowGhost() function, as well as the functions that create
// objects:  Box(), Polygon(), Arc(), Wire(), and Label().  The
// functions Move(), Copy(), Logo(), and Place() naturally use the
// current transform and are unaffected by this function.
//
bool
geom1_funcs::IFuseTransform(Variable *res, Variable *args, void*)
{
    bool yesno;
    ARG_CHK(arg_boolean(args, 0, &yesno))
    int x;
    ARG_CHK(arg_coord(args, 1, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 2, &y, DSP()->CurMode()))

    res->content.value = SIlcx()->applyTransform() ? 1.0 : 0.0;
    res->type = TYP_SCALAR;
    if (yesno)
        SIlcx()->setApplyTransform(true, x, y);
    else
        SIlcx()->setApplyTransform(false, 0, 0);
    return (OK);
}


//-------------------------------------------------------------------------
// Derived Layers
//-------------------------------------------------------------------------

// (int) AddDerivedLayer(lname, index, lexpr)
//
// This will add a derived layer to the database, under the name given
// in the first argument.  The second argument is an integer layer
// number for the layer, which is used for ordering then the derived
// layers are printed, for example to an updated technology file.  If
// not positive, Xic will generate a number to be used for a new
// layer.  Numbers need not be unique, sorting is alphabetic among
// derived layer names with the same index number.  If a derived layer
// of the same name already exists, it will be silently overwritten. 
// The third argument is a string giving a layer expression.  The
// expression can reference by name ordinary layers and derived
// layers.  The expression is not parsed until evaluation time.
//
// The function fails if either the name or expression are null or
// empty strings.
//
bool
geom1_funcs::IFaddDerivedLayer(Variable *res, Variable *args, void*)
{
    const char *lname;
    ARG_CHK(arg_string(args, 0, &lname))
    int index;
    ARG_CHK(arg_int(args, 1, &index))
    const char *lexpr;
    ARG_CHK(arg_string(args, 2, &lexpr))

    if (!lname || !*lname) {
        Errs()->add_error("AddDerivedLayer: null or empty layer name");
        return (BAD);
    }
    if (!lexpr || !*lexpr) {
        Errs()->add_error("AddDerivedLayer: null or empty layer expr");
        return (BAD);
    }

    int mode = CLdefault;
    const char *bak = lexpr; 
    char *tok = lstring::gettok(&lexpr);
    if (lstring::cieq(tok, "join"))
        mode = CLjoin;
    else if (lstring::cieq(tok, "split") || lstring::cieq(tok, "splith"))
        mode = CLsplitH;
    else if (lstring::cieq(tok, "splitv"))
        mode = CLsplitV;
    else
        lexpr = bak;
    delete [] tok;

    res->type = TYP_SCALAR;
    res->content.value =
        (CDldb()->addDerivedLayer(lname, index, mode, lexpr) != 0);
    return (OK);
}


// (int) RemDerivedLayer(lname)
//
// If a derived layer exists with the given name, remove the
// definition from the internal registry, so that the derived layer
// definition and any existing geometry becomes inaccessible.  The
// derived layer definition can be restored with AddDerivedLayer.  If
// the derived layer is found and removed, this function will return
// 1, otherwise 0 is returned.
//
bool
geom1_funcs::IFremDerivedLayer(Variable *res, Variable *args, void*)
{
    const char *lname;
    ARG_CHK(arg_string(args, 0, &lname))

    res->type = TYP_SCALAR;
    res->content.value = (CDldb()->remDerivedLayer(lname) != 0);
    return (OK);
}


// (int) IsDerivedLayer(lname)
//
// This function will return 1 if the string argument matches a
// derived layer name in the database, 0 otherwise.  Matching is
// case-insensitive.
//
// The name can be in the form "layer:purpose" as for normal Xic
// layers, however the entire token is taken verbatim.  This is a
// subtle difference from normal layers, where for example
// "m1:drawing" and "m1" are equivalent (the <tt>drawing</tt> purpose
// being the default).  As derived layer names, the two would differ,
// and the notion of a purpose does not apply to derived layers.
//
bool
geom1_funcs::IFisDerivedLayer(Variable *res, Variable *args, void*)
{
    const char *lname;
    ARG_CHK(arg_string(args, 0, &lname))

    res->type = TYP_SCALAR;
    res->content.value = (CDldb()->findDerivedLayer(lname) != 0);
    return (OK);
}


// (int) GetDerivedLayerIndex(lname)
//
// This returns a positive integer which is the layer index number of
// the derived layer whose name was given, or 0 if no derived layer
// can be found with that name (case insensitive).
//
bool
geom1_funcs::IFgetDerivedLayerIndex(Variable *res, Variable *args, void*)
{
    const char *lname;
    ARG_CHK(arg_string(args, 0, &lname))

    res->type = TYP_SCALAR;
    CDl *ld = CDldb()->findDerivedLayer(lname);
    res->content.value = (ld ? ld->drvIndex() : 0);
    return (OK);
}


// (string) GetDerivedLayerExpString(lname)
//
// This returns the layer expression string for the derived layer
// whose name is passed.  If the derived layer is not found, a null
// string is returned.
//
bool
geom1_funcs::IFgetDerivedLayerExpString(Variable *res, Variable *args, void*)
{
    const char *lname;
    ARG_CHK(arg_string(args, 0, &lname))

    res->type = TYP_STRING;
    CDl *ld = CDldb()->findDerivedLayer(lname);
    res->content.string = (ld ? lstring::copy(ld->drvExpr()) : 0);
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (layerexpr) GetDerivedLayerLexpr(lname, noexp)
//
// This returns a parsed layer expression object created from the
// layer expression of the derived layer whose name is passed.  This
// can be passed to other functions which can use this data type.  If
// there is a parse error, the function fails fatally.  Otherwise the
// return is a valid parse tree object.
//
// The boolean second argument will suppress derived layer expansion
// if set.
//
// There are two ways to handle derived layers.  Generally, layer
// expression parse trees are expanded (second argument is false),
// meaning that when a derived layer is encountered, the parser
// recursively descends into the layer's expression.  The resulting
// tree references only normal layers, and evaluation is
// straightforward.
//
// A second approach might be faster.  The parse trees are not
// expanded (second argument is true), and a parse node to a derived
// layer contains a layer descriptor, just as for normal layers. 
// Before any computation, EvalDerivedLayers must be called, which
// actually creates database objects in a database for the derived
// layer.  Evaluation involves only finding the geometry in the search
// area, as for a normal layer.
//
bool
geom1_funcs::IFgetDerivedLayerLexpr(Variable *res, Variable *args, void*)
{
    const char *lname;
    ARG_CHK(arg_string(args, 0, &lname))
    bool noexp;
    ARG_CHK(arg_boolean(args, 1, &noexp))

    CDl *ld = CDldb()->findDerivedLayer(lname);
    res->type = TYP_LEXPR;
    if (ld) {
        const char *expr = ld->drvExpr();
        sLspec *lspec = new sLspec;
        if (lspec->parseExpr(&expr, noexp)) {
            if (!lspec->setup()) {
                delete lspec;
                return (BAD);
            }
            res->content.lspec = lspec;
            return (OK);
        }
        delete lspec;
    }
    return (BAD);
}


// (string) EvalDerivedLayers(list, array)
//
// Derived layer evaluation objects (such as the return from
// GetDerivedLayerLexpr) that are not recursively expanded must have
// derived layer geometry precomputed before use.  This function
// creates derived layer geometry for this purpose.
//
// Evaluation creates the geometry described by the layer expression. 
// Derived layers are never visible, so this geometry is internal, but
// can be accessed, e.g., by design rule evaluation functions, or used
// to create normal layers with the !layer command or the Evaluate
// Layer Expression panel from the Edit menu.
//
// The first argument is a string containing a list of derived layer
// names, separated by commas or white space.  The function will
// evaluate these derived layers, and any derived layers referenced in
// their layer expressions, in an order such that the derived layers
// will be evaluated before being referenced during another
// evaluation.
//
// All geometry created will exist in the current cell, and the layer
// expressions will source all levels of the hierarchy.  Any geometry
// left in the current cell from a previous evaluation will be cleared
// first.  Derived layer geometry in subcells is ignored.
//
// The second argument can set the area where the layers will be
// evaluated, which can be any rectangular region of the current cell. 
// This can be an array of size four or larger, specifying left,
// bottom, right, and top coordinates in microns in the 0, 1, 2, 3
// indices.  The argument can also be a scalar 0 which indicates to
// use the entire current cell.
//
// The return is a string listing all of the derived layers evaluated,
// which will include derived layers referenced by the original list
// but not included in the list.  This should be passed to
// ClearDerivedLayers when finished using the layers.
//
bool
geom1_funcs::IFevalDerivedLayers(Variable *res, Variable *args, void*)
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    const char *list;
    ARG_CHK(arg_string(args, 0, &list))
    double *vals = 0;
    if (args[1].type == TYP_SCALAR)
        ;
    else {
        ARG_CHK(arg_array_if(args, 1, &vals, 4))
    }

    res->type = TYP_STRING;
    CDll *l0 = 0;
    if (list && *list) {
        char *tok;
        while ((tok = lstring::gettok(&list, ",")) != 0) {
            CDl *ld = CDldb()->findDerivedLayer(tok);
            if (!ld) {
                Errs()->add_error(
                    "EvalDerivedLayers: no such derived layer %s.", tok);
                delete [] tok;
                l0->free();
                return (BAD);
            }
            l0 = new CDll(ld, l0);
            delete [] tok;
        }
    }
    if (l0) {
        BBox *AOI = 0, BB;
        if (vals) {
            BB.left = INTERNAL_UNITS(vals[0]);
            BB.bottom = INTERNAL_UNITS(vals[1]);
            BB.right = INTERNAL_UNITS(vals[2]);
            BB.top = INTERNAL_UNITS(vals[3]);
            BB.fix();
            AOI = &BB;
        }
        if (ED()->evalDerivedLayers(&l0, cursdp, AOI) != XIok) {
            Errs()->add_error("EvalDerivedLayers: evaluation failed.");
            for (CDll *l = l0; l; l = l->next)
                cursdp->clearLayer(l->ldesc);
            l0->free();
            return (BAD);
        }
        sLstr lstr;
        for (CDll *l = l0; l; l = l->next) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add(l->ldesc->name());
        }
        l0->free();
        res->content.string = lstr.string_trim();
    }
    else
        res->content.string = lstring::copy("");
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) ClearDerivedLayers(list)
//
// The argument is a string containing a list of derived layer names,
// separated by commas or white space.  This may be the return from
// EvalDerivedLayers.  All of the layers listed will be cleared in the
// current cell.  If a layer name is not resolved as a derived layer,
// it is silently ignored.  Clearing already clear layers is not an
// error.  Derived layers should be cleared after their work is done,
// to recycle memory.  The return value is an integer count of the
// number of derived layers that were cleared.
//
bool
geom1_funcs::IFclearDerivedLayers(Variable *res, Variable *args, void*)
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    const char *list;
    ARG_CHK(arg_string(args, 0, &list))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (!list || !*list)
        return (OK);
    char *tok;
    int cnt = 0;
    while ((tok = lstring::gettok(&list, ",")) != 0) {
        CDl *ld = CDldb()->findDerivedLayer(tok);
        if (ld) {
            cursdp->clearLayer(ld);
            cnt++;
        }
        delete [] tok;
    }
    res->content.value = cnt;
    return (OK);
}


//-------------------------------------------------------------------------
// Grips
//-------------------------------------------------------------------------

/* XXX Not documented, incomplete, development needed.
What can be done with these?

Idea: how about an electrical device or object that allows control
of a parameter by dragging a slider?
*/

// (int) AddGrip(x1, y1, x2, y2)
bool
geom1_funcs::IFaddGrip(Variable *res, Variable *args, void*)
{
    int x1;
    ARG_CHK(arg_coord(args, 0, &x1, Physical))
    int y1;
    ARG_CHK(arg_coord(args, 1, &y1, Physical))
    int x2;
    ARG_CHK(arg_coord(args, 2, &x2, Physical))
    int y2;
    ARG_CHK(arg_coord(args, 3, &y2, Physical))

    res->type = TYP_SCALAR;
    if (!ED()->getGripDb())
        ED()->setGripDb(new cGripDb);
    sGrip *grip = new sGrip(0);
    grip->set(x1, y1, x2, y2);
    res->content.value = ED()->getGripDb()->saveGrip(grip);
    return (OK);
}


// (int) RemoveGrip(id)
bool
geom1_funcs::IFremoveGrip(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_int(args, 0, &id))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (ED()->getGripDb())
        res->content.value = ED()->getGripDb()->deleteGrip(0, id);;
    return (OK);
}


//-------------------------------------------------------------------------
// Object Management by Handles
//-------------------------------------------------------------------------

// (object_handle) ListElecInstances()
//
// This function returns a handle to a complete list of cell instances
// found in the electrical part of the current cell.  Operation is
// identical in electrical and physical modes.  In the schematic, cell
// instances represent subcircuits, devices, and pins.  The
// GetInstanceXxx functions can be used to obtain information about
// the instances.
//
bool
geom1_funcs::IFlistElecInstances(Variable *res, Variable*, void*)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (BAD);
    CDol *ol = 0, *oe = 0;
    CDg gdesc;
    gdesc.init_gen(cursde, CellLayer());
    CDo *od;
    while ((od = gdesc.next()) != 0) {
        if (!ol)
            ol = oe = new CDol(od, 0);
        else {
            oe->next = new CDol(od, 0);
            oe = oe->next;
        }
    }
    sHdl *hdl = new sHdlObject(ol, cursde);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (object_handle) ListPhysInstances()
//
// This function returns a handle to a complete list of cell instances
// found in the physical layout of the current cell.  Operation is
// identical in electrical and physical modes.  The GetInstanceXxx
// functions can be used to obtain information about the cell
// instances.
//
bool
geom1_funcs::IFlistPhysInstances(Variable *res, Variable*, void*)
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    CDol *ol = 0, *oe = 0;
    CDg gdesc;
    gdesc.init_gen(cursdp, CellLayer());
    CDo *od;
    while ((od = gdesc.next()) != 0) {
        if (!ol)
            ol = oe = new CDol(od, 0);
        else {
            oe->next = new CDol(od, 0);
            oe = oe->next;
        }
    }
    sHdl *hdl = new sHdlObject(ol, cursdp);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (object_handle) SelectHandle()
//
// This function returns a handle to the list of objects currently
// selected.  The list is copied internally, and so is unchanged if
// the objects are subsequently deselected.
//
// A handle to the object list is returned.  The ObjectNext() function
// is used to advance the handle to point to the next object in the
// list.  The HandleContent() function returns the number of objects
// remaining in the list.
//
bool
geom1_funcs::IFselectHandle(Variable *res, Variable*, void*)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    CDol *ol = 0, *oe = 0;
    sSelGen sg(Selections, cursd);
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (!ol)
            ol = oe = new CDol(od, 0);
        else {
            oe->next = new CDol(od, 0);
            oe = oe->next;
        }
    }
    sHdl *hdl = new sHdlObject(ol, cursd);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (object_handle) SelectHandleTypes(types)
//
// This function returns a handle to a list of objects that are
// currently selected, but only the types of objects specified in the
// argument are included.  The argument is a string which specifies
// the types of objects to include.  If zero or an empty string is
// passed, all types are included, and the function is equivalent to
// SelectHandle().  Otherwise the characters in the string signify
// which objects to include:
//
//      'b'   boxes
//      'p'   polygons
//      'w'   wires
//      'l'   labels
//      'c'   subcells
//
// For example, passing "pwb" would include polygons, wires, and boxes
// only.  The order of the characters is unimportant.
//
bool
geom1_funcs::IFselectHandleTypes(Variable *res, Variable *args, void*)
{
    const char *types;
    ARG_CHK(arg_string(args, 0, &types))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDs *cursd = CurCell();
    if (!cursd)
        return (OK);
    CDol *o0 = 0, *oe = 0;
    sSelGen sg(Selections, cursd, types);
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (!o0)
            o0 = oe = new CDol(od, 0);
        else {
            oe->next = new CDol(od, 0);
            oe = oe->next;
        }
    }
    if (o0) {
        sHdl *hdl = new sHdlObject(o0, cursd);
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    return (OK);
}


// (object_handle) AreaHandle(l, b, r, t, types)
//
// This function creates a list of objects that touch the rectangular
// area specified by the first four coordinates (which are the left,
// bottom, right, and top values of the rectangle).  The fifth
// argument is a string which specifies the types of objects to
// include.  If zero or an empty string is passed, all types are
// included, otherwise the characters in the string signify which
// objects to include:
//
//      'b'   boxes
//      'p'   polygons
//      'w'   wires
//      'l'   labels
//      'c'   subcells
//
// For example, passing "pwb" would list polygons, wires, and boxes
// only.  The order of the characters is unimportant.
//
// A handle to the object list is returned.  The ObjectNext() function
// is used to advance the handle to point to the next object in the
// list.  The HandleContent() function returns the number of objects
// remaining in the list.
//
bool
geom1_funcs::IFareaHandle(Variable *res, Variable *args, void*)
{
    BBox BB;
    ARG_CHK(arg_coord(args, 0, &BB.left, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 1, &BB.bottom, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 2, &BB.right, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 3, &BB.top, DSP()->CurMode()))
    const char *types;
    ARG_CHK(arg_string(args, 4, &types))

    BB.fix();
    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    if (BB.left > BB.right)
        mmSwapInts(BB.left, BB.right);
    if (BB.top < BB.bottom)
        mmSwapInts(BB.bottom, BB.top);
    CDol *slist = Selections.selectItems(cursd, types, &BB,
        PSELstrict_area);
    CDol *ol = sHdl::sel_list(slist);
    slist->free();
    sHdl *hdl = new sHdlObject(ol, cursd);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (object_handle) ObjectHandleDup(object_handle, types)
//
// This function creates a new handle and list of objects.  The new
// object list consists of those objects in the list referenced by the
// argument whose types are given in the string types argument.  If
// zero or an empty string is passed, all types are included,
// otherwise the characters in the string signify which objects to
// include:
//
//      'b'   boxes
//      'p'   polygons
//      'w'   wires
//      'l'   labels
//      'c'   subcells
//
// The return value is a handle, or 0 if an error occurred.  Note that
// the new handle may be empty if there were no matching objects.  The
// function will fail if the handle argument is not a pointer to an
// object list.
//
bool
geom1_funcs::IFobjectHandleDup(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *types;
    ARG_CHK(arg_string(args, 1, &types))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *o0 = 0, *oe = 0;
        bool copies = ((sHdlObject*)hdl)->copies;
        for (CDol *ol = (CDol*)hdl->data; ol; ol = ol->next) {
            if (!types || !*types || strchr(types, ol->odesc->type())) {
                if (!o0)
                    o0 = oe = new CDol(ol->odesc, 0);
                else {
                    oe->next = new CDol(ol->odesc, 0);
                    oe = oe->next;
                }
                if (copies)
                    oe->odesc = oe->odesc->copyObject();
            }
        }
        sHdl *h = new sHdlObject(o0, ((sHdlObject*)hdl)->sdesc, copies);
        res->type = TYP_HANDLE;
        res->content.value = h->id;
    }
    return (OK);
}


// (int) ObjectHandlePurge(object_handle, types)
//
// This function will purge from the list of objects referenced by the
// handle argument objects with types listed in the types string.  If
// zero or an empty string is passed, all types are deleted, otherwise
// the characters in the string signify which objects to delete:
//
//      'b'   boxes
//      'p'   polygons
//      'w'   wires
//      'l'   labels
//      'c'   subcells
//
// The return value is the number of objects remaining in the list.
// The function will fail if the handle argument does not reference a
// list of objects.
//
bool
geom1_funcs::IFobjectHandlePurge(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *types;
    ARG_CHK(arg_string(args, 1, &types))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *op = 0, *on;
        for (CDol *ol = (CDol*)hdl->data; ol; ol = on) {
            on = ol->next;
            if (!types || !*types || strchr(types, ol->odesc->type())) {
                if (!op)
                    hdl->data = on;
                else
                    op->next = on;
                if (((sHdlObject*)hdl)->copies)
                    delete ol->odesc;
                delete ol;
                continue;
            }
            op = ol;
        }
        int cnt = 0;
        for (CDol *ol = (CDol*)hdl->data; ol; ol = ol->next, cnt++) ;
        res->content.value = cnt;
    }
    return (OK);
}


// (int) ObjectNext(object_handle)
//
// This function is called with a handle to a list of objects, and
// causes the handle to reference the next object in the list.  If
// there are no more objects, the handle is deleted, and this function
// returns zero.  Otherwise, 1 is returned.  This function will fail
// if the handle passed is not a handle to an object list.
//
bool
geom1_funcs::IFobjectNext(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        // iterator may free hdl
        bool cp = ((sHdlObject*)hdl)->copies;
        CDo *obj = (CDo*)hdl->iterator();
        if (cp)
            delete obj;
        if (sHdl::get(id))
            res->content.value = 1.0;
    }
    return (OK);
}


// (object_handle) MakeObjectCopy(numpts, array)
//
// This function creates an object copy from the numpts coordinate
// pairs in the array.  The function returns an object list handle
// referencing the "copy", which can be used in the same manner as
// copies of "real" objects.  The coordinate list must be closed,
// i.e., the last coordinate pair must be the same as the first.  If
// the coordinates represent a rectangle, a box object is created,
// otherwise the object is a polygon.  Coordinates are in microns,
// relative to the origin of the current cell.  The object is
// associated with the current layer (but of course it really does not
// exist on that layer).
//
bool
geom1_funcs::IFmakeObjectCopy(Variable *res, Variable *args, void*)
{
    Poly poly;
    ARG_CHK(arg_int(args, 0, &poly.numpts))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 2*poly.numpts))

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    poly.points = new Point[poly.numpts];
    if (cursd->isElectrical()) {
        int i, j;
        for (j = i = 0; i < poly.numpts; i++, j += 2)
            poly.points[i].set(
                ELEC_INTERNAL_UNITS(vals[j]), ELEC_INTERNAL_UNITS(vals[j+1]));
    }
    else {
        int i, j;
        for (j = i = 0; i < poly.numpts; i++, j += 2)
            poly.points[i].set(
                INTERNAL_UNITS(vals[j]), INTERNAL_UNITS(vals[j+1]));
    }

    CDo *cdo;
    if (poly.is_rect()) {
        BBox BB(poly.points);
        delete poly.points;
        cdo = new CDo(LT()->CurLayer(), &BB);
    }
    else
        cdo = new CDpo(LT()->CurLayer(), &poly);
    cdo->set_copy(true);
    poly.points = 0;
    CDol *o0 = new CDol(cdo, 0);
    sHdl *hnew = new sHdlObject(o0, cursd, true);
    res->type = TYP_HANDLE;
    res->content.value = hnew->id;
    return (OK);
}


// (string) ObjectString(object_handle)
//
// This function returns a CIF-like string describing the object
// pointed to by the given object handle.  This provides all of the
// geometric information for the object.  Strings of this format can
// be reconverted to object copies with the ObjectCopyFromString
// function.
//
// On error or for an empty handle, a null string is returned.  The
// function will fail if the argument is not a handle to an object
// list.
//
bool
geom1_funcs::IFobjectString(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    if (id < 0)
        return (BAD);
    res->type = TYP_STRING;
    res->content.string = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            res->content.string = ol->odesc->cif_string(0, 0, true);
            res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (object_handle) ObjectCopyFromString(string, layer)
//
// This function will create an object copy from the CIF-like string,
// as generated by the ObjectString function.  Boxes, polygons, and
// wires are supported, labels and subcells will not return a handle.
// The object will be associated with the layer named in the second
// argument.  The layer will be created if it does not exist.  Only
// physical layers are accepted.
//
// On success, a handle to an object list containing the new copy is
// returned.  On error, a scalar zero is returned.  The function will
// fail if the string is null or a new layer cannot be created.
//
bool
geom1_funcs::IFobjectCopyFromString(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))
    const char *layer;
    ARG_CHK(arg_string(args, 1, &layer))

    if (!string)
        return (BAD);
    CDl *ld;
    if (!layer || !*layer)
        ld = LT()->CurLayer();
    else
        ld = CDldb()->newLayer(layer, Physical);
    if (!ld)
        return (BAD);

    CDo *od = CDo::fromCifString(ld, string);
    if (od) {
        res->type = TYP_HANDLE;
        sHdl *hdl = new sHdlObject(new CDol(od, 0), 0, true);
        res->content.value = hdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0;
    }
    return (OK);
}


// (object_handle) FilterObjects(object_list, template_list, all, touchok,
//   remove)
//
// This function creates a handle to a list of objects that is a
// subset of the objects contained in the object_list.  The objects in
// the new list are those that touch or overlap objects in the
// template_list, which is also a handle to a list of objects.
//
// If all is nonzero, all of the objects in the template_list will be
// used for comparison, otherwise only the head object in the template
// list will be used.
//
// If touchok is nonzero, objects in the object list that touch but do
// not overlap the template object(s) will be added to the new list,
// otherwise not.
//
// If remove is nonzero, objects that are added to the new list are
// removed from the object_list, otherwise the object_list is not
// touched.  The function will fail if the handle arguments are of the
// wrong type.  The return value is a new handle to a list of objects.
//
bool
geom1_funcs::IFfilterObjects(Variable *res, Variable *args, void*)
{
    int id_list;
    ARG_CHK(arg_handle(args, 0, &id_list))
    int id_tmpl;
    ARG_CHK(arg_handle(args, 1, &id_tmpl))
    bool useall;
    ARG_CHK(arg_boolean(args, 2, &useall))
    bool t_ok;
    ARG_CHK(arg_boolean(args, 3, &t_ok))
    bool remove;
    ARG_CHK(arg_boolean(args, 4, &remove))

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    sHdl *hdl = sHdl::get(id_list);
    sHdl *hdl_t = sHdl::get(id_tmpl);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl && hdl_t) {
        if (hdl->type != HDLobject || hdl_t->type != HDLobject)
            return (BAD);
        bool copies = ((sHdlObject*)hdl)->copies;
        CDol *o0 = 0, *oe = 0, *op = 0, *on;
        for (CDol *o1 = (CDol*)hdl->data; o1; o1 = on) {
            on = o1->next;
            bool ok = false;
            for (CDol *o2 = (CDol*)hdl_t->data; o2; o2 = o2->next) {
                if (o1->odesc->intersect(o2->odesc, t_ok)) {
                    ok = true;
                    break;
                }
                if (!useall)
                    break;
            }
            if (ok) {
                if (remove) {
                    if (op)
                        op->next = on;
                    else
                        hdl->data = on;
                    o1->next = 0;
                    if (!o0)
                        o0 = oe = o1;
                    else {
                        oe->next = o1;
                        oe = oe->next;
                    }
                    continue;
                }
                if (!o0)
                    o0 = oe = new CDol(o1->odesc, 0);
                else {
                    oe->next = new CDol(o1->odesc, 0);
                    oe = oe->next;
                }
                if (copies)
                    oe->odesc = oe->odesc->copyObject();
            }
            op = o1;
        }
        if (o0) {
            sHdl *hdltmp = new sHdlObject(o0, cursd, copies);
            res->type = TYP_HANDLE;
            res->content.value = hdltmp->id;
        }
    }
    return (OK);
}


// (object_handle) FilterObjectsA(object_list, array, array_size, touchok,
//   remove)
//
// This function creates a handle to a list of objects, which consist
// of the objects in the object_list that touch or overlap the polygon
// defined in the array.  The array_size is the number of x-y
// coordinates represented in the array.  In the array, the values are
// x-y coordinate pairs representing the polygon vertices, and the
// first pair must match the last pair (i.e., the figure must be
// closed).  The values are specified in microns.  If touchok is
// nonzero, objects that touch but do not overlap the polygon will be
// added to the list, otherwise not.  If remove is nonzero, objects
// that are added to the new list are removed from the object_list,
// otherwise the object_list is not touched.
//
// The function will fail if array_size is less than 4, or the size of
// the array is less than 2Xarray_size, or if the handle argument is
// not a handle to a list of objects.  The return value is a new
// handle to a list of objects.
//
bool
geom1_funcs::IFfilterObjectsA(Variable *res, Variable *args, void*)
{
    int id_list;
    ARG_CHK(arg_handle(args, 0, &id_list))
    int asize;
    ARG_CHK(arg_int(args, 2, &asize))
    double *vals;
    ARG_CHK(arg_array_if(args, 1, &vals, 2*asize))
    bool t_ok;
    ARG_CHK(arg_boolean(args, 3, &t_ok))
    bool remove;
    ARG_CHK(arg_boolean(args, 4, &remove))

    if (asize < 4)
        return (BAD);
    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    sHdl *hdl = sHdl::get(id_list);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        Poly po;
        po.points = new Point[asize];
        po.numpts = asize;
        if (cursd->isElectrical()) {
            for (int i = 0; i < asize; i++)
                po.points[i].set(ELEC_INTERNAL_UNITS(vals[2*i]),
                    ELEC_INTERNAL_UNITS(vals[2*i + 1]));
        }
        else {
            for (int i = 0; i < asize; i++)
                po.points[i].set(INTERNAL_UNITS(vals[2*i]),
                    INTERNAL_UNITS(vals[2*i + 1]));
        }

        bool copies = ((sHdlObject*)hdl)->copies;
        CDol *o0 = 0, *oe = 0, *op = 0, *on;
        for (CDol *o1 = (CDol*)hdl->data; o1; o1 = on) {
            on = o1->next;
            if (o1->odesc->intersect(&po, t_ok)) {
                if (remove) {
                    if (op)
                        op->next = on;
                    else
                        hdl->data = on;
                    o1->next = 0;
                    if (!o0)
                        o0 = oe = o1;
                    else {
                        oe->next = o1;
                        oe = oe->next;
                    }
                    continue;
                }
                if (!o0)
                    o0 = oe = new CDol(o1->odesc, 0);
                else {
                    oe->next = new CDol(o1->odesc, 0);
                    oe = oe->next;
                }
                if (copies)
                    oe->odesc = oe->odesc->copyObject();
            }
            op = o1;
        }
        if (o0) {
            sHdl *hdltmp = new sHdlObject(o0, cursd, copies);
            res->type = TYP_HANDLE;
            res->content.value = hdltmp->id;
        }
        delete [] po.points;
    }
    return (OK);
}


// (int) CheckObjectsConnected(object_handle)
//
// This function returns 1 unless the list contains objects on the
// layer of the first object in the list that are mutually disjoint,
// meaning that there exist two objects and one can not draw a curve
// from the interior of one to the other without crossing empty area.
// If disjoint objects are found, 0 is returned.
//
bool
geom1_funcs::IFcheckObjectsConnected(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    res->type = TYP_SCALAR;
    res->content.value = 1.0;

    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;

        CDl *ld = 0;
        Zlist *zl0 = 0, *ze = 0;
        while (ol) {
            if (!ld)
                ld = ol->odesc->ldesc();
            if (ld == ol->odesc->ldesc()) {
                Zlist *z = ol->odesc->toZlist();
                if (!zl0)
                    zl0 = ze = z;
                else {
                    while (ze->next)
                        ze = ze->next;
                    ze->next = z;
                }
            }
            ol = ol->next;
        }
        if (zl0 && zl0->next) {
            Ylist *yl = new Ylist(zl0);
            try {
                yl = new Ylist(yl->repartition());
            }
            catch (XIrt ret) {
                if (ret == XIintr) {
                    SI()->SetInterrupt();
                    return (OK);
                }
                else
                    return (BAD);
            }
            yl = yl->connected(&zl0);
            Zlist::free(zl0);
            if (yl) {
                res->content.value = 0;
                yl->free();
            }
        }
    }
    return (OK);
}


// (int) CheckForHoles(object_handle, all)
//
// This function returns 1 if the object, or collection of objects,
// has "holes", i.e., uncovered areas completely surrounded by
// geometry.  The first argument is a handle to a list of objects.  If
// the second argument is nonzero, the geompetry represented by all
// objects in the list is checked.  If zero, only the first object
// (which might be a complex polygon containing holes) is checked.  If
// no holes are found, 0 is returned.
//
// When all is true, only objects on the same layer as the first
// object in the list are considered.
//
bool
geom1_funcs::IFcheckForHoles(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))

    res->type = TYP_SCALAR;
    res->content.value = 0;

    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;

        CDl *ld = 0;
        Zlist *zl0 = 0, *ze = 0;
        while (ol) {
            if (!ld)
                ld = ol->odesc->ldesc();
            if (ld == ol->odesc->ldesc()) {
                Zlist *z = ol->odesc->toZlist();
                if (!zl0)
                    zl0 = ze = z;
                else {
                    while (ze->next)
                        ze = ze->next;
                    ze->next = z;
                }
            }
            if (!all)
                break;
            ol = ol->next;
        }
        if (zl0 && zl0->next) {
            try {
                zl0 = Zlist::repartition(zl0);
            }
            catch (XIrt ret) {
                if (ret == XIintr) {
                    SI()->SetInterrupt();
                    return (OK);
                }
                else
                    return (BAD);
            }
            BBox BB;
            Zlist::BB(zl0, BB);
            BB.bloat(10);
            Zlist *zarea = new Zlist(&BB);
            XIrt ret = Zlist::zl_andnot(&zarea, zl0);
            if (ret == XIbad)
                return (BAD);
            else if (ret == XIintr) {
                SI()->SetInterrupt();
                return (OK);
            }
            Ylist *yl = new Ylist(zarea);
            yl = yl->connected(&zl0);
            Zlist::free(zl0);
            if (yl) {
                res->content.value = 1.0;
                yl->free();
            }
        }
    }
    return (OK);
}


// (object_handle) BloatObjects(object_handle, all, dimen, lname, mode)
//
// This function returns a handle to a list of object copies which are
// bloated versions of the objects referenced by the handle argument,
// similar to the !bloat command.  The passed handle and objects are
// not affected.  Edges will be pushed outward or pulled inward by
// dimen (positive values push outward).  The dimen is given in
// microns.
//
// The all argument is a boolean that if nonzero indicates that all
// objects in the list referenced by the handle may be processed.  If
// zero, only the first object in the list will be processed.
//
// The lname argument is a layer name.  If this argument is
// zero, or a null or empty string, all objects on the returned list
// are associated with the layer of the first object in the passed
// list, and only objects on this layer in the passed list are
// processed.  Otherwise, the layer will be created if it does not
// exist, and all new objects will be associated with this layer, and
// all objects in the passed list will be processed.
//
// The mode argument is an integer that specifies the algorithm to
// use for bloating.  Giving zero specifies the default algorithm,
// which rounds corners but is rather complex and slow.  See the
// description of the !bloat command for documentation of the
// algorithms available.
//
// The DeleteObjects() function can be called to delete the old
// objects.  The CopyObjects() function can be called on the returned
// objects to add them to the database.  This function returns a
// handle to the new list upon success, or 0 if there are no objects.
// The function will fail if the first argument is not a handle to a
// list of objects or copies, or the lname argument is non-null and
// not a vaild layer name.
//
// This function uses the JoinMax...  variables in processing.  There
// is no effect on objects in the list whose handle is passed as the
// argument, or on the handle.
//
bool
geom1_funcs::IFbloatObjects(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))
    int dimen;
    ARG_CHK(arg_coord(args, 2, &dimen, DSP()->CurMode()))
    const char *lname;
    ARG_CHK(arg_string(args, 3, &lname))
    int mode;
    ARG_CHK(arg_int(args, 4, &mode))

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    if (cursd->isElectrical()) {
        Errs()->add_error(phys_msg, "BloatObjects");
        return (BAD);
    }
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDl *ldset = 0;
    if (lname && *lname) {
        ldset = CDldb()->newLayer(lname, DSP()->CurMode());
        if (!ldset)
            return (BAD);
    }
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;

        CDl *ldfirst = 0;
        Zlist *zl0 = 0, *ze = 0;
        while (ol) {
            if (!ldfirst)
                ldfirst = ol->odesc->ldesc();
            if (ldset || ol->odesc->ldesc() == ldfirst) {
                Zlist *z = ol->odesc->toZlist();
                if (!zl0)
                    zl0 = ze = z;
                else {
                    while (ze->next)
                        ze = ze->next;
                    ze->next = z;
                }
            }
            if (!all)
                break;
            ol = ol->next;
        }
        if (zl0) {
            XIrt ret = Zlist::zl_bloat(&zl0, dimen, mode);
            if (ret == XIbad)
                return (BAD);
            else if (ret == XIintr) {
                SI()->SetInterrupt();
                return (OK);
            }
            PolyList *p0 = zl0->to_poly_list();
            CDol *o0 = p0->to_olist(ldset ? ldset : ldfirst);
            if (o0) {
                sHdl *hnew = new sHdlObject(o0, cursd, true);
                res->type = TYP_HANDLE;
                res->content.value = hnew->id;
            }
        }
    }
    return (OK);
}


// (object_handle) EdgeObjects(object_handle, all, dimen, lname, mode)
//
// This function creates new polygon copies that cover the edges of
// the figures in the passed handle.  The dimen is half the effective
// path width of the generated wire-like shapes that cover the edges.
//
// If the boolean argument all is nonzero, all of the objects in the
// passed list may be processed, otherwise only the object at the head
// of the list will be processed.
//
// The lname argument is a layer name.  If this argument is
// zero, or a null or empty string, all objects on the returned list
// are associated with the layer of the first object in the passed
// list, and only objects on this layer in the passed list are
// processed.  Otherwise, the layer will be created if it does not
// exist, and all new objects will be associated with this layer, and
// all objects in the passed list will be processed.
//
// The mode is an integer which specifies the algorithm to use.  The
// algorithms are described with the EdgesZ function.
//
// The DeleteObjects() function can be called to delete the old
// objects.  The CopyObjects() function can be called on the returned
// objects to add them to the database.  This function returns a
// handle to the new list upon success, or 0 if there are no objects. 
// The function will fail if the first argument is not a handle to a
// list of objects or copies, or the lname argument is non-null and
// not a vaild layer name.
//
bool
geom1_funcs::IFedgeObjects(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))
    int dimen;
    ARG_CHK(arg_coord(args, 2, &dimen, DSP()->CurMode()))
    const char *lname;
    ARG_CHK(arg_string(args, 3, &lname))
    int mode;
    ARG_CHK(arg_int(args, 4, &mode))

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    if (cursd->isElectrical()) {
        Errs()->add_error(phys_msg, "EdgeObjects");
        return (BAD);
    }
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDl *ldset = 0;
    if (lname && *lname) {
        ldset = CDldb()->newLayer(lname, DSP()->CurMode());
        if (!ldset)
            return (BAD);
    }
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;

        CDl *ldfirst = 0;
        Zlist *zl0 = 0, *ze = 0;
        while (ol) {
            if (!ldfirst)
                ldfirst = ol->odesc->ldesc();
            if (ldset || ol->odesc->ldesc() == ldfirst) {
                Zlist *z = ol->odesc->toZlist();
                if (!zl0)
                    zl0 = ze = z;
                else {
                    while (ze->next)
                        ze = ze->next;
                    ze->next = z;
                }
            }
            if (!all)
                break;
            ol = ol->next;
        }
        if (zl0) {
            Zlist *zret = 0;
            try {
                if (mode >= 0 && mode <= 3)
                    zret = Zlist::bloat(zl0, dimen,
                        BL_EDGE_ONLY | (mode << BL_CORNER_MODE_SHIFT));
                if (mode == 4)
                    zret = Zlist::halo(zl0, dimen);
                else if (mode == 5)
                    zret = Zlist::wire_edges(zl0, dimen);
                else if (mode == 6)
                    zret = Zlist::edges(zl0, dimen);
                else
                    zret = Zlist::bloat(zl0, dimen, BL_EDGE_ONLY);
                Zlist::free(zl0);
                zl0 = zret;
            }
            catch (XIrt tmpret) {
                zret = 0;
                Zlist::free(zl0);
                zl0 = 0;
                if (tmpret == XIintr)
                    SI()->SetInterrupt();
                else
                    return (BAD);
            }
        }
        if (zl0) {
            PolyList *p0 = zl0->to_poly_list();
            CDol *o0 = p0->to_olist(ldset ? ldset : ldfirst);
            if (o0) {
                sHdl *hnew = new sHdlObject(o0, cursd, true);
                res->type = TYP_HANDLE;
                res->content.value = hnew->id;
            }
        }

    }
    return (OK);
}


// (object_handle) ManhattanizeObjects(object_handle, all, dimen, lname, mode)
//
// This function will convert the objects pointed to by the handle
// argument into a list of copies, which is referenced by the returned
// handle.  The supplied objects and handle are not affected.  Each
// new object is a Manhattan approximation of the original object. 
// The dimen argument is the minimum height or width in microns of
// rectangles created to approximate the non-Manhattan parts.
//
// The all argument is a boolean that if nonzero indicates that all
// objects in the list referenced by the handle may be processed.  If
// zero, only the first object in the list will be processed.
//
// The lname argument is a layer name, or zero.  If a layer name is
// given, the new objects will be associated with that layer, which
// will be created if it does not exist.  If 0 or an empty string is
// passed, the new objects will be associatd with the layer of the
// original object.
//
// The mode argument is a boolean value which selects one of two
// Manhattanizing algorithms to employ.  These algorithms are
// described with the !manh command.
//
// The function will fail if the first argument is not a handle to a
// list of objects or copies, or the lname argument is non-null and
// not a vaild layer name, or the dimen argument is smaller than 0.01. 
// On success, a handle to the list of copies is returned.  Each
// object in the returned list is a box or Manhattan polygon which
// approximates one of the original objects.  Of course, if an
// original object is Manhattan, the shape will be unchanged, though
// the coordinates will be moved to a dimen grid if the gridding mode
// (mode nonzero) is given.
//
// The DeleteObjects() function can be called to delete the old
// objects.  The CopyObjects() function can be called on the returned
// objects to add them to the database.
//
// This function uses the JoinMax...  variables in processing.  There
// is no effect on objects in the list whose handle is passed as the
// argument, or on the handle.
//
bool
geom1_funcs::IFmanhattanizeObjects(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))
    int bsize;
    ARG_CHK(arg_coord(args, 2, &bsize, DSP()->CurMode()))
    const char *lname;
    ARG_CHK(arg_string(args, 3, &lname))
    int mode;
    ARG_CHK(arg_int(args, 4, &mode))

    if (bsize < 10)
        return (BAD);
    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    if (cursd->isElectrical()) {
        Errs()->add_error(phys_msg, "ManhattanizeObjects");
        return (BAD);
    }
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDl *ldset = 0;
    if (lname && *lname) {
        ldset = CDldb()->newLayer(lname, DSP()->CurMode());
        if (!ldset)
            return (BAD);
    }

    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;

        CDol *o0 = 0, *oe = 0;
        while (ol) {
            CDl *ld = ldset ? ldset : ol->odesc->ldesc();
            Zlist *z = ol->odesc->toZlist();
            z = Zlist::manhattanize(z, bsize, mode);
            PolyList *p0 = z->to_poly_list();
            if (!o0)
                o0 = p0->to_olist(ld, &oe);
            else
                p0->to_olist(ld, &oe);
            if (!all)
                break;
            ol = ol->next;
        }
        if (o0) {
            sHdl *hnew = new sHdlObject(o0, cursd, true);
            res->type = TYP_HANDLE;
            res->content.value = hnew->id;
        }
    }
    return (OK);
}


// (int) GroupObjects(object_handle, array)
//
// This function acts on the first object in the list and all other
// objects on the same layer found in the list.  The objects are
// copied, then sorted into groups, so that each group forms a single
// figure, i.e., no two members of the same group are disjoint.  The
// groups are then joined into polygons, and a handle to each group is
// returned in the array.  The array will be resized if necessary.
// The returned value is the number of groups, corresponding to the
// used entries in the array.  The H() function should be used on the
// array elements to convert the values to an object handle data type,
// similar to the treatment of the array returned from the
// HandleArray() function.  The CloseArray() function can be used to
// close the handles.  The created objects are copies, so are not
// added to the database.
//
// This function uses the JoinMax...  variables in processing.  There
// is no effect on objects in the list whose handle is passed as the
// first argument, or on the handle.  The value 0 is returned on error
// or if the list is empty.
//
bool
geom1_funcs::IFgroupObjects(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    if (args[1].type != TYP_ARRAY)
        return (BAD);

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    if (cursd->isElectrical()) {
        Errs()->add_error(phys_msg, "GroupObjects");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    res->content.value = 1.0;

    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;

        CDl *ld = 0;
        Zlist *zl0 = 0, *ze = 0;
        while (ol) {
            if (!ld)
                ld = ol->odesc->ldesc();
            if (ld == ol->odesc->ldesc()) {
                Zlist *z = ol->odesc->toZlist();
                if (!zl0)
                    zl0 = ze = z;
                else {
                    while (ze->next)
                        ze = ze->next;
                    ze->next = z;
                }
            }
            ol = ol->next;
        }
        if (zl0) {
            Zgroup *g = zl0->group(Zlist::JoinMaxGroup);
            if (ADATA(args[1].content.a)->resize(g->num) == BAD) {
                delete g;
                return (OK);
            }
            res->content.value = g->num;
            for (int i = 0; i < g->num; i++) {
                PolyList *p0 = g->to_poly_list(i, Zlist::JoinMaxVerts);
                CDol *o0 = p0->to_olist(ld);
                if (o0) {
                    sHdl *hnew = new sHdlObject(o0, cursd, true);
                    args[1].content.a->values()[i] = hnew->id;
                }
            }
            delete g;
        }
    }
    return (OK);
}


// (object_handle) JoinObjects(object_handle, lname)
//
// This function will combine the objects in the list passed as the
// first argument, if possible, into a new list of object copies,
// which is returned.  The passed handle and objects are not affected.
// All objects in the returned list will be associated with the layer
// named in the second argument.  This layer will be created if it
// does not exist, and the output will consist of the joined outlines
// of all of the objects in the passed list, from any layer.  If 0, or
// a null or empty string is passed, the new objects will be
// associated with the layer of the first object in the passed list,
// and only the outlines of objects on this layer found in the passed
// list will contribute to the result.
//
// The DeleteObjects() function can be called to delete the old
// objects.  The CopyObjects() function can be called on the returned
// objects to add them to the database.  This function returns a
// handle to the new list upon success, or 0 if there are no objects.
// The function will fail if the first argument is not a handle to a
// list of objects or copies, or the lname argument is non-null and
// not a vaild layer name.
//
// This function uses the JoinMax...  variables in processing.  There
// is no effect on objects in the list whose handle is passed as the
// argument, or on the handle.
//
bool
geom1_funcs::IFjoinObjects(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *lname;
    ARG_CHK(arg_string(args, 1, &lname))

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    if (cursd->isElectrical()) {
        Errs()->add_error(phys_msg, "JoinObjects");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    CDl *ldset = 0;
    if (lname && *lname) {
        ldset = CDldb()->newLayer(lname, DSP()->CurMode());
        if (!ldset)
            return (BAD);
    }
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;

        CDl *ldfirst = 0;
        PolyList *p0 = 0, *pe = 0;
        Zlist *zl0 = 0, *ze = 0;
        int zcnt = 0;
        while (ol) {
            if (!ldfirst)
                ldfirst = ol->odesc->ldesc();
            if (ldset || ol->odesc->ldesc() == ldfirst) {
                Zlist *z = ol->odesc->toZlist();
                int n = Zlist::length(z);
                if (Zlist::JoinMaxQueue <= 0 || zcnt + n <
                        Zlist::JoinMaxQueue) {
                    if (!zl0)
                        zl0 = ze = z;
                    else {
                        while (ze->next)
                            ze = ze->next;
                        ze->next = z;
                    }
                    zcnt += n;
                    ol = ol->next;
                    continue;
                }
                if (!p0)
                    p0 = pe = zl0->to_poly_list();
                else {
                    while (pe->next)
                        pe = pe->next;
                    pe->next = zl0->to_poly_list();
                }
                zl0 = ze = z;
                zcnt = n;
            }
            ol = ol->next;
        }
        if (zcnt) {
            if (!p0)
                p0 = pe = zl0->to_poly_list();
            else {
                while (pe->next)
                    pe = pe->next;
                pe->next = zl0->to_poly_list();
            }
        }
        CDol *o0 = p0->to_olist(ldset ? ldset : ldfirst);
        if (o0) {
            sHdl *hnew = new sHdlObject(o0, cursd, true);
            res->type = TYP_HANDLE;
            res->content.value = hnew->id;
        }
    }
    return (OK);
}


// (object_handle) SplitObjects(object_handle, all, lname, vert)
//
// This function will split the objects in the list passed as the
// first argument into horizontal or vertical trapezoids (polygons or
// boxes) and return a list of the new objects.  The new objects are
// "object copies" and are not added to the database.
//
// If the boolean argument all is nonzero, all of the objects in the
// list referenced by the handle will be processed.  Otherwise, only
// the first object will be processed.
//
// The new objects are placed on the layer with the name given in
// lname, which is created if it does not exist, independent of the
// originating layer of the objects.  If a null string or 0 is passed
// for lname, the target layer will be the layer of the first object
// found in the object list.
//
// The vert argument is a boolean which if nonzero indicates a
// vertical decomposition, otherwise a horizontal decomposition is
// produced.
//
// The handle and objects passed are untouched.  The DeleteObjects()
// function can be called to delete the old objects.  The CopyObjects()
// function can be called on the returned objects to add them to the
// database.  This function returns a handle to the new list upon
// success, or 0 if there are no objects.  The function will fail if
// the first argument is not a handle to a list of objects or copies,
// or the lname argument is non-null and not a vaild layer name.
//
bool
geom1_funcs::IFsplitObjects(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))
    const char *lname;
    ARG_CHK(arg_string(args, 2, &lname))
    bool vert;
    ARG_CHK(arg_boolean(args, 3, &vert))

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    if (cursd->isElectrical()) {
        Errs()->add_error(phys_msg, "SplitObjects");
        return (BAD);
    }
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDl *ld = 0;
    if (lname && *lname) {
        ld = CDldb()->newLayer(lname, DSP()->CurMode());
        if (!ld)
            return (BAD);
    }
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;

        Zlist *zl0 = 0, *ze = 0;
        while (ol) {
            if (!ld)
                ld = ol->odesc->ldesc();
            Zlist *z = vert ? ol->odesc->toZlistR() : ol->odesc->toZlist();
            if (!zl0)
                zl0 = ze = z;
            else {
                while (ze->next)
                    ze = ze->next;
                ze->next = z;
            }
            if (!all)
                break;
            ol = ol->next;
        }
        CDol *o0 = 0;
        for (Zlist *z = zl0; z; z = ze) {
            ze = z->next;
            CDo *cdo = 0;
            if (z->Z.is_rect()) {
                if (vert) {
                    BBox BB(z->Z.yl, -z->Z.xur, z->Z.yu, -z->Z.xll);
                    if (BB.valid())
                        cdo = new CDo(ld, &BB);
                }
                else {
                    BBox BB(z->Z.xll, z->Z.yl, z->Z.xur, z->Z.yu);
                    if (BB.valid())
                        cdo = new CDo(ld, &BB);
                }
            }
            else {
                Poly po;
                if (z->Z.mkpoly(&po.points, &po.numpts, vert))
                    cdo = new CDpo(ld, &po);
            }
            if (cdo) {
                cdo->set_copy(true);
                o0 = new CDol(cdo, o0);
            }
            delete z;
        }
        if (o0) {
            sHdl *hnew = new sHdlObject(o0, cursd, true);
            res->type = TYP_HANDLE;
            res->content.value = hnew->id;
        }
    }
    return (OK);
}


// (int) DeleteObjects(object_handle, all)
//
// Calling this function will delete referenced objects from the
// current cell.  If the boolean argument all is nonzero, all objects
// in the list will be deleted.  Otherwise, only the first object in
// the list will be deleted.  Once deleted, the objects are no longer
// referenced by the handle, which may become empty as a result.
//
// This function will fail if the handle passed is not a handle to an
// object list.  The number of objects deleted is returned.
//
bool
geom1_funcs::IFdeleteObjects(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    int cnt = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        if (((sHdlObject*)hdl)->copies)
            return (OK);
        if (((sHdlObject*)hdl)->sdesc->isImmutable())
            return (OK);
        CDol *ol = (CDol*)hdl->data;
        while (ol) {
            CDol *onxt = ol->next;
            if (((sHdlObject*)hdl)->sdesc == CurCell()) {
                Ulist()->RecordObjectChange(((sHdlObject*)hdl)->sdesc,
                    ol->odesc, 0);
                cnt++;
            }
            if (!all)
                break;
            ol = onxt;
        }
    }
    res->content.value = cnt;
    return (OK);
}


// (int) SelectObjects(object_handle, all)
//
// This function will select objects referenced by the handle.  If the
// boolean argument all is nonzero, all objects in the list will be
// selected.  Otherwise, only the first object in the list will be
// selected.
//
// It is not possible to select object copies, 0 is returned if the
// passed handle represents copies.  Otherwise the return value is the
// number of newly selected objects.
//
// This function will fail if the handle passed is not a handle to an
// object list.
//
bool
geom1_funcs::IFselectObjects(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))

    int cnt = 0;
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        if (((sHdlObject*)hdl)->copies)
            return (OK);
        CDs *cursd = CurCell();
        CDol *ol = (CDol*)hdl->data;
        while (ol) {
            if (((sHdlObject*)hdl)->sdesc == CurCell() &&
                    ol->odesc->state() == CDVanilla) {
                Selections.insertObject(cursd, ol->odesc);
                cnt++;
            }
            if (!all)
                break;
            ol = ol->next;
        }
    }
    res->content.value = cnt;
    return (OK);
}


// (int) DeselectObjects(object_handle, all)
//
// This function will deselect objects referenced by the handle.  If
// the boolean argument all is nonzero, all objects in the list will
// be deselected.  Otherwise, only the first object in the list will
// be deselected.
//
// It is not possible to select object copies, 0 is returned if the
// passed handle represents copies.  Otherwise the return value is the
// number of newly deselected objects.
//
// This function will fail if the handle passed is not a handle to an
// object list.
//
bool
geom1_funcs::IFdeselectObjects(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))

    int cnt = 0;
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        if (((sHdlObject*)hdl)->copies)
            return (OK);
        CDs *cursd = CurCell();
        if (!cursd)
            return (OK);
        CDol *ol = (CDol*)hdl->data;
        while (ol) {
            if (((sHdlObject*)hdl)->sdesc == CurCell() &&
                    ol->odesc->state() == CDSelected) {
                Selections.removeObject(cursd, ol->odesc);
                cnt++;
            }
            if (!all)
                break;
            ol = ol->next;
        }
    }
    res->content.value = cnt;
    return (OK);
}


namespace {
    // Function to transform an object desc for move/copy functions. 
    // In move mode, the odpnew returns a new transformed object.  In
    // copy mode, the transformed object is added to the database, and
    // odpnew returns a pointer to the new object.  False is returned
    // on error.
    //
    // In copy mode, if odpnew is null, merging will apply to the new
    // object.  If odpnew is passed, merging will be skipped here, the
    // caller can merge later if needed.
    //
    bool
    translate_copy(const CDo *od, CDo **odpnew, int ref_x, int ref_y,
        int x, int y, CDl *ldold, CDl *ldnew, CDmcType  mc, const char *hdr)
    {
        if (odpnew)
            *odpnew = 0;
        CDs *cursd = CurCell();
        if (!cursd)
            return (false);
        cTfmStack stk;
        stk.TPush();
        GEO()->applyCurTransform(&stk, ref_x, ref_y, x, y);

        CDl *ld = od->ldesc();
        if (ldnew && (!ldold || ldold == ld))
            ld = ldnew;
        if (od->type() == CDBOX) {
            BBox BB = od->oBB();
            Poly poly;
            stk.TBB(&BB, &poly.points);
            stk.TPop();
            if (poly.points) {
                poly.numpts = 5;
                if (GEO()->curTx()->magset())
                    Point::scale(poly.points, 5, GEO()->curTx()->magn(), x, y);
                if (mc == CDcopy) {
                    CDpo *newo;
                    if (cursd->makePolygon(ld, &poly, &newo) != CDok) {
                        Errs()->add_error("makePolygon failed");
                        Log()->ErrorLog(hdr, Errs()->get_error());
                        return (false);
                    }
                    Ulist()->RecordObjectChange(cursd, 0, newo);
                    if (odpnew)
                        *odpnew = newo;
                    else {
                        if (!cursd->mergeBoxOrPoly(newo, true)) {
                            Errs()->add_error("mergeBoxOrPoly failed");
                            Log()->ErrorLog(hdr, Errs()->get_error());
                        }
                    }
                }
                else if (odpnew) {
                    *odpnew = new CDpo(ld, &poly);
                    (*odpnew)->set_copy(true);
                }
            }
            else {
                if (GEO()->curTx()->magset())
                    BB.scale(GEO()->curTx()->magn(), x, y);
                if (mc == CDcopy) {
                    CDo *newo;
                    if (cursd->makeBox(ld, &BB, &newo) != CDok) {
                        Errs()->add_error("makeBox failed");
                        Log()->ErrorLog(hdr, Errs()->get_error());
                        return (false);
                    }
                    Ulist()->RecordObjectChange(cursd, 0, newo);
                    if (odpnew)
                        *odpnew = newo;
                    else {
                        if (!cursd->mergeBoxOrPoly(newo, true)) {
                            Errs()->add_error("mergeBoxOrPoly failed");
                            Log()->ErrorLog(hdr, Errs()->get_error());
                        }
                    }
                }
                else if (odpnew) {
                    *odpnew = new CDo(ld, &BB);
                    (*odpnew)->set_copy(true);
                }
            }
        }
        else if (od->type() == CDPOLYGON) {
            int num = ((const CDpo*)od)->numpts();
            Poly poly(num, Point::dup_with_xform(((const CDpo*)od)->points(),
                &stk, num));
            stk.TPop();
            BBox BB;
            if (poly.to_box(&BB)) {
                delete [] poly.points;
                // rectangular, convert to box
                if (GEO()->curTx()->magset())
                    BB.scale(GEO()->curTx()->magn(), x, y);
                if (mc == CDcopy) {
                    CDo *newo;
                    if (cursd->makeBox(ld, &BB, &newo) != CDok) {
                        Errs()->add_error("makeBox failed");
                        Log()->ErrorLog(hdr, Errs()->get_error());
                        return (false);
                    }
                    Ulist()->RecordObjectChange(cursd, 0, newo);
                    if (odpnew)
                        *odpnew = newo;
                    else {
                        if (!cursd->mergeBoxOrPoly(newo, true)) {
                            Errs()->add_error("mergeBoxOrPoly failed");
                            Log()->ErrorLog(hdr, Errs()->get_error());
                        }
                    }
                }
                else if (odpnew) {
                    *odpnew = new CDo(ld, &BB);
                    (*odpnew)->set_copy(true);
                }
            }
            else {
                if (GEO()->curTx()->magset()) {
                    Point::scale(poly.points, poly.numpts,
                        GEO()->curTx()->magn(), x, y);
                }
                if (mc == CDcopy) {
                    CDpo *newo;
                    if (cursd->makePolygon(ld, &poly, &newo) != CDok) {
                        Errs()->add_error("makePolygon failed");
                        Log()->ErrorLog(hdr, Errs()->get_error());
                        return (false);
                    }
                    Ulist()->RecordObjectChange(cursd, 0, newo);
                    if (odpnew)
                        *odpnew = newo;
                    else {
                        if (!cursd->mergeBoxOrPoly(newo, true)) {
                            Errs()->add_error("mergeBoxOrPoly failed");
                            Log()->ErrorLog(hdr, Errs()->get_error());
                        }
                    }
                }
                else if (odpnew) {
                    *odpnew = new CDpo(ld, &poly);
                    (*odpnew)->set_copy(true);
                }
            }
        }
        else if (od->type() == CDWIRE) {
            Wire wire(((const CDw*)od)->numpts(), 0,
                ((const CDw*)od)->attributes());
            wire.points = Point::dup_with_xform(((const CDw*)od)->points(),
                &stk, wire.numpts);
            stk.TPop();
            if (GEO()->curTx()->magset()) {
                Point::scale(wire.points, wire.numpts, GEO()->curTx()->magn(),
                    x, y);
                if (!ED()->noWireWidthMag())
                    wire.set_wire_width(
                        mmRnd(wire.wire_width()*GEO()->curTx()->magn()));
            }
            if (mc == CDcopy) {
                CDw *newo;
                if (cursd->makeWire(ld, &wire, &newo) != CDok) {
                    Errs()->add_error("makeWire failed");
                    Log()->ErrorLog(hdr, Errs()->get_error());
                    return (false);
                }
                Ulist()->RecordObjectChange(cursd, 0, newo);
                if (odpnew)
                    *odpnew = newo;
                else
                    cursd->mergeWire(newo, true);
            }
            else if (odpnew) {
                *odpnew = new CDw(ld, &wire);
                (*odpnew)->set_copy(true);
            }
        }
        else
            stk.TPop();
        return (true);
    }
}


// (int) MoveObjects(object_handle, all, refx, refy, x, y)
//
// This function is similar to the Move() function, however it
// operates on the object(s) referenced by the handle.  The object is
// moved such that the coordinate refx, refy is translated to x, y.
// The current transform will be applied to the move.  If all is
// nonzero, all objects in the list are moved, otherwise only the
// object currently referenced is moved.  The function returns the
// number of objects moved.  This function will fail if the handle
// passed is not a handle to an object list.
//
// If the handle references object copies, each copy is translated and
// possibly transformed as described above.  The handle will
// subsequently reference the modified object.
//
bool
geom1_funcs::IFmoveObjects(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))
    int ref_x;
    ARG_CHK(arg_coord(args, 2, &ref_x, DSP()->CurMode()))
    int ref_y;
    ARG_CHK(arg_coord(args, 3, &ref_y, DSP()->CurMode()))
    int x;
    ARG_CHK(arg_coord(args, 4, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 5, &y, DSP()->CurMode()))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDs *cursd = CurCell();
        if (!cursd || cursd->isImmutable())
            return (OK);
        CDol *ol = (CDol*)hdl->data;
        if (ol && ((sHdlObject*)hdl)->sdesc == CurCell()) {
            int cnt = 0;

            if (((sHdlObject*)hdl)->copies) {
                do {
                    CDo *od;
                    if (translate_copy(ol->odesc, &od, ref_x, ref_y, x, y,
                            0, 0, CDmove, "object creation, in MoveObjects")) {
                        delete ol->odesc;
                        ol->odesc = od;
                        cnt++;
                    }
                    ol = ol->next;
                } while (ol && all);
            }
            else {
                CDol *oltmp = 0;
                if (!all) {
                    oltmp = ol->next;
                    ol->next = 0;
                }
                selqueue_t sq;
                Selections.getQueue(cursd, &sq);
                Selections.insertList(cursd, ol);
                cnt += ED()->moveQueue(ref_x, ref_y, x, y, 0, 0);
                Selections.setQueue(&sq);
                if (!all)
                    ol->next = oltmp;
            }
            res->content.value = cnt;
        }
    }
    return (OK);
}


namespace {
    // Layer argument, if not null must match a layer name.
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


// (int) MoveObjectsToLayer(object_handle, all, refx, refy, x, y,
//     oldlayer, newlayer)
//
// This is similar to the MoveObjects() function, but allows layer
// change.  If newlayer is 0, null, or empty, oldlayer is ignored and
// the function behaves identically to MoveObjects().  Otherwise the
// newlayer string must be a layer name.  If oldlayer is 0, null, or
// empty, all moved objects are placed on newlayer.  Otherwise,
// oldlayer must be a layer name, in which case only objects on
// oldlayer will be placed on newlayer, other objects will remain on
// the same layer.  Subcell objects are moved as in MoveObjects(),
// i.e., the layer arguments are ignored.
//
bool
geom1_funcs::IFmoveObjectsToLayer(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))
    int ref_x;
    ARG_CHK(arg_coord(args, 2, &ref_x, DSP()->CurMode()))
    int ref_y;
    ARG_CHK(arg_coord(args, 3, &ref_y, DSP()->CurMode()))
    int x;
    ARG_CHK(arg_coord(args, 4, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 5, &y, DSP()->CurMode()))
    CDl *ldold;
    ARG_CHK(arg_layer_zok(args, 6, &ldold))
    CDl *ldnew;
    ARG_CHK(arg_layer_zok(args, 7, &ldnew))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDs *cursd = CurCell();
        if (!cursd || cursd->isImmutable())
            return (OK);
        CDol *ol = (CDol*)hdl->data;
        if (ol && ((sHdlObject*)hdl)->sdesc == cursd) {
            int cnt = 0;

            if (((sHdlObject*)hdl)->copies) {
                do {
                    CDo *od;
                    if (translate_copy(ol->odesc, &od, ref_x, ref_y, x, y,
                            ldold, ldnew, CDmove,
                            "object creation, in MoveObjectsToLayer")) {
                        delete ol->odesc;
                        ol->odesc = od;
                        cnt++;
                    }
                    ol = ol->next;
                } while (ol && all);
            }
            else {
                CDol *oltmp = 0;
                if (!all) {
                    oltmp = ol->next;
                    ol->next = 0;
                }
                selqueue_t sq;
                Selections.getQueue(cursd, &sq);
                Selections.insertList(cursd, ol);
                cnt += ED()->moveQueue(ref_x, ref_y, x, y, ldold, ldnew);
                Selections.setQueue(&sq);
                if (!all)
                    ol->next = oltmp;
            }
            res->content.value = cnt;
        }
    }
    return (OK);
}


// (int) CopyObjects(object_handle, all, refx, refy, x, y, repcnt)
//
// This function is similar to the Copy() function, however it
// operates on the object(s) referenced by the handle.  The object is
// copied such that the coordinate refx, refy is translated to x, y.
//
// The repcnt is an integer replication count in the range 1-100000,
// which will be silently taken as 1 if out of range.  If not one,
// multiple copies are made, at mutiples of the translation factors
// given.
//
// The current transform will be applied to the copy.  If all is
// nonzero, all of the objects in the list are copied, otherwise only
// the object currently being referenced is copied.  The function
// returns the number of objects copied.  This function will fail if
// the handle passed is not a handle to an object list.
//
// If the handle references object copies, the object copies that are
// referenced remain untouched, however the new objects, translated
// and possibly transformed as described above, are added to the
// database.  The repcnt argument is ignored in this case.
//
bool
geom1_funcs::IFcopyObjects(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))
    int ref_x;
    ARG_CHK(arg_coord(args, 2, &ref_x, DSP()->CurMode()))
    int ref_y;
    ARG_CHK(arg_coord(args, 3, &ref_y, DSP()->CurMode()))
    int x;
    ARG_CHK(arg_coord(args, 4, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 5, &y, DSP()->CurMode()))
    int rep;
    ARG_CHK(arg_int(args, 6, &rep))
    if (rep < 1 || rep > 100000)
        rep = 1;

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDs *cursd = CurCell();
        if (!cursd || cursd->isImmutable())
            return (OK);
        CDol *ol = (CDol*)hdl->data;
        if (ol && (((sHdlObject*)hdl)->copies ||
                ((sHdlObject*)hdl)->sdesc == cursd)) {
            int cnt = 0;

            if (((sHdlObject*)hdl)->copies) {
                do {
                    cnt += translate_copy(ol->odesc, 0, ref_x, ref_y, x, y,
                        0, 0, CDcopy, "object creation, in CopyObjects");
                    ol = ol->next;
                } while (ol && all);
            }
            else {
                CDol *oltmp = 0;
                if (!all) {
                    oltmp = ol->next;
                    ol->next = 0;
                }
                selqueue_t sq;
                Selections.getQueue(cursd, &sq);
                Selections.insertList(cursd, ol);
                cnt += ED()->replicateQueue(ref_x, ref_y, x, y, rep, 0, 0);
                Selections.setQueue(&sq);
                if (!all)
                    ol->next = oltmp;
            }
            res->content.value = cnt;
        }
    }
    return (OK);
}


// (int) CopyObjectsToLayer(object_handle, all, refx, refy, x, y,
//     oldlayer, newlayer, repcnt)
//
// This is similar to the CopyObjects() function, but allows layer
// change.  If newlayer is 0, null, or empty, oldlayer is ignored and
// the function behaves identically to CopyObjects().  Otherwise the
// newlayer string must be a layer name.  If oldlayer is 0, null, or
// empty, all copied objects are placed on newlayer.  Otherwise,
// oldlayer must be a layer name, in which case only objects on
// oldlayer will be placed on newlayer, other objects will remain on
// the same layer.  Subcell objects are copied as in CopyObjects(),
// i.e., the layer arguments are ignored.
//
bool
geom1_funcs::IFcopyObjectsToLayer(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))
    int ref_x;
    ARG_CHK(arg_coord(args, 2, &ref_x, DSP()->CurMode()))
    int ref_y;
    ARG_CHK(arg_coord(args, 3, &ref_y, DSP()->CurMode()))
    int x;
    ARG_CHK(arg_coord(args, 4, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 5, &y, DSP()->CurMode()))
    CDl *ldold;
    ARG_CHK(arg_layer_zok(args, 6, &ldold))
    CDl *ldnew;
    ARG_CHK(arg_layer_zok(args, 7, &ldnew))
    int rep;
    ARG_CHK(arg_int(args, 8, &rep))
    if (rep < 1 || rep > 100000)
        rep = 1;

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDs *cursd = CurCell();
        if (!cursd || cursd->isImmutable())
            return (OK);
        CDol *ol = (CDol*)hdl->data;
        if (ol && (((sHdlObject*)hdl)->copies ||
                ((sHdlObject*)hdl)->sdesc == cursd)) {
            int cnt = 0;

            if (((sHdlObject*)hdl)->copies) {
                do {
                    cnt += translate_copy(ol->odesc, 0, ref_x, ref_y, x, y,
                        ldold, ldnew, CDcopy,
                        "object creation, in CopyObjectsToLayer");
                    ol = ol->next;
                } while (ol && all);
            }
            else {
                CDol *oltmp = 0;
                if (!all) {
                    oltmp = ol->next;
                    ol->next = 0;
                }
                selqueue_t sq;
                Selections.getQueue(cursd, &sq);
                Selections.insertList(cursd, ol);
                cnt += ED()->replicateQueue(ref_x, ref_y, x, y, rep,
                    ldold, ldnew);
                Selections.setQueue(&sq);
                if (!all)
                    ol->next = oltmp;
            }
            res->content.value = cnt;
        }
    }
    return (OK);
}


// (object handle) CopyObjectsH(object_handle, all, refx, refy, x, y,
//     oldlayer, newlayer, todb)
//
// This function returns an object handle, containing copies of the
// objects in the handle passed as the first argument.  If boolean all
// is set, all passed objects will be copied, otherwise only the first
// object in the list will be copied.  The next four arguments set the
// copy translation, with refx and refy in the passed object
// translated to x, y in the copy.  The current transform is also
// applied to the copy.
//
// The two layer name arguments behave as in CopyObjectToLayer.  If
// newlayer is 0, null, or empty, oldlayer is ignored and no object
// layers will change.  Otherwise the newlayer string must be a layer
// name.  If oldlayer is 0, null, or empty, all copied objects are
// placed on newlayer.  Otherwise, oldlayer must be a layer name, in
// which case only objects on oldlayer will be placed on newlayer,
// other objects will remain on the same layer.  Subcell objects are
// copied as in CopyObjects, i.e., the layer arguments are ignored.
//
// The final argument is a boolean that when true, the copies are
// added to the database, and the returned handle points to the
// database objects. If false, the returned handle contains "object
// copies" which do not appear in the database.  Note that when copies
// are added to the database, unlike other copy functions merging is
// disabled, and the replication feature is not available.
//
bool
geom1_funcs::IFcopyObjectsH(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))
    int ref_x;
    ARG_CHK(arg_coord(args, 2, &ref_x, DSP()->CurMode()))
    int ref_y;
    ARG_CHK(arg_coord(args, 3, &ref_y, DSP()->CurMode()))
    int x;
    ARG_CHK(arg_coord(args, 4, &x, DSP()->CurMode()))
    int y;
    ARG_CHK(arg_coord(args, 5, &y, DSP()->CurMode()))
    CDl *ldold;
    ARG_CHK(arg_layer_zok(args, 6, &ldold))
    CDl *ldnew;
    ARG_CHK(arg_layer_zok(args, 7, &ldnew))
    bool todb;
    ARG_CHK(arg_boolean(args, 8, &todb))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDs *cursd = CurCell();
        if (!cursd || cursd->isImmutable())
            return (OK);
        CDol *ol = (CDol*)hdl->data;
        if (ol && (((sHdlObject*)hdl)->copies ||
                ((sHdlObject*)hdl)->sdesc == cursd)) {

            CDol *ol0 = 0, *oe = 0;
            do {
                CDo *onew;
                if (translate_copy(ol->odesc, &onew, ref_x, ref_y,
                        x, y, ldold, ldnew, todb ? CDcopy : CDmove,
                        "object creation, in CopyObjectsH")) {
                    if (!ol0)
                        ol0 = oe = new CDol(onew, 0);
                    else {
                        oe->next = new CDol(onew, 0);
                        oe = oe->next;
                    }
                }
                ol = ol->next;
            } while (ol && all);
            if (ol0) {
                hdl = new sHdlObject(ol0, cursd, !todb);
                res->type = TYP_HANDLE;
                res->content.value = hdl->id;
            }
        }
    }
    return (OK);
}


// (string) GetObjectType(object_handle)
//
// This function returns a one-character string representing the type
// of object referenced by the handle argument.  If the handle is
// invalid, a null string is returned.  The types are:
//
//      'b'   boxes
//      'p'   polygons
//      'w'   wires
//      'l'   labels
//      'c'   subcells
//
// This function will fail if the handle passed is not a handle to an
// object list.
//
bool
geom1_funcs::IFgetObjectType(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            res->content.string = lstring::copy(" ");
            res->content.string[0] = ol->odesc->type();
            res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (int) GetObjectID(object_handle)
//
// This function returns a unique id number for the object.  The id is
// actually the address of the object in the process memory, so it is
// valid only for the current Xic process.  If the referenced object
// is a copy, the id returned is the address of the real object, not
// the copy.  If no object is referenced by the handle, 0 is returned.
// The function fails if the handle is not an object list type.
//
bool
geom1_funcs::IFgetObjectID(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            if (((sHdlObject*)hdl)->copies)
                res->content.value = (long)ol->odesc->const_next_odesc();
            else
                res->content.value = (long)ol->odesc;
        }
    }
    return (OK);
}


// (real) GetObjectArea(object_handle)
//
// Return the area in square microns of the object pointed to by the
// handle.  Zero is returned for a defunct handle or upon error.
//
bool
geom1_funcs::IFgetObjectArea(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol)
            res->content.value = ol->odesc->area();
    }
    return (OK);
}


// (real) GetObjectPerim(object_handle)
//
// Return the perimeter in microns of the object pointed to by the
// handle.  Zero is returned for a defunct handle or upon error.
//
bool
geom1_funcs::IFgetObjectPerim(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol)
            res->content.value = ol->odesc->perim();
    }
    return (OK);
}


// (real) GetObjectCentroid(object_handle, retval)
//
// Return the centroid coordinates in microns of the object pointed to
// by the handle.  The second argument is an array of size two or
// larger that will contain the centroid coordinates upon successful
// return.  The return value is zero for a defunct handle or upon
// error, one if success.
//
bool
geom1_funcs::IFgetObjectCentroid(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 2))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            res->content.value = 1;
            ol->odesc->centroid(vals, vals+1);
        }
    }
    return (OK);
}


// (int) GetObjectBB(object_handle, array)
//
// This function loads the left, bottom, right, and top coordinates of
// the object's bounding box (in microns) into the array passed.  This
// function will fail if the handle passed is not a handle to an
// object list, or if the size of the array is less than 4.  The
// return value is 1 if successful, 0 otherwise.
//
bool
geom1_funcs::IFgetObjectBB(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 4))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            vals[0] = MICRONS(ol->odesc->oBB().left);
            vals[1] = MICRONS(ol->odesc->oBB().bottom);
            vals[2] = MICRONS(ol->odesc->oBB().right);
            vals[3] = MICRONS(ol->odesc->oBB().top);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) SetObjectBB(object_handle, array)
//
// This function will alter the shape of the object pointed to by the
// handle such that it has the bounding box passed.  The array
// contains the left, bottom, right, and top coordinates, in microns. 
// This function will fail if the handle passed is not a handle to an
// object list, or if the size of the array is less than 4.  The
// return value is 1 if successful, 0 otherwise.  This function has
// no effect on subcells, but other types of object will be rescaled
// to the new bounding box.
//
bool
geom1_funcs::IFsetObjectBB(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 4))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() != CDINSTANCE) {
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        if (!sdesc)
            return (OK);
        char buf[256];
        if (sdesc->isElectrical()) {
            sprintf(buf, "%d,%d %d,%d",
                ELEC_INTERNAL_UNITS(vals[0]), ELEC_INTERNAL_UNITS(vals[1]),
                ELEC_INTERNAL_UNITS(vals[2]), ELEC_INTERNAL_UNITS(vals[3]));
        }
        else {
            sprintf(buf, "%d,%d %d,%d",
                INTERNAL_UNITS(vals[0]), INTERNAL_UNITS(vals[1]),
                INTERNAL_UNITS(vals[2]), INTERNAL_UNITS(vals[3]));
        }
        if (ED()->acceptPseudoProp(ol->odesc, sdesc, XprpBB, buf))
            res->content.value = 1;
    }
    return (OK);
}


// (int) GetObjectListBB(object_handle, array)
//
// This is similar to GetObjectBB, but computes the bounding box of
// all objects in the list of objects referenced by the handle.  not
// just the list head.  The function loads the left, bottom, right,
// and top coordinates of the aggregate bounding box (in microns) into
// the array passed.  This function will fail if the handle passed is
// not a handle to an object list, or if the size of the array is less
// than 4.  The return value is a count of the objects in the list.
//
bool
geom1_funcs::IFgetObjectListBB(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 4))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (!ol)
            return (OK);
        BBox BB(ol->odesc->oBB());
        int nobjs = 1;
        ol = ol->next;
        while (ol) {
            BB.add(&ol->odesc->oBB());
            ol = ol->next;
            nobjs++;
        }
        vals[0] = MICRONS(BB.left);
        vals[1] = MICRONS(BB.bottom);
        vals[2] = MICRONS(BB.right);
        vals[3] = MICRONS(BB.top);
        res->content.value = nobjs;
    }
    return (OK);
}


// (int) GetObjectXY(object_handle, array)
//
// This function will retrieve the "XY" position from the object
// pointed to by the handle into the array, which must have size 2 or
// larger.  This is a coordinate, in microns, the interpretation of
// which depends on the object type.  For boxes, that value is the
// lower-left corner of the box.  For wires and polygons, the value is
// the first vertex in the coordinate list.  For labels, the value is
// the text anchor position.  For subcells, the value is the
// instanitation point, the same as the translation in the
// instantiation transform.
//
// On success, the return value is 1, with the array values set. 
// Otherwise, 0 is returned.
//
bool
geom1_funcs::IFgetObjectXY(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 2))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol && ol->odesc) {
            if (ol->odesc->type() == CDBOX) {
                vals[0] = MICRONS(ol->odesc->oBB().left);
                vals[1] = MICRONS(ol->odesc->oBB().bottom);
                res->content.value = 1;
            }
            else if (ol->odesc->type() == CDPOLYGON) {
                vals[0] = MICRONS(OPOLY(ol->odesc)->points()->x);
                vals[1] = MICRONS(OPOLY(ol->odesc)->points()->y);
                res->content.value = 1;
            }
            else if (ol->odesc->type() == CDWIRE) {
                vals[0] = MICRONS(OWIRE(ol->odesc)->points()->x);
                vals[1] = MICRONS(OWIRE(ol->odesc)->points()->y);
                res->content.value = 1;
            }
            else if (ol->odesc->type() == CDLABEL) {
                vals[0] = MICRONS(OLABEL(ol->odesc)->xpos());
                vals[1] = MICRONS(OLABEL(ol->odesc)->ypos());
                res->content.value = 1;
            }
            else if (ol->odesc->type() == CDINSTANCE) {
                vals[0] = MICRONS(OCALL(ol->odesc)->posX());
                vals[1] = MICRONS(OCALL(ol->odesc)->posY());
                res->content.value = 1;
            }
        }
    }
    return (OK);
}


// (int) SetObjectXY(object_handle, x, y)
//
// This function will set the "XY" coordinate of the object pointed to
// by the handle, as if setting the XprpXY pseudo-property number 7215
// on the object.  This has the effect of moving the object to a new
// location.  The interpretation of the coordinate, which is supplied
// in microns, depends on the type of object.  For boxes, the
// lower-left corner will assume the new value.  For polygons and
// wires, the object will be moved so that the first vertex in the
// coordinate list will assume the new value.  For labels, the text
// will be anchored at the new value, and for subcells, the new value
// will set the translation part of the instantiation transform.
//
// A value of 1 is returned if the operation succeeds, and the object
// will be moved.  On failure, 0 is returned.
//
bool
geom1_funcs::IFsetObjectXY(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double x;
    ARG_CHK(arg_real(args, 1, &x))
    double y;
    ARG_CHK(arg_real(args, 2, &y))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc) {
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        if (!sdesc)
            return (OK);
        char buf[64];
        if (sdesc->isElectrical())
            sprintf(buf, "%d,%d",
                ELEC_INTERNAL_UNITS(x), ELEC_INTERNAL_UNITS(y));
        else
            sprintf(buf, "%d,%d", INTERNAL_UNITS(x), INTERNAL_UNITS(y));
        if (ED()->acceptPseudoProp(ol->odesc, sdesc, XprpXY, buf))
            res->content.value = 1;
    }
    return (OK);
}


// (string) GetObjectLayer(object_handle)
//
// This function returns the name of the layer on which the object
// referenced by the handle is defined.  For subcells, this layer is
// named "$$", but objects will return a layer from the layer table.
// This function will fail if the handle passed is not a handle to an
// object list.  A stale handle will return a null string.
//
bool
geom1_funcs::IFgetObjectLayer(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            res->content.string = lstring::copy(ol->odesc->ldesc()->name());
            res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (int) SetObjectLayer(handle, layername)
//
// This function will move the object to the layer named in the
// string layername.  This will have no effect on subcells.  A value
// 1 is returned if successful, 0 otherwise.  This function will fail
// if the handle passed is not a handle to an object list.
//
bool
geom1_funcs::IFsetObjectLayer(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *lname;
    ARG_CHK(arg_string(args, 1, &lname))

    if (!lname || !*lname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() != CDINSTANCE) {
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        if (ED()->acceptPseudoProp(ol->odesc, sdesc, XprpLayer, lname))
            res->content.value = 1;
    }
    return (OK);
}


// (int) GetObjectFlags(object_handle)
//
// This function returns internal flag data from the object referenced
// by the handle.  This function will fail if the handle passed is not
// a handle to an object list.  A stale handle will return 0.
//
// The following flags are defined:
// Name             Bit     Description
// MergeDeleted     0x1     Object has been deleted due to merge.
// MergeCreated     0x2     Object has been created due to merge.
// NoDRC            0x4     Skip DRC tests on this object.
// Expand           0x8     Five flags are used to keep track of cell
//                          expansion in main plus four sub-windows,
//                          in cell instances only.
// Mark1            0x100   General purpose application flag.
// Mark2            0x200   General purpose application flag.
// MarkExtG         0x400   Extraction system, in grouping phonycell.
// MarkExtE         0x800   Extraction system, in extraction phonycell.
// InQueue          0x1000  Object is in selection queue.
// NoMerge          0x4000  Object will not be merged.
// IsCopy           0x8000  Object is a copy, not in database.
//
// The bitwise logic functions such as AndBits can be used to check the
// state of the flags.  Of these, only NoDRC, Mark1, and Mark2 can be
// arbitrarily set by the user, using functions described below.
//
bool
geom1_funcs::IFgetObjectFlags(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        res->content.value = ol ? ol->odesc->flags() : 0;
    }
    return (OK);
}


// (int) SetObjectNoDrcFlag(object_handle, val)
//
// This will set the state of the NoDRC flag of the object referenced
// by the handle.  The second argument is a boolean representing the
// flag state.  This can be called on any object, but is only
// significant for boxes, polygons, and wires in the database. 
// Objects with this flag set are ignored during design rule checking.
//
// The return value is 0 or 1 representing the previous state of the
// flag, or -1 on error.

//
bool
geom1_funcs::IFsetObjectNoDrcFlag(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool val;
    ARG_CHK(arg_boolean(args, 1, &val))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            res->content.value = (ol->odesc->flags() & CDnoDRC) != 0;
            if (val)
                ol->odesc->set_flag(CDnoDRC);
            else
                ol->odesc->unset_flag(CDnoDRC);
        }
    }
    return (OK);
}


// (int) SetObjectMark1Flag(object_handle, val)
//
// This will set the state of the Mark1 flag of the object referenced
// by the handle.  The second argument is a boolean representing the
// flag state.  This can be called on any object.  The flag is unused
// by Xic, but can be set and tested by the user for any purpose.  The
// flag persists as long as the object is in memory.
//
// The return value is 0 or 1 representing the previous state of the
// flag, or -1 on error.
//
bool
geom1_funcs::IFsetObjectMark1Flag(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool val;
    ARG_CHK(arg_boolean(args, 1, &val))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            res->content.value = (ol->odesc->flags() & CDoMark1) != 0;
            if (val)
                ol->odesc->set_flag(CDoMark1);
            else
                ol->odesc->unset_flag(CDoMark1);
        }
    }
    return (OK);
}


// (int) SetObjectMark2Flag(object_handle, val)
//
// This will set the state of the Mark2 flag of the object referenced
// by the handle.  The second argument is a boolean representing the
// flag state.  This can be called on any object.  The flag is unused
// by Xic, but can be set and tested by the user for any purpose.  The
// flag persists as long as the object is in memory.
//
// The return value is 0 or 1 representing the previous state of the
// flag, or -1 on error.
//
bool
geom1_funcs::IFsetObjectMark2Flag(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool val;
    ARG_CHK(arg_boolean(args, 1, &val))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            res->content.value = (ol->odesc->flags() & CDoMark2) != 0;
            if (val)
                ol->odesc->set_flag(CDoMark2);
            else
                ol->odesc->unset_flag(CDoMark2);
        }
    }
    return (OK);
}


// (int) GetObjectState(object_handle)
//
// This function returns a status value for the object referenced by
// the handle.  The status values are:
//
//     0    normal state
//     1    object is selected
//     2    object is deleted
//     3    object is incomplete
//     4    object is internal only
//
// Only values 0 and 1 are likely to be seen.  This function will fail
// if the handle passed is not a handle to an object list.  A stale
// handle will return 0.
//
bool
geom1_funcs::IFgetObjectState(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        res->content.value = ol ? ol->odesc->state() : 0;
    }
    return (OK);
}


// (int) GetObjectGroup(object_handle)
//
// This function returns the conductor group number of the object,
// which is a non-negative integer of -1 in certain cases, and is
// assigned by the extraction system.  This is used by the extraction
// system to establish connectivity nets of boxes, polygons, and
// wires, and for subcell indexing.  If extraction is unavailable or
// not being used, then an arbitrary integer can be applied for other
// uses with the SetObjectGroup function.
//
// This function will fail if the handle passed is not a handle to an
// object list.  If no group has been assigned, or the handle is
// stale, or the object is part of the "ground" group, 0 is returned. 
// Otherwise, and assigned number will be returned.
//
bool
geom1_funcs::IFgetObjectGroup(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        res->content.value = ol ? ol->odesc->group() : DEFAULT_GROUP;
    }
    return (OK);
}


// (int) SetObjectGroup(object_handle, group_num)
//
// This function will assign the group number to the object.  All
// objects and instances may recieve a group number, which is an
// arbitrary integer.  The group number is usually assigned and used
// by the extraction system, and should not be assigned with this
// function if extraction is being used.  However, if extraction is
// unavailable or not being used, then this function allows an
// arbitrary integer to be associated with an object, which might be
// useful.  Beware that this number is zeroed if the object is
// modified, or in copies.
//
// The GetObjectGroup function can be used to obtain the group number
// of an object or cell instance.
//
// This function will fail if the handle passed is not a handle to an
// object list.  If the group number is successfully assigned, 1 is
// returned, 0 is returned otherwise.
//
bool
geom1_funcs::IFsetObjectGroup(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int gnum;
    ARG_CHK(arg_int(args, 1, &gnum))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol && ol->odesc) {
            ol->odesc->set_group(gnum);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GetObjectCoords(object_handle, array)
//
// This function will obtain the vertex list for polygons and wires,
// or the bounding box vertices of other objects, starting from the
// lower left corner and working clockwise.  If an array is passed,
// the vertex coordinates are copied into the array, and the vertex
// count is returned.  The array will contain the x, y values of the
// vertices, in microns, if successful.  The coordinates are copied
// only if the array is large enough, or can be resized.  If the array
// is a pointer to a too small array, or the array is too small but
// has other variables pointing to it, resizing is impossible and the
// copying is skipped.  In this case, the returned value is the
// negative vertex count.  If 0 is passed instead of the array, the
// (positive) vertex count is returned.  Zero is returned if there is
// an error.  This function will fail if the handle passed is not a
// handle to an object list.
//
bool
geom1_funcs::IFgetObjectCoords(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array_if(args, 1, &vals, 1))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            Point pbuf[5];
            const Point *pts = 0;
            int numpts = 0;
            if (ol->odesc->type() == CDPOLYGON) {
                pts = ((const CDpo*)ol->odesc)->points();
                numpts = ((const CDpo*)ol->odesc)->numpts();
            }
            else if (ol->odesc->type() == CDWIRE) {
                pts = ((CDw*)ol->odesc)->points();
                numpts = ((CDw*)ol->odesc)->numpts();
            }
            else {
                ol->odesc->oBB().to_path(pbuf);
                pts = pbuf;
                numpts = 5;
            }
            res->content.value = numpts;
            if (vals && pts && numpts) {
                if (ADATA(args[1].content.a)->resize(numpts*2) == BAD) {
                    // negative count returned
                    res->content.value = -res->content.value;
                    return (OK);
                }
                vals = args[1].content.a->values();
                for (int i = 0; i < numpts; i++) {
                    *vals++ = MICRONS(pts[i].x);
                    *vals++ = MICRONS(pts[i].y);
                }
            }
        }
    }
    return (OK);
}


// (int) SetObjectCoords(object_handle, array, size)
//
// This function will modify a physical object to have the vertex list
// passed in the array.  The size is the number of vertices (one half
// the size of the array used).  For all but wires, the first and last
// vertices must coincide, thus the minimum number of vertices is
// four.  The array consists of x, y coordinates of the vertices.  If
// the operation is successful, 1 is returned, otherwise 0 is
// returned.  The coordinates in the array are in microns.  If the
// coordinates represent a rectangle, the new object will be a box, if
// it was previously a polygon or box.  A box may be converted to a
// polygon if the coordinates are not those of a rectangle.  For
// labels, the coordinates must represent a rectangle, and the label
// will be stretched to the new box.  The function has no effect on
// instances.  This function will fail if the handle passed is not a
// handle to an object list.
//
bool
geom1_funcs::IFsetObjectCoords(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int size;
    ARG_CHK(arg_int(args, 2, &size))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 2*size))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    const char *hdr = "object creation, in SetObjectCoords";
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        if (((sHdlObject*)hdl)->copies)
            return (OK);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
            if (sdesc && sdesc == CurCell() && !sdesc->isElectrical()) {
                if (ol->odesc->type() == CDBOX ||
                        ol->odesc->type() == CDPOLYGON) {
                    if (size < 4)
                        return (BAD);
                    Poly poly(size, new Point[size]);
                    for (int i = 0; i < size; i++)
                        poly.points[i].set(INTERNAL_UNITS(vals[2*i]),
                            INTERNAL_UNITS(vals[2*i+1]));
                    if (poly.is_rect()) {
                        BBox BB(poly.points);
                        delete [] poly.points;
                        CDo *newo = sdesc->newBox(ol->odesc, &BB,
                            ol->odesc->ldesc(), ol->odesc->prpty_list());
                        if (!newo) {
                            Errs()->add_error("newBox failed");
                            Log()->ErrorLog(hdr, Errs()->get_error());
                        }
                        else if (!sdesc->mergeBoxOrPoly(newo, true)) {
                            Errs()->add_error("mergeBoxOrPoly failed");
                            Log()->ErrorLog(hdr, Errs()->get_error());
                        }
                        else
                            res->content.value = 1;
                    }
                    else {
                        CDo *newo = sdesc->newPoly(ol->odesc, &poly,
                            ol->odesc->ldesc(), ol->odesc->prpty_list(),
                            false);
                        if (!newo) {
                            Errs()->add_error("newPoly failed");
                            Log()->ErrorLog(hdr, Errs()->get_error());
                        }
                        else if (!sdesc->mergeBoxOrPoly(newo, true)) {
                            Errs()->add_error("mergeBoxOrPoly failed");
                            Log()->ErrorLog(hdr, Errs()->get_error());
                        }
                        else
                            res->content.value = 1;
                    }
                }
                else if (ol->odesc->type() == CDWIRE) {
                    if (size < 2)
                        return (BAD);
                    Wire wire(size, new Point[size],
                        ((const CDw*)ol->odesc)->attributes());
                    for (int i = 0; i < size; i++)
                        wire.points[i].set(INTERNAL_UNITS(vals[2*i]),
                            INTERNAL_UNITS(vals[2*i+1]));
                    CDo *newo = sdesc->newWire(ol->odesc, &wire,
                        ol->odesc->ldesc(), ol->odesc->prpty_list(), false);
                    if (!newo) {
                        Errs()->add_error("newWire failed");
                        Log()->ErrorLog(hdr, Errs()->get_error());
                    }
                    else
                        res->content.value = 1;
                }
                else if (ol->odesc->type() == CDLABEL && size == 5) {
                    Point p[5];
                    for (int i = 0; i < size; i++)
                        p[i].set(INTERNAL_UNITS(vals[2*i]),
                            INTERNAL_UNITS(vals[2*i+1]));
                    if ((p[0].x == p[1].x && p[1].y == p[2].y &&
                            p[2].x == p[3].x && p[3].y == p[0].y) ||
                            (p[0].y == p[1].y && p[1].x == p[2].x &&
                            p[2].y == p[3].y && p[3].x == p[0].x)) {
                        BBox BB(p);
                        Label lab(OLABEL(ol->odesc)->la_label());
                        lab.x = BB.left;
                        lab.y = BB.bottom;
                        lab.width = BB.width();
                        lab.height = BB.height();
                        CDo *newo = sdesc->newLabel(ol->odesc, &lab,
                            ol->odesc->ldesc(), ol->odesc->prpty_list(), true);
                        if (!newo) {
                            Errs()->add_error("newLabel failed");
                            Log()->ErrorLog(hdr, Errs()->get_error());
                        }
                        else
                            res->content.value = 1;
                    }
                }
            }
        }
    }
    return (OK);
}


// (real) GetObjectMagn(object_handle)
//
// This function returns the magnification part of the transform if
// the object referenced by the handle is a subcell, or 1.0 for other
// objects.  Only physical subcells can have non-unit magnification.
// This function will fail if the handle passed is not a handle to an
// object list.  A stale handle returns 0.
//
bool
geom1_funcs::IFgetObjectMagn(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            if (ol->odesc->type() == CDINSTANCE) {
                CDtx tx(OCALL(ol->odesc));
                res->content.value = tx.magn;
            }
            else
                res->content.value = 1.0;
        }
    }
    return (OK);
}


// (int) SetObjectMagn(object_handle, magn)
//
// This will set the magnification of the subcell referenced by the
// handle, or scale other physical objects.  The real number magn must
// be between .001 and 1000 inclusive.  It applies in physical views
// only.  If the operation is successful, 1 is returned, otherwise 0
// is returned.  This function will fail if the handle passed is not
// a handle to an object list.
//
bool
geom1_funcs::IFsetObjectMagn(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double magn;
    ARG_CHK(arg_real(args, 1, &magn))

    if (magn < CDMAGMIN || magn > CDMAGMAX)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc) {
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        char buf[64];
        sprintf(buf, "%.15f", magn);
        if (ED()->acceptPseudoProp(ol->odesc, sdesc, XprpMagn, buf))
            res->content.value = 1;
    }
    return (OK);
}


// (real) GetWireWidth(object_handle)
//
// This function will return the wire width if the object referenced
// by the handle is a wire, otherwise zero is returned.  This function
// will fail if the handle passed is not a handle to an object list.
//
bool
geom1_funcs::IFgetWireWidth(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol && ol->odesc->type() == CDWIRE)
            res->content.value = MICRONS(OWIRE(ol->odesc)->wire_width());
    }
    return (OK);
}


// (int) SetWireWidth(object_handle, width)
//
// This function will set the width of the wire referenced by the
// handle to the given width (in microns).  If the operation is
// successful, 1 is returned, otherwise 0 is returned.  This function
// will fail if the handle passed is not a handle to an object list.
//
bool
geom1_funcs::IFsetWireWidth(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double width;
    ARG_CHK(arg_real(args, 1, &width))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() == CDWIRE) {
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        if (!sdesc || sdesc->isElectrical())
            return (OK);
        char buf[64];
        sprintf(buf, "%d", INTERNAL_UNITS(width));
        if (ED()->acceptPseudoProp(ol->odesc, sdesc, XprpWwidth, buf))
            res->content.value = 1;
    }
    return (OK);
}


// (int) GetWireStyle(object_handle)
//
// This function returns the end style code of the wire pointed to by
// the handle, or -1 if the object is not a wire.  The codes are
//
//     0  flush ends
//     1  projecting rounded ends
//     2  projecting square ends
//
// This function will fail if the handle passed is not a handle to an
// object list.
//
bool
geom1_funcs::IFgetWireStyle(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol && ol->odesc->type() == CDWIRE)
            res->content.value = OWIRE(ol->odesc)->wire_style();
    }
    return (OK);
}


// (int) SetWireStyle(object_handle, code)
//
// This function will change the end style of the wire referenced by
// the handle to the given code.  The code is an integer which can
// take the following values
//
//     0  flush ends
//     1  projecting rounded ends
//     2  projecting square ends
//
// If the operation succeeds, 1 is returned, otherwise zero.  This can
// apply to physical wires only.  This function will fail if the
// handle passed is not a handle to an object list.
//
bool
geom1_funcs::IFsetWireStyle(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int style;
    ARG_CHK(arg_int(args, 1, &style))

    if (style < 0 || style > 2)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() == CDWIRE) {
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        if (!sdesc || sdesc->isElectrical())
            return (OK);
        char buf[64];
        sprintf(buf, "%d", style);
        if (ED()->acceptPseudoProp(ol->odesc, sdesc, XprpWstyle, buf))
            res->content.value = 1;
    }
    return (OK);
}


// (int) SetWireToPoly(object_handle)
//
// This function converts the wire object referenced by the handle to
// a polygon object.  If the conversion is done, the handle will
// reference the new polygon object.  The conversion will be done only
// if the wire has nonzero width.  If the wire is not a copy, the wire
// object in the database will be converted to a polygon.  Otherwise,
// only the copy will be changed.  Upon success, the function returns
// 1, otherwise 0 is returned.  The function fails if the argument is
// not a handle to an object list.
//
bool
geom1_funcs::IFsetWireToPoly(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol && ol->odesc->type() == CDWIRE) {
            CDw *cdw = OWIRE(ol->odesc);
            if (cdw->wire_width() > 0) {
                if (((sHdlObject*)hdl)->copies) {
                    Poly poly;
                    if (((const CDw*)cdw)->w_toPoly(
                            &poly.points, &poly.numpts)) {
                        CDpo *cdp = new CDpo(cdw->ldesc());
                        cdp->set_points(poly.points);
                        cdp->set_numpts(poly.numpts);
                        poly.points = 0;
                        cdp->set_oBB(cdw->oBB());
                        cdp->set_copy(true);
                        delete cdw;
                        ol->odesc = cdp;
                        res->content.value = 1;
                    }
                }
                else {
                    CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
                    if (sdesc && sdesc == CurCell() &&
                            !sdesc->isElectrical()) {
                        Poly poly;
                        if (((const CDw*)cdw)->w_toPoly(
                                &poly.points, &poly.numpts)) {
                            sdesc->newPoly(cdw, &poly, cdw->ldesc(), 0, false);
                            res->content.value = 1;
                        }
                    }
                }
            }
        }
    }
    return (OK);
}


// (int) GetWirePoly(object_handle, array)
//
// This function returns the polygon used for rendering a wire.  This
// will be different from the wire vertices, if the wire has nonzero
// width.  The first argument is a handle to an object list which
// references a wire object.  The second argument is an array which
// will hold the polygon coordinates.  This argument can be 0, if the
// polygon points are not needed.  The array will be resized if
// necessary (and possible).  The return value is the number of
// vertices required or used in the polygon.  If an error occurs, the
// return value is 0.  If an array is passed which can't be resized
// because it is referenced by a pointer, the return value is a
// negative value, the negative vertex count required.  The function
// will fail if the first argument is not a handle to an object list,
// or the second argument is not an array or zero.  The coordinates
// returned in the array are in microns, relative to the origin of the
// current cell.

bool
geom1_funcs::IFgetWirePoly(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array_if(args, 1, &vals, 1))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol && ol->odesc->type() == CDWIRE) {
            CDw *cdw = OWIRE(ol->odesc);
            int numpts;
            Point *pts;
            if (((const CDw*)cdw)->w_toPoly(&pts, &numpts)) {
                res->content.value = numpts;
                if (vals) {
                    if (ADATA(args[1].content.a)->resize(numpts*2) == BAD)
                        // negative count returned
                        res->content.value = -res->content.value;
                    else {
                        vals = args[1].content.a->values();
                        for (int i = 0; i < numpts; i++) {
                            *vals++ = MICRONS(pts[i].x);
                            *vals++ = MICRONS(pts[i].y);
                        }
                    }
                }
                delete [] pts;
            }
        }
    }
    return (OK);
}


// (string) GetLabelText(object_handle)
//
// This function returns the label text if the object referenced by
// the handle is a label.  Otherwise, a null string is returned.  The
// actual text is always returned, and not the symbolic text that is
// shown on-screen for script and long text labels.  This function
// will fail if the handle passed is not a handle to an object list.
//
bool
geom1_funcs::IFgetLabelText(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol && ol->odesc->type() == CDLABEL) {
            res->content.string =
                OLABEL(ol->odesc)->label()->string(HYcvPlain, true);
            if (res->content.string)
                res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (int) SetLabelText(object_handle, text)
//
// This function will set the label text of a label referenced by the
// handle.  Setting the text in this manner will cause a long-text
// label to revert to a normal label.  If the operation succeeds, the
// return value is 1, otherwise 0 is returned.  This function will
// fail if the handle passed is not a handle to an object list.
//
bool
geom1_funcs::IFsetLabelText(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() == CDLABEL) {
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        if (ED()->acceptPseudoProp(ol->odesc, sdesc, XprpText, string))
            res->content.value = 1;
    }
    return (OK);
}


// (int) GetLabelFlags(object_handle)
//
// This function returns the orientation code of the label referenced
// by the handle, or 0 if the object is not a label.  The orientation
// code is a bit field with the following significance:
//
//    bits    description
//    0-1     0-no rotation, 1-90, 2-180, 3-270.
//    2       mirror y after rotation
//    3       mirror x after rotation and mirror y
//    4       shift rotation to 45, 135, 225, 315
//    5-6     horiz justification 00,11 left, 01 center, 10 right
//    7-8     vert justification 00,11 bottom, 01 center, 10 top
//    9-10    font
//
// This function will fail if the handle passed is not a handle to an
// object list.
//
bool
geom1_funcs::IFgetLabelFlags(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol && ol->odesc->type() == CDLABEL)
            res->content.value = OLABEL(ol->odesc)->xform();
    }
    return (OK);
}


// (int) SetLabelFlags(object_handle, xform)
//
// This function will apply the given orientation code to the label
// referenced by the handle.  If the operation is successful, 1 is
// returned, otherwise 0 is returned.  This function will fail if the
// handle passed is not a handle to an object list.
//
bool
geom1_funcs::IFsetLabelFlags(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int xform;
    ARG_CHK(arg_int(args, 1, &xform))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() == CDLABEL) {
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        char buf[64];
        sprintf(buf, "%x", xform);
        if (ED()->acceptPseudoProp(ol->odesc, sdesc, XprpXform, buf))
            res->content.value = 1;
    }
    return (OK);
}


// (int) GetInstanceArray(object_handle, array)
//
// This function fills in the array, which must have size of four or
// larger, with the array parameters for the instance referenced by
// the handle.  If the operation succeeds, 1 is returned, and the
// array components have the following values:
//
//     array[0]   number of cells along x
//     array[1]   number of cells along y
//     array[2]   center to center x spacing (in microns)
//     array[3]   center to center y spacing (in microns)
//
// If the operation fails, 0 is returned.  This function will fail if
// the handle passed is not a handle to an object list.
//
bool
geom1_funcs::IFgetInstanceArray(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 4))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol && ol->odesc->type() == CDINSTANCE) {
            CDap ap(OCALL(ol->odesc));
            vals[0] = ap.nx;
            vals[1] = ap.ny;
            vals[2] = MICRONS(ap.dx);
            vals[3] = MICRONS(ap.dy);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) SetInstanceArray(object_handle, array)
//
// This function will change the array parameters of the instance
// referenced by the handle to the indicated values.  The array values
// are in the format as returned from GetInstanceArray().  Only
// physical mode subcells can be changed by this function, arrays are
// not supported in electrical mode.  If the operation succeeds, 1 is
// returned, otherwise 0 is returned.  This function will fail if the
// handle passed is not a handle to an object list.
//
bool
geom1_funcs::IFsetInstanceArray(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 4))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    if (((sHdlObject*)hdl)->copies)
        return (OK);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() == CDINSTANCE) {
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        if (sdesc && !sdesc->isElectrical()) {
            char buf[256];
            int nx = (int)vals[0];
            int ny = (int)vals[1];
            int dx = INTERNAL_UNITS(vals[2]);
            int dy = INTERNAL_UNITS(vals[2]);
            if (nx < 1)
                nx = 1;
            if (ny < 1)
                ny = 1;
            if (dx == 0)
                dx = ol->odesc->oBB().width();
            if (dy == 0)
                dy = ol->odesc->oBB().height();
            sprintf(buf, "%d,%d %d,%d", nx, ny, dx, dy);
            if (ED()->acceptPseudoProp(ol->odesc, sdesc, XprpArray, buf))
                res->content.value = 1;
        }
    }
    return (OK);
}


// (string) GetInstanceXform(object_handle)
//
// This function returns a string giving the CIF transformation code
// for the instance referenced by the handle.  If the object is not an
// instance, a null string is returned.  This function will fail if
// the handle passed is not a handle to an object list.
//
bool
geom1_funcs::IFgetInstanceXform(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol)
            res->content.string =
                XM()->GetPseudoProp(ol->odesc, XprpTransf);
    }
    return (OK);
}


// (int) GetInstanceXformA(object_handle, array)
//
// This function fills in the array, which must have size 4 or larger,
// with the components of the transformation of the instance
// referenced by the handle.  The values are:
//  array[0]    1 if mirror-y, 0 if no mirror-y
//  array[1]    angle in degrees
//  array[2]    translation x in microns
//  array[3]    translation y in microns
// This is the same data as provided by the GetInstanceXform()
// function, but in numerical rather than string form.  The transform
// components are applied in the order as found in the array, i.e.,
// mirror first, then rotate, then translate.  The function returns 1
// if successful, 0 otherwise.  It will fail if the handle passed is
// not a handle to an object list.
//
bool
geom1_funcs::IFgetInstanceXformA(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 4))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            if (ol->odesc->type() == CDINSTANCE) {
                CDc *cd = OCALL(ol->odesc);
                CDtx tx(cd);
                vals[0] = tx.refly ? 1.0 : 0.0;
                if (tx.ay == 1) {
                    if (tx.ax == 1)
                        vals[1] = 45.0;
                    else if (tx.ax == 0)
                        vals[1] = 90.0;
                    else if (tx.ax == -1)
                        vals[1] = 135.0;
                }
                else if (tx.ay == 0) {
                    if (tx.ax == 1)
                        vals[1] = 0.0;
                    else if (tx.ax == -1)
                        vals[1] = 180.0;
                }
                else if (tx.ay == -1) {
                    if (tx.ax == -1)
                        vals[1] = 225.0;
                    else if (tx.ax == 0)
                        vals[1] = 270.0;
                    else if (tx.ax == 1)
                        vals[1] = 315.0;
                }
                vals[2] = MICRONS(tx.tx);
                vals[3] = MICRONS(tx.ty);
                res->content.value = 1.0;
            }
        }
    }
    return (OK);
}


// (int) SetInstanceXform(object_handle, transform)
//
// This function applies the given transform to the instance
// referenced by the handle.  The transform is in the form of a CIF
// transformation string, as returned by GetInstanceXform().  Note
// that coordinates in the transform string are in internal units (1
// unit = .001 micron).  Only physical-mode subcells can be modified
// by this function.  If the operation succeeds, 1 is returned,
// otherwise 0 is returned.  This function will fail if the handle
// passed is not a handle to an object list.
//
bool
geom1_funcs::IFsetInstanceXform(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *xform;
    ARG_CHK(arg_string(args, 1, &xform))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    if (((sHdlObject*)hdl)->copies)
        return (OK);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() == CDINSTANCE) {
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        if (sdesc && !sdesc->isElectrical()) {
            if (ED()->acceptPseudoProp(ol->odesc, sdesc, XprpTransf, xform))
                res->content.value = 1;
        }
    }
    return (OK);
}


// (int) SetInstanceXformA(object_handle, array)
//
// This function applies the given transform parameters in the array
// to the instance referenced by the handle.  The parameters are:
//  array[0]    1 if mirror-y, 0 if no mirror-y
//  array[1]    angle in degrees
//  array[2]    translation x in microns
//  array[3]    translation y in microns
// Only physical-mode subcells can be modified by this function.  If
// the operation succeeds, 1 is returned, otherwise 0 is returned.
// The transform components are applied in the order as found in the
// array, i.e., mirror first, then rotate, then translate.  The
// function returns 1 if successful, 0 otherwise.  It will fail if the
// handle passed is not a handle to an object list.
//
bool
geom1_funcs::IFsetInstanceXformA(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 4))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    if (((sHdlObject*)hdl)->copies)
        return (OK);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() == CDINSTANCE) {
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        if (sdesc && !sdesc->isElectrical()) {
            char buf[256];
            buf[0] = 0;
            if (to_boolean(vals[0]))
                strcpy(buf, " MY");
            if (to_boolean(vals[1]))
                sprintf(buf + strlen(buf), " R %d", (int)vals[1]);
            sprintf(buf + strlen(buf), " T %d %d", INTERNAL_UNITS(vals[2]),
                INTERNAL_UNITS(vals[3]));
            if (ED()->acceptPseudoProp(ol->odesc, sdesc, XprpTransf, buf+1))
                res->content.value = 1;
        }
    }
    return (OK);
}


// (string) GetInstanceMaster(object_handle)
//
// Note: prior to 4.2.12, this function was called GetInstanceName.
//
// This function returns the master cell name of the instance referenced by
// the handle.  If the object is not a cell instance, a null string is
// returned.  This function will fail if the handle passed is not a
// handle to an object list.  The cell instance can be electrical or
// physical, and operation is identical in electrical and physical
// mode.
//
bool
geom1_funcs::IFgetInstanceMaster(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol && ol->odesc && ol->odesc->type() == CDINSTANCE) {
            CDc *cd = (CDc*)ol->odesc;
            res->content.string = lstring::copy(cd->cellname()->string());
            res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (int) SetInstanceMaster(object_handle, newname)
//
// Note: prior to 4.2.12, this function was called SetInstanceName.
//
// This currently works with physical cell data only.
//
// This function will replace the instance referenced by the handle
// with an instance of the cell given as newname, in the parent cell
// of the referenced instance.  The current transform is added to the
// transform of the new instance.  This function will fail if the
// handle passed is not a handle to an object list.  If successful, 1
// is returned, otherwise 0 is returned.
//
bool
geom1_funcs::IFsetInstanceMaster(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))

    if (!string)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    if (((sHdlObject*)hdl)->copies)
        return (OK);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() == CDINSTANCE) {
        CDs *sdesc = ((sHdlObject*)hdl)->sdesc;
        if (sdesc && !sdesc->isElectrical()) {
            if (ED()->acceptPseudoProp(ol->odesc, sdesc, XprpName, string))
                res->content.value = 1;
        }
    }
    return (OK);
}


// (string) GetInstanceName(object_handle)
//
// Note:  prior to 4.2.12, this function returned the name of the
// instance master cell.  The GetInstanceMaster function now performs
// that operation.
//
// This function returns a name for the electrical cell instance
// referenced by the handle.  This is the name of the object, as would
// appear in a generated SPICE file.
//
// Currently, for physical instances, and for unnamed electrical
// instances, a null string is returned.
//
// Internally, names are generated in the following way.  Each device
// has a prefix, as specified in the technology file.  The prefix for
// subcircuits is "X", which is defined internally.  The prefixes
// follow (or should follow) SPICE conventions.  The database of
// instance placements is scanned in order of the placement location
// (upper-left corner of the instance bounding box) top to bottom,
// then left to right.  Each instance encountered is given an index
// number as a count of the same prefix previously encountered in the
// scan.  The prefix followed by the index forms the instance name. 
// This will identify each instance uniquely, and the sequencing is
// predictable from spatial location in the schematic.  For example. 
// X1 will be above or to the left of X2.
//
// Rather than the internal name.  this function will return an
// assigned name, if one has been given using SetInstanceName or by
// setting the name property,
//
// The index number can be obtained as an integer with
// GetInstanceIdNum.  See also GetInstanceAltName for a different
// subcircuit name style.
//
bool
geom1_funcs::IFgetInstanceName(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    res->type = TYP_STRING;
    res->content.string = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() == CDINSTANCE) {
        CDc *cd = (CDc*)ol->odesc;
        CDs *msd = cd->masterCell();
        if (!msd)
            return (BAD);
        if (msd->isElectrical()) {
            CDp_name *pna = (CDp_name*)cd->prpty(P_NAME);
            if (pna) {
                bool copied;
                hyList *hyl = pna->label_text(&copied, cd);
                res->content.string = hyl->string(HYcvPlain, false);
                if (copied)
                    hyl->free();
            }
        }
    }
    return (OK);
}


// (string) SetInstanceName(object_handle, newname)
//
// Note:  prior to 4.2.12, this function would re-master the instance,
// the same as the present SetInstanceMaster function.
//
// This will set a name for the electrical instance referenced by the
// handle, which is in effect applying a name property to the
// instance.  this makes sense for devices, subcircuits, and terminal
// devices.  The new name will be used when generating netlist output,
// so should conform to any requirements, for example SPICE
// conventions, being in force.
//
// If the string is null or 0, any applied name will be deleted,
// equivalent to "removing" a name property.
//
// The return value is 1 on success, 0 otherwise.
//
bool
geom1_funcs::IFsetInstanceName(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() == CDINSTANCE) {
        CDc *cd = (CDc*)ol->odesc;
        CDs *msd = cd->masterCell();
        if (!msd)
            return (BAD);
        if (msd->isElectrical()) {
            CDp_name *pna = (CDp_name*)cd->prpty(P_NAME);
            if (pna) {
                res->content.value = (ED()->prptyModify(cd, 0, P_NAME,
                    string, 0) != 0);
            }
        }
    }
    return (OK);
}


// (string) GetInstanceAltName(object_handle))
//
// This returns an alternative instance name for the electrical
// subcircuit cell instance referenced by the handle.  The format is
// the master cell name, followed by an underscore, followed by an
// integer.  The integer is zero-based and sequntial among instances
// of a given master.  For example, instances of master "foo" would
// have names foo_0, foo_1, etc.  This is more useful an some cases
// than the SPICE-style names X1, X2, ...  as returned by
// GetInstanceName.
//
// For electrical device instances, this function returns the same
// name As the GetInstanceName function.
//
// The GetInstanceAltIdNum function returns the index number used, as
// an integer.  This is different from the regular index, where every
// instance, of whatever type, has a unique index.  Here, instances of
// each master each have an index count starting from zero.  The order
// that instances appear, however, is the same in both lists.
//
// Presently, this function returns a null string for physical
// instances.
//
bool
geom1_funcs::IFgetInstanceAltName(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    res->type = TYP_STRING;
    res->content.string = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() == CDINSTANCE) {
        CDc *cd = (CDc*)ol->odesc;
        CDs *msd = cd->masterCell();
        if (!msd)
            return (BAD);
        if (msd->isElectrical()) {
            CDp_name *pna = (CDp_name*)cd->prpty(P_NAME);
            if (pna) {
                CDelecCellType tp = msd->elecCellType();
                if (tp == CDelecMacro || tp == CDelecSubc) {
                    char buf[16];
                    sprintf(buf, "%d", pna->scindex());
                    char *nm = new char[strlen(msd->cellname()->string()) +
                        strlen(buf) + 2];
                    char *e = lstring::stpcpy(nm, msd->cellname()->string());
                    *e++ = '_';
                    strcpy(e, buf);
                    res->content.string = nm;
                    res->flags |= VF_ORIGINAL;
                }
                else {
                    bool copied;
                    hyList *hyl = pna->label_text(&copied, cd);
                    res->content.string = hyl->string(HYcvPlain, false);
                    if (copied)
                        hyl->free();
                    res->flags |= VF_ORIGINAL;
                }
            }
        }
    }
    return (OK);
}


// (string) GetInstanceType(object_handle)
//
// This function will return a string consisting of a single letter
// that indicates the type of cell instance referenced by the handle. 
// The function will fail if the handle is of the wrong type.  A null
// string is returned it the object referenced is not a cell instance. 
// Otherwise, the following strings may be returned.
//
// These apply to electrical cell instances.
// "b"
//    The instance is "bad".  There has been an error.
//
// "n"
//    The instance type is "null" meaning that it has no electrical
//    significance in a schematic.
//
// "g"
//    The instance is a ground pin.  It has a "hot spot" that when
//    placed forces a ground contact at that location.
//
// "t"
//    This is a terminal device, which has a name label and hot spot.
//    When placed, it forces a contact to a net named in the label
//    at the hot spot location.
//
// "d"
//    The instance represents a device, such as a resistor, capacitor,
//    or transistor.
//
// "m"
//    This is a macro, which implements a subcircuit that is placed
//    in the schematic, as a "black box".  Unlike a subcircuit, a
//    macro has no sub-structure.
//
// "s"
//    This is an instance of a circuit cell, i.e., a subcircuit.  Its
//    master contains instances of devices and other objects representing
//    a circuit.
//
// For physical instances, at present there is only one return.
// "p"
//    This is a physical instance.
//
bool
geom1_funcs::IFgetInstanceType(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    res->type = TYP_STRING;
    res->content.string = 0;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() == CDINSTANCE) {
        CDc *cd = (CDc*)ol->odesc;
        CDs *msd = cd->masterCell();
        if (!msd)
            return (BAD);
        if (msd->isElectrical()) {
            const char *t = 0;
            switch (msd->elecCellType()) {
            case CDelecBad:
                t = "b";
                break;
            case CDelecNull:
                t = "n";
                break;
            case CDelecGnd:
                t = "g";
                break;
            case CDelecTerm:
                t = "t";
                break;
            case CDelecDev:
                t = "d";
                break;
            case CDelecMacro:
                t = "m";
                break;
            case CDelecSubc:
                t = "s";
                break;
            }
            res->content.string = lstring::copy(t);
            res->flags |= VF_ORIGINAL;
        }
        else {
            res->content.string = lstring::copy("p");
            res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (int) GetInstanceIdNum(object_handle)
//
// This function returns the integer index number used in electrical
// device and subcircuit instance names.  See the GetInstanceName
// description for information about how the numbers are computed. 
// Each subcircuit will have a unique number.  Devices are numbered
// according to their prefix strings, each unique prefix has its own
// number sequence.  These values are always non-negative.
//
// For physical instances, an internal indexing number used by the
// extraction system is returned.
//
// This function will return -1 on error.
//
bool
geom1_funcs::IFgetInstanceIdNum(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    res->type = TYP_SCALAR;
    res->content.value = -1;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() == CDINSTANCE) {
        CDc *cd = (CDc*)ol->odesc;
        CDs *msd = cd->masterCell();
        if (!msd)
            return (BAD);
        if (msd->isElectrical()) {
            CDp_name *pna = (CDp_name*)cd->prpty(P_NAME);
            if (pna)
                res->content.value = pna->number();
        }
        else
            res->content.value = cd->group();
    }
    return (OK);
}


// (int) GetInstanceAltIdNum(object_handle)
//
// This returns an alternative index for exectrical subcircuits, as
// used in the GetInstanceAltName function.  Every subcircuit master
// will have its instances numbered sequentially starting with 0.  The
// ordering is set by the instance placement location in the
// schematic, top to bottom then left to right, with the upper-left
// corner of the bounding box being the reference location.
//
// For other instances, the return value is the same as
// GetInstanceIdNum.
//
bool
geom1_funcs::IFgetInstanceAltIdNum(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    res->type = TYP_SCALAR;
    res->content.value = -1;
    sHdl *hdl = sHdl::get(id);
    if (!hdl)
        return (OK);
    if (hdl->type != HDLobject)
        return (BAD);
    CDol *ol = (CDol*)hdl->data;
    if (ol && ol->odesc && ol->odesc->type() == CDINSTANCE) {
        CDc *cd = (CDc*)ol->odesc;
        CDs *msd = cd->masterCell();
        if (!msd)
            return (BAD);
        if (msd->isElectrical()) {
            CDp_name *pna = (CDp_name*)cd->prpty(P_NAME);
            if (pna)
                res->content.value = pna->scindex();
        }
        else
            res->content.value = cd->group();
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Variables (export)
//-------------------------------------------------------------------------

// JoinLimits(flag)
//
// This is a convenience function to set/unset the variables which
// control the polygon joining process, ie, JoinMaxPolyVerts.
// JoinMaxPolyQueue, and JoinMaxPolyGroup.  If the argument is zero,
// each of these variables is set to zero, removing all limits.  If
// the argument is nonzero, the variables are unset, meaning that the
// default limits will be applied.  The default limits generally speed
// processing, but will often leave unjoined joinable pieces when
// complex polygons are constructed.  The status of the variables will
// persist after the script terminates.  This function has no return
// value.
//
bool
geom1_funcs::IFjoinLimits(Variable*, Variable *args, void*)
{
    bool flag;
    ARG_CHK(arg_boolean(args, 0, &flag))

    if (!flag) {
        CDvdb()->setVariable(VA_JoinMaxPolyVerts, "0");
        CDvdb()->setVariable(VA_JoinMaxPolyQueue, "0");
        CDvdb()->setVariable(VA_JoinMaxPolyGroup, "0");
    }
    else {
        CDvdb()->clearVariable(VA_JoinMaxPolyVerts);
        CDvdb()->clearVariable(VA_JoinMaxPolyQueue);
        CDvdb()->clearVariable(VA_JoinMaxPolyGroup);
    }
    return (OK);
}

