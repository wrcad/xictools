
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
#include "variable.h"
#include "toolbar.h"
#include "filestat.h"
#include "miscutil.h"

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#else
#ifdef HAVE_TERMIO_H
#include <termio.h>
#else
#ifdef HAVE_SGTTY_H
#include <sgtty.h>
#endif
#endif
#endif

#ifdef HAVE_GETPWUID
#include <pwd.h>
#endif
#ifdef HAVE_SIGNAL
#include <signal.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <errno.h>

#ifdef WIN32
#include "msw.h"
#endif

// Interface to the TTY.
//
bool sTTYioIF::isInteractive()        { return (CP.GetFlag(CP_INTERACTIVE)); }
int sTTYioIF::getchar()               { return (CP.RawGetc(fileno(stdin))); }
bool sTTYioIF::getWaiting()           { return (CP.GetFlag(CP_WAITING)); }
void sTTYioIF::setWaiting(bool b)     { CP.SetFlag(CP_WAITING, b); }
wordlist *sTTYioIF::getWordlist()     { return (CP.LexString(0)); }
void sTTYioIF::stripList(wordlist *wl)    { CP.StripList(wl); }
sTTYioIF TTYif;


//
// The main entry point for cshpar.
//

// Things go as follows:
// (1) Read the line and do some initial quoting (by setting the 8th bit), 
//  and command ignoring. Also deal with command completion.
// (2) Do history substitutions. (!, ^)
// (3) Do alias substitution.
// 
// In front.c these things get done:
// (4) Do variable substitution. ($varname)
// (5) Do backquote substitution. (``)
// (6) Do globbing. (*, ?, [], {}, ~)
// (7) Do io redirection.


namespace {
    inline bool fileexists(const char *name)
    {
        return (access(name, 0) == 0);
    }
}


void
CommandTab::com_echo(wordlist *wlist)
{
    bool nl = true;
    if (wlist && lstring::eq(wlist->wl_word, "-n")) {
        wlist = wlist->wl_next;
        nl = false;
    }

    while (wlist) {
        char *t = lstring::copy(wlist->wl_word);
        CP.Unquote(t);
        TTY.printf_force("%s", t);
        delete [] t;
        if (wlist->wl_next)
            TTY.printf_force(" ");
        wlist = wlist->wl_next;
    }
    if (nl)
        TTY.printf_force("\n");
    else
        TTY.flush();
}


namespace {
    // Return the path to use for a shell.
    //
    char *get_shell()
    {
        const char *shellpth = 0;
        if (!shellpth)    
            shellpth = getenv("SHELL");
#ifdef WIN32
        if (!shellpth)
            shellpth = getenv("COMSPEC");
        else {
            // SHELL might have been set by cygwin
            if (shellpth[0] == '/' && shellpth[1] == '/') {
                char *s = new char[strlen(shellpth)];
                s[0] = shellpth[2];
                s[1] = ':';
                strcpy(s+2, shellpth + 3);
                return (s);
            }
        }
#endif
        if (!shellpth)
            shellpth = "/bin/sh";
        return (lstring::copy(shellpth));
    }
}


// Fork a shell.
//
void
CommandTab::com_shell(wordlist *wl)
{
    char buf[256];
    char *shellpth = get_shell();
    if (!wl) {
        if (GRpkgIf()->CurDev() &&
                GRpkgIf()->CurDev()->devtype == GRmultiWindow)
            miscutil::fork_terminal(shellpth);
        else
            CP.System(shellpth);
        delete [] shellpth;
        return;
    }

    char *shell = lstring::strip_path(shellpth);

    char tbuf[64];
    strcpy(tbuf, shell);
    shell = tbuf;
    char *t = strrchr(shell, '.');
    if (t)
        *t = 0;
    char *com = wordlist::flatten(wl);

    int shtype = 0;
    if (lstring::eq(shell, "csh") || lstring::eq(shell, "tcsh"))
        shtype = 1;
#ifdef WIN32
    else if (lstring::eq(shell, "cmd"))
        shtype = 2;
#endif
    if (shtype == 0 || shtype == 1) {
        if (GRpkgIf()->CurDev() &&
                GRpkgIf()->CurDev()->devtype == GRmultiWindow) {
            char *tf = filestat::make_temp("xx");
            FILE *fp = fopen(tf, "wb");
            if (fp) {
                filestat::queue_deletion(tf);
                fprintf(fp, shtype == 0 ?
                    "#! %s\n%s\necho press Enter to exit\nread xx\nrm -f %s" :
                    "#! %s\n%s\necho press Enter to exit\n$<\nrm -f %s",
                    shellpth, com, tf);
                fclose(fp);
                sprintf(buf, "%s %s", shellpth, tf);
                miscutil::fork_terminal(buf);
            }
            delete [] tf;
        }
        else
            CP.System(com);
    }
    else {
        // command.com
#ifdef WIN32
        char *tf = filestat::make_temp("xx");
        char *tt = new char[strlen(tf) + 5];
        strcpy(tt, tf);
        strcat(tt, ".bat");
        delete [] tf;
        tf = tt;
        FILE *fp = fopen(tf, "wb");
        if (fp) {
            filestat::queue_deletion(tf);
            tt = tf;
            while (*tt) {
                if (*tt == '/')
                    *tt = '\\';
                tt++;
            }
            fprintf(fp, "echo off\n%s\npause\ndel %s", com, tf);
            fclose(fp);
            PROCESS_INFORMATION *info =
                msw::NewProcess(tf, CREATE_NEW_CONSOLE, false);
            delete info;
        }
        delete [] tf;
#endif
    }
    delete [] shellpth;
    delete [] com;
}


void
CommandTab::com_rehash(wordlist*)
{
    if (!CP.GetFlag(CP_DOUNIXCOM)) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "unixcom not set.\n");
        return;
    }
    char *s = getenv("PATH");
    if (s)
        CP.Rehash(s, !CP.GetFlag(CP_NOCC));
    else
        GRpkgIf()->ErrPrintf(ET_ERROR, "no PATH in environment.\n");
}


void
CommandTab::com_cd(wordlist *wl)
{
#ifdef HAVE_CHDIR
    bool copied = false;
    char *s = 0;
    if (wl == 0) {
        s = getenv("HOME");
#ifdef HAVE_GETPWUID
        if (!s) {
            passwd *pw = getpwuid(getuid());
            if (pw == 0) {
                GRpkgIf()->Perror("getpwuid");
                return;
            }           
            s = pw->pw_dir;
        }
#endif
        if (!s) {
            if ((s = getcwd(0, 0)) != 0) {
                TTY.printf("current directory: %s\n", s);
                delete [] s;
            }
            else
                GRpkgIf()->Perror("getcwd");
            return;
        }
        if (!s) {
            TTY.printf("directory not specified.\n");
            return;
        }

    }
    else {
        s = lstring::copy(wl->wl_word);
        CP.Unquote(s);
        copied = true;
    }

    if (*s && chdir(s) == -1)
        GRpkgIf()->Perror(s);

    if (copied)
        delete [] s;

    ToolBar()->UpdateFiles();
    if (CP.GetFlag(CP_DOUNIXCOM))
        CP.Rehash(0, !CP.GetFlag(CP_NOCC));
#else
    GRpkgIf()->ErrPrintf(ET_ERROR, "'chdir' not available.\n");
#endif
}


void
CommandTab::com_pwd(wordlist*)
{
    char *s = getcwd(0, 0);
    if (s) {
        TTY.printf("%s\n", s);
        delete [] s;
    }
    else
        GRpkgIf()->Perror("getcwd");
}


// The following are string comparison functions.  The expression
// parser does not handle strings, except as vector names.  These
// functions do the string comparison at the shell level and return
// the result in the global return, which can be accessed with "$?".

// The case-sensitive strcmp function.  This can handle the three
// argument form for compatibility with Spice3 and older WRspice.
//
void
CommandTab::com_strcmp(wordlist *wl)
{
    char *var = wl->wl_word;
    char *s1 = lstring::copy(wl->wl_next->wl_word);
    char *s2;
    if (wl->wl_next->wl_next)
        s2 = lstring::copy(wl->wl_next->wl_next->wl_word);
    else {
        s2 = s1;
        s1 = lstring::copy(var);
        var = 0;
    }
    CP.Unquote(s1);
    CP.Unquote(s2);

    if (var) {
        variable v;
        v.set_integer(strcmp(s1, s2));
        CP.RawVarSet(var, true, &v);
    }
    CP.SetReturnVal(strcmp(s1, s2));
    delete [] s1;
    delete [] s2;
}


// Case-insensitive string comparison.
//
void
CommandTab::com_strcicmp(wordlist *wl)
{
    char *s1 = lstring::copy(wl->wl_word);
    CP.Unquote(s1);
    char *s2 = lstring::copy(wl->wl_next->wl_word);
    CP.Unquote(s2);

    CP.SetReturnVal(strcasecmp(s1, s2));
    delete [] s1;
    delete [] s2;
}


// Prefix function, returns 1 if s1 is a prefix of s2, case sensitive.
//
void
CommandTab::com_strprefix(wordlist *wl)
{
    char *s1 = lstring::copy(wl->wl_word);
    CP.Unquote(s1);
    char *s2 = lstring::copy(wl->wl_next->wl_word);
    CP.Unquote(s2);

    CP.SetReturnVal(lstring::prefix(s1, s2) != 0);
    delete [] s1;
    delete [] s2;
}


// Prefix function, returns 1 if s1 is a prefix of s2, case insensitive.
//
void
CommandTab::com_strciprefix(wordlist *wl)
{
    char *s1 = lstring::copy(wl->wl_word);
    CP.Unquote(s1);
    char *s2 = lstring::copy(wl->wl_next->wl_word);
    CP.Unquote(s2);

    CP.SetReturnVal(lstring::ciprefix(s1, s2) != 0);
    delete [] s1;
    delete [] s2;
}


// Set the global return value, may be useful in scripts to return a
// value to the caller.
//
void
CommandTab::com_retval(wordlist *wl)
{
    char *s1 = lstring::copy(wl->wl_word);
    CP.Unquote(s1);

    CP.SetReturnVal(atof(s1));
    delete [] s1;
}
// End of CommandTab functions.


CshPar::CshPar()
{
    cp_program          = 0;
    cp_display          = 0;
    cp_promptstring     = 0;
    cp_altprompt        = 0;

    cp_input            = 0;
    cp_aliases          = 0;
    cp_lastone          = 0;

    cp_srcfiles         = 0;

    cp_event            = 1;
    cp_histlength       = 0;
    cp_maxhistlength    = 1000;
    cp_numdgt           = 0;

    cp_acct_sock        = -1;
    cp_mesg_sock        = -1;
    cp_port             = 0;

    cp_flags[CP_CWAIT]           = false;
    cp_flags[CP_DEBUG]           = false;
    cp_flags[CP_DOUNIXCOM]       = false;
    cp_flags[CP_IGNOREEOF]       = false;
    cp_flags[CP_INTERACTIVE]     = true;
    cp_flags[CP_INTRPT]          = false;
    cp_flags[CP_MESSAGE]         = false;
    cp_flags[CP_NOCC]            = false;
    cp_flags[CP_NOCLOBBER]       = false;
    cp_flags[CP_NOEDIT]          = false;
#ifdef Win32
    cp_flags[CP_LOCK_NOEDIT]     = true;
#else
    cp_flags[CP_LOCK_NOEDIT]     = false;
#endif
    cp_flags[CP_NOGLOB]          = false;
    cp_flags[CP_NONOMATCH]       = false;
    cp_flags[CP_NOTTYIO]         = false;
    cp_flags[CP_WAITING]         = false;
    cp_flags[CP_RAWMODE]         = false;

    cp_amp              = '&';
    cp_back             = '`';
    cp_bang             = '!';
    cp_cbrac            = ']';
    cp_ccurl            = '}';
    cp_comma            = ',';
    cp_csep             = ';';
    cp_dol              = '$';
    cp_gt               = '>';
    cp_hash             = '#';
    cp_hat              = '^';
    cp_huh              = '?';
    cp_lt               = '<';
    cp_obrac            = '[';
    cp_ocurl            = '{';
    cp_star             = '*';
    cp_til              = '~';

    cp_var_catchar      = DEF_VAR_CATCHAR;

    cp_didhsubst        = false;
    cp_bqflag           = false;

    TTY.ioReset();
}


// note for cp_waiting
// To deal with asynchronous text from error messages and the like,
// the global variable cp_waiting is set at the prompt, and reset
// when newline is entered from the keyboard.  During this time,
// text should have an additional '\n' sent first so as not to appear
// on the same line as the prompt.  Further, a pointer to the line
// buffer is set while the cp_lexer stack is valid, so that the prompt
// routine can reprint the characters after an asynchronous output.
//

// cp_cwait
// Are we waiting for a command? This lets signal handling be more clever

// cp_intrpt
// If the user interrupts, set cp_evloop() to ignore subsequent lines
// in scripts, reset by the next prompt.
//


wordlist *
CshPar::Parse(const char *string)
{
    if (!string && cp_flags[CP_INTERACTIVE])
        Prompt();
    wordlist *wlist = Lexer(string);

    if (!string)
        cp_event++;

    if (!wlist || !wlist->wl_word)
        return (wlist);

    pwlist(wlist, "Initial parse");

    HistSubst(&wlist);
    if (!wlist || !wlist->wl_word)
        return (wlist);

    pwlist(wlist, "After history substitution");

    if (cp_flags[CP_INTERACTIVE] && cp_didhsubst) {
        wordlist::print(wlist, stdout);
        putc('\n', stdout);
    }

    // Add the word list to the history
    if (!string && *wlist->wl_word)
        AddHistEnt(cp_event - 1, wlist);

    DoAlias(&wlist);
    pwlist(wlist, "After alias substitution");

    return (wlist);
}


// This routine sets the cp_{in, out, err} pointers and takes the io
// directions out of the command line.
//
void
CshPar::Redirect(wordlist **list)
{
    // redirection symbols are single tokens
    bool gotinput = false, gotoutput = false, goterror = false;
    bool app = false, erralso = false;
    int qcnt = 0;
    if (list == 0)
        return;
    wordlist *wl = *list;
    wordlist *w = wl->wl_next;    // Don't consider empty commands
    while (w) {
        // Don't redirect if in double quotes.  This can happen if a
        // backquote evaluation is inside a double quoted substring.
        //
        char *q = w->wl_word;
        while ((q = strchr(q, '"')) != 0) {
            qcnt++;
            q++;
        }
        if (qcnt & 1) {
            w = w->wl_next;
            continue;
        }
        if (*w->wl_word == cp_lt) {
            wordlist *bt = w;
            if (gotinput) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "ambiguous input redirect.\n");
                goto error;
            }
            gotinput = true;
            w = w->wl_next;
            if (w == 0) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "missing name for input.\n");
                goto error;
            }
            if (*w->wl_word == cp_lt) {
                // Do reasonable stuff here...
            }
            else {
                char *t = lstring::copy(w->wl_word);
                Unquote(t);
                FILE *tmpfp = fopen(t, "r");
                delete [] t;
                if (!tmpfp) {
                    GRpkgIf()->Perror(w->wl_word);
                    goto error;
                }
                else
                    TTY.ioRedirect(tmpfp, 0, 0);
            }
#ifdef CPDEBUG
            if (cp_debug)
                GRpkgIf()->ErrPrintf(ET_MSGS, "Input file is %s...\n",
                    w->wl_word);
#endif
            bt->wl_prev->wl_next = w->wl_next;
            if (w->wl_next)
                w->wl_next->wl_prev = bt->wl_prev;
            wordlist *nw = w->wl_next;
            w->wl_next = 0;
            w = nw;
            wordlist::destroy(bt);
        }
        else if (*w->wl_word == cp_gt) {
            wordlist *bt = w;
            if (gotoutput) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "ambiguous output redirect.\n");
                goto error;
            }
            gotoutput = true;
            w = w->wl_next;
            if (w == 0) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "missing name for output.\n");
                goto error;
            }
            if (*w->wl_word == cp_gt) {
                app = true;
                w = w->wl_next;
                if (w == 0) {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "missing name for output.\n");
                    goto error;
                }
            }
            if (*w->wl_word == cp_amp) {
                erralso = true;
                if (goterror) {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "ambiguous error redirect.\n");
                    goto error;
                }
                goterror = true;
                w = w->wl_next;
                if (w == 0) {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "missing name for output.\n");
                    goto error;
                }
            }
            char *s = lstring::copy(w->wl_word);
            Unquote(s);
            if (cp_flags[CP_NOCLOBBER] && fileexists(s)) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s: file exists.\n", s);
                delete [] s;
                s = 0;
                goto error;
            }
            FILE *tmpfp;
            if (app)
                tmpfp = fopen(s, "a");
            else
                tmpfp = fopen(s, "w+");
            delete [] s;
            s = 0;
            if (!tmpfp) {
                GRpkgIf()->Perror(w->wl_word);
                goto error;
            }
            else
                TTY.ioRedirect(0, tmpfp, 0);
#ifdef CPDEBUG
            if (cp_debug)
                GRpkgIf()->ErrPrintf(ET_MSGS, "Output file is %s... %s\n", 
                    w->wl_word, app ? "(append)" : "");
#endif
            bt->wl_prev->wl_next = w->wl_next;
            if (w->wl_next)
                w->wl_next->wl_prev = bt->wl_prev;
            w = w->wl_next;
            bt->wl_next->wl_next = 0;
            wordlist::destroy(bt);
            if (erralso)
                TTY.ioRedirect(0, 0, TTY.outfile());
        }
        else
            w = w->wl_next;
    }
    *list = wl;
    return;

error:
    wordlist::destroy(wl);
    *list = 0;
}


void
CshPar::pwlist(wordlist *wlist, const char *name)
{
    if (!CP.cp_flags[CP_DEBUG])
        return;
    GRpkgIf()->ErrPrintf(ET_MSGS, "%s : [ ", name);
    for (wordlist *wl = wlist; wl; wl = wl->wl_next) {
        char *word = lstring::copy(wl->wl_word);
        CP.Strip(word);
        GRpkgIf()->ErrPrintf(ET_MSGS, "%s ", word);
        delete [] word;
    }
    GRpkgIf()->ErrPrintf(ET_MSGS, "]\n");
}

