
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

#include "cd.h"
#include "cd_types.h"
#include "geo_ylist.h"


//------------------------------------------------------------------------
// Zoid Functions
//------------------------------------------------------------------------

void
Zoid::print(FILE *fp) const
{
    if (!fp)
        fp = stdout;
    fprintf(fp, "yl=%d yu=%d ll=%d ul=%d lr=%d ur=%d\n",
        yl, yu, xll, xul, xlr, xur);
}


void
Zoid::show() const
{
    GEO()->ifDrawZoid(this);
}


// This shrinks a zoid by 2, ensuring that all 45s remain 45s.
//
// We can't just scale coordinates and expect 45s to remain exact. 
// One can see this by considering a square.  The square is 5x5, and
// the lower left corner is 1,2.  The upper right corner is therefor
// 6,7.  After simple scaling the coordinates are 0,1 -- 3,3.  The
// scaled box is no longer square, having width 3 and height 2.
//
// The algorithm below down scales a trapezoid while maintaining exact
// 45s.  It does so by special-casing exact 45s, and repairing them
// after scaling, by moving a point horizontally if necessary.  Given
// that we know that the left side, for example, needs fixing, which
// of xll, xul do we move?
//
// We assert that exactly one of the values will have been on an odd x
// coordinate, so its location is ambiguous after down scaling.  We
// choose this one to move.
//
// Assertion:
// If abs(dx) != dy after down-scaling, then exactly one endpoint will
// have the odd flag set (had an odd x-coordinate before scaling).
//
// Proof:
// Suppose that both end flags are the same (both odd or both even). 
// The difference is therefor even.  This requires that the height be
// even, and yu/yl be both either odd or even.  After down-scaling by
// 2, it is not possible for abs(w) to not equal h.
//
void
Zoid::shrink_by_2()
{
    int h = yu - yl;
    int w = xul - xll;
    int l45 = 0;
    if (w == h)
        l45 = 1;
    else if (w == -h)
        l45 = -1;
    w = xur - xlr;
    int r45 = 0;
    if (w == h)
        r45 = 1;
    else if (w == -h)
        r45 = -1;

    yu /= 2;
    yl /= 2;
    h = yu - yl;

    bool ll_odd = xll & 1;
    bool ul_odd = xul & 1;
    xll /= 2;
    xul /= 2;
    if (l45 == 1) {
        w = xul - xll;
        if (w != h) {
            if (ll_odd)
                xll = xul - h;
            else if (ul_odd)
                xul = xll + h;
        }
    }
    else if (l45 == -1) {
        w = xul - xll;
        if (w != -h) {
            if (ll_odd)
                xll = xul + h;
            else if (ul_odd)
                xul = xll - h;
        }
    }

    bool lr_odd = xlr & 1;
    bool ur_odd = xur & 1;
    xlr /= 2;
    xur /= 2;
    if (r45 == 1) {
        w = xur - xlr;
        if (w != h) {
            if (lr_odd)
                xlr = xur - h;
            else if (ur_odd)
                xur = xlr + h;
        }
    }
    else if (r45 == -1) {
        w = xur - xlr;
        if (w != -h) {
            if (lr_odd)
                xlr = xur + h;
            else if (ur_odd)
                xur = xlr - h;
        }
    }

    // Take care of the special case where the edge segments
    // intersect, This may happen, for example, if the top was
    // originally a triangle point on an odd x value.

    if (xul > xur) {
        int dx = xul - xur;
        if (dx == 2) {
            xul--;
            xll--;
            xur++;
            xlr++;
        }
        else if (dx == 1) {
            if (ul_odd) {
                xul--;
                xll--;
            }
            else {
                xur++;
                xlr++;
            }
        }
    }
    else if (xll > xlr) {
        int dx = xll - xlr;
        if (dx == 2) {
            xul--;
            xll--;
            xur++;
            xlr++;
        }
        else if (dx == 1) {
            if (ll_odd) {
                xul--;
                xll--;
            }
            else {
                xur++;
                xlr++;
            }
        }
    }
}


// Construct a polygon from the zoid.
//
bool
Zoid::mkpoly(Point **pts, int *num, bool vert) const
{
    *pts = 0;
    *num = 0;
    if (xll != xlr && xul != xur) {
        Point *p = new Point[5];
        if (vert) {
            p[0].set(yl, -xll);
            p[1].set(yu, -xul);
            p[2].set(yu, -xur);
            p[3].set(yl, -xlr);
        }
        else {
            p[0].set(xll, yl);
            p[1].set(xul, yu);
            p[2].set(xur, yu);
            p[3].set(xlr, yl);
        }
        p[4] = p[0];
        Poly po(5, p);
        if (po.valid()) {
            *pts = p;
            *num = 5;
            return (true);
        }
        delete [] p;
    }
    else {
        Point *p = new Point[4];
        int i = 0;
        if (vert) {
            p[i].set(yl, -xll);
            i++;
            p[i].set(yu, -xul);
            i++;
            if (xul != xur) {
                p[i].set(yu, -xur);
                i++;
            }
            if (xll != xlr) {
                p[i].set(yl, -xlr);
                i++;
            }
        }
        else {
            p[i].set(xll, yl);
            i++;
            p[i].set(xul, yu);
            i++;
            if (xul != xur) {
                p[i].set(xur, yu);
                i++;
            }
            if (xll != xlr) {
                p[i].set(xlr, yl);
                i++;
            }
        }
        p[i++] = p[0];
        if (i == 4) {
            Poly po(4, p);
            if (po.valid()) {
                *pts = p;
                *num = 4;
                return (true);
            }
        }
        delete [] p;
    }
    return (false);
}


// If possible, join ZB into this and return true.  Return false if
// not joined.
//
bool
Zoid::join_below(const Zoid &ZB)
{
    if (yl != ZB.yu || xll != ZB.xul || xlr != ZB.xur)
        return (false);

    if (is_rect() && ZB.is_rect()) {
        xll = ZB.xll;
        xlr = ZB.xlr;
        yl = ZB.yl;
        return (true);
    }

    if (!cGEO::check_colinear(ZB.xll, ZB.yl, xul, yu, xll, yl, 0))
        return (false);
    if (!cGEO::check_colinear(ZB.xlr, ZB.yl, xur, yu, xlr, yl, 0))
        return (false);

    xll = ZB.xll;
    xlr = ZB.xlr;
    yl = ZB.yl;
    return (true);
}


bool
Zoid::join_above(const Zoid &ZT)
{
    if (yu != ZT.yl || xul != ZT.xll || xur != ZT.xlr)
        return (false);

    if (is_rect() && ZT.is_rect()) {
        xul = ZT.xul;
        xur = ZT.xur;
        yu = ZT.yu;
        return (true);
    }

    if (!cGEO::check_colinear(xll, yl, ZT.xul, ZT.yu, xul, yu, 0))
        return (false);
    if (!cGEO::check_colinear(xlr, yl, ZT.xur, ZT.yu, xur, yu, 0))
        return (false);

    xul = ZT.xul;
    xur = ZT.xur;
    yu = ZT.yu;
    return (true);
}


// The zoid must represent a right triangle, which is clipped into
// three new zoids.  The Manhattan part is added to zbox, if w and h
// >= mindim, and the triangular parts call this function recursively
// if big enough.  This effectively Manhattanizes the triangle.
//
void
Zoid::rt_triang(Zlist **zbox, int mindim)
{
    if (xul == xur) {
        int h = (yu - yl)/2;
        int d = (xlr - xll)/2;
        if (h < mindim || d < mindim)
            return;
        if (xll == xul) {
            *zbox = new Zlist(xll, xll + d, yl, xll, xll + d, yl + h,
                *zbox);
            Zoid z1(xll + d, xlr, yl, xll + d, xll + d, yl + h);
            z1.rt_triang(zbox, mindim);
            Zoid z2(xll, xll + d, yl + h, xll, xll, yu);
            z2.rt_triang(zbox, mindim);
        }
        else if (xlr == xur) {
            *zbox = new Zlist(xlr - d, xlr, yl, xlr - d, xlr, yl + h,
                *zbox);
            Zoid z1(xll, xlr - d, yl, xlr - d, xlr - d, yl + h);
            z1.rt_triang(zbox, mindim);
            Zoid z2(xlr - d, xlr, yl + h, xlr, xlr, yu);
            z2.rt_triang(zbox, mindim);
        }
    }
    else if (xll == xlr) {
        int h = (yu - yl)/2;
        int d = (xur - xul)/2;
        if (h < mindim || d < mindim)
            return;
        if (xll == xul) {
            *zbox = new Zlist(xll, xll + d, yl + h, xll, xll + d, yu,
                *zbox);
            Zoid z1(xll + d, xll + d, yl + h, xll + d, xur, yu);
            z1.rt_triang(zbox, mindim);
            Zoid z2(xll, xll, yl, xll, xll + d, yl + h);
            z2.rt_triang(zbox, mindim);
        }
        else if (xlr == xur) {
            *zbox = new Zlist(xlr - d, xlr, yl + h, xlr - d, xlr, yu,
                *zbox);
            Zoid z1(xlr - d, xlr - d, yl + h, xul, xlr - d, yu);
            z1.rt_triang(zbox, mindim);
            Zoid z2(xlr, xlr, yl, xlr - d, xlr, yl + h);
            z2.rt_triang(zbox, mindim);
        }
    }
}


// The test_coverage and test_existance functions that follow are for
// use in DRC.  In these, we filter out result zoids with dimensions
// less than the minsz value passed.

// Return false if only partially covered by the Zlist.  Otherwise,
// return true and set the coverage flag to indicate whether totally
// covered (true) or totally uncovered (false).  Argument zlp points
// to the list of zoids left after the covered parts are clipped out,
// if partial coverage.
//
bool
Zoid::test_coverage(const Zlist *zl0, bool *covered, int minsz,
    Zlist **zlp) const
{
    *covered = false;
    if (zlp)
        *zlp = 0;
    if (!zl0)
        return (true);

    Zlist *head = new Zlist(this);

    for (const Zlist *zl = zl0; zl; zl = zl->next) {
        Zlist::zl_andnot(&head, &zl->Z);
        if (!head) {
            *covered = true;
            return (true);
        }
    }
    head = Zlist::filter_drc_slivers(head, minsz);
    if (!head) {
        *covered = true;
        return (true);
    }
    if (!head->next & (head->Z == *this)) {
        Zlist::destroy(head);
        return (true);
    }

    // something reasonably large is left
    if (zlp)
        *zlp = head;
    else
        Zlist::destroy(head);
    return (false);
}


// Return true and set the covered flag if completely covered or
// uncovered by objects on ld.  If partially covered, return false.
// Argument zlp points to the list of zoids left after the covered parts
// are clipped out, if partial coverage.
//
bool
Zoid::test_coverage(const CDs *sdesc, const CDl *ld, bool *covered, int minsz,
    Zlist **zlp) const
{
    *covered = false;
    if (zlp)
        *zlp = 0;
    if (!ld)
        return (true);
    Zlist *head = new Zlist;
    head->Z = *this;
    BBox tBB;
    computeBB(&tBB);
    sPF gen(sdesc, &tBB, ld, CDMAXCALLDEPTH);
    CDo *odesc;
    Zlist *zl;
    while ((odesc = gen.next(false, false)) != 0) {
        if (odesc->type() == CDBOX) {
            Zoid z(&odesc->oBB());
            delete odesc;
            Zlist::zl_andnot(&head, &z);
            if (!head) {
                *covered = true;
                return (true);
            }
        }
        else if (odesc->type() == CDWIRE) {
            Zlist *zl0 = ((const CDw*)odesc)->w_toZlist();
            delete odesc;
            for (zl = zl0; zl; zl = zl->next) {
                Zlist::zl_andnot(&head, &zl->Z);
                if (!head) {
                    Zlist::destroy(zl0);
                    *covered = true;
                    return (true);
                }
            }
            Zlist::destroy(zl0);
        }
        else if (odesc->type() == CDPOLYGON) {
            Zlist *zl0 = ((const CDpo*)odesc)->po_toZlist();
            delete odesc;
            for (zl = zl0; zl; zl = zl->next) {
                Zlist::zl_andnot(&head, &zl->Z);
                if (!head) {
                    Zlist::destroy(zl0);
                    *covered = true;
                    return (true);
                }
            }
            Zlist::destroy(zl0);
        }
    }
    head = Zlist::filter_drc_slivers(head, minsz);
    if (!head) {
        *covered = true;
        return (true);
    }
    if (!head->next & (head->Z == *this)) {
        Zlist::destroy(head);
        return (true);
    }
    if (zlp)
        *zlp = Zlist::repartition_ni(head);
    else
        Zlist::destroy(head);
    return (false);
}


// Return true if a region of ld exists inside ZB.
//
bool
Zoid::test_existence(const CDs *sdesc, const CDl *ld, int minsz) const
{
    BBox tBB;
    computeBB(&tBB);
    sPF gen(sdesc, &tBB, ld, CDMAXCALLDEPTH);
    CDo *odesc;
    while ((odesc = gen.next(false, false)) != 0) {
        if (odesc->type() == CDBOX) {
            Zlist *zn = clip_to(&odesc->oBB());
            for (Zlist *z = zn; z; z = z->next) {
                if (!z->Z.is_drc_sliver(minsz)) {
                    Zlist::destroy(zn);
                    delete odesc;
                    return (true);
                }
            }
            Zlist::destroy(zn);
        }
        else if (odesc->type() == CDWIRE) {
            Zlist *zl0 = ((const CDw*)odesc)->w_toZlist();
            for (Zlist *zl = zl0; zl; zl = zl->next) {
                Zlist *zn = clip_to(&zl->Z);
                for (Zlist *z = zn; z; z = z->next) {
                    if (!z->Z.is_drc_sliver(minsz)) {
                        Zlist::destroy(zn);
                        Zlist::destroy(zl0);
                        delete odesc;
                        return (true);
                    }
                }
                Zlist::destroy(zn);
            }
            Zlist::destroy(zl0);
        }
        else if (odesc->type() == CDPOLYGON) {
            Zlist *zl0 = ((const CDpo*)odesc)->po_toZlist();
            for (Zlist *zl = zl0; zl; zl = zl->next) {
                Zlist *zn = clip_to(&zl->Z);
                for (Zlist *z = zn; z; z = z->next) {
                    if (!z->Z.is_drc_sliver(minsz)) {
                        Zlist::destroy(zn);
                        Zlist::destroy(zl0);
                        delete odesc;
                        return (true);
                    }
                }
                Zlist::destroy(zn);
            }
            Zlist::destroy(zl0);
        }
        delete odesc;
    }
    return (false);
}

