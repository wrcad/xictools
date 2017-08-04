
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
#include "errorlog.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "filestat.h"
#include "pathlist.h"
#include "miscutil.h"

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>


//
// Error Logging
//

// Message header exports.
namespace mh {
    const char *DRC             = "DRC";
    const char *DRCViolation    = "DRC violation";
    const char *ObjectCreation  = "object creation";
    const char *EditOperation   = "edit operation";
    const char *Flatten         = "flatten";
    const char *CellPlacement   = "cell placement";
    const char *Techfile        = "techfile";
    const char *Initialization  = "initialization";
    const char *Internal        = "internal";
    const char *NetlistCreation = "netlist creation";
    const char *Variables       = "variables";
    const char *PCells          = "pcells";
    const char *Properties      = "properties";
    const char *JobControl      = "job control";
    const char *Processing      = "processing";
    const char *OpenAccess      = "OpenAccess";
    const char *InputOutput     = "Input/Output";
}

// We keep a list of the last KEEPMSGS-1 error messages in memory, all are
// written to an errors file.  Each error is given an error count.

#define KEEPMSGS 21

cErrLog *cErrLog::instancePtr = 0;

cErrLog::cErrLog()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cErrLog already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    el_log_directory = 0;
    el_mem_err_log_name = 0;
    el_mail_address = 0;

    setupInterface();
}


// Private static error exit.
//
void
cErrLog::on_null_ptr()
{
    fprintf(stderr, "Singleton class cErrLog used before instantiated.\n");
    exit(1);
}


//-----------------------------------------------------------------------
// The error/warning display functions follow.  Each will pop up a
// window, or use an existing error window if already present, and
// display a message.  The ErrorLog and similar functions assign an
// error number and record the message in a list, which is saved in
// the error log file.  The function will display the current error,
// along with the last few errors.  The PopUpErr and similar functions
// do not record the message, but simply display the text.
//
// Most errors and warnings should use the ErrorLog group of
// functions, as this avoids a problem with PopUpErr:  The messages
// will be overwritten by subsequent errors/warnings of any type, and
// may therefor be unseen.  The logging functions will always record
// the error in the log, so it won't be lost.
//
// We use the PopUpErrs functions only for very simple messages in
// commands, which are probably instructional, and occur just before a
// natural pause so that the message will be displayed immediately and
// not overwritten.
//-----------------------------------------------------------------------

// Pop up the top of the error list with the new errstr added.
//
void
cErrLog::ErrorLog(const char *header, const char *errstr)
{
    el_list.add_msg(false, header, errstr, LW_CENTER);
}


// Formatting version of above, note that literal '%' characters cause
// trouble so this function is not a potential replacement for the
// string version above.
//
void
cErrLog::ErrorLogV(const char *header, const char *fmt, ...)
{
    if (!fmt || !*fmt)
        return;
    int len = strlen(fmt) + 1024;
    char *bf = new char[len];
    va_list args;
    va_start(args, fmt);
    vsnprintf(bf, len, fmt, args);
    va_end(args);
    el_list.add_msg(false, header, bf, LW_CENTER);
    delete [] bf;
}


void
cErrLog::WarningLog(const char *header, const char *errstr)
{
    el_list.add_msg(true, header, errstr, LW_CENTER);
}


void
cErrLog::WarningLogV(const char *header, const char *fmt, ...)
{
    if (!fmt || !*fmt)
        return;
    int len = strlen(fmt) + 1024;
    char *bf = new char[len];
    va_list args;
    va_start(args, fmt);
    vsnprintf(bf, len, fmt, args);
    va_end(args);
    el_list.add_msg(true, header, bf, LW_LR);
    delete [] bf;
}


// Print the string in an error pop-up.
//
void
cErrLog::PopUpErr(const char *string)
{
    if (!string || !*string)
        return;
    if (XM()->RunMode() == ModeNormal)
        DSPmainWbag(PopUpErr(MODE_ON, string, STY_NORM))
    else
        fputs(string, stderr);
}


// Print the string in an error pop-up, with some extra options.
//
void
cErrLog::PopUpErrEx(const char *string, bool sty, int lw_code)
{
    if (!string || !*string)
        return;
    if (XM()->RunMode() == ModeNormal) {
        if (lw_code < 0)
            // use default placement
            DSPmainWbag(PopUpErr(MODE_ON, string,
                sty ? STY_FIXED : STY_NORM))
        else {
            // assigned placement
            GRloc loc((LWenum)lw_code);
            DSPmainWbag(PopUpErr(MODE_ON, string,
                sty ? STY_FIXED : STY_NORM, loc))
        }
    }
    else
        fputs(string, stderr);
}


// Format and print a message in an error pop-up.  This is a separate
// function from above, since literal '%' characters are a problem.
//
void
cErrLog::PopUpErrV(const char *fmt, ...)
{
    if (!fmt || !*fmt)
        return;
    int len = strlen(fmt) + 1024;
    char *bf = new char[len];
    va_list args;
    va_start(args, fmt);
    vsnprintf(bf, len, fmt, args);
    va_end(args);
    if (XM()->RunMode() == ModeNormal)
        DSPmainWbag(PopUpErr(MODE_ON, bf))
    else
        fputs(bf, stderr);
    delete [] bf;
}


void
cErrLog::PopUpWarn(const char *string)
{
    if (!string || !*string)
        return;
    if (XM()->RunMode() == ModeNormal)
        DSPmainWbag(PopUpWarn(MODE_ON, string))
    else
        fputs(string, stderr);
}


// Print the string in a warning pop-up.
//
void
cErrLog::PopUpWarnEx(const char *string, bool sty, int lw_code)
{
    if (!string || !*string)
        return;
    if (XM()->RunMode() == ModeNormal) {
        if (lw_code < 0)
            // use default placement
            DSPmainWbag(PopUpWarn(MODE_ON, string,
                sty ? STY_FIXED : STY_NORM))
        else {
            // assigned placement
            GRloc loc((LWenum)lw_code);
            DSPmainWbag(PopUpWarn(MODE_ON, string,
                sty ? STY_FIXED : STY_NORM, loc))
        }
    }
    else
        fputs(string, stderr);
}


// Format and print a message in an error pop-up.  This is a separate
// function from above, since literal '%' characters are a problem
//
void
cErrLog::PopUpWarnV(const char *fmt, ...)
{
    if (!fmt || !*fmt)
        return;
    int len = strlen(fmt) + 1024;
    char *bf = new char[len];
    va_list args;
    va_start(args, fmt);
    vsnprintf(bf, len, fmt, args);
    va_end(args);
    if (XM()->RunMode() == ModeNormal)
        DSPmainWbag(PopUpWarn(MODE_ON, bf))
    else
        fputs(bf, stderr);
    delete [] bf;
}


// Open a directory for the log files, create if necessary.
//
bool
cErrLog::OpenLogDir(const char *app_root)
{
    const char *path  = getenv("XIC_LOG_DIR");
    if (!path || !*path)
        path = getenv("XIC_TMP_DIR");
    if (!path || !*path)
        path = getenv("TMPDIR");
    if (!path || !*path)
        path = "/tmp";
    path = pathlist::expand_path(path, true, true);

#ifdef WIN32
    if (lstring::is_dirsep(path[0]) && !lstring::is_dirsep(path[1])) {
        // Add a drive specifier.  If the CWD is a share, use "C:".
        char *cwd = getcwd(0, 0);
        if (!isalpha(cwd[0]) || cwd[1] != ':') {
            cwd[0] = 'c';
            cwd[1] = ':';
        }
        cwd[2] = 0;
        char *t = new char[strlen(path) + 4];
        strcpy(t, cwd);
        free(cwd);
        strcpy(t+2, path);
        delete [] path;
        path = t;
    }

    // In Windows, create the directory.
    mkdir(path);
#endif

    char buf[64];
    if (!app_root)
        app_root = "errlog";
    strcpy(buf, app_root);
    char *e = strrchr(buf, '.');
    // Strip exec suffix, if any.
    if (e && lstring::cieq(e, ".exe"))
        *e = 0;
    e = buf + strlen(buf);
    sprintf(e, ".%d", (int)getpid());
    char *logdir = pathlist::mk_path(path, buf);
#ifdef WIN32
    if (mkdir(logdir) && errno != EEXIST) {
#else
    if (mkdir(logdir, 0755) && errno != EEXIST) {
#endif
        delete [] logdir;
        fprintf(stderr,
    "Error: Could not create directory in %s.\n"
    "I need to create a directory in /tmp, or in a directory given by\n"
    "any of the environment variables XIC_LOG_DIR, XIC_TMP_DIR, TMPDIR.\n"
    "This directory must exist and have write permission for the user.\n",
            path);
        delete [] path;
        return (false);
    }
    delete [] path;

    el_log_directory = logdir;

    if (el_list.log_filename()) {
        // Export the log file path to the graphics system.  A button
        // will appear in error windows which will pop up the file
        // viewer loaded with this file.

        path = pathlist::mk_path(el_log_directory, el_list.log_filename());
        DSPmainWbag(SetErrorLogName(path));
        delete [] path;
    }
    return (true);
}


// Return a list of the files in the log directory.
//
stringlist *
cErrLog::ListLogDir()
{
    if (!el_log_directory)
        return (0);
    stringlist *s0 = 0;
    DIR *wdir = opendir(el_log_directory);
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
        stringlist::sort(s0);
    }
    return (s0);
}


// Remove the log directory, done on normal exit.
//
void
cErrLog::CloseLogDir()
{
    if (!el_log_directory)
        return;

    // Mail the memory errors file, if it exists.
    if (el_mail_address && el_mem_err_log_name) {
        char *path = pathlist::mk_path(el_log_directory, el_mem_err_log_name);
        FILE *fp = fopen(path, "r");
        if (fp) {
            sLstr lstr;
            char buf[256];
            while (fgets(buf, 256, fp) != 0)
                lstr.add(buf);
            fclose(fp);

            if (!getenv("XICNOMAIL") && !getenv("XTNOMAIL")) {
                miscutil::send_mail(el_mail_address, "XicMemoryErrors",
                    lstr.string_trim());
            }
            else {
                fp = fopen(el_mem_err_log_name, "w");
                if (fp)
                    fputs(lstr.string_trim(), fp);
                fclose(fp);
                fprintf(stderr,
                    "Memory errors were detected. Please email %s to\n%s.\n",
                    el_mem_err_log_name, el_mail_address);
            }
        }
        delete [] path;
    }

    if (XM()->RunMode() == ModeServer) {
        // All descriptors to the stdout log must be closed, or the
        // log file won't be unlinked before the rmdir call in some
        // cases, e.g., when the directory is on an NFS share under
        // Linux.  This causes rmdir to fail and the empty directory
        // will remain.
        //
        close(1);
        close(2);
    }
    DIR *wdir = opendir(el_log_directory);
    if (wdir) {
        char *path = new char[strlen(el_log_directory) + 256];
        strcpy(path, el_log_directory);
        char *t = path + strlen(path) - 1;
        if (!lstring::is_dirsep(*t)) {
            t++;
            *t++ = '/';
            *t = 0;
        }
        else
            t++;
        struct dirent *de;
        while ((de = readdir(wdir)) != 0) {
            if (!strcmp(de->d_name, "."))
                continue;
            if (!strcmp(de->d_name, ".."))
                continue;
            strcpy(t, de->d_name);
            unlink(path);
        }
        closedir(wdir);
        delete [] path;
    }
    rmdir(el_log_directory);
}


// Special fopen() to open a log file in the log directory.
//
FILE *
cErrLog::OpenLog(const char *name, const char *mode, bool sizetest)
{
    if (!name || !mode)
        return (0);
    char *path;
    if (el_log_directory && *el_log_directory)
        path = pathlist::mk_path(el_log_directory, name);
    else
        path = lstring::copy(name);

    if (*mode == 'a' && sizetest) {
        // make sure file doesn't grow too large
        struct stat st;
        if (stat(path, &st) == 0 && st.st_size > 100000) {
            char *bak = new char[strlen(path) + 3];
            strcpy(bak, path);
            strcat(bak, ".0");
            if (!filestat::move_file_local(bak, path))
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
            delete [] bak;
            FILE *fp = fopen(path, "w");
            if (fp)
                fputs("# size limit exceeded, file rollover\n", fp);
            delete [] path;
            return (fp);
        }
    }
    if (*mode == 'w') {
        if (!filestat::create_bak(path)) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
            return (0);
        }
        char *bak = new char[strlen(path) + 3];
        strcpy(bak, path);
        strcat(bak, ".0");
        if (!access(bak, F_OK)) {
            if (!filestat::create_bak(bak))
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
            unlink(bak);
        }
        delete [] bak;
    }
    FILE *fp = fopen(path, mode);
    delete [] path;
    return (fp);
}
// End of cErrLog functions.


void
sMsgList::set_filename(const char *fname)
{
    delete [] ml_log_filename;
    ml_log_filename = lstring::copy(fname);
}


// Return a *copy* of the message corresponding to num, or 0 if out of
// range.  The range is the current error number and the positive
// KEEPMSGS below that.
//
char *
sMsgList::get_msg(int num)
{
    if (!ml_msg_list)
        return (0);
    char *s = ml_msg_list->string;
    int curnum;
    if (sscanf(s, "(%d)", &curnum) != 1)
        // List corrupted!
        return (0);
    if (num > curnum || num <= 0 || num <= curnum - KEEPMSGS + 1)
        return (0);
    for (stringlist *sl = ml_msg_list; sl; sl = sl->next) {
        if (num == curnum)
            return (lstring::copy(sl->string));
        curnum--;
    }
    return (0);
}


// Add msgstr to a running list of error messages, and pop up the
// message list (scroll bars come automatically if needed).
//
// The posn is an LWenum value (graphics.h).
//
void
sMsgList::add_msg(bool warn, const char *header, const char *msgstr,
    int posn)
{
    if (!msgstr || !*msgstr)
        return;
    if (!ml_log_filename) {
        fprintf(stderr, "%s: %s\n", warn ? "Warning" : "Error", msgstr);
        return;
    }

    ml_msg_count++;
    char buf[256];
    FILE *fp = Log()->OpenLog(ml_log_filename, ml_msg_count == 1 ? "w" : "a",
        true);
    if (!fp && ml_msg_count == 1) {
        sprintf(buf,
            "(%d) Warning [initialization]\n"
            "Can't open %s file, errors and warningss won't be logged.",
            ml_msg_count, ml_log_filename);
        ml_msg_list = new stringlist(lstring::copy(buf), ml_msg_list);
        ml_msg_count++;
    }

    int hlen = 24 + (header ? strlen(header) : 0);
    char *mbuf = new char[strlen(msgstr) + hlen];
    if (header)
        sprintf(mbuf, "(%d) %s [%s]\n%s", ml_msg_count,
            warn ? "Warning" : "Error", header, msgstr);
    else
        sprintf(mbuf, "(%d) %s\n%s", ml_msg_count,
            warn ? "Warning" : "Error", msgstr);
    char *t = mbuf + strlen(mbuf) - 1;
    if (*t == '\n')
        *t = 0;
    ml_msg_list = new stringlist(mbuf, ml_msg_list);

    const char *sepstring = "----------------------------\n";
    if (fp) {
        if (ml_msg_count == 1)
            fprintf(fp, "# %s\n", XM()->IdString());
        else
            fputs(sepstring, fp);
        fprintf(fp, "%s\n", mbuf);
    }
    int cnt = 0;
    for (stringlist *sl = ml_msg_list; sl; sl = sl->next) {
        cnt++;
        if (cnt == KEEPMSGS) {
            delete [] sl->string;
            if (fp)
                sprintf(buf, "See %s file.", ml_log_filename);
            else
                strcpy(buf, "No logfile, message list truncated.");
            sl->string = lstring::copy(buf);
            stringlist::destroy(sl->next);
            sl->next = 0;
            break;
        }
    }
    if (fp)
        fclose(fp);

    sLstr lstr;
    for (stringlist *sl = ml_msg_list; sl; sl = sl->next) {
        lstr.add(sl->string);
        lstr.add_c('\n');
        if (sl->next)
            lstr.add(sepstring);
    }
    if (DSP()->MainWdesc() && DSP()->MainWdesc()->Wbag())
        Log()->PopUpErrEx(lstr.string(), false, posn);
    else
        fprintf(stderr, "%s: %s\n", warn ? "Warning" : "Error", msgstr);
}

