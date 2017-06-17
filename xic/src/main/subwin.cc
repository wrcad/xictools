
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
 $Id: subwin.cc,v 5.15 2015/01/09 05:52:50 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "dsp_inlines.h"
#include "events.h"
#include "menu.h"
#include "promptline.h"
#include "ghost.h"


// State for subwindow definition.  The user points to define
// a rectangular area to be displayed in the subwindow.
//
namespace {
    namespace main_subwin {
        struct SubwState : public CmdState
        {
            SubwState(const char*, const char*);
            virtual ~SubwState();

            void setCaller(GRobject c)  { Caller = c; }

            void b1down();
            void b1up();
            void esc();
            void subw_doit(int, int);
            void message() { if (Level == 1) PL()->ShowPrompt(msg1);
                else PL()->ShowPrompt(msg2); }

        private:
            void SetLevel1(bool show) { Level = 1; if (show) message(); }
            void SetLevel2() { Level = 2; message(); }

            GRobject Caller;
            int Refx, Refy;

            static const char *msg1;
            static const char *msg2;
        };

        SubwState *SubwCmd;
    }
}

using namespace main_subwin;

const char *SubwState::msg1 =
    "Click twice or drag to define rectangular area.";
const char *SubwState::msg2 = "Click on second diagonal endpoint.";


// Menu function for vport command.
//
void
cMain::SubWindowExec(CmdDesc *cmd)
{
    if (SubwCmd) {
        SubwCmd->esc();
        return;
    }
    SubwCmd = new SubwState("SUBW", "xic:vport");
    SubwCmd->setCaller(cmd ? cmd->caller : 0);
    if (!EV()->PushCallback(SubwCmd)) {
        if (cmd)
            Menu()->Deselect(cmd->caller);
        delete SubwCmd;
        return;
    }
    SubwCmd->message();
}


SubwState::SubwState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    Refx = Refy = 0;
    Level = 1;
}


SubwState::~SubwState()
{
    SubwCmd = 0;
}


void
SubwState::b1down()
{
    if (Level == 1) {
        // Button1 down.  Record location, start drawing ghost rectangle.
        //
        EV()->Cursor().get_raw(&Refx, &Refy);
        Gst()->SetGhost(GFbox_ns);
        EV()->DownTimer(GFbox_ns);
    }
    else {
        // Second button1 press, create subwindow.
        //
        DSPmainDraw(ShowGhost(ERASE))
        int xr, yr;
        EV()->Cursor().get_raw(&xr, &yr);
        subw_doit(xr, yr);
    }
}


void
SubwState::b1up()
{
    if (Level == 1) {
        // Button1 up.  Create subwindow if the pointer moved appreciably.
        // Otherwise wait for user to click button1 again.
        //
        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok() &&
                EV()->CurrentWin()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            int xr, yr;
            EV()->Cursor().get_raw(&xr, &yr);
            int dx = (int)((x - xr) * EV()->CurrentWin()->Ratio());
            int dy = (int)((y - yr) * EV()->CurrentWin()->Ratio());
            if (dx*dx + dy*dy > 256) {
                // moved more than 16 pixels
                DSPmainDraw(ShowGhost(ERASE))
                subw_doit(x, y);
                esc();
                DSPmainDraw(ShowGhost(DISPLAY))
                return;
            }
        }
        SetLevel2();
    }
    else {
        // Second button1 release, exit.
        //
        esc();
        DSPmainDraw(ShowGhost(DISPLAY))
    }
}


// Abort subwindow creation.
//
void
SubwState::esc()
{
    Gst()->SetGhost(GFnone);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


// Actually create the subwindow, unless there are too many already.
//
void
SubwState::subw_doit(int x, int y)
{
    BBox AOI;
    AOI.left = mmMin(Refx, x);
    AOI.bottom = mmMin(Refy, y);
    AOI.right = mmMax(Refx, x);
    AOI.top = mmMax(Refy, y);

    DSP()->OpenSubwin(&AOI);
}

