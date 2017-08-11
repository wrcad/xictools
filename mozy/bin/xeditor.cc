
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
 * xeditor -- Simple text editor.                                         *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include "config.h"
#include "ginterf/graphics.h"
#include "miscutil/pathlist.h"
#include "miscutil/childproc.h"
#include "help/help_defs.h"
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef WIN32
#include "miscutil/msw.h"
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

#ifdef WIN32
// Tell the msw interface that we're Generation 4.
const char *msw::MSWpkgSuffix = "-4";
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
    int  NumArgs;
    char **Args;

    const char *nextarg();
    bool callback(const char*, void*, XEtype);
    char *get_default_path();
    void init_signals();
#ifndef WIN32
    void setup_gtk();
#endif
}


int
main(int argc, char **argv)
{
#ifdef WIN32
    // The following solves a problem: when running from a cygwin/bash
    // window, pressing Ctrl-C always kills the program.  One way to avoid
    // this is to SetConsoleMode(h, 0), but then, who wants a console?
    // Here we avoid the problem by forking a new process that is
    // independent of bash and the console.

    bool winbg = false;
    if (!strcmp(argv[argc-1], "-winbg")) {
        winbg = true;
        argc--;
    }
    if (!winbg) {
        // This is the parent process.  Since we don't have fork(), create
        // a new process with winbg set, disconnected from the controlling
        // terminal.

        char cmdline[256];
        GetModuleFileName(0, cmdline, 256);
        char *cmdstr = GetCommandLine();
        while (*cmdstr && !isspace(*cmdstr))
            cmdstr++;
        sprintf(cmdline + strlen(cmdline), "%s -winbg", cmdstr);

        PROCESS_INFORMATION *info = msw::NewProcess(cmdline,
            DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, false);
        if (!info)
            exit (1);
        CloseHandle(info->hProcess);
        delete info;
        exit (0);
    }
#else
    setup_gtk();
#endif

    if (GRpkgIf()->InitPkg(GR_CONFIG, &argc, argv))
        return (1);
    GRpkgIf()->InitColormap(0, 0, false);

    char *path = get_default_path();
    HLP()->set_path(path, false);
#ifdef WIN32
    HLP()->define("Windows");
#endif
    HLP()->define("Xic");
    HLP()->define("WRspice");

    init_signals();
    NumArgs = argc;
    Args = argv;
    if (NumArgs > 1) {
        Args++;
        NumArgs--;
        GRwbag *cx = GRpkgIf()->NewWbag("xeditor", 0);
        cx->SetCreateTopLevel();
        cx->PopUpTextEditor(*Args, callback, 0, false);
    }
    else {
        GRwbag *cx = GRpkgIf()->NewWbag("xeditor", 0);
        cx->SetCreateTopLevel();
        cx->PopUpTextEditor(0, callback, 0, false);
    }
    GRpkgIf()->MainLoop();
    return (0);
}


// Called on unrecoverable error.
//
void
panic_callback()
{
    fprintf(stderr, "Damn!  Fatal memory error.  Bye!\n");
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}


namespace {
    const char *
    nextarg()
    {
        if (NumArgs > 1) {
            Args++;
            NumArgs--;
            return (*Args);
        }
        return ("");
    }


    // Callback to communicate with the widget.
    //
    bool
    callback(const char *s, void*, XEtype type)
    {
        switch (type) {
        case XE_QUIT:
            // yes, really quit
            signal(SIGINT, SIG_DFL);
            raise(SIGINT);
            break;
        case XE_LOAD:
            // tell the popup which file to load
            // s is not really const in this case
            strcpy((char*)s, nextarg());
            break;
        case XE_HELP:
        default:
            break;
        }
        return (true);
    }


#ifdef WIN32
    // If str contains white space or ':', return a double-quoted copy
    // of str and delete str.
    //
    const char *
    enquote(const char *str)
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


    // Set path to help files
    //
    char *
    get_default_path()
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
            sprintf(buf, "( %s/%s/%s/help )", prefix, TOOLS_ROOT, "wrspice");
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


    // The signal handler
    //
    static void
    sig_hdlr(int sig)
    {
        signal(sig, sig_hdlr);  // reset for SysV

        if (sig == SIGSEGV) {
            fprintf(stderr, "Fatal internal error: segmentation violation.\n");
            signal(SIGINT, SIG_DFL);
            raise(SIGINT);
        }
#ifdef SIGBUS
        else if (sig == SIGBUS) {
            fprintf(stderr, "Fatal internal error: bus error.\n");
            signal(SIGINT, SIG_DFL);
            raise(SIGINT);
        }
#endif
        else if (sig == SIGILL) {
            fprintf(stderr, "Fatal internal error: illegal instruction.\n");
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


    void
    init_signals()
    {
        signal(SIGINT, SIG_IGN);
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


#ifndef WIN32
    // There is a portability problem with the statically-linked gtk when
    // theme engines are imported.  In particular, libpng has very
    // restrictive verson matching, and if an engine (such as eazel) tries
    // to read a png image, and the shared code is compiled against a
    // different libpng release than the one we supply, the application
    // will exit.  Thus, we (by default) turn off themes generally, and
    // supply our own, known to work.
    //
    void
    setup_gtk()
    {
#ifndef DYNAMIC_LIBS
        if (getenv("XT_USE_GTK_THEMES"))
            return;
        char buf[1024];

        // The accessories don't supply our theme, but will attempt to use
        // one installed with Xic or WRspice.

        const char *prefix = getenv("XT_PREFIX");
        if (!prefix || !lstring::is_rooted(prefix))
            prefix = PREFIX;

        sprintf(buf, "%s/%s/%s/startup/default_theme/gtkrc", prefix,
            TOOLS_ROOT, "xic");

        FILE *fp = fopen(buf, "r");
        if (!fp) {
            sprintf(buf, "%s/%s/%s/startup/default_theme/gtkrc", prefix,
                TOOLS_ROOT, "wrspice");
            fp = fopen(buf, "r");
        }
        if (fp) {
            fclose(fp);

            // We're going to check for gtkrc in default_theme and its
            // parent dir, parent dir last so it has precedence.  If the
            // user modifies gtkrc, it should be copied to the parent dir
            // so it won't get clobbered by a software update.

            char *path = buf;
            char *rcp = new char[2*strlen(path) + 2];
            char *e = lstring::stpcpy(rcp, path);
            char *ee = e;
            *e++ = ':';
            strcpy(e, path);
            e = lstring::strrdirsep(ee);
            if (e) {
                char *fn = e + 1;
                *e = 0;
                e = lstring::strrdirsep(ee);
                if (e) {
                    e++;
                    while ((*e = *fn++) != 0)
                        e++;
                }
                else
                    *ee = 0;
            }
            else
                *ee = 0;

            char *v1 = new char[strlen(rcp) + 16];
            sprintf(v1, "%s=%s", "GTK_RC_FILES", rcp);
            putenv(v1);
            delete [] rcp;

            e = lstring::strrdirsep(path);
            if (e)
                *e = 0;
            else
                *path = 0;
            char *v2 = new char[strlen(path) + 18];
            sprintf(v2, "%s=%s", "GTK_EXE_PREFIX", path);
            putenv(v2);
        }
        else
            // Don't read any gtkrc files, so no themes
            putenv(lstring::copy("GTK_RC_FILES="));
#endif
    }
#endif
}

