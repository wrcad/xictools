
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
 $Id: cd_objvirt.cc,v 5.18 2016/05/31 06:23:09 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "cd.h"
#include "cd_types.h"
#include "cd_propnum.h"
#include "cd_hypertext.h"
#include "cd_celldb.h"
#include "cd_terminal.h"
#include "geo_zlist.h"
#include "texttf.h"


//
// This implements the "virtual" functions for CDo and derivatives.
//
// In order to minimize in-core memory usage, facilitate application
// interfacing, and possibly improve performance, C++ virtual
// functions are not used in CDo.  Instead, we use the gcc computed
// goto extension for dispatching from the oType field.  The global
// static CDo_helper simply contains an offset translation method for
// dispatching.
//
// THIS CODE REQUIRES GCC, or a compiler that handles this syntax.

CDo_helper CDo::o_hlpr;


// This is called from the RTelem destructor, handles clean up.
//
void
CDo::destroy()
{
    {
        CDo *ot = this;
        if (!ot)
            return;
    }

#ifdef CD_PRPTY_TAB
    CD()->DestroyPrptyList(this);
#else
    oPrptyList->freeList();
    oPrptyList = 0;
#endif

#ifdef CD_GROUP_TAB
    // Leave the group number as the default.  This has no effect
    // unless the group number was set.  If a non-default group number
    // was assigned for this, the record is not purged from the table. 
    // If the address of this is ever re-used as a CDo, the CDo will
    // inherit the group record in the table.
    //
    CD()->SetGroup(this, DEFAULT_GROUP);
#endif
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
    return;
poly:
    delete [] ((CDpo*)this)->points();
    return;
wire:
    delete [] ((CDw*)this)->points();
    return;
label:
    hyList::destroy(((CDla*)this)->label());
    return;
inst:
    ((CDc*)this)->cleanup();
    return;
}


bool
CDo::intersect(const CDo *od, bool tok) const
{
    {
        const CDo *ot = this;
        if (!ot)
            return (false);
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
label:
inst:
    return (od->intersect(&oBB(), tok));
poly:
    if (!od->oBB().intersect(&oBB(), tok))
        return (false);
    if (od->type() != CDPOLYGON && od->type() != CDWIRE && oBB() <= od->oBB())
        return (true);
    {
        const Poly po(((const CDpo*)this)->po_poly());
        return (od->intersect(&po, tok));
    }
wire:
    if (!od->oBB().intersect(&oBB(), tok))
        return (false);
    if (od->type() != CDPOLYGON && od->type() != CDWIRE && oBB() <= od->oBB())
        return (true);
    {
        const Wire w(((const CDw*)this)->w_wire());
        return (od->intersect(&w, tok));
    }
}


bool
CDo::intersect(const Point *px, bool tok) const
{
    {
        const CDo *ot = this;
        if (!ot)
            return (false);
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
label:
inst:
    return (oBB().intersect(px, tok));
poly:
    return (oBB().intersect(px, tok) &&
        ((const CDpo*)this)->po_intersect(px, tok));
wire:
    return (oBB().intersect(px, tok) &&
        ((const CDw*)this)->w_intersect(px, tok));
}


bool
CDo::intersect(const BBox *BB, bool tok) const
{
    {
        const CDo *ot = this;
        if (!ot)
            return (false);
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
label:
inst:
    return (oBB().intersect(BB, tok));
poly:
    return (((const CDpo*)this)->po_intersect(BB, tok));
wire:
    return (((const CDw*)this)->w_intersect(BB, tok));
}


bool
CDo::intersect(const Poly *p, bool tok) const
{
    {
        const CDo *ot = this;
        if (!ot)
            return (false);
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
label:
inst:
    return (oBB().intersect(p, tok));
poly:
    return (((const CDpo*)this)->po_intersect(p, tok));
wire:
    return (((const CDw*)this)->w_intersect(p, tok));
}


bool
CDo::intersect(const Wire *wx, bool tok) const
{
    {
        const CDo *ot = this;
        if (!ot)
            return (false);
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
label:
inst:
    return (oBB().intersect(wx, tok));
poly:
    return (((const CDpo*)this)->po_intersect(wx, tok));
wire:
    return (((const CDw*)this)->w_intersect(wx, tok));
}


void
CDo::computeBB()
{
    {
        const CDo *ot = this;
        if (!ot)
            return;
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
    return;
poly:
    ((const CDpo*)this)->po_computeBB(&e_BB);
    return;
wire:
    ((const CDw*)this)->w_computeBB(&e_BB);
    return;
label:
    {
        CDla *ladesc = (CDla*)this;
        e_BB.left   = ladesc->xpos();
        e_BB.bottom = ladesc->ypos();
        e_BB.right  = ladesc->xpos() + ladesc->width();
        e_BB.top    = ladesc->ypos() + ladesc->height();
        Label::TransformLabelBB(ladesc->xform(), &e_BB, 0);
        return;
    }
inst:
    {
        cTfmStack stk;
        CDc *cdesc = (CDc*)this;
        CDs *sdesc = cdesc->masterCell();
        if (sdesc) {
            if (!sdesc->isBBsubng() && sdesc->BB()->left != CDinfinity) {

                // Cell exists, has no instances with undefined boundaries,
                // and has content.
                stk.TPush();
                stk.TApplyTransform(cdesc);
                e_BB = *sdesc->BB();
                CDap ap(cdesc);
                if (ap.nx > 1) {
                    if (ap.dx > 0)
                        e_BB.right += (ap.nx - 1)*ap.dx;
                    else
                        e_BB.left += (ap.nx - 1)*ap.dx;
                }
                if (ap.ny > 1) {
                    if (ap.dy > 0)
                        e_BB.top += (ap.ny - 1)*ap.dy;
                    else
                        e_BB.bottom += (ap.ny - 1)*ap.dy;
                }
                stk.TBB(&e_BB, 0);
                stk.TPop();
                return;
            }
        }

        // Parent sdesc now has bad BB.
        if (cdesc->parent())
            cdesc->parent()->reflectBadBB();
        e_BB.left = e_BB.right = cdesc->posX();
        e_BB.bottom = e_BB.top = cdesc->posY();
    }
}


double
CDo::area() const
{
    {
        const CDo *ot = this;
        if (!ot)
            return (0.0);
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
label:
inst:
    return (oBB().area());
poly:
    return (((const CDpo*)this)->po_area());
wire:
    return (((const CDw*)this)->w_area());
}


double
CDo::perim() const
{
    {
        const CDo *ot = this;
        if (!ot)
            return (0.0);
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
label:
inst:
    return (MICRONS(2*(oBB().width() + oBB().height())));
poly:
    return (MICRONS(((const CDpo*)this)->po_perim()));
wire:
    return (MICRONS(((const CDw*)this)->w_perim()));
}


// Return the centroid in microns.
//
void
CDo::centroid(double *pcx, double *pcy) const
{
    if (pcx)
        *pcx = 0.0;
    if (pcy)
        *pcy = 0.0;
    {
        const CDo *ot = this;
        if (!ot)
            return;
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
label:
inst:
    if (pcx)
        *pcx = 0.5*MICRONS(oBB().left + oBB().right);
    if (pcy)
        *pcy = 0.5*MICRONS(oBB().bottom + oBB().top);
    return;
poly:
    ((const CDpo*)this)->po_centroid(pcx, pcy);
    return;
wire:
    ((const CDw*)this)->w_centroid(pcx, pcy);
    return;
}


Zlist *
CDo::toZlist() const
{
    {
        const CDo *ot = this;
        if (!ot)
            return (0);
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
label:
    return (new Zlist(&e_BB));
poly:
    return (((const CDpo*)this)->po_toZlist());
wire:
    return (((const CDw*)this)->w_toZlist());
inst:
    {
        // do 45's right, if possible
        cTfmStack stk;
        CDc *cdesc = (CDc*)this;
        if (cdesc->master() && cdesc->master()->isNullCelldesc())
            return (new Zlist(&e_BB));
        CDs *sd = cdesc->masterCell();
        if (!sd)
            return (new Zlist(&e_BB));
        BBox cBB = *sd->BB();
        CDap ap(cdesc);
        if (ap.nx > 1 || ap.ny > 1) {
            cBB.right += (ap.nx-1)*ap.dx;
            cBB.top += (ap.ny-1)*ap.dy;
        }
        stk.TPush();
        stk.TApplyTransform(cdesc);
        Point *pts;
        stk.TBB(&cBB, &pts);
        stk.TPop();
        if (!pts)
            return (new Zlist(&cBB));
        Zlist *zn = Point::toZlist(pts, 5);
        delete [] pts;
        return (zn);
    }
}


Zlist *
CDo::toZlistR() const
{
    {
        const CDo *ot = this;
        if (!ot)
            return (0);
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
label:
    {
        Zlist *zl = new Zlist;
        zl->Z.xll = zl->Z.xul = -oBB().top;
        zl->Z.xlr = zl->Z.xur = -oBB().bottom;
        zl->Z.yl = oBB().left;
        zl->Z.yu = oBB().right;
        return (zl);
    }
poly:
    return (((const CDpo*)this)->po_toZlistR());
wire:
    return (((const CDw*)this)->w_toZlistR());
inst:
    {
        // do 45's right, if possible
        cTfmStack stk;
        CDc *cdesc = (CDc*)this;
        if (cdesc->master() && cdesc->master()->isNullCelldesc()) {
            Zlist *zl = new Zlist;
            zl->Z.xll = zl->Z.xul = -oBB().top;
            zl->Z.xlr = zl->Z.xur = -oBB().bottom;
            zl->Z.yl = oBB().left;
            zl->Z.yu = oBB().right;
            return (zl);
        }
        CDs *sd = cdesc->masterCell();
        if (!sd) {
            Zlist *zl = new Zlist;
            zl->Z.xll = zl->Z.xul = -oBB().top;
            zl->Z.xlr = zl->Z.xur = -oBB().bottom;
            zl->Z.yl = oBB().left;
            zl->Z.yu = oBB().right;
            return (zl);
        }
        BBox cBB = *sd->BB();
        CDap ap(cdesc);
        if (ap.nx > 1 || ap.ny > 1) {
            cBB.right += (ap.nx-1)*ap.dx;
            cBB.top += (ap.ny-1)*ap.dy;
        }
        stk.TPush();
        stk.TRotate(0, 1);
        stk.TPush();
        stk.TApplyTransform(cdesc);
        stk.TPremultiply();
        Point *pts;
        stk.TBB(&cBB, &pts);
        stk.TPop();
        stk.TPop();
        if (!pts)
            return (new Zlist(&cBB));
        Zlist *zn = Point::toZlist(pts, 5);
        delete [] pts;
        return (zn);
    }
}


// Duplicate the object, including copying the properties, into an
// object suitable for database insertion.
//
CDo *
CDo::dup() const
{
    {
        const CDo *ot = this;
        if (!ot)
            return (0);
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
    {
        CDo *newo = new CDo(ldesc(), &oBB());
        newo->prptyAddCopyList(prpty_list());
        return (newo);
    }
poly:
    {
        CDpo *newp = new CDpo(ldesc());
        newp->e_BB = oBB();
        newp->set_numpts(((const CDpo*)this)->numpts());
        newp->set_points(
            Point::dup(((const CDpo*)this)->points(), newp->numpts()));
        newp->prptyAddCopyList(prpty_list());
        return (newp);
    }
wire:
    {
        CDw *neww = new CDw(ldesc());
        neww->e_BB = oBB();
        neww->set_attributes(((const CDw*)this)->attributes());
        neww->set_numpts(((const CDw*)this)->numpts());
        neww->set_points(
            Point::dup(((const CDw*)this)->points(), neww->numpts()));
        neww->prptyAddCopyList(prpty_list());
        return (neww);
    }
label:
    {
        CDla *newla = new CDla(ldesc());
        newla->e_BB = oBB();
        newla->set_xpos(((CDla*)this)->xpos());
        newla->set_ypos(((CDla*)this)->ypos());
        newla->set_width(((CDla*)this)->width());
        newla->set_height(((CDla*)this)->height());
        newla->set_xform(((CDla*)this)->xform());
        newla->set_label(hyList::dup(((CDla*)this)->label()));
        newla->prptyAddCopyList(prpty_list());
        return (newla);
    }
inst:
    {
        CDc *newc = new CDc(ldesc());
        newc->e_BB = oBB();
        newc->setAttr(((CDc*)this)->attr());
        newc->setPosX(((CDc*)this)->posX());
        newc->setPosY(((CDc*)this)->posY());
        newc->setMaster(((CDc*)this)->master());
        newc->prptyAddCopyList(prpty_list());
        return (newc);
    }
    return (0);
}


// Unlike dup() above, this one calls sdesc->insert() and takes care of
// bound labels.
//
CDo *
CDo::dup(CDs *sdesc) const
{
    if (!sdesc)
        return (0);
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
    {
        CDo *newo = new CDo(ldesc(), &oBB());
        newo->prptyAddCopyList(prpty_list());
        if (!sdesc->insert(newo)) {
            delete newo;
            newo = 0;
        }
        return (newo);
    }
poly:
    {
        const CDpo *cdpo = (const CDpo*)this;
        CDpo *newp = new CDpo(ldesc());
        newp->e_BB = oBB();
        newp->set_numpts(cdpo->numpts());
        newp->set_points(Point::dup(cdpo->points(), newp->numpts()));
        newp->prptyAddCopyList(prpty_list());
        if (!sdesc->insert(newp)) {
            delete newp;
            newp = 0;
        }
        return (newp);
    }
wire:
    {
        const CDw *cdw = (const CDw*)this;
        CDw *neww = new CDw(ldesc());
        neww->e_BB = oBB();
        neww->set_attributes(cdw->attributes());
        neww->set_numpts(cdw->numpts());
        neww->set_points(Point::dup(cdw->points(), neww->numpts()));
        if (!sdesc->insert(neww)) {
            delete neww;
            return (0);
        }
        // Create any labels that reference this object, while copying
        // the properties.
        //
        for (CDp *pdesc = prpty_list(); pdesc; pdesc = pdesc->next_prp()) {
            if (prpty_reserved(pdesc->value()))
                continue;
            CDp *pnew = pdesc->dup();
            if (!pnew)
                continue;
            neww->link_prpty_list(pnew);
            if (!sdesc->isElectrical())
                continue;
            pnew->bind(0);
            CDla *olabel = pdesc->bound();
            if (olabel) {
                CDla *nlabel = (CDla*)olabel->dup(sdesc);
                pnew->bind(nlabel);
                // Just ignore duping a bad link.
                if (!nlabel->link(sdesc, neww, pnew))
                    Errs()->get_error();
            }
        }
        return (neww);
    }
label:
    {
        const CDla *cdla = (const CDla*)this;
        CDla *newla = new CDla(ldesc());
        newla->e_BB = oBB();
        newla->set_xpos(cdla->xpos());
        newla->set_ypos(cdla->ypos());
        newla->set_width(cdla->width());
        newla->set_height(cdla->height());
        newla->set_xform(cdla->xform());
        newla->set_label(hyList::dup(cdla->label()));
        newla->prptyAddCopyList(prpty_list());
        if (!sdesc->insert(newla)) {
            delete newla;
            newla = 0;
        }
        return (newla);
    }
inst:
    {
        const CDc *cdc = (const CDc*)this;
        CallDesc calldesc;
        cdc->call(&calldesc);

        // This function is used when duplicating cells from another
        // symbol table, in which case the cMaster->mSdesc is in the
        // originating symbol table.  Here, we substitute the cell from
        // the present symbol table.
        //
        calldesc.setCelldesc(CDcdb()->findCell(cdc->cellname(),
            sdesc->displayMode()));

        CDap ap(cdc);
        CDtx tx(cdc);
        CDc *newc;
        if (OIfailed(sdesc->makeCall(&calldesc, &tx, &ap, CDcallNone, &newc)))
            return (0);
        if (newc) {
            newc->prptyFreeList();

            // Create any labels that reference this object, while copying
            // the properties.
            //

            if (!sdesc->isElectrical()) {
                for (CDp *pdesc = prpty_list(); pdesc;
                        pdesc = pdesc->next_prp()) {
                    if (prpty_reserved(pdesc->value()))
                        continue;
                    CDp *pnew = pdesc->dup();
                    if (!pnew)
                        continue;
                    newc->link_prpty_list(pnew);
                }
            }
            else {
                // Two passes, the name property must be linked first.

                for (CDp *pdesc = prpty_list(); pdesc;
                        pdesc = pdesc->next_prp()) {
                    if (pdesc->value() != P_NAME)
                        continue;
                    if (prpty_reserved(pdesc->value()))
                        continue;
                    CDp *pnew = pdesc->dup();
                    if (!pnew)
                        continue;
                    newc->link_prpty_list(pnew);
                    pnew->bind(0);
                    CDla *olabel = pdesc->bound();
                    if (olabel) {
                        CDla *nlabel = (CDla*)olabel->dup(sdesc);
                        pnew->bind(nlabel);
                        // Just ignore duping a bad link.
                        if (!nlabel->link(sdesc, newc, pnew))
                            Errs()->get_error();
                    }
                }
                for (CDp *pdesc = prpty_list(); pdesc;
                        pdesc = pdesc->next_prp()) {
                    if (pdesc->value() == P_NAME)
                        continue;
                    if (prpty_reserved(pdesc->value()))
                        continue;
                    CDp *pnew = pdesc->dup();
                    if (!pnew)
                        continue;
                    newc->link_prpty_list(pnew);

                    if (pdesc->value() == P_NODE) {
                        CDp_cnode *pn = (CDp_cnode*)pnew;
                        if (pn->inst_terminal()) {
                            CDp_cnode *tcn = (CDp_cnode*)pdesc;
                            int ix = tcn->inst_terminal()->inst_index();
                            pn->inst_terminal()->set_instance(newc, ix);
                        }
                    }
                    pnew->bind(0);
                    CDla *olabel = pdesc->bound();
                    if (olabel) {
                        CDla *nlabel = (CDla*)olabel->dup(sdesc);
                        pnew->bind(nlabel);
                        // Just ignore duping a bad link.
                        if (!nlabel->link(sdesc, newc, pnew))
                            Errs()->get_error();
                    }
                }
            }
        }
        return (newc);
    }
}


// Copy the object, returning a local object with the "copy" flag
// set.  Such objects can not be inserted into the database.
//
// If bndry_mode is set, outlining objects will be returned for labels
// and instances, otherwise null is returned for these objects.
//
CDo *
CDo::copyObject(bool bndry_mode) const
{
    {
        const CDo *ot = this;
        if (!ot)
            return (0);
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
    {
        CDo *od = new CDo(ldesc());
        od->e_BB = oBB();
        od->set_state(state());
        od->set_flag(e_flags);
        od->set_copy(true);
        od->set_next_odesc(is_copy() ? const_next_odesc() : this);
        return (od);
    }
poly:
    {
        CDpo *od = new CDpo(ldesc());
        od->set_numpts(((const CDpo*)this)->numpts());
        od->set_points(Point::dup(((const CDpo*)this)->points(), od->numpts()));
        od->e_BB = oBB();
        od->set_state(state());
        od->set_flag(e_flags);
        od->set_copy(true);
        od->set_next_odesc(is_copy() ? const_next_odesc() : this);
        return (od);
    }
wire:
    {
        CDw *od = new CDw(ldesc());
        od->set_attributes(((const CDw*)this)->attributes());
        od->set_numpts(((const CDw*)this)->numpts());
        od->set_points(Point::dup(((const CDw*)this)->points(), od->numpts()));
        od->e_BB = oBB();
        od->set_state(state());
        od->set_flag(e_flags);
        od->set_copy(true);
        od->set_next_odesc(is_copy() ? const_next_odesc() : this);
        return (od);
    }
label:
inst:
    {
        if (!bndry_mode)
            return (0);
        BBox BB(oBB());
        Point *pts;
        boundary(&BB, &pts);
        CDo *od;
        if (pts) {
            od = new CDpo(ldesc());
            ((CDpo*)od)->set_numpts(5);
            ((CDpo*)od)->set_points(pts);
        }
        else
            od = new CDo(ldesc());
        od->e_BB = BB;
        od->set_state(state());
        od->set_flag(e_flags);
        od->set_copy(true);
        od->set_next_odesc(is_copy() ? const_next_odesc() : this);
        return (od);
    }
    return (0);
}


// Copy the object, applying the transformation, returning a local
// object with the "copy" flag set.  Such objects can not be inserted
// into the database.
//
// If bndry_mode is set, outlining objects will be returned for labels
// and instances, otherwise null is returned for these objects.
//
CDo *
CDo::copyObjectWithXform(const cTfmStack *tstk, bool bndry_mode) const
{
    {
        const CDo *ot = this;
        if (!ot)
            return (0);
    }

    BBox BB(oBB());
    Point *pts = 0;
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
    {
        if (tstk)
            tstk->TBB(&BB, &pts);
        CDo *od;
        if (pts) {
            // 45-rotated box, convert to poly
            od = new CDpo(ldesc());
            ((CDpo*)od)->set_numpts(5);
            ((CDpo*)od)->set_points(pts);
        }
        else
            od = new CDo(ldesc());
        od->e_BB = BB;
        od->set_state(state());
        od->set_flag(e_flags);
        od->set_copy(true);
        od->set_next_odesc(is_copy() ? const_next_odesc() : this);
        return (od);
    }
poly:
    {
        if (tstk)
            tstk->TBB(&BB, 0);
        CDpo *od = new CDpo(ldesc());
        od->set_numpts(((const CDpo*)this)->numpts());
        od->set_points(Point::dup_with_xform(((const CDpo*)this)->points(),
            tstk, od->numpts()));
        od->e_BB = BB;
        od->set_state(state());
        od->set_flag(e_flags);
        od->set_copy(true);
        od->set_next_odesc(is_copy() ? const_next_odesc() : this);
        return (od);
    }
wire:
    {
        int width = ((const CDw*)this)->wire_width();
        if (tstk) {
            tstk->TBB(&BB, 0);
            CDtf tf;
            tstk->TCurrent(&tf);
            if (tf.mag() > 0 && tf.mag() != 1.0)
                width = (int)(width * tf.mag());
        }
        CDw *od = new CDw(ldesc());
        od->set_wire_width(width);
        od->set_wire_style(((CDw*)this)->wire_style());
        od->set_numpts(((const CDw*)this)->numpts());
        od->set_points(Point::dup_with_xform(((const CDw*)this)->points(),
            tstk, od->numpts()));
        od->e_BB = BB;
        od->set_state(state());
        od->set_flag(e_flags);
        od->set_copy(true);
        od->set_next_odesc(is_copy() ? const_next_odesc() : this);
        return (od);
    }
label:
inst:
    {
        if (!bndry_mode)
            return (0);
        boundary(&BB, &pts);
        if (!pts) {
            pts = new Point[5];
            BB.to_path(pts);
        }
        if (tstk)
            tstk->TPath(5, pts);
        Poly po(5, pts);
        po.computeBB(&BB);
        CDo *od;
        if (!po.is_rect()) {
            od = new CDpo(ldesc());
            ((CDpo*)od)->set_numpts(5);
            ((CDpo*)od)->set_points(pts);
        }
        else {
            od = new CDo(ldesc());
            delete [] pts;
        }
        od->e_BB = BB;
        od->set_state(state());
        od->set_flag(e_flags);
        od->set_copy(true);
        od->set_next_odesc(is_copy() ? const_next_odesc() : this);
        return (od);
    }
    return (0);
}


// Function to find the boundary of an object.
//
void
CDo::boundary(BBox *BB, Point **pts) const
{
    {
        const CDo *ot = this;
        if (!ot) {
            if (pts)
                *pts = 0;
            return;
        }
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
poly:
wire:
    {
        *BB = oBB();
        if (pts)
            *pts = 0;
        return;
    }
label:
    {
        CDla *ladesc = (CDla*)this;
        if (ladesc->xform() & TXTF_45) {
            BBox tBB(ladesc->xpos(), ladesc->ypos(),
                ladesc->xpos() + ladesc->width(),
                ladesc->ypos() + ladesc->height());
            Label::TransformLabelBB(ladesc->xform(), &tBB, pts);
            *BB = tBB;
        }
        else {
            *BB = oBB();
            if (pts)
                *pts = 0;
        }
        return;
    }
inst:
    {
        CDc *cdesc = (CDc*)this;
        CDs *sd = cdesc->masterCell();
        if (!sd || (oBB().width() == 0 && oBB().height() == 0)) {
            // empty cell
            *pts = 0;
            *BB = oBB();
            return;
        }
        cTfmStack stk;
        stk.TPush();
        stk.TApplyTransform(cdesc);
        BBox tBB = *sd->BB();
        CDap ap(cdesc);
        if (ap.nx > 1) {
            if (ap.dx > 0)
                tBB.right += (ap.nx - 1)*ap.dx;
            else
                tBB.left += (ap.nx - 1)*ap.dx;
        }
        if (ap.ny > 1) {
            if (ap.dy > 0)
                tBB.top += (ap.ny - 1)*ap.dy;
            else
                tBB.bottom += (ap.ny - 1)*ap.dy;
        }
        stk.TBB(&tBB, pts);
        *BB = tBB;
        stk.TPop();
        return;
    }
}


char *
CDo::cif_string(int xo, int yo, bool use_wire_extension) const
{
    {
        const CDo *ot = this;
        if (!ot)
            return (0);
    }

    char buf[256];
    sLstr lstr;
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
    {
        buf[0] = 'B';
        buf[1] = ' ';
        char *s = mmItoA(buf + 2, oBB().width());
        *s++ = ' ';
        s = mmItoA(s, oBB().height());
        *s++ = ' ';
        s = mmItoA(s, (oBB().left + oBB().right)/2 + xo);
        *s++ = ' ';
        mmItoA(s, (oBB().bottom + oBB().top)/2 + yo);
        return (lstring::copy(buf));
    }
poly:
    {
        lstr.add("P");
        int len = lstr.length() + 2;
        for (int i = 0; i < ((CDpo*)this)->numpts(); i++) {
            buf[0] = ' ';
            char *s = mmItoA(buf + 1, ((CDpo*)this)->points()[i].x + xo);
            *s++ = ' ';
            mmItoA(s, ((CDpo*)this)->points()[i].y + yo);

            int len1 = strlen(buf);
            if (len + len1 < 79) {
                lstr.add(buf);
                len += len1;
            }
            else {
                lstr.add_c('\n');
                lstr.add(buf);
                len = len1;
            }
        }
        return (lstr.string_trim());
    }
wire:
    {
        lstr.add("W");
        if (use_wire_extension)
            lstr.add_i(((CDw*)this)->wire_style());
        lstr.add_c(' ');
        lstr.add_i(((CDw*)this)->wire_width());
        int len = lstr.length() + 2;
        for (int i = 0; i < ((CDw*)this)->numpts(); i++) {
            buf[0] = ' ';
            char *s = mmItoA(buf + 1, ((CDw*)this)->points()[i].x + xo);
            *s++ = ' ';
            mmItoA(s, ((CDw*)this)->points()[i].y + yo);

            int len1 = strlen(buf);
            if (len + len1 < 79) {
                lstr.add(buf);
                len += len1;
            }
            else {
                lstr.add_c('\n');
                lstr.add(buf);
                len = len1;
            }
        }
        return (lstr.string_trim());
    }
label:
    {
        // use long text for unbound labels
        CDp_lref *prf = (CDp_lref*)prpty(P_LABRF);
        char *ltxt;
        ((CDla*)this)->format_string(&ltxt, 0, ((CDla*)this)->label(),
            !(prf && prf->devref()), Fnative);
        lstr.add("94 ");
        lstr.add(ltxt);
        delete [] ltxt;

        lstr.add_c(' ');
        lstr.add_i(((CDla*)this)->xpos() + xo);
        lstr.add_c(' ');
        lstr.add_i(((CDla*)this)->ypos() + yo);
        lstr.add_c(' ');
        lstr.add_i(((CDla*)this)->xform());
        lstr.add_c(' ');
        lstr.add_i(((CDla*)this)->width());
        lstr.add_c(' ');
        lstr.add_i(((CDla*)this)->height());
        return (lstr.string_trim());
    }
inst:
    {
        // This is not standard CIF, used for comparison output
        CDc *c = (CDc*)this;
        lstr.add("C (");
        lstr.add(c->cellname()->string());
        CDattr at;
        if (!CD()->FindAttr(c->attr(), &at)) {
            lstr.add(" unresolved transform ticket");
            lstr.add_c(')');
        }
        else {
            if (at.magn != 1.0) {
                lstr.add(" Mag ");
                lstr.add_d(at.magn, 6);
            }
            if (at.nx > 1 || at.ny > 1) {
                lstr.add_c(' ');
                lstr.add_u(at.nx);
                lstr.add_c(',');
                lstr.add_u(at.ny);
                lstr.add_c(' ');
                lstr.add_i(at.dx);
                lstr.add_c(',');
                lstr.add_i(at.dy);
            }
            lstr.add_c(')');
            if (at.refly)
                lstr.add(" MY");
            if (at.ax != 1 || at.ay != 0) {
                lstr.add(" R ");
                lstr.add_i(at.ax);
                lstr.add_c(' ');
                lstr.add_i(at.ay);
            }
        }
        if (c->posX() || c->posY()) {
            lstr.add(" T ");
            lstr.add_i(c->posX());
            lstr.add_c(' ');
            lstr.add_i(c->posY());
        }
        return (lstr.string_trim());
    }
}


bool
CDo::prptyAdd(int value, const char *str, DisplayMode mode)
{
    {
        const CDo *ot = this;
        if (!ot)
            return (false);
    }

    if (!str)
        return (true);
    CDp *pdesc;
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(this)
#else
    CONDITIONAL_JUMP(this)
#endif
box:
poly:
    {
        if (mode == Physical) {
            if ((value == XICP_NODRC || value == OLD_NODRC_PROP) &&
                    !strcmp(str, "NODRC")) {
                set_flag(CDnoDRC);
                return (true);
            }
        }
        link_prpty_list(new CDp(str, value));
        return (true);
    }
wire:
    {
        if (mode == Physical) {
            if ((value == XICP_NODRC || value == OLD_NODRC_PROP) &&
                    !strcmp(str, "NODRC")) {
                set_flag(CDnoDRC);
                return (true);
            }
        }
        if (value == P_BNODE && mode == Electrical) {
            pdesc = new CDp_bnode;
            if (!((CDp_bnode*)pdesc)->parse_bnode(str)) {
                delete pdesc;
                return (false);
            }
        }
        else if (value == P_NODE && mode == Electrical) {
            pdesc = new CDp_node;
            if (!((CDp_node*)pdesc)->parse_node(str)) {
                delete pdesc;
                return (false);
            }
        }
        else
            pdesc = new CDp(str, value);
        link_prpty_list(pdesc);
        return (true);
    }
label:
    {
        if (value == P_LABRF && mode == Electrical) {
            pdesc = new CDp_lref;
            if (!((CDp_lref*)pdesc)->parse_lref(str)) {
                delete pdesc;
                return (false);
            }
        }
        else
            pdesc = new CDp(str, value);
        link_prpty_list(pdesc);
        return (true);
    }
inst:
    {
        if (mode == Electrical) {
            switch (value) {
            case P_MODEL:
            case P_VALUE:
            case P_PARAM:
            case P_OTHER:
            case P_DEVREF:
                pdesc = new CDp_user(((CDc*)this)->parent(), str, value);
                break;
            case P_RANGE:
                pdesc = new CDp_range;
                if (!((CDp_range*)pdesc)->parse_range(str)) {
                    delete pdesc;
                    return (false);
                }
                break;
            case P_BNODE:
                pdesc = new CDp_bcnode;
                if (!((CDp_bcnode*)pdesc)->parse_bcnode(str)) {
                    delete pdesc;
                    return (false);
                }
                break;
            case P_NODE:
                pdesc = new CDp_cnode;
                if (!((CDp_cnode*)pdesc)->parse_cnode(str)) {
                    delete pdesc;
                    return (false);
                }
                break;
            case P_NAME:
                pdesc = new CDp_name;
                if (!((CDp_name*)pdesc)->parse_name(str)) {
                    delete pdesc;
                    return (false);
                }
                break;
            case P_BRANCH:
                pdesc = new CDp_branch;
                if (!((CDp_branch*)pdesc)->parse_branch(str)) {
                    delete pdesc;
                    return (false);
                }
                break;
            case P_SYMBLC:
                pdesc = 0;
                if (str) {
                    const char *s = str;
                    while (isspace(*s))
                        s++;
                    // Ignore if not inactive setting.
                    if (*s == '0') {
                        CDp_sym *p = new CDp_sym;
                        p->set_active(false);
                        pdesc = p;
                    }
                }
                if (!pdesc)
                    return (true);
                break;

            default:
                pdesc = new CDp(str, value);
                break;
            }
        }
        else
            pdesc = new CDp(str, value);

        link_prpty_list(pdesc);
        return (true);
    }
}

