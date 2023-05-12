
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
#include "vtxedit.h"
#include "scedif.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "tech.h"
#include "events.h"
#include "layertab.h"
#include "menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "ghost.h"


// The Style menu commands.
//
void
cEdit::setWireAttribute(WsType wtype)
{
    if (wtype == WsWidth) {
        execWireWidth();
        return;
    }
    if (wtype == WsFlush)
        ed_wire_style = CDWIRE_FLUSH;
    else if (wtype == WsRound)
        ed_wire_style = CDWIRE_ROUND;
    else if (wtype == WsExtnd)
        ed_wire_style = CDWIRE_EXTEND;
    else
        return;
    widthCallback();
    execWireStyle();
}


namespace {
    bool mark_vertices(bool);

    int mark_idle(void*)
    {
        mark_vertices(DISPLAY);
        return (0);
    }

#define MARK_SIZE 8

    // Struct to keep track of operations for undo/redo.
    //
    enum V_OP {V_CRT, V_NEWV, V_CHGV, V_STY, V_WID};
    // V_CRT   wire created
    // V_NEWV  single vertex added
    // V_CHGV  vertices moved or deleted
    // V_STY   style changed
    // V_WID   width changed

    struct sUndoV
    {
        sUndoV(sUndoV *n)
            {
                newo = old = 0;
                list = 0;
                x = y = 0;
                operation = V_CRT;
                next = n;
            }

        ~sUndoV()
            {
                Ochg::destroy(list, false);
            }

        static void destroy(sUndoV *u)
            {
                while (u) {
                    sUndoV *ux = u;
                    u = u->next;
                    delete ux;
                }
            }

        CDo *newo;       // new wire created
        CDo *old;        // old wire deleted
        Ochg *list;      // list of changes for V_CHGV
        int x, y;        // vertex location
        V_OP operation;  // operation code
        sUndoV *next;
    };

    namespace ed_wires {
        struct WireState : public CmdState
        {
            friend void cEdit::widthCallback();
            friend void cEdit::execWireStyle();
            friend void cEdit::execWireWidth();

            WireState(const char*, const char*);
            virtual ~WireState();

            void setCaller(GRobject c)  { Caller = c; }

            void b1down();
            void b1up();
            void desel();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();
            void message();

        private:
            void create_wire();
            void copy_objlist();
            bool stretch(int, int, int*, int*);
            void delete_vertices();
            bool add_vertex();
            bool allocate_wire(int, WireStyle);

            void SetLevel1() { Level = 1; message(); }
            void SetLevel2() { Level = 2; }
            void SetLevel3() { Level = 3; }

            void GhostOn(int x, int y)
                {
                    PthX = x;
                    PthY = y;
                    PthOn = true;
                    ED()->pthSet(x, y);
                    Gst()->SetGhostAt(GFwireseg, x, y);
                }

            void GhostOff()
                {
                    PthOn = false;
                    Gst()->SetGhost(GFnone);
                }

            GRobject Caller;      // button
            int State;            // status number
            int NumPts;           // number of vertices currently in wire
            Point *Points;        // current vertex
            Plist *Phead;         // head of a list of vertices for undo
            int Firstx, Firsty;   // first vertex
            int ChainWidth;       // matched wire width
            WireStyle ChainStyle; // matched wire style
            bool UseChain;        // use matched width/style
            bool Override;        // Shift key down
            bool Simple45;        // Ctrl key down
            bool SelectingVertex; // set while in vertex selection
            bool SelectingWires;  // set while selecting wires to edit
            sUndoV *UndoList;     // list of vertex operations for undo
            sUndoV *RedoList;     // list of vertex operations for redo
            char Types[4];        // selection types
            int Refx, Refy;       // stretch reference point
            int Newx, Newy;       // stretch-to point
            sObj *Objlist_back;   // backup for stretch undo
            BBox RdBB;            // bounding box for redraw
            int PthX, PthY;       // ghost state, need to reset after Shift or
            bool PthOn;           //  funny first point may occur

            static const char *msg0;
            static const char *msg;
            static const char *msgA;
            static const char *msgB;
        };

        WireState *WireCmd;
    }
}

using namespace ed_wires;

const char *WireState::msg0 =
    "Current width %.*f, style %s.  %s";
const char *WireState::msg =
    "Click to create vertices, click twice to end.";
const char *WireState::msgA =
    "Click to create/delete vertices, hold Shift to select vertices to move.";
const char *WireState::msgB =
    "Select more vertices, or drag or click twice to move selected vertices.";


// Menu command to create wires.  The wire is formed by clicking on
// the vertices.  Click on a vertex twice to terminate the wire.
// The partially complete wire is shown, and the last segment is
// ghost drawn with the line attached to the pointer.
// If a wire is selected before this command is entered, vertex
// editing mode is active.  The vertices of the selected wire(s)
// are marked with a box.  Pointing at the selected wire(s) not at
// a vertex will create a new vertex.  Pointing at a vertex will
// delete the vertex, unless the shift key is pressed in which case
// the vertex can be moved (similar to the stretch command).
//
void
cEdit::makeWiresExec(CmdDesc *cmd)
{
    if (WireCmd) {
        WireCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;

    WireCmd = new WireState("WIRE", "xic:wire");
    WireCmd->setCaller(cmd ? cmd->caller : 0);
    Ulist()->ListCheck(WireCmd->Name(), CurCell(), false);
    if (!EV()->PushCallback(WireCmd)) {
        delete WireCmd;
        return;
    }
    WireCmd->message();
    ds.clear();
}


namespace {
    // Delete the wire under construction, and fix the cell's BB if
    // necessary.
    //
    bool
    delete_inc()
    {
        CDs *cursd = CurCell();
        if (!cursd)
            return (false);
        if (DSP()->IncompleteObject()) {
            if (cursd->isElectrical())
                ScedIf()->uninstall(DSP()->IncompleteObject(), cursd);
            if (!cursd->unlink(DSP()->IncompleteObject(), false))
                Log()->PopUpErr(Errs()->get_error());
            DSP()->SetIncompleteObject(0);
            cursd->computeBB();
            return (true);
        }
        return (false);
    }
}


WireState::WireState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    State = 0;
    NumPts = 0;
    Points = 0;
    Phead = 0;
    Firstx = Firsty = 0;
    ChainWidth = 0;
    ChainStyle = CDWIRE_EXTEND;
    UseChain = false;
    Override = false;
    Simple45 = false;
    SelectingVertex = false;
    SelectingWires = false;
    UndoList = RedoList = 0;
    Types[0] = CDWIRE;
    Types[1] = 0;
    Refx = Refy = 0;
    Newx = Newy = 0;
    Objlist_back = 0;
    PthX = PthY = 0;
    PthOn = false;
    DSP()->SetInEdgeSnappingCmd(true);

    SetLevel1();
    mark_vertices(DISPLAY);

    ED()->pthSetSimple(Simple45, Override);
    EV()->SetConstrained(false);
    ED()->clearObjectList();
}


WireState::~WireState()
{
    WireCmd = 0;
    delete [] Points;
    DSP()->SetInEdgeSnappingCmd(false);
}


namespace {
    // Return the spacing in screen pixels between the two points.
    //
    double pixdist(int x1, int y1, int x2, int y2)
    {
        WindowDesc *wdesc = EV()->CurrentWin();
        if (!wdesc)
            return (0.0);
        double dx = x1 - x2;
        double dy = y1 - y2;
        double d = sqrt(dx*dx + dy*dy);
        d *= wdesc->Ratio();
        return (d);
    }
}


// Add a vertex to a wire, or start a new wire.  Two successive points
// to the same location terminates the wire.
//
void
WireState::b1down()
{
    if (Level == 2) {
        // Finish the stretch.
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        if (stretch(Refx, Refy, &x, &y)) {
            Newx = x;
            Newy = y;
            XM()->SetCoordMode(CO_ABSOLUTE);
            if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
                DSP()->ShowCellTerminalMarks(DISPLAY);
            SetLevel3();
        }
        return;
    }
    CDs *cursd = CurCell();
    if (!cursd)
        return;

    sObj::destroy(Objlist_back);
    Objlist_back = 0;
    sUndoV::destroy(RedoList);
    RedoList = 0;
    if (!NumPts)
        Ulist()->ListCheck(WireCmd->StateName, cursd, false);

    while (Phead) {
        Plist *pn = Phead->next;
        delete Phead;
        Phead = pn;
    }

    State = 0;
    if (SelectingVertex || SelectingWires) {
        cEventHdlr::sel_b1down();
        return;
    }

    // Pressing Ctrl will force start of a new wire when editing
    // vertices.
    if (!NumPts && !(EV()->Cursor().get_downstate() & GR_CONTROL_MASK)) {
        if (!sObj::empty(ED()->objectList()) &&
                !(EV()->Cursor().get_downstate() & GR_SHIFT_MASK)) {
            // Start stretching
            EV()->Cursor().get_xy(&Refx, &Refy);
            XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
            Gst()->SetGhostAt(GFstretch, Refx, Refy);
            EV()->DownTimer(GFstretch);
            SetLevel2();
            State = 1;
            return;
        }
        if (Selections.hasTypes(CurCell(), Types)) {
            // Start selecting vertices
            SelectingVertex = true;
            cEventHdlr::sel_b1down();
            return;
        }
    }

    int width = dsp_prm(LT()->CurLayer())->wire_width();
    WireStyle style = ED()->getWireStyle();
    message();
    int x, y;
    EV()->Cursor().get_xy(&x, &y);
    if (!NumPts) {
        UseChain = false;
        // If we start a wire on an end vertex of an existing wire on
        // the current layer, use the same width and end style of the
        // existing wire.  The new wire will be merged with the
        // existing wire unless merging is disabled.

        BBox BB(x, y, x, y);
        CDol *slist = Selections.selectItems(cursd, "w", &BB, PSELpoint);
        for (CDol *sl = slist; sl; sl = sl->next) {
            if (sl->odesc->ldesc() != LT()->CurLayer())
                continue;
            const Point *pts = ((CDw*)sl->odesc)->points();
            int numpts = ((CDw*)sl->odesc)->numpts();
            if ((pts[0].x == x && pts[0].y == y) ||
                    (pts[numpts-1].x == x && pts[numpts-1].y)) {
                width = ((CDw*)sl->odesc)->wire_width();
                style = ((CDw*)sl->odesc)->wire_style();
                UseChain = true;
                ChainWidth = width;
                ChainStyle = style;
            }
        }

        Firstx = x;
        Firsty = y;
        delete [] Points;
        Points = new Point[1];
        Points->set(x, y);
        NumPts = 1;
        if (!allocate_wire(width, style))
            return;
        DSP()->ShowCrossMark(DISPLAY, x, y, HighlightingColor, 20,
            DSP()->CurMode());
        XM()->SetCoordMode(CO_RELATIVE, x, y);
        GhostOn(x, y);
        return;
    }
    if (UseChain) {
        width = ChainWidth;
        style = ChainStyle;
    }
    GhostOff();
    XM()->SetCoordMode(CO_ABSOLUTE);
    if (x != Points[NumPts-1].x || y != Points[NumPts-1].y) {
        int ox = Points[NumPts-1].x;
        int oy = Points[NumPts-1].y;
        // If the point is within two pixels of the previous point, assume
        // wire termination.
        if (pixdist(x, y, ox, oy) <= DSP()->PixelDelta() + 0.1) {
            x = ox;
            y = oy;
        }
        else {
            int np = NumPts;
            int xx = 0, yy = 0;
            bool pt_added = false;
            // See note in 45s.cc about logic.
            if ((!Tech()->IsConstrain45() && (Override || Simple45)) ||
                    (Tech()->IsConstrain45() && !Override)) {
                // reg45 or simple45
                ED()->pthGet(&xx, &yy);
                Points = Point::append(Points, &NumPts, xx, yy);
                pt_added = true;
            }
            if ((!Tech()->IsConstrain45() && !Simple45) ||
                    (Tech()->IsConstrain45() && (!Simple45 || Override))) {
                // reg45 or no45
                // Only add the second point if it is 8 pixels or more
                // away, otherwise it is too easy to add spurious
                // vertices.
                if (!pt_added || pixdist(x, y, xx, yy) >= 8.0)
                    Points = Point::append(Points, &NumPts, x, y);
            }
            if (np != NumPts) {
                if (allocate_wire(width, style)) {
                    DSPmainDraw(SetColor(dsp_prm(LT()->CurLayer())->pixel()))
                    DSP()->RedisplayArea(&RdBB);
                    GhostOn(Points[NumPts-1].x, Points[NumPts-1].y);
                }
                return;
            }
            // Must be roundoff error, tweek last point so we can terminate
            Points[NumPts-1].set(x, y);
        }
    }
    if (x == Points[NumPts-1].x && y == Points[NumPts-1].y) {
        // click twice to terminate
        if (NumPts == 1) {
            delete_inc();
            DSP()->ShowCrossMark(ERASE, Firstx, Firsty, HighlightingColor,
                20, DSP()->CurMode());
            DSP()->RedisplayArea(&RdBB);
            NumPts = 0;
        }
        else
            create_wire();
    }
}


void
WireState::b1up()
{
    if (SelectingWires) {
        BBox BB;
        CDol *slist;
        if (cEventHdlr::sel_b1up(&BB, Types, &slist)) {
            mark_vertices(ERASE);
            for (CDol *sl = slist; sl; sl = sl->next) {
                if (sl->odesc->type() != CDWIRE)
                    continue;
                if (sl->odesc->state() == CDobjSelected)
                    Selections.removeObject(CurCell(), sl->odesc);
                else
                    Selections.insertObject(CurCell(), sl->odesc);
            }
            mark_vertices(DISPLAY);
            CDol::destroy(slist);
        }
        SelectingWires = false;
        message();
        return;
    }
    if (SelectingVertex) {
        BBox BB;
        CDol *slist;
        if (!cEventHdlr::sel_b1up(&BB, Types, &slist))
            return;
        SelectingVertex = false;
        sObj::mark_vertices(ED()->objectList(), ERASE);

        WindowDesc *wdesc = EV()->ButtonWin();
        if (!wdesc)
            wdesc = DSP()->MainWdesc();
        int delta = 1 + (int)(DSP()->PixelDelta()/wdesc->Ratio());
        int xr, yr;
        EV()->Cursor().get_raw(&xr, &yr);
        if (BB.width() < 2*delta) {
            BB.left = xr - delta;
            BB.right = xr + delta;
        }
        if (BB.height() < 2*delta) {
            BB.bottom = yr - delta;
            BB.top = yr + delta;
        }

        ED()->setObjectList(sObj::mklist(ED()->objectList(), slist, &BB));
        if (!sObj::empty(ED()->objectList())) {
            sObj::mark_vertices(ED()->objectList(), DISPLAY);
            PL()->ShowPrompt(msgB);
        }
        else if (!add_vertex())
            mark_vertices(DISPLAY);
        CDol::destroy(slist);
        return;
    }
    if (Level == 2) {
        // Finish the stretch if the pointer moved, otherwise set up to
        // look for the next button 1 press.
        //
        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok() &&
                EV()->CurrentWin()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            int xc, yc;
            EV()->Cursor().get_xy(&xc, &yc);
            if (x != xc || y != yc) {
                if (stretch(Refx, Refy, &x, &y)) {
                    Newx = x;
                    Newy = y;
                    XM()->SetCoordMode(CO_ABSOLUTE);
                    if (DSP()->CurMode() == Electrical &&
                            DSP()->ShowTerminals())
                        DSP()->ShowCellTerminalMarks(DISPLAY);
                    SetLevel1();
                    State = 2;
                    return;
                }
            }
        }
    }
    if (Level == 3) {
        SetLevel1();
        State = 2;
    }
}


// Desel pressed, reset.
//
void
WireState::desel()
{
    if (SelectingVertex || SelectingWires)
        cEventHdlr::sel_esc();
    mark_vertices(ERASE);
    ED()->clearObjectList();
    sObj::destroy(Objlist_back);
    Objlist_back = 0;
    sUndoV::destroy(UndoList);
    UndoList = 0;
    sUndoV::destroy(RedoList);
    RedoList = 0;
}


// Esc entered, clean up and exit.
//
void
WireState::esc()
{
    if (!sObj::empty(ED()->objectList())) {
        GhostOff();
        sObj::mark_vertices(ED()->objectList(), ERASE);
        sObj::destroy(Objlist_back);
        Objlist_back = ED()->objectList();
        ED()->setObjectList(0);
        SetLevel1();
        State = 0;
        mark_vertices(DISPLAY);
        return;
    }

    if (SelectingVertex || SelectingWires)
        cEventHdlr::sel_esc();
    else
        GhostOff();
    XM()->SetCoordMode(CO_ABSOLUTE);
    PL()->ErasePrompt();
    mark_vertices(ERASE);
    ED()->clearObjectList();
    EV()->SetConstrained(false);
    sObj::destroy(Objlist_back);
    sUndoV::destroy(UndoList);
    sUndoV::destroy(RedoList);
    if (delete_inc()) {
        DSP()->RedisplayArea(&RdBB);
        DSP()->ShowCrossMark(ERASE, Firstx, Firsty, HighlightingColor,
            20, DSP()->CurMode());
    }
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


// Handle keypresses.  The Shift key alters the constraint for both
// vertex stretching and path addition.  The Ctrl key sets the "simple"
// 45 mode.
//
bool
WireState::key(int code, const char*, int)
{
    switch (code) {
    case SHIFTUP_KEY:
        DSPmainDraw(ShowGhost(ERASE))
        EV()->SetConstrained(false);
        if (PthOn)
            ED()->pthSet(PthX, PthY);
        Override = false;
        ED()->pthSetSimple(Simple45, Override);
        DSPmainDraw(ShowGhost(DISPLAY))
        break;
    case SHIFTDN_KEY:
        DSPmainDraw(ShowGhost(ERASE))
        EV()->SetConstrained(true);
        if (PthOn)
            ED()->pthSet(PthX, PthY);
        Override = true;
        ED()->pthSetSimple(Simple45, Override);
        DSPmainDraw(ShowGhost(DISPLAY))
        break;
    case CTRLUP_KEY:
        DSPmainDraw(ShowGhost(ERASE))
        if (PthOn)
            ED()->pthSet(PthX, PthY);
        Simple45 = false;
        ED()->pthSetSimple(Simple45, Override);
        DSPmainDraw(ShowGhost(DISPLAY))
        break;
    case CTRLDN_KEY:
        DSPmainDraw(ShowGhost(ERASE))
        if (PthOn)
            ED()->pthSet(PthX, PthY);
        Simple45 = true;
        ED()->pthSetSimple(Simple45, Override);
        DSPmainDraw(ShowGhost(DISPLAY))
        break;
    case RETURN_KEY:
        if (Level == 1) {
            if (!NumPts) {
                SelectingWires = true;
                PL()->ShowPrompt("Click or drag to select wires for editing.");
            }
            else if (NumPts > 1 && !SelectingVertex && !SelectingWires) {
                // Terminate, create wire.
                sObj::destroy(Objlist_back);
                Objlist_back = 0;
                sUndoV::destroy(RedoList);
                RedoList = 0;
                while (Phead) {
                    Plist *pn = Phead->next;
                    delete Phead;
                    Phead = pn;
                }
                State = 0;
                GhostOff();
                XM()->SetCoordMode(CO_ABSOLUTE);
                create_wire();
            }
        }
        break;
    case DELETE_KEY:
        delete_vertices();
        break;
    case BSP_KEY:
        if (!sObj::empty(ED()->objectList())) {
            GhostOff();
            sObj::mark_vertices(ED()->objectList(), ERASE);
            sObj::destroy(Objlist_back);
            Objlist_back = ED()->objectList();
            ED()->setObjectList(0);
            SetLevel1();
            State = 0;
            mark_vertices(DISPLAY);
            return (true);
        }
    }
    return (false);
}


// Undo the vertices previously placed.
//
void
WireState::undo()
{
    if (Level == 2) {
        GhostOff();
        XM()->SetCoordMode(CO_ABSOLUTE);
        SetLevel1();
        return;
    }
    if (!sObj::empty(ED()->objectList())) {
        sObj::mark_vertices(ED()->objectList(), ERASE);
        sObj::destroy(Objlist_back);
        Objlist_back = ED()->objectList();
        ED()->setObjectList(0);
        return;
    }
    if (!NumPts) {
        const char *vmsg = "No more vertex operations to undo.";
        if (UndoList) {
            if (Ulist()->HasUndo()) {
                mark_vertices(ERASE);
                Ulist()->UndoOperation();
                sUndoV *v_op = UndoList;
                UndoList = v_op->next;
                CDs *cursd = CurCell();
                if (v_op->list) {
                    BBox BB(CDnullBB);
                    for (Ochg *oc = v_op->list; oc; oc = oc->next_chg()) {
                        if (oc->odel()) {
                            Selections.insertObject(cursd, oc->odel(), true);
                            BB.add(&oc->odel()->oBB());
                        }
                    }
                    DSP()->RedisplayArea(&BB);
                }
                else if (v_op->old) {
                    Selections.insertObject(cursd, v_op->old, true);
                    DSP()->RedisplayArea(&v_op->old->oBB());
                }
                v_op->next = RedoList;
                RedoList = v_op;
                // Draw after update, or marks get clipped.
                DSPpkg::self()->RegisterIdleProc(mark_idle, 0);
            }
            else {
                // hit the end of the (truncated) undo list
                sUndoV::destroy(UndoList);
                UndoList = 0;
                PL()->ShowPrompt(vmsg);
            }
        }
        else if (RedoList)
            PL()->ShowPrompt(vmsg);
        else {
            mark_vertices(ERASE);
            Ulist()->UndoOperation();
            mark_vertices(DISPLAY);
        }
        return;
    }
    GhostOff();
    XM()->SetCoordMode(CO_ABSOLUTE);
    Phead = new Plist(Points[NumPts-1].x, Points[NumPts-1].y, Phead);
    Points = Point::remove_last(Points, &NumPts);
    BBox oldBB = RdBB;
    if (!NumPts) {
        delete_inc();
        DSP()->ShowCrossMark(ERASE, Firstx, Firsty, HighlightingColor,
            20, DSP()->CurMode());
        DSP()->RedisplayArea(&oldBB);
    }
    else {
        int width =
            UseChain ? ChainWidth : dsp_prm(LT()->CurLayer())->wire_width();
        WireStyle style = UseChain ? ChainStyle : ED()->getWireStyle();
        if (!allocate_wire(width, style))
            return;
        DSP()->RedisplayArea(&oldBB);
        XM()->SetCoordMode(CO_RELATIVE, Points[NumPts-1].x, Points[NumPts-1].y);
        GhostOn(Points[NumPts-1].x, Points[NumPts-1].y);
    }
    message();
}


// Put back any undone vertices.
//
void
WireState::redo()
{
    if (Ulist()->HasRedo()) {
        mark_vertices(ERASE);
        Ulist()->RedoOperation();
        if (RedoList) {
            sUndoV *v_op = RedoList;
            RedoList = v_op->next;
            CDs *cursd = CurCell();
            if (v_op->list) {
                BBox BB(CDnullBB);;
                for (Ochg *oc = v_op->list; oc; oc = oc->next_chg()) {
                    if (oc->oadd()) {
                        Selections.insertObject(cursd, oc->oadd(), true);
                        BB.add(&oc->oadd()->oBB());
                    }
                }
                DSP()->RedisplayArea(&BB);
            }
            else if (v_op->newo) {
                Selections.insertObject(cursd, v_op->newo, true);
                DSP()->RedisplayArea(&v_op->newo->oBB());
            }
            v_op->next = UndoList;
            UndoList = v_op;
        }
        // Draw after update, or marks get clipped.
        DSPpkg::self()->RegisterIdleProc(mark_idle, 0);
        return;
    }

    if (Level == 2)
        return;
    if (!sObj::empty(Objlist_back)) {
        ED()->setObjectList(Objlist_back);
        Objlist_back = 0;
        mark_vertices(ERASE);
        sObj::mark_vertices(ED()->objectList(), DISPLAY);
        return;
    }
    if (State >= 1 && !sObj::empty(ED()->objectList())) {
        XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
        Gst()->SetGhostAt(GFstretch, Refx, Refy);
        SetLevel2();
        return;
    }

    if (Phead) {
        GhostOff();
        XM()->SetCoordMode(CO_ABSOLUTE);
        int x = Phead->x;
        int y = Phead->y;
        Plist *p = Phead;
        Phead = Phead->next;
        delete p;
        if (!NumPts) {
            delete [] Points;
            Points = new Point[1];
            Points->set(x, y);
            NumPts++;
            DSP()->ShowCrossMark(DISPLAY, x, y, HighlightingColor,
                20, DSP()->CurMode());
        }
        else {
            int width =
                UseChain ? ChainWidth : dsp_prm(LT()->CurLayer())->wire_width();
            WireStyle style = UseChain ? ChainStyle : ED()->getWireStyle();
            Points = Point::append(Points, &NumPts, x, y);
            if (!allocate_wire(width, style))
                return;
            DSPmainDraw(SetColor(dsp_prm(LT()->CurLayer())->pixel()))
            DSP()->RedisplayArea(&RdBB);
        }
        XM()->SetCoordMode(CO_RELATIVE, x, y);
        GhostOn(x, y);
        message();
        return;
    }
}


void
WireState::message()
{
    if (Level == 2) {
        PL()->ShowPrompt(msgB);
        return;
    }
    if (DSP()->CurMode() == Physical) {
        const char *s;
        switch (ED()->getWireStyle()) {
        case CDWIRE_FLUSH:
            s = "Flush";
            break;
        case CDWIRE_ROUND:
            s = "Rounded";
            break;
        default:
            s = "Extended";
            break;
        }
        int ndgt = CD()->numDigits();
        if (Selections.hasTypes(CurCell(), Types))
            PL()->ShowPromptV(msg0, ndgt,
                MICRONS(dsp_prm(LT()->CurLayer())->wire_width()), s, msgA);
        else
            PL()->ShowPromptV(msg0, ndgt,
                MICRONS(dsp_prm(LT()->CurLayer())->wire_width()), s, msg);
    }
    else {
        if (Selections.hasTypes(CurCell(), Types))
            PL()->ShowPrompt(msgA);
        else
            PL()->ShowPrompt(msg);
    }
}


void
WireState::create_wire()
{
    CDs *cursd = CurCell();
    CDw *neww = (CDw*)DSP()->IncompleteObject();
    Wire wire;
    wire.numpts = neww->numpts();
    wire.points = Point::dup(neww->points(), wire.numpts);
    wire.set_wire_style(neww->wire_style());
    wire.set_wire_width(neww->wire_width());
    CDl *ldesc = neww->ldesc();
    delete_inc();

    Errs()->init_error();
    mark_vertices(ERASE);
    Point *rpts = 0;
    int npts;
    for (;;) {
        if (DSP()->CurMode() == Physical)
            wire.checkWireVerts(&rpts, &npts);
        if (cursd->makeWire(ldesc, &wire, &neww) != CDok) {
            Errs()->add_error("makeWire failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            return;
        }
        Ulist()->RecordObjectChange(cursd, 0, neww);
        if (!cursd->mergeWire(neww, true)) {
            Errs()->add_error("mergeWire failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            return;
        }

        if (!rpts)
            break;
        wire.points = rpts;
        wire.numpts = npts;
    }
    mark_vertices(DISPLAY);

    DSP()->ShowCrossMark(ERASE, Firstx, Firsty, HighlightingColor,
        20, DSP()->CurMode());
    Ulist()->CommitChanges(true);

    // Add a placeholder in the UndoList
    UndoList = new sUndoV(UndoList);
    // clear any redo operations
    sUndoV::destroy(RedoList);
    RedoList = 0;
    NumPts = 0;
}


// Copy the current object list for undo.  It is important to zero out
// objects that are added and deleted in the list.  These are freed in
// CommitChanges(), so would otherwise be bad pointers.  This happens
// when the present operation causes a wire merge.
//
void
WireState::copy_objlist()
{
    UndoList->list = Ochg::copy(Ulist()->CurOp().obj_list());
    for (Ochg *oc1 = UndoList->list; oc1; oc1 = oc1->next_chg()) {
        if (oc1->oadd() && oc1->oadd()->state() == CDobjDeleted) {
            for (Ochg *oc2 = UndoList->list; oc2; oc2 = oc2->next_chg()) {
                if (oc2->odel() == oc1->oadd()) {
                    oc1->set_oadd(0);
                    oc2->set_odel(0);
                    break;
                }
            }
        }
    }
}


// Perform the vertex move operation.
//
bool
WireState::stretch(int ref_x, int ref_y, int *map_x, int *map_y)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);

    bool ret = false;
    if (!sObj::empty(ED()->objectList())) {
        if (Tech()->IsConstrain45() ^ EV()->IsConstrained()) {
            int rx, ry, xm, ym;
            if (ED()->get_wire_ref(&rx, &ry, &xm, &ym)) {
                int dx = ref_x - xm;
                int dy = ref_y - ym;
                *map_x -= dx;
                *map_y -= dy;
                XM()->To45(rx, ry, map_x, map_y);
                *map_x += dx;
                *map_y += dy;
            }
            else
                XM()->To45(ref_x, ref_y, map_x, map_y);
        }

        ED()->doStretchObjList(ref_x, ref_y, *map_x, *map_y, true);

        if (Ulist()->HasChanged()) {
            Gst()->SetGhost(GFnone);
            sObj::mark_vertices(ED()->objectList(), ERASE);
            ED()->clearObjectList();

            // save operation for undo
            UndoList = new sUndoV(UndoList);
            UndoList->operation = V_CHGV;
            copy_objlist();
            // clear any redo operations
            sUndoV::destroy(RedoList);
            RedoList = 0;

            mark_vertices(DISPLAY);
            if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
                DSP()->ShowCellTerminalMarks(ERASE);
            Ulist()->CommitChanges(true);
            ret = true;
        }
        XM()->ShowParameters();
    }
    return (ret);
}


// Delete the marked (movable) vertices, if possible.
//
void
WireState::delete_vertices()
{
    if (sObj::empty(ED()->objectList()))
        return;
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    mark_vertices(ERASE);
    sObj *onext;
    for (sObj *o = ED()->objectList(); o; o = onext) {
        onext = o->next_obj();
        if (o->object()->type() != CDWIRE)
            continue;
        CDw *wdesc = (CDw*)o->object();
        Point *pts = new Point[wdesc->numpts()];
        int npts = 0;
        bool merge = false;
        for (Vertex *v = o->points(); v; v = v->next()) {
            if (v->movable())
                continue;
            pts[npts].x = v->px();
            pts[npts].y = v->py();
            // If removing an end vertex, attempt to merge with existing
            // wires.
            if (npts == 0 || npts == wdesc->numpts()-1)
                merge = true;
            npts++;
        }
        if (npts < 2 || npts == wdesc->numpts()) {
            delete [] pts;
            continue;
        }

        Wire wire;
        wire.points = pts;
        wire.numpts = npts;
        wire.attributes = wdesc->attributes();
        CDw *neww = cursd->newWire(wdesc, &wire, wdesc->ldesc(),
            wdesc->prpty_list(), false);
        if (!neww) {
            Errs()->add_error("newWire failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            return;
        }
        if (merge && !cursd->mergeWire(neww, true)) {
            Errs()->add_error("mergeWire failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        }
        if (neww->state() != CDobjDeleted)
            Selections.replaceObject(cursd, wdesc, neww);
        ED()->purgeObjectList(wdesc);
    }
    ED()->clearObjectList();

    // save operation for undo
    UndoList = new sUndoV(UndoList);
    UndoList->operation = V_CHGV;
    copy_objlist();

    // clear any redo operations
    sUndoV::destroy(RedoList);
    RedoList = 0;

    mark_vertices(DISPLAY);
    if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
        DSP()->ShowCellTerminalMarks(ERASE);
    Ulist()->CommitChanges(true);
}


// Add a single vertex, if possible, atg the last button press
// location.
//
bool
WireState::add_vertex()
{
    if (NumPts || !Selections.hasTypes(CurCell(), Types))
        return (false);
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);

    // User can select a vertex off-grid, but new vertices are created
    // on-grid.
    //
    WindowDesc *wdesc = EV()->ButtonWin();
    if (!wdesc)
        wdesc = DSP()->MainWdesc();
    int delta = 1 + (int)(DSP()->PixelDelta()/wdesc->Ratio());
    int xr, yr;
    EV()->Cursor().get_raw(&xr, &yr);
    BBox BB(xr, yr, xr, yr);
    BB.bloat(delta);
    CDol *slist = Selections.selectItems(cursd, Types, &BB, PSELstrict_area);
    if (!slist)
        return (false);
    CDw *wrdesc = 0;

    for (CDol *s = slist; s; s = s->next) {
        if (s->odesc->state() == CDobjSelected &&
                Selections.inQueue(cursd, s->odesc)) {
            wrdesc = OWIRE(s->odesc);
            break;
        }
    }
    CDol::destroy(slist);
    if (!wrdesc)
        return (false);

    const Point *pts = wrdesc->points();
    int num = wrdesc->numpts();
    int secnum = 0;
    Point px;
    EV()->Cursor().get_xy(&px.x, &px.y);
    {
        int i, tx, ty;
        EV()->Cursor().get_raw(&tx, &ty);
        for (i = 0; i < num; i++) {
            if (abs(tx - pts[i].x) <= delta && abs(ty - pts[i].y) <= delta)
                return (false);
        }
        for (i = 1; i < num; i++) {
            if (Point::inPath(pts+i-1, &px, delta + wrdesc->wire_width()/4,
                    0, 2)) {
                secnum = i;
                break;
            }
        }
    }
    if (secnum == 0) {
        // Clicked on selected wire, but not close to center.
        return (false);
    }
    if (cursd->isElectrical() && !cursd->checkVertex(px.x, px.y, wrdesc)) {
        if (wrdesc->ldesc()->isWireActive())
            PL()->FlashMessage(
                "Can't add Manhattan vertex not over contact.");
        else
            PL()->FlashMessage("Can't add vertex at this location.");
        return (false);
    }

    // Pointing at wire but not at existing vertex, add one.
    Wire wire;
    Point *npts = new Point[num+1];
    wire.points = npts;
    wire.attributes = wrdesc->attributes();
    int j;
    for (j = 0; j < secnum; j++)
        npts[j] = pts[j];
    npts[j].set(px.x, px.y);
    for (j++; j <= num; j++)
        npts[j] = pts[j-1];
    wire.numpts = num + 1;

    Errs()->init_error();
    CDw *neww = cursd->newWire(wrdesc, &wire, wrdesc->ldesc(),
        wrdesc->prpty_list(), false);
    if (!neww) {
        Errs()->add_error("newWire failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        return (false);
    }

    mark_vertices(ERASE);
    if (neww->state() != CDobjDeleted)
        Selections.replaceObject(cursd, wrdesc, neww);

    Ulist()->CommitChanges(true);
    ED()->purgeObjectList(wrdesc);
    mark_vertices(DISPLAY);

    // save operation for undo
    UndoList = new sUndoV(UndoList);
    UndoList->newo = neww;
    UndoList->old = wrdesc;
    UndoList->x = px.x;
    UndoList->y = px.y;
    UndoList->operation = V_NEWV;

    // clear any redo operations
    sUndoV::destroy(RedoList);
    RedoList = 0;

    return (true);
}


// Allocate a new wire.  Free the older partially complete wire.
//
bool
WireState::allocate_wire(int width, WireStyle style)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);
    CD()->SetNotStrict(true);
    CDw *neww;
    Errs()->init_error();

    CDl *ld = LT()->CurLayer();
    if (!ld)
        return (false);
    if (ld->isInvisible()) {
        Log()->ErrorLog(mh::ObjectCreation,
            "Creation of wire on invisible layer not allowed.");
        return (false);
    }
    Wire wire(width,
        (DSP()->CurMode() == Physical ? style : CDWIRE_EXTEND),
        NumPts, Point::dup(Points, NumPts));
    if (cursd->makeWire(ld, &wire, &neww) != CDok) {
        CD()->SetNotStrict(false);
        Errs()->add_error("makeWire failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        return (false);
    }
    CD()->SetNotStrict(false);
    neww->set_state(CDobjIncomplete);
    RdBB = neww->oBB();
    RdBB.bloat(width);

    delete_inc();
    DSP()->SetIncompleteObject(neww);

    if (!cursd->isSymbolic()) {
        ScedIf()->install(neww, cursd, true);
        NumPts = neww->numpts();
        delete [] Points;
        Points = Point::dup(neww->points(), NumPts);
    }
    return (true);
}
// End of WireState methods


namespace {
    bool
    mark_vertices(bool DisplayOrErase)
    {
        if (DisplayOrErase == ERASE) {
            DSP()->EraseMarks(MARK_BOX);
            return (false);
        }
        bool found = false;
        sSelGen sg(Selections, CurCell(), "w", false);
        CDo *od;
        while ((od = sg.next()) != 0) {
            found = true;
            const Point *pts = OWIRE(od)->points();
            int num = OWIRE(od)->numpts();
            for (int i = 0; i < num; i++)
                DSP()->ShowBoxMark(DisplayOrErase, pts[i].x, pts[i].y,
                    HighlightingColor, MARK_SIZE, DSP()->CurMode());
        }
        return (found);
    }
}


// Command to change the end style of selected wires.
//
void
cEdit::execWireStyle()
{
    if (noEditing())
        return;
    if (!XM()->CheckCurMode(Physical))
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, false, Physical))
        return;

    bool found = false;
    sSelGen sg(Selections, CurCell(), "w");
    CDw *wd;
    while ((wd = (CDw*)sg.next()) != 0) {
        if (!wd->is_normal())
            continue;
        if (wd->state() == CDobjSelected && wd->ldesc()->isSelectable()) {
            found = true;
            break;
        }
    }
    if (!found)
        return;
    Ulist()->ListCheck("style", CurCell(), true);
    if (WireCmd)
        mark_vertices(ERASE);
    for ( ; wd; wd = (CDw*)sg.next()) {
        if (!wd->is_normal())
            continue;
        if (wd->state() == CDobjSelected && wd->ldesc()->isSelectable()) {
            // Change selected wires to new end style.
            int num = wd->numpts();
            Wire wire(num, Point::dup(wd->points(), num), wd->attributes());
            wire.set_wire_style(ed_wire_style);
            Errs()->init_error();
            CDw *neww = CurCell()->newWire(wd, &wire, wd->ldesc(),
                wd->prpty_list(), false);
            if (!neww) {
                Errs()->add_error("newWire failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                continue;
            }
            sg.replace(neww);
        }
    }
    if (WireCmd) {
        // save operation for undo
        WireCmd->UndoList = new sUndoV(WireCmd->UndoList);
        WireCmd->UndoList->operation = V_STY;
        WireCmd->copy_objlist();
        // clear any redo operations
        sUndoV::destroy(WireCmd->RedoList);
        WireCmd->RedoList = 0;

        mark_vertices(DISPLAY);
        WireCmd->message();
    }

    Ulist()->CommitChanges(true);
    XM()->ShowParameters();
    if (WireCmd)
        Ulist()->ListCheck(WireCmd->StateName, CurCell(), true);
}


namespace {
    // Width command mode
    enum WWstate { WWnone, WWset, WWchange };

    WWstate width_state;
}

// Command to change the width of wires, or set the default width.
// If wires have been selected (on the current layer in layer specific
// mode) then their new width is prompted for, and the change made,
// if possible.  Otherwise, the user is prompted for the default wire
// width to use for the current layer.
//
void
cEdit::execWireWidth()
{
    if (noEditing())
        return;
    if (!XM()->CheckCurMode(Physical))
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(false, false, Physical))
        return;

    char buf[128], tbuf[256];
    int ndgt = CD()->numDigits();
    if (CurCell() && !CurCell()->isImmutable()) {
        bool found = false;
        sSelGen sg(Selections, CurCell(), "w");
        CDw *wd;
        while ((wd = (CDw*)sg.next()) != 0) {
            if (!wd->is_normal())
                continue;
            if (wd->state() == CDobjSelected && wd->ldesc()->isSelectable()) {
                found = true;
                break;
            }
        }
        if (found) {
            // There is a wire selected
            int width = 0;
            for (;;) {
                sprintf(buf, "%.*f", ndgt,
                    MICRONS(dsp_prm(LT()->CurLayer())->wire_width()));
                strcpy(tbuf, "Enter new width for selected wires: ");
                width_state = WWchange;
                char *in = PL()->EditPrompt(tbuf, buf);
                width_state = WWnone;
                if (!in) {
                    PL()->ErasePrompt();
                    return;
                }
                if (!LT()->CurLayer()) {
                    PL()->ShowPrompt("No current layer!");
                    return;
                }
                double d;
                if (sscanf(in, "%lg", &d) == 1 && d >= 0) {
                    width = INTERNAL_UNITS(d);
                    break;
                }
            }

            Ulist()->ListCheck("width", CurCell(), true);
            if (WireCmd)
                mark_vertices(ERASE);
            for ( ; wd; wd = (CDw*)sg.next()) {
                if (!wd->is_normal())
                    continue;
                if (wd->state() == CDobjSelected &&
                        wd->ldesc()->isSelectable()) {
                    int num = wd->numpts();
                    Wire wire(num, Point::dup(wd->points(), num),
                        wd->attributes());
                    wire.set_wire_width(width);
                    Errs()->init_error();
                    CDw *neww = CurCell()->newWire(wd, &wire, wd->ldesc(),
                        wd->prpty_list(), false);
                    if (!neww) {
                        Errs()->add_error("newWire failed");
                        Log()->ErrorLog(mh::ObjectCreation,
                            Errs()->get_error());
                        continue;
                    }
                    if (!CurCell()->mergeWire(neww, true)) {
                        Errs()->add_error("mergeWire failed");
                        Log()->ErrorLog(mh::ObjectCreation,
                            Errs()->get_error());
                    }
                    if (neww->state() != CDobjDeleted)
                        sg.replace(neww);
                }
            }
            if (WireCmd) {
                // save operation for undo
                WireCmd->UndoList = new sUndoV(WireCmd->UndoList);
                WireCmd->UndoList->operation = V_WID;
                WireCmd->copy_objlist();
                // clear any redo operations
                sUndoV::destroy(WireCmd->RedoList);
                WireCmd->RedoList = 0;

                mark_vertices(DISPLAY);
                WireCmd->message();
            }
            else
                PL()->ErasePrompt();

            Ulist()->CommitChanges(true);
            XM()->ShowParameters();
            if (WireCmd)
                Ulist()->ListCheck(WireCmd->StateName, CurCell(), true);
            return;
        }
    }

    strcpy(tbuf, "Wire width for current layer: ");
    for (;;) {
        sprintf(buf, "%.*f", ndgt,
            MICRONS(dsp_prm(LT()->CurLayer())->wire_width()));
        width_state = WWset;
        char *in = PL()->EditPrompt(tbuf, buf);
        width_state = WWnone;
        if (in && *in) {
            if (!LT()->CurLayer()) {
                PL()->ShowPrompt("No current layer!");
                return;
            }
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d >= 0) {
                int width = INTERNAL_UNITS(d);
                dsp_prm(LT()->CurLayer())->set_wire_width(width);
                break;
            }
            else {
                strcpy(tbuf, "Bad value, try again: ");
                continue;
            }
        }
        break;
    }
    PL()->ErasePrompt();
}


// This is called whenever a new current layer is selected.
// It updates the prompt if the width command is active.
// Function supports both width and wire commands.
//
void
cEdit::widthCallback()
{
    if (!LT()->CurLayer())
        return;
    char buf[128], tbuf[256];
    if (width_state != WWnone) {
        int ndgt = CD()->numDigits();
        sprintf(buf, "%.*f", ndgt,
            MICRONS(dsp_prm(LT()->CurLayer())->wire_width()));
        if (width_state == WWchange)
            strcpy(tbuf, "CHANGE WIDTH of selected wires to: ");
        else if (width_state == WWset)
            strcpy(tbuf, "Wire width for current layer: ");
        else
            return;
        PL()->EditPrompt(tbuf, buf, PLedUpdate);
    }
    else if (WireCmd)
        WireCmd->message();
}


//----------------
// Ghost Rendering

// Show the current segment of a wire as it is being constructed.  Handle
// the connection indication.
//
void
cEditGhost::showGhostWireSeg(int new_x, int new_y, int ref_x, int ref_y)
{
    showGhostPathSeg(new_x, new_y, ref_x, ref_y);
}


// Show the wire as an outline, for use by the ghost drawing
// functions.
//
void
cEditGhost::showGhostWire(Wire *wire)
{
    if (wire->numpts == 0)
        return;
    if (wire->wire_width() == 0) {
        Gst()->ShowGhostPath(wire->points, wire->numpts);
        return;
    }
    Point *polypts;
    int polynum;
    if (wire->toPoly(&polypts, &polynum)) {
        Gst()->ShowGhostPath(polypts, polynum);
        delete [] polypts;
    }
}

