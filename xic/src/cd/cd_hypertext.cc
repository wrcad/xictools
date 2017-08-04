
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
#include "cd_hypertext.h"
#include "cd_propnum.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include <ctype.h>


/**************************************************************************
 *
 * Core functions for hypertext, for CD.
 *
 **************************************************************************/


// Make sure all references have a device pointer attached.
// Called after entire circuit has been read in.
//
void
CDs::hyInit() const
{
    if (!isElectrical() || !getHY())
        return;
    for (hyEnt **hent = getHY(); *hent; hent++)
        (*hent)->check_ref();
}


// Return a string naming the node/branch located in BB.
// Node numbers are returned as v(#).
// BBox *AOI;           region to search
// int typemask;        return these types only
// struct hyEnt **hret; returned point descriptor
//
char *
CDs::hyString(cTfmStack *stk, const BBox *AOI, int typemask, hyEnt **hret)
{
    if (hret)
        *hret = 0;
    hyEnt *h = hyPoint(stk, AOI, typemask);
    if (h == 0)
        return (0);
    char *tname = h->get_subname(true);
    if (!tname) {
        delete h;
        return (0);
    }
    if (hret)
        *hret = h;
    else
        delete h;
    return (tname);
}


// Check the objects in AOI, and return an entry if the AOI encloses
// an active part of an object.
//
hyEnt *
CDs::hyPoint(cTfmStack *stk, const BBox *AOI, int mask)
{
    if (!AOI)
        return (0);
    BBox InvAOI(*AOI);
    stk->TInverse();
    stk->TInverseBB(&InvAOI, 0);

    if (mask == HY_CELL || !isElectrical()) {
        // Return cell names only.
        CDol *sl0 = 0;
        CDg gdesc;
        stk->TInitGen(this, CellLayer(), AOI, &gdesc);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0)
            sl0 = new CDol(odesc, sl0);
        if (sl0) {
            // return the name of the smallest cell
            BBox tBB;
            CDol::computeBB(sl0, &tBB);
            double marea = ((double)tBB.width())*tBB.height();
            odesc = sl0->odesc;
            CDol *sl;
            for (sl = sl0; sl; sl = sl->next) {
                double a = ((double)sl->odesc->oBB().width()) *
                    sl->odesc->oBB().height();
                if (a > 0.0 && a < marea) {
                    odesc = sl->odesc;
                    marea = a;
                }
            }
            for ( ; sl0; sl0 = sl) {
                sl = sl0->next;
                delete sl0;
            }
            int x = (odesc->oBB().left + odesc->oBB().right)/2;
            int y = (odesc->oBB().bottom + odesc->oBB().top)/2;
            return (new hyEnt(this, x, y, odesc, HYrefCell, HYorNone));
        }
        if (mask == HY_CELL)
            return (0);
    }
    else {
        if (isSymbolic()) {
            if (mask & HY_NODE) {
                CDp_snode *pn = (CDp_snode*)prpty(P_NODE);
                for ( ; pn; pn = pn->next()) {
                    for (unsigned int ix = 0; ; ix++) {
                        int x, y;
                        if (!pn->get_pos(ix, &x, &y))
                            break;
                        if (InvAOI.intersect(x, y, true))
                            return (new hyEnt(this, x, y, pn->index()));
                    }
                }
            }
        }
        else {
            // First check the active wires.
            if (mask & HY_NODE) {
                CDsLgen gen(this);
                CDl *ld;
                while ((ld = gen.next()) != 0) {
                    if (!ld->isWireActive())
                        continue;
                    CDg gdesc;
                    stk->TInitGen(this, ld, AOI, &gdesc);
                    Point_c px((InvAOI.left + InvAOI.right)/2,
                        (InvAOI.bottom + InvAOI.top)/2);
                    Point po;
                    CDo *odesc;
                    while ((odesc = gdesc.next()) != 0) {
                        if (odesc->type() != CDWIRE)
                            continue;
                        CDp_node *pn = (CDp_node*)odesc->prpty(P_NODE);
                        if (pn) {
                            const Point *pts = ((CDw*)odesc)->points();
                            int num = ((CDw*)odesc)->numpts();
                            // The d argument to inPath is a little
                            // subtle, assume that a point overlapping
                            // a finite-width wire is equivalent to a
                            // box intersecting a zero-width wire.
                            int d = ((CDw*)odesc)->wire_width() +
                                (AOI->width() + AOI->height())/2;
                            if (!Point::inPath(pts, &px, d/2, &po, num))
                                continue;
                            return (new hyEnt(this, po.x, po.y, odesc,
                                HYrefNode, HYorNone));
                        }
                    }
                }
            }

            // No wire selected, try the devices and subcircuits.  First
            // make a list, use CDol since non-const is needed.
            CDol *cl0 = 0, *ce = 0;
            CDg gdesc;
            stk->TInitGen(this, CellLayer(), AOI, &gdesc);
            CDc *cinst;
            while ((cinst = (CDc*)gdesc.next()) != 0) {
                if (!cl0)
                    cl0 = ce = new CDol(cinst, 0);
                else {
                    ce->next = new CDol(cinst, 0);
                    ce = ce->next;
                }
            }

            // Check nodes and branches.
            for (CDol *cl = cl0; cl; cl = cl->next) {
                cinst = (CDc*)cl->odesc;

                // Check the nodes.
                if (mask & HY_NODE) {
                    CDp_cnode *pn = (CDp_cnode*)cinst->prpty(P_NODE);
                    for ( ; pn; pn = pn->next()) {
                        for (unsigned int ix = 0; ; ix++) {
                            int x, y;
                            if (!pn->get_pos(ix, &x, &y))
                                break;

                            if (InvAOI.intersect(x, y, true)) {
                                CDol::destroy(cl0);
                                return (new hyEnt(this, x, y, cinst,
                                    HYrefNode, HYorNone));
                            }
                        }
                    }
                }

                CDs *msdesc = cinst->masterCell();
                if (!msdesc)
                    continue;

                // If a device, look for a branch area.
                if ((mask & HY_BRAN) && msdesc->isDevice()) {
                    CDp_branch *pb = (CDp_branch*)cinst->prpty(P_BRANCH);;
                    for ( ; pb; pb = pb->next()) {
                        Point_c px(pb->pos_x(), pb->pos_y());
                        if (InvAOI.intersect(&px, true)) {
                            // pointing at the "branch" area
                            int x1 = pb->rot_x();
                            int y1 = pb->rot_y();
                            HYorType orient;
                            if (x1 == 0) {
                                if (y1 == 1)
                                    orient = HYorUp;
                                else
                                    orient = HYorDn;
                            }
                            else {
                                if (x1 == 1)
                                    orient = HYorRt;
                                else
                                    orient = HYorLt;
                            }
                            CDol::destroy(cl0);
                            return (new hyEnt(this, pb->pos_x(), pb->pos_y(),
                                cinst, HYrefBranch, orient));
                        }
                    }
                }
            }

            if (mask & HY_DEVN) {
                // If there is a device, return its name.
                for (CDol *cl = cl0; cl; cl = cl->next) {
                    cinst = (CDc*)cl->odesc;
                    CDs *msdesc = cinst->masterCell();
                    if (!msdesc)
                        continue;
                    if (msdesc->isDevice()) {
                        // Devices only!
                        int x = (InvAOI.left + InvAOI.right)/2;
                        int y = (InvAOI.bottom + InvAOI.top)/2;
                        CDol::destroy(cl0);
                        return (new hyEnt(this, x, y, cinst, HYrefDevice,
                            HYorNone));
                    }
                }
            }
            if (mask & HY_NODE) {
                // Redundant except for virtual terminals.
                CDp_snode *pn = (CDp_snode*)prpty(P_NODE);
                for ( ; pn; pn = pn->next()) {
                    if (isSymbolic()) {
                        for (unsigned int ix = 0; ; ix++) {
                            int x, y;
                            if (!pn->get_pos(ix, &x, &y))
                                break;
                            if (InvAOI.intersect(x, y, true))
                                return (new hyEnt(this, x, y, pn->index()));
                        }
                    }
                    else {
                        int x, y;
                        pn->get_schem_pos(&x, &y);
                        if (InvAOI.intersect(x, y, true))
                            return (new hyEnt(this, x, y, pn->index()));
                    }
                }
            }

            // Look inside subcircuits.
            for (CDol *cl = cl0; cl; cl = cl->next) {
                cinst = (CDc*)cl->odesc;
                CDs *msdesc = cinst->masterCell();
                if (!msdesc)
                    continue;
                if (!msdesc->isDevice()) {
                    hyEnt *h = hyPoint(stk, cinst, AOI, mask);
                    if (h) {
                        CDol::destroy(cl0);
                        h->set_owner(this);
                        return (h);
                    }
                }
            }

            if (mask & HY_DEVN) {
                // Return the name of a subcircuit.
                for (CDol *cl = cl0; cl; cl = cl->next) {
                    cinst = (CDc*)cl->odesc;
                    CDs *msdesc = cinst->masterCell();
                    if (!msdesc)
                        continue;
                    if (!msdesc->isDevice()) {
                        // Subckts only!
                        int x = (InvAOI.left + InvAOI.right)/2;
                        int y = (InvAOI.bottom + InvAOI.top)/2;
                        CDol::destroy(cl0);
                        return (new hyEnt(this, x, y, cinst, HYrefDevice,
                            HYorNone));
                    }
                }
            }
            CDol::destroy(cl0);
        }
    }

    if (mask & HY_LABEL) {
        CDsLgen lgen(this);
        CDl *ld;
        while ((ld = lgen.next()) != 0) {
            CDg gdesc;
            stk->TInitGen(this, ld, AOI, &gdesc);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                if (odesc->type() != CDLABEL)
                    continue;
                int x = (odesc->oBB().left + odesc->oBB().right)/2;
                int y = (odesc->oBB().bottom + odesc->oBB().top)/2;
                return (new hyEnt(this, x, y, odesc, HYrefLabel, HYorNone));
            }
        }
    }

    return (0);
}


// Return an entry from the subcell within AOI, of type governed by
// mask.
//
hyEnt *
CDs::hyPoint(cTfmStack *stk, CDc *cdesc, const BBox *AOI, int mask)
{
    if (!cdesc)
        return (0);
    CDs *sdesc = cdesc->masterCell();
    if (!sdesc)
        return (0);
    stk->TPush();
    stk->TApplyTransform(cdesc);
    stk->TPremultiply();
    hyEnt *h = sdesc->hyPoint(stk, AOI, mask);
    stk->TPop();
    if (h)
        h->set_parent(new hyParent(cdesc, 0, 0, h->parent()));
    return (h);
}


// Return a hyEnt corresponding to a node in sdesc at x, y (electrical
// mode).
//
hyEnt *
CDs::hyNode(cTfmStack *stk, int x, int y)
{
    int delta = 5;
    BBox tBB;
    tBB.left = x - delta;
    tBB.bottom = y - delta;
    tBB.right = x + delta;
    tBB.top = y + delta;
    return (hyPoint(stk, &tBB, HY_NODE));
}


// Given a property and its containers, return a hyperlist of the
// contents.
//
hyList *
CDs::hyPrpList(CDo *odesc, const CDp *pdesc)
{
    if (!pdesc)
        return (0);
    const char *lttok = HYtokPre HYtokLT HYtokSuf;
    if (isElectrical()) {
        if (!odesc)
            return (0);
        if (odesc->type() == CDINSTANCE) {
            if (pdesc->value() == P_NAME) {
                hyList *hp = new hyList(HLrefDevice);
                int x = (odesc->oBB().left + odesc->oBB().right)/2;
                int y = (odesc->oBB().bottom + odesc->oBB().top)/2;
                hyEnt *ent = new hyEnt(this, x, y, odesc, HYrefDevice,
                    HYorNone);
                ent->add();
                hp->set_hent(ent);
                return (hp);
            }
            if (pdesc->value() == P_MODEL || pdesc->value() == P_VALUE ||
                    pdesc->value() == P_PARAM || pdesc->value() == P_OTHER ||
                    pdesc->value() == P_DEVREF)
                return (hyList::dup(((CDp_user*)pdesc)->data()));
            if (pdesc->value() == P_NOPHYS) {
                hyList *hp = new hyList(HLrefText);
                hp->set_text(lstring::copy(pdesc->string()));
                return (hp);
            }
            if (pdesc->value() == P_VIRTUAL) {
                hyList *hp = new hyList(HLrefText);
                hp->set_text(lstring::copy(pdesc->string()));
                return (hp);
            }
            if (pdesc->value() == P_FLATTEN) {
                hyList *hp = new hyList(HLrefText);
                hp->set_text(lstring::copy(pdesc->string()));
                return (hp);
            }
            if (pdesc->value() == P_NODE) {
                CDp_cnode *pn = (CDp_cnode*)pdesc;
                hyList *hp0 = 0, *hpe = 0;
                for (unsigned int ix = 0; ; ix++) {
                    int x, y;
                    if (!pn->get_pos(ix, &x, &y))
                        break;
                    hyList *hp = new hyList(HLrefNode);
                    hyEnt *ent = new hyEnt(this, x, y, odesc, HYrefNode,
                        HYorNone);
                    ent->add();
                    hp->set_hent(ent);
                    if (!hp0)
                        hp0 = hpe = hp;
                    else {
                        hpe->set_next(hp);
                        hpe = hp;
                    }
                }
                return (hp0);
            }
            if (pdesc->value() == P_BRANCH) {
                CDp_branch *pb = (CDp_branch*)pdesc;
                hyList *hp = new hyList(HLrefBranch);
                hyEnt *ent = new hyEnt(this, pb->pos_x(), pb->pos_y(), odesc,
                    HYrefBranch, HYorNone);
                ent->add();
                hp->set_hent(ent);
                return (hp);
            }
            if (pdesc->value() == P_MUTLRF) {
                hyList *hp = new hyList(HLrefText);
                hp->set_text(lstring::copy(pdesc->string()));
                return (hp);
            }
        }
        else if (odesc->type() == CDWIRE) {
            if (pdesc->value() == P_NODE) {
                hyList *hp = new hyList(HLrefNode);
                const Point *pts = ((CDw*)odesc)->points();
                hyEnt *ent = new hyEnt(this, pts[0].x, pts[0].y, odesc,
                    HYrefNode, HYorNone);
                ent->add();
                hp->set_hent(ent);
                return (hp);
            }
        }
        else if (odesc->type() == CDLABEL) {
            if (pdesc->value() == P_LABRF) {
                CDp_lref *pl = (CDp_lref*)pdesc;
                hyList *hp = new hyList(HLrefText);

                char tbuf[128];
                char *s = lstring::stpcpy(tbuf,
                    pl->name() ? Tstring(pl->name()) : "?");
                *s++ = ' ';
                s = mmItoA(s, pl->number());
                *s++ = ' ';
                s = mmItoA(s, pl->propnum());

                hp->set_text(lstring::copy(tbuf));
                return (hp);
            }
        }
        hyList *hp = new hyList(HLrefText);
        const char *string = pdesc->string() ? pdesc->string() : "";
        if (lstring::prefix(lttok, string))
            // use symbolic form for long text
            string = "[text]";
        hp->set_text(lstring::copy(string));
        return (hp);
    }
    else {
        const char *string = pdesc->string() ? pdesc->string() : "";
        return (new hyList(this, string, HYcvAscii));
    }
    return (0);
}


// The next four functions are called as geometry is modified or
// deleted.  They maintain the hypertext list associated with the
// current cell and its parents.  For example, if a plot reference to
// a node in a subcell exists at the top level, we should be able to
// push to the subcell, move the device which is referenced, and pop
// back without losing the reference.  We should also be able to move
// a subcell containing references without losing the references.


namespace {
    // Fix references attached to objects being moved.  Note that this only
    // has to be done for top level references, as the parent list supplies
    // the transforms for lower level references.  This is called from the
    // context of the cell containing the object being moved, with the
    // move transform in effect.
    //
    void hy_tx_move_core(const CDs *sdesc, cTfmStack *stk, const CDo *odesc,
        CDo *new_odesc, bool undoable)
    {
        if (!odesc)
            return;
        hyEnt **hent = sdesc->getHY();
        if (!hent)
            return;
        for ( ; *hent; hent++) {
            if (odesc->type() == CDINSTANCE) {
                if ((*hent)->odesc() != odesc) {
                    hyParent *p;
                    for (p = (*hent)->parent(); p; p = p->next()) {
                        if (p->cdesc() == odesc)
                            break;
                    }
                    if (p) {
                        // The parent is being moved
                        if (undoable)
                            // add to undo list
                            CD()->ifRecordHYent(*hent, true);
                        p->set_cdesc((CDc*)new_odesc);
                    }
                    else {
                        for (p = (*hent)->proxy(); p; p = p->next()) {
                            if (p->cdesc() == odesc)
                                break;
                        }
                        if (p) {
                            // The parent is being moved
                            if (undoable)
                                // add to undo list
                                CD()->ifRecordHYent(*hent, true);
                            p->set_cdesc((CDc*)new_odesc);
                            int x = p->posx();
                            int y = p->posy();
                            stk->TPoint(&x, &y);
                            p->set_posx(x);
                            p->set_posy(y);
                        }
                    }
                    continue;
                }
            }

            // wire or call
            if (odesc == (*hent)->odesc()) {
                if (undoable)
                    CD()->ifRecordHYent(*hent, true);  // add to undo list
                int x = (*hent)->pos_x();
                int y = (*hent)->pos_y();
                stk->TPoint(&x, &y);
                (*hent)->set_pos_x(x);
                (*hent)->set_pos_y(y);
                (*hent)->set_odesc(new_odesc);

                switch ((*hent)->orient()) {
                default:
                case HYorNone:
                    continue;
                case HYorDn:
                    x = 0;
                    y = -1;
                    break;
                case HYorRt:
                    x = 1;
                    y = 0;
                    break;
                case HYorUp:
                    x = 0;
                    y = 1;
                    break;
                case HYorLt:
                    x = -1;
                    y = 0;
                    break;
                }
                int x1 = 0;
                int y1 = 0;
                stk->TPoint(&x, &y);
                stk->TPoint(&x1, &y1);
                x -= x1;
                y -= y1;

                if (x == 0) {
                    if (y == 1)
                        (*hent)->set_orient(HYorUp);
                    else
                        (*hent)->set_orient(HYorDn);
                }
                else {
                    if (x == 1)
                        (*hent)->set_orient(HYorRt);
                    else
                        (*hent)->set_orient(HYorLt);
                }
            }
        }
    }
}


// Fix references attached to objects being moved.
//
void
CDs::hyTransformMove(cTfmStack *stk, const CDo *odesc, CDo *new_odesc,
    bool undoable)
{
    if (isElectrical()) {
        if (!odesc || !new_odesc)
            return;
        CDgenHierUp_s gen(this);
        bool err;
        CDs *sd;
        while ((sd = gen.next(&err)) != 0)
            hy_tx_move_core(sd, stk, odesc, new_odesc, undoable);
    }
}


namespace {
    // Fix references attached to wires in sdesc that are being stretched.
    //
    void hy_tx_stretch_core(const CDs *sdesc, const CDo *odesc,
        CDo *new_odesc, int ref_x, int ref_y, int new_x, int new_y,
        bool undoable)
    {
        if (!odesc)
            return;
        hyEnt **hent = sdesc->getHY();
        if (!hent)
            return;
        for ( ; *hent; hent++) {
            if ((*hent)->odesc() != odesc)
                continue;
            Point_c px((*hent)->pos_x(), (*hent)->pos_y());
            const Point *p = ((CDw*)odesc)->points();
            int nump = ((CDw*)odesc)->numpts() - 1;
            for (int i = 0; i < nump; i++) {
                double x1, y1, d, d0;
                if (!Point::inPath(p+i, &px, 20, 0, 2))
                    continue;
                if (ref_x == p[i].x && ref_y == p[i].y) {
                    x1 = p[i+1].x - (*hent)->pos_x();
                    y1 = p[i+1].y - (*hent)->pos_y();
                }
                else if (ref_x == p[i+1].x && ref_y == p[i+1].y) {
                    x1 = (*hent)->pos_x() - p[i].x;
                    y1 = (*hent)->pos_y() - p[i].y;
                }
                else {
                    // shouldn't happen
                    return;
                }
                d  = x1*x1 + y1*y1;
                x1 = p[i+1].x - p[i].x;
                y1 = p[i+1].y - p[i].y;
                d0 = x1*x1 + y1*y1;
                d = sqrt(d/d0);
                if (undoable)
                    CD()->ifRecordHYent(*hent, true);  // add to undo list
                (*hent)->set_pos_x((*hent)->pos_x() + mmRnd((new_x - ref_x)*d));
                (*hent)->set_pos_y((*hent)->pos_y() + mmRnd((new_y - ref_y)*d));
                (*hent)->set_odesc(new_odesc);
                break;
            }
        }
    }
}


// Fix references attached to wires that are being stretched.
//
void
CDs::hyTransformStretch(const CDo *odesc, CDo *new_odesc,
    int ref_x, int ref_y, int new_x, int new_y, bool undoable)
{
    if (isElectrical()) {
        if (!odesc || odesc->type() != CDWIRE)
            return;
        if (!new_odesc || new_odesc->type() != CDWIRE)
            return;
        CDgenHierUp_s gen(this);
        bool err;
        CDs *sd;
        while ((sd = gen.next(&err)) != 0) {
            hy_tx_stretch_core(sd, odesc, new_odesc, ref_x, ref_y,
                new_x, new_y, undoable);
        }
    }
}


namespace {
    // Change references in sdesc from odesc to new_odesc.
    //
    void hy_merge_ref_core(const CDs *sdesc, const CDo *odesc, CDo *new_odesc,
        bool undoable)
    {
        if (!odesc)
            return;
        hyEnt **hent = sdesc->getHY();
        if (!hent)
            return;
        for ( ; *hent; hent++) {
            if ((*hent)->odesc() != odesc)
                continue;
            if (undoable)
                CD()->ifRecordHYent(*hent, true);  // add to undo list
            (*hent)->set_odesc(new_odesc);
        }
    }
}


// The wire referenced by odesc is being merged into new_odesc.
// Change the references in the current and parent cells.
//
void
CDs::hyMergeReference(const CDo *odesc, CDo *new_odesc, bool undoable)
{
    if (isElectrical()) {
        if (!odesc || odesc->type() != CDWIRE)
            return;
        if (!new_odesc || new_odesc->type() != CDWIRE)
            return;
        CDgenHierUp_s gen(this);
        bool err;
        CDs *sd;
        while ((sd = gen.next(&err)) != 0)
            hy_merge_ref_core(sd, odesc, new_odesc, undoable);
    }
}


namespace {
    // The object referenced by pointer is being deleted from sdesc.
    // Null the references in sdesc.
    //
    void hy_delete_ref_core(const CDs *sdesc, const CDo *odesc, bool undoable)
    {
        if (!odesc)
            return;
        hyEnt **hent = sdesc->getHY();
        if (!hent)
            return;
        for ( ; *hent; hent++) {
            if ((*hent)->odesc() != odesc) {
                hyParent *p = 0;
                if (odesc->type() == CDINSTANCE) {
                    for (p = (*hent)->parent(); p; p = p->next()) {
                        if (p->cdesc() == odesc)
                            break;
                    }
                    if (!p) {
                        for (p = (*hent)->proxy(); p; p = p->next()) {
                            if (p->cdesc() == odesc)
                                break;
                        }
                    }
                }
                if (!p)
                    continue;
            }
            if (undoable)
                CD()->ifRecordHYent(*hent, true);  // add to undo list

            (*hent)->set_noref();
        }
    }
}


// The object referenced by odesc is being deleted.  Null the
// references in the cells.
//
void
CDs::hyDeleteReference(const CDo *odesc, bool undoable)
{
    if (isElectrical()) {
        if (!odesc)
            return;
        CDgenHierUp_s gen(this);
        bool err;
        CDs *sd;
        while ((sd = gen.next(&err)) != 0)
            hy_delete_ref_core(sd, odesc, undoable);
    }
}
// End of CDs functions.


hyEnt::hyEnt(CDs *sd, int x, int y, CDo *od, HYrefType r, HYorType o)
{
    hyX = x;
    hyY = y;
    hySdesc = sd;
    hyOdesc = od;
    hyRefType = r;
    hyOrient = o;
}


hyEnt::hyEnt(CDs *sd, int x, int y, int ix)
{
    hyX = x;
    hyY = y;
    hySdesc = sd;
    hyRefType = HYrefNode;
    hyTindex = ix,
    hyIsTerm = true;
}


hyEnt::~hyEnt()
{
    if (hyLinked && hySdesc) {
        hyEnt **oldh = hySdesc->getHY();
        if (oldh) {
            for (int i = 0; oldh[i]; i++) {
                if (oldh[i] == this) {
                    for ( ; oldh[i]; i++)
                        oldh[i] = oldh[i+1];
                    break;
                }
            }
        }
    }

    CD()->ifRecordHYent(this, false);  // purge from undo list
    hyParent::destroy(hyPrnt);
    hyParent::destroy(hyPrxy);
}


// Add ent to the hypertext database for sdesc.  This is called in the
// hyList constructor, dup(), and the editor, so that this call is
// needed only if an hyEnt struct is explicitly created in a user
// function and never copied or pushed through the editor.
//
bool
hyEnt::add()
{
    if (!hySdesc || hyLinked)
        return (true);
    // Presently, there are no hypertxt properties in Physical mode.
    if (!hySdesc->isElectrical()) {
        hySdesc = CDcdb()->findCell(hySdesc->cellname(), Electrical);
        if (!hySdesc)
            return (false);
    }
    // hySdesc sets the cell where entries are recorded.
    hyEnt **oldh = hySdesc->getHY();
    if (!oldh) {
        hyEnt **newh = new hyEnt*[2];
        newh[0] = this;
        newh[1] = 0;
        hySdesc->setHY(newh);
        hyLinked = true;
        return (true);
    }
    int i, j;
    for (i = 0; oldh[i]; i++) ;
    hyEnt **newh = new hyEnt*[i+2];
    for (j = 0; j < i; j++)
        newh[j] = oldh[j];
    newh[j++] = this;
    newh[j] = 0;
    delete [] oldh;
    hySdesc->setHY(newh);
    hyLinked = true;
    return (true);
}


bool
hyEnt::remove()
{
    if (!hySdesc || !hyLinked)
        return (true);
    hyEnt **oldh = hySdesc->getHY();
    if (oldh) {
        for (int i = 0; oldh[i]; i++) {
            if (oldh[i] == this) {
                for ( ; oldh[i]; i++)
                    oldh[i] = oldh[i+1];
                hyLinked = false;
                return (true);
            }
        }
    }
    return (false);
}


// Copy a hypertext entry.
//
hyEnt *
hyEnt::dup() const
{
    hyEnt *newh = new hyEnt(*this);
    newh->hyPrnt = hyParent::dup(hyPrnt);
    newh->hyPrxy = hyParent::dup(hyPrxy);
    newh->hyLinked = false;
    if (hyLinked)
        newh->add();
    return (newh);
}


// Find the node number for the entry.
//
int
hyEnt::nodenum() const
{
    if (hyRefType == HYrefNode) {
        if (hyOdesc) {
            if (hyOdesc->type() == CDWIRE) {
                CDp_node *pn = (CDp_node*)hyOdesc->prpty(P_NODE);
                if (pn)
                    return (pn->enode());
            }
            else if (hyOdesc->type() == CDINSTANCE) {
                CDp_cnode *pn = (CDp_cnode*)hyOdesc->prpty(P_NODE);
                for ( ; pn; pn = pn->next()) {
                    for (unsigned int ix = 0; ; ix++) {
                        int x, y;
                        if (!pn->get_pos(ix, &x, &y))
                            break;
                        if (hyX == x && hyY == y)
                            return (pn->enode());
                    }
                }
            }
        }
        else if (hyIsTerm) {
            CDp_snode *pn = (CDp_snode*)hySdesc->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                if (pn->index() == hyTindex)
                    return (pn->enode());
            }
        }
    }
    return (-1);
}


// Transform the reference point to top-level coordinates.
//
void
hyEnt::get_tfpoint(int *x, int *y) const
{
    // The coordinate is relative to the cell of the instance
    // containing the referenced object.  If there is a parent list,
    // the coordinate must be transformed back to the top level.  Why
    // not save the top level coordinate in the ent's instead?
    // Because for example if one of the parents is moved, then we
    // would have to search through the list of saved ents for those
    // in or under the moved instance and fix the coordinates, an ugly
    // job.  This way, we have only to fix the coordinates of objects
    // directly moved, which can be identified with a simple
    // comparison of the pointer field.
    //
    *x = hyX;
    *y = hyY;
    if (hyPrnt) {
        cTfmStack stk;
        stk.TPush();
        for (hyParent *p = hyPrnt; p; p = p->next()) {
            stk.TPush();
            stk.TApplyTransform(p->cdesc());
            stk.TPremultiply();
        }
        stk.TPoint(x, y);
        for (hyParent *p = hyPrnt; p; p = p->next())
            stk.TPop();
        stk.TPop();
    }
}


namespace {
    // Return a pointer to a subcircuit under x,y if any.
    //
    CDc *find_instance(CDs *sdesc, int x, int y)
    {
        if (!sdesc || !sdesc->isElectrical())
            return (0);
        BBox BB(x, y, x, y);
        BB.bloat(10);
        CDg gdesc;
        gdesc.init_gen(sdesc, CellLayer(), &BB);
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {
            CDs *msd = cdesc->masterCell();
            if (!msd)
                continue;
            if (msd->elecCellType() != CDelecSubc)
                continue;
            break;
        }
        return (cdesc);
    }
}


// Return the string associated with ent, if successful
// update entries in ent.
//
char *
hyEnt::stringUpdate(cTfmStack *tstk)
{
    cTfmStack stk;
    if (!tstk)
        tstk = &stk;

    if (hyIsTerm) {
        // This points to a hySdesc terminal location.  There can be
        // no parent or proxy list.

        hyParent::destroy(hyPrnt);
        hyParent::destroy(hyPrxy);
        hyFixProxy = false;

        return (get_subname(true));
    }

    if (hyFixProxy) {
        hyFixProxy = false;

        // In the coordinates list used in the ASCII representation,
        // the first coordinate is the top-level instance reference
        // location, the second is the next instance reference
        // location, and the final point is the object reference
        // location.  Each location is relative to its immediate
        // container.
        //
        // At this point, just after reading the ASCII data, the
        // element x,y is the first coordinate, and the hyPrxy list
        // provides the additional coordinates in order.
        //
        // The proxy list after processing starts with the immediate
        // parent instance and ends with the top level instance.  The
        // element x,y is the reference location.
        //
        // So, we need to
        // 1. Permute the coordinates to the proper order and storage
        //    location.
        // 2. Walk the hierarchy and identify the instance parents.

        if (hyPrxy) {
            if (hyPrxy->next()) {
                // Move last element to first, and swap coordinates
                // with the element.

                hyParent *pv = 0;
                for (hyParent *p = hyPrxy; p->next(); pv = p, p = p->next()) ;

                hyParent *p = pv->next();
                pv->set_next(0);
                p->set_next(hyPrxy);
                hyPrxy = p;
                // We need to reverse the list later.
            }
            int x = hyX;
            int y = hyY;
            hyX = hyPrxy->posx();
            hyY = hyPrxy->posy();
            hyPrxy->set_posx(x);
            hyPrxy->set_posy(y);

            // Now try to get the instance pointers.  This is easier
            // with the present list order, so we defer list reversal.

            CDs *sd = hySdesc;
            for (hyParent *p = hyPrxy; p; p = p->next()) {
                CDc *cdesc = find_instance(sd, p->posx(), p->posy());
                if (!cdesc) {
                    set_noref();
                    return (0);
                }
                p->set_cdesc(cdesc);
                sd = cdesc->masterCell();
                if (!sd) {
                    set_noref();
                    return (0);
                }
            }

            if (hyPrxy->next()) {
                // Now reverse the proxy list.
                hyParent *p0 = 0;
                while (hyPrxy) {
                    hyParent *p = hyPrxy;
                    hyPrxy = hyPrxy->next();
                    p->set_next(p0);
                    p0 = p;
                }
                hyPrxy = p0;
            }
        }
    }

    int x, y;
    get_tfpoint(&x, &y);
    tstk->TPoint(&x, &y);
    BBox BB;
    int delta = 10; // some slop, should already be centered
    BB.left = x - delta;
    BB.right = x + delta;
    BB.bottom = y - delta;
    BB.top = y + delta;

    hyEnt *newent;
    char *s = container()->hyString(tstk, &BB, hyRefType, &newent);
    if (s) {
        if (newent->hyIsTerm) {
            // Found a terminal instead of an object, no good.
            delete newent;
            delete [] s;
            return (0);
        }
        hyParent::destroy(hyPrnt);
        hyPrnt = newent->hyPrnt;
        newent->hyPrnt = 0;

        hyOdesc = newent->hyOdesc;
        hyRefType = newent->hyRefType;
        hyOrient = newent->hyOrient;

        // Don't update x, y! Can change by a pixel, then we lose marks
        // If there is a big change in x, y then we probably have top
        // level coordinates from an ascii string in ent, in which case
        // update x, y.
        //
        if (abs(newent->hyX - hyX) > 1 || abs(newent->hyY - hyY) > 1) {
            hyX = newent->hyX;
            hyY = newent->hyY;
        }
        delete newent;
    }
    if (hyPrxy) {
        delete [] s;
        s = get_subname(true);
    }

    return (s);
}


// Return the appropriate string name for the property.  If addv is true,
// return v(nodename) for node names.
//
char *
hyEnt::get_subname(bool addv) const
{
    if (hyRefType == HYrefNode) {
        char *tname = get_nodename(nodenum());
        if (!tname)
            return (0);
        if (addv) {
            char *tt = new char[strlen(tname) + 4];
            tt[0] = 'v';
            tt[1] = '(';
            char *s = lstring::stpcpy(tt + 2, tname);
            *s++ = ')';
            *s = 0;
            delete [] tname;
            tname = tt;
        }
        return (tname);
    }
    if (hyOdesc && hyOdesc->type() == CDINSTANCE) {
        CDs *msdesc = ((CDc*)hyOdesc)->masterCell();
        if (hyRefType == HYrefBranch && msdesc && msdesc->isDevice())
            return (parse_bstring());
        if (hyRefType == HYrefDevice && msdesc)
            return (get_devname());
    }
    // Note that HY_CELL and HY_LABEL get here.
    return (lstring::copy("unknown"));
}


// Return the expanded branch string.
//
char *
hyEnt::parse_bstring() const
{
    const char *badstr = "_UNKNOWN_";
    if (!hyOdesc)
        return (lstring::copy(badstr));
    CDo *od = hyOdesc;
    CDp *pd;
    for (pd = od->prpty_list(); pd; pd = pd->next_prp()) {
        if (pd->value() == P_BRANCH)
            break;
    }
    if (!pd)
        return (lstring::copy(badstr));
    sLstr lstr;
    if (!((CDp_branch*)pd)->br_string()) {
        // default is <name>#branch
        char *dn = get_devname();
        lstr.add(dn);
        delete [] dn;
        lstr.add("#branch");
        return (lstr.string_trim());
    }
    const char *s = ((CDp_branch*)pd)->br_string();
    while (*s) {
        if (*s != '<') {
            lstr.add_c(*s++);
            continue;
        }
        s++;
        if (*s == 'v' && *(s+1) == '>') {
            s += 2;
            // expand <v> to device voltage, assume first two terminals
            CDp_cnode *pn = (CDp_cnode*)od->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                if (pn->index() == 0)
                    break;
            }
            if (!pn)
                return (lstring::copy(badstr));
            char *nn_1 = get_nodename(pn->enode());
            pn = (CDp_cnode*)od->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                if (pn->index() == 1)
                    break;
            }
            if (!pn)
                return (lstring::copy(badstr));
            char *nn_2 = get_nodename(pn->enode());
            if (strlen(nn_1) > 1 || *nn_1 != '0') {
                if (strlen(nn_2) > 1 || *nn_2 != '0') {
                    lstr.add("(v(");
                    lstr.add(nn_1);
                    lstr.add(")-v(");
                    lstr.add(nn_2);
                    lstr.add("))");
                }
                else {
                    lstr.add("v(");
                    lstr.add(nn_1);
                    lstr.add_c(')');
                }
            }
            else {
                if (strlen(nn_2) > 1 || *nn_2 != '0') {
                    lstr.add("-v(");
                    lstr.add(nn_2);
                    lstr.add_c(')');
                }
                else {
                    lstr.add_c('0');
                }
            }
            delete [] nn_1;
            delete [] nn_2;
        }
        else if (*s == 'v' && *(s+1) == 'a' && *(s+2) == 'l' &&
                *(s+3) == 'u' && *(s+4) == 'e' && *(s+5) == '>') {
            s += 6;
            // expand <value> to value of device
            for (pd = od->prpty_list(); pd; pd = pd->next_prp()) {
                if (pd->value() == P_VALUE)
                    break;
            }
            if (!pd)
                return (lstring::copy(badstr));
            char *str = hyList::string(((CDp_user*)pd)->data(), HYcvPlain,
                false);
            if (!str)
                return (lstring::copy(badstr));
            lstr.add(str);
            delete [] str;
        }
        else if (*s == 'n' && *(s+1) == 'a' && *(s+2) == 'm' &&
                *(s+3) == 'e' && *(s+4) == '>') {
            s += 5;
            // expand <name> to device name
            char *dv = get_devname();
            lstr.add(dv);
            delete [] dv;
        }
        else {
            // no token match, take as literal
            lstr.add_c('<');
            if (!*s)
                break;
            lstr.add_c(*s++);
        }
    }
    return (lstr.string_trim());
}


// Return the path-expanded device name, caller must free.
//
char *
hyEnt::get_devname() const
{
    const char *badstr = "_UNKNOWN_";
    if (!hyOdesc || hyOdesc->type() != CDINSTANCE)
        return (lstring::copy(badstr));

    sLstr lstr;
    if (CD()->GetSubcCatmode() == cCD::SUBC_CATMODE_WR) {
        hyParent *stack[CDMAXCALLDEPTH + 1];
        int nl = 0;
        for (hyParent *p = hyPrxy; p; p = p->next())
            nl++;
        int sp = -1;
        for (hyParent *p = hyPrxy; p; p = p->next()) {
            sp++;
            stack[nl - sp - 1] = p;
        }
        for (hyParent *p = hyPrnt; p; p = p->next())
            stack[++sp] = p;

        lstr.add(((CDc*)hyOdesc)->getBaseName());
        if (sp >= 0) {
            for ( ; sp >= 0; sp--) {
                lstr.add_c(CD()->GetSubcCatchar());
                lstr.add_c('x');
                lstr.add(stack[sp]->cdesc()->getBaseName() + 1);
            }
        }
    }
    else {
        const char *nbuf = ((CDc*)hyOdesc)->getBaseName();
        lstr.add_c(*nbuf);
        hyParent *p = hyPrnt;
        if (p)
            lstr.add_c(CD()->GetSubcCatchar());
        for ( ; p; p = p->next()) {
            lstr.add(p->cdesc()->getBaseName() + 1);
            lstr.add_c(CD()->GetSubcCatchar());
        }
        lstr.add(nbuf+1);
    }
    return (lstr.string_trim());
}


namespace {
    // If the selected node is tied to a node in the parent
    // subcircuit, return the parent's node, otherwise
    // return -1.
    //
    inline int
    get_enode(int node, CDc *cdesc)
    {
        CDs *sdesc = cdesc->masterCell(true);
        CDp_snode *ps = (CDp_snode*)sdesc->prpty(P_NODE);
        for ( ; ps; ps = ps->next()) {
            if (node == ps->enode()) {
                CDp_cnode *pc = (CDp_cnode*)cdesc->prpty(P_NODE);
                for ( ; pc; pc = pc->next()) {
                    if (ps->index() == pc->index())
                        return (pc->enode());
                }
                break;
            }
        }
        return (-1);
    }
}


// Return the path-expanded node string for node.
//
char *
hyEnt::get_nodename(int node) const
{
    if (node == 0)
        return (lstring::copy("0"));

    hyParent *stack[CDMAXCALLDEPTH + 1];
    int nl = 0;
    for (hyParent *p = hyPrxy; p; p = p->next())
        nl++;
    int sp = -1;
    for (hyParent *p = hyPrxy; p; p = p->next()) {
        sp++;
        stack[nl - sp - 1] = p;
    }
    for (hyParent *p = hyPrnt; p; p = p->next())
        stack[++sp] = p;
    for ( ; sp >= 0; sp--) {
        int nnode = get_enode(node, stack[sp]->cdesc());
        if (nnode == -1)
            break;
        node = nnode;
    }
    sLstr lstr;
    if (CD()->GetSubcCatmode() == cCD::SUBC_CATMODE_WR) {
        CDc *lastpar = sp >= 0 ? stack[sp]->cdesc() : 0;

        // Deal with node name mapping.
        CDs *sd = (lastpar ? lastpar->masterCell() : hySdesc);
        bool isglobal;
        const char *nn = CD()->ifNodeName(sd, node, &isglobal);
        if (!nn || !*nn)
            lstr.add_i(node);
        else {
            if (isglobal)
                return (lstring::copy(nn));
            lstr.add(nn);
        }

        for ( ; sp >= 0; sp--) {
            lstr.add_c(CD()->GetSubcCatchar());
            lstr.add_c('x');
            const char *instname = stack[sp]->cdesc()->getBaseName();
            lstr.add(instname+1);
        }
    }
    else {
        CDc *lastpar = sp >= 0 ? stack[sp]->cdesc() : 0;

        // create text and free
        for (int i = 0; i <= sp; i++) {
            const char *instname = stack[i]->cdesc()->getBaseName();
            lstr.add(instname+1);
            lstr.add_c(CD()->GetSubcCatchar());
        }

        // Deal with node name mapping.
        CDs *sd = (lastpar ? lastpar->masterCell() : hySdesc);
        bool isglobal;
        const char *nn = CD()->ifNodeName(sd, node, &isglobal);
        if (!nn || !*nn)
            lstr.add_i(node);
        else {
            if (isglobal)
                return (lstring::copy(nn));
            lstr.add(nn);
        }
    }
    return (lstr.string_trim());
}


// Static function.
// Compare functions for hypertext entries.
//
int
hyEnt::hy_strcmp(hyEnt *ent1, hyEnt *ent2)
{
    int j = 1;
    if (ent1 && ent2 && ent1->ref_type() == ent2->ref_type()) {
        char *s1 = ent1->stringUpdate(0);
        char *s2 = ent2->stringUpdate(0);
        if (s1 && s2)
            j = strcmp(s1, s2);
        delete [] s1;
        delete [] s2;
    }
    return (j);
}
// End of hyEnt functions.


hyList::hyList(CDs *sdesc, const char *s, HYcvType cvtype)
{
    hlRefType = HLrefText;
    hlText = 0;
    hlEnt = 0;
    hlNext = 0;
    if (!s)
        s = "";
    while (isspace(*s))
        s++;

    if (cvtype == HYcvPlain) {
        // copy string verbatim
        hlText = lstring::copy(s);
        return;
    }

    // Return a hypertext list from ascii encoded string.  Called during
    // cell parsing.
    //
    sLstr lstr;
    hyList *hh = this, *h0 = 0;
    while (*s) {
        const char *ts;
        char *tok;
        const char *nxt = hy_token(s, &ts, &tok);
        if (tok) {
            for ( ; s < ts; s++)
                lstr.add_c(*s);

            int x, y, ref, n;
            if (sscanf(tok, HYfmtRefBegin"%n", &ref, &x, &y, &n) >= 3) {
                // Token: (||reftype:x y [x y]...||) for hypertext entry
                // shouldn't get this after "long text"

                hyParent *p0 = new hyParent(0, x, y, 0);
                hyParent *pe = p0;

                const char *ss = tok + n;
                while (sscanf(ss, "%d %d%n", &x, &y, &n) >= 2) {
                    ss += n;
                    pe->set_next(new hyParent(0, x, y, 0));
                    pe = pe->next();
                }

                if (lstr.length()) {
                    if (!h0)
                        h0 = hh = this;
                    else {
                        hh->hlNext = new hyList;
                        hh = hh->hlNext;
                    }
                    hh->hlRefType = HLrefText;
                    hh->hlText = lstr.string_trim();
                    lstr.free();
                }
                if (!h0)
                    h0 = hh = this;
                else {
                    hh->hlNext = new hyList;
                    hh = hh->hlNext;
                }
                // Just store coords in hyEnt, will fill in pointers
                // later.

                HLrefType hlref;
                HYrefType hyref;
                switch (ref) {
                case HLrefNode:
                    hlref = HLrefNode;
                    hyref = HYrefNode;
                    break;
                case HLrefBranch:
                    hlref = HLrefBranch;
                    hyref = HYrefBranch;
                    break;
                case HLrefDevice:
                    hlref = HLrefDevice;
                    hyref = HYrefDevice;
                    break;
                default:
                    hlref = HLrefEnd;
                    hyref = HYrefBogus;
                    break;
                }

                hh->hlRefType = hlref;
                hh->hlEnt = new hyEnt(sdesc, p0->posx(), p0->posy(), 0,
                    hyref, HYorNone);
                pe = p0;
                p0 = p0->next();
                delete pe;
                if (p0) {
                    hh->hlEnt->set_proxy(p0);
                    hh->hlEnt->set_fix_proxy(true);
                    // The hlEnt has the wrong sdesc and coordinates.
                }

                hh->hlEnt->add();
            }
            else if (!strcmp(tok, "sc")) {
                // Token: (||sc||)  for encoded semicolon
                lstr.add_c(';');
            }
            else if (!strcmp(tok, "text")) {
                // Token: (||text||) for long text, must be first
                if (hh == this) {
                    hh->hlRefType = HLrefLongText;
                    // strip semicolon refs
                    char *t = hyList::hy_strip(nxt);
                    lstr.add(t);
                    delete [] t;
                    hh->hlText = lstr.string_trim();
                    lstr.free();
                    delete [] tok;
                    break;
                }
            }
            else {
                // unknown token, take as text
                lstr.add(HYtokPre);
                lstr.add(tok);
                lstr.add(HYtokSuf);
            }
            delete [] tok;
        }
        else {
            lstr.add(s);
            if (!h0)
                h0 = hh = this;
            else {
                hh->hlNext = new hyList;
                hh = hh->hlNext;
            }
            hh->hlRefType = HLrefText;
            hh->hlText = lstr.string_trim();
            lstr.free();
            break;
        }
        s = nxt;
    }
    if (hlRefType == HLrefText && !hlText)
        hlText = lstring::copy("");
}


namespace {
    // Return the name for the script.
    //
    char *
    script_name(char *str)
    {
        char *name = 0;
        lstring::advtok(&str);
        while (*str) {
            if (lstring::ciprefix("name=", str)) {
                str += 5;
                name = lstring::getqtok(&str);
                break;
            }
            if (lstring::ciprefix("path=", str)) {
                str += 5;
                lstring::advqtok(&str);
                continue;
            }
            break;
        }
        if (!name)
            name = lstring::copy(HY_SCRNAME);
        return (name);
    }
}


// Static function.
// Copy a hypertext list.
//
hyList *
hyList::dup(const hyList *thishy)
{
    hyList *h0 = 0, *h1 = 0;
    for (const hyList *hh = thishy; hh; hh = hh->hlNext) {
        if (!h0)
            h0 = h1 = new hyList(hh->hlRefType);
        else {
            h1->hlNext = new hyList(hh->hlRefType);
            h1 = h1->hlNext;
        }
        if (hh->hlRefType == HLrefText || hh->hlRefType == HLrefLongText) {
            if (hh->hlText)
                h1->hlText = lstring::copy(hh->hlText);
        }
        else if (hh->hlEnt)
            h1->hlEnt = hh->hlEnt->dup();
    }
    const hyList *hyl = thishy;
    if (hyl && hyl->hlRefType == HLrefLongText)
        HYlt::lt_copy(hyl, h0);
    return (h0);
}


// Return an expanded string with up to date entries.
//
char *
hyList::string_prv(HYcvType cvtype, bool allow_long)
{
    hyList *hh;
    char *s;
    sLstr lstr;
    if (cvtype == HYcvPlain) {
        if (!allow_long && is_label_script())
            return (script_name(hlText));
        if (hlRefType == HLrefLongText)
            return (allow_long ?
                lstring::copy(hlText) : lstring::copy(HY_LT_MSG));
        for (hh = this; hh; hh = hh->hlNext) {
            s = hh->get_entry_string_prv();
            if (!s)
                lstr.add("_UNKNOWN_");
            else {
                lstr.add(s);
                delete [] s;
            }
        }
        return (lstr.string_trim());
    }

    // Return an expanded string with ascii encoded entries.  Used when
    // writing to file.
    //
    if (hlRefType == HLrefLongText) {
        lstr.add(HYtokPre);
        lstr.add(HYtokLT);
        lstr.add(HYtokSuf);
        if (!allow_long)
            lstr.add(HY_LT_MSG);
        else if (hlText)  {
            for (char *t = hlText; *t; t++) {
                if (*t == ';') {
                    lstr.add(HYtokPre);
                    lstr.add(HYtokSC);
                    lstr.add(HYtokSuf);
                }
                else
                    lstr.add_c(*t);
            }
        }
        return (lstr.string_trim());
    }
    for (hh = this; hh; hh = hh->hlNext) {
        if (hh->hlRefType == HLrefText) {
            if (hh->hlText) {
                for (char *t = hh->hlText; *t; t++) {
                    if (*t == ';') {
                        lstr.add(HYtokPre);
                        lstr.add(HYtokSC);
                        lstr.add(HYtokSuf);
                    }
                    else
                        lstr.add_c(*t);
                }
            }
        }
        else if (hh->hlEnt) {
            // The top level coordinates are saved in the ascii reference.
            // Hope we can find the same reference again!
            //
            // pre 4.2.9        type:x y
            // 4.2.9 and later  type:x y [x1 y1]...
            // Format according to HYfmtRefBegin.
            // Order: proxy_last ... proxy_first x,y

            lstr.add(HYtokPre);
            lstr.add_u(hh->hlRefType);
            lstr.add_c(':');

            hyParent *ary[CDMAXCALLDEPTH];
            int cnt = 0;
            for (hyParent *p = hh->hlEnt->proxy(); p; p = p->next())
                ary[cnt++] = p;
            bool first = true;
            while (cnt--) {
                hyParent *p = ary[cnt];
                if (!first)
                    lstr.add_c(' ');
                lstr.add_i(p->posx());
                lstr.add_c(' ');
                lstr.add_i(p->posy());
                first = false;
            }

            int x, y;
            hh->hlEnt->get_tfpoint(&x, &y);
            if (!first)
                lstr.add_c(' ');
            lstr.add_i(x);
            lstr.add_c(' ');
            lstr.add_i(y);
            lstr.add(HYtokSuf);
        }
    }
    return (lstr.string_trim());
}


// Return a string representing the list entry.
//
char *
hyList::get_entry_string_prv()
{
    switch (hlRefType) {
    case HLrefText:
        return (lstring::copy(hlText));
    case HLrefLongText:
        return (lstring::copy(HY_LT_MSG));
    case HLrefNode:
    case HLrefBranch:
    case HLrefDevice:
        if (!hlEnt || hlEnt->ref_type() == HYrefBogus)
            return (0);
        return (hlEnt->stringUpdate(0));
    default:
        return (0);
    }
}


// Trim off leading and trailing white space.
//
void
hyList::trim_white_space()
{
    if (hlRefType == HLrefText && hlText) {
        char *t = hlText;
        while (isspace(*t))
            t++;
        if (t != hlText) {
            char *s = hlText;
            while ((*s++ = *t++) != 0) ;
        }
    }
    hyList *h = this;
    while (h->hlNext)
        h = h->hlNext;
    if (h->hlRefType == HLrefText && hlText) {
        char *t = h->hlText + strlen(h->hlText) - 1;
        while (t >= h->hlText && isspace(*t))
            *t-- = 0;
    }
}


// Static function.
// Parse special token.  Tokens are
//  <HYtokPre>sc<HYtokSuf>        for encoded semicolon
//  <HYtokPre>text<HYtokSuf>      for long text block
//  <HYtokPre>%d:%d...<HYtokSuf>  for hypertext entry
// The position after token is returned, tstart is start of token.
//
const char *
hyList::hy_token(const char *str, const char **tstart, char **tok)
{
    const char *pre = HYtokPre;
    int plen = strlen(HYtokPre);
    const char *suf = HYtokSuf;
    int slen = strlen(HYtokSuf);
    for (const char *s = str; *s; s++) {
        int i;
        for (i = 0; i < plen; i++)
            if (*(s+i) != pre[i])
                break;
        if (i != plen)
            continue;
        // found prefix token
        *tstart = s;
        s += plen;
        while (isspace(*s))
            s++;
        for (const char *ss = s; *ss; ss++) {
            for (i = 0; i < slen; i++)
                if (*(ss+i) != suf[i])
                    break;
            if (i != slen)
                continue;
            // found suffix token
            char *token = new char[ss - s + 1];
            strncpy(token, s, ss - s);
            token[ss-s] = 0;
            // null trailing space
            s = ss + slen; // where to resume
            for (char *t = token + strlen(token) - 1;
                    t >= token && isspace(*t); t--)
                *t = 0;
            *tok = token;
            return (s);
        }
    }
    *tok = 0;
    return (0);
}


#define SCALE(n) mmRnd((n)*scale)

// Static function.
// Replace any hypertext references in str with the scaled values.  If
// references are found, str is deleted.  The new string, or str, is
// returned.
//
// I guess that it is theoretically possible to scale electrical data,
// but it is emphatically not recommended.
//
char *
hyList::hy_scale(char *str, double scale)
{
    sLstr lstr;
    if (!str)
        return (0);
    if (scale == 1.0)
        return (str);

    const char *pre = HYtokPre;
    int plen = strlen(HYtokPre);
    const char *suf = HYtokSuf;
    int slen = strlen(HYtokSuf);
    for (char *s = str; *s; ) {
        char *t = s;
        for (;;) {
            while (*t) {
                int i;
                for (i = 0; i < plen; i++) {
                    if (*(t+i) != pre[i])
                        break;
                }
                if (i == plen)
                    break;
                t++;
            }
            if (*t) {
                int i1, i2, i3, n;
                t += plen;
                int sc = sscanf(t, HYfmtRefBegin"%n", &i1, &i2, &i3, &n);
                if (sc >= 3) {
                    char c = *t;
                    *t = 0;
                    lstr.add(s);
                    *t = c;
                    lstr.add_u(i1);
                    lstr.add_c(':');
                    lstr.add_i(SCALE(i2));
                    lstr.add_c(' ');
                    lstr.add_i(SCALE(i3));

                    t += n;
                    while (sscanf(t, "%d %d%n", &i2, &i3, &n) >= 2) {
                        t += n;
                        lstr.add_c(' ');
                        lstr.add_i(SCALE(i2));
                        lstr.add_c(' ');
                        lstr.add_i(SCALE(i3));
                    }
                    lstr.add(HYtokSuf);
                    while (*t) {
                        int i;
                        for (i = 0; i < slen; i++) {
                            if (*(t+i) != suf[i])
                                break;
                        }
                        if (i == slen)
                            break;
                        t++;
                    }
                    if (*t)
                        t += slen;
                    break;
                }
            }
            else {
                if (s != str)
                    lstr.add(s);
                break;
            }
        }
        s = t;
    }
    if (lstr.length()) {
        delete [] str;
        str = lstr.string_trim();
    }
    return (str);
}


// Static function.
int
hyList::hy_strcmp(hyList *hl1, hyList *hl2)
{
    int j = 1;
    if (hl1 && hl2) {
        char *s1 = hl1->string_prv(HYcvPlain, false);
        char *s2 = hl2->string_prv(HYcvPlain, false);
        if (s1 && s2)
            j = strcmp(s1, s2);
        delete [] s1;
        delete [] s2;
    }
    return (j);
}


// Static function.
// Strip the string of the "long text" and semicolon encoding.
// Semicolons must be encoded due to CIF.  Note that this leaves
// ordinary hypertext references untouched, so there shouldn't be any
// in the text processed by this functon.
//
char *
hyList::hy_strip(const char *s)
{
    if (!s)
        s = "";
    while (isspace(*s))
        s++;

    sLstr lstr;
    while (*s) {
        const char *ts;
        char *tok;
        const char *nxt = hyList::hy_token(s, &ts, &tok);
        if (tok) {
            for ( ; s < ts; s++)
                lstr.add_c(*s);
            if (!strcmp(tok, "text")) {
                // long text, do nothing
                ;
            }
            else if (!strcmp(tok, "sc")) {
                // Token: (||sc||) for encoded semicolon
                lstr.add_c(';');
            }
            else {
                // unknown token, take as literal text
                lstr.add(HYtokPre);
                lstr.add(tok);
                lstr.add(HYtokSuf);
            }
            delete [] tok;
        }
        else {
            lstr.add(s);
            break;
        }
        s = nxt;
    }
    if (!lstr.string())
        return (lstring::copy(""));
    return (lstr.string_trim());
}
// End of hyList functions.


// To allow asynchronous edits of long text, we set up a list of
// entries containing long text, with tracking functions for copying
// and deletion.

namespace {
    // List item for long text destination.
    //
    struct LTdest
    {
        LTdest(hyList *hh, void(*ltcb)(hyList*, void*), void *ltarg,
            LTdest *n)
            {
                h = hh;
                cb = ltcb;
                arg = ltarg;
                next = n;
            }

        hyList *h;
        void (*cb)(hyList*, void*);
        void *arg;
        LTdest *next;
    };

    // List item for lists of long text destinations.
    struct LTdlist
    {
        LTdlist(LTdest *l, LTdlist *n) { lt = l; next = n; }

        LTdest *lt;
        LTdlist *next;
    };
}
LTdlist *HYlt::LtList;


// Static function.
void *
HYlt::lt_new(hyList *hh, void(*ltcb)(hyList*, void*), void *ltarg)
{
    LTdest *lt = new LTdest(hh, ltcb, ltarg, 0);
    LtList = new LTdlist(lt, LtList);
    return (LtList);
}


// Static function.
void
HYlt::lt_clear()
{
    // Called when all editor windows are being terminated.
    LTdlist *ln;
    for (LTdlist *l = LtList; l; l = ln) {
        ln = l->next;
        LTdest *ldn;
        for (LTdest *ld = l->lt; ld; ld = ldn) {
            ldn = ld->next;
            delete ld;
        }
        delete l;
    }
    LtList = 0;
}


// Static function.
void
HYlt::lt_update(void *arg, const char *string)
{
    LTdlist *lp = 0;
    for (LTdlist *l = LtList; l; l = l->next) {
        if (l == (LTdlist*)arg) {
            if (!lp)
                LtList = l->next;
            else
                lp->next = l->next;
            LTdest *ldn;
            for (LTdest *ld = l->lt; ld; ld = ldn) {
                ldn = ld->next;
                if (ld->h->ref_type() == HLrefLongText && string) {
                    delete [] ld->h->text();
                    ld->h->set_text(lstring::copy(string));
                    if (ld->cb)
                        (*ld->cb)(ld->h, ld->arg);
                }
                delete ld;
            }
            l->lt = 0;
            break;
        }
        lp = l;
    }
}


// Static function.
// Find the list containing ho, and add hn.
//
void
HYlt::lt_copy(const hyList *ho, hyList *hn)
{
    if (!ho || !hn)
        return;
    for (LTdlist *l = LtList; l; l = l->next) {
        for (LTdest *ld = l->lt; ld; ld = ld->next) {
            if (ld->h == ho) {
                l->lt = new LTdest(hn, 0, 0, l->lt);
                return;
            }
        }
    }
}


// Static function.
// Find the list containing hl, and delete the entry.
//
void
HYlt::lt_free(const hyList *hl)
{
    for (LTdlist *l = LtList; l; l = l->next) {
        LTdest *ldp = 0;
        for (LTdest *ld = l->lt; ld; ld = ld->next) {
            if (ld->h == hl) {
                if (!ldp)
                    l->lt = ld->next;
                else
                    ldp->next = ld->next;
                delete ld;
                return;
            }
            ldp = ld;
        }
    }
}
// End of HYlt functions.

