
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
 $Id: geo_zoidclip.cc,v 1.36 2017/04/12 05:02:50 stevew Exp $
 *========================================================================*/

#include "geo.h"
#include "geo_zlist.h"
#include "cd.h"
#include "cd_const.h"
#include "cd_chkintr.h"
#include <sys/types.h>
#include <algorithm>


//#define ZC_DEBUG

#ifdef ZC_DEBUG
namespace {
    void test_clip_to(const Zlist*, const Zoid*, const Zoid*);
    bool test_clip_around(const Zoid*, const Zoid*, const Zoid*);
    void test_clip_around(const Zlist*, const Zoid*, const Zoid*);
}
#endif


//------------------------------------------------------------------------
// ZoidClipper
// A trapezoid clipping class.
//------------------------------------------------------------------------

namespace {
    struct ZoidClipper
    {
        ZoidClipper(const Zoid&, const Zoid&, bool = false);

        Zlist *clip_to();
        Zlist *clip_around();

        bool no_overlap()   { return (zc_no_overlap); }
        int  num_scans()    { return (zc_nscans); }
        const int *scans()  { return (zc_scans); }

        // Return a list of Zoids representing the intersection of z1
        // and z2.
        //
        static Zlist *clip_to(const Zoid &z1, const Zoid &z2)
            {
                ZoidClipper zclp(z1, z2);
                return (zclp.clip_to());
            }

        // Return a list of zoids representing the parts of z1 that do
        // not overlap z2.  If the zoids don't overlap, return 0 and
        // set no_ovlp.
        //
        static Zlist *clip_around(const Zoid &z1, const Zoid &z2,
            bool *no_ovlp)
            {
                ZoidClipper zclp(z1, z2);
                if (zclp.no_overlap()) {
                    *no_ovlp = true;
                    return (0);
                }
                *no_ovlp = false;
                return (zclp.clip_around());
            }

private:
        Zlist *clip_to_scan(int, int);
        Zlist *clip_around_scan(int, int);

        // Below we check intersection points between the sides.  In
        // general, the point will be off-grid, so we consider the y
        // values that straddle the intersection.  If at either value
        // the x values are equal, we'll take that point, otherwise we
        // take both points.  Scan values are only taken if they are
        // inside the min and max y values, which are the first two
        // entries of the scans array (before sort).

        void add_scan(int y)
            {
                if (y > zc_scans[0] && y < zc_scans[1])
                    zc_scans[zc_nscans++] = y;
            }

        void x1r2l()
            {
                double B = zc_z2sl - zc_z1sr;
                if (B == 0.0)
                    return;
                double A = Z1.xlr - zc_z1sr*Z1.yl - Z2.xll + zc_z2sl*Z2.yl;
                A /= B;

                int yb = (int)floor(A);
                int yt = yb+1;

                if (A - yb < 0.5) {
                    int xb1 = mmRnd(Z1.xlr + zc_z1sr*(yb - Z1.yl));
                    int xb2 = mmRnd(Z2.xll + zc_z2sl*(yb - Z2.yl));
                    if (xb1 == xb2) {
                        add_scan(yb);
                        return;
                    }
                }
                else {
                    int xt1 = mmRnd(Z1.xlr + zc_z1sr*(yt - Z1.yl));
                    int xt2 = mmRnd(Z2.xll + zc_z2sl*(yt - Z2.yl));
                    if (xt1 == xt2) {
                        add_scan(yt);
                        return;
                    }
                }
                add_scan(yb);
                add_scan(yt);
            }

        void x1l2r()
            {
                double B = zc_z1sl - zc_z2sr;
                if (B == 0.0)
                    return;
                double A = Z2.xlr - zc_z2sr*Z2.yl - Z1.xll + zc_z1sl*Z1.yl;
                A /= B;

                int yb = (int)floor(A);
                int yt = yb+1;

                if (A - yb < 0.5) {
                    int xb1 = mmRnd(Z1.xll + zc_z1sl*(yb - Z1.yl));
                    int xb2 = mmRnd(Z2.xlr + zc_z2sr*(yb - Z2.yl));
                    if (xb1 == xb2) {
                        add_scan(yb);
                        return;
                    }
                }
                else {
                    int xt1 = mmRnd(Z1.xll + zc_z1sl*(yt - Z1.yl));
                    int xt2 = mmRnd(Z2.xlr + zc_z2sr*(yt - Z2.yl));
                    if (xt1 == xt2) {
                        add_scan(yt);
                        return;
                    }
                }
                add_scan(yb);
                add_scan(yt);
            }

        void x1r2r()
            {
                double B = zc_z2sr - zc_z1sr;
                if (B == 0.0)
                    return;
                double A = Z1.xlr - zc_z1sr*Z1.yl - Z2.xlr + zc_z2sr*Z2.yl;
                A /= B;

                int yb = (int)floor(A);
                int yt = yb+1;

                if (A - yb < 0.5) {
                    int xb1 = mmRnd(Z1.xlr + zc_z1sr*(yb - Z1.yl));
                    int xb2 = mmRnd(Z2.xlr + zc_z2sr*(yb - Z2.yl));
                    if (xb1 == xb2) {
                        add_scan(yb);
                        return;
                    }
                }
                else {
                    int xt1 = mmRnd(Z1.xlr + zc_z1sr*(yt - Z1.yl));
                    int xt2 = mmRnd(Z2.xlr + zc_z2sr*(yt - Z2.yl));
                    if (xt1 == xt2) {
                        add_scan(yt);
                        return;
                    }
                }
                add_scan(yb);
                add_scan(yt);
            }

        void x1l2l()
            {
                double B = zc_z2sl - zc_z1sl;
                if (B == 0.0)
                    return;
                double A = Z1.xll - zc_z1sl*Z1.yl - Z2.xll + zc_z2sl*Z2.yl;
                A /= B;

                int yb = (int)floor(A);
                int yt = yb+1;

                if (A - yb < 0.5) {
                    int xb1 = mmRnd(Z1.xll + zc_z1sl*(yb - Z1.yl));
                    int xb2 = mmRnd(Z2.xll + zc_z2sl*(yb - Z2.yl));
                    if (xb1 == xb2) {
                        add_scan(yb);
                        return;
                    }
                }
                else {
                    int xt1 = mmRnd(Z1.xll + zc_z1sl*(yt - Z1.yl));
                    int xt2 = mmRnd(Z2.xll + zc_z2sl*(yt - Z2.yl));
                    if (xt1 == xt2) {
                        add_scan(yt);
                        return;
                    }
                }
                add_scan(yb);
                add_scan(yt);
            }

        const Zoid Z1, Z2;

        // Edge slopes.
        double zc_z1sl, zc_z1sr;
        double zc_z2sl, zc_z2sr;

        // Scan lines.  These are the yu/yl for each, plus the
        // off-grid intersced points, which with a one-count fix can
        // total 8.
        int zc_scans[12];
        int zc_nscans;
        bool zc_no_overlap;
    };


    ZoidClipper::ZoidClipper(const Zoid &z1, const Zoid &z2, bool nosort) :
        Z1(z1), Z2(z2)
    {
#ifdef ZC_DEBUG
        if (Z1.xur < Z1.xul || Z1.xlr < Z1.xll ||
                Z2.ur < Z2.ul || Z2.lr < Z2.ll)
            printf("ZoidClipper: bad zoid passed\n");
#endif
        zc_no_overlap = false;
        zc_nscans = 0;
        int ymin = mmMax(Z1.yl, Z2.yl);
        int ymax = mmMin(Z1.yu, Z2.yu);
        if (ymax <= ymin) {
            zc_no_overlap = true;
            return;
        }

        // Compute edge slopes.
        zc_z1sl = Z1.slope_left();
        zc_z1sr = Z1.slope_right();
        zc_z2sl = Z2.slope_left();
        zc_z2sr = Z2.slope_right();

        // Find the "scan lines", we'll sort later.  First, the tops
        // and bottoms.  Keep intersection range in the first to slots,
        // these are used in the intersection check functions.
        zc_scans[zc_nscans++] = ymin;  // highest bottom
        zc_scans[zc_nscans++] = ymax;  // lowest top

        if (Z1.yl != ymin)
            zc_scans[zc_nscans++] = Z1.yl;
        else if (Z2.yl != ymin)
            zc_scans[zc_nscans++] = Z2.yl;
        if (Z1.yu != ymax)
            zc_scans[zc_nscans++] = Z1.yu;
        else if (Z2.yu != ymax)
            zc_scans[zc_nscans++] = Z2.yu;

        // Look for edge intersections, add these to the scans list.
        x1r2r();
        x1l2l();
        x1r2l();
        x1l2r();

        // Sort scans into ascending order.
        if (!nosort)
            std::sort(zc_scans, zc_scans + zc_nscans);
    }


    // Return a list of Zoids representing the intersection of Z1 and Z2.
    //
    Zlist *
    ZoidClipper::clip_to()
    {
        if (zc_no_overlap)
            return (0);

        Zlist *z0 = 0;
        int yb = zc_scans[0];
        for (int i = 1; i < zc_nscans; i++) {
            int yt = zc_scans[i];
            if (yt == yb)
                continue;

            Zlist *zx = clip_to_scan(yb, yt);
            if (zx) {
                if (z0) {
                    Zlist *zn = zx;
                    while (zn->next)
                        zn = zn->next;
                    zn->next = z0;
                }
                z0 = zx;
            }
            yb = yt;
        }

#ifdef ZC_DEBUG
        test_clip_to(z0, &Z1, &Z2);
#endif
        return (z0);
    }


    // Return a list of zoids representing the parts of Z1 that do not
    // overlap Z2.  If the zoids don't overlap, return 0 with the no
    // overlap flag set.
    //
    Zlist *
    ZoidClipper::clip_around()
    {
        if (zc_no_overlap)
            return (0);

        Zlist *z0 = 0;
        int yb = zc_scans[0];
        for (int i = 1; i < zc_nscans; i++) {
            int yt = zc_scans[i];
            if (yt == yb)
                continue;

            Zlist *zx = clip_around_scan(yb, yt);
            if (zx) {
                if (z0) {
                    Zlist *zn = zx;
                    while (zn->next)
                        zn = zn->next;
                    zn->next = z0;
                }
                z0 = zx;
            }
            yb = yt;
        }

#ifdef ZC_DEBUG
        test_clip_around(z0, &Z1, &Z2);
#endif
        return (z0);
    }


    // Private function to clip within the band.
    //
    Zlist *
    ZoidClipper::clip_to_scan(int yb, int yt)
    {
        if (Z1.yl >= yt || Z1.yu <= yb)
            return (0);
        if (Z2.yl >= yt || Z2.yu <= yb)
            return (0);

        int xll, xlr, xul, xur;
        if (yb > Z1.yl) {
            xll = mmRnd(Z1.xll + zc_z1sl*(yb - Z1.yl));
            xlr = mmRnd(Z1.xlr + zc_z1sr*(yb - Z1.yl));
        }
        else {
            xll = Z1.xll;
            xlr = Z1.xlr;
        }
        if (yt < Z1.yu) {
            xul = mmRnd(Z1.xll + zc_z1sl*(yt - Z1.yl));
            xur = mmRnd(Z1.xlr + zc_z1sr*(yt - Z1.yl));
        }
        else {
            xul = Z1.xul;
            xur = Z1.xur;
        }
        Zoid z1(xll, xlr, yb, xul, xur, yt);

        if (yb > Z2.yl) {
            xll = mmRnd(Z2.xll + zc_z2sl*(yb - Z2.yl));
            xlr = mmRnd(Z2.xlr + zc_z2sr*(yb - Z2.yl));
        }
        else {
            xll = Z2.xll;
            xlr = Z2.xlr;
        }
        if (yt < Z2.yu) {
            xul = mmRnd(Z2.xll + zc_z2sl*(yt - Z2.yl));
            xur = mmRnd(Z2.xlr + zc_z2sr*(yt - Z2.yl));
        }
        else {
            xul = Z2.xul;
            xur = Z2.xur;
        }
        Zoid z2(xll, xlr, yb, xul, xur, yt);

        // We know that z1 and z2 have the same top and bottom, and
        // that no sides intersect if the height is larger than 1.
        // The conditionals handle the unit-height case properly.

        if (z1.xll > z2.xlr || z1.xlr < z2.xll)
            return (0);
        if (z1.xul > z2.xur || z1.xur < z2.xul)
            return (0);

        if (z1.xul < z2.xul)
            z1.xul = z2.xul;
        if (z1.xll < z2.xll)
            z1.xll = z2.xll;
        if (z1.xur > z2.xur)
            z1.xur = z2.xur;
        if (z1.xlr > z2.xlr)
            z1.xlr = z2.xlr;
        if (!z1.is_bad())
            return (new Zlist(&z1));
        return (0);
    }


    // Private function to clip within the band.
    //
    Zlist *
    ZoidClipper::clip_around_scan(int yb, int yt)
    {
        if (Z1.yl >= yt || Z1.yu <= yb)
            return (0);

        int xll, xlr, xul, xur;
        if (yb > Z1.yl) {
            xll = mmRnd(Z1.xll + zc_z1sl*(yb - Z1.yl));
            xlr = mmRnd(Z1.xlr + zc_z1sr*(yb - Z1.yl));
        }
        else {
            xll = Z1.xll;
            xlr = Z1.xlr;
        }
        if (yt < Z1.yu) {
            xul = mmRnd(Z1.xll + zc_z1sl*(yt - Z1.yl));
            xur = mmRnd(Z1.xlr + zc_z1sr*(yt - Z1.yl));
        }
        else {
            xul = Z1.xul;
            xur = Z1.xur;
        }
        Zoid z1(xll, xlr, yb, xul, xur, yt);

        if (Z2.yl >= yt || Z2.yu <= yb)
            return (new Zlist(&z1));

        if (yb > Z2.yl) {
            xll = mmRnd(Z2.xll + zc_z2sl*(yb - Z2.yl));
            xlr = mmRnd(Z2.xlr + zc_z2sr*(yb - Z2.yl));
        }
        else {
            xll = Z2.xll;
            xlr = Z2.xlr;
        }
        if (yt < Z2.yu) {
            xul = mmRnd(Z2.xll + zc_z2sl*(yt - Z2.yl));
            xur = mmRnd(Z2.xlr + zc_z2sr*(yt - Z2.yl));
        }
        else {
            xul = Z2.xul;
            xur = Z2.xur;
        }
        Zoid z2(xll, xlr, yb, xul, xur, yt);

        // We know that z1 and z2 have the same top and bottom, and
        // that no sides intersect if the height is larger than 1.
        // The conditionals handle the unit-height case properly.

        Zlist *z0 = 0;
        if (z1.xul < z2.xul || z1.xll < z2.xll) {
            xlr = mmMin(z2.xll, z1.xlr);
            xur = mmMin(z2.xul, z1.xur);
            Zoid zx(z1.xll, xlr, yb, z1.xul, xur, yt);
            if (!zx.is_bad())
                z0 = new Zlist(&zx, z0);
        }
        if (z1.xur > z2.xur || z1.xlr > z2.xlr) {
            xll = mmMax(z2.xlr, z1.xll);
            xul = mmMax(z2.xur, z1.xul);
            Zoid zx(xll, z1.xlr, yb, xul, z1.xur, yt);
            if (!zx.is_bad())
                z0 = new Zlist(&zx, z0);
        }
        return (z0);
    }
}


//------------------------------------------------------------------------
// Zoid clipping functions.
//------------------------------------------------------------------------

// Use the ZoidClipper to find the "scan lines" of intersecting
// non-Manhattan zoids.  Thes are the tops and bottoms, and any edge
// intersections not on a top or bottom.  Order is smallest bottom,
// largest top, second bottom if different, second top if different,
// edge intersections.  The pary should have size 12 or larger.
//
void
Zoid::scanlines(const Zoid *Zref, int *pary, int *psize)
{
    if (psize)
        *psize = 0;
    if (!(void*)this || !Zref)
        return;
    if (is_rect() && Zref->is_rect())
        return;
    if (yl >= Zref->yu || yu <= Zref->yl)
        return;
    if (minleft() >= Zref->maxright() || maxright() <= Zref->minleft())
        return;
    ZoidClipper zc(*this, *Zref, true);
    if (psize)
        *psize = zc.num_scans();
    if (pary) {
        for (int i = 0; i < zc.num_scans(); i++)
            pary[i] = zc.scans()[i];
    }
}


// Return a list of zoids which represent overlapping areas with the
// zoid given.
//
Zlist *
Zoid::clip_to(const Zoid *Zref) const
{
    if (!(void*)this || !Zref)
        return (0);
    if (yl >= Zref->yu || yu <= Zref->yl)
        return (0);
    if (is_rect() && Zref->is_rect()) {
        // Easy Manhattan case
        if (xll >= Zref->xlr || xlr <= Zref->xll)
            return (0);
        return (new Zlist(
            mmMax(xll, Zref->xll), mmMax(yl, Zref->yl),
            mmMin(xlr, Zref->xlr), mmMin(yu, Zref->yu)));
    }
    if (minleft() >= Zref->maxright() || maxright() <= Zref->minleft())
        return (0);
    return (ZoidClipper::clip_to(*this, *Zref));
}


// As above, but the clipping outline is explicitly rectangular.
//
Zlist *
Zoid::clip_to(const BBox *cBB) const
{
    if (!(void*)this || !cBB)
        return (0);
    if (yl >= cBB->top || yu <= cBB->bottom)
        return (0);
    if (is_rect()) {
        int yb = mmMax(yl, cBB->bottom);
        int yt = mmMin(yu, cBB->top);
        int xl = mmMax(xll, cBB->left);
        int xr = mmMin(xlr, cBB->right);
        if (xr <= xl)
            return (0);
        return (new Zlist(xl, yb, xr, yt));
    }
    if (minleft() >= cBB->right || maxright() <= cBB->left)
        return (0);
    Zoid Zref(cBB);
    return (ZoidClipper::clip_to(*this, Zref));
}


// Return a list of zoids which represent areas which do not overlap
// Zref.  If there is no overlap, set no_overlap and return 0.
//
Zlist *
Zoid::clip_out(const Zoid *Zref, bool *no_overlap) const
{
    if (!(void*)this || !Zref) {
        *no_overlap = true;
        return (0);
    }
    if (yl >= Zref->yu || yu <= Zref->yl) {
        *no_overlap = true;
        return (0);
    }

    if (is_rect() && Zref->is_rect()) {
        // Easy Manhattan case
        if (xll >= Zref->xlr || xlr <= Zref->xll) {
            *no_overlap = true;
            return (0);
        }
        *no_overlap = false;
        Zlist *z0 = 0;
        if (yu > Zref->yu)
            z0 = new Zlist(xll, Zref->yu, xur, yu, z0);
        if (yl < Zref->yl)
            z0 = new Zlist(xll, yl, xur, Zref->yl, z0);
        int t = Zref->yu < yu ? Zref->yu : yu;
        int b = Zref->yl > yl ? Zref->yl : yl;
        if (xll < Zref->xll)
            z0 = new Zlist(xll, b, Zref->xll, t, z0);
        if (xur > Zref->xur)
            z0 = new Zlist(Zref->xur, b, xur, t, z0);
        return (z0);
    }

    if (minleft() >= Zref->maxright() || maxright() <= Zref->minleft()) {
        *no_overlap = true;
        return (0);
    }
    return (ZoidClipper::clip_around(*this, *Zref, no_overlap));
}


// Return true if the line segment overlaps the zoid with non-unit
// length.  Return the intersection endpoints in pret[2] if passed.
// The pret values are ordered 1,2.
//
bool
Zoid::line_clip(int x1, int y1, int x2, int y2, Point *pret) const
{
    if (x1 == x2) {
        if (y1 == y2)
            return (false);
        bool swap = false;
        if (y1 > y2) {
            int t = y1;
            y1 = y2;
            y2 = t;
            swap = true;
        }
        if (y2 <= yl || y1 >= yu)
            return (false);

        int minl, maxl;
        if (xll < xul) {
            minl = xll;
            maxl = xul;
        }
        else {
            minl = xul;
            maxl = xll;
        }
        int minr, maxr;
        if (xlr < xur) {
            minr = xlr;
            maxr = xur;
        }
        else {
            minr = xur;
            maxr = xlr;
        }
        if (x1 < minl || x1 > maxr)
            return (false);

        int ylwr = 0;
        int yupr = 0;
        if (x1 >= maxl && x1 <= minr) {
            ylwr = yl;
            yupr = yu;
        }
        else {
            if (xul < xll && x1 > xul && x1 < xll) {
                double T1 = x1 - xll;
                double T2 = xul - xll;
                ylwr = yl + mmRnd(T1/T2);
            }
            else if (x1 >= xll && x1 <= xlr)
                ylwr = yl;
            else if (xur > xlr && x1 < xur && x1 > xlr) {
                double T1 = x1 - xlr;
                double T2 = xur - xlr;
                ylwr = yl + mmRnd(T1/T2);
            }
            else return (false);

            if (xll < xul && x1 > xll && x1 < xul) {
                double T1 = x1 - xll;
                double T2 = xul - xll;
                yupr = yl + mmRnd(T1/T2);
            }
            else if (x1 >= xul && x1 <= xur)
                yupr = yu;
            else if (xlr > xur && x1 < xlr && x1 > xur) {
                double T1 = x1 - xlr;
                double T2 = xur - xlr;
                yupr = yl + mmRnd(T1/T2);
            }
        }
        if (y1 > ylwr)
            ylwr = y1;
        if (y2 < yupr)
            yupr = y2;
        if (yupr > ylwr) {
            if (pret) {
                if (swap) {
                    pret[0].set(x1, yupr);
                    pret[1].set(x1, ylwr);
                }
                else {
                    pret[0].set(x1, ylwr);
                    pret[1].set(x1, yupr);
                }
            }
            return (true);
        }
        return (false);
    }
    else if (y1 == y2) {
        if (y1 > yu || y1 < yl)
            return (false);
        bool swap = false;
        if (x1 > x2) {
            int t = x1;
            x1 = x2;
            x2 = t;
            swap = true;
        }
        int xlwr = 0, xupr = 0;
        if (y1 == yu) {
            xlwr = xul;
            xupr = xur;
        }
        else if (y1 == yl) {
            xlwr = xll;
            xupr = xlr;
        }
        else {
            if (xul == xll)
                xlwr = xll;
            else
                xlwr = xll + mmRnd((y1 - yl)*slope_left());
            if (xur == xlr)
                xupr = xlr;
            else
                xupr = xlr + mmRnd((y1 - yl)*slope_right());
        }
        if (x1 > xlwr)
            xlwr = x1;
        if (x2 < xupr)
            xupr = x2;
        if (xupr > xlwr) {
            if (pret) {
                if (swap) {
                    pret[0].set(xupr, y1);
                    pret[1].set(xlwr, y1);
                }
                else {
                    pret[0].set(xlwr, y1);
                    pret[1].set(xupr, y1);
                }
            }
            return (true);
        }
        return (false);
    }

    BBox lBB(x1, y1, x2, y2);
    lBB.fix();
    if (lBB.top < yl || lBB.bottom > yu)
        return (false);
    if (lBB.right < minleft() || lBB.left > maxright())
        return (false);

    Point_c p1(x1, y1);
    bool in1 = intersect(&p1, true);
    Point_c p2(x2, y2);
    bool in2 = intersect(&p2, true);
    if (in1 && in2) {
        if (pret) {
            pret[0].set(x1, y1);
            pret[1].set(x2, y2);
        }
        return (true);
    }
    Point px[2];
    int pcnt = 0;
    if (in1) {
        px[0] = p1;
        pcnt++;
    }
    else if (in2) {
        px[0] = p2;
        pcnt++;
    }
    int dx = x2 - x1;
    int dy = y2 - y1;
    bool check_y = abs(dy) > abs(dx);

    Point_c pz1(xll, yl);
    Point_c pz2(xul, yu);
    if (GEO()->lines_cross(p1, p2, pz1, pz2, px + pcnt)) {
        if (!pcnt || (px[0] != px[1]))
            pcnt++;
        if (pcnt == 2)
            goto good;
    }
    pz1 = Point_c(xur, yu);
    if (GEO()->lines_cross(p1, p2, pz2, pz1, px + pcnt)) {
        if (!pcnt || (px[0] != px[1]))
            pcnt++;
        if (pcnt == 2)
            goto good;
    }
    pz2 = Point_c(xlr, yl);
    if (GEO()->lines_cross(p1, p2, pz1, pz2, px + pcnt)) {
        if (!pcnt || (px[0] != px[1]))
            pcnt++;
        if (pcnt == 2)
            goto good;
    }
    pz1 = Point_c(xll, yl);
    if (GEO()->lines_cross(p1, p2, pz2, pz1, px + pcnt)) {
        if (!pcnt || (px[0] != px[1]))
            pcnt++;
        if (pcnt == 2)
            goto good;
    }
    return (false);
good:
    if (pret) {
        bool swap = false;
        if (check_y) {
            if (dy > 0 && px[1].y < px[0].y)
                swap = true;
            else if (dy < 0 && px[1].y > px[0].y)
                swap = true;
        }
        else {
            if (dx > 0 && px[1].x < px[0].x)
                swap = true;
            else if (dx < 0 && px[1].x > px[0].x)
                swap = true;
        }
        if (swap) {
            pret[0] = px[1];
            pret[1] = px[0];
        }
        else {
            pret[0] = px[0];
            pret[1] = px[1];
        }
    }
    return (true);
}


// Private function to do the messy part for zoid/point intersection.
//
bool
Zoid::isp_prv(const Point *p, bool touchok) const
{
    if (touchok) {
        if (p->x < xl_y(p->y))
            return (false);
        if (p->x > xr_y(p->y))
            return (false);
    }
    else {
        if (p->x <= xl_y(p->y))
            return (false);
        if (p->x >= xr_y(p->y))
            return (false);
    }
    return (true);
}


// Private function to do the messy part for zoid/box intersection.
//
bool
Zoid::isb_prv(const BBox *pBB, bool touchok) const
{
    int x1, x2, x3, x4;
    if (pBB->bottom > yl) {
        x1 = xl_y(pBB->bottom);
        x3 = xr_y(pBB->bottom);
    }
    else {
        x1 = xll;
        x3 = xlr;
    }
    if (pBB->top < yu) {
        x2 = xl_y(pBB->top);
        x4 = xr_y(pBB->top);
    }
    else {
        x2 = xul;
        x4 = xur;
    }
    if (touchok) {
        if (pBB->right < x1 && pBB->right < x2)
            return (false);
        if (pBB->left > x3 && pBB->left > x4)
            return (false);
    }
    else {
        if (pBB->right <= x1 && pBB->right <= x2)
            return (false);
        if (pBB->left >= x3 && pBB->left >= x4)
            return (false);
    }
    return (true);
}


// Private function to do the messy part for zoid/zoid intersection.
//
bool
Zoid::isz_prv(const Zoid *z2, bool touchok) const
{
    const Zoid *z1 = this;

    int ymin = mmMax(z1->yl, z2->yl);
    int ymax = mmMin(z1->yu, z2->yu);

    int ll1, lr1, ll2, lr2;
    if (z1->yl != ymin) {
        ll1 = z1->xl_y(ymin);
        lr1 = z1->xr_y(ymin);
    }
    else {
        ll1 = z1->xll;
        lr1 = z1->xlr;
    }
    if (z2->yl != ymin) {
        ll2 = z2->xl_y(ymin);
        lr2 = z2->xr_y(ymin);
    }
    else {
        ll2 = z2->xll;
        lr2 = z2->xlr;
    }
    if (touchok) {
        if (lr1 >= ll2 && lr2 >= ll1)
            return (true);
    }
    else {
        if (lr1 >= ll2 && lr2 >= ll1)
            return (true);
    }

    int ul1, ur1, ul2, ur2;
    if (z1->yu != ymax) {
        ul1 = z1->xl_y(ymax);
        ur1 = z1->xr_y(ymax);
    }
    else {
        ul1 = z1->xul;
        ur1 = z1->xur;
    }
    if (z2->yu != ymax) {
        ul2 = z2->xl_y(ymax);
        ur2 = z2->xr_y(ymax);
    }
    else {
        ul2 = z2->xul;
        ur2 = z2->xur;
    }

    if (touchok) {
        if ((lr1 < ll2 && ur1 < ul2) || (lr2 < ll1 && ur2 < ul1))
            return (false);
    }
    else {
        if ((lr1 < ll2 && ur1 < ul2) || (lr2 < ll1 && ur2 < ul1))
            return (false);
    }
    return (true);
}


//------------------------------------------------------------------------
// Testing and Diagnostics
//------------------------------------------------------------------------

#ifdef ZC_DEBUG

namespace {
    bool
    ztest(const Zoid *z)
    {
        bool ok = true;

        if (z->xll > z->xlr) {
            printf("trouble: xll > xlr %d %d\n",  z->xll, z->xlr);
            ok = false;
        }
        if (z->xul > z->xur) {
            printf("trouble: xul > xur %d %d\n",  z->xul, z->xur);
            ok = false;
        }
        if (z->yl >= z->yu) {
            printf("trouble: yl > yu %d %d\n",  z->yl, z->yu);
            ok = false;
        }
        return (ok);
    }
}

#endif

#ifdef ZC_DEBUG

#define SCRFILE "zout.scr"

namespace {
    int ccnt;

    void
    out_zoid(FILE *fp, const char *aname, const Zoid *z)
    {
        int ndgt = CD()->numDigits();
        fprintf(fp, "%s[10]\n", aname);
        fprintf(fp, "%s[0] = %.*f\n", aname, ndgt, MICRONS(z->xll));
        fprintf(fp, "%s[1] = %.*f\n", aname, ndgt, MICRONS(z->yl));
        fprintf(fp, "%s[2] = %.*f\n", aname, ndgt, MICRONS(z->xul));
        fprintf(fp, "%s[3] = %.*f\n", aname, ndgt, MICRONS(z->yu));
        fprintf(fp, "%s[4] = %.*f\n", aname, ndgt, MICRONS(z->xur));
        fprintf(fp, "%s[5] = %.*f\n", aname, ndgt, MICRONS(z->yu));
        fprintf(fp, "%s[6] = %.*f\n", aname, ndgt, MICRONS(z->xlr));
        fprintf(fp, "%s[7] = %.*f\n", aname, ndgt, MICRONS(z->yl));
        fprintf(fp, "%s[8] = %.*f\n", aname, ndgt, MICRONS(z->xll));
        fprintf(fp, "%s[9] = %.*f\n", aname, ndgt, MICRONS(z->yl));
    }


    void
    test_clip_to(const Zlist *z0, const Zoid *z1, const Zoid *z2)
    {
        bool ok = true;
        ccnt++;
        int ll1, lr1, ll2, lr2;
        int ul1, ur1, ul2, ur2;
        for (const Zlist *zl = z0; zl; zl = zl->next) {
            ll1 = z1->xl_y(zl->Z.yl);
            lr1 = z1->xr_y(zl->Z.yl);
            ll2 = z2->xl_y(zl->Z.yl);
            lr2 = z2->xr_y(zl->Z.yl);
            ul1 = z1->xl_y(zl->Z.yu);
            ur1 = z1->xr_y(zl->Z.yu);
            ul2 = z2->xl_y(zl->Z.yu);
            ur2 = z2->xr_y(zl->Z.yu);

            int l = mmMax(ll1, ll2);
            if (abs(l - zl->Z.xll) > 0) {
                printf("trouble xll: %d %d\n", l, zl->Z.xll);
                ok = false;
            }
            int r = mmMin(lr1, lr2);
            if (abs(r - zl->Z.xlr) > 0) {
                printf("trouble xlr: %d %d\n", r, zl->Z.xlr);
                ok = false;
            }
            l = mmMax(ul1, ul2);
            if (abs(l - zl->Z.xul) > 0) {
                printf("trouble xul: %d %d\n", l, zl->Z.xul);
                ok = false;
            }
            r = mmMin(ur1, ur2);
            if (abs(r != zl->Z.xur) > 0) {
                printf("trouble xur: %d %d\n", r, zl->Z.xur);
                ok = false;
            }
            if (!ztest(&zl->Z))
                ok = false;
        }
        if (!ok) {
            FILE *fp = fopen(SCRFILE, "a");
            printf("---- %d\n", ccnt);
            fprintf(fp, "%d\n", ccnt);
            out_zoid(fp, "a", z1);
            out_zoid(fp, "b", z2);
            fclose(fp);
        }
    }

    bool
    test_clip_around(const Zoid *z0, const Zoid *z1, const Zoid *z2)
    {
        bool ok = true;
        if (z0->yl >= z2->yu || z0->yu <= z2->yl)
            return (ok);

        int ll1 = z1->xl_y(z0->yl);
        int lr1 = z1->xr_y(z0->yl);
        int ul1 = z1->xl_y(z0->yu);
        int ur1 = z1->xr_y(z0->yu);

        if (z0->xll < ll1) {
            printf("trouble xll/ll1 %d %d\n", z0->xll, ll1);
            ok = false;
        }
        if (z0->xlr > lr1) {
            printf("trouble xlr/lr1 %d %d\n", z0->xlr, lr1);
            ok = false;
        }
        if (z0->xul < ul1) {
            printf("trouble xul/ul1 %d %d\n", z0->xul, ul1);
            ok = false;
        }
        if (z0->xur > ur1) {
            printf("trouble xur/ur1 %d %d\n", z0->xur, ur1);
            ok = false;
        }

        int ll2 = z2->xl_y(z0->yl);
        int lr2 = z2->xr_y(z0->yl);
        int ul2 = z2->xl_y(z0->yu);
        int ur2 = z2->xr_y(z0->yu);

        if (z0->xll > ll2 && z0->xll < lr2) {
            printf("trouble xll in 2 %d ll2=%d lr2=%d\n", z0->xll, ll2, lr2);
            ok = false;
        }
        if (z0->xlr > ll2 && z0->xlr < lr2) {
            printf("trouble xlr in 2 %d ll2=%d lr2=%d\n", z0->xlr, ll2, lr2);
            ok = false;
        }
        if (z0->xul > ul2 && z0->xul < ur2) {
            printf("trouble xul in 2 %d ul2=%d ur2=%d\n", z0->xul, ul2, ur2);
            ok = false;
        }
        if (z0->xur > ul2 && z0->xur < ur2) {
            printf("trouble xur in 2 %d ul2=%d ur2=%d\n", z0->xur, ul2, ur2);
            ok = false;
        }
        if (!ztest(z0))
            ok = false;
        return (ok);
    }

    void
    test_clip_around(const Zlist *z0, const Zoid *z1, const Zoid *z2)
    {
        bool ok = true;
        ccnt++;
        for (const Zlist *zl = z0; zl; zl = zl->next) {
            if (!test_clip_around(&zl->Z, z1, z2))
                ok = false;
        }
        if (!ok) {
            FILE *fp = fopen(SCRFILE, "a");
            printf("---- %d\n", ccnt);
            fprintf(fp, "%d\n", ccnt);
            out_zoid(fp, "a", z1);
            out_zoid(fp, "b", z2);
            fclose(fp);
        }
    }
}

#endif

