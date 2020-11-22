
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "editif.h"
#include "extif.h"
#include "drcif.h"
#include "python_if.h"
#include "tcltk_if.h"
#include "cd_lgen.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_assemble.h"
#include "fio_gdsii.h"
#include "si_parsenode.h"
#include "si_handle.h"
#include "si_args.h"
#include "si_parser.h"
#include "si_interp.h"
#include "layertab.h"
#include "kwstr_ext.h"
#include "tech_layer.h"
#include "menu.h"
#include "misc_menu.h"
#include "promptline.h"
#include "cvrt.h"
#include "select.h"
#include "tech.h"


////////////////////////////////////////////////////////////////////////
//
// Script Functions:  Non-Editing Geometry Management Functions
//
////////////////////////////////////////////////////////////////////////

namespace {
    namespace misc3_funcs {
        inline WindowDesc *
        getwin(int win)
        {
            if (win < 0 || win >= DSP_NUMWINS)
                return (0);
            return (DSP()->Window(win));
        }

        // Grid and Edge Snapping
        bool IFsetMfgGrid(Variable*, Variable*, void*);
        bool IFgetMfgGrid(Variable*, Variable*, void*);
        bool IFsetGrid(Variable*, Variable*, void*);
        bool IFgetGridInterval(Variable*, Variable*, void*);
        bool IFgetSnapInterval(Variable*, Variable*, void*);
        bool IFgetGridSnap(Variable*, Variable*, void*);
        bool IFclipToGrid(Variable*, Variable*, void*);
        bool IFsetEdgeSnappingMode(Variable*, Variable*, void*);
        bool IFsetEdgeOffGrid(Variable*, Variable*, void*);
        bool IFsetEdgeNonManh(Variable*, Variable*, void*);
        bool IFsetEdgeWireEdge(Variable*, Variable*, void*);
        bool IFsetEdgeWirePath(Variable*, Variable*, void*);
        bool IFgetEdgeSnappingMode(Variable*, Variable*, void*);
        bool IFgetEdgeOffGrid(Variable*, Variable*, void*);
        bool IFgetEdgeNonManh(Variable*, Variable*, void*);
        bool IFgetEdgeWireEdge(Variable*, Variable*, void*);
        bool IFgetEdgeWirePath(Variable*, Variable*, void*);
        bool IFsetRulerSnapToGrid(Variable*, Variable*, void*);
        bool IFsetRulerEdgeSnappingMode(Variable*, Variable*, void*);
        bool IFsetRulerEdgeOffGrid(Variable*, Variable*, void*);
        bool IFsetRulerEdgeNonManh(Variable*, Variable*, void*);
        bool IFsetRulerEdgeWireEdge(Variable*, Variable*, void*);
        bool IFsetRulerEdgeWirePath(Variable*, Variable*, void*);
        bool IFgetRulerSnapToGrid(Variable*, Variable*, void*);
        bool IFgetRulerEdgeSnappingMode(Variable*, Variable*, void*);
        bool IFgetRulerEdgeOffGrid(Variable*, Variable*, void*);
        bool IFgetRulerEdgeNonManh(Variable*, Variable*, void*);
        bool IFgetRulerEdgeWireEdge(Variable*, Variable*, void*);
        bool IFgetRulerEdgeWirePath(Variable*, Variable*, void*);

        // Grid Presentation
        bool IFshowGrid(Variable*, Variable*, void*);
        bool IFshowAxes(Variable*, Variable*, void*);
        bool IFsetGridStyle(Variable*, Variable*, void*);
        bool IFgetGridStyle(Variable*, Variable*, void*);
        bool IFsetGridCrossSize(Variable*, Variable*, void*);
        bool IFgetGridCrossSize(Variable*, Variable*, void*);
        bool IFsetGridOnTop(Variable*, Variable*, void*);
        bool IFgetGridOnTop(Variable*, Variable*, void*);
        bool IFsetGridCoarseMult(Variable*, Variable*, void*);
        bool IFgetGridCoarseMult(Variable*, Variable*, void*);
        bool IFsaveGrid(Variable*, Variable*, void*);
        bool IFrecallGrid(Variable*, Variable*, void*);

        // Current Layer
        bool IFgetCurLayer(Variable*, Variable*, void*);
        bool IFgetCurLayerIndex(Variable*, Variable*, void*);
        bool IFsetCurLayer(Variable*, Variable*, void*);
        bool IFsetCurLayerFast(Variable*, Variable*, void*);
        bool IFnewCurLayer(Variable*, Variable*, void*);
        bool IFgetCurLayerAlias(Variable*, Variable*, void*);
        bool IFsetCurLayerAlias(Variable*, Variable*, void*);
        bool IFgetCurLayerDescr(Variable*, Variable*, void*);
        bool IFsetCurLayerDescr(Variable*, Variable*, void*);

        // Layer Table
        bool IFlayersUsed(Variable*, Variable*, void*);
        bool IFaddLayer(Variable*, Variable*, void*);
        bool IFremoveLayer(Variable*, Variable*, void*);
        bool IFrenameLayer(Variable*, Variable*, void*);
        bool IFlayerHandle(Variable*, Variable*, void*);
        bool IFgenLayers(Variable*, Variable*, void*);
        bool IFgetLayerPalette(Variable*, Variable*, void*);
        bool IFsetLayerPalette(Variable*, Variable*, void*);

        // Layer Database
        bool IFgetLayerNum(Variable*, Variable*, void*);
        bool IFgetLayerName(Variable*, Variable*, void*);
        bool IFisPurposeDefined(Variable*, Variable*, void*);
        bool IFgetPurposeNum(Variable*, Variable*, void*);
        bool IFgetPurposeName(Variable*, Variable*, void*);

        // Layers
        bool IFgetLayerLayerNum(Variable*, Variable*, void*);
        bool IFgetLayerPurposeNum(Variable*, Variable*, void*);
        bool IFgetLayerAlias(Variable*, Variable*, void*);
        bool IFsetLayerAlias(Variable*, Variable*, void*);
        bool IFgetLayerDescr(Variable*, Variable*, void*);
        bool IFsetLayerDescr(Variable*, Variable*, void*);
        bool IFisLayerDefined(Variable*, Variable*, void*);
        bool IFisLayerVisible(Variable*, Variable*, void*);
        bool IFsetLayerVisible(Variable*, Variable*, void*);
        bool IFisLayerSelectable(Variable*, Variable*, void*);
        bool IFsetLayerSelectable(Variable*, Variable*, void*);
        bool IFisLayerSymbolic(Variable*, Variable*, void*);
        bool IFsetLayerSymbolic(Variable*, Variable*, void*);
        bool IFisLayerNoMerge(Variable*, Variable*, void*);
        bool IFsetLayerNoMerge(Variable*, Variable*, void*);
        bool IFgetLayerMinDimension(Variable*, Variable*, void*);
        bool IFgetLayerWireWidth(Variable*, Variable*, void*);
        bool IFaddLayerGdsOutMap(Variable*, Variable*, void*);
        bool IFremoveLayerGdsOutMap(Variable*, Variable*, void*);
        bool IFaddLayerGdsInMap(Variable*, Variable*, void*);
        bool IFclearLayerGdsInMap(Variable*, Variable*, void*);
        bool IFsetLayerNoDRCdatatype(Variable*, Variable*, void*);

        // Layers - Extraction Support
        bool IFsetLayerExKeyword(Variable*, Variable*, void*);
        bool IFsetCurLayerExKeyword(Variable*, Variable*, void*);
        bool IFremoveLayerExKeyword(Variable*, Variable*, void*);
        bool IFremoveCurLayerExKeyword(Variable*, Variable*, void*);
        bool IFisLayerConductor(Variable*, Variable*, void*);
        bool IFisLayerRouting(Variable*, Variable*, void*);
        bool IFisLayerGround(Variable*, Variable*, void*);
        bool IFisLayerContact(Variable*, Variable*, void*);
        bool IFisLayerVia(Variable*, Variable*, void*);
        bool IFisLayerViaCut(Variable*, Variable*, void*);
        bool IFisLayerDielectric(Variable*, Variable*, void*);
        bool IFisLayerDarkField(Variable*, Variable*, void*);
        bool IFgetLayerThickness(Variable*, Variable*, void*);
        bool IFgetLayerRho(Variable*, Variable*, void*);
        bool IFgetLayerResis(Variable*, Variable*, void*);
        bool IFgetLayerTau(Variable*, Variable*, void*);
        bool IFgetLayerEps(Variable*, Variable*, void*);
        bool IFgetLayerCap(Variable*, Variable*, void*);
        bool IFgetLayerCapPerim(Variable*, Variable*, void*);
        bool IFgetLayerLambda(Variable*, Variable*, void*);

        // Selections
        bool IFsetLayerSpecific(Variable*, Variable*, void*);
        bool IFsetLayerSearchUp(Variable*, Variable*, void*);
        bool IFsetSelectMode(Variable*, Variable*, void*);
        bool IFsetSelectTypes(Variable*, Variable*, void*);
        bool IFselect(Variable*, Variable*, void*);
        bool IFdeselect(Variable*, Variable*, void*);

        // Pseudo-Flat Generator
        bool IFflatObjList(Variable*, Variable*, void*);
        bool IFflatObjGen(Variable*, Variable*, void*);
        bool IFflatObjGenLayers(Variable*, Variable*, void*);
        bool IFflatGenNext(Variable*, Variable*, void*);
        bool IFflatGenCount(Variable*, Variable*, void*);
        bool IFflatOverlapList(Variable*, Variable*, void*);

        // Geometry Measurement
        bool IFdistance(Variable*, Variable*, void*);
        bool IFminDistPointToSeg(Variable*, Variable*, void*);
        bool IFminDistPointToObj(Variable*, Variable*, void*);
        bool IFminDistSegToObj(Variable*, Variable*, void*);
        bool IFminDistObjToObj(Variable*, Variable*, void*);
        bool IFmaxDistPointToObj(Variable*, Variable*, void*);
        bool IFmaxDistObjToObj(Variable*, Variable*, void*);
        bool IFintersect(Variable*, Variable*, void*);
    }
    using namespace misc3_funcs;

#ifdef HAVE_PYTHON
    // Python wrappers.

    // Grid and Edge Snapping
    PY_FUNC(SetMfgGrid,             1,  IFsetMfgGrid);
    PY_FUNC(GetMfgGrid,             0,  IFgetMfgGrid);
    PY_FUNC(SetGrid,                3,  IFsetGrid);
    PY_FUNC(GetGridInterval,        1,  IFgetGridInterval);
    PY_FUNC(GetSnapInterval,        1,  IFgetSnapInterval);
    PY_FUNC(GetGridSnap,            1,  IFgetGridSnap);
    PY_FUNC(ClipToGrid,             2,  IFclipToGrid);
    PY_FUNC(SetEdgeSnappingMode,    2,  IFsetEdgeSnappingMode);
    PY_FUNC(SetEdgeOffGrid,         2,  IFsetEdgeOffGrid);
    PY_FUNC(SetEdgeNonManh,         2,  IFsetEdgeNonManh);
    PY_FUNC(SetEdgeWireEdge,        2,  IFsetEdgeWireEdge);
    PY_FUNC(SetEdgeWirePath,        2,  IFsetEdgeWirePath);
    PY_FUNC(GetEdgeSnappingMode,    1,  IFgetEdgeSnappingMode);
    PY_FUNC(GetEdgeOffGrid,         1,  IFgetEdgeOffGrid);
    PY_FUNC(GetEdgeNonManh,         1,  IFgetEdgeNonManh);
    PY_FUNC(GetEdgeWireEdge,        1,  IFgetEdgeWireEdge);
    PY_FUNC(GetEdgeWirePath,        1,  IFgetEdgeWirePath);
    PY_FUNC(SetRulerSnapToGrid,     1,  IFsetRulerSnapToGrid);
    PY_FUNC(SetRulerEdgeSnappingMode,1, IFsetRulerEdgeSnappingMode);
    PY_FUNC(SetRulerEdgeOffGrid,    1,  IFsetRulerEdgeOffGrid);
    PY_FUNC(SetRulerEdgeNonManh,    1,  IFsetRulerEdgeNonManh);
    PY_FUNC(SetRulerEdgeWireEdge,   1,  IFsetRulerEdgeWireEdge);
    PY_FUNC(SetRulerEdgeWirePath,   1,  IFsetRulerEdgeWirePath);
    PY_FUNC(GetRulerSnapToGrid,     0,  IFgetRulerSnapToGrid);
    PY_FUNC(GetRulerEdgeSnappingMode,0, IFgetRulerEdgeSnappingMode);
    PY_FUNC(GetRulerEdgeOffGrid,    0,  IFgetRulerEdgeOffGrid);
    PY_FUNC(GetRulerEdgeNonManh,    0,  IFgetRulerEdgeNonManh);
    PY_FUNC(GetRulerEdgeWireEdge,   0,  IFgetRulerEdgeWireEdge);
    PY_FUNC(GetRulerEdgeWirePath,   0,  IFgetRulerEdgeWirePath);

    // Grid Presentation
    PY_FUNC(ShowGrid,               2,  IFshowGrid);
    PY_FUNC(ShowAxes,               2,  IFshowAxes);
    PY_FUNC(SetGridStyle,           2,  IFsetGridStyle);
    PY_FUNC(GetGridStyle,           1,  IFgetGridStyle);
    PY_FUNC(SetGridCrossSize,       2,  IFsetGridCrossSize);
    PY_FUNC(GetGridCrossSize,       1,  IFgetGridCrossSize);
    PY_FUNC(SetGridOnTop,           2,  IFsetGridOnTop);
    PY_FUNC(GetGridOnTop,           1,  IFgetGridOnTop);
    PY_FUNC(SetGridCoarseMult,      2,  IFsetGridCoarseMult);
    PY_FUNC(GetGridCoarseMult,      1,  IFgetGridCoarseMult);
    PY_FUNC(SaveGrid,               2,  IFsaveGrid);
    PY_FUNC(RecallGrid,             2,  IFrecallGrid);

    // Current Layer
    PY_FUNC(GetCurLayer,            0,  IFgetCurLayer);
    PY_FUNC(GetCurLayerIndex,       0,  IFgetCurLayerIndex);
    PY_FUNC(SetCurLayer,            1,  IFsetCurLayer);
    PY_FUNC(SetCurLayerFast,        1,  IFsetCurLayerFast);
    PY_FUNC(NewCurLayer,            1,  IFnewCurLayer);
    PY_FUNC(GetCurLayerAlias,       0,  IFgetCurLayerAlias);
    PY_FUNC(SetCurLayerAlias,       1,  IFsetCurLayerAlias);
    PY_FUNC(GetCurLayerDescr,       0,  IFgetCurLayerDescr);
    PY_FUNC(SetCurLayerDescr,       1,  IFsetCurLayerDescr);

    // Layer Table
    PY_FUNC(LayersUsed,             0,  IFlayersUsed);
    PY_FUNC(AddLayer,               2,  IFaddLayer);
    PY_FUNC(RemoveLayer,            1,  IFremoveLayer);
    PY_FUNC(RenameLayer,            2,  IFrenameLayer);
    PY_FUNC(LayerHandle,            1,  IFlayerHandle);
    PY_FUNC(GenLayers,              1,  IFgenLayers);
    PY_FUNC(GetLayerPalette,        1,  IFgetLayerPalette);
    PY_FUNC(SetLayerPalette,        2,  IFsetLayerPalette);

    // Layer Database
    PY_FUNC(GetLayerNum,            1,  IFgetLayerNum);
    PY_FUNC(GetLayerName,           1,  IFgetLayerName);
    PY_FUNC(IsPurposeDefined,       1,  IFisPurposeDefined);
    PY_FUNC(GetPurposeNum,          1,  IFgetPurposeNum);
    PY_FUNC(GetPurposeName,         1,  IFgetPurposeName);

    // Layers
    PY_FUNC(GetLayerLayerNum,       1,  IFgetLayerLayerNum);
    PY_FUNC(GetLayerPurposeNum,     1,  IFgetLayerPurposeNum);
    PY_FUNC(GetLayerAlias,          1,  IFgetLayerAlias);
    PY_FUNC(SetLayerAlias,          2,  IFsetLayerAlias);
    PY_FUNC(GetLayerDescr,          1,  IFgetLayerDescr);
    PY_FUNC(SetLayerDescr,          2,  IFsetLayerDescr);
    PY_FUNC(IsLayerDefined,         1,  IFisLayerDefined);
    PY_FUNC(IsLayerVisible,         1,  IFisLayerVisible);
    PY_FUNC(SetLayerVisible,        2,  IFsetLayerVisible);
    PY_FUNC(IsLayerSelectable,      1,  IFisLayerSelectable);
    PY_FUNC(SetLayerSelectable,     2,  IFsetLayerSelectable);
    PY_FUNC(IsLayerSymbolic,        1,  IFisLayerSymbolic);
    PY_FUNC(SetLayerSymbolic,       2,  IFsetLayerSymbolic);
    PY_FUNC(IsLayerNoMerge,         1,  IFisLayerNoMerge);
    PY_FUNC(SetLayerNoMerge,        2,  IFsetLayerNoMerge);
    PY_FUNC(GetLayerMinDimension,   1,  IFgetLayerMinDimension);
    PY_FUNC(GetLayerWireWidth,      1,  IFgetLayerWireWidth);
    PY_FUNC(AddLayerGdsOutMap,      3,  IFaddLayerGdsOutMap);
    PY_FUNC(RemoveLayerGdsOutMap,   3,  IFremoveLayerGdsOutMap);
    PY_FUNC(AddLayerGdsInMap,       2,  IFaddLayerGdsInMap);
    PY_FUNC(ClearLayerGdsInMap,     1,  IFclearLayerGdsInMap);
    PY_FUNC(SetLayerNoDRCdatatype,  2,  IFsetLayerNoDRCdatatype);

    // Layers - Extraction Support
    PY_FUNC(SetLayerExKeyword,      2,  IFsetLayerExKeyword);
    PY_FUNC(SetCurLayerExKeyword,   1,  IFsetCurLayerExKeyword);
    PY_FUNC(RemoveLayerExKeyword,   2,  IFremoveLayerExKeyword);
    PY_FUNC(RemoveCurLayerExKeyword,1,  IFremoveCurLayerExKeyword);
    PY_FUNC(IsLayerConductor,       1,  IFisLayerConductor);
    PY_FUNC(IsLayerRouting,         1,  IFisLayerRouting);
    PY_FUNC(IsLayerGround,          1,  IFisLayerGround);
    PY_FUNC(IsLayerContact,         1,  IFisLayerContact);
    PY_FUNC(IsLayerVia,             1,  IFisLayerVia);
    PY_FUNC(IsLayerViaCut,          1,  IFisLayerViaCut);
    PY_FUNC(IsLayerDielectric,      1,  IFisLayerDielectric);
    PY_FUNC(IsLayerDarkField,       1,  IFisLayerDarkField);
    PY_FUNC(GetLayerThickness,      1,  IFgetLayerThickness);
    PY_FUNC(GetLayerRho,            1,  IFgetLayerRho);
    PY_FUNC(GetLayerResis,          1,  IFgetLayerResis);
    PY_FUNC(GetLayerTau,            1,  IFgetLayerTau);
    PY_FUNC(GetLayerEps,            1,  IFgetLayerEps);
    PY_FUNC(GetLayerCap,            1,  IFgetLayerCap);
    PY_FUNC(GetLayerCapPerim,       1,  IFgetLayerCapPerim);
    PY_FUNC(GetLayerLambda,         1,  IFgetLayerLambda);

    // Selections
    PY_FUNC(SetLayerSpecific,       1,  IFsetLayerSpecific);
    PY_FUNC(SetLayerSearchUp,       1,  IFsetLayerSearchUp);
    PY_FUNC(SetSelectMode,          3,  IFsetSelectMode);
    PY_FUNC(SetSelectTypes,         1,  IFsetSelectTypes);
    PY_FUNC(Select,                 5,  IFselect);
    PY_FUNC(Deselect,               0,  IFdeselect);

    // Pseudo-Flat Generator Functions
    PY_FUNC(FlatObjList,            5,  IFflatObjList);
    PY_FUNC(FlatObjGen,             5,  IFflatObjGen);
    PY_FUNC(FlatObjGenLayers,       6,  IFflatObjGenLayers);
    PY_FUNC(FlatGenNext,            1,  IFflatGenNext);
    PY_FUNC(FlatGenCount,           1,  IFflatGenCount);
    PY_FUNC(FlatOverlapList,        4,  IFflatOverlapList);

    // Geometry Measurement
    PY_FUNC(Distance,               4,  IFdistance);
    PY_FUNC(MinDistPointToSeg,      7,  IFminDistPointToSeg);
    PY_FUNC(MinDistPointToObj,      4,  IFminDistPointToObj);
    PY_FUNC(MinDistSegToObj,        6,  IFminDistSegToObj);
    PY_FUNC(MinDistObjToObj,        3,  IFminDistObjToObj);
    PY_FUNC(MaxDistPointToObj,      4,  IFmaxDistPointToObj);
    PY_FUNC(MaxDistObjToObj,        3,  IFmaxDistObjToObj);
    PY_FUNC(Intersect,              3,  IFintersect);

    void py_register_misc3()
    {
      // Grid and Edge Snapping
      cPyIf::register_func("SetMfgGrid",             pySetMfgGrid);
      cPyIf::register_func("GetMfgGrid",             pyGetMfgGrid);
      cPyIf::register_func("SetGrid",                pySetGrid);
      cPyIf::register_func("GetGridInterval",        pyGetGridInterval);
      cPyIf::register_func("GetSnapInterval",        pyGetSnapInterval);
      cPyIf::register_func("GetGridSnap",            pyGetGridSnap);
      cPyIf::register_func("ClipToGrid",             pyClipToGrid);
      cPyIf::register_func("SetEdgeSnappingMode",    pySetEdgeSnappingMode);
      cPyIf::register_func("SetEdgeOffGrid",         pySetEdgeOffGrid);
      cPyIf::register_func("SetEdgeNonManh",         pySetEdgeNonManh);
      cPyIf::register_func("SetEdgeWireEdge",        pySetEdgeWireEdge);
      cPyIf::register_func("SetEdgeWirePath",        pySetEdgeWirePath);
      cPyIf::register_func("GetEdgeSnappingMode",    pyGetEdgeSnappingMode);
      cPyIf::register_func("GetEdgeOffGrid",         pyGetEdgeOffGrid);
      cPyIf::register_func("GetEdgeNonManh",         pyGetEdgeNonManh);
      cPyIf::register_func("GetEdgeWireEdge",        pyGetEdgeWireEdge);
      cPyIf::register_func("GetEdgeWirePath",        pyGetEdgeWirePath);
      cPyIf::register_func("SetRulerSnapToGrid",     pySetRulerSnapToGrid);
      cPyIf::register_func("SetRulerEdgeSnappingMode",pySetRulerEdgeSnappingMode);
      cPyIf::register_func("SetRulerEdgeOffGrid",    pySetRulerEdgeOffGrid);
      cPyIf::register_func("SetRulerEdgeNonManh",    pySetRulerEdgeNonManh);
      cPyIf::register_func("SetRulerEdgeWireEdge",   pySetRulerEdgeWireEdge);
      cPyIf::register_func("SetRulerEdgeWirePath",   pySetRulerEdgeWirePath);
      cPyIf::register_func("GetRulerSnapToGrid",     pyGetRulerSnapToGrid);
      cPyIf::register_func("GetRulerEdgeSnappingMode",pyGetRulerEdgeSnappingMode);
      cPyIf::register_func("GetRulerEdgeOffGrid",    pyGetRulerEdgeOffGrid);
      cPyIf::register_func("GetRulerEdgeNonManh",    pyGetRulerEdgeNonManh);
      cPyIf::register_func("GetRulerEdgeWireEdge",   pyGetRulerEdgeWireEdge);
      cPyIf::register_func("GetRulerEdgeWirePath",   pyGetRulerEdgeWirePath);

      // Grid Presentation
      cPyIf::register_func("ShowGrid",               pyShowGrid);
      cPyIf::register_func("ShowAxes",               pyShowAxes);
      cPyIf::register_func("SetGridStyle",           pySetGridStyle);
      cPyIf::register_func("GetGridStyle",           pyGetGridStyle);
      cPyIf::register_func("SetGridCrossSize",       pySetGridCrossSize);
      cPyIf::register_func("GetGridCrossSize",       pyGetGridCrossSize);
      cPyIf::register_func("SetGridOnTop",           pySetGridOnTop);
      cPyIf::register_func("GetGridOnTop",           pyGetGridOnTop);
      cPyIf::register_func("SetGridCoarseMult",      pySetGridCoarseMult);
      cPyIf::register_func("GetGridCoarseMult",      pyGetGridCoarseMult);
      cPyIf::register_func("SaveGrid",               pySaveGrid);
      cPyIf::register_func("RecallGrid",             pyRecallGrid);

      // Current Layer
      cPyIf::register_func("GetCurLayer",            pyGetCurLayer);
      cPyIf::register_func("GetCurLayerIndex",       pyGetCurLayerIndex);
      cPyIf::register_func("SetCurLayer",            pySetCurLayer);
      cPyIf::register_func("SetCurLayerFast",        pySetCurLayerFast);
      cPyIf::register_func("NewCurLayer",            pyNewCurLayer);
      cPyIf::register_func("GetCurLayerAlias",       pyGetCurLayerAlias);
      cPyIf::register_func("SetCurLayerAlias",       pySetCurLayerAlias);
      cPyIf::register_func("GetCurLayerDescr",       pyGetCurLayerDescr);
      cPyIf::register_func("SetCurLayerDescr",       pySetCurLayerDescr);

      // Layer Table
      cPyIf::register_func("LayersUsed",             pyLayersUsed);
      cPyIf::register_func("AddLayer",               pyAddLayer);
      cPyIf::register_func("RemoveLayer",            pyRemoveLayer);
      cPyIf::register_func("RenameLayer",            pyRenameLayer);
      cPyIf::register_func("LayerHandle",            pyLayerHandle);
      cPyIf::register_func("GenLayers",              pyGenLayers);
      cPyIf::register_func("GetLayerPalette",        pyGetLayerPalette);
      cPyIf::register_func("SetLayerPalette",        pySetLayerPalette);

      // Layer Database
      cPyIf::register_func("GetLayerNum",            pyGetLayerNum);
      cPyIf::register_func("GetLayerName",           pyGetLayerName);
      cPyIf::register_func("IsPurposeDefined",       pyIsPurposeDefined);
      cPyIf::register_func("GetPurposeNum",          pyGetPurposeNum);
      cPyIf::register_func("GetPurposeName",         pyGetPurposeName);

      // Layers
      cPyIf::register_func("GetLayerLayerNum",       pyGetLayerLayerNum);
      cPyIf::register_func("GetLayerPurposeNum",     pyGetLayerPurposeNum);
      cPyIf::register_func("GetLayerAlias",          pyGetLayerAlias);
      cPyIf::register_func("SetLayerAlias",          pySetLayerAlias);
      cPyIf::register_func("GetLayerDescr",          pyGetLayerDescr);
      cPyIf::register_func("SetLayerDescr",          pySetLayerDescr);
      cPyIf::register_func("IsLayerDefined",         pyIsLayerDefined);
      cPyIf::register_func("IsLayerVisible",         pyIsLayerVisible);
      cPyIf::register_func("SetLayerVisible",        pySetLayerVisible);
      cPyIf::register_func("IsLayerSelectable",      pyIsLayerSelectable);
      cPyIf::register_func("SetLayerSelectable",     pySetLayerSelectable);
      cPyIf::register_func("IsLayerSymbolic",        pyIsLayerSymbolic);
      cPyIf::register_func("SetLayerSymbolic",       pySetLayerSymbolic);
      cPyIf::register_func("IsLayerNoMerge",         pyIsLayerNoMerge);
      cPyIf::register_func("SetLayerNoMerge",        pySetLayerNoMerge);
      cPyIf::register_func("GetLayerMinDimension",   pyGetLayerMinDimension);
      cPyIf::register_func("GetLayerWireWidth",      pyGetLayerWireWidth);
      cPyIf::register_func("AddLayerGdsOutMap",      pyAddLayerGdsOutMap);
      cPyIf::register_func("RemoveLayerGdsOutMap",   pyRemoveLayerGdsOutMap);
      cPyIf::register_func("AddLayerGdsInMap",       pyAddLayerGdsInMap);
      cPyIf::register_func("ClearLayerGdsInMap",     pyClearLayerGdsInMap);
      cPyIf::register_func("SetLayerNoDRCdatatype",  pySetLayerNoDRCdatatype);

      // Layers - Extraction Support
      cPyIf::register_func("SetLayerExKeyword",      pySetLayerExKeyword);
      cPyIf::register_func("SetCurLayerExKeyword",   pySetCurLayerExKeyword);
      cPyIf::register_func("RemoveLayerExKeyword",   pyRemoveLayerExKeyword);
      cPyIf::register_func("RemoveCurLayerExKeyword",pyRemoveCurLayerExKeyword);
      cPyIf::register_func("IsLayerConductor",       pyIsLayerConductor);
      cPyIf::register_func("IsLayerRouting",         pyIsLayerRouting);
      cPyIf::register_func("IsLayerGround",          pyIsLayerGround);
      cPyIf::register_func("IsLayerContact",         pyIsLayerContact);
      cPyIf::register_func("IsLayerVia",             pyIsLayerVia);
      cPyIf::register_func("IsLayerViaCut",          pyIsLayerViaCut);
      cPyIf::register_func("IsLayerDielectric",      pyIsLayerDielectric);
      cPyIf::register_func("IsLayerDarkField",       pyIsLayerDarkField);
      cPyIf::register_func("GetLayerThickness",      pyGetLayerThickness);
      cPyIf::register_func("GetLayerRho",            pyGetLayerRho);
      cPyIf::register_func("GetLayerResis",          pyGetLayerResis);
      cPyIf::register_func("GetLayerEps",            pyGetLayerEps);
      cPyIf::register_func("GetLayerCap",            pyGetLayerCap);
      cPyIf::register_func("GetLayerCapPerim",       pyGetLayerCapPerim);
      cPyIf::register_func("GetLayerLambda",         pyGetLayerLambda);

      // Selections
      cPyIf::register_func("SetLayerSpecific",       pySetLayerSpecific);
      cPyIf::register_func("SetLayerSearchUp",       pySetLayerSearchUp);
      cPyIf::register_func("SetSelectMode",          pySetSelectMode);
      cPyIf::register_func("SetSelectTypes",         pySetSelectTypes);
      cPyIf::register_func("Select",                 pySelect);
      cPyIf::register_func("Deselect",               pyDeselect);

      // Pseudo-Flat Generator Functions
      cPyIf::register_func("FlatObjList",            pyFlatObjList);
      cPyIf::register_func("FlatObjGen",             pyFlatObjGen);
      cPyIf::register_func("FlatObjGenLayers",       pyFlatObjGenLayers);
      cPyIf::register_func("FlatGenNext",            pyFlatGenNext);
      cPyIf::register_func("FlatGenCount",           pyFlatGenCount);
      cPyIf::register_func("FlatOverlapList",        pyFlatOverlapList);

      // Geometry Measurement
      cPyIf::register_func("Distance",               pyDistance);
      cPyIf::register_func("MinDistPointToSeg",      pyMinDistPointToSeg);
      cPyIf::register_func("MinDistPointToObj",      pyMinDistPointToObj);
      cPyIf::register_func("MinDistSegToObj",        pyMinDistSegToObj);
      cPyIf::register_func("MinDistObjToObj",        pyMinDistObjToObj);
      cPyIf::register_func("MaxDistPointToObj",      pyMaxDistPointToObj);
      cPyIf::register_func("MaxDistObjToObj",        pyMaxDistObjToObj);
      cPyIf::register_func("Intersect",              pyIntersect);
    }
#endif  // HAVE_PYTHON

#ifdef HAVE_TCL
    // Tcl/Tk wrappers.

    // Grid and Edge Snapping
    TCL_FUNC(SetMfgGrid,             1,  IFsetMfgGrid);
    TCL_FUNC(GetMfgGrid,             0,  IFgetMfgGrid);
    TCL_FUNC(SetGrid,                3,  IFsetGrid);
    TCL_FUNC(GetGridInterval,        1,  IFgetGridInterval);
    TCL_FUNC(GetSnapInterval,        1,  IFgetSnapInterval);
    TCL_FUNC(GetGridSnap,            1,  IFgetGridSnap);
    TCL_FUNC(ClipToGrid,             2,  IFclipToGrid);
    TCL_FUNC(SetEdgeSnappingMode,    2,  IFsetEdgeSnappingMode);
    TCL_FUNC(SetEdgeOffGrid,         2,  IFsetEdgeOffGrid);
    TCL_FUNC(SetEdgeNonManh,         2,  IFsetEdgeNonManh);
    TCL_FUNC(SetEdgeWireEdge,        2,  IFsetEdgeWireEdge);
    TCL_FUNC(SetEdgeWirePath,        2,  IFsetEdgeWirePath);
    TCL_FUNC(GetEdgeSnappingMode,    1,  IFgetEdgeSnappingMode);
    TCL_FUNC(GetEdgeOffGrid,         1,  IFgetEdgeOffGrid);
    TCL_FUNC(GetEdgeNonManh,         1,  IFgetEdgeNonManh);
    TCL_FUNC(GetEdgeWireEdge,        1,  IFgetEdgeWireEdge);
    TCL_FUNC(GetEdgeWirePath,        1,  IFgetEdgeWirePath);
    TCL_FUNC(SetRulerSnapToGrid,     1,  IFsetRulerSnapToGrid);
    TCL_FUNC(SetRulerEdgeSnappingMode,1, IFsetRulerEdgeSnappingMode);
    TCL_FUNC(SetRulerEdgeOffGrid,    1,  IFsetRulerEdgeOffGrid);
    TCL_FUNC(SetRulerEdgeNonManh,    1,  IFsetRulerEdgeNonManh);
    TCL_FUNC(SetRulerEdgeWireEdge,   1,  IFsetRulerEdgeWireEdge);
    TCL_FUNC(SetRulerEdgeWirePath,   1,  IFsetRulerEdgeWirePath);
    TCL_FUNC(GetRulerSnapToGrid,     0,  IFgetRulerSnapToGrid);
    TCL_FUNC(GetRulerEdgeSnappingMode,0, IFgetRulerEdgeSnappingMode);
    TCL_FUNC(GetRulerEdgeOffGrid,    0,  IFgetRulerEdgeOffGrid);
    TCL_FUNC(GetRulerEdgeNonManh,    0,  IFgetRulerEdgeNonManh);
    TCL_FUNC(GetRulerEdgeWireEdge,   0,  IFgetRulerEdgeWireEdge);
    TCL_FUNC(GetRulerEdgeWirePath,   0,  IFgetRulerEdgeWirePath);

    // Grid Presentation
    TCL_FUNC(ShowGrid,               2,  IFshowGrid);
    TCL_FUNC(ShowAxes,               2,  IFshowAxes);
    TCL_FUNC(SetGridStyle,           2,  IFsetGridStyle);
    TCL_FUNC(GetGridStyle,           1,  IFgetGridStyle);
    TCL_FUNC(SetGridCrossSize,       2,  IFsetGridCrossSize);
    TCL_FUNC(GetGridCrossSize,       1,  IFgetGridCrossSize);
    TCL_FUNC(SetGridOnTop,           2,  IFsetGridOnTop);
    TCL_FUNC(GetGridOnTop,           1,  IFgetGridOnTop);
    TCL_FUNC(SetGridCoarseMult,      2,  IFsetGridCoarseMult);
    TCL_FUNC(GetGridCoarseMult,      1,  IFgetGridCoarseMult);
    TCL_FUNC(SaveGrid,               2,  IFsaveGrid);
    TCL_FUNC(RecallGrid,             2,  IFrecallGrid);

    // Current Layer
    TCL_FUNC(GetCurLayer,            0,  IFgetCurLayer);
    TCL_FUNC(GetCurLayerIndex,       0,  IFgetCurLayerIndex);
    TCL_FUNC(SetCurLayer,            1,  IFsetCurLayer);
    TCL_FUNC(SetCurLayerFast,        1,  IFsetCurLayerFast);
    TCL_FUNC(NewCurLayer,            1,  IFnewCurLayer);
    TCL_FUNC(GetCurLayerAlias,       0,  IFgetCurLayerAlias);
    TCL_FUNC(SetCurLayerAlias,       1,  IFsetCurLayerAlias);
    TCL_FUNC(GetCurLayerDescr,       0,  IFgetCurLayerDescr);
    TCL_FUNC(SetCurLayerDescr,       1,  IFsetCurLayerDescr);

    // Layer Table
    TCL_FUNC(LayersUsed,             0,  IFlayersUsed);
    TCL_FUNC(AddLayer,               2,  IFaddLayer);
    TCL_FUNC(RemoveLayer,            1,  IFremoveLayer);
    TCL_FUNC(RenameLayer,            2,  IFrenameLayer);
    TCL_FUNC(LayerHandle,            1,  IFlayerHandle);
    TCL_FUNC(GenLayers,              1,  IFgenLayers);
    TCL_FUNC(GetLayerPalette,        1,  IFgetLayerPalette);
    TCL_FUNC(SetLayerPalette,        2,  IFsetLayerPalette);

    // Layer Database
    TCL_FUNC(GetLayerNum,            1,  IFgetLayerNum);
    TCL_FUNC(GetLayerName,           1,  IFgetLayerName);
    TCL_FUNC(IsPurposeDefined,       1,  IFisPurposeDefined);
    TCL_FUNC(GetPurposeNum,          1,  IFgetPurposeNum);
    TCL_FUNC(GetPurposeName,         1,  IFgetPurposeName);

    // Layers
    TCL_FUNC(GetLayerLayerNum,       1,  IFgetLayerLayerNum);
    TCL_FUNC(GetLayerPurposeNum,     1,  IFgetLayerPurposeNum);
    TCL_FUNC(GetLayerAlias,          1,  IFgetLayerAlias);
    TCL_FUNC(SetLayerAlias,          2,  IFsetLayerAlias);
    TCL_FUNC(GetLayerDescr,          1,  IFgetLayerDescr);
    TCL_FUNC(SetLayerDescr,          2,  IFsetLayerDescr);
    TCL_FUNC(IsLayerDefined,         1,  IFisLayerDefined);
    TCL_FUNC(IsLayerVisible,         1,  IFisLayerVisible);
    TCL_FUNC(SetLayerVisible,        2,  IFsetLayerVisible);
    TCL_FUNC(IsLayerSelectable,      1,  IFisLayerSelectable);
    TCL_FUNC(SetLayerSelectable,     2,  IFsetLayerSelectable);
    TCL_FUNC(IsLayerSymbolic,        1,  IFisLayerSymbolic);
    TCL_FUNC(SetLayerSymbolic,       2,  IFsetLayerSymbolic);
    TCL_FUNC(IsLayerNoMerge,         1,  IFisLayerNoMerge);
    TCL_FUNC(SetLayerNoMerge,        2,  IFsetLayerNoMerge);
    TCL_FUNC(GetLayerMinDimension,   1,  IFgetLayerMinDimension);
    TCL_FUNC(GetLayerWireWidth,      1,  IFgetLayerWireWidth);
    TCL_FUNC(AddLayerGdsOutMap,      3,  IFaddLayerGdsOutMap);
    TCL_FUNC(RemoveLayerGdsOutMap,   3,  IFremoveLayerGdsOutMap);
    TCL_FUNC(AddLayerGdsInMap,       2,  IFaddLayerGdsInMap);
    TCL_FUNC(ClearLayerGdsInMap,     1,  IFclearLayerGdsInMap);
    TCL_FUNC(SetLayerNoDRCdatatype,  2,  IFsetLayerNoDRCdatatype);

    // Layers - Extraction Support
    TCL_FUNC(SetLayerExKeyword,      2,  IFsetLayerExKeyword);
    TCL_FUNC(SetCurLayerExKeyword,   1,  IFsetCurLayerExKeyword);
    TCL_FUNC(RemoveLayerExKeyword,   2,  IFremoveLayerExKeyword);
    TCL_FUNC(RemoveCurLayerExKeyword,1,  IFremoveCurLayerExKeyword);
    TCL_FUNC(IsLayerConductor,       1,  IFisLayerConductor);
    TCL_FUNC(IsLayerRouting,         1,  IFisLayerRouting);
    TCL_FUNC(IsLayerGround,          1,  IFisLayerGround);
    TCL_FUNC(IsLayerContact,         1,  IFisLayerContact);
    TCL_FUNC(IsLayerVia,             1,  IFisLayerVia);
    TCL_FUNC(IsLayerViaCut,          1,  IFisLayerViaCut);
    TCL_FUNC(IsLayerDielectric,      1,  IFisLayerDielectric);
    TCL_FUNC(IsLayerDarkField,       1,  IFisLayerDarkField);
    TCL_FUNC(GetLayerThickness,      1,  IFgetLayerThickness);
    TCL_FUNC(GetLayerRho,            1,  IFgetLayerRho);
    TCL_FUNC(GetLayerResis,          1,  IFgetLayerResis);
    TCL_FUNC(GetLayerTau,            1,  IFgetLayerTau);
    TCL_FUNC(GetLayerEps,            1,  IFgetLayerEps);
    TCL_FUNC(GetLayerCap,            1,  IFgetLayerCap);
    TCL_FUNC(GetLayerCapPerim,       1,  IFgetLayerCapPerim);
    TCL_FUNC(GetLayerLambda,         1,  IFgetLayerLambda);

    // Selections
    TCL_FUNC(SetLayerSpecific,       1,  IFsetLayerSpecific);
    TCL_FUNC(SetLayerSearchUp,       1,  IFsetLayerSearchUp);
    TCL_FUNC(SetSelectMode,          3,  IFsetSelectMode);
    TCL_FUNC(SetSelectTypes,         1,  IFsetSelectTypes);
    TCL_FUNC(Select,                 5,  IFselect);
    TCL_FUNC(Deselect,               0,  IFdeselect);

    // Pseudo-Flat Generator Functions
    TCL_FUNC(FlatObjList,            5,  IFflatObjList);
    TCL_FUNC(FlatObjGen,             5,  IFflatObjGen);
    TCL_FUNC(FlatObjGenLayers,       6,  IFflatObjGenLayers);
    TCL_FUNC(FlatGenNext,            1,  IFflatGenNext);
    TCL_FUNC(FlatGenCount,           1,  IFflatGenCount);
    TCL_FUNC(FlatOverlapList,        4,  IFflatOverlapList);

    // Geometry Measurement
    TCL_FUNC(Distance,               4,  IFdistance);
    TCL_FUNC(MinDistPointToSeg,      7,  IFminDistPointToSeg);
    TCL_FUNC(MinDistPointToObj,      4,  IFminDistPointToObj);
    TCL_FUNC(MinDistSegToObj,        6,  IFminDistSegToObj);
    TCL_FUNC(MinDistObjToObj,        3,  IFminDistObjToObj);
    TCL_FUNC(MaxDistPointToObj,      4,  IFmaxDistPointToObj);
    TCL_FUNC(MaxDistObjToObj,        3,  IFmaxDistObjToObj);
    TCL_FUNC(Intersect,              3,  IFintersect);

    void tcl_register_misc3()
    {
      // Grid and Edge Snapping
      cTclIf::register_func("SetMfgGrid",             tclSetMfgGrid);
      cTclIf::register_func("GetMfgGrid",             tclGetMfgGrid);
      cTclIf::register_func("SetGrid",                tclSetGrid);
      cTclIf::register_func("GetGridInterval",        tclGetGridInterval);
      cTclIf::register_func("GetSnapInterval",        tclGetSnapInterval);
      cTclIf::register_func("GetGridSnap",            tclGetGridSnap);
      cTclIf::register_func("ClipToGrid",             tclClipToGrid);
      cTclIf::register_func("SetEdgeSnappingMode",    tclSetEdgeSnappingMode);
      cTclIf::register_func("SetEdgeOffGrid",         tclSetEdgeOffGrid);
      cTclIf::register_func("SetEdgeNonManh",         tclSetEdgeNonManh);
      cTclIf::register_func("SetEdgeWireEdge",        tclSetEdgeWireEdge);
      cTclIf::register_func("SetEdgeWirePath",        tclSetEdgeWirePath);
      cTclIf::register_func("GetEdgeSnappingMode",    tclGetEdgeSnappingMode);
      cTclIf::register_func("GetEdgeOffGrid",         tclGetEdgeOffGrid);
      cTclIf::register_func("GetEdgeNonManh",         tclGetEdgeNonManh);
      cTclIf::register_func("GetEdgeWireEdge",        tclGetEdgeWireEdge);
      cTclIf::register_func("GetEdgeWirePath",        tclGetEdgeWirePath);
      cTclIf::register_func("SetRulerSnapToGrid",     tclSetRulerSnapToGrid);
      cTclIf::register_func("SetRulerEdgeSnappingMode",tclSetRulerEdgeSnappingMode);
      cTclIf::register_func("SetRulerEdgeOffGrid",    tclSetRulerEdgeOffGrid);
      cTclIf::register_func("SetRulerEdgeNonManh",    tclSetRulerEdgeNonManh);
      cTclIf::register_func("SetRulerEdgeWireEdge",   tclSetRulerEdgeWireEdge);
      cTclIf::register_func("SetRulerEdgeWirePath",   tclSetRulerEdgeWirePath);
      cTclIf::register_func("GetRulerSnapToGrid",     tclGetRulerSnapToGrid);
      cTclIf::register_func("GetRulerEdgeSnappingMode",tclGetRulerEdgeSnappingMode);
      cTclIf::register_func("GetRulerEdgeOffGrid",    tclGetRulerEdgeOffGrid);
      cTclIf::register_func("GetRulerEdgeNonManh",    tclGetRulerEdgeNonManh);
      cTclIf::register_func("GetRulerEdgeWireEdge",   tclGetRulerEdgeWireEdge);
      cTclIf::register_func("GetRulerEdgeWirePath",   tclGetRulerEdgeWirePath);

      // Grid Presentation
      cTclIf::register_func("ShowGrid",               tclShowGrid);
      cTclIf::register_func("ShowAxes",               tclShowAxes);
      cTclIf::register_func("SetGridStyle",           tclSetGridStyle);
      cTclIf::register_func("GetGridStyle",           tclGetGridStyle);
      cTclIf::register_func("SetGridCrossSize",       tclSetGridCrossSize);
      cTclIf::register_func("GetGridCrossSize",       tclGetGridCrossSize);
      cTclIf::register_func("SetGridOnTop",           tclSetGridOnTop);
      cTclIf::register_func("GetGridOnTop",           tclGetGridOnTop);
      cTclIf::register_func("SetGridCoarseMult",      tclSetGridCoarseMult);
      cTclIf::register_func("GetGridCoarseMult",      tclGetGridCoarseMult);
      cTclIf::register_func("SaveGrid",               tclSaveGrid);
      cTclIf::register_func("RecallGrid",             tclRecallGrid);

      // Current Layer
      cTclIf::register_func("GetCurLayer",            tclGetCurLayer);
      cTclIf::register_func("GetCurLayerIndex",       tclGetCurLayerIndex);
      cTclIf::register_func("SetCurLayer",            tclSetCurLayer);
      cTclIf::register_func("SetCurLayerFast",        tclSetCurLayerFast);
      cTclIf::register_func("NewCurLayer",            tclNewCurLayer);
      cTclIf::register_func("GetCurLayerAlias",       tclGetCurLayerAlias);
      cTclIf::register_func("SetCurLayerAlias",       tclSetCurLayerAlias);
      cTclIf::register_func("GetCurLayerDescr",       tclGetCurLayerDescr);
      cTclIf::register_func("SetCurLayerDescr",       tclSetCurLayerDescr);

      // Layer Table
      cTclIf::register_func("LayersUsed",             tclLayersUsed);
      cTclIf::register_func("AddLayer",               tclAddLayer);
      cTclIf::register_func("RemoveLayer",            tclRemoveLayer);
      cTclIf::register_func("RenameLayer",            tclRenameLayer);
      cTclIf::register_func("LayerHandle",            tclLayerHandle);
      cTclIf::register_func("GenLayers",              tclGenLayers);
      cTclIf::register_func("GetLayerPalette",        tclGetLayerPalette);
      cTclIf::register_func("SetLayerPalette",        tclSetLayerPalette);

      // Layer Database
      cTclIf::register_func("GetLayerNum",            tclGetLayerNum);
      cTclIf::register_func("GetLayerName",           tclGetLayerName);
      cTclIf::register_func("IsPurposeDefined",       tclIsPurposeDefined);
      cTclIf::register_func("GetPurposeNum",          tclGetPurposeNum);
      cTclIf::register_func("GetPurposeName",         tclGetPurposeName);

      // Layers
      cTclIf::register_func("GetLayerLayerNum",       tclGetLayerLayerNum);
      cTclIf::register_func("GetLayerPurposeNum",     tclGetLayerPurposeNum);
      cTclIf::register_func("GetLayerAlias",          tclGetLayerAlias);
      cTclIf::register_func("SetLayerAlias",          tclSetLayerAlias);
      cTclIf::register_func("GetLayerDescr",          tclGetLayerDescr);
      cTclIf::register_func("SetLayerDescr",          tclSetLayerDescr);
      cTclIf::register_func("IsLayerDefined",         tclIsLayerDefined);
      cTclIf::register_func("IsLayerVisible",         tclIsLayerVisible);
      cTclIf::register_func("SetLayerVisible",        tclSetLayerVisible);
      cTclIf::register_func("IsLayerSelectable",      tclIsLayerSelectable);
      cTclIf::register_func("SetLayerSelectable",     tclSetLayerSelectable);
      cTclIf::register_func("IsLayerSymbolic",        tclIsLayerSymbolic);
      cTclIf::register_func("SetLayerSymbolic",       tclSetLayerSymbolic);
      cTclIf::register_func("IsLayerNoMerge",         tclIsLayerNoMerge);
      cTclIf::register_func("SetLayerNoMerge",        tclSetLayerNoMerge);
      cTclIf::register_func("GetLayerMinDimension",   tclGetLayerMinDimension);
      cTclIf::register_func("GetLayerWireWidth",      tclGetLayerWireWidth);
      cTclIf::register_func("AddLayerGdsOutMap",      tclAddLayerGdsOutMap);
      cTclIf::register_func("RemoveLayerGdsOutMap",   tclRemoveLayerGdsOutMap);
      cTclIf::register_func("AddLayerGdsInMap",       tclAddLayerGdsInMap);
      cTclIf::register_func("ClearLayerGdsInMap",     tclClearLayerGdsInMap);
      cTclIf::register_func("SetLayerNoDRCdatatype",  tclSetLayerNoDRCdatatype);

      // Layers Extraction Support
      cTclIf::register_func("SetLayerExKeyword",      tclSetLayerExKeyword);
      cTclIf::register_func("SetCurLayerExKeyword",   tclSetCurLayerExKeyword);
      cTclIf::register_func("RemoveLayerExKeyword",   tclRemoveLayerExKeyword);
      cTclIf::register_func("RemoveCurLayerExKeyword",tclRemoveCurLayerExKeyword);
      cTclIf::register_func("IsLayerConductor",       tclIsLayerConductor);
      cTclIf::register_func("IsLayerRouting",         tclIsLayerRouting);
      cTclIf::register_func("IsLayerGround",          tclIsLayerGround);
      cTclIf::register_func("IsLayerContact",         tclIsLayerContact);
      cTclIf::register_func("IsLayerVia",             tclIsLayerVia);
      cTclIf::register_func("IsLayerViaCut",          tclIsLayerViaCut);
      cTclIf::register_func("IsLayerDielectric",      tclIsLayerDielectric);
      cTclIf::register_func("IsLayerDarkField",       tclIsLayerDarkField);
      cTclIf::register_func("GetLayerThickness",      tclGetLayerThickness);
      cTclIf::register_func("GetLayerRho",            tclGetLayerRho);
      cTclIf::register_func("GetLayerResis",          tclGetLayerResis);
      cTclIf::register_func("GetLayerEps",            tclGetLayerEps);
      cTclIf::register_func("GetLayerCap",            tclGetLayerCap);
      cTclIf::register_func("GetLayerCapPerim",       tclGetLayerCapPerim);
      cTclIf::register_func("GetLayerLambda",         tclGetLayerLambda);

      // Selections
      cTclIf::register_func("SetLayerSpecific",       tclSetLayerSpecific);
      cTclIf::register_func("SetLayerSearchUp",       tclSetLayerSearchUp);
      cTclIf::register_func("SetSelectMode",          tclSetSelectMode);
      cTclIf::register_func("SetSelectTypes",         tclSetSelectTypes);
      cTclIf::register_func("Select",                 tclSelect);
      cTclIf::register_func("Deselect",               tclDeselect);

      // Pseudo-Flat Generator Functions
      cTclIf::register_func("FlatObjList",            tclFlatObjList);
      cTclIf::register_func("FlatObjGen",             tclFlatObjGen);
      cTclIf::register_func("FlatObjGenLayers",       tclFlatObjGenLayers);
      cTclIf::register_func("FlatGenNext",            tclFlatGenNext);
      cTclIf::register_func("FlatGenCount",           tclFlatGenCount);
      cTclIf::register_func("FlatOverlapList",        tclFlatOverlapList);

      // Geometry Measurement
      cTclIf::register_func("Distance",               tclDistance);
      cTclIf::register_func("MinDistPointToSeg",      tclMinDistPointToSeg);
      cTclIf::register_func("MinDistPointToObj",      tclMinDistPointToObj);
      cTclIf::register_func("MinDistSegToObj",        tclMinDistSegToObj);
      cTclIf::register_func("MinDistObjToObj",        tclMinDistObjToObj);
      cTclIf::register_func("MaxDistPointToObj",      tclMaxDistPointToObj);
      cTclIf::register_func("MaxDistObjToObj",        tclMaxDistObjToObj);
      cTclIf::register_func("Intersect",              tclIntersect);
    }
#endif  // HAVE_TCL
}


// Export to load functions in this script library.
//
void
cMain::load_funcs_misc3()
{
  using namespace misc3_funcs;

  // Grid and Edge Snapping
  SIparse()->registerFunc("SetMfgGrid",             1,  IFsetMfgGrid);
  SIparse()->registerFunc("GetMfgGrid",             0,  IFgetMfgGrid);
  SIparse()->registerFunc("SetGrid",                3,  IFsetGrid);
  SIparse()->registerFunc("GetGridInterval",        1,  IFgetGridInterval);
  SIparse()->registerFunc("GetSnapInterval",        1,  IFgetSnapInterval);
  SIparse()->registerFunc("GetGridSnap",            1,  IFgetGridSnap);
  SIparse()->registerFunc("ClipToGrid",             2,  IFclipToGrid);
  SIparse()->registerFunc("SetEdgeSnappingMode",    2,  IFsetEdgeSnappingMode);
  SIparse()->registerFunc("SetEdgeOffGrid",         2,  IFsetEdgeOffGrid);
  SIparse()->registerFunc("SetEdgeNonManh",         2,  IFsetEdgeNonManh);
  SIparse()->registerFunc("SetEdgeWireEdge",        2,  IFsetEdgeWireEdge);
  SIparse()->registerFunc("SetEdgeWirePath",        2,  IFsetEdgeWirePath);
  SIparse()->registerFunc("GetEdgeSnappingMode",    1,  IFgetEdgeSnappingMode);
  SIparse()->registerFunc("GetEdgeOffGrid",         1,  IFgetEdgeOffGrid);
  SIparse()->registerFunc("GetEdgeNonManh",         1,  IFgetEdgeNonManh);
  SIparse()->registerFunc("GetEdgeWireEdge",        1,  IFgetEdgeWireEdge);
  SIparse()->registerFunc("GetEdgeWirePath",        1,  IFgetEdgeWirePath);
  SIparse()->registerFunc("SetRulerSnapToGrid",     1,  IFsetRulerSnapToGrid);
  SIparse()->registerFunc("SetRulerEdgeSnappingMode",1, IFsetRulerEdgeSnappingMode);
  SIparse()->registerFunc("SetRulerEdgeOffGrid",    1,  IFsetRulerEdgeOffGrid);
  SIparse()->registerFunc("SetRulerEdgeNonManh",    1,  IFsetRulerEdgeNonManh);
  SIparse()->registerFunc("SetRulerEdgeWireEdge",   1,  IFsetRulerEdgeWireEdge);
  SIparse()->registerFunc("SetRulerEdgeWirePath",   1,  IFsetRulerEdgeWirePath);
  SIparse()->registerFunc("GetRulerSnapToGrid",     0,  IFgetRulerSnapToGrid);
  SIparse()->registerFunc("GetRulerEdgeSnappingMode",0, IFgetRulerEdgeSnappingMode);
  SIparse()->registerFunc("GetRulerEdgeOffGrid",    0,  IFgetRulerEdgeOffGrid);
  SIparse()->registerFunc("GetRulerEdgeNonManh",    0,  IFgetRulerEdgeNonManh);
  SIparse()->registerFunc("GetRulerEdgeWireEdge",   0,  IFgetRulerEdgeWireEdge);
  SIparse()->registerFunc("GetRulerEdgeWirePath",   0,  IFgetRulerEdgeWirePath);

  // Grid Presentation
  SIparse()->registerFunc("ShowGrid",               2,  IFshowGrid);
  SIparse()->registerFunc("ShowAxes",               2,  IFshowAxes);
  SIparse()->registerFunc("SetGridStyle",           2,  IFsetGridStyle);
  SIparse()->registerFunc("GetGridStyle",           1,  IFgetGridStyle);
  SIparse()->registerFunc("SetGridCrossSize",       2,  IFsetGridCrossSize);
  SIparse()->registerFunc("GetGridCrossSize",       1,  IFgetGridCrossSize);
  SIparse()->registerFunc("SetGridOnTop",           2,  IFsetGridOnTop);
  SIparse()->registerFunc("GetGridOnTop",           1,  IFgetGridOnTop);
  SIparse()->registerFunc("SetGridCoarseMult",      2,  IFsetGridCoarseMult);
  SIparse()->registerFunc("GetGridCoarseMult",      1,  IFgetGridCoarseMult);
  SIparse()->registerFunc("SaveGrid",               2,  IFsaveGrid);
  SIparse()->registerFunc("RecallGrid",             2,  IFrecallGrid);

  // Current Layer
  SIparse()->registerFunc("GetCurLayer",            0,  IFgetCurLayer);
  SIparse()->registerFunc("GetCurLayerIndex",       0,  IFgetCurLayerIndex);
  SIparse()->registerFunc("SetCurLayer",            1,  IFsetCurLayer);
  SIparse()->registerFunc("SetCurLayerFast",        1,  IFsetCurLayerFast);
  SIparse()->registerFunc("NewCurLayer",            1,  IFnewCurLayer);
  SIparse()->registerFunc("GetCurLayerAlias",       0,  IFgetCurLayerAlias);
  SIparse()->registerFunc("SetCurLayerAlias",       1,  IFsetCurLayerAlias);
  SIparse()->registerFunc("GetCurLayerDescr",       0,  IFgetCurLayerDescr);
  SIparse()->registerFunc("SetCurLayerDescr",       1,  IFsetCurLayerDescr);

  // Layer Table
  SIparse()->registerFunc("LayersUsed",             0,  IFlayersUsed);
  SIparse()->registerFunc("AddLayer",               2,  IFaddLayer);
  SIparse()->registerFunc("RemoveLayer",            1,  IFremoveLayer);
  SIparse()->registerFunc("RenameLayer",            2,  IFrenameLayer);
  SIparse()->registerFunc("LayerHandle",            1,  IFlayerHandle);
  SIparse()->registerFunc("GenLayers",              1,  IFgenLayers);
  SIparse()->registerFunc("GetLayerPalette",        1,  IFgetLayerPalette);
  SIparse()->registerFunc("SetLayerPalette",        2,  IFsetLayerPalette);

  // Layer Database
  SIparse()->registerFunc("GetLayerNum",            1,  IFgetLayerNum);
  SIparse()->registerFunc("GetLayerName",           1,  IFgetLayerName);
  SIparse()->registerFunc("IsPurposeDefined",       1,  IFisPurposeDefined);
  SIparse()->registerFunc("GetPurposeNum",          1,  IFgetPurposeNum);
  SIparse()->registerFunc("GetPurposeName",         1,  IFgetPurposeName);

  // Layers
  SIparse()->registerFunc("GetLayerLayerNum",       1,  IFgetLayerLayerNum);
  SIparse()->registerFunc("GetLayerPurposeNum",     1,  IFgetLayerPurposeNum);
  SIparse()->registerFunc("GetLayerAlias",          1,  IFgetLayerAlias);
  SIparse()->registerFunc("SetLayerAlias",          2,  IFsetLayerAlias);
  SIparse()->registerFunc("GetLayerDescr",          1,  IFgetLayerDescr);
  SIparse()->registerFunc("SetLayerDescr",          2,  IFsetLayerDescr);
  SIparse()->registerFunc("IsLayerDefined",         1,  IFisLayerDefined);
  SIparse()->registerFunc("IsLayerVisible",         1,  IFisLayerVisible);
  SIparse()->registerFunc("SetLayerVisible",        2,  IFsetLayerVisible);
  SIparse()->registerFunc("IsLayerSelectable",      1,  IFisLayerSelectable);
  SIparse()->registerFunc("SetLayerSelectable",     2,  IFsetLayerSelectable);
  SIparse()->registerFunc("IsLayerSymbolic",        1,  IFisLayerSymbolic);
  SIparse()->registerFunc("SetLayerSymbolic",       2,  IFsetLayerSymbolic);
  SIparse()->registerFunc("IsLayerNoMerge",         1,  IFisLayerNoMerge);
  SIparse()->registerFunc("SetLayerNoMerge",        2,  IFsetLayerNoMerge);
  SIparse()->registerFunc("GetLayerMinDimension",   1,  IFgetLayerMinDimension);
  SIparse()->registerFunc("GetLayerWireWidth",      1,  IFgetLayerWireWidth);
  SIparse()->registerFunc("AddLayerGdsOutMap",      3,  IFaddLayerGdsOutMap);
  SIparse()->registerFunc("RemoveLayerGdsOutMap",   3,  IFremoveLayerGdsOutMap);
  SIparse()->registerFunc("AddLayerGdsInMap",       2,  IFaddLayerGdsInMap);
  SIparse()->registerFunc("ClearLayerGdsInMap",     1,  IFclearLayerGdsInMap);
  SIparse()->registerFunc("SetLayerNoDRCdatatype",  2,  IFsetLayerNoDRCdatatype);

  // Layers - Extraction Support
  SIparse()->registerFunc("SetLayerExKeyword",      2,  IFsetLayerExKeyword);
  SIparse()->registerFunc("SetCurLayerExKeyword",   1,  IFsetCurLayerExKeyword);
  SIparse()->registerFunc("RemoveLayerExKeyword",   1,  IFremoveLayerExKeyword);
  SIparse()->registerFunc("RemoveCurLayerExKeyword",1,  IFremoveCurLayerExKeyword);
  SIparse()->registerFunc("IsLayerConductor",       1,  IFisLayerConductor);
  SIparse()->registerFunc("IsLayerRouting",         1,  IFisLayerRouting);
  SIparse()->registerFunc("IsLayerGround",          1,  IFisLayerGround);
  SIparse()->registerFunc("IsLayerContact",         1,  IFisLayerContact);
  SIparse()->registerFunc("IsLayerVia",             1,  IFisLayerVia);
  SIparse()->registerFunc("IsLayerViaCut",          1,  IFisLayerViaCut);
  SIparse()->registerFunc("IsLayerDielectric",      1,  IFisLayerDielectric);
  SIparse()->registerFunc("IsLayerDarkField",       1,  IFisLayerDarkField);
  SIparse()->registerFunc("GetLayerThickness",      1,  IFgetLayerThickness);
  SIparse()->registerFunc("GetLayerRho",            1,  IFgetLayerRho);
  SIparse()->registerFunc("GetLayerResis",          1,  IFgetLayerResis);
  SIparse()->registerFunc("GetLayerTau",            1,  IFgetLayerTau);
  SIparse()->registerFunc("GetLayerEps",            1,  IFgetLayerEps);
  SIparse()->registerFunc("GetLayerCap",            1,  IFgetLayerCap);
  SIparse()->registerFunc("GetLayerCapPerim",       1,  IFgetLayerCapPerim);
  SIparse()->registerFunc("GetLayerLambda",         1,  IFgetLayerLambda);

  // Selections
  SIparse()->registerFunc("SetLayerSpecific",       1,  IFsetLayerSpecific);
  SIparse()->registerFunc("SetLayerSearchUp",       1,  IFsetLayerSearchUp);
  SIparse()->registerFunc("SetSelectMode",          3,  IFsetSelectMode);
  SIparse()->registerFunc("SetSelectTypes",         1,  IFsetSelectTypes);
  SIparse()->registerFunc("Select",                 5,  IFselect);
  SIparse()->registerFunc("Deselect",               0,  IFdeselect);

  // Pseudo-Flat Generator Functions
  SIparse()->registerFunc("FlatObjList",            5,  IFflatObjList);
  SIparse()->registerFunc("FlatObjGen",             5,  IFflatObjGen);
  SIparse()->registerFunc("FlatObjGenLayers",       6,  IFflatObjGenLayers);
  SIparse()->registerFunc("FlatGenNext",            1,  IFflatGenNext);
  SIparse()->registerFunc("FlatGenCount",           1,  IFflatGenCount);
  SIparse()->registerFunc("FlatOverlapList",        4,  IFflatOverlapList);

  // Geometry Measurement
  SIparse()->registerFunc("Distance",               4,  IFdistance);
  SIparse()->registerFunc("MinDistPointToSeg",      7,  IFminDistPointToSeg);
  SIparse()->registerFunc("MinDistPointToObj",      4,  IFminDistPointToObj);
  SIparse()->registerFunc("MinDistSegToObj",        6,  IFminDistSegToObj);
  SIparse()->registerFunc("MinDistObjToObj",        3,  IFminDistObjToObj);
  SIparse()->registerFunc("MaxDistPointToObj",      4,  IFmaxDistPointToObj);
  SIparse()->registerFunc("MaxDistObjToObj",        3,  IFmaxDistObjToObj);
  SIparse()->registerFunc("Intersect",              3,  IFintersect);

#ifdef HAVE_PYTHON
  py_register_misc3();
#endif
#ifdef HAVE_TCL
  tcl_register_misc3();
#endif
}


// Handle a layer argument.  This can be an integer index number
// into the layer table, where the index is 1-based, and values
// less than 1 return the current layer.  The argument can also be
// a string, giving a layer name in name[:purpose] form, or an
// alias name.  If the string is null or empty, the current layer
// is returned.  This will always return a layer desc or fail,
// unless that flag is set which allows unresolved layer names.
//
bool
arg_layer(Variable *args, int ix, CDl **ldp, bool drvtoo, bool notfound_ok)
{
    if (args->type == TYP_SCALAR) {
        int indx;
        ARG_CHK(arg_int(args, ix, &indx))
        CDl *ld;
        if (indx < 1) {
            ld = LT()->CurLayer();
            if (!ld) {
                Errs()->add_error("no current layer");
                return (false);
            }
        }
        else {
            ld = CDldb()->layer(ix, DSP()->CurMode());
            if (!ld) {
                Errs()->add_error("no layer for index");
                return (false);
            }
        }
        *ldp = ld;
        return (true);
    }
    if (args->type == TYP_STRING) {
        const char *lstring;
        ARG_CHK(arg_string(args, ix, &lstring))
        CDl *ld;
        if (!lstring || !*lstring) {
            ld = LT()->CurLayer();
            if (!ld) {
                Errs()->add_error("no current layer");
                return (false);
            }
        }
        else {
            ld = CDldb()->findLayer(lstring, DSP()->CurMode());
            if (!ld && drvtoo)
                ld = CDldb()->findDerivedLayer(lstring);
            if (!ld && !notfound_ok) {
                Errs()->add_error("layer %s not found.", lstring);
                return (false);
            }
        }
        *ldp = ld;
        return (true);
    }
    Errs()->add_error("bad layer argument type");
    return (false);
}


//-------------------------------------------------------------------------
// Grid and Edge Snapping
//-------------------------------------------------------------------------

// (int) SetMfgGrid(mfg_grid)
//
// This will set the manufacturing grid to the value of the argument,
// provided that the value is in the range 0.0 - 100.0 microns.  When
// the manufacturing grid is nonzero, the snap grid is constrained to
// integer multiples of the manufacturing grid.  The function returns
// 1 if the argument is in range, in which case the value is accepted,
// 0 otherwise.
//
bool
misc3_funcs::IFsetMfgGrid(Variable *res, Variable *args, void*)
{
    double mfgg;
    ARG_CHK(arg_real(args, 0, &mfgg))

    res->type = TYP_SCALAR;
    res->content.value = Tech()->SetMfgGrid(mfgg);
    return (OK);
}


// (real) GetMfgGrid()
//
// This function returns the value of the manufacturing grid.  When
// nonzero, the snap grid is constrained to integer multiples of the
// manufacturing grid.
//
bool
misc3_funcs::IFgetMfgGrid(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = Tech()->MfgGrid();
    return (OK);
}


// (int) SetGrid(interval, snap, win)
//
// This function sets the grid parameters for the window indicated by
// the third argument, which is 0 for the main window or 1-4 for the
// sub-windows.  The interval argument sets snap grid spacing, in
// microns.  This value can be zero, in which case the present value
// is retained.
//
// The snap value is an integer in the range of -10 to 10.  If
// positive, the number provides the number of snap grid intervals
// between fine grid lines.  If negative, the absolute value is the
// number of fine grid lines displayed per snap grid interval.  If
// zero, the present setting is retained.
//
// For electrical mode windows, the snap points must be on multiples
// of one micron.  If not, this function returns 0 and the grid is
// unchanged.  The function also returns 0 if the window argument does
// not correspond to an existing window.  The return is 1 if the
// operation succeeds.
//
// The function does not redraw the window.  The Redraw() function
// can be called to redraw the window if necessary.
//
bool
misc3_funcs::IFsetGrid(Variable *res, Variable *args, void*)
{
    double resol;
    ARG_CHK(arg_real(args, 0, &resol))
    int snap;
    ARG_CHK(arg_int(args, 1, &snap))
    int win;
    ARG_CHK(arg_int(args, 2, &win))

    WindowDesc *wd = getwin(win);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (wd) {
        GridDesc *grd = wd->Attrib()->grid(wd->Mode());
        if (snap && snap >= -10 && snap <= 10)
            grd->set_snap(snap);
        grd->set_spacing(resol);
        grd->set_spacing(grd->spacing(wd->Mode()));  // fixup
        if (wd->Wbag())
            wd->Wbag()->PopUpGrid(0, MODE_UPD);
        res->content.value = 1;
    }
    return (OK);
}


// (int) GetGridInterval(win)
//
// This function returns the fine grid interval in microns for the
// grid in the window indicated by the argument, which is 0 for the
// main window or 1-4 for the sub-windows.  The function returns 0 if
// the argument does not correspond to an existing window.
//
bool
misc3_funcs::IFgetGridInterval(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))

    WindowDesc *wd = getwin(win);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (wd) {
        double spa = wd->Attrib()->grid(wd->Mode())->spacing(wd->Mode());
        int snap = wd->Attrib()->grid(wd->Mode())->snap();
        if (snap < 0)
            res->content.value = -spa/snap;
        else
            res->content.value = spa*snap;
    }
    return (OK);
}


// (int) GetSnapInterval(win)
//
// This function returns the snap grid interval in microns for the
// grid in the window indicated by the argument, which is 0 for the
// main window or 1-4 for the sub-windows.  The function returns 0 if
// the argument does not correspond to an existing window.
//
bool
misc3_funcs::IFgetSnapInterval(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))

    WindowDesc *wd = getwin(win);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (wd)
        res->content.value =
            wd->Attrib()->grid(wd->Mode())->spacing(wd->Mode());
    return (OK);
}


// (int) GetGridSnap(win)
//
// This function returns the snap number for the grid in the window
// specified by the argument, which is 0 for the main window or 1-4
// for the sub-windows.  The snap number determines the number of snap
// grid intervals between fine grid lines if positive, or fine grid
// lines per snap interval if negative.  The function returns 0 if the
// argument does not correspond to an existing window.
//
bool
misc3_funcs::IFgetGridSnap(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))

    WindowDesc *wd = getwin(win);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (wd)
        res->content.value = wd->Attrib()->grid(wd->Mode())->snap();
    return (OK);
}


// (real) ClipToGrid(coord, win)
//
// The first argument to this function is a coordinate in microns.
// The return value is the coordinate, in microns, snapped to the
// nearest snap point of the grid of the window given in the second
// argument.  The second argument is 0 for the main window, or 1-4 for
// the sub-windows.  The function fails if the window argument does
// not correspond to an existing window.
//
// Note that this function must be called twice for an x,y coordinate
// pair.  This function ignores the edge-snapping modes, only taking
// into account the grid resolution and snap values.
//
bool
misc3_funcs::IFclipToGrid(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 1, &win))

    WindowDesc *wd = getwin(win);
    if (!wd)
        return (BAD);

    int coord;
    ARG_CHK(arg_coord(args, 0, &coord, wd->Mode()))

    int dummy = 0;
    DSPattrib *a = wd->Attrib();
    EdgeSnapMode esm = a->edge_snapping();
    a->set_edge_snapping(EdgeSnapNone);
    wd->Snap(&coord, &dummy);
    a->set_edge_snapping(esm);

    res->type = TYP_SCALAR;
    if (wd->Mode() == Physical)
        res->content.value = MICRONS(coord);
    else
        res->content.value = ELEC_MICRONS(coord);
    return (OK);
}


// (int) SetEdgeSnappingMode(win, mode)
//
// Change the edge snapping mode in a drawing window.  The first
// argument is an integer representing the drawing window:  0 for the
// main window, and 1-4 for subwindows.  The change will apply only
// to that window, though changes in the main window will apply to
// new sub-windows.  The second argument is an integer in the range 0
// - 2.  The effects are
//
//   0  No edge snapping.
//   1  Edge snapping is enabled in some commands.
//   2  Edge snapping is always enabled.
//
// The return value is 1 if the window edge snapping was updated, 0
// otherwise.
//
bool
misc3_funcs::IFsetEdgeSnappingMode(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))
    int arg;
    ARG_CHK(arg_int(args, 1, &arg))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd && arg >= 0 && arg <= 2) {
        wd->Attrib()->set_edge_snapping((EdgeSnapMode)arg);
        if (wd->Wbag())
            wd->Wbag()->PopUpGrid(0, MODE_UPD);
        res->content.value = 1;
    }
    return (OK);
}


// (int) SetEdgeOffGrid(win, off_grid)
//
// This will enable snapping to off-grid locations when edge snapping
// is enabled, in the given window.  The first argument is an integer
// representing the drawing window:  0 for the main window, and 1-4
// for subwindows.  The second argument is a boolean which will allow
// off-grid snapping when true.  The return value is 1 if the window
// parameter was updated, 0 otherwise.
//
bool
misc3_funcs::IFsetEdgeOffGrid(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))
    bool arg;
    ARG_CHK(arg_boolean(args, 1, &arg))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd) {
        wd->Attrib()->set_edge_off_grid(arg);
        if (wd->Wbag())
            wd->Wbag()->PopUpGrid(0, MODE_UPD);
        res->content.value = 1;
    }
    return (OK);
}


// (int) SetEdgeNonManh(win, non_manh)
//
// This will enable snapping to non-Manhattan edges when edge
// snapping is enabled, in the given window.  The first argument is
// an integer representing the drawing window:  0 for the main
// window, and 1-4 for subwindows.  The second argument is a boolean
// which will allow snapping to non-Manhattan edges when true.  The
// return value is 1 if the window parameter was updated, 0
// otherwise.
//
bool
misc3_funcs::IFsetEdgeNonManh(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))
    bool arg;
    ARG_CHK(arg_boolean(args, 1, &arg))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd) {
        wd->Attrib()->set_edge_non_manh(arg);
        if (wd->Wbag())
            wd->Wbag()->PopUpGrid(0, MODE_UPD);
        res->content.value = 1;
    }
    return (OK);
}


// (int) SetEdgeWireEdge(win, wire_edge)
//
// This will enable snapping to wire edges when edge snapping is
// enabled, in the given window.  The first argument is an integer
// representing the drawing window:  0 for the main window, and 1-4
// for subwindows.  The second argument is a boolean which will allow
// snapping to wire edges when true.  The return value is 1 if the
// window parameter was updated, 0 otherwise.
//
bool
misc3_funcs::IFsetEdgeWireEdge(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))
    bool arg;
    ARG_CHK(arg_boolean(args, 1, &arg))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd) {
        wd->Attrib()->set_edge_wire_edge(arg);
        if (wd->Wbag())
            wd->Wbag()->PopUpGrid(0, MODE_UPD);
        res->content.value = 1;
    }
    return (OK);
}


// (int) SetEdgeWirePath(win, wire_path)
//
// This will enable snapping to the wire path when edge snapping is
// enabled, in the given window.  The path is the set of line
// segments that invisibly run along the center of the displayed
// wire, which, along with the wire width and end style, actually
// defines the wire.  The first argument is an integer representing
// the drawing window:  0 for the main window, and 1-4 for
// subwindows.  The second argument is a boolean which will allow
// snapping to the wire path when true.  The return value is 1 if the
// window parameter was updated, 0 otherwise.
//
bool
misc3_funcs::IFsetEdgeWirePath(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))
    bool arg;
    ARG_CHK(arg_boolean(args, 1, &arg))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd) {
        wd->Attrib()->set_edge_wire_path(arg);
        if (wd->Wbag())
            wd->Wbag()->PopUpGrid(0, MODE_UPD);
        res->content.value = 1;
    }
    return (OK);
}


// (int) GetEdgeSnappingMode(win)
//
// This function returns the edge snapping mode in effect for the
// given window.  The argument is an integer representing the drawing
// window:  0 for the main window, and 1-4 for subwindows.  The
// return value is -1 if the window is not found, 0-2 otherwise.
//
//   0  No edge snapping.
//   1  Edge snapping is enabled in some commands.
//   2  Edge snapping is always enabled.
//
//
bool
misc3_funcs::IFgetEdgeSnappingMode(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))

    res->type = TYP_SCALAR;
    res->content.value = -1;
    WindowDesc *wd = getwin(win);
    if (wd)
        res->content.value = wd->Attrib()->edge_snapping();
    return (OK);
}


// (int) GetEdgeOffGrid(win)
//
// This returns the setting of the allow off-grid edge snapping flag
// for the given window.  The argument is an integer representing the
// drawing window:  0 for the main window, and 1-4 for subwindows. 
// The return value is -1 if the window is not found, 0 or 1 otherwise
// tracking the state of the flag.
//
bool
misc3_funcs::IFgetEdgeOffGrid(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))

    res->type = TYP_SCALAR;
    res->content.value = -1;
    WindowDesc *wd = getwin(win);
    if (wd)
        res->content.value = wd->Attrib()->edge_off_grid();
    return (OK);
}


// (int) GetEdgeNonManh(win)
//
// This returns the setting of the allow non-Manhattan edge snapping
// flag for the given window.  The argument is an integer
// representing the drawing window:  0 for the main window, and 1-4
// for subwindows.  The return value is -1 if the window is not
// found, 0 or 1 otherwise tracking the state of the flag.
//
bool
misc3_funcs::IFgetEdgeNonManh(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))

    res->type = TYP_SCALAR;
    res->content.value = -1;
    WindowDesc *wd = getwin(win);
    if (wd)
        res->content.value = wd->Attrib()->edge_non_manh();
    return (OK);
}


// (int) GetEdgeWireEdge(win)
//
// This returns the setting of the allow wire-edge edge snapping flag
// for the given window.  The argument is an integer representing the
// drawing window:  0 for the main window, and 1-4 for subwindows. 
// The return value is -1 if the window is not found, 0 or 1 otherwise
// tracking the state of the flag.
//
bool
misc3_funcs::IFgetEdgeWireEdge(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))

    res->type = TYP_SCALAR;
    res->content.value = -1;
    WindowDesc *wd = getwin(win);
    if (wd)
        res->content.value = wd->Attrib()->edge_wire_edge();
    return (OK);
}


// (int) GetEdgeWirePath(win)
//
// This returns the setting of the allow wire-path edge snapping flag
// for the given window.  The argument is an integer representing the
// drawing window:  0 for the main window, and 1-4 for subwindows. 
// The return value is -1 if the window is not found, 0 or 1 otherwise
// tracking the state of the flag.
//
bool
misc3_funcs::IFgetEdgeWirePath(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))

    res->type = TYP_SCALAR;
    res->content.value = -1;
    WindowDesc *wd = getwin(win);
    if (wd)
        res->content.value = wd->Attrib()->edge_wire_path();
    return (OK);
}


// (int) SetRulerSnapToGrid(snap)
//
// This function sets the snap-to-grid behavior when creating rulers
// in the Rulers command.  When set, the mouse cursor will snap to
// grid locations, otherwise not.  In either case the cursor may snap
// to object edges if edge snapping is enabled.  If the ruler command
// is active the mode will change immediately, otherwise the new mode
// will apply when the command becomes active.  The return value is
// 0 or 1 representing the previous flag value.
//
bool
misc3_funcs::IFsetRulerSnapToGrid(Variable *res, Variable *args, void*)
{
    bool arg;
    ARG_CHK(arg_boolean(args, 0, &arg))

    res->type = TYP_SCALAR;
    bool osnap;
    DSP()->RulerGetSnapDefaults(0, &osnap, false);
    res->content.value = osnap;
    DSP()->RulerSetSnapDefaults(0, &arg);
    return (OK);
}


// (int) SetRulerEdgeSnappingMode(mode)
//
// This sets the edge snapping mode which is applied during the
// Rulers command.  This command has its own default edge snapping
// state.  This function changes only the initial state when the
// command starts, and will have no effect in a running command (use
// SetEdgeSnappingMode to alter the current setting).  The argument
// is an integer 0-2.
//
//   0  No edge snapping.
//   1  Edge snapping is enabled in some commands.
//   2  Edge snapping is always enabled.
//
// The function returns -1 if the argument is out of range, or 0-2
// representing the previous state otherwise.
//
bool
misc3_funcs::IFsetRulerEdgeSnappingMode(Variable *res, Variable *args, void*)
{
    int arg;
    ARG_CHK(arg_int(args, 0, &arg))

    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (arg >= 0 && arg <= 2) {
        DSPattrib a;
        DSP()->RulerGetSnapDefaults(&a, 0, false);
        res->content.value = a.edge_snapping();
        a.set_edge_snapping((EdgeSnapMode)arg);
        DSP()->RulerSetSnapDefaults(&a, 0);
    }
    return (OK);
}


// (int) SetRulerEdgeOffGrid(off_grid)
//
// This sets the edge snapping allow off-grid flag which is applied
// during the Rulers command.  This command has its own default edge
// snapping state.  This function changes only the initial state when
// the command starts, and will have no effect in a running command
// (use SetEdgeOffGrid to alter the current setting).  The argument
// is a boolean value which enables the flag when true.
//
// The return value is 0 or 1 representing the previous flag state.
//
bool
misc3_funcs::IFsetRulerEdgeOffGrid(Variable *res, Variable *args, void*)
{
    bool arg;
    ARG_CHK(arg_boolean(args, 0, &arg))

    res->type = TYP_SCALAR;
    DSPattrib a;
    DSP()->RulerGetSnapDefaults(&a, 0, false);
    res->content.value = a.edge_off_grid();
    a.set_edge_off_grid(arg);
    DSP()->RulerSetSnapDefaults(&a, 0);
    return (OK);
}


// (int) SetRulerEdgeNonManh(non_manh)
//
// This sets the edge snapping allow non-Manhattan flag which is
// applied during the Rulers command.  This command has its own
// default edge snapping state.  This function changes only the
// initial state when the command starts, and will have no effect in
// a running command (use SetEdgeNonManh to alter the current
// setting).  The argument is a boolean value which enables the flag
// when true.
//
// The return value is 0 or 1 representing the previous flag state.
//
bool
misc3_funcs::IFsetRulerEdgeNonManh(Variable *res, Variable *args, void*)
{
    bool arg;
    ARG_CHK(arg_boolean(args, 0, &arg))

    res->type = TYP_SCALAR;
    DSPattrib a;
    DSP()->RulerGetSnapDefaults(&a, 0, false);
    res->content.value = a.edge_non_manh();
    a.set_edge_non_manh(arg);
    DSP()->RulerSetSnapDefaults(&a, 0);
    return (OK);
}


// (int) SetRulerEdgeWireEdge(wire_edge)
//
// This sets the edge snapping allow wire-edge flag which is applied
// during the Rulers command.  This command has its own default edge
// snapping state.  This function changes only the initial state when
// the command starts, and will have no effect in a running command
// (use SetEdgeWireEdge to alter the current setting).  The argument
// is a boolean value which enables the flag when true.
//
// The return value is 0 or 1 representing the previous flag state.
//
bool
misc3_funcs::IFsetRulerEdgeWireEdge(Variable *res, Variable *args, void*)
{
    bool arg;
    ARG_CHK(arg_boolean(args, 0, &arg))

    res->type = TYP_SCALAR;
    DSPattrib a;
    DSP()->RulerGetSnapDefaults(&a, 0, false);
    res->content.value = a.edge_wire_edge();
    a.set_edge_wire_edge(arg);
    DSP()->RulerSetSnapDefaults(&a, 0);
    return (OK);
}


// (int) SetRulerEdgeWirePath(wire_path)
//
// This sets the edge snapping allow wire-path flag which is applied
// during the Rulers command.  This command has its own default edge
// snapping state.  This function changes only the initial state when
// the command starts, and will have no effect in a running command
// (use SetEdgeWirePath to alter the current setting).  The argument
// is a boolean value which enables the flag when true.
//
// The return value is 0 or 1 representing the previous flag state.
//
bool
misc3_funcs::IFsetRulerEdgeWirePath(Variable *res, Variable *args, void*)
{
    bool arg;
    ARG_CHK(arg_boolean(args, 0, &arg))

    res->type = TYP_SCALAR;
    DSPattrib a;
    DSP()->RulerGetSnapDefaults(&a, 0, false);
    res->content.value = a.edge_wire_path();
    a.set_edge_wire_path(arg);
    DSP()->RulerSetSnapDefaults(&a, 0);
    return (OK);
}


// (int) GetRulerSnapToGrid()
//
// This returns the present default snap-to-grid state used during
// the Rulers command.  The values are 0 or 1 depending on the state.
//
bool
misc3_funcs::IFgetRulerSnapToGrid(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    bool osnap;
    DSP()->RulerGetSnapDefaults(0, &osnap, false);
    res->content.value = osnap;
    return (OK);
}


// (int) GetRulerEdgeSnappingMode()
//
// The return value is an integer 0-2 representing the default edge
// snapping mode to use during the Rulers command.
//
//   0  No edge snapping.
//   1  Edge snapping is enabled in some commands.
//   2  Edge snapping is always enabled.
//
bool
misc3_funcs::IFgetRulerEdgeSnappingMode(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    DSPattrib a;
    DSP()->RulerGetSnapDefaults(&a, 0, false);
    res->content.value = a.edge_snapping();
    return (OK);
}


// (int) GetRulerEdgeOffGrid()
//
// The return value is 0 or 1 depending on the setting of the edge
// snapping allow off-grid flag which is the default in the Rulers
// command.
//
bool
misc3_funcs::IFgetRulerEdgeOffGrid(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    DSPattrib a;
    DSP()->RulerGetSnapDefaults(&a, 0, false);
    res->content.value = a.edge_off_grid();
    return (OK);
}


// (int) GetRulerEdgeNonManh()
//
// The return value is 0 or 1 depending on the setting of the edge
// snapping allow non-Manhattan flag which is the default in the
// Rulers command.
//
bool
misc3_funcs::IFgetRulerEdgeNonManh(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    DSPattrib a;
    DSP()->RulerGetSnapDefaults(&a, 0, false);
    res->content.value = a.edge_non_manh();
    return (OK);
}


// (int) GetRulerEdgeWireEdge()
//
// The return value is 0 or 1 depending on the setting of the edge
// snapping allow wire-edge flag which is the default in the
// Rulers command.
//
bool
misc3_funcs::IFgetRulerEdgeWireEdge(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    DSPattrib a;
    DSP()->RulerGetSnapDefaults(&a, 0, false);
    res->content.value = a.edge_wire_edge();
    return (OK);
}


// (int) GetRulerEdgeWirePath()
//
// The return value is 0 or 1 depending on the setting of the edge
// snapping allow wire-path flag which is the default in the
// Rulers command.
//
bool
misc3_funcs::IFgetRulerEdgeWirePath(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    DSPattrib a;
    DSP()->RulerGetSnapDefaults(&a, 0, false);
    res->content.value = a.edge_wire_path();
    return (OK);
}


//-------------------------------------------------------------------------
// Grid
//-------------------------------------------------------------------------

// (int) ShowGrid(on, win)
//
// This function sets whether or not the grid is shown in a window.
// If the first argument is nonzero, the grid will be shown, otherwise
// the grid will not be shown.  The second argument is an integer
// representing the drawing window:  0 for the main window, and 1-4
// for subwindows.  The change will not be visible until the window is
// redrawn (one can call Redraw()).  If success, 1 is returned, or 0
// is returned if the window does not exist.
//
bool
misc3_funcs::IFshowGrid(Variable *res, Variable *args, void*)
{
    bool show;
    ARG_CHK(arg_boolean(args, 0, &show))
    int win;
    ARG_CHK(arg_int(args, 1, &win))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd) {
        wd->Attrib()->grid(wd->Mode())->set_displayed(show);
        if (wd->Wbag())
            wd->Wbag()->PopUpGrid(0, MODE_UPD);
        res->content.value = 1;
    }
    return (OK);
}


// (int) ShowAxes(style, win)
//
// This function sets the axes presentation style in physical mode
// windows.  the first argument is an integer 0-2, where 0 suppresses
// drawing of axes, 1 indicates plain axes, and 2 (or anything else)
// indicates axes with a box at the origin.  The second argument is an
// integer representing the drawing window:  0 for the main window,
// 1-4 for subwindows.  Axes are never shown in electrical mode
// windows.  On success, 1 is returned.  If the window does not exist
// or is not showing a physical view, 0 is returned.  The change will
// not be visible until the window is redrawn (one can call Redraw()).
//
bool
misc3_funcs::IFshowAxes(Variable *res, Variable *args, void*)
{
    int style;
    ARG_CHK(arg_int(args, 0, &style))
    int win;
    ARG_CHK(arg_int(args, 1, &win))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd && wd->Mode() == Physical) {
        if (style == 0)
            wd->Attrib()->grid(wd->Mode())->set_axes(AxesNone);
        else if (style == 1)
            wd->Attrib()->grid(wd->Mode())->set_axes(AxesPlain);
        else
            wd->Attrib()->grid(wd->Mode())->set_axes(AxesMark);
        if (wd->Wbag())
            wd->Wbag()->PopUpGrid(0, MODE_UPD);
        res->content.value = 1;
    }
    return (OK);
}


// (int) SetGridStyle(style, win)
//
// This function sets the line style used for grid rendering.  The
// first argument is an integer mask that defines the on-off pattern.
// The pattern starts at the most significant '1' bit and continues
// through the least significant bit, and repeats.  Set bits are
// rendered as the visible part of the pattern.  If the style is 0, a
// dot is shown at each grid point.  Passing -1 will give continuous
// lines.  The second argument is an integer representing the drawing
// window:  0 for the main window, 1-4 for subwindows.  The function
// returns 1 on success, 0 if the window does not exist.  The change
// will not be visible until the window is redrawn (one can call
// Redraw()).
//
bool
misc3_funcs::IFsetGridStyle(Variable *res, Variable *args, void*)
{
    int style;
    ARG_CHK(arg_int(args, 0, &style))
    int win;
    ARG_CHK(arg_int(args, 1, &win))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd) {
        wd->Wdraw()->defineLinestyle(&wd->Attrib()->grid(
            wd->Mode())->linestyle(), style);
        if (wd->Wbag())
            wd->Wbag()->PopUpGrid(0, MODE_UPD);
        res->content.value = 1;
    }
    return (OK);
}


// (int) GetGridStyle(win)
//
// This function returns the line style mask used for rendering the
// grid in the given window.  The mask has the interpretation
// described in the description of SetGridStyle().  The argument is an
// integer representing the window:  0 for the main window, and 1-4
// for subwindows.  If the window does not exist, 0 is returned.
//
bool
misc3_funcs::IFgetGridStyle(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd)
        res->content.value =
            wd->Attrib()->grid(wd->Mode())->linestyle().mask;
    return (OK);
}


// (int) SetGridCrossSize(xsize, win)
//
// This applies only to grids with style 0 (dot grid).  The xsize is
// an integer 0-6 which indicates tne number of pixels to draw in the
// four compass directions around the central pixel.  Thus, for
// nonzero values, the "dot" is rendered as a small cross.  The second
// argument is an integer representing the drawing window:  0 for the
// main window, 1-4 for subwindows.  The function returns 1 on
// success, 0 if the window does not exist or the style is nonzero. 
// The change will not be visible until the window is redrawn (one can
// call Redraw()).
//
bool
misc3_funcs::IFsetGridCrossSize(Variable *res, Variable *args, void*)
{
    int csz;
    ARG_CHK(arg_int(args, 0, &csz))
    int win;
    ARG_CHK(arg_int(args, 1, &win))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd && wd->Attrib()->grid(wd->Mode())->linestyle().mask == 0) {
        if (csz < 0)
            csz = 0;
        else if (csz > 6)
            csz = 6;
        wd->Attrib()->grid(wd->Mode())->set_dotsize(csz);
        if (wd->Wbag())
            wd->Wbag()->PopUpGrid(0, MODE_UPD);
        res->content.value = 1;
    }
    return (OK);
}


// (int) GetGridCrossSize(win)
//
// This returns an integer 0-6, which will be nonzero only for grid
// style 0 (dot grid), and if the "dots" are being rendered as small
// crosses via a call to SetGridCrossSize or otherwise.  The argument
// is an integer representing the window:  0 for the main window, and
// 1-4 for subwindows.  If the window does not exist, 0 is returned.
//
bool
misc3_funcs::IFgetGridCrossSize(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd && wd->Attrib()->grid(wd->Mode())->linestyle().mask == 0) {
        res->content.value =
            wd->Attrib()->grid(wd->Mode())->dotsize();
    }
    return (OK);
}


// (int) SetGridOnTop(ontop, win)
//
// This function sets whether the grid is shown above or below
// rendered objects.  If the first argument is nonzero, the grid will
// be shown above rendered objects.  The second argument is an integer
// representing the drawing window:  0 for the main window and 1-4 for
// subwindows.  The function returns 1 on success, 0 if the window
// does not exist.  The change will not be visible until the window is
// redrawn (one can call Redraw()).
//
bool
misc3_funcs::IFsetGridOnTop(Variable *res, Variable *args, void*)
{
    bool ontop;
    ARG_CHK(arg_boolean(args, 0, &ontop))
    int win;
    ARG_CHK(arg_int(args, 1, &win))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd) {
        wd->Attrib()->grid(wd->Mode())->set_show_on_top(ontop);
        if (wd->Wbag())
            wd->Wbag()->PopUpGrid(0, MODE_UPD);
        res->content.value = 1;
    }
    return (OK);
}


// (int) GetGridOnTop(win)
//
// This function returns 1 is the grid is shown on top of objects.
// The argument is an integer representing the drawing window:  0 for
// the main window and 1-4 for subwindows.  If the grid is shown below
// rendered objects, 0 is returned.  If the window does not exist, -1
// is returned.
//
bool
misc3_funcs::IFgetGridOnTop(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd)
        res->content.value = wd->Attrib()->grid(wd->Mode())->show_on_top();
    else
        res->content.value = -1;
    return (OK);
}


// (int) SetGridCoarseMult(mult, win)
//
// This sets the number of fine grid lines per coarse grid line.  The
// first argument is an integer 1-50 that provides this multiple (it
// is clipped to this range).  If 1, the coarse grid color is used for
// all grid lines.  The second argument represents the drawing window
// whose grid is being changed, 0 for the main drawing window, and 1-4
// for sub-windows.  The change will not be visible until the window
// is redrawn (one can call Redraw()).
//
// The return value is 1 on success, 0 if the window does not exist.
//
bool
misc3_funcs::IFsetGridCoarseMult(Variable *res, Variable *args, void*)
{
    int mult;
    ARG_CHK(arg_int(args, 0, &mult))
    int win;
    ARG_CHK(arg_int(args, 1, &win))

    if (mult < 1)
        mult = 1;
    else if (mult > 50)
        mult = 50;
    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd) {
        wd->Attrib()->grid(wd->Mode())->set_coarse_mult(mult);
        res->content.value = 1;
    }
    return (OK);
}


// (int) GetGridCoarseMult(win)
//
// This returns the number of fine grid lines per coarse grid
// interval, as being used in the drawing window indicated by the
// argument.  The argument is 0 for the main drawing window, 1-4 for
// sub-windows.  If the window does not exist, zero is returned.
//
bool
misc3_funcs::IFgetGridCoarseMult(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (wd)
        res->content.value = wd->Attrib()->grid(wd->Mode())->coarse_mult();
    return (OK);
}


// (int) SaveGrid(regnum, win)
//
// This will save a grid parameter set to a register.  The first
// argument is a register index value 0-7.  Register 0 is used
// internally for the "last" value whenever grid parameters are
// changed, so is probably not a good choice unless this behavior is
// expected.  These are the same registers as used with the Grid
// Parameters panel, and are associated with the PhysGridReg and
// ElecGridReg keyword families in the technology file.
//
// The second argument represents the drawing window whose grid
// parameters are to be saved.  The value is 0 for the main drawing
// window, and 1-4 for sub-windows.  Note that separate registers
// exist for electrical and physical mode, so register numbers can be
// reused in the two modes.
//
// The return value is 1 on success, 0 if the indicated window does
// not exist, or the register value is out of range.
//
bool
misc3_funcs::IFsaveGrid(Variable *res, Variable *args, void*)
{
    int reg;
    ARG_CHK(arg_int(args, 0, &reg))
    int win;
    ARG_CHK(arg_int(args, 1, &win))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (reg >= 0 && reg <= 7 && wd) {
        Tech()->SetGridReg(reg, *wd->Attrib()->grid(wd->Mode()), wd->Mode());
        res->content.value = 1;
    }
    return (OK);
}


// (int) RecallGrid(regnum, win)
//
// This will recall a grid parameter set from a register, and update
// the grid of a drawing window.  The first argument is a register
// index value 0-7.  Register 0 is used internally for the "last"
// value whenever grid parameters are changed, so is probably not a
// good choice unless this behavior is expected.  These are the same
// registers as used with the Grid Parameters panel, and are
// associated with the PhysGridReg and ElecGridReg keyword families in
// the technology file.
//
// The second argument represents the drawing window whose grid
// parameters are to be saved.  The value is 0 for the main drawing
// window, and 1-4 for sub-windows.  Note that separate registers
// exist for electrical and physical mode, so register numbers can be
// reused in the two modes.
//
// The return value is 1 on success, 0 if the indicated window does
// not exist.  The change will not be visible until the window is
// redrawn (one can call Redraw()).
//
bool
misc3_funcs::IFrecallGrid(Variable *res, Variable *args, void*)
{
    int reg;
    ARG_CHK(arg_int(args, 0, &reg))
    int win;
    ARG_CHK(arg_int(args, 1, &win))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wd = getwin(win);
    if (reg >= 0 && reg <= 7 && wd) {
        wd->Attrib()->grid(wd->Mode())->set(*Tech()->GridReg(reg, wd->Mode()));
        res->content.value = 1;
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Current Layer
//-------------------------------------------------------------------------

// Standard Layer Argument
// 
// This can be an integer index number into the layer table, where the
// index is 1-based, and values less than 1 return the current layer. 
// The argument can also be a string, giving a layer name in
// name[:purpose] form, or an alias name.  If the string is null or
// empty, the current layer is returned.  Unless stated otherwise, if
// the layer is not found, a fatal error results.


// (string) GetCurLayer()
//
// This function returns a string containing the name of the current
// layer.  If no current layer is defined, a null string is returned.
//
bool
misc3_funcs::IFgetCurLayer(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string = 0;
    if (!LT()->CurLayer())
        return (OK);
    res->content.string = lstring::copy(LT()->CurLayer()->name());
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) GetCurLayerIndex()
//
// This function returns the 1-based index of the current layer in the
// layer table.  If no current layer is defined, 0 is returned.
//
bool
misc3_funcs::IFgetCurLayerIndex(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (!LT()->CurLayer())
        return (OK);
    res->content.value = LT()->CurLayer()->index(DSP()->CurMode());
    return (OK);
}


// (int) SetCurLayer(stdlyr)
// 
// This function sets the current layer as indicated by the standard
// layer argument.  The return value is the 1-based index of the
// previous current layer in the layer table, or 0 if there was no
// current layer.  This return can be passed as the argument to revert
// to the previous current layer.
//
bool
misc3_funcs::IFsetCurLayer(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld))

    res->type = TYP_SCALAR;
    res->content.value =
        LT()->CurLayer() ? LT()->CurLayer()->index(DSP()->CurMode()) : 0;
    LT()->SetCurLayer(ld);
    return (OK);
}


// (int) SetCurLayerFast(stdlyr)
//
// This is like GetCurLayer, but there is no visible update, i.e., the
// layer table indication, and the current layer shown in various
// pop-ups, is unchanged.  This is for speed when drawing.  When
// drawing is finished, this should be called with the original
// current layer, or SetCurLayer should be called with some layer. 
// The return value is the 1-based index of the previous current layer
// in the layer table, or 0 if there was no current layer.  This
// return can be passed as the argument to revert to the previous
// current layer.
//
bool
misc3_funcs::IFsetCurLayerFast(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld))

    res->type = TYP_SCALAR;
    res->content.value =
        LT()->CurLayer() ? LT()->CurLayer()->index(DSP()->CurMode()) : 0;
    LT()->ProvisionallySetCurLayer(ld);
    return (OK);
}


// (int) NewCurLayer(stdlyr)
//
// If the standard layer argument matches an existing layer, the
// current layer is set to that layer.  Otherwise, a new layer is
// created, if possible, and the current layer is set to the new
// layer.  The function will fail if it is not possible to create a
// new layer, for example if the name is not a valid layer name.
//
// If the name is not in the layer:purpose form, any new layer created
// will use the default "drawing" purpose.
//
// The return value is the 1-based index of the previous current layer
// in the layer table, or 0 if there was no current layer.  This
// return can be passed as the argument to revert to the previous
// current layer.
//
bool
misc3_funcs::IFnewCurLayer(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, false, true))
    if (!ld) {
        const char *lname;
        ARG_CHK(arg_string(args, 0, &lname))
        ld = CDldb()->newLayer(lname, DSP()->CurMode());
    }
    if (!ld)
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value =
        LT()->CurLayer() ? LT()->CurLayer()->index(DSP()->CurMode()) : 0;
    LT()->SetCurLayer(ld);
    return (OK);
}


// (string) GetCurLayerAlias()
//
// This function is deprecated, see GetLayerAlias.
// Return the alias name of the current layer, or a null string if
// there is no alias.
//
bool
misc3_funcs::IFgetCurLayerAlias(Variable *res, Variable*, void*)
{
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return (BAD);

    res->type = TYP_STRING;
    res->content.string = lstring::copy(ld->lppName());
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) SetCurLayerAlias(alias)
//
// This function is deprecated, see SetLayerAlias.
// Set the alias name of the current layer.  Returns 1 on success, 0
// otherwise (possibly indicating a name clash).
//
bool
misc3_funcs::IFsetCurLayerAlias(Variable *res, Variable *args, void*)
{
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return (BAD);
    const char *alias;
    ARG_CHK(arg_string(args, 0, &alias))

    res->type = TYP_SCALAR;
    res->content.value = CDldb()->setLPPname(ld, alias);
    return (OK);
}


// (string) GetCurLayerDescr()
//
// This function is deprecated, see GetLayerDescr.
// Return the description string of the current layer.  This will be
// null if no description has been set.
//
bool
misc3_funcs::IFgetCurLayerDescr(Variable *res, Variable*, void*)
{
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return (BAD);

    res->type = TYP_STRING;
    res->content.string = lstring::copy(ld->description());
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) SetCurLayerDescr(descr)
//
// This function is deprecated, see SetLayerDescr.
// Set the description string of the current layer.  The return value
// is always 1.
//
bool
misc3_funcs::IFsetCurLayerDescr(Variable *res, Variable *args, void*)
{
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return (BAD);
    const char *descr;
    ARG_CHK(arg_string(args, 0, &descr))

    res->type = TYP_SCALAR;
    res->content.value = 1.0;
    ld->setDescription(descr);
    return (OK);
}


//-------------------------------------------------------------------------
// Layer Table
//-------------------------------------------------------------------------

// (int) LayersUsed()
// This returns a count of the layers in the layer table for the
// current display mode.
//
bool
misc3_funcs::IFlayersUsed(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = CDldb()->layersUsed(DSP()->CurMode());
    return (OK);
}


// (int) AddLayer(name, index)
//
// This adds the named layer to the layer table, in the position
// specified by the integer second argument.  If the second argument
// is negative, the new layer will be added at the end, above all
// existing layers.  If the index is 0, the new layer will be
// positioned at the index of the current layer, and the current layer
// and those above moved up.  Otherwise, the index is a 1-based index
// into the layer table, where the new layer will be inserted.  The
// layer at that index and those above will be moved up.
//
// The name can match the name of an existing layer that has been
// removed from the layer table.  It can also be a unique new name,
// and a new layer will be created.  If the name matches an existing
// layer in the table, a new layer will also be created, but with an
// internally generated name.
//
// The function will return 0 if it is not possible to create a new
// layer, for example if the name is not a valid layer name.  On
// success 1 is returned.
//
// If the name is not in the layer:purpose form, any new layer created
// will use the default "drawing" purpose.
//
bool
misc3_funcs::IFaddLayer(Variable *res, Variable *args, void*)
{
    const char *lname;
    ARG_CHK(arg_string(args, 0, &lname))
    int ix;
    ARG_CHK(arg_int(args, 1, &ix))

    if (!lname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = LT()->AddLayer(lname, ix);
    return (OK);
}


// (int) RemoveLayer(stdlyr)
//
// This removes the layer indicated by the standard layer argument
// from the layer table if found.  This returns 1 if the layer is
// found and removed, 0 otherwise.
//
bool
misc3_funcs::IFremoveLayer(Variable *res, Variable *args, void*)
{
    const char *lname;
    ARG_CHK(arg_string(args, 0, &lname))

    if (!lname)
        return (BAD);

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    const char *s = LT()->RemoveLayer(lname, DSP()->CurMode());
    if (s) {
        PL()->ShowPrompt(s);
        res->content.value = 0;
    }
    else
        res->content.value = 1;
    res->type = TYP_SCALAR;
    return (OK);
}


// (int) RenameLayer(oldname, newname)
//
// The oldname is a standard layer argument.  The newname is a string
// providing a new layer/purpose name in the layer[:purpose] form.  If
// no purpose field is given, the default "drawing" purpose is
// assumed.  This renames the layer specified in oldname to newname. 
// The renamed layer will have any alias name removed.
//
// This fails if oldname is unresolved or newname is null, and returns
// 0 on error, with an error message available from GetError.
//
bool
misc3_funcs::IFrenameLayer(Variable *res, Variable *args, void*)
{
    CDl *oldld;
    ARG_CHK(arg_layer(args, 0, &oldld))

    const char *newname;
    ARG_CHK(arg_string(args, 1, &newname))
    if (!newname)
        return (BAD);

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    res->type = TYP_SCALAR;
    res->content.value = LT()->RenameLayer(oldld->name(), newname);
    return (OK);
}


// (stringlist_handle) LayerHandle(down)
//
// This function returns a handle to a list of the layer names from
// the layer table.  If the argument is 0, the list is in ascending
// order.  If the argument is nonzero, the list is in descending
// order.  The layers used in the current display mode are listed.
//
bool
misc3_funcs::IFlayerHandle(Variable *res, Variable *args, void*)
{
    bool dec;
    ARG_CHK(arg_boolean(args, 0, &dec))

    stringlist *s0 = 0;
    CDl *ld;
    CDlgen lgen(DSP()->CurMode());
    while ((ld = lgen.next()) != 0)
        s0 = new stringlist(lstring::copy(ld->name()), s0);
    if (!dec)
        stringlist::reverse(s0);
    sHdl *hdl = new sHdlString(s0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (string) GenLayers(stringlist_handle)
//
// This function returns a string containing a layer name from the
// layer table.  The argument is the handle returned by LayerHandle. 
// A different layer is returned for each call.  The null string is
// returned after all layers have been cycled through.  This is
// equivalent to ListNext.
//
bool
misc3_funcs::IFgenLayers(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLstring)
            return (BAD);
        res->content.string = (char*)hdl->iterator();
        if (res->content.string)
            res->flags |= VF_ORIGINAL;
    }
    return (OK);
}


// (stringlist_handle) GetLayerPalette(regnum)
//
// The argument is an integer 0-7 corresponding to a layer palette
// register, as used with the Layer Palette panel, and associated with
// the PhysLayerPalette and ElecLayerPalette technology file keyword
// families.  The return value is a stringlist handle, where the
// strings are the names of layers saved in the indexed palette
// register corresponding to the display mode of the main drawing
// window.
//
// If the palette register is empty, or the argument is out of range,
// a scalar 0 is returned.
//
// The register with index 0 is used internally to save the last Layer
// Palette user area before it pops down.  Thus, this index should not
// be used unless this behavior is expected.
//
bool
misc3_funcs::IFgetLayerPalette(Variable *res, Variable *args, void*)
{
    int reg;
    ARG_CHK(arg_int(args, 0, &reg))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (reg < 0 || reg >= TECH_NUM_PALETTES)
        return (OK);
    const char *ll = Tech()->LayerPaletteReg(reg, DSP()->CurMode());
    if (!ll)
        return (OK);

    stringlist *s0 = 0, *se = 0;
    char *tok;
    while ((tok = lstring::gettok(&ll)) != 0) {
        if (!s0)
            s0 = se = new stringlist(tok, 0);
        else {
            se->next = new stringlist(tok, 0);
            se = se->next;
        }
    }
    sHdl *hdl = new sHdlString(s0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (int) SetLayerPalette(list, regnum)
//
// The second argument is an integer 0-7 corresponding to a layer palette
// register, as used with the Layer Palette panel, and associated with
// the PhysLayerPalette and ElecLayerPalette technology file keyword
// families.
//
// The first argument provides a list of layers, or null, to be saved
// in the indexed palette register corresponding to the display mode
// of the main drawing window.  If the argument is a scalar 0, or a
// null string, the palette register will be cleared.  Otherwise this
// argument can be a string consisting of space-separated layer names,
// or a stringlist handle, where the strings are layer names.  The
// handle is unaffected by this function call.
//
// The function returns 1 on success, 0 if the register index is out
// of range.  The call will fail (halt the script) if a bad argument
// is passed.
//
// There is no checking of the validity of the string saved as palette
// register data.
//
bool
misc3_funcs::IFsetLayerPalette(Variable *res, Variable *args, void*)
{
    int reg;
    ARG_CHK(arg_int(args, 1, &reg))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (reg < 0 || reg >= TECH_NUM_PALETTES)
        return (OK);
    if (args[0].type == TYP_SCALAR) {
        if (args[0].content.value != 0.0)
            return (BAD);
        Tech()->SetLayerPaletteReg(reg, DSP()->CurMode(), 0);
        res->content.value = 1;
        return (OK);
    }
    if (args[0].type == TYP_STRING) {
        Tech()->SetLayerPaletteReg(reg, DSP()->CurMode(),
            args[0].content.string);
        res->content.value = 1;
        return (OK);
    }
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLstring)
            return (BAD);
        stringlist *s0 = (stringlist*)hdl->data;
        sLstr lstr;
        for (stringlist *s = s0; s; s = s->next) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add(s->string);
        }
        res->content.value = 1;
        Tech()->SetLayerPaletteReg(reg, DSP()->CurMode(), lstr.string());
        return (OK);
    }
    return (BAD);
}


//-------------------------------------------------------------------------
// Layer Database
//-------------------------------------------------------------------------

// (int) GetLayerNum(name)
//
// Return the OpenAccess layer number given the OpenAccess layer name. 
// This is the "layer" part of the general layer[:purpose] layer name
// used in Xic.  Each such name has a corresponding number in the
// database.  If the name is not found, the return value is -1, which
// is reserved and is not a valid OpenAccess layer number.
//
bool
misc3_funcs::IFgetLayerNum(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    if (!name || !*name)
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = CDldb()->getOAlayerNum(name);
    return (OK);
}


// (string) GetLayerName(num)
//
// Return the OpenAccess layer name given the OpenAccess layer number. 
// If there is no name associated with the number, a null string is
// returned.
//
bool
misc3_funcs::IFgetLayerName(Variable *res, Variable *args, void*)
{
    int num;
    ARG_CHK(arg_int(args, 0, &num))

    res->type = TYP_STRING;
    res->content.string = lstring::copy(CDldb()->getOAlayerName(num));
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}

// (int) IsPurposeDefined(name)
//
// This returns 1 if the name matches a known purpose, 0 otherwise.
//
bool
misc3_funcs::IFisPurposeDefined(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    if (!name || !*name)
        return (BAD);

    bool unknown;
    unsigned int ret = CDldb()->getOApurposeNum(name, &unknown);
    res->type = TYP_SCALAR;
    res->content.value = (ret != CDL_PRP_DRAWING_NUM || !unknown);
    return (OK);
}


// (int) GetPurposeNum(name);
//
// This will return a purpose number associated with the name.  If the
// name is not recognized, is null or empty, or matches "drawing"
// without case sensitivity, -1 is returned.  This is the "drawing"
// purpose number.
//
bool
misc3_funcs::IFgetPurposeNum(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))

    res->type = TYP_SCALAR;
    if (!name || !*name) {
        res->content.value = CDL_PRP_DRAWING_NUM;
        return (OK);
    }
    bool unknown;
    res->content.value = CDldb()->getOApurposeNum(name, &unknown);
    return (OK);
}


// (string) GetPurposeName(num)
//
// Return a string giving the purpose name corresponding to the passed
// purpose number.  If the purpose number is not recognized, or is the
// "drawing" purpose value of -1, a null string is returned.
//
bool
misc3_funcs::IFgetPurposeName(Variable *res, Variable *args, void*)
{
    int num;
    ARG_CHK(arg_int(args, 0, &num))

    res->type = TYP_STRING;
    res->content.string = lstring::copy(CDldb()->getOApurposeName(num));
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


//-------------------------------------------------------------------------
// Layers
//-------------------------------------------------------------------------

// (int) GetLayerLayerNum(stdlyr)
//
// Return the OpenAccess layer number associated with the layer
// indicated by the standard layer argument.
//
bool
misc3_funcs::IFgetLayerLayerNum(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld))

    res->type = TYP_SCALAR;
    res->content.value = ld->oaLayerNum();
    return (OK);
}


// (int) GetLayerPurposeNum(stdlyr)
//
// Return the OpenAccess purpose number associated with the layer
// indicated by the standard layer argument.
//
bool
misc3_funcs::IFgetLayerPurposeNum(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld))

    res->type = TYP_SCALAR;
    res->content.value = ld->oaPurposeNum();
    return (OK);
}


// (string) GetLayerAlias(stdlyr)
//
// This function returns a string containing the alias name of the
// layer indicated by the standard layer argument.  The string will be
// null if no alias is set.
//
bool
misc3_funcs::IFgetLayerAlias(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld))

    res->type = TYP_STRING;
    res->content.string = lstring::copy(ld->lppName());
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) SetLayerAlias(stdlyr, alias)
//
// This function sets the alias name of the layer indicated by the
// standard layer first argument to the string given as the second
// argument, as for the LppName technology file keyword.  The alias
// name is an optional secondary name for a layer/purpose pair.  Most
// if not all functions that take a layer name argument will also
// accept an alias name.
//
// The alias name will hide other layers if there is a name clash. 
// This can be used for layer remapping, but the user must be careful
// with this.  Layer name comparisons are case-insensitive.
//
// Unlike the normal layer names, the alias name can have arbitrary
// punctuation, embedded white space, etc.  However, leading and
// trailing white space is removed, and if the resulting string is
// empty or null, the existing alias name (if any) will be removed.
//
// The function returns 1 if the alias name is applied to the layer, 0
// if an error occurs.  It is not possible to set the same name on
// more than one layer.
//
bool
misc3_funcs::IFsetLayerAlias(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld))
    const char *alias;
    ARG_CHK(arg_string(args, 1, &alias))

    res->type = TYP_SCALAR;
    res->content.value = CDldb()->setLPPname(ld, alias);
    return (OK);
}


// (string) GetLayerDescr(stdlyr)
//
// This function returns a string containing the description of the
// layer indicated by the argument, which is a standard layer argument
// or a derived layer name string.  If no description has been set, a
// null string is returned.
//
bool
misc3_funcs::IFgetLayerDescr(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_STRING;
    res->content.string = lstring::copy(ld->description());
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) SetLayerDescr(stdlyr, descr)
//
// This function sets the description of the layer indicated by the
// first argument, which is a standard layer argument or a derived
// layer name string, to the string given as the second argument.  The
// description is an optional text string associated with the layer. 
// The function always returns 1.
//
bool
misc3_funcs::IFsetLayerDescr(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))
    const char *descr;
    ARG_CHK(arg_string(args, 1, &descr))

    res->type = TYP_SCALAR;
    res->content.value = 1.0;
    ld->setDescription(descr);
    return (OK);
}


// (int) IsLayerDefined(name)
//
// The string argument contains a layer name.  This can be the
// standard layer[:purpose] form, or can be an alias name.  This
// function returns 1 if the argument can be resolved as the name of a
// layer in the layer table, in the current (electrical/physical)
// mode.  If the layer can't be resolved, 0 is returned.  The function
// will fail fatally if the argument is null or empty.
//
bool
misc3_funcs::IFisLayerDefined(Variable *res, Variable *args, void*)
{
    const char *lname;
    ARG_CHK(arg_string(args, 0, &lname))

    if (!lname || !*lname)
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = (CDldb()->findLayer(lname, DSP()->CurMode()) != 0);
    return (OK);
}


// (int) IsLayerVisible(stdlyr)
//
// The function returns 1 if the layer indicated by the argument,
// which is a standard layer argument or a derived layer name string,
// is currently visible (i.e., the visibility flag is set), 0
// otherwise.  If the layer is derived, the return is the flag status,
// derived layers are never actually visible.
//
bool
misc3_funcs::IFisLayerVisible(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = !ld->isInvisible();
    return (OK);
}


// (int) SetLayerVisible(stdlyr, visible)
//
// This will set the visibility of the layer indicated in the first
// argument, which is a standard layer argument or a derived layer
// name string.  The layer will be visible if the boolean second
// argument is nonzero, invisible otherwise.  The previous visibility
// status is returned.  If the layer is derived, the flag status is
// set, however derived layers are never visible.
//
bool
misc3_funcs::IFsetLayerVisible(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))
    bool vis;
    ARG_CHK(arg_boolean(args, 1, &vis))

    res->type = TYP_SCALAR;
    res->content.value = !ld->isInvisible();
    ld->setInvisible(!vis);
    LT()->ShowLayerTable();
    return (OK);
}


// (int) IsLayerSelectable(stdlyr)
//
// The function returns 1 if the layer indicated by the argument,
// which is a standard layer argument or a derived layer name string,
// is currently selectable (i.e., the selectability flag is set), 0
// otherwise.
//
bool
misc3_funcs::IFisLayerSelectable(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = !ld->isSelectable();
    return (OK);
}


// (int) SetLayerSelectable(stdlyr, selectable)
//
// This will set the selectability of the layer indicated in the first
// argument, which is a standard layer argument or a derived layer name
// string.  The layer will be selectable if the boolean second
// argument is nonzero, not selectable otherwise.  The previous
// selectability status is returned.
//
bool
misc3_funcs::IFsetLayerSelectable(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))
    bool sel;
    ARG_CHK(arg_boolean(args, 1, &sel))

    res->type = TYP_SCALAR;
    res->content.value = ld->isSelectable();
    ld->setNoSelect(!sel);
    LT()->ShowLayerTable();
    return (OK);
}


// (int) IsLayerSymbolic(stdlyr)
//
// The function returns 1 if the layer indicated by the argument,
// which is a standard layer argument or a derived layer name string,
// is currently symbolic (i.e., the Symbolic attribute is set), 0
// otherwise.
//
bool
misc3_funcs::IFisLayerSymbolic(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = ld->isSymbolic();
    return (OK);
}


// (int) SetLayerSymbolic(stdlyr, symbolic)
//
// This will set the Symbolic attribute of the layer indicated in the
// first argument, which is a standard layer argument or a derived layer
// name string.  The layer will be symbolic if the boolean second
// argument is nonzero, not symbolic otherwise.  The previous symbolic
// status is returned.
//
bool
misc3_funcs::IFsetLayerSymbolic(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))
    bool smb;
    ARG_CHK(arg_boolean(args, 1, &smb))

    res->type = TYP_SCALAR;
    res->content.value = ld->isSymbolic();
    ld->setSymbolic(smb);
    LT()->ShowLayerTable();
    return (OK);
}


// (int) IsLayerNoMerge(stdlyr)
//
// The function returns 1 if the NoMerge attribute is set in the layer
// indicated by the argument, which is a standard layer argument or a
// derived layer name string, 0 otherwise.
//
bool
misc3_funcs::IFisLayerNoMerge(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = ld->isNoMerge();
    return (OK);
}


// (int) SetLayerNoMerge(stdlyr, nomerge)
//
// This will set the NoMerge attribute of the layer indicated in the
// first argument, which is a standard layer argument or a derived
// layer name string.  The layer will be given the NoMerge attribute
// if the boolean second argument is nonzero, or the attribute will be
// removed if present otherwise.  The previous NoMerge status is
// returned.
//
bool
misc3_funcs::IFsetLayerNoMerge(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))
    bool mrg;
    ARG_CHK(arg_boolean(args, 1, &mrg))

    res->type = TYP_SCALAR;
    res->content.value = ld->isNoMerge();
    ld->setNoMerge(mrg);
    LT()->ShowLayerTable();
    return (OK);
}


// (real) GetLayerMinDimension(stdlyr)
//
// The return value is the MinWidth design rule value in microns for
// the layer indicated by the argument, which is a standard layer
// argument or a derived layer name string.  If there is no MinWidth
// rule, or the DRC package is not available, 0 is returned.
//
bool
misc3_funcs::IFgetLayerMinDimension(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = MICRONS(DrcIf()->minDimension(ld));
    return (OK);
}


// (real) GetLayerWireWidth(stdlyr)
//
// The function returns the default wire width for the layer indicated
// by the argument, which is a standard layer argument or a derived
// layer name string.

//
bool
misc3_funcs::IFgetLayerWireWidth(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = MICRONS(dsp_prm(ld)->wire_width());
    return (OK);
}


// (int) AddLayerGdsOutMap(stdlyr, layer_num, datatype)
//
// This function will add a mapping from the layer in the first
// argument (a standard layer argument or a derived layer name string)
// to the given GDSII layer number and data type.  The layer number
// and data type are integers which define the layer in the GDSII
// world.  When a GDSII file is written, the present layer will appear
// on the given layer number and data type in the GDSII file.  It is
// possible to have multiple mappings of the layer, in which case the
// geometry from the named layer will appear on each layer number/data
// type given.
//
// The function returns 1 on success, or 0 if the layer number or data
// type number is out of range.  The acceptable range for the layer
// number and data type is [0 - 65535].
//
bool
misc3_funcs::IFaddLayerGdsOutMap(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))
    int lnum;
    ARG_CHK(arg_int(args, 1, &lnum))
    int dtype;
    ARG_CHK(arg_int(args, 2, &dtype))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (lnum < 0 || lnum >= GDS_MAX_LAYERS)
        return (OK);
    if (dtype < 0 || dtype >= GDS_MAX_DTYPES)
        return (OK);
    ld->addStrmOut(lnum, dtype);

    if (LT()->CurLayer() == ld)
        XM()->PopUpLayerParamEditor(0, MODE_UPD, 0, 0);
    res->content.value = 1;
    return (OK);
}


// (int) RemoveLayerGdsOutMap(stdlyr, layer_num, datatype)
//
// This function will remove a GDSII output layer mapping for the
// layer indicated in the first argument (a standard layer argument or
// a derived layer name string).  The mapping may have been applied in
// the technology file, with the Conversion Parameter Editor, or by
// calling the AddLayerGdsOutMap() function.  The mappings removed
// match the given layer number and data type integers provided. 
// These are in the range [-1 - 65535], where the value '-1' indicates
// a wild-card which will match all layer numbers or data types.
//
// The return value is -1 if the layer number or data type is out of
// range.  Otherwise, the return value is the number of mappings
// removed.
//
bool
misc3_funcs::IFremoveLayerGdsOutMap(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))
    int lnum;
    ARG_CHK(arg_int(args, 1, &lnum))
    int dtype;
    ARG_CHK(arg_int(args, 2, &dtype))

    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (lnum < -1)
        lnum = -1;
    else if (lnum >= GDS_MAX_LAYERS)
        return (OK);
    if (dtype < -1)
        dtype = -1;
    else if (dtype >= GDS_MAX_DTYPES)
        return (OK);

    res->content.value = ld->clearStrmOut(lnum, dtype);
    if (LT()->CurLayer() == ld)
        XM()->PopUpLayerParamEditor(0, MODE_UPD, 0, 0);
    return (OK);
}


// (int) AddLayerGdsInMap(stdlyr, string)
//
// This function adds a GDSII input mapping record to the layer whose
// name is indicated in the first argument (a standard layer argument
// or a derived layer name string).  The second argument is a string
// listing the layer numbers and data types which will map to the
// named layer, in the same syntax as used in the technology file. 
// This is "l1 l2-l3 ..., d1 d2-d3 ...", where there are two comma
// separated fields.  the left field consists of individual layer
// numbers and/or ranges of layer numbers, similarly the right field
// consists of individual data types and/or ranges of data types. 
// Each field can have an arbitrary number of space-separated terms. 
// For each layer listed or in a range, all of the data types listed
// or in a range will map to the named layer.  There can be multiple
// input mappings applied to the named layer.
//
// The function returns 0 if there was a syntax error.  The function
// returns 1 if the mapping is successfully added.
//
bool
misc3_funcs::IFaddLayerGdsInMap(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))

    if (!string)
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = ld->setStrmIn(string);
    if (LT()->CurLayer() == ld)
        XM()->PopUpLayerParamEditor(0, MODE_UPD, 0, 0);
    return (OK);
}


// (int) ClearLayerGdsInMap(stdlyr)
//
// This function deletes all of the GDSII input mappings applied to
// the layer indicated in the argument, which is a standard layer
// argument or a derived layer name string.  These mappings may have
// been applied through the technology file, added with the Conversion
// Parameter Editor, or added with the AddLayerGdsInMap() function. 
// The return value is the number of mapping records deleted.
//
bool
misc3_funcs::IFclearLayerGdsInMap(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = ld->clearStrmIn();
    if (LT()->CurLayer() == ld)
        XM()->PopUpLayerParamEditor(0, MODE_UPD, 0, 0);
    return (OK);
}


// (int) SetLayerNoDRCdatatype(stdlyr, datatype)
//
// This function assigns a data type to be used for objects with the
// DRC skip flag set.  The first argument is a standard layer argument
// indicating a physical layer or a derived layer name string.  The
// second argument is the data type in the range [0 - 65535], or -1. 
// If -1 is given, any previously defined data type is cleared.  The
// function returns 0 if the data type is out of range.  The value 1
// is returned on success.
//
bool
misc3_funcs::IFsetLayerNoDRCdatatype(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    int dtype;
    ARG_CHK(arg_int(args, 1, &dtype))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (dtype < -1)
        dtype = -1;
    else if (dtype >= GDS_MAX_DTYPES)
        return (OK);
    if (dtype < 0) {
        ld->setNoDRC(false);
        ld->setDatatype(CDNODRC_DT, 0);
    }
    else {
        ld->setNoDRC(true);
        ld->setDatatype(CDNODRC_DT, dtype);
    }
    res->content.value = 1;
    if (LT()->CurLayer() == ld)
        XM()->PopUpLayerParamEditor(0, MODE_UPD, 0, 0);
    return (OK);
}


//-------------------------------------------------------------------------
// Layers - Extraction Support
//-------------------------------------------------------------------------

// (string) SetLayerExKeyword(stdlyr, string)
//
// The first argument is a standard layer argument indicating a
// physical layer, or a derived layer name string.
// The string argument is an extraction keyword and associated text,
// as would appear in a layer block in the technology file.  The
// specification will be applied to the layer, overriding
// existing settings and possibly causing incompatible or redundant
// existing keywords to be deleted.  This is similar to the editing
// functions of the Edit Extraction command in the Extract Menu.
//
// The return is a status or error string, which may be null.
//
// The following keywords can be specified:
//    Conductor
//    Routing
//    GroundPlane
//    GroundPlaneDark
//    GroundPlaneClear
//    TermDefault
//    Contact
//    Via
//    Dielectric
//    DarkField
//    Resistance
//    Capacitance
//    Tranline
//    Thickness
//    Rho
//    Sigma
//    EpsRel
//    Lambda
//
bool
misc3_funcs::IFsetLayerExKeyword(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))

    extKWstruct kw;
    kw.load_keywords(ld, 0);
    char *before = kw.list_keywords();
    char *errstr = kw.insert_keyword_text(string);
    char *after = kw.list_keywords();
    if (strcmp(before, after)) {
        delete [] errstr;
        errstr = extKWstruct::set_settings(ld, after);
    }
    delete [] before;
    delete [] after;
    res->type = TYP_STRING;
    res->content.string = errstr;
    if (errstr)
        res->flags |= VF_ORIGINAL;
    if (ld == LT()->CurLayer())
        XM()->PopUpLayerParamEditor(0, MODE_UPD, 0, 0);
    return (OK);
}


// (string) SetCurLayerExKeyword(string)
//
// This is similar to SetLayerExKeyword, but applies to the current
// layer.  This function is deprecated and not recommended for use in
// new scripts.
//
bool
misc3_funcs::IFsetCurLayerExKeyword(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))

    extKWstruct kw;
    kw.load_keywords(LT()->CurLayer(), 0);
    char *before = kw.list_keywords();
    char *errstr = kw.insert_keyword_text(string);
    char *after = kw.list_keywords();
    if (strcmp(before, after)) {
        delete [] errstr;
        errstr = extKWstruct::set_settings(LT()->CurLayer(), after);
    }
    delete [] before;
    delete [] after;
    res->type = TYP_STRING;
    res->content.string = errstr;
    if (errstr)
        res->flags |= VF_ORIGINAL;
    XM()->PopUpLayerParamEditor(0, MODE_UPD, 0, 0);
    return (OK);
}


// (int) RemoveLayerExKeyword(stdlyr, string)
//
// The first argument is a standard layer argument indicating a
// physical layer, or a derived layer name string.  This will remove
// the specification for the extract keyword given in the argument
// from the layer.  The argument must be one of the extraction
// keywords, i.e., those listed for SetLayerExKeyword.  The return
// value is 1 if a specification was removed, 0 otherwise.
//
bool
misc3_funcs::IFremoveLayerExKeyword(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))

    extKWstruct kw;
    exKW type = kw.kwtype(string);
    kw.load_keywords(ld, 0);
    char *before = kw.list_keywords();
    kw.remove_keyword_text(type);
    char *after = kw.list_keywords();
    res->type = TYP_SCALAR;
    if (strcmp(before, after)) {
        char *errstr = extKWstruct::set_settings(ld, after);
        delete [] errstr;
        res->content.value = 1;
    }
    else
        res->content.value = 0;
    delete [] before;
    delete [] after;
    if (ld == LT()->CurLayer())
        XM()->PopUpLayerParamEditor(0, MODE_UPD, 0, 0);
    return (OK);
}


// (int) RemoveCurLayerExKeyword(string)
//
// This is similar to RemoveLayerExKeyword but applies to the current
// layer.  This function is deprecated and not recommended for use in
// new scripts.
//
bool
misc3_funcs::IFremoveCurLayerExKeyword(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))

    extKWstruct kw;
    exKW type = kw.kwtype(string);
    kw.load_keywords(LT()->CurLayer(), 0);
    char *before = kw.list_keywords();
    kw.remove_keyword_text(type);
    char *after = kw.list_keywords();
    res->type = TYP_SCALAR;
    if (strcmp(before, after)) {
        char *errstr = extKWstruct::set_settings(LT()->CurLayer(), after);
        delete [] errstr;
        res->content.value = 1;
    }
    else
        res->content.value = 0;
    delete [] before;
    delete [] after;
    XM()->PopUpLayerParamEditor(0, MODE_UPD, 0, 0);
    return (OK);
}


// (int) IsLayerConductor(stdlyr)
//
// The function returns 1 if the Conductor keyword is given or implied
// for the layer indicated by the argument, which is a standard layer
// argument or a derived layer name string, 0 otherwise.
//
bool
misc3_funcs::IFisLayerConductor(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = ld->isConductor();
    return (OK);
}


// (int) IsLayerRouting(stdlyr)
//
// The function returns 1 if the Routing keyword is given for the
// layer indicated by the argument, which is a standard layer argument
// or a derived layer name string, 0 otherwise.
//
bool
misc3_funcs::IFisLayerRouting(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = ld->isRouting();
    return (OK);
}


// (int) IsLayerGround(stdlyr)
//
// The function returns 1 if one of the GroundPlane keywords was given
// for the layer indicated by the argument, which is a standard layer
// argument or a derived layer name string, 0 otherwise.
//
bool
misc3_funcs::IFisLayerGround(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = ld->isGroundPlane();
    return (OK);
}


// (int) IsLayerContact(stdlyr)
//
// The function returns 1 if the Contact keyword is given for the
// layer indicated by the argument, which is a standard layer argument
// or a derived layer name string, 0 otherwise.
//
bool
misc3_funcs::IFisLayerContact(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = ld->isInContact();
    return (OK);
}


// (int) IsLayerVia(stdlyr)
//
// The function returns 1 if the Via keyword is given for the layer
// indicated by the argument, which is a standard layer argument or a
// derived layer name string, 0 otherwise.
//
bool
misc3_funcs::IFisLayerVia(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = ld->isVia();
    return (OK);
}


// (int) IsLayerViaCut(stdlyr)
//
// The function returns 1 if the ViaCut keyword is given for the layer
// indicated by the argument, which is a standard layer argument or a
// derived layer name string, 0 otherwise.
//
bool
misc3_funcs::IFisLayerViaCut(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = ld->isViaCut();
    return (OK);
}


// (int) IsLayerDielectric(stdlyr)
//
// The function returns 1 if the Dielectric keyword is given for the
// layer indicated by the argument, which is a standard layer argument
// or a derived layer name string, 0 otherwise.
//
bool
misc3_funcs::IFisLayerDielectric(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = ld->isDielectric();
    return (OK);
}


// (int) IsLayerDarkField(stdlyr)
//
// The function returns 1 if the DarkField keyword is given or implied
// for the layer indicated by the argument, which is a standard layer
// argument or a derived layer name string, 0 otherwise.
//
bool
misc3_funcs::IFisLayerDarkField(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = ld->isDarkField();
    return (OK);
}


// (real) GetLayerThickness(stdlyr)
//
// The function returns the value of the Thickness parameter given for
// the layer indicated by the argument, which is a standard layer
// argument or a derived layer name string.
//
bool
misc3_funcs::IFgetLayerThickness(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = dsp_prm(ld)->thickness();
    return (OK);
}


// (real) GetLayerRho(stdlyr)
//
// The function returns the resistivity in ohm-meters of the layer
// indicated by the argument, which is a standard layer argument or a
// derived layer name string, as given by the Rho or Sigma parameters,
// if given.  If neither of these is given, and Rsh and Thickness are
// given, the return value will be Rsh*Thickness.
//
bool
misc3_funcs::IFgetLayerRho(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    double rho = tech_prm(ld)->rho();
    if (rho <= 0.0 && tech_prm(ld)->ohms_per_sq() > 0.0 &&
            dsp_prm(ld)->thickness() > 0.0)
        rho = 1e-6*tech_prm(ld)->ohms_per_sq()*dsp_prm(ld)->thickness();
    res->content.value = rho;
    return (OK);
}


// (real) GetLayerResis(stdlyr)
//
// The function returns the the sheet resistance for the layer
// indicated by the arguemnt, which is a standard layer argument or a
// derived layer name string.  This will be the value of the Rsh
// parameter, if given, or the values of Rho/Thickness, if Rho or
// Sigma and Thickness are given, or 0 if no value is available.
//
bool
misc3_funcs::IFgetLayerResis(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    double rsh = tech_prm(ld)->ohms_per_sq();
    if (rsh <= 0.0 && tech_prm(ld)->rho() > 0.0 &&
            dsp_prm(ld)->thickness() > 0.0)
        rsh = 1e6*tech_prm(ld)->rho()/dsp_prm(ld)->thickness();

    res->content.value = rsh;
    return (OK);
}


// (real) GetLayerTau(stdlyr)
//
// The function returns the Drude model relaxation time associated
// with the layer.  This will add kinetic inductance to a normal
// metal or resistive layer, if a nonzero resistance has been set. 
// The effective complex conductivity is sigma/(1-i*omega*tau) at
// frequency omega.
//
bool
misc3_funcs::IFgetLayerTau(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = tech_prm(ld)->tau();
    return (OK);
}


// (real) GetLayerEps(stdlyr)
//
// The function returns the relative dielectric constant for the layer
// indicated by the argument, which is a standard layer argument or a
// derived layer name string, as given by the EpsRel parameter if
// applied.
//
bool
misc3_funcs::IFgetLayerEps(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = tech_prm(ld)->epsrel();
    return (OK);
}


// (real) GetLayerCap(stdlyr)
//
// The function returns the per-area capacitance for the layer
// indicated by the argument, which is a standard layer argument or a
// derived layer name string.
//
bool
misc3_funcs::IFgetLayerCap(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = tech_prm(ld)->cap_per_area();
    return (OK);
}


// (real) GetLayerCapPerim(stdlyr)
//
// The function returns the per-perimeter capacitance for the layer
// indicated by the argument, which is a standard layer argument or a
// derived layer name string.
//
bool
misc3_funcs::IFgetLayerCapPerim(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = tech_prm(ld)->cap_per_perim();
    return (OK);
}


// (real) GetLayerLambda(stdlyr)
//
// The function returns the value of the Lambda parameter for the
// layer indicated by the argument, which is a standard layer
// argumnent or a derived layer name string.
//
bool
misc3_funcs::IFgetLayerLambda(Variable *res, Variable *args, void*)
{
    CDl *ld;
    ARG_CHK(arg_layer(args, 0, &ld, true))

    res->type = TYP_SCALAR;
    res->content.value = tech_prm(ld)->lambda();
    return (OK);
}


//-------------------------------------------------------------------------
// Selections
//-------------------------------------------------------------------------

// (int) SetLayerSpecific(state)
//
// If the boolean state value is nonzero, all layers except for the
// current layer will become unselectable.  Otherwise, all layers will
// be set to their default selectability state.  The return value is
// always 1.
//
bool
misc3_funcs::IFsetLayerSpecific(Variable *res, Variable *args, void*)
{
    bool state;
    ARG_CHK(arg_boolean(args, 0, &state))

    res->type = TYP_SCALAR;
    res->content.value = 1;
    XM()->SetLayerSpecificSelections(state);
    return (OK);
}


// (int) SetLayerSearchUp(state)
//
// This function will set layer-search-up selection mode if the
// argument is nonzero, or normal mode otherwise.  The return value is
// 1 or 0 representing the previous layer-search_up mode status.
//
bool
misc3_funcs::IFsetLayerSearchUp(Variable *res, Variable *args, void*)
{
    bool state;
    ARG_CHK(arg_boolean(args, 0, &state))

    res->type = TYP_SCALAR;
    res->content.value = Selections.layerSearchUp();
    XM()->SetLayerSearchUpSelections(state);
    return (OK);
}


// (string) SetSelectMode(ptr_mode, area_mode, sel_mode)
//
// The function allows the various selection modes to be set.  These
// are the same modes that can be set with the pop-up provided by the
// layer button.  If an input value is given as -1, that particular
// parameter will be unchanged.  Otherwise, the possible values are
//
//   ptr_mode        area_mode         sel_mode
//   0    Normal     0    Normal       0    Normal
//   1    Select     1    Enclosed     1    Toggle
//   2    Modify     2    All          2    Add
//                                     3    Remove
//
// The return value is a string, where the first three characters are
// the previous values of ptr_mode, area_mode, and sel_mode.
//
bool
misc3_funcs::IFsetSelectMode(Variable *res, Variable *args, void*)
{
    int pmode;
    ARG_CHK(arg_int(args, 0, &pmode))
    int amode;
    ARG_CHK(arg_int(args, 1, &amode))
    int smode;
    ARG_CHK(arg_int(args, 2, &smode))

    if (pmode == -1)
        pmode = Selections.ptrMode();
    else if (pmode < PTRnormal)
        pmode = PTRnormal;
    else if (pmode > PTRmodify)
        pmode = PTRmodify;
    if (amode == -1)
        amode = Selections.areaMode();
    else if (amode < ASELnormal)
        amode = ASELnormal;
    else if (amode > ASELall)
        amode = ASELall;
    if (smode == -1)
        smode = Selections.selMode();
    else if (smode < SELnormal)
        smode = SELnormal;
    else if (smode > SELdesel)
        smode = SELdesel;
    res->type = TYP_STRING;
    char *aa = new char[4];
    res->content.string = aa;
    res->flags |= VF_ORIGINAL;
    aa[0] = Selections.ptrMode();
    aa[1] = Selections.areaMode();
    aa[2] = Selections.selMode();
    aa[3] = 0;
    Selections.setPtrMode((PTRmode)pmode);
    Selections.setAreaMode((ASELmode)amode);
    Selections.setSelMode((SELmode)smode);
    if (aa[0] != pmode || aa[1] != amode || aa[2] != smode)
        XM()->PopUpSelectControl(0, MODE_UPD);
    return (OK);
}


// SetSelectTypes(string)
//
// This function allows setting of the object types that can be
// selected.  This provides the default selection types, but does not
// apply to functions that provide an explicit argument for selection
// types.
//
// The string argument consists of a sequence of characters whose
// presence indicates that the corresponding object type is
// selectable.  These are
//
// c  cell instances
// b  boxes
// p  polygons
// w  wires
// l  labels
//
// Other characters are ignored.  If the string is null, empty, or
// contains none of the listed characters, all objects are enabled, as
// if the string "cbpwl" was entered.
//
// This function always returns 1.
//
bool
misc3_funcs::IFsetSelectTypes(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))

    enum { inst, box, poly, wire, label, end };
    bool active[end];
    for (int i = 0; i < end; i++)
        active[i] = false;
    if (string) {
        for (const char *s = string; *s; s++) {
            if (*s == CDINSTANCE)
                active[inst] = true;
            else if (*s == CDBOX)
                active[box] = true;
            else if (*s == CDPOLYGON)
                active[poly] = true;
            else if (*s == CDWIRE)
                active[wire] = true;
            else if (*s == CDLABEL)
                active[label] = true;
        }
    }
    if (!active[inst] && !active[box] && !active[poly] && !active[wire] &&
            !active[label]) {
        active[inst] = true;
        active[box] = true;
        active[poly] = true;
        active[wire] = true;
        active[label] = true;
    }
    Selections.setSelectType(CDINSTANCE, active[inst]);
    Selections.setSelectType(CDBOX, active[box]);
    Selections.setSelectType(CDPOLYGON, active[poly]);
    Selections.setSelectType(CDWIRE, active[wire]);
    Selections.setSelectType(CDLABEL, active[label]);
    XM()->PopUpSelectControl(0, MODE_UPD);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) Select(left, bottom, right, top, types)
//
// This function performs a selection operation in the rectangle
// defined by the first four arguments.  The fifth argument is a
// string whose characters serve to enable selection of a given type
// of object:  'b' for boxes, 'p' for polygons, 'w' for wires, 'l' for
// labels, and 'c' for instances.  If this string is blank or NULL,
// then all objects will be selected.
//
bool
misc3_funcs::IFselect(Variable *res, Variable *args, void*)
{
    BBox BB;
    ARG_CHK(arg_coord(args, 0, &BB.left, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 1, &BB.bottom, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 2, &BB.right, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 3, &BB.top, DSP()->CurMode()))
    const char *types = 0;
    ARG_CHK(arg_string(args, 4, &types))

    BB.fix();
    Selections.selection(CurCell(), types, &BB, true);
    unsigned int i;
    Selections.countQueue(CurCell(), &i, 0);
    res->type = TYP_SCALAR;
    res->content.value = i;
    return (OK);
}


// (int) Deselect()
//
// This function unselects all selected objects, as if the desel menu
// button was pressed.
//
bool
misc3_funcs::IFdeselect(Variable *res, Variable*, void*)
{
    Selections.deselectTypes(CurCell(), 0);
    res->type = TYP_SCALAR;
    res->content.value = 1.0;
    return (OK);
}


//-------------------------------------------------------------------------
// Pseudo-Flat Generator Functions
//-------------------------------------------------------------------------

// (object_handle) FlatObjList(l, b, r, t, depth)
//
// This function provides access to the "pseudo-flat" object access
// functions that are part of internal DRC routines in Xic.  This
// enables cycling through objects in the database without regard to
// the cell hierarchy.  The first four arguments are the coordinates
// in microns of the bounding box to search in.  The depth is the
// search depth, which can be an integer which sets the maximum depth
// to search (0 means search the current cell only, 1 means search the
// current cell plus the subcells, etc., and a negative integer sets
// the depth to search the entire hierarchy).  This argument can also
// be a string starting with 'a' such as "a" or "all" which indicates
// to search the entire hierarchy.
//
// The return value is a list of box, polygon, and wire objects found
// in the given region on the current layer.  Label and subcell
// objects are never returned.  If depth is 0, the actual object
// pointers are returned in the list, and all of the object
// manipulation functions are available.  Otherwise, the list
// references copies of the actual objects, transformed to the
// coordinate space of the current cell.  A list of copies behaves in
// most respects like an ordinary object list, except that the objects
// can not be modified.  The functions that modify objects will fail
// quietly (returning 0).  The ObjectCopy() function can be used to
// create a new database object from a returned copy.  The returned
// copies can be used in the first argument to the clipping functions
// such as ClipTo(), but not the second.  The handle manuipulation
// functions such as HandleCat() work, but lists of copies can not be
// mixed with lists of database objects, HandleCat() will fail quietly
// if this is attempted.  Copies can not be selected.
//
// The copies of the objects can use substantial memory if the list is
// very long.  The FlatObjGen() function provides another access
// interface that can use less memory.
//
bool
misc3_funcs::IFflatObjList(Variable *res, Variable *args, void*)
{
    BBox BB;
    ARG_CHK(arg_coord(args, 0, &BB.left, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 1, &BB.bottom, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 2, &BB.right, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 3, &BB.top, DSP()->CurMode()))
    int depth;
    ARG_CHK(arg_depth(args, 4, &depth))

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    BB.fix();
    sPF gen(cursd, &BB, LT()->CurLayer(), depth);

    CDo *odesc;
    CDol *ol = 0, *oe = 0;
    while ((odesc = gen.next((depth == 0), false)) != 0) {
        if (!ol)
            ol = oe = new CDol(odesc, 0);
        else {
            oe->next = new CDol(odesc, 0);
            oe = oe->next;
        }
    }

    sHdl *hdl = new sHdlObject(ol, cursd, (depth != 0));
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;

    return (OK);
}


// (handle) FlatObjGen(l, b, r, t, depth)
//
// This function provides access to the "pseudo-flat" object access
// functions that are part of internal DRC routines in Xic.  This
// enables cycling through objects in the database without regard to
// the cell hierarchy.  The first four arguments are the coordinates
// in microns of the bounding box to search in.  The depth is the
// search depth, which can be an integer which sets the maximum depth
// to search (0 means search the current cell only, 1 means search the
// current cell plus the subcells, etc., and a negative integer sets
// the depth to search the entire hierarchy).  This argument can also
// be a string starting with 'a' such as "a" or "all" which indicates
// to search the entire hierarchy.
//
// Similar to FlatObjList(), objects on the current layer are
// returned, but through an intermediate handle rather than through a
// list, which can require significant memory.  This function returns
// a special handle which is passed to the FlatGenNext() function to
// actually retrieve the objects.  Although this handle can be passed
// to the generic handle functions, most of these functions will have
// no effect.  HandleContent() will return 1, or 0 if the handle is
// exhausted.  Handle next will advance to the next object without
// saving the object.  The other functions will return 0 and do
// nothing.  The Close() function should be called to delete the
// handle unless the handle is iterated to completion with
// FlatGenNext() or HandleNext().
//
// If depth is 0, the object pointers returned from FlatGenNext()
// represent the actual object, and all object manipulation functions
// are available.  Otherwise, transformed copies of the actual objects
// are returned, and there are restrictions on the operations that can
// be performed.  See the description of the FlatObjList() function
// for more information.
//
bool
misc3_funcs::IFflatObjGen(Variable *res, Variable *args, void*)
{
    BBox BB;
    ARG_CHK(arg_coord(args, 0, &BB.left, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 1, &BB.bottom, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 2, &BB.right, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 3, &BB.top, DSP()->CurMode()))
    int depth;
    ARG_CHK(arg_depth(args, 4, &depth))

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    BB.fix();
    sPF *gen = new sPF(cursd, &BB, LT()->CurLayer(), depth);
    gdrec *r0 = new gdrec(&BB, depth, DSP()->CurMode(), 0);
    sHdl *hdl = new sHdlGen(gen, cursd, r0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;

    return (OK);
}


// (handle) FlatObjGenLayers(l, b, r, t, depth, layers)
//
// This function is very similar to FlatObjGen(), however it returns
// objects from layers named in the layers string.  If the string is
// null or empty, objects on all layers will be returned.  Otherwise,
// the string is a space separated list of layer names.  The names are
// expected to match layers in the current display mode.  Names that
// do not match any layer are silently ignored, though the function
// fails if no layer can be recognized.
//
bool
misc3_funcs::IFflatObjGenLayers(Variable *res, Variable *args, void*)
{
    BBox BB;
    ARG_CHK(arg_coord(args, 0, &BB.left, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 1, &BB.bottom, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 2, &BB.right, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 3, &BB.top, DSP()->CurMode()))
    int depth;
    ARG_CHK(arg_depth(args, 4, &depth))
    const char *lstring;
    ARG_CHK(arg_string(args, 5, &lstring))

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    BB.fix();

    stringlist *s0 = 0, *se = 0;
    if (lstring && *lstring) {
        char *tok;
        while ((tok = lstring::gettok(&lstring)) != 0) {
            if (!s0)
                s0 = se = new stringlist(tok, 0);
            else {
                se->next = new stringlist(tok, 0);
                se = se->next;
            }
        }
    }
    else {
        CDl *ld;
        CDlgen lgen(DSP()->CurMode());
        while ((ld = lgen.next()) != 0) {
            if (!s0)
                s0 = se = new stringlist(lstring::copy(ld->name()), 0);
            else {
                se->next = new stringlist(lstring::copy(ld->name()), 0);
                se = se->next;
            }
        }
    }
    CDl *ld = 0;
    while (!ld && s0) {
        ld = CDldb()->findLayer(s0->string, DSP()->CurMode());
        stringlist *sn = s0->next;
        delete [] s0->string;
        delete s0;
        s0 = sn;
    }
    if (!ld)
        return (BAD);

    sPF *gen = new sPF(cursd, &BB, ld, depth);
    gdrec *r0 = new gdrec(&BB, depth, DSP()->CurMode(), s0);
    sHdl *hdl = new sHdlGen(gen, cursd, r0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;

    return (OK);
}


// (object_handle) FlatGenNext(handle)
//
// This function takes as an argument the handle returned from
// FlatObjGen() or FlatObjGenLayers(), and returns an object handle
// which contains a single object returned from the generator.  If the
// depth argument passed to these functions was nonzero, The object is
// a copy, and the comments regarding object copies in the description
// of the FlatObjList() function apply.  The returned handles should
// be closed after use by calling Close(), or by calling an iterating
// function such as HandleNext() or ObjectNext().
//
// A new handle is returned for each call of this function, until no
// further objects are available in which case this function returns
// 0, and the handle passed as the argument will be closed.
//
bool
misc3_funcs::IFflatGenNext(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLgen)
            return (BAD);
        int hid = (long)hdl->iterator();
        res->type = TYP_HANDLE;
        res->content.value = hid;
    }
    return (OK);
}


// (int) FlatGenCount(handle)
//
// This function returns the number of objects that can be generated
// with the generator handle passed, which must be returned from
// FlatObjGen() or FlatObjGenLayers().  Generator handles do not cache
// an internal list of objects, so that the number of objects is
// unknown, which is why HandleContent() returns 1 for generator
// handles.  This function duplicates the generator context and
// iterates through the loop, counting returned objects.  This can be
// an expensive operation.
//
bool
misc3_funcs::IFflatGenCount(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLgen)
            return (BAD);
        sPF *gen = (sPF*)hdl->data;
        gen = gen->dup();
        gdrec *rec = ((sHdlGen*)hdl)->rec;
        rec = gdrec::dup(rec);
        CDs *sdesc = ((sHdlGen*)hdl)->sdesc;
        int cnt = 0;
        for (;;) {
            while (gen->next(true, false) != 0)
                cnt++;
            CDl *ld = 0;
            while (!ld && rec->names) {
                ld = CDldb()->findLayer(rec->names->string, rec->mode);
                stringlist *sn = rec->names->next;
                delete [] rec->names->string;
                delete rec->names;
                rec->names = sn;
            }
            if (ld) {
                if (gen->reinit(sdesc, &rec->AOI, ld, rec->depth))
                    continue;
            }
            break;
        }
        delete rec;
        delete gen;
        res->content.value = cnt;
    }
    return (OK);
}


namespace {
    inline bool
    objects_same(CDo *o1, CDo *o2)
    {
        if (o1->type() != o2->type() || o1->ldesc() != o2->ldesc() ||
                o1->oBB() != o2->oBB())
            return (false);
        if (o1->type() == CDPOLYGON) {
            int num = ((const CDpo*)o1)->numpts();
            if (num != ((const CDpo*)o2)->numpts())
                return (false);
            const Point *pts1 = ((const CDpo*)o1)->points();
            const Point *pts2 = ((const CDpo*)o2)->points();
            for (int i = 0; i < num; i++) {
                if (pts1[i] != pts2[i])
                    return (false);
            }
        }
        else if (o1->type() == CDWIRE) {
            int num = ((const CDw*)o1)->numpts();
            if (num != ((const CDw*)o2)->numpts())
                return (false);
            if (((const CDw*)o1)->attributes() !=
                    ((const CDw*)o2)->attributes())
                return (false);
            const Point *pts1 = ((const CDw*)o1)->points();
            const Point *pts2 = ((const CDw*)o2)->points();
            for (int i = 0; i < num; i++) {
                if (pts1[i] != pts2[i])
                    return (false);
            }
        }
        return (true);
    }
}


// (object_handle) FlatOverlapList(object_handle, touch_ok, depth, layers)
//
// This function returns a handle to a list of objects that touch or
// overlap the object referenced by the object_handle argument.  If
// touch_ok is nonzero, objects that touch but have zero overlap area
// will be included; if touch_ok is zero these objects will be
// skipped.  The depth is the search depth, which can be an integer
// which sets the maximum depth to search (0 means search the current
// cell only, 1 means search the current cell plus the subcells, etc.,
// and a negative integer sets the depth to search the entire
// hierarchy).  This argument can also be a string starting with 'a'
// such as "a" or "all" which indicates to search the entire
// hierarchy.  If depth is not 0, the objects returned are transformed
// copies, otherwise the actual objects are returned.  The copies have
// restrictions as described in the description of the FlatObjList()
// function.  The layer argument is a string containing
// space-separated layer names of the layers to search for objects.
// If this is empty or null, all layers will be searched.  The
// function fails if the handle argument is not a handle to an object
// list.  The return value is a handle to a list of objects, or 0 if
// no overlapping or touching objects are found.
//
// Only boxes, polygons, and wires are returned.  The reference object
// can be any object.  If the reference object is a subcell, objects
// from within the cell will be returned if depth is nonzero.
//
bool
misc3_funcs::IFflatOverlapList(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool t_ok;
    ARG_CHK(arg_boolean(args, 1, &t_ok))
    int depth;
    ARG_CHK(arg_depth(args, 2, &depth))
    const char *lstring;
    ARG_CHK(arg_string(args, 3, &lstring))

    CDs *cursd = CurCell();
    if (!cursd)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (!ol)
            return (OK);
        CDo *oref = ol->odesc;

        stringlist *s0 = 0, *se = 0;
        if (lstring && *lstring) {
            char *tok;
            while ((tok = lstring::gettok(&lstring)) != 0) {
                if (!s0)
                    s0 = se = new stringlist(tok, 0);
                else {
                    se->next = new stringlist(tok, 0);
                    se = se->next;
                }
            }
        }
        else {
            CDl *ld;
            CDlgen lgen(DSP()->CurMode());
            while ((ld = lgen.next()) != 0) {
                if (!s0)
                    s0 = se = new stringlist(lstring::copy(ld->name()), 0);
                else {
                    se->next = new stringlist(lstring::copy(ld->name()), 0);
                    se = se->next;
                }
            }
        }

        CDol *o0 = 0, *oe = 0;
        for (stringlist *s = s0; s; s = s->next) {
            CDl *ld = CDldb()->findLayer(s->string, DSP()->CurMode());
            if (!ld)
                continue;
            sPF gen(cursd, &oref->oBB(), ld, depth);
            CDo *odesc;
            while ((odesc = gen.next((depth == 0), t_ok)) != 0) {
                if (objects_same(oref, odesc)) {
                    if (depth)
                        delete odesc;
                    continue;
                }
                if (oref->intersect(odesc, t_ok)) {
                    if (!o0)
                        o0 = oe = new CDol(odesc, 0);
                    else {
                        oe->next = new CDol(odesc, 0);
                        oe = oe->next;
                    }
                    continue;
                }
                if (depth)
                    delete odesc;
            }
        }
        stringlist::destroy(s0);
        if (o0) {
            sHdl *hdltmp = new sHdlObject(o0, cursd, (depth != 0));
            res->type = TYP_HANDLE;
            res->content.value = hdltmp->id;
        }
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Geometry Measurement
//-------------------------------------------------------------------------

// (real) Distance(x, y, x1, y1)
//
// This function computes the distance between two points, given in
// microns, returning the distance between the points in microns.
//
bool
misc3_funcs::IFdistance(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, Physical))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, Physical))
    int x1;
    ARG_CHK(arg_coord(args, 2, &x1, Physical))
    int y1;
    ARG_CHK(arg_coord(args, 3, &y1, Physical))

    res->type = TYP_SCALAR;
    res->content.value = MICRONS(mmRnd(sqrt(
        (x - x1)*(double)(x - x1) + (y - y1)*(double)(y - y1))));
    return (OK);
}


// (real) MinDistPointToSeg(x, y, x1, y1, x2, y2, aret)
//
// This function computes the shortest distance from x,y to the line
// segment defined by the next four arguments.  The aret is an array
// of size at least 4, used for returned coordinates.  If no return is
// needed, this argument can be set to 0.  Upon return of a value
// greater than 0, the first two values in aret are x and y, the next
// two values are the point on the segment closest to x, y.  All
// values are in microns.
//
bool
misc3_funcs::IFminDistPointToSeg(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, Physical))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, Physical))
    int x1;
    ARG_CHK(arg_coord(args, 2, &x1, Physical))
    int y1;
    ARG_CHK(arg_coord(args, 3, &y1, Physical))
    int x2;
    ARG_CHK(arg_coord(args, 4, &x2, Physical))
    int y2;
    ARG_CHK(arg_coord(args, 5, &y2, Physical))
    double *aret;
    ARG_CHK(arg_array_if(args, 6, &aret, 4))

    Point_c pr(x, y);
    Point_c p0(x1, y1);
    Point_c p1(x2, y2);
    Point_c px;
    res->type = TYP_SCALAR;
    res->content.value = MICRONS(cGEO::mindist(&pr, &p0, &p1, &px));
    if (aret) {
        aret[0] = MICRONS(pr.x);
        aret[1] = MICRONS(pr.y);
        aret[2] = MICRONS(px.x);
        aret[3] = MICRONS(px.y);
    }
    return (OK);
}


// (real) MinDistPointToObj(x, y, object_handle, aret)
//
// This function computes the minimum distance from the point x,y to
// the boundary of the object given by the handle.  The aret is an
// array of size at least 4 for return coordinates.  If the return is
// not needed, this argument can be given as 0.  Upon return of a
// value greater than 0, the first two values of aret will be x and y,
// the next two values will be the point on the boundary of the object
// closest to x,y.  The function returns 0 if x,y touch or are
// enclosed in the object.  The function will fail if the handle is
// not a reference to an object list.  If there is an internal error,
// -1 is returned.  All coordinates are in microns.
//
bool
misc3_funcs::IFminDistPointToObj(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, Physical))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, Physical))
    int id;
    ARG_CHK(arg_handle(args, 2, &id))
    double *aret;
    ARG_CHK(arg_array_if(args, 3, &aret, 4))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            Point_c pr(x, y);
            Point_c px;
            res->content.value = MICRONS(cGEO::mindist(&pr, ol->odesc, &px));
            if (aret) {
                aret[0] = MICRONS(pr.x);
                aret[1] = MICRONS(pr.y);
                aret[2] = MICRONS(px.x);
                aret[3] = MICRONS(px.y);
            }
        }
    }
    return (OK);
}


// (real) MinDistSegToObj(x, y, x1, y1, object_handle, aret)
//
// This function computes the minimum distance from the line segment
// defined by the first four arguments to the boundary of the object
// given by the handle.  The aret is an array of size at least 4 for
// return coordinates.  If the return is not needed, this argument can
// be given as 0.  Upon return of a value greater than 0, the first
// two values of aret will be the point on the line segment nearest
// the object, the next two values will be the point on the boundary
// of the object nearest to the line segment.  The function returns 0
// if the line segment touches or overlaps the object.  The function
// will fail if the handle is not a reference to an object list.  If
// there is an internal error, -1 is returned.  All coordinates are in
// microns.
//
bool
misc3_funcs::IFminDistSegToObj(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, Physical))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, Physical))
    int x1;
    ARG_CHK(arg_coord(args, 2, &x1, Physical))
    int y1;
    ARG_CHK(arg_coord(args, 3, &y1, Physical))
    int id;
    ARG_CHK(arg_handle(args, 4, &id))
    double *aret;
    ARG_CHK(arg_array_if(args, 5, &aret, 4))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            Point_c p0(x, y);
            Point_c p1(x1, y1);
            Point_c px1, px2;
            res->content.value =
                MICRONS(cGEO::mindist(&p0, &p1, ol->odesc, &px1, &px2));
            if (aret) {
                aret[0] = MICRONS(px1.x);
                aret[1] = MICRONS(px1.y);
                aret[2] = MICRONS(px2.x);
                aret[3] = MICRONS(px2.y);
            }
        }
    }
    return (OK);
}


// (real) MinDistObjToObj(object_handle1, object_handle2, aret)
//
// This function computes the minimum distance between the two objects
// referenced by the handles.  The aret is an array of size at least 4
// for return coordinates.  If the return is not needed, this argument
// can be given as 0.  Upon return of a value greater than 0, the
// first two values of aret will be the point on the boundary of the
// first object nearest the second object, the next two values will be
// the point on the boundary of the second object nearest to the first
// object.  The function returns 0 if the objects touch or overlap.
// The function will fail if either handle is not a reference to an
// object list.  If there is an internal error, -1 is returned.  All
// coordinates are in microns.
//
bool
misc3_funcs::IFminDistObjToObj(Variable *res, Variable *args, void*)
{
    int id1;
    ARG_CHK(arg_handle(args, 0, &id1))
    int id2;
    ARG_CHK(arg_handle(args, 1, &id2))
    double *aret;
    ARG_CHK(arg_array_if(args, 2, &aret, 4))

    sHdl *hdl1 = sHdl::get(id1);
    sHdl *hdl2 = sHdl::get(id2);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl1 && hdl2) {
        if (hdl1->type != HDLobject)
            return (BAD);
        if (hdl2->type != HDLobject)
            return (BAD);
        CDol *ol1 = (CDol*)hdl1->data;
        CDol *ol2 = (CDol*)hdl2->data;
        if (ol1 && ol2) {
            Point px1, px2;
            res->content.value =
                MICRONS(cGEO::mindist(ol1->odesc, ol2->odesc, &px1, &px2));
            if (aret) {
                aret[0] = MICRONS(px1.x);
                aret[1] = MICRONS(px1.y);
                aret[2] = MICRONS(px2.x);
                aret[3] = MICRONS(px2.y);
            }
        }
    }
    return (OK);
}


// (real) MaxDistPointToObj(x, y, object_handle, aret)
//
// This function finds the vertext of the object referenced by the
// handle farthest from the point x,y and returns this distance.  The
// aret is an array of size at least 4 for return coordinates.  If the
// return is not needed, this argument can be given as 0.  Upon return
// of a value greater than 0, the first two values of aret will be x
// and y, the next two values will be the vertex of the object
// farthest from x,y.  The function will fail if the handle is not a
// reference to an object list.  If there is an internal error, -1 is
// returned.  All coordinates are in microns.
//
bool
misc3_funcs::IFmaxDistPointToObj(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, Physical))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, Physical))
    int id;
    ARG_CHK(arg_handle(args, 2, &id))
    double *aret;
    ARG_CHK(arg_array_if(args, 3, &aret, 4))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol) {
            Point_c pr(x, y);
            Point_c px;
            res->content.value = MICRONS(cGEO::maxdist(&pr, ol->odesc, &px));
            if (aret) {
                aret[0] = MICRONS(pr.x);
                aret[1] = MICRONS(pr.y);
                aret[2] = MICRONS(px.x);
                aret[3] = MICRONS(px.y);
            }
        }
    }
    return (OK);
}


// (real) MaxDistObjToObj(object_handle1, object_handle2, aret)
//
// This function finds the pair of vertices, one from each object,
// that are farthest apart.  Both handles can be the same.  The aret
// is an array of size at least 4 for return coordinates.  If the
// return is not needed, this argument can be given as 0.  Upon return
// of a value greater than 0, the first two values of aret will be the
// vertex from the first object, the next two values will be the
// vertex from the second object.  The function will fail if either
// handle is not a reference to an object list.  If there is an
// internal error, -1 is returned.  All coordinates are in microns.
//
bool
misc3_funcs::IFmaxDistObjToObj(Variable *res, Variable *args, void*)
{
    int id1;
    ARG_CHK(arg_handle(args, 0, &id1))
    int id2;
    ARG_CHK(arg_handle(args, 1, &id2))
    double *aret;
    ARG_CHK(arg_array_if(args, 2, &aret, 4))

    sHdl *hdl1 = sHdl::get(id1);
    sHdl *hdl2 = sHdl::get(id2);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl1 && hdl2) {
        if (hdl1->type != HDLobject)
            return (BAD);
        if (hdl2->type != HDLobject)
            return (BAD);
        CDol *ol1 = (CDol*)hdl1->data;
        CDol *ol2 = (CDol*)hdl2->data;
        if (ol1 && ol2) {
            Point px1, px2;
            res->content.value =
                MICRONS(cGEO::maxdist(ol1->odesc, ol2->odesc, &px1, &px2));
            if (aret) {
                aret[0] = MICRONS(px1.x);
                aret[1] = MICRONS(px1.y);
                aret[2] = MICRONS(px2.x);
                aret[3] = MICRONS(px2.y);
            }
        }
    }
    return (OK);
}


// (int) Intersect(object_handle1, object_handle2, touchok)
//
// This function determines whether the two objects referenced by the
// handles touch or overlap.  The return value is 1 if the objects
// touch or overlap, 0 if the objects do not touch or overlap, or -1
// if either handle points to an empty list or some other error
// occurred.  The function fails if either handle is not a reference
// to an object list.  If the touchok argument is nonzero, 1 will be
// returned if the objects touch but do not overlap.  If touchok is 0,
// objects must overlap (have nonzero intersection area) for 1 to be
// returned.
//
bool
misc3_funcs::IFintersect(Variable *res, Variable *args, void*)
{
    int id1;
    ARG_CHK(arg_handle(args, 0, &id1))
    int id2;
    ARG_CHK(arg_handle(args, 1, &id2))
    bool touchok;
    ARG_CHK(arg_boolean(args, 2, &touchok))

    sHdl *hdl1 = sHdl::get(id1);
    sHdl *hdl2 = sHdl::get(id2);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl1 && hdl2) {
        if (hdl1->type != HDLobject)
            return (BAD);
        if (hdl2->type != HDLobject)
            return (BAD);
        CDol *ol1 = (CDol*)hdl1->data;
        CDol *ol2 = (CDol*)hdl2->data;
        if (ol1 && ol2)
            res->content.value =
                ol1->odesc->intersect(ol2->odesc, touchok);
    }
    return (OK);
}

