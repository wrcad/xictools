
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
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "cshell.h"
#include "commands.h"
#include "graphics.h"


//
// Do alias substitution.
//


// The function for the "alias" command.
//
void
CommandTab::com_alias(wordlist *wl)
{
    if (wl == 0)
        CP.PrintAliases(0);
    else if (wl->wl_next == 0)
        CP.PrintAliases(wl->wl_word);
    else
        CP.SetAlias(wl->wl_word, wl->wl_next);
}


// The function for the "unalias" command.
//
void
CommandTab::com_unalias(wordlist *wl)
{
    while (wl != 0) {
        CP.Unalias(wl->wl_word);
        wl = wl->wl_next;
    }
}
// End of CommandTab functions.


// Expand the aliases
//
void
CshPar::DoAlias(wordlist **list)
{
    if (list == 0)
        return;

    wordlist *wlist = *list;

    // strip off leading command separators
    while (wlist && *wlist->wl_word == cp_csep && !wlist->wl_word[1]) {
        wordlist *nwl = wlist->wl_next;
        delete [] wlist->wl_word;
        delete wlist;
        wlist = nwl;
    }
    wlist->wl_prev = 0;
    *list = wlist;

    // The alias process is going to modify the "last" line typed, 
    // so save a copy of what it really is and restore it after
    // aliasing is done. We have to do tricky things do get around
    // the problems with ; ...
    //
    wordlist *realw;
    if (!cp_lastone) {
        cp_lastone = new sHistEnt(0, 0);
        realw = 0;
    }
    else 
        realw = wordlist::copy(cp_lastone->text());
    wordlist *comm = wlist;
    wordlist *nextc;
    do {
        wordlist *end = comm->wl_prev;
        comm->wl_prev = 0;
        for (nextc = comm; nextc; nextc = nextc->wl_next) {
            if (*nextc->wl_word == cp_csep && !nextc->wl_word[1]) {
                if (nextc->wl_prev)
                    nextc->wl_prev->wl_next = 0;
                break;
            }
        }

        cp_lastone->update(comm);
        int ntries;
        for (ntries = 21; ntries; ntries--) {

            char *word = comm->wl_word;
            if (*word == '\\') {
                while (*word) {
                    *word = *(word+1);
                    word++;
                }
                break;
            }
            sAlias *al = GetAlias(word);
            if (!al)
                break;

            wordlist *nwl = wordlist::copy(al->text());
            HistSubst(&nwl);

            if (cp_didhsubst) {
                // Make sure that we have an up-to-date last history entry
                cp_lastone->update(nwl);
            }
            else {
                // If it had no history args, then append the rest of the nwl
                wordlist *w;
                for (w = nwl; w->wl_next; w = w->wl_next) ;
                w->wl_next = wordlist::copy(comm->wl_next);
                if (w->wl_next)
                    w->wl_next->wl_prev = w;
            }
            if (lstring::eq(nwl->wl_word, comm->wl_word)) {
                // Just once through...
                wordlist::destroy(comm);
                comm = nwl;
                break;
            }
            else {
                wordlist::destroy(comm);
                comm = nwl;
            }
        }

        if (!ntries) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "alias loop.\n");
            wordlist::destroy(wlist);
            *list = 0;
            return;
        }
        comm->wl_prev = end;
        if (!end)
            wlist = comm;
        else
            end->wl_next = comm;
        while (comm->wl_next)
            comm = comm->wl_next;
        comm->wl_next = nextc;
        if (nextc) {
            nextc->wl_prev = comm;
            nextc = nextc->wl_next;
            comm = nextc;
        }
    } while (nextc);

    if (realw) {
        cp_lastone->update(realw);
        wordlist::destroy(realw);
    }
    else {
        delete cp_lastone;
        cp_lastone = 0;
    }

    *list = wlist;
}


// #define DEBUG

// If we use this, aliases will be in alphabetical order
//
void
CshPar::SetAlias(const char *word, wordlist *wlist)
{
    if (lstring::eq(word, "*")) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't alias \"*\".");
        return;
    }
    Unalias(word);
    AddKeyword(CT_ALIASES, word);
    sAlias *al;
    if (cp_aliases == 0) {
#ifdef DEBUG
        printf("first one...\n");
#endif
        al = cp_aliases = new sAlias(word, wlist);
    }
    else {
#ifdef DEBUG
        printf("inserting %s: %s ...\n", word, wlist->wl_word);
#endif
        for (al = cp_aliases; al->next(); al = al->next()) {
#ifdef DEBUG
            printf("checking %s...\n", al->name());
#endif
            if (strcmp(al->name(), word) > 0)
                break;
        }
        if (!al->next() && strcmp(al->name(), word) < 0) {
            al->set_next(new sAlias(word, wlist));
            al->next()->set_prev(al);
            al = al->next();
        }
        else {
            // The new one goes before al.
            if (al->prev()) {
                al = al->prev();
                sAlias *ta = al->next();
                al->set_next(new sAlias(word, wlist));
                al->next()->set_prev(al);
                al = al->next();
                al->set_next(ta);
                ta->set_prev(al);
            }
            else {
                cp_aliases = new sAlias(word, wlist);
                cp_aliases->set_next(al);
                al->set_prev(cp_aliases);
                al = cp_aliases;
            }
        }
    }
    StripList(al->text());

    // We can afford to not worry about the bits, because before
    // the keyword lookup is done the alias is evaluated. 
    // Make everything file completion, just in case...
    //
    unsigned bits[4];
    bits[0] = bits[1] = bits[2] = bits[3] = 1;
    AddCommand(word, bits);
#ifdef DEBUG
    printf("word %s, next = %s, prev = %s...\n", al->al_name, 
            al->al_next ? al->al_next->al_name : "(none)", 
            al->al_prev ? al->al_prev->al_name : "(none)");
#endif
}


void
CshPar::SetAlias(const char *word, const char *list)
{
    wordlist *wl = new wordlist(list);
    SetAlias(word, wl);
    wordlist::destroy(wl);
}


// Remove word from alias list.  If word is "*", clear the alias list.
// Note that "*" can't be aliased.
//
void
CshPar::Unalias(const char *word)
{
    if (lstring::eq(word, "*")) {
        while (cp_aliases) {
            sAlias *al = cp_aliases->next();
            RemKeyword(CT_ALIASES, cp_aliases->name());
            RemKeyword(CT_COMMANDS, cp_aliases->name());
            delete cp_aliases;
            cp_aliases = al;
        }
        return;
    }

    RemKeyword(CT_ALIASES, word);
    sAlias *al = GetAlias(word);
    if (al == 0)
        return;
    if (al->next())
        al->next()->set_prev(al->prev());
    if (al->prev())
        al->prev()->set_next(al->next());
    else {
        al->next()->set_prev(0);
        cp_aliases = al->next();
    }
    delete al;
    RemKeyword(CT_COMMANDS, word);
}


void
CshPar::PrintAliases(const char *word)
{
    for (sAlias *al = cp_aliases; al; al = al->next()) {
        if (!word || lstring::eq(al->name(), word)) {
            if (!word)
                TTY.printf("%s\t", al->name());
            TTY.wlprint(al->text());
            TTY.send("\n");
        }
    }
}


sAlias *
CshPar::GetAlias(const char *word)
{
    for (sAlias *al = cp_aliases; al; al = al->next()) {
        if (lstring::eq(al->name(), word))
            return (al);
    }
    return (0);
}

