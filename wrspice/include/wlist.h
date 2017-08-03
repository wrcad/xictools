
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
Authors: 1986 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef WLIST_H
#define WLIST_H

#include "lstring.h"


// Doubly linked lists of words
//
struct wordlist
{
    wordlist()
        {
            wl_next = wl_prev = 0;
            wl_word = 0;
        };

    wordlist(const char *word, wordlist *prev)
        {
            wl_next = 0;
            wl_prev = prev;
            wl_word = lstring::copy(word);
        };

    // Build a wordlist from a string.  This has obvious limitations.
    //
    wordlist(const char *s)
        {
            wl_word = lstring::gettok(&s);
            wl_prev = 0;
            wl_next = *s ? new wordlist(s) : 0;
            if (wl_next)
                wl_next->wl_prev = this;
        }

    // Turn an array of char *'s into a wordlist.
    //
    static wordlist *wl_build(char *v[])
        {
            wordlist *wlist = 0, *wl = 0;
            if (v) {
                while (*v) {
                    wordlist *cwl = new wordlist(*v, wl);
                    if (wl)
                        wl->wl_next = cwl;
                    else
                        wlist = cwl;
                    wl = cwl;
                    v++;
                }
            }
            return (wlist);
        }

    static void destroy(wordlist *wl)
        {
            while (wl) {
                wordlist *wx = wl;
                wl = wl->wl_next;
                delete [] wx->wl_word;
                delete wx;
            }
        }

    static int length(const wordlist *thiswl)
        {
            int i = 0;
            for (const wordlist *wl = thiswl; wl; wl = wl->wl_next)
                i++;
            return (i);
        }


    // Copy the list, and the words.
    //
    static wordlist *copy(const wordlist *thiswl)
        {
            wordlist *nwl = 0, *w = 0;
            for (const wordlist *wl = thiswl; wl; wl = wl->wl_next) {
                if (nwl == 0)
                    nwl = w = new wordlist(wl->wl_word, 0);
                else {
                    w->wl_next = new wordlist(wl->wl_word, w);
                    w = w->wl_next;
                }
            }
            return (nwl);
        }

    // Substitute a wordlist for one element of a wordlist, and return
    // a pointer to the last element of the inserted list. 
    // wl_splice(elt, list) -> elt->splice(list)
    //
    wordlist *splice(wordlist *list)
        {
            if (list)
                list->wl_prev = wl_prev;
            if (wl_prev)
                wl_prev->wl_next = list;
            if (list) {
                while (list->wl_next)
                    list = list->wl_next;
                list->wl_next = wl_next;
            }
            if (wl_next)
                wl_next->wl_prev = list;
            delete [] wl_word;
            delete this;
            return (list);
        }

    // Link nwl to end of wordlist.
    //
    static wordlist *append(wordlist *thiswl, wordlist *nwl)
        {
            if (!thiswl)
                return (nwl);
            if (!nwl)
                return (thiswl);
            wordlist *wl = thiswl;
            for ( ; wl->wl_next; wl = wl->wl_next) ;
            wl->wl_next = nwl;
            nwl->wl_prev = wl;
            return (thiswl);
        }

    // Print a word list. (No \n at the end...)
    // The 8'th bit is unset here to strip shell quoting.
    //
    static void print(const wordlist *thiswl, FILE *fp)
        {
            for (const wordlist *wl = thiswl; wl; wl = wl->wl_next) {
                if (!wl->wl_word)
                    continue;
                for (const char *s = wl->wl_word; *s; s++)
                    putc((*s & 0x7f), fp);
                if (wl->wl_next)
                    putc(' ', fp);
            }
        }

    // Turn a wordlist into an array of char *'s.
    //
    static char **mkvec(const wordlist *thiswl)
        {
            if (!thiswl)
                return (0);
            const wordlist *wl = thiswl;
            int len = length(wl);
            char **v = new char*[len+1];
            int i;
            for (i = 0; i < len; i++) {
                v[i] = lstring::copy(wl->wl_word);
                wl = wl->wl_next;
            }
            v[i] = 0;
            return (v);
        }

    // Reverse a word list.
    //
    static wordlist *reverse(wordlist *thiswl)
        {
            if (!thiswl)
                return (0);
            wordlist *w, *t;
            for (w = thiswl; ; w = t) {
                 t = w->wl_next;
                 w->wl_next = w->wl_prev;
                 w->wl_prev = t;
                 if (t == 0)
                    break;
            }
            return (w);
        }

    // Convert a wordlist into a string.
    //
    static char *flatten(const wordlist *thiswl)
        {
            if (!thiswl)
                return (0);
            const wordlist *tw;
            int i = 0;
            for (tw = thiswl; tw; tw = tw->wl_next)
                i += strlen(tw->wl_word) + 1;
            char *buf = new char[i + 1];

            buf[0] = '\0';
            char *t = buf;
            for (tw = thiswl; tw; tw = tw->wl_next) {
                strcpy(t, tw->wl_word);
                while (*t)
                    t++;
                if (tw->wl_next)
                    *t++ = ' ';
            }
            return (buf);
        }

    // Return the nth element of a wordlist, or the last one if n is
    // too big.  Numbering starts at 0... 
    //
    static wordlist *nthelem(wordlist *thiswl, int i)
        {
            if (!thiswl)
                return (0);
            wordlist *ww = thiswl;
            while ((i-- > 0) && ww->wl_next)
                ww = ww->wl_next;
            return (ww);
        }

    static void sort(wordlist*);
    static wordlist *range(wordlist*, int, int);

    char *wl_word;
    wordlist *wl_next;
    wordlist *wl_prev;
};

struct wl_gc
{
    wl_gc(wordlist *w = 0)  { wl = w; }
    ~wl_gc()                { wordlist::destroy(wl); }

    void set(wordlist *w)   { wl = w; }
    void flush()            { wordlist::destroy(wl); wl = 0; }

private:
    wordlist *wl;
};
#endif

