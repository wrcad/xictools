
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
 $Id: stretch.cc,v 1.40 2017/03/14 01:26:34 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "edit.h"
#include "undolist.h"
#include "vtxedit.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "tech.h"
#include "events.h"
#include "menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "ghost.h"
//#include "texttf.h"


//
// Commands to stretch objects.  Also contains vertex editor code.
//

//
// The stretch command and operation support.
//

namespace {
    namespace ed_stretch {
        struct StrState : public CmdState
        {
            StrState(const char*, const char*);
            virtual ~StrState();

            void setCaller(GRobject c)  { Caller = c; }

            void b1down();
            void b1up();
            void desel();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();
            void message()
                {
                    if (Level == 1)
                        PL()->ShowPrompt(msg1);
                    else if (Level == 2) {
                        PL()->ShowPrompt(
                            sObj::empty(ED()->objectList()) ? msg2 : msg4);
                    }
                    else
                        PL()->ShowPrompt(msg3);
                }

        private:
            void SetLevel1(bool show) { Level = 1; if (show) message(); }
            void SetLevel2(bool show) { Level = 2; if (show) message(); }
            void SetLevel3() { Level = 3; message(); }

            static int MsgTimeout(void*);

            GRobject Caller;      // button
            sObj *Objlist_back;   // backup for undo
            bool Vselect;         // true when selecting vertices
            bool ShiftResponse;   // true to print message for Shift
            bool GhostOn;         // true when showing ghost
            bool GotOne;          // true if stretchable object selected
            char Types[8];        // stretchable objects
            int State;            // internal state variable
            int Refx, Refy;       // original point
            int Newx, Newy;       // new point
            CDol *OrigObjs;       // original objects

            static const char *msg1;
            static const char *msg2;
            static const char *msg3;
            static const char *msg4;
        };

        StrState *StrCmd;
    }
}

using namespace ed_stretch;

const char *StrState::msg1 = "Select objects to modify.";
const char *StrState::msg2 =
    "Click on vertex, or select vertices to stretch (hold Shift).";
const char *StrState::msg3 = "Click to finalize stretch.";
const char *StrState::msg4 = "Select more vertices, or drag to new location.";


// Menu command to stretch objects to new dimensions.  For boxes,
// the corner closest to where the user pointed is moved.  Similar
// for labels, and the text is adjusted to the new aspect ratio.
// For polygons and wires, the closest vertex is moved.
//
void
cEdit::stretchExec(CmdDesc *cmd)
{
    if (StrCmd) {
        StrCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, false, DSP()->CurMode()))
        return;

    StrCmd = new StrState("STRETCH", "xic:strch");
    StrCmd->setCaller(cmd ? cmd->caller : 0);
    // Original objects are selected after undo.
    Ulist()->ListCheck(StrCmd->Name(), CurCell(), true);
    if (!EV()->PushCallback(StrCmd)) {
        delete StrCmd;
        StrCmd = 0;
        return;
    }
    StrCmd->message();
    ds.clear();
}


StrState::StrState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    Objlist_back = 0;
    Vselect = false;
    ShiftResponse = false;
    GhostOn = true;
    Types[0] = CDPOLYGON;
    Types[1] = CDWIRE;
    Types[2] = CDBOX;
    Types[3] = CDLABEL;
    Types[4] = '\0';
    GotOne = Selections.hasTypes(CurCell(), Types, false);
    State = 0;
    Refx = Refy = 0;
    Newx = Newy = 0;
    OrigObjs = 0;
    if (!GotOne)
        SetLevel1(false);
    else
        SetLevel2(false);

    EV()->SetConstrained(false);
    ED()->setStretchBoxCode(0);
    ED()->clearObjectList();
}


StrState::~StrState()
{
    StrCmd = 0;
    CDol::destroy(OrigObjs);
}


void
StrState::b1down()
{
    if (Level == 1) {
        ShiftResponse = false;
        cEventHdlr::sel_b1down();
    }
    else if (Level == 2) {
        if (EV()->Cursor().get_downstate() & GR_SHIFT_MASK) {
            // select vertices to move
            Vselect = true;
            cEventHdlr::sel_b1down();
            return;
        }
        // Identify the closest vertex, and begin the stretch.
        //
        EV()->Cursor().get_xy(&Refx, &Refy);
        if (sObj::empty(ED()->objectList()))
            ED()->setStretchRef(&Refx, &Refy);
        XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
        State = 2;
        Selections.setShowSelected(CurCell(), Types, false);
        Gst()->SetGhostAt(GFstretch, Refx, Refy);
        GhostOn = true;
        ShiftResponse = true;
        EV()->DownTimer(GFstretch);
    }
    else if (Level == 3) {
        // Finish the stretch.
        //
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        BBox nBB;
        if (ED()->doStretch(Refx, Refy, &x, &y)) {
            Newx = x;
            Newy = y;
            GhostOn = false;
            sObj::mark_vertices(ED()->objectList(), ERASE);
            XM()->SetCoordMode(CO_ABSOLUTE);
            if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
                DSP()->ShowCellTerminalMarks(DISPLAY);
            State = 4;
        }
    }
}


void
StrState::b1up()
{
    if (Level == 1) {
        // Confirm the selection, and set up for stretch.
        //
        if (!cEventHdlr::sel_b1up(0, Types, 0))
            return;
        if (!Selections.hasTypes(CurCell(), Types, false))
            return;
        SetLevel2(true);
        State = 1;

        ED()->clearObjectList();
        sObj::destroy(Objlist_back);
        Objlist_back = 0;
    }
    else if (Level == 2) {
        if (Vselect) {
            Vselect = false;
            BBox BB;
            if (!cEventHdlr::sel_b1up(&BB, Types, B1UP_NOSEL))
                return;
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

            CDol *st = Selections.listQueue(CurCell());
            ED()->setObjectList(sObj::mklist(ED()->objectList(), st, &BB));
            CDol::destroy(st);
            sObj::mark_vertices(ED()->objectList(), DISPLAY);
            message();
            return;
        }
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
                BBox nBB;
                if (ED()->doStretch(Refx, Refy, &x, &y)) {
                    Newx = x;
                    Newy = y;
                    GhostOn = false;
                    sObj::mark_vertices(ED()->objectList(), ERASE);
                    XM()->SetCoordMode(CO_ABSOLUTE);
                    State = 3;
                    if (DSP()->CurMode() == Electrical &&
                            DSP()->ShowTerminals())
                        DSP()->ShowCellTerminalMarks(DISPLAY);
                    ShiftResponse = false;
                    SetLevel1(true);
                    return;
                }
            }
        }
        SetLevel3();
    }
    else if (Level == 3) {
        // Done.
        //
        if (State == 4) {
            State = 3;
            ShiftResponse = false;
            SetLevel1(true);
        }
    }
}


// Desel pressed, reset state.
//
void
StrState::desel()
{
    if (Level != 1)
        Gst()->SetGhost(GFnone);
    sObj::mark_vertices(ED()->objectList(), ERASE);
    ED()->clearObjectList();
    sObj::destroy(Objlist_back);
    Objlist_back = 0;
    if (Level == 1)
        cEventHdlr::sel_esc();
    else
        XM()->SetCoordMode(CO_ABSOLUTE);

    GotOne = false;
    State = 0;
    SetLevel1(true);
}


// Esc entered, clean up and quit.
//
void
StrState::esc()
{
    if (Level != 1)
        Gst()->SetGhost(GFnone);
    sObj::mark_vertices(ED()->objectList(), ERASE);
    ED()->clearObjectList();
    sObj::destroy(Objlist_back);
    if (!GotOne) {
        Selections.setShowSelected(CurCell(), Types, false);
        Selections.removeTypes(CurCell(), Types);
    }
    else
        Selections.setShowSelected(CurCell(), Types, true);
    if (Level == 1)
        cEventHdlr::sel_esc();
    else
        XM()->SetCoordMode(CO_ABSOLUTE);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


bool
StrState::key(int code, const char*, int)
{
    switch (code) {
    case SHIFTDN_KEY:
        DSPmainDraw(ShowGhost(ERASE))
        EV()->SetConstrained(true);
        if (ShiftResponse)
            PL()->ShowPromptV("Stretch %s.",
                Tech()->IsConstrain45() ? "unconstrained" : "constrained");
        DSPmainDraw(ShowGhost(DISPLAY))
        break;
    case SHIFTUP_KEY:
        DSPmainDraw(ShowGhost(ERASE))
        EV()->SetConstrained(false);
        if (ShiftResponse)
            PL()->ErasePrompt();
        DSPmainDraw(ShowGhost(DISPLAY))
        break;
    case BSP_KEY:
    case DELETE_KEY:
        if (Level == 2 && !ED()->objectList()) {
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


// Undo the stretch that was just done, or the last operation.
//
void
StrState::undo()
{
    if (Level == 1) {
        if (!cEventHdlr::sel_undo()) {
            // The undo system has placed the original objects into
            // the selection list, as selected.

            if (State == 3) {
                Gst()->SetGhostAt(GFstretch, Refx, Refy);
                sObj::mark_vertices(ED()->objectList(), DISPLAY);
                GhostOn = true;
                ShiftResponse = true;
                SetLevel3();
                return;
            }
            dspPkgIf()->RegisterTimeoutProc(1500, MsgTimeout, 0);
        }
    }
    else if (Level == 2) {
        // Undo the selection.
        //
        if (ED()->objectList()) {
            sObj::mark_vertices(ED()->objectList(), ERASE);
            sObj::destroy(Objlist_back);
            Objlist_back = ED()->objectList();
            ED()->setObjectList(0);
            return;
        }
        CDol::destroy(OrigObjs);
        OrigObjs = Selections.listQueue(CurCell(), Types, false);
        Selections.setShowSelected(CurCell(), Types, false);
        Selections.removeTypes(CurCell(), Types);
        if (State == 3)
            State = 4;
        SetLevel1(true);
    }
    else if (Level == 3) {
        // Undo the stretch in progress.
        //
        ShiftResponse = false;
        Gst()->SetGhost(GFnone);
        GhostOn = false;
        sObj::mark_vertices(ED()->objectList(), DISPLAY);
        XM()->SetCoordMode(CO_ABSOLUTE);
        Selections.setShowSelected(CurCell(), Types, true);
        SetLevel2(true);
    }
}


// Redo the last undone stretch, or last operation.
//
void
StrState::redo()
{
    if (Level == 1) {
        if (((State == 1 || State == 2) && !Ulist()->HasRedo()) ||
                (State == 4 && Ulist()->HasOneRedo())) {
            Selections.insertList(CurCell(), OrigObjs);
            Selections.setShowSelected(CurCell(), Types, true);
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
        // Restart the last unfinished stretch.
        //
        if (Objlist_back) {
            ED()->setObjectList(Objlist_back);
            Objlist_back = 0;
            sObj::mark_vertices(ED()->objectList(), DISPLAY);
            return;
        }
        if (State == 2 || State == 3) {
            Selections.setShowSelected(CurCell(), Types, false);
            Gst()->SetGhostAt(GFstretch, Refx, Refy);
            GhostOn = true;
            XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
            ShiftResponse = true;
            SetLevel3();
        }
    }
    else if (Level == 3) {
        // Redo last undo.
        //
        if (State == 3) {
            Gst()->SetGhost(GFnone);
            GhostOn = false;
            sObj::mark_vertices(ED()->objectList(), ERASE);
            XM()->SetCoordMode(CO_ABSOLUTE);
            ShiftResponse = false;
            SetLevel1(true);
            Ulist()->RedoOperation();
        }
    }
}


// Timer callback, reprint the command prompt after an undo/redo, which
// will display a message.
//
int
StrState::MsgTimeout(void*)
{
    if (StrCmd)
        StrCmd->message();
    return (0);
}
// End of StrState methods


// Return in pointers the vertex closest to the given coordinates, as
// obtained from objects in the selection list for the current cell. 
// Checks rectangle vertices as well as polys and wires.  If a
// rectangle vertex is closest, this function returns a code
// identifying the vertex (1 BL, 2 TL, 3 TR, 4 BR), otherwise 0 is
// returned.
//
void
cEdit::setStretchRef(int *x, int *y)
{
    ed_stretch_box_code = 0;
    CDo *pointer = 0;
    int indx = 0;
    double mind = 1e30;
    sSelGen sg(Selections, CurCell(), "bpwl");
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (!od->is_normal())
            continue;
        if (set_stretch_ref(od, x, y, &mind, &indx, &ed_stretch_box_code,
                false))
            pointer = od;
    }
    if (!pointer)
        return;
    set_stretch_ref(pointer, x, y, &mind, &indx, &ed_stretch_box_code, true);
}


// Handle the stretching of objects with selected vertices, used by the
// vertex editor for poly/wire creation.
//
bool
cEdit::doStretchObjList(int ref_x, int ref_y, int map_x, int map_y, bool reins)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);

    CDol *o0 = 0;
    for (sObj *obj = ed_object_list; obj; obj = obj->next_obj()) {
        if (obj->object())
            o0 = new CDol(obj->object(), o0);
    }
    CDmergeInhibit inh(o0);
    CDol::destroy(o0);

    for (sObj *obj = ed_object_list; obj; obj = obj->next_obj()) {
        Ochg *op = Ulist()->CurOp().obj_list();
        if (obj->object() && stretch(obj->object(), cursd, ref_x, ref_y,
                map_x, map_y, 0, obj->points())) {
            if (Selections.inQueue(cursd, obj->object())) {
                Selections.removeObject(cursd, obj->object());
                if (reins) {
                    // Insert new objects into the selection queue.  Have to
                    // be a little careful here, use a hash table to track
                    // deletions.
                    Ochg *ox = Ulist()->CurOp().obj_list();
                    SymTab tab(false, false);
                    while (ox && ox != op) {
                        if (ox->odel())
                            tab.add((unsigned long)ox->odel(), 0, false);
                        if (ox->oadd() && SymTab::get(&tab,
                                (unsigned long)ox->oadd()) == ST_NIL) {
                            Selections.insertObject(CurCell(), ox->oadd(),
                                true);
                        }
                        ox = ox->next_chg();
                    }
                }
            }
        }
        else {
            Errs()->add_error("stretch failed");
            Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
        }
    }
    return (true);
}


namespace {
    bool
    wire_ref(int refx, int refy, int *px, int *py)
    {
        CDw *owire = 0;
        sSelGen sg(Selections, CurCell(), "bpwl");
        CDo *od;
        while ((od = sg.next()) != 0) {
            if (od->type() != CDWIRE)
                return (false);
            if (owire)
                return (false);
            owire = (CDw*)od;
        }
        if (!owire)
            return (false);

        const Point *points = owire->points();
        int numpts = owire->numpts();
        const Point *p = Point::nearestVertex(points, numpts, refx, refy);
        if (p == points) {
            *px = points[1].x;
            *py = points[1].y;
            return (true);
        }
        if (p == points + numpts - 1) {
            *px = points[numpts - 2].x;
            *py = points[numpts - 2].y;
            return (true);
        }
        return (false);
    }
}


// Perform a stretch operation on the selected objects.
//
bool
cEdit::doStretch(int ref_x, int ref_y, int *map_x, int *map_y)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);

    Errs()->init_error();
    if (!sObj::empty(ed_object_list)) {
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
        doStretchObjList(ref_x, ref_y, *map_x, *map_y, false);
    }
    else {
        if (Tech()->IsConstrain45() ^ EV()->IsConstrained()) {
            int rx, ry;
            if (wire_ref(ref_x, ref_y, &rx, &ry))
                XM()->To45(rx, ry, map_x, map_y);
            else
                XM()->To45(ref_x, ref_y, map_x, map_y);
        }
        CDol *st = Selections.listQueue(cursd);
        CDmergeInhibit inh(st);
        CDol::destroy(st);
        sSelGen sg(Selections, cursd, "bpwl");
        CDo *od;
        while ((od = sg.next()) != 0) {
            if (stretch(od, cursd, ref_x, ref_y, *map_x, *map_y,
                    ed_stretch_box_code, 0)) {
                sg.remove();
            }
            else {
                Errs()->add_error("stretch failed");
                Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
            }
        }
    }

    if (Ulist()->HasChanged()) {
        Gst()->SetGhost(GFnone);
        if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
            DSP()->ShowCellTerminalMarks(ERASE);
        Ulist()->CommitChanges(true);
        return (true);
    }
    XM()->ShowParameters();
    return (false);
}


// Static function.
// Stretch the label boundary.
//
void
cEdit::stretch_label_vertex(Point *pts, int x, int y, int new_x, int new_y)
{
    Point *pp, *pn, *pa;
    double mind = 1e30;
    int i, indx = 0;
    for (pp = pts, i = 0; i < 4; pp++, i++) {
        double dx = pp->x - x;
        double dy = pp->y - y;
        double d = dx*dx + dy*dy;
        if (d < mind) {
            mind = d;
            indx = i;
        }
    }
    switch (indx) {
    case 0:
        pa = pts+2;
        pp = pts+3;
        pn = pts+1;
        break;
    case 1:
        pa = pts+3;
        pp = pts+0;
        pn = pts+2;
        break;
    case 2:
        pa = pts+0;
        pp = pts+1;
        pn = pts+3;
        break;
    default:
        pa = pts+1;
        pp = pts+2;
        pn = pts+0;
        break;
    }
    pts[indx].x += new_x - x;
    pts[indx].y += new_y - y;

    double dy = pts[indx].y - pa->y;
    double dx = pts[indx].x - pa->x;
    i = (int)((dy-dx)/2.0);

    pn->x = pts[indx].x + i;
    pn->y = pts[indx].y - i;
    pp->x = pa->x - i;
    pp->y = pa->y + i;
    pts[4] = pts[0];
}


// Static function.
// Stretch oBB to nBB.
//
void
cEdit::stretch_box_vertex(BBox *nBB, const BBox *oBB, int x, int y, int new_x,
    int new_y, int code)
{
    int indx=0;
    if (code == 0) {
        double mind = 1e30;
        double dx = oBB->left - x;
        double dy = oBB->bottom - y;
        double d = dx*dx + dy*dy;
        if (d < mind) {
            mind = d;
            indx = 1;
        }
        dy = oBB->top - y;
        d = dx*dx + dy*dy;
        if (d < mind) {
            mind = d;
            indx = 2;
        }
        dx = oBB->right - x;
        d = dx*dx + dy*dy;
        if (d < mind) {
            mind = d;
            indx = 3;
        }
        dy = oBB->bottom - y;
        d = dx*dx + dy*dy;
        if (d < mind) {
            mind = d;
            indx = 4;
        }
        code = indx;
    }
    if (nBB != oBB)
        *nBB = *oBB;

    switch (code) {
    case 1:
        nBB->left += new_x - x;
        nBB->bottom += new_y - y;
        break;
    case 2:
        nBB->left += new_x - x;
        nBB->top += new_y - y;
        break;
    case 3:
        nBB->right += new_x - x;
        nBB->top += new_y - y;
        break;
    case 4:
        nBB->right += new_x - x;
        nBB->bottom += new_y - y;
        break;
    }
    if (nBB->left > nBB->right)
        SwapInts(nBB->left, nBB->right);
    if (nBB->bottom > nBB->top)
        SwapInts(nBB->bottom, nBB->top);
}


// Private function to set up stretch references.
//
bool
cEdit::set_stretch_ref(CDo *odesc, int *x, int *y, double *mind, int *indx,
    int *code, bool mode)
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
        if (!mode) {
            // pass 1, identify nearest corner
            bool closer = false;
            double dx = odesc->oBB().left - *x;
            double dy = odesc->oBB().bottom - *y;
            double d = dx*dx + dy*dy;
            if (d < *mind) {
                *mind = d;
                *indx = 1;
                closer = true;
            }
            dy = odesc->oBB().top - *y;
            d = dx*dx + dy*dy;
            if (d < *mind) {
                *mind = d;
                *indx = 2;
                closer = true;
            }
            dx = odesc->oBB().right - *x;
            d = dx*dx + dy*dy;
            if (d < *mind) {
                *mind = d;
                *indx = 3;
                closer = true;
            }
            dy = odesc->oBB().bottom - *y;
            d = dx*dx + dy*dy;
            if (d < *mind) {
                *mind = d;
                *indx = 4;
                closer = true;
            }
            return (closer);
        }
        // pass2, set x,y to closest corner
        switch (*indx) {
        case 1:
            *x = odesc->oBB().left;
            *y = odesc->oBB().bottom;
            break;
        case 2:
            *x = odesc->oBB().left;
            *y = odesc->oBB().top;
            break;
        case 3:
            *x = odesc->oBB().right;
            *y = odesc->oBB().top;
            break;
        case 4:
            *x = odesc->oBB().right;
            *y = odesc->oBB().bottom;
            break;
        }
        *code = *indx;
        return (true);
    }
poly:
    {
        int num = ((const CDpo*)odesc)->numpts();
        const Point *pts = ((const CDpo*)odesc)->points();
        if (!mode) {
            // pass1, find nearest vertex
            int i;
            const Point *p;
            bool closer = false;
            for (p = pts, i = 0; i < num; p++, i++) {
                double dx = p->x - *x;
                double dy = p->y - *y;
                double d = dx*dx + dy*dy;
                if (d < *mind) {
                    *mind = d;
                    *indx = i;
                    closer = true;
                }
            }
            return (closer);
        }
        // pass2, set to nearest vertex
        *x = pts[*indx].x;
        *y = pts[*indx].y;
        return (true);
    }
wire:
    {
        int num = ((const CDw*)odesc)->numpts();
        const Point *pts = ((const CDw*)odesc)->points();
        if (!mode) {
            // pass1, find nearest vertex
            int i;
            const Point *p;
            bool closer = false;
            for (p = pts, i = 0; i < num; p++, i++) {
                double dx = p->x - *x;
                double dy = p->y - *y;
                double d = dx*dx + dy*dy;
                if (d < *mind) {
                    *mind = d;
                    *indx = i;
                    closer = true;
                }
            }
            return (closer);
        }
        // pass2, set to nearest vertex
        *x = pts[*indx].x;
        *y = pts[*indx].y;
        return (true);
    }
label:
    {
        BBox BB;
        Point *pts;
        if (!mode) {
            // pass1, find nearest corner/vertex
            double dx, dy, d;
            int i;
            Point *p;
            bool closer = false;
            BB.left = ((CDla*)odesc)->xpos();
            BB.bottom = ((CDla*)odesc)->ypos();
            BB.right = BB.left + ((CDla*)odesc)->width();
            BB.top = BB.bottom + ((CDla*)odesc)->height();
            Label::TransformLabelBB(((CDla*)odesc)->xform(), &BB, &pts);
            if (pts) {
                for (p = pts, i = 0; i < 4; p++, i++) {
                    dx = p->x - *x;
                    dy = p->y - *y;
                    d = dx*dx + dy*dy;
                    if (d < *mind) {
                        *mind = d;
                        *indx = i;
                        closer = true;
                    }
                }
                delete [] pts;
            }
            else {
                dx = BB.left - *x;
                dy = BB.bottom - *y;
                d = dx*dx + dy*dy;
                if (d < *mind) {
                    *mind = d;
                    *indx = 1;
                    closer = true;
                }
                dy = BB.top - *y;
                d = dx*dx + dy*dy;
                if (d < *mind) {
                    *mind = d;
                    *indx = 2;
                    closer = true;
                }
                dx = BB.right - *x;
                d = dx*dx + dy*dy;
                if (d < *mind) {
                    *mind = d;
                    *indx = 3;
                    closer = true;
                }
                dy = BB.bottom - *y;
                d = dx*dx + dy*dy;
                if (d < *mind) {
                    *mind = d;
                    *indx = 4;
                    closer = true;
                }
            }
            return (closer);
        }
        // pass2, ser to nearest corner/vertex
        BB.left = ((CDla*)odesc)->xpos();
        BB.bottom = ((CDla*)odesc)->ypos();
        BB.right = BB.left + ((CDla*)odesc)->width();
        BB.top = BB.bottom + ((CDla*)odesc)->height();
        Label::TransformLabelBB(((CDla*)odesc)->xform(), &BB, &pts);
        if (pts) {
            *x = pts[*indx].x;
            *y = pts[*indx].y;
            delete [] pts;
        }
        else {
            switch (*indx) {
            case 1:
                *x = BB.left;
                *y = BB.bottom;
                break;
            case 2:
                *x = BB.left;
                *y = BB.top;
                break;
            case 3:
                *x = BB.right;
                *y = BB.top;
                break;
            case 4:
                *x = BB.right;
                *y = BB.bottom;
                break;
            }
            *code = *indx;
        }
        return (true);
    }
inst:
    return (false);
}


// Private function to perform stretch.
//
bool
cEdit::stretch(CDo *odesc, CDs *sdesc, int refx, int refy, int mapx,
    int mapy, int code, Vtex *vtx)
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
        if (vtx) {
            Point *px0 = new Point[5];
            Point *px = px0;
            for (Vtex *v = vtx; v; v = v->v_next) {
                *px = v->v_p;
                if (v->v_movable) {
                    px->x += (mapx - refx);
                    px->y += (mapy - refy);
                }
                px++;
            }
            *px = px0[0];
            Poly poly = Poly(5, px0);
            CDpo *newo = sdesc->newPoly(odesc, &poly, odesc->ldesc(),
                odesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newPoly failed");
                return (false);
            }
            if (!sdesc->mergeBoxOrPoly(newo, true)) {
                Errs()->add_error("mergeBoxOrPoly failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
        }
        else {
            BBox BB;
            stretch_box_vertex(&BB, &odesc->oBB(), refx, refy, mapx, mapy,
                code);
            CDo *newo = sdesc->newBox(odesc, &BB, odesc->ldesc(),
                odesc->prpty_list());
            if (!newo) {
                Errs()->add_error("newBox failed");
                return (false);
            }
            if (!sdesc->mergeBoxOrPoly(newo, true)) {
                Errs()->add_error("mergeBoxOrPoly failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
        }
        return (true);
    }
poly:
    {
        int num = ((const CDpo*)odesc)->numpts();
        Poly poly(num, 0);
        if (vtx) {
            poly.points = new Point[poly.numpts];
            Point *px = poly.points;
            for (Vtex *v = vtx; v; v = v->v_next) {
                *px = v->v_p;
                // The final vertex movability isn't set!
                if (v->v_movable || (!v->v_next && vtx->v_movable)) {
                    px->x += (mapx - refx);
                    px->y += (mapy - refy);
                }
                px++;
            }
        }
        else {
            poly.points = Point::dup(((const CDpo*)odesc)->points(),
                poly.numpts);
            Point *p = Point::nearestVertex(poly.points, poly.numpts,
                refx, refy);
            if (poly.is_manhattan()) {
                Point *pp, *pn = p+1;
                if (pn == &poly.points[poly.numpts-1])
                    pn = poly.points;
                if (p == poly.points)
                    pp = &poly.points[poly.numpts - 2];
                else
                    pp = p - 1;
                Point pb[3];
                pb[0] = *pp;
                pb[1] = *p;
                pb[2] = *pn;
                p->x += mapx - refx;
                p->y += mapy - refy;
                if (pb[0].x == pb[1].x)
                    pp->x += mapx - refx;
                else
                    pp->y += mapy - refy;
                if (pb[2].x == pb[1].x)
                    pn->x += mapx - refx;
                else
                    pn->y += mapy - refy;
                poly.points[poly.numpts-1] = poly.points[0];

            }
            else {
                p->x += mapx - refx;
                p->y += mapy - refy;
                if (p == poly.points)
                    poly.points[poly.numpts-1] = *p;
            }
        }

        CDpo *newo = sdesc->newPoly((CDpo*)odesc, &poly, odesc->ldesc(),
            odesc->prpty_list(), false);
        if (!newo) {
            Errs()->add_error("newPoly failed");
            return (false);
        }
        if (!sdesc->mergeBoxOrPoly(newo, true)) {
            Errs()->add_error("mergeBoxOrPoly failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        }
        return (true);
    }
wire:
    {
        int num = ((const CDw*)odesc)->numpts();
        Wire wire(num, 0, ((const CDw*)odesc)->attributes());
        if (vtx) {
            wire.points = new Point[wire.numpts];
            Point *px = wire.points;
            for (Vtex *v = vtx; v; v = v->v_next) {
                *px = v->v_p;
                if (v->v_movable) {
                    px->x += (mapx - refx);
                    px->y += (mapy - refy);
                }
                px++;
            }
        }
        else {
            Point *points = Point::dup(((const CDw*)odesc)->points(),
                wire.numpts);
            Point *p = Point::nearestVertex(points, wire.numpts, refx, refy);
            p->x += mapx - refx;
            p->y += mapy - refy;
            if (wire.numpts == 2 && points[0].x == points[1].x &&
                    points[0].y == points[1].y) {
                // this would reduce to a single vertex, don't allow this
                delete [] points;
                return (false);
            }
            wire.points = points;
        }

        CDw *newo = sdesc->newWire((CDw*)odesc, &wire, odesc->ldesc(),
            odesc->prpty_list(), false);
        if (!newo) {
            Errs()->add_error("newWire failed");
            return (false);
        }

        if (DSP()->CurMode() == Electrical) {
            cTfmStack stk;
            stk.TPush();
            stk.TTranslate(mapx - refx, mapy - refy);

            // Look through the cell reference points.  If any
            // correspond to a wire vertex that was just moved,
            // transform the reference.
            //
            CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                int x, y;
                pn->get_schem_pos(&x, &y);
                if (refx == x && refy == y) {
                    stk.TPoint(&x, &y);
                    pn->set_schem_pos(x, y);
                }
            }
            sdesc->hyTransformStretch((CDw*)odesc, newo,
                refx, refy, mapx, mapy, true);
            stk.TPop();
        }
        if (!sdesc->mergeWire(newo, true)) {
            Errs()->add_error("mergeWire failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        }
        return (true);
    }
label:
    {
        Label label(((CDla*)odesc)->la_label());
        BBox BB;
        BB.left = label.x;
        BB.bottom = label.y;
        BB.right = BB.left + label.width;
        BB.top = BB.bottom + label.height;
        Point *pts;
        Label::TransformLabelBB(label.xform, &BB, &pts);
        BBox tBB;
        if (pts)
            stretch_label_vertex(pts, refx, refy, mapx, mapy);
        else {
            stretch_box_vertex(&tBB, &BB, refx, refy, mapx, mapy, code);
            int w = tBB.width();
            if (w > tBB.height())
                w = tBB.height();
            if (w <= 0) {
                Log()->PopUpErr("Can't set width/height to zero.");
                return (false);
            }
        }
        Label::InvTransformLabelBB(label.xform, &tBB, pts);
        if (pts)
            delete pts;
        label.x = tBB.left;
        label.y = tBB.bottom;
        label.width = tBB.width();
        label.height = tBB.height();

        if (label.xform & TXTF_HJC)
            label.x += label.width/2;
        else if (label.xform & TXTF_HJR)
            label.x += label.width;
        if (label.xform & TXTF_VJC)
            label.y += label.height/2;
        else if (label.xform & TXTF_VJT)
            label.y += label.height;

        CDla *newo = sdesc->newLabel((CDla*)odesc, &label, odesc->ldesc(),
            odesc->prpty_list(), true);
        if (!newo) {
            Errs()->add_error("newLabel failed");
            return (false);
        }
        if (DSP()->CurMode() == Electrical)
            sdesc->prptyLabelUpdate(newo, (CDla*)odesc);
        return (true);
    }
inst:
    Errs()->add_error("Can't stretch an instance.");
    return (false);
}


//----------------
// Ghost Rendering

namespace {
    void display_ghost_stretch(CDo *odesc, int refx, int refy,
        int mapx, int mapy, int code, Vtex *vtx)
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
            if (vtx) {
                Point *px0 = new Point[5];
                Point *px = px0;
                for (Vtex *v = vtx; v; v = v->v_next) {
                    *px = v->v_p;
                    if (v->v_movable) {
                        px->x += (mapx - refx);
                        px->y += (mapy - refy);
                    }
                    px++;
                }
                *px = px0[0];
                Gst()->ShowGhostPath(px0, 5);
                delete [] px0;
            }
            else {
                BBox BB, tBB = odesc->oBB();
                cEdit::stretch_box_vertex(&BB, &tBB, refx, refy, mapx, mapy,
                    code);
                Gst()->ShowGhostBox(BB.left, BB.bottom, BB.right, BB.top);
            }
            return;
        }
    poly:
        {
            int num = ((const CDpo*)odesc)->numpts();
            Poly po(num, 0);
            if (vtx) {
                po.points = new Point[po.numpts];
                Point *px = po.points;
                for (Vtex *v = vtx; v; v = v->v_next) {
                    *px = v->v_p;
                    // The final vertex movability isn't set!
                    if (v->v_movable || (!v->v_next && vtx->v_movable)) {
                        px->x += (mapx - refx);
                        px->y += (mapy - refy);
                    }
                    px++;
                }
                Gst()->ShowGhostPath(po.points, po.numpts);
            }
            else {
                po.points = Point::dup(((const CDpo*)odesc)->points(),
                    po.numpts);
                Point *p = Point::nearestVertex(po.points, po.numpts,
                    refx, refy);
                if (po.is_manhattan()) {
                    Point *pp, *pn = p+1;
                    if (pn == &po.points[po.numpts-1])
                        pn = po.points;
                    if (p == po.points)
                        pp = &po.points[po.numpts - 2];
                    else
                        pp = p - 1;
                    Point pb[3];
                    pb[0] = *pp;
                    pb[1] = *p;
                    pb[2] = *pn;
                    p->x += mapx - refx;
                    p->y += mapy - refy;
                    if (pb[0].x == pb[1].x)
                        pp->x += mapx - refx;
                    else
                        pp->y += mapy - refy;
                    if (pb[2].x == pb[1].x)
                        pn->x += mapx - refx;
                    else
                        pn->y += mapy - refy;
                    po.points[po.numpts-1] = po.points[0];
                    Gst()->ShowGhostPath(po.points, po.numpts);
                    *pp = pb[0];
                    *p = pb[1];
                    *pn = pb[2];
                    po.points[po.numpts-1] = po.points[0];
                }
                else {
                    p->x += mapx - refx;
                    p->y += mapy - refy;
                    if (p == po.points)
                        po.points[po.numpts-1] = *p;
                    Gst()->ShowGhostPath(po.points, po.numpts);
                    p->x -= mapx - refx;
                    p->y -= mapy - refy;
                    if (p == po.points)
                        po.points[po.numpts-1] = *p;
                }
            }
            delete [] po.points;
            return;
        }
    wire:
        {
            int num = ((const CDw*)odesc)->numpts();
            Wire wire(num, 0, ((const CDw*)odesc)->attributes());
            if (vtx) {
                wire.points = new Point[wire.numpts];
                Point *px = wire.points;
                for (Vtex *v = vtx; v; v = v->v_next) {
                    *px = v->v_p;
                    if (v->v_movable) {
                        px->x += (mapx - refx);
                        px->y += (mapy - refy);
                    }
                    px++;
                }
                EGst()->showGhostWire(&wire);
            }
            else {
                wire.points = Point::dup(((const CDw*)odesc)->points(),
                    wire.numpts);
                Point *p = Point::nearestVertex(wire.points, wire.numpts,
                    refx, refy);
                p->x += mapx - refx;
                p->y += mapy - refy;
                EGst()->showGhostWire(&wire);
                p->x -= mapx - refx;
                p->y -= mapy - refy;
            }
            delete [] wire.points;
            return;
        }
    label:
        {
            BBox BB;
            BB.left = ((CDla*)odesc)->xpos();
            BB.bottom = ((CDla*)odesc)->ypos();
            BB.right = BB.left + ((CDla*)odesc)->width();
            BB.top = BB.bottom + ((CDla*)odesc)->height();
            Point *pts;
            Label::TransformLabelBB(((CDla*)odesc)->xform(), &BB, &pts);
            if (pts) {
                cEdit::stretch_label_vertex(pts, refx, refy, mapx, mapy);
                Gst()->ShowGhostPath(pts, 5);
                delete [] pts;
            }
            else {
                BBox tBB;
                cEdit::stretch_box_vertex(&tBB, &BB, refx, refy, mapx, mapy,
                    code);
                Gst()->ShowGhostBox(tBB.left, tBB.bottom, tBB.right, tBB.top);
            }
            return;
        }
    inst:
        return;
    }


    struct ol_t
    {
        ol_t(CDo *o, Vtex *v, ol_t *n)
            {
                odesc = o;
                vtx = v;
                next = n;
            }

        static void destroy(ol_t *o)
            {
                while (o) {
                    ol_t *ox = o;
                    o = o->next;
                    delete ox;
                }
            }

        CDo *odesc;
        Vtex *vtx;
        ol_t *next;
    };


    // Return the object with the largest area from among the next
    // n items in the list, and advance the list.
    //
    ol_t *pick_one(ol_t **plist, int n)
    {
        double amx = 0.0;
        int i = 0;
        ol_t *opick = 0;
        while (*plist && i < n) {
            ol_t *ol = *plist;
            *plist = (*plist)->next;
            i++;
            double a = ol->odesc->area();
            if (a > amx) {
                amx = a;
                opick = ol;
            }
        }
        return (opick);
    }


    // The objects list for ghosting during stretch.
    ol_t *ghost_display_list;
}


// Function to show a ghost-drawn view of the geometry being stretched
// attached to the pointer.  Called from rubber banding function.
//
void
cEditGhost::showGhostStretch(int map_x, int map_y, int ref_x, int ref_y)
{
    if (!sObj::empty(ED()->objectList())) {
        if (Tech()->IsConstrain45() ^ EV()->IsConstrained()) {
            int rx, ry, xm, ym;
            if (ED()->get_wire_ref(&rx, &ry, &xm, &ym)) {
                int dx = ref_x - xm;
                int dy = ref_y - ym;
                map_x -= dx;
                map_y -= dy;
                XM()->To45(rx, ry, &map_x, &map_y);
                map_x += dx;
                map_y += dy;
            }
            else
                XM()->To45(ref_x, ref_y, &map_x, &map_y);
        }
        for (ol_t *ol = ghost_display_list; ol; ol = ol->next) {
            display_ghost_stretch(ol->odesc, ref_x, ref_y, map_x, map_y,
                0, ol->vtx);
        }
        return;
    }

    if (Tech()->IsConstrain45() ^ EV()->IsConstrained()) {
        int rx, ry;
        if (wire_ref(ref_x, ref_y, &rx, &ry))
            XM()->To45(rx, ry, &map_x, &map_y);
        else
            XM()->To45(ref_x, ref_y, &map_x, &map_y);
    }
    for (ol_t *ol = ghost_display_list; ol; ol = ol->next) {
        display_ghost_stretch(ol->odesc, ref_x, ref_y, map_x, map_y,
            ED()->stretchBoxCode(), 0);
    }
}


// Static function.
// Initialize/free the object list for ghosting during stretch.
//
void
cEditGhost::ghost_stretch_setup(bool on)
{
    if (on) {
        ol_t::destroy(ghost_display_list);  // should never need this
        ghost_display_list = 0;
        unsigned int n = 0;
        if (!sObj::empty(ED()->objectList())) {
            for (sObj *obj = ED()->objectList(); obj; obj = obj->next_obj()) {
                if (!obj->object())
                    continue;
                ghost_display_list =
                    new ol_t(obj->object(), obj->points(), ghost_display_list);
                n++;
            }
        }
        else {
            sSelGen sg(Selections, CurCell());
            CDo *od;
            while ((od = sg.next()) != 0) {
                ghost_display_list = new ol_t(od, 0, ghost_display_list);
                n++;
            }
        }
        if (n > EGst()->eg_max_ghost_objects) {
            // Too many objects to show efficiently.  Keep only the
            // objects with the largest area among groups.

            unsigned int n1 = n/EGst()->eg_max_ghost_objects;
            unsigned int n2 = n%EGst()->eg_max_ghost_objects;
            ol_t *o0 = ghost_display_list;
            ol_t *ol0 = 0;
            unsigned int j = 0, np;
            for (unsigned int i = 0; i < n; i += np) {
                np = n1 + (++j < n2);
                ol_t *od = pick_one(&o0, np);
                if (!od)
                    break;
                ol0 = new ol_t(od->odesc, od->vtx, ol0);
            }
            ol_t::destroy(ghost_display_list);
            ghost_display_list = ol0;
        }
    }
    else {
        ol_t::destroy(ghost_display_list);
        ghost_display_list = 0;
    }
}
// End of Stretch functions.
// End of cEdit functions.

