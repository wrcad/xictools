
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
#include "scedif.h"
#include "python_if.h"
#include "tcltk_if.h"
#include "dsp_color.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "cd_layer.h"
#include "cd_celldb.h"
#include "fio.h"
#include "fio_assemble.h"
#include "fio_library.h"
#include "fio_alias.h"
#include "si_parsenode.h"
// xdraw.h must come before si_handle.h to enable xdraw handle.
#include "ginterf/xdraw.h"
#include "si_handle.h"
#include "si_args.h"
#include "si_parser.h"
#include "si_interp.h"
#include "tech_attr_cx.h"
#include "main_scriptif.h"
#include "cfilter.h"
#include "promptline.h"
#include "select.h"
#include "ghost.h"
#include "keymap.h"
#include "pushpop.h"
#include "tech.h"
#include "errorlog.h"
#include "miscutil/filestat.h"
#include "miscutil/crypt.h"
#ifdef WIN32
#include "miscutil/msw.h"
#endif


////////////////////////////////////////////////////////////////////////
//
// Script Functions:  Control Functions
//
////////////////////////////////////////////////////////////////////////

namespace {
    namespace misc1_funcs {
        inline WindowDesc *
        getwin(int win)
        {
            if (win < 0 || win >= DSP_NUMWINS)
                return (0);
            return (DSP()->Window(win));
        }

        // Current Cell
        bool IFedit(Variable*, Variable*, void*);
        bool IFopenCell(Variable*, Variable*, void*);
        bool IFtouchCell(Variable*, Variable*, void*);
        bool IFpush(Variable*, Variable*, void*);
        bool IFpushElement(Variable*, Variable*, void*);
        bool IFpop(Variable*, Variable*, void*);
        bool IFnewCellName(Variable*, Variable*, void*);
        bool IFcurCellName(Variable*, Variable*, void*);
        bool IFtopCellName(Variable*, Variable*, void*);
        bool IFfileName(Variable*, Variable*, void*);
        bool IFcurCellBB(Variable*, Variable*, void*);
        bool IFsetCellFlag(Variable*, Variable*, void*);
        bool IFgetCellFlag(Variable*, Variable*, void*);
        bool IFsave(Variable*, Variable*, void*);
        bool IFupdateNative(Variable*, Variable*, void*);

        // Cell Info
        bool IFcellBB(Variable*, Variable*, void*);
        bool IFlistSubcells(Variable*, Variable*, void*);
        bool IFlistParents(Variable*, Variable*, void*);
        bool IFinitGen(Variable*, Variable*, void*);
        bool IFcellsHandle(Variable*, Variable*, void*);
        bool IFgenCells(Variable*, Variable*, void*);

        // Database
        bool IFclear(Variable*, Variable*, void*);
        bool IFclearAll(Variable*, Variable*, void*);
        bool IFisCellInMem(Variable*, Variable*, void*);
        bool IFisFileInMem(Variable*, Variable*, void*);
        bool IFnumCellsInMem(Variable*, Variable*, void*);
        bool IFlistCellsInMem(Variable*, Variable*, void*);
        bool IFlistTopCellsInMem(Variable*, Variable*, void*);
        bool IFlistModCellsInMem(Variable*, Variable*, void*);
        bool IFlistTopFilesInMem(Variable*, Variable*, void*);

        // Symbol Tables
        bool IFsetSymbolTable(Variable*, Variable*, void*);
        bool IFclearSymbolTable(Variable*, Variable*, void*);
        bool IFcurSymbolTable(Variable*, Variable*, void*);

        // Display
        bool IFwindow(Variable*, Variable*, void*);
        bool IFgetWindow(Variable*, Variable*, void*);
        bool IFgetWindowView(Variable*, Variable*, void*);
        bool IFgetWindowMode(Variable*, Variable*, void*);
        bool IFexpand(Variable*, Variable*, void*);
        bool IFdisplay(Variable*, Variable*, void*);
        bool IFfreezeDisplay(Variable*, Variable*, void*);
        bool IFredraw(Variable*, Variable*, void*);

        // Exit
        bool IFexit(Variable*, Variable*, void*);

        // Annotation
        bool IFaddMark(Variable*, Variable*, void*);
        bool IFeraseMark(Variable*, Variable*, void*);
        bool IFdumpMarks(Variable*, Variable*, void*);
        bool IFreadMarks(Variable*, Variable*, void*);

        // Ghost Rendering
        bool IFpushGhost(Variable*, Variable*, void*);
        bool IFpushGhostBox(Variable*, Variable*, void*);
        bool IFpushGhostH(Variable*, Variable*, void*);
        bool IFpopGhost(Variable*, Variable*, void*);
        bool IFshowGhost(Variable*, Variable*, void*);

        // Graphics
        bool IFgrOpen(Variable*, Variable*, void*);
        bool IFgrCheckError(Variable*, Variable*, void*);
        bool IFgrCreatePixmap(Variable*, Variable*, void*);
        bool IFgrDestroyPixmap(Variable*, Variable*, void*);
        bool IFgrCopyDrawable(Variable*, Variable*, void*);
        bool IFgrDraw(Variable*, Variable*, void*);
        bool IFgrGetDrawableSize(Variable*, Variable*, void*);
        bool IFgrResetDrawable(Variable*, Variable*, void*);
        bool IFgrClear(Variable*, Variable*, void*);
        bool IFgrPixel(Variable*, Variable*, void*);
        bool IFgrPixels(Variable*, Variable*, void*);
        bool IFgrLine(Variable*, Variable*, void*);
        bool IFgrPolyLine(Variable*, Variable*, void*);
        bool IFgrLines(Variable*, Variable*, void*);
        bool IFgrBox(Variable*, Variable*, void*);
        bool IFgrBoxes(Variable*, Variable*, void*);
        bool IFgrArc(Variable*, Variable*, void*);
        bool IFgrPolygon(Variable*, Variable*, void*);
        bool IFgrText(Variable*, Variable*, void*);
        bool IFgrTextExtent(Variable*, Variable*, void*);
        bool IFgrDefineColor(Variable*, Variable*, void*);
        bool IFgrSetBackground(Variable*, Variable*, void*);
        bool IFgrSetWindowBackground(Variable*, Variable*, void*);
        bool IFgrSetColor(Variable*, Variable*, void*);
        bool IFgrDefineLinestyle(Variable*, Variable*, void*);
        bool IFgrSetLinestyle(Variable*, Variable*, void*);
        bool IFgrDefineFillpattern(Variable*, Variable*, void*);
        bool IFgrSetFillpattern(Variable*, Variable*, void*);
        bool IFgrUpdate(Variable*, Variable*, void*);
        bool IFgrSetMode(Variable*, Variable*, void*);

        // Hard Copy
        bool IFhcListDrivers(Variable*, Variable*, void*);
        bool IFhcSetDriver(Variable*, Variable*, void*);
        bool IFhcGetDriver(Variable*, Variable*, void*);
        bool IFhcSetResol(Variable*, Variable*, void*);
        bool IFhcGetResol(Variable*, Variable*, void*);
        bool IFhcGetResols(Variable*, Variable*, void*);
        bool IFhcSetBestFit(Variable*, Variable*, void*);
        bool IFhcGetBestFit(Variable*, Variable*, void*);
        bool IFhcSetLegend(Variable*, Variable*, void*);
        bool IFhcGetLegend(Variable*, Variable*, void*);
        bool IFhcSetLandscape(Variable*, Variable*, void*);
        bool IFhcGetLandscape(Variable*, Variable*, void*);
        bool IFhcSetMetric(Variable*, Variable*, void*);
        bool IFhcGetMetric(Variable*, Variable*, void*);
        bool IFhcSetSize(Variable*, Variable*, void*);
        bool IFhcGetSize(Variable*, Variable*, void*);
        bool IFhcShowAxes(Variable*, Variable*, void*);
        bool IFhcShowGrid(Variable*, Variable*, void*);
        bool IFhcSetGridInterval(Variable*, Variable*, void*);
        bool IFhcSetGridStyle(Variable*, Variable*, void*);
        bool IFhcSetGridCrossSize(Variable*, Variable*, void*);
        bool IFhcSetGridOnTop(Variable*, Variable*, void*);
        bool IFhcDump(Variable*, Variable*, void*);
        bool IFhcErrorString(Variable*, Variable*, void*);
        bool IFhcListPrinters(Variable*, Variable*, void*);
        bool IFhcMedia(Variable*, Variable*, void*);

        // Keyboard
        bool IFreadKeymap(Variable*, Variable*, void*);

        // Libraries
        bool IFopenLibrary(Variable*, Variable*, void*);
        bool IFcloseLibrary(Variable*, Variable*, void*);

        // Mode
        bool IFmode(Variable*, Variable*, void*);
        bool IFcurMode(Variable*, Variable*, void*);

        // Prompt Line
        bool IFstuffText(Variable*, Variable*, void*);
        bool IFtextCmd(Variable*, Variable*, void*);
        bool IFgetLastPrompt(Variable*, Variable*, void*);

        // Scripts
        bool IFlistFunctions(Variable*, Variable*, void*);
        bool IFexec(Variable*, Variable*, void*);
        bool IFsetKey(Variable*, Variable*, void*);
        bool IFhasPython(Variable*, Variable*, void*);
        bool IFrunPython(Variable*, Variable*, void*);
        bool IFrunPythonModFunc(Variable*, Variable*, void*);
        bool IFresetPython(Variable*, Variable*, void*);
        bool IFhasTcl(Variable*, Variable*, void*);
        bool IFhasTk(Variable*, Variable*, void*);
        bool IFrunTcl(Variable*, Variable*, void*);
        bool IFresetTcl(Variable*, Variable*, void*);
        bool IFhasGlobalVariable(Variable*, Variable*, void*);
        bool IFgetGlobalVariable(Variable*, Variable*, void*);
        bool IFsetGlobalVariable(Variable*, Variable*, void*);

        // Technology File
        bool IFgetTechName(Variable*, Variable*, void*);
        bool IFgetTechExt(Variable*, Variable*, void*);
        bool IFsetTechExt(Variable*, Variable*, void*);
        bool IFtechParseLine(Variable*, Variable*, void*);
        bool IFtechGetFkeyString(Variable*, Variable*, void*);
        bool IFtechSetFkeyString(Variable*, Variable*, void*);

        // Variables
        bool IFset(Variable*, Variable*, void*);
        bool IFunset(Variable*, Variable*, void*);
        bool IFpushSet(Variable*, Variable*, void*);
        bool IFpopSet(Variable*, Variable*, void*);
        bool IFsetExpand(Variable*, Variable*, void*);
        bool IFget(Variable*, Variable*, void*);

        // Xic Version
        bool IFversionString(Variable*, Variable*, void*);
    }
    using namespace misc1_funcs;

#ifdef HAVE_PYTHON
    // Python wrappers.

    // Current Cell
    PY_FUNC(Edit,                   2,  IFedit);
    PY_FUNC(OpenCell,               3,  IFopenCell);
    PY_FUNC(TouchCell,              2,  IFtouchCell);
    PY_FUNC(Push,                   1,  IFpush);
    PY_FUNC(PushElement,            3,  IFpushElement);
    PY_FUNC(Pop,                    0,  IFpop);
    PY_FUNC(NewCellName,            0,  IFnewCellName);
    PY_FUNC(CurCellName,            0,  IFcurCellName);
    PY_FUNC(TopCellName,            0,  IFtopCellName);
    PY_FUNC(FileName,               0,  IFfileName);
    PY_FUNC(CurCellBB,              1,  IFcurCellBB);
    PY_FUNC(SetCellFlag,            3,  IFsetCellFlag);
    PY_FUNC(GetCellFlag,            2,  IFgetCellFlag);
    PY_FUNC(Save,                   1,  IFsave);
    PY_FUNC(UpdateNative,           1,  IFupdateNative);

    // Cell Info
    PY_FUNC(CellBB,           VARARGS,  IFcellBB);
    PY_FUNC(ListSubcells,           4,  IFlistSubcells);
    PY_FUNC(ListParents,            1,  IFlistParents);
    PY_FUNC(InitGen,                0,  IFinitGen);
    PY_FUNC(CellsHandle,            2,  IFcellsHandle);
    PY_FUNC(GenCells,               1,  IFgenCells);

    // Database
    PY_FUNC(Clear,                  1,  IFclear);
    PY_FUNC(ClearAll,               1,  IFclearAll);
    PY_FUNC(IsCellInMem,            1,  IFisCellInMem);
    PY_FUNC(IsFileInMem,            1,  IFisFileInMem);
    PY_FUNC(NumCellsInMem,          0,  IFnumCellsInMem);
    PY_FUNC(ListCellsInMem,         1,  IFlistCellsInMem);
    PY_FUNC(ListTopCellsInMem,      0,  IFlistTopCellsInMem);
    PY_FUNC(ListModCellsInMem,      0,  IFlistModCellsInMem);
    PY_FUNC(ListTopFilesInMem,      0,  IFlistTopFilesInMem);

    // Symbol Tables
    PY_FUNC(SetSymbolTable,         1,  IFsetSymbolTable);
    PY_FUNC(ClearSymbolTable,       1,  IFclearSymbolTable);
    PY_FUNC(CurSymbolTable,         0,  IFcurSymbolTable);

    // Display
    PY_FUNC(Window,                 4,  IFwindow);
    PY_FUNC(GetWindow,              0,  IFgetWindow);
    PY_FUNC(GetWindowView,          2,  IFgetWindowView);
    PY_FUNC(GetWindowMode,          1,  IFgetWindowMode);
    PY_FUNC(Expand,                 2,  IFexpand);
    PY_FUNC(Display,                6,  IFdisplay);
    PY_FUNC(FreezeDisplay,          1,  IFfreezeDisplay);
    PY_FUNC(Redraw,                 1,  IFredraw);

    // Exit
    PY_FUNC(Exit,                   0,  IFexit);

    // Annotation
    PY_FUNC(AddMark,          VARARGS,  IFaddMark);
    PY_FUNC(EraseMark,              1,  IFeraseMark);
    PY_FUNC(DumpMarks,              1,  IFdumpMarks);
    PY_FUNC(ReadMarks,              1,  IFreadMarks);

    // Ghost Rendering
    PY_FUNC(PushGhost,              2,  IFpushGhost);
    PY_FUNC(PushGhostBox,           4,  IFpushGhostBox);
    PY_FUNC(PushGhostH,             2,  IFpushGhostH);
    PY_FUNC(PopGhost,               0,  IFpopGhost);
    PY_FUNC(ShowGhost,              1,  IFshowGhost);

    // Graphics
    PY_FUNC(GRopen,                 2,  IFgrOpen);
    PY_FUNC(GRcheckError,           0,  IFgrCheckError);
    PY_FUNC(GRcreatePixmap,         3,  IFgrCreatePixmap);
    PY_FUNC(GRdestroyPixmap,        2,  IFgrDestroyPixmap);
    PY_FUNC(GRcopyDrawable,         9,  IFgrCopyDrawable);
    PY_FUNC(GRdraw,                 5,  IFgrDraw);
    PY_FUNC(GRgetDrawableSize,      3,  IFgrGetDrawableSize);
    PY_FUNC(GRresetDrawable,        2,  IFgrResetDrawable);
    PY_FUNC(GRclear,                1,  IFgrClear);
    PY_FUNC(GRpixel,                3,  IFgrPixel);
    PY_FUNC(GRpixels,               3,  IFgrPixels);
    PY_FUNC(GRline,                 5,  IFgrLine);
    PY_FUNC(GRpolyLine,             3,  IFgrPolyLine);
    PY_FUNC(GRlines,                3,  IFgrLines);
    PY_FUNC(GRbox,                  5,  IFgrBox);
    PY_FUNC(GRboxes,                3,  IFgrBoxes);
    PY_FUNC(GRarc,                  7,  IFgrArc);
    PY_FUNC(GRpolygon,              3,  IFgrPolygon);
    PY_FUNC(GRtext,                 5,  IFgrText);
    PY_FUNC(GRtextExtent,           3,  IFgrTextExtent);
    PY_FUNC(GRdefineColor,          4,  IFgrDefineColor);
    PY_FUNC(GRsetBackground,        2,  IFgrSetBackground);
    PY_FUNC(GRsetWindowBackground,  2,  IFgrSetWindowBackground);
    PY_FUNC(GRsetColor,             2,  IFgrSetColor);
    PY_FUNC(GRdefineLinestyle,      3,  IFgrDefineLinestyle);
    PY_FUNC(GRsetLinestyle,         2,  IFgrSetLinestyle);
    PY_FUNC(GRdefineFillpattern,    5,  IFgrDefineFillpattern);
    PY_FUNC(GRsetFillpattern,       2,  IFgrSetFillpattern);
    PY_FUNC(GRupdate,               1,  IFgrUpdate);
    PY_FUNC(GRsetMode,              2,  IFgrSetMode);

    // Hard Copy
    PY_FUNC(HClistDrivers,          0,  IFhcListDrivers);
    PY_FUNC(HCsetDriver,            1,  IFhcSetDriver);
    PY_FUNC(HCgetDriver,            0,  IFhcGetDriver);
    PY_FUNC(HCsetResol,             1,  IFhcSetResol);
    PY_FUNC(HCgetResol,             0,  IFhcGetResol);
    PY_FUNC(HCgetResols,            1,  IFhcGetResols);
    PY_FUNC(HCsetBestFit,           1,  IFhcSetBestFit);
    PY_FUNC(HCgetBestFit,           0,  IFhcGetBestFit);
    PY_FUNC(HCsetLegend,            1,  IFhcSetLegend);
    PY_FUNC(HCgetLegend,            0,  IFhcGetLegend);
    PY_FUNC(HCsetLandscape,         1,  IFhcSetLandscape);
    PY_FUNC(HCgetLandscape,         0,  IFhcGetLandscape);
    PY_FUNC(HCsetMetric,            1,  IFhcSetMetric);
    PY_FUNC(HCgetMetric,            0,  IFhcGetMetric);
    PY_FUNC(HCsetSize,              4,  IFhcSetSize);
    PY_FUNC(HCgetSize,              1,  IFhcGetSize);
    PY_FUNC(HCshowAxes,             1,  IFhcShowAxes);
    PY_FUNC(HCshowGrid,             2,  IFhcShowGrid);
    PY_FUNC(HCsetGridInterval,      2,  IFhcSetGridInterval);
    PY_FUNC(HCsetGridStyle,         2,  IFhcSetGridStyle);
    PY_FUNC(HCsetGridCrossSize,     2,  IFhcSetGridCrossSize);
    PY_FUNC(HCsetGridOnTop,         2,  IFhcSetGridOnTop);
    PY_FUNC(HCdump,                 6,  IFhcDump);
    PY_FUNC(HCerrorString,          0,  IFhcErrorString);
    PY_FUNC(HClistPrinters,         0,  IFhcListPrinters);
    PY_FUNC(HCmedia,                1,  IFhcMedia);

    // Keyboard
    PY_FUNC(ReadKeymap,             1,  IFreadKeymap);

    // Libraries
    PY_FUNC(OpenLibrary,            1,  IFopenLibrary);
    PY_FUNC(CloseLibrary,           1,  IFcloseLibrary);

    // Mode
    PY_FUNC(Mode,                   2,  IFmode);
    PY_FUNC(CurMode,                1,  IFcurMode);

    // Prompt Line
    PY_FUNC(StuffText,              1,  IFstuffText);
    PY_FUNC(TextCmd,                1,  IFtextCmd);
    PY_FUNC(GetLastPrompt,          0,  IFgetLastPrompt);

    // Scripts
    PY_FUNC(ListFunctions,          0,  IFlistFunctions);
    PY_FUNC(Exec,                   1,  IFexec);
    PY_FUNC(SetKey,                 1,  IFsetKey);
    PY_FUNC(HasPython,              0,  IFhasPython);
    PY_FUNC(RunPython,              1,  IFrunPython);
    PY_FUNC(RunPythonModFunc, VARARGS,  IFrunPythonModFunc);
    PY_FUNC(ResetPython,            0,  IFresetPython);
    PY_FUNC(HasTcl,                 0,  IFhasTcl);
    PY_FUNC(HasTk,                  0,  IFhasTk);
    PY_FUNC(RunTcl,           VARARGS,  IFrunTcl);
    PY_FUNC(ResetTcl,               0,  IFresetTcl);
    PY_FUNC(HasGlobalVariable,      1,  IFhasGlobalVariable);
    PY_FUNC(GetGlobalVariable,      1,  IFgetGlobalVariable);
    PY_FUNC(SetGlobalVariable,      2,  IFsetGlobalVariable);

    // Technology File
    PY_FUNC(GetTechName,            0,  IFgetTechName);
    PY_FUNC(GetTechExt,             0,  IFgetTechExt);
    PY_FUNC(SetTechExt,             1,  IFsetTechExt);
    PY_FUNC(TechParseLine,          1,  IFtechParseLine);
    PY_FUNC(TechGetFkeyString,      1,  IFtechGetFkeyString);
    PY_FUNC(TechSetFkeyString,      2,  IFtechSetFkeyString);

    // Variables
    PY_FUNC(Set,                    2,  IFset);
    PY_FUNC(Unset,                  1,  IFunset);
    PY_FUNC(PushSet,                2,  IFpushSet);
    PY_FUNC(PopSet,                 1,  IFpopSet);
    PY_FUNC(SetExpand,              2,  IFsetExpand);
    PY_FUNC(Get,                    1,  IFget);
    // JoinLimits imported from edit

    // Xic Version
    PY_FUNC(VersionString,          0,  IFversionString);


    void py_register_misc1()
    {
      // Current Cell
      cPyIf::register_func("Edit",                   pyEdit);
      cPyIf::register_func("OpenCell",               pyOpenCell);
      cPyIf::register_func("TouchCell",              pyTouchCell);
      cPyIf::register_func("Push",                   pyPush);
      cPyIf::register_func("PushElement",            pyPushElement);
      cPyIf::register_func("Pop",                    pyPop);
      cPyIf::register_func("NewCellName",            pyNewCellName);
      cPyIf::register_func("CurCellName",            pyCurCellName);
      cPyIf::register_func("TopCellName",            pyTopCellName);
      cPyIf::register_func("FileName",               pyFileName);
      cPyIf::register_func("CurCellBB",              pyCurCellBB);
      cPyIf::register_func("SetCellFlag",            pySetCellFlag);
      cPyIf::register_func("GetCellFlag",            pyGetCellFlag);
      cPyIf::register_func("Save",                   pySave);
      cPyIf::register_func("UpdateNative",           pyUpdateNative);

      // Cell Info
      cPyIf::register_func("CellBB",                 pyCellBB);
      cPyIf::register_func("ListSubcells",           pyListSubcells);
      cPyIf::register_func("ListParents",            pyListParents);
      cPyIf::register_func("InitGen",                pyInitGen);
      cPyIf::register_func("CellsHandle",            pyCellsHandle);
      cPyIf::register_func("GenCells",               pyGenCells);

      // Database
      cPyIf::register_func("Clear",                  pyClear);
      cPyIf::register_func("ClearAll",               pyClearAll);
      cPyIf::register_func("IsCellInMem",            pyIsCellInMem);
      cPyIf::register_func("IsFileInMem",            pyIsFileInMem);
      cPyIf::register_func("NumCellsInMem",          pyNumCellsInMem);
      cPyIf::register_func("ListCellsInMem",         pyListCellsInMem);
      cPyIf::register_func("ListTopCellsInMem",      pyListTopCellsInMem);
      cPyIf::register_func("ListModCellsInMem",      pyListModCellsInMem);
      cPyIf::register_func("ListTopFilesInMem",      pyListTopFilesInMem);

      // Symbol Tables
      cPyIf::register_func("SetSymbolTable",         pySetSymbolTable);
      cPyIf::register_func("ClearSymbolTable",       pyClearSymbolTable);
      cPyIf::register_func("CurSymbolTable",         pyCurSymbolTable);

      // Display
      cPyIf::register_func("Window",                 pyWindow);
      cPyIf::register_func("GetWindow",              pyGetWindow);
      cPyIf::register_func("GetWindowView",          pyGetWindowView);
      cPyIf::register_func("GetWindowMode",          pyGetWindowMode);
      cPyIf::register_func("Expand",                 pyExpand);
      cPyIf::register_func("Display",                pyDisplay);
      cPyIf::register_func("FreezeDisplay",          pyFreezeDisplay);
      cPyIf::register_func("Redraw",                 pyRedraw);

      // Exit
      cPyIf::register_func("Exit",                   pyExit);
      cPyIf::register_func("Halt",                   pyExit);  // alias

      // Annotation
      cPyIf::register_func("AddMark",                pyAddMark);
      cPyIf::register_func("EraseMark",              pyEraseMark);
      cPyIf::register_func("DumpMarks",              pyDumpMarks);
      cPyIf::register_func("ReadMarks",              pyReadMarks);

      // Ghost Rendering
      cPyIf::register_func("PushGhost",              pyPushGhost);
      cPyIf::register_func("PushGhostBox",           pyPushGhostBox);
      cPyIf::register_func("PushGhostH",             pyPushGhostH);
      cPyIf::register_func("PopGhost",               pyPopGhost);
      cPyIf::register_func("ShowGhost",              pyShowGhost);

      // Graphics
      cPyIf::register_func("GRopen",                 pyGRopen);
      cPyIf::register_func("GRcheckError",           pyGRcheckError);
      cPyIf::register_func("GRcreatePixmap",         pyGRcreatePixmap);
      cPyIf::register_func("GRdestroyPixmap",        pyGRdestroyPixmap);
      cPyIf::register_func("GRcopyDrawable",         pyGRcopyDrawable);
      cPyIf::register_func("GRdraw",                 pyGRdraw);
      cPyIf::register_func("GRgetDrawableSize",      pyGRgetDrawableSize);
      cPyIf::register_func("GRresetDrawable",        pyGRresetDrawable);
      cPyIf::register_func("GRclear",                pyGRclear);
      cPyIf::register_func("GRpixel",                pyGRpixel);
      cPyIf::register_func("GRpixels",               pyGRpixels);
      cPyIf::register_func("GRline",                 pyGRline);
      cPyIf::register_func("GRpolyLine",             pyGRpolyLine);
      cPyIf::register_func("GRlines",                pyGRlines);
      cPyIf::register_func("GRbox",                  pyGRbox);
      cPyIf::register_func("GRboxes",                pyGRboxes);
      cPyIf::register_func("GRarc",                  pyGRarc);
      cPyIf::register_func("GRpolygon",              pyGRpolygon);
      cPyIf::register_func("GRtext",                 pyGRtext);
      cPyIf::register_func("GRtextExtent",           pyGRtextExtent);
      cPyIf::register_func("GRdefineColor",          pyGRdefineColor);
      cPyIf::register_func("GRsetBackground",        pyGRsetBackground);
      cPyIf::register_func("GRsetWindowBackground",  pyGRsetWindowBackground);
      cPyIf::register_func("GRsetColor",             pyGRsetColor);
      cPyIf::register_func("GRdefineLinestyle",      pyGRdefineLinestyle);
      cPyIf::register_func("GRsetLinestyle",         pyGRsetLinestyle);
      cPyIf::register_func("GRdefineFillpattern",    pyGRdefineFillpattern);
      cPyIf::register_func("GRsetFillpattern",       pyGRsetFillpattern);
      cPyIf::register_func("GRupdate",               pyGRupdate);
      cPyIf::register_func("GRsetMode",              pyGRsetMode);

      // Hard Copy
      cPyIf::register_func("HClistDrivers",          pyHClistDrivers);
      cPyIf::register_func("HCsetDriver",            pyHCsetDriver);
      cPyIf::register_func("HCgetDriver",            pyHCgetDriver);
      cPyIf::register_func("HCsetResol",             pyHCsetResol);
      cPyIf::register_func("HCgetResol",             pyHCgetResol);
      cPyIf::register_func("HCgetResols",            pyHCgetResols);
      cPyIf::register_func("HCsetBestFit",           pyHCsetBestFit);
      cPyIf::register_func("HCgetBestFit",           pyHCgetBestFit);
      cPyIf::register_func("HCsetLegend",            pyHCsetLegend);
      cPyIf::register_func("HCgetLegend",            pyHCgetLegend);
      cPyIf::register_func("HCsetLandscape",         pyHCsetLandscape);
      cPyIf::register_func("HCgetLandscape",         pyHCgetLandscape);
      cPyIf::register_func("HCsetMetric",            pyHCsetMetric);
      cPyIf::register_func("HCgetMetric",            pyHCgetMetric);
      cPyIf::register_func("HCsetSize",              pyHCsetSize);
      cPyIf::register_func("HCgetSize",              pyHCgetSize);
      cPyIf::register_func("HCshowAxes",             pyHCshowAxes);
      cPyIf::register_func("HCshowGrid",             pyHCshowGrid);
      cPyIf::register_func("HCsetGridInterval",      pyHCsetGridInterval);
      cPyIf::register_func("HCsetGridStyle",         pyHCsetGridStyle);
      cPyIf::register_func("HCsetGridCrossSize",     pyHCsetGridCrossSize);
      cPyIf::register_func("HCsetGridOnTop",         pyHCsetGridOnTop);
      cPyIf::register_func("HCdump",                 pyHCdump);
      cPyIf::register_func("HCerrorString",          pyHCerrorString);
      cPyIf::register_func("HClistPrinters",         pyHClistPrinters);
      cPyIf::register_func("HCmedia",                pyHCmedia);

      // Keyboard
      cPyIf::register_func("ReadKeymap",             pyReadKeymap);

      // Libraries
      cPyIf::register_func("OpenLibrary",            pyOpenLibrary);
      cPyIf::register_func("CloseLibrary",           pyCloseLibrary);

      // Mode
      cPyIf::register_func("Mode",                   pyMode);
      cPyIf::register_func("CurMode",                pyCurMode);

      // Prompt Line
      cPyIf::register_func("StuffText",              pyStuffText);
      cPyIf::register_func("TextCmd",                pyTextCmd);
      cPyIf::register_func("GetLastPrompt",          pyGetLastPrompt);

      // Scripts
      cPyIf::register_func("ListFunctions",          pyListFunctions);
      cPyIf::register_func("Exec",                   pyExec);
      cPyIf::register_func("SetKey",                 pySetKey);
      cPyIf::register_func("HasPython",              pyHasPython);
      cPyIf::register_func("RunPython",              pyRunPython);
      cPyIf::register_func("RunPythonModFunc",       pyRunPythonModFunc);
      cPyIf::register_func("ResetPython",            pyResetPython);
      cPyIf::register_func("HasTcl",                 pyHasTcl);
      cPyIf::register_func("HasTk",                  pyHasTk);
      cPyIf::register_func("RunTcl",                 pyRunTcl);
      cPyIf::register_func("ResetTcl",               pyResetTcl);
      cPyIf::register_func("HasGlobalVariable",      pyHasGlobalVariable);
      cPyIf::register_func("GetGlobalVariable",      pyGetGlobalVariable);
      cPyIf::register_func("SetGlobalVariable",      pySetGlobalVariable);

      // Technology File
      cPyIf::register_func("GetTechName",            pyGetTechName);
      cPyIf::register_func("GetTechExt",             pyGetTechExt);
      cPyIf::register_func("SetTechExt",             pySetTechExt);
      cPyIf::register_func("TechParseLine",          pyTechParseLine);
      cPyIf::register_func("TechGetFkeyString",      pyTechGetFkeyString);
      cPyIf::register_func("TechSetFkeyString",      pyTechSetFkeyString);

      // Variables
      cPyIf::register_func("Set",                    pySet);
      cPyIf::register_func("Unset",                  pyUnset);
      cPyIf::register_func("PushSet",                pyPushSet);
      cPyIf::register_func("PopSet",                 pyPopSet);
      cPyIf::register_func("SetExpand",              pySetExpand);
      cPyIf::register_func("Get",                    pyGet);
      // JoinLimits imported from edit

      // Xic Version
      cPyIf::register_func("VersionString",          pyVersionString);
    }
#endif  // HAVE_PYTHON

#ifdef HAVE_TCL
    // TclTk wrappers.

    // Current Cell
    TCL_FUNC(Edit,                   2,  IFedit);
    TCL_FUNC(OpenCell,               3,  IFopenCell);
    TCL_FUNC(TouchCell,              2,  IFtouchCell);
    TCL_FUNC(Push,                   1,  IFpush);
    TCL_FUNC(PushElement,            3,  IFpushElement);
    TCL_FUNC(Pop,                    0,  IFpop);
    TCL_FUNC(NewCellName,            0,  IFnewCellName);
    TCL_FUNC(CurCellName,            0,  IFcurCellName);
    TCL_FUNC(TopCellName,            0,  IFtopCellName);
    TCL_FUNC(FileName,               0,  IFfileName);
    TCL_FUNC(CurCellBB,              1,  IFcurCellBB);
    TCL_FUNC(SetCellFlag,            3,  IFsetCellFlag);
    TCL_FUNC(GetCellFlag,            2,  IFgetCellFlag);
    TCL_FUNC(Save,                   1,  IFsave);
    TCL_FUNC(UpdateNative,           1,  IFupdateNative);

    // Cell Info
    TCL_FUNC(CellBB,           VARARGS,  IFcellBB);
    TCL_FUNC(ListSubcells,           4,  IFlistSubcells);
    TCL_FUNC(ListParents,            1,  IFlistParents);
    TCL_FUNC(InitGen,                0,  IFinitGen);
    TCL_FUNC(CellsHandle,            2,  IFcellsHandle);
    TCL_FUNC(GenCells,               1,  IFgenCells);

    // Database
    TCL_FUNC(Clear,                  1,  IFclear);
    TCL_FUNC(ClearAll,               1,  IFclearAll);
    TCL_FUNC(IsCellInMem,            1,  IFisCellInMem);
    TCL_FUNC(IsFileInMem,            1,  IFisFileInMem);
    TCL_FUNC(NumCellsInMem,          0,  IFnumCellsInMem);
    TCL_FUNC(ListCellsInMem,         1,  IFlistCellsInMem);
    TCL_FUNC(ListTopCellsInMem,      0,  IFlistTopCellsInMem);
    TCL_FUNC(ListModCellsInMem,      0,  IFlistModCellsInMem);
    TCL_FUNC(ListTopFilesInMem,      0,  IFlistTopFilesInMem);

    // Symbol Tables
    TCL_FUNC(SetSymbolTable,         1,  IFsetSymbolTable);
    TCL_FUNC(ClearSymbolTable,       1,  IFclearSymbolTable);
    TCL_FUNC(CurSymbolTable,         0,  IFcurSymbolTable);

    // Display
    TCL_FUNC(Window,                 4,  IFwindow);
    TCL_FUNC(GetWindow,              0,  IFgetWindow);
    TCL_FUNC(GetWindowView,          2,  IFgetWindowView);
    TCL_FUNC(GetWindowMode,          1,  IFgetWindowMode);
    TCL_FUNC(Expand,                 2,  IFexpand);
    TCL_FUNC(Display,                6,  IFdisplay);
    TCL_FUNC(FreezeDisplay,          1,  IFfreezeDisplay);
    TCL_FUNC(Redraw,                 1,  IFredraw);

    // Exit
    TCL_FUNC(Exit,                   0,  IFexit);

    // Annotation
    TCL_FUNC(AddMark,          VARARGS,  IFaddMark);
    TCL_FUNC(EraseMark,              1,  IFeraseMark);
    TCL_FUNC(DumpMarks,              1,  IFdumpMarks);
    TCL_FUNC(ReadMarks,              1,  IFreadMarks);

    // Ghost Rendering
    TCL_FUNC(PushGhost,              2,  IFpushGhost);
    TCL_FUNC(PushGhostBox,           4,  IFpushGhostBox);
    TCL_FUNC(PushGhostH,             2,  IFpushGhostH);
    TCL_FUNC(PopGhost,               0,  IFpopGhost);
    TCL_FUNC(ShowGhost,              1,  IFshowGhost);

    // Graphics
    TCL_FUNC(GRopen,                 2,  IFgrOpen);
    TCL_FUNC(GRcheckError,           0,  IFgrCheckError);
    TCL_FUNC(GRcreatePixmap,         3,  IFgrCreatePixmap);
    TCL_FUNC(GRdestroyPixmap,        2,  IFgrDestroyPixmap);
    TCL_FUNC(GRcopyDrawable,         9,  IFgrCopyDrawable);
    TCL_FUNC(GRdraw,                 5,  IFgrDraw);
    TCL_FUNC(GRgetDrawableSize,      3,  IFgrGetDrawableSize);
    TCL_FUNC(GRresetDrawable,        2,  IFgrResetDrawable);
    TCL_FUNC(GRclear,                1,  IFgrClear);
    TCL_FUNC(GRpixel,                3,  IFgrPixel);
    TCL_FUNC(GRpixels,               3,  IFgrPixels);
    TCL_FUNC(GRline,                 5,  IFgrLine);
    TCL_FUNC(GRpolyLine,             3,  IFgrPolyLine);
    TCL_FUNC(GRlines,                3,  IFgrLines);
    TCL_FUNC(GRbox,                  5,  IFgrBox);
    TCL_FUNC(GRboxes,                3,  IFgrBoxes);
    TCL_FUNC(GRarc,                  7,  IFgrArc);
    TCL_FUNC(GRpolygon,              3,  IFgrPolygon);
    TCL_FUNC(GRtext,                 5,  IFgrText);
    TCL_FUNC(GRtextExtent,           3,  IFgrTextExtent);
    TCL_FUNC(GRdefineColor,          4,  IFgrDefineColor);
    TCL_FUNC(GRsetBackground,        2,  IFgrSetBackground);
    TCL_FUNC(GRsetWindowBackground,  2,  IFgrSetWindowBackground);
    TCL_FUNC(GRsetColor,             2,  IFgrSetColor);
    TCL_FUNC(GRdefineLinestyle,      3,  IFgrDefineLinestyle);
    TCL_FUNC(GRsetLinestyle,         2,  IFgrSetLinestyle);
    TCL_FUNC(GRdefineFillpattern,    5,  IFgrDefineFillpattern);
    TCL_FUNC(GRsetFillpattern,       2,  IFgrSetFillpattern);
    TCL_FUNC(GRupdate,               1,  IFgrUpdate);
    TCL_FUNC(GRsetMode,              2,  IFgrSetMode);

    // Hard Copy
    TCL_FUNC(HClistDrivers,          0,  IFhcListDrivers);
    TCL_FUNC(HCsetDriver,            1,  IFhcSetDriver);
    TCL_FUNC(HCgetDriver,            0,  IFhcGetDriver);
    TCL_FUNC(HCsetResol,             1,  IFhcSetResol);
    TCL_FUNC(HCgetResol,             0,  IFhcGetResol);
    TCL_FUNC(HCgetResols,            1,  IFhcGetResols);
    TCL_FUNC(HCsetBestFit,           1,  IFhcSetBestFit);
    TCL_FUNC(HCgetBestFit,           0,  IFhcGetBestFit);
    TCL_FUNC(HCsetLegend,            1,  IFhcSetLegend);
    TCL_FUNC(HCgetLegend,            0,  IFhcGetLegend);
    TCL_FUNC(HCsetLandscape,         1,  IFhcSetLandscape);
    TCL_FUNC(HCgetLandscape,         0,  IFhcGetLandscape);
    TCL_FUNC(HCsetMetric,            1,  IFhcSetMetric);
    TCL_FUNC(HCgetMetric,            0,  IFhcGetMetric);
    TCL_FUNC(HCsetSize,              4,  IFhcSetSize);
    TCL_FUNC(HCgetSize,              1,  IFhcGetSize);
    TCL_FUNC(HCshowAxes,             1,  IFhcShowAxes);
    TCL_FUNC(HCshowGrid,             2,  IFhcShowGrid);
    TCL_FUNC(HCsetGridInterval,      2,  IFhcSetGridInterval);
    TCL_FUNC(HCsetGridStyle,         2,  IFhcSetGridStyle);
    TCL_FUNC(HCsetGridCrossSize,     2,  IFhcSetGridCrossSize);
    TCL_FUNC(HCsetGridOnTop,         2,  IFhcSetGridOnTop);
    TCL_FUNC(HCdump,                 6,  IFhcDump);
    TCL_FUNC(HCerrorString,          0,  IFhcErrorString);
    TCL_FUNC(HClistPrinters,         0,  IFhcListPrinters);
    TCL_FUNC(HCmedia,                1,  IFhcMedia);

    // Keyboard
    TCL_FUNC(ReadKeymap,             1,  IFreadKeymap);

    // Libraries
    TCL_FUNC(OpenLibrary,            1,  IFopenLibrary);
    TCL_FUNC(CloseLibrary,           1,  IFcloseLibrary);

    // Mode
    TCL_FUNC(Mode,                   2,  IFmode);
    TCL_FUNC(CurMode,                1,  IFcurMode);

    // Prompt Line
    TCL_FUNC(StuffText,              1,  IFstuffText);
    TCL_FUNC(TextCmd,                1,  IFtextCmd);
    TCL_FUNC(GetLastPrompt,          0,  IFgetLastPrompt);

    // Scripts
    TCL_FUNC(ListFunctions,          0,  IFlistFunctions);
    TCL_FUNC(Exec,                   1,  IFexec);
    TCL_FUNC(SetKey,                 1,  IFsetKey);
    TCL_FUNC(HasPython,              0,  IFhasPython);
    TCL_FUNC(RunPython,              1,  IFrunPython);
    TCL_FUNC(RunPythonModFunc, VARARGS,  IFrunPythonModFunc);
    TCL_FUNC(ResetPython,            0,  IFresetPython);
    TCL_FUNC(HasTcl,                 0,  IFhasTcl);
    TCL_FUNC(HasTk,                  0,  IFhasTk);
    TCL_FUNC(RunTcl,           VARARGS,  IFrunTcl);
    TCL_FUNC(ResetTcl,               0,  IFresetTcl);
    TCL_FUNC(HasGlobalVariable,      1,  IFhasGlobalVariable);
    TCL_FUNC(GetGlobalVariable,      1,  IFgetGlobalVariable);
    TCL_FUNC(SetGlobalVariable,      2,  IFsetGlobalVariable);

    // Technology File
    TCL_FUNC(GetTechName,            0,  IFgetTechName);
    TCL_FUNC(GetTechExt,             0,  IFgetTechExt);
    TCL_FUNC(SetTechExt,             1,  IFsetTechExt);
    TCL_FUNC(TechParseLine,          1,  IFtechParseLine);
    TCL_FUNC(TechGetFkeyString,      1,  IFtechGetFkeyString);
    TCL_FUNC(TechSetFkeyString,      2,  IFtechSetFkeyString);

    // Variables
    TCL_FUNC(Set,                    2,  IFset);
    TCL_FUNC(Unset,                  1,  IFunset);
    TCL_FUNC(PushSet,                2,  IFpushSet);
    TCL_FUNC(PopSet,                 1,  IFpopSet);
    TCL_FUNC(SetExpand,              2,  IFsetExpand);
    TCL_FUNC(Get,                    1,  IFget);
    // JoinLimits imported from edit

    // Xic Version
    TCL_FUNC(VersionString,          0,  IFversionString);


    void tcl_register_misc1()
    {
      // Current Cell
      cTclIf::register_func("Edit",                   tclEdit);
      cTclIf::register_func("OpenCell",               tclOpenCell);
      cTclIf::register_func("TouchCell",              tclTouchCell);
      cTclIf::register_func("Push",                   tclPush);
      cTclIf::register_func("PushElement",            tclPushElement);
      cTclIf::register_func("Pop",                    tclPop);
      cTclIf::register_func("NewCellName",            tclNewCellName);
      cTclIf::register_func("CurCellName",            tclCurCellName);
      cTclIf::register_func("TopCellName",            tclTopCellName);
      cTclIf::register_func("FileName",               tclFileName);
      cTclIf::register_func("CurCellBB",              tclCurCellBB);
      cTclIf::register_func("SetCellFlag",            tclSetCellFlag);
      cTclIf::register_func("GetCellFlag",            tclGetCellFlag);
      cTclIf::register_func("Save",                   tclSave);
      cTclIf::register_func("UpdateNative",           tclUpdateNative);

      // Cell Info
      cTclIf::register_func("CellBB",                 tclCellBB);
      cTclIf::register_func("ListSubcells",           tclListSubcells);
      cTclIf::register_func("ListParents",            tclListParents);
      cTclIf::register_func("InitGen",                tclInitGen);
      cTclIf::register_func("CellsHandle",            tclCellsHandle);
      cTclIf::register_func("GenCells",               tclGenCells);

      // Database
      cTclIf::register_func("Clear",                  tclClear);
      cTclIf::register_func("ClearAll",               tclClearAll);
      cTclIf::register_func("IsCellInMem",            tclIsCellInMem);
      cTclIf::register_func("IsFileInMem",            tclIsFileInMem);
      cTclIf::register_func("NumCellsInMem",          tclNumCellsInMem);
      cTclIf::register_func("ListCellsInMem",         tclListCellsInMem);
      cTclIf::register_func("ListTopCellsInMem",      tclListTopCellsInMem);
      cTclIf::register_func("ListModCellsInMem",      tclListModCellsInMem);
      cTclIf::register_func("ListTopFilesInMem",      tclListTopFilesInMem);

      // Symbol Tables
      cTclIf::register_func("SetSymbolTable",         tclSetSymbolTable);
      cTclIf::register_func("ClearSymbolTable",       tclClearSymbolTable);
      cTclIf::register_func("CurSymbolTable",         tclCurSymbolTable);

      // Display
      cTclIf::register_func("Window",                 tclWindow);
      cTclIf::register_func("GetWindow",              tclGetWindow);
      cTclIf::register_func("GetWindowView",          tclGetWindowView);
      cTclIf::register_func("GetWindowMode",          tclGetWindowMode);
      cTclIf::register_func("Expand",                 tclExpand);
      cTclIf::register_func("Display",                tclDisplay);
      cTclIf::register_func("FreezeDisplay",          tclFreezeDisplay);
      cTclIf::register_func("Redraw",                 tclRedraw);

      // Exit
      cTclIf::register_func("Exit",                   tclExit);
      cTclIf::register_func("Halt",                   tclExit);  // alias

      // Annotation
      cTclIf::register_func("AddMark",                tclAddMark);
      cTclIf::register_func("EraseMark",              tclEraseMark);
      cTclIf::register_func("DumpMarks",              tclDumpMarks);
      cTclIf::register_func("ReadMarks",              tclReadMarks);

      // Ghost Rendering
      cTclIf::register_func("PushGhost",              tclPushGhost);
      cTclIf::register_func("PushGhostBox",           tclPushGhostBox);
      cTclIf::register_func("PushGhostH",             tclPushGhostH);
      cTclIf::register_func("PopGhost",               tclPopGhost);
      cTclIf::register_func("ShowGhost",              tclShowGhost);

      // Graphics
      cTclIf::register_func("GRopen",                 tclGRopen);
      cTclIf::register_func("GRcheckError",           tclGRcheckError);
      cTclIf::register_func("GRcreatePixmap",         tclGRcreatePixmap);
      cTclIf::register_func("GRdestroyPixmap",        tclGRdestroyPixmap);
      cTclIf::register_func("GRcopyDrawable",         tclGRcopyDrawable);
      cTclIf::register_func("GRdraw",                 tclGRdraw);
      cTclIf::register_func("GRgetDrawableSize",      tclGRgetDrawableSize);
      cTclIf::register_func("GRresetDrawable",        tclGRresetDrawable);
      cTclIf::register_func("GRclear",                tclGRclear);
      cTclIf::register_func("GRpixel",                tclGRpixel);
      cTclIf::register_func("GRpixels",               tclGRpixels);
      cTclIf::register_func("GRline",                 tclGRline);
      cTclIf::register_func("GRpolyLine",             tclGRpolyLine);
      cTclIf::register_func("GRlines",                tclGRlines);
      cTclIf::register_func("GRbox",                  tclGRbox);
      cTclIf::register_func("GRboxes",                tclGRboxes);
      cTclIf::register_func("GRarc",                  tclGRarc);
      cTclIf::register_func("GRpolygon",              tclGRpolygon);
      cTclIf::register_func("GRtext",                 tclGRtext);
      cTclIf::register_func("GRtextExtent",           tclGRtextExtent);
      cTclIf::register_func("GRdefineColor",          tclGRdefineColor);
      cTclIf::register_func("GRsetBackground",        tclGRsetBackground);
      cTclIf::register_func("GRsetWindowBackground",  tclGRsetWindowBackground);
      cTclIf::register_func("GRsetColor",             tclGRsetColor);
      cTclIf::register_func("GRdefineLinestyle",      tclGRdefineLinestyle);
      cTclIf::register_func("GRsetLinestyle",         tclGRsetLinestyle);
      cTclIf::register_func("GRdefineFillpattern",    tclGRdefineFillpattern);
      cTclIf::register_func("GRsetFillpattern",       tclGRsetFillpattern);
      cTclIf::register_func("GRupdate",               tclGRupdate);
      cTclIf::register_func("GRsetMode",              tclGRsetMode);

      // Hard Copy
      cTclIf::register_func("HClistDrivers",          tclHClistDrivers);
      cTclIf::register_func("HCsetDriver",            tclHCsetDriver);
      cTclIf::register_func("HCgetDriver",            tclHCgetDriver);
      cTclIf::register_func("HCsetResol",             tclHCsetResol);
      cTclIf::register_func("HCgetResol",             tclHCgetResol);
      cTclIf::register_func("HCgetResols",            tclHCgetResols);
      cTclIf::register_func("HCsetBestFit",           tclHCsetBestFit);
      cTclIf::register_func("HCgetBestFit",           tclHCgetBestFit);
      cTclIf::register_func("HCsetLegend",            tclHCsetLegend);
      cTclIf::register_func("HCgetLegend",            tclHCgetLegend);
      cTclIf::register_func("HCsetLandscape",         tclHCsetLandscape);
      cTclIf::register_func("HCgetLandscape",         tclHCgetLandscape);
      cTclIf::register_func("HCsetMetric",            tclHCsetMetric);
      cTclIf::register_func("HCgetMetric",            tclHCgetMetric);
      cTclIf::register_func("HCsetSize",              tclHCsetSize);
      cTclIf::register_func("HCgetSize",              tclHCgetSize);
      cTclIf::register_func("HCshowAxes",             tclHCshowAxes);
      cTclIf::register_func("HCshowGrid",             tclHCshowGrid);
      cTclIf::register_func("HCsetGridInterval",      tclHCsetGridInterval);
      cTclIf::register_func("HCsetGridStyle",         tclHCsetGridStyle);
      cTclIf::register_func("HCsetGridCrossSize",     tclHCsetGridCrossSize);
      cTclIf::register_func("HCsetGridOnTop",         tclHCsetGridOnTop);
      cTclIf::register_func("HCdump",                 tclHCdump);
      cTclIf::register_func("HCerrorString",          tclHCerrorString);
      cTclIf::register_func("HClistPrinters",         tclHClistPrinters);
      cTclIf::register_func("HCmedia",                tclHCmedia);

      // Keyboard
      cTclIf::register_func("ReadKeymap",             tclReadKeymap);

      // Libraries
      cTclIf::register_func("OpenLibrary",            tclOpenLibrary);
      cTclIf::register_func("CloseLibrary",           tclCloseLibrary);

      // Mode
      cTclIf::register_func("Mode",                   tclMode);
      cTclIf::register_func("CurMode",                tclCurMode);

      // Prompt Line
      cTclIf::register_func("StuffText",              tclStuffText);
      cTclIf::register_func("TextCmd",                tclTextCmd);
      cTclIf::register_func("GetLastPrompt",          tclGetLastPrompt);

      // Scripts
      cTclIf::register_func("ListFunctions",          tclListFunctions);
      cTclIf::register_func("Exec",                   tclExec);
      cTclIf::register_func("SetKey",                 tclSetKey);
      cTclIf::register_func("HasPython",              tclHasPython);
      cTclIf::register_func("RunPython",              tclRunPython);
      cTclIf::register_func("RunPythonModFunc",       tclRunPythonModFunc);
      cTclIf::register_func("ResetPython",            tclResetPython);
      cTclIf::register_func("HasTcl",                 tclHasTcl);
      cTclIf::register_func("HasTk",                  tclHasTk);
      cTclIf::register_func("RunTcl",                 tclRunTcl);
      cTclIf::register_func("ResetTcl",               tclResetTcl);
      cTclIf::register_func("HasGlobalVariable",      tclHasGlobalVariable);
      cTclIf::register_func("GetGlobalVariable",      tclGetGlobalVariable);
      cTclIf::register_func("SetGlobalVariable",      tclSetGlobalVariable);

      // Technology File
      cTclIf::register_func("GetTechName",            tclGetTechName);
      cTclIf::register_func("GetTechExt",             tclGetTechExt);
      cTclIf::register_func("SetTechExt",             tclSetTechExt);
      cTclIf::register_func("TechParseLine",          tclTechParseLine);
      cTclIf::register_func("TechGetFkeyString",      tclTechGetFkeyString);
      cTclIf::register_func("TechSetFkeyString",      tclTechSetFkeyString);

      // Variables
      cTclIf::register_func("Set",                    tclSet);
      cTclIf::register_func("Unset",                  tclUnset);
      cTclIf::register_func("PushSet",                tclPushSet);
      cTclIf::register_func("PopSet",                 tclPopSet);
      cTclIf::register_func("SetExpand",              tclSetExpand);
      cTclIf::register_func("Get",                    tclGet);
      // JoinLimits imported from edit

      // Xic Version
      cTclIf::register_func("VersionString",          tclVersionString);
    }
#endif  // HAVE_TCL
}


// Export to load functions in this script library.
//
void
cMain::load_funcs_misc1()
{
  using namespace misc1_funcs;

  // Current Cell
  SIparse()->registerFunc("Edit",                   2,  IFedit);
  SIparse()->registerFunc("OpenCell",               3,  IFopenCell);
  SIparse()->registerFunc("TouchCell",              2,  IFtouchCell);
  SIparse()->registerFunc("Push",                   1,  IFpush);
  SIparse()->registerFunc("PushElement",            3,  IFpushElement);
  SIparse()->registerFunc("Pop",                    0,  IFpop);
  SIparse()->registerFunc("NewCellName",            0,  IFnewCellName);
  SIparse()->registerFunc("CurCellName",            0,  IFcurCellName);
  SIparse()->registerFunc("TopCellName",            0,  IFtopCellName);
  SIparse()->registerFunc("FileName",               0,  IFfileName);
  SIparse()->registerFunc("CurCellBB",              1,  IFcurCellBB);
  SIparse()->registerFunc("SetCellFlag",            3,  IFsetCellFlag);
  SIparse()->registerFunc("GetCellFlag",            2,  IFgetCellFlag);
  SIparse()->registerFunc("Save",                   1,  IFsave);
  SIparse()->registerFunc("UpdateNative",           1,  IFupdateNative);

  // Cell Info
  SIparse()->registerFunc("CellBB",           VARARGS,  IFcellBB);
  SIparse()->registerFunc("ListSubcells",           4,  IFlistSubcells);
  SIparse()->registerFunc("ListParents",            1,  IFlistParents);
  SIparse()->registerFunc("InitGen",                0,  IFinitGen);
  SIparse()->registerFunc("CellsHandle",            2,  IFcellsHandle);
  SIparse()->registerFunc("GenCells",               1,  IFgenCells);

  // Database
  SIparse()->registerFunc("Clear",                  1,  IFclear);
  SIparse()->registerFunc("ClearAll",               1,  IFclearAll);
  SIparse()->registerFunc("IsCellInMem",            1,  IFisCellInMem);
  SIparse()->registerFunc("IsFileInMem",            1,  IFisFileInMem);
  SIparse()->registerFunc("NumCellsInMem",          0,  IFnumCellsInMem);
  SIparse()->registerFunc("ListCellsInMem",         1,  IFlistCellsInMem);
  SIparse()->registerFunc("ListTopCellsInMem",      0,  IFlistTopCellsInMem);
  SIparse()->registerFunc("ListModCellsInMem",      0,  IFlistModCellsInMem);
  SIparse()->registerFunc("ListTopFilesInMem",      0,  IFlistTopFilesInMem);

  // Symbol Tables
  SIparse()->registerFunc("SetSymbolTable",         1,  IFsetSymbolTable);
  SIparse()->registerFunc("ClearSymbolTable",       1,  IFclearSymbolTable);
  SIparse()->registerFunc("CurSymbolTable",         0,  IFcurSymbolTable);

  // Display
  SIparse()->registerFunc("Window",                 4,  IFwindow);
  SIparse()->registerFunc("GetWindow",              0,  IFgetWindow);
  SIparse()->registerFunc("GetWindowView",          2,  IFgetWindowView);
  SIparse()->registerFunc("GetWindowMode",          1,  IFgetWindowMode);
  SIparse()->registerFunc("Expand",                 2,  IFexpand);
  SIparse()->registerFunc("Display",                6,  IFdisplay);
  SIparse()->registerFunc("FreezeDisplay",          1,  IFfreezeDisplay);
  SIparse()->registerFunc("Redraw",                 1,  IFredraw);

  // Exit
  SIparse()->registerFunc("Exit",                   0,  IFexit);
  SIparse()->registerFunc("Halt",                   0,  IFexit);  // alias

  // Annotation
  SIparse()->registerFunc("AddMark",          VARARGS,  IFaddMark);
  SIparse()->registerFunc("EraseMark",              1,  IFeraseMark);
  SIparse()->registerFunc("DumpMarks",              1,  IFdumpMarks);
  SIparse()->registerFunc("ReadMarks",              1,  IFreadMarks);

  // Ghost Rendering
  SIparse()->registerFunc("PushGhost",              2,  IFpushGhost);
  SIparse()->registerFunc("PushGhostBox",           4,  IFpushGhostBox);
  SIparse()->registerFunc("PushGhostH",             2,  IFpushGhostH);
  SIparse()->registerFunc("PopGhost",               0,  IFpopGhost);
  SIparse()->registerFunc("ShowGhost",              1,  IFshowGhost);

  // Graphics
  SIparse()->registerFunc("GRopen",                 2,  IFgrOpen);
  SIparse()->registerFunc("GRcheckError",           0,  IFgrCheckError);
  SIparse()->registerFunc("GRcreatePixmap",         3,  IFgrCreatePixmap);
  SIparse()->registerFunc("GRdestroyPixmap",        2,  IFgrDestroyPixmap);
  SIparse()->registerFunc("GRcopyDrawable",         9,  IFgrCopyDrawable);
  SIparse()->registerFunc("GRdraw",                 5,  IFgrDraw);
  SIparse()->registerFunc("GRgetDrawableSize",      3,  IFgrGetDrawableSize);
  SIparse()->registerFunc("GRresetDrawable",        2,  IFgrResetDrawable);
  SIparse()->registerFunc("GRclear",                1,  IFgrClear);
  SIparse()->registerFunc("GRpixel",                3,  IFgrPixel);
  SIparse()->registerFunc("GRpixels",               3,  IFgrPixels);
  SIparse()->registerFunc("GRline",                 5,  IFgrLine);
  SIparse()->registerFunc("GRpolyLine",             3,  IFgrPolyLine);
  SIparse()->registerFunc("GRlines",                3,  IFgrLines);
  SIparse()->registerFunc("GRbox",                  5,  IFgrBox);
  SIparse()->registerFunc("GRboxes",                3,  IFgrBoxes);
  SIparse()->registerFunc("GRarc",                  7,  IFgrArc);
  SIparse()->registerFunc("GRpolygon",              3,  IFgrPolygon);
  SIparse()->registerFunc("GRtext",                 5,  IFgrText);
  SIparse()->registerFunc("GRtextExtent",           3,  IFgrTextExtent);
  SIparse()->registerFunc("GRdefineColor",          4,  IFgrDefineColor);
  SIparse()->registerFunc("GRsetBackground",        2,  IFgrSetBackground);
  SIparse()->registerFunc("GRsetWindowBackground",  2,  IFgrSetWindowBackground);
  SIparse()->registerFunc("GRsetColor",             2,  IFgrSetColor);
  SIparse()->registerFunc("GRdefineLinestyle",      3,  IFgrDefineLinestyle);
  SIparse()->registerFunc("GRsetLinestyle",         2,  IFgrSetLinestyle);
  SIparse()->registerFunc("GRdefineFillpattern",    5,  IFgrDefineFillpattern);
  SIparse()->registerFunc("GRsetFillpattern",       2,  IFgrSetFillpattern);
  SIparse()->registerFunc("GRupdate",               1,  IFgrUpdate);
  SIparse()->registerFunc("GRsetMode",              2,  IFgrSetMode);

  // Hard Copy
  SIparse()->registerFunc("HClistDrivers",          0,  IFhcListDrivers);
  SIparse()->registerFunc("HCsetDriver",            1,  IFhcSetDriver);
  SIparse()->registerFunc("HCgetDriver",            0,  IFhcGetDriver);
  SIparse()->registerFunc("HCsetResol",             1,  IFhcSetResol);
  SIparse()->registerFunc("HCgetResol",             0,  IFhcGetResol);
  SIparse()->registerFunc("HCgetResols",            1,  IFhcGetResols);
  SIparse()->registerFunc("HCsetBestFit",           1,  IFhcSetBestFit);
  SIparse()->registerFunc("HCgetBestFit",           0,  IFhcGetBestFit);
  SIparse()->registerFunc("HCsetLegend",            1,  IFhcSetLegend);
  SIparse()->registerFunc("HCgetLegend",            0,  IFhcGetLegend);
  SIparse()->registerFunc("HCsetLandscape",         1,  IFhcSetLandscape);
  SIparse()->registerFunc("HCgetLandscape",         0,  IFhcGetLandscape);
  SIparse()->registerFunc("HCsetMetric",            1,  IFhcSetMetric);
  SIparse()->registerFunc("HCgetMetric",            0,  IFhcGetMetric);
  SIparse()->registerFunc("HCsetSize",              4,  IFhcSetSize);
  SIparse()->registerFunc("HCgetSize",              1,  IFhcGetSize);
  SIparse()->registerFunc("HCshowAxes",             1,  IFhcShowAxes);
  SIparse()->registerFunc("HCshowGrid",             2,  IFhcShowGrid);
  SIparse()->registerFunc("HCsetGridInterval",      2,  IFhcSetGridInterval);
  SIparse()->registerFunc("HCsetGridStyle",         2,  IFhcSetGridStyle);
  SIparse()->registerFunc("HCsetGridCrossSize",     2,  IFhcSetGridCrossSize);
  SIparse()->registerFunc("HCsetGridOnTop",         2,  IFhcSetGridOnTop);
  SIparse()->registerFunc("HCdump",                 6,  IFhcDump);
  SIparse()->registerFunc("HCerrorString",          0,  IFhcErrorString);
  SIparse()->registerFunc("HClistPrinters",         0,  IFhcListPrinters);
  SIparse()->registerFunc("HCmedia",                1,  IFhcMedia);

  // Keyboard
  SIparse()->registerFunc("ReadKeymap",             1,  IFreadKeymap);

  // Libraries
  SIparse()->registerFunc("OpenLibrary",            1,  IFopenLibrary);
  SIparse()->registerFunc("CloseLibrary",           1,  IFcloseLibrary);

  // Mode
  SIparse()->registerFunc("Mode",                   2,  IFmode);
  SIparse()->registerFunc("CurMode",                1,  IFcurMode);

  // Prompt Line
  SIparse()->registerFunc("StuffText",              1,  IFstuffText);
  SIparse()->registerFunc("TextCmd",                1,  IFtextCmd);
  SIparse()->registerFunc("GetLastPrompt",          0,  IFgetLastPrompt);

  // Scripts
  SIparse()->registerFunc("ListFunctions",          0,  IFlistFunctions);
  SIparse()->registerFunc("Exec",                   1,  IFexec);
  SIparse()->registerFunc("SetKey",                 1,  IFsetKey);
  SIparse()->registerFunc("HasPython",              0,  IFhasPython);
  SIparse()->registerFunc("RunPython",              1,  IFrunPython);
  SIparse()->registerFunc("RunPythonModFunc", VARARGS,  IFrunPythonModFunc);
  SIparse()->registerFunc("ResetPython",            0,  IFresetPython);
  SIparse()->registerFunc("HasTcl",                 0,  IFhasTcl);
  SIparse()->registerFunc("HasTk",                  0,  IFhasTk);
  SIparse()->registerFunc("RunTcl",           VARARGS,  IFrunTcl);
  SIparse()->registerFunc("ResetTcl",               0,  IFresetTcl);
  SIparse()->registerFunc("HasGlobalVariable",      1,  IFhasGlobalVariable);
  SIparse()->registerFunc("GetGlobalVariable",      1,  IFgetGlobalVariable);
  SIparse()->registerFunc("SetGlobalVariable",      2,  IFsetGlobalVariable);

  // Technology File
  SIparse()->registerFunc("GetTechName",            0,  IFgetTechName);
  SIparse()->registerFunc("GetTechExt",             0,  IFgetTechExt);
  SIparse()->registerFunc("SetTechExt",             1,  IFsetTechExt);
  SIparse()->registerFunc("TechParseLine",          1,  IFtechParseLine);
  SIparse()->registerFunc("TechGetFkeyString",      1,  IFtechGetFkeyString);
  SIparse()->registerFunc("TechSetFkeyString",      2,  IFtechSetFkeyString);

  // Variables
  SIparse()->registerFunc("Set",                    2,  IFset);
  SIparse()->registerFunc("Unset",                  1,  IFunset);
  SIparse()->registerFunc("PushSet",                2,  IFpushSet);
  SIparse()->registerFunc("PopSet",                 1,  IFpopSet);
  SIparse()->registerFunc("SetExpand",              2,  IFsetExpand);
  SIparse()->registerFunc("Get",                    1,  IFget);
  // JoinLimits imported from edit

  // Xic Version
  SIparse()->registerFunc("VersionString",          0,  IFversionString);

#ifdef HAVE_PYTHON
  py_register_misc1();
#endif
#ifdef HAVE_TCL
  tcl_register_misc1();
#endif
}


//-------------------------------------------------------------------------
// Current Cell
//-------------------------------------------------------------------------

// (int) Edit(filename, cellname)
//
// This command will read in the named file or cell and make it, or
// the top level cell in the hierarchy, the current cell.  If the
// present cell has been modified, in graphics mode the user is
// prompted for whether to save the cell before reading the new one.
// The filename argument can be null or empty, in which case the user
// will be prompted for a file or cell to open for editing, if in
// graphics mode.  If not in graphics mode, an empty cell is created
// in memory and made the current cell.
//
// The name provided can be an archive file, the name of an Xic cell,
// a library file, or the "database name" of a Cell Hierarchy Digest.
// If a CHD name or the name of an archive file is given, the name of
// the cell to open can be provided as cellname.  If cellname is null or
// empty, the CHD's default name, or the top level cell (the one not
// used as a subcell by any other cells in the file) is the one opened
// for editing.  If there is more than one top level cell, the user is
// presented with a pop-up choice menu and asked to make a selection,
// if in graphics mode.  If not in graphics mode, the current cell
// won't be set, but all cells are in memory.  If the file is a
// library file, the cellname can be given, and it should be one of the
// reference names from the library, or the name of a cell defined in
// the library.  If cellname is passed 0, a pop-up listing the library
// contents will appear if in graphics mode, allowing the user to
// select a reference or cell.  If not in graphics mode, the current
// cell is unchanged, and nothing is read.
//
// See this table for the features that apply during a call to this
// function.  This function is consistent with the Edit menu command
// in that cell name aliasing, layer filtering and modification, and
// scaling are not available (unlike in the pre-3.0.0 version of this
// function).  If these features are needed, the OpenCell function
// should be used instead.
//
// The return value is one of the following integers, representing
// the command status:
//  -2      The function call was reentered.  This is not likely to
//          happen in scripts.
//  -1      The user aborted the operation.
//   0      The open failed: bad file name, parse error, etc.
//   1      The operation succeeded.
//   2      The read was successful on an archive with multiple top-
//          level cells but the cells to edit can't be determined.
//          The current cell has not been set, but the cells are in
//          memory.  The second argument could have been used to
//          resolve the ambiguity.
//   3      The cell name was the name of the device.lib or model.lib
//          file, which has been opened for text editing (in graphic
//          mode only).
//
bool
misc1_funcs::IFedit(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    const char *cellname;
    ARG_CHK(arg_string(args, 1, &cellname))

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    res->type = TYP_SCALAR;

    FIOreadPrms prms;
    res->content.value = XM()->EditCell(name, true, &prms, cellname);
    return (OK);
}


// (int) OpenCell(filename, cellname, curcell)
//
// This function will read a file into memory, similar to the Edit
// function.  The first two arguments are the same as would be passed
// to Edit.  The third argument is a boolean value.
//
// See this table for the features that apply during a call to this
// function.
//
// If curcell is nonzero, then this function will behave like the Edit
// function in switching the current cell to a newly-read cell.  The
// only difference from Edit is that scaling, layer filtering and
// aliasing, and cell name modification are allowed, as in the
// pre-3.0.0 versions of the Edit function.  The return values are
// those listed for the Edit function.
//
// If curcell is zero, the new cell will not be the current cell.
// Once in memory, the cell is available by its simple name, for use
// by the Place function for example.  If name is the name of an
// archive or library file, cellname is the cell or reference to open,
// similar to the Edit function.  In this mode, the return value is 1
// on success, 0 otherwise.
//
bool
misc1_funcs::IFopenCell(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    const char *cellname;
    ARG_CHK(arg_string(args, 1, &cellname))
    bool curcell;
    ARG_CHK(arg_boolean(args, 2, &curcell))

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    res->type = TYP_SCALAR;

    FIOreadPrms prms;
    prms.set_scale(FIO()->ReadScale());
    prms.set_alias_mask(CVAL_AUTO_NAME | CVAL_CASE | CVAL_FILE | CVAL_PFSF);
    prms.set_allow_layer_mapping(true);
    if (curcell)
        res->content.value = XM()->EditCell(name, true, &prms, cellname);
    else
        res->content.value = FIO()->OpenImport(name, &prms, cellname);

    return (OK);
}


// (int) TouchCell(cellname, curcell)
//
// If no cell exists in the current symbol table for the current mode
// with the given name, create an empty cell for cellname and add it
// to the symbol table.  If the boolean curcell is true, switch the
// current cell to cellname.  This can be much faster than Edit or
// OpenCell for cells already in memory.  The return value is -1 on
// error, 0 if no new cell was created, or 1 if a new cell was
// created.
//
bool
misc1_funcs::IFtouchCell(Variable *res, Variable *args, void*)
{
    const char *cname;
    ARG_CHK(arg_string(args, 0, &cname))
    bool curcell;
    ARG_CHK(arg_boolean(args, 1, &curcell))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (!cname) {
        if (curcell) {
            Errs()->add_error("TouchCell: null cell name given");
            return (BAD);
        }
        return (OK);
    }
    while (isspace(*cname))
        cname++;
    if (!*cname) {
        if (curcell) {
            Errs()->add_error("TouchCell: empty cell name given");
            return (BAD);
        }
        return (OK);
    }

    OItype ot = XM()->TouchCell(cname, curcell);
    if (ot == OInew)
        res->content.value = 1;
    else if (ot == OIerror)
        res->content.value = -1;
    return (OK);
}


// (int) Push(object_handle)
//
// This function will push the editing context to the cell of the
// instance referenced by the handle.  The handle is the return value
// from the SelectHandle() or AreaHandle() functions.  This is similar
// to the Push command in Xic.  The editing context can be restored
// with the Pop() function.  If the instance is an array, the 0,0
// element will be pushed (see PushElement).
//
// If successful, 1 is returned, otherwise 0 is returned.  This
// function will fail if the handle passed is not a handle to an
// object list.
//
// This function implicitly calls Commit before the context change.
//
bool
misc1_funcs::IFpush(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol && ol->odesc->type() == CDINSTANCE) {
            PP()->PushContext(OCALL(ol->odesc), 0, 0);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) PushElement(object_handle, xind, yind)
//
// This is very similar to Push, but allows passing indices which
// select the instance element to push if the instance is arrayed. 
// The indices are always effectively 0 in the Push function.  An out
// of range index value will cause the function to return 0 and not
// push the context.  If both index values are zero, the function is
// identical to Push.  The selection of the array element only affects
// the graphical display.
//
// This function implicitly calls Commit before the context change.
//
bool
misc1_funcs::IFpushElement(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int indx;
    ARG_CHK(arg_int(args, 1, &indx))
    int indy;
    ARG_CHK(arg_int(args, 2, &indy))

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        if (ol && ol->odesc->type() == CDINSTANCE) {
            CDap ap((CDc*)ol->odesc);
            if (indx < 0 || indx >= (int)ap.nx ||
                    indy < 0 || indy >= (int)ap.ny)
                Errs()->add_error("PushElement: index out of range.");
            else {
                PP()->PushContext(OCALL(ol->odesc), indx, indy);
                res->content.value = 1;
            }
        }
    }
    return (OK);
}


// (int) Pop()
//
// This function will pop the editing context to the parent cell, to
// be used after the Push() function or a Push command in Xic.  The
// Pop() function always returns 1, and has no effect if there was no
// corresponding push.
//
bool
misc1_funcs::IFpop(Variable *res, Variable*, void*)
{
    // implicit "Commit"
    EditIf()->ulCommitChanges();

    res->type = TYP_SCALAR;
    res->content.value = 1;
    PP()->PopContext();
    return (OK);
}


// (string) NewCellName()
//
// This function returns a string which is a valid cell name that does
// not conflict with any cell in the current symbol table.  The cell
// is not actually created.  This can be used with the Edit function
// to open a new cell for editing, similar to the New button in the
// File Menu.  This function never fails.
//
bool
misc1_funcs::IFnewCellName(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string = XM()->NewCellName();
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) CurCellBB(array)
//
// This function will return the bounding box of the current cell, in
// microns, in the array, as l, b, r, t.  The array must have size 4
// or larger.  The function returns 1 on success, 0 if there is no
// current cell.
//
// In electrical mode, the bounding box returned will be for the
// schematiic or symbolic representation, matching how the cell is
// displayed in the main window.  See the CellBB function for
// an alternative.
//
bool
misc1_funcs::IFcurCellBB(Variable *res, Variable *args, void*)
{
    double *vals;
    ARG_CHK(arg_array(args, 0, &vals, 4))

    CDs *cursd = CurCell();
    res->type = TYP_SCALAR;
    res->content.value = cursd ? 1 : 0;
    if (cursd) {
        cursd->computeBB();
        const BBox *BB = cursd->BB();
        vals[0] = MICRONS(BB->left);
        vals[1] = MICRONS(BB->bottom);
        vals[2] = MICRONS(BB->right);
        vals[3] = MICRONS(BB->top);
    }
    return (OK);
}


// (string) CurCellName()
//
// The return value of this function is a string containing the name
// of the current editing cell.
//
bool
misc1_funcs::IFcurCellName(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string = lstring::copy(Tstring(DSP()->CurCellName()));
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (string) TopCellName()
//
// The return value of this function is a string containing the name
// of the top level cell in the hierarchy being edited.  This is
// different from the current cell name while in a subedit.
//
bool
misc1_funcs::IFtopCellName(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string = lstring::copy(Tstring(DSP()->TopCellName()));
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (string) FileName()
//
// This function returns the name of the file from which the current
// cell was read.  If there is no such file, a null string is
// returned.
//
bool
misc1_funcs::IFfileName(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string = 0;
    if (DSP()->CurCellName()) {
        CDcbin cbin(DSP()->CurCellName());
        if (cbin.fileType() == Fnative) {
            if (cbin.fileName()) {
                char *s = new char[strlen(cbin.fileName()) +
                    strlen(Tstring(cbin.cellname())) + 2];
                sprintf(s, "%s/%s", cbin.fileName(), Tstring(cbin.cellname()));
                res->content.string = s;
            }
        }
        else if (FIO()->IsSupportedArchiveFormat(cbin.fileType()))
            res->content.string = lstring::copy(cbin.fileName());
    }
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) SetCellFlag(cellname, flagname, set)
//
// This will set a flag in the cell whose name is passed as the first
// argument.  If this argument is 0, or a null or empty string, the
// current cell is understood.  The second argument is a string giving
// the flag name.  This must be the name of a user-modifiable flag. 
// The third argument is a boolean indicating the new flag state, a
// nonzero value will set the flag, zero will unset it.  The return
// value is the previous flag status (0 or 1), or -1 on error.  On
// error, a message can be obtained from GetError.
//
// Warning:  This affects the user flags directly, and does not update
// the property used to hold flag status that is is written to disk
// when the cell is saved.  These flags should be set by setting the
// Flags property (property number 7105) with AddProperty or
// AddCellProperty, if the values need to persist when the cell is
// written to disk and reread.
//
bool
misc1_funcs::IFsetCellFlag(Variable *res, Variable *args, void*)
{
    const char *cname;
    ARG_CHK(arg_string(args, 0, &cname))
    const char *fgname;
    ARG_CHK(arg_string(args, 1, &fgname))
    bool set;
    ARG_CHK(arg_boolean(args, 2, &set))

    if (!fgname || !*fgname) {
        Errs()->add_error("SetCellFlag: null or empty flagname.");
        return (BAD);
    }

    res->type = TYP_SCALAR;
    res->content.value = -1;

    unsigned int flg = 0;
    for (FlagDef *f = SdescFlags; f->name; f++) {
        if (lstring::cieq(fgname, f->name)) {
            if (f->user_settable) {
                flg = f->value;
                break;
            }
            Errs()->add_error("SetCellFlag: flag %s read-only.", fgname);
            return (OK);
        }
    }
    if (!flg) {
        Errs()->add_error("SetCellFlag: unknown flag %s.", fgname);
        return (OK);
    }

    CDs *sdesc = 0;
    if (!cname || !*cname) {
        sdesc = CurCell(true);
        if (!sdesc) {
            Errs()->add_error("SetCellFlag: no current cell!");
            return (OK);
        }
    }
    else {
        sdesc = CDcdb()->findCell(cname, DSP()->CurMode());
        if (!sdesc) {
            Errs()->add_error("SetCellFlag: cell %s not found.", cname);
            return (OK);
        }
    }
    res->content.value = (sdesc->getFlags() & flg) ? 1 : 0;
    if (set)
        sdesc->setFlags(sdesc->getFlags() | flg);
    else
        sdesc->setFlags(sdesc->getFlags() & ~flg);
    return (OK);
}


// (int) GetCellFlag(cellname, flagname)
//
// This will query a flag in the cell whose name is passed as the
// first argument.  If this argument is 0, or a null or empty string,
// the current cell is understood.  The second argument is a string
// giving the flag name, which can be any or the flag names.  The
// return value is the flag status (0 or 1), or -1 on error.  On
// error, a message can be obtained from GetError.
//
bool
misc1_funcs::IFgetCellFlag(Variable *res, Variable *args, void*)
{
    const char *cname;
    ARG_CHK(arg_string(args, 0, &cname))
    const char *fgname;
    ARG_CHK(arg_string(args, 1, &fgname))

    if (!fgname || !*fgname) {
        Errs()->add_error("GetCellFlag: null or empty flagname.");
        return (BAD);
    }

    res->type = TYP_SCALAR;
    res->content.value = -1;

    unsigned int flg = 0;
    for (FlagDef *f = SdescFlags; f->name; f++) {
        if (lstring::cieq(fgname, f->name)) {
            flg = f->value;
            break;
        }
    }
    if (!flg) {
        Errs()->add_error("GetCellFlag: unknown flag %s.", fgname);
        return (OK);
    }

    CDs *sdesc = 0;
    if (!cname || !*cname) {
        sdesc = CurCell(true);
        if (!sdesc) {
            Errs()->add_error("GetCellFlag: no current cell!");
            return (OK);
        }
    }
    else {
        sdesc = CDcdb()->findCell(cname, DSP()->CurMode());
        if (!sdesc) {
            Errs()->add_error("GetCellFlag: cell %s not found.", cname);
            return (OK);
        }
    }
    res->content.value = (sdesc->getFlags() & flg) ? 1 : 0;
    return (OK);
}


// (int) Save(newname)
//
// This command will save to disk file the current cell, and its
// descendents if the cell originated from an archive file.  If the
// argument is NULL or the empty string, the current structure name is
// used, suffixed with one of the following if saving as an archive:
//    CGX     .cgx
//    CIF     .cif
//    GDSII   .gds
//    OASIS   .oas
// The default format will be the format of the original input file,
// though format conversion can be imposed by adding one of these
// suffixes or .xic to the newname.  The cell is saved
// unconditionally; there is no user prompt.
//
// See this table for the features that apply during a call to this
// function.
//
// This function returns 1 on success, 0 otherwise.  On error, a message
// is likely available from GetError.
//
bool
misc1_funcs::IFsave(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))

    res->type = TYP_SCALAR;

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    // Ok if name is null or empty, will prompt in interactive mode, or
    // create a name from the top cell otherwise.
    res->content.value = XM()->SaveCellAs(name, true);
    return (OK);
}


// (int) UpdateNative(dir)
//
// This will write to disk all of the modified cells in the current
// hierarchy as native cell files in the directory given as the
// argument.  If the argument is null or empty, cells will be written
// in the current directory.  The return value is the number of cells
// written.
//
// Note that only modified or internally created cells will be
// written.  To write all cells as native cell files, use the ToXIC
// function.
//
bool
misc1_funcs::IFupdateNative(Variable *res, Variable *args, void*)
{
    static char defdir[] = ".";
    const char *dir;
    ARG_CHK(arg_string(args, 0, &dir))
    if (!dir || !*dir)
        dir = defdir;

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    char buf[256];
    int ccnt = 0;
    if (DSP()->CurCellName()) {
        CDcbin cbin(DSP()->CurCellName());
        if (cbin.isModified()) {
            sprintf(buf, "%s/%s", dir, Tstring(cbin.cellname()));
            if (!FIO()->WriteNative(&cbin, buf))
                return (BAD);
            ccnt++;
        }

        CDgenHierDn_cbin sgen(&cbin);
        bool err;
        CDcbin tcbin;
        while (sgen.next(&tcbin, &err)) {
            if (tcbin.isModified()) {
                sprintf(buf, "%s/%s", dir, Tstring(tcbin.cellname()));
                if (!FIO()->WriteNative(&tcbin, buf))
                    return (BAD);
                ccnt++;
            }
        }
        if (err)
            return (BAD);
    }
    res->type = TYP_SCALAR;
    res->content.value = ccnt;
    return (OK);
}


//-------------------------------------------------------------------------
// Cell Info
//-------------------------------------------------------------------------

// (int) CellBB(cellname, array [,symbolic])
//
// This function will return the bounding box of the named cell in
// the current mode, in microns, in the array, as l, b, r, t.  If
// cellname is null or empty, the current cell is used.  The array must
// have size 4 or larger.  The function returns 1 on success, 0 if
// the cell is not found in memory.
//
// The optional boolean third argument applies to electrical cells. 
// If not given or set to false, the schematic bounding box is always
// returned.  If this argument is true, and the cell has a symbolic
// representation, the symbolic representation bounding box is
// returned, or the function fails and returns 0 if the cell has no
// symbolic representation.
//
bool
misc1_funcs::IFcellBB(Variable *res, Variable *args, void*)
{
    const char *cellname;
    ARG_CHK(arg_string(args, 0, &cellname))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 4))
    bool symbolic = false;
    if (args[2].type == TYP_SCALAR) {
        ARG_CHK(arg_boolean(args, 2, &symbolic))
    }

    CDs *sd = 0;
    if (!cellname || !*cellname)
        sd = CurCell();
    else {
        sd = CDcdb()->findCell(cellname, DSP()->CurMode());
        if (sd && sd->cellname() == DSP()->CurCellName())
            sd = CurCell();
    }
    if (sd) {
        if (sd->isElectrical()) {
            if (symbolic) {
                if (!sd->isSymbolic())
                    sd = sd->symbolicRep(0);
            }
            else if (sd->isSymbolic())
                sd = sd->owner();
        }
    }
    res->type = TYP_SCALAR;
    res->content.value = sd ? 1 : 0;
    if (sd) {
        sd->computeBB();
        const BBox *BB = sd->BB();
        vals[0] = MICRONS(BB->left);
        vals[1] = MICRONS(BB->bottom);
        vals[2] = MICRONS(BB->right);
        vals[3] = MICRONS(BB->top);
    }
    return (OK);
}


// (stringlist handle) ListSubcells(cellname, depth, array, incl_top)
//
// This function returns a handle to a sorted list of subcell names
// found under the named cell, to the given depth, and only if
// instantiated so as to overlap a rectangular area (if given).  These
// apply to the current mode, electrical or physical.  If cellname is
// null or empty, the current cell is used.  The depth is the search
// depth, which can be an integer which sets the maximum depth to
// search (0 means search cellname only and return its subcell names,
// 1 means search cellname plus its subcells, etc., and a negative
// integer sets the depth to search the entire hierarchy).  This
// argument can also be a string starting with 'a' such as "a" or
// "all" which indicates to search the entire hierarchy.
//
// The cell will be read into memory if not already there.  The
// function fails if the cell can not be found.
//
// The array argument can be passed 0, which indicates no area
// testing.  Otherwise, the array should be size four or larger, with
// the values being the left (array[0]), bottom, right, and top
// coordinates of a rectangular region of cellname.  Only cells that
// are instantiated such that the instance bounding box, when
// reflected to top-level coordinates, intersects the region will be
// listed.
//
// If the boolean incl_top is nonzero, the top cell name (cellname)
// will be included in the list, unless an array is given and there is
// no overlap with the top cell.
//
// The return is a handle to a list of cell names, and can be empty. 
// The GenCells or ListNext functions can be used to iterate through
// the list.
//
bool
misc1_funcs::IFlistSubcells(Variable *res, Variable *args, void*)
{
    const char *cellname;
    ARG_CHK(arg_string(args, 0, &cellname))
    int depth;
    ARG_CHK(arg_depth(args, 1, &depth))
    double *vals;
    ARG_CHK(arg_array(args, 2, &vals, 4))
    bool incl_top;
    ARG_CHK(arg_boolean(args, 3, &incl_top))

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    CDs *sdesc;
    if (cellname && *cellname) {
        CDcbin cbin;
        if (OIfailed(CD()->OpenExisting(cellname, &cbin)))
            return (BAD);
        sdesc = cbin.celldesc(DSP()->CurMode());
    }
    else {
        sdesc = CurCell(true);
        if (!sdesc)
            return (BAD);
    }

    const BBox *pBB = 0;
    BBox BB;
    if (vals) {
        BB.left = INTERNAL_UNITS(vals[0]);
        BB.bottom = INTERNAL_UNITS(vals[1]);
        BB.right = INTERNAL_UNITS(vals[2]);
        BB.top = INTERNAL_UNITS(vals[3]);
        BB.fix();
        pBB = &BB;
    }

    stringlist *l0 = sdesc->listSubcells(depth, incl_top, false, pBB);
    sHdl *hdl = new sHdlString(l0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (stringlist handle) ListParents(cellname)
//
// This function returns a list of cell names, each of which contain
// an instance of the cell name passed as the argument.  These apply
// to the current mode, electrical or physical.  If cellname is null or
// empty, the current cell is used.
//
// function fails if the cell can not be found in memory.
//
// The return is a handle to a list of cell names, and can be empty. 
// The GenCells or ListNext functions can be used to iterate through
// the list.
//
bool
misc1_funcs::IFlistParents(Variable *res, Variable *args, void*)
{
    const char *cellname;
    ARG_CHK(arg_string(args, 0, &cellname))

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    CDs *sdesc;
    if (cellname && *cellname)
        sdesc = CDcdb()->findCell(cellname, DSP()->CurMode());
    else
        sdesc = CurCell(true);
    if (!sdesc)
        return (BAD);

    stringnumlist *l0 = 0;
    sdesc->listParents(&l0, false);

    stringlist *s0 = 0;
    for (stringnumlist *s = l0; s; s = s->next) {
        s0 = new stringlist(s->string, s0);
        s->string = 0;
    }
    stringnumlist::destroy(l0);
    stringlist::sort(s0);

    sHdl *hdl = new sHdlString(s0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


namespace {
    // Recursively create a list of subcells, use a symbol table to
    // test for uniqueness.
    //
    stringlist *
    setgen(SymTab *t, CDs *sdesc)
    {
        if (!sdesc)
            return (0);
        stringlist *l = 0, *l0 = 0;
        CDm_gen mgen(sdesc, GEN_MASTERS);
        for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
            stringlist *lx = setgen(t, m->celldesc());
            if (lx) {
                if (!l)
                    l = l0 = lx;
                else
                    l->next = lx;
                while (l->next)
                    l = l->next;
            }
        }
        if (t->add(Tstring(sdesc->cellname()), 0, true)) {
            stringlist *lx = new stringlist(
                lstring::copy(Tstring(sdesc->cellname())), 0);
            if (!l)
                l = l0 = lx;
            else
                l->next = lx;
        }
        return (l0);
    }
}


// (stringlist_handle) InitGen()
//
// This function returns a handle to a list of names of cells used in
// the hierarchy of the current cell, either the physical or
// electrical part according to the current mode.  Each cell is listed
// once only, and all cells are listed, including the current cell
// which is returned last.
//
// The return is a handle to a list of cell names, and can be empty. 
// The GenCells or ListNext functions can be used to iterate through
// the list.
//
bool
misc1_funcs::IFinitGen(Variable *res, Variable*, void*)
{
    // implicit "Commit"
    EditIf()->ulCommitChanges();

    SymTab *t = new SymTab(false, false);
    stringlist *l0 = setgen(t, CurCell(true));
    delete t;
    sHdl *hdl = new sHdlString(l0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


namespace {
    stringlist *
    setgen(CDs *sdesc, stringlist *l0, int depth)
    {
        if (sdesc && depth >= 0) {
            CDm_gen mgen(sdesc, GEN_MASTERS);
            for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
                l0 = new stringlist(
                    lstring::copy(Tstring(m->cellname())), l0);
                l0 = setgen(m->celldesc(), l0, depth - 1);
            }
        }
        return (l0);
    }
}


// (stringlist_handle) CellsHandle(cellname, depth)
//
// This function returns a handle to a list of subcell names found in
// cellname, to the given hierarchy depth.  If cellname is null or empty, the
// current cell is used.  The depth is the search depth, which can be
// an integer which sets the maximum depth to search (0 means search
// cellname only and return its subcell names, 1 means search cellname
// plus its subcells, etc., and a negative integer sets the depth to
// search the entire hierarchy).  This argument can also be a string
// starting with 'a' such as "a" or "all" which indicates to search
// the entire hierarchy.  The listing order is as a tree, with a
// subcell listed followed by the descent into that subcell.
//
// The cell will be read into memory if not already there.  The
// function fails if the cell can not be found.
//
// With "all", passed, the output is similar to that of the InitGen()
// function, except that the top-level cell name is not listed, and
// duplicate entries are not removed (ListUnique() can be called to
// remove duplicate names).
//
// Be aware that the listing will generally contain lots of duplicate
// names.  This function is not recommended for general hierarchy
// traversal.
//
// The return is a handle to a list of cell names, and can be empty. 
// The GenCells or ListNext functions can be used to iterate through
// the list.
//
bool
misc1_funcs::IFcellsHandle(Variable *res, Variable *args, void*)
{
    const char *cellname;
    ARG_CHK(arg_string(args, 0, &cellname))
    int depth;
    ARG_CHK(arg_depth(args, 1, &depth))

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    CDs *sdesc;
    if (cellname && *cellname) {
        CDcbin cbin;
        if (OIfailed(CD()->OpenExisting(cellname, &cbin)))
            return (BAD);
        sdesc = cbin.celldesc(DSP()->CurMode());
    }
    else {
        sdesc = CurCell(true);
        if (!sdesc)
            return (BAD);
    }

    stringlist *l0 = setgen(sdesc, 0, depth);
    sHdl *hdl = new sHdlString(l0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (string) GenCells(stringlist_handle)
//
// This function is identical to ListNext but is provided as a
// companion to other functions in this module.  It will return the
// string at the front of the list referenced by the handle, and set
// the handle to reference the next string in the list.  The function
// will fail if the handle is not a reference to a list of strings.  A
// null string is returned if the handle is not found, or after all
// strings in the list have been returned.
//
bool
misc1_funcs::IFgenCells(Variable *res, Variable *args, void*)
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


//-------------------------------------------------------------------------
// Database
//-------------------------------------------------------------------------

//  Clear(cellname)
//
// If cellname is not empty, any matching cell and all its descendents
// are cleared from the database, unless they are referenced by
// another cell not being cleared.  If cellname is null or empty, the
// entire database is cleared.  This function is obviously very
// dangerous.
//
bool
misc1_funcs::IFclear(Variable*, Variable *args, void*)
{
    const char *cellname;
    ARG_CHK(arg_string(args, 0, &cellname))

    XM()->Clear(cellname);
    return (OK);
}


//  ClearAll(clear_tech)
//
// This will clear all cells from the present symbol table, clear and
// delete any other symbol tables that may be defined, and revert the
// layer database.  If the boolean argument is nonzero, layers read
// from the technology file will be cleared, otherwise the layer
// database is reverted to the state just after the technology file
// was read.  This function does NOT automatically open a new cell. 
// This is for server mode, to give the system a good scrubbing
// between jobs.
//
bool
misc1_funcs::IFclearAll(Variable*, Variable *args, void*)
{
    bool clear_tech;
    ARG_CHK(arg_boolean(args, 0, &clear_tech))
    XM()->ClearAll(clear_tech);
    return (OK);
}


// (int) IsCellInMem(cellname)
//
// This function returns 1 if the string cellname is the name of a
// cell in the current symbol table, 0 otherwise.  If the string
// contains a path prefix, it will be ignored, and the last (filename)
// component used for the test.
//
bool
misc1_funcs::IFisCellInMem(Variable *res, Variable *args, void*)
{
    const char *cellname;
    ARG_CHK(arg_string(args, 0, &cellname))

    cellname = lstring::strip_path(cellname);
    res->type = TYP_SCALAR;
    res->content.value = CDcdb()->findSymbol(cellname) ? 1.0 : 0.0;
    return (OK);
}


// (int) IsFileInMem(filename)
//
// This will compare the string filename to the source file names
// saved with cells in the current symbol table.  If
// filename is a full path, the function returns 1 if an exact match
// is found.  If filename is not rooted, the function returns 1 if the
// last path component matches.  In either case, 0 is returned if no
// match is seen.
//
bool
misc1_funcs::IFisFileInMem(Variable *res, Variable *args, void*)
{
    const char *fname;
    ARG_CHK(arg_string(args, 0, &fname))
    const char *fsp = lstring::strip_path(fname);

    res->type = TYP_SCALAR;
    res->content.value = 0.0;

    CDgenTab_cbin sgen;
    CDcbin cbin;
    while (sgen.next(&cbin)) {
        if (cbin.isLibrary() && cbin.isDevice())
            // don't list library devices
            continue;
        if (!cbin.fileName())
            continue;
        if (cbin.fileType() == Fnative) {
            if (!strcmp(fsp, Tstring(cbin.cellname()))) {
                if (lstring::is_rooted(fname) && fsp > fname+1) {
                    if (!strncmp(fname, cbin.fileName(), fsp - fname - 1)) {
                        res->content.value = 1.0;
                        break;
                    }
                }
                else {
                    res->content.value = 1.0;
                    break;
                }
            }
        }
        else {
            if (cbin.isSubcell())
                continue;
            if (!FIO()->IsSupportedArchiveFormat(cbin.fileType()))
                continue;
            if (!strcmp(fsp, lstring::strip_path(cbin.fileName()))) {
                if (lstring::is_rooted(fname)) {
                    if (!strcmp(fname, cbin.fileName())) {
                        res->content.value = 1.0;
                        break;
                    }
                }
                else {
                    res->content.value = 1.0;
                    break;
                }
            }
        }
    }
    return (OK);
}


// (int) NumCellsInMem()
//
// This function returns an integer giving the number of cells in the
// current symbol table.
//
bool
misc1_funcs::IFnumCellsInMem(Variable *res, Variable*, void*)
{
    int cnt = 0;
    CDgenTab_cbin sgen;
    CDcbin cbin;
    while (sgen.next(&cbin)) {
        if (cbin.isLibrary() && cbin.isDevice())
            // don't count library devices
            continue;
        cnt++;
    }
    res->type = TYP_SCALAR;
    res->content.value = cnt;
    return (OK);
}


// (stringlist_handle) ListCellsInMem(options_str)
//
// This function returns a handle to a list of strings, sorted
// alphabetically, giving the names of cells found in the current
// symbol table.
//
// A fairly extensive filtering capability is available, which is
// configured through a string passed as the argument.  If 0 is
// passed, or the options string is null or empty, all cells listed.
//
// The string consists of a space-separated list of keywords, each of
// which represents a condition for filtering.  The cells listed will
// be the logical AND of all option clauses.  The keywords are
// described with the Cell List Filter panel.
//
bool
misc1_funcs::IFlistCellsInMem(Variable *res, Variable *args, void*)
{
    const char *opts;
    ARG_CHK(arg_string(args, 0, &opts))

    res->type = TYP_SCALAR;
    res->content.value = 0;

    // Parse errors are ignored, this always returns a valid struct.
    cfilter_t *cf = cfilter_t::parse(opts, DSP()->CurMode(), 0);

    stringlist *s0 = 0;
    CDgenTab_cbin sgen;
    CDcbin cbin;
    while (sgen.next(&cbin)) {
        if (cf) {
            if (!cf->inlist(&cbin))
                continue;
        }
        else {
            if (cbin.isLibrary() && cbin.isDevice())
                // don't list library devices
                continue;
        }
        s0 = new stringlist(lstring::copy(Tstring(cbin.cellname())), s0);
    }
    if (s0) {
        stringlist::sort(s0);
        sHdl *hdl = new sHdlString(s0);
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    delete cf;
    return (OK);
}


// (stringlist_handle) ListTopCellsInMem()
//
// This function returns a handle to a list of strings, sorted
// alphabetically, giving the names of top-level cells in the current
// symbol table.  These are the cells that are not used as subcells,
// in either physical or electrical mode.
//
bool
misc1_funcs::IFlistTopCellsInMem(Variable *res, Variable*, void*)
{
    stringlist *s0 = 0;
    CDgenTab_cbin sgen;
    CDcbin cbin;
    while (sgen.next(&cbin)) {
        if (cbin.isLibrary() && cbin.isDevice())
            // don't list library devices
            continue;
        if (cbin.isSubcell())
            continue;
        s0 = new stringlist(lstring::copy(Tstring(cbin.cellname())), s0);
    }
    if (s0) {
        stringlist::sort(s0);
        sHdl *hdl = new sHdlString(s0);
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0;
    }
    return (OK);
}


// (stringlist_handle) ListModCellsInMem()
//
// This function returns a handle to a list of strings, sorted
// alphabetically, giving the names of modified cells in the current
// symbol table.  A cell is modified if the contents have changed
// since the cell was read or last written to disk.
//
bool
misc1_funcs::IFlistModCellsInMem(Variable *res, Variable*, void*)
{
    stringlist *s0 = 0;
    CDgenTab_cbin sgen;
    CDcbin cbin;
    while (sgen.next(&cbin)) {
        if (cbin.isLibrary() && cbin.isDevice())
            // don't list library devices
            continue;
        if (!cbin.isModified())
            continue;
        s0 = new stringlist(lstring::copy(Tstring(cbin.cellname())), s0);
    }
    if (s0) {
        stringlist::sort(s0);
        sHdl *hdl = new sHdlString(s0);
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0;
    }
    return (OK);
}


// (stringlist_handle) ListTopFilesInMem()
//
// This function returns a handle to a list of strings, alphabetically
// sorted, giving the source file names of the top-level cells in the
// current symbol table.
//
bool
misc1_funcs::IFlistTopFilesInMem(Variable *res, Variable*, void*)
{
    stringlist *s0 = 0;
    CDgenTab_cbin sgen;
    CDcbin cbin;
    while (sgen.next(&cbin)) {
        if (cbin.isLibrary() && cbin.isDevice())
            // don't list library devices
            continue;
        if (cbin.isSubcell())
            continue;
        if (!cbin.fileName())
            continue;
        if (cbin.fileType() == Fnative) {
            char *s = new char[strlen(cbin.fileName()) +
                strlen(Tstring(cbin.cellname())) + 2];
            sprintf(s, "%s/%s", cbin.fileName(), Tstring(cbin.cellname()));
            s0 = new stringlist(s, s0);
        }
        else if (FIO()->IsSupportedArchiveFormat(cbin.fileType())) {
            // Only list cells that were originally top level.
            if (cbin.isArchiveTopLevel())
                s0 = new stringlist(lstring::copy(cbin.fileName()), s0);
        }
    }
    if (s0) {
        stringlist::sort(s0);
        sHdl *hdl = new sHdlString(s0);
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0;
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Symbol Tables
//-------------------------------------------------------------------------

// (string) SetSymbolTable(tabname)
//
// This function will set the current symbol table to the table named
// in the argument string.  If the string is null or empty, the
// default "main" table is understood.  If a table by the given name
// does not exist, a new table will be created for that name.
//
// The return value is a string giving the name of the active table
// before the switch.
//
bool
misc1_funcs::IFsetSymbolTable(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))

    res->type = TYP_STRING;
    res->content.string = lstring::copy(CDcdb()->tableName());
    res->flags |= VF_ORIGINAL;
    XM()->SetSymbolTable(name);
    return (OK);
}


// ClearSymbolTable(destroy)
//
// This function will clear or destroy the current symbol table.  If
// the boolean argument is nonzero, and the current table is not the
// "main" table, the current table and its contents will be
// destroyed.  Otherwise, the current table will be cleared, i.e., all
// cells will be destroyed.  If the current symbol table is destroyed,
// a new current table will be installed from among the internal list
// of existing tables.
//
// This function always returns 1.
//
bool
misc1_funcs::IFclearSymbolTable(Variable *res, Variable *args, void*)
{
    bool destr;
    ARG_CHK(arg_boolean(args, 0, &destr))

    if (destr)
        XM()->ClearSymbolTable();
    else
        XM()->Clear(0);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (string) CurSymbolTable()
//
// This function returns a string giving the name of the current
// symbol table.
//
bool
misc1_funcs::IFcurSymbolTable(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string = lstring::copy(CDcdb()->tableName());
    res->flags |= VF_ORIGINAL;
    return (OK);
}


//-------------------------------------------------------------------------
// Display
//-------------------------------------------------------------------------

// (int) Window(x, y, width, win)
//
// The window view is changed so that it is centered at x, y and has
// width set by the third argument.  If the width argument is less
// than or equal to zero, a centered, full view of the current cell is
// obtained.  In this case, the x, y arguments are ignored.  The
// win is an integer 0-4 which specifies the window:
//    0    Main drawing window
//    1-4  Subwindow (number as shown in title bar)
// The function returns 1 on success, 0 if the indicated window does
// not exist.
//
bool
misc1_funcs::IFwindow(Variable *res, Variable *args, void*)
{
    int wnum;
    ARG_CHK(arg_int(args, 3, &wnum))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wdesc = getwin(wnum);
    if (!wdesc)
        return (OK);

    int x;
    ARG_CHK(arg_coord(args, 0, &x, wdesc->Mode()))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, wdesc->Mode()))
    int width;
    ARG_CHK(arg_coord(args, 2, &width, wdesc->Mode()))

    if (width <= 0)
        wdesc->CenterFullView();
    else
        wdesc->InitWindow(x, y, width);
    wdesc->Redisplay(0);
    res->content.value = 1;
    return (OK);
}


// (int) GetWindow()
//
// This function returns the window number of the drawing window that
// contains the pointer.  The window number is an integer 0-4:
//    0    Main drawing window
//    1-4  Subwindow (number as shown in title bar)
// If the pointer is not in a drawing window, 0 is returned.
//
bool
misc1_funcs::IFgetWindow(Variable *res, Variable*, void*)
{
    WindowDesc *wd = EV()->CurrentWin();
    if (!wd)
        wd = DSP()->MainWdesc();
    res->type = TYP_SCALAR;
    res->content.value = 0;
    for (int i = 0; i < DSP_NUMWINS; i++) {
        if (DSP()->Window(i) == wd) {
            res->content.value = i;
            break;
        }
    }
    return (OK);
}


// (int) GetWindowView(win, array)
//
// This function returns the view area (visible cell coordinates) of
// the given window win, which is an integer 0-4 where 0 is the main
// window and 1-4 represent subwindows.  The view coordinates, in
// microns, are returned in the array, in order L, B, R, T.  On
// success, 1 is returned, otherwise 0 is returned and the array is
// untouched.
//
bool
misc1_funcs::IFgetWindowView(Variable *res, Variable *args, void*)
{
    int wnum;
    ARG_CHK(arg_int(args, 0, &wnum))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 4))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    WindowDesc *wdesc = getwin(wnum);
    if (wdesc) {
        vals[0] = MICRONS(wdesc->Window()->left);
        vals[1] = MICRONS(wdesc->Window()->bottom);
        vals[2] = MICRONS(wdesc->Window()->right);
        vals[3] = MICRONS(wdesc->Window()->top);
        res->content.value = 1;
    }
    return (OK);
}


// (int) GetWindowMode(win)
//
// This function returns the display mode of the given window win,
// which is 0 for physical mode, 1 for electrical, or -1 if the window
// does not exist.  The argument is an integer 0-4, where 0 represents
// the main window and 1-4 indicate subwindows.  The return for window
// 0 (the main window) is the same as the return from CurMode().
//
bool
misc1_funcs::IFgetWindowMode(Variable *res, Variable *args, void*)
{
    int wnum;
    ARG_CHK(arg_int(args, 0, &wnum))

    res->type = TYP_SCALAR;
    res->content.value = -1;
    WindowDesc *wdesc = getwin(wnum);
    if (wdesc)
        res->content.value = wdesc->Mode();
    return (OK);
}


// (int) Expand(winnum, string)
//
// This sets the expansion mode for the display in the window
// specified in winnum.  The winnum argument is an integer 0-4, where
// 0 refers to the main window, and 1-4 correspond to the subwindows
// brought up with the Viewport command.  The string contains
// characters which modify the display mode, as would be given to the
// expnd command in the menus.
//    integer set expand level
//    n       set level to 0
//    a       expand all
//    +       increment expand level
//    -       decrement expand level
//
bool
misc1_funcs::IFexpand(Variable *res, Variable *args, void*)
{
    int winnum;
    ARG_CHK(arg_int(args, 0, &winnum))
    const char *str;
    ARG_CHK(arg_string(args, 1, &str))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (str) {
        if (winnum < 0 || winnum > 4)
            winnum = 0;
        WindowDesc *wdesc = 0;
        if (winnum == 0)
            wdesc = DSP()->MainWdesc();
        else if (DSP()->Window(winnum))
            wdesc = DSP()->Window(winnum);
        if (wdesc) {
            if (wdesc->Expand(str)) {
                DSP()->MainWdesc()->Redisplay(0);
                res->content.value = 1;
            }
        }
    }
    return (OK);
}


// (int) Display(display_name, window, l, b, r, t)
//
// This function will render the current cell in a foreign X window.
// The X window id is passed as an integer in the second argument.
// The first argument is the X display string corresponding to the
// server in which the window is cached.  The remaining arguments set
// the area to be displayed, in microns.  The function returns 1 upon
// success, 0 otherwise.  This function is useful for rendering a
// layout if interactive graphics is not enabled, such as in server
// mode.  This function will not work under Microsoft Windows.
//
// The GRopen() interface functions provide a hugely more flexible
// interface for exporting graphics.
//
bool
misc1_funcs::IFdisplay(Variable *res, Variable *args, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
#ifdef WIN32
    (void)args;
#else
    const char *dname;
    ARG_CHK(arg_string(args, 0, &dname))
    unsigned win;
    ARG_CHK(arg_unsigned(args, 1, &win))
    int l;
    ARG_CHK(arg_coord(args, 2, &l, DSP()->CurMode()))
    int b;
    ARG_CHK(arg_coord(args, 3, &b, DSP()->CurMode()))
    int r;
    ARG_CHK(arg_coord(args, 4, &r, DSP()->CurMode()))
    int t;
    ARG_CHK(arg_coord(args, 5, &t, DSP()->CurMode()))

    if (!dname || !*dname)
        return (BAD);
    Xdraw *xd = new Xdraw(dname, win);
    if (xd && !xd->check_error())
        res->content.value = xd->draw(l, b, r, t);
    delete xd;
#endif
    return (OK);
}


// (int) FreezeDisplay(freeze)
//
// When this function is called with a nonzero argument, the graphical
// display in the drawing windows will be frozen until a subsequent
// call of this function with a zero argument, or the script
// terminates.  This is useful for speeding execution, and eliminating
// distracting screen drawing while a script is running.  When the
// function is called with a zero argument, all drawing windows are
// refreshed.
//
bool
misc1_funcs::IFfreezeDisplay(Variable *res, Variable *args, void*)
{
    int freeze;
    ARG_CHK(arg_int(args, 0, &freeze))

    res->type = TYP_SCALAR;
    res->content.value = DSP()->NoRedisplay();
    SIlcx()->setFrozen(freeze);
    return (OK);
}


// (int) Redraw(win)
//
// This function will redraw the window indicated by the argument,
// which is 0 for the main window or 1-4 for the sub-windows.  The
// function returns 0 if the argument does not correspond to an
// existing window, 1 otherwise.
//
bool
misc1_funcs::IFredraw(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))

    WindowDesc *wd = getwin(win);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (wd) {
        wd->Redisplay(0);
        res->content.value = 1;
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Exit
//-------------------------------------------------------------------------

// Exit()
//
// Calling this function terminates execution of the script.
//
bool
misc1_funcs::IFexit(Variable*, Variable*, void*)
{
    SI()->Halt();
    return (OK);
}


//-------------------------------------------------------------------------
// Annotation
//-------------------------------------------------------------------------

// AddMark(type, arguments...)
//
// This function will add a "user mark" to a display list, which is
// rendered as highlighting in the current cell.  These can be used
// for illustrative purposes.  The marks are not included in the
// design database, but are persistent to the current cell and are
// remembered as long as the current cell exists in memory.  Any call
// can have associated marks, whether electrical or physical.  Marks
// are shown in any window displaying the cell as the top level. 
// Marks are not shown in expanded subcells.
//
// The arguments that follow the type argument vary depending
// upon the type.  The type argument can be an integer code, or a
// string whose first character signifies the type.  The return value,
// if nonzero, is a unique mark id, which can be passed to EraseMark
// to erase the mark.  A zero return indicates that an error occurred.
//
// The table below describes the marks available.  All coordinates and
// dimensions are in microns, in the coordinate system of the current
// cell.  Each mark takes an optional attribute argument, which is an
// integer whose set bits indicate a display property.  These bits are
//
// bit 0:  Draw with a textured (dashed) line if set, otherwise use a
//         solid line.
// bit 1:  Cause the mark to blink, using the selection colors.
// bit 2:  Render the mark in an alternate color (bit 1 is ignored).
//
// Type: 1 or "l"
// Arguments: x1, y1, x2, y2 [, attribute]
// Draw a line segment form x1,y1 to x2,y2.
//
// Type: 2 or "b"
// Arguments: l, b, r, t [, attribute]
// Draw an open box, l,b is lower-left corner and r,t is upper-right
// corner.
//
// Type: 3 or "u"
// Arguments: xl, xr, yb [, yt, attribute]
// Draw an open triangle.  The two base vertices are xl,yb and xr,yb.
// The third vertex is (xl+xr)/2,yt.  If yt is not given, it is set to
// make the triangle equilateral.
//
// Type: 4 or "t"
// Arguments: yl, yu, xb [, xt, attribute]
// Draw an open triangle.  The two base vertices are xb,yl and xb,yu.
// The third vertex is xt,(yl+yu)/2.  If xt is not given, it is set to
// make the triangle equilateral.
//
// Type: 5 or "c"
// Arguments: xc, yc, rad [, attribute]
// Draw a circle of radius rad centered at xc,yc.
//
// Type: 6 or "e"
// Arguments: xc, yc, rx, ry [, attribute]
// Draw an ellipse centered at xc,yc using radii rx and ry.
//
// Type: 7 or "p"
// Arguments: numverts, xy_array [, attribute]
// Draw an open polygon or path.  The number of vertices is given
// first, followed by an array of size 2*numverts or larger that
// contains the vertex coordinates as x-y pairs.  For a polygon, The
// vertex list should be closed, i.e., the first and last vertices
// listed (and counted) should be the same.
//
// Type: 8 or "s"
// Arguments: string, x, y [, width, height, xform, attribute]
// Draw a text string.  The string is followed by the coordinates of
// the reference point, which for default justification is the
// lower-left corner of the bounding box.  The width, height, and
// xform arguments are analogous to those of the Label script
// function, providing the rendering size and justification and
// transformation information.  Unlike the Label function, the settings
// of the Justify and UseTransform functions are ignored,
// transformation and justification must be set through the xform
// argument.
//
bool
misc1_funcs::IFaddMark(Variable *res, Variable *args, void*)
{
    int type = 0;
    if (args[0].type == TYP_STRING || args[0].type == TYP_NOTYPE) {
        if (!args[0].content.string)
            return (BAD);
        type = *args[0].content.string;
        if (isupper(type))
            type = tolower(type);
    }
    else if (args[0].type == TYP_SCALAR)
        type = (int)args[0].content.value;
    else
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (type == hlLine || type == 'l') {
        int x1;
        ARG_CHK(arg_coord(args, 1, &x1, DSP()->CurMode()))
        int y1;
        ARG_CHK(arg_coord(args, 2, &y1, DSP()->CurMode()))
        int x2;
        ARG_CHK(arg_coord(args, 3, &x2, DSP()->CurMode()))
        int y2;
        ARG_CHK(arg_coord(args, 4, &y2, DSP()->CurMode()))
        int at = 0;
        if (args[5].type != TYP_ENDARG) {
            ARG_CHK(arg_int(args, 5, &at))
        }
        res->content.value = DSP()->AddUserMark(hlLine, x1, y1, x2, y2, at);
    }
    else if (type == hlBox || type == 'b') {
        int l;
        ARG_CHK(arg_coord(args, 1, &l, DSP()->CurMode()))
        int b;
        ARG_CHK(arg_coord(args, 2, &b, DSP()->CurMode()))
        int r;
        ARG_CHK(arg_coord(args, 3, &r, DSP()->CurMode()))
        int t;
        ARG_CHK(arg_coord(args, 4, &t, DSP()->CurMode()))
        int at = 0;
        if (args[5].type != TYP_ENDARG) {
            ARG_CHK(arg_int(args, 5, &at))
        }
        if (l > r) SwapInts(l, r);
        if (b > t) SwapInts(b, t);
        res->content.value = DSP()->AddUserMark(hlBox, l, b, r, t, at);
    }
    else if (type == hlVtriang || type == 'u') {
        int xl;
        ARG_CHK(arg_coord(args, 1, &xl, DSP()->CurMode()))
        int xr;
        ARG_CHK(arg_coord(args, 2, &xr, DSP()->CurMode()))
        int yb;
        ARG_CHK(arg_coord(args, 3, &yb, DSP()->CurMode()))
        int yt = yb + (int)(abs(xr - xl)*0.5*sqrt(3.0));
        if (args[4].type != TYP_ENDARG) {
            ARG_CHK(arg_coord(args, 4, &yt, DSP()->CurMode()))
        }
        int at = 0;
        if (args[5].type != TYP_ENDARG) {
            ARG_CHK(arg_int(args, 5, &at))
        }
        res->content.value = DSP()->AddUserMark(hlVtriang, xl, xr, yb, yt, at);
    }
    else if (type == hlHtriang || type == 't') {
        int yl;
        ARG_CHK(arg_coord(args, 1, &yl, DSP()->CurMode()))
        int yu;
        ARG_CHK(arg_coord(args, 2, &yu, DSP()->CurMode()))
        int xb;
        ARG_CHK(arg_coord(args, 3, &xb, DSP()->CurMode()))
        int xt = xb + (int)(abs(yu - yl)*0.5*sqrt(3.0));
        if (args[4].type != TYP_ENDARG) {
            ARG_CHK(arg_coord(args, 4, &xt, DSP()->CurMode()))
        }
        int at = 0;
        if (args[5].type != TYP_ENDARG) {
            ARG_CHK(arg_int(args, 5, &at))
        }
        res->content.value = DSP()->AddUserMark(hlHtriang, xb, xt, yl, yu, at);
    }
    else if (type == hlCircle || type == 'c') {
        int xc;
        ARG_CHK(arg_coord(args, 1, &xc, DSP()->CurMode()))
        int yc;
        ARG_CHK(arg_coord(args, 2, &yc, DSP()->CurMode()))
        int rad;
        ARG_CHK(arg_coord(args, 3, &rad, DSP()->CurMode()))
        int at = 0;
        if (args[4].type != TYP_ENDARG) {
            ARG_CHK(arg_int(args, 4, &at))
        }
        res->content.value = DSP()->AddUserMark(hlCircle, xc, yc, rad, at);
    }
    else if (type == hlEllipse || type == 'e') {
        int xc;
        ARG_CHK(arg_coord(args, 1, &xc, DSP()->CurMode()))
        int yc;
        ARG_CHK(arg_coord(args, 2, &yc, DSP()->CurMode()))
        int rx;
        ARG_CHK(arg_coord(args, 3, &rx, DSP()->CurMode()))
        int ry;
        ARG_CHK(arg_coord(args, 4, &ry, DSP()->CurMode()))
        int at = 0;
        if (args[5].type != TYP_ENDARG) {
            ARG_CHK(arg_int(args, 5, &at))
        }
        res->content.value = DSP()->AddUserMark(hlEllipse, xc, yc, rx, ry, at);
    }
    else if (type == hlPoly || type == 'p') {
        int num;
        ARG_CHK(arg_int(args, 1, &num))
        double *vals;
        ARG_CHK(arg_array(args, 2, &vals, num*2))
        int at = 0;
        if (args[3].type != TYP_ENDARG) {
            ARG_CHK(arg_int(args, 3, &at))
        }
        Point *pts = new Point[num];
        for (int i = 0; i < num; i++)
            pts[i].set(INTERNAL_UNITS(vals[2*i]), INTERNAL_UNITS(vals[2*i+1]));
        res->content.value = DSP()->AddUserMark(hlPoly, pts, num, at);
        delete [] pts;
    }
    else if (type == hlText || type == 's') {
        const char *str;
        ARG_CHK(arg_string(args, 1, &str))
        int x;
        ARG_CHK(arg_coord(args, 2, &x, DSP()->CurMode()))
        int y;
        ARG_CHK(arg_coord(args, 3, &y, DSP()->CurMode()))
        int width = 0;
        if (args[4].type != TYP_ENDARG) {
            ARG_CHK(arg_coord(args, 4, &width, DSP()->CurMode()))
        }
        int height = 0;
        if (args[5].type != TYP_ENDARG) {
            ARG_CHK(arg_coord(args, 5, &height, DSP()->CurMode()))
        }
        int xform = 0;
        if (args[6].type != TYP_ENDARG) {
            ARG_CHK(arg_int(args, 6, &xform))
        }
        int at = 0;
        if (args[7].type != TYP_ENDARG) {
            ARG_CHK(arg_int(args, 7, &at))
        }
        if (width <= 0 || height <= 0) {
            int w, h;
            DSP()->DefaultLabelSize(str, DSP()->CurMode(), &w, &h);
            if (width > 0)
                height = (int)((width*h)/w);
            else if (height > 0)
                width = (int)((height*w)/h);
            else {
                width = w;
                height = h;
            }
        }
        res->content.value =
            DSP()->AddUserMark(hlText, str, x, y, width, height, xform, at);
    }
    else
        return (BAD);
    return (OK);
}


// (int) EraseMark(id)
//
// Remove a mark from the "user marks" display list.  The argument is
// the id number returned from AddMark.  If zero is passed instead,
// all marks will be erased.  The return value is 1 if any marks were
// erased.
//
bool
misc1_funcs::IFeraseMark(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_int(args, 0, &id))
    res->type = TYP_SCALAR;
    res->content.value = DSP()->RemoveUserMark(id);
    return (OK);
}


// (int) DumpMarks(filename)
//
// This function will save the marks currently defined in the current
// cell to a file.  If the argument is null or empty (or scalar 0), a
// file name will be composed:  cellname.mode.marks, where mode is
// "phys" or "elec".  The return is the number of marks written, or -1
// if error.  On error, a message may be available from GetError.  If
// 0, no file was produced, as no marks were found.

bool
misc1_funcs::IFdumpMarks(Variable *res, Variable *args, void*)
{
    const char *fname;
    ARG_CHK(arg_string(args, 0, &fname))

    res->type = TYP_SCALAR;
    res->content.value = 0;

    CDs *sd = CurCell(true);
    if (!sd)
        return (OK);
    char buf[256];
    if (!fname || !*fname) {
        sprintf(buf, "%s.%s.marks", Tstring(sd->cellname()),
            sd->isElectrical() ? "elec" : "phys");
        fname = buf;
    }
    res->content.value = DSP()->DumpUserMarks(fname, sd);
    return (OK);
}


// (int) ReadMarks(filename)
//
// This function will read the marks found in a file into the current
// cell.  The file must be in the format produced by DumpMarks, and
// apply to the same name and display mode as the current cell.  A
// null or empty or 0 argument will imply a cell name composed as
// described for DumpMarks.  The return value is the number of marks
// read, or -1 if error.  On error, a message may be available from
// GetEreror.
//
bool
misc1_funcs::IFreadMarks(Variable *res, Variable *args, void*)
{
    const char *fname;
    ARG_CHK(arg_string(args, 0, &fname))

    res->type = TYP_SCALAR;
    res->content.value = 0;

    CDs *sd = CurCell(true);
    if (!sd)
        return (OK);
    char buf[256];
    if (!fname || !*fname) {
        sprintf(buf, "%s.%s.marks", Tstring(sd->cellname()),
            sd->isElectrical() ? "elec" : "phys");
        fname = buf;
    }
    res->content.value = DSP()->ReadUserMarks(fname);
    return (OK);
}


//-------------------------------------------------------------------------
// Ghost Rendering
//-------------------------------------------------------------------------

// (int) PushGhost(array, numpts)
//
// This function allows a polygon to be added to the list of polygons
// used for dynamic highlighting with the ShowGhost() function.  The
// outline of the polygon will be "attached" to the mouse pointer.
// The return value is the number of polygons in the list, after the
// present one is added.  The array is an array of x-y values forming
// the polygon.  The numpts value is the number of x-y pairs that
// constitute the polygon.  If this value is less than 2 or greater
// than the real size of the array, the real size of the array will be
// assumed.  The second argument is useful when the polygon data do
// not entirely fill the array, and can be set to 0 otherwise.
//
bool
misc1_funcs::IFpushGhost(Variable *res, Variable *args, void*)
{
    double *vals;
    ARG_CHK(arg_array(args, 0, &vals, 4))
    int asize;
    ARG_CHK(arg_int(args, 1, &asize))

    res->type = TYP_SCALAR;
    Poly po;
    if (asize >= 2 && asize <= args[0].content.a->length()/2)
        po.numpts = asize;
    else
        po.numpts = args[0].content.a->length()/2;
    po.points = new Point[po.numpts];
    for (int i = 0; i < po.numpts; i++)
        po.points[i].set(INTERNAL_UNITS(vals[2*i]),
            INTERNAL_UNITS(vals[2*i+1]));
    SIlcx()->setGhostList(new PolyList(po, SIlcx()->ghostList()));
    int cnt = 0;
    for (PolyList *p = SIlcx()->ghostList(); p; p = p->next)
        cnt++;
    res->content.value = cnt;
    return (OK);
}


// (int) PushGhostBox(left, bottom, right, top)
//
// This function is similar to PushGhost().  It allows a box outline
// to be added to the list of polygons used for ghosting with the
// ShowGhost() function.  The outline of the box will be "attached" to
// the mouse pointer.  The return value is the number of polygons in
// the list, after the present one is added.  The arguments are the
// coordinates of the lower left and upper right corners of the box,
// where "0" is the point attached to the mouse pointer.  The
// PopGhost() function is used to remove the most recently added
// object from the list.
//
bool
misc1_funcs::IFpushGhostBox(Variable *res, Variable *args, void*)
{
    BBox BB;
    ARG_CHK(arg_coord(args, 0, &BB.left, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 1, &BB.bottom, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 2, &BB.right, DSP()->CurMode()))
    ARG_CHK(arg_coord(args, 3, &BB.top, DSP()->CurMode()))

    res->type = TYP_SCALAR;
    Poly po;
    po.numpts = 5;
    po.points = new Point[5];
    BB.fix();
    BB.to_path(po.points);
    SIlcx()->setGhostList(new PolyList(po, SIlcx()->ghostList()));
    int cnt = 0;
    for (PolyList *p = SIlcx()->ghostList(); p; p = p->next)
        cnt++;
    res->content.value = cnt;
    return (OK);
}


// (int) PushGhostH(object_handle, all)
//
// Push the outline of the figure referenced by the handle onto the
// ghost list.  If boolean all is true, push all objects in the list
// represented by the handle, otherwise push the single object at the
// head of the list.  The return value is an integer count of the
// number of outlines added to the ghost list.
//
bool
misc1_funcs::IFpushGhostH(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    bool all;
    ARG_CHK(arg_boolean(args, 1, &all))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLobject)
            return (BAD);
        CDol *ol = (CDol*)hdl->data;
        int cnt = 0;
        while (ol) {
            CDo *od = ol->odesc;
            if (od->type() == CDPOLYGON) {
                Poly po;
                po.numpts = ((CDpo*)od)->numpts();
                po.points = Point::dup(((CDpo*)od)->points(), po.numpts);
                SIlcx()->setGhostList(new PolyList(po, SIlcx()->ghostList()));
                cnt++;
            }
            else if (od->type() == CDWIRE) {
                Poly po;
                if (!((CDw*)od)->w_toPoly(&po.points, &po.numpts))
                    continue;
                SIlcx()->setGhostList(new PolyList(po, SIlcx()->ghostList()));
                cnt++;
            }
            else {
                // box, label, subcell
                Poly po;
                po.numpts = 5;
                po.points = new Point[5];
                od->oBB().to_path(po.points);
                SIlcx()->setGhostList(new PolyList(po, SIlcx()->ghostList()));
                cnt++;
            }

            ol = ol->next;
            if (!all)
                break;
        }
        res->content.value = cnt;
    }
    return (OK);
}


// (int) PopGhost()
//
// This function removes the last ghosting polygon passed to
// PushGhost() from the internal list, and returns the number of
// polygons remaining in the list.
//
bool
misc1_funcs::IFpopGhost(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    if (SIlcx()->ghostList()) {
        res->content.value = 1.0;
        PolyList *p = SIlcx()->ghostList();
        SIlcx()->setGhostList(SIlcx()->ghostList()->next);
        delete p;
    }
    else
        res->content.value = 0.0;
    return (OK);
}


// (int) ShowGhost(type)
//
// Show dynamic highlighting.  This function turns on/off the
// ghosting, i.e., the display of certain features which are
// "attached" to the mouse pointer.  The argument is one of the
// numeric codes from the table below.
//
//    0  Turn off ghosting
//    1  full-screen horiz line, snapped to grid
//    2  full-screen vert line, snapped
//    3  full-screen horiz line, not snapped
//    4  full-screen vert line, not snapped
//    5  vector from last point location to pointer
//    6  box, snapped
//    7  box, not snapped
//    8  display polygon list from PushGhost()
//    9  vector from last point location to pointer
//    10 vector from last point location to pointer
//    11 vector from last point location to pointer
//
bool
misc1_funcs::IFshowGhost(Variable *res, Variable *args, void*)
{
    int num;
    ARG_CHK(arg_int(args, 0, &num))

    GFtype g = GFnone;
    switch (num) {
    default:
    case 0:
        SIlcx()->setCurGhost(0);
        EV()->SetConstrained(false);
        break;
    case 1:
        SIlcx()->setCurGhost(1);
        g = GFline;
        Gst()->SetGhostLineVert(false);
        break;
    case 2:
        SIlcx()->setCurGhost(2);
        g = GFline;
        Gst()->SetGhostLineVert(true);
        break;
    case 3:
        SIlcx()->setCurGhost(3);
        g = GFline_ns;
        Gst()->SetGhostLineVert(false);
        break;
    case 4:
        SIlcx()->setCurGhost(4);
        g = GFline_ns;
        Gst()->SetGhostLineVert(true);
        break;
    case 5:
        SIlcx()->setCurGhost(5);
        g = GFvector;
        {
            int xr, yr;
            EV()->Cursor().get_xy(&xr, &yr);
            SIlcx()->setLastXY(xr, yr);
        }
        break;
    case 6:
        SIlcx()->setCurGhost(6);
        g = GFbox;
        break;
    case 7:
        SIlcx()->setCurGhost(7);
        g = GFbox_ns;
        break;
    case 8:
        SIlcx()->setCurGhost(8);
        g = GFscript;
        break;
    case 9:
        SIlcx()->setCurGhost(9);
        g = GFvector_ns;
        {
            int xr, yr;
            EV()->Cursor().get_raw(&xr, &yr);
            SIlcx()->setLastXY(xr, yr);
        }
        break;
    case 10:
        SIlcx()->setCurGhost(10);
        g = GFvector;
        {
            int xr, yr;
            EV()->Cursor().get_xy(&xr, &yr);
            SIlcx()->setLastXY(xr, yr);
        }
        EV()->SetConstrained(true);
        break;
    case 11:
        SIlcx()->setCurGhost(11);
        g = GFvector_ns;
        {
            int xr, yr;
            EV()->Cursor().get_raw(&xr, &yr);
            SIlcx()->setLastXY(xr, yr);
        }
        EV()->SetConstrained(true);
        break;
    }
    if (g == GFnone)
        SIlcx()->decGhostCount();
    else
        SIlcx()->incGhostCount();
    Gst()->SetGhost(g);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


//-------------------------------------------------------------------------
// Graphics
//-------------------------------------------------------------------------

// (handle) GRopen(display, window)
//
// This function returns a handle to a graphical interface that can be
// used to export graphics to a foreign X window, possibly on another
// machine.  The first argument is the X display string, corresponding
// to the server which owns the target window.  The second argument is
// the X window id of the target window to which graphics rendering is
// to be exported.  If all goes well, and the user has permission to
// access the window, a positive integer handle is returned.  If the
// open fails, 0 is returned.  The handle should be closed with the
// Close() function when done.  This interface is not available on
// Microsoft Windows.
//
bool
misc1_funcs::IFgrOpen(Variable *res, Variable *args, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
#ifdef WIN32
    (void)args;
#else
    const char *dname;
    ARG_CHK(arg_string(args, 0, &dname))
    unsigned win;
    ARG_CHK(arg_unsigned(args, 1, &win))

    if (!dname || !*dname)
        return (BAD);
    Xdraw *xd = new Xdraw(dname, win);
    if (xd && !xd->check_error()) {
        sHdl *hdl = new sHdlGraph(xd);
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
#endif
    return (OK);
}


// (int) GRcheckError()
//
// This function returns 1 if the previous operation by any of the GR
// interface functions caused an X error.
//
bool
misc1_funcs::IFgrCheckError(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = Xdraw::check_error();
    return (OK);
}


// (drawable) GRcreatePixmap(handle, width, height)
//
// This function returns the X id of a new pixmap.  The first argument
// is a handle returned from GRopen().  The remaining arguments set
// the size of the pixmap.  If the operation fails, 0 is returned.
//
bool
misc1_funcs::IFgrCreatePixmap(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int width;
    ARG_CHK(arg_int(args, 1, &width))
    int height;
    ARG_CHK(arg_int(args, 2, &height))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd)
            res->content.value = xd->create_pixmap(width, height);
    }
    return (OK);
}


// (int) GRdestroyPixmap(handle, pixmap)
//
// This function destroys a pixmap created with GRcreatePixmap().  The
// first argument is a handle returned from GRopen().  The second
// argument is the pixmap id returned from GRcreatePixmap().  The
// function returns 1 on success, 0 if there was an error.
//
bool
misc1_funcs::IFgrDestroyPixmap(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int pixmap;
    ARG_CHK(arg_int(args, 1, &pixmap))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->destroy_pixmap(pixmap);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRcopyDrawable(handle, dst, src, xs, ys, ws, hs, x, y)
//
// This function is used to copy area between drawables, which can be
// windows or pixmaps.  The first argument is a handle returned from
// GRopen().  The next two arguments are the ids of destination and
// source drawables.  The area copied in the source drawable is given
// by the next four arguments.  The coordinates are pixel values, with
// the origin in the upper left corner.  If these four values are all
// zero, the entire source drawable is understood.  The final two
// values give the upper left corner of the copied-to area in the
// destination drawable.
//
bool
misc1_funcs::IFgrCopyDrawable(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int dst;
    ARG_CHK(arg_int(args, 1, &dst))
    int src;
    ARG_CHK(arg_int(args, 2, &src))
    int xs;
    ARG_CHK(arg_int(args, 3, &xs))
    int ys;
    ARG_CHK(arg_int(args, 4, &ys))
    int ws;
    ARG_CHK(arg_int(args, 5, &ws))
    int hs;
    ARG_CHK(arg_int(args, 6, &hs))
    int x;
    ARG_CHK(arg_int(args, 7, &x))
    int y;
    ARG_CHK(arg_int(args, 8, &y))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd)
            res->content.value = xd->copy_drawable(dst, src, xs, ys+hs, xs+ws,
                ys, x, y);
    }
    return (OK);
}


// (int) GRdraw(handle, l, b, r, t)
//
// This function renders an Xic cell.  The first argument is a handle
// returned from GRopen().  The remaining arguments are the
// coordinates of the cell to render, in microns.  The action is the
// same as the Display() function.  The function returns 1 on success,
// 0 if there was an error.
//
bool
misc1_funcs::IFgrDraw(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int l;
    ARG_CHK(arg_coord(args, 1, &l, DSP()->CurMode()))
    int b;
    ARG_CHK(arg_coord(args, 2, &b, DSP()->CurMode()))
    int r;
    ARG_CHK(arg_coord(args, 3, &r, DSP()->CurMode()))
    int t;
    ARG_CHK(arg_coord(args, 4, &t, DSP()->CurMode()))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd)
            res->content.value = xd->draw(l, b, r, t);
    }
    return (OK);
}


// (int) GRgetDrawableSize(handle, drawable, array)
//
// This function returns the size, in pixels, of a drawable.  The
// first argument is a handle returned from GRopen().  The second
// argument is the id of a window or pixmap.  The third argument is an
// array of size two or larger that will contain the pixel width and
// height of the drawable.  Upon success, 1 is returned, and the array
// values are set, otherwise 0 is returned.  The width is in the 0'th
// array element.
//
bool
misc1_funcs::IFgrGetDrawableSize(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int win;
    ARG_CHK(arg_int(args, 1, &win))
    double *vals;
    ARG_CHK(arg_array(args, 2, &vals, 2))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            int w, h;
            res->content.value = xd->get_drawable_size(win, &w, &h);
            if (to_boolean(res->content.value)) {
                vals[0] = w;
                vals[1] = h;
            }
        }
    }
    return (OK);
}


// (drawable) GRresetDrawable(handle, drawable)
//
// This function allows the target window of the graphical context to
// be changed.  Then, the rendering functions will draw into the new
// window or pixmap, rather than the one passed to GRopen().  The
// return value is the previous drawable id, or 0 if there is an
// error.
//
bool
misc1_funcs::IFgrResetDrawable(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int dable;
    ARG_CHK(arg_int(args, 1, &dable))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd)
            res->content.value = xd->reset_drawable(dable);
    }
    return (OK);
}


// (int) GRclear(handle)
//
// This function clears the window.  The argument is a handle returned
// from GRopen().  Upon success, 1 is returned, otherwise 0 is
// returned.
//
bool
misc1_funcs::IFgrClear(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->Clear();
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRpixel(handle, x, y)
//
// This function draws a single pixel at the pixel coordinates given
// in the second and third arguments, using the current color.  The
// first argument is a handle returned from GRopen().  Upon success, 1
// is returned, otherwise 0 is returned.
//
bool
misc1_funcs::IFgrPixel(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int x;
    ARG_CHK(arg_int(args, 1, &x))
    int y;
    ARG_CHK(arg_int(args, 2, &y))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->Pixel(x, y);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRpixels(handle, array, num)
//
// This function will draw multiple pixels using the current color.
// The first argument is a handle returned from GRopen().  The second
// argument is an array of pixel coordinates, taken as x-y pairs.  The
// third argument is the number of pixels to draw (half the length of
// the array).  Upon success, 1 is returned, otherwise 0 is returned.
//
bool
misc1_funcs::IFgrPixels(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int num;
    ARG_CHK(arg_int(args, 2, &num))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 2*num))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            GRmultiPt p(num);
            for (int i = 0; i < num; i++) {
                int x = (int)(*vals++);
                int y = (int)(*vals++);
                p.assign(i, x, y);
            }
            xd->Pixels(&p, num);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRline(handle, x1, y1, x2, y2)
//
// This function renders a line using the current color and line
// style.  The first argument is a handle returned from GRopen().  The
// next four arguments are the endpoints of the line in pixel
// coordinates.  Upon success, 1 is returned, otherwise 0 is returned.
//
bool
misc1_funcs::IFgrLine(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int x1;
    ARG_CHK(arg_int(args, 1, &x1))
    int y1;
    ARG_CHK(arg_int(args, 2, &y1))
    int x2;
    ARG_CHK(arg_int(args, 3, &x2))
    int y2;
    ARG_CHK(arg_int(args, 4, &y2))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->Line(x1, y1, x2, y2);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRpolyLine(handle, array, num)
//
// This function renders a polyline in the current color and line
// style.  The first argument is a handle returned from GRopen().  The
// second argument is an array containing vertex coordinates in pixels
// as x-y pairs.  The line will be continued to each successive
// vertex.  The third argument is the number of vertices (half the
// length of the array).  Upon success, 1 is returned, otherwise 0 is
// returned.
//
bool
misc1_funcs::IFgrPolyLine(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int num;
    ARG_CHK(arg_int(args, 2, &num))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 2*num))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            GRmultiPt p(num);
            for (int i = 0; i < num; i++) {
                int x = (int)(*vals++);
                int y = (int)(*vals++);
                p.assign(i, x, y);
            }
            xd->PolyLine(&p, num);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRlines(handle, array, num)
//
// This function renders multiple distinct lines, each using the
// current color and line style.  The first argument is a handle
// returned by GRopen().  The second argument is an array of
// coordinates, in pixels, which if taken four at a time give the x-y
// endpoints of each line.  The third argument is the number of lines
// in the array (one fourth the array length).  Upon success, 1 is
// returned, otherwise 0 is returned.
//
bool
misc1_funcs::IFgrLines(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int num;
    ARG_CHK(arg_int(args, 2, &num))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 4*num))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            int n = num*2;
            GRmultiPt p(n);
            for (int i = 0; i < n; i++) {
                int x = (int)(*vals++);
                int y = (int)(*vals++);
                p.assign(i, x, y);
            }
            xd->Lines(&p, num);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRbox(handle, l, b, r, t)
//
// This function renders a rectangular area in the current color with
// the current fill pattern.  The first argument is a handle returned
// from GRopen().  The remaining arguments provide the diagonal
// vertices of the rectangle, in pixels.  Upon success, 1 is returned,
// otherwise 0 is returned.
//
bool
misc1_funcs::IFgrBox(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int x1;
    ARG_CHK(arg_int(args, 1, &x1))
    int y1;
    ARG_CHK(arg_int(args, 2, &y1))
    int x2;
    ARG_CHK(arg_int(args, 3, &x2))
    int y2;
    ARG_CHK(arg_int(args, 4, &y2))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->Box(x1, y1, x2, y2);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRboxes(handle, array, num)
//
// This function renders multiple rectangles, each using the current
// color and fill pattern.  The first argument is a handle returned
// from GRopen().  the second argument is an array of pixel
// coordinates which specify the boxes.  Taken four at a time, the
// values are the upper-left corner (x-y), width, and height.  The
// third argument is the number of boxes represented in the array (one
// fourth the array length).  Upon success, 1 is returned, otherwise 0
// is returned.
//
bool
misc1_funcs::IFgrBoxes(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int num;
    ARG_CHK(arg_int(args, 2, &num))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 4*num))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            // each box is specified by four values: x, y, w, h
            int n = num*2;
            GRmultiPt p(n);
            for (int i = 0; i < n; i++) {
                int x = (int)(*vals++);
                int y = (int)(*vals++);
                p.assign(i, x, y);
            }
            xd->Boxes(&p, num);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRarc(handle, x0, y0, rx, ry, theta1, theta2)
//
// This function renders an arc, using the current color and line
// style.  The first argument is a handle returned from GRopen().  The
// next two arguments are the pixel coordinates of the center of the
// ellipse containing the arc.  The remaining arguments are the x and
// y radii, and the starting and ending angles.  The angles are in
// radians, relative to the three-o'clock position.  Upon succes, 1 is
// returned, otherwise 0 is returned.
//
bool
misc1_funcs::IFgrArc(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int x0;
    ARG_CHK(arg_int(args, 1, &x0))
    int y0;
    ARG_CHK(arg_int(args, 2, &y0))
    int rx;
    ARG_CHK(arg_int(args, 3, &rx))
    int ry;
    ARG_CHK(arg_int(args, 4, &ry))
    double theta1;
    ARG_CHK(arg_real(args, 5, &theta1))
    double theta2;
    ARG_CHK(arg_real(args, 6, &theta2))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->Arc(x0, y0, rx, ry, theta1, theta2);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRpolygon(handle, array, num)
//
// This function renders a polygon, using the current color and fill
// pattern.  The first argument is a handle returned from GRopen().
// The second argument is an array containing the vertices, as x-y
// pairs of pixel coordinates.  The third argument is the number of
// vertices (half the length of the array).  The polygon will be
// closed automatically if the first and last vertices do not
// coincide.  Upon success, 1 is returned, otherwise 0 is returned.
//
bool
misc1_funcs::IFgrPolygon(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int num;
    ARG_CHK(arg_int(args, 2, &num))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 2*num))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            GRmultiPt p(num);
            for (int i = 0; i < num; i++) {
                int x = (int)(*vals++);
                int y = (int)(*vals++);
                p.assign(i, x, y);
            }
            xd->Polygon(&p, num);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRtext(handle, text, x, y, transform)
//
// This function renders text in the current color.  The first
// argument is a handle returned form GRopen().  The second argument
// is the text string to render.  The next two argumments give the
// anchor point in pixel coordinates.  If there is no transformation,
// this will be the lower-left of the bounding box of the rendered
// text.  The next argument is a transformation code, which allows the
// text to be rotated and/or reflected about the anchor point.  The
// bits in this code have the following effects:
//
//   0-1  0-no rotation, 00-90, 10-180, 11-270.
//   2    mirror y after rotation
//   3    mirror x after rotation and mirror y
//   4    shift rotation to 45, 135, 225, 315
//   5-6  horiz justification 00,11 left, 01 center, 10 right
//   7-8  vert justification 00,11 bottom, 01 center, 10 top
//
// Upon success, 1 is returned, otherwise 0 is returned.
//
bool
misc1_funcs::IFgrText(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *text;
    ARG_CHK(arg_string(args, 1, &text))
    int x;
    ARG_CHK(arg_int(args, 2, &x))
    int y;
    ARG_CHK(arg_int(args, 3, &y))
    int xform;
    ARG_CHK(arg_int(args, 4, &xform))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (text) {
        sHdl *hdl = sHdl::get(id);
        if (hdl) {
            if (hdl->type != HDLgraph)
                return (BAD);
            Xdraw *xd = (Xdraw*)hdl->data;
            if (xd) {
                xd->Text(text, x, y, xform);
                res->content.value = 1;
            }
        }
    }
    return (OK);
}


// (int) GRtextExtent(handle, text, array)
//
// This function returns the width and height in pixels needed to
// render a text string.  The first argument is a handle returned from
// GRopen().  The second argument is the string to measure.  If the
// string is null or empty, a "typical" single character width and
// height is returned, which can be simply multiplied for the
// fixed-pitch font in use.  The third argument is an array of size
// two or larger which will receive the width (0'th index) and height.
// The function returns 1 on success, 0 otherwise.
//
bool
misc1_funcs::IFgrTextExtent(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *text;
    ARG_CHK(arg_string(args, 1, &text))
    double *vals;
    ARG_CHK(arg_array(args, 2, &vals, 2))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            int w, h;
            xd->TextExtent(text, &w, &h);
            vals[0] = w;
            vals[1] = h;
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRdefineColor(handle, red, green, blue)
//
// This function will return a color code corresponding to the given
// color.  The first argument is a handle returned from GRopen().  The
// next three arguments are color component values, each in a range
// 0-255, giving the red, green, and blue intensity.  The return value
// is a color code representing the nearest displayable color to that
// given.  If an error occurs, 0 (black) is returned.  The returned
// color code can be passed to GRsetColor() to actually change the
// drawing color.
//
bool
misc1_funcs::IFgrDefineColor(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int red;
    ARG_CHK(arg_int(args, 1, &red))
    int green;
    ARG_CHK(arg_int(args, 2, &green))
    int blue;
    ARG_CHK(arg_int(args, 3, &blue))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            int pix;
            xd->DefineColor(&pix, red, green, blue);
            res->content.value = pix;
        }
    }
    return (OK);
}


// (int) GRsetBackground(handle, pixel)
//
// This function sets the default background color assumed by the
// graphics context.  The first argument is a handle returned from
// GRopen().  The second argument is a color code returned from
// GRdefineColor().  Upon success, 1 is returned, otherwise 0 is
// returned.
//
bool
misc1_funcs::IFgrSetBackground(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int pix;
    ARG_CHK(arg_int(args, 1, &pix))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->SetBackground(pix);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRsetWindowBackground(handle, pixel)
//
// This function sets the color used to render the window background
// when the window is cleared.  The first argument is a handle
// returned from GRopen().  The second argument is a color code
// returned from GRdefineColor().  The function returns 1 on success,
// 0 otherwise.
//
bool
misc1_funcs::IFgrSetWindowBackground(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int pix;
    ARG_CHK(arg_int(args, 1, &pix))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->SetWindowBackground(pix);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRsetColor(handle, pixel)
//
// This function sets the current color, used for all rendering
// functions.  The first argument is a handle returned from GRopen().
// The second argument is a color code returned from GRdefineColor().
// Upon success, 1 is returned, otherwise 0 is returned.
//
bool
misc1_funcs::IFgrSetColor(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int pix;
    ARG_CHK(arg_int(args, 1, &pix))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->SetColor(pix);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRdefineLinestyle(handle, index, mask)
//
// This function defines a line style.  The first argument is a handle
// returned from GRopen().  The second argument is an index value 1-15
// which corresponds to an internal line style register.  The third
// argument is an integer value whose bits set the line on/off
// pattern.  the pattern starts with the most significant '1' bit in
// the mask.  The '1' bits will be drawn.  The pattern continues to
// the least significant bit, and is repeated as the line is rendered.
// The indices 1-10 contain pre-defined line styles, which can be
// overwritten with this function.  The SetLinestyle() function is
// used to set the pattern actually used for rendering.  Upon success,
// 1 is returned, otherwise 0 is returned.
//
bool
misc1_funcs::IFgrDefineLinestyle(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int indx;
    ARG_CHK(arg_int(args, 1, &indx))
    int mask;
    ARG_CHK(arg_int(args, 2, &mask))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (indx < 1 || indx >= XD_NUM_LINESTYLES)
        return (OK);
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->DefineLinestyle(indx, mask);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRsetLinestyle(handle, index)
//
// This function sets the line style used to render lines.  The first
// argument is a handle returned from GRopen().  The second argument
// is an integer 0-15 which corresponds to an internal style register.
// Index 0 is always solid, whereas the other values can be set with
// GRdefineLinestyle().  The function returns 1 on success, 0
// otherwise.
//
bool
misc1_funcs::IFgrSetLinestyle(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int indx;
    ARG_CHK(arg_int(args, 1, &indx))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (indx < 0 || indx >= XD_NUM_LINESTYLES)
        return (OK);
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->SetLinestyle(indx);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRdefineFillpattern(handle, index, nx, ny, array)
//
// This function is used to define a fill pattern for rendering boxes
// and polygons.  The first argument is a handle returned from
// GRopen().  The second argument is an integer 1-15 which corresponds
// to internal fill pattern registers.  The next two arguments set the
// x and y size of the pixel map used for the fill pattern.  These can
// take values of 8 or 16 only.  The final argument is a character
// string which contains the pixel map.  The most significant bit of
// the first byte is the upper left corner of the map.  The
// SetFillpattern() function is used to set the fill pattern actually
// used for rendering.  The function returns 1 on success, 0
// otherwise.
//
bool
misc1_funcs::IFgrDefineFillpattern(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int indx;
    ARG_CHK(arg_int(args, 1, &indx))
    int nx;
    ARG_CHK(arg_int(args, 2, &nx))
    int ny;
    ARG_CHK(arg_int(args, 3, &ny))
    const char *arry;
    ARG_CHK(arg_string(args, 4, &arry))

    if (nx != 8 && nx != 16)
        return (BAD);
    if (ny != 8 && ny != 16)
        return (BAD);
    if (!arry || (int)strlen(arry) < (nx*ny)/8)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (indx < 0 || indx >= XD_NUM_LINESTYLES)
        return (OK);
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->DefineFillpattern(indx, nx, ny, (unsigned char*)arry);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRsetFillpattern(handle, index)
//
// This function sets the fill pattern used for rendering boxes and
// polygons.  The first argument is a handle returned from GRopen().
// The second argument is an integer index 0-15 which corresponds to
// internal fill pattern registers.  The value 0 is always solid fill.
// The other values can be set with GRdefineFillpattern().  Upon
// success, 1 is returned, otherwise 0 is returned.
//
bool
misc1_funcs::IFgrSetFillpattern(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int indx;
    ARG_CHK(arg_int(args, 1, &indx))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (indx < 0 || indx >= XD_NUM_FILLPATTS)
        return (OK);
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->SetFillpattern(indx);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRupdate(handle)
//
// This function flushes the X queue ad causes any pending operations
// to be performed.  This should be called after completing a sequence
// of drawing functions, to force a screen update.  Upon success, 1 is
// returned, otherwise 0 is returned.
//
bool
misc1_funcs::IFgrUpdate(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->Update();
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GRsetMode(handle, mode)
//
// This function sets the drawing mode used for rendering.  The first
// argument is a handle returned from GRopen().  The second argument
// is one of the following:
//   0       normal drawing
//   1       XOR
//   2       OR
//   3       AND-interted
// Modes 2,3 are probably not useful on other than 8-plane displays.
// The function returns 1 on success, 0 otherwise.
//
bool
misc1_funcs::IFgrSetMode(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int mode;
    ARG_CHK(arg_int(args, 1, &mode))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLgraph)
            return (BAD);
        Xdraw *xd = (Xdraw*)hdl->data;
        if (xd) {
            xd->SetXOR(mode);
            res->content.value = 1;
        }
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Hard Copy
//-------------------------------------------------------------------------

// The following functions provide an interface for plot and graphical
// file output.  This is completely outside of the normal printing
// interface.

namespace {
    // The printer interface operates outside of the normal graphical
    // interface, this holds the state
    //
    struct hcstate
    {
        void set_desc(HCdesc*, int);

        HCdesc *desc;   // driver desc
        int resol;      // resolution to pass to driver
        bool best_fit;  // true if best fit enabled
        bool legend;    // true if legend enabled
        bool landscape; // true if landscape mode
        bool metric;    // true if metric values
        double x, y;    // image offsets
        double w, h;    // image size
        char *errmsg;   // error message string
        int media;      // media index (Windows Native driver only)
        int drvrnum;    // driver index
    };
    hcstate HCstate;


    // Set the defaults for a new driver
    //
    void
    hcstate::set_desc(HCdesc *d, int drvr)
    {
        desc = d;
        resol = 0;
        if (d->limits.resols) {
            const char *s = d->limits.resols[d->defaults.defresol];
            resol = atoi(s);
        }
        best_fit = !(d->limits.flags & HCnoBestOrient);
        legend = (d->defaults.legend != HClegNone);
        landscape = false;
        metric = false;
        x = 0.0;
        if (!(d->limits.flags & HCdontCareXoff))
            x = d->defaults.defxoff;
        y = 0.0;
        if (!(d->limits.flags & HCdontCareYoff))
            y = d->defaults.defyoff;
        w = 0.0;
        if (!(d->limits.flags & HCdontCareWidth))
            w = d->defaults.defwidth;
        h = 0.0;
        if (!(d->limits.flags & HCdontCareHeight))
            h = d->defaults.defheight;
        errmsg = 0;
        media = 0;
        drvrnum = drvr;
    }


    // Make an argv-type string array from string str.
    //
    void
    mkargv(int *acp, char **av, char *str)
    {
        char *s = str;
        int j = 0;
        for (;;) {
            while (isspace(*s)) s++;
            if (!*s) {
                *acp = j;
                return;
            }
            char *t = s;
            while (*t && !isspace(*t)) t++;
            if (*t)
                *t++ = '\0';
            av[j++] = s;
            s = t;
        }
    }


    // Execute the print command sting
    //
    int
    printit(const char *str, const char *filename)
    {
#ifdef WIN32
        const char *s = msw::RawFileToPrinter(str, filename);
        if (s) {
            Log()->WarningLogV("printing", "printing", "%s\n", s);
            return (1);
        }
        return (0);

#else

        // Check for '%s' and substitute filename, otherwise cat the
        // filename.
        bool submade = false;
        const char *s = str;
        char buf[256];
        char *t = buf;
        while (*s) {
            if (*s == '%' && *(s+1) == 's') {
                strcpy(t, filename);
                while (*t)
                    t++;
                s += 2;
                submade = true;
            }
            else
                *t++ = *s++;
        }
        if (!submade) {
            *t++ = ' ';
            strcpy(t, filename);
        }
        else
            *t = '\0';
        return (cMain::System(buf));
#endif
    }
}


// (stringlist_handle) HClistDrivers()
//
// This function returns a handle to a list of available printer
// drivers.  The returned handle can be processed by any of the
// functions that operate on stringlist handles.
//
bool
misc1_funcs::IFhcListDrivers(Variable *res, Variable*, void*)
{
    stringlist *s0 = 0;
    for (int i = 0; GRpkgIf()->HCof(i); i++)
        s0 = new stringlist(lstring::copy(GRpkgIf()->HCof(i)->keyword), s0);
    stringlist::sort(s0);
    sHdl *hdl = new sHdlString(s0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (int) HCsetDriver(driver)
//
// This function will set the current print driver to the name passed
// (as a string).  The name must be one of the internal driver names
// as returned from HClistDrivers().  If the operation succeeds, the
// function returns 1, otherwise 0 is returned.
//
bool
misc1_funcs::IFhcSetDriver(Variable *res, Variable *args, void*)
{
    const char *drvr;
    ARG_CHK(arg_string(args, 0, &drvr))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (drvr) {
        HCdesc *hcdesc = GRpkgIf()->FindHCdesc(drvr);
        if (hcdesc) {
            HCstate.set_desc(hcdesc, GRpkgIf()->FindHCindex(drvr));
            res->content.value = 1;
        }
    }
    return (OK);
}


// (string) HCgetDriver()
//
// This function returns the internal name of the current driver.  If
// no driver has been set, a null string is returned.
//
bool
misc1_funcs::IFhcGetDriver(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string =
        HCstate.desc ? lstring::copy(HCstate.desc->keyword) : 0;
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) HCsetResol(resol)
//
// This function will set the resolution of the current driver to the
// value passed.  The scalar argument should be one of the values
// supported by the driver, as returned from HCgetResols().  If the
// resolution is set successfully, 1 is returned.  If no driver has
// been set, or the driver does not support the given resolution, 0 is
// returned.
//
bool
misc1_funcs::IFhcSetResol(Variable *res, Variable *args, void*)
{
    int resol;
    ARG_CHK(arg_int(args, 0, &resol))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (HCstate.desc && HCstate.desc->limits.resols) {
        for (const char **s = HCstate.desc->limits.resols; *s; s++) {
            if (resol == atoi(*s)) {
                HCstate.resol = resol;
                res->content.value = 1;
                break;
            }
        }
    }
    return (OK);
}


// (int) HCgetResol()
//
// This function returns the resolution set for the current driver, or
// 0 if no driver has been set or the driver does not provide settable
// resolutions.
//
bool
misc1_funcs::IFhcGetResol(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = HCstate.resol;
    return (OK);
}


// (int) HCgetResols(array)
//
// This function sets the array values to the resolutions supported by
// the current driver.  The array must have size 8 or larger.  The
// return value is the number of resolutions supported.  If no driver
// has been set, or the driver has fixed resolution, 0 is returned.
//
bool
misc1_funcs::IFhcGetResols(Variable *res, Variable *args, void*)
{
    double *vals;
    ARG_CHK(arg_array(args, 0, &vals, 8))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (HCstate.desc && HCstate.desc->limits.resols) {
        int i = 0;
        int len = args[0].content.a->length();
        for (const char **s = HCstate.desc->limits.resols; *s; s++) {
            if (i == len)
                // This shouldn't happen, since existing drivers have
                // 7 values max.  Really should dynamically allocate.
                break;
            vals[i] = atof(*s);
            i++;
        }
        res->content.value = i;
    }
    return (OK);
}


// (int) HCsetBestFit(best_fit)
//
// This function will set or reset the "best fit" flag for the current
// driver.  In best fit mode, the image will be rotated 90 degrees if
// this is a better match to the aspect ratio of the rendering area.
// If the operation succeeds, 1 is returned.  If there is no driver
// set or the driver does not allow best fit mode, 0 is returned.  If
// the argument is nonzero, best fit mode will be set if possible,
// otherwise the mode is unset.
//
bool
misc1_funcs::IFhcSetBestFit(Variable *res, Variable *args, void*)
{
    bool bf;
    ARG_CHK(arg_boolean(args, 0, &bf))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (HCstate.desc && !(HCstate.desc->limits.flags & HCnoBestOrient)) {
        res->content.value = 1;
        HCstate.best_fit = bf;
    }
    return (OK);
}


// (int) HCgetBestFit()
//
// This function returns 1 if the current driver is in "best fit"
// mode, 0 otherwise.
//
bool
misc1_funcs::IFhcGetBestFit(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = HCstate.best_fit;
    return (OK);
}


// (int) HCsetLegend(legend)
//
// This function will set or reset the "legend" flag for the current
// driver.  If set, a legend will be shown with the rendered image.
// If the operation succeeds, 1 is returned.  If there is no driver
// set or the driver does not allow a legend, 0 is returned.  If the
// argument is nonzero, the legend mode will be set if possible,
// otherwise the mode is unset.
//
bool
misc1_funcs::IFhcSetLegend(Variable *res, Variable *args, void*)
{
    bool leg;
    ARG_CHK(arg_boolean(args, 0, &leg))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (HCstate.desc && HCstate.desc->defaults.legend != HClegNone) {
        res->content.value = 1;
        HCstate.legend = leg;
    }
    return (OK);
}


// (int) HCgetLegend()
//
// This function returns 1 if the current driver has the "legend" mode
// set, 0 otherwise.
//
bool
misc1_funcs::IFhcGetLegend(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = HCstate.legend;
    return (OK);
}


// (int) HCsetLandscape(landscape)
//
// This function will set or reset the "landscape" flag for the
// current driver.  If set, the image will be rotated 90 degrees.  If
// the operation succeeds, 1 is returned.  If there is no driver set
// or the driver does not allow landscape mode, 0 is returned.  If the
// argument is nonzero, the landscape mode will be set if possible,
// otherwise the mode is unset.
//
bool
misc1_funcs::IFhcSetLandscape(Variable *res, Variable *args, void*)
{
    bool lands;
    ARG_CHK(arg_boolean(args, 0, &lands))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (HCstate.desc && !(HCstate.desc->limits.flags & HCnoLandscape)) {
        res->content.value = 1;
        HCstate.landscape = lands;
    }
    return (OK);
}


// (int) HCgetLandscape()
//
// This function returns 1 if the current driver has the "landscape"
// mode set, 0 otherwise.
//
bool
misc1_funcs::IFhcGetLandscape(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = HCstate.landscape;
    return (OK);
}


// (int) HCsetMetric(metric)
//
// This function sets a flag in the current driver which indicates
// that the rendering area is given in millimeters.  If not set, the
// values are taken in inches.  This pertains to the values passed to
// the HCsetSize() function.  If the operation succeeds, 1 is
// returned.  If there is no driver set, 0 is returned.  If the
// argument is nonzero, the metric mode will be set if possible,
// otherwise the mode is unset.
//
bool
misc1_funcs::IFhcSetMetric(Variable *res, Variable *args, void*)
{
    bool metric;
    ARG_CHK(arg_boolean(args, 0, &metric))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (HCstate.desc) {
        res->content.value = 1;
        HCstate.metric = metric;
    }
    return (OK);
}


// (int) HCgetMetric()
//
// This function returns 1 if the current driver has the "metric"
// mode set, 0 otherwise.
//
bool
misc1_funcs::IFhcGetMetric(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = HCstate.metric;
    return (OK);
}


namespace {
    inline double
    mm(double x)
    {
        return (HCstate.metric ? x*25.4 : x);
    }
}


// (int) HCsetSize(x, y, w, h)
//
// This function sets the size and offset of the rendering area.  The
// numbers correspond to the entries in the Print panel.  The values
// are scalars, in inches unless metric mode is in effect (with
// HCsetMetric()) in which case the values are in millimeters.  The
// values are clipped to the limits provided in the technology file.
// Most drivers accept 0 for one of w, h, indicating auto dimensioning
// mode.  The function returns 1 on success, 0 if no driver has been
// set.  Not all drivers use all four parameters, unused parameters
// are ignored.
//
bool
misc1_funcs::IFhcSetSize(Variable *res, Variable *args, void*)
{
    double xoff;
    ARG_CHK(arg_real(args, 0, &xoff))
    double yoff;
    ARG_CHK(arg_real(args, 1, &yoff))
    double wid;
    ARG_CHK(arg_real(args, 2, &wid))
    double hei;
    ARG_CHK(arg_real(args, 3, &hei))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (HCstate.desc) {
        if (!(HCstate.desc->limits.flags & HCdontCareWidth)) {
            if (wid != 0 || (HCstate.desc->limits.flags & HCnoAutoWid)) {
                if (wid < mm(HCstate.desc->limits.minwidth))
                    wid = mm(HCstate.desc->limits.minwidth);
                if (wid > mm(HCstate.desc->limits.maxwidth))
                    wid = mm(HCstate.desc->limits.maxwidth);
            }
        }
        else
            wid = 0.0;
        if (!(HCstate.desc->limits.flags & HCdontCareHeight)) {
            if (hei != 0 || (HCstate.desc->limits.flags & HCnoAutoHei)) {
                if (hei < mm(HCstate.desc->limits.minheight))
                    hei = mm(HCstate.desc->limits.minheight);
                if (hei > mm(HCstate.desc->limits.maxheight))
                    hei = mm(HCstate.desc->limits.maxheight);
            }
        }
        else
            hei = 0.0;
        if (!(HCstate.desc->limits.flags & HCdontCareXoff)) {
            if (xoff < mm(HCstate.desc->limits.minxoff))
                xoff = mm(HCstate.desc->limits.minxoff);
            if (xoff > mm(HCstate.desc->limits.maxxoff))
                xoff = mm(HCstate.desc->limits.maxxoff);
        }
        else
            xoff = 0.0;
        if (!(HCstate.desc->limits.flags & HCdontCareYoff)) {
            if (yoff < mm(HCstate.desc->limits.minyoff))
                yoff = mm(HCstate.desc->limits.minyoff);
            if (yoff > mm(HCstate.desc->limits.maxyoff))
                yoff = mm(HCstate.desc->limits.maxyoff);
        }
        else
            yoff = 0.0;

        HCstate.x = xoff;
        HCstate.y = yoff;
        HCstate.w = wid;
        HCstate.h = hei;
        res->content.value = 1;
    }
    return (OK);
}


// (int) HCgetSize(array)
//
// This function returns the rendering area parameters for the current
// driver.  The array argument must have size 4 or larger.  The values
// are returned in the order x, y, w, h.  If the function succeeds,
// the values are set in the array and 1 is returned.  Otherwise, 0
// is returned.
//
bool
misc1_funcs::IFhcGetSize(Variable *res, Variable *args, void*)
{
    double *vals;
    ARG_CHK(arg_array(args, 0, &vals, 4))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (HCstate.desc) {
        vals[0] = HCstate.x;
        vals[1] = HCstate.y;
        vals[2] = HCstate.w;
        vals[3] = HCstate.h;
        res->content.value = 1;
    }
    return (OK);
}


// (int) HCshowAxes(style)
//
// This function sets the style or visibility of axes shown in plots
// of physical data (electrical plots never include axes).  The
// argument is an integer 0-2, where 0 suppresses drawing of axes, 1
// indicates plain axes, and 2 (or anything else) indicates axes with
// a box at the origin.  The return value is the previous setting.
//
bool
misc1_funcs::IFhcShowAxes(Variable *res, Variable *args, void*)
{
    int style;
    ARG_CHK(arg_int(args, 0, &style))

    sAttrContext *cx = Tech()->GetAttrContext(HCstate.drvrnum, true);
    if (!cx) {
        Errs()->add_error("HCshowAxes: internal error, context not found.");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    res->content.value = cx->attr()->grid(Physical)->axes();
    if (style == 0)
        cx->attr()->grid(Physical)->set_axes(AxesNone);
    else if (style == 1)
        cx->attr()->grid(Physical)->set_axes(AxesPlain);
    else
        cx->attr()->grid(Physical)->set_axes(AxesMark);
    return (OK);
}


// (int) HCshowGrid(show, mode)
//
// This function determines whether or not the grid is shown in plots.
// If the first argument is nonzero, the grid will be shown, otherwise
// the grid will not be shown.  The second argument indicates the type
// of data affected:  zero for physical data, nonzero for electrical
// data.  The return value is the previous setting.
//
bool
misc1_funcs::IFhcShowGrid(Variable *res, Variable *args, void*)
{
    bool show;
    ARG_CHK(arg_boolean(args, 0, &show))
    bool mode;
    ARG_CHK(arg_boolean(args, 1, &mode))

    sAttrContext *cx = Tech()->GetAttrContext(HCstate.drvrnum, true);
    if (!cx) {
        Errs()->add_error("HCshowGrid: internal error, context not found.");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    if (!mode) {
        res->content.value = cx->attr()->grid(Physical)->displayed();
        cx->attr()->grid(Physical)->set_displayed(show);
    }
    else {
        res->content.value = cx->attr()->grid(Electrical)->displayed();
        cx->attr()->grid(Electrical)->set_displayed(show);
    }
    return (OK);
}


// (real) HCsetGridInterval(spacing, mode)
//
// This function sets the grid spacing used in plots.  The first
// argument is the interval in microns.  The second argument indicates
// the type of data affected:  zero for physical data, nonzero for
// electrical data.  For electrical data, the spacing in microns is
// rather meaningless, except as being relative to the default which
// is 1.0.  The return value is the previous setting.
//
bool
misc1_funcs::IFhcSetGridInterval(Variable *res, Variable *args, void*)
{
    double resol;
    ARG_CHK(arg_real(args, 0, &resol))
    bool mode;
    ARG_CHK(arg_boolean(args, 1, &mode))

    if (resol < 0)
        return (BAD);
    sAttrContext *cx = Tech()->GetAttrContext(HCstate.drvrnum, true);
    if (!cx) {
        Errs()->add_error("HCsetGridInterval: internal error, context not found.");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    if (!mode) {
        res->content.value = cx->attr()->grid(Physical)->spacing(Physical);
        cx->attr()->grid(Physical)->set_spacing(resol);
        cx->attr()->grid(Physical)->set_snap(1);
    }
    else {
        res->content.value = cx->attr()->grid(Electrical)->spacing(Electrical);
        cx->attr()->grid(Electrical)->set_spacing(resol);
        cx->attr()->grid(Electrical)->set_snap(1);
    }
    return (OK);
}


// (int) HCsetGridStyle(linemod, mode)
//
// This function sets the line style used for the grid lines in plots.
// The first argument is an integer mask that defines the on-off
// pattern.  The pattern starts at the most significant '1' bit and
// continues through the least significant bit, and repeats.  Set bits
// are rendered as the visible part of the pattern.  If the style is
// 0, a dot is shown at each grid point.  Passing -1 will give
// continuous lines.  The second argument indicates the type of data
// affected:  zero for physical data, nonzero for electrical data.
// The return value is the previous setting.
//
bool
misc1_funcs::IFhcSetGridStyle(Variable *res, Variable *args, void*)
{
    int style;
    ARG_CHK(arg_int(args, 0, &style))
    bool mode;
    ARG_CHK(arg_boolean(args, 1, &mode))

    sAttrContext *cx = Tech()->GetAttrContext(HCstate.drvrnum, true);
    if (!cx) {
        Errs()->add_error("HCsetGridStyle: internal error, context not found.");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    if (!mode) {
        res->content.value = cx->attr()->grid(Physical)->linestyle().mask;
        cx->attr()->grid(Physical)->linestyle().mask = style;
    }
    else {
        res->content.value = cx->attr()->grid(Electrical)->linestyle().mask;
        cx->attr()->grid(Electrical)->linestyle().mask = style;
    }
    return (OK);
}


// HCsetGridCrossSize(xsize, mode)
//
// This applies only to grids with style 0 (dot grid).  The xsize is
// an integer 0-6 which indicates tne number of pixels to draw in the
// four compass directions around the central pixel.  Thus, for
// nonzero values, the "dot" is rendered as a small cross.  The second
// argument indicates the type of data affected:  zero for physical
// data, nonzero for electrical data.  The return value is 1 if the
// cross size was set, 0 if the grid style was nonzero in which case
// the cross size was not set.
//
bool
misc1_funcs::IFhcSetGridCrossSize(Variable *res, Variable *args, void*)
{
    int crs;
    ARG_CHK(arg_int(args, 0, &crs))
    bool mode;
    ARG_CHK(arg_boolean(args, 1, &mode))

    sAttrContext *cx = Tech()->GetAttrContext(HCstate.drvrnum, true);
    if (!cx) {
        Errs()->add_error("HCsetGridStyle: internal error, context not found.");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (!mode) {
        if (cx->attr()->grid(Physical)->linestyle().mask == 0) {
            cx->attr()->grid(Physical)->set_dotsize(crs);
            res->content.value = 1;
        }
    }
    else {
        if (cx->attr()->grid(Electrical)->linestyle().mask == 0) {
            cx->attr()->grid(Electrical)->set_dotsize(crs);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) HCsetGridOnTop(on_top, mode)
//
// This function sets whether the grid lines are drawn after the
// geometry ("on top") or before the geometry.  If the first argument
// is nonzero, the grid will be rendered on top.  The second argument
// indicates the type of data affected:  zero for physical data,
// nonzero for electrical data.  The return value is the previous
// setting.
//
bool
misc1_funcs::IFhcSetGridOnTop(Variable *res, Variable *args, void*)
{
    bool on_top;
    ARG_CHK(arg_boolean(args, 0, &on_top))
    bool mode;
    ARG_CHK(arg_boolean(args, 1, &mode))

    sAttrContext *cx = Tech()->GetAttrContext(HCstate.drvrnum, true);
    if (!cx) {
        Errs()->add_error("HCsetGridOnTop: internal error, context not found.");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    if (!mode) {
        res->content.value = cx->attr()->grid(Physical)->show_on_top();
        cx->attr()->grid(Physical)->set_show_on_top(on_top);
    }
    else {
        res->content.value = cx->attr()->grid(Electrical)->show_on_top();
        cx->attr()->grid(Electrical)->set_show_on_top(on_top);
    }
    return (OK);
}


// (int) HCdump(l, b, r, t, filename, command)
//
// This is the function which actually generates a plot or graphics
// file.  The first four arguments set the area in microns in current
// cell coordinates to render.  If these values are all 0, a full view
// of the current cell will be rendered.  The next argument is the
// name of the file to use for the graphical output.  If this string
// is null or empty, a temporary file will be used.  Under Windows,
// the final argument is the name of a printer, as known to the
// operating system.  These names can be obtained with
// HClistPrinters().  Under Unix/Linux, the last argument is a command
// string that will be executed to generate a plot.  In any case if
// this argument is null or empty, the plot file will be generated,
// but no further action will be taken.  In the command string, the
// character sequence "%s" will be replaced by the file name.  If the
// sequence does not appear, the file name will be appended.  If
// successful, 1 is returned, otherwise 0 is returned, and an error
// message can be obtained with HCerrorString().
//
// The filename, or the temporary file that is used if no filename is
// given, is *not* removed.  The user must remove the file explicitly.
//
// The Windows Native driver (Windows only) has slightly different
// behavior.  For this driver, the command string must specify a
// printer name, and can not be null or empty.  If filename is not
// null or empty, the output goes to that file and is *not* sent to
// the printer.  Otherwise, the output goes to the printer.
//
bool
misc1_funcs::IFhcDump(Variable *res, Variable *args, void*)
{
    int l;
    ARG_CHK(arg_coord(args, 0, &l, DSP()->CurMode()))
    int b;
    ARG_CHK(arg_coord(args, 1, &b, DSP()->CurMode()))
    int r;
    ARG_CHK(arg_coord(args, 2, &r, DSP()->CurMode()))
    int t;
    ARG_CHK(arg_coord(args, 3, &t, DSP()->CurMode()))
    const char *fname;
    ARG_CHK(arg_string(args, 4, &fname))
    const char *cmd;
    ARG_CHK(arg_string(args, 5, &cmd))

    HCcb &cb = Tech()->HC();
    res->type = TYP_SCALAR;
    res->content.value = 0;
    delete [] HCstate.errmsg;
    HCstate.errmsg = 0;
    if (!HCstate.desc) {
        HCstate.errmsg = lstring::copy("No current driver.");
        return (OK);
    }
    if (!DSP()->CurCellName()) {
        HCstate.errmsg = lstring::copy("No current cell.");
        return (OK);
    }

    // This sets color pixels in non-interactive graphics modes.
    XM()->FixupColors(0);

    if (l || b || r || t) {
        if (l > r)
            mmSwapInts(l, r);
        if (b > t)
            mmSwapInts(b, t);
        if (l == r || b == t) {
            HCstate.errmsg = lstring::copy("Bad frame.");
            return (OK);
        }
        (*cb.hcframe)(HCframeSet, 0, &l, &b, &r, &t, 0);
        (*cb.hcframe)(HCframeOn, 0, 0, 0, 0, 0, 0);
    }

#ifdef WIN32
    bool file_given = false;
#endif
    if (!fname || !*fname)
        fname = filestat::make_temp("hc");
    else {
        fname = lstring::copy(fname);
#ifdef WIN32
        file_given = true;
#endif
    }

    char buf[256];
    sprintf(buf, HCstate.desc->fmtstring, fname, HCstate.resol,
        HCstate.w, HCstate.h, HCstate.x, HCstate.y);
    if (HCstate.landscape)
        strcat(buf, " -l");

    char *cmdstr = lstring::copy(buf);
    char *argv[20];
    int argc;
    mkargv(&argc, argv, cmdstr);

#ifdef WIN32
    // butt-ugly hack to pass printer name and media index to
    // Windows Native driver
    if (!strcmp(HCstate.desc->keyword, "windows_native")) {
        if (!cmd || !*cmd) {
            delete [] cmdstr;
            delete [] fname;
            return (BAD);
        }
        argv[argc++] = (char*)"-nat";
        argv[argc++] = (char*)cmd;
        argv[argc++] = (char*)HCstate.media;
        if (file_given)
            cmd = 0;
    }
#endif

    HCswitchErr err = GRpkgIf()->SwitchDev(HCstate.desc->drname, &argc, argv);
    if (err == HCSinhc)
        HCstate.errmsg = lstring::copy("Internal error - aborted.");
    else if (err == HCSnotfnd)
        HCstate.errmsg = lstring::copy("Driver not available.");
    else if (err == HCSinit)
        HCstate.errmsg = lstring::copy(
            "Driver initialization failed - aborted.");
    else {
        bool ok = true;
        HCorientFlags ot = 0;
        if (HCstate.best_fit &&
                !(HCstate.desc->limits.flags & HCnoBestOrient))
            ot |= HCbest;
        if (HCstate.landscape &&
                !(HCstate.desc->limits.flags & HCnoLandscape) &&
                (HCstate.desc->limits.flags & HCnoCanRotate))
            // pass the landscape flag only if the driver can't rotate
            ot |= HClandscape;

        int drnum = 0;
        for ( ; GRpkgIf()->HCof(drnum); drnum++) {
            if (GRpkgIf()->HCof(drnum) == HCstate.desc)
                break;
        }
        if (GRpkgIf()->HCof(drnum) != HCstate.desc) {
            // "can't happen"
            HCstate.errmsg = lstring::copy("Driver not available.");
            return (OK);
        }
        (*cb.hcsetup)(true, drnum, true, 0);
        int ret = (*cb.hcgo)(ot, HCstate.legend ? HClegOn : HClegOff, 0);
        (*cb.hcframe)(HCframeOff, 0, 0, 0, 0, 0, 0);
        (*cb.hcsetup)(false, drnum, true, 0);
        if (ret) {
            HCstate.errmsg = lstring::copy(
                "Error encountered in hardcopy generation - aborted");
            ok = false;
            unlink(fname);
        }
        GRpkgIf()->SwitchDev(0, 0, 0);
        if (ok) {
            ret = 0;
            if (cmd && *cmd)
                ret = printit(cmd, fname);
            if (ret) {
                sprintf(buf, "Print command returned error status %d.", ret);
                HCstate.errmsg = lstring::copy(buf);
            }
            else
                res->content.value = 1;
        }
    }
    delete [] cmdstr;
#ifdef WIN32
    if (!file_given)
        unlink(fname);
#endif
    delete [] fname;
    return (OK);
}


// (string) HCerrorString()
//
// This function returns a string indicating the error generated by
// HCdump().  If there were no errors, a null string is returned.
//
bool
misc1_funcs::IFhcErrorString(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string = HCstate.errmsg;
    return (OK);
}


// (stringlist_handle) HClistPrinters()
//
// Under Microsoft Windows, this function returns a handle to a list
// of printer names available from the current host.  The first name
// is the name of the default printer.  The remaining names,
// alphabetized, follow.  If there are no printers available, or if
// not running under Windows, the function returns 0.  The returned
// names can be supplied to the HCdump() function to initiate a print
// job.
//
bool
misc1_funcs::IFhcListPrinters(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
#ifdef WIN32
    char **printers;
    int cur = 0;
    int num = msw::ListPrinters(&cur, &printers);
    if (num) {
        stringlist *s0 = 0;
        for (int i = 0; i < num; i++) {
            if (i == cur)
                continue;
            s0 = new stringlist(printers[i], s0);
        }
        stringlist::sort(s0);
        s0 = new stringlist(printers[cur], s0);
        delete [] printers;
        sHdl *hdl = new sHdlString(s0);
        res->type = TYP_HANDLE;
        res->content.value = hdl->id;
    }
#endif
    return (OK);
}


// (int) HCmedia(index)
//
// This function sets the media index, which is used by the Windows Native
// driver under Microsoft Windows only.  The media index sets the assumed
// paper size.  The argument is one of the following integers:
//
//  Index    Name         Size (mm)
//
//  0        Letter       612   792
//  1        Legal        612   1008
//  2        Tabloid      792   1224
//  3        Ledger       1224  792
//  4        10x14        720   1008
//  5        11x17        792   1224
//  6        12x18        864   1296
//  7        17x22 "C"    1224  1584
//  8        18x24        1296  1728
//  9        22x34 "D"    1584  2448
//  10       24x36        1728  2592
//  11       30x42        2160  3024
//  12       34x44 "E"    2448  3168
//  13       36x48        2592  3456
//  14       Statement    396   612
//  15       Executive    540   720
//  16       Folio        612   936
//  17       Quarto       610   780
//  18       A0           2384  3370
//  19       A1           1684  2384
//  20       A2           1190  1684
//  21       A3           842   1190
//  22       A4           595   842
//  23       A5           420   595
//  24       A6           298   420
//  25       B0           2835  4008
//  26       B1           2004  2835
//  27       B2           1417  2004
//  28       B3           1001  1417
//  29       B4           729   1032
//  30       B5           516   729
//
// The returned value is the previous setting of the media index.
//
bool
misc1_funcs::IFhcMedia(Variable *res, Variable *args, void*)
{
    int media;
    ARG_CHK(arg_int(args, 0, &media))

    res->type = TYP_SCALAR;
    res->content.value = HCstate.media;
    HCstate.media = media;
    return (OK);
}


//-------------------------------------------------------------------------
// Keyboard
//-------------------------------------------------------------------------

// (int) ReadKeymap(mapfile)
//
// Read and assert a keyboard mapping file, as generated from within
// Xic with the Key Map button in the Attributes Menu.  If the mapfile
// is not rooted, it is searched for in the current directory, the
// user's home directory, and in the library search path.  If success,
// 1 is returned, and the supplied mapping is installed.  Otherwise, 0
// is returned, and an error message is available from GetError.
//
bool
misc1_funcs::IFreadKeymap(Variable *res, Variable *args, void*)
{
    const char *filename;
    ARG_CHK(arg_string(args, 0, &filename))

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (filename && *filename) {
        char *estr = Kmap()->ReadMapFile(filename);
        if (!estr) {
            Errs()->add_error(
                "Error reading key mapping file: file not found.");
        }
        else if (strcmp(estr, "ok")) {
            Errs()->add_error(estr);
            Errs()->add_error("Error reading key mapping file:");
        }
        else
            res->content.value = 1;
        delete [] estr;
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Libraries
//-------------------------------------------------------------------------

// (int) OpenLibrary(path_name)
//
// This function will open the named library.  The name is either a
// full path to the library file, or the name of a library file to
// find in the search path.  Zero is returned on error, nonzero on
// success.
//
bool
misc1_funcs::IFopenLibrary(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))

    res->type = TYP_SCALAR;
    res->content.value = FIO()->OpenLibrary(FIO()->PGetPath(), name);
    return (OK);
}


// (int) CloseLibrary(path_name)
//
// This function will close the named library, or all user libraries
// if name is NULL.  The name can be a full path to a previously
// opened library file, or just the file name.  This function always
// returns 0.
//
bool
misc1_funcs::IFcloseLibrary(Variable *res, Variable *args, void*)
{
    const char *path;
    ARG_CHK(arg_string(args, 0, &path))

    FIO()->CloseLibrary(path, LIBuser);
    res->type = TYP_SCALAR;
    res->content.value = 1.0;
    return (OK);
}


//-------------------------------------------------------------------------
// Mode
//-------------------------------------------------------------------------

// (int) Mode(window, mode)
//
// This function switches Xic between physical and electrical modes,
// or switches subwindows between the two viewing modes.  The first
// argument is an integer 0-4, where 0 represents the main window, in
// which case the application mode is set, and 1-4 represent the
// subwindows, in which case the viewing mode of that subwindow is
// set.  The subwindow number is the same number as shown in the
// window title bar.
//
// The second argument can be a number or a string.  If a number and
// the nearest integer is not zero, the mode is electrical, otherwise
// physical.  If a string that starts with 'e' or 'E', the mode is
// electrical, otherwise physical.
//
// The return value is the new mode setting (0 or 1) or -1 if the
// indicated subwindow is not active.
//
bool
misc1_funcs::IFmode(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))
    int mode;
    if (args[1].type == TYP_SCALAR) {
        ARG_CHK(arg_int(args, 1, &mode))
    }
    else {
        const char *str;
        ARG_CHK(arg_string(args, 1, &str))
        mode = (str && (*str == 'e' || *str == 'E'));
    }

    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (win >= 0 && win < DSP_NUMWINS && DSP()->Window(win)) {
        XM()->SetMode(mode ? Electrical : Physical, win);
        res->content.value = DSP()->Window(win)->Mode();
    }
    return (OK);
}


// (int) CurMode(window)
//
// This function returns the current mode (physical or electrical) of
// the main window or subwindows.  The argument is an integer 0-4
// where 0 represents the main window (and the application mode) and
// 1-4 represent subwindow viewing modes.  The return value is 0 for
// physical mode, 1 for electrical mode, or -1 if the indicated
// subwindow does not exist.
//
bool
misc1_funcs::IFcurMode(Variable *res, Variable *args, void*)
{
    int win;
    ARG_CHK(arg_int(args, 0, &win))

    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (win == 0)
        res->content.value = DSP()->CurMode();
    else if (win > 0 && win < DSP_NUMWINS && DSP()->Window(win))
        res->content.value = DSP()->Window(win)->Mode();
    return (OK);
}


//-------------------------------------------------------------------------
// Prompt Line
//-------------------------------------------------------------------------

// (int) StuffText(string)
//
// The StuffText function stores the string in a buffer, which will be
// retrieved into the edit line on the next call to an editing
// function.  The edit will terminate immediately, as if the user has
// typed string.  Multiple lines can be stuffed, and will be retrieved
// in order.  This function must be issued before the function which
// invokes the editor.  Once a "stuffed" line is used, it is
// discarded.
//
bool
misc1_funcs::IFstuffText(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))

    if (!string)
        return (BAD);
    PL()->StuffEditBuf(string);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) TextCmd(string)
//
// This executes the command in string as if it were one of the "!"
// commands in Xic.  The leading !  is optional.
//
bool
misc1_funcs::IFtextCmd(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))

    if (!string || !*string)
        return (BAD);
    if (*string == '!')
        string++;

    // implicit "Commit"
    EditIf()->ulCommitChanges();

    XM()->TextCmd(string, false);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (string) GetLastPrompt()
//
// This function returns the most recent message that was shown on
// the prompt line, or would normally have been shown if Xic is not in
// graphics mode.  Although the prompt line may have been erased, the
// last message is available until the next message is sent to the
// prompt line.  The text on the prompt line while in edit mode is not
// saved and is not accessible with this function.  An empty string is
// returned if there is no current message.  This function never
// fails.
//
bool
misc1_funcs::IFgetLastPrompt(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string = lstring::copy(PL()->GetLastPrompt());
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


//-------------------------------------------------------------------------
// Scripts
//-------------------------------------------------------------------------

// (stringlist_handle) ListFunctions()
//
// This function will re-read all of the library files in the script
// search path, and returns a handle to a string list of the functions
// available from the libraries.
//
bool
misc1_funcs::IFlistFunctions(Variable *res, Variable*, void*)
{
    umenu *u = XM()->GetFunctionList();
    umenu::destroy(u);
    stringlist *s0 = SI()->GetSubfuncList();
    sHdl *hdl = new sHdlString(s0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (int) Exec(script)
//
// This function will execute a script.  The argument is a string
// giving the script name or path.  If the script is a file, it must
// have a ".scr" extension.  The ".scr" extension is optional in the
// argument.  If no path is given, the script will be opened from the
// search path or from the internal list of scripts read from the
// technology file or added with the !script command.  If a path is
// given, that file will be executed, if found.  It is also possible
// to reference a script which appears in a submenu of the User Menu
// by giving a modified path of the form "@@/libname/.../scriptname".
// The libname is the name of the script menu, the ...  indicates more
// script menus if the menu is more than one deep, and the last
// component is the name of the script.  The function returns the
// return status of the script, or 0 if the script was not found.
//
bool
misc1_funcs::IFexec(Variable *res, Variable *args, void*)
{
    const char *func;
    ARG_CHK(arg_string(args, 0, &func))

    if (!func || !*func)
        return (BAD);
    SIfile *sfp;
    res->type = TYP_SCALAR;
    res->content.value = 0;
    stringlist *sl;
    XM()->OpenScript(func, &sfp, &sl);
    if (sfp || sl) {
        SI()->Interpret(sfp, sl, 0, (siVariable*)res);
        if (sfp)
            delete sfp;
    }
    return (OK);
}


// (int) SetKey(password)
//
// Set the password to allow decryption of encrypted scripts.  Returns
// 1 on success, 0 if the password is no good.
//
bool
misc1_funcs::IFsetKey(Variable *res, Variable *args, void*)
{
    const char *pw;
    ARG_CHK(arg_string(args, 0, &pw))

    if (!pw)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    sCrypt cr;
    if (!cr.getkey(pw)) {
        char k[13];
        cr.readkey(k);
        SI()->SetKey(k);
        res->content.value = 1;
    }
    for (char *s = args[0].content.string; *s; s++)
        *s = '*';
    return (OK);
}


// (int) HasPython()
//
// This function returns 1 if the Python language support plug-in has
// been successfully loaded, 0 otherwise.
//
bool
misc1_funcs::IFhasPython(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = PyIf()->hasPy();
    return (OK);
}


// (int) RunPython(command [, arg, ...])
//
// Pass a command string to the Python interpreter for evaluation. 
// The first argument is a path to a Python script file.  Arguments
// that follow are concatenated and passed to the script.  Presently,
// only string and scalar type arguments are accepted.  The
// interpreter will have available the entire Xic scripting interface,
// though only the basic data types are useful.  The Python interface
// description provides information about the header lines needed to
// instantiate the interface to Xic from Python.
//
// This function exists only if the Python language support plug-in
// has been successfully loaded.  The function returns 1 on success, 0
// otherwise with an error message available from GetError.
//
bool
misc1_funcs::IFrunPython(Variable *res, Variable *args, void*)
{
    sLstr lstr;
    int i = 0;
    for ( ; ; i++) {
        if (args[i].type == TYP_ENDARG)
            break;
        if (i > MAXARGC) {
            Errs()->add_error("RunPython: too many arguments.");
            return (BAD);
        }
        if (args[i].type == TYP_NOTYPE || args[i].type == TYP_STRING) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add(args[i].content.string);
        }
        else if (args[i].type == TYP_SCALAR) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add_e(args[i].content.value, 15);
        }
        else if (args[i].type == TYP_ARRAY) {
            Errs()->add_error("RunPython: can't handle array argument.");
            return (BAD);
        }
        else if (args[i].type == TYP_ZLIST) {
            Errs()->add_error("RunPython: can't handle zlist argument.");
            return (BAD);
        }
        else if (args[i].type == TYP_LEXPR) {
            Errs()->add_error("RunPython: can't handle lexpr argument.");
            return (BAD);
        }
        else if (args[i].type == TYP_HANDLE) {
            Errs()->add_error("RunPython: can't handle handle argument.");
            return (BAD);
        }
        else {
            Errs()->add_error("RunPython: unknown argument type.");
            return (BAD);
        }
    }
    if (i < 1) {
        Errs()->add_error("RunPython: too few arguments.");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    res->content.value = 1;
    if (!PyIf()->run(lstr.string())) {
        Errs()->add_error("RunPython: command failed.");
        return (BAD);
    }
    return (OK);
}
 

// (int) RunPythonModFunc(module, function [, arg, ...])
//
// This function will call the Python interpreter, to execute the
// module function specified in the arguments.  The first argument is
// the name of the module, which must be known to Python.  The second
// argument is the name of the function within the module to evaluate. 
// Following are zero or more function arguments, as required by the
// function.
//
// This function exists only if the Python language support plug-in
// has been successfully loaded.  The function returns 1 on success, 0
// otherwise with an error message available from GetError.
//
bool
misc1_funcs::IFrunPythonModFunc(Variable *res, Variable *args, void*)
{
    const char *mod;
    ARG_CHK(arg_string(args, 0, &mod))
    const char *func;
    ARG_CHK(arg_string(args, 1, &func))

    if (!PyIf()->runModuleFunc(mod, func, res, args + 2)) {
        Errs()->add_error("RunPythonModFunc: command failed.");
        return (BAD);
    }
    return (OK);
}


// (int) ResetPython()
//
// Reset the Python interpreter.  It is not clear that a user would
// ever need to call this.
//
// This function exists only if the Python language support plug-in
// has been successfully loaded.  The function always returns 1.
//
bool
misc1_funcs::IFresetPython(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 1;
    PyIf()->reset();
    return (OK);
}


// (int) HasTcl()
//
// This function returns 1 if the Tcl language support plug-in was
// successfully loaded, 0 otherwise.
//
bool
misc1_funcs::IFhasTcl(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = TclIf()->hasTcl();
    return (OK);
}


// (int) HasTk()
//
// This function returns 1 if the Tcl with Tk language support plug-in
// was successfully loaded, 0 otherwise.
//
bool
misc1_funcs::IFhasTk(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = TclIf()->hasTk();
    return (OK);
}


// (int) RunTcl(command [, arg, ...])
//
// Pass a command string to the Tcl interpreter for evaluation.  The
// first argument is a path to a Tck/Tk script.  If both Tcl and Tk
// are available, the script file must have a .tcl or .tk extension. 
// If only Tcl is available, there is no extension requirement, but
// the file should contain only Tcl commands.  A Tcl script is
// executed linearly and returns.  A Tk script blocks, handling events
// until the last window is destroyed, at which time it returns.
//
// Arguments that follow are concatenated and passed to the script. 
// Presently, only string and scalar type arguments are accepted.  The
// interpreter will have available the entire Xic scripting interface,
// though only the basic data types are useful.  The Tcl/Tk interface
// description provides more information.
//
// This function exists only if the Tcl language support plug-in has
// been successfully loaded.  The function returns 1 on success, 0 The
// function returns 1 on success, 0 otherwise with an error message
// available from GetError.
//
bool
misc1_funcs::IFrunTcl(Variable *res, Variable *args, void*)
{
    sLstr lstr;
    int i = 0;
    for ( ; ; i++) {
        if (args[i].type == TYP_ENDARG)
            break;
        if (i > MAXARGC) {
            Errs()->add_error("RunTcl: too many arguments.");
            return (BAD);
        }
        if (args[i].type == TYP_NOTYPE || args[i].type == TYP_STRING) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add(args[i].content.string);
        }
        else if (args[i].type == TYP_SCALAR) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add_e(args[i].content.value, 15);
        }
        else if (args[i].type == TYP_ARRAY) {
            Errs()->add_error("RunTcl: can't handle array argument.");
            return (BAD);
        }
        else if (args[i].type == TYP_ZLIST) {
            Errs()->add_error("RunTcl: can't handle zlist argument.");
            return (BAD);
        }
        else if (args[i].type == TYP_LEXPR) {
            Errs()->add_error("RunTcl: can't handle lexpr argument.");
            return (BAD);
        }
        else if (args[i].type == TYP_HANDLE) {
            Errs()->add_error("RunTcl: can't handle handle argument.");
            return (BAD);
        }
        else {
            Errs()->add_error("RunTcl: unknown argument type.");
            return (BAD);
        }
    }
    if (i < 1) {
        Errs()->add_error("RunTcl: too few arguments.");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    res->content.value = 1;
    if (!TclIf()->run(lstr.string())) {
        Errs()->add_error("RunTcl: command failed.");
        return (BAD);
    }
    return (OK);
}
 

// (int) ResetTcl()
//
// Reset the Tcl/Tk interpreter.  It is not clear that a user would
// ever need to call this.
//
// This function exists only if the Tcl language support plug-in has
// been successfully loaded.  The function always returns 1.
//
bool
misc1_funcs::IFresetTcl(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 1;
    TclIf()->reset();
    return (OK);
}


// (int) HasGlobalVariable(globvar)
//
// Return true if the passed string is the name of a global variable
// currently in scope.  This is part of the exported global variable
// interface to Python and Tcl.
//
bool
misc1_funcs::IFhasGlobalVariable(Variable *res, Variable *args, void*)
{
    const char *varstr;
    ARG_CHK(arg_string(args, 0, &varstr))

    if (!varstr || !*varstr) {
        Errs()->add_error("HasGlobalVariable: null or empty variable name.");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    res->content.value = SIparse()->hasGlobalVariable(varstr);
    return (OK);
}


// (any) GetGlobalVar(globvar)
//
// Return the value of the global variable whose name is passed.  The
// function will generate a fatal error, halting the script, if the
// variable is not found, so one may need to check existence with
// HasGlobalVariable.  The return type is the type of the variable,
// which can be any known type.  This is for use in Python or Tcl
// scripts, providing access to the global variables maintained in the
// Xic script interpreter.
//
bool
misc1_funcs::IFgetGlobalVariable(Variable *res, Variable *args, void*)
{
    const char *varstr;
    ARG_CHK(arg_string(args, 0, &varstr))

    if (!varstr || !*varstr) {
        Errs()->add_error("GetGlobalVariable: null or empty variable name.");
        return (BAD);
    }
    if (SIparse()->getGlobalVariable(varstr, (siVariable*)res) != OK) {
        Errs()->add_error("GetGlobalVariable: action failed.");
        return (BAD);
    }
    return (OK);
}


// (int) SetGlobalVariable(globvar, value)
//
// Set the value of the global variable named in the first argument. 
// The function will generate a fatal error if the variable is not
// found, or the assignment fails due to type mismatch.  This is for
// use in Python or Tcl scripts, providing access to the global
// variables maintained in the Xic script interpreter.  Note that
// global variables can not be created from Python or Tcl, but values
// can be set with this function.  Global variables can be used to
// return data to a top-level native script from a Tcl or Python
// sub-script.
//
bool
misc1_funcs::IFsetGlobalVariable(Variable *res, Variable *args, void*)
{
    const char *varstr;
    ARG_CHK(arg_string(args, 0, &varstr))

    if (!varstr || !*varstr) {
        Errs()->add_error("SetGlobalVariable: null or empty variable name.");
        return (BAD);
    }

    if (SIparse()->setGlobalVariable(varstr, (siVariable*)args + 1) != OK) {
        Errs()->add_error("SetGlobalVariable: action failed.");
        return (BAD);
    }
    res->type = TYP_SCALAR;
    res->content.value = 1.0;
    return (OK);
}


//-------------------------------------------------------------------------
// Technology File
//-------------------------------------------------------------------------

// GetTechName(techname)
//
// This returns a string containing the current technology name,
// as set in the technology file with the Technology keyword.
//
bool
misc1_funcs::IFgetTechName(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string = (Tech()->TechnologyName() ?
        lstring::copy(Tech()->TechnologyName()) : lstring::copy(""));
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (string) GetTechExt()
//
// This returns a string containing the current technology file name
// extension.
//
bool
misc1_funcs::IFgetTechExt(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string = (Tech()->TechExtension() ?
        lstring::copy(Tech()->TechExtension()) : lstring::copy(""));
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) SetTechExt(extension)
//
// This sets the current technology file extension to the string
// argument.  It alters the name of new technology files created with
// the Save Tech button in the Attributes Menu.
//
bool
misc1_funcs::IFsetTechExt(Variable *res, Variable *args, void*)
{
    const char *ext;
    ARG_CHK(arg_string(args, 0, &ext))

    Tech()->SetTechExtension(ext);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) TechParseLine(line)
//
// This function will parse and process a line of text is if read from
// a technology file.  It can therefor modify parameters that are
// otherwise set in the technology file, after a technololgy file has
// been read, or if no technology file was read.
//
// However, there are limitations.
// 1.  There is no macro processing done on the line, it is parsed
//     verbatim, and macro directives will not be understood.
// 2.  There is no line continuation, all related text must appear in
//     the given string.
// 3.  The print driver block keywords are not recognized, nor are any
//     other block forms, such as device blocks for extraction.
// 4.  Layer block keywords are acceptable, however they must be given
//     in a special format, which is
//       [elec]layer layername layer_block_line...
//     i.e., the text must be prefaced by the layer/eleclayer keyword
//     followed by an existing layer name.  Note that new layers must be
//     created first, before calling this function.
//
// If the line is recognized and successfully processed, the function
// returns 1.  Otherwise, 0 is returned, and a message is available
// from GetError.
//
bool
misc1_funcs::IFtechParseLine(Variable *res, Variable *args, void*)
{
    const char *line;
    ARG_CHK(arg_string(args, 0, &line))

    res->type = TYP_SCALAR;
    res->content.value = Tech()->ParseLine(line);
    return (OK);
}


// (string) TechGetFkeyString(fkeynum)
//
// This function returns the string which encodes the functional
// assignment of a function key.  This is the same format as used in
// the technology file for the F1Key - F12Key keyword assignments. 
// The argument is an integer with value 1-12 representing the
// function key number.  The return value is a null string if the
// argument is out of range, or if no assignment has been made.
//
bool
misc1_funcs::IFtechGetFkeyString(Variable *res, Variable *args, void*)
{
    int num;
    ARG_CHK(arg_int(args, 0, &num))

    num--;
    res->type = TYP_STRING;
    if (num < 0 || num > 11) {
        res->content.string = 0;
        return (OK);
    }
    res->content.string = lstring::copy(Tech()->GetFkeyString(num));
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// TechSetFkeyString(fkeynum, string)
//
// This function sets the string which encodes the functional
// assignment of a function key.  This is the same format as used in
// the technology file for the F1Key - F12Key keyword assignments. 
// The first argument is an integer with value 1-12 representing the
// function key number.  The second argument is the string, or 0 to
// clear the assignment.  The return value is 1 if an assignment
// was made, 0 if the first argument is out of range.
//
bool
misc1_funcs::IFtechSetFkeyString(Variable *res, Variable *args, void*)
{
    int num;
    ARG_CHK(arg_int(args, 0, &num))
    const char *string;
    ARG_CHK(arg_string(args, 1, &string))

    num--;
    res->type = TYP_SCALAR;
    if (num < 0 || num > 11) {
        res->content.value = 0;
        return (OK);
    }
    res->content.value = 1;
    Tech()->SetFkeyString(num, string);
    return (OK);
}


//-------------------------------------------------------------------------
// Variables
//-------------------------------------------------------------------------

namespace {
    const char *pcvarmsg =
        "Set/Unset not allowed in PCell script, use PushVar/PopVar.";
}

// Set(name, string)
//
// The Set function allows variable name to be set to string as with
// the !set keyboard operation in Xic.  Some variables, such as the
// search paths, directly affect Xic operation.  The Set function can
// also set arbitrary variables, which may be useful to the script
// programmer.  To set a variable, both arguments should be strings.
// If the second argument is the constant zero (0, not "0") or a null
// string, the variable will be unset if set.  As with !set, forms
// like $(name) are expanded.  If name matches the name of a
// previously set variable, that variable's value string replaces the
// form.  Otherwise, if name matches an environment variable, the
// environment variable text replaces the form.
//
// The Set function will permanently change the variable value.  See
// the PushSet function for an alternative.
//
bool
misc1_funcs::IFset(Variable*, Variable *args, void*)
{
    if (SIlcx()->doingPCell()) {
        Errs()->add_error(pcvarmsg);
        return (BAD);
    }

    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    if (!name || !*name)
        return (BAD);
    char *string;
    if (args[1].type == TYP_STRING || args[1].type == TYP_NOTYPE) {
        string = args[1].content.string;
        if (!string)
            CDvdb()->clearVariable(name);
        else
            CDvdb()->setVariable(name, string);
        return (OK);
    }
    if (args[1].type == TYP_SCALAR) {
        if (args[1].content.value == 0)
            CDvdb()->clearVariable(name);
        else
            return (BAD);
    }
    return (OK);
}


// Unset(name)
//
// This function will unset the variable.  No action is taken if the
// variable is not already set.  This is equivalent to Set(name, 0).
//
bool
misc1_funcs::IFunset(Variable*, Variable *args, void*)
{
    if (SIlcx()->doingPCell()) {
        Errs()->add_error(pcvarmsg);
        return (BAD);
    }

    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    if (!name || !*name)
        return (BAD);
    CDvdb()->clearVariable(name);
    return (OK);
}


// PushSet(name, value)
//
// This function is similar to Set, however the previous value is
// stored internally, and can be restored with PopSet.  In addition,
// all variables set (or unset) with PushSet are reverted to original
// values when the script exits, thus avoiding permanent changes. 
// There can be arbitrarily many PushSet and PopSet operations on a
// variable.
//
bool
misc1_funcs::IFpushSet(Variable*, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    if (!name || !*name)
        return (BAD);
    char *string = 0;
    if (args[1].type == TYP_STRING || args[1].type == TYP_NOTYPE)
        string = args[1].content.string;
    else if (args[1].type == TYP_SCALAR) {
        if (args[1].content.value != 0)
            return (BAD);
    }
    else
        return (BAD);
    SIlcx()->pushVar(name, string);
    return (OK);
}


// PopSet(name)
//
// This reverts a variable set with PushSet to its previous state.  If
// the variable has not been set (or unset) with PushSet, no action is
// taken.
//
bool
misc1_funcs::IFpopSet(Variable*, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    if (!name || !*name)
        return (BAD);
    SIlcx()->popVar(name);
    return (OK);
}


// (string) SetExpand(string, use_env)
//
// This function returns a copy of string which expands variable
// references in the form $(word) in string.  The word is expected to
// be a variable previously set with the Set() function or !set
// command.  The value of the variable replaces the reference in the
// returned string.  If the integer use_env is nonzero, variables
// found in the environment will also be substituted.
//
bool
misc1_funcs::IFsetExpand(Variable *res, Variable *args, void*)
{
    const char *string;
    ARG_CHK(arg_string(args, 0, &string))
    bool use_env;
    ARG_CHK(arg_boolean(args, 1, &use_env))

    res->type = TYP_STRING;
    res->content.string = CDvdb()->expand(string, use_env);
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (string) Get(name)
//
// The Get function returns a string containing the value of name,
// which has been previously set with the Set command, or otherwise
// from within Xic.  A NULL string is returned if the named variable
// has not been set.
//
bool
misc1_funcs::IFget(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))

    if (!name)
        return (BAD);
    while (isspace(*name))
        name++;
    if (!*name)
        return (BAD);
    const char *string = CDvdb()->getVariable(name);
    res->type = TYP_STRING;
    res->content.string = (string ? lstring::copy(string) : 0);
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


//-------------------------------------------------------------------------
// Xic Version
//-------------------------------------------------------------------------

// (string) VersionString()
//
// This returns a string giving the current Xic version in a form like
// "2.4.53".
//
bool
misc1_funcs::IFversionString(Variable *res, Variable*, void*)
{
    res->type = TYP_STRING;
    res->content.string = lstring::copy(XM()->VersionString());
    res->flags |= VF_ORIGINAL;
    return (OK);
}

