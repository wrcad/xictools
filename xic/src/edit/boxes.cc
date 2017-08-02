
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

#include "main.h"
#include "edit.h"
#include "undolist.h"
#include "extif.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "events.h"
#include "layertab.h"
#include "menu.h"
#include "select.h"
#include "promptline.h"
#include "errorlog.h"
#include "ghost.h"
#include "pathlist.h"


//
// Menu command to create rectangular objects
//

namespace {
    namespace ed_boxes {
        struct BoxState : public CmdState
        {
            BoxState(const char*, const char*);
            virtual ~BoxState();

            void setCaller(GRobject c)  { Caller = c; }

            void setup();
            void b1down();
            void b1up();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();

        private:
            bool makebox(int, int);
            void message() { if (Level == 1) PL()->ShowPrompt(msg1);
                else PL()->ShowPrompt(msg2); }
            void SetLevel1(bool show)
                { Level = 1; First = true; if (show) message(); }
            void SetLevel2() { Level = 2; First = false; message(); }

            GRobject Caller;  // calling button
            int State;        // internal state
            bool First;       // true before first point accepted
            int Lastx;        // reference point
            int Lasty;

            static const char *msg1;
            static const char *msg2;
        };

        BoxState *BoxCmd;
    }
}

using namespace ed_boxes;

const char *BoxState::msg1 = "Click twice or drag to define rectangle.";
const char *BoxState::msg2 = "Click on second diagonal endpoint.";


void
cEdit::makeBoxesExec(CmdDesc *cmd)
{
    if (BoxCmd) {
        BoxCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;

    BoxCmd = new BoxState("BOX", "xic:box");
    BoxCmd->setCaller(cmd ? cmd->caller : 0);
    Ulist()->ListCheck(BoxCmd->Name(), CurCell(), false);
    if (!EV()->PushCallback(BoxCmd)) {
        delete BoxCmd;
        return;
    }
    ds.clear();
    BoxCmd->setup();
}


BoxState::BoxState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    State = 0;
    Lastx = Lasty = 0;
    DSP()->SetInEdgeSnappingCmd(true);

    SetLevel1(false);
}


BoxState::~BoxState()
{
    BoxCmd = 0;
    DSP()->SetInEdgeSnappingCmd(false);
}


void
BoxState::setup()
{
    if (Exported) {
        // Accept the exported coordinate as the anchor point, if given.
        Lastx = ExportX;
        Lasty = ExportY;
        State = 1;
        XM()->SetCoordMode(CO_RELATIVE, Lastx, Lasty);
        Gst()->SetGhostAt(GFbox, Lastx, Lasty);
        SetLevel2();
        Exported = false;
    }
    else
        message();
}


// Button 1 press handler.
//
void
BoxState::b1down()
{
    if (Level == 1) {
        // Anchor corner of box, or if Control is pressed, paint the smallest
        // cell pointed to.
        //
        EV()->Cursor().get_xy(&Lastx, &Lasty);

        State = 0;
        if ((EV()->Cursor().get_downstate() & GR_CONTROL_MASK) &&
                !(EV()->Cursor().get_downstate() & GR_SHIFT_MASK) &&
                DSP()->CurMode() == Physical) {
            BBox AOI;
            AOI.left = AOI.right = Lastx;
            AOI.bottom = AOI.top = Lasty;
            char types[2];
            types[0] = CDINSTANCE;
            types[1] = '\0';
            CDol *slist = Selections.selectItems(CurCell(), types, &AOI,
                PSELpoint);
            if (!slist)
                return;
            CDol *slp = slist;
            if (slist->next) {
                // find the entry with least area
                double a, amin;
                CDol *sl = slist;
                amin = sl->odesc->oBB().width();
                amin *= sl->odesc->oBB().height();
                for (sl = sl->next; sl; sl = sl->next) {
                    a = sl->odesc->oBB().width();
                    a *= sl->odesc->oBB().height();
                    if (a < amin) {
                        amin = a;
                        slp = sl;
                    }
                }
            }
            Lastx = slp->odesc->oBB().left;
            Lasty = slp->odesc->oBB().bottom;
            makebox(slp->odesc->oBB().right, slp->odesc->oBB().top);
            CDol::destroy(slist);
            return;
        }
        State = 1;
        XM()->SetCoordMode(CO_RELATIVE, Lastx, Lasty);
        Gst()->SetGhostAt(GFbox, Lastx, Lasty);
        EV()->DownTimer(GFbox);
    }
    else {
        // Create the box.
        //
        int xc, yc;
        EV()->Cursor().get_xy(&xc, &yc);
        if (makebox(xc, yc))
            State = 3;
    }
}


// Button 1 release handler.
//
void
BoxState::b1up()
{
    if (Level == 1) {
        // Create the box, if the pointer has moved, and the box has area.
        // Otherwise set the state for next button 1 press.
        //
        if (!State)
            return;
        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok() &&
                EV()->CurrentWin()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            if (Lastx != x && Lasty != y) {
                if (makebox(x, y)) {
                    State = 2;
                    return;
                }
            }
        }
        SetLevel2();
    }
    else {
        // If a box was created, reset the state for the next box.
        //
        if (State == 3) {
            State = 2;
            SetLevel1(true);
        }
    }
}


// Esc entered, clean up and abort.
//
void
BoxState::esc()
{
    if (EnableExport && Level == 2) {
        // Export the last anchor point, the next command may use this.
        Exported = true;
        ExportX = Lastx;
        ExportY = Lasty;
    }

    Gst()->SetGhost(GFnone);
    XM()->SetCoordMode(CO_ABSOLUTE);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    if (Caller)
        Menu()->Deselect(Caller);
    delete this;
}


// If Control pressed and pointing at a subcell, cover the subcell
// with the current layer.
//
bool
BoxState::key(int code, const char*, int)
{
    switch (code) {
    case CTRLDN_KEY:
        if (DSP()->CurMode() == Physical && First)
            PL()->ShowPrompt("Paint subcells");
        break;
    case CTRLUP_KEY:
        if (DSP()->CurMode() == Physical && First) {
            // Avoid pop-up.
            if (!EV()->InCoordEntry())
                PL()->ShowPrompt(msg1);
        }
        break;
    default:
        return (false);
    }
    return (true);
}


// Undo handler.
//
void
BoxState::undo()
{
    if (Level == 1) {
        Ulist()->UndoOperation();
        if (State == 2) {
            XM()->SetCoordMode(CO_RELATIVE, Lastx, Lasty);
            Gst()->SetGhostAt(GFbox, Lastx, Lasty);
            SetLevel2();
        }
    }
    else {
        // Undo the corner anchor.
        //
        Gst()->SetGhost(GFnone);
        XM()->SetCoordMode(CO_ABSOLUTE);
        if (State == 2)
            State = 3;
        SetLevel1(true);
    }
}


// Redo handler.
//
void
BoxState::redo()
{
    if (Level == 1) {
        // Redo undone corner anchor or operation.
        //
        if ((State == 1 && !Ulist()->HasRedo()) ||
                (State == 3 && Ulist()->HasOneRedo())) {
            XM()->SetCoordMode(CO_RELATIVE, Lastx, Lasty);
            Gst()->SetGhostAt(GFbox, Lastx, Lasty);
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


// Actually create the box.  Returns true on success.
//
bool
BoxState::makebox(int x, int y)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return (false);
    if (ld->isInvisible()) {
        Log()->ErrorLog(mh::ObjectCreation,
            "Creation of box on invisible layer not allowed.");
        Gst()->SetGhost(GFnone);
        XM()->SetCoordMode(CO_ABSOLUTE);
        return (true);
    }
    BBox BB(x, y, Lastx, Lasty);
    BB.fix();
    if (BB.width() == 0 || BB.height() == 0)
        return (false);
    CDo *newbox = cursd->newBox(0, &BB, ld, 0);
    if (!newbox) {
        Errs()->add_error("newBox failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        return (false);
    }
    if (!cursd->mergeBoxOrPoly(newbox, true)) {
        Errs()->add_error("mergeBoxOrPoly failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
    }
    Gst()->SetGhost(GFnone);
    XM()->SetCoordMode(CO_ABSOLUTE);
    Ulist()->CommitChanges(true);
    return (true);
}
// End of BoxState functions

