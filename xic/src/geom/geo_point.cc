
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

// Static function.
// Duplicate a Point array with transformation.
//
Point *
Point::dup_with_xform(const Point *thispt, const cTfmStack *tstk, int n)
{
    if (!thispt)
        return (0);
    if (n < 1)
        n = 1;
    Point *p = new Point[n];
    if (tstk)
        tstk->TPath(n, p, thispt);
    else {
        while (n--)
            p[n] = thispt[n];
    }
    return (p);
}


// Static function.
// Transform the coordinates.
//
void
Point::xform(Point *thispt, const cTfmStack *tstk, int n)
{
    if (thispt && tstk && n > 0)
        tstk->TPath(n, thispt);
}


// Static function.
// Copy to a new array one unit larger, adding x,y to end, but only if
// the new point differes from the provious last point.
// 'This' is deleted!
//
Point *
Point::append(Point *thispt, int *n, int xx, int yy)
{
    if (!thispt)
        *n = 0;
    else if (xx == thispt[*n - 1].x && yy == thispt[*n - 1].y)
        return (thispt);
    Point *p = new Point[*n + 1];
    int i = *n;
    while (i--)
        p[i] = thispt[i];
    p[*n].set(xx, yy);
    (*n)++;
    delete [] thispt;
    return (p);
}


// Static function.
// Copy to a new array one unit smaller, removing the last point.
// 'This' is deleted!
//
Point *
Point::remove_last(Point *thispt, int *n)
{
    if (!thispt || *n < 2) {
        *n = 0;
        delete [] thispt;
        return (0);
    }
    (*n)--;
    Point *p = dup(thispt, *n);
    delete [] thispt;
    return (p);
}


// Static function.
// Magnify the path around x, y.
//
void
Point::scale(Point *thispt, int n, double magn, int xx, int yy)
{
    if (!thispt || n < 1)
        return;
    while (n--) {
        thispt[n].set(mmRnd(xx + (thispt[n].x - xx)*magn),
            mmRnd(yy + (thispt[n].y - yy)*magn));
    }
}


// Static function.
//
Zlist *
Point::toZlist(Point *thispt, int npts)
{
    Poly po = Poly(npts, thispt);
    return (po.toZlist());
}


// Static function.
// Rotate -90 degrees before decomposing.
//
Zlist *
Point::toZlistR(Point *thispt, int npts)
{
    Poly po = Poly(npts, thispt);
    return (po.toZlistR());
}


// Static function.
// Is pi on the path?
// If yes, return true, and set po to an exact point on the path, however
// if within d of a vertex, favor the vertex point.
// pi:       input
// d:        assumed half-width of path
// po:       output, if not 0
// numpts:   assumed size of this
//
bool
Point::inPath(const Point *thispt, const Point *p, int d, Point *po,
    int numpts)
{
    if (!thispt || numpts < 2)
        return (false);

    d++;  // Avoid quantization error for small dimensions.
    if (d < 1)
        d = 1;
    const Point *p1 = thispt;
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


// Static function.
// Return the point nearest x, y.
//
Point *
Point::nearestVertex(Point *thispt, int numpts, int xx, int yy)
{
    if (!thispt)
        return (0);
    unsigned int minv = 0xffffffff;
    Point *p;
    int i, indx = 0;
    for (p = thispt, i = 0; i < numpts; p++, i++) {
        unsigned int d = abs(p->x - xx) + abs(p->y - yy);
        if (d < minv) {
            minv = d;
            indx = i;
        }
    }
    return (thispt + indx);
}


// Static function.
// Return the point nearest x, y.
//
const Point *
Point::nearestVertex(const Point *thispt, int numpts, int xx, int yy)
{
    if (!thispt)
        return (0);
    unsigned int minv = 0xffffffff;
    const Point *p;
    int i, indx = 0;
    for (p = thispt, i = 0; i < numpts; p++, i++) {
        unsigned int d = abs(p->x - xx) + abs(p->y - yy);
        if (d < minv) {
            minv = d;
            indx = i;
        }
    }
    return (thispt + indx);
}

