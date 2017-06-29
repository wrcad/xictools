
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
 $Id: geo_zlist.cc,v 1.83 2016/04/03 21:30:13 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "geo_ylist.h"
#include "geo_zgroup.h"
#include "geo_linedb.h"
#include <algorithm>


int Zlist::JoinMaxVerts = DEF_JoinMaxVerts;
int Zlist::JoinMaxGroup = DEF_JoinMaxGroup;
int Zlist::JoinMaxQueue = DEF_JoinMaxQueue;
bool Zlist::JoinBreakClean = false;
bool Zlist::JoinSplitWires = false;


//------------------------------------------------------------------------
// Zlist Functions
//------------------------------------------------------------------------


// Static function.
// Fill in the bounding box of the zoid list
//
void
Zlist::BB(const Zlist *z0, BBox &tBB)
{
    tBB = CDnullBB;
    for (const Zlist *z = z0; z; z = z->next) {
        int left = z->Z.minleft();
        if (left < tBB.left)
            tBB.left = left;
        if (z->Z.yl < tBB.bottom)
            tBB.bottom = z->Z.yl;
        int right = z->Z.maxright();
        if (right > tBB.right)
            tBB.right = right;
        if (z->Z.yu > tBB.top)
            tBB.top = z->Z.yu;
    }
}


// Static function.
// Function to copy a Zlist.
//
Zlist *
Zlist::copy(const Zlist *thiszl)
{
    Zlist *z0 = 0, *ze = 0;
    for (const Zlist *zl = thiszl; zl; zl = zl->next) {
        if (!z0)
            z0 = ze = new Zlist(&zl->Z);
        else {
            ze->next = new Zlist(&zl->Z);
            ze = ze->next;
        }
    }
    return (z0);
}


// Static function.
// Remove the sub-dimensional elements.
//
Zlist *
Zlist::filter_slivers(Zlist *thiszl, int d)
{
    Zlist *zl = thiszl, *zp = 0, *zn;
    for (Zlist *z = zl; z; z = zn) {
        zn = z->next;
        if (z->Z.is_sliver(d)) {
            if (!zp)
                zl = zn;
            else
                zp->next = zn;
            delete z;
            continue;
        }
        zp = z;
    }
    return (zl);
}


// Static function.
// Remove the sub-dimensional elements, using the criteria for DRC.
//
Zlist *
Zlist::filter_drc_slivers(Zlist *thiszl, int d)
{
    Zlist *zl = thiszl, *zp = 0, *zn;
    for (Zlist *z = zl; z; z = zn) {
        zn = z->next;
        if (z->Z.is_drc_sliver(d)) {
            if (!zp)
                zl = zn;
            else
                zp->next = zn;
            delete z;
            continue;
        }
        zp = z;
    }
    return (zl);
}


namespace {
    // Default sort - ascending in yl, min(xll, xul).
    //
    inline bool
    zcmp0(const Zlist *zl1, const Zlist *zl2)
    {
        const Zoid *z1 = &zl1->Z;
        const Zoid *z2 = &zl2->Z;
        if (z1->yl < z2->yl)
            return (true);
        if (z1->yl > z2->yl)
            return (false);
        int m1 = mmMin(z1->xll, z1->xul);
        int m2 = mmMin(z2->xll, z2->xul);
        return (m1 < m2);
    }


    // Descending in yu, ascending in xul then xll, etc.  Used for Ylist.
    //
    inline bool
    zcmp1(const Zlist *zl1, const Zlist *zl2)
    {
        const Zoid *z1 = &zl1->Z;
        const Zoid *z2 = &zl2->Z;
        return (z1->zcmp(z2) < 0);
    }
}


// Static function.
//
Zlist *
Zlist::sort(Zlist *thiszl, int mode)
{
    Zlist *z0 = thiszl;
    if (z0 && z0->next) {
        int len = 0;
        for (Zlist *z = z0; z; z = z->next, len++) ;
        if (len == 2) {
            if (mode == 1) {
                if (zcmp1(z0, z0->next))
                    return (z0);
            }
            else {
                if (zcmp0(z0, z0->next))
                    return (z0);
            }
            Zlist *zt = z0->next;
            zt->next = z0;
            z0->next = 0;
            return (zt);
        }
        Zlist **tz = new Zlist*[len];
        int i = 0;
        for (Zlist *z = z0; z; z = z->next, i++)
            tz[i] = z;
        if (mode == 1)
            std::sort(tz, tz + len, zcmp1);
        else
            std::sort(tz, tz + len, zcmp0);
        len--;
        for (i = 0; i < len; i++)
            tz[i]->next = tz[i+1];
        tz[i]->next = 0;
        z0 = tz[0];
        delete [] tz;
    }
    return (z0);
}


// Static function.
// Expand or shrink a list of zoids, 'this' is untouched.
// On exception: 'this' is untouched.
//
//
Zlist *
Zlist::bloat(const Zlist *thiszl, int delta, int blflags) throw (XIrt)
{
    if (!thiszl)
        return (0);
    if (delta == 0)
        return (copy(thiszl));

    int blmode = blflags & BL_MODE_MASK;

    if (blmode == DRC_BLOAT) {
        // mode == 3
        // The old slow algorithm, but takes into account unselected
        // objects.
        XIrt retp = XIok;
        Zlist *zl = GEO()->ifBloatList(thiszl, delta, (blflags & BL_EDGE_ONLY),
            &retp);
        if (retp != XIok)
            throw (retp);
        return (zl);
    }

    if (blflags & BL_OLD_MODES) {
        if (delta > 0) {
            // Just have to OR-in the edges.
            Zlist *z0;
            try {
                if (blmode == 0)
                    z0 = halo(thiszl, delta);
                else if (blmode == 1)
                    // This method should be faster, but not recommended
                    // for non-Manhattan.
                    z0 = edges(thiszl, delta);
                else if (blmode == 2)
                    // This method creates a wire from the vertex list
                    // of each maximal polygon, then either joins the
                    // wire or clips the wire to accomplish the bloat.
                    // If delta < 0, we invert first, using edges around
                    // the clear areas.  Otherwise, there are problems
                    // with polygons with holes.
                    z0 = wire_edges(thiszl, 2*delta);
                else
                    z0 = halo(thiszl, delta);
            }
            catch (XIrt) {
                throw;
            }
            Zlist *zx = copy(thiszl);
            XIrt ret = zl_or(&z0, zx);
            if (ret != XIok)
                throw (ret);
            return (z0);
        }
        else {
            // Find the bounding box and expand by abs(delta), and
            // clip out the poly areas (effectively inverting).
            BBox tBB;
            BB(thiszl, tBB);
            tBB.bloat(-delta);
            Zlist *z1 = copy(thiszl);
            Zlist *za = new Zlist(&tBB);
            XIrt ret = zl_andnot(&za, z1);
            if (ret != XIok)
                throw (ret);

            try {
                za = repartition(za);
            }
            catch (XIrt) {
                throw;
            }

            // Find halo zoids around inverted area.
            Zlist *z0;
            try {
                if (blmode == 0)
                    z0 = halo(za, delta);
                else if (blmode == 1)
                    z0 = edges(za, delta);
                else if (blmode == 2)
                    z0 = wire_edges(za, 2*delta);
                else
                    z0 = halo(za, delta);
                free(za);
            }
            catch (XIrt) {
                free(za);
                throw;
            }

            // Subtract halo from original poly area.
            za = copy(thiszl);
            ret = zl_andnot(&za, z0);
            if (ret != XIok)
                throw (ret);
            return (za);
        }
    }

    Zlist *zt = copy(thiszl);

    // BL_NO_MERGE_OUT suppresses all merging if BL_EDGE_ONLY.
    if (blflags & BL_NO_MERGE_OUT) {
        if (!(blflags & BL_EDGE_ONLY))
            blflags &= ~BL_NO_MERGE_OUT;
    }

    if (blflags & BL_NO_GROUP) {
        if (!(blflags & BL_NO_MERGE_IN)) {
            try {
                zt = repartition(zt);
            }
            catch (XIrt) {
                throw;
            }
        }
        Zlist *z0 = zt->ext_zoids(delta, blflags);

        if (blflags & BL_EDGE_ONLY) {
            free(zt);
            zt = z0;
            // The returned list has had repartition called, unless
            // BL_NO_MERGE_OUT is set,
        }
        else if (delta > 0) {
            XIrt ret = zl_or(&zt, z0);
            if (ret != XIok)
                throw (ret);
        }
        else {
            if (blflags & BL_SCALE_FIX) {
                try {
                    expand_by_2(zt);
                    expand_by_2(z0);
                    XIrt ret = zl_andnot(&zt, z0);
                    if (ret != XIok)
                        throw (ret);
                    zt = repartition(zt);
                    zt = shrink_by_2(zt);
                }
                catch (XIrt) {
                    throw;
                }
            }
            else {
                XIrt ret = zl_andnot(&zt, z0);
                if (ret != XIok)
                    throw (ret);
                try {
                    zt = repartition(zt);
                }
                catch (XIrt) {
                    throw;
                }
            }
        }
    }
    else if (blflags & BL_EDGE_ONLY) {
        Zgroup *g = zt->group(0);
        for (int i = 0; i < g->num; i++) {
            Zlist *zx = g->list[i];
            if (!(blflags & BL_NO_MERGE_IN)) {
                try {
                    zx = repartition(zx);
                }
                catch (XIrt) {
                    g->list[i] = 0;
                    delete g;
                    throw;
                }
            }
            g->list[i] = zx->ext_zoids(delta, blflags);
            free(zx);
        }
        zt = g->zoids();
        if (!(blflags & BL_NO_MERGE_OUT)) {
            try {
                zt = repartition(zt);
            }
            catch (XIrt) {
                throw;
            }
        }
    }
    else if (delta > 0) {
        Zgroup *g = zt->group(0);
        for (int i = 0; i < g->num; i++) {
            Zlist *zx = g->list[i];
            if (!(blflags & BL_NO_MERGE_IN)) {
                try {
                    zx = repartition(zx);
                }
                catch (XIrt) {
                    g->list[i] = 0;
                    delete g;
                    throw;
                }
            }
            g->list[i] = zx->ext_zoids(delta, blflags);
            Zlist *zn = g->list[i];
            if (!zn)
                zn = zx;
            else {
                while (zn->next)
                    zn = zn->next;
                zn->next = zx;
            }
        }
        zt = g->zoids();
        try {
            zt = repartition(zt);
        }
        catch (XIrt) {
            throw;
        }
    }
    else {
        Zgroup *g = zt->group(0);
        for (int i = 0; i < g->num; i++) {
            Zlist *zx = g->list[i];
            if (!(blflags & BL_NO_MERGE_IN)) {
                try {
                    zx = repartition(zx);
                }
                catch (XIrt) {
                    g->list[i] = 0;
                    delete g;
                    throw;
                }
            }
            if (blflags & BL_SCALE_FIX) {
                try {
                    Zlist *zedg = zx->ext_zoids(delta, blflags);

                    expand_by_2(zedg);
                    expand_by_2(zx);
                    XIrt ret = zl_andnot(&zx, zedg);
                    if (ret != XIok) {
                        g->list[i] = 0;
                        delete g;
                        throw (ret);
                    }
                    zx = repartition(zx);
                    zx = shrink_by_2(zx);
                }
                catch (XIrt) {
                    g->list[i] = 0;
                    delete g;
                    throw;
                }
            }
            else {
                Zlist *zedg = zx->ext_zoids(delta, blflags);
                XIrt ret = zl_andnot(&zx, zedg);
                if (ret != XIok) {
                    g->list[i] = 0;
                    delete g;
                    throw (ret);
                }
                try {
                    zx = repartition(zx);
                }
                catch (XIrt) {
                    g->list[i] = 0;
                    delete g;
                    throw;
                }
            }
            g->list[i] = zx;
        }
        zt = g->zoids();
    }
    return (zt);
}


// Static function.
// Return a list representing the boundaries extending out from the
// polygons formed from this.  'this' is untouched.
//
Zlist *
Zlist::halo(const Zlist *thiszl, int delta) throw (XIrt)
{
    if (!thiszl)
        return (0);

    // Merge into maximal polys, no limits.
    int tg = JoinMaxGroup;
    int tv = JoinMaxVerts;
    JoinMaxGroup = 0;
    JoinMaxVerts = 0;
    PolyList *p0 = copy(thiszl)->to_poly_list();
    JoinMaxGroup = tg;
    JoinMaxVerts = tv;

    Zlist *z0 = 0, *ze = 0;
    // ACTUNG! Poly::halo assumes clockwise winding, i.e., toPolyList
    // must always produce clockwise-wound polys (which is true).
    for (PolyList *p = p0; p; p = p->next) {
        Zlist *zl = p->po.halo(abs(delta));
        if (!z0)
            z0 = ze = zl;
        else {
            while (ze->next)
                ze = ze->next;
            ze->next = zl;
        }
    }
    p0->free();

    try {
        z0 = repartition(z0);

        // Clip out original poly area, needed for internal edges.
        XIrt ret = zl_andnot(&z0, copy(thiszl));
        if (ret != XIok)
            throw (ret);
        return (z0);
    }
    catch (XIrt) {
        throw;
    }
}


// Static function.
// Return a list of zoids that cover the edges of each zoid in the list
// extending +/- dim normal to the edge.  Not really recommended for
// non-Manhattan.  'this' is untouched.
//
Zlist *
Zlist::edges(const Zlist *thiszl, int dim) throw (XIrt)
{
    if (dim == 0)
        return (copy(thiszl));
    int d = abs(dim);
    Zlist *z0 = 0;
    for (const Zlist *z = thiszl; z; z = z->next) {

        int yut = z->Z.yu + d;
        int yub = z->Z.yu - d;
        int ylt = z->Z.yl + d;
        int ylb = z->Z.yl - d;

        if (ylt >= yub)
            z0 = new Zlist(z->Z.xll - d, z->Z.xlr + d, ylb,
                z->Z.xul - d, z->Z.xur + d, yut, z0);
        else {
            if (z->Z.xll == z->Z.xul) {
                if (z->Z.xlr == z->Z.xur) {
                    z0 = new Zlist(z->Z.xll - d, ylt, z->Z.xul + d,
                        yub, z0);
                    z0 = new Zlist(z->Z.xul - d, yub, z->Z.xur + d,
                        yut, z0);
                    z0 = new Zlist(z->Z.xlr - d, ylt, z->Z.xur + d,
                        yub, z0);
                    z0 = new Zlist(z->Z.xll - d, ylb, z->Z.xlr + d,
                        ylt, z0);
                }
                else {
                    double sr = z->Z.slope_right();
                    double dxr = d*sqrt(1 + sr*sr);
                    double dr = d*sr;

                    z0 = new Zlist(z->Z.xll - d, ylt, z->Z.xul + d,
                        yub, z0);
                    z0 = new Zlist(z->Z.xul - d,
                        mmRnd(z->Z.xur - dr + dxr), yub,
                        z->Z.xul - d, z->Z.xur + d, yut, z0);
                    z0 = new Zlist(mmRnd(z->Z.xlr + dr - dxr),
                        mmRnd(z->Z.xlr + dr + dxr), ylt,
                        mmRnd(z->Z.xur - dr - dxr),
                        mmRnd(z->Z.xur - dr + dxr), yub, z0);
                    z0 = new Zlist(z->Z.xll - d, z->Z.xlr + d, ylb,
                        z->Z.xll - d, mmRnd(z->Z.xlr + dr + dxr), ylt, z0);
                }
            }
            else {
                if (z->Z.xlr == z->Z.xur) {
                    double sl = z->Z.slope_left();
                    double dxl = d*sqrt(1 + sl*sl);
                    double dl = d*sl;

                    z0 = new Zlist(mmRnd(z->Z.xll + dl - dxl),
                        mmRnd(z->Z.xll + dl + dxl), ylt,
                        mmRnd(z->Z.xul - dl - dxl),
                        mmRnd(z->Z.xul - dl + dxl), yub, z0);
                    z0 = new Zlist(mmRnd(z->Z.xul - dl - dxl),
                        z->Z.xur + d, yub,
                        z->Z.xul - d, z->Z.xur + d, yut, z0);
                    z0 = new Zlist(z->Z.xlr - d, ylt,
                        z->Z.xur + d, yub, z0);
                    z0 = new Zlist(z->Z.xll - d, z->Z.xlr + d, ylb,
                        mmRnd(z->Z.xll + dl - dxl), z->Z.xlr + d, ylt, z0);
                }
                else {
                    double sr = z->Z.slope_right();
                    double sl = z->Z.slope_left();
                    double dxl = d*sqrt(1 + sl*sl);
                    double dxr = d*sqrt(1 + sr*sr);
                    double dl = d*sl;
                    double dr = d*sr;

                    z0 = new Zlist(mmRnd(z->Z.xll + dl - dxl),
                        mmRnd(z->Z.xll + dl + dxl), ylt,
                        mmRnd(z->Z.xul - dl - dxl),
                        mmRnd(z->Z.xul - dl + dxl), yub, z0);
                    z0 = new Zlist(mmRnd(z->Z.xul - dl - dxl),
                        mmRnd(z->Z.xur - dr + dxr), yub,
                        z->Z.xul - d, z->Z.xur + d, yut, z0);
                    z0 = new Zlist(mmRnd(z->Z.xlr + dr - dxr),
                        mmRnd(z->Z.xlr + dr + dxr), ylt,
                        mmRnd(z->Z.xur - dr - dxr),
                        mmRnd(z->Z.xur - dr + dxr), yub, z0);
                    z0 = new Zlist(z->Z.xll - d, z->Z.xlr + d, ylb,
                        mmRnd(z->Z.xll + dl - dxl),
                        mmRnd(z->Z.xlr + dr + dxr), ylt, z0);
                }
            }
        }
    }
    try {
        z0 = repartition(z0);
        return (z0);
    }
    catch (XIrt) {
        throw;
    }
}


// Static function.
// Return a list of zoids that represent a wire along the edges of
// each joined polygon.  'this' is untouched.
//
Zlist *
Zlist::wire_edges(const Zlist *thiszl, int delta) throw (XIrt)
{
    if (!thiszl)
        return (0);

    // Merge into maximal polys, no limits.
    int tg = JoinMaxGroup;
    int tv = JoinMaxVerts;
    JoinMaxGroup = 0;
    JoinMaxVerts = 0;
    PolyList *p0 = copy(thiszl)->to_poly_list();
    JoinMaxGroup = tg;
    JoinMaxVerts = tv;

    // For each poly, create a wire around the edge.
    Zlist *z0 = 0, *ze = 0;
    PolyList *pn;
    for (PolyList *p = p0; p; p = pn) {
        pn = p->next;
        Point *pts = p->po.points;
        int num = p->po.numpts;
        Wire w(abs(delta), CDWIRE_FLUSH, num, pts);
        Zlist *zx = w.toZlist();
        delete p;
        if (!z0)
            z0 = ze = zx;
        else {
            while (ze->next)
                ze = ze->next;
            ze->next = zx;
        }
    }
    try {
        z0 = repartition(z0);
        return (z0);
    }
    catch (XIrt) {
        throw;
    }
}


namespace {
    inline int cvt(int x, int d)
    {
        return (x > 0 ? (x + (d >> 1))/d : (x - (d >> 1))/d);
    }
}


// Static function.
// Convert this (which is destroyed) to a Manhattan representation.
// The returned list consists of rectangles only.  Rectangles added
// from triangular parts have w and h <= mindim.
//
Zlist *
Zlist::manhattanize(Zlist *thiszl, int mindim, int mode)
{
   if (mindim < 10)
        mindim = 10;
    Zlist *z0 = 0, *z = thiszl;

    if (mode) {
        // This version first puts all coordinates on a grid defined by
        // mindim, then uses Bresenham's method to walk up the sides,
        // creating Manhattan zoids aa we go.

        while (z) {
            z->Z.yl = cvt(z->Z.yl, mindim);
            z->Z.yu = cvt(z->Z.yu, mindim);
            z->Z.xll = cvt(z->Z.xll, mindim);
            z->Z.xul = cvt(z->Z.xul, mindim);
            z->Z.xlr = cvt(z->Z.xlr, mindim);
            z->Z.xur = cvt(z->Z.xur, mindim);
            if (z->Z.is_bad()) {
                Zlist *zt = z->next;
                delete z;
                z = zt;
                continue;
            }

            int xll = z->Z.xll;
            int xlr = z->Z.xlr;

            int dy = z->Z.yu - z->Z.yl;
            int dx1 = z->Z.xul - z->Z.xll;
            int dx2 = z->Z.xlr - z->Z.xur;
            int s1 = 1;
            int s2 = -1;
            if (z->Z.xll >= z->Z.xul) { dx1 = -dx1; s1 = -1; }
            if (z->Z.xur >= z->Z.xlr) { dx2 = -dx2; s2 = 1; }
            int errterm1 = dx1/2;
            int errterm2 = dx2/2;

            Zoid tZ(xll, xlr, z->Z.yl, xll, xlr, z->Z.yu);

            for (int y = z->Z.yl; y <= z->Z.yu; y++) {
                if (tZ.xul != xll || tZ.xur != xlr) {
                    tZ.yu = y;
                    if (!tZ.is_bad()) {
                        z0 = new Zlist(&tZ, z0);
                        z0->Z.yl *= mindim;
                        z0->Z.yu *= mindim;
                        z0->Z.xll *= mindim;
                        z0->Z.xul *= mindim;
                        z0->Z.xlr *= mindim;
                        z0->Z.xur *= mindim;
                    }

                    tZ.xul = xll;
                    tZ.xur = xlr;
                    tZ.yl = y;
                    tZ.xll = xll;
                    tZ.xlr = xlr;
                }
                errterm1 += dx1;
                errterm2 += dx2;
                while (errterm1 > 0 && z->Z.xul != xll) {
                    errterm1 -= dy;
                    xll += s1;
                }
                while (errterm2 > 0 && xlr != z->Z.xur) {
                    errterm2 -= dy;
                    xlr += s2;
                }
            }
//XXX thiszl?
            if (thiszl->Z.yu != z->Z.yu) {
                tZ.yu = z->Z.yu;
                tZ.xul = z->Z.xul;
                tZ.xur = z->Z.xur;
                tZ.xll = z->Z.xul;
                tZ.xlr = z->Z.xur;
                if (!tZ.is_bad()) {
                    z0 = new Zlist(&tZ, z0);
                    z0->Z.yl *= mindim;
                    z0->Z.yu *= mindim;
                    z0->Z.xll *= mindim;
                    z0->Z.xul *= mindim;
                    z0->Z.xlr *= mindim;
                    z0->Z.xur *= mindim;
                }
            }

            Zlist *zt = z->next;
            delete z;
            z = zt;
        }
    }
    else {
        // This version decomposes the zoids into right-triangles, then
        // recursively cuts out the Manhattan parts.  The coordinates are
        // not forced to any grid, and the rectangular dimensions vary.

        while (z) {
            int xrmin = mmMin(z->Z.xlr, z->Z.xur);
            int xrmax = mmMax(z->Z.xlr, z->Z.xur);
            int xlmin = mmMin(z->Z.xll, z->Z.xul);
            int xlmax = mmMax(z->Z.xll, z->Z.xul);
            if (xrmin >= xlmax) {
                if (xrmin - xlmax >= mindim)
                    z0 = new Zlist(xlmax, xrmin, z->Z.yl, xlmax, xrmin,
                        z->Z.yu, z0);
                else if (((xrmin == xrmax && xlmin == xlmax) ||
                        xrmax - xrmin >= mindim ||
                        xlmax - xlmin >= mindim) && xrmin > xlmax)
                    // keep this if there will be an adjacent zoid, to avoid
                    // breaking continuity
                    z0 = new Zlist(xlmax, xrmin, z->Z.yl, xlmax, xrmin,
                        z->Z.yu, z0);
                if (xrmax - xrmin >= mindim) {
                    Zoid z1(xrmin, z->Z.xlr, z->Z.yl, xrmin, z->Z.xur,
                        z->Z.yu);
                    z1.rt_triang(&z0, mindim);
                }
                if (xlmax - xlmin >= mindim) {
                    Zoid z1(z->Z.xll, xlmax, z->Z.yl, z->Z.xul, xlmax,
                        z->Z.yu);
                    z1.rt_triang(&z0, mindim);
                }
                Zlist *zt = z->next;
                delete z;
                z = zt;
            }
            else {
                double sl = z->Z.slope_left();
                double sr = z->Z.slope_right();

                if (z->Z.xur - z->Z.xul < z->Z.xlr - z->Z.xll) {
                    int h;
                    if (z->Z.xur < z->Z.xll)
                        h = mmRnd((z->Z.xll - z->Z.xlr)/sr);
                    else
                        h = mmRnd((z->Z.xlr - z->Z.xll)/sl);
                    if (h < mindim) {
                        Zlist *zt = z->next;
                        delete z;
                        z = zt;
                        continue;
                    }

                    int xl = mmRnd(z->Z.xll + h*sl);
                    if (xl > z->Z.xlr)
                        xl = z->Z.xlr;
                    int xr = mmRnd(z->Z.xlr + h*sr);
                    if (xr < z->Z.xll)
                        xr = z->Z.xll;
                    Zlist *zx = new Zlist(z->Z.xll, z->Z.xlr, z->Z.yl,
                        xl, xr, z->Z.yl + h, 0);
                    z->Z.yl += h;
                    z->Z.xll = xl;
                    z->Z.xlr = xr;
                    zx->next = z;
                    z = zx;
                }
                else {
                    int h;
                    if (z->Z.xul > z->Z.xlr)
                        h = mmRnd((z->Z.xur - z->Z.xul)/sr);
                    else
                        h = mmRnd((z->Z.xul - z->Z.xur)/sl);
                    if (h < mindim) {
                        Zlist *zt = z->next;
                        delete z;
                        z = zt;
                        continue;
                    }

                    int xl = mmRnd(z->Z.xul - h*sl);
                    if (xl > z->Z.xur)
                        xl = z->Z.xur;
                    int xr = mmRnd(z->Z.xur - h*sr);
                    if (xr < z->Z.xul)
                        xr = z->Z.xul;
                    Zlist *zx = new Zlist(xl, xr, z->Z.yu - h, z->Z.xul,
                        z->Z.xur, z->Z.yu, 0);
                    z->Z.yu -= h;
                    z->Z.xul = xl;
                    z->Z.xur = xr;
                    zx->next = z;
                    z = zx;
                }
            }
        }
    }
    return (z0);
}


// Static function.
// Create a new Zlist that is a transformed copy of this (this is not
// affected).  The zoids are converted to polygons, transformed, and
// split into a new zoid list.
//
Zlist *
Zlist::transform(const Zlist *thiszl, cTfmStack *tstk)
{
    PolyList *p0 = PolyList::new_poly_list(thiszl, false);
    for (PolyList *p = p0; p; p = p->next)
        tstk->TPath(p->po.numpts, p->po.points);

    Zlist *z0 = 0, *ze = 0;
    for (PolyList *p = p0; p; p = p->next) {
        Zlist *zl = p->po.toZlist();
        if (!z0)
            z0 = ze = zl;
        else {
            while (ze->next)
                ze = ze->next;
            ze->next = zl;
        }
    }

    p0->free();
    return (z0);
}


// This will attempt to find the linewidth of collection of
// trapezoids.  The algorithm is to create a histogram of the height
// and mid-height width of each zoid.  The value with the longest run
// length is taken as the linewidth.  If there is a tie, the smaller
// value is chosen.
//
int
Zlist::linewidth() const
{
    {
        const Zlist *zt = this;
        if (!zt)
            return (0);
    }

    if (!next) {
        int h = Z.yu - Z.yl;
        int w = ((Z.xur + Z.xlr) - (Z.xul + Z.xll))/2;
        return (mmMin(w, h));
    }

    SymTab *tab = new SymTab(false, false);
    for (const Zlist *z = this; z; z = z->next) {
        unsigned long ht = z->Z.yu - z->Z.yl;
        unsigned long wd = ((z->Z.xur + z->Z.xlr) - (z->Z.xul + z->Z.xll))/2;
        SymTabEnt *h = tab->get_ent(ht);
        if (!h)
            tab->add(ht, (void*)wd, false);
        else
            h->stData = (void*)(wd + (long)h->stData);

        h = tab->get_ent(wd);
        if (!h)
            tab->add(wd, (void*)ht, false);
        else
            h->stData = (void*)(ht + (long)h->stData);
    }

    SymTabGen gen(tab);
    SymTabEnt *h;
    int hstmax = 0;
    int hstval = 0;
    while ((h = gen.next()) != 0) {
        if ((long)h->stData > hstmax) {
            hstmax = (int)(long)h->stData;
            hstval = (int)(long)h->stTag;
        }
        else if ((long)h->stData == hstmax) {
            if ((long)h->stTag < hstval)
                hstval = (int)(long)h->stTag;
        }
    }
    delete tab;
    return (hstval);
}


// Compute the sum of the lengths of external edges of the trapezoid
// list.  The external edges are the parts of edges next to empty
// space.  This assumes the trapezoids don't overlap!  (repartition()
// called).  If a BBox is passed, the segments will be clipped to this
// BB, and segments that lie on the BB will contribute 1/2 length
// (Praesagus uses this).
//
double
Zlist::ext_perim(const BBox *psgBB) const
{
    {
        const Zlist *zt = this;
        if (!zt)
            return (0);
    }

    if (!next) {
        double perim = 0.0;
        Point_c p1(Z.xll, Z.yl);
        Point_c p2(Z.xul, Z.yu);
        if (!psgBB || !cGEO::line_clip(&p1.x, &p1.y, &p2.x, &p2.y, psgBB)) {
            if (Z.xll == Z.xul) {
                unsigned int diff = abs(p1.y - p2.y);
                if (psgBB && (p1.x == psgBB->left || p1.x == psgBB->right)) {
                    perim += (diff >> 1);
                    if (diff & 1)
                        perim += 0.5;
                }
                else
                    perim += diff;
            }
            else {
                double dx = p1.x - p2.x;
                double dy = p1.y - p2.y;
                perim += sqrt(dx*dx + dy*dy);
            }
        }

        p1.set(Z.xlr, Z.yl);
        p2.set(Z.xur, Z.yu);
        if (!psgBB || !cGEO::line_clip(&p1.x, &p1.y, &p2.x, &p2.y, psgBB)) {
            if (Z.xlr == Z.xur) {
                unsigned int diff = abs(p1.y - p2.y);
                if (psgBB && (p1.x == psgBB->left || p1.x == psgBB->right)) {
                    perim += (diff >> 1);
                    if (diff & 1)
                        perim += 0.5;
                }
                else
                    perim += diff;
            }
            else {
                double dx = p1.x - p2.x;
                double dy = p1.y - p2.y;
                perim += sqrt(dx*dx + dy*dy);
            }
        }

        p1.set(Z.xul, Z.yu);
        p2.set(Z.xur, Z.yu);
        if (!psgBB || !cGEO::line_clip(&p1.x, &p1.y, &p2.x, &p2.y, psgBB)) {
            unsigned int diff = abs(p1.x - p2.x);
            if (psgBB && (p1.y == psgBB->bottom || p1.y == psgBB->top)) {
                perim += (diff >> 1);
                if (diff & 1)
                    perim += 0.5;
            }
            else
                perim += diff;
        }

        p1.set(Z.xll, Z.yl);
        p2.set(Z.xlr, Z.yl);
        if (!psgBB || !cGEO::line_clip(&p1.x, &p1.y, &p2.x, &p2.y, psgBB)) {
            unsigned int diff = abs(p1.x - p2.x);
            if (psgBB && (p1.y == psgBB->bottom || p1.y == psgBB->top)) {
                perim += (diff >> 1);
                if (diff & 1)
                    perim += 0.5;
            }
            else
                perim += diff;
        }
        return (perim);
    }
    linedb_t ldb;
    for (const Zlist *z = this; z; z = z->next)
        ldb.add(&z->Z, psgBB);
    return (ldb.perim(psgBB));
}


// Return a list of the external edges of the trapezoids.  The
// external edges are the parts of edges next to empty space.  This
// assumes the trapezoids don't overlap!  (repartition() called).
//
edg_t *
Zlist::ext_edges() const
{
    {
        const Zlist *zt = this;
        if (!zt)
            return (0);
    }

    linedb_t ldb;
    for (const Zlist *z = this; z; z = z->next)
        ldb.add(&z->Z);
    return (ldb.edges());
}


// Return a list of the external edges of the trapezoids, as
// trapezoids bloated by delta.  The external edges are the parts of
// edges next to empty space.  This assumes the trapezoids don't
// overlap! (repartition() called).
//
Zlist *
Zlist::ext_zoids(int delta, int mode) const
{
    {
        const Zlist *zt = this;
        if (!zt)
            return (0);
    }

    linedb_t ldb;
    for (const Zlist *z = this; z; z = z->next)
        ldb.add(&z->Z);
    return (ldb.zoids(delta, mode));
}


// Group the list according to connectivity.  If max_in_grp is nonzero,
// groups are *approximately* limited to this number of entries.  In this
// case, the groups are not necessarily disjoint.  'this' is consumed.
//
Zgroup *
Zlist::group(int max_in_grp)
{
    Zlist *zt = this;
    if (!zt || !zt->next) {
        Zgroup *g = new Zgroup;
        if (zt) {
            g->num = 1;
            g->list = new Zlist*[2];
            g->list[0] = zt;
            g->list[1] = 0;
        }
        return (g);
    }
    return ((new Ylist(zt))->group(max_in_grp));
}


// Create a layer from name, and add the zoid list to sdesc.
// The zoidlist is freed.  The ttl_flags recognized are:
//
//   TTLinternal    : Create a CDLinternal layer, otherwise CDLnormal.
//
//   TTLnoinsert    : Don't add to layer table, put new layer in
//                    removed list instead.
//
//   TTLjoin        : Join joids into polygons on new layer.
//
// The first two flags apply only when a new layer is created here.
//
CDl *
Zlist::to_temp_layer(const char *name, int ttl_flags, CDs *sdesc, XIrt *retp)
{
    *retp = XIbad;
    if (!sdesc)
        return (0);

    // Careful:  addNewLayer will create a new layer with an internal
    // name if the name is already in use.  Reuse the existing layer
    // if it exists.

    CDl *ld = CDldb()->findLayer(name, Physical);
    if (!ld) {
        ld = CDldb()->addNewLayer(name, Physical,
            (ttl_flags & TTLinternal) ? CDLinternal : CDLnormal, -1,
            (ttl_flags & TTLnoinsert));
    }
    Zlist *zthis = this;
    if (ld && zthis && sdesc) {
        if (ttl_flags & TTLjoin)
            *retp = to_poly_add(sdesc, ld, false);
        else {
            add(sdesc, ld, false);
            free(this);
            *retp = XIok;
        }
    }
    else
        free(this);
    return (ld);
}


// Create a polygon from the zoids which touch (recursively) the top
// left zoid in the list.  The zoids used are removed from the list.
// It is assumed that repartition() has been called!
//
Zlist *
Zlist::to_poly(Point **pts, int *num)
{
    Zlist *zt = this;
    if (!zt) {
        *pts = 0;
        *num = 0;
        return (0);
    }
    Ylist *y = new Ylist(zt);
    y = y->to_poly(pts, num, JoinMaxVerts);
    return (y->to_zlist());
}


// Convert the zoid list to a list of polygons.  The zoid list is consumed.
//
PolyList *
Zlist::to_poly_list()
{
    {
        Zlist *zt = this;
        if (!zt)
            return (0);
    }

    Ylist *y = new Ylist(this);
    Zgroup *g = y->group(JoinMaxGroup);
    if (!g)
        return (0);

    PolyList *p0 = 0;
    for (int i = 0; i < g->num; i++) {
        PolyList *pn = g->to_poly_list(i, JoinMaxVerts);
        if (pn) {
            PolyList *px = pn;
            while (px->next)
                px = px->next;
            px->next = p0;
            p0 = pn;
        }
    }

    delete g;
    return (p0);
}


// Convert the zoid list to polygons, which are added to the database.
// This frees the zoid list.
//
XIrt
Zlist::to_poly_add(CDs *sdesc, CDl *ld, bool undoable, const cTfmStack *tstk,
    bool use_merge)
{
    {
        Zlist *zt = this;
        if (!zt)
            return (XIok);
    }
    if (!sdesc || !ld)
        return (XIbad);

    Ylist *y = new Ylist(this);
    Zgroup *g = y->group(JoinMaxGroup);
    if (!g)
        return (XIok);

    Errs()->init_error();
    for (int i = 0; i < g->num; i++) {
        PolyList *p0 = g->to_poly_list(i, JoinMaxVerts);
        if (p0) {
            if (sdesc->addToDb(p0, ld, undoable, 0, tstk, use_merge) != CDok)
                GEO()->ifInfoMessage(IFMSG_LOG_ERR,
                    Errs()->get_error());
            p0->free();
        }
    }
    delete g;
    return (XIok);
}


// Convert the zoid list to a list of object descriptors.  The new objects
// are not added to the database (the CDisCopy flag is set), the zoid list
// is freed.  The objects are linked along the next_odesc() pointer.
//
CDo *
Zlist::to_obj_list(CDl *ld, bool nomerge)
{
    {
        Zlist *zt = this;
        if (!zt)
            return (0);
    }

    CDo *od0 = 0;
    if (nomerge) {
        for (Zlist *z = this; z; z = z->next) {
            if (z->Z.is_rect()) {
                BBox tBB(z->Z.xll, z->Z.yl, z->Z.xlr, z->Z.yu);
                if (tBB.valid()) {
                    CDo *od = new CDo(ld, &tBB);
                    od->set_copy(true);
                    od->set_next_odesc(od0);
                    od0 = od;
                }
            }
            else {
                Poly po;
                if (z->Z.mkpoly(&po.points, &po.numpts, false)) {
                    CDpo *od = new CDpo(ld, &po);
                    od->set_copy(true);
                    od->set_next_odesc(od0);
                    od0 = od;
                }
            }
        }
    }
    else {
        Ylist *y = new Ylist(this);
        Zgroup *g = y->group(JoinMaxGroup);
        if (!g)
            return (0);

        for (int i = 0; i < g->num; i++) {
            PolyList *p0 = g->to_poly_list(i, JoinMaxVerts);
            if (p0) {
                CDo *od = p0->to_odesc(ld);
                if (od) {
                    CDo *on = od;
                    while (on->next_odesc())
                        on = on->next_odesc();
                    on->set_next_odesc(od0);
                    od0 = od;
                }
            }
        }
        delete g;
    }
    return (od0);
}


// Add the zoids as objects in the database.
//
void
Zlist::add(CDs *sdesc, CDl *ld, bool undoable, bool use_merge) const
{
    if (!sdesc || !ld)
        return;
    Errs()->init_error();
    for (const Zlist *z = this; z; z = z->next) {
        if (z->Z.xll == z->Z.xul && z->Z.xlr == z->Z.xur) {
            if (z->Z.xll == z->Z.xlr || z->Z.yl >= z->Z.yu)
                // bad zoid, ignore
                continue;
            BBox tBB;
            tBB.left = z->Z.xll;
            tBB.bottom = z->Z.yl;
            tBB.right = z->Z.xur;
            tBB.top = z->Z.yu;
            if (tBB.left == tBB.right || tBB.top == tBB.bottom)
                continue;
            CDo *newo;
            if (sdesc->makeBox(ld, &tBB, &newo) != CDok) {
                Errs()->add_error("makeBox failed");
                GEO()->ifInfoMessage(IFMSG_LOG_ERR, Errs()->get_error());
                continue;
            }
            if (undoable) {
                GEO()->ifRecordObjectChange(sdesc, 0, newo);
                if (use_merge && !sdesc->mergeBoxOrPoly(newo, true)) {
                    Errs()->add_error("mergeBoxOrPoly failed");
                    GEO()->ifInfoMessage(IFMSG_LOG_ERR, Errs()->get_error());
                    continue;
                }
            }
        }
        else {
            if (z->Z.xll > z->Z.xlr || z->Z.xul > z->Z.xur ||
                    (z->Z.xll == z->Z.xlr && z->Z.xul == z->Z.xur) ||
                    z->Z.yl >= z->Z.yu)
                // bad zoid, ignore
                continue;

            Poly poly;
            if (z->Z.mkpoly(&poly.points, &poly.numpts, false)) {
                CDpo *newo;
                if (sdesc->makePolygon(ld, &poly, &newo) != CDok) {
                    Errs()->add_error("makePolygon failed");
                    GEO()->ifInfoMessage(IFMSG_LOG_ERR, Errs()->get_error());
                    continue;
                }
                if (undoable) {
                    GEO()->ifRecordObjectChange(sdesc, 0, newo);
                    if (use_merge && !sdesc->mergeBoxOrPoly(newo, true)) {
                        Errs()->add_error("mergeBoxOrPoly failed");
                        GEO()->ifInfoMessage(IFMSG_LOG_ERR,
                            Errs()->get_error());
                        continue;
                    }
                }
            }
        }
    }
}


// Assume coordinates need +90 degree rotation.
//
void
Zlist::add_r(CDs *sdesc, CDl *ld, bool undoable, bool use_merge) const
{
    if (!sdesc || !ld)
        return;
    for (const Zlist *z = this; z; z = z->next) {
        if (z->Z.xll == z->Z.xul && z->Z.xlr == z->Z.xur) {
            if (z->Z.xll == z->Z.xlr || z->Z.yl >= z->Z.yu)
                // bad zoid, ignore
                continue;
            BBox tBB;
            tBB.left = z->Z.yl;
            tBB.bottom = -z->Z.xur;
            tBB.right = z->Z.yu;
            tBB.top = -z->Z.xll;
            if (tBB.left == tBB.right || tBB.top == tBB.bottom)
                continue;
            CDo *newo;
            if (sdesc->makeBox(ld, &tBB, &newo) != CDok) {
                Errs()->add_error("makeBox failed");
                GEO()->ifInfoMessage(IFMSG_LOG_ERR, Errs()->get_error());
                continue;
            }
            if (undoable) {
                GEO()->ifRecordObjectChange(sdesc, 0, newo);
                if (use_merge && !sdesc->mergeBoxOrPoly(newo, true)) {
                    Errs()->add_error("mergeBoxOrPoly failed");
                    GEO()->ifInfoMessage(IFMSG_LOG_ERR, Errs()->get_error());
                    continue;
                }
            }
        }
        else {
            if (z->Z.xll > z->Z.xlr || z->Z.xul > z->Z.xur ||
                    (z->Z.xll == z->Z.xlr && z->Z.xul == z->Z.xur) ||
                    z->Z.yl >= z->Z.yu)
                // bad zoid, ignore
                continue;

            Poly poly;
            if (z->Z.mkpoly(&poly.points, &poly.numpts, true)) {
                CDpo *newo;
                if (sdesc->makePolygon(ld, &poly, &newo) != CDok) {
                    Errs()->add_error("makePolygon failed");
                    GEO()->ifInfoMessage(IFMSG_LOG_ERR, Errs()->get_error());
                    continue;
                }
                if (undoable) {
                    GEO()->ifRecordObjectChange(sdesc, 0, newo);
                    if (use_merge && !sdesc->mergeBoxOrPoly(newo, true)) {
                        Errs()->add_error("mergeBoxOrPoly failed");
                        GEO()->ifInfoMessage(IFMSG_LOG_ERR,
                            Errs()->get_error());
                        continue;
                    }
                }
            }
        }
    }
}


// Rotate the trapezoid list by -90 degrees (x -> y, y -> -x) and
// create a new canonical trapezoid list.  This is freed.
//
Zlist *
Zlist::to_r()
{
    Zlist *z0 = 0, *zn;
    for (Zlist *z = this; z; z = zn) {
        zn = z->next;
        if (z->Z.is_rect())
            // Manhattan, easy case.
            z0 = new Zlist(-z->Z.yu, -z->Z.yl, z->Z.xll,
                -z->Z.yu, -z->Z.yl, z->Z.xlr, z0);
        else {
            Point p[4];
            p[0].set(-z->Z.yu, z->Z.xul);
            p[1].set(-z->Z.yl, z->Z.xll);
            p[2].set(-z->Z.yl, z->Z.xlr);
            p[3].set(-z->Z.yu, z->Z.xur);

            if (p[1].y < p[0].y) {
                Point pt = p[0];
                p[0] = p[1];
                p[1] = pt;
            }
            if (p[3].y < p[2].y) {
                Point pt = p[2];
                p[2] = p[3];
                p[3] = pt;
            }
            if (p[1].y <= p[2].y) {
                // No interpolation required.
                if (p[0].y != p[1].y)
                    z0 = new Zlist(p[0].x, p[0].x, p[0].y,
                        -z->Z.yu, -z->Z.yl, p[1].y, z0);
                if (p[1].y != p[2].y)
                    z0 = new Zlist(-z->Z.yu, -z->Z.yl, p[1].y,
                        -z->Z.yu, -z->Z.yl, p[2].y, z0);
                if (p[2].y != p[3].y)
                    z0 = new Zlist(-z->Z.yu, -z->Z.yl, p[2].y,
                        p[3].x, p[3].x, p[3].y, z0);
            }
            else {
                int dx1 = mmRnd((p[2].y - p[0].y)*(double)(z->Z.yu - z->Z.yl)/
                    (p[1].y - p[0].y));
                int dx2 = mmRnd((p[3].y - p[1].y)*(double)(z->Z.yu - z->Z.yl)/
                    (p[3].y - p[2].y));
                if (p[1].x > p[0].x) {
                    if (p[0].y != p[2].y)
                        z0 = new Zlist(p[0].x, p[0].x, p[0].y,
                            -z->Z.yu, -z->Z.yu + dx1, p[2].y, z0);
                    z0 = new Zlist(-z->Z.yu, -z->Z.yu + dx1, p[2].y,
                        -z->Z.yl - dx2, -z->Z.yl, p[1].y, z0);
                    if (p[1].y != p[3].y)
                        z0 = new Zlist(-z->Z.yl - dx2, -z->Z.yl, p[1].y,
                            p[3].x, p[3].x, p[3].y, z0);
                }
                else {
                    if (p[0].y != p[2].y)
                        z0 = new Zlist(p[0].x, p[0].x, p[0].y,
                            -z->Z.yl - dx1, -z->Z.yl, p[2].y, z0);
                    z0 = new Zlist(-z->Z.yl - dx1, -z->Z.yl, p[2].y,
                        -z->Z.yu, -z->Z.yu + dx2, p[1].y, z0);
                    if (p[1].y != p[3].y)
                        z0 = new Zlist(-z->Z.yu, -z->Z.yu + dx2, p[1].y,
                            p[3].x, p[3].x, p[3].y, z0);
                }
            }
        }
        delete z;
    }
    // Apply the canonicalization so that vertical partitioning will be
    // complete.
    return (repartition_ni(z0));
}


// Static function.
// Revert to default join parameters.
//
void
Zlist::reset_join_params()
{
    JoinMaxVerts = DEF_JoinMaxVerts;
    JoinMaxGroup = DEF_JoinMaxGroup;
    JoinMaxQueue = DEF_JoinMaxQueue;
    JoinBreakClean = false;
}

