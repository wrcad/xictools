
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
#include "scedif.h"
#include "undolist.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "cd_layer.h"
#include "events.h"
#include "layertab.h"
#include "menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "ghost.h"
#include "miscutil/texttf.h"


//
// The Move command.
//

namespace {
    namespace ed_move {
        struct MoveState : public CmdState
        {
            MoveState(const char*, const char*);
            virtual ~MoveState();

            void setCaller(GRobject c)  { Caller = c; }

            void b1down();
            void b1up();
            void desel();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();

            void message() { if (Level == 1) PL()->ShowPrompt(mmsg1);
                else if (Level == 2) PL()->ShowPrompt(mmsg2);
                else PL()->ShowPrompt(mmsg3); }

        private:
            void SetLevel1(bool show) { Level = 1; if (show) message(); }
            void SetLevel2(bool show) { Level = 2; if (show) message(); }
            void SetLevel3() { Level = 3; message(); }

            static int MsgTimeout(void*);

            GRobject Caller;
            bool GotOne;
            bool GhostOn;
            int State;
            int Refx, Refy;
            int Orgx, Orgy;
            int Corner;
            CDol *OrigObjs;

            static const char *mmsg1;
            static const char *mmsg2;
            static const char *mmsg3;
        };

        MoveState *MoveCmd;
    }
}

using namespace ed_move;

const char *MoveState::mmsg1 = "Select objects to move.";
const char *MoveState::mmsg2 =
    "Click or drag, button-down is the reference point.";
const char *MoveState::mmsg3 =
    "Click on location where the selected items will be moved.";


// Menu Command to move objects.
//
void
cEdit::moveExec(CmdDesc *cmd)
{
    if (MoveCmd) {
        MoveCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurCell(true, false, DSP()->CurMode()))
        return;

    MoveCmd = new MoveState("MOVE", "xic:move");
    MoveCmd->setCaller(cmd ? cmd->caller : 0);
    // Original objects are selected after undo.
    Ulist()->ListCheck(MoveCmd->Name(), CurCell(), true);
    if (!EV()->PushCallback(MoveCmd)) {
        delete MoveCmd;
        return;
    }
    MoveCmd->message();
    ed_move_or_copy = CDmove;
    ds.clear();
}


MoveState::MoveState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    GotOne = Selections.hasTypes(CurCell(), 0);
    GhostOn = false;
    State = 0;
    Refx = Refy = 0;
    Orgx = Orgy = 0;
    Corner = 0;
    OrigObjs = 0;
    if (GotOne)
        Level = 2;
    else
        Level = 1;
}


MoveState::~MoveState()
{
    MoveCmd = 0;
    CDol::destroy(OrigObjs);
}


void
MoveState::b1down()
{
    if (Level == 1)
        cEventHdlr::sel_b1down();
    else if (Level == 2) {
        EV()->SetConstrained(
            (EV()->Cursor().get_downstate() & GR_CONTROL_MASK) ||
            (EV()->Cursor().get_downstate() & GR_SHIFT_MASK));
        EV()->Cursor().get_xy(&Refx, &Refy);
        XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
        Corner = 0;
        State = 2;
        GhostOn = true;
        Gst()->SetGhostAt(GFmove, Refx, Refy);
        ED()->setPressLayer(LT()->CurLayer());
        EV()->DownTimer(GFmove);
    }
    else if (Level == 3) {
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        ED()->findContact(&x, &y, Refx, Refy, false);

        CDl *ldold = 0, *ldnew = 0;
        if (LT()->CurLayer() != ED()->pressLayer()) {
            // Current layer changed.
            if (ED()->getLchgMode() == LCHGcur) {
                // Map old current to new.
                ldnew = LT()->CurLayer();
                ldold = ED()->pressLayer();
            }
            else if (ED()->getLchgMode() == LCHGall) {
                // Map all layers to new.
                ldnew = LT()->CurLayer();
                ldold = 0;
            }
        }
        if (ED()->moveQueue(Refx, Refy, x, y, ldold, ldnew)) {
            Gst()->SetGhost(GFnone);
            GhostOn = false;
            XM()->SetCoordMode(CO_ABSOLUTE);
            Ulist()->CommitChanges(true);
            State = 4;
        }
    }
}


void
MoveState::b1up()
{
    if (Level == 1) {
        if (!cEventHdlr::sel_b1up(0, 0, 0))
            return;
        if (!Selections.hasTypes(CurCell(), 0))
            return;
        SetLevel2(true);
        State = 1;
    }
    else if (Level == 2) {
        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok() &&
                EV()->CurrentWin()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            ED()->findContact(&x, &y, Refx, Refy, false);
            if (ED()->moveQueue(Refx, Refy, x, y, 0, 0)) {
                Gst()->SetGhost(GFnone);
                GhostOn = false;
                XM()->SetCoordMode(CO_ABSOLUTE);
                State = 3;
                Ulist()->CommitChanges(true);
                XM()->SetCoordMode(CO_ABSOLUTE);
                SetLevel1(true);
                return;
            }
        }
        SetLevel3();
    }
    else if (Level == 3) {
        if (State == 4) {
            State = 3;
            SetLevel1(true);
        }
    }
}


// Desel pressed, reset.
//
void
MoveState::desel()
{
    if (Level == 1)
        cEventHdlr::sel_esc();
    else {
        Gst()->SetGhost(GFnone);
        XM()->SetCoordMode(CO_ABSOLUTE);
    }
    GotOne = false;
    State = 0;
    SetLevel1(true);
}


void
MoveState::esc()
{
    if (Level == 1)
        cEventHdlr::sel_esc();
    else {
        Gst()->SetGhost(GFnone);
        if (!GotOne) {
            Selections.deselectTypes(CurCell(), 0);
            Selections.removeTypes(CurCell(), 0);
        }
        XM()->SetCoordMode(CO_ABSOLUTE);
    }
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    EV()->SetConstrained(false);
    delete this;
}


bool
MoveState::key(int code, const char*, int)
{
    switch (code) {
    case SHIFTDN_KEY:
    case CTRLDN_KEY:
        if (GhostOn)
            Gst()->SetGhost(GFnone);
        EV()->SetConstrained(true);
        if (GhostOn) {
            Gst()->SetGhostAt(GFmove, Refx, Refy);
            Gst()->RepaintGhost();
        }
        return (true);
    case CTRLUP_KEY:
    case SHIFTUP_KEY:
        {
            unsigned int state = 0;
            DSPmainDraw(QueryPointer(0, 0, &state))
            if (!(state & GR_CONTROL_MASK) && !(state & GR_SHIFT_MASK)) {
                if (GhostOn)
                    Gst()->SetGhost(GFnone);
                EV()->SetConstrained(false);
                if (GhostOn) {
                    Gst()->SetGhostAt(GFmove, Refx, Refy);
                    Gst()->RepaintGhost();
                }
            }
            return (true);
        }
    case RETURN_KEY:
        if (GhostOn && DSP()->CurMode() == Physical) {
            BBox sBB;
            if (Selections.computeBB(CurCell(), &sBB, false)) {
                Gst()->SetGhost(GFnone);
                switch (Corner) {
                case 0:
                default:
                    Orgx = Refx;
                    Orgy = Refy;
                    Refx = sBB.left;
                    Refy = sBB.bottom;
                    Corner = 1;
                    break;
                case 1:
                    Refx = sBB.left;
                    Refy = sBB.top;
                    Corner = 2;
                    break;
                case 2:
                    Refx = sBB.right;
                    Refy = sBB.top;
                    Corner = 3;
                    break;
                case 3:
                    Refx = sBB.right;
                    Refy = sBB.bottom;
                    Corner = 4;
                    break;
                case 4:
                    Refx = Orgx;
                    Refy = Orgy;
                    Corner = 0;
                    break;
                }
                XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
                Gst()->SetGhostAt(GFmove, Refx, Refy);
                Gst()->RepaintGhost();
            }
            return (true);
        }
        break;

    case BSP_KEY:
    case DELETE_KEY:
        if (Level == 2) {
            // Revert to selection level so user can update selections.
            WindowDesc *wd = EV()->CurrentWin();
            if (!wd)
                wd = DSP()->MainWdesc();
            if (PL()->KeyPos(wd) == 0) {
                State = 0;
                SetLevel1(true);
                return (true);;
            }
        }
        break;
    }
    return (false);
}


void
MoveState::undo()
{
    if (Level == 1) {
        if (!cEventHdlr::sel_undo()) {
            // The undo system has placed the original objects into
            // the selection list, as selected.

            if (State == 3) {
                Gst()->SetGhostAt(GFmove, Refx, Refy);
                GhostOn = true;
                SetLevel3();
                return;
            }
            dspPkgIf()->RegisterTimeoutProc(1500, MsgTimeout, 0);
        }
    }
    else if (Level == 2) {
        CDol::destroy(OrigObjs);
        OrigObjs = Selections.listQueue(CurCell());
        Selections.deselectTypes(CurCell(), 0);
        Selections.removeTypes(CurCell(), 0);
        if (State == 3)
            State = 4;
        SetLevel1(true);
    }
    else if (Level == 3) {
        Gst()->SetGhost(GFnone);
        GhostOn = false;
        XM()->SetCoordMode(CO_ABSOLUTE);
        SetLevel2(true);
    }
}


void
MoveState::redo()
{
    if (Level == 1) {
        if (((State == 1 || State == 2) && !Ulist()->HasRedo()) ||
                (State == 4 && Ulist()->HasOneRedo())) {
            Selections.insertList(CurCell(), OrigObjs);
            Selections.setShowSelected(CurCell(), 0, true);
            if (State == 4)
                State = 3;
            SetLevel2(true);
        }
        else {
            cEventHdlr::sel_redo();
            dspPkgIf()->RegisterTimeoutProc(1500, MsgTimeout, 0);
        }
    }
    else if (Level == 2) {
        Gst()->SetGhostAt(GFmove, Refx, Refy);
        GhostOn = true;
        SetLevel3();
    }
    else if (Level == 3) {
        if (State == 3) {
            Gst()->SetGhost(GFnone);
            GhostOn = false;
            XM()->SetCoordMode(CO_ABSOLUTE);
            CDol::destroy(OrigObjs);
            OrigObjs = Selections.listQueue(CurCell());
            Selections.removeTypes(CurCell(), 0);
            Ulist()->RedoOperation();
            SetLevel1(true);
        }
    }
}


// Timer callback, reprint the command prompt after an undo/redo, which
// will display a message.
//
int
MoveState::MsgTimeout(void*)
{
    if (MoveCmd)
        MoveCmd->message();
    return (0);
}
// End of MoveState functions.


// This function is called by the front-end pointer handler to effect
// a move.
//
void
cEdit::moveObjects(int ref_x, int ref_y, int mov_x, int mov_y,
    CDl *ldold, CDl *ldnew)
{
    CDs *sdesc = CurCell();
    if (!sdesc)
        return;

    if (mov_x == ref_x && mov_y == ref_y && GEO()->curTx()->is_identity())
        return;
    findContact(&mov_x, &mov_y, ref_x, ref_y, false);
    // Original objects are selected after undo.
    Ulist()->ListCheck("move", sdesc, true);
    if (moveQueue(ref_x, ref_y, mov_x, mov_y, ldold, ldnew))
        Ulist()->CommitChanges(true);
}


// Perform the move of the objects in the current cell selection list. 
// New objects are placed in the add list, and moved objects are
// deleted from the selection list and placed in the delete list. 
// Take care of updating references to the objects altered.  If ldnew
// is non-nil, new objects will be on this layer if ldold is nil or
// matches the current object.
//
bool
cEdit::moveQueue(int ref_x, int ref_y, int new_x, int new_y,
    CDl *ldold, CDl *ldnew)
{
    CDs *sdesc = CurCell();
    if (!sdesc)
        return (0);

    if (EV()->IsConstrained())
        XM()->To45(ref_x, ref_y, &new_x, &new_y);

    if (ref_x == new_x && ref_y == new_y && GEO()->curTx()->is_identity())
        return (false);
    CDtf tfnew, tfold;
    cTfmStack stk;
    stk.TCurrent(&tfold);
    stk.TPush();
    GEO()->applyCurTransform(&stk, ref_x, ref_y, new_x, new_y);
    stk.TCurrent(&tfnew);
    stk.TPop();

    bool copied = false;
    if (DSP()->CurMode() == Electrical) {
        // Remove labels referenced from cells from queue.
        Selections.purgeLabels(sdesc);
    }

    CDol *st = Selections.listQueue(sdesc);
    CDmergeInhibit *inh = new CDmergeInhibit(st);
    CDol::destroy(st);

    if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
        // These might move
        DSP()->ShowCellTerminalMarks(ERASE);

    // sp_changed is set if a cell node is moved
    bool sp_changed = false;
    Errs()->init_error();
    int failcnt = 0;
    sSelGen sg(Selections, sdesc);
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (translate(od, sdesc, &tfold, &tfnew, ref_x, ref_y, new_x, new_y,
                ldold, ldnew, CDmove, &sp_changed)) {
            copied = true;
            sg.remove();
        }
        else
            failcnt++;
    }
    if (failcnt) {
        Errs()->add_error("Move failed for %d %s.", failcnt,
            failcnt == 1 ? "object" : "objects");
        Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
    }
    delete inh;
    if (sp_changed)
        sdesc->reflectTerminals();
    if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
        DSP()->ShowCellTerminalMarks(DISPLAY);
    XM()->ShowParameters();
    return (copied);
}

