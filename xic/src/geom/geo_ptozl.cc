
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

#include "geo.h"
#include "geo_poly.h"
#include "geo_ylist.h"
#include "geo_ptozl.h"
#include <string.h>


//------------------------------------------------------------------------
// Poly to Zlist functions
//------------------------------------------------------------------------

namespace {
    // Sort comparison for Point, sort descending in Y, ascending in X.
    //
    inline bool
    pt_cmp(const Point &p1, const Point &p2)
    {
        if (p1.y > p2.y)
            return (true);
        if (p1.y < p2.y)
            return (false);
        return (p1.x < p2.x);
    }
}


// Static function.
// Return a trapezoid list representing the triangle formed by the
// three points in pts, z0 is appended.
//
Zlist *
Poly::pts3_to_zlist(const Point *pts, Zlist *z0)
{
    Point p[3] = { pts[0], pts[1], pts[2] };
    std::sort(p, p+3, pt_cmp);

    if (p[0].y == p[1].y)
        z0 = new_zlist(p[2].x, p[2].x, p[2].y, p[0].x, p[1].x, p[0].y, z0);
    else if (p[1].y == p[2].y)
        z0 = new_zlist(p[1].x, p[2].x, p[2].y, p[0].x, p[0].x, p[0].y, z0);
    else {
        double sl = (p[0].x - p[2].x)/(double)(p[0].y - p[2].y);
        int x1 = mmRnd(p[0].x - sl*(p[0].y - p[1].y));
        int x2 = p[1].x;
        int xl = mmMin(x1, x2);
        int xr = mmMax(x1, x2);
        z0 = new_zlist(xl, xr, p[1].y, p[0].x, p[0].x, p[0].y, z0);
        z0 = new_zlist(p[2].x, p[2].x, p[2].y, xl, xr, p[1].y, z0);
    }
    return (z0);
}


// Static function.
// Return a trapezoid list representing the quadralateral formed by
// the four points in pts, z0 is appended.
//
Zlist *
Poly::pts4_to_zlist(const Point *pts, Zlist *z0)
{
    Point p[5] = { pts[0], pts[1], pts[2], pts[3], pts[0] };
    int num = 4;
    Point::removeDups(p, &num);
    if (num == 3)
        return (pts3_to_zlist(p, z0));
    if (num < 3)
        return (z0);

    // Check for and do the simple case of a horizontal trapezoid.
    Point *ptmp = p;
    for (int pass = 0; pass < 2; pass++) {
        if (ptmp[0].y == ptmp[1].y && ptmp[2].y == ptmp[3].y) {
            if ((ptmp[0].x < ptmp[1].x && ptmp[3].x < ptmp[2].x) ||
                    (ptmp[0].x > ptmp[1].x && ptmp[3].x > ptmp[2].x)) {
                // horizontal trapezoid
                if (ptmp[0].y < ptmp[2].y) {
                    return (new_zlist(
                        mmMin(ptmp[0].x, ptmp[1].x),
                        mmMax(ptmp[0].x, ptmp[1].x), ptmp[0].y,
                        mmMin(ptmp[2].x, ptmp[3].x),
                        mmMax(ptmp[2].x, ptmp[3].x), ptmp[2].y, z0));
                }
                if (ptmp[0].y > ptmp[2].y) {
                    return (new_zlist(
                        mmMin(ptmp[2].x, ptmp[3].x),
                        mmMax(ptmp[2].x, ptmp[3].x), ptmp[2].y,
                        mmMin(ptmp[0].x, ptmp[1].x),
                        mmMax(ptmp[0].x, ptmp[1].x), ptmp[0].y, z0));
                }
                return (0);
            }
        }
        ptmp++;
    }

    if (cGEO::lines_cross(p[0], p[1], p[2], p[3], 0) ||
            cGEO::lines_cross(p[1], p[2], p[3], p[0], 0)) {

        // The polygon is not simple, use the poly_splitter since the
        // code below fails in this case.
        // WARNING: the cross test must detect the case where the lines
        // don't quite touch in floating-point coords, but would
        // touch in integer space.  This indicates the presence of
        // a needle, which will also cause failure below.
        
        Poly po(5, p);
        poly_splitter ps;
        Zlist *zl = ps.split(po);
        if (zl) {
            Zlist *zn = zl;
            while (zn->next)
                zn = zn->next;
            zn->next = z0;
            z0 = zl;
        }
        return (z0);
    }

    std::sort(p, p+4, pt_cmp);
    if (p[0].y == p[3].y)
        return (z0);

    // The 4-sided figure is not necessarily convex, meaning that the
    // boundary is not uniquely determined by the sorted points list.
    // Determine if p[0] is connected to p[3] and p[1].
    //
    bool zero_three = false;    // true when 0,3 adjacent
    bool zero_one = false;      // true when 0,1 adjacent
    for (int i = 0; i < 4; i++) {
        if (pts[i] == p[0]) {
            int ii = (i == 3 ? 0 : i+1);
            if (pts[ii] == p[1]) {
                zero_one = true;
                ii = (ii == 3 ? 0 : ii+1);
                if (pts[ii] == p[2])
                    zero_three = true;  // 0-1-2-3
                // else 0-1-3-2
            }
            else if (pts[ii] == p[3]) {
                zero_three = true;
                ii = (ii == 3 ? 0 : ii+1);
                if (pts[ii] == p[2])
                    zero_one = true;  // 0-3-2-1
                // else 0-3-1-2
            }
            else {
                ii = (ii == 3 ? 0 : ii+1);
                if (pts[ii] == p[3])
                    zero_one = true;  // 0-2-3-1
                else
                    zero_three = true;  // 0-2-1-3
            }
            break;
        }
    }

    if (p[0].y == p[1].y) {
        int xl, xr;
        if (zero_one) {
            if (zero_three) {
                double sl = (p[0].x - p[3].x)/(double)(p[0].y - p[3].y);
                xl = mmRnd(p[0].x - sl*(p[0].y - p[2].y));
                xr = p[2].x;
            }
            else {
                double sl = (p[1].x - p[3].x)/(double)(p[1].y - p[3].y);
                xl = p[2].x;
                xr = mmRnd(p[1].x - sl*(p[1].y - p[2].y));
            }
            z0 = new_zlist(xl, xr, p[2].y, p[0].x, p[1].x, p[0].y, z0);
            z0 = new_zlist(p[3].x, p[3].x, p[3].y, xl, xr, p[2].y, z0);
        }
        else {
            double sl = (p[0].x - p[3].x)/(double)(p[0].y - p[3].y);
            xl = mmRnd(p[0].x - sl*(p[0].y - p[2].y));
            sl = (p[1].x - p[3].x)/(double)(p[1].y - p[3].y);
            xr = mmRnd(p[1].x - sl*(p[1].y - p[2].y));
            z0 = new_zlist(xl, p[2].x, p[2].y, p[0].x, p[0].x, p[0].y, z0);
            z0 = new_zlist(p[2].x, xr, p[2].y, p[1].x, p[1].x, p[1].y, z0);
            z0 = new_zlist(p[3].x, p[3].x, p[3].y, xl, xr, p[2].y, z0);
        }
        return (z0);
    }

    if (p[2].y == p[3].y) {
        int xl, xr;
        if (zero_one) {
            if (zero_three) {
                double sl = (p[0].x - p[3].x)/(double)(p[0].y - p[3].y);
                xl = p[1].x;
                xr = mmRnd(p[0].x - sl*(p[0].y - p[1].y));
            }
            else {
                double sl = (p[0].x - p[2].x)/(double)(p[0].y - p[2].y);
                xl = mmRnd(p[0].x - sl*(p[0].y - p[1].y));
                xr = p[1].x;
            }
            z0 = new_zlist(xl, xr, p[1].y, p[0].x, p[0].x, p[0].y, z0);
            z0 = new_zlist(p[2].x, p[3].x, p[3].y, xl, xr, p[1].y, z0);
        }
        else {
            double sl = (p[0].x - p[2].x)/(double)(p[0].y - p[2].y);
            xl = mmRnd(p[0].x - sl*(p[0].y - p[1].y));
            sl = (p[0].x - p[3].x)/(double)(p[0].y - p[3].y);
            xr = mmRnd(p[0].x - sl*(p[0].y - p[1].y));
            z0 = new_zlist(p[0].x, p[0].x, p[0].y, xl, xr, p[1].y, z0);
            z0 = new_zlist(p[2].x, p[2].x, p[2].y, xl, p[1].x, p[1].y, z0);
            z0 = new_zlist(p[3].x, p[3].x, p[3].y, p[1].x, xr, p[1].y, z0);
        }
        return (z0);
    }

    int x1l, x1r, x2l, x2r;
    if (zero_one) {
        if (zero_three) {
            double sl = (p[0].x - p[3].x)/(double)(p[0].y - p[3].y);
            int x1 = mmRnd(p[0].x - sl*(p[0].y - p[1].y));
            int x2 = p[1].x;
            x1l = mmMin(x1, x2);
            x1r = mmMax(x1, x2);
            if (p[1].y - p[2].y <= 1) {
                x2 = p[2].x;
                x2l = mmMin(x1, x2);
                x2r = mmMax(x1, x2);
                z0 = new_zlist(x1l, x1r, p[1].y, p[0].x, p[0].x, p[0].y, z0);
                z0 = new_zlist(p[3].x, p[3].x, p[3].y, x2l, x2r, p[1].y, z0);
                return (z0);
            }
            x1 = mmRnd(p[0].x - sl*(p[0].y - p[2].y));
            x2 = p[2].x;
            x2l = mmMin(x1, x2);
            x2r = mmMax(x1, x2);
        }
        else {
            if (p[1].y - p[2].y <= 1) {
                int x1 = p[1].x;
                int x2 = p[2].x;
                int xl = mmMin(x1, x2);
                int xr = mmMax(x1, x2);
                z0 = new_zlist(xl, xr, p[1].y, p[0].x, p[0].x, p[0].y, z0);
                z0 = new_zlist(p[3].x, p[3].x, p[3].y, xl, xr, p[1].y, z0);
                return (z0);
            }
            double sl = (p[0].x - p[2].x)/(double)(p[0].y - p[2].y);
            int x1 = mmRnd(p[0].x - sl*(p[0].y - p[1].y));
            int x2 = p[1].x;
            x1l = mmMin(x1, x2);
            x1r = mmMax(x1, x2);
            sl = (p[1].x - p[3].x)/(double)(p[1].y - p[3].y);
            x1 = mmRnd(p[1].x - sl*(p[1].y - p[2].y));
            x2 = p[2].x;
            x2l = mmMin(x1, x2);
            x2r = mmMax(x1, x2);
        }
        z0 = new_zlist(x1l, x1r, p[1].y, p[0].x, p[0].x, p[0].y, z0);
        z0 = new_zlist(x2l, x2r, p[2].y, x1l, x1r, p[1].y, z0);
        z0 = new_zlist(p[3].x, p[3].x, p[3].y, x2l, x2r, p[2].y, z0);
        return (z0);
    }

    if (p[1].y - p[2].y <= 1) {
        int ymid = p[1].y;
        double sl = (p[0].x - p[3].x)/(double)(p[0].y - p[3].y);
        int x1 = mmRnd(p[0].x - sl*(p[0].y - ymid));
        int x2 = p[2].x;
        x1l = mmMin(x1, x2);
        x1r = mmMax(x1, x2);
        x2 = p[1].x;
        x2l = mmMin(x1, x2);
        x2r = mmMax(x1, x2);
        z0 = new_zlist(x1l, x1r, ymid, p[0].x, p[0].x, p[0].y, z0);
        z0 = new_zlist(p[3].x, p[3].x, p[3].y, x2l, x2r, ymid, z0);
    }
    else {
        double sl1 = (p[0].x - p[3].x)/(double)(p[0].y - p[3].y);
        double sl2 = (p[0].x - p[2].x)/(double)(p[0].y - p[2].y);
        int x1 = mmRnd(p[0].x - sl1*(p[0].y - p[1].y));
        int x2 = mmRnd(p[0].x - sl2*(p[0].y - p[1].y));
        if (x1 < p[1].x && p[1].x < x2) {
            z0 = new_zlist(x1, x2, p[1].y, p[0].x, p[0].x, p[0].y, z0);
            z0 = new_zlist(p[3].x, p[3].x, p[3].y, x1, p[1].x, p[1].y, z0);
            z0 = new_zlist(p[2].x, p[2].x, p[2].y, p[1].x, x2, p[1].y, z0);
        }
        else if (x2 < p[1].x && p[1].x < x1) {
            z0 = new_zlist(x2, x1, p[1].y, p[0].x, p[0].x, p[0].y, z0);
            z0 = new_zlist(p[3].x, p[3].x, p[3].y, p[1].x, x1, p[1].y, z0);
            z0 = new_zlist(p[2].x, p[2].x, p[2].y, x2, p[1].x, p[1].y, z0);
        }
        else {
            sl2 = (p[1].x - p[3].x)/(double)(p[1].y - p[3].y);
            x1 = mmRnd(p[0].x - sl1*(p[0].y - p[2].y));
            x2 = mmRnd(p[1].x - sl2*(p[1].y - p[2].y));
            if (x1 < p[2].x && p[2].x < x2) {
                z0 = new_zlist(x1, p[2].x, p[2].y, p[0].x, p[0].x, p[0].y, z0);
                z0 = new_zlist(p[2].x, x2, p[2].y, p[1].x, p[1].x, p[1].y, z0);
                z0 = new_zlist(p[3].x, p[3].x, p[3].y, x1, x2, p[2].y, z0);
            }
            else if (x2 < p[2].x && p[2].x < x1) {
                z0 = new_zlist(p[2].x, x1, p[2].y, p[0].x, p[0].x, p[0].y, z0);
                z0 = new_zlist(x2, p[2].x, p[2].y, p[1].x, p[1].x, p[1].y, z0);
                z0 = new_zlist(p[3].x, p[3].x, p[3].y, x2, x1, p[2].y, z0);
            }
        }
    }
    return (z0);
}


// Main function to split a polygon into a list of trapezoids.
//
Zlist *
Poly::toZlist() const
{
    if (numpts < 4)
        // poly is degenerate
        return (0);

    if (numpts == 4) {

        // Do the triangle case here.
        Point p[3] = { points[0], points[1], points[2] };
        std::sort(p, p+3, pt_cmp);

        Zlist *z0;
        if (p[0].y == p[1].y)
            z0 = new_zlist(p[2].x, p[2].x, p[2].y, p[0].x, p[1].x, p[0].y, 0);
        else if (p[1].y == p[2].y)
            z0 = new_zlist(p[1].x, p[2].x, p[2].y, p[0].x, p[0].x, p[0].y, 0);
        else {
            double sl = (p[0].x - p[2].x)/(double)(p[0].y - p[2].y);
            int x1 = mmRnd(p[0].x - sl*(p[0].y - p[1].y));
            int x2 = p[1].x;
            int xl = mmMin(x1, x2);
            int xr = mmMax(x1, x2);
            z0 = new_zlist(xl, xr, p[1].y, p[0].x, p[0].x, p[0].y, 0);
            z0 = new_zlist(p[2].x, p[2].x, p[2].y, xl, xr, p[1].y, z0);
        }
        return (z0);
    }
    else if (numpts == 5) {

        // Check for and do the simple case of a horizontal trapezoid.
        Point *pts = points;
        for (int pass = 0; pass < 2; pass++) {
            if (pts[0].y == pts[1].y && pts[2].y == pts[3].y) {
                if ((pts[0].x < pts[1].x && pts[3].x < pts[2].x) ||
                        (pts[0].x > pts[1].x && pts[3].x > pts[2].x)) {
                    // horizontal trapezoid
                    if (pts[0].y < pts[2].y) {
                        return (new_zlist(
                            mmMin(pts[0].x, pts[1].x),
                            mmMax(pts[0].x, pts[1].x), pts[0].y,
                            mmMin(pts[2].x, pts[3].x),
                            mmMax(pts[2].x, pts[3].x), pts[2].y, 0));
                    }
                    if (pts[0].y > pts[2].y) {
                        return (new_zlist(
                            mmMin(pts[2].x, pts[3].x),
                            mmMax(pts[2].x, pts[3].x), pts[2].y,
                            mmMin(pts[0].x, pts[1].x),
                            mmMax(pts[0].x, pts[1].x), pts[0].y, 0));
                    }
                    return (0);
                }
            }
            pts++;
        }
    }
    poly_splitter ps;
    return (ps.split(*this));
}


// Rotate -90 degrees before decomposing into trapezoids.
//
Zlist *
Poly::toZlistR() const
{
    if (numpts < 4)
        // poly is degenerate
        return (0);
    Point *pts = new Point[numpts];
    for (int i = 0; i < numpts; i++)
        pts[i].set(-points[i].y, points[i].x);
    Poly po(numpts, pts);
    Zlist *zl = po.toZlist();
    delete [] pts;
    return (zl);
}
// End of Poly functions.


//
// Poly-Splitter class, convert a polygon to a trapezoid list.
//

// The exported function to split a polygon into a trapezoid list.
//
Zlist *
poly_splitter::split(const Poly &po)
{
    int n = po.numpts - 1;

    ps_bands_used = 0;
    ps_segs_used = 0;
   
    // Create and sort an array of vertex y values.  Also check
    // whether the poly is Manhattan.
    //
    bool is_manh = true;
    Point *pts = po.points;
    int *ary = new int[n];
    int ylast = 0;
    int cnt = 0;
    for (int i = 0; i < n; i++) {
        if (is_manh && pts[0].x != pts[1].x && pts[0].y != pts[1].y)
            is_manh = false;
        if (!i || pts->y != ylast) {
            ylast = pts->y;
            ary[cnt++] = ylast;
        }
        pts++;
    }
    std::sort(ary, ary + cnt, icmp);

    // Create and link into an ordered list a band struct for each unique
    // y value.
    //
    ps_band *be = 0;
    ylast = ary[0];
    int num_bands = 0;
    for (int i = 1; i < cnt; i++) {
        if (ary[i] == ylast)
            continue;
        ps_band *b = new_band();
        num_bands++;
        b->b_top = ylast;
        ylast = ary[i];
        b->b_bot = ylast;
        if (!be)
            be = ps_top_band = b;
        else {
            be->next = b;
            be = b;
        }
    }
    if (!be) {
        // Strange case, but can get here by calling to_poly on a
        // zero-width horizontal wire.
        delete [] ary;
        return (0);
    }

    // Add a dummy terminator.
    //
    be->next = new_band();
    num_bands++;
    be->next->b_top = be->b_bot;
    be->next->b_bot = be->b_bot;

    delete [] ary;

    // Array for binary search, for insert.
    //
    ps_bands = new ps_band*[num_bands];
    cnt = 0;
    for (ps_band *b = ps_top_band; b; b = b->next)
        ps_bands[cnt++] = b;

#ifdef PSPLIT_DEBUG
    if (cnt != num_bands)
        printf("poly_splitter foo 1\n");
    for (int i = 0; i < num_bands - 1; i++) {
        if (ps_bands[i]->b_top <= ps_bands[i]->b_bot)
            printf("poly_splitter foo 2\n");
        if (i && ps_bands[i]->b_top != ps_bands[i-1]->b_bot)
            printf("poly_splitter foo 3\n");
    }
#endif

    // Loop through the edges.  For each overlapping band, clip and add
    // a segment element.
    //
    ps_num_bands = num_bands;
    pts = po.points;
    for (int i = 0; i < n; i++) {
        if (pts[0].y != pts[1].y)
            insert(pts[0], pts[1]);
        pts++;
    }

    // Done with the bands array, zero out the terminator.
    //
    ps_bands[num_bands - 2]->next = 0;
    delete [] ps_bands;
    ps_bands = 0;

    // Sort the seg lists in each band.  At the same time, look for segs
    // that cross.  This can only happen if the poly is non-Manhattan.
    // If an intersection is found, split the band at that y value.
    //
    for (ps_band *p = ps_top_band; p; p = p->next) {
        p->sort_segs();
        if (!is_manh)
            p->check_cross(this);
    }

    // Stitch together the segments into trapezoids.
    //
    Ylist *y0 = 0;
    for (ps_band *p = ps_top_band; p; p = p->next) {
        Ylist *y = new Ylist(p->to_zlist());
        Ylist::scl_merge(y->y_zlist);
        if (!y0)
            y0 = y;
        else
            Ylist::merge_end_row(y0, y);
    }
    return (Ylist::to_zlist(y0));
}


// Split up the edge whose endpoints are given into segments which sre
// added to the appropriate bands.  The p2 point is after p1 in vertex
// order.  If the segment is ascending, it is added to the "up" list,
// otherwise it is added to the "down" list.  We know that the edge is
// not horizontal.
//
void
poly_splitter::insert(const Point &p1, const Point &p2)
{
    ps_band *b1 = find(p1.y);
    ps_band *b2 = find(p2.y);

#ifdef PSPLIT_DEBUG
    if (!b1) {
        printf("poly_splitter search fail b1 %d\n", p1.y);
        for (int i = 0; i < ps_num_bands; i++) {
            if (ps_bands[i]->b_top == p1.y) {
                b1 = ps_bands[i];
                printf("found b1\n");
                break;
            }
        }
    }
    if (!b2) {
        printf("poly_splitter search fail b2 %d\n", p2.y);
        for (int i = 0; i < ps_num_bands; i++) {
            if (ps_bands[i]->b_top == p2.y) {
                b2 = ps_bands[i];
                printf("found b2\n");
                break;
            }
        }
    }
#endif

    if (!b1 || !b2) {
        // This should never happen.
        fprintf(stderr, "poly_splitter error: vertex binary search failed.\n");
        return;
    }

    if (p2.y > p1.y) {
        int dx = p2.x - p1.x;
        double slp = dx ? dx/(double)(p2.y - p1.y) : 0.0;
        int xu = p2.x;
        for (ps_band *b = b2; b != b1; b = b->next) {
            int dy = b->b_bot - p1.y;
            int xl = dy && dx ? mmRnd(p1.x + dy*slp) : p1.x;

            ps_seg *sg = new_seg();
            sg->xl = xl;
            sg->xu = xu;
            sg->next = b->b_segs_up;
            b->b_segs_up = sg;
            xu = xl;
        }
    }
    else {
        int dx = p1.x - p2.x;
        double slp = dx ? dx/(double)(p1.y - p2.y) : 0.0;
        int xu = p1.x;
        for (ps_band *b = b1; b != b2; b = b->next) {
            int dy = b->b_bot - p2.y;
            int xl = dy && dx ? mmRnd(p2.x + dy*slp) : p2.x;

            ps_seg *sg = new_seg();
            sg->xl = xl;
            sg->xu = xu;
            sg->next = b->b_segs_dn;
            b->b_segs_dn = sg;
            xu = xl;
        }
    }
}


// Use a binary search to find the band struct whose top value matches
// y.  Return 0 if no match, which shouldn't happen.
//
poly_splitter::ps_band *
poly_splitter::find(int y)
{
    int hi = ps_num_bands;
    int mid = hi/2;
    int low = 0;

    for (;;) {
        if (ps_bands[mid]->b_top < y) {
            hi = mid;
            if (hi == low)
                return (0);
            mid = low + (hi - low)/2;
        }
        else if (ps_bands[mid]->b_top > y) {
            low = mid + 1;
            if (hi == low)
                return (0);
            mid = low + (hi - low)/2;
        }
        else
            return (ps_bands[mid]);
    }
    // not reached
}


// Sort the segments in the two lists in ascending order of the x
// value of the midpoint.  Also make sure that the lists are of equal
// length.
//
void
poly_splitter::ps_band::sort_segs()
{
    int u_cnt = 0;
    for (ps_seg *s = b_segs_up; s; s = s->next)
        u_cnt++;
    int d_cnt = 0;
    for (ps_seg *s = b_segs_dn; s; s = s->next)
        d_cnt++;
    if (u_cnt != d_cnt) {
        // This should never happen.  If it does, ignore this band.
        b_segs_up = 0;
        b_segs_dn = 0;
        fprintf(stderr,
            "poly_splitter error: up/down segment count mismatch.\n");
        return;
    }
    if (d_cnt > 1) {
        ps_seg **ary = new ps_seg*[d_cnt];

        int cnt = 0;
        for (ps_seg *s = b_segs_up; s; s = s->next)
            ary[cnt++] = s;
        std::sort(ary, ary + cnt, scmp);
        cnt--;
        for (int i = 0; i < cnt; i++)
            ary[i]->next = ary[i+1];
        ary[cnt]->next = 0;
        b_segs_up = ary[0];

        cnt = 0;
        for (ps_seg *s = b_segs_dn; s; s = s->next)
            ary[cnt++] = s;
        std::sort(ary, ary + cnt, scmp);
        cnt--;
        for (int i = 0; i < cnt; i++)
            ary[i]->next = ary[i+1];
        ary[cnt]->next = 0;
        b_segs_dn = ary[0];

        delete [] ary;
    }
}


// Iteratively look for crossing segments, and if found divide the
// band at the intersection y value and add a new following band.  On
// return, 'this' has no crossings, and may be followed by
// newly-created bands that should also be tested.
//
void
poly_splitter::ps_band::check_cross(poly_splitter *ps)
{
    for (;;) {
        int yval;
        if (find_cross(&yval)) {
            ps_band *b = ps->new_band();

            ps_seg *es = 0;
            for (ps_seg *s = b_segs_up; s; s = s->next) {

                double slp = (s->xu - s->xl)/(double)(b_top - b_bot);
                int x = mmRnd(s->xl + (yval - b_bot)*slp);
                ps_seg *ns = ps->new_seg();
                ns->xu = x;
                ns->xl = s->xl;
                s->xl = x;

                if (!es)
                    es = b->b_segs_up = ns;
                else {
                    es->next = ns;
                    es = ns;
                }
            }

            es = 0;
            for (ps_seg *s = b_segs_dn; s; s = s->next) {

                double slp = (s->xu - s->xl)/(double)(b_top - b_bot);
                int x = mmRnd(s->xl + (yval - b_bot)*slp);
                ps_seg *ns = ps->new_seg();
                ns->xu = x;
                ns->xl = s->xl;
                s->xl = x;

                if (!es)
                    es = b->b_segs_dn = ns;
                else {
                    es->next = ns;
                    es = ns;
                }
            }

            b->b_top = yval;
            b->b_bot = b_bot;
            b_bot = yval;

            b->next = next;
            next = b;

            // Sort again.
            sort_segs();
            b->sort_segs();

            continue;
        }
        break;
    }
}


// Search for segments that cross.  Return true and set yval for the
// first crossing found.  Only non-Manhattan polys can have crossing
// segments.
//
bool
poly_splitter::ps_band::find_cross(int *yval)
{
    for (ps_seg *s = b_segs_up; s; s = s->next) {
        for (ps_seg *s1 = s->next; s1; s1 = s1->next) {
            int dl = s->xl - s1->xl;
            int du = s->xu - s1->xu;
            if ((dl < 0 && du > 0) || (dl > 0 && du < 0)) {
                double d = dl - du;
                int y = mmRnd(b_bot + (b_top - b_bot)*(dl/d));
                if (y < b_bot && y < b_top) {
                    *yval = y;
                    return (true);
                }
            }
        }
    }
    for (ps_seg *s = b_segs_dn; s; s = s->next) {
        for (ps_seg *s1 = s->next; s1; s1 = s1->next) {
            int dl = s->xl - s1->xl;
            int du = s->xu - s1->xu;
            if ((dl < 0 && du > 0) || (dl > 0 && du < 0)) {
                double d = dl - du;
                int y = mmRnd(b_bot + (b_top - b_bot)*(dl/d));
                if (y < b_bot && y < b_top) {
                    *yval = y;
                    return (true);
                }
            }
        }
    }
    for (ps_seg *s = b_segs_dn; s; s = s->next) {
        for (ps_seg *s1 = b_segs_up; s1; s1 = s1->next) {
            int dl = s->xl - s1->xl;
            int du = s->xu - s1->xu;
            if ((dl < 0 && du > 0) || (dl > 0 && du < 0)) {
                double d = dl - du;
                int y = mmRnd(b_bot + (b_top - b_bot)*(dl/d));
                if (y < b_bot && y < b_top) {
                    *yval = y;
                    return (true);
                }
            }
        }
    }
    return (false);
}


// Create the trapezoids from the segment lists.
//
Zlist *
poly_splitter::ps_band::to_zlist()
{
    Zlist *z0 = 0;
    ps_seg *su = b_segs_up;
    ps_seg *sd = b_segs_dn;
    for ( ; su; su = su->next, sd = sd->next) {
        if (su->xl == sd->xl && su->xu == sd->xu)
            continue;
        int xll, xlr;
        if (su->xl < sd->xl) {
            xll = su->xl;
            xlr = sd->xl;
        }
        else {
            xll = sd->xl;
            xlr = su->xl;
        }
        int xul, xur;
        if (su->xu < sd->xu) {
            xul = su->xu;
            xur = sd->xu;
        }
        else {
            xul = sd->xu;
            xur = su->xu;
        }
        z0 = new_zlist(xll, xlr, b_bot, xul, xur, b_top, z0);
    }
    return (z0);
}


// Allocation functions.  Structs are block-allocated.  The first block
// is static, more are allocated as needed.

// Allocator for ps_band structs.
//
poly_splitter::ps_band *
poly_splitter::new_band()
{
    if (ps_bands_used == PS_BLKSZ) {
        ps_band_blk *bn = new ps_band_blk;
        bn->next = ps_static_band_blk.next;
        ps_static_band_blk.next = bn;
        ps_band_base = bn->ary;
        ps_bands_used = 0;
    }
    ps_band *b = ps_band_base + ps_bands_used++;
    memset(b, 0, sizeof(ps_band));
    return (b);
}


// Allocator for ps_seg structs.
//
poly_splitter::ps_seg *
poly_splitter::new_seg()
{
    if (ps_segs_used == PS_BLKSZ) {
        ps_seg_blk *bn = new ps_seg_blk;
        bn->next = ps_static_seg_blk.next;
        ps_static_seg_blk.next = bn;
        ps_seg_base = bn->ary;
        ps_segs_used = 0;
    }
    ps_seg *s = ps_seg_base + ps_segs_used++;
    memset(s, 0, sizeof(ps_seg));
    return (s);
}
// End of poly_splitter functions.

