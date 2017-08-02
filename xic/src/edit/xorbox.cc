
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

#include "config.h"
#include "main.h"
#include "edit.h"
#include "undolist.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "geo_zlist.h"
#include "events.h"
#include "layertab.h"
#include "menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "ghost.h"


// This file supports a command which adds boxes to the database
// such as to invert a region on a layer.  Previously existing
// objects of the same layer which intersect the added box are deleted,
// and become clear areas in the added box.

namespace {
    namespace ed_xorbox {
        struct XorState : public CmdState
        {
            XorState(const char*, const char*);
            virtual ~XorState();

            void setCaller(GRobject c)  { Caller = c; }

            void setup();
            void b1down();
            void b1up();
            void esc();
            void undo();
            void redo();
            void message() { if (Level == 1) PL()->ShowPrompt(msg1);
                else PL()->ShowPrompt(msg2); }

        private:
            void SetLevel1(bool show) { Level = 1; if (show) message(); }
            void SetLevel2() { Level = 2; message(); }

            GRobject Caller;
            int State;
            int Refx, Refy;

            static const char *msg1;
            static const char *msg2;
        };

        XorState *XorCmd;
    }
}

using namespace ed_xorbox;

const char *XorState::msg1 =
    "Click twice or drag to define rectangular area to xor.";
const char *XorState::msg2 = "Click on second diagonal endpoint.";


void
cEdit::makeXORboxExec(CmdDesc *cmd)
{
    if (XorCmd) {
        XorCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;

    XorCmd = new XorState("XOR", "xic:xor");
    XorCmd->setCaller(cmd ? cmd->caller : 0);
    Ulist()->ListCheck(XorCmd->Name(), CurCell(), false);
    if (!EV()->PushCallback(XorCmd)) {
        delete XorCmd;
        return;
    }
    ds.clear();
    XorCmd->setup();
}


XorState::XorState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    State = 0;
    Refx = Refy = 0;
    DSP()->SetInEdgeSnappingCmd(true);

    SetLevel1(false);
}


XorState::~XorState()
{
    XorCmd = 0;
    DSP()->SetInEdgeSnappingCmd(false);
}


void
XorState::setup()
{
    if (Exported) {
        // Accept the exported coordinate as the anchor point, if given.
        Refx = ExportX;
        Refy = ExportY;
        State = 1;
        XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
        Gst()->SetGhostAt(GFbox, Refx, Refy);
        SetLevel2();
        Exported = false;
    }
    else
        message();
}


void
XorState::b1down()
{
    if (Level == 1) {
        EV()->Cursor().get_xy(&Refx, &Refy);
        State = 1;
        XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
        Gst()->SetGhostAt(GFbox, Refx, Refy);
        EV()->DownTimer(GFbox);
    }
    else {
        int xc, yc;
        EV()->Cursor().get_xy(&xc, &yc);
        if (ED()->xorArea(xc, yc, Refx, Refy)) {
            Gst()->SetGhost(GFnone);
            Ulist()->CommitChanges(true);
            State = 3;
        }
    }
}


void
XorState::b1up()
{
    if (Level == 1) {
        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok() &&
                EV()->CurrentWin()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            if (x != Refx && y != Refy) {
                if (ED()->xorArea(x, y, Refx, Refy)) {
                    Gst()->SetGhost(GFnone);
                    Ulist()->CommitChanges(true);
                    State = 2;
                    return;
                }
            }
        }
        SetLevel2();
    }
    else {
        if (State == 3) {
            State = 2;
            SetLevel1(true);
        }
    }
}


void
XorState::esc()
{
    if (EnableExport && Level == 2) {
        // Export the last anchor point, the next command may use this.
        Exported = true;
        ExportX = Refx;
        ExportY = Refy;
    }

    Gst()->SetGhost(GFnone);
    XM()->SetCoordMode(CO_ABSOLUTE);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


void
XorState::undo()
{
    if (Level == 1) {
        Ulist()->UndoOperation();
        if (State == 2) {
            XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
            Gst()->SetGhostAt(GFbox, Refx, Refy);
            SetLevel2();
        }
    }
    else {
        Gst()->SetGhost(GFnone);
        XM()->SetCoordMode(CO_ABSOLUTE);
        if (State == 2)
            State = 3;
        SetLevel1(true);
    }
}


void
XorState::redo()
{
    if (Level == 1) {
        if ((State == 1 && !Ulist()->HasRedo()) ||
                (State == 3 && Ulist()->HasOneRedo())) {
            XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
            Gst()->SetGhostAt(GFbox, Refx, Refy);
            if (State == 3)
                State = 2;
            SetLevel2();
        }
        else
            Ulist()->RedoOperation();
    }
    else {
        if (State == 2) {
            Gst()->SetGhost(GFnone);
            XM()->SetCoordMode(CO_ABSOLUTE);
            Ulist()->RedoOperation();
            SetLevel1(true);
        }
    }
}
// End of XorState functions.


bool
cEdit::xorArea(int x1, int y1, int x2, int y2)
{
    if (DSP()->CurMode() != Physical)
        return (false);
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);
    BBox AOI(x1, y1, x2, y2);
    AOI.fix();
    XM()->SetCoordMode(CO_ABSOLUTE);
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return (false);
    if (ld->isInvisible()) {
        Log()->ErrorLog(mh::ObjectCreation,
            "Creation of box on invisible layer not allowed.");
        return (true);
    }

    // conditionally delete overlapping objects, store in delete list
    CDg gdesc;
    gdesc.init_gen(cursd, ld, &AOI);
    CDo *pointer;
    while ((pointer = gdesc.next()) != 0) {
        // ignore selected objects
        if (pointer->state() == CDSelected)
            continue;
        // nonzero overlap area only
        if (pointer->oBB().right <= AOI.left ||
                pointer->oBB().left >= AOI.right ||
                pointer->oBB().top <= AOI.bottom ||
                pointer->oBB().bottom >= AOI.top)
            continue;
        if (pointer->type() == CDBOX)
            Ulist()->RecordObjectChange(cursd, pointer, 0);
        else if (pointer->type() == CDWIRE) {
            if (((const CDw*)pointer)->wire_width() > 0 &&
                    ((const CDw*)pointer)->w_intersect(&AOI, false))
                Ulist()->RecordObjectChange(cursd, pointer, 0);
        }
        else if (pointer->type() == CDPOLYGON) {
            if (((const CDpo*)pointer)->po_intersect(&AOI, false))
                Ulist()->RecordObjectChange(cursd, pointer, 0);
        }
    }

    // convert wires to polys
    for (Ochg *oc = Ulist()->CurOp().obj_list(); oc; oc = oc->next_chg()) {
        if (oc->odel() && oc->odel()->type() == CDWIRE) {
            CDw *owire = (CDw*)oc->odel();
            Poly poly;
            if (((const CDw*)owire)->w_toPoly(&poly.points, &poly.numpts)) {
                CDpo *newp = cursd->newPoly(0, &poly, owire->ldesc(), 0,
                    false);
                if (newp)
                    Ulist()->RecordObjectChange(cursd, newp, 0);
            }
        }
    }

    // Now the good stuff
    Errs()->init_error();
    Zlist *z0 = 0;
    for (Ochg *oc = Ulist()->CurOp().obj_list(); oc; oc = oc->next_chg()) {
        if (!oc->odel())
            continue;
        if (oc->odel()->type() == CDBOX) {
            Zoid Z(&oc->odel()->oBB());
            Zlist *z = new Zlist(&Z);
            z->next = z0;
            z0 = z;
        }
        else if (oc->odel()->type() == CDPOLYGON) {
            Zlist *z = oc->odel()->toZlist();
            if (z) {
                Zlist *ze = z;
                while (ze->next)
                    ze = ze->next;
                ze->next = z0;
                z0 = z;
            }
        }
    }
    if (!z0) {
        // Nothing intersecting, just make a box.
        CDo *newb = cursd->newBox(0, &AOI, ld, 0);
        if (!newb) {
            Errs()->add_error("newBox failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        }
        else if (!cursd->mergeBoxOrPoly(newb, true)) {
            Errs()->add_error("mergeBoxOrPoly failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        }
        return (true);
    }

    Zoid Z(&AOI);
    Zlist::zl_and(&z0, &Z);
    z0 = Zlist::filter_slivers(z0, 0);
    if (!z0)
        return (false);
    Zlist zt(&Z);

    Zlist *zinv = Zlist::copy(&zt);
    XIrt ret = Zlist::zl_andnot(&zinv, z0);
    if (ret != XIok) {
        Errs()->add_error("Error or interrupt during inversion.");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
    }
    else {
        PolyList *p0 = Zlist::to_poly_list(zinv);

        for (PolyList *p = p0; p; p = p->next) {
            if (p->po.is_rect()) {
                BBox BB(p->po.points);
                CDo *newo = cursd->newBox(0, &BB, ld, 0);
                if (!newo) {
                    Errs()->add_error("newBox failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
                else if (!cursd->mergeBoxOrPoly(newo, true)) {
                    Errs()->add_error("mergeBoxOrPoly failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
            }
            else {
                CDpo *newo = cursd->newPoly(0, &p->po, ld, 0, true);
                if (!newo) {
                    Errs()->add_error("newPoly failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
                else if (!cursd->mergeBoxOrPoly(newo, true)) {
                    Errs()->add_error("mergeBoxOrPoly failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
            }
        }
        PolyList::destroy(p0);
    }

    // replace the part of the object outside of the AOI
    for (Ochg *oc = Ulist()->CurOp().obj_list(); oc; oc = oc->next_chg()) {
        if (oc->odel())
            ED()->process_xor(oc->odel(), cursd, &AOI);
    }
    return (true);
}


// Private implementation function.
//
void
cEdit::process_xor(CDo *odesc, CDs *sdesc, BBox *AOI)
{
    if (!odesc)
        return;
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
    {
        Errs()->init_error();
        BBox BB = odesc->oBB();
        if (BB.top > AOI->top) {
            if (!sdesc->newBox(0, BB.left, AOI->top, BB.right, BB.top,
                    odesc->ldesc(), 0)) {
                Errs()->add_error("newBox failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
            BB.top = AOI->top;
        }
        if (BB.bottom < AOI->bottom) {
            if (!sdesc->newBox(0, BB.left, BB.bottom, BB.right, AOI->bottom,
                    odesc->ldesc(), 0)) {
                Errs()->add_error("newBox failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
            BB.bottom = AOI->bottom;
        }
        if (BB.left < AOI->left) {
            if (!sdesc->newBox(0, BB.left, BB.bottom, AOI->left, BB.top,
                    odesc->ldesc(), 0)) {
                Errs()->add_error("newBox failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
        }
        if (BB.right > AOI->right) {
            if (!sdesc->newBox(0, AOI->right, BB.bottom, BB.right, BB.top,
                    odesc->ldesc(), 0)) {
                Errs()->add_error("newBox failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
        }
        return;
    }
poly:
    {
        Errs()->init_error();
        BBox BB;
        if (odesc->oBB().top > AOI->top) {
            BB.left = odesc->oBB().left;
            BB.bottom = AOI->top;
            BB.right = odesc->oBB().right;
            BB.top = odesc->oBB().top;
            PolyList *p0 = ((const CDpo*)odesc)->po_clip(&BB);
            for (PolyList *p = p0; p; p = p->next) {
                if (!sdesc->newPoly(0, &p->po, odesc->ldesc(), 0, false)) {
                    Errs()->add_error("newPoly failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
            }
            PolyList::destroy(p0);
        }
        if (odesc->oBB().bottom < AOI->bottom) {
            BB.left = odesc->oBB().left;
            BB.bottom = odesc->oBB().bottom;
            BB.right = odesc->oBB().right;
            BB.top = AOI->bottom;
            PolyList *p0 = ((const CDpo*)odesc)->po_clip(&BB);
            for (PolyList *p = p0; p; p = p->next) {
                if (!sdesc->newPoly(0, &p->po, odesc->ldesc(), 0, false)) {
                    Errs()->add_error("newPoly failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
            }
            PolyList::destroy(p0);
        }
        if (odesc->oBB().left < AOI->left) {
            BB.left = odesc->oBB().left;
            BB.bottom = AOI->bottom;
            BB.right = AOI->left;
            BB.top = AOI->top;
            PolyList *p0 = ((const CDpo*)odesc)->po_clip(&BB);
            for (PolyList *p = p0; p; p = p->next) {
                if (!sdesc->newPoly(0, &p->po, odesc->ldesc(), 0, false)) {
                    Errs()->add_error("newPoly failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
            }
            PolyList::destroy(p0);
        }
        if (odesc->oBB().right > AOI->right) {
            BB.left = AOI->right;
            BB.bottom = AOI->bottom;
            BB.right = odesc->oBB().right;
            BB.top = AOI->top;
            PolyList *p0 = ((const CDpo*)odesc)->po_clip(&BB);
            for (PolyList *p = p0; p; p = p->next) {
                if (!sdesc->newPoly(0, &p->po, odesc->ldesc(), 0, false)) {
                    Errs()->add_error("newPoly failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
            }
            PolyList::destroy(p0);
        }
        return;
    }
wire:
label:
inst:
    return;
}

