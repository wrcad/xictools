
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
#include "geo_line.h"


// Static function.
// Return true if the two line segments intersect, and set pret to the
// intersecting point if passed non-null.
//
bool
cGEO::lines_cross(const Point &p11, const Point &p12,
    const Point &p21, const Point &p22, Point *pret)
{
    lsegx_t sx(p11, p12, p21, p22);
    if (sx.dd == 0.0)
        return (false);

    // Expand the test regions by about a half database unit, to
    // catch the cases where the floating-point test would fail, but
    // the lines would touch in integer space.  The fabs() sums
    // are approximate segment lengths.

    double n1 = sx.n1/sx.dd;
    double del1 = 0.5/(fabs(sx.x10) + fabs(sx.y10));
    if (n1 < -del1 || n1 > 1.0 + del1)
        return (false);
    double n2 = sx.n2/sx.dd;
    double del2 = 0.5/(fabs(sx.x22) + fabs(sx.y22));
    if (n2 < -del2 || n2 > 1.0 + del2)
        return (false);

    if (pret)
        pret->set(mmRnd(p11.x + n1*sx.x10), mmRnd(p11.y + n1*sx.y10));
    return (true);
}


namespace { const double fudge = 1e-9; }

// Return true if the two lines touch.  It touchok is false, the two
// lines must have an interior point in common, and not be parallel.
//
bool
sLine::lineIntersect(sLine *line, bool touchok)
{
    if (touchok) {
        if (BB.isect_i(&line->BB)) {
            lsegx_t sx(p1, p2, line->p1, line->p2);
            if (fabs(sx.dd) <= fudge) {
                // lines are parallel
                if (fabs(sx.n1) > fudge || fabs(sx.n2) > fudge)
                    return (false);
                // they are colinear, BBox test above implies overlap
                return (true);
            }
            double a1 = sx.n1/sx.dd;
            double a2 = sx.n2/sx.dd;
            if (a1 >= 0 && a1 <= 1.0 && a2 >= 0 && a2 <= 1.0)
                return (true);
        }
    }
    else {
        if (BB.isect_x(&line->BB)) {
            lsegx_t sx(p1, p2, line->p1, line->p2);
            if (fabs(sx.dd) <= fudge)
                // lines are parallel
                return (false);
            double a1 = sx.n1/sx.dd;
            double a2 = sx.n2/sx.dd;
            if (a1 > 0 && a1 < 1.0 && a2 > 0 && a2 < 1.0)
                return (true);
        }
    }
    return (false);
}


// Return true if the lines cross in [p1, p2).  If an endpoint of line
// is on this, set a code based on orientation.  This function is for
// testing path intersection.
//
bool
sLine::lineIntersect(sLine *line, int *code)
{
    *code = 0;
    if (BB.isect_i(&line->BB)) {
        lsegx_t sx(p1, p2, line->p1, line->p2);
        if (fabs(sx.dd) <= fudge)
            // lines are parallel
            return (false);
        double a1 = sx.n1/sx.dd;
        double a2 = sx.n2/sx.dd;
        if (a1 > -fudge && a1 < 1.0 - fudge) {

            // If the intersection is sufficiently close to an endpoint,
            // move the endpoint so as to avoid crossing.  Return a code
            // indicating that the endpoint is on the segment.
            //
            if (a2 > -fudge && a2 < 0.1) {
                double x = a2*sx.x22;
                double y = a2*sx.y22;
                double fx = fabs(x);
                double fy = fabs(y);
                if (fx <= 1.0 && fy <= 1.0) {
                    if (fx > 0)
                        line->p1.x += (x > 0.0 ? 1 : -1);
                    if (fy > 0)
                        line->p1.y += (y > 0.0 ? 1 : -1);
                    *code = -1;
                    return (false);
                }
            }
            else if (a2 > 0.9 && a2 < 1.0 + fudge) {
                double a2c = 1.0 - a2;
                double x = a2c*sx.x22;
                double y = a2c*sx.y22;
                double fx = fabs(x);
                double fy = fabs(y);
                if (fx <= 1.0 && fy <= 1.0) {
                    if (fx > 0)
                        line->p2.x += (x > 0.0 ? -1 : 1);
                    if (fy > 0)
                        line->p2.y += (y > 0.0 ? -1 : 1);
                    *code = 1;
                    return (false);
                }
            }
            if (a1 > fudge && a2 > fudge && a2 < 1.0 - fudge)
                // crossing (p1, p2)
                return (true);
        }
    }
    return (false);
}

