
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
#include "scedif.h"
#include "drcif.h"
#include "extif.h"
#include "pushpop.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "events.h"
#include "menu.h"
#include "promptline.h"
#include "select.h"


ContextDesc::ContextDesc(const CDc *cdesc,
    unsigned int xind, unsigned int yind)
{
    cInst = cdesc;
    cIndX = xind;
    cIndY = yind;
    cParentDesc = CurCell();
    cState = EditIf()->newPPstate();

    for (int i = 0; i < DSP_NUMWINS; i++) {
        WindowDesc *wdesc = DSP()->Window(i);
        if (!wdesc)
            continue;
        if (wdesc->IsSimilar(DSP()->MainWdesc(), WDsimXmode)) {
            const BBox *BB = wdesc->Window();
            cWinState[i].width = BB->width();
            cWinState[i].x = (BB->left + BB->right)/2;
            cWinState[i].y = (BB->bottom + BB->top)/2;
            cWinState[i].sdesc = wdesc->CurCellDesc(wdesc->Mode());

            cWinViews[i] = *wdesc->Views();
            wdesc->Views()->zero();
        }
    }
    cNext = PP()->Context();
}


ContextDesc::~ContextDesc()
{
    ContextDesc *cdt = this;
    if (!cdt)
        return;
    delete cState;
    for (int i = 0; i < DSP_NUMWINS; i++)
       cWinViews[i].clear();
}


void
ContextDesc::purge(const CDs *sd, const CDl *ld)
{
    for (ContextDesc *cx = this; cx; cx = cx->cNext) {
        if (cx->cState)
            cx->cState->purge(sd, ld);
    }
}


ContextDesc *
ContextDesc::purge(const CDs *sd, const CDo *od)
{
    CDc *cd = (od->type() == CDINSTANCE ? (CDc*)od : 0);
    ContextDesc *c0 = this, *cp = 0;
    for (ContextDesc *cx = c0; cx; cx = cx->cNext) {
        if (cd && cd == cx->cInst) {
            // if we delete a cell, we don't want to pop back into it!
            if (cp)
                cp->cNext = 0;
            else
                c0 = 0;
            ContextDesc::clear(cx);
            return (c0);
        }
        if (cx->cState)
            cx->cState->purge(sd, od);
        cp = cx;
    }
    return (c0);
}


// Static function.
void
ContextDesc::clear(ContextDesc *c)
{
    while (c) {
        ContextDesc *cn = c->cNext;
        delete c;
        c = cn;
    }
}
// End of ContextDesc functions.


/**************************************************************************
 *
 * Push and pop, edit context manipulation functions.
 *
 **************************************************************************/


cPushPop *cPushPop::instancePtr = 0;

cPushPop::cPushPop()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cPushPop already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    context = 0;
    context_history = 0;
    no_display = false;

    setupInterface();
}


// Private static error exit.
//
void
cPushPop::on_null_ptr()
{
    fprintf(stderr, "Singleton class cPushPop used before instantiated.\n");
    exit(1);
}


// Push editing context to cdesc, saving the state in the Context
// list.  The indices apply when cdesc is arrayed.
//
// All drawing windows showing the current cell in the current mode
// will be switched to the pushed-to cell and redrawn.  Windows
// created while a push is active will not have context, i.e., the
// current and top cells will be the same for these windows, and they
// won't be touched in the subsequent pop.
//
void
cPushPop::PushContext(const CDc *cdesc, unsigned int xind, unsigned int yind)
{
    if (!CurCell() || CurCell()->isSymbolic())
        return;

    CDs *msdesc;
    cTfmStack stk;
    if (!cdesc || (context_history && cdesc == context_history->cInst &&
            context_history->cIndX == xind &&
            context_history->cIndY == yind)) {
        // push to the instance last popped from
        if (!context_history)
            return;
        if (context_history->cParentDesc != CurCell())
            return;
        ContextDesc *cx = context_history;
        context_history = context_history->cNext;
        cx->cNext = context;
        context = cx;
        cdesc = context->cInst;
        xind = context->cIndX;
        yind = context->cIndY;
        msdesc = cdesc->masterCell();
        if (!msdesc) {
            // shouldn't happen
            context = context->cNext;
            return;
        }
        cx = new ContextDesc(context->cInst, context->cIndX, context->cIndY);
        cx->cParentDesc = context->cParentDesc;
        cx->cNext = context->cNext;

        stk.TPush();
        stk.TLoad(CDtfRegI0);     // load the current inverse transform
        stk.TCurrent(&context->cTF);
        stk.TInverse();           // compute the inverse (actual transform)
        stk.TLoadInverse();       // load it

        stk.TPush();
        stk.TApplyTransform(cdesc);
        stk.TPremultiply();
        if (xind || yind) {
            CDap ap(cdesc);
            if (xind < ap.nx && yind < ap.ny)
                stk.TTransMult(xind*ap.dx, yind*ap.dy);
        }

        stk.TInverse();           // compute the inverse
        stk.TLoadInverse();       // load it
        stk.TStore(CDtfRegI0);    // save it for use in redisplay()
        stk.TPop();
        stk.TPop();

        cx->cTF = context->cTF;

        for (int i = 1; i < DSP_NUMWINS; i++) {
            WindowDesc *wdesc = DSP()->Window(i);
            if (wdesc && wdesc->IsSimilar(DSP()->MainWdesc()))
                wdesc->SetCurCellName(msdesc->cellname());
        }
        CDcbin cbin(msdesc);
        XM()->SetNewContext(&cbin, false);

        // Fix views and redisplay.
        for (int i = 0; i < DSP_NUMWINS; i++) {
            WindowDesc *wdesc = DSP()->Window(i);
            if (wdesc && wdesc->IsSimilar(DSP()->MainWdesc())) {
                if (wdesc->CurCellDesc(wdesc->Mode()) ==
                        context->cWinState[i].sdesc) {
                    wdesc->InitWindow(context->cWinState[i].x,
                        context->cWinState[i].y, context->cWinState[i].width);

                    wdesc->Views()->clear();
                    *wdesc->Views() = context->cWinViews[i];
                    context->cWinViews[i].zero();
                }
                else {
                    wdesc->CenterFullView();
                    wdesc->Views()->clear();
                    context->cWinViews[i].clear();
                }
                if (!no_display)
                    wdesc->Redisplay(0);
            }
        }
        if (!no_display && DSP()->ShowTerminals())
            DSP()->ShowTerminals(DISPLAY);

        if (context->cState)
            context->cState->rotate();

        delete context;
        context = cx;
    }
    else {
        msdesc = cdesc->masterCell();
        if (!msdesc)
            return;
        ContextDesc::clear(context_history);  // clear history
        context_history = 0;
        context = new ContextDesc(cdesc, xind, yind);

        // Find the inverse transform of the cell, used when displaying
        // context.

        stk.TPush();
        stk.TLoad(CDtfRegI0);     // load the current inverse transform
        stk.TCurrent(&context->cTF);
        stk.TInverse();           // compute the inverse (actual transform)
        stk.TLoadInverse();       // load it

        stk.TPush();
        stk.TApplyTransform(cdesc);
        stk.TPremultiply();
        if (xind || yind) {
            CDap ap(cdesc);
            if (xind < ap.nx && yind < ap.ny)
                stk.TTransMult(xind*ap.dx, yind*ap.dy);
        }

        stk.TInverse();           // compute the inverse
        stk.TLoadInverse();       // load it
        stk.TStore(CDtfRegI0);    // save it for use in redisplay()
        stk.TPop();
        stk.TPop();

        for (int i = 1; i < DSP_NUMWINS; i++) {
            WindowDesc *wdesc = DSP()->Window(i);
            if (wdesc && wdesc->IsSimilar(DSP()->MainWdesc()))
                wdesc->SetCurCellName(msdesc->cellname());
        }
        CDcbin cbin(msdesc);
        XM()->SetNewContext(&cbin, false);

        // Fix views and redisplay.
        for (int i = 0; i < DSP_NUMWINS; i++) {
            WindowDesc *wdesc = DSP()->Window(i);
            if (wdesc && wdesc->IsSimilar(DSP()->MainWdesc())) {
                wdesc->CenterFullView();
                wdesc->Views()->clear();
                if (!no_display)
                    wdesc->Redisplay(0);
            }
        }
        if (!no_display && DSP()->ShowTerminals())
            DSP()->ShowTerminals(DISPLAY);
    }
    if (!no_display) {
        PL()->ShowPromptV("Current cell is %s.", DSP()->CurCellName());
        DSP()->MainWdesc()->ShowTitle();
        DSP()->MainWdesc()->UpdateProxy();
        XM()->ShowParameters();
    }
}


// Pop the editing context up a level.  All windows showing the
// current cell in the current mode that have a different top cell
// will be updated and redrawn.
//
void
cPushPop::PopContext()
{
    if (!context) {
        if (!no_display)
            PL()->ShowPrompt("There isn't a subedit to pop from.");
        return;
    }

    // Save the current context.
    ContextDesc *cx =
        new ContextDesc(context->cInst, context->cIndX, context->cIndY);
    cx->cParentDesc = context->cParentDesc;
    cx->cTF = context->cTF;
    cx->cNext = context_history;
    context_history = cx;

    XM()->CommitCell();

    if (context->cParentDesc)
        context->cParentDesc->computeBB();

    for (int i = 1; i < DSP_NUMWINS; i++) {
        WindowDesc *wdesc = DSP()->Window(i);
        if (wdesc && wdesc->IsSimilar(DSP()->MainWdesc())) {
            if (wdesc->TopCellName() != wdesc->CurCellName())
                wdesc->SetCurCellName(context->cParentDesc->cellname());
        }
    }
    CDcbin cbin(context->cParentDesc);
    XM()->SetNewContext(&cbin, false);

    if (context->cState)
        context->cState->rotate();

    cTfmStack stk;
    stk.TPush();
    stk.TLoadCurrent(&context->cTF);
    stk.TStore(CDtfRegI0);
    stk.TPop();

    // Fix views and redisplay.
    for (int i = 0; i < DSP_NUMWINS; i++) {
        WindowDesc *wdesc = DSP()->Window(i);
        if (wdesc && wdesc->IsSimilar(DSP()->MainWdesc())) {
            if (wdesc->CurCellDesc(wdesc->Mode()) ==
                    context->cWinState[i].sdesc) {
                wdesc->InitWindow(context->cWinState[i].x,
                    context->cWinState[i].y, context->cWinState[i].width);
                wdesc->Views()->clear();
                *wdesc->Views() = context->cWinViews[i];
                context->cWinViews[i].zero();
            }
            else {
                wdesc->CenterFullView();
                wdesc->Views()->clear();
                context->cWinViews[i].clear();
            }
            if (!no_display)
                wdesc->Redisplay(0);
        }
    }

    if (DSP()->CurMode() == Electrical)
        ScedIf()->checkRepositionLabels(context->cInst);

    if (!no_display && DSP()->ShowTerminals())
        DSP()->ShowTerminals(DISPLAY);

    cx = context;
    context = context->cNext;
    delete cx;

    if (!no_display) {
        DSP()->MainWdesc()->ShowTitle();
        DSP()->MainWdesc()->UpdateProxy();
        XM()->ShowParameters();
        PL()->ShowPromptV("Current cell is %s.", DSP()->CurCellName());
    }
}


// Pop back up to the top of the hierarchy.  This is done before
// opening a new cell, or switching display modes.
//
void
cPushPop::ClearContext(bool skip_commit)
{
    no_display = true;
    while (context)
        PopContext();
    no_display = false;
    ContextDesc::clear(context_history);
    context_history = 0;
    if (!skip_commit)
        XM()->CommitCell();

    // No need to redraw the main window, but redraw draw the
    // sub-windows.
    for (int i = 1; i < DSP_NUMWINS; i++) {
        WindowDesc *wdesc = DSP()->Window(i);
        if (wdesc && wdesc->IsSimilar(DSP()->MainWdesc()))
            wdesc->Redisplay(0);
    }
}


void
cPushPop::ClearLists()
{
    ContextDesc::clear(context);
    context = 0;
    ContextDesc::clear(context_history);
    context_history = 0;
}


void
cPushPop::InvalidateLayer(const CDs *sd, const CDl *ld)
{
    context->purge(sd, ld);
    context_history->purge(sd, ld);
}


void
cPushPop::InvalidateObject(const CDs *sd, const CDo *od, bool)
{
    if (!sd)
        return;
    context = context->purge(sd, od);
    context_history = context_history->purge(sd, od);
}


CXstate *
cPushPop::PopState()
{
    CXstate *ecx = new CXstate(DSP()->CurCellName(), context, context_history);
    context = 0;
    context_history = 0;
    return (ecx);
}


void
cPushPop::PushState(CXstate *ecx)
{
    ContextDesc::clear(context);
    context = 0;
    ContextDesc::clear(context_history);
    context_history = 0;

    if (ecx) {
        if (ecx->cellname == DSP()->CurCellName()) {
            context = ecx->context;
            ecx->context = 0;
            context_history = ecx->context_history;
            ecx->context_history = 0;
        }
        delete ecx;
    }
}


namespace {
    // Return the instance that was pushed into.  Its cell will not be
    // rendered (showing context only).
    //
    const CDc *
    context_cell()
    {
        return (PP()->Instance());
    }
}


void
cPushPop::setupInterface()
{
    DSP()->Register_context_cell(context_cell);
}
// End of cPushPop functions


/*========================================================================*
 * The Push command
 *========================================================================*/

namespace {
    namespace main_pushpop {
        struct PushState : public CmdState
        {
            PushState(const char*, const char*);
            virtual ~PushState();

            void setCaller(GRobject c)  { Caller = c; }

            void b1down() { cEventHdlr::sel_b1down(); }
            void b1up();
            void esc();
            bool key(int, const char*, int);
            void undo() { cEventHdlr::sel_undo(); }
            void redo() { cEventHdlr::sel_redo(); }
            void message() { PL()->ShowPrompt(msg1); }

        private:
            GRobject Caller;
            char Types[4];
            BBox AOI;

            static const char *msg1;
        };

        PushState *PushCmd;
    }
}

using namespace main_pushpop;

const char *PushState::msg1 = "Click on instance to push to.";


// If the Info command is active, grab data necessary to push to the
// cell containing the selected object.  This has to be done before
// the Info command is exited when the Push command starts.
//
void
cMain::pushSetup()
{
    CDclxy::destroy(xm_push_data);
    xm_push_data = XM()->GetPushList();
}


// Menu command to push the editing context to a subcell.  If no
// subcell has been selected, the user is asked to select one.
//
void
cMain::pushExec(CmdDesc *cmd)
{
    if (PushCmd) {
        PushCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (!XM()->CheckCurCell(true, false, DSP()->CurMode()))
        return;
    CDs *cursd = CurCell();
    if (!cursd || !cursd->hasSubcells()) {
        PL()->ShowPrompt("Current cell contains no instances.");
        return;
    }

    // If the Info command was active, push to the cell containing the
    // selected object.  Otherwise, fetch the first instance desc in
    // select queue if any.  Its master is the cell we will push to.

    if (xm_push_data) {
        for (CDclxy *c = xm_push_data; c; c = c->next)
            PP()->PushContext(c->cdesc, c->xind, c->yind);
        CDclxy::destroy(xm_push_data);
        xm_push_data = 0;
        return;
    }

    // Note that if the cdesc is arrayed, we're selecting the 0,0
    // index when we do this.  If we start with no selected instance,
    // then we can select a particular array component.
    CDc *cdesc = (CDc*)Selections.firstObject(CurCell(), "c");
    if (cdesc) {
        PP()->PushContext(cdesc, 0, 0);
        return;
    }

    // Click to select an instance.
    PushCmd = new PushState("PUSH", "xic:push");
    PushCmd->setCaller(cmd ? cmd->caller : 0);
    if (!EV()->PushCallback(PushCmd)) {
        delete PushCmd;
        return;
    }
    PushCmd->message();
    ds.clear();
}


PushState::PushState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    Types[0] = CDINSTANCE;
    Types[1] = '\0';
    Types[2] = '\0';
    Types[3] = '\0';
}


PushState::~PushState()
{
    PushCmd = 0;
}


namespace {
    void
    ary_ix(const CDc *cdesc, int x, int y, unsigned int *ix, unsigned int *iy)
    {
        *ix = 0;
        *iy = 0;
        if (!cdesc)
            return;
        CDap ap(cdesc);
        if (ap.nx <= 1 && ap.ny <= 1)
            return;
        cTfmStack stk;
        stk.TPush();
        stk.TApplyTransform(cdesc);
        CDs *sd = cdesc->masterCell();
        if (!sd)
            return;
        const BBox *sBB = sd->BB();
        stk.TIndex(sBB, x, y, ap.nx, ap.dx, ap.ny, ap.dy, ix, iy, true);
    }
}


// Finish selection.  If success, do the push.
//
void
PushState::b1up()
{
    if (!cEventHdlr::sel_b1up(&AOI, Types, 0))
        return;
    CDc *cd = (CDc*)Selections.firstObject(CurCell(), "c");
    if (cd) {
        int x, y;
        EV()->Cursor().get_raw(&x, &y);
        unsigned int ix, iy;
        ary_ix(cd, x, y, &ix, &iy);

        PP()->PushContext(cd, ix, iy);
        EV()->PopCallback(this);
        Menu()->Deselect(Caller);
        delete this;
    }
}


bool
PushState::key(int code, const char*, int state)
{
    // press Enter to revisit cells recently popped from
    if (code == RETURN_KEY) {
        if (state & GR_SHIFT_MASK)
            PP()->PopContext();
        else
            PP()->PushContext(0, 0, 0);
        if (!(state & GR_CONTROL_MASK)) {
            // If Ctrl is also held, keep Push mode active
            EV()->PopCallback(this);
            Menu()->Deselect(Caller);
            delete this;
        }
        return (true);
    }
    return (false);
}


// Esc entered, clean up and exit.
//
void
PushState::esc()
{
    cEventHdlr::sel_esc();
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}
// End of PushState functions

