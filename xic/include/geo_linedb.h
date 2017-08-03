
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

#ifndef GEO_LINEDB_H
#define GEO_LINEDB_H

#include "geo_zlist.h"
#include "geo_alloc.h"
#include "symtab.h"


// Classes for a line segment database, which provides an efficient
// overlap-clipping capability.  This is used to find the "external"
// edges of polygons.

struct linedb_t;

// Line segment.
//
struct seg_t
{
    Point p1;
    Point p2;
};

// Database for the end points of Manhattan segments.  There will be
// separate databases for horizontal and vertical segments.
//
struct mdb_t
{
    mdb_t(bool vert)
        {
            m_segs = 0;
            m_size = 0;
            m_vert = vert;
            m_clipped = false;
            m_skip_clip = false;
        }

    ~mdb_t()
        {
            delete [] m_segs;
        }

    void add(int x1, int y1, int x2, int y2)
        {
            seg_t *m = m_allocator.new_obj();
            m->p1.set(x1, y1);
            m->p2.set(x2, y2);
        }

    void set_skip_clip()
        {
            m_skip_clip = true;
        }

    double perim(const BBox*);
    edg_t *edges();
    bool edge_cross(const Point&, const Point&, bool);
    Zlist *zoids(int, Zlist*, linedb_t*);

private:
    void clip();

    seg_t **m_segs;                 // array used for sorting
    int m_size;                     // size of array
    bool m_vert;                    // true when segments vertical
    bool m_clipped;                 // true after clip
    bool m_skip_clip;               // sort but don't clip
    tGEOfact<seg_t> m_allocator;    // allocator for seg_t structs
};

// Non-Manhattan line segment database.
//
struct nmdb_t
{
    nmdb_t() { nm_clipped = false; }

    void add(int x1, int y1, int x2, int y2)
        {
            seg_t *nm = nm_allocator.new_obj();
            nm->p1.set(x1, y1);
            nm->p2.set(x2, y2);
        }

    void set_skip_clip()
        {
            nm_clipped = true;
        }

    bool has_content();
    double perim();
    edg_t *edges();
    bool edge_cross(const Point&, const Point&, bool);
    Zlist *zoids(int, bool, Zlist*, linedb_t*);

private:
    void clip();

    tGEOfact<seg_t> nm_allocator;   // allocator for seg_t structs
    bool nm_clipped;
};

// Element used in the vertex table.
//
struct vtx_t
{
    friend struct linedb_t;

    int tab_x() const           { return (x); }
    int tab_y() const           { return (y); }
    vtx_t *tab_next() const     { return (next); }
    void set_tab_next(vtx_t *n) { next = n; }
    vtx_t *tgen_next(bool) const { return (next); }

private:
    int x;                  // this vertex X
    int y;                  // this vertex Y
    vtx_t *next;            // table link
    Point p1;               // connected vertex
    union {
        Point p2;           // connected vertex if addcnt == 2
        vtx_t *dups;        // extra vertices if addcnt > 2
    } u;
    int addcnt;             // calls, should be 2 when done
};


// Top-level struct for line segment database.
//
struct linedb_t
{
    friend struct mdb_t;
    friend struct nmdb_t;

    linedb_t() : ldb_v_segs(true), ldb_h_segs(false)
        {
            ldb_xytab = 0;
            ldb_no_verts = false;
            ldb_allow_open_path = false;
            ldb_no_prj_fix = false;
            ldb_doing_scale_fix = false;
        }

    ~linedb_t()
        {
            delete ldb_xytab;
        }

    double perim(const BBox *clipBB = 0)
        {
            return (ldb_v_segs.perim(clipBB) + ldb_h_segs.perim(clipBB) +
                ldb_nm_segs.perim());
        }

    bool is_scaling()
        {
            return (ldb_doing_scale_fix);
        }

    bool edge_cross(const Point &p1, const Point &p2)
        {
            return (ldb_h_segs.edge_cross(p1, p2, ldb_doing_scale_fix) ||
                ldb_v_segs.edge_cross(p1, p2, ldb_doing_scale_fix) ||
                ldb_nm_segs.edge_cross(p1, p2, ldb_doing_scale_fix));
        }

    void add(const Zoid*, const BBox* = 0);
    void add(const Poly*, const BBox* = 0);
    void add(const Wire*, const BBox* = 0);

    edg_t *edges();
    Zlist *zoids(int, int);

    static Zlist *segZ(int, int, int, int, int, Zlist*);
    Zlist *cornerZ(BLCtype, int, const Point*, const Point*, const Point*, Zlist*);

protected:
    void add_v(int x, int y1, int y2, const BBox *clipBB)
        {
            if (clipBB) {
                if (x < clipBB->left || x > clipBB->right)
                    return;
                if (y1 < clipBB->bottom)
                    y1 = clipBB->bottom;
                else if (y1 > clipBB->top)
                    y1 = clipBB->top;
                if (y2 < clipBB->bottom)
                    y2 = clipBB->bottom;
                else if (y2 > clipBB->top)
                    y2 = clipBB->top;
            }
            if (y1 == y2)
                return;
            ldb_v_segs.add(x, y1, x, y2);
        }

    void add_h(int y, int x1, int x2, const BBox *clipBB)
        {
            if (clipBB) {
                if (y < clipBB->bottom || y > clipBB->top)
                    return;
                if (x1 < clipBB->left)
                    x1 = clipBB->left;
                else if (x1 > clipBB->right)
                    x1 = clipBB->right;
                if (x2 < clipBB->left)
                    x2 = clipBB->left;
                else if (x2 > clipBB->right)
                    x2 = clipBB->right;
            }
            if (x1 == x2)
                return;
            ldb_h_segs.add(x1, y, x2, y);
        }

    void add_nm(int x1, int y1, int x2, int y2, const BBox *clipBB)
        {
            if (clipBB && GEO()->line_clip(&x1, &y1, &x2, &y2, clipBB))
                return;
            ldb_nm_segs.add(x1, y1, x2, y2);
        }

    void add_vertex(int, int, int, int);
    Zlist *vertex_zoids(int, BLCtype);

    mdb_t ldb_v_segs;                   // vertical segments
    mdb_t ldb_h_segs;                   // horizontal segments
    nmdb_t ldb_nm_segs;                 // non-Manhattan segments
    tGEOfact<vtx_t> ldb_vtx_allocator;  // vtx_t memory manager
    xytable_t<vtx_t> *ldb_xytab;        // vertex table
    bool ldb_no_verts;                  // don't save vertices
    bool ldb_allow_open_path;           // edges repesent a path, not a poly
    bool ldb_no_prj_fix;                // skip projection fix
    bool ldb_doing_scale_fix;           // doing scaling when zoidifying
};

#endif

