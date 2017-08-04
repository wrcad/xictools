
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
#include "editif.h"
#include "fio.h"
#include "fio_chd.h"
#include "fio_gdsii.h"
#include "fio_alias.h"
#include "cd_digest.h"
#include "scedif.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "cvrt_menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "tech.h"
#include "layertab.h"
#include "tech_kwords.h"
#include "filestat.h"
#include "pathlist.h"

#include <sys/stat.h>


namespace {
    namespace cvrt_menu {
        MenuFunc  M_Export;
        MenuFunc  M_Import;
        MenuFunc  M_Convert;
        MenuFunc  M_Assemble;
        MenuFunc  M_Diff;
        MenuFunc  M_Cut;
        MenuFunc  M_TextEdit;
    }
}

using namespace cvrt_menu;

namespace {
    MenuEnt CvrtMenu[cvrtMenu_END + 1];
    MenuBox CvrtMenuBox;
}

MenuBox *
cMain::createCvrtMenu()
{
    CvrtMenu[cvrtMenu] =
        MenuEnt(&M_NoOp,     "",        ME_MENU,     CMD_SAFE,
        0);
    CvrtMenu[cvrtMenuExprt] =
        MenuEnt(&M_Export,   MenuEXPRT, ME_TOGGLE,   CMD_SAFE,
        MenuEXPRT": Write cell data to file.");
    CvrtMenu[cvrtMenuImprt] =
        MenuEnt(&M_Import,   MenuIMPRT, ME_TOGGLE,   CMD_SAFE,
        MenuIMPRT": Read cell data from file.");
    CvrtMenu[cvrtMenuConvt] =
        MenuEnt(&M_Convert,  MenuCONVT, ME_TOGGLE,   CMD_SAFE,
        MenuCONVT": Convert file formats, do not touch database.");
    CvrtMenu[cvrtMenuAssem] =
        MenuEnt(&M_Assemble, MenuASSEM, ME_TOGGLE,   CMD_SAFE,
        MenuASSEM": layout file assemble/merge tool.");
    CvrtMenu[cvrtMenuDiff] =
        MenuEnt(&M_Diff,     MenuDIFF,  ME_TOGGLE | ME_SEP, CMD_SAFE,
        MenuDIFF": compare layouts.");
    CvrtMenu[cvrtMenuCut] =
        MenuEnt(&M_Cut,      MenuCUT,   ME_VANILLA,  CMD_NOTSAFE,
        MenuCUT": export rectangular region.");
    CvrtMenu[cvrtMenuTxted] =
        MenuEnt(&M_TextEdit, MenuTXTED, ME_VANILLA,  CMD_SAFE,
        MenuTXTED": Pop up a text editor containing the current cell file.");
    CvrtMenu[cvrtMenu_END] =
        MenuEnt();

    CvrtMenuBox.name = "Convert";
    CvrtMenuBox.menu = CvrtMenu;
    return (&CvrtMenuBox);
}


namespace {
    // Check if we are about to write a file with a non-matching type
    // extension.  Return true if the user decides to abort.
    //
    bool
    check_fname(const char *s, FileType dst_ft)
    {
        if (!s)
            return (false);
        FileType tp = FIO()->TypeExt(s);
        if (tp != Fnone && tp != dst_ft) {
            char buf[128];
            sprintf(buf, "Warning: writing %s format with an extension "
                "reserved for %s.  Continue? ",
                FIO()->TypeName(dst_ft), FIO()->TypeName(tp));
            char *in = PL()->EditPrompt(buf, "n");
            in = lstring::strip_space(in);
            if (!in || (*in != 'y' && *in != 'Y')) {
                PL()->ShowPrompt("Cancelled.");
                return (true);
            }
            PL()->ErasePrompt();
        }
        return (false);
    }
}


//-----------------------------------------------------------------------------
// The EXPRT command.
//

namespace {
    // Return a token to use as the "cellname" in a filename.
    //
    void
    def_cellname(char *buf, const stringlist *list)
    {
        if (Cvt()->WriteFilename())
            strcpy(buf, Cvt()->WriteFilename());
        else {
            if (!strcmp(list->string, FIO_CUR_SYMTAB))
                strcpy(buf, "symtab_cells");
            else
                strcpy(buf, list->string);
            char *s = strrchr(buf,'.');
            if (s && s != buf)
                *s = '\0';
            if (FIO()->IsStripForExport())
                strcat(buf, ".phys");
        }
    }


    // Return false if calling pop-up should be closed.  If allcells
    // is true, all cells in current symbol table may be output,
    // otherwise only those in the hierarchy of the current cell.
    //
    bool
    out_cb(FileType type, bool allcells, void*)
    {
        if (type == Fnone) {
            // Clear pushed filename and AOI params, if any.
            if (Cvt()->WriteFilename()) {
                Cvt()->SetWriteFilename(0);
                Cvt()->SetupWriteCut(0);
            }
            return (true);
        }

        if (!DSP()->CurCellName()) {
            Log()->PopUpErr("No current cell!");
            return (false);
        }
        stringlist *namelist = new stringlist(lstring::copy(
            allcells ? FIO_CUR_SYMTAB : Tstring(DSP()->CurCellName())), 0);
        GCdestroy<stringlist> gc_namelist(namelist);

        CDcbin cbin(DSP()->CurCellName());
        if (!cbin.isSubcell())
            EditIf()->assignGlobalProperties(&cbin);

        FIOcvtPrms prms;
        prms.set_scale(FIO()->WriteScale());
        prms.set_alias_mask(CVAL_CASE | CVAL_PFSF | CVAL_FILE);
        if (!allcells && FIO()->OutFlatten()) {
            prms.set_flatten(true);
            if (FIO()->OutUseWindow()) {
                prms.set_use_window(true);
                prms.set_window(FIO()->OutWindow());
                prms.set_clip(FIO()->OutClip());
            }
        }

        char buf[256];
        if (type == Fnative) {
            bool flatten = FIO()->OutFlatten();
            char *s;
            if (!allcells && flatten) {
                strcpy(buf, namelist->string);
                strcat(buf, "_flat");
                s = XM()->SaveFileDlg("Path/name for new flat native cell? ",
                   buf);
            }
            else
                s = XM()->SaveFileDlg("Directory for saved files? ", 0);
            if (!s) {
                // An empty string means the current directory.
                PL()->ShowPrompt("Aborted.");
                return (true);
            }
            char *path = pathlist::expand_path(s, false, true);
            if (path && *path) {
                switch (filestat::get_file_type(path)) {
                case GFT_NONE:
#ifdef WIN32
                    if (mkdir(path) < 0) {
#else
                    if (mkdir(path, 0755) < 0) {
#endif
                        Log()->ErrorLogV(mh::Initialization,
                            "Failed to create directory %s.", path);
                        return (true);
                    }
                    break;
                    
                case GFT_FILE:
                case GFT_OTHER:
                    Log()->ErrorLogV(mh::Initialization,
                        "Native save failed, %s is not a directory.", path);
                    return (true);
                case GFT_DIR:
                    break;
                }
            }

            prms.set_destination(path, Fnative);
            FIO()->ConvertToNative(namelist, &prms);
            delete [] path;
            return (false);
        }
        else if (type == Fcif) {
            def_cellname(buf, namelist);
            strcat(buf, ".cif");
            char *s = XM()->SaveFileDlg("New CIF file name? ", buf);
            if (!s || !*s) {
                PL()->ShowPrompt("Aborted.");
                return (true);
            }
            char *cifname = pathlist::expand_path(s, false, true);
            if (check_fname(cifname, Fcif)) {
                delete [] cifname;
                return (true);
            }

            prms.set_destination(cifname, Fcif);
            FIO()->ConvertToCif(namelist, &prms);
            delete [] cifname;
            return (false);
        }
        if (type == Fgds) {
            def_cellname(buf, namelist);
            strcat(buf, ".gds");
            char *s = XM()->SaveFileDlg("New GDSII file name? ", buf);
            if (!s || !*s) {
                PL()->ShowPrompt("Aborted.");
                return (true);
            }
            char *gdsname = pathlist::expand_path(s, false, true);
            if (check_fname(gdsname, Fgds)) {
                delete [] gdsname;
                return (true);
            }

            prms.set_destination(gdsname, Fgds);
            FIO()->ConvertToGds(namelist, &prms);
            delete [] gdsname;
            return (false);
        }
        if (type == Fcgx) {
            def_cellname(buf, namelist);
            strcat(buf, ".cgx");
            char *s = XM()->SaveFileDlg("New CGX file name? ", buf);
            if (!s || !*s) {
                PL()->ShowPrompt("Aborted.");
                return (true);
            }
            char *cgxname = pathlist::expand_path(s, false, true);
            if (check_fname(cgxname, Fcgx)) {
                delete [] cgxname;
                return (true);
            }

            prms.set_destination(cgxname, Fcgx);
            FIO()->ConvertToCgx(namelist, &prms);
            delete [] cgxname;
            return (false);
        }
        if (type == Foas) {
            def_cellname(buf, namelist);
            strcat(buf, ".oas");
            char *s = XM()->SaveFileDlg("New OASIS file name? ", buf);
            if (!s || !*s) {
                PL()->ShowPrompt("Aborted.");
                return (true);
            }
            char *oasname = pathlist::expand_path(s, false, true);
            if (check_fname(oasname, Foas)) {
                delete [] oasname;
                return (true);
            }

            prms.set_destination(oasname, Foas);
            FIO()->ConvertToOas(namelist, &prms);
            delete [] oasname;
            return (false);
        }
        return (true);
    }
}


// Menu command to pop up a panel which initiates conversion to various
// output formats.
//
void
cvrt_menu::M_Export(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller)) {
        if (!DSP()->CurCellName()) {
            PL()->ShowPrompt("No current cell!");
            if (cmd)
                Menu()->Deselect(cmd->caller);
            return;
        }
        Cvt()->PopUpExport(cmd->caller, MODE_ON, out_cb, 0);
    }
    else
        Cvt()->PopUpExport(0, MODE_OFF, 0, 0);
}


//-----------------------------------------------------------------------------
// The IMPRT command.
//

namespace {
    bool
    in_cb(int type, void*)
    {
        if (type < 0)
            return (false);
        FIOreadPrms prms;
        prms.set_scale(FIO()->ReadScale());
        prms.set_alias_mask(
            CVAL_AUTO_NAME | CVAL_CASE | CVAL_FILE | CVAL_PFSF);
        prms.set_allow_layer_mapping(true);
        if (type == 0)
            XM()->EditCell(0, false, &prms);
        else if (type == 1)
            Cvt()->ReadIntoCurrent(0, 0, false, &prms);
        else
            Cvt()->ReadIntoCurrent(0, 0, true, &prms);
        return (false);
    }
}


void
cvrt_menu::M_Import(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        Cvt()->PopUpImport(cmd->caller, MODE_ON, in_cb, 0);
    else
        Cvt()->PopUpImport(0, MODE_OFF, 0, 0);
}


//-----------------------------------------------------------------------------
// The CONVT command.
//

namespace {
    char *
    newfilename(char *file, FileType ft)
    {
        file = lstring::strip_path(file);
        char *s = new char[strlen(file) + 10];
        strcpy(s, file);
        char *d = strrchr(s, '.');
        if (d && !strcmp(d, ".gz")) {
            *d = 0;
            d = strrchr(s, '.');
        }
        if (!d)
            d = s + strlen(s);
        if (ft == Fnone) {
            if (!strcmp(d, ".txt"))
                strcpy(d, "_new.txt");
            else if (d == s) {
                delete [] s;
                s = lstring::copy("newfile.txt");
            }
            else
                strcpy(d, ".txt");
        }
        else if (ft == Fgds) {
            if (!strcmp(d, ".gds"))
                strcpy(d, "_new.gds");
            else if (d == s) {
                delete [] s;
                s = lstring::copy("newfile.gds");
            }
            else
                strcpy(d, ".gds");
        }
        else if (ft == Fcgx) {
            if (!strcmp(d, ".cgx"))
                strcpy(d, "_new.cgx");
            else if (d == s) {
                delete [] s;
                s = lstring::copy("newfile.cgx");
            }
            else
                strcpy(d, ".cgx");
        }
        else if (ft == Foas) {
            if (!strcmp(d, ".oas"))
                strcpy(d, "_new.oas");
            else if (d == s) {
                delete [] s;
                s = lstring::copy("newfile.oas");
            }
            else
                strcpy(d, ".oas");
        }
        else if (ft == Fcif) {
            if (!strcmp(d, ".cif"))
                strcpy(d, "_new.cif");
            else if (d == s) {
                delete [] s;
                s = lstring::copy("newfile.cif");
            }
            else
                strcpy(d, ".cif");
        }
        else {
            delete [] s;
            return (0);
        }
        return (s);
    }


    bool
    cv_cb(int type, void*)
    {
        if (type < 0) {
            // Clear pushed filename and AOI params if any.
            if (Cvt()->ConvertFilename()) {
                Cvt()->SetConvertFilename(0);
                Cvt()->SetupConvertCut(0);
            }
            return (false);
        }
        int src_type = type >> 16;
        // One of cConvert::CVlayoutFile, etc. enum.

        FileType dst_ft;
        switch (type & 0xffff) {
        case cConvert::cvGds:
            dst_ft = Fgds;
            break;
        case cConvert::cvOas:
            dst_ft = Foas;
            break;
        case cConvert::cvCif:
            dst_ft = Fcif;
            break;
        case cConvert::cvCgx:
            dst_ft = Fcgx;
            break;
        case cConvert::cvXic:
            dst_ft = Fnative;
            break;
        case cConvert::cvTxt:
            dst_ft = Fnone;
            break;
        default:
            return (false);
        }

        const char *in_prompt;
        if (src_type == cConvert::CVlayoutFile)
            in_prompt = "Input file? ";
        else if (src_type == cConvert::CVgdsText)
            in_prompt = "GDS text input file? ";
        else if (src_type == cConvert::CVchdName)
            in_prompt = "CHD name and optional cell name? ";
        else if (src_type == cConvert::CVchdFile) {
            in_prompt = "CHD file and optional cell name? ";
        }
        else if (src_type == cConvert::CVnativeDir)
            in_prompt = "Directory containing cell files? ";
        else
            return (true);

        char *infile = 0;
        char *cellname = 0;
        {
            const char *s = Cvt()->ConvertFilename();
            if (!s) {
                if (src_type == cConvert::CVchdName)
                    s = PL()->EditPrompt(in_prompt, 0);
                else
                    s = XM()->OpenFileDlg(in_prompt, 0);
                if (!s || !*s) {
                    PL()->ShowPrompt("Aborted.");
                    return (true);
                }
            }
            infile = lstring::getqtok(&s);
            cellname = lstring::getqtok(&s);
        }

        FileType src_ft = Fnone;
        cCHD *chd = CDchd()->chdRecall(infile, false);
        if (chd) {
            // Source name was a CHD in memory, which is allowed.
            src_ft = chd->filetype();
        }
        else {
            char *ftmp = infile;
            infile = pathlist::expand_path(ftmp, false, true);
            delete [] ftmp;
            if (src_type == cConvert::CVlayoutFile) {
                char *realname;
                FILE *fp = FIO()->POpen(infile, "rb", &realname);
                if (!fp) {
                    PL()->ShowPrompt("Can not open source file.");
                    delete [] infile;
                    delete [] cellname;
                    return (true);
                }
                delete [] infile;
                infile = realname;
                src_ft = FIO()->GetFileType(fp);
                fclose(fp);
                if (!FIO()->IsSupportedArchiveFormat(src_ft)) {
                    PL()->ShowPrompt("Source is not an archive file.");
                    delete [] infile;
                    delete [] cellname;
                    return (true);
                }
            }
            else if (src_type == cConvert::CVchdFile) {
                FILE *fp = FIO()->POpen(infile, "rb");
                if (!fp) {
                    PL()->ShowPrompt("Can not open source file.");
                    delete [] infile;
                    delete [] cellname;
                    return (true);
                }
                fclose(fp);
                sCHDin chd_in;
                if (!chd_in.check(infile)) {
                    PL()->ShowPrompt("Not a supported file format.");
                    delete [] infile;
                    delete [] cellname;
                    return (true);
                }
                dspPkgIf()->SetWorking(true);
                chd = chd_in.read(infile, sCHDin::get_default_cgd_type());
                dspPkgIf()->SetWorking(false);
                if (!chd) {
                    PL()->ShowPrompt("Read error occurred.");
                    delete [] infile;
                    delete [] cellname;
                    return (false);
                }
                char *dbname = CDchd()->newChdName();
                CDchd()->chdStore(dbname, chd);
                delete [] infile;
                infile = dbname;
                src_ft = chd->filetype();
                PL()->ShowPromptV("New CHD created, name:  %s", dbname);
            }
            else if (src_type == cConvert::CVnativeDir) {
                if (!filestat::is_directory(infile)) {
                    PL()->ShowPrompt("Can't open directory.");
                    delete [] infile;
                    delete [] cellname;
                    return (true);
                }
                src_ft = Fnative;
            }
        }

        const char *out_prompt;
        if (dst_ft == Fnone)
            out_prompt = "Text output file? ";
        else if (dst_ft == Fnative)
            out_prompt = "Directory for native files? ";
        else if (dst_ft == Fgds)
            out_prompt = "GDSII output file? ";
        else if (dst_ft == Fcgx)
            out_prompt = "CGX output file? ";
        else if (dst_ft == Foas)
            out_prompt = "OASIS output file? ";
        else if (dst_ft == Fcif)
            out_prompt = "CIF output file? ";
        else {
            delete [] infile;
            delete [] cellname;
            return (true);
        }

        if (src_ft == Fcif && dst_ft == Fnone) {
            PL()->ShowPrompt(
                "CIF source is already in readable text format.");
            delete [] infile;
            delete [] cellname;
            return (true);
        }

        char *outfile = 0;
        {
            char *fn = infile;
            if (Cvt()->ConvertFilename()) {
                //-------------------------------
                // HACK: This is the Cut command.
                //-------------------------------
                fn = new char[strlen(Cvt()->ConvertFilename()) + 5];
                char *tt = lstring::stpcpy(fn, Cvt()->ConvertFilename());
                strcpy(tt, "-cut");
            }

            char *prm_file = newfilename(fn, dst_ft);
            const char *s = XM()->SaveFileDlg(out_prompt, prm_file);
            delete [] prm_file;
            if (fn != infile)
                delete [] fn;
            outfile = lstring::copy(s);
        }
        if (!outfile) {
            PL()->ShowPrompt("Aborted.");
            delete [] infile;
            delete [] cellname;
            return (true);
        }
        {
            char *tmp = outfile;
            outfile = pathlist::expand_path(tmp, false, true);
            delete [] tmp;
            if (filestat::is_same_file(infile, outfile)) {
                PL()->ShowPrompt("Source and destination files are the same!");
                delete [] infile;
                delete [] cellname;
                delete [] outfile;
                return (true);
            }
            if (check_fname(outfile, dst_ft)) {
                delete [] infile;
                delete [] cellname;
                delete [] outfile;
                return (true);
            }
        }

        char *text_args = 0;

        if (dst_ft == Fnone &&
                (src_ft == Fgds || src_ft == Fcgx || src_ft == Foas)) {

            // Save the last options string for the file type.
            static char *gds_text_args;
            static char *cgx_text_args;
            static char *oas_text_args;
            if (src_ft == Fgds)
                text_args = gds_text_args;
            else if (src_ft == Fcgx)
                text_args = cgx_text_args;
            else if (src_ft == Foas)
                text_args = oas_text_args;

            char *in = PL()->EditPrompt(
        "Options ([start_ofs[-end_ofs]] [-r num_records] [-c num_cells])? ",
                text_args);
            if (!in) {
                delete [] infile;
                delete [] cellname;
                delete [] outfile;
                PL()->ErasePrompt();
                return (true);
            }
            if (!text_args)
                text_args = lstring::copy(in);
            else if (strcmp(text_args, in)) {
                delete [] text_args;
                text_args = lstring::copy(in);
            }
            if (src_ft == Fgds)
                gds_text_args = text_args;
            else if (src_ft == Fcgx)
                cgx_text_args = text_args;
            else if (src_ft == Foas)
                oas_text_args = text_args;
        }

        FIOcvtPrms prms;
        if (src_ft != Fnative) {
            // These seem to work when creating a CGD, might be useful
            // in some case.

            prms.set_scale(FIO()->TransScale());
            if (FIO()->CvtUseWindow()) {
                prms.set_use_window(true);
                prms.set_window(FIO()->CvtWindow());
                prms.set_clip(FIO()->CvtClip());
            }
            prms.set_flatten(FIO()->CvtFlatten());
            prms.set_ecf_level(FIO()->CvtECFlevel());
        }
        prms.set_alias_mask(CVAL_CASE | CVAL_PFSF | CVAL_FILE);
        prms.set_allow_layer_mapping(true);
        prms.set_destination(outfile, dst_ft, false);

        // The cellname applies only when infile is a CHD name.
        if (src_ft == Fnative)
            FIO()->ConvertFromNative(infile, &prms);
        else if (src_ft == Fgds) {
            if (dst_ft == Fnone)
                FIO()->ConvertToText(Fgds, infile, outfile, text_args);
            else
                FIO()->ConvertFromGds(infile, &prms, cellname);
        }
        else if (src_ft == Foas) {
            if (dst_ft == Fnone)
                FIO()->ConvertToText(Foas, infile, outfile, text_args);
            else
                FIO()->ConvertFromOas(infile, &prms, cellname);
        }
        else if (src_ft == Fcgx) {
            if (dst_ft == Fnone)
                FIO()->ConvertToText(Fcgx, infile, outfile, text_args);
            else
                FIO()->ConvertFromCgx(infile, &prms, cellname);
        }
        else if (src_ft == Fcif)
            FIO()->ConvertFromCif(infile, &prms, cellname);
        else if (src_ft == Fnone)
            FIO()->ConvertFromGdsText(infile, outfile);

        delete [] infile;
        delete [] cellname;
        delete [] outfile;
        return (false);
    }
}


void
cvrt_menu::M_Convert(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        Cvt()->PopUpConvert(cmd->caller, MODE_ON, 0, cv_cb, 0);
    else
        Cvt()->PopUpConvert(0, MODE_OFF, 0, 0, 0);
}


//-----------------------------------------------------------------------------
// The ASSEM command.
//
void
cvrt_menu::M_Assemble(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        Cvt()->PopUpAssemble(cmd->caller, MODE_ON);
    else
        Cvt()->PopUpAssemble(0, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The DIFF command.
//
void
cvrt_menu::M_Diff(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        Cvt()->PopUpCompare(cmd->caller, MODE_ON);
    else
        Cvt()->PopUpCompare(0, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The CUT command.
//
void
cvrt_menu::M_Cut(CmdDesc *cmd)
{
    Cvt()->CutWindowExec(cmd);
}


//-----------------------------------------------------------------------------
// The TXTED command.
//
// Open a text editor with the current cell, if it is an ascii text
// file, or a temp file name.
//
void
cvrt_menu::M_TextEdit(CmdDesc*)
{
    char *fn = 0;
    if (DSP()->CurCellName()) {
        CDcbin cbin(DSP()->CurCellName());
        if (!cbin.isLibrary() || !cbin.isDevice()) {
            FileType ft = cbin.fileType();
            if (ft == Fnative) {
                if (cbin.fileName()) {
                    fn = new char[strlen(cbin.fileName()) +
                        strlen(Tstring(cbin.cellname())) + 2];
                    sprintf(fn, "%s/%s", cbin.fileName(),
                        Tstring(cbin.cellname()));
                }
            }
            else if (ft == Fcif) {
                if (cbin.fileName())
                    fn = lstring::copy(cbin.fileName());
            }
        }
    }
    if (!fn)
        PL()->ShowPrompt("No ascii file available for current cell.");
    DSPmainWbag(PopUpTextEditor(fn, 0, 0, false))
}

