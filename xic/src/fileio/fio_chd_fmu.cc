
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
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
 $Id: fio_chd_fmu.cc,v 1.10 2010/05/09 00:17:43 stevew Exp $
 *========================================================================*/

#include "fio.h"
#include "fio_chd.h"
#include "fio_chd_flat.h"
#include "fio_cvt_base.h"


//-----------------------------------------------------------------------------
// fmu_t
// Class for estimating memory requirements for flat_read.
//-----------------------------------------------------------------------------

fmu_t::fmu_t(cCHD *chd, symref_t *top)
{
    box_cnt = 0;
    wire_cnt = 0;
    poly_cnt = 0;
    vtex_cnt = 0;
    fchd = chd;
    ftop = top;
    ftab = 0;

    if (!top && chd) {
        syrlist_t *sl = chd->topCells(Physical, true);
        if (sl)
            ftop = sl->symref;
        sl->free();
    }
}


fmu_t::~fmu_t()
{
    delete ftab;
}


bool
fmu_t::est_flat_memory_use(BBox *aoi, double *mbytes)
{
    *mbytes = 0.0;
    if (!fchd || !ftop) {
        Errs()->add_error(
            "fmu_t::est_flat_memory_use: no CHD or no top-level cell "
            "reference.");
        return (false);
    }
    if (aoi)
        AOI = *aoi;
    else
        AOI = BBox(0, 0, 0, 0);

    bool ret = find_totals();
    if (ret) {
        double sz = 0;
        sz += box_cnt * sizeof(CDo);
        sz += poly_cnt * sizeof(CDpo);
        sz += wire_cnt * sizeof(CDw);
        sz += vtex_cnt * sizeof(Point);
        double nobjs = box_cnt + poly_cnt + wire_cnt;
        sz += nobjs * .3 * sizeof(RTelem);
        *mbytes = sz*1e-6;
    }
    return (ret);
}


// Recurse through the hierarchy and totalize the counts for used
// instances.
//
bool
fmu_t::find_totals()
{
    if (!fchd || !fchd->pcInfo(Physical)) {
        Errs()->add_error( "fmu_t::find_totals: no CHD or no info.");
        return (false);
    }

    if (!ftab) {
        // Create a hash table to map the symrefs to the cell offset number.
        syrlist_t *sl = fchd->listing(Physical, true);
        ftab = new SymTab(false, false);

        unsigned int cnt = 0;
        for (syrlist_t *s = sl; s; s = s->next) {
            ftab->add((unsigned long)s->symref, (void*)(long)cnt, false);
            cnt++;
        }
        sl->free();
    }

    cTfmStack stk;
    return (fmu_core_rc(ftop, &stk));
}


// Recursively add the counts from each instance, normalized to the
// overlap area.
//
bool
fmu_t::fmu_core_rc(symref_t *p, cTfmStack *tstk)
{
    nametab_t *ntab = fchd->nameTab(Physical);
    crgen_t gen(ntab, p);
    const cref_o_t *cr;
    while ((cr = gen.next()) != 0) {
        symref_t *cp = ntab->find_symref(cr->srfptr);
        if (!cp || cp->should_skip())
            continue;

        tstk->TPush();
        CDattr at;
        if (!CD()->FindAttr(cr->attr, &at)) {
            Errs()->add_error(
                "fmu_t::fmu_core_rc: unresolved transform ticket %d.",
                cr->attr);
            return (false);
        }
        int x, y, dx, dy;
        x = cr->tx;
        y = cr->ty;
        dx = at.dx;
        dy = at.dy;

        tstk->TApply(x, y, at.ax, at.ay, at.magn, at.refly);
        tstk->TPremultiply();
        int tx, ty;
        tstk->TGetTrans(&tx, &ty);
        unsigned int i = 0;
        unsigned int j = 0;
        bool ret = true;
        while (ret) {
            tstk->TTransMult(j*dx, i*dy);
            BBox sBB = *cp->get_bb();
            tstk->TBB(&sBB, 0);

            if (sBB.intersect(&AOI, false) && cp->get_crefs())
                // recurse
                ret = fmu_core_rc(cp, tstk);

            if (ret) {
                Blist *bl = sBB.clip_to(&AOI);
                if (bl) {
                    double a = 0.0;
                    for (Blist *b = bl; b; b = b->next)
                        a += b->BB.area();
                    Blist::destroy(bl);
                    double a_max = sBB.area();
                    ret = sum_counts(a/a_max, cp);
                }
            }
            tstk->TSetTrans(tx, ty);

            j++;
            if (j >= at.nx) {
                j = 0;
                i++;
                if (i >= at.ny)
                    break;
            }
        }
        tstk->TPop();
    }
    return (true);
}


// Add the counts from sref normalized by sc to the running sum.
//
bool
fmu_t::sum_counts(double sc, symref_t *sref)
{
    if (sc > .999999)
        sc = 1.0;
    if (!ftab) {
        Errs()->add_error("fmu_t::sum_counts: null offset table.");
        return (false);
    }
    if (!sref) {
        Errs()->add_error("fmu_t::sum_counts: null symbol reference.");
        return (false);
    }
    unsigned int offs = (unsigned long)ftab->get((unsigned long)sref);
    if (offs == (unsigned int)(long)ST_NIL) {
        Errs()->add_error(
            "fmu_t::sum_counts: unresolved symbol reference.");
        return (false);
    }
    itable_t<pc_data> *pctab = fchd->pcInfo(Physical)->pcdata_tab();
    pc_data *pc = pctab ? pctab->find(sref->get_name()) : 0;
    if (!pc) {
        Errs()->add_error("fmu_t::sum_counts: unresolved offset reference.");
        return (false);
    }
    box_cnt += (uint64_t)(sc*pc->total_boxes());
    uint64_t pcnt = (uint64_t)(sc*pc->total_polys());
    uint64_t wcnt = (uint64_t)(sc*pc->total_wires());
    poly_cnt += pcnt;
    wire_cnt += wcnt;
    if (pcnt + wcnt)
        vtex_cnt += (uint64_t)(sc*pc->total_vertices());
    return (true);
}

