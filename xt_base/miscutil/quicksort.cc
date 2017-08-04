
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "quicksort.h"


// The following was lifted from FreeBSD 5.3.  The FreeBSD qsort is
// faster than others, *much* faster than RHEL-3 Linux, particularly
// in 64bit EM64T mode.

namespace {
    inline char *med3(char *, char *, char *, cmp_t *);
    inline void  swapfunc(char *, char *, int, int);
}

#define qsmin(a, b)   (a) < (b) ? a : b

//
// Qsort routine from Bentley & McIlroy's "Engineering a Sort Function".
//
#define swapcode(TYPE, parmi, parmj, n) {   \
    long i = (n) / sizeof (TYPE);           \
    TYPE *pi = (TYPE *) (parmi);            \
    TYPE *pj = (TYPE *) (parmj);            \
    do {                                    \
        TYPE    t = *pi;                    \
        *pi++ = *pj;                        \
        *pj++ = t;                          \
        } while (--i > 0);                  \
}

#define SWAPINIT(a, es) swaptype = ((char *)a - (char *)0) % sizeof(long) || \
    es % sizeof(long) ? 2 : es == sizeof(long)? 0 : 1;

namespace {
    inline void
    swapfunc(char *a, char *b, int n, int swaptype)
    {
        if(swaptype <= 1)
            swapcode(long, a, b, n)
        else
            swapcode(char, a, b, n)
    }
}

#define swap(a, b)                          \
    if (swaptype == 0) {                    \
        long t = *(long *)(a);              \
        *(long *)(a) = *(long *)(b);        \
        *(long *)(b) = t;                   \
    } else                                  \
        swapfunc(a, b, es, swaptype)

#define vecswap(a, b, n)    if ((n) > 0) swapfunc(a, b, n, swaptype)

#define CMP(x, y) (cmp((x), (y)))

namespace {
    inline char *
    med3(char *a, char *b, char *c, cmp_t *cmp)
    {
        return CMP(a, b) < 0 ?
            (CMP(b, c) < 0 ? b : (CMP(a, c) < 0 ? c : a ))
                :(CMP(b, c) > 0 ? b : (CMP(a, c) < 0 ? a : c ));
    }
}

void
quicksort::qsort(void *a, size_t n, size_t es, cmp_t *cmp)
{
    char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
    int d, r, swaptype, swap_cnt;

loop:   SWAPINIT(a, es);
    swap_cnt = 0;
    if (n < 7) {
        for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
            for (pl = pm;
                 pl > (char *)a && CMP(pl - es, pl) > 0;
                 pl -= es)
                swap(pl, pl - es);
        return;
    }
    pm = (char *)a + (n / 2) * es;
    if (n > 7) {
        pl = (char*)a;
        pn = (char *)a + (n - 1) * es;
        if (n > 40) {
            d = (n / 8) * es;
            pl = med3(pl, pl + d, pl + 2 * d, cmp);
            pm = med3(pm - d, pm, pm + d, cmp);
            pn = med3(pn - 2 * d, pn - d, pn, cmp);
        }
        pm = med3(pl, pm, pn, cmp);
    }
    swap((char*)a, pm);
    pa = pb = (char *)a + es;


    pc = pd = (char *)a + (n - 1) * es;
    for (;;) {
        while (pb <= pc && (r = CMP(pb, a)) <= 0) {
            if (r == 0) {
                swap_cnt = 1;
                swap(pa, pb);
                pa += es;
            }
            pb += es;
        }
        while (pb <= pc && (r = CMP(pc, a)) >= 0) {
            if (r == 0) {
                swap_cnt = 1;
                swap(pc, pd);
                pd -= es;
            }
            pc -= es;
        }
        if (pb > pc)
            break;
        swap(pb, pc);
        swap_cnt = 1;
        pb += es;
        pc -= es;
    }
    if (swap_cnt == 0) {  // Switch to insertion sort
        for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
            for (pl = pm;
                 pl > (char *)a && CMP(pl - es, pl) > 0;
                 pl -= es)
                swap(pl, pl - es);
        return;
    }

    pn = (char *)a + n * es;
    r = qsmin(pa - (char *)a, pb - pa);
    vecswap((char*)a, pb - r, r);
    r = qsmin(pd - pc, pn - pd - (long)es);
    vecswap(pb, pn - r, r);
    if ((r = pb - pa) > (long)es)
        qsort(a, r / es, es, cmp);
    if ((r = pd - pc) > (long)es) {
        // Iterate rather than recurse to save stack space
        a = pn - r;
        n = r / es;
        goto loop;
    }
}


namespace { inline char  *med3_r(char *, char *, char *, cmp_r_t *, void *); }

#define CMP_R(t, x, y) (cmp((t), (x), (y)))

namespace {
    inline char *
    med3_r(char *a, char *b, char *c, cmp_r_t *cmp, void *thunk)
    {
        return CMP_R(thunk, a, b) < 0 ?
            (CMP_R(thunk, b, c) < 0 ? b : (CMP_R(thunk, a, c) < 0 ? c : a ))
              :(CMP_R(thunk, b, c) > 0 ? b : (CMP_R(thunk, a, c) < 0 ? a : c ));
    }
}

void
quicksort::qsort_r(void *a, size_t n, size_t es, void *thunk, cmp_r_t *cmp)
{
    char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
    int d, r, swaptype, swap_cnt;

loop:   SWAPINIT(a, es);
    swap_cnt = 0;
    if (n < 7) {
        for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
            for (pl = pm;
                 pl > (char *)a && CMP_R(thunk, pl - es, pl) > 0;
                 pl -= es)
                swap(pl, pl - es);
        return;
    }
    pm = (char *)a + (n / 2) * es;
    if (n > 7) {
        pl = (char*)a;
        pn = (char *)a + (n - 1) * es;
        if (n > 40) {
            d = (n / 8) * es;
            pl = med3_r(pl, pl + d, pl + 2 * d, cmp, thunk);
            pm = med3_r(pm - d, pm, pm + d, cmp, thunk);
            pn = med3_r(pn - 2 * d, pn - d, pn, cmp, thunk);
        }
        pm = med3_r(pl, pm, pn, cmp, thunk);
    }
    swap((char*)a, pm);
    pa = pb = (char *)a + es;


    pc = pd = (char *)a + (n - 1) * es;
    for (;;) {
        while (pb <= pc && (r = CMP_R(thunk, pb, a)) <= 0) {
            if (r == 0) {
                swap_cnt = 1;
                swap(pa, pb);
                pa += es;
            }
            pb += es;
        }
        while (pb <= pc && (r = CMP_R(thunk, pc, a)) >= 0) {
            if (r == 0) {
                swap_cnt = 1;
                swap(pc, pd);
                pd -= es;
            }
            pc -= es;
        }
        if (pb > pc)
            break;
        swap(pb, pc);
        swap_cnt = 1;
        pb += es;
        pc -= es;
    }
    if (swap_cnt == 0) {  // Switch to insertion sort
        for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
            for (pl = pm;
                 pl > (char *)a && CMP_R(thunk, pl - es, pl) > 0;
                 pl -= es)
                swap(pl, pl - es);
        return;
    }

    pn = (char *)a + n * es;
    r = qsmin(pa - (char *)a, pb - pa);
    vecswap((char*)a, pb - r, r);
    r = qsmin(pd - pc, pn - pd - (long)es);
    vecswap(pb, pn - r, r);
    if ((r = pb - pa) > (long)es)
        qsort_r(a, r / es, es, thunk, cmp);
    if ((r = pd - pc) > (long)es) {
        // Iterate rather than recurse to save stack space
        a = pn - r;
        n = r / es;
        goto loop;
    }
}

