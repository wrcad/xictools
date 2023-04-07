
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
    bool(*callback)(const char*, void*), void *arg)
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

// qtcoord.cc

void
cMain::SetCoordMode(int x, int y, bool relative, bool snap)
{
}

// qtcursor.cc

void
cMain::PopUpCursor(GRobject caller, ShowMode mode)
{
}

// qtcv.cc

void
cConvert::PopUpConvert(GRobject caller, ShowMode mode, int inp_type,
    bool(*callback)(int, void*), void *arg)
{
}

// qtcvedit.cc

void
cConvert::PopUpParamEditor(GRobject caller, ShowMode mode, const char *msg,
    const char *string, void(*callback)(const char*), void(*downproc)())
{
}

// qtcvin.cc

void
cConvert::PopUpImportPrms(GRobject caller, ShowMode mode, int x, int y)
{
}

// qtcvout.cc

void
cConvert::PopUpExportPrms(GRobject caller, ShowMode mode, int x, int y)
{
}

// qtcvread.cc

void
cConvert::PopUpImport(GRobject caller, ShowMode mode, int x, int y,
    bool (*callback)(int, void*), void *arg)
{
}

// qtcvwrite.cc

void
cConvert::PopUpExport(GRobject caller, ShowMode mode, int x, int y,
    bool (*callback)(FileType, void*), void *arg)
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
cSced::PopUpDevs(ShowMode mode)
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

// qtdrc.cc

void
cDRC::PopUpRules(GRobject caller, ShowMode mode)
{
}

// qtdrclim.cc

void
cDRC::PopUpDrcLimits(GRobject caller, ShowMode mode, int x, int y)
{
}

// qtdvedit.cc

void
cSced::PopUpDevEdit(ShowMode mode)
{
}

// qtempty.cc

void
cEdit::PopUpEmpties(stringlist *list)
{
}

// qtextcmd.cc

void
cEXT::PopUpExtCmd(GRobject caller, ShowMode mode, s_excmd *cmd, int x, int y,
    bool (*action_cb)(const char*, void*, bool, const char*, int, int),
    void *action_arg, int depth)
{
}

// qtextedit.cc

void
cEXT::PopUpExtract(GRobject caller, ShowMode mode, const char *msg,
    const char *string, void(*callback)(const char*), void(*downproc)())
{
}

// qtextterm.cc

void
cEXT::PopUpTermEdit(ShowMode mode, te_info_t *tinfo,
    void(*action)(te_info_t*, void*), void *arg, int x, int y)
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
cEdit::PopUpFlatten(GRobject caller, ShowMode mode, int x, int y,
    bool (*callback)(const char*, bool, const char*, void*),
    void *arg, int depth, bool fmode)
{
}

// qtlibs.cc

void
cConvert::PopUpLibraries(GRobject caller, ShowMode mode)
{
}

// qtlogo.cc

void
cEdit::PopUpLogoFont(GRobject caller, ShowMode mode)
{
}

// qtlpal.cc

void
cMain::PopUpLayerPalette(GRobject caller, ShowMode mode, bool showinfo,
    CDl *ldesc)
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
cEdit::PopUpMergeControl(mitem_t *list)
{
    return (false);
}

// qtmerge2.cc

bool
cEdit::PopUpMergeControl2(ShowMode mode, mitem_t *mi)
{
    return (false);
}

// qtmodif.cc

PMretType
cEdit::PopUpModified(stringlist *list, bool(*saveproc)(const char*))
{
    return (PMok);
}

// qtnodmp.cc

void
cSced::PopUpNodeMap(ShowMode mode, int x, int y)
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

// qtprpedit.cc

void
cEdit::PopUpProperties(CDo *odesc, ShowMode mode)
{
}

Ptxt *
cEdit::PropertyResolve(int code, int offset, CDo **odp)
{
    return (0);
}

void
cEdit::PropertyPurge(CDo *odold, CDo *odnew)
{
}

Ptxt *
cEdit::PropertySelect(int which)
{
    return (0);
}

Ptxt *
cEdit::PropertyCycle(CDp *pd, bool obj, bool rev)
{
    return (0);
}

void
cEdit::RegisterPrptyBtnCallback(int(*cb)(Ptxt*))
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
cMain::PopUpSymTabs(GRobject caller, ShowMode mode, int x, int y)
{
}

// qttree.cc

void
cMain::PopUpTree(GRobject caller, ShowMode mode, const char *root,
    const char *oldroot)
{
}

// qtxform.cc

void
cEdit::PopUpTransform(GRobject caller, ShowMode mode, int x, int y,
    bool (*callback)(const char*, bool, const char*, void*), void *arg)
{
}

