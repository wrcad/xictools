
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
#include "undolist.h"
#include "scedif.h"
#include "dsp_tkif.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "tech.h"
#include "events.h"
#include "layertab.h"
#include "menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "ghost.h"


#ifndef M_PI
#define	M_PI  3.14159265358979323846  // pi
#endif

namespace {
    namespace ed_rotate {
        struct RotState : public CmdState
        {
            friend void cEditGhost::showGhostRotate(int, int, int, int, bool);

            RotState(const char*, const char*);
            virtual ~RotState();

            void setCaller(GRobject c)  { Caller = c; }

            void b1down();
            void b1up();
            void desel();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();

            void message() { if (Level == 1) PL()->ShowPrompt(msg1);
                else if (Level == 2) PL()->ShowPrompt(msg2);
                else if (Level == 3) PL()->ShowPrompt(msg3);
                else if (Level == 4) PL()->ShowPrompt(msg4); }

        private:
            double get_ang(int, int, int, int);
            void SetLevel1(bool show) { Level = 1; if (show) message(); }
            void SetLevel2(bool show) { Level = 2; if (show) message(); }
            void SetLevel3() { Level = 3; message(); }
            void SetLevel4() { Level = 4; message(); }
            void SetLevel5() { Level = 5; }

            static void accum_init()
                {
                    WindowDesc *wdesc;
                    WDgen wgen(WDgen::MAIN, WDgen::CHD);
                    while ((wdesc = wgen.next()) != 0) {
                        if (Gst()->ShowingGhostInWindow(wdesc))
                            wdesc->SetAccumMode(WDaccumStart);
                    }
                }

            static void accum_final()
                {
                    WindowDesc *wdesc;
                    WDgen wgen(WDgen::MAIN, WDgen::CHD);
                    while ((wdesc = wgen.next()) != 0) {
                        if (Gst()->ShowingGhostInWindow(wdesc))
                            wdesc->GhostFinalUpdate();
                    }
                }
                

            static int MsgTimeout(void*);

            GRobject Caller;
            char *Types;
            int State;
            int Refx, Refy;
            int Movx, Movy;
            bool GotOne;
            bool Constrain;
            bool ForceConstrain;
            bool GhostOn;
            double RefAng;
            CDol *OrigObjs;

            static const char *msg1;
            static const char *msg2;
            static const char *msg3;
            static const char *msg4;
        };

        RotState *RotCmd;
    }

    bool ShowDegrees = true;
}

using namespace ed_rotate;

const char *RotState::msg1 = "Select objects to rotate.";
const char *RotState::msg2 =
    "Click on the rotation origin, or drag to also set angle.";
const char *RotState::msg3 =
    "Click to define angle (press Enter orclick on crosshair for text entry).";
const char *RotState::msg4 =
    "Press Enter, or Click on new location (hold Shift for copy).";


void
cEdit::rotationExec(CmdDesc *cmd)
{
    if (RotCmd) {
        RotCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, false, DSP()->CurMode()))
        return;

    RotCmd = new RotState("ROTATE", "xic:spin");
    RotCmd->setCaller(cmd ? cmd->caller : 0);
    // Original objects are selected after undo.
    Ulist()->ListCheck(RotCmd->Name(), CurCell(), true);
    if (!EV()->PushCallback(RotCmd)) {
        delete RotCmd;
        return;
    }
    RotCmd->message();
    ds.clear();
}


namespace { const char Types45[] = { CDINSTANCE, CDLABEL, 0 }; }

RotState::RotState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    Types = 0;  // this can be used to restrict types
    State = 0;
    Refx = Refy = 0;
    Movx = Movy = 0;
    Constrain = false;
    ForceConstrain = false;
    GhostOn = false;
    RefAng = 0.0;
    OrigObjs = 0;
    GotOne = Selections.hasTypes(CurCell(), Types, false);
    if (GotOne) {
        ForceConstrain = Selections.hasTypes(CurCell(), Types45, false);
        SetLevel2(false);
    }
    else
        SetLevel1(false);
}


RotState::~RotState()
{
    RotCmd = 0;
    CDol::destroy(OrigObjs);
}


void
RotState::b1down()
{
    if (Level == 1)
        cEventHdlr::sel_b1down();
    else if (Level == 2) {
        // Identify reference point.
        //
        EV()->Cursor().get_xy(&Refx, &Refy);
        DSP()->ShowCrossMark(DISPLAY, Refx, Refy, HighlightingColor,
            20, DSP()->CurMode());
        XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
        State = 2;
        Selections.setShowSelected(CurCell(), Types, false);
        Gst()->SetGhostAt(GFrotate, Refx, Refy);
        accum_init();
        GhostOn = true;
        ED()->setPressLayer(LT()->CurLayer());
        EV()->DownTimer(GFrotate);
    }
    else if (Level == 3) {
        // Finish the rotate.
        //
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        int delta = 1 + (int)(DSP()->PixelDelta()/EV()->CurrentWin()->Ratio());
        if (abs(x - Refx) <= delta && abs(y - Refy) <= delta) {
            Gst()->SetGhost(GFnone);
            accum_final();
            GhostOn = false;
            char *in = PL()->EditPrompt(
                "Enter angle, degrees counter-clockwise from x-axis: ", 0);
            if (!RotCmd)
                return;
            if (in && sscanf(in, "%lf", &RefAng) == 1)
                RefAng *= M_PI/180;
            else
                RefAng = 0;
            State = 3;
            SetLevel4();  // EditPrompt() swallows btn up event
            Gst()->SetGhostAt(GFrotate, Refx, Refy);
            accum_init();
            GhostOn = true;
        }
        else {
            RefAng = get_ang(x, y, Refx, Refy);
            State = 3;
            SetLevel4();
        }
    }
    else if (Level == 4) {
        EV()->Cursor().get_xy(&Movx, &Movy);
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
        if (ED()->rotateQueue(Refx, Refy, RefAng, Movx, Movy, ldold, ldnew,
                (EV()->Cursor().get_downstate() & GR_SHIFT_MASK) ?
                CDcopy : CDmove)) {
            Gst()->SetGhost(GFnone);
            accum_final();
            GhostOn = false;
            XM()->SetCoordMode(CO_ABSOLUTE);
            State = 4;
            DSP()->ShowCrossMark(ERASE, Refx, Refy, HighlightingColor,
                20, DSP()->CurMode());
            Ulist()->CommitChanges(true);
            SetLevel5();
        }
    }
}


void
RotState::b1up()
{
    if (Level == 1) {
        // Confirm the selection, and set up for rotate.
        //
        if (!cEventHdlr::sel_b1up(0, Types, 0))
            return;
        if (!Selections.hasTypes(CurCell(), Types, false))
            return;
        ForceConstrain = Selections.hasTypes(CurCell(), Types45, false);
        SetLevel2(true);
        State = 1;
    }
    else if (Level == 2) {
        // Finish the rotate if the pointer moved, otherwise set up to
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
                RefAng = get_ang(x, y, Refx, Refy);
                State = 3;
                SetLevel4();
                return;
            }
        }
        SetLevel3();
    }
    else if (Level == 3)
        SetLevel4();
    else if (Level == 4) {
        if (EV()->Cursor().is_release_ok()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            int xc, yc;
            EV()->Cursor().get_xy(&xc, &yc);
            if (x != xc || y != yc) {
                Movx = x;
                Movy = y;
                if (ED()->rotateQueue(Refx, Refy, RefAng, Movx, Movy, 0, 0,
                        (EV()->Cursor().get_downstate() & GR_SHIFT_MASK) ?
                        CDcopy : CDmove)) {
                    Gst()->SetGhost(GFnone);
                    accum_final();
                    GhostOn = false;
                    XM()->SetCoordMode(CO_ABSOLUTE);
                    State = 4;
                    DSP()->ShowCrossMark(ERASE, Refx, Refy, HighlightingColor,
                        20, DSP()->CurMode());
                    Ulist()->CommitChanges(true);
                    SetLevel1(true);
                    return;
                }
            }
        }
    }
    else if (Level == 5) {
        // Done.
        //
        if (State == 4)
            SetLevel1(true);
    }
}


// Desel pressed, reset.
void
RotState::desel()
{
    if (Level == 1)
        cEventHdlr::sel_esc();
    else {
        Gst()->SetGhost(GFnone);
        accum_final();
        GhostOn = false;
        DSP()->ShowCrossMark(ERASE, Refx, Refy, HighlightingColor,
            20, DSP()->CurMode());
        XM()->SetCoordMode(CO_ABSOLUTE);
    }
    GotOne = false;
    State = 0;
    SetLevel1(true);
}


// Esc entered, clean up and quit.
//
void
RotState::esc()
{
    if (Level == 1)
        cEventHdlr::sel_esc();
    else {
        Gst()->SetGhost(GFnone);
        accum_final();
        GhostOn = false;
        if (!GotOne) {
            Selections.deselectTypes(CurCell(), Types);
            Selections.removeTypes(CurCell(), Types);
        }
        else
            Selections.setShowSelected(CurCell(), Types, true);
        DSP()->ShowCrossMark(ERASE, Refx, Refy, HighlightingColor,
            20, DSP()->CurMode());
        XM()->SetCoordMode(CO_ABSOLUTE);
    }
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


bool
RotState::key(int code, const char *text, int)
{
    switch (code) {
    case SHIFTDN_KEY:
        if (GhostOn)
            Gst()->SetGhost(GFnone);
        Constrain = true;
        if (GhostOn)
            Gst()->SetGhostAt(GFrotate, Refx, Refy);
        return (true);
    case SHIFTUP_KEY:
        if (GhostOn)
            Gst()->SetGhost(GFnone);
        Constrain = false;
        if (GhostOn)
            Gst()->SetGhostAt(GFrotate, Refx, Refy);
        return (true);
    case RETURN_KEY:
        if (Level == 3) {
            Gst()->SetGhost(GFnone);
            accum_final();
            GhostOn = false;
            char *in = PL()->EditPrompt(
                "Enter angle, degrees counter-clockwise from x-axis: ", 0);
            if (!RotCmd)
                return (false);
            if (in && sscanf(in, "%lf", &RefAng) == 1)
                RefAng *= M_PI/180;
            else
                RefAng = 0;
            State = 3;
            SetLevel4();  // EditPrompt() swallows btn up event
            Gst()->SetGhostAt(GFrotate, Refx, Refy);
            accum_init();
            GhostOn = true;
        }
        else if (Level == 4) {
            Gst()->SetGhost(GFnone);
            accum_final();
            if (ED()->rotateQueue(Refx, Refy, RefAng, Refx, Refy, 0, 0,
                    Constrain ? CDcopy : CDmove)) {
                GhostOn = false;
                XM()->SetCoordMode(CO_ABSOLUTE);
                State = 4;
                DSP()->ShowCrossMark(ERASE, Refx, Refy, HighlightingColor,
                    20, DSP()->CurMode());
                Ulist()->CommitChanges(true);
                SetLevel1(true);
            }
            else {
                Gst()->SetGhostAt(GFrotate, Refx, Refy);
                accum_init();
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
    default:
        if (text && text[0] && !text[1]) {
            switch (text[0]) {
            case 'd':
            case 'D':
                if (!ShowDegrees) {
                    if (GhostOn)
                        Gst()->SetGhost(GFnone);
                    ShowDegrees = true;
                    if (GhostOn)
                        Gst()->SetGhostAt(GFrotate, Refx, Refy);
                }
                return (true);
            case 'r':
            case 'R':
                if (ShowDegrees) {
                    if (GhostOn)
                        Gst()->SetGhost(GFnone);
                    ShowDegrees = false;
                    if (GhostOn)
                        Gst()->SetGhostAt(GFrotate, Refx, Refy);
                }
                return (true);
            case ' ':
                if (GhostOn)
                    Gst()->SetGhost(GFnone);
                ShowDegrees = !ShowDegrees;;
                if (GhostOn)
                    Gst()->SetGhostAt(GFrotate, Refx, Refy);
                return (true);
            }
        }
    }
    return (false);
}


void
RotState::undo()
{
    if (Level == 1) {
        if (!cEventHdlr::sel_undo()) {
            // The undo system has placed the original objects into
            // the selection list, as selected (but only for move!).

            if (State == 4) {
                State = 5;
                DSP()->ShowCrossMark(DISPLAY, Refx, Refy, HighlightingColor,
                    20, DSP()->CurMode());
                Gst()->SetGhostAt(GFrotate, Refx, Refy);
                accum_init();
                GhostOn = true;
                SetLevel4();
                return;
            }
            dspPkgIf()->RegisterTimeoutProc(1500, MsgTimeout, 0);
        }
    }
    else if (Level == 2) {
        // Undo the selection.
        //
        CDol::destroy(OrigObjs);
        OrigObjs = Selections.listQueue(CurCell(), Types, false);
        Selections.setShowSelected(CurCell(), Types, false);
        Selections.removeTypes(CurCell(), Types);
        SetLevel1(true);
    }
    else if (Level == 3) {
        // Undo the rotate in progress.
        //
        Gst()->SetGhost(GFnone);
        accum_final();
        GhostOn = false;
        XM()->SetCoordMode(CO_ABSOLUTE);
        Selections.setShowSelected(CurCell(), Types, true);
        DSP()->ShowCrossMark(ERASE, Refx, Refy, HighlightingColor,
            20, DSP()->CurMode());
        SetLevel2(true);
    }
    else if (Level == 4)
        SetLevel3();
}


void
RotState::redo()
{
    if (Level == 1) {
        // Redo the last undone stretch, or last operation.
        //
        if (Ulist()->HasRedo() && (!Ulist()->HasOneRedo() || State < 4)) {
            cEventHdlr::sel_redo();
            dspPkgIf()->RegisterTimeoutProc(1500, MsgTimeout, 0);
        }
        else if (State == 1 || State == 2 || State == 3 || State == 5) {
            Selections.insertList(CurCell(), OrigObjs);
            Selections.setShowSelected(CurCell(), Types, true);
            SetLevel2(true);
        }
        else {
            cEventHdlr::sel_redo();
            dspPkgIf()->RegisterTimeoutProc(1500, MsgTimeout, 0);
        }
    }
    else if (Level == 2) {
        // Restart the last unfinished rotate.
        //
        if (State == 2 || State == 3 || State == 5) {
            Selections.setShowSelected(CurCell(), Types, false);
            DSP()->ShowCrossMark(DISPLAY, Refx, Refy, HighlightingColor,
                20, DSP()->CurMode());
            Gst()->SetGhostAt(GFrotate, Refx, Refy);
            accum_init();
            GhostOn = true;
            XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
            SetLevel3();
        }
    }
    else if (Level == 3) {
        if (State == 3 || State == 5)
            SetLevel4();
    }
    else if (Level == 4) {
        // Redo last undo.
        //
        if (State == 5) {
            State = 4;
            Gst()->SetGhost(GFnone);
            accum_final();
            GhostOn = false;
            XM()->SetCoordMode(CO_ABSOLUTE);
            DSP()->ShowCrossMark(ERASE, Refx, Refy, HighlightingColor,
                20, DSP()->CurMode());
            SetLevel1(true);
            Ulist()->RedoOperation();
        }
    }
}


double
RotState::get_ang(int x, int y, int cx, int cy)
{
    x -= cx;
    y -= cy;
    if (!x && !y)
        return (0.0);
    double ang = atan2((double)y, (double)x);
    if (ForceConstrain || (Constrain ^ Tech()->IsConstrain45())) {
        if (ang >= 0)
            ang += M_PI/8;
        else
            ang -= M_PI/8;
        int n = (int)(ang/(M_PI/4));
        ang = n*(M_PI/4);
    }
    return (ang);
}


// Timer callback, reprint the command prompt after an undo/redo, which
// will display a message.
//
int
RotState::MsgTimeout(void*)
{
    if (RotCmd)
        RotCmd->message();
    return (0);
}
// End of RotState functions.


// Rotate the objects in the selection queue.  If the queue contains
// instances or labels, the angle must be a multiple of pi/4.  In move
// mode, objects are removed from the queue, they are retained
// otherwise.
//
bool
cEdit::rotateQueue(int rx, int ry, double ang, int mx, int my,
    CDl *ldold, CDl *ldnew, CDmcType mc)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);
    bool do45 = false;
    sSelGen sg(Selections, cursd);
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (strchr(Types45, od->type())) {
            do45 = true;
            break;
        }
    }
    if (do45) {
        double deg = (180.0/M_PI)*ang;
        int ideg = mmRnd(deg);
        if (!(ideg % 45) && fabs(ideg - deg) < 0.01) {
            if (ideg < 0)
                ideg += 360;
            saveCurTransform(0);
            setCurTransform(ideg, false, false, 1.0);
            if (mc == CDmove)
                moveQueue(rx, ry, mx, my, ldold, ldnew);
            else
                copyQueue(rx, ry, mx, my, ldold, ldnew);
            recallCurTransform(0);
            return (true);
        }
        return (false);
    }

    Errs()->init_error();
    bool rotated = false;
    cTfmStack stk;
    stk.TPush();
    stk.TTranslate(mx - rx, my - ry);

    CDmergeInhibit *inh = 0;
    if (mc == CDmove) {
        CDol *st = Selections.listQueue(cursd);
        inh = new CDmergeInhibit(st);
        CDol::destroy(st);
    }
    sg = sSelGen(Selections, cursd, "bpw");
    while ((od = sg.next()) != 0) {
        if (rotate(&stk, od, cursd, rx, ry, ang, ldold, ldnew, mc)) {
            rotated = true;
            if (mc == CDmove)
                sg.remove();
        }
        else {
            Errs()->add_error("rotate failed");
            Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
        }
    }
    delete inh;
    stk.TPop();
    XM()->ShowParameters();
    return (rotated);
}


namespace {
    void
    rotate_path(Point *p, int n, int x, int y, double ang)
    {
        for (int i = 0; i < n; i++) {
            int px, py;
            px = mmRnd((p[i].x - x)*cos(ang) - (p[i].y - y)*sin(ang));
            py = mmRnd((p[i].x - x)*sin(ang) + (p[i].y - y)*cos(ang));
            p[i].x = px + x;
            p[i].y = py + y;
        }
    }
}


// Private function to rotate an object.
//
bool
cEdit::rotate(const cTfmStack *tstk, CDo *odesc, CDs *sdesc, int x, int y,
    double ang, CDl *ldold, CDl *ldnew, CDmcType mc)
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
        Point p[5];
        odesc->oBB().to_path(p);
        rotate_path(p, 5, x, y, ang);
        tstk->TPath(5, p);
        CDl *ld = odesc->ldesc();
        if (ldnew && (!ldold || ldold == odesc->ldesc()))
            ld = ldnew;
        Poly poly(5, p);
        if (poly.is_rect()) {
            BBox BB(poly.points);
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
            poly.points = Point::dup(p, 5);
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
        return (true);
    }
poly:
    {
        int num = ((const CDpo*)odesc)->numpts();
        Poly poly(num, Point::dup(((const CDpo*)odesc)->points(), num));
        rotate_path(poly.points, poly.numpts, x, y, ang);
        tstk->TPath(poly.numpts, poly.points);
        CDl *ld = odesc->ldesc();
        if (ldnew && (!ldold || ldold == odesc->ldesc()))
            ld = ldnew;
        if (poly.is_rect()) {
            BBox BB(poly.points);
            delete [] poly.points;
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
        int num = ((const CDw*)odesc)->numpts();
        Wire wire(num, Point::dup(((const CDw*)odesc)->points(), num),
            ((const CDw*)odesc)->attributes());
        rotate_path(wire.points, wire.numpts, x, y, ang);
        tstk->TPath(wire.numpts, wire.points);
        CDl *ld = odesc->ldesc();
        if (ldnew && (!ldold || ldold == odesc->ldesc()))
            ld = ldnew;
        CDw *newo = sdesc->newWire(mc == CDmove ? odesc : 0, &wire, ld,
            odesc->prpty_list(), false);
        if (!newo) {
            Errs()->add_error("newWire failed");
            return (false);
        }
        if (!sdesc->mergeWire(newo, true)) {
            Errs()->add_error("mergeWire failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        }
        return (true);
    }
label:
    Errs()->add_error("Can't rotate a label.");
    return (false);
inst:
    Errs()->add_error("Can't rotate an instance.");
    return (false);
}


//----------------
// Ghost Rendering

namespace {
    void
    display_ghost_rotate(CDo *odesc, int refx, int refy, double ang)
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
            Point p[5];
            odesc->oBB().to_path(p);
            rotate_path(p, 5, refx, refy, ang);
            Gst()->ShowGhostPath(p, 5);
            return;
        }
    poly:
        {
            int num = ((const CDpo*)odesc)->numpts();
            Poly poly(num, Point::dup(((const CDpo*)odesc)->points(), num));
            rotate_path(poly.points, poly.numpts, refx, refy, ang);
            Gst()->ShowGhostPath(poly.points, poly.numpts);
            delete [] poly.points;
            return;
        }
    wire:
        {
            int num = ((const CDw*)odesc)->numpts();
            Wire wire(num, Point::dup(((const CDw*)odesc)->points(), num),
                ((const CDw*)odesc)->attributes());
            rotate_path(wire.points, wire.numpts, refx, refy, ang);
            EGst()->showGhostWire(&wire);
            delete [] wire.points;
            return;
        }
    label:
    inst:
        {
            BBox BB;
            Point *pts;
            odesc->boundary(&BB, &pts);
            if (!pts) {
                pts = new Point[5];
                BB.to_path(pts);
            }
            rotate_path(pts, 5, refx, refy, ang);
            Gst()->ShowGhostPath(pts, 5);
            delete [] pts;
            return;
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


    // The objects list for ghosting during rotate.
    CDol *ghost_display_list;
}


// Function to show a ghost-drawn view of the geometry being rotated
// attached to the pointer.
//
void
cEditGhost::showGhostRotate(int map_x, int map_y, int ref_x, int ref_y,
    bool erase)
{
    if (!RotCmd)
        return;
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    double ang = 0.0;
    if (RotCmd->Level < 4) {
        int delta = 1 + (int)(DSP()->PixelDelta()/EV()->CurrentWin()->Ratio());
        bool atorg = (abs(map_x - ref_x) <= delta &&
            abs(map_y - ref_y) <= delta);
        // Near ref mark, clicking will allow text angle entry.

        if (!atorg) {
            ang = RotCmd->get_ang(map_x, map_y, ref_x, ref_y);
            for (CDol *ol = ghost_display_list; ol; ol = ol->next)
                display_ghost_rotate(ol->odesc, ref_x, ref_y, ang);
        }

        // Print the angle in lower-left window corner.
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CHD);
        while ((wdesc = wgen.next()) != 0) {
            if (Gst()->ShowingGhostInWindow(wdesc)) {
                char buf[128];
                if (atorg) {
                    strcpy(buf, "text entry");
                    int bxsz = (int)(10.0/wdesc->Ratio());
                    wdesc->Wdraw()->SetLinestyle(DSP()->BoxLinestyle());
                    wdesc->ShowLineBox(
                        RotCmd->Refx - bxsz, RotCmd->Refy - bxsz,
                        RotCmd->Refx + bxsz, RotCmd->Refy + bxsz);
                    wdesc->Wdraw()->SetLinestyle(0);
                }
                else if (ShowDegrees)
                    sprintf(buf, "%.5f", 180.0*ang/M_PI);
                else
                    sprintf(buf, "%.5f", ang);
                int x = 4;
                int y = wdesc->ViewportHeight() - 5;
                if (erase) {
                    int w = 0, h = 0;
                    wdesc->Wdraw()->TextExtent(buf, &w, &h);
                    BBox BB(x, y, x+w, y-h);
                    wdesc->GhostUpdate(&BB);
                }
                else
                    wdesc->Wdraw()->Text(buf, x, y, 0, 1, 1);
            }
        }
    }
    else {
        ang = RotCmd->RefAng;
        DSP()->TPush();
        DSP()->TTranslate(map_x - RotCmd->Refx, map_y - RotCmd->Refy);
        for (CDol *ol = ghost_display_list; ol; ol = ol->next)
            display_ghost_rotate(ol->odesc, RotCmd->Refx, RotCmd->Refy, ang);
        DSP()->TPop();
    }
}


// Static function.
// Initialize/free the object list for ghosting during rotate.
//
void
cEditGhost::ghost_rotate_setup(bool on)
{
    if (on) {
        CDol::destroy(ghost_display_list);  // should never need this
        ghost_display_list = Selections.listQueue(CurCell());
        unsigned int n = Selections.queueLength(CurCell());
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
            CDol::destroy(ghost_display_list);
            ghost_display_list = ol0;
        }
    }
    else {
        CDol::destroy(ghost_display_list);
        ghost_display_list = 0;
    }
}

