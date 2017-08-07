
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
 * mozy -- Stand-alone help/www browser                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "ginterf/graphics.h"
#include "miscutil/pathlist.h"
#include "miscutil/childproc.h"
#include "miscutil/filestat.h"
#include "help/help_defs.h"
#include "help/help_context.h"

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef WIN32
#include "miscutil/msw.h"
#else
#include <pwd.h>
#endif


#ifndef DEF_TOPIC
#define DEF_TOPIC "mozy"
#endif
#ifndef PREFIX
#define PREFIX "/usr/local"
#endif
#ifndef TOOLS_ROOT
#define TOOLS_ROOT "xictools"
#endif
#ifndef APP_ROOT
#define APP_ROOT "mozy"
#endif
#ifndef VERSION_STR
#define VERSION_STR "unknown"
#endif

#ifndef HAVE_STRERROR
#ifndef SYS_ERRLIST_DEF
extern char *sys_errlist[];
#endif
#endif


// Set up the ginterf package, support for all devices, no hardcopy
// drivers.
//
#define GR_CONFIG (GR_ALL_PKGS)
#include "ginterf/gr_pkg_setup.h"

namespace {
    // Create the graphics package.
    GRpkg _gp_;
    GRappCallStubs _calls_;

    char *get_default_path(bool, bool);
    void quit_proc(void*);
    void init_signals();
}


#ifdef WIN32

// Tell the msw interface that we're Generation 4.
const char *msw::MSWpkgSuffix = "-4";

#else

#define MOZY_FIFO "mozyfifo"

namespace {
    // If there is nothing to read on stdin, return false.  Otherwise
    // redirect this to the named pipe, if it exists, and return true. 
    // If the named pipe does not exist, start up and create the pipe.
    // The stdin goes to a temp file, which will be shown.
    //
    // This is all to enable piping html into mozy for view, from
    // something like the mutt mail client.
    //
    bool check_pipe(const char *cmd)
    {
        static const char *tempfile;

        // Look and see if there is standard input pending.  The stdin
        // file descriptor is 0.
        for (;;) {
            int ifd = fileno(stdin);
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(ifd, &readfds);
            timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 500;
            int i = select(1, &readfds, 0, 0, &timeout);
            if (i < 0) {
                // interrupted
                continue;
            }
            if (i == 0) {
                // nothing to read
                return (false);
            }
            break;
        }

        // Get the default name of the FIFO.
        const char *fname = getenv("MOZY_FIFO");
        if (!fname)
            fname = MOZY_FIFO;
        sLstr lstr; 
        passwd *pw = getpwuid(getuid());
        if (pw == 0)
            GRpkgIf()->Perror("getpwuid");
        else {
            lstr.add(pw->pw_dir);
            lstr.add_c('/');
        }
        lstr.add(fname);
        fname = lstr.string();

        // Read stdin into a temporary file.
        size_t fs = 0;
        if (!tempfile)
            tempfile = filestat::make_temp("moz");
        FILE *fp = fopen(tempfile, "w");
        if (fp) {
            filestat::queue_deletion(tempfile);
            int c;
            while ((c = getc(stdin)) != EOF) {
                putc(c, fp);
                fs++;
            }
            fclose(fp);
        }
        if (!fs) {
            // Stdin was really empty, bail.
            return (false);
        }

        // See if the FIFO exists, if not spawn another copy of mozy
        // with the "--p" option given, a special option for this
        // purpose.
        struct stat st;
        if (stat(fname, &st) < 0) {
            if (errno == ENOENT) {
                // Not found, run 'argv[0] --p &'.
                sLstr tlstr;
                tlstr.add(cmd);
                tlstr.add(" --p &");
                system(tlstr.string());

                // Wait until the FIFO appears.
                int cnt = 0;
                while (stat(fname, &st) < 0) {
                    timespec ts;
                    ts.tv_sec = 0;
                    ts.tv_nsec = 100000000;
                    nanosleep(&ts, 0);
                    if (cnt == 10)
                        break;
                    cnt++;
                }
                if (cnt == 10) {
                    // The FIFO never appeared in the time given.
                    return (true);
                }

                // Wait some more, otherwise images can get lost or
                // corrupted for some reason.
                timespec ts;
                ts.tv_sec = 0;
                ts.tv_nsec = 500000000;
                nanosleep(&ts, 0);
            }
            else {
                // Some other error.
                return (true);
            }
        }
        else if (!(st.st_mode & S_IFIFO)) {
            // The named pipe file exists, but is not a FIFO!
            return (true);
        }

        // Copy the temporary file into the FIFO.
        fp = fopen(fname, "w");
        if (fp) {
            int c;
            FILE *ip = fopen(tempfile, "r");
            if (ip) {
                while ((c = getc(ip)) != EOF)
                    putc(c, fp);
                fclose(ip);
            }
            fclose (fp);
        }
        return (true);
    }
}

#endif


int main(int argc, char **argv)
{
    bool do_xic = false;
    bool do_wrs = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--xic")) {
            do_xic = true;
            argc--;
            for (int j = i; j < argc; j++)
                argv[j] = argv[j+1];
            i--;
            continue;
        }
        if (!strcmp(argv[i], "--wrs") || !strcmp(argv[i], "--wrspice")) {
            do_wrs = true;
            argc--;
            for (int j = i; j < argc; j++)
                argv[j] = argv[j+1];
            i--;
            continue;
        }
    }

    // If argument "--p" is given, ignore any other arguments and start
    // with the fifo listening.
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--p")) {
            HLP()->set_fifo_start(true);
            argv[1] = lstring::copy(".");
            argc = 2;
        }
    }

#ifndef WIN32
    if (!HLP()->fifo_start()) {
        if (check_pipe(argv[0]))
            return (0);
    }
#endif

    if (GRpkgIf()->InitPkg(GR_CONFIG, &argc, argv))
        return (1);
    GRpkgIf()->InitColormap(0, 0, false);

    char *path = get_default_path(do_xic, do_wrs);
    HLP()->set_path(path, false);
#ifdef WIN32
    HLP()->define("Windows");
#endif
    if (do_xic)
        HLP()->define("Xic");
    if (do_wrs)
        HLP()->define("WRspice");
    HLP()->define("Mozy");
    HLP()->set_name("mozy-" VERSION_STR);

    HLP()->context()->registerQuitHelpProc(quit_proc);
    init_signals();

#ifdef WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        MessageBox(0,
            "Windows Socket Architecture failed to initialize",
            "ERROR", MB_ICONSTOP);
        return (EXIT_FAILURE);
    }
#endif

    const char *url = 0;
    if (argc > 1) {
        if (!access(argv[1], R_OK)) {
            // Argument is a local file, pass the full path.
            if (!lstring::is_rooted(argv[1])) {
                char *cwd = getcwd(0, 0);
                if (cwd) {
                    char *t = new char[strlen(cwd) + strlen(argv[1]) + 2];
                    sprintf(t, "%s/%s", cwd, argv[1]);
                    delete [] cwd;
                    url = t;
                }
            }
        }
        if (!url)
            url = argv[1];
    }
    else {
        char *def_topic = getenv("MOZY_DEF_TOPIC");
        if (def_topic && *def_topic)
            url = def_topic;
        else
            url = DEF_TOPIC;
    }

    HLP()->word(url);
    char *err = 0;
    if (HLP()->error_msg())
        err = lstring::copy(HLP()->error_msg());
    if (err) {
        HLP()->word(".");
        if (HLP()->error_msg()) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s", HLP()->error_msg());
            exit (1);
        }
        GRpkgIf()->ErrPrintf(ET_WARN, "%s", err);
    }

    GRpkgIf()->MainLoop();
    return (0);
}


// Called on unrecoverable error.
//
void panic_callback()
{
    fprintf(stderr, "Damn!  Fatal memory error.  Bye!\n");
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}


namespace {
#ifdef WIN32
    // If str contains white space or ':', return a double-quoted copy
    // of str and delete str.
    //
    const char *enquote(const char *str)
    {
        bool needq = false;
        for (const char *s = str; *s; s++) {
            if (isspace(*s) || *s == ':') {
                needq = true;
                break;
            }
        }
        if (needq) {
            char *s = new char[strlen(str) + 3];
            s[0] = '"';
            char *t = lstring::stpcpy(s+1, str);
            *t++ = '"';
            *t = 0;
            delete [] str;
            return (s);
        }
        return (str);
    }
#endif


    // Set path to help files.
    //
    char *get_default_path(bool do_xic, bool do_wrs)
    {
        const char *prefix = getenv("XT_PREFIX");
        if (!prefix || !lstring::is_rooted(prefix))
            prefix = PREFIX;

        char buf[2048];
        sLstr lstr;
        lstr.add_c('(');

        const char *string = getenv("MOZY_HLP_PATH");
#ifdef WIN32
        if (!string) {  
            string = msw::GetProgramRoot("Mozy");
            if (string) {
                string = enquote(string);
                sprintf(buf, "%s/help", string);
                delete [] string;
                string = buf;
            }
        }
#endif
        if (!string) {
            sprintf(buf, "%s/%s/%s/help", prefix, TOOLS_ROOT, APP_ROOT);
            string = buf;
        }
        if (string && *string) {
            pathgen pg(string);
            char *dir;
            while ((dir = pg.nextpath(false)) != 0) {
                if (!access(dir, F_OK)) {
                    lstr.add_c(' ');
                    lstr.add(dir);
                    delete [] dir;
                }
            }
        }

        if (do_xic) {
            string = getenv("XIC_HLP_PATH");
#ifdef WIN32
            if (!string) {  
                string = msw::GetProgramRoot("Xic");
                if (string) {
                    string = enquote(string);
                    sprintf(buf, "%s/help", string);
                    delete [] string;
                    string = buf;
                }
            }
#endif
            if (!string) {
                sprintf(buf, "%s/%s/%s/help", prefix, TOOLS_ROOT, "xic");
                string = buf;
            }   
            if (string && *string) {
                pathgen pg(string);
                char *dir;
                while ((dir = pg.nextpath(false)) != 0) {
                    if (!access(dir, F_OK)) {
                        lstr.add_c(' ');
                        lstr.add(dir);
                        delete [] dir;
                    }
                }
            }
        }

        if (do_wrs) {
            string = getenv("SPICE_HLP_PATH");
#ifdef WIN32
            if (!string) {  
                string = msw::GetProgramRoot("WRspice");
                if (string) {
                    string = enquote(string);
                    sprintf(buf, "%s/help", string);
                    delete [] string;
                    string = buf;
                }
            }
#endif
            if (!string) {
                sprintf(buf, "%s/%s/%s/help", prefix, TOOLS_ROOT, "wrspice");
                string = buf;
            }   
            if (string && *string) {
                pathgen pg(string);
                char *dir;
                while ((dir = pg.nextpath(false)) != 0) {
                    if (!access(dir, F_OK)) {
                        lstr.add_c(' ');
                        lstr.add(dir);
                        delete [] dir;
                    }
                }
            }
        }

        lstr.add(" )");

        char *path = lstr.string_trim();
#ifdef WIN32
        // Prepend the installation drive to rooted path components
        // without one.
        //
        path = msw::AddPathDrives(path, "XicTools accessories");
#endif
        return (path);
    }


    void quit_proc(void*)
    {
        exit(0);
    }


    // The signal handler.
    //
    void sig_hdlr(int sig)
    {
        signal(sig, sig_hdlr);  // reset for SysV

        if (sig == SIGINT)
            return;

        if (sig == SIGTERM)
            exit(127);

        if (sig == SIGSEGV) {
            fprintf(stderr, "Fatal internal error: segmentation violation.\n");
            filestat::delete_files();
            signal(SIGINT, SIG_DFL);
            raise(SIGINT);
        }
#ifdef SIGBUS
        else if (sig == SIGBUS) {
            fprintf(stderr, "Fatal internal error: bus error.\n");
            filestat::delete_files();
            signal(SIGINT, SIG_DFL);
            raise(SIGINT);
        }
#endif
        else if (sig == SIGILL) {
            fprintf(stderr, "Fatal internal error: illegal instruction.\n");
            filestat::delete_files();
            signal(SIGINT, SIG_DFL);
            raise(SIGINT);
        }
        else if (sig == SIGFPE) {
            fprintf(stderr, "Warning: floating point exception.\n");
            return;
        }
#ifdef SIGCHLD
        else if (sig == SIGCHLD)
            Proc()->SigchldHandler();
#endif
    }


    void init_signals()
    {
        signal(SIGINT, sig_hdlr);
        signal(SIGTERM, sig_hdlr);
        signal(SIGSEGV, sig_hdlr);
#ifdef SIGBUS
        signal(SIGBUS, sig_hdlr);
#endif
        signal(SIGILL, sig_hdlr);
        signal(SIGFPE, sig_hdlr);
#ifdef SIGCHLD
        signal(SIGCHLD, sig_hdlr);
#endif
    }
}

