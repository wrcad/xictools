
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
#include "fio.h"
#include "oa_if.h"
#include "oa.h"
#include "oa_errlog.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "errorlog.h"
#include "promptline.h"
#include "miscutil/filestat.h"


//
// Logging and error recording for use during connection.
//

const char *
cOA::set_debug_flags(const char *on, const char *off)
{
    static char buf[64];
    if (on) {
        for (const char *s = on; *s; s++) {
            if (*s == 'l' || *s == 'L')
                OAerrLog.set_debug_load(true);
            else if (*s == 'p' || *s == 'P')
                OAerrLog.set_debug_pcell(true);
            else if (*s == 'n' || *s == 'N')
                OAerrLog.set_debug_net(true);
        }
    }
    if (off) {
        for (const char *s = off; *s; s++) {
            if (*s == 'l' || *s == 'L')
                OAerrLog.set_debug_load(false);
            else if (*s == 'p' || *s == 'P')
                OAerrLog.set_debug_pcell(false);
            else if (*s == 'n' || *s == 'N')
                OAerrLog.set_debug_net(false);
        }
    }
    snprintf(buf, sizeof(buf), "load=%d net=%d pcell=%d",
        OAerrLog.debug_load(), OAerrLog.debug_net(), OAerrLog.debug_pcell());
    return (buf);
}
// End of cOA functions.


cOAerrLog::cOAerrLog()
{
    el_name = 0;
    el_logfp = 0;
    el_errfp = 0;
    el_logfile = 0;
    el_errfile = 0;
    el_names = 0;

    el_errcnt = 0;
    el_level = 0;
    el_warned_log = false;
    el_warned_err = false;
    el_syserr = false;
    el_return = 0;
    el_dbg_load = false;
    el_dbg_net = false;
    el_dbg_pcell = false;
}


cOAerrLog::~cOAerrLog()
{
    close_files();
    delete [] el_logfile;
    delete [] el_errfile;
    stringlist::destroy(el_names);
}


// Push a logging context.
// name:  Cell name or section name, if not null messages will have
//        '"in %s: ", name'  prepended.
// errs:  Name of errors file to open in logfiles area.  This can also
//        be "stdout" or "stderr".  If null, errors are sent to the
//        main system logger.
// log;   Name of log file to open in logfiles area.  This can also be
//        "stdout" or "stderr".  In null or empty, no logging.
//
// The errs and log arguments are ignored except at the top level.
//
void
cOAerrLog::start_logging(const char *name, const char *errs, const char *log)
{
    el_names = new stringlist(el_name, el_names);
    el_name = lstring::copy(name);
    if (el_level == 0) {
        el_errcnt = 0;
    }
    else {
        errs = 0;
        log = 0;
    }
    el_level++;
    open_files(errs, log);
    if (el_logfp)
        PL()->TeePrompt(el_logfp);
}


// Pop to previous logging context.
//
void
cOAerrLog::end_logging()
{
    if (el_level)
        el_level--;
    if (el_names) {
        delete [] el_name;
        el_name = el_names->string;
        stringlist *t = el_names;
        el_names = el_names->next;
        t->string = 0;
        delete t;
    }
    if (el_level == 0) {
        close_files();
        if (el_errcnt > 0 && el_errfile)
            DSPmainWbag(PopUpFileBrowser(el_errfile))
        el_errcnt = 0;
    }
}


void
cOAerrLog::add_log(OAlogType type, const char *fmt, ...)
{
    if (!el_logfp)
        return;
    if (type == OAlogLoad && !el_dbg_load)
        return;
    if (type == OAlogNet && !el_dbg_net)
        return;
    if (type == OAlogPCell && !el_dbg_pcell)
        return;

    fprintf(el_logfp, "In %s: ", el_name);
    va_list args;
    va_start(args, fmt);
    vfprintf(el_logfp, fmt, args);
    va_end(args);
    fputc('\n', el_logfp);
}


void
cOAerrLog::add_err(int err_type, const char *fmt, ...)
{
    if (!el_errfp)
        return;
    if (el_syserr) {
        if (err_type == IFLOG_WARN || err_type == IFLOG_FATAL)
            el_errcnt++;
        char buf[2048];
        snprintf(buf, sizeof(buf), "In %s: ", el_name);
        int n = strlen(buf);
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf + n, 2048 - n, fmt, args);
        FIO()->ifPrintCvLog((OUTmsgType)err_type, buf);
    }
    else {
        if (err_type == IFLOG_WARN || err_type == IFLOG_FATAL) {
            el_errcnt++;
            fprintf(el_errfp, "(%d) In %s: ", el_errcnt, el_name);
        }
        else
            fprintf(el_errfp, "(INFO) In %s: ", el_name);

        va_list args;
        va_start(args, fmt);
        vfprintf(el_errfp, fmt, args);
        va_end(args);
        fputc('\n', el_errfp);
    }
}


void
cOAerrLog::set_show_log(bool show)
{
    if (el_syserr || el_errfp == Cvt()->LogFp())
        Cvt()->SetShowLog(show);
}


void
cOAerrLog::open_files(const char *errs, const char *log)
{
    sLstr lstr_errs;
    if (errs) {
        // If true, we are already reading a hierarchy, direct
        // messages to the existing file.
        if (!el_errfp)
            el_errfp = Cvt()->LogFp();

        if (!el_errfp) {
            if (!strcmp(errs, "stdout"))
                el_errfp = stdout;
            else if (!strcmp(errs, "stderr"))
                el_errfp = stderr;
            else if (!strcmp(errs, "system")) {
                el_errfp = FIO()->ifInitCvLog(OA_READLOG);
                CD()->SetReading(true);
                el_syserr = true;
            }
            else {
                sLstr lstr;
                if (Log()->LogDirectory() && *Log()->LogDirectory()) {
                    lstr.add(Log()->LogDirectory());
                    lstr.add_c('/');
                }
                lstr.add(errs);

                delete [] el_errfile;
                el_errfile = 0;
                if (filestat::create_bak(lstr.string())) {
                    el_errfp = fopen(lstr.string(), "w");
                    if (el_errfp) {
                        fprintf(el_errfp, "# %s  %s\n", errs,
                            XM()->IdString());
                        el_errfile = lstr.string_trim();
                    }
                }
            }
        }
        if (!el_errfp) {
            if (!el_warned_err) {
                lstr_errs.add("Can't open ");
                lstr_errs.add(errs);
                lstr_errs.add(", error messages will go to console window.\n");
                lstr_errs.add("Error: ");
                lstr_errs.add(filestat::error_msg());
                lstr_errs.add_c('\n');
                el_warned_err = true;
            }
            el_errfp = stderr;
        }
    }
    if (log) {
        if (!el_logfp) {
            if (!strcmp(log, "stdout"))
                el_logfp = stdout;
            else if (!strcmp(log, "stderr"))
                el_logfp = stderr;
            else {
                sLstr lstr;
                if (Log()->LogDirectory() && *Log()->LogDirectory()) {
                    lstr.add(Log()->LogDirectory());
                    lstr.add_c('/');
                }
                lstr.add(log);

                delete [] el_logfile;
                el_logfile = 0;
                if (filestat::create_bak(lstr.string())) {
                    el_logfp = fopen(lstr.string(), "w");
                    if (el_logfp) {
                        fprintf(el_logfp, "# %s  %s\n", log,
                            XM()->IdString());
                        el_logfile = lstr.string_trim();
                    }
                }
            }
        }
        if (!el_logfp) {
            if (!el_warned_log) {
                lstr_errs.add("Can't open ");
                lstr_errs.add(log);
                lstr_errs.add(", no logfile will be generated.\n");
                lstr_errs.add("Error: ");
                lstr_errs.add(filestat::error_msg());
                lstr_errs.add_c('\n');
                el_warned_log = true;
            }
        }
    }
    if (lstr_errs.string())
        DSPpkg::self()->ErrPrintf(ET_ERROR, "%s", lstr_errs.string());
}


void
cOAerrLog::close_files()
{
    PL()->TeePrompt(0);
    if (el_logfp) {
        fflush(el_logfp);
        if (el_logfp != stdout && el_logfp != stderr)
            fclose(el_logfp);
        el_logfp = 0;
    }
    if (el_errfp) {
        fflush(el_errfp);
        if (!el_syserr && el_errfp != stdout && el_errfp != stderr &&
                el_errfp != Cvt()->LogFp())
            fclose(el_errfp);
        if (el_syserr) {
            FIO()->ifShowCvLog(el_errfp, (OItype)el_return, OA_READLOG);
            CD()->SetReading(false);
            el_syserr = false;
        }
        el_errfp = 0;
    }
}

