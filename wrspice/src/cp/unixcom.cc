
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "config.h"
#include "cshell.h"
#include "commands.h"
#include "frontend.h"
#include "toolbar.h"
#include "hash.h"
#include "lstring.h"
#include "pathlist.h"

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_SIGNAL
#include <signal.h>
#endif

#include <errno.h>
#include <sys/types.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#ifndef direct
#define direct dirent
#endif
#else
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#endif


//
// Functions to do execution of operating system commands.
//

namespace {
    void upd_hash(bool);
    void add_dir(const char*, bool, sHtab**);

    sHtab *hashtabp; // before . in path
    sHtab *hashtabc; // .
    sHtab *hashtabs; // after . in path
}


// Create the hash table for the given search path.  pathlist is a : 
// seperated list of directories.  If docc is true, then all the
// commands found are added to the command completion lists.
//
void
CshPar::Rehash(const char *pathlist, bool docc)
{
    if (!pathlist) {
        // called from the cd command
        upd_hash(docc);
        return;
    }

    // Clear out the old hash table
    hashtabp->clear_data(0, 0);
    delete hashtabp;
    hashtabp = 0;
    hashtabc->clear_data(0, 0);
    delete hashtabc;
    hashtabc = 0;
    hashtabs->clear_data(0, 0);
    delete hashtabs;
    hashtabs = 0;

    if (docc)
        Cmds.CcSetup();

    int tabp = 0;
    char buf[BSIZE_SP];
    while (pathlist && *pathlist) {
        int i = 0;
        while (*pathlist && *pathlist != ';' && *pathlist != ':')
            buf[i++] = *pathlist++;
        while (*pathlist == ';' || *pathlist == ':')
            pathlist++;
        buf[i] = '\0';
        if (lstring::eq(buf, ".")) {
            if (tabp)
                continue;
            tabp++;
        }
        if (tabp == 0)
            add_dir(buf, docc, &hashtabp);
        else if (tabp == 1) {
            add_dir(buf, docc, &hashtabc);
            tabp++;
        }
        else
            add_dir(buf, docc, &hashtabs);
    }
}


// The return value is false if no command was found, and true if it was.
//
bool
CshPar::UnixCom(wordlist *wl)
{
    if (!wl)
        return (false);
    char *name = wl->wl_word;
    char **argv = wordlist::mkvec(wl);
    if (cp_flags[CP_DEBUG]) {
        GRpkgIf()->ErrPrintf(ET_MSGS, "name: %s, argv: ", name);
        for (wordlist *ww = wl; ww; ww = ww->wl_next) {
            char *s = lstring::copy(ww->wl_word);
            Strip(s);
            GRpkgIf()->ErrPrintf(ET_MSGS, "%s ", s);
            delete [] s;
        }
        GRpkgIf()->ErrPrintf(ET_MSGS, "\n");
    }
    bool ret;
    if (lstring::strdirsep(name))
        ret = ShellExec(name, argv);
    else {
        char buf[BSIZE_SP];
        char *path = (char*)sHtab::get(hashtabp, name);
        if (path) {
            sprintf(buf, "%s/%s", path, name);
            ret = ShellExec(buf, argv);
        }
        else {
            path = (char*)sHtab::get(hashtabc, name);
            if (path) {
                sprintf(buf, "%s", name);
                ret = ShellExec(buf, argv);
            }
            else {
                path = (char*)sHtab::get(hashtabs, name);
                if (path) {
                    sprintf(buf, "%s/%s", path, name);
                    ret = ShellExec(buf, argv);
                }
                else
                    ret = false;
            }
        }
    }
    for (int i = 0; argv[i]; i++)
        delete [] argv[i];
    delete [] argv;
    return (ret);
}


// Debugging
//
void
CshPar::HashStat()
{
    sHtab::print(hashtabp, "i = %d, name = %s, path = %s\n");
    sHtab::print(hashtabc, "i = %d, name = %s, path = %s\n");
    sHtab::print(hashtabs, "i = %d, name = %s, path = %s\n");
}


namespace {
    // update hash tables - quicker than rebuilding after cd.
    //
    void upd_hash(bool docc)
    {
        wordlist *wl0 = sHtab::wl(hashtabc);
        wordlist *wl = wl0;
        while (wl) {
            if (!sHtab::get(hashtabp, wl->wl_word) &&
                    !sHtab::get(hashtabs, wl->wl_word))
                CP.RemKeyword(CT_COMMANDS, wl->wl_word);
            wl = wl->wl_next;
        }
        wordlist::destroy(wl0);

        hashtabc->clear_data(0, 0);
        delete hashtabc;
        hashtabc = 0;
        add_dir(".", docc, &hashtabc);
    }


    void add_dir(const char *dir, bool docc, sHtab **htab)
    {
        if (*htab == 0) {
            // Operating system commands are case-sensitive.
            *htab = new sHtab(false);
        }
        DIR *wdir;
        if (!(wdir = opendir(dir))) return;
        struct direct *de;
        while ((de = readdir(wdir)) != 0) {
            if (!CP.IsExecutable(de->d_name, dir))
                continue;
#ifdef WIN32
            char *s = strrchr(de->d_name, '.');
            if (s && (lstring::cieq(s+1, "exe") || lstring::cieq(s+1, "com") ||
                    lstring::cieq(s+1, "bat") || lstring::cieq(s+1, "cmd"))) {
                char buf[128];
                strcpy(buf, de->d_name);
                buf[s - de->d_name] = 0;
                (*htab)->add(buf, lstring::copy(dir));
                if (docc) {
                    // Add to completion hash table.
                    unsigned bits[4];
                    bits[0] = bits[1] = bits[2] = bits[3] = 0;
                    CP.AddCommand(buf, bits);
                }
            }
            else
#endif
            {
                (*htab)->add(de->d_name, lstring::copy(dir));
                if (docc) {
                    // Add to completion hash table
                    unsigned bits[4];
                    bits[0] = bits[1] = bits[2] = bits[3] = 0;
                    CP.AddCommand(de->d_name, bits);
                }
            }
        }
        closedir(wdir);
    }
}


// Fork and execute.
//
bool
CshPar::ShellExec(const char *name, char **argv)
{
#ifdef HAVE_FORK
    int status;
    int pid = fork( );
    if (pid == 0) {
        TTY.fixDescriptors();
        ToolBar()->CloseGraphicsConnection();
        execv(name, argv);
        _exit(120);  // A random value
    }
    else {
#ifdef HAVE_SIGACTION
        struct sigaction sa[3];
        sigaction(SIGINT, 0, sa);
        sigaction(SIGQUIT, 0, sa+1);
        sigaction(SIGTSTP, 0, sa+2);
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
#else
        void (*svint)(int), (*svquit)(int), (*svtstp)(int);
        svint = signal(SIGINT, SIG_DFL);
        svquit = signal(SIGQUIT, SIG_DFL);
        svtstp = signal(SIGTSTP, SIG_DFL);
#endif
        int j;
        do {
            j = wait(&status);
        } while (j != pid && j != -1);
#ifdef HAVE_SIGACTION
        sigaction(SIGINT, sa, 0);
        sigaction(SIGQUIT, sa+1, 0);
        sigaction(SIGTSTP, sa+2, 0);
#else
        signal(SIGINT, svint);
        signal(SIGQUIT, svquit);
        signal(SIGTSTP, svtstp);
#endif
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 120)
        return (false);
    return (true);
#else
    argv++;
    wordlist *wl = new wordlist(name, 0);
    if (*argv) {
        wl->wl_next = wordlist::wl_build(argv);
        wl->wl_next->wl_prev = wl;
    }
    char *s = wl->flatten();
    wordlist::destroy(wl);
    System(s);
    putc('\n', stdout);
    delete [] s;
    return (true);

#endif
}


// Return true if the file can be executed.
//
bool
CshPar::IsExecutable(const char *file, const char *dir)
{
#ifdef MSDOS
    char *c;
    if ((c = index(file,'.')) == 0)
        return (false);
    *c = '\0';
    c++;
    if (!cieq(c, "exe") && !cieq(c, "com") && !cieq(c, "bat"))
        return (false);
#else
    char *p = pathlist::mk_path(dir, file);
    int ret = access(p, X_OK);
    delete [] p;
    if (ret)
        return (false);
#endif
    return (true);
}


// A replacement for the stdlib system() which closes the connection
// to X in the child.  The regular system() sometimes causes problems
// in the parent after the child exits.
//
int
CshPar::System(const char *cmd)
{
    if (!cmd)
        return (1);
    CP.SetupTty(TTY.infileno(), true);
#ifdef WIN32
    int rval = system(cmd);
    CP.SetupTty(TTY.infileno(), false);
    return (rval);
#else
#ifndef HAVE_SYS_WAIT_H
    int rval = system(cmd);
    CP.SetupTty(TTY.infileno(), false);
    return (rval);
#else
#ifdef HAVE_SIGACTION
    struct sigaction sa[2];
    sigaction(SIGINT, 0, sa);
    sigaction(SIGQUIT, 0, sa+1);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
#else
    void(*tmpi)(int) = signal(SIGINT, SIG_IGN);
    void(*tmpq)(int) = signal(SIGQUIT, SIG_IGN);
#endif
    // block SIGCHLD
    sigset_t newsigblock, oldsigblock;
    sigemptyset(&newsigblock);
    sigaddset(&newsigblock, SIGCHLD);
    sigprocmask(SIG_BLOCK, &newsigblock, &oldsigblock);

    int pid = fork();
    if (pid == 0) {
        if (CP.Display())
            ToolBar()->CloseGraphicsConnection();
#ifdef HAVE_SIGACTION
        sigaction(SIGINT, sa, 0);
        sigaction(SIGQUIT, sa+1, 0);
#else
        signal(SIGINT, tmpi);
        signal(SIGQUIT, tmpq);
#endif
        sigprocmask(SIG_SETMASK, &oldsigblock, 0);
        setsid();
        execl("/bin/sh", "sh", "-c", cmd, (char*)0);
        _exit(127);
    }
    int status = 0;
    if (pid != -1) {
        do {
            pid = waitpid(pid, &status, 0);
        } while (pid == -1 && errno == EINTR);
    }
#ifdef HAVE_SIGACTION
    sigaction(SIGINT, sa, 0);
    sigaction(SIGQUIT, sa+1, 0);
#else
    signal(SIGINT, tmpi);
    signal(SIGQUIT, tmpq);
#endif
    sigprocmask(SIG_SETMASK, &oldsigblock, 0);
    CP.SetupTty(TTY.infileno(), false);
    return (pid == -1 ? -1 : status);
#endif
#endif
}

