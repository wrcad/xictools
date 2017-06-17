
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2015 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: geo_intdb.h,v 1.1 2015/10/07 04:07:33 stevew Exp $
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

