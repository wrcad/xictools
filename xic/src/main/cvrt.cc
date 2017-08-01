
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
 $Id: cvrt.cc,v 5.48 2016/03/02 00:39:45 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "cvrt.h"
#include "cd_celldb.h"
#include "fio.h"
#include "fio_alias.h"
#include "fio_cvt_base.h"
#include "editif.h"
#include "dsp_inlines.h"
#include "ghost.h"
#include "events.h"
#include "promptline.h"
#include "menu.h"
#include "cvrt_menu.h"
#include "pathlist.h"
#include "filestat.h"


cConvert *cConvert::instancePtr = 0;

cConvert::cConvert()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cConvert already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    cvt_log_fp = 0;
    cvt_show_log = false;

    cvt_cv_filename = 0;
    cvt_cv_win = false;
    cvt_cv_clip = false;
    cvt_cv_flat = false;

    cvt_wr_filename = 0;
    cvt_wr_win = false;
    cvt_wr_clip = false;
    cvt_wr_flat = false;

    setupInterface();
    setupVariables();
    load_funcs_cvrt();
}


// Private static error exit.
//
void
cConvert::on_null_ptr()
{
    fprintf(stderr, "Singleton class cConvert used before instantiated.\n");
    exit(1);
}


// Call the update method of all visible related pop-ups.  Called when,
// e.g., a variable changes.  We update everything rather than try to
// keep track of which panel is affected by each variable.
//
void
cConvert::UpdatePopUps()
{
    PopUpAssemble(0, MODE_UPD);
    PopUpOasAdv(0, MODE_UPD, 0, 0);
    PopUpExport(0, MODE_UPD, 0, 0);
    PopUpImport(0, MODE_UPD, 0, 0);
    PopUpConvert(0, MODE_UPD, 0, 0, 0);
    PopUpChdOpen(0, MODE_UPD, 0, 0, 0, 0, 0, 0);
    PopUpChdConfig(0, MODE_UPD, 0, 0, 0);
    PopUpHierarchies(0, MODE_UPD);
    PopUpAuxTab(0, MODE_UPD);
    PopUpLibraries(0, MODE_UPD);
}


// Look through the current hierarchy for empty cells, and give the
// user the opportunity to delete them.  If force_delete_all, there is
// no pop-up, and all empties will be deleted if possible (it is not
// possible to delete empty cells that are instantianted in immutable
// parents).
//
void
cConvert::CheckEmpties(bool force_delete_all)
{
    if (!DSP()->CurCellName())
        return;
    if (XM()->RunMode() != ModeNormal && !force_delete_all)
        return;
    CDcbin cbin(DSP()->CurCellName());
    stringlist *sl = cbin.listEmpties();
    if (!sl)
        return;
    if (!force_delete_all) {
        PopUpEmpties(sl);
        stringlist::destroy(sl);
        return;
    }
    SymTab *tab = new SymTab(true, false);
    bool changed = false;
    for (;;) {
        bool didone = false;
        for (stringlist *s = sl; s; s = s->next) {
            if (SymTab::get(tab, s->string) == ST_NIL) {
                CDcbin cbtmp;
                if (CDcdb()->findSymbol(s->string, &cbtmp))
                    cbtmp.deleteCells();
                tab->add(lstring::copy(s->string), 0, false);
                didone = true;
            }
        }
        stringlist::destroy(sl);
        if (!didone)
            break;
        else
            changed = true;
        sl = cbin.listEmpties();
    }
    delete tab;
    if (changed) {
        cbin.fixBBs();
        XM()->PopUpCells(0, MODE_UPD);
        XM()->PopUpTree(0, MODE_UPD, 0, TU_CUR);
        DSP()->RedisplayAll();
    }
}


// Export for use in a script function.
//
bool
cConvert::Export(const char *filename, bool allcells)
{
    if (!DSP()->CurCellName()) {
        Errs()->add_error("Export: no current cell");
        return (false);
    }

    char *path = pathlist::expand_path(filename, false, true);
    FileType ft = cFIO::TypeExt(path);
    if (ft == Fnone) {
        if (!path) {
            Errs()->add_error("Export: null filename");
            return (false);
        }
        GFTtype gft = filestat::get_file_type(path);
        if (gft != GFT_DIR) {
            Errs()->add_error("Export: can't identify file type");
            return (false);
        }
        ft = Fnative;
    }
    else if (ft == Fnative) {
        char *p = strrchr(path, '.');
        if (p && lstring::cieq(p, ".xic")) {
            *p = 0;
            GFTtype gft = filestat::get_file_type(path);
            if (gft != GFT_DIR) {
                *p = '.';
                gft = filestat::get_file_type(path);
                if (gft != GFT_DIR) {
                    Errs()->add_error("Export: directory not found");
                    return (false);
                }
            }
        }
        else {
            Errs()->add_error("Export: internal error");
            return (false);
        }
    }

    stringlist *namelist = new stringlist(lstring::copy(
        allcells ? FIO_CUR_SYMTAB : Tstring(DSP()->CurCellName())), 0);

    GCdestroy<stringlist> gc_namelist(namelist);
    GCarray<char*> gc_path(path);

    CDcbin cbin(DSP()->CurCellName());
    if (!cbin.isSubcell())
        EditIf()->assignGlobalProperties(&cbin);

    FIOcvtPrms prms;
    prms.set_scale(FIO()->WriteScale());
    prms.set_alias_mask(CVAL_CASE | CVAL_PFSF | CVAL_FILE);
    if (!allcells && FIO()->OutFlatten()) {
        if (FIO()->OutUseWindow()) {
            prms.set_use_window(true);
            prms.set_window(FIO()->OutWindow());
            prms.set_clip(FIO()->OutClip());
        }
    }

    bool ret = false;
    if (ft == Fnative) {
        prms.set_destination(path, Fnative);
        ret = FIO()->ConvertToNative(namelist, &prms);
    }
    else if (ft == Fgds) {
        prms.set_destination(path, Fgds);
        ret = FIO()->ConvertToGds(namelist, &prms);
    }
    else if (ft == Fcgx) {
        prms.set_destination(path, Fcgx);
        ret = FIO()->ConvertToCgx(namelist, &prms);
    }
    else if (ft == Foas) {
        prms.set_destination(path, Foas);
        ret = FIO()->ConvertToOas(namelist, &prms);
    }
    else if (ft == Fcif) {
        prms.set_destination(path, Fcif);
        ret = FIO()->ConvertToCif(namelist, &prms);
    }
    else {
        Errs()->add_error("Export: internal error");
        return (false);
    }
    return (ret);
}


// Function to import the current mode part of another cell into the
// current cell, possibly recursively.
//
void
cConvert::ReadIntoCurrent(const char *filename, const char *cellname,
    bool recurse, FIOreadPrms *prms)
{
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;

    if (!filename || !*filename)
        filename = XM()->OpenFileDlg("Name of cell or file to read? ", 0);
    if (!filename) {
        PL()->ErasePrompt();
        return;
    }
    char *infile = pathlist::expand_path(filename, false, true);
    if (!strcmp(Tstring(DSP()->CurCellName()), infile)) {
        PL()->ShowPrompt("Can't import from current cell!");
        delete [] infile;
        return;
    }

    CDs *sdesc = CDcdb()->findCell(infile, DSP()->CurMode());
    if (sdesc) {
        // found the cell, dup it in
        CDs *sdc = CurCell(true);
        if (!sdc)
            // can't happen
            return;
        sdesc->cloneCell(sdc);
        PL()->ErasePrompt();
        delete infile;
        DSP()->RedisplayAll();
        return;
    }

    FILE *fp = FIO()->POpen(infile, "rb");
    if (!fp) {
        PL()->ShowPromptV("Can't find cell or file named %s.", infile);
        delete infile;
        return;
    }
    fclose(fp);

    // The name must refer to a file.  Set up a temporary symbol table
    // and read in the named file.  Look for a cell with the same name as
    // the current cell, or prompt the user for the cell name to source.
    //
    bool nogo = false;

    const char *stbak = CDcdb()->tableName();
    const char *stname = "ReadInto_tab";
    CDcdb()->switchTable(stname);

    CDcbin cbin;
    if (!FIO()->OpenImport(infile, prms, cellname, 0, &cbin)) {
        PL()->ShowPromptV("Error opening %s.", infile);
        nogo = true;
    }
    else {
        if ((!cellname || !*cellname) &&
                cbin.cellname() != DSP()->CurCellName() &&
                cbin.fileType() != Fnative) {
            if (!CDcdb()->findSymbol(DSP()->CurCellName(), &cbin)) {
                cellname = PL()->EditPrompt("Name of source cell? ", 0);
                for (;;) {
                    if (!cellname) {
                        nogo = true;
                        break;
                    }
                    if (!CDcdb()->findSymbol(cellname, &cbin)) {
                        cellname = PL()->EditPrompt(
                            "Not found, name of source cell? ", cellname);
                        continue;
                    }
                    break;
                }
                PL()->ErasePrompt();
            }
            if (!nogo && cbin.cellname())
                sdesc = cbin.celldesc(DSP()->CurMode());
        }
    }

    // Have to switch back to normal symbol table before dup.  Referenced
    // subcells may be empty.
    CDcdb()->switchTable(stbak);
    if (!nogo && sdesc) {
        if (recurse)
            MergeHier(CurCell(true), sdesc, stbak, stname);
        else {
            CDs *cursd = CurCell(true);
            if (!cursd)
                return;
            sdesc->cloneCell(cursd);
        }
    }
    CDcdb()->switchTable(stname);
    CDcdb()->destroyTable(false);
    CDcdb()->switchTable(stbak);

    DSP()->MainWdesc()->CenterFullView();
    DSP()->RedisplayAll();
    delete [] infile;
}


void
cConvert::MergeHier(CDs *s1, CDs *s2, const char *st1, const char *st2)
{
    ptrtab_t tab;
    mergeHier_rc(s1->cellname(), s2->cellname(), st1, st2, &tab);
    s1->fixBBs();
}


// Private recursive function to merge the cname hierarchies under
// cname from st2 into st1.
//
void
cConvert::mergeHier_rc(CDcellName cn1, CDcellName cn2, const char *st1,
    const char *st2, ptrtab_t *tab)
{
    CDcdb()->switchTable(st2);
    CDs *s2 = CDcdb()->findCell(cn2, DSP()->CurMode());
    CDcdb()->switchTable(st1);
    if (!s2)
        return;
    CDs *s1 = CDcdb()->findCell(cn1, DSP()->CurMode());
    if (!s1)
        return;
    s2->cloneCell(s1);
    tab->add(cn1);

    CDm_gen mgen = CDm_gen(s1, GEN_MASTERS);
    for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
        if (m->celldesc()) {
            if (!tab->find(m->cellname()))
                mergeHier_rc(m->cellname(), m->cellname(), st1, st2, tab);
        }
    }
}


// The CUT command, user clicks to define a rectangle on-screen.  The
// contents of this region can then be exported to a new file.

namespace {
    namespace main_cvrt {
        struct CutState : public CmdState
        {
            friend void cConvert::CutWindowExec(CmdDesc*);

            CutState(const char*, const char*);
            virtual ~CutState();

            void SetCaller(GRobject c)  { Caller = c; }

        private:
            void b1down();
            void b1up();
            void esc();
            void cut_doit(int, int);
            void message() { if (Level == 1) PL()->ShowPrompt(msg1);
                else PL()->ShowPrompt(msg2); }
            void SetLevel1(bool show) { Level = 1; if (show) message(); }
            void SetLevel2() { Level = 2; message(); }

            GRobject Caller;
            int Refx, Refy;

            static const char *msg1;
            static const char *msg2;
        };

        CutState *CutCmd;
    }
}

using namespace main_cvrt;

const char *CutState::msg1 =
    "Click twice or drag to define rectangular area.";
const char *CutState::msg2 = "Click on second diagonal endpoint.";


// Menu function for cut command.
//
void
cConvert::CutWindowExec(CmdDesc *cmd)
{
    if (CutCmd)
        CutCmd->esc();
    if (!XM()->CheckCurMode(Physical))
        return;
    if (DSP()->MainWdesc()->DbType() == WDcddb) {
        if (!XM()->CheckCurCell(false, false, DSP()->CurMode()))
            return;
    }

    CutCmd = new CutState("CUT", "xic:cut");
    CutCmd->SetCaller(cmd ? cmd->caller : 0);
    if (!EV()->PushCallback(CutCmd)) {
        if (cmd)
            Menu()->Deselect(cmd->caller);
        delete CutCmd;
        return;
    }
    CutCmd->message();
}


// Push a file name into the Conversion pop-up to avoid a prompt.
// Called when using the Conversion panel in the Cut command.
//
void
cConvert::SetConvertFilename(const char *fname)
{
    if (fname) {
        if (Menu()->MenuButtonStatus("conv", MenuCONVT) == 1) {
            delete [] cvt_cv_filename;
            cvt_cv_filename = lstring::copy(fname);
        }
    }
    else {
        delete [] cvt_cv_filename;
        cvt_cv_filename = 0;
    }
}


// Push/pop the AOI parameters for the Conversion panel.
// Called when using the Conversion panel in the Cut command.
//
void
cConvert::SetupConvertCut(const BBox *AOI)
{
    if (AOI) {
        cvt_cv_aoi = *FIO()->CvtWindow();
        FIO()->SetCvtWindow(AOI);
        cvt_cv_win = FIO()->CvtUseWindow();
        FIO()->SetCvtUseWindow(true);
        cvt_cv_clip = FIO()->CvtClip();
        FIO()->SetCvtClip(true);
        cvt_cv_flat = FIO()->CvtFlatten();
        FIO()->SetCvtFlatten(true);
    }
    else {
        FIO()->SetCvtWindow(&cvt_cv_aoi);
        FIO()->SetCvtUseWindow(cvt_cv_win);
        FIO()->SetCvtClip(cvt_cv_clip);
        FIO()->SetCvtFlatten(cvt_cv_flat);
    }
}


// Push a file name into the Write File pop-up to avoid a prompt.
// Called when using the Write File panel in the Cut command.
//
void
cConvert::SetWriteFilename(const char *fname)
{
    if (fname) {
        if (Menu()->MenuButtonStatus("conv", MenuEXPRT) == 1) {
            delete [] cvt_wr_filename;
            cvt_wr_filename = lstring::copy(fname);
        }
    }
    else {
        delete [] cvt_wr_filename;
        cvt_wr_filename = 0;
    }
}


// Push/pop the AOI parameters for the Write File panel.
// Called when using the Write File panel in the Cut command.
//
void
cConvert::SetupWriteCut(const BBox *AOI)
{
    if (AOI) {
        cvt_wr_aoi = *FIO()->OutWindow();
        FIO()->SetOutWindow(AOI);
        cvt_wr_win = FIO()->OutUseWindow();
        FIO()->SetOutUseWindow(true);
        cvt_wr_clip = FIO()->OutClip();
        FIO()->SetOutClip(true);
        cvt_wr_flat = FIO()->OutFlatten();
        FIO()->SetOutFlatten(true);
    }
    else {
        FIO()->SetOutWindow(&cvt_wr_aoi);
        FIO()->SetOutUseWindow(cvt_wr_win);
        FIO()->SetOutClip(cvt_wr_clip);
        FIO()->SetOutFlatten(cvt_wr_flat);
    }
}


CutState::CutState(const char *nm, const char* hk) : CmdState(nm, hk)
{
    Caller = 0;
    Refx = Refy = 0;
    Level = 1;
}


CutState::~CutState()
{
    CutCmd = 0;
}


void
CutState::b1down()
{
    if (Level == 1) {
        // Button1 down.  Record location, start drawing ghost rectangle.
        //
        EV()->Cursor().get_xy(&Refx, &Refy);
        Gst()->SetGhostAt(GFbox, Refx, Refy);
        EV()->DownTimer(GFbox);
    }
    else {
        // Second button1 press, create subwindow.
        //
        DSPmainDraw(ShowGhost(ERASE))
        int xc, yc;
        EV()->Cursor().get_xy(&xc, &yc);
        cut_doit(xc, yc);
    }
}


void
CutState::b1up()
{
    if (Level == 1) {
        // Button1 up.  Do operation if the pointer moved appreciably.
        // Otherwise wait for user to click button1 again.
        //
        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok() &&
                EV()->CurrentWin()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            if (x != Refx && y != Refy) {
                DSPmainDraw(ShowGhost(ERASE))
                cut_doit(x, y);
                esc();
                DSPmainDraw(ShowGhost(DISPLAY))
                return;
            }
        }
        SetLevel2();
    }
    else {
        // Second button1 release, exit.
        //
        esc();
        DSPmainDraw(ShowGhost(DISPLAY))
    }
}


// Abort subwindow creation.
//
void
CutState::esc()
{
    Gst()->SetGhost(GFnone);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


// Pop up or update the appropriate dialog.
//
void
CutState::cut_doit(int x, int y)
{
    BBox AOI;
    AOI.left = mmMin(Refx, x);
    AOI.bottom = mmMin(Refy, y);
    AOI.right = mmMax(Refx, x);
    AOI.top = mmMax(Refy, y);

    // Save the window in storage register 0.
    *FIO()->savedBB(0) = AOI;

    char namebuf[256];
    if (DSP()->MainWdesc()->DbType() == WDcddb) {
        Cvt()->SetupWriteCut(&AOI);
        if (Menu()->MenuButtonStatus("conv", MenuEXPRT) == 0)
            Menu()->MenuButtonPress("conv", MenuEXPRT);
        sprintf(namebuf, "%s-cut", Tstring(DSP()->CurCellName()));
        Cvt()->SetWriteFilename(namebuf);
        Cvt()->PopUpExport(0, MODE_UPD, 0, 0);
    }
    else if (DSP()->MainWdesc()->DbType() == WDchd) {
        Cvt()->SetupConvertCut(&AOI);
        if (Menu()->MenuButtonStatus("conv", MenuCONVT) == 0)
            Menu()->MenuButtonPress("conv", MenuCONVT);
        Cvt()->SetConvertFilename(DSP()->MainWdesc()->DbName());
        // "-cut" is appended to this name in the pop-up handler.
        Cvt()->PopUpConvert(0, MODE_UPD, cConvert::cvChdName, 0, 0);
    }
}

