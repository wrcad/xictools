
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
 $Id: break.cc,v 1.43 2015/01/09 05:52:50 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "edit.h"
#include "undolist.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "events.h"
#include "menu.h"
#include "promptline.h"
#include "select.h"
#include "errorlog.h"
#include "ghost.h"


/**************************************************************************
 *
 * Command to sever objects into smaller objects.
 *
 **************************************************************************/

namespace {
    namespace ed_break {
        struct BreakState : public CmdState
        {
            BreakState(const char*, const char*);
            virtual ~BreakState();

            void setCaller(GRobject c)  { Caller = c; }

            void b1down();
            void b1up();
            void desel();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();

            void message() { if (Level == 1) PL()->ShowPrompt(msg1);
                else PL()->ShowPrompt(msg2); }

        private:
            void SetLevel1(bool show) { Level = 1; if (show) message(); }
            void SetLevel2(bool show) { Level = 2; if (show) message(); }

            static int MsgTimeout(void*);

            GRobject Caller;       // calling button
            bool GotOne;           // true if target object(s) selected
            bool GhostOn;          // true if showing ghost break line
            BrkType Orient;        // orintation of break line
            int State;             // internal state
            char Types[4];         // type codes of breakable objects
            CDol *OrigObjs;        // original objects

            static const char *msg1;
            static const char *msg2;
        };

        BreakState *BreakCmd;
    }
}

using namespace ed_break;

const char *BreakState::msg1 = "Select objects to break.";
const char *BreakState::msg2 =
    "Click where to break, '/' key changes orientation.";


// Menu command to break objects.  The orientation of the break is set
// by the current rotation, or by a pointed-to wire.  A vertical or
// horizontal line is ghost drawn and attached to the cursor, signifying
// where the break will occur.
//
void
cEdit::breakExec(CmdDesc *cmd)
{
    if (BreakCmd) {
        BreakCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, false, DSP()->CurMode()))
        return;

    BreakCmd = new BreakState("BREAK", "xic:break");
    BreakCmd->setCaller(cmd ? cmd->caller : 0);
    // Original objects are selected after undo.
    Ulist()->ListCheck(BreakCmd->Name(), CurCell(), true);
    if (!EV()->PushCallback(BreakCmd)) {
        delete BreakCmd;
        return;
    }
    if (BreakCmd->CmdLevel() == 2)
        Gst()->SetGhost(GFline);
    BreakCmd->message();
    ds.clear();
}


BreakState::BreakState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    Orient = BrkVert;
    Gst()->SetGhostLineVert(true);
    State = 0;
    // only these types can be broken
    Types[0] = CDBOX;
    Types[1] = CDPOLYGON;
    Types[2] = CDWIRE;
    Types[3] = '\0';
    OrigObjs = 0;
    GotOne = Selections.hasTypes(CurCell(), Types, false);
    if (GotOne) {
        Level = 2;
        GhostOn = true;
        DSP()->SetInEdgeSnappingCmd(true);
    }
    else {
        Level = 1;
        GhostOn = false;
    }
}


BreakState::~BreakState()
{
    BreakCmd = 0;
    OrigObjs->free();
    DSP()->SetInEdgeSnappingCmd(false);
}


void
BreakState::b1down()
{
    if (Level == 1)
        // Start a selection.
        //
        cEventHdlr::sel_b1down();
    else {
        // Set the location for the break, and perform the break if an object
        // is in the break path.
        //
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        if (ED()->doBreak(x, y, false, Orient)) {
            // some object will be broken
            Gst()->SetGhost(GFnone);
            GhostOn = false;
            ED()->doBreak(x, y, true, Orient);
            Ulist()->CommitChanges(true);
            State = 3;
        }
    }
}


void
BreakState::b1up()
{
    if (Level == 1) {
        // If selection is successful, set state to start break operation.
        //
        if (!cEventHdlr::sel_b1up(0, Types, 0))
            return;
        if (!Selections.hasTypes(CurCell(), Types, false))
            return;
        SetLevel2(true);
        State = 1;
        Gst()->SetGhost(GFline);
        GhostOn = true;
        DSP()->SetInEdgeSnappingCmd(true);
    }
    else {
        // If objects were selected before the break command, exit.
        // Otherwise, clear the selection queue and start over.
        //
        if (State == 3) {
            State = 2;
            CDs *cursd = CurCell();
            OrigObjs->free();
            OrigObjs = Selections.listQueue(cursd, Types);
            Selections.deselectTypes(cursd, Types);
            Selections.removeTypes(cursd, Types);
            SetLevel1(true);
        }
    }
}


// Desel pressed, reset.
//
void
BreakState::desel()
{
    if (Level == 1)
        cEventHdlr::sel_esc();
    else {
        Gst()->SetGhost(GFnone);
        GhostOn = false;
    }
    GotOne = false;
    State = 0;
    SetLevel1(true);
}


// Esc entered, clean up and quit.
//
void
BreakState::esc()
{
    if (Level == 1)
        cEventHdlr::sel_esc();
    else {
        Gst()->SetGhost(GFnone);
        GhostOn = false;
        if (!GotOne) {
            CDs *cursd = CurCell();
            Selections.deselectTypes(cursd, Types);
            Selections.removeTypes(cursd, Types);
        }
    }
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


// If '/' or '\\' pressed, toggle break line orientation.
//
bool
BreakState::key(int code, const char *text, int)
{
    switch (code) {
    case BSP_KEY:
    case DELETE_KEY:
        if (Level == 2) {
            // Revert to selection level so user can update selections.
            WindowDesc *wd = EV()->CurrentWin();
            if (!wd)
                wd = DSP()->MainWdesc();
            if (PL()->KeyPos(wd) == 0) {
                Gst()->SetGhost(GFnone);
                GhostOn = false;
                State = 0;
                SetLevel1(true);
                return (true);;
            }
        }
        break;
    }
    if (text && (*text == '/' || *text == '\\')) {
        if (GhostOn == true)
            Gst()->SetGhost(GFnone);
        if (Orient == BrkVert) {
            Orient = BrkHoriz;
            Gst()->SetGhostLineVert(false);
        }
        else {
            Orient = BrkVert;
            Gst()->SetGhostLineVert(true);
        }
        if (GhostOn == true) {
            Gst()->SetGhost(GFline);
            Gst()->RepaintGhost();
        }
        return (true);
    }
    return (false);
}


void
BreakState::undo()
{
    if (Level == 1) {
        if (!cEventHdlr::sel_undo()) {
            // The undo system has placed the original objects into
            // the selection list, as selected.

            if (State == 2) {
                Gst()->SetGhost(GFline);
                GhostOn = true;
                SetLevel2(true);
                return;
            }
            dspPkgIf()->RegisterTimeoutProc(1500, MsgTimeout, 0);
        }
    }
    else {
        // Undo the selection, if there was one, or the last operation.
        //
        Gst()->SetGhost(GFnone);
        GhostOn = false;
        OrigObjs->free();
        OrigObjs = Selections.listQueue(CurCell(), Types);
        Selections.deselectTypes(CurCell(), Types);
        Selections.removeTypes(CurCell(), Types);
        if (State == 2)
            State = 3;
        SetLevel1(true);
    }
}


// Redo the last undone selection or operation.
//
void
BreakState::redo()
{
    if (Level == 1) {
        if ((State == 1 && !Ulist()->HasRedo()) ||
                (State == 3 && Ulist()->HasOneRedo())) {
            Gst()->SetGhost(GFline);
            GhostOn = true;
            Selections.insertList(CurCell(), OrigObjs);
            Selections.setShowSelected(CurCell(), Types, true);
            if (State == 3)
                State = 2;
            SetLevel2(true);
        }
        else {
            cEventHdlr::sel_redo();
            dspPkgIf()->RegisterTimeoutProc(1500, MsgTimeout, 0);
        }
    }
    else {
        if (State == 2) {
            Gst()->SetGhost(GFnone);
            GhostOn = false;
            Ulist()->RedoOperation();
            SetLevel1(true);
        }
    }
}


// Timer callback, reprint the command prompt after an undo/redo, which
// will display a message.
//
int
BreakState::MsgTimeout(void*)
{
    if (BreakCmd)
        BreakCmd->message();
    return (0);
}
// End of BreakState function.


// Actually perform the break.
//
bool
cEdit::doBreak(int ref_x, int ref_y, bool flag, BrkType orient)
{
    // If flag is false, returns true if a break would be performed,
    // but operation is not done.
    // If flag is true, perform the break, deleting broken object from
    // select queue and adding to delete list.  New objects are added
    // to the add list.
    //
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);
    sSelGen sg(Selections, cursd, "bpw");
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (split(od, cursd, ref_x, ref_y, flag, orient)) {
            if (!flag)
                return (true);
            sg.remove();
            continue;
        }
    }
    XM()->ShowParameters();
    return (false);
}


namespace { void mkwire(Plist*, CDs*, CDw*); }

bool
cEdit::split(CDo *odesc, CDs *sdesc, int refx, int refy, bool flag,
    int orient)
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
        Errs()->init_error();
        int left = odesc->oBB().left;
        int right = odesc->oBB().right;
        int top = odesc->oBB().top;
        int bottom = odesc->oBB().bottom;
        if (orient == BrkVert) {
            if (refx <= left || refx >= right)
                return (false);
            if (!flag)
                return (true);

            CDo *newo = sdesc->newBox(0, left, bottom, refx, top,
                odesc->ldesc(), odesc->prpty_list());
            if (!newo) {
                Errs()->add_error("newBox failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
            newo = sdesc->newBox(0, refx, bottom, right, top,
                odesc->ldesc(), odesc->prpty_list());
            if (!newo) {
                Errs()->add_error("newBox failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
        }
        else {
            if (refy <= bottom || refy >= top)
                return (false);
            if (!flag)
                return (true);

            CDo *newo = sdesc->newBox(0, left, bottom, right, refy,
                odesc->ldesc(), odesc->prpty_list());
            if (!newo) {
                Errs()->add_error("newBox failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
            newo = sdesc->newBox(0, left, refy, right, top,
                odesc->ldesc(), odesc->prpty_list());
            if (!newo) {
                Errs()->add_error("newBox failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
        }
        Ulist()->RecordObjectChange(sdesc, odesc, 0);
        return (true);
    }
poly:
    {
        BBox BB1, BB2;
        if (orient == BrkVert) {
            if (refx <= odesc->oBB().left || refx >= odesc->oBB().right)
                return (false);
            if (!flag)
                return (true);
            BB1.left = odesc->oBB().left;
            BB1.bottom = odesc->oBB().bottom;
            BB1.right = refx;
            BB1.top = odesc->oBB().top;
            BB2.left = refx;
            BB2.bottom = odesc->oBB().bottom;
            BB2.right = odesc->oBB().right;
            BB2.top = odesc->oBB().top;
        }
        else {
            if (refy <= odesc->oBB().bottom || refy >= odesc->oBB().top)
                return (false);
            if (!flag)
                return (true);
            BB1.left = odesc->oBB().left;
            BB1.bottom = odesc->oBB().bottom;
            BB1.right = odesc->oBB().right;
            BB1.top = refy;
            BB2.left = odesc->oBB().left;
            BB2.bottom = refy;
            BB2.right = odesc->oBB().right;
            BB2.top = odesc->oBB().top;
        }

        Errs()->init_error();
        PolyList *p0 = ((const CDpo*)odesc)->po_clip(&BB1);
        for (PolyList *p = p0; p; p = p->next) {
            if (!sdesc->newPoly(0, &p->po, odesc->ldesc(),
                    odesc->prpty_list(), false)) {
                Errs()->add_error("newPoly failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
        }
        PolyList::destroy(p0);

        p0 = ((const CDpo*)odesc)->po_clip(&BB2);
        for (PolyList *p = p0; p; p = p->next) {
            if (!sdesc->newPoly(0, &p->po, odesc->ldesc(),
                    odesc->prpty_list(), false)) {
                Errs()->add_error("newPoly failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
        }
        PolyList::destroy(p0);
        Ulist()->RecordObjectChange(sdesc, odesc, 0);
        return (true);
    }
wire:
    {
        if (orient == BrkVert) {
            if (refx <= odesc->oBB().left || refx >= odesc->oBB().right)
                return (false);
            if (!flag)
                return (true);

            Plist *p0 = 0;
            for (int i = 0; i < ((CDw*)odesc)->numpts(); i++)
                p0 = new Plist(((CDw*)odesc)->points()[i].x,
                    ((CDw*)odesc)->points()[i].y, p0);
            Plist *pn;
            for (Plist *p = p0; p->next; p = pn) {
                pn = p->next;

                if ((p->x <= refx && pn->x > refx) ||
                        (p->x >= refx && pn->x < refx)) {
                    int x = refx;
                    int y = p->y + ((x - p->x)*(pn->y - p->y))/
                        (pn->x - p->x);
                    if (x == p->x && y == p->y) {
                        if (p == p0)
                            continue;
                        pn = new Plist(x, y, pn);
                        p->next = 0;
                    }
                    else {
                        pn = new Plist(x, y, pn);
                        p->next = new Plist(x, y, 0);
                    }
                    mkwire(p0, sdesc, (CDw*)odesc);
                    p0 = pn;
                }
            }
            mkwire(p0, sdesc, (CDw*)odesc);
        }
        else {
            if (refy <= odesc->oBB().bottom || refy >= odesc->oBB().top)
                return (false);
            if (!flag)
                return (true);

            Plist *p0 = 0;
            for (int i = 0; i < ((CDw*)odesc)->numpts(); i++)
                p0 = new Plist(((CDw*)odesc)->points()[i].x,
                    ((CDw*)odesc)->points()[i].y, p0);
            Plist *pn;
            for (Plist *p = p0; p->next; p = pn) {
                pn = p->next;

                if ((p->y <= refy && pn->y > refy) ||
                        (p->y >= refy && pn->y < refy)) {
                    int y = refy;
                    int x = p->x + ((y - p->y)*(pn->x - p->x))/
                        (pn->y - p->y);
                    if (x == p->x && y == p->y) {
                        if (p == p0)
                            continue;
                        pn = new Plist(x, y, pn);
                        p->next = 0;
                    }
                    else {
                        pn = new Plist(x, y, pn);
                        p->next = new Plist(x, y, 0);
                    }
                    mkwire(p0, sdesc, (CDw*)odesc);
                    p0 = pn;
                }
            }
            mkwire(p0, sdesc, (CDw*)odesc);
        }
        Ulist()->RecordObjectChange(sdesc, odesc, 0);
        return (true);
    }
label:
inst:
    return (false);
}


namespace {
    void
    mkwire(Plist *p0, CDs *sdesc, CDw *wrd)
    {
        int i = 0;
        for (Plist *p = p0; p; p = p->next, i++) ;
        if (i >= 2) {
            Wire wire(i, new Point[i], wrd->attributes());
            i--;
            for (Plist *p = p0; p; p = p->next, i--)
                wire.points[i].set(p->x, p->y);
            Errs()->init_error();
            if (!sdesc->newWire(0, &wire, wrd->ldesc(), wrd->prpty_list(),
                    false)) {
                Errs()->add_error("newWire failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
        }
        Plist *pn;
        for (Plist *p = p0; p; p = pn) {
            pn = p->next;
            delete p;
        }
    }
}

