
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
 $Id: polygns.cc,v 1.96 2017/04/16 20:28:01 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "edit.h"
#include "undolist.h"
#include "scedif.h"
#include "drcif.h"
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


#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#define RADTODEG 57.29577951

namespace {
    void
    sides_cb(double sides, bool affirmed, void*)
    {
        if (affirmed) {
            int n = mmRnd(sides);
            if (DSP()->CurMode() == Electrical) {
                if (n != DEF_RoundFlashSides) {
                    char buf[32];
                    sprintf(buf, "%d", n);
                    CDvdb()->setVariable(VA_ElecRoundFlashSides, buf);
                }
                else
                    CDvdb()->clearVariable(VA_ElecRoundFlashSides);
            }
            else {
                if (n != DEF_RoundFlashSides) {
                    char buf[32];
                    sprintf(buf, "%d", n);
                    CDvdb()->setVariable(VA_RoundFlashSides, buf);
                }
                else
                    CDvdb()->clearVariable(VA_RoundFlashSides);
            }
        }
    }
}


// Menu command to change the number of segments per 360 degrees used
// to approximate round objects.
//
void
cEdit::sidesExec(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller)) {
        int x, y;
        Menu()->Location(cmd->caller, &x, &y);
        cmd->wdesc->Wbag()->PopUpNumeric(cmd->caller, GRloc(LW_XYA, x, y),
            "Number of sides for round objects? ",
            GEO()->roundFlashSides(DSP()->CurMode() == Electrical),
                MIN_RoundFlashSides,
                MAX_RoundFlashSides, 1.0, 0, sides_cb, cmd);
    }
}


namespace {
    bool wire_to_poly();
    bool mark_vertices(bool);

    int mark_idle(void*)
    {
        mark_vertices(DISPLAY);
        return (0);
    }

#define MARK_SIZE 8

    // Struct to keep track of operations for undo/redo
    //
    enum V_OP {V_CRT, V_NEWV, V_CHGV};
    // V_CRT   poly created
    // V_NEWV  single vertex added
    // V_CHGV  vertices moved or deleted


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
                list->free(false);
            }

        void free()
            {
                sUndoV *u = this;
                while (u) {
                    sUndoV *ux = u;
                    u = u->next;
                    delete ux;
                }
            }

        CDo *newo;       // new poly created
        CDo *old;        // old poly deleted
        Ochg *list;      // list of changes for V_CHGV
        int x, y;        // vertex location
        V_OP operation;  // operation
        sUndoV *next;
    };

    namespace ed_polygns {
        struct PolyState : public CmdState
        {
            PolyState(const char*, const char*);
            virtual ~PolyState();

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
                        PL()->ShowPrompt(Selections.hasTypes(
                            CurCell(), Types) ? msgA : msg);
                    else if (Level == 2)
                        PL()->ShowPrompt(msgB);
                }

        private:
            void copy_objlist();
            bool stretch(int, int, int*, int*);
            void delete_vertices();
            bool add_vertex();
            bool allocate_poly();

            void SetLevel1() { Level = 1; message(); }
            void SetLevel2() { Level = 2; }
            void SetLevel3() { Level = 3; }

            void GhostOn(int x, int y)
                {
                    PthX = x;
                    PthY = y;
                    PthOn = true;
                    ED()->pthSet(x, y);
                    Gst()->SetGhostAt(GFpathseg, x, y);
                }

            void GhostOff()
                {
                    PthOn = false;
                    Gst()->SetGhost(GFnone);
                }

            GRobject Caller;      // button
            int State;            // status number
            int NumPts;           // number of vertices currently in poly
            Point *Points;        // current vertex
            Plist *Phead;         // head of a list of vertices for undo
            int Firstx, Firsty;   // first vertex
            bool Override;        // Shift key down
            bool Simple45;        // Ctrl key down
            bool SelectingVertex; // set while in vertex selection
            bool SelectingPolys;  // set while selecting polys to edit
            sUndoV *UndoList;     // list of vertex operations for undo
            sUndoV *RedoList;     // list of vertex operations for redo
            char Types[4];        // selection types
            int Refx, Refy;       // stretch reference point
            int Newx, Newy;       // stretch-to point
            sObj *Objlist_back;   // backup for stretch undo
            BBox RdBB;            // bounding box for redraw
            int PthX, PthY;       // ghost state, need to reset after Shift or
            bool PthOn;           //  funny first point may occur

            static const char *msg;
            static const char *msgA;
            static const char *msgB;
        };

        PolyState *PolyCmd;
    }
}

using namespace ed_polygns;

const char *PolyState::msg =
    "Click to create vertices.  Close path to terminate polygon.";
const char *PolyState::msgA =
    "Click to create/delete vertices, hold Shift to select vertices to move.";
const char *PolyState::msgB =
    "Select more vertices, or drag or click twice to move selected vertices.";


void
cEdit::makePolygonsExec(CmdDesc *cmd)
{
    if (PolyCmd) {
        PolyCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;

    if (DSP()->CurMode() == Physical) {
        if (wire_to_poly())
            return;
    }
    PolyCmd = new PolyState("POLYGON", "xic:polyg");
    PolyCmd->setCaller(cmd ? cmd->caller : 0);
    Ulist()->ListCheck(PolyCmd->Name(), CurCell(), false);
    if (!EV()->PushCallback(PolyCmd)) {
        delete PolyCmd;
        return;
    }
    PolyCmd->message();
    ds.clear();
}


namespace {
    // Delete the poly under construction, and fix the cell's BB if
    // necessary.
    //
    bool
    delete_inc()
    {
        CDs *cursd = CurCell();
        if (!cursd)
            return (false);
        if (DSP()->IncompleteObject()) {
            cursd->unlink(DSP()->IncompleteObject(), false);
            DSP()->SetIncompleteObject(0);
            cursd->computeBB();
            return (true);
        }
        return (false);
    }
}


PolyState::PolyState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    State = 0;
    NumPts = 0;
    Points = 0;
    Phead = 0;
    Firstx = Firsty = 0;
    Override = false;
    Simple45 = false;
    SelectingVertex = false;
    SelectingPolys = false;
    UndoList = RedoList = 0;
    Types[0] = CDPOLYGON;
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


PolyState::~PolyState()
{
    PolyCmd = 0;
    DSP()->SetInEdgeSnappingCmd(false);
    delete [] Points;
}


namespace {
    // Return the spacing in screen pixels between the two points.
    //
    double
    pixdist(int x1, int y1, int x2, int y2)
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


// Button1 press handler for polygon creation.  Each press defines a
// vertex.  Pointing at the first vertex or the same vertex twice
// completes the polygon.  Button release events are ignored.
//
void
PolyState::b1down()
{
    static const char *msg1 =
        "Can't allow a polygon with fewer than 3 points.";
    static const char *msg2 = "Bad polygon, rejected by database.";

    if (Level == 2) {
        // Finish the stretch.
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        BBox nBB;
        if (stretch(Refx, Refy, &x, &y)) {
            Newx = x;
            Newy = y;
            XM()->SetCoordMode(CO_ABSOLUTE);
            SetLevel3();
        }
        return;
    }
    CDs *cursd = CurCell();
    if (!cursd)
        return;

    Objlist_back->free();
    Objlist_back = 0;
    RedoList->free();
    RedoList = 0;
    if (!NumPts)
        Ulist()->ListCheck(PolyCmd->StateName, cursd, false);

    while (Phead) {
        Plist *pn = Phead->next;
        delete Phead;
        Phead = pn;
    }

    State = 0;
    if (SelectingVertex || SelectingPolys) {
        cEventHdlr::sel_b1down();
        return;
    }

    if (!NumPts && !(EV()->Cursor().get_downstate() & GR_CONTROL_MASK)) {
        if (!ED()->objectList()->empty() &&
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

    int xc, yc;
    EV()->Cursor().get_xy(&xc, &yc);

    PL()->ShowPrompt(msg);
    int x, y;
    if (!NumPts) {
        Firstx = x = xc;
        Firsty = y = yc;
        delete [] Points;
        Points = new Point[1];
        Points->set(x, y);
        NumPts = 1;
        if (!allocate_poly())
            return;
        DSP()->ShowCrossMark(DISPLAY, x, y, HighlightingColor, 20,
            DSP()->CurMode());
        XM()->SetCoordMode(CO_RELATIVE, x, y);
        GhostOn(x, y);
        return;
    }
    GhostOff();
    XM()->SetCoordMode(CO_ABSOLUTE);
    ED()->pthGet(&x, &y);

    // See note in 45s.cc about logic.
    if ((!Tech()->IsConstrain45() && (Override || Simple45)) ||
            (Tech()->IsConstrain45() && !Override)) {
        // reg45 or simple45
        if (xc == Points[NumPts-1].x && yc == Points[NumPts-1].y) {
            XM()->SetCoordMode(CO_RELATIVE, x, y);
            GhostOn(x, y);
            return;
        }
        Points = Points->append(&NumPts, x, y);
        if (!Simple45) {
            // Only add the second point if it is 8 pixels or more
            // away, otherwise it is too easy to add spurious
            // vertices.
            if (pixdist(x, y, xc, yc) >= 8.0) {
                x = xc;
                y = yc;
                Points = Points->append(&NumPts, x, y);
            }
        }
    }
    else {
        if (xc == Points[NumPts-1].x && yc == Points[NumPts-1].y) {
            x = Firstx;
            y = Firsty;
        }
        else {
            x = xc;
            y = yc;
        }
        Points = Points->append(&NumPts, x, y);
    }
    if (x == Firstx && y == Firsty) {
        if (NumPts <= 3) {
            if (delete_inc())
                DSP()->RedisplayArea(&RdBB);
            PL()->ShowPrompt(msg1);
        }
        else {
            Points = Points->dup(NumPts);
            Poly poly(NumPts, Points->dup(NumPts));

            // Be sure poly checking is on.
            int pchk_flags;
            bool tmp_nc = CD()->IsNoPolyCheck();
            CD()->SetNoPolyCheck(false);

            CDl *ld = LT()->CurLayer();

            CDpo *newp;
            if (cursd->makePolygon(ld, &poly, &newp, &pchk_flags) != CDok) {
                if (delete_inc())
                    DSP()->RedisplayArea(&RdBB);
                Errs()->add_error("newPoly failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                PL()->ShowPrompt(msg2);
            }
            else {
                delete_inc();
                Ulist()->RecordObjectChange(cursd, 0, newp);
                if (pchk_flags & PCHK_REENT) {
                    Log()->WarningLog(mh::ObjectCreation,
                    "You have just created a reentrant or otherwise\n"
                    "degenerate polygon.  You should probably delete this\n"
                    "and start over.");
                }
                else if (!cursd->mergeBoxOrPoly(newp, true)) {
                    Errs()->add_error("mergeBoxOrPoly failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
                Ulist()->CommitChanges();

                // add a placeholder in the UndoList
                UndoList = new sUndoV(UndoList);
                // clear any redo operations
                RedoList->free();
                RedoList = 0;
            }
            CD()->SetNoPolyCheck(tmp_nc);
        }
        DSP()->ShowCrossMark(ERASE, Firstx, Firsty, HighlightingColor,
            20, DSP()->CurMode());
        DSP()->RedisplayArea(&RdBB);
        NumPts = 0;
    }
    else {
        if (!allocate_poly())
            return;
        DSPmainDraw(SetColor(dsp_prm(LT()->CurLayer())->pixel()))
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (wdesc->IsSimilar(DSP()->MainWdesc()))
                wdesc->ShowPath(Points, NumPts, false);
        }
        XM()->SetCoordMode(CO_RELATIVE, x, y);
        GhostOn(x, y);
    }
}


void
PolyState::b1up()
{
    if (SelectingPolys) {
        BBox BB;
        CDol *slist;
        if (cEventHdlr::sel_b1up(&BB, Types, &slist)) {
            mark_vertices(ERASE);
            for (CDol *sl = slist; sl; sl = sl->next) {
                if (sl->odesc->type() != CDPOLYGON)
                    continue;
                if (sl->odesc->state() == CDSelected)
                    Selections.removeObject(CurCell(), sl->odesc);
                else
                    Selections.insertObject(CurCell(), sl->odesc);
            }
            mark_vertices(DISPLAY);
            slist->free();
        }
        SelectingPolys = false;
        message();
        return;
    }
    if (SelectingVertex) {
        BBox BB;
        CDol *slist;
        if (!cEventHdlr::sel_b1up(&BB, Types, &slist))
            return;
        SelectingVertex = false;
        ED()->objectList()->mark_vertices(ERASE);

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

        ED()->setObjectList(ED()->objectList()->mklist(slist, &BB));
        if (!ED()->objectList()->empty()) {
            ED()->objectList()->mark_vertices(DISPLAY);
            PL()->ShowPrompt(msgB);
        }
        else if (!add_vertex())
            mark_vertices(DISPLAY);
        slist->free();
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
                BBox nBB;
                if (stretch(Refx, Refy, &x, &y)) {
                    Newx = x;
                    Newy = y;
                    XM()->SetCoordMode(CO_ABSOLUTE);
                    SetLevel1();
                    State = 2;
                    return;
                }
            }
        }
        // Make it appear that the last click was over the reference
        // vertex, for ^E
    }
    if (Level == 3) {
        SetLevel1();
        State = 2;
    }
}


// Desel pressed, reset.
//
void
PolyState::desel()
{
    if (SelectingVertex || SelectingPolys)
        cEventHdlr::sel_esc();
    mark_vertices(ERASE);
    ED()->clearObjectList();
    Objlist_back->free();
    Objlist_back = 0;
    UndoList->free();
    UndoList = 0;
    RedoList->free();
    RedoList = 0;
}


// Esc entered, abort polygon creation.
//
void
PolyState::esc()
{
    if (!ED()->objectList()->empty()) {
        GhostOff();
        ED()->objectList()->mark_vertices(ERASE);
        Objlist_back->free();
        Objlist_back = ED()->objectList();
        ED()->setObjectList(0);
        SetLevel1();
        State = 0;
        mark_vertices(DISPLAY);
        return;
    }

    if (SelectingVertex || SelectingPolys)
        cEventHdlr::sel_esc();
    else
        GhostOff();

    XM()->SetCoordMode(CO_ABSOLUTE);
    PL()->ErasePrompt();
    mark_vertices(ERASE);
    ED()->clearObjectList();
    EV()->SetConstrained(false);
    Objlist_back->free();

    // Go through the undo list.  For polys that are in the database
    // that were created by adding a vertex, remove inline vertices. 
    // We want to avoid inline vertices in the database (OpenAccess
    // will not accept them), however we allow these while vertex
    // editing, since the user may move the new inline vertex to a new
    // location.
    //
    for (sUndoV *ul = UndoList; ul; ul = ul->next) {
        if (ul->operation == V_NEWV && ul->newo->in_db())
            ((CDpo*)ul->newo)->po_check_poly(0, false);
    }

    UndoList->free();
    RedoList->free();
    if (delete_inc()) {
        DSP()->RedisplayArea(&RdBB);
        DSP()->ShowCrossMark(ERASE, Firstx, Firsty, HighlightingColor,
            20, DSP()->CurMode());
    }
    EV()->PopCallback(this);
    if (Caller)
        Menu()->Deselect(Caller);
    delete this;
}


// Handle keypresses.  The Shift key alters the constraint for both
// vertex stretching and path addition.  The Ctrl key sets the "simple"
// 45 mode
//
bool
PolyState::key(int code, const char*, int)
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
        if (Level == 1 && !NumPts) {
            SelectingPolys = true;
            PL()->ShowPrompt("Click or drag to select polys for editing.");
        }
        break;
    case DELETE_KEY:
        delete_vertices();
        break;
    case BSP_KEY:
        if (!ED()->objectList()->empty()) {
            GhostOff();
            ED()->objectList()->mark_vertices(ERASE);
            Objlist_back->free();
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


// Undo the vertices previously placed during polygon creation.
//
void
PolyState::undo()
{
    if (Level == 2) {
        GhostOff();
        XM()->SetCoordMode(CO_ABSOLUTE);
        SetLevel1();
        return;
    }
    if (!ED()->objectList()->empty()) {
        ED()->objectList()->mark_vertices(ERASE);
        Objlist_back->free();
        Objlist_back = ED()->objectList();
        ED()->setObjectList(0);
        mark_vertices(DISPLAY);
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
                    BBox BB(CDnullBB);;
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
                dspPkgIf()->RegisterIdleProc(mark_idle, 0);
            }
            else {
                // hit the end of the (truncated) undo list
                UndoList->free();
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
    Points = Points->remove_last(&NumPts);
    BBox oldBB = RdBB;
    if (!NumPts) {
        delete_inc();
        DSP()->ShowCrossMark(ERASE, Firstx, Firsty, HighlightingColor,
            20, DSP()->CurMode());
        DSP()->RedisplayArea(&oldBB);
    }
    else {
        if (!allocate_poly())
            return;
        DSP()->RedisplayArea(&oldBB);
        XM()->SetCoordMode(CO_RELATIVE, Points[NumPts-1].x, Points[NumPts-1].y);
        GhostOn(Points[NumPts-1].x, Points[NumPts-1].y);
    }
    PL()->ShowPrompt(msg);
}


// Add back the polygon vertices previously undone.
//
void
PolyState::redo()
{
    if (Ulist()->HasRedo()) {
        mark_vertices(ERASE);
        Ulist()->RedoOperation();
        if (RedoList) {
            sUndoV *v_op = RedoList;
            RedoList = v_op->next;
            CDs *cursd = CurCell();
            if (v_op->list) {
                BBox BB(CDnullBB);
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
        dspPkgIf()->RegisterIdleProc(mark_idle, 0);
        return;
    }

    if (Level == 2)
        return;
    if (!Objlist_back->empty()) {
        ED()->setObjectList(Objlist_back);
        Objlist_back = 0;
        mark_vertices(ERASE);
        ED()->objectList()->mark_vertices(DISPLAY);
        return;
    }
    if (State >= 1 && !ED()->objectList()->empty()) {
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
            Points = Points->append(&NumPts, x, y);
            if (!allocate_poly())
                return;
            DSPmainDraw(SetColor(dsp_prm(LT()->CurLayer())->pixel()))
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0) {
                if (wdesc->IsSimilar(DSP()->MainWdesc()))
                    wdesc->ShowPath(Points, NumPts, false);
            }
        }
        XM()->SetCoordMode(CO_RELATIVE, x, y);
        GhostOn(x, y);
        PL()->ShowPrompt(msg);
        return;
    }
}


// Copy the current object list for undo.  Zero out objects that are
// added and deleted in the list.  These are freed in CommitChanges(),
// so would otherwise be bad pointers.
//
void
PolyState::copy_objlist()
{
    UndoList->list = Ulist()->CurOp().obj_list()->copy();
    for (Ochg *oc1 = UndoList->list; oc1; oc1 = oc1->next_chg()) {
        if (oc1->oadd() && oc1->oadd()->state() == CDDeleted) {
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
PolyState::stretch(int ref_x, int ref_y, int *map_x, int *map_y)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);

    bool ret = false;
    if (!ED()->objectList()->empty()) {
        if (Tech()->IsConstrain45() ^ EV()->IsConstrained())
            XM()->To45(ref_x, ref_y, map_x, map_y);

        ED()->doStretchObjList(ref_x, ref_y, *map_x, *map_y, true);

        if (Ulist()->HasChanged()) {
            Gst()->SetGhost(GFnone);
            ED()->objectList()->mark_vertices(ERASE);
            ED()->clearObjectList();

            // save operation for undo
            UndoList = new sUndoV(UndoList);
            UndoList->operation = V_CHGV;
            copy_objlist();
            // clear any redo operations
            RedoList->free();
            RedoList = 0;

            mark_vertices(DISPLAY);
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
PolyState::delete_vertices()
{
    if (ED()->objectList()->empty())
        return;
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    mark_vertices(ERASE);
    sObj *onext;
    for (sObj *o = ED()->objectList(); o; o = onext) {
        onext = o->next_obj();
        if (o->object()->type() != CDPOLYGON)
            continue;
        CDpo *podesc = (CDpo*)o->object();
        Point *pts = new Point[podesc->numpts()];
        int npts = 0;
        for (Vtex *v = o->points(); v; v = v->v_next) {
            if (v->v_movable)
                continue;
            pts[npts].x = v->v_p.x;
            pts[npts].y = v->v_p.y;
            npts++;
        }
        if (npts < 4 || npts == podesc->numpts()) {
            delete [] pts;
            continue;
        }

        Poly poly;
        poly.points = pts;
        poly.numpts = npts;
        CDpo *newp = cursd->newPoly(podesc, &poly, podesc->ldesc(),
            podesc->prpty_list(), false);
        if (!newp) {
            Errs()->add_error("newPoly failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            return;
        }
        if (!cursd->mergeBoxOrPoly(newp, true)) {
            Errs()->add_error("mergeBoxOrPoly failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        }
        if (newp->state() != CDDeleted)
            Selections.replaceObject(cursd, podesc, newp);
        ED()->purgeObjectList(podesc);
    }
    ED()->clearObjectList();

    // save operation for undo
    UndoList = new sUndoV(UndoList);
    UndoList->operation = V_CHGV;
    copy_objlist();

    // clear any redo operations
    RedoList->free();
    RedoList = 0;

    mark_vertices(DISPLAY);
    Ulist()->CommitChanges(true);
}


// Add a single vertex, if possible, at the last button press
// location.
//
bool
PolyState::add_vertex()
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
    CDpo *podesc = 0;

    for (CDol *s = slist; s; s = s->next) {
        if (s->odesc->state() == CDSelected &&
                Selections.inQueue(cursd, s->odesc)) {
            podesc = OPOLY(s->odesc);
            break;
        }
    }
    slist->free();
    if (!podesc)
        return (false);

    const Point *pts = podesc->points();
    int num = podesc->numpts();
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
            if ((pts+i-1)->inPath(&px, delta, 0, 2)) {
                secnum = i;
                break;
            }
        }
    }
    if (secnum == 0) {
        // Shouldn't happen.
        return (false);
    }

    // Pointing at poly edge but not at existing vertex, add one.
    Poly poly;
    Point *npts = new Point[num+1];
    poly.points = npts;
    int j;
    for (j = 0; j < secnum; j++)
        npts[j] = pts[j];
    npts[j].set(px.x, px.y);
    for (j++; j <= num; j++)
        npts[j] = pts[j-1];
    poly.numpts = num + 1;

    Errs()->init_error();

    // Allow a new inline vertex (these would otherwise be
    // removed).  The user will want to move to a new location. 
    // Any not moved will be fixed when the command exits.
    CD()->SetNotStrict(true);

    CDpo *newp = cursd->newPoly(podesc, &poly, podesc->ldesc(),
        podesc->prpty_list(), false);
    CD()->SetNotStrict(false);
    if (!newp) {
        Errs()->add_error("newPoly failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        return (false);
    }
    mark_vertices(ERASE);
    if (!cursd->mergeBoxOrPoly(newp, true)) {
        Errs()->add_error("mergeBoxOrPoly failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
    }
    if (newp->state() != CDDeleted)
        Selections.replaceObject(cursd, podesc, newp);

    Ulist()->CommitChanges(true);
    ED()->purgeObjectList(podesc);
    mark_vertices(DISPLAY);

    // save operation for undo
    UndoList = new sUndoV(UndoList);
    UndoList->newo = newp;
    UndoList->old = podesc;
    UndoList->x = px.x;
    UndoList->y = px.y;
    UndoList->operation = V_NEWV;

    // clear any redo operations
    RedoList->free();
    RedoList = 0;

    return (true);
}


// Create a new incomplete polygon, and delete the previous one,
// if any.  Called after each new vertex is added.
//
bool
PolyState::allocate_poly()
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);
    CD()->SetNotStrict(true);
    Errs()->init_error();
    CDpo *newp;

    CDl *ld = LT()->CurLayer();
    if (!ld)
        return (false);
    if (ld->isInvisible()) {
        Log()->ErrorLog(mh::ObjectCreation,
            "Creation of polygon on invisible layer not allowed.");
        return (false);
    }
    Poly poly(NumPts, Points->dup(NumPts));
    if (cursd->makePolygon(ld, &poly, &newp) != CDok) {
        CD()->SetNotStrict(false);
        Errs()->add_error("makePolygon failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        return (false);
    }
    CD()->SetNotStrict(false);
    newp->set_state(CDIncomplete);
    RdBB = newp->oBB();

    delete_inc();
    DSP()->SetIncompleteObject(newp);
    return (true);
}
// End of PolyState functions.


namespace {
    // Give the user the option of converting selected wires to polygons.
    // Return true if a conversion is performed.
    //
    bool
    wire_to_poly()
    {
        if (DSP()->CurMode() != Physical)
            return (false);
        CDs *cursd = CurCell();
        if (!cursd)
            return (false);
        bool found = false;
        sSelGen sg(Selections, cursd, "w");
        CDw *owire;
        while ((owire = (CDw*)sg.next()) != 0) {
            if (owire->wire_width() > 0) {
                found = true;
                break;
            }
        }
        if (!found)
            return (false);

        char *in = PL()->EditPrompt(
            "Convert selected wires to polygons? ", "n");
        in = lstring::strip_space(in);
        if (!in)
            // important - return true so polyg command exits, get here if
            // another command button was pressed
            return (true);
        if (*in != 'y' && *in != 'Y')
            return (false);

        Ulist()->ListCheck("wr2poly", cursd, false);
        sg = sSelGen(Selections, cursd, "w");
        while ((owire = (CDw*)sg.next()) != 0) {
            if (owire->wire_width() > 0) {
                Poly poly;
                if (owire->w_toPoly(&poly.points, &poly.numpts)) {
                    cursd->newPoly(owire, &poly, owire->ldesc(), 0, false);
                    sg.remove();
                }
            }
        }
        if (Ulist()->CommitChanges(true)) {
            PL()->ShowPrompt("Selected wires converted to polygons.");
            return (true);
        }
        XM()->ShowParameters();
        return (false);
    }


    bool
    mark_vertices(bool DisplayOrErase)
    {
        if (DisplayOrErase == ERASE) {
            DSP()->EraseMarks(MARK_BOX);
            return (false);
        }
        bool found = false;
        sSelGen sg(Selections, CurCell(), "p", false);
        CDo *od;
        while ((od = sg.next()) != 0) {
            found = true;
            const Point *pts = OPOLY(od)->points();
            int num = OPOLY(od)->numpts();
            for (int i = 0; i < num-1; i++)
                DSP()->ShowBoxMark(DisplayOrErase, pts[i].x, pts[i].y,
                    HighlightingColor, MARK_SIZE, DSP()->CurMode());
        }
        return (found);
    }
}


/*========================================================================*
 *
 * Disks, donuts, arcs.
 *
 *========================================================================*/

namespace {
    const char *ROUND_CMD = "ROUND";
    const char *DONUT_CMD = "DONUT";
    const char *ARC_CMD   = "ARC";

    namespace ed_polygns {

        // Constraint type for ellipse creation
        enum RndHoldType {RndHoldNone, RndHoldX, RndHoldY};

        // Code for last object created
        enum Rtype {Rnone, Rdisk, Rdonut, Rarc};

        // State variables shared by disk, donut, and arc functions.
        struct RoundState : public CmdState
        {
            friend void cEditGhost::showGhostDisk(int, int, int, int, bool);
            friend void cEditGhost::showGhostDonut(int, int, int, int, bool);
            friend void cEditGhost::showGhostArc(int, int, int, int, bool);

            RoundState(const char*, const char*);
            virtual ~RoundState();

            void setup(GRobject c, Rtype w)
                {
                    Caller = c;
                    Which = w;
                }

            // atan2 with check for domain
            //
            double arctan(int y, int x)
                {
                    if (x == 0 && y == 0)
                        return (0.0);
                    double ang = atan2((double)y, (double)x);
                    if (Ctrl) {
                        if (ang > 0.0)
                            ang = (M_PI/4)*(int)((ang + M_PI/8)/(M_PI/4));
                        else
                            ang = (M_PI/4)*(int)((ang - M_PI/8)/(M_PI/4));
                    }
                    return (ang);
                }

            int compute_radius(int x, int y)
                {
                    double x2 = x*(double)x;
                    double y2 = y*(double)y;
                    if (Ctrl) {
                        if (x2 < y2)
                            x2 = 0.0;
                        else
                            y2 = 0.0;
                    }
                    return ((int)sqrt(x2 + y2));
                }

            void b1down();
            void b1up();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();

            bool makedisk(int, int);
            bool makedonut(int, int);
            bool makearc(double);

            void message()
                {
                    if (Level == 1)
                        PL()->ShowPrompt(fmsg1);
                    else if (Level == 2)
                        PL()->ShowPrompt(Which == Rdisk ? fmsg2 : dmsg2);
                    else if (Level == 3)
                        PL()->ShowPrompt(dmsg3);
                    else if (Level == 4)
                        PL()->ShowPrompt(amsg4);
                    else
                        PL()->ShowPrompt(amsg5);
                }

        private:
            void SetLevel1(bool show)
                {
                    Level = 1;
                    Mode = RndHoldNone;
                    AcceptKey = 0;
                    if (show)
                        message();
                }

            void SetLevel2()
                {
                    Level = 2;
                    message();
                }

            void SetLevel3()
                {
                    Level = 3;
                    message();
                }

            void SetLevel4()
                {
                    Level = 4;
                    AcceptKey = 0;
                    Mode = RndHoldNone;
                    message();
                }

            void SetLevel5()
                {
                    Level = 5;
                    AcceptKey = 0;
                    Mode = RndHoldNone;
                    message();
                }

            GRobject Caller;
            int State;
            int Cx, Cy;
            int Rad1x, Rad1y;
            int Rad2x, Rad2y;
            int AcceptKey;
            double Ang1;
            bool ArcMode;
            RndHoldType Mode;
            Rtype Last;
            Rtype Which;
            bool Ctrl;

            static const char *fmsg1;
            static const char *fmsg2;
            static const char *dmsg2;
            static const char *dmsg3;
            static const char *amsg4;
            static const char *amsg5;
        };

        RoundState *RoundCmd;
    }
}

const char *RoundState::fmsg1 =
    "Click on center, or drag to define center and perimeter.";
const char *RoundState::fmsg2 = "Click on perimeter.";
const char *RoundState::dmsg2 =
    "Click on inner radius, or drag to define inner and outer radii.";
const char *RoundState::dmsg3 = "Click on outer radius.";
const char *RoundState::amsg4 =
    "Click on start of clockwise arc, or drag to also define end.";
const char *RoundState::amsg5 = "Click on end of clockwise arc.";


// Menu command for disk creation.  If the user pushes the shift key
// after the center is located and before the perimeter is located,
// the current radius is held for x, or y depending on whether the
// pointer was closer to center y or center x, respectively.  This
// allows ellipses to be created.
//
void
cEdit::makeDisksExec(CmdDesc *cmd)
{
    if (RoundCmd) {
        RoundCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;

    RoundCmd = new RoundState(ROUND_CMD, "xic:round");
    RoundCmd->setup(cmd ? cmd->caller : 0, Rdisk);
    Ulist()->ListCheck(RoundCmd->Name(), CurCell(), false);
    if (!EV()->PushCallback(RoundCmd)) {
        delete RoundCmd;
        return;
    }
    RoundCmd->message();
    ds.clear();
}


// Menu command for annular ring generation.  If the shift key is held
// while the first perimeter point is defined, elliptical rings are
// generated.
//
void
cEdit::makeDonutsExec(CmdDesc *cmd)
{
    if (RoundCmd) {
        RoundCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;

    RoundCmd = new RoundState(DONUT_CMD, "xic:donut");
    RoundCmd->setup(cmd ? cmd->caller : 0, Rdonut);
    Ulist()->ListCheck(RoundCmd->Name(), CurCell(), false);
    if (!EV()->PushCallback(RoundCmd)) {
        delete RoundCmd;
        return;
    }
    RoundCmd->message();
    ds.clear();
}


// Menu command for arc creation.  If the shift key is held while the
// first perimeter point is defined, elliptical arc sections are
// generated.
//
void
cEdit::makeArcsExec(CmdDesc *cmd)
{
    if (RoundCmd) {
        RoundCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;

    RoundCmd = new RoundState(ARC_CMD, "xic:arc");
    RoundCmd->setup(cmd ? cmd->caller : 0, Rarc);
    Ulist()->ListCheck(RoundCmd->Name(), CurCell(), false);
    if (!EV()->PushCallback(RoundCmd)) {
        delete RoundCmd;
        return;
    }
    RoundCmd->message();
    ds.clear();
}


RoundState::RoundState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    State = 0;
    Cx = Cy = 0;
    Rad1x = Rad1y = 0;
    Rad2x = Rad2y = 0;
    Ang1 = 0.0;
    ArcMode = false;
    Last = Rnone;
    Which = Rnone;
    Ctrl = false;
    DSP()->SetInEdgeSnappingCmd(true);

    SetLevel1(false);
}


RoundState::~RoundState()
{
    RoundCmd = 0;
    DSP()->SetInEdgeSnappingCmd(false);
}


void
RoundState::b1down()
{
    if (Level == 1) {
        // Define the center point.
        //
        EV()->Cursor().get_xy(&Cx, &Cy);
        DSP()->ShowCrossMark(DISPLAY, Cx, Cy, HighlightingColor,
            20, DSP()->CurMode());
        XM()->SetCoordMode(CO_RELATIVE, Cx, Cy);
        Gst()->SetGhostAt(GFdisk, Cx, Cy);
        // start looking for shift press
        AcceptKey = 1;
        State = 1;
        EV()->DownTimer(GFdisk);
    }
    else if (Level == 2) {
        // Obtain radius.
        //
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        int rad = compute_radius(x - Cx, y - Cy);
        if (Mode == RndHoldX)
            Rad1y = rad;
        else if (Mode == RndHoldY)
            Rad1x = rad;
        else
            Rad1x = Rad1y = rad;
        if (rad) {
            if (Which == Rdisk) {
                State = 1;
                if (makedisk(Rad1x, Rad1y))
                    State = 6;
            }
            else {
                Gst()->SetGhost(GFnone);
                Gst()->SetGhostAt(GFdonut, Cx, Cy);
                AcceptKey = 2;
                State = 2;
                EV()->DownTimer(GFdonut);
            }
        }
    }
    else if (Level == 3) {
        // Define the periphery of the second surface.
        //
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        int rad = compute_radius(x - Cx, y - Cy);
        if (Mode == RndHoldX)
            Rad2y = rad;
        else if (Mode == RndHoldY)
            Rad2x = rad;
        else
            Rad2x = Rad2y = rad;

        if (Rad2x == Rad1x && Rad2y == Rad1y) {
            // if the radii are equal, make a disk
            State = 2;
            if (makedisk(Rad1x, Rad1y))
                State = 6;
        }
        else if ((double)(Rad2x - Rad1x)*(Rad2y - Rad1y) > 0.0) {
            if (Which == Rdonut) {
                State = 2;
                if (makedonut(Rad2x, Rad2y))
                    State = 6;
            }
            else if ((Rad2x == 0 && Rad2y == 0) ||
                    CDvdb()->getVariable(VA_NoConstrainRound) ||
                    DrcIf()->donutEval(Rad1x, Rad1y, Rad2x, Rad2y,
                    LT()->CurLayer())) {
                Gst()->SetGhost(GFnone);
                ArcMode = false;
                Gst()->SetGhostAt(GFarc, Cx, Cy);
                State = 3;
                EV()->DownTimer(GFarc);
            }
        }
    }
    else if (Level == 4) {
        // Set the arc anchor point.
        //
        Gst()->SetGhost(GFnone);
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        Ang1 = arctan(y - Cy, x - Cx);
        ArcMode = true;
        State = 4;
        Gst()->SetGhostAt(GFarc, Cx, Cy);
        EV()->DownTimer(GFarc);
    }
    else if (Level == 5) {
        // Create arc.
        //
        State = 4;
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        if (makearc(arctan(y - Cy, x - Cx)))
            State = 6;
    }
}


void
RoundState::b1up()
{
    if (Level == 1) {
        // Define the periphery of the first surface, if possible. 
        // Otherwise set state to define first periphery on next
        // button 1 press.
        //
        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok() &&
                EV()->CurrentWin()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            int rad = compute_radius(x - Cx, y - Cy);
            if (Mode == RndHoldX)
                Rad1y = rad;
            else if (Mode == RndHoldY)
                Rad1x = rad;
            else
                Rad1x = Rad1y = rad;
            if (rad) {
                if (Which == Rdisk) {
                    if (makedisk(Rad1x, Rad1y)) {
                        State = 2;
                        Mode = RndHoldNone;
                        AcceptKey = 0;
                        return;
                    }
                }
                else {
                    Gst()->SetGhost(GFnone);
                    SetLevel3();
                    Gst()->SetGhostAt(GFdonut, Cx, Cy);
                    State = 2;
                    return;
                }
            }
        }
        SetLevel2();
    }
    else if (Level == 2) {
        // If the first periphery was successfully defined, define
        // the periphery of the second surface and create the donut
        // if possible.  If the second periphery is bogus, set state
        // to wait for next button 1 press.
        //
        if (State == 6) {
            State = 2;
            SetLevel1(true);
            return;
        }
        if (State == 2) {
            if (!EV()->UpTimer() && EV()->Cursor().is_release_ok() &&
                    EV()->CurrentWin()) {
                int x, y;
                EV()->Cursor().get_release(&x, &y);
                EV()->CurrentWin()->Snap(&x, &y);
                int rad = compute_radius(x - Cx, y - Cy);
                if (Mode == RndHoldX)
                    Rad2y = rad;
                else if (Mode == RndHoldY)
                    Rad2x = rad;
                else
                    Rad2x = Rad2y = rad;

                if (Rad2x == Rad1x && Rad2y == Rad1y) {
                    // if the radii are equal, make a disk
                    if (makedisk(Rad1x, Rad1y)) {
                        SetLevel1(true);
                        return;
                    }
                }
                else if ((double)(Rad2x - Rad1x)*(Rad2y - Rad1y) > 0.0) {
                    if (Which == Rdonut) {
                        if (makedonut(Rad2x, Rad2y)) {
                            State = 3;
                            SetLevel1(true);
                            return;
                        }
                    }
                    else if ((Rad2x == 0 && Rad2y == 0) ||
                            CDvdb()->getVariable(VA_NoConstrainRound) ||
                            DrcIf()->donutEval(Rad1x, Rad1y, Rad2x, Rad2y,
                            LT()->CurLayer())) {
                        Gst()->SetGhost(GFnone);
                        // ghost off, safe to reset Rad1
                        SetLevel4();
                        ArcMode = false;
                        Gst()->SetGhostAt(GFarc, Cx, Cy);
                        State = 3;
                        return;
                    }
                }
            }
            SetLevel3();
        }
    }
    else if (Level == 3) {
        // If the second periphery was defined, define the arc anchor
        // point if the button release occurred in a window.
        //
        if (State == 6) {
            State = 3;
            SetLevel1(true);
            return;
        }
        if (State == 3) {
            if (!EV()->UpTimer() && EV()->Cursor().is_release_ok() &&
                    EV()->CurrentWin()) {
                int x, y;
                EV()->Cursor().get_release(&x, &y);
                EV()->CurrentWin()->Snap(&x, &y);
                Ang1 = arctan(y - Cy, x - Cx);
                Gst()->SetGhost(GFnone);
                SetLevel5();
                ArcMode = true;
                Gst()->SetGhostAt(GFarc, Cx, Cy);
                State = 4;
                return;
            }
            SetLevel4();
        }
    }
    else if (Level == 4) {
        // Finish the arc, if possible, if the anchor point was set. 
        // If not created, wait for next button 1 press.
        //
        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok() &&
                EV()->CurrentWin()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            if (makearc(arctan(y - Cy, x - Cx))) {
                State = 5;
                SetLevel1(true);
                return;
            }
        }
        SetLevel5();
    }
    else if (Level == 5) {
        // If arc was created, reset state to start next arc.
        //
        if (State == 6) {
            State = 5;
            SetLevel1(true);
        }
    }
}


// Esc entered, clean up and exit.
//
void
RoundState::esc()
{
    DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor, 20,
        DSP()->CurMode());
    Gst()->SetGhost(GFnone);
    XM()->SetCoordMode(CO_ABSOLUTE);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


// Look for shift keypress for ellipse mode.
//
bool
RoundState::key(int code, const char*, int)
{
    switch (code) {
    case SHIFTDN_KEY:
        if (!AcceptKey)
            return (false);
        DSPmainDraw(ShowGhost(ERASE))
        int x, y;
        if (XM()->WhereisPointer(&x, &y)) {
            WindowDesc *wdesc =
                EV()->CurrentWin() ? EV()->CurrentWin() : DSP()->MainWdesc();
            wdesc->Snap(&x, &y);
            int rad = compute_radius(x - Cx, y - Cy);
            int rx = x - Cx;
            if (rx < 0)
                rx = -rx;
            int ry = y - Cy;
            if (ry < 0)
                ry = -ry;
            if (rx > ry) {
                if (AcceptKey == 2)
                    Rad2y = rad;
                else
                    Rad1y = rad;
                Mode = RndHoldY;
            }
            else {
                if (AcceptKey == 2)
                    Rad2x = rad;
                else
                    Rad1x = rad;
                Mode = RndHoldX;
            }
        }
        DSPmainDraw(ShowGhost(DISPLAY))
        break;
    case SHIFTUP_KEY:
        if (!AcceptKey)
            return (false);
        DSPmainDraw(ShowGhost(ERASE))
        Mode = RndHoldNone;
        DSPmainDraw(ShowGhost(DISPLAY))
        break;
    case CTRLDN_KEY:
        DSPmainDraw(ShowGhost(ERASE))
        Ctrl = true;
        DSPmainDraw(ShowGhost(DISPLAY))
        break;
    case CTRLUP_KEY:
        DSPmainDraw(ShowGhost(ERASE))
        Ctrl = false;
        DSPmainDraw(ShowGhost(DISPLAY))
        break;
    case RETURN_KEY:
        if (Level == 2) {
            DSPmainDraw(ShowGhost(ERASE))
            char *in = PL()->EditPrompt( "Radius in microns? ", "");
            if (!RoundCmd)
                return (false);
            if (!in) {
                DSPmainDraw(ShowGhost(DISPLAY))
                break;
            }
            double drx, dry;
            int n = sscanf(in, "%lf %lf", &drx, &dry);
            if (n < 1 || drx <= 0.0 || (n > 1 && dry <= 0)) {
                DSPmainDraw(ShowGhost(DISPLAY))
                break;
            }
            if (n == 1)
                dry = drx;
            int rx = INTERNAL_UNITS(drx);
            int ry = INTERNAL_UNITS(dry);
            if (Which == Rdisk) {
                if (makedisk(rx, ry)) {
                    State = 2;
                    Mode = RndHoldNone;
                    AcceptKey = 0;
                    break;
                }
            }
            else {
                Rad1x = rx;
                Rad1y = ry;
                Gst()->SetGhost(GFnone);
                SetLevel3();
                Gst()->SetGhostAt(GFdonut, Cx, Cy);
                State = 2;
            }
            DSPmainDraw(ShowGhost(DISPLAY))
        }
        else if (Level == 3) {
            DSPmainDraw(ShowGhost(ERASE))
            char *in = PL()->EditPrompt( "Outer radius in microns? ", "");
            if (!RoundCmd)
                return (false);
            if (!in) {
                DSPmainDraw(ShowGhost(DISPLAY))
                break;
            }
            double drx, dry;
            int n = sscanf(in, "%lf %lf", &drx, &dry);
            if (n < 1 || drx < 0.0 || (n > 1 && dry < 0)) {
                DSPmainDraw(ShowGhost(DISPLAY))
                break;
            }
            if (n == 1)
                dry = drx;
            int rx = INTERNAL_UNITS(drx);
            int ry = INTERNAL_UNITS(dry);

            if (rx == Rad1x && ry == Rad1y) {
                // if the radii are equal, make a disk
                if (makedisk(rx, ry)) {
                    SetLevel1(true);
                    break;
                }
            }
            else if ((double)(rx - Rad1x)*(ry - Rad1y) > 0.0) {
                if (Which == Rdonut) {
                    if (makedonut(rx, ry)) {
                        State = 3;
                        SetLevel1(true);
                        break;
                    }
                }
                else {
                    if ((rx == 0 && ry == 0) ||
                            CDvdb()->getVariable(VA_NoConstrainRound) ||
                            DrcIf()->donutEval(Rad1x, Rad1y, rx, ry,
                            LT()->CurLayer())) {
                        Gst()->SetGhost(GFnone);
                        Rad2x = rx;
                        Rad2y = ry;
                        SetLevel4();
                        ArcMode = false;
                        Gst()->SetGhostAt(GFarc, Cx, Cy);
                        State = 3;
                        break;
                    }
                }
            }
            DSPmainDraw(ShowGhost(DISPLAY))
        }
        else if (Level == 4) {
            DSPmainDraw(ShowGhost(ERASE))
            char *in = PL()->EditPrompt( "Start angle degrees? ", "");
            if (!RoundCmd)
                return (false);
            if (!in) {
                DSPmainDraw(ShowGhost(DISPLAY))
                break;
            }
            double d;
            if (sscanf(in, "%lf", &d) != 1) {
                DSPmainDraw(ShowGhost(DISPLAY))
                break;
            }
            Ang1 = -d*M_PI/180.0;
            Gst()->SetGhost(GFnone);
            SetLevel5();
            ArcMode = true;
            Gst()->SetGhostAt(GFarc, Cx, Cy);
            State = 4;
            DSPmainDraw(ShowGhost(DISPLAY))
        }
        else if (Level == 5) {
            DSPmainDraw(ShowGhost(ERASE))
            char *in = PL()->EditPrompt( "End angle degrees? ", "");
            if (!RoundCmd)
                return (false);
            if (!in) {
                DSPmainDraw(ShowGhost(DISPLAY))
                break;
            }
            double d;
            if (sscanf(in, "%lf", &d) != 1) {
                DSPmainDraw(ShowGhost(DISPLAY))
                break;
            }
            if (makearc(-d*(M_PI/180.0))) {
                State = 5;
                SetLevel1(true);
                break;
            }
            DSPmainDraw(ShowGhost(DISPLAY))
        }
    }
    return (false);
}


void
RoundState::undo()
{
    if (Level == 1) {
        // Undo the creation just made, or the last operation.
        //
        Ulist()->UndoOperation();
        if (Which == Rdisk || Last == Rdisk) {
            if (State == 2) {
                DSP()->ShowCrossMark(DISPLAY, Cx, Cy, HighlightingColor,
                    20, DSP()->CurMode());
                XM()->SetCoordMode(CO_RELATIVE, Cx, Cy);
                Gst()->SetGhostAt(GFdisk, Cx, Cy);
                // start looking for shift press
                AcceptKey = 1;
                SetLevel2();
                return;
            }
        }
        else if (Which == Rdonut || Last == Rdonut) {
            if (State == 3) {
                DSP()->ShowCrossMark(DISPLAY, Cx, Cy, HighlightingColor,
                    20, DSP()->CurMode());
                XM()->SetCoordMode(CO_RELATIVE, Cx, Cy);
                Gst()->SetGhostAt(GFdonut, Cx, Cy);
                AcceptKey = 2;
                SetLevel3();
                return;
            }
        }
        else if (Which == Rarc || Last == Rarc) {
            if (State == 5) {
                DSP()->ShowCrossMark(DISPLAY, Cx, Cy, HighlightingColor,
                    20, DSP()->CurMode());
                ArcMode = true;
                XM()->SetCoordMode(CO_RELATIVE, Cx, Cy);
                Gst()->SetGhostAt(GFarc, Cx, Cy);
                SetLevel5();
                return;
            }
        }
    }
    else if (Level == 2) {
        // Undo the center definition, start over.
        //
        if (Which == Rdisk) {
            if (State == 2)
                State = 3;
        }
        else if (Which == Rdonut) {
            if (State == 3)
                State = 4;
        }
        else if (Which == Rarc) {
            if (State == 5)
                State = 6;
        }
        else
            return;
        DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor, 20,
            DSP()->CurMode());
        Gst()->SetGhost(GFnone);
        XM()->SetCoordMode(CO_ABSOLUTE);
        AcceptKey = 0;
        SetLevel1(true);
    }
    else if (Level == 3) {
        // Undo the first surface definition.
        //
        Gst()->SetGhost(GFnone);
        SetLevel2();
        Gst()->SetGhostAt(GFdisk, Cx, Cy);
        AcceptKey = 1;
    }
    else if (Level == 4) {
        // Undo second surface definition.
        //
        Gst()->SetGhost(GFnone);
        SetLevel3();
        Gst()->SetGhostAt(GFdonut, Cx, Cy);
        AcceptKey = 2;
    }
    else if (Level == 5) {
        // Undo anchor point.
        //
        Gst()->SetGhost(GFnone);
        SetLevel4();
        ArcMode = false;
        Gst()->SetGhostAt(GFarc, Cx, Cy);
    }
}


// Redo the last undo, or operation.
//
void
RoundState::redo()
{
    int T = 0;
    if (Level == 1) {
        if (Which == Rdisk || Last == Rdisk)
            T = 3;
        else if (Which == Rdonut || Last == Rdonut)
            T = 4;
        else if (Which == Rarc || Last == Rarc)
            T = 6;
        if (T) {
            if ((State == 1 && !Ulist()->HasRedo()) ||
                    (State == T && Ulist()->HasOneRedo())) {
                if (State == T)
                    State = T-1;
                DSP()->ShowCrossMark(DISPLAY, Cx, Cy, HighlightingColor,
                    20, DSP()->CurMode());
                SetLevel2();
                AcceptKey = 1;
                XM()->SetCoordMode(CO_RELATIVE, Cx, Cy);
                Gst()->SetGhostAt(GFdisk, Cx, Cy);
                return;
            }
        }
        Ulist()->RedoOperation();
    }
    else if (Level == 2) {
        // Redo first periphery definition, or operation.
        //
        if (Which == Rdisk && State == 2) {
            Gst()->SetGhost(GFnone);
            XM()->SetCoordMode(CO_ABSOLUTE);
            DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor,
                20, DSP()->CurMode());
            Ulist()->RedoOperation();
            SetLevel1(true);
            return;
        }
        if (State >= 2) {
            Gst()->SetGhost(GFnone);
            SetLevel3();
            Gst()->SetGhostAt(GFdonut, Cx, Cy);
            AcceptKey = 2;
        }
    }
    else if (Level == 3) {
        // Redo the second surface definition, or undone operation.
        //
        if (Which == Rdonut && State == 3) {
            Gst()->SetGhost(GFnone);
            XM()->SetCoordMode(CO_ABSOLUTE);
            DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor,
                20, DSP()->CurMode());
            Ulist()->RedoOperation();
            SetLevel1(true);
            return;
        }
        if (State >= 3) {
            Gst()->SetGhost(GFnone);
            SetLevel4();
            ArcMode = false;
            Gst()->SetGhostAt(GFarc, Cx, Cy);
        }
    }
    else if (Level == 4) {
        // Redo anchor point definition, or undone operation.
        //
        if (State >= 4) {
            Gst()->SetGhost(GFnone);
            SetLevel5();
            ArcMode = true;
            Gst()->SetGhostAt(GFarc, Cx, Cy);
        }
    }
    else if (Level == 5) {
        // Redo last undo.
        //
        if (Which == Rarc && State == 5) {
            Gst()->SetGhost(GFnone);
            XM()->SetCoordMode(CO_ABSOLUTE);
            DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor,
                20, DSP()->CurMode());
            Ulist()->RedoOperation();
            SetLevel1(true);
            return;
        }
    }
}


// Create the disk.  Returns true if successful.
//
bool
RoundState::makedisk(int rx, int ry)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return (false);
    if (ld->isInvisible()) {
        Log()->ErrorLog(mh::ObjectCreation,
            "Creation of polygon on invisible layer not allowed.");
        Gst()->SetGhost(GFnone);
        XM()->SetCoordMode(CO_ABSOLUTE);
        DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor, 20,
            DSP()->CurMode());
        return (true);
    }
    if (!CDvdb()->getVariable(VA_NoConstrainRound) &&
            !DrcIf()->diskEval(rx, ry, ld))
        return (false);
    DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor, 20,
        DSP()->CurMode());

    int ss = GEO()->spotSize();
    if (Name() == ARC_CMD)
        GEO()->setSpotSize(0);
    else if (ss < 0)
        GEO()->setSpotSize(INTERNAL_UNITS(Tech()->MfgGrid()));
    Poly poly;
    poly.points = GEO()->makeArcPath(&poly.numpts, cursd->isElectrical(),
        Cx, Cy, rx, ry);
    GEO()->setSpotSize(ss);

    Errs()->init_error();
    CDpo *newp = cursd->newPoly(0, &poly, ld, 0, false);
    if (!newp) {
        Errs()->add_error("newPoly failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        return (false);
    }
    if (!cursd->mergeBoxOrPoly(newp, true)) {
        Errs()->add_error("mergeBoxOrPoly failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
    }
    Gst()->SetGhost(GFnone);
    XM()->SetCoordMode(CO_ABSOLUTE);
    Ulist()->CommitChanges(true);
    Last = Rdisk;
    return (true);
}


// Create the donut.  Returns true if successful.
//
bool
RoundState::makedonut(int r2x, int r2y)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return (false);
    if (ld->isInvisible()) {
        Log()->ErrorLog(mh::ObjectCreation,
            "Creation of polygon on invisible layer not allowed.");
        Gst()->SetGhost(GFnone);
        XM()->SetCoordMode(CO_ABSOLUTE);
        DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor, 20,
            DSP()->CurMode());
        return (true);
    }
    if ((double)(r2x - Rad1x)*(r2y - Rad1y) <= 0.0)
        return (false);
    if (!CDvdb()->getVariable(VA_NoConstrainRound) &&
            !DrcIf()->donutEval(Rad1x, Rad1y, r2x, r2y, ld))
        return (false);

    int ss = GEO()->spotSize();
    if (Name() == ARC_CMD)
        GEO()->setSpotSize(0);
    else if (ss < 0)
        GEO()->setSpotSize(INTERNAL_UNITS(Tech()->MfgGrid()));
    Poly poly;
    Errs()->init_error();
    poly.points = GEO()->makeArcPath(&poly.numpts, cursd->isElectrical(),
        Cx, Cy, Rad1x, Rad1y, r2x, r2y);
    GEO()->setSpotSize(ss);

    CDpo *newp = cursd->newPoly(0, &poly, ld, 0, false);
    if (!newp) {
        Errs()->add_error("newPoly failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        return (false);
    }
    if (!cursd->mergeBoxOrPoly(newp, true)) {
        Errs()->add_error("mergeBoxOrPoly failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
    }
    Gst()->SetGhost(GFnone);
    XM()->SetCoordMode(CO_ABSOLUTE);
    DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor, 20,
        DSP()->CurMode());
    Ulist()->CommitChanges(true);
    Last = Rdonut;
    return (true);
}


// Create the arc.  Returns true on success.
//
bool
RoundState::makearc(double a)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return (false);
    if (ld->isInvisible()) {
        Log()->ErrorLog(mh::ObjectCreation,
            "Creation of polygon on invisible layer not allowed.");
        Gst()->SetGhost(GFnone);
        XM()->SetCoordMode(CO_ABSOLUTE);
        DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor, 20,
            DSP()->CurMode());
        return (true);
    }
    if (a != Ang1 && !CDvdb()->getVariable(VA_NoConstrainRound) &&
            !DrcIf()->arcEval(Rad1x, Rad1y, Rad2x, Rad2y, Ang1, a, ld))
        return (false);

    int ss = GEO()->spotSize();
    GEO()->setSpotSize(0);
    Poly poly;
    poly.points = GEO()->makeArcPath(&poly.numpts, cursd->isElectrical(),
        Cx, Cy, Rad1x, Rad1y, Rad2x, Rad2y, Ang1, a);
    GEO()->setSpotSize(ss);

    Errs()->init_error();
    CDpo *newp = cursd->newPoly(0, &poly, ld, 0, false);
    if (!newp) {
        Errs()->add_error("newPoly failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        return (false);
    }
    if (!cursd->mergeBoxOrPoly(newp, true)) {
        Errs()->add_error("mergeBoxOrPoly failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
    }
    Gst()->SetGhost(GFnone);
    XM()->SetCoordMode(CO_ABSOLUTE);
    DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor, 20,
        DSP()->CurMode());
    Ulist()->CommitChanges(true);
    Last = Rarc;
    return (true);
}
// End of RoundState functions


// Return the bounding box of pts in BB.
//
void
cEdit::pathBB(Point *pts, int num, BBox *BB)
{
    BB->left = BB->right = pts[0].x;
    BB->bottom = BB->top = pts[0].y;
    for (int i = 1; i < num; i++) {
        if (pts[i].x < BB->left)
            BB->left = pts[i].x;
        else if (pts[i].x > BB->right)
            BB->right = pts[i].x;
        if (pts[i].y < BB->bottom)
            BB->bottom = pts[i].x;
        else if (pts[i].y > BB->top)
            BB->top = pts[i].y;
    }
}
// End of cEdit functions


//----------------
// Ghost Rendering

// Callback to display ghost disk as it is being created.
//
void
cEditGhost::showGhostDisk(int x, int y, int cen_x, int cen_y, bool erase)
{
    if (!RoundCmd)
        return;
    int rad = RoundCmd->compute_radius(x - cen_x, y - cen_y);
    int rx, ry;
    if (RoundCmd->Mode == RndHoldX) {
        rx = RoundCmd->Rad1x;
        ry = rad;
    }
    else if (RoundCmd->Mode == RndHoldY) {
        rx = rad;
        ry = RoundCmd->Rad1y;
    }
    else
        rx = ry = rad;

    int ss = GEO()->spotSize();
    if (RoundCmd->Name() == ARC_CMD)
        GEO()->setSpotSize(0);
    else if (ss < 0)
        GEO()->setSpotSize(INTERNAL_UNITS(Tech()->MfgGrid()));
    int numpts;
    Point *points = GEO()->makeArcPath(&numpts,
        (DSP()->CurMode() == Electrical), cen_x, cen_y, rx, ry);
    GEO()->setSpotSize(ss);
    Gst()->ShowGhostPath(points, numpts);
    delete [] points;

    // Print the radii on-screen.
    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::CHD);
    while ((wdesc = wgen.next()) != 0) {
        if (Gst()->ShowingGhostInWindow(wdesc)) {
            char buf[128];
            if (rx == ry)
                sprintf(buf, "%.4f", MICRONS(rx));
            else
                sprintf(buf, "%.4f %.4f", MICRONS(rx), MICRONS(ry));
            int xo = 4;
            int yo = wdesc->ViewportHeight() - 5;
            if (erase) {
                int w = 0, h = 0;
                wdesc->Wdraw()->TextExtent(buf, &w, &h);
                BBox BB(xo, yo, xo+w, yo-h);
                wdesc->GhostUpdate(&BB);
            }
            else
                wdesc->Wdraw()->Text(buf, xo, yo, 0, 1, 1);
        }
    }
}


// Callback to display ghost donut as it is being created.
//
void
cEditGhost::showGhostDonut(int x, int y, int cen_x, int cen_y, bool erase)
{
    if (!RoundCmd)
        return;

    int rad = RoundCmd->compute_radius(x - cen_x, y - cen_y);
    int rx, ry;
    if (RoundCmd->Mode == RndHoldX) {
        rx = RoundCmd->Rad2x;
        ry = rad;
    }
    else if (RoundCmd->Mode == RndHoldY) {
        rx = rad;
        ry = RoundCmd->Rad2y;
    }
    else
        rx = ry = rad;

    if ((double)(rx - RoundCmd->Rad1x)*(ry - RoundCmd->Rad1y) <= 0.0)
        return;

    int ss = GEO()->spotSize();
    if (RoundCmd->Name() == ARC_CMD)
        GEO()->setSpotSize(0);
    else if (ss < 0)
        GEO()->setSpotSize(INTERNAL_UNITS(Tech()->MfgGrid()));
    int numpts;
    Point *points;
    points = GEO()->makeArcPath(&numpts, (DSP()->CurMode() == Electrical),
        cen_x, cen_y, RoundCmd->Rad1x, RoundCmd->Rad1y, rx, ry);
    GEO()->setSpotSize(ss);
    Gst()->ShowGhostPath(points, numpts);
    delete [] points;

    // Print the radii on-screen.
    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::CHD);
    while ((wdesc = wgen.next()) != 0) {
        if (Gst()->ShowingGhostInWindow(wdesc)) {
            char buf[128];
            if (rx == ry)
                sprintf(buf, "%.4f", MICRONS(rx));
            else
                sprintf(buf, "%.4f %.4f", MICRONS(rx), MICRONS(ry));
            int xo = 4;
            int yo = wdesc->ViewportHeight() - 5;
            if (erase) {
                int w = 0, h = 0;
                wdesc->Wdraw()->TextExtent(buf, &w, &h);
                BBox BB(xo, yo, xo+w, yo-h);
                wdesc->GhostUpdate(&BB);
            }
            else
                wdesc->Wdraw()->Text(buf, xo, yo, 0, 1, 1);
        }
    }
}


// Callback to display ghost arc as it is being created.
//
void
cEditGhost::showGhostArc(int x, int y, int cen_x, int cen_y, bool erase)
{
    if (!RoundCmd)
        return;
    double a = RoundCmd->arctan(y - cen_y, x - cen_x);
    double a1, a2;
    if (!RoundCmd->ArcMode) {
        a1 = a;
        a2 = a1 - 2*M_PI + .1;
    }
    else {
        a1 = RoundCmd->Ang1;
        a2 = a;
        if (a2 == a1)
            a2 += M_PI/180;  // for visibility
    }
    int ss = GEO()->spotSize();
    GEO()->setSpotSize(0);
    int numpts;
    Point *points = GEO()->makeArcPath(&numpts,
        (DSP()->CurMode() == Electrical), cen_x, cen_y,
        RoundCmd->Rad1x, RoundCmd->Rad1y, RoundCmd->Rad2x, RoundCmd->Rad2y,
        a1, a2);
    GEO()->setSpotSize(ss);
    Gst()->ShowGhostPath(points, numpts);
    delete [] points;

    // Print the angle.
    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::CHD);
    while ((wdesc = wgen.next()) != 0) {
        if (Gst()->ShowingGhostInWindow(wdesc)) {
            char buf[128];
            sprintf(buf, "%.5f", 180.0*a/M_PI);
            int xo = 4;
            int yo = wdesc->ViewportHeight() - 5;
            if (erase) {
                int w = 0, h = 0;
                wdesc->Wdraw()->TextExtent(buf, &w, &h);
                BBox BB(xo, yo, xo+w, yo-h);
                wdesc->GhostUpdate(&BB);
            }
            else
                wdesc->Wdraw()->Text(buf, xo, yo, 0, 1, 1);
        }
    }
}

