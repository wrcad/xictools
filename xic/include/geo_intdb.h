
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef GEO_INTDB_H
#define GEO_INTDB_H

#include "symtab.h"
#include <algorithm>

//
// An integer database.  Call add with arbitrary integer values.  Call
// list for a sorted array of the unique values.
//

#define IDB_HEADS 256

struct intDb
{
    struct ilst
    {
        ilst *next;
        long value;
    };

    intDb()     { memset(idb_heads, 0, IDB_HEADS*sizeof(ilst*)); }

    void add(int y)
        {
            int ym = y & (IDB_HEADS-1);
            ilst *h = idb_heads[ym];
            if (!h || y < h->value) {
                h = idb_factory.new_element();
                h->next = idb_heads[ym];
                h->value = y;
                idb_heads[ym] = h;
                return;
            }
            for ( ; h; h = h->next) {
                if (y == h->value)
                    return;
                if (!h->next || y < h->next->value) {
                    ilst *hx = idb_factory.new_element();
                    hx->next = h->next;
                    hx->value = y;
                    h->next = hx;
                    return;
                }
            }
        }

    void list(int **parray, int *psize)
        {
            int cnt = 0;
            for (int i = 0; i < IDB_HEADS; i++) {
                for (ilst *h = idb_heads[i]; h; h = h->next)
                    cnt++;
            }
            if (!cnt) {
                *parray = 0;
                *psize = 0;
                return;
            }
            int *ary = new int[cnt];
            cnt = 0;
            for (int i = 0; i < IDB_HEADS; i++) {
                for (ilst *h = idb_heads[i]; h; h = h->next)
                    ary[cnt++] = h->value;
            }
            std::sort(ary, ary+cnt);
            *parray = ary;
            *psize = cnt;
        }

protected:
    ilst *idb_heads[IDB_HEADS];

    eltab_t<ilst> idb_factory;
};

#endif

