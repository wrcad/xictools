
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
#include "geo_line.h"
#include "geo_polyobj.h"


//------------------------------------------------------------------------
// Measurement Functions
//------------------------------------------------------------------------

namespace {
    // Return the distance between the two points
    //
    inline int
    dist(const Point *p1, const Point *p2)
    {
        if (p1->x == p2->x)
            return (abs(p1->y - p2->y));
        if (p1->y == p2->y)
            return (abs(p1->x - p2->x));
        double dx = p2->x - p1->x;
        double dy = p2->y - p1->y;
        double d = sqrt(dx*dx + dy*dy);
        return (mmRnd(d));
    }
}


// Static function.
// Return the minimum distance from pr to the line segment p0,p1.  If
// pret is given, pr,pret are the endpoints of the shortest segment.
//
int
cGEO::mindist(const Point *pr, const Point *p0, const Point *p1, Point *pret)
{
    // Let p = p0 + s(p1 - p0)
    // If p is the intersection point of the perpendicular that passes
    // through pr, (pr - p) dot (p1 - p0) = 0, or
    //  (pr - p)x * (p1 - p0)x + (pr - p)y * (p1 - p0)y = 0
    // let dx = (p1 - p0)x, dy = (p1 - p0)y
    //  (prx - p0x - s*dx)*dx + (pry - p0y - s*dy)*dy = 0
    //  (prx - p0x)*dx + (pry - p0y)*dy = s*(dx*dx + dy*dy)
    //
    double dx = p1->x - p0->x;
    double dy = p1->y - p0->y;
    double s = ((pr->x - p0->x)*dx + (pr->y - p0->y)*dy)/(dx*dx + dy*dy);
    if (s <= 0) {
        if (pret)
            *pret = *p0;
        return (dist(pr, p0));
    }
    if (s >= 1) {
        if (pret)
            *pret = *p1;
        return (dist(pr, p1));
    }
    Point_c p(mmRnd(p0->x + s*dx), mmRnd(p0->y + s*dy));
    if (pret)
        *pret = p;
    return (dist(pr, &p));
}


// Static function.
// Return the minimum distance from pr to an edge of odesc.  If pr is
// on odesc, zero is returned.  If odesc is a wire that fails
// conversion to a polygon, -1 is returned.  If pret is given, it
// returns the endpoint of the shortest segment (pr is the other
// endpoint)
//
int
cGEO::mindist(const Point *pr, const CDo *odesc, Point *pret)
{
    if (odesc->intersect(pr, true))
        return (0);
    PolyObj drcpo(odesc, false);
    if (!drcpo.points())
        return (-1);
    int mind = CDinfinity;
    for (int i = 1; i < drcpo.numpts(); i++) {
        Point px;
        int d = mindist(pr, &drcpo.points()[i-1], &drcpo.points()[i], &px);
        if (d < mind) {
            if (pret)
                *pret = px;
            mind = d;
        }
    }
    return (mind);
}


// Static function.
// Return the minimum distance from the line segment to the object.
// If the line segment touches or overlaps the object, 0 is returned.
// If the object is a wire that fails conversion to a polygon, -1 is
// returned.  If pr1 and pr2 are given, they return the endpoints of
// the shortest segment
//
int
cGEO::mindist(const Point *p0, const Point *p1, const CDo *odesc,
    Point *pr1, Point *pr2)
{
    sLine l1(p0, p1);
    PolyObj drcpo(odesc, false);
    if (!drcpo.points())
        return (-1);
    int mind = CDinfinity;
    for (int i = 1; i < drcpo.numpts(); i++) {
        sLine l2(&drcpo.points()[i-1], &drcpo.points()[i]);
        if (l1.lineIntersect(&l2, true))
            return (0);
        Point px;
        int d = mindist(p0, &drcpo.points()[i-1], &drcpo.points()[i], &px);
        if (d < mind) {
            if (pr1)
                *pr1 = *p0;
            if (pr2)
                *pr2 = px;
            mind = d;
        }
        d = mindist(p1, &drcpo.points()[i-1], &drcpo.points()[i], &px);
        if (d < mind) {
            if (pr1)
                *pr1 = *p1;
            if (pr2)
                *pr2 = px;
            mind = d;
        }
        d = mindist(&drcpo.points()[i], p0, p1, &px);
        if (d < mind) {
            if (pr1)
                *pr1 = px;
            if (pr2)
                *pr2 = drcpo.points()[i];
            mind = d;
        }
    }
    if (odesc->intersect(p0, true))
        return (0);
    return (mind);
}


// Static function.
// Return the minimum distance between the two objects.  If the
// objects overlap, 0 is returned.  If either object is a wire that
// fails conversion to a polygon, -1 is returned.  If pr1 and pr2 are
// given, they return the endpoints of the shortest segment
//
int
cGEO::mindist(const CDo *odesc1, const CDo *odesc2, Point *pr1, Point *pr2)
{
    if (odesc1->intersect(odesc2, true))
        return (0);
    PolyObj drcpo1(odesc1, false);
    if (!drcpo1.points())
        return (-1);
    PolyObj drcpo2(odesc2, false);
    if (!drcpo2.points())
        return (-1);
    int mind = CDinfinity;
    for (int i = 1; i < drcpo1.numpts(); i++) {
        for (int j = 1; j < drcpo2.numpts(); j++) {
            Point px;
            int d = mindist(&drcpo1.points()[i],
                &drcpo2.points()[j-1], &drcpo2.points()[j], &px);
            if (d < mind) {
                if (pr1)
                    *pr1 = drcpo1.points()[i];
                if (pr2)
                    *pr2 = px;
                mind = d;
            }
            d = mindist(&drcpo2.points()[j],
                &drcpo1.points()[i-1], &drcpo1.points()[i], &px);
            if (d < mind) {
                if (pr1)
                    *pr1 = px;
                if (pr2)
                    *pr2 = drcpo2.points()[j];
                mind = d;
            }
        }
    }
    return (mind);
}


// Static function.
// Return the maximum distance between pr and the vertices of the
// object.  If the object is a wire that fails conversion to a
// polygon, -1 is returned.  If pret is given, it returns the
// endpoint of the lingest segment (pr is the other endpoint)
//
int
cGEO::maxdist(const Point *pr, const CDo *odesc, Point *pret)
{
    PolyObj drcpo(odesc, false);
    if (!drcpo.points())
        return (-1);
    int maxd = 0;
    for (int i = 1; i < drcpo.numpts(); i++) {
        int d = dist(pr, &drcpo.points()[i]);
        if (d > maxd) {
            if (pret)
                *pret = drcpo.points()[i];
            maxd = d;
        }
    }
    return (maxd);
}


// Static function.
// Return the maximum distance between vertices of the two objects
// (the objects can be the same).  If either object is a wire that
// fails conversion to a polygon, -1 is returned.  The pr1 and pr2,
// if given, return the longest segment endpoints
//
int
cGEO::maxdist(const CDo *odesc1, const CDo *odesc2, Point *pr1, Point *pr2)
{
    PolyObj drcpo1(odesc1, false);
    if (!drcpo1.points())
        return (-1);
    PolyObj drcpo2(odesc2, false);
    if (!drcpo2.points())
        return (-1);
    int maxd = 0;
    for (int i = 1; i < drcpo1.numpts(); i++) {
        for (int j = 1; j < drcpo2.numpts(); j++) {
            int d = dist(&drcpo1.points()[i], &drcpo2.points()[j]);
            if (d > maxd) {
                if (pr1)
                    *pr1 = drcpo1.points()[i];
                if (pr2)
                    *pr2 = drcpo2.points()[j];
                maxd = d;
            }
        }
    }
    return (maxd);
}

