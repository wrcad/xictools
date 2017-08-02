
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

#include "geo.h"
#include "geo_box.h"
#include "geo_poly.h"
#include "geo_linedb.h"
#include "geo_wire.h"
#include "smartarray.h"
#include <string.h>

// Uncomment for debugging output.
//#define WDEBUG

#ifdef WDEBUG
#include "timedbg.h"
#endif


// Compute the bounding box (pretty expensive).
//
void
Wire::computeBB(BBox *BB) const
{
    if (numpts <= 1) {
        // Assume a square, independent of end style.
        int halfw = wire_width()/2;
        BB->left = points[0].x - halfw;
        BB->bottom = points[0].y - halfw;
        BB->right = points[0].x + halfw;
        BB->top = points[0].y + halfw;
        return;
    }

    Poly po;
    if (toPoly(&po.points, &po.numpts)) {
        po.computeBB(BB);
        delete [] po.points;
        return;
    }

    // Bad points list, fake it.
    BB->left = points[0].x;
    BB->bottom = points[0].y;
    BB->right = points[0].x;
    BB->top = points[0].y;
    for (int i = 1; i < numpts; i++)
        BB->add(points[i].x, points[i].y);
    BB->bloat(wire_width()/2);
}


double
Wire::area() const
{
    double a = 0.0;
    Poly po;
    if (toPoly(&po.points, &po.numpts)) {
        a = po.area();
        delete [] po.points;
    }
    return (a);
}


int
Wire::perim() const
{
    int p = 0;
    Poly po;
    if (toPoly(&po.points, &po.numpts)) {
        p = po.perim();
        delete [] po.points;
    }
    return (p);
}


void
Wire::centroid(double *pcx, double *pcy) const
{
    Poly po;
    if (toPoly(&po.points, &po.numpts)) {
        po.centroid(pcx, pcy);
        delete [] po.points;
    }
    else {
        if (pcx)
            *pcx = 0.0;
        if (pcy)
            *pcy = 0.0;
    }
}


// Clipping function for wires, in a rectangle.  The clipped paths are
// returned in a PolyList, each element of which is the path points for
// a wire with the original end style and width.  If return_outside is
// set, the parts outside of the BB are returned
//
PolyList *
Wire::clip(const BBox *BB, bool return_outside) const
{
    PolyList *p0 = 0;
    Point *pi = points;
    Point *tmp = new Point[numpts+2];
    int i = 0;

    if (return_outside) {

        for (int n = 1; n < numpts; n++) {
            Point pf = pi[n-1];
            Point pl = pi[n];
            if (!cGEO::line_clip(&pf.x, &pf.y, &pl.x, &pl.y, BB)) {
                if (pf.x != pi[n-1].x || pf.y != pi[n-1].y) {
                    if (i == 0) {
                        tmp[0] = pi[n-1];
                        i = 1;
                    }
                    tmp[i] = pf;
                    i++;

                    Poly po;
                    po.points = new Point[i];
                    memcpy(po.points, tmp, i*sizeof(Point));
                    po.numpts = i;
                    p0 = new PolyList(po, p0);
                    i = 0;
                }
                if (pl.x != pi[n].x || pl.y != pi[n].y) {
                    tmp[0] = pl;
                    tmp[1] = pi[n];
                    i = 2;
                }
            }
            else {
                if (i == 0) {
                    tmp[0] = pf;
                    i = 1;
                }
                tmp[i] = pl;
                i++;
            }
        }
    }
    else {
        for (int n = 1; n < numpts; n++) {
            Point pf = pi[n-1];
            Point pl = pi[n];
            if (!cGEO::line_clip(&pf.x, &pf.y, &pl.x, &pl.y, BB)) {
                if (i == 0) {
                    tmp[i] = pf;
                    i++;
                }
                if (tmp[i-1].x != pl.x || tmp[i-1].y != pl.y) {
                    tmp[i] = pl;
                    i++;
                }
                if (pl.x != pi[n].x || pl.y != pi[n].y) {
                    Poly po;
                    po.points = new Point[i];
                    memcpy(po.points, tmp, i*sizeof(Point));
                    po.numpts = i;
                    p0 = new PolyList(po, p0);
                    i = 0;
                }
            }
            else if (i > 0) {
                if (i > 1) {
                    Poly po;
                    po.points = new Point[i];
                    memcpy(po.points, tmp, i*sizeof(Point));
                    po.numpts = i;
                    p0 = new PolyList(po, p0);
                }
                i = 0;
            }
        }
    }
    if (i > 1) {
        Poly po;
        po.points = new Point[i];
        memcpy(po.points, tmp, i*sizeof(Point));
        po.numpts = i;
        p0 = new PolyList(po, p0);
    }
    delete [] tmp;
    return (p0);
}


// Return true if the point is in the wire.  If touchok is true, the points
// on the wire boundary are considered as inside the wire.
//
bool
Wire::intersect(const Point *px, bool touchok) const
{
    Poly wp;
    bool ret = false;
    if (toPoly(&wp.points, &wp.numpts)) {
        if (wp.points) {
            ret = wp.intersect(px, touchok);
            delete [] wp.points;
        }
    }
    return (ret);
}


// Return true if the wire and box touch or overlap.  If touchok is true,
// the boundaries are considered internal points.
//
bool
Wire::intersect(const BBox *BB, bool touchok) const
{
    BBox tBB(points[0].x, points[0].y, points[0].x, points[0].y);
    for (int i = 1; i < numpts; i++)
        tBB.add(points[i].x, points[i].y);
    int wid = wire_width()/2;
    wid += wid/2;
    tBB.bloat(wid);
    if (!tBB.intersect(BB, false))
        return (false);

    bool ret = false;
    Poly wp;
    if (toPoly(&wp.points, &wp.numpts)) {
        ret = wp.intersect(BB, touchok);
        delete [] wp.points;
    }
    return (ret);
}


// Return true it the wire and polygon touch or overlap.  If touckok is
// true, the boundaries are considered internal points.
//
bool
Wire::intersect(const Poly *poly, bool touchok) const
{
    bool ret = false;
    Poly wp;
    if (toPoly(&wp.points, &wp.numpts)) {
        ret = wp.intersect(poly, touchok);
        delete [] wp.points;
    }
    return (ret);
}


// Return true if the two wires touch or overlap.  If touckok is true, the
// boundaries are considered internal points.
//
bool
Wire::intersect(const Wire *wire, bool touchok) const
{
    bool ret = false;
    Poly wp1, wp2;
    if (toPoly(&wp1.points, &wp1.numpts)) {
        if (wire->toPoly(&wp2.points, &wp2.numpts)) {
            ret = wp1.intersect(&wp2, touchok);
            delete [] wp2.points;
        }
        delete [] wp1.points;
    }
    return (ret);
}


namespace geo_wire {
    // A "smart" Point array for use in Wire::toPoly.  This reallocates
    // itself as needed to avoid overflow.
    //
    struct ptList : public SmartArray<Point>
    {
        ptList(int init_sz) : SmartArray<Point>(init_sz)
            {
                // Start with a the minimum size, add will fail otherwise.
                pl_num = 0;
                sa_list = new Point[init_sz];
                sa_size = init_sz;
            }

        // Append a point.
        void add(int x, int y)
            {
                if (pl_num >= sa_size) {
                    Point *ptmp = new Point[sa_size + sa_size];
                    memcpy(ptmp, sa_list, sa_size * sizeof(Point));
                    sa_size += sa_size;
                    delete [] sa_list;
                    sa_list = ptmp;
                }
                sa_list[pl_num++].set(x, y);
            }

        // Append the polygon closure point.
        void add_closure()
            {
                if (pl_num)
                    add(sa_list[0].x, sa_list[0].y);
            }

        Point *cur_point()
            {
                if (pl_num == 0)
                    return (0);
                return (sa_list + pl_num - 1);
            }

        int cur_num()               { return (pl_num); }
        void set_cur_num(int n)     { pl_num = n; }

        void remove_dups()          { Point::removeDups(sa_list, &pl_num); }

        int poly_check()
            {
                Poly po(pl_num, sa_list);
#ifdef WDEBUG
                Tdbg()->start_timing("polycheck");
                printf("orig %d\n", po.numpts);
#endif
                int ret = po.check_poly(PCHK_NOREENT, false);
                pl_num = po.numpts;
#ifdef WDEBUG
                printf("after %d\n", po.numpts);
                Tdbg()->stop_timing("polycheck");
                Tdbg()->stop_timing("wiretopoly");
#endif
                 return (ret);
            }

        // Shrink the internal array to minimum size.
        void resize()
            {
                if (pl_num != sa_size) {
                    Point *ptmp = new Point[pl_num];
                    memcpy(ptmp, sa_list, pl_num * sizeof(Point));
                    delete [] sa_list;
                    sa_list = ptmp;
                    sa_size = pl_num;
                }
            }

        // Return the array and used size.  This DOES NOT allow reuse,
        // as sa_list is zeroed.
        void finish(Point **pts, int *num)
            {
                *num = pl_num;
                pl_num = 0;
                clear(pts);
            }

    private:
        int pl_num;
    };
}


// Create a polygon from a wire description.  This can also return some
// flags describing wire characteristics.
//
bool
Wire::toPoly(Point **polypts, int *polynum, unsigned int *retflgs) const
{
    *polypts = 0;
    *polynum = 0;
    if (retflgs)
        *retflgs = 0;
    if (numpts < 1) {
        if (retflgs)
            *retflgs = CDWIRE_NOPTS;
        return (false);
    }

    int width = wire_width()/2;
    int style = wire_style();

    // Determine if there is only one vertex, don't trust the vertex
    // count.
    bool onev = true;
    for (int i = 1; i < numpts; i++) {
        if (points[i] != points[0]) {
            onev = false;
            break;
        }
    }
    if (onev) {
        if (retflgs)
            *retflgs |= CDWIRE_ONEVRT;
        if (width <= 0) {
            if (retflgs)
                *retflgs |= CDWIRE_BADWIDTH;
            return (false);
        }
        if (style == CDWIRE_EXTEND) {
            // Make a box.
            Point *p = new Point[5];
            p[0].set(points[0].x - width, points[0].y - width);
            p[1].set(points[0].x - width, points[0].y + width);
            p[2].set(points[0].x + width, points[0].y + width);
            p[3].set(points[0].x + width, points[0].y - width);
            p[4] = p[0];
            *polypts = p;
            *polynum = 5;
            return (true);
        }
        if (style == CDWIRE_ROUND) {
            // Make an octagon.
            Point *p = new Point[9];
            int hw = width/2;
            p[0].set(points[0].x - hw, points[0].y - width);
            p[1].set(points[0].x - width, points[0].y - hw);
            p[2].set(points[0].x - width, points[0].y + hw);
            p[3].set(points[0].x - hw, points[0].y + width);
            p[4].set(points[0].x + hw, points[0].y + width);
            p[5].set(points[0].x + width, points[0].y + hw);
            p[6].set(points[0].x + width, points[0].y - hw);
            p[7].set(points[0].x + hw, points[0].y - width);
            p[8] = p[0];
            *polypts = p;
            *polynum = 9;
            return (true);
        }
        if (retflgs)
            *retflgs |= CDWIRE_BADSTYLE;
        return (false);
    }

    if (width <= 0) {
        // If no width, return something semi-reasonable.
        if (retflgs)
            *retflgs |= CDWIRE_ZEROWIDTH;

        Point *pts = new Point[2*numpts - 1];
        pts[0] = points[0];
        int np = 0;
        for (int i = 1; i < numpts; i++) {
            if (points[i] != pts[np])
                pts[++np] = points[i];
        }
        for (int i = numpts - 2; i >= 0; i--) {
            if (points[i] != pts[np])
                pts[++np] = points[i];
        }
        *polypts = pts;
        *polynum = np + 1;
        return (true);
    }

    const Point *pts = points;
    int num = numpts;
    bool free_pts = false;

    // If there is a duplicate vertex, copy the points list and remove
    // dups.
    for (int i = 1; i < numpts; i++) {
        if (pts[i-1] == pts[i]) {
            Point *npts = new Point[num];
            memcpy(npts, points, num*sizeof(Point));
            Point::removeDups(npts, &num);
            pts = npts;
            free_pts = true;
            break;
        }
    }

    if (num == 2) {
        geo_wire::ptList ptl(12);
        wire_end(pts, pts+1, width, style, false, ptl);
        wire_end(pts, pts+1, width, style, true, ptl);
        wire_end(pts+1, pts, width, style, false, ptl);
        wire_end(pts+1, pts, width, style, true, ptl);
        ptl.add_closure();
        ptl.remove_dups();
        ptl.resize();
        ptl.finish(polypts, polynum);
        if (free_pts)
            delete [] pts;
        return (true);
    }

#ifdef WDEBUG
    Tdbg()->start_timing("wiretopoly");
#endif

    // Work area for the directly generated polygon.
    geo_wire::ptList ptl(3*num);

    // If a flush-end wire loops back to first vertex, construct the
    // end so it doesn't self-intersect.
    // Why bother? Good question, maybe because physical text looks
    // better on-screen.
    //
    // bool loopfix =  (style == CDWIRE_FLUSH && pts[0] == pts[num-1]);
    //
    // Good grief, that was obviously very stupid.
    bool loopfix = false;

    SmartArray<int> jvals(3*num);
    double dimen = 2*width;

    if (loopfix) {
        wire_vertex(pts+num-2, pts, pts+1, width, ptl);
        // Keep only the last point, if more than one.
        if (ptl.cur_num() > 1) {
            ptl[0] = *ptl.cur_point();
            ptl.set_cur_num(1);
        }
    }
    else
        wire_end(pts, pts+1, width, style, false, ptl);

    // The logic below is rather hideous, this needs a do-over.

    bool did_end = false;
    int nend = num - 1;
    bool idone = false;
    for (int i = 1; i < nend; i++) {
        int jfirst = ptl.cur_num();
        if (idone) {
            jfirst--;
            idone = false;
        }
        else
            wire_vertex(pts+i-1, pts+i, pts+i+1, width, ptl);
        double vdst = Point::distance(&pts[i], &pts[i+1]);
        if (vdst > dimen)
            continue;

        if (retflgs && vdst < width)
            *retflgs |= CDWIRE_CLOSEVRTS;

        // The following code attempts to clip out the mung caused by
        // closely-spaced vertices.  We look ahead for segments that
        // cross, and eliminate the enclosed points.  We can't be too
        // aggressive with this, since the wire itself may
        // self-intersect.

        // The jvals array contains a table of the offsets into the
        // poly points for the wire vertices, starting with the
        // present one.
        jvals[0] = jfirst;
        jvals[1] = ptl.cur_num();

        // Fill in the values for the points that are within dimen
        // of the present vertex.
        int jv0 = 1 - i;
        int k = i+1;
        bool tight = false;
        bool found = false;
        for ( ; ; k++) {
            if (k == nend) {
                if (loopfix)
                    wire_vertex(pts+k-1, pts+k, pts+1, width, ptl);
                else
                    wire_end(pts+k-1, pts+k, width, style, true, ptl);
            }
            else
                wire_vertex(pts+k-1, pts+k, pts+k+1, width, ptl);
            jvals[k + jv0] = ptl.cur_num();

            // If one of the segments crosses the present segment, we
            // still do the fix, but only if the new point is within
            // dimen of the old point.  Thus, we assume that such
            // points are due to the close vertices aand not the wire
            // intersection.

            if (!tight && k < nend) {
                if (cGEO::lines_cross(pts[i-1], pts[i], pts[k], pts[k+1], 0))
                    tight = true;
            }
            if (k == nend || Point::distance(&pts[i], &pts[k]) > dimen)
                break;
        }
        for (int j1 = jvals[0]; j1 < jvals[k + jv0] - 2; j1++) {
            for (int j2 = jvals[k + jv0] - 1; j2 > j1 + 1; j2--) {
                // Check for intersections.  If we find one, fix up
                // the points list.

                Point px;
                found = cGEO::lines_cross(ptl[j1-1], ptl[j1],
                    ptl[j2-1], ptl[j2], &px);
                if (found && tight && Point::distance(&px, &pts[j1]) > dimen)
                    found = false;
                if (found) {
                    int jx = j1;
                    ptl[jx++] = px;
                    for (int nk = i+1; ; nk++) {
                        // Find the value of k that corresponds to j2,
                        // we resume from there.
                        if (j2 > jvals[nk + jv0])
                            continue;
                        i = nk;
                        // If j2 was part of a multi-point run, include
                        // the remaining segments.
                        while (j2 < jvals[nk + jv0])
                            ptl[jx++] = ptl[j2++];
                        break;
                    }
                    ptl.set_cur_num(jx);
                    break;
                }
            }
            if (found)
                break;
        }
        if (!found) {
            idone = true;
            i = k-1;
            if (k == nend)
                did_end = true;
        }
        else if (retflgs)
            *retflgs |= CDWIRE_CLIPFIX;
        if (i == nend) {
            did_end = true;
            break;
        }
    }

    if (loopfix) {
        if (!did_end)
            wire_vertex(pts+num-2, pts+num-1, pts+1, width, ptl);
        wire_vertex(pts+1, pts+num-1, pts+num-2, width, ptl);
    }
    else {
        if (!did_end)
            wire_end(pts+num-2, pts+num-1, width, style, true, ptl);
        wire_end(pts+num-1, pts+num-2, width, style, false, ptl);
    }

    did_end = false;
    idone = false;
    for (int i = num - 2; i > 0; i--) {
        int jfirst = ptl.cur_num();
        if (idone) {
            jfirst--;
            idone = false;
        }
        else
            wire_vertex(pts+i+1, pts+i, pts+i-1, width, ptl);
        if (Point::distance(&pts[i], &pts[i-1]) > dimen)
            continue;

        jvals[0] = jfirst;
        jvals[1] = ptl.cur_num();
        int jv0 = 1 + i;
        int k = i-1;
        bool tight = false;
        for ( ; ; k--) {
            if (k == 0) {
                if (loopfix)
                    wire_vertex(pts+k+1, pts+k, pts+num-2, width, ptl);
                else
                    wire_end(pts+1, pts, width, style, true, ptl);
            }
            else
                wire_vertex(pts+k+1, pts+k, pts+k-1, width, ptl);
            jvals[jv0 - k] = ptl.cur_num();
            if (!tight && k > 0) {
                if (cGEO::lines_cross(pts[i+1], pts[i], pts[k], pts[k-1], 0))
                    tight = true;
            }
            if (k == 0 || Point::distance(&pts[i], &pts[k]) > dimen)
                break;
        }
        bool found = false;
        for (int j1 = jvals[0]; j1 < jvals[jv0-k] - 2; j1++) {
            for (int j2 = jvals[jv0 - k] - 1; j2 > j1+1; j2--) {
                // if segs intersect, take isect value.
                Point px;
                found = cGEO::lines_cross(ptl[j1-1], ptl[j1],
                    ptl[j2-1], ptl[j2], &px);
                if (found && tight && Point::distance(&px, &pts[j1]) > dimen)
                    found = false;
                if (found) {
                    int jx = j1;
                    ptl[jx++] = px;
                    for (int nk = i-1; ; nk--) {
                        if (j2 > jvals[jv0 - nk])
                            continue;
                        i = nk;
                        while (j2 < jvals[jv0 - nk])
                            ptl[jx++] = ptl[j2++];
                        break;
                    }
                    ptl.set_cur_num(jx);
                    break;
                }
            }
            if (found)
                break;
        }
        if (!found) {
            idone = true;
            i = k+1;
            if (k == nend)
                did_end = true;
        }
        else if (retflgs)
            *retflgs |= CDWIRE_CLIPFIX;
        if (i == 0) {
            did_end = true;
            break;
        }
    }
    if (!did_end) {
        if (loopfix)
            wire_vertex(pts+1, pts, pts+num-2, width, ptl);
        else
            wire_end(pts+1, pts, width, style, true, ptl);
    }
    ptl.add_closure();

    if (free_pts)
        delete [] pts;

    ptl.poly_check();
    ptl.resize();

    if (ptl.cur_num() > 600 && retflgs)
        *retflgs |= CDWIRE_BIGPOLY;

    ptl.finish(polypts, polynum);
    return (true);
}


// Return a trapezoid list representing the wire, based on the edge
// construction developed for bloating.  The list is raw and probably
// needs clip/merge before use.
//
// Too slow for general use.
//
Zlist *
Wire::toZlistFancy() const
{
    int width = wire_width()/2;
    int style = wire_style();

    linedb_t ldb;
    ldb.add(this, 0);
    int f = 2;  // "bloat mode" 2
    f |= BLCextend1 << BL_CORNER_MODE_SHIFT;
    Zlist *z0 = ldb.zoids(width, f);
    if (!z0)
        return (0);

    if (style != CDWIRE_FLUSH) {

        // Find adjacent point, making sure to skip duplicates.
        const Point *p = 0;
        for (int i = 1; i < numpts; i++) {
            if (points[i] != points[0]) {
                p = &points[i];
                break;
            }
        }

        Zlist *zl = cap_zoids(points, p, width, style);
        if (zl) {
            Zlist *zn = zl;
            while (zn->next)
                zn = zn->next;
            zn->next = z0;
            z0 = zl;
        }

        p = 0;
        int nv = numpts - 1;
        for (int i = nv - 1; i >= 0; i--) {
            if (points[i] != points[nv]) {
                p = &points[i];
                break;
            }
        }

        zl = cap_zoids(points + nv, p, width, style);
        if (zl) {
            Zlist *zn = zl;
            while (zn->next)
                zn = zn->next;
            zn->next = z0;
            z0 = zl;
        }
    }
    return (z0);
}


Zlist *
Wire::toZlist() const
{
    Poly wp;
    if (toPoly(&wp.points, &wp.numpts)) {
        Zlist *zl = wp.toZlist();
        delete [] wp.points;
        return (zl);
    }
    return (0);
}


// Rotate -90 degrees before decomposing
//
Zlist *
Wire::toZlistR() const
{
    Poly wp;
    if (toPoly(&wp.points, &wp.numpts)) {
        Zlist *zl = wp.toZlistR();
        delete [] wp.points;
        return (zl);
    }
    return (0);
}


// Return false if the wire is bad or questionable.  A wire is "bad" if
// it can't be rendered.  A wire is "questionable" if
//   - The width is zero (applies to physical data).
//   - The clipping fix is needed for rendering, bacause vertices are too
//     close.  This implies that rendering is non-trivial.
//   - The equivalent polygon is too big, again suggesting that there might
//     be a rendering problem.
//
bool
Wire::checkWire()
{
    Point *p;
    int n;
    unsigned int flags;
    if (!toPoly(&p, &n, &flags))
        return (false);
    delete [] p;
    unsigned int qflags = CDWIRE_ZEROWIDTH | CDWIRE_CLIPFIX | CDWIRE_BIGPOLY;
    return (!(flags & qflags));
}


namespace {
    // If the three vertices are colinear, return the point that is in
    // the interior, favoring p2 if p2 duplicates p1 or p3.
    //
    Point *internal_colinear(Point *p1, Point *p2, Point *p3)
    {
        if (GEO()->check_colinear(p1, p2, p3->x, p3->y, 0)) {
            int minx = p1->x;
            if (p3->x < minx)
                minx = p3->x;
            int maxx = p1->x;
            if (p3->x > maxx)
                maxx = p3->x;
            int miny = p1->y;
            if (p3->y < miny)
                miny = p3->y;
            int maxy = p1->y;
            if (p3->y > maxy)
                maxy = p3->y;
            if (p2->x >= minx && p2->x <= maxx &&
                    p2->y >= miny && p2->y <= maxy)
                return (p2);

            minx = p1->x;
            if (p2->x < minx)
                minx = p2->x;
            maxx = p1->x;
            if (p2->x > maxx)
                maxx = p2->x;
            miny = p1->y;
            if (p2->y < miny)
                miny = p2->y;
            maxy = p1->y;
            if (p2->y > maxy)
                maxy = p2->y;
            if (p3->x >= minx && p3->x <= maxx &&
                    p3->y >= miny && p3->y <= maxy)
                return (p3);
            return (p1);
        }
        return (0);
    }
}


// This function removes duplicate and co-linear vertices from the wire.
// If the wire doubles back on itself, it will be terminated, and the
// residual points will be returned in the arguments, which can be used
// to create another wire.
// WARNING: This assumes that the points list is free-store.
// WARNING: Applies to physical mode only.  In electrical mode, vertices
// indicate connection points and may well be inline.
//
void
Wire::checkWireVerts(Point **residual, int *nresidual)
{
    *residual = 0;
    *nresidual = 0;
    if (!points || !numpts)
        return;
    int i = numpts - 1;
    Point *p1, *p2;
    for (p1 = points, p2 = p1 + 1; i > 0; i--, p2++) {
        if (*p1 == *p2) {
            numpts--;
            continue;
        }
        if (i > 1) {
            Point *p3 = p2+1;
            Point *px = internal_colinear(p1, p2, p3);
            if (px) {

                // Points p1, p2, p3 are colinear.
                if (px == p2) {
                    // Point p2 is within p1,p3 delete p2.
                    numpts--;
                    continue;
                }
                if (px == p1) {
                    // Point p1 is within p2,p3.
                    if (p1 == points) {
                        // Point p1 is the start point, effectively
                        // delete it.
                        *p1 = *p2;
                        numpts--;
                        continue;
                    }
                    // Otherwise, terminate path and start a new one.
                    int n1 = p1 - points + 1;
                    Point *pts = new Point[n1];
                    memcpy(pts, points, n1*sizeof(Point));
                    Point *xpts = points;
                    points = pts;
                    int n2 = numpts - n1;
                    numpts = n1;

                    pts = new Point[n2];
                    memcpy(pts, p2, n2*sizeof(Point));
                    delete [] xpts;
                    *residual = pts;
                    *nresidual = n2;
                    return;
                }
                if (px == p3) {
                    // Point p3 is within p1,p2.
                    if (p3 == points + numpts - 1) {
                        // Point p3 is the end point, effectively
                        // delete it.
                        *p3 = *p2;
                        numpts--;
                        break;
                    }
                    // Otherwise, terminate path and start a new one.
                    int n1 = p1 - points + 2;
                    Point *pts = new Point[n1];
                    memcpy(pts, points, (n1-1)*sizeof(Point));
                    pts[n1-1] = *p2;
                    Point *xpts = points;
                    points = pts;
                    int n2 = numpts - n1;
                    numpts = n1;

                    pts = new Point[n2];
                    memcpy(pts, p3, n2*sizeof(Point));
                    delete [] xpts;
                    *residual = pts;
                    *nresidual = n2;
                    return;
                }
            }
        }
        p1++;
        if (p1 != p2)
            *p1 = *p2;
    }
}


// Static function.
// Compose a message in buf listing the toPoly flags.
//
void
Wire::flagWarnings(char *buf, unsigned int flags, const char *msgstart)
{
    char *t = buf;
    if (msgstart)
        t = lstring::stpcpy(t, msgstart);

    if (!flags) {
        strcpy(t, "no flags set");
        return;
    }

    char *t0 = t;
    if (flags & CDWIRE_ONEVRT)
        t = lstring::stpcpy(t, "ONEVERT");
    if (flags & CDWIRE_ZEROWIDTH) {
        if (t0 != t)
            *t++ = ',';
        t = lstring::stpcpy(t, "ZEROWIDTH");
    }
    if (flags & CDWIRE_CLOSEVRTS) {
        if (t0 != t)
            *t++ = ',';
        t = lstring::stpcpy(t, "CLOSEVERTS");
    }
    if (flags & CDWIRE_CLIPFIX) {
        if (t0 != t)
            *t++ = ',';
        t = lstring::stpcpy(t, "CLIPFIX");
    }
    if (flags & CDWIRE_BIGPOLY) {
        if (t0 != t)
            *t++ = ',';
        t = lstring::stpcpy(t, "BIGPOLY");
    }
}


// Static functiion.
// Return a trapezoid list representing the wire end cap, for non-flush
// wires.  Point pf is the endpoint, pn is the adjacent point, w is the
// half-width.
//
Zlist *
Wire::cap_zoids(const Point *pf, const Point *pn, int w, int style)
{
    if (style == CDWIRE_FLUSH)
        return (0);

    const double p3 = .333333;
    int dx1 = pn->x - pf->x;
    int dy1 = pn->y - pf->y;

    Point pts[5];
    if (dx1 == 0) {
        if (dy1 == 0)
            return (0);
        int z = (dy1 >= 0 ? w : -w);
        if (style == CDWIRE_ROUND) {
            // contoured end
            pts[0].set(mmRnd(pf->x - p3*z), pf->y - z);
            pts[1].set(pf->x - z, pf->y);
            pts[2].set(pf->x + z, pf->y);
            pts[3].set(mmRnd(pf->x + p3*z), pf->y - z);
        }
        else {
            // extended end (CDWIRE_EXTEND)
            pts[0].set(pf->x - z, pf->y - z);
            pts[1].set(pf->x - z, pf->y);
            pts[2].set(pf->x + z, pf->y);
            pts[3].set(pf->x + z, pf->y - z);
        }
    }
    else if (dy1 == 0) {
        int z = (dx1 >= 0 ? w : -w);
        if (style == CDWIRE_ROUND) {
            // contoured end
            pts[0].set(pf->x - z, mmRnd(pf->y + p3*z));
            pts[1].set(pf->x, pf->y + z);
            pts[2].set(pf->x, pf->y - z);
            pts[3].set(pf->x - z, mmRnd(pf->y - p3*z));
        }
        else {
            // extended end (CDWIRE_EXTEND)
            pts[0].set(pf->x - z, pf->y + z);
            pts[1].set(pf->x, pf->y + z);
            pts[2].set(pf->x, pf->y - z);
            pts[3].set(pf->x - z, pf->y - z);
        }
    }
    else {
        double d1 = sqrt((double)dx1*dx1 + (double)dy1*dy1);
        if (d1 == 0)
            return (0);
        double z = w/d1;
        if (style == CDWIRE_ROUND) {
            // contoured end
            pts[0].set(mmRnd(pf->x - (p3*dy1 + dx1)*z),
                mmRnd(pf->y + (p3*dx1 - dy1)*z));
            pts[1].set(mmRnd(pf->x - dy1*z), mmRnd(pf->y + dx1*z));
            pts[2].set(mmRnd(pf->x + dy1*z), mmRnd(pf->y - dx1*z));
            pts[3].set(mmRnd(pf->x + (p3*dy1 - dx1)*z),
                mmRnd(pf->y - (p3*dx1 + dy1)*z));
        }
        else {
            // extended end (CDWIRE_EXTEND)
            pts[0].set(mmRnd(pf->x - (dy1 + dx1)*z),
                mmRnd(pf->y + (dx1 - dy1)*z));
            pts[1].set(mmRnd(pf->x - dy1*z), mmRnd(pf->y + dx1*z));
            pts[2].set(mmRnd(pf->x + dy1*z), mmRnd(pf->y - dx1*z));
            pts[3].set(mmRnd(pf->x + (dy1 - dx1)*z),
                mmRnd(pf->y - (dx1 + dy1)*z));
        }
    }
    pts[4] = pts[0];

    Poly ply(5, pts);
    return (ply.toZlist());
}


// Static function.
// Convert between GDS pathtype 0 (flush ends) and
// pathtype 2 (extended ends).
// int *xe, *ye;      coordinate of endpoint
// int xb, yb;        coordinate of previous or next point in path
// int width;         half path width
// int totwo;         true if pathtype 0 -> 2
//
void
Wire::convert_end(int *xe, int *ye, int xb, int yb, int width, bool totwo)
{
    if (width == 0)
        return;
    if (totwo) {
        // retract
        if (*xe == xb) {
            if (*ye > yb)
                *ye -= width;
            else
                *ye += width;
        }
        else if (*ye == yb) {
            if (*xe > xb)
                *xe -= width;
            else
                *xe += width;
        }
        else {
            double delta_x = (double)(*xe - xb);
            double delta_y = (double)(*ye - yb);
            double angle = atan2(delta_y, delta_x);
            delta_x = (double)(width) * cos(angle);
            delta_y = (double)(width) * sin(angle);
            *xe -= mmRnd(delta_x);
            *ye -= mmRnd(delta_y);
        }
    }
    else {
        // extend
        if (*xe == xb) {
            if (*ye > yb)
                *ye += width;
            else
                *ye -= width;
        }
        else if (*ye == yb) {
            if (*xe > xb)
                *xe += width;
            else
                *xe -= width;
        }
        else {
            double delta_x = (double)(*xe - xb);
            double delta_y = (double)(*ye - yb);
            double angle = atan2(delta_y, delta_x);
            delta_x = (double)(width) * cos(angle);
            delta_y = (double)(width) * sin(angle);
            *xe += mmRnd(delta_x);
            *ye += mmRnd(delta_y);
        }
    }
}


namespace {
    inline bool OPP(int a, int b)
    {
        return ((a > 0 && b < 0) || (a < 0 && b > 0));
    }
}


// Static Function (private);
// Compute the points for the polygon at interior vertex pm.  pf is
// the previous point, and pl the next.  If the angle is too sharp,
// clip the projection, and add an additional point.
//
bool
Wire::wire_vertex(const Point *pf, const Point *pm, const Point *pl,
    int w, geo_wire::ptList &ptl)
{
    if (w == 0) {
        ptl.add(pm->x, pm->y);
        return (true);
    }
    int dx1 = pm->x - pf->x;
    int dy1 = pm->y - pf->y;
    if (!dx1 && !dy1)
        return (false);
    int dx2 = pl->x - pm->x;
    int dy2 = pl->y - pm->y;
    if (!dx2 && !dy2)
        return (false);

    if ((!dx1 || !dy1) && (!dx2 || !dy2)) {
        // Manhattan case.

        if (!dx1)  {
            int xx = (dy1 > 0 ? -w : w);
            if (!dx2) {
                if (OPP(dy1, dy2)) {
                    // double back
                    ptl.add(pm->x + xx, pm->y);
                    ptl.add(pm->x - xx, pm->y);
                }
                else
                    return (true);
            }
            else {
                int yy = (dx2 > 0 ? w : -w);
                ptl.add(pm->x + xx, pm->y + yy);
            }
        }
        else {
            int yy = (dx1 > 0 ? w : -w);
            if (!dx2) {
                int xx = (dy2 > 0 ? -w : w);
                ptl.add(pm->x + xx, pm->y + yy);
            }
            else {
                if (OPP(dx1, dx2)) {
                    // double back
                    ptl.add(pm->x, pm->y + yy);
                    ptl.add(pm->x, pm->y - yy);
                }
                else
                    return (true);
            }
        }
        return (true);
    }

    double d1, xx1, yy1;
    if (dx1 == 0) {
        d1 = abs(dy1);
        xx1 = (dy1 > 0 ? -w : w);
        yy1 = 0;
    }
    else if (dy1 == 0) {
        d1 = abs(dx1);
        xx1 = 0;
        yy1 = (dx1 > 0 ? w : -w);
    }
    else {
        d1 = sqrt(dx1*(double)dx1 + dy1*(double)dy1);
        double z = w/d1;
        xx1 = -dy1*z;
        yy1 = dx1*z;
    }

    double d2, xx2, yy2;
    if (dx2 == 0) {
        d2 = abs(dy2);
        xx2 = (dy2 > 0 ? -w : w);
        yy2 = 0;
    }
    else if (dy2 == 0) {
        d2 = abs(dx2);
        xx2 = 0;
        yy2 = (dx2 > 0 ? w : -w);
    }
    else {
        d2 = sqrt(dx2*(double)dx2 + dy2*(double)dy2);
        double z = w/d2;
        xx2 = -dy2*z;
        yy2 = dx2*z;
    }

    /********
    // Solve for the intersection of the boundary segments.
    //
    Point_c p11(mmRnd(pm->x + xx1), mmRnd(pm->y + yy1));
    Point_c p12(mmRnd(pf->x + xx1), mmRnd(pf->y + yy1));
    Point_c p21(mmRnd(pm->x + xx2), mmRnd(pm->y + yy2));
    Point_c p22(mmRnd(pl->x + xx2), mmRnd(pl->y + yy2));

    lsegx_t sx(p11, p12, p21, p22);
    if (sx.dd == 0.0)
        return (true);

    ptl.add(mmRnd(mmRnd(p11.x + sx.n1*(p12.x - p11.x)/sx.dd)),
        mmRnd(mmRnd(p11.y + sx.n1*(p12.y - p11.y)/sx.dd)));
    ********/

    double x10 = -dx1;
    double y10 = -dy1;
    double x22 = dx2;
    double y22 = dy2;
    double dd = x22*y10 - y22*x10;
    if (dd == 0.0) {
        if (OPP(dx1, dx2)) {
            // Both segments are non-Manhattan, could check x or y.
            ptl.add(mmRnd(pm->x + xx1), mmRnd(pm->y + yy1));
            ptl.add(mmRnd(pm->x - xx1), mmRnd(pm->y - yy1));
        }
        return (true);
    }
    double x20 = xx2 - xx1;
    double y20 = yy2 - yy1;
    double n1 = x22*y20 - y22*x20;

    double n1d = n1/dd;
    ptl.add(mmRnd(pm->x + xx1 + n1d*x10), mmRnd(pm->y + yy1 + n1d*y10));

    double dpx = ptl.cur_point()->x - pm->x;
    double dpy = ptl.cur_point()->y - pm->y;
    double dp = dpx*dpx + dpy*dpy;
    bool out = (n1d < 0);
    double w2 = w;
    w2 *= 2.0*w2;
    if ((out && w2 < 0.9*dp) ||
        // 'outisde' point && corner sticks out too far
            (!out && (dp > d1*d1 + w2 || dp > d2*d2 + w2))) {
        // 'inside' point && doubles back
        ptl.cur_point()->x = mmRnd(pm->x + xx1);
        ptl.cur_point()->y = mmRnd(pm->y + yy1);

        dp = sqrt(dp);
        if (out)
            ptl.add(mmRnd(pm->x + w*dpx/dp), mmRnd(pm->y + w*dpy/dp));
        else
            ptl.add(mmRnd(pm->x - w*dpx/dp), mmRnd(pm->y - w*dpy/dp));

        ptl.add(mmRnd(pm->x + xx2), mmRnd(pm->y + yy2));
    }
    return (true);
}


// Static Function (private).
// Compute the end poly points for the wire.  Return false if
// duplicate vertex.
//
bool
Wire::wire_end(const Point *pf, const Point *pn, int w, int style, int end,
    geo_wire::ptList &ptl)
{
    const double p3 = .333333;
    int dx1 = pn->x - pf->x;
    int dy1 = pn->y - pf->y;
    if (!dx1 && !dy1)
         return (false);

    if (!end) {
        // starting path
        if (dx1 == 0) {
            int z = (dy1 >= 0 ? w : -w);
            if (style == CDWIRE_FLUSH)
                ptl.add(pf->x - z, pf->y);
            else if (style == CDWIRE_ROUND) {
                ptl.add(mmRnd(pf->x - p3*z), pf->y - z);
                ptl.add(pf->x - z, pf->y);
            }
            else // extended end (CDWIRE_EXTEND)
                ptl.add(pf->x - z, pf->y - z);
        }
        else if (dy1 == 0) {
            int z = (dx1 >= 0 ? w : -w);
            if (style == CDWIRE_FLUSH)
                ptl.add(pf->x, pf->y + z);
            else if (style == CDWIRE_ROUND) {
                ptl.add(pf->x - z, mmRnd(pf->y + p3*z));
                ptl.add(pf->x, pf->y + z);
            }
            else // extended end (CDWIRE_EXTEND)
                ptl.add(mmRnd(pf->x - z), mmRnd(pf->y + z));
        }
        else {
            double d1 = sqrt((double)dx1*dx1 + (double)dy1*dy1);
            double z = w/d1;
            if (style == CDWIRE_FLUSH)
                ptl.add(mmRnd(pf->x - dy1*z), mmRnd(pf->y + dx1*z));
            else if (style == CDWIRE_ROUND) {
                ptl.add(mmRnd(pf->x - (p3*dy1 + dx1)*z),
                    mmRnd(pf->y + (p3*dx1 - dy1)*z));
                ptl.add(mmRnd(pf->x - dy1*z), mmRnd(pf->y + dx1*z));
            }
            else // extended end (CDWIRE_EXTEND)
                ptl.add(mmRnd(pf->x - (dy1 + dx1)*z),
                    mmRnd(pf->y + (dx1 - dy1)*z));
        }
    }
    else {
        // ending path
        if (dx1 == 0) {
            int z = (dy1 >= 0 ? w : -w);
            if (style == CDWIRE_FLUSH)
                ptl.add(pn->x - z, pn->y);
            else if (style == CDWIRE_ROUND) {
                ptl.add(pn->x - z, pn->y);
                ptl.add(mmRnd(pn->x - p3*z), pn->y + z);
            }
            else // extended end (CDWIRE_EXTEND)
                ptl.add(pn->x - z, pn->y + z);
        }
        else if (dy1 == 0) {
            int z = (dx1 >= 0 ? w : -w);
            if (style == CDWIRE_FLUSH)
                ptl.add(pn->x, pn->y + z);
            else if (style == CDWIRE_ROUND) {
                ptl.add(pn->x, pn->y + z);
                ptl.add(pn->x + z, mmRnd(pn->y + p3*z));
            }
            else // extended end (CDWIRE_EXTEND)
                ptl.add(pn->x + z, pn->y + z);
        }
        else {
            double d1 = sqrt((double)dx1*dx1 + (double)dy1*dy1);
            double z = w/d1;
            if (style == CDWIRE_FLUSH)
                ptl.add(mmRnd(pn->x - dy1*z), mmRnd(pn->y + dx1*z));
            else if (style == CDWIRE_ROUND) {
                ptl.add(mmRnd(pn->x - dy1*z), mmRnd(pn->y + dx1*z));
                ptl.add(mmRnd(pn->x - (p3*dy1 - dx1)*z),
                    mmRnd(pn->y + (p3*dx1 + dy1)*z));
            }
            else // extended end (CDWIRE_EXTEND)
                ptl.add(mmRnd(pn->x - (dy1 - dx1)*z),
                    mmRnd(pn->y + (dx1 + dy1)*z));
        }
    }
    return (true);
}

