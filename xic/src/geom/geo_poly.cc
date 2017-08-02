
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
#include "geo_poly.h"
#include "geo_zlist.h"
#include "geo_line.h"
#include "geo_linedb.h"
#include "geo_wire.h"
#include "geo_ptozl.h"
#include <string.h>


#ifndef M_PI
#define M_PI        3.14159265358979323846  // pi
#endif
#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923  // pi/2
#endif
#ifndef M_PI_4
#define M_PI_4      0.78539816339744830962  // pi/4
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2   0.70710678118654752440  // 1/sqrt(2)
#endif


// Return true if poly is a rectangle, and set the BBox entries.
//
bool
Poly::to_box(BBox *BB)
{
    if (is_rect()) {
        *BB = BBox(points);
        return (true);
    }
    return (false);
}


// Compute the bounding box.
//
void
Poly::computeBB(BBox *BB) const
{
    BB->left = points->x;
    BB->bottom = points->y;
    BB->right = points->x;
    BB->top = points->y;
    int n = numpts - 1;
    for (Point *p = points + 1; n; p++, n--)
        BB->add(p->x, p->y);
}


// Determine whether the poly is wound clockwise or not.
//
Otype
Poly::winding() const
{
    double a = 0.0;
    Point p0(points[0]);
    double x0 = (points[1].x - p0.x);
    double y0 = (points[1].y - p0.y);
    const int n = numpts - 2;
    for (int i = 1; i < n; i++) {
        double x1 = (points[i+1].x - p0.x);
        double y1 = (points[i+1].y - p0.y);
        a += (x0*y1 - x1*y0);
        x0 = x1;
        y0 = y1;
    }
    if (a < 0)
        return (Ocw);
    if (a > 0)
        return (Occw);
    return (Onone);
}


// Remove vertex i from the polygon, return false if there are too few
// points.
//
bool
Poly::remove_vertex(int i)
{
    if (i == 0)
        points[numpts-1] = points[1];
    numpts--;
    for (int k = i; k < numpts; k++)
        points[k] = points[k+1];
    return (numpts > 3);
}


namespace {
    inline double dst(int x, int y)
    {
        if (!y)
            return ((double)abs(x));
        if (!x)
            return ((double)abs(y));
        if (x == y)
            return (M_SQRT2*abs(x));
        return (sqrt(x*(double)x + y*(double)y));
    }


    inline bool is_inline(int dx, int dy, int dx1, int dy1)
    {
        if ((!dy && !dy1) || (!dx && !dx1))
            return (true);
        if ((!dy && !dx1) || (!dx && !dy1))
            return (false);
        if (dx + dx1 == 0 && dy + dy1 == 0)
            return (true);

        double cross = dx*(double)dy1 - dx1*(double)dy;
        double dot = dx*(double)dx1 + dy*(double)dy1;
        if (fabs(cross) >= fabs(dot))
            return (false);

        // area = 0.5*fabs(cross) = 0.5*base*height,
        // where base = length of longest side of formed triangle.
        // We take inline vertices if height < THR.

#define THR 1.0
        if (dot > 0.0) {
            // In line, not a needle, p0-p2 must be the longest edge.
            double dd = dst(dx + dx1, dy + dy1);
            return (fabs(cross)/dd < THR);
        }
        double d1 = dst(dx, dy);
        double d2 = dst(dx1, dy1);
        if (d1 > d2)
            return (fabs(cross)/d1 < THR);
        return (fabs(cross)/d2 < THR);
    }


    inline int quad(int x, int y)
    {
        // 0,0 never gets here
        if (x > 0)
            return (y >= 0 ? 0 : 1);
        if (x == 0)
            return (y < 0 ? 1 : 3);
        return (y <= 0 ? 2 : 3);
    }
}


// Test the polygon for vertex count, closure, reentrancy, etc. 
// Inline vertices and "needles" are removed, unless nofix is true. 
// If a needle or inline vertex is found, the PCHK_ZERANG flag is set
// in the return, as is PCHK_FIXED if the vertex is removed.  If
// removing a vertex causes too few remaining vertices, or the poly
// never had enough vertices, PCHK_NVERTS is set in the return.  If
// the points list is not closed, PCHK_OPEN is set if nofix.  These
// flags should be considered "errors", others are warnings or
// informational.
//
// The test_flags argument controls which tests to apply, beyond the
// basic testing above.  The following bits are recognized:
// PCHK_REENT
//   Perform an expensive but more careful test for reentrancy, if the
//   basic winding test succeeds. 
// PCHK_ACUTE
//   Set this flag in the return if the polygon contains an acute angle.
// PCHK_NON45
//   Set this flag in the return if the polygon contains a non-45 angle.
// PCHK_NOREENT
//   Set this to avoid any reentrancy testing.
//
int
Poly::check_poly(int test_flags, bool nofix)
{
    // Check vertex count.
    if (numpts < 3)
        return (PCHK_NVERTS);

    // Check closure.
    if (points[0] != points[numpts-1]) {
        if (nofix)
            return (PCHK_OPEN);
        // Add the closure point.
        Point *p = new Point[numpts+1];
        memcpy(p, points, numpts * sizeof(Point));
        numpts++;
        p[numpts-1] = p[0];
        delete [] points;
        points = p;
    }
    else if (numpts == 3)
        return (PCHK_NVERTS);

    bool test_acute = (test_flags & PCHK_ACUTE);
    bool test_non45 = (test_flags & PCHK_NON45);

    // If the poly represents a horizontal or vertical trapezoid, it
    // will be accepted.
    //
    bool is_trap = is_trapezoid();
    if (is_trap && !test_acute && !test_non45)
        return (0);

    int dx = points[0].x - points[numpts-2].x;
    int dy = points[0].y - points[numpts-2].y;
    while (dx == 0 && dy == 0) {
        // Duplicate vertex, remove it.
        if (!remove_vertex(0))
            return (PCHK_NVERTS);
        dx = points[0].x - points[numpts-2].x;
        dy = points[0].y - points[numpts-2].y;
    }

    // First check for vertices that can be removed.
    int dx1;
    int dy1;
    int ret = 0;
    int npts = numpts - 1;
    for (int i = 0; i < npts; i++, dx = dx1, dy = dy1) {

        dx1 = points[i+1].x - points[i].x;
        dy1 = points[i+1].y - points[i].y;
        if (dx1 == 0 && dy1 == 0) {
            // Duplicate vertex, remove it.
            do {
                if (!remove_vertex(i+1)) {
                    ret &= ~PCHK_FIXED;
                    ret |= PCHK_NVERTS;
                    return (ret);
                }
                npts--;
                if (i == npts)
                    break;
                dx1 = points[i+1].x - points[i].x;
                dy1 = points[i+1].y - points[i].y;
            } while (dx1 == 0 && dy1 == 0);
            if (i == npts)
                break;
        }

        if (!is_trap && is_inline(dx, dy, dx1, dy1)) {

            // Inline vertex or needle.
            ret |= PCHK_ZERANG;
            if (!nofix) {
                // Try to get rid of bad vertex.  If this is not possible,
                // PCHK_NVERTS will be set.  On success, PCHK_FIXED will
                // be set.
                if (!remove_vertex(i)) {
                    ret &= ~PCHK_FIXED;
                    ret |= PCHK_NVERTS;
                    return (ret);
                }
                ret |= PCHK_FIXED;

                if (i > 2) {
                    // Back up 2 vertices to re-do inverted-needle check.
                    dx1 = points[i-2].x - points[i-3].x;
                    dy1 = points[i-2].y - points[i-3].y;
                    npts = numpts - 1;
                    is_trap = is_trapezoid();
                    i -= 3;
                }
                else {
                    // Start the loop over.
                    dx1 = points[0].x - points[numpts-2].x;
                    dy1 = points[0].y - points[numpts-2].y;
                    npts = numpts - 1;
                    is_trap = is_trapezoid();
                    i = -1;
                }
            }
        }
    }
    bool noreent = is_trap ? true : (test_flags & PCHK_NOREENT);
    if (noreent && !test_acute && !test_non45)
        return (ret);

    // Now test for acute angles, non-45s, and rentrancy.

    dx = points[0].x - points[numpts-2].x;
    dy = points[0].y - points[numpts-2].y;

    int windnum = 0;
    npts = numpts - 1;
    for (int i = 0; i < npts; i++, dx = dx1, dy = dy1) {
        dx1 = points[i+1].x - points[i].x;
        dy1 = points[i+1].y - points[i].y;

        if (test_acute) {
            double dot = dx*(double)dx1 + dy*(double)dy1;
            if (dot < -0.001) {
                ret |= PCHK_ACUTE;
                test_acute = false;
            }
        }
        if (test_non45) {
            if (abs(dx) > 1 && abs(dy) > 1 && abs(abs(dx) - abs(dy)) > 1) {
                ret |= PCHK_NON45;
                test_non45 = false;
            }
        }
        if (noreent)
            continue;

        //-----------------------------------------------------------------
        // How this works.
        // One can obtain the winding by summing the angles:
        //     double dot = dx*(double)dx1 + dy*(double)dy1;
        //     double theta = atan2(cross, dot);
        //     asum += theta;
        // The sum will be a multiple of 2*M_PI, with a factor other
        // than +/-1 indicating multiple winding.  The code below gets
        // around the need for the expensive atan2 call.
        //
        // Consider the following:
        //     double th1 = atan2(dx, dy);
        //     double th2 = atan2(dx1, dy1);
        //     double th = th1 - th2;
        //     if (th > M_PI) {
        //         th -= 2*M_PI;
        //         asum -= 2*M_PI;
        //     }
        //     if (th < -M_PI) {
        //         th += 2*M_PI;
        //         asum += 2*M_PI;
        //     }
        //     assert: th == theta
        //
        // If we sum th1 - th2, the sum will always be 0 (obviously). 
        // However, modifying the difference by keeping the difference
        // in the +/- M_PI range as above causes the difference to
        // equate to the theta from the first example.  So, summing th
        // would yield the same result as summing theta.  Note,
        // however, that we really only need to add to the sum when
        // the 2*M_PI corrections are added to th. 
        //
        // Thus, we need to recognize when the th1 - th2 term would
        // require the correction.  The following code accomplishes
        // this.  The quadrant which contains each vector is
        // determined, and the difference is used to identify when a
        // correction is needed.  Quads 2-1 and 1-2 always need a
        // correction, a difference of +/-2 may need a correction
        // depending on the sign of cross (same as theta).  Other
        // differences never need the correction. 
        // ----------------------------------------------------------------

        int q = quad(dx, dy);
        int q1 = quad(dx1, dy1);
        if (q1 == q)
            continue;
        int qdiff = q1 - q;
        if (qdiff == 3 || qdiff == -3)
            continue;
        if (q1 == 2 && q == 1) {
            windnum += 1;
            continue;
        }
        if (q1 == 1 && q == 2) {
            windnum -= 1;
            continue;
        }
        if (qdiff == 1 || qdiff == -1)
            continue;

        if (qdiff == 2) {
            double cross = dx*(double)dy1 - dx1*(double)dy;
            if (cross < 0)
                windnum += 1;
        }
        else if (qdiff == -2) {
            double cross = dx*(double)dy1 - dx1*(double)dy;
            if (cross > 0)
                windnum -= 1;
        }
    }
    if (noreent)
        return (ret);

    if (windnum != 1 && windnum != -1) {
        // Poly in reentrant, or has unfixed needles.  The poly may have
        // multiple cycles, figure-8, etc.
        ret |= PCHK_REENT;
        return (ret);
    }

    // If the test above passes, there may still be loops in pairs
    // with opposite winding sense.  This requires at minimum five
    // vertices.

    if (!(test_flags & PCHK_REENT) || numpts < 6)
        return (ret);

    Otype wind = windnum < 0 ? Occw : Ocw;

    // Look for segments that cross.
    for (int i = 1; i < numpts; i++) {
        sLine l1 = sLine(&points[i-1], &points[i]);
        int n = (i == 1 ? numpts-1 : numpts);
        for (int j = i+2; j < n; j++) {
            int code;
            sLine l2 = sLine(&points[j-1], &points[j]);
            if (l1.lineIntersect(&l2, &code)) {
                ret |= PCHK_REENT;
                return (ret);
            }
            if (code) {
                // code == -1 ==> crossing at j-1
                // code ==  1 ==> crossing at j

                if (vertex_cross_test(code, i, j, wind)) {
                    ret |= PCHK_REENT;
                    return (ret);
                }
                // Check the sub-polygon for reentrancy, this should
                // catch cases where the number of cycles is odd and
                // larger than 1.
                int jj = (code == 1 ? j : j-1);
                Poly poly;
                poly.points = new Point[jj+2-i];
                poly.points[0] = points[jj];
                for (int k = i; k <= jj; k++)
                    poly.points[k-i+1] = points[k];
                poly.numpts = jj + 2 - i;

                // Need to enable "fixing" below, since needles due to
                // temporary endpoints can cause reentrant test to fail.
                int nret = poly.check_poly(0, false);

                delete [] poly.points;
                if (nret & PCHK_REENT) {
                    ret |= PCHK_REENT;
                    return (ret);
                }
            }
        }
    }
    return (ret);
}


namespace {
    // Return 1 if p3 is in the "positive" side of p1,p2 in a
    // right-handed coordinate system, -1 if in the "negative" side,
    // or 0 if p3 is on the line p1,p2
    //
    inline int
    dsgn(Point *p1, Point *p2, Point *p3)
    {
        double a = (p2->x - p1->x)*(double)(p3->y - p1->y) -
            (p3->x - p1->x)*(double)(p2->y - p1->y);
        if (a > 0.0)
            return (1);
        if (a < 0.0)
            return (-1);
        return (0);
    }
}


// Private function.
// Given two segments i-1,i and j-1,j such that j is on [i-1,i) if code == 1,
// or j-1 is on [i-1,i) if code == -1, return true if the j branch extends
// between the interior to a non-interior point.
//
bool
Poly::vertex_cross_test(int code, int i, int j, Otype wind)
{
    Point *p1, *p2, *p3, *p4, *p5;
    if (code == 1) {
        // j is on [i-1,i)
        if (points[j].x == points[i-1].x && points[j].y == points[i-1].y)
            p1 = (i == 1 ? &points[numpts-2] : &points[i-2]);
        else
            p1 = &points[i-1];
        p2 = &points[j];
        p3 = &points[i];
        p4 = &points[j-1];
        p5 = (j == numpts-1 ? &points[1] : &points[j+1]);
    }
    else if (code == -1) {
        // j-1 is on [i-1,i)
        if (points[j-1].x == points[i-1].x && points[j-1].y == points[i-1].y)
            p1 = (i == 1 ? &points[numpts-2] : &points[i-2]);
        else
            p1 = &points[i-1];
        p2 = &points[j-1];
        p3 = &points[i];
        p4 = &points[j-2];
        p5 = &points[j];
    }
    else
        return (false);

    int s41, s43, s51, s53;
    if (wind == Ocw) {
        s41 = dsgn(p2, p1, p4);
        s43 = -dsgn(p2, p3, p4);
        s51 = dsgn(p2, p1, p5);
        s53 = -dsgn(p2, p3, p5);
    }
    else if (wind == Occw) {
        s41 = -dsgn(p2, p1, p4);
        s43 = dsgn(p2, p3, p4);
        s51 = -dsgn(p2, p1, p5);
        s53 = dsgn(p2, p3, p5);
    }
    else
        return (false);

    if (s41 > 0 && s43 > 0) {
        if (s51 > 0 && s53 > 0)
            return (false);
        else
            return (true);
    }
    else if (s51 > 0 && s53 > 0)
        return (true);
    return (false);
}


// Remove vertices so that spacing is larger than one.  Do this in a
// way that preserves Manhattanness.  Return false if the poly is no
// longer valid after vertex removal.
//
bool
Poly::fix_tiny_jogs()
{
    int npts = numpts - 1;
    for (int i = 0; i < npts; i++) {
        int dx1 = points[i+1].x - points[i].x;
        int dy1 = points[i+1].y - points[i].y;
        if (abs(dx1) <= 1 && abs(dy1) <= 1) {

            // If the delta is one count in x or y, fix up the
            // points to preserve Manhattan-ness.
            if (dy1 == 0) {
                int x1 = points[i].x;
                int x2 = points[i+1].x;
                int xold = (x2 & 1) ? x2 : x1;
                int xnew = (x2 & 1) ? x1 : x2;
                for (int j = 0; j < numpts; j++) {
                    if (points[j].x == xold)
                        points[j].x = xnew;
                }
                if (!remove_vertex(i))
                    return (false);
                i = -1;
                npts--;
                continue;
            }
            if (dy1 == 0) {
                int y1 = points[i].y;
                int y2 = points[i+1].y;
                int yold = (y2 & 1) ? y2 : y1;
                int ynew = (y2 & 1) ? y1 : y2;
                for (int j = 0; j < numpts; j++) {
                    if (points[j].y == yold)
                        points[j].y = ynew;
                }
                if (!remove_vertex(i))
                    return (true);
                i = -1;
                npts--;
                continue;
            }

            if (!remove_vertex(i))
                return (false);
            i--;
            npts--;
            continue;
        }
    }
    return (true);
}


// Return the polygon's area in SQUARE MICRONS.  This assumes that the
// poly is not self-intersecting.
//
double
Poly::area() const
{
    double a = 0.0;
    Point p0(points[0]);
    double x0 = (points[1].x - p0.x);
    double y0 = (points[1].y - p0.y);
    const int n = numpts - 2;
    for (int i = 1; i < n; i++) {
        double x1 = (points[i+1].x - p0.x);
        double y1 = (points[i+1].y - p0.y);
        a += (x0*y1 - x1*y0);
        x0 = x1;
        y0 = y1;
    }
    return (0.5*fabs(a)/(CDphysResolution*CDphysResolution));
}


// Return the perimeter length in INTERNAL UNITS.
//
int
Poly::perim() const
{
    double ptot = 0.0;
    for (int i = 1; i < numpts; i++)
        ptot += Point::distance(&points[i-1], &points[i]);
    return (mmRnd(ptot));
}


void
Poly::centroid(double *pcx, double *pcy) const
{
    // From wikipedia:
    // Cx = (1/6A) sum{i = 0 to n-1} (x[i] + x[i+1])(x[i]y[i+1] - x[i+1]y[i])
    // Cy = (1/6A) sum{i = 0 to n-1} (y[i] + y[i+1])(x[i]y[i+1] - x[i+1]y[i])
    // A = (1/2) sum{i=0 to n-1} (x[i]y[i+1] - x[i+1]y[i])

    if (!pcx || !pcy)
        return;
    double a = 0.0, cx = 0.0, cy = 0.0;
    Point p0(points[0]);
    double x0 = (points[1].x - p0.x);
    double y0 = (points[1].y - p0.y);
    const int n = numpts - 2;
    for (int i = 1; i < n; i++) {
        double x1 = (points[i+1].x - p0.x);
        double y1 = (points[i+1].y - p0.y);
        double t = (x0*y1 - x1*y0);
        a += t;
        cx += (x0 + x1)*t;
        cy += (y0 + y1)*t;
        x0 = x1;
        y0 = y1;
    }
    a *= 3.0;
    cx /= a;
    cy /= a;
    *pcx = (cx + p0.x)/CDphysResolution;
    *pcy = (cy + p0.y)/CDphysResolution;
}


namespace {
    inline int
    isct(int y, Point *pts)
    {
        if (pts->x == (pts-1)->x)
            return (pts->x);
        return ((int)((y - (pts-1)->y)*
            ((double)(pts->x - (pts-1)->x)/
            (pts->y - (pts-1)->y))) + (pts-1)->x);
    }
}


// Return true if the point is in the polygon.  If touchok is true, the
// points on the boundary are considered as inside the polygon.
//
bool
Poly::intersect(const Point *px, bool touchok) const
{
    int i;
    for (i = numpts - 1; i >= 0; i--) {
        if (points[i].y != px->y)
            break;
    }
    if (i < 0)
        // strange case
        return (false);
    bool below = (points[i].y < px->y);

    Point *pts = points;
    int num = numpts;
    int count = 0;
    if (!touchok) {
        // count the crossings to the left of px
        for (pts++, num--; num; pts++, num--) {
            if ((below && pts->y > px->y) || (!below && pts->y < px->y)) {
                if (isct(px->y, pts) < px->x) {
                    if (below)
                        count++;
                    else
                        count--;
                }
                below = !below;
            }
        }
    }
    else {
        // count the crossings to the left of px, and look for boundary
        // intersections
        //
        for (pts++, num--; num; pts++, num--) {
            if ((below && pts->y > px->y) || (!below && pts->y < px->y)) {
                int x = isct(px->y, pts);
                if (x < px->x) {
                    if (below)
                        count++;
                    else
                        count--;
                }
                if (x == px->x)
                    return (true);
                below = !below;
            }
            if (pts->y == px->y) {
                if ((pts-1)->y != pts->y) {
                    if (px->x == pts->x)
                        return (true);
                }
                else {
                    if (((pts-1)->x <= px->x && px->x <= pts->x) ||
                            (pts->x <= px->x && px->x <= (pts-1)->x))
                        return (true);
                }
            }
        }
    }
    return (count != 0);
}


// Return true if the polygon and box touch or overlap.  If touchok is true,
// the boundaries are considered internal points.
//
bool
Poly::intersect(const BBox *BB, bool touchok) const
{
    if (BB->left != BB->right && BB->bottom != BB->top) {
        if (cGEO::path_box_intersect(points, numpts, BB, touchok))
            return (true);
        // catch poly entirely in box
        if (BB->intersect(points, touchok))
            return (true);
    }
    // catch box entirely in poly
    Point_c px((BB->left + BB->right)/2, (BB->bottom + BB->top)/2);
    if (intersect(&px, touchok))
        return (true);
    return (false);
}


// Return true if the two polygons touch or overlap.  If touchok is true,
// the boundaries are considered internal points.
//
bool
Poly::intersect(const Poly *poly2, bool touchok) const
{
    if (cGEO::path_path_intersect(points, numpts, poly2->points,
            poly2->numpts, touchok))
        return (true);
    if (intersect(poly2->points, touchok))
        return (true);
    if (poly2->intersect(points, touchok))
        return (true);
    return (false);
}


bool
Poly::intersect(const Wire *w, bool touchok) const
{
    return (w->intersect(this, touchok));
}


// Divide the polygon into two or more pieces to reduce the vertex
// count.  This function is recursive, and generally should be called
// at the top level if numpts > maxverts.
//
PolyList *
Poly::divide(int maxverts) const
{
    BBox pBB;
    computeBB(&pBB);
    int *vals = new int[numpts];
    PolyList *p0, *p1;
    if (pBB.width() > pBB.height()) {
        int npm1 = numpts - 1;
        for (int i = 0; i < npm1; i++)
            vals[i] = points[i].x;
        std::sort(vals, vals + npm1);
        int med_x = vals[npm1/2];
        Zlist *z0 = toZlist();
        Zlist *zl = 0, *zr = 0;
        while (z0) {
            Zlist *z = z0;
            z0 = z0->next;
            int xl = z->Z.minleft();
            int xr = z->Z.maxright();
            if (xr <= med_x) {
                z->next = zl;
                zl = z;
            }
            else if (xl >= med_x) {
                z->next = zr;
                zr = z;
            }
            else {
                Zoid zref;
                zref.xll = zref.xul = xl;
                zref.xlr = zref.xur = med_x;
                zref.yl = z->Z.yl;
                zref.yu = z->Z.yu;
                Zlist *zx = z->Z.clip_to(&zref);
                if (zx->next)
                    zx->next->next = zl;
                else
                    zx->next = zl;
                zl = zx;

                zref.xll = zref.xul = med_x;
                zref.xlr = zref.xur = xr;
                zx = z->Z.clip_to(&zref);
                if (zx->next)
                    zx->next->next = zr;
                else
                    zx->next = zr;
                zr = zx;
                delete z;
            }
        }
        p0 = Zlist::to_poly_list(zl);
        p1 = Zlist::to_poly_list(zr);
    }
    else {
        int npm1 = numpts - 1;
        for (int i = 0; i < npm1; i++)
            vals[i] = points[i].y;
        std::sort(vals, vals + npm1);
        int med_y = vals[npm1/2];
        Zlist *z0 = toZlist();
        Zlist *zb = 0, *zt = 0;
        while (z0) {
            Zlist *z = z0;
            z0 = z0->next;
            if (z->Z.yu <= med_y) {
                z->next = zb;
                zb = z;
            }
            else if (z->Z.yl >= med_y) {
                z->next = zt;
                zt = z;
            }
            else {
                Zoid zref;
                zref.xll = zref.xul = z->Z.minleft();
                zref.xlr = zref.xur = z->Z.maxright();
                zref.yu = z->Z.yu;
                zref.yl = med_y;
                Zlist *zx = z->Z.clip_to(&zref);
                zx->next = zt;
                zt = zx;

                zref.yu = med_y;
                zref.yl = z->Z.yl;
                zx = z->Z.clip_to(&zref);
                zx->next = zb;
                zb = zx;
                delete z;
            }
        }
        p0 = Zlist::to_poly_list(zb);
        p1 = Zlist::to_poly_list(zt);
    }
    if (!p0)
        p0 = p1;
    else {
        PolyList *pn = p0;
        while (pn->next)
            pn = pn->next;
        pn->next = p1;
    }
    PolyList *px0 = 0;
    while (p0) {
        PolyList *p = p0;
        p0 = p0->next;
        if (p->po.numpts <= maxverts) {
            p->next = px0;
            px0 = p;
        }
        else {
            PolyList *pn = p->po.divide(maxverts);
            PolyList *px = pn;
            while (px->next)
                px = px->next;
            px->next = px0;
            px0 = pn;
            delete p;
        }
    }
    return (px0);
}


//
// Polygon clipping.
//

//#define PO_DEBUG

#ifdef PO_DEBUG
#define inline
#endif

namespace {
    struct PolyClipper
    {
        PolyClipper(const Point*, int, const BBox*);
        ~PolyClipper() { delete [] pc_pointbuf; }

        PolyList *clip()
        {
            if (pc_numused < 0)
                return (0);
            PolyList *plst = polysplit(pc_pointbuf, pc_numused, 0);
            pc_pointbuf = 0;
            return (plst);
        }

    private:
        inline int onbb(Point*);
        bool clip_crude(Point&, Point&);
        int filter_vertices(Point*, int);
        int findptrs(int, Point*, char*, int, Point**, Point**, Point**);
        PolyList *polysplit(Point*, int, PolyList*);
        bool check_candidate(Point*, int, int, int, int);

        Point *pc_pointbuf;
        int pc_bufsize;
        int pc_numused;
        BBox pc_clipBB;
    };
}


// The global polygon clipping function.  This returns a list of polygons
// clipped to the BB.  If the enclosed pointer is passed, and clipping is
// not needed, the pointer is set and no clipping is done.
//
PolyList *
Poly::clip(const BBox *BB, bool *enclosed) const
{
    // Is clipping really needed?
    bool needs_clip = false;
    int n = numpts - 1;
    while (n-- > 0) {
        if (!BB->intersect(points + n, true)) {
            // found a vertex outside the box
            needs_clip = true;
            break;
        }
    }
    if (!needs_clip) {
        if (enclosed) {
            *enclosed = true;
            return (0);
        }
        Point *pts = new Point[numpts];
        memcpy(pts, points, numpts*sizeof(Point));
        Poly po(numpts, pts);
        return (new PolyList(po, 0));
    }
    if (enclosed)
        *enclosed = false;

    PolyClipper pc(points, numpts, BB);
    return (pc.clip());
}


namespace {
    PolyClipper::PolyClipper(const Point *points, int numpts, const BBox *BB)
    {
#ifdef PO_DEBUG
     printf("BB %d,%d %d,%d\n", BB->left, BB->bottom, BB->right, BB->top);
     for (int t = 0; t < numpts; t++)
      printf("   %d,%d\n", points[t].x, points[t].y);
     printf("\n");
#endif
        // First copy and clip the points.  The clipping may add vertices.

        pc_clipBB = *BB;
        pc_bufsize = numpts*2;
        pc_pointbuf = new Point[pc_bufsize];
        int j = 0;
        Point p1 = points[numpts-2];
        Point p2 = points[0];
        clip_crude(p1, p2);
        pc_pointbuf[0] = p2;
        for (int i = 0; i < numpts-1; i++) {
            p1 = points[i];
            p2 = points[i+1];
            clip_crude(p1, p2);
            if (pc_pointbuf[j].x != p1.x || pc_pointbuf[j].y != p1.y)
                j++;
            pc_pointbuf[j++] = p1;
            pc_pointbuf[j] = p2;
        }
        pc_numused = j+1;

        // The polygon is now clipped to the BB, but in general contains a
        // lot of extra vertices and zero-width segments.  The rest of
        // this function removes these.

        pc_numused = filter_vertices(pc_pointbuf, pc_numused);

        if (pc_numused <= 3) {
            delete [] pc_pointbuf;
            pc_pointbuf = 0;
            pc_bufsize = 0;
            pc_numused = -1;
            return;
        }

        // Reallocate points array to actual size.

        if (pc_numused < pc_bufsize) {
            Point *tmp = new Point[pc_numused];
            for (int i = 0; i < pc_numused; i++)
                tmp[i] = pc_pointbuf[i];
            delete [] pc_pointbuf;
            pc_pointbuf = tmp;
            pc_bufsize = pc_numused;
        }

        // The polygon no longer has zero-width appendages, but may have
        // multiple areas connected by zero-width sections.  A call to
        // polysplit will split the polygon into pieces, removing the
        // zero-width sections.  The resulting list of indivisible
        // polygons will be returned.
    }


#define C_LEFT   1
#define C_BOTTOM 2
#define C_RIGHT  4
#define C_TOP    8

    // Return a code indicating which side(s) of the clipping BB the point
    // lies outside of or on.
    //
    inline int
    PolyClipper::onbb(Point *p)
    {
        int code = 0;
        if (p->x <= pc_clipBB.left)
            code |= C_LEFT;
        else if (p->x >= pc_clipBB.right)
            code |= C_RIGHT;
        if (p->y <= pc_clipBB.bottom)
            code |= C_BOTTOM;
        else if (p->y >= pc_clipBB.top)
            code |= C_TOP;
        return (code);
    }


    // Clip the segments and the vertices of the polygon to the
    // clipping box.  This creates a polygon which lies within the
    // clipping box, but has in general many extra vertices and
    // zero-width sections.
    //
    bool
    PolyClipper::clip_crude(Point &p1, Point &p2)
    {
        BBox *BB = &pc_clipBB;
        if (cGEO::line_clip(&p1.x, &p1.y, &p2.x, &p2.y, BB)) {
            Point po1 = p1;
            Point po2 = p2;
            if (p1.x < BB->left)
                p1.x = BB->left;
            else if (p1.x > BB->right)
                p1.x = BB->right;
            if (p1.y < BB->bottom)
                p1.y = BB->bottom;
            else if (p1.y > BB->top)
                p1.y = BB->top;
            if (p2.x < BB->left)
                p2.x = BB->left;
            else if (p2.x > BB->right)
                p2.x = BB->right;
            if (p2.y < BB->bottom)
                p2.y = BB->bottom;
            else if (p2.y > BB->top)
                p2.y = BB->top;
            int c1 = onbb(&p1);
            int c2 = onbb(&p2);
            if (!(c1 & c2)) {
                double x1 = po2.x - po1.x;
                double y1 = po2.y - po1.y;
                double x2 = p1.x - po1.x;
                double y2 = p1.y - po1.y;
                if (x1*y2 > x2*y1) {
                    // clip box is to left along po1-po2, ccw order
                    if (c2 & C_LEFT) {
                        if (c2 & C_TOP)
                            p2.x = BB->right;
                        else
                            p2.y = BB->top;
                    }
                    else if (c2 & C_BOTTOM) {
                        if (c2 & C_LEFT)
                            p2.y = BB->top;
                        else
                            p2.x = BB->left;
                    }
                    else if (c2 & C_RIGHT) {
                        if (c2 & C_BOTTOM)
                            p2.x = BB->left;
                        else
                            p2.y = BB->bottom;
                    }
                    else {
                        if (c2 & C_RIGHT)
                            p2.y = BB->bottom;
                        else
                            p2.x = BB->right;
                    }
                }
                else {
                    // clip box is to right along po1-po2, cw order
                    if (c2 & C_LEFT) {
                        if (c2 & C_BOTTOM)
                            p2.x = BB->right;
                        else
                            p2.y = BB->bottom;
                    }
                    else if (c2 & C_BOTTOM) {
                        if (c2 & C_RIGHT)
                            p2.y = BB->top;
                        else
                            p2.x = BB->right;
                    }
                    else if (c2 & C_RIGHT) {
                        if (c2 & C_TOP)
                            p2.x = BB->left;
                        else
                            p2.y = BB->top;
                    }
                    else {
                        if (c2 & C_LEFT)
                            p2.y = BB->bottom;
                        else
                            p2.x = BB->left;
                    }
                }
            }
            return (true);
        }
        return (false);
    }


    // Return true if the three vertices are Manhattan and colinear, so pt
    // can be removed.  Also return true if pt and pf are equal.
    //
    inline bool check_vert(Point *pf, Point *pt, Point *pl)
    {
        if (pt->x == pf->x) {
            if (pt->y == pf->y)
                return (true);
            if (pt->x == pl->x)
                return (true);
        }
        else if (pt->y == pf->y) {
            if (pt->y == pl->y)
                return (true);
        }
        return (false);
    }


    // Remove, in place, colinear and duplicate vertices.  If the polygon
    // is no longer viable, return -1, otherwise return the new array size.
    //
    int
    PolyClipper::filter_vertices(Point *pts, int num)
    {
        // Remove all duplicate and colinear vertices
        char *flags = new char[num];
        memset(flags, 0, num);
        for (int i = 0; i < num-1; i++) {
            if (flags[i])
                continue;
            Point *pf, *pt, *pl;
            int im = findptrs(i, pts, flags, num, &pf, &pt, &pl);
            if (im < 0) {
                delete [] flags;
                return (-1);
            }
            while (check_vert(pf, pt, pl)) {
#ifdef PO_DEBUG
     printf("* %d  %d,%d %d,%d %d,%d\n", i, pf->x, pf->y, pt->x, pt->y,
      pl->x, pl->y);
#endif
                flags[im] = 1;
                im = findptrs(im-1, pts, flags, num, &pf, &pt, &pl);
                if (im < 0) {
                    delete [] flags;
                    return (-1);
                }
            }
        }
#ifdef PO_DEBUG
     printf("Marked vertices:\n");
     for (int i = 0; i < num; i++)
      printf("%d  %d,%d\n", flags[i], pts[i].x, pts[i].y);
     printf("\n");
#endif
        int j = 0;
        for (int i = 0; i < num-1; i++) {
            if (!flags[i])
                pts[j++] = pts[i];
        }
        pts[j++] = pts[0];
        num = j;
#ifdef PO_DEBUG
     printf("After vertex removal:\n");
     for (int i = 0; i < num; i++)
      printf("%d  %d,%d\n", i, pts[i].x, pts[i].y);
     printf("\n");
#endif
        delete [] flags;
        return (num);
    }


    // This returns the three pointers, for three consecutive active
    // vertices, taking account of cyclicity.
    //
    int
    PolyClipper::findptrs(int ix, Point *p, char *flags, int num, Point **pf,
        Point **pt, Point **pl)
    {
        // find the "test" (midpoint) vertex, backing up until we find
        // an active one
        int i;
        if (ix < 0)
            ix = num-2;
        for (i = ix; i >= 0; i--)
            if (!flags[i])
                break;
        if (i < 0) {
            for (i = num-2; i > ix; i--)
                if (!flags[i])
                    break;
            if (i == ix)
                return (-1);
        }
        int im = i;
        *pt = p+i;
#ifdef PO_DEBUG
        printf("  %d", i);
#endif

        // find the "first" vertex, the one logically before the test vertex
        int ii = i-1;
        if (ii < 0)
            ii = num-2;
        for (i = ii; i >= 0; i--)
            if (!flags[i])
                break;
        if (i < 0) {
            for (i = num-2; i > ii; i--)
                if (!flags[i])
                    break;
            if (i == ii)
                return (-1);
        }
        *pf = p+i;
#ifdef PO_DEBUG
        printf("  %d", i);
#endif

        // find the "last" vertex, the one that logically follows the test
        // vertex
        ii = ix+1;
        if (ii > num-2)
            ii = 0;
        for (i = ii; i < num-1; i++)
            if (!flags[i])
                break;
        if (i == num-1) {
            for (i = 0; i < ii; i++)
                if (!flags[i])
                    break;
            if (i == ii)
                return (-1);
        }
        *pl = p+i;
#ifdef PO_DEBUG
        printf("  %d\n", i);
#endif
        return (im);
    }


    // Return true if the two segments, known to be from the same side
    // of the clipping BB, overlap with nonzero length.
    //
    inline bool check_inline(Point *pf, Point *pl, Point *p1, Point *p2)
    {
        if (pf->x == pl->x) {
            if (p1->y >= pf->y && p1->y >= pl->y &&
                    p2->y >= pf->y && p2->y >= pl->y)
                return (false);
            if (p1->y <= pf->y && p1->y <= pl->y &&
                    p2->y <= pf->y && p2->y <= pl->y)
                return (false);
        }
        else if (pf->y == pl->y) {
            if (p1->x >= pf->x && p1->x >= pl->x &&
                    p2->x >= pf->x && p2->x >= pl->x)
                return (false);
            if (p1->x <= pf->x && p1->x <= pl->x &&
                    p2->x <= pf->x && p2->x <= pl->x)
                return (false);
        }
        return (true);
    }


    // Look in the polygon for the situation where there is overlapping
    // segments on one of the clipping BB edges.  Divide the poly in two
    // at that point, and repeat the test for each piece.  The return is
    // a list of indivisible polygons.  The original polygon points array
    // is freed (or used).
    //
    PolyList *
    PolyClipper::polysplit(Point *p, int num, PolyList *P)
    {
        if (num >=  7)  {
            for (int i = 0; i < num-1; i++) {
                int code = onbb(p+i) & onbb(p+i+1);
                if (!code)
                    continue;
                // Find the next segment on the same side of the clipping BB
                // that overlaps i,i+1.
                int jsave = -1;
                int numc = num - 6;
                for (int c = 0; c < numc; c++) {
                    int is = (i+3+c) % (num-1);
                    int ie = (i+4+c) % (num-1);
                    if ((onbb(p+is) & onbb(p+ie) & code) &&
                            check_inline(p+i, p+i+1, p+is, p+ie)) {
                        jsave = is;
                        break;
                    }
                }
                if (jsave >= 0 && check_candidate(p, num, i, jsave, code)) {
#ifdef PO_DEBUG
     printf("joining %d,%d-%d,%d (%d) and %d,%d-%d,%d (%d)\n",
      p[i].x,p[i].y,p[i+1].x,p[i+1].y,i,
      p[jsave].x,p[jsave].y,p[jsave+1].x,p[jsave+1].y,jsave);
#endif
                    int isave = i;
                    if (jsave < isave)
                        { int t = jsave; jsave = isave; isave = t; }

                    int nnum = jsave-isave+1;
                    Point *npts = new Point[nnum];
                    int k;
                    for (k = 0; k < nnum; k++)
                        npts[k] = p[isave+k];
                    npts[0] = npts[k-1];
                    int lstnum = nnum;
                    nnum = filter_vertices(npts, nnum);
                    if (nnum > 3) {
#ifdef PO_DEBUG
     for (int t = 0; t < nnum; t++)
      printf("   %d,%d\n", npts[t].x, npts[t].y);
     printf("\n");
#endif
                        P = polysplit(npts, nnum, P);
                    }
                    else
                        delete [] npts;
                    nnum = num - lstnum + 1;
                    npts = new Point[nnum];
                    for (k = 0; k <= isave; k++)
                        npts[k] = p[k];
                    for (k = isave+lstnum; k < num; k++)
                        npts[k-lstnum+1] = p[k];
                    nnum = filter_vertices(npts, nnum);
                    if (nnum > 3) {
#ifdef PO_DEBUG
     for (int t = 0; t < nnum; t++)
      printf("   %d,%d\n", npts[t].x, npts[t].y);
     printf("\n");
#endif
                        P = polysplit(npts, nnum, P);
                    }
                    else
                        delete [] npts;
                    delete [] p;
                    return (P);
                }
            }
        }
        Poly po;
        po.numpts = num;
        po.points = p;
        return (new PolyList(po, P));
    }


    // Return true if the two segments, known to be colinear, are
    // oriented in opposing directions.
    //
    inline bool check_orient(Point *pf, Point *pl, Point *p1, Point *p2)
    {
        if (pf->x == pl->x) {
            if (pf->y > pl->y && p1->y > p2->y)
                return (false);
            if (pf->y < pl->y && p1->y < p2->y)
                return (false);
        }
        else if (pf->y == pl->y) {
            if (pf->x > pl->x && p1->x > p2->x)
                return (false);
            if (pf->x < pl->x && p1->x < p2->x)
                return (false);
        }
        return (true);
    }


    // Return true if emin,emax is enclosed in pf,pl.
    //
    inline bool check_enclosing(int emin, int emax, Point *pf, Point *pl)
    {
        if (pf->x == pl->x) {
            if (pf->y >= emax && pl->y <= emin)
                return (true);
            if (pl->y >= emax && pf->y <= emin)
                return (true);
        }
        else if (pf->y == pl->y) {
            if (pf->x >= emax && pl->x <= emin)
                return (true);
            if (pl->x >= emax && pf->x <= emin)
                return (true);
        }
        return (false);
    }


    // Set emin, emax to the max and min values of the overlap area of
    // the two segments.  The two segments are known to be colinear
    // and to overlap.
    //
    inline void set_interval(Point *pf, Point *pl, Point *p1, Point *p2,
        int *emin, int *emax)
    {
        if (pf->x == pl->x) {
            int mx1 = mmMax(pf->y, pl->y);
            int mn1 = mmMin(pf->y, pl->y);
            int mx2 = mmMax(p1->y, p2->y);
            int mn2 = mmMin(p1->y, p2->y);
            *emax = mmMin(mx1, mx2);
            *emin = mmMax(mn1, mn2);
        }
        else if (pf->y == pl->y) {
            int mx1 = mmMax(pf->x, pl->x);
            int mn1 = mmMin(pf->x, pl->x);
            int mx2 = mmMax(p1->x, p2->x);
            int mn2 = mmMin(p1->x, p2->x);
            *emax = mmMin(mx1, mx2);
            *emin = mmMax(mn1, mn2);
        }
    }


    // Return true if it is ok to split the poly p at i, j.
    //
    bool
    PolyClipper::check_candidate(Point *p, int num, int i, int j, int code)
    {
        if (!check_orient(p+i, p+i+1, p+j, p+j+1))
            // Orientation not opposing
            return (false);
        int emin = 0, emax = 0;
        set_interval(p+i, p+i+1, p+j, p+j+1, &emin, &emax);
        if (emin >= emax)
            // "impossible"
            return (false);
        // The interval emin,emax is the intersection of the two sides
        // we propose to join.  Look of other segments along this side
        // which cover emin,emax.  If we find any, return false, since
        // we can't join here (in general).
        for (int k = 0; k < num-1; k++) {
            if (k == i || k == j)
                continue;
            if ((onbb(p+k) & onbb(p+k+1) & code) &&
                    check_enclosing(emin, emax, p+k, p+k+1))
                return (false);
        }
        return (true);
    }
    // End of PolyClipper functions.
}


// Add vertices to the polygon if necessary to make sure that no angle
// is acute.  If the poly needs fixing, return a pointer to a new
// repaired polygon.  Argument rad is a "min dimension" used to set
// the distance to new points added.
//
Poly *
Poly::clip_acute(int rad) const
{
    if (rad <= 0)
        rad = CDphysResolution;

    Point *npts = new Point[2*numpts];
    int nnpts = 0;

    bool changed = false;
    const double fudge = 1e-3;
    for (int i = 0; i < numpts-1; i++) {
        Point *p0 = points + i;
        Point *p1 = (i == 0 ? points + numpts - 2 : points + i - 1);
        Point *p2 = points + i + 1;

        int dx1 = p0->x - p1->x;
        int dy1 = p0->y - p1->y;
        double r1 = sqrt(dx1*(double)dx1 + dy1*(double)dy1);
        double c1 = dx1/r1;
        double s1 = dy1/r1;

        int dx2 = p0->x - p2->x;
        int dy2 = p0->y - p2->y;
        double r2 = sqrt(dx2*(double)dx2 + dy2*(double)dy2);

        double a = atan2(-dx2*s1 + dy2*c1, dx2*c1 + dy2*s1);
        if (a < 0)
            a = -a;

        if (a < fudge) {
            // "zero" angle
            *p0 = *p1;
            changed = true;
            continue;
        }
        if (a < M_PI_2 - fudge) {
            // acute angle
            double r = rad;
            if (r1 < r)
                r = r1;
            if (r2 < r)
                r = r2;
            npts[nnpts].set(mmRnd(p0->x - dx1*r/r1), mmRnd(p0->y - dy1*r/r1));
            nnpts++;
            npts[nnpts].set(mmRnd(p0->x - dx2*r/r2), mmRnd(p0->y - dy2*r/r2));
            nnpts++;
            changed = true;
            continue;
        }
        npts[nnpts++] = points[i];
    }
    npts[nnpts++] = npts[0];

    if (changed) {
        Poly *p = new Poly(nnpts, new Point[nnpts]);
        memcpy(p->points, npts, nnpts*sizeof(Point));
        delete [] npts;
        return (p);
    }
    delete [] npts;
    return (0);
}


// Return an edge list, with the self-overlapping parts removed.  This
// will eliminate parts of needles and internal edges (edges with
// polygon fill on both sides).
//
edg_t *
Poly::edges() const
{
    if (numpts >= 6) {
        linedb_t ldb;
        ldb.add(this);
        return (ldb.edges());
    }

    edg_t *edgs = 0;
    for (int i = 1; i < numpts; i++) {
        if (points[i-1] != points[i])
            edgs = new edg_t(points[i-1].x, points[i-1].y,
                points[i].x, points[i].y, edgs);
    }
    return (edgs);
}


namespace {
    // Return p12 side and p2 corner zoids, for Poly::halo.
    //
    Zlist *
    halo_zoids(Point *p1, Point *p2, Point *p3, int d)
    {
        Zlist *zl;
        if (p1->x == p2->x) {
            if (p2->y > p1->y)
                zl = new Zlist(p1->x - d, p1->y, p1->x, p2->y);
            else
                zl = new Zlist(p2->x, p2->y, p2->x + d, p1->y);

            if ((p3->x - p1->x)*(double)(p2->y - p1->y) > 0.0) {
                double y21 = p2->y < p1->y ? -1.0 : 1.0;

                Point p[5];
                Poly po(5, p);
                p[0] = *p2;
                p[1].set(mmRnd(-d*y21 + p2->x), p2->y);
                if (p2->y == p3->y) {
                    double x32 = p3->x < p2->x ? -1.0 : 1.0;
                    p[2].set(mmRnd(-d*M_SQRT1_2*y21 + p2->x),
                        mmRnd(d*M_SQRT1_2*x32 + p2->y));
                    p[3].set(p2->x, mmRnd(d*x32 + p2->y));
                }
                else {
                    double p32x = p3->x - p2->x;
                    double p32y = p3->y - p2->y;
                    double p32 = sqrt(p32x*p32x + p32y*p32y);
                    double x32 = p32x/p32;
                    double y32 = p32y/p32;

                    double aa = sqrt(0.5/(1.0 + y21*y32));
                    p[2].set(mmRnd(-d*aa*(y21 + y32) + p2->x),
                        mmRnd(d*aa*x32 + p2->y));
                    p[3].set(mmRnd(-d*y32 + p2->x), mmRnd(d*x32 + p2->y));
                }
                p[4] = p[0];
                zl->next = po.toZlist();
            }
        }
        else if (p1->y == p2->y) {
            if (p2->x > p1->x)
                zl = new Zlist(p1->x, p1->y, p2->x, p1->y + d);
            else
                zl = new Zlist(p2->x, p2->y - d, p1->x, p2->y);

            if ((p2->x - p1->x)*(double)(p3->y - p1->y) < 0.0) {
                double x21 = p2->x < p1->x ? -1.0 : 1.0;

                Point p[5];
                Poly po(5, p);
                p[0] = *p2;
                p[1].set(p2->x, mmRnd(d*x21 + p2->y));
                if (p2->x == p3->x) {
                    double y32 = p3->y < p2->y ? -1.0 : 1.0;
                    p[2].set(mmRnd(-d*M_SQRT1_2*y32 + p2->x),
                        mmRnd(d*M_SQRT1_2*x21 + p2->y));
                    p[3].set(mmRnd(-d*y32 + p2->x), p2->y);
                }
                else {
                    double p32x = p3->x - p2->x;
                    double p32y = p3->y - p2->y;
                    double p32 = sqrt(p32x*p32x + p32y*p32y);
                    double x32 = p32x/p32;
                    double y32 = p32y/p32;

                    double aa = sqrt(0.5/(1.0 + x21*x32));
                    p[2].set(mmRnd(-d*aa*y32 + p2->x),
                        mmRnd(d*aa*(x21 + x32) + p2->y));
                    p[3].set(mmRnd(-d*y32 + p2->x), mmRnd(d*x32 + p2->y));
                }
                p[4] = p[0];
                zl->next = po.toZlist();
            }
        }
        else {
            double p21x = p2->x - p1->x;
            double p21y = p2->y - p1->y;
            double p21 = sqrt(p21x*p21x + p21y*p21y);
            double x21 = p21x/p21;
            double y21 = p21y/p21;

            Point p[5];
            Poly po(5, p);

            p[0].set(mmRnd(-d*y21 + p1->x), mmRnd(d*x21 + p1->y));
            p[1].set(mmRnd(-d*y21 + p2->x), mmRnd(d*x21 + p2->y));
            p[2] = *p2;
            p[3] = *p1;
            p[4] = p[0];
            zl = po.toZlist();

            double a = p21x*(p3->y - p1->y) - (p3->x - p1->x)*p21y;
            if (a < 0) {

                double p32x = p3->x - p2->x;
                double p32y = p3->y - p2->y;
                double p32 = sqrt(p32x*p32x + p32y*p32y);
                double x32 = p32x/p32;
                double y32 = p32y/p32;

                p[0] = *p2;
                p[1].set(mmRnd(-d*y21 + p2->x), mmRnd(d*x21 + p2->y));

                double aa = sqrt(0.5/(1.0 + y21*y32 + x21*x32));
                p[2].set(mmRnd(-d*aa*(y21 + y32) + p2->x),
                    mmRnd(d*aa*(x21 + x32) + p2->y));

                p[3].set(mmRnd(-d*y32 + p2->x), mmRnd(d*x32 + p2->y));
                p[4] = p[0];

                Zlist *zx = zl;
                while (zx->next)
                    zx = zx->next;
                zx->next = po.toZlist();
            }
        }
        return (zl);
    }
}


// Return a list of trapezoids that form a halo around the polygon.
// THE WINDING MUST BE CLOCKWISE for positive d to create a halo
// outside of the polygon.
//
Zlist *
Poly::halo(int d) const
{
    Zlist *z0 = 0, *ze = 0;
    for (int i = 0; i < numpts-1; i++) {
        Zlist *zl = halo_zoids(i > 0 ? &points[i-1] : &points[numpts-2],
            &points[i], &points[i+1], d);
        if (!z0)
            z0 = ze = zl;
        else {
            while (ze->next)
                ze = ze->next;
            ze->next = zl;
        }
    }
    return (z0);
}


// Return a list of the external edges, as trapezoids bloated by
// delta.  The external edges are adjacent to empty space.
//
Zlist *
Poly::ext_zoids(int delta, int mode) const
{
    linedb_t ldb;
    ldb.add(this);
    return (ldb.zoids(delta, mode));
}

