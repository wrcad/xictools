
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

#include "dsp.h"
#include "dsp_snap.h"
#include "dsp_window.h"
#include "dsp_inlines.h"
#include "cd_lgen.h"


//
// Object methods for taking care of grid and edge snapping.
//

void
DSPsnapper::snap(WindowDesc *wdesc, int *x, int *y)
{
    if (!wdesc)
        return;
    sn_mode = wdesc->Mode();
    sn_x_init = *x;
    sn_y_init = *y;
    sn_x_final = *x;
    sn_y_final = *y;
    sn_vertex = false;
    sn_x_snapped = false;
    sn_y_snapped = false;

    GridDesc *g = wdesc->Attrib()->grid(wdesc->Mode());
    int fr, cr;
    if (sn_mode == Physical) {
        if (DSP()->NoGridSnapping())
            fr = cr = 1;
        else {
            double spa = g->spacing(Physical);
            if (g->snap() < 0) {
                fr = INTERNAL_UNITS(spa);
                cr = fr;
            }
            else {
                fr = INTERNAL_UNITS(spa);
                cr = INTERNAL_UNITS(spa*g->snap());
            }
        }
    }
    else {
        double spa = g->spacing(Electrical);
        if (g->snap() < 0) {
            fr = ELEC_INTERNAL_UNITS(spa);
            cr = fr;
        }
        else {
            fr = ELEC_INTERNAL_UNITS(spa);
            cr = ELEC_INTERNAL_UNITS(spa*g->snap());
        }
    }
    sn_coarse_resol = cr;
    sn_fine_resol = fr;

    if (sn_mode == Electrical) {
        int delta = 1 + (int)(DSP()->PixelDelta()/wdesc->Ratio());
        int flg, nx, ny;
        if (wdesc->FindContact(sn_x_final, sn_y_final, &nx, &ny, &flg,
                delta, 0, false)) {
            if (flg & FC_CX) {
                sn_x_final = nx;
                sn_x_snapped = true;
            }
            if (flg & FC_CY) {
                sn_y_final = ny;
                sn_y_snapped = true;
            }
        }
        if (wdesc->FindBterm(sn_x_final, sn_y_final, &nx, &ny, &flg,
                delta, 0, false)) {
            if (flg & FC_CX) {
                sn_x_final = nx;
                sn_x_snapped = true;
            }
            if (flg & FC_CY) {
                sn_y_final = ny;
                sn_y_snapped = true;
            }
        }
    }

    if (sn_use_edges && sn_mode == Physical) {
        check_snap_edges(wdesc,
            1 + (int)(DSP()->PixelDelta()/wdesc->Ratio()));
    }
    bool x_set = sn_x_snapped;
    bool y_set = sn_y_snapped;

    if (!sn_x_snapped)
        sn_x_snapped = check_snap(&sn_x_final);
    if (!sn_y_snapped) {
        // Do the YScale here, so that the snap position will
        // always match the displayed grid.

        fr = sn_fine_resol;
        sn_fine_resol = mmRnd(sn_fine_resol*wdesc->YScale());
        if (!sn_fine_resol)
            sn_fine_resol = 1;
        cr = sn_coarse_resol;
        sn_coarse_resol = sn_fine_resol*g->snap();
        sn_y_snapped = check_snap(&sn_y_final);
        sn_coarse_resol = cr;
        sn_fine_resol = fr;
    }

    if (sn_indicate && (x_set || y_set)) {
        if (sn_use_edges || sn_mode == Electrical) {
            DSPmainDraw(SetLinestyle(DSP()->BoxLinestyle()))
            if (sn_vertex) {
                int delta = (int)(10.0/wdesc->Ratio());
                BBox BB(sn_x_final - delta, sn_y_final - delta,
                    sn_x_final + delta, sn_y_final + delta);
                wdesc->ShowLineBox(&BB);
            }
            int delta = (int)(8.0/wdesc->Ratio());
            BBox BB(sn_x_final - delta, sn_y_final - delta,
                sn_x_final + delta, sn_y_final + delta);
            wdesc->ShowLineBox(&BB);
            DSPmainDraw(SetLinestyle(0))
        }
    }
    *x = sn_x_final;
    *y = sn_y_final;
}


bool
DSPsnapper::check_snap(int *u)
{
    int ui = *u;
    int j = (ui/sn_coarse_resol)*sn_coarse_resol;
    int k;
    if (ui >= 0) {
        k = ui - j;
        k = ((k + sn_fine_resol/2)/sn_fine_resol)*sn_fine_resol;
        *u = j + k;
    }
    else {
        k = j - ui;
        k = ((k + sn_fine_resol/2)/sn_fine_resol)*sn_fine_resol;
        *u = j - k;
    }
    return (ui != *u);
}


bool
DSPsnapper::check_x_edge(const BBox &BB, int *x, int x0)
{
    if (BB.left <= x0 && BB.right >= x0) {
        if (sn_allow_off_grid || !check_snap(&x0)) {
            *x = x0;
            return (true);
        }
    }
    return (false);
}


bool
DSPsnapper::check_y_edge(const BBox &BB, int *y, int y0)
{
    if (BB.bottom <= y0 && BB.top >= y0) {
        if (sn_allow_off_grid || !check_snap(&y0)) {
            *y = y0;
            return (true);
        }
    }
    return (false);
}


bool
DSPsnapper::check_region(int u, int v, int p)
{
    if (u < v)
        return (u <= p && p <= v);
    return (v <= p && p <= u);
}


void
DSPsnapper::check_box(const BBox &oBB, const BBox &eBB)
{
    bool isvx = !sn_x_snapped && !sn_y_snapped;
    if (!sn_x_snapped &&
            check_region(oBB.bottom, oBB.top, sn_y_final) &&
            (check_x_edge(eBB, &sn_x_final, oBB.left) ||
            check_x_edge(eBB, &sn_x_final, oBB.right)))
        sn_x_snapped = true;
    if (!sn_y_snapped &&
            check_region(oBB.left, oBB.right, sn_x_final) &&
            (check_y_edge(eBB, &sn_y_final, oBB.bottom) ||
            check_y_edge(eBB, &sn_y_final, oBB.top)))
        sn_y_snapped = true;
    if (isvx && sn_x_snapped && sn_y_snapped)
        sn_vertex = true;
}


void
DSPsnapper::check_poly(const Poly &po, const BBox &eBB, bool incomplete)
{
    Point *p = po.points;
    int nstart = 1;
    int nend = po.numpts;
    if (incomplete) {
        // Polygon being created, locate start/end vertex.
        nstart++;
        nend -= 2;
        if (!sn_x_snapped && !sn_y_snapped &&
                check_x_edge(eBB, &sn_x_final, p[0].x) &&
                check_y_edge(eBB, &sn_y_final, p[0].y)) {
            sn_x_snapped = true;
            sn_y_snapped = true;
            sn_vertex = true;
            return;
        }
        if (!sn_x_snapped && !sn_y_snapped &&
                check_x_edge(eBB, &sn_x_final, p[nend].x) &&
                check_y_edge(eBB, &sn_y_final, p[nend].y)) {
            sn_x_snapped = true;
            sn_y_snapped = true;
            sn_vertex = true;
            return;
        }
    }
    for (int i = nstart; i < nend; i++) {
        if (!sn_x_snapped && !sn_y_snapped &&
                check_x_edge(eBB, &sn_x_final, p[i].x) &&
                check_y_edge(eBB, &sn_y_final, p[i].y)) {
            sn_x_snapped = true;
            sn_y_snapped = true;
            sn_vertex = true;
            return;
        }

        if (p[i].x == p[i-1].x) {
            if (!sn_x_snapped &&
                    check_region(p[i].y, p[i-1].y, sn_y_final) &&
                    check_x_edge(eBB, &sn_x_final, p[i].x)) {
                sn_x_snapped = true;
                return;
            }
        }
        else if (p[i].y == p[i-1].y) {
            if (!sn_y_snapped &&
                    check_region(p[i].x, p[i-1].x, sn_x_final) &&
                    check_y_edge(eBB, &sn_y_final, p[i].y)) {
                sn_y_snapped = true;
                return;
            }
        }
        else if (sn_do_non_manh &&
                check_region(p[i].y, p[i-1].y, sn_y_final) &&
                check_region(p[i].x, p[i-1].x, sn_x_final)) {
            if (!sn_x_snapped) {
                double dx = p[i].x - p[i-1].x;
                double dy = p[i].y - p[i-1].y;
                int yi = sn_y_final;
                check_snap(&yi);
                int xi = p[i-1].x + mmRnd((dx/dy)*(yi - p[i-1].y));
                if (check_x_edge(eBB, &sn_x_final, xi)) {
                    sn_x_snapped = true;
                    return;
                }
            }
            if (!sn_y_snapped) {
                double dx = p[i].x - p[i-1].x;
                double dy = p[i].y - p[i-1].y;
                int xi = sn_x_final;
                check_snap(&xi);
                int yi = p[i-1].y + mmRnd((dy/dx)*(xi - p[i-1].x));
                if (check_y_edge(eBB, &sn_y_final, yi)) {
                    sn_y_snapped = true;
                    return;
                }
            }
        }
    }
}


void
DSPsnapper::check_wire(const Wire &w, const BBox &eBB, bool incomplete)
{
    Point *p = w.points;
    if (incomplete) {
        // Wire being created, locate end vertex.
        if (!sn_x_snapped && !sn_y_snapped &&
                check_x_edge(eBB, &sn_x_final, p[w.numpts - 1].x) &&
                check_y_edge(eBB, &sn_y_final, p[w.numpts - 1].y)) {
            sn_x_snapped = true;
            sn_y_snapped = true;
            sn_vertex = true;
        }
        return;
    }
    if (sn_do_wire_edges && sn_mode == Physical && w.wire_width() > 1) {
        Poly po;
        if (w.toPoly(&po.points, &po.numpts)) {
            check_poly(po, eBB, false);
            delete [] po.points;
        }
        if (sn_x_snapped || sn_y_snapped)
            return;
    }

    for (int i = 0; i < w.numpts; i++) {
        if (!sn_x_snapped && !sn_y_snapped &&
                check_x_edge(eBB, &sn_x_final, p[i].x) &&
                check_y_edge(eBB, &sn_y_final, p[i].y)) {
            sn_x_snapped = true;
            sn_y_snapped = true;
            sn_vertex = true;
            return;
        }
        if (i == 0)
            continue;

        if (sn_do_wire_path || sn_mode == Electrical) {
            if (p[i].x == p[i-1].x) {
                if (!sn_x_snapped &&
                        check_region(p[i].y, p[i-1].y, sn_y_final) &&
                        check_x_edge(eBB, &sn_x_final, p[i].x)) {
                    sn_x_snapped = true;
                    return;
                }
            }
            else if (p[i].y == p[i-1].y) {
                if (!sn_y_snapped &&
                        check_region(p[i].x, p[i-1].x, sn_x_final) &&
                        check_y_edge(eBB, &sn_y_final, p[i].y)) {
                    sn_y_snapped = true;
                    return;
                }
            }
            else if (sn_do_non_manh &&
                    check_region(p[i].y, p[i-1].y, sn_y_final) &&
                    check_region(p[i].x, p[i-1].x, sn_x_final)) {
                if (!sn_x_snapped) {
                    double dx = p[i].x - p[i-1].x;
                    double dy = p[i].y - p[i-1].y;
                    int yi = sn_y_final;
                    check_snap(&yi);
                    int xi = p[i-1].x + mmRnd((dx/dy)*(yi - p[i-1].y));
                    if (check_x_edge(eBB, &sn_x_final, xi)) {
                        sn_x_snapped = true;
                        return;
                    }
                }
                if (!sn_y_snapped) {
                    double dx = p[i].x - p[i-1].x;
                    double dy = p[i].y - p[i-1].y;
                    int xi = sn_x_final;
                    check_snap(&xi);
                    int yi = p[i-1].y + mmRnd((dy/dx)*(xi - p[i-1].x));
                    if (check_y_edge(eBB, &sn_y_final, yi)) {
                        sn_y_snapped = true;
                        return;
                    }
                }
            }
        }
    }
}


void
DSPsnapper::check_snap_edges(WindowDesc *wdesc, int delta)
{
    if (!wdesc)
        return;
    if (wdesc->DbType() == WDcddb) {
        if (sn_mode != Physical)
            return;
        CDs *sdesc = CurCell(Physical);
        if (!sdesc)
            return;
        BBox BB(sn_x_final, sn_y_final, sn_x_final, sn_y_final);
        BB.bloat(delta);

        int depth = wdesc->Attrib()->expand_level(DSP()->CurMode());
        if (depth < 0)
            depth = CDMAXCALLDEPTH;

        CDlgen lgen(Physical, CDlgen::TopToBotWithCells);
        CDl *ldesc;
        int cnt = 0;
        while ((ldesc = lgen.next()) != 0) {
            if (ldesc->isInvisible() && ldesc != CellLayer())
                continue;
            sPF gen(sdesc, &BB, ldesc, depth);
            gen.set_info_mode(wdesc->DisplFlags());
            CDo *odesc;
            while ((odesc = gen.next(false, false)) != 0) {
                if (odesc->next_odesc()->type() == CDLABEL) {
                    delete odesc;
                    continue;
                }
                cnt++;
                if (odesc->type() == CDBOX || odesc->type() == CDINSTANCE)
                    check_box(odesc->oBB(), BB);
                else if (odesc->type() == CDPOLYGON) {
                    const Poly po(((const CDpo*)odesc)->po_poly());
                    check_poly(po, BB, (odesc->state() == CDIncomplete));
                }
                else if (odesc->type() == CDWIRE) {
                    const Wire w(((const CDw*)odesc)->w_wire());
                    check_wire(w, BB, (odesc->state() == CDIncomplete));
                }
                delete odesc;
                if (sn_x_snapped && sn_y_snapped)
                    break;
            }
            if (sn_x_snapped && sn_y_snapped)
                break;

            if (cnt > 25) {
                // If too many objects, give up since they
                // probably can't be differentiated visually.

                if (sn_x_snapped) {
                    sn_x_final = sn_x_init;
                    sn_x_snapped = false;
                }
                if (sn_y_snapped) {
                    sn_y_final = sn_y_init;
                    sn_y_snapped = false;
                }
                return;
            }
        }
    }
}

