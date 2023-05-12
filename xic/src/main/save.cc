
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
#include "fio.h"
#include "cvrt_variables.h"
#include "editif.h"
#include "scedif.h"
#include "cd_celldb.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "events.h"
#include "pushpop.h"
#include "oa_if.h"
#include "miscutil/filestat.h"
#include "miscutil/pathlist.h"
#include <sys/stat.h>


// Name of panic log file.
#define PANIC_LOG "xic_panic.log"

namespace {
    // Helper functions.
    namespace SaveHlpr
    {
        bool save_panic(CDcbin*);
        bool save_cb(const char*);
        char *get_new_name(bool);
        FileType interpret_response(const char*, char**, char**);
        bool save_cell_as(CDcellName, FileType, const char*, const char*,
            bool);
        bool write_native(CDcbin&, const char*, const char*);
        bool write_export(CDcellName, const char*, FileType);
    };

    // Keyword for error log.
    const char *save_file = "save file";
}


// Save the current cell and modified cells in memory.
//
void
cMain::Save()
{
    EV()->InitCallback();
    if (DSP()->CurCellName()) {
        switch (XM()->CheckModified(false)) {
        case CmodAborted:
            PL()->ShowPrompt("Operation ABORTED, save incomplete.");
            break;
        case CmodFailed:
            PL()->ShowPrompt("Error: write FAILED, save incomplete.");
            break;
        case CmodOK:
            PL()->ShowPrompt("Done.");
            break;
        case CmodNoChange:
            PL()->ShowPrompt(
                "Not modified, nothing written.  Use Save As to force write.");
            break;
        }
    }
}


// Look through the hierarchy for modified cells, and give the user a
// chance to save them.  If panic is true, all modified cells are
// saved in a "panic.pid" subdirectory.
//
CmodType
cMain::CheckModified(bool panic)
{
    if (!DSP()->TopCellName())
        return (CmodNoChange);
    if (XM()->RunMode() != ModeNormal)
        return (CmodOK);

    if (panic) {
        DisableDialogs();

        // this directs log messages to the panic log
        xm_panic_fp = fopen(PANIC_LOG, "w");
        if (xm_panic_fp) {
            fprintf(xm_panic_fp, "# %s crash log, pid: %d\n",
                XM()->IdString(), (int)getpid());
            fprintf(xm_panic_fp, "cell: %s top: %s mode: %d\n",
                DSP()->CurCellName() ? Tstring(DSP()->CurCellName()) : "none",
                DSP()->TopCellName() ? Tstring(DSP()->TopCellName()) : "none",
                DSP()->CurMode());
            EV()->PanicPrint(xm_panic_fp);
            if (Log()->LogDirectory()) {
                fprintf(xm_panic_fp, "Logfiles retained in %s.\n",
                    Log()->LogDirectory());
            }
            fprintf(xm_panic_fp, "Dumping modified cells...\n");
        }
    }

    CmodType retval = CmodNoChange;

    if (panic) {
        // First loop through all cells and save the archive
        // hierarchies.  This resets modified flags.

        CDgenTab_cbin *sgen = new CDgenTab_cbin;
        CDcbin cbin;
        while (sgen->next(&cbin)) {
            if (!cbin.isSubcell() &&
                    FIO()->IsSupportedArchiveFormat(cbin.fileType())) {
                // Top level cell of an archive hierarchy.
                if ((cbin.phys() && cbin.phys()->isHierModified()) ||
                        (cbin.elec() && cbin.elec()->isHierModified())) {
                    if (!SaveHlpr::save_panic(&cbin))
                        retval = CmodFailed;
                }
            }
        }
        delete sgen;

        // Loop through again, and save any remaining modified
        // or saventv cells.

        sgen = new CDgenTab_cbin;
        while (sgen->next(&cbin)) {
            if (cbin.fileType() == Foa) {
                if (cbin.isModified()) {
                    // Panic saved as native cells.
                    if (!SaveHlpr::save_panic(&cbin))
                        retval = CmodFailed;
                }
            }
            else if (cbin.fileType() == Fnone ||
                    cbin.fileType() == Fnative) {
                if (cbin.isModified() || cbin.isSaventv()) {
                    if (cbin.isLibrary() && cbin.isDevice())
                        continue;
                    if (!SaveHlpr::save_panic(&cbin))
                        retval = CmodFailed;
                }
            }
            else {
                // Cells from a hierarchy, where an ancestor cell
                // was overwritten by non-archive cell.

                if (cbin.isModified()) {
                    cbin.setFileType(Fnone);
                    cbin.setFileName(0);
                    if (!SaveHlpr::save_panic(&cbin))
                        retval = CmodFailed;
                }
            }
        }
        delete sgen;
        return (retval);
    }

    // Make a list of all modified cells with an archive file type.
    SymTab *modtab = new SymTab(false, false);
    CDgenTab_cbin *sgen = new CDgenTab_cbin;
    CDcbin cbin;
    while (sgen->next(&cbin)) {
        if (cbin.isModified() &&
                FIO()->IsSupportedArchiveFormat(cbin.fileType()))
            modtab->add((uintptr_t)cbin.cellname(), 0, false);
    }
    delete sgen;

    stringlist *s0 = 0;
    sgen = new CDgenTab_cbin;
    while (sgen->next(&cbin)) {

        if (cbin.fileType() == Fnone || cbin.fileType() == Fnative) {
            if (cbin.isModified() || cbin.isSaventv()) {
                if (cbin.isLibrary() && cbin.isDevice())
                    continue;
                s0 = new stringlist(
                    lstring::copy(Tstring(cbin.cellname())), s0);
            }
        }
        else if (cbin.fileType() == Foa) {
            if (cbin.isModified()) {
                s0 = new stringlist(
                    lstring::copy(Tstring(cbin.cellname())), s0);
            }
        }
        else if (!cbin.isSubcell() &&
                FIO()->IsSupportedArchiveFormat(cbin.fileType())) {
            // Top level cell of hierarchy.
            if ((cbin.phys() && cbin.phys()->isHierModified()) ||
                    (cbin.elec() && cbin.elec()->isHierModified())) {
                s0 = new stringlist(
                    lstring::copy(Tstring(cbin.cellname())), s0);

                // Purge all modified archive cells in this hierarchy
                // from the modified table.

                CDgenHierDn_cbin gen(&cbin);
                CDcbin cbret;
                bool err;
                while (gen.next(&cbret, &err)) {
                    if (cbret.isModified() && 
                            FIO()->IsSupportedArchiveFormat(cbret.fileType()))
                        modtab->remove((uintptr_t)cbret.cellname());
                }
            }
        }
    }
    delete sgen;

    // Anything left in the modified table is a modified cell that
    // originated from an archive file, but is now parented by a
    // native or OA hierarchy.  Save these as native cells, and clear
    // file type.

    SymTabGen stgen(modtab);
    SymTabEnt *ent;
    while ((ent = stgen.next()) != 0) {
        if (CDcdb()->findSymbol((CDcellName)ent->stTag, &cbin)) {
            cbin.setFileType(Fnone);
            cbin.setFileName(0);
            s0 = new stringlist(
                lstring::copy(Tstring(cbin.cellname())), s0);
        }
    }
    delete modtab;

    if (s0) {
        switch (EditIf()->PopUpModified(s0, SaveHlpr::save_cb)) {
        case PMok:
            retval = CmodOK;
            break;
        case PMerr:
            retval = CmodFailed;
            break;
        case PMabort:
            retval = CmodAborted;
            break;
        }
        stringlist::destroy(s0);
    }
    return (retval);
}


namespace {
    bool is_oa_lib(const char *name)
    {
        bool is_open = false;  // don't care about this
        return (OAif()->hasOA() && OAif()->is_library(name, &is_open));
    }
}


// Function to save the current cell or hierarchy, possibly under a
// new name and/or format.  The file type defaults to the type from
// which the cell originated.  If no name is given, a new name is
// prompted for.  If silent_errors, don't use an error pop-up, put
// message in Errs, assume non-interactive.
//
// false is returned on Esc or error.
//
bool
cMain::SaveCellAs(const char *name, bool silent_errors)
{
    EV()->InitCallback();
    if (!DSP()->CurCellName()) {
        const char *msg = "No current cell, nothing to save!";
        if (silent_errors) {
            Errs()->add_error(msg);
            return (false);
        }
        PL()->ShowPrompt(msg);
        return (true);
    }
    CDcbin cbin(DSP()->CurCellName());

    if (!xm_saving_dev) {
        // Check if editing a library device and we were not called
        // from the device editor.  Divert to device editor if so.

        if (cbin.isLibrary() && cbin.isDevice()) {
            if (silent_errors) {
                Errs()->add_error("Can't save device library cell.");
                return (false);
            }
            ScedIf()->PopUpDevEdit(0, MODE_ON);
            return (true);
        }
    }

    FileType filetype = cbin.fileType();
#ifdef DBG_SAVE
    printf("%s\n", FIO()->TypeName(filetype));
#endif
    if (filetype == Fnone)
        filetype = Fnative;

    ScedIf()->connectAll(false);
    Selections.deselectTypes(CurCell(), 0);

    char *string = 0;
    if (name && *name)
        string = lstring::copy(name);

    bool showmsg = false;
    for (;;) {
        if (!string) {
            string = SaveHlpr::get_new_name(silent_errors); 
            if (!string) {
                PL()->ShowPrompt("Aborted.");
                return (false);
            }
        }
#ifdef DBG_SAVE
        printf("%s\n", string);
#endif

        if (!strcmp(string, DeviceLibName())) {
            // If the device library file is given, assume that the user
            // wants to save the current non-library cell as a device.
            //
            delete [] string;
            string = 0;
            if (silent_errors) {
                Errs()->add_error("Can't save device library cell.");
                return (false);
            }
            cbin.setDevice(true);
            cbin.setLibrary(true);
            ScedIf()->PopUpDevEdit(0, MODE_ON);
            return (true);
        }
        if (!strcmp(string, ModelLibName())) {
            // There are a lot of names that should be avoided, we check
            // this one since device.lib is accepted, and the user may
            // misremember it as "model.lib".

            delete [] string;
            string = 0;
            Errs()->add_error("Unacceptable cell name: %s.", ModelLibName());
            if (silent_errors)
                return (false);
            Log()->ErrorLog(save_file, Errs()->get_error());
            continue;
        }

        char *token1 = 0, *token2 = 0;
        FileType ft = SaveHlpr::interpret_response(string, &token1, &token2);
        delete [] string;
        string = 0;
#ifdef DBG_SAVE
        printf("%s %s\n", token1 ? token1 : "null", token2 ? token2 : "null");
#endif
        char buf[256];

        if (ft == Fnone) {
            // Unable to determine file type (error return).

            Errs()->add_error("Ambiguous or undetermined format type.");
            if (silent_errors)
                return (false);
            Log()->ErrorLog(save_file, Errs()->get_error());
            continue;
        }
        if (!showmsg) {
            showmsg = true;
            sLstr lstr;
            if (CDvdb()->getVariable(VA_StripForExport)) {
                lstr.add(
                    "The \"StripForExport\" variable is set, but will be "
                    "ignored\n"
                    "in the present operation.  You must use the Convert "
                    "write\n"
                    "functions to obtain stripped output.\n");
            }
            if (CDvdb()->getVariable(VA_KeepLibMasters)) {
                lstr.add(
                    "The \"KeepLibMasters\" variable is set, but will be "
                    "ignored\n"
                    "in the present operation.  You must use the Convert "
                    "write\n"
                    "functions to include library cells in output.\n");
            }
            if (CDvdb()->getVariable(VA_SkipInvisible)) {
                lstr.add(
                    "The \"SkipInvisible\" variable is set, but will be "
                    "ignored\n"
                    "in the present operation.  You must use the Convert "
                    "write\n"
                    "functions to skip invisible layers in output.\n");
            }
            if (CDvdb()->getVariable(VA_PCellKeepSubMasters)) {
                lstr.add(
                    "The \"PCellKeepSubMasters\" variable is set, but will "
                    "be ignored\n"
                    "in the present operation.  You must use the Convert "
                    "write\n"
                    "functions to include pcell sub-masters in output.\n");
            }
            if (CDvdb()->getVariable(VA_ViaKeepSubMasters)) {
                lstr.add(
                    "The \"ViaKeepSubMasters\" variable is set, but will "
                    "be ignored\n"
                    "in the present operation.  You must use the Convert "
                    "write\n"
                    "functions to include standard via sub-masters in output.\n");
            }
            if (CDvdb()->getVariable(VA_Out32nodes)) {
                lstr.add(
                    "The \"Out32nodes\" variable is set, but will be "
                    "ignored\n"
                    "in the present operation.  You must use the Convert "
                    "write\n"
                    "functions to use Gen-3 node syntax in output.\n");
            }
            if (lstr.string())
                Log()->PopUpWarn(lstr.string());
        }
        if (ft != Fgds && ft != Fcgx && token1) {
            // Don't allow a ".gz" suffix on files that aren't GDSII
            // or CGX.

            char *t = strrchr(token1, '.');
            if (t && lstring::cieq(t, ".gz")) {
                Errs()->add_error(
                    "The \".gz\" extension is not allowed on %s names.",
                    FIO()->TypeName(ft));
                delete [] token1;
                delete [] token2;
                if (silent_errors)
                    return (false);
                Log()->ErrorLog(save_file, Errs()->get_error());
                continue;
            }
        }
        if (ft != filetype && !silent_errors) {
            // User is coercing a different format from the original.  Will
            // ask for confirmation.

            const char *prmsg_h =
                "Save current cell hierarchy in %s format? ";
            const char *prmsg_c =
                "Save current cell (only) in %s format? ";
            const char *prmsg_hoa =
                "Save current cell hierarchy to OpenAccess library %s? ";
            const char *prmsg_coa =
                "Save current cell (only) to OpenAccess library %s? ";

            if (FIO()->IsSupportedArchiveFormat(ft))
                sprintf(buf, prmsg_h, FIO()->TypeName(ft));
            else if (ft == Fnative) {
                if (token2 && !strcmp(token2, "*"))
                    sprintf(buf, prmsg_h, FIO()->TypeName(ft));
                else
                    sprintf(buf, prmsg_c, FIO()->TypeName(ft));
            }
            else if (ft == Foa) {
                const char *t1 = token1;
                const char *t2 = token2;
                if (!t1)
                    t1 = "";
                else if (t1[0] == '*' && t1[1] == 0) {
                    t2 = t1;
                    t1 = "";
                }
                if (t2 && !strcmp(t2, "*"))
                    sprintf(buf, prmsg_hoa, t1);
                else
                    sprintf(buf, prmsg_coa, t1);
            }
            else {
                // Can't get here.
                Errs()->add_error("Unknown format!  Internal error.");
                delete [] token1;
                delete [] token2;
                Log()->ErrorLog(save_file, Errs()->get_error());
                continue;
            }

            char *s = PL()->EditPrompt(buf, "y");
            s = lstring::strip_space(s);
            if (!s) {
                // Esc entered, user abort.
                delete [] token1;
                delete [] token2;
                PL()->ShowPrompt("Aborted.");
                return (false);
            }
            if (*s != 'y') {
                delete [] token1;
                delete [] token2;
                continue;
            }
        }
        if (ft == Foa) {
            if (!OAif()->hasOA()) {
                Errs()->add_error("OpenAccess database not available.");
                if (!silent_errors) {
                    Log()->ErrorLog(save_file, Errs()->get_error());
                    continue;
                }
                return (false);
            }
            if (!token1) {
                const char *s = CDvdb()->getVariable(VA_OaDefLibrary);
                token1 = lstring::gettok(&s);
            }
            else if (!token2) {
                if (token1[0] == '*' && token1[1] == 0) {
                    token2 = token1;
                    const char *s = CDvdb()->getVariable(VA_OaDefLibrary);
                    token1 = lstring::gettok(&s);
                }
            }
            if (!token1) {
                Errs()->add_error(
                    "Can't save %s, no library given and no default.",
                    DSP()->CurCellName());
                delete [] token1;
                delete [] token2;
                if (silent_errors)
                    return (false);
                Log()->ErrorLog(save_file, Errs()->get_error());
                    continue;
            }
            else if (!is_oa_lib(token1)) {
                sprintf(buf, "Library %s does not exist, create it? ",
                    token1);
                char *s = PL()->EditPrompt(buf, "y");
                s = lstring::strip_space(s);
                if (!s) {
                    // Esc entered, user abort.
                    delete [] token1;
                    delete [] token2;
                    PL()->ShowPrompt("Aborted.");
                    return (false);
                }
                if (*s != 'y') {
                    delete [] token1;
                    delete [] token2;
                    continue;
                }
                const char *oldlib = 0;
                if (cbin.fileType() == Foa)
                    oldlib = cbin.fileName();
                if (!OAif()->create_lib(token1, oldlib)) {
                    Errs()->add_error("OpenAccess library creation failed.");
                    delete [] token1;
                    delete [] token2;
                    if (silent_errors)
                        return (false);
                    Log()->ErrorLog(save_file, Errs()->get_error());
                    continue;
                }
            }
        }

        bool ret = SaveHlpr::save_cell_as(DSP()->CurCellName(), ft,
            token1, token2, silent_errors);
        delete [] token1;
        delete [] token2;
        if (!ret) {
            if (silent_errors)
                return (false);
            // An error message has been popped up in save_cell_as.
            continue;
        }
        break;
    }
    return (true);
}


// Called when cell is saved, clears undo list and recomputes BB.
//
void
cMain::CommitCell(bool keep_undo)
{
    EditIf()->ulListFinalize(keep_undo);

    // Recompute the BB.
    CDs *cursd = CurCell(true);
    if (cursd) {
        cursd->computeBB();
        cursd->reflect();
    }
}
// End of cMain functions.


namespace {
    // Return the name of a subdirectory to use to save modified cells.
    // Returns 0 if directory creation fails.
    //
    const char *get_panicdir()
    {
        static char panicdir[32];
        if (panicdir[0]) {
            if (panicdir[0] == 1)
                return (0);
            return (panicdir);
        }

        int pid = getpid();
        sprintf(panicdir, "panic.%d", pid);
        int cnt = 0;
        for (;;) {
            GFTtype g = filestat::get_file_type(panicdir);
            if (g == GFT_NONE) {
#ifdef WIN32
                if (mkdir(panicdir) < 0)
#else
                if (mkdir(panicdir, 0755) < 0)
#endif
                {
                    perror("mkdir");
                    fprintf(stderr,
                        "Can't create panic directory, no cells saved.\n");
                    panicdir[0] = 1;
                }
                break;
            }
            else if (g == GFT_DIR)
                break;
            else {
                cnt++;
                sprintf(panicdir + strlen(panicdir), "_%d", cnt);
            }
        }
        return (panicdir);
    }
}


// Called when dumping modified cells during a program abort.
//
// Modified cells or hierarchies are saved in a "panic.pid" subdirectory.
//
bool
SaveHlpr::save_panic(CDcbin *cbin)
{
    const char *dn = get_panicdir();
    if (!dn)
        return (false);
    XM()->SetPanicDir(dn);

    char buf[256];
    sprintf(buf, "%s/%s", dn, Tstring(cbin->cellname()));
    FileType ft = cbin->fileType();
    if (ft != Fnone && ft != Fnative && ft != Foa) {
        const char *ext = FIO()->GetTypeExt(ft);
        if (ext)
            strcat(buf, ext);
    }  

    Errs()->init_error();

    if (FIO()->IsSupportedArchiveFormat(ft)) {
        PP()->ClearContext();
        bool ret = write_export(cbin->cellname(), buf, ft);
        return (ret);
    }
    else if (ft == Fnative || ft == Fnone || ft == Foa) {
        // OpenAccess cells are saved as a native cell, NOT back into
        // the OA database.

        if (!cbin->isSubcell())
            EditIf()->assignGlobalProperties(cbin);
        bool ret = FIO()->WriteNative(cbin, buf);
        const char *cn = Tstring(cbin->cellname());
        if (ret) {
            PL()->ShowPromptV("%s saved.", cn);
            if (XM()->PanicFp())
                fprintf(XM()->PanicFp(), "%s saved.\n", cn);
        }
        else {
            PL()->ShowPromptV("Error occurred: Can't save %s.", cn);
            if (XM()->PanicFp()) {
                Errs()->add_error("Error occurred: Can't save %s", cn);
                fprintf(XM()->PanicFp(), "%s\n", Errs()->get_error());
            }
        }
        return (ret);
    }
    return (false);
}


// Function to save cell or hierarchy, passed to dialog.
//
bool
SaveHlpr::save_cb(const char *name)
{
    if (!name || !*name)
        return (false);
    CDcbin cbin;
    if (!CDcdb()->findSymbol(name, &cbin))
        return (false);

    Errs()->init_error();
    bool ret = false;
    if (cbin.fileType() == Fgds) {
        char *fname = FIO()->DefaultFilename(&cbin, cbin.fileType());
        PP()->ClearContext();
        ret = write_export(cbin.cellname(), fname, Fgds);
        delete [] fname;
    }
    else if (cbin.fileType() == Fcgx) {
        char *fname = FIO()->DefaultFilename(&cbin, cbin.fileType());
        PP()->ClearContext();
        ret = write_export(cbin.cellname(), fname, Fcgx);
        delete [] fname;
    }
    else if (cbin.fileType() == Foas) {
        char *fname = FIO()->DefaultFilename(&cbin, cbin.fileType());
        PP()->ClearContext();
        ret = write_export(cbin.cellname(), fname, Foas);
        delete [] fname;
    }
    else if (cbin.fileType() == Fcif) {
        char *fname = FIO()->DefaultFilename(&cbin, cbin.fileType());
        PP()->ClearContext();
        ret = write_export(cbin.cellname(), fname, Fcif);
        delete [] fname;
    }
    else if (cbin.fileType() == Fnative || cbin.fileType() == Fnone) {
        char *fname = FIO()->DefaultFilename(&cbin, cbin.fileType());
        ScedIf()->connectAll(false);
        if (!cbin.isSubcell())
            EditIf()->assignGlobalProperties(&cbin);
        ret = FIO()->WriteNative(&cbin, fname);
        if (ret) {
            if (cbin.cellname() == DSP()->TopCellName() &&
                    cbin.cellname() == DSP()->CurCellName() &&
                    cbin.elec() && !cbin.elec()->isEmpty()) {
                FILE *fp = fopen(fname, "a");
                if (fp) {
                    ScedIf()->dumpSpiceDeck(fp);
                    fclose(fp);
                }
            }
            PL()->ShowPromptV("%s saved.", cbin.cellname());
            if (cbin.phys() && !cbin.phys()->reflect()) {
                Errs()->add_error("reflect failed");
                Log()->ErrorLog(save_file, Errs()->get_error());
            }
            if (cbin.elec() && !cbin.elec()->reflect()) {
                Errs()->add_error("reflect failed");
                Log()->ErrorLog(save_file, Errs()->get_error());
            }
        }
        else {
            PL()->ShowPromptV("Error occurred: Can't save %s.",
                cbin.cellname());
            Errs()->add_error("Error occurred: Can't save %s.",
                cbin.cellname());
            Log()->ErrorLog(save_file, Errs()->get_error());
        }
        delete [] fname;
    }
    else if (cbin.fileType() == Foa) {
        ret = OAif()->save(&cbin, cbin.fileName());
    }
    XM()->ShowParameters();
    return (ret);
}


// Function to obtain a new file/cell name, for the current cell. 
// Prompt the user for an alternate, unless no_prompt is true.  Return
// must be freed!
//
char *
SaveHlpr::get_new_name(bool no_prompt)
{
    if (!DSP()->CurCellName())
        return (0);
    CDcbin cbin(DSP()->CurCellName());
    FileType ft = cbin.fileType();

    char *in = 0;
    if (ft == Fnative || ft == Fnone) {
        char *fn = 0;
        if (cbin.fileType() == Fnative && cbin.fileName()) {
            fn = new char[strlen(cbin.fileName()) +
                strlen(Tstring(cbin.cellname())) + 2];
            sprintf(fn, "%s/%s", cbin.fileName(), Tstring(cbin.cellname()));
        }
        if (!fn)
            fn = lstring::copy(Tstring(cbin.cellname()));
        if (no_prompt)
            return (fn);
        in = XM()->SaveFileDlg("Save as native: ", fn);
        delete [] fn;
    }
    else if (ft == Fgds) {
        char *fn = FIO()->DefaultFilename(&cbin, ft);
        if (no_prompt)
            return (fn);
        char *t = strrchr(fn, '.');
        if (t && lstring::cieq(t, ".gz"))
            in = XM()->SaveFileDlg("Save as gzipped GDSII: ", fn);
        else
            in = XM()->SaveFileDlg("Save as GDSII: ", fn);
        delete [] fn;
    }
    else if (ft == Fcgx) {
        char *fn = FIO()->DefaultFilename(&cbin, ft);
        if (no_prompt)
            return (fn);
        char *t = strrchr(fn, '.');
        if (t && lstring::cieq(t, ".gz"))
            in = XM()->SaveFileDlg("Save as gzipped CGX: ", fn);
        else
            in = XM()->SaveFileDlg("Save as CGX: ", fn);
        delete [] fn;
    }
    else if (ft == Foas) {
        char *fn = FIO()->DefaultFilename(&cbin, ft);
        if (no_prompt)
            return (fn);
        in = XM()->SaveFileDlg("Save as OASIS: ", fn);
        delete [] fn;
    }
    else if (ft == Fcif) {
        char *fn = FIO()->DefaultFilename(&cbin, ft);
        if (no_prompt)
            return (fn);
        in = XM()->SaveFileDlg("Save as CIF: ", fn);
        delete [] fn;
    }
    else if (ft == Foa) {
        char buf[256];
        if (cbin.fileType() == Foa && cbin.fileName()) {
            sprintf(buf, "oa %s %s", cbin.fileName(),
                Tstring(cbin.cellname()));
        }
        else {
            // Shouldn't happen.
            sprintf(buf, "oa %s %s", "xic_unknown",
                Tstring(cbin.cellname()));
        }
        if (no_prompt)
            return (lstring::copy(buf));
        in = PL()->EditPrompt("Save in OpenAccess library/cell: ", buf);
    }
    return (lstring::copy(in));
}


// Parse and interpret the response string, obtaining the format
// type, and a file/directory path or library, and a cell name. 
// On success, the return value is a format code other than Fnone,
// Fnone is returned on error or if the format type can't be
// determined.
//
// The response string has the general format
//
//   [ftype] path_or_libname [cellname]
//
// The optional ftype is a file type extension, without the
// period, and including "oa" for OpenAccess.
//
// The interpretation depends on file type.  For native and OA, if
// cellname is "*", the whole hierarchy will be saved, otherwise only
// the current cell will be saved.
//
FileType
SaveHlpr::interpret_response(const char *resp, char **ppath, char **pcname)
{
    char *tok1 = lstring::getqtok(&resp);
    char *tok2 = lstring::getqtok(&resp);
    char *tok3 = lstring::getqtok(&resp);
    if (!tok1) {
        Errs()->add_error("No argument given!");
        return (Fnone);
    }

    FileType ft = Fnone;
    if (!lstring::strdirsep(tok1) && !strchr(tok1, '.') && strlen(tok1) < 7) {
        // See if the first token is a format specifier.
        if (lstring::cieq(tok1, "oa"))
            ft = Foa;
        else {
            char tbuf[12];
            memset(tbuf, 0, 12);
            tbuf[0] = '.';
            strncpy(tbuf+1, tok1, 8);
            ft = FIO()->TypeExt(tbuf);
        }
    }
    if (ft != Fnone) {
        // The format specifier was given.
        delete [] tok1;
        tok1 = tok2;
        tok2 = tok3;
        tok3 = 0;
        if (!tok1) {
            if (ft == Foa)
                return (ft);
            Errs()->add_error("No argument given!");
            return (Fnone);
        }
    }
    else {
        // No format specifier, try to obtain format from file extension.
        ft = FIO()->TypeExt(tok1);
        if (ft == Fnone) {
            CDcbin cbin(DSP()->CurCellName());

            // No file extension match, only possibilities are OA and
            // native.
            if (filestat::is_directory(tok1))
                // Match to existing subdirectory.
                ft = Fnative;
            else if (lstring::strdirsep(tok1))
                // Looks like a file path, must be native.
                ft = Fnative;
            else if (is_oa_lib(tok1))
                // Match to existing OA library.
                ft = Foa;
            else if (cbin.fileType() == Foa)
                ft = Foa;
            else
                ft = Fnative;
        }
    }

    if (FIO()->IsSupportedArchiveFormat(ft)) {
        if (tok2) {
            Log()->WarningLogV(mh::InputOutput,
                "Unexpected argument \"%s\" ignored.", tok2);
            delete [] tok2;
            tok2 = 0;
        }
        *ppath = pathlist::expand_path(tok1, false, false);
        delete [] tok1;
        *pcname = 0;
    }
    else if (ft == Fnative) {
        *ppath = tok1;
        *pcname = tok2;
    }
    else if (ft == Foa) {
        if (lstring::strdirsep(tok1)) {
            delete [] tok1;
            delete [] tok2;
            Errs()->add_error(
                "Directory separator in OpenAccess library name not allowed.");
            return (Fnone);
        }
        *ppath = tok1;
        *pcname = tok2;
    }
    return (ft);
}


namespace {
    void emit_error(const char *what, bool silent_errors)
    {
        Errs()->add_error("%s returned error.", what);
        if (!silent_errors)
            Log()->ErrorLog(save_file, Errs()->get_error());
    }
}


// Back end for cMain::SaveCellAs, takes care of the actual save
// operation.
//
bool
SaveHlpr::save_cell_as(CDcellName cname, FileType ft, const char *token1,
    const char *token2, bool silent_errors)
{
    const char *msg1 = "Writing cell.  Please wait.";

    bool ret = true;
    PL()->ShowPrompt(msg1);
    Errs()->init_error();
    DSPpkg::self()->SetWorking(true);

    // Finalize the cell before save.  The undo list is retained,
    // so recent changes can be undone after the save.  However,
    // the modification logic is changed so that undo will increment
    // the modification count.

    XM()->CommitCell(true);

    // NOTE:  Update fileName and/or fileType after save?
    //
    // If fileType is Fnone, it will be set to the saved type, and
    // fileName will be set.  If not Fnone, it will never be changed
    // (unless the cell is overwritten in a subsequent read).
    //
    // Neither field is changed if there is a format or cell name
    // change.
    //
    // If a cell hierarchy is saved to an archive file of the original
    // type, the fileName of all cells in the hierarchy is updated.
    //
    // If a native cell or hierarchy is saved to a new directory, the
    // fileName, which is actually the directory path, is updated.
    //
    // If an OA cell or hierarchy is saved to a new OA library, the
    // fileName, which is actually the library name, is updated.

    CDcbin cbin(cname);
    if (FIO()->IsSupportedArchiveFormat(ft)) {
        if (!write_export(cname, token1, ft)) {
            emit_error("write_export", silent_errors);
            ret = false;
        }
        else {
            const char *msg3 = "Hierarchy has been saved in %s.";
            PL()->ShowPromptV(msg3, token1);
        }
    }
    else if (ft == Fnative) {
        if (!token1) {
            // Save current cell in current directory.

            char *cwd = getcwd(0, 0);
            ret = write_native(cbin, cwd, 0);
            if (!ret)
                emit_error("write_native", silent_errors);
            free(cwd);
        }
        else if (*token1 == '*') {
            if (*(token1+1) || token2) {
                // extra junk
                Errs()->add_error(
                    "Native save failed, command syntax, unknown "
                    "junk after \'*\'.");
                if (!silent_errors)
                    Log()->ErrorLog(save_file, Errs()->get_error());
                ret = false;
            }
            else {
                // Save current hierarchy in current directory as
                // native cells.

                char *cwd = getcwd(0, 0);
                ret = write_native(cbin, cwd, "*");
                if (!ret)
                    emit_error("write_native", silent_errors);
                free(cwd);
            }
        }
        else if (!token2) {
            switch(filestat::get_file_type(token1)) {
            case GFT_NONE:
            case GFT_FILE:
            case GFT_OTHER:
                {
                    // This is a new name for the cell, may have a path
                    char *dir = lstring::copy(token1);
                    char *file = lstring::strrdirsep(dir);
                    if (file) {
                        *file++ = 0;
                        ret = write_native(cbin, dir, file);
                        if (!ret)
                            emit_error("write_native", silent_errors);
                    }
                    else {
                        char *cwd = getcwd(0, 0);
                        ret = write_native(cbin, cwd, token1);
                        if (!ret)
                            emit_error("write_native", silent_errors);
                        free(cwd);
                    }
                    delete [] dir;
                }
                break;
                
            case GFT_DIR:
                // If token1 is an existing directory, save current
                // cell in that directory.

                ret = write_native(cbin, token1, 0);
                if (!ret)
                    emit_error("write_native", silent_errors);
                break;
            }
        }
        else {
            // The token1 must be a directory, create if needed,
            // save current cell or hierarchy cells there.

            switch(filestat::get_file_type(token1)) {
            case GFT_NONE:
#ifdef WIN32
                ret = (mkdir(token1) == 0);
#else
                ret = (mkdir(token1, 0755) == 0);
#endif
                if (!ret) {
                    Errs()->add_error(
                        "Native save failed, can't create directory %s.", 
                        token1);
                    if (!silent_errors)
                        Log()->ErrorLog(save_file, Errs()->get_error());
                }
                break;
                
            case GFT_FILE:
            case GFT_OTHER:
                Errs()->add_error(
                    "Native save failed, %s is not a directory.", token1);
                if (!silent_errors)
                    Log()->ErrorLog(save_file, Errs()->get_error());
                ret = false;
                break;

            case GFT_DIR:
                // If token1 is an existing directory, will save in
                // that directory.

                break;
            }
            if (ret) {
                // Token2 should be a simple name, if there is a path
                // we will strip it.
                ret = write_native(cbin, token1, lstring::strip_path(token2));
                if (!ret)
                    emit_error("write_native", silent_errors);
            }
        }
    }
    else if (ft == Foa) {
        if (!OAif()->hasOA()) {
            Errs()->add_error("OpenAccess database not available.");
            if (!silent_errors)
                Log()->ErrorLog(save_file, Errs()->get_error());
            XM()->ShowParameters();
            DSPpkg::self()->SetWorking(false);
            return (false);
        }
        // Assign internal properties to the top level cell.
        if (!cbin.isSubcell())
            EditIf()->assignGlobalProperties(&cbin);

        if (!token1) {
            // Save cell in library named in OaDefLibrary variable.

            const char *s = CDvdb()->getVariable(VA_OaDefLibrary);
            char *libname = lstring::gettok(&s);
            if (!libname) {
                Errs()->add_error(
                    "Can't save %s, no library given and no default.",
                    Tstring(cbin.cellname()));
                if (!silent_errors)
                    Log()->ErrorLog(save_file, Errs()->get_error());
                ret = false;
            }
            else if (!is_oa_lib(libname)) {
                Errs()->add_error("Can't save %s, unknown library %s.",
                    Tstring(cbin.cellname()), libname);
                if (!silent_errors)
                    Log()->ErrorLog(save_file, Errs()->get_error());
                ret = false;
            }
            else {
                if (!OAif()->save(&cbin, libname)) {
                    Errs()->add_error(
                        "Error occurred: can't save %s in OA lib %s.",
                        Tstring(cbin.cellname()), libname);
                    if (!silent_errors)
                        Log()->ErrorLog(save_file, Errs()->get_error());
                    ret = false;
                }
                else {
                    PL()->ShowPromptV(
                        "Current cell has been saved as %s in OA lib %s.",
                        Tstring(cbin.cellname()), libname);
                }
                delete [] libname;
            }
        }
        else if (!token2) {
            if (token1[0] == '*' && token1[1] == 0) {
                // Save cell hier. in library named in OaDefLibrary variable.

                const char *s = CDvdb()->getVariable(VA_OaDefLibrary);
                char *libname = lstring::gettok(&s);
                if (!libname) {
                    Errs()->add_error(
                        "Can't save %s, no library given and no default.",
                        Tstring(cbin.cellname()));
                    if (!silent_errors)
                        Log()->ErrorLog(save_file, Errs()->get_error());
                    ret = false;
                }
                else if (!is_oa_lib(libname)) {
                    Errs()->add_error(
                        "Can't save %s hierarchy, unknown library %s.",
                        Tstring(cbin.cellname()), libname);
                    if (!silent_errors)
                        Log()->ErrorLog(save_file, Errs()->get_error());
                    ret = false;
                }
                else {
                    if (!OAif()->save(&cbin, libname, true)) {
                        Errs()->add_error(
                            "OpenAccess hierarchy save returned error.");
                        if (!silent_errors)
                            Log()->ErrorLog(save_file, Errs()->get_error());
                        ret = false;
                    }
                    else
                        PL()->ShowPromptV("Hierarchy saved OA lib %s.",
                            libname);
                    delete [] libname;
                }
            }
            else {
                // Save cell in library named in token1.

                if (!is_oa_lib(token1)) {
                    Errs()->add_error("Can't save %s, unknown library %s.",
                        Tstring(cbin.cellname()), token1);
                    if (!silent_errors)
                        Log()->ErrorLog(save_file, Errs()->get_error());
                    ret = false;
                }
                else if (!OAif()->save(&cbin, token1)) {
                    Errs()->add_error(
                        "Error occurred: can't save %s in OA lib %s.",
                        Tstring(cbin.cellname()), token1);
                    if (!silent_errors)
                        Log()->ErrorLog(save_file, Errs()->get_error());
                    ret = false;
                }
                else {
                    PL()->ShowPromptV(
                        "Current cell has been saved as %s in OA lib %s.",
                        Tstring(cbin.cellname()), token1);
                }
            }
        }
        else {
            if (token2[0] == '*' && token2[1] == 0) {
                // Save cell hier. in library named in token1.

                if (!is_oa_lib(token1)) {
                    Errs()->add_error(
                        "Can't save %s hierarchy, unknown library %s.",
                        Tstring(cbin.cellname()), token1);
                    if (!silent_errors)
                        Log()->ErrorLog(save_file, Errs()->get_error());
                    ret = false;
                }
                else if (!OAif()->save(&cbin, token1, true)) {
                    Errs()->add_error(
                        "OpenAccess hierarchy save returned error.");
                    if (!silent_errors)
                        Log()->ErrorLog(save_file, Errs()->get_error());
                    ret = false;
                }
                else
                    PL()->ShowPromptV("Hierarchy saved OA lib %s.", token1);
            }
            else {
                // Save cell in library named in token1 as token2, create lib
                // if it doesn't exist.

                if (!OAif()->save(&cbin, token1, false, token2)) {
                    Errs()->add_error(
                        "Error occurred: can't save %s in OA lib %s.",
                        token2, token1);
                    if (!silent_errors)
                        Log()->ErrorLog(save_file, Errs()->get_error());
                    ret = false;
                }
                else {
                    PL()->ShowPromptV(
                        "Current cell has been saved as %s in OA lib %s.",
                        token2, token1);
                }
            }
        }
    }

    XM()->ShowParameters();
    DSPpkg::self()->SetWorking(false);
    return (ret);
}


// Write cbin and possibly its hierarchy as native symbol files.  If
// newname == "*", all cells in the hierarchy are written.  If newname
// is "." or null or empty, the cell has no name change, otherwise the
// cell is saved as newname.
//
bool
SaveHlpr::write_native(CDcbin &cbin, const char *dir, const char *newname)
{
    // Assign internal properties to the top level cell.
    if (!cbin.isSubcell())
        EditIf()->assignGlobalProperties(&cbin);

    if (lstring::strdirsep(newname)) {
        Errs()->add_error(
            "write_native: cell name contains directory separators.");
        return (false);
    }

    if (!dir)
        dir = ".";
    const CDcellName cname = cbin.cellname();

    char *tn = lstring::getqtok(&newname);
    GCarray<char*> gc_tn(tn);
    newname = tn;

    bool allfiles = false;
    if (newname) {
        if (newname[0] == '.' && newname[1] == 0)
            newname = 0;
        else if (newname[0] == '*' && newname[1] == 0)
            allfiles = true;
    }
    if (allfiles) {
        // Write all cells in hierarchy as native.

        bool ret = write_export(cname, dir, Fnative);
        if (!ret)
            Errs()->add_error("write_export returned error.");
        else {
            const char *msg3 = "Native cell hierarchy saved under \"%s\".";
            PL()->ShowPromptV(msg3, dir);
        }
        return (ret);
    }
    if (newname && strcmp(newname, Tstring(cbin.cellname()))) {
        // Don't allow writing a native cell that would conflict
        // with the name of an existing cell in memory.

        if (CDcdb()->findSymbol(newname)) {
            Errs()->add_error("Name %s is already in use.", newname);
            return (false);
        }
    }

    sLstr lstr;
    lstr.add(dir);
    lstr.add_c('/');
    if (newname)
        lstr.add(newname);
    else
        lstr.add(Tstring(cname));

    bool ret = FIO()->WriteNative(&cbin, lstr.string());
    if (!ret)
        Errs()->add_error("Error occurred: can't save %s.", lstr.string());
    else {
        const char *newcn = lstring::strip_path(lstr.string());

        if (!newcn || !strcmp(newcn, Tstring(cname))) {
            // Update fileName field if cell name not changed.

            char *p = pathlist::expand_path(lstr.string(), true, true);
            char *t = lstring::strrdirsep(p);
            if (t) {
                // Strip cell name, save dir. path only.
                *t = 0;
                cbin.setFileName(p);
            }
            delete [] p;
        }
        if (cname == DSP()->TopCellName() &&
                cbin.elec() && !cbin.elec()->isEmpty()) {
            // Add SPICE trailer.
            FILE *fp = FIO()->POpen(lstr.string(), "a");
            if (fp) {
                ScedIf()->dumpSpiceDeck(fp);
                fclose(fp);
            }
        }
        const char *msg2 = "Current cell %s has been saved as %s.";
        PL()->ShowPromptV(msg2, Tstring(cname), lstr.string());
    }
    return (ret);
}


// This function writes both physical and electrical data to the file,
// overriding StripForExport and SkipInvisible.  Applies to archive
// and native formats only, not OpenAccess.
//
bool
SaveHlpr::write_export(CDcellName cellname, const char *fname, FileType ftype)
{
    if (!FIO()->IsSupportedArchiveFormat(ftype) && ftype != Fnative)
        return (false);
    if (!cellname)
        return (false);
    CDcbin cbin(cellname);
    if (!cbin.cellname())
        return (false);
    if (!cbin.isSubcell())
        EditIf()->assignGlobalProperties(&cbin);
    stringlist *namelist =
        new stringlist(lstring::copy(Tstring(cellname)), 0);
    GCdestroy<stringlist> gc_namelist(namelist);

    bool tmp[7];
    tmp[0] = FIO()->IsStripForExport();
    FIO()->SetStripForExport(false);
    tmp[1] = FIO()->IsKeepLibMasters();
    FIO()->SetKeepLibMasters(false);
    tmp[2] = FIO()->IsSkipInvisiblePhys();
    FIO()->SetSkipInvisiblePhys(false);
    tmp[3] = FIO()->IsSkipInvisibleElec();
    FIO()->SetSkipInvisibleElec(false);
    tmp[4] = FIO()->IsKeepPCellSubMasters();
    FIO()->SetKeepPCellSubMasters(false);
    tmp[5] = FIO()->IsKeepViaSubMasters();
    FIO()->SetKeepViaSubMasters(false);
    tmp[6] = CD()->Out32nodes();
    CD()->SetOut32nodes(false);

    FIOcvtPrms prms;
    prms.set_destination(fname, ftype);

    bool ret = false;
    switch (ftype) {
    case Fgds:
        ret = FIO()->ConvertToGds(namelist, &prms);
        break;
    case Foas:
        ret = FIO()->ConvertToOas(namelist, &prms);
        break;
    case Fcif:
        ret = FIO()->ConvertToCif(namelist, &prms);
        break;
    case Fcgx:
        ret = FIO()->ConvertToCgx(namelist, &prms);
        break;
    case Fnative:
        ret = FIO()->ConvertToNative(namelist, &prms);
        break;
    }

    FIO()->SetStripForExport(tmp[0]);
    FIO()->SetKeepLibMasters(tmp[1]);
    FIO()->SetSkipInvisiblePhys(tmp[2]);
    FIO()->SetSkipInvisibleElec(tmp[3]);
    FIO()->SetKeepPCellSubMasters(tmp[4]);
    FIO()->SetKeepViaSubMasters(tmp[5]);
    CD()->SetOut32nodes(tmp[6]);

    if (ret) {
        if (FIO()->IsSupportedArchiveFormat(ftype)) {
            CDgenHierDn_cbin gen(&cbin);
            bool err;
            CDcbin tcbin;
            while (gen.next(&tcbin, &err)) {
                if (tcbin.phys())
                    tcbin.phys()->clearModified();
                if (tcbin.elec())
                    tcbin.elec()->clearModified();
            }
        }
        // Fnative and Foa cells have clearModified called in writer.

        if (!XM()->PanicFp()) {
            if (cbin.fileType() == ftype && ftype != Fnative) {
                // Update the file name.
                char *pathname;
                FILE *fp = FIO()->POpen(fname, "r", &pathname);
                if (fp) {
                    fclose(fp);
                    cbin.updateFileName(pathname);
                }
                delete [] pathname;
            }
        }
    }
    return (ret);
}

