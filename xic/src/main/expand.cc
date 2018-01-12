
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
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "events.h"
#include "menu.h"
#include "promptline.h"


/*========================================================================*
 * The expnd command
 *========================================================================*/

namespace {
    namespace main_expand {
        struct PeekState : public CmdState
        {
            friend void cMain::ExpandExec(CmdDesc*);

            PeekState(const char*, const char*);
            virtual ~PeekState();

            void SetCaller(GRobject c)  { Caller = c; }

            void b1down() { cEventHdlr::sel_b1down(); }
            void b1up();
            void esc();
            bool key(int, const char*, int);
            void undo() { cEventHdlr::sel_undo(); }
            void redo() { cEventHdlr::sel_redo(); }
            void message() { PL()->ShowPrompt(pmsg); }

        private:
            CDol *expand(BBox*, CDs*, WindowDesc*, int, bool*);
            CDol *unexpand(BBox*, CDs*, WindowDesc*, int);
            Blist *find_blist(CDs*, WindowDesc*, CDc*, int);
            void redisplay(WindowDesc*, CDol*);

            static void process_list(CDol*, BBox*, WindowDesc*, bool, bool);
            static bool exp_cb(const char*, void*);

            GRobject Caller;
            BBox AOI;
            cTfmStack Stack;

            static WindowDesc *RedisplayWin;
            static const char *pmsg;
        };

        PeekState *PeekCmd;
    }
}

using namespace main_expand;

const char *PeekState::pmsg =
    "Click on instance to expand, Shift-click to unexpand.";
WindowDesc *PeekState::RedisplayWin = 0;


void
cMain::ExpandExec(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc)
        return;
    if (PeekCmd && cmd->wdesc == DSP()->MainWdesc()) {
        PeekCmd->esc();
        return;
    }
    if (Menu()->GetStatus(cmd->caller)) {
        PeekState::RedisplayWin = 0;
        DSPattrib *a = cmd->wdesc->Attrib();
        const char *cur;
        if (a->expand_level(cmd->wdesc->Mode()) < 0)
            cur = "none";
        else if (!a->expand_level(cmd->wdesc->Mode()))
            cur = "all";
        else
            cur = "+";

        if (a->expand_level(cmd->wdesc->Mode()) >= 0 &&
                cmd->wdesc == DSP()->MainWdesc()) {
            if (cmd->wdesc->Wbag())
                cmd->wdesc->Wbag()->PopUpExpand(cmd->caller, MODE_ON,
                    PeekState::exp_cb, cmd, cur, false);
        }
        else {
            if (cmd->wdesc->Wbag())
                cmd->wdesc->Wbag()->PopUpExpand(cmd->caller, MODE_ON,
                    PeekState::exp_cb, cmd, cur, true);
        }
    }
    else {
        if (cmd->wdesc->Wbag())
            cmd->wdesc->Wbag()->PopUpExpand(cmd->caller, MODE_OFF,
                0, 0, 0, false);
    }
}


PeekState::PeekState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
}


PeekState::~PeekState()
{
    PeekCmd = 0;
}


// Finish selection.  If success, do the peek.
//
void
PeekState::b1up()
{
    if (!cEventHdlr::sel_b1up(&AOI, 0, B1UP_NOSEL))
        return;
    WindowDesc *wdesc;
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    if ((wdesc = EV()->ButtonWin()) != 0) {
        CDol *sl;
        if (EV()->Cursor().get_upstate() & GR_SHIFT_MASK) {
            sl = unexpand(&AOI, cursd, wdesc, 0);
            process_list(sl, &AOI, wdesc, false, false);
        }
        else {
            bool filtered = false;
            sl = expand(&AOI, cursd, wdesc, 0, &filtered);
            process_list(sl, &AOI, wdesc, true, filtered);
        }
        redisplay(wdesc, sl);
    }
}


bool
PeekState::key(int code, const char *text, int)
{
    switch (code) {
    case SHIFTDN_KEY:
        PL()->ShowPrompt("Unexpanding");
        break;
    case SHIFTUP_KEY:
        PL()->ErasePrompt();
        break;
    case NUPLUS_KEY:
    case NUMINUS_KEY:
        // Don't confuse these with '+', '-'.
        return (false);
    default:
        WindowDesc *wdesc;
        if (text && (wdesc = EV()->CurrentWin()) != 0) {
            DSPattrib *a = wdesc->Attrib();
            if (*text == '+') {
                int lev = a->expand_level(wdesc->Mode());
                a->set_expand_level(wdesc->Mode(), lev + 1);
            }
            else if (*text == '-') {
                int lev = a->expand_level(wdesc->Mode());
                if (lev > 0) {
                    lev--;
                    a->set_expand_level(wdesc->Mode(), lev);
                }
            }
            else if (isdigit(*text)) {
                int lev = atoi(text);
                a->set_expand_level(wdesc->Mode(), lev);
                if (lev == 0)
                    wdesc->ClearExpand();
            }
            else if (*text == 'y' || *text == 'a' ||
                    *text == 'Y' || *text == 'A')
                a->set_expand_level(wdesc->Mode(), -1);
            else if (*text == 'n' || *text == 'N') {
                a->set_expand_level(wdesc->Mode(), 0);
                wdesc->ClearExpand();
            }
            else
                return (false);
        }
        else
            return (false);
        wdesc->Redisplay(0);
        break;
    }
    return (true);
}


// Esc entered, clean up and exit.
//
void
PeekState::esc()
{
    cEventHdlr::sel_esc();
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


// Set the expand flag of any unexpanded cells above the hierarchy level
// which overlap the BB.  Return a list of the odescs with flag changed.
// Set *pfilteresd true if the instance filter pop-up is shown.
//
CDol *
PeekState::expand(BBox *BB, CDs* sdesc, WindowDesc *wdesc, int hierlev,
    bool *pfiltered)
{
    if (Stack.TFull())
        return (0);

    CDol *ce = 0, *c0 = 0;
    CDg gdesc;
    Stack.TInitGen(sdesc, CellLayer(), BB, &gdesc);
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        CDs *msdesc = cdesc->masterCell();
        if (!msdesc || msdesc->isDevice())
            continue;
        CDol *cl = new CDol(cdesc, 0);
        if (!c0)
            c0 = ce = cl;
        else {
            ce->next = cl;
            ce = ce->next;
        }
    }
    if (c0 && c0->next && c0->next->next) {
        c0 = XM()->PopUpFilterInstances(c0);
        if (pfiltered)
            *pfiltered = true;
    }

    CDol *se = 0, *s0 = 0;
    for (CDol *cl = c0; cl; cl = cl->next) {
        cdesc = (CDc*)cl->odesc;

        if (cdesc->has_flag(wdesc->DisplFlags()) ||
                hierlev < wdesc->Attrib()->expand_level(wdesc->Mode())) {

            Stack.TPush();
            unsigned int x1, x2, y1, y2;
            if (Stack.TOverlapInst(cdesc, BB, &x1, &x2, &y1, &y2)) {
                // we really need to call the function over the area of the
                // cell once only
                if (y2 >= y1 + 2)
                    y2 = ++y1;
                if (x2 >= x1 + 2)
                    x2 = ++x1;

                CDs *msdesc = cdesc->masterCell();
                CDap ap(cdesc);
                int tx, ty;
                Stack.TGetTrans(&tx, &ty);
                xyg_t xyg(x1, x2, y1, y2);
                do {
                    Stack.TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                    CDol *sl =
                        expand(BB, msdesc, wdesc, hierlev + 1, pfiltered);
                    if (sl) {
                        if (!s0)
                            s0 = se = sl;
                        else
                            se->next = sl;
                        while (se->next)
                            se = se->next;
                    }
                    Stack.TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            Stack.TPop();

        }
        else {
            CDol *sl = new CDol(cdesc, 0);
            if (!s0)
                s0 = se = sl;
            else {
                se->next = sl;
                se = se->next;
            }
        }
    }
    CDol::destroy(c0);
    return (s0);
}


// Unset the expand flag of any expanded cells above the hierarchy level
// which overlap the BB.  Return a list of odescs with flag changed.
//
CDol *
PeekState::unexpand(BBox *BB, CDs *sdesc, WindowDesc *wdesc, int hierlev)
{
    if (Stack.TFull())
        return (0);

    CDol *se = 0, *s0 = 0;
    CDg gdesc;
    Stack.TInitGen(sdesc, CellLayer(), BB, &gdesc);
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        CDs *msdesc = cdesc->masterCell();
        if (!msdesc || msdesc->isDevice())
            continue;
        if (cdesc->has_flag(wdesc->DisplFlags()) ||
                hierlev < wdesc->Attrib()->expand_level(wdesc->Mode())) {

            Stack.TPush();
            bool subchanged = false;
            unsigned int x1, x2, y1, y2;
            if (Stack.TOverlapInst(cdesc, BB, &x1, &x2, &y1, &y2)) {
                // we really need to call the function over the area of the
                // cell once only
                if (y2 >= y1 + 2)
                    y2 = ++y1;
                if (x2 >= x1 + 2)
                    x2 = ++x1;

                CDap ap(cdesc);
                int tx, ty;
                Stack.TGetTrans(&tx, &ty);
                xyg_t xyg(x1, x2, y1, y2);
                do {
                    Stack.TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                    CDol *sl = unexpand(BB, msdesc, wdesc, hierlev+1);
                    if (sl) {
                        if (!s0)
                            s0 = se = sl;
                        else
                            se->next = sl;
                        while (se->next)
                            se = se->next;
                        subchanged = true;
                    }
                    Stack.TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            Stack.TPop();

            if (hierlev >= wdesc->Attrib()->expand_level(wdesc->Mode()) &&
                    !subchanged) {
                CDol *sl = new CDol(cdesc, 0);
                if (!s0)
                    s0 = se = sl;
                else {
                    se->next = sl;
                    se = se->next;
                }
            }
        }
    }
    return (s0);
}


// Return a box list of areas that need to be updated.
//
Blist *
PeekState::find_blist(CDs *sdesc, WindowDesc *wdesc, CDc *odesc, int hierlev)
{
    if (Stack.TFull())
        return (0);

    Blist *be = 0, *b0 = 0;
    BBox *BB = wdesc->Window();
    CDg gdesc;
    Stack.TInitGen(sdesc, CellLayer(), BB, &gdesc);
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        CDs *msdesc = cdesc->masterCell();
        if (!msdesc || msdesc->isDevice())
            continue;
        if (cdesc == odesc) {
            Blist *bl = new Blist;
            bl->BB = odesc->oBB();
            Stack.TBB(&bl->BB, 0);
            return (bl);
        }
        else if (cdesc->has_flag(wdesc->DisplFlags()) ||
                hierlev < wdesc->Attrib()->expand_level(wdesc->Mode())) {

            Stack.TPush();
            unsigned int x1, x2, y1, y2;
            if (Stack.TOverlapInst(cdesc, BB, &x1, &x2, &y1, &y2)) {
                // we really need to call the function over the area of the
                // cell once only
                if (y2 >= y1 + 2)
                    y2 = ++y1;
                if (x2 >= x1 + 2)
                    x2 = ++x1;

                CDap ap(cdesc);
                int tx, ty;
                Stack.TGetTrans(&tx, &ty);
                xyg_t xyg(x1, x2, y1, y2);
                do {
                    Stack.TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                    Blist *bl = find_blist(msdesc, wdesc, odesc, hierlev+1);
                    if (bl) {
                        if (!b0)
                            b0 = be = bl;
                        else
                            be->next = bl;
                        while (be->next)
                            be = be->next;
                    }
                    Stack.TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            Stack.TPop();
        }
    }
    return (b0);
}


// Redisplay the objects, and free the list.
//
void
PeekState::redisplay(WindowDesc *wdesc, CDol *sl)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    Blist *be = 0, *b0 = 0;
    CDol *sn;
    for (CDol *s = sl; s; s = sn) {
        sn = s->next;
        Blist *bl = find_blist(cursd, wdesc, (CDc*)s->odesc, 0);
        delete s;
        if (bl) {
            if (!b0)
                b0 = be = bl;
            else
                be->next = bl;
            while (be->next)
                be = be->next;
        }
    }
    if (b0) {
        b0 = Blist::merge(b0);
        for (Blist *bl = b0; bl; bl = be) {
            be = bl->next;
            wdesc->Redisplay(&bl->BB);
            delete bl;
        }
    }
}


// Static function.
// Take care of thinning out unwanted objects as well as setting the
// flags.  Arg exp is true when expanding.  Arg filtered is true if
// the instance filtering pop-up was shown.
//
void
PeekState::process_list(CDol *sl, BBox *BB, WindowDesc *wdesc, bool exp,
    bool filtered)
{
    if (!sl)
        return;
    if (BB->width() <= 10 && BB->height() <= 10 && !filtered) {
        // A "point" select that was not filtered, keep only the smallest
        // cell instance in list.

        double area =
            ((double)sl->odesc->oBB().width())*sl->odesc->oBB().height();
        CDol *sm = sl;
        for (CDol *s = sl->next; s; s = s->next) {
            double a =
                ((double)s->odesc->oBB().width())*s->odesc->oBB().height();
            if (a > 0.0 && a < area) {
                a = area;
                sm = s;
            }
        }
        sl->odesc = sm->odesc;
        CDol::destroy(sl->next);
        sl->next = 0;
    }
    if (exp) {
        for (CDol *s = sl; s; s = s->next)
            s->odesc->set_flag(wdesc->DisplFlags());
    }
    else {
        for (CDol *s = sl; s; s = s->next)
            s->odesc->unset_flag(wdesc->DisplFlags());
    }
}


// Static function.
bool
PeekState::exp_cb(const char *string, void *arg)
{
    if (!string) {
        if (RedisplayWin)
            RedisplayWin->Redisplay(0);
        RedisplayWin = 0;
        return (false);
    }
    CmdDesc *cmd = (CmdDesc*)arg;
    if (!cmd)
        return (false);
    WindowDesc *wdesc = cmd->wdesc;
    if (!wdesc)
        return (false);
    DSPattrib *a = wdesc->Attrib();
    if (!a)
        return (false);
    if (string && *string) {
        while (isspace(*string))
            string++;
        if (*string == 'p') {
            if (a->expand_level(wdesc->Mode()) >= 0 &&
                    wdesc == DSP()->MainWdesc()) {
                PeekCmd = new PeekState("PEEK", "xic:expnd#peek");
                PeekCmd->SetCaller(cmd->caller);
                if (!EV()->PushCallback(PeekCmd)) {
                    delete PeekCmd;
                    PeekCmd = 0;
                    goto again;
                }
                PeekCmd->message();
                return (false);
            }
            else
                goto again;
        }
        else if (!wdesc->Expand(string))
            goto again;
    }
    RedisplayWin = cmd->wdesc;
    return (false);

again:
    const char *cur;
    if (a->expand_level(wdesc->Mode()) < 0)
        cur = "none";
    else if (!a->expand_level(wdesc->Mode()))
        cur = "all";
    else
        cur = "+";
    if (wdesc->Wbag())
        wdesc->Wbag()->PopUpExpand(0, MODE_UPD, 0, 0, cur, false);
    return (true);
}

