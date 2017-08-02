
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

#include "config.h"
#include "geo.h"
#include "geo_linedb.h"
#include "geo_ylist.h"
#include "geo_poly.h"
#include "geo_wire.h"
#include "geo_line.h"
#include <algorithm>


//-----------------------------------------------------------------------------
// Implementation of a line segment database, which provides an
// efficient overlap-clipping capability.  This is used to find the
// "external" edges of polygons.
//-----------------------------------------------------------------------------

#ifndef M_SQRT1_2
#define	M_SQRT1_2	0.70710678118654752440	// 1/sqrt(2)
#endif
#ifndef M_PI
#define M_PI        3.14159265358979323846  // pi
#endif

#define SCALE_VAL 2


// Uncomment to display duplicate vertex warnings.  These should be
// handled transparently so we really don't care about these except
// for debugging.
//#define WARN_DUP_VTX


// Compute the "external" perimeter, i.e., the sum of the lengths of
// the parts of the original lines that do not overlap.
//
double
mdb_t::perim(const BBox *BB)
{
    if (!m_clipped)
        clip();
    double ptot = 0.0;
    if (m_vert) {
        for (int i = 0; i < m_size; i++) {
            seg_t *s = m_segs[i];
            unsigned int diff = s->p2.y - s->p1.y;
            if (BB && (s->p1.x == BB->left || s->p1.x == BB->right)) {
                ptot += (diff >> 1);
                if (diff & 1)
                    ptot += 0.5;
            }
            else
                ptot += diff;
        }
    }
    else {
        for (int i = 0; i < m_size; i++) {
            seg_t *s = m_segs[i];
            unsigned int diff = s->p2.x - s->p1.x;
            if (BB && (s->p1.y == BB->bottom || s->p1.y == BB->top)) {
                ptot += (diff >> 1);
                if (diff & 1)
                    ptot += 0.5;
            }
            else
                ptot += diff;
        }
    }
    return (ptot);
}


// Return a list of the clipped edges.
//
edg_t *
mdb_t::edges()
{
    if (!m_clipped)
        clip();
    edg_t *e0 = 0;
    for (int i = 0; i < m_size; i++) {
        seg_t *s = m_segs[i];
        e0 = new edg_t(s->p1.x, s->p1.y, s->p2.x, s->p2.y, e0);
    }
    return (e0);
}


// Return true if the line segment p1,p2 crosses one of the database
// segments in the interior, i.e., not on an endpoint.  The lines can
// not be parallel.
//
bool
mdb_t::edge_cross(const Point &p1, const Point &p2, bool scalefix)
{
    if (!m_clipped)
        clip();
    if (m_vert) {
        if (p1.x == p2.x)
            return (false);
        if (p1.y == p2.y) {
            sLine l0(&p1, &p2);
            for (int i = 0; i < m_size; i++) {
                seg_t *s = m_segs[i];
                Point_c sp1(s->p1);
                Point_c sp2(s->p2);
                if (scalefix) {
                    sp1.x *= SCALE_VAL;
                    sp1.y *= SCALE_VAL;
                    sp2.x *= SCALE_VAL;
                    sp2.y *= SCALE_VAL;
                }
                sLine l(&sp1, &sp2);
                if (l.BB.bottom > p1.y || l.BB.top < p1.y)
                    continue;
                if (l.BB.left > l0.BB.left && l.BB.left < l0.BB.right)
                    return (true);
            }
            return (false);
        }
    }
    else {
        if (p1.y == p2.y)
            return (false);
        if (p1.x == p2.x) {
            sLine l0(&p1, &p2);
            for (int i = 0; i < m_size; i++) {
                seg_t *s = m_segs[i];
                Point_c sp1(s->p1);
                Point_c sp2(s->p2);
                if (scalefix) {
                    sp1.x *= SCALE_VAL;
                    sp1.y *= SCALE_VAL;
                    sp2.x *= SCALE_VAL;
                    sp2.y *= SCALE_VAL;
                }
                sLine l(&sp1, &sp2);
                if (l.BB.left > p1.x || l.BB.right < p1.x)
                    continue;
                if (l.BB.bottom > l0.BB.bottom && l.BB.bottom < l0.BB.top)
                    return (true);
            }
            return (false);
        }
    }
    for (int i = 0; i < m_size; i++) {
        seg_t *s = m_segs[i];
        Point_c sp1(s->p1);
        Point_c sp2(s->p2);
        if (scalefix) {
            sp1.x *= SCALE_VAL;
            sp1.y *= SCALE_VAL;
            sp2.x *= SCALE_VAL;
            sp2.y *= SCALE_VAL;
        }
        Point pret;
        if (GEO()->lines_cross(p1, p2, sp1, sp2, &pret) &&
               p1 != pret && p2 != pret)
           return (true);
    }
    return (false);
}


//-----------------------------------------------------------------------------
// Note about the scaling fix.
//
// Inherent in the way that the edge construction works, the minimum
// spacing between 45 degree diagonals is sqrt(2), for a bloat value
// of 1 unit.  However, an alternate construction would yield a
// diagonal spacing of 1/sqrt(2) units, which is actually closer to
// the "real" spacing of 1 unit.
//
// There is a "magic" way to obtain this alternate construction. 
// Scale delta and all coordinates by 2.  Compute the edge template
// using the BLCextend corner fill.  Call repartition.  For all angles
// that are multiples of 45 degrees, the template trapezoid vertices
// are all on even coordinates!  Dividing all coordinates by 2 yields
// the alternate construction.  This relies on repartition doing the
// right thing!
//
// However, this is only beneficial for angles that are multiples of
// 45 degrees, and can actually be bad for other angles.  The scaling
// maps intersect values .25-0.75 into 0.5.  When this is truncated in
// the scale down, values .5-.75 are incorrectly rounded lower. 
// Alternatively, if the .5 values are incremented, values .25-.5 are
// incorrectly incremented.
//
// One might ask if scaling by a larger value would help.  Natural
// values 4 and 8 do not work, for numerical reasons.  Non-power 2
// values might work, but this can be accomplished readily by using a
// larger DatabaseResolution value.
//
// Here, we will provide the factor of 2 scaling fix, with the caveat
// that it should be used when only 45s are present.  It will provide
// better accuracy at the minimum resolution.
//-----------------------------------------------------------------------------

// Append to z0 a list of the clipped edges, as zoids bloated by
// delta.  If ldb is given, add the vertices to the database.
//
Zlist *
mdb_t::zoids(int delta, Zlist *z0, linedb_t *ldb)
{
    if (!m_clipped)
        clip();
    if (m_vert) {
        for (int i = 0; i < m_size; i++) {
            seg_t *s = m_segs[i];
            Point_c p1(s->p1);
            Point_c p2(s->p2);
            if (ldb && ldb->is_scaling()) {
                p1.x *= SCALE_VAL;
                p1.y *= SCALE_VAL;
                p2.x *= SCALE_VAL;
                p2.y *= SCALE_VAL;
            }
            z0 = new_zlist(p1.x - delta, p1.y, p2.x + delta, p2.y, z0);
            if (ldb) {
                ldb->add_vertex(p1.x, p1.y, p2.x, p2.y);
                ldb->add_vertex(p2.x, p2.y, p1.x, p1.y);
            }
        }
    }
    else {
        for (int i = 0; i < m_size; i++) {
            seg_t *s = m_segs[i];
            Point_c p1(s->p1);
            Point_c p2(s->p2);
            if (ldb && ldb->is_scaling()) {
                p1.x *= SCALE_VAL;
                p1.y *= SCALE_VAL;
                p2.x *= SCALE_VAL;
                p2.y *= SCALE_VAL;
            }
            z0 = new_zlist(p1.x, p1.y - delta, p2.x, p2.y + delta, z0);
            if (ldb) {
                ldb->add_vertex(p1.x, p1.y, p2.x, p2.y);
                ldb->add_vertex(p2.x, p2.y, p1.x, p1.y);
            }
        }
    }
    return (z0);
}


namespace {
    inline bool
    vcmp(const seg_t *s1, const seg_t *s2)
    {
        return (s1->p1.x < s2->p1.x);
    }


    inline bool
    hcmp(const seg_t *s1, const seg_t *s2)
    {
        return (s1->p1.y < s2->p1.y);
    }
}


// Sort the points.  The sorted points are left in m_segs.
//
void
mdb_t::clip()
{
    // Create an array loaded with pointers to the segments.
    m_size = m_allocator.allocated();
    if (!m_size)
        return;
    m_segs = new seg_t*[m_size];

    int cnt = 0;
    tGEOfact<seg_t>::gen gen(&m_allocator);
    seg_t *m;
    while ((m = gen.next()) != 0)
        m_segs[cnt++] = m;

    // Sort by the fixed coordinate, e.g., x for vertical.
    if (m_vert)
        std::sort(m_segs, m_segs + m_size, vcmp);
    else
        std::sort(m_segs, m_segs + m_size, hcmp);

    // Now, for each "scan line" (y/x), sort the (x/y) values (the
    // count is even).  Then, modify the segments to take these values
    // two at a time.  Ignore those with zero length, those that
    // remain are the clipped edges.

    // If m_skip_clip, keep all edges but adjust order.  This is used
    // for rendering wire paths.

    cnt = 0;
    int *ary = new int[2*m_size];
    int icnt = 0;
    if (m_vert) {
        int val = m_segs[0]->p1.x;
        for (int i = 0; ; i++) {
            ary[icnt++] = m_segs[i]->p1.y;
            ary[icnt++] = m_segs[i]->p2.y;
            int nxt = i+1;
            if (m_skip_clip || nxt == m_size || m_segs[nxt]->p1.x != val) {
                if (icnt == 2) {
                    if (ary[0] > ary[1]) {
                        int t = ary[0];
                        ary[0] = ary[1];
                        ary[1] = t;
                    }
                }
                else
                    std::sort(ary, ary + icnt);
                for (int j = 0; j < icnt; j += 2) {
                    if (ary[j+1] > ary[j]) {
                        m_segs[cnt]->p1.set(val, ary[j]);
                        m_segs[cnt]->p2.set(val, ary[j+1]);
                        cnt++;
                    }
                }
                if (nxt == m_size)
                    break;
                icnt = 0;
                val = m_segs[nxt]->p1.x;
            }
        }
    }
    else {
        int val = m_segs[0]->p1.y;
        for (int i = 0; ; i++) {
            ary[icnt++] = m_segs[i]->p1.x;
            ary[icnt++] = m_segs[i]->p2.x;
            int nxt = i+1;

            if (m_skip_clip || nxt == m_size || m_segs[nxt]->p1.y != val) {
                if (icnt == 2) {
                    if (ary[0] > ary[1]) {
                        int t = ary[0];
                        ary[0] = ary[1];
                        ary[1] = t;
                    }
                }
                else
                    std::sort(ary, ary + icnt);
                for (int j = 0; j < icnt; j += 2) {
                    if (ary[j+1] > ary[j]) {
                        m_segs[cnt]->p1.set(ary[j], val);
                        m_segs[cnt]->p2.set(ary[j+1], val);
                        cnt++;
                    }
                }
                if (nxt == m_size)
                    break;
                icnt = 0;
                val = m_segs[nxt]->p1.y;
            }
        }
    }
    m_size = cnt;
    delete [] ary;
    m_clipped = true;
}
// End of mdb_t functions.


// Return true if there is at least one non-Manhattan segment with
// nonzero length.
//
bool
nmdb_t::has_content()
{
    if (nm_allocator.is_empty())
        return (false);
    if (!nm_clipped)
        clip();
    tGEOfact<seg_t>::gen gen(&nm_allocator);
    seg_t *nm;
    while ((nm = gen.next()) != 0) {
        if (nm->p1 == nm->p2)
            continue;
        return (true);
    }
    return (false);
}


// Compute the perimeter for the non-Manhattan lines.
//
double
nmdb_t::perim()
{
    if (nm_allocator.is_empty())
        return (0);
    if (!nm_clipped)
        clip();
    double p = 0.0;
    tGEOfact<seg_t>::gen gen(&nm_allocator);
    seg_t *nm;
    while ((nm = gen.next()) != 0) {
        if (nm->p1 == nm->p2)
            continue;
        double dx = nm->p2.x - nm->p1.x;
        double dy = nm->p2.y - nm->p1.y;
        p += sqrt(dx*dx + dy*dy);
    }
    return (p);
}


edg_t *
nmdb_t::edges()
{
    if (nm_allocator.is_empty())
        return (0);
    if (!nm_clipped)
        clip();
    edg_t *e0 = 0;
    tGEOfact<seg_t>::gen gen(&nm_allocator);
    seg_t *nm;
    while ((nm = gen.next()) != 0) {
        if (nm->p1 == nm->p2)
            continue;
        e0 = new edg_t(nm->p1.x, nm->p1.y, nm->p2.x, nm->p2.y, e0);
    }
    return (e0);
}


// Return true if the line segment p1,p2 crosses one of the database
// segments in the interior, i.e., not on an endpoint.  The lines can
// not be parallel.
//
bool
nmdb_t::edge_cross(const Point &p1, const Point &p2, bool scalefix)
{
    if (nm_allocator.is_empty())
        return (false);
    if (p1 == p2)
        return (false);
    if (!nm_clipped)
        clip();

    tGEOfact<seg_t>::gen gen(&nm_allocator);
    seg_t *nm;
    while ((nm = gen.next()) != 0) {
        if (nm->p1 == nm->p2)
            continue;
        Point_c sp1(nm->p1);
        Point_c sp2(nm->p2);
        if (scalefix) {
            sp1.x *= SCALE_VAL;
            sp1.y *= SCALE_VAL;
            sp2.x *= SCALE_VAL;
            sp2.y *= SCALE_VAL;
        }
        Point pret;
        if (GEO()->lines_cross(p1, p2, sp1, sp2, &pret) &&
               p1 != pret && p2 != pret)
           return (true);
    }
    return (false);
}


Zlist *
nmdb_t::zoids(int delta, bool simple, Zlist *z0, linedb_t *ldb)
{
    if (nm_allocator.is_empty())
        return (0);
    if (!nm_clipped)
        clip();
    if (simple) {
        tGEOfact<seg_t>::gen gen(&nm_allocator);
        seg_t *nm;
        while ((nm = gen.next()) != 0) {
            if (nm->p1 == nm->p2)
                continue;
            if (nm->p1.y > nm->p2.y)
                z0 = new_zlist(nm->p2.x - delta, nm->p2.x + delta, nm->p2.y,
                    nm->p1.x - delta, nm->p1.x + delta, nm->p1.y, z0);
            else
                z0 = new_zlist(nm->p1.x - delta, nm->p1.x + delta, nm->p1.y,
                    nm->p2.x - delta, nm->p2.x + delta, nm->p2.y, z0);
        }
    }
    else {
        tGEOfact<seg_t>::gen gen(&nm_allocator);
        seg_t *nm;
        while ((nm = gen.next()) != 0) {
            if (nm->p1 == nm->p2)
                continue;
            Point_c p1(nm->p1);
            Point_c p2(nm->p2);
            if (ldb && ldb->is_scaling()) {
                p1.x *= SCALE_VAL;
                p1.y *= SCALE_VAL;
                p2.x *= SCALE_VAL;
                p2.y *= SCALE_VAL;
            }
            z0 = linedb_t::segZ(p1.x, p1.y, p2.x, p2.y, delta, z0);
            if (ldb) {
                ldb->add_vertex(p1.x, p1.y, p2.x, p2.y);
                ldb->add_vertex(p2.x, p2.y, p1.x, p1.y);
            }
        }
    }
    return (z0);
}


// Clip the non-Manhattan edges so that overlapping parts are removed.
//
void
nmdb_t::clip()
{
    tGEOfact<seg_t>::gen gen(&nm_allocator);
    tGEOfact<seg_t>::gen gen1 = gen;
    seg_t *nm1;
    while ((nm1 = gen.next()) != 0) {
        if (nm1->p1 == nm1->p2)
            continue;
        tGEOfact<seg_t>::gen gen2 = gen;
        seg_t *nm2;
        while ((nm2 = gen2.next()) != 0) {
            if (cGEO::clip_colinear(nm1->p1, nm1->p2, nm2->p1, nm2->p2)) {
                // nm1 changed, have to do over
                gen = gen1;
                break;
            }
        }
        gen1 = gen;
    }
    nm_clipped = true;
}
// End of nmdb_t functions.


// Add the edges of the trapezoid to the various containers.
//
void
linedb_t::add(const Zoid *z, const BBox *clipBB)
{
    add_h(z->yl, z->xll, z->xlr, clipBB);
    add_h(z->yu, z->xul, z->xur, clipBB);
    if (z->xll == z->xul)
        add_v(z->xll, z->yl, z->yu, clipBB);
    else
        add_nm(z->xll, z->yl, z->xul, z->yu, clipBB);
    if (z->xlr == z->xur)
        add_v(z->xlr, z->yl, z->yu, clipBB);
    else
        add_nm(z->xlr, z->yl, z->xur, z->yu, clipBB);
}


// Add the edges of the polygon to the various containers.
//
void
linedb_t::add(const Poly *po, const BBox *clipBB)
{
    for (int i = 1; i < po->numpts; i++) {
        Point *p2 = po->points + i;
        Point *p1 = p2 - 1;
        if (*p1 != *p2) {
            if (p1->x == p2->x)
                add_v(p1->x, p1->y, p2->y, clipBB);
            else if (p1->y == p2->y)
                add_h(p1->y, p1->x, p2->x, clipBB);
            else
                add_nm(p1->x, p1->y, p2->x, p2->y, clipBB);
        }
    }
}


// This is used to construct a path representation (from the zoids
// method.  This should be called once only, i.e., one wire only.
// No other add method sould be called.
//
void
linedb_t::add(const Wire *w, const BBox *clipBB)
{
    // Don't whine about singleton vertices.
    ldb_allow_open_path = true;

    // Don't clip, it is ok if the wire doubles back on itself, we don't
    // want to remove these parts.
    ldb_v_segs.set_skip_clip();
    ldb_h_segs.set_skip_clip();
    ldb_nm_segs.set_skip_clip();

    for (int i = 1; i < w->numpts; i++) {
        Point *p2 = w->points + i;
        Point *p1 = p2 - 1;
        if (*p1 != *p2) {
            if (p1->x == p2->x)
                add_v(p1->x, p1->y, p2->y, clipBB);
            else if (p1->y == p2->y)
                add_h(p1->y, p1->x, p2->x, clipBB);
            else
                add_nm(p1->x, p1->y, p2->x, p2->y, clipBB);
        }
    }
}


// Return a list of the external edges.
//
edg_t *
linedb_t::edges()
{
    edg_t *e0 = ldb_v_segs.edges();
    edg_t *ee = e0;
    edg_t *eh = ldb_h_segs.edges();
    if (eh) {
        if (ee) {
            while (ee->next)
                ee = ee->next;
            ee->next = eh;
        }
        else
            e0 = ee = eh;
    }
    edg_t *en = ldb_nm_segs.edges();
    if (en) {
        if (ee) {
            while (ee->next)
                ee = ee->next;
            ee->next = en;
        }
        else
            e0 = ee = en;
    }
    return (e0);
}


// Return a list of the external edges, as trapezoids bloated by delta.
//
// mode 0:
// If all segments are Manhattan, extend the vertical segments by
// delta.  Otherwise, compute the vertex trapezoids.  Thus, this mode
// will preserve Manhattan-ness of objects.
//
// mode 1:
// Extend vertical segments by delta, and use a single trapezoid for
// non-Manhattan segments.  This mode is probably fastest, but is not
// recommended for non-Manhattan geometry.
//
// mode 2:
// Compute all vertices, same as mode 0 but non-Manhattan objects will
// have corner fill-in.
//
// mode 3:
// Return only the edge trapezoids, no corners.  Non-Manhattan edges
// use the three-trapezoid rotated rectangle (non-simple) case, and
// vertical segments are not extended.
//
// Corner modes:  enum BLCtype { BLCclip, BLCflat, BLCextend1, BLCextend2 };
//
Zlist *
linedb_t::zoids(int delta, int mode)
{
    BLCtype cmode =
        (BLCtype)((mode & BL_CORNER_MODE_MASK) >> BL_CORNER_MODE_SHIFT);
    bool nomerge = mode & BL_NO_MERGE_OUT;
    bool doscale = mode & BL_SCALE_FIX;
    ldb_no_prj_fix = mode & BL_NO_PRJ_FIX;
    mode &= BL_MODE_MASK;

    // The "doscale" mode will effectively multiply all coords and
    // delta by two, then shrink the list by two when finished.  This
    // gives more accurate results for collections that contain angles
    // near 45 degrees.

    bool m0_manh = (mode == 0 ? !ldb_nm_segs.has_content() : true);
    if (m0_manh || mode == 1 || mode == 3) {
        doscale = false;
        ldb_no_verts = true;
    }
    delta = abs(delta);
    ldb_doing_scale_fix = doscale;
    if (doscale)
        delta += delta;

    Zlist *z0 = ldb_nm_segs.zoids(delta, mode == 1, 0, this);
    z0 = ldb_v_segs.zoids(delta, z0, this);
    if (m0_manh || mode == 1) {
        // Extend segments to cover corners.
        for (Zlist *z = z0; z; z = z->next) {
            z->Z.yu += delta;
            z->Z.yl -= delta;
        }
    }
    z0 = ldb_h_segs.zoids(delta, z0, this);
    if (!z0)
        return (0);

    if ((mode == 0 || mode == 2) && !ldb_no_verts && cmode < BLCunused1) {
        Zlist *zc = vertex_zoids(delta, cmode);
        if (zc) {
            Zlist *zn = z0;
            while (zn->next)
                zn = zn->next;
            zn->next = zc;
        }
    }
    if (!nomerge)
       z0 = Zlist::repartition_ni(z0);
    if (ldb_doing_scale_fix) {
        z0 = Zlist::shrink_by_2(z0);
        ldb_doing_scale_fix = false;
    }
    return (z0);
}


// When constructing the edge trapezoid list, we keep a hash list of the
// vertices, which are converted to squares or octagons.
//
void
linedb_t::add_vertex(int x, int y, int xx, int yy)
{
    if (ldb_no_verts)
        return;
    if (!ldb_xytab)
        ldb_xytab = new xytable_t<vtx_t>;
    vtx_t *e = ldb_xytab->find(x, y);
    if (!e) {
        e = ldb_vtx_allocator.new_obj();
        e->x = x;
        e->y = y;
        e->next = 0;
        e->p1.set(xx, yy);
        e->addcnt = 1;
        ldb_xytab->link(e, false);
        ldb_xytab = ldb_xytab->check_rehash();
    }
    else {
        if (e->addcnt == 1) {
            e->u.p2.set(xx, yy);
            e->addcnt = 2;
        }
        else if (e->addcnt == 2) {
            // Duplicate vertex, the boundary loops back to this
            // location at least once.  There is no reasonable way to
            // associate the from and to points "properly",
            // fortunately it doesn't matter!  However, have to be
            // careful with corner fill-in, which we deal with later.

#ifdef WARN_DUP_VTX
            GEO()->ifInfoMessage(IFMSG_LOG_WARN,
                "Line database/clipper warning:  "
                "duplicate vertex at %d %d.\n", x, y);
#endif

            vtx_t *ex = ldb_vtx_allocator.new_obj();
            ex->next = 0;
            ex->x = e->u.p2.x;
            ex->y = e->u.p2.y;
            ex->p1.set(xx, yy);
            e->u.dups = ex;
            e->addcnt = 3;
        }
        else if (e->addcnt == 3) {
            e->u.dups->u.p2.set(xx, yy);
            e->addcnt = 4;
        }
        else {
            vtx_t *ex = e->u.dups;
            while (ex->next)
                ex = ex->next;
            if (!((e->addcnt - 1) % 3)) {
                ex->next = ldb_vtx_allocator.new_obj();
                ex = ex->next;
                ex->next = 0;
                ex->x = xx;
                ex->y = yy;
            }
            else if (!((e->addcnt - 2) % 3))
                ex->p1.set(xx, yy);
            else
                ex->u.p2.set(xx, yy);
            e->addcnt++;
        }
    }
}


namespace {
    struct aval_t
    {
        void set(int xx, int yy)
            {
                x = xx;
                y = yy;
                ang = atan2(y, x);
            }

        double ang;
        int x;
        int y;
    };

    inline bool avcmp(const aval_t &a1, const aval_t &a2)
    {
        return (a1.ang < a2.ang);
    }

    // The p vector consists of the reference point in p[0], and the
    // endpoints of the "spokes".  If there are two adjacent spokes
    // that wind more that PI, return true with the endpoints in p[1]
    // and p[2].  These will be used for corner fill-in.  Otherwise,
    // return false, no corner fill is needed.
    //
    // This is an expensive operation, but not called much, if ever,
    // for "reasonable" design data.
    //
    bool resolve_multi_vertex(Point *p, int np)
    {
        // Sort by angle.
        int ap = np - 1;
        aval_t *av = new aval_t[ap];
        for (int i = 1; i < np; i++)
            av[i-1].set(p[i].x - p[0].x, p[i].y - p[0].y);
        std::sort(av, av + ap, avcmp);

        // If there are two adjacent "spokes" with an angle > PI, use
        // these.  Otherwise, no corner fill-in is needed.

        for (int i = 0; i < ap; i++) {
            double da;
            if (i == 0) {
                da = 2*M_PI + av[0].ang - av[ap-1].ang;
                if (da >= M_PI) {
                    p[1].x = av[0].x + p[0].x;
                    p[1].y = av[0].y + p[0].y;
                    p[2].x = av[ap-1].x + p[0].x;
                    p[2].y = av[ap-1].y + p[0].y;
                    delete [] av;
                    return (true);
                }
            }
            else {
                da = av[i].ang - av[i-1].ang;
                if (da >= M_PI) {
                    p[1].x = av[i].x + p[0].x;
                    p[1].y = av[i].y + p[0].y;
                    p[2].x = av[i-1].x + p[0].x;
                    p[2].y = av[i-1].y + p[0].y;
                    delete [] av;
                    return (true);
                }
            }
        }
        delete [] av;
        return (false);
    }
}


// Add the vertex trapezoids.
//
Zlist *
linedb_t::vertex_zoids(int delta, BLCtype cmode)
{
    Zlist *z0 = 0;
    tgen_t<vtx_t> gen(ldb_xytab);
    vtx_t *e;
    while ((e = gen.next()) != 0) {
        if (e->addcnt == 1) {
            // Uh-oh, the matching vertex wasn't found.  It is probably
            // off by one, so try to find it.
            vtx_t *ee = ldb_xytab->find(e->x+1, e->y);
            if (!ee || ee->addcnt != 1) {
                ee = ldb_xytab->find(e->x-1, e->y);
                if (!ee || ee->addcnt != 1) {
                    ee = ldb_xytab->find(e->x, e->y+1);
                    if (!ee || ee->addcnt != 1)
                        ee = ldb_xytab->find(e->x, e->y-1);
                }
            }
            if (ee && ee->addcnt == 1) {
                // found it, consolidate in e and invalidate ee.
                e->u.p2.set(ee->p1.x, ee->p1.y);
                e->addcnt = 2;
                ee->addcnt = 0;
            }
            else if (!ldb_allow_open_path) {
                GEO()->ifInfoMessage(IFMSG_LOG_WARN,
                    "Line database/clipper warning:  "
                    "singleton vertex at %d %d.\n", e->x, e->y);
            }
        }
        else if (e->addcnt == 2) {
            Point_c p(e->x, e->y);
            z0 = cornerZ(cmode, delta, &e->p1, &p, &e->u.p2, z0);
        }
        else {
            if (e->addcnt == 0)
                continue;
            if (e->addcnt & 1) {
                if (!ldb_allow_open_path) {
                    GEO()->ifInfoMessage(IFMSG_LOG_WARN,
                        "Line database/clipper warning:  "
                        "odd multi-vertex count at %d %d.\n", e->x, e->y);
                }
                continue;
            }
            // There are multiple vertices at this location.  Find the
            // two segments that are the most open, i.e., closest to
            // forming a straight line.  This will be the only corner
            // fill-in done, the rest will be covered.

            int np = e->addcnt + 1;
            Point *pts = new Point[np];
            pts[0].set(e->x, e->y);
            pts[1] = e->p1;
            vtx_t *ex = e->u.dups;
            for (int i = 2; i < np; i++) {
                if (!((i+1) % 3))
                    pts[i].set(ex->x, ex->y);
                else if (!(i % 3))
                    pts[i] = ex->p1;
                else {
                    pts[i] = ex->u.p2;
                    ex = ex->next;
                }
            }
            if (resolve_multi_vertex(pts, np))
                z0 = cornerZ(cmode, delta, &pts[1], &pts[0], &pts[2], z0);
            delete [] pts;
        }
    }
    return (z0);
}


// Static Function
// Return zoids to form a rotated rectangle covering the segment.
//
Zlist *
linedb_t::segZ(int x1, int y1, int x2, int y2, int d, Zlist *z0)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (!dx && !dy)
        return (z0);

    double d1 = sqrt((double)dx*dx + (double)dy*dy);
    double z = d/d1;
    double dxz = dx*z;
    double dyz = dy*z;

    // The p array will contain the rectangle corners, sorted
    // descending in Y, ascending in X.  Since the segment is
    // non-Manhattan, p[0] and p[3] are always unique in Y.
    //
    Point p[4];
    if (dy > 0) {
        if (dx > 0) {
            p[0].set(mmRnd(x2 - dyz), mmRnd(y2 + dxz));
            p[1].set(mmRnd(x2 + dyz), mmRnd(y2 - dxz));
            p[2].set(mmRnd(x1 - dyz), mmRnd(y1 + dxz));
            p[3].set(mmRnd(x1 + dyz), mmRnd(y1 - dxz));
        }
        else {
            p[0].set(mmRnd(x2 + dyz), mmRnd(y2 - dxz));
            p[1].set(mmRnd(x2 - dyz), mmRnd(y2 + dxz));
            p[2].set(mmRnd(x1 + dyz), mmRnd(y1 - dxz));
            p[3].set(mmRnd(x1 - dyz), mmRnd(y1 + dxz));
        }
    }
    else {
        if (dx > 0) {
            p[0].set(mmRnd(x1 - dyz), mmRnd(y1 + dxz));
            p[1].set(mmRnd(x1 + dyz), mmRnd(y1 - dxz));
            p[2].set(mmRnd(x2 - dyz), mmRnd(y2 + dxz));
            p[3].set(mmRnd(x2 + dyz), mmRnd(y2 - dxz));
        }
        else {
            p[0].set(mmRnd(x1 + dyz), mmRnd(y1 - dxz));
            p[1].set(mmRnd(x1 - dyz), mmRnd(y1 + dxz));
            p[2].set(mmRnd(x2 + dyz), mmRnd(y2 - dxz));
            p[3].set(mmRnd(x2 - dyz), mmRnd(y2 + dxz));
        }
    }

    if (p[2].y > p[1].y || (p[2].y == p[1].y && p[2].x < p[1].x)) {
        Point ptmp = p[1];
        p[1] = p[2];
        p[2] = ptmp;
    }

    int x1l, x1r, x2l, x2r;
    if (p[1].y != p[2].y) {
        double sl = (p[0].x - p[2].x)/(double)(p[0].y - p[2].y);
        if (p[1].x > p[2].x) {
            x1l = mmRnd(p[0].x - sl*(p[0].y - p[1].y));
            x1r = p[1].x;
            x2l = p[2].x;
            x2r = mmRnd(p[3].x + sl*(p[2].y - p[3].y));
        }
        else {
            x1l = p[1].x;
            x1r = mmRnd(p[0].x - sl*(p[0].y - p[1].y));
            x2l = mmRnd(p[3].x + sl*(p[2].y - p[3].y));
            x2r = p[2].x;
        }
    }
    else {
        x1l = p[1].x;
        x1r = p[2].x;
        x2l = x1l;
        x2r = x1r;
    }
    z0 = new_zlist(x1l, x1r, p[1].y, p[0].x, p[0].x, p[0].y, z0);
    if (p[1].y != p[2].y)
        z0 = new_zlist(x2l, x2r, p[2].y, x1l, x1r, p[1].y, z0);
    z0 = new_zlist(p[3].x, p[3].x, p[3].y, x2l, x2r, p[2].y, z0);
    return (z0);
}


namespace {
    inline bool csi(const Point *p1, const Point *p2, double &cx, double &cy)
    {
        if (p2->x == p1->x) {
            int dy = p2->y - p1->y;
            if (!dy)
                return (false);
            cx = 0.0;
            cy = dy > 0 ? 1.0 : -1.0;
            return (true);
        }
        if (p2->y == p1->y) {
            int dx = p2->x - p1->x;
            cx = dx > 0 ? 1.0 : -1.0;
            cy = 0;
            return (true);
        }

        double p21x = p2->x - p1->x;
        double p21y = p2->y - p1->y;
        double p21 = sqrt(p21x*p21x + p21y*p21y);
        cx = p21x/p21;
        cy = p21y/p21;
        return (true);
    }
}


// Create to corner fill-in zoids, adding them to the z0 list.
//
Zlist *
linedb_t::cornerZ(BLCtype bt, int d, const Point *p1, const Point *p2,
    const Point *p3, Zlist *z0)
{
    if ((p3->x - p1->x)*(double)(p2->y - p1->y) <
            (p3->y - p1->y)*(double)(p2->x - p1->x)) {
        const Point *tmp = p1;
        p1 = p3;
        p3 = tmp;
    }

    double x21, y21;
    if (!csi(p1, p2, x21, y21))
        return (z0);

    double x32, y32;
    if (!csi(p2, p3, x32, y32))
        return (z0);

    double d1x = -d*y21;
    double d1y = d*x21;
    double d2x = -d*y32;
    double d2y = d*x32;

    Point p[4];
    p[0] = *p2;
    p[1].set(mmRnd(d1x + p2->x), mmRnd(d1y + p2->y));
    p[2].set(mmRnd(d2x + p2->x), mmRnd(d2y + p2->y));

#ifdef HAVE_COMPUTED_GOTO
    static void *cj_array[] = { &&Lclip, &&Lflat, &&Lextend, &&Lextend1,
        &&Lextend2, &&Lunused1, &&Lunused2, &&Lnone };
    goto *cj_array[bt];
#else
    switch (bt) {
    case BLCclip:
        goto Lclip;
    case BLCflat:
        goto Lflat;
    case BLCextend:
        goto Lextend;
    case BLCextend1:
        goto Lextend1;
    case BLCextend2:
        goto Lextend2;
    case BLCunused1:
        goto Lunused1;
    case BLCunused2:
        goto Lunused2;
    case BLCnone:
        goto Lnone;
    }
#endif

Lclip:
    {
        // Return trapezoids to fill the open area at p2, by use of a
        // four-sided figure with the fourth point extended out by d,
        // producing a "rounded" effect.

        // For small angles, use flat corner fill-in.
        int dp = abs(p[2].x - p[1].x) + abs(p[2].y - p[1].y);
        if (dp < d/16)
            return (Poly::pts3_to_zlist(p, z0));
        p[3] = p[2];

        double t = 1.0 + y21*y32 + x21*x32;
        if (t <= 0.0)
            return (z0);
        t = sqrt(0.5/t);

        p[2].set(mmRnd(t*(d1x + d2x) + p2->x), mmRnd(t*(d1y + d2y) + p2->y));
        return (Poly::pts4_to_zlist(p, z0));
    }

Lflat:
    // Return trapezoids to fill the open area at p2, by creating
    // corner fill-in triangle.

    return (Poly::pts3_to_zlist(p, z0));

Lextend:
    {
        // Return trapezoids to fill the open area at p2, by use of a
        // four-sided figure with the fourth point the intersection of the
        // two bloated sides.

        // For very small angles, use flat corner fill-in.
        if (abs(p[2].x - p[1].x) <= 1 && abs(p[2].y - p[1].y) <= 1)
            return (Poly::pts3_to_zlist(p, z0));
        p[3] = p[2];

        Point_c pp1(mmRnd(d1x + p1->x), mmRnd(d1y + p1->y));
        Point_c pp3(mmRnd(d2x + p3->x), mmRnd(d2y + p3->y));

        lsegx_t lsg(p[1], pp1, p[3], pp3);
        if (lsg.dd == 0.0)
            return (z0);
        double rn = lsg.n1/lsg.dd;

        p[2].set(mmRnd(p[1].x + (pp1.x - p[1].x)*rn),
            mmRnd(p[1].y + (pp1.y - p[1].y)*rn));

        if (!ldb_no_prj_fix && edge_cross(p[0], p[2])) {
            // Here we prevent distortion that can be caused by a
            // corner projection which is too close to another edge,
            // creating a spurious shape on the far side of the edge
            // halo.  If an edge crosses the line segment between the
            // corner and the projected corner, we don't project this
            // corner.

            // The code below is friendly to layouts containing
            // Manhattan + 45s.  We try hard not to add non-45s to
            // such layouts.

            double dot = d1x*d2x + d1y*d2y;
            if (dot == 0) {
                // 90 degrees, we can use a flat corner here since
                // angles will be 45 degrees.

                p[2] = p[3];
                return (Poly::pts3_to_zlist(p, z0));
            }
            if (dot < 0) {
                // If either edge is Manhattan, form the corner by
                // extending it.

                if (p[2].y == p[1].y)
                    p[2].x = p[3].x;
                else if (p[2].y == p[3].y)
                    p[2].x = p[1].x;
                else if (p[2].x == p[1].x)
                    p[2].y = p[3].y;
                else if (p[2].x == p[3].x)
                    p[2].y = p[1].y;
                else {
                    // Neither edge is Manhattan, use a flat corner,
                    // which won't be a 45 but tough luck.

                    p[2] = p[3];
                    return (Poly::pts3_to_zlist(p, z0));
                }
            }
            // We don't clip obtuse angles.
        }

        return (Poly::pts4_to_zlist(p, z0));
    }

Lextend1:
    {
        // Return trapezoids to fill the open area at p2, by use of a
        // four-sided figure with the fourth point the intersection of the
        // two bloated sides.  The projection is truncated it it extends
        // too far.

        // For very small angles, use flat corner fill-in.
        if (abs(p[2].x - p[1].x) <= 1 && abs(p[2].y - p[1].y) <= 1)
            return (Poly::pts3_to_zlist(p, z0));
        p[3] = p[2];

        Point_c pp1(mmRnd(d1x + p1->x), mmRnd(d1y + p1->y));
        Point_c pp3(mmRnd(d2x + p3->x), mmRnd(d2y + p3->y));

        lsegx_t lsg(p[1], pp1, p[3], pp3);
        if (lsg.dd == 0.0)
            return (z0);
        double rn = lsg.n1/lsg.dd;

        p[2].set(mmRnd(p[1].x + (pp1.x - p[1].x)*rn),
            mmRnd(p[1].y + (pp1.y - p[1].y)*rn));

        // Truncate the projection if necessary.
        int dx = p[2].x - p[0].x;
        int dy = p[2].y - p[0].y;
        double dd = dx*(double)dx + dy*(double)dy;
        double dt = 2.1*d*d;
        if (dd > dt) {
            dt /= dd;
            p[2].set(mmRnd(p[0].x + dt*dx), mmRnd(p[0].y + dt*dy));
        }
        if (!ldb_no_prj_fix && edge_cross(p[0], p[2])) {
            // Here we prevent distortion that can be caused by a
            // corner projection which is too close to another edge,
            // creating a spurious shape on the far side of the edge
            // halo.  If an edge crosses the line segment between the
            // corner and the projected corner, we don't project this
            // corner.

            p[2] = p[3];
            return (Poly::pts3_to_zlist(p, z0));
        }

        return (Poly::pts4_to_zlist(p, z0));
    }

Lextend2:
    {
        // Return trapezoids to fill the open area at p2, by use of a
        // four-sided figure with the fourth point the intersection of the
        // two bloated sides.  For acute angles, a truncation method is
        // used.

        // For very small angles, use flat corner fill-in.
        if (abs(p[2].x - p[1].x) <= 1 && abs(p[2].y - p[1].y) <= 1)
            return (Poly::pts3_to_zlist(p, z0));
        p[3] = p[2];

        double dot = d1x*d2x + d1y*d2y;
        if (dot < 0.0) {
            // Angle is less than 90 degrees.
            double cross = d1x*d2y - d2x*d1y;
            double h = fabs(cross)/d;

            // For the segment that is closest to vertical or horizontal,
            // extend the p2 end of the outer edge-rectangle edge, and use
            // this point as the quadralateral vertex.

            if (fabs(fabs(x21) - fabs(y21)) > fabs(fabs(x32) - fabs(y32)))
                p[2].set(mmRnd(p[1].x + h*x21), mmRnd(p[1].y + h*y21));
            else
                p[2].set(mmRnd(p[3].x - h*x32), mmRnd(p[3].y - h*y32));
            if (!ldb_no_prj_fix && edge_cross(p[0], p[2])) {
                p[2] = p[3];
                return (Poly::pts3_to_zlist(p, z0));
            }
            return (Poly::pts4_to_zlist(p, z0));
        }

        Point_c pp1(mmRnd(d1x + p1->x), mmRnd(d1y + p1->y));
        Point_c pp3(mmRnd(d2x + p3->x), mmRnd(d2y + p3->y));

        lsegx_t lsg(p[1], pp1, p[3], pp3);
        if (lsg.dd == 0.0)
            return (z0);
        double rn = lsg.n1/lsg.dd;

        p[2].set(mmRnd(p[1].x + (pp1.x - p[1].x)*rn),
            mmRnd(p[1].y + (pp1.y - p[1].y)*rn));
        if (!ldb_no_prj_fix && edge_cross(p[0], p[2])) {
            // Here we prevent distortion that can be caused by a
            // corner projection which is too close to another edge,
            // creating a spurious shape on the far side of the edge
            // halo.  If an edge crosses the line segment between the
            // corner and the projected corner, we don't project this
            // corner.

            p[2] = p[3];
            return (Poly::pts3_to_zlist(p, z0));
        }

        return (Poly::pts4_to_zlist(p, z0));
    }

Lunused1:
Lunused2:
Lnone:
    return (z0);
}

