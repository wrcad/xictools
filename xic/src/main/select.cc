
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
#include "editif.h"
#include "drcif.h"
#include "extif.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_layer.h"
#include "cd_propnum.h"
#include "geo_ylist.h"
#include "si_parsenode.h"
#include "si_lexpr.h"
#include "events.h"
#include "layertab.h"
#include "menu.h"
#include "promptline.h"
#include "tech.h"
#include "errorlog.h"
#include "select.h"
#include "miscutil/pathlist.h"
#include "miscutil/texttf.h"
#include <algorithm>

#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#include "regex/regex.h"
#endif

#ifdef HAVE_SECURE
#include <signal.h>
#include "miscutil/miscutil.h"
extern int StateInitialized;  // Part of the security system
#endif


unsigned int cSelections::blink_thresh = DEF_MAX_BLINKING_OBJECTS;

// If fewer than this to deselect, do them individually, otherwise
// redraw enclosing area.
#define DEL_AGGREG 8


//-----------------------------------------------------------------------------
// Menu command to deselect everything.  There is provision for registering
// an alternative handler, used by the Properties Editor.

namespace { void(*desel_override)(CmdDesc*); }

void
cMain::RegisterDeselOverride(void(*f)(CmdDesc*))
{
    desel_override = f;
}

// Menu Command to deselect everything.
//
void
cMain::DeselectExec(CmdDesc *cmd)
{
    if (desel_override)
        (*desel_override)(cmd);
    else {
        Selections.deselectAll();
        ExtIf()->deselect();
        XM()->ShowParameters();
        PopUpSelectInstances(0);
    }
    if (EV()->CurCmd())
        EV()->CurCmd()->desel();
}
// End of cMain functions.


//-----------------------------------------------------------------------------
// Object selection code.

namespace {
    // Return true if the object type matches the type string.
    //
    inline bool match_type(const char *t, CDo *od)
    {
        if (od)
            return (!t || !*t || strchr(t, od->type()));
        return (false);
    }

    // Return true if BB is small enough to be considered as a
    // point.
    //
    bool is_pointselect(const BBox *BB, int *delta)
    {
        WindowDesc *wdesc = EV()->ButtonWin(true);
        if (!wdesc)
            wdesc = DSP()->MainWdesc();

        int dlt = 1 + (int)(DSP()->PixelDelta()/wdesc->Ratio());
        if (delta)
            *delta = dlt;
        int width = BB->width();
        int height = BB->height();
        if (width < 0)
            width = -width;
        if (height < 0)
            height = -height;
        if (width <= dlt && height <= dlt)
            return (true);
        return (false);
    }
}


// The Selection code extensively uses the CD State field.  The following
// convention is used:
//
//   State = CDobjVanilla      Object is unselected.
//  *State = CDobjSelected     Object is selected.
//   State = CDobjDeleted      Object is conditionally deleted.
//   State = CDobjIncomplete   Object is being created.
//   State = CDobjInternal     Object is internal, not selectable.
//
//  * means that redisplay will highlight these objects.


// Select items of types in AOI and link into selection queue.  Set
// the Info field and redisplay the objects.
//
// If this function is being called in a noninteractive situation,
// such as from a script, the argument strict should be true.  In this
// case, the keyboard is ignored, and strict area testing is used. 
// The is_poinselect() call uses the last button1-press window in
// non-strict mode.
//
// If list is passed, it replaces the call to selectItems.
// ** IT IS CONSUMED **!!
//
// True is returned if an enabled object was clicked on.
//
bool
cSelections::selection(const CDs *sd, const char *types, const BBox *AOI,
    bool strict, CDol *list)
{
    if (!sd)
        return (false);
    if (!types)
        types = s_types;

    SELmode addmode = s_sel_mode;
    bool iterate_mode = false;
    CDol *sel_list = 0;
    CDol *unsel_list = 0;
    {
        if (!list) {
            list = selectItems(sd, types, AOI,
                strict ? PSELstrict_area : PSELpoint, true);
        }
        if (!list)
            return (false);

        if (!strict) {
            iterate_mode = !CDvdb()->getVariable(VA_NoAltSelection);
            list = filter(sd, list, AOI, iterate_mode);
            if (!list)
                return (false);
        }

        if (!strict && addmode == SELnormal) {
            unsigned int modif = 0;
            DSPmainDraw(QueryPointer(0, 0, &modif))
            if ((modif & GR_SHIFT_MASK) && (modif & GR_CONTROL_MASK))
                addmode = SELtoggle;
            else if (modif & GR_SHIFT_MASK)
                addmode = SELselect;
            else if (modif & GR_CONTROL_MASK)
                addmode = SELdesel;
        }

#ifdef HAVE_SECURE
        // Below is a booby trap in case the call to Validate() is patched
        // over.  This is part of the security system.
        if (!StateInitialized) {
            char *uname = pathlist::get_user_name(false);
            char tbuf[256];

            snprintf(tbuf, sizeof(tbuf), "xic: select %s\n", uname);
            miscutil::send_mail(Log()->MailAddress(), "SecurityReport:App1",
                tbuf);
            delete [] uname;
            raise(SIGTERM);
        }
#endif

        // Count selections and remove internal objects.
        int nsel = 0;
        int nusel = 0;
        CDol *cp = 0, *cn;
        for (CDol *c = list; c; c = cn) {
            cn = c->next;
            if (!c->odesc->is_normal()) {
                if (cp)
                    cp->next = cn;
                else
                    list = cn;
                delete c;
                continue;
            }
            cp = c;
            if (c->odesc->state() == CDobjSelected)
                nsel++;
            else
                nusel++;
        }

        // If only physical instances are in list (instances are
        // listed last) and there are 3 or more, use a pop-up to
        // control the selections.
        //
        if (!sd->isElectrical()) {
            bool instonly = (list->odesc->type() == CDINSTANCE);
            if (instonly && (nsel+nusel >= 3)) {
                XM()->PopUpSelectInstances(list);
                CDol::destroy(list);
                return (true);
            }
        }

        if (iterate_mode && is_pointselect(AOI, 0)) {
            // Keep at most one selected and unselected item.  The
            // unselected item is first in the list, or first
            // following the selected item.

            if (nsel > 1) {
                // Keep first selected only.
                while (list) {
                    if (list->odesc->state() == CDobjSelected) {
                        CDol::destroy(list->next);
                        list->next = 0;
                        sel_list = list;
                        break;
                    }
                    CDol *ct = list;
                    list = list->next;
                    delete ct;
                }
            }
            else if (nsel == 1) {
                // Keep selected and first one following only.
                while (list) {
                    if (list->odesc->state() == CDobjSelected) {
                        if (list->next) {
                            CDol::destroy(list->next->next);
                            list->next->next = 0;
                            unsel_list = list->next;
                        }
                        list->next = 0;
                        sel_list = list;
                        break;
                    }
                    CDol *ct = list;
                    list = list->next;
                    delete ct;
                }
            }
            else {
                // Keep first one only.
                CDol::destroy(list->next);
                list->next = 0;
                unsel_list = list;
            }
        }
        else {
            CDol *cnx, *se = 0, *ue = 0;
            for (CDol *c = list; c; c = cnx) {
                cnx = c->next;
                c->next = 0;
                if (c->odesc->is_normal()) {
                    if (c->odesc->state() == CDobjSelected) {
                        if (!se)
                            sel_list = se = c;
                        else {
                            se->next = c;
                            se = c;
                        }
                    }
                    else {
                        if (!ue)
                            unsel_list = ue = c;
                        else {
                            ue->next = c;
                            ue = c;
                        }
                    }
                }
                else
                    delete c;
            }
        }
    }

    selqueue_t *sq = findQueue(sd, true);
    if (!sq) {
        CDol::destroy(sel_list);
        CDol::destroy(unsel_list);
        return (false);
    }

    if (addmode == SELselect) {
        sq->insert_and_show(unsel_list);
        CDol::destroy(unsel_list);
        unsel_list = 0;
        CDol::destroy(sel_list);
        sel_list = 0;
    }
    else if (addmode == SELdesel) {
    }
    else if (addmode == SELtoggle) {
        sq->insert_and_show(unsel_list);
        CDol::destroy(unsel_list);
        unsel_list = 0;
    }
    else {
        if (sel_list && !sel_list->next && !unsel_list &&
                sel_list->odesc->type() == CDINSTANCE) {
            selqueue_t::show_unselected(sd, sel_list->odesc);
            sq->remove_object(sel_list->odesc);
            CDol::destroy(sel_list);
            return (true);
        }
        if (unsel_list && !unsel_list->next && !sel_list &&
                unsel_list->odesc->type() == CDINSTANCE) {
            if (!XM()->IsBoundaryVisible(sd, unsel_list->odesc)) {
                // get here in strict mode only

                bool cells_only = (types && *types ==
                    CDINSTANCE && *(types+1) == '\0');
                if (!cells_only) {
                    CDol::destroy(unsel_list);
                    return (true);
                }
                else {
                    const char *sym_name =
                        Tstring(OCALL(unsel_list->odesc)->cellname());
                    PL()->ShowPromptV(
                        "You have selected an instance of %s.",
                        sym_name);
                }
            }
            sq->insert_object(unsel_list->odesc, SQinsShow);
            CDol::destroy(unsel_list);
            return (true);
        }
        int selcnt = 0;
        if (!strict) {
            if (is_pointselect(AOI, 0)) {
                // count entries already selected
                for (CDol *c = sel_list; c; c = c->next)
                    selcnt++;
            }
        }
        if (selcnt > 1) {
            if (sel_list) {
                CDol::destroy(sel_list->next);
                sel_list->next = 0;
            }
        }
        else {
            sq->insert_and_show(unsel_list);
            CDol::destroy(unsel_list);
            unsel_list = 0;
        }
    }

    if (sel_list) {
        for (CDol *c = sel_list; c; c = c->next)
            sq->remove_object(c->odesc);
        selqueue_t::redisplay_list(sd, sel_list);
        CDol::destroy(sel_list);
    }
    sq->count_queue(&s_display_count, 0);

    CDol::destroy(unsel_list);
    return (true);
}


// Return a list of selectable objects in the neighborhood of AOI as
// returned from the generator.  Only types in argument types are
// returned, all are returned if this is 0.  If psel is PSELpoint,
// objects are selected according to the current area_mode.
// Otherwise, the BB is taken as absolute, and all objects which touch
// BB are returned.
//
// PSELpoint should not be passed unless AOI was derived from button
// operations in a window, since the last button1-press window is used
// in is_pointselect().
//
// Objects are in database order, with instances last.
//
CDol *
cSelections::selectItems(const CDs *sd, const char *types, const BBox *AOI,
    PSELmode psel, bool nopopup)
{
    if (!sd)
        return (0);
    ASELmode asel = (psel == PSELpoint ? s_area_mode : ASELall);
    int delta = 0;
    BBox BB = *AOI;
    BB.fix();

    if (psel == PSELpoint) {
        if (is_pointselect(&BB, &delta)) {
            BB.right = BB.left;
            BB.top = BB.bottom;
            BB.bloat(delta);
        }
        else
            psel = PSELstrict_area;
    }

    if (sd->isElectrical()) {
        WindowDesc *wdesc = EV()->ButtonWin(true);
        if (!wdesc)
            wdesc = DSP()->MainWdesc();
        const CDc *top_cdesc = 0;
        if (wdesc->Attrib()->no_elec_symbolic() &&
                wdesc->CurCellName() == wdesc->TopCellName())
            top_cdesc = CD_NO_SYMBOLIC;
        else if (wdesc->CurCellName() != wdesc->TopCellName() &&
                DSP()->MainWdesc()->Mode() == Electrical &&
                DSP()->MainWdesc()->CurCellName() == wdesc->CurCellName())
            top_cdesc = DSP()->context_cell();
        CDs *rep = sd->symbolicRep(top_cdesc);
        if (rep)
            sd = rep;
    }
    DisplayMode mode = sd->displayMode();

    CDol *c0 = 0, *ce = 0;
    if (mode == Physical && (ExtIf()->isExtractionView() ||
            ExtIf()->isExtractionSelect())) {
        // Special handling when in Extraction View, and/or extraction
        // temporary objects are included, as when selecting paths.

        c0 = ExtIf()->selectItems(types, &BB, psel, asel, delta);
        if (c0) {
            ce = c0;
            while (ce->next)
                ce = ce->next;
        }
    }
    else {

        CDlgen lgen(mode, mode == Physical && !s_search_up ?
            CDlgen::TopToBotNoCells : CDlgen::BotToTopNoCells);

        CDl *ldesc;
        while ((ldesc = lgen.next()) != 0) {
            if (!ldesc->isSelectable())
                continue;

            CDg gdesc;
            gdesc.init_gen(sd, ldesc, &BB);
            if (psel == PSELstrict_area_nt)
                gdesc.setflags(GEN_RET_NOTOUCH);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                if (!odesc->is_normal())
                    continue;
                if (!match_type(types, odesc))
                    continue;
                if (processSelect(odesc, &BB, psel, asel, delta)) {
                    if (c0 == 0)
                        c0 = ce = new CDol(odesc, 0);
                    else {
                        ce->next = new CDol(odesc, 0);
                        ce = ce->next;
                    }
                }
            }
        }
    }

    // Now for the instances...

    if (types && *types && !strchr(types, CDINSTANCE))
        return (c0);

    CDg gdesc;
    gdesc.init_gen(sd, CellLayer(), &BB);
    if (psel == PSELstrict_area_nt)
        gdesc.setflags(GEN_RET_NOTOUCH);
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        if (processSelect(cdesc, &BB, psel, asel, 0)) {
            if (!c0)
                c0 = ce = new CDol(cdesc, 0);
            else {
                ce->next = new CDol(cdesc, 0);
                ce = ce->next;
            }
        }
    }
    if (!nopopup && !sd->isElectrical() && c0 &&
            c0->odesc->type() == CDINSTANCE && c0->next && c0->next->next) {
        c0 = XM()->PopUpFilterInstances(c0);
    }
    return (c0);
}


namespace {
    // Return the CDol with the smallest bounding box area.
    //
    CDol *
    smallest_bb(CDol *list)
    {
        // find the smallest cell
        BBox BB;
        CDol::computeBB(list, &BB);
        double Area = BB.width();
        Area *= BB.height();
        CDol *cret = list;
        for (CDol *c = list; c; c = c->next) {
            double A = c->odesc->oBB().width();
            A *= c->odesc->oBB().height();
            if (A > 0.0 && A < Area) {
                cret = c;
                Area = A;
            }
        }
        return (cret);
    }


    // Return true if the object covers all bounding boxes in the list.
    //
    bool
    is_covering_obj(CDo *o, CDol *olist)
    {
        for (CDol *ol = olist; ol; ol = ol->next) {
            const BBox *tbb = &ol->odesc->oBB();

            if ((o->oBB().left <= tbb->left &&
                    o->oBB().bottom <= tbb->bottom &&
                    o->oBB().right >= tbb->right &&
                    o->oBB().top >= tbb->top) &&
                    (o->oBB().left != tbb->left ||
                    o->oBB().bottom != tbb->bottom ||
                    o->oBB().right != tbb->right ||
                    o->oBB().top != tbb->top)) {

                if (o->type() == CDPOLYGON || o->type() == CDWIRE) {
                    Zlist *zl0 = o->toZlist();
                    if (!zl0)
                        return (false);
                    Zoid Z(tbb);
                    bool cover;
                    bool cov = Z.test_coverage(zl0, &cover, 0);
                    Zlist::destroy(zl0);
                    if (!cov)
                        return (false);
                }
            }
            else
                return (false);
        }
        return (true);
    }


    bool ocmp(const CDo *od1, const CDo *od2)
    {
        return (od1->oBB().area() < od2->oBB().area());
    }
}


// Filter the list of selected items, removing objects which should
// probably not be included.  If iterate_mode is true, use existing
// selections to allow iteration through candidate objects (physical
// mode only).
//
CDol *
cSelections::filter(const CDs *sd, CDol *list, const BBox *AOI,
    bool iterate_mode)
{
    if (!sd || !list)
        return (0);

    // Throw out objects that are not selected and the boundary is not
    // visible.
    {
        CDol *cp = 0, *cn;
        for (CDol *c = list; c; c = cn) {
            cn = c->next;
            if (c->odesc->state() != CDobjSelected &&
                    !XM()->IsBoundaryVisible(sd, c->odesc)) {
                if (!cp)
                    list = cn;
                else
                    cp->next = cn;
                delete c;
                continue;
            }
            cp = c;
        }
    }

    if (!is_pointselect(AOI, 0))
        return (list);

    // Sort the list of objects into a priority order, with the highest
    // priority objects first.  If editing in electrical mode, return the
    // wires, labels, instances, boxes only, if the higher priority
    // objects are not found.
    //
    // Within object classes, preserve database order.

    if (sd->isElectrical()) {
        // Electrical mode
        CDol *c0 = 0, *ce = 0;
        CDol *cp = 0, *cn;
        // check for polygons
        for (CDol *c = list; c; c = cn) {
            cn = c->next;
            if (c->odesc->type() == CDPOLYGON) {
                // shouldn't be any polygons, except perhaps in device
                // representations.
                //
                if (!cp)
                    list = cn;
                else
                    cp->next = cn;
                c->next = 0;
                if (!c0)
                    c0 = ce = c;
                else {
                    ce->next = c;
                    ce = c;
                }
                continue;
            }
            cp = c;
        }

        // check for wires
        if (!c0) {
            cp = 0;
            for (CDol *c = list; c; c = cn) {
                cn = c->next;
                if (c->odesc->type() == CDWIRE) {
                    if (!cp)
                        list = cn;
                    else
                        cp->next = cn;
                    c->next = 0;
                    if (!c0)
                        c0 = ce = c;
                    else {
                        ce->next = c;
                        ce = c;
                    }
                    continue;
                }
                cp = c;
            }
        }

        // check for labels
        if (!c0) {
            cp = 0;
            for (CDol *c = list; c; c = cn) {
                cn = c->next;
                if (c->odesc->type() == CDLABEL) {
                    if (!cp)
                        list = cn;
                    else
                        cp->next = cn;
                    c->next = 0;
                    if (!c0)
                        c0 = ce = c;
                    else {
                        ce->next = c;
                        ce = c;
                    }
                    continue;
                }
                cp = c;
            }
        }

        // check for instances
        if (!c0) {
            cp = 0;
            for (CDol *c = list; c; c = cn) {
                cn = c->next;
                if (c->odesc->type() == CDINSTANCE) {
                    if (!cp)
                        list = cn;
                    else
                        cp->next = cn;
                    c->next = 0;
                    if (!c0)
                        c0 = ce = c;
                    else {
                        ce->next = c;
                        ce = c;
                    }
                    continue;
                }
                cp = c;
            }
        }

        if (c0) {
            CDol::destroy(list);
            list = c0;
        }
        // else list contains only boxes
    }
    else {
        // Physical

        if (iterate_mode) {
            // New:  treat all objects the same, sort by increasing
            // area so smaller objects have priority.

            int cnt = 0;
            for (CDol *ol = list; ol; ol = ol->next, cnt++) ;
            if (cnt > 1) {
                CDo **ary = new CDo*[cnt];
                cnt = 0;
                for (CDol *ol = list; ol; ol = ol->next)
                    ary[cnt++] = ol->odesc;
                std::sort(ary, ary + cnt, ocmp);

                // Throw out object with BB that encloses smaller
                // objects.
                //
                BBox BB(ary[0]->oBB());
                for (int i = 1; i < cnt; i++) {
                    if (BB < ary[i]->oBB()) {
                        cnt = i;
                        break;
                    }
                    BB.add(&ary[i]->oBB());
                }
                int i = 0;
                for (CDol *ol = list; ol; ol = ol->next) {
                    ol->odesc = ary[i++];
                    if (i == cnt) {
                        CDol::destroy(ol->next);
                        ol->next = 0;
                        break;
                    }
                }
                delete [] ary;
            }
            return (list);
        }

        // Older algorithm.
        // The order is:
        //   geometry first
        //   subcell(s)
        //   geometry that covers subcells
        //

        // Pull out the instances, keep the original ordering.
        CDol *c0 = 0;
        CDol *cp = 0, *cn;
        int nsel = 0;
        {
            CDol *ce = 0;
            for (CDol *c = list; c; c = cn) {
                cn = c->next;
                if (c->odesc->type() == CDINSTANCE) {
                    if (c->odesc->state() == CDobjSelected)
                        nsel++;
                    if (!cp)
                        list = cn;
                    else
                        cp->next = cn;
                    c->next = 0;
                    if (!c0)
                        c0 = ce = c;
                    else {
                        ce->next = c;
                        ce = c;
                    }
                    continue;
                }
                cp = c;
            }
        }

        if (c0) {
            if (iterate_mode) {
                if (nsel > 1) {
                    // Keep first selected only.
                    while (c0) {
                        if (c0->odesc->state() == CDobjSelected) {
                            CDol::destroy(c0->next);
                            c0->next = 0;
                            break;
                        }
                        CDol *ct = c0;
                        c0 = c0->next;
                        delete ct;
                    }
                }
                else if (nsel == 1) {
                    // Keep selected and first one following only.
                    while (c0) {
                        if (c0->odesc->state() == CDobjSelected) {
                            if (c0->next) {
                                CDol::destroy(c0->next->next);
                                c0->next->next = 0;
                            }
                            break;
                        }
                        CDol *ct = c0;
                        c0 = c0->next;
                        delete ct;
                    }
                }
                else {
                    // Keep first one only.
                    CDol::destroy(c0->next);
                    c0->next = 0;
                }
            }
            else {
                CDol *c = smallest_bb(c0);
                if (!c) {
                    CDol::destroy(c0);
                    c0 = 0;
                }
                else {
                    c0->odesc = c->odesc;
                    CDol::destroy(c0->next);
                    c0->next = 0;
                }
            }

            // Move objects that completely cover the subcells to the
            // end of the list.
            CDol *e0 = 0, *ce = 0;
            cp = 0;
            for (CDol *c = list; c; c = cn) {
                cn = c->next;
                if (is_covering_obj(c->odesc, c0)) {
                    if (!cp)
                        list = cn;
                    else
                        cp->next = cn;
                    c->next = 0;
                    if (!e0)
                        e0 = ce = c;
                    else {
                        ce->next = c;
                        ce = c;
                    }
                    continue;
                }
                cp = c;
            }
            if (e0) {
                ce = c0;
                while (ce->next)
                    ce = ce->next;
                ce->next = e0;
            }

            if (!list) {
                list = c0;
                if (!iterate_mode) {
                    // No leading geometry, save the instance only.
                    CDol::destroy(list->next);
                    list->next = 0;
                }
            }
            else {
                if (!iterate_mode) {
                    // Keep only the geometry ahead of instances.
                    CDol::destroy(c0);
                }
                else {
                    ce = list;
                    while (ce->next)
                        ce = ce->next;
                    ce->next = c0;
                }
            }
        }
    }

    return (list);
}


// Return a list of objects, that match types, selecected only if all
// is false.  The list should be freed by the caller.
//
CDol *
cSelections::listQueue(const CDs *sd, const char *types, bool all)
{
    CDol *ol0 = 0, *oe = 0;
    sSelGen sg(*this, sd, types, all);
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (!ol0)
            ol0 = oe = new CDol(od, 0);
        else {
            oe->next = new CDol(od, 0);
            oe = oe->next;
        }
    }
    return (ol0);
}


// Show selected objects in selection queue by highlighting.
//
void
cSelections::show(WindowDesc *wdesc)
{
    if (DSP()->NoRedisplay())
        return;
    if (!wdesc || !wdesc->Wdraw())
        return;
    CDs *sd = wdesc->CurCellDesc(wdesc->Mode());
    if (!sd)
        return;
    selqueue_t *sq = findQueue(sd, false);
    if (!sq)
        return;

    // This is called by the color timer in high-color modes, so prevent
    // reentrancy.
    static bool lockout;
    if (lockout)
        return;
    lockout = true;

    if (wdesc == DSP()->MainWdesc())
        s_display_count = 0;
    s_display_count += sq->show(wdesc);

    lockout = false;
}


// Check the queues for consistency.
//
void
cSelections::check()
{
    for (int i = 0; i < SEL_NUMQUEUES; i++) {
        selqueue_t *sq = s_queues + i;
        if (sq->has_types(0, true) && !sq->celldesc()) {
            // Orphaned queue, should never happen.
            sq->remove_types(0);
            if (XM()->DebugFlags() & DBG_SELECT)
                Log()->ErrorLogV("selections",
                    "Selection list with null cell pointer deleted.");
            continue;
        }
        if (sq->celldesc()) {
            // Looks for non-empty lists for cells not shown in any
            // window.
            bool found = false;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            WindowDesc *wdesc;
            while ((wdesc = wgen.next()) != 0) {
                if (wdesc->IsShowing(sq->celldesc())) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                // Cell is not being shown in a drawing window.  If
                // there are queue entries, this is an error.

                unsigned int s, n;
                sq->count_queue(&s, &n);
                n += s;
                if (n) {
                    if (XM()->DebugFlags() & DBG_SELECT)
                        Log()->ErrorLogV("selections",
                            "Selection list for nondisplayed %s cell %s with "
                            "length %d deleted.",
                            DisplayModeNameLC(sq->celldesc()->displayMode()),
                            Tstring(sq->celldesc()->cellname()), n);
                    sq->remove_types(0);
                }
            }
            else
                sq->check();
        }
    }
}


//
// The processing for the !select and !deselect commands.
//

void
cSelections::parseSelections(const CDs *sd, const char *string, bool select)
{
    if (!sd)
        return;

    // !select qualifier names
    // qualifier
    // layer, cell, name, model, value, param, other
    // l,     c,    n,    m,     v,     p|i,   o
    const char *s = string;
    if (!*s || !strcmp(s, "all") || !strcmp(s, ".")) {
        // select/deselect:
        //  no arg or ".", everything
        //  "all", all geometry

        if (!*s && !select) {
            // do a complete deselection
            XM()->DeselectExec(0);
            return;
        }

        CDol *s0 = 0;
        lmatch(sd, s, select, &s0);
        if (strcmp(s, "all"))
            cmatch(sd, s, select, &s0);
        if (s0) {
            selqueue_t::redisplay_list(sd, s0);
            CDol::destroy(s0);
        }
        return;
    }
    while (isspace(*s))
       s++;
    char key = *s;
    while (*s && !isspace(*s))
       s++;
    while (isspace(*s))
       s++;
    // *s = 0 implies "all"

    if (isupper(key))
        key = tolower(key);
    switch (key) {
    case 'l':
        lmatch(sd, s, select);
        break;
    case 'c':
        cmatch(sd, s, select);
        break;
    case 'n':
        pmatch(sd, P_NAME, s, select);
        break;
    case 'm':
        pmatch(sd, P_MODEL, s, select);
        break;
    case 'v':
        pmatch(sd, P_VALUE, s, select);
        break;
    case 'i':
    case 'p':
        pmatch(sd, P_PARAM, s, select);
        break;
    case 'o':
        pmatch(sd, P_OTHER, s, select);
        break;
    case 'y':
        pmatch(sd, P_NOPHYS, s, select);
        break;
    case 'f':
        pmatch(sd, P_FLATTEN, s, select);
        break;
    case 'd':
        pmatch(sd, P_DEVREF, s, select);
        break;
    }
}


// Static function
// Perform initial selection filtering.  Return true if odesc is a candidate
// for selection.
//
bool
cSelections::processSelect(CDo *odesc, const BBox *BB, PSELmode psel,
    ASELmode asel, int delta)
{
    if (!odesc)
        return (false);
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
    {
        if (psel != PSELpoint && asel != ASELall) {
            if (odesc->oBB().left >= BB->left &&
                    odesc->oBB().right <= BB->right &&
                    odesc->oBB().bottom >= BB->bottom &&
                    odesc->oBB().top <= BB->top)
                return (true);
            if (asel == ASELenclosed)
                return (false);
            if (odesc->oBB().left < BB->left &&
                    odesc->oBB().right > BB->right &&
                    odesc->oBB().bottom < BB->bottom &&
                    odesc->oBB().top > BB->top)
                return (false);
        }
        return (true);
    }
poly:
    {
        if (psel == PSELpoint)
            return (((const CDpo*)odesc)->po_intersect(BB, true));
        if (odesc->oBB().left >= BB->left &&
                odesc->oBB().right <= BB->right &&
                odesc->oBB().bottom >= BB->bottom &&
                odesc->oBB().top <= BB->top)
            return (true);
        if (asel == ASELenclosed)
            return (false);
        int num = ((const CDpo*)odesc)->numpts();
        const Point *pts = ((const CDpo*)odesc)->points();
        if (cGEO::path_box_intersect(pts, num, BB, true))
            return (true);
        if (asel == ASELall) {
            Point_c p(BB->left, BB->bottom);
            return (((const CDpo*)odesc)->po_intersect(&p, true));
        }
        return (false);
    }
wire:
    {
        if (((CDw*)odesc)->wire_width() > 0) {
            Poly po;
            if (!((CDw*)odesc)->w_toPoly(&po.points, &po.numpts))
                return (false);
            GCarray<Point*> gc_points(po.points);

            if (psel == PSELpoint)
                return (po.intersect(BB, true));
            if (odesc->oBB().left >= BB->left &&
                    odesc->oBB().right <= BB->right &&
                    odesc->oBB().bottom >= BB->bottom &&
                    odesc->oBB().top <= BB->top)
                return (true);
            if (asel == ASELenclosed)
                return (false);
            if (cGEO::path_box_intersect(po.points, po.numpts, BB, true))
                return (true);
            if (asel == ASELall) {
                Point_c p(BB->left, BB->bottom);
                return (po.intersect(&p, true));
            }
            return (false);
        }
        else {
            if (psel == PSELpoint) {
                Point_c px(BB->left + delta, BB->bottom + delta);
                const Point *pts = ((CDw*)odesc)->points();
                int num = ((CDw*)odesc)->numpts();
                return (Point::inPath(pts, &px, delta, 0, num));
            }
            if (odesc->oBB().left >= BB->left &&
                    odesc->oBB().right <= BB->right &&
                    odesc->oBB().bottom >= BB->bottom &&
                    odesc->oBB().top <= BB->top)
                return (true);
            if (asel == ASELenclosed)
                return (false);
            return (cGEO::path_box_intersect(((CDw*)odesc)->points(),
                ((CDw*)odesc)->numpts(), BB, true));
        }
    }
label:
    {
        BBox tBB;
        bool set = false;
        int xform = ((CDla*)odesc)->xform();
        if (xform & (TXTF_SHOW | TXTF_HIDE)) {
            if (xform & TXTF_HIDE) {
                WindowDesc::LabelHideBB((CDla*)odesc, &tBB);
                if (!tBB.intersect(BB, true))
                    return (false);
                set = true;
            }
        }
        else if (WindowDesc::LabelHideTest((CDla*)odesc)) {
            WindowDesc::LabelHideBB((CDla*)odesc, &tBB);
            if (!tBB.intersect(BB, true))
                return (false);
            set = true;
        }

        Point *pts = 0;
        if (!set)
            odesc->boundary(&tBB, &pts);
        if (pts) {
            Poly poly = Poly(5, pts);
            if (psel == PSELpoint) {
                bool ret = poly.intersect(BB, true);
                delete [] pts;
                return (ret);
            }
            if (tBB.left >= BB->left && tBB.right <= BB->right &&
                    tBB.bottom >= BB->bottom && tBB.top <= BB->top) {
                delete [] pts;
                return (true);
            }
            if (asel == ASELenclosed) {
                delete [] pts;
                return (false);
            }
            if (cGEO::path_box_intersect(poly.points, poly.numpts, BB, true)) {
                delete [] pts;
                return (true);
            }
            if (asel == ASELall) {
                Point_c p(BB->left, BB->bottom);
                bool ret = poly.intersect(&p, true);
                delete [] pts;
                return (ret);
            }
            delete [] pts;
            return (false);
        }
        if (psel != PSELpoint && asel != ASELall) {
            if (tBB.left >= BB->left && tBB.right <= BB->right &&
                    tBB.bottom >= BB->bottom && tBB.top <= BB->top)
                return (true);
            if (asel == ASELenclosed)
                return (false);
            if (tBB.left < BB->left && tBB.right > BB->right &&
                    tBB.bottom < BB->bottom && tBB.top > BB->top)
                return (false);
        }
        return (true);
    }
inst:
    {
        BBox tBB;
        Point *pts;
        odesc->boundary(&tBB, &pts);
        if (pts) {
            Poly poly = Poly(5, pts);
            if (psel == PSELpoint) {
                bool ret = poly.intersect(BB, true);
                delete [] pts;
                return (ret);
            }
            if (tBB.left >= BB->left && tBB.right <= BB->right &&
                    tBB.bottom >= BB->bottom && tBB.top <= BB->top) {
                delete [] pts;
                return (true);
            }
            if (asel == ASELenclosed) {
                delete [] pts;
                return (false);
            }
            if (cGEO::path_box_intersect(poly.points, poly.numpts, BB, true)) {
                delete [] pts;
                return (true);
            }
            if (asel == ASELall) {
                Point_c p(BB->left, BB->bottom);
                bool ret = poly.intersect(&p, true);
                delete [] pts;
                return (ret);
            }
            delete [] pts;
            return (false);
        }
        if (psel != PSELpoint && asel != ASELall) {
            if (odesc->oBB().left >= BB->left &&
                    odesc->oBB().right <= BB->right &&
                    odesc->oBB().bottom >= BB->bottom &&
                    odesc->oBB().top <= BB->top)
                return (true);
            if (asel == ASELenclosed)
                return (false);
            if (odesc->oBB().left < BB->left &&
                    odesc->oBB().right > BB->right &&
                    odesc->oBB().bottom < BB->bottom &&
                    odesc->oBB().top > BB->top)
                return (false);
        }
        return (true);
    }
}


// Add all devices to the selection queue which have a property type num
// that matches the regular expression in str.
//
void
cSelections::pmatch(const CDs *sd, int num, const char *str, bool select)
{
    if (!sd)
        return;

    regex_t preg;
    bool all = !*str;
    if (!all) {
        if (regcomp(&preg, str, REG_EXTENDED | REG_ICASE | REG_NOSUB)) {
            PL()->ShowPrompt("Regular expression syntax error.");
            return;
        }
    }

    // If sd is physical, we will operate on extracted duals.
    const CDs *cursde;
    if (sd->isElectrical())
        cursde = sd;
    else
        cursde = CDcdb()->findCell(sd->cellname(), Electrical);
    if (!cursde)
        return;

    if (select) {
        selqueue_t *sq = 0;
        CDg gdesc;
        gdesc.init_gen(cursde, CellLayer());
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {
            if (!cdesc->is_normal())
                continue;
            CDp *pn = cdesc->prpty(num);
            for ( ; pn; pn = pn->next_prp()) {
                if (pn->value() != num)
                    continue;
                char *text;
                if (num == P_NAME) {
                    text = lstring::copy(
                        cdesc->getElecInstBaseName((CDp_cname*)pn));
                }
                else
                    pn->string(&text);
                if (all || !regexec(&preg, text, 0, 0, 0)) {
                    if (!sq)
                        sq = findQueue(sd, true);
                    if (sq) {
                        if (sd->isElectrical())
                            sq->insert_object(cdesc, SQinsShow);
                        else {
                            // If electrical instance is vectored, insert
                            // all physical elements.

                            CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
                            CDgenRange rgen(pr);
                            int vecix = 0;
                            while (rgen.next(0)) {
                                // Note that if cdual is an array, the
                                // insert call will be made multiple
                                // times, but this is ok as we test
                                // for duplicates.

                                CDc *cdual = cdesc->findPhysDualOfElec(vecix,
                                    0, 0);
                                if (cdual)
                                    sq->insert_object(cdual, SQinsShow);
                                vecix++;
                            }
                        }
                    }
                }
                delete [] text;
            }
        }
    }
    else {
        CDol *s0 = 0;
        sSelGen sg(*this, sd, "c");
        CDo *od;
        while ((od = sg.next()) != 0) {
            if (sd->isElectrical()) {
                for (CDp *pn = od->prpty(num); pn; pn = pn->next_prp()) {
                    if (pn->value() != num)
                        continue;
                    char *text;
                    if (num == P_NAME) {
                        text = lstring::copy(
                                OCALL(od)->getElecInstBaseName((CDp_cname*)pn));
                    }
                    else
                        pn->string(&text);
                    if (all || !regexec(&preg, text, 0, 0, 0)) {
                        s0 = new CDol(od, s0);
                        sg.remove();
                        delete [] text;
                        break;
                    }
                    delete [] text;
                }
                continue;
            }

            CDap ap(OCALL(od));
            for (unsigned int iy = 0; iy < ap.ny; iy++) {
                for (unsigned int ix = 0; ix < ap.nx; ix++) {
                    int vecix; // ignored
                    CDc *ed = OCALL(od)->findElecDualOfPhys(&vecix, ix, iy);
                    if (!ed)
                        continue;
                    for (CDp *pn = ed->prpty(num); pn; pn = pn->next_prp()) {
                        if (pn->value() != num)
                            continue;
                        char *text;
                        if (num == P_NAME)
                            text = ed->getElecInstName(vecix);
                        else
                            pn->string(&text);
                        if (all || !regexec(&preg, text, 0, 0, 0)) {
                            s0 = new CDol(od, s0);
                            sg.remove();
                            delete [] text;
                            break;
                        }
                        delete [] text;
                    }
                }
            }
        }
        if (s0) {
            selqueue_t::redisplay_list(sd, s0);
            CDol::destroy(s0);
        }
    }
    if (!all)
        regfree(&preg);
}


// If sx is given, pass back list of defunct entries, so we can consolidate
// the redisplays
//
void
cSelections::cmatch(const CDs *sd, const char *str, bool select, CDol **sx)
{
    if (!sd)
        return;

    regex_t preg;
    bool all = !*str;
    if (!all) {
        if (regcomp(&preg, str, REG_EXTENDED | REG_ICASE | REG_NOSUB)) {
            PL()->ShowPrompt("Regular expression syntax error.");
            return;
        }
    }
    if (select) {
        selqueue_t *sq = 0;
        CDm_gen mgen(sd, GEN_MASTERS);
        for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
            if (all || !regexec(&preg, Tstring(m->cellname()), 0, 0, 0)) {
                if (!sq)
                    sq = findQueue(sd, true);
                if (sq) {
                    CDc_gen cgen(m);
                    for (CDc *c = cgen.c_first(); c; c = cgen.c_next())
                        sq->insert_object(c, SQinsShow);
                }
            }
        }
    }
    else {
        CDol *s0 = 0;
        sSelGen sg(*this, sd, "c");
        CDo *od;
        while ((od = sg.next()) != 0) {
            if (all || !regexec(&preg, Tstring(OCALL(od)->cellname()),
                    0, 0, 0)) {
                s0 = new CDol(od, s0);
                sg.remove();
            }
        }
        if (s0) {
            if (sx) {
                CDol *sp = s0;
                while (s0->next)
                    s0 = s0->next;
                s0->next = *sx;
                *sx = sp;
            }
            else {
                selqueue_t::redisplay_list(sd, s0);
                CDol::destroy(s0);
            }
        }
    }
    if (!all)
        regfree(&preg);
}


namespace {
    CDll *match_layers(const char *str)
    {
        bool all = (!str || !strcmp(str, "all"));
        if (!all && str[0] == '-' && !str[1]) {
            // A '-' is a stand-in for the current layer.
            CDl *ld = LT()->CurLayer();
            if (ld)
                return (new CDll(ld, 0));
            PL()->ShowPrompt("No current layer.");
            return (0);
        }

        CDll *l0 = 0;
        regex_t preg;
        if (!all) {
            if (regcomp(&preg, str, REG_EXTENDED | REG_ICASE | REG_NOSUB)) {
                PL()->ShowPrompt("Regular expression syntax error.");
                return (0);
            }
        }
        CDl *ld;
        CDlgen lgen(DSP()->CurMode());
        while ((ld = lgen.next()) != 0) {
            if (all || !regexec(&preg, ld->name(), 0, 0, 0) ||
                   (ld->lppName() && !regexec(&preg, ld->lppName(), 0, 0, 0)))
                l0 = new CDll(ld, l0);
        }
        if (!all)
            regfree(&preg);

        return (l0);
    }


    // Stuff imported from DRC, does not require DRC module.

    sLspec *
    parseCoverageTest(const char *str, DRCtype *type)
    {
        char *tok = lstring::gettok(&str);
        if (lstring::cieq(tok, "Overlap"))
            *type = drOverlap;
        else if (lstring::cieq(tok, "IfOverlap"))
            *type = drIfOverlap;
        else if (lstring::cieq(tok, "NoOverlap"))
            *type = drNoOverlap;
        else if (lstring::cieq(tok, "AnyOverlap"))
            *type = drAnyOverlap;
        else if (lstring::cieq(tok, "PartOverlap"))
            *type = drPartOverlap;
        else if (lstring::cieq(tok, "AnyNoOverlap"))
            *type = drAnyNoOverlap;
        else {
            delete [] tok;
            return (0);
        }
        delete [] tok;

        sLspec *lspec = new sLspec;
        if (lspec->parseExpr(&str)) {
            if (lspec->setup())
                return (lspec);
        }
        Log()->ErrorLogV("selections", "Layer expression parse failed:\n%s",
            Errs()->get_error());
        delete lspec;
        return (0);
    }


    // Return the result of an Overlap test on zl.
    // Return XIintr if there was an interrupt *or an internal error*.
    //
    XIrt
    coverageTest(const Zlist *zl, sLspec *lspec, DRCtype type, bool *istrue)
    {
        int fudge = 2*Tech()->AngleSupport();
        if (type == drOverlap) {
            *istrue = true;
            SIlexprCx cx(0, CDMAXCALLDEPTH, zl);
            CovType cov;
            XIrt ret = lspec->testZlistCovFull(&cx, &cov, fudge);
            if (ret != XIok)
                return (ret);
            if (cov != CovFull)
                *istrue = false;
            return (XIok);
        }
        if (type == drIfOverlap) {
            *istrue = true;
            SIlexprCx cx(0, CDMAXCALLDEPTH, zl);
            CovType cov;
            XIrt ret = lspec->testZlistCovPartial(&cx, &cov, fudge);
            if (ret != XIok)
                return (ret);
            if (cov == CovPartial)
                *istrue = false;
            return (XIok);
        }
        if (type == drNoOverlap) {
            *istrue = true;
            SIlexprCx cx(0, CDMAXCALLDEPTH, zl);
            CovType cov;
            XIrt ret = lspec->testZlistCovNone(&cx, &cov, fudge);
            if (ret != XIok)
                return (ret);
            if (cov != CovNone)
                *istrue = false;
            return (XIok);
        }
        if (type == drAnyOverlap) {
            *istrue = false;
            SIlexprCx cx(0, CDMAXCALLDEPTH, zl);
            CovType cov;
            XIrt ret = lspec->testZlistCovNone(&cx, &cov, fudge);
            if (ret != XIok)
                return (ret);
            if (cov != CovNone)
                *istrue = true;
            return (XIok);
        }
        if (type == drPartOverlap) {
            *istrue = false;
            SIlexprCx cx(0, CDMAXCALLDEPTH, zl);
            CovType cov;
            XIrt ret = lspec->testZlistCovPartial(&cx, &cov, fudge);
            if (ret != XIok)
                return (ret);
            if (cov == CovPartial)
                *istrue = true;
            return (XIok);
        }
        if (type == drAnyNoOverlap) {
            *istrue = false;
            SIlexprCx cx(0, CDMAXCALLDEPTH, zl);
            CovType cov;
            XIrt ret = lspec->testZlistCovFull(&cx, &cov, fudge);
            if (ret != XIok)
                return (ret);
            if (cov != CovFull)
                *istrue = true;
            return (XIok);
        }
        return (XIbad);
    }


    inline XIrt
    coverageTest(const CDo *odesc, sLspec *lspec, DRCtype type, bool *istrue)
    {
        Zlist *zl = odesc->toZlist();
        XIrt ret = ::coverageTest(zl, lspec, type, istrue);
        Zlist::destroy(zl);
        return (ret);
    }
}


// Static function.
void
cSelections::lmatch(const CDs *sd, const char *str, bool select, CDol **sx)
{
    // lname w[ires] p[olygons] b[oxes] l[abels], default everything
    // this can be followed by one of the drc overlap keywords and
    // a layer expression, which must be true to select/deselect

    if (!sd)
        return;

    char *ltok = lstring::gettok(&str);
    CDll *ll = match_layers(ltok);
    if (!ll)
        return;
    delete [] ltok;

    bool wires = false;
    bool polys = false;
    bool boxes = false;
    bool labels = false;
    bool all = true;
    DRCtype testtype = drNoRule;
    sLspec *lspec = 0;
    while (*str) {
        if (lstring::ciprefix("overlap", str) ||
                lstring::ciprefix("ifoverlap", str) ||
                lstring::ciprefix("nooverlap", str) ||
                lstring::ciprefix("anyoverlap", str) ||
                lstring::ciprefix("partoverlap", str) ||
                lstring::ciprefix("anynooverlap", str)) {
            lspec = parseCoverageTest(str, &testtype);
            if (!lspec) {
                CDll::destroy(ll);
                return;
            }
            break;
        }
        char key = *str;
        if (isupper(key))
            key = tolower(key);
        switch (key) {
        case 'b':
            boxes = true;
            all = false;
            break;
        case 'p':
            polys = true;
            all = false;
            break;
        case 'w':
            wires = true;
            all = false;
            break;
        case 'l':
            labels = true;
            all = false;
            break;
        }
        while (*str && !isspace(*str))
            str++;
        while (isspace(*str))
            str++;
    }
    if (select) {
        selqueue_t *sq = 0;
        for (CDll *l = ll; l; l = l->next) {
            CDg gdesc;
            gdesc.init_gen(sd, l->ldesc);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                bool istrue;
                if ((odesc->type() == CDBOX && (boxes || all)) ||
                        (odesc->type() == CDPOLYGON && (polys || all)) ||
                        (odesc->type() == CDWIRE && (wires || all)) ||
                        (odesc->type() == CDLABEL && (labels || all))) {
                    if (!lspec || (lspec && coverageTest(odesc, lspec,
                            testtype, &istrue) == XIok && istrue)) {

                        if (!sq)
                            sq = findQueue(sd, true);
                        if (sq)
                            sq->insert_object(odesc, SQinsShow);
                    }
                }
            }
        }
    }
    else {
        CDol *s0 = 0;
        sSelGen sg(*this, sd, "bpwl");
        CDo *od;
        while ((od = sg.next()) != 0) {
            if (CDll::inlist(ll, od->ldesc())) {
                if ((od->type() == CDBOX && (boxes || all)) ||
                        (od->type() == CDPOLYGON && (polys || all)) ||
                        (od->type() == CDWIRE && (wires || all)) ||
                        (od->type() == CDLABEL && (labels || all))) {

                    bool istrue;
                    if (!lspec || (lspec && coverageTest(od, lspec,
                            testtype, &istrue) == XIok && istrue)) {
                        s0 = new CDol(od, s0);
                        sg.remove();
                    }
                }
            }
        }
        if (s0) {
            if (sx) {
                CDol *sp = s0;
                while (s0->next)
                    s0 = s0->next;
                s0->next = *sx;
                *sx = sp;
            }
            else {
                selqueue_t::redisplay_list(sd, s0);
                CDol::destroy(s0);
            }
        }
    }
    CDll::destroy(ll);
    delete lspec;
}
// End of cSelections functions.


// Generator for list elements, constructor.
//
sSelGen::sSelGen(cSelections &sel, const CDs *sd, const char *types, bool all)
{
    sg_sq = sel.findQueue(sd, false);
    sg_el = sg_sq ? sg_sq->queue() : 0;
    sg_obj = 0;
    sg_types = types;
    sg_all = all;
}


CDo *
sSelGen::next()
{
    while (sg_el) {
        CDo *od = sg_el->odesc;
        sg_el = sg_el->next;
        if (match_type(sg_types, od) &&
                (sg_all || od->state() == CDobjSelected)) {
            sg_obj = od;
            return (od);
        }
    }
    return (0);
}


// Have to be a little careful with list changes while the generator
// is active.  The two convenience functions below are guaranteed ok. 
// Simple insertions are also ok, as they are placed on the list head,
// outside the generator range.

// Conveniently remove the last return from next() from the queue.
//
void
sSelGen::remove()
{
    if (sg_sq && sg_obj)
        sg_sq->remove_object(sg_obj);
}


// Conveniently replace the last return from next() with newo.
//
void
sSelGen::replace(CDo *newo)
{
    if (newo && sg_sq && sg_obj)
        sg_sq->replace_object(sg_obj, newo);
}
// End of sSelGen functions.


// Return a new element, all data fields are clear.  This and the
// following function whould be used for all allocation/deallocation
// of elements.
//
sqel_t *
sqelfct_t::new_element()
{
    if (sqf_deleted) {
        sqel_t *sqf = sqf_deleted;
        sqf_deleted = sqf_deleted->next;
        sqf->next = 0;
        return (sqf);
    }
    if (!sqf_blocks || sqf_alloc == SQF_BLSIZE) {
        sqf_blocks = new sqfblock_t(sqf_blocks);
        sqf_alloc = 0;
    }
    sqel_t *sqf = &sqf_blocks->elements[sqf_alloc++];
    memset(sqf, 0, sizeof(sqel_t));
    return (sqf);
}


// Hook the "deleted" element onto the deleted list for potential
// reuse.
//
void
sqelfct_t::delete_element(sqel_t *el)
{
    el->next = sqf_deleted;
    sqf_deleted = el;
    el->prev = 0;
    el->set_tab_next(0);
    el->odesc = 0;
}
// End of sqelfct_t functions.


// Insert an object into the queue, this becomes the new list head. 
// True is returned if the object is in the list.  If sm is SQinsShow,
// the object will be displayed.  If sm is SQinsProvShow, the object
// will be displayed if its state is CDobjVanilla.  If sm is SQinsNoShow,
// to object won't be displayed.  In any case, the object state is set
// to CDobjSelected.
//
bool
selqueue_t::insert_object(CDo *od, SQinsMode sm)
{
    if (!od || !od->is_normal())
        return (false);

    // These flags (actually, CDmergeDeleted) are used by the undo
    // system, and are consulted during an undo operation.  They are
    // cleared here, i.e., when the object is placed in the selection
    // list.
    //
    od->unset_flag(CDmergeCreated | CDmergeDeleted);

    if (!sq_tab)
        sq_tab = new itable_t<sqel_t>;
    if (sq_tab->find(od))
        return (true); // already in list

    sqel_t *el = sq_fct.new_element();
    el->next = sq_list;
    sq_list = el;
    if (el->next)
        el->next->prev = el;
    el->prev = 0;
    el->odesc = od;
    sq_tab->link(el, false);
    sq_tab = sq_tab->check_rehash();
    if (sm == SQinsShow) {
        od->set_state(CDobjVanilla);
        show_selected(sq_sdesc, od);
    }
    else if (sm == SQinsProvShow)
        show_selected(sq_sdesc, od);
    else
        od->set_state(CDobjSelected);
    return (true);
}


// Replace od with odnew, return true on success.
//
bool
selqueue_t::replace_object(CDo *od, CDo *odnew)
{
    if (!od || !odnew || !odnew->is_normal())
        return (false);

    // See note above.
    odnew->unset_flag(CDmergeCreated | CDmergeDeleted);

    if (!sq_tab)
        return (false);
    if (sq_tab->find(odnew))
        return (false);  // can't have duplicates
    sqel_t *el = sq_tab->remove(od);
    if (!el)
        return (false);
    el->odesc = odnew;
    odnew->set_state(CDobjSelected);
    sq_tab->link(el);
    return (true);
}


// Add objects to the list as selected, and display them.
//
void
selqueue_t::insert_and_show(CDol *list)
{
    for (CDol *c = list; c; c = c->next) {
        CDo *od = c->odesc;
        if (!od)
            continue;
        insert_object(od, SQinsNoShow);
    }
    if (!DSP()->NoRedisplay()) {
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (!wdesc->Wdraw())
                continue;
            if (wdesc->IsShowing(sq_sdesc)) {
                wdesc->Wdraw()->SetColor(wdesc->Mode() == Physical ?
                    DSP()->SelectPixelPhys() : DSP()->SelectPixelElec());
                for (CDol *c = list; c; c = c->next) {
                    // Test for user interrupt
                    if (DSP()->Interrupt()) {
                        DSP()->RedisplayAfterInterrupt();
                        return;
                    }
                    if (c->odesc)
                        wdesc->DisplaySelected(c->odesc);
                }
            }
        }
    }
}


// Remove od from the list, return true if found.  No redisplay is
// done.
//
bool
selqueue_t::remove_object(CDo *od)
{
    if (!od || !sq_tab)
        return (false);
    sqel_t *el = sq_tab->remove(od);
    if (!el)
        return (false);
    if (el->prev)
        el->prev->next = el->next;
    else
        sq_list = el->next;
    if (el->next)
        el->next->prev = el->prev;
    if (el->odesc->state() == CDobjSelected)
        el->odesc->set_state(CDobjVanilla);
    sq_fct.delete_element(el);

    // Undisplay stretch handles in pCell instance.
    if (sq_sdesc == CurCell(Physical) && od->type() == CDINSTANCE)
        EditIf()->unregisterGrips((CDc*)od);

    return (true);
}


// Remove objects that match types, whether or not selected.  No
// redisplay is done.
//
void
selqueue_t::remove_types(const char *types)
{
    sqel_t *en;
    for (sqel_t *el = sq_list; el; el = en) {
        en = el->next;
        if (match_type(types, el->odesc))
            remove_object(el->odesc);
    }
}


// Remove objects on ld.  No redisplay is done.
//
void
selqueue_t::remove_layer(const CDl *ld)
{
    if (!ld)
        return;
    sqel_t *en;
    for (sqel_t *el = sq_list; el; el = en) {
        en = el->next;
        if (el->odesc->ldesc() == ld)
            remove_object(el->odesc);
    }
}


// Deselect all selected objects listed in types.  Delete them from
// the list.  Non-selected objects are *not* deleted.
//
void
selqueue_t::deselect_types(const char *types)
{
    CDol *o0 = 0;
    sqel_t *en;
    for (sqel_t *el = sq_list; el; el = en) {
        en = el->next;
        if (match_type(types, el->odesc) &&
                el->odesc->state() == CDobjSelected) {
            o0 = new CDol(el->odesc, o0);
            remove_object(el->odesc);
        }
    }
    redisplay_list(sq_sdesc, o0);
    CDol::destroy(o0);
}


// Deselect all selected objects on ld, and delete them from the list. 
// Non-selected objects are *not* deleted.
//
void
selqueue_t::deselect_layer(const CDl *ld)
{
    CDol *o0 = 0;
    sqel_t *en;
    for (sqel_t *el = sq_list; el; el = en) {
        en = el->next;
        if (el->odesc->ldesc() == ld && el->odesc->state() == CDobjSelected) {
            o0 = new CDol(el->odesc, o0);
            remove_object(el->odesc);
        }
    }
    redisplay_list(sq_sdesc, o0);
    CDol::destroy(o0);
}


// If the list head is selected, deselect and remove it.
//
bool
selqueue_t::deselect_last()
{
    if (sq_list) {
        CDo *od = sq_list->odesc;
        if (od->state() == CDobjSelected) {
            show_unselected(sq_sdesc, od);
            remove_object(od);
            return (true);
        }
    }
    return (false);
}


// Compute bounding box of the queued and selected objects.  This is the
// actual physical BB, and does not include the instance origin marks.
//
bool
selqueue_t::compute_bb(BBox *nBB, bool all) const
{
    bool ret = false;
    if (nBB) {
        *nBB = CDnullBB;
        if (all) {
            for (sqel_t *el = sq_list; el; el = el->next) {
                nBB->add(&el->odesc->oBB());
                ret = true;
            }
        }
        else {
            for (sqel_t *el = sq_list; el; el = el->next) {
                if (el->odesc->state() == CDobjSelected) {
                    nBB->add(&el->odesc->oBB());
                    ret = true;
                }
            }
        }
    }
    return (ret);
}


// Return the number of selected objects in the queue (first arg) and
// the number of unselected objects (second arg).
//
void
selqueue_t::count_queue(unsigned int *ns, unsigned int *nu) const
{
    unsigned int s = 0, u = 0;
    for (sqel_t *el = sq_list; el; el = el->next) {
        if (el->odesc->state() == CDobjSelected)
            s++;
        else
            u++;
    }
    if (ns)
        *ns = s;
    if (nu)
        *nu = u;
}


// Turn off/on the display of selected object highlighting of objects
// of types in selection queue.
//
void
selqueue_t::set_show_selected(const char *types, bool on)
{
    if (on) {
        for (sqel_t *c = sq_list; c; c = c->next) {
            if (match_type(types, c->odesc) &&
                    c->odesc->state() == CDobjVanilla) {
                c->odesc->set_state(CDobjSelected);
                WindowDesc *wdesc;
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                while ((wdesc = wgen.next()) != 0) {
                    if (wdesc->IsShowing(sq_sdesc)) {
                        DSPmainDraw(SetColor(wdesc->Mode() == Physical ?
                            DSP()->SelectPixelPhys() :
                            DSP()->SelectPixelElec()))
                        wdesc->DisplaySelected(c->odesc);
                    }
                }
            }
        }
        return;
    }
    if (!DSP()->NoPixmapStore()) {
        BBox BB(CDnullBB);
        int cnt = 0;
        for (sqel_t *c = sq_list; c; c = c->next) {
            if (match_type(types, c->odesc) &&
                    c->odesc->state() == CDobjSelected) {
                c->odesc->set_state(CDobjVanilla);
                BB.add(&c->odesc->oBB());
                cnt++;
            }
        }
        if (cnt) {
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0) {
                if (wdesc->IsShowing(sq_sdesc))
                    wdesc->Refresh(&BB);
            }
        }
        return;
    }

    int cnt = 0;
    for (sqel_t *c = sq_list; c; c = c->next) {
        if (match_type(types, c->odesc) &&
                c->odesc->state() == CDobjSelected) {
            if (cnt++ > DEL_AGGREG)
                break;
        }
    }
    if (cnt > DEL_AGGREG) {
        BBox BB(CDnullBB);
        for (sqel_t *c = sq_list; c; c = c->next) {
            if (match_type(types, c->odesc) &&
                    c->odesc->state() == CDobjSelected) {
                c->odesc->set_state(CDobjVanilla);
                BB.add(&c->odesc->oBB());
            }
        }
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (wdesc->IsShowing(sq_sdesc))
                wdesc->Redisplay(&BB);
        }
    }
    else {
        for (sqel_t *c = sq_list; c; c = c->next) {
            if (match_type(types, c->odesc) &&
                    c->odesc->state() == CDobjSelected) {
                c->odesc->set_state(CDobjVanilla);
                WindowDesc *wdesc;
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                while ((wdesc = wgen.next()) != 0) {
                    if (wdesc->IsShowing(sq_sdesc))
                        wdesc->DisplayUnselected(c->odesc);
                }
            }
        }
    }
}


// Show all selections.
//
unsigned int
selqueue_t::show(WindowDesc *wdesc) const
{
    unsigned int dcnt = 0;
    wdesc->Wdraw()->SetColor(wdesc->Mode() == Physical ?
        DSP()->SelectPixelPhys() : DSP()->SelectPixelElec());
    for (sqel_t *el = sq_list; el; el = el->next) {
        // Test for user interrupt.
        if (DSP()->Interrupt()) {
            DSP()->RedisplayAfterInterrupt();
            break;
        }
        if (el->odesc->state() == CDobjSelected) {
            wdesc->DisplaySelected(el->odesc);

            // Display stretch handles in pcell instances.
            if (wdesc->Mode()== Physical && el->odesc->type() == CDINSTANCE)
                EditIf()->registerGrips((CDc*)el->odesc);

            dcnt++;
        }
    }
    return (dcnt);
}


namespace {
    // Compute the bounding box of the list, taking into account the
    // origin mark of instances, which may extend outside of the
    // instance bounding box.
    //
    bool compute_BB(WindowDesc *wdesc, CDol *list, BBox *nBB)
    {
        bool ret = false;
        *nBB = CDnullBB;
        for (CDol *c = list; c; c = c->next) {
            if (c->odesc) {
                if (c->odesc->type() == CDINSTANCE) {
                    BBox mBB;
                    if (wdesc->InstanceOriginMarkBB((CDc*)c->odesc, &mBB))
                        nBB->add(&mBB);
                }
                nBB->add(&c->odesc->oBB());
                ret = true;
            }
        }
        return (ret);
    }

    // Return a list of edge boxes, used for screen refresh.
    //
    Blist *addEdges(CDo *odesc, WindowDesc *wd, Blist *bl)
    {
        if (!odesc || !wd->Wdraw())
            return (bl);
#ifdef HAVE_COMPUTED_GOTO
        COMPUTED_JUMP(odesc)
#else
        CONDITIONAL_JUMP(odesc)
#endif
    box:
        return (wd->AddEdges(bl, &odesc->oBB()));
    poly:
    wire:
    label:
        return (new Blist(&odesc->oBB(), bl));
    inst:
        {
            CDs *sdesc = ((CDc*)odesc)->masterCell();
            if (!sdesc)
                return (bl);
            if (!sdesc->isElectrical() || !sdesc->isLibrary() ||
                    !sdesc->isDevice()) {
                BBox BB;
                Point *pts;
                odesc->boundary(&BB, &pts);
                BBox mBB;
                if (wd->InstanceOriginMarkBB((CDc*)odesc, &mBB))
                    bl = new Blist(&mBB, bl);
                if (pts) {
                    delete [] pts;
                    return (new Blist(&BB, bl));
                }
                return (wd->AddEdges(bl, &BB));
            }
            return (new Blist(&odesc->oBB(), bl));
        }
    }
}


// Returns true if one of types is in the selection queue, and is selected
// or all is set.
//
bool
selqueue_t::has_types(const char *types, bool all) const
{
    for (sqel_t *el = sq_list; el; el = el->next) {
        if (match_type(types, el->odesc) &&
                (all || el->odesc->state() == CDobjSelected))
            return (true);
    }
    return (false);
}


// Return true if od is in the list.
//
bool
selqueue_t::in_queue(CDo *od) const
{
    if (od && sq_tab)
        return (sq_tab->find(od) != 0);
    return (false);
}


// Return the first matching object found in the list.
//
CDo *
selqueue_t::first_object(const char *types, bool all, bool array_only) const
{
    for (sqel_t *el = sq_list; el; el = el->next) {
        if (match_type(types, el->odesc) &&
                (all || el->odesc->state() == CDobjSelected)) {
            if (el->odesc->type() == CDINSTANCE) {
                CDc *cdesc = OCALL(el->odesc);
                CDs *sdesc = cdesc->masterCell();
                if (!sdesc || (sdesc->isLibrary() && sdesc->isDevice()))
                    continue;
                if (array_only) {
                    CDap ap(cdesc);
                    if (ap.nx <= 1 && ap.ny <= 1)
                        continue;
                }
            }
            return (el->odesc);
        }
    }
    return (0);
}


// Remove any entries that are CDDeleted.
//
void
selqueue_t::purge_deleted()
{
    sqel_t *en;
    for (sqel_t *el = sq_list; el; el = en) {
        en = el->next;
        if (el->odesc->state() == CDobjDeleted)
            remove_object(el->odesc);
    }
}


// Make sure that all labels associated with the selected cells are in
// the list.
//
void
selqueue_t::add_labels()
{
    if (!sq_sdesc->isElectrical())
        return;

    // First extract cells from the list.
    CDol *list = 0;
    sqel_t *en;
    for (sqel_t *el = sq_list; el; el = en) {
        en = el->next;
        if (el->odesc->type() == CDINSTANCE) {
            CDo *od = el->odesc;
            CDobjState state = od->state();
            remove_object(od);
            od->set_state(state);
            list = new CDol(od, list);
        }
        else if (el->odesc->type() == CDWIRE) {
            CDp_node *pn = (CDp_node*)el->odesc->prpty(P_NODE);
            if (!pn || !pn->bound()) {
                CDp_bnode *pb = (CDp_bnode*)el->odesc->prpty(P_BNODE);
                if (!pb || !pb->bound())
                    continue;
            }
            CDo *od = el->odesc;
            CDobjState state = od->state();
            remove_object(od);
            od->set_state(state);
            list = new CDol(od, list);
        }
        else if (el->odesc->type() == CDLABEL) {
            CDp_lref *pl = (CDp_lref*)el->odesc->prpty(P_LABRF);
            if (pl && pl->devref() && (pl->devref()->type() == CDINSTANCE ||
                    pl->devref()->type() == CDWIRE)) {
                CDo *od = el->odesc;
                CDobjState state = od->state();
                remove_object(od);
                od->set_state(state);
                list = new CDol(od, list);
            }
        }
    }
    if (!list)
        return;

    for (CDol *c = list; c; c = c->next) {
        if (c->odesc->state() != CDobjSelected)
            continue;
        if (c->odesc->type() == CDLABEL) {
            CDp_lref *pl = (CDp_lref*)c->odesc->prpty(P_LABRF);
            if (!pl || !pl->devref())
                continue;
            if (pl->devref()->type() == CDINSTANCE ||
                    pl->devref()->type() == CDWIRE) {
                insert_object(pl->devref(), SQinsShow);
            }
            continue;
        }
        for (CDp *pd = c->odesc->prpty_list(); pd; pd = pd->next_prp()) {
            if (pd->value() == P_MUTLRF) {
                CDp_nmut *pm = (CDp_nmut*)sq_sdesc->prpty(P_NEWMUT);
                for ( ; pm; pm = pm->next()) {
                    CDc *other;
                    if (pm->match((CDc*)c->odesc, &other) && other &&
                            other->state() == CDobjSelected) {
                        CDla *olabel = pm->bound();
                        if (olabel)
                            insert_object(olabel, SQinsShow);
                    }
                }
            }
            else {
                CDla *olabel = pd->bound();
                if (olabel)
                    insert_object(olabel, SQinsShow);
            }
        }
    }

    // Put the queue back in order.
    for (CDol *ol = list; ol; ol = ol->next) {
        // Keep the present state.
        CDobjState st = ol->odesc->state();
        insert_object(ol->odesc, SQinsNoShow);
        ol->odesc->set_state(st);
    }
    CDol::destroy(list);
}


// Remove from the select queue any labels that are referenced by
// included cells, or any property labels if all is set.
//
void
selqueue_t::purge_labels(bool all)
{
    if (!sq_sdesc->isElectrical())
        return;

    if (all) {
        sqel_t *en;
        for (sqel_t *el = sq_list; el; el = en) {
            en = el->next;
            if (el->odesc->type() == CDLABEL) {
                if (el->odesc->prpty(P_LABRF)) {
                    show_unselected(sq_sdesc, el->odesc);
                    remove_object(el->odesc);
                }
            }
        }
        return;
    }

    // First extract cells and wires from the list.
    CDol *list = 0;
    sqel_t *en;
    for (sqel_t *el = sq_list; el; el = en) {
        en = el->next;
        if (el->odesc->type() == CDINSTANCE) {
            CDo *od = el->odesc;
            CDobjState state = od->state();
            remove_object(od);
            od->set_state(state);
            list = new CDol(od, list);
        }
        else if (el->odesc->type() == CDWIRE) {
            CDp_node *pn = (CDp_node*)el->odesc->prpty(P_NODE);
            if (!pn || !pn->bound()) {
                CDp_bnode *pb = (CDp_bnode*)el->odesc->prpty(P_BNODE);
                if (!pb || !pb->bound())
                    continue;
            }
            CDo *od = el->odesc;
            CDobjState state = od->state();
            remove_object(od);
            od->set_state(state);
            list = new CDol(od, list);
        }
    }
    if (!list)
        return;

    // Now purge the associated labels.
    for (CDol *c = list; c; c = c->next) {
        for (CDp *pd = c->odesc->prpty_list(); pd; pd = pd->next_prp()) {
            CDla *olabel;
            if (pd->value() == P_MUTLRF) {
                CDp_nmut *pm = (CDp_nmut*)sq_sdesc->prpty(P_NEWMUT);
                for ( ; pm; pm = pm->next()) {
                    if (pm->match((CDc*)c->odesc, NULL)) {
                        if ((olabel = pm->bound()) != 0) {
                            show_unselected(sq_sdesc, olabel);
                            remove_object(olabel);
                        }
                        break;
                    }
                }
            }
            else if ((olabel = pd->bound()) != 0) {
                show_unselected(sq_sdesc, olabel);
                remove_object(olabel);
            }
        }
    }

    // Put the queue back in order.
    for (CDol *ol = list; ol; ol = ol->next) {
        // Keep the present state.
        CDobjState st = ol->odesc->state();
        insert_object(ol->odesc, SQinsNoShow);
        ol->odesc->set_state(st);
    }
    CDol::destroy(list);
}


// Move instances to the front of the list.  When adding cells in
// electrical mode, to set up the reference pointers properly the
// calls must be added first.
//
void
selqueue_t::inst_to_front()
{
    sqel_t *ex = 0;  
    sqel_t *en;
    for (sqel_t *el = sq_list; el; el = en) {
        en = el->next;
        if (el->odesc->type() == CDINSTANCE) {
            if (!el->prev)
                sq_list = en;
            else
                el->prev->next = en;
            if (en)
                en->prev = el->prev;
            el->next = ex;
            ex = el;
            el->prev = 0;
        }
    }
    while (ex) {
        sqel_t *el = ex;
        ex = ex->next;
        el->next = sq_list;
        if (sq_list)
            sq_list->prev = el;
        sq_list = el;
    }
}


// Return a trapezoid list from objects on ld in the queue, clipped to
// zref.
//
Zlist *
selqueue_t::get_zlist(const CDl *ld, const Zlist *zref) const
{
    if (!ld || !zref)
        return (0);

    CDol *ol0 = 0;
    for (sqel_t *el = sq_list; el; el = el->next) {
        if (el->odesc->ldesc() == ld && el->odesc->type() != CDLABEL &&
                el->odesc->is_normal())
            ol0 = new CDol(el->odesc, ol0);
    }
    if (!ol0)
        return (0);

    BBox sBB;
    CDol::computeBB(ol0, &sBB);

    Zlist *z0 = 0;
    for (const Zlist *zl = zref; zl; zl = zl->next) {
        BBox tBB;
        zl->Z.BB(&tBB);
        bool manh = zl->Z.is_rect();
        bool noclip = (zl->Z.yu >= sBB.top && zl->Z.yl <= sBB.bottom &&
                    zl->Z.xll <= sBB.left && zl->Z.xul <= sBB.left &&
                    zl->Z.xlr >= sBB.right && zl->Z.xur >= sBB.right);

        for (CDol *ol = ol0; ol; ol = ol->next) {
            Zlist *zx = ol->odesc->toZlist();
            if (!noclip && (!manh || !(ol->odesc->oBB() <= tBB)))
                Zlist::zl_and(&zx, &zl->Z);
            if (zx) {
                Zlist *zn = zx;
                while (zn->next)
                    zn = zn->next;
                zn->next = z0;
                z0 = zx;
            }
        }
    }
    CDol::destroy(ol0);

    try {
        z0 = Zlist::repartition(z0);
        return (z0);
    }
    catch (XIrt) {
        return (0);
    }
}


// List consistency check.
// Remove from the selection queue any nonselected objects, and any
// duplicates.  Report an error if these conditions exist.  Return
// true if the queue is initially clean.
//
bool
selqueue_t::check()
{
    if (!sq_list)
        return (true);
    int unst = 0;
    sqel_t *en;
    for (sqel_t *el = sq_list; el; el = en) {
        en = el->next;
        if (el->odesc->state() != CDobjSelected) {
            remove_object(el->odesc);
            unst++;
        }
    }
    if (unst) {
        const char *mesg =
            "Internal selection list inconsistency:  %d unset.";
        if (XM()->DebugFlags() & DBG_SELECT)
            Log()->ErrorLogV("selections", mesg, unst);
        return (false);
    }
    return (true);
}


// Static function.
// Show the object as selected.  Object must be CDobjVanilla, and if so
// it is set to CDobjSelected and displayed.  The object need not be in
// any list.
//
void
selqueue_t::show_selected(const CDs *sd, CDo *cd)
{
    if (!sd || !cd || cd->state() != CDobjVanilla)
        return;
    cd->set_state(CDobjSelected);
    if (DSP()->NoRedisplay())
        return;
    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wdesc = wgen.next()) != 0) {
        if (wdesc->IsShowing(sd)) {
            DSPmainDraw(SetColor(wdesc->Mode() == Physical ?
                DSP()->SelectPixelPhys() : DSP()->SelectPixelElec()))
            wdesc->DisplaySelected(cd);
        }
    }
}


// Static function.
// Show object as unselected.  Object must be CDobjSelected, and if so it
// is set to CDobjVanilla and shown as unselected.  The object need not be
// in any list.
//
void
selqueue_t::show_unselected(const CDs *sd, CDo *od)
{
    if (!sd || !od || od->state() != CDobjSelected)
        return;
    od->set_state(CDobjVanilla);
    if (DSP()->NoRedisplay())
        return;
    if (!DSP()->NoPixmapStore()) {
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (wdesc->IsShowing(sd)) {
                if (DSP()->NumberVertices() && od->type() == CDPOLYGON) {
                    int d = (int)(30.0/wdesc->Ratio());
                    BBox BB = od->oBB();
                    BB.bloat(d);
                    wdesc->Refresh(&BB);
                }
                else if (od->type() == CDINSTANCE) {
                    BBox BB = od->oBB();
                    BBox mBB;
                    if (wdesc->InstanceOriginMarkBB((CDc*)od, &mBB))
                        BB.add(&mBB);
                    if (wdesc->Mode() == Electrical) {
                        // The instance bounding box may not include dots
                        // if dots are enabled, bloat to compensate.
                        BB.bloat(CDelecResolution/2);
                    }
                    wdesc->Refresh(&BB);
                }
                else
                    wdesc->Refresh(&od->oBB());
            }
        }
        return;
    }

    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wdesc = wgen.next()) != 0) {
        if (wdesc->IsShowing(sd))
            wdesc->DisplayUnselected(od);
    }
}


// Static function.
// Update the display area of objects in the list.
//
void
selqueue_t::redisplay_list(const CDs *sd, CDol *c0)
{
    if (!sd || !c0)
        return;

    if (!DSP()->NoPixmapStore()) {
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (wdesc->IsShowing(sd)) {
                BBox BB;
                if (compute_BB(wdesc, c0, &BB)) {
                    int delta = 4.0/wdesc->Ratio();
                    BB.bloat(delta);
                    wdesc->Refresh(&BB);
                }
            }
        }
        return;
    }

    // If more than DEL_AGGREG objects, just redisplay total
    // bounding box.
    int cnt = 0;
    for (CDol *c = c0; c; c = c->next) {
        cnt++;
        if (cnt > DEL_AGGREG)
            break;
    }
    if (cnt > DEL_AGGREG) {
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (wdesc->IsShowing(sd)) {
                BBox BB;
                if (compute_BB(wdesc, c0, &BB))
                    wdesc->Redisplay(&BB);
            }
        }
    }
    else {
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (wdesc->IsShowing(sd)) {
                Blist *bl = 0;
                for (CDol *c = c0; c; c = c->next)
                    bl = addEdges(c->odesc, wdesc, bl);
                if (bl) {
                    wdesc->RedisplayList(bl);
                    Blist::destroy(bl);
                }
            }
        }
    }
}

