
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
 $Id: wlist.cc,v 2.53 2015/06/20 01:58:12 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "wlist.h"
#include <stdlib.h>


//
// Wordlist manipulation stuff.
//

// Build a wordlist from a string.  This has obvious limitations.
//
wordlist::wordlist(const char *s)
{
    wl_word = lstring::gettok(&s);
    wl_prev = 0;
    wl_next = *s ? new wordlist(s) : 0;
    if (wl_next)
        wl_next->wl_prev = this;
}


// Static function.
// Turn an array of char *'s into a wordlist.
//
wordlist *
wordlist::wl_build(char *v[])
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


// Free the storage used by a word list.
//
void
wordlist::free()
{
    wordlist *wl, *nw;
    for (wl = this; wl; wl = nw) {
        nw = wl->wl_next;
        delete [] wl->wl_word;
        delete wl;
    }
}


// Determine the length of a word list.
//
int
wordlist::length() const
{
    int i = 0;
    for (const wordlist *wl = this; wl; wl = wl->wl_next)
        i++;
    return (i);
}


// Copy a wordlist and the words.
//
wordlist *
wordlist::copy() const
{
    wordlist *nwl = 0, *w = 0;
    for (const wordlist *wl = this; wl; wl = wl->wl_next) {
        if (nwl == 0)
            nwl = w = new wordlist(wl->wl_word, 0);
        else {
            w->wl_next = new wordlist(wl->wl_word, w);
            w = w->wl_next;
        }
    }
    return (nwl);
}


// Substitute a wordlist for one element of a wordlist, and return a pointer
// to the last element of the inserted list.
// wl_splice(elt, list) -> elt->splice(list)
//
wordlist *
wordlist::splice(wordlist *list)
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
wordlist *
wordlist::append(wordlist *nwl)
{
    const wordlist *thiswl = this;
    if (!thiswl)
        return (nwl);
    if (!nwl)
        return (this);
    wordlist *wl;
    for (wl = this; wl->wl_next; wl = wl->wl_next) ;
    wl->wl_next = nwl;
    nwl->wl_prev = wl;
    return (this);
}


// Print a word list. (No \n at the end...)
// The 8'th bit is unset here to strip shell quoting.
//
void
wordlist::print(FILE *fp) const
{
    const wordlist *thiswl = this;
    if (!thiswl)
        return;
    for (const wordlist *wl = this; wl; wl = wl->wl_next) {
        if (!wl->wl_word)
            continue;
        for (const char *s = wl->wl_word; *s; s++)
            putc((*s & 0x7f), fp);
        if (wl->wl_next)
            putc(' ', fp);
    }
}


// Turn a wordlist into an array of char *'s
//
char **
wordlist::mkvec() const
{
    const wordlist *thiswl = this;
    if (!thiswl)
        return (0);
    const wordlist *wl = this;
    int len = length();
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
wordlist *
wordlist::reverse()
{
    const wordlist *thiswl = this;
    if (!thiswl)
        return (0);
    wordlist *w, *t;
    for (w = this; ; w = t) {
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
char *
wordlist::flatten() const
{
    const wordlist *thiswl = this;
    if (!thiswl)
        return (0);
    const wordlist *tw;
    int i = 0;
    for (tw = this; tw; tw = tw->wl_next)
        i += strlen(tw->wl_word) + 1;
    char *buf = new char[i + 1];

    buf[0] = '\0';
    char *t = buf;
    for (tw = this; tw; tw = tw->wl_next) {
        strcpy(t, tw->wl_word);
        while (*t)
            t++;
        if (tw->wl_next)
            *t++ = ' ';
    }
    return (buf);
}


// Return the nth element of a wordlist, or the last one if n is too big.
// Numbering starts at 0... 
//
wordlist *
wordlist::nthelem(int i)
{
    const wordlist *thiswl = this;
    if (!thiswl)
        return (0);
    wordlist *ww = this;
    while ((i-- > 0) && ww->wl_next)
        ww = ww->wl_next;
    return (ww);
}


// Sort so that strings containing an integer value will be sorted in
// order of the value, if the alpha parts are the same.  This will put
// x2 ahead of x10, for example,
//
static int
wlcomp(const void *s, const void *t)
{
    const char *s1 = *(const char**)s;
    const char *s2 = *(const char**)t;

    while (*s1 && *s2) {
        if (isdigit(*s1) && isdigit(*s2)) {
            int d = atoi(s1) - atoi(s2);
            if (d != 0)
                return (d);
            // same number, but may be leading 0's
            while (isdigit(*s1)) {
                d = *s1 - *s2;
                if (d)
                    return (d);
                s1++;
                s2++;
            }
            continue;
        }
        int d = *s1 - *s2;
        if (d)
            return (d);
        s1++;
        s2++;
    }
    return (*s1 - *s2);
}


// Sort a wordlist
//
void
wordlist::sort()
{
    wordlist *ww = this;
    int i;
    for (i = 0; ww; i++)
        ww = ww->wl_next;
    if (i < 2)
        return;
    char **stuff = new char*[i];
    for (i = 0, ww = this; ww; i++, ww = ww->wl_next)
        stuff[i] = ww->wl_word;
    qsort((char *) stuff, i, sizeof (char *), wlcomp);
    for (i = 0, ww = this; ww; i++, ww = ww->wl_next)
        ww->wl_word = stuff[i];
    delete [] stuff;
}


// Return a range of wordlist elements...
//
wordlist *
wordlist::range(int low, int up)
{
    const wordlist *thiswl = this;
    if (!thiswl)
        return (0);
    bool rev = false;
    if (low > up) {
        int i = up;
        up = low;
        low = i;
        rev = true;
    }
    up -= low;
    wordlist *wl = this, *tt;
    while (wl && (low > 0)) {
        tt = wl->wl_next;
        delete [] wl->wl_word;
        delete wl;
        wl = tt;
        if (wl)
            wl->wl_prev = 0;
        low--;
    }
    tt = wl; 
    while (tt && (up > 0)) {
        tt = tt->wl_next;
        up--; 
    } 
    if (tt && tt->wl_next) {
        tt->wl_next->free();
        tt->wl_next = 0;
    }
    if (rev)
        wl = wl->reverse();
    return (wl);
}

