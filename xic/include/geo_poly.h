
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

#ifndef GEO_POLY_H
#define GEO_POLY_H

#include "geo_point.h"


struct Zoid;
struct BBox;
struct Wire;
struct PolyList;
struct CDo;
struct CDol;
struct CDl;
struct CDs;
struct edg_t;

// winding code for poly
enum Otype {Onone, Ocw, Occw};

// Return flags for Poly::check_poly().  The REENT, ACUTE, and NON45,
// and NOREENT flags are (also) recognized in the value passed, to
// enable the "hard" reentrancy test, and checking for acute and
// non-45 angles, and disable all reentrancy tests.
//
#define PCHK_NVERTS    0x1
    // Too few vertices (error).
#define PCHK_OPEN      0x2
    // Last point not same as first (error).
#define PCHK_REENT     0x4
    // Poly overlaps itself (warning).
#define PCHK_ACUTE     0x8
    // Contains < 90 degree angle (warning).
#define PCHK_NON45     0x10
    // Contains non-45 degree multiple (warning).
#define PCHK_ZERANG    0x20
    // Needle vertex found (warning).
#define PCHK_FIXED     0x40
    // Vertices removed to fix problem (warning).
#define PCHK_NOREENT   0x80
    // Input only, disable all reentrancy testing (overrides REENT).
#define PCHK_ALL       (PCHK_REENT | PCHK_ACUTE | PCHK_NON45)
    // Convenience, for input.

// Polygon element.
//
struct Poly
{
    Poly() { points = 0; numpts = 0; }
    Poly(int num, Point *pts) { points = pts; numpts = num; }

    Poly *dup() const { return new Poly(numpts, Point::dup(points, numpts)); }

    static bool chkp(Point *p1, Point *p2, int n, int npoints)
        {
            int j = n+1;
            for (int i = 1; i < npoints; i++, j++) {
                if (j >= npoints)
                   j -= npoints;
                if (p1[i] != p2[j])
                    return (false);
            }
            return (true);
        }

    static bool chkp_r(Point *p1, Point *p2, int n, int npoints)
        {
            int j = n-1;
            for (int i = 1; i < npoints; i++, j--) {
                if (j < 0)
                   j += npoints;
                if (p1[i] != p2[j])
                    return (false);
            }
            return (true);
        }

    bool v_compare(const Poly &po) const
        {
            if (numpts != po.numpts)
                return (false);
            if (!numpts)
                return (true);
            Point *p = po.points;
            int npoints = numpts - 1;
            for (int i = 0; i < npoints; i++) {
                if (points[0] == p[i]) {
                    if (chkp(points, p, i, npoints))
                        return (true);
                    if (chkp_r(points, p, i, npoints))
                        return (true);
                }
            }
            return (false);
        }

    bool check_quick() const
        {
            if (numpts < 4)
                return (false);
            if (numpts == 4) {
                Point *p = points;
                if ((p[1].y - p[0].y)*(p[2].x - p[1].x) ==
                        (p[1].x - p[0].x)*(p[2].y - p[1].y)) {
                    return (false);
                }
            }
            else if (numpts == 5) {
                Point *p = points;
                if ((((p[1].y - p[0].y)*(p[2].x - p[1].x) ==
                        (p[1].x - p[0].x)*(p[2].y - p[1].y)) &&
                        ((p[3].y - p[2].y)*(p[0].x - p[3].x) ==
                        (p[3].x - p[2].x)*(p[0].y - p[3].y))) ||
                        (((p[0].y - p[3].y)*(p[1].x - p[0].x) ==
                        (p[0].x - p[3].x)*(p[1].y - p[0].y)) &&
                        ((p[2].y - p[1].y)*(p[3].x - p[2].x) ==
                        (p[2].x - p[1].x)*(p[3].y - p[2].y)))) {
                    return (false);
                }
            }
            return (true);
        }

    bool valid()
        {
            return (check_quick() && !(check_poly() & PCHK_NVERTS));
        }

    bool is_rect() const
        {
            if (numpts != 5)
                return (false);
            if (points[0].x == points[1].x && points[1].y == points[2].y &&
                    points[2].x == points[3].x && points[3].y == points[0].y)
                return (true);
            if (points[0].y == points[1].y && points[1].x == points[2].x &&
                    points[2].y == points[3].y && points[3].x == points[0].x)
                return (true);
            return (false);
        }

    bool is_manhattan() const
        {
            int n = numpts - 1;
            for (int i = 0; i < n; i++)
                if (points[i].x != points[i+1].x &&
                        points[i].y != points[i+1].y)
                    return (false);
            return (true);
        }

    // Return true if an angle is not a multiple of 45 degrees.
    bool has_non45() const
        {
            int n = numpts - 1;
            for (int i = 0; i < n; i++) {
                int dx = points[i+1].x - points[i].x;
                if (!dx)
                    continue;
                int dy = points[i+1].y - points[i].y;
                if (!dy)
                    continue;
                if (abs(dx) != abs(dy))
                    return (true);
            }
            return (false);
        }

    bool is_trapezoid() const
        {
            if (numpts == 4) {
                for (int i = 0; i < 3; i++) {
                    if (points[i].y == points[i+1].y) {
                        // horizontal triangle trapezoid
                        return (true);
                    }
                    if (points[i].x == points[i+1].x) {
                        // vertical triangle trapezoid
                        return (true);
                    }
                }
            }
            else if (numpts == 5) {
                Point *pts = points;
                for (int pass = 0; pass < 2; pass++) {
                    if (pts[0].y == pts[1].y && pts[2].y == pts[3].y) {
                        if ((pts[0].x < pts[1].x && pts[3].x < pts[2].x) ||
                                (pts[0].x > pts[1].x && pts[3].x > pts[2].x)) {
                            // horizontal trapezoid
                            return (true);
                        }
                    }
                    if (pts[0].x == pts[1].x && pts[2].x == pts[3].x) {
                        if ((pts[0].y < pts[1].y && pts[3].y < pts[2].y) ||
                                (pts[0].y > pts[1].y && pts[3].y > pts[2].y)) {
                            // vertical trapezoid
                            return (true);
                        }
                    }
                    pts++;
                }
            }
            return (false);
        }

    void reverse()
        {
            for (int i = 0, j = numpts-1; i < j; i++, j--) {
                Point p(points[i]);
                points[i] = points[j];
                points[j] = p;
            }
        }

    bool to_box(BBox*);
    void computeBB(BBox*) const;
    Otype winding() const;
    bool remove_vertex(int);
    int check_poly(int=0, bool=false);
    bool fix_tiny_jogs();
    double area() const;
    int perim() const;
    void centroid(double*, double*) const;
    bool intersect(const Point*, bool) const;
    bool intersect(const BBox*, bool) const;
    bool intersect(const Poly*, bool) const;
    bool intersect(const Wire*, bool) const;
    PolyList *divide(int) const;
    PolyList *clip(const BBox*, bool* = 0) const;
    Poly *clip_acute(int) const;
    edg_t *edges() const;
    Zlist *halo(int) const;
    Zlist *ext_zoids(int, int) const;

    // geo_ptozl.cc
    Zlist *toZlist() const;
    Zlist *toZlistR() const;

    static Zlist *pts3_to_zlist(const Point*, Zlist*);
    static Zlist *pts4_to_zlist(const Point*, Zlist*);

private:
    bool vertex_cross_test(int, int, int, Otype);

public:
    Point *points;
    int numpts;
};

// List element for polygons.
//
struct PolyList
{
    PolyList(Poly &p, PolyList *pn) { next = pn; po = p; }
    ~PolyList() { delete [] po.points; }

    static PolyList *new_poly_list(const Zlist*, bool);

    static void destroy(PolyList *p)
    {
        while (p) {
            PolyList *px = p;
            p = p->next;
            delete px;
        }
    }

    static CDo *to_odesc(PolyList*, CDl*);
    static CDol *to_olist(PolyList*, CDl*, CDol** = 0);

    PolyList *next;
    Poly po;
};

#endif

