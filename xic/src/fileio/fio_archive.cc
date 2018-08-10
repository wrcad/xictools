
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

#include "fio.h"
#include "fio_chd.h"
#include "fio_cvt_base.h"
#include "fio_library.h"
#include "cd_propnum.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "miscutil/filestat.h"
#include "miscutil/pathlist.h"

#include <ctype.h>
#include <errno.h>


//
// High-level functions to read/write/translate archives and native
// cell files.
//

namespace {
    void cv_failed(const char*, const char*);
    bool init_translation(const char*, char**);
    void setup_layers(bool);
    bool parse_offs(const char**, int64_t*);
    void parse_textargs(const char*, int64_t*, int64_t*, int*, int*);

    const char *list_name(const stringlist *list, bool n = false)
    {
        if (list->next && !n)
            return ("cell list");
        if (!strcmp(list->string, FIO_CUR_SYMTAB))
            return ("symbol table cells");
        return (list->string);
    }

    const char *msg_failed = "Translation of %s failed.";
    const char *msg_ok = "Translation of %s succeeded.";
    const char *msg_ok_s = "Translation of %s succeeded (scale changed).";
}


// Write the cell hierarchy for each cell name in the list to native
// cell files.
//
bool
cFIO::ConvertToNative(const stringlist *namelist, const FIOcvtPrms *prms)
{
    Errs()->init_error();
    bool success = false;

    if (!namelist || !namelist->string || !prms)
        return (false);
    FILE *lfp = ifInitCvLog(TOXIC_FN);
    ifInfoMessage(IFMSG_INFO, "Working...");

    OItype oiret = OIerror;
    for (const stringlist *sl = namelist; sl; sl = sl->next) {
        setup_layers(true);
        oiret = ToNative(sl->string, prms);
        setup_layers(false);
        if (OIfailed(oiret)) {
            ifPrintCvLog(IFLOG_FATAL, Errs()->get_error());
            ifPrintCvLog(IFLOG_INFO_SHOW,
                prms->flatten() ?
                "Writing %s as flat native cell file failed." :
                "Writing %s as native cell files failed.",
                    list_name(namelist, true));
        }
        else {
            ifPrintCvLog(IFLOG_INFO_SHOW, (WriteScale() != 1.0 ?
                "Done (scaled)." : "Done."));
            success = true;
        }
        if (prms->flatten())
            break;
    }
    ifShowCvLog(lfp, oiret, TOXIC_FN);
    return (success);
}


// Write the cell hierarchy for each cell name in the list to a CIF
// file.
//
bool
cFIO::ConvertToCif(const stringlist *namelist, const FIOcvtPrms *prms)
{
    Errs()->init_error();
    bool success = false;

    if (!namelist || !namelist->string || !prms)
        return (false);
    const char *outfilename = prms->destination();
    char *bakname;
    if (init_translation(outfilename, &bakname)) {
        FILE *lfp = ifInitCvLog(TOCIF_FN);

        const char *fmt = "Using cell name convention: %s.";
        switch (FIO()->CifStyle().cname_type()) {
        case EXTcnameDef:
            ifPrintCvLog(IFLOG_INFO, fmt, "IGS  \"9 cellname;\"");
            break;
        case EXTcnameNCA:
            ifPrintCvLog(IFLOG_INFO, fmt, "Stanford/NCA  \"(cellname);\"");
            break;
        case EXTcnameICARUS:
            ifPrintCvLog(IFLOG_INFO, fmt, "Icarus  \"(9 cellname);\"");
            break;
        case EXTcnameSIF:
            ifPrintCvLog(IFLOG_INFO, fmt, "SIF  \"(Name: cellname);\"");
            break;
        case EXTcnameNone:
            ifPrintCvLog(IFLOG_INFO, fmt, "no cell names");
            break;
        }
        ifInfoMessage(IFMSG_INFO, "Working...");

        setup_layers(true);
        OItype oiret = ToCIF(namelist, prms);
        setup_layers(false);
        if (OIfailed(oiret)) {
            ifPrintCvLog(IFLOG_FATAL, Errs()->get_error());
            cv_failed(outfilename, bakname);
            ifPrintCvLog(IFLOG_INFO_SHOW, msg_failed, list_name(namelist));
        }
        else {
            ifPrintCvLog(IFLOG_INFO_SHOW,
                WriteScale() != 1.0 ? msg_ok_s : msg_ok, list_name(namelist));
            success = true;
        }
        ifShowCvLog(lfp, oiret, TOCIF_FN);
    }
    else
        ifInfoMessage(IFMSG_INFO, "Translation failed: %s",
            Errs()->get_error());
    delete [] bakname;
    return (success);
}


// Write the cell hierarchy for each cell name in the list to a GDSII
// file.
//
bool
cFIO::ConvertToGds(const stringlist *namelist, const FIOcvtPrms *prms)
{
    Errs()->init_error();
    bool success = false;

    if (!namelist || !namelist->string || !prms)
        return (false);
    const char *outfilename = prms->destination();
    char *bakname;
    if (init_translation(outfilename, &bakname)) {
        FILE *lfp = ifInitCvLog(TOGDS_FN);
        ifInfoMessage(IFMSG_INFO, "Working...");

        setup_layers(true);
        OItype oiret = ToGDSII(namelist, prms);
        setup_layers(false);
        if (OIfailed(oiret)) {
            ifPrintCvLog(IFLOG_FATAL, Errs()->get_error());
            cv_failed(outfilename, bakname);
            ifPrintCvLog(IFLOG_INFO_SHOW, msg_failed, list_name(namelist));
        }
        else {
            ifPrintCvLog(IFLOG_INFO_SHOW,
                WriteScale() != 1.0 ? msg_ok_s : msg_ok, list_name(namelist));
            success = true;
        }
        ifShowCvLog(lfp, oiret, TOGDS_FN);
    }
    else
        ifInfoMessage(IFMSG_INFO, "Translation failed: %s",
            Errs()->get_error());
    delete [] bakname;
    return (success);
}


// Write the cell hierarchy for each cell name in the list to a CGX
// file.
//
bool
cFIO::ConvertToCgx(const stringlist *namelist, const FIOcvtPrms *prms)
{
    Errs()->init_error();
    bool success = false;

    if (!namelist || !namelist->string || !prms)
        return (false);
    const char *outfilename = prms->destination();
    char *bakname;
    if (init_translation(outfilename, &bakname)) {
        FILE *lfp = ifInitCvLog(TOCGX_FN);
        ifInfoMessage(IFMSG_INFO, "Working...");

        setup_layers(true);
        OItype oiret = ToCGX(namelist, prms);
        setup_layers(false);
        if (OIfailed(oiret)) {
            ifPrintCvLog(IFLOG_FATAL, Errs()->get_error());
            cv_failed(outfilename, bakname);
            ifPrintCvLog(IFLOG_INFO_SHOW, msg_failed, list_name(namelist));
        }
        else {
            ifPrintCvLog(IFLOG_INFO_SHOW,
                WriteScale() != 1.0 ? msg_ok_s : msg_ok, list_name(namelist));
            success = true;
        }
        ifShowCvLog(lfp, oiret, TOCGX_FN);
    }
    else
        ifInfoMessage(IFMSG_INFO, "Translation failed: %s",
            Errs()->get_error());
    delete [] bakname;
    return (success);
}


// Write the cell hierarchy for each cell name in the list to an OASIS
// file.
//
bool
cFIO::ConvertToOas(const stringlist *namelist, const FIOcvtPrms *prms)
{
    Errs()->init_error();
    bool success = false;

    if (!namelist || !namelist->string || !prms)
        return (false);
    const char *outfilename = prms->destination();
    char *bakname;
    if (init_translation(outfilename, &bakname)) {
        FILE *lfp = ifInitCvLog(TOOAS_FN);
        ifInfoMessage(IFMSG_INFO, "Working...");

        setup_layers(true);
        OItype oiret = ToOASIS(namelist, prms);
        setup_layers(false);
        if (OIfailed(oiret)) {
            ifPrintCvLog(IFLOG_FATAL, Errs()->get_error());
            cv_failed(outfilename, bakname);
            ifPrintCvLog(IFLOG_INFO_SHOW, msg_failed, list_name(namelist));
        }
        else {
            ifPrintCvLog(IFLOG_INFO_SHOW,
                WriteScale() != 1.0 ? msg_ok_s : msg_ok, list_name(namelist));
            success = true;
        }
        ifShowCvLog(lfp, oiret, TOOAS_FN);
    }
    else
        ifInfoMessage(IFMSG_INFO, "Translation failed: %s",
            Errs()->get_error());
    delete [] bakname;
    return (success);
}


// Convert GDSII/CGX/OASIS file to text file.
//
bool
cFIO::ConvertToText(FileType ft, const char *infilename,
    const char *outfilename, const char *text_args)
{
    Errs()->init_error();
    bool success = false;

    if (ft == Fgds || ft == Fcgx || ft == Foas) {
        // infile   path to archive file
        // outfile  path to text file
        //
        if (outfilename && !filestat::create_bak(outfilename)) {
            Errs()->add_error(filestat::error_msg());
            Errs()->add_error("Error creating backup file.");
            ifInfoMessage(IFMSG_INFO, Errs()->get_error());
        }
        else {
            int64_t start, end;
            int numrecs, numcells;
            parse_textargs(text_args, &start, &end, &numrecs, &numcells);
            ifInitCvLog(0);
            if (!GetCHDinfo(infilename, ft, outfilename, start, end,
                    numrecs, numcells))
                ifInfoMessage(IFMSG_INFO, "Translation failed: %s",
                    Errs()->get_error());
            else {
                ifInfoMessage(IFMSG_INFO, "Translation succeeded.");
                success = true;
            }
            ifShowCvLog(0, OIok, 0);
        }
    }
    return (success);
}


// Translate Native cell files in directory to another format.
//
bool
cFIO::ConvertFromNative(const char *dirname, const FIOcvtPrms *prms)
{
    Errs()->init_error();
    bool success = false;

    if (!dirname || !prms)
        return (false);
    const char *outfilename = prms->destination();

    FILE *lfp = ifInitCvLog(FRXIC_FN);
    ifInfoMessage(IFMSG_INFO, "Working...");
    char *bakname = 0;
    OItype oiret = OIerror;
    if (filestat::create_bak(outfilename, &bakname)) {
        oiret = TranslateDir(dirname, prms);
        if (OIfailed(oiret)) {
            ifPrintCvLog(IFLOG_FATAL, Errs()->get_error());
            if (IsSupportedArchiveFormat(prms->filetype()))
                cv_failed(outfilename, bakname);
            ifPrintCvLog(IFLOG_INFO_SHOW, msg_failed, dirname);
        }
        else {
            ifPrintCvLog(IFLOG_INFO_SHOW,
                (TransScale() != 1.0 ? msg_ok_s : msg_ok), dirname);
            success = true;
        }
    }
    else
        ifPrintCvLog(IFLOG_FATAL, filestat::error_msg());
    ifShowCvLog(lfp, oiret, FRXIC_FN);
    delete [] bakname;
    return (success);
}


// Translate CIF file to another format.
//
bool
cFIO::ConvertFromCif(const char *infilename, const FIOcvtPrms *prms,
    const char *chdcell)
{
    Errs()->init_error();
    bool success = false;

    if (!infilename || !prms)
        return (false);
    const char *outfilename = prms->destination();

    FILE *lfp = ifInitCvLog(FRCIF_FN);
    ifInfoMessage(IFMSG_INFO, "Working...");
    char *bakname = 0;
    OItype oiret = OIerror;
    if (prms->filetype() == Fnative || prms->to_cgd() ||
            filestat::create_bak(outfilename, &bakname)) {
        oiret = FromCIF(infilename, prms, chdcell);
        if (OIfailed(oiret)) {
            ifPrintCvLog(IFLOG_FATAL, Errs()->get_error());
            if (IsSupportedArchiveFormat(prms->filetype()))
                cv_failed(outfilename, bakname);
            ifPrintCvLog(IFLOG_INFO_SHOW, msg_failed, infilename);
        }
        else {
            ifPrintCvLog(IFLOG_INFO_SHOW,
                (TransScale() != 1.0 ? msg_ok_s : msg_ok), infilename);
            success = true;
        }
    }
    else
        ifPrintCvLog(IFLOG_FATAL, filestat::error_msg());
    ifShowCvLog(lfp, oiret, FRCIF_FN);
    delete [] bakname;
    return (success);
}


// Translate GDSII file to another format.
//
bool
cFIO::ConvertFromGds(const char *infilename, const FIOcvtPrms *prms,
    const char *chdcell)
{
    Errs()->init_error();
    bool success = false;

    if (!infilename || !prms)
        return (false);
    const char *outfilename = prms->destination();

    FILE *lfp = ifInitCvLog(FRGDS_FN);
    ifInfoMessage(IFMSG_INFO, "Working...");
    char *bakname = 0;
    OItype oiret = OIerror;
    if (prms->filetype() == Fnative || prms->to_cgd() ||
            filestat::create_bak(outfilename, &bakname)) {
        oiret = FromGDSII(infilename, prms, chdcell);
        if (OIfailed(oiret)) {
            ifPrintCvLog(IFLOG_FATAL, Errs()->get_error());
            if (IsSupportedArchiveFormat(prms->filetype()))
                cv_failed(outfilename, bakname);
            ifPrintCvLog(IFLOG_INFO_SHOW, msg_failed, infilename);
        }
        else {
            ifPrintCvLog(IFLOG_INFO_SHOW,
                (TransScale() != 1.0 ? msg_ok_s : msg_ok), infilename);
            success = true;
        }
    }
    else
        ifPrintCvLog(IFLOG_FATAL, filestat::error_msg());
    ifShowCvLog(lfp, oiret, FRGDS_FN);
    delete [] bakname;
    return (success);
}


// Translate CGX file to another format.
//
bool
cFIO::ConvertFromCgx(const char *infilename, const FIOcvtPrms *prms,
    const char *chdcell)
{
    Errs()->init_error();
    bool success = false;

    if (!infilename || !prms)
        return (false);
    const char *outfilename = prms->destination();

    FILE *lfp = ifInitCvLog(FRCGX_FN);
    ifInfoMessage(IFMSG_INFO, "Working...");
    char *bakname = 0;
    OItype oiret = OIerror;
    if (prms->filetype() == Fnative || prms->to_cgd() ||
            filestat::create_bak(outfilename, &bakname)) {
        oiret = FromCGX(infilename, prms, chdcell);
        if (OIfailed(oiret)) {
            ifPrintCvLog(IFLOG_FATAL, Errs()->get_error());
            if (IsSupportedArchiveFormat(prms->filetype()))
                cv_failed(outfilename, bakname);
            ifPrintCvLog(IFLOG_INFO_SHOW, msg_failed, infilename);
        }
        else {
            ifPrintCvLog(IFLOG_INFO_SHOW,
                (TransScale() != 1.0 ? msg_ok_s : msg_ok), infilename);
            success = true;
        }
    }
    else
        ifPrintCvLog(IFLOG_FATAL, filestat::error_msg());
    ifShowCvLog(lfp, oiret, FRCGX_FN);
    delete [] bakname;
    return (success);
}


// Translate OASIS file to another format.
//
bool
cFIO::ConvertFromOas(const char *infilename, const FIOcvtPrms *prms,
    const char *chdcell)
{
    Errs()->init_error();
    bool success = false;

    if (!infilename || !prms)
        return (false);
    const char *outfilename = prms->destination();

    FILE *lfp = ifInitCvLog(FROAS_FN);
    ifInfoMessage(IFMSG_INFO, "Working...");
    char *bakname = 0;
    OItype oiret = OIerror;
    if (prms->filetype() == Fnative || prms->to_cgd() ||
            filestat::create_bak(outfilename, &bakname)) {
        oiret = FromOASIS(infilename, prms, chdcell);
        if (OIfailed(oiret)) {
            ifPrintCvLog(IFLOG_FATAL, Errs()->get_error());
            if (IsSupportedArchiveFormat(prms->filetype()))
                cv_failed(outfilename, bakname);
            ifPrintCvLog(IFLOG_INFO_SHOW, msg_failed, infilename);
        }
        else {
            ifPrintCvLog(IFLOG_INFO_SHOW,
                (TransScale() != 1.0 ? msg_ok_s : msg_ok), infilename);
            success = true;
        }
    }
    else
        ifPrintCvLog(IFLOG_FATAL, filestat::error_msg());
    ifShowCvLog(lfp, oiret, FROAS_FN);
    delete [] bakname;
    return (success);
}


// Convert gds-text file to GDSII file.
//
bool
cFIO::ConvertFromGdsText(const char *infilename, const char *outfilename)
{
    Errs()->init_error();
    bool success = false;

    if (!outfilename) {
        Errs()->add_error("Null GDSII output file name.");
        ifInfoMessage(IFMSG_INFO, Errs()->get_error());
        return (false);
    }
    if (!infilename) {
        Errs()->add_error("Null gds-text file name.");
        ifInfoMessage(IFMSG_INFO, Errs()->get_error());
        return (false);
    }
    if (!filestat::create_bak(outfilename)) {
        Errs()->add_error(filestat::error_msg());
        Errs()->add_error("Error creating backup file.");
        ifInfoMessage(IFMSG_INFO, Errs()->get_error());
    }
    else {
        ifInitCvLog(0);
        if (!GdsFromText(infilename, outfilename))
            ifInfoMessage(IFMSG_INFO, "Translation failed: %s",
                Errs()->get_error());
        else {
            ifInfoMessage(IFMSG_INFO, "Translation succeeded.");
            success = true;
        }
        ifShowCvLog(0, OIok, 0);
    }
    return (success);
}


namespace {
    // Handle failure:  delete output, or move to ".BAD", move bakfile to
    // outfile.
    //
    void
    cv_failed(const char *outfile, const char *bakfile)
    {
        if (!access(outfile, F_OK)) {
            if (FIO()->IsKeepBadArchive()) {
                // Keep the file for diagnostics, but give it a ".BAD" suffix
                char *tfn = new char[strlen(outfile) + 5];
                sprintf(tfn, "%s.BAD", outfile);
                filestat::move_file_local(tfn, outfile);
                FIO()->ifPrintCvLog(IFLOG_INFO,
                    "Moving %s to %s", outfile, tfn);
                delete [] tfn;
            }
            else {
                FIO()->ifPrintCvLog(IFLOG_INFO,
                    "Deleting output file %s due to errors.\n", outfile);
                unlink(outfile);
            }
        }
        if (bakfile)
            filestat::move_file_local(outfile, bakfile);
    }


    bool
    init_translation(const char *fname, char **bakname)
    {
        if (!fname) {
            Errs()->add_error("null filename");
            return (false);
        }
        if (!filestat::create_bak(fname, bakname)) {
            Errs()->add_error(filestat::error_msg());
            return (false);
        }
        return (true);
    }


    // Set skip flags in invisible layers, if this mode is on.
    //
    void
    setup_layers(bool setup)
    {
        CDl *ld;
        CDlgen lgen(Physical);
        while ((ld = lgen.next()) != 0) {
            if (setup && ld->isInvisible() && FIO()->IsSkipInvisiblePhys())
                ld->setTmpSkip(true);
            else
                ld->setTmpSkip(false);
        }
        lgen = CDlgen(Electrical);
        while ((ld = lgen.next()) != 0) {
            if (setup && ld->isInvisible() && FIO()->IsSkipInvisibleElec())
                ld->setTmpSkip(true);
            else
                ld->setTmpSkip(false);
        }
    }


    // Parse a number in decimal or "0x" hex format into num, and advance s.
    //
    bool
    parse_offs(const char **s, int64_t *num)
    {
        const char *t = *s;
        if (t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) {
            int n = 0;
            long long ll;
#ifdef WIN32
            if (sscanf(t+2, "%I64x%n", &ll, &n) == 1) {
#else
            if (sscanf(t+2, "%llx%n", &ll, &n) == 1) {
#endif
                *num = ll;
                (*s) += n+2;
                return (true);
            }
        }
        else if (isdigit(*t)) {
            int n = 0;
            long long ll;
#ifdef WIN32
            if (sscanf(t, "%I64u%n", &ll, &n) == 1) {
#else
            if (sscanf(t, "%llu%n", &ll, &n) == 1) {
#endif
                *num = ll;
                (*s) += n;
                return (true);
            }
        }
        return (false);
    }


    // Parse "[start[-end]]  [-c cells] [-r recs]" for archive-to-text
    // functions.
    //
    void
    parse_textargs(const char *str, int64_t *start, int64_t *end,
        int *numrecs, int *numcells)
    {
        *start = 0;
        *end = 0;
        *numrecs = -1;
        *numcells = -1;

        bool found_start = false;
        char *tok;
        const char *s = str;
        while ((tok = lstring::gettok(&s)) != 0) {
            if (!strcmp(tok, "-c")) {
                delete [] tok;
                tok = lstring::gettok(&s);
                if (tok) {
                    int d;
                    if (sscanf(tok, "%d", &d) == 1)
                        *numcells = d;
                }
            }
            else if (!strcmp(tok, "-r")) {
                delete [] tok;
                tok = lstring::gettok(&s);
                if (tok) {
                    int d;
                    if (sscanf(tok, "%d", &d) == 1)
                        *numrecs = d;
                }
            }
            else if (isdigit(*tok)) {
                const char *t = tok;
                if (found_start)
                    parse_offs(&t, end);
                else if (parse_offs(&t, start)) {
                    if (*t == '-') {
                        t++;
                        parse_offs(&t, end);
                    }
                    found_start = true;
                }
            }
            delete [] tok;
        }
    }
}


namespace {
    const char *oi_msg1 = "File %s was not found.";
    const char *oi_msg2 = "Could not find %s in %s.";
}


typedef void(*AMBIGUITY_CALLBACK)(const char*, void*);

// Open a file 'fullname' and add the contents to the database.  The
// cell desc is returned in cbret.  callback is a function to call
// to resolve ambiguity if an archive file with more than one top
// level cell is opened, or fullname is a library and cellname is nil.
// If cellname is given, that cell will be opened when resolving
// ambiguity (does not have to be top-level in that case).  If
// fullname is a library, then cellname can be a reference or cell
// name for that library.  If context is given along with cellname,
// then fullname is ignored.
//
//  Return values:
//    OIerror       unspecified error
//    OIok          file was read and new cells opened ok
//    OIold         cell already in memory
//    OInew         new cell created in memory (native file not found)
//    OIambiguous   multiple top-cells or library entries
//    OIaborted     user aborted
//
OItype
cFIO::OpenImport(const char *fullname, const FIOreadPrms *prms,
    const char *cellname, cCHD *chd, CDcbin *cbret, char **pathname,
    void (*callback)(const char*, OIcbData*), OIcbData *cbdata)
{
    if (!prms)
        return (OIerror);

    // This takes care of enabling and cleaning up after the
    // MergeControl pop-up.
    sMCenable mc_enable;

    if (cbret)
        cbret->reset();
    if (cellname && !*cellname)
        cellname = 0;

    // Strip off the path prefix, if any.  POpen() will look only in
    // the directory given, otherwise will use search path.
    //
    if (chd)
        fullname = chd->filename();
    else if (!fullname || !*fullname) {
        Errs()->add_error("OpenImport: null file/cell name encountered.");
        return (OIerror);
    }
    const char *name = lstring::strip_path(fullname);
    if (pathname)
        *pathname = 0;

    if (!cellname && name == fullname) {
        // Opening a bare name will resolve to an existing cell.
        CDcbin cbin;
        if (CDcdb()->findSymbol(name, &cbin)) {
            // Already in memory, return
            if (cbin.elec() && cbin.elec()->isBBsubng()) {
                if (!cbin.elec()->fixBBs())
                    return (OIerror);
            }
            if (cbin.phys() && cbin.phys()->isBBsubng()) {
                if (!cbin.phys()->fixBBs())
                    return (OIerror);
            }
            if (cbret)
                *cbret = cbin;
            if (pathname)
                *pathname = lstring::copy("(in memory)");
            return (OIold);
        }
    }

    bool free_chd = false;
    FileType type = Fnone;
    char *fullpath = 0;
    sLibRef *libref = 0;
    sLib *libptr = 0;
    // name:      name of file
    // fullname:  path to file given by user, can be relative to cwd
    // fullpath:  full path to file

    if (chd) {
        // context given
        type = chd->filetype();
        fullpath = lstring::copy(chd->filename());
    }
    else {
        // Device library cell?
        if (!cellname) {
            if (cbret) {
                if (OpenLibCell(0, name, LIBdevice | LIBuser | LIBnativeOnly,
                        cbret) == OIok) {
                    // OpenLibCell calls OpenNative directly for
                    // speed, so does not call fixBBs, have to do this
                    // here.

                    if (cbret->phys())
                         cbret->phys()->fixBBs();
                    if (cbret->elec())
                         cbret->elec()->fixBBs();
                    if (cbret->phys() || cbret->elec())
                        return (OIok);
                }
            }
            else {
                CDcbin cbin;
                if (OpenLibCell(0, name, LIBdevice | LIBuser | LIBnativeOnly,
                        &cbin) == OIok) {
                    // OpenLibCell calls OpenNative directly for
                    // speed, so does not call fixBBs, have to do this
                    // here.

                    if (cbin.phys())
                        cbin.phys()->fixBBs();
                    if (cbin.elec())
                        cbin.elec()->fixBBs();
                    if (cbin.phys() || cbin.elec())
                        return (OIok);
                }
            }
        }
        else if (name == fullname) {
            if (ifOpenOA(name, cellname, cbret))
                return (OIok);
        }

        FILE *fp;
        OItype oiret = open_symbol_file(fullname, cellname,
            callback, cbdata, &fp, &fullpath, &libref, &libptr);
        if (OIfailed(oiret) || oiret == OIambiguous)
            return (oiret);

        if (!fp) {
            if (errno && errno != ENOENT) {
                // Permission error, probably.
                Errs()->sys_error(fullname);
                return (OIerror);
            }
            if (strcmp(name, fullname)) {
                // A path was given, and nothing was found, flag an error.
                Errs()->add_error(oi_msg1, fullname);
                return (OIerror);
            }
            FileType ft = TypeExt(name);
            if (ft == Fnone || ft == Fnative) {

                if (!IsNoStrictCellnames()) {
                    // Check for viable cell name.
                    for (const char *t = name; *t; t++) {
                        if (*t > ' ') 
                            continue;
                        Errs()->add_error(
                            "cell name \"%s\" contains unsupported character.",
                            name);
                        return (OIerror);
                    }
                }

                // Open a new cell since the name is reasonable. 
                // Create a cell only for the current display mode.
                bool oldsym = true;
                CDcbin cbin;
                if (!CDcdb()->findSymbol(name, &cbin)) {
                    CDs *sd = CDcdb()->insertCell(name, ifCurrentDisplayMode());
                    if (!sd)
                        return (OIerror);
                    if (sd->isElectrical())
                        cbin.setElec(sd);
                    else
                        cbin.setPhys(sd);
                    oldsym = false;
                }

                // Already in memory
                if (cbin.elec() && cbin.elec()->isBBsubng()) {
                    if (!cbin.elec()->fixBBs())
                        return (OIerror);
                }
                if (cbin.phys() && cbin.phys()->isBBsubng()) {
                    if (!cbin.phys()->fixBBs())
                        return (OIerror);
                }
                if (cbret)
                    *cbret = cbin;
                if (pathname)
                    *pathname = lstring::copy("(in memory)");
                if (cbin.isDevice() && cbin.isLibrary())
                    return (OIold);
                return (oldsym ? OIok : OInew);
            }
            // don't create a cell that would be confused with an archive file
            Errs()->add_error("%s file %s was not found.", TypeName(ft),
                name);
            return (OIerror);
        }

        // Determine file type.  The IsGDSII() test MUST come first
        //
        bool issced = false;
        CFtype ciftype = CFnone;
        if (IsGDSII(fp))
            type = Fgds;
        else if (IsCGX(fp))
            type = Fcgx;
        else if (IsOASIS(fp))
            type = Foas;
        else if (IsCIF(fp, &ciftype, &issced)) {
            if (ciftype == CFnative)
                type = Fnative;
            else
                type = Fcif;
        }
        else {
            sCHDin chd_in;
            if (!chd_in.check(fullpath)) {
                Errs()->add_error(
                    "File %s has unknown format.  Known formats are:\n"
                    " library, CHD, Xic, CGX, CIF, GDSII, OASIS.", name);
                fclose(fp);
                delete [] fullpath;
                return (OIerror);
            }
            else {
                chd = chd_in.read(fullpath, sCHDin::get_default_cgd_type());
                if (!chd) {
                    Errs()->add_error("Error reading CHD file.");
                    fclose(fp);
                    delete [] fullpath;
                    return (OIerror);
                }
                free_chd = true;
                fullname = chd->filename();
                name = lstring::strip_path(fullname);
                type = chd->filetype();
                delete [] fullpath;
                fullpath = lstring::copy(chd->filename());
            }
        }
        fclose(fp);
    }
    if (type == Fnone) {
        // never gets here
        delete [] fullpath;
        return (OIerror);
    }

    if (pathname)
        *pathname = lstring::copy(fullpath);

    // Now open the cell in the file.

    const char *new_cellname = 0;
    if (libref) {
        if (IsSupportedArchiveFormat(type) && !libref->cellname()) {
            // The library entry lacks a cell name.
            delete [] fullpath;
            if (callback) {
                stringlist *list = GetLibNamelist(libptr->filename(), LIBuser);
                if (cbdata)
                    cbdata->lib_filename = libptr->filename();
                ifAmbiguityCallback(list,
                    "References found in library - click to select",
                    libptr->filename(), (AMBIGUITY_CALLBACK)callback, cbdata);
                stringlist::destroy(list);
                return (OIambiguous);
            }
            else {
                Errs()->add_error("Ambiguous, no resolution method.");
                return (OIerror);
            }
        }
        // The aliasing is set from the last library, i.e., the one that
        // calls the archive file.  The cells are read into memory using
        // the key names from this library if given, or the original names
        // if not.
        //
        // The specified cell (only) is renamed from the first library,
        // i.e., that containing cellname.

        new_cellname = cellname;
        cellname = libref->name();
        if (!new_cellname)
            new_cellname = cellname;

        FIO()->SetReadingLibrary(true);
        // Newly read cells will have the Library flag set.
    }

    CDcbin cbin;
    OItype oiret = OIok;
    if (type == Fnative) {
        FILE *lfp = ifInitCvLog(READXIC_FN);
        oiret = FromNative(fullpath, &cbin, prms->scale());
        ifShowCvLog(lfp, oiret, READXIC_FN);
    }
    else if (type == Fcif)
        oiret = open_cif(fullpath, cellname, chd, prms, libref, libptr,
            callback, cbdata, &cbin);
    else if (type == Fgds)
        oiret = open_gds(fullpath, cellname, chd, prms, libref, libptr,
            callback, cbdata, &cbin);
    else if (type == Fcgx)
        oiret = open_cgx(fullpath, cellname, chd, prms, libref, libptr,
            callback, cbdata, &cbin);
    else if (type == Foas)
        oiret = open_oas(fullpath, cellname, chd, prms, libref, libptr,
            callback, cbdata, &cbin);
    delete [] fullpath;
    if (free_chd)
        delete chd;

    if (libref) {
        FIO()->SetReadingLibrary(false);
        if (OIfailed(oiret) || oiret == OIambiguous)
            return (oiret);
        if (oiret == OInew) {
            char *p = pathlist::mk_path(libref->dir(), libref->file());
            Errs()->add_error(oi_msg1, p);
            delete [] p;
            CD()->Close(cbin.cellname());
            return (OIerror);
        }

        // Change the name of the new cell, if necessary.
        if (strcmp(Tstring(cbin.cellname()), new_cellname)) {
            CDcellName oldname = cbin.cellname();
            if (!cbin.rename(new_cellname)) {
                Errs()->add_error(
                    "Name change from %s to %s failed,\n"
                    "cell is in memory under first name.",
                    Tstring(oldname), new_cellname);
                oiret = OIerror;
            }
        }
    }
    if (cbret)
        *cbret = cbin;
    return (oiret);
}


// Return the default file name (malloc'ed).  If ft is not the saved
// type, don't use the saved name.
//
char *
cFIO::DefaultFilename(CDcbin *cbin, FileType ft)
{
    if (!cbin)
        return (0);
    if (cbin->fileName() && ft == cbin->fileType() && !cbin->isAltered()) {
        // All cells from an archive file have the filename set to the
        // original source file.  We use this only when saving the
        // original top-level cell of the hierarchy.

        if (IsSupportedArchiveFormat(ft)) {
            if (cbin->isArchiveTopLevel())
                return (lstring::copy(cbin->fileName()));
        }
        else if (ft == Fnative) {
            const char *path = cbin->fileName();  // source directory
            const char *cn = Tstring(cbin->cellname());
            char *fn = new char[strlen(path) + strlen(cn) + 2];
            sprintf(fn, "%s/%s", path, cn);
            return (fn);
        }
    }
    if (ft == Fnone || ft == Fnative)
        return (lstring::copy(Tstring(cbin->cellname())));
    if (ft == Fgds) {
        char *fn = new char[strlen(Tstring(cbin->cellname())) + 8];
        strcpy(fn, Tstring(cbin->cellname()));
        if (TypeExt(fn) != Fgds)
            strcat(fn, ".gds");
        if (cbin->isCompressed())
            strcat(fn, ".gz");
        return (fn);
    }
    if (ft == Fcgx) {
        char *fn = new char[strlen(Tstring(cbin->cellname())) + 8];
        strcpy(fn, Tstring(cbin->cellname()));
        if (TypeExt(fn) != Fcgx)
            strcat(fn, ".cgx");
        if (cbin->isCompressed())
            strcat(fn, ".gz");
        return (fn);
    }
    if (ft == Foas) {
        char *fn = new char[strlen(Tstring(cbin->cellname())) + 8];
        strcpy(fn, Tstring(cbin->cellname()));
        if (TypeExt(fn) != Foas)
            strcat(fn, ".oas");
        return (fn);
    }
    if (ft == Fcif) {
        char *fn = new char[strlen(Tstring(cbin->cellname())) + 5];
        strcpy(fn, Tstring(cbin->cellname()));
        if (TypeExt(fn) != Fcif)
            strcat(fn, ".cif");
        return (fn);
    }
    return (0);
}


// Open filename/cellname, traversing libraries.  The returned file
// pointer points to a cell file (or inline native cell), if not
// nil.
//
OItype
cFIO::open_symbol_file(const char *fullname, const char *cellname,
    void (*callback)(const char*, OIcbData*), OIcbData *cbdata,
    FILE **pfp, char **ppath, sLibRef **pref, sLib **plib)
{
    *pfp = 0;
    *ppath = 0;
    *pref = 0;
    *plib = 0;

    char *fullpath;
    sLibRef *libref;
    sLib *libptr;
    FILE *fp = POpen(fullname, "rb", &fullpath, LIBuser, &libref, &libptr);

    // If fullname is rooted, the file will always be opened directly,
    // returning fp, copying the path into fullpath, and nulling
    // libref.  Otherwise, if fullname is a name token only, the
    // libraries are searched first, then the cell path, for a match. 
    // If libref is returned nonzero, the fullname token was found in
    // a library, then fullpath is the path to file opened
    // (libref->cellfile), and libref->u.symfile may be set if fp is
    // to an archive or library.

    if (!fp)
        return (OIok);
    if (filestat::get_file_type(fullpath) != GFT_FILE) {
        delete [] fullpath;
        fclose(fp);
        return (OIerror);
    }

    while (fp && IsLibrary(fp)) {
        fclose(fp);
        fp = 0;

        // Make sure that the library is open.  Note that reading from a
        // library has the effect of opening it.
        if (!OpenLibrary(0, fullpath)) {
            delete [] fullpath;
            return (OIerror);
        }
        if (cellname && *cellname) {
            fp = OpenLibFile(fullpath, cellname, LIBdevice | LIBuser,
                &libref, &libptr);
            if (!fp) {
                Errs()->add_error(oi_msg2, cellname, fullpath);
                delete [] fullpath;
                return (OIerror);
            }
            delete [] fullpath;
            if (libref->dir()) {
                fullpath = pathlist::mk_path(libref->dir(), libref->file());
                cellname = libref->cellname();
            }
            else {
                fullpath = lstring::copy(libref->name());
                cellname = 0;
            }
        }
        else if (callback) {
            libptr = FindLibrary(fullpath);
            delete [] fullpath;  // same as libptr->libfilename
            if (!libptr) {
                Errs()->add_error(
                    "OpenImport: insanity detected, no path to library!");
                return (OIerror);
            }
            stringlist *list = GetLibNamelist(libptr->filename(), LIBuser);
            if (cbdata)
                cbdata->lib_filename = libptr->filename();
            ifAmbiguityCallback(list,
                "References found in library - click to select",
                libptr->filename(), (AMBIGUITY_CALLBACK)callback, cbdata);
            stringlist::destroy(list);
            return (OIambiguous);
        }
        else {
            delete [] fullpath;
            Errs()->add_error("Ambiguous, no resolution method.");
            return (OIerror);
        }
    }
    *pfp = fp;
    *ppath = fullpath;
    *pref = libref;
    *plib = libptr;
    return (OIok);
}


namespace {
    const char *lib_msg = "Reading library cell %s from %s.";
}

// Private supporting function to open a CIF file.
//
OItype
cFIO::open_cif(const char *fullpath, const char *cellname, cCHD *chd,
    const FIOreadPrms *prms, sLibRef *libref, sLib *libptr,
    void (*callback)(const char*, OIcbData*), OIcbData *cbdata, CDcbin *cbret)
{
    OItype oiret = OIok;
    bool from_lib = libptr && libref;
    FILE *lfp = 0;
    if (!from_lib)
        lfp = ifInitCvLog(READCIF_FN);
    const char *fin = 0;
    if (cellname && *cellname) {
        if (from_lib)
            chd = libptr->get_chd(libref);

        if (chd) {
            fin = lstring::strip_path(chd->filename());
            if (from_lib) {
                FIO()->ifPrintCvLog(IFLOG_INFO, lib_msg, cellname,
                    libref->file());
                oiret = chd->open(cbret, cellname, prms, true);
            }
            else
                oiret = chd->open(cbret, cellname, prms, true);
        }
        else {
            FIOaliasTab *atab = 0;
            if (from_lib) {
                atab = new FIOaliasTab(false, false);
                atab->add_lib_alias(libref, libptr);
                atab->set_auto_rename(IsAutoRename());
            }
            fin = lstring::strip_path(fullpath);
            chd = NewCHD(fullpath, Fcif, Electrical, atab);
            delete atab;
            if (chd) {
                if (from_lib) {
                    libptr->set_chd(libref, chd);
                    FIO()->ifPrintCvLog(IFLOG_INFO, lib_msg, cellname,
                        libref->file());
                    oiret = chd->open(cbret, cellname, prms, true);
                }
                else {
                    oiret = chd->open(cbret, cellname, prms, true);
                    delete chd;
                }
            }
            else
                oiret = OIerror;
        }
    }
    else {
        if (chd) {
            fin = lstring::strip_path(chd->filename());
            oiret = chd->open(cbret, cellname, prms, true);
        }
        else {
            fin = lstring::strip_path(fullpath);
            stringlist *tlp = 0, *tle = 0;
            oiret = DbFromCIF(fullpath, prms, &tlp, &tle);
            if (!OIfailed(oiret)) {
                bool ambg;
                find_top_symbol(cbret, callback, cbdata, tlp, tle, fullpath,
                    &ambg);
                if (ambg)
                    oiret = OIambiguous;
            }
            stringlist::destroy(tlp);
            stringlist::destroy(tle);
        }
    }
    if (!from_lib)
        ifShowCvLog(lfp, oiret, READCIF_FN);
    if (!OIfailed(oiret) && !cbret->phys() && !cbret->elec()) {
        if (cellname && *cellname) {
            Errs()->add_error(oi_msg2, cellname, fin ? fin : "input");
            oiret = OIerror;
        }
        else if (oiret != OIambiguous) {
            Errs()->add_error(oi_msg2, "any cells", fin ? fin : "input");
            oiret = OIerror;
        }
    }
    return (oiret);
}


// Private supporting function to open a GDSII file.
//
OItype
cFIO::open_gds(const char *fullpath, const char *cellname, cCHD *chd,
    const FIOreadPrms *prms, sLibRef *libref, sLib *libptr,
    void (*callback)(const char*, OIcbData*), OIcbData *cbdata, CDcbin *cbret)
{
    OItype oiret = OIok;
    bool from_lib = libptr && libref;
    FILE *lfp = 0;
    if (!from_lib)
        lfp = ifInitCvLog(READGDS_FN);
    const char *fin = 0;
    if (cellname && *cellname) {
        if (from_lib)
            chd = libptr->get_chd(libref);

        if (chd) {
            fin = lstring::strip_path(chd->filename());
            if (from_lib) {
                FIO()->ifPrintCvLog(IFLOG_INFO, lib_msg, cellname,
                    libref->file());
                oiret = chd->open(cbret, cellname, prms, true);
            }
            else
                oiret = chd->open(cbret, cellname, prms, true);
        }
        else {
            FIOaliasTab *atab = 0;
            if (from_lib) {
                atab = new FIOaliasTab(false, false);
                atab->add_lib_alias(libref, libptr);
                atab->set_auto_rename(IsAutoRename());
            }
            fin = lstring::strip_path(fullpath);
            chd = NewCHD(fullpath, Fgds, Electrical, atab);
            delete atab;
            if (chd) {
                if (from_lib) {
                    libptr->set_chd(libref, chd);
                    FIO()->ifPrintCvLog(IFLOG_INFO, lib_msg, cellname,
                        libref->file());
                    oiret = chd->open(cbret, cellname, prms, true);
                }
                else {
                    oiret = chd->open(cbret, cellname, prms, true);
                    delete chd;
                }
            }
            else
                oiret = OIerror;
        }
    }
    else {
        if (chd) {
            fin = lstring::strip_path(chd->filename());
            oiret = chd->open(cbret, cellname, prms, true);
        }
        else {
            fin = lstring::strip_path(fullpath);
            stringlist *tlp = 0, *tle = 0;
            oiret = DbFromGDSII(fullpath, prms, &tlp, &tle);
            if (!OIfailed(oiret)) {
                bool ambg;
                find_top_symbol(cbret, callback, cbdata, tlp, tle, fullpath,
                    &ambg);
                if (ambg)
                    oiret = OIambiguous;
            }
            stringlist::destroy(tlp);
            stringlist::destroy(tle);
        }
    }
    if (!from_lib)
        ifShowCvLog(lfp, oiret, READGDS_FN);
    if (!OIfailed(oiret) && !cbret->phys() && !cbret->elec()) {
        if (cellname && *cellname) {
            Errs()->add_error(oi_msg2, cellname, fin ? fin : "input");
            oiret = OIerror;
        }
        else if (oiret != OIambiguous) {
            Errs()->add_error(oi_msg2, "any cells", fin ? fin : "input");
            oiret = OIerror;
        }
    }
    return (oiret);
}


// Private supporting function to open a CGX file.
//
OItype
cFIO::open_cgx(const char *fullpath, const char *cellname, cCHD *chd,
    const FIOreadPrms *prms, sLibRef *libref, sLib *libptr,
    void (*callback)(const char*, OIcbData*), OIcbData *cbdata, CDcbin *cbret)
{
    OItype oiret = OIok;
    bool from_lib = libptr && libref;
    FILE *lfp = 0;
    if (!from_lib)
        lfp = ifInitCvLog(READCGX_FN);
    const char *fin = 0;
    if (cellname && *cellname) {
        if (from_lib)
            chd = libptr->get_chd(libref);

        if (chd) {
            fin = lstring::strip_path(chd->filename());
            if (from_lib) {
                FIO()->ifPrintCvLog(IFLOG_INFO, lib_msg, cellname,
                    libref->file());
                oiret = chd->open(cbret, cellname, prms, true);
            }
            else
                oiret = chd->open(cbret, cellname, prms, true);
        }
        else {
            FIOaliasTab *atab = 0;
            if (libref) {
                atab = new FIOaliasTab(false, false);
                atab->add_lib_alias(libref, libptr);
                atab->set_auto_rename(IsAutoRename());
            }
            fin = lstring::strip_path(fullpath);
            chd = NewCHD(fullpath, Fcgx, Electrical, atab);
            delete atab;
            if (chd) {
                if (from_lib) {
                    libptr->set_chd(libref, chd);
                    FIO()->ifPrintCvLog(IFLOG_INFO, lib_msg, cellname,
                        libref->file());
                    oiret = chd->open(cbret, cellname, prms, true);
                }
                else {
                    oiret = chd->open(cbret, cellname, prms, true);
                    delete chd;
                }
            }
            else
                oiret = OIerror;
        }
    }
    else {
        if (chd) {
            fin = lstring::strip_path(chd->filename());
            oiret = chd->open(cbret, cellname, prms, true);
        }
        else {
            fin = lstring::strip_path(fullpath);
            stringlist *tlp = 0, *tle = 0;
            oiret = DbFromCGX(fullpath, prms, &tlp, &tle);
            if (!OIfailed(oiret)) {
                bool ambg;
                find_top_symbol(cbret, callback, cbdata, tlp, tle, fullpath,
                    &ambg);
                if (ambg)
                    oiret = OIambiguous;
            }
            stringlist::destroy(tlp);
            stringlist::destroy(tle);
        }
    }
    if (!from_lib)
        ifShowCvLog(lfp, oiret, READCGX_FN);
    if (!OIfailed(oiret) && !cbret->phys() && !cbret->elec()) {
        if (cellname && *cellname) {
            Errs()->add_error(oi_msg2, cellname, fin ? fin : "input");
            oiret = OIerror;
        }
        else if (oiret != OIambiguous) {
            Errs()->add_error(oi_msg2, "any cells", fin ? fin : "input");
            oiret = OIerror;
        }
    }
    return (oiret);
}


// Private supporting function to open an OASIS file.
//
OItype
cFIO::open_oas(const char *fullpath, const char *cellname, cCHD *chd,
    const FIOreadPrms *prms, sLibRef *libref, sLib *libptr,
    void (*callback)(const char*, OIcbData*), OIcbData *cbdata, CDcbin *cbret)
{
    OItype oiret = OIok;
    bool from_lib = libptr && libref;
    FILE *lfp = 0;
    if (!from_lib)
        lfp = ifInitCvLog(READOAS_FN);
    const char *fin = 0;
    if (cellname && *cellname) {
        if (from_lib)
            chd = libptr->get_chd(libref);

        if (chd) {
            fin = lstring::strip_path(chd->filename());
            if (from_lib) {
                FIO()->ifPrintCvLog(IFLOG_INFO, lib_msg, cellname,
                    libref->file());
                oiret = chd->open(cbret, cellname, prms, true);
            }
            else
                oiret = chd->open(cbret, cellname, prms, true);
        }
        else {
            FIOaliasTab *atab = 0;
            if (libref) {
                atab = new FIOaliasTab(false, false);
                atab->add_lib_alias(libref, libptr);
                atab->set_auto_rename(IsAutoRename());
            }
            fin = lstring::strip_path(fullpath);
            chd = NewCHD(fullpath, Foas, Electrical, atab);
            delete atab;
            if (chd) {
                if (from_lib) {
                    libptr->set_chd(libref, chd);
                    FIO()->ifPrintCvLog(IFLOG_INFO, lib_msg, cellname,
                        libref->file());
                    oiret = chd->open(cbret, cellname, prms, true);
                }
                else {
                    oiret = chd->open(cbret, cellname, prms, true);
                    delete chd;
                }
            }
            else
                oiret = OIerror;
        }
    }
    else {
        if (chd) {
            fin = lstring::strip_path(chd->filename());
            oiret = chd->open(cbret, cellname, prms, true);
        }
        else {
            fin = lstring::strip_path(fullpath);
            stringlist *tlp = 0, *tle = 0;
            oiret = DbFromOASIS(fullpath, prms, &tlp, &tle);
            if (!OIfailed(oiret)) {
                bool ambg;
                find_top_symbol(cbret, callback, cbdata, tlp, tle, fullpath,
                    &ambg);
                if (ambg)
                    oiret = OIambiguous;
            }
            stringlist::destroy(tlp);
            stringlist::destroy(tle);
        }
    }
    if (!from_lib)
        ifShowCvLog(lfp, oiret, READOAS_FN);
    if (!OIfailed(oiret) && !cbret->phys() && !cbret->elec()) {
        if (cellname && *cellname) {
            Errs()->add_error(oi_msg2, cellname, fin ? fin : "input");
            oiret = OIerror;
        }
        else if (oiret != OIambiguous) {
            Errs()->add_error(oi_msg2, "any cells", fin ? fin : "input");
            oiret = OIerror;
        }
    }
    return (oiret);
}


// Find the top-level cell for the current display mode and return it. 
// If there are multiple top-level cells, pop up a selection box and
// set *ambiguous if a callback is given, and return the first symbol
// in alpha order.
//
void
cFIO::find_top_symbol(CDcbin *cbret, void (*callback)(const char*, OIcbData*),
    OIcbData *cbdata, stringlist *psl, stringlist *esl, const char *fname,
    bool *ambiguous)
{
    cbret->reset();
    *ambiguous = false;
    if (!psl && !esl)
        return;
    bool physmode = (ifCurrentDisplayMode() == Physical);
    stringlist *sl = physmode ? psl : esl;
    stringlist *bsl = physmode ? esl : psl;
    if (!sl) {
        sl = bsl;
        bsl = 0;
    }

    if ((!sl->next) && (!bsl || !bsl->next)) {
        if (!bsl || !strcmp(sl->string, bsl->string)) {
            if (CDcdb()->findSymbol(sl->string, cbret))
                return;
        }
    }

    stringlist::sort(sl);
    if (!sl->next)
        CDcdb()->findSymbol(sl->string, cbret);
    else {

        // See if there is one cell that is in both lists.
        int cnt = 0;
        char *smatch = 0;
        for (stringlist *s1 = sl; s1; s1 = s1->next) {
            for (stringlist *s2 = bsl; s2; s2 = s2->next) {
                if (!strcmp(s1->string, s2->string)) {
                    cnt++;
                    smatch = s1->string;
                    break;
                }
            }
        }
        if (cnt == 1) {
            CDcdb()->findSymbol(smatch, cbret);
            return;
        }

        if (callback) {
            ifAmbiguityCallback(sl,
                "Multiple top-level cell found - click to select",
                lstring::strip_path(fname), (AMBIGUITY_CALLBACK)callback,
                cbdata);
            *ambiguous = true;
        }
    }
}

