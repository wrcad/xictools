
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

#ifndef GEO_PFINDER_H
#define GEO_PFINDER_H

#include "geo_alloc.h"
#include "miscutil/symtab.h"


//
// EdgeFinder
// This identifies the "outside" edges of a collection of trapezoids,
// i.e., those edges that are adjacent to unfilled area.  It retains
// orientation, allowing the segments to be linked into polygons. 
// Note, however, that the returned polys will contain no holes; any
// holes are returned as separate polygons.
//
class EdgeFinder;

enum ZEcode
{
    ZE_L = 0x0,
    ZE_T = 0x1,
    ZE_R = 0x2,
    ZE_B = 0x3
};


// Oriented line segment.  For horizontal, p1.x < p2.x, and for
// vertical or non-Manhattan, p1.y < p2.y, always.
//
struct oseg_t
{
    // Left or top order p1->p2, right or bottom p2->p1.
    int start_x()   { return ((ecode & ZE_R) ? p2.x : p1.x); }
    int start_y()   { return ((ecode & ZE_R) ? p2.y : p1.y); }
    int end_x()     { return ((ecode & ZE_R) ? p1.x : p2.x); }
    int end_y()     { return ((ecode & ZE_R) ? p1.y : p2.y); }

    Point p1;
    Point p2;
    ZEcode ecode;
};


// A database for Manhattan ordered segments.  These will be
// self-clipped to remove overlapping parts, and sorted into the
// array.
//
struct omdb_t
{
    omdb_t(bool vert)
        {
            m_segs = 0;
            m_size = 0;
            m_vert = vert;
            m_clipped = false;
        }

    ~omdb_t()
        {
            delete [] m_segs;
        }

    void add(int x1, int y1, int x2, int y2, ZEcode c)
        {
            oseg_t *s = m_allocator.new_obj();
            s->p1.set(x1, y1);
            s->p2.set(x2, y2);
            s->ecode = c;
        }

    void add_vertices(EdgeFinder*);

private:
    void clip();

    oseg_t **m_segs;                // array used for sorting
    int m_size;                     // size of array
    bool m_vert;                    // true when segments vertical
    bool m_clipped;                 // true after clip
    tGEOfact<oseg_t> m_allocator;   // allocator for oseg_t structs
};


// Non-Manhattan ordered line segment database.
//
struct onmdb_t
{
    onmdb_t()
        {
            nm_clipped = false;
        }

    void add(int x1, int y1, int x2, int y2, ZEcode c)
        {
            oseg_t *s = nm_allocator.new_obj();
            s->p1.set(x1, y1);
            s->p2.set(x2, y2);
            s->ecode = c;
        }

    void add_vertices(EdgeFinder*);

private:
    void clip();

    tGEOfact<oseg_t> nm_allocator;      // allocator for oseg_t structs
    bool nm_clipped;
};


// Element used in the vertex table, used for linking segments.
//
struct ovtx_t
{
    // Prerequisites.
    int tab_x()             const { return (v_x); }
    int tab_y()             const { return (v_y); }
    ovtx_t *tab_next()      const { return (v_next); }
    void set_tab_next(ovtx_t *n)  { v_next = n; }
    ovtx_t *tgen_next(bool) const { return (v_next); }

    void set(oseg_t *s)
        {
            v_x = s->start_x();
            v_y = s->start_y();
            v_next = 0;
            v_seg = s;
            v_dups = 0;
        }

    ovtx_t *dup_link()            { return (v_dups); }
    void set_dup_link(ovtx_t *v)  { v_dups = v; }

    oseg_t *seg()               { return (v_seg); }
    ovtx_t *dups()              { return (v_dups); }
    void set_dups(ovtx_t *d)    { v_dups = d; }

private:
    int v_x;                    // this vertex X
    int v_y;                    // this vertex Y
    ovtx_t *v_next;             // table link
    oseg_t *v_seg;              // containing segment
    ovtx_t *v_dups;             // duplicates
};


// Class to extract the outer edges from a collection of trapezoids. 
// The trapezoids must have been clipped/merged before being used
// here.
//
class EdgeFinder
{
public:
    EdgeFinder() : v_segs(true), h_segs(false)
        {
            xytab = 0;
        }

    ~EdgeFinder()
        {
            delete xytab;
        }

    // Warning: zoids added can't overlap!
    void add(const Zoid *z)
        {
            if (z->xul != z->xur)
                h_segs.add(z->xul, z->yu, z->xur, z->yu, ZE_T);
            if (z->xll != z->xlr)
                h_segs.add(z->xll, z->yl, z->xlr, z->yl, ZE_B);
            if (z->yu != z->yl) {
                if (z->xll == z->xul)
                    v_segs.add(z->xll, z->yl, z->xll, z->yu, ZE_L);
                else
                    nm_segs.add(z->xll, z->yl, z->xul, z->yu, ZE_L);
                if (z->xlr == z->xur)
                    v_segs.add(z->xlr, z->yl, z->xlr, z->yu, ZE_R);
                else
                    nm_segs.add(z->xlr, z->yl, z->xur, z->yu, ZE_R);
            }
        }

    void insert_vtx(oseg_t*);
    int get_poly(Point**, bool = false);

protected:
    void add_vertices()
        {
            xytab = new xytable_t<ovtx_t>;
            h_segs.add_vertices(this);
            v_segs.add_vertices(this);
            nm_segs.add_vertices(this);
        }

    ovtx_t *extract(ovtx_t*, ovtx_t*);
    void print_vertices();

    omdb_t v_segs;                  // vertical segments
    omdb_t h_segs;                  // horizontal segments
    onmdb_t nm_segs;                // non-Manhattan segments
    xytable_t<ovtx_t> *xytab;       // vertex table
    tGEOfact<ovtx_t> vtx_allocator; // ovtx_t memory manager
};

#endif

