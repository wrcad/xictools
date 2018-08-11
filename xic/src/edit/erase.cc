
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
#include "yankbuf.h"
#include "scedif.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_layer.h"
#include "cd_lgen.h"
#include "geo_zlist.h"
#include "events.h"
#include "layertab.h"
#include "menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "ghost.h"


//
// Functions to delete objects and erase areas.
//

// Menu command to delete all selected objects.  Also called by the
// front end pointer handler when the Delete key is pressed.
//
void
cEdit::deleteExec(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!Selections.hasTypes(CurCell(), 0)) {
        PL()->ShowPrompt("You must first select objects to delete.");
        return;
    }
    if (noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, false, DSP()->CurMode()))
        return;

    Ulist()->ListCheck("delete", CurCell(), true);
    deleteQueue();
    if (Ulist()->HasChanged()) {
        if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
            // These might be deleted
            DSP()->ShowCellTerminalMarks(ERASE);
        Ulist()->CommitChanges(true);
        if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
            DSP()->ShowCellTerminalMarks(DISPLAY);
    }
    PL()->ShowPrompt("Selected objects have been deleted.");
}


void
cEdit::deleteQueue()
{
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    if (!Selections.hasTypes(cursd, 0))
        return;
    if (DSP()->CurMode() == Electrical)
        // First purge the linked labels from the queue.
        Selections.purgeLabels(cursd);

    // Now delete everything, and shrink the list.
    sSelGen sg(Selections, cursd, 0, false);
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (DSP()->CurMode() == Electrical) {
            if (od->type() == CDINSTANCE || od->type() == CDWIRE)
                cursd->prptyUnref((CDc*)od);
            else if (od->type() == CDLABEL)
                cursd->prptyLabelUpdate(0, (CDla*)od);
        }
        Ulist()->RecordObjectChange(cursd, od, 0);
        sg.remove();
    }
    XM()->ShowParameters();
}


void
cEdit::eraseUnder()
{
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    Zlist *zs0 = 0, *zse = 0;
    sSelGen sg(Selections, cursd);
    CDo *od;
    while ((od = sg.next()) != 0) {
        Zlist *zx = od->toZlist();
        if (!zs0)
            zs0 = zse = zx;
        else {
            while (zse->next)
                zse = zse->next;
            zse->next = zx;
        }
    }
    if (!zs0)
        return;

    if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals()) {
        // These might be deleted
        DSP()->ShowCellTerminalMarks(ERASE);
    }
    PolyList *ps = Zlist::to_poly_list(zs0);

    for (PolyList *pl = ps; pl; pl = pl->next) {

        BBox BB;
        pl->po.computeBB(&BB);

        Zlist *zl = pl->po.toZlist();
        if (!zl)
            continue;

        CDsLgen lgen(cursd);
        CDl *ld;
        while ((ld = lgen.next()) != 0) {
            // Only visible, selectable layers will be clipped.
            if (ld->isInvisible())
                continue;
            if (ld->isNoSelect())
                continue;

            CDg gdesc;
            gdesc.init_gen(cursd, ld, &BB);
            CDo *odesc;
            Zlist *z0 = 0;
            while ((odesc = gdesc.next()) != 0) {
                if (odesc->state() != CDobjVanilla)
                    continue;
                if (odesc->type() == CDLABEL)
                    continue;
                if (odesc->intersect(&pl->po, false)) {
                    Zlist *zo = odesc->toZlist();
                    Ulist()->RecordObjectChange(cursd, odesc, 0);
                    for (Zlist *z = zl; z; z = z->next) {
                        Zlist::zl_andnot(&zo, &z->Z);
                        if (!zo)
                            break;
                    }
                    if (zo) {
                        Zlist *ze = zo;
                        while (ze->next)
                            ze = ze->next;
                        ze->next = z0;
                        z0 = zo;
                    }
                }
            }
            if (z0) {
                PolyList *p0 = Zlist::to_poly_list(z0);
                for (PolyList *pp = p0; pp; pp = pp->next)
                    cursd->newPoly(0, &pp->po, ld, 0, true);
                PolyList::destroy(p0);
            }
        }
        Zlist::destroy(zl);
    }
    PolyList::destroy(ps);
    if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
        DSP()->ShowCellTerminalMarks(DISPLAY);
}


// The Erase command
//
namespace {
    // Return a list of objects found in the box defined by the
    // coordinates.  Also set BB to this box.
    //
    CDol *get_list(CDs *sdesc, int x1, int y1, int x2, int y2, BBox *BB)
    {
        char types[4];
        types[0] = CDBOX;
        types[1] = CDPOLYGON;
        types[2] = CDWIRE;
        types[3] = '\0';
        BB->left = mmMin(x1, x2);
        BB->bottom = mmMin(y1, y2);
        BB->right = mmMax(x1, x2);
        BB->top = mmMax(y1, y2);
        return (Selections.selectItems(sdesc, types, BB, PSELstrict_area_nt));
    }

    enum { Erase, Clip };

    namespace ed_erase {
        struct EraseState : public CmdState
        {
            EraseState(const char*, const char*);
            virtual ~EraseState();

            void setCaller(GRobject c)  { Caller = c; }

            void setup();
            void b1down();
            void b1up();
            void b1down_altw();
            void b1up_altw();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();

        private:
            void message();
            void SetLevel1(bool show) { Level = 1; if (show) message(); }
            void SetLevel2() { Level = 2; message(); }

            GRobject Caller;
            int State;
            int Refx;
            int Refy;
            int ECmode;
        };

        EraseState *EraseCmd;
    }
}

using namespace ed_erase;


// Menu command to erase a rectangular region.  In layer specific
// mode, only the current layer is erased.
//
void
cEdit::eraseExec(CmdDesc *cmd)
{
    if (EraseCmd) {
        EraseCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurCell(true, false, DSP()->CurMode()))
        return;

    EraseCmd = new EraseState("ERASE", "xic:erase");
    EraseCmd->setCaller(cmd ? cmd->caller : 0);
    Ulist()->ListCheck(EraseCmd->Name(), CurCell(), false);
    if (!EV()->PushCallback(EraseCmd)) {
        delete EraseCmd;
        return;
    }
    ds.clear();
    EraseCmd->setup();
}


EraseState::EraseState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    State = 0;
    Refx = Refy = 0;
    ECmode = Erase;
    DSP()->SetInEdgeSnappingCmd(true);

    SetLevel1(false);
}


EraseState::~EraseState()
{
    EraseCmd = 0;
    DSP()->SetInEdgeSnappingCmd(false);
}


void
EraseState::setup()
{
    if (Exported) {
        // Accept the exported coordinate as the anchor point, if given.
        Refx = ExportX;
        Refy = ExportY;
        State = 1;
        XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
        Gst()->SetGhostAt(GFbox, Refx, Refy);
        SetLevel2();
        Exported = false;
    }
    else
        message();
}


void
EraseState::b1down()
{
    if (Level == 1) {
        // Set the anchor point, display a ghost rectangle.
        //
        EV()->Cursor().get_xy(&Refx, &Refy);
        State = 0;
        if ((EV()->Cursor().get_downstate() & GR_CONTROL_MASK) &&
                DSP()->CurMode() == Physical) {
            BBox AOI;
            AOI.left = AOI.right = Refx;
            AOI.bottom = AOI.top = Refy;
            char types[2];
            types[0] = CDINSTANCE;
            types[1] = '\0';
            CDol *slist = Selections.selectItems(CurCell(), types, &AOI,
                PSELpoint);
            if (!slist)
                return;
            CDol *slp = slist;
            if (slist->next) {
                // find the entry with least area
                CDol *sl = slist;
                double amin = ((double)sl->odesc->oBB().width())*
                    sl->odesc->oBB().height();
                for (sl = sl->next; sl; sl = sl->next) {
                    double a = ((double)sl->odesc->oBB().width())*
                        sl->odesc->oBB().height();
                    if (a < amin) {
                        amin = a;
                        slp = sl;
                    }
                }
            }
            Refx = slp->odesc->oBB().left;
            Refy = slp->odesc->oBB().bottom;
            AOI = slp->odesc->oBB();
            CDol::destroy(slist);

            slist = get_list(CurCell(),
                AOI.left, AOI.bottom, AOI.right, AOI.top, &AOI);
            if (slist) {
                if (ECmode == Erase)
                    ED()->yank(slist, &AOI, false);
                else
                    ED()->yank(slist, &AOI, true);
                if (!(EV()->Cursor().get_downstate() & GR_SHIFT_MASK)) {
                    if (ECmode == Erase) {
                        if (ED()->eraseList(slist, &AOI))
                            Ulist()->CommitChanges(true);
                    }
                    else {
                        if (ED()->clipList(slist, &AOI))
                            Ulist()->CommitChanges(true);
                    }
                }
                else
                    PL()->FlashMessage("Yanked...");
                CDol::destroy(slist);
            }
            return;
        }
        State = 1;
        XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
        Gst()->SetGhostAt(GFbox, Refx, Refy);
        EV()->DownTimer(GFbox);
    }
    else {
        // Do the erase, if the rectangle has area.
        //
        int xc, yc;
        EV()->Cursor().get_xy(&xc, &yc);
        if (xc != Refx && yc != Refy) {
            BBox AOI;
            CDol *slist = get_list(CurCell(), xc, yc, Refx, Refy, &AOI);
            if (slist) {
                if (ECmode == Erase)
                    ED()->yank(slist, &AOI, false);
                else
                    ED()->yank(slist, &AOI, true);
                if (!(EV()->Cursor().get_downstate() & GR_SHIFT_MASK)) {
                    if (ECmode == Erase) {
                        if (ED()->eraseList(slist, &AOI)) {
                            Gst()->SetGhost(GFnone);
                            XM()->SetCoordMode(CO_ABSOLUTE);
                            Ulist()->CommitChanges(true);
                            State = 3;
                            CDol::destroy(slist);
                            return;
                        }
                    }
                    else {
                        if (ED()->clipList(slist, &AOI)) {
                            Gst()->SetGhost(GFnone);
                            XM()->SetCoordMode(CO_ABSOLUTE);
                            Ulist()->CommitChanges(true);
                            State = 3;
                            CDol::destroy(slist);
                            return;
                        }
                    }
                }
                else if (DSP()->CurMode() == Physical) {
                    Gst()->SetGhost(GFnone);
                    XM()->SetCoordMode(CO_ABSOLUTE);
                    PL()->FlashMessage("Yanked...");
                    State = 4;
                    CDol::destroy(slist);
                    return;
                }
                CDol::destroy(slist);
            }
        }
    }
}


void
EraseState::b1up()
{
    if (Level == 1) {
        // Do the erase if the pointer has moved and the rectangle has area.
        // Otherwise set state for next button 1 press.
        //
        if (!State)
            return;
        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok() &&
                EV()->CurrentWin()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            if (x != Refx && y != Refy) {
                BBox BB;
                CDol *slist = get_list(CurCell(), x, y, Refx, Refy, &BB);
                if (slist) {
                    if (ECmode == Erase)
                        ED()->yank(slist, &BB, false);
                    else
                        ED()->yank(slist, &BB, true);
                    if (!(EV()->Cursor().get_upstate() & GR_SHIFT_MASK)) {
                        if (ECmode == Erase) {
                            if (ED()->eraseList(slist, &BB)) {
                                Gst()->SetGhost(GFnone);
                                XM()->SetCoordMode(CO_ABSOLUTE);
                                Ulist()->CommitChanges(true);
                                State = 2;
                                CDol::destroy(slist);
                                return;
                            }
                        }
                        else {
                            if (ED()->clipList(slist, &BB)) {
                                Gst()->SetGhost(GFnone);
                                XM()->SetCoordMode(CO_ABSOLUTE);
                                Ulist()->CommitChanges(true);
                                State = 2;
                                CDol::destroy(slist);
                                return;
                            }
                        }
                    }
                    else if (DSP()->CurMode() == Physical) {
                        Gst()->SetGhost(GFnone);
                        XM()->SetCoordMode(CO_ABSOLUTE);
                        PL()->FlashMessage("Yanked...");
                        CDol::destroy(slist);
                        return;
                    }
                    CDol::destroy(slist);
                }
            }
        }
        SetLevel2();
    }
    else {
        // Reset the state for next erase, if erase was performed.
        //
        if (State == 3) {
            State = 2;
            SetLevel1(true);
        }
        if (State == 4) {
            State = 1;
            SetLevel1(true);
        }
    }
}


// Handle the alt-window button presses so that objects in windows
// which are showing another cell can be yanked.

void
EraseState::b1down_altw()
{
    if (DSP()->CurMode() == Electrical)
        return;
    if (!DSP()->MainWdesc()->IsSimilar(EV()->ButtonWin(true), WDsimXcell))
        return;
    WindowDesc *wd = EV()->ButtonWin(true);
    // This is the alt window.
    if (!wd)
        return;
    CDs *sdesc = wd->CurCellDesc(wd->Mode());
    if (!sdesc)
        return;

    if (Level == 1) {
        // Set the anchor point, display a ghost rectangle.
        //
        EV()->Cursor().get_alt_down(&Refx, &Refy);
        EV()->CurrentWin()->Snap(&Refx, &Refy);
        State = 0;
        if ((EV()->Cursor().get_alt_downstate() & GR_CONTROL_MASK) &&
                DSP()->CurMode() == Physical) {

            BBox AOI;
            AOI.left = AOI.right = Refx;
            AOI.bottom = AOI.top = Refy;
            char types[2];
            types[0] = CDINSTANCE;
            types[1] = '\0';
            CDol *slist = Selections.selectItems(sdesc, types, &AOI,
                PSELpoint);
            if (!slist)
                return;
            CDol *slp = slist;
            if (slist->next) {
                // find the entry with least area
                CDol *sl = slist;
                double amin = ((double)sl->odesc->oBB().width())*
                    sl->odesc->oBB().height();
                for (sl = sl->next; sl; sl = sl->next) {
                    double a = ((double)sl->odesc->oBB().width())*
                        sl->odesc->oBB().height();
                    if (a < amin) {
                        amin = a;
                        slp = sl;
                    }
                }
            }
            Refx = slp->odesc->oBB().left;
            Refy = slp->odesc->oBB().bottom;
            AOI = slp->odesc->oBB();
            CDol::destroy(slist);

            slist = get_list(sdesc,
                AOI.left, AOI.bottom, AOI.right, AOI.top, &AOI);
            if (slist) {
                if (ECmode == Erase)
                    ED()->yank(slist, &AOI, false);
                else
                    ED()->yank(slist, &AOI, true);
                PL()->FlashMessage("Yanked...");
                CDol::destroy(slist);
            }
            return;
        }
        State = 1;
        XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
        Gst()->SetGhostAt(GFbox, Refx, Refy, GhostAltOnly);
        EV()->DownTimer(GFbox);
    }
    else {
        // Do the yank, if the rectangle has area.
        //
        int xc, yc;
        EV()->Cursor().get_alt_down(&xc, &yc);
        EV()->CurrentWin()->Snap(&xc, &xc);
        if (xc != Refx && yc != Refy) {
            BBox AOI;
            CDol *slist = get_list(sdesc, xc, yc, Refx, Refy, &AOI);
            if (slist) {
                if (ECmode == Erase)
                    ED()->yank(slist, &AOI, false);
                else
                    ED()->yank(slist, &AOI, true);
                Gst()->SetGhost(GFnone);
                XM()->SetCoordMode(CO_ABSOLUTE);
                PL()->FlashMessage("Yanked...");
                State = 4;
                CDol::destroy(slist);
            }
        }
    }
}


void
EraseState::b1up_altw()
{
    if (DSP()->CurMode() == Electrical)
        return;
    if (!DSP()->MainWdesc()->IsSimilar(EV()->ButtonWin(true), WDsimXcell))
        return;
    WindowDesc *wd = EV()->ButtonWin(true);
    // This is the alt window.
    if (!wd)
        return;
    CDs *sdesc = wd->CurCellDesc(wd->Mode());
    if (!sdesc)
        return;

    if (Level == 1) {
        // Do the yank if the pointer has moved and the rectangle has area.
        // Otherwise set state for next button 1 press.
        //
        if (!State)
            return;
        if (!EV()->UpTimer() && EV()->CurrentWin() &&
                EV()->Cursor().get_press_alt()) {
            int x, y;
            EV()->Cursor().get_alt_up(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            if (x != Refx && y != Refy) {
                BBox BB;

                CDol *slist = get_list(sdesc, x, y, Refx, Refy, &BB);
                if (slist) {
                    if (ECmode == Erase)
                        ED()->yank(slist, &BB, false);
                    else
                        ED()->yank(slist, &BB, true);
                    Gst()->SetGhost(GFnone);
                    XM()->SetCoordMode(CO_ABSOLUTE);
                    PL()->FlashMessage("Yanked...");
                    CDol::destroy(slist);
                    return;
                }
            }
        }
        SetLevel2();
    }
    else {
        // Reset the state for next yank, if yank was performed.
        //
        if (State == 3) {
            State = 2;
            SetLevel1(true);
        }
        if (State == 4) {
            State = 1;
            SetLevel1(true);
        }
    }
}


// Esc entered, clean up and exit.
//
void
EraseState::esc()
{
    if (EnableExport && Level == 2) {
        // Export the last anchor point, the next command may use this.
        Exported = true;
        ExportX = Refx;
        ExportY = Refy;
    }

    Gst()->SetGhost(GFnone);
    XM()->SetCoordMode(CO_ABSOLUTE);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


// If Shift pressed, don't erase, but add geometry that would have
// been erased to the yank/put buffer.  If Control pressed and pointing
// at a subcell, erase over the subcell's bounding box
//
bool
EraseState::key(int code, const char *text, int)
{
    switch (code) {
    case SHIFTDN_KEY:
        if (DSP()->CurMode() == Physical)
            PL()->ShowPrompt("Yank geometry, no deletion.");
        break;
    case SHIFTUP_KEY:
        if (DSP()->CurMode() == Physical) {
            // Avoid pop-up.
            if (!EV()->InCoordEntry())
                message();
        }
        break;
    case CTRLDN_KEY:
        if (DSP()->CurMode() == Physical && Level == 1)
            PL()->ShowPrompt("Use subcell boundary as template.");
        break;
    case CTRLUP_KEY:
        if (DSP()->CurMode() == Physical && Level == 1) {
            // Avoid pop-up.
            if (!EV()->InCoordEntry())
                message();
        }
        break;
    default:
        if (*text == ' ') {
            // Space bar, toggle erase/clip mode.
            if (ECmode == Erase)
                ECmode = Clip;
            else
                ECmode = Erase;
            message();
        }
        else
            return (false);
    }
    return (true);
}


void
EraseState::undo()
{
    if (Level == 1) {
        // Undo last operation.
        //
        Ulist()->UndoOperation();
        if (State == 2) {
            XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
            Gst()->SetGhostAt(GFbox, Refx, Refy);
            SetLevel2();
        }
    }
    else {
        // Undo anchor point.
        //
        Gst()->SetGhost(GFnone);
        XM()->SetCoordMode(CO_ABSOLUTE);
        if (State == 2)
            State = 3;
        SetLevel1(true);
    }
}


void
EraseState::redo()
{
    if (Level == 1) {
        // Redo undone anchor location or operation.
        //
        if ((State == 1 && !Ulist()->HasRedo()) ||
                (State == 3 && Ulist()->HasOneRedo())) {
            XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
            Gst()->SetGhostAt(GFbox, Refx, Refy);
            if (State == 3)
                State = 2;
            SetLevel2();
        }
        else
            Ulist()->RedoOperation();
    }
    else {
        // Redo the last undo.
        //
        if (State == 2) {
            Gst()->SetGhost(GFnone);
            XM()->SetCoordMode(CO_ABSOLUTE);
            Ulist()->RedoOperation();
            SetLevel1(true);
        }
    }
}


void
EraseState::message()
{
    if (Level == 1)
        PL()->ShowPromptV("%s mode (SpaceBar toggles), click twice or drag to "
            "define template rectangle.", ECmode == ERASE ? "ERASE" : "CLIP");
    else
        PL()->ShowPromptV("%s mode (SpaceBar toggles), click on second "
            "endpoint.", ECmode == ERASE ? "ERASE" : "CLIP");
}
// End of EraseState functions.


// Erase the objects in slist in BB.  Return true is anything was
// changed.
//
bool
cEdit::eraseList(CDol *slist, BBox *BB)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);
    bool erased = false;
    CDmergeInhibit inh(slist);
    for (CDol *sl = slist; sl; sl = sl->next) {
        // ignore selected objects
        if (!sl->odesc->is_normal() || sl->odesc->state() == CDobjSelected)
            continue;
        if (erase(sl->odesc, cursd, BB))
            erased = true;
    }
    return (erased);
}

                                
// Clip the objects in slist to BB.  Return true is anything was
// changed.
//
bool
cEdit::clipList(CDol *slist, BBox *BB)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);
    bool clipped = false;
    CDmergeInhibit inh(slist);
    for (CDol *sl = slist; sl; sl = sl->next) {
        // ignore selected objects
        if (!sl->odesc->is_normal() || sl->odesc->state() == CDobjSelected)
            continue;
        if (sl->odesc->oBB() <= *BB)
            continue;
        if (clip(sl->odesc, cursd, BB))
            clipped = true;
    }
    return (clipped);
}


// Perform an erase operation.  Boxes, polygons and wires only.
// External function for user function interface.  Return true if
// anything was changed (yank_only false) or yanked (yank_only
// true).
//
bool
cEdit::eraseArea(bool yank_only, int x1, int y1, int x2, int y2)
{
    if (!DSP()->CurCellName())
        return (false);
    bool ret = false;
    BBox BB;
    CDol *slist = get_list(CurCell(), x1, y1, x2, y2, &BB);
    if (slist) {
        yank(slist, &BB, false);
        if (!yank_only)
            ret = eraseList(slist, &BB);
        else
            ret = true;
        CDol::destroy(slist);
    }
    return (ret);
}


//
// Code to handle yank/put.
//


// The Put command
namespace {
    namespace ed_erase {
        struct PutState : public CmdState
        {
            friend void cEdit::putExec(CmdDesc*);

            PutState(const char*, const char*);
            virtual ~PutState();

            void setCaller(GRobject c)  { Caller = c; }

            int index() { return (Index); }

        private:
            void b1down() { GotBtn1 = true; Gst()->SetGhost(GFnone); }
            void b1up();
            void esc();
            bool key(int, const char*, int);

            GRobject Caller;
            int Index;
            bool GotBtn1;

            static const char *pmsg;
        };

        PutState *PutCmd;
    }
}

const char *PutState::pmsg =
    "Click to place yank buffer %d contents.  Arrow keys cycle yank buffers.";


// The put command.  Place the contents of the yank buffer where the
// user points.
//
void
cEdit::putExec(CmdDesc *cmd)
{
    if (PutCmd) {
        PutCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;

    if (!ed_yank_buffer[0]) {
        PL()->ShowPrompt("Yank buffer is empty.");
        return;
    }
    PutCmd = new PutState("PUT", "xic:put");
    PutCmd->setCaller(cmd ? cmd->caller : 0);
    if (!EV()->PushCallback(PutCmd)) {
        delete PutCmd;
        return;
    }
    Gst()->SetGhost(GFput);
    PL()->ShowPromptV(PutState::pmsg, 0);
    ds.clear();
}


PutState::PutState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    Index = 0;
    GotBtn1 = false;
}


PutState::~PutState()
{
    PutCmd = 0;
}


void
PutState::b1up()
{
    if (GotBtn1) {
        GotBtn1 = false;
        if (EV()->Cursor().is_release_ok()) {
            CDs *cursd = CurCell();
            if (cursd) {
                Ulist()->ListCheck(StateName, cursd, true);
                int x, y;
                EV()->Cursor().get_xy(&x, &y);
                ED()->put(x, y, Index);
                Ulist()->CommitChanges(true);
            }
        }
        Gst()->SetGhost(GFput);
    }
}


void
PutState::esc()
{
    Gst()->SetGhost(GFnone);
    EV()->PopCallback(this);
    PL()->ErasePrompt();
    Menu()->Deselect(Caller);
    delete this;
}


bool
PutState::key(int code, const char*, int mstate)
{
    switch (code) {
    case LEFT_KEY:
    case DOWN_KEY:
        if (mstate & (GR_SHIFT_MASK | GR_CONTROL_MASK))
            break;
        if (Index) {
            Gst()->SetGhost(GFnone);
            Index--;
            Gst()->SetGhost(GFput);
            PL()->ShowPromptV(pmsg, Index);
        }
        return (true);
    case RIGHT_KEY:
    case UP_KEY:
        if (mstate & (GR_SHIFT_MASK | GR_CONTROL_MASK))
            break;
        if (Index < ED_YANK_DEPTH - 1 && ED()->yankBuffer()[Index + 1]) {
            Gst()->SetGhost(GFnone);
            Index++;
            Gst()->SetGhost(GFput);
            PL()->ShowPromptV(pmsg, Index);
        }
        return (true);
    }
    return (false);
}
// End of PutState functions.


// Actually create the new objects from the yank buffer list.
//
void
cEdit::put(int x, int y, int buffer)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    CDtf tfold;
    cTfmStack stk;
    stk.TCurrent(&tfold);
    stk.TPush();
    GEO()->applyCurTransform(&stk, 0, 0, x, y);
    stk.TPremultiply();

    Errs()->init_error();
    CDtf tftmp;
    stk.TCurrent(&tftmp);
    for (yb *yx = ed_yank_buffer[buffer]; yx; yx = yx->next) {
        if (yx->type == CDBOX) {
            Poly poly;
            BBox BB = ((yb_b*)yx)->BB;
            stk.TBB(&BB, &poly.points);
            if (poly.points) {
                poly.numpts = 5;
                CDpo *newo = cursd->newPoly(0, &poly, yx->ldesc, 0, false);
                if (!newo) {
                    Errs()->add_error("newPoly failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
                else {
                    stk.TLoadCurrent(&tfold);
                    if (!cursd->mergeBoxOrPoly(newo, true)) {
                        Errs()->add_error("mergeBoxOrPoly failed");
                        Log()->ErrorLog(mh::ObjectCreation,
                            Errs()->get_error());
                    }
                    stk.TLoadCurrent(&tftmp);
                }
            }
            else {
                CDo *newo = cursd->newBox(0, &BB, yx->ldesc, 0);
                if (!newo) {
                    Errs()->add_error("newBox failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
                else {
                    stk.TLoadCurrent(&tfold);
                    if (!cursd->mergeBoxOrPoly(newo, true)) {
                        Errs()->add_error("mergeBoxOrPoly failed");
                        Log()->ErrorLog(mh::ObjectCreation,
                            Errs()->get_error());
                    }
                    stk.TLoadCurrent(&tftmp);
                }
            }
        }
        else if (yx->type == CDPOLYGON) {
            Poly poly = ((yb_p*)yx)->poly;
            poly.points = Point::dup_with_xform(poly.points, &stk, poly.numpts);
            CDpo *newo = cursd->newPoly(0, &poly, yx->ldesc, 0, false);
            if (!newo) {
                Errs()->add_error("newPoly failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
            else {
                stk.TLoadCurrent(&tfold);
                if (!cursd->mergeBoxOrPoly(newo, true)) {
                    Errs()->add_error("mergeBoxOrPoly failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
                stk.TLoadCurrent(&tftmp);
            }
        }
        else if (yx->type == CDWIRE) {
            Wire wire = ((yb_w*)yx)->wire;
            wire.points = Point::dup_with_xform(wire.points, &stk, wire.numpts);
            CDo *newo = cursd->newWire(0, &wire, yx->ldesc, 0, false);
            if (!newo) {
                Errs()->add_error("newWire failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
            else {
                stk.TLoadCurrent(&tfold);
                if (!cursd->mergeWire(OWIRE(newo), true)) {
                    Errs()->add_error("mergeWire failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
                stk.TLoadCurrent(&tftmp);
            }
        }
    }
    stk.TPop();
}


// Yank the parts of the objects in slist that overlap AOI into the
// buffer.
//
void
cEdit::yank(CDol *slist, BBox *AOI, bool clipping)
{
    yb *y0 = 0;
    for (CDol *sl = slist; sl; sl = sl->next) {
        if (!sl->odesc->is_normal() || sl->odesc->state() == CDobjSelected)
            continue;
        yb *yx = add_yank(sl->odesc, AOI, y0, clipping);
        if (yx)
            y0 = yx;
    }
    if (y0) {
        yb::destroy(ed_yank_buffer[ED_YANK_DEPTH-1]);
        for (int i = ED_YANK_DEPTH-1; i > 0; i--)
            ed_yank_buffer[i] = ed_yank_buffer[i-1];
        ed_yank_buffer[0] = y0;
    }
}


// For debugging, don't join poly list.
//#define DEBUG_NO_JOIN

#define C_LEFT   1
#define C_BOTTOM 2
#define C_RIGHT  4
#define C_TOP    8

namespace {
    // Return a code indicating which side(s) of the clipping BB the point
    // lies on
    //
    inline int
    onbb(Point *p, BBox *BB)
    {
        int code = 0;
        if (p->x == BB->left)
            code |= C_LEFT;
        else if (p->x == BB->right)
            code |= C_RIGHT;
        if (p->y == BB->bottom)
            code |= C_BOTTOM;
        else if (p->y == BB->top)
            code |= C_TOP;
        return (code);
    }


    // This is for correcting the end vertices of manhattan wires with
    // projecting end styles after clipping.  The vertex is moved back
    // from the edge by width/2.  If true is returned, the end vertex
    // should be removed
    //
    bool
    fix_end(Point *pe, Point *pn, int width, BBox *BB, bool outside)
    {
        int code = onbb(pe, BB);
        if (code) {
            if (pe->x == pn->x) {
                if (pn->y > pe->y &&
                        (code & (outside ? C_TOP : C_BOTTOM))) {
                    if (pn->y - pe->y > width/2)
                        pe->y += width/2;
                    else
                        return (true);
                }
                else if (pn->y < pe->y &&
                        (code & (outside ? C_BOTTOM : C_TOP))) {
                    if (pe->y - pn->y > width/2)
                        pe->y -= width/2;
                    else
                        return (true);
                }
            }
            else if (pe->y == pn->y) {
                if (pn->x > pe->x &&
                        (code & (outside ? C_RIGHT : C_LEFT))) {
                    if (pn->x - pe->x > width/2)
                        pe->x += width/2;
                    else
                        return (true);
                }
                else if (pn->x < pe->x &&
                        (code & (outside ? C_LEFT : C_RIGHT))) {
                    if (pe->x - pn->x > width/2)
                        pe->x -= width/2;
                    else
                        return (true);
                }
            }
        }
        return (false);
    }
}


bool
cEdit::clip(CDo *odesc, CDs *sdesc, BBox *AOI)
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
        Ulist()->RecordObjectChange(sdesc, odesc, 0);
        Blist *b0 = odesc->oBB().clip_to(AOI);
        if (b0) {
            Errs()->init_error();
            CDo *newo = sdesc->newBox(0, &b0->BB, odesc->ldesc(),
                odesc->prpty_list());
            if (!newo) {
                Errs()->add_error("newBox failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
            else if (!sdesc->mergeBoxOrPoly(newo, true)) {
                Errs()->add_error("mergeBoxOrPoly failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
            Blist::destroy(b0);
        }
        return (true);
    }
poly:
    {
        Ulist()->RecordObjectChange(sdesc, odesc, 0);
        Errs()->init_error();
        Zlist *zl = odesc->toZlist();
        Zoid Z(AOI);
        Zlist::zl_and(&zl, &Z);
#ifdef DEBUG_NO_JOIN
        for (Zlist *z = zl; z; z = z->next) {
            Poly po;
            if (z->Z.mkpoly(&po.points, &po.numpts, false)) {
                CDpo *newo = sdesc->newPoly(0, &po, odesc->ldesc(),
                    odesc->prpty_list(), false);
                if (!newo) {
                    Errs()->add_error("newPoly failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
                else if (!sdesc->mergeBoxOrPoly(newo, true)) {
                    Errs()->add_error("mergeBoxOrPoly failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
            }
        }
#else
        PolyList *p0 = Zlist::to_poly_list(zl);
        for (PolyList *p = p0; p; p = p->next) {
            CDpo *newo = sdesc->newPoly(0, &p->po, odesc->ldesc(),
                odesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newPoly failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
            else if (!sdesc->mergeBoxOrPoly(newo, true)) {
                Errs()->add_error("mergeBoxOrPoly failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
        }
        PolyList::destroy(p0);
#endif
        return (true);
    }
wire:
    {
        Ulist()->RecordObjectChange(sdesc, odesc, 0);
        PolyList *p0 = ((const CDw*)odesc)->w_clip(AOI, false);
        for (PolyList *p = p0; p; p = p->next) {
            Wire wire;
            wire.points = p->po.points;
            p->po.points = 0;
            wire.numpts = p->po.numpts;
            wire.attributes = ((CDw*)odesc)->attributes();
            if (wire.wire_style() != CDWIRE_FLUSH) {
                if (wire.numpts == 1) {
                    BBox BB(wire.points[0].x, wire.points[0].y,
                        wire.points[0].x, wire.points[0].y);
                    BB.bloat(wire.wire_width()/2);
                    if (!(BB <= *AOI)) {
                        delete [] wire.points;
                        continue;
                    }
                }
                else {
                    if (fix_end(wire.points, wire.points+1, wire.wire_width(),
                            AOI, false)) {
                        wire.numpts--;
                        Point *npts = new Point[wire.numpts];
                        memcpy(npts, wire.points+1,
                            wire.numpts*sizeof(Point));
                        delete [] wire.points;
                        wire.points = npts;
                    }
                    if (wire.numpts == 1) {
                        BBox BB(wire.points[0].x, wire.points[0].y,
                            wire.points[0].x, wire.points[0].y);
                        BB.bloat(wire.wire_width()/2);
                        if (!(BB <= *AOI)) {
                            delete [] wire.points;
                            continue;
                        }
                    }
                    else {
                        if (fix_end(wire.points + wire.numpts - 1,
                                wire.points + wire.numpts - 2,
                                wire.wire_width(), AOI, false)) {
                            wire.numpts--;
                            Point *npts = new Point[wire.numpts];
                            memcpy(npts, wire.points,
                                wire.numpts*sizeof(Point));
                            delete [] wire.points;
                            wire.points = npts;
                        }
                    }
                }
            }
            Errs()->init_error();
            CDw *newo = sdesc->newWire(0, &wire, odesc->ldesc(),
                odesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newWire failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
        }
        PolyList::destroy(p0);
        return (true);
    }
label:
inst:
    return (false);
}


bool
cEdit::erase(CDo *odesc, CDs *sdesc, BBox *AOI)
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
        Ulist()->RecordObjectChange(sdesc, odesc, 0);
        Blist *b0 = odesc->oBB().clip_out(AOI);
        for (Blist *b = b0; b; b = b->next) {
            Errs()->init_error();
            CDo *newo = sdesc->newBox(0, b->BB.left, b->BB.bottom,
                b->BB.right, b->BB.top, odesc->ldesc(), odesc->prpty_list());
            if (!newo) {
                Errs()->add_error("newBox failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
            else if (!sdesc->mergeBoxOrPoly(newo, true)) {
                Errs()->add_error("mergeBoxOrPoly failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
        }
        Blist::destroy(b0);
        return (true);
    }
poly:
    {
        Ulist()->RecordObjectChange(sdesc, odesc, 0);
        Errs()->init_error();
        Zlist *zl = odesc->toZlist();
        Zoid Z(AOI);
        Zlist::zl_andnot(&zl, &Z);
#ifdef DEBUG_NO_JOIN
        for (Zlist *z = zl; z; z = z->next) {
            Poly po;
            if (z->Z.mkpoly(&po.points, &po.numpts, false)) {
                CDpo *newo = sdesc->newPoly(0, &po, odesc->ldesc(),
                        odesc->prpty_list(), false);
                if (!newo) {
                    Errs()->add_error("newPoly failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
                else if (!sdesc->mergeBoxOrPoly(newo, true)) {
                    Errs()->add_error("mergeBoxOrPoly failed");
                    Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
                }
            }
        }
#else
        PolyList *p0 = Zlist::to_poly_list(zl);
        for (PolyList *p = p0; p; p = p->next) {
            CDpo *newo = sdesc->newPoly(0, &p->po, odesc->ldesc(),
                    odesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newPoly failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
            else if (!sdesc->mergeBoxOrPoly(newo, true)) {
                Errs()->add_error("mergeBoxOrPoly failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
        }
        PolyList::destroy(p0);
#endif
        return (true);
    }
wire:
    {
        Ulist()->RecordObjectChange(sdesc, odesc, 0);
        PolyList *p0 = ((const CDw*)odesc)->w_clip(AOI, true);
        for (PolyList *p = p0; p; p = p->next) {
            Wire wire;
            wire.points = p->po.points;
            p->po.points = 0;
            wire.numpts = p->po.numpts;
            wire.attributes = ((CDw*)odesc)->attributes();
            if (wire.wire_style() != CDWIRE_FLUSH) {
                if (wire.numpts == 1) {
                    BBox BB(wire.points[0].x, wire.points[0].y,
                        wire.points[0].x, wire.points[0].y);
                    BB.bloat(wire.wire_width()/2);
                    if (BB.intersect(AOI, false)) {
                        delete [] wire.points;
                        continue;
                    }
                }
                else {
                    if (fix_end(wire.points, wire.points+1, wire.wire_width(),
                            AOI, true)) {
                        wire.numpts--;
                        Point *npts = new Point[wire.numpts];
                        memcpy(npts, wire.points+1,
                            wire.numpts*sizeof(Point));
                        delete [] wire.points;
                        wire.points = npts;
                    }
                    if (wire.numpts == 1) {
                        BBox BB(wire.points[0].x, wire.points[0].y,
                            wire.points[0].x, wire.points[0].y);
                        BB.bloat(wire.wire_width()/2);
                        if (BB.intersect(AOI, false)) {
                            delete [] wire.points;
                            continue;
                        }
                    }
                    else {
                        if (fix_end(wire.points + wire.numpts - 1,
                                wire.points + wire.numpts - 2,
                                wire.wire_width(), AOI, true)) {
                            wire.numpts--;
                            Point *npts = new Point[wire.numpts];
                            memcpy(npts, wire.points,
                                wire.numpts*sizeof(Point));
                            delete [] wire.points;
                            wire.points = npts;
                        }
                    }
                }
            }
            Errs()->init_error();
            CDw *newo = sdesc->newWire(0, &wire, odesc->ldesc(),
                odesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newWire failed");
                Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            }
        }
        PolyList::destroy(p0);
        return (true);
    }
label:
inst:
    return (false);
}


yb *
cEdit::add_yank(CDo *odesc, BBox *AOI, yb *y0, bool clipout)
{
    if (!odesc)
        return (y0);
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
    {
        Blist *b0 =
            clipout ? odesc->oBB().clip_out(AOI) : odesc->oBB().clip_to(AOI);
        for (Blist *b = b0; b; b = b->next) {
            b->BB.left -= AOI->left;
            b->BB.bottom -= AOI->bottom;
            b->BB.right -= AOI->left;
            b->BB.top -= AOI->bottom;
            yb *yx = new yb_b(odesc->ldesc(), &b->BB);
            yx->next = y0;
            y0 = yx;
        }
        Blist::destroy(b0);
        return (y0);
    }
poly:
    {
        Zlist *zl = odesc->toZlist();
        Zoid Z(AOI);
        if (clipout)
            Zlist::zl_andnot(&zl, &Z);
        else
            Zlist::zl_and(&zl, &Z);
        PolyList *p0 = Zlist::to_poly_list(zl);
        for (PolyList *p = p0; p; p = p->next) {
            for (int i = 0; i < p->po.numpts; i++) {
                p->po.points[i].x -= AOI->left;
                p->po.points[i].y -= AOI->bottom;
            }
            yb *yx = new yb_p(odesc->ldesc(), &p->po);
            p->po.points = 0;
            yx->next = y0;
            y0 = yx;
        }
        PolyList::destroy(p0);
        return (y0);
    }
wire:
    {
        PolyList *p0 = ((const CDw*)odesc)->w_clip(AOI, clipout);
        for (PolyList *p = p0; p; p = p->next) {
            Wire wire;
            wire.points = p->po.points;
            p->po.points = 0;
            wire.numpts = p->po.numpts;
            wire.attributes = ((CDw*)odesc)->attributes();
            if (wire.wire_style() != CDWIRE_FLUSH) {
                if (wire.numpts == 1) {
                    BBox BB(wire.points[0].x, wire.points[0].y,
                        wire.points[0].x, wire.points[0].y);
                    BB.bloat(wire.wire_width()/2);
                    if (clipout) {
                        if (BB.intersect(AOI, false)) {
                            delete [] wire.points;
                            continue;
                        }
                    }
                    else {
                        if (!(BB <= *AOI)) {
                            delete [] wire.points;
                            continue;
                        }
                    }
                }
                else {
                    if (fix_end(wire.points, wire.points+1, wire.wire_width(),
                            AOI, clipout)) {
                        wire.numpts--;
                        Point *npts = new Point[wire.numpts];
                        memcpy(npts, wire.points+1, wire.numpts*sizeof(Point));
                        delete [] wire.points;
                        wire.points = npts;
                    }
                    if (wire.numpts == 1) {
                        BBox BB(wire.points[0].x, wire.points[0].y,
                            wire.points[0].x, wire.points[0].y);
                        BB.bloat(wire.wire_width()/2);
                        if (clipout) {
                            if (BB.intersect(AOI, false)) {
                                delete [] wire.points;
                                continue;
                            }
                        }
                        else {
                            if (!(BB <= *AOI)) {
                                delete [] wire.points;
                                continue;
                            }
                        }
                    }
                    else {
                        if (fix_end(wire.points + wire.numpts - 1,
                                wire.points + wire.numpts - 2,
                                wire.wire_width(), AOI, clipout)) {
                            wire.numpts--;
                            Point *npts = new Point[wire.numpts];
                            memcpy(npts, wire.points,
                                wire.numpts*sizeof(Point));
                            delete [] wire.points;
                            wire.points = npts;
                        }
                    }
                }
            }
            for (int j = 0; j < wire.numpts; j++) {
                wire.points[j].x -= AOI->left;
                wire.points[j].y -= AOI->bottom;
            }
            yb *yx = new yb_w(odesc->ldesc(), &wire);
            yx->next = y0;
            y0 = yx;
        }
        PolyList::destroy(p0);
        return (y0);
    }
label:
inst:
    return (y0);
}
// End of cEdit functions


//----------------
// Ghost Rendering

namespace {
    struct ybl_t
    {
        ybl_t(yb *elt, ybl_t *n)
            {
                yb_elt = elt;
                next = n;
            }

        static void destroy(ybl_t *y)
            {
                while (y) {
                    ybl_t *yx = y;
                    y = y->next;
                    delete yx;
                }
            }

        yb *yb_elt;
        ybl_t *next;
    };

    void display_ghost_put(yb *yx, CDtf *tfold, CDtf *tftmp)
    {
        if (yx->type == CDBOX) {
            BBox BB = ((yb_b*)yx)->BB;
            DSP()->TLoadCurrent(tfold);
            Point *points;
            DSP()->TBB(&BB, &points);
            DSP()->TLoadCurrent(tftmp);
            if (points) {
                Gst()->ShowGhostPath(points, 5);
                delete [] points;
            }
            else
                Gst()->ShowGhostBox(BB.left, BB.bottom, BB.right, BB.top);
        }
        else if (yx->type == CDPOLYGON) {
            Poly *po = &((yb_p*)yx)->poly;
            DSP()->TLoadCurrent(tfold);
            Point *points = Point::dup_with_xform(po->points, DSP(),
                po->numpts);
            DSP()->TLoadCurrent(tftmp);
            Gst()->ShowGhostPath(points, po->numpts);
            delete [] points;
        }
        else if (yx->type == CDWIRE) {
            Wire *w = &((yb_w*)yx)->wire;
            Point *points = w->points;
            DSP()->TLoadCurrent(tfold);
            w->points = Point::dup_with_xform(points, DSP(), w->numpts);
            DSP()->TLoadCurrent(tftmp);
            EGst()->showGhostWire(w);
            delete [] w->points;
            w->points = points;
        }
    }


    // Return the object with the largest area from among the next
    // n items in the list, and advance the list.
    //
    yb *pick_one(ybl_t **plist, int n)
    {
        double amx = 0.0;
        int i = 0;
        yb *ypick = 0;
        while (*plist && i < n) {
            yb *y = (*plist)->yb_elt;
            *plist = (*plist)->next;
            i++;
            double a = 0.0;
            if (y->type == CDBOX) {
                BBox BB = ((yb_b*)y)->BB;
                a = BB.area();
            }
            else if (y->type == CDPOLYGON) {
                Poly *po = &((yb_p*)y)->poly;
                a = po->area();
            }
            else if (y->type == CDWIRE) {
                Wire *w = &((yb_w*)y)->wire;
                a = w->area();
            }
            else
                // shouldn't happen
                continue;
            if (a > amx) {
                amx = a;
                ypick = y;
            }
        }
        return (ypick);
    }


    // The objects list for ghosting during put.
    ybl_t *ghost_display_list;
}


// Function to show the contents of the yank buffer while in the
// put command.
//
void
cEditGhost::showGhostYankBuf(int x, int y, int, int)
{
    CDtf tfold;
    DSP()->TCurrent(&tfold);
    DSP()->TPush();
    GEO()->applyCurTransform(DSP(), 0, 0, x, y);
    DSP()->TPremultiply();
    CDtf tftmp;
    DSP()->TCurrent(&tftmp);

    for (ybl_t *ybl = ghost_display_list; ybl; ybl = ybl->next)
        display_ghost_put(ybl->yb_elt, &tfold, &tftmp);
    DSP()->TPop();
}


// Static function.
// Initialize/free the object list for ghosting during put.
//
void
cEditGhost::ghost_put_setup(bool on)
{
    if (on) {
        ybl_t::destroy(ghost_display_list);  // should never need this
        ghost_display_list = 0;
        if (!PutCmd)
            return;
        unsigned int n = 0;
        for (yb *yx = ED()->yankBuffer()[PutCmd->index()]; yx; yx = yx->next) {
            ghost_display_list = new ybl_t(yx, ghost_display_list);
            n++;
        }
        if (n > EGst()->eg_max_ghost_objects) {
            // Too many objects to show efficiently.  Keep only the
            // objects with the largest area among groups.

            unsigned int n1 = n/EGst()->eg_max_ghost_objects;
            unsigned int n2 = n%EGst()->eg_max_ghost_objects;
            ybl_t *y0 = ghost_display_list;
            ybl_t *yl0 = 0;
            unsigned int j = 0, np;
            for (unsigned int i = 0; i < n; i += np) {
                np = n1 + (++j < n2);
                yb *y = pick_one(&y0, np);
                if (!y)
                    break;
                yl0 = new ybl_t(y, yl0);
            }
            ybl_t::destroy(ghost_display_list);
            ghost_display_list = yl0;
        }
    }
    else {
        ybl_t::destroy(ghost_display_list);
        ghost_display_list = 0;
    }
}

