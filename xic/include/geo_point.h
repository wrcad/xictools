
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
 $Id: geo_point.h,v 5.19 2015/06/11 05:54:06 stevew Exp $
 *========================================================================*/

#ifndef GEO_POINT_H
#define GEO_POINT_H


#include "miscmath.h"
#include <stdlib.h>

struct Zlist;
class cTfmStack;

// Coordinate element.  Note that this has no constructor so must
// be explicitly initialized before use.
//
struct Point
{
    bool operator==(const Point &p) const { return (x == p.x && y == p.y); }
    bool operator!=(const Point &p) const { return (x != p.x || y != p.y); }

    void set(int xx, int yy) { x = xx; y = yy; }
    void set(const Point &p) { x = p.x; y = p.y; }

    // Methods are static so we can freely allow a null "this" pointer.

    static Point *dup(const Point *pts, int n)
        {
            if (!pts)
                return (0);
            if (n < 1)
                n = 1;
            Point *p = new Point[n];
            while (n--)
                p[n] = pts[n];
            return (p);
        }

    // Remove duplicates in array by copying down.
    //
    static void removeDups(Point *pts, int *numpts)
        {
            if (!pts)
                return;
            int i = *numpts - 1;
            Point *p1, *p2;
            for (p1 = pts, p2 = p1 + 1; i > 0; i--, p2++) {
                if (*p1 == *p2) {
                    (*numpts)--;
                    continue;
                }
                p1++;
                if (p1 != p2)
                    *p1 = *p2;
            }
        }

    static Point *dup_with_xform(const Point*, const cTfmStack*, int);
    static void xform(Point*, const cTfmStack*, int);
    static Point *append(Point*, int*, int, int);
    static Point *remove_last(Point*, int*);
    static void scale(Point*, int, double, int, int);
    static Zlist *toZlist(Point*, int);
    static Zlist *toZlistR(Point*, int);

    static bool inPath(const Point*, const Point*, int, Point*, int);
    static Point *nearestVertex(Point*, int, int, int);
    static const Point *nearestVertex(const Point*, int, int, int);

    static double distance(const Point *p1, const Point *p2)
        {
            int dx = abs(p1->x - p2->x);
            int dy = abs(p1->y - p2->y);
            if (!dx)
                return ((double)dy);
            if (!dy)
                return ((double)dx);
            if (dx == dy)
                return (M_SQRT2*dy);
            return (sqrt(dx*(double)dx + dy*(double)dy));
        }

    int x, y;
};

// Point with constructor.
//
struct Point_c : public Point
{
    Point_c() { x = 0; y = 0; }
    Point_c(int xx, int yy) { x = xx; y = yy; }
    Point_c(const Point &p) { x = p.x; y = p.y; }
};


// Coordinate element (shorts).
struct PtShort
{
    short x, y;
};

// PtShort with constructor.
//
struct PtShort_c : public PtShort
{
    PtShort_c() {x = 0; y = 0; }
    PtShort_c(int xx, int yy) {x = xx; y = yy; }
    PtShort_c(const PtShort &p) { x = p.x; y = p.y; }
};


// This can be instantiated to provide a "big enough" point buffer.
//
template <class T>
struct PointBuf
{
    PointBuf(int initsize)
        {
            b_points = 0;
            b_numpts = 0;
            init(initsize);
        }

    ~PointBuf()
        {
            delete [] b_points;
        }

    void init(int n)
        {
            n += 10;
            if (n > b_numpts) {
                delete [] b_points;
                b_points = new T[n];
                b_numpts = n;
            }
        }

    T *points()     { return (b_points); }

private:
    T *b_points;
    int b_numpts;
};


// Coordinate list element.
//
struct Plist : public Point_c
{
    Plist() { next = 0; }
    Plist(int xx, int yy, Plist *n = 0) : Point_c(xx, yy) { next = n; }

    static void destroy(const Plist *p)
        {
            while (p) {
                const Plist *px = p;
                p = p->next;
                delete px;
            }
        }

    Plist *next;
};

#define PLIST_BLSIZE 64

//
// The following is a factory for batch allocation of Plist structs.
// Obviously, if the factory is used, one must not delete the Plist
// elements returned.
//

// Plist pool block
struct plist_blk_t
{
    plist_blk_t(plist_blk_t *n) { next = n; }

    plist_blk_t *next;
    char blk[sizeof(Plist[PLIST_BLSIZE])];
};

// Plist factory
struct plist_factory_t
{
    plist_factory_t() { blocks = 0; offs = 0; }

    ~plist_factory_t()
        {
            while (blocks) {
                plist_blk_t *b = blocks;
                blocks = blocks->next;
                delete b;
            }
        }

    Plist *new_Plist(int x, int y, Plist *n = 0)
        {
            if (!blocks || offs == PLIST_BLSIZE) {
                blocks = new plist_blk_t(blocks);
                offs = 0;
            }
            Plist *p = (Plist*)blocks->blk + offs++;
            p->x = x;
            p->y = y;
            p->next = n;
            return (p);
        }

private:
    plist_blk_t *blocks;
    int offs;
};

// List element for polygon edge.
//
struct edg_t
{
    edg_t(int x1, int y1, int x2, int y2, edg_t *n) :
        p1(x1, y1), p2(x2, y2) { next = n; }

    static void destroy(const edg_t *e)
        {
            while (e) {
                const edg_t *ex = e;
                e = e->next;
                delete ex;
            }
        }

    Point_c p1;
    Point_c p2;
    edg_t *next;
};

#endif

