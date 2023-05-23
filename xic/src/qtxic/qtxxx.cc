
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

#include "main.h"
#include "cvrt.h"
#include "edit.h"
#include "sced.h"
#include "drc.h"
#include "ext.h"
#include "oa_if.h"


// qtasm.cc

// Exported function to pop up/down the tool.
//
void
cConvert::PopUpAssemble(GRobject, ShowMode)
{
}

// qtattri.cc

void
cMain::PopUpAttributes(GRobject, ShowMode)
{
}

// qtauxtab.cc

void
cConvert::PopUpAuxTab(GRobject, ShowMode)
{
}

// qtcells.cc

void
cMain::PopUpCells(GRobject, ShowMode)
{
}

// qtcflags.cc

void
cMain::PopUpCellFlags(GRobject, ShowMode, const stringlist*, int)
{
}

// qtcgdlist.cc

void
cConvert::PopUpGeometries(GRobject, ShowMode)
{
}

// qtchdcfg.cc

void
cConvert::PopUpChdConfig(GRobject, ShowMode, const char*, int, int)
{
}

// qtchdlist.cc

void
cConvert::PopUpHierarchies(GRobject, ShowMode)
{
}

// qtchdopen.cc

void
cConvert::PopUpChdOpen(GRobject, ShowMode,
    const char*, const char*, int, int,
    bool(*)(const char*, const char*, int, void*), void*)
{
}

void
cMain::PopUpDebugFlags(GRobject, ShowMode)
{
}


void
cMain::PopUpSelectInstances(CDol*)
{
}

CDol *
cMain::PopUpFilterInstances(CDol*)
{
    return (0);
}

void
cMain::PopUpCellFilt(GRobject, ShowMode, DisplayMode,
    void(*)(cfilter_t*, void*), void*)
{
}

CursorType
cMain::GetCursor()
{
    return ((CursorType)0);
}

void
cMain::UpdateCursor(WindowDesc*, CursorType, bool)
{
}

void
cMain::PopUpLayerParamEditor(GRobject, ShowMode, const char*, const char*)
{
}

void
cMain::PopUpTechWrite(GRobject, ShowMode)
{
}


void
cMain::SetNoToTop(bool)
{
}

void
cMain::SetLowerWinOffset(int)
{
}


// qtcv.cc

void
cConvert::PopUpConvert(GRobject, ShowMode, int,
    bool(*)(int, void*), void*)
{
}

// qtcvin.cc

void
cConvert::PopUpImport(GRobject, ShowMode,
    bool (*)(int, void*), void*)
{
}

void
cConvert::PopUpCompare(GRobject, ShowMode)
{
}

void
cConvert::PopUpPropertyFilter(GRobject, ShowMode)
{
}


// qtcvout.cc

void
cConvert::PopUpExport(GRobject, ShowMode, 
    bool (*)(FileType, bool, void*), void*)
{
}

// qtdebug.cc

void
cMain::PopUpDebug(GRobject, ShowMode)
{
}

bool
cMain::DbgLoad(MenuEnt*)
{
    return (false);
}

// qtdevs.cc

void
cSced::PopUpDevs(GRobject, ShowMode)
{
}

void
cSced::DevsEscCallback()
{
}

// qtdlgcb.cc

char *
cMain::GetCurFileSelection()
{ 
    return (0);
}

void
cMain::DisableDialogs()
{
}

// qtdvedit.cc

void
cSced::PopUpDevEdit(GRobject, ShowMode)
{
}


// qtextcmd.cc

void
cExt::PopUpExtCmd(GRobject, ShowMode, sExtCmd*,
    bool(*)(const char*, void*, bool, const char*, int, int),
    void*, int)
{
}

void
cExt::PopUpExtSetup(GRobject, ShowMode)
{
}

void
cExt::PopUpSelections(GRobject, ShowMode)
{
}

void
cExt::PopUpDevices(GRobject, ShowMode)
{
}

void
cExt::PopUpPhysTermEdit(GRobject, ShowMode, TermEditInfo*,
    void(*)(TermEditInfo*, CDsterm*), CDsterm*, int, int)
{
}

// qtextterm.cc

void
cSced::PopUpTermEdit(GRobject, ShowMode, TermEditInfo*,
    void(*)(TermEditInfo*, CDp*), CDp*, int, int)
{
}

void
cSced::PopUpSpiceIf(GRobject, ShowMode)
{
}

void
cSced::PopUpDots(GRobject, ShowMode)
{
}

// qtfiles.cc

void
cConvert::PopUpFiles(GRobject, ShowMode)
{
}


// qtflatten.cc

void
cEdit::PopUpFlatten(GRobject, ShowMode,
        bool(*)(const char*, bool, const char*, void*),
        void*, int, bool)
{
}

// qtlibs.cc

void
cConvert::PopUpLibraries(GRobject, ShowMode)
{
}

// qtlogo.cc

void
cEdit::PopUpLogo(GRobject, ShowMode)
{
}

// qtlpal.cc

void
cMain::PopUpLayerPalette(GRobject, ShowMode, bool, CDl*)
{
}

void
cEdit::PopUpLayerChangeMode(ShowMode)
{
}

// atltalias.cc

void
cMain::PopUpLayerAliases(GRobject, ShowMode)
{
}

// qtmem.cc

void
cMain::PopUpMemory(ShowMode)
{
}

// qtprpcedit.cc

void
cEdit::PopUpCellProperties(ShowMode)
{
}

void
cEdit::PopUpPolytextFont(GRobject, ShowMode)
{
}

void
cEdit::polytextExtent(const char*, int*, int*, int*)
{
}
PolyList *
cEdit::polytext(const char*, int, int, int)
{
    return (0);
}

// qtprpedit.cc

void
cEdit::PopUpProperties(CDo*, ShowMode, PRPmode)
{
}

void
cEdit::PopUpStdVia(GRobject, ShowMode, CDc*)
{
}

void
cEdit::PopUpEditSetup(GRobject, ShowMode)
{
}

void
cEdit::PopUpJoin(GRobject, ShowMode)
{
}

void
cEdit::PopUpLayerExp(GRobject, ShowMode)
{
}


PrptyText *
cEdit::PropertyResolve(int, int, CDo**)
{
    return (0);
}

void
cEdit::PropertyPurge(CDo*, CDo*)
{
}

PrptyText *
cEdit::PropertySelect(int)
{
    return (0);
}

PrptyText *
cEdit::PropertyCycle(CDp*, bool (*)(const CDp*), bool)
{
    return (0);
}

void
cEdit::RegisterPrptyBtnCallback(int(*)(PrptyText*))
{
}

// qtprpinfo.cc

void
cEdit::PopUpPropertyInfo(CDo*, ShowMode)
{
}

void
cEdit::PropertyInfoPurge(CDo*, CDo*)
{
}


// qtsim.cc

void
cSced::PopUpSim(SpType)
{
}


/* ----- */

void
cDRC::PopUpRules(GRobject, ShowMode)
{
}

void
cDRC::PopUpDrcLimits(GRobject, ShowMode)
{
}

void
cDRC::PopUpDrcRun(GRobject, ShowMode)
{
}

void
cDRC::PopUpRuleEdit(GRobject, ShowMode, DRCtype, const char*,
    bool(*)(const char*, void*), void*, const DRCtestDesc*)
{
}

void
cOAif::PopUpOAlibraries(GRobject, ShowMode)
{
}

void
cOAif::GetSelection(const char**, const char**)
{
}

void
cOAif::PopUpOAtech(GRobject, ShowMode, int, int)
{
}

void
cOAif::PopUpOAdefs(GRobject, ShowMode, int, int)
{
}

bool
cSced::PopUpNodeMap(GRobject, ShowMode, int)
{
}

void
cConvert::PopUpOasAdv(GRobject, ShowMode, int, int)
{
}

void
cEdit::PopUpPCellCtrl(GRobject, ShowMode)
{
}

struct PCellParam;
bool
cEdit::PopUpPCellParams(GRobject, ShowMode, PCellParam*,
    const char*, pcpMode)
{
}
