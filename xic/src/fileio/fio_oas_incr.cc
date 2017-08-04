
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

#include "fio.h"
#include "fio_chd.h"
#include "fio_oasis.h"


//
// The incremental byte stream reader.
//

cv_incr_reader::cv_incr_reader(oas_byte_stream *bstream,
    const FIOcvtPrms *prms) : ir_oas(false)
{
    ir_scale = 1.0;
    ir_object = 0;
    ir_ldesc = 0;
    ir_bstream = bstream;  // this will be destroyed in destructor
    ir_useAOI = false;
    ir_clip = false;
    ir_error = false;

    ir_oas.set_byte_stream(bstream);

    if (prms->scale() < .001 || prms->scale() > 1000.0) {
        Errs()->add_error("bad scale.");
        ir_error = true;
        return;
    }
    ir_scale = prms->scale();

    if (prms->use_window()) {
        ir_useAOI = true;
        ir_clip = prms->clip();
        ir_AOI = *prms->window();
    }
    if (!ir_oas.setup_backend(this)) {
        Errs()->add_error("setup_backend failed.");
        ir_error = true;
        return;
    }
}


cv_incr_reader::~cv_incr_reader()
{
    ir_oas.setup_backend(0);
    delete ir_bstream;
}


// This returns objects from the byte stream until the byte stream is
// exhausted.  The objects should be freed after use.
//
CDo*
cv_incr_reader::next_object()
{
    if (ir_object) {
        CDo *odesc = ir_object;
        ir_object = ir_object->next_odesc();
        odesc->set_next_odesc(0);
        return (odesc);
    }
    for (;;) {
        oasState state = ir_oas.get_state();
        if (state == oasNeedInit || state == oasNeedRecord)
            ir_oas.parse_incremental(ir_scale);
        if (state == oasDone || state == oasError)
            break;
        if (state == oasHasPlacement)
            ir_oas.run_placement();
        else if (state == oasHasText)
            ir_oas.run_text();
        else if (state == oasHasRectangle)
            ir_oas.run_rectangle();
        else if (state == oasHasPolygon)
            ir_oas.run_polygon();
        else if (state == oasHasPath)
            ir_oas.run_path();
        else if (state == oasHasTrapezoid)
            ir_oas.run_trapezoid();
        else if (state == oasHasCtrapezoid)
            ir_oas.run_ctrapezoid();
        else if (state == oasHasCircle)
            ir_oas.run_circle();
        else if (state == oasHasXgeometry)
            ir_oas.run_xgeometry();
        if (ir_object) {
            CDo *odesc = ir_object;
            ir_object = ir_object->next_odesc();
            odesc->set_next_odesc(0);
            return (odesc);
        }
    }
    return (0);
}


bool
cv_incr_reader::write_box(const BBox *BB)
{
    BBox nBB = *BB;
    if (!ir_useAOI || nBB.intersect(&ir_AOI, false)) {
        if (ir_clip) {
            if (nBB.left < ir_AOI.left)
                nBB.left = ir_AOI.left;
            if (nBB.bottom < ir_AOI.bottom)
                nBB.bottom = ir_AOI.bottom;
            if (nBB.right > ir_AOI.right)
                nBB.right = ir_AOI.right;
            if (nBB.top > ir_AOI.top)
                nBB.top = ir_AOI.top;
        }
        ir_object = new CDo(ir_ldesc, &nBB);
        ir_object->set_copy(true);
    }
    return (true);
}


bool
cv_incr_reader::write_poly(const Poly *po)
{
    if (!ir_useAOI || po->intersect(&ir_AOI, false)) {
        bool need_out = true;
        if (ir_clip) {
            need_out = false;
            PolyList *pl = po->clip(&ir_AOI, &need_out);
            CDo *oend = 0;
            for (PolyList *px = pl; px; px = px->next) {
                if (!oend)
                    ir_object = oend = new CDpo(ir_ldesc, &px->po);
                else {
                    oend->set_next_odesc(new CDpo(ir_ldesc, &px->po));
                    oend = oend->next_odesc();
                }
                oend->set_copy(true);
                px->po.points = 0;
            }
            PolyList::destroy(pl);
        }
        if (need_out) {
            Poly poly(*po);
            poly.points = new Point[po->numpts];
            memcpy(poly.points, po->points, po->numpts*sizeof(Point));
            ir_object = new CDpo(ir_ldesc, &poly);
            ir_object->set_copy(true);
        }
    }
    return (true);
}


bool
cv_incr_reader::write_wire(const Wire *w)
{
    Poly wp;
    if (!ir_useAOI || (w->toPoly(&wp.points, &wp.numpts) &&
            wp.intersect(&ir_AOI, false))) {
        bool need_out = true;
        if (ir_clip) {
            need_out = false;
            PolyList *pl = wp.clip(&ir_AOI, &need_out);
            CDo *oend = 0;
            for (PolyList *px = pl; px; px = px->next) {
                if (!oend)
                    ir_object = oend = new CDpo(ir_ldesc, &px->po);
                else {
                    oend->set_next_odesc(new CDpo(ir_ldesc, &px->po));
                    oend = oend->next_odesc();
                }
                oend->set_copy(true);
                px->po.points = 0;
            }
            PolyList::destroy(pl);
        }
        if (need_out) {
            Wire wire(*w);
            wire.points = new Point[w->numpts];
            memcpy(wire.points, w->points, w->numpts*sizeof(Point));
            ir_object = new CDw(ir_ldesc, &wire);
            ir_object->set_copy(true);
        }
    }
    delete [] wp.points;
    return (true);
}

