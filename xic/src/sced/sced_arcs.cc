
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
#include "sced.h"
#include "edit.h"
#include "undolist.h"
#include "dsp_tkif.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "events.h"
#include "layertab.h"
#include "menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "ghost.h"


#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

/**************************************************************************
 * Flash, donut, arc creation.
 **************************************************************************/

namespace { Point *make_arc_path(int, int, int, int, double, double, int*); }

// Constraint type for ellipse creation
enum RndHoldType {RndHoldNone, RndHoldX, RndHoldY};

namespace {
    namespace sced_arcs {
        struct ArcState : public CmdState
        {
            friend void cScedGhost::showGhostDiskPath(int, int, int, int);
            friend void cScedGhost::showGhostArcPath(int, int, int, int);

            ArcState(const char*, const char*);
            virtual ~ArcState();

            void setCaller(GRobject c)  { Caller = c; }

        private:
            static inline int compute_radius(int, int);
            static inline double arctan(int, int);

        public:
            void b1down();
            void b1up();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();
            bool makearc(int, int);
            void message() { if (Level == 1) PL()->ShowPrompt(msg1);
                else if (Level == 2) PL()->ShowPrompt(msg2);
                else if (Level == 3) PL()->ShowPrompt(msg3);
                else PL()->ShowPrompt(msg4); }

        private:
            void SetLevel1(bool show) { Level = 1; Mode = RndHoldNone;
                if (show) message(); }
            void SetLevel2() { Level = 2; message(); }
            void SetLevel3() { Level = 3; message(); }
            void SetLevel4() { Level = 4; message(); }

            GRobject Caller;
            int State;
            int Cx, Cy;
            int Rad1, Rad1x, Rad1y;
            double Ang1;
            bool ArcMode;
            bool AcceptKeys;
            bool Ctrl;
            RndHoldType Mode;

            static const char *msg1;
            static const char *msg2;
            static const char *msg3;
            static const char *msg4;
        };

        ArcState *ArcCmd;
    }
}

using namespace sced_arcs;

const char *ArcState::msg1 = "Point to center.";
const char *ArcState::msg2 = "Point to perimeter.";
const char *ArcState::msg3 = "Point to beginning of arc in a clockwise path.";
const char *ArcState::msg4 = "Point to end of arc in a clockwise path.";


// Menu command for arc creation.  If the shift key is held while the
// first perimeter point is defined, elliptical arc sections are
// generated.
//
void
cSced::makeArcPathExec(CmdDesc *cmd)
{
    if (ArcCmd) {
        ArcCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, true, Electrical))
        return;

    ArcCmd = new ArcState("ELECARC", "xic:shapes");
    ArcCmd->setCaller(cmd ? cmd->caller : 0);
    Ulist()->ListCheck(ArcCmd->Name(), CurCell(), false);
    if (!EV()->PushCallback(ArcCmd)) {
        delete ArcCmd;
        return;
    }
    ArcCmd->message();
    ds.clear();
}
// End of cSced functions


// Callback to display ghost flash as it is being created.
//
void
cScedGhost::showGhostDiskPath(int x, int y, int cen_x, int cen_y)
{
    int rad = ArcState::compute_radius(x - cen_x, y - cen_y);
    int rx, ry;
    if (ArcCmd->Mode == RndHoldX) {
        rx = ArcCmd->Rad1x;
        ry = rad;
    }
    else if (ArcCmd->Mode == RndHoldY) {
        rx = rad;
        ry = ArcCmd->Rad1y;
    }
    else
        rx = ry = rad;
    int numpts;
    Point *points = make_arc_path(cen_x, cen_y, rx, ry, 0.0, 0.0, &numpts);
    Gst()->ShowGhostPath(points, numpts);
    delete [] points;
}


// Callback to display ghost arc as it is being created.
//
void
cScedGhost::showGhostArcPath(int x, int y, int cen_x, int cen_y)
{
    double ang = ArcState::arctan(y - cen_y, x - cen_x);
    double ang1, ang2;
    if (!ArcCmd->ArcMode) {
        ang1 = ang;
        ang2 = ang1 - 2*M_PI + .1;
    }
    else {
        ang1 = ArcCmd->Ang1;
        ang2 = ang;
        if (ang2 == ang1)
            ang2 += M_PI/90;  // for visibility
    }
    int numpts;
    Point *points = make_arc_path(cen_x, cen_y, ArcCmd->Rad1x, ArcCmd->Rad1y,
        ang1, ang2, &numpts);
    Gst()->ShowGhostPath(points, numpts);
    delete [] points;
}
// End of cScedGhost functions


ArcState::ArcState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    State = 0;
    Cx = Cy = 0;
    Rad1 = Rad1x = Rad1y = 0;
    Ang1 = 0.0;
    ArcMode = AcceptKeys = false;
    Ctrl = false;
    SetLevel1(false);
}


ArcState::~ArcState()
{
    ArcCmd = 0;
}


// Static function.
inline int
ArcState::compute_radius(int x, int y)
{
    double x2 = x*(double)x;
    double y2 = y*(double)y;
    if (ArcCmd && ArcCmd->Ctrl) {
        if (x2 < y2)
            x2 = 0.0;
        else
            y2 = 0.0;
    }
    return ((int)sqrt(x2 + y2));
}


// Static function.
// Atan2 with check for domain.
//
inline double
ArcState::arctan(int y, int x)
{
    if (x == 0 && y == 0)
        return (0.0);
    double ang = atan2((double)y, (double)x);
    if (ArcCmd && ArcCmd->Ctrl) {
        if (ang > 0.0)
            ang = (M_PI/4)*(int)((ang + M_PI/8)/(M_PI/4));
        else
            ang = (M_PI/4)*(int)((ang - M_PI/8)/(M_PI/4));
    }
    return (ang);
}


void
ArcState::b1down()
{
    if (Level == 1) {
        // Define the center point.
        //
        EV()->Cursor().get_xy(&Cx, &Cy);
        DSP()->ShowCrossMark(DISPLAY, Cx, Cy, HighlightingColor,
            20, DSP()->CurMode());
        XM()->SetCoordMode(CO_RELATIVE, Cx, Cy);
        Gst()->SetGhostAt(GFdiskpth, Cx, Cy);
        // start looking for shift press
        AcceptKeys = true;
        State = 1;
        EV()->DownTimer(GFdiskpth);
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
            Rad1 = rad;
            Gst()->SetGhost(GFnone);
            ArcMode = false;
            Gst()->SetGhostAt(GFarcpth, Cx, Cy);
            // stop looking for shift press
            AcceptKeys = false;
            State = 2;
            EV()->DownTimer(GFarcpth);
        }
    }
    else if (Level == 3) {
        // Set the arc anchor point.
        //
        Gst()->SetGhost(GFnone);
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        Ang1 = arctan(y - Cy, x - Cx);
        ArcMode = true;
        State = 4;
        Gst()->SetGhostAt(GFarcpth, Cx, Cy);
        EV()->DownTimer(GFarcpth);
    }
    else if (Level == 4) {
        // Create arc.
        //
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        if (makearc(x, y))
            State = 5;
    }
}


// Define the radius, if possible.  Otherwise
// set state to define radius on next button 1 press.
//
void
ArcState::b1up()
{
    if (Level == 1) {
        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok()) {
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
                Rad1 = rad;
                Gst()->SetGhost(GFnone);
                SetLevel3();
                ArcMode = false;
                Gst()->SetGhostAt(GFarcpth, Cx, Cy);
                State = 2;
                return;
            }
        }
        SetLevel2();
    }
    else if (Level == 2) {
        // If the radius was successfully defined, define the anchor point if
        // possible.
        //
        if (State == 2) {
            if (!EV()->UpTimer() && EV()->Cursor().is_release_ok()) {
                int x, y;
                EV()->Cursor().get_release(&x, &y);
                EV()->CurrentWin()->Snap(&x, &y);
                int xc, yc;
                EV()->Cursor().get_xy(&xc, &yc);
                if (x != xc || y != yc) {
                    Ang1 = arctan(y - Cy, x - Cx);
                    ArcMode = true;
                    State = 4;
                    Gst()->SetGhost(GFnone);
                    SetLevel4();
                    Gst()->SetGhostAt(GFarcpth, Cx, Cy);
                    return;
                }
            }
            SetLevel3();
        }
    }
    else if (Level == 3) {
        // Finish the arc, if possible, if the anchor point was set.
        // If not created, wait for next button 1 press.
        //
        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            int xc, yc;
            EV()->Cursor().get_xy(&xc, &yc);
            if (x != xc || y != yc) {
                if (makearc(x, y)) {
                    State = 4;
                    SetLevel1(true);
                    return;
                }
            }
        }
        SetLevel4();
    }
    else if (Level == 4) {
        // If arc was created, reset state to start next arc.
        //
        if (State == 5) {
            State = 4;
            SetLevel1(true);
        }
    }
}


// Esc entered, clean up and exit.
//
void
ArcState::esc()
{
    DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor, 20,
        DSP()->CurMode());
    Gst()->SetGhost(GFnone);
    XM()->SetCoordMode(CO_ABSOLUTE);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    if (Caller)
        Menu()->Deselect(Caller);
    delete this;
}


// Look for shift keypress for ellipse mode.
//
bool
ArcState::key(int code, const char*, int)
{
    int x, y;
    switch (code) {
    case SHIFTDN_KEY:
        if (!AcceptKeys)
            return (false);
        DSPmainDraw(ShowGhost(ERASE))
        if (XM()->WhereisPointer(&x, &y)) {
            EV()->CurrentWin()->Snap(&x, &y);
            int rad = compute_radius(x - Cx, y - Cy);
            int rx = x - Cx;
            if (rx < 0)
                rx = -rx;
            int ry = y - Cy;
            if (ry < 0)
                ry = -ry;
            if (rx > ry) {
                Rad1y = rad;
                Mode = RndHoldY;
            }
            else {
                Rad1x = rad;
                Mode = RndHoldX;
            }
        }
        DSPmainDraw(ShowGhost(DISPLAY))
        break;
    case SHIFTUP_KEY:
        if (!AcceptKeys)
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
    }
    return (false);
}


void
ArcState::undo()
{
    if (Level == 1) {
        // Undo the creation just done, or the last operation.
        //
        Ulist()->UndoOperation();
        if (State == 4) {
            DSP()->ShowCrossMark(DISPLAY, Cx, Cy, HighlightingColor, 20,
                DSP()->CurMode());
            ArcMode = true;
            XM()->SetCoordMode(CO_RELATIVE, Cx, Cy);
            Gst()->SetGhostAt(GFarcpth, Cx, Cy);
            SetLevel4();
            return;
        }
    }
    else if (Level == 2) {
        // Undo the center definition, start over.
        //
        if (State == 4)
            State = 5;
        DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor, 20,
            DSP()->CurMode());
        Gst()->SetGhost(GFnone);
        XM()->SetCoordMode(CO_ABSOLUTE);
        AcceptKeys = false;
        SetLevel1(true);
    }
    else if (Level == 3) {
        // Undo radius definition.
        //
        Gst()->SetGhost(GFnone);
        SetLevel2();
        AcceptKeys = true;
        Gst()->SetGhostAt(GFdiskpth, Cx, Cy);
    }
    else if (Level == 4) {
        // Undo anchor point.
        //
        Gst()->SetGhost(GFnone);
        SetLevel3();
        ArcMode = false;
        Gst()->SetGhostAt(GFarcpth, Cx, Cy);
    }
}


void
ArcState::redo()
{
    if (Level == 1) {
        // Redo the last undo, or operation.
        //
        if ((State == 1 && !Ulist()->HasRedo()) ||
                (State == 5 && Ulist()->HasOneRedo())) {
            if (State == 5)
                State = 4;
            DSP()->ShowCrossMark(DISPLAY, Cx, Cy, HighlightingColor,
                20, DSP()->CurMode());
            SetLevel2();
            AcceptKeys = true;
            XM()->SetCoordMode(CO_RELATIVE, Cx, Cy);
            Gst()->SetGhostAt(GFdiskpth, Cx, Cy);
            return;
        }
        Ulist()->RedoOperation();
    }
    else if (Level == 2) {
        // Redo first periphery definition, or operation.
        //
        if (State >= 2) {
            Gst()->SetGhost(GFnone);
            AcceptKeys = false;
            SetLevel3();
            ArcMode = false;
            Gst()->SetGhostAt(GFarcpth, Cx, Cy);
        }
    }
    else if (Level == 3) {
        // Redo anchor point definition, or undone operation.
        //
        if (State >= 3) {
            Gst()->SetGhost(GFnone);
            SetLevel4();
            ArcMode = true;
            Gst()->SetGhostAt(GFarcpth, Cx, Cy);
        }
    }
    else if (Level == 4) {
        // Redo last undo.
        //
        if (Caller && State == 4) {
            Gst()->SetGhost(GFnone);
            XM()->SetCoordMode(CO_ABSOLUTE);
            DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor, 20,
                DSP()->CurMode());
            Ulist()->RedoOperation();
            SetLevel1(true);
            return;
        }
    }
}


// Create the arc.  Returns true on success.
//
bool
ArcState::makearc(int x, int y)
{
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return (false);

    CDs *cursd = CurCell();
    if (!cursd)
        return (false);

    double ang2 = arctan(y - Cy, x - Cx);
    Wire wire;
    wire.points = make_arc_path(Cx, Cy, Rad1x, Rad1y, Ang1, ang2,
        &wire.numpts);
    wire.set_wire_width(0);
    Errs()->init_error();
    CDw *newo = cursd->newWire(0, &wire, ld, 0, false);
    if (!newo) {
        Errs()->add_error("newWire failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        return (false);
    }
    if (!cursd->mergeWire(newo, true)) {
        Errs()->add_error("mergeWire failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
    }
    Gst()->SetGhost(GFnone);
    XM()->SetCoordMode(CO_ABSOLUTE);
    DSP()->ShowCrossMark(ERASE, Cx, Cy, HighlightingColor, 20,
        DSP()->CurMode());
    Ulist()->CommitChanges(true);
    return (true);
}
// End of ArcState methods


namespace {
    // Create the arc path.
    //
    Point *
    make_arc_path(int cen_x, int cen_y, int rad1x, int rad1y, double ang1,
        double ang2, int *numpts)
    {
        if (ang1 <= ang2)
            ang1 += 2*M_PI;

        int sides =
            (int)(((ang1 - ang2)/(2*M_PI))*GEO()->roundFlashSides(true));
        if (sides < 3)
            sides = 3;

        double DPhi = (ang1 - ang2)/sides;
        *numpts = sides + 1;
        Point *points = new Point[*numpts];

        double A1 = ang2;
        for (int i = 0; i <= sides; i++) {
            points[i].set(mmRnd(cen_x + rad1x*cos(A1)),
                mmRnd(cen_y + rad1y*sin(A1)));
            A1 += DPhi;
        }
        return (points);
    }
}

