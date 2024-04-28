
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
#include "editif.h"
#include "scedif.h"
#include "drcif.h"
#include "extif.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "fio.h"
#include "fio_chd.h"
#include "fio_library.h"
#include "layertab.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "events.h"
#include "cvrt.h"
#include "pcell.h"
#include "oa_if.h"
#include "miscutil/filestat.h"
#include "miscutil/pathlist.h"

//#define TIMEDBG
#ifdef TIMEDBG
#include "miscutil/tvals.h"
#endif

#include <sys/types.h>
#include <time.h>
#include <dirent.h>


/*************************************************************************
 *
 * Functions to open a new cell for display.
 *
 *************************************************************************/

namespace {
    namespace OpenHlpr { bool upd_callback(const char*, void*, XEtype); };

    // Prompt to revert if user undoes symbolic mode.
    CDs *symb_cell;
    bool symb_mode;
}


// Main function to open a cell/file for editing.  The file will be
// opened and read into the database as needed.  If the current cell
// is modified, the user will be prompted to save it, unless noask is
// true.  If chd is given, file_or_cell_name is ignored.  Symname is
// the cell to open in an archive or library, if null then the
// top-level cell will be opened, or the user will be prompted with
// choices.
//
EditType
cMain::EditCell(const char *file_or_cell_name, bool noask,
    const FIOreadPrms *prms, const char *cellname, cCHD *chd)
{
    // This is not reentrant.  If the user drops a file while waiting
    // for a prompt from here, bad things happen.
    static bool inhere;
    if (inhere)
        return (EditReentered);
    inhere = true;

    if (!prms)
        prms = FIO()->DefReadPrms();

    EV()->InitCallback();

    // See if we have a named CHD given.
    if (file_or_cell_name && !chd) {
        chd = CDchd()->chdRecall(file_or_cell_name, false);
        if (chd && CDvdb()->getVariable(VA_ChdLoadTopOnly)) {
            if (!cellname) {
                // Allow the default name.
                symref_t *p = chd->defaultSymref(Physical);
                if (p)
                    cellname = Tstring(p->get_name());
            }
            if (chd->loadCell(cellname) != OIok) {
                DSPpkg::self()->ErrPrintf(ET_ERROR, "%s", Errs()->get_error());
                inhere = false;
                return (EditFailed);
            }
            Cvt()->PopUpAuxTab(0, MODE_UPD);
            inhere = false;
            return (Load(DSP()->MainWdesc(), cellname));
        }
    }

    FileType filetype;
    if (chd)
        filetype = chd->filetype();
    else if (DSP()->TopCellName()) {
        CDcbin cbin(DSP()->TopCellName());
        filetype = cbin.fileType();
    }
    else
        filetype = Fnative;
    if (filetype == Fnone)
        filetype = Fnative;

    if (!noask && XM()->RunMode() == ModeNormal && DSP()->CurCellName()) {
        if (symb_cell && symb_mode) {
            // If the user has undone symbolic mode, ask to revert. 
            // It is too easy to change mode to look at something,
            // then edit a new cell but find that the instance
            // placements of the previous cell are now all messed up.

            bool reverted = false;
            CDs *esd = CDcdb()->findCell(DSP()->CurCellName(), Electrical);
            if (esd == symb_cell && !esd->symbolicRep(0)) {
                char *in = PL()->EditPrompt(
                    "You've changed this cell to non-symbolic.  "
                    "Do you want to revert back? ", "y");
                in = lstring::strip_space(in);
                if (in == 0) {
                    PL()->ErasePrompt();
                    inhere = false;
                    return (EditAborted);
                }
                if (*in == 'y' || *in == 'Y') {
                    CDp_sym *ps = (CDp_sym*)esd->prpty(P_SYMBLC);
                    if (ps) {
                        ps->set_active(true);
                        ScedIf()->assertSymbolic(true);
                        DSP()->RedisplayAll(Electrical);
                        reverted = true;
                    }
                }
            }
            if (!reverted) {
                // If we just change symbolic mode, we don't increment
                // the modified count or undo association.  We do that
                // here, if the user decides to keep the change.  This
                // may be redundant if other changes were made.

                esd->incModified();
                CDs *psd = CDcdb()->findCell(DSP()->CurCellName(), Physical);
                if (psd)
                    psd->setAssociated(false);
            }
        }
        if (filetype == Fnative && CDvdb()->getVariable(VA_AskSaveNative)) {
            // This used to be on all of the time and is really
            // annoying.  The variable can be set if the user wants
            // this.

            CDcbin cbin(DSP()->CurCellName());

            bool asksv = false;
            bool ispsm = cbin.phys() && cbin.phys()->isPCellSubMaster();
            bool isvsm = cbin.phys() && cbin.phys()->isViaSubMaster();
            if (!ispsm && !isvsm && cbin.isModified())
                asksv = true;
            else if (cbin.phys()->countModified() &&
                    ((ispsm && (cbin.phys()->isPCellReadFromFile() ||
                        FIO()->IsKeepPCellSubMasters())) ||
                    (isvsm && FIO()->IsKeepViaSubMasters())))
                asksv = true;
            if (asksv) {
                char *in = PL()->EditPrompt(
                    "You've modified this cell.  Do you want to save it? ",
                    "y");
                in = lstring::strip_space(in);
                if (in == 0) {
                    PL()->ErasePrompt();
                    inhere = false;
                    return (EditAborted);
                }
                if (*in == 'y' || *in == 'Y')
                    SaveCellAs(0);
            }
        }
    }

    if (chd)
        file_or_cell_name = chd->filename();

    EditType ret;
    if (!file_or_cell_name || !*file_or_cell_name) {
        // Default cell name, check in order
        //  selection from Cells/Files/Tree popups
        //  selected subcell in drawing window
        //  next command line cell
        //  current cell
        //
        file_or_cell_name = GetCurFileSelection();
        if (!file_or_cell_name) {
            CDc *cd = (CDc*)Selections.firstObject(CurCell(), "c");
            if (cd)
                file_or_cell_name = Tstring(cd->cellname());
        }
        if (!file_or_cell_name)
            file_or_cell_name = NextArg();
        if (!file_or_cell_name) {
            if (DSP()->CurCellName())
                file_or_cell_name = Tstring(DSP()->CurCellName());
            else
                file_or_cell_name = "";
        }

        if (!noask && XM()->RunMode() == ModeNormal) {
            file_or_cell_name = OpenFileDlg("File, CHD and/or cell? ",
                file_or_cell_name);
            if (file_or_cell_name == 0) {
                if (DSP()->CurCellName()) {
                    PL()->ErasePrompt();
                    inhere = false;
                    return (EditAborted);
                }
            }
        }

        // If two args, the first name is the name of an archive file,
        // the second name is the cell name to edit.  Single or
        // double quotes can be used to preserve white space in path
        // names
        const char *ctmp = file_or_cell_name;
        char *fcname = lstring::getqtok(&ctmp);
        char *syname = lstring::getqtok(&ctmp);
        if (!fcname)
            fcname = lstring::copy(XM()->DefaultEditName());
        ret = Load(DSP()->MainWdesc(), fcname, prms, syname);
        delete [] syname;

        if (!DSP()->CurCellName()) {
            if (strcmp(fcname, XM()->DefaultEditName())) {
                delete [] fcname;
                fcname = lstring::copy(XM()->DefaultEditName());
                ret = Load(DSP()->MainWdesc(), fcname, prms);
            }
        }
        if (!DSP()->CurCellName()) {
            if (!strcmp(fcname, XM()->DefaultEditName())) {
                delete [] fcname;
                fcname = NewCellName();
                ret = Load(DSP()->MainWdesc(), fcname, prms);
            }
        }
        delete [] fcname;
        if (!DSP()->CurCellName()) {
            fprintf(stderr, "Can not open cell, unknown error, exiting.\n");
            DSPpkg::self()->Halt();
            inhere = false;
            return (EditFailed);
        }
    }
    else
        ret = Load(DSP()->MainWdesc(), file_or_cell_name, prms, cellname, chd);
    inhere = false;
    return (ret);
}


namespace {
    // Callback data for OpenImport, used in Load.
    //
    struct LoadCbData : public OIcbData
    {
        int win_num;
        FIOreadPrms readPrms;
    };

    // Callback for OpenImport.
    //
    void
    load_cb(const char *name, OIcbData *data)
    {
        LoadCbData *lcdata = static_cast<LoadCbData*>(data);
        if (!lcdata)
            return;
        if (!name) {
            // Called from pop-up destructor.
            delete lcdata;
            return;
        }
        WindowDesc *wdesc = DSP()->Window(lcdata->win_num);
        if (!wdesc)
            return;
        if (lcdata->lib_filename)
            XM()->Load(wdesc, lcdata->lib_filename, &lcdata->readPrms, name);
        else
            XM()->Load(wdesc, name, &lcdata->readPrms);
    }
}


// Read the given file or cell into the database, and display it in
// the window.  If the window is the main, the current editing context
// is set.  The cellname is used to resolve ambiguities if there are
// multiple top level cells defined in the file given in
// file_or_cell_name If the file is a library text file, open it in
// text mode.
//
EditType
cMain::Load(WindowDesc *wdesc, const char *file_or_cell_name,
    const FIOreadPrms *prms, const char *cellname, cCHD *chd)
{
    // Turn off terminals display while loading, turn it back on when
    // we leave this function.
    struct show_terms_ctrl
    {
        show_terms_ctrl()
            {
                show_terms = DSP()->ShowTerminals();
                if (show_terms)
                    DSP()->ShowTerminals(ERASE);
            }

        ~show_terms_ctrl()
            {
                DSP()->SetShowTerminals(show_terms);
                if (DSP()->ShowTerminals())
                    DSP()->ShowTerminals(DISPLAY);
            }
    private:
        bool show_terms;
    };

#ifdef TIMEDBG
    double T1 = Tvals::millisec();
#endif
    if (!wdesc)
        wdesc = DSP()->MainWdesc();
    if (!wdesc)
        return (EditFailed);
    if (!file_or_cell_name && !chd)
        return (EditFailed);

    show_terms_ctrl st_ctrl;
    EV()->InitCallback();

    if (wdesc->DbType() == WDchd) {
        const char *dbname = file_or_cell_name;
        const char *cname = cellname;
        if (!cname) {
            dbname = 0;
            cname = file_or_cell_name;
        }
        if (!dbname)
            dbname = wdesc->DbName();
        else if (strcmp(dbname, wdesc->DbName())) {
            DSPpkg::self()->ErrPrintf(ET_ERROR, "CHD name does not match %s.",
                wdesc->DbName());
            return (EditFailed);
        }
        cCHD *tchd = CDchd()->chdRecall(dbname, false);
        if (!tchd) {
            DSPpkg::self()->ErrPrintf(ET_ERROR, "unknown CHD %s.", dbname);
            return (EditFailed);
        }
        if (!tchd->findSymref(cname, wdesc->Mode(), true)) {
            DSPpkg::self()->ErrPrintf(ET_ERROR, "cell name unknown in CHD %s.",
                dbname);
            return (EditFailed);
        }

        wdesc->SetHierDisplayMode(dbname, cname, 0);
        return (EditOK);
    }
    if (wdesc->DbType() != WDcddb)
        return (EditFailed);

    // Clear persistent OA cells-loaded table.
    OAif()->clear_name_table();

    if (!prms)
        prms = FIO()->DefReadPrms();

    // Consistency test:  make sure that present symbol is in table,
    // zero it if not.
    if (wdesc->CurCellName()) {
        if (!CDcdb()->findSymbol(wdesc->CurCellName(), 0)) {
            wdesc->SetCurCellName(0);
            wdesc->SetTopCellName(0);
        }
    }
    const CDtptr *last_cur_cell = DSP()->CurCellName();

    // See if we have a named CHD given.
    if (file_or_cell_name && !chd) {
        chd = CDchd()->chdRecall(file_or_cell_name, false);
        if (chd && CDvdb()->getVariable(VA_ChdLoadTopOnly)) {
            if (!cellname) {
                // Allow the default name.
                symref_t *p = chd->defaultSymref(Physical);
                if (p)
                    cellname = Tstring(p->get_name());
            }
            if (chd->loadCell(cellname) != OIok) {
                DSPpkg::self()->ErrPrintf(ET_ERROR, "%s", Errs()->get_error());
                return (EditFailed);
            }
            Cvt()->PopUpAuxTab(0, MODE_UPD);
            return (Load(wdesc, cellname));
        }
    }

    bool ismain = (wdesc == DSP()->MainWdesc());
    const char *name = lstring::strip_path(file_or_cell_name);

    if (!strcmp(name, XM()->DeviceLibName()) ||
            !strcmp(name, XM()->ModelLibName())) {

        // If library is called by name, copy the library into the
        // current directory if it is not originating from the current
        // directory
        //
        char *realp;
        FILE *fp = pathlist::open_path_file(file_or_cell_name,
            CDvdb()->getVariable(VA_LibPath), "r", &realp, true);
        bool noedit = false;
        if (fp) {
            if (!filestat::is_same_file(realp, name)) {
                // curdir file is not the opened file
                if (filestat::create_bak(name)) {
                    FILE *gp = fopen(name, "w");
                    if (gp) {
                        int c;
                        while ((c = getc(fp)) != EOF)
                            putc(c, gp);
                        fclose(gp);
                    }
                }
                else {
                    DSPpkg::self()->ErrPrintf(ET_ERROR, "%s",
                        filestat::error_msg());
                    noedit = true;
                }
            }
            fclose(fp);
            delete [] realp;
        }
        if (!noedit)
            DSPmainWbag(PopUpTextEditor(name, OpenHlpr::upd_callback, 0,
                false))
        PL()->ErasePrompt();
        return (EditText);
    }

    if (ismain) {
        ClearReferences(true);
        PL()->ShowPrompt("Building database.  Please wait.");
    }

    LoadCbData *lcdata = new LoadCbData;
    lcdata->readPrms = *prms;

    if (wdesc == DSP()->MainWdesc())
        lcdata->win_num = 0;
    else if (wdesc == DSP()->Window(1))
        lcdata->win_num = 1;
    else if (wdesc == DSP()->Window(2))
        lcdata->win_num = 2;
    else if (wdesc == DSP()->Window(3))
        lcdata->win_num = 3;
    else if (wdesc == DSP()->Window(4))
        lcdata->win_num = 4;
    else {
        delete lcdata;
        return (EditFailed);
    }

    Errs()->init_error();
    char *pathname = 0;

#ifdef TIMEDBG
    double T2 = Tvals::millisec();
#endif
    CDcbin cbin;
    LT()->FreezeLayerTable(true);
    OItype oiret = FIO()->OpenImport(file_or_cell_name, prms, cellname, chd,
        &cbin, &pathname, load_cb, lcdata);
    LT()->FreezeLayerTable(false);
#ifdef TIMEDBG
    double T3 = Tvals::millisec();
#endif

    // New cells may now be in memory, even if read failed.  Update
    // the listing.
    PopUpCells(0, MODE_UPD);

    if (oiret == OIerror) {
        Errs()->add_error("Error opening %s",
            pathname ? pathname : file_or_cell_name);
        Log()->ErrorLog(mh::Initialization, Errs()->get_error());
        delete [] pathname;
        ScedIf()->recomputeDots();
        delete lcdata;
        return (EditFailed);
    }
    if (oiret == OIaborted) {
        PL()->ShowPrompt("Aborted.");
        delete [] pathname;
        ScedIf()->recomputeDots();
        delete lcdata;
        return (EditAborted);
    }

    if (oiret == OIambiguous) {
        // The user opened an archive file with multiple top level
        // cells.  OpenImport has launched a popup for the user to
        // choose which one to edit.
        //
        delete [] pathname;
        ScedIf()->recomputeDots();
        // The lcdata is deleted in the callback.
        return (EditAmbiguous);
    }
    delete lcdata;

    if (!ismain) {
        delete [] pathname;
        wdesc->SetSymbol(&cbin);
        return (EditOK);
    }

    // Replacing the current cell.

    DrcIf()->clearCurError();   // DRC errors, do this here only, not in
                                // context change.

    ScedIf()->checkElectrical(&cbin);
    SetNewContext(&cbin, true);
    cbin = CDcbin(DSP()->CurCellName());
    CDs *cursd = wdesc->Mode() == Physical ? cbin.phys() : cbin.elec();

    if (DSP()->CurCellName()) {
        // These operations are sane when cell is immutable.
        if (!CDvdb()->getVariable(VA_NoCheckEmpties))
            Cvt()->CheckEmpties(false);
        if (cbin.phys())
            cbin.phys()->computeBB();
        if (cbin.elec())
            cbin.elec()->computeBB();
    }

    if (!cursd || cursd->db_is_empty(0))
        wdesc->DefaultWindow();
    else
        wdesc->CenterFullView();
    wdesc->Redisplay(0);
    wdesc->ShowTitle();
    wdesc->UpdateProxy();

    if (!wdesc->CurCellName()) {
        // can't happen
        PL()->ShowPrompt("Error: no current cell!");
        return (EditFailed);
    }

    // If the cell is a pcell sub-master, add it to the table so it
    // will resolve calls to instantiate with its parameter set.
    //
    PC()->recordIfSubMaster(cbin.phys());

    ShowParameters();
    PushOpenCellName(DSP()->CurCellName(), last_cur_cell);

    if (oiret == OInew)
        PL()->ShowPromptV("Current cell is %s.", wdesc->CurCellName());
    else if (cbin.isLibrary() && cbin.isDevice())
        PL()->ShowPromptV("Current cell is library device %s.",
            wdesc->CurCellName());
    else if (oiret == OIold)
        PL()->ShowPromptV("Current cell is %s (already in memory).",
            wdesc->CurCellName());
    else {
        if (cursd && cursd->fileType() == Foa) {
            PL()->ShowPromptV(
                "Current cell is %s, from OpenAccess library %s.",
                wdesc->CurCellName(), file_or_cell_name);
        }
        else {
            PL()->ShowPromptV("Current cell is %s, from file %s.",
                wdesc->CurCellName(),
                pathname && *pathname ? pathname : file_or_cell_name);
        }
    }

    // Save electrical symbolic state.
    symb_cell = cbin.elec();
    symb_mode = symb_cell ? symb_cell->symbolicRep(0) != 0 : false;

#ifdef TIMEDBG
    double T4 = Tvals::millisec();
    printf("%g %g %g\n", T2-T1, T3-T2, T4-T3);
#endif
    delete [] pathname;
    return (EditOK);
}


// If the named cell is not found in memory, create it as an empty
// cell.  if tocur is set, switch the current cell to the specified
// cell.  Returns are OIerror, OIok if no new cell created, OInew if
// a cell was created.
//
OItype
cMain::TouchCell(const char *cname, bool tocur)
{
    if (!cname || !*cname) {
        Errs()->add_error("TouchCell: null or empty cell name.");
        return (OIerror);
    }
    bool newcell = false;
    if (!CDcdb()->findCell(cname, DSP()->CurMode())) {
        if (!CDcdb()->insertCell(cname, DSP()->CurMode())) {
            Errs()->add_error("TouchCell: failed to create new cell.");
            return (OIerror);
        }
        PopUpCells(0, MODE_UPD);
        newcell = true;
    }
    if (!tocur)
        return (newcell ? OInew : OIok);

    WindowDesc *wdesc = DSP()->MainWdesc();
    if (!wdesc) {
        Errs()->add_error("TouchCell: no main window!");
        return (OIerror);
    }
    if (wdesc->DbType() != WDcddb) {
        Errs()->add_error("TouchCell: main window wrong type");
        return (OIerror);
    }
    EV()->InitCallback();

    // Clear persistent OA cells-loaded table.
    OAif()->clear_name_table();

    ClearReferences(true);

    DrcIf()->clearCurError();   // DRC errors, do this here only, not in
                                // context change.

    CDcbin cbin;
    CDcdb()->findSymbol(cname, &cbin);

    SetNewContext(&cbin, true);
    wdesc->ShowTitle();

    // Save electrical symbolic state.
    symb_cell = cbin.elec();
    symb_mode = symb_cell ? symb_cell->symbolicRep(0) != 0 : false;
    return (newcell ? OInew : OIok);
}


// Suppose that one has a collection of pcell sub-master Xic cells that
// have been imported from a foreign OpenAccess tool such as Virtuoso. 
// These are assumed to not be portable pcells.  One would like to use
// these cells to resolve pcells when reading directly from the
// OpenAccess database.  There are two issues:  1) the system needs to
// know that these cells are available, and 2) one has to remap the
// cell names.  The first issue is fixed simply by making the
// sub-masters available through the library mechanism.  The second
// issue is due to the simple naming convention of the sub-master
// instantiations, which suffixes the pcell name with "$$" followed by
// an integer.  The integer is a count of when the cell was generated,
// and is consistent with the design output at the time, but there is
// no guarantee the the names are consistent with the design at other
// times.
//
// This function will read a collection of cells into a temporary
// symbool table.  Those that are pcell sub-masters have the property
// strings entered into the internal pcell database, under the
// existing cell name.  This will cause the correct cell name to be
// associated with a given parameter set.  The cells are not saved,
// but the entries in the pcell table persist so that resolution, when
// reading OpenAccess or otherwise, will reference the correct cells. 
// The cell collection must be available through an open library, and
// this function must be run, before loading the design.
//
bool
cMain::RegisterSubMasters(const char *s)
{
    if (!s || !*s) {
        return (false);
        Errs()->add_error("no file/directory given");
    }
    GFTtype gft = filestat::get_file_type(s);
    if (gft == GFT_DIR) {
        stringlist *s0 = 0;
        DIR *wdir = opendir(s);
        if (wdir) {
            struct dirent *de;
            while ((de = readdir(wdir)) != 0) {
                if (!strcmp(de->d_name, "."))
                    continue;
                if (!strcmp(de->d_name, ".."))
                    continue;
                s0 = new stringlist(lstring::copy(de->d_name), s0);
            }
            closedir(wdir);
        }

        const char *stbak = CDcdb()->tableName();
        const char *stname = "tmp_tab";
        CDcdb()->switchTable(stname);

        for (stringlist *sl = s0; sl; sl = sl->next) {
            char *p = pathlist::mk_path(s, sl->string);
            CDcbin cbin;
            FIO()->OpenImport(p, FIO()->DefReadPrms(), 0, 0, &cbin); 
            PC()->recordIfSubMaster(cbin.phys());
            delete [] p;
            delete cbin.phys();
            delete cbin.elec();
        }
        stringlist::destroy(s0);

        CDcdb()->destroyTable(false);
        CDcdb()->switchTable(stbak);
        return (true);
    }
    if (gft == GFT_FILE) {
        const char *stbak = CDcdb()->tableName();
        const char *stname = "tmp_tab";
        CDcdb()->switchTable(stname);

        FIO()->OpenImport(s, FIO()->DefReadPrms(), 0, 0, 0); 

        CDgenTab_s gen(Physical);
        CDs *sd;
        while ((sd = gen.next()) != 0) {
            PC()->recordIfSubMaster(sd);
        }

        CDcdb()->destroyTable(false);
        CDcdb()->switchTable(stbak);
        return (true);
    }
    if (gft == GFT_OTHER)
        Errs()->add_error("unknown file type");
    else if (gft == GFT_NONE)
        Errs()->add_error("file/directory not found");
    return (false);
}


// Called after a new cell is opened for editing.  From the main edit,
// toplevel is true.  From push/pop, toplevel is false.
//
void
cMain::SetNewContext(CDcbin *cbin, bool toplevel)
{
    ExtIf()->preCurCellChangeCallback();
    if (!toplevel)
        ClearReferences(false);
    if (cbin) {
        CDcbin curcbin(DSP()->CurCellName());
        if (DSP()->CurMode() == Physical)
            SetRulers(cbin->phys(), curcbin.phys());
        else
            SetRulers(cbin->elec(), curcbin.elec());
        if (toplevel)
            DSP()->SetTopCellName(cbin->cellname());
        ScedIf()->assertSymbolic(false);
        DSP()->SetCurCellName(cbin->cellname());
    }
    EditIf()->ulListBegin(false, false);
    if (!cbin)
        return;

    if (toplevel) {
        // do before calling CDs::hy_init() in CDs::connectAll()
        EditIf()->assertGlobalProperties(cbin);

        bool trd = DSP()->NoRedisplay();
        DSP()->SetNoRedisplay(true);
        if (DSP()->CurMode() == Electrical && cbin->elec())
            // don't bother in physical mode
            ScedIf()->connectAll(false);
        if (ScedIf()->showingDots() != DotsNone)
            ScedIf()->recomputeDots();
        DSP()->SetNoRedisplay(trd);

        // Initialize the drc list.  This is done after opening the
        // cell, so that new layers created are available for
        // reference.
        DrcIf()->initRules();
    }
    ExtIf()->postCurCellChangeCallback();
    ScedIf()->PopUpNodeMap(0, MODE_UPD);
    EditIf()->prptyRelist();
    CDs *sd = DSP()->CurMode() == Physical ? cbin->phys() : cbin->elec();
    EditIf()->setEditingMode(!sd || !sd->isImmutable());
    EditIf()->registerGrips(0);

    // Assert "symbl" button and setup menu, *after* setEditingMode.
    ScedIf()->assertSymbolic(true);
}


// Return a cell name (malloc'ed) that is guaranteed to not be
// presently in use in the current symbol table.
//
char *
cMain::NewCellName()
{
    char buf[256];
    time_t t = time(0);
    tm *tm = gmtime(&t);
    snprintf(buf, sizeof(buf), "$%02d%02d%02d%02d%02d%02d",
        tm->tm_min+1, tm->tm_mday, tm->tm_year-100,
        tm->tm_hour, tm->tm_min, tm->tm_sec);
    int len = strlen(buf);
    char *e = buf + len;
    int cnt = 1;
    while (CDcdb()->findSymbol(buf)) {
        snprintf(e, sizeof(buf) - len, "_%d", cnt);
        cnt++;
    }
    return (lstring::copy(buf));
}
// End of cMain functions.


bool
OpenHlpr::upd_callback(const char *fname, void*, XEtype fromsave)
{
    if (fromsave == XE_SAVE) {
        if (!strcmp(fname, XM()->DeviceLibName())) {
            EV()->InitCallback();
            FIO()->CloseLibrary(XM()->DeviceLibName(), LIBdevice);
            FIO()->OpenLibrary(
                CDvdb()->getVariable(VA_LibPath), XM()->DeviceLibName());
            CDs *topsde = CDcdb()->findCell(DSP()->TopCellName(), Electrical);
            if (topsde)
                topsde->fixBBs();
            ScedIf()->PopUpDevs(0, MODE_UPD);
            PL()->ShowPrompt("Device library updated.");
        }
        else if (!strcmp(fname, XM()->ModelLibName())) {
            ScedIf()->modelLibraryClose();
            ScedIf()->modelLibraryOpen(XM()->ModelLibName());
            PL()->ShowPrompt("Model library updated.");
        }
    }
    return (true);
}

