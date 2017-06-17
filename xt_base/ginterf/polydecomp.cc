
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
 $Id: polydecomp.cc,v 2.3 2008/07/13 06:17:35 stevew Exp $
 *========================================================================*/

#include "graphics.h"
#include "polydecomp.h"

//
// Function to render a polygon as a collection of trapezoids passed
// to the drawing function.  This should draw any polygon, using the
// fill rule.  All arithmetic is integer, appropriate for typical
// graphical device coordinates.
//

poly_decomp poly_decomp::pd_instance;


void
poly_decomp::polygon(GRdraw *drw, GRmultiPt *data, int num)
{
    // This assumes that the path is closed
    if (num < 4) {
        // poly is degenerate
        return;
    }

    setup(data, num);

    // Starting at the top of the polygon, list the intersecting edge
    // segments in each band of the Y-points list.

    int ytop = pd_ypts[0], ybot;
    for (int i = 1; i < num; i++) {
        if (pd_ypts[i] == ytop)
            continue;
        ybot = pd_ypts[i];
        int sgcnt = 0;
        seg *su = pd_segs_up;
        seg *sd = pd_segs_dn;
        bool non_manh = false;
        for (int j = 0; j < pd_nedges; j++) {
            GRmultiPt::pt_i p0 = data->at(pd_edges[j]);
            GRmultiPt::pt_i p1 = data->at(pd_edges[j] + 1);
            if (p1.y > p0.y) {
                // Rising edgs.
                if (ytop <= p0.y || ybot >= p1.y)
                    continue;
                int dx = p1.x - p0.x;
                if (dx) {
                    int dy = p1.y - p0.y;
                    su->xl = ((ybot - p0.y)*dx)/dy + p0.x;
                    su->xu = ((ytop - p0.y)*dx)/dy + p0.x;
                    non_manh = true;
                }
                else {
                    su->xl = p0.x;
                    su->xu = p0.x;
                }
                su++;
                sgcnt++;
            }
            else {
                // Falling edge.
                if (ytop <= p1.y || ybot >= p0.y)
                    continue;
                int dx = p1.x - p0.x;
                if (dx) {
                    int dy = p1.y - p0.y;
                    sd->xl = ((ybot - p1.y)*dx)/dy + p1.x;
                    sd->xu = ((ytop - p1.y)*dx)/dy + p1.x;
                    non_manh = true;
                }
                else {
                    sd->xl = p1.x;
                    sd->xu = p1.x;
                }
                sd++;
                sgcnt++;
            }
        }

        // The sgcnt should be an even number twice as long as either
        // list.
        sgcnt >>= 1;

        // We now have unsorted lists of all of the rising and falling
        // edges, clipped to the ybot-ytop band.  The next step is to
        // check if any of these edges cross, and make a list of the Y
        // intersection values.  This can only happen if an edge was
        // non-manhattan.

        int intcnt = 0;
        if (non_manh) {
            for (int j = 0; j < sgcnt; j++) {
                for (int k = 0; k < sgcnt; k++) {

                    seg *s1 = pd_segs_up + j;
                    seg *s2 = pd_segs_dn + k;
                    if ((s1->xu < s2->xu && s1->xl > s2->xl) ||
                            (s1->xu > s2->xu && s1->xl < s2->xl)) {

                        int dxu = s1->xu - s1->xl;
                        int dxll = s1->xl - s2->xl;
                        int dxd = s2->xu - s2->xl;
                        pd_tmp_ypts[intcnt++] =
                            ybot + ((ytop - ybot)*dxll)/(dxd - dxu);
                    }

                    if (k <= j)
                        continue;

                    s1 = pd_segs_up + j;
                    s2 = pd_segs_up + k;
                    if ((s1->xu < s2->xu && s1->xl > s2->xl) ||
                            (s1->xu > s2->xu && s1->xl < s2->xl)) {

                        int dxu = s1->xu - s1->xl;
                        int dxll = s1->xl - s2->xl;
                        int dxd = s2->xu - s2->xl;
                        pd_tmp_ypts[intcnt++] =
                            ybot + ((ytop - ybot)*dxll)/(dxd - dxu);
                    }

                    s1 = pd_segs_dn + j;
                    s2 = pd_segs_dn + k;
                    if ((s1->xu < s2->xu && s1->xl > s2->xl) ||
                            (s1->xu > s2->xu && s1->xl < s2->xl)) {

                        int dxu = s1->xu - s1->xl;
                        int dxll = s1->xl - s2->xl;
                        int dxd = s2->xu - s2->xl;
                        pd_tmp_ypts[intcnt++] =
                            ybot + ((ytop - ybot)*dxll)/(dxd - dxu);
                    }
                }
            }
        }

        // If there were intersections, we look in the sub-bands and
        // compute new intersection lists.

        if (intcnt) {
            // Add the endpoints to the intersection Y-points list, and
            // sort the list.
            pd_tmp_ypts[intcnt++] = ytop;
            pd_tmp_ypts[intcnt++] = ybot;
            std::sort(pd_tmp_ypts, pd_tmp_ypts + intcnt, icmp);

            // Loop throught the sub-bands, top to bottom.
            int ttop = pd_tmp_ypts[0], tbot;
            for (int j = 1; j < intcnt; j++) {
                if (pd_tmp_ypts[j] == ttop)
                    continue;
                tbot = pd_tmp_ypts[j];

                int tsgcnt = 0;
                seg *tsu = pd_tmp_segs_up;
                for (int k = 0; k < sgcnt; k++) {

                    int x = pd_segs_up[k].xl;
                    int dx = pd_segs_up[k].xu - x;
                    if (dx) {
                        int dy = ytop - ybot;
                        tsu->xl = ((tbot - ybot)*dx)/dy + x;
                        tsu->xu = ((ttop - ybot)*dx)/dy + x;
                    }
                    else {
                        tsu->xl = x;
                        tsu->xu = x;
                    }
                    tsu++;
                    tsgcnt++;
                }

                seg *tsd = pd_tmp_segs_dn;
                for (int k = 0; k < sgcnt; k++) {

                    int x = pd_segs_dn[k].xl;
                    int dx = pd_segs_dn[k].xu - x;
                    if (dx) {
                        int dy = ytop - ybot;
                        tsd->xl = ((tbot - ybot)*dx)/dy + x;
                        tsd->xu = ((ttop - ybot)*dx)/dy + x;
                    }
                    else {
                        tsd->xl = x;
                        tsd->xu = x;
                    }
                    tsd++;
                    tsgcnt++;
                }
                tsgcnt >>= 1;

                // Sort and output the sub-band edges.
                std::sort(pd_tmp_segs_up, pd_tmp_segs_up + tsgcnt, scmp);
                std::sort(pd_tmp_segs_dn, pd_tmp_segs_dn + tsgcnt, scmp);
                to_zoids(drw, pd_tmp_segs_up, pd_tmp_segs_dn, tsgcnt,
                    tbot, ttop);
                ttop = tbot;
            }
        }
        else {
            // Sort and output the band edges.
            std::sort(pd_segs_up, pd_segs_up + sgcnt, scmp);
            std::sort(pd_segs_dn, pd_segs_dn + sgcnt, scmp);
            to_zoids(drw, pd_segs_up, pd_segs_dn, sgcnt, ybot, ytop);
        }
        ytop = ybot;
    }
}

