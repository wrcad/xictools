
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

#ifndef GEO_ZOID_H
#define GEO_ZOID_H

#include "geo_box.h"


struct Zgroup;
struct Zlist;
struct Ylist;
struct PolyList;
struct CDs;
struct CDl;

// Horizontal trapezoid
//
struct Zoid
{
    Zoid() { xll = yl = xur = 0; yu = xul = xlr = 0; }
    Zoid(int zll, int zlr, int zl, int zul, int zur, int zu)
        { xll = zll; xlr = zlr; yl = zl; xul = zul; xur = zur; yu = zu; }
    Zoid(int l, int b, int r, int t)
        { xll = xul = l; yl = b; xlr = xur = r; yu = t; }
    Zoid(const BBox *pBB) { xll = xul = pBB->left; yl = pBB->bottom;
        xlr = xur = pBB->right; yu = pBB->top; }

    bool is_rect() const { return (xll == xul && xlr == xur); }
    bool is_manh() const { return (xll == xul && xlr == xur); }

    bool is_45() const
        {
            int dy = yu - yl;
            int dx = xul - xll;
            if (dx && abs(dx) != dy)
                return (false);
            dx = xur - xlr;
            if (dx && abs(dx) != dy)
                return (false);
            return (true);
        }

    int minleft() const { return (xll < xul ? xll : xul); }
    int maxright() const { return (xlr > xur ? xlr : xur); }
    void BB(BBox *bb) const { bb->left = minleft(); bb->bottom = yl;
        bb->right = maxright(); bb->top = yu; }
    double area() const { return ((((double)(yu-yl))*(xur-xul + xlr-xll))/
        (2*CDphysResolution*CDphysResolution)); }

    bool operator==(const Zoid &zc)
        {
            return (yl == zc.yl && yu == zc.yu && xll == zc.xll &&
                xul == zc.xul && xlr == zc.xlr && xur == zc.xur);
        }

    bool operator!=(const Zoid &zc)
        {
            return (yl != zc.yl || yu != zc.yu || xll != zc.xll ||
                xul != zc.xul || xlr != zc.xlr || xur != zc.xur);
        }

    double slope_left() const
        {
            int dx = xul - xll;
            if (!dx)
                return (0.0);
            double dy = yu - yl;
            return (dx/dy);
        }

    double slope_right() const
        {
            int dx = xur - xlr;
            if (!dx)
                return (0.0);
            double dy = yu - yl;
            return (dx/dy);
        }

    int xl_y(int y) const
        {
            int dx = xul - xll;
            if (!dx)
                return (xll);
            double dy = yu - yl;
            return (mmRnd(xll + (y - yl)*(dx/dy)));
        }

    int xr_y(int y) const
        {
            int dx = xur - xlr;
            if (!dx)
                return (xlr);
            double dy = yu - yl;
            return (mmRnd(xlr + (y - yl)*(dx/dy)));
        }

    int xlt_y(int y) const
        {
            int dx = xul - xll;
            if (!dx)
                return (xul);
            double dy = yu - yl;
            return (mmRnd(xul + (y - yu)*(dx/dy)));
        }

    int xrt_y(int y) const
        {
            int dx = xur - xlr;
            if (!dx)
                return (xur);
            double dy = yu - yl;
            return (mmRnd(xur + (y - yu)*(dx/dy)));
        }

    // This returns +1, 0, -1 if the argument is before, equal, after
    // this in descending in yu/ascending in x order.  For Ylist.
    //
    int zcmp(const Zoid *zc) const
        {
            if (yu < zc->yu)
                return (1);
            if (yu > zc->yu)
                return (-1);

            {
                int ml = minleft();
                int mlc = zc->minleft();
                if (ml > mlc)
                    return (1);
                if (ml < mlc)
                    return (-1);
            }

            if (xul > zc->xul)
                return (1);
            if (xul < zc->xul)
                return (-1);
            if (xll > zc->xll)
                return (1);
            if (xll < zc->xll)
                return (-1);

            {
                int mr = maxright();
                int mrc = zc->maxright();
                if (mr < mrc)
                    return (1);
                if (mr > mrc)
                    return (-1);
            }

            if (xur < zc->xur)
                return (1);
            if (xur > zc->xur)
                return (-1);
            if (xlr < zc->xlr)
                return (1);
            if (xlr > zc->xlr)
                return (-1);

            if (yl > zc->yl)
                return (1);
            if (yl < zc->yl)
                return (-1);
            return (0);
        }

    bool intersect(const Point *p, bool touchok) const
        {
            if (touchok) {
                if (p->y > yu || p->y < yl)
                    return (false);
                if (p->x < minleft() || p->x > maxright())
                    return (false);
            }
            else {
                if (p->y >= yu || p->y <= yl)
                    return (false);
                if (p->x <= minleft() || p->x >= maxright())
                    return (false);
            }
            if (is_rect())
                return (true);
            return (isp_prv(p, touchok));
        }

    bool intersect(const BBox *pBB, bool touchok) const
        {
            if (touchok) {
                if (pBB->bottom > yu || pBB->top < yl)
                    return (false);
                if (maxright() < pBB->left || pBB->right < minleft())
                    return (false);
            }
            else {
                if (pBB->bottom >= yu || pBB->top <= yl)
                    return (false);
                if (maxright() <= pBB->left || pBB->right <= minleft())
                    return (false);
            }
            if (is_rect())
                return (true);
            return (isb_prv(pBB, touchok));
        }

    bool intersect(const Zoid *Z, bool touchok) const
        {
            if (touchok) {
                if (yl > Z->yu || Z->yl > yu)
                    return (false);
                if (maxright() < Z->minleft() || Z->maxright() < minleft())
                    return (false);
                if (is_rect() && Z->is_rect())
                    return (true);
                if (yl != Z->yl || yu != Z->yu)
                    return (isz_prv(Z, touchok));
                if (xur < Z->xul && xlr < Z->xll)
                    return (false);
                if (Z->xur < xul && Z->xlr < xll)
                    return (false);
            }
            else {
                if (yl >= Z->yu || Z->yl >= yu)
                    return (false);
                if (maxright() <= Z->minleft() || Z->maxright() <= minleft())
                    return (false);
                if (is_rect() && Z->is_rect())
                    return (true);
                if (yl != Z->yl || yu != Z->yu)
                    return (isz_prv(Z, touchok));
                if (xur < Z->xul && xlr < Z->xll)
                    return (false);
                if (Z->xur < xul && Z->xlr < xll)
                    return (false);
            }
            return (true);
        }

    bool is_interior(const Point *p) const
        {
            const int f = 2;
            if (p->y >= yu - f)
                return (false);
            if (p->y <= yl + f)
                return (false);
            if (p->x <= xl_y(p->y) + f)
                return (false);
            if (p->x >= xr_y(p->y) - f)
                return (false);
            return (true);
        }

    bool touching(const Zoid *Z) const
        {
            if (is_rect() && Z->is_rect())
                return (true);
            if (yl != Z->yl || yu != Z->yu)
                return (isz_prv(Z, true));
            if (xur < Z->xul && xlr < Z->xll)
                return (false);
            if (Z->xur < xul && Z->xlr < xll)
                return (false);
            return (true);
        }

    // Return true if the zoid is degenerate, fix order if necessary.
    //
    bool is_bad()
        {
            if (yu <= yl)
                return (true);

            if (yu - yl == 1) {
                // For unit-high slivers, Manhattanize any angles
                // sharper than 45s.  This helps prevent clipping
                // errors.

                if (abs(xul-xll) > 1) {
                    int t = (xul + xll) >> 1;
                    xul = t;
                    xll = t;
                }
                if (abs(xur-xlr) > 1) {
                    int t = (xur + xlr) >> 1;
                    xur = t;
                    xlr = t;
                }
            }
            if (xur <= xul && xlr <= xll)
                return (true);

            // If the top or bottom is a "point" but the vertices are
            // out of order by one or two units, move the sides to get
            // a valid zoid.  If the difference is one, (arbitrarily)
            // move the side that keeps the x value of the point even.
            // Note that we don't change angles!

            if (xul > xur) {
                int dx = xul - xur;
                if (dx == 2) {
                    xul--;
                    xll--;
                    xur++;
                    xlr++;
                }
                else if (dx == 1) {
                    if (xul & 1) {
                        xul--;
                        xll--;
                    }
                    else {
                        xur++;
                        xlr++;
                    }
                }
                else
                    return (true);
            }
            else if (xll > xlr) {
                int dx = xll - xlr;
                if (dx == 2) {
                    xul--;
                    xll--;
                    xur++;
                    xlr++;
                }
                else if (dx == 1) {
                    if (xll & 1) {
                        xul--;
                        xll--;
                    }
                    else {
                        xur++;
                        xlr++;
                    }
                }
                else
                    return (true);
            }
            return (false);
        }

    // Return true if the zoid is a sliver and should be thrown out.
    //
    bool is_sliver(int d) const
        {
            // Keep any finite-size rectangular bits.
            if (is_rect())
                return (yu <= yl || xlr <= xll);

            int dy = yu - yl;
            if (dy <= d)
                return (true);
            int dx1 = xur - xul + xlr - xll;
            if (dx1 <= d)
                return (true);
            if (d) {
                int dx2 = xur + xul - xlr - xll;
                if (dx2 < 0)
                    dx2 = -dx2;
                if (dx1 <= d*dx2/dy)
                    return (true);
            }
            return (false);
        }

    // As above, but throw out the too-small rects as well, avoiding
    // DRC false-positives.  The regular is_sliver is ideal for
    // avoiding tiny gaps and notches when assembling polygons from
    // trapezoid lists (for example), but the unit-sized Manhattan
    // slivers produced in is_bad will cause trouble in DRC.
    //
    bool is_drc_sliver(int d) const
        {
            int dy = yu - yl;
            if (dy <= d)
                return (true);
            if (xll == xul && xlr == xur && xlr - xll <= d)
                return (true);

            int dx1 = xur - xul + xlr - xll;
            if (dx1 <= d)
                return (true);
            if (d) {
                int dx2 = xur + xul - xlr - xll;
                if (dx2 < 0)
                    dx2 = -dx2;
                if (dx1 <= d*dx2/dy)
                    return (true);
            }
            return (false);
        }

    void set(const BBox *pBB)
        {
            xll = xul = pBB->left;
            xlr = xur = pBB->right;
            yl = pBB->bottom;
            yu = pBB->top;
        }

    void computeBB(BBox *pBB) const
        {
            pBB->left = (xll < xul ? xll : xul);
            pBB->right = (xlr > xur ? xlr : xur);
            pBB->bottom = yl;
            pBB->top = yu;
        }

    bool join_right(const Zoid &ZR)
        {
            if (yu != ZR.yu || yl != ZR.yl)
                return (false);
            if (ZR.xul > xur || ZR.xll > xlr)
                return (false);
            if (ZR.xul < xul || ZR.xll < xll)
                return (false);
            if (ZR.xur < xur || ZR.xlr < xlr)
                return (false);
            xur = ZR.xur;
            xlr = ZR.xlr;
            return (true);
        }

    void expand_by_2()
        {
            yl <<= 1;
            yu <<= 1;
            xll <<= 1;
            xul <<= 1;
            xlr <<= 1;
            xur <<= 1;
        }

    // geo_zoid.cc
    void print(FILE* = 0) const;
    void show() const;
    void shrink_by_2();
    bool mkpoly(Point**, int*, bool = false) const;
    bool join_below(const Zoid&);
    bool join_above(const Zoid&);
    void rt_triang(Zlist**, int);
    bool test_coverage(const Zlist*, bool*, int, Zlist** = 0) const;
    bool test_coverage(const CDs*, const CDl*, bool*, int, Zlist** = 0) const;
    bool test_existence(const CDs*, const CDl*, int) const;

    // geo_zoidclip.cc
    void scanlines(const Zoid*, int*, int*);
    Zlist *clip_to(const Zoid*) const;
    Zlist *clip_to(const BBox*) const;
    Zlist *clip_out(const Zoid*, bool*) const;
    bool line_clip(int, int, int, int, Point*) const;

private:
    // geo_zoidclip.cc
    bool isp_prv(const Point*, bool) const;
    bool isb_prv(const BBox*, bool) const;
    bool isz_prv(const Zoid*, bool) const;

public:
    int yl;    // lower Y
    int yu;    // upper Y
    int xll;   // lewer left
    int xul;   // upper left
    int xlr;   // lower right
    int xur;   // upper right
};


// Used in clipping functions.
struct ovlchk_t
{
    ovlchk_t(int l, int r, int y) { minleft = l; maxright = r; yu = y; }

    // Return true if Z can't overlap saved reference zoid, but the next
    // Z in order might overlap.
    //
    bool check_continue(const Zoid &Z)
        {
            return (Z.maxright() <= minleft || Z.yl >= yu);
        }

    // Return true if Z can't overlap saved reference zoid, and no other
    // ordered zoids can overlap.
    //
    bool check_break(const Zoid &Z)
        {
            return (Z.minleft() >= maxright);
        }

private:
    int maxright;
    int minleft;
    int yu;
};

#endif

