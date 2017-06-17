
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2015 Whiteley Research Inc, all rights reserved.        *
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
 $Id: wlist.h,v 2.2 2015/07/30 17:27:11 stevew Exp $
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

    wordlist(const char*);

    static wordlist *wl_build(char**);

    void free();

    int length() const;
    wordlist *copy() const;
    wordlist *splice(wordlist*);
    wordlist *append(wordlist*);
    void print(FILE*) const;
    char **mkvec() const;
    wordlist *reverse();
    char *flatten() const;
    wordlist *nthelem(int);
    void sort();
    wordlist *range(int, int);

    char *wl_word;
    wordlist *wl_next;
    wordlist *wl_prev;
};

struct wl_gc
{
    wl_gc(wordlist *w = 0)  { wl = w; }
    ~wl_gc()                { wl->free(); }

    void set(wordlist *w)   { wl = w; }
    void flush()            { wl->free(); wl = 0; }

private:
    wordlist *wl;
};
#endif

