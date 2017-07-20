
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
 $Id: ext_term.cc,v 5.77 2016/06/02 03:53:52 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "edit.h"
#include "ext.h"
#include "ext_extract.h"
#include "sced.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "cd_lgen.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "dsp_layer.h"
#include "fio_gencif.h"
#include "events.h"
#include "promptline.h"
#include "errorlog.h"
#include "undolist.h"
#include "ghost.h"
#include "menu.h"


// Return a cell node property found be iterating through electrical
// cell node properties.
//
CDp_snode *
cExt::findTerminal(const char *name, int index, const Point *pe,
    const Point *pp)
{
    CDs *cursde = CurCell(Electrical, true);
    if (!cursde)
        return (0);
    cursde->checkTerminals();

    if (!name && index < 0 && !pe && !pp)
        return (0);

    CDnetName name_stab = CDnetex::name_tab_find(name);
    bool issy = cursde->isSymbolic();

    CDp_snode *pn = (CDp_snode*)cursde->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {
        if (name_stab && pn->term_name() != name_stab)
            continue;
        if (index >= 0 && pn->index() != (unsigned int)index)
            continue;
        if (pe) {
            if (issy) {
                bool fnd = false;
                for (unsigned int ix = 0; ; ix++) {
                    int xx, yy;
                    if (!pn->get_pos(ix, &xx, &yy))
                        break;
                    if (xx == pe->x && yy == pe->y) {
                        fnd = true;
                        break;
                    }
                }
                if (!fnd)
                    continue;
            }
            else {
                int xx, yy;
                pn->get_schem_pos(&xx, &yy);
                if (xx != pe->x || yy != pe->y)
                    continue;
            }
        }
        if (pp) {
            CDsterm *term = pn->cell_terminal();
            if (!term)
                continue;
            if (term->lx() != pp->x || term->ly() != pp->y)
                continue;
        }
        // have a match.
        return (pn);
    }
    return (0);
}


namespace {
    void term_change_finalize()
    {
        CDs *cursde = CurCell(Electrical);
        if (cursde) {

            // Make sure a name property exists, subckts have 3 entries in
            // name field.
            if (!cursde->prpty(P_NAME)) {
                char tbuf[256];
                sprintf(tbuf, "X 0 %s", cursde->cellname()->string());
                cursde->prptyAdd(P_NAME, tbuf);
            }

            // Update BB for symbolic mode.
            CDs *cdsymb = cursde->symbolicRep(0);
            if (cdsymb) {
                cdsymb->setBBvalid(false);
                cdsymb->computeBB();
            }

            // Remove any global properties.
            ED()->stripGlobalProperties(cursde);

            // This will compact the index numbers.
            cursde->reflectTerminals();
            cursde->reflectTermNames();
            cursde->unsetConnected();
        }

        CDs *cursdp = CurCell(Physical);
        if (cursdp) {
            ED()->stripGlobalProperties(cursdp);

            // The terminals are assigned during extraction in parent,
            // have to extract again since terminals may have changed.
            cursdp->reflectBadExtract();
        }
        Ulist()->CommitChanges();
    }
}


CDp_snode *
cExt::createTerminal(const char *name, const Point *pe, const char *termtype)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (0);
    if (cursde->isSymbolic()) {
        // Can't do it if the cell is symbolic.
        return (0);
    }

    int max_ix = -1;
    CDp_snode *ps = (CDp_snode*)cursde->prpty(P_NODE);
    for ( ; ps; ps = ps->next()) {
        if (max_ix < (int)ps->index())
            max_ix = ps->index();
    }
    ps = new CDp_snode;
    ps->set_enode(-1);
    ps->set_index(max_ix + 1);

    ps->set_term_name(name);
    if (pe) {
        ps->set_schem_pos(pe->x, pe->y);
        ps->set_pos(0, pe->x, pe->y);
    }
    if (termtype) {
        for (FlagDef *f = TermTypes; f->name; f++) {
            if (lstring::cieq(f->name, termtype)) {
                ps->set_termtype((CDtermType)f->value);
                break;
            }
        }
    }
    ps->set_next_prp(cursde->prptyList());
    cursde->setPrptyList(ps);
    term_change_finalize();
    return (ps);
}


// Return a cell terminal found by searching through the physical cell
// pins list.
//
CDsterm *
cExt::findPhysTerminal(const char *name, const Point *pp)
{
    CDs *cursde = CurCell(Electrical, true);
    if (cursde)
        cursde->checkTerminals();

    if (!name && !pp)
        return (0);

    CDnetName name_stab = CDnetex::name_tab_find(name);

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (0);
    for (CDpin *p = cursdp->pins(); p; p = p->next()) {

        if (name_stab && p->term()->name() != name_stab)
            continue;
        if (pp) {
            if (p->term()->lx() != pp->x || p->term()->ly() != pp->y)
                continue;
        }
        // have a match.
        return (p->term());
    }
    return (0);
}


// Create a physical terminal if one by the name doesn't exist, or
// update an existing terminal.  This does not require presence of
// electrical data.
//
bool
cExt::createPhysTerminal(CDs *psd, const char *name, const Point *pp,
    const char *lname)
{
    if (!psd || !name)
        return (false);
    CDnetName nn = CDnetex::name_tab_add(name);
    CDsterm *term = psd->findPinTerm(nn, true);
    if (!term->node_prpty()) {
        CDcbin cbin(psd);
        if (cbin.elec()) {
            for (CDp_snode *ps = (CDp_snode*)cbin.elec()->prpty(P_NODE); ps;
                    ps = ps->next()) {
                if (ps->term_name() == nn) {
                    ps->set_terminal(term);
                    term->set_node_prpty(ps);
                    break;
                }
            }
        }
    }
    if (pp)
        term->set_loc(pp->x, pp->y);
    if (lname) {
        CDl *ld = CDldb()->findLayer(lname);
        if (ld && ld->isRouting())
            term->set_layer(ld);
    }
    term_change_finalize();
    return (true);
}


bool
cExt::destroyTerminal(CDp_snode *ps)
{
    if (!ps)
        return (false);
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (false);
    if (!cursde->prptyUnlink(ps))
        return (false);
    delete ps;   // also deletes physical terminal
    term_change_finalize();
    return (true);
}


bool
cExt::destroyPhysTerminal(CDsterm *t)
{
    if (!t)
        return (false);
    delete t;
    term_change_finalize();
    return (true);
}


bool
cExt::setTerminalName(CDsterm *t, const char *name)
{
    if (!t)
        return (false);
    CDp_snode *ps = t->node_prpty();
    if (ps)
        ps->set_term_name(name);
    else
        t->set_name(CDnetex::name_tab_add(name));
    term_change_finalize();
    return (true);
}


bool
cExt::setTerminalType(CDp_snode *ps, const char *termtype)
{
    if (!ps)
        return (false);
    if (termtype) {
        for (FlagDef *f = TermTypes; f->name; f++) {
            if (lstring::cieq(f->name, termtype)) {
                ps->set_termtype((CDtermType)f->value);
                return (true);
            }
        }
        return (false);
    }
    ps->set_termtype((CDtermType)0);
    return (true);
}


bool
cExt::setTerminalLayer(CDsterm *t, const char *lname)
{
    if (!t)
        return (false);
    CDl *ld = 0;
    if (lname) {
        ld = CDldb()->findLayer(lname);
        if (!ld || !ld->isRouting())
            return (false);
    }
    t->set_layer(ld);
    term_change_finalize();
    return (true);
}


bool
cExt::setElecTerminalLoc(CDp_snode *ps, const Point *pe)
{
    if (!ps)
        return (false);
    if (!pe)
        return (false);
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (false);
    bool is_sym = cursde->isSymbolic();
    if (!is_sym) {
        ps->set_schem_pos(pe->x, pe->y);
        ps->set_pos(0, pe->x, pe->y);
    }
    else {
        unsigned int ix = 0;
        for ( ; ; ix++) {
            int x, y;
            if (!ps->get_pos(ix, &x, &y))
                break;
            if (x == pe->x && y == pe->y)
                return (true);
        }
        Point *pts = new Point[ix+1];
        for (ix = 0; ; ix++) {
            int x, y;
            if (!ps->get_pos(ix, &x, &y))
                break;
            pts[ix].x = x;
            pts[ix].y = y;
        }
        pts[ix].x = pe->x;
        pts[ix].y = pe->y;
        ps->alloc_pos(ix+1);
        for (ix = 0; ; ix++) {
            if (!ps->set_pos(ix, pts[ix].x, pts[ix].y))
                break;
        }
        delete [] pts;
    }
    term_change_finalize();
    return (true);
}


bool
cExt::clearElecTerminalLoc(CDp_snode *ps, const Point *pe)
{
    if (!ps)
        return (false);
    if (!pe)
        return (false);
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (false);
    bool is_sym = cursde->isSymbolic();
    if (!is_sym)
        return (true);

    unsigned int ix = 0;
    bool fnd = false;
    for ( ; ; ix++) {
        int x, y;
        if (!ps->get_pos(ix, &x, &y))
            break;
        if (x == pe->x && y == pe->y)
            fnd = true;
    }
    if (!fnd || ix <= 1)
        return (true);

    Point *pts = new Point[ix];
    unsigned int ii = 0;
    for (ix = 0; ; ix++) {
        int x, y;
        if (!ps->get_pos(ix, &x, &y))
            break;
        if (x == pe->x && y == pe->y)
            continue;
        pts[ii].x = x;
        pts[ii].y = y;
        ii++;
    }
    ps->alloc_pos(ii);
    for (ix = 0; ; ix++) {
        if (!ps->set_pos(ix, pts[ix].x, pts[ix].y))
            break;
    }
    delete [] pts;

    term_change_finalize();
    return (true);
}


bool
cExt::setPhysTerminalLoc(CDsterm *t, const Point *pp)
{
    if (!t)
        return (false);
    if (!pp)
        return (false);
    t->set_loc(pp->x, pp->y);
    term_change_finalize();
    return (true);
}


//-----------------------------------------------------------------------------
// The TEDIT command.
//
// Menu command to display the physical locations of terminal points.
// The terminals can be moved by a selection operation followed by
// clicking on the new location.  Only available in physical mode.

namespace {
    struct sTop
    {
        sTop(CDsterm *tnew, sTop *n)
            {
                term_bak = new CDsterm(0, 0);
                if (tnew)
                    *term_bak = *tnew;
                term_cur = tnew;
                sibling = 0;
                next = n;
            }

        ~sTop()
            {
                if (term_bak) {
                    CDsterm tmp(0, 0);
                    *term_bak = tmp;
                }
                delete term_bak;
                destroy(sibling);
            }

        static void destroy(sTop *op)
            {
                while (op) {
                    sTop *opx = op;
                    op = op->next;
                    delete opx;
                }
            }

        CDsterm *term_bak;
        CDsterm *term_cur;
        sTop *sibling;
        sTop *next;
    };

    namespace ext_term {
        struct PtState : public CmdState
        {
            friend void cExtGhost::showGhostPhysTerms(int, int, int, int);

            PtState(const char*, const char*);
            virtual ~PtState();

            void setup(GRobject c, GRobject ef)
                {
                    Caller = c;
                    EndFix = ef;
                }

            void terminalEdit(CDsterm*);

            static void te_cb(te_info_t*, CDsterm*);

            void b1down();
            void b1up();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();

            void message() { PL()->ShowPrompt(te_msg1); }

        private:
            void move_terms(int, int);
            void SetLevel1() { Level = 1; }
            void SetLevel2() { Level = 2; }

            GRobject Caller;
            GRobject EndFix;
            CDpin *Terms;
            CDsterm *EditTerm;
            sTop *Opers;
            sTop *Redos;
            int State;
            bool Moved;

            static const char *te_msg1;
        };

        PtState *PtCmd;
    }
}

using namespace ext_term;

const char *PtState::te_msg1 =
    "SHIFT- or double-click on cell terminals to edit properties,"
    " click/drag to move.";

namespace {
    bool are_terms()   
    {
        CDs *cursde = CurCell(Electrical, true);
        if (!cursde)
            return (false);
        CDp_snode *pn = (CDp_snode*)cursde->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            if (pn->cell_terminal())
                return (true);
        }
        return (false);
    }
}


void
cExt::editTermsExec(GRobject caller, GRobject endfix)
{
    if (PtCmd) {
        PtCmd->esc();
        return;
    }
    if (!caller)
        return;
    if (!XM()->CheckCurMode(Physical)) {
        Menu()->Deselect(caller);
        return;
    }
    if (!XM()->CheckCurCell(true, false, Physical)) {
        Menu()->Deselect(caller);
        return;
    }
    if (!XM()->CheckCurCell(true, false, Electrical)) {
        Menu()->Deselect(caller);
        return;
    }

    // Note that we don't call associate here, but ensure that the
    // cell's physical terminal database is constructed and
    // initialized.

    CDs *sdp = CurCell(Physical);
    if (!sdp->checkPhysTerminals(true))
        Log()->ErrorLog(mh::Processing, Errs()->get_error());

    if (!are_terms()) {
        PL()->ShowPrompt("No terminals found to edit.");
        return;
    }

    // This will arrange the uninitialized terminals and formal
    // terminals with bad referencing.  We only care about the formal
    // terminals.
    //
    dspPkgIf()->SetWorking(true);
    if (!DSP()->ContactsVisible()) {
        DSP()->SetContactsVisible(true);
        if (!DSP()->TerminalsVisible()) {
            if (CurCell(Physical)->groups()) {
                CDcbin cbin(DSP()->CurCellName());
                EX()->arrangeTerms(&cbin, false);
            }
            DSP()->ShowTerminals(DISPLAY);
        }
    }
    dspPkgIf()->SetWorking(false);

    PtCmd = new PtState("TERMEDIT", "xic:tedit");
    PtCmd->setup(caller, endfix);
    if (!EV()->PushCallback(PtCmd)) {
        delete PtCmd;
        Menu()->Deselect(caller);
        return;
    }
    PtCmd->message();
}


void
cExt::editTermsPush(CDsterm *term)
{
    if (PtCmd)
        PtCmd->terminalEdit(term);
}


void
cExt::showPhysTermsExec(GRobject caller, bool contacts_only)
{
    if (!caller)
        return;
    if (!XM()->CheckCurMode(Physical)) {
        Menu()->Deselect(caller);
        return;
    }
    if (!XM()->CheckCurCell(false, false, Physical)) {
        Menu()->Deselect(caller);
        return;
    }
    if (!Menu()->GetStatus(caller)) {
        if (DSP()->TerminalsVisible()) {
            DSP()->ShowTerminals(ERASE);
            DSP()->SetTerminalsVisible(false);
            if (DSP()->ContactsVisible()) {
                if (PtCmd) {
                    DSP()->SetShowTerminals(true);
                    DSP()->ShowTerminals(DISPLAY);
                }
                else
                    DSP()->SetContactsVisible(false);
            }
        }
        else if (DSP()->ContactsVisible() && !PtCmd) {
            DSP()->ShowTerminals(ERASE);
            DSP()->SetContactsVisible(false);
        }
        return;
    }
    if (PtCmd && contacts_only && !DSP()->TerminalsVisible() &&
            DSP()->ContactsVisible())
        return;

    DSP()->ShowTerminals(ERASE);
    if (!PtCmd)
        DSP()->SetContactsVisible(false);
    DSP()->SetTerminalsVisible(false);

    if (!EX()->associate(CurCell(Physical))) {
        PL()->ShowPrompt("Association failed!");
        Menu()->Deselect(caller);
        return;
    }
    cGroupDesc *gd = CurCell(Physical)->groups();
    if (!gd || gd->isempty() || !gd->elec_nets()) {
        PL()->ShowPrompt("No terminals found to display.");
        Menu()->Deselect(caller);
        return;
    }
    if (contacts_only)
        DSP()->SetContactsVisible(true);
    else
        DSP()->SetTerminalsVisible(true);
    DSP()->ShowTerminals(DISPLAY);
}


PtState::PtState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    EndFix = 0;
    Terms = 0;
    EditTerm = 0;
    Opers = 0;
    Redos = 0;
    State = 0;
    Moved = false;
    SetLevel1();
}


PtState::~PtState()
{
    if (Moved) {
        // We moved a terminal location.  Have to update all of the
        // instances.
        CDm_gen gen(CurCell(Physical), GEN_MASTER_REFS);
        for (CDm *m = gen.m_first(); m; m = gen.m_next()) {
            CDc_gen cgen(m);
            for (CDc *cp = cgen.c_first(); cp; cp = cgen.c_next()) {
                CDap ap(cp);
                for (unsigned int iy = 0; iy < ap.ny; iy++) {
                    for (unsigned int ix = 0; ix < ap.nx; ix++) {
                        int vecix;
                        CDc *ed = cp->findElecDualOfPhys(&vecix, ix, iy);
                        if (!ed)
                            continue;
                        EX()->placePhysSubcTerminals(ed, vecix, cp, ix, iy);
                    }
                }
            }
        }
    }
    sTop::destroy(Opers);
    sTop::destroy(Redos);
    PtCmd = 0;
}


void
PtState::terminalEdit(CDsterm *term)
{
    if (!term)
        return;
    CDpin::destroy(Terms);
    Terms = new CDpin(term, 0);

    if (EditTerm) {
        CDpin p(EditTerm, 0);
        DSP()->HlitePhysTermList(ERASE, &p);
    }
    int x, y;
    Menu()->PointerRootLoc(&x, &y);
    te_info_t tinfo(term);
    EX()->PopUpPhysTermEdit(0, MODE_UPD, &tinfo, 0, term, 0, 0);
    DSP()->HlitePhysTermList(DISPLAY, Terms);
    EditTerm = Terms->term();
    SetLevel2();
    State = 3;
}


// Static function.
// Calback for property setting pop-up, perform name or layer binding
// change on the currently selected terminal.
//
void
PtState::te_cb(te_info_t *tinfo, CDsterm *term)
{
    if (tinfo) {
        if (!term)
            return;

        CDcbin cbin(DSP()->CurCellName());
        if (!cbin.elec())
            return;

        CDl *ld = CDldb()->findLayer(tinfo->layer_name(), Physical);

        if (PtCmd && PtCmd->EditTerm) {
            CDpin p(PtCmd->EditTerm, 0);
            DSP()->HlitePhysTermList(ERASE, &p);
        }
        DSP()->ShowCellTerminalMarks(ERASE);

        // Save for undo.
        PtCmd->Opers = new sTop(term, PtCmd->Opers);

        CDl *old = term->layer();
        if (old != ld) {
            term->set_layer(ld);
            // don't touch vgroup
            if (term->get_ref())
                term->set_ref(0);
            term->set_uninit(true);
            if (cbin.phys())
                cbin.phys()->reflectBadExtract();
        }
        term->set_fixed(tinfo->flags() & TE_FIXED);
        DSP()->ShowCellTerminalMarks(DISPLAY);
        if (PtCmd && PtCmd->EditTerm) {
            CDpin p(PtCmd->EditTerm, 0);
            DSP()->HlitePhysTermList(DISPLAY, &p);
        }
    }
    else if (PtCmd && PtCmd->EditTerm) {
        CDpin p(PtCmd->EditTerm, 0);
        DSP()->HlitePhysTermList(ERASE, &p);
    }
}


void
PtState::b1down()
{
    if (Level == 1) {
        State = 0;
        BBox BB;
        EV()->Cursor().get_raw(&BB.left, &BB.top);
        BB.right = BB.left;
        BB.bottom = BB.top;
        int d = (int)(5.0/EV()->CurrentWin()->Ratio());
        if (!d)
            d = 1;
        BB.bloat(d);
        CDpin *tlist = EX()->pointAtPins(&BB);
        if (!tlist) {
            cEventHdlr::sel_b1down();
            return;
        }
        CDpin::destroy(Terms);
        Terms = tlist;
        unsigned int downstate = EV()->Cursor().get_downstate();
        if (downstate & GR_SHIFT_MASK) {
            CDpin::destroy(Terms->next());
            Terms->set_next(0);
            if (EditTerm) {
                CDpin p(EditTerm, 0);
                DSP()->HlitePhysTermList(ERASE, &p);
            }
            int x, y;
            Menu()->PointerRootLoc(&x, &y);
            te_info_t tinfo(Terms->term());
            EX()->PopUpPhysTermEdit(0, MODE_ON, &tinfo, te_cb,
                Terms->term(), x + 50, y + 20);
            DSP()->HlitePhysTermList(DISPLAY, Terms);
            EditTerm = Terms->term();
            SetLevel2();
            State = 3;
            return;
        }
        Gst()->SetGhost(GFpterms);
        EV()->DownTimer(GFeterms);
        State = 1;
    }
    else {
        Gst()->SetGhost(GFnone);
        if (Terms) {
            int dx, dy;
            EV()->Cursor().get_dxdy(&dx, &dy);
            if (dx != 0 || dy != 0)
                move_terms(dx, dy);
            else if (!Terms->next()) {
                if (EditTerm) {
                    CDpin p(EditTerm, 0);
                    DSP()->HlitePhysTermList(ERASE, &p);
                }
                int x, y;
                Menu()->PointerRootLoc(&x, &y);
                te_info_t tinfo(Terms->term());
                EX()->PopUpPhysTermEdit(0, MODE_ON, &tinfo, te_cb,
                    Terms->term(), x + 50, y + 20);
                DSP()->HlitePhysTermList(DISPLAY, Terms);
                EditTerm = Terms->term();
            }
            State = 3;
        }
    }
}


void
PtState::b1up()
{
    if (Level == 1) {
        BBox AOI;
        if (EV()->Cursor().is_release_ok()) {
            if (!State) {
                cEventHdlr::sel_b1up(&AOI, 0, B1UP_NOSEL);
                CDpin *tlist = EX()->pointAtPins(&AOI);
                if (tlist) {
                    CDpin::destroy(Terms);
                    Terms = tlist;
                    Gst()->SetGhost(GFpterms);
                    State = 1;
                    SetLevel2();
                }
                return;
            }
            if (!EV()->UpTimer()) {
                int x, y;
                EV()->Cursor().get_release(&x, &y);
                EV()->CurrentWin()->Snap(&x, &y);
                Gst()->SetGhost(GFnone);
                int xc, yc;
                EV()->Cursor().get_xy(&xc, &yc);
                if (x == xc && y == yc) {
                    if (Terms)
                        SetLevel2();
                    return;
                }
                move_terms(x - xc, y - yc);
                State = 2;
                return;
            }
        }
        if (Terms)
            SetLevel2();
    }
    else if (State == 3) {
        CDpin::destroy(Terms);
        Terms = 0;
        SetLevel1();
    }
}


void
PtState::esc()
{
    if (State == 1) {
        // When moving, just exit state.
        Gst()->SetGhost(GFnone);
        Level = 1;
        State = 0;
        return;
    }
    if (EditTerm) {
        CDpin p(EditTerm, 0);
        DSP()->HlitePhysTermList(ERASE, &p);
    }
    EX()->PopUpPhysTermEdit(0, MODE_OFF, 0, 0, 0, 0, 0);
    cEventHdlr::sel_esc();
    Gst()->SetGhost(GFnone);
    PL()->ErasePrompt();
    if (EndFix) {
        // EndFix is the "Show Cell Terms Only" button.
        if (!DSP()->TerminalsVisible()) {
            if (!Menu()->GetStatus(EndFix)) {
                DSP()->SetContactsVisible(false);
                DSP()->ShowTerminals(ERASE);
            }
        }
        else
            DSP()->SetContactsVisible(false);
    }
    else {
        if (!DSP()->TerminalsVisible())
            DSP()->ShowTerminals(ERASE);
        DSP()->SetContactsVisible(false);
    }
    EV()->PopCallback(this);
    if (Caller)
        Menu()->Deselect(Caller);
    CDpin::destroy(Terms);
    CDs *cursde = CurCell(Electrical, true);
    if (cursde)
        cursde->reflectTermNames();
    delete this;
}


// Handle Shift key
//
bool
PtState::key(int code, const char*, int)
{
    switch (code) {
    case BSP_KEY:
        if ((Level == 1 || Level == 2) && State == 1) {
            // When moving, just exit state.
            Gst()->SetGhost(GFnone);
            Level = 1;
            State = 0;
        }
        break;
    default:
        return (false);
    }
    return (true);
}


void
PtState::undo()
{
    if (!Opers) {
        PL()->ShowPrompt("No more terminal actions to undo.");
        return;
    }
    sTop *op = Opers;
    Opers = Opers->next;

    CDpin *tltmp = 0;
    for (sTop *o = op; o; o = o->sibling)
        tltmp = new CDpin(o->term_cur, tltmp);

    DSP()->ShowPhysTermList(ERASE, tltmp);
    cGroupDesc *gd = CurCell(Physical)->groups();
    CDsterm ttmp(0, 0);
    const CDsterm mtterm(0, 0);
    for (sTop *o = op; o; o = o->sibling) {
        CDsterm *tbak = o->term_bak;
        CDsterm *tcur = o->term_cur;
        int ogrp = tbak->group();
        int ngrp = tcur->group();

        // NOTE:  This won't work, because it calls the constructor,
        // which doesn't copy the property pointer!
        //  CDsterm ttmp = *tbak;

        ttmp = *tbak;
        *tbak = *tcur;
        *tcur = ttmp;
        ttmp = mtterm;

        if (gd) {
            if (ngrp >= 0 && ngrp != ogrp) {
                // remove
                sGroup *gp = gd->group_for(ngrp);
                if (gp) {
                    CDpin *pp = 0, *pn;
                    for (CDpin *p = gp->termlist(); p; p = pn) {
                        pn = p->next();
                        if (p->term() == tcur) {
                            if (pp)
                                pp->set_next(pn);
                            else
                                gp->set_termlist(pn);
                            delete p;
                            break;
                        }
                        pp = p;
                    }
                }
            }
            if (ogrp >= 0 && ogrp != ngrp) {
                // add
                sGroup *gp = gd->group_for(ogrp);
                if (gp)
                    gp->set_termlist(new CDpin(tcur, gp->termlist()));
            }
        }
        if (tcur == EditTerm) {
            te_info_t tinfo(EditTerm);
            EX()->PopUpPhysTermEdit(0, MODE_UPD, &tinfo, 0, EditTerm, 0, 0);
        }
    }
    DSP()->ShowPhysTermList(DISPLAY, tltmp);
    CDpin::destroy(tltmp);
    op->next = Redos;
    Redos = op;
}


void
PtState::redo()
{
    if (!Redos) {
        PL()->ShowPrompt("No more terminal actions to redo.");
        return;
    }
    sTop *op = Redos;
    Redos = Redos->next;

    CDpin *tltmp = 0;
    for (sTop *o = op; o; o = o->sibling)
        tltmp = new CDpin(o->term_cur, tltmp);

    DSP()->ShowPhysTermList(ERASE, tltmp);
    cGroupDesc *gd = CurCell(Physical)->groups();
    CDsterm ttmp(0, 0);
    const CDsterm mtterm(0, 0);
    for (sTop *o = op; o; o = o->sibling) {
        CDsterm *tbak = o->term_bak;
        CDsterm *tcur = o->term_cur;
        int ogrp = tbak->group();
        int ngrp = tcur->group();

        // NOTE:  This won't work, because it calls the constructor,
        // which doesn't copy the property pointer!
        //  CDsterm ttmp = *tbak;

        ttmp = *tbak;
        *tbak = *tcur;
        *tcur = ttmp;
        ttmp = mtterm;

        if (gd) {
            if (ngrp >= 0 && ngrp != ogrp) {
                // remove
                sGroup *gp = gd->group_for(ngrp);
                if (gp) {
                    CDpin *pp = 0, *pn;
                    for (CDpin *p = gp->termlist(); p; p = pn) {
                        pn = p->next();
                        if (p->term() == tcur) {
                            if (pp)
                                pp->set_next(pn);
                            else
                                gp->set_termlist(pn);
                            delete p;
                            break;
                        }
                        pp = p;
                    }
                }
            }
            if (ogrp >= 0 && ogrp != ngrp) {
                // add
                sGroup *gp = gd->group_for(ogrp);
                if (gp)
                    gp->set_termlist(new CDpin(tcur, gp->termlist()));
            }
        }
        if (tcur == EditTerm) {
            te_info_t tinfo(EditTerm);
            EX()->PopUpPhysTermEdit(0, MODE_UPD, &tinfo, 0, EditTerm, 0, 0);
        }
    }
    DSP()->ShowPhysTermList(DISPLAY, tltmp);
    CDpin::destroy(tltmp);
    op->next = Opers;
    Opers = op;
}


void
PtState::move_terms(int dx, int dy)
{
    if (dx == 0 && dy == 0)
        return;
    // pull any coincident terminals into the dups list
    CDpin *t0 = Terms->next(), *tn;
    CDpin *dups = 0;
    Terms->set_next(0);
    for ( ; t0; t0 = tn) {
        tn = t0->next();
        CDpin *t;
        for (t = Terms; t; t = t->next()) {
            if (t->term()->is_loc_same(t0->term()))
                break;
        }
        if (t) {
            t0->set_next(dups);
            dups = t0;
        }
        else {
            t0->set_next(Terms);
            Terms = t0;
        }
    }

    CDcbin cbin(DSP()->CurCellName());

    sTop *oe = 0;

    bool grp_changed = false;
    bool lyr_changed = false;
    DSP()->ShowPhysTermList(ERASE, Terms);
    for (CDpin *t = Terms; t; t = t->next()) {
        CDsterm *term = t->term();
        // Terms contains only movable terms

        sTop *op = new sTop(term, 0); 
        if (!oe) {
            op->next = Opers;
            Opers = op;
        }
        else
            oe->sibling = op;
        oe = op;

        int x = term->lx() + dx;
        int y = term->ly() + dy;
        WindowDesc *wdesc =
            EV()->CurrentWin() ? EV()->CurrentWin() : DSP()->MainWdesc();
        wdesc->Snap(&x, &y);
        term->set_loc(x, y);
        if (term->lx() != op->term_bak->lx() ||
                term->ly() != op->term_bak->ly())
            Moved = true;

        cGroupDesc *gd = cbin.groups();
        if (gd && !gd->isempty()) {
            int oldgrp = term->group();
            CDl *oldld = term->layer();
            term->set_ref(0);
            if (!term->layer())
                term->set_layer(oldld);
            if (!gd->bind_term_group_at_location(term)) {
                term->set_layer(0);
                gd->bind_term_group_at_location(term);
            }
            int newgrp = term->group();
            CDl *newld = term->layer();
            if (oldgrp != newgrp)
                grp_changed = true;
            if (oldld != newld)
                lyr_changed = true;
        }

        // If placed inside the cell's BB, make the position fixed,
        // otherwise not.
        term->set_fixed(cbin.phys()->BB()->intersect(x, y, true));
    }

    DSP()->ShowPhysTermList(DISPLAY, Terms);
    if (dups) {
        DSP()->ShowPhysTermList(DISPLAY, dups);
        CDpin::destroy(dups);
    }
    if (cbin.elec())
        cbin.elec()->incModified();
    if (grp_changed || lyr_changed) {
        if (cbin.phys())
            cbin.phys()->reflectBadExtract();
    }
}
// End of PtState functions


//----------------
// Ghost Rendering

// Render the ghost-drawn terminals and instance labels during a move.
//
void
cExtGhost::showGhostPhysTerms(int x, int y, int refx, int refy)
{
    if (!PtCmd)
        return;
    WindowDesc *wd;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wd = wgen.next()) != 0) {
        if (wd->IsSimilar(Physical, DSP()->MainWdesc())) {
            int delta = (int)(DSP_DEF_PTRM_DELTA/wd->Ratio());
            for (CDpin *t = PtCmd->Terms; t; t = t->next()) {
                int xx = t->term()->lx() + x - refx;
                int yy = t->term()->ly() + y - refy;
                wd->ShowLineW(xx-delta, yy-delta, xx-delta, yy+delta);
                wd->ShowLineW(xx-delta, yy+delta, xx+delta, yy+delta);
                wd->ShowLineW(xx+delta, yy+delta, xx+delta, yy-delta);
                wd->ShowLineW(xx+delta, yy-delta, xx-delta, yy-delta);
            }
        }
    }
}

