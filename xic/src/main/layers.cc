
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
#include "editif.h"
#include "drcif.h"
#include "extif.h"
#include "cd_lgen.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "cd_sdb.h"
#include "events.h"
#include "menu.h"
#include "attr_menu.h"
#include "view_menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "ghost.h"
#include "tech_ldb3d.h"
#include <algorithm>


//
// Some layer-related functions and menu commands.
//


// Return a list of the layer names that touch or overlap AOI, depth is
// search depth, 0 for cell only.
//
stringlist *
cMain::ListLayersInArea(CDs *sdesc, BBox *AOI, int depth)
{
    stringlist *l0 = 0;
    CDl *ld;
    CDlgen lgen(Physical, CDlgen::TopToBotNoCells);
    while ((ld = lgen.next()) != 0) {
        sPF gen(sdesc, AOI, ld, depth);
        CDo *odesc;
        while ((odesc = gen.next(false, false)) != 0) {
            bool ovl = odesc->intersect(AOI, false);
            delete odesc;
            if (ovl) {
                l0 = new stringlist(lstring::copy(ld->name()), l0);
                gen.clear();
                break;
            }
        }
    }
    return (l0);
}


// The Peek command
//
namespace {
    namespace main_layers {
        struct LpeekState : public CmdState
        {
            LpeekState(const char*, const char*);
            virtual ~LpeekState();

            void setCaller(GRobject c)  { Caller = c; }

            void b1down();
            void b1up();
            void esc();
            void peek_doit(int, int);
            void message() { if (Level == 1) PL()->ShowPrompt(msg1);
                else PL()->ShowPrompt(msg2); }

        private:
            void SetLevel1(bool show) { Level = 1; if (show) message(); }
            void SetLevel2() { Level = 2; message(); }

            GRobject Caller;
            unsigned long WinId;
            int Refx, Refy;

            static const char *msg1;
            static const char *msg2;
        };

        LpeekState *LpeekCmd;
    }
}

using namespace main_layers;

const char *LpeekState::msg1 =
    "Click twice or drag to define rectangular area.";
const char *LpeekState::msg2 = "Click on second diagonal endpoint.";


// Menu function for peek command.
//
void
cMain::PeekExec(CmdDesc *cmd)
{
    if (LpeekCmd) {
        LpeekCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (!XM()->CheckCurCell(false, false, DSP()->CurMode()))
        return;
    LpeekCmd = new LpeekState("LPEEK", "xic:peek");
    LpeekCmd->setCaller(cmd ? cmd->caller : 0);
    if (!EV()->PushCallback(LpeekCmd)) {
        delete LpeekCmd;
        return;
    }
    LpeekCmd->message();
    ds.clear();
}


LpeekState::LpeekState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    WinId = (unsigned long)-1;
    Refx = Refy = 0;
    Level = 1;
}


LpeekState::~LpeekState()
{
    LpeekCmd = 0;
}


void
LpeekState::b1down()
{
    if (Level == 1) {
        // Button1 down.  Record location, start drawing ghost rectangle.
        //
        EV()->Cursor().get_raw(&Refx, &Refy);
        WinId = EV()->Cursor().get_window();
        Gst()->SetGhost(GFbox_ns);
        EV()->DownTimer(GFbox_ns);
    }
    else {
        // Second button1 press, show layers
        //
        int xr, yr;
        EV()->Cursor().get_raw(&xr, &yr);
        peek_doit(xr, yr);
    }
}


void
LpeekState::b1up()
{
    if (Level == 1) {
        // Button1 up.  Lpeek if the pointer moved appreciably.
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
                peek_doit(x, y);
                SetLevel1(false);
                return;
            }
        }
        SetLevel2();
    }
    else
        SetLevel1(false);
}


// Abort peek command.
//
void
LpeekState::esc()
{
    Gst()->SetGhost(GFnone);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


// Redraw the layers, slowly, so user can see.
//
void
LpeekState::peek_doit(int x, int y)
{
    Gst()->SetGhost(GFnone);
    BBox AOI;
    AOI.left = mmMin(Refx, x);
    AOI.bottom = mmMin(Refy, y);
    AOI.right = mmMax(Refx, x);
    AOI.top = mmMax(Refy, y);

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    AOI.bloat(-1);  // avoid false positive from edge coincidence
    stringlist *l0 = XM()->ListLayersInArea(cursdp, &AOI, CDMAXCALLDEPTH);
    if (l0)
        // The redisplay now updates the layer names in the prompt line
        // as drawing occurs.
        stringlist::destroy(l0);
    else {
        PL()->ShowPrompt("No layers found.");
        return;
    }

    AOI.bloat(1);
    WindowDesc *wd = DSP()->Windesc(WinId);
    if (!wd)
        return;
    DSP()->SetSlowMode(true);
    wd->Redisplay(&AOI);
    DSP()->SetSlowMode(false);
}
// End of LpeekState functions


// The Cross Section command
//
namespace {
    namespace main_layers {
        struct ProfState : public CmdState
        {
            ProfState(const char*, const char*);
            virtual ~ProfState();

            void setCaller(GRobject c)  { Caller = c; }

            void b1down();
            void b1up();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void message()
                { if (Level == 1)
                    PL()->ShowPromptV(msg3, Snap ? "on" : "off");
                else PL()->ShowPrompt(msg4); }

        private:
            void SetLevel1() { Level = 1; message(); }
            void SetLevel2() { Level = 2; message(); }

            static void show_cross_section(int, int, int, int);
            static int action_idle(void*);

            GRobject Caller;
            int Refx;
            int Refy;
            bool Snap;
            bool GhostOn;

            static const char *msg3;
            static const char *msg4;
            static int sv_x1, sv_y1, sv_x2, sv_y2;
            static CDcellName sv_last;
        };

        ProfState *ProfCmd;
    }
}

const char *ProfState::msg3 =
    "Click/drag defines endpoints, keys: . snap (now %s), Enter redo, "
    "Ctrl unlock.";
const char *ProfState::msg4 = "Click on second endpoint.";

int ProfState::sv_x1;
int ProfState::sv_y1;
int ProfState::sv_x2;
int ProfState::sv_y2;

CDcellName ProfState::sv_last;


void
cMain::ProfileExec(CmdDesc *cmd)
{
    if (ProfCmd) {
        ProfCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (!XM()->CheckCurCell(false, false, DSP()->CurMode()))
        return;
    ProfCmd = new ProfState("PROFILE", "xic:csect");
    ProfCmd->setCaller(cmd ? cmd->caller : 0);
    if (!EV()->PushCallback(ProfCmd)) {
        delete ProfCmd;
        return;
    }
    ProfCmd->message();
    ds.clear();
}


ProfState::ProfState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    Refx = Refy = 0;
    Snap = false;
    GhostOn = false;
    SetLevel1();
}


ProfState::~ProfState()
{
    ProfCmd = 0;
}


void
ProfState::b1down()
{
    if (Level == 1) {
        // Anchor first end
        //
        if (Snap) {
            EV()->Cursor().get_xy(&Refx, &Refy);
            Gst()->SetGhost(GFvector);
            EV()->DownTimer(GFvector);
        }
        else {
            EV()->Cursor().get_raw(&Refx, &Refy);
            Gst()->SetGhostAt(GFvector_ns, Refx, Refy);
            EV()->DownTimer(GFvector_ns);
        }
        GhostOn = true;
    }
    else {
        // Finish
        //
        Gst()->SetGhost(GFnone);
        GhostOn = false;
        int x, y;
        if (Snap)
            EV()->Cursor().get_xy(&x, &y);
        else
            EV()->Cursor().get_raw(&x, &y);
        if (XM()->To45snap(&x, &y, Refx, Refy)) {
            sv_x1 = Refx;
            sv_y1 = Refy;
            sv_x2 = x;
            sv_y2 = y;
            sv_last = CurCell()->cellname();
            dspPkgIf()->RegisterIdleProc(action_idle, 0);
        }
    }
}


void
ProfState::b1up()
{
    if (Level == 1) {
        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok() &&
                EV()->CurrentWin()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            if (Snap)
                EV()->CurrentWin()->Snap(&x, &y);
            if (XM()->To45snap(&x, &y, Refx, Refy)) {
                Gst()->SetGhost(GFnone);
                GhostOn = false;
                sv_x1 = Refx;
                sv_y1 = Refy;
                sv_x2 = x;
                sv_y2 = y;
                sv_last = CurCell()->cellname();
                dspPkgIf()->RegisterIdleProc(action_idle, 0);
                return;
            }
        }
        SetLevel2();
    }
    else
        SetLevel1();
}


// Esc entered, clean up and abort.
//
void
ProfState::esc()
{
    Gst()->SetGhost(GFnone);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


bool
ProfState::key(int code, const char *text, int)
{
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
    if (code == RETURN_KEY) {
        // If the last points are valid, display the same cross-sect.
        if (sv_last == CurCell()->cellname())
            show_cross_section(sv_x1, sv_y1, sv_x2, sv_y2);
        return (false);
    }
    if (text && *text == '.') {
        if (GhostOn) {
            Gst()->SetGhost(GFnone);
            Snap = !Snap;
            if (Snap) {
                EV()->CurrentWin()->Snap(&Refx, &Refy);
                Gst()->SetGhostAt(GFvector, Refx, Refy);
            }
            else
                Gst()->SetGhostAt(GFvector_ns, Refx, Refy);
            Gst()->RepaintGhost();
        }
        else
            Snap = !Snap;
        message();
        return (true);
    }
    return (false);
}


void
ProfState::undo()
{
    if (Level == 1)
            EditIf()->ulUndoOperation();
    else {
        // Undo the anchor.
        //
        Gst()->SetGhost(GFnone);
        SetLevel1();
    }
}


// Static function
// Pop-up a window displaying the cross section along the line
// defined by the arguments.
//
void
ProfState::show_cross_section(int x1, int y1, int x2, int y2)
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    Point_c p1(x1, y1);
    Point_c p2(x2, y2);
    if (p1 == p2)
        return;

    Ldb3d ldb;
    if (Ldb3d::logging())
        ldb.set_logfp(DBG_FP);

    BBox BB(p1.x, p1.y, p2.x, p2.y);
    BB.fix();
    BB.bloat(2);
    dspPkgIf()->SetWorking(true);
    if (!ldb.init_cross_sect(cursdp, &BB)) {
        Errs()->add_error("Error building 3-d database.");
        Log()->ErrorLog(mh::Initialization, Errs()->get_error());
        dspPkgIf()->SetWorking(false);
        return;
    }
    Blist **boxes = ldb.line_scan(&p1, &p2);

    // Save the boxlist in a polymorphic database.
    SymTab *tab = new SymTab(false, false);
    int lnum = 0;
    for (Layer3d *l = ldb.layers(); l; l = l->next()) {
        bdb_t *db = 0;
        for (Blist *b = boxes[lnum]; b; b = b->next) {
            if (!db)
                db = new bdb_t;
            db->add(&b->BB);
        }
        if (db) {
            CDl *ld = l->layer_desc();
            tab->add((unsigned long)ld, db, false);
        }
        Blist::destroy(boxes[lnum]);
        lnum++;
    }
    delete [] boxes;
    dspPkgIf()->SetWorking(false);

    static int xs_count;
    char buf[64];
    sprintf(buf, "CrossSection-%d", xs_count);
    xs_count++;

    cSDB *sdb = new cSDB(buf, tab, sdbBdb);
    int wnum = DSP()->OpenSubwin(sdb->BB(), WDblist, buf, true);
    if (wnum < 0) {
        delete sdb;
        return;
    }
    sdb->set_owner(wnum);
    CDsdb()->saveDB(sdb);

    bool auto_y = (CDvdb()->getVariable(VA_XSectNoAutoY) == 0);
    double yscale = 1.0, ys;
    const char *var = CDvdb()->getVariable(VA_XSectYScale);
    if (var && sscanf(var, "%lf", &ys) == 1 && ys >= CDSCALEMIN &&
            ys <= CDSCALEMAX)
        yscale = ys;

    DSP()->Window(wnum)->SetXSect(true);
    DSP()->Window(wnum)->SetXSectAutoY(auto_y);
    DSP()->Window(wnum)->SetXSectYScale(yscale);
    DSP()->Window(wnum)->CenterFullView();
    DSP()->Window(wnum)->Redisplay(0);
}


// Idle function to do the work.  We don't want to grab all events
// while the work is being done, which happens without the idle proc.
//
int
ProfState::action_idle(void*)
{
    show_cross_section(sv_x1, sv_y1, sv_x2, sv_y2);
    return (0);
}
// End of ProfState methods.

