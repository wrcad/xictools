
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
 $Id: measure.cc,v 5.104 2016/01/20 04:20:03 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "editif.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "dsp_window.h"
#include "events.h"
#include "menu.h"
#include "promptline.h"
#include "ghost.h"


namespace {
    namespace main_measure {
        struct RuState : public CmdState
        {
            friend void cGhost::ShowGhostRuler(int, int, int, int, bool);

            RuState(const char*, const char*);
            ~RuState();

            void setCaller(GRobject c)  { Caller = c; }

            void message();

            void b1down();
            void b1up();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();
            bool check_similar(const WindowDesc*, const WindowDesc*);

        private:
            void SetLevel1() { Level = 1; message(); }
            void SetLevel2() { Level = 2; message(); }

            GRobject Caller;
            WindowDesc *Wdesc;
            int Refx;
            int Refy;
            double SegLen;
            double ChainLen;
            sRuler *RedoList;
            bool Mirror;
            bool DownOK;
        };

        SymTab *RulerTab;
        RuState *RuCmd;
    }
}

using namespace main_measure;


// Menu command to draw a ruler.
//
void
cMain::RulerExec(CmdDesc *cmd)
{
    if (RuCmd) {
        RuCmd->esc();
        return;
    }
    RuCmd = new RuState("RULER", "xic:ruler");
    RuCmd->setCaller(cmd ? cmd->caller : 0);
    if (!EV()->PushCallback(RuCmd)) {
        if (cmd)
            Menu()->Deselect(cmd->caller);
        delete RuCmd;
        return;
    }
    RuCmd->message();
}


RuState::RuState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    Wdesc = 0;
    Refx = Refy = 0;
    SegLen = 0.0;
    ChainLen = 0.0;
    RedoList = 0;
    Mirror = false;
    DownOK = false;
    SetLevel1();

    DSP()->StartRulerCmd();
}


RuState::~RuState()
{
    RedoList->free();
    RuCmd = 0;
    DSP()->EndRulerCmd();
}


void
RuState::message()
{
    const char *msg1 = "Click/drag defines ruler, keys: / flip,"
        " . snap (now %s), Ctrl unlock, Shift chain.";
    const char *msg2 = "Click on second endpoint, keys: / flip,"
        " . snap (now %s), Ctrl unlock, Shift chain.";

    bool snap;
    DSP()->RulerGetSnapDefaults(0, &snap, false);
    char buf[256];
    if (Level == 1) {
        if (SegLen > 0.0) {
            sprintf(buf, msg1, snap ? "on" : "off");
            sprintf(buf + strlen(buf), "  Last ruler length is %.4f microns.",
                SegLen);
            PL()->ShowPrompt(buf);
        }
        else
            PL()->ShowPromptV(msg1, snap ? "on" : "off");
    }
    else
        PL()->ShowPromptV(msg2, snap ? "on" : "off");
}


namespace {
    double dist(int x1, int y1, int x2, int y2)
    {
        double dx = MICRONS(x2 - x1);
        double dy = MICRONS(y2 - y1);
        return (sqrt(dx*dx + dy*dy));
    }
}


void
RuState::b1down()
{
    if (Level == 1) {
        // Anchor first end of ruler.
        //
        EV()->Cursor().get_raw(&Refx, &Refy);
        Wdesc = EV()->CurrentWin() ?
            EV()->CurrentWin() : DSP()->MainWdesc();
        EV()->CurrentWin()->Snap(&Refx, &Refy);
        Refy = mmRnd(Refy/Wdesc->YScale());
        Gst()->SetGhostAt(GFruler, Refx, Refy);
        Wdesc->SetAccumMode(WDaccumStart);  // Defer highlighting redisplay
        SegLen = 0.0;
        EV()->DownTimer(GFruler);
    }
    else {
        // Create the ruler
        //
        DownOK = false;
        int x, y;
        EV()->Cursor().get_raw(&x, &y);
        EV()->CurrentWin()->Snap(&x, &y);
        y = mmRnd(y/Wdesc->YScale());
        if (XM()->To45snap(&x, &y, Refx, Refy)) {
            Gst()->SetGhost(GFnone);
            SegLen = dist(Refx, Refy, x, y);
            DSP()->SetRulers(new sRuler(Wdesc->WinNumber(), Refx,
                Refy, x, y, (bool)Mirror, ChainLen, DSP()->Rulers()));
            DSP()->Rulers()->show(DISPLAY);
            if (EV()->Cursor().get_downstate() & GR_SHIFT_MASK) {
                ChainLen += sqrt((x - Refx)*(double)(x - Refx) +
                    (y - Refy)*(double)(y - Refy))/CDphysResolution;
                Refx = x;
                Refy = y;
            }
            else
                ChainLen = 0.0;
            RedoList->free();
            RedoList = 0;
            DownOK = true;
            Wdesc->GhostFinalUpdate();  // Final highlighting redisplay.
        }
    }
}


// Maybe create ruler, or prepare for next press.
//
void
RuState::b1up()
{
    if (Level == 1) {
        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok() &&
                EV()->CurrentWin()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            y = mmRnd(y/Wdesc->YScale());
            if (XM()->To45snap(&x, &y, Refx, Refy)) {
                Gst()->SetGhost(GFnone);
                SegLen = dist(Refx, Refy, x, y);
                DSP()->SetRulers(new sRuler(Wdesc->WinNumber(),
                    Refx, Refy, x, y, (bool)Mirror, ChainLen,
                    DSP()->Rulers()));
                DSP()->Rulers()->show(DISPLAY);
                if (EV()->Cursor().get_upstate() & GR_SHIFT_MASK) {
                    ChainLen += sqrt((x - Refx)*(double)(x - Refx) +
                        (y - Refy)*(double)(y - Refy))/CDphysResolution;
                    Refx = x;
                    Refy = y;
                    Gst()->SetGhostAt(GFruler, Refx, Refy);
                    SetLevel2();
                }
                else {
                    ChainLen = 0.0;
                    message();
                }
                RedoList->free();
                RedoList = 0;
                Wdesc->GhostFinalUpdate();  // Final highlighting redisplay.
                return;
            }
        }
        SetLevel2();
    }
    else {
        // Reset state
        //
        if (DownOK) {
            if (ChainLen == 0.0)
                SetLevel1();
            else
                Gst()->SetGhostAt(GFruler, Refx, Refy);
        }
    }
}


// Esc entered, clean up and abort.
//
void
RuState::esc()
{
    Gst()->SetGhost(GFnone);
    if (Wdesc)
        Wdesc->GhostFinalUpdate();  // Final highlighting redisplay.
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


bool
RuState::key(int code, const char *text, int)
{
    if (code == DELETE_KEY) {
        WindowDesc *wd = EV()->CurrentWin();
        sRuler *rp = 0, *rn;
        for (sRuler *ruler = DSP()->Rulers(); ruler; ruler = rn) {
            rn = ruler->next;
            if (!wd || ruler->win_num == wd->WinNumber()) {
                ruler->show(ERASE);
                if (!rp)
                    DSP()->SetRulers(rn);
                else
                    rp->next = rn;
                delete ruler;
                break;
            }
            rp = ruler;
        }
        return (true);
    }
    if (code == CTRLDN_KEY) {
        DSPmainDraw(ShowGhost(ERASE))
        EV()->SetConstrained(true);
        DSPmainDraw(ShowGhost(DISPLAY))
        return (false);
    }
    if (code == CTRLUP_KEY) {
        DSPmainDraw(ShowGhost(ERASE))
        EV()->SetConstrained(false);
        DSPmainDraw(ShowGhost(DISPLAY))
        return (false);
    }
    if (text && (*text == '/' || *text == '\\')) {
        // Flip the side used for gradations
        DSPmainDraw(ShowGhost(ERASE))
        Mirror = !Mirror;
        DSPmainDraw(ShowGhost(DISPLAY))
        return (true);
    }
    if (text && *text == '.') {
        if (Level == 2) {
            Gst()->SetGhost(GFnone);
            bool snap;
            DSP()->RulerGetSnapDefaults(0, &snap, false);
            snap = !snap;
            DSP()->RulerSetSnapDefaults(0, &snap);
            DSP()->SetNoGridSnapping(!snap);
            EV()->CurrentWin()->Snap(&Refx, &Refy);
            Gst()->SetGhostAt(GFruler, Refx, Refy);
            Gst()->RepaintGhost();
        }
        else {
            DSPmainDraw(ShowGhost(ERASE))
            bool snap;
            DSP()->RulerGetSnapDefaults(0, &snap, false);
            snap = !snap;
            DSP()->RulerSetSnapDefaults(0, &snap);
            DSP()->SetNoGridSnapping(!snap);
            DSPmainDraw(ShowGhost(DISPLAY))
        }
        message();
        return (true);
    }
    return (false);
}


void
RuState::undo()
{
    if (Level == 1) {
        sRuler *ruler = DSP()->Rulers();
        if (ruler) {
            DSP()->SetRulers(ruler->next);
            ruler->next = RedoList;
            RedoList = ruler;
            ruler->show(ERASE);
            SegLen = 0.0;

            if (ruler->win_num >= 0 && ruler->win_num < DSP_NUMWINS) {
                Wdesc = DSP()->Window(ruler->win_num);
                if (Wdesc) {
                    Refx = ruler->p1.x;
                    Refy = ruler->p1.y;
                    Mirror = ruler->mirror;
                    ChainLen = ruler->loff;
                    Gst()->SetGhostAt(GFruler, Refx, Refy);
                    Gst()->RepaintGhost();
                    SetLevel2();
                }
            }
        }
        else
            EditIf()->ulUndoOperation();
    }
    else {
        // Undo the anchor.
        //
        Gst()->SetGhost(GFnone);
        SetLevel1();
    }
}


void
RuState::redo()
{
    if (Level == 1) {
        sRuler *ruler = RedoList;
        if (ruler) {
            if (ruler->win_num >= 0 && ruler->win_num < DSP_NUMWINS) {
                Wdesc = DSP()->Window(ruler->win_num);
                if (Wdesc) {
                    Refx = ruler->p1.x;
                    Refy = ruler->p1.y;
                    Mirror = ruler->mirror;
                    ChainLen = ruler->loff;
                    Gst()->SetGhostAt(GFruler, Refx, Refy);
                    Gst()->RepaintGhost();
                    SetLevel2();
                }
            }
        }
        else
            EditIf()->ulRedoOperation();
    }
    else {
        sRuler *ruler = RedoList;
        if (ruler) {
            Gst()->SetGhost(GFnone);
            RedoList = RedoList->next;
            ruler->next = DSP()->Rulers();
            DSP()->SetRulers(ruler);
            ruler->show(DISPLAY);
            SetLevel1();
        }
    }
}


// While the ruler command is active, we send events to all windows
// showing physical data, including cross-section displays and CHD
// displays.  Thus, all Physical windows may have rulers.
//
bool
RuState::check_similar(const WindowDesc *wd1, const WindowDesc *wd2)
{
    return (wd1->Mode() == Physical && wd2->Mode() == Physical);
}
// End of RuState methods


// Change ruler context, called when current cell changes.
//
void
cMain::SetRulers(CDs *sdesc, CDs *oldsdesc)
{
    if (!RulerTab) {
        if (DSP()->Rulers())
            RulerTab = new SymTab(false, false);
        else
            return;
    }

    if (oldsdesc) {
        RulerTab->remove((unsigned long)oldsdesc);
        if (DSP()->Rulers()) {
            RulerTab->add((unsigned long)oldsdesc, DSP()->Rulers(), false);
            DSP()->SetRulers(0);
        }
    }
    if (sdesc) {
        DSP()->SetRulers((sRuler*)RulerTab->get((unsigned long)sdesc));
        if (DSP()->Rulers() == (sRuler*)ST_NIL)
            DSP()->SetRulers(0);
    }
}


void
cMain::EraseRulers(CDs *sdesc, WindowDesc *wd, int num)
{
    if (sdesc) {
        // Delete all rulers for this cell, sdesc is being destroyed
        if (sdesc == CurCell(Physical)) {
            for (sRuler *r = DSP()->Rulers(); r; r = DSP()->Rulers()) {
                DSP()->SetRulers(r->next);
                r->show(ERASE);
                delete r;
            }
            if (RulerTab)
                RulerTab->remove((unsigned long)sdesc);
        }
        else if (RulerTab) {
            sRuler *r0 = (sRuler*)RulerTab->get((unsigned long)sdesc);
            if (r0 != (sRuler*)ST_NIL) {
                while (r0) {
                    sRuler *r = r0;
                    r0 = r0->next;
                    delete r;
                }
                RulerTab->remove((unsigned long)sdesc);
            }
        }
        return;
    }

    if (wd) {
        // Delete all rulers for this window, wd is being destroyed.
        int wnum = wd->WinNumber();
        if (wnum >= 0 && wnum < DSP_NUMWINS) {
            sRuler *rp = 0, *rn;
            for (sRuler *r = DSP()->Rulers(); r; r = rn) {
                rn = r->next;
                if (r->win_num == wnum) {
                    if (!rp)
                        DSP()->SetRulers(rn);
                    else
                        rp->next = rn;
                    delete r;
                    continue;
                }
                rp = r;
            }
        }
        return;
    }

    // Delete the num'th ruler, or all if num < 0
    int cnt = 0;
    sRuler *rp = 0, *rn;
    for (sRuler *r = DSP()->Rulers(); r; r = rn) {
        rn = r->next;
        if (num < 0 || cnt++ == num) {
            if (!rp)
                DSP()->SetRulers(rn);
            else
                rp->next = rn;
            r->show(ERASE);
            delete r;
            continue;
        }
        rp = r;
    }
}
// End of cMain functions.


void
cGhost::ShowGhostRuler(int x, int y, int refx, int refy, bool erase)
{
    if (!RuCmd || !RuCmd->Wdesc)
        return;
    RuCmd->Wdesc->Snap(&x, &y, true);
    y = mmRnd(y/RuCmd->Wdesc->YScale());
    if (XM()->To45snap(&x, &y, refx, refy)) {
        sRuler ru(RuCmd->Wdesc->WinNumber(), RuCmd->Refx,
            RuCmd->Refy, x, y, (bool)RuCmd->Mirror, RuCmd->ChainLen, 0);
        ru.show(DISPLAY, erase);
    }
}

