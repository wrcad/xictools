
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
#include "extif.h"
#include "undolist.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "events.h"
#include "menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "ghost.h"


//
// Functions to provide user specified subcircut connections.
//

namespace {
    bool selectNode(int *xo, int *yo)
    {
        WindowDesc *wdesc = EV()->ButtonWin();
        if (!wdesc)
            wdesc = DSP()->MainWdesc();
        CDs *esd = wdesc->CurCellDesc(Electrical, true);
        if (!esd)
            return (false);
        int delta = 1 + (int)(DSP()->PixelDelta()/wdesc->Ratio());
        return (esd->checkVertex(xo, yo, delta, true));
    }
}


// Menu switch to set display of terminals.
//
void
cSced::showTermsExec(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        DSP()->ShowTerminals(DISPLAY);
    else {
        DSP()->ShowTerminals(ERASE);
        subcircuitShowConnectPts(DISPLAY);
    }
}


// Render the ghost-drawn terminals during a move.
//
void
cScedGhost::showGhostElecTerms(int x, int y, int, int)
{
    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wdesc = wgen.next()) != 0) {
        if (wdesc->IsSimilar(Electrical, DSP()->MainWdesc())) {
            int delta = (int)(10.0/wdesc->Ratio());
            wdesc->ShowLineW(x-delta, y-delta, x-delta, y+delta);
            wdesc->ShowLineW(x-delta, y+delta, x+delta, y+delta);
            wdesc->ShowLineW(x+delta, y+delta, x+delta, y-delta);
            wdesc->ShowLineW(x+delta, y-delta, x-delta, y-delta);
        }
    }
}


//
// The SUBCT command
//

// Operation list element for undo/redo.
namespace {
    enum NchgType
    {
        N_NOP, N_BAT,
        N_ADD, N_DEL, N_MOV, N_RNM,
        N_BADD, N_BDEL, N_BMOV, N_BRNM
    };

    struct sNop
    {
        sNop(sNop *b, sNop *nx)
            {
                type = N_BAT;
                indx = 0;
                oindx = 0;
                flags = 0;
                beg = 0;
                end = 0;
                prp = 0;
                bprp = 0;
                loc.set(0, 0);
                sypts = 0;
                netexp = 0;
                name = 0;
                next = nx;
                batch = b;
            }

        sNop(NchgType t, int i, CDp_snode *p, sNop *nx)
            {
                type = t;
                indx = i;
                oindx = 0;
                flags = 0;
                beg = 0;
                end = 0;
                prp = p;
                bprp = 0;
                loc.set(0, 0);
                sypts = 0;
                netexp = 0;
                name = 0;
                next = nx;
                batch = 0;
            }

        sNop(NchgType t, int i, CDp_bsnode *p, sNop *nx)
            {
                type = t;
                indx = i;
                oindx = 0;
                flags = 0;
                beg = 0;
                end = 0;
                prp = 0;
                bprp = p;
                loc.set(0, 0);
                sypts = 0;
                netexp = 0;
                name = 0;
                next = nx;
                batch = 0;
            }

        ~sNop()
            {
                CDnetex::destroy(netexp);
            }

        static void freeUndo(sNop *op)
            {
                while (op) {
                    sNop *opx = op;
                    op = op->next;
                    if (opx->type == N_DEL)
                        delete opx->prp;
                    else if (opx->type == N_BDEL)
                        delete opx->bprp;
                    Plist::destroy(opx->sypts);
                    delete opx;
                }
            }

        static void freeRedo(sNop *op)
            {
                while (op) {
                    sNop *opx = op;
                    op = op->next;
                    if (opx->type == N_ADD)
                        delete opx->prp;
                    else if (opx->type == N_BADD)
                        delete opx->bprp;
                    Plist::destroy(opx->sypts);
                    delete opx;
                }
            }

        NchgType type;
        unsigned int indx;
        unsigned int oindx;
        unsigned int flags;
        unsigned int beg, end;
        CDp_snode *prp;
        CDp_bsnode *bprp;
        Point loc;
        Plist *sypts;
        CDnetex *netexp;
        CDnetName name;
        sNop *next;
        sNop *batch;
    };

    namespace sced_subckt {
        struct SubcState : public CmdState
        {
            SubcState(const char*, const char*);
            virtual ~SubcState();

            void setCaller(GRobject c)  { Caller = c; }

            void show_connections(bool show)
                {
                    show_terms(show, 0);
                    show_bterms(show);
                }

            bool delete_current()
                {
                    if (EditNode)
                        return (delete_terminal(EditNode));
                    if (EditBterm)
                        return (delete_bterm(EditBterm));
                    return (false);
                }

            bool edit_term(int*, bool*);
            bool set_edit_term(int, bool);
            bool delete_terminal(CDp_snode*);
            bool delete_bterm(CDp_bsnode*);
            bool check_bterm_bits();
            bool order_bterm_bits();
            bool bterm_bits_flags(bool, int);
            bool insert_terminal(CDp_bsnode*, CDp_snode*);
            bool change_index(CDp_snode*, int);

            void b1down();
            void b1up();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();
            void save_nodes();

        private:
            void update();

            void SetLevel1() { Level = 1; }
            void SetLevel2() { Level = 2; }

            // terminals
            CDp_snode *find_term(int, int, int*, int*);
            CDp_snode *find_term(int, int, bool = false);
            bool insert_terminal(int, int);
            bool delete_terminal(int, int);
            bool move_terminal(int, int, int, int, bool);
            void show_terms(bool, int);
            void show_term(bool, CDp_snode*, int);
            static void te_cb(TermEditInfo*, CDp*);
            void edit_terminal(CDp_snode*, bool);
            int term_disp_num(int);

            // bus terms
            CDp_bsnode *find_bterm(int, int);
            bool insert_bterm(int, int);
            bool delete_bterm(int, int);
            bool move_bterm(int, int, int, int, bool);
            void show_bterms(bool);
            void show_bterm(bool, CDp_bsnode*);
            void edit_bterm(CDp_bsnode*, bool);

            GRobject Caller;
            bool Changed;
            int Lastx, Lasty;
            int State;
            int Ulev;
            sNop *Opers;
            sNop *Redos;
            CDp_snode **Nodes;
            int Nodesize;
            CDp_snode *EditNode;
            CDp_bsnode *EditBterm;
            bool EditVisible;
            bool ShowInvis;
        };

        SubcState *SubcCmd;
    }
}

using namespace sced_subckt;


// Menu command that allows user to define subcircuit connection
// points.
//
void
cSced::subcircuitExec(CmdDesc *cmd)
{
    if (SubcCmd) {
        SubcCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(true, true, Electrical))
        return;

    Selections.deselectTypes(CurCell(), 0);
    SubcCmd = new SubcState("SUBCKT", "xic:subct");
    SubcCmd->setCaller(cmd ? cmd->caller : 0);
    Ulist()->ListCheck(SubcCmd->Name(), CurCell(), false);
    if (!EV()->PushCallback(SubcCmd)) {
        delete SubcCmd;
        return;
    }
    PL()->ShowPrompt(
        "SHIFT- or double-click on cell terminals to edit properties,"
        " click/drag to move.  CTRL-click for virtual, SHIFT-click for"
        " bus term.");
    if (DSP()->ShowTerminals())
        // erase the cell's marks only
        DSP()->ShowCellTerminalMarks(ERASE);
    SubcCmd->save_nodes();
    SubcCmd->show_connections(DISPLAY);
    ds.clear();
}


// Export
//
void
cSced::subcircuitShowConnectPts(bool show)
{
    if (SubcCmd)
        SubcCmd->show_connections(show);
}


// Export
// Return index and type of terminal being edited.
//
bool
cSced::subcircuitEditTerm(int *pindx, bool *pbterm)
{
    if (SubcCmd)
        return (SubcCmd->edit_term(pindx, pbterm));
    if (pindx)
        *pindx = -1;
    if (pbterm)
        *pbterm = false;
    return  (false);
}


// Export
// Switching to editing the indicated terminal if it exists.
//
bool
cSced::subcircuitSetEditTerm(int indx, bool bterm)
{
    if (SubcCmd)
        return (SubcCmd->set_edit_term(indx, bterm));
    return  (false);
}


// Export
// Delete the current terminal, save it for possible reuse.
//
bool
cSced::subcircuitDeleteTerm()
{
    if (SubcCmd)
        return (SubcCmd->delete_current());
    return (false);
}


// Export
// Create any missing bits of the present named bus terminal.
//
bool
cSced::subcircuitBits(bool reorder)
{
    if (SubcCmd) {
        if (reorder)
            return (SubcCmd->order_bterm_bits());
        return (SubcCmd->check_bterm_bits());
    }
    return (false);
}


// Export
// Set all visibility flags of the present bus terminal's bits.
//
bool
cSced::subcircuitBitsVisible(int flag)
{
    if (SubcCmd) {
        if (SubcCmd->bterm_bits_flags(false, flag)) {
            int xx, yy;
            Menu()->PointerRootLoc(&xx, &yy);
            PL()->FlashMessageHereV(xx, yy,
                "%s bit terminals are now visible",
                flag == TE_SCINVIS ? "Schematic" : "Symbol");
            return (true);
        }
    }
    return (false);
}


// Export
// Unset all visibility flags of the present bus terminal's bits.
//
bool
cSced::subcircuitBitsInvisible(int flag)
{
    if (SubcCmd) {
        if (SubcCmd->bterm_bits_flags(true, flag)) {
            int xx, yy;
            Menu()->PointerRootLoc(&xx, &yy);
            PL()->FlashMessageHereV(xx, yy,
                "%s bit terminals are now invisible",
                flag == TE_SCINVIS ? "Schematic" : "Symbol");
            return (true);
        }
    }
    return (false);
}
// End of cSced functions.


SubcState::SubcState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    Changed = false;
    Lastx = Lasty = 0;
    State = 0;
    Ulev = 0;
    Opers = Redos = 0;
    Nodes = 0;
    Nodesize = 0;
    EditNode = 0;
    EditBterm = 0;
    EditVisible = false;
    ShowInvis = false;
    SetLevel1();
    ExtIf()->PopUpExtSetup(0, MODE_UPD);
}


SubcState::~SubcState()
{
    SubcCmd = 0;
    delete [] Nodes;
    ExtIf()->PopUpExtSetup(0, MODE_UPD);
}


// Export.  Return the index and type of the terminal being edited.
//
bool
SubcState::edit_term(int *pindx, bool *pbterm)
{
    if (pindx)
        *pindx = -1;
    if (pbterm)
        *pbterm = false;
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (false);

    if (EditNode) {
        *pindx = term_disp_num(EditNode->index());
        return (true);
    }
    if (EditBterm) {
        *pindx = EditBterm->index();
        *pbterm = true;
        return (true);
    }
    return (false);
}


// Export.  Edit the scalar node terminal with the given index number
// and return true if it exists, return false otherwise.
//
bool
SubcState::set_edit_term(int indx, bool bterm)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (false);

    int topn = 0;
    for (int i = 0; i < Nodesize; i++) {
        if (!Nodes[i])
            continue;
        topn++;
    }
    if (topn < 1)
        return (false);

    while (indx >= topn)
        indx -= topn;
    while (indx < 0)
        indx += topn;

    if (bterm) {
        CDp_bsnode *pb = (CDp_bsnode*)cursde->prpty(P_BNODE);
        for ( ; pb; pb = pb->next()) {
            if (pb->index() == (unsigned)indx) {
                Gst()->SetGhost(GFnone);
                SetLevel2();
                edit_bterm(pb, false);
                State = 3;
                return (true);;
            }
        }
        return (false);
    }
    int cnt = 0;
    for (int i = 0; i < topn; i++) {
        if (!Nodes[i])
            continue;
        if (cnt == indx) {
            Gst()->SetGhost(GFnone);
            SetLevel2();
            edit_terminal(Nodes[i], false);
            State = 3;
            return (true);
        }
        cnt++;
    }
    return (false);
}


bool
SubcState::delete_terminal(CDp_snode *pn)
{
    if (!pn)
        return (false);
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (false);
    if (cursde->isSymbolic())
        return (false);

    int cnt = 0, index = 0;;
    for (int i = 0; i < Nodesize; i++) {
        if (!Nodes[i])
            continue;
        if (pn == Nodes[i]) {
            index = i;
            break;
        }
        cnt++;
    }

    show_terms(ERASE, cnt);
    cursde->prptyUnlink(pn);
    if (EditNode == Nodes[index]) {
        EditNode = 0;
        TermEditInfo tinfo((CDp_snode*)0, 0);
        SCD()->PopUpTermEdit(0, MODE_UPD, &tinfo, te_cb, 0, 0, 0);
    }
    Nodes[index] = 0;
    Opers = new sNop(N_DEL, index, pn, Opers);
    show_terms(DISPLAY, cnt);

    ED()->PopUpCellProperties(MODE_UPD);
    Changed = true;
    return (true);
}


bool
SubcState::delete_bterm(CDp_bsnode *pn)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (false);

    show_bterms(ERASE);
    if (cursde->isSymbolic())
        return (false);
    cursde->prptyUnlink(pn);
    if (EditBterm == pn) {
        EditBterm = 0;
        TermEditInfo tinfo((CDp_bsnode*)0);
        SCD()->PopUpTermEdit(0, MODE_UPD, &tinfo, te_cb, 0, 0, 0);
    }
    Opers = new sNop(N_BDEL, pn->index(), pn, Opers);
    show_bterms(DISPLAY);

    ED()->PopUpCellProperties(MODE_UPD);
    Changed = true;
    return (true);
}


// If any bit terminals are missing, create them.  New terminals are
// appended to the existing list.  This will reset the bus terminal
// index to the index of the bit, if different.
//
bool
SubcState::check_bterm_bits()
{
    if (!EditBterm) {
        Errs()->add_error("no current bus terminal");
        return (false);
    }
    CDp_bsnode *pb = EditBterm;

    if (!pb->has_name()) {
        Errs()->add_error("current bus terminal has no name");
        return (false);
    }

    bool ret = true;
    int added = 0;
    sNop *otmp = Opers;
    Opers = 0;
    CDnetexGen ngen(pb);
    CDnetName nm;
    int n;
    unsigned int indx = 0;
    while (ngen.next(&nm, &n)) {

        int cnt = 0;
        bool found = false;
        for (int i = 0; i < Nodesize; i++) {
            if (!Nodes[i])
                continue;
            CDp_snode *ps = Nodes[i];
            if (ps->get_term_name()) {
                CDnetName tnm;
                int tn;
                if (!CDnetex::parse_bit(Tstring(ps->get_term_name()),
                        &tnm, &tn)) {
                    Errs()->get_error();
                    cnt++;
                    continue;
                }
                if (nm == tnm && n == tn) {
                    found = true;
                    break;
                }
            }
            cnt++;
        }
        if (!found) {
            CDnetName tnm = CDnetex::mk_name(Tstring(nm), n);
            CDp_snode *pn = new CDp_snode;
            pn->CDp_node::set_term_name(tnm);
            if (!insert_terminal(EditBterm, pn)) {
                ret = false;
                break;
            }
            added++;
        }
        if (indx == 0)
            pb->set_index(cnt);
        indx++;
    }
    if (Opers)
        Opers = new sNop(Opers, otmp);
    else
        Opers = otmp;
    ED()->PopUpCellProperties(MODE_UPD);
    edit_bterm(pb, false);

    if (ret) {
        int xx, yy;
        Menu()->PointerRootLoc(&xx, &yy);
        PL()->FlashMessageHereV(xx, yy, "Added %d new properties", added);
    }
    return (ret);
}


// As above, but reorder the terminal list so that bit terminals are
// indexed in order starting with the bus terminal index.
//
bool
SubcState::order_bterm_bits()
{
    if (!EditBterm) {
        Errs()->add_error("no current bus terminal");
        return (false);
    }
    CDp_bsnode *pb = EditBterm;

    if (!pb->has_name()) {
        Errs()->add_error("current bus terminal has no name");
        return (false);
    }

    {
        // Count existing nodes.
        unsigned int ncnt = 0;
        for (int i = 0; i < Nodesize; i++) {
            if (!Nodes[i])
                continue;
            ncnt++;
        }

        // We need to test for duplicate bit names in the netex.
        SymTab tab(false, false);

        int needed = 0;
        CDnetexGen ngen(pb);
        CDnetName nm;
        int n;
        while (ngen.next(&nm, &n)) {
            CDnetName nnm = CDnetex::mk_name(Tstring(nm), n);
            if (SymTab::get(&tab, (uintptr_t)nnm) == ST_NIL)
                tab.add((uintptr_t)nnm, 0, false);
            else
                continue;

            bool found = false;
            for (int i = 0; i < Nodesize; i++) {
                if (!Nodes[i])
                    continue;
                CDp_snode *ps = Nodes[i];
                if (ps->get_term_name()) {
                    CDnetName tnm;
                    int tn;
                    if (!CDnetex::parse_bit(Tstring(ps->get_term_name()),
                            &tnm, &tn)) {
                        Errs()->get_error();
                        continue;
                    }
                    if (nm == tnm && n == tn) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found)
                needed++;
        }
        if (pb->index() + pb->width() >= ncnt + needed) {
            Errs()->add_error("current bus terminal index too large, max %d",
                ncnt - pb->width() - 1);
            return (false);
        }
    }

    // We need to test for duplicate bit names in the netex.
    SymTab tab(false, false);

    bool ret = true;
    int added = 0;
    show_terms(ERASE, -1);
    sNop *otmp = Opers;
    Opers = 0;
    CDnetexGen ngen(pb);
    CDnetName nm;
    int n;
    unsigned int indx = pb->index();
    while (ngen.next(&nm, &n)) {
        CDnetName nnm = CDnetex::mk_name(Tstring(nm), n);
        if (SymTab::get(&tab, (uintptr_t)nnm) == ST_NIL)
            tab.add((uintptr_t)nnm, 0, false);
        else
            continue;

        int node = -1;
        CDp_snode *pn = 0;
        for (int i = 0; i < Nodesize; i++) {
            if (!Nodes[i])
                continue;
            CDp_snode *ps = Nodes[i];
            if (ps->get_term_name()) {
                CDnetName tnm;
                int tn;
                if (!CDnetex::parse_bit(Tstring(ps->get_term_name()),
                        &tnm, &tn)) {
                    Errs()->get_error();
                    continue;
                }
                if (nm == tnm && n == tn) {
                    pn = ps;
                    node = i;
                    break;
                }
            }
        }
        if (!pn) {
            pn = new CDp_snode;
            pn->CDp_node::set_term_name(nnm);
            if (!insert_terminal(EditBterm, pn)) {
                ret = false;
                break;
            }
            added++;
        }
        change_index(pn, indx);
        int newindex = pn->index();
        if (newindex != node) {
            Opers = new sNop(N_RNM, newindex, pn, Opers);
            Opers->name = pn->get_term_name();
            Opers->flags = pn->term_flags();
            Opers->oindx = node;
        }
        indx++;
    }
    if (Opers)
        Opers = new sNop(Opers, otmp);
    else
        Opers = otmp;
    show_terms(DISPLAY, -1);
    ED()->PopUpCellProperties(MODE_UPD);
    edit_bterm(pb, false);

    if (ret) {
        int xx, yy;
        Menu()->PointerRootLoc(&xx, &yy);
        PL()->FlashMessageHereV(xx, yy, "properties reordered, %d created",
            added);
    }
    return (ret);
}


bool
SubcState::bterm_bits_flags(bool set, int flag)
{
    if (!EditBterm)
        return (false);
    CDp_bsnode *pb = EditBterm;

    flag &= (TE_BYNAME | TE_SCINVIS | TE_SYINVIS);
    if (!flag)
        return (true);

    if (!pb->has_name())
        return (false);

    show_bterms(ERASE);
    show_terms(ERASE, -1);
    CDnetexGen ngen(pb);
    CDnetName nm;
    int n;
    while (ngen.next(&nm, &n)) {

        for (int i = 0; i < Nodesize; i++) {
            if (!Nodes[i])
                continue;
            CDp_snode *ps = Nodes[i];
            if (ps->get_term_name()) {
                CDnetName tnm;
                int tn;
                if (!CDnetex::parse_bit(Tstring(ps->get_term_name()),
                        &tnm, &tn)) {
                    Errs()->get_error();
                    continue;
                }
                if (nm == tnm && n == tn) {
                    if (set)
                        ps->set_flag(flag);
                    else
                        ps->unset_flag(flag);
                    break;
                }
            }
        }
    }
    show_terms(DISPLAY, -1);
    show_bterms(DISPLAY);
    ED()->PopUpCellProperties(MODE_UPD);
    return (true);
}


bool
SubcState::insert_terminal(CDp_bsnode *pb, CDp_snode *pn)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde) {
        Errs()->add_error("no current cell");
        return (false);
    }
    if (cursde->isSymbolic()) {
        Errs()->add_error("can't add terminal from symbol view");
        return (false);
    }

    int last = -1;
    int count = 0;
    for (int i = 0; i < Nodesize; i++) {
        if (Nodes[i]) {
            last = i;
            count++;
        }
    }
    last++;
    if (last == Nodesize) {
        CDp_snode **n = new CDp_snode*[2*Nodesize];
        int i;
        for (i = 0; i < Nodesize; i++)
            n[i] = Nodes[i];
        Nodesize *= 2;
        for ( ; i < Nodesize; i++)
            n[i] = 0;
        delete [] Nodes;
        Nodes = n;
    }

    show_terms(ERASE, count);

    int x, y;
    pb->get_schem_pos(&x, &y);
    pn->set_schem_pos(x, y);
    pn->set_index(last);
    pn->set_flag(TE_SCINVIS | TE_SYINVIS | TE_BYNAME);
    pn->set_next_prp(cursde->prptyList());
    cursde->setPrptyList(pn);

    Changed = true;
    Nodes[last] = pn;
    show_terms(DISPLAY, count);
    Opers = new sNop(N_ADD, last, Nodes[last], Opers);
    ED()->PopUpCellProperties(MODE_UPD);
    return (true);
}


bool
SubcState::change_index(CDp_snode *pn, int newix)
{
    int cnt = 0;
    for (int i = 0; i < Nodesize; i++) {
        if (!Nodes[i])
            continue;
        cnt++;
    }
    if (newix < 0 || newix >= cnt)
        return (false);

    for (int i = 0; i < Nodesize; i++) {
        if (!Nodes[i])
            continue;
        if (Nodes[i] == pn) {
            for ( ; i+1 < Nodesize; i++) {
                Nodes[i] = Nodes[i+1];
                if (Nodes[i])
                    Nodes[i]->set_index(i);
            }
            Nodes[Nodesize - 1] = 0;
            break;
        }
    }
    cnt = 0;
    for (int i = 0; i < Nodesize; i++) {
        if (cnt == newix) {
            if (Nodes[i]) {
                for (int j = Nodesize-1; j > i; j--) {
                    Nodes[j] = Nodes[j-1];
                    if (Nodes[j])
                        Nodes[j]->set_index(j);
                }
            }
            Nodes[i] = pn;
            Nodes[i]->set_index(i);
            break;
        }
        if (!Nodes[i])
            continue;
        cnt++;
    }
    return (true);
}


// Button 1 press response.
//
void
SubcState::b1down()
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;
    int x, y;
    EV()->Cursor().get_xy(&x, &y);
    unsigned int downstate = EV()->Cursor().get_downstate();
    if (Level == 1) {
        State = 0;
        if (find_term(x, y) || find_bterm(x, y)) {
            if (downstate & GR_SHIFT_MASK) {
                int index, count;
                CDp_snode *pn = find_term(x, y, &index, &count);
                if (pn) {
                    Gst()->SetGhost(GFnone);
                    SetLevel2();
                    edit_terminal(pn, false);
                    State = 3;
                    return;
                }
                CDp_bsnode *pb = find_bterm(x, y);
                if (pb) {
                    Gst()->SetGhost(GFnone);
                    SetLevel2();
                    edit_bterm(pb, false);
                    State = 3;
                    return;
                }
            }
            Lastx = x;
            Lasty = y;
            Gst()->SetGhost(GFeterms);
            EV()->DownTimer(GFeterms);
            State = 1;
        }
        else if (downstate & GR_SHIFT_MASK) {
            if (!cursde->isSymbolic())
                insert_bterm(x, y);
        }
        else if ((downstate & GR_CONTROL_MASK) || cursde->isDevice() ||
                selectNode(&x, &y)) {
            if (!cursde->isSymbolic())
                insert_terminal(x, y);
        }
    }
    else {
        CDp_snode *pn = find_term(Lastx, Lasty);
        if (pn) {
            if (!pn->has_flag(TE_BYNAME)) {
                if (x != Lastx || y != Lasty) {
                    if (find_term(x, y, true) || find_bterm(x, y)) {
                        int xx, yy;
                        Menu()->PointerRootLoc(&xx, &yy);
                        PL()->FlashMessageHere(xx, yy,
                            "Coincident terminals not allowed!");
                        return;
                    }
                }
            }
            if (cursde->isSymbolic()) {
                bool cpy = (downstate & (GR_CONTROL_MASK | GR_SHIFT_MASK));
                if (move_terminal(Lastx, Lasty, x, y, cpy)) {
                    Gst()->SetGhost(GFnone);
                    State = 3;
                }
                return;
            }
            if ((downstate & GR_CONTROL_MASK) || cursde->isDevice() ||
                    pn->has_flag(TE_BYNAME) || selectNode(&x, &y)) {
                if (move_terminal(Lastx, Lasty, x, y, false)) {
                    Gst()->SetGhost(GFnone);
                    State = 3;
                }
            }
            return;
        }

        CDp_bsnode *pb = find_bterm(Lastx, Lasty);
        if (pb) {
            if (x != Lastx || y != Lasty) {
                if (find_term(x, y, true) || find_bterm(x, y)) {
                    int xx, yy;
                    Menu()->PointerRootLoc(&xx, &yy);
                    PL()->FlashMessageHere(xx, yy,
                        "Coincident terminals not allowed!");
                    return;
                }
            }
            if (cursde->isSymbolic()) {
                bool cpy = (downstate & (GR_CONTROL_MASK | GR_SHIFT_MASK));
                if (move_bterm(Lastx, Lasty, x, y, cpy)) {
                    Gst()->SetGhost(GFnone);
                    State = 3;
                }
                return;
            }
            if (move_bterm(Lastx, Lasty, x, y, false)) {
                Gst()->SetGhost(GFnone);
                State = 3;
            }
        }
    }
}


// Button 1 release response.
//
void
SubcState::b1up()
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;
    if (Level == 1) {
        if (!State)
            return;
        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            EV()->CurrentWin()->Snap(&x, &y);
            unsigned int upstate = EV()->Cursor().get_upstate();

            CDp_snode *pn = find_term(Lastx, Lasty);
            if (pn) {
                if (!pn->has_flag(TE_BYNAME)) {
                    if (x != Lastx || y != Lasty) {
                        if (find_term(x, y, true) || find_bterm(x, y)) {
                            int xx, yy;
                            Menu()->PointerRootLoc(&xx, &yy);
                            PL()->FlashMessageHere(xx, yy,
                                "Coincident terminals not allowed!");
                            return;
                        }
                    }
                }
                if (cursde->isSymbolic()) {
                    bool cpy = (upstate & (GR_CONTROL_MASK | GR_SHIFT_MASK));
                    if (move_terminal(Lastx, Lasty, x, y, cpy)) {
                        Gst()->SetGhost(GFnone);
                        State = 2;
                    }
                    else {
                        SetLevel2();
                    }
                    return;
                }
                if ((upstate & GR_CONTROL_MASK) || cursde->isDevice() ||
                        pn->has_flag(TE_BYNAME) || selectNode(&x, &y)) {
                    if (move_terminal(Lastx, Lasty, x, y, false)) {
                        Gst()->SetGhost(GFnone);
                        State = 2;
                    }
                    else {
                        SetLevel2();
                    }
                }
                return;
            }
                
            CDp_bsnode *pb = find_bterm(Lastx, Lasty);
            if (pb) {
                if (x != Lastx || y != Lasty) {
                    if (find_term(x, y, true) || find_bterm(x, y)) {
                        int xx, yy;
                        Menu()->PointerRootLoc(&xx, &yy);
                        PL()->FlashMessageHere(xx, yy,
                            "Coincident terminals not allowed!");
                        return;
                    }
                }
                if (cursde->isSymbolic()) {
                    bool cpy = (upstate & (GR_CONTROL_MASK | GR_SHIFT_MASK));
                    if (move_bterm(Lastx, Lasty, x, y, cpy)) {
                        Gst()->SetGhost(GFnone);
                        State = 2;
                    }
                    return;
                }
                if (move_bterm(Lastx, Lasty, x, y, false)) {
                    Gst()->SetGhost(GFnone);
                    State = 2;
                    return;
                }
            }
        }
        SetLevel2();
    }
    else {
        // If a terminal was moved, reset the state for the next operation.
        //
        if (State == 1) {
            int x, y;
            EV()->Cursor().get_xy(&x, &y);
            if (x == Lastx && y == Lasty) {
                int index, count;
                CDp_snode *pn = find_term(Lastx, Lasty, &index, &count);
                if (pn) {
                    Gst()->SetGhost(GFnone);
                    SetLevel1();
                    edit_terminal(pn, false);
                    State = 0;
                }
                CDp_bsnode *pb = find_bterm(Lastx, Lasty);
                if (pb) {
                    Gst()->SetGhost(GFnone);
                    SetLevel1();
                    edit_bterm(pb, false);
                    State = 0;
                }
            }
        }
        if (State == 3) {
            if (SubcCmd) {
                // may have Esc'ed
                State = 2;
                SetLevel1();
            }
        }
    }
}


// Exit code.
//
void
SubcState::esc()
{
    if (State == 1) {
        // When moving, just exit state.
        Gst()->SetGhost(GFnone);
        Level = 1;
        State = 0;
        return;
    }
    CDs *cursde = CurCell(Electrical, true);
    if (!cursde)
        return;
    DSP()->SetShowInvisMarks(false);
    SCD()->PopUpTermEdit(0, MODE_OFF, 0, 0, 0, 0, 0);
    Gst()->SetGhost(GFnone);
    show_connections(ERASE);
    update();
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    if (DSP()->ShowTerminals())
        // add the cell's marks only
        DSP()->ShowCellTerminalMarks(DISPLAY);
    sNop::freeUndo(Opers);
    sNop::freeRedo(Redos);
    delete this;
}


bool
SubcState::key(int code, const char*, int)  
{
    switch (code) {
    case DELETE_KEY:
        if ((Level == 1 || Level == 2) && State == 1) {
            if (find_term(Lastx, Lasty)) {
                Gst()->SetGhost(GFnone);
                delete_terminal(Lastx, Lasty);
                State = 0;
                SetLevel1();
                return (true);
            }
            if (find_bterm(Lastx, Lasty)) {
                Gst()->SetGhost(GFnone);
                delete_bterm(Lastx, Lasty);
                State = 0;
                SetLevel1();
                return (true);
            }
        }
        break;

    case BSP_KEY:
        if ((Level == 1 || Level == 2) && State == 1) {
            // When moving, just exit state.
            Gst()->SetGhost(GFnone);
            State = 0;
            SetLevel1();
            return (true);
        }
        break;

    case PAGEDN_KEY:
    case PAGEUP_KEY:
        show_terms(ERASE, 0);
        ShowInvis = !ShowInvis;
        DSP()->SetShowInvisMarks(ShowInvis);
        show_terms(DISPLAY, 0);
        break;
    }
    return (false);
}


void
SubcState::undo()
{
    if (!Opers) {
        PL()->ShowPrompt("No more terminal actions to undo.");
        return;
    }
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;
    sNop *op = Opers;
    Opers = Opers->next;
    int cnt;
    switch (op->type) {
    case N_BAT:
        {
            bool tv = EditVisible;
            EditVisible = false;
            Ulev++;
            sNop *otmp = Opers;
            Opers = op->batch;
            sNop *rtmp = Redos;
            Redos = 0;

            while (Opers)
                undo();
            op->batch = Redos;
            Redos = rtmp;
            Opers = otmp;
            Ulev--;
            EditVisible = tv;
        }
        break;
    case N_ADD:
        cnt = term_disp_num(op->indx);
        show_terms(ERASE, cnt);
        if (!Nodes[op->indx]) {
            for (int i = op->indx; i < Nodesize - 1; i++) {
                Nodes[i] = Nodes[i+1];
                if (Nodes[i])
                    Nodes[i]->set_index(i);
            }
        }
        cursde->prptyUnlink(op->prp);
        if (EditNode == Nodes[op->indx])
            EditNode = 0;
        Nodes[op->indx] = 0;
        show_terms(DISPLAY, cnt);
        break;
    case N_DEL:
        cnt = term_disp_num(op->indx);
        show_terms(ERASE, cnt);
        Nodes[op->indx] = op->prp;
        op->prp->set_next_prp(cursde->prptyList());
        cursde->setPrptyList(op->prp);
        show_terms(DISPLAY, cnt);
        break;
    case N_MOV:
        cnt = term_disp_num(op->indx);
        show_terms(ERASE, cnt);
        {
            CDp_snode *pn = Nodes[op->indx];
            int px, py;
            pn->get_schem_pos(&px, &py);
            pn->set_schem_pos(op->loc.x, op->loc.y);
            op->loc.x = px;
            op->loc.y = py;

            Plist *p0 = 0, *pe = 0;
            for (unsigned int ix = 0; ; ix++) {
                if (!pn->get_pos(ix, &px, &py))
                    break;
                if (!p0)
                    p0 = pe = new Plist(px, py, 0);
                else {
                    pe->next = new Plist(px, py, 0);
                    pe = pe->next;
                }
            }

            unsigned int len = 0;
            for (Plist *p = op->sypts; p; p = p->next)
                len++;
            pn->alloc_pos(len);
            unsigned int ix = 0;
            for (Plist *p = op->sypts; p; p = p->next) {
                pn->set_pos(ix, p->x, p->y);
                ix++;
            }
            Plist::destroy(op->sypts);
            op->sypts = p0;
        }
        show_terms(DISPLAY, cnt);
        break;
    case N_RNM:
        show_terms(ERASE, -1);
        DSP()->HliteElecBsc(ERASE, EditBterm);
        EditNode = 0;
        EditBterm = 0;
        {
            CDp_snode *ps = Nodes[op->indx];
            CDnetName n = ps->get_term_name();
            ps->CDp_node::set_term_name(op->name);
            op->name = n;
            unsigned int fbak = ps->term_flags();
            if (ps->cell_terminal())
                ps->cell_terminal()->set_fixed(op->flags & TE_FIXED);
            if (op->flags & TE_BYNAME)
                ps->set_flag(TE_BYNAME);
            else
                ps->unset_flag(TE_BYNAME);
            if (op->flags & TE_SCINVIS)
                ps->set_flag(TE_SCINVIS);
            else
                ps->unset_flag(TE_SCINVIS);
            if (op->flags & TE_SYINVIS)
                ps->set_flag(TE_SYINVIS);
            else
                ps->unset_flag(TE_SYINVIS);
            op->flags = fbak;
            if (EditVisible)
                EditNode = Nodes[op->indx];

            if (op->indx != op->oindx) {
                for (int i = op->indx; i+1 < Nodesize; i++) {
                    Nodes[i] = Nodes[i+1];
                    if (Nodes[i])
                        Nodes[i]->set_index(i);
                }
                Nodes[Nodesize-1] = 0;

                if (Nodes[op->oindx]) {
                    for (unsigned int i = Nodesize-1; i > op->oindx; i--) {
                        Nodes[i] = Nodes[i-1];
                        if (Nodes[i])
                            Nodes[i]->set_index(i);
                    }
                }
                Nodes[op->oindx] = ps;
                ps->set_index(op->oindx);
                int tmp = op->oindx;
                op->oindx = op->indx;
                op->indx = tmp;
            }

            cursde->unsetConnected();
            SCD()->PopUpNodeMap(0, MODE_UPD);
        }
        show_terms(DISPLAY, -1);
        edit_terminal(EditNode, false);
        break;
    case N_BADD:
        show_bterms(ERASE);
        cursde->prptyUnlink(op->bprp);
        if (EditBterm == op->bprp)
            EditBterm = 0;
        show_bterms(DISPLAY);
        break;
    case N_BDEL:
        show_bterms(ERASE);
        op->bprp->set_next_prp(cursde->prptyList());
        cursde->setPrptyList(op->bprp);
        show_bterms(DISPLAY);
        break;
    case N_BMOV:
        show_bterms(ERASE);
        {
            CDp_bsnode *pn = op->bprp;
            int px, py;
            pn->get_schem_pos(&px, &py);
            pn->set_schem_pos(op->loc.x, op->loc.y);
            op->loc.x = px;
            op->loc.y = py;

            Plist *p0 = 0, *pe = 0;
            for (unsigned int ix = 0; ; ix++) {
                if (!pn->get_pos(ix, &px, &py))
                    break;
                if (!p0)
                    p0 = pe = new Plist(px, py, 0);
                else {
                    pe->next = new Plist(px, py, 0);
                    pe = pe->next;
                }
            }

            unsigned int len = 0;
            for (Plist *p = op->sypts; p; p = p->next)
                len++;
            pn->alloc_pos(len);
            unsigned int ix = 0;
            for (Plist *p = op->sypts; p; p = p->next) {
                pn->set_pos(ix, p->x, p->y);
                ix++;
            }
            Plist::destroy(op->sypts);
            op->sypts = p0;
        }
        show_bterms(DISPLAY);
        break;
    case N_BRNM:
        show_bterms(ERASE);
        DSP()->HliteElecTerm(ERASE, EditNode, 0, 0);
        EditNode = 0;
        EditBterm = 0;
        {
            unsigned int tindx = op->bprp->index();
            unsigned int tbeg = op->bprp->beg_range();
            unsigned int tend = op->bprp->end_range();
            CDnetName tname = op->bprp->get_term_name();
            CDnetex *tnx = CDnetex::dup(op->bprp->bundle_spec());
            op->bprp->set_index(op->indx);
            if (op->netexp)
                op->bprp->update_bundle(op->netexp);
            else
                op->bprp->set_range(op->beg, op->end);
            op->indx = tindx;
            op->beg = tbeg;
            op->end = tend;
            op->name = tname;
            op->netexp = tnx;
            unsigned int fbak = op->bprp->flags();
            if (op->flags & TE_SCINVIS)
                op->bprp->set_flag(TE_SCINVIS);
            else
                op->bprp->unset_flag(TE_SCINVIS);
            if (op->flags & TE_SYINVIS)
                op->bprp->set_flag(TE_SYINVIS);
            else
                op->bprp->unset_flag(TE_SYINVIS);
            op->flags = fbak;
            if (EditVisible)
                EditBterm = op->bprp;
        }
        show_bterms(DISPLAY);
        edit_bterm(EditBterm, true);
        break;

    default:
        break;
    }
    op->next = Redos;
    Redos = op;
    if (Ulev == 0)
        ED()->PopUpCellProperties(MODE_UPD);
}


void
SubcState::redo()
{
    if (!Redos) {
        PL()->ShowPrompt("No more terminal actions to redo.");
        return;
    }
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;
    sNop *op = Redos;
    Redos = Redos->next;
    int cnt;
    switch (op->type) {
    case N_BAT:
        {
            bool tv = EditVisible;
            EditVisible = false;
            Ulev++;
            sNop *rtmp = Redos;
            Redos = op->batch;
            sNop *otmp = Opers;
            Opers = 0;

            while (Redos)
                redo();
            op->batch = Opers;
            Opers = otmp;
            Redos = rtmp;
            Ulev--;
            EditVisible = tv;
        }
        break;
    case N_ADD:
        cnt = term_disp_num(op->indx);
        show_terms(ERASE, cnt);
        if (Nodes[op->indx]) {
            for (unsigned int i = Nodesize - 1; i > op->indx; i--) {
                Nodes[i] = Nodes[i-1];
                if (Nodes[i])
                    Nodes[i]->set_index(i);
            }
            Nodes[op->indx] = 0;
        }
        Nodes[op->indx] = op->prp;
        op->prp->set_next_prp(cursde->prptyList());
        cursde->setPrptyList(op->prp);
        show_terms(DISPLAY, cnt);
        break;
    case N_DEL:
        cnt = term_disp_num(op->indx);
        show_terms(ERASE, cnt);
        cursde->prptyUnlink(op->prp);
        Nodes[op->indx] = 0;
        show_terms(DISPLAY, cnt);
        break;
    case N_MOV:
        cnt = term_disp_num(op->indx);
        show_terms(ERASE, cnt);
        {
            CDp_snode *pn = Nodes[op->indx];
            int px, py;
            pn->get_schem_pos(&px, &py);
            pn->set_schem_pos(op->loc.x, op->loc.y);
            op->loc.x = px;
            op->loc.y = py;

            Plist *p0 = 0, *pe = 0;
            for (unsigned int ix = 0; ; ix++) {
                if (!pn->get_pos(ix, &px, &py))
                    break;
                if (!p0)
                    p0 = pe = new Plist(px, py, 0);
                else {
                    pe->next = new Plist(px, py, 0);
                    pe = pe->next;
                }
            }

            unsigned int len = 0;
            for (Plist *p = op->sypts; p; p = p->next)
                len++;
            pn->alloc_pos(len);
            unsigned int ix = 0;
            for (Plist *p = op->sypts; p; p = p->next) {
                pn->set_pos(ix, p->x, p->y);
                ix++;
            }
            Plist::destroy(op->sypts);
            op->sypts = p0;
        }
        show_terms(DISPLAY, cnt);
        break;
    case N_RNM:
        show_terms(ERASE, -1);
        DSP()->HliteElecBsc(ERASE, EditBterm);
        EditNode = 0;
        EditBterm = 0;
        {
            CDp_snode *ps = Nodes[op->indx];
            CDnetName n = ps->get_term_name();
            ps->CDp_node::set_term_name(op->name);
            op->name = n;
            unsigned int fbak = ps->term_flags();
            if (ps->cell_terminal())
                ps->cell_terminal()->set_fixed(op->flags & TE_FIXED);
            if (op->flags & TE_BYNAME)
                ps->set_flag(TE_BYNAME);
            else
                ps->unset_flag(TE_BYNAME);
            if (op->flags & TE_SCINVIS)
                ps->set_flag(TE_SCINVIS);
            else
                ps->unset_flag(TE_SCINVIS);
            if (op->flags & TE_SYINVIS)
                ps->set_flag(TE_SYINVIS);
            else
                ps->unset_flag(TE_SYINVIS);
            op->flags = fbak;
            if (EditVisible)
                EditNode = Nodes[op->indx];

            if (op->indx != op->oindx) {
                for (int i = op->indx ; i+1 < Nodesize; i++) {
                    Nodes[i] = Nodes[i+1];
                    if (Nodes[i])
                        Nodes[i]->set_index(i);
                }
                Nodes[Nodesize-1] = 0;

                if (Nodes[op->oindx]) {
                    for (unsigned int i = Nodesize-1; i > op->oindx; i--) {
                        Nodes[i] = Nodes[i-1];
                        if (Nodes[i])
                            Nodes[i]->set_index(i);
                    }
                }
                Nodes[op->oindx] = ps;
                ps->set_index(op->oindx);
                int tmp = op->oindx;
                op->oindx = op->indx;
                op->indx = tmp;
            }

            cursde->unsetConnected();
            SCD()->PopUpNodeMap(0, MODE_UPD);
        }
        show_terms(DISPLAY, -1);
        edit_terminal(EditNode, false);
        break;
    case N_BADD:
        show_bterms(ERASE);
        op->bprp->set_next_prp(cursde->prptyList());
        cursde->setPrptyList(op->bprp);
        show_bterms(DISPLAY);
        break;
    case N_BDEL:
        show_bterms(ERASE);
        cursde->prptyUnlink(op->bprp);
        show_bterms(DISPLAY);
        break;
    case N_BMOV:
        show_bterms(ERASE);
        {
            CDp_bsnode *pn = op->bprp;
            int px, py;
            pn->get_schem_pos(&px, &py);
            pn->set_schem_pos(op->loc.x, op->loc.y);
            op->loc.x = px;
            op->loc.y = py;

            Plist *p0 = 0, *pe = 0;
            for (unsigned int ix = 0; ; ix++) {
                if (!pn->get_pos(ix, &px, &py))
                    break;
                if (!p0)
                    p0 = pe = new Plist(px, py, 0);
                else {
                    pe->next = new Plist(px, py, 0);
                    pe = pe->next;
                }
            }

            unsigned int len = 0;
            for (Plist *p = op->sypts; p; p = p->next)
                len++;
            pn->alloc_pos(len);
            unsigned int ix = 0;
            for (Plist *p = op->sypts; p; p = p->next) {
                pn->set_pos(ix, p->x, p->y);
                ix++;
            }
            Plist::destroy(op->sypts);
            op->sypts = p0;
        }
        show_bterms(DISPLAY);
        break;
    case N_BRNM:
        show_bterms(ERASE);
        DSP()->HliteElecTerm(ERASE, EditNode, 0, 0);
        EditNode = 0;
        EditBterm = 0;
        {
            unsigned int tindx = op->bprp->index();
            unsigned int tbeg = op->bprp->beg_range();
            unsigned int tend = op->bprp->end_range();
            CDnetName tname = op->bprp->get_term_name();
            CDnetex *tnx = CDnetex::dup(op->bprp->bundle_spec());
            op->bprp->set_index(op->indx);
            if (op->netexp)
                op->bprp->update_bundle(op->netexp);
            else
                op->bprp->set_range(op->beg, op->end);
            op->indx = tindx;
            op->beg = tbeg;
            op->end = tend;
            op->name = tname;
            op->netexp = tnx;
            unsigned int fbak = op->bprp->flags();
            if (op->flags & TE_SCINVIS)
                op->bprp->set_flag(TE_SCINVIS);
            else
                op->bprp->unset_flag(TE_SCINVIS);
            if (op->flags & TE_SYINVIS)
                op->bprp->set_flag(TE_SYINVIS);
            else
                op->bprp->unset_flag(TE_SYINVIS);
            op->flags = fbak;
            if (EditVisible)
                EditBterm = op->bprp;
        }
        show_bterms(DISPLAY);
        edit_bterm(EditBterm, true);
        break;

    default:
        break;
    }
    op->next = Opers;
    Opers = op;
    if (Ulev == 0)
        ED()->PopUpCellProperties(MODE_UPD);
}


// Save the nodes in the Nodes array.
//
void
SubcState::save_nodes()
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;

    // First the terminals, find largest usable index.
    unsigned int max_val = 0;
    CDp_snode *pn = (CDp_snode*)cursde->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {
        if (pn->index() > P_NODE_MAX_INDEX)
            continue;
        if (pn->index() > max_val)
            max_val = pn->index();
    }
    Nodesize = max_val + 32;
    Nodes = new CDp_snode*[Nodesize];
    memset(Nodes, 0, Nodesize*sizeof(CDp_snode*));

    // Remove node properties into array.
    while ((pn = (CDp_snode*)cursde->prpty(P_NODE)) != 0) {
        cursde->prptyUnlink(pn);
        if (pn->index() > P_NODE_MAX_INDEX) {
            // sanity
            delete pn;
            continue;
        }
        if (!Nodes[pn->index()])
            Nodes[pn->index()] = pn;
        else
            delete pn;  // bad index
    }

    // Put 'em back.
    for (int i = 0; i < Nodesize; i++) {
        if (!Nodes[i])
            continue;
        Nodes[i]->set_next_prp(cursde->prptyList());
        cursde->setPrptyList(Nodes[i]);
    }
}


void
SubcState::update()
{
    if (Changed) {
        CDs *cursde = CurCell(Electrical, true);
        if (!cursde)
            return;

        // add name property if needed
        if (!cursde->prpty(P_NAME))
            cursde->prptyAdd(P_NAME, P_NAME_SUBC_STR);
        // update BB
        CDs *cdsymb = cursde->symbolicRep(0);
        if (cdsymb)
            cdsymb->setBBvalid(false);
        cursde->setBBvalid(false);
        cursde->computeBB();

        // remove any global properties
        ED()->stripGlobalProperties(cursde);
        cursde->reflectTerminals();  // This will compact the index numbers.
        CDs *cursdp = CurCell(Physical);
        if (cursdp)
            cursdp->unsetConnected();
        cursde->unsetConnected();
        Ulist()->CommitChanges();
        cursde->reflectTermNames();
        SCD()->PopUpNodeMap(0, MODE_UPD);
        Changed = false;
    }
}


// Return the node property being pointed to, or 0.  If a node is
// returned, index is the index in the Nodes array of the node, and count
// is the display number.
//
CDp_snode *
SubcState::find_term(int x, int y, int *index, int *count)
{
    int cnt = 0;
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (0);
    bool issy = cursde->isSymbolic();
    for (int i = 0; i < Nodesize; i++) {
        if (!Nodes[i])
            continue;
        CDp_snode *pn = Nodes[i];
        bool invis = (!issy && pn->has_flag(TE_SCINVIS)) ||
                (issy && pn->has_flag(TE_SYINVIS));
        if (invis && !DSP()->ShowInvisMarks())
            continue;
        if (issy) {
            for (unsigned int ix = 0; ; ix++) {
                int xx, yy;
                if (!pn->get_pos(ix, &xx, &yy))
                    break;
                if (xx == x && yy == y) {
                    *count = cnt;
                    *index = i;
                    return (pn);
                }
            }
        }
        else {
            int xx, yy;
            pn->get_schem_pos(&xx, &yy);
            if (xx == x && yy == y) {
                *count = cnt;
                *index = i;
                return (pn);
            }
        }
        cnt++;
    }
    return (0);
}


// Return the node at x, y if any.  Use the cell for the search, not
// the Nodes list.  If not_by_name, return only terms with the
// TE_BYNAME flag not set.
//
CDp_snode *
SubcState::find_term(int x, int y, bool not_by_name)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (0);
    bool issy = cursde->isSymbolic();
    if (issy) {
        CDp_snode *pn = (CDp_snode*)cursde->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            if (pn->has_flag(TE_SYINVIS) && !DSP()->ShowInvisMarks())
                continue;
            if (not_by_name && pn->has_flag(TE_BYNAME))
                continue;
            for (unsigned int ix = 0; ; ix++) {
                int xx, yy;
                if (!pn->get_pos(ix, &xx, &yy))
                    break;
                if (xx == x && yy == y)
                    return (pn);
            }
        }
    }
    else {
        CDp_snode *pn = (CDp_snode*)cursde->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            if (pn->has_flag(TE_SCINVIS) && !DSP()->ShowInvisMarks())
                continue;
            if (not_by_name && pn->has_flag(TE_BYNAME))
                continue;
            int xx, yy;
            pn->get_schem_pos(&xx, &yy);
            if (xx == x && yy == y)
                return (pn);
        }
    }
    return (0);
}


namespace {
    int keypress_index()
    {
        char buf[32];
        PL()->GetTextBuf(EV()->CurrentWin(), buf);
        bool digits = false;
        for (char *s = buf; *s; s++) {
            if (isdigit(*s))
                digits = true;
            else if (digits) {
                digits = false;
                break;
            }
        }
        return (digits ? atoi(buf) : -1);
    }
}


// Add new terminal, return true if terminal added.
//
bool
SubcState::insert_terminal(int x, int y)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde || cursde->isSymbolic())
        return (false);
    if (find_term(x, y) || find_bterm(x, y))
        return (false);

    int last = -1;
    int count = 0;
    for (int i = 0; i < Nodesize; i++) {
        if (Nodes[i]) {
            last = i;
            count++;
        }
    }
    last++;
    if (last == Nodesize) {
        CDp_snode **n = new CDp_snode*[2*Nodesize];
        int i;
        for (i = 0; i < Nodesize; i++)
            n[i] = Nodes[i];
        Nodesize *= 2;
        for ( ; i < Nodesize; i++)
            n[i] = 0;
        delete [] Nodes;
        Nodes = n;
    }

    int posn = keypress_index();
    if (posn >= count)
        posn = -1;
    if (posn >= 0) {
        last = -1;
        count = 0;
        for (int i = 0; i < Nodesize; i++) {
            if (Nodes[i]) {
                last = i;
                if (count >= posn)
                    break;
                count++;
            }
        }
        if (last >= 0) {
            show_terms(ERASE, count);
            if (count >= posn) {
                for (int i = Nodesize-1; i != last; i--) {
                    Nodes[i] = Nodes[i-1];
                    if (Nodes[i])
                        Nodes[i]->set_index(i);
                }
                Nodes[last] = 0;
            }
        }
        else
            last = 0;
    }
    else
        show_terms(ERASE, count);

    char tbuf[128];
    unsigned int flgs = TE_UNINIT << 8;
    // 5 10 node index ex,, ey,, [flgs.ttype name px py lname]
    snprintf(tbuf, sizeof(tbuf), "-1 %d %d %d 0x%x", last, x, y, flgs);
    cursde->prptyAdd(P_NODE, tbuf);
    Changed = true;
    Nodes[last] = (CDp_snode*)cursde->prptyList();  // new one
    show_terms(DISPLAY, count);
    Opers = new sNop(N_ADD, last, Nodes[last], Opers);
    edit_terminal(Nodes[last], false);
    SCD()->addParentConnection(CurCell(), x, y);
    ED()->PopUpCellProperties(MODE_UPD);
    return (true);
}


// Delete the terminal at x,y, return true if terminal deleted.
//
bool
SubcState::delete_terminal(int x, int y)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (false);
    int index, count;
    CDp_snode *pn = find_term(x, y, &index, &count);
    if (!pn)
        return (false);
    if (!pn)
        return (false);
    show_terms(ERASE, count);
    if (cursde->isSymbolic()) {
        Plist *p0 = 0, *pe = 0;
        for (unsigned int ix = 0; ; ix++) {
            int px, py;
            if (!pn->get_pos(ix, &px, &py))
                break;
            if (!p0)
                p0 = pe = new Plist(px, py, 0);
            else {
                pe->next = new Plist(px, py, 0);
                pe = pe->next;
            }
        }
        if (p0 && !p0->next) {
            // If there is only one pin, skip delete.  The
            // node can only be deleted when not symbolic.
            Plist::destroy(p0);
            show_terms(DISPLAY, count);
            int xx, yy;
            Menu()->PointerRootLoc(&xx, &yy);
            PL()->FlashMessageHere(xx, yy, "Can't delete from symbolic view!");
            return (false);
        }
        Opers = new sNop(N_MOV, index, pn, Opers);
        pn->get_schem_pos(&Opers->loc.x, &Opers->loc.y);
        Opers->sypts = p0;

        unsigned int len = 0;
        for (Plist *p = p0; p; p = p->next) {
            if (p->x == x && p->y == y)
                continue;
            len++;
        }
        pn->alloc_pos(len);
        len = 0;
        for (Plist *p = p0; p; p = p->next) {
            if (p->x == x && p->y == y)
                continue;
            pn->set_pos(len, p->x, p->y);
            len++;
        }
        show_terms(DISPLAY, count);
    }
    else {
        cursde->prptyUnlink(pn);
        if (EditNode == Nodes[index]) {
            EditNode = 0;
            TermEditInfo tinfo((CDp_snode*)0, 0);
            SCD()->PopUpTermEdit(0, MODE_UPD, &tinfo, te_cb, 0, 0, 0);
        }
        Nodes[index] = 0;
        Opers = new sNop(N_DEL, index, pn, Opers);
        show_terms(DISPLAY, count);
    }
    Changed = true;
    ED()->PopUpCellProperties(MODE_UPD);
    return (true);
}


// Move terminal from oldx,oldy to x,y.  return true if terminal
// moved.
//
bool
SubcState::move_terminal(int oldx, int oldy, int x, int y, bool cpy)
{
    if (oldx == x && oldy == y)
        return (false);
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (false);
    if (find_term(x, y) || find_bterm(x, y))
        return (false);

    // Only symbolic view terminals have multiple pins and can be copied.
    bool issym = cursde->isSymbolic();
    if (!issym && cpy)
        return (false);

    int index, count;
    CDp_snode *pn = find_term(oldx, oldy, &index, &count);
    if (!pn)
        return (false);

    Plist *p0 = 0, *pe = 0;
    for (unsigned int ix = 0; ; ix++) {
        int px, py;
        if (!pn->get_pos(ix, &px, &py))
            break;
        // Don't allow coincident pins.
        if (issym && px == x && py == y) {
            Plist::destroy(p0);
            return (false);
        }
        if (!p0)
            p0 = pe = new Plist(px, py, 0);
        else {
            pe->next = new Plist(px, py, 0);
            pe = pe->next;
        }
    }
    Opers = new sNop(N_MOV, index, pn, Opers);
    pn->get_schem_pos(&Opers->loc.x, &Opers->loc.y);
    Opers->sypts = p0;

    show_term(ERASE, pn, count);
    if (issym) {
        if (cpy) {
            unsigned int len = 0;
            for (Plist *p = p0; p; p = p->next)
                len++;
            pn->alloc_pos(len + 1);
            len = 0;
            for (Plist *p = p0; p; p = p->next) {
                pn->set_pos(len, p->x, p->y);
                len++;
            }
            pn->set_pos(len, x, y);
        }
        else {
            for (unsigned int ix = 0; ; ix++) {
                int xx, yy;
                if (!pn->get_pos(ix, &xx, &yy))
                    break;
                if (xx == oldx && yy == oldy) {
                    pn->set_pos(ix, x, y);
                    break;
                }
            }
        }
    }
    else {
        pn->set_schem_pos(x, y);
        if (!cursde->prpty(P_SYMBLC)) {
            // If no P_SYMBLC property, the sy fields should track x,y.
            pn->set_pos(0, x, y);
        }
    }
    show_term(DISPLAY, pn, count);
    Changed = true;
    SCD()->addParentConnection(CurCell(), x, y);
    ED()->PopUpCellProperties(MODE_UPD);
    return (true);
}


// Show the connection points as a numbered box, or erase the
// connection points.  Numdo is a count of how many properties to
// process at the end of the list, set to -1 to process all
// node properties.  This will also take properties from the cell,
// not the Nodes list.
//
void
SubcState::show_terms(bool display, int numdo)
{
    CDs *cursde = CurCell(Electrical, true);
    if (!cursde)
        return;

    // We create marks for both schematic and symbolic, if symbolic
    // exists.
    CDs *sd = cursde->symbolicRep(0);

    int count = 0;
    for (int i = 0; i < Nodesize; i++) {
        CDp_snode *pn = Nodes[i];
        if (!pn)
            continue;
        if (count >= numdo) {
            int x, y;
            if (sd) {
                for (unsigned int ix = 0; ; ix++) {
                    if (!pn->get_pos(ix, &x, &y))
                        break;
                    DSP()->ShowStermMark(display, x, y, HighlightingColor, 40,
                        pn, count);
                }
            }
            pn->get_schem_pos(&x, &y);
            DSP()->ShowEtermMark(display, x, y, HighlightingColor, 40,
                pn, count);
        }
        count++;
    }
    if (EditNode) {
        count = display ? term_disp_num(EditNode->index()) : 0;
        DSP()->HliteElecTerm(display, EditNode, 0, count);
    }
}


void
SubcState::show_term(bool display, CDp_snode *pn, int count)
{
    CDs *cursde = CurCell(Electrical, true);
    if (!cursde)
        return;
    int x, y;
    if (cursde->symbolicRep(0)) {
        for (unsigned int ix = 0; ; ix++) {
            if (!pn->get_pos(ix, &x, &y))
                break;
            DSP()->ShowStermMark(display, x, y, HighlightingColor, 40,
                pn, count);
        }
    }
    pn->get_schem_pos(&x, &y);
    DSP()->ShowEtermMark(display, x, y, HighlightingColor, 40,
        pn, count);
    if (EditNode == pn) {
        count = display ? term_disp_num(EditNode->index()) : 0;
        DSP()->HliteElecTerm(display, EditNode, 0, count);
    }
}


// Static function.
// Calback for property setting pop-up, perform change on the
// currently selected node/terminal.
//
void
SubcState::te_cb(TermEditInfo *tinfo, CDp *prp)
{
    if (!tinfo) {
        // Terminal editor popping down.
        if (SubcCmd) {
            DSP()->HliteElecTerm(ERASE, SubcCmd->EditNode, 0, 0);
            DSP()->HliteElecBsc(ERASE, SubcCmd->EditBterm);
            SubcCmd->EditVisible = false;
            SubcCmd->EditNode = 0;
            SubcCmd->EditBterm = 0;
        }
        return;
    }
    if (!prp)
        return;
    CDcbin cbin(DSP()->CurCellName());
    if (!cbin.elec())
        return;

    const char *tname = tinfo->name();

    if (tinfo->has_bterm()) {
        CDp_bsnode *pb = (CDp_bsnode*)prp;
        const char *nxstr = tinfo->netex();

        // The name and nxstr strings are whatever junk the user
        // entered.  We have to make sense here.

        // For the name string, accept any part of the simple vector
        // form name<b:e>.
        CDnetex *netex;
        if (!CDnetex::parse(tname, &netex)) {
            Log()->ErrorLogV(mh::Properties,
                "Bad name string, %s.", Errs()->get_error());
            return;
        }
        CDnetName name = 0;
        int beg = -1, end = -1;
        if (netex) {
            bool ret = netex->is_scalar(&name) ||
                netex->is_simple(&name, &beg, &end);
            CDnetex::destroy(netex);
            if (!ret) {
                Log()->ErrorLogV(mh::Properties,
                    "Bad name string %s, too complex.", tname);
                return;
            }
        }

        if (!CDnetex::parse(nxstr, &netex)) {
            Log()->PopUpErrV(mh::NetlistCreation,
                "Bad net expression string, %s.", Errs()->get_error());
            return;
        }
        if (netex) {
            CDnetName n = 0;
            int b = -1, e = -1;
            bool ret = netex->is_scalar(&n) || netex->is_simple(&n, &b, &e);
            if (ret) {
                CDnetex::destroy(netex);
                netex = 0;
                if (name && n && name != n) {
                    Log()->ErrorLog(mh::NetlistCreation,
                        "Conflicting names given.");
                    return;
                }
                if (!name)
                    name = n;
                if ((beg >= 0 && b >= 0 && beg != b) ||
                        (end >= 0 && e >= 0 && end != e)) {
                    Log()->ErrorLog(mh::NetlistCreation,
                        "Conflicting ranges given.");
                    return;
                }
                if (beg < 0)
                    beg = b;
                if (end < 0)
                    end = e;
                if (beg < 0) {
                    Log()->ErrorLog(mh::NetlistCreation, "No range given.");
                    return;
                }
                if (end < 0)
                    end = beg;
            }
        }
        if (beg < 0)
            beg = 0;
        if (end < 0)
            end = beg;
        SubcCmd->show_bterms(ERASE);

        unsigned int oidx = pb->index();
        bool changed = (tinfo->index() != oidx);
        pb->set_index(tinfo->index());

        CDnetName oname = pb->get_term_name();
        if (name != oname) {
            changed = true;
            pb->CDp_bnode::set_term_name(name);
        }

        CDnetex *onx = CDnetex::dup(pb->bundle_spec());
        int obeg = pb->beg_range();
        int oend = pb->end_range();

        if (!netex) {
            // simple vector case
            if (pb->bundle_spec())
                changed = true;
            else if ((beg != obeg) || (end != oend))
                changed = true;
            pb->set_range(beg, end);
        }
        else {
            CDnetex *onl = CDnetex::dup(pb->bundle_spec());
            pb->update_bundle(netex);
            if (!CDnetex::cmp(onl, pb->bundle_spec()))
                changed = true;
            CDnetex::destroy(onl);
        }

        unsigned int oflags = pb->flags();
        if (tinfo->has_flag(TE_SCINVIS))
            pb->set_flag(TE_SCINVIS);
        else
            pb->unset_flag(TE_SCINVIS);
        if (tinfo->has_flag(TE_SYINVIS))
            pb->set_flag(TE_SYINVIS);
        else
            pb->unset_flag(TE_SYINVIS);
        if (pb->flags() != oflags)
            changed = true;

        if (changed) {
            SubcCmd->Changed = true;
            SubcCmd->Opers = new sNop(N_BRNM, oidx, pb, SubcCmd->Opers);
            SubcCmd->Opers->beg = obeg;
            SubcCmd->Opers->end = oend;
            SubcCmd->Opers->name = oname;
            SubcCmd->Opers->netexp = onx;
            SubcCmd->Opers->flags = oflags;
        }
        SubcCmd->show_bterms(DISPLAY);
        ED()->PopUpCellProperties(MODE_UPD);
    }
    else {
        CDp_snode *ps = (CDp_snode*)prp;

        if (CDnetex::is_default_name(tname)) {
            // If the terminal uses the default name, keep the
            // field in the property null.
            tname = 0;

            PL()->ShowPromptV("Default name for terminal is %s.",
                Tstring(CDnetex::default_name(ps->index())));
        }
        else {
            CDp_snode *pt = (CDp_snode*)cbin.elec()->prpty(P_NODE);
            for ( ; pt; pt = pt->next()) {
                if (pt == ps)
                    continue;
                if (!pt->get_term_name())
                    continue;
                if (!strcmp(tname, Tstring(pt->get_term_name()))) {
                    Log()->ErrorLogV(mh::NetlistCreation,
                        "Terminal name %s already in use.", tname);
                    return;
                }
            }
        }

        int index = 0;
        for (int i = 0; i < SubcCmd->Nodesize; i++) {
            if (!SubcCmd->Nodes[i])
                continue;
            if (SubcCmd->Nodes[i] == ps) {
                index = i;
                break;
            }
        }
        SubcCmd->change_index(ps, tinfo->index());
        int newindex = ps->index();

        SubcCmd->show_terms(ERASE, -1);

        CDnetName oname = ps->get_term_name();
        unsigned int oflags = ps->term_flags();

        ps->set_term_name(tname);

        if (tinfo->has_phys()) {
            if (!ps->cell_terminal()) {
                if (cbin.phys())
                    ps->set_terminal(cbin.phys()->findPinTerm(oname, true));
                else {
                    ps->set_terminal(new CDsterm(0, oname));
                    ps->set_flag(TE_OWNTERM);
                }
                ps->cell_terminal()->set_uninit(true);
                ps->cell_terminal()->set_node_prpty(ps);
            }
        }
        else
            delete ps->cell_terminal();

        CDsterm *term = ps->cell_terminal();
        if (term) {
            CDl *ld = CDldb()->findLayer(tinfo->layer_name(), Physical);
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
        }
        if (tinfo->has_flag(TE_BYNAME))
            ps->set_flag(TE_BYNAME);
        else
            ps->unset_flag(TE_BYNAME);
        if (tinfo->has_flag(TE_SCINVIS))
            ps->set_flag(TE_SCINVIS);
        else
            ps->unset_flag(TE_SCINVIS);
        if (tinfo->has_flag(TE_SYINVIS))
            ps->set_flag(TE_SYINVIS);
        else
            ps->unset_flag(TE_SYINVIS);

        if (ps->get_term_name() != oname || ps->term_flags() != oflags ||
                index != newindex) {
            SubcCmd->Changed = true;
            SubcCmd->Opers = new sNop(N_RNM, newindex, ps, SubcCmd->Opers);
            SubcCmd->Opers->name = oname;
            SubcCmd->Opers->flags = oflags;
            SubcCmd->Opers->oindx = index;
        }
        cbin.elec()->unsetConnected();
        SCD()->PopUpNodeMap(0, MODE_UPD);
        ED()->PopUpCellProperties(MODE_UPD);
        SubcCmd->show_terms(DISPLAY, -1);
    }
}


// Pop up the terminal editor so user can enter name and flags.  If
// the editor is already visible, it will be updated for the new
// terminal.
//
void
SubcState::edit_terminal(CDp_snode *pn, bool upd)
{
    if (!pn)
        return;
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;
    DSP()->HliteElecTerm(ERASE, EditNode, 0, 0);
    DSP()->HliteElecBsc(ERASE, EditBterm);
    EditNode = 0;
    EditBterm = 0;
    int x, y;
    Menu()->PointerRootLoc(&x, &y);

    int cnt = 0;
    for (int i = 0; i < Nodesize; i++) {
        if (!Nodes[i])
            continue;
        if (Nodes[i] == pn)
            break;
        cnt++;
    }

    TermEditInfo tinfo(pn, cnt);
    SCD()->PopUpTermEdit(0, upd ? MODE_UPD : MODE_ON, &tinfo, te_cb, pn,
        x + 50, y + 20);
    if (!upd) {
        EditVisible = true;
        EditNode = pn;
        DSP()->HliteElecTerm(DISPLAY, pn, 0, cnt);
    }
}


int
SubcState::term_disp_num(int index)
{
    int cnt = 0;
    for (int i = 0; i < index; i++) {
        if (Nodes[i])
            cnt++;
    }
    return (cnt);
}


// Return the property for a bterm located at x,y if any.
//
CDp_bsnode *
SubcState::find_bterm(int x, int y)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (0);
    bool issy = cursde->isSymbolic();
    if (issy) {
        CDp_bsnode *pn = (CDp_bsnode*)cursde->prpty(P_BNODE);
        for ( ; pn; pn = pn->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int xx, yy;
                if (!pn->get_pos(ix, &xx, &yy))
                    break;
                if (xx == x && yy == y)
                    return (pn);
            }
        }
    }
    else {
        CDp_bsnode *pn = (CDp_bsnode*)cursde->prpty(P_BNODE);
        for ( ; pn; pn = pn->next()) {
            int xx, yy;
            pn->get_schem_pos(&xx, &yy);
            if (xx == x && yy == y)
                return (pn);
        }
    }
    return (0);
}


// Insert/delete function for bus terminals.
//
bool
SubcState::insert_bterm(int x, int y)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde || cursde->isSymbolic())
        return (false);
    if (find_term(x, y) || find_bterm(x, y))
        return (false);

    CDp_bsnode *pb = (CDp_bsnode*)cursde->prpty(P_BNODE);
    unsigned int ix = 0;
    for ( ; pb; pb = pb->next()) {
        unsigned int nn = pb->index() + pb->width();
        if (nn > ix)
            ix = nn;
    }

    char tbuf[128];
    snprintf(tbuf, sizeof(tbuf), "%d %d %d %d %d", ix, 0, 0, x, y);
    cursde->prptyAdd(P_BNODE, tbuf);
    Changed = true;
    pb  = (CDp_bsnode*)cursde->prptyList();  // new one
    show_bterm(DISPLAY, pb);
    Opers = new sNop(N_BADD, 0, pb, Opers);
    edit_bterm(pb, false);
    ED()->PopUpCellProperties(MODE_UPD);
    return (true);
}


// Delete function for bus terminals.
//
bool
SubcState::delete_bterm(int x, int y)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (false);
    CDp_bsnode *pn = find_bterm(x, y);
    if (!pn)
        return (false);

    show_bterms(ERASE);
    if (cursde->isSymbolic()) {
        Plist *p0 = 0, *pe = 0;
        for (unsigned int ix = 0; ; ix++) {
            int px, py;
            if (!pn->get_pos(ix, &px, &py))
                break;
            if (!p0)
                p0 = pe = new Plist(px, py, 0);
            else {
                pe->next = new Plist(px, py, 0);
                pe = pe->next;
            }
        }
        if (p0 && !p0->next) {
            // If there is only one pin, skip delete.  The
            // node can only be deleted when not symbolic.
            Plist::destroy(p0);
            show_bterms(DISPLAY);
            return (false);
        }
        Opers = new sNop(N_BMOV, pn->index(), pn, Opers);
        pn->get_schem_pos(&Opers->loc.x, &Opers->loc.y);
        Opers->sypts = p0;

        unsigned int len = 0;
        for (Plist *p = p0; p; p = p->next) {
            if (p->x == x && p->y == y)
                continue;
            len++;
        }
        pn->alloc_pos(len);
        len = 0;
        for (Plist *p = p0; p; p = p->next) {
            if (p->x == x && p->y == y)
                continue;
            pn->set_pos(len, p->x, p->y);
            len++;
        }
        show_bterms(DISPLAY);
    }
    else {
        cursde->prptyUnlink(pn);
        if (EditBterm == pn) {
            EditBterm = 0;
            TermEditInfo tinfo((CDp_bsnode*)0);
            SCD()->PopUpTermEdit(0, MODE_UPD, &tinfo, te_cb, 0, 0, 0);
        }
        Opers = new sNop(N_BDEL, pn->index(), pn, Opers);
        show_bterms(DISPLAY);
    }
    Changed = true;
    ED()->PopUpCellProperties(MODE_UPD);
    return (true);
}


// Move a bus terminal.
//
bool
SubcState::move_bterm(int oldx, int oldy, int x, int y, bool cpy)
{
    if (oldx == x && oldy == y)
        return (false);
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (false);
    if (find_term(x, y) || find_bterm(x, y))
        return (false);

    // Only symbolic view terminals have multiple pins and can be copied.
    bool issym = cursde->isSymbolic();
    if (!issym && cpy)
        return (false);

    CDp_bsnode *pn = find_bterm(oldx, oldy);
    if (!pn)
        return (false);

    Plist *p0 = 0, *pe = 0;
    for (unsigned int ix = 0; ; ix++) {
        int px, py;
        if (!pn->get_pos(ix, &px, &py))
            break;
        // Don't allow coincident pins.
        if (issym && px == x && py == y) {
            Plist::destroy(p0);
            return (false);
        }
        if (!p0)
            p0 = pe = new Plist(px, py, 0);
        else {
            pe->next = new Plist(px, py, 0);
            pe = pe->next;
        }
    }
    Opers = new sNop(N_BMOV, pn->index(), pn, Opers);
    pn->get_schem_pos(&Opers->loc.x, &Opers->loc.y);
    Opers->sypts = p0;

    show_bterm(ERASE, pn);
    if (issym) {
        if (cpy) {
            unsigned int len = 0;
            for (Plist *p = p0; p; p = p->next)
                len++;
            pn->alloc_pos(len + 1);
            len = 0;
            for (Plist *p = p0; p; p = p->next) {
                pn->set_pos(len, p->x, p->y);
                len++;
            }
            pn->set_pos(len, x, y);
        }
        else {
            for (unsigned int ix = 0; ; ix++) {
                int xx, yy;
                if (!pn->get_pos(ix, &xx, &yy))
                    break;
                if (xx == oldx && yy == oldy) {
                    pn->set_pos(ix, x, y);
                    break;
                }
            }
        }
    }
    else {
        pn->set_schem_pos(x, y);
        if (!cursde->prpty(P_SYMBLC)) {
            // If no P_SYMBLC property, the sy fields should track x,y.
            pn->set_pos(0, x, y);
        }
    }
    show_bterm(DISPLAY, pn);
    Changed = true;
    ED()->PopUpCellProperties(MODE_UPD);
    return (true);
}


// Erase or display the bus terminals.
//
void
SubcState::show_bterms(bool display)
{
    CDs *cursde = CurCell(Electrical, true);
    if (!cursde)
        return;

    CDs *sd = cursde->symbolicRep(0);

    CDp_bsnode *pn = (CDp_bsnode*)cursde->prpty(P_BNODE);
    for ( ; pn; pn = pn->next()) {
        int x, y;
        if (sd) {
            for (unsigned int ix = 0; ; ix++) {
                if (!pn->get_pos(ix, &x, &y))
                    break;
                DSP()->ShowSyBscMark(display, x, y, HighlightingColor, 40,
                    pn, pn->index());
            }
        }
        pn->get_schem_pos(&x, &y);
        DSP()->ShowBscMark(display, x, y, HighlightingColor, 40,
            pn, pn->index());
    }
    DSP()->HliteElecBsc(display, EditBterm);
}


void
SubcState::show_bterm(bool display, CDp_bsnode *pn)
{
    CDs *cursde = CurCell(Electrical, true);
    if (!cursde)
        return;
    int x, y;
    if (cursde->symbolicRep(0)) {
        for (unsigned int ix = 0; ; ix++) {
            if (!pn->get_pos(ix, &x, &y))
                break;
            DSP()->ShowSyBscMark(display, x, y, HighlightingColor, 40,
                pn, pn->index());
        }
    }
    pn->get_schem_pos(&x, &y);
    DSP()->ShowBscMark(display, x, y, HighlightingColor, 40, pn, pn->index());
    if (EditBterm == pn)
        DSP()->HliteElecBsc(display, EditBterm);
}


// Prompt for and set the bus terminal index and width.  Return false
// if Esc is entered.
//
void
SubcState::edit_bterm(CDp_bsnode *pn, bool upd)
{
    if (!pn)
        return;
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;
    DSP()->HliteElecTerm(ERASE, EditNode, 0, 0);
    DSP()->HliteElecBsc(ERASE, EditBterm);
    EditNode = 0;
    EditBterm = 0;
    int x, y;
    Menu()->PointerRootLoc(&x, &y);
    TermEditInfo tinfo(pn);
    SCD()->PopUpTermEdit(0, MODE_ON, &tinfo, te_cb, pn, x + 50, y + 20);
    if (!upd) {
        EditVisible = true;
        EditBterm = pn;
        DSP()->HliteElecBsc(DISPLAY, pn);
    }
}

