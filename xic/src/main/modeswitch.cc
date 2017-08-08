
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

#include "config.h"  // for HAVE_LOCAL_ALLOCATOR
#include "main.h"
#include "cvrt.h"
#include "editif.h"
#include "drcif.h"
#include "extif.h"
#include "scedif.h"
#include "cd_celldb.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "dsp_color.h"
#include "fio.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_spt.h"
#include "si_interp.h"
#include "layertab.h"
#include "promptline.h"
#include "tech.h"
#include "select.h"
#include "events.h"
#include "menu.h"
#include "pushpop.h"
#include "file_menu.h"
#include "cell_menu.h"
#include "view_menu.h"
#include "attr_menu.h"
#include "cvrt_menu.h"
#include "miscutil/filestat.h"
#ifdef HAVE_LOCAL_ALLOCATOR
#include "malloc/local_malloc.h"
#endif


// Switch between physical and electrical modes.  If wnum is larger than
// zero, switch the mode of the corresponding subwindow.
//
void
cMain::SetMode(DisplayMode mode, int wnum)
{
    if (!ScedIf()->hasSced())
        return;
    if (mode == Electrical && DSP()->MainWdesc()->DbType() != WDcddb)
        return;
    if (wnum != 0) {
        WindowDesc *wd = DSP()->Window(wnum);
        if (wd) {
            wd->SetSubwinMode(mode);
            UpdateCursor(wd, GetCursor());
        }
        return;
    }
    if (mode == DSP()->CurMode())
        return;
    if (CDvdb()->getVariable(VA_LockMode) != 0)
        return;

    PL()->ErasePrompt();
    DSPmainWbag(HCupdate(&Tech()->HC(), 0))

    // If in hardcopy mode, switch back temporarily, transient=false
    // to reset colors in ldescs.
    //
    int save_drvr = Tech()->HcopyDriver();
    if (DSP()->DoingHcopy())
        HCswitchMode(false, false, Tech()->HcopyDriver());

    DrcIf()->cancelNext();              // cancel DRC 'next' mode
    Selections.deselectTypes(CurCell(), 0); // clear the old selection queue
    DSP()->ShowTerminals(ERASE);        // need this for subwindows
    DSP()->ClearWindowMarks();          // terminals, etc
    if (DSP()->DoingHcopy())
        HCkillFrame();
    CDcbin cbin(DSP()->CurCellName());
    if (cbin.cellname() == DSP()->TopCellName())
        EditIf()->assignGlobalProperties(&cbin);
    DSP()->ClearViews();
    ScedIf()->PopUpNodeMap(0, MODE_OFF);
    XM()->PopUpLayerPalette(0, MODE_OFF, false, 0);

    ScedIf()->assertSymbolic(false);

    Menu()->CleanupBeforeModeSwitch();
    hist().saveCurrent();
    if (DSP()->CurMode() == Physical)
        DSP()->SetCurMode(Electrical);
    else
        DSP()->SetCurMode(Physical);
    hist().assertSaved();
    Menu()->SwitchMenu();
    DSPmainWbag(HCsetFormat(Tech()->HC().format))

    EditIf()->ulListBegin(false, false);
    LT()->SetCurLayer(0);

    XM()->SetLayerSpecificSelections(false);
    EditIf()->setCurTransform(0, false, false, GEO()->curTx()->magn());
    EditIf()->setArrayParams(iap_t());
    DSP()->SetShowTerminals(false);
    ScedIf()->assertSymbolic(true);

    DSPmainDraw(SetBackground(DSP()->Color(BackgroundColor)))
    DSPmainDraw(SetWindowBackground(DSP()->Color(BackgroundColor)))

    if (DSP()->CurMode() == Physical) {
        DSPmainDraw(SetGhostColor(DSP()->Color(PhysGhostColor)))
        DrcIf()->initRules();
        if (cbin.cellname()) {
            EditIf()->assertGlobalProperties(&cbin);
            SetRulers(cbin.phys(), cbin.elec());
        }
    }
    else {
        DSPmainDraw(SetGhostColor(DSP()->Color(ElecGhostColor)))
        if (cbin.cellname()) {
            EditIf()->assertGlobalProperties(&cbin);
            SetRulers(cbin.elec(), cbin.phys());
        }
    }

    if (DSP()->DoingHcopy()) {
        LT()->InitLayerTable();
        HCswitchMode(true, false, save_drvr);
    }

    if (CurCell(true)) {
        CurCell(true)->fixBBs();
        if (DSP()->CurMode() == Electrical) {
            ScedIf()->connectAll(false);
            if (ScedIf()->showingDots() != DotsNone)
                ScedIf()->recomputeDots();
        }
    }

    LT()->InitLayerTable();
    CDl *cld = hist().currentLd();
    if (cld)
        LT()->SetCurLayer(cld);
    LT()->ShowLayerTable();
    DSP()->MainWdesc()->Redisplay(0);

    Menu()->InitAfterModeSwitch(0);
    PopUpCells(0, MODE_UPD);
    if (!TreeCaptive()) {
        // If the tree is under control of the Cells panel, don't
        // update here.

        PopUpTree(0, MODE_UPD, 0,
            DSP()->CurMode() == Physical ? TU_PHYS : TU_ELEC);
    }
    PopUpLayerParamEditor(0, MODE_UPD, 0, 0);
    Cvt()->PopUpHierarchies(0, MODE_UPD);
    EditIf()->prptyRelist();
    EditIf()->PopUpTransform(0, MODE_UPD, 0, 0);
    DSPmainWbag(PopUpGrid(0, MODE_UPD))
    ExtIf()->postModeSwitchCallback();
    ShowParameters();
    PopUpColor(0, MODE_UPD);
    UpdateCursor(DSP()->MainWdesc(), GetCursor());
    DSP()->MainWdesc()->UpdateProxy();
}


void
cMain::ClearAltModeUndo()
{
    hist().clearHist();
}


// Notes:
// 1. It is possible to switch to chd mode from physical mode only.
// 2. In chd mode, it is impossible to switch to electrical mode.

void
cMain::SetHierDisplayMode(const char *cxname, const char *cellname,
    const BBox *BB)
{
    if (DSP()->CurMode() != Physical)
        return;
    if (cxname && *cxname) {
        EV()->InitCallback();
        Menu()->HideButtonMenu(true);
        Menu()->DisableMainMenuItem("file", MenuOPEN, true);
        Menu()->DisableMainMenuItem("file", MenuFSEL, true);
        Menu()->DisableMainMenuItem("cell", MenuPUSH, true);
        Menu()->DisableMainMenuItem("cell", MenuPOP, true);
        Menu()->DisableMainMenuItem("cell", MenuSTABS, true);

        Menu()->DisableMainMenuItem("edit", 0, true);
        Menu()->DisableMainMenuItem("mod", 0, true);

        Menu()->DisableMainMenuItem("view", MenuSCED, true);
        Menu()->DisableMainMenuItem("view", MenuPEEK, true);
        Menu()->DisableMainMenuItem("view", MenuCSECT, true);
        Menu()->DisableMainMenuItem("view", MenuINFO, true);

        Menu()->DisableMainMenuItem("attr", MenuCNTXT, true);
        Menu()->DisableMainMenuItem("attr", MenuPROPS, true);

        Menu()->DisableMainMenuItem("conv", MenuIMPRT, true);
        Menu()->DisableMainMenuItem("conv", MenuEXPRT, true);
        Menu()->DisableMainMenuItem("conv", MenuTXTED, true);

        Menu()->DisableMainMenuItem("drc", 0, true);
        Menu()->DisableMainMenuItem("ext", 0, true);

        hist().saveCurrent();
        DSP()->MainWdesc()->SetHierDisplayMode(cxname, cellname, BB);
    }
    else {
        hist().assertSaved();
        DSP()->MainWdesc()->SetHierDisplayMode(0, 0, 0);

        Menu()->HideButtonMenu(false);

        Menu()->DisableMainMenuItem("file", MenuOPEN, false);
        Menu()->DisableMainMenuItem("file", MenuFSEL, false);
        Menu()->DisableMainMenuItem("cell", MenuPUSH, false);
        Menu()->DisableMainMenuItem("cell", MenuPOP, false);
        Menu()->DisableMainMenuItem("cell", MenuSTABS, false);

        Menu()->DisableMainMenuItem("edit", 0, false);
        Menu()->DisableMainMenuItem("mod", 0, false);

        Menu()->DisableMainMenuItem("view", MenuSCED, false);
        Menu()->DisableMainMenuItem("view", MenuPEEK, false);
        Menu()->DisableMainMenuItem("view", MenuCSECT, false);
        Menu()->DisableMainMenuItem("view", MenuINFO, false);

        Menu()->DisableMainMenuItem("attr", MenuCNTXT, false);
        Menu()->DisableMainMenuItem("attr", MenuPROPS, false);

        Menu()->DisableMainMenuItem("conv", MenuIMPRT, false);
        Menu()->DisableMainMenuItem("conv", MenuEXPRT, false);
        Menu()->DisableMainMenuItem("conv", MenuTXTED, false);

        Menu()->DisableMainMenuItem("drc", 0, false);
        Menu()->DisableMainMenuItem("ext", 0, false);

        EditIf()->setEditingMode(!CurCell() || !CurCell()->isImmutable());
    }
    PopUpSymTabs(0, MODE_OFF);
    PopUpCells(0, MODE_UPD);
    if (!TreeCaptive()) {
        // If the tree is under control of the Cells panel, don't
        // update here.

        PopUpTree(0, MODE_UPD, cellname,
            DSP()->CurMode() == Physical ? TU_PHYS : TU_ELEC);
    }
    Cvt()->PopUpAuxTab(0, MODE_OFF);
    Cvt()->PopUpFiles(0, MODE_UPD);
    Cvt()->PopUpLibraries(0, MODE_UPD);
}


void
cMain::SetSymbolTable(const char *name)
{
    const char *present_tabname = CDcdb()->tableName();
    CDcdb()->switchTable(name);
    const char *new_tabname = CDcdb()->tableName();

    if (new_tabname != present_tabname) {
        // Switch back temporarily and clear anything that refers to
        // the current cell.
        CDcdb()->switchTable(present_tabname);

        CDcdb()->setTableCellname(DSP()->CurCellName());

        switch (CheckModified(false)) {
        case CmodAborted:
            PL()->ShowPrompt("Command ABORTED.");
            delete [] name;
            return;
        case CmodFailed:
            PL()->ShowPrompt("Error: write FAILED, command aborted.");
            delete [] name;
            return;
        case CmodOK:
        case CmodNoChange:
            break;
        }
        EV()->InitCallback();
        ClearReferences(true);

        DSP()->SetCurCellName(0);
        DSP()->SetTopCellName(0);
        CDcdb()->switchTable(new_tabname);

        EditCell(CDcdb()->tableCellName(), false);
        PopUpSymTabs(0, MODE_UPD);
        Cvt()->PopUpAuxTab(0, MODE_UPD);
    }
}


void
cMain::ClearSymbolTable()
{
    switch (CheckModified(false)) {
    case CmodAborted:
        PL()->ShowPrompt("Command ABORTED.");
        return;
    case CmodFailed:
        PL()->ShowPrompt("Error: write FAILED, command aborted.");
        return;
    case CmodOK:
    case CmodNoChange:
        break;
    }
    EV()->InitCallback();
    ClearReferences(true);

    DSP()->SetCurCellName(0);
    DSP()->SetTopCellName(0);
    CDcdb()->destroyTable(true);

    EditCell(CDcdb()->tableCellName(), false);
    PopUpSymTabs(0, MODE_UPD);
    Cvt()->PopUpAuxTab(0, MODE_UPD);
}


// Clear lists and state that may contain references to cells in
// memory, call before operations which may remove or overwrite memory
// cells, or change context.  This is called in Push/Pop with toplevel
// false.
//
// If clearing is true, the cell is being cleared for overwrite as a
// layout file is being read.  In this case, skip recomputation of the
// BB and reflection as this can cause trouble.  Also skip adding
// global properties.  This applies only when toplevel is true.
//
void
cMain::ClearReferences(bool toplevel, bool clearing)
{
    if (toplevel) {
        EditIf()->ulListFinalize(false);

        if (!clearing) {
            // Recompute the BB.
            CDs *cursd = CurCell(true);
            if (cursd) {
                cursd->computeBB();
                cursd->reflect();
            }
        }
    }
    DrcIf()->cancelNext();
    Selections.removeTypes(CurCell(), 0);
    DSP()->ClearWindowMarks();
    if (DSP()->DoingHcopy())
        HCkillFrame();
    if (toplevel) {
        if (!clearing && DSP()->CurCellName() &&
                DSP()->CurCellName() == DSP()->TopCellName()) {
            // Assign global properties to the old cell, so if it is ever
            // edited again, the properties will be there.  This puts the
            // plot and iplot lists in the global properties.
            //
            CDcbin cbin(DSP()->CurCellName());
            EditIf()->assignGlobalProperties(&cbin);
        }
        ScedIf()->clearPlots();

        ClearAltModeUndo();
        ScedIf()->clearDots();
        PP()->ClearContext(clearing);
        DSP()->ClearViews();
        DSP()->MainWdesc()->ClearProxy();
    }
}


#ifdef HAVE_LOCAL_ALLOCATOR
namespace {
    void free_cb(size_t sz)
    {
        PL()->ShowPromptV("Memory blocks deallocated: %u", sz);
    }
}
#endif


// Clear the named cell from the database, or clear the entire database
// if name is null or empty.  VERY DANGEROUS
//
void
cMain::Clear(const char *name)
{
    if (!name || !*name) {
        switch (CheckModified(false)) {
        case CmodAborted:
            PL()->ShowPrompt("Command ABORTED.");
            return;
        case CmodFailed:
            PL()->ShowPrompt("Error: write FAILED, command aborted.");
            return;
        case CmodOK:
        case CmodNoChange:
            break;
        }
        // clear anything that refers to current cell
        EV()->InitCallback();
#ifdef HAVE_LOCAL_ALLOCATOR
        Memory()->register_free_talk(free_cb);
#endif
        ClearReferences(true);
        EditIf()->ulListBegin(true, false);
        SI()->UpdateCell(0);

        CDcdb()->clearTable();

        CD()->CompactPrptyTab();
        CD()->CompactGroupTab();
        CD()->ClearStringTables();
#ifdef HAVE_LOCAL_ALLOCATOR
        Memory()->register_free_talk(0);
#endif
        FIOreadPrms prms;
        EditCell(XM()->DefaultEditName(), false, &prms);
        WDgen gen(WDgen::SUBW, WDgen::CDDB);
        WindowDesc *wd;
        while ((wd = gen.next()) != 0)
            XM()->Load(wd, Tstring(DSP()->CurCellName()), 0, 0);
    }
    else {
        CDcbin cbin;
        if (!CDcdb()->findSymbol(name, &cbin)) {
            PL()->ShowPromptV("Symbol %s not found in database.", name);
            return;
        }

        // Clear top-level empty parts.
        if (cbin.phys() && cbin.phys()->isEmpty() &&
                !cbin.phys()->isSubcell()) {
            delete cbin.phys();
            cbin.setPhys(0);
        }
        if (cbin.elec() && cbin.elec()->isEmpty() &&
                !cbin.elec()->isSubcell()) {
            delete cbin.elec();
            cbin.setElec(0);
        }

        if (cbin.phys() || cbin.elec()) {
            if (cbin.isSubcell()) {
                PL()->ShowPromptV(
                    "Symbol %s is called by another cell, can't clear.",
                    name);
                return;
            }
#ifdef HAVE_LOCAL_ALLOCATOR
            Memory()->register_free_talk(free_cb);
#endif

            bool clear_current =
                (!DSP()->CurCellName() || !DSP()->TopCellName() ||
                cbin.cellname() == DSP()->TopCellName());

            if (clear_current) {
                EV()->InitCallback();
                ClearReferences(true);
                EditIf()->ulListBegin(true, false);
            }
            if (cbin.phys())
                SI()->UpdateCell(cbin.phys());
            if (cbin.elec())
                SI()->UpdateCell(cbin.elec());

            CD()->Clear(CD()->CellNameTableFind(name));
#ifdef HAVE_LOCAL_ALLOCATOR
            Memory()->register_free_talk(0);
#endif
        }

        // The cell and top cell will either be gone, or both ok.
        if (!DSP()->TopCellName()) {
            FIOreadPrms prms;
            EditCell(XM()->DefaultEditName(), false, &prms);
        }
        else {
            EditIf()->ulListBegin(false, false);
            for (int i = 1; i < DSP_NUMWINS; i++) {
                if (DSP()->Window(i) && !DSP()->Window(i)->CurCellName())
                    DSP()->Window(i)->SetCurCellName(DSP()->CurCellName());
            }
        }
        WDgen gen(WDgen::SUBW, WDgen::CDDB);
        WindowDesc *wd;
        while ((wd = gen.next()) != 0) {
            if (!CDcdb()->findCell(wd->CurCellName(), wd->Mode()))
                XM()->Load(wd, Tstring(DSP()->CurCellName()), 0, 0);
        }
    }
    PopUpCells(0, MODE_UPD);
    PopUpTree(0, MODE_UPD, 0,
        DSP()->CurMode() == Physical ? TU_PHYS : TU_ELEC);
}


// This clears all symbol tables (including the library devices), and
// reverts the layer database.  If clear_tech, layers read from the
// tech file will be cleared, otherwise the layer database is reverted
// to the state just after the tech file was read.
//
// This does NOT begin editing a new cell.  This is mostly for server
// mode.
//
void
cMain::ClearAll(bool clear_tech)
{
    EV()->InitCallback();

    PopUpCells(0, MODE_OFF);
    PopUpDebug(0, MODE_OFF);
    PopUpTree(0, MODE_OFF, 0, TU_CUR);
    PopUpSymTabs(0, MODE_OFF);
    Cvt()->PopUpFiles(0, MODE_OFF);
    Cvt()->PopUpGeometries(0, MODE_OFF);
    Cvt()->PopUpHierarchies(0, MODE_OFF);
    Cvt()->PopUpLibraries(0, MODE_OFF);
    Cvt()->PopUpAuxTab(0, MODE_OFF);

    filestat::delete_files();

#ifdef HAVE_LOCAL_ALLOCATOR
    Memory()->register_free_talk(free_cb);
#endif
    ClearReferences(true);
    DSP()->ClearUserMarks(0);
    EditIf()->ulListBegin(true, false);
    SI()->UpdateCell(0);
    spt_t::clearSpatialParameterTable(0);
    nametab::clearNametabs();
    GEO()->clearAll();
    FIO()->ClearAll();
    CD()->ClearAll(clear_tech);
    CDvdb()->revertToBackup(xm_var_bak);
#ifdef HAVE_LOCAL_ALLOCATOR
    Memory()->register_free_talk(0);
#endif
    Errs()->clear();

    DSP()->RedisplayAll(Physical);
    DSP()->RedisplayAll(Electrical);
    ShowParameters();

    if (clear_tech)
        LT()->InitElecLayers();
    Tech()->StdViaReset(clear_tech);

    // reopen device library
    FIO()->OpenLibrary(CDvdb()->getVariable(VA_LibPath),
        XM()->DeviceLibName());
}

// End of cMain functions


sModeSave::sModeSave()
{
    PhysMainWin = BBox(0, 0, 0, 0);
    ElecMainWin = BBox(0, 0, 0, 0);
    PhysMagn = 1.0;
    PhysCurCellName = 0;
    PhysTopCellName = 0;
    ElecCurCellName = 0;
    ElecTopCellName = 0;
    PhysCharWidth = CDphysDefTextWidth;
    PhysCharHeight = CDphysDefTextHeight;
    ElecCharWidth = CDelecDefTextWidth;;
    ElecCharHeight = CDelecDefTextHeight;
    PhysLd = 0;
    ElecLd = 0;
    EditState = 0;
}


void
sModeSave::saveCurrent()
{
    DisplayMode oldmode = DSP()->CurMode();

    // Note: CharcellWidth/Height aren't used globally, no need to save
    // these at present.

    cTfmStack stk;
    if (oldmode == Physical) {
        PhysMainWin = *DSP()->MainWdesc()->Window();
        PhysCurCellName = DSP()->CurCellName();
        PhysTopCellName = DSP()->TopCellName();
        PhysCharWidth = DSP()->PhysCharWidth();
        PhysCharHeight = DSP()->PhysCharHeight();
        stk.TPush();
        stk.TLoad(CDtfRegI0);
        stk.TCurrent(&PhysTf);
        stk.TPop();
        Tech()->SetPhysHcFormat(Tech()->HC().format);
        PhysMagn = GEO()->curTx()->magn();
        PhysLd = LT()->CurLayer();
    }
    else {
        ElecMainWin = *DSP()->MainWdesc()->Window();
        ElecCurCellName = DSP()->CurCellName();
        ElecTopCellName = DSP()->TopCellName();
        ElecCharWidth = DSP()->ElecCharWidth();
        ElecCharHeight = DSP()->ElecCharHeight();
        stk.TPush();
        stk.TLoad(CDtfRegI0);
        stk.TCurrent(&ElecTf);
        stk.TPop();
        Tech()->SetElecHcFormat(Tech()->HC().format);
        ElecLd = LT()->CurLayer();
    }
    EditIf()->popState(oldmode);
}


void
sModeSave::assertSaved()
{
    DisplayMode newmode = DSP()->CurMode();

    cTfmStack stk;
    if (newmode == Physical) {
        if (DSP()->CurCellName() == PhysCurCellName) {
            DSP()->SetTopCellName(PhysTopCellName);
            stk.TPush();
            stk.TLoadCurrent(&PhysTf);
            stk.TStore(CDtfRegI0);
            stk.TPop();
            DSP()->MainWdesc()->InitWindow(&PhysMainWin);
        }
        else {
            DSP()->SetTopCellName(DSP()->CurCellName());
            stk.TPush();
            stk.TStore(CDtfRegI0);
            stk.TPop();
            DSP()->MainWdesc()->CenterFullView();
        }
        DSP()->SetPhysCharWidth(PhysCharWidth);
        DSP()->SetPhysCharHeight(PhysCharHeight);
        Tech()->HC().format = Tech()->PhysHcFormat();
        sCurTx ct = *GEO()->curTx();
        ct.set_magn(PhysMagn);
        GEO()->setCurTx(ct);
    }
    else {
        if (DSP()->CurCellName() == ElecCurCellName) {
            DSP()->SetTopCellName(ElecTopCellName);
            stk.TPush();
            stk.TLoadCurrent(&ElecTf);
            stk.TStore(CDtfRegI0);
            stk.TPop();
            DSP()->MainWdesc()->InitWindow(&ElecMainWin);
        }
        else {
            DSP()->SetTopCellName(DSP()->CurCellName());
            stk.TPush();
            stk.TStore(CDtfRegI0);
            stk.TPop();
            DSP()->MainWdesc()->CenterFullView();
        }
        DSP()->SetElecCharWidth(ElecCharWidth);
        DSP()->SetElecCharHeight(ElecCharHeight);
        Tech()->HC().format = Tech()->ElecHcFormat();
        sCurTx ct = *GEO()->curTx();
        ct.set_magn(1.0);
        GEO()->setCurTx(ct);
    }
    EditIf()->pushState(newmode);
}


void
sModeSave::clearHist()
{
    PhysMainWin = BBox(0, 0, 0, 0);
    ElecMainWin = BBox(0, 0, 0, 0);
    PhysMagn = 1.0;
    PhysCurCellName = 0;
    PhysTopCellName = 0;
    ElecCurCellName = 0;
    ElecTopCellName = 0;
    PhysCharWidth = CDphysDefTextWidth;
    PhysCharHeight = CDphysDefTextHeight;
    ElecCharWidth = CDelecDefTextWidth;
    ElecCharHeight = CDelecDefTextHeight;
    cTfmStack stk;
    stk.TPush();
    stk.TCurrent(&PhysTf);
    stk.TCurrent(&ElecTf);
    stk.TPop();
    PhysLd = 0;
    ElecLd = 0;
    EditIf()->clearSaveState();
}


CDl *
sModeSave::currentLd()
{
    return (DSP()->CurMode() == Physical ? PhysLd : ElecLd);
}

