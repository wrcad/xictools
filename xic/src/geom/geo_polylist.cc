
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

#include "cd.h"
#include "cd_types.h"
#include "geo_zlist.h"


// Static function.
// Create a PolyList from a Zlist.
//
PolyList *
PolyList::new_poly_list(const Zlist *zl, bool vert)
{
    PolyList *plist = 0, *pe = 0;
    for (const Zlist *z = zl; z; z = z->next) {
        Poly po;
        if (z->Z.mkpoly(&po.points, &po.numpts, vert)) {
            if (!plist)
                plist = pe = new PolyList(po, 0);
            else {
                pe->next = new PolyList(po, 0);
                pe = pe->next;
            }
        }
    }
    return (plist);
}


// Static function.
// Convert the list to a chain of polygon object copies, the list is
// destroyed.
//
CDo *
PolyList::to_odesc(PolyList *thisp, CDl *ld)
{
    if (!ld)
        return (0);
    CDo *od0 = 0;
    PolyList *p = thisp;
    while (p) {
        CDo *od = 0;
        if (p->po.is_rect()) {
            BBox BB(p->po.points);
            if (BB.valid())
                od = new CDo(ld, &BB);
            delete [] p->po.points;
        }
        else if (p->po.valid())
            od = new CDpo(ld, &p->po);
        if (od) {
            od->set_copy(true);
            od->set_next_odesc(od0);
            od0 = od;
        }
        p->po.points = 0;
        PolyList *px = p;
        p = p->next;
        delete px;
    }
    return (od0);
}


// Static function.
// Convert the list to an object list of polygon descriptors, the list
// is destroyed.
//
CDol *
PolyList::to_olist(PolyList *thisp, CDl *ld, CDol **endp)
{
    if (!ld)
        return (0);
    CDol *ol0 = endp ? *endp : 0;
    CDol *oe = ol0;
    PolyList *p = thisp;
    while (p) {
        CDo *od = 0;
        if (p->po.is_rect()) {
            BBox BB(p->po.points);
            if (BB.valid())
                od = new CDo(ld, &BB);
            delete [] p->po.points;
        }
        else if (p->po.valid())
            od = new CDpo(ld, &p->po);
        if (od) {
            od->set_copy(true);
            if (!ol0)
                ol0 = oe = new CDol(od, 0);
            else {
                oe->next = new CDol(od, 0);
                oe = oe->next;
            }
        }
        p->po.points = 0;
        PolyList *px = p;
        p = p->next;
        delete px;
    }
    if (endp)
        *endp = oe;
    return (ol0);
}

