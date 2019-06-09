
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
#include "ext.h"
#include "ext_extract.h"
#include "ext_errlog.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "errorlog.h"
#include "promptline.h"
#include "miscutil/filestat.h"

//
// Logging and error recording for use during extraction.
//


cExtErrLog::cExtErrLog()
{
    el_cellname = 0;
    el_logfp = 0;
    el_errfp = 0;
    el_logfile = 0;
    el_errfile = 0;

    el_group_errcnt = 0;
    el_extract_errcnt = 0;
    el_associate_errcnt = 0;
    el_state = ELidle;
    el_warned_g_log = false;
    el_warned_e_log = false;
    el_warned_a_log = false;
    el_warned_err = false;

    el_log_grouping = false;
    el_log_extracting = false;
    el_log_associating = false;
    el_verbose = false;

    el_rlex_fp = 0;
    el_rlex_log = false;
    el_rlex_msg = false;
}


cExtErrLog::~cExtErrLog()
{
    close_files();
    delete [] el_logfile;
    delete [] el_errfile;
    if (el_rlex_fp)
        fclose(el_rlex_fp);
}


void
cExtErrLog::start_logging(ExtLogType type, CDcellName cellname)
{
    el_cellname = cellname;
    switch (type) {
    case ExtLogGrp:
    case ExtLogGrpV:
        el_state = ELgrouping;
        el_group_errcnt = 0;
        break;
    case ExtLogExt:
    case ExtLogExtV:
        el_state = ELextracting;
        el_extract_errcnt = 0;
        break;
    case ExtLogAssoc:
    case ExtLogAssocV:
        el_state = ELassociating;
        el_associate_errcnt = 0;
        break;
    }

    open_files();
    if (el_logfp)
        PL()->TeePrompt(el_logfp);
}


void
cExtErrLog::end_logging()
{
    close_files();
    if (el_state == ELgrouping) {
        if (el_group_errcnt > 0) {
            if (el_errfile)
                DSPmainWbag(PopUpFileBrowser(el_errfile))
        }
    }
    else if (el_state == ELextracting) {
        if (el_extract_errcnt > 0) {
            if (el_errfile)
                DSPmainWbag(PopUpFileBrowser(el_errfile))
        }
    }
    else if (el_state == ELassociating) {
        if (el_associate_errcnt > 0) {
            if (el_errfile)
                DSPmainWbag(PopUpFileBrowser(el_errfile))
        }
    }
    el_state = ELidle;
}


// If logging, print the line to the log file, and add a newline.  The
// text is printed verbatim.
//
void
cExtErrLog::add_log(ExtLogType type, const char *fmt, ...)
{
    if (!el_logfp)
        return;
    switch (type) {
    case ExtLogGrpV:
        if (!el_verbose)
            return;
        // fallthrough
    case ExtLogGrp:
        if (!el_log_grouping)
            return;
        break;
    case ExtLogExtV:
        if (!el_verbose)
            return;
        // fallthrough
    case ExtLogExt:
        if (!el_log_extracting)
            return;
        break;
    case ExtLogAssocV:
        if (!el_verbose)
            return;
        // fallthrough
    case ExtLogAssoc:
        if (!el_log_associating)
            return;
        break;
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(el_logfp, fmt, args);
    va_end(args);
    fputc('\n', el_logfp);
}


// Print an error message and trailing newline.
//
void
cExtErrLog::add_err(const char *fmt, ...)
{
    if (!el_errfp)
        return;
    va_list args;
    va_start(args, fmt);
    vfprintf(el_errfp, fmt, args);
    va_end(args);
    fputc('\n', el_errfp);
    if (el_logfp) {
        va_start(args, fmt);
        vfprintf(el_logfp, fmt, args);
        va_end(args);
        fputc('\n', el_logfp);
    }

    if (el_state == ELgrouping)
        el_group_errcnt++;
    else if (el_state == ELextracting)
        el_extract_errcnt++;
    else if (el_state == ELassociating)
        el_associate_errcnt++;
}


// Add a message and trailing newline.
//
void
cExtErrLog::add_dev_err(const CDs *sdesc, const sDevInst *di,
    const char *string, ...)
{
    if (!el_errfp)
        return;
    if (!sdesc)
        return;
    if (di) {
        if (di->desc()->prefix()) {
            fprintf(el_errfp, "In %s, device %s, prefix %s, index %d:\n",
                TstringNN(sdesc->cellname()), TstringNN(di->desc()->name()),
                di->desc()->prefix(), di->index());
            if (el_logfp) {
                fprintf(el_logfp, "In %s, device %s, prefix %s, index %d:\n",
                    TstringNN(sdesc->cellname()),
                    TstringNN(di->desc()->name()),
                    di->desc()->prefix(), di->index());
            }
        }
        else {
            fprintf(el_errfp, "In %s, device %s, index %d:\n",
                TstringNN(sdesc->cellname()), TstringNN(di->desc()->name()),
                di->index());
            if (el_logfp) {
                fprintf(el_logfp, "In %s, device %s, index %d:\n",
                    TstringNN(sdesc->cellname()),
                    TstringNN(di->desc()->name()), di->index());
            }
        }
    }
    else {
        fprintf(el_errfp, "In %s: ", TstringNN(sdesc->cellname()));
        if (el_logfp)
            fprintf(el_logfp, "In %s: ", TstringNN(sdesc->cellname()));
    }

    char buf[2048];
    va_list args;
    va_start(args, string);
    vsnprintf(buf, 2048, string, args); 
    va_end(args);

    char *t = buf + strlen(buf) - 1;
    while (t > buf && isspace(*t))
        *t-- = 0;
    if (*t != '.') {
        t[1] = '.';
        t[2] = 0;
    }
    fputs(buf, el_errfp);
    fputc('\n', el_errfp);
    if (el_logfp) {
        fputs(buf, el_logfp);
        fputc('\n', el_logfp);
    }

    if (el_state == ELgrouping)
        el_group_errcnt++;
    else if (el_state == ELextracting)
        el_extract_errcnt++;
    else if (el_state == ELassociating)
        el_associate_errcnt++;
}


FILE *
cExtErrLog::open_rlsolver_log()
{
    sLstr lstr;
    if (Log()->LogDirectory() && *Log()->LogDirectory()) {
        lstr.add(Log()->LogDirectory());
        lstr.add_c('/');
    }
    lstr.add(EL_RLSOLVERLOG);

    FILE *fp = 0;
    if (filestat::create_bak(lstr.string())) {
        fp = fopen(lstr.string(), "w");
        if (fp) {
            fprintf(fp, "# %s  %s\n\n", EL_RLSOLVERLOG,
                XM()->IdString());
        }
    }
    return (fp);
}


void
cExtErrLog::open_files()
{
    close_files();

    sLstr lstr_errs;
    sLstr lstr;

    if (Log()->LogDirectory() && *Log()->LogDirectory()) {
        lstr.add(Log()->LogDirectory());
        lstr.add_c('/');
    }
    lstr.add(EL_ERRFILE);

    if (el_state == ELgrouping) {
        delete [] el_errfile;
        el_errfile = 0;
        if (filestat::create_bak(lstr.string())) {
            el_errfp = fopen(lstr.string(), "w");
            if (el_errfp) {
                fprintf(el_errfp, "# %s  %s\n", EL_ERRFILE, XM()->IdString());
                el_errfile = lstr.string_trim();
                fprintf(el_errfp, "Grouping top cell: %s\n",
                    Tstring(el_cellname));
            }
        }
    }
    else if (el_state == ELextracting) {
        el_errfp = fopen(lstr.string(), "a");
        if (el_errfp)
            fprintf(el_errfp, "Extracting top cell: %s\n",
                Tstring(el_cellname));
    }
    else if (el_state == ELassociating) {
        el_errfp = fopen(lstr.string(), "a");
        if (el_errfp)
            fprintf(el_errfp, "Associating top cell: %s\n",
                Tstring(el_cellname));
    }
    if (!el_errfp) {
        if (!el_warned_err) {
            lstr_errs.add("Can't open ");
            lstr_errs.add(EL_ERRFILE);
            lstr_errs.add(", error messages will go to console window.\n");
            lstr_errs.add("Error: ");
            lstr_errs.add(filestat::error_msg());
            lstr_errs.add_c('\n');
            el_warned_err = true;
        }
        el_errfp = stderr;
    }

    lstr.free();

    if (Log()->LogDirectory() && *Log()->LogDirectory()) {
        lstr.add(Log()->LogDirectory());
        lstr.add_c('/');
    }

    if (el_state == ELgrouping) {
        delete [] el_logfile;
        el_logfile = 0;
        if (el_log_grouping) {
            lstr.add(EL_GROUP_LOG);
            if (filestat::create_bak(lstr.string())) {
                el_logfp = fopen(lstr.string(), "w");
                if (el_logfp) {
                    fprintf(el_logfp, "# %s  %s\n", EL_GROUP_LOG,
                        XM()->IdString());
                    el_logfile = lstr.string_trim();
                    fprintf(el_logfp, "Grouping top cell: %s\n",
                        Tstring(el_cellname));
                }
                else if (!el_warned_g_log) {
                    lstr_errs.add("Can't open ");
                    lstr_errs.add(EL_GROUP_LOG);
                    lstr_errs.add(", no logfile will be generated.\n");
                    lstr_errs.add("Error: ");
                    lstr_errs.add(filestat::error_msg());
                    lstr_errs.add_c('\n');
                    el_warned_g_log = true;
                }
            }
        }
    }
    else if (el_state == ELextracting) {
        if (el_log_extracting) {
            lstr.add(EL_EXTRACT_LOG);
            if (filestat::create_bak(lstr.string())) {
                el_logfp = fopen(lstr.string(), "w");
                if (el_logfp) {
                    fprintf(el_logfp, "# %s  %s\n", EL_EXTRACT_LOG,
                        XM()->IdString());
                    el_logfile = lstr.string_trim();
                    fprintf(el_logfp, "Extracting top cell: %s\n",
                        Tstring(el_cellname));
                }
                else if (!el_warned_e_log) {
                    lstr_errs.add("Can't open ");
                    lstr_errs.add(EL_EXTRACT_LOG);
                    lstr_errs.add(", no logfile will be generated.\n");
                    lstr_errs.add("Error: ");
                    lstr_errs.add(filestat::error_msg());
                    lstr_errs.add_c('\n');
                    el_warned_e_log = true;
                }
            }
        }
    }
    else if (el_state == ELassociating) {
        if (el_log_associating) {
            lstr.add(EL_ASSOCIATE_LOG);
            if (filestat::create_bak(lstr.string())) {
                el_logfp = fopen(lstr.string(), "w");
                if (el_logfp) {
                    fprintf(el_logfp, "# %s  %s\n", EL_ASSOCIATE_LOG,
                        XM()->IdString());
                    el_logfile = lstr.string_trim();
                    fprintf(el_logfp, "Associating top cell: %s\n",
                        Tstring(el_cellname));
                }
                else if (!el_warned_a_log) {
                    lstr_errs.add("Can't open ");
                    lstr_errs.add(EL_ASSOCIATE_LOG);
                    lstr_errs.add(", no logfile will be generated.\n");
                    lstr_errs.add("Error: ");
                    lstr_errs.add(filestat::error_msg());
                    lstr_errs.add_c('\n');
                    el_warned_a_log = true;
                }
            }
        }
    }
    if (lstr_errs.string())
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s", lstr_errs.string());
}


void
cExtErrLog::close_files()
{
    PL()->TeePrompt(0);
    if (el_logfp) {
        fflush(el_logfp);
        if (el_logfp != stdout)
            fclose(el_logfp);
        el_logfp = 0;
    }
    if (el_errfp) {
        fflush(el_errfp);
        if (el_errfp != stderr)
            fclose(el_errfp);
        el_errfp = 0;
    }
}

