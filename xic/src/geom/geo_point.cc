
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: geo_point.cc,v 1.17 2015/06/11 05:54:06 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "geo_poly.h"


//------------------------------------------------------------------------
// Point Functions
//------------------------------------------------------------------------

// Duplicate a Point array with transformation.
//
Point *
Point::dup_with_xform(const cTfmStack *tstk, int n) const
{
    {
        const Point *pt = this;
        if (!pt)
            return (0);
    }
    if (n < 1)
        n = 1;
    Point *p = new Point[n];
    if (tstk)
        tstk->TPath(n, p, this);
    else {
        while (n--)
            p[n] = this[n];
    }
    return (p);
}


// Transform the coordinates.
//
void
Point::xform(const cTfmStack *tstk, int n)
{
    Point *pt = this;
    if (pt && tstk && n > 0)
        tstk->TPath(n, pt);
}


// Copy to a new array one unit larger, adding x,y to end, but only if
// the new point differes from the provious last point.
// 'This' is deleted!
//
Point *
Point::append(int *n, int xx, int yy)
{
    Point *pt = this;
    if (!pt)
        *n = 0;
    else if (xx == pt[*n - 1].x && yy == pt[*n - 1].y)
        return (pt);
    Point *p = new Point[*n + 1];
    int i = *n;
    while (i--)
        p[i] = pt[i];
    p[*n].set(xx, yy);
    (*n)++;
    delete [] pt;
    return (p);
}


// Copy to a new array one unit smaller, removing the last point.
// 'This' is deleted!
//
Point *
Point::remove_last(int *n)
{
    Point *pt = this;
    if (!pt || *n < 2) {
        *n = 0;
        delete [] pt;
        return (0);
    }
    (*n)--;
    Point *p = pt->dup(*n);
    delete [] pt;
    return (p);
}


// Magnify the path around x, y.
//
void
Point::scale(int n, double magn, int xx, int yy)
{
    Point *pt = this;
    if (!pt || n < 1)
        return;
    while (n--) {
        pt[n].set(mmRnd(xx + (pt[n].x - xx)*magn),
            mmRnd(yy + (pt[n].y - yy)*magn));
    }
}


Zlist *
Point::toZlist(int npts)
{
    Poly po = Poly(npts, this);
    return (po.toZlist());
}


// Rotate -90 degrees before decomposing.
//
Zlist *
Point::toZlistR(int npts)
{
    Poly po = Poly(npts, this);
    return (po.toZlistR());
}


// Is pi on the path?
// If yes, return true, and set po to an exact point on the path, however
// if within d of a vertex, favor the vertex point.
// pi:       input
// d:        assumed half-width of path
// po:       output, if not 0
// numpts:   assumed size of this
//
bool
Point::inPath(const Point *p, int d, Point *po, int numpts) const
{
    d++;  // Avoid quantization error for small dimensions.
    if (d < 1)
        d = 1;
    const Point *p1 = this;
    for (numpts--; numpts; numpts--, p1++) {
        const Point *p2 = p1 + 1;
        if (p1->y == p2->y) {
            if (abs(p->y - p1->y) <= d) {
                int xmin = mmMin(p1->x, p2->x);
                int xmax = mmMax(p1->x, p2->x);
                if (xmin - d <= p->x && p->x <= xmax + d) {
                    if (po) {
                        po->y = p1->y;
                        if (xmax == p2->x) {
                            if (p2->x - p->x < d)
                                po->x = p2->x;
                            else if (p->x - p1->x < d)
                                po->x = p1->x;
                            else
                                po->x = p->x;
                        }
                        else {
                            if (p1->x - p->x < d)
                                po->x = p1->x;
                            else if (p->x - p2->x < d)
                                po->x = p2->x;
                            else
                                po->x = p->x;
                        }
                    }
                    return (true);
                }
            }
        }
        else if (p1->x == p2->x) {
            if (abs(p->x - p1->x) <= d) {
                int ymin = mmMin(p1->y, p2->y);
                int ymax = mmMax(p1->y, p2->y);
                if (ymin - d <= p->y && p->y <= ymax + d) {
                    if (po) {
                        po->x = p1->x;
                        if (ymax == p2->y) {
                            if (p2->y - p->y < d)
                                po->y = p2->y;
                            else if (p->y - p1->y < d)
                                po->y = p1->y;
                            else
                                po->y = p->y;
                        }
                        else {
                            if (p1->y - p->y < d)
                                po->y = p1->y;
                            else if (p->y - p2->y < d)
                                po->y = p2->y;
                            else
                                po->y = p->y;
                        }
                    }
                    return (true);
                }
            }
        }
        else {
            double l1sq = (p->x - p1->x)*(double)(p->x - p1->x) +
                (p->y - p1->y)*(double)(p->y - p1->y);
            double l2sq = (p->x - p2->x)*(double)(p->x - p2->x) +
                (p->y - p2->y)*(double)(p->y - p2->y);
            double lsq = (p1->x - p2->x)*(double)(p1->x - p2->x) +
                (p1->y - p2->y)*(double)(p1->y - p2->y);
            double l = sqrt(lsq);
            // l1sq - x^2 = h^2 = l2sq - (l-x)^2
            // l1sq - l2sq = x^2 - (l-x)^2
            // (lsq + l1sq - l2sq)/2l = x
            double tmp = 0.5*(lsq + l1sq - l2sq)/l;
            if (tmp >= 0.0 && tmp <= l && l1sq - tmp*tmp <= d*(double)d) {
                if (po) {
                    if (tmp < d)
                        *po = *p1;
                    else if (l - tmp < d)
                        *po = *p2;
                    else {
                        double dx = p2->x - p1->x;
                        double dy = p2->y - p1->y;
                        po->x = mmRnd(p1->x + dx*tmp/l);
                        po->y = mmRnd(p1->y + dy*tmp/l);
                    }
                }
                return (true);
            }
        }
    }
    return (false);
}


// Return the point nearest x, y.
//
Point *
Point::nearestVertex(int numpts, int xx, int yy)
{
    Point *pt = this;
    if (!pt)
        return (0);
    unsigned int minv = 0xffffffff;
    Point *p;
    int i, indx = 0;
    for (p = pt, i = 0; i < numpts; p++, i++) {
        unsigned int d = abs(p->x - xx) + abs(p->y - yy);
        if (d < minv) {
            minv = d;
            indx = i;
        }
    }
    return (pt + indx);
}


// Return the point nearest x, y.
//
const Point *
Point::nearestVertex(int numpts, int xx, int yy) const
{
    const Point *pt = this;
    if (!pt)
        return (0);
    unsigned int minv = 0xffffffff;
    const Point *p;
    int i, indx = 0;
    for (p = pt, i = 0; i < numpts; p++, i++) {
        unsigned int d = abs(p->x - xx) + abs(p->y - yy);
        if (d < minv) {
            minv = d;
            indx = i;
        }
    }
    return (pt + indx);
}

