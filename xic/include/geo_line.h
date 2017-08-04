
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

#ifndef GEO_LINE_H
#define GEO_LINE_H

#include "geo_box.h"


// line segment element
struct sLine
{
    sLine() { }
    sLine(const Point *pt1, const Point *pt2) :
        p1(pt1->x, pt1->y), p2(pt2->x, pt2->y),
        BB(mmMin(pt1->x, pt2->x), mmMin(pt1->y, pt2->y),
            mmMax(pt1->x, pt2->x), mmMax(pt1->y, pt2->y)) { }

    // geo_line.cc
    bool lineIntersect(sLine*, bool);
    bool lineIntersect(sLine*, int*);

    Point_c p1;
    Point_c p2;
    BBox BB;
};

// Struct to compute the intersection of two line segments.  Upon
// creation, dd == 0 if the lines are parallel.  Otherwise, n1/dd is
// proportional from p11 to the intersection, 1.0 at p12, and n2/dd is
// proportional from p21 to the intersection, 1.0 at p22.  Thus, the
// segments intersect if 0 <= n1/dd <= 1.0 and 0 <= n2/dd <= 1.0.
//
struct lsegx_t
{
    lsegx_t(const Point &p11, const Point &p12,
        const Point &p21, const Point &p22)
    {
        x10 = p12.x - p11.x;
        y10 = p12.y - p11.y;
        x20 = p21.x - p11.x;
        y20 = p21.y - p11.y;
        x22 = p22.x - p21.x;
        y22 = p22.y - p21.y;
        n1 = x22*y20 - y22*x20;
        n2 = x10*y20 - y10*x20;
        dd = x22*y10 - y22*x10;
    }

    double x10, y10, x20, y20, x22, y22, n1, n2, dd;
};

#endif

