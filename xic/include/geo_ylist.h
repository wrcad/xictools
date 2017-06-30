
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
 $Id: geo_ylist.h,v 5.35 2016/01/26 02:17:15 stevew Exp $
 *========================================================================*/

#ifndef GEO_YLIST_H
#define GEO_YLIST_H

#include "geo_memmgr_cfg.h"
#include "geo_zlist.h"


struct intDb;
struct Zgroup;
namespace geo_ylist {
    struct topoly_t;
}

// Container for sorted trapezoid lists.  The list is in descending
// order in yu.  All zoids in zlist have the same yu, and are sorted
// in ascending order in xul.  The yl is the minimum in zlist.
//
struct Ylist
{
#ifdef GEO_USE_MANAGER
    void *operator new(size_t);
    void operator delete(void*, size_t);
#endif
    Ylist(Zlist*, bool = false);
    ~Ylist() { Zlist::destroy(y_zlist); }

    void free()
        {
            Ylist *y = this;
            while (y) {
                Ylist *yn = y->next;
                delete y;
                y = yn;
            }
        }

    unsigned int count_zoids()
        {
            unsigned int cnt = 0;
            Ylist *y = this;
            while (y) {
                cnt += Zlist::length(y->y_zlist);
                y = y->next;
            }
            return (cnt);
        }

    void set_zlist(Zlist *zl)
        {
            Zlist::destroy(y_zlist);
            y_zlist = zl;
        }

    Zlist *to_zlist()
        {
            Zlist *z0 = 0, *ze = 0;
            Ylist *yn;
            for (Ylist *y = this; y; y = yn) {
                yn = y->next;
                if (y->y_zlist) {
                    if (!z0)
                        z0 = ze = y->y_zlist;
                    else
                        ze->next = y->y_zlist;
                    while (ze->next)
                        ze = ze->next;
                    y->y_zlist = 0;
                }
                delete y;
            }
            return (z0);
        }

    Ylist *strip_empty()
        {
            Ylist *y0 = this;
            Ylist *yp = 0, *yn;
            for (Ylist *y = y0; y; y = yn) {
                yn = y->next;
                if (!y->y_zlist) {
                    if (yp)
                        yp->next = yn;
                    else
                        y0 = yn;
                    delete y;
                    continue;
                }
                yp = y;
            }
            return (y0);
        }

    Ylist *clear_to_next()
        {
            Ylist *yl = this;
            if (!yl)
                return (0);
            Ylist *yn = yl->next;
            delete yl;
            return (yn);
        }

    void col_row_merge() throw (XIrt)
        {
            bool chg = true;
            while (chg) {
                chg = merge_cols();
                if (chg)
                    chg = merge_rows();
            }
        }

    void computeBB(BBox *BB) const
        {
            if (!BB)
                return;
            *BB = CDnullBB;
            const Ylist *y0 = this;
            if (!y0)
                return;
            BB->top = y0->y_yu;
            BB->bottom = y0->y_yl;
            for (const Ylist *y = y0; y; y = y->next) {
                if (y->y_yl < BB->bottom)
                    BB->bottom = y->y_yl;
                for (Zlist *z = y->y_zlist; z; z = z->next) {
                    if (z->Z.xll < BB->left)
                        BB->left = z->Z.xll;
                    if (z->Z.xul < BB->left)
                        BB->left = z->Z.xul;
                    if (z->Z.xlr > BB->right)
                        BB->right = z->Z.xlr;
                    if (z->Z.xur > BB->right)
                        BB->right = z->Z.xur;
                }
            }
        }

    // Merge the scan list.
    //
    static void scl_merge(Zlist *z0)
        {
            Zlist *zn;
            for (Zlist *z = z0; z; z = zn) {
                zn = z->next;
                if (!zn)
                    break;
                Zoid &z1 = z->Z;
                Zoid &z2 = zn->Z;
                //
                // *** Note that we will fill 1-unit gaps.
                //
                if (z2.xul > z1.xur+1 || z2.xll > z1.xlr+1)
                    continue;

                // If height is > 1, we know that no edges cross,
                // which simplifies the logic.  This may not be true
                // for unit height.  The logic below works in either
                // case.

                if (z1.xul > z2.xul)
                    z1.xul = z2.xul;
                if (z1.xll > z2.xll)
                    z1.xll = z2.xll;
                if (z1.xur < z2.xur)
                    z1.xur = z2.xur;
                if (z1.xlr < z2.xlr)
                    z1.xlr = z2.xlr;
                z->next = zn->next;
                delete zn;
                zn = z;
            }
        }

    // geo_ylist.cc
    bool intersect(const Ylist*, bool) const;
    const Zoid *find_container(const Point*);
    Ylist *copy() const;
    Ylist *to_poly(Point**, int*, int);
    Zgroup *group(int = 0);
    Ylist *connected(Zlist**);
    Ylist *remove_backg(const BBox*);
    void scanlines(const Ylist*, intDb&) const;
    Ylist *slice(const int*, int) throw (XIrt);
    Zlist *repartition() throw (XIrt);
    Zlist *repartition_ni();
    Ylist *repartition_group() throw (XIrt);
    bool merge_rows() throw (XIrt);
    bool merge_cols() throw (XIrt);
    bool merge_end_row(Ylist*);
    Ylist *filter_slivers(int = 0);
    Zlist *clip_to() throw (XIrt);
    Zlist *clip_to(const Zoid*) const;
    Zlist *clip_to(const Ylist*) const throw (XIrt);
    Zlist *clip_out() const throw (XIrt);
    Ylist *clip_out(const Zoid*);
    Zlist *clip_out(const Ylist*) const throw (XIrt);

    Ylist *scl_clip_to(Ylist*) throw (XIrt);
    Ylist *scl_clip_out() throw (XIrt);
    Ylist *scl_clip_out(Ylist*) throw (XIrt);
    Ylist *scl_clip_out2(Ylist**) throw (XIrt);
    Ylist *scl_clip_xor(Ylist*) throw (XIrt);
    bool debug();
    bool debug_row();

    static void remove_common(Ylist**, Ylist**);

private:
    void remove_next(Zlist *zp, Zlist *z)
        {
            if (!zp)
                y_zlist = z->next;
            else
                zp->next = z->next;
            if (z->Z.yl == y_yl && y_zlist) {
                int y = y_zlist->Z.yl;
                if (y != y_yl) {
                    for (Zlist *zl = y_zlist->next; zl; zl = zl->next) {
                        if (zl->Z.yl < y)
                            y = zl->Z.yl;
                        if (y == y_yl)
                            break;
                    }
                    y_yl = y;
                }
            }
        }

    void link_into_this_row(Zlist *zt)
        {
            Zlist *zz = y_zlist;
            if (!zz || zt->Z.zcmp(&zz->Z) <= 0) {
                zt->next = zz;
                y_zlist = zt;
            }
            else {
                for ( ; zz->next; zz = zz->next) {
                    if (zt->Z.zcmp(&zz->next->Z) <= 0)
                        break;
                }
                zt->next = zz->next;
                zz->next = zt;
            }
            if (zt->Z.yl < y_yl)
                y_yl = zt->Z.yl;
        }

    // geo_ylist.cc
    Ylist *find_bottom(geo_ylist::topoly_t*);
    Ylist *find_top(geo_ylist::topoly_t*);
    Ylist *find_left(geo_ylist::topoly_t*);
    Ylist *find_right(geo_ylist::topoly_t*);
    Ylist *find_left_nm(geo_ylist::topoly_t*);
    Ylist *find_right_nm(geo_ylist::topoly_t*);
    Zlist *overlapping(const Zoid*) const;
    Ylist *touching(Zlist**, const Zoid*);
    Ylist *first(Zlist**);
    void insert(Zlist*);

    static void remove_common_zoids(Ylist*, Ylist*);

public:
    Ylist *next;        // next->y_yu, < y_yu
    Zlist *y_zlist;     // list of zoids, each have Z.yu == y_yu
    int y_yu;           // top of zoids
    int y_yl;           // minimum Z.yl in y_zlist
};


// Static function.
//
inline Zlist *
Zlist::repartition(Zlist *z0) throw (XIrt)
{
    if (!z0 || !z0->next)
        return (z0);
    Ylist *yl = new Ylist(z0);
    return (yl->repartition());
}


// Static function.
//
inline Zlist *
Zlist::repartition_ni(Zlist *z0)
{
    if (!z0 || !z0->next)
        return (z0);
    Ylist *yl = new Ylist(z0);
    return (yl->repartition_ni());
}

#endif

