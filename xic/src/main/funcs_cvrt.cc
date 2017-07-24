
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
 $Id: funcs_cvrt.cc,v 5.119 2016/03/02 00:39:46 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "cvrt.h"
#include "editif.h"
#include "python_if.h"
#include "tcltk_if.h"
#include "dsp_inlines.h"
#include "cd_compare.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "fio.h"
#include "fio_alias.h"
#include "fio_assemble.h"
#include "fio_cif.h"
#include "fio_chd.h"
#include "fio_chd_diff.h"
#include "fio_chd_flat.h"
#include "fio_oasis.h"
#include "fio_cgd.h"
#include "fio_cgd_lmux.h"
#include "si_parsenode.h"
#include "si_handle.h"
#include "si_args.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_daemon.h"
#include "events.h"
#include "pathlist.h"
#include "filestat.h"
#include "services.h"


namespace {
    namespace cvrt_funcs {

        // Layer Aliasing
        bool IFreadLayerCvAliases(Variable*, Variable*, void*);
        bool IFdumpLayerCvAliases(Variable*, Variable*, void*);
        bool IFclearLayerCvAliases(Variable*, Variable*, void*);
        bool IFaddLayerCvAlias(Variable*, Variable*, void*);
        bool IFremoveLayerCvAlias(Variable*, Variable*, void*);
        bool IFgetLayerCvAlias(Variable*, Variable*, void*);

        // Cell Name Mapping
        bool IFsetMapToLower(Variable*, Variable*, void*);
        bool IFsetMapToUpper(Variable*, Variable*, void*);

        // Cell Table
        bool IFcellTabAdd(Variable*, Variable*, void*);
        bool IFcellTabCheck(Variable*, Variable*, void*);
        bool IFcellTabRemove(Variable*, Variable*, void*);
        bool IFcellTabList(Variable*, Variable*, void*);
        bool IFcellTabClear(Variable*, Variable*, void*);

        // Windowing and Flattening
        bool IFsetConvertFlags(Variable*, Variable*, void*);
        bool IFsetConvertArea(Variable*, Variable*, void*);

        // Scale Factor
        bool IFsetConvertScale(Variable*, Variable*, void*);

        // Export Flags
        bool IFsetStripForExport(Variable*, Variable*, void*);
        bool IFsetSkipInvisLayers(Variable*, Variable*, void*);

        // Import Flags
        bool IFsetMergeInRead(Variable*, Variable*, void*);

        // Layout File Format Conversion
        bool IFfromArchive(Variable*, Variable*, void*);
        bool IFfromTxt(Variable*, Variable*, void*);
        bool IFfromNative(Variable*, Variable*, void*);

        // Export Layout File
        bool IFsaveCellAsNative(Variable*, Variable*, void*);
        bool IFexport(Variable*, Variable*, void*);
        bool IFtoXIC(Variable*, Variable*, void*);
        bool IFtoCGX(Variable*, Variable*, void*);
        bool IFtoCIF(Variable*, Variable*, void*);
        bool IFtoGDS(Variable*, Variable*, void*);
        bool IFtoGdsLibrary(Variable*, Variable*, void*);
        bool IFtoOASIS(Variable*, Variable*, void*);
        bool IFtoTxt(Variable*, Variable*, void*);

        // Cell Hierarchy Digest
        bool IFfileInfo(Variable*, Variable*, void*);
        bool IFopenCellHierDigest(Variable*, Variable*, void*);
        bool IFwriteCellHierDigest(Variable*, Variable*, void*);
        bool IFreadCellHierDigest(Variable*, Variable*, void*);
        bool IFchdList(Variable*, Variable*, void*);
        bool IFchdChangeName(Variable*, Variable*, void*);
        bool IFchdIsValid(Variable*, Variable*, void*);
        bool IFchdDestroy(Variable*, Variable*, void*);
        bool IFchdInfo(Variable*, Variable*, void*);
        bool IFchdFileName(Variable*, Variable*, void*);
        bool IFchdFileType(Variable*, Variable*, void*);
        bool IFchdTopCells(Variable*, Variable*, void*);
        bool IFchdListCells(Variable*, Variable*, void*);
        bool IFchdLayers(Variable*, Variable*, void*);
        bool IFchdInfoMode(Variable*, Variable*, void*);
        bool IFchdInfoLayers(Variable*, Variable*, void*);
        bool IFchdInfoCells(Variable*, Variable*, void*);
        bool IFchdInfoCounts(Variable*, Variable*, void*);
        bool IFchdCellBB(Variable*, Variable*, void*);
        bool IFchdSetDefCellName(Variable*, Variable*, void*);
        bool IFchdDefCellName(Variable*, Variable*, void*);
        bool IFchdLoadGeometry(Variable*, Variable*, void*);
        bool IFchdLinkCgd(Variable*, Variable*, void*);
        bool IFchdGetGeomName(Variable*, Variable*, void*);
        bool IFchdClearGeometry(Variable*, Variable*, void*);
        bool IFchdSetSkipFlag(Variable*, Variable*, void*);
        bool IFchdClearSkipFlags(Variable*, Variable*, void*);
        bool IFchdCompare(Variable*, Variable*, void*);
        bool IFchdCompareFlat(Variable*, Variable*, void*);
        bool IFchdEdit(Variable*, Variable*, void*);
        bool IFchdOpenFlat(Variable*, Variable*, void*);
        bool IFchdSetFlatReadTransform(Variable*, Variable*, void*);
        bool IFchdEstFlatMemoryUse(Variable*, Variable*, void*);
        bool IFchdWrite(Variable*, Variable*, void*);
        bool IFchdWriteSplit(Variable*, Variable*, void*);
        bool IFchdCreateReferenceCell(Variable*, Variable*, void*);
        bool IFchdLoadCell(Variable*, Variable*, void*);
        bool IFchdIterateOverRegion(Variable*, Variable*, void*);
        bool IFchdWriteDensityMaps(Variable*, Variable*, void*);

        // Cell Geometry Digests
        bool IFopenCellGeomDigest(Variable*, Variable*, void*);
        bool IFnewCellGeomDigest(Variable*, Variable*, void*);
        bool IFwriteCellGeomDigest(Variable*, Variable*, void*);
        bool IFcgdList(Variable*, Variable*, void*);
        bool IFcgdChangeName(Variable*, Variable*, void*);
        bool IFcgdIsValid(Variable*, Variable*, void*);
        bool IFcgdDestroy(Variable*, Variable*, void*);
        bool IFcgdIsValidCell(Variable*, Variable*, void*);
        bool IFcgdIsValidLayer(Variable*, Variable*, void*);
        bool IFcgdRemoveCell(Variable*, Variable*, void*);
        bool IFcgdIsCellRemoved(Variable*, Variable*, void*);
        bool IFcgdRemoveLayer(Variable*, Variable*, void*);
        bool IFcgdAddCells(Variable*, Variable*, void*);
        bool IFcgdContents(Variable*, Variable*, void*);
        bool IFcgdOpenGeomStream(Variable*, Variable*, void*);
        bool IFgsReadObject(Variable*, Variable*, void*);
        bool IFgsDumpOasisText(Variable*, Variable*, void*);

        // Assembly Stream
        bool IFstreamOpen(Variable*, Variable*, void*);
        bool IFstreamTopCell(Variable*, Variable*, void*);
        bool IFstreamSource(Variable*, Variable*, void*);
        bool IFstreamInstance(Variable*, Variable*, void*);
        bool IFstreamRun(Variable*, Variable*, void*);
    }
    using namespace cvrt_funcs;

#ifdef HAVE_PYTHON
    // Python wrappers.

    // Layer Aliasing
    PY_FUNC(ReadLayerCvAliases,     1,  IFreadLayerCvAliases);
    PY_FUNC(DumpLayerCvAliases,     1,  IFdumpLayerCvAliases);
    PY_FUNC(ClearLayerCvAliases,    0,  IFclearLayerCvAliases);
    PY_FUNC(AddLayerCvAlias,        2,  IFaddLayerCvAlias);
    PY_FUNC(RemoveLayerCvAlias,     1,  IFremoveLayerCvAlias);
    PY_FUNC(GetLayerCvAlias,        1,  IFgetLayerCvAlias);

    // Cell Name Mapping
    PY_FUNC(SetMapToLower,          2,  IFsetMapToLower);
    PY_FUNC(SetMapToUpper,          2,  IFsetMapToUpper);

    // Cell Table
    PY_FUNC(CellTabAdd,             2,  IFcellTabAdd);
    PY_FUNC(CellTabCheck,           1,  IFcellTabCheck);
    PY_FUNC(CellTabRemove,          1,  IFcellTabRemove);
    PY_FUNC(CellTabList,            0,  IFcellTabList);
    PY_FUNC(CellTabClear,           0,  IFcellTabClear);

    // Windowing and Flattening
    PY_FUNC(SetConvertFlags,        5,  IFsetConvertFlags);
    PY_FUNC(SetConvertArea,         5,  IFsetConvertArea);

    // Scale Factor
    PY_FUNC(SetConvertScale,        2,  IFsetConvertScale);

    // Export Flags
    PY_FUNC(SetStripForExport,      1,  IFsetStripForExport);
    PY_FUNC(SetSkipInvisLayers,     1,  IFsetSkipInvisLayers);

    // Import Flags
    PY_FUNC(SetMergeInRead,         1,  IFsetMergeInRead);

    // Layout File Format Conversion
    PY_FUNC(FromArchive,            2,  IFfromArchive);
    PY_FUNC(FromTxt,                2,  IFfromTxt);
    PY_FUNC(FromNative,             2,  IFfromNative);

    // Export Layout File
    PY_FUNC(SaveCellAsNative,       2,  IFsaveCellAsNative);
    PY_FUNC(Export,                 2,  IFexport);
    PY_FUNC(ToXIC,                  1,  IFtoXIC);
    PY_FUNC(ToCGX,                  1,  IFtoCGX);
    PY_FUNC(ToCIF,                  1,  IFtoCIF);
    PY_FUNC(ToGDS,                  1,  IFtoGDS);
    PY_FUNC(ToGdsLibrary,           2,  IFtoGdsLibrary);
    PY_FUNC(ToOASIS,                1,  IFtoOASIS);
    PY_FUNC(ToTxt,                  3,  IFtoTxt);

    // Cell Hierarchy Digest
    PY_FUNC(FileInfo,               3,  IFfileInfo);
    PY_FUNC(OpenCellHierDigest,     2,  IFopenCellHierDigest);
    PY_FUNC(WriteCellHierDigest,    4,  IFwriteCellHierDigest);
    PY_FUNC(ReadCellHierDigest,     2,  IFreadCellHierDigest);
    PY_FUNC(ChdList,                0,  IFchdList);
    PY_FUNC(ChdChangeName,          2,  IFchdChangeName);
    PY_FUNC(ChdIsValid,             1,  IFchdIsValid);
    PY_FUNC(ChdDestroy,             1,  IFchdDestroy);
    PY_FUNC(ChdInfo,                3,  IFchdInfo);
    PY_FUNC(ChdFileName,            1,  IFchdFileName);
    PY_FUNC(ChdFileType,            1,  IFchdFileType);
    PY_FUNC(ChdTopCells,            1,  IFchdTopCells);
    PY_FUNC(ChdListCells,           4,  IFchdListCells);
    PY_FUNC(ChdLayers,              1,  IFchdLayers);
    PY_FUNC(ChdInfoMode,            1,  IFchdInfoMode);
    PY_FUNC(ChdInfoLayers,          2,  IFchdInfoLayers);
    PY_FUNC(ChdInfoCells,           1,  IFchdInfoCells);
    PY_FUNC(ChdInfoCounts,          4,  IFchdInfoCounts);
    PY_FUNC(ChdCellBB,              3,  IFchdCellBB);
    PY_FUNC(ChdSetDefCellName,      2,  IFchdSetDefCellName);
    PY_FUNC(ChdDefCellName,         1,  IFchdDefCellName);
    PY_FUNC(ChdLoadGeometry,        1,  IFchdLoadGeometry);
    PY_FUNC(ChdLinkCgd,             2,  IFchdLinkCgd);
    PY_FUNC(ChdGetGeomName,         1,  IFchdGetGeomName);
    PY_FUNC(ChdClearGeometry,       1,  IFchdClearGeometry);
    PY_FUNC(ChdSetSkipFlag,         3,  IFchdSetSkipFlag);
    PY_FUNC(ChdClearSkipFlags,      1,  IFchdClearSkipFlags);
    PY_FUNC(ChdCompare,             10, IFchdCompare);
    PY_FUNC(ChdCompareFlat,         11, IFchdCompareFlat);
    PY_FUNC(ChdEdit,                3,  IFchdEdit);
    PY_FUNC(ChdOpenFlat,            5,  IFchdOpenFlat);
    PY_FUNC(ChdSetFlatReadTransform,3,  IFchdSetFlatReadTransform);
    PY_FUNC(ChdEstFlatMemoryUse,    4,  IFchdEstFlatMemoryUse);
    PY_FUNC(ChdWrite,               9,  IFchdWrite);
    PY_FUNC(ChdWriteSplit,          9,  IFchdWriteSplit);
    PY_FUNC(ChdCreateReferenceCell, 2,  IFchdCreateReferenceCell);
    PY_FUNC(ChdLoadCell,            2,  IFchdLoadCell);
    PY_FUNC(ChdIterateOverRegion,   7,  IFchdIterateOverRegion);
    PY_FUNC(ChdWriteDensityMaps,    7,  IFchdWriteDensityMaps);

    // Cell Geometry Digest
    PY_FUNC(OpenCellGeomDigest,     3,  IFopenCellGeomDigest);
    PY_FUNC(NewCellGeomDigest,      0,  IFnewCellGeomDigest);
    PY_FUNC(WriteCellGeomDigest,    2,  IFwriteCellGeomDigest);
    PY_FUNC(CgdList,                0,  IFcgdList);
    PY_FUNC(CgdChangeName,          2,  IFcgdChangeName);
    PY_FUNC(CgdIsValid,             1,  IFcgdIsValid);
    PY_FUNC(CgdDestroy,             1,  IFcgdDestroy);
    PY_FUNC(CgdIsValidCell,         2,  IFcgdIsValidCell);
    PY_FUNC(CgdIsValidLayer,        3,  IFcgdIsValidLayer);
    PY_FUNC(CgdRemoveCell,          2,  IFcgdRemoveCell);
    PY_FUNC(CgdIsCellRemoved,       2,  IFcgdIsCellRemoved);
    PY_FUNC(CgdRemoveLayer,         3,  IFcgdRemoveLayer);
    PY_FUNC(CgdAddCells,            3,  IFcgdAddCells);
    PY_FUNC(CgdContents,            3,  IFcgdContents);
    PY_FUNC(CgdOpenGeomStream,      3,  IFcgdOpenGeomStream);
    PY_FUNC(GsReadObject,           1,  IFgsReadObject);
    PY_FUNC(GsDumpOasisText,        1,  IFgsDumpOasisText);

    // Assembly Stream
    PY_FUNC(StreamOpen,             1,  IFstreamOpen);
    PY_FUNC(StreamTopCell,          2,  IFstreamTopCell);
    PY_FUNC(StreamSource,           5,  IFstreamSource);
    PY_FUNC(StreamInstance,         13, IFstreamInstance);
    PY_FUNC(StreamRun,              1,  IFstreamRun);


    void py_register_cvrt()
    {
      // Layer Aliasing
      cPyIf::register_func("ReadLayerCvAliases",     pyReadLayerCvAliases);
      cPyIf::register_func("DumpLayerCvAliases",     pyDumpLayerCvAliases);
      cPyIf::register_func("ClearLayerCvAliases",    pyClearLayerCvAliases);
      cPyIf::register_func("AddLayerCvAlias",        pyAddLayerCvAlias);
      cPyIf::register_func("RemoveLayerCvAlias",     pyRemoveLayerCvAlias);
      cPyIf::register_func("GetLayerCvAlias",        pyGetLayerCvAlias);

      // Cell Name Mapping
      cPyIf::register_func("SetMapToLower",          pySetMapToLower);
      cPyIf::register_func("SetMapToUpper",          pySetMapToUpper);

      // Cell Table
      cPyIf::register_func("CellTabAdd",             pyCellTabAdd);
      cPyIf::register_func("CellTabCheck",           pyCellTabCheck);
      cPyIf::register_func("CellTabRemove",          pyCellTabRemove);
      cPyIf::register_func("CellTabList",            pyCellTabList);
      cPyIf::register_func("CellTabClear",           pyCellTabClear);

      // Windowing and Flattening
      cPyIf::register_func("SetConvertFlags",        pySetConvertFlags);
      cPyIf::register_func("SetConvertArea",         pySetConvertArea);

      // Scale Factor
      cPyIf::register_func("SetConvertScale",        pySetConvertScale);

      // Export Flags
      cPyIf::register_func("SetStripForExport",      pySetStripForExport);
      cPyIf::register_func("SetSkipInvisLayers",     pySetSkipInvisLayers);

      // Import Flags
      cPyIf::register_func("SetMergeInRead",         pySetMergeInRead);

      // Layout File Format Conversion
      cPyIf::register_func("FromArchive",            pyFromArchive);
      cPyIf::register_func("FromTxt",                pyFromTxt);
      cPyIf::register_func("FromNative",             pyFromNative);

      // Export Layout File
      cPyIf::register_func("SaveCellAsNative",       pySaveCellAsNative);
      cPyIf::register_func("Export",                 pyExport);
      cPyIf::register_func("ToXIC",                  pyToXIC);
      cPyIf::register_func("ToCGX",                  pyToCGX);
      cPyIf::register_func("ToCIF",                  pyToCIF);
      cPyIf::register_func("ToGDS",                  pyToGDS);
      cPyIf::register_func("ToGdsLibrary",           pyToGdsLibrary);
      cPyIf::register_func("ToOASIS",                pyToOASIS);
      cPyIf::register_func("ToTxt",                  pyToTxt);

      // Cell Hierarchy Digest
      cPyIf::register_func("FileInfo",               pyFileInfo);
      cPyIf::register_func("OpenCellHierDigest",     pyOpenCellHierDigest);
      cPyIf::register_func("WriteCellHierDigest",    pyWriteCellHierDigest);
      cPyIf::register_func("ReadCellHierDigest",     pyReadCellHierDigest);
      cPyIf::register_func("ChdList",                pyChdList);
      cPyIf::register_func("ChdChangeName",          pyChdChangeName);
      cPyIf::register_func("ChdIsValid",             pyChdIsValid);
      cPyIf::register_func("ChdDestroy",             pyChdDestroy);
      cPyIf::register_func("ChdInfo",                pyChdInfo);
      cPyIf::register_func("ChdFileName",            pyChdFileName);
      cPyIf::register_func("ChdFileType",            pyChdFileType);
      cPyIf::register_func("ChdTopCells",            pyChdTopCells);
      cPyIf::register_func("ChdListCells",           pyChdListCells);
      cPyIf::register_func("ChdLayers",              pyChdLayers);
      cPyIf::register_func("ChdInfoMode",            pyChdInfoMode);
      cPyIf::register_func("ChdInfoLayers",          pyChdInfoLayers);
      cPyIf::register_func("ChdInfoCells",           pyChdInfoCells);
      cPyIf::register_func("ChdInfoCounts",          pyChdInfoCounts);
      cPyIf::register_func("ChdCellBB",              pyChdCellBB);
      cPyIf::register_func("ChdSetDefCellName",      pyChdSetDefCellName);
      cPyIf::register_func("ChdDefCellName",         pyChdDefCellName);
      cPyIf::register_func("ChdLoadGeometry",        pyChdLoadGeometry);
      cPyIf::register_func("ChdLinkCgd",             pyChdLinkCgd);
      cPyIf::register_func("ChdGetGeomName",         pyChdGetGeomName);
      cPyIf::register_func("ChdClearGeometry",       pyChdClearGeometry);
      cPyIf::register_func("ChdSetSkipFlag",         pyChdSetSkipFlag);
      cPyIf::register_func("ChdClearSkipFlags",      pyChdClearSkipFlags);
      cPyIf::register_func("ChdCompare",             pyChdCompare);
      cPyIf::register_func("ChdCompareFlat",         pyChdCompareFlat);
      cPyIf::register_func("ChdEdit",                pyChdEdit);
      cPyIf::register_func("ChdOpenFlat",            pyChdOpenFlat);
      cPyIf::register_func("ChdSetFlatReadTransform",pyChdSetFlatReadTransform);
      cPyIf::register_func("ChdEstFlatMemoryUse",    pyChdEstFlatMemoryUse);
      cPyIf::register_func("ChdWrite",               pyChdWrite);
      cPyIf::register_func("ChdWriteSplit",          pyChdWriteSplit);
      cPyIf::register_func("ChdCreateReferenceCell", pyChdCreateReferenceCell);
      cPyIf::register_func("ChdLoadCell",            pyChdLoadCell);
      cPyIf::register_func("ChdIterateOverRegion",   pyChdIterateOverRegion);
      cPyIf::register_func("ChdWriteDensityMaps",    pyChdWriteDensityMaps);

      // Cell Geometry Digest
      cPyIf::register_func("OpenCellGeomDigest",     pyOpenCellGeomDigest);
      cPyIf::register_func("NewCellGeomDigest",      pyNewCellGeomDigest);
      cPyIf::register_func("WriteCellGeomDigest",    pyWriteCellGeomDigest);
      cPyIf::register_func("CgdList",                pyCgdList);
      cPyIf::register_func("CgdChangeName",          pyCgdChangeName);
      cPyIf::register_func("CgdIsValid",             pyCgdIsValid);
      cPyIf::register_func("CgdDestroy",             pyCgdDestroy);
      cPyIf::register_func("CgdIsValidCell",         pyCgdIsValidCell);
      cPyIf::register_func("CgdIsValidLayer",        pyCgdIsValidLayer);
      cPyIf::register_func("CgdRemoveCell",          pyCgdRemoveCell);
      cPyIf::register_func("CgdIsCellRemoved",       pyCgdIsCellRemoved);
      cPyIf::register_func("CgdRemoveLayer",         pyCgdRemoveLayer);
      cPyIf::register_func("CgdAddCells",            pyCgdAddCells);
      cPyIf::register_func("CgdContents",            pyCgdContents);
      cPyIf::register_func("CgdOpenGeomStream",      pyCgdOpenGeomStream);
      cPyIf::register_func("GsReadObject",           pyGsReadObject);
      cPyIf::register_func("GsDumpOasisText",        pyGsDumpOasisText);

      // Assembly Stream
      cPyIf::register_func("StreamOpen",             pyStreamOpen);
      cPyIf::register_func("StreamTopCell",          pyStreamTopCell);
      cPyIf::register_func("StreamSource",           pyStreamSource);
      cPyIf::register_func("StreamInstance",         pyStreamInstance);
      cPyIf::register_func("StreamRun",              pyStreamRun);
    }
#endif  // HAVE_PYTHON

#ifdef HAVE_TCL
    // Tcl/Tk wrappers.

    // Layer Aliasing
    TCL_FUNC(ReadLayerCvAliases,     1,  IFreadLayerCvAliases);
    TCL_FUNC(DumpLayerCvAliases,     1,  IFdumpLayerCvAliases);
    TCL_FUNC(ClearLayerCvAliases,    0,  IFclearLayerCvAliases);
    TCL_FUNC(AddLayerCvAlias,        2,  IFaddLayerCvAlias);
    TCL_FUNC(RemoveLayerCvAlias,     1,  IFremoveLayerCvAlias);
    TCL_FUNC(GetLayerCvAlias,        1,  IFgetLayerCvAlias);

    // Cell Name Mapping
    TCL_FUNC(SetMapToLower,          2,  IFsetMapToLower);
    TCL_FUNC(SetMapToUpper,          2,  IFsetMapToUpper);

    // Cell Table
    TCL_FUNC(CellTabAdd,             2,  IFcellTabAdd);
    TCL_FUNC(CellTabCheck,           1,  IFcellTabCheck);
    TCL_FUNC(CellTabRemove,          1,  IFcellTabRemove);
    TCL_FUNC(CellTabList,            0,  IFcellTabList);
    TCL_FUNC(CellTabClear,           0,  IFcellTabClear);

    // Windowing and Flattening
    TCL_FUNC(SetConvertFlags,        5,  IFsetConvertFlags);
    TCL_FUNC(SetConvertArea,         5,  IFsetConvertArea);

    // Scale Factor
    TCL_FUNC(SetConvertScale,        2,  IFsetConvertScale);

    // Export Flags
    TCL_FUNC(SetStripForExport,      1,  IFsetStripForExport);
    TCL_FUNC(SetSkipInvisLayers,     1,  IFsetSkipInvisLayers);

    // Import Flags
    TCL_FUNC(SetMergeInRead,         1,  IFsetMergeInRead);

    // Layout File Format Conversion
    TCL_FUNC(FromArchive,            2,  IFfromArchive);
    TCL_FUNC(FromTxt,                2,  IFfromTxt);
    TCL_FUNC(FromNative,             2,  IFfromNative);

    // Export Layout File
    TCL_FUNC(SaveCellAsNative,       2,  IFsaveCellAsNative);
    TCL_FUNC(Export,                 2,  IFexport);
    TCL_FUNC(ToXIC,                  1,  IFtoXIC);
    TCL_FUNC(ToCGX,                  1,  IFtoCGX);
    TCL_FUNC(ToCIF,                  1,  IFtoCIF);
    TCL_FUNC(ToGDS,                  1,  IFtoGDS);
    TCL_FUNC(ToGdsLibrary,           2,  IFtoGdsLibrary);
    TCL_FUNC(ToOASIS,                1,  IFtoOASIS);
    TCL_FUNC(ToTxt,                  3,  IFtoTxt);

    // Cell Hierarchy Digest
    TCL_FUNC(FileInfo,               3,  IFfileInfo);
    TCL_FUNC(OpenCellHierDigest,     2,  IFopenCellHierDigest);
    TCL_FUNC(WriteCellHierDigest,    4,  IFwriteCellHierDigest);
    TCL_FUNC(ReadCellHierDigest,     2,  IFreadCellHierDigest);
    TCL_FUNC(ChdList,                0,  IFchdList);
    TCL_FUNC(ChdChangeName,          2,  IFchdChangeName);
    TCL_FUNC(ChdIsValid,             1,  IFchdIsValid);
    TCL_FUNC(ChdDestroy,             1,  IFchdDestroy);
    TCL_FUNC(ChdInfo,                3,  IFchdInfo);
    TCL_FUNC(ChdFileName,            1,  IFchdFileName);
    TCL_FUNC(ChdFileType,            1,  IFchdFileType);
    TCL_FUNC(ChdTopCells,            1,  IFchdTopCells);
    TCL_FUNC(ChdListCells,           4,  IFchdListCells);
    TCL_FUNC(ChdLayers,              1,  IFchdLayers);
    TCL_FUNC(ChdInfoMode,            1,  IFchdInfoMode);
    TCL_FUNC(ChdInfoLayers,          2,  IFchdInfoLayers);
    TCL_FUNC(ChdInfoCells,           1,  IFchdInfoCells);
    TCL_FUNC(ChdInfoCounts,          4,  IFchdInfoCounts);
    TCL_FUNC(ChdCellBB,              3,  IFchdCellBB);
    TCL_FUNC(ChdSetDefCellName,      2,  IFchdSetDefCellName);
    TCL_FUNC(ChdDefCellName,         1,  IFchdDefCellName);
    TCL_FUNC(ChdLoadGeometry,        1,  IFchdLoadGeometry);
    TCL_FUNC(ChdLinkCgd,             2,  IFchdLinkCgd);
    TCL_FUNC(ChdGetGeomName,         1,  IFchdGetGeomName);
    TCL_FUNC(ChdClearGeometry,       1,  IFchdClearGeometry);
    TCL_FUNC(ChdSetSkipFlag,         3,  IFchdSetSkipFlag);
    TCL_FUNC(ChdClearSkipFlags,      1,  IFchdClearSkipFlags);
    TCL_FUNC(ChdCompare,             10, IFchdCompare);
    TCL_FUNC(ChdCompareFlat,         11, IFchdCompareFlat);
    TCL_FUNC(ChdEdit,                3,  IFchdEdit);
    TCL_FUNC(ChdOpenFlat,            5,  IFchdOpenFlat);
    TCL_FUNC(ChdSetFlatReadTransform,3,  IFchdSetFlatReadTransform);
    TCL_FUNC(ChdEstFlatMemoryUse,    4,  IFchdEstFlatMemoryUse);
    TCL_FUNC(ChdWrite,               9,  IFchdWrite);
    TCL_FUNC(ChdWriteSplit,          9,  IFchdWriteSplit);
    TCL_FUNC(ChdCreateReferenceCell, 2,  IFchdCreateReferenceCell);
    TCL_FUNC(ChdLoadCell,            2,  IFchdLoadCell);
    TCL_FUNC(ChdIterateOverRegion,   7,  IFchdIterateOverRegion);
    TCL_FUNC(ChdWriteDensityMaps,    7,  IFchdWriteDensityMaps);

    // Cell Geometry Digest
    TCL_FUNC(OpenCellGeomDigest,     3,  IFopenCellGeomDigest);
    TCL_FUNC(NewCellGeomDigest,      0,  IFnewCellGeomDigest);
    TCL_FUNC(WriteCellGeomDigest,    2,  IFwriteCellGeomDigest);
    TCL_FUNC(CgdList,                0,  IFcgdList);
    TCL_FUNC(CgdChangeName,          2,  IFcgdChangeName);
    TCL_FUNC(CgdIsValid,             1,  IFcgdIsValid);
    TCL_FUNC(CgdDestroy,             1,  IFcgdDestroy);
    TCL_FUNC(CgdIsValidCell,         2,  IFcgdIsValidCell);
    TCL_FUNC(CgdIsValidLayer,        3,  IFcgdIsValidLayer);
    TCL_FUNC(CgdRemoveCell,          2,  IFcgdRemoveCell);
    TCL_FUNC(CgdIsCellRemoved,       2,  IFcgdIsCellRemoved);
    TCL_FUNC(CgdRemoveLayer,         3,  IFcgdRemoveLayer);
    TCL_FUNC(CgdAddCells,            3,  IFcgdAddCells);
    TCL_FUNC(CgdContents,            3,  IFcgdContents);
    TCL_FUNC(CgdOpenGeomStream,      3,  IFcgdOpenGeomStream);
    TCL_FUNC(GsReadObject,           1,  IFgsReadObject);
    TCL_FUNC(GsDumpOasisText,        1,  IFgsDumpOasisText);

    // Assembly Stream
    TCL_FUNC(StreamOpen,             1,  IFstreamOpen);
    TCL_FUNC(StreamTopCell,          2,  IFstreamTopCell);
    TCL_FUNC(StreamSource,           5,  IFstreamSource);
    TCL_FUNC(StreamInstance,         13, IFstreamInstance);
    TCL_FUNC(StreamRun,              1,  IFstreamRun);


    void tcl_register_cvrt()
    {
      // Layer Aliasing
      cTclIf::register_func("ReadLayerCvAliases",     tclReadLayerCvAliases);
      cTclIf::register_func("DumpLayerCvAliases",     tclDumpLayerCvAliases);
      cTclIf::register_func("ClearLayerCvAliases",    tclClearLayerCvAliases);
      cTclIf::register_func("AddLayerCvAlias",        tclAddLayerCvAlias);
      cTclIf::register_func("RemoveLayerCvAlias",     tclRemoveLayerCvAlias);
      cTclIf::register_func("GetLayerCvAlias",        tclGetLayerCvAlias);

      // Cell Name Mapping
      cTclIf::register_func("SetMapToLower",          tclSetMapToLower);
      cTclIf::register_func("SetMapToUpper",          tclSetMapToUpper);

      // Cell Table
      cTclIf::register_func("CellTabAdd",             tclCellTabAdd);
      cTclIf::register_func("CellTabCheck",           tclCellTabCheck);
      cTclIf::register_func("CellTabRemove",          tclCellTabRemove);
      cTclIf::register_func("CellTabList",            tclCellTabList);
      cTclIf::register_func("CellTabClear",           tclCellTabClear);

      // Windowing and Flattening
      cTclIf::register_func("SetConvertFlags",        tclSetConvertFlags);
      cTclIf::register_func("SetConvertArea",         tclSetConvertArea);

      // Scale Factor
      cTclIf::register_func("SetConvertScale",        tclSetConvertScale);

      // Export Flags
      cTclIf::register_func("SetStripForExport",      tclSetStripForExport);
      cTclIf::register_func("SetSkipInvisLayers",     tclSetSkipInvisLayers);

      // Import Flags
      cTclIf::register_func("SetMergeInRead",         tclSetMergeInRead);

      // Layout File Format Conversion
      cTclIf::register_func("FromArchive",            tclFromArchive);
      cTclIf::register_func("FromTxt",                tclFromTxt);
      cTclIf::register_func("FromNative",             tclFromNative);

      // Export Layout File
      cTclIf::register_func("SaveCellAsNative",       tclSaveCellAsNative);
      cTclIf::register_func("Export",                 tclExport);
      cTclIf::register_func("ToXIC",                  tclToXIC);
      cTclIf::register_func("ToCGX",                  tclToCGX);
      cTclIf::register_func("ToCIF",                  tclToCIF);
      cTclIf::register_func("ToGDS",                  tclToGDS);
      cTclIf::register_func("ToGdsLibrary",           tclToGdsLibrary);
      cTclIf::register_func("ToOASIS",                tclToOASIS);
      cTclIf::register_func("ToTxt",                  tclToTxt);

      // Cell Hierarchy Digest
      cTclIf::register_func("FileInfo",               tclFileInfo);
      cTclIf::register_func("OpenCellHierDigest",     tclOpenCellHierDigest);
      cTclIf::register_func("WriteCellHierDigest",    tclWriteCellHierDigest);
      cTclIf::register_func("ReadCellHierDigest",     tclReadCellHierDigest);
      cTclIf::register_func("ChdList",                tclChdList);
      cTclIf::register_func("ChdChangeName",          tclChdChangeName);
      cTclIf::register_func("ChdIsValid",             tclChdIsValid);
      cTclIf::register_func("ChdDestroy",             tclChdDestroy);
      cTclIf::register_func("ChdInfo",                tclChdInfo);
      cTclIf::register_func("ChdFileName",            tclChdFileName);
      cTclIf::register_func("ChdFileType",            tclChdFileType);
      cTclIf::register_func("ChdTopCells",            tclChdTopCells);
      cTclIf::register_func("ChdListCells",           tclChdListCells);
      cTclIf::register_func("ChdLayers",              tclChdLayers);
      cTclIf::register_func("ChdInfoMode",            tclChdInfoMode);
      cTclIf::register_func("ChdInfoLayers",          tclChdInfoLayers);
      cTclIf::register_func("ChdInfoCells",           tclChdInfoCells);
      cTclIf::register_func("ChdInfoCounts",          tclChdInfoCounts);
      cTclIf::register_func("ChdCellBB",              tclChdCellBB);
      cTclIf::register_func("ChdSetDefCellName",      tclChdSetDefCellName);
      cTclIf::register_func("ChdDefCellName",         tclChdDefCellName);
      cTclIf::register_func("ChdLoadGeometry",        tclChdLoadGeometry);
      cTclIf::register_func("ChdLinkCgd",             tclChdLinkCgd);
      cTclIf::register_func("ChdGetGeomName",         tclChdGetGeomName);
      cTclIf::register_func("ChdClearGeometry",       tclChdClearGeometry);
      cTclIf::register_func("ChdSetSkipFlag",         tclChdSetSkipFlag);
      cTclIf::register_func("ChdClearSkipFlags",      tclChdClearSkipFlags);
      cTclIf::register_func("ChdCompare",             tclChdCompare);
      cTclIf::register_func("ChdCompareFlat",         tclChdCompareFlat);
      cTclIf::register_func("ChdEdit",                tclChdEdit);
      cTclIf::register_func("ChdOpenFlat",            tclChdOpenFlat);
      cTclIf::register_func("ChdSetFlatReadTransform",tclChdSetFlatReadTransform);
      cTclIf::register_func("ChdEstFlatMemoryUse",    tclChdEstFlatMemoryUse);
      cTclIf::register_func("ChdWrite",               tclChdWrite);
      cTclIf::register_func("ChdWriteSplit",          tclChdWriteSplit);
      cTclIf::register_func("ChdCreateReferenceCell", tclChdCreateReferenceCell);
      cTclIf::register_func("ChdLoadCell",            tclChdLoadCell);
      cTclIf::register_func("ChdIterateOverRegion",   tclChdIterateOverRegion);
      cTclIf::register_func("ChdWriteDensityMaps",    tclChdWriteDensityMaps);

      // Cell Geometry Digest
      cTclIf::register_func("OpenCellGeomDigest",     tclOpenCellGeomDigest);
      cTclIf::register_func("NewCellGeomDigest",      tclNewCellGeomDigest);
      cTclIf::register_func("WriteCellGeomDigest",    tclWriteCellGeomDigest);
      cTclIf::register_func("CgdList",                tclCgdList);
      cTclIf::register_func("CgdChangeName",          tclCgdChangeName);
      cTclIf::register_func("CgdIsValid",             tclCgdIsValid);
      cTclIf::register_func("CgdDestroy",             tclCgdDestroy);
      cTclIf::register_func("CgdIsValidCell",         tclCgdIsValidCell);
      cTclIf::register_func("CgdIsValidLayer",        tclCgdIsValidLayer);
      cTclIf::register_func("CgdRemoveCell",          tclCgdRemoveCell);
      cTclIf::register_func("CgdIsCellRemoved",       tclCgdIsCellRemoved);
      cTclIf::register_func("CgdRemoveLayer",         tclCgdRemoveLayer);
      cTclIf::register_func("CgdAddCells",            tclCgdAddCells);
      cTclIf::register_func("CgdContents",            tclCgdContents);
      cTclIf::register_func("CgdOpenGeomStream",      tclCgdOpenGeomStream);
      cTclIf::register_func("GsReadObject",           tclGsReadObject);
      cTclIf::register_func("GsDumpOasisText",        tclGsDumpOasisText);

      // Assembly Stream
      cTclIf::register_func("StreamOpen",             tclStreamOpen);
      cTclIf::register_func("StreamTopCell",          tclStreamTopCell);
      cTclIf::register_func("StreamSource",           tclStreamSource);
      cTclIf::register_func("StreamInstance",         tclStreamInstance);
      cTclIf::register_func("StreamRun",              tclStreamRun);
    }
#endif  // HAVE_TCL
}


void
cConvert::load_funcs_cvrt()
{
  using namespace cvrt_funcs;

  // Layer Aliasing
  SIparse()->registerFunc("ReadLayerCvAliases",     1,  IFreadLayerCvAliases);
  SIparse()->registerFunc("DumpLayerCvAliases",     1,  IFdumpLayerCvAliases);
  SIparse()->registerFunc("ClearLayerCvAliases",    0,  IFclearLayerCvAliases);
  SIparse()->registerFunc("AddLayerCvAlias",        2,  IFaddLayerCvAlias);
  SIparse()->registerFunc("RemoveLayerCvAlias",     1,  IFremoveLayerCvAlias);
  SIparse()->registerFunc("GetLayerCvAlias",        1,  IFgetLayerCvAlias);

  // Cell Name Mapping
  SIparse()->registerFunc("SetMapToLower",          2,  IFsetMapToLower);
  SIparse()->registerFunc("SetMapToUpper",          2,  IFsetMapToUpper);

  // Cell Table
  SIparse()->registerFunc("CellTabAdd",             2,  IFcellTabAdd);
  SIparse()->registerFunc("CellTabCheck",           1,  IFcellTabCheck);
  SIparse()->registerFunc("CellTabRemove",          1,  IFcellTabRemove);
  SIparse()->registerFunc("CellTabList",            0,  IFcellTabList);
  SIparse()->registerFunc("CellTabClear",           0,  IFcellTabClear);

  // Windowing and Flattening
  SIparse()->registerFunc("SetConvertFlags",        5,  IFsetConvertFlags);
  SIparse()->registerFunc("SetConvertArea",         5,  IFsetConvertArea);

  // Scale Factor
  SIparse()->registerFunc("SetConvertScale",        2,  IFsetConvertScale);

  // Export Flags
  SIparse()->registerFunc("SetStripForExport",      1,  IFsetStripForExport);
  SIparse()->registerFunc("SetSkipInvisLayers",     1,  IFsetSkipInvisLayers);

  // Import Flags
  SIparse()->registerFunc("SetMergeInRead",         1,  IFsetMergeInRead);

  // Layout File Format Conversion
  SIparse()->registerFunc("FromArchive",            2,  IFfromArchive);
  SIparse()->registerFunc("FromTxt",                2,  IFfromTxt);
  SIparse()->registerFunc("FromNative",             2,  IFfromNative);

  // Export Layout File
  SIparse()->registerFunc("SaveCellAsNative",       2,  IFsaveCellAsNative);
  SIparse()->registerFunc("Export",                 2,  IFexport);
  SIparse()->registerFunc("ToXIC",                  1,  IFtoXIC);
  SIparse()->registerFunc("ToCGX",                  1,  IFtoCGX);
  SIparse()->registerFunc("ToCIF",                  1,  IFtoCIF);
  SIparse()->registerFunc("ToGDS",                  1,  IFtoGDS);
  SIparse()->registerFunc("ToGdsLibrary",           2,  IFtoGdsLibrary);
  SIparse()->registerFunc("ToOASIS",                1,  IFtoOASIS);
  SIparse()->registerFunc("ToTxt",                  3,  IFtoTxt);

  // Cell Hierarchy Digest
  SIparse()->registerFunc("FileInfo",               3,  IFfileInfo);
  SIparse()->registerFunc("OpenCellHierDigest",     2,  IFopenCellHierDigest);
  SIparse()->registerFunc("WriteCellHierDigest",    4,  IFwriteCellHierDigest);
  SIparse()->registerFunc("ReadCellHierDigest",     2,  IFreadCellHierDigest);
  SIparse()->registerFunc("ChdList",                0,  IFchdList);
  SIparse()->registerFunc("ChdChangeName",          2,  IFchdChangeName);
  SIparse()->registerFunc("ChdIsValid",             1,  IFchdIsValid);
  SIparse()->registerFunc("ChdDestroy",             1,  IFchdDestroy);
  SIparse()->registerFunc("ChdInfo",                3,  IFchdInfo);
  SIparse()->registerFunc("ChdFileName",            1,  IFchdFileName);
  SIparse()->registerFunc("ChdFileType",            1,  IFchdFileType);
  SIparse()->registerFunc("ChdTopCells",            1,  IFchdTopCells);
  SIparse()->registerFunc("ChdListCells",           4,  IFchdListCells);
  SIparse()->registerFunc("ChdLayers",              1,  IFchdLayers);
  SIparse()->registerFunc("ChdInfoMode",            1,  IFchdInfoMode);
  SIparse()->registerFunc("ChdInfoLayers",          2,  IFchdInfoLayers);
  SIparse()->registerFunc("ChdInfoCells",           1,  IFchdInfoCells);
  SIparse()->registerFunc("ChdInfoCounts",          4,  IFchdInfoCounts);
  SIparse()->registerFunc("ChdCellBB",              3,  IFchdCellBB);
  SIparse()->registerFunc("ChdSetDefCellName",      2,  IFchdSetDefCellName);
  SIparse()->registerFunc("ChdDefCellName",         1,  IFchdDefCellName);
  SIparse()->registerFunc("ChdLoadGeometry",        1,  IFchdLoadGeometry);
  SIparse()->registerFunc("ChdLinkCgd",             2,  IFchdLinkCgd);
  SIparse()->registerFunc("ChdGetGeomName",         1,  IFchdGetGeomName);
  SIparse()->registerFunc("ChdClearGeometry",       1,  IFchdClearGeometry);
  SIparse()->registerFunc("ChdSetSkipFlag",         3,  IFchdSetSkipFlag);
  SIparse()->registerFunc("ChdClearSkipFlags",      1,  IFchdClearSkipFlags);
  SIparse()->registerFunc("ChdCompare",             10, IFchdCompare);
  SIparse()->registerFunc("ChdCompareFlat",         11, IFchdCompareFlat);
  SIparse()->registerFunc("ChdEdit",                3,  IFchdEdit);
  SIparse()->registerFunc("ChdOpenFlat",            5,  IFchdOpenFlat);
  SIparse()->registerFunc("ChdSetFlatReadTransform",3,  IFchdSetFlatReadTransform);
  SIparse()->registerFunc("ChdEstFlatMemoryUse",    4,  IFchdEstFlatMemoryUse);
  SIparse()->registerFunc("ChdWrite",               9,  IFchdWrite);
  SIparse()->registerFunc("ChdWriteSplit",          9,  IFchdWriteSplit);
  SIparse()->registerFunc("ChdCreateReferenceCell", 2,  IFchdCreateReferenceCell);
  SIparse()->registerFunc("ChdLoadCell",            2,  IFchdLoadCell);
  SIparse()->registerFunc("ChdIterateOverRegion",   7,  IFchdIterateOverRegion);
  SIparse()->registerFunc("ChdWriteDensityMaps",    7,  IFchdWriteDensityMaps);

  // Cell Geometry Digest
  SIparse()->registerFunc("OpenCellGeomDigest",     3,  IFopenCellGeomDigest);
  SIparse()->registerFunc("NewCellGeomDigest",      0,  IFnewCellGeomDigest);
  SIparse()->registerFunc("WriteCellGeomDigest",    2,  IFwriteCellGeomDigest);
  SIparse()->registerFunc("CgdList",                0,  IFcgdList);
  SIparse()->registerFunc("CgdChangeName",          2,  IFcgdChangeName);
  SIparse()->registerFunc("CgdIsValid",             1,  IFcgdIsValid);
  SIparse()->registerFunc("CgdDestroy",             1,  IFcgdDestroy);
  SIparse()->registerFunc("CgdIsValidCell",         2,  IFcgdIsValidCell);
  SIparse()->registerFunc("CgdIsValidLayer",        3,  IFcgdIsValidLayer);
  SIparse()->registerFunc("CgdRemoveCell",          2,  IFcgdRemoveCell);
  SIparse()->registerFunc("CgdIsCellRemoved",       2,  IFcgdIsCellRemoved);
  SIparse()->registerFunc("CgdRemoveLayer",         3,  IFcgdRemoveLayer);
  SIparse()->registerFunc("CgdAddCells",            3,  IFcgdAddCells);
  SIparse()->registerFunc("CgdContents",            3,  IFcgdContents);
  SIparse()->registerFunc("CgdOpenGeomStream",      3,  IFcgdOpenGeomStream);
  SIparse()->registerFunc("GsReadObject",           1,  IFgsReadObject);
  SIparse()->registerFunc("GsDumpOasisText",        1,  IFgsDumpOasisText);

  // Assembly Stream
  SIparse()->registerFunc("StreamOpen",             1,  IFstreamOpen);
  SIparse()->registerFunc("StreamTopCell",          2,  IFstreamTopCell);
  SIparse()->registerFunc("StreamSource",           5,  IFstreamSource);
  SIparse()->registerFunc("StreamInstance",         13, IFstreamInstance);
  SIparse()->registerFunc("StreamRun",              1,  IFstreamRun);

#ifdef HAVE_PYTHON
  py_register_cvrt();
#endif
#ifdef HAVE_TCL
  tcl_register_cvrt();
#endif
}


//-------------------------------------------------------------------------
// Layer Conversion Aliasing
//-------------------------------------------------------------------------

// (int) ReadLayerCvAliases(handle_or_filename)
//
// The argument can be either a string giving a file name, or a file
// handle as returned from the Open function or equivalent (opened for
// reading).  This function will read layer aliases, adding the
// definitions to the layer alias table.  The format consists of lines
// of the form
//
//   name=newname
//
// where both name and newname are four-character CIF-type layer
// names, and there is one definition per line.  Lines with a syntax
// error or bad layer name are silently ignored.  When the layer alias
// table is active, layers read from an input file will be substiuted,
// i.e., if a layer named name is read, it will be replaced with
// newname.  For data formats that use layer number and datatype
// numbers, such as GDSII, the layer names should be in the form of a
// four or eight-byte hex number, using upper case, where the left
// bytes represent the hex value of the layer number, zero padded, and
// the right bytes represent the zero padded datatype number.  The
// eight-byte form should be used if the layer or datatype is larger
// than 255.  Alternatvely, the decimal form L,D is accepted for
// layer tokens, where the decimal layer and datatype numbers are
// separated by a comma with no space.
//
// The function returns 1 on success, 0 otherwise.
//
bool
cvrt_funcs::IFreadLayerCvAliases(Variable *res, Variable *args, void*)
{
    if (args[0].type == TYP_HANDLE) {
        int id;
        ARG_CHK(arg_handle(args, 0, &id))

        if (id < 0)
            return (BAD);
        sHdl *hdl = sHdl::get(id);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        if (hdl) {
            if (hdl->type != HDLfd)
                return (BAD);
            FILE *fp = fdopen(id, "r");
            if (fp) {
                FIOlayerAliasTab latab;
                latab.readFile(fp);
                char *t = latab.toString(false);
                CDvdb()->setVariable(VA_LayerAlias, t);
                delete [] t;
                res->content.value = 1;
                fclose(fp);
            }
        }
    }
    else {
        const char *fname;
        ARG_CHK(arg_string(args, 0, &fname))

        if (!fname || !*fname)
            return (BAD);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        FILE *fp = fopen(fname, "r");
        if (fp) {
            FIOlayerAliasTab latab;
            latab.readFile(fp);
            char *t = latab.toString(false);
            CDvdb()->setVariable(VA_LayerAlias, t);
            delete [] t;
            res->content.value = 1;
            fclose(fp);
        }
    }
    return (OK);
}


// (int) DumpLayerCvAliases(handle_or_filename)
//
// The argument can be either a string giving a file name, or a file
// handle as returned from the Open function or equivalent (opened for
// writing).  This function will dump the layer alias table.  The
// format consists of lines of the form
//
//   name=newname
//
// one definition per line, where name and newname are CIF-type four
// character layer names, with newname being the replacement.  The
// function returns 1 on success, 0 otherwise.
//
bool
cvrt_funcs::IFdumpLayerCvAliases(Variable *res, Variable *args, void*)
{
    if (args[0].type == TYP_HANDLE) {
        int id;
        ARG_CHK(arg_handle(args, 0, &id))

        if (id < 0)
            return (BAD);
        sHdl *hdl = sHdl::get(id);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        if (hdl) {
            if (hdl->type != HDLfd)
                return (BAD);
            FILE *fp = fdopen(id, "w");
            if (fp) {
                FIOlayerAliasTab latab;
                latab.parse(CDvdb()->getVariable(VA_LayerAlias));
                latab.dumpFile(fp);
                res->content.value = 1;
                fclose(fp);
            }
        }
    }
    else {
        const char *fname;
        ARG_CHK(arg_string(args, 0, &fname))

        if (!fname || !*fname)
            return (BAD);
        res->type = TYP_SCALAR;
        res->content.value = 0;
        FILE *fp = fopen(fname, "w");
        if (fp) {
            FIOlayerAliasTab latab;
            latab.parse(CDvdb()->getVariable(VA_LayerAlias));
            latab.dumpFile(fp);
            res->content.value = 1;
            fclose(fp);
        }
    }
    return (OK);
}


// (int) ClearLayerCvAliases()
//
// This function will remove all entries in the layer alias table.
// The function always returns 1.
//
bool
cvrt_funcs::IFclearLayerCvAliases(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 1;
    CDvdb()->clearVariable(VA_LayerAlias);
    return (OK);
}


// (int) AddLayerCvAlias(lname, new_lname)
//
// This function will add the layer name string new_lname as an alias
// for the layer name string lname to the layer alias table.  If an
// error occurs, or an alias for lname already exists in the table (it
// will not be replaced) the function returns 0.  The function
// otherwise returns 1.
//
bool
cvrt_funcs::IFaddLayerCvAlias(Variable *res, Variable *args, void*)
{
    const char *lname;
    ARG_CHK(arg_string(args, 0, &lname))
    const char *new_lname;
    ARG_CHK(arg_string(args, 1, &new_lname))

    if (!lname || !new_lname)
        return (BAD);
    res->type = TYP_SCALAR;
    FIOlayerAliasTab latab;
    latab.parse(CDvdb()->getVariable(VA_LayerAlias));
    res->content.value = latab.addAlias(lname, new_lname);
    char *t = latab.toString(false);
    CDvdb()->setVariable(VA_LayerAlias, t);
    delete [] t;
    return (OK);
}


// (int) RemoveLayerCvAlias(lname)
//
// This function removes any alias for lname from the layer alias
// table.  The function always returns 1.
//
bool
cvrt_funcs::IFremoveLayerCvAlias(Variable *res, Variable *args, void*)
{
    const char *lname;
    ARG_CHK(arg_string(args, 0, &lname))

    res->type = TYP_SCALAR;
    res->content.value = 1;
    FIOlayerAliasTab latab;
    latab.parse(CDvdb()->getVariable(VA_LayerAlias));
    latab.remove(lname);
    char *t = latab.toString(false);
    CDvdb()->setVariable(VA_LayerAlias, t);
    delete [] t;
    return (OK);
}


// (string) GetLayerCvAlias(lname)
//
// This function returns a string containing the alias for the passed
// layer name string, obtained from the layer alias table.  If no
// alias exists for lname, a null string is returned.
//
bool
cvrt_funcs::IFgetLayerCvAlias(Variable *res, Variable *args, void*)
{
    const char *lname;
    ARG_CHK(arg_string(args, 0, &lname))

    if (!lname)
        return (BAD);
    res->type = TYP_STRING;
    FIOlayerAliasTab latab;
    latab.parse(CDvdb()->getVariable(VA_LayerAlias));
    res->content.string = lstring::copy(latab.alias(lname));
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


//-------------------------------------------------------------------------
// Cell Name Mapping
//-------------------------------------------------------------------------

// (int) SetMapToLower(state, rw)
//
// This function sets a flag which causes upper case cell names to be
// mapped to lower case when reading, writing, or format converting
// archive files.  The first argument is a boolean value which if
// nonzero indicates case conversion will be applied, and if zero case
// conversion will be disabled.
//
// The second argument is a boolean value that if zero indicates that
// case conversion will be appied when reading or format converting
// archive files, and nonzero will apply case conversion when writing
// an archive file from memory.
//
// Within Xic, this flag can also be set from the panels available
// from the Convert Menu.  The internal effect is to set or clear the
// InToUpper or OutToUpper variables.  The return value is the
// previous setting of the variable.
//
bool
cvrt_funcs::IFsetMapToLower(Variable *res, Variable *args, void*)
{
    bool state;
    ARG_CHK(arg_boolean(args, 0, &state))
    bool rw;
    ARG_CHK(arg_boolean(args, 1, &rw))

    res->type = TYP_SCALAR;
    if (rw) {
        res->content.value = CDvdb()->getVariable(VA_OutToLower) ? 1 : 0;
        if (state)
            CDvdb()->setVariable(VA_OutToLower, 0);
        else
            CDvdb()->clearVariable(VA_OutToLower);
    }
    else {
        res->content.value = CDvdb()->getVariable(VA_InToLower) ? 1 : 0;
        if (state)
            CDvdb()->setVariable(VA_InToLower, 0);
        else
            CDvdb()->clearVariable(VA_InToLower);
    }
    return (OK);
}


// (int) SetMapToUpper(state, rw)
//
// This function sets a flag which causes lower case cell names to be
// mapped to upper case when reading, writing, or format converting
// archive files.  The first argument is a boolean value which if
// nonzero indicates case conversion will be applied, and if zero case
// conversion will be disabled.
//
// The second argument is a boolean value that if zero indicates that
// case conversion will be appied when reading or format converting
// archive files, and nonzero will apply case conversion when writing
// an archive file from memory.
//
// Within Xic, this flag can also be set from the panels available
// from the Convert menu.  The internal effect is to set or clear the
// InToUpper or OutToUpper variables.  The return value is the
// previous setting of the variable.
//
bool
cvrt_funcs::IFsetMapToUpper(Variable *res, Variable *args, void*)
{
    bool state;
    ARG_CHK(arg_boolean(args, 0, &state))
    bool rw;
    ARG_CHK(arg_boolean(args, 1, &rw))

    res->type = TYP_SCALAR;
    if (rw) {
        res->content.value = CDvdb()->getVariable(VA_OutToUpper) ? 1 : 0;
        if (state)
            CDvdb()->setVariable(VA_OutToUpper, 0);
        else
            CDvdb()->clearVariable(VA_OutToUpper);
    }
    else {
        res->content.value = CDvdb()->getVariable(VA_InToUpper) ? 1 : 0;
        if (state)
            CDvdb()->setVariable(VA_InToUpper, 0);
        else
            CDvdb()->clearVariable(VA_InToUpper);
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Cell Table
//-------------------------------------------------------------------------

// (int) CellTabAdd(cellname, expand)
//
// This function is used to add cell names to the cell table for the
// current symbol table.  The cellname must match a name in the global
// string table, which includes all cells read into memory or
// referenced by a CHD in memory.
//
// If the boolean argument expand is nonzero, and the name matches a
// cell in the main database, the cell and all of the cells in its
// hierarchy will be added to the table, otherwise only the named cell
// will be added.  It is not an error to add the same cell more than
// once, duplicates will be ignored.
//
// If the UseCellTab variable is set, when a Cell Hierarchy Digest
// (CHD) is used to process a cell hierarchy for anything other than
// reading cells into the main database, cells listed in the cell
// table will override cells of the same name in the CHD.  Thus, for
// example, one can substitute modified versions of cells as a layout
// file is being written.
//
// The return value is 1 if all goes well, 0 if the table is not
// initialized or the cell is not found.
//
bool
cvrt_funcs::IFcellTabAdd(Variable *res, Variable *args, void*)
{
    const char *cname;
    ARG_CHK(arg_string(args, 0, &cname))
    bool expand;
    ARG_CHK(arg_boolean(args, 1, &expand))

    if (!cname || !*cname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (CDcdb()->auxCellTab())
        res->content.value = CDcdb()->auxCellTab()->add(cname, expand);
    return (OK);
}


// (int) CellTabCheck(cellname)
//
// This function returns 1 if cellname is in the current cell table.
// If the cellname is valid but cellname is not in the table, 0 is
// returned.  If the cellname is invalid (not a known cell name) or
// the cell table is uninitialized, the return value is -1.
//
bool
cvrt_funcs::IFcellTabCheck(Variable *res, Variable *args, void*)
{
    const char *cname;
    ARG_CHK(arg_string(args, 0, &cname))

    if (!cname || !*cname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDcellName nm = CD()->CellNameTableFind(cname);
    if (nm && CDcdb()->auxCellTab())
        res->content.value = (CDcdb()->auxCellTab()->find(nm) != 0);
    else
        res->content.value = -1;
    return (OK);
}


// (int) CellTabRemove(cellname)
//
// If cellname is found in the current cell table, it will be removed.
// If the name was found in the table and removed, the return value is
// 1, otherwise the function returns 0.
//
bool
cvrt_funcs::IFcellTabRemove(Variable *res, Variable *args, void*)
{
    const char *cname;
    ARG_CHK(arg_string(args, 0, &cname))

    if (!cname || !*cname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDcellName nm = CD()->CellNameTableFind(cname);
    if (nm && CDcdb()->auxCellTab())
        res->content.value = (CDcdb()->auxCellTab()->remove(nm) != 0);
    return (OK);
}


// (stringlist_handle) CellTabList()
//
// This function returns a handle to a list of cell name strings
// obtained from the current cell table.  If the table is empty, a
// scalar 0 is returned.
//
bool
cvrt_funcs::IFcellTabList(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDcellTab *ct = CDcdb()->auxCellTab();
    if (ct) {
        stringlist *s0 = ct->list();
        if (s0) {
            sHdl *hdltmp = new sHdlString(s0);
            res->type = TYP_HANDLE;
            res->content.value = hdltmp->id;
        }
    }
    return (OK);
}


// (int) CellTabClear()
//
// This function will clear the cell table.  The function always
// returns 1.
//
bool
cvrt_funcs::IFcellTabClear(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 1;
    if (CDcdb()->auxCellTab())
        CDcdb()->auxCellTab()->clear();
    return (OK);
}


//-------------------------------------------------------------------------
// Windowing and Flattening
//-------------------------------------------------------------------------

// (int) SetConvertFlags(use_window, clip, flatten, ecf_level, rw)
//
// This function sets the status of flags used in format conversions. 
// The first three arguments correspond to the Use Window, Clip to
// Window, and Flatten Hierarchy buttons in the Conversion pop-up.  A
// nonzero integer value will set the flag, 0 will reset the flag.
//
// The ecf_level is an integer 0-3 which sets the empty cell filtering
// level, as described for the Conversion panel.  The values are
//
//   0   No empty cell filtering.
//   1   Apply pre- and post-filtering.
//   2   Apply pre-filtering only.
//   3   Apply post-filtering only.
//
// The fifth argument is a boolean value that if zero indicates that
// the flags will be applied when converting archive files, as if set
// from the Conversion panel, and nonzero will apply when writing an
// file from memory, as if set from the Write Layout File panel.

// With rw zero, The flags will affect conversions initiated from the
// Conversion panel, and the FromArchive script function.  The data
// window can be set with the SetConvertArea script function.  To
// apply clipping, both the use_window and clip flags must be set.
//
// With rw nonzero, the flags apply when writing output with the Write
// Layout File panel, or when using the Export and ToXXX script
// functions.  In this case, the no_empties flag is ignored, and the
// windowing is ignored except when flattening.
//
// This function returns the previous value of the internal variable
// that contains the flags.  The two ecf filter bits encode the filtering
// level as above.  The bits are:
//    flatten       0x1
//    use_window    0x2
//    clip          0x4
//    ecf level0    0x8
//    ecf level1    0x10
//
bool
cvrt_funcs::IFsetConvertFlags(Variable *res, Variable *args, void*)
{
    bool win;
    ARG_CHK(arg_boolean(args, 0, &win))
    bool clip;
    ARG_CHK(arg_boolean(args, 1, &clip))
    bool flat;
    ARG_CHK(arg_boolean(args, 2, &flat))
    int ecf_level;
    ARG_CHK(arg_int(args, 3, &ecf_level))
    bool rw;
    ARG_CHK(arg_boolean(args, 4, &rw))

    ecf_level &= 0x3;

    res->type = TYP_SCALAR;
    int f = 0;
    if (!rw) {
        if (FIO()->CvtFlatten())
            f |= 0x1;
        if (FIO()->CvtUseWindow())
            f |= 0x2;
        if (FIO()->CvtClip())
            f |= 0x4;
        if (FIO()->CvtECFlevel() == ECFall || FIO()->CvtECFlevel() == ECFpost)
            f |= 0x8;
        if (FIO()->CvtECFlevel() == ECFpre || FIO()->CvtECFlevel() == ECFpost)
            f |= 0x10;

        res->content.value = f;
        FIO()->SetCvtUseWindow(win);
        FIO()->SetCvtClip(clip);
        FIO()->SetCvtFlatten(flat);
        FIO()->SetCvtECFlevel((ECFlevel)ecf_level);
    }
    else {
        if (FIO()->OutFlatten())
            f |= 0x1;
        if (FIO()->OutUseWindow())
            f |= 0x2;
        if (FIO()->OutClip())
            f |= 0x4;
        if (FIO()->OutECFlevel() == ECFall || FIO()->OutECFlevel() == ECFpost)
            f |= 0x8;
        if (FIO()->OutECFlevel() == ECFpre || FIO()->OutECFlevel() == ECFpost)
            f |= 0x10;

        res->content.value = f;
        FIO()->SetOutUseWindow(win);
        FIO()->SetOutClip(clip);
        FIO()->SetOutFlatten(flat);
        FIO()->SetOutECFlevel((ECFlevel)ecf_level);
    }
    Cvt()->UpdatePopUps();
    return (OK);
}


// (int) SetConvertArea(l, b, r, t, rw)
//
// This function sets the rectangular window used to filter or clip
// objects during format conversions or file writing.  The first four
// arguments are the window coordinates in microns, in the coordinate
// system of the top level cell, after scaling (if any).
//
// The fifth argument is a boolean value that if zero indicates that
// the values will be applied when converting archive files, as if set
// from the Conversion panel, and nonzero will apply when writing a
// file from memory, as if set from the Write Layout File panel.
//
// Use of the window can be enabled with the SetConvertFlags script
// function.
//
// With rw zero, the window will affect conversions initiated from the
// Conversion panel, and the FromArchive script function.  With rw
// nonzero, the flags apply when writing output with the Write Layout
// File panel, or when using the Export and ToXXX script functions. 
// In this case, windowing is ignored except when flattening.
//
// The function always returns 1.
//
bool
cvrt_funcs::IFsetConvertArea(Variable *res, Variable *args, void*)
{
    BBox BB;
    ARG_CHK(arg_coord(args, 0, &BB.left, Physical))
    ARG_CHK(arg_coord(args, 1, &BB.bottom, Physical))
    ARG_CHK(arg_coord(args, 2, &BB.right, Physical))
    ARG_CHK(arg_coord(args, 3, &BB.top, Physical))
    bool rw;
    ARG_CHK(arg_boolean(args, 4, &rw))
    BB.fix();

    if (!rw)
        FIO()->SetCvtWindow(&BB);
    else
        FIO()->SetOutWindow(&BB);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


//-------------------------------------------------------------------------
// Scale Factor
//-------------------------------------------------------------------------

// (real) SetConvertScale(real, which)
//
// This sets the scale used for conversions.  There are three such
// scales, and the one to set is specified by the second argument,
// which is an integer 0-2.
//   which = 0:
// Set the scale used when converting an archive file directly to
// another format, with the FromArchive() script function or similar
// or with the Conversion pop-up.
//   which = 1:
// Set the scale used when writing a file with the Export and ToXXX()
// script functions or similar, or the Conversion - Export panel.
//   which = 2:
// Set the scale used when reading a file into Xic with the Edit() or
// OpenCell() functions and similar, or from the Convert - Import
// panel in Xic.
//
// Script functions that read, write, or convert archive file data will
// in general make use of one of these scale factors, however if the
// function takes a scale value as an argument, that value will be used
// rather than the values set with this function.
//
// The scale argument is a real value in the inclusive range 0.001 -
// 1000.0.  The return value is the previous scale value.
//
bool
cvrt_funcs::IFsetConvertScale(Variable *res, Variable *args, void*)
{
    double scale;
    ARG_CHK(arg_real(args, 0, &scale))
    int which;
    ARG_CHK(arg_int(args, 1, &which))
    if (which < 0 || which > 2)
        return (BAD);

    if (scale < 0)
        scale = -scale;
    if (scale < CDSCALEMIN)
        scale = CDSCALEMIN;
    else if (scale > CDSCALEMAX)
        scale = CDSCALEMAX;
    res->type = TYP_SCALAR;
    if (which == 0) {
        res->content.value = FIO()->TransScale();
        FIO()->SetTransScale(scale);
        Cvt()->PopUpConvert(0, MODE_UPD, 0, 0, 0);
    }
    else if (which == 1) {
        res->content.value = FIO()->WriteScale();
        FIO()->SetWriteScale(scale);
        Cvt()->PopUpExport(0, MODE_UPD, 0, 0);
    }
    else {
        res->content.value = FIO()->ReadScale();
        FIO()->SetReadScale(scale);
        Cvt()->PopUpImport(0, MODE_UPD, 0, 0);
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Export Flags
//-------------------------------------------------------------------------

// (int) SetStripForExport(state)
//
// This function sets the state of the Strip For Export flag.  When
// set, output from the conversion functions will contain physical
// information only.  This should be applied when generating output
// for mask fabrication.  See the Conversion - Export panel
// description for more information.  If the integer argument is
// nonzero, the state will be set active.  The return value is the
// previous state of the flag.
//
bool
cvrt_funcs::IFsetStripForExport(Variable *res, Variable *args, void*)
{
    bool state;
    ARG_CHK(arg_boolean(args, 0, &state))

    res->type = TYP_SCALAR;
    res->content.value = CDvdb()->getVariable(VA_StripForExport) ? 1 : 0;
    if (state)
        CDvdb()->setVariable(VA_StripForExport, 0);
    else
        CDvdb()->clearVariable(VA_StripForExport);
    return (OK);
}


// (int) SetSkipInvisLayers(code)
//
// This function sets the variable which controls how invisible layers
// are treated by the output conversion functions.  Layer visiblity is
// set by clicking in the layer table with mouse button 2, or through
// the SetLayerVisible() script function.  If code is 0 or negative,
// invisible layers will be converted.  If code is 1, invisible
// physical layers will not be converted.  If code is 2, invisible
// electrical layers will not be converted.  if code is 3 or larger,
// both electrical and physical invisible layers will not be
// converted.  The return value is the previous code, which represents
// the state of the SkipInvisible variable, and the check boxes in the
// Conversion - Export panel.
//
bool
cvrt_funcs::IFsetSkipInvisLayers(Variable *res, Variable *args, void*)
{
    int code;
    ARG_CHK(arg_int(args, 0, &code))

    res->type = TYP_SCALAR;
    int oldcode = 0;
    const char *s = CDvdb()->getVariable(VA_SkipInvisible);
    if (s) {
        if (*s != 'p' && *s != 'P')
            oldcode |= 1;
        if (*s != 'e' && *s != 'E')
            oldcode |= 2;
    }
    res->content.value = oldcode;
    if (code <= 0)
        CDvdb()->clearVariable(VA_SkipInvisible);
    else if (code >= 3)
        CDvdb()->setVariable(VA_SkipInvisible, 0);
    else if (code == 1)
        CDvdb()->setVariable(VA_SkipInvisible, "p");
    else
        CDvdb()->setVariable(VA_SkipInvisible, "e");
    return (OK);
}


//-------------------------------------------------------------------------
// Import Flags
//-------------------------------------------------------------------------

// (int) SetMergeInRead(state)
//
// This function controls the setting of an internal flag which
// enables merging of boxes and wires while a file is being read.
// This flag is set from with Xic in the Conversions - Input panel.
// If the integer argument is nonzero, the flag will be set.  The
// return value is the previous state of the flag.
//
bool
cvrt_funcs::IFsetMergeInRead(Variable *res, Variable *args, void*)
{
    bool state;
    ARG_CHK(arg_boolean(args, 0, &state))

    res->type = TYP_SCALAR;
    res->content.value = CDvdb()->getVariable(VA_MergeInput) ? 1 : 0;
    if (state)
        CDvdb()->setVariable(VA_MergeInput, 0);
    else
        CDvdb()->clearVariable(VA_MergeInput);
    return (OK);
}


//-------------------------------------------------------------------------
// Layout File Format Conversion
//-------------------------------------------------------------------------

// (int) FromArchive(archive_file_or_chd, destination)
//
// This function will read an archive (GDSII, CIF, CGX, or OASIS) file
// and translate the contents to another format.  The
// archive_file_or_chd argument is a string giving a path to the
// source archive file, or the name of a Cell Hierarchy Digest (CHD)
// in memory.
//
// The type of file written is implied by the destination.  If the
// destination is null or empty, native cell files will be created in
// the current directory.  If the destination is the name of an
// existing directory, native cell files will be created in that
// directory.  Otherwise, the extension of the destination determines
// the file
// type:
//  CGX     .cgx
//  CIF     .cif
//  GDSII   .gds, .str, .strm, .stream
//  OASIS   .oas
// Only these extensions are recognized, however CGX and GDSII allow
// an additional .gz which will imply compression.
//
// See this table for the features that apply during a call to this
// function.
//
// The value 1 is returned on success, 0 otherwise, with possibly an
// error message available from GetError.
//
bool
cvrt_funcs::IFfromArchive(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    const char *dstn;
    ARG_CHK(arg_string(args, 1, &dstn))

    if (!name || !*name)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;

    FileType ftype = Fnone;
    cCHD *chd = CDchd()->chdRecall(name, false);
    if (chd) {
        ftype = chd->filetype();
    }
    else {
        FILE *fp = FIO()->POpen(name, "rb");
        if (!fp) {
            Errs()->add_error("can't open file: %s", name);
            return (BAD);
        }
        ftype = FIO()->GetFileType(fp);
        if (!FIO()->IsSupportedArchiveFormat(ftype)) {
            Errs()->add_error("not an archive format: %s", name);
            fclose (fp);
            return (BAD);
        }
        fclose (fp);
    }

    FileType dst_ft = FIO()->TypeExt(dstn);
    if (dst_ft == Fnone && (!dstn || !*dstn || filestat::is_directory(dstn)))
        dst_ft = Fnative;

    FIOcvtPrms prms;
    prms.set_scale(FIO()->TransScale());
    if (FIO()->CvtUseWindow()) {
        prms.set_use_window(true);
        prms.set_window(FIO()->CvtWindow());
        prms.set_clip(FIO()->CvtClip());
    }
    prms.set_flatten(FIO()->CvtFlatten());
    prms.set_ecf_level(FIO()->CvtECFlevel());
    prms.set_alias_mask(CVAL_CASE | CVAL_PFSF | CVAL_FILE);
    prms.set_allow_layer_mapping(true);
    prms.set_destination(dstn, dst_ft);

    if (ftype == Fgds)
        res->content.value = FIO()->ConvertFromGds(name, &prms);
    else if (ftype == Fcgx)
        res->content.value = FIO()->ConvertFromCgx(name, &prms);
    else if (ftype == Fcif)
        res->content.value = FIO()->ConvertFromCif(name, &prms);
    else if (ftype == Foas)
        res->content.value = FIO()->ConvertFromOas(name, &prms);
    else {
        Errs()->add_error("unknown file type");
        return (BAD);
    }
    return (OK);
}


// (int) FromTxt(text_file, gds_file)
//
// This function will translate a text file in the format produced by
// the ToTxt command into a GDSII format file.  This is useful after
// text mode editing has been performed on the file, to repair
// corruption or incompatibilities.  If gds_file is null of empty,
// the name is generated from the text_file and given a ".gds" suffix.
//
bool
cvrt_funcs::IFfromTxt(Variable *res, Variable *args, void*)
{
    const char *textfile;
    ARG_CHK(arg_string(args, 0, &textfile))
    const char *gdsfile;
    ARG_CHK(arg_string(args, 1, &gdsfile))

    if (!textfile || !*textfile)
        return (BAD);
    if (!DSP()->CurCellName())
        return (BAD);
    char buf[256];
    if (!gdsfile || !*gdsfile) {
        strcpy(buf, textfile);
        char *t = lstring::strip_path(buf);
        char *s = strrchr(t, '.');
        if (s && s != t)
            strcpy(s, ".gds");
        else
            strcat(buf, ".gds");
        gdsfile = buf;
    }
    res->type = TYP_SCALAR;
    res->content.value = FIO()->ConvertFromGdsText(textfile, gdsfile);
    return (OK);
}


// (int) FromNative(archive_file, dir_path)
//
// This function will translate native cell files found in the
// directory given in dir_path into an archive file given in the first
// argument.  The format of the archive file produced is determined by
// the file extension provided, as for the FromArchive function.  All
// native cell files found in the directory, except those with a
// ".bak" extension or whose name is the same as a device library
// symbol, are translated and concatenated, independently of any
// hierarchical relationship between the cells.
//
// See this table for the features that apply during a call to this
// function.  The supported manipulations are cell name aliasing,
// layer filtering, and scaling.  Windowing manipulations and
// flattening are not supported.  If a file named "aliases.alias"
// exists in the dir_path, it will be used as an input alias list for
// conversion.  Each line consists of a native cell name followed by
// an alias to be used in the archive file, separated by white space.
//
// The value 1 is returned on success, 0 otherwise, with possibly an
// error message available from GetError.
//
bool
cvrt_funcs::IFfromNative(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    const char *dirpath;
    ARG_CHK(arg_string(args, 1, &dirpath))

    if (!name || !*name || !dirpath || !*dirpath)
        return (BAD);

    res->type = TYP_SCALAR;

    FILE *fp = FIO()->POpen(name, "rb");
    if (!fp) {
        Errs()->add_error("can't open file: %s", name);
        return (OK);
    }
    FileType ftype = FIO()->GetFileType(fp);
    if (!FIO()->IsSupportedArchiveFormat(ftype)) {
        Errs()->add_error("not a supported format: %s", name);
        fclose (fp);
        return (OK);
    }
    fclose (fp);

    FIOcvtPrms prms;
    prms.set_scale(FIO()->TransScale());
    prms.set_alias_mask(CVAL_CASE | CVAL_PFSF | CVAL_FILE);
    prms.set_allow_layer_mapping(true);
    prms.set_destination(name, ftype);
    res->content.value = FIO()->ConvertFromNative(dirpath, &prms);

    return (OK);
}


//-------------------------------------------------------------------------
// Export Layout File
//-------------------------------------------------------------------------

// SaveCellAsNative(cellname, directory)
//
// Save the cell named in the first (string) argument, which must
// exist in the current symbol table, to a native format file in the
// directory.  If the directory string is null or empty (or 0 is
// passed for this argument), the cell is saved in the current
// directory.  The function returns 1 if the save was successful, 0
// otherwise.
//
// See this table for the features that apply during a call to this
// function.
//
// This functions returns 1 on success, 0 otherwise, with an error
// message likely available from GetError.
//
bool
cvrt_funcs::IFsaveCellAsNative(Variable *res, Variable *args, void*)
{
    const char *cellname;
    ARG_CHK(arg_string(args, 0, &cellname))
    const char *dirname;
    ARG_CHK(arg_string(args, 1, &dirname))

    if (!cellname || !*cellname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDcbin cbin;
    if (CDcdb()->findSymbol(cellname, &cbin)) {
        if (dirname && *dirname) {
            char *p = pathlist::mk_path(dirname, cellname);
            res->content.value = FIO()->WriteNative(&cbin, p);
            delete [] p;
        }
        else
            res->content.value = FIO()->WriteNative(&cbin, cellname);
    }
    else
        Errs()->add_error("Named cell not found");
    return (OK);
}


// (int) Export(filepath, allcells)
//
// This function exports design data to a disk file (or files).  It
// can perform the same operations as the ToXXX functions also
// described in this section.  The type of file produced is set by the
// extension found on the filepath string.  Recognized extensions are
//
//  native  .xic
//  CGX     .cgx
//  CIF     .cif
//  GDSII   .gds, .str, .strm, .stream
//  OASIS   .oas
//
// Only these extensions are recognized, however CGX and GDSII allow
// an additional ".gz" which will imply compression.  For native cell
// file output, the filepath must provide a path to an existing
// directory.  If none of the other formats is matched, and the
// filepath exists as a directory, then native cell files will be
// written to that directory.  Alternatively, if the filepath has a
// ".xic" extension, and the filepath with the .xic stripped is an
// existing directory, or the filepath including the .xic is an
// existing directory (checked in this order), again native cell files
// will be written to that directory.
//
// The second argument is a boolean.  If false, then the current cell
// hierarchy is written to output.  If true, all cells found in the
// current symbol table will be written to output.  In either case, by
// default cells that are sub-masters or library cells are not written
// unless the controlling variables are set, as from the Write Layout
// File panel.  The other controls for windowing, flattening, scaling,
// and cell name mapping found in this panel apply as well, as do
// their underlying variables.  These flags and values can also be set
// with the SetConvertFlags, SetConvertArea, and SetConvertScale
// functions, and others that apply to output generation.  When
// writing all files, any windowing or flattening in force is ignored.
//
// See this table for the features that apply during a call to this
// function.
//
// The function return 1 on success, 0 otherwise with an error message
// available from GetError.
//
bool
cvrt_funcs::IFexport(Variable *res, Variable *args, void*)
{
    const char *dstn;
    ARG_CHK(arg_string(args, 0, &dstn))
    bool allcells;
    ARG_CHK(arg_boolean(args, 1, &allcells))

    res->type = TYP_SCALAR;
    res->content.value = Cvt()->Export(dstn, allcells);
    return (OK);
}


// (int) ToXIC(distination_dir)
//
// The ToXIC command will write the current cell hierarchy to disk
// files in native format, no questions asked.  The argument is the
// directory where the Xic files will be created.  If this argument is
// a null or empty string or zero, the Xic files will be created in
// the current directory.
//
// See this table for the features that apply during a call to this
// function.
//
// This functions returns 1 on success, 0 otherwise, with an error
// message likely available from GetError.
//
bool
cvrt_funcs::IFtoXIC(Variable *res, Variable *args, void*)
{
    const char *dstn;
    ARG_CHK(arg_string(args, 0, &dstn))

    if (!DSP()->CurCellName())
        return (BAD);

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    res->type = TYP_SCALAR;
    CDcbin cbin(DSP()->CurCellName());
    if (!cbin.isSubcell())
        EditIf()->assignGlobalProperties(&cbin);
    stringlist *namelist =
        new stringlist(lstring::copy(Tstring(DSP()->CurCellName())), 0);

    FIOcvtPrms prms;
    prms.set_scale(FIO()->WriteScale());
    prms.set_alias_mask(CVAL_CASE | CVAL_PFSF | CVAL_FILE);
    if (FIO()->OutFlatten()) {
        prms.set_flatten(true);
        if (FIO()->OutUseWindow()) {
            prms.set_use_window(true);
            prms.set_window(FIO()->OutWindow());
            prms.set_clip(FIO()->OutClip());
        }
    }
    prms.set_destination(dstn, Fnative);
    res->content.value = FIO()->ConvertToNative(namelist, &prms);

    stringlist::destroy(namelist);
    return (OK);
}


// (int) ToCGX(cgx_name)
//
// This function will write the current cell hierarchy to a CGX format
// file on disk.  The argument is the name of the CGX file to create.
// If the cgx_name is NULL or the empty string, the name used will be
// the top level cell name suffixed with .cgx.
//
// See this table for the features that apply during a call to this
// function.
//
// This functions returns 1 on success, 0 otherwise, with an error
// message likely available from GetError.
//
bool
cvrt_funcs::IFtoCGX(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))

    if (!DSP()->CurCellName())
        return (BAD);
    char buf[128];
    if (!name || !*name) {
        strcpy(buf, Tstring(DSP()->CurCellName()));
        strcat(buf, ".cgx");
        name = buf;
    }

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    res->type = TYP_SCALAR;
    CDcbin cbin(DSP()->CurCellName());
    if (!cbin.isSubcell())
        EditIf()->assignGlobalProperties(&cbin);
    stringlist *namelist =
        new stringlist(lstring::copy(Tstring(DSP()->CurCellName())), 0);

    FIOcvtPrms prms;
    prms.set_scale(FIO()->WriteScale());
    prms.set_alias_mask(CVAL_CASE | CVAL_PFSF | CVAL_FILE);
    if (FIO()->OutFlatten()) {
        prms.set_flatten(true);
        if (FIO()->OutUseWindow()) {
            prms.set_use_window(true);
            prms.set_window(FIO()->OutWindow());
            prms.set_clip(FIO()->OutClip());
        }
    }
    prms.set_destination(name, Fcgx);
    res->content.value = FIO()->ConvertToCgx(namelist, &prms);

    stringlist::destroy(namelist);
    return (OK);
}


// (int) ToCIF(cif_name)
//
// This function will write the current cell hierarchy to a CIF format
// file on disk.  The argument is the name of the CIF file to create.
// If the cif_name is NULL or the empty string, the name used will be
// the top level cell name suffixed with .cif.
//
// See this table for the features that apply during a call to this
// function.
//
// This functions returns 1 on success, 0 otherwise, with an error
// message likely available from GetError.
//
bool
cvrt_funcs::IFtoCIF(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))

    if (!DSP()->CurCellName())
        return (BAD);
    char buf[128];
    if (!name || !*name) {
        strcpy(buf, Tstring(DSP()->CurCellName()));
        strcat(buf, ".cif");
        name = buf;
    }

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    res->type = TYP_SCALAR;
    CDcbin cbin(DSP()->CurCellName());
    if (!cbin.isSubcell())
        EditIf()->assignGlobalProperties(&cbin);
    stringlist *namelist =
        new stringlist(lstring::copy(Tstring(DSP()->CurCellName())), 0);

    FIOcvtPrms prms;
    prms.set_scale(FIO()->WriteScale());
    prms.set_alias_mask(CVAL_CASE | CVAL_PFSF | CVAL_FILE);
    if (FIO()->OutFlatten()) {
        prms.set_flatten(true);
        if (FIO()->OutUseWindow()) {
            prms.set_use_window(true);
            prms.set_window(FIO()->OutWindow());
            prms.set_clip(FIO()->OutClip());
        }
    }
    prms.set_destination(name, Fcif);
    res->content.value = FIO()->ConvertToCif(namelist, &prms);

    stringlist::destroy(namelist);
    return (OK);
}


// (int) ToGDS(gds_name)
//
// This function will write the current cell hierarchy to a GDSII
// format file on disk.  The argument is the name of the GDSII file to
// create.  If the gds_name is NULL or the empty string, the name used
// will be the top level cell name suffixed with .gds.
//
// See this table for the features that apply during a call to this
// function.
//
// This functions returns 1 on success, 0 otherwise, with an error
// message likely available from GetError.
//
bool
cvrt_funcs::IFtoGDS(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))

    if (!DSP()->CurCellName())
        return (BAD);
    char buf[128];
    if (!name || !*name) {
        strcpy(buf, Tstring(DSP()->CurCellName()));
        strcat(buf, ".gds");
        name = buf;
    }

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    res->type = TYP_SCALAR;
    CDcbin cbin(DSP()->CurCellName());
    if (!cbin.isSubcell())
        EditIf()->assignGlobalProperties(&cbin);
    stringlist *namelist =
        new stringlist(lstring::copy(Tstring(DSP()->CurCellName())), 0);

    FIOcvtPrms prms;
    prms.set_scale(FIO()->WriteScale());
    prms.set_alias_mask(CVAL_CASE | CVAL_PFSF | CVAL_FILE);
    if (FIO()->OutFlatten()) {
        prms.set_flatten(true);
        if (FIO()->OutUseWindow()) {
            prms.set_use_window(true);
            prms.set_window(FIO()->OutWindow());
            prms.set_clip(FIO()->OutClip());
        }
    }
    prms.set_destination(name, Fgds);
    res->content.value = FIO()->ConvertToGds(namelist, &prms);

    stringlist::destroy(namelist);
    return (OK);
}


// (int) ToGdsLibrary(gds_name, symbol_list)
//
// This function will create a GDSII file from a list of cells in
// memory.  The first argument is the name of the GDSII file to
// create.  The second argument is a string consisting of
// space-separated cell names.  The cells must be in memory, in the
// current symbol table.  Both arguments must provide values as there
// are no defaults.  The GDSII file will contain the hierarchy under
// each cell given, but any cell is added once only.  The resulting
// file will in general contain multiple top-level cells.
//
// See this table for the features that apply during a call to this
// function.
//
// This functions returns 1 on success, 0 otherwise, with an error
// message likely available from GetError.
//
bool
cvrt_funcs::IFtoGdsLibrary(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    const char *symlist;
    ARG_CHK(arg_string(args, 1, &symlist))

    if (!name || !*name || !symlist || !*symlist)
        return (BAD);
    if (!DSP()->CurCellName())
        return (BAD);

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    stringlist *namelist = 0, *ne = 0;
    char *tok;
    while ((tok = lstring::gettok(&symlist)) != 0) {
        if (!namelist)
            namelist = ne = new stringlist(tok, 0);
        else {
            ne->next = new stringlist(tok, 0);
            ne = ne->next;
        }
    }

    res->type = TYP_SCALAR;

    FIOcvtPrms prms;
    prms.set_scale(FIO()->WriteScale());
    prms.set_alias_mask(CVAL_CASE | CVAL_PFSF | CVAL_FILE);
    prms.set_flatten(FIO()->OutFlatten());
    prms.set_destination(name, Fgds);
    res->content.value = FIO()->ConvertToGds(namelist, &prms);

    stringlist::destroy(namelist);
    return (OK);
}


// (int) ToOASIS(oas_name)
//
// This function will write the current cell hierarchy to an OASIS
// format file on disk.  The argument is the name of the OASIS file to
// create.  If the oas_name is NULL or the empty string, the name used
// will be the top level cell name suffixed with .oas.
//
// See this table for the features that apply during a call to this
// function.
//
// This functions returns 1 on success, 0 otherwise, with an error
// message likely available from GetError.
//
bool
cvrt_funcs::IFtoOASIS(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))

    if (!DSP()->CurCellName())
        return (BAD);
    char buf[128];
    if (!name || !*name) {
        strcpy(buf, Tstring(DSP()->CurCellName()));
        strcat(buf, ".oas");
        name = buf;
    }

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    res->type = TYP_SCALAR;
    CDcbin cbin(DSP()->CurCellName());
    if (!cbin.isSubcell())
        EditIf()->assignGlobalProperties(&cbin);
    stringlist *namelist =
        new stringlist(lstring::copy(Tstring(DSP()->CurCellName())), 0);

    FIOcvtPrms prms;
    prms.set_scale(FIO()->WriteScale());
    prms.set_alias_mask(CVAL_CASE | CVAL_PFSF | CVAL_FILE);
    if (FIO()->OutFlatten()) {
        prms.set_flatten(true);
        if (FIO()->OutUseWindow()) {
            prms.set_use_window(true);
            prms.set_window(FIO()->OutWindow());
            prms.set_clip(FIO()->OutClip());
        }
    }
    prms.set_destination(name, Foas);
    res->content.value = FIO()->ConvertToOas(namelist, &prms);

    stringlist::destroy(namelist);
    return (OK);
}


// (int) ToTxt(archive_file, text_file, cmdargs)
//
// This command will create an ascii text file text_file from the
// contents of the archive file.  The human-readable text file is
// useful for diagnostics.  If text_file is null or empty, the name is
// derived from the archive_file and given a ".txt" extension.  No
// output is produced for CIF, since these are already in readable
// format.
//
// The third argument is a string, which can be passed to specify the
// range of the conversion.  If this argument is passed 0, or the
// string is null or empty, the entire archive file will be converted.
// The string is in the form
//
//   [start_offs[-end_offs]] [-r rec_count] [-c cell_count]
//
// The square brackets indicate optional terms.  The meanings are
//
//   start_offs
//     An integer, in decimal or "0x" hex format (a hex digit
//     preceded by "0x").  The printing will begin at the first record
//     with offset greater than or equal to this value.
//
//   end_offs
//     An integer in decimal or "0x" hex format.  If this value
//     is greater than start_offs, the last record printed is at most
//     the one containing this offset.  If given, this should appear
//     after a '-' character following the start_offs, with no space.
//
//   rec_count
//     A positive integer, at most this many records will be printed.
//
//   cell_count
//     A non-negative integer, At most the records for this many
//     cell definitions will be printed.  If given as 0, the records
//     from the start_offs to the next cell definition will be
//     printed.
//
// See this table for the features that apply during a call to this
// function.
//
// The function returns 1 on success, 0 otherwise with an error
// message likely available from GetError.
//
bool
cvrt_funcs::IFtoTxt(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    const char *textfile;
    ARG_CHK(arg_string(args, 1, &textfile))
    const char *cmdargs;
    ARG_CHK(arg_string(args, 2, &cmdargs))

    if (!name || !*name)
        return (BAD);
    if (!DSP()->CurCellName())
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;

    FILE *fp = FIO()->POpen(name, "rb");
    if (!fp) {
        Errs()->add_error("can't open file: %s", name);
        return (OK);
    }
    FileType ftype = FIO()->GetFileType(fp);
    if (!FIO()->IsSupportedArchiveFormat(ftype) || ftype == Fcif) {
        Errs()->add_error("not a supported format: %s", name);
        fclose (fp);
        return (OK);
    }
    fclose (fp);

    if (ftype != Fgds && ftype != Fcgx && ftype != Foas) {
        Errs()->add_error("unknown file type");
        return (OK);
    }

    char buf[256];
    if (!textfile || !*textfile) {
        strcpy(buf, name);
        char *t = lstring::strip_path(buf);
        char *s = strrchr(t, '.');
        if (s && s != t)
            strcpy(s, ".txt");
        else
            strcat(buf, ".txt");
        textfile = buf;
    }

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    res->content.value = FIO()->ConvertToText(ftype, name, textfile, cmdargs);
    return (OK);
}


//-------------------------------------------------------------------------
// Cell Hierarchy Digest
//-------------------------------------------------------------------------

// (string) FileInfo(filename, handle_or_filename, flags)
//
// This function provides information about the archive file given by
// the first argument.  If the second argument is a string giving the
// name of a file, output will go to that file.  If the second
// argument is a handle returned from the Open command or similar
// (opened for writing), output goes to the handle stream.  In either
// case, the return value is a null string.  If the second argument is
// a scalar 0, the output will be in the from of a string which is
// returned.
//
// The third argument is an integer or string which determines the
// type of information to return.  If an integer, the bits are flags
// that control the possible data fields and printing modes.  The
// string form is a space or comma-separated list of text tokens (from
// the list below, case insensitive) or hex integers.  The hex numbers
// or equivalent values for the text tokens are ored together to form
// the flags integer.
//
// This is really just a convenience wrapper around the ChdInfo
// function.  See the description of that function for a description
// of the flags.  In this function, the following keyword flags will
// show as follows:
//
// scale
//   This will always be 1.0.
//
// alias
//   No aliasing is applied.
//
// flags
//   The flags will always be 0.
//
// On error, a null string is returned, with an error message likely
// available from GetError.
//
bool
cvrt_funcs::IFfileInfo(Variable *res, Variable *args, void*)
{
    const char *fname;
    ARG_CHK(arg_string(args, 0, &fname))
    int fid = -1;
    const char *oname = 0;
    if (args[1].type == TYP_HANDLE) {
        ARG_CHK(arg_handle(args, 1, &fid))
    }
    else {
        ARG_CHK(arg_string(args, 1, &oname))
    }
    int flags;
    if (args[2].type == TYP_SCALAR) {
        ARG_CHK(arg_int(args, 2, &flags))
    }
    else {
        const char *string;
        ARG_CHK(arg_string(args, 2, &string))
        flags = cCHD::infoFlags(string);
    }

    if (!fname || !*fname)
        return (BAD);
    res->type = TYP_STRING;
    res->content.string = 0;
    char *realname;
    FILE *fp = FIO()->POpen(fname, "r", &realname);
    if (fp) {
        FileType ft = FIO()->GetFileType(fp);
        fclose(fp);
        if (FIO()->IsSupportedArchiveFormat(ft)) {
            cCHD *chd = FIO()->NewCHD(realname, ft, Physical, 0, cvINFOplpc);
            if (chd) {
                FILE *op = 0;
                if (fid >= 0) {
                    sHdl *fh = sHdl::get(fid);
                    if (fh) {
                        if (fh->type != HDLfd)
                            return (BAD);
                        op = fdopen(dup(fid), "w");
                    }
                }
                else if (oname)
                    op = fopen(oname, "w");
                res->content.string = chd->prInfo(op, Physical, flags);
                if (op)
                    fclose(op);
                if (res->content.string)
                    res->flags |= VF_ORIGINAL;
                delete chd;
            }
        }
        delete [] realname;
    }
    return (OK);
}


// (chd_name) OpenCellHierDigest(filename, info_saved)
//
// This function returns an access name to a new Cell Hierarchy
// Digest (CHD), obtained from the archive file given as the argument.
// The new CHD will be listed in the Cell Hierarchy Digests panel, and
// the access name is used by other functions to access the CHD.
//
// See this table for the features that apply during a call to this
// function.  In particular, the names of cells saved in the CHD
// reflect any aliasing that was in force at the time the CHD was
// created.
//
// The file is opened from the library search path, if a full path is
// not provided.  The CHD in a data structure that provides
// information about the hierarchy in compact form, and does not use
// that main database.  The second argument is an integer that
// determines the level of statistical information about the hierarchy
// saved.  This info is available from the ChdInfo function and by
// other means.  The values can be:
//
//   0   no information is saved.
//   1   only total object counts are saved (default if out of range).
//   2   object totals are saved per layer.
//   3   object totals are saved per cell.
//   4   objects counts are saved per cell and per layer.
//
// The larger the value, the more memory is required, so it is best
// to only save information that will be used.
//
// If the ChdEstFlatMemoryUse function will be called from the new
// CHD, the per-cell totals must be specified (value 3 or 4) or the
// estimate will be wildly inacurate.
//
// The CHD refers to physical information only.  On error, a null
// string is returned, and an error message may be available with the
// GetError function.
//
bool
cvrt_funcs::IFopenCellHierDigest(Variable *res, Variable *args, void*)
{
    const char *fname;
    ARG_CHK(arg_string(args, 0, &fname))
    int infoflags;
    ARG_CHK(arg_int(args, 1, &infoflags))

    if (!fname || !*fname)
        return (BAD);
    res->type = TYP_STRING;
    res->content.string = 0;

    char *realname;
    FILE *fp = FIO()->POpen(fname, "r", &realname);
    if (fp) {
        FileType ft = FIO()->GetFileType(fp);
        fclose(fp);
        if (FIO()->IsSupportedArchiveFormat(ft)) {
            cvINFO cvinfo;
            switch (infoflags) {
            case cvINFOnone:
                cvinfo = cvINFOnone;
                break;
            case cvINFOtotals:
                cvinfo = cvINFOtotals;
                break;
            case cvINFOpl:
                cvinfo = cvINFOpl;
                break;
            case cvINFOpc:
                cvinfo = cvINFOpc;
                break;
            case cvINFOplpc:
                cvinfo = cvINFOplpc;
                break;
            default:
                cvinfo = cvINFOtotals;
                break;
            }
            unsigned int alias_mask = (CVAL_CASE | CVAL_PFSF | CVAL_FILE);
            FIOaliasTab *tab = FIO()->NewReadingAlias(alias_mask);
            if (tab)
                tab->read_alias(realname);
            cCHD *chd = FIO()->NewCHD(realname, ft, Electrical, tab, cvinfo);
            delete tab;
            if (chd) {
                char *dbname = CDchd()->newChdName();
                CDchd()->chdStore(dbname, chd);
                res->content.string = dbname;
                res->flags |= VF_ORIGINAL;
            }
        }
        delete [] realname;
    }
    else
        Errs()->add_error("Error: can't open %s.", fname);
    return (OK);
}


// (int) WriteCellHierDigest(chd_name, filename, incl_geom, no_compr)
//
// This function will write a disk file representation of the Cell
// Hierarchy Digest (CHD) associated with the access name given as the
// first argument, into the file whose name is given as the second
// argument.  Subsequently, the file can be read with
// ReadCellHierDigest to recreate the CHD.  The file has no other
// use and the format is not documented.
//
// The CHD (and thus the file) contains offsets onto the target
// archive, as well as the archive location.  There is no checksum or
// other protection currently, so it is up to the user to make sure
// that the target archive is not moved or modified while the CHD
// is potentially or actually in use.
//
// If the boolean argument incl_geom is true, and the CHD has a
// linked CGD (as from ChdLinkCgd), then geometry records will be
// written to the file as well.  When the file is read, a new CGD
// will be created and linked to the new CHD.  Presently, the linked
// CGD must have memory or file type, as described for
// OpenCellGeomDigest.
//
// The boolean argument no_compr, if true, will skip use of
// compression of the CHD records.  This is unnecessary and not
// recommended, unless compatibility with Xic releases earlier than
// 3.2.17, which did not support compression, is needed.
//
// The function returns 1 if the file was written successfully, 0
// otherwise, with an error message likely available from GetError.
//
bool
cvrt_funcs::IFwriteCellHierDigest(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *fname;
    ARG_CHK(arg_string(args, 1, &fname))
    bool incl_geom;
    ARG_CHK(arg_boolean(args, 2, &incl_geom))
    bool no_compr;
    ARG_CHK(arg_boolean(args, 3, &no_compr))

    if (!chdname || !*chdname)
        return (BAD);
    if (!fname || !*fname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;

    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        unsigned int f = 0;
        if (incl_geom)
            f |= CHD_WITH_GEOM;
        if (no_compr)
            f |= CHD_NO_GZIP;
        sCHDout chdout(chd);
        res->content.value = chdout.write(fname, f);
    }
    else
        Errs()->add_error("Unresolved CHD access name");

    return (OK);
}


// (chd_name) ReadCellHierDigest(filename, cgd_type)
//
// This function returns an access name to a new Cell Hierarchy Digest
// (CHD) created from the file whose name is passed as an argument.
// The file must have been created with WriteCellHierDigest, or with
// the Save button in the Cell Hierarchy Digests panel.
//
// If the file was written with geometry records included, a new Cell
// Geometry Digest (CGD) may also be created (with an internally
// generated access name), and linked to the new CHD.  If the integer
// argument cgd_type is 0, a "memory" CGD will be created, which has
// the compresed geometry data stored in memory.  If cgd_type is 1, a
// "file" CGD will be created, which will use offsets to obtain
// geometry from the CHD file when needed.  If cgd_type is any other
// value, or the file does not contain geometry records, no CGD will
// be produced.
//
// On error, a null string is returned, with an error message
// probably available from GetError.
//
bool
cvrt_funcs::IFreadCellHierDigest(Variable *res, Variable *args, void*)
{
    const char *fname;
    ARG_CHK(arg_string(args, 0, &fname))
    bool cgd_type;
    ARG_CHK(arg_boolean(args, 1, &cgd_type))

    if (!fname || !*fname)
        return (BAD);

    ChdCgdType tp = CHD_CGDnone;
    if (cgd_type == 0)
        tp = CHD_CGDmemory;
    else if (cgd_type == 1)
        tp = CHD_CGDfile;

    res->type = TYP_STRING;
    res->content.string = 0;
    sCHDin chdin;
    cCHD *chd = chdin.read(fname, tp);
    if (chd) {
        char *dbname = CDchd()->newChdName();
        CDchd()->chdStore(dbname, chd);
        res->content.string = dbname;
        res->flags |= VF_ORIGINAL;
    }
    return (OK);
}


// (stringlist_handle) ChdList()
//
// This function returns a handle to a list of access strings to Cell
// Hierarchy Digests that are currently in memory.  The function never
// fails, though the handle may reference an empty list.
//
bool
cvrt_funcs::IFchdList(Variable *res, Variable*, void*)
{
    stringlist *s0 = CDchd()->chdList();
    sHdl *hdltmp = new sHdlString(s0);
    res->type = TYP_HANDLE;
    res->content.value = hdltmp->id;
    return (OK);
}


// (int) ChdChangeName(old_chd_name, new_chd_name)
//
// This function allows the user to change the access name of an
// existing Cell Hierarchy Digest (CHD) to a user-supplied name.  The
// new name must not already be in use by another CHD.
//
// The first argument is the access name of an existing CHD, the
// second argument is the new access name, with which the CHD will
// subsequently be accessed.  This name can be any text string, but
// can not be null.
//
// The function returns 1 on success, 0 otherwise, with an error
// message likely available from GetError.
//
bool
cvrt_funcs::IFchdChangeName(Variable *res, Variable *args, void*)
{
    const char *oldname;
    ARG_CHK(arg_string(args, 0, &oldname))
    const char *newname;
    ARG_CHK(arg_string(args, 1, &newname))

    if (!oldname || !*oldname)
        return (BAD);
    if (!newname || !*newname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(oldname, true);
    if (chd) {
        int ok = CDchd()->chdStore(newname, chd);
        if (!ok)
            // put it back
            CDchd()->chdStore(oldname, chd);
        res->content.value = ok;
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (int) ChdIsValid(cgd_name)
//
// This function returns one if the string argument is an access name
// of a Cell Hierarchy Digest currently in memory, zero otherwise.
//
bool
cvrt_funcs::IFchdIsValid(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd)
        res->content.value = 1;
    return (OK);
}


// (int) ChdDestroy(chd_name)
//
// If the string argument is an access name of a Cell Hierarchy Digest
// (CHD) currently in memory, the CHD will be destroyed and its memory
// freed.  One is returned on success, zero otherwise, with an error
// message likely available with GetError.
//
bool
cvrt_funcs::IFchdDestroy(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, true);
    if (chd) {
        delete chd;
        res->content.value = 1;
    }
    return (OK);
}


// (string) ChdInfo(chd_name, handle_or_filename, flags)
//
// This function provides information about the archive file
// represented by the Cell Hierarchy Digest (CHD) whose access name is
// given as the first argument.  If the second argument is a string
// giving the name of a file, output will go to that file.  If the
// second argument is a handle returned from the Open command or
// similar (opened for writing), output goes to the handle stream.  In
// either case, the return value is a null string.  If the second
// argument is a scalar 0, the output will be in the from of a string
// which is returned.
//
// The third argument is an integer or string which determines the
// type of information to return.  If an integer, the bits are flags
// that control the possible data fields and printing modes.  The
// string form is a space or comma-separated list of text tokens (from
// the list below, case insensitive) or hex integers.  The hex numbers
// or equivalent values for the text tokens are or'ed together to form
// the flags integer.
//
// If this argument is 0, all flags except for allcells, instances,
// and flags are implied.  Thus, the sometimes very lengthly
// cells/instances listing is skipped by default.  To obtain all
// available information, pass -1 or all as the flags value.
//
//   Keyword      Value    Description
//   filename     0x1      File name.
//   filetype     0x2      File type ("CIF", "CGX", "GDSII", or "OASIS").
//   scale        0x4      Applied scale factor.
//   alias        0x8      Applied cell name aliasing modes.
//   reccounts    0x10     Table of record type counts (file format
//                          dependent).
//   objcounts    0x20     Table of object counts.
//   depthcnts    0x40     Tabulate the number of cell
//                          instances at each hierarchy level.
//   estsize      0x80     Estimated memory needed to load file.
//   estchdsize   0x100    Estimated memory used by context struct.
//   layers       0x200    List of layer names found, as for ChdLayers
//                          function.
//   unresolved   0x400    List any cells that are referenced
//                          but not defined in the file.
//   topcells     0x800    List top-level cells.
//   allcells     0x1000   List all cells.
//   offsort      0x2000   Sort cells by offset.
//   offset       0x4000   Print offset.
//   instances    0x8000   Print instances with cells.
//   bbs          0x10000  Print bounding boxes with cells, and
//                          attributes with instances.
//   flags        0x20000  Unused.
//   all          -1       Set all flags.
//
// The information provided by these flags is more fully described
// below.
//
// filename
//   Print the name of the archive file for which the information
//   applies.
//
// filetype
//   Print a string giving the format of the archive file:  one of
//   "CIF", "CGX", "GDSII", or "OASIS".
//
// scale
//   Print the scale factor that was in effect when the CHD was created.
//
// alias
//   Print a string giving the cell name aliasing modes that were in
//   effect when the CHD was created.
//
// reccounts
//   Print a table of the counts for record types found in the archive.
//   This is format-dependent.
//
// objcounts
//   Print a table of object counts found in the archive file.  The
//   table contains the following keywords, each followed by a number.
//     Keyword         Descripption
//     Records         Total record count
//     Symbols         Number of cell definitions
//     Boxes           Number of rectangles
//     Polygons        Number of polygons
//     Wires           Number of wire paths
//     Avg Verts       Average vertex count per poly or wire
//     Labels          Number of (non-physical) labels
//     Srefs           Number of non-arrayed instances
//     Arefs           Number of arrayed instances
//
// depthcnts
//   A table of the number of cell instantiations at each hierarchy
//   level is printed, for each top-level cell found in the file.  The
//   count for depth 0 is 1 (the top-level cell), the count at depth 1
//   is the number of subcells of the top-level cell, depth 2 is the
//   number of subcells of these subcells, etc.  Arrays are given one
//   count, the same as a single instance.
//
// estsize
//   This flag will enable printing of the estimated memory required to
//   read the entire file into <i>Xic</i>.  The system must be able to
//   provide at least this much memory for a read to succeed.
//
// estchdsize
//   Print an estimate of the memory required by the present CHD.
//
//   By default, a compression mechanism is used to reduce the data
//   storage needed for instance lists.  The NoCompressContext variable,
//   if set, will turn off use of compression.  If compression is used,
//   extchdsize field will include compression statistics.  The "ratio"
//   is the space actually used to the space used if not compressed.
//
// layers
//   Print a list of the layer names encountered in the archive, as for
//   the <a href="ChdLayers"><tt>ChdLayers</tt></a> function.
//
// unresolved
//   This will list cells that are referenced but not defined in the
//   file.  These will also be listed if allcells is given.
//   A valid archive file will not contain unresolved references.
//
// topcells
//   List the top-level cells, i.e., the cells in the file that are not
//   used as a subcell by another cell in the file.  If allcells is also
//   given, only the names are listed, otherwise the cells are listed
//   including the offset, instances, bbs, and flags fields if these
//   flags are set.  The list will be sorted as per offsort.
//
// allcells
//   All cells found in the file are listed by name, including the
//   offset, instances, bbs, and flags fields if these flags are also
//   given.  The list will be sorted as per offsort.
//
// The following flags apply only if at least one of topcells or
// allcells is given.
//
// offsort
//   If this flag is set, the cells will be listed in ascending order of
//   the file offset, i.e., in the order in which the cell definitions
//   appear in the archive file.  If not set, cells are listed
//   alphabetically.
//
// offset
//   When set, the cell name is followed by the offset of the cell
//   definition record in the archive file.  This is given as a decimal
//   number enclosed in square brackets.
//
// instances
//   For each cell, the subcells used in the cell are listed.  The
//   subcell names are indented and listed below the cell name.
//
// bbs
//   For each cell the bounding box is shown, in L,B R,T form.  For
//   subcells, the position, transformation, and array parameters are
//   shown.  Coordinates are given in microns.  The subcell
//   transformation and array parameters are represented by a
//   concatenation of the following tokens, which follow the subcell
//   reference position.  These are similar to the transformation tokens
//   found in CIF, and have the same meanings.
//
//     MY           Mirror about the x-axis.
//     Ri,j         Rotate by an angle given by the vector i,j.
//     Mmag         Magnify by mag.
//     nx,ny,dx,dy  Specifies an array, nx x ny with spacings dx, dy.
//
// flags
//   This is currently unused and ignored.
//
//
// all
//   This enables all flags.
//
// On error, a null string is returned, with an error message likely
// available from GetError.
//
// This function is similar to the !fileinfo command and to the FileInfo
// script function.
//
bool
cvrt_funcs::IFchdInfo(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    int fid = -1;
    const char *fname = 0;
    if (args[1].type == TYP_HANDLE) {
        ARG_CHK(arg_handle(args, 1, &fid))
    }
    else {
        ARG_CHK(arg_string(args, 1, &fname))
    }
    int flags;
    if (args[2].type == TYP_SCALAR) {
        ARG_CHK(arg_int(args, 2, &flags))
    }
    else {
        const char *string;
        ARG_CHK(arg_string(args, 2, &string))
        flags = cCHD::infoFlags(string);
    }

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_STRING;
    res->content.string = 0;

    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        FILE *fp = 0;
        if (fid >= 0) {
            sHdl *fh = sHdl::get(fid);
            if (fh) {
                if (fh->type != HDLfd)
                    return (BAD);
                fp = fdopen(dup(fid), "w");
            }
        }
        else if (fname)
            fp = fopen(fname, "w");
        res->content.string = chd->prInfo(fp, Physical, flags);
        if (fp)
            fclose(fp);
        if (res->content.string)
            res->flags |= VF_ORIGINAL;
    }
    else
        Errs()->add_error("Unresolved CHD access name");

    return (OK);
}


// (string) ChdFileName(chd_name)
//
// This function returns a string containing the full pathname of the
// file associated with the Cell Hierarchy Digest (CHD) whose access
// name was given in the argument.  A null string is returned on
// error, with an error message likely available from GetError.
//
bool
cvrt_funcs::IFchdFileName(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_STRING;
    res->content.string = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        res->content.string = chd->prFilename(0);
        res->flags |= VF_ORIGINAL;
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (string) ChdFileType(chd_name)
//
// This function returns a string containing the file format of the
// file associated with the Cell Hierarchy Digest (CHD) whose access
// name was given in the argument.  A null string is returned on
// error, with an error message likely available from GetError.  Other
// possible returns are "CIF", "GDSII", "CGX", and "OASIS".
//
bool
cvrt_funcs::IFchdFileType(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_STRING;
    res->content.string = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        res->content.string = chd->prFiletype(0);
        res->flags |= VF_ORIGINAL;
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (stringlist_handle) ChdTopCells(chd_name)
//
// This function returns a handle to a list of strings that contain
// the top-level cell names in the Cell Hierarchy Digest (CHD) whose
// access name was given in the argument (physical cells only).  The
// top-level cells are those not used as a subcell by another cell in
// the CHD.  A scalar zero is returned on error, with an error message
// likely available from GetError.
//
bool
cvrt_funcs::IFchdTopCells(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        syrlist_t *s0 = chd->topCells(Physical, false);
        if (s0) {
            // Make a stringlist.
            stringlist *sl0 = 0, *se = 0;
            for (syrlist_t *s = s0; s; s = s->next) {
                if (!sl0)
                    sl0 = se = new stringlist(
                        lstring::copy(Tstring(s->symref->get_name())), 0);
                else {
                    se->next = new stringlist(
                        lstring::copy(Tstring(s->symref->get_name())), 0);
                    se = se->next;
                }
            }
            syrlist_t::destroy(s0);

            sHdl *hdltmp = new sHdlString(sl0);
            res->type = TYP_HANDLE;
            res->content.value = hdltmp->id;
        }
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (stringlist_handle) ChdListCells(chd_name, cellname, mode, all)
//
// This function returns a handle to a list of cellnames from among
// those found in the CHD, whose access name is given as the first
// argument.  There are two basic modes, depending on whether the
// boolean argument all is true or not.
//
// If all is true, the cellname argument is ignored, and the list will
// consist of all cells found in the CHD.  If the integer mode
// argument is 0, all physical cell names are listed.  If mode is 1,
// all electrical cell names will be returned.  If any other value,
// the listing will contain all physical and electrical cell names,
// with no duplicates.
//
// If all is false, the listing will contain the names of all cells
// under the hierarchy of the cell named in the cellname argument
// (including cellname).  If cellname is 0, empty, or null, the
// default cell for the CHD is assumed, i.e., the cell which has been
// configured, or the first top-level cell found.  The mode argument
// is 0 for physical cells, nonzero for electrical cells (there is no
// merging of lists in this case).
//
// On error, a scalar 0 is returned, and a message may be available
// from GetError.
//
bool
cvrt_funcs::IFchdListCells(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))
    int dmode;
    ARG_CHK(arg_int(args, 2, &dmode))
    bool all;
    ARG_CHK(arg_boolean(args, 3, &all))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        stringlist *s0;
        if (all)
            s0 = chd->listCellnames(dmode, false);
        else
            s0 = chd->listCellnames(cname,
                dmode == 0 ? Physical : Electrical);
        if (s0) {
            sHdl *hdltmp = new sHdlString(s0);
            res->type = TYP_HANDLE;
            res->content.value = hdltmp->id;
        }
    }
    return (OK);
}


// (stringlist_handle) ChdLayers(chd_name)
//
// This function returns a handle to a list of strings that contain
// the names of layers used in the file represented by the Cell
// Hierarchy Digest whose access name is passed as the argument
// (physical cells only).  For file formats that use a layer/datatype,
// the names are four-byte hex integers, where the left two bytes are
// the zero-padded hex value of the layer number, and the right two
// bytes are the zero-padded value of the datatype number.  This
// applies for GDSII/OASIS files that follow the standard convention
// that layer and datatype numbers are 0-255.  If either number is
// larger than 255, the layer "name" will consist of eight hex bytes,
// the left four for layer number, the right four for datatype.
//
// The layers listing is available only if the CHD was created with
// info available, i.e., OpenCellHierDigest was called with the
// info_saved argument set to a value other than 0.
//
// Each unique combination or layer name is listed.  A scalar zero is
// returned on error, in which case an error message may be available
// from GetError.
//
bool
cvrt_funcs::IFchdLayers(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        stringlist *s0 = chd->layers(Physical);
        if (s0) {
            stringlist::sort(s0);
            sHdl *hdltmp = new sHdlString(s0);
            res->type = TYP_HANDLE;
            res->content.value = hdltmp->id;
        }
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (int) ChdInfoMode(chd_name)
//
// This function returns the saved info mode of the Cell Hierarchy
// Digest whose access name is passed as the argument.  This is the
// info_saved value passed to OpenCellHierDigest.  The values are:
//
//   0   no information is saved.
//   1   only total object counts are saved.
//   2   object totals are saved per layer.
//   3   object totals are saved per cell.
//   4   objects counts are saved per cell and per layer.
//
// If the CHD name is not resolved, the return value is -1, with an
// error message available from GetError.
//
bool
cvrt_funcs::IFchdInfoMode(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd)
        res->content.value = cv_info::savemode(chd->pcInfo(Physical));
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (stringlist_handle) ChdInfoLayers(chd_name, cellname)
//
// This is identical to the ChdLayers function when the cellname is 0,
// null, or empty.  If the CHD was created with OpenCellHierDigest
// with the info_saved argument set to 4 (per-cell and per-layer info
// saved), then a cellname string can be passed.  In this case, the
// return is a handle to a list of layers used in the named cell.  A
// scalar 0 is returned on error, with an error message probably
// available from GetError.
//
bool
cvrt_funcs::IFchdInfoLayers(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *cellname;
    ARG_CHK(arg_string(args, 1, &cellname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        cv_info *info = chd->pcInfo(Physical);
        if (info) {
            stringlist *s0 = info->layers(cellname);
            if (s0) {
                stringlist::sort(s0);
                sHdl *hdltmp = new sHdlString(s0);
                res->type = TYP_HANDLE;
                res->content.value = hdltmp->id;
            }
            else
                Errs()->add_error("Failed to find any layers.");
        }
        else
            Errs()->add_error("No info saved in CHD.");
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (stringlist_handle) ChdInfoCells(chd_name)
//
// If the CHD whose access name is given as the argument was created
// with OpenCellHierDigest with the info_saved argument set to 3
// (per-cell data saved) or 4 (per-cell and per-layer data saved),
// then this function will return a handle to a list of cell names
// from the source file.  On error, a scalar 0 is returned, with an
// error message probably available from GetError.
//
bool
cvrt_funcs::IFchdInfoCells(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        cv_info *info = chd->pcInfo(Physical);
        if (info) {
            stringlist *s0 = info->cells();
            if (s0) {
                stringlist::sort(s0);
                sHdl *hdltmp = new sHdlString(s0);
                res->type = TYP_HANDLE;
                res->content.value = hdltmp->id;
            }
            else
                Errs()->add_error("Failed to find any cells.");
        }
        else
            Errs()->add_error("No info saved in CHD.");
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (int) ChdInfoCounts(chd_name, cellname, layername, array)
//
// This function will return object count statistics in the array,
// which must have size 4 or larger.  The counts are obtained when the
// CHD, whose access name is given as the first argument, was created. 
// The types of counts available depend on the info_saved value passed
// to OpenCellHierDigest when the CHD was created.
//
// The array is filled in as follows:
//   array[0]    Box count.
//   array[1]    Polygon count.
//   array[2]    Wire count.
//   array[3]    Vertex count (polygons plus wires).
//
// The following counts are available for the various info_saved modes.
//
// info_saved = 0
//   No information is available.
//
// info_saved = 1
//   Both arguments are ignored, the return provides file totals.
//
// info_saved = 2
//   The cellname argument is ignored.  If layername is 0, null, or
//   empty, the return provides file totals.  Otherwise, the return
//   provides totals for layername, if found.
//
// info_saved = 3
//   The layername argument is ignored.  If cellname is 0, null, or
//   empty, the return represents file totals.  Otherwise, the return
//   provides totals for cellname, if found.
//
// info_saved = 4
//   If both arguments are 0, null, or empty, the return represents file
//   totals.  If cellname is 0, null, or empty, the return represents
//   totals for the layer given.  If layername is 0, null, or empty, the
//   return provides totals for the cell name given.  If both names are
//   given, the return provides totals for the given layer in the given
//   cell.
//
// If a cell or layer is not found, or data are not available for some
// reason, or an error occurs, the return value is 0, and an error
// message may be available from GetError.  Otherwise, the return
// value is 1, and the array is filled in.
//
bool
cvrt_funcs::IFchdInfoCounts(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *cellname;
    ARG_CHK(arg_string(args, 1, &cellname))
    const char *layername;
    ARG_CHK(arg_string(args, 2, &layername))
    double *vals;
    ARG_CHK(arg_array(args, 3, &vals, 4))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        cv_info *info = chd->pcInfo(Physical);
        if (info) {
            pl_data *pld = info->info(cellname, layername);
            if (pld) {
                vals[0] = pld->box_count();
                vals[1] = pld->poly_count();
                vals[2] = pld->wire_count();
                vals[3] = pld->vertex_count();
                delete pld;
                res->content.value = 1;
            }
            else
                Errs()->add_error("Failed to find any cells.");
        }
        else
            Errs()->add_error("No info saved in CHD.");
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (int) ChdCellBB(chd_name, cellname, array)
//
// This returns the bounding box of the named cell.  The cellname is a
// string giving the name of a physical cell found in the Cell
// Hierarchy Digest whose access name is given in the first argument.
//
// The cellname, if nonzero, must be the cell name after any aliasing
// that was in force when the CHD was created.  If cellname is passed
// 0, the default cell for the CHD is understood.  This is a cell name
// configured into the CHD, or the first top-level cell found in the
// archive file.
//
// The values are returned in the array, which must have size 4 or
// larger.  the order is l,b,r,t.  One is returned on success, zero
// otherwise, with an error message likely available from GetError.
//
// The cell bounding boxes for geometry are computed as the file is
// read, so that if the NoReadLabels variable is set during the read,
// i.e., when OpenCellHierDigest is called, text labels will not
// contribute to the bounding box computation.
//
bool
cvrt_funcs::IFchdCellBB(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *name;
    ARG_CHK(arg_string(args, 1, &name))
    double *vals;
    ARG_CHK(arg_array(args, 2, &vals, 4))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        const char *cname = name;
        if (!cname || !*cname) {
            cname = chd->defaultCell(Physical);
            if (!cname)
                return (BAD);
        }
        symref_t *p = chd->findSymref(cname, Physical);
        if (p && chd->setBoundaries(p)) {
            const BBox *bbp = p->get_bb();
            if (bbp) {
                vals[0] = MICRONS(bbp->left);
                vals[1] = MICRONS(bbp->bottom);
                vals[2] = MICRONS(bbp->right);
                vals[3] = MICRONS(bbp->top);
                res->content.value = 1.0;
            }
        }
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (int) ChdSetDefCellName(chd_name, cellname)
//
// This will set or unset the configuration of a default cell name in
// the Cell Hierarchy Digest whose access name is given in the first
// argument.
//
// If the cellname argument in not 0 or null, it must be a cell name
// after any aliasing that was in force when the CHD was created, that
// exists in the CHD.  This will set the default cell name for the CHD
// which will be used subsequently by the CHD whenever a cell name is
// not otherwise specified.  The current default cell name is returned
// from the ChdDefCellName function.  If cellname is 0 or null, the
// default cellname is unconfigured.  In this case, the CHD will use
// the first top-level cell found (lowest offset on the archive file).
// A top-level cell is one that is not used as a subcell by any other
// cell in the CHD.
//
// One is returned on success, zero otherwise, with an error message
// likely available with GetError.
//
bool
cvrt_funcs::IFchdSetDefCellName(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *cellname;
    ARG_CHK(arg_string(args, 1, &cellname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd)
        res->content.value = chd->setDefaultCellname(cellname, 0);
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (string) ChdDefCellName(chd_name)
//
// This will return the default cell name of the Cell Hierarchy Digest
// whose access name is given in the argument.  This will be the cell
// name configured (with ChdSetDefCellName), or if no cell name is
// configured the return will be the name of the first top-level cell
// found (lowest offset on the archive file).  A top-level cell is one
// that is not used as a subcell by any other cell in the CHD.
//
// On error, a null string is returned, with an error message likely
// available from GetError.
//
bool
cvrt_funcs::IFchdDefCellName(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_STRING;
    res->content.string = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        res->content.string = lstring::copy(chd->defaultCell(Physical));
        res->flags |= VF_ORIGINAL;
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// ChdLoadGeometry(chd_name)
//
// This function will read the geometry from the original layout
// file from the Cell Hierarchy Digest (CHD) whose access name is
// given in the argument into a new Cell Geometry Digest (CGD) in
// memory, and configures the CHD to link to the new CGD for use
// when reading.  The new CGD is given an internally-generated
// access name, and will store all geometry data in memory.  The new
// CGD will be destroyed when unlinked.
//
// This is a convenience function, one can explicitly create a CGD
// (with OpenCellGeomDigest) and link it to the CHD (with
// ChdLinkCgd) if extended features are needed.
//
// See this table for the features that apply during a call to this
// function.
//
// The return value is 1 on success, 0 otherwise, with an error
// message likely available from GetError.
//
bool
cvrt_funcs::IFchdLoadGeometry(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        cCGD *cgd = FIO()->NewCGD(0, 0, CGDmemory, chd);
        if (!cgd)
            return (OK);
        // Link the new CHD, and set the flag to delete the CGD when
        // unlinked.
        cgd->set_free_on_unlink(true);
        chd->setCgd(cgd);
        res->content.value = 1.0;
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (int) ChdLinkCgd(chdname, cgdname)
//
// This function links or unlinks a Cell Geometry Digest (CGD) whose
// access name is given as the second argument, to the Cell Hierarchy
// Digest (CHD) whose access name is given as the first argument. 
// With a CGD linked, when the CHD is used to access geometry data,
// the data will be obtained from the CGD, if it exists in the CGD,
// and from the original layout file if not provided by the CGD.  The
// CGD is a "geometry cache" which resides in memory.
//
// If the cgdname is null or empty (0 can be passed for this
// argument) any CGD linked to the CHD will be unlinked.  If the CGD
// was created specifically to link with the CHD, such as with with
// LoadGeometry, it will be freed from memory, otherwise it will be
// retained.
//
// This function returns 1 on success, 0 otherwise with an error
// message likely available from GetError.
//
bool
cvrt_funcs::IFchdLinkCgd(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *cgdname;
    ARG_CHK(arg_string(args, 1, &cgdname))

    if (!chdname || !*chdname)
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        if (!cgdname || !*cgdname) {
            chd->setCgd(0);
            res->content.value = 1;
        }
        else
            res->content.value = chd->setCgd(0, cgdname);
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (string) ChdGetGeomName(chd_name)
//
// The string argument is an access name for a Cell Hierarchy Digest
// (CHD) in memory.  If the CHD exists and has an associated Cell
// Geometry Digest (CGD) linked (e.g., ChdLoadGeometry was called),
// this function returns the access name of the CGD.  If the CHD is
// not found or not configured with a CGD, a null string is returned.
//
bool
cvrt_funcs::IFchdGetGeomName(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_STRING;
    res->content.string = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd)
        res->content.string = lstring::copy(chd->getCgdName());
    return (OK);
}


// (int) ChdClearGeometry(chd_name)
//
// This function will clear the link to the Cell Geometry Digest
// within the Cell Hierarchy Digest.  If a CGD was linked, and it was
// created explicitly for linking into the CHD as in LoadGeometry, the
// CGD will be freed, otherwise it will be retained.  The return value
// is 1 if the CHD was found, 0 otherwise, with a message available
// from GetError.
//
// This function is identical to ChdLinkCgd with a null second
// argument.
//
bool
cvrt_funcs::IFchdClearGeometry(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        res->content.value = chd->hasCgd();
        chd->setCgd(0);
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (int) ChdSetSkipFlag(chd_name, cellname, skip)
//
// This will set/unset the skip flag in the Cell Hierarchy Digest
// (CHD) whose access name is given in the first argument for the cell
// named in cellname (physical only).
//
// The cellname, if nonzero, must be the cell name after any aliasing
// that was in force when the CHD was created.  If cellname is passed
// 0, the default cell for the CHD is understood.  This is a cell name
// configured into the CHD, or the first top-level cell found in the
// archive file.
//
// With the skip flag set, the cell is ignored in the CHD, i.e., the
// cell and its instances will not be included in output or when
// reading into memory when the CHD is used to acess layout data.  The
// last argument is a boolean value:  0 to unset the skip flag,
// nonzero to set it.  The return value is 1 if a flag was altered, 0
// otherwise, with an error message likely available from GetError.
//
bool
cvrt_funcs::IFchdSetSkipFlag(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *name;
    ARG_CHK(arg_string(args, 1, &name))
    bool skip;
    ARG_CHK(arg_boolean(args, 2, &skip))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        if (chd && chd->nameTab(Physical)) {
            const char *cname = name;
            if (!cname || !*cname) {
                cname = chd->defaultCell(Physical);
                if (!cname)
                    return (BAD);
            }
            symref_t *p = chd->findSymref(cname, Physical);
            if (p) {
                p->set_skip(skip);
                res->content.value = 1;
            }
        }
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (int) ChdClearSkipFlags(chd_name)
//
// This will clear the skip flags for all cells in the Cell Hierarchy
// Digest whose access name is given in the argument.  The skip flags
// are set with SetSkipFlag.  The return value is 1 on success, 0
// otherwise, with an error message likely available with GetError.
//
bool
cvrt_funcs::IFchdClearSkipFlags(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        chd->clearSkipFlags();
        res->content.value = 1;
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (int) ChdCompare(chd_name1, cname1, chd_name2, cname2, layer_list,
//  skip_layers, maxdiffs, obj_types, geometric, array)
//
// This will compare the contents of two cells, somewhat similar to
// the !compare command and the Compare Layouts operation in the
// Convert Menu.  However, only one cell pair is compared, taking
// account only of features within the cells.  The ChdCompareFlat
// function is similar, but flattens geometry before comparison.
//
// When comparing subcells, arrays will be expanded into individual
// instances before comparison, avoiding false differences between
// arrayed and unarrayed equivalents.  The returned handles (if any)
// contain the differences, as lists of objects.  Properties are ignored.
//
//   chd_name1      Access name of a Cell Hierarchy Digest (CHD) in memory.
//   cname1         Name of cell in chd_name1 to compare, if null (0 passed)
//                  the default cell in chd_name1 is used.
//   chd_name2      If not null or empty (one can pass 0 for this argument),
//                  the name of another CHD.
//                  If null, a cell cname2 should exist in memory.
//   cname2         Name of cell in the second CHD, or in memory, to compare.
//                  If null, or 0 is passed, and a second CHD was specified,
//                  the second CHD's default cell is understood.  Otherwise,
//                  the name will be assumed the same as cname1.
//   layer_list     String of space-separated layer names, or zero which
//                  implies all layers.
//   skip_layers    If this boolean value is nonzero and a layer_list
//                  was given, the layers in the list will be skipped.
//                  Otherwise, only the layers in the list will be compared
//                  (all layers if layer_list is passed zero).
//   maxdiffs       The function will return after recording this many
//                  differences.  If 0 or negative, there is no limit.
//   obj_types      String consisting of the characters c,b,p,w,l, which
//                  determines objects to consider (subcells, boxes, polygons,
//                  wires, and labels), or zero.  If zero, "cbpw" is the
//                  default, i.e., labels are ignored.  If the geometric
//                  argument is nonzero, all but 'c' will be ignored, and
//                  boxes, polygons, and wires will be compared.
//   geometric      If this boolean value is nonzero, a geometric comparison
//                  will be performed, otherwise objects are compared
//                  directly.
//   array          This is a two-element or larger array, or zero.  If
//                  an array is passed, upon return the elements are
//                  handles to lists of box, polygon, and wire object copies
//                  (labels and subcells are not returned): array[0] contains
//                  a list of objects in chd_name1 and not in chd_name2, and
//                  array[1] contains objects in chd_name2 and not in chd_name1.
//                  The H function must be used on the array elements to
//                  access the handles.  If the argument is passed zero, no
//                  object lists are returned.
//
// The cells for the current mode (electrical or physical) are compared.
// The scalar return can take the following values:
//   -1             An error occurred, with a message possibly available
//                  from the GetError function.
//    0             Successful comparison, no differences found.
//    1             Successful comparson, differences found.
//    2             The cell was not found in chd_name1.
//    3             The cell was not found in chd_name2 or memory.
//    4             The cell was not found in either source.
//
bool
cvrt_funcs::IFchdCompare(Variable *res, Variable *args, void*)
{
    const char *chdname1;
    ARG_CHK(arg_string(args, 0, &chdname1))
    const char *cname1;
    ARG_CHK(arg_string(args, 1, &cname1))
    const char *chdname2;
    ARG_CHK(arg_string(args, 2, &chdname2))
    const char *cname2;
    ARG_CHK(arg_string(args, 3, &cname2))
    const char *layer_list;
    ARG_CHK(arg_string(args, 4, &layer_list))
    bool skip_layers;
    ARG_CHK(arg_boolean(args, 5, &skip_layers))
    int maxdiffs;
    ARG_CHK(arg_int(args, 6, &maxdiffs))
    const char *obj_types;
    ARG_CHK(arg_string(args, 7, &obj_types))
    bool geometric;
    ARG_CHK(arg_boolean(args, 8, &geometric))
    double *array;
    ARG_CHK(arg_array_if(args, 9, &array, 2))

    if (!chdname1 || !*chdname1)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = -1;

    DisplayMode dmode = DSP()->CurMode();

    cCHD *chd1 = CDchd()->chdRecall(chdname1, false);
    if (!chd1) {
        Errs()->add_error("Unresolved CHD 1 access name");
        return (OK);
    }
    cCHD *chd2 = 0;
    if (chdname2 && *chdname2) {
        chd2 = CDchd()->chdRecall(chdname2, false);
        if (!chd2) {
            Errs()->add_error("Unresolved CHD 2 access name");
            return (OK);
        }
    }
    if (!cname1 || !*cname1) {
        cname1 = chd1->defaultCell(dmode);
        if (!cname1) {
            Errs()->add_error("No cells found in CHD-1.");
            return (OK);
        }
    }
    if (!cname2 || !*cname2) {
        if (chd2) {
            cname2 = chd2->defaultCell(dmode);
            if (!cname2) {
                Errs()->add_error("No cells found in CHD-2.");
                return (OK);
            }
        }
        else
            cname2 = cname1;
    }

    CHDdiff chd_diff(chd1, chd2);
    chd_diff.set_layers(layer_list, skip_layers);
    chd_diff.set_types(obj_types);
    chd_diff.set_max_diffs(maxdiffs);
    chd_diff.set_geometric(geometric);
    chd_diff.set_exp_arrays(true);

    Sdiff *sdiff = 0;
    DFtype df = chd_diff.diff(cname1, cname2, dmode, array ? &sdiff : 0);
    if (df == DFabort)
        SI()->SetInterrupt();
    else if (df == DFerror)
        return (BAD);
    else {
        res->content.value = df;
        if (sdiff) {
            CDol *ol12 = 0;
            CDol *ol21 = 0;
            Ldiff *l = sdiff->ldiffs();
            while (l) {
                Ldiff *ln = l->next_ldiff();
                for (stringlist *s = l->list12(); s; s = s->next) {
                    CDo *od = CDo::fromCifString(l->ldesc(), s->string);
                    if (od)
                        ol12 = new CDol(od, ol12);
                }
                for (stringlist *s = l->list21(); s; s = s->next) {
                    CDo *od = CDo::fromCifString(l->ldesc(), s->string);
                    if (od)
                        ol21 = new CDol(od, ol21);
                }
                delete l;
                l = ln;
            }
            sdiff->set_ldiffs(0);
            delete sdiff;

            sHdl *hdl = new sHdlObject(ol12, 0, true);
            array[0] = hdl->id;
            hdl = new sHdlObject(ol21, 0, true);
            array[1] = hdl->id;
        }
    }
    return (OK);
}


// (int) ChdCompareFlat(chd_name1, cname1, chd_name2, cname2, layer_list,
//  skip_layers, maxdiffs, area, coarse_mult, fine_grid, array)
//
// This will compare the contents of two hierarchies, using a flat
// geometry model similar to the flat options of the !compare command
// and the Compare Layouts operation in the Convert Menu.  The
// ChdCompare function is similar, but does not flatten.
//
// The returned handles (if any) contain the differences, as lists of
// objects.  Properties are ignored.
//
//   chd_name1      Access name of a Cell Hierarchy Digest (CHD) in memory.
//   cname1         Name of cell in chd1 to compare, if null (0 passed)
//                  the default cell in chd1 is used.
//   chd_name2      Access name of another CHD in memory.  This argument
//                  can not be null as in ChdCompare, flat comparison to
//                  memory cells is unavailable.
//   cname2         Name of cell in the second CHD to compare.
//                  If null, or 0 is passed, the second CHD's default cell
//                  is understood.
//   layer_list     String of space-separated layer names, or zero which
//                  implies all layers.
//   skip_layers    If this boolean value is nonzero and a layer_list
//                  was given, the layers in the list will be skipped.
//                  Otherwise, only the layers in the list will be compared
//                  (all layers if layer_list is passed zero).
//   maxdiffs       The function will return after recording this many
//                  differences.  If 0 or negative, there is no limit.
//   area           This argument can be an array of size 4 or larger, or
//                  0.  If an array, it contains a rectangle description
//                  in order L,B,R,T in microns, which specifies the area
//                  to compare.  If 0 is passed, the area compared will
//                  contain the two hierarchies entirely.
//   coarse_mult    The comparison is performed in the manner described
//                  for the ChdIterateOverRegion function, using a fine
//                  grid and a coarse grid.  This argument specifies the
//                  size of the coarse grid in multiples of the fine grid
//                  size.  All of the geometry needed for a coarse grid
//                  cell is brought into memory at once, so this size
//                  should be consistent with memory availability and layout
//                  feature density.  Values of 1-100 are accepted for this
//                  argument, with 20 a reasonable initial choice.
//  fine_grid       Comparison is made within a fine grid cell.  The
//                  optimum fine grid size depends on factors including
//                  layout feature density and memory availability.  Larger
//                  sizes usually run faster, but may require excessive
//                  memory.  The value is given in microns, with the
//                  acceptable range being 1.0 - 100.0 microns.  A reasonable
//                  initial choice is 20.0, but experimentation can often
//                  yield better performance.
//   array          This is a two-element or larger array, or zero.  If
//                  an array is passed, upon return the elements are
//                  handles to lists of box, polygon, and wire object copies
//                  (labels and subcells are not returned): array[0] contains
//                  a list of objects in chd_name1 and not in chd_name2, and
//                  array[1] contains objects in chd_name2 and not in chd_name1.
//                  The H function must be used on the array elements to
//                  access the handles.  If the argument is passed zero, no
//                  object lists are returned.
//
// The cells for the physical mode are compared, it is not possible to
// compare electrical cells in flat mode.  The return value is an
// integer, -1 on error (with a message likely available from
// GetError), 0 if no differences were seen, or positive giving the
// number of differences seen.
//
bool
cvrt_funcs::IFchdCompareFlat(Variable *res, Variable *args, void*)
{
    const char *chdname1;
    ARG_CHK(arg_string(args, 0, &chdname1))
    const char *cname1;
    ARG_CHK(arg_string(args, 1, &cname1))
    const char *chdname2;
    ARG_CHK(arg_string(args, 2, &chdname2))
    const char *cname2;
    ARG_CHK(arg_string(args, 3, &cname2))
    const char *layer_list;
    ARG_CHK(arg_string(args, 4, &layer_list))
    bool skip_layers;
    ARG_CHK(arg_boolean(args, 5, &skip_layers))
    int maxdiffs;
    ARG_CHK(arg_int(args, 6, &maxdiffs))
    double *area;
    ARG_CHK(arg_array_if(args, 7, &area, 4))
    int coarse_mult;
    ARG_CHK(arg_int(args, 8, &coarse_mult))
    int fine_grid;
    ARG_CHK(arg_coord(args, 9, &fine_grid, Physical))
    double *array;
    ARG_CHK(arg_array_if(args, 10, &array, 2))

    if (!chdname1 || !*chdname1)
        return (BAD);
    if (!chdname2 || !*chdname2)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = -1;

    cCHD *chd1 = CDchd()->chdRecall(chdname1, false);
    if (!chd1) {
        Errs()->add_error("Unresolved CHD 1 access name");
        return (OK);
    }
    cCHD *chd2 = CDchd()->chdRecall(chdname2, false);
    if (!chd2) {
        Errs()->add_error("Unresolved CHD 2 access name");
        return (OK);
    }
    if (!cname1 || !*cname1) {
        cname1 = chd1->defaultCell(Physical);
        if (!cname1) {
            Errs()->add_error("No cells found in CHD-1.");
            return (OK);
        }
    }
    if (!cname2 || !*cname2) {
        cname2 = chd2->defaultCell(Physical);
        if (!cname2) {
            Errs()->add_error("No cells found in CHD-2.");
            return (OK);
        }
    }
    if (maxdiffs < 0)
        maxdiffs = 0;

    BBox BB;
    if (area) {
        BB.left = INTERNAL_UNITS(area[0]);
        BB.bottom = INTERNAL_UNITS(area[1]);
        BB.right = INTERNAL_UNITS(area[2]);
        BB.top = INTERNAL_UNITS(area[3]);
        BB.fix();
    }

    Sdiff *sdiff = 0;
    unsigned int diffcnt = 0;
    XIrt ret = cCHD::compareCHDs_sd(chd1, cname1, chd2, cname2,
        area ? &BB : 0, layer_list, skip_layers, array ? &sdiff : 0,
        maxdiffs, &diffcnt, coarse_mult, fine_grid);

    if (ret == XIintr)
        SI()->SetInterrupt();
    else if (ret == XIbad)
        return (BAD);
    else {
        res->content.value = diffcnt;
        if (sdiff) {
            CDol *ol12 = 0;
            CDol *ol21 = 0;
            Ldiff *l = sdiff->ldiffs();
            while (l) {
                Ldiff *ln = l->next_ldiff();
                for (stringlist *s = l->list12(); s; s = s->next) {
                    CDo *od = CDo::fromCifString(l->ldesc(), s->string);
                    if (od)
                        ol12 = new CDol(od, ol12);
                }
                for (stringlist *s = l->list21(); s; s = s->next) {
                    CDo *od = CDo::fromCifString(l->ldesc(), s->string);
                    if (od)
                        ol21 = new CDol(od, ol21);
                }
                delete l;
                l = ln;
            }
            sdiff->set_ldiffs(0);
            delete sdiff;

            sHdl *hdl = new sHdlObject(ol12, 0, true);
            array[0] = hdl->id;
            hdl = new sHdlObject(ol21, 0, true);
            array[1] = hdl->id;
        }
    }
    return (OK);

}


// (int) ChdEdit(chd_name, scale, cellname)
//
// This will read the given cell and its descendents into memory and
// open the cell for editing, similar to the Edit function, however
// the layout data will be accessed through the Cell Hierarchy Digest
// whose access name is given in the first argument.  The return value
// takes the same values as the Edit function return.
//
// See this table for the features that apply during a call to this
// function.
//
// The scale will multiply the scale factor in effect when the CHD was
// created, e.g., as provided to GetCellHierDigest.
//
// The cellname, if nonzero, must be the cell name after any aliasing
// that was in force when the CHD was created.  If cellname is passed
// 0, the default cell for the CHD is understood.  This is a cell name
// configured into the CHD, or the first top-level cell found in the
// archive file.
//
bool
cvrt_funcs::IFchdEdit(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    double scale;
    ARG_CHK(arg_real(args, 1, &scale))
    const char *name;
    ARG_CHK(arg_string(args, 2, &name))

    if (!chdname || !*chdname)
        return (BAD);
    if (scale < .001 || scale > 1000.0)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        const char *cname = name;
        if (!cname || !*cname) {
            cname = chd->defaultCell(Physical);
            if (!cname)
                return (BAD);
        }
        EV()->InitCallback();

        // implicit "Commit"
        EditIf()->ulCommitChanges();

        FIOreadPrms prms;
        prms.set_scale(scale);
        prms.set_allow_layer_mapping(true);
        res->content.value = XM()->EditCell(0, false, &prms, cname, chd);
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// (int) ChdOpenFlat(chd_name, scale, cellname, array, clip)
//
// This will read the cell named in the cellname string and its
// subcells into memory, creating a flat cell with the same name.  The
// Cell Hierarchy Digest (CHD) whose access name is given in the first
// argument is used to obtain the layout data.
//
// See this table for the features that apply during a call to this
// function.  Text labels are ignored.
//
// The cellname, if nonzero, must be the cell name after any aliasing
// that was in force when the CHD was created.  If cellname is passed
// 0, the default cell for the CHD is understood.  This is a cell name
// configured into the CHD, or the first top-level cell found in the
// archive file.
//
// If the cell already exists in memory, it will be overwritten.
//
// The scale will multiply the scale factor in effect when the CHD was
// created, e.g., as provided to GetCellHierDigest.
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
// render the window will be read.  This window should be equal to
// or contained in the window used to configure the CHD, if any.
//
// If the boolean value clip is nonzero and an array is given, objects
// will be clipped to the window.  Otherwise no clipping is done.
//
// Before calling ChdOpenFlat, the memory use can be estimated by calling
// the ChdEstFlatMemoryUse function.  An overall transformation can be
// set with ChdSetFlatReadTransform.
//
// The return value is 1 on success, 0 on error, or -1 if an interrupt
// was received.  In the case of an error return, an error message may
// be available through GetError.
//
bool
cvrt_funcs::IFchdOpenFlat(Variable *res, Variable *args, void*)
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

    if (!chdname || !*chdname)
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
        res->content.value = chd->readFlat(cname, &prms);
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// ChdSetFlatReadTransform(tfstring, x, y)
//
// This rather arcane function will set up a transformation which will
// be used during calls to the following functions:
//
//    ChdOpenFlat
//    ChdWriteSplit
//    ChdGetZlist
//    ChdOpenOdb
//    ChdOpenZdb
//    ChdOpenZbdb
//
// The transform will be applied to all of the objects read through
// the CHD with these functions.  Why might this function be used?
// Consider the following:  suppose we have a CHD describing a cell
// hierarchy, the top-level cell of which is to be instantiated under
// another cell we'll call "root", with a given transformation.  We
// would like to consider the objects from the CHD from the
// perspective of the "root" cell.  This function would be called to
// set the transformation, then one of the flat read functions would
// be called and the returned objects accumulated.  The returned
// objects will have coordinates relative to the "root" cell, rather
// than relative to the top-level cell of the CHD.
//
// The tfstring describes the rotation and mirroring part of the
// transformation.  It is either one of the special tokens to be
// described, or a sequence of the following tokens:
//
//    MX
//      Flip the X axis.
//    MY
//      Flip the Y axis.
//    Rnnn
//      Rotate by nnn degrees.  The nnn must be one of 0, 45, 90, 135,
//      180, 225, 270, 315.
//
// White space can appear between tokens.  The operations are
// performed in order.  Note that, e.g., "MXR90" is very different
// from "R90MX".
//
// Alternatively, the tfstring can contain a single "Lef/Def" token as
// listed below.  The second column is the equivalent string using the
// syntax previously described.
//
//    N     null or empty or R0
//    S     R180
//    W     R90
//    E     R270
//    FN    MX
//    FS    MY
//    FW    MYR90
//    FE    MXR90
//
// The x and y are the translation part of the transformation.  These
// are coordinates, given in microns.
//
// If tfstring is null or empty, no rotations or mirroring will be
// used.
//
// The function returns 1 on success, 0 if the tfstring contains an
// error.
//
bool
cvrt_funcs::IFchdSetFlatReadTransform(Variable *res, Variable *args, void*)
{
    const char *tfname;
    ARG_CHK(arg_string(args, 0, &tfname))
    int x;
    ARG_CHK(arg_coord(args, 1, &x, Physical))
    int y;
    ARG_CHK(arg_coord(args, 2, &y, Physical))

    cTfmStack stk;
    bool ok = true;
    res->type = TYP_SCALAR;
    if (!tfname || !*tfname || lstring::cieq(tfname, "N")) {
        stk.TPush();
        stk.TTranslate(x, y);
        stk.TStore(CDtfRegI2);
        stk.TPop();
    }
    else if (lstring::cieq(tfname, "S")) {
        stk.TPush();
        stk.TRotate(-1, 0);
        stk.TTranslate(x, y);
        stk.TStore(CDtfRegI2);
        stk.TPop();
    }
    else if (lstring::cieq(tfname, "W")) {
        stk.TPush();
        stk.TRotate(0, 1);
        stk.TTranslate(x, y);
        stk.TStore(CDtfRegI2);
        stk.TPop();
    }
    else if (lstring::cieq(tfname, "E")) {
        stk.TPush();
        stk.TRotate(0, -1);
        stk.TTranslate(x, y);
        stk.TStore(CDtfRegI2);
        stk.TPop();
    }
    else if (lstring::cieq(tfname, "FN")) {
        stk.TPush();
        stk.TMX();
        stk.TTranslate(x, y);
        stk.TStore(CDtfRegI2);
        stk.TPop();
    }
    else if (lstring::cieq(tfname, "FS")) {
        stk.TPush();
        stk.TMY();
        stk.TTranslate(x, y);
        stk.TStore(CDtfRegI2);
        stk.TPop();
    }
    else if (lstring::cieq(tfname, "FW")) {
        stk.TPush();
        stk.TMY();
        stk.TRotate(0, 1);
        stk.TTranslate(x, y);
        stk.TStore(CDtfRegI2);
        stk.TPop();
    }
    else if (lstring::cieq(tfname, "FE")) {
        stk.TPush();
        stk.TMX();
        stk.TRotate(0, 1);
        stk.TTranslate(x, y);
        stk.TStore(CDtfRegI2);
        stk.TPop();
    }
    else {
        stk.TPush();
        char buf[32];
        const char *s = tfname;
        while (*s) {
            if (isspace(*s)) {
                s++;
                continue;
            }
            if (*s == 'R' || *s == 'r') {
                s++;
                char *t = buf;
                while (isdigit(*s) && t - buf < 4)
                    *t++ = *s++;
                *t = 0;
                if (!strcmp(buf, "0"))
                   ;
                else if (!strcmp(buf, "90"))
                    stk.TRotate(0, 1);
                else if (!strcmp(buf, "180"))
                    stk.TRotate(-1, 0);
                else if (!strcmp(buf, "270"))
                    stk.TRotate(0, -1);
                else if (!strcmp(buf, "45"))
                    stk.TRotate(1, 1);
                else if (!strcmp(buf, "135"))
                    stk.TRotate(-1, 1);
                else if (!strcmp(buf, "225"))
                    stk.TRotate(-1, -1);
                else if (!strcmp(buf, "315"))
                    stk.TRotate(1, -1);
                else {
                    ok = false;
                    break;
                }
                continue;
            }
            if (*s == 'M') {
                s++;
                if (*s == 'X' || *s == 'x') {
                    stk.TMX();
                    s++;
                    continue;
                }
                if (*s == 'Y' || *s == 'y') {
                    stk.TMY();
                    s++;
                    continue;
                }
            }
            ok = false;
            break;
        }
        if (ok) {
            stk.TTranslate(x, y);
            stk.TStore(CDtfRegI2);
        }
        stk.TPop();
    }
    res->content.value = ok;

    return (OK);
}


// (real) ChdEstFlatMemoryUse(chd_name, cellname, array, counts_array)
//
// This function will return an estimate of the memory required to
// perform a ChdOpenFlat call.  The first argument is the access name
// of an existing Cell Hierarchy Digest that was created with per-cell
// object counts saved (e.g., a call to OpenCellHierDigest with the
// info_saved argument set to 3 or 4).
//
// The cellname, if nonzero, must be the cell name after any aliasing
// that was in force when the CHD was created.  If cellname is passed
// 0, the default cell for the CHD is understood.  This is a cell name
// configured into the CHD, or the first top-level cell found in the
// archive file.
//
// The third argument is an array of size four or larger that contains
// the rectangular area as passed to the OpenFlat call.  The
// components are
//  array[0]  X left
//  array[1]  Y bottom
//  array[2]  X right
//  array[3]  Y top
// This argument can also be zero to indicate that the full area of
// the top level cell is to be considered.
//
// The final argument is also an array of size four or larger, or
// zero.  If an array is passed, and the function succeeds, the
// components are filled with the following values:
//  counts_array[0]  estimated total box count
//  counts_array[1]  estimated total polygon count
//  counts_array[2]  estimated total wire count
//  counts_array[3]  estimated total vertex count
// These are counts of objects that would be saved in the top-level
// cell during the OpenFlat call.  These are estimates, based on area
// normalization, and do not include any clipping or merging.  The
// vertex count is an estimate of the total number of polygon and wire
// vertices.
//
// The return value is an estimate, in megabytes, of the incremental
// memory required to perform the OpenFlat call.  This does not
// include normal overhead.
//
bool
cvrt_funcs::IFchdEstFlatMemoryUse(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))
    double *vals;
    ARG_CHK(arg_array_if(args, 2, &vals, 4))
    double *cnts;
    ARG_CHK(arg_array_if(args, 3, &cnts, 4))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        symref_t *top = 0;
        if (cname)
            top = chd->findSymref(cname, Physical);

        fmu_t fmu(chd, top);
        BBox BB;
        if (vals) {
            BB.left = INTERNAL_UNITS(vals[0]);
            BB.bottom = INTERNAL_UNITS(vals[1]);
            BB.right = INTERNAL_UNITS(vals[2]);
            BB.top = INTERNAL_UNITS(vals[3]);
            BB.fix();
        }

        if (!fmu.est_flat_memory_use(vals ? &BB : 0, &res->content.value))
            return (BAD);

        if (cnts) {
            cnts[0] = fmu.box_cnt;
            cnts[1] = fmu.poly_cnt;
            cnts[2] = fmu.wire_cnt;
            cnts[3] = fmu.vtex_cnt;
        }
    }
    else
        Errs()->add_error("Unresolved CHD access name");

    return (OK);
}


// (int) ChdWrite(chd_handle, scale, cellname, array, clip, all, flatten,
// ecf_level, outfile)
//
// This will write the cell named in the cellname string to the output
// file given in outfile, using the Cell Hierarchy Digest whose access
// name is given in the first argument to obtain layout data.
//
// If the outfile is null or empty, the geometry will be "written" as
// cells in the main database, hierarchically if all is true.  This
// allows windowing to be applied when converting a hierarchy, which
// will attempt to convert only objects and cells needed to render the
// window area.  This has the potential to hopelessly scramble your
// in-memory design data so be careful.
//
// See this table for the features that apply during a call to this
// function.
//
// The cellname, if nonzero, must be the cell name after any aliasing
// that was in force when the CHD was created.  If cellname is passed
// 0, the default cell for the CHD is understood.  This is a cell name
// configured into the CHD, or the first top-level cell found in the
// archive file.
//
// If the boolean argument all is nonzero, the hierarchy under the
// cell is written, otherwise only the named cell is written.  If the
// outfile is null or empty, native cell files will be created in the
// current directory.  If the outfile is the name of an existing
// directory, native cell files will be created in that directory.
// Otherwise, the extension of the outfile determines the file type:
//  CGX     .cgx
//  CIF     .cif
//  GDSII   .gds, .str, .strm, .stream
//  OASIS   .oas
// Only these extensions are recognized, however CGX and GDSII allow
// an additional .gz which will imply compression.
//
// The scale will multiply the scale factor in effect when the CHD was
// created, e.g., as provided to GetCellHierDigest.
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
// render the window will be written.  This window should be equal to
// or contained in the window used to configure the CHD, if any.
//
// If the boolean value clip is nonzero and an array is given, objects
// will be clipped to the window.  Otherwise no clipping is done.
//
// If the boolean value all is nonzero, the hierarchy under cellname
// is written, otherwise not.  If windowing is applied, this applies
// only to cellname, and not subcells.
//
// If the boolean variable flatten is nonzero, the objects in the
// hierarchy under cellname will be written into cellname, i.e.,
// flattened.  The all argument is ignored in this case.  Otherwise,
// no flattening is done.
//
// The ecf_level is an integer 0-3 which sets the empty cell filtering
// level, as described for the Conversion panel.  The values are
//
//   0   No empty cell filtering.
//   1   Apply pre- and post-filtering.
//   2   Apply pre-filtering only.
//   3   Apply post-filtering only.
//
// The return value is 1 on success, 0 on error, or -1 if an interrupt
// was received.  In the case of an error return, an error message may
// be available through GetError.
//
bool
cvrt_funcs::IFchdWrite(Variable *res, Variable *args, void*)
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
    bool allcells;
    ARG_CHK(arg_boolean(args, 5, &allcells))
    bool flatten;
    ARG_CHK(arg_boolean(args, 6, &flatten))
    int ecf_level;
    ARG_CHK(arg_int(args, 7, &ecf_level))
    const char *fname;
    ARG_CHK(arg_string(args, 8, &fname))

    ecf_level &= 0x3;

    if (!chdname || !*chdname)
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
        if (array) {
            prms.set_use_window(true);
            prms.set_window(&BB);
            prms.set_clip(clip);
        }
        prms.set_flatten(flatten);
        prms.set_ecf_level((ECFlevel)ecf_level);
        prms.set_allow_layer_mapping(true);
        prms.set_destination(fname, Fnone);

        res->content.value = chd->write(cname, &prms, allcells);
    }
    else
        Errs()->add_error("Unresolved CHD access name");
    return (OK);
}


// ChdWriteSplit(chd_name, cellname, basename, array, regions_or_gridsize,
//   numregions_or_bloatval, maxdepth, scale, flags)
//
// This function will read the geometry data through the Cell
// Hierarchy Digest (CHD) whose name is given as the first argument,
// into a collection of files representing rectangular regions of the
// top-level cell.  Each output file contains only the cells and
// geometry necessary to represent the region.  The regions can be
// specified as a list of rectangles, or as a grid.
//
// See this table for the features that apply during a call to this
// function.
//
// cellname
// The cellname, if nonzero, must be the cell name after any aliasing
// that was in force when the CHD was created.  If cellname is passed
// 0, the default cell for the CHD is understood.  This is a cell name
// configured into the CHD, or the first top-level cell found in the
// archive file.
//
// basename
// The basename is a cell path name in the form [/path/to/]basename.ext,
// where the extension ext gives the type of file to create.  One of
// the following extensions must be provided:
//
// .cgx  CGX output
// .cif  CIF output
// .gds, .str, .strm, .stream  GDSII output
// .oas, .oasis  OASIS output
//
// A ".gz" second extension is allowed following CGX and GDSII
// extensions in which case the files will be compressed using the
// gzip format.
//
// When writing a list of regions, the output files will be named in
// the form basename_N.ext, where the .ext is the extension supplied,
// and N is a 0-based index of the region, ordered as given.  When
// writing a grid, the output files will be named in the form
// basename_X_Y.ext, where the .ext is the extension supplied, and X,Y
// are integer indices representing the grid cell (origin is the
// lower-left corner).  If a directory path is prepended to the
// basename, the files will be found in that directory (which must
// exist, it will not be created).
//
// array
// The array argument can be 0, or the name of an array of size four
// or larger that contains a rectangle specification, in microns, in
// order L,B,R,T.  If given, the rectangle should intersect the
// bounding box of the top-level cell (cellname).  Only cells and
// geometry within this region will be written to output.  If 0 is
// passed, the entire bounding box of the top cell is understood.
//
// When writing grid files, the origin of the grid, before bloating,
// is at the lower-left corner of the area to be output.
//
// regions_or_gridsize
// This argument can be an array, or a scalar value.  If an array, the
// array consists of one or more rectangular area specifications, in
// order L,B,R,T in microns.  These are the regions that will be
// written to output files.
//
// If this argument is a number, it represents the size of a square
// grid cell, in microns.
//
// numregions_or_bloatval
// If an array was passed as the previous argument, then this argument
// is an integer giving the number of regions in the array to be
// written.  The size of the array is at least four times the number
// of regions.
//
// If instead a grid value was provided in the previous argument, then
// this argument provides a bloating value.  The grid cells will be
// bloated by this value (in microns) if the value is nonzero.  A
// positive value pushes out the grid cell edges by the value given, a
// negative value does the reverse.
//
// maxdepth
// This integer value applies only when flattening, and sets the
// maximum hierarchy depth for include in output.  If 0, only objects
// in the top-level cell will be included,
//
// scale
// This is a scale factor which will be applied to all output.  The
// gridsize, bloatval, and array coordinates are the sizes found in
// output, and are independent of the scale factor.  The valid range
// is 0.001 - 1000.0.
//
// flags
// This argument is a string consisting of specific letters, the presence
// of which sets one of several available modes.  These are
//   p      parallel
//   f      flatten
//   c      clip
//   n[N]   empty cell filtering
//   m      map names
//
// The character recognition is case-insensitive.  A null or empty string
// indicates no flags set.
//
// p
// If p is given, a parallel writing algorithm is used.  Otherwise,
// the output files are generated in sequence.  The files should be
// identical from either writing mode.  The parallel mode may be a
// little faster, but requires more internal memory.  When writing in
// parallel, the user may encounter system limitations on the number
// of file descriptors open simultaneously.
//
// f
// If f is given, the output will be flattened.  When flattening,
// an overall transformation can be set with
// ChdSetFlatReadTransform, in which case the given area description
// would apply in the "root" coordinates.
//
// If not given, the output files will be hierarchical, but only the
// subbcells needed to render the grid cell area, each containing only
// the geometry needed, will be written.
//
// c
// If c is given, objects will be clipped at the grid cell boundaries.
// This also applies to objects in subcells, when not flattening.
//
// n[N]
// The 'n' can optionally be followed by an integer 0-3.  If no
// integer follows, '3' is understood.  This sets the empty cell
// filtering level as described for the Conversion panel.  The values
// are
//
//   0   No empty cell filtering (no operation).
//   1   Apply pre- and post-filtering.
//   2   Apply pre-filtering only.
//   3   Apply post-filtering only.
//
// m
// If m is given, and f is also given (flattening), the top-level cell
// names in the output files will be modified so as to be unique in
// the collection.  A suffix "_N" is added to the cell name, where N
// is a grid cell or region index.  The index is 0 for the lower-left
// grid cell, and is incremented in the sweep order left to right,
// bottom to top.  If writing regions, the index is 0-based, in the
// order of the regions given.  Furthermore, a native cell file is
// written, named "basename_root", which calls each of the output
// files.  Loading this file will load the entire output collection,
// memory limits permitting.
//
// The function returns 1 on success, 0 otherwise, with an error
// message likely available from GetError.
//
bool
cvrt_funcs::IFchdWriteSplit(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))
    const char *bname;
    ARG_CHK(arg_string(args, 2, &bname))
    double *array;
    ARG_CHK(arg_array_if(args, 3, &array, 4))
    int gridsize = 0;
    double *regions = 0;
    int sz_or_bv = 0;
    if (args[4].type == TYP_SCALAR) {
        ARG_CHK(arg_coord(args, 4, &gridsize, Physical))
        ARG_CHK(arg_coord(args, 5, &sz_or_bv, Physical))
    }
    else if (args[4].type == TYP_ARRAY) {
        ARG_CHK(arg_array(args, 4, &regions, 4))
        ARG_CHK(arg_int(args, 5, &sz_or_bv))
    }
    int maxdepth;
    ARG_CHK(arg_depth(args, 6, &maxdepth))
    double scale;
    ARG_CHK(arg_real(args, 7, &scale))
    const char *flag_str;
    ARG_CHK(arg_string(args, 8, &flag_str))

    bool parallel = false;
    bool flat = false;
    bool clip = false;
    bool flat_map = false;
    ECFlevel ecf_level = ECFnone;
    if (flag_str) {
        for (const char *s = flag_str; *s; s++) {
            char c = isupper(*s) ? tolower(*s) : *s;
            if (c == 'p')
                parallel = true;
            else if (c == 'f')
                flat = true;
            else if (c == 'c')
                clip = true;
            else if (c == 'n') {
                if (isdigit(*(s+1))) {
                    s++;
                    if (*s == 1)
                        ecf_level = ECFall;
                    else if (*s == 2)
                        ecf_level = ECFpre;
                    else if (*s == 3)
                        ecf_level = ECFpost;
                }
                else
                    ecf_level = ECFall;
            }
            else if (c == 'm')
                flat_map = true;
        }
    }

    if (!chdname || !*chdname)
        return (BAD);
    if (!bname || !*bname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (!chd) {
        Errs()->add_error("Unresolved CHD access name");
        return (OK);
    }
    if (scale < CDSCALEMIN || scale > CDSCALEMAX) {
        Errs()->add_error("Scale factor out of range.");
        return (OK);
    }

    BBox BB;
    if (array) {
        BB.left = INTERNAL_UNITS(array[0]);
        BB.bottom = INTERNAL_UNITS(array[1]);
        BB.right = INTERNAL_UNITS(array[2]);
        BB.top = INTERNAL_UNITS(array[3]);
        BB.fix();
    }
    FileType ftype = FIO()->TypeExt(bname);
    if (!FIO()->IsSupportedArchiveFormat(ftype)) {
        Errs()->add_error(
            "Unrecognized or unsupported basename format extension.");
        return (OK);
    }
    char *bname_tmp = lstring::copy(bname);
    char *s = strrchr(bname_tmp, '.');
    if (s) {
        // Strip off the format extension, add back ".gz" if found.
        bool has_gz = false;
        if (lstring::cieq(s, ".gz")) {
            has_gz = true;
            *s = 0;
            s = strrchr(bname_tmp, '.');
        }
        *s = 0;
        if (has_gz && (ftype == Fgds || ftype == Fcgx))
            strcpy(s, ".gz");
    }
    bname = bname_tmp;

    FIOcvtPrms prms;
    prms.set_allow_layer_mapping(true);
    prms.set_destination(bname, ftype);
    if (array) {
        prms.set_use_window(true);
        prms.set_window(&BB);
    }
    prms.set_scale(scale);
    prms.set_flatten(flat);
    prms.set_clip(clip);
    prms.set_ecf_level(ecf_level);
    delete [] bname;

    Blist *bl0 = 0, *be = 0;
    if (regions) {
        if (sz_or_bv < 1) {
            Errs()->add_error("Region list size negative or 0.");
            return (OK);
        }
        if (args[4].content.a->length() < 4*sz_or_bv) {
            Errs()->add_error("Region array size too small for region count.");
            return (OK);
        }
        for (int i = 0; i < sz_or_bv; i++) {
            if (!bl0)
                bl0 = be = new Blist;
            else {
                be->next = new Blist;
                be = be->next;
            }
            be->BB.left = INTERNAL_UNITS(*regions);
            regions++;
            be->BB.bottom = INTERNAL_UNITS(*regions);
            regions++;
            be->BB.right = INTERNAL_UNITS(*regions);
            regions++;
            be->BB.top = INTERNAL_UNITS(*regions);
            regions++;
            be->BB.fix();
        }
    }

    res->content.value = chd->writeMulti(cname, &prms, bl0, gridsize,
        sz_or_bv, maxdepth, flat_map, parallel);

    Blist::destroy(bl0);
    return (OK);
}


// (int) ChdCreateReferenceCell(chdname, cellname)
//
// This function will create a "reference cell" in memory.  A
// reference cell is a special cell that references a cell hierarchy
// in an archive file, but does not have its own content.  Reference
// cells can be instantiated during editing like any other cell, but
// their content is not visible.  When a reference cell is written to
// disk as part of a cell hierarchy, the hierarchy of the reference
// cell is extracted from its source and streamed into the output.
//
// The first argument is a string giving the name of a Cell Hierarchy
// Digest (CHD) already in memory.  The second argument is the name of
// a cell in the CHD, which must include aliasing if aliasing was
// applied when the CHD was created.  This will also be the name of
// the reference cell.  A cell with this name should not already exist
// in current symbol table.
//
// Although the CHD is required for reference cell creation, it is not
// required when the reference cell is written, but will be used if
// present.  The archive file associated with the CHD should not be
// moved or altered before the reference cell is written to disk.
//
// A value 0 is returned on error, with a message probably available
// from GetError.  The value 1 is returned on success.
//
bool
cvrt_funcs::IFchdCreateReferenceCell(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))

    if (!chdname || !*chdname)
        return (BAD);
    if (!cname || !*cname)
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = 0;

    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (!chd) {
        Errs()->add_error("CHD not found with name %s.", chdname);
        return (OK);
    }
    res->content.value = chd->createReferenceCell(cname);
    return (OK);
}


// (int) ChdLoadCell(chdname, cellname)
//
// This function will load a cell into the main editing database, and
// subcells of the cell will be loaded as reference cells.  This
// allows the cell to be edited, without loading the hierarchy into
// memory.  When written to disk as part of a hierarchy, the cell
// hierarchies of the reference cells will be extracted from the input
// source and streamed to output.
//
// The first argument is a string giving the name of a Cell Hierarchy
// Digest (CHD) already in memory.  The second argument is the name of
// a cell in the CHD, which must include aliasing if aliasing was
// applied when the CHD was created.  This cell will be read into
// memory.  Any subcells used by the cell will be created in memory as
// reference cells, which a special cells which have no content but
// point to a source for their content.
//
// Although the CHD is required for reference cell creation, it is not
// required when the reference cell is written, but will be used if
// present.  The archive file associated with the CHD should not be
// moved or altered before the reference cell is written to disk.
//
// A value 0 is returned on error, with a message probably available
// from GetError.  The value 1 is returned on success.
//
bool
cvrt_funcs::IFchdLoadCell(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))

    if (!chdname || !*chdname)
        return (BAD);
    if (!cname || !*cname)
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = 0;

    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (!chd) {
        Errs()->add_error("CHD not found with name %s.", chdname);
        return (OK);
    }
    res->content.value = chd->loadCell(cname);
    return (OK);
}


// (int) ChdIterateOverRegion(chd_name, cellname, funcname, array,
//    coarse_mult, fine_grid, bloat_val)
//
// This function is an interface to a system which creates a logical
// rectangular grid over a cell hierarchy, then iterates over the
// partitions in the grid, performing some action on the logically flattened
// geometry.
//
// A Cell Hierarchy Digest is used to obtain the flattened geometry,
// with or without the assistance of a Cell Geometry Digest.  There
// are actually two levels of gridding:  the coarse grid, and the fine
// grid.  The area of interest is first logically partitioned into the
// coarse grid.  For each cell of the coarse grid, a "ZBDB" special
// database is created, using the fine grid.  For example, one might
// choose 400x400 microns for the coarse grid, and 20x20 microns for
// the fine grid.  Thus, geometry access is in 400x400 "chunks".  The
// geometry is extracted, flattened, and split into separate trapezoid
// lists for each fine grid area, for each layer.
//
// As each fine grid cell is visited, a user-supplied script function
// is called.  The operations performed are completely up to the user,
// and the framework is intended to be as flexible as possible.  As an
// example, one might extract geometric parameters such as density,
// minimum line width and spacing, for use by a process analysis tool. 
// Scalar parameters can be conveniently saved in spatial parameter
// tables (SPTs).
//
// The first argument is the access name of a CHD in memory.  The
// second argument is the top-level cell from the CHD, or if passed 0,
// the CHD's default cell will be used.
//
// The third argument is the name of a user-supplied script function
// which will implement the user's calculations.  The function should
// already be in memory before ChdIterateOverRegion is called.  This
// function is described in more detail below.
//
// The array argument can be 0, in which case the area of interest is
// the entire top-level cell.  Otherwise, the argument should be an
// array of size four or larger containing the rectangular area of
// interest, in order L,B,R,T in microns.  The coarse and find grid
// origin is at the lower left corner of the area of interest.
//
// The fine_grid argument is the size of the fine grid (which is
// square) in microns.  The coarse_mult is an integer representing the
// size of the coarse grid, in fine_grid quanta.
//
// The bloat_val argument specifies an amount, in microns, that the
// grid cells (both coarse and fine) should be expanded when returning
// geometry.  Geometry is clipped to the bloated grid.  Thus, it is
// possible to have some overlap in the geometry returned from
// adjacent grid cells.  This value can be negative, in which case
// grid cells will effectively shrink.
//
// The callback function has the following prototype.
//    (int) callback(db_name, j, i, spt_x, spt_y, data, cell_name,
//    chd_name)
// The function definition must start with the db_name and include the
// arguments in the order shown, but unused arguments to the right of
// the last needed argument can be omitted.
//
// db_name (string)
//         The access name of the ZBDB database containing geometry.
// j (integer)
//         The X index of the current fine grid cell.
// i (integer)
//         The Y index of the current fine grid cell.
// spt_x (real)
//         The X coordinate value in microns of the current grid cell
//         in a spatial parameter table:
//         coarse_grid_cell.left + j*fine_grid + fine_grid/2
// spt_y (real)
//         The Y coordinate value in microns of the current grid cell
//         in a spatial parameter table:
//         coarse_grid_cell.bottom + i*fini_grid + fine_grid/2
// data    (real array)
//         An array containing miscellaneous parameters, described below.
// cell_name (string)
//         The name of the top-level cell.
// chd_name (string)
//         The access name of the CHD.
//
// The data argument is an array that contains the following parameters.
// 
// index      description
// 0          The spatial parameter table column size.
// 1          The spatial parameter table row size.
// 2          The fine grid period, in microns.
// 3          The coarse grid period, in microns.
// 4          The amount of grid cell expansion in microns.
// 5          Area of interest left in microns.
// 6          Area of interest bottom in microns.
// 7          Area of interest right in microns.
// 8          Area of interest top in microns.
// 9          Coarse grid cell left in microns.
// 10         Coarse grid cell bottom in microns.
// 11         Coarse grid cell right in microns.
// 12         Coarse grid cell top in microns.
// 13         Fine grid cell left in microns.
// 14         Fine grid cell bottom in microns.
// 15         Fine grid cell right in microns.
// 16         Fine grid cell top in microns.
// 
// The trapezoid data for the grid cells can be accessed, from within
// the callback function, with the GetZlistZbdb function.
//     GetZlistZbdb(database_name, layer_lname, j, i)
//
// Example:
//    Here is a function that simply prints out the fine grid indices,
//    and the number of trapezoids in the grid location on a layer
//    named "M1".
//
//    function myfunc(dbname, j, i, x, y, data, cellname, chdname)
//        zlist = GetZlistZbdb(dbname, "M1", j, i);
//        Print("Location", j, i, "contains", Zlength(zlist), "zoids on M1.")
//    endfunc
//
// If the function returns a nonzero value, the operation will abort.
// If there is no explicit return statement, the return value is 0.
//
//   if (some error)
//       return 1
//   end
//
//
// If all goes well, ChdIterateOverRegion returns 1, otherwise 0 is
// returned, with an error message possibly available from GetError.
//
// This function is intended for OEM users, customization is possible.
// Contact Whiteley Research for more information.
//
bool
cvrt_funcs::IFchdIterateOverRegion(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))
    const char *funcname;
    ARG_CHK(arg_string(args, 2, &funcname))
    double *array;
    ARG_CHK(arg_array_if(args, 3, &array, 4))
    int cgm;
    ARG_CHK(arg_int(args, 4, &cgm))
    int fg;
    ARG_CHK(arg_coord(args, 5, &fg, Physical))
    int bv;
    ARG_CHK(arg_coord(args, 6, &bv, Physical))

    if (!chdname || !*chdname)
        return (BAD);
    if (!funcname || !*funcname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;

    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (!chd) {
        Errs()->add_error("CHD not found with name %s.", chdname);
        return (OK);
    }
    BBox BB;
    if (array) {
        BB.left = INTERNAL_UNITS(array[0]);
        BB.bottom = INTERNAL_UNITS(array[1]);
        BB.right = INTERNAL_UNITS(array[2]);
        BB.top = INTERNAL_UNITS(array[3]);
        BB.fix();
    }
    res->content.value = chd->iterateOverRegion(cname, funcname,
        array ? &BB : 0, cgm, fg, bv);
    return (OK);
}


// (int/stringlist_handle) ChdWriteDensityMaps(chd_name, cellname, array,
//    coarse_mult, fine_grid, bloat_val, save)
//
// This function uses the same framework as ChdIterateOverRegion, but
// is hard-coded to extract density values only.  The chd_name,
// cellname, array, coarse_mult, fine_grid and bloat arguments are as
// described for that function.
//
// When called, the function will iterate over the given area, and
// compute the fraction of dark area for each layer in a fine grid
// cell, saving the values in a spatial parameter table (SPT).  The
// access names of these SPTs are in the form cellname.layername,
// where cellname is the name of the top-level cell being processed. 
// The layername is the name of the layer, possibly in hex format as
// used elsewhere.
//
// If the boolean save argument is nonzero, the SPTs will be retained
// in memory after the function returns.  Otherwise, the SPTs will be
// dumped to files in the current directory, and destroyed.  The file
// names are the same as the SPT names, with a ".spt" extension added. 
// These files can be read with ReadSPtable, and are in the format
// described for that function, with the "reference coordinates" the
// central points of the fine grid cells.
//
// If all goes well, ChdWriteDensityMaps returns 1, otherwise 0 is
// returned, with an error message possibly available from GetError.
//
bool
cvrt_funcs::IFchdWriteDensityMaps(Variable *res, Variable *args, void*)
{
    const char *chdname;
    ARG_CHK(arg_string(args, 0, &chdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))
    double *array;
    ARG_CHK(arg_array_if(args, 2, &array, 4))
    int cgm;
    ARG_CHK(arg_int(args, 3, &cgm))
    int fg;
    ARG_CHK(arg_coord(args, 4, &fg, Physical))
    int bv;
    ARG_CHK(arg_int(args, 5, &bv))
    bool save;
    ARG_CHK(arg_boolean(args, 6, &save))

    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;

    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (!chd) {
        Errs()->add_error("CHD not found with name %s.", chdname);
        return (OK);
    }
    BBox BB;
    if (array) {
        BB.left = INTERNAL_UNITS(array[0]);
        BB.bottom = INTERNAL_UNITS(array[1]);
        BB.right = INTERNAL_UNITS(array[2]);
        BB.top = INTERNAL_UNITS(array[3]);
        BB.fix();
    }
    stringlist *spts = 0;
    bool ret = chd->createDensityMaps(cname, array ? &BB : 0, cgm, fg, bv,
        save ? &spts : 0);
    if (ret) {
        if (save) {
            sHdl *hdltmp = new sHdlString(spts);
            res->type = TYP_HANDLE;
            res->content.value = hdltmp->id;
        }
        else
            res->content.value = 1.0;
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Cell Geometry Digest
//-------------------------------------------------------------------------

// (cgd_name) OpenCellGeomDigest(idname, string, type)
//
// This function returns an access name to a new Cell Geometry
// Digest (CGD) which is created in memory.  A CGD is a data
// structure that provides access to cell geometry saved in compact
// form, and does not use the main cell database.  The CGD refers to
// physical data only.  The new CGD will be listed in the Cell
// Geometry Digests panel, and the access name is used by other
// functions to access the CGD.
//
// See this table for the features that apply during a call to this
// function.  In particular, the names of cells saved in the CGD
// reflect any aliasing that was in force at the time the CGD was
// created.
//
// The first argument is a specified access name (which will be
// returned on success).  This name can not be in use, meaning that
// the name can not access an existing CGD which is currently linked
// to a CHD.  If there is a name match to an unlinked CGD, the new
// CGD will replace the old (which is destroyed).  This argument can
// be passed 0 or an empty string.  If a null or empty string is
// passed, a new access name will be generated and assigned.
//
// The third argument is an integer 0-2 which specifies the type of
// CGD to create.  The second (string) argument depends on what type
// of CGD is being created.
//
// Type 0 (actually, mode not 1 or 2)
//
// This will create a "memory" CGD, where all geometry data will be
// stored in memory, in highly-compressed form.  This provides the
// most efficient access, but very large databases may exceed memory
// limitations.
//
// In this mode, the string argument can be one of the following:
// 1.  A layout (archive) file.  The file will be read and the
//     geometry extracted.
// 2.  The access name of a Cell Hierarchy Digest (CHD) in memory. 
//     The CHD will be used to read the geometry from the file it
//     references.
// 3.  A saved CHD file.  The file will be read, and a new CHD will
//     be created in memory.  This CHD will be used to read the geometry
//     from the file referenced.
// 4.  A saved CGD file name.  The file will be read into an
//     in-memory CGD.
//
// Files are opened from the library search path, if a full path is
// not provided.
//
// Type 1
//
// This will create a "file" CGD, where geometry data are stored in a
// CGD file on disk, and geometry is retrieved when needed via saved
// file offsets.  This uses less memory, but is not quite as fast as
// saving geometry data in memory.  It is generally much faster than
// reading geometry from the original layout file since 1) the data
// are highly compressed, and 2) the objects are pre-sorted by
// layer.
//
// In this mode, the string is a path to a saved CGD file (only). 
// The in-memory CGD will access this file.  The file is opened from
// the library search path, if a full path is not provided.
//
// Type 2
//
// This will create a stub CGD which obtains geometry information
// from a remote host which is running Xic in server mode.  The
// server must have a CGD in memory, from which data are obtained.
//
// In this mode, the string must be in the format
//
//    hostname[:port]/idname
//
// The [...] indicates "optional" and is not literal.  The hostname
// is the network name of the machine running the server.  If the
// server is using a non-default port number, the same port number
// should be provided after the host name, separated by a colon. 
// Following the hostname or port is the access name on the
// server of the CGD to access, separated by a forward slash.  The
// entire string should contain no white space.
//
// On error, a null string is returned, and an error message may be
// available with the GetError function.
//
bool
cvrt_funcs::IFopenCellGeomDigest(Variable *res, Variable *args, void*)
{
    const char *idname;
    ARG_CHK(arg_string(args, 0, &idname))
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))
    int mode;
    ARG_CHK(arg_int(args, 2, &mode))

    if (!string || !*string)
        return (BAD);
    res->type = TYP_STRING;
    res->content.string = 0;

    char *dbname;
    if (idname && *idname)
        dbname = lstring::copy(idname);
    else
        dbname = CDcgd()->newCgdName();
    CgdType tp = CGDmemory;
    if (mode == 1)
        tp = CGDfile;
    else if (mode == 2)
        tp = CGDremote;
    cCGD *cgd = FIO()->NewCGD(dbname, string, tp);
    if (!cgd)
        delete [] dbname;
    else {
        res->content.string = dbname;
        res->flags += VF_ORIGINAL;
    }
    return (OK);
}


// (string) NewCellGeomDigest()
//
// This function creates a new, empty Cell Geometry Digest, and returns
// the access name.  The CgdAddCells function can be used to add cell
// geometry.
//
bool
cvrt_funcs::IFnewCellGeomDigest(Variable *res, Variable*, void*)
{
    char *cgdname = CDcgd()->newCgdName();
    cCGD *cgd = new cCGD(0);
    CDcgd()->cgdStore(cgdname, cgd);

    res->type = TYP_STRING;
    res->content.string = cgdname;
    res->flags += VF_ORIGINAL;
    return (OK);
}


// (int) WriteCellGeomDigest(cgd_name, filename)
//
// This function will write a disk file representation of the Cell
// Geometry Digest (CGD) associated with the access name given as the
// first argument, into the file whose name is given as the second
// argument.  Subsequently, the file can be read with
// OpenCellGeomDigest to recreate the context.  The file has no other
// use and the format is not documented.
//
// The function returns 1 if the file was written successfully, 0
// otherwise, with an error message likely available from GetError.
//
bool
cvrt_funcs::IFwriteCellGeomDigest(Variable *res, Variable *args, void*)
{
    const char *cgdname;
    ARG_CHK(arg_string(args, 0, &cgdname))
    const char *filename;
    ARG_CHK(arg_string(args, 1, &filename))

    if (!cgdname || !*cgdname)
        return (BAD);
    if (!filename || !*filename)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCGD *cgd = CDcgd()->cgdRecall(cgdname, false);
    if (cgd)
        res->content.value = cgd->write(filename);
    else
        Errs()->add_error("Unresolved CGD access name");
    return (OK);
}


// (stringlist_handle) CgdList()
//
// This function returns a handle to a list of access strings to Cell
// Geometry Digests that are currently in memory.  The function never
// fails, though the handle may reference an empty list.
//
bool
cvrt_funcs::IFcgdList(Variable *res, Variable*, void*)
{
    stringlist *s0 = CDcgd()->cgdList();
    sHdl *hdltmp = new sHdlString(s0);
    res->type = TYP_HANDLE;
    res->content.value = hdltmp->id;
    return (OK);
}


// (int) CgdChangeName(old_cgd_name, new_cgd_name)
//
// This function allows the user to change the access name of an
// existing Cell Geometry Digest (CGD) to a user-supplied name.  The
// new name must not already be in use by another CGD.
//
// The first argument is the access name of an existing CGD, the
// second argument is the new access name, with which the CGD will
// subsequently be accessed.  This name can be any text string, but
// can not be null.
//
// The function returns 1 on success, 0 otherwise, with an error
// message likely available from GetError.
//
bool
cvrt_funcs::IFcgdChangeName(Variable *res, Variable *args, void*)
{
    const char *oldname;
    ARG_CHK(arg_string(args, 0, &oldname))
    const char *newname;
    ARG_CHK(arg_string(args, 1, &newname))

    if (!oldname || !*oldname)
        return (BAD);
    if (!newname || !*newname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCGD *cgd = CDcgd()->cgdRecall(oldname, true);
    if (cgd) {
        bool ok = CDcgd()->cgdStore(newname, cgd);
        if (!ok)
            // put it back
            CDcgd()->cgdStore(oldname, cgd);
        res->content.value = ok;
    }
    else
        Errs()->add_error("Unresolved CGD access name");
    return (OK);
}


// (int) CgdIsValid(cgd_name)
//
// This function returns one if the string argument is an access name
// of a Cell Geometry Digest currently in memory, zero otherwise.
//
bool
cvrt_funcs::IFcgdIsValid(Variable *res, Variable *args, void*)
{
    const char *cgdname;
    ARG_CHK(arg_string(args, 0, &cgdname))

    if (!cgdname || !*cgdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCGD *cgd = CDcgd()->cgdRecall(cgdname, false);
    if (cgd)
        res->content.value = 1;
    return (OK);
}


// (int) CgdDestroy(cgd_name)
//
// The string argument is the access name of a Cell Geometry Digest
// (CGD) currently in memory.  If the CGD is not currently linked to
// a Cell Hierarchy Digest (CHD), then the CGD will be destroyed and
// its memory freed.  One is returned if the CGD is found and
// deleted, zero otherwise.
//
bool
cvrt_funcs::IFcgdDestroy(Variable *res, Variable *args, void*)
{
    const char *cgdname;
    ARG_CHK(arg_string(args, 0, &cgdname))

    if (!cgdname || !*cgdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCGD *cgd = CDcgd()->cgdRecall(cgdname, false);
    if (cgd && !cgd->refcnt()) {
        delete cgd;
        res->content.value = 1;
    }
    return (OK);
}


// (int) CgdIsValidCell(cgdname, cellname)
//
// This function will return 1 if a Cell Geometry Digest (CGD) with an
// access name given as the first argument exists and contains data
// for the cell whose name is given as the second argument. 
// Otherwise, 0 is returned.
//
bool
cvrt_funcs::IFcgdIsValidCell(Variable *res, Variable *args, void*)
{
    const char *cgdname;
    ARG_CHK(arg_string(args, 0, &cgdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))

    if (!cgdname || !*cgdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCGD *cgd = CDcgd()->cgdRecall(cgdname, false);
    if (cgd)
        res->content.value = cgd->cell_test(cname);
    return (OK);
}


// (int) CgdIsValidLayer(cgdname, cellname, layername)
//
// This function returns 1 if the cgdname is an access name of a Cell
// Geometry Digest (CGD) in memory, which contains a cell cellname
// that has data for layer layername.  Otherwise, 0 is returned.
//
bool
cvrt_funcs::IFcgdIsValidLayer(Variable *res, Variable *args, void*)
{
    const char *cgdname;
    ARG_CHK(arg_string(args, 0, &cgdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))
    const char *lname;
    ARG_CHK(arg_string(args, 2, &lname))

    if (!cgdname || !*cgdname)
        return (BAD);
    if (!cname || !*cname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCGD *cgd = CDcgd()->cgdRecall(cgdname, false);
    if (cgd)
        res->content.value = cgd->layer_test(cname, lname);
    return (OK);
}


// (int) CgdRemoveCell(cgdname, cellname)
//
// This function will remove and destroy the data for the cell
// cellname from the Cell Geometry Digest (CGD) with access name
// cgdname.  This applies to all CGD types, as described for
// OpenCellGeomDigest.  If the CGD is accessing geometry from a
// remote server, the cell data are removed from the server.
//
// The names of cells that have been removed are retained, and can
// be checked with CgdIsCellRemoved.
//
// If the CGD is found, and it contains cellname, the cellname data
// are destroyed and the function returns 1.  Otherwise, 0 is
// returned, with an error message available from GetError.
//
bool
cvrt_funcs::IFcgdRemoveCell(Variable *res, Variable *args, void*)
{
    const char *cgdname;
    ARG_CHK(arg_string(args, 0, &cgdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))

    if (!cgdname || !*cgdname)
        return (BAD);
    if (!cname || !*cname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCGD *cgd = CDcgd()->cgdRecall(cgdname, false);
    if (cgd) {
        bool ret = cgd->remove_cell(cname);
        res->content.value = ret;
        if (!ret)
            Errs()->add_error("Unresolved cell name");
    }
    else
        Errs()->add_error("Unresolved CGD access name");
    return (OK);
}


// (int) CgdIsCellRemoved(cgdname, cellname)
//
// This function returns 1 if a CGD is found with access name as
// given in cgdname, and the cellname is the name of a cell that has
// been removed from the CGD, for example with CgdRemoveCell. 
// Otherwise, the return value is 0.
//
bool
cvrt_funcs::IFcgdIsCellRemoved(Variable *res, Variable *args, void*)
{
    const char *cgdname;
    ARG_CHK(arg_string(args, 0, &cgdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))

    if (!cgdname || !*cgdname)
        return (BAD);
    if (!cname || !*cname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCGD *cgd = CDcgd()->cgdRecall(cgdname, false);
    if (cgd)
        res->content.value = cgd->unlisted_test(cname);
    return (OK);
}


// (int) CgdRemoveLayer(cgdname, cellname, layername)
//
// If the CGD exists, and contains data for a cell cellname that
// contains data for layername, the layername data will be deleted
// from the cellname record, and the function returns 1.  Otherwise,
// 0 is returned, with an error message likely available from
// GetError.
//
// This applies to memory and file type CGDs, as described for
// OpenCellGeomDigest.  The data, if found, are freed, and (unlike
// CgdRemoveCell) no record of removed layers is retained.  This
// actually reduces memory use only for memory type CGDs.
//
bool
cvrt_funcs::IFcgdRemoveLayer(Variable *res, Variable *args, void*)
{
    const char *cgdname;
    ARG_CHK(arg_string(args, 0, &cgdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))
    const char *lname;
    ARG_CHK(arg_string(args, 2, &lname))

    if (!cgdname || !*cgdname)
        return (BAD);
    if (!cname || !*cname)
        return (BAD);
    if (!lname || !*lname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCGD *cgd = CDcgd()->cgdRecall(cgdname, false);
    if (cgd) {
        bool ret = cgd->remove_cell_layer(cname, lname);
        res->content.value = ret;
        if (!ret)
            Errs()->add_error("Unresolved cell or layer name");
    }
    else
        Errs()->add_error("Unresolved CGD access name");
    return (OK);
}


// (int) CgdAddCells(cgdname, chdname, cells_list)
//
// This function will add a list of cells to the Cell Geometry Digest
// (CGD) whose access name is given as the first argument.  The cells
// will be read using the Cell Hierarchy Digest (CHD) whose access
// name is given as the second argument.
//
// This, and the CgdRemoveCell function can be used to implement a
// cache for cell data.  When a CHD is used for access, and a CGD has
// been linked to the CHD, the CHD will read geometry information for
// cells in the CGD from the CGD, and cells not found in the CGD will
// be read from the layout file.  Thus, if memory is tight, one can
// put only the heavily-used cells into the CGD, instead of all cells.
//
// If the CGD already contains data for a cell to add, the data will
// be overwritten with the new cell data.
//
// For the cells_list argument, one can pass either a handle to a list
// of strings that contain cell names, or a string containing
// space-separated cell names.  If a cell named in the list is not
// found in the CHD, it will be silently ignored.
//
// This applies to memory and file type CGDs, as described for
// OpenCellGeomDigest.  The geometry records are saved in memory,
// whether or not the CGD is file type.  Individual records set the
// access method, so it is possible to have mixed file access and
// memory access records in the same CGD.
//
// On success, 1 is returned.  If an error occurs, 0 is returned, and
// a message mey be available from GetError.
//
bool
cvrt_funcs::IFcgdAddCells(Variable *res, Variable *args, void*)
{
    const char *cgdname;
    ARG_CHK(arg_string(args, 0, &cgdname))
    const char *chdname;
    ARG_CHK(arg_string(args, 1, &chdname))

    if (!cgdname || !*cgdname)
        return (BAD);
    if (!chdname || !*chdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;

    stringlist *sl = 0;
    bool free_sl = false;
    if (args[2].type == TYP_HANDLE) {
        int id;
        ARG_CHK(arg_handle(args, 2, &id))
        sHdl *hdl = sHdl::get(id);
        if (hdl) {
            if (hdl->type == HDLstring)
                sl = (stringlist*)hdl->data;
            else
                return (BAD);
        }
    }
    else if (args[2].type == TYP_STRING || args[2].type == TYP_NOTYPE) {
        const char *string;
        ARG_CHK(arg_string(args, 2, &string))
        stringlist *s0 = 0, *se = 0;
        const char *s = string;
        char *tok;
        while ((tok = lstring::gettok(&s)) != 0) {
            if (!s0)
                s0 = se = new stringlist(tok, 0);
            else {
                se->next = new stringlist(tok, 0);
                se = se->next;
            }
        }
        sl = s0;
        free_sl = true;
    }
    if (!sl) {
        res->content.value = 1;
        return (OK);
    }

    cCGD *cgd = CDcgd()->cgdRecall(cgdname, false);
    if (cgd) {
        cCHD *chd = CDchd()->chdRecall(chdname, false);
        if (chd)
            res->content.value = cgd->load_cells(chd, sl, 1.0, true);
        else
            Errs()->add_error("Unresolved CHD access name");
    }
    else
        Errs()->add_error("Unresolved CGD access name");
    if (free_sl)
        stringlist::destroy(sl);
    return (OK);
}


// (stringlist_handle) CgdContents(cgd_name, cellname, layername)
//
// This function returns content listings from the Cell Geometry
// Digest (CGD) whose access name is given in the first argument.  The
// remaining string arguments give the cell name and layer name to
// query.  Either or both of these arguments can be null (passed 0).
//
// If the cellname is null, a handle to a list if strings giving the
// cell names in the CGD is returned.  otherwise, the cellname must be
// a cell name from the CGD.
//
// If layername is null, the return value is handle to a list of layer
// name strings for layers used in cellname.  If layername is not
// null, it should be one of the layer names contained in the
// cellname.
//
// The return value when both cellname and layername are non-null is a
// handle to a list of two strings.  The first string gives the
// integer number of bytes of compressed geometry for the cell/layer.
// The second string gives the size of the geometry string after
// decompresion.  The compressed size can be 0, in which case
// compression was not used as the block is too small for compression
// to be effective.
//
// If the arguments are unresolved, the return value is a scalar 0.
//
bool
cvrt_funcs::IFcgdContents(Variable *res, Variable *args, void*)
{
    const char *cgdname;
    ARG_CHK(arg_string(args, 0, &cgdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))
    const char *lname;
    ARG_CHK(arg_string(args, 2, &lname))

    if (!cgdname || !*cgdname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCGD *cgd = CDcgd()->cgdRecall(cgdname, false);
    if (cgd) {
        stringlist *s0 = 0;
        if (!cname)
            s0 = cgd->cells_list();
        else if (!lname)
            s0 = cgd->layer_list(cname);
        else {
            size_t csz, usz;
            const unsigned char *data;
            if (!cgd->find_block(cname, lname, &csz, &usz, &data))
                return (BAD);
            if (data) {
                char buf[64];
                sprintf(buf, "%u", (unsigned int)usz);
                s0 = new stringlist(lstring::copy(buf), s0);
                sprintf(buf, "%u", (unsigned int)csz);
                s0 = new stringlist(lstring::copy(buf), s0);
            }
        }
        if (s0) {
            sHdl *hdltmp = new sHdlString(s0);
            res->type = TYP_HANDLE;
            res->content.value = hdltmp->id;
        }
    }
    else
        Errs()->add_error("Unresolved CGD access name");
    return (OK);
}


// (gs_handle) CgdOpenGeomStream(cgd_name, cellname, layername)
//
// This function creates a handle to an iterator for decompressing the
// geometry in a Cell Geometry Digest.  The first argument is the
// access name of the CGD.  The second argument is the name of one of
// the cells contained in the CGD.  The third argument is the name of
// a layer used by the cell.  The cells and layers in the CGD can be
// listed with CgdContents.
//
// The return value is a handle to an incremental reader, loaded with
// the compressed geometry for the cell and layer.  This can be passed
// to GsReadObject to obtain the geometrical objects.
//
// The Close function can be used to destroy the reader.  It will be
// closed automatically if GsReadObject iterates through all objects
// contained in the stream.
//
// A scalar 0 is returned if the arguments are not resolved.
//
bool
cvrt_funcs::IFcgdOpenGeomStream(Variable *res, Variable *args, void*)
{
    const char *cgdname;
    ARG_CHK(arg_string(args, 0, &cgdname))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))
    const char *lname;
    ARG_CHK(arg_string(args, 2, &lname))

    if (!cgdname || !*cgdname)
        return (BAD);
    if (!cname || !*cname)
        return (BAD);
    if (!lname || !*lname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    cCGD *cgd = CDcgd()->cgdRecall(cgdname, false);
    if (cgd) {
        size_t csz, usz;
        const unsigned char *data;
        if (!cgd->find_block(cname, lname, &csz, &usz, &data))
            return (BAD);
        if (data) {
            bstream_t *bs = new bstream_t(data, csz, usz);

            FIOcvtPrms prms;
            cv_incr_reader *ircr = new cv_incr_reader(bs, &prms);

            sHdl *hdltmp = new sHdlBstream(ircr);
            res->type = TYP_HANDLE;
            res->content.value = hdltmp->id;
        }
    }
    return (OK);
}


// (obj_handle) GsReadObject(gs_handle)
//
// This function takes the handle created with CgdOpenGeomStream and
// returns an object handle which points to a single object.  A
// different object will be returned with each call until all objects
// have been returned, at which time the geometry stream handle is
// closed.  Further calls will return a scalar 0.
//
// The ConvertReply function can also return a handle for use by this
// function.
//
bool
cvrt_funcs::IFgsReadObject(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLbstream)
            return (BAD);
        cv_incr_reader *irdr = (cv_incr_reader*)hdl->iterator();
        CDo *odesc = irdr->next_object();
        if (odesc) {
            CDol *ol = new CDol(odesc, 0);
            hdl = new sHdlObject(ol, 0, true);
            res->type = TYP_HANDLE;
            res->content.value = hdl->id;
        }
        else
            hdl->close(0);
    }
    return (OK);
}


// (int) GsDumpOasisText(gs_handle)
//
// This function will dump the geometry stream in OASIS ascii text
// representation to the console window (standard output).  The
// handle is freed.  This may be useful for debugging.
//
bool
cvrt_funcs::IFgsDumpOasisText(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLbstream)
            return (BAD);
        cv_incr_reader *irdr = (cv_incr_reader*)hdl->iterator();
        irdr->reader()->setup_ascii_out("", 0, 0, -1, -1);
        irdr->reader()->parse(Physical, false, 1.0);
        hdl->close(0);
        res->content.value = 1;
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Assembly Stream
//-------------------------------------------------------------------------

// (stream_handle) StreamOpen(outfile)
//
// Open an assembly stream to the file outfile.  The file format that
// will be used is obtained from the extention of the name given,
// which must be one of
//
//  CGX     .cgx
//  CIF     .cif
//  GDSII   .gds, .str, .strm, .stream
//  OASIS   .oas
//
// If successful, a handle to the stream control structure is
// returned, which can be passed to other functions which require this
// data type.  A scalar zero is returned on error.  The returned
// handle is used to implement merging of archive data similar to the
// !assemble command.
//
bool
cvrt_funcs::IFstreamOpen(Variable *res, Variable *args, void*)
{
    const char *fname;
    ARG_CHK(arg_string(args, 0, &fname))

    if (!fname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    ajob_t *job = new ajob_t(fname);
    if (job->open_stream()) {
        sHdl *hdl = new sHdlAjob(job);
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    else
        delete job;
    return (OK);
}


// (int) StreamTopCell(stream_handle, cellname)
//
// Define the name of a top-level cell that will be created in the
// output stream.  At most one definition is possible in a stream.  If
// successful one is returned, otherwise zero is returned.
//
bool
cvrt_funcs::IFstreamTopCell(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))

    if (id < 0)
        return (BAD);
    if (!cname)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLajob)
            return (BAD);
        ajob_t *job = (ajob_t*)hdl->data;
        res->content.value = job->add_topcell(cname);
    }
    return (OK);
}


// (int) StreamSource(stream_handle, file_or_chd, scale, layer_filter,
//   name_change)
//
// This function will add a source specification to a stream.  The
// specification can refer to either an archive file, or to a Cell
// Hierarchy Digest (CHD).  Upon successful return, the source will be
// queued for writing to the stream (initiated with StreamRun).
// Arguments set various modes and conditions that will apply during
// the write.
//
// This function specifies the equivalent of a Source Block as
// described for the !assemble command.  The StreamInstance function
// is used to add "Placement Blocks".
//
// stream_handle
//   Handle to the stream object.
//
// file_or_chd
//   This argument can be either a string giving a path to an archive
//   file, or the access name of a Cell Hierarchy Digest in memory.
//
// scale
//   This is a scaling factor which applies only when streaming the
//   entire file, which will occur if no instances are specified for the
//   source with the StreamInstance function.  It is ignored if an
//   instance is specified.  When used, all coordinates read from the
//   source file will be multiplied by the factor, which can be in the
//   range 0.001 - 1000.0.
//
// layer_filter
//   This is a switch integer that enables or disables use of the layer
//   filtering and aliasing capability.  If 0, no layer filtering or
//   aliasing will be done.  If nonzero, layer filtering and aliasing
//   will be be performed when reading from the source, according to
//   the present values of the variables listed below.  These values are
//   saved, so that the variables can subsequently change.
//
//     LayerList
//     UseLayerList
//     LayerAlias
//     UseLayerAlias
//
//   If needed, these variables should be set to the desired values
//   before calling this function, then reset to the previous values
//   after the call.  This can be done with the Get, and Set functions.
//
// name_change
//   This is a switch integer that enables or disables use of the Cell
//   Name Mapping capability.  If 0, no cell name changes are done,
//   except that if a name clash is detected, a new name will be
//   supplied, similar to the auto-aliasing feature.  If nonzero, cell
//   name mapping will be performed when the source is read according to
//   the present values of the variables listed below.  These values are
//   saved, so that the variables can subsequently change.
//
//     InCellNamePrefix
//     InCellNameSuffix
//     InToLower
//     InToUpper
//
//   If needed, these variables should be set to the desired values
//   before calling this function, then reset to the previous values
//   after the call.  This can be done with the Get, and Set functions.
//
// The function returns one on success, zero otherwise with an
// error message probably available through GetError.
//
bool
cvrt_funcs::IFstreamSource(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *chd_or_fname = 0;
    ARG_CHK(arg_string(args, 1, &chd_or_fname))
    double scale;
    ARG_CHK(arg_real(args, 2, &scale))
    int lfilt;
    ARG_CHK(arg_int(args, 3, &lfilt))
    int namechg;
    ARG_CHK(arg_int(args, 4, &namechg))

    if (id < 0)
        return (BAD);
    if (!chd_or_fname || !*chd_or_fname)
        return (BAD);

    const char *layer_list =
        lfilt ? CDvdb()->getVariable(VA_LayerList) : 0;
    const char *use_layer_list =
        lfilt ? CDvdb()->getVariable(VA_UseLayerList) : 0;
    const char *only_layers = 0;
    const char *skip_layers = 0;
    if (use_layer_list) {
        if (*use_layer_list == 'n' || *use_layer_list == 'N')
            skip_layers = layer_list;
        else
            only_layers = layer_list;
    }
    const char *layer_aliases = 0;
    if (lfilt && CDvdb()->getVariable(VA_UseLayerAlias))
        layer_aliases = CDvdb()->getVariable(VA_LayerAlias);

    const char *cname_prefix =
        namechg ? CDvdb()->getVariable(VA_InCellNamePrefix) : 0;
    const char *cname_suffix =
        namechg ? CDvdb()->getVariable(VA_InCellNameSuffix) : 0;
    int ccvt = 0;
    if (namechg) {
        if (CDvdb()->getVariable(VA_InToLower))
            ccvt |= 1;
        if (CDvdb()->getVariable(VA_InToUpper))
            ccvt |= 2;
    }

    cCHD *chd = CDchd()->chdRecall(chd_or_fname, false);
    if (chd)
        chd_or_fname = 0;
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLajob)
            return (BAD);
        ajob_t *job = (ajob_t*)hdl->data;
        res->content.value = job->add_source(chd_or_fname, chd, scale,
            only_layers, skip_layers, layer_aliases, cname_prefix,
            cname_suffix, ccvt);
    }
    return (OK);
}


// (int) StreamInstance(stream_handle, cellname, x, y, my, ang, magn,
//     scale, no_hier, ecf_level, flatten, array, clip)
//
// This function will add a placement name to the most recently added
// source file (using StreamSource).  A source must have been
// specified before this function can be called successfully.  This
// function specifies the equivalent of a Placement Block as described
// for the !assemble command.
//
// The cellname must match the name of a cell found in the source,
// including any aliasing in effect.  There are two consequences of
// calling this function:  the named cell and possibly its subcell
// hierarchy will be written to output, and if a top cell was
// specified (with StreamTopCell), an instance of the named cell will
// be placed in the top cell.  The placement is governed by the x, y,
// my, ang, and magn arguments, which are ignored if there is no top
// cell.
//
// The x,y are the translation coordinates of the cell origin.  The my
// is a flag indicating Y-reflection before rotation.  The ang is the
// rotation angle, in degrees, and must be a multiple of 45 degrees. 
// The magn is the magnification factor for the placement.  These
// apply to the instantiation only, and have no effect on the cell
// definitions.
//
// The remaining arguments affect the cell definitions that are
// created in the output file.
//
// scale
//   This is a scale factor by which all coordinates are scaled in cell
//   definition output, and is a real number in the range 0.001 -
//   1000.0.  This is different from the magn factor, which applies only
//   to the instance placement.
//
// no_hier
//   This is a boolean value that when nonzero indicates that only the
//   named cell, and not its hierarchy, is written to output.  This can
//   cause the output file to have unresolved references.
//
// ecf_level
//   This is an integer 0-3 which specifies the empty cell filtering
//   level as described for the Conversion panel.  The values are
//
//   0   No empty cell filtering.
//   1   Apply pre- and post-filtering.
//   2   Apply pre-filtering only.
//   3   Apply post-filtering only.
//
// flatten
//   If the boolean variable flatten is nonzero, the objects in the
//   hierarchy under cellname will be created in cellname, thus only one
//   cell, containing all geometry, will be written.
//
// array
//   If the array argument is passed 0, no windowing will be used. 
//   Otherwise the array should have four components which specify a
//   rectangle, in microns, in the coordinates of cellname.  The values
//   are
//     array[0] X left
//     array[1] Y bottom
//     array[2] X right
//     array[3] Y top
//
//   If an array is given, only the objects and subcells needed to
//   render the window will be written.  This window should be equal to
//   or contained in the window used to configure the context, if any.
//
// clip
//   If the boolean value clip is nonzero and an array is given, objects
//   will be clipped to the window.  Otherwise no clipping is done.
//
// The function returns one on success, zero otherwise with an error
// message probably available through GetError.
//
//
bool
cvrt_funcs::IFstreamInstance(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *cname;
    ARG_CHK(arg_string(args, 1, &cname))
    int tx;
    ARG_CHK(arg_coord(args, 2, &tx, Physical))
    int ty;
    ARG_CHK(arg_coord(args, 3, &ty, Physical))
    bool my;
    ARG_CHK(arg_boolean(args, 4, &my))
    double ang;
    ARG_CHK(arg_real(args, 5, &ang))
    double magn;
    ARG_CHK(arg_real(args, 6, &magn))
    double scale;
    ARG_CHK(arg_real(args, 7, &scale))
    bool no_hier;
    ARG_CHK(arg_boolean(args, 8, &no_hier))
    int ecf_level;
    ARG_CHK(arg_int(args, 9, &ecf_level))
    bool flatten;
    ARG_CHK(arg_boolean(args, 10, &flatten))
    double *array;
    ARG_CHK(arg_array_if(args, 11, &array, 4))
    bool clip;
    ARG_CHK(arg_boolean(args, 12, &clip))

    ecf_level &= 0x3;

    if (id < 0)
        return (BAD);
    if (!cname)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLajob)
            return (BAD);
        ajob_t *job = (ajob_t*)hdl->data;
        BBox BB;
        if (array) {
            BB.left = INTERNAL_UNITS(array[0]);
            BB.bottom = INTERNAL_UNITS(array[1]);
            BB.right = INTERNAL_UNITS(array[2]);
            BB.top = INTERNAL_UNITS(array[3]);
            BB.fix();
        }
        res->content.value = job->add_instance(cname, tx, ty, my, ang, magn,
            scale, no_hier, (ECFlevel)ecf_level, flatten,
            array ? &BB : 0, (array && clip));
    }
    return (OK);
}


// (int) StreamRun(stream_handle)
//
// This function will initiate the writing from the sources previously
// specified with SteamSource into the output file.  The real work is
// done here.  The function returns one on success, zero otherwise with an
// error message probably available through GetError.
//
bool
cvrt_funcs::IFstreamRun(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    if (id < 0)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLajob)
            return (BAD);
        ajob_t *job = (ajob_t*)hdl->data;
        res->content.value = job->run(0);
    }
    return (OK);
}

