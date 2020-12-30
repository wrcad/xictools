
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "fio.h"
#include "fio_crgen.h"
#include "fio_chd.h"


// Return a stream with tickets remapped to new values, used for
// file i/o.
//
unsigned char *
crgen_t::next_remap(ticket_t ntkt, tkt_map_t *map, SymTab *attab,
    unsigned int *slen, bool *chained)
{
    const unsigned char *str = ts_stream;
#ifdef WITH_CMPRS
    cmp_stream stbak = crg_cstream;
    crg_cstream.cs_stream = 0;
    crg_cstream.cs_size = 0;

    cmp_stream cs(0);
    if (ntkt) {
        cs.cs_ticket = ntkt;
        crg_cx_fct->find_space(&cs);
        str = cs.cs_stream;
    }
#else
    if (ntkt)
        str = crg_cx_fct->find_space(ntkt);
#endif

    const unsigned char *tmpstr = ts_stream;
    ts_stream = str;
    read_table();
    unsigned int tab_bytes = ts_offset;
    unsigned char *nstr = 0;

    ts_writer tsw(0, 0, true);  // First pass, count bytes only.
    for (int pass = 0; pass < 2; pass++) {
        for (;;) {
            unsigned char c = read_uchar();
            tsw.add_uchar(c);
            if (c == CREF_CHAIN) {
                tsw.add_uchar(read_uchar());
                tsw.add_uchar(read_uchar());
                tsw.add_uchar(read_uchar());
                tsw.add_uchar(read_uchar());
                if (chained)
                    *chained = true;
                break;
            }
            if (c == CREF_END) {
                if (chained)
                    *chained = false;
                break;
            }
            if (c & CREF_SPTR) {
                int n = read_signed();
                if (map) {
                    if (!map->map((ticket_t*)&n)) {
                        delete [] nstr;
                        nstr = 0;
                        goto done;
                    }
                }
                tsw.add_signed(n);
            }
            if (c & CREF_ATTR) {
                int n = read_signed();
                if (n >= 16 && attab) {
                    void *xx = SymTab::get(attab, n);
                    if (xx == ST_NIL) {
                        Errs()->add_error("remap: ticket not in table.");
                        delete [] nstr;
                        nstr = 0;
                        goto done;
                    }
                    n = (int)((intptr_t)xx - 1);
                }
                tsw.add_signed(n);
            }
            if (c & CREF_TX) {
                int n = read_signed();
                tsw.add_signed(n);
            }
            if (c & CREF_TY) {
                int n = read_signed();
                tsw.add_signed(n);
            }
        }
        if (pass == 0) {
            if (slen)
                *slen = tab_bytes + tsw.bytes_used();
            nstr = new unsigned char[tab_bytes + tsw.bytes_used()];
            memcpy(nstr, str, tab_bytes);
            clear();
            ts_stream = str;
            ts_offset = tab_bytes;
            tsw = ts_writer(nstr + tab_bytes, 0, false);
        }
    }

done:
    clear();
    crg_u.header_ptr = 0;
    crg_call_cnt = -1;
    crg_tkt = 0;
    ts_stream = tmpstr;
    ts_offset = 0;
#ifdef WITH_CMPRS
    if (cs.cs_stream && cs.cs_size)
        delete [] cs.cs_stream;
    crg_cstream = stbak;
#endif

    return (nstr);
}
// End of crgen_t functions.


// Build the table of integers, used to replace large integers with
// smaller 1 or 2 byte integers in the stream.  This is only needed if
// more than one cref.
//
SymTab *
cr_writer::create_table(symref_t *p, unsigned int *pcnt, unsigned *preccnt,
    ticket_t last_tkt)
{
    *pcnt = 0;
    *preccnt = 0;
    ticket_t tc = p->get_crefs();
    if (tc == 0)
        return (0);
    cref_t *cf = crw_factory->find_cref(tc);
    if (!cf)
        return (0);
    if (cf->last_cref()) {
        *preccnt = 1;
        return (0);
    }

    // List has more than one cref.

    SymTab *tab = new SymTab(false, false, 6);
    unsigned int tab_cnt = 1;

    // Use differences between parameters for successive crefs. 
    // These are usually smaller numbers, which saves space in
    // encoding.
    unsigned int rec_cnt = 0;
    int xmod = 0, ymod = 0;
    for (ticket_t t = tc; ; t++) {
        cref_t *c = crw_factory->find_cref(t);
        if (!c)
            break;

        // Ignore numbers that encode into one byte.  Each value
        // is added to the table with data 0.  If seen more than
        // once, the table data is changed to a 1-based index.

        int tx = c->pos_x() - xmod;
        xmod = c->pos_x();

        if (abs(tx) >= 64) {
            SymTabEnt *h = SymTab::get_ent(tab, (uintptr_t)tx);
            if (!h)
                tab->add((uintptr_t)tx, 0, false);
            else if (h->stData == 0)
                h->stData = (void*)(intptr_t)tab_cnt++;
        }

        int ty = c->pos_y() - ymod;
        ymod = c->pos_y();

        if (abs(ty) >= 64) {
            SymTabEnt *h = SymTab::get_ent(tab, (uintptr_t)ty);
            if (!h)
                tab->add((uintptr_t)ty, 0, false);
            else if (h->stData == 0)
                h->stData = (void*)(intptr_t)tab_cnt++;
        }

        if (c->last_cref())
            break;
        if (last_tkt && t == last_tkt)
            break;
        rec_cnt++;
    }
    *pcnt = tab_cnt - 1;
    *preccnt = rec_cnt;
    return (tab);
}


// This requires two calls, the first with count_only true.  In the
// first pass, the length of the stream is computed, and the memory
// allocated.  On the second pass, the data are written.  The ticket
// is returned on the first pass, 0 being an error.  On the second
// pass, the return is always 0.
//
ticket_t
cr_writer::build_list(symref_t *p, SymTab *tab, unsigned int num_values,
    ticket_t last_tkt, bool count_only)
{
    ticket_t tc = p->get_crefs();
    if (tc == 0)
        return (0);
    cref_t *cf = crw_factory->find_cref(tc);
    if (!cf)
        return (0);

    ticket_t tkt = 0;  // The return value.

    ts_cnt_only = count_only;
    if (count_only)
        ts_stream = 0;
    ts_used = 0;

    unsigned int rec_cnt = 0;
    if (!cf->last_cref()) {
        // List has more than one cref.

        // Create the values table.
        int *values_ary = num_values ? new int[num_values] : 0;

        if (values_ary) {
            SymTabGen gen(tab, false);
            SymTabEnt *h;
            while ((h = gen.next()) != 0) {
                unsigned int i = (unsigned int)(intptr_t)h->stData;
                if (i)
                    values_ary[i-1] = (int)(intptr_t)h->stTag;
            }
        }

        // Write table size and values if any.
        add_unsigned(num_values);
        for (unsigned int i = 0; i < num_values; i++)
            add_signed(values_ary[i]);
        delete [] values_ary;

        crw_cref.srfptr = 0;
        crw_cref.attr = 0;
        crw_cref.tx = 0;
        crw_cref.ty = 0;
        int xmod = 0;
        int ymod = 0;
        for (ticket_t t = tc; ; t++) {
            cref_t *c = crw_factory->find_cref(t);
            if (!c)
                break;

            // Get the header flags for this record, co will
            // contain record data.

            unsigned char flg = 0;
            cref_o_t co;
            co.flag = c->get_flg();
            co.srfptr = c->refptr();
            co.attr = c->get_tkt();

            int tx = c->pos_x() - xmod;
            xmod = c->pos_x();

            unsigned int indx =
                (unsigned int)(intptr_t)SymTab::get(tab, (uintptr_t)tx);
            if (!indx || indx == (unsigned int)(intptr_t)ST_NIL)
                co.tx = tx;
            else {
                co.tx = indx-1;
                flg |= CREF_TXi;
            }

            int ty = c->pos_y() - ymod;
            ymod = c->pos_y();

            indx = (unsigned int)(intptr_t)SymTab::get(tab, (uintptr_t)ty);
            if (!indx || indx == (unsigned int)(intptr_t)ST_NIL)
                co.ty = ty;
            else {
                co.ty = indx-1;
                flg |= CREF_TYi;
            }

            // Set the modality flags, and write the non-repeating
            // values.

            if (crw_cref.srfptr != co.srfptr)
                flg |= CREF_SPTR;
            if (crw_cref.attr != co.attr)
                flg |= CREF_ATTR;
            if (crw_cref.tx != co.tx)
                flg |= CREF_TX;
            if (crw_cref.ty != co.ty)
                flg |= CREF_TY;
            if (co.flag)
                flg |= CREF_FLAG;
            add_uchar(flg);

            if (crw_cref.srfptr != co.srfptr) {
                add_signed(co.srfptr);
                crw_cref.srfptr = co.srfptr;
            }
            if (crw_cref.attr != co.attr) {
                add_signed(co.attr);
                crw_cref.attr = co.attr;
            }
            if (crw_cref.tx != co.tx) {
                add_signed(co.tx);
                crw_cref.tx = co.tx;
            }
            if (crw_cref.ty != co.ty) {
                add_signed(co.ty);
                crw_cref.ty = co.ty;
            }

            if (c->last_cref())
                break;
            if (last_tkt && t == last_tkt)
                break;
            rec_cnt++;
        }
    }
    else {
        // 1 cref

        add_unsigned(0);

        crw_cref.srfptr = 0;
        crw_cref.attr = 0;
        crw_cref.tx = 0;
        crw_cref.ty = 0;

        unsigned int attr = cf->get_tkt();
        unsigned char flg = 0;
        if (crw_cref.srfptr != cf->refptr())
            flg |= CREF_SPTR;
        if (crw_cref.attr != attr)
            flg |= CREF_ATTR;
        if (crw_cref.tx != cf->pos_x())
            flg |= CREF_TX;
        if (crw_cref.ty != cf->pos_y())
            flg |= CREF_TY;
        if (cf->get_flg())
            flg |= CREF_FLAG;
        add_uchar(flg);

        if (crw_cref.srfptr != cf->refptr()) {
            add_signed(cf->refptr());
            crw_cref.srfptr = cf->refptr();
        }
        if (crw_cref.attr != attr) {
            add_signed(attr);
            crw_cref.attr = attr;
        }
        if (crw_cref.tx != cf->pos_x()) {
            add_signed(cf->pos_x());
            crw_cref.tx = cf->pos_x();
        }
        if (crw_cref.ty != cf->pos_y()) {
            add_signed(cf->pos_y());
            crw_cref.ty = cf->pos_y();
        }
        rec_cnt++;
    }
    cref_t *c = crw_factory->find_cref(tc + rec_cnt);
    if (!c || c->last_cref())
        add_uchar(CREF_END);
    else {
        add_uchar(CREF_CHAIN);
        add_uchar(0x0);
        add_uchar(0x0);
        add_uchar(0x0);
        add_uchar(0x0);
    }

    if (count_only) {
        // Failure reported by caller.
        tkt = crw_factory->get_space(ts_used + 1, rec_cnt);
        if (!tkt)
            return (0);
        ts_stream = crw_factory->find_space(tkt);
    }
    else
        ts_stream = 0;
    ts_length = 0;

    return (tkt);
}
// End of cr_writer functions.

