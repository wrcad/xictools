
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
 $Id: cd_merge.cc,v 5.53 2015/06/11 05:54:05 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "cd_hypertext.h"
#include "cd_propnum.h"
#include "geo_zlist.h"
#include <algorithm>


//
// Functions for merging new objects with existing objects in the
// database.
//

// CDmergeInhibit will set/unset the CDnoMerge flags in a list of
// objects.  It keeps its own list of flagged objects, to be sure
// all objects are reset.
//
// This is needed, e.g., when multiple objects are being moved.  We
// can't merge objects that are waiting is a queue to be moved.

// Constructor.  Set the NoMerge flag in each object, and save a list
// of objects.
//
CDmergeInhibit::CDmergeInhibit(CDol *list)
{
    mi_objects = 0;
    int cnt = 0;
    for (CDol *o = list; o; o = o->next) {
        if (o->odesc)
            cnt++;
    }
    mi_count = cnt;
    if (!cnt)
        return;

    mi_objects = new CDo*[mi_count];
    cnt = 0;
    for (CDol *o = list; o; o = o->next) {
        if (o->odesc) {
            o->odesc->set_flag(CDnoMerge);
            mi_objects[cnt++] = o->odesc;
        }
    }
}


// Called from destructor, revert the flags and free the list.
//
void
CDmergeInhibit::revert()
{
    for (int i = 0; i < mi_count; i++)
        mi_objects[i]->unset_flag(CDnoMerge);
    delete [] mi_objects;
    mi_objects = 0;
    mi_count = 0;
}
// End of CDmergeInhibit functions.


namespace {
    // A class for merging property lists, filters duplicates.
    //
    struct prp_accum_t
    {
        prp_accum_t(bool elec)
            {
                pa_list = 0;
                pa_elec = elec;
            }
        ~prp_accum_t() { CDp::destroy(pa_list); }

        void add_properties(CDo*);

        CDp *list_properties()
            {
                CDp *p = pa_list;
                pa_list = 0;
                return (p);
            }

    private:
        CDp *pa_list;
        bool pa_elec;
    };


    void
    prp_accum_t::add_properties(CDo *odesc)
    {
        if (!odesc)
            return;
        for (CDp *pp = odesc->prpty_list(); pp; pp = pp->next_prp()) {
            if (pa_elec) {
                // In electrical mode, merge only unique "other" properties.
                if (pp->value() != P_OTHER)
                    continue;
                CDp *pend = 0;
                bool found = false;
                char *s = 0;
                for (CDp *p = pa_list; p; pend = p, p = p->next_prp()) {
                    if (!s) {
                        s = hyList::string(((CDp_user*)pp)->data(), HYcvAscii,
                            true);
                    }
                    char *s1 = hyList::string(((CDp_user*)p)->data(),
                        HYcvAscii, true);
                    if (!s && !s1) {
                        found = true;
                        delete [] s1;
                        break;
                    }
                    int n = (s && s1) ? strcmp(s, s1) : 1;
                    delete [] s1;
                    if (!n) {
                        found = true;
                        delete [] s1;
                        break;
                    }
                }
                delete [] s;
                if (!found) {
                    CDp *px = pp->dup();
                    if (px) {
                        px->set_next_prp(0);
                        if (pend)
                            pend->set_next_prp(px);
                        else
                            pa_list = px;
                    }
                }
            }
            else {
                CDp *pend = 0;
                bool found = false;
                for (CDp *p = pa_list; p; pend = p, p = p->next_prp()) {
                    if (p->value() != pp->value())
                        continue;
                    if (!p->string() && !pp->string()) {
                        found = true;
                        break;
                    }
                    if (p->string() && pp->string() &&
                            !strcmp(p->string(), pp->string())) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    CDp *px = pp->dup();
                    if (px) {
                        px->set_next_prp(0);
                        if (pend)
                            pend->set_next_prp(px);
                        else
                            pa_list = px;
                    }
                }
            }
        }
    }
}


namespace {

#ifdef notdef
    // Recursion is probably overkill, and is very slow.

    // Recursively add to tab objects that touch odesc.
    //
    void
    list(SymTab *tab, CDs *sdesc, CDo *odesc)
    {
        CDg gdesc;
        gdesc.init_gen(sdesc, odesc->oLdesc, &odesc->oBB);
        CDo *pointer;
        while ((pointer = gdesc.next()) != 0) {
            if (!pointer->is_normal() ||
                    (pointer->oType != CDBOX && pointer->oType != CDPOLYGON) ||
                    (pointer->oFlags & CDnoMerge) || pointer == odesc)
                continue;
            if (!odesc->intersect(pointer, true))
                continue;
            if (tab->get((unsigned long)pointer) == ST_NIL) {
                tab->add((unsigned long)pointer, 0, false);
                list(tab, sdesc, pointer);
            }
        }
    }
#endif


    // Return a list of objects that touch odesc (not including odesc).
    //
    CDol *
    list(CDs *sdesc, CDo *odesc)
    {
        CDol *o0 = 0;
        CDg gdesc;
        gdesc.init_gen(sdesc, odesc->ldesc(), &odesc->oBB());
        CDo *pointer;
        while ((pointer = gdesc.next()) != 0) {
            if (!pointer->is_normal() ||
                    (pointer->type() != CDBOX &&
                        pointer->type() != CDPOLYGON) ||
                    pointer->has_flag(CDnoMerge) || pointer == odesc)
                continue;
            if (!odesc->intersect(pointer, true))
                continue;
            o0 = new CDol(pointer, o0);
        }
        return (o0);
    }
}


//
// Note about CDmergeCreated, CDmergeDeleted:
// CDmergeCreated is set in every new object created as the result of
// a merge.
//
// CDmergeDeleted is set for every UNSELECTED object deleted as the
// result of a merge.  This can be used in undo processing, objects
// with this flag set should not be reinserted into the selections
// queue.
//

// Find the boxes and polys that touch odesc.  Merge these into new
// polygons, deleting originals.  The odesc is in the database.
//
bool
CDs::mergeBoxOrPoly(CDo *odesc, bool Undoable)
{
    if (CD()->IsNoMergeObjects())
        return (true);
    if (CD()->IsNoMergePolys())
        return (mergeBox(odesc, Undoable));
    CDs *sdt = this;
    if (!sdt || !odesc)
        return (true);
    if (odesc->type() != CDBOX && odesc->type() != CDPOLYGON)
        return (true);
    if (odesc->has_flag(CDnoMerge))
        return (true);
    if (odesc->ldesc()->isNoMerge())
        return (true);
    if (!odesc->is_normal())
        return (true);

    CDol *o0 = list(this, odesc);
    if (!o0)
        return (true);
    o0 = new CDol(odesc, o0);

    PolyList *p0 = 0, *pe = 0;
    Zlist *zl0 = 0, *ze = 0;
    int zcnt = 0;

    for (CDol *ol = o0; ol; ol = ol->next) {
        CDo *od = ol->odesc;
        Zlist *z = od->toZlist();
        int n = Zlist::length(z);
        if (Zlist::JoinMaxQueue <= 0 || zcnt + n < Zlist::JoinMaxQueue) {
            if (!zl0)
                zl0 = ze = z;
            else {
                while (ze->next)
                    ze = ze->next;
                ze->next = z;
            }
            zcnt += n;
            continue;
        }
        if (!p0)
            p0 = pe = Zlist::to_poly_list(zl0);
        else {
            while (pe->next)
                pe = pe->next;
            pe->next = Zlist::to_poly_list(zl0);
        }
        zl0 = ze = z;
        zcnt = n;
    }
    if (zcnt) {
        if (!p0)
            p0 = pe = Zlist::to_poly_list(zl0);
        else {
            while (pe->next)
                pe = pe->next;
            pe->next = Zlist::to_poly_list(zl0);
        }
    }

    bool ok = true;
    for (PolyList *pl = p0; pl; pl = pl->next) {
        CDo *newo = 0;
        if (pl->po.is_rect()) {
            BBox tBB;
            pl->po.computeBB(&tBB);
            if (!makeBox(odesc->ldesc(), &tBB, &newo))
                ok = false;
            delete [] pl->po.points;
            pl->po.points = 0;
        }
        else {
            CDpo *newpo;
            if (!makePolygon(odesc->ldesc(), &pl->po, &newpo))
                ok = false;
            newo = newpo;
        }
        if (ok && newo) {
            // Add properties.  Copy and accumulate the properties from
            // overlapping objects, filter duplicates.
            //
            prp_accum_t acc(isElectrical());
            for (CDol *ol = o0; ol; ol = ol->next) {
                if (newo->intersect(ol->odesc, false))
                    acc.add_properties(ol->odesc);
            }
            CDp *p = acc.list_properties();
            if (p)
                newo->set_prpty_list(p);

            if (Undoable) {
                CD()->ifRecordObjectChange(this, 0, newo);
                newo->set_flag(CDmergeCreated);
            }
        }
    }
    PolyList::destroy(p0);

    for (CDol *ol = o0; ol; ol = ol->next) {
        CDo *od = ol->odesc;
        if (Undoable) {
            bool nosl = (od->state() != CDSelected);
            CD()->ifRecordObjectChange(this, od, 0);
            if (nosl)
                od->set_flag(CDmergeDeleted);
        }
        else
            unlink(od, false);
    }
    CDol::destroy(o0);

    return (true);
}


// Merge the specifed box, which already exists in the database, with
// neighboring boxes, if possible.  This is (optionally) called from
// the I/O file readers when adding boxes to the database.
//
bool
CDs::mergeBox(CDo *odesc, bool Undoable)
{
    if (!odesc || odesc->type() != CDBOX)
        return (true);
    if (odesc->has_flag(CDnoMerge))
        return (true);
    if (odesc->ldesc()->isNoMerge())
        return (true);

    BBox tBB = odesc->oBB();
    CDg gdesc;
    gdesc.init_gen(this, odesc->ldesc(), &tBB);
    CDo *pointer;
    while ((pointer = gdesc.next()) != 0) {
        if (!pointer->is_normal() || pointer->type() != CDBOX ||
                pointer->has_flag(CDnoMerge) || pointer == odesc)
            continue;
        bool merged;
        if (!CD()->ClipMerge(odesc, pointer, this, &merged, Undoable))
            return (false);
        if (merged)
            return (true);
    }
    return (true);
}


namespace {
    // Return a path consisting of maximum num1 points from pts1
    // followed by num2 points from pts2.  If the shorter section
    // retraces the vertices of the longer section, the shorter
    // section is not copied.  Arg newnum is the number of points in
    // the returned path.
    //
    Point *
    link_paths(const Point *pts1, int num1, const Point *pts2, int num2,
        int *newnum)
    {
        bool link2 = false;
        Point *p = new Point[num1+num2];
        int i, j;
        if (num1 + 1 >= num2) {
            for (i = 0; i < num1; i++)
                p[i] = pts1[i];
            p[i++] = pts2[0];
            j = i;
            for (i = 1; i < num2; i++) {
                if (pts1[num1-i].x != pts2[i].x ||
                        pts1[num1-i].y != pts2[i].y) {
                    link2 = true;
                    break;
                }
            }
            if (link2) {
                for (i = 1; i < num2; i++, j++)
                    p[j] = pts2[i];
            }
        }
        else {
            for (i = 1; i <= num1; i++) {
                if (pts1[num1-i].x != pts2[i].x ||
                        pts1[num1-i].y != pts2[i].y) {
                    link2 = true;
                    break;
                }
            }
            j = 0;
            if (link2) {
                for (i = 0; i < num1; i++)
                    p[i] = pts1[i];
                j = i;
            }
            for (i = 0; i < num2; i++, j++)
                p[j] = pts2[i];
        }
        *newnum = j;
        if (j != num1 + num2) {
            Point *op = p;
            p = new Point[j];
            while (j--)
               p[j] = op[j];
           delete [] op;
        }
        return (p);
    }
}


namespace {
    // Perform the object "deletion".
    //
    void do_delete(CDs *sdesc, CDo *odesc, bool Undoable)
    {
        if (Undoable) {
            bool nosl = (odesc->state() != CDSelected);
            CD()->ifRecordObjectChange(sdesc, odesc, 0);
            if (nosl)
                odesc->set_flag(CDmergeDeleted);
        }
        else
            sdesc->unlink(odesc, false);
    }

    // Return true if the wire has a bound node name label.
    //
    bool has_label(const CDw *wdesc)
    {
        CDp_node *pn = (CDp_node*)wdesc->prpty(P_NODE);
        if (pn && pn->bound())
            return (true);
        CDp_bnode *pb = (CDp_bnode*)wdesc->prpty(P_BNODE);
        if (pb && pb->bound())
            return (true);
        return (false);
    }
}


// If two wires of equal width share an endpoint, merge into one.
//
bool
CDs::mergeWire(CDw *wdesc, bool Undoable)
{
    if (CD()->IsNoMergeObjects())
        return (true);
    CDs *sdt = this;
    if (!sdt || !wdesc || wdesc->type() != CDWIRE)
        return (true);
    if (wdesc->has_flag(CDnoMerge))
        return (true);
    if (wdesc->ldesc()->isNoMerge())
        return (true);
    if (!wdesc->is_normal())
        return (true);

    // Never merge a wire with a label.
    if (isElectrical() && has_label(wdesc))
        return (true);

    bool merged1 = false, merged2 = false;
    int wid = wdesc->wire_width()/2;
    const Point *path = wdesc->points();
    int numpts = wdesc->numpts();
    BBox BBs, BBe;
    BBs.left = path->x - wid;
    BBs.bottom = path->y - wid;
    BBs.right = path->x + wid;
    BBs.top = path->y + wid;
    BBe.left = path[numpts-1].x - wid;
    BBe.bottom = path[numpts-1].y - wid;
    BBe.right = path[numpts-1].x + wid;
    BBe.top = path[numpts-1].y + wid;
    CDg gdesc;
    gdesc.init_gen(this, wdesc->ldesc(), &BBs);
    CDo *pointer1;
    while ((pointer1 = gdesc.next()) != 0) {
        if (!pointer1->is_normal() || pointer1->type() != CDWIRE ||
                pointer1->has_flag(CDnoMerge) || wdesc == (CDw*)pointer1)
            continue;
        CDw *wold = (CDw*)pointer1;
        if (isElectrical() && has_label(wold))
            continue;
        if (wold->wire_width() != wdesc->wire_width())
            continue;
        if (path->x == wold->points()[wold->numpts() - 1].x &&
                path->y == wold->points()[wold->numpts() - 1].y) {
            path = link_paths(wold->points(), wold->numpts() - 1,
                path, numpts, &numpts);
            merged1 = true;
            break;
        }
        if (path->x == wold->points()->x &&
                path->y == wold->points()->y) {
            wold->reverse_list();
            path = link_paths(wold->points(), wold->numpts() - 1,
                path, numpts, &numpts);
            merged1 = true;
            break;
        }
    }
    gdesc.init_gen(this, wdesc->ldesc(), &BBe);
    CDo *pointer2;
    while ((pointer2 = gdesc.next()) != 0) {
        if (!pointer2->is_normal() || pointer2->type() != CDWIRE ||
                pointer2->has_flag(CDnoMerge) || wdesc == (CDw*)pointer2)
            continue;
        CDw *wold = (CDw*)pointer2;
        if (isElectrical() && has_label(wold))
            continue;
        if (wold->wire_width() != wdesc->wire_width())
            continue;
        if (path[numpts-1].x == wold->points()[wold->numpts() - 1].x &&
                path[numpts-1].y == wold->points()[wold->numpts() - 1].y) {
            wold->reverse_list();
            Point *path1 = link_paths(path, numpts - 1,
                wold->points(), wold->numpts(), &numpts);
            if (merged1)
                delete [] path;
            path = path1;
            merged2 = true;
            break;
        }
        if (path[numpts-1].x == wold->points()->x &&
                path[numpts-1].y == wold->points()->y) {
            Point *path1 = link_paths(path, numpts - 1,
                wold->points(), wold->numpts(), &numpts);
            if (merged1)
                delete [] path;
            path = path1;
            merged2 = true;
            break;
        }
    }
    if (!merged1 && !merged2)
        return (true);

    Wire wnew(2*wid, wdesc->wire_style(), numpts, (Point*)path);
    CDw *wpointer;
    CDerrType err = makeWire(wdesc->ldesc(), &wnew, &wpointer);
    if (err != CDok) {
        delete [] path;
        if (err == CDbadWire)
            return (true);
        return (false);
    }
    prp_accum_t acc(isElectrical());
    if (pointer1 && wpointer->intersect(pointer1, false))
        acc.add_properties(pointer1);
    if (pointer2 && wpointer->intersect(pointer2, false))
        acc.add_properties(pointer2);
    wpointer->set_prpty_list(acc.list_properties());

    if (Undoable) {
        CD()->ifRecordObjectChange(this, 0, wpointer);
        wpointer->set_flag(CDmergeCreated);
    }
    do_delete(this, wdesc, Undoable);
    if (merged1)
        do_delete(this, pointer1, Undoable);
    if (merged2)
        do_delete(this, pointer2, Undoable);

    return (true);
}
// End of CDs functions.


namespace {
    inline bool
    newbox(CDs *sdesc, CDl *ldesc, int l, int b, int r, int t, CDo **pointer)
    {
        BBox BB(l, b, r, t);
        if (sdesc->makeBox(ldesc, &BB, pointer) != CDok)
            return (false);
        return (true);
    }
}


bool
cCD::ClipMerge(CDo *o1, CDo *o2, CDs *sdesc, bool *merged, bool Undoable)
{
    CDl *ldesc = o1->ldesc();
    *merged = false;
    if (o1->oBB().right < o2->oBB().left || o1->oBB().left > o2->oBB().right ||
            o1->oBB().top < o2->oBB().bottom ||
                o1->oBB().bottom > o2->oBB().top)
        // no touching or overlap
        return (true);

    CDo *pointer;
    if (o1->oBB().left == o2->oBB().left &&
            o1->oBB().right == o2->oBB().right) {
        // combine
        BBox tBB(o1->oBB().left, mmMin(o1->oBB().bottom, o2->oBB().bottom),
            o1->oBB().right, mmMax(o1->oBB().top, o2->oBB().top));
        if (Undoable) {
            bool nosl1 = (o1->state() != CDSelected);
            bool nosl2 = (o2->state() != CDSelected);
            ifRecordObjectChange(sdesc, o1, 0);
            ifRecordObjectChange(sdesc, o2, 0);
            if (nosl1)
                o1->set_flag(CDmergeDeleted);
            if (nosl2)
                o2->set_flag(CDmergeDeleted);
        }
        else {
            sdesc->unlink(o1, false);
            sdesc->unlink(o2, false);
        }
        if (!newbox(sdesc, ldesc, tBB.left, tBB.bottom, tBB.right, tBB.top,
                &pointer))
            return (false);
        if (pointer) {
            prp_accum_t acc(sdesc->isElectrical());
            if (o1 && pointer->intersect(o1, false))
                acc.add_properties(o1);
            if (o2 && pointer->intersect(o2, false))
                acc.add_properties(o2);
            pointer->set_prpty_list(acc.list_properties());

            if (Undoable) {
                ifRecordObjectChange(sdesc, 0, pointer);
                pointer->set_flag(CDmergeCreated);
            }
            if (!sdesc->mergeBox(pointer, Undoable))
                return (false);
        }
        *merged = true;
        return (true);
    }
    if (o1->oBB().bottom == o2->oBB().bottom &&
            o1->oBB().top == o2->oBB().top) {
        // combine
        BBox tBB(mmMin(o1->oBB().left, o2->oBB().left), o1->oBB().bottom,
            mmMax(o1->oBB().right, o2->oBB().right), o1->oBB().top);
        if (Undoable) {
            bool nosl1 = (o1->state() != CDSelected);
            bool nosl2 = (o2->state() != CDSelected);
            ifRecordObjectChange(sdesc, o1, 0);
            ifRecordObjectChange(sdesc, o2, 0);
            if (nosl1)
                o1->set_flag(CDmergeDeleted);
            if (nosl2)
                o2->set_flag(CDmergeDeleted);
        }
        else {
            sdesc->unlink(o1, false);
            sdesc->unlink(o2, false);
        }
        if (!newbox(sdesc, ldesc, tBB.left, tBB.bottom, tBB.right, tBB.top,
                &pointer))
            return (false);
        if (pointer) {
            prp_accum_t acc(sdesc->isElectrical());
            if (o1 && pointer->intersect(o1, false))
                acc.add_properties(o1);
            if (o2 && pointer->intersect(o2, false))
                acc.add_properties(o2);
            pointer->set_prpty_list(acc.list_properties());

            if (Undoable) {
                ifRecordObjectChange(sdesc, 0, pointer);
                pointer->set_flag(CDmergeCreated);
            }
            if (!sdesc->mergeBox(pointer, Undoable))
                return (false);
        }
        *merged = true;
        return (true);
    }
    if (o1->oBB().right <= o2->oBB().left ||
            o1->oBB().left >= o2->oBB().right ||
            o1->oBB().top <= o2->oBB().bottom ||
            o1->oBB().bottom >= o2->oBB().top)
        // no overlap
        return (true);

    // overlapping boxes
    BBox BB1 = o1->oBB();
    BBox BB2 = o2->oBB();
    if (Undoable) {
        bool nosl1 = (o1->state() != CDSelected);
        bool nosl2 = (o2->state() != CDSelected);
        ifRecordObjectChange(sdesc, o1, 0);
        ifRecordObjectChange(sdesc, o2, 0);
        if (nosl1)
            o1->set_flag(CDmergeDeleted);
        if (nosl2)
            o2->set_flag(CDmergeDeleted);
    }
    else {
        sdesc->unlink(o1, false);
        sdesc->unlink(o2, false);
    }
    Blist *top = 0;
    Blist *bot = 0;
    *merged = true;
    if (BB2.top > BB1.top) {
        top = new Blist;
        top->BB = BB2;
        top->BB.bottom = BB1.top;
    }
    else if (BB1.top > BB2.top) {
        top = new Blist;
        top->BB = BB1;
        top->BB.bottom = BB2.top;
    }
    if (BB2.bottom < BB1.bottom) {
        bot = new Blist;
        bot->BB = BB2;
        bot->BB.top = BB1.bottom;
    }
    else if (BB1.bottom < BB2.bottom) {
        bot = new Blist;
        bot->BB = BB1;
        bot->BB.top = BB2.bottom;
    }
    // look at the overlap region
    BBox BB;
    BB.top = mmMin(BB1.top, BB2.top);
    BB.bottom = mmMax(BB1.bottom, BB2.bottom);
    BB.left = mmMin(BB1.left, BB2.left);
    BB.right = mmMax(BB1.right, BB2.right);
    if (top) {
        if (BB.left == top->BB.left && BB.right == top->BB.right)
            BB.top = top->BB.top;
        else {
            if (!newbox(sdesc, ldesc, top->BB.left, top->BB.bottom,
                    top->BB.right, top->BB.top, &pointer))
                return (false);
            if (pointer) {
                prp_accum_t acc(sdesc->isElectrical());
                if (o1 && pointer->intersect(o1, false))
                    acc.add_properties(o1);
                if (o2 && pointer->intersect(o2, false))
                    acc.add_properties(o2);
                pointer->set_prpty_list(acc.list_properties());

                if (Undoable) {
                    ifRecordObjectChange(sdesc, 0, pointer);
                    pointer->set_flag(CDmergeCreated);
                }
                if (!sdesc->mergeBox(pointer, Undoable))
                    return (false);
            }
        }
        delete top;
    }
    if (bot) {
        if (BB.left == bot->BB.left && BB.right == bot->BB.right)
            BB.bottom = bot->BB.bottom;
        else {
            if (!newbox(sdesc, ldesc, bot->BB.left, bot->BB.bottom,
                    bot->BB.right, bot->BB.top, &pointer))
                return (false);
            if (pointer) {
                prp_accum_t acc(sdesc->isElectrical());
                if (o1 && pointer->intersect(o1, false))
                    acc.add_properties(o1);
                if (o2 && pointer->intersect(o2, false))
                    acc.add_properties(o2);
                pointer->set_prpty_list(acc.list_properties());

                if (Undoable) {
                    ifRecordObjectChange(sdesc, 0, pointer);
                    pointer->set_flag(CDmergeCreated);
                }
                if (!sdesc->mergeBox(pointer, Undoable))
                    return (false);
            }
        }
        delete bot;
    }
    if (!newbox(sdesc, ldesc, BB.left, BB.bottom, BB.right, BB.top,
            &pointer))
        return (false);
    if (pointer) {
        prp_accum_t acc(sdesc->isElectrical());
        if (o1 && pointer->intersect(o1, false))
            acc.add_properties(o1);
        if (o2 && pointer->intersect(o2, false))
            acc.add_properties(o2);
        pointer->set_prpty_list(acc.list_properties());

        if (Undoable) {
            ifRecordObjectChange(sdesc, 0, pointer);
            pointer->set_flag(CDmergeCreated);
        }
        if (!sdesc->mergeBox(pointer, Undoable))
            return (false);
    }
    return (true);
}

