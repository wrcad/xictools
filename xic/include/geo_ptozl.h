
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

#ifndef GEO_PTOZL_H
#define GEO_PTOZL_H

#include <algorithm>


// Class to implement polygon decomposition to a trapezoid list. 
// This works with any polygon, using fill rule to define filled
// regions.  The returned trapezoid list is clipped/merged.

#define PS_BLKSZ 64

class poly_splitter
{
public:
    struct ps_seg
    {
        ps_seg *next;
        int xl;
        int xu;
    };

    struct ps_seg_blk
    {
        ps_seg ary[PS_BLKSZ];
        ps_seg_blk *next;
    };

    struct ps_band
    {
        void sort_segs();
        void check_cross(poly_splitter*);
        bool find_cross(int*);
        Zlist *to_zlist();

        int b_top;
        int b_bot;
        ps_seg *b_segs_up;
        ps_seg *b_segs_dn;
        ps_band *next;
    };

    struct ps_band_blk
    {
        ps_band ary[PS_BLKSZ];
        ps_band_blk *next;
    };

    poly_splitter()
        {
            ps_top_band = 0;
            ps_bands = 0;
            ps_num_bands = 0;

            ps_band_base = ps_static_band_blk.ary;
            ps_seg_base = ps_static_seg_blk.ary;
            ps_bands_used = 0;
            ps_segs_used = 0;
            ps_static_band_blk.next = 0;
            ps_static_seg_blk.next = 0;
        }

    ~poly_splitter()
        {
            delete [] ps_bands;

            ps_band_blk *b = ps_static_band_blk.next;
            while (b) {
                ps_band_blk *x = b;
                b = b->next;
                delete x;
            }

            ps_seg_blk *s = ps_static_seg_blk.next;
            while (s) {
                ps_seg_blk *x = s;
                s = s->next;
                delete x;
            }
        }

    Zlist *split(const Poly&);

private:
    void insert(const Point&, const Point&);
    ps_band *find(int);

    // Sort comparison for non-overlapping edges.
    static bool scmp(const ps_seg *s1, const ps_seg *s2)
        { return (s1->xl + s1->xu < s2->xl + s2->xu); }

    // Integer array sort, descending.
    static bool icmp(const int &i1, const int &i2)
        { return (i2 < i1); }

    ps_band *new_band();
    ps_seg *new_seg();

    ps_band *ps_top_band;
    ps_band **ps_bands;
    int ps_num_bands;

    ps_band *ps_band_base;
    ps_seg *ps_seg_base;
    int ps_bands_used;
    int ps_segs_used;

    ps_band_blk ps_static_band_blk;
    ps_seg_blk ps_static_seg_blk;
};

#endif

