
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

#ifndef FIO_CRSTREAM_H
#define FIO_CRSTREAM_H

#include "fio_tstream.h"
#include "fio_cxfact.h"


#define CREF_SPTR   0x1
#define CREF_ATTR   0x2
#define CREF_TX     0x4
#define CREF_TY     0x8
#define CREF_TXi    0x10
#define CREF_TYi    0x20
#define CREF_FLAG   0x40
#define CREF_END    0x80
#define CREF_CHAIN  0xff

// Writer for compressed cref stream.
//
struct cr_writer : public ts_writer
{
    cr_writer(cxfact_t *cf) : ts_writer(0, 0, false) { crw_factory = cf; }

    void set(unsigned char *s) { ts_stream = s; }

    SymTab *create_table(symref_t*, unsigned int*, unsigned int*, ticket_t);
    ticket_t build_list(symref_t*, SymTab*, unsigned int, ticket_t, bool);

private:
    cxfact_t    *crw_factory;
    cref_o_t    crw_cref;
};

struct tkt_map_t;

// Generator for cref_t lists.  This transparently handles the case
// where the cref list is compressed, in which case the return from
// next is volatile.
//
struct crgen_t : public ts_reader
{
    crgen_t(cxfact_t *fct, const symref_t *p) : ts_reader(0)
        {
            crg_cx_fct = fct;
            crg_symref = p;
            crg_compressed = !p || p->get_cmpr();
#ifdef WITH_CMPRS
            crg_dirty = false;
#endif
            crg_call_cnt = -1;

            crr_value_table = 0;
            crr_last_tx = 0;
            crr_last_ty = 0;
            crr_table_read = false;

            reset();
        }

    ~crgen_t()
        {
            delete [] crr_value_table;
#ifdef WITH_CMPRS
            if (crg_cstream.cs_size) {
                if (crg_dirty)
                    crg_cx_fct->update(&crg_cstream);
                delete [] crg_cstream.cs_stream;
            }
#endif
        }

    const cref_o_t *next(ticket_t *next_tkt = 0)
        {
            crg_call_cnt++;
            if (crg_compressed)
                return (read_record(next_tkt));
            else {
                crg_u.cur_cref = crg_cx_fct->find_cref(crg_tkt);
                if (!crg_u.cur_cref || crg_u.cur_cref->last_cref())
                    crg_tkt = 0;
                else
                    crg_tkt++;
                return (crg_u.cur_cref ? read_cref(crg_u.cur_cref) : 0);
            }
        }

#ifdef notused
    // Presently not used.
    // There is provision for setting a flag for each instance.  This
    // does not consume space in the instance lists.
    //
    void set_flag(bool f)
        {
            if (crg_compressed) {
                if (crg_u.header_ptr) {
                    if (f)
                        *crg_u.header_ptr |= CREF_FLAG;
                    else
                        *crg_u.header_ptr &= ~CREF_FLAG;
#ifdef WITH_CMPRS
                    crg_dirty = true;
#endif
                }
            }
            else if (crg_u.cur_cref)
                crg_u.cur_cref->set_flg(f);
        }
#endif

    unsigned char *next_remap(ticket_t, tkt_map_t*, SymTab*, unsigned int*,
        bool*);

    void set(const unsigned char *s) { ts_stream = s; }

    void clear()
        {
            crr_cref.srfptr = 0;
            crr_cref.attr = 0;
            crr_cref.tx = 0;
            crr_cref.ty = 0;
            crr_cref.flag = false;
            delete [] crr_value_table;
            crr_value_table = 0;
#ifdef WITH_CMPRS
            if (crg_cstream.cs_size) {
                if (crg_dirty)
                    crg_cx_fct->update(&crg_cstream);
                delete [] crg_cstream.cs_stream;
            }
            crg_cstream.cs_stream = 0;
            crg_cstream.cs_size = 0;
            crg_cstream.cs_ticket = 0;
            crg_dirty = false;
#endif
            crr_last_tx = 0;
            crr_last_ty = 0;
            crr_table_read = false;
        }

    void reset()
        {
            clear();
            crg_u.header_ptr = 0;
            crg_call_cnt = -1;
            if (crg_compressed) {
#ifdef WITH_CMPRS
                if (crg_symref) {
                    crg_cstream.cs_ticket = crg_symref->get_crefs();
                    crg_cx_fct->find_space(&crg_cstream);
                }
                ts_stream = crg_cstream.cs_stream;
#else
                ts_stream = crg_symref ?
                    crg_cx_fct->find_space(crg_symref->get_crefs()) : 0;
#endif
                ts_offset = 0;
                crg_tkt = 0;
            }
            else
                crg_tkt = crg_symref->get_crefs();
        }

    const cref_o_t *read_cref(cref_t *c)
        {
            crr_cref.set(c);
            return (&crr_cref);
        }

    void read_table()
        {
            unsigned int nv = read_unsigned();
            if (nv) {
                crr_value_table = new int[nv];
                for (unsigned int i = 0; i < nv; i++)
                    crr_value_table[i] = read_signed();
            }
            crr_table_read = true;
        }

    const cref_o_t *read_record(ticket_t *next_tkt = 0)
        {
            if (!crr_table_read)
                read_table();
            crg_u.header_ptr = (unsigned char*)ts_stream + ts_offset;
            unsigned char c = read_uchar();
            if (c == CREF_CHAIN) {
                if (!crg_cx_fct)
                    return (0);
                ticket_t tkt = 0;
                tkt |= read_uchar();
                tkt |= (read_uchar() << 8);
                tkt |= (read_uchar() << 16);
                tkt |= (read_uchar() << 24);
                clear();
                ts_offset = 0;
                if (next_tkt) {
                    // If next_tkt is passed, stop and return ticket.
                    *next_tkt = tkt;
                    return (0);
                }
#ifdef WITH_CMPRS
                crg_cstream.cs_ticket = tkt;
                crg_cx_fct->find_space(&crg_cstream);
                ts_stream = crg_cstream.cs_stream;
#else
                ts_stream = crg_cx_fct->find_space(tkt);
#endif
                if (!ts_stream)
                    return (0);
                return (read_record());
            }

            if (c == CREF_END)
                return (0);
            crr_cref.flag = (c & CREF_FLAG);
            if (c & CREF_SPTR)
                crr_cref.srfptr = read_signed();

            if (c & CREF_ATTR)
                crr_cref.attr = read_signed();

            if (c & CREF_TX)
                crr_last_tx = read_signed();
            if (c & CREF_TXi)
                crr_cref.tx += crr_value_table[crr_last_tx];
            else
                crr_cref.tx += crr_last_tx;

            if (c & CREF_TY)
                crr_last_ty = read_signed();
            if (c & CREF_TYi)
                crr_cref.ty += crr_value_table[crr_last_ty];
            else
                crr_cref.ty += crr_last_ty;
            return (&crr_cref);
        }

    int call_count() const { return (crg_call_cnt); }

private:
    cxfact_t        *crg_cx_fct;
    const symref_t  *crg_symref;
    union           { unsigned char *header_ptr; cref_t *cur_cref; } crg_u;
    ticket_t        crg_tkt;
    int             crg_call_cnt;
    bool            crg_compressed;
#ifdef WITH_CMPRS
    bool crg_dirty;
#endif
    bool            crr_table_read;

    cref_o_t        crr_cref;
    int             *crr_value_table;
#ifdef WITH_CMPRS
    cmp_stream      crg_cstream;
#endif
    int             crr_last_tx;
    int             crr_last_ty;
};

#endif

