
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

#include "wlist.h"
#include <stdlib.h>


//
// Wordlist manipulation, non-inlined stuff.
//

namespace {
    // Sort so that strings containing an integer value will be sorted in
    // order of the value, if the alpha parts are the same.  This will put
    // x2 ahead of x10, for example,
    //
    static int wlcomp(const void *s, const void *t)
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
}


// Static function.
// Sort a wordlist.
//
void
wordlist::sort(wordlist *thiswl)
{
    wordlist *ww = thiswl;
    int i;
    for (i = 0; ww; i++)
        ww = ww->wl_next;
    if (i < 2)
        return;
    char **stuff = new char*[i];
    for (i = 0, ww = thiswl; ww; i++, ww = ww->wl_next)
        stuff[i] = ww->wl_word;
    qsort((char *) stuff, i, sizeof (char *), wlcomp);
    for (i = 0, ww = thiswl; ww; i++, ww = ww->wl_next)
        ww->wl_word = stuff[i];
    delete [] stuff;
}


// Static function.
// Return a range of wordlist elements...
//
wordlist *
wordlist::range(wordlist *thiswl, int low, int up)
{
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
    wordlist *wl = thiswl, *tt;
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
        destroy(tt->wl_next);
        tt->wl_next = 0;
    }
    if (rev)
        wl = reverse(wl);
    return (wl);
}

