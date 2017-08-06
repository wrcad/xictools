
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
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef POLYDECOMP_H
#define POLYDECOMP_H


#include <algorithm>

// Struct for polygon decomposition into trapezoids, for rendering.
//
// This manages internal buffer areas, which are resized as necessary,
// but not freed (except in the destructor).  This is allocated as a
// static instance available throught the instance() method.  Separate
// instances can be created and destroyed using the constructor and
// destructor.
//
// All arithmetic uses integers, suitable for viewport coordinates.

#define PO_MINSIZE 64

struct poly_decomp
{
    struct seg { int xl, xu; };

    poly_decomp()
        {
            pd_ypts = 0;
            pd_tmp_ypts = 0;
            pd_edges = 0;
            pd_segs_up = 0;
            pd_segs_dn = 0;
            pd_tmp_segs_up = 0;
            pd_tmp_segs_dn = 0;
            pd_size = 0;
        }

    ~poly_decomp()
        {
            delete [] pd_ypts;
            delete [] pd_tmp_ypts;
            delete [] pd_edges;
            delete [] pd_segs_up;
            delete [] pd_segs_dn;
            delete [] pd_tmp_segs_up;
            delete [] pd_tmp_segs_dn;
        }

    void polygon(GRdraw*, GRmultiPt*, int);

    static poly_decomp &instance() { return (pd_instance); }

private:
    void setup(GRmultiPt *pts, int num)
        {
            if (num > pd_size) {
                // Resize buffer stuff.
                delete [] pd_ypts;
                delete [] pd_tmp_ypts;
                delete [] pd_edges;
                delete [] pd_segs_up;
                delete [] pd_segs_dn;
                delete [] pd_tmp_segs_up;
                delete [] pd_tmp_segs_dn;
                pd_size = num + (num >> 1);
                if (pd_size < PO_MINSIZE)
                    pd_size = PO_MINSIZE;
                pd_ypts = new int[pd_size];
                pd_edges = new int[pd_size];
                int sz2 = pd_size >> 1;
                pd_tmp_ypts = new int[sz2];
                pd_segs_up = new seg[sz2];
                pd_segs_dn = new seg[sz2];
                pd_tmp_segs_up = new seg[sz2];
                pd_tmp_segs_dn = new seg[sz2];
            }

            // Create a list of y values, and a list of edge segs
            // (keeping only non-horizontal).
            int i, nm = num-1;
            for (pd_nedges = 0, i = 0; i < nm; i++) {
                pd_ypts[i] = pts->at(i).y;
                if (pts->at(i).y != pts->at(i+1).y)
                    pd_edges[pd_nedges++] = i;
            }
            pd_ypts[i] = pts->at(i).y;

            // Sort ypts descending.
            //
            std::sort(pd_ypts, pd_ypts + num, icmp);
        }

    // Construct and draw trapezoids using the up and down edges,
    // taken in sequence (they are sorted, and do not intersect).
    //
    static void to_zoids(GRdraw *drw, seg *segs_up, seg *segs_dn,
        int nsegs, int ybot, int ytop)
        {
            seg *su = segs_up;
            seg *sd = segs_dn;
            while (nsegs--) {
                if (su->xl != sd->xl || su->xu != sd->xu) {
                    int xll, xlr;
                    if (su->xl < sd->xl) {
                        xll = su->xl;
                        xlr = sd->xl;
                    }
                    else {
                        xll = sd->xl;
                        xlr = su->xl;
                    }
                    int xul, xur;
                    if (su->xu < sd->xu) {
                        xul = su->xu;
                        xur = sd->xu;
                    }
                    else {
                        xul = sd->xu;
                        xur = su->xu;
                    }
                    drw->Zoid(ybot, ytop, xll, xul, xlr, xur);
                }
                su++;
                sd++;
            }
        }

    // Sort comparison for non-overlapping edges.
    static bool scmp(const seg &s1, const seg &s2)
        { return (s1.xl + s1.xu < s2.xl + s2.xu); }

    // Integer array sort, descending.
    static bool icmp(const int &i1, const int &i2)
        { return (i2 < i1); }

    int *pd_ypts;
    int *pd_tmp_ypts;
    int *pd_edges;
    seg *pd_segs_up;
    seg *pd_segs_dn;
    seg *pd_tmp_segs_up;
    seg *pd_tmp_segs_dn;
    int pd_size;
    int pd_nedges;

    static poly_decomp pd_instance;
};

#endif

