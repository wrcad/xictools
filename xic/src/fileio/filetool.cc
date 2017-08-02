
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#include "cd.h"
#include "fio.h"
#include "fio_assemble.h"
#include "fio_compare.h"
#include "fio_chd.h"
#include "geo.h"
#include "geo_zlist.h"
#include "geo_grid.h"
#include "filetool.h"
#include "main_variables.h"
#include "cvrt_variables.h"
#include "edit_variables.h"
#include "filestat.h"
#include "timedbg.h"

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>


//
// The core of the filetool program.  This can be used from within a
// larger application, or linked with a main as a stand-alone
// program.
//

cFileTool *cFileTool::ft_instance = 0;

const char *cFileTool::ft_usage =
    "\nfiletool\n"
    "  [-set var[=value] ...]\n"
    "  -eval script_file_to_read |\n"
    "  -info layout_file [flags] |\n"
    "  -text layout_file [text_opts] |\n"
    "  -comp layout_file1 layout_file2 [comp_opts] |\n"
    "  -split split_opts |\n"
    "  -cfile -i layout_file -o output_file [-g] [-c] |\n"
    "  translate_opts\n"
    "\n"
    "text_opts:\n"
    "  [-o output_file] [start[-end]] [-c cells] [-r recs]\n"
    "\n"
    "comp_opts:\n"
    "  [-c1 cell_list [-c2 cell_list]]  cells to compare\n"
    "  [-l layer_list [-s]]             layer list, only or skip\n"
    "  [-d]                             don't record differences, count only\n"
    "  [-r max_diffs]                   difference limit\n"
    "  [[-g]                            per-cell geometric mode\n"
    "    [-t obj_types]                 obejct type code\n"
    "    [-x]                           expand subcell arrays\n"
    "    [-h]                           check hierarchy under cell\n"
    "    [-e]] |                        compare electrical cells\n"
    "  [-f                              flat comparison mode\n"
    "    [-a L,B,R,T]                   comparison area\n"
    "    [-i fine_grid]                 fine grid cell size\n"
    "    [-m coarse_mult]]              coarse grid multiple of find grid\n"
    "\n"
    "split_opts:\n"
    "  -i layout_file\n"
    "  -o basename.ext                  eg. \"outname.gds\"\n"
    "  [-c cellname]                    top level cell to split\n"
    "  -g gridsize | -r l,b,r,t...      grid size or region list\n"
    "  [-b bloatval]\n"
    "  [-w l,b,r,t]                     area to split\n"
    "  [-f]                             flat output files\n"
    "  [-m]                             make flat top cellnames unique\n"
    "  [-cl]                            clip to grid in output\n"
    "  [-e]                             elide empty cells\n"
    "  [-p]                             use parallel write mode\n"
    "\n"
    "translate_opts:\n"
    "  [-d script_file_to_write]        no conversion, dump script\n"
    "  [-v[v]]                          verbose messages\n"
    "  -o output_file                   output file to create\n"
    "  [-t top_cell_name]               top cell name in output\n"
    "  [per-source_opts]\n"
    "  [per-instance_opts]\n"
    "  -i input_file                    input file path\n"
    "    [per-source_opts]\n"
    "    [per-instance_opts]\n"
    "    [[-c cellname | -ctop]         cell name in file, or use default\n"
    "      [-ca cellname_alias]         new name for cell\n"
    "      [per-instance_opts]\n"
    "    ... ]                          multiple cell blocks allowed\n"
    "  [-i ... ]                        multiple file blocks allowed\n"
    "\n"
    "per-source_opts:\n"
    "  -l layer_list        layer list\n"
    "  -n[-]                use only layers in list\n"
    "  -k[-]                skip layers in list\n"
    "  -a layer_aliases     layer alias list\n"
    "  -cs scale            scale factor if no instances\n"
    "  -p prefix            cell name prefix to add\n"
    "  -u suffix            cell name suffix to add\n"
    "  -tlo[-]              upper case cell names to lower case\n"
    "  -tup[-]              lower case cell names to upper case\n"
    "\n"
    "per-instance_opts:\n"
    "  -m magn              placement magnification\n"
    "  -x x                 placement x\n"
    "  -y y                 placement y\n"
    "  -ang angle           rotation angle, 45 multiple\n"
    "  -my[-]               mirror y\n"
    "  -s scale             scale factor\n"
    "  -h[-]                no hierarchy\n"
    "  -f[-]                flatten\n"
    "  -e[-]                delete empty cells\n"
    "  -w l,b,r,t           window\n"
    "  -cl[-]               clip to window\n\n";

// Special "set" variables that alter the filetool (only one so far).
//
// If set (as boolean) turn on time measurement debugs.  If this has a
// value, take is as a file name for time debug output.
#define FT_TIMEDBG "timedbg"

#define FT_DEF_LOGFILE "filetool.log"

// Processing method for job.
//
bool
jobdesc::run(const char *program, cFileTool *ft)
{
    ajob_t job(0);
    if (!job.parse(cmdstr)) {
        Errs()->add_error("%s: job setup failed.", program);
        return (false);
    }
    if (script_out && *script_out) {
        if (!filestat::create_bak(script_out)) {
            ft->printline(false, "%s: %s.\n", program, filestat::error_msg());
            return (false);
        }
        FILE *fp = fopen(script_out, "w");
        if (!fp) {
            Errs()->add_error("%s: error opening script file.", program);
            return (false);
        }
        if (!job.dump(fp)) {
            Errs()->add_error("%s: error writing script file.", program);
            return (false);
        }
        fclose(fp);
        return (true);
    }

    bool abort = false;
    if (!job.run(FT_DEF_LOGFILE)) {
        if (!abort)
            Errs()->add_error("%s: processing failed.", program);
        else
            Errs()->add_error("$s: task ABORTED on user request.", program);
        return (false);
    }
    return (true);
}
// End of jobdesc functions.


cFileTool::cFileTool(const char *version)
{
    if (!version)
        version = "0.0";
    ft_version = lstring::copy(version);

    ft_verbose = 0;
    ft_holding = false;
    ft_error = false;

    ft_instance = this;

    setupInterface();
    setupVariables();
}


cFileTool::~cFileTool()
{
    delete [] ft_version;
    revertInterface();
    CDvdb()->restoreInternal(ft_vars);
    ft_instance = 0;
}


bool
cFileTool::run(int argc, char **argv)
{
    CDvdb()->setVariable("VA_StripForExport", "");

    jobdesc *job = process_args(argc, argv);
    if (!job) {
        if (ft_error && Errs()->has_error())
            printline(false, "%s\n", Errs()->get_error());
        return (!ft_error);
    }

    TimeDbg tdbg("run");

    Errs()->init_error();
    bool ret = job->run(argv[0], this);
    if (ret) {
        if (job->script_out && *job->script_out)
            printline(false, "%s written successfully.\n",
                lstring::strip_path(job->script_out));
        else
            printline(false, "Operation completed successfully.\n");
    }
    else {
        printline(false, "%s\n", Errs()->get_error());
        ft_error = true;
    }

    delete job;
    return (ret);
}


// Defines for process_args.
//
#define match(x) !strcasecmp(a+1, x)
#define ck_arg(x) \
    if (!x) { \
        fprintf(stderr, tok_msg, argv[0], a); \
        delete job; \
        ft_error = true; \
        return (0); \
    }

namespace {
    // Return the next argument from argv, or 0 if not found.
    //
    inline char *
    nextarg(const char *s, int &i, int argc, char **argv)
    {
        i++;
        if (i == argc) {
            fprintf(stderr, "%s:  %s, missing token.\n", argv[0], s);
            return (0);
        }
        return (argv[i]);
    }

    char *
    to_string(int i, int argc, char **argv)
    {
        sLstr lstr;
        lstr.add(argv[i]);
        for (i++; i < argc; i++) {
            lstr.add_c(' ');
            bool hassp = false;
            for (char *s = argv[i]; *s; s++) {
                if (isspace(*s)) {
                    hassp = true;
                    break;
                }
            }
            if (hassp && (argv[i][0] != '"' ||
                    argv[i][strlen(argv[i])-1] != '"')) {
                lstr.add_c('"');
                lstr.add(argv[i]);
                lstr.add_c('"');
            }
            else
                lstr.add(argv[i]);
        }
        return (lstr.string_trim());
    }
}


// Set up and return a jobdesc according to the command line options.
//
jobdesc *
cFileTool::process_args(int argc, char **argv)
{
    jobdesc *job = 0;
    const char *bvmsg = "%s: %s, bad value %s.\n";
    const char *ord_msg = "%s: %s out of order.\n";
    const char *tok_msg = "%s: %s, token missing.\n";

    if (argc <= 1) {
        fputs(ft_usage, stderr);
        return (0);
    }
    if (argc == 2 && (!strcmp(argv[1], "?") || *argv[1] == '-')) {
        fputs(ft_usage, stderr);
        return (0);
    }

    int i = 1;
    for ( ; i < argc; i++) {
        char *a = argv[i];
        if (*a == '-') {
            if (match("set")) {
                char *ss = nextarg(a, i, argc, argv);
                ck_arg(ss)
                if (!process_set(ss)) {
                    fprintf(stderr, bvmsg, argv[0], a, ss);
                    delete job;
                    ft_error = true;
                    return (0);
                }
                continue;
            }
            if (match("eval")) {
                char *fn = nextarg(a, i, argc, argv);
                ck_arg(fn)
                job = new jobdesc;
                job->script_in = lstring::copy(fn);
                return (job);
            }
            if (match("info")) {
                char *fn = nextarg(a, i, argc, argv);
                ck_arg(fn)
                int flags = 0;
                if (i < argc - 1) {
                    char *fl = nextarg(a, i, argc, argv);
                    flags = cCHD::infoFlags(fl);
                }
                ft_error = !print_info(fn, flags);
                return (0);
            }
            if (match("comp")) {
                char *str = to_string(i+1, argc, argv);
                ft_error = !do_compare(str);
                delete [] str;
                return (0);
            }
            if (match("text")) {
                if (i != 1) {
                    fprintf(stderr, ord_msg, argv[0], a);
                    ft_error = true;
                    return (0);
                }
                char *fn = nextarg(a, i, argc, argv);
                ck_arg(fn)

                char *ofn = 0;
                sLstr lstr;
                for ( ; i < argc; i++) {
                    a = argv[i];
                    if (*a == '-' && match("o")) {
                        ofn = nextarg(a, i, argc, argv);
                        ck_arg(ofn)
                    }
                    else {
                        if (lstr.string())
                            lstr.add_c(' ');
                        lstr.add(argv[i]);
                    }
                }
                ft_error = !print_text(fn, ofn, lstr.string());
                return (0);
            }
            if (match("split")) {
                char *str = to_string(i+1, argc, argv);
                ft_error = !do_split(str);
                delete [] str;
                return (0);
            }
            if (match("cfile")) {
                char *str = to_string(i+1, argc, argv);
                ft_error = !do_chd_file(str);
                delete [] str;
                return (0);
            }
        }
        else {
            fprintf(stderr, "%s: unknown token %s.\n", argv[0], a);
            ft_error = true;
            return (0);
        }

        // Translation options follow.
        break;
    }

    job = new jobdesc;
    for ( ; i < argc; i++) {
        char *a = argv[i];
        if (*a == '-') {
            if (match("d")) {
                char *fn = nextarg(a, i, argc, argv);
                ck_arg(fn)
                job->script_out = lstring::copy(fn);
            }
            else if (match("v"))
                ft_verbose = 1;
            else if (match("vv"))
                ft_verbose = 2;
            else {
                job->cmdstr = to_string(i, argc, argv);
                break;
            }
        }
        else {
            fprintf(stderr, "%s: unknown token %s.\n", argv[0], a);
            ft_error = true;
            delete job;
            return (0);
        }
    }
    return (job);
}


// Handle the strings from the -set option.  The string can have any
// number of name and name=value tokens, with or without white space
// around '='.  If a value contains white space, it must be quoted.
//
bool
cFileTool::process_set(const char *str)
{
    char *ts = lstring::copy(str);
    GCarray<char*> gc_ts(ts);
    for (;;) {
        char *p = strchr(ts, '=');
        if (p) {
            *p++ = 0;
            int tcnt = 0;
            char *ts0 = ts;
            while (lstring::advtok(&ts))
                tcnt++;
            ts = ts0;
            if (tcnt < 1)
                return (false);
            for (int i = 1; i < tcnt; i++) {
                char *tok = lstring::gettok(&ts);
                CDvdb()->setVariable(tok, "");
                delete [] tok;
            }
            char *t1 = lstring::gettok(&ts);
            char *t2 = lstring::getqtok(&p);
            CDvdb()->setVariable(t1, t2 ? t2 : "");
            if (lstring::cieq(t1, FT_TIMEDBG)) {
                Tdbg()->set_active(true);
                if (t2)
                    Tdbg()->set_logfile(t2);
            }
            delete [] t1;
            delete [] t2;
            if (!t2)
                break;
            ts = p;
        }
        else {
            char *tok;
            while ((tok = lstring::gettok(&ts)) != 0) {
                CDvdb()->setVariable(tok, "");
                if (lstring::cieq(tok, FT_TIMEDBG))
                    Tdbg()->set_active(true);
                delete [] tok;
            }
            break;
        }
    }
    return (true);
}


// Print file info on stdout.
//
bool
cFileTool::print_info(const char *fname, int flags)
{
    if (!fname || !*fname) {
        Errs()->add_error("print_info: no filename given.\n");
        return (false);
    }
    FILE *fp = large_fopen(fname, "rb");
    if (!fp) {
        Errs()->add_error("print_info: can't open %s.\n", fname);
        return (false);
    }

    FileType ft = FIO()->GetFileType(fp);
    fclose(fp);
    if (FIO()->IsSupportedArchiveFormat(ft)) {
        cCHD *cx = FIO()->NewCHD(fname, ft, Physical, 0, cvINFOplpc);
        if (cx) {
            cx->prInfo(stdout, Physical, flags);
            delete cx;
        }
        else
            return (false);
    }
    else {
        Errs()->add_error("print_info: unknown file type.\n");
        return (false);
    }
    return (true);
}


bool
cFileTool::print_text(const char *infile, const char *outfile,
    const char *cmdargs)
{
    FILE *fp = large_fopen(infile, "rb");
    if (!fp) {
        Errs()->add_error("print_text: error opening archive file.");
        return (false);
    }
    FileType ft = FIO()->GetFileType(fp);
    fclose(fp);
    if (ft != Fgds && ft != Fcgx && ft != Foas) {
        Errs()->add_error("print_text: unsupported archive file type.");
        return (false);
    }
    return (FIO()->ConvertToText(ft, infile, outfile, cmdargs));
}


void
cFileTool::printline(bool hold, const char *fmt, ...)
{
    va_list args;

    if (ft_holding && !hold) {
        fprintf(stdout, "\n");
        ft_holding = false;
    }
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    ft_holding = hold;
}


bool
cFileTool::do_split(const char *s)
{
    bool ret = FIO()->SplitArchive(s);
    if (ret)
        printf("Done, success.\n");
    else
        printf("Done, FAILED.\n");
    return (ret);
}


bool
cFileTool::do_chd_file(const char *s)
{
    bool ret = FIO()->WriteCHDfile(s);
    if (ret)
        printf("Done, success.\n");
    else
        printf("Done, FAILED.\n");
    return (ret);
}


#define DIFF_LOG_FILE "diff.log"

bool
cFileTool::do_compare(const char *string)
{
    cCompare cmp;
    if (!cmp.parse(string)) {
        fprintf(stderr, "Error: %s", Errs()->get_error());
        return (false);
    }
    if (!cmp.setup()) {
        fprintf(stderr, "Error: %s", Errs()->get_error());
        return (false);
    }
    DFtype df = cmp.compare();

    if (df == DFabort) {
        printf("\nComparison interrupted.");
        return (false);
    }
    if (df == DFerror) {
        printf("\nComparison failed: %s.", Errs()->get_error());
        return (false);
    }
    printf("\nComparison data written to file \"%s\".\n", DIFF_LOG_FILE);
    return (true);
}


//------------------------------------------------------------------
// The CD/FIO interface.
//
// This completely encapsulates the interface state to the provided
// callbacks.  The interface is established in the constructor, and
// reverted to the provious state in the destructor.
//------------------------------------------------------------------

// Set up our private interface, saving the previous callbacks.  This
// is called in the constructor.
//
void
cFileTool::setupInterface()
{
    ft_cvt_log_fp = 0;
    ft_cvt_show_log = false;

    cdif_info_message = CD()->RegisterIfInfoMessage(ifInfoMessage);
    cdif_id_string = CD()->RegisterIfIdString(ifIdString);
    cdif_check_interrupt = CD()->RegisterIfCheckInterrupt(ifCheckInterrupt);

    fioif_init_cv_log = FIO()->RegisterIfInitCvLog(ifInitCvLog);
    fioif_print_cv_log = FIO()->RegisterIfPrintCvLog(ifPrintCvLog);
    fioif_show_cv_log = FIO()->RegisterIfShowCvLog(ifShowCvLog);
    fioif_info_message = FIO()->RegisterIfInfoMessage(ifInfoMessage);
    fioif_ambiguity_callback =
        FIO()->RegisterIfAmbiguityCallback(ifAmbiguityCallback);
}


// Revert to the initial interface, called from destructor.
void
cFileTool::revertInterface()
{
    CD()->RegisterIfInfoMessage(cdif_info_message);
    CD()->RegisterIfIdString(cdif_id_string);
    CD()->RegisterIfCheckInterrupt(cdif_check_interrupt);

    FIO()->RegisterIfInitCvLog(fioif_init_cv_log);
    FIO()->RegisterIfPrintCvLog(fioif_print_cv_log);
    FIO()->RegisterIfShowCvLog(fioif_show_cv_log);
    FIO()->RegisterIfInfoMessage(fioif_info_message);
    FIO()->RegisterIfAmbiguityCallback(fioif_ambiguity_callback);
}


//
// The callbacks.
//

// Static function.
FILE *
cFileTool::ifInitCvLog(const char *fname)
{
    FILE *fp = 0;
    if (fname) {
        fp = fopen(fname, "w");
        if (fp)
            fprintf(fp, "# %s\n", ifIdString());
        else
            CD()->ifInfoMessage(IFMSG_INFO,
                "Warning: Can't open log file.  Continuing anyway.");
    }
    ft_instance->ft_cvt_log_fp = fp;
    ft_instance->ft_cvt_show_log = false;
    return (fp);
}


// Static function.
// Function used by the translators to issue messages.
//
void
cFileTool::ifPrintCvLog(OUTmsgType msgtype, const char *fmt, va_list args)
{
    char buf[BUFSIZ];
    *buf = 0;
    if (msgtype == IFLOG_WARN) {
        strcpy(buf, "**  Warning: ");
        if (ft_instance->ft_cvt_log_fp)
            ft_instance->ft_cvt_show_log = true;
    }
    else if (msgtype == IFLOG_FATAL) {
        strcpy(buf, "*** Error: ");
        if (ft_instance->ft_cvt_log_fp)
            ft_instance->ft_cvt_show_log = true;
    }
    char *s = buf + strlen(buf);
    vsnprintf(s, BUFSIZ - (s - buf), fmt, args);

    if (ft_instance->ft_cvt_log_fp)
        fprintf(ft_instance->ft_cvt_log_fp, "%s\n", buf);
}


// Static function.
void
cFileTool::ifShowCvLog(FILE *fp, OItype, const char*)
{
    ft_instance->ft_cvt_log_fp = 0;
    if (!fp) {
        ft_instance->ft_cvt_show_log = false;
        return;
    }
    fclose(fp);
    if (!ft_instance->ft_cvt_show_log)
        return;
    ft_instance->ft_cvt_show_log = false;
}


// Static function.
// Process an informational message.  These messages would generally
// go to the screen.
//
void
cFileTool::ifInfoMessage(INFOmsgType code, const char *string, va_list args)
{
    static char pbuf1[80];
    static char pbuf2[80];

    char buf[512];
    if (!string)
        string = "";
    vsnprintf(buf, 512, string, args);

    switch (code) {
    case IFMSG_INFO:
        if (ft_instance->ft_verbose > 0)
            ft_instance->printline(false, "%s\n", buf);
        break;
    case IFMSG_RD_PGRS:
        if (ft_instance->ft_verbose > 1) {
            strcpy(pbuf1, buf);
            ft_instance->printline(true, "\r%-32s %-32s\r", pbuf1, pbuf2);
            fflush(stdout);
        }
        break;
    case IFMSG_WR_PGRS:
        if (ft_instance->ft_verbose > 1) {
            strcpy(pbuf2, buf);
            ft_instance->printline(true, "\r%-32s %-32s\r", pbuf1, pbuf2);
            fflush(stdout);
        }
        break;
    case IFMSG_CNAME:
        if (ft_instance->ft_verbose > 1)
            ft_instance->printline(false, "%s\n", buf);
        break;
    case IFMSG_POP_ERR:
    case IFMSG_LOG_ERR:
        ft_instance->printline(false, "%s\n", buf);
        break;
    case IFMSG_POP_WARN:
    case IFMSG_LOG_WARN:
        if (ft_instance->ft_verbose > 0)
            ft_instance->printline(false, "%s\n", buf);
        break;
    case IFMSG_POP_INFO:
        if (ft_instance->ft_verbose > 0)
            ft_instance->printline(false, "%s\n", buf);
        break;
    }
}


// Static function.
// Return static string containing version data and date
//
const char *
cFileTool::ifIdString()
{
    return(ft_instance->ft_version);
}


// Static function.
void
cFileTool::ifAmbiguityCallback(stringlist*, const char*, const char*,
    void (*)(const char*, void*), void*)
{
}


// Static function.
// Check for interrupt, return true if interrupted.
//
bool
cFileTool::ifCheckInterrupt(const char*)
{
    return (false);
}


//------------------------------------------------------------------
// The Variables interface.
//
// This encapsulates the variable handling.  The prior variable state
// and action procs are restored in the destructor.

//
// Local action procs
//

namespace {
    // A smarter atoi()
    //
    inline bool
    str_to_int(int *iret, const char *s)
    {
        if (!s)
            return (false);
        return (sscanf(s, "%d", iret) == 1);
    }

    // A smarter atof()
    //
    inline bool
    str_to_dbl(double *dret, const char *s)
    {
        if (!s)
            return (false);
        return (sscanf(s, "%lf", dret) == 1);
    }
}


//
// Database Setup
//

namespace {
    bool
    evDatabaseResolution(const char *vstring, bool set)
    {
        if (set) {
            int res = atoi(vstring);
            if (res == 1000 || res == 2000 || res == 5000 || res == 10000)
                CDphysResolution = res;
            else {
                fprintf(stderr,
        "Incorrect DatabaseResolution: must be 1000, 2000, 5000, or 10000.\n");
                exit(1);
            }
        }
        else
            CDphysResolution = 1000;
        return (true);
    }
}


//
// Symbol Path
//

namespace {
    bool
    evPath(const char *vstring, bool set)
    {
        if (set)
            FIO()->PSetPath(vstring);

        // This variables can't be unset.
        return (set);
    }

    bool
    evNoReadExclusive(const char*, bool set)
    {
        FIO()->SetNoReadExclusive(set);
        return (true);
    }

    bool
    evAddToBack(const char*, bool set)
    {
        FIO()->SetAddToBack(set);
        return (true);
    }
}


//
// Conversion - General
//

namespace {
    bool
    evChdFailOnUnresolved(const char*, bool set)
    {
        FIO()->SetChdFailOnUnresolved(set);
        return (true);
    }

    bool
    evMultiMapOk(const char*, bool set)
    {
        FIO()->SetMultiLayerMapOk(set);
        return (true);
    }

    bool
    evUnknownGdsLayerBase(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 65535)
                FIO()->SetUnknownGdsLayerBase(i);
            else {
                fprintf(stderr,
                "Incorrect UnknownGdsLayerBase: requires integer 0-65535.\n");
                exit(1);
            }
        }
        else
            FIO()->SetUnknownGdsLayerBase(FIO_UNKNOWN_LAYER_BASE);
        return (true);
    }

    bool
    evUnknownGdsDatatype(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 65535)
                FIO()->SetUnknownGdsDatatype(i);
            else {
                fprintf(stderr,
                "Incorrect UnknownGdsDatatype: requires integer 0-65535.\n");
                return (false);
            }
        }
        else
            FIO()->SetUnknownGdsDatatype(FIO_UNKNOWN_DATATYPE);
        return (true);
    }

    bool
    evNoStrictCellNames(const char*, bool set)
    {
        FIO()->SetNoStrictCellnames(set);
        return (true);
    }
}


//
// Conversion - Import and Conversion Commands
//

namespace {
    bool
    evAutoRename(const char*, bool set)
    {
        FIO()->SetAutoRename(set);
        return (true);
    }

    bool
    evNoOverwritePhys(const char*, bool set)
    {
        FIO()->SetNoOverwritePhys(set);
        return (true);
    }

    bool
    evNoOverwriteElec(const char*, bool set)
    {
        FIO()->SetNoOverwriteElec(set);
        return (true);
    }

    bool
    evNoOverwriteLibCells(const char*, bool set)
    {
        FIO()->SetNoOverwriteLibCells(set);
        return (true);
    }

    bool
    evNoCheckEmpties(const char*, bool set)
    {
        FIO()->SetNoCheckEmpties(set);
        return (true);
    }

    bool
    evNoReadLabels(const char*, bool set)
    {
        FIO()->SetNoReadLabels(set);
        return (true);
    }

    bool
    evMergeInput(const char*, bool set)
    {
        FIO()->SetMergeInput(set);
        return (true);
    }

    bool
    evNoPolyCheck(const char*, bool set)
    {
        CD()->SetNoPolyCheck(set);
        return (true);
    }

    bool
    evDupCheckMode(const char *vset, bool set)
    {
        if (set) {
            if (vset && (*vset == 'r' || *vset == 'R'))
                CD()->SetDupCheckMode(cCD::DupRemove);
            else if (vset && (*vset == 'w' || *vset == 'W'))
                CD()->SetDupCheckMode(cCD::DupWarn);
            else
                CD()->SetDupCheckMode(cCD::DupNoTest);
        }
        else
            CD()->SetDupCheckMode(cCD::DupWarn);
        return (true);
    }

    bool
    evLayerList(const char *vstring, bool set)
    {
        FIO()->SetLayerList(set ? vstring : 0);
        return (true);
    }

    bool
    evUseLayerList(const char *vstring, bool set)
    {
        if (set) {
            if (*vstring == 'n' || *vstring == 'N')
                FIO()->SetUseLayerList(ULLskipList);
            else
                FIO()->SetUseLayerList(ULLonlyList);
        }
        else
            FIO()->SetUseLayerList(ULLnoList);
        return (true);
    }

    bool
    evLayerAlias(const char *vstring, bool set)
    {
        FIO()->SetLayerAlias(set ? vstring : 0);
        return (true);
    }

    bool
    evUseLayerAlias(const char*, bool set)
    {
        FIO()->SetUseLayerAlias(set);
        return (true);
    }

    bool
    evInToLower(const char*, bool set)
    {
        FIO()->SetInToLower(set);
        return (true);
    }

    bool
    evInToUpper(const char*, bool set)
    {
        FIO()->SetInToUpper(set);
        return (true);
    }

    bool
    evInUseAlias(const char *vstring, bool set)
    {
        if (set) {
            if (*vstring == 'r' || *vstring == 'R')
                FIO()->SetInUseAlias(UAread);
            else if (*vstring == 'w' || *vstring == 'W' ||
                    *vstring == 's' || *vstring == 'S')
                FIO()->SetInUseAlias(UAwrite);
            else
                FIO()->SetInUseAlias(UAupdate);
        }
        else
            FIO()->SetInUseAlias(UAnone);
        return (true);
    }

    bool
    evInCellNamePrefix(const char *vstring, bool set)
    {
        FIO()->SetInCellNamePrefix(set ? vstring : 0);
        return (true);
    }

    bool
    evInCellNameSuffix(const char *vstring, bool set)
    {
        FIO()->SetInCellNameSuffix(set ? vstring : 0);
        return (true);
    }

    bool
    evNoMapDatatypes(const char*, bool set)
    {
        FIO()->SetNoMapDatatypes(set);
        return (true);
    }

    bool
    evCifLayerMode(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 2)
                FIO()->CifStyle().set_lread_type((EXTlreadType)i);
            else {
                fprintf(stderr,
                    "Incorrect CifLayerMode: requires integer 0-2.\n");
                exit(1);
            }
        }
        else
            FIO()->CifStyle().set_lread_type(EXTlreadDef);
        return (true);
    }

    bool
    evOasReadNoChecksum(const char*, bool set)
    {
        FIO()->SetOasReadNoChecksum(set);
        return (true);
    }

    bool
    evOasPrintNoWrap(const char*, bool set)
    {
        FIO()->SetOasPrintNoWrap(set);
        return (true);
    }

    bool
    evOasPrintOffset(const char*, bool set)
    {
        FIO()->SetOasPrintOffset(set);
        return (true);
    }
}


//
// Conversion - Export Commands
//

namespace {
    bool
    evStripForExport(const char*, bool set)
    {
        FIO()->SetStripForExport(set);
        return (true);
    }

    bool
    evWriteAllCells(const char*, bool set)
    {
        FIO()->SetWriteAllCells(set);
        return (true);
    }

    bool
    evSkipInvisible(const char *vstring, bool set)
    {
        FIO()->SetSkipInvisiblePhys(false);
        FIO()->SetSkipInvisibleElec(false);
        if (set) {
            if (*vstring != 'p' && *vstring != 'P')
                FIO()->SetSkipInvisibleElec(true);
            if (*vstring != 'e' && *vstring != 'E')
                FIO()->SetSkipInvisiblePhys(true);
        }
        return (true);
    }

    bool
    evKeepBadArchive(const char*, bool set)
    {
        FIO()->SetKeepBadArchive(set);
        return (true);
    }

    bool
    evNoCompressContext(const char*, bool set)
    {
        FIO()->SetNoCompressContext(set);
        return (true);
    }

    bool
    evRefCellAutoRename(const char*, bool set)
    {
        FIO()->SetRefCellAutoRename(set);
        return (true);
    }

    bool
    evUseCellTab(const char*, bool set)
    {
        FIO()->SetUseCellTab(set);
        return (true);
    }

    bool
    evSkipOverrideCells(const char*, bool set)
    {
        FIO()->SetSkipOverrideCells(set);
        return (true);
    }

    bool
    evOutToLower(const char*, bool set)
    {
        FIO()->SetOutToLower(set);
        return (true);
    }

    bool
    evOutToUpper(const char*, bool set)
    {
        FIO()->SetOutToUpper(set);
        return (true);
    }

    bool
    evOutUseAlias(const char *vstring, bool set)
    {
        if (set) {
            if (*vstring == 'r' || *vstring == 'R')
                FIO()->SetOutUseAlias(UAread);
            else if (*vstring == 'w' || *vstring == 'W' ||
                    *vstring == 's' || *vstring == 'S')
                FIO()->SetOutUseAlias(UAwrite);
            else
                FIO()->SetOutUseAlias(UAupdate);
        }
        else
            FIO()->SetOutUseAlias(UAnone);
        return (true);
    }

    bool
    evOutCellNamePrefix(const char *vstring, bool set)
    {
        FIO()->SetOutCellNamePrefix(set ? vstring : 0);
        return (true);
    }

    bool
    evOutCellNameSuffix(const char *vstring, bool set)
    {
        FIO()->SetOutCellNameSuffix(set ? vstring : 0);
        return (true);
    }

    bool
    evCifOutStyle(const char *vstring, bool set)
    {
        if (set) {
            Errs()->init_error();
            if (!FIO()->CifStyle().set(vstring)) {
                fprintf(stderr, "Incorrect CifOutStyle: %s\n",
                    Errs()->get_error());
                exit (1);
            }
        }
        else
            FIO()->CifStyle().set_def();
        return (true);
    }

    bool
    evCifOutExtensions(const char *vstring, bool set)
    {
        if (set) {
            int i1, i2;
            if (sscanf(vstring, "%d %d", &i1, &i2) != 2) {
                fprintf(stderr,
                    "Incorrect CitOutExtensions: requires two integers.\n");
                exit(1);
            }
            FIO()->CifStyle().set_flags(i1 & 0xfff);
            FIO()->CifStyle().set_flags_export(i2 & 0xfff);
        }
        else {
            FIO()->CifStyle().set_flags(0xfff);
            FIO()->CifStyle().set_flags_export(0);
        }
        return (true);
    }

    bool
    evCifAddBBox(const char*, bool set)
    {
        FIO()->CifStyle().set_add_obj_bb(set);
        return (true);
    }

    bool
    evGdsOutLevel(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 2)
                FIO()->SetGdsOutLevel(i);
            else {
                fprintf(stderr,
                    "Incorrect GdsOutLevel: requires integer 0-2.\n");
                exit(1);
            }
        }
        else
            FIO()->SetGdsOutLevel(0);
        return (true);
    }

    bool
    evGdsMunit(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) && d >= 0.01 && d <= 100.0) {
                FIO()->SetGdsMunit(d);
            }
            else {
                fprintf(stderr,
                    "Incorrect GdsMunit: range is 0.01 - 100.0.\n");
                exit(1);
            }
        }
        else
            FIO()->SetGdsMunit(1.0);
        return (true);
    }

    bool
    evNoGdsMapOk(const char*, bool set)
    {
        FIO()->SetNoGdsMapOk(set);
        return (true);
    }

    bool
    evOasWriteCompressed(const char *vstring, bool set)
    {
        if (set) {
            if (*vstring == 'f' || *vstring == 'F')
                FIO()->SetOasWriteCompressed(OAScompForce);
            else
                FIO()->SetOasWriteCompressed(OAScompSmart);
        }
        else
            FIO()->SetOasWriteCompressed(OAScompNone);
        return (true);
    }

    bool
    evOasWriteNameTab(const char*, bool set)
    {
        FIO()->SetOasWriteNameTab(set);
        return (true);
    }

    bool
    evOasWriteRep(const char *vstring, bool set)
    {
        FIO()->SetOasWriteRep(set ? vstring : 0);
        return (true);
    }

    bool
    evOasWriteChecksum(const char *vstring, bool set)
    {
        if (set) {
            if (*vstring == 2 || lstring::ciprefix("ch", vstring))
                FIO()->SetOasWriteChecksum(OASchksumBsum);
            else
                FIO()->SetOasWriteChecksum(OASchksumCRC);
        }
        else
            FIO()->SetOasWriteChecksum(OASchksumNone);
        return (true);
    }

    bool
    evOasWriteNoTrapezoids(const char*, bool set)
    {
        FIO()->SetOasWriteNoTrapezoids(set);
        return (true);
    }

    bool
    evOasWriteWireToBox(const char*, bool set)
    {
        FIO()->SetOasWriteWireToBox(set);
        return (true);
    }

    bool
    evOasWriteRndWireToPoly(const char*, bool set)
    {
        FIO()->SetOasWriteRndWireToPoly(set);
        return (true);
    }

    bool
    evOasWriteNoGCDcheck(const char*, bool set)
    {
        FIO()->SetOasWriteNoGCDcheck(set);
        return (true);
    }

    bool
    evOasWriteUseFastSort(const char*, bool set)
    {
        FIO()->SetOasWriteUseFastSort(set);
        return (true);
    }

    bool
    evOasWritePrptyMask(const char *vstring, bool set)
    {
        if (set) {
            int d;
            int msk = (OAS_PRPMSK_GDS_LBL | OAS_PRPMSK_XIC_LBL);
            if (!*vstring)
                FIO()->SetOasWritePrptyMask(msk);
            else if (sscanf(vstring, "%d", &d) == 1)
                FIO()->SetOasWritePrptyMask(msk & d);
            else
                FIO()->SetOasWritePrptyMask(OAS_PRPMSK_ALL);
        }
        else
            FIO()->SetOasWritePrptyMask(0);
        return (true);
    }
}


//
// Geometry
//

namespace {
    bool
    evJoinMaxPolyVerts(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && (i == 0 || (i >= 20 && i <= 8000)))
                Zlist::JoinMaxVerts = i;
            else {
                fprintf(stderr,
                    "Incorrect JoinMaxPolyVerts: 0 or range 20-8000.\n");
                exit(1);
            }
        }
        else
            Zlist::JoinMaxVerts = DEF_JoinMaxVerts;
        return (true);
    }

    bool
    evJoinMaxPolyGroup(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0)
                Zlist::JoinMaxGroup = i;
            else {
                fprintf(stderr,
                    "Incorrect JoinMaxPolyGroup: must be 0 or larger.\n");
                exit(1);
            }
        }
        else
            Zlist::JoinMaxGroup = DEF_JoinMaxGroup;
        return (true);
    }

    bool
    evJoinMaxPolyQueue(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0)
                Zlist::JoinMaxQueue = i;
            else {
                fprintf(stderr,
                    "Incorrect JoinMaxPolyQueue: must be 0 or larger.\n");
                return (false);
            }
        }
        else
            Zlist::JoinMaxQueue = DEF_JoinMaxQueue;
        return (true);
    }

    bool
    evJoinBreakClean(const char*, bool set)
    {
        Zlist::JoinBreakClean = set;
        return (true);
    }

    bool
    evPartitionSize(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) && INTERNAL_UNITS(d) >= 0)
                grd_t::set_def_gridsize(INTERNAL_UNITS(d));
            else {
                fprintf(stderr, "Incorrect PartitionSize: must be >= 0.\n");
                return (false);
            }
        }
        else
            grd_t::set_def_gridsize(INTERNAL_UNITS(DEF_GRD_PART_SIZE));
        return (true);
    }
}


#define HOOKVAR(n, f) ft_vars = CDvdb()->pushInternal(ft_vars, n,  f)

// Set up the variables' action procs, and save the current state. 
// This is called from the constructer.  The variables are reverted
// in the destructor.
//
void
cFileTool::setupVariables()
{
    ft_vars = 0;

    // Database Setup
    HOOKVAR(VA_DatabaseResolution,  evDatabaseResolution);

    // Symbol Path
    HOOKVAR(VA_Path,                evPath);
    HOOKVAR(VA_NoReadExclusive,     evNoReadExclusive);
    HOOKVAR(VA_AddToBack,           evAddToBack);

    // Conversion - General
    HOOKVAR(VA_ChdFailOnUnresolved, evChdFailOnUnresolved);
    HOOKVAR(VA_MultiMapOk,          evMultiMapOk);
    HOOKVAR(VA_UnknownGdsLayerBase, evUnknownGdsLayerBase);
    HOOKVAR(VA_UnknownGdsDatatype,  evUnknownGdsDatatype);
    HOOKVAR(VA_NoStrictCellnames,   evNoStrictCellNames);

    // Conversion - Import and Conversion Commands
    HOOKVAR(VA_AutoRename,          evAutoRename);
    HOOKVAR(VA_NoOverwritePhys,     evNoOverwritePhys);
    HOOKVAR(VA_NoOverwriteElec,     evNoOverwriteElec);
    HOOKVAR(VA_NoOverwriteLibCells, evNoOverwriteLibCells);
    HOOKVAR(VA_NoCheckEmpties,      evNoCheckEmpties);
    HOOKVAR(VA_NoReadLabels,        evNoReadLabels);
    HOOKVAR(VA_MergeInput,          evMergeInput);
    HOOKVAR(VA_NoPolyCheck,         evNoPolyCheck);
    HOOKVAR(VA_DupCheckMode,        evDupCheckMode);
    HOOKVAR(VA_LayerList,           evLayerList);
    HOOKVAR(VA_UseLayerList,        evUseLayerList);
    HOOKVAR(VA_LayerAlias,          evLayerAlias);
    HOOKVAR(VA_UseLayerAlias,       evUseLayerAlias);
    HOOKVAR(VA_InToLower,           evInToLower);
    HOOKVAR(VA_InToUpper,           evInToUpper);
    HOOKVAR(VA_InUseAlias,          evInUseAlias);
    HOOKVAR(VA_InCellNamePrefix,    evInCellNamePrefix);
    HOOKVAR(VA_InCellNameSuffix,    evInCellNameSuffix);
    HOOKVAR(VA_NoMapDatatypes,      evNoMapDatatypes);
    HOOKVAR(VA_CifLayerMode,        evCifLayerMode);
    HOOKVAR(VA_OasReadNoChecksum,   evOasReadNoChecksum);
    HOOKVAR(VA_OasPrintNoWrap,      evOasPrintNoWrap);
    HOOKVAR(VA_OasPrintOffset,      evOasPrintOffset);

    // Conversion - Export Commands
    HOOKVAR(VA_StripForExport,      evStripForExport);
    HOOKVAR(VA_WriteAllCells,       evWriteAllCells);
    HOOKVAR(VA_SkipInvisible,       evSkipInvisible);
    HOOKVAR(VA_KeepBadArchive,      evKeepBadArchive);
    HOOKVAR(VA_NoCompressContext,   evNoCompressContext);
    HOOKVAR(VA_RefCellAutoRename,   evRefCellAutoRename);
    HOOKVAR(VA_UseCellTab,          evUseCellTab);
    HOOKVAR(VA_SkipOverrideCells,   evSkipOverrideCells);
    HOOKVAR(VA_OutToLower,          evOutToLower);
    HOOKVAR(VA_OutToUpper,          evOutToUpper);
    HOOKVAR(VA_OutUseAlias,         evOutUseAlias);
    HOOKVAR(VA_OutCellNamePrefix,   evOutCellNamePrefix);
    HOOKVAR(VA_OutCellNameSuffix,   evOutCellNameSuffix);
    HOOKVAR(VA_CifOutStyle,         evCifOutStyle);
    HOOKVAR(VA_CifOutExtensions,    evCifOutExtensions);
    HOOKVAR(VA_CifAddBBox,          evCifAddBBox);
    HOOKVAR(VA_GdsOutLevel,         evGdsOutLevel);
    HOOKVAR(VA_GdsMunit,            evGdsMunit);
    HOOKVAR(VA_NoGdsMapOk,          evNoGdsMapOk);
    HOOKVAR(VA_OasWriteCompressed,  evOasWriteCompressed);
    HOOKVAR(VA_OasWriteNameTab,     evOasWriteNameTab);
    HOOKVAR(VA_OasWriteRep,         evOasWriteRep);
    HOOKVAR(VA_OasWriteChecksum,    evOasWriteChecksum);
    HOOKVAR(VA_OasWriteNoTrapezoids,evOasWriteNoTrapezoids);
    HOOKVAR(VA_OasWriteWireToBox,   evOasWriteWireToBox);
    HOOKVAR(VA_OasWriteRndWireToPoly,evOasWriteRndWireToPoly);
    HOOKVAR(VA_OasWriteNoGCDcheck,  evOasWriteNoGCDcheck);
    HOOKVAR(VA_OasWriteUseFastSort, evOasWriteUseFastSort);
    HOOKVAR(VA_OasWritePrptyMask,   evOasWritePrptyMask);

    // Geometry
    HOOKVAR(VA_JoinMaxPolyVerts,    evJoinMaxPolyVerts);
    HOOKVAR(VA_JoinMaxPolyGroup,    evJoinMaxPolyGroup);
    HOOKVAR(VA_JoinMaxPolyQueue,    evJoinMaxPolyQueue);
    HOOKVAR(VA_JoinBreakClean,      evJoinBreakClean);
    HOOKVAR(VA_PartitionSize,       evPartitionSize);
}

