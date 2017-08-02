
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#include "cd.h"
#include "cd_types.h"
#include "geo_polyobj.h"
#include "geo_zlist.h"


//------------------------------------------------------------------------
// PolyObj Functions
//------------------------------------------------------------------------

PolyObj::PolyObj(const BBox &BB)
{
    po_zlist = 0;
    po_odesc = 0;
    Point *p = new Point[5];
    BB.to_path(p);
    po_po.points = p;
    po_po.numpts = 5;
    po_cwset = true;
    po_cw = true;
    po_isrect = true;
    po_freepts = true;
    int w = BB.width();
    int h = BB.height();
    po_width = mmMin(w, h);
}


PolyObj::PolyObj(Poly &p, bool fp)
{
    po_zlist = 0;
    po_odesc = 0;
    po_po = p;
    po_cwset = false;
    po_cw = false;
    po_isrect = false;
    po_freepts = fp;
    po_width = 0;
}


PolyObj::PolyObj(const CDo *obj, bool ckw)
{
    po_zlist = 0;
    po_odesc = obj;
    po_cw = false;
    po_cwset = false;
    po_isrect = false;
    po_freepts = false;
    if (obj->type() == CDBOX || obj->type() == CDLABEL ||
            obj->type() == CDINSTANCE) {
        int w = obj->oBB().width();
        int h = obj->oBB().height();
        po_width = mmMin(w, h);
        if (ckw && po_width <= 0)
            return;

        Point *p = new Point[5];
        obj->oBB().to_path(p);
        po_po.points = p;
        po_po.numpts = 5;
        po_cwset = true;
        po_cw = true;
        po_isrect = true;
        po_freepts = true;
    }
    else if (obj->type() == CDWIRE) {
        po_width = ((const CDw*)obj)->wire_width();
        if (ckw && po_width <= 0)
            return;

        ((const CDw*)obj)->w_toPoly(&po_po.points, &po_po.numpts);
        po_freepts = true;
        // We know toPoly does this.
        po_cw = true;
        po_cwset = true;
    }
    else if (obj->type() == CDPOLYGON) {
        // Yes, po.points is a pointer into the object, so be careful.
        po_po = ((const CDpo*)obj)->po_poly();
        po_width = 0;
    }
}


PolyObj::~PolyObj()
{
    if (po_freepts)
        delete [] po_po.points;
    Zlist::destroy(po_zlist);
}


void
PolyObj::set_eff_width()
{
    BBox BB;
    po_po.computeBB(&BB);
    int w = BB.width();
    int h = BB.height();
    po_width = mmMin(w, h);
}
