
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

#ifndef FIO_CHD_CVTAB_H
#define FIO_CHD_CVTAB_H

#include "geo_alloc.h"


//
// A unified vectorized table for cell overlap areas and flattening
// transformation streams.  This is used with CHDs for accessing
// file data.
//
// The vectorization means that multiple "channels" coresponding to
// different regions of a cell hierarchy can be independently saved. 
// This is used in the "parallel" gridded writing function, each
// channel corresponds to a grid element.
//
// The architecture consists of a chd-keyed table of vectors of
// instance tables:
//
//   itable(chd) -> array[ix] -> itable(p)
//

struct chd_intab;

// Number of "local" instance use flags available in a 64-bit int.  The
// lsb bit is the "local" flag, all other bits are available.
//
#define CTAB_NUMLFLAGS (sizeof(unsigned long) - 1)

// Data item, bulk allocatd.  This contains:
//   - An effective bounding box, representing the area of the cell
//     needed to render the AOI.
//   - A set of instance-used flags, set if a subcell is needed to
//     render the AOI.
//   - A transformation list head and list length, for rendering.
//     This is a list of instances of the present master used in
//     the AOI.
//
struct cvtab_item_t : public cv_fgif
{
    // itable<T> requirements.  Note that this is bulk allocated and
    // there is no constructor or destructor.  Caller is responsible
    // for initialization and cleanup.

    unsigned long tab_key()         { return ((unsigned long)cvi_symref); }
    cvtab_item_t *tab_next()        { return (cvi_next); }
    void set_tab_next(cvtab_item_t *n)  { cvi_next = n; }
    cvtab_item_t *tgen_next(bool)   { return (cvi_next); }

    symref_t *symref()        const { return (cvi_symref); }

    void init(symref_t *p, int chd_tkt, const BBox *BB)
        {
            cvi_symref = p;
            cvi_next = 0;
            if (BB)
                cvi_BB = *BB;
            else if (p)
                cvi_BB = *p->get_bb();
            cvi_flags = 0;
            cvi_bytes_used = 0;
            cvi_ticket = 0;
            cvi_chd_tkt = chd_tkt;
            cvi_full_area = false;
            // For now, we always use flags.  These are needed for
            // empty cell filtering, whether or not an AOI is used.
            cvi_use_flags = true;
        }

    // Effective bounding box.
    const BBox *get_bb()      const { return (&cvi_BB); }
    void add_area(const BBox *BB)   { cvi_BB.add(BB); }

    // Instance flag setup and access, set for Physical data only,
    // but called for both Physical/Electrical so test the pointer!

    bool init_flags(const cCHD*, const symref_t*, bytefact_t*);

    void set_flag(int n)
        {
            if (cvi_use_flags) {
                if (cvi_flags & 0x1) {
                    cvi_flags |= (0x1L << (n+1));
                    return;
                }
                unsigned char *ptr = (unsigned char*)(cvi_flags & ~0x1L);
                if (ptr)
                    ptr[n >> 3] |= (0x1L << (n & 0x7));
            }
        }

    void unset_flag(int n)
        {
            if (cvi_use_flags) {
                if (cvi_flags & 0x1) {
                    cvi_flags &= ~(0x1L << (n+1));
                    return;
                }
                unsigned char *ptr = (unsigned char*)(cvi_flags & ~0x1L);
                if (ptr)
                    ptr[n >> 3] &= ~(0x1L << (n & 0x7));
            }
        }

    tristate_t get_flag(int n) const
        {
            if (!cvi_use_flags || !cvi_flags)
                return (ts_ambiguous);
            if (cvi_flags & 0x1)
                return ((cvi_flags & (0x1L << (n+1))) ? ts_set : ts_unset);
            unsigned char *ptr = (unsigned char*)(cvi_flags & ~0x1L);
            return ((ptr[n >> 3] & (0x1L << (n & 0x7))) ? ts_set : ts_unset);
        }

    void set_full_area(bool b)      { cvi_full_area = b; }
    bool get_full_area()      const { return (cvi_full_area); }

    int get_chd_tkt()         const { return (cvi_chd_tkt); }

    // Transform list manipulation.

    ticket_t ticket()         const { return (cvi_ticket); }
    void set_ticket(ticket_t t)     { cvi_ticket = t; }

    unsigned long bytes_used()        const { return (cvi_bytes_used); }
    void set_bytes_used(unsigned long u)    { cvi_bytes_used = u; }

    // Allocate transform stream memory.
    bool allocate_tstream(bytefact_t *bytefact)
        {
            if (cvi_bytes_used && !cvi_ticket) {
                cvi_bytes_used++; // Account for termination byte.
                cvi_ticket = bytefact->get_space_volatile(cvi_bytes_used);
                if (!cvi_ticket) {
                    alloc_failed(bytefact);
                    return (false);
                }
                cvi_bytes_used = 0;
            }
            return (true);
        }

    void clear_tstream()
        {
            cvi_bytes_used = 0;
            cvi_ticket = 0;
        }

    void print()
        {
            printf("  %-16s effBB= %d,%d %d,%d ts_bytes= %lu full= %d\n",
                Tstring(cvi_symref->get_name()),
                cvi_BB.left, cvi_BB.bottom, cvi_BB.right, cvi_BB.top,
                cvi_bytes_used, get_full_area());
        }

private:
    // The if_flags is either a local bit field for instance-use
    // flags, or the address of a larger bit field for the flags.  The
    // two lsbs are the "local" indication, and the "full_area" flag. 
    // The full_area flag can be used with the address, since the
    // alignment of the address precludes use of this bit, and we mask
    // it off.  The full_area flag is set if the full area of the cell
    // is within the search area, so we can avoid traversing its
    // hierarchy more than once.

    static void alloc_failed(bytefact_t*);

    symref_t *cvi_symref;           // symref_t pointer
    cvtab_item_t *cvi_next;         // link for table
    BBox cvi_BB;                    // effective bounding box
    unsigned long cvi_flags;        // instance use flags or address
    unsigned long cvi_bytes_used;   // count of bytes used
    ticket_t cvi_ticket;            // string ticket
    unsigned char cvi_chd_tkt;      // CHD ticket
    bool cvi_full_area;             // effective BB == real BB
    bool cvi_use_flags;             // use the instance use flag vector
};

typedef itable_t<cvtab_item_t> cvtab_t;


// Keep track of CHDs in use.  There is expected to be only a few CHDs
// in use (usually 1).  Each CHD has a small integer index.
//
struct chd_db_t
{
    chd_db_t()
        {
            cdb_num = 0;
            cdb_size = 10;
            cdb_array = new cCHD*[cdb_size];
        }

    ~chd_db_t()
        {
            delete [] cdb_array;
        }

    int add(cCHD *tchd)
        {
            for (unsigned int i = 0; i < cdb_num; i++) {
                if (tchd == cdb_array[i])
                    return (i);
            }
            if (cdb_num == cdb_size) {
                cdb_size += cdb_size;
                cCHD **tmp = new cCHD*[cdb_size];
                memcpy(tmp, cdb_array, cdb_num*sizeof(cCHD*));
                delete [] cdb_array;
                cdb_array = tmp;
            }
            cdb_array[cdb_num++] = tchd;
            return (cdb_num - 1);
        }

    int find(const cCHD *tchd) const
        {
            for (unsigned int i = 0; i < cdb_num; i++) {
                if (tchd == cdb_array[i])
                    return (i);
            }
            return (-1);
        }

    cCHD *chd(unsigned int i) const
        {
            if (i < cdb_num)
                return (cdb_array[i]);
            return (0);
        }

private:
    cCHD **cdb_array;
    unsigned int cdb_num;
    unsigned int cdb_size;
};

// Argument to cCVtab::prune_hier_rc.
namespace fio_chd_cvtab {
    struct ph_item_t;
}

// A vectored symref/cvtab_item_t table with local allocation.
//
// If true is passed to the constructor, unresolved cells that resolve
// as inline library cells will be opened in the database (if they
// aren't already).  This should only be done in electrical mode, or
// when we know scaling is unity.
//
class cCVtab : public cTfmStack, public cv_bbif
{
public:
    friend struct CVtabGen;

    cCVtab(bool, unsigned int);
    ~cCVtab();

    // BBs and instance use flags.
    bool build_BB_table(cCHD*, symref_t*, unsigned int, const BBox*,
        bool = false);

    // Transform lists, call after build_BB_table.
    bool build_TS_table(symref_t*, unsigned int, unsigned int);

    // Misc. functions, require that build_BB_table called first.
    OItype prune_empties(const symref_t*, chd_intab*, unsigned int);
    syrlist_t *symref_list(const cCHD*);
    syrlist_t *listing(unsigned int);
    unsigned int num_cells(unsigned int);

    unsigned int vec_size() const { return (ct_size); }

    cvtab_item_t *get(const symref_t *p, unsigned int ix) const
        {
            if (!p || ix >= ct_size)
                return (0);
            if (!ct_tables[ix])
                return (0);
            return (ct_tables[ix]->find(p));
        }

    cCHD *get_chd(int i) const
        {
            return (ct_chd_db.chd(i));
        }

    cvtab_item_t *resolve_item(symref_t **pp, unsigned int ix)
        {
            symref_t *p = *pp;
            if (!p->get_defseen()) {
                cvtab_item_t *item = get(p, ix);
                if (!item)
                    return (0);
                cCHD *tchd = ct_chd_db.chd(item->get_chd_tkt());

                symref_t *pt = resolve_symref(tchd, p);
                if (pt) {
                    p = pt;
                    *pp = pt;
                }
            }
            if (p->should_skip())
                return (0);
            return (get(p, ix));
        }

    // cv_bbif interface.
    const BBox *resolve_BB(symref_t **psymref, unsigned int ix)
        {
            const cvtab_item_t *item = resolve_item(psymref, ix);
            return (item ? item->get_bb() : 0);
        }

    const cv_fgif *resolve_fgif(symref_t **psymref, unsigned int ix)
        {
            return (resolve_item(psymref, ix));
        }

    // Obtain the transform stream from an item.
    unsigned char *get_tstream(cvtab_item_t *item)
        {
            if (item->ticket())
                return (ct_ts_bytefact.find(item->ticket()));
            return (0);
        }

    // Diagnostics.
    void dbg_print(const cCHD*, const symref_t*, unsigned int, bool);
    unsigned long memuse();

private:
    cvtab_item_t *add(cCHD*, symref_t*, unsigned int, const BBox* = 0);
    symref_t *resolve_symref(const cCHD*, const symref_t*, cCHD** = 0);
    OItype prune_empties_core(chd_intab*, unsigned int);
    OItype prune_empties_rc(cvtab_item_t*, fio_chd_cvtab::ph_item_t*);

    // BB table.
    bool build_BB_table_full_rc(cvtab_item_t*, unsigned int);
    bool build_BB_table_full_norc(cvtab_item_t*);
    bool build_BB_table_rc(cvtab_item_t*, unsigned int);
    bool build_BB_table_norc(cvtab_item_t*);

    // Transform lists.
    bool build_TS_table_rc(cvtab_item_t*, unsigned int, unsigned int = 0);

// Some per-channel flags
#define CVT_BB_DONE 0x1
#define CVT_TS_DONE 0x2
#define CVT_TS_CNT  0x4
#define CVT_NO_AOI  0x8

    bool bb_done(unsigned int ix)   { return (ct_flags[ix] & CVT_BB_DONE); }
        // True after build_BB_table called.
    bool ts_done(unsigned int ix)   { return (ct_flags[ix] & CVT_TS_DONE); }
        // True after build_TS_table called.
    bool ts_cnt(unsigned int ix)    { return (ct_flags[ix] & CVT_TS_CNT); }
        // True during internal first-pass of build_TS_table.
    bool no_aoi(unsigned int ix)    { return (ct_flags[ix] & CVT_NO_AOI); }
        // True if no constraint area.

    void set_bb_done(bool b, unsigned int ix)
        {
            if (b)
                ct_flags[ix] |= CVT_BB_DONE;
            else
                ct_flags[ix] &= ~CVT_BB_DONE;
        }

    void set_ts_done(bool b, unsigned int ix)
        {
            if (b)
                ct_flags[ix] |= CVT_TS_DONE;
            else
                ct_flags[ix] &= ~CVT_TS_DONE;
        }

    void set_ts_cnt(bool b, unsigned int ix)
        {
            if (b)
                ct_flags[ix] |= CVT_TS_CNT;
            else
                ct_flags[ix] &= ~CVT_TS_CNT;
        }

    void set_no_aoi(bool b, unsigned int ix)
        {
            if (b)
                ct_flags[ix] |= CVT_NO_AOI;
            else
                ct_flags[ix] &= ~CVT_NO_AOI;
        }

    BBox            ct_AOI;         // Present window area, if any.
    cvtab_t         **ct_tables;    // Vector of cvtab_item_t tables.
    unsigned char   *ct_flags;      // Array of flags.
    bool            ct_open;        // Open unresolved inline lib cells.

    unsigned int    ct_size;        // Vector size.
    unsigned int    ct_maxdepth;    // Hierarchy depth limit.

    chd_db_t        ct_chd_db;      // Keep track of CHDs
    tGEOfact<cvtab_item_t> ct_cvtab_allocator;
    bytefact_t      ct_bytefact;    // Allocator for instance flag arrays.
    bytefact_t      ct_ts_bytefact; // Transform string allocator.
};


// The generator, for traversal.
//
struct CVtabGen
{
    CVtabGen(const cCVtab*, unsigned int, const symref_t* = 0);
    ~CVtabGen() { delete [] cg_array; }

    cvtab_item_t *next()
        {
            if (cg_count < cg_size)
                return (cg_array[cg_count++]);
            return (0);
        }

private:
    cvtab_item_t **cg_array;        // Array for sorting.
    unsigned int cg_size;           // Used size of sorted array.
    unsigned int cg_count;          // Current output index.
};

#endif

