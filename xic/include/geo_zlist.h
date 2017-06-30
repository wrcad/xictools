
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
 $Id: geo_zlist.h,v 5.42 2015/10/30 04:37:52 stevew Exp $
 *========================================================================*/

#ifndef GEO_ZLIST_H
#define GEO_ZLIST_H

#include "geo_memmgr_cfg.h"
#include "geo_zoid.h"

// Mode for bloating corner fill-in.
enum BLCtype { BLCclip, BLCflat, BLCextend, BLCextend1,
    BLCextend2, BLCunused1, BLCunused2, BLCnone };

// Mode flags for Zlist::bloat
#define BL_MODE_MASK        0x3
    // Mask for the mode.  Mode 3 is the "DRC" mode and other flags
    // don't apply.
#define BL_OLD_MODES        0x4
    // Use legacy algorithms, flags below are ignored in this case.
#define BL_EDGE_ONLY        0x8
    // No bloating, return the edge construct used for clipping.

#define BL_CORNER_MODE_MASK 0x70
#define BL_CORNER_MODE_SHIFT 4
    // These bits, when shifted down, provide the corner fill modes,
    // as cast to BLCtype.
#define BL_NO_PRJ_FIX       0x80
    // The corner modes with that extend the corner beyond the bloat
    // distance have an expensive fix to prevent spurrious aberrations.
    // This will turn the fix off.

#define BL_NO_GROUP         0x100
    // Skip intermediate grouping, possible if zoids are part of a
    // mutually touching set.
#define BL_NO_MERGE_IN      0x200
    // Skip merging of input zoids, possible if collection already
    // clip/merged.
#define BL_SCALE_FIX        0x400
    // Scale/unscale operations for obscure numerical reasons.
#define BL_NO_MERGE_OUT     0x800
    // Skip merging of output zoids, applies only to edge collection
    // returned with BL_EDGE_ONLY, or if BL_SCALE_FIX is set.
#define BL_NO_TO_POLY       0x1000
    // Skip linking zoids back to polys (used in cEdit::bloatQueue).

// Defaults for the Zlist::Join parameters.
// Used to be 600, 300, 1000.  Turned off limiting of group/queue
// since these can cause unexpected ugliness in bloating and
// elsewhere.  They can be set by the user to possibly improve speed
// when needed.
// 
#define DEF_JoinMaxVerts    600
#define DEF_JoinMaxGroup    0
#define DEF_JoinMaxQueue    0

// Flags for to_temp_layer.
#define TTLinternal     0x1
#define TTLnoinsert     0x2
#define TTLjoin         0x4

// List of zoids
//
struct Zlist
{
#ifdef GEO_USE_MANAGER
    void *operator new(size_t);
    void operator delete(void*, size_t);
#endif
    Zlist() { next = 0; }
    Zlist(const Zoid *z, Zlist *zn = 0) { next = zn; Z = *z; }
    Zlist(const BBox *pBB, Zlist *zn = 0) : Z(pBB) { next = zn; }
    Zlist(int l, int b, int r, int t, Zlist *zn = 0) :
        Z(l, r, b, l, r, t) { next = zn; }
    Zlist(int xll, int xlr, int yl, int xul, int xur, int yu, Zlist *zn) :
            Z(xll, xlr, yl, xul, xur, yu) { next = zn; };

    static void destroy(const Zlist *zl)
    {
        while (zl) {
            const Zlist *zx = zl;
            zl = zl->next;
            delete zx;
        }
    }

    static double area(const Zlist *thiszl)
        {
            double d = 0.0;
            for (const Zlist *z = thiszl; z; z = z->next) {
                double w = (z->Z.xur + z->Z.xlr) - (z->Z.xul + z->Z.xll);
                double h = z->Z.yu - z->Z.yl;
                d += w*h;
            }
            return (0.5*d/(CDphysResolution*CDphysResolution));
        }

    static bool intersect(const Zlist *thiszl, const Point *p, bool touchok)
        {
            for (const Zlist *z = thiszl; z; z = z->next) {
                if (z->Z.intersect(p, touchok))
                    return (true);
            }
            return (false);
        }

    static bool intersect(const Zlist *thiszl, const BBox *pBB, bool touchok)
        {
            for (const Zlist *z = thiszl; z; z = z->next) {
                if (z->Z.intersect(pBB, touchok))
                    return (true);
            }
            return (false);
        }

    static bool intersect(const Zlist *thiszl, const Zoid *Zx, bool touchok)
        {
            for (const Zlist *z = thiszl; z; z = z->next) {
                if (z->Z.intersect(Zx, touchok))
                    return (true);
            }
            return (false);
        }

    static bool intersect(const Zlist *thiszl, const Zlist *zl, bool touchok)
        {
            for (const Zlist *z = thiszl; z; z = z->next) {
                if (Zlist::intersect(zl, &z->Z, touchok))
                    return (true);
            }
            return (false);
        }

    static int length(const Zlist *thiszl)
        {
            int cnt = 0;
            for (const Zlist *z = thiszl; z; z = z->next, cnt++) ;
            return (cnt);
        }

    static void print(const Zlist *thiszl, FILE *fp = 0)
        {
            for (const Zlist *z = thiszl; z; z = z->next)
                z->Z.print(fp);
        }

    static void show(const Zlist *zl)
        {
            for ( ; zl; zl = zl->next)
                (zl->Z).show();
        }

    static Zlist *expand_by_2(Zlist *thiszl)
        {
            for (Zlist *z = thiszl; z; z = z->next)
                z->Z.expand_by_2();
            return (thiszl);
        }

    static Zlist *shrink_by_2(Zlist *thiszl)
        {
            Zlist *z0 = thiszl, *zp = 0, *zn;
            for (Zlist *z = z0; z; z = zn) {
                zn = z->next;
                z->Z.shrink_by_2();
                if (z->Z.is_bad()) {
                    if (zp)
                        zp->next = zn;
                    else
                        z0 = zn;
                    delete z;
                    continue;
                }
                zp = z;
            }
            return (z0);
        }

    static bool is_manh(const Zlist *thiszl)
        {
            for (const Zlist *z = thiszl; z; z = z->next) {
                if (!z->Z.is_manh())
                    return (false);
            }
            return (true);
        }

    static bool is_45(const Zlist *thiszl)
        {
            for (const Zlist *z = thiszl; z; z = z->next) {
                if (!z->Z.is_45())
                    return (false);
            }
            return (true);
        }

    // geo_ylist.h
    inline static Zlist *repartition(Zlist*) throw (XIrt);
    inline static Zlist *repartition_ni(Zlist*);

    // geo_zlist,cc
    static void BB(const Zlist*, BBox&);
    static Zlist *copy(const Zlist*);
    static Zlist *filter_slivers(Zlist*, int = 0);
    static Zlist *filter_drc_slivers(Zlist*, int = 0);
    static Zlist *sort(Zlist*, int=0);
    static Zlist *bloat(const Zlist*, int, int) throw (XIrt);
    static Zlist *halo(const Zlist*, int) throw (XIrt);
    static Zlist *edges(const Zlist*, int) throw (XIrt);
    static Zlist *wire_edges(const Zlist*, int) throw (XIrt);
    static Zlist *manhattanize(Zlist*, int, int);
    static Zlist *transform(const Zlist*, cTfmStack*);
    static int linewidth(const Zlist*);
    static edg_t *ext_edges(const Zlist*);
    static Zlist *ext_zoids(const Zlist*, int, int);
    static Zgroup *group(Zlist*, int = 0);
    static CDl *to_temp_layer(Zlist*, const char*, int, CDs*, XIrt*);
    static Zlist *to_poly(Zlist*, Point**, int*);
    static PolyList *to_poly_list(Zlist*);
    static XIrt to_poly_add(Zlist*, CDs*, CDl*, bool, const cTfmStack* = 0,
        bool = false);
    static CDo *to_obj_list(Zlist*, CDl*, bool = false);
    static void add(const Zlist*, CDs*, CDl*, bool, bool = false);
    static void add_r(const Zlist*, CDs*, CDl*, bool, bool = false);
    static Zlist *to_r(Zlist*);

    static void reset_join_params();

    // geo_zlfuncs.cc
    static bool zl_intersect(Zlist*, Zlist*, bool);
    static XIrt zl_or(Zlist**, Zlist*);
    static XIrt zl_and(Zlist**);
    static XIrt zl_and(Zlist**, const Zoid*);
    static XIrt zl_and(Zlist**, Zlist*);
    static XIrt zl_and(Zlist**, const Ylist*);
    static XIrt zl_andnot(Zlist**);
    static XIrt zl_andnot(Zlist**, const Zoid*);
    static XIrt zl_andnot(Zlist**, Zlist*);
    static XIrt zl_andnot(Zlist**, const Ylist*);
    static XIrt zl_andnot2(Zlist**, Zlist**);
    static XIrt zl_xor(Zlist**, Zlist*);
    static XIrt zl_bloat(Zlist**, int, int);

    Zlist *next;
    Zoid Z;

    // The "join" parameters, apply when converting Zlist to polygons.
    //
    static int JoinMaxVerts;    // Approx max number of vertices in polys
                                //  created by join.
    static int JoinMaxGroup;    // Max number of zoids used in poly group
                                //  for join operation.
    static int JoinMaxQueue;    // Max number of zoids in toPoly queue for
                                //  join operation.
    static bool JoinBreakClean; // Improve boundary in join when limited.
    static bool JoinSplitWires; // Include wires in join/split.
};

// A couple of helpful constructor wrappers.

inline Zlist *
new_zlist(int xll, int xlr, int yl, int xul, int xur, int yu, Zlist *z0)
{
    Zoid Z(xll, xlr, yl, xul, xur, yu);
    if (Z.is_bad())
        return (z0);
    return (new Zlist(&Z, z0));
}

inline Zlist *
new_zlist(int l, int b, int r, int t, Zlist *z0)
{
    Zoid Z(l, r, b, l, r, t);
    if (Z.is_bad())
        return (z0);
    return (new Zlist(&Z, z0));
}

#endif

