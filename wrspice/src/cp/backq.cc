
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: backq.cc,v 2.49 2015/06/20 01:58:11 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "config.h"
#include "cshell.h"
#include "filestat.h"
#include "graphics.h"
#ifdef WIN32
#include <setjmp.h>
#endif


//
// Do backquote substitution on a word list. 
//

void
CshPar::BackQuote(wordlist **list)
{
    if (list == 0)
        return;
    wordlist *wlist = *list;
    for (wordlist *wl = wlist; wl; wl = wl->wl_next) {
        char *t = wl->wl_word;
        if (!t)
            continue;

        char *s;
        while ((s = strchr(t, cp_back)) != 0) {

            char *wbuf = new char[s - wl->wl_word + 1];
            strncpy(wbuf, wl->wl_word, s - wl->wl_word);
            wbuf[s - wl->wl_word] = 0;
            char *bcmd = lstring::copy(s+1);
            t = s+1;
            s = bcmd;

            bool quoted = false;
            for (char *tm = wbuf; *tm; tm++) {
                if (*tm == '"')
                    quoted = !quoted;
            }

            while (*s && (*s != cp_back)) {
                // Get s and t past the next backquote
                t++;
                s++;
            }
            *s = '\0';
            if (*t)
                t++;    // Get past the second ` 
            wordlist *nwl = BackEval(bcmd);
            delete [] bcmd;
            if (!nwl || !nwl->wl_word) {
                delete [] wbuf;
                wordlist::destroy(wlist);
                *list = 0;
                return;
            }
            if (quoted && nwl->wl_next) {
                char *tt = wordlist::flatten(nwl);
                if (strlen(tt) + strlen(wbuf) + strlen(t) >= BSIZE_SP) {
                    tt[BSIZE_SP - strlen(wbuf) - strlen(t) - 1] = 0;
                    GRpkgIf()->ErrPrintf(ET_WARN,
                        "line too long, truncated.\n");
                }
                wordlist::destroy(nwl->wl_next);
                nwl->wl_next = 0;
                delete [] nwl->wl_word;
                nwl->wl_word = tt;
            }

            char *tt = new char[strlen(wbuf) + strlen(nwl->wl_word) + 1];
            strcpy(tt, wbuf);
            strcat(tt, nwl->wl_word);
            delete [] nwl->wl_word;
            nwl->wl_word = tt;

            delete [] wbuf;

            t = lstring::copy(t);
            bool upd = (wl == wlist);
            wl = wl->splice(nwl);
            if (upd)
                wlist = nwl;

            int i = strlen(wl->wl_word);
            tt = new char[i + strlen(t) + 1];
            strcpy(tt, wl->wl_word);
            strcat(tt, t);
            delete [] t;
            delete [] wl->wl_word;
            wl->wl_word = tt;
            t = &wl->wl_word[i];
        }
    }
    *list = wlist;
}


// Do a popen with the string, and then reset the file pointers so that
// we can use the first pass of the parser on the output.
//
wordlist *
CshPar::BackEval(const char *string)
{
    if (!*string)
        return (0);
    const char *sbak = string;
    char *tok = lstring::gettok(&string);
    if (tok && !strcasecmp(tok, "shell")) {
        delete [] tok;

        // This is the original Spice3 implementation.  Now, we
        // prepend the command with "shell" to get here.

        FILE *proc = popen(string, "r");
        if (proc == 0) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "can't evaluate %s.\n", string);
            return (0);
        }
        FILE *old = cp_input;
        cp_input = proc;
        bool intv = cp_flags[CP_INTERACTIVE];
        cp_flags[CP_INTERACTIVE] = false;
        cp_bqflag = true;
        wordlist *wl = Lexer(0);
        cp_bqflag = false;
        cp_input = old;
        cp_flags[CP_INTERACTIVE] = intv;
        pclose(proc);
        return (wl);
    }
    else {
        delete [] tok;
        string = sbak;

        // New implementation, use our own shell for evaluation.

        char *tempfile = filestat::make_temp("sp");
        FILE *fp = fopen(tempfile, "w+");
        TTY.ioPush(fp);
        CP.PushControl();
        bool intr = cp_flags[CP_INTERACTIVE];
        cp_flags[CP_INTERACTIVE] = false;

#ifdef WIN32
        extern jmp_buf msw_jbf[4];
        extern int msw_jbf_sp;

        bool dopop = false;
        if (msw_jbf_sp < 3) {
            msw_jbf_sp++;
            dopop = true;
        }
        if (setjmp(msw_jbf[msw_jbf_sp]) == 0) {
            EvLoop(string);
        }
        if (dopop)
            msw_jbf_sp--;
#else
        try { EvLoop(string); }
        catch (int) { }
#endif

        cp_flags[CP_CWAIT] = true;  // have to reset this
        cp_flags[CP_INTERACTIVE] = intr;
        CP.PopControl();
        TTY.ioPop();

        fflush(fp);
        rewind(fp);

        FILE *tfp = cp_input;
        cp_input = fp;
        cp_flags[CP_INTERACTIVE] = false;
        cp_bqflag = true;
        wordlist *wl = Lexer(0);
        cp_bqflag = false;
        cp_input = tfp;
        cp_flags[CP_INTERACTIVE] = intr;
        fclose(fp);
        unlink(tempfile);
        delete [] tempfile;
        return (wl);
    }
}

