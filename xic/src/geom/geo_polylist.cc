
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
 $Id: geo_polylist.cc,v 1.12 2012/06/24 05:11:02 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "geo_zlist.h"


// Static functions to create a PolyList from a Zlist.
//
PolyList *
PolyList::new_poly_list(Zlist *zl, bool vert)
{
    PolyList *plist = 0, *pe = 0;
    for (Zlist *z = zl; z; z = z->next) {
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


// Convert the list to a chain of polygon object copies, the list is
// destroyed.
//
CDo *
PolyList::to_odesc(CDl *ld)
{
    if (!ld)
        return (0);
    CDo *od0 = 0;
    PolyList *p = this;
    while (p) {
        CDo *od;
        if (p->po.is_rect()) {
            BBox BB(p->po.points);
            delete [] p->po.points;
            od = new CDo(ld, &BB);
        }
        else
            od = new CDpo(ld, &p->po);
        od->set_copy(true);
        od->set_next_odesc(od0);
        od0 = od;
        p->po.points = 0;
        PolyList *px = p;
        p = p->next;
        delete px;
    }
    return (od0);
}


// Convert the list to an object list of polygon descriptors, the list
// is destroyed.
//
CDol *
PolyList::to_olist(CDl *ld, CDol **endp)
{
    if (!ld)
        return (0);
    CDol *ol0 = endp ? *endp : 0;
    CDol *oe = ol0;
    PolyList *p = this;
    while (p) {
        CDo *od;
        if (p->po.is_rect()) {
            BBox BB(p->po.points);
            delete [] p->po.points;
            od = new CDo(ld, &BB);
        }
        else
            od = new CDpo(ld, &p->po);
        od->set_copy(true);
        if (!ol0)
            ol0 = oe = new CDol(od, 0);
        else {
            oe->next = new CDol(od, 0);
            oe = oe->next;
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


void
PolyList::free()
{
    PolyList *p = this;
    while (p) {
        PolyList *pn = p->next;
        delete p;
        p = pn;
    }
}

