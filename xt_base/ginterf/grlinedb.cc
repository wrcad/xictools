
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2012 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id: grlinedb.cc,v 2.3 2012/12/04 01:42:29 stevew Exp $
 *========================================================================*/

#include "grlinedb.h"
#include "miscmath.h"
#include <stdlib.h>

using namespace ginterf;

// GRlineDb
//
// When using XOR mode for drawing ghost objects, if all or part of a
// line is drawn an even number of times, it will disappear, which is
// surprising and confusing to the user.
//
// This is a database that is used when XOR drawing to ensure that
// lines are drawn once only.

namespace {
    // The following was taken from the Xic geometry package for use
    // here.

    struct Point
    {
        Point()
            { x = 0; y = 0; }
        Point (int xx, int yy)
            { x = xx; y = yy; }

        bool operator==(const Point &p) const
            { return (x == p.x && y == p.y); }

        int x;
        int y;
    };

    bool clip_colinear(Point&, Point&, Point&, Point&);
}


// Add a vertical line to the database, return a list of lines that
// should be rendered.  This clips out the parts of the line that have
// already been drawn, if any.
//
// Note here that single-pixel lines are "vertical", it is important
// that the line draw function draw vertical lines first!
//
// The list elements are bulk-allocated and owned by the database,
// don't free!
//
const llist_t *
GRlineDb::add_vert(int x, int y1, int y2)
{

    if (!ldb_vline_tab)
        ldb_vline_tab = new itable_t<lelt_t>;
    llist_t *ll = ldb_list_factory.new_element();
    ll->set(y1, y2);
    lelt_t *elt = ldb_vline_tab->find(x);
    if (!elt) {
        elt = ldb_elt_factory.new_element();
        elt->list = ll;
        elt->key = x;
        elt->next = 0;
        ldb_vline_tab->link(elt, false);
        ldb_vline_tab = ldb_vline_tab->check_rehash();
        return (ll);
    }
    ll = clip_out(ll, elt->list);
    if (ll) {
        llist_t *lret = copy(ll);
        elt->list = merge(elt->list, ll);
        return (lret);
    }
    return (0);
}


// Add a horizontal line to the database, return a list of lines that
// should be rendered.  This clips out the parts of the line that have
// already been drawn, if any.
//
// The list elements are bulk-allocated and owned by the database,
// don't free!
//
const llist_t *
GRlineDb::add_horz(int y, int x1, int x2)
{
    if (!ldb_hline_tab)
        ldb_hline_tab = new itable_t<lelt_t>;
    llist_t *ll = ldb_list_factory.new_element();
    ll->set(x1, x2);
    lelt_t *elt = ldb_hline_tab->find(y);
    if (!elt) {
        elt = ldb_elt_factory.new_element();
        elt->list = ll;
        elt->key = y;
        elt->next = 0;
        ldb_hline_tab->link(elt, false);
        ldb_hline_tab = ldb_hline_tab->check_rehash();
        return (ll);
    }
    ll = clip_out(ll, elt->list);
    if (ll) {
        llist_t *lret = copy(ll);
        elt->list = merge(elt->list, ll);
        return (lret);
    }
    return (0);
}


// Add a non-Manhattan line to the database, return a list of lines
// that should be rendered.  This clips out the parts of the line that
// have already been drawn, if any.
//
// The list elements are bulk-allocated and owned by the database,
// don't free!
//
const nmllist_t *
GRlineDb::add_nm(int x1, int y1, int x2, int y2)
{
    nmllist_t *n0 = ldb_nm_line_store;
    if (n0)
        ldb_nm_line_store = n0->next();
    else
        n0 = ldb_nmlist_factory.new_element();
    n0->set(x1, y1, x2, y2);
    n0->set_next(0);

    nmllist_t *np = 0, *nm_next;
    for (nmllist_t *nl = n0; nl; nl = nm_next) {
        nm_next = nl->next();

        bool rid = false;
        for (nmllist_t *nml = ldb_nm_lines; nml; nml = nml->next()) {

            Point p11(nl->x1(), nl->y1());
            Point p12(nl->x2(), nl->y2());
            Point p21(nml->x1(), nml->y1());
            Point p22(nml->x2(), nml->y2());
            if (!clip_colinear(p11, p12, p21, p22))
                continue;

            if (p11 == p12) {
                if (p21 == p22) {
                    // Line is completely overlapped by existing line.
                    rid = true;
                    break;
                }
                nl->set(p21.x, p21.y, p22.x, p22.y);
            }
            else {
                nl->set(p11.x, p11.y, p12.x, p12.y);
                if (p21 == p22)
                    continue;
                nmllist_t *nx = ldb_nm_line_store;
                if (nx)
                    ldb_nm_line_store = nx->next();
                else
                    nx = ldb_nmlist_factory.new_element();
                nx->set(p21.x, p21.y, p22.x, p22.y);
                nx->set_next(nm_next);
                nm_next = nx;
                nl->set_next(nx);
            }
        }
        if (rid) {
            if (np)
                np->set_next(nl->next());
            else
                n0 = nl->next();
            nl->set_next(ldb_nm_line_store);
            ldb_nm_line_store = nl;
            continue;
        }
        np = nl;
    }
    if (n0) {
        // Really should merge here.
        if (!ldb_nm_lines)
            ldb_nm_lines = n0;
        else {
            nmllist_t *nx = ldb_nm_lines;
            while (nx->next())
                nx = nx->next();
            nx->set_next(n0);
        }
    }
    return (n0);
}


// Clip lc out of ll, and return whatever is left.  The ll is consumed.
//
llist_t *
GRlineDb::clip_out(llist_t *ll, const llist_t *lc)
{
    if (!ll)
        return (0);
    while (lc && lc->vmax() < ll->vmin())
        lc = lc->next();
    if (!lc)
        return (ll);

    llist_t *lp = 0;
    for (llist_t *l = ll; l; l = l->next()) {
        for ( ; lc; lc = lc->next()) {
            if (lc->vmin() > l->vmax())
                break;
            if (lc->vmax() < l->vmin())
                continue;
            if (l->vmin() < lc->vmin()) {
                if (l->vmax() >= lc->vmin()) {
                    if (l->vmax() > lc->vmax()) {
                        llist_t *lt = ldb_list_factory.new_element();
                        lt->set(lc->vmax() + 1, l->vmax());
                        lt->set_next(l->next());
                        l->set_next(lt);
                    }
                    l->set_vmax(lc->vmin() - 1);
                    break;
                }
            }
            else {
                if (l->vmax() > lc->vmax()) {
                    llist_t *lt = ldb_list_factory.new_element();
                    lt->set(lc->vmax() + 1, l->vmax());
                    lt->set_next(l->next());
                    l->set_next(lt);
                }
                if (lp) {
                    lp->set_next(l->next());
                    l = lp;
                }
                else {
                    ll = l->next();
                    l = ll;
                    if (!l)
                        break;
                }
            }
        }
        if (!l)
            break;
        lp = l;
    }
    return (ll);
}


// Stitch the two lists together, in order, collapsing any
// continuations.  Both lists are consumed.
//
llist_t *
GRlineDb::merge(llist_t *la, llist_t *lb)
{
    // Merge the two lists, we know that they are both ordered, and
    // no elements overlap.
    //
    llist_t *lret = 0, *le = 0;
    while (la || lb) {
        llist_t *lt;
        if (!lb) {
            lt = la;
            la = la->next();
        }
        else if (!la) {
            lt = lb;
            lb = lb->next();
        }
        else if (la->vmin() < lb->vmin()) {
            lt = la;
            la = la->next();
        }
        else {
            lt = lb;
            lb = lb->next();
        }
        lt->set_next(0);
        if (!lret)
            lret = le = lt;
        else {
            le->set_next(lt);
            le = lt;
        }
    }

    // Merge adjacent segments.
    //
    llist_t *ln;
    for (llist_t *l = lret; l; l = ln) {
        ln = l->next();
        while (ln && l->vmax() + 1 == ln->vmin()) {
            l->set_vmax(ln->vmax());
            ln = ln->next();
            l->set_next(ln);
        }
    }
    return (lret);
}


// Return a copy of the list.
//
llist_t *
GRlineDb::copy(const llist_t *lx)
{
    llist_t *lr = 0, *le = 0;
    while (lx) {
        llist_t *lt = ldb_list_factory.new_element();
        lt->set(lx->vmin(), lx->vmax());
        if (!lr)
            lr = le = lt;
        else {
            le->set_next(lt);
            le = lt;
        }
        lx = lx->next();
    }
    return (lr);
}
// End of GRlineDb functions.


namespace {
    // The following was taken from the Xic geometry package for use
    // here.

    struct BBox
    {
        BBox() { left = 0; bottom = 0; right = 0; top = 0; }
        BBox(int l, int b, int r, int t)
            { left = l; bottom = b; right = r; top = t; }

        void add(int x, int y)
            {
                if (x < left)
                    left = x;
                if (x > right)
                    right = x;
                if (y < bottom)
                    bottom = y;
                if (y > top)
                    top = y;
            }

        // Swap values if necessary.
        void fix()
            {
                if (left > right) { int t = left; left = right; right = t; }
                if (bottom > top) { int t = bottom; bottom = top; top = t; }
            }

        bool isect_i(const BBox *BB) const    // touching ok
            { return (right >= BB->left && BB->right >= left &&
                top >= BB->bottom && BB->top >= bottom); }

        bool isect_x(const BBox *BB) const    // touching not ok
            { return (right > BB->left && BB->right > left &&
                top > BB->bottom && BB->top > bottom); }
        
        bool intersect(const BBox *BB, bool touchok) const
            { return ((bool)(touchok ? isect_i(BB) : isect_x(BB))); }

        int left;
        int bottom;
        int right;
        int top;
    };


    // Return true if the three points are colinear within +/- n units.
    //
    bool check_colinear(const Point &p1, const Point &p2, const Point &p3,
        int n)
    {
        // The "cross product" is 2X the area of the triangle
        unsigned int crs = (unsigned int)
            fabs((p2.x - p1.x)*(double)(p2.y - p3.y) -
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


    // If the two line segments are colinear, remove the part that
    // overlaps, if any.  The points are modified so that they
    // describe the non-overlapping parts of line 1, or they will
    // describe a segment with zero length.  True is returned if the
    // segments are colinear and overlap, false otherwise.
    //
    bool clip_colinear(Point &p1_1, Point &p1_2, Point &p2_1, Point &p2_2)
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

            if (p1_1.x > p1_2.x) {
                mmSwapInts(p1_1.x, p1_2.x);
            }
            if (p2_1.x > p2_2.x) {
                mmSwapInts(p2_1.x, p2_2.x);
            }
            if (p1_2.x <= p2_1.x || p2_2.x <= p1_1.x)
                return (false);

            p1_2.x = p2_1.x;
            if (p1_1.x > p2_1.x)
                p1_1.x = p2_1.x;

            p2_1.x = p2_2.x;
            if (p1_2.x > p2_2.x)
                p2_2.x = p1_2.x;
            return (true);
        }

        // Process the vertical case.
        if (p1_1.x == p1_2.x) {
            if (p2_1.x != p2_2.x)
                return (false);
            if (p1_1.x != p2_1.x)
                return (false);

            if (p1_1.y > p1_2.y) {
                mmSwapInts(p1_1.y, p1_2.y);
            }
            if (p2_1.y > p2_2.y) {
                mmSwapInts(p2_1.y, p2_2.y);
            }
            if (p1_2.y <= p2_1.y || p2_2.y <= p1_1.y)
                return (false);

            p1_2.y = p2_1.y;
            if (p1_1.y > p2_1.y)
                p1_1.y = p2_1.y;

            p2_1.y = p2_2.y;
            if (p1_2.y > p2_2.y)
                p2_2.y = p1_2.y;
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

        Point pt1[4];
        Point pt2[4];
        const int pval = 0;

        BB.add(p2_1.x, p2_1.y);
        BB.add(p2_2.x, p2_2.y);
        if (BB.right - BB.left > BB.top - BB.bottom) {

            if (p1_1.x > p1_2.x) {
                Point pt = p1_1;
                p1_1 = p1_2;
                p1_2 = pt;
            }
            if (p2_1.x > p2_2.x) {
                Point pt = p2_1;
                p2_1 = p2_2;
                p2_2 = pt;
            }
            if (p1_2.x <= p2_1.x || p2_2.x <= p1_1.x)
                return (false);

            if (p1_1.x < p2_1.x) {
                pt1[0] = p1_1;
                pt1[1] = p2_1;
                pt2[0] = p1_1;
                pt2[1] = p2_1;
            }
            else {
                pt1[0] = p2_1;
                pt1[1] = p1_1;
                pt2[0] = p2_1;
                pt2[1] = p2_1;
            }
            if (p1_2.x > p2_2.x) {
                pt1[2] = p2_2;
                pt1[3] = p1_2;
                pt2[2] = p2_2;
                pt2[3] = p1_2;
            }
            else {
                pt1[2] = p1_2;
                pt1[3] = p2_2;
                pt2[2] = p2_2;
                pt2[3] = p2_2;
            }
        }
        else {
            if (p1_1.y > p1_2.y) {
                Point pt = p1_1;
                p1_1 = p1_2;
                p1_2 = pt;
            }
            if (p2_1.y > p2_2.y) {
                Point pt = p2_1;
                p2_1 = p2_2;
                p2_2 = pt;
            }
            if (p1_2.y <= p2_1.y || p2_2.y <= p1_1.y)
                return (false);

            if (p1_1.y < p2_1.y) {
                pt1[0] = p1_1;
                pt1[1] = p2_1;
                pt2[0] = p1_1;
                pt2[1] = p2_1;
            }
            else {
                pt1[0] = p2_1;
                pt1[1] = p1_1;
                pt2[0] = p2_1;
                pt2[1] = p2_1;
            }
            if (p1_2.y > p2_2.y) {
                pt1[2] = p2_2;
                pt1[3] = p1_2;
                pt2[2] = p2_2;
                pt2[3] = p1_2;
            }
            else {
                pt1[2] = p1_2;
                pt1[3] = p2_2;
                pt2[2] = p2_2;
                pt2[3] = p2_2;
            }
        }
        if (!check_colinear(pt1[0], pt1[3], pt1[1], pval))
            return (false);
        if (!check_colinear(pt1[0], pt1[3], pt1[2], pval))
            return (false);
        p1_1 = pt2[0];
        p1_2 = pt2[1];
        p2_1 = pt2[2];
        p2_2 = pt2[3];
        return (true);
    }
}

