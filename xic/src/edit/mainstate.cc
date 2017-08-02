
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
#include "edit_menu.h"
#include "undolist.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "tech.h"
#include "events.h"
#include "layertab.h"
#include "menu.h"
#include "modf_menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "ghost.h"

#include "cd_propnum.h"
#include "pcell.h"
#include "pcell_params.h"

//-----------------------------------------------------------------------------
// The following sets up the default state, outside of commands.  A
// click over no selected objects or with neither the Shift or Control
// keys pressed performs a point selection operation.  A drag that
// begins immediately with neither the Shift or Control keys pressed
// initiates an area select operation.  If the button is pressed and
// neither the Shift or Control keys are pressed, and the pointer does
// not move for a delay period, a move/copy is initiated.  If the
// Shift key is down when the move/copy is terminated, a copy is
// performed, otherwise a move is performed.
//
// If the Shift key is down before the initial button press, and the
// pointer is over a selected object, move/copy will be performed on
// all selected objects.
//
// If the Control key is down before the initial button press, and the
// pointer is over a selected and stretchable object, a stretch
// operation is initiated.  Pressing the Shift key during the stretch
// will constrain the stretch to the x or y dimension, which ever is
// greater.

namespace {
    namespace ed_mainstate {
        enum ActionType {Inactive, SelectObj, MoveObj, StretchObj};

        // Undo/redo state.
        enum URtype { URnone, URundo, URredo };

        inline struct MainState *Mcmd();

        struct MainState : public CmdState
        {
            friend inline MainState *Mcmd()
                {
                    return (MainState::instancePtr);
                }

            MainState(const char*, const char*);

            static int timeout1(void*);
            static void show_mcmesg();
            static void show_strmesg();
            static void show_mc_sel_mesg();
            static void show_str_sel_mesg();

            void b1down();
            void b1up();
            void b1down_altw();
            void b1up_altw();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();

        private:
            void set_op(ActionType);
            int rep_count();

            void SetLevel1() { Level = 1; EV()->SetMainStateReady(true); }
            void SetLevel2() { Level = 2; EV()->SetMainStateReady(false); }

            char *Types;
            BBox AOI;
            int Refx, Refy;
            int Orgx, Orgy;
            int Id;
            ActionType CurrentOp;
            bool MessageOn;
            bool PhysPropEdit;
            bool B1down;
            int Corner;
            int Ncopies;
            int Nundo;
            URtype OperState;
            URtype GhostState;

            static CDmcType CopyMode;
            static char def_str_types[];
            static const char *mcmesg;
            static const char *smesg;
            static MainState *instancePtr;
        };
    }
}

using namespace ed_mainstate;

CDmcType MainState::CopyMode = CDmove;
char MainState::def_str_types[] = {CDBOX, CDLABEL, CDWIRE, CDPOLYGON, '\0'};
const char *MainState::mcmesg = "Move/copy selected object.";
const char *MainState::smesg = "Stretch selected object.";

MainState *MainState::instancePtr = 0;


void
cEdit::initMainState()
{
    if (!Mcmd()) {
        EV()->RegisterMainState(new MainState("MAIN", 0));
        setMoveOrCopy(CDmove);
    }
}


MainState::MainState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    instancePtr = this;
    Level = 1;
    Types = def_str_types;
    Refx = Refy = 0;
    Orgx = Orgy = 0;
    Id = 0;
    CurrentOp = Inactive;
    MessageOn = false;
    PhysPropEdit = false;
    B1down = false;
    Corner = 0;
    Ncopies = 0;
    Nundo = 0;
    OperState = URnone;
    GhostState = URnone;
}


void
MainState::b1down()
{
    B1down = true;
    ED()->clearObjectList();
    ED()->setStretchBoxCode(0);
    if (Level == 1) {
        // Main button1 down.
        // Initialize: start the timer, and assume a pending selection.
        //

        // Revert to move, this seems better than leaving the state
        // persistent.
        CopyMode = CDmove;

        Ncopies = 0;
        Nundo = 0;
        Corner = 0;
        OperState = URnone;
        GhostState = URnone;
        PhysPropEdit = false;

        EV()->SetConstrained(EV()->Cursor().get_downstate() & GR_CONTROL_MASK);
        if (Selections.ptrMode() != PTRselect &&
                !(EV()->Cursor().get_downstate() &
                    (GR_SHIFT_MASK | GR_CONTROL_MASK | GR_ALT_MASK))) {
            // a modifier-less b1down in other than Select mode allows
            // physical properties to be edited or label scripts to be
            // exec'ed
            if (ED()->editPhysPrpty() || ED()->execLabelScript()) {
                PhysPropEdit = true;
                return;
            }
        }
        ED()->setPressLayer(LT()->CurLayer());
        EV()->Cursor().get_xy(&Refx, &Refy);
        EV()->SetMainStateReady(false);

        int dtime = 250;
        const char *s = CDvdb()->getVariable(VA_SelectTime);
        if (s && isdigit(*s)) {
            dtime = atoi(s);
            if (dtime < 100)
                dtime = 100;
            if (dtime > 1000)
                dtime = 1000;
        }
        Id = GRpkgIf()->AddTimer(dtime, timeout1, 0);
    }
    else {
        // Complete the move/copy.
        //
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        BBox nBB;
        if (CurrentOp == MoveObj) {
            if (x != Refx || y != Refy || !GEO()->curTx()->is_identity()) {
                Gst()->SetGhost(GFnone);
                XM()->SetCoordMode(CO_ABSOLUTE);
                OperState = URundo;
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
                if (CopyMode == CDcopy) {
                    int repcnt = rep_count();
                    ED()->copyObjects(Refx, Refy, x, y, ldold, ldnew, repcnt);
                    Ncopies++;
                    Nundo = 0;
                }
                else
                    ED()->moveObjects(Refx, Refy, x, y, ldold, ldnew);
            }
        }
        else if (x != Refx || y != Refy) {
            if (ED()->doStretch(Refx, Refy, &x, &y)) {
                XM()->SetCoordMode(CO_ABSOLUTE);
                OperState = URundo;
                if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
                    DSP()->ShowCellTerminalMarks(DISPLAY);
                Tech()->SetC45Callback(0);
                if (MessageOn) {
                    PL()->ErasePrompt();
                    MessageOn = false;
                }
            }
        }
    }
}


void
MainState::b1down_altw()
{
    // Nothing to do here.
}


#define STRETCHABLE(s) (s->odesc->type() == CDBOX || \
    s->odesc->type() == CDLABEL || s->odesc->type() == CDWIRE || \
    s->odesc->type() == CDPOLYGON)

namespace {
    void
    no_object_proc()
    {
        if (!CDvdb()->getVariable("NoDeselLast")) {
            CDs *cursd = CurCell();
            if (cursd) {
                sSelGen sg(Selections, cursd);
                CDo *od = sg.next();
                if (od) {
                    Selections.removeObject(cursd, od);
                    XM()->ShowParameters();
                }
            }
        }
    }
}


namespace {
    struct ms_pill
    {
        ms_pill(CDs *sd, hyEnt *ent)
            {
                sdesc = sd;
                hent = ent;
            }

        ~ms_pill()
            {
                delete hent;
            }

        CDs *sdesc;
        hyEnt *hent;
    };


    int subw_idle(void *arg)
    {
        ms_pill *p = (ms_pill*)arg;
        DSP()->OpenSubwin(p->sdesc, p->hent, true);
        delete p;
        return (false);
    }
}


void
MainState::b1up()
{
    B1down = false;
    if (Level == 1) {
        // Main button1 release.  If in selection mode, perform the
        // selection.  Otherwise, if the pointer has moved, perform
        // move/copy according to whether shift key is down.  If the
        // pointer has not moved, set the state to complete the
        // move/copy on the next button1 press.
        //
        if (PhysPropEdit)
            // Physical property edit, ignore b1up.
            return;
        XM()->SetCoordMode(CO_ABSOLUTE);
        bool clicked = false;
        if (Id) {
            // user clicked
            clicked = true;
            GRpkgIf()->RemoveTimer(Id);
            Id = 0;
        }
        CDs *cursd = CurCell();
        if (!cursd)
            return;
        if (clicked) {
click:
            // see if clicking on selected object
            EV()->Cursor().get_raw(&AOI.left, &AOI.bottom);
            AOI.right = AOI.left;
            AOI.top = AOI.bottom;
            if (Selections.ptrMode() == PTRselect) {
                set_op(SelectObj);
                if (!Selections.selection(CurCell(), 0, &AOI))
                    no_object_proc();
                return;
            }
            CDol *slist = Selections.selectItems(CurCell(), 0, &AOI,
                PSELpoint);

            // Handle property label hide/show.
            if ((EV()->Cursor().get_upstate() & GR_SHIFT_MASK)) {
                if (WindowDesc::LabelHideHandler(slist)) {
                    CDol::destroy(slist);
                    return;
                }
            }

            CDol *sl;
            for (sl = slist; sl; sl = sl->next) {
                if (sl->odesc->state() == CDSelected)
                    break;
            }
            unsigned int upstate = EV()->Cursor().get_upstate();
            bool shft = (upstate & GR_SHIFT_MASK);
            bool ctrl = (upstate & GR_CONTROL_MASK);

            if (Selections.ptrMode() == PTRmodify) {
                if (sl) {
                    if (!ctrl) {
                        set_op(MoveObj);
                        Gst()->SetGhostAt(GFmove, Refx, Refy);
                        SetLevel2();
                        show_mcmesg();
                    }
                    else if (STRETCHABLE(sl)) {
                        set_op(StretchObj);
                        Ulist()->ListCheck("stretch", cursd, true);
                        ED()->setStretchRef(&Refx, &Refy);
                        // Make it seem that the last point was
                        // pressed on the reference vertex, so that ^E
                        // will be relative to the reference.

                        EV()->SetConstrained(false);
                        Gst()->SetGhostAt(GFstretch, Refx, Refy);
                        SetLevel2();
                        show_strmesg();
                        Tech()->SetC45Callback(show_strmesg);
                    }
                }
            }
            else {
                if (sl && shft && !ctrl) {
                    set_op(MoveObj);
                    Gst()->SetGhostAt(GFmove, Refx, Refy);
                    SetLevel2();
                    show_mcmesg();
                }
                else if (shft && ctrl && slist &&
                        slist->odesc->type() == CDINSTANCE) {
                    // ctrl+shift on an pop up a sub-window showing
                    // the master if a subcircuit or in physical mode.

                    CDc *cd = (CDc*)slist->odesc;
                    CDs *msd = cd->masterCell(true);
                    if (msd && (!msd->isElectrical() ||
                            msd->elecCellType() == CDelecSubc)) {
                        hyEnt *ent = 0;
                        if (msd->isElectrical()) {
                            WindowDesc *wdesc = EV()->CurrentWin() ?
                                EV()->CurrentWin() : DSP()->MainWdesc();
                            int x, y;
                            EV()->Cursor().get_raw(&x, &y);
                            ent = new hyEnt;
                            ent->set_ref_type(HYrefDevice);
                            ent->set_odesc(cd);
                            ent->set_pos_x(x);
                            ent->set_pos_y(y);
                            ent->set_owner(CurCell(Electrical));
                            ent->set_proxy(wdesc->ProxyList());
                        }
                        ms_pill *p = new ms_pill(msd, ent);
                        dspPkgIf()->RegisterIdleProc(subw_idle, p);
                    }
                }
                else if (ctrl && sl) {
                    if (STRETCHABLE(sl)) {
                        if (shft) {
                            // Call the "real" stretch command, allows
                            // vertex selection.

                            CDol::destroy(slist);
                            Menu()->MenuButtonPress("main", MenuSTRCH);
                            return;
                        }
                        set_op(StretchObj);
                        Ulist()->ListCheck("stretch", cursd, true);
                        ED()->setStretchRef(&Refx, &Refy);
                        // Make it seem that the last point was
                        // pressed on the reference vertex, so that ^E
                        // will be relative to the reference
                        EV()->SetConstrained(false);
                        Gst()->SetGhostAt(GFstretch, Refx, Refy);
                        SetLevel2();
                        show_strmesg();
                        Tech()->SetC45Callback(show_strmesg);
                    }
                    else if (sl->odesc->type() == CDINSTANCE) {
                        // Ctrl-clicked on a selected instance.  If
                        // the instance is a pcell, show the
                        // parameters, otherwise pop up the property
                        // editor.

                        CDc *cd = (CDc*)sl->odesc;
                        CDs *msd = cd->masterCell();
                        if (msd && msd->isPCellSubMaster()) {
                            char *pstr;
                            if (ED()->reparameterize(cd, &pstr)) {
                                CDs *sd = CurCell(Physical);
                                Ulist()->ListCheck("pccng", sd, false);
                                CDp *newp = new CDp(pstr, XICP_PC_PARAMS);
                                CDp *oldp = cd->prpty(XICP_PC_PARAMS);
                                Ulist()->RecordPrptyChange(sd, cd, oldp,
                                    newp);
                                Ulist()->CommitChanges(true);
                                delete [] pstr;
                            }
                        }
                        else if (msd && msd->isViaSubMaster()) {
                            // Edit standard via parameters.
                            ED()->PopUpStdVia(0, MODE_ON, cd);
                        }
                        else if (!Menu()->MenuButtonStatus("edit", MenuPRPTY))
                            Menu()->MenuButtonPress("edit", MenuPRPTY);
                    }
                }
                else {
                    set_op(SelectObj);
                    if (!Selections.selection(CurCell(), 0, &AOI))
                        no_object_proc();
                }
            }
            CDol::destroy(slist);
            return;
        }
        if (EV()->Cursor().is_release_ok() && EV()->CurrentWin()) {
            if (ED()->resetGrips()) {
                // This test must come first.
                // We've handled moving a grip.
                return;
            }
            if (CurrentOp == SelectObj) {
                EV()->Cursor().get_raw(&AOI.left, &AOI.top);
                EV()->Cursor().get_release(&AOI.right, &AOI.bottom);
                AOI.fix();
                Gst()->SetGhost(GFnone);

                // If the pointer hasn't moved, treat it as a click.
                if (AOI.width() == 0 && AOI.height() == 0)
                    goto click;

                XM()->SetCoordMode(CO_ABSOLUTE);
                Selections.selection(CurCell(), 0, &AOI);
                return;
            }

            int x, y;
            EV()->Cursor().get_release(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            if (x == Refx && y == Refy) {
                // Complete operation on next button-down.
                SetLevel2();
                return;
            }

            if (CurrentOp == MoveObj) {
                Gst()->SetGhost(GFnone);
                XM()->SetCoordMode(CO_ABSOLUTE);
                OperState = URundo;
                if (CopyMode == CDcopy) {
                    int repcnt = rep_count();
                    ED()->copyObjects(Refx, Refy, x, y, 0, 0, repcnt);
                    Ncopies++;
                    Nundo = 0;
                }
                else
                    ED()->moveObjects(Refx, Refy, x, y, 0, 0);
                if (CopyMode == CDcopy) {
                    SetLevel2();
                    Gst()->SetGhostAt(GFmove, Refx, Refy);
                    XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
                }
                else if (MessageOn) {
                    PL()->ErasePrompt();
                    MessageOn = false;
                }
            }
            else if (CurrentOp == StretchObj) {
                BBox nBB;
                if (ED()->doStretch(Refx, Refy, &x, &y)) {
                    XM()->SetCoordMode(CO_ABSOLUTE);
                    OperState = URundo;
                    if (DSP()->CurMode() == Electrical &&
                            DSP()->ShowTerminals())
                        DSP()->ShowCellTerminalMarks(DISPLAY);
                    Tech()->SetC45Callback(0);
                    if (MessageOn) {
                        PL()->ErasePrompt();
                        MessageOn = false;
                    }
                }
                else {
                    // Complete stretch on next button1 press.
                    SetLevel2();
                }
            }
        }
        else {
            if (CurrentOp == Inactive)
                return;
            if (CurrentOp == SelectObj) {
                Gst()->SetGhost(GFnone);
                XM()->SetCoordMode(CO_ABSOLUTE);
            }
            else
                SetLevel2();
        }
    }
    else {
        // Finished.  Reset state if success.
        //
        Tech()->SetC45Callback(0);
        OperState = URundo;
        if (CurrentOp == MoveObj && CopyMode == CDcopy) {
            SetLevel2();
            Gst()->SetGhostAt(GFmove, Refx, Refy);
            XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
        }
        else {
            SetLevel1();
            if (MessageOn) {
                PL()->ErasePrompt();
                MessageOn = false;
            }
        }
    }
}


namespace {
    // Return a pointer to a subcircuit under x,y if any.
    //
    CDc *find_instance(CDs *sdesc, int x, int y)
    {
        if (!sdesc)
            return (0);
        BBox BB(x, y, x, y);
        BB.bloat(10);
        CDg gdesc;   
        gdesc.init_gen(sdesc, CellLayer(), &BB);
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {
            CDs *msd = cdesc->masterCell();
            if (!msd)
                continue;
            if (msd->isElectrical() && msd->elecCellType() != CDelecSubc)
                continue;
            break;
        }
        return (cdesc);
    }
}


void
MainState::b1up_altw()
{
    // If Ctrl+Shift click on an instance, pop up a sub-window showing
    // the instance master.  Create the proxy chain in electrical
    // mode.

    unsigned int state = 0;
    DSPmainDraw(QueryPointer(0, 0, &state))
    if (!(state & GR_SHIFT_MASK) || !(state & GR_CONTROL_MASK))
        return;

    WindowDesc *wdesc = EV()->CurrentWin();
    if (!wdesc)
        return;

    CDs *sdesc = wdesc->CurCellDesc(wdesc->Mode(), true);
    if (!sdesc)
        return;

    if (!EV()->Cursor().get_press_alt())
        return;
    int x, y;
    EV()->Cursor().get_alt_down(&x, &y);

    // Ignore drag.
    int xp = x;
    int yp = y;
    wdesc->Snap(&xp, &yp);
    int xr, yr;
    EV()->Cursor().get_alt_up(&xr, &yr);
    wdesc->Snap(&xr, &yr);
    if (xp != xr || yp != yr)
        return;

    CDc *cd = find_instance(sdesc, x, y);
    if (!cd)
        return;

    CDs *msd = cd->masterCell(true);
    if (msd && (!msd->isElectrical() ||
            msd->elecCellType() == CDelecSubc)) {
        hyEnt *ent = 0;
        if (msd->isElectrical()) {
            ent = new hyEnt;
            ent->set_ref_type(HYrefDevice);
            ent->set_odesc(cd);
            ent->set_pos_x(x);
            ent->set_pos_y(y);
            ent->set_owner(CurCell(Electrical));
            ent->set_proxy(wdesc->ProxyList());
        }
        ms_pill *p = new ms_pill(msd, ent);
        dspPkgIf()->RegisterIdleProc(subw_idle, p);
    }
}


// Esc entered, abort any pending operation.
//
void
MainState::esc()
{
    SetLevel1();
    if (Id) {
        GRpkgIf()->RemoveTimer(Id);
        Id = 0;
    }
    Corner = 0;
    Ncopies = 0;
    Nundo = 0;
    OperState = URnone;
    GhostState = URnone;
    Gst()->SetGhost(GFnone);
    Tech()->SetC45Callback(0);
    XM()->SetCoordMode(CO_ABSOLUTE);
    PL()->ErasePrompt();
    MessageOn = false;
    PhysPropEdit = false;
    B1down = false;
    set_op(Inactive);
    ED()->setMoveOrCopy(CDmove);
    ED()->resetGrips(true);
}


// Set the switch for the shift key, and respond to Delete key
// presses by calling the delete code.
//
bool
MainState::key(int code, const char *text, int)
{
    if (Level == 1) {
        switch (code) {
        case CTRLDN_KEY:
            if (CurrentOp != MoveObj) {
                if (Selections.hasTypes(CurCell(), "bpwl"))
                    EV()->SetMotionTask(show_str_sel_mesg);
            }
            break;
        case CTRLUP_KEY:
            EV()->SetMotionTask(0);
            if (CurrentOp != MoveObj && CurrentOp != StretchObj) {
                if (MessageOn) {
                    PL()->ErasePrompt();
                    MessageOn = false;
                }
            }
            break;
        case SHIFTDN_KEY:
            if (B1down) {
                if (CurrentOp == MoveObj) {
                    DSPmainDraw(ShowGhost(ERASE))
                    EV()->SetConstrained(true);
                    DSPmainDraw(ShowGhost(DISPLAY))
                    show_mcmesg();
                }
                else if (CurrentOp == StretchObj) {
                    DSPmainDraw(ShowGhost(ERASE))
                    EV()->SetConstrained(true);
                    DSPmainDraw(ShowGhost(DISPLAY))
                    show_strmesg();
                }
                break;
            }
            if (Selections.hasTypes(CurCell(), 0))
                EV()->SetMotionTask(show_mc_sel_mesg);
            break;
        case SHIFTUP_KEY:
            if (B1down) {
                if (CurrentOp == MoveObj) {
                    DSPmainDraw(ShowGhost(ERASE))
                    EV()->SetConstrained(false);
                    DSPmainDraw(ShowGhost(DISPLAY))
                    show_mcmesg();
                }
                else if (CurrentOp == StretchObj) {
                    DSPmainDraw(ShowGhost(ERASE))
                    EV()->SetConstrained(false);
                    DSPmainDraw(ShowGhost(DISPLAY))
                    show_strmesg();
                }
                break;
            }
            EV()->SetMotionTask(0);
            if (MessageOn) {
                PL()->ErasePrompt();
                MessageOn = false;
            }
            break;
        case DELETE_KEY:
            EV()->SetMotionTask(0);
            esc();
            ED()->deleteExec(0);
            break;
        case RETURN_KEY:
            if (B1down && CurrentOp == MoveObj &&
                    DSP()->CurMode() == Physical) {
                BBox sBB;
                Gst()->SetGhost(GFnone);
                Selections.computeBB(CurCell(), &sBB, false);
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
                break;
            }
            return (false);
        default:
            EV()->SetMotionTask(0);
            if (DSP()->CurMode() == Electrical) {
                if (text && *text == 1) {    // Ctrl-A
                    Selections.addLabels(CurCell());
                    break;
                }
                if (text && *text == 16) {   // Ctrl-P
                    Selections.purgeLabels(CurCell());
                    break;
                }
            }
            if (*text == ' ') {
                // space bar
                if (B1down && CurrentOp == MoveObj) {
                    CopyMode = (CopyMode == CDcopy ? CDmove : CDcopy);
                    show_mcmesg();
                    DSPmainDraw(ShowGhost(ERASE))
                    ED()->setMoveOrCopy(CopyMode);
                    DSPmainDraw(ShowGhost(DISPLAY))
                    Ncopies = 0;
                    Nundo = 0;
                    break;
                }
            }
            return (false);
        }
        return (true);
    }
    else {
        switch (code) {
        case CTRLDN_KEY:
            break;
        case CTRLUP_KEY:
            break;
        case SHIFTDN_KEY:
            if (CurrentOp == MoveObj) {
                DSPmainDraw(ShowGhost(ERASE))
                EV()->SetConstrained(true);
                DSPmainDraw(ShowGhost(DISPLAY))
                show_mcmesg();
            }
            else if (CurrentOp == StretchObj) {
                DSPmainDraw(ShowGhost(ERASE))
                EV()->SetConstrained(true);
                DSPmainDraw(ShowGhost(DISPLAY))
                show_strmesg();
            }
            break;
        case SHIFTUP_KEY:
            if (CurrentOp == MoveObj) {
                DSPmainDraw(ShowGhost(ERASE))
                EV()->SetConstrained(false);
                DSPmainDraw(ShowGhost(DISPLAY))
                // Avoid pop-up.
                if (!EV()->InCoordEntry())
                    show_mcmesg();
            }
            else if (CurrentOp == StretchObj) {
                DSPmainDraw(ShowGhost(ERASE))
                EV()->SetConstrained(false);
                DSPmainDraw(ShowGhost(DISPLAY))
                // Avoid pop-up.
                if (!EV()->InCoordEntry())
                    show_strmesg();
            }
            break;
        case DELETE_KEY:
            esc();
            ED()->deleteExec(0);
            break;
        case RETURN_KEY:
            if (CurrentOp == MoveObj && DSP()->CurMode() == Physical) {
                BBox sBB;
                Gst()->SetGhost(GFnone);
                Selections.computeBB(CurCell(), &sBB, false);
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
                break;
            }
            return (false);
        default:
            if (*text == ' ') {
                // space bar
                if (CurrentOp == MoveObj) {
                    CopyMode = (CopyMode == CDcopy ? CDmove : CDcopy);
                    show_mcmesg();
                    DSPmainDraw(ShowGhost(ERASE))
                    ED()->setMoveOrCopy(CopyMode);
                    DSPmainDraw(ShowGhost(DISPLAY))
                    Ncopies = 0;
                    Nundo = 0;
                    break;
                }
            }
            return (false);
        }
        return (true);
    }
}


void
MainState::undo()
{
    if (Level == 1) {
        if (OperState == URundo) {
            if (CurrentOp == MoveObj && CopyMode == CDcopy)
                Ulist()->UndoOperation();
            else {
                // Finished complete operation.
                OperState = URredo;
                Ulist()->UndoOperation();
                if (!Selections.hasTypes(CurCell(), 0))
                    esc();
                if (CurrentOp == StretchObj) {
                    Gst()->SetGhostAt(GFstretch, Refx, Refy);
                    XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
                    SetLevel2();
                    Tech()->SetC45Callback(show_strmesg);
                }
                else {
                    Gst()->SetGhostAt(GFmove, Refx, Refy);
                    XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
                    SetLevel2();
                }
            }
        }
        else
            Ulist()->UndoOperation();
    }
    else if (Level == 2) {
        if (CurrentOp == MoveObj && CopyMode == CDcopy) {
            if (Ncopies) {
                if (Nundo == 0)
                    Nundo = Ncopies;
                Ncopies--;
                Ulist()->UndoOperation();
                return;
            }
            OperState = URredo;
        }
        GhostState = URredo;
        Gst()->SetGhost(GFnone);
        XM()->SetCoordMode(CO_ABSOLUTE);
        Tech()->SetC45Callback(0);
        SetLevel1();
        if (MessageOn) {
            PL()->ErasePrompt();
            MessageOn = false;
        }
    }
}


// If a selection was just undone, redo it.  Otherwise call the
// application redo code.
//
void
MainState::redo()
{
    if (Level == 1) {
        if (GhostState == URredo) {
            GhostState = URundo;
            if (CurrentOp == StretchObj) {
                Gst()->SetGhostAt(GFstretch, Refx, Refy);
                XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
                SetLevel2();
                Tech()->SetC45Callback(show_strmesg);
            }
            else {
                Gst()->SetGhostAt(GFmove, Refx, Refy);
                XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
                SetLevel2();
            }
        }
        else
            Ulist()->RedoOperation();
    }
    else if (Level == 2) {
        if (OperState == URredo) {
            if (Ulist()->HasRedo()) {
                if (Ncopies < Nundo)
                    Ncopies++;
                Ulist()->RedoOperation();
                return;
            }
            if (CurrentOp == MoveObj && CopyMode == CDcopy)
                return;
            Gst()->SetGhost(GFnone);
            XM()->SetCoordMode(CO_ABSOLUTE);
            Tech()->SetC45Callback(0);
            Ulist()->RedoOperation();
            SetLevel1();
            OperState = URundo;
        }
    }
}


void
MainState::set_op(ActionType op)
{
    CurrentOp = op;
    /* We don't do this anymore, as it is more confusing than it is
     * worth, and the logic is inconsistant.
    if (op == Inactive)
        StateName = "Idle";
    else if (op == SelectObj) 
        StateName = "Select";
    else if (op == MoveObj)
        StateName = "Move";
    else if (op == StretchObj)
        StateName = "Stretch";
    */
    XM()->ShowParameters();
}


// Callback for timer started by button1 press.  If the pointer is on
// a selected object, and has not moved appreciably between press and
// current position, attach the object to the pointer and start a
// move/copy operation.  Otherwise, show the ghost rectangle for a
// selection operation.
//
int
MainState::timeout1(void*)
{
    if (Mcmd()->Id) {
        Mcmd()->Id = 0;

        CDs *cursd = CurCell();
        if (!cursd)
            return (false);

        const unsigned int downstate = EV()->Cursor().get_downstate();

        // If we're over a grip, maybe start a grip drag operation.
        if (!(downstate & (GR_SHIFT_MASK | GR_CONTROL_MASK)) &&
                ED()->checkGrips())
            return (false);

        // see if pointing at selected object
        BBox BB;
        EV()->Cursor().get_raw(&BB.left, &BB.top);
        BB.right = BB.left;
        BB.bottom = BB.top;
        if (Selections.ptrMode() == PTRselect) {
            Gst()->SetGhost(GFbox_ns);
            XM()->SetCoordMode(CO_RELATIVE, BB.left, BB.bottom);
            Mcmd()->set_op(SelectObj);
            return (false);
        }
        CDol *slist = Selections.selectItems(CurCell(), 0, &BB, PSELpoint);
        CDol *sl;
        for (sl = slist; sl; sl = sl->next)
            if (sl->odesc->state() == CDSelected)
                break;
        if (Selections.ptrMode() == PTRmodify) {
            if (sl && !(downstate & GR_CONTROL_MASK)) {
                XM()->SetCoordMode(CO_RELATIVE, Mcmd()->Refx, Mcmd()->Refy);
                Gst()->SetGhostAt(GFmove, Mcmd()->Refx, Mcmd()->Refy);
                Mcmd()->set_op(MoveObj);
                show_mcmesg();
            }
            else if (sl && STRETCHABLE(sl) && !(downstate & GR_SHIFT_MASK)) {
                Ulist()->ListCheck("stretch", cursd, true);
                ED()->setStretchRef(&Mcmd()->Refx, &Mcmd()->Refy);
                EV()->SetConstrained(false);
                XM()->SetCoordMode(CO_RELATIVE, Mcmd()->Refx, Mcmd()->Refy);
                Gst()->SetGhostAt(GFstretch, Mcmd()->Refx, Mcmd()->Refy);
                Mcmd()->set_op(StretchObj);
                show_strmesg();
                Tech()->SetC45Callback(show_strmesg);
            }
        }
        else {
            bool moved = false;
            if (!(downstate & (GR_SHIFT_MASK | GR_CONTROL_MASK))) {
                // see if pointer has moved
                int x, y;
                WindowDesc *wdesc = EV()->CurrentWin() ?
                    EV()->CurrentWin() : DSP()->MainWdesc();
                wdesc->Wdraw()->QueryPointer(&x, &y, 0);
                wdesc->PToL(x, y, x, y);
                int xr, yr;
                EV()->Cursor().get_raw(&xr, &yr);
                x -= xr;
                y -= yr;
                x = (int)(x*wdesc->Ratio());
                y = (int)(y*wdesc->Ratio());
                moved = ((x*x + y*y) > 256);
                // moved more than 16 pixels
            }
            if (sl && !moved && !(downstate & GR_CONTROL_MASK)) {
                XM()->SetCoordMode(CO_RELATIVE, Mcmd()->Refx, Mcmd()->Refy);
                Gst()->SetGhostAt(GFmove, Mcmd()->Refx, Mcmd()->Refy);
                Mcmd()->set_op(MoveObj);
                show_mcmesg();
            }
            else if (sl && STRETCHABLE(sl) && !moved &&
                    !(downstate & GR_SHIFT_MASK)) {
                Ulist()->ListCheck("stretch", cursd, true);
                ED()->setStretchRef(&Mcmd()->Refx, &Mcmd()->Refy);
                EV()->SetConstrained(false);
                XM()->SetCoordMode(CO_RELATIVE, Mcmd()->Refx, Mcmd()->Refy);
                Gst()->SetGhostAt(GFstretch, Mcmd()->Refx, Mcmd()->Refy);
                Mcmd()->set_op(StretchObj);
                show_strmesg();
                Tech()->SetC45Callback(show_strmesg);
            }
            else {
                Gst()->SetGhost(GFbox_ns);
                XM()->SetCoordMode(CO_RELATIVE, BB.left, BB.bottom);
                Mcmd()->set_op(SelectObj);
            }
        }
        CDol::destroy(slist);
    }
    return (false);
}


void
MainState::show_mcmesg()
{
    if (CopyMode == CDcopy) {
        if (EV()->IsConstrained())
            PL()->ShowPrompt(
                "COPY mode (space bar toggles), motion constrained.");
        else
            PL()->ShowPrompt(
                "COPY mode (space bar toggles).");
    }
    else {
        if (EV()->IsConstrained())
            PL()->ShowPrompt(
                "MOVE mode (space bar toggles), motion constrained.");
        else
            PL()->ShowPrompt(
                "MOVE mode (space bar toggles).");
    }
    Mcmd()->MessageOn = true;
    EV()->SetMotionTask(0);
}


void
MainState::show_strmesg()
{
    if (Tech()->IsConstrain45() ^ EV()->IsConstrained())
        PL()->ShowPrompt("Stretching, motion constrained.");
    else
        PL()->ShowPrompt("Stretching");
    Mcmd()->MessageOn = true;
    EV()->SetMotionTask(0);
}


void
MainState::show_mc_sel_mesg()
{
    PL()->ShowPrompt(mcmesg);
    Mcmd()->MessageOn = true;
    EV()->SetMotionTask(0);
}


void
MainState::show_str_sel_mesg()
{
    PL()->ShowPrompt(smesg);
    Mcmd()->MessageOn = true;
    EV()->SetMotionTask(0);
}


int
MainState::rep_count()
{
    char buf[32];
    PL()->GetTextBuf(EV()->CurrentWin(), buf);
    bool digits = false;
    for (char *s = buf; *s; s++) {
        if (isdigit(*s))
            digits = true;
        else if (digits) {
            digits = false;
            break;
        }
    }
    int repcnt = digits ? atoi(buf) : 1;
    if (repcnt < 0 || repcnt > 100000)
        repcnt = 1;
    return (repcnt);
}

