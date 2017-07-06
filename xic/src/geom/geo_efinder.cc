
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
 $Id: geo_efinder.cc,v 1.5 2015/07/17 19:44:02 stevew Exp $
 *========================================================================*/

#include "geo.h"
#include "geo_zoid.h"
#include "geo_poly.h"
#include "geo_efinder.h"
#include "errorrec.h"
#include <algorithm>


//
// EdgeFinder
// This identifies the "outside" edges of a collection of trapezoids,
// i.e., those edges that are adjacent to unfilled area.  It retains
// orientation, allowing the segments to be linked into polygons. 
// Note, however, that the returned polys will contain no holes; any
// holes are returned as separate polygons.
//

// Turn on debugging
#define EF_DEBUG

// Set debugging output level, 0-3.
#define EF_DEBUG_VERBOSE 0


#ifdef NOTDEF
#include "geo_ylist.h"
#include "geo_zgroup.h"
#include "cd.h"
#include "cd_types.h"

// A debugging hack for the efinder, causes toPolyAdd to use polys
// from the efinder (these don't have holes!) if the variable "ET" is
// set.  Maybe someday the efinder can reassemble the polys and holes
// to be useful as a general zoid->poly function.
//
// Convert the zoid list to polygons, which are added to the database.
// This frees the zoid list.
//
XIrt
Zlist::to_poly_add(Zlist *thiszl, CDs *sdesc, CDl *ld, bool undoable,
    const cTfmStack *tstk, bool use_merge)
{
    if (!thiszl)
        return (XIok);
    if (!sdesc || !ld)
        return (XIbad);

    Ylist *y = new Ylist(thiszl);
    Zgroup *g = Ylist::group(y, JoinMaxGroup);
    if (!g)
        return (XIok);

    // Debugging hack for the efinder.  Holes and polys are returned as
    // separate polygons.

    if (CDvdb()->getVariable("ET")) {
        for (int i = 0; i < g->num; i++) {
            EdgeFinder ef;
            for (Zlist *zl = g->list[i]; zl; zl = zl->next)
                ef.add(&zl->Z);

            PolyList *p0 = 0;
            Poly po;
            for (;;) {
                po.numpts = ef.get_poly(&po.points);
                if (po.numpts < 0)
                    return (XIbad);
                if (!po.numpts)
                    break; 
                p0 = new PolyList(po, p0);
            }
            if (p0) {
                if (sdesc->addToDb(p0, ld, undoable, 0, tstk, use_merge) !=
                        CDok)
                    GEO()->ifInfoMessage(IFMSG_LOG_ERR, Errs()->get_error());
                PolyList::destroy(p0);
            }
        }
    }
    else {
        Errs()->init_error();
        for (int i = 0; i < g->num; i++) {
            PolyList *p0 = g->to_poly_list(i, JoinMaxVerts);
            if (p0) {
                if (sdesc->addToDb(p0, ld, undoable, 0, tstk, use_merge) !=
                        CDok)
                    GEO()->ifInfoMessage(IFMSG_LOG_ERR,
                        Errs()->get_error());
                PolyList::destroy(p0);
            }
        }
    }

    /* replaced code
    Errs()->init_error();
    for (int i = 0; i < g->num; i++) {
        PolyList *p0 = g->to_poly_list(i, JoinMaxVerts);
        if (p0) {
            if (sdesc->addToDb(p0, ld, undoable, 0, tstk, use_merge) != CDok)
                GEO()->ifInfoMessage(IFMSG_LOG_ERR,
                    Errs()->get_error());
            PolyList::destroy(p0);
        }
    }
    */

    delete g;
    return (XIok);
}
#endif


// Add the segment vertices to the EdgeFinder database.
//
void
omdb_t::add_vertices(EdgeFinder *ef)
{
    if (!m_clipped)
        clip();
    for (int i = 0; i < m_size; i++) {
        oseg_t *s = m_segs[i];
        if (s->p1 == s->p2)
            continue;
        ef->insert_vtx(s);
    }
}


// Sorting comparison functions.

namespace {
    // Vertical segments, sort ascending in x.  For equal x, ascending
    // in p1.y.  For equal p1.y, ascending in p2.y.
    //
    inline bool
    vcmp(const oseg_t *s1, const oseg_t *s2)
    {
        if (s1->p1.x < s2->p1.x)
            return (true);
        if (s1->p1.x == s2->p1.x) {
            if (s1->p1.y < s2->p1.y)
                return (true);
            if (s1->p1.y == s2->p1.y) {
                if (s1->p2.y < s2->p2.y)
                    return (true);
            }
        }
        return (false);
    }


    // Horizontal segments, sort ascending in y.  For equal y, ascending
    // in p1.x.  For equal p1.x, ascending in p2.x.
    //
    inline bool
    hcmp(const oseg_t *s1, const oseg_t *s2)
    {
        if (s1->p1.y < s2->p1.y)
            return (true);
        if (s1->p1.y == s2->p1.y) {
            if (s1->p1.x < s2->p1.x)
                return (true);
            if (s1->p1.x == s2->p1.x) {
                if (s1->p2.x < s2->p2.x)
                    return (true);
            }
        }
        return (false);
    }


    // Manhattan segment endpoint, retains edge code and start/end
    // status.
    struct cdint_t
    {
        int val;
        short code;
        bool end;
    };

    // Comparison function for Manhattan segment endpoints.
    //
    inline bool
    ci_cmp(const cdint_t &i1, const cdint_t &i2)
    {
        if (i1.val < i2.val)
            return (true);
        if (i1.val == i2.val && i1.end && !i2.end)
            return (true);
        return (false);
    }
}


// Sort and clip the segments to remove the overlapping parts.  The
// sorted list is in m_segs.  Note that some elements may have zero
// length after processing, which indicates that they should be
// ignored.
//
void
omdb_t::clip()
{
    if (m_clipped)
        return;

    // The "opposite" code, R<->L, Y<->B.
    static char opp_code[4] = { 2, 3, 0, 1 };

    // Create an array loaded with pointers to the segments.
    m_size = m_allocator.allocated();
    if (!m_size)
        return;
    m_segs = new oseg_t*[m_size];

    int cnt = 0;
    tGEOfact<oseg_t>::gen gen(&m_allocator);
    oseg_t *s;
    while ((s = gen.next()) != 0)
        m_segs[cnt++] = s;

    if (m_vert)
        std::sort(m_segs, m_segs + m_size, vcmp);
    else
        std::sort(m_segs, m_segs + m_size, hcmp);

    // The database contains segs of two codes, R/L or B/T.  Since the
    // inserted zoids have been clipped/merged, only segs with
    // different types will overlap.

    for (int nstart = 0; nstart < m_size; ) {
        ZEcode c = m_segs[nstart]->ecode;
        int nend = nstart;
        cdint_t *ci;
        if (m_vert) {
            int xval = m_segs[nstart]->p1.x;

            int oppcnt = 0;
            while (nend < m_size && m_segs[nend]->p1.x == xval) {
                if (m_segs[nend]->ecode != c)
                    oppcnt++;
                nend++;
            }
            if (!oppcnt) {
                nstart = nend;
                continue;
            }

            // This scan line contains both code types, we need to remove the
            // overlap regions.

            // The number of segs in the scan line is the same after clipping,
            // though some segs may be invalidated (with p1==p2).

            ci = new cdint_t[2*(nend - nstart)];
            int j = 0;
            for (int i = nstart; i < nend; i++) {
                ci[j].val = m_segs[i]->p1.y;
                ci[j].code = m_segs[i]->ecode;
                ci[j].end = false;
                j++;
                ci[j].val = m_segs[i]->p2.y;
                ci[j].code = m_segs[i]->ecode;
                ci[j].end = true;
                j++;
            }
            std::sort(ci, ci + j, ci_cmp);
        }
        else {
            int yval = m_segs[nstart]->p1.y;

            int oppcnt = 0;
            while (nend < m_size && m_segs[nend]->p1.y == yval) {
                if (m_segs[nend]->ecode != c)
                    oppcnt++;
                nend++;
            }
            if (!oppcnt) {
                nstart = nend;
                continue;
            }

            // This scan line contains both code types, we need to remove the
            // overlap regions.

            // The number of segs in the scan line is the same after clipping,
            // though some segs may be invalidated with p1==p2.

            ci = new cdint_t[2*(nend - nstart)];
            int j = 0;
            for (int i = nstart; i < nend; i++) {
                ci[j].val = m_segs[i]->p1.x;
                ci[j].code = m_segs[i]->ecode;
                ci[j].end = false;
                j++;
                ci[j].val = m_segs[i]->p2.x;
                ci[j].code = m_segs[i]->ecode;
                ci[j].end = true;
                j++;
            }
            std::sort(ci, ci + j, ci_cmp);
        }
        int j = 0;
        for (int i = nstart; i < nend; i++) {
            cdint_t &c1 = ci[j++];
            cdint_t &c2 = ci[j++];

            if (m_vert) {
                m_segs[i]->p1.y = c1.val;
                m_segs[i]->p2.y = c2.val;
            }
            else {
                m_segs[i]->p1.x = c1.val;
                m_segs[i]->p2.x = c2.val;
            }

            // Now for the tricky part: assigning the code.
            if (c1.val == c2.val)
                // Zero length segment, code doesn't matter.
                continue;

            if (c1.end) {
                if (c2.end) {
                    // end->end, c1 and c2 will have opposite code, take
                    // c2 code.
                    m_segs[i]->ecode = (ZEcode)c2.code;
                }
                else {
                    // end->start, c1 and c2 will have same code and this
                    // will represent a space between clipped-out segments,
                    // occupied by the opposite code.
                    m_segs[i]->ecode = (ZEcode)opp_code[c1.code];
                }
            }
            else {
                // start->end, c1 and c2 will have same code, simple case.
                // or
                // start->start, c1 and c2 will have opposite code, take
                // c1 code.
                m_segs[i]->ecode = (ZEcode)c1.code;
            }
        }
        delete [] ci;

        nstart = nend;
    }
    m_clipped = true;
}
// End of omdb_t functions.


// Add the segment vertices to the EdgeFinder database.
//
void
onmdb_t::add_vertices(EdgeFinder *ef)
{
    if (!nm_clipped)
        clip();
    tGEOfact<oseg_t>::gen gen(&nm_allocator);
    oseg_t *s;
    while ((s = gen.next()) != 0) {
        if (s->p1 == s->p2)
            continue;
        ef->insert_vtx(s);
    }
}


namespace {
    // p1-p2 and p3-p4 are colinear.  Return true if p1-p2 entirely
    // overlaps p3-p4.
    //
    bool
    contained_in(Point &p1, Point &p2, Point &p3, Point &p4)
    {
        if (abs(p1.x - p2.x) > abs(p1.y - p2.y)) {
            int xmin = p1.x;
            int xmax = p2.x;
            if (xmax < xmin)
                mmSwapInts(xmax, xmin);
            if (p3.x < xmin || p3.x > xmax || p4.x < xmin || p4.x > xmax)
                return (false);
        }
        else {
            int ymin = p1.y;
            int ymax = p2.y;
            if (ymax < ymin)
                mmSwapInts(ymax, ymin);
            if (p3.y < ymin || p3.y > ymax || p4.y < ymin || p4.y > ymax)
                return (false);
        }
        return (true);
    }
}


// Clip the non-Manhattan edges so that overlapping parts are removed.
//
void
onmdb_t::clip()
{
    if (nm_clipped)
        return;
    tGEOfact<oseg_t>::gen gen(&nm_allocator);
    tGEOfact<oseg_t>::gen gen1 = gen;
    oseg_t *s1;
    while ((s1 = gen.next()) != 0) {
        if (s1->p1 == s1->p2)
            continue;
        tGEOfact<oseg_t>::gen gen2 = gen;
        oseg_t *s2;
        while ((s2 = gen2.next()) != 0) {
            if (s2->ecode == s1->ecode)
                continue;
            if (s2->p1 == s2->p2)
                continue;
            Point p11(s1->p1);
            Point p12(s1->p2);
            Point p21(s2->p1);
            Point p22(s2->p2);

#ifdef EF_DEBUG
#if EF_DEBUG_VERBOSE > 2
            printf("---\n");
            printf("s1 %d, %d  %d, %d %d\n", p11.x, p11.y, p12.x, p12.y,
                s1->ecode);
            printf("s2 %d, %d  %d, %d %d\n", p21.x, p21.y, p22.x, p22.y,
                s2->ecode);
#endif
#endif

            if (cGEO::clip_colinear(p11, p12, p21, p22)) {
                // The following mess takes care of setting the correct
                // edge code.  The p11,...p22 values are ordered, however
                // s1,s2 are not.
                //    
                // The residual segments are p11-p12 and p21-p22.
                // Each of these will be contained in at least one of
                // s1,s2.  Equal endpoints mean that the segment has
                // been merged away (don't care about edge code in this
                // case).

                if (p11 != p12) {
                    if (p21 != p22) {
                        if (contained_in(s1->p1, s1->p2, p11, p12)) {
                            if (contained_in(s1->p1, s1->p2, p21, p22)) {
                                s1->p1 = p11;
                                s1->p2 = p12;
                                s2->p1 = p21;
                                s2->p2 = p22;
                                s2->ecode = s1->ecode;
                            }
                            else {
                                // s2->p1,2 contains p21,p22
                                s1->p1 = p11;
                                s1->p2 = p12;
                                s2->p1 = p21;
                                s2->p2 = p22;
                            }
                        }
                        else {
                            // s2->p1,2 contains p11,p12
                            if (contained_in(s2->p1, s2->p2, p21, p22)) {
                                s1->p1 = p11;
                                s1->p2 = p12;
                                s2->p1 = p21;
                                s2->p2 = p22;
                                s1->ecode = s2->ecode;
                            }
                            else {
                                // s1->p1,2 contains p21,p22
                                s1->p1 = p21;
                                s1->p2 = p22;
                                s2->p1 = p11;
                                s2->p2 = p12;
                            }
                        }
                    }
                    else {
                        if (contained_in(s1->p1, s1->p2, p11, p12)) {
                            s1->p1 = p11;
                            s1->p2 = p12;
                            s2->p1 = p21;
                            s2->p2 = p22;
                        }
                        else {
                            // s2->p1,2 contains p11,p12
                            s1->p1 = p21;
                            s1->p2 = p22;
                            s2->p1 = p11;
                            s2->p2 = p12;
                        }
                    }
                }
                else {
                    if (p21 != p22) {
                        if (contained_in(s1->p1, s1->p2, p21, p22)) {
                            s1->p1 = p21;
                            s1->p2 = p22;
                            s2->p1 = p11;
                            s2->p2 = p12;
                        }
                        else {
                            // s2->p1,2 contains p21,p22
                            s1->p1 = p11;
                            s1->p2 = p12;
                            s2->p1 = p21;
                            s2->p2 = p22;
                        }
                    }
                    else {
                        s1->p1 = p11;
                        s1->p2 = p12;
                        s2->p1 = p21;
                        s2->p2 = p22;
                    }
                }
                // s1 changed, have to do over
                gen = gen1;
#ifdef EF_DEBUG
#if EF_DEBUG_VERBOSE > 2
                printf("clipped\n");
                printf("s1 %d, %d  %d, %d %d\n", s1->p1.x, s1->p1.y, s1->p2.x,
                    s1->p2.y, s1->ecode);
                printf("s2 %d, %d  %d, %d %d\n", s2->p1.x, s2->p1.y, s2->p2.x,
                    s2->p2.y, s2->ecode);
#endif
#endif
                break;
            }
        }
        gen1 = gen;
    }
    nm_clipped = true;
}
// End of onmdb_t functions.


// Add a vertex for the segment.  The vertex represents the start
// point of the segment.
//
void
EdgeFinder::insert_vtx(oseg_t *s)
{
    ovtx_t *v = vtx_allocator.new_obj();
    v->set(s);

    ovtx_t *v0 = xytab->find(v->tab_x(), v->tab_y());
    if (!v0) {
        xytab->link(v, false);
        xytab = xytab->check_rehash();
        return;
    }
    v->set_dups(v0->dups());
    v0->set_dups(v);
}


// Return polygon points array and vertex count.  This will remove the
// points used from the database.  This function should be called
// until 0 is returned to obtain all of the polygons that correspond
// to the passed zoids.  On error, -1 is returned.
//
// The returned polygons will contain no holes; holes are returned as
// separate polygons.  This is NOT a general-purpose zoid->polygon
// function.
//
// By default, polys are wound clockwise, and holes will be wound
// counter-clockwise.  If the ccw flag is set, this is reversed.
//
int
EdgeFinder::get_poly(Point **ppts, bool ccw)
{
    *ppts = 0;
    if (!xytab)
        add_vertices();

#ifdef EF_DEBUG
#if EF_DEBUG_VERBOSE > 1
    print_vertices();
#endif
#endif

again:
    // Grab a vertex and remove it from the database.  This will seed
    // the polygon.  Any vertex should work.
    //
    tgen_t<ovtx_t> gen(xytab);
    ovtx_t *v;
    while ((v = gen.next()) != 0) {
        if (!v->dups()) {
            xytab->unlink(v);
            break;
        }
        ovtx_t *vx = v->dups();
        v->set_dups(vx->dups());
        vx->set_dups(0);
        v = vx;
        break;
    }
    if (!v)
        return (0);

    // Link together the extracted ovtx_ts until we get back to the
    // original coordinate.  This will form a polygon.

    ovtx_t *v0 = v;
    ovtx_t *ve = v;
    int xn = v->seg()->end_x();
    int yn = v->seg()->end_y();
#ifdef EF_DEBUG
#if EF_DEBUG_VERBOSE > 0
    printf("start: %d, %d  next: %d, %d\n", v->tab_x(), v->tab_y(), xn, yn);
#endif
#endif

    int cnt = 1;
    for (;;) {
        ovtx_t *vnxt = xytab->find(xn, yn);
        if (!vnxt) {
            Errs()->add_error("get_poly: vertex linking failed.");
#ifdef EF_DEBUG
            printf("vertex linking failed: %d, %d\n", xn, yn);
#endif
            return (-1);
        }
        if (!vnxt->dups())
            xytab->unlink(vnxt);
        else {
            ovtx_t *vv = extract(vnxt, v);
            if (vv)
                vnxt = vv;
            else {
                // "can't happen"
                Errs()->add_error("get_poly: vertex id failed.");
                return (-1);
            }
        }
        cnt++;
        v = vnxt;
        v->set_tab_next(0);
        ve->set_tab_next(v);
        ve = ve->tab_next();
        xn = ve->seg()->end_x();
        yn = ve->seg()->end_y();
#ifdef EF_DEBUG
#if EF_DEBUG_VERBOSE > 0
        printf("%d, %d  next: %d, %d\n", ve->tab_x(), ve->tab_y(), xn, yn);
#endif
#endif
        if (xn == v0->tab_x() && yn == v0->tab_y())
            break;
    }
    if (cnt > 2) {
        Point *pts = new Point[cnt + 1];
        cnt = 0;
        for (v = v0; v; v = v->tab_next())
            pts[cnt++].set(v->tab_x(), v->tab_y());
        pts[cnt++] = pts[0];

        Poly po(cnt, pts);
        // valid() removes inline verts, etc.
        if (po.valid()) {
            if (ccw)
                po.reverse();
            *ppts = po.points;
            return (po.numpts);
        }
        delete [] pts;
#ifdef EF_DEBUG
        printf("found invalid polygon, continuing\n");
#endif
        goto again;
    }
#ifdef EF_DEBUG
    printf("found degenerate polygon, continuing\n");
#endif
    goto again;
}


namespace {
    // Return monotonically increasing function of the angle.  This is
    // (hopefully) faster than an atan2 call.  Return range is [0 - 4).
    //
    double angx(Point &p1, Point &p2, Point &p3)
    {
        double dx1 = p1.x - p2.x;
        double dy1 = p1.y - p2.y;
        double dx3 = p3.x - p2.x;
        double dy3 = p3.y - p2.y;

        double dd = sqrt((dx1*dx1 + dy1*dy1)*(dx3*dx3 + dy3*dy3));
        double cross = (dx1*dy3 - dy1*dx3)/dd;
        double dot = (dx1*dx3 + dy1*dy3);

        if (cross > 0.0) {
            if (dot > 0.0)
                return (cross);
            else
                return (2.0 - cross);
        }
        else {
            if (dot > 0.0)
                return (2.0 - cross);
            else
                return (4.0 + cross);
        }
    }
}


// Remove and return the element in vlist that makes the sharpest
// right turn from vref.  We know that the polygon will be wound
// clockwise, so looking forward, the fill is on the right.
//
ovtx_t *
EdgeFinder::extract(ovtx_t *vlist, ovtx_t *vref)
{
    double amin = 1000.0;
    Point_c p1(vref->tab_x(), vref->tab_y());
    Point_c p2(vlist->tab_x(), vlist->tab_y());
    ovtx_t *v0 = 0;
    for (ovtx_t *v = vlist; v; v = v->dups()) {
        Point_c p3(v->seg()->end_x(), v->seg()->end_y());
        double ax = angx(p1, p2, p3);
        if (ax < amin) {
            amin = ax;
            v0 = v;
        }
    }
    if (v0 == vlist) {
        xytab->unlink(vlist);
        ovtx_t *vx = vlist->dups();
        vlist->set_dups(0);
        xytab->link(vx);
    }
    else {
        for (ovtx_t *v = vlist; v; v = v->dups()) {
            if (v->dups() == v0) {
                v->set_dups(v0->dups());
                v0->set_dups(0);
                break;
            }
        }
    }
    return (v0);
}


// For debugging.
//
void
EdgeFinder::print_vertices()
{
    const char *flgs = "LTRB";
    tgen_t<ovtx_t> gen(xytab);
    ovtx_t *v;
    while ((v = gen.next()) != 0) {
        printf("At x=%d y=%d\n", v->tab_x(), v->tab_y());
        for (ovtx_t *vx = v; vx; vx = vx->dups()) {
            int xn = vx->seg()->end_x();
            int yn = vx->seg()->end_y();

            if (vx->tab_x() != vx->seg()->start_x() ||
                    vx->tab_y() != vx->seg()->start_y())
                printf("BAD: vertex not equal to start point!\n");

            printf(" nx=%d ny=%d %c\n", xn, yn, flgs[vx->seg()->ecode]);
        }
    }
}

