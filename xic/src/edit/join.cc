
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

#include "main.h"
#include "edit.h"
#include "undolist.h"
#include "cd_lgen.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "geo_zlist.h"
#include "layertab.h"
#include "promptline.h"
#include "select.h"
#include "events.h"


//
// Functions for joining objects into polygons, and splitting polys
// into trapezoids.
//


namespace {
    // List element for an object and its Zlist representation, for
    // joining.
    //
    struct j_list
    {
        j_list(CDo *o) { j_odesc = o; j_zlist = 0; next = 0; }
        ~j_list() { Zlist::destroy(j_zlist); }

        static XIrt join(j_list*, CDs*, CDl*, bool);

        j_list *next;

    private:
        CDo *j_odesc;
        Zlist *j_zlist;
    };


    // Static function.
    XIrt
    j_list::join(j_list *j0, CDs *cursd, CDl *ld, bool use_sq)
    {
        if (!j0)
            return (XIok);
        int ocnt = 0;
        for (j_list *j = j0; j; j = j->next)
            ocnt++;

        int cnt = 0;
        for (;;) {
            int zcnt = 0;
            Zlist *z0 = 0, *ze = 0;
            while (j0) {
                j_list *jl = j0;
                j0 = j0->next;

                if (!jl->j_zlist)
                    jl->j_zlist = jl->j_odesc->toZlist();

                int n = Zlist::length(jl->j_zlist);
                if (Zlist::JoinMaxQueue > 0 && n >= Zlist::JoinMaxQueue) {
                    // Complex object, no need to rejoin.  Just remove
                    // from queue and continue.
                    if (use_sq)
                        Selections.removeObject(CurCell(), jl->j_odesc);
                    Zlist::destroy(jl->j_zlist);
                    jl->j_zlist = 0;
                    cnt++;
                    delete jl;
                    continue;
                }
                if (Zlist::JoinMaxQueue <= 0 ||
                        zcnt + n < Zlist::JoinMaxQueue) {
                    if (!z0)
                        z0 = ze = jl->j_zlist;
                    else {
                        while (ze->next)
                            ze = ze->next;
                        ze->next = jl->j_zlist;
                    }
                    zcnt += n;
                    jl->j_zlist = 0;
                    if (!(cnt % 50))
                        PL()->ShowPromptV("Working... %6d/%d", cnt, ocnt);
                    Ulist()->RecordObjectChange(cursd, jl->j_odesc, 0);
                    cnt++;
                    delete jl;
                    continue;
                }
                j0 = jl;
                break;
            }
            if (!z0)
                break;
            XIrt ret = Zlist::to_poly_add(z0, cursd, ld, true);
            if (ret != XIok) {
                DSP()->SetInterrupt(DSPinterNone);
                Ulist()->RestoreObjects();
                if (use_sq)
                    Selections.setShowSelected(CurCell(), 0, true);
                while (j0) {
                    j_list *jl = j0;
                    j0 = j0->next;
                    delete jl;
                }
                return (ret);
            }
        }
        return (XIok);
    }
}
// End of j_list functions.


// Command to join all objects in current cell.  Objects on visible,
// selectable, and mergeable layers will be joined.
//
bool
cEdit::joinAllCmd()
{
    CDs *cursd = CurCell();
    if (!cursd) {
        Errs()->add_error("No current cell!");
        return (false);
    }

    EV()->InitCallback();
    Ulist()->ListCheck("JOINALL", cursd, false);
    DSPpkg::self()->SetWorking(true);
    PL()->ShowPrompt("Working...");

    bool incl_wires = DSP()->CurMode() == Physical && Zlist::JoinSplitWires;
    CDsLgen lgen(cursd);
    CDl *ld;
    while ((ld = lgen.next()) != 0) {
        // Only visible, selectable, mergeable layers will be joined.
        if (ld->isInvisible())
            continue;
        if (ld->isNoSelect())
            continue;
        if (ld->isNoMerge())
            continue;

        j_list *j0 = 0, *je = 0;
        CDg gdesc;
        gdesc.init_gen(cursd, ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (odesc->type() == CDLABEL || odesc->type() == CDINSTANCE)
                continue;
            if (odesc->type() == CDWIRE) {
                if (!incl_wires)
                    continue;
                if (((CDw*)odesc)->wire_width() == 0)
                    continue;
            }
            if (!odesc->is_normal())
                continue;
            if (!j0)
                j0 = je = new j_list(odesc);
            else {
                je->next = new j_list(odesc);
                je = je->next;
            }
        }
        XIrt ret = j_list::join(j0, cursd, ld, false);
        if (ret != XIok) {
            XM()->ShowParameters();
            DSPpkg::self()->SetWorking(false);
            if (ret == XIbad)
                Errs()->add_error("FAILED: internal error.");
            else if (ret == XIintr)
                Errs()->add_error("Interrupt, aborted.");
            return (false);
        }
    }
    XM()->ShowParameters();
    DSPpkg::self()->SetWorking(false);

    Ulist()->CommitChanges(true);
    return (true);
}


// Command to join all objects on the current layer.  Objects will be
// joined if the current layer is visible and mergeable.
//
bool
cEdit::joinLyrCmd()
{
    CDs *cursd = CurCell();
    if (!cursd) {
        Errs()->add_error("No current cell!");
        return (false);
    }
    CDl *ld = LT()->CurLayer();
    if (!ld) {
        Errs()->add_error("No current layer!");
        return (false);
    }
    if (ld->isInvisible()) {
        Errs()->add_error("Current layer is invisible, join not allowed.");
        return (false);
    }
    if (ld->isNoMerge()) {
        Errs()->add_error("Current layer has NoMerge set, join not allowed.");
        return (false);
    }

    EV()->InitCallback();
    Ulist()->ListCheck("JOINLYR", cursd, false);
    DSPpkg::self()->SetWorking(true);
    PL()->ShowPrompt("Working...");

    bool incl_wires = DSP()->CurMode() == Physical && Zlist::JoinSplitWires;

    j_list *j0 = 0, *je = 0;
    CDg gdesc;
    gdesc.init_gen(cursd, ld);
    CDo *odesc;
    while ((odesc = gdesc.next()) != 0) {
        if (odesc->type() == CDLABEL || odesc->type() == CDINSTANCE)
            continue;
        if (odesc->type() == CDWIRE) {
            if (!incl_wires)
                continue;
            if (((CDw*)odesc)->wire_width() == 0)
                continue;
        }
        if (!odesc->is_normal())
            continue;
        if (!j0)
            j0 = je = new j_list(odesc);
        else {
            je->next = new j_list(odesc);
            je = je->next;
        }
    }
    XIrt ret = j_list::join(j0, cursd, ld, false);
    if (ret != XIok) {
        XM()->ShowParameters();
        DSPpkg::self()->SetWorking(false);
        if (ret == XIbad)
            Errs()->add_error("FAILED: internal error.");
        else if (ret == XIintr)
            Errs()->add_error("Interrupt, aborted.");
        return (false);
    }
    XM()->ShowParameters();
    DSPpkg::self()->SetWorking(false);

    Ulist()->CommitChanges(true);
    return (true);
}


// Command to join selected objects into polygons.
//
bool
cEdit::joinCmd()
{
    CDs *cursd = CurCell();
    if (!cursd) {
        Errs()->add_error("No current cell!");
        return (false);
    }
    bool incl_wires = DSP()->CurMode() == Physical && Zlist::JoinSplitWires;
    if (Selections.hasTypes(cursd, incl_wires ? "bpw" : "bp")) {
        EV()->InitCallback();
        Ulist()->ListCheck("JOIN", cursd, true);
        XIrt ret = ED()->joinQueue();
        if (ret == XIbad) {
            Errs()->add_error("FAILED: internal error.");
            return (false);
        }
        if (ret == XIintr) {
            Errs()->add_error("Interrupt, aborted.");
            return (false);
        }
        BBox BB1;
        Selections.computeBB(cursd, &BB1, false);
        Selections.removeTypes(cursd, "bpw");
        Ulist()->CommitChanges(true);
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0)
            wdesc->Refresh(&BB1);
        return (true);
    }
    Errs()->add_error("No joinable objects are selected.");
    return (false);
}


namespace {
    // Remove and return objects on the same layer as the first object.
    //
    CDol *filter_firstlayer(CDol **olp)
    {
        if (!olp || !*olp)
            return (0);
        CDl *ld = (*olp)->odesc->ldesc();
        CDol *o0 = *olp;
        CDol *oe = o0;;
        *olp = o0->next;
        o0->next = 0;

        CDol *op = 0, *on;
        for (CDol *ol = *olp; ol; ol = on) {
            on = ol->next;
            if (ol->odesc->ldesc() == ld) {
                ol->next = 0;
                oe->next = ol;
                oe = oe->next;
                if (op)
                    op->next = on;
                else
                    *olp = on;
                continue;
            }
            op = ol;
        }
        return (o0);
    }
}


// Join overlapping objects in the queue into single polygons.
//
XIrt
cEdit::joinQueue()
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (XIbad);
    DSPpkg::self()->SetWorking(true);
    PL()->ShowPrompt("Working...");
    bool incl_wires = DSP()->CurMode() == Physical && Zlist::JoinSplitWires;

    CDol *olst = Selections.listQueue(cursd, "bpw", true);
    while (olst) {
        CDol *o0 = filter_firstlayer(&olst);
        if (!o0)
            break;
        CDl *ld = o0->odesc->ldesc();
        j_list *j0 = 0, *je = 0;

        // Merge object on visible, selectable, layers only.  We allow
        // operations on NoMerge layers here, presumably the user,
        // having selected the objects, seriously wants them joined.

        if (ld->isSelectable()) {
            for (CDol *ol = o0; ol; ol = ol->next) {
                CDo *od = ol->odesc;
                if (!od->is_normal())
                    continue;
                if (od->state() != CDobjSelected)
                    continue;
                if (od->type() == CDWIRE) {
                    if (!incl_wires)
                        continue;
                    if (((CDw*)od)->wire_width() == 0)
                        continue;
                }
                if (!j0)
                    j0 = je = new j_list(od);
                else {
                    je->next = new j_list(od);
                    je = je->next;
                }
            }
        }
        CDol::destroy(o0);
        if (j0) {
            XIrt ret = j_list::join(j0, cursd, ld, true);
            if (ret != XIok) {
                XM()->ShowParameters();
                DSPpkg::self()->SetWorking(false);
                CDol::destroy(olst);
                return (ret);
            }
        }
    }

    PL()->ErasePrompt();
    XM()->ShowParameters();
    DSPpkg::self()->SetWorking(false);
    return (XIok);
}


// Take the most recently selected wire from the selection list. 
// Recursively attempt to merge this wire with similar wires that
// share an endpoint.
//
bool
cEdit::joinWireCmd()
{
    CDs *cursd = CurCell();
    if (!cursd) {
        Errs()->add_error("No current cell!");
        return (false);
    }

    CDw *wdesc = 0;
    CDol *olst = Selections.listQueue(cursd, "w", true);
    for (CDol *o = olst; o; o = o->next) {
        if (o->odesc->type() == CDWIRE) {
            wdesc = (CDw*)o->odesc;
            break;
        }
    }
    CDol::destroy(olst);

    if (!wdesc) {
        Errs()->add_error("No wire is selected.");
        return (false);
    }
    if (!wdesc->is_normal()) {
        Errs()->add_error("Wire is abnormal, can't merge (internal error).");
        return (false);
    }
    if (cursd->isElectrical() && wdesc->has_label()) {
        Errs()->add_error("Can't merge a wire with an associated label.");
        return (false);
    }
    if (wdesc->has_flag(CDnoMerge)) {
        Errs()->add_error("Wire has the NoMerge flag set, can't merge.");
        return (false);
    }

    // Back up the NoMerge and layer NoMerge flags, we override these
    // here.

    bool no_merge = CD()->IsNoMergeObjects();
    CD()->SetNoMergeObjects(false);

    CDl *ld = wdesc->ldesc();
    bool l_no_merge = ld->isNoMerge();
    ld->setNoMerge(false);

    EV()->InitCallback();
    Ulist()->ListCheck("MERGW", cursd, true);

    bool didmrg = false;
    bool ret = true;
    while (wdesc) {
        CDw *neww;
        if (!cursd->mergeWire(wdesc, true, &neww)) {
            Errs()->add_error("mergeWire: wire creation failed.");
            ret = false;
            break;
        }
        if (neww)
            didmrg = true;
        wdesc = neww;
    }
    ld->setNoMerge(l_no_merge);
    CD()->SetNoMergeObjects(no_merge);

    if (ret) {
        if (didmrg)
            PL()->ShowPrompt("Done, selected wire was merged.");
        else
            PL()->ShowPrompt("Done, selected wire could not be merged.");
    }
    XM()->ShowParameters();

    BBox BB1;
    Selections.computeBB(cursd, &BB1, false);
    Selections.removeTypes(cursd, "w");
    Ulist()->CommitChanges(true);
    WindowDesc *wd;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wd = wgen.next()) != 0)
        wd->Refresh(&BB1);
    return (ret);
}


// Join all wires on the current layer that share an endpoint.
//
bool
cEdit::joinWireLyrCmd()
{
    CDs *cursd = CurCell();
    if (!cursd) {
        Errs()->add_error("No current cell!");
        return (false);
    }

    CDl *ld = LT()->CurLayer();
    if (!ld) {
        Errs()->add_error("No current layer!");
        return (false);
    }
    if (ld->isInvisible()) {
        Errs()->add_error("Current layer is invisible, join not allowed.");
        return (false);
    }
    if (ld->isNoMerge()) {
        Errs()->add_error("Current layer has NoMerge set, join not allowed.");
        return (false);
    }

    // Make a list of all joinable wires on the current layer.
    CDol *ol0 = 0;
    CDg gdesc;
    gdesc.init_gen(cursd, ld);
    CDo *odesc;
    while ((odesc = gdesc.next()) != 0) {
        if (odesc->type() == CDWIRE) {
            CDw *wdesc = (CDw*)odesc;
            if (cursd->isElectrical() && wdesc->has_label())
                continue;
            if (wdesc->has_flag(CDnoMerge))
                continue;
            ol0 = new CDol(wdesc, ol0);
        }
    }
    if (!ol0) {
        Errs()->add_error("No joinable wire found on current layer.");
        return (false);
    }

    EV()->InitCallback();
    Ulist()->ListCheck("MERGW", cursd, true);

    int mcnt = 0;
    bool ret = true;
    while (ol0) {
        CDw *wdesc = (CDw*)ol0->odesc;
        CDol *ox = ol0;
        ol0 = ol0->next;
        delete ox;

        // Skip the deleted wires, these are produced as join
        // operations are performed.  Newly created wires can be
        // ignored, as we know that no further joining is possible.

        if (!wdesc->is_normal())
            continue;
        bool didmrg = 0;
        while (wdesc) {
            CDw *neww;
            if (!cursd->mergeWire(wdesc, true, &neww)) {
                Errs()->add_error("mergeWire: wire creation failed.");
                ret = false;
                break;
            }
            if (neww)
                didmrg = true;
            wdesc = neww;
        }
        if (didmrg)
            mcnt++;
    }

    if (ret) {
        PL()->ShowPromptV("Done, %d %s merged on current layer.",
            mcnt, mcnt == 1 ? "wire" : "wires");
    }
    XM()->ShowParameters();

    Ulist()->CommitChanges(true);
    WindowDesc *wd;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wd = wgen.next()) != 0)
        wd->Redisplay(0);
    return (ret);
}


// Command to decompose objects into trapezoids, favoring vertical
// orientation if vert is true.
//
bool
cEdit::splitCmd(bool vert)
{
    CDs *cursd = CurCell();
    if (!cursd) {
        Errs()->add_error("No current cell!");
        return (false);
    }
    bool incl_wires = DSP()->CurMode() == Physical && Zlist::JoinSplitWires;
    if (Selections.hasTypes(cursd, incl_wires ? "pw" : "p")) {
        EV()->InitCallback();
        Ulist()->ListCheck("SPLIT", cursd, true);
        ED()->splitQueue(vert);
        BBox BB1;
        Selections.computeBB(cursd, &BB1, false);
        Selections.removeTypes(cursd, "bpw");
        Ulist()->CommitChanges(true);
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0)
            wdesc->Refresh(&BB1);
        return (true);
    }
    if (Selections.hasTypes(cursd, "b")) {
        // Boxes are a no-op, if only boxes are selected, deselect
        // them and continue without comment.

        Selections.deselectTypes(cursd, "b");
        return (true);
    }
    Errs()->add_error("No splitable objects are selected.");
    return (false);
}


// Split polygons and wires into boxes and four-sided polygons.
//
XIrt
cEdit::splitQueue(bool vert)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (XIbad);

    bool incl_wires = DSP()->CurMode() == Physical && Zlist::JoinSplitWires;
    sSelGen sg(Selections, cursd, "pw");
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (od->type() == CDWIRE) {
            if (!incl_wires)
                continue;
            if (((CDw*)od)->wire_width() == 0)
                continue;
        }
        if (!od->is_normal())
            continue;
        Zlist *zl = vert ? od->toZlistR() : od->toZlist();
        for (Zlist *z = zl; z; z = z->next) {
            if (z->Z.xll == z->Z.xul && z->Z.xlr == z->Z.xur) {
                if (vert)
                    cursd->newBox(0, z->Z.yl, -z->Z.xur,
                        z->Z.yu, -z->Z.xll, od->ldesc(), 0);
                else
                    cursd->newBox(0, z->Z.xll, z->Z.yl,
                        z->Z.xur, z->Z.yu, od->ldesc(), 0);
            }
            else {
                Poly po;
                if (z->Z.mkpoly(&po.points, &po.numpts, vert))
                    cursd->newPoly(0, &po, od->ldesc(), 0, false);
            }
        }
        Zlist::destroy(zl);
        Ulist()->RecordObjectChange(cursd, od, 0);
    }
    return (XIok);
}

