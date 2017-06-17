
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
 $Id: sced_symbolic.cc,v 5.9 2016/02/21 18:49:44 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "sced.h"
#include "edit.h"
#include "undolist.h"
#include "dsp_inlines.h"
#include "cd_lgen.h"
#include "events.h"
#include "menu.h"
#include "ext_menu.h"
#include "ebtn_menu.h"
#include "pbtn_menu.h"
#include "edit_menu.h"
#include "cell_menu.h"
#include "promptline.h"
#include "select.h"
#include "errorlog.h"


//
// Command and functions for symbolic mode - the cell displays as a
// symbol rather than a scmenatic.
//

namespace {
    // Desensitize certain buttons in symbolic mode.
    //
    void
    set_sensitive(bool state)
    {
        if (!state && !CurCell()->isSymbolic()) {
            // The current cell is symbolic, but the editing context
            // is normal, so keep these buttons active.
            state = true;
        }

        MenuEnt *ent = Menu()->FindEntry("cell", MenuPUSH);
        if (ent)
            Menu()->SetSensitive(ent->cmd.caller, state);

        ent = Menu()->FindEntry("side", MenuPLACE);
        if (ent)
            Menu()->SetSensitive(ent->cmd.caller, state);
        ent = Menu()->FindEntry("edit", MenuPRPTY, 0);
        if (ent)
            Menu()->SetSensitive(ent->cmd.caller, state);
        ent = Menu()->FindEntry("edit", MenuFLATN);
        if (ent)
            Menu()->SetSensitive(ent->cmd.caller, state);

        ent = Menu()->FindEntry(0, MenuMUTUL);
        if (ent)
            Menu()->SetSensitive(ent->cmd.caller, state);

        ent = Menu()->FindEntry("ext", MenuEXSEL);
        if (ent)
            Menu()->SetSensitive(ent->cmd.caller, state);
        ent = Menu()->FindEntry("ext", MenuDVSEL);
        if (ent)
            Menu()->SetSensitive(ent->cmd.caller, state);
    }
}


// Menu command to switch to or create a symbolic view of the current
// cell.  The symbolic view is a simple figure with terminals, which
// is never expanded if saved with symbolic mode active.
//
void
cSced::symbolicExec(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(true, false, Electrical))
        return;

    bool status = cmd && Menu()->GetStatus(cmd->caller);
    if (!setCurSymbolic(status)) {
        Log()->PopUpErr(Errs()->get_error());
        return;
    }
    ds.clear();

    if (CurCell()->isSymbolic())
        PL()->ShowPrompt("You are editing the symbolic view.");
    else if (status)
        PL()->ShowPrompt(
            "You are editing the schematic view of a symbolic cell.");
    else
        PL()->ShowPrompt("You are editing the schematic view.");
}


// Switch between symbolic and non-symbolic mode, taking care of
// everything.
//
bool
cSced::setCurSymbolic(bool symb)
{
    if (!CurCell()) {
        Errs()->add_error("setCurSymbolic: no current cell.");
        return (false);
    }
    if (!CurCell()->isElectrical()) {
        Errs()->add_error("setCurSymbolic: called in physical mode.");
        return (false);
    }
    CDs *cursde = CurCell(true);

    // exit incompatible comands
    if (Menu()->MenuButtonStatus(0, MenuPRPTY) == 1)
        Menu()->MenuButtonPress(0, MenuPRPTY);
    if (EV()->CurCmd() && EV()->CurCmd()->Name() &&
            !strcmp(EV()->CurCmd()->Name(), "MUTUAL"))
        // mutul command, in device menu
        EV()->InitCallback();

    subcircuitShowConnectPts(ERASE);
    bool showterms = DSP()->ShowTerminals();
    if (showterms)
        DSP()->ShowTerminals(ERASE);
    CDp_sym *ps;
    if ((ps = (CDp_sym*)cursde->prpty(P_SYMBLC)) != 0) {
        if (symb) {
            Ulist()->ListCheck(MenuSYMBL, cursde, false);
            ps->set_active(true);
            set_sensitive(false);
            Ulist()->CommitChanges();
        }
        else {
            Ulist()->ListCheck("symbln", cursde, false);
            ps->set_active(false);
            set_sensitive(true);
            Ulist()->CommitChanges();
        }
        // Reflect BB and terminal location change to instances.
        cursde->computeBB();
        cursde->reflect();
        cursde->reflectTerminals();
        ED()->prptyRelist();
    }
    else if (symb) {
        Ulist()->ListCheck(MenuSYMBL, cursde, false);
        cursde->prptyAdd(P_SYMBLC, "1");
        Ulist()->CommitChanges();
        set_sensitive(false);
    }
    addParentConnections(CurCell());
    EV()->InitCallback();
    PopUpNodeMap(0, MODE_UPD);

    // clear the selection queue and redisplay
    Selections.deselectTypes(cursde, 0);
    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wdesc = wgen.next()) != 0) {
        if (wdesc->IsSimilar(Electrical, DSP()->MainWdesc()))
            wdesc->CenterFullView();
    }
    DSP()->RedisplayAll(Electrical);
    subcircuitShowConnectPts(DISPLAY);
    if (showterms)
        DSP()->ShowTerminals(DISPLAY);
    assertSymbolic(symb);
    return (true);
}


// Change the active flag in an existing P_SYMBLC property.  The
// caller should return to the original state quickly, before any
// major operation or screen redraw, or bad things are likely to
// happen.  This is for making quick updates without the overhead of a
// full-blown mode change.  Use with care.
//
bool
cSced::setCurSymbolicFast(bool symb)
{
    CDs *cursde = CurCell();
    if (!cursde || !cursde->isElectrical())
        return (false);
    bool ret = false;
    CDp_sym *ps = (CDp_sym*)cursde->prpty(P_SYMBLC);
    if (ps) {
        ret = ps->active();
        if (symb)
            ps->set_active(true);
        else
            ps->set_active(false);
    }
    return (ret);
}


// Set/unset the symbolic mode indication.  Called when editing
// context changes.  If arg is true, turn on symbolic editing mode
// indication if cell is symbolic.  Otherwise turn off symbolic
// editing mode indication.
//
void
cSced::assertSymbolic(bool mode)
{
    if (DSP()->CurMode() != Electrical)
        return;
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;
    if (mode && (cursde->isSymbolic() || cursde->symbolicRep(0))) {
        // Set the button if the cell is symbolic, whether or not it
        // is being displayed as such, set_sensitive will do the right
        // thing.
        Menu()->MenuButtonSet("main", MenuSYMBL, true);
        set_sensitive(false);
    }
    else {
        Menu()->MenuButtonSet("main", MenuSYMBL, false);
        set_sensitive(true);
    }
    cursde->computeBB();
    cursde->reflect();
    PopUpNodeMap(0, MODE_UPD);
}


// This will create a very simple symbolic representation for the
// electrical view of the current cell, and assert it.  Any existing
// symbolic representation will be cleared first.
//
bool
cSced::makeSymbolic()
{
    CDs *cursde = CurCell(Electrical, true);
    if (!cursde) {
        Errs()->add_error("makeSymbolic: no electrical current cell!");
        return (false);
    }

    CDp_snode *pn = (CDp_snode*)cursde->prpty(P_NODE);
    int ncnt = 0;
    for ( ; pn; pn = pn->next())
        ncnt++;
    if (!ncnt) {
        Errs()->add_error("makeSymbolic: cell has no terminals!");
        return (false);
    }

    Ulist()->ListCheck("MKSYM", cursde, false);
    CDp_sym *ps;
    if ((ps = (CDp_sym*)cursde->prpty(P_SYMBLC)) != 0)
        ps->set_active(true);
    else
        cursde->prptyAdd(P_SYMBLC, "1");
    Ulist()->CommitChanges();

    CDs *cdsymb = cursde->symbolicRep(0);
    if (!cdsymb) {
        Errs()->add_error("makeSymbolic: failed to create symbolic cell!");
        return (false);
    }

    Ulist()->ListCheck("MKSYM1", cdsymb, false);

    // Clear existing symbolic geometry.
    CDsLgen lgen(cdsymb);
    CDl *ld;
    while ((ld = lgen.next()) != 0) {
        CDg gdesc;
        gdesc.init_gen(cdsymb, ld);
        CDo *od;
        while ((od = gdesc.next()) != 0)
            Ulist()->RecordObjectChange(cdsymb, od, 0);
    }

    int tsp = 5*CDelecResolution;
    int w = (ncnt - 1)*tsp + 2*CDelecResolution;

    // Make a box for the cell body rep.
    BBox BB(-CDelecResolution, 0, w - CDelecResolution, 6*CDelecResolution);
    ld = CDldb()->findLayer("ETC1", Electrical);
    if (!ld)
        ld = CDldb()->findLayer("SCED", Electrical);
    CDo *newo;
    if (cdsymb->makeBox(ld, &BB, &newo, false) != CDok) {
        Errs()->add_error("internal, box creation failed.");
        return (false);
    }
    Ulist()->RecordObjectChange(cdsymb, 0, newo);

    // Add terminal wires, and move terminals to location.
    pn = (CDp_snode*)cursde->prpty(P_NODE);
    ld = CDldb()->findLayer("SCED", Electrical);
    for (int i = ncnt-1; i >= 0; i--) {
        Wire wire;
        wire.points = new Point[2];
        wire.numpts = 2;
        wire.points[0].x = i*tsp;
        wire.points[0].y = 6*CDelecResolution;
        wire.points[1].x = i*tsp;
        wire.points[1].y = 10*CDelecResolution;
        pn->set_pos(0, wire.points[1].x, wire.points[1].y);
        CDw *neww;
        if (cdsymb->makeWire(ld, &wire, &neww, 0, false) != CDok) {
            Errs()->add_error("internal. wire creation failed.");
            return (false);
        }
        Ulist()->RecordObjectChange(cdsymb, 0, neww);
        pn = pn->next();
    }

    // Put a name label in the box.
    Label la;
    la.label = new hyList(cursde, cursde->cellname()->string(), HYcvAscii);
    la.x = 0;
    la.y = 0;
    double wid, hei;
    CD()->DefaultLabelSize(cdsymb->cellname()->string(), Electrical,
        &wid, &hei);
    la.width = ELEC_INTERNAL_UNITS(2.0*wid);
    la.height = ELEC_INTERNAL_UNITS(2.0*hei);
    ld = CDldb()->findLayer("ETC2", Electrical);
    if (!ld)
        ld = CDldb()->findLayer("SCED", Electrical);
    CDla *newla;
    if (cdsymb->makeLabel(ld, &la, &newla, false) != CDok) {
        Errs()->add_error("internal. label creation failed.");
        return (false);
    }
    Ulist()->RecordObjectChange(cdsymb, 0, newla);

    Ulist()->CommitChanges();

    // Reflect BB and terminal location change to instances.
    cursde->computeBB();
    cursde->reflect();
    cursde->reflectTerminals();
    ED()->prptyRelist();
    assertSymbolic(true);

    if (cursde == CurCell(true))
        DSP()->RedisplayAll(Electrical);

    return (true);
}

