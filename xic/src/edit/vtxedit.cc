
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

#include "config.h"
#include "main.h"
#include "edit.h"
#include "vtxedit.h"
#include "dsp_color.h"
#include "dsp_inlines.h"


//
// Support for the vertex editor, used in polygons and wires, and the
// stretch command.
//

void
cEdit::clearObjectList()
{
    sObj::destroy(ed_object_list);
    ed_object_list = 0;
}


void
cEdit::purgeObjectList(CDo *od)
{
    sObj *op = 0, *on;
    for (sObj *o = ed_object_list; o; o = on) {
        on = o->next_obj();
        if (o->object() == od) {
            if (op)
                op->set_next_obj(on);
            else
                ed_object_list = on;
            delete o;
            continue;
        }
        op = o;
    }
}


bool
cEdit::get_wire_ref(int *xrp, int *yrp, int *xmp, int *ymp)
{
    if (!ed_object_list)
        return (false);
    return (ed_object_list->get_wire_ref(xrp, yrp, xmp, ymp));
}
// End of cEdit functions.


// Static function.
// Return true if there are no movable vertices in the list.
//
bool
sObj::empty(const sObj *thiso)
{
    for (const sObj *o = thiso; o; o = o->o_next) {
        for (const Vertex *v = o->o_pts; v; v = v->cnext()) {
            if (v->movable())
                return (false);
        }
    }
    return (true);
}


// Static function.
// Create, add to, or modify the selected status of the objlist according
// to the objects in slist and the given selection rectangle AOI.  The
// modified list is returned.
//
sObj *
sObj::mklist(sObj *thiso, CDol *slist, BBox *AOI)
{
    sObj *objlist = thiso;
    for (CDol *sl = slist; sl; sl = sl->next) {
        if (!sl->odesc)
            continue;
        if (sl->odesc->type() == CDINSTANCE)
            continue;
        if (sl->odesc->type() == CDLABEL)
            continue;
        if (sl->odesc->state() != CDobjSelected)
            continue;
        if (sl->odesc->type() == CDBOX) {
            Point_c p(sl->odesc->oBB().left, sl->odesc->oBB().bottom);
            if (!AOI->intersect(&p, true)) {
                p.set(sl->odesc->oBB().left, sl->odesc->oBB().top);
                if (!AOI->intersect(&p, true)) {
                    p.set(sl->odesc->oBB().right, sl->odesc->oBB().top);
                    if (!AOI->intersect(&p, true)) {
                        p.set(sl->odesc->oBB().right,
                            sl->odesc->oBB().bottom);
                        if (!AOI->intersect(&p, true))
                            continue;
                    }
                }
            }
            sObj *o;
            for (o = objlist; o; o = o->o_next)
                if (o->o_obj == sl->odesc)
                    break;
            if (o) {
                for (Vertex *v = o->o_pts; v; v = v->next())
                    if (AOI->intersect(v->point(), true))
                        v->set_movable(!v->movable());
            }
            else {
                objlist = new sObj(sl->odesc, objlist);
                Vertex *v = 0;
                p.set(objlist->o_obj->oBB().left,
                    objlist->o_obj->oBB().bottom);
                v = objlist->o_pts = new Vertex(p);
                if (AOI->intersect(v->point(), true))
                    v->set_movable(true);
                p.set(objlist->o_obj->oBB().left,
                    objlist->o_obj->oBB().top);
                v->set_next(new Vertex(p));
                v = v->next();
                if (AOI->intersect(v->point(), true))
                    v->set_movable(true);
                p.set(objlist->o_obj->oBB().right,
                    objlist->o_obj->oBB().top);
                v->set_next(new Vertex(p));
                v = v->next();
                if (AOI->intersect(v->point(), true))
                    v->set_movable(true);
                p.set(objlist->o_obj->oBB().right,
                    objlist->o_obj->oBB().bottom);
                v->set_next(new Vertex(p));
                v = v->next();
                if (AOI->intersect(v->point(), true))
                    v->set_movable(true);
            }
        }
        else if (sl->odesc->type() == CDPOLYGON) {
            int num = ((const CDpo*)sl->odesc)->numpts();
            const Point *pnts = ((const CDpo*)sl->odesc)->points();
            int i;
            for (i = 0; i < num; i++)
                if (AOI->intersect(&pnts[i], true))
                    break;
            if (i < num) {
                sObj *o;
                for (o = objlist; o; o = o->o_next) {
                    if (o->o_obj == sl->odesc)
                        break;
                }

                // Note that we prevent selecting two vertices at the
                // same location in the same poly (shouldn't see this
                // anyway).
                // NOTE:  This means that the movable field of the
                // last vertex is NOT set to match the first.

                if (o) {
                    Plist *p0 = 0;
                    for (Vertex *v = o->o_pts; v; v = v->next()) {

                        if (AOI->intersect(v->point(), true)) {
                            if (v->movable()) {
                                v->set_movable(false);
                                continue;
                            }

                            bool found = false;
                            for (Plist *p = p0; p; p = p->next) {
                                if (*(Point*)p == *v->point()) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                v->set_movable(true);
                                p0 = new Plist(v->px(), v->py(), p0);
                            }
                        }
                    }
                    Plist::destroy(p0);
                }
                else {
                    objlist = new sObj(sl->odesc, objlist);
                    Vertex *v = 0;
                    Plist *p0 = 0;
                    for (i = 0; i < num; i++) {
                        if (!v)
                            v = objlist->o_pts = new Vertex(pnts[i]);
                        else {
                            v->set_next(new Vertex(pnts[i]));
                            v = v->next();
                        }
                        if (AOI->intersect(v->point(), true)) {
                            bool found = false;
                            for (Plist *p = p0; p; p = p->next) {
                                if (*(Point*)p == *v->point()) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                v->set_movable(true);
                                p0 = new Plist(v->px(), v->py(), p0);
                            }
                        }
                    }
                    Plist::destroy(p0);
                }
            }
        }
        else if (sl->odesc->type() == CDWIRE) {
            int num = ((const CDw*)sl->odesc)->numpts();
            const Point *pnts = ((const CDw*)sl->odesc)->points();
            int i;
            for (i = 0; i < num; i++)
                if (AOI->intersect(&pnts[i], true))
                    break;
            if (i < num) {
                sObj *o;
                for (o = objlist; o; o = o->o_next) {
                    if (o->o_obj == sl->odesc)
                        break;
                }

                // Note that we prevent selecting two vertices at the
                // same location in the same wire.

                if (o) {
                    Plist *p0 = 0;
                    for (Vertex *v = o->o_pts; v; v = v->next()) {

                        if (AOI->intersect(v->point(), true)) {
                            if (v->movable()) {
                                v->set_movable(false);
                                continue;
                            }

                            bool found = false;
                            for (Plist *p = p0; p; p = p->next) {
                                if (*(Point*)p == *v->point()) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                v->set_movable(true);
                                p0 = new Plist(v->px(), v->py(), p0);
                            }
                        }
                    }
                    Plist::destroy(p0);
                }
                else {
                    objlist = new sObj(sl->odesc, objlist);
                    Vertex *v = 0;
                    Plist *p0 = 0;
                    for (i = 0; i < num; i++) {
                        if (!v)
                            v = objlist->o_pts = new Vertex(pnts[i]);
                        else {
                            v->set_next(new Vertex(pnts[i]));
                            v = v->next();
                        }
                        if (AOI->intersect(v->point(), true)) {
                            bool found = false;
                            for (Plist *p = p0; p; p = p->next) {
                                if (*(Point*)p == *v->point()) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                v->set_movable(true);
                                p0 = new Plist(v->px(), v->py(), p0);
                            }
                        }
                    }
                    Plist::destroy(p0);
                }
            }
        }
    }
    return (objlist);
}


// Static function.
// Mark all movable vertices in the list, or erase all vertex marks.
//
void
sObj::mark_vertices(const sObj *thiso, bool DisplayOrErase)
{
    if (DisplayOrErase == ERASE) {
        DSP()->EraseMarks(MARK_BOX);
        return;
    }
    for (const sObj *o = thiso; o; o = o->o_next) {
        for (const Vertex *v = o->o_pts; v; v = v->cnext()) {
            if (v->movable()) {
                DSP()->ShowBoxMark(DISPLAY, v->px(), v->py(),
                    HighlightingColor, 12, DSP()->CurMode());
            }
        }
    }
}


// If there is only one object and it is a wire, and the vertex list
// contains one endpoint and possibly adjacent points, return the
// coordinates of the next vertex not in the list.  This will be used
// as the reference vertex for vertex moving, so 45 constraints are
// reasonable.
//
bool
sObj::get_wire_ref(int *xrp, int *yrp, int *xmp, int *ymp)
{
    if (o_next)
        return (false);
    if (!o_obj || o_obj->type() != CDWIRE)
        return (false);

    int nv = 0;
    for (Vertex *v = o_pts; v; v = v->next()) {
        if (v->movable())
            nv++;
    }
    if (!nv)
        return (false);

    const Point *wpts = ((CDw*)o_obj)->points();
    int npts = ((CDw*)o_obj)->numpts();
    if (nv == npts)
        return (false);

    bool has_end = false;
    for (Vertex *v = o_pts; v; v = v->next()) {
        if (!v->movable())
            continue;
        if (v->px() == wpts[0].x && v->py() == wpts[0].y) {
            has_end = true;
            break;
        }
    }
    if (has_end) {
        for (int i = 1; i < nv; i++) {
            has_end = false;
            for (Vertex *v = o_pts; v; v = v->next()) {
                if (!v->movable())
                    continue;
                if (v->px() == wpts[i].x && v->py() == wpts[i].y) {
                    has_end = true;
                    break;
                }
            }
            if (!has_end)
                return (false);
        }
        if (xrp)
            *xrp = wpts[nv].x;
        if (yrp)
            *yrp = wpts[nv].y;
        if (xmp)
            *xmp = wpts[nv-1].x;
        if (ymp)
            *ymp = wpts[nv-1].y;
        return (true);
    }
    for (Vertex *v = o_pts; v; v = v->next()) {
        if (!v->movable())
            continue;
        if (v->px() == wpts[npts-1].x && v->py() == wpts[npts-1].y) {
            has_end = true;
            break;
        }
    }
    if (has_end) {
        for (int i = 1; i < nv; i++) {
            has_end = false;
            for (Vertex *v = o_pts; v; v = v->next()) {
                if (!v->movable())
                    continue;
                if (v->px() == wpts[npts-i-1].x && v->py() == wpts[npts-i-1].y) {
                    has_end = true;
                    break;
                }
            }
            if (!has_end)
                return (false);
        }
        if (xrp)
            *xrp = wpts[npts-nv-1].x;
        if (yrp)
            *yrp = wpts[npts-nv-1].y;
        if (xmp)
            *xmp = wpts[npts-nv].x;
        if (ymp)
            *ymp = wpts[npts-nv].y;
        return (true);
    }
    return (false);
}
// End of sObj functions

