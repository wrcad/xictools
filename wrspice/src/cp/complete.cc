
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
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "config.h"
#include "cshell.h"
#include "simulator.h"
#include "graph.h"
#include "output.h"

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
#ifdef HAVE_GETPWUID
#include <pwd.h>
#endif


// Command completion code. We keep a data structure with information on
// each command, to make lookups fast.   Each command has an array of 4
// bitmasks (hardwired), stating whether the command takes that
// particular class of keywords in that position. Class 0 always means
// filename completion.

namespace {
    void printem(wordlist*);

    const char *msg1 = "cp_addkword: bad class %d.\n";
}


void
CshPar::Complete(wordlist *wlist, const char *buf, bool esc)
{
    char wbuf[BSIZE_SP], xbuf[BSIZE_SP];
    strcpy(xbuf, buf);
    Unquote(xbuf);
    Strip(xbuf);
    int i = 0;
    wordlist *pmatches = 0;
    if (wlist->wl_next) {   // Not the first word
        const char *first = wlist->wl_word;
        sTrie **commands = CP.CcClass(CT_COMMANDS);
        if (!commands) {
            GRpkgIf()->ErrPrintf(ET_INTERR, msg1, CT_COMMANDS);
            return;
        }
        // First look for aliases. Just interested in the first word... 
        // Don't bother doing history yet -- that might get complicated.
        //
        int ntries = 21;
        while (ntries-- > 0) {
            sAlias *al = GetAlias(first);
            if (al == 0)
                break;
            first = al->name();
        }
        if (ntries == 0) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "alias loop.\n");
            return;
        }
        sTrie *cc = (*commands)->lookup(first, false, false);
        if (cc && cc->invalid())
            cc = 0;
        int arg = wordlist::length(wlist) - 2;
        if (arg > 3)
            arg = 3;
        // First filenames
        if (cc && (cc->keyword(arg) & 1)) {
            pmatches = FileC(xbuf);
            char *s = lstring::strrdirsep(xbuf);
            i = strlen(s ? s + 1 : xbuf);
            if ((*xbuf == '~') && !lstring::strdirsep(xbuf))
                i--;
        }

        // The keywords
        for (int j = 1; ; j++) {
            if (j == CT_COMMANDS)
                continue;
            sTrie **kw = CcClass(j);
            if (!kw)
                break;
            if (cc && (cc->keyword(arg) & (1 << j))) {
                // Find all the matching keywords
                wordlist *a = (*kw)->match(xbuf);
                i = strlen(xbuf);
                pmatches = wordlist::append(pmatches, a);
            }
        }
        wordlist::sort(pmatches);
    }
    else {
        sTrie **commands = CcClass(CT_COMMANDS);
        if (commands) {
            pmatches = (*commands)->match(xbuf);
            i = strlen(xbuf);
        }
    }
    if (!esc) {
        if (pmatches) {
            // erase "^D"
            if (cp_flags[CP_NOEDIT])
                TTY.out_printf("\b\b  \b\b");
            TTY.out_printf("...  matches:\n");
            printem(pmatches);
            wordlist::destroy(pmatches);
            Prompt();
        }
        else {
            TTY.out_printf("\007");
            // erase "^D"
            if (cp_flags[CP_NOEDIT]) {
                TTY.out_printf("\b\b  \b\b");
                Prompt();
            }
        }
        return;
    }
    if (pmatches == 0) {
        TTY.out_printf("\007");
        // erase "^["
        if (cp_flags[CP_NOEDIT]) {
            TTY.out_printf("\b\b  \r");
            Prompt();
        }
        return;
    }
    // erase "^["
    if (cp_flags[CP_NOEDIT]) {
        TTY.out_printf("\b\b  \r");
        Prompt();
    }
    if (pmatches->wl_next == 0) {
        strcpy(wbuf, &pmatches->wl_word[i]);
        goto found;
    }
    // Now we know which words might work. Extend the command as much
    // as possible, then write the characters out.
    //
    for (int j = 0;; j++, i++) {
        wbuf[j] = pmatches->wl_word[i];
        for (wordlist *a = pmatches->wl_next; a; a = a->wl_next)
                if (a->wl_word[i] != wbuf[j]) {
                    TTY.flush();
                    wbuf[j] = '\0';
                    goto found;
                }
        if (wbuf[j] == '\0')
            goto found;
    }
found:
    StuffChars(wbuf);
    wordlist::destroy(pmatches);
}


// Add a keyword to the database.
//
void
CshPar::AddKeyword(int kclass, const char *word)
{
    if (cp_flags[CP_NOCC] || !word || !*word)
        return;
    sTrie **kw = CcClass(kclass);
    if (!kw) {
        GRpkgIf()->ErrPrintf(ET_INTERR, msg1, kclass);
        return;
    }
    if (!*kw) {
        char buf[4];
        buf[0] = *word;
        buf[1] = '\0';
        *kw = new sTrie(buf);
        if (word[1])
            (*kw)->set_invalid(true);
    }
    sTrie *cc = (*kw)->lookup(word, false, true);
    cc->set_invalid(false);
}


// Add a command to the database.
//
void
CshPar::AddCommand(const char *word, unsigned *bits)
{
    if (cp_flags[CP_NOCC] || !word || !*word)
        return;
    sTrie **commands = CcClass(CT_COMMANDS);
    if (!commands) {
        GRpkgIf()->ErrPrintf(ET_INTERR, msg1, CT_COMMANDS);
        return;
    }
    if (!*commands) {
        char buf[4];
        buf[0] = *word;
        buf[1] = '\0';
        *commands = new sTrie(buf);
        if (word[1])
            (*commands)->set_invalid(true);
    }
    sTrie *cc = (*commands)->lookup(word, false, true);
    cc->set_invalid(false);
    cc->set_keyword(0, bits[0]);
    cc->set_keyword(1, bits[1]);
    cc->set_keyword(2, bits[2]);
    cc->set_keyword(3, bits[3]);
}


// Remove a keyword from the database.
//
void
CshPar::RemKeyword(int kclass, const char *word)
{
    sTrie **kw = CcClass(kclass);
    if (!kw) {
        GRpkgIf()->ErrPrintf(ET_INTERR, msg1, kclass);
        return;
    }
    if (!*kw)
        return;
    sTrie *cc = (*kw)->lookup(word, false, false);
    if (cc)
        cc->set_invalid(true);
}


// Throw away all the stuff and prepare to rebuild it from scratch...
//
void
CshPar::CcRestart(bool kwords)
{
    sTrie **commands = CcClass(CT_COMMANDS);
    if (commands) {
        delete *commands;
        *commands = 0;
        if (kwords) {
            for (int i = 1; ; i++) {
                if (i == CT_COMMANDS)
                    continue;
                sTrie **kw = CcClass(i);
                if (!kw)
                    break;
                delete *kw;
                *kw = 0;
            }
        }
    }
}


// Return the address of the keyword completion structure for kclass.
//
sTrie **
CshPar::CcClass(int kclass)
{
    static sTrie *keywords[12];
    switch (kclass) {
    default:
    case CT_FILENAME:
        break;
    case CT_COMMANDS:
        return (keywords+0);
    case CT_ALIASES:
        return (keywords+1);
    case CT_RUSEARGS:
        return (keywords+2);
    case CT_OPTARGS:
        return (keywords+3);
    case CT_STOPARGS:
        return (keywords+4);
    case CT_LISTINGARGS:
        return (keywords+5);
    case CT_PLOTKEYWORDS:
        return (keywords+6);
    case CT_VARIABLES:
        return (keywords+7);
    case CT_UDFUNCS:
        return (keywords+8);
    case CT_CKTNAMES:
        return (keywords+9);
    case CT_PLOT:
        return (keywords+10);
    case CT_TYPENAMES:
        return (keywords+11);
    case CT_VECTOR:
        if (OP.curPlot())
            return (OP.curPlot()->ccom());
        break;
    case CT_DEVNAMES:
        if (Sp.CurCircuit())
            return (Sp.CurCircuit()->devices());
        break;
    case CT_MODNAMES:
        if (Sp.CurCircuit())
            return (Sp.CurCircuit()->models());
        break;
    case CT_NODENAMES:
        if (Sp.CurCircuit())
            return (Sp.CurCircuit()->nodes());
        break;
    }
    return (0);
}


// Figure out what files match the prefix.
//
wordlist *
CshPar::FileC(const char *buf)
{
    DIR *wdir = 0;
    wordlist *wl = 0;
    const char *lcomp = lstring::strrdirsep(buf);
    if (lcomp == 0) {
        lcomp = buf;
#ifdef HAVE_GETPWUID
        if (*buf == cp_til) {   // User name completion...
            buf++;
            struct passwd *pw;
            while ((pw = getpwent()) != 0) {
                if (lstring::prefix(buf, pw->pw_name)) {
                    if (wl == 0)
                        wl = new wordlist(pw->pw_name, 0);
                    else {
                        wordlist *t = wl;
                        wl = new wordlist(pw->pw_name, 0);
                        wl->wl_next = t;
                        t->wl_prev = wl;
                    }
                }
            }
            endpwent();
            return (wl);
        }
#endif
        wdir = opendir(".");
    }
    else {
        char *ct = lstring::copy(buf);
        ct[lcomp - buf] = 0;
        char *dir = TildeExpand(ct);
        delete [] ct;
        lcomp++;
        if (dir == 0)
            return (0);
        wdir = opendir(dir);
        delete [] dir;
    }
    if (wdir == 0)
        return (0);
    struct direct *de;
    while ((de = readdir(wdir)) !=0) {
        if ((lstring::prefix(lcomp, de->d_name)) && (*lcomp ||
                (*de->d_name != '.'))) {
            if (wl == 0)
                wl = new wordlist(de->d_name, 0);
            else {
                wordlist *t = wl;
                wl = new wordlist(de->d_name, 0);
                wl->wl_next = t;
                t->wl_prev = wl;
            }
        }
    }
    closedir(wdir);
    wordlist::sort(wl);
    return (wl);
}
// End of CshPar functions.


void
sTrie::free()
{
    for (int i = 1; ; i++) {
        sTrie **kw = CP.CcClass(i);
        if (!kw)
            break;
        if (this == *kw)
            *kw = 0;
    }
    delete this;
}


// See what keywords or commands match the prefix. Check extra also for
// matches, if it is non-0. Return a wordlist which is in alphabetical
// order. Note that we have to call this once for each class.
//
wordlist *
sTrie::match(const char *word)
{
    wordlist *twl = 0;
    sTrie *cc = lookup(word, true, false);
    if (cc) {
        if (*word)  // This is a big drag
            twl = cc->wl(false);
        else
            twl = cc->wl(true);
    }
    return (twl);
}


wordlist *
sTrie::wl(bool sib)
{
    wordlist *twl;
    if (!cc_invalid) {
        twl = new wordlist(cc_name, 0);
        twl->wl_next = cc_child ? cc_child->wl(true) : 0;
        if (twl->wl_next)
            twl->wl_next->wl_prev = twl;
    }
    else
        twl = cc_child ? cc_child->wl(true) : 0;
    if (sib) {
        wordlist *end;
        if (twl) {
            for (end = twl; end->wl_next; end = end->wl_next) ;
            end->wl_next = cc_sibling ? cc_sibling->wl(true) : 0;
            if (end->wl_next)
                end->wl_next->wl_prev = twl;
        }
        else
            twl = cc_sibling ? cc_sibling->wl(true) : 0;
    }
    return (twl);
}


// Look up a word in the database.  Because of the way the tree is set
// up, this also works for looking up all words with a given prefix
// (if the pref arg is true).  If create is true, then the node is
// created if it doesn't already exist.
//
sTrie *
sTrie::lookup(const char *word, bool pref, bool create)
{
    sTrie *place = this;
    if (!place || !word || !*word)
        return place;
    int ind = 0;
    while (word[ind]) {
        // Walk down the sibling list until we find a node that
        // matches 'word' to 'ind' places.
        //
        while ((place->cc_name[ind] < word[ind]) && place->cc_sibling)
            place = place->cc_sibling;
        if (place->cc_name[ind] < word[ind]) {

            // This line doesn't go out that far...
            if (!create)
                return (0);

            place->cc_sibling = new sTrie(0);
            place->cc_sibling->cc_ysibling = place;
            place->cc_sibling->cc_parent = place->cc_parent;
            place = place->cc_sibling;
            char *ct =  new char[ind + 2];
            for (int i = 0; i < ind + 1; i++)
                ct[i] = word[i];
            ct[ind + 1] = '\0';
            place->cc_name = ct;
            place->cc_invalid = 1;
        }
        else if (place->cc_name[ind] > word[ind]) {
            if (!create)
                return (0);

            // Put this one between place and its pred
            sTrie *tmpc = new sTrie(0);
            tmpc->cc_parent = place->cc_parent;
            tmpc->cc_sibling = place;
            tmpc->cc_ysibling = place->cc_ysibling;
            place->cc_ysibling = tmpc;
            place = tmpc;
            if (tmpc->cc_ysibling)
                tmpc->cc_ysibling->cc_sibling = tmpc;
            else if (tmpc->cc_parent)
                tmpc->cc_parent->cc_child = tmpc;
            else {
                for (int i = 1; ; i++) {
                    sTrie **kw = CP.CcClass(i);
                    if (!kw)
                        break;
                    if (this == *kw) {
                        *kw = place;
                        break;
                    }
                }
            }
            char *ct = new char[ind + 2];
            for (int i = 0; i < ind + 1; i++)
                ct[i] = word[i];
            ct[ind + 1] = '\0';
            place->cc_name = ct;
            place->cc_invalid = 1;
        }

        // place now points to that node that matches the word for
        // ind + 1 characters.
        //
        // printf("place %s, word %s, ind %d\n", place->cc_name, word, ind); 
        //
        if (word[ind + 1]) {    // More to go...
            if (!place->cc_child) {

                // No children, maybe make one and go on
                if (!create)
                    return (0);
                sTrie *tmpc = new sTrie(0);
                tmpc->cc_parent = place;
                place->cc_child = tmpc;
                place = tmpc;
                char *ct = new char[ind + 3];
                for (int i = 0; i < ind + 2; i++)
                    ct[i] = word[i];
                ct[ind + 2] = '\0';
                place->cc_name = ct;
                if (word[ind + 2])
                    place->cc_invalid = 1;
            }
            else
                place = place->cc_child;
            ind++;
        }
        else
            break;
    }
    if (!pref && !create && place->cc_invalid) {
        // This is no good, we want a real word
        return (0);
    }
    return (place);
}


namespace {
    // Print the words in the wordlist in columns, they are already
    // sorted. 
    //
    void printem(wordlist *wl)
    {
        int maxl = 0;
        int shrink = 0;
        if (wl == 0)
            return;
        int width;
        TTY.winsize(&width, 0);
        TTY.init_more();
        int num = wordlist::length(wl);
        for (wordlist *ww = wl; ww; ww = ww->wl_next) {
            int j = strlen(ww->wl_word);
            if (j > maxl)
                maxl = j;
        }
        if (++maxl % 8)
            maxl += 8 - (maxl % 8);
        int ncols = width / maxl;
        if (width == maxl * ncols) shrink = 1;
        if (ncols == 0)
            ncols = 1;
        int nlines = num / ncols + (num % ncols ? 1 : 0);
        for (int k = 0; k < nlines; k++) {
            for (int i = 0; i < ncols; i++) {
                int j = i*nlines + k;
                if (j < num) {
                    TTY.printf("%-*s", maxl-shrink*(i+1)/ncols, 
                        wordlist::nthelem(wl, j)->wl_word);
                }
                else
                    break;
            }
            TTY.send("\n");
        }
        TTY.send("\n");
    }
}

