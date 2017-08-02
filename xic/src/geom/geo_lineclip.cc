
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


#define CODEXMIN 1
#define CODEYMIN 2
#define CODEXMAX 4
#define CODEYMAX 8

#define CODE(x, y, c) \
    c = 0;\
    if (x < xmin) c = CODEXMIN; \
    else if (x > xmax) c = CODEXMAX; \
    if (y < ymin) c |= CODEYMIN; \
    else if (y > ymax) c |= CODEYMAX;


// Static function.
// This will clip a line to a rectangular area.  The returned
// value is 'true' if the line is out of the AOI (therefore does not
// need to be displayed) and 'false' if the line is in the AOI.
//
bool
cGEO::line_clip(int *px1, int *py1, int *px2, int *py2, const BBox *BB)
{
    int xmin, xmax;
    if (BB->right > BB->left) {
        xmin = BB->left;
        xmax = BB->right;
    }
    else {
        xmin = BB->right;
        xmax = BB->left;
    }

    int ymin, ymax;
    if (BB->top > BB->bottom) {
        ymin = BB->bottom;
        ymax = BB->top;
    }
    else {
        ymin = BB->top;
        ymax = BB->bottom;
    }

    int x1 = *px1;
    int y1 = *py1;
    int c1;
    CODE(x1, y1, c1)

    int x2 = *px2;
    int y2 = *py2;
    int c2;
    CODE(x2, y2, c2)

    while (c1 || c2) {
        if (c1 & c2)
            return (true); // Line is invisible.
        int x, y, c = (c1 ? c1 : c2);
        if (c & CODEXMIN) {
            y = y1;
            int dy = y2 - y1;
            if (dy)
                y = mmRnd(y + dy*((double)(xmin - x1))/(x2 - x1));
            x = xmin;
        }
        else if (c & CODEXMAX) {
            y = y1;
            int dy = y2 - y1;
            if (dy)
                y = mmRnd(y + dy*((double)(xmax - x1))/(x2 - x1));
            x = xmax;
        }
        else if (c & CODEYMIN) {
            x = x1;
            int dx = x2 - x1;
            if (dx)
                x = mmRnd(x + dx*((double)(ymin - y1))/(y2 - y1));
            y = ymin;
        }
        else if (c & CODEYMAX) {
            x = x1;
            int dx = x2 - x1;
            if (dx)
                x = mmRnd(x + dx*((double)(ymax - y1))/(y2 - y1));
            y = ymax;
        }
        else {
            x = 0;
            y = 0;
        }
        if (c == c1) {
            x1 = x;
            y1 = y;
            CODE(x, y, c1)
        }
        else {
            x2 = x;
            y2 = y;
            CODE(x, y, c2)
        }
    }
    *px1 = x1;
    *py1 = y1;
    *px2 = x2;
    *py2 = y2;
    return (false); // Line is at least partially visible.
}


// Static function.
// Return true if the three points are colinear within +/- n units.
//
bool
cGEO::check_colinear(int x1, int y1, int x2, int y2, int x3, int y3, int n)
{
    // The "cross product" is 2X the area of the triangle
    double crs =
        fabs((x2 - x1)*(double)(y2 - y3) - (x2 - x3)*(double)(y2 - y1));

    // Instead of calling sqrt, use the following approximations for
    // distances between points.
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


// Static function.
// Return true if the three points are colinear within +/- n units.
//
bool
cGEO::check_colinear(const Point &p1, const Point &p2, const Point &p3, int n)
{
    // The "cross product" is 2X the area of the triangle
    double crs = fabs((p2.x - p1.x)*(double)(p2.y - p3.y) -
            (p2.x - p3.x)*(double)(p2.y - p1.y));

    // Instead of calling sqrt, use the following approximations for
    // distances between points.
    unsigned int dd21 = abs(p2.x - p1.x) + abs(p2.y - p1.y);
    unsigned int dd31 = abs(p3.x - p1.x) + abs(p3.y - p1.y);
    unsigned int dd32 = abs(p3.x - p2.x) + abs(p3.y - p2.y);

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


// Static function.
// Return true if the three points are colinear within +/- n units.
//
bool
cGEO::check_colinear(const Point *p1, const Point *p2, int x3, int y3, int n)
{
    // The "cross product" is 2X the area of the triangle
    double crs = fabs((p2->x - p1->x)*(double)(p2->y - y3) -
            (p2->x - x3)*(double)(p2->y - p1->y));

    // Instead of calling sqrt, use the following approximations for
    // distances between points.
    unsigned int dd21 = abs(p2->x - p1->x) + abs(p2->y - p1->y);
    unsigned int dd31 = abs(x3 - p1->x) + abs(y3 - p1->y);
    unsigned int dd32 = abs(x3 - p2->x) + abs(y3 - p2->y);

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


// Static function.
// If the two line segments are colinear, remove the part that
// overlaps, if any.  The points are modified so that they describe
// the non-overlapping parts, or they will describe a segment with
// zero length.  True is returned if the segments are colinear and
// overlap, false otherwise.
//
bool
cGEO::clip_colinear(Point &p1_1, Point &p1_2, Point &p2_1, Point &p2_2)
{
    // If either line has zero length, return;
    if (p1_1 == p1_2 || p2_1 == p2_2)
        return (false);

    // Process the horizontal case.
    if (p1_1.y == p1_2.y) {
        if (p2_1.y != p2_2.y)
            return (false);
        if (p1_1.y != p2_1.y)
            return (false);
        int min1 = mmMin(p1_1.x, p1_2.x);
        int max1 = mmMax(p1_1.x, p1_2.x);
        int min2 = mmMin(p2_1.x, p2_2.x);
        int max2 = mmMax(p2_1.x, p2_2.x);
        if (max1 <= min2 || max2 <= min1)
            return (false);

        p1_1.x = min1;
        p1_2.x = min2;
        p2_1.x = max1;
        p2_2.x = max2;
        return (true);
    }

    // Process the vertical case.
    if (p1_1.x == p1_2.x) {
        if (p2_1.x != p2_2.x)
            return (false);
        if (p1_1.x != p2_1.x)
            return (false);
        int min1 = mmMin(p1_1.y, p1_2.y);
        int max1 = mmMax(p1_1.y, p1_2.y);
        int min2 = mmMin(p2_1.y, p2_2.y);
        int max2 = mmMax(p2_1.y, p2_2.y);
        if (max1 <= min2 || max2 <= min1)
            return (false);

        p1_1.y = min1;
        p1_2.y = min2;
        p2_1.y = max1;
        p2_2.y = max2;
        return (true);
    }

    // We know line 1 is non-Manhattan, return if line2 is Manhattan.
    if (p2_1.x == p2_2.x || p2_1.y == p2_2.y)
        return (false);

    // Find the bounding boxes of the two non-Manhattan lines, return
    // if these do not overlap.
    BBox BB(p1_1.x, p1_1.y, p1_2.x, p1_2.y);
    BB.fix();
    BBox BB2(p2_1.x, p2_1.y, p2_2.x, p2_2.y);
    BB2.fix();
    if (!BB.intersect(&BB2, false))
        return (false);

    Point px[4];
    const int pval = 0;

    BB.add(p2_1.x, p2_1.y);
    BB.add(p2_2.x, p2_2.y);
    if (BB.right - BB.left > BB.top - BB.bottom) {
        if (p1_1.x < p1_2.x) {
            if (p2_1.x < p2_2.x) {
                if (p1_1.x < p2_1.x) {
                    px[0] = p1_1;
                    px[1] = p1_2;
                    px[2] = p2_1;
                    px[3] = p2_2;
                }
                else {
                    px[2] = p1_1;
                    px[3] = p1_2;
                    px[0] = p2_1;
                    px[1] = p2_2;
                }
            }
            else {
                if (p1_1.x < p2_2.x) {
                    px[0] = p1_1;
                    px[1] = p1_2;
                    px[2] = p2_2;
                    px[3] = p2_1;
                }
                else {
                    px[2] = p1_1;
                    px[3] = p1_2;
                    px[0] = p2_2;
                    px[1] = p2_1;
                }
            }
        }
        else {
            if (p2_1.x < p2_2.x) {
                if (p1_2.x < p2_1.x) {
                    px[0] = p1_2;
                    px[1] = p1_1;
                    px[2] = p2_1;
                    px[3] = p2_2;
                }
                else {
                    px[2] = p1_2;
                    px[3] = p1_1;
                    px[0] = p2_1;
                    px[1] = p2_2;
                }
            }
            else {
                if (p1_2.x < p2_2.x) {
                    px[0] = p1_2;
                    px[1] = p1_1;
                    px[2] = p2_2;
                    px[3] = p2_1;
                }
                else {
                    px[2] = p1_2;
                    px[3] = p1_1;
                    px[0] = p2_2;
                    px[1] = p2_1;
                }
            }
        }
        if (px[1].x <= px[2].x)
            return (false);
        if (px[1].x > px[3].x) {
            Point ptmp(px[1]);
            px[1] = px[3];
            px[3] = ptmp;
        }
        if (px[1].x > px[2].x) {
            Point ptmp(px[1]);
            px[1] = px[2];
            px[2] = ptmp;
        }
    }
    else {
        if (p1_1.y < p1_2.y) {
            if (p2_1.y < p2_2.y) {
                if (p1_1.y < p2_1.y) {
                    px[0] = p1_1;
                    px[1] = p1_2;
                    px[2] = p2_1;
                    px[3] = p2_2;
                }
                else {
                    px[2] = p1_1;
                    px[3] = p1_2;
                    px[0] = p2_1;
                    px[1] = p2_2;
                }
            }
            else {
                if (p1_1.y < p2_2.y) {
                    px[0] = p1_1;
                    px[1] = p1_2;
                    px[2] = p2_2;
                    px[3] = p2_1;
                }
                else {
                    px[2] = p1_1;
                    px[3] = p1_2;
                    px[0] = p2_2;
                    px[1] = p2_1;
                }
            }
        }
        else {
            if (p2_1.y < p2_2.y) {
                if (p1_2.y < p2_1.y) {
                    px[0] = p1_2;
                    px[1] = p1_1;
                    px[2] = p2_1;
                    px[3] = p2_2;
                }
                else {
                    px[2] = p1_2;
                    px[3] = p1_1;
                    px[0] = p2_1;
                    px[1] = p2_2;
                }
            }
            else {
                if (p1_2.y < p2_2.y) {
                    px[0] = p1_2;
                    px[1] = p1_1;
                    px[2] = p2_2;
                    px[3] = p2_1;
                }
                else {
                    px[2] = p1_2;
                    px[3] = p1_1;
                    px[0] = p2_2;
                    px[1] = p2_1;
                }
            }
        }
        if (px[1].y <= px[2].y)
            return (false);
        if (px[1].y > px[3].y) {
            Point ptmp(px[1]);
            px[1] = px[3];
            px[3] = ptmp;
        }
        if (px[1].y > px[2].y) {
            Point ptmp(px[1]);
            px[1] = px[2];
            px[2] = ptmp;
        }
    }

    if (!cGEO::check_colinear(px[0], px[3], px[1], pval))
        return (false);
    if (!cGEO::check_colinear(px[0], px[3], px[2], pval))
        return (false);

    p1_1 = px[0];
    p1_2 = px[1];
    p2_1 = px[2];
    p2_2 = px[3];
    return (true);
}

