
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef POLYSPLIT_H
#define POLYSPLIT_H

//
// The following code performs trapezoid decomp for ploygons.
//

// Stack space reserved for vertex/edge storage, additional
// allocated from heap.
#define PS_STACK_VERTS 64

// Stack space reserved for trapezoids, additional allocated from heap.
#define PS_ZBLKSZ 64

namespace qtinterf
{
    // Test if polygon is complex
    extern bool test_convex(GRmultiPt::pt_i*, int);

    // Returned trapezoid linked element, these are owned by polysplit.
    //
    struct zoid_t
    {
        bool join_above(zoid_t*);

        bool is_rect() { return (xll == xlr && xul == xur); }

        zoid_t *next;
        int xll, xlr, yl;
        int xul, xur, yu;
    };

    // Allocation block of zoid_t elements.
    //
    struct zblk_t
    {
        zblk_t(zblk_t *n = 0) { next = n; }

        zoid_t zoids[PS_ZBLKSZ];
        zblk_t *next;
    };

    // Intersecting edge, for polysplit.
    //
    struct isect_t
    {
        double sl;  // dx/dy
        int xbot;   // X intersect along bottom
        int xtop;   // X intersect along top
        int wrap;   // +1 ascending, -1 descending
    };

    // Save an edge or vertex, for polysplit.
    //
    struct etype_t
    {
        GRmultiPt::pt_i pt; // vertex containing largest Y value
        GRmultiPt::pt_i pb; // other vertex
        int asc;            // code: 1 ascending, -1 descending, 0 place holder
    };

    // Struct to split a polygon into a trapezoid list.
    //
    struct polysplit
    {
        polysplit(GRmultiPt::pt_i*, int, int = 2);
        ~polysplit();
        zoid_t *extract_zlist();

    private:
        void extract_row_intersect(int);
        void create_row_zlist();
        void add_row();
        zoid_t *new_zoid(int, int, int, int, int, int);

        etype_t *new_etype(GRmultiPt::pt_i *p1, GRmultiPt::pt_i *p2, int a)
            {
                etype_t *e = ps_etype_reserv + ps_ecnt;
                e->pt = *p1;
                e->pb = *p2;
                e->asc = a;
                return (e);
            }

        isect_t *new_isect(int b, int t, double s, int w)
            {
                isect_t *i = ps_isect_reserv + ps_icnt;
                i->xbot = b;
                i->xtop = t;
                i->sl = s;
                i->wrap = w;
                return (i);
            }

        etype_t **ps_etype_array;       // sorted edges and vertex markers
        isect_t **ps_isect_array;       // sorted intersections
        zoid_t *ps_z0;                  // cumulative trapezoid list
        zoid_t *ps_zrow;                // current row of trapezoids
        zoid_t *ps_zrow_end;            // end pointer for current list

        etype_t *ps_etype_reserv;       // allocation pool for edges
        isect_t *ps_isect_reserv;       // allocation pool for intersects

        int ps_ybot;                    // current y-range bottom
        int ps_ytop;                    // current y-range top
        int ps_ecnt;                    // edges/vertex markers used
        int ps_icnt;                    // intersects used

        // Memory pool for small polys, to avoid malloc/free.
        // A "peak" vertex ( /\ ) will require 2 etype_t elements, all
        // others only one.  Total needed is less than nv + nv/2.

        etype_t *ps_ea[PS_STACK_VERTS + PS_STACK_VERTS/2];
        isect_t *ps_ia[PS_STACK_VERTS];
        etype_t ps_epool[PS_STACK_VERTS + PS_STACK_VERTS/2];
        isect_t ps_ipool[PS_STACK_VERTS];
        int min_size;                   // size filter
        bool ps_use_heap;               // true when pool from heap

        zblk_t initblk;
        int zelcnt;
    };
}

#endif

