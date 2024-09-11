
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

#include "grconfig.h"
#include "main.h"
#include "editif.h"
#include "drcif.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "events.h"
#include "menu.h"
#include "promptline.h"
#include "select.h"
#include "errorlog.h"
#include "ghost.h"


void CmdState::undo()       { EditIf()->ulUndoOperation(); }
void CmdState::redo()       { EditIf()->ulRedoOperation(); }
bool CmdState::Abort;
bool CmdState::EnableExport;
bool CmdState::Exported;
int CmdState::ExportX;
int CmdState::ExportY;
// End of CmdState functions.


Deselector::Deselector(CmdDesc *cmd)
{
    d_cmd = cmd;
}


Deselector::~Deselector()
{
    if (d_cmd && d_cmd->caller)
        MainMenu()->Deselect(d_cmd->caller);
}
// End of Deselector functions.


namespace {
    // This is a convenience object that can be instantianted in
    // functions where we need to suppress dispatching events in
    // cGrPkg::CheckForInterrupt.
    //
    struct EventDispatchSuppress
    {
        EventDispatchSuppress()
            {
                dispatch_events =
                    DSPpkg::self()->CheckForInterruptDispatch(false);
            }

        ~EventDispatchSuppress()
            {
                DSPpkg::self()->CheckForInterruptDispatch(dispatch_events);
            }

    private:
        bool dispatch_events;
    };
}


//-----------------------------------------------------------------------------
// The following sets up the default state, outside of commands.  A
// click over no selected objects or with neither the Shift or Control
// keys pressed performs a point selection operation.  A drag that
// begins immediately with neither the Shift or Control keys pressed
// initiates an area select operation.

namespace {
    namespace main_events {
        struct EVmainState : public CmdState
        {
            EVmainState(const char *nm, const char *hk) : CmdState(nm, hk) { }

            // The MainStateReady flag enables deselection when Esc
            // is pressed twice.
            void b1down()   { cEventHdlr::sel_b1down();
                              EV()->SetMainStateReady(false); }
            void b1up()     { cEventHdlr::sel_b1up(&AOI, 0, 0); }
            void desel()    { cEventHdlr::sel_esc(); }
            void esc()      { cEventHdlr::sel_esc();
                              EV()->SetMainStateReady(true); }
            bool key(int, const char*, int) { return (false); }
            void undo()     { cEventHdlr::sel_undo(); }
            void redo()     { cEventHdlr::sel_redo(); }

        private:
            BBox AOI;
        };

        EVmainState ev_mainstate(0, 0);
    }
}

using namespace main_events;


cEventHdlr *cEventHdlr::instancePtr = 0;
cEventHdlr::selstate_t cEventHdlr::selstate;

cEventHdlr::cEventHdlr()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cEventHdlr already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    for (int i = 0; i < CallStackDepth; i++)
        ev_callbacks[i] = 0;
    ev_current_win = 0;
    ev_keypress_win = 0;
    ev_zoom_win = 0;
    ev_constrained = false;
    ev_main_cmd = &ev_mainstate;
    ev_motion_task = 0;
    ev_in_coord_entry = false;
    ev_main_state_ready = false;
    ev_constrained = false;
    ev_shift_down = false;
    ev_ctrl_down = false;
    ev_alt_down = false;
    ev_from_prline = false;

    PushCallback(ev_main_cmd);  // never popped
}


// Private static error exit.
//
void
cEventHdlr::on_null_ptr()
{
    fprintf(stderr, "Singleton class cEventHdlr used before instantiated.\n");
    exit(1);
}


//-----------------------------------------------------------------------------
// Pan operation
// If the user clicks, the point will appear centered in the window.
// If the user drags in the same window, the button down point will be
// translated to the button up location.  If the user drags to a
// different window, the second window will be updated, with the
// button down location translated to the button up location.
//
namespace {
    namespace main_events {
        struct PanState
        {
            PanState(WindowDesc*, int, int);
            ~PanState();

            int RefX, RefY;
            WindowDesc *InitWdesc;
        };

        PanState *PanCmd;
    }
}


PanState::PanState(WindowDesc *w, int x, int y)
{
    InitWdesc = w;
    RefX = x;
    RefY = y;
}


PanState::~PanState()
{
    PanCmd = 0;
}


//-----------------------------------------------------------------------------
// Zoom operation
// There are three modes of operation:
//   (1) If button 3 is dragged and then pressed, the new window is
//   centered on the drag region, with a width given by the ratio of
//   the diagonals (p - drag_start)/(drag_end - drag_start).  This
//   allows the user to zoom out.
//   (2) If button 3 is dragged and then pressed in a different window
//   than the initial press, the second window will display the dragged
//   area.  This allows the user to assign a new view to a subwindow.
//   (3) If button 3 is clicked twice, the new window is the
//   inscribed rectangle.
//
namespace {
    namespace main_events {
        struct ZoomState : public CmdState
        {
            ZoomState(const char*, const char*);
            virtual ~ZoomState();

            void esc();
            void action(WindowDesc*, int, int);

            unsigned long InitId;
            bool DidMark;
            bool GhostSet;
            int Refx, Refy;
            int Mx, My;
            int Trefx, Trefy;

            static GRlineType Linestyle;
            static bool LsSet;
        };

        ZoomState *ZoomCmd;
    }
}

GRlineType ZoomState::Linestyle;
bool ZoomState::LsSet;


ZoomState::ZoomState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    DidMark = false;
    InitId = 0;
    GhostSet = false;
    Refx = Refy = 0;
    Mx = My = 0;

    // Save the current reference point, this will be restored
    // after the zoom.
    EV()->GetReference(&Trefx, &Trefy);
}


ZoomState::~ZoomState()
{
    ZoomCmd = 0;
    EV()->SetReference(Trefx, Trefy);
}


// Esc entered, abort.
//
void
ZoomState::esc()
{
    if (GhostSet) {
        Gst()->SetGhost(GFnone);
        EV()->FinishZoom();
        GhostSet = false;
    }
    EV()->PopCallback(this);
    delete this;
}


void
ZoomState::action(WindowDesc *wdesc, int x, int y)
{
    int width, height, tmp;
    WindowDesc *initdesc = DSP()->Windesc(InitId);
    if (!initdesc) {
        initdesc = wdesc;
        ZoomCmd->InitId = wdesc->WindowId();
    }
    if (!wdesc->IsSimilar(initdesc))
        return;
    Gst()->SetGhost(GFnone);
    EV()->FinishZoom();
    GhostSet = false;
    wdesc->PToL(x, y, x, y);
    double newwidth;
    if (DidMark) {
        // intervening release was recorded
        DidMark = false;
        if (wdesc != initdesc) {
            // set up params for new window
            width = Mx - Refx;
            if (width < 0)
                width = -width;
            height = My - Refy;
            if (height < 0)
                height = -height;
            if (Mx < Refx)
                Refx = Mx;
            if (My < Refy)
                Refy = My;
            tmp = (int)(height*wdesc->Aspect());
            newwidth = (width > tmp ? width : tmp);
            if (newwidth < 100)
                newwidth = 100;
            Refx += width/2;
            Refy += height/2;
        }
        else {
            // scale the view in the present window
            double mult = 1.0;
            double A = ((double)(Mx - Refx))*(Mx - Refx) +
                ((double)(My - Refy))*(My - Refy);
            double B = ((double)(x - Refx))*(x - Refx) +
                ((double)(y - Refy))*(y - Refy);
            if (B != 0.0) {
                mult = sqrt(A/B);
                if (mult > 50.0)
                    mult = 50.0;
            }
            Refx += (Mx - Refx)/2;
            Refy += (My - Refy)/2;
            newwidth = mult*wdesc->Window()->width();
            if (newwidth < wdesc->MinWidth())
                newwidth = wdesc->MinWidth();
        }
    }
    else {
        // zoomin to defined area, window of first button press
        initdesc = DSP()->Windesc(InitId);
        if (!initdesc) {
            initdesc = wdesc;
            InitId = wdesc->WindowId();
        }
        else
            wdesc = initdesc;
        width = x - Refx;
        if (width < 0)
            width = -width;
        height = y - Refy;
        if (height < 0)
            height = -height;
        if (x < Refx)
            Refx = x;
        if (y < Refy)
            Refy = y;
        tmp = (int)(height*wdesc->Aspect());
        newwidth = (width > tmp ? width : tmp);
        if (newwidth < wdesc->MinWidth())
            newwidth = wdesc->MinWidth();
        Refx += width/2;
        Refy += height/2;

    }
    wdesc->InitWindow(Refx, Refy, newwidth);
    esc();
    wdesc->Redisplay(0);
}


//-----------------------------------------------------------------------------
// A timer for general button press handling.  DownTimer() is called
// on button press, UpTimer() on release.  If UpTimer() returns true,
// treat as click, otherwise as drag.

namespace {
    int downtimeout(void*);
    bool btn_down;
}

void
cEventHdlr::DownTimer(int which)
{
    DSPpkg::self()->AddTimer(250, downtimeout, (void*)(uintptr_t)which);
    btn_down = true;
}


bool
cEventHdlr::UpTimer()
{
    bool i = btn_down;
    btn_down = false;
    return (i);
}


namespace {
    int
    downtimeout(void *client_data)
    {
        if (btn_down) {
            Gst()->BumpGhostPointer((uintptr_t)client_data);
            btn_down = false;
        }
        return (false);
    }
}


//------------------------------------------------------------------------------
// State control, manage the state stack.

// Initialize the main state, called from the application initialization
// code.
//
void
cEventHdlr::RegisterMainState(CmdState *cmd)
{
    if (cmd != ev_main_cmd) {
        for (int i = 0; i < CallStackDepth; i++) {
            if (ev_callbacks[i] == ev_main_cmd) {
                ev_callbacks[i] = cmd;
                break;
            }
        }
        ev_main_cmd = cmd;
    }
}


//
// Command stack
//

// Push the new command on the call stack.
//
bool
cEventHdlr::PushCallback(CmdState *cb)
{
    if (!cb) {
        // just reset main state
        if (ev_main_cmd && ev_callbacks[0] == ev_main_cmd &&
                !ev_in_coord_entry) {
            // Want to reset main state, except when entering coordinate
            // from the keyboard (^E).
            ev_main_cmd->esc();
        }
        return (true);
    }

    if (ev_main_cmd && ev_callbacks[CallStackDepth - 1] == ev_main_cmd) {
        Log()->ErrorLog("command processing",
            "Command stack maximum depth reached.");
        return (false);
    }

    for (int i = CallStackDepth - 1; i > 0; i--)
        ev_callbacks[i] = ev_callbacks[i-1];
    ev_callbacks[0] = cb;
    if (ev_main_cmd && ev_callbacks[1] == ev_main_cmd && !ev_in_coord_entry) {
        // Want to reset main state, except when entering coordinate
        // from the keyboard (^E).
        ev_main_cmd->esc();
    }

    XM()->ShowParameters();
    return (true);
}


// Pop the call stack, revert to previous command.
//
void
cEventHdlr::PopCallback(CmdState *thisone)
{
    bool found = false;
    for (int i = 0; i < CallStackDepth; i++) {
        if (ev_callbacks[i] == thisone) {
            found = true;
            break;
        }
    }
    if (!found)
        return;

    // Exit any commands above thisone.
    while (ev_callbacks[0] != thisone)
        ev_callbacks[0]->esc();  // recursively calls PopCallback

    for (int i = 0; i < CallStackDepth - 1; i++)
        ev_callbacks[i] = ev_callbacks[i+1];
    ev_callbacks[CallStackDepth - 1] = 0;
    XM()->ShowParameters();
}


// Pop callback stack to top, escaping each command.
//
void
cEventHdlr::InitCallback()
{
    if (!ev_callbacks[0])
        return;
    CmdState::SetAbort(true);
    // The Abort flag can be referenced in the esc() method to tell if
    // the esc is caused by a reset (here) or a simple Esc keypress.
    for (;;) {
        if (ev_main_cmd && ev_callbacks[0] == ev_main_cmd) {
            // At top, call esc() and return.
            ev_callbacks[0]->esc();
            break;
        }
        CmdState *c = ev_callbacks[0];
        if (!c)
            break;
        // If state doesn't change when esc() is called, return.  This
        // shouldn't happen.
        if (ev_callbacks[0])
            ev_callbacks[0]->esc();
        if (ev_callbacks[0] == c)
            break;
    }
    CmdState::SetAbort(false);
    XM()->ShowParameters();
    // Do any queued redisplays now.
    DSPpkg::self()->CheckForInterrupt();
}


// Return true if there is a command pushed.
//
bool
cEventHdlr::IsCallback()
{
    if (ev_callbacks[0] == ev_main_cmd)
        return (false);
    return (true);
}


// ----------------------------------------------------------------------------
// Event callbacks.  All viewport events are dispatched to the
// application through these functions.

// Set a new window from the new viewport width, height.
//
void
cEventHdlr::ResizeCallback(WindowDesc *wdesc, int width, int height)
{
    wdesc->InitViewport(width, height);
    if (wdesc->Window()->width() <= 0)
        return;

    if (wdesc->Ratio() == 0.0)
        wdesc->SetRatio(1.0);
    wdesc->InitWindow((wdesc->Window()->left + wdesc->Window()->right)/2,
        (wdesc->Window()->top + wdesc->Window()->bottom)/2,
        wdesc->ViewportWidth()/wdesc->Ratio());
}


// Handle keypress events.  Dispatch the escape and key callbacks,
// as appropriate.
//
bool
cEventHdlr::KeypressCallback(WindowDesc *wdesc, int code, char *text,
    int mstate)
{
    if (code == SHIFTDN_KEY)
        return (ProcessModState(mstate | GR_SHIFT_MASK));
    if (code == SHIFTUP_KEY)
        return (ProcessModState(mstate & ~GR_SHIFT_MASK));
    if (code == CTRLDN_KEY)
        return (ProcessModState(mstate | GR_CONTROL_MASK));
    if (code == CTRLUP_KEY)
        return (ProcessModState(mstate & ~GR_CONTROL_MASK));

    if (ev_callbacks[0])
        if (ev_callbacks[0]->key(code, text, mstate))
            return (true);

    if (!wdesc)
        // shouldn't happen
        return (true);

    return (false);
}


namespace {
    // Parse two floating-point m.eee numbers, separated by white space
    // and/or a comma.  The returned int is the number of conversions,
    // i.e., if 1, only the x return is valid.
    //
    int
    parse_xy(const char *s, double *x, double *y)
    {
        int n;
        if (sscanf(s, "%lf%n", x, &n) != 1)
            return (0);
        s += n;
        while (isspace(*s) || *s == ',')
            s++;
        if (sscanf(s, "%lf%n", y, &n) == 1)
            return (2);
        return (1);
    }
}


// Action handler for keyboard input.  Return true if no further
// processing should be done.
//
bool
cEventHdlr::KeyActions(WindowDesc *wdesc, eKeyAction action, int *code)
{
    // save press window
    ev_keypress_win = wdesc;

    char *in;
    switch (action) {
    case No_action:
        return (true);
    case Iconify_action:
        DSPpkg::self()->Iconify(true);
        return (true);
    case Interrupt_action:
        DSP()->SetInterrupt(DSPinterUser);
        return (true);
    case Escape_action:
        if (!IsCallback() && ev_main_state_ready)
            XM()->DeselectExec(0);
        else if (ev_callbacks[0])
            ev_callbacks[0]->esc();
        PL()->SetKeys(wdesc, 0);
        PL()->ShowKeys(wdesc);
        return (true);
    case Redisplay_action:
        wdesc->Redisplay(0);
        return (true);
    case Delete_action:
        *code = DELETE_KEY;
        return (false);
    case Bsp_action:
        PL()->BspKeys(wdesc);
        PL()->ShowKeys(wdesc);
        return (true);
    case CodeUndo_action:
        *code = UNDO_KEY;
        return (false);
    case CodeRedo_action:
        *code = REDO_KEY;
        return (false);
    case Undo_action:
        PL()->SetKeys(wdesc, "undo");
        PL()->CheckExec(wdesc, true);
        return (true);
    case Redo_action:
        PL()->SetKeys(wdesc, "redo");
        PL()->CheckExec(wdesc, true);
        return (true);
    case Expand_action:
        PL()->SetKeys(wdesc, "expnd");
        PL()->CheckExec(wdesc, true);
        return (true);
    case Grid_action:
        // This is handled by the menu ctrl-g accelerator, except in
        // QT in Apple, where the accelerator would appear in the main
        // menu only, leaving out subwindow support.  In that case
        // handle ctrl-g here, and leave out the ctrl-g menu accelerator.
#ifdef __APPLE__
#if defined(WITH_QT5) || defined(WITH_QT6)
        PL()->SetKeys(wdesc, "grid");
        PL()->CheckExec(wdesc, true);
#endif
#endif
        return (true);
    case ClearKeys_action:
        PL()->SetKeys(wdesc, 0);
        PL()->ShowKeys(wdesc);
        return (true);

    case DRCb_action:
    case DRCf_action:
    case DRCp_action:
        return (DrcIf()->actionHandler(wdesc, action));

    case SetNextView_action:
        wdesc->SetView("next");
        return (true);
    case SetPrevView_action:
        wdesc->SetView("prev");
        return (true);
    case FullView_action:
        wdesc->SetView("full");
        return (true);
    case DecRot_action:
        EditIf()->incrementRotation(false);
        return (true);
    case IncRot_action:
        EditIf()->incrementRotation(true);
        return (true);
    case FlipY_action:
        EditIf()->flipY();
        return (true);
    case FlipX_action:
        EditIf()->flipX();
        return (true);
    case PanLeft_action:
        wdesc->Pan(DirWest, 0.5);
        return (true);
    case PanDown_action:
        wdesc->Pan(DirSouth, 0.5);
        return (true);
    case PanRight_action:
        wdesc->Pan(DirEast, 0.5);
        return (true);
    case PanUp_action:
        wdesc->Pan(DirNorth, 0.5);
        return (true);
    case ZoomIn_action:
        wdesc->Zoom(0.5);
        return (true);
    case ZoomOut_action:
        wdesc->Zoom(2.0);
        return (true);
    case ZoomInFine_action:
        wdesc->Zoom(0.9);
        return (true);
    case ZoomOutFine_action:
        wdesc->Zoom(1.0/0.9);
        return (true);
    case Command_action:
        in = PL()->EditPrompt("! ", 0);
        if (in != 0)
            XM()->TextCmd(in, false);
        else
            PL()->ErasePrompt();
        return (true);
    case Help_action:
        in = PL()->EditPrompt("? ", 0);
        if (in != 0)
            XM()->TextCmd(in, true);
        else
            PL()->ErasePrompt();
        return (true);
    case Coord_action:
        if (ev_callbacks[0]) {
            // unset ctrl modifier action from Ctrl-E
            char c = 0;
            ev_callbacks[0]->key(CTRLUP_KEY, &c, 0);
        }
        ev_in_coord_entry = true;
        in = PL()->EditPrompt(
            "Enter x y or + x y (optional '+' if relative to last point): ",
            0);
        in = lstring::strip_space(in);
        ev_in_coord_entry = false;
        if (in != 0) {
            PL()->ErasePrompt();
            while (isspace(*in))
                in++;
            bool rel = false;
            if (*in == '+' || *in == 'r') {
                rel = true;
                in++;
            }
            double xd, yd;
            int i = parse_xy(in, &xd, &yd);
            if (i > 0) {
                int xr, yr;
                GetReference(&xr, &yr);
                int x = INTERNAL_UNITS(xd);
                if (rel)
                    x += xr;
                int y = yr;
                if (i == 2) {
                    y = INTERNAL_UNITS(yd);
                    if (rel)
                        y += yr;
                }
                ev_cursor_desc.phony_press(wdesc, x, y);
            }
        }
        return (true);
    case SaveView_action:
        wdesc->SaveViewOnStack();
        return (true);
    case NameView_action:
        if (*code >= 'a' && *code <= 'z') {
            char buf[4];
            buf[0] = 'A' + (*code - 'a');
            buf[1] = 0;
            if (wdesc->SetView(buf))
                return (true);
        }
        if (*code >= 'A' && *code <= 'Z') {
            char buf[4];
            buf[0] = 'A' + (*code - 'A');
            buf[1] = 0;
            if (wdesc->SetView(buf))
                return (true);
        }
        return (false);
    case Version_action:
        XM()->LegalMsg();
        return (true);
    case PanLeftFine_action:
        wdesc->Pan(DirWest, 0.1);
        return (true);
    case PanDownFine_action:
        wdesc->Pan(DirSouth, 0.1);
        return (true);
    case PanRightFine_action:
        wdesc->Pan(DirEast, 0.1);
        return (true);
    case PanUpFine_action:
        wdesc->Pan(DirNorth, 0.1);
        return (true);
    case IncExpand_action:
        if (wdesc->Attrib()->expand_level(wdesc->Mode()) >= 0) {
            wdesc->Expand("+");
            wdesc->Redisplay(0);
        }
        return (true);
    case DecExpand_action:
        if (wdesc->Attrib()->expand_level(wdesc->Mode()) < 0) {
            wdesc->Expand("0");
            wdesc->Redisplay(0);
        }
        else if (wdesc->Attrib()->expand_level(wdesc->Mode()) > 0) {
            wdesc->Expand("-");
            wdesc->Redisplay(0);
        }
        return (true);
    default:
        break;
    }
    return (false);
}


void
cEventHdlr::PanPress(WindowDesc *wdesc, int x, int y, int)
{
    delete PanCmd;
    if (wdesc) {
        wdesc->PToL(x, y, x, y);
        PanCmd = new PanState(wdesc, x, y);
    }
}


void
cEventHdlr::PanRelease(WindowDesc *wdesc, int x, int y, int)
{
    if (PanCmd) {
        if (!wdesc || !wdesc->IsSimilar(PanCmd->InitWdesc)) {
            delete PanCmd;
            return;
        }
        int xp = x;
        int yp = y;
        wdesc->PToL(x, y, x, y);
        bool trns = (PanCmd->InitWdesc != wdesc);
        if (!trns) {
            // In same window, set trns if drag.
            int xlp, ylp;
            wdesc->LToP(PanCmd->RefX, PanCmd->RefY, xlp, ylp);
            if (abs(xp - xlp) > 2 || abs(yp - ylp) > 2)
                trns = true;
        }
        if (trns) {
            // Translate down point to up point.
            x = (wdesc->Window()->right + wdesc->Window()->left)/2 +
                PanCmd->RefX - x;
            y = (wdesc->Window()->bottom + wdesc->Window()->top)/2 +
                PanCmd->RefY - y;
        }
        wdesc->Center(x, y);
        delete PanCmd;
    }
}


void
cEventHdlr::ZoomPress(WindowDesc *wdesc, int x, int y, int state)
{
    if (!wdesc)
        // shouldn't happen
        return;
    if (!ZoomState::LsSet) {
        // set a linestyle for the zoom box
        wdesc->Wdraw()->defineLinestyle(&ZoomState::Linestyle,
            DEF_BoxLineStyle);
        wdesc->Wdraw()->SetLinestyle(0);
        ZoomState::LsSet = true;
    }
    if (!ZoomCmd) {
        ZoomCmd = new ZoomState(0, 0);
        // record initial press
        wdesc->PToL(x, y, ZoomCmd->Refx, ZoomCmd->Refy);
        // Have to be careful about the ghosting.  The present ghost
        // context has to be restored after the button 3 manipulation.
        //
        ZoomCmd->GhostSet = true;

        ZoomCmd->InitId = wdesc->WindowId();
        ZoomCmd->DidMark = false;
        ev_zoom_win = wdesc;
        Gst()->SetGhostAt(GFzoom, ZoomCmd->Refx, ZoomCmd->Refy);
        // Push the call stack, but don't reset main state.
        for (int i = CallStackDepth - 1; i > 0; i--)
            ev_callbacks[i] = ev_callbacks[i-1];
        ev_callbacks[0] = ZoomCmd;

        DownTimer(GFbox_ns);
    }
    else if (state & (GR_SHIFT_MASK | GR_CONTROL_MASK)) {
        wdesc->PToL(x, y, x, y);
        int dx = (int)((x - ZoomCmd->Refx)*wdesc->Ratio());
        int dy = (int)((y - ZoomCmd->Refy)*wdesc->Ratio());
        if ((dx*dx + dy*dy) > 256) {
            // dragged > 16 pixels
            Gst()->SetGhost(GFnone);
            ZoomCmd->DidMark = true;
            ZoomCmd->Mx = x;
            ZoomCmd->My = y;
            Gst()->SetGhostAt(GFzoom, ZoomCmd->Refx, ZoomCmd->Refy);
        }
    }
    else
        ZoomCmd->action(wdesc, x, y);
}


void
cEventHdlr::ZoomRelease(WindowDesc *wdesc, int x, int y, int state)
{
    if (!ZoomCmd || !wdesc)
        return;
    WindowDesc *initdesc = DSP()->Windesc(ZoomCmd->InitId);
    if (!initdesc) {
        initdesc = wdesc;
       ZoomCmd->InitId = wdesc->WindowId();
    }
    if (UpTimer() || !wdesc->IsSimilar(initdesc))
        return;
    if (!ZoomCmd->DidMark && (state & (GR_SHIFT_MASK | GR_CONTROL_MASK))) {
        wdesc->PToL(x, y, x, y);
        int dx = (int)((x - ZoomCmd->Refx)*wdesc->Ratio());
        int dy = (int)((y - ZoomCmd->Refy)*wdesc->Ratio());
        if ((dx*dx + dy*dy) > 256) {
            // dragged > 16 pixels
            Gst()->SetGhost(GFnone);
            ZoomCmd->DidMark = true;
            ZoomCmd->Mx = x;
            ZoomCmd->My = y;
            Gst()->SetGhostAt(GFzoom, ZoomCmd->Refx, ZoomCmd->Refy);
        }
    }
    else {
        int xx = x, yy = y;
        wdesc->PToL(xx, yy, xx, yy);
        int dx = (int)((xx - ZoomCmd->Mx)*wdesc->Ratio());
        int dy = (int)((yy - ZoomCmd->My)*wdesc->Ratio());
        if ((dx*dx + dy*dy) > 256)
            ZoomCmd->action(wdesc, x, y);
    }
}


// Handle button1 press events.
//
void
cEventHdlr::Button1Callback(WindowDesc *wdesc, int x, int y, int state)
{
    ev_cursor_desc.press_handler(wdesc, x, y, state);
}


// Handle button1 release events.
//
void
cEventHdlr::Button1ReleaseCallback(WindowDesc *wdesc, int x, int y, int state)
{
    ev_cursor_desc.release_handler(wdesc, x, y, state);
}


// Handle button2 press events.
//
void
cEventHdlr::Button2Callback(WindowDesc *wdesc, int x, int y, int state)
{
    PanPress(wdesc, x, y, state);
}


// Handle button2 release events.
//
void
cEventHdlr::Button2ReleaseCallback(WindowDesc *wdesc, int x, int y, int state)
{
    PanRelease(wdesc, x, y, state);
}


// Handle button3 press events.
//
void
cEventHdlr::Button3Callback(WindowDesc *wdesc, int x, int y, int state)
{
    if (!ZoomCmd && (state & (GR_SHIFT_MASK | GR_CONTROL_MASK)))
        PanPress(wdesc, x, y, state);
    else
        ZoomPress(wdesc, x, y, state);
}


// Handle button3 release events.
//
void
cEventHdlr::Button3ReleaseCallback(WindowDesc *wdesc, int x, int y, int state)
{
    if (PanCmd)
        PanRelease(wdesc, x, y, state);
    else
        ZoomRelease(wdesc, x, y, state);
}


// Handle "no-op" button press events.  Print the coordinates, but don't
// really change anything.
//
void
cEventHdlr::ButtonNopCallback(WindowDesc *wdesc, int x, int y, int)
{
    ev_cursor_desc.noop_handler(wdesc, x, y);
}


// Handle "no-op" button release events (does nothing).
//
void
cEventHdlr::ButtonNopReleaseCallback(WindowDesc*, int, int, int)
{
}


// Record the window where most recent motion occurred.
//
void
cEventHdlr::MotionCallback(WindowDesc *wdesc, int state)
{
    ev_current_win = wdesc;
    if (ev_motion_task)
        (*ev_motion_task)();
    ProcessModState(state);
}


// Clear any pending motion task.
//
void
cEventHdlr::MotionClear()
{
    ev_motion_task = 0;
}


// Common point for processing modifier key changes.  The motion
// events are tracked as well as the key up/down events, to avoid
// problems with the key up events being lost due to a menu grab.
//
bool
cEventHdlr::ProcessModState(int mstate)
{
    // Also send the up event to the background command, if any, to
    // reset any ghost drawing mode which might be in effect, such as
    // 45-constraint.

    bool state = (mstate & GR_SHIFT_MASK);
    if (ev_shift_down != state) {
        ev_shift_down = state;
        if (ev_callbacks[1] && !ev_shift_down)
            ev_callbacks[1]->key(SHIFTUP_KEY, "", mstate);
        if (ev_callbacks[0]) {
            if (ev_callbacks[0]->key(
                    ev_shift_down ? SHIFTDN_KEY : SHIFTUP_KEY, "", mstate))
                return (true);
        }
    }
    state = (mstate & GR_CONTROL_MASK);
    if (ev_ctrl_down != state) {
        ev_ctrl_down = state;
        if (ev_callbacks[1] && !ev_ctrl_down)
            ev_callbacks[1]->key(CTRLUP_KEY, "", mstate);
        if (ev_callbacks[0]) {
            if (ev_callbacks[0]->key(
                    ev_ctrl_down ? CTRLDN_KEY : CTRLUP_KEY, "", mstate))
                return (true);
        }
    }
    state = (mstate & GR_ALT_MASK);
    if (ev_alt_down != state) {
        ev_alt_down = state;
    }
    return (false);
}


void
cEventHdlr::PanicPrint(FILE *panicFp)
{
    fprintf(panicFp, "cmd: %s level: %d\n",
        ev_callbacks[0] ? (ev_callbacks[0]->Name() ?
            ev_callbacks[0]->Name() : "->null") : "null",
        ev_callbacks[0] ? ev_callbacks[0]->CmdLevel() : 0);
}


// Return the window of the last button 1 press, or 0 if the window
// doesn't exist.
//
WindowDesc *
cEventHdlr::ButtonWin(bool alt_ok)
{
    if (alt_ok && (ev_cursor_desc.get_press_alt() ||
            ev_cursor_desc.get_was_press_alt()))
        return (DSP()->Windesc(ev_cursor_desc.get_alt_window()));
    return (DSP()->Windesc(ev_cursor_desc.get_window()));
}


//-----------------------------------------------------------------------------
// Callbacks for performing selection operations.
// These functions are used in the state functions of commands that
// require a selection operation.

// Static function.
// Button1 press.  Start the timer if initial press.  Complete the AOI if
// second press.
//
void
cEventHdlr::sel_b1down()
{
    int xr, yr;
    EV()->Cursor().get_raw(&xr, &yr);
    if (selstate.state <= 0) {

        // save click AOI
        selstate.AOI.left = xr;
        selstate.AOI.bottom = yr;
        selstate.AOI.right = xr;
        selstate.AOI.top = yr;
        selstate.id = DSPpkg::self()->AddTimer(250, timeout, 0);
    }
    else {
        selstate.AOI.right = xr;
        selstate.AOI.top = yr;
        selstate.AOI.fix();
    }
    selstate.state = 0;
}


// Static function.
// Button1 release.  Perform the selection operation.  Giving
// selection value 1 (defined as B1UP_NOSEL) skips any selection
// operation.  Return false and skip selection if drag mode and no
// pointer motion.
//
bool
cEventHdlr::sel_b1up(BBox *AOI, const char *types, CDol **selection,
    bool empty_click_desel)
{
    selstate.id = 0;
    if (selstate.state < 0) {
        selstate.state = 0;
        return (false);
    }
    if (selstate.state == 1) {
        if (!EV()->Cursor().is_release_ok()) {
            selstate.state = 2;
            return (false);
        }
        // dragged...
        int xr, yr;
        EV()->Cursor().get_release(&xr, &yr);
        selstate.AOI.right = xr;
        selstate.AOI.top = yr;
        selstate.AOI.fix();
        WindowDesc *wdesc = EV()->ButtonWin();
        if (!wdesc)
            wdesc = DSP()->MainWdesc();
        int delta = (int)(2.0/wdesc->Ratio());
        if (delta >= selstate.AOI.width() && delta >= selstate.AOI.height()) {
            // ...but pointer didn't move enough
            selstate.state = 2;
            return (false);
        }
    }

    if (AOI)
        *AOI = selstate.AOI;
    Gst()->SetGhost(GFnone);
    selstate.ghost_on = false;
    XM()->SetCoordMode(CO_ABSOLUTE);
    if (selection) {
        if (selection != B1UP_NOSEL) {
            *selection = Selections.selectItems(CurCell(), types,
                &selstate.AOI, PSELpoint);
            *selection = Selections.filter(CurCell(), *selection,
                &selstate.AOI, false);
            if (*selection && (*selection)->odesc->type() == CDINSTANCE &&
                    !XM()->IsBoundaryVisible(CurCell(), (*selection)->odesc)) {
                bool cells_only = (types && *types ==
                    CDINSTANCE && *(types+1) == '\0');
                if (!cells_only) {
                    CDol::destroy(*selection);
                    *selection = 0;
                }
                else {
                    const char *sym_name =
                        Tstring(OCALL((*selection)->odesc)->cellname());
                    PL()->ShowPromptV("You have selected an instance of %s.",
                        sym_name);
                }
            }
        }
    }
    else if (!Selections.selection(CurCell(), types, &selstate.AOI)) {
        if (empty_click_desel && !CDvdb()->getVariable("NoDeselLast")) {
            if (Selections.deselectLast(CurCell()))
                XM()->ShowParameters();
        }
    }
    selstate.state = 0;
    return (true);
}


// Static function.
// As b1down, but handle selections in windows with dissimilar content
// from main window.
//
void
cEventHdlr::sel_b1down_altw()
{
    int xr, yr;
    EV()->Cursor().get_alt_down(&xr, &yr);
    if (selstate.state <= 0) {

        // save click AOI
        selstate.AOI.left = xr;
        selstate.AOI.bottom = yr;
        selstate.AOI.right = xr;
        selstate.AOI.top = yr;
        selstate.id = DSPpkg::self()->AddTimer(250, timeout, (void*)1L);
    }
    else {
        selstate.AOI.right = xr;
        selstate.AOI.top = yr;
        selstate.AOI.fix();
    }
    selstate.state = 0;
}


// Static function.
// As b1up, but handle selections in windows with dissimilar content
// from main window.
//
bool
cEventHdlr::sel_b1up_altw(BBox *AOI, const char *types, CDol **selection,
    uintptr_t *win_id, bool empty_click_desel)
{
    selstate.id = 0;
    if (selstate.state < 0) {
        selstate.state = 0;
        return (false);
    }
    if (selstate.state == 1) {
        if (!EV()->Cursor().get_press_alt()) {
            selstate.state = 2;
            return (false);
        }
        // dragged...
        int xr, yr;
        EV()->Cursor().get_alt_up(&xr, &yr);
        selstate.AOI.right = xr;
        selstate.AOI.top = yr;
        selstate.AOI.fix();
        WindowDesc *wdesc = EV()->ButtonWin();
        if (!wdesc)
            wdesc = DSP()->MainWdesc();
        int delta = (int)(2.0/wdesc->Ratio());
        if (delta >= selstate.AOI.width() && delta >= selstate.AOI.height()) {
            // ...but pointer didn't move enough
            selstate.state = 2;
            return (false);
        }
    }

    if (AOI)
        *AOI = selstate.AOI;
    if (win_id)
        *win_id = EV()->Cursor().get_alt_window();
    WindowDesc *wdesc = EV()->ButtonWin(true);
    if (!wdesc)
        return (false);
    CDs *sdesc = wdesc->CurCellDesc(wdesc->Mode());
    if (!sdesc)
        return (false);

    Gst()->SetGhost(GFnone);
    selstate.ghost_on = false;
    XM()->SetCoordMode(CO_ABSOLUTE);
    if (selection) {
        if (selection != B1UP_NOSEL) {
            *selection = Selections.selectItems(sdesc, types,
                &selstate.AOI, PSELpoint);
            *selection = Selections.filter(sdesc, *selection,
                &selstate.AOI, false);
            if (*selection && (*selection)->odesc->type() == CDINSTANCE &&
                    !XM()->IsBoundaryVisible(sdesc, (*selection)->odesc)) {
                bool cells_only = (types && *types ==
                    CDINSTANCE && *(types+1) == '\0');
                if (!cells_only) {
                    CDol::destroy(*selection);
                    *selection = 0;
                }
                else {
                    const char *sym_name =
                        Tstring(OCALL((*selection)->odesc)->cellname());
                    PL()->ShowPromptV("You have selected an instance of %s.",
                        sym_name);
                }
            }
        }
    }
    else {
        if (!Selections.selection(sdesc, types, &selstate.AOI)) {
            if (empty_click_desel) {
                if (Selections.deselectLast(sdesc))
                    XM()->ShowParameters();
            }
        }
    }
    selstate.state = 0;
    return (true);
}


// Static function.
// Escape entered, abort.
//
void
cEventHdlr::sel_esc()
{
    selstate.state = 0;
    selstate.id = 0;
    if (selstate.ghost_on) {
        Gst()->SetGhost(GFnone);
        selstate.ghost_on = false;
    }
    XM()->SetCoordMode(CO_ABSOLUTE);
}


// Static function.
// Undo corner anchor.
//
bool
cEventHdlr::sel_undo()
{
    if (selstate.state == 1) {
        selstate.state = -1;
        selstate.id = 0;
        Gst()->SetGhost(GFnone);
        selstate.ghost_on = false;
        XM()->SetCoordMode(CO_ABSOLUTE);
        return (true);
    }
    EditIf()->ulUndoOperation();
    return (false);
}


// Static function.
// Redo corner anchor.  The was_alt argument should be true if selecting
// in alt windows was enabled.
//
void
cEventHdlr::sel_redo(bool was_alt)
{
    if (EditIf()->ulHasRedo())
        EditIf()->ulRedoOperation();
    else if (selstate.state == -1) {
        selstate.state = 1;
        Gst()->SetGhostAt(GFbox_ns, selstate.AOI.left, selstate.AOI.bottom,
            was_alt ? GhostAltOnly : GhostVanilla);
        selstate.ghost_on = true;
        XM()->SetCoordMode(CO_RELATIVE,
            selstate.AOI.left, selstate.AOI.bottom);
    }
}


// Static function.
// After the timer set by button1 down expires, start showing the
// ghost rectangle used to indicate the selection area.  Thus, click
// select will not alter the display.
//
// The argument is an "alt window" flag.
//
int
cEventHdlr::timeout(void *arg)
{
    if (selstate.id) {
        XM()->SetCoordMode(CO_RELATIVE,
            selstate.AOI.left, selstate.AOI.bottom);
        if (arg)
            Gst()->SetGhost(GFbox_ns, GhostAltOnly);
        else
            Gst()->SetGhost(GFbox_ns);
        selstate.ghost_on = true;
        selstate.id = 0;
        selstate.state = 1;
    }
    return (false);
}
// End of cEventHdlr functions.


void
CursorDesc::press_handler(WindowDesc *wdesc, int x, int y, int state)
{
    // Don't dispatch events from DSPpkg::self()->CheckInterrupt while
    // this function is active.  Otherwise, the b1up functions can be
    // called prematurely from here, breaking command logic.
    EventDispatchSuppress evs;

    if (!wdesc)
        return;
    set_release_ok(false);
    set_was_press_alt(false);
    if (wdesc->IsSimilar(DSP()->MainWdesc())) {
        // Button press in main or similar window.

        EV()->SetCurrentWin(wdesc);
        set_press_alt(false);
        set_alt_window(0);
        int xr, yr;
        wdesc->PToL(x, y, xr, yr);
        int xg = xr;
        int yg = yr;
        wdesc->Snap(&xg, &yg);
        update_down(wdesc->WindowId(), xr, yr, xg, yg, state);
        set_press_ok(true);
        if (EV()->CurCmd())
            EV()->CurCmd()->b1down();
    }
    else {
        // Button press in non-similar window, save the alt location
        // parameters, and the reference (so measurement works).

        int xr, yr;
        wdesc->PToL(x, y, xr, yr);
        set_alt_down(xr, yr);
        set_alt_downstate(state);
        wdesc->Snap(&xr, &yr);
        EV()->SetReference(xr, yr);
        set_alt_window(wdesc->WindowId());
        set_press_alt(true);
        set_press_ok(false);
        if (EV()->CurCmd())
            EV()->CurCmd()->b1down_altw();
    }
    XM()->ShowParameters();
}


void
CursorDesc::release_handler(WindowDesc *wdesc, int x, int y, int state)
{
    // Set this false, again, in case button was pressed outside
    // of windows.
    set_release_ok(false);

    if (!wdesc && EV()->MainCmd() && EV()->CurCmd() == EV()->MainCmd())
        EV()->MainCmd()->esc();

    if (is_press_ok()) {
        if (wdesc && wdesc->IsSimilar(DSP()->MainWdesc())) {
            int xr, yr;
            wdesc->PToL(x, y, xr, yr);
            update_up(xr, yr, state);
            set_release_ok(true);
        }
        // With release_ok false, the b1up function should simply do
        // state cleanup and return.
        if (EV()->CurCmd())
            EV()->CurCmd()->b1up();
        XM()->ShowParameters();
    }
    else if (get_press_alt()) {
        if (wdesc) {
            int xr, yr;
            wdesc->PToL(x, y, xr, yr);
            set_alt_up(xr, yr);
            set_alt_upstate(state);
            if (!wdesc->IsSimilar(DSP()->Windesc(get_alt_window()))) {
                // Down in alt window, up in dissimilar window.  The flags
                // below represent this case.
                set_press_alt(false);
                set_was_press_alt(true);
            }
        }
        else
            set_press_alt(false);
        if (EV()->CurCmd())
            EV()->CurCmd()->b1up_altw();
    }
    set_press_ok(false);
}


void
CursorDesc::noop_handler(WindowDesc *wdesc, int x, int y)
{
    set_release_ok(false);
    int xr, yr;
    wdesc->PToL(x, y, xr, yr);
    wdesc->Snap(&xr, &yr);
    EV()->SetReference(xr, yr);
    XM()->ShowParameters();
}


void
CursorDesc::phony_press(WindowDesc *wdesc, int x, int y)
{
    update_down(wdesc->WindowId(), x, y, x, y, 0);
    set_press_ok(true);

    if (EV()->CurCmd()) {
        // Unset any modifier action from input.
        char ctmp = 0;
        EV()->CurCmd()->key(CTRLUP_KEY, &ctmp, 0);
        EV()->CurCmd()->key(SHIFTUP_KEY, &ctmp, 0);
    }
    if (EV()->CurCmd())
        EV()->CurCmd()->b1down();

    update_up(x, y, 0);
    set_release_ok(true);
    if (EV()->CurCmd())
        EV()->CurCmd()->b1up();
    XM()->ShowParameters();
    Gst()->RepaintGhost();
}
// End of CursorDesc functions.


// Function to ghost-draw the boxes used in the button 3 zoom
// operation.
//
void
cGhost::ShowGhostZoom(int new_x, int new_y, int ref_x, int ref_y)
{
    if (!ZoomCmd)
        return;
    WindowDesc *initdesc = DSP()->Windesc(ZoomCmd->InitId);
    if (!initdesc) {
        initdesc = EV()->CurrentWin() ?
            EV()->CurrentWin() : DSP()->MainWdesc();
        ZoomCmd->InitId = initdesc->WindowId();
    }
    if (!ZoomCmd->DidMark) {
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::ALL);
        while ((wdesc = wgen.next()) != 0) {
            if (!wdesc->IsSimilar(initdesc))
                continue;
            wdesc->ShowLineBox(new_x, new_y, ref_x, ref_y);
        }
    }
    else {
        if (EV()->CurrentWin() == initdesc)
            initdesc->ShowLineBox(new_x, new_y, ref_x, ref_y);

        DSPmainDraw(SetLinestyle(&ZoomCmd->Linestyle))
        new_x = ZoomCmd->Mx;
        new_y = ZoomCmd->My;
        initdesc->ShowLineBox(new_x, new_y, ref_x, ref_y);
        DSPmainDraw(SetLinestyle(0))
    }
}

