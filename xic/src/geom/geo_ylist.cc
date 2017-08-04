
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

#include "cd.h"
#include "geo_zgroup.h"
#include "geo_ylist.h"
#include "geo_poly.h"
#include "geo_intdb.h"
#include "geo_ymgr.h"
#include "cd_chkintr.h"
#include "timedbg.h"


// Ylist constructor, z0 is consumed.
//
Ylist::Ylist(Zlist *z0, bool sub)
{
    y_zlist = 0;
    y_yu = y_yl = 0;
    next = 0;

    if (z0) {
        if (sub) {
            y_zlist = z0;
            y_yu = z0->Z.yu;
            y_yl = z0->Z.yl;
        }
        else {
            z0 = Zlist::sort(z0, 1);
            y_zlist = z0;
            y_yu = z0->Z.yu;
            y_yl = z0->Z.yl;
            Ylist *ylast = this;
            while (z0) {
                if (ylast->y_yl > z0->Z.yl)
                    ylast->y_yl = z0->Z.yl;
                if (z0->next && z0->next->Z.yu != ylast->y_yu) {
                    ylast->next = new Ylist(z0->next, true);
                    ylast = ylast->next;
                    Zlist *zt = z0->next;
                    z0->next = 0;
                    z0 = zt;
                    continue;
                }
                z0 = z0->next;
            }
        }
    }
}


namespace geo_ylist {
    // Helper struct for Ylist::to_poly
    //
    struct topoly_t
    {
        topoly_t()
            {
                tp_p = 0;
                tp_head = 0;
                tp_vcnt = 0;
                tp_again = false;
            }

        int append_point(Plist **pp, int x, int y)
            {
                Plist *p = *pp;
                if (p->x != x || p->y != y) {
                    p->next = tp_fact.new_Plist(x, y);
                    *pp = p->next;
                    return (1);
                }
                return (0);
            }

        Plist *tp_p;                // current list element
        Plist *tp_head;             // list head;
        plist_factory_t tp_fact;    // source for list elements
        int tp_vcnt;                // current vertex count
        bool tp_again;              // flag return from find_xxx
    };
}


// Static function.
//
bool
Ylist::intersect(const Ylist *thisyl, const Ylist *yl0, bool touchok)
{
    Ymgr ym(yl0);
    for (const Ylist *y = thisyl; y; y = y->next) {
        for (Zlist *zl1 = y->y_zlist; zl1; zl1 = zl1->next) {
            ovlchk_t ovl(zl1->Z.minleft(), zl1->Z.maxright(), zl1->Z.yu);
            ym.reset();
            const Ylist *yy;
            while ((yy = ym.next(zl1->Z.yu)) != 0) {
                if (yy->y_yu <= zl1->Z.yl)
                    break;
                for (Zlist *zl2 = yy->y_zlist; zl2; zl2 = zl2->next) {
                    if (ovl.check_break(zl2->Z))
                        break;
                    if (ovl.check_continue(zl2->Z))
                        continue;
                    if (zl1->Z.intersect(&zl2->Z, touchok))
                        return (true);
                }
            }
        }
    }
    return (false);
}


// Static function.
// Return the first zoid found that encloses p.
//
const Zoid *
Ylist::find_container(const Ylist *thisyl, const Point *p)
{
    ovlchk_t ovl(p->x, p->x, p->y);
    for (const Ylist *y = thisyl; y; y = y->next) {
        if (y->y_yl >= p->y)
            continue;
        if (y->y_yu <= p->y)
            break;
        for (Zlist *z = y->y_zlist; z; z = z->next) {
            if (ovl.check_break(z->Z))
                break;
            if (ovl.check_continue(z->Z))
                continue;
            if (z->Z.is_interior(p))
                return (&z->Z);
        }
    }
    return (0);
}


// Static function.
//
Ylist *
Ylist::copy(const Ylist *thisyl)
{
    Ylist *y0 = 0, *ye = 0;
    for (const Ylist *yl = thisyl; yl; yl = yl->next) {
        Ylist *yn = new Ylist(*yl);
        yn->next = 0;
        yn->y_zlist = Zlist::copy(yn->y_zlist);
        if (!y0)
            y0 = ye = yn;
        else {
            ye->next = yn;
            ye = ye->next;
        }
    }
    return (y0);
}


// Static function.
//
Ylist *
Ylist::to_poly(Ylist *thisyl, Point **pts, int *num, int maxv)
{
    TimeDbgAccum ac("to_poly");

    *pts = 0;
    *num = 0;
    if (!thisyl)
        return(0);

    Zlist *zl;
    Ylist *yl0 = thisyl->first(&zl);
    if (!zl)
        return (0);
    if (!yl0) {
        zl->Z.mkpoly(pts, num);
        delete zl;
        return (0);
    }

    geo_ylist::topoly_t tp;

    tp.tp_p = tp.tp_fact.new_Plist(zl->Z.xll, zl->Z.yl);
    Plist *p = tp.tp_p;
    int np = 1;
    np += tp.append_point(&p, zl->Z.xul, zl->Z.yu);
    np += tp.append_point(&p, zl->Z.xur, zl->Z.yu);
    np += tp.append_point(&p, zl->Z.xlr, zl->Z.yl);
    np += tp.append_point(&p, zl->Z.xll, zl->Z.yl);
    tp.tp_vcnt = np;
    tp.tp_head = tp.tp_p;

    delete zl;

    if (maxv > 8)
        maxv -= 4;

    for (;;) {
        tp.tp_again = false;
        p = tp.tp_p;
        Plist *pn = p->next;
        if (!pn)
            break;
        if (p->y == pn->y) {
            if (p->x < pn->x)
                yl0 = yl0->find_bottom(&tp);
            else
                yl0 = yl0->find_top(&tp);
        }
        else if (p->x == pn->x) {
            if (p->y < pn->y)
                yl0 = yl0->find_right(&tp);
            else
                yl0 = yl0->find_left(&tp);
        }
        else {
            if (p->y < pn->y)
                yl0 = yl0->find_right_nm(&tp);
            else
                yl0 = yl0->find_left_nm(&tp);
        }
        if (!yl0 || (maxv > 0 && tp.tp_vcnt > maxv))
            break;
        if (tp.tp_again)
            continue;
         tp.tp_p = p->next;
    }

    Poly po;
    po.points = new Point[tp.tp_vcnt];
    int i = 0;
    Plist *pn;
    for (Plist *pp = tp.tp_head; pp; pp = pn, i++) {
        pn = pp->next;
        po.points[i].set(pp->x, pp->y);
    }
    po.numpts = i;

    if (po.valid()) {
        *pts = po.points;
        *num = po.numpts;
        return (yl0);
    }
    else {
        // The poly found was bad, throw it away and try again,
        // recursively.
        delete [] po.points;
        return (to_poly(yl0, pts, num, maxv));
    }
}


// Static function.
// Return a group struct containing the groups of elements that are
// mutually connected, 'this' is consumed.  If max_in_grp is nonzero,
// groups are *approximately* limited to this number of entries.  In
// this case, the groups are not necessarily disjoint
//
Zgroup *
Ylist::group(Ylist *thisyl, int max_in_grp)
{
    TimeDbgAccum ac("group");

    struct gtmp_t
    {
        static void destroy(gtmp_t *g)
            {
                while (g) {
                    gtmp_t *x = g;
                    g = g->next;
                    delete x;
                }
            }

        gtmp_t *next;
        Zlist *lst[256];
    } *g0 = 0, *ge = 0;

    Ylist *yl0 = thisyl;

    int gcnt = 0;
    while (yl0) {
        Zlist *ze;
        yl0 = yl0->first(&ze);
        if (!ze)
            break;

        int gc = gcnt & 0xff;
        if (!gc) {
            if (!g0)
                g0 = ge = new gtmp_t;
            else {
                ge->next = new gtmp_t;
                ge = ge->next;
            }
            ge->next = 0;
            ge->lst[0] = ze;
        }
        else
            ge->lst[gc] = ze;

        gcnt++;

        int cnt = 1;
        if (yl0) {
            for (Zlist *z = ze; z; z = z->next) {
                Zlist *zret;
                yl0 = yl0->touching(&zret, &z->Z);
                if (zret) {
                    while (ze->next) {
                        ze = ze->next;
                        cnt++;
                    }
                    ze->next = zret;
                }
                if (!yl0)
                    break;
                if (max_in_grp > 0 && cnt > max_in_grp)
                    break;
            }
        }
    }
    Zgroup *g = new Zgroup;
    g->num = gcnt;
    g->list = new Zlist*[gcnt+1];

    Zlist **p = g->list;
    for (gtmp_t *gt = g0; gt; gt = gt->next) {
        int n = mmMin(gcnt, 256);
        for (int i = 0; i < n; i++)
            *p++ = gt->lst[i];
        gcnt -= n;
    }
    gtmp_t::destroy(g0);

    g->list[g->num] = 0;
    return (g);
}


// Static function.
// Remove, recursively, all zoids that are connected to the AOI
// border.
//
Ylist *
Ylist::remove_backg(Ylist *thisyl, const BBox *AOI)
{
    if (!thisyl)
        return (0);
    Zlist *z0 = 0;
    z0 = new Zlist(-CDinfinity, AOI->top, CDinfinity, CDinfinity, z0);
    Zlist *ze = z0;
    z0 = new Zlist(-CDinfinity, AOI->bottom, AOI->left, AOI->top, z0);
    z0 = new Zlist(AOI->right, AOI->bottom, CDinfinity, AOI->top, z0);
    z0 = new Zlist(-CDinfinity, -CDinfinity, CDinfinity, AOI->bottom, z0);

    Ylist *yl0 = thisyl;
    while (z0) {
        Zlist *zret;
        yl0 = yl0->touching(&zret, &z0->Z);
        if (zret) {
            while (ze->next)
                ze = ze->next;
            ze->next = zret;
        }
        if (!yl0)
            break;
        Zlist *zx = z0;
        z0 = z0->next;
        delete zx;
    }
    return (yl0);
}


// Static function.
// Pull the "first" zoid, and all those that recursively touch it, out
// of this and return them in zp.
//
Ylist *
Ylist::connected(Ylist *thisyl, Zlist **zp)
{
    *zp = 0;
    Ylist *yl0 = thisyl;
    if (yl0) {
        Zlist *ze;
        yl0 = yl0->first(&ze);
        if (!ze)
            return (0);
        *zp = ze;
        if (yl0) {
            for (Zlist *z = ze; z; z = z->next) {
                Zlist *zret;
                yl0 = yl0->touching(&zret, &z->Z);
                if (zret) {
                    while (ze->next)
                        ze = ze->next;
                    ze->next = zret;
                }
                if (!yl0)
                    break;
            }
        }
    }
    return (yl0);
}


// Static function.
// If y1 is null, compute all "scan" values for this, which are zoid
// tops and bottoms, and edge y-intersections not on a top or bottom. 
// If y1 is not null, compute the edge intersections between zoids of
// the two lists that are not on a zoid top or bottom.
//
void
Ylist::scanlines(const Ylist *thisyl, const Ylist *y1, intDb &idb)
{
    if (!thisyl)
        return;
    if (y1) {
        const Ylist *y0 = thisyl;
        if (y1->y_yu > y0->y_yu) {
            const Ylist *yt = y0;
            y0 = y1;
            y1 = yt;
        }
        for ( ; y0; y0 = y0->next) {
            for (Zlist *z0 = y0->y_zlist; z0; z0 = z0->next) {

                ovlchk_t ovl(z0->Z.minleft(), z0->Z.maxright(), z0->Z.yu);
                for (const Ylist *y = y1; y; y = y->next) {
                    if (y->y_yl >= z0->Z.yu)
                        continue;
                    if (y->y_yu <= z0->Z.yl)
                        break;
                    for (Zlist *z = y->y_zlist; z; z = z->next) {
                        if (ovl.check_break(z->Z))
                            break;
                        if (ovl.check_continue(z->Z))
                            continue;
                        int yvals[12];
                        int sz;
                        z0->Z.scanlines(&z->Z, yvals, &sz);
                        if (sz) {
                            int i = 2;
                            if (z0->Z.yl != z->Z.yl)
                                i++;
                            if (z0->Z.yu != z->Z.yu)
                                i++;
                            for ( ; i < sz; i++)
                                idb.add(yvals[i]);
                        }
                    }
                }
            }
        }
        return;
    }

    for (const Ylist *y0 = thisyl; y0; y0 = y0->next) {
        idb.add(y0->y_yu);
        for (Zlist *z0 = y0->y_zlist; z0; z0 = z0->next) {
            idb.add(z0->Z.yl);

            ovlchk_t ovl(z0->Z.minleft(), z0->Z.maxright(), z0->Z.yu);
            for (Zlist *z = z0->next; z; z = z->next) {
                if (ovl.check_break(z->Z))
                    break;
                if (ovl.check_continue(z->Z))
                    continue;
                int yvals[12];
                int sz;
                z0->Z.scanlines(&z->Z, yvals, &sz);
                if (sz) {
                    // Skip the bottoms/tops, only need edge crossings.
                    int i = 2;
                    if (z0->Z.yl != z->Z.yl)
                        i++;
                    if (z0->Z.yu != z->Z.yu)
                        i++;
                    for ( ; i < sz; i++)
                        idb.add(yvals[i]);
                }
            }
            for (Ylist *y = y0->next; y; y = y->next) {
                if (y->y_yl >= z0->Z.yu)
                    continue;
                if (y->y_yu <= z0->Z.yl)
                    break;
                for (Zlist *z = y->y_zlist; z; z = z->next) {
                    if (ovl.check_break(z->Z))
                        break;
                    if (ovl.check_continue(z->Z))
                        continue;
                    int yvals[12];
                    int sz;
                    z0->Z.scanlines(&z->Z, yvals, &sz);
                    if (sz) {
                        int i = 2;
                        if (z0->Z.yl != z->Z.yl)
                            i++;
                        if (z0->Z.yu != z->Z.yu)
                            i++;
                        for ( ; i < sz; i++)
                            idb.add(yvals[i]);
                    }
                }
            }
        }
    }
}


// Static function.
// Cut all zoids at the passed y values.  Return a new Ylist, this is
// consumed.  The scans array is sorted ascending.
//
Ylist *
Ylist::slice(Ylist *thisyl, const int *scans, int nscans) THROW_XIrt
{
    if (!thisyl)
        return (0);
    Ylist *y0 = thisyl;
    Zlist *z0 = 0;
    int indx = nscans - 1;
    while (y0) {
        Zlist *zl;
        y0 = y0->first(&zl);
        if (!zl)
            continue;

        if (checkInterrupt()) {
            destroy(y0);
            throw XIintr;
        }

        double sl = zl->Z.slope_left();
        double sr = zl->Z.slope_right();
        while (zl->Z.yu <= scans[indx] && indx)
            indx--;
        int ix = indx;
        while (scans[ix] > zl->Z.yl) {
            int dy = scans[ix] - zl->Z.yl;
            int xl = mmRnd(zl->Z.xll + sl*dy);
            int xr = mmRnd(zl->Z.xlr + sr*dy);
            Zoid zt(xl, xr, scans[ix], zl->Z.xul, zl->Z.xur, zl->Z.yu);
            if (!zt.is_bad()) 
                z0 = new Zlist(&zt, z0);
            zl->Z.xul = xl;
            zl->Z.xur = xr;
            zl->Z.yu = scans[ix];
            ix--;
            if (ix < 0)
                break;  // just in case...
        }
        if (zl->Z.is_bad())
            delete zl;
        else {
            zl->next = z0;
            z0 = zl;
        }
    }
    y0 = new Ylist(z0);
    return (y0);
}


// Static function.
// Clip and merge the list using the "scan lines", so as to produce a
// representation where each zoid is as wide as possible.
//
// This will process the trapezoids by group, which is generally far
// more efficient.
//
// On interrupt, 'this' is freed.
//
Zlist *
Ylist::repartition(Ylist *thisyl) THROW_XIrt
{
    TimeDbgAccum ac("repartition");

    Ylist *yl0 = thisyl;
    Zlist *zl0 = 0;

    try {
        while (yl0) {
            Zlist *ze;
            yl0 = yl0->first(&ze);
            if (!ze)
                break;

            Zlist *zl = ze;
            if (yl0) {
                for (Zlist *z = ze; z; z = z->next) {
                    Zlist *zret;
                    yl0 = yl0->touching(&zret, &z->Z);
                    if (zret) {
                        while (ze->next)
                            ze = ze->next;
                        ze->next = zret;
                    }
                    if (!yl0)
                        break;
                }
            }
            if (!zl->next) {
                zl->next = zl0;
                zl0 = ze;
                continue;
            }

            Ylist *yl = new Ylist(zl);
            zl = to_zlist(repartition_group(yl));
            if (zl) {
                Zlist *zn = zl;
                while (zn->next)
                    zn = zn->next;
                zn->next = zl0;
                zl0 = zl;
            }
        }
        return (zl0);
    }
    catch (XIrt) {
        destroy(yl0);
        Zlist::destroy(zl0);
        throw;
    }
}


// Static function.
// As above, but ignore interrupts.
//
Zlist *
Ylist::repartition_ni(Ylist *thisyl)
{
    if (!thisyl)
        return (0);
    CD()->SetIgnoreIntr(true);
    Zlist *zl = repartition(thisyl);
    CD()->SetIgnoreIntr(false);
    return (zl);
}


// Static function.
// Clip and merge the list using the "scan lines", so as to produce a
// representation where each zoid is as wide as possible.
//
// Call this on a group of zoids known to intersect, call reposition
// on a general collection.
//
// On interrupt, 'this' is freed.
//
Ylist *
Ylist::repartition_group(Ylist *thisyl) THROW_XIrt
{
    // Check trivial cases.
    Ylist *y0 = thisyl;
    if (!y0)
        return (0);
    if (!y0->next) {
        if (!y0->y_zlist) {
            delete y0;
            return (0);
        }
        if (!y0->y_zlist->next)
            return (y0);
    }

    // First find the "scan lines" where we will cut horizontally. 
    // These are zoid tops and bottoms, plus (and importantly) edge
    // intersections not on a top or bottom.

    intDb idb;
    scanlines(y0, 0, idb);
    int *scans;
    int nscans;
    idb.list(&scans, &nscans);
    // The scanlines (y values) are in ascending order.

    if (checkInterrupt()) {
        delete [] scans;
        destroy(y0);
        throw XIintr;
    }

    // Now clip each zoid at the scan lines.
    try {
        y0 = slice(y0, scans, nscans);
        delete [] scans;
    }
    catch (XIrt) {
        delete [] scans;
        throw;
    }

    // Now, there is no overlap between scans, and every zoid in a
    // scan has the same height.  We need to merge the scans.

    for (Ylist *y = y0; y; y = y->next) {

        if (checkInterrupt()) {
            destroy(y0);
            throw XIintr;
        }

        scl_merge(y->y_zlist);
    }

    // Finally, attempt to merge adjacent zoids to reduce count.
    col_row_merge(y0);
    return (y0);
}


// Static function.
// It is known that yend is a single row that is not linked, which
// would logically lie at the end of this.  Attempt to merge the
// elements, and if it is not merged away, link yend to the the end. 
// Used by polygon splitting code.
//
bool
Ylist::merge_end_row(Ylist *thisyl, Ylist *yend)
{
    if (!yend)
        return (true);
    Ylist *yp = 0;
    for (Ylist *y = thisyl; y; yp = y, y = y->next) {
        if (y->y_yl < yend->y_yu)
            continue;

        Zlist *z2p = 0;
        for (Zlist *zl = y->y_zlist; zl; zl = zl->next) {
            if (zl->Z.yl != yend->y_yu)
                continue;
            for (Zlist *zl2 = z2p ? z2p->next : yend->y_zlist; zl2;
                    z2p = zl2, zl2 = zl2->next) {

                // The trapezoids are inherently clipped/merged from
                // poly splitting, so logic below is ok.
                //
                if (zl2->Z.xul < zl->Z.xll)
                    continue;
                if (zl2->Z.xul > zl->Z.xll)
                    break;
                 
                if (zl->Z.join_below(zl2->Z)) {
                    if (y->y_yl > zl2->Z.yl)
                        y->y_yl = zl2->Z.yl;
                    yend->remove_next(z2p, zl2);
                    delete zl2;
                    if (!yend->y_zlist) {
                        delete yend;
                        return (true);
                    }
                    break;
                }
            }
        }
    }
    yp->next = yend;
    return (true);
}


// Static function.
// Purge slivers.
//
Ylist *
Ylist::filter_slivers(Ylist *thisyl, int d)
{
    Ylist *yl0 = thisyl;
    Ylist *yp = 0, *yn;
    for (Ylist *y = yl0; y; y = yn) {
        yn = y->next;

        y->y_zlist = Zlist::filter_slivers(y->y_zlist, d);
        if (!y->y_zlist) {
            if (!yp)
                yl0 = yn;
            else
                yp->next = yn;
            delete y;
            continue;
        }
        yp = y;
    }
    return (yl0);
}


// Static function.
// Return a list of the intersection areas, 'this' is destroyed.
// On exception: 'this' is freed.
//
// This should be "safe", see note in Ylist::clip_to_zoid(Zoid*)
// below.  No zoid is clipped more than once.
//
Zlist *
Ylist::clip_to_self(Ylist *thisyl) THROW_XIrt
{
    if (!thisyl)
        return (0);
    Zlist *z0 = 0, *ze = 0, *zl;
    Ylist *yl0 = thisyl->first(&zl);
    if (zl) {
        for (;;) {
            if (!yl0) {
                delete zl;
                break;
            }
            if (checkInterrupt()) {
                delete zl;
                destroy(yl0);
                Zlist::destroy(z0);
                throw (XIintr);
            }
            Zlist *zret = clip_to_zoid(yl0, &zl->Z);
            delete zl;
            if (zret) {
                if (!z0)
                    z0 = ze = zret;
                else
                    ze->next = zret;
                while (ze->next)
                    ze = ze->next;
            }
            yl0 = yl0->first(&zl);
        }
    }
    return (z0);
}


// Static function.
// Return the zoids clipped to Z, this is not touched.
//
// I think that this should be "safe" as the same zoid is used as a
// template for all clipping, and no zoid is clipped more than once. 
// Problems arise when a zoid is clipped into smaller zoids, which are
// clipped again, etc.  That is not the case here.
//
Zlist *
Ylist::clip_to_zoid(const Ylist *thisyl, const Zoid *Z)
{
    if (!thisyl)
        return (0);
    Zlist *z0 = 0, *ze = 0;
    for (const Ylist *y = thisyl; y; y = y->next) {
        if (y->y_yl >= Z->yu)
            continue;
        if (y->y_yu <= Z->yl)
            break;
        ovlchk_t ovl(Z->minleft(), Z->maxright(), Z->yu);
        for (Zlist *z = y->y_zlist; z; z = z->next) {
            if (ovl.check_break(z->Z))
                break;
            if (ovl.check_continue(z->Z))
                continue;
            Zlist *zt = Z->clip_to(&z->Z);
            if (zt) {
                if (ze) {
                    while (ze->next)
                        ze = ze->next;
                    ze->next = zt;
                }
                else
                    z0 = ze = zt;
            }
        }
    }
    return (z0);
}


// Static function.
// The original clip-to function for Ylists.
// Return a zoid list representing (this & yl0).  On exception: 
// 'this' and yl0 are untouched.
//
// This function is probably "safe" as no zoid is clipped more than
// once.  See note for Ylist::clip_to(Zoid*).
//
Zlist *
Ylist::clip_to_ylist(const Ylist *thisyl, const Ylist *yl0) THROW_XIrt
{
    Ymgr ym(yl0);
    Zlist *z0 = 0, *ze = 0;
    try {
        for (const Ylist *y = thisyl; y; y = y->next) {
            for (Zlist *zl1 = y->y_zlist; zl1; zl1 = zl1->next) {
                if (checkInterrupt())
                    throw (XIintr);
                ovlchk_t ovl(zl1->Z.minleft(), zl1->Z.maxright(), zl1->Z.yu);
                ym.reset();
                const Ylist *yy;
                while ((yy = ym.next(zl1->Z.yu)) != 0) {
                    if (yy->y_yu <= zl1->Z.yl)
                        break;
                    for (Zlist *zl2 = yy->y_zlist; zl2; zl2 = zl2->next) {
                        if (ovl.check_break(zl2->Z))
                            break;
                        if (ovl.check_continue(zl2->Z))
                            continue;
                        Zlist *zt = (zl1->Z).clip_to(&zl2->Z);
                        if (zt) {
                            if (ze) {
                                while (ze->next)
                                    ze = ze->next;
                                ze->next = zt;
                            }
                            else
                                z0 = ze = zt;
                        }
                    }
                }
            }
        }
        return (z0);
    }
    catch (XIrt) {
        Zlist::destroy(z0);
        throw;
    }
}


// Static function.
// New Ylist clip-out function, uses scan lines so is safe for
// all-angles.  Returns a list with the common areas clipped out.
//
// This should be faster than calling scl_clip_out directly.
//
Zlist *
Ylist::clip_out_self(const Ylist *thisyl) THROW_XIrt
{
    Zlist *z0 = 0;
    try {
        for (const Ylist *y = thisyl; y; y = y->next) {
            for (Zlist *z = y->y_zlist; z; z = z->next) {
                if (checkInterrupt()) {
                    Zlist::destroy(z0);
                    throw (XIintr);
                }

                // Grab a list of zoids that overlap.  We will employ
                // scanline clipping with this collection.  This
                // appears to be much faster than using scl_clip_out
                // directly on the entire list.

                Zlist *zb = thisyl->overlapping(&z->Z);
                if (!zb) {
                    // Won't include argument.
                    // No overlap, zoid is not clipped, save it.
                    z0 = new Zlist(&z->Z, z0);
                }
                else if (!zb->next) {
                    // Just one overlapping zoid, use the zoid clipper.
                    bool no_ovl;
                    Zlist *zx = z->Z.clip_out(&zb->Z, &no_ovl);
                    if (no_ovl) {
                        // Can't happen, we know that there is overlap.
                        z0 = new Zlist(&z->Z, z0);
                    }
                    else if (zx) {
                        Zlist *zn = zx;
                        while (zn->next)
                            zn = zn->next;
                        zn->next = z0;
                        z0 = zx;
                    }
                    delete zb;
                }
                else {
                    // More than one zoid, use the scanline clip-out.
                    Ylist *ya = new Ylist(new Zlist(&z->Z));
                    Ylist *yb = new Ylist(zb);
                    Zlist *zx = to_zlist(scl_clip_out_ylist(ya, yb));
                    if (zx) {
                        Zlist *zn = zx;
                        while (zn->next)
                            zn = zn->next;
                        zn->next = z0;
                        z0 = zx;
                    }
                }
            }
        }
        return (z0);
    }
    catch (XIrt) {
        Zlist::destroy(z0);
        throw;
    }
}


// Static function.
// Clip this (destructively) around Z, returning the clipped Ylist,
// which can be nil.
//
// The new version should be be "safe", zoids are clipped around Z,
// and the fragments replace the original zoid in this.
//
// WARNING: Calling this again for a different zoid is unsafe in the
// all-angle case, should use scl_clip_out.
//
// In the original version the fragments may be clipped around Z
// again, which is dangerous.
//
Ylist *
Ylist::clip_out_zoid(Ylist *thisyl, const Zoid *Z)
{
    // This version saves the fragments and reinserts after the
    // clipping pass, so that no fragment is re-clipped.  The
    // re-clipping might cause numerical artifacts in all-angle cases.

    Zlist *z0 = 0, *ze = 0;
    Ylist *yl0 = thisyl, *yp = 0, *yn = 0;
    for (Ylist *y = yl0; y; y = yn) {
        yn = y->next;
        if (y->y_yl >= Z->yu) {
            yp = y;
            continue;
        }
        if (y->y_yu <= Z->yl)
            break;
        Zlist *zn, *zp = 0;

        ovlchk_t ovl(Z->minleft(), Z->maxright(), Z->yu);
        for (Zlist *z = y->y_zlist; z; z = zn) {
            zn = z->next;
            if (ovl.check_break(z->Z))
                break;
            if (ovl.check_continue(z->Z)) {
                zp = z;
                continue;
            }

            bool no_ovl;
            Zlist *zt = z->Z.clip_out(Z, &no_ovl);
            if (no_ovl) {
                zp = z;
                continue;
            }
            y->remove_next(zp, z);
            delete z;
            if (zt) {
                if (ze) {
                    while (ze->next)
                        ze = ze->next;
                    ze->next = zt;
                }
                else
                    z0 = ze = zt;
            }
        }
        if (!y->y_zlist) {
            if (!yp)
                yl0 = yn;
            else
                yp->next = yn;
            delete y;
            continue;
        }
        yp = y;
    }
    if (z0) {
        if (!yl0)
            yl0 = new Ylist(z0);
        else
            yl0->insert(z0);
    }
    return (yl0);
}


// Static function.
// New Ylist clip-out function, uses scan lines so is safe for
// all-angles.  Ylists are not touched.
//
Zlist *
Ylist::clip_out_ylist(const Ylist *thisyl, const Ylist *yr) THROW_XIrt
{
    Zlist *z0 = 0;
    try {
        for (const Ylist *y = thisyl; y; y = y->next) {
            for (Zlist *zl1 = y->y_zlist; zl1; zl1 = zl1->next) {
                if (checkInterrupt())
                    throw (XIintr);

                // Grab a list of zoids that overlap.  We will employ
                // scanline clipping with this collection.  This
                // appears to be much faster than using scl_clip_out
                // directly on the entire list.

                Zlist *zb = yr->overlapping(&zl1->Z);
                if (!zb) {
                    // No overlap, zoid is not clipped, save it.
                    z0 = new Zlist(&zl1->Z, z0);
                    continue;
                }
                if (!zb->next) {
                    // Just one overlapping zoid, use the zoid clipper.
                    bool no_ovl;
                    Zlist *zx = zl1->Z.clip_out(&zb->Z, &no_ovl);
                    if (no_ovl) {
                        // Can't happen, we know that there is overlap.
                        z0 = new Zlist(&zl1->Z, z0);
                    }
                    else if (zx) {
                        Zlist *zn = zx;
                        while (zn->next)
                            zn = zn->next;
                        zn->next = z0;
                        z0 = zx;
                    }
                    delete zb;
                    continue;
                }

                // More than one zoid, use the scanline clip-out.
                Ylist *ya = new Ylist(new Zlist(&zl1->Z));
                Ylist *yb = new Ylist(zb);
                Zlist *zx = to_zlist(scl_clip_out_ylist(ya, yb));
                if (zx) {
                    Zlist *zn = zx;
                    while (zn->next)
                        zn = zn->next;
                    zn->next = z0;
                    z0 = zx;
                }
            }
        }
        return (z0);
    }
    catch (XIrt) {
        Zlist::destroy(z0);
        throw;
    }
}


namespace {
    // Clip left to right.  Zoids are from the same scanline band and
    // all have the same height, and no edges cross if height > 1.
    //
    Zlist *band_clip_to(const Zlist *zleft, const Zlist *zright)
    {
        Zlist *z0 = 0;
        for (const Zlist *za = zleft; za; za = za->next) {
            ovlchk_t ovl(za->Z.minleft(), za->Z.maxright(), za->Z.yu);
            for (const Zlist *zb = zright; zb; zb = zb->next) {
                if (ovl.check_break(zb->Z))
                    break;
                if (ovl.check_continue(zb->Z))
                    continue;

                int xul = mmMax(za->Z.xul, zb->Z.xul);
                int xur = mmMin(za->Z.xur, zb->Z.xur);
                int xll = mmMax(za->Z.xll, zb->Z.xll);
                int xlr = mmMin(za->Z.xlr, zb->Z.xlr);
                if (xul <= xur && xll <= xlr) {
                   Zoid zt(xll, xlr, za->Z.yl, xul, xur, za->Z.yu);
                   if (!zt.is_bad())
                       z0 = new Zlist(&zt, z0);
                }
            }
        }
        return (z0);
    }


    // Compute all the scan lines for the two lists and cross terms,
    // and slice the two lists at the scans.
    //
    void slice_scans(Ylist **pyl, Ylist **pyr) THROW_XIrt
    {
        TimeDbgAccum ac("slice_scans");

        Ylist *yl = *pyl;
        Ylist *yr = *pyr;
        intDb idb;
        Ylist::scanlines(yl, 0, idb);
        Ylist::scanlines(yr, 0, idb);
        if (yr)
            Ylist::scanlines(yl, yr, idb);

        int *scans, nscans;
        idb.list(&scans, &nscans);

        try {
            *pyl = Ylist::slice(yl, scans, nscans);
        }
        catch (XIrt) {
            Ylist::destroy(yr);
            *pyl = 0;
            *pyr = 0;
            delete [] scans;
            throw;
        }
        try {
            *pyr = Ylist::slice(yr, scans, nscans);
        }
        catch (XIrt) {
            Ylist::destroy(*pyl);
            *pyl = 0;
            *pyr = 0;
            delete [] scans;
            throw;
        }
        delete [] scans;
    }
}


// Static function.
// A clip-to function based on scan lines.
//
// This is not used.  The clip-to operation does not clip zoids
// multiply and sequentially and therefor does not produce accumulated
// vertex placement errors in all-angle collections.  The non
// scan-line clip-to is much faster.
//
Ylist *
Ylist::scl_clip_to_ylist(Ylist *thisyl, Ylist *y) THROW_XIrt
{
    Ylist *y0 = thisyl;
    Ylist *yb = y;
    if (!y0 || !yb) {
        destroy(y0);
        destroy(yb);
        return (0);
    }
    slice_scans(&y0, &yb);

    // Run down the lists, processing each band.  We know that if
    // zoids exist in a band, all have the same height, and none of
    // the edges overlap if height > 1.  We keep the results in y0.

    Ylist *ya = y0;
    while (ya || yb) {
        if (ya && (!yb || ya->y_yu > yb->y_yu)) {
            ya->set_zlist(0);
            ya = ya->next;
            continue;
        }
        if (ya && ya->y_yu == yb->y_yu) {
            if (!ya->y_zlist || !yb->y_zlist)
                ya->set_zlist(0);
            else {
                Zlist *z0 = band_clip_to(ya->y_zlist, yb->y_zlist);
                z0 = Zlist::sort(z0, 1);
                scl_merge(z0);
                ya->set_zlist(z0);
            }
            if (checkInterrupt()) {
                destroy(y0);
                destroy(yb);
                throw XIintr;
            }
            ya = ya->next;
        }
        yb = clear_to_next(yb);
    }
    y0 = strip_empty(y0);
    col_row_merge(y0);
    return (y0);
}


namespace {
    // Clip the common areas out of the list.  Zoids are from the same
    // scanline band and all have the same height, and no edges cross
    // if height > 1.  The passed list is consumed.
    //
    Zlist *band_clip_out(Zlist *z0)
    {
        Zlist *zap = 0, *zn;
        for (Zlist *za = z0; za; za = zn) {
            zn = za->next;
            ovlchk_t ovl(za->Z.minleft(), za->Z.maxright(), za->Z.yu);
            Zoid &Za = za->Z;
            for (const Zlist *zb = zn; zb; zb = zb->next) {
                const Zoid &Zb = zb->Z;
                if (ovl.check_break(Zb))
                    break;
                if (ovl.check_continue(Zb))
                    continue;
                if (Za.xlr <= Zb.xll && Za.xur <= Zb.xul)
                    continue;
                if (Zb.xlr <= Za.xll && Zb.xur <= Za.xul)
                    continue;

                Zoid Zl(Za.xll, mmMin(Za.xlr, Zb.xll), Za.yl,
                    Za.xul, mmMin(Za.xur, Zb.xul), Za.yu);
                Zoid Zr(mmMax(Za.xll, Zb.xlr), Za.xlr, Za.yl,
                    mmMax(Za.xul, Zb.xur), Za.xur, Za.yu);

                bool lok = (Zl.xur >= Zl.xul && Zl.xlr >= Zl.xll &&
                    !Zl.is_bad());
                bool rok = (Zr.xur >= Zr.xul && Zr.xlr >= Zr.xll &&
                    !Zr.is_bad());
                if (!lok) {
                    if (!rok) {
                        if (zap)
                            zap->next = zn;
                        else
                            z0 = zn;
                        delete za;
                    }
                    else {
                        za->Z = Zr;
                        zn = za;
                    }
                    za = 0;
                }
                else {
                    za->Z = Zl;
                    if (rok) {
                        za->next = new Zlist(&Zr, zn);
                        zn = za->next;
                    }
                }
                break;
            }
            if (za)
                zap = za;
        }
        return (z0);
    }
}


// Static function.
// A self clip-out based on scan lines.  This may be less prone to
// artifacts in all-angle collections than other methods.  The 'this'
// list is destroyed.
//
Ylist *
Ylist::scl_clip_out_self(Ylist *thisyl) THROW_XIrt
{
    Ylist *y0 = thisyl;
    if (!y0)
        return (0);
    if (!y0->next) {
        if (!y0->y_zlist) {
            delete y0;
            return (0);
        }
        if (!y0->y_zlist->next)
            return (y0);
    }

    // First find the "scan lines" where we will cut horizontally. 
    // These are zoid tops and bottoms, plus (and importantly) edge
    // intersections not on a top or bottom.

    intDb idb;
    scanlines(y0, 0, idb);
    int *scans;
    int nscans;
    idb.list(&scans, &nscans);
    // The scanlines (y values) are in ascending order.

    if (checkInterrupt()) {
        delete [] scans;
        destroy(y0);
        throw XIintr;
    }

    // Now clip each zoid at the scan lines.
    try {
        y0 = slice(y0, scans, nscans);
        delete [] scans;
    }
    catch (XIrt) {
        delete [] scans;
        throw;
    }

    // Now, there is no overlap between scans, and every zoid in a
    // scan has the same height.

    for (Ylist *y = y0; y; y = y->next) {

        if (checkInterrupt()) {
            destroy(y0);
            throw XIintr;
        }

        Zlist *zl = band_clip_out(y->y_zlist);
        zl = Zlist::sort(zl, 1);
        scl_merge(zl);
        y->y_zlist = zl;
    }
    col_row_merge(y0);
    return (y0);
}


namespace {
    // Clip right out of left.  Zoids are from the same scanline band
    // and all have the same height, and no edges cross if height > 1.
    //
    Zlist *band_clip_out(const Zlist *zleft, const Zlist *zright)
    {
        Zlist *z0 = Zlist::copy(zleft);
        Zlist *zap = 0, *zn;
        for (Zlist *za = z0; za; za = zn) {
            zn = za->next;
            ovlchk_t ovl(za->Z.minleft(), za->Z.maxright(), za->Z.yu);
            Zoid &Za = za->Z;
            for (const Zlist *zb = zright; zb; zb = zb->next) {
                const Zoid &Zb = zb->Z;
                if (ovl.check_break(Zb))
                    break;
                if (ovl.check_continue(Zb))
                    continue;
                if (Za.xlr <= Zb.xll && Za.xur <= Zb.xul)
                    continue;
                if (Zb.xlr <= Za.xll && Zb.xur <= Za.xul)
                    continue;

                Zoid Zl(Za.xll, mmMin(Za.xlr, Zb.xll), Za.yl,
                    Za.xul, mmMin(Za.xur, Zb.xul), Za.yu);
                Zoid Zr(mmMax(Za.xll, Zb.xlr), Za.xlr, Za.yl,
                    mmMax(Za.xul, Zb.xur), Za.xur, Za.yu);

                bool lok = (Zl.xur >= Zl.xul && Zl.xlr >= Zl.xll &&
                    !Zl.is_bad());
                bool rok = (Zr.xur >= Zr.xul && Zr.xlr >= Zr.xll &&
                    !Zr.is_bad());
                if (!lok) {
                    if (!rok) {
                        if (zap)
                            zap->next = zn;
                        else
                            z0 = zn;
                        delete za;
                    }
                    else {
                        za->Z = Zr;
                        zn = za;
                    }
                    za = 0;
                }
                else {
                    za->Z = Zl;
                    if (rok) {
                        za->next = new Zlist(&Zr, zn);
                        zn = za->next;
                    }
                }
                break;
            }
            if (za)
                zap = za;
        }
        return (z0);
    }
}


// Static function.
// A clip-out based on scan lines.  This may be less prone to
// artifacts in all-angle collections than other methods.  Both this
// and the passed list are destroyed.
//
// This avoids vertex location errors seen in the original non
// scan-line clip-out function.  In the original clip-out algorithm, a
// zoid would be clipped, and the resulting pieces would be clipped
// again by other zoids, and so on.  The problem is that the angles in
// the pieces are likely a little different than the angles in the
// original zoids, due to coordinate quantization.  This can cause
// vertex locations to not be well defined, which can cause
// aberrations such as needles and gaps in the result.
//
// With the scan lines, the clipping is done at each scan line before
// the and-not operation, so the angles are based on the original zoid
// angles, and vertex locations are well defined.
//
// Probably, one should use clip_out rather than calling this function
// directly, for performance reasons.
//
Ylist *
Ylist::scl_clip_out_ylist(Ylist *thisyl, Ylist *y) THROW_XIrt
{
    Ylist *y0 = thisyl;
    Ylist *yb = y;
    if (!y0) {
        destroy(yb);
        return (0);
    }
    if (!yb)
        return (y0);
    slice_scans(&y0, &yb);

    // Run down the lists, processing each band.  We know that if
    // zoids exist in a band, all have the same height, and none of
    // the edges overlap if height > 1.

    Ylist *ya = y0;
    while (ya || yb) {
        if (ya && (!yb || ya->y_yu > yb->y_yu)) {
            ya = ya->next;
            continue;
        }
        if (ya && ya->y_yu == yb->y_yu) {
            if (ya->y_zlist && yb->y_zlist) {
                Zlist *z0 = band_clip_out(ya->y_zlist, yb->y_zlist);
                z0 = Zlist::sort(z0, 1);
                scl_merge(z0);
                ya->set_zlist(z0);
            }
            if (checkInterrupt()) {
                destroy(y0);
                destroy(yb);
                throw XIintr;
            }
            ya = ya->next;
        }
        yb = clear_to_next(yb);
    }
    y0 = strip_empty(y0);
    col_row_merge(y0);
    return (y0);
}


// Static function.
// As above, but also return y - this in *py.
//
Ylist *
Ylist::scl_clip_out2_ylist(Ylist *thisyl, Ylist **py) THROW_XIrt
{
    Ylist *y0 = thisyl;
    Ylist *y1 = *py;
    if (!y0)
        return (0);
    if (!y1)
        return (y0);
    slice_scans(&y0, &y1);

    // Run down the lists, processing each band.  We know that if
    // zoids exist in a band, all have the same height, and none of
    // the edges overlap if height > 1.

    Ylist *ya = y0;
    Ylist *yb = y1;
    while (ya || yb) {
        if (ya && (!yb || ya->y_yu > yb->y_yu)) {
            ya = ya->next;
            continue;
        }
        if (ya && ya->y_yu == yb->y_yu) {
            if (ya->y_zlist && yb->y_zlist) {
                Zlist *z0 = band_clip_out(ya->y_zlist, yb->y_zlist);
                z0 = Zlist::sort(z0, 1);
                scl_merge(z0);
                Zlist *z1 = band_clip_out(yb->y_zlist, ya->y_zlist);
                z1 = Zlist::sort(z1, 1);
                scl_merge(z1);
                ya->set_zlist(z0);
                yb->set_zlist(z1);
            }
            if (checkInterrupt()) {
                destroy(y0);
                destroy(y1);
                throw XIintr;
            }
            ya = ya->next;
        }
        yb = yb->next;
    }
    y0 = strip_empty(y0);
    y1 = strip_empty(y1);
    try {
        col_row_merge(y0);
    }
    catch (XIrt) {
        destroy(y1);
        throw;
    }
    try {
        col_row_merge(y1);
    }
    catch (XIrt) {
        destroy(y0);
        throw;
    }
    *py = y1;
    return (y0);
}


namespace {
    // Xor the lists.  Zoids are from the same scanline band and all
    // have the same height, and no edges cross if height > 1.
    //
    Zlist *band_clip_xor(const Zlist *zleft, const Zlist *zright)
    {
        Zlist *z12 = band_clip_out(zleft, zright);
        Zlist *z21 = band_clip_out(zright, zleft);
        if (!z12)
            z12 = z21;
        else if (z21) {
            Zlist *zn = z12;
            while (zn->next)
                zn = zn->next;
            zn->next = z21;
        }
        return (z12);
    }
}


// Static function.
// A clip-xor function based on scan lines.  This may be less prone
// to artifacts in all-angle collections than other methods.  Both
// this and the passed list are destroyed.
//
Ylist *
Ylist::scl_clip_xor_ylist(Ylist *thisyl, Ylist *y) THROW_XIrt
{
    Ylist *y0 = thisyl;
    Ylist *yb = y;
    if (!y0)
        return (yb);
    if (!yb)
        return (y0);
    slice_scans(&y0, &yb);

    // Run down the lists, processing each band.  We know that if
    // zoids exist in a band, all have the same height, and none of
    // the edges overlap if height > 1.

    Ylist *ya = y0;
    Ylist *yap = 0;
    while (ya || yb) {
        if (ya && (!yb || ya->y_yu > yb->y_yu)) {
            yap = ya;
            ya = ya->next;
            continue;
        }
        if (yb && !ya) {
            // Append the remainder of the b list onto a.
            if (yap)
                yap->next = yb;
            else
                y0 = yb;
            break;
        }
        if (yb->y_yu > ya->y_yu) {
            // Add yb to the a list before ya.
            if (yap)
                yap->next = yb;
            else
                y0 = yb;
            Ylist *yx = yb;
            yb = yb->next;
            yx->next = ya;
            yap = yx;
        }
        else {
            if (!ya->y_zlist) {
                ya->y_zlist = yb->y_zlist;
                yb->y_zlist = 0;
            }
            else if (yb->y_zlist) {
                Zlist *z0 = band_clip_xor(ya->y_zlist, yb->y_zlist);
                z0 = Zlist::sort(z0, 1);
                scl_merge(z0);
                ya->set_zlist(z0);
            }
            yap = ya;
            ya = ya->next;
            yb = clear_to_next(yb);
        }
        if (checkInterrupt()) {
            destroy(y0);
            destroy(yb);
            throw XIintr;
        }
    }
    y0 = strip_empty(y0);
    col_row_merge(y0);
    return (y0);
}


// Static function.
// Do some testing and print messages if errors found, for debugging.
//
bool
Ylist::debug(Ylist *thisyl)
{
    for (Ylist *y = thisyl; y; y = y->next) {
        if (!y->debug_row())
            return (false);
    }
    return (true);
}


bool
Ylist::debug_row()
{
    if (next && next->y_yu >= y_yu) {
        printf("FU 1: Ylist out of order\n");
        return (false);
    }
    Zlist *zp = 0;
    for (Zlist *z = y_zlist; z; z = z->next) {
        if (z->Z.yu != y_yu) {
            printf("FT 2: Zlist and Ylist yu differ\n");
            return (false);
        }

        if (z->Z.yl < y_yl) {
            printf("FU 3: Zlist yl out of range\n");
            return (false);
        }

        if (zp && z->Z.zcmp(&zp->Z) < 0) {
            printf("FU 5: Zlist out of order\n");
            zp->Z.print();
            z->Z.print();
            return (false);
        }
        zp = z;
    }
    return (true);
}


//--- Start of Ylist private functions

// For each row, join adjacent elements.  On exception:  'this' is
// freed.
//
bool
Ylist::merge_rows() THROW_XIrt
{
    bool change = false;
    for (Ylist *y = this; y; y = y->next) {
        if (checkInterrupt()) {
            destroy(this);
            throw (XIintr);
        }
        for (Zlist *zl1 = y->y_zlist; zl1; zl1 = zl1->next) {
            Zlist *zn;
            Zlist *zp = zl1;
            for (Zlist *zl2 = zl1->next; zl2; zl2 = zn) {
                if (zl2->Z.minleft() > zl1->Z.maxright())
                    break;
                zn = zl2->next;
                if (zl1->Z.join_right(zl2->Z)) {
                    zp->next = zn;
                    delete zl2;
                    change = true;
                    continue;
                }
                zp = zl2;
            }
        }
    }
    return (change);
}


// Join elements in the vertical sense.  On exception:  'this' is
// freed.
//
bool
Ylist::merge_cols() THROW_XIrt
{
    bool change = false;
    for (Ylist *y = this; y; y = y->next) {
        if (checkInterrupt()) {
            destroy(this);
            throw (XIintr);
        }
        bool rsort = false;
        Ylist *last_yp = 0;
        Zlist *last_zp1 = 0, *last_zp2 = 0;
        for (Zlist *zl1 = y->y_zlist; zl1; last_zp1 = zl1, zl1 = zl1->next) {

            Ylist *yp, *yn, *yy;
            Zlist *z2 = 0;
            if (last_yp && last_zp1->Z.yl == zl1->Z.yl
                   && last_zp1->Z.xll < zl1->Z.xll) {
                // If we are searching the same line as before,
                // we can skip the looping already done.
                yp = last_yp;
                yy = yp->next;
                if (yy) {
                    yn = yy->next;
                    z2 = last_zp2 && last_zp2->next ? last_zp2 : yy->y_zlist;
                    goto restart;
                }
            }

            yp = y;
            for (yy = y->next; yy; yy = yn) {
                yn = yy->next;

                if (yy->y_yu > zl1->Z.yl) {
                    yp = yy;
                    continue;
                }
                if (yy->y_yu < zl1->Z.yl)
                    break;

                last_yp = yp;
                last_zp2 = 0;
                z2 = yy->y_zlist;
restart:
                for (Zlist *zl2 = z2; zl2; zl2 = zl2->next) {

                    // Zoids are clipped/merged, so logic below is ok.
                    //
                    if (zl2->Z.xul < zl1->Z.xll) {
                        last_zp2 = zl2;
                        continue;
                    }
                    if (zl2->Z.xul > zl1->Z.xll)
                        break;

                    if (zl1->Z.join_below(zl2->Z)) {
                        if (y->y_yl > zl1->Z.yl)
                            y->y_yl = zl1->Z.yl;
                        yy->remove_next(last_zp2, zl2);
                        delete zl2;
                        change = true;
                        last_yp = 0;

                        // This may change the slopes slightly, causing the
                        // zoid to be out of order.  Check for this, will
                        // re-sort if necessary.
                        if (!rsort) {
                            if (last_zp1 && zl1->Z.zcmp(&last_zp1->Z) < 0)
                                rsort = true;
                            else if (zl1->next &&
                                    zl1->next->Z.zcmp(&zl1->Z) < 0)
                                rsort = true;
                        }
                    }
                    break;
                }
                if (!yy->y_zlist) {
                    yp->next = yn;
                    delete yy;
                    continue;
                }
                yp = yy;
            }
        }
        if (rsort)
            y->y_zlist = Zlist::sort(y->y_zlist, 1);
    }
    return (change);
}


// Private support function for to_poly().
// Find zoid to link on the bottom to Manhattan segment.
//
Ylist *
Ylist::find_bottom(geo_ylist::topoly_t *tp)
{
    Plist *p = tp->tp_p;
    int xmin = p->x;
    int xmax = p->next->x;
    int yval = p->y;
    Ylist *yl0 = this, *yp = 0;

    for (Ylist *y = yl0; y; yp = y, y = y->next) {
        if (y->y_yl > yval)
            continue;
        if (y->y_yu <= yval)
            break;
        Zlist *zp = 0;
        for (Zlist *z = y->y_zlist; z; zp = z, z = z->next) {

            // The zoids have been clipped/merged, so this should work
            // with minleft or xul sorting.
            //
            if (z->Z.xlr <= xmin)
                continue;
            if (z->Z.xll >= xmax) {
                if (z->Z.xul >= xmax)
                    break;
                continue;
            }
            if (z->Z.yl != yval)
                continue;

            y->remove_next(zp, z);
            z->next = 0;
            if (!y->y_zlist) {
                if (!yp)
                    yl0 = y->next;
                else
                    yp->next = y->next;
                delete y;
            }

            Plist *pn = p->next;
            int vc = 0;
            if (z->Z.xll != xmin)
                vc += tp->append_point(&p, z->Z.xll, z->Z.yl);
            vc += tp->append_point(&p, z->Z.xul, z->Z.yu);
            vc += tp->append_point(&p, z->Z.xur, z->Z.yu);
            if (z->Z.xlr != xmax)
                vc += tp->append_point(&p, z->Z.xlr, z->Z.yl);
            p->next = pn;
            delete z;
            tp->tp_again = true;
            tp->tp_vcnt += vc;
            return (yl0);
        }
    }
    return (yl0);
}


// Private support function for to_poly().
// Find zoid to link on the top to Manhattan segment.
//
Ylist *
Ylist::find_top(geo_ylist::topoly_t *tp)
{
    Plist *p = tp->tp_p;
    int xmin = p->next->x;
    int xmax = p->x;
    int yval = p->y;
    Ylist *yl0 = this, *yp = 0;

    for (Ylist *y = yl0; y; yp = y, y = y->next) {
        if (y->y_yu > yval)
            continue;
        if (y->y_yu < yval)
            break;
        Zlist *zp = 0;
        for (Zlist *z = y->y_zlist; z; zp = z, z = z->next) {

            // The zoids have been clipped/merged, so this should work
            // with minleft or xul sorting.
            //
            if (z->Z.xur <= xmin)
                continue;
            if (z->Z.xul >= xmax)
                break;

            y->remove_next(zp, z);
            z->next = 0;
            if (!y->y_zlist) {
                if (!yp)
                    yl0 = y->next;
                else
                    yp->next = y->next;
                delete y;
            }

            Plist *pn = p->next;
            int vc = 0;
            if (z->Z.xur != xmax)
                vc += tp->append_point(&p, z->Z.xur, z->Z.yu);
            vc += tp->append_point(&p, z->Z.xlr, z->Z.yl);
            vc += tp->append_point(&p, z->Z.xll, z->Z.yl);
            if (z->Z.xul != xmin)
                vc += tp->append_point(&p, z->Z.xul, z->Z.yu);
            p->next = pn;
            delete z;
            tp->tp_again = true;
            tp->tp_vcnt += vc;
            return (yl0);
        }
    }
    return (yl0);
}


// Private support function for to_poly().
// Find zoid to link on the left to Manhattan segment.
//
Ylist *
Ylist::find_left(geo_ylist::topoly_t *tp)
{
    Plist *p = tp->tp_p;
    int ymin = p->next->y;
    int ymax = p->y;
    int xval = p->x;
    Ylist *yl0 = this, *yp = 0;

    for (Ylist *y = yl0; y; yp = y, y = y->next) {
        if (y->y_yl >= ymax)
            continue;
        if (y->y_yu <= ymin)
            break;
        Zlist *zp = 0;
        for (Zlist *z = y->y_zlist; z; zp = z, z = z->next) {

            // The zoids have been clipped/merged, so this should work
            // with minleft or xul sorting.
            //
            if (z->Z.xul < xval)
                continue;
            if (z->Z.xul > xval)
                break;
            if (z->Z.yl >= ymax || z->Z.xll != xval)
                continue;

            y->remove_next(zp, z);
            z->next = 0;
            if (!y->y_zlist) {
                if (!yp)
                    yl0 = y->next;
                else
                    yp->next = y->next;
                delete y;
            }

            Plist *pn = p->next;
            int vc = 0;
            if (z->Z.yu != ymax)
                vc += tp->append_point(&p, z->Z.xul, z->Z.yu);
            vc += tp->append_point(&p, z->Z.xur, z->Z.yu);
            vc += tp->append_point(&p, z->Z.xlr, z->Z.yl);
            if (z->Z.yl != ymin)
                vc += tp->append_point(&p, z->Z.xll, z->Z.yl);
            p->next = pn;
            delete z;
            tp->tp_again = true;
            tp->tp_vcnt += vc;
            return (yl0);
        }
    }
    return (yl0);
}


// Private support function for to_poly().
// Find zoid to link on the right to Manhattan segment.
//
Ylist *
Ylist::find_right(geo_ylist::topoly_t *tp)
{
    Plist *p = tp->tp_p;
    int ymin = p->y;
    int ymax = p->next->y;
    int xval = p->x;
    Ylist *yl0 = this, *yp = 0;

    for (Ylist *y = yl0; y; yp = y, y = y->next) {
        if (y->y_yl >= ymax)
            continue;
        if (y->y_yu <= ymin)
            break;
        Zlist *zp = 0;
        for (Zlist *z = y->y_zlist; z; zp = z, z = z->next) {

            // The zoids have been clipped/merged, so this should work
            // with minleft or xul sorting.
            //
            if (z->Z.xur < xval)
                continue;
            if (z->Z.xur > xval)
                break;
            if (z->Z.yl >= ymax || z->Z.xlr != xval)
                continue;

            y->remove_next(zp, z);
            z->next = 0;
            if (!y->y_zlist) {
                if (!yp)
                    yl0 = y->next;
                else
                    yp->next = y->next;
                delete y;
            }

            Plist *pn = p->next;
            int vc = 0;
            if (z->Z.yl != ymin)
                vc += tp->append_point(&p, z->Z.xlr, z->Z.yl);
            vc += tp->append_point(&p, z->Z.xll, z->Z.yl);
            vc += tp->append_point(&p, z->Z.xul, z->Z.yu);
            if (z->Z.yu != ymax)
                vc += tp->append_point(&p, z->Z.xur, z->Z.yu);
            p->next = pn;
            delete z;
            tp->tp_again = true;
            tp->tp_vcnt += vc;
            return (yl0);
        }
    }
    return (yl0);
}


// Private support function for to_poly().
// Find zoid to link on the left to descending non-Manhattan segment.
//
Ylist *
Ylist::find_left_nm(geo_ylist::topoly_t *tp)
{
    Plist *p = tp->tp_p;
    Plist *pn = p->next;
    int xmin = mmMin(p->x, pn->x);
    int xmax = mmMax(p->x, pn->x);

    Ylist *yl0 = this, *yp = 0;
    for (Ylist *y = yl0; y; yp = y, y = y->next) {
        if (y->y_yl >= p->y)
            continue;
        if (y->y_yu <= pn->y)
            break;
        Zlist *zp = 0;
        for (Zlist *z = y->y_zlist; z; zp = z, z = z->next) {

            // The zoids have been clipped/merged, so this should work
            // with minleft or xul sorting.
            //
            if (z->Z.xll <= xmin && z->Z.xul <= xmin)
                continue;
            if (z->Z.xll >= xmax && z->Z.xul >= xmax)
                break;
            if (z->Z.yl >= p->y)
                continue;

            const int pval = 1;

            if (z->Z.yu == p->y) {
                if (abs(z->Z.xul - p->x) > pval)
                    continue;
            }
            else if (z->Z.yu > p->y) {
                if (!cGEO::check_colinear(z->Z.xll, z->Z.yl,
                        z->Z.xul, z->Z.yu, p->x, p->y, pval))
                    continue;
            }
            else {
                if (!cGEO::check_colinear(pn->x, pn->y,
                        p->x, p->y, z->Z.xul, z->Z.yu, pval))
                    continue;
            }

            if (z->Z.yl == pn->y) {
                if (abs(z->Z.xll - pn->x) > pval)
                    continue;
            }
            else if (z->Z.yl < pn->y) {
                if (!cGEO::check_colinear(z->Z.xll, z->Z.yl,
                        z->Z.xul, z->Z.yu, pn->x, pn->y, pval))
                    continue;
            }
            else {
                if (!cGEO::check_colinear(pn->x, pn->y,
                        p->x, p->y, z->Z.xll, z->Z.yl, pval))
                    continue;
            }

            y->remove_next(zp, z);
            z->next = 0;
            if (!y->y_zlist) {
                if (!yp)
                    yl0 = y->next;
                else
                    yp->next = y->next;
                delete y;
            }

            int vc = 0;
            if (z->Z.yu != p->y)
                vc += tp->append_point(&p, z->Z.xul, z->Z.yu);
            vc += tp->append_point(&p, z->Z.xur, z->Z.yu);
            vc += tp->append_point(&p, z->Z.xlr, z->Z.yl);
            if (z->Z.yl != pn->y)
                vc += tp->append_point(&p, z->Z.xll, z->Z.yl);
            p->next = pn;
            delete z;
            tp->tp_again = true;
            tp->tp_vcnt += vc;
            return (yl0);
        }
    }
    return (yl0);
}


// Private support function for to_poly().
// Find zoid to link on the right to ascending non-Manhattan segment.
//
Ylist *
Ylist::find_right_nm(geo_ylist::topoly_t *tp)
{
    Plist *p = tp->tp_p;
    Plist *pn = p->next;
    int xmin = mmMin(p->x, pn->x);
    int xmax = mmMax(p->x, pn->x);

    Ylist *yl0 = this, *yp = 0;
    for (Ylist *y = yl0; y; yp = y, y = y->next) {
        if (y->y_yl >= pn->y)
            continue;
        if (y->y_yu <= p->y)
            break;
        Zlist *zp = 0;
        for (Zlist *z = y->y_zlist; z; zp = z, z = z->next) {

            // The zoids have been clipped/merged, so this should work
            // with minleft or xul sorting.
            //
            if (z->Z.xlr <= xmin && z->Z.xur <= xmin)
                continue;
            if (z->Z.xlr >= xmax && z->Z.xur >= xmax)
                break;
            if (z->Z.yl >= pn->y)
                continue;

            const int pval = 1;

            if (z->Z.yl == p->y) {
                if (abs(z->Z.xlr - p->x) > pval)
                    continue;
            }
            else if (z->Z.yl < p->y) {
                if (!cGEO::check_colinear(z->Z.xlr, z->Z.yl,
                        z->Z.xur, z->Z.yu, p->x, p->y, pval))
                    continue;
            }
            else {
                if (!cGEO::check_colinear(p->x, p->y,
                        pn->x, pn->y, z->Z.xlr, z->Z.yl, pval))
                    continue;
            }

            if (z->Z.yu == pn->y) {
                if (abs(z->Z.xur - pn->x) > pval)
                    continue;
            }
            else if (z->Z.yu > pn->y) {
                if (!cGEO::check_colinear(z->Z.xlr, z->Z.yl,
                        z->Z.xur, z->Z.yu, pn->x, pn->y, pval))
                    continue;
            }
            else {
                if (!cGEO::check_colinear(p->x, p->y,
                        pn->x, pn->y, z->Z.xur, z->Z.yu, pval))
                    continue;
            }

            y->remove_next(zp, z);
            z->next = 0;
            if (!y->y_zlist) {
                if (!yp)
                    yl0 = y->next;
                else
                    yp->next = y->next;
                delete y;
            }

            int vc = 0;
            if (z->Z.yl != p->y)
                vc += tp->append_point(&p, z->Z.xlr, z->Z.yl);
            vc += tp->append_point(&p, z->Z.xll, z->Z.yl);
            vc += tp->append_point(&p, z->Z.xul, z->Z.yu);
            if (z->Z.yu != pn->y)
                vc += tp->append_point(&p, z->Z.xur, z->Z.yu);
            p->next = pn;
            delete z;
            tp->tp_again = true;
            tp->tp_vcnt += vc;
            return (yl0);
        }
    }
    return (yl0);
}


// Return a list of zoids that overlap Z.  If Z is from an element of
// this, don't return that element.
//
Zlist *
Ylist::overlapping(const Zoid *Z) const
{
    Zlist *z0 = 0;
    const Ylist *yl0 = this;
    ovlchk_t ovl(Z->minleft() - 1, Z->maxright() + 1, Z->yu + 1);
    for (const Ylist *y = yl0; y; y = y->next) {
        if (y->y_yl > Z->yu)
            continue;
        if (y->y_yu < Z->yl)
            break;
        for (Zlist *z = y->y_zlist; z; z = z->next) {
            if (ovl.check_break(z->Z))
                break;
            if (ovl.check_continue(z->Z))
                continue;
            if (Z == &z->Z)
                continue;
            if (Z->intersect(&z->Z, false))
                z0 = new Zlist(&z->Z, z0);
        }
    }
    return (z0);
}


// Private support function for group().
// Remove elements that touch or overlap Z.
//
Ylist *
Ylist::touching(Zlist **zret, const Zoid *Z)
{
    *zret = 0;
    Ylist *yl0 = this, *yp = 0, *yn;
    ovlchk_t ovl(Z->minleft() - 1, Z->maxright() + 1, Z->yu + 1);
    for (Ylist *y = yl0; y; y = yn) {
        yn = y->next;
        if (y->y_yl > Z->yu) {
            yp = y;
            continue;
        }
        if (y->y_yu < Z->yl)
            break;
        Zlist *zp = 0, *zn;
        for (Zlist *z = y->y_zlist; z; z = zn) {
            zn = z->next;
            if (ovl.check_break(z->Z))
                break;
            if (ovl.check_continue(z->Z)) {
                zp = z;
                continue;
            }
            if (Z->touching(&z->Z)) {
                y->remove_next(zp, z);
                z->next = *zret;
                *zret = z;
                continue;
            }
            zp = z;
        }
        if (!y->y_zlist) {
            if (!yp)
                yl0 = yn;
            else
                yp->next = yn;
            delete y;
            continue;
        }
        yp = y;
    }
    return (yl0);
}


// Remove and return the top left element.  Empty rows are deleted.
//
Ylist *
Ylist::first(Zlist **zp)
{
    *zp = 0;
    Ylist *yl0 = this, *yn;
    for (Ylist *y = yl0; y; y = yn) {
        yn = y->next;
        if (y->y_zlist) {
            *zp = y->y_zlist;
            y->remove_next(0, y->y_zlist);
            (*zp)->next = 0;
        }
        if (!y->y_zlist) {
            yl0 = yn;
            delete y;
        }
        if (*zp)
            break;
    }
    return (yl0);
}


// Add (destructively) z0 to this.  Each zoid *must* have yu <=
// this->yu.
//
void
Ylist::insert(Zlist *z0)
{
    z0 = Zlist::sort(z0, 1);
    Ylist *y = this;
    Zlist *zn, *zp = 0;
    for (Zlist *z = z0; z; zp = z, z = zn) {
        zn = z->next;

        if (zp && zp->Z.yu == z->Z.yu) {
            Zlist *zz = zp;
            for ( ; zz->next; zz = zz->next) {
                if (z->Z.zcmp(&zz->next->Z) <= 0)
                    break;
            }
            z->next = zz->next;
            zz->next = z;
            continue;
        }
        for ( ; y; y = y->next) {
            Ylist *yy;
            if (y->y_yu == z->Z.yu) {
                yy = y;
                if (yy->y_yl > z->Z.yl)
                    yy->y_yl = z->Z.yl;
            }
            else if (!y->next || y->next->y_yu < z->Z.yu) {
                yy = new Ylist(0);
                yy->next = y->next;
                y->next = yy;
                yy->y_yu = z->Z.yu;
                yy->y_yl = z->Z.yl;
            }
            else
                continue;
            yy->link_into_this_row(z);
            break;
        }
    }
}


// Static function.
// Remove common identical zoids from the two Ylists.
//
void
Ylist::remove_common(Ylist **pyl, Ylist **pyx)
{
    Ylist *yl = *pyl;
    Ylist *ylp = 0;
    Ylist *yx = *pyx;
    Ylist *yxp = 0;

    while (yl && yx) {
        while (yx && yx->y_yu > yl->y_yu) {
            yxp = yx;
            yx = yx->next;
        }
        if (!yx)
            break;
        while (yl && yl->y_yu > yx->y_yu) {
            ylp = yl;
            yl = yl->next;
        }
        if (!yl)
            break;
        if (yl->y_yu != yx->y_yu)
            continue;

        remove_common_zoids(yl, yx);

        if (!yl->y_zlist) {
            if (ylp)
                ylp->next = yl->next;
            else
                *pyl = yl->next;
            Ylist *yt = yl;
            yl = yl->next;
            delete yt;
        }
        else {
            ylp = yl;
            yl = yl->next;
        }
        if (!yx->y_zlist) {
            if (yxp)
                yxp->next = yx->next;
            else
                *pyx = yx->next;
            Ylist *yt = yx;
            yx = yx->next;
            delete yt;
        }
        else {
            yxp = yx;
            yx = yx->next;
        }
    }
}


// Static function.
// Remove identical zoids from the two zlists, which must have the same yu.
//
void
Ylist::remove_common_zoids(Ylist *y1, Ylist *y2)
{
    if (y1->y_yu != y2->y_yu)
        return;
    Zlist *z1 = y1->y_zlist, *z1p = 0;
    Zlist *z2 = y2->y_zlist, *z2p = 0;

    int icmp = 0;
    while (z1 && z2) {

        while (z1 && (icmp = z1->Z.zcmp(&z2->Z)) < 0) {
            z1p = z1;
            z1 = z1->next;
        }
        if (!z1)
            break;
        if (icmp) {
            while (z2 && (icmp = z2->Z.zcmp(&z1->Z)) < 0) {
                z2p = z2;
                z2 = z2->next;
            }
            if (!z2)
                break;
        }
        if (!icmp) {
            Zlist *z1tmp = z1;
            if (z1p) {
                z1p->next = z1->next;
                z1 = z1->next;
            }
            else {
                y1->y_zlist = z1->next;
                z1 = y1->y_zlist;
            }
            delete z1tmp;
            Zlist *z2tmp = z2;
            if (z2p) {
                z2p->next = z2->next;
                z2 = z2->next;
            }
            else {
                y2->y_zlist = z2->next;
                z2 = y2->y_zlist;
            }
            delete z2tmp;
        }
    }
}
// End of Ylist member functions

