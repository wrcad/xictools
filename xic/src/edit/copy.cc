
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
 $Id: copy.cc,v 1.119 2016/08/13 19:47:51 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "extif.h"
#include "edit.h"
#include "undolist.h"
#include "scedif.h"
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
#include "texttf.h"


//
// The Copy command.
//

namespace {
    namespace ed_copy {
        struct CopyState : public CmdState
        {
            CopyState(const char*, const char*);
            virtual ~CopyState();

            void setCaller(GRobject c)  { Caller = c; }

            CDs *alt_source() { return (AltSource); }

            void b1down();
            void b1up();
            void b1down_altw();
            void b1up_altw();
            void desel();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();

            void message() { if (Level == 1) PL()->ShowPrompt(cmsg1);
                else if (Level == 2) PL()->ShowPrompt(cmsg2);
                else PL()->ShowPrompt(cmsg3); }

            static int RepCount;

        private:
            void SetLevel1(bool show) { Level = 1; if (show) message(); }
            void SetLevel2(bool show) { Level = 2; if (show) message(); }
            void SetLevel3() { Level = 3; Ncopies = 0; message(); }

            GRobject Caller;
            bool GotOne;
            bool GhostOn;
            CDs *AltSource;
            int State;
            int Count;
            int Refx, Refy;
            int Orgx, Orgy;
            BBox AOI;
            int Corner;
            int Ncopies;

            static const char *cmsg1;
            static const char *cmsg2;
            static const char *cmsg3;
        };

        CopyState *CopyCmd;
    }
}

using namespace ed_copy;

int CopyState::RepCount = 1;

const char *CopyState::cmsg1 = "Select objects to copy.";
const char *CopyState::cmsg2 =
    "Click or drag, button-down is the reference point.";
const char *CopyState::cmsg3 =
    "Click on locations where the selected items will be copied.";


// Menu command to copy objects.
//
void
cEdit::copyExec(CmdDesc *cmd)
{
    if (CopyCmd) {
        CopyCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;

    // Create a cell desc if we don't have one, so we can copy from
    // foreign windows.
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;

    char buf[32];
    sprintf(buf, "%d", CopyState::RepCount);
    char *in = PL()->EditPrompt("Enter replication count: ", buf);
    for (;;) {
        if (!in)
            return;
        int n;
        if (sscanf(in, "%d", &n) != 1 || n < 1) {
            in = PL()->EditPrompt(
                "Bad replication count, must be positive integer, try again: ",
                buf);
            continue;
        }
        CopyState::RepCount = n;
        break;
    }

    CopyCmd = new CopyState("COPY", "xic:copy");
    CopyCmd->setCaller(cmd ? cmd->caller : 0);
    Ulist()->ListCheck(CopyCmd->Name(), CurCell(), CopyCmd->CmdLevel() == 2);
    if (!EV()->PushCallback(CopyCmd)) {
        delete CopyCmd;
        return;
    }
    CopyCmd->message();
    ed_move_or_copy = CDcopy;
    ds.clear();
}


CopyState::CopyState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    GotOne = Selections.hasTypes(CurCell(), 0);
    GhostOn = false;
    AltSource = 0;
    State = 0;
    Count = 0;
    Refx = Refy = 0;
    Orgx = Orgy = 0;
    Corner = 0;
    Ncopies = 0;

    if (GotOne)
        Level = 2;
    else
        Level = 1;
}


CopyState::~CopyState()
{
    CopyCmd = 0;
}


void
CopyState::b1down()
{
    if (Level == 1)
        cEventHdlr::sel_b1down();
    else if (Level == 2) {
        if (!Selections.hasTypes(CurCell(), 0, false))
            return;
        EV()->SetConstrained(
            (EV()->Cursor().get_downstate() & GR_CONTROL_MASK) ||
            (EV()->Cursor().get_downstate() & GR_SHIFT_MASK));
        EV()->Cursor().get_xy(&Refx, &Refy);
        XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
        AltSource = 0;
        Corner = 0;
        State = 2;
        Gst()->SetGhostAt(GFmove, Refx, Refy);
        GhostOn = true;
        ED()->setPressLayer(LT()->CurLayer());
        EV()->DownTimer(GFmove);
    }
    else if (Level == 3) {
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        ED()->findContact(&x, &y, Refx, Refy, false);

        if ((!AltSource || AltSource == CurCell()) &&
                x == Refx && y == Refy && GEO()->curTx()->is_identity()) {
            PL()->ShowPrompt("Can't copy objects directly over themselves.");
            Gst()->RepaintGhost();
            return;
        }

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
        if (ED()->replicateQueue(Refx, Refy, x, y, RepCount, ldold, ldnew,
                AltSource)) {
            Count++;
            Gst()->SetGhost(GFnone);
            GhostOn = false;
            Ulist()->CommitChanges(true);
            if (Selections.hasTypes(AltSource ? AltSource : CurCell(), 0)) {
                Gst()->SetGhostAt(GFmove, Refx, Refy,
                    AltSource ? GhostAltIncluded : GhostVanilla);
                GhostOn = true;
            }
            Ncopies++;
        }
    }
}


void
CopyState::b1up()
{
    if (Level == 1) {
        if (!cEventHdlr::sel_b1up(&AOI, 0, 0))
            return;
        if (!Selections.hasTypes(CurCell(), 0))
            return;
        State = 1;
        SetLevel2(true);
    }
    else if (Level == 2) {
        if (!Selections.hasTypes(CurCell(), 0, false))
            return;
        if (!EV()->UpTimer() && EV()->CurrentWin() &&
                EV()->Cursor().is_release_ok()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            ED()->findContact(&x, &y, Refx, Refy, false);
            if (ED()->replicateQueue(Refx, Refy, x, y, RepCount, 0, 0)) {
                Count++;
                Gst()->SetGhost(GFnone);
                GhostOn = false;
                Ulist()->CommitChanges(true);
                if (!Selections.hasTypes(CurCell(), 0)) {
                    // Note that we exit if the original objects are
                    // all merged.
                    //
                    esc();
                    return;
                }
                else {
                    Gst()->SetGhostAt(GFmove, Refx, Refy);
                    GhostOn = true;
                    SetLevel3();
                    Ncopies++;  // After SetLevel3 !
                    return;
                }
            }
        }
        SetLevel3();
    }
    else if (Level == 3) {
        if (!Selections.hasTypes(CurCell(), 0)) {
            if (!AltSource || !Selections.hasTypes(AltSource, 0))
                esc();
        }
    }
}


// Handle the alt-window button presses so that objects in windows
// which are showing another cell can be selected and copied into the
// editing window.

void
CopyState::b1down_altw()
{
    if (!DSP()->MainWdesc()->IsSimilar(EV()->ButtonWin(true), WDsimXcell))
        return;
    if (Level == 1)
        cEventHdlr::sel_b1down_altw();
    else if (Level == 2) {
        // Start the copy operation, the objects are attached to the
        // mouse pointer in the usual way.  The ghosting will appear
        // in the class of windows which contain the mouse cursor,
        // either the alt window or the main window.
        //
        // In this mode, there is no meaningful reference point, so we
        // don't use motion constraints or relative coordinate mode.

        WindowDesc *wd = EV()->ButtonWin(true);
        // This is the alt window.
        if (!wd)
            return;
        CDs *sd = wd->CurCellDesc(wd->Mode());
        if (!sd)
            return;
        if (!Selections.hasTypes(sd, 0, true))
            return;
        EV()->Cursor().get_alt_down(&Refx, &Refy);
        wd->Snap(&Refx, &Refy);

        AltSource = sd;
        Corner = 0;
        State = 2;
        Gst()->SetGhostAt(GFmove, Refx, Refy, GhostAltIncluded);
        GhostOn = true;
        ED()->setPressLayer(LT()->CurLayer());
        EV()->DownTimer(GFmove);
    }
}


void
CopyState::b1up_altw()
{
    if (!DSP()->MainWdesc()->IsSimilar(EV()->ButtonWin(true), WDsimXcell))
        return;
    if (Level == 1) {
        int win_id;
        if (!cEventHdlr::sel_b1up_altw(&AOI, 0, 0, &win_id))
            return;
        WindowDesc *wd = DSP()->Windesc(win_id);
        if (!wd)
            return;
        CDs *sd = wd->CurCellDesc(wd->Mode());
        if (!wd)
            return;
        if (!Selections.hasTypes(sd, 0))
            return;
        State = 1;
        SetLevel2(true);
    }
    else if (Level == 2) {
        if (!EV()->UpTimer() && EV()->CurrentWin() &&
                EV()->Cursor().get_was_press_alt() &&
                EV()->CurrentWin()->IsSimilar(DSP()->MainWdesc())) {
            // Drag from alt window into a drawing window.

            int x, y;
            EV()->Cursor().get_alt_up(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            ED()->findContact(&x, &y, Refx, Refy, false);
            if (ED()->replicateQueue(Refx, Refy, x, y, 1, 0, 0, AltSource)) {
                Count++;
                Gst()->SetGhost(GFnone);
                GhostOn = false;
                Ulist()->CommitChanges(true);
                Gst()->SetGhostAt(GFmove, Refx, Refy, GhostAltIncluded);
                GhostOn = true;
                SetLevel3();
                Ncopies++;  // After SetLevel3 !
                return;
            }
        }
        SetLevel3();
    }
}


// Desel pressed, reset.
//
void
CopyState::desel()
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
CopyState::esc()
{
    if (Level == 1)
        cEventHdlr::sel_esc();
    else {
        Gst()->SetGhost(GFnone);
        Selections.deselectAll(GotOne ? CurCell() : 0);
        XM()->SetCoordMode(CO_ABSOLUTE);
    }
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    EV()->SetConstrained(false);
    ED()->setMoveOrCopy(CDmove);
    delete this;
}


bool
CopyState::key(int code, const char*, int)
{
    switch (code) {
    case SHIFTDN_KEY:
    case CTRLDN_KEY:
        if (!AltSource) {
            if (GhostOn)
                Gst()->SetGhost(GFnone);
            EV()->SetConstrained(true);
            if (GhostOn) {
                Gst()->SetGhostAt(GFmove, Refx, Refy);
                Gst()->RepaintGhost();
            }
        }
        return (true);
    case CTRLUP_KEY:
    case SHIFTUP_KEY:
        if (!AltSource) {
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
        }
        return (true);
    case RETURN_KEY:
        if (GhostOn && DSP()->CurMode() == Physical) {
            BBox sBB;
            if (Selections.computeBB(CurCell(), &sBB, (AltSource != 0))) {
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
                if (!AltSource)
                    XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
                Gst()->SetGhostAt(GFmove, Refx, Refy,
                    AltSource ? GhostAltIncluded : GhostVanilla);
                Gst()->RepaintGhost();
            }
            return (true);
        }

    case BSP_KEY:
    case DELETE_KEY:
        WindowDesc *wd = EV()->CurrentWin();
        if (!wd)
            wd = DSP()->MainWdesc();
        if (PL()->KeyPos(wd) == 0) {
            if (Level == 2) {
                // Revert to selection level so user can update selections.
                // Note that these can't be undone/redone, GotOne -> true.
                GotOne = true;
                State = 0;
                SetLevel1(true);
                return (true);;
            }
            if (Level == 3) {
                // Revert to object pick-up so user can change reference
                // point of use another source window..
                Gst()->SetGhost(GFnone);
                GhostOn = false;
                State = 1;
                SetLevel2(true);
                return (true);;
            }
        }
        break;
    }
    return (false);
}


void
CopyState::undo()
{
    if (Level == 1)
        cEventHdlr::sel_undo();
    else if (Level == 2) {
        if (!GotOne) {
            Selections.deselectAll();
            SetLevel1(true);
            return;
        }
        Ulist()->UndoOperation();
        if (!Selections.hasTypes(CurCell(), 0))
            esc();
    }
    else if (Level == 3) {
        if (Ncopies > 0) {
            Ncopies--;
            Ulist()->UndoOperation();
        }
        else {
            Gst()->SetGhost(GFnone);
            GhostOn = false;
            SetLevel2(true);
        }
    }
}


void
CopyState::redo()
{
    if (Level == 1) {
        if (State && Count >= Ulist()->CountRedo()) {
            Selections.selection(AltSource ? AltSource : CurCell(), 0, &AOI);
            SetLevel2(true);
        }
        else
            cEventHdlr::sel_redo(true);
    }
    else if (Level == 2) {
        if (State == 2 && Count >= Ulist()->CountRedo()) {
            Gst()->SetGhostAt(GFmove, Refx, Refy,
                AltSource ? GhostAltIncluded : GhostVanilla);
            GhostOn = true;
            SetLevel3();
            return;
        }
        if (!GotOne)
            return;
        Ulist()->RedoOperation();
    }
    else if (Level == 3) {
        if (Ulist()->CountRedo())
            Ncopies++;
        Ulist()->RedoOperation();
    }
}
// End of CopyState functions.


// This function is called by the front-end pointer handler to effect
// a copy.
//
void
cEdit::copyObjects(int ref_x, int ref_y, int mov_x, int mov_y,
    CDl *ldold, CDl *ldnew, int repcnt)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    if (mov_x == ref_x && mov_y == ref_y && GEO()->curTx()->is_identity()) {
        PL()->ShowPrompt("Can't copy objects directly over themselves.");
        return;
    }
    findContact(&mov_x, &mov_y, ref_x, ref_y, false);
    Ulist()->ListCheck("copy", cursd, true);
    if (replicateQueue(ref_x, ref_y, mov_x, mov_y, repcnt, ldold, ldnew))
        Ulist()->CommitChanges(true);
}


// Repeat a copy operation repcnt times, incrementing translation.  If
// the source is a foreign cell, we can't replicate.
//
bool
cEdit::replicateQueue(int ref_x, int ref_y, int new_x, int new_y, int repcnt,
    CDl *ldold, CDl *ldnew, CDs *srcsd)
{
    if (srcsd && srcsd != CurCell())
        repcnt = 1;
    int dx = new_x - ref_x;
    int dy = new_y - ref_y;
    for (int i = 1; i <= repcnt; i++) {
        int nx = ref_x + i*dx;
        int ny = ref_y + i*dy;
        if (!copyQueue(ref_x, ref_y, nx, ny, ldold, ldnew, srcsd))
            return (false);
    }
    return (true);
}


// Perform the copy of the objects in the selection list.  New objects
// are placed in the add list.  If ldnew is non-nil, new objects will
// be on this layer if ldold is nil or matches the current object.
//
bool
cEdit::copyQueue(int ref_x, int ref_y, int new_x, int new_y,
    CDl *ldold, CDl *ldnew, CDs *srcsd)
{
    if (EV()->IsConstrained())
        XM()->To45(ref_x, ref_y, &new_x, &new_y);
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);
    if (!srcsd)
        srcsd = cursd;
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
    CDol *cpq = 0;
    if (DSP()->CurMode() == Electrical) {
        // Remove labels referenced from cells from queue.
        Selections.purgeLabels(srcsd);

        stk.TLoadCurrent(&tfnew);
        copied = translate_muts(&stk, ref_x, ref_y, new_x, new_y, &cpq, srcsd);
        stk.TLoadCurrent(&tfold);
    }

    Errs()->init_error();
    int failcnt = 0;
    sSelGen sg(Selections, srcsd);
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (translate(od, cursd, &tfold, &tfnew, ref_x, ref_y, new_x, new_y,
                ldold, ldnew, CDcopy))
            copied = true;
        else
            failcnt++;
    }
    if (failcnt) {
        Errs()->add_error("Copy failed for %d %s.", failcnt,
            failcnt == 1 ? "object" : "objects");
        Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
    }
    if (cpq) {
        // Mutual inductor copies, put back in queue.
        Selections.insertList(srcsd, cpq);
        cpq->free();
    }
    XM()->ShowParameters();
    return (copied);
}


// If in electrical mode and a selected object is a device or
// subcircuit, snap new_x, new_y so that terminals make contact to
// existing objects and return true.  If indicate is true, show a box
// around the object when it is in position.
//
bool
cEdit::findContact(int *new_x, int *new_y, int ref_x, int ref_y,
    bool indicate)
{
    if (DSP()->CurMode() != Electrical)
        return (false);
    CDs *cursd = CurCell(Electrical);
    if (!cursd || cursd->isSymbolic())
        return (false);
    bool found = false;
    bool fndx = false, fndy = false;
    WindowDesc *wdesc =
        EV()->CurrentWin() ? EV()->CurrentWin() : DSP()->MainWdesc();
    cTfmStack stk;
    int delta = 1 + (int)(DSP()->PixelDelta()/wdesc->Ratio());
    int box_delta = 1 + (int)(8.0/wdesc->Ratio());
    sSelGen sg(Selections, cursd, "wc");
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (od->type() == CDINSTANCE) {
            CDs *sdesc = OCALL(od)->masterCell();
            if (!sdesc)
                continue;
            if (sdesc->owner())
                sdesc = sdesc->owner();
            bool issym = sdesc->symbolicRep((CDc*)od);
            CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
            stk.TPush();
            stk.TApplyTransform(OCALL(od));
            GEO()->applyCurTransform(&stk, ref_x, ref_y, *new_x, *new_y);
            int tdx = 0, tdy = 0;
            for ( ; pn; pn = pn->next()) {
                for (unsigned int ix = 0; ; ix++) {
                    int x, y;
                    if (issym) {
                        if (!pn->get_pos(ix, &x, &y))
                            break;
                    }
                    else {
                        if (ix)
                            break;
                        pn->get_schem_pos(&x, &y);
                    }
                    stk.TPoint(&x, &y);
                    x += tdx;
                    y += tdy;
                    int nx, ny, flg;
                    if (wdesc->FindContact(x, y, &nx, &ny, &flg,
                            found ? 0 : delta,
                            moveOrCopy() == CDmove ? od : 0,
                            (moveOrCopy() == CDmove))) {
                        if (!fndx && (flg & FC_CX)) {
                            *new_x += nx - x;
                            tdx = nx - x;
                            fndx = true;
                        }
                        if (!fndy && (flg & FC_CY)) {
                            *new_y += ny - y;
                            tdy = ny - y;
                            fndy = true;
                        }
                        if (indicate) {
                            BBox BB;
                            BB.left = nx - box_delta;
                            BB.bottom = ny - box_delta;
                            BB.right = nx + box_delta;
                            BB.top = ny + box_delta;
                            DSPmainDraw(SetLinestyle(DSP()->BoxLinestyle()))
                            Gst()->ShowGhostBox(BB.left, BB.bottom, BB.right,
                                BB.top);
                            DSPmainDraw(SetLinestyle(0))
                        }
                        found = true;
                    }
                }
            }

            CDp_bsnode *pb = (CDp_bsnode*)sdesc->prpty(P_BNODE);
            for ( ; pb; pb = pb->next()) {
                for (unsigned int ix = 0; ; ix++) {
                    int x, y;
                    if (issym) {
                        if (!pb->get_pos(ix, &x, &y))
                            break;
                    }
                    else {
                        if (ix)
                            break;
                        pb->get_schem_pos(&x, &y);
                    }
                    stk.TPoint(&x, &y);
                    x += tdx;
                    y += tdy;
                    int nx, ny, flg;
                    if (wdesc->FindBterm(x, y, &nx, &ny, &flg,
                            found ? 0 : delta,
                            moveOrCopy() == CDmove ? od : 0,
                            (moveOrCopy() == CDmove))) {
                        if (!fndx && (flg & FC_CX)) {
                            *new_x += nx - x;
                            tdx = nx - x;
                            fndx = true;
                        }
                        if (!fndy && (flg & FC_CY)) {
                            *new_y += ny - y;
                            tdy = ny - y;
                            fndy = true;
                        }
                        if (indicate) {
                            BBox BB;
                            BB.left = nx - box_delta;
                            BB.bottom = ny - box_delta;
                            BB.right = nx + box_delta;
                            BB.top = ny + box_delta;
                            DSPmainDraw(SetLinestyle(DSP()->BoxLinestyle()))
                            Gst()->ShowGhostBox(BB.left, BB.bottom, BB.right,
                                BB.top);
                            DSPmainDraw(SetLinestyle(0))
                        }
                        found = true;
                    }
                }
            }
            stk.TPop();
        }
        else if (od->type() == CDWIRE) {
            stk.TPush();
            GEO()->applyCurTransform(&stk, ref_x, ref_y, *new_x, *new_y);
            CDw *wd = OWIRE(od);
            for (int i = 0; i < wd->numpts(); i++) {
                int x = wd->points()[i].x;
                int y = wd->points()[i].y;
                stk.TPoint(&x, &y);
                int nx, ny, flg;
                if (wdesc->FindContact(x, y, &nx, &ny, &flg,
                        found ? 0 : delta,
                        moveOrCopy() == CDmove ? od : 0,
                        (moveOrCopy() == CDmove))) {
                    if (!fndx && (flg & FC_CX)) {
                        *new_x += nx - x;
                        fndx = true;
                    }
                    if (!fndy && (flg & FC_CY)) {
                        *new_y += ny - y;
                        fndy = true;
                    }
                    if (indicate) {
                        BBox BB;
                        BB.left = nx - box_delta;
                        BB.bottom = ny - box_delta;
                        BB.right = nx + box_delta;
                        BB.top = ny + box_delta;
                        DSPmainDraw(SetLinestyle(DSP()->BoxLinestyle()))
                        Gst()->ShowGhostBox(BB.left, BB.bottom, BB.right,
                            BB.top);
                        DSPmainDraw(SetLinestyle(0))
                    }
                    found = true;
                }
            }
            stk.TPop();
        }
    }
    return (found);
}


// Return true if both inductors of the mutual inductor property
// have valid odescs which are in the selection queue for sdesc.
//
bool
cEdit::mutSelected(CDs *sdesc, CDp *pdesc)
{
    if (!sdesc || !sdesc->isElectrical() || !pdesc)
        return (false);
    CDc *odesc1, *odesc2;
    if (!sdesc->prptyMutualFind(pdesc, &odesc1, &odesc2))
        return (false);
    int found1 = false, found2 = false;
    sSelGen sg(Selections, sdesc, "c");
    CDc *cd;
    while ((cd = (CDc*)sg.next()) != 0) {
        if (odesc1 == cd)
            found1 = true;
        else if (odesc2 == cd)
            found2 = true;
        if (found1 && found2)
            return (true);
    }
    return (false);
}


// Create a new instance of that in old_pointer at new_x, new_y.  The
// internal properties are set in CD, copy the user properties.  The
// new object is placed in the add list, and old_pointer is placed in
// the delete list if mc == Move.
//
bool
cEdit::copy_call(int ref_x, int ref_y, int new_x, int new_y,
    CDc *old_pointer, CDmcType mc, CDc **newcall)
{
    CDs *cursd = CurCell();
    if (!cursd || cursd->isSymbolic())
        return (false);
    CallDesc calldesc;
    old_pointer->call(&calldesc);

    cTfmStack stk;
    stk.TPush();
    stk.TApplyTransform(old_pointer);

    int x = 0, y = 0;
    const BBox *sBB = 0;
    // This and the block below attempt to compensate for round-off
    // errors in 45 degree rotations.  It is important that the
    // transform is exactly "right" otherwise connectivity to adjacent
    // objects will be lost.
    //
    if (DSP()->CurMode() == Physical && GEO()->curTx()->angle() % 90 == 45) {
        CDs *ss = old_pointer->masterCell();
        sBB = ss->BB();
        x = sBB->left;
        y = sBB->bottom;
        if (GEO()->curTx()->magset()) {
            int nx = 0, ny = 0;
            stk.TPoint(&nx, &ny);
            nx -= ref_x;
            ny -= ref_y;
            nx = (int)(nx*(1.0 - GEO()->curTx()->magn()));
            ny = (int)(ny*(1.0 - GEO()->curTx()->magn()));
            stk.TTranslate(-nx, -ny);
            stk.TPoint(&x, &y);
            stk.TTranslate(nx, ny);
        }
        else
            stk.TPoint(&x, &y);
        stk.TPush();
        GEO()->applyCurTransform(&stk, ref_x, ref_y, new_x, new_y);
        stk.TPoint(&x, &y);
        stk.TPop();
    }

    // now add the previous transform to that of the move
    if (GEO()->curTx()->magset()) {
        int nx = 0, ny = 0;
        stk.TPoint(&nx, &ny);
        nx -= ref_x;
        ny -= ref_y;
        nx = (int)(nx*(1.0 - GEO()->curTx()->magn()));
        ny = (int)(ny*(1.0 - GEO()->curTx()->magn()));
        stk.TTranslate(-nx, -ny);
    }
    GEO()->applyCurTransform(&stk, ref_x, ref_y, new_x, new_y);

    if (DSP()->CurMode() == Physical && GEO()->curTx()->angle() % 90 == 45) {
        int xx = sBB->left;
        int yy = sBB->bottom;
        stk.TPoint(&xx, &yy);
        if (xx != x || yy != y)
            stk.TTranslate(x - xx, y - yy);
    }

    CDtx tx;
    stk.TCurrent(&tx);
    stk.TPop();
    CDap ap(old_pointer);
    tx.add_mag(GEO()->curTx()->magn());
    CDc *cdesc;
    if (OIfailed(cursd->makeCall(&calldesc, &tx, &ap, CDcallNone, &cdesc)))
        return (false);

    if (DSP()->CurMode() == Electrical) {

        // Remove any default user properties, and if in a move,
        // remove the node and name properties.  Then, copy the
        // previous properties, transforming the node coordinates.
        // Don't want to disturb the terminal fields of the node
        // and name properties.
        //
        if (mc == CDmove) {
            stk.TPush();
            GEO()->applyCurTransform(&stk, ref_x, ref_y, new_x, new_y);
            cdesc->prptyRemove(P_NAME);
            cdesc->prptyRemove(P_NODE);
        }
        cdesc->prptyRemove(P_VALUE);
        cdesc->prptyRemove(P_MODEL);
        cdesc->prptyRemove(P_PARAM);
        cdesc->prptyRemove(P_OTHER);
        cdesc->prptyRemove(P_NOPHYS);
        cdesc->prptyRemove(P_FLATTEN);
        cdesc->prptyRemove(P_RANGE);
        cdesc->prptyRemove(P_DEVREF);

        // Copy old properties.
        CDp *pdesc;
        for (pdesc = old_pointer->prpty_list(); pdesc;
                pdesc = pdesc->next_prp()) {
            switch (pdesc->value()) {
            case P_NODE:
                if (mc == CDcopy)
                    break;
                else {
                    CDp_node *pnd = (CDp_node*)cdesc->prptyAddCopy(pdesc);
                    if (pnd)
                        pnd->transform(&stk);
                }
                break;
            case P_NAME:
                if (mc == CDcopy)
                    break;
            case P_RANGE:
            case P_BNODE:
            case P_MODEL:
            case P_VALUE:
            case P_PARAM:
            case P_OTHER:
            case P_NOPHYS:
            case P_FLATTEN:
            case P_MUTLRF:
            case P_DEVREF:
                cdesc->prptyAddCopy(pdesc);
                break;
            case P_SYMBLC:
                cdesc->prptyAddCopy(pdesc);
                cdesc->computeBB();
                break;
            }
        }
        if (mc == CDmove)
            stk.TPop();
    }
    else {
        cdesc->prptyAddCopyList(old_pointer->prpty_list());
        if (mc == CDcopy) {
            // If copying and the instance has abutment reversion
            // properties, add a temporary property to flag this as a
            // copy.  This will allow parameter reversion of cdesc
            // without touching the partners of the original.

            if (cdesc->prpty(XICP_AB_PRIOR)) {
                CDp *pd = new CDp("copy", XICP_AB_COPY);
                pd->set_next_prp(cdesc->prpty_list());
                cdesc->set_prpty_list(pd);
            }
        }
    }

    Ulist()->RecordObjectChange(cursd, mc == CDmove ? old_pointer : 0, cdesc);
    *newcall = cdesc;
    return (true);
}


// Core function to move/copy an object.  The sprop_changed argument
// is used in move only, and if passed should be initialized to false
// before calling this function.  It returns true if a cell property
// reference point was updated.  In copy mode, it is not necessary
// that odesc be in sdesc.
//
bool
cEdit::translate(CDo *odesc, CDs *sdesc, CDtf *tfold, CDtf *tfnew,
    int refx, int refy, int x, int y, CDl *ldold, CDl *ldnew, CDmcType mc,
    bool *sprop_changed)
{
    if (!odesc)
        return (false);
    cTfmStack stk;
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
    {
        BBox BB = odesc->oBB();
        Poly poly;
        stk.TLoadCurrent(tfnew);
        stk.TBB(&BB, &poly.points);
        stk.TLoadCurrent(tfold);
        CDl *ld = odesc->ldesc();
        if (ldnew && (!ldold || ldold == odesc->ldesc()))
            ld = ldnew;
        if (poly.points) {
            // non-manhattan, convert to polygon
            poly.numpts = 5;
            if (GEO()->curTx()->magset())
                poly.points->scale(5, GEO()->curTx()->magn(), x, y);
            CDo *newo = sdesc->newPoly(mc == CDmove ? odesc : 0, &poly, ld,
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
            if (GEO()->curTx()->magset())
                BB.scale(GEO()->curTx()->magn(), x, y);
            CDo *newo = sdesc->newBox(mc == CDmove ? odesc : 0, &BB, ld,
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
        Poly poly(((const CDpo*)odesc)->numpts(), 0);
        stk.TLoadCurrent(tfnew);
        poly.points = ((const CDpo*)odesc)->points()->dup_with_xform(&stk,
            poly.numpts);
        stk.TLoadCurrent(tfold);
        CDl *ld = odesc->ldesc();
        if (ldnew && (!ldold || ldold == odesc->ldesc()))
            ld = ldnew;
        BBox BB;
        if (poly.to_box(&BB)) {
            delete [] poly.points;
            // rectangular, convert to box
            if (GEO()->curTx()->magset())
                BB.scale(GEO()->curTx()->magn(), x, y);
            CDo *newo = sdesc->newBox(mc == CDmove ? odesc : 0, &BB, ld,
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
        else {
            if (GEO()->curTx()->magset())
                poly.points->scale(poly.numpts, GEO()->curTx()->magn(), x, y);
            CDpo *newo = sdesc->newPoly(mc == CDmove ? odesc : 0, &poly, ld,
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
        return (true);
    }
wire:
    {
        Wire wire(((const CDw*)odesc)->numpts(), 0,
            ((const CDw*)odesc)->attributes());
        stk.TLoadCurrent(tfnew);
        wire.points = ((const CDw*)odesc)->points()->dup_with_xform(&stk,
            wire.numpts);
        stk.TLoadCurrent(tfold);
        if (GEO()->curTx()->magset()) {
            wire.points->scale(wire.numpts, GEO()->curTx()->magn(), x, y);
            if (!ED()->noWireWidthMag())
                wire.set_wire_width(
                    mmRnd(wire.wire_width()*GEO()->curTx()->magn()));
        }
        CDl *ld = odesc->ldesc();
        if (ldnew && (!ldold || ldold == odesc->ldesc()))
            ld = ldnew;
        CDw *newo = sdesc->newWire(mc == CDmove ? odesc : 0, &wire, ld,
            odesc->prpty_list(), false);
        if (!newo) {
            Errs()->add_error("newWire failed");
            return (false);
        }
        if (DSP()->CurMode() == Electrical) {
            // take care of label references
            stk.TLoadCurrent(tfnew);
            if (ld->isWireActive())
                sdesc->prptyWireLink(&stk, newo, (CDw*)odesc, mc);
            if (mc == CDmove) {
                stk.TLoadCurrent(tfnew);
                sdesc->hyTransformMove(&stk, odesc, newo, true);
                if (sdesc->prptyTransformRefs(&stk, (CDw*)odesc)) {
                    if (sprop_changed)
                        *sprop_changed = true;
                }
            }
            stk.TLoadCurrent(tfold);
        }
        if (!sdesc->mergeWire(newo, true)) {
            Errs()->add_error("mergeWire failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        }
        return (true);
    }
label:
    {
        if (mc == CDcopy && DSP()->CurMode() == Electrical) {
            CDp_lref *prf = (CDp_lref*)odesc->prpty(P_LABRF);
            if (prf && prf->devref()) {
                // Don't copy associated labels, since the associated
                // object is not being copied.
                Errs()->add_error(
                    "Copy failed, bound property labels can't be copied.");
                return (false);
            }
        }
        Label label(((CDla*)odesc)->la_label());
        if (GEO()->curTx()->magset()) {
            label.width = (int)(label.width * GEO()->curTx()->magn());
            label.height = (int)(label.height * GEO()->curTx()->magn());
        }
        stk.TLoadCurrent(tfnew);
        stk.TPoint(&label.x, &label.y);
        if (GEO()->curTx()->magset()) {
            label.x = x + (int)((label.x - x)*GEO()->curTx()->magn());
            label.y = y + (int)((label.y - y)*GEO()->curTx()->magn());
        }
        stk.TLoadCurrent(tfold);

        // Update the label xform
        stk.TSetTransformFromXform(label.xform, 0, 0);
        GEO()->applyCurTransform(&stk, 0, 0, 0, 0);
        CDtf tf;
        stk.TCurrent(&tf);
        label.xform &= ~TXTF_XF;
        label.xform |= (tf.get_xform() & TXTF_XF);
        stk.TPop();

        CDl *ld = odesc->ldesc();
        if (ldnew && (!ldold || ldold == odesc->ldesc()))
            ld = ldnew;
        CDla *newo = sdesc->newLabel(mc == CDmove ? odesc : 0, &label, ld,
            odesc->prpty_list(), true);
        if (!newo) {
            Errs()->add_error("newLabel failed");
            return (false);
        }
        if (mc == CDmove && DSP()->CurMode() == Electrical)
            sdesc->prptyLabelUpdate(newo, (CDla*)odesc);
        return (true);
    }
inst:
    {
        stk.TLoadCurrent(tfnew);

        CDc *newo;
        if (!copy_call(refx, refy, x, y, (CDc*)odesc, mc, &newo)) {
            Errs()->add_error("CopyCall failed");
            stk.TLoadCurrent(tfold);
            return (false);
        }
        if (DSP()->CurMode() == Electrical) {
            // take care of label and mutual inductor references
            sdesc->prptyInstLink(&stk, newo, (CDc*)odesc, mc);
            if (mc == CDmove) {
                sdesc->hyTransformMove(&stk, odesc, newo, true);
                if (sdesc->prptyTransformRefs(&stk, (CDc*)odesc)) {
                    if (sprop_changed)
                        *sprop_changed = true;
                }
            }
        }
        stk.TLoadCurrent(tfold);
        return (true);
    }
}


// Called in electrical mode for CDcopy only.
// Deal specially with mutual inductor pairs which may be in the list. 
// For old style muts, creating a new P_MUT property with translated
// coordinates is sufficient.  For new muts being copied, we have to
// be a little careful.  The transform must be active.  Returns true
// if a pair was copied.  Remove NEWMUT pair from srcsd queue.
//
bool
cEdit::translate_muts(const cTfmStack *tstk, int ref_x, int ref_y,
    int new_x, int new_y, CDol **cpq, CDs *srcsd)
{
    CDs *cursde = CurCell();
    if (!cursde || !cursde->isElectrical())
        return (false);
    if (cursde->isSymbolic())
        return (false);
    if (!srcsd)
        srcsd = cursde;
    CDol *s0 = 0;
    bool copied = false;
    for (CDp *pdesc = srcsd->prptyList(); pdesc; pdesc = pdesc->next_prp()) {
        if (pdesc->value() == P_MUT) {
            if (mutSelected(srcsd, pdesc)) {
                // link to head of list
                CDp_mut *newp = (CDp_mut*)cursde->prptyAddCopy(pdesc);
                if (newp)
                    // transform mutual inductor coords
                    newp->transform(tstk);
            }
        }
        else if (pdesc->value() == P_NEWMUT) {
            if (mutSelected(srcsd, pdesc)) {
                Errs()->init_error();
                CDc *odesc1 = ((CDp_nmut*)pdesc)->l1_dev();
                CDc *odesc2 = ((CDp_nmut*)pdesc)->l2_dev();
                CDc *new1;
                if (!copy_call(ref_x, ref_y, new_x, new_y, odesc1,
                        CDcopy, &new1)) {
                    Errs()->add_error("CopyCall failed");
                    Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
                    continue;
                }
                CDc *new2;
                if (!copy_call(ref_x, ref_y, new_x, new_y, odesc2,
                        CDcopy, &new2)) {
                    Errs()->add_error("CopyCall failed");
                    Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
                    continue;
                }
                // Remove from select queue.
                sSelGen sg(Selections, srcsd, "c");
                CDo *od;
                while ((od = sg.next()) != 0) {
                    if (od == odesc1 || od == odesc2) {
                        // Keep the present oState, these objects will be
                        // linked back into selection list.
                        int info = od->state();
                        sg.remove();
                        od->set_state(info);
                        s0 = new CDol(od, s0);
                    }
                }
                cursde->prptyMutualLink(tstk, new1, new2, (CDp_nmut*)pdesc);
                copied = true;
            }
        }
    }
    *cpq = s0;
    return (copied);
}


//----------------
// Ghost Rendering

namespace {
    void display_centroid(CDo *odesc, CDtf *tfold, CDtf *tfnew, int x, int y)
    {
        if (DSP()->ShowObjectCentroidMark() && DSP()->CurMode() == Physical) {
            double dx, dy;
            odesc->centroid(&dx, &dy);
            int xc = INTERNAL_UNITS(dx);
            int yc = INTERNAL_UNITS(dy);
            if (GEO()->curTx()->magset()) {
                dx = (xc - x)*GEO()->curTx()->magn();
                xc = mmRnd(dx) + x;
                dy = (yc - y)*GEO()->curTx()->magn();
                yc = mmRnd(dy) + y;
            }
            const int delta = -8;  // Negative => log-scaling.
            DSP()->TLoadCurrent(tfnew);
            Gst()->ShowGhostCross(xc, yc, delta, false);
            DSP()->TLoadCurrent(tfold);
        }
    }

    // Flag set by display_ghost_move.
    bool ghost_not_done;

    unsigned int display_ghost_move(CDo *odesc, CDtf *tfold, CDtf *tfnew,
        int refx, int refy, int x, int y, int dmode)
    {
        if (!odesc)
            return (0);
#ifdef HAVE_COMPUTED_GOTO
        COMPUTED_JUMP(odesc)
#else
        CONDITIONAL_JUMP(odesc)
#endif
    box:
        {
            Point *pts;
            BBox BB = odesc->oBB();
            DSP()->TLoadCurrent(tfnew);
            DSP()->TBB(&BB, &pts);
            DSP()->TLoadCurrent(tfold);
            if (pts) {
                if (GEO()->curTx()->magset())
                    pts->scale(5, GEO()->curTx()->magn(), x, y);
                Gst()->ShowGhostPath(pts, 5);
                delete [] pts;
            }
            else {
                if (GEO()->curTx()->magset())
                    BB.scale(GEO()->curTx()->magn(), x, y);
                Gst()->ShowGhostBox(BB.left, BB.bottom, BB.right, BB.top);
            }
            display_centroid(odesc, tfold, tfnew, refx, refy);
            return (1);
        }
    poly:
        {
            int num = ((const CDpo*)odesc)->numpts();
            DSP()->TLoadCurrent(tfnew);
            Point *pts = ((const CDpo*)odesc)->points()->dup_with_xform(
                DSP(), num);
            DSP()->TLoadCurrent(tfold);
            if (GEO()->curTx()->magset())
                pts->scale(num, GEO()->curTx()->magn(), x, y);
            Gst()->ShowGhostPath(pts, num);
            delete [] pts;
            display_centroid(odesc, tfold, tfnew, refx, refy);
            return ((num+1)/4);
        }
    wire:
        {
            int num = ((const CDw*)odesc)->numpts();
            DSP()->TLoadCurrent(tfnew);
            Wire wire(num, 
                ((const CDw*)odesc)->points()->dup_with_xform(DSP(), num),
                ((const CDw*)odesc)->attributes());
            DSP()->TLoadCurrent(tfold);
            if (GEO()->curTx()->magset()) {
                wire.points->scale(wire.numpts, GEO()->curTx()->magn(), x, y);
                if (!ED()->noWireWidthMag())
                    wire.set_wire_width(
                        mmRnd(wire.wire_width()*GEO()->curTx()->magn()));
            }
            EGst()->showGhostWire(&wire);
            delete [] wire.points;
            display_centroid(odesc, tfold, tfnew, refx, refy);
            return ((num+1)/2);
        }
    label:
        {
            BBox BB;
            Point *pts;
            odesc->boundary(&BB, &pts);
            if (pts) {
                DSP()->TLoadCurrent(tfnew);
                pts->xform(DSP(), 5);
                DSP()->TLoadCurrent(tfold);
                if (GEO()->curTx()->magset())
                    pts->scale(5, GEO()->curTx()->magn(), x, y);
                Gst()->ShowGhostPath(pts, 5);
                delete [] pts;
            }
            else {
                DSP()->TLoadCurrent(tfnew);
                DSP()->TBB(&BB, &pts);
                DSP()->TLoadCurrent(tfold);
                if (pts) {
                    if (GEO()->curTx()->magset())
                        pts->scale(5, GEO()->curTx()->magn(), x, y);
                    Gst()->ShowGhostPath(pts, 5);
                    delete [] pts;
                }
                else {
                    if (GEO()->curTx()->magset())
                        BB.scale(GEO()->curTx()->magn(), x, y);
                    Gst()->ShowGhostBox(BB.left, BB.bottom, BB.right, BB.top);
                }
            }

            // Mark origin.
            bool b1 = (((CDla*)odesc)->xform() & TXTF_45);
            bool b2 = (GEO()->curTx()->angle() % 90);
            bool do45 = !(b1 + b2 == 1);

            x = ((CDla*)odesc)->xpos();
            y = ((CDla*)odesc)->ypos();
            if (GEO()->curTx()->magset()) {
                double dx = (x - refx)*GEO()->curTx()->magn();
                x = mmRnd(dx) + refx;
                double dy = (y - refy)*GEO()->curTx()->magn();
                y = mmRnd(dy) + refy;
            }
            const int delta = -3;  // Negative => log-scaling.
            DSP()->TLoadCurrent(tfnew);
            Gst()->ShowGhostCross(x, y, delta, do45);
            DSP()->TLoadCurrent(tfold);
            return (2);
        }
    inst:
        {
            CDs *msdesc = ((CDc*)odesc)->masterCell();
            int cnt = 0;
            if (msdesc) {
                bool show_detail = false;
                if (DSP()->CurMode() == Electrical) {
                    if (msdesc->isSymbolic() || msdesc->isDevice())
                        show_detail = true;
                }
                else {
                    if (msdesc->isPCellSubMaster() || msdesc->isViaSubMaster())
                        show_detail = true;
                    else {
                        int expand = DSP()->MainWdesc()->Attrib()->
                            expand_level(DSP()->CurMode());
                        if (DSP()->CurMode() == Physical &&
                                EGst()->maxGhostDepth() >= 0) {
                            if (expand < 0 ||
                                    expand > EGst()->maxGhostDepth())
                                expand = EGst()->maxGhostDepth();
                        }
                        show_detail = expand != 0 ||
                            odesc->has_flag(DSP()->MainWdesc()->DisplFlags());
                    }
                }
                if (show_detail) {
                    DSP()->TPush();
                    DSP()->TApplyTransform((CDc*)odesc);
                    GEO()->applyCurTransform(DSP(), refx, refy, x, y);
                    DSP()->TPremultiply();

                    CDap ap((CDc*)odesc);
                    int tx, ty;
                    DSP()->TGetTrans(&tx, &ty);
                    xyg_t xyg(0, ap.nx - 1, 0, ap.ny - 1);
                    do {
                        bool done;
                        DSP()->TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                        cnt += EGst()->showTransformedLevel(msdesc,
                            tfold, dmode, &done);
                        if (!done)
                            ghost_not_done = true;
                        DSP()->TSetTrans(tx, ty);
                    } while (xyg.advance());
                    DSP()->TPop();
                    if (DSP()->CurMode() == Electrical) {
                        if (msdesc->isDevice())
                            return (cnt);
                    }
                }
            }
            if (DSP()->CurMode() == Electrical) {
                // Show the terminals
                WindowDesc *wdesc;
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                while ((wdesc = wgen.next()) != 0) {
                    if (wdesc->IsSimilar(Electrical, DSP()->MainWdesc())) {
                        int delta = (int)(2.0/wdesc->Ratio());
                        CDp_cnode *pn =
                            (CDp_cnode*)((CDc*)odesc)->prpty(P_NODE);
                        for ( ; pn; pn = pn->next()) {
                            for (unsigned int ix = 0; ; ix++) {
                                int xx, yy;
                                if (!pn->get_pos(ix, &xx, &yy))
                                    break;
                                DSP()->TLoadCurrent(tfnew);
                                DSP()->TPoint(&xx, &yy);
                                DSP()->TLoadCurrent(tfold);
                                wdesc->ShowLineW(xx - delta, yy,
                                    xx + delta, yy);
                                wdesc->ShowLineW(xx, yy - delta,
                                    xx, yy + delta);
                                cnt++;
                            }
                        }
                    }
                }
                if (msdesc && msdesc->isSymbolic())
                    return (cnt);
            }

            // Show the bounding box.

            BBox BB;
            Point *pts;
            odesc->boundary(&BB, &pts);
            if (pts) {
                DSP()->TLoadCurrent(tfnew);
                pts->xform(DSP(), 5);
                DSP()->TLoadCurrent(tfold);
                if (GEO()->curTx()->magset())
                    pts->scale(5, GEO()->curTx()->magn(), x, y);
                Gst()->ShowGhostPath(pts, 5);
                delete [] pts;
            }
            else {
                DSP()->TLoadCurrent(tfnew);
                DSP()->TBB(&BB, &pts);
                DSP()->TLoadCurrent(tfold);
                if (pts) {
                    if (GEO()->curTx()->magset())
                        pts->scale(5, GEO()->curTx()->magn(), x, y);
                    Gst()->ShowGhostPath(pts, 5);
                    delete [] pts;
                }
                else {
                    if (GEO()->curTx()->magset())
                        BB.scale(GEO()->curTx()->magn(), x, y);
                    Gst()->ShowGhostBox(BB.left, BB.bottom, BB.right, BB.top);
                }
            }
            cnt++;

            // Show origin.
            if (msdesc && !msdesc->isElectrical() &&
                    DSP()->ShowInstanceOriginMark()) {
                // See WindowDesc::ShowInstanceOriginMark.

                int xc = 0;
                int yc = 0;
                DSP()->TPush();
                DSP()->TApplyTransform((CDc*)odesc);
                DSP()->TPoint(&xc, &yc);
                DSP()->TPop();
                if (GEO()->curTx()->magset()) {
                    double dx = (xc - refx)*GEO()->curTx()->magn();
                    xc = mmRnd(dx) + refx;
                    double dy = (yc - refy)*GEO()->curTx()->magn();
                    yc = mmRnd(dy) + refy;
                }
                CDtx tx((CDc*)odesc);
                bool b1 = tx.ax && tx.ay;
                bool b2 = (GEO()->curTx()->angle() % 90);
                bool do45 = !(b1 + b2 == 1);

                const int delta = -4;  // Negative => log-scaling.
                DSP()->TLoadCurrent(tfnew);
                Gst()->ShowGhostCross(xc, yc, delta, do45);
                DSP()->TLoadCurrent(tfold);
                cnt++;
            }
            return (cnt);
        }
    }


    // Return the object with the largest area from among the next
    // n items in the list, and advance the list.
    //
    CDo *pick_one(CDol **plist, int n)
    {
        double amx = 0.0;
        int i = 0;
        CDo *opick = 0;
        while (*plist && i < n) {
            CDo *od = (*plist)->odesc;
            *plist = (*plist)->next;
            i++;
            double a = od->area();
            if (a > amx) {
                amx = a;
                opick = od;
            }
        }
        return (opick);
    }


    // The objects list for ghosting during move/copy.
    CDol *ghost_display_list;
    CDol *ghost_display_inst_list;
}


// Function to show the current object(s) ghost-drawn and attached to
// the pointer.
//
void
cEditGhost::showGhostMove(int new_x, int new_y, int ref_x, int ref_y)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return;

    if (EV()->IsConstrained())
        XM()->To45(ref_x, ref_y, &new_x, &new_y);
    CDtf tfold, tfnew;
    DSP()->TCurrent(&tfold);
    DSP()->TPush();
    ED()->findContact(&new_x, &new_y, ref_x, ref_y, true);
    GEO()->applyCurTransform(DSP(), ref_x, ref_y, new_x, new_y);
    DSP()->TCurrent(&tfnew);
    DSP()->TPop();

    unsigned int cnt = 0;
    // Show display list objects first.
    for (CDol *ol = ghost_display_list; ol; ol = ol->next) {
        cnt += display_ghost_move(ol->odesc, &tfold, &tfnew, ref_x, ref_y,
            new_x, new_y, 0);
        if (cnt > eg_max_ghost_objects)
            return;
    }

    // Show subcells, drawing upper cells before deeper ones.
    for (int dmode = 0; dmode < CDMAXCALLDEPTH; dmode++) {
        ghost_not_done = false;
        for (CDol *ol = ghost_display_inst_list; ol; ol = ol->next) {
            cnt += display_ghost_move(ol->odesc, &tfold, &tfnew, ref_x, ref_y,
                new_x, new_y, dmode);
            if (cnt > eg_max_ghost_objects)
                break;
        }
        if (!ghost_not_done)
            break;
        if (cnt > eg_max_ghost_objects)
            break;  // Limit reached, we're done.
    }
}


// Static function.
// Initialize/free the object list for ghosting during move/copy.
//
void
cEditGhost::ghost_move_setup(bool on)
{
    if (on) {
        CDs *sd = CurCell();
        if (CopyCmd && CopyCmd->alt_source())
            sd = CopyCmd->alt_source();
        ghost_display_list->free();         // should never need this
        ghost_display_list = Selections.listQueue(sd);
        ghost_display_inst_list->free();    // should never need this
        ghost_display_inst_list = 0;

        unsigned int n = Selections.queueLength(sd);
        if (n > EGst()->eg_max_ghost_objects) {
            // Too many objects to show efficiently.  Keep only the
            // objects with the largest area among groups.

            unsigned int n1 = n/EGst()->eg_max_ghost_objects;
            unsigned int n2 = n%EGst()->eg_max_ghost_objects;
            CDol *o0 = ghost_display_list;
            CDol *ol0 = 0;
            unsigned int j = 0, np;
            for (unsigned int i = 0; i < n; i += np) {
                np = n1 + (++j < n2);
                CDo *od = pick_one(&o0, np);
                if (!od)
                    break;
                ol0 = new CDol(od, ol0);
            }
            ghost_display_list->free();
            ghost_display_list = ol0;
        }
        // Separate the cell instances.
        CDol *ox, *op = 0;
        for (CDol *o = ghost_display_list; o; o = ox) {
            ox = o->next;
            if (o->odesc->type() == CDINSTANCE) {
                if (op)
                    op->next = ox;
                else
                    ghost_display_list = ox;
                o->next = ghost_display_inst_list;
                ghost_display_inst_list = o;
                continue;
            }
            op = o;
        }
    }
    else {
        ghost_display_list->free();
        ghost_display_list = 0;
        ghost_display_inst_list->free();
        ghost_display_inst_list = 0;
    }
}

