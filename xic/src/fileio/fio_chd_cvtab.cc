
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
#include "fio_chd.h"
#include "fio_cvt_base.h"
#include "fio_chd_cvtab.h"
#include "miscutil/timedbg.h"
#include <algorithm>


//
// A unified vectorized table for cell overlap areas and flattening
// transformation streams.  This is used with CHDs for accessing file
// data.
//
// The tables will contain symrefs used in a windowed (optional)
// hierarchy, which may include symrefs from multiple CHDs.  This is
// used when processing an area-constrained hierarchy.  The
// vectorization is useful for keeping track of multiple windowing
// areas.
//
// The symref data may also include a compressed instance
// transformation list string.  This is used when flattening, each
// used symref has an associated list of transformations.
//

cCVtab::cCVtab(bool b, unsigned int vsize)
{
    if (vsize < 1)
        vsize = 1;

    ct_tables = new cvtab_t*[vsize];
    memset(ct_tables, 0, vsize*sizeof(cvtab_t*));
    ct_flags = new unsigned char[vsize];
    memset(ct_flags, 0, vsize);
    ct_open = b;

    ct_size = vsize;
    ct_maxdepth = CDMAXCALLDEPTH;

    CD()->RegisterCreate("cCVtab");
}


cCVtab::~cCVtab()
{
    for (unsigned int i = 0; i < ct_size; i++)
        delete ct_tables[i];
    delete [] ct_tables;
    delete [] ct_flags;

    CD()->RegisterDestroy("cCVtab");
}


// Recursively add to table the "active" elements, i.e., those with CVok
// and passing area test if area configuration was applied.  If cBB is
// given, an "area used" bounding box is saved with the entry.  If cBB
// is nil, no data are saved with the entry.
//
// This recurses through the entire hierarchy, resolving "unseen"
// symrefs through the library mechanism.
//
// The windowing is available only in physical mode.
//
// This can be called more than once, previous state will be cleared.
//
// This all applies to the given vector index.
//
bool
cCVtab::build_BB_table(cCHD *chd, symref_t *p, unsigned int ix,
    const BBox *BB, bool norecurse)
{
    TimeDbg tdbg("cell_table_build");

    if (!chd || !p) {
        Errs()->add_error("build_BB_table: unexpected null argument.");
        return (false);
    }
    if (ix >= ct_size) {
        Errs()->add_error("build_BB_table: index out of range.");
        return (false);
    }
    if (!p->get_defseen()) {
        Errs()->add_error(
            "build_BB_table: top-level symref %s not resolved in CHD.",
            Tstring(p->get_name()));
        return (false);
    }
    if (BB && p->get_elec()) {
        Errs()->add_error(
            "build_BB_table: windowing available in physical mode only.");
        return (false);
    }

    set_bb_done(false, ix);
    set_ts_done(false, ix);

    // Check for any existing data in this channel, clear if so.  Note
    // that no factory memory is freed if there are multiple channels,
    // so reusing channels is not a good thing to do in this case.
    //
    if (ct_tables[ix]) {
        delete ct_tables[ix];
        ct_tables[ix] = 0;
        if (ct_size == 1) {
            // This is the only channel, so clear the factories.
            ct_cvtab_allocator.clear();
            ct_bytefact.clear();
            ct_ts_bytefact.clear();
        }
    }
    if (p->should_skip()) {
        set_bb_done(true, ix);
        return (true);
    }

    if (!BB) {
        set_no_aoi(true, ix);
        cvtab_item_t *item = add(chd, p, ix);
        if (norecurse)
            return (build_BB_table_full_norc(item));
        bool ret = build_BB_table_full_rc(item, ix);
        if (ret)
            set_bb_done(true, ix);
        return (ret);
    }

    set_no_aoi(false, ix);
    ct_AOI = *BB;
    BBox sBB = *p->get_bb();
    BBox iBB = ct_AOI;
    TInverse();
    if (!TInvBBcheck(&iBB, &sBB)) {
        set_bb_done(true, ix);
        return (true);
    }

    if (sBB.left < iBB.left)
        sBB.left = iBB.left;
    if (sBB.bottom < iBB.bottom)
        sBB.bottom = iBB.bottom;
    if (sBB.right > iBB.right)
        sBB.right = iBB.right;
    if (sBB.top > iBB.top)
        sBB.top = iBB.top;

    cvtab_item_t *item = add(chd, p, ix, &sBB);
    if (!item)
        return (false);

    if (norecurse)
        return (build_BB_table_norc(item));

    bool ret = build_BB_table_rc(item, ix);
    if (ret)
        set_bb_done(true, ix);
    return (ret);
}


// Build the transformation list table, used when flattening.  This
// must be called after build_BB_table.
//
bool
cCVtab::build_TS_table(symref_t *p, unsigned int ix, unsigned int maxdepth)
{
    TimeDbg tdbg("inst_tfm_table_build");

    if (!p) {
        Errs()->add_error("build_TS_table: null symref argument.");
        return (false);
    }
    if (ix >= ct_size) {
        Errs()->add_error("build_TS_table: index out of range.");
        return (false);
    }
    if (!bb_done(ix)) {
        Errs()->add_error("build_TS_table: BB table not built.");
        return (false);
    }
    if (!p->get_defseen()) {
        Errs()->add_error(
            "build_TS_table: top-level symref %s not resolved in CHD.",
            Tstring(p->get_name()));
        return (false);
    }
    if (p->get_elec()) {
        Errs()->add_error(
            "build_TS_table: flattening available in physical mode only.");
        return (false);
    }
    if (p->should_skip()) {
        set_ts_done(true, ix);
        return (true);
    }

    if (ts_done(ix)) {
        // Zero existing tstream data.  This will actually clear the
        // existing tstream database only if there is one channel. 
        // With multiple channels, reuse of channels is not a good
        // idea.

        tgen_t<cvtab_item_t> gen(ct_tables[ix]);
        cvtab_item_t *item;
        while ((item = gen.next()) != 0)
            item->clear_tstream();

        if (ct_size == 1)
            ct_ts_bytefact.clear();
        set_ts_done(false, ix);
    }

    ct_maxdepth = maxdepth;
    set_ts_cnt(true, ix);

    // Save byte count for identity transform in top-level cell.

    cvtab_item_t *item = ct_tables[ix] ? ct_tables[ix]->find(p) : 0;
    if (!item) {
        Errs()->add_error("build_TS_table: top symref not found in CHD.");
        return (false);
    }

    // When flattening, the I2 transform register is applied to the
    // top of the transform stack, and so will be applied to the
    // output objects.  This enables setting up a pseudo "instance"
    // transformation, which might be useful.
    //
    TPush();
    TLoad(CDtfRegI2);

    item->set_ticket(0);
    ts_writer tsw(0, 0, true);
    CDtx tx;
    TCurrent(&tx);
    tsw.add_record(&tx, 1, 1, 0, 0);
    item->set_bytes_used(tsw.bytes_used());

    // Get the counts for the rest of the hierarchy.
    if (ct_maxdepth > 0 && !build_TS_table_rc(item, ix)) {
        TPop();
        return (false);
    }

    set_ts_cnt(false, ix);

    if (!item->allocate_tstream(&ct_ts_bytefact)) {
        TPop();
        return (false);
    }

    // Load the identity transform.
    unsigned char *str = ct_ts_bytefact.find(item->ticket());
    if (str) {
        tsw = ts_writer(str, 0, false);
        tx = CDtx();
        TCurrent(&tx);
        tsw.add_record(&tx, 1, 1, 0, 0);
    }

    // Load the rest of the transforms.
    if (ct_maxdepth > 0 && !build_TS_table_rc(item, ix)) {
        TPop();
        return (false);
    }

    set_ts_done(true, ix);
    TPop();
    return (true);
}


namespace fio_chd_cvtab {
    // State for prune_empties_rc.
    //
    struct ph_item_t
    {
        ph_item_t(chd_intab *it, unsigned int ix)
            {
                ph_itab = it;
                ph_done_tab = new ptrtab_t;
                ph_index = ix;
                ph_empty_cnt = 0;
            }

        ~ph_item_t()
            {
                delete ph_done_tab;
            }

        chd_intab *itab()           { return (ph_itab); }
        unsigned int index()        { return (ph_index); }
        void add(symref_t *p)       { ph_done_tab->add(p); }
        bool find(symref_t *p)      { return (ph_done_tab->find(p)); }
        void increment()            { ph_empty_cnt++; }
        unsigned int empty_cnt()    { return (ph_empty_cnt); }

    private:
        chd_intab *ph_itab;
        ptrtab_t *ph_done_tab;
        unsigned int ph_index;
        unsigned int ph_empty_cnt;
    };
}


// Traverse the table, and remove entries for empty cells.  Return
// values:  OIok, OIerror, OIabort.
//
OItype
cCVtab::prune_empties(const symref_t *ptop, chd_intab *itab, unsigned int ix)
{
    TimeDbg tdbg("prune_empties");

    if (!itab)
        return (OIerror);
    if (ix >= ct_size) {
        Errs()->add_error("prune_empties: index out of range.");
        return (OIerror);
    }

    bool in_gzipped = false;
    SymTabGen tgen(itab);
    SymTabEnt *h;
    while ((h = tgen.next()) != 0) {
        cv_in *in = (cv_in*)h->stData;
        if (in->gzipped()) {
            in_gzipped = true;
            break;
        }
    }
    if (in_gzipped) {
        OItype oiret = prune_empties_core(itab, ix);
        if (oiret != OIok)
            return (oiret);
    }
    else {
        // Similar to prune_empties_core, but requires only one pass
        // through the hierarchy.  However, use is limited since
        // access to the file is not sorted by offset, which can be
        // *very* slow for gzipped files.  It may be a little faster
        // than prune_empties_core for normal files.

        fio_chd_cvtab::ph_item_t ph(itab, ix);
        cvtab_item_t *pitem = get(ptop, ix);
        if (!pitem) {
            Errs()->add_error("prune_empties: top symref not in table.");
            return (OIerror);
        }
        OItype oiret = prune_empties_rc(pitem, &ph);
        if (oiret != OIok)
            return (oiret);
        Tdbg()->save_message("pruned %u", ph.empty_cnt());
    }

#ifdef notdef
// No, we keep them now, so this should not be needed.
    // Do another pass to ensure that missing symrefs are not referenced
    // in the instance lists.
    //
    CVtabGen gen(this, ix);
    cCHD *tchd;
    symref_t *p;
    cvtab_item_t *item;
    while ((item = gen.next()) != 0) {
        p = item->symref();
        if (p->should_skip())
            continue;

        tchd = ct_chd_db.chd(item->get_chd_tkt());
        nametab_t *ntab = tchd->nameTab(p->mode());
        crgen_t cgen(ntab, p);
        const cref_o_t *c;
        while ((c = cgen.next()) != 0) {
            symref_t *cp = ntab->find_symref(c->srfptr);
            if (!cp) {
                Errs()->add_error(
                    "prune_empties: unresolved instance symref back-pointer.");
                return (OIerror);
            }

            if (!cp->get_defseen()) {
                symref_t *tcp = resolve_symref(tchd, cp);
                if (tcp)
                    cp = tcp;
            }
            if (cp->should_skip())
                continue;
            if (!get(cp, ix))
                item->unset_flag(cgen.call_count());
        }
    }
#endif

    return (OIok);
}


namespace {
    // Sort comparison, descending in offset.
    //
    inline bool
    symcmp(const symref_t *s1, const symref_t *s2)
    {
        int64_t n1 = s1->get_offset();
        int64_t n2 = s2->get_offset();
        return (n1 > n2);
    }
}


// Return a list of the symrefs used for any index, sorted in
// ascending order in the file offset.
//
syrlist_t *
cCVtab::symref_list(const cCHD *chd)
{
    if (!chd)
        return (0);
    int chd_ix = ct_chd_db.find(chd);
    if (chd_ix < 0)
        return (0);
    ptrtab_t tab;
    for (unsigned int i = 0; i < ct_size; i++) {
        if (!ct_tables[i])
            continue;
        tgen_t<cvtab_item_t> gen(ct_tables[i]);
        cvtab_item_t *item;
        while ((item = gen.next()) != 0) {
            if (item->get_chd_tkt() == chd_ix)
                tab.add((void*)item->tab_key());
        }
    }
    symref_t **ary = new symref_t*[tab.allocated()];
    ptrgen_t gen(&tab);
    symref_t *px;
    int cnt = 0;
    while ((px = (symref_t*)gen.next()) != 0)
        ary[cnt++] = px;
    std::sort(ary, ary + cnt, symcmp);

    syrlist_t *s0 = 0;
    for (int i = 0; i < cnt; i++)
        s0 = new syrlist_t(ary[i], s0);
    delete [] ary;
    return (s0);
}


// Return a list of the symrefs for the passed index.
//
syrlist_t *
cCVtab::listing(unsigned int ix)
{
    if (ix >= ct_size)
        return (0);
    if (!ct_tables[ix])
        return (0);
    syrlist_t *s0 = 0;
    tgen_t<cvtab_item_t> gen(ct_tables[ix]);
    cvtab_item_t *item;
    while ((item = gen.next()) != 0) {
        symref_t *p = (symref_t*)item->tab_key();
        s0 = new syrlist_t(p, s0);
    }
    return (s0);
}


// Return a count of the cells in the ix channel.
//
unsigned int
cCVtab::num_cells(unsigned int ix)
{
    if (ix >= ct_size)
        return (0);
    if (!ct_tables[ix])
        return (0);
    return (ct_tables[ix]->allocated());
}


// For debugging.
//
void
cCVtab::dbg_print(const cCHD *chd, const symref_t *p, unsigned int ix,
    bool pr_symref)
{
    if (ix >= ct_size) {
        printf("dbg_print: index out of range.\n");
        return;
    }
    if (chd) {
        if (ct_chd_db.find(chd) < 0) {
            printf("dbg_print: CHD not found.\n");
            return;
        }
        if (p || pr_symref) {
            if (!ct_tables[ix]) {
                printf("dbg_print: symref tab not found.\n");
                return;
            }
            if (p) {
                cvtab_item_t *item = ct_tables[ix]->find(p);
                if (!item) {
                    printf("dbg_print: symref not found.\n");
                    return;
                }
                item->print();
            }
            else {
                tgen_t<cvtab_item_t> gen(ct_tables[ix]);
                cvtab_item_t *item;
                while ((item = gen.next()) != 0)
                    item->print();
            }
        }
        return;
    }
    if (p) {
        if (!ct_tables[ix]) {
            printf("dbg_print: symref tab not found.\n");
            return;
        }
        cvtab_item_t *item = ct_tables[ix]->find(p);
        if (item) {
            item->print();
            return;
        }
        printf("dbg_print: symref not found.\n");
        return;
    }

    if (pr_symref) {
        if (!ct_tables[ix]) {
            printf("dbg_print: symref tab not found.\n");
            return;
        }
        tgen_t<cvtab_item_t> tgen(ct_tables[ix]);
        cvtab_item_t *item;
        while ((item = tgen.next()) != 0)
            item->print();
        printf("\n");
    }
}


// Return the memory space used for the table (does not include
// allocator overhead).
//
uint64_t
cCVtab::memuse()
{
    uint64_t bytes = ct_cvtab_allocator.memuse();
    bytes += ct_bytefact.memuse();
    bytes += ct_ts_bytefact.memuse();

    return (bytes);
}


// Private function to add an element to the table.
//
cvtab_item_t *
cCVtab::add(cCHD *chd, symref_t *p, unsigned int ix, const BBox *BB)
{
    if (!chd || !p)
        return (0);
    if (ix >= ct_size)
        return (0);
    if (!ct_tables[ix])
        ct_tables[ix] = new cvtab_t;
    cvtab_item_t *item = ct_tables[ix]->find(p);
    if (!item) {
        item = ct_cvtab_allocator.new_obj();
        int chd_tkt = ct_chd_db.add(chd);
        item->init(p, chd_tkt, BB);
        ct_tables[ix]->link(item);
        ct_tables[ix] = ct_tables[ix]->check_rehash();
        if (!item->init_flags(chd, p, &ct_bytefact))
            return (0);
    }
    // This should not be called with item already in the table, just
    // ignore if so.
    return (item);
}


// Private function, the symref p has the defseen flag not set, so
// isn't defined in the passed CHD.  The "real" symref should therefor
// exist in one of the other CHDs.  Hunt it down and return it, along
// with its CHD.
//
symref_t *
cCVtab::resolve_symref(const cCHD *chd, const symref_t *p, cCHD **chdret)
{
    if (!chd || !p)
        return (0);
    DisplayMode mode = p->mode();

    for (unsigned int i = 0; ; i++) {
        cCHD *tchd = ct_chd_db.chd(i);
        if (!tchd)
            break;
        if (tchd == chd)
            continue;
        nametab_t *ntab = tchd->nameTab(mode);
        if (!ntab)
            continue;
        symref_t *pret = ntab->get(p->get_name());
        if (pret) {
            if (chdret)
                *chdret = tchd;
            return (pret);
        }
    }
    if (chdret)
        *chdret = 0;
    return (0);
}


// Main work procedure for prune_empties, file accesses are sorted by
// offset, which is pretty much a requirement for gzipped files.  This
// considers all symrefs in the table.
//
OItype
cCVtab::prune_empties_core(chd_intab *itab, unsigned int ix)
{
    // Table of cells known to have geometry in view.
    ptrtab_t geo_tab;
    unsigned int total = 0;

    for (;;) {
        int cnt = 0;

        CVtabGen gen(this, ix);
        cCHD *tchd;
        symref_t *p;
        cvtab_item_t *item;
        while ((item = gen.next()) != 0) {
            p = item->symref();
            if (p->should_skip() || geo_tab.find(p))
                continue;
            tchd = ct_chd_db.chd(item->get_chd_tkt());
            const BBox *tBB = !item->get_full_area() ? item->get_bb() : 0;

            bool has_sc = false;
            DisplayMode mode = p->mode();
            nametab_t *ntab = tchd->nameTab(mode);
            crgen_t cgen(ntab, p);
            const cref_o_t *c;
            while ((c = cgen.next()) != 0) {
                if (item->get_flag(cgen.call_count()) == ts_unset)
                    continue;
                symref_t *cp = ntab->find_symref(c->srfptr);
                if (!cp) {
                    Errs()->add_error(
                    "prune_empties: unresolved instance symref back-pointer.");
                    return (OIerror);
                }
                if (!cp->get_defseen()) {
                    // Not defined in tchd, try to resolve through
                    // the other CHDs in the list.
                    cp = resolve_symref(tchd, cp);
                    if (!cp)
                        continue;
                }
                if (cp->should_skip())
                    continue;
                if (!get(cp, ix))
                    continue;

                // Don't set the has_sc flag unless a subcell
                // intersects the AOI.
                if (tBB) {
                    cvtab_item_t *ci = get(cp, ix);
                    if (!ci)
                        continue;

                    CDattr at;
                    if (!CD()->FindAttr(c->attr, &at)) {
                        Errs()->add_error(
                        "prune_empties: unresolved transform ticket %d.",
                            c->attr);
                        return (OIerror);
                    }

                    Instance inst;
                    inst.magn = at.magn;
                    inst.name = Tstring(cp->get_name());
                    inst.nx = at.nx;
                    inst.ny = at.ny;
                    inst.dx = at.dx;
                    inst.dy = at.dy;
                    inst.origin.set(c->tx, c->ty);
                    inst.reflection = at.refly;
                    inst.set_angle(at.ax, at.ay);
                    if (!inst.check_overlap(this, ci->get_bb(), tBB))
                        continue;
                }
                has_sc = true;
                break;
            }
            if (has_sc)
                continue;

            cv_in *in = itab->find(tchd);
            if (!in) {
                Errs()->add_error(
                    "prune_empties: input channel not found in table.");
                return (OIerror);
            }

            OItype oiret;
            if (!in->header_read()) {
                // This sets the correct scaling, needed if the input
                // file has differing m-units than our resolution.
                //
                // For OASIS, we also have to transfer the string
                // table pointers from the CHD, if they are there. 
                // When this happens, header_read is not persistently
                // set, so we take this branch every time (but is has
                // very low overhead.  If the header is actually read
                // from the file, we take this branch once at most.

                in->chd_setup(tchd, this, 0, mode, 1.0);
                oiret = in->has_geom(p, tBB);
                in->chd_finalize();
            }
            else
                oiret = in->has_geom(p, tBB);

            if (oiret == OIok)
                // has geometry
                geo_tab.add(p);
            else if (oiret == OIambiguous) {
                // empty cell
                cnt++;
                ct_tables[ix]->unlink(item);
            }
            else if (oiret == OIaborted)
                return (OIaborted);
            else {
                Errs()->add_error("prune_empties: error processing %s.",
                    Tstring(p->get_name()));
                return (OIerror);
            }
        }
        total += cnt;
        Tdbg()->save_message("%u %u", cnt, total);
        if (!cnt)
            break;
    }
    return (OIok);
}


// Alternative recursive part for prune_empties.  This traverses the
// hierarchy under pitem->symref(), removing empty cells as found. 
// However, file accesses are in random offset order, which can be
// very slow for gzipped files.
//
OItype
cCVtab::prune_empties_rc(cvtab_item_t *pitem, fio_chd_cvtab::ph_item_t *ph)
{
    symref_t *p = pitem->symref();
    cCHD *chd = ct_chd_db.chd(pitem->get_chd_tkt());
    nametab_t *ntab = chd->nameTab(p->mode());
    if (!ntab)
        return (OIok);
    const BBox *tBB = !pitem->get_full_area() ? pitem->get_bb() : 0;

    ticket_t last_srf = 0;
    cCHD *tchd = 0;
    symref_t *cp = 0;
    cvtab_item_t *ci = 0;

    bool has_sc = false;
    crgen_t gen(ntab, p);
    const cref_o_t *c;
    while ((c = gen.next()) != 0) {
        if (c->srfptr != last_srf) {
            last_srf = c->srfptr;

            cp = ntab->find_symref(c->srfptr);
            if (!cp) {
                Errs()->add_error(
                    "prune_empties_rc: unresolved symref back-pointer.");
                return (OIerror);
            }

            tchd = chd;
            if (!cp->get_defseen()) {
                // This will resolve library cells and via/pcell
                // sub-masters.  If the cell resolves to an archive, a
                // CHD and new symref will be returned.  The remaining
                // "unseen" symrefs will be resolved to cells in
                // memory later.

                RESOLVtype rsv = FIO()->ResolveUnseen(&tchd, &cp);
                if (rsv == RESOLVerror)
                    return (OIerror);
                if (rsv == RESOLVnone) {
                    cp = 0;
                    continue;
                }
            }
            if (cp->should_skip()) {
                cp = 0;
                continue;
            }

            // Don't set the has_sc flag unless a subcell
            // intersects the AOI.
            ci = get(cp, ph->index());
            if (!ci) {
                cp = 0;
                continue;
            }
        }
        if (cp) {
            if (!ph->find(cp))
                prune_empties_rc(ci, ph);
            if (get(cp, ph->index())) {
                if (tBB) {
                    CDattr at;
                    if (!CD()->FindAttr(c->attr, &at)) {
                        Errs()->add_error(
                        "prune_empties_rc: unresolved transform ticket %d.",
                            c->attr);
                        return (OIerror);
                    }

                    Instance inst;
                    inst.magn = at.magn;
                    inst.name = Tstring(cp->get_name());
                    inst.nx = at.nx;
                    inst.ny = at.ny;
                    inst.dx = at.dx;
                    inst.dy = at.dy;
                    inst.origin.set(c->tx, c->ty);
                    inst.reflection = at.refly;
                    inst.set_angle(at.ax, at.ay);
                    if (!inst.check_overlap(this, ci->get_bb(), tBB))
                        continue;
                }
                has_sc = true;
            }
            else
                cp = 0;
        }
    }

    if (!has_sc) {
        cv_in *in = ph->itab()->find(chd);
        if (!in) {
            Errs()->add_error(
                "prune_empties_rc: input channel not found in table.");
            return (OIerror);
        }

        OItype oiret;
        if (!in->header_read()) {
            // This sets the correct scaling, needed if the input
            // file has differing m-units than our resolution.
            //
            // For OASIS, we also have to transfer the string
            // table pointers from the CHD, if they are there. 
            // When this happens, header_read is not persistently
            // set, so we take this branch every time (but is has
            // very low overhead.  If the header is actually read
            // from the file, we take this branch once at most.

            in->chd_setup(chd, this, 0, p->mode(), 1.0);
            oiret = in->has_geom(p, tBB);
            in->chd_finalize();
        }
        else
            oiret = in->has_geom(p, tBB);

        if (oiret == OIok)
            // has geometry
            ;
        else if (oiret == OIambiguous) {
            // empty cell
            ph->increment();
            ct_tables[ph->index()]->unlink(pitem);
        }
        else if (oiret == OIaborted)
            return (OIaborted);
        else {
            Errs()->add_error("prune_empties_rc: error processing %s.",
                Tstring(p->get_name()));
            return (OIerror);
        }
    }
    ph->add(p);
    return (OIok);
}


// Recursively set the full_area flag and add to the BBox table or
// update table BB, for all symrefs p and under.
//
bool
cCVtab::build_BB_table_full_rc(cvtab_item_t *pitem, unsigned int ix)
{
    if (pitem->get_full_area())
        return (true);
    pitem->set_full_area(true);

    symref_t *p = pitem->symref();
    cCHD *chd = ct_chd_db.chd(pitem->get_chd_tkt());
    nametab_t *ntab = chd->nameTab(p->mode());
    if (!ntab)
        return (true);

    ticket_t last_srf = 0;
    cCHD *tchd = 0;
    symref_t *cp = 0;

    crgen_t gen(ntab, p);
    const cref_o_t *c;
    while ((c = gen.next()) != 0) {
        if (c->srfptr != last_srf) {
            last_srf = c->srfptr;

            cp = ntab->find_symref(c->srfptr);
            if (!cp) {
                Errs()->add_error(
                    "build_BB_table_full_rc: unresolved symref back-pointer.");
                return (false);
            }

            tchd = chd;
            if (!cp->get_defseen()) {
                // This will resolve library cells and via/pcell
                // sub-masters.  If the cell resolves to an archive, a
                // CHD and new symref will be returned.  The remaining
                // "unseen" symrefs will be resolved to cells in
                // memory later.

                RESOLVtype rsv = FIO()->ResolveUnseen(&tchd, &cp);
                if (rsv == RESOLVerror)
                    return (false);
                if (rsv == RESOLVnone) {
                    cp = 0;
                    continue;
                }
            }
            if (cp->should_skip()) {
                cp = 0;
                continue;
            }

            cvtab_item_t *item = get(cp, ix);
            if (!item) {
                item = add(tchd, cp, ix);
                if (!build_BB_table_full_rc(item, ix))
                    return (false);
            }
            else if (!item->get_full_area()) {
                item->add_area(cp->get_bb());
                if (!build_BB_table_full_rc(item, ix))
                    return (false);
            }
        }
        if (!cp)
            continue;

        pitem->set_flag(gen.call_count());
    }
    return (true);
}


// Non-recursive version of above.  This is a separate function rather
// than passing a flag to avoid the flag checking overhead.
//
bool
cCVtab::build_BB_table_full_norc(cvtab_item_t *pitem)
{
    if (pitem->get_full_area())
        return (true);
    pitem->set_full_area(true);

    symref_t *p = pitem->symref();
    cCHD *chd = ct_chd_db.chd(pitem->get_chd_tkt());
    nametab_t *ntab = chd->nameTab(p->mode());
    if (!ntab)
        return (true);

    ticket_t last_srf = 0;
    cCHD *tchd = 0;
    symref_t *cp = 0;

    crgen_t gen(ntab, p);
    const cref_o_t *c;
    while ((c = gen.next()) != 0) {
        if (c->srfptr != last_srf) {
            last_srf = c->srfptr;

            cp = ntab->find_symref(c->srfptr);
            if (!cp) {
                Errs()->add_error(
                "build_BB_table_full_norc: unresolved symref back-pointer.");
                return (false);
            }

            tchd = chd;
            if (!cp->get_defseen()) {
                // This will resolve library cells and via/pcell
                // sub-masters.  If the cell resolves to an archive, a
                // CHD and new symref will be returned.  The remaining
                // "unseen" symrefs will be resolved to cells in
                // memory later.

                RESOLVtype rsv = FIO()->ResolveUnseen(&tchd, &cp);
                if (rsv == RESOLVerror)
                    return (false);
                if (rsv == RESOLVnone) {
                    cp = 0;
                    continue;
                }
            }
            if (cp->should_skip()) {
                cp = 0;
                continue;
            }
        }
        if (!cp)
            continue;

        pitem->set_flag(gen.call_count());
    }
    return (true);
}


// Private recursive function to build the table, with bounding boxes.
// Physical mode only.
//
bool
cCVtab::build_BB_table_rc(cvtab_item_t *pitem, unsigned int ix)
{
    symref_t *p = pitem->symref();
    cCHD *chd = ct_chd_db.chd(pitem->get_chd_tkt());
    nametab_t *ntab = chd->nameTab(p->mode());
    if (!ntab)
        return (true);

    ticket_t last_srf = 0;
    symref_t *cp = 0;
    cCHD *tchd = 0;
    const BBox *rBB = 0;
    cvtab_item_t *item = 0;

    crgen_t gen(ntab, p);
    const cref_o_t *c;
    while ((c = gen.next()) != 0) {
        if (c->srfptr != last_srf) {
            last_srf = c->srfptr;

            cp = ntab->find_symref(c->srfptr);
            if (!cp) {
                Errs()->add_error(
                    "build_BB_table_rc: unresolved symref back-pointer.");
                return (false);
            }

            tchd = chd;
            if (!cp->get_defseen()) {
                // This will resolve library cells and via/pcell
                // sub-masters.  If the cell resolves to an archive, a
                // CHD and new symref will be returned.  The remaining
                // "unseen" symrefs will be resolved to cells in
                // memory later.

                RESOLVtype rsv = FIO()->ResolveUnseen(&tchd, &cp);
                if (rsv == RESOLVerror)
                    return (false);
                if (rsv == RESOLVnone) {
                    cp = 0;
                    continue;
                }
            }
            if (cp->should_skip()) {
                cp = 0;
                continue;
            }
            item = get(cp, ix);
            rBB = item ? item->get_bb() : 0;
        }
        if (!cp)
            continue;

        // Note that if we didn't have to deal with the instance use
        // flags, we could continue here with logic like:
        // if (item && item->get_full_area()) {
        //     continue;
        // }
        // if (rBB && !cp->get_crefs() && *rBB == *cp->get_bb()) {
        //     item->set_full_area(true);
        //     continue;
        // }

        CDattr at;
        if (!CD()->FindAttr(c->attr, &at)) {
            Errs()->add_error(
                "build_BB_table_rc: unresolved transform ticket %d.", c->attr);
            return (false);
        }

        TPush();
        TApply(c->tx, c->ty, at.ax, at.ay, at.magn, at.refly);
        TPremultiply();

        unsigned int x1, x2, y1, y2;
        if (TOverlap(cp->get_bb(), &ct_AOI, at.nx, at.dx, at.ny, at.dy,
                &x1, &x2, &y1, &y2)) {

            bool all_in = false;
            int *ifoo = 0;
            if (x2 - x1 >= 2 && y2 - y1 >= 2)
                all_in = true;
            else {
                ifoo = new int[(x2-x1+1)*(y2-y1+1)*4];
                BBox *iBB = (BBox*)(void*)ifoo;
                int cnt = 0;
                int tx, ty;
                TGetTrans(&tx, &ty);
                xyg_t xyg(x1, x2, y1, y2);
                do {
                    TTransMult(xyg.x*at.dx, xyg.y*at.dy);
                    TInverse();
                    iBB[cnt] = ct_AOI;
                    TInverseBB(iBB + cnt, 0);
                    TSetTrans(tx, ty);
                    if (*cp->get_bb() <= iBB[cnt]) {
                        all_in = true;
                        delete [] ifoo;
                        break;
                    }
                    cnt++;
                } while (xyg.advance());
            }

            if (all_in) {
                // At least one element is completely exposed, cp
                // and its heirarchy should all be included fully.

                pitem->set_flag(gen.call_count());
                if (!item) {
                    item = add(tchd, cp, ix);
                    rBB = item->get_bb();
                    if (!build_BB_table_full_rc(item, ix)) {
                        TPop();
                        return (false);
                    }
                }
                else if (!item->get_full_area()) {
                    item->add_area(cp->get_bb());
                    if (!build_BB_table_full_rc(item, ix)) {
                        TPop();
                        return (false);
                    }
                }
            }
            else {
                // Be careful about setting instance use flag, we want to
                // set the flag only if the instance or array component
                // actually overlaps the AOI.  The TOverlap return only
                // indicates that the array BB touches or overlaps AOI.

                if (item && (item->get_full_area() ||
                        (!cp->get_crefs() && *rBB == *cp->get_bb()))) {

                    delete [] ifoo;
                    bool ok = false;
                    BBox iBB(*cp->get_bb());
                    if (at.nx <= 1 && at.ny <= 1)
                        ok = TBBcheck(&iBB, &ct_AOI);
                    else if ((abs(at.dx) - iBB.width() < ct_AOI.width()) &&
                            (abs(at.dy) - iBB.height() < ct_AOI.height())) {
                        // It is sufficient to see overlap of arrayed BB.

                        if (at.dx > 0)
                            iBB.right += (at.nx - 1)*at.dx;
                        else
                            iBB.left += (at.nx - 1)*at.dx;
                        if (at.dy > 0)
                            iBB.top += (at.ny - 1)*at.dy;
                        else
                            iBB.bottom += (at.ny - 1)*at.dy;
                        ok = TBBcheck(&iBB, &ct_AOI);
                    }
                    else {
                        // Need to check individual elements, since ct_AOI
                        // could fit in the spacing between elements.

                        int tx, ty;
                        TGetTrans(&tx, &ty);
                        xyg_t xyg(x1, x2, y1, y2);
                        do {
                            TTransMult(xyg.x*at.dx, xyg.y*at.dy);
                            ok = TBBcheck(&iBB, &ct_AOI);
                            TSetTrans(tx, ty);
                            if (ok)
                                break;

                        } while (xyg.advance());
                    }
                    if (ok)
                        pitem->set_flag(gen.call_count());
                    TPop();
                    continue;
                }

                BBox *iBB = (BBox*)(void*)ifoo;
                bool flag_set = false;
                int cnt = 0;
                int tx, ty;
                TGetTrans(&tx, &ty);
                xyg_t xyg(x1, x2, y1, y2);
                do {
                    TTransMult(xyg.x*at.dx, xyg.y*at.dy);

                    BBox *tBB = iBB + cnt;
                    BBox sBB = *cp->get_bb();
                    if (sBB.isect_x(tBB)) {
                        if (!flag_set) {
                            pitem->set_flag(gen.call_count());
                            flag_set = true;
                        }
                        if (sBB.left < tBB->left)
                            sBB.left = tBB->left;
                        if (sBB.bottom < tBB->bottom)
                            sBB.bottom = tBB->bottom;
                        if (sBB.right > tBB->right)
                            sBB.right = tBB->right;
                        if (sBB.top > tBB->top)
                            sBB.top = tBB->top;

                        if (!item) {
                            item = add(tchd, cp, ix, &sBB);
                            if (!item) {
                                TPop();
                                return (false);
                            }
                            rBB = item->get_bb();
                        }
                        else {
                            if (item->get_full_area())
                                break;
                            item->add_area(&sBB);
                        }

                        if (!build_BB_table_rc(item, ix)) {
                            TPop();
                            delete [] ifoo;
                            return (false);
                        }
                    }
                    TSetTrans(tx, ty);
                    cnt++;
                } while (xyg.advance());
                delete [] ifoo;
            }
        }
        TPop();
    }
    return (true);
}


// Non-recursive version of above.  This is a separate function rather
// than passing a flag to avoid the flag checking overhead.
//
bool
cCVtab::build_BB_table_norc(cvtab_item_t *pitem)
{
    symref_t *p = pitem->symref();
    cCHD *chd = ct_chd_db.chd(pitem->get_chd_tkt());
    nametab_t *ntab = chd->nameTab(p->mode());
    if (!ntab)
        return (true);

    ticket_t last_srf = 0;
    symref_t *cp = 0;
    cCHD *tchd = 0;

    crgen_t gen(ntab, p);
    const cref_o_t *c;
    while ((c = gen.next()) != 0) {
        if (c->srfptr != last_srf) {
            last_srf = c->srfptr;

            cp = ntab->find_symref(c->srfptr);
            if (!cp) {
                Errs()->add_error(
                    "build_BB_table_norc: unresolved symref back-pointer.");
                return (false);
            }

            tchd = chd;
            if (!cp->get_defseen()) {
                // This will resolve library cells and via/pcell
                // sub-masters.  If the cell resolves to an archive, a
                // CHD and new symref will be returned.  The remaining
                // "unseen" symrefs will be resolved to cells in
                // memory later.

                RESOLVtype rsv = FIO()->ResolveUnseen(&tchd, &cp);
                if (rsv == RESOLVerror)
                    return (false);
                if (rsv == RESOLVnone) {
                    cp = 0;
                    continue;
                }
            }
            if (cp->should_skip()) {
                cp = 0;
                continue;
            }
        }
        if (!cp)
            continue;

        CDattr at;
        if (!CD()->FindAttr(c->attr, &at)) {
            Errs()->add_error(
            "build_BB_table_norc: unresolved transform ticket %d.", c->attr);
            return (false);
        }

        TPush();
        TApply(c->tx, c->ty, at.ax, at.ay, at.magn, at.refly);
        TPremultiply();

        unsigned int x1, x2, y1, y2;
        if (TOverlap(cp->get_bb(), &ct_AOI, at.nx, at.dx, at.ny, at.dy,
                &x1, &x2, &y1, &y2)) {

            bool all_in = false;
            int *ifoo = 0;
            if (x2 - x1 >= 2 && y2 - y1 >= 2)
                all_in = true;
            else {
                ifoo = new int[(x2-x1+1)*(y2-y1+1)*4];
                BBox *iBB = (BBox*)(void*)ifoo;
                int cnt = 0;
                int tx, ty;
                TGetTrans(&tx, &ty);
                xyg_t xyg(x1, x2, y1, y2);
                do {
                    TTransMult(xyg.x*at.dx, xyg.y*at.dy);
                    TInverse();
                    iBB[cnt] = ct_AOI;
                    TInverseBB(iBB + cnt, 0);
                    TSetTrans(tx, ty);
                    if (*cp->get_bb() <= iBB[cnt]) {
                        all_in = true;
                        delete [] ifoo;
                        break;
                    }
                    cnt++;
                } while (xyg.advance());
            }

            if (all_in) {
                // At least one element is completely exposed, cp
                // and its heirarchy should all be included fully.

                pitem->set_flag(gen.call_count());
            }
            else {
                BBox *iBB = (BBox*)(void*)ifoo;
                int cnt = 0;
                int tx, ty;
                TGetTrans(&tx, &ty);
                xyg_t xyg(x1, x2, y1, y2);
                do {
                    TTransMult(xyg.x*at.dx, xyg.y*at.dy);

                    BBox *tBB = iBB + cnt;
                    const BBox *sBB = cp->get_bb();
                    if (sBB->isect_x(tBB)) {
                        pitem->set_flag(gen.call_count());
                        break;
                    }
                    TSetTrans(tx, ty);
                    cnt++;
                } while (xyg.advance());
                delete [] ifoo;
            }
        }
        TPop();
    }
    return (true);
}


// Private function to recursively walk the hierarchy and build up the
// table of transform list heads.
//
bool
cCVtab::build_TS_table_rc(cvtab_item_t *pitem, unsigned int ix,
    unsigned int depth)
{
    symref_t *p = pitem->symref();
    cCHD *chd = ct_chd_db.chd(pitem->get_chd_tkt());
    nametab_t *ntab = chd->nameTab(p->mode());
    if (!ntab)
        return (true);

    // Loop thru instances.
    crgen_t gen(ntab, p);
    const cref_o_t *cr;
    while ((cr = gen.next()) != 0) {
        if (pitem && pitem->get_flag(gen.call_count()) == ts_unset)
            continue;

        symref_t *cp = ntab->find_symref(cr->srfptr);
        if (!cp) {
            Errs()->add_error(
                "build_TS_table_rc: unresolved symref back-pointer.");
            return (false);
        }

        if (!cp->get_defseen()) {
            symref_t *tcp = resolve_symref(chd, cp);
            if (tcp)
                cp = tcp;

        }
        if (cp->should_skip())
            continue;
        cvtab_item_t *item = get(cp, ix);

        if (!item)
            continue;
        const BBox *pBB = item->get_bb();

        CDattr at;
        if (!CD()->FindAttr(cr->attr, &at)) {
            Errs()->add_error(
                "build_TS_table_rc: unresolved transform ticket %d.",
                cr->attr);
            return (false);
        }

        TPush();
        TApply(cr->tx, cr->ty, at.ax, at.ay, at.magn, at.refly);
        TPremultiply();

        // Scalarize case of multiple coincident rows or columns.
        if (at.dx == 0)
            at.nx = 1;
        if (at.dy == 0)
            at.ny = 1;

        // For each cell used, hash a list of transforms.
        bool doit;
        bool check_elts = false;
        unsigned int x1, x2, y1, y2;
        if (no_aoi(ix)) {
            doit = true;
            x1 = 0;
            x2 = at.nx - 1;
            y1 = 0;
            y2 = at.ny - 1;
        }
        else {
            if (!pBB)
                pBB = cp->get_bb();
            doit = TOverlap(pBB, &ct_AOI, at.nx, at.dx, at.ny, at.dy,
                &x1, &x2, &y1, &y2);
            if (doit && ((at.nx > 1 &&
                    abs(at.dx) - pBB->width() >= ct_AOI.width()) ||
                    (at.ny > 1 &&
                    abs(at.dy) - pBB->height() >= ct_AOI.height())))
                check_elts = true;
        }
        if (!doit) {
            TPop();
            continue;
        }
        if (cp->get_crefs()) {
            // Not a leaf cell, expand array and recurse subcells.

            int tx, ty;
            TGetTrans(&tx, &ty);

            xyg_t xyg(x1, x2, y1, y2);
            do {
                TTransMult(xyg.x*at.dx, xyg.y*at.dy);
                if (check_elts) {
                    BBox tmpBB(*pBB);
                    if (!TBBcheck(&tmpBB, &ct_AOI)) {
                        TSetTrans(tx, ty);
                        continue;
                    }
                }
                if (depth+1 < ct_maxdepth) {
                    if (!build_TS_table_rc(item, ix, depth+1)) {
                        TPop();
                        return (false);
                    }
                }
                CDtx ttx;
                TCurrent(&ttx);
                if (ts_cnt(ix)) {
                    ts_writer tsw(0, 0, true);
                    tsw.add_record(&ttx, 1, 1, 0, 0);
                    item->set_bytes_used(
                        item->bytes_used() + tsw.bytes_used());
                }
                else {
                    if (!item->allocate_tstream(&ct_ts_bytefact)) {
                        TPop();
                        return (false);
                    }
                    if (item->ticket()) {
                        unsigned char *str =
                            ct_ts_bytefact.find(item->ticket());
                        ts_writer tsw(str, item->bytes_used(), false);
                        tsw.add_record(&ttx, 1, 1, 0, 0);
                        item->set_bytes_used(tsw.bytes_used());
                    }
                }
                TSetTrans(tx, ty);

            } while (xyg.advance());
        }
        else {
            // Leaf cell, deal with array transforms at read-time.

            CDtx tx;
            TCurrent(&tx);
            if (ts_cnt(ix)) {
                ts_writer tsw(0, 0, true);
                tsw.add_record(&tx, at.nx, at.ny, at.dx, at.dy);
                item->set_bytes_used(item->bytes_used() +  tsw.bytes_used());
            }
            else {
                if (!item->allocate_tstream(&ct_ts_bytefact)) {
                    TPop();
                    return (false);
                }
                if (item->ticket()) {
                    unsigned char *str = ct_ts_bytefact.find(item->ticket());
                    ts_writer tsw(str, item->bytes_used(), false);
                    tsw.add_record(&tx, at.nx, at.ny, at.dx, at.dy);
                    item->set_bytes_used(tsw.bytes_used());
                }
            }
        }
        TPop();
    }
    return (true);
}
// End of cCVtab functions.


// Initialize the cvi_flags.  If the count is sufficiently small, the
// cvi_flags field is used directly.  Otherwise, it is a pointer to the
// actual flag storage (obtained from fact).
//
bool
cvtab_item_t::init_flags(const cCHD *chd, const symref_t *p, bytefact_t *fact)
{
    cvi_flags = 0;
    if (!cvi_use_flags)
        return (true);
    if (!chd || !p || !fact) {
        Errs()->add_error("init_flags: null argument.");
        return (false);
    }
    if (!p->get_elec()) {
        nametab_t *ntab = chd->nameTab(Physical);
        crgen_t gen(ntab, p);
        const cref_o_t *c;
        while ((c = gen.next()) != 0) ;
        unsigned int cnt = gen.call_count();

        if (cnt <= CTAB_NUMLFLAGS) {
            // This indicates that flags are saved locally.
            cvi_flags = 0x1;
        }
        else {
            unsigned int sz = (cnt >> 3) + ((cnt & 0x7) != 0);
            // Make sure that addresses are at least 2-byte aligned.
            if (sz & 0x1)
                sz++;
            ticket_t t = fact->get_space_nonvolatile(sz);
            if (!t) {
                if (fact->is_full()) {
                    Errs()->add_error(
                        "Ticket allocation failed, internal block "
                        "limit %d reached in\ninstance use flag allocation.",
                        BDB_MAX);
                }
                else {
                    Errs()->add_error(
                        "Ticket allocation failure, request for 0 "
                        "bytes in instance\nuse flag allocation.");
                }
                return (false);
            }
            unsigned char *s = fact->find(t);
            memset(s, 0, sz);
            cvi_flags = (uintptr_t)s;
        }
    }
    return (true);
}


// Static function.
//
void
cvtab_item_t::alloc_failed(bytefact_t *bytefact)
{
    if (bytefact->is_full()) {
        Errs()->add_error(
            "Ticket allocation failed, internal block limit "
            "%d reached in\ntransform string allocation.",
            BDB_MAX);
    }   
    else {
        Errs()->add_error(
            "Ticket allocation failure, request for 0 bytes "
            "in transform\nstring allocation.");
    }
}
// End of cvtab_item_t functions.


// Generator methods.  The cCHD/symref_t passed to the constructor will
// be returned first.

namespace {
    // Sort comparison function, sort by chd index, then by offset.
    //
    inline bool
    cmp_off(const cvtab_item_t *e1, const cvtab_item_t *e2)
    {
        int c1 = e1->get_chd_tkt();
        int c2 = e2->get_chd_tkt();
        if (c1 == c2) {
            int64_t n1 = e1->symref()->get_offset();
            int64_t n2 = e2->symref()->get_offset();
            return (n1 < n2);
        }
        return (c1 < c2);
    }
}


CVtabGen::CVtabGen(const cCVtab *ct, unsigned int ix, const symref_t *p)
{
    cg_array = 0;
    cg_size = 0;
    cg_count = 0;

    tgen_t<cvtab_item_t> gen(ct->ct_tables[ix]);
    cg_array = new cvtab_item_t*[ct->ct_tables[ix]->allocated()];
    cvtab_item_t *item;
    unsigned int cnt = 0;
    while ((item = gen.next()) != 0)
        cg_array[cnt++] = item;
    cg_size = cnt;

    if (cnt > 1) {
        bool adv1 = false;
        if (p) {
            item = ct->get(p, ix);
            if (item && item != cg_array[0]) {
                for (unsigned int i = 1; i < cg_size; i++) {
                    if (item == cg_array[i]) {
                        cg_array[i] = cg_array[0];
                        cg_array[0] = item;
                        adv1 = true;
                        break;
                    }
                }
            }
        }
        if (adv1)
            std::sort(cg_array + 1, cg_array + cnt, cmp_off);
        else
            std::sort(cg_array, cg_array + cnt, cmp_off);
    }
}
// End of CVtabGen functions.

