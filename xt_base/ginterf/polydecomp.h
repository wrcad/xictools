
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id: polydecomp.h,v 2.2 2008/03/02 07:24:00 stevew Exp $
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

