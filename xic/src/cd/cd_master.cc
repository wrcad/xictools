
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
#include "cd_propnum.h"
#include "cd_celldb.h"
#include "miscutil/pathlist.h"


CDm::~CDm()
{
    // name is in CellNameTable, don't delete!
    if (mObjRefs & 1)
        delete (ptable_t<CDc>*)(mObjRefs & ~1);
    if (mUnlinked & 1)
        delete (ptable_t<CDc>*)(mUnlinked & ~1);
}


// Delete all the instances, which has the effect of deleting the
// master desc itself.  However, if there are instances in the
// mObjRefs list and parent is null, the master desc is not deleted.
//
bool
CDm::clear()
{
    if (parent()) {
        if (parent()->isImmutable())
            // Dont touch this!
            return (false);
        // This will remove any references, and put the instance in
        // the mUnlinked list.
        CDc_gen cgen(this);
        for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
            if (!parent()->unlink(c, true))
                Errs()->get_error();
        }
    }
    CDc_gen cgen(this, true);
    for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
        unlinkCdesc(c, true);
        c->setMaster(0);    // don't free this
        delete c;
    }
    if (!hasInstances()) {
        unlink();  // unlink from parent
        if (celldesc())
            unlinkRef();
        delete this;
        return (true);
    }
    return (false);
}


void
CDm::linkCdesc(CDc *cd, bool unl_list)
{
    if (!cd)
        return;
    if (unl_list) {
        if (mUnlinked & 1) {
            ptable_t<CDc> *t = (ptable_t<CDc>*)(mUnlinked & ~1);
            t->add(cd);
            t = t->check_rehash();
            mUnlinked = (unsigned long)t | 1;
        }
        else {
            int cnt = 0;
            for (CDc *c = (CDc*)mUnlinked; c; c = c->ptab_next())
                cnt++;
            if (cnt < ST_MAX_DENS) {
                cd->set_ptab_next((CDc*)mUnlinked);
                mUnlinked = (unsigned long)cd;
            }
            else {
                ptable_t<CDc> *t = new ptable_t<CDc>;
                CDc *c = (CDc*)mUnlinked;
                while (c) {
                    CDc *cx = c;
                    c = c->ptab_next();
                    cx->set_ptab_next(0);
                    t->add(cx);
                }
                t->add(cd);
                t = t->check_rehash();
                mUnlinked = (unsigned long)t | 1;
            }
        }
    }
    else {
        if (mObjRefs & 1) {
            ptable_t<CDc> *t = (ptable_t<CDc>*)(mObjRefs & ~1);
            t->add(cd);
            t = t->check_rehash();
            mObjRefs = (unsigned long)t | 1;
        }
        else {
            int cnt = 0;
            for (CDc *c = (CDc*)mObjRefs; c; c = c->ptab_next())
                cnt++;
            if (cnt < ST_MAX_DENS) {
                cd->set_ptab_next((CDc*)mObjRefs);
                mObjRefs = (unsigned long)cd;
            }
            else {
                ptable_t<CDc> *t = new ptable_t<CDc>;
                CDc *c = (CDc*)mObjRefs;
                while (c) {
                    CDc *cx = c;
                    c = c->ptab_next();
                    cx->set_ptab_next(0);
                    t->add(cx);
                }
                t->add(cd);
                t = t->check_rehash();
                mObjRefs = (unsigned long)t | 1;
            }
        }
    }
}


void
CDm::unlinkCdesc(CDc *cd, bool unl_list)
{
    if (!cd)
        return;
    if (unl_list) {
        if (mUnlinked & 1)
            ((ptable_t<CDc>*)(mUnlinked & ~1))->remove(cd);
        else {
            CDc *cp = 0;
            for (CDc *c = (CDc*)mUnlinked; c; c = c->ptab_next()) {
                if (c == cd) {
                    if (!cp)
                        mUnlinked = (unsigned long)c->ptab_next();
                    else
                        cp->set_ptab_next(c->ptab_next());
                    cd->set_ptab_next(0);
                    break;
                }
                cp = c;
            }
        }
    }
    else {
        if (mObjRefs & 1)
            ((ptable_t<CDc>*)(mObjRefs & ~1))->remove(cd);
        else {
            CDc *cp = 0;
            for (CDc *c = (CDc*)mObjRefs; c; c = c->ptab_next()) {
                if (c == cd) {
                    if (!cp)
                        mObjRefs = (unsigned long)c->ptab_next();
                    else
                        cp->set_ptab_next(c->ptab_next());
                    cd->set_ptab_next(0);
                    break;
                }
                cp = c;
            }
        }
    }
}


// Update the object descriptor BB's, and reinsert object descriptors into
// appropriate bins.
//
bool
CDm::reflect()
{
    CDc_gen cgen(this);
    for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
        if (!cdesc->updateBB())
            return (false);
    }
    return (true);
}


int
CDm::listInstance(stringnumlist **l, bool expand)
{
    int count = 0;
    CDc_gen cgen(this);
    for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
        if (expand) {
            if (mSdesc && mSdesc->isElectrical()) {
                CDp_range *pr = (CDp_range*)c->prpty(P_RANGE);
                count += pr ? pr->width() : 1;
            }
            else {
                CDap ap(c);
                count += ap.nx*ap.ny;
            }
        }
        else
            count++;
    }
    if (l)
        *l = new stringnumlist(lstring::copy(mName), count, *l);
    return (count);
}


// Set the mSdesc pointer if it is null.
//
void
CDm::setSdesc()
{
    if (!mSdesc && mParent) {
        DisplayMode mode = mParent->displayMode();
        if (mode == Physical ||
                (mode == Electrical && !CD()->IsNoElectrical())) {
            CDs *sd = 0;
            CDcbin cbin;
            if (OIfailed(CD()->OpenExisting(mName, &cbin))) {
                // silently ignore error
                Errs()->get_error();
                sd = CDcdb()->insertCell(mName, mode);
            }
            else {
                sd = cbin.celldesc(mode);
                if (!sd)
                    sd = CDcdb()->insertCell(mName, mode);
            }
            linkRef(sd);
        }
    }
}


void
CDm::setParent(CDs *s)
{
    if (s && s->isSymbolic()) {
        s = s->owner();
        CD()->DbgError("symbolic", "CDm::setParent");
    }
    mParent = s;
}


void
CDm::setCelldesc(CDs *s)
{
    if (s && s->isSymbolic()) {
        s = s->owner();
        CD()->DbgError("symbolic", "CDm::setCelldesc");
    }
    mSdesc = s;
}

