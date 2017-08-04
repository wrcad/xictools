
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
#include "sced.h"
#include "sced_errlog.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "errorlog.h"
#include "promptline.h"
#include "filestat.h"


//
// Logging and error recording for use during connection.
//

cScedErrLog::cScedErrLog()
{
    el_cellname = 0;
    el_logfp = 0;
    el_errfp = 0;
    el_logfile = 0;
    el_errfile = 0;

    el_errcnt = 0;
    el_level = 0;
    el_warned_log = false;
    el_warned_err = false;
    el_log_connect = false;
}


cScedErrLog::~cScedErrLog()
{
    close_files();
    delete [] el_logfile;
    delete [] el_errfile;
}


void
cScedErrLog::start_logging(CDcellName cellname)
{
    el_cellname = cellname;
    if (el_level == 0)
        el_errcnt = 0;
    el_level++;
    open_files();
    if (el_logfp)
        PL()->TeePrompt(el_logfp);
}


void
cScedErrLog::end_logging()
{
    if (el_level)
        el_level--;
    if (el_level == 0) {
        close_files();
        if (el_errcnt > 0 && el_errfile)
            DSPmainWbag(PopUpFileBrowser(el_errfile))
        el_errcnt = 0;
    }
}


void
cScedErrLog::add_log(const char *fmt, ...)
{
    if (!el_logfp)
        return;
    if (!el_log_connect)
        return;

    fprintf(el_logfp, "In %s: ", Tstring(el_cellname));
    va_list args;
    va_start(args, fmt);
    vfprintf(el_logfp, fmt, args);
    va_end(args);
    fputc('\n', el_logfp);
}


void
cScedErrLog::add_err(const char *fmt, ...)
{
    if (!el_errfp)
        return;
    el_errcnt++;
    fprintf(el_errfp, "(%d) In %s: ", el_errcnt, Tstring(el_cellname));

    va_list args;
    va_start(args, fmt);
    vfprintf(el_errfp, fmt, args);
    va_end(args);
    fputc('\n', el_errfp);
}


// Uncomment to always append to an existing file rather than backing
// up the existing file and starting a new one.
#define APPEND_FILES

void
cScedErrLog::open_files()
{
    sLstr lstr_errs;
    if (!el_errfp) {
        sLstr lstr;
        if (Log()->LogDirectory() && *Log()->LogDirectory()) {
            lstr.add(Log()->LogDirectory());
            lstr.add_c('/');
        }
        lstr.add(SC_ERRFILE);

        delete [] el_errfile;
        el_errfile = 0;
#ifdef APPEND_FILES
        el_errfp = fopen(lstr.string(), "a");
        if (el_errfp) {
            fprintf(el_errfp, "# %s  %s\n", SC_ERRFILE, XM()->IdString());
            el_errfile = lstr.string_trim();
        }
#else
        if (filestat::create_bak(lstr.string())) {
            el_errfp = fopen(lstr.string(), "w");
            if (el_errfp) {
                fprintf(el_errfp, "# %s  %s\n", SC_ERRFILE, XM()->IdString());
                el_errfile = lstr.string_trim();
            }
        }
#endif
    }
    if (!el_errfp) {
        if (!el_warned_err) {
            lstr_errs.add("Can't open ");
            lstr_errs.add(SC_ERRFILE);
            lstr_errs.add(", error messages will go to console window.\n");
            lstr_errs.add("Error: ");
            lstr_errs.add(filestat::error_msg());
            lstr_errs.add_c('\n');
            el_warned_err = true;
        }
        el_errfp = stderr;
    }

    if (el_log_connect) {
        if (!el_logfp) {
            sLstr lstr;
            if (Log()->LogDirectory() && *Log()->LogDirectory()) {
                lstr.add(Log()->LogDirectory());
                lstr.add_c('/');
            }
            lstr.add(SC_LOGFILE);

            delete [] el_logfile;
            el_logfile = 0;
#ifdef APPEND_FILES
            el_logfp = fopen(lstr.string(), "a");
            if (el_logfp) {
                fprintf(el_logfp, "# %s  %s\n", SC_LOGFILE,
                    XM()->IdString());
                el_logfile = lstr.string_trim();
            }
#else
            if (filestat::create_bak(lstr.string())) {
                el_logfp = fopen(lstr.string(), "w");
                if (el_logfp) {
                    fprintf(el_logfp, "# %s  %s\n", SC_LOGFILE,
                        XM()->IdString());
                    el_logfile = lstr.string_trim();
                }
            }
#endif
        }
        if (!el_logfp) {
            if (!el_warned_log) {
                lstr_errs.add("Can't open ");
                lstr_errs.add(SC_LOGFILE);
                lstr_errs.add(", no logfile will be generated.\n");
                lstr_errs.add("Error: ");
                lstr_errs.add(filestat::error_msg());
                lstr_errs.add_c('\n');
                el_warned_log = true;
            }
        }
    }

    if (lstr_errs.string())
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s", lstr_errs.string());
}


void
cScedErrLog::close_files()
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

