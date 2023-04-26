
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


//XXX
extern bool load_qtmain;
void
phony_balogna()
{
    load_qtmain = true;
}

// qtasm.cc

// Exported function to pop up/down the tool.
//
void
cConvert::PopUpAssemble(GRobject caller, ShowMode mode)
{
}

// qtattri.cc

void
cMain::PopUpAttributes(GRobject caller, ShowMode mode)
{
}

// qtauxtab.cc

void
cConvert::PopUpAuxTab(GRobject caller, ShowMode mode)
{
}

// qtcells.cc

void
cMain::PopUpCells(GRobject caller, ShowMode mode)
{
}

// qtcflags.cc

void
cMain::PopUpCellFlags(GRobject caller, ShowMode mode, const stringlist *list,
    int dmode)
{
}

// qtcgdlist.cc

void
cConvert::PopUpGeometries(GRobject caller, ShowMode mode)
{
}

// qtchdcfg.cc

void
cConvert::PopUpChdConfig(GRobject caller, ShowMode mode,
    const char *chdname, int x, int y)
{
}

// qtchdlist.cc

void
cConvert::PopUpHierarchies(GRobject caller, ShowMode mode)
{
}

// qtchdopen.cc

void
cConvert::PopUpChdOpen(GRobject caller, ShowMode mode,
    const char *prompt_str, const char *init_str, int x, int y,
    bool(*callback)(const char*, const char*, int, void*), void *arg)
{
}

// qtcolor.cc

void
cMain::PopUpColor(GRobject caller, ShowMode mode)
{
}

void
cMain::ColorTimerInit()
{
}

void
cMain::PopUpDebugFlags(GRobject, ShowMode)
{
}

char *
cMain::SaveFileDlg(const char*, const char*)
{
    return (0);
}

char *
cMain::OpenFileDlg(const char*, const char*)
{
    return (0);
}

void
cMain::PopUpFileSel(const char*, void(*)(const char*, void*), void*)
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
cMain::FixupColors(void*)
{
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

sLcb *
cMain::PopUpLayerEditor(GRobject)
{
    return (0);
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
cConvert::PopUpConvert(GRobject caller, ShowMode mode, int inp_type,
    bool(*callback)(int, void*), void *arg)
{
}

// qtcvin.cc

void
cConvert::PopUpImport(GRobject caller, ShowMode mode,
    bool (*callback)(int, void*), void *arg)
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
cConvert::PopUpExport(GRobject caller, ShowMode mode, 
    bool (*callback)(FileType, bool, void*), void *arg)
{
}

// qtdebug.cc

void
cMain::PopUpDebug(GRobject caller, ShowMode mode)
{
}

bool
cMain::DbgLoad(MenuEnt *ent)
{
    return (false);
}

// qtdevs.cc

void
cSced::PopUpDevs(GRobject caller, ShowMode mode)
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
cSced::PopUpDevEdit(GRobject caller, ShowMode mode)
{
}

// qtempty.cc

void
cConvert::PopUpEmpties(stringlist *list)
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
cSced::PopUpTermEdit(GRobject, ShowMode mode, TermEditInfo *tinfo,
    void(*action)(TermEditInfo*, CDp*), CDp *prp, int x, int y)
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
cConvert::PopUpFiles(GRobject caller, ShowMode mode)
{
}

// qtfillp.cc

void
cMain::PopUpFillEditor(GRobject caller, ShowMode mode)
{
}

void
cMain::FillLoadCallback(LayerFillData *dd, CDl *ld)
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
cConvert::PopUpLibraries(GRobject caller, ShowMode mode)
{
}

// qtlogo.cc

void
cEdit::PopUpLogo(GRobject caller, ShowMode mode)
{
}

// qtlpal.cc

void
cMain::PopUpLayerPalette(GRobject caller, ShowMode mode, bool showinfo,
    CDl *ldesc)
{
}

void
cEdit::PopUpLayerChangeMode(ShowMode)
{
}

// atltalias.cc

void
cMain::PopUpLayerAliases(GRobject caller, ShowMode mode)
{
}

// qtmem.cc

void
cMain::PopUpMemory(ShowMode mode)
{
}

// qtmerge.cc

bool
cConvert::PopUpMergeControl(ShowMode mode, mitem_t *list)
{
    return (false);
}

// qtmodif.cc

PMretType
cEdit::PopUpModified(stringlist*, bool(*)(const char*))
{
    return (PMok);
}

void
cEdit::PopUpPCellCtrl(GRobject, ShowMode)
{
}

bool
cEdit::PopUpPCellParams(GRobject, ShowMode, PCellParam*, const char*,
    pcpMode)
{
}

// qtnodmp.cc

bool
cSced::PopUpNodeMap(GRobject caller, ShowMode mode, int node)
{
}

// qtoasis.h

void
cConvert::PopUpOasAdv(GRobject caller, ShowMode mode, int x, int y)
{
}

// qtplace.cc

void
cEdit::PopUpPlace(ShowMode mode, bool noprompt)
{
}

// qtprpcedit.cc

void
cEdit::PopUpCellProperties(ShowMode mode)
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
cEdit::PopUpProperties(CDo *odesc, ShowMode mode, PRPmode activ)
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
cEdit::PropertyResolve(int code, int offset, CDo **odp)
{
    return (0);
}

void
cEdit::PropertyPurge(CDo *odold, CDo *odnew)
{
}

PrptyText *
cEdit::PropertySelect(int which)
{
    return (0);
}

PrptyText *
cEdit::PropertyCycle(CDp *pd, bool (*checkfunc)(const CDp*), bool rev)
{
    return (0);
}

void
cEdit::RegisterPrptyBtnCallback(int(*cb)(PrptyText*))
{
}

// qtprpinfo.cc

void
cEdit::PopUpPropertyInfo(CDo *odesc, ShowMode mode)
{
}

void
cEdit::PropertyInfoPurge(CDo *odold, CDo *odnew)
{
}

// qtsel.cc

void
cMain::PopUpSelectControl(GRobject caller, ShowMode mode)
{
}

// qtsim.cc

void
cSced::PopUpSim(SpType status)
{
}

// qtstab.cc

void
cMain::PopUpSymTabs(GRobject caller, ShowMode mode)
{
}

// qttree.cc

void
cMain::PopUpTree(GRobject caller, ShowMode mode, const char *root,
    TreeUpdMode dmode, const char *oldroot)
{
}

// qtxform.cc

void
cEdit::PopUpTransform(GRobject caller, ShowMode mode,
    bool (*callback)(const char*, bool, const char*, void*), void *arg)
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

