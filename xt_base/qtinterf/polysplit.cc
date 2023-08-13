
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "ginterf/graphics.h"
#include "polysplit.h"
#include <algorithm>
#include <math.h>
#include <limits.h>
//#include <stdio.h>


//
// The following code performs trapezoid decomp for ploygons.
//

using namespace qtinterf;

// General purpose double->int rounding function.  Using fabs timed
// faster than two comparisons, probably not true when fabs is not
// inlined (gcc inlines fabs).
//
// 0.4999999999999999 is two ticks below 0.5
//
// In order for a+cdrnd(b) == cdrnd(a+b) where a is an arbitrary int,
// -x.5 should round to -x, thus the asymmetry below.  The two-tick
// difference overcomes the internal rounding.
//
inline int
cdrnd(double a)
{
    static const double zmax = (double)INT_MAX;
    if (fabs(a) >= zmax)
        return (a > 0.0 ? INT_MAX : -INT_MAX);
    return (a >= 0.0 ? (int)(a + 0.5) : (int)(a - 0.4999999999999999));
}


namespace {
    // Comparison function for edge_t, sort descending in ytop.
    //
    inline bool
    etype_cmp(const etype_t *e1, const etype_t *e2)
    {
        return (e1->pt.y > e2->pt.y);
    }


    // Comparison function for isect_t elements.
    //
    inline bool
    isect_cmp(const isect_t *w1, const isect_t *w2)
    {
        if (w1->xtop < w2->xtop)
            return (true);
        if (w1->xtop > w2->xtop)
            return (false);
        return (w1->xbot < w2->xbot);
    }
}


// Constructor, initialize the edge_t list and other variables.  This
// assumes that the path is closed.
//
polysplit::polysplit(GRmultiPt::pt_i *points, int numpts, int szf)
{
    if (numpts > PS_STACK_VERTS) {
        ps_etype_array = new etype_t*[numpts + numpts/2];
        ps_isect_array = new isect_t*[numpts];
        ps_etype_reserv = new etype_t[numpts + numpts/2];
        ps_isect_reserv = new isect_t[numpts];
        ps_use_heap = true;
    }
    else {
        ps_etype_array = ps_ea;
        ps_isect_array = ps_ia;
        ps_etype_reserv = ps_epool;
        ps_isect_reserv = ps_ipool;
        ps_use_heap = false;
    }
    ps_ecnt = 0;
    min_size = szf;
    zelcnt = 0;

    GRmultiPt::pt_i *p = points;
    GRmultiPt::pt_i *pp = points + numpts - 2;
    for (int i = 0; i < numpts - 1; i++) {

        // We need an entry for each vertex.  Save the edges
        // associated with this vertex where this vertex has the
        // largest Y value, or save a place-holder to mark the vertex.

        GRmultiPt::pt_i *pn = p + 1;      // next vertex
        int ee = ps_ecnt;
        if (p->y > pp->y) {
            // ascending from previous
            ps_etype_array[ps_ecnt] = new_etype(p, pp, 1);
            ps_ecnt++;
        }
        if (pn->y < p->y) {
            // descending to next
            ps_etype_array[ps_ecnt] = new_etype(p, pn, -1);
            ps_ecnt++;
        }
        if (ee == ps_ecnt) {
            // add place holder
            ps_etype_array[ps_ecnt] = new_etype(p, p, 0);
            ps_ecnt++;
        }
        pp = p;
        p = pn;
    }

    // sort descending in etype_t::pt::y
    std::sort(ps_etype_array, ps_etype_array + ps_ecnt, etype_cmp);

    ps_ytop = ps_etype_array[0]->pt.y;
    ps_ybot = ps_ytop;

    ps_z0 = 0;
    ps_zrow = 0;
    ps_zrow_end = 0;
}


polysplit::~polysplit()
{
    if (ps_use_heap) {
        delete [] ps_etype_array;
        delete [] ps_isect_array;
        delete [] ps_etype_reserv;
        delete [] ps_isect_reserv;
    }
    while (initblk.next) {
        zblk_t *b = initblk.next;
        initblk.next = initblk.next->next;
        delete b;
    }
}


// Allocator for zoid_t elements.  These should *never* be deleted in
// user code.  The pool will be freed when this is destroyed.
//
zoid_t *
polysplit::new_zoid(int xll, int xlr, int yl, int xul, int xur, int yu)
{
    if (zelcnt < PS_ZBLKSZ) {
        zoid_t *z = &initblk.zoids[zelcnt];
        z->next = 0;
        z->xll = xll;
        z->xlr = xlr;
        z->yl = yl;
        z->xul = xul;
        z->xur = xur;
        z->yu = yu;
        zelcnt++;
        return (z);
    }
    if (zelcnt == (PS_ZBLKSZ << 1))
        zelcnt = PS_ZBLKSZ;
    if (zelcnt == PS_ZBLKSZ)
        initblk.next = new zblk_t(initblk.next);

    zoid_t *z = &initblk.next->zoids[zelcnt - PS_ZBLKSZ];
    z->next = 0;
    z->xll = xll;
    z->xlr = xlr;
    z->yl = yl;
    z->xul = xul;
    z->xur = xur;
    z->yu = yu;
    zelcnt++;
    return (z);
}


// Export to compute and retrieve the trapezoid list.
//
zoid_t *
polysplit::extract_zlist()
{
    for (int i = 1; i < ps_ecnt; i++) {
        ps_ybot = ps_etype_array[i]->pt.y;
        if (ps_ybot == ps_ytop)
            continue;

        // ignore subdimensionals, fill gaps
        if (ps_ytop - ps_ybot <= min_size) {
            for (zoid_t *zl = ps_z0; zl; zl = zl->next) {
                if (zl->yl == ps_ytop)
                    zl->yl = ps_ybot;
                else
                    break;
            }
            ps_ytop = ps_ybot;
            continue;
        }

        // i-1 is the last element of the previous row
        extract_row_intersect(i-1);

        if (ps_icnt) {
            // Create the zoids for this row.  ps_icnt should always
            // be even and positive.
            create_row_zlist();
            add_row();
        }
        ps_ytop = ps_ybot;
    }
    return (ps_z0);
}


// Create a list of intersections.  The ps_etype_array elements for
// index larger than last_i have top <= ybot, so can be ignored.  Look
// for edges in ps_etype_array with index last_i or smaller with
// bottom less than ps_ytop, and save the intersection values in
// ps_isect_array.  This sets ps_icnt to the number of intersections.
//
void
polysplit::extract_row_intersect(int last_i)
{
    ps_icnt = 0;
    for (int j = last_i; j >= 0; j--) {
        if (ps_ytop <= ps_etype_array[j]->pb.y)
            continue;
        if (ps_etype_array[j]->asc) {
            GRmultiPt::pt_i *p = &ps_etype_array[j]->pt;
            GRmultiPt::pt_i *q = &ps_etype_array[j]->pb;
            double sl = ((double)(q->x - p->x))/(q->y - p->y);
            ps_isect_array[ps_icnt] =
                new_isect(cdrnd((ps_ybot - q->y)*sl) + q->x,
                    cdrnd((ps_ytop - q->y)*sl) + q->x,
                    sl, ps_etype_array[j]->asc);
            ps_icnt++;
        }
    }
}


// Create the zoids in this row, ordered left to right, merging if
// possible.  The zoids are constructed from the ps_isect_array points,
// with the list head left in ps_zrow and tail in ps_zrow_end.
//
void
polysplit::create_row_zlist()
{
    // Sort the intersection list
    std::sort(ps_isect_array, ps_isect_array + ps_icnt, isect_cmp);

    ps_zrow = 0;
    ps_zrow_end = 0;
    for (int j = 0; j < ps_icnt; j++) {
        isect_t *wl = ps_isect_array[j];
        if (!wl)
            continue;
        for (int k = j+1; k < ps_icnt; k++) {
            isect_t *wr = ps_isect_array[k];
            if (!wr)
                continue;
            if (wl->wrap == wr->wrap)
                continue;

            if (wr->xbot - wl->xbot > min_size ||
                    wr->xtop - wl->xtop > min_size) {
                if (ps_zrow_end && ps_zrow_end->xlr == wl->xbot &&
                        ps_zrow_end->xur == wl->xtop) {
                    ps_zrow_end->xlr = wr->xbot;
                    ps_zrow_end->xur = wr->xtop;
                }
                else {
                    zoid_t *zl = new_zoid(wl->xbot, wr->xbot, ps_ybot,
                        wl->xtop, wr->xtop, ps_ytop);
                    if (!ps_zrow)
                        ps_zrow = ps_zrow_end = zl;
                    else {
                        ps_zrow_end->next = zl;
                        ps_zrow_end = zl;
                    }
                }
            }
            ps_isect_array[k] = 0;
            break;
        }
        ps_isect_array[j] = 0;
    }
}


// Try to merge zoids in the new row with previous zoids, and link in
// the new row at the head of the list.
//
void
polysplit::add_row()
{
    // Attempt to merge vertically with previous zoids
    for (zoid_t *z = ps_zrow; z; z = z->next) {
        zoid_t *zp = 0, *zn;
        for (zoid_t *zl = ps_z0; zl; zl = zn) {
            if (zl->yl > ps_ytop)
                // end of previous row
                break;
            zn = zl->next;
            if (z->join_above(zl)) {
                if (!zp)
                    ps_z0 = zn;
                else
                    zp->next = zn;
                break;
            }
            zp = zl;
        }
    }

    // Add this row
    if (ps_zrow) {
        ps_zrow_end->next = ps_z0;
        ps_z0 = ps_zrow;
    }
}
// End of polysplit functions


// Return true if the three points are colinear within +/- n units.
//
static bool
check_colinear(int x1, int y1, int x2, int y2, int x3, int y3, int n)
{
    // The "cross product" is 2X the area of the triangle
    unsigned int crs = (unsigned int)
        fabs((x2 - x1)*(double)(y2 - y3) - (x2 - x3)*(double)(y2 - y1));

    // Instead of calling sqrt, use the following approximations for
    // distances between points
    unsigned int dd21 = abs(x2 - x1) + abs(y2 - y1);
    unsigned int dd31 = abs(x3 - x1) + abs(y3 - y1);
    unsigned int dd32 = abs(x3 - x2) + abs(y3 - y2);

    // The largest side is the triangle base
    unsigned int d = dd21;
    if (dd31 > d)
        d = dd31;
    if (dd32 > d)
        d = dd32;

    // crs/d is the triangle height, which is the distance from a vertex
    // to the line segment formed by the other two vertices.

    return (crs < d*n + d/2);
}


bool
zoid_t::join_above(zoid_t *zt)
{
    if (abs(yu - zt->yl) > 1 || abs(xul - zt->xll) > 1 ||
            abs(xur - zt->xlr) > 1)
        return (false);

    if (is_rect() && zt->is_rect()) {
        if (yu == zt->yl && xul == zt->xll && xur == zt->xlr) {
            xul = zt->xul;
            xur = zt->xur;
            yu = zt->yu;
            return (true);
        }
        return (false);
    }

    bool match = check_colinear(xll, yl, zt->xul, zt->yu, xul, yu, 1);
    if (!match && (zt->yl != yu || zt->xll != xul))
        match = check_colinear(xll, yl, zt->xul, zt->yu, zt->xll, zt->yl, 1);
    if (!match)
        return (false);

    match = check_colinear(xlr, yl, zt->xur, zt->yu, xur, yu, 1);
    if (!match && (zt->yl != yu || zt->xlr != xur))
        match = check_colinear(xlr, yl, zt->xur, zt->yu, zt->xlr, zt->yl, 1);
    if (!match)
        return (false);

    xul = zt->xul;
    xur = zt->xur;
    yu = zt->yu;

    // If a side is off-Manhattan by one, make it Manhattan by choosing
    // the even value
    if (abs(xul - xll) == 1) {
        if (xul & 1)
            xul = xll;
        else
            xll = xul;
    }
    if (abs(xur - xlr) == 1) {
        if (xur & 1)
            xur = xlr;
        else
            xlr = xur;
    }
    return (true);
}


namespace qtinterf
{
    // Return true if polygon is comvex.  It is convex if all adjacent edge
    // cross products have the same sign.
    //
    bool
    test_convex(GRmultiPt::pt_i *p, int numpts)
    {
        int xp0 = (p[0].x - p[numpts-2].x)*(p[1].y - p[0].y) -
            (p[0].y - p[numpts-2].y)*(p[1].x - p[0].x);

        numpts--;  // first and last points are the same
        numpts--;  // already tested first point
        while (numpts--) {
            int xp = (p[1].x - p[0].x)*(p[2].y - p[1].y) -
                (p[1].y - p[0].y)*(p[2].x - p[1].x);
            if (xp0 == 0)
                xp0 = xp;
            else if ((xp0 > 0 && xp < 0) || (xp0 < 0 && xp > 0)) {
                if (abs(xp0) > 2 && abs(xp) > 2)
                    return (false);
            }
            p++;
        }
        return (true);
    }
}

