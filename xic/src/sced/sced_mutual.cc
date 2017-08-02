
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
#include "undolist.h"
#include "cd_propnum.h"
#include "cd_lgen.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "events.h"
#include "layertab.h"
#include "menu.h"
#include "ebtn_menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"

// Code for handling mutual inductors

// The following two functions are general purpose utilities


// Set a name or value for mutual inductor.  True is returned if the
// mutual was updated.
//
bool
cSced::setMutParam(const char *name, int type, const char *string)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (false);
    while (isspace(*string))
        string++;
    if (type == P_VALUE) {
        // don't allow bogus numeric value
        double val;
        if (sscanf(string, "%lf", &val) == 1 && (val < -1 || val > 1))
            return (false);
    }
    else if (type != P_NAME)
        return (false);

    CDp_nmut *pm = (CDp_nmut*)cursde->prpty(P_NEWMUT);
    for ( ; pm; pm = pm->next()) {
        const char *dname;
        char buf[128];
        if (pm->assigned_name())
            dname = pm->assigned_name();
        else {
            sprintf(buf, "%s%d", MUT_CODE, pm->index());
            dname = buf;
        }
        if (strcmp(name, dname))
            continue;

        Ulist()->ListCheckPush("setmut", cursde, false, true);
        if (type == P_VALUE)
            pm->set_coeff_str(string);
        else
            pm->set_assigned_name(string);
        Errs()->init_error();
        if (!setMutLabel(cursde, pm, pm)) {
            Errs()->add_error("SetMutLabel failed");
            Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
        }
        Ulist()->ListPop();
        return (true);
    }
    return (false);
}


// Get a mutual inductor property (value or name) string.
//
char *
cSced::getMutParam(const char *name, int type)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (0);
    while (isspace(*name))
        name++;
    if (type != P_VALUE && type != P_NAME)
        return (0);

    CDp_nmut *pm = (CDp_nmut*)cursde->prpty(P_NEWMUT);
    for ( ; pm; pm = pm->next()) {
        const char *dname;
        char buf[128];
        if (pm->assigned_name())
            dname = pm->assigned_name();
        else {
            sprintf(buf, "%s%d", MUT_CODE, pm->index());
            dname = buf;
        }
        if (strcmp(name, dname))
            continue;

        if (type == P_VALUE) {
            sprintf(buf, "%s", pm->coeff_str());
            return (lstring::copy(buf));
        }
        else
            // silly, but why not
            return (lstring::copy(dname));
    }
    return (0);
}


// Link in newp, obtaining positioning info from oldp if non-nil (which
// can be newp also).
//
bool
cSced::setMutLabel(CDs *sd, CDp_nmut *newp, CDp_nmut *oldp)
{
    if (!sd)
        return (false);

    bool copied;
    if (!oldp && newp->bound()) {
        // update text in existing label
        hyList *htxt = newp->label_text(&copied);
        BBox tBB;
        if (!updateLabelText(newp->bound(), sd, htxt, &tBB))
            hyList::destroy(htxt);
        else
            DSP()->RedisplayArea(&tBB, Electrical);
        return (true);
    }

    Label label;
    label.label = newp->label_text(&copied);
    DSP()->DefaultLabelSize(label.label, Electrical, &label.width,
        &label.height);
    CDla *olabel;
    if (oldp)
        olabel = oldp->bound();
    else
        olabel = 0;

    BBox tBB(CDnullBB);
    if (olabel) {
        label.x = olabel->oBB().left;
        label.y = olabel->oBB().bottom;
        label.width = olabel->width();
        label.height = olabel->height();
        label.xform = olabel->xform();
        DSP()->LabelResize(label.label, olabel->label(), &label.width,
            &label.height);
        tBB.add(&olabel->oBB());
    }
    else {
        if (!newp->l1_dev() || !newp->l2_dev()) {
            Errs()->add_error("missing object pointer");
            return (false);
        }
        label.x = (newp->l1_dev()->oBB().right + newp->l1_dev()->oBB().left +
            newp->l2_dev()->oBB().right + newp->l2_dev()->oBB().left)/4;
        label.y = (newp->l1_dev()->oBB().top + newp->l1_dev()->oBB().bottom +
            newp->l2_dev()->oBB().top + newp->l2_dev()->oBB().bottom)/4;
        label.x -= label.width/2;
        label.y -= label.height/2;
    }
    CDla *nlabel = sd->newLabel(olabel, &label,
        olabel ? olabel->ldesc() : defaultLayer(newp), 0, true);
    if (copied)
        hyList::destroy(label.label);
    if (!nlabel) {
        Errs()->add_error("newLabel failed");
        return (false);;
    }
    newp->bind(nlabel);
    bool ret = nlabel->link(sd, 0, 0);
    if (!ret)
        Errs()->add_error("link failed");
    tBB.add(&nlabel->oBB());
    DSP()->RedisplayArea(&tBB, Electrical);
    return (ret);
}
// End of utilities


namespace {
    struct sSel
    {
        sSel(CDc *p1, CDc *p2, CDp *pd) { pointer1 = p1; pointer2 = p2;
            pdesc = pd; next = prev = 0; }
        CDp *pdesc;
        CDc *pointer1;
        CDc *pointer2;
        sSel *next;
        sSel *prev;
    };

    CDc *select_inductor(int, int, CDs*);
    CDla *select_mlabel(int, int, CDs*);

    namespace sced_mutual {
        struct MutState : public CmdState
        {
            MutState(const char*, const char*);
            virtual ~MutState();

            void setCaller(GRobject c)  { Caller = c; }

            void mut_init();
            void b1down();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();
            void show_mut(int);

            void message() { if (Level == 1) PL()->ShowPrompt(msg1);
                else if (Level == 2) PL()->ShowPrompt(msg2);
                else PL()->ShowPrompt(msg3); }

        private:
            void delete_mutual();
            void change_mutual();
            void SetLevel1(bool show)
                { Level = 1; if (show) message(); }
            void SetLevel2(bool show) { Level = 2; if (show) message(); }
            void SetLevel3() { Level = 3; message(); }

            GRobject Caller;
            sSel *Shead;
            sSel *Scur;
            CDc *Odesc;
            bool Uflag;

            static const char *msg1;
            static const char *msg2;
            static const char *msg3;
        };

        MutState *MutCmd;
    }
}

using namespace sced_mutual;

const char *MutState::msg1 =
"Use a, d, k to add, delete, or change mutual, arrow keys for next pair.";
const char *MutState::msg2 = "Click on first coupled inductor.";
const char *MutState::msg3 = "Click on second coupled inductor.";


void
cSced::showMutualExec(CmdDesc *cmd)
{
    if (MutCmd) {
        MutCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (!XM()->CheckCurCell(false, false, Electrical))
        return;
    if (DSP()->CurMode() == Electrical &&
            Menu()->MenuButtonStatus("main", MenuSYMBL) == 1) {
        PL()->ShowPrompt("Can't show mutual inductors in symbolic mode.");
        return;
    }
    MutCmd = new MutState("MUTUAL", "dev:mut");
    MutCmd->setCaller(cmd ? cmd->caller : 0);
    if (!EV()->PushCallback(MutCmd)) {
        delete MutCmd;
        return;
    }
    MutCmd->message();
    MutCmd->show_mut(DISPLAY);
    ds.clear();
}


MutState::MutState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    Shead = Scur = 0;
    Odesc = 0;
    Uflag = false;
    mut_init();
    Scur = Shead;
    if (!Shead)
        SetLevel2(false);
    else
        SetLevel1(false);
}


MutState::~MutState()
{
    MutCmd = 0;
}


void
MutState::mut_init()
{

    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;
    for (sSel *s = Shead; s; s = Shead) {
        Shead = s->next;
        delete [] s;
    }
    sSel *sd = 0, *sd0 = 0;
    for (CDp *pdesc = cursde->prptyList(); pdesc; pdesc = pdesc->next_prp()) {
        CDc *odesc1, *odesc2;
        if (pdesc->value() == P_MUT) {
            // Referenced by lower left corner
            int l1x, l2x, l1y, l2y;
            PMUT(pdesc)->get_coords(&l1x, &l1y, &l2x, &l2y);
            odesc1 = CDp_mut::find(l1x, l1y, cursde);
            odesc2 = CDp_mut::find(l2x, l2y, cursde);
            if (odesc1 == 0 || odesc2 == 0)
                continue;
        }
        else if (pdesc->value() == P_NEWMUT) {
            if (!PNMU(pdesc)->get_descs(&odesc1, &odesc2))
                continue;
        }
        else
            continue;

        if (!sd)
            sd = sd0 = new sSel(odesc1, odesc2, pdesc);
        else {
            sd->next = new sSel(odesc1, odesc2, pdesc);
            sd->next->prev = sd;
            sd = sd->next;
        }
    }
    Shead = sd0;
}


void
MutState::b1down()
{
    const char *umsg = "Existing mutual inductor updated";
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;
    if (Level == 1) {
        Uflag = false;
        Odesc = 0;
        // change current pair to one being pointed to
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        CDc *cdesc = select_inductor(x, y, cursde);
        if (cdesc) {
            sSel *s;
            for (s = Shead; s; s = s->next)
                if (s->pointer1 == cdesc || s->pointer2 == cdesc)
                    break;
            if (s && s != Scur) {
                // set current pair
                Scur = s;
                show_mut(ERASE);
                show_mut(DISPLAY);
            }
            else if (!s) {
                // start add
                Odesc = cdesc;
                show_mut(ERASE);
                DSP()->ShowCurrentObject(DISPLAY, Odesc);
                SetLevel3();
            }
            return;
        }
        // pointing at mutual label?
        CDla *la = select_mlabel(x, y, cursde);
        if (la) {
            sSel *s;
            for (s = Shead; s; s = s->next)
                if (s->pdesc->value() == P_NEWMUT &&
                        ((CDp_nmut*)s->pdesc)->bound() == la)
                    break;
            if (s) {
                if (s != Scur) {
                    // set current pair
                    Scur = s;
                    show_mut(ERASE);
                    show_mut(DISPLAY);
                }
                else {
                    // clicking on current pair's label is equivalent
                    // to pressing 'k'
                    Selections.showSelected(cursde, la);
                    change_mutual();
                    Selections.showUnselected(cursde, la);
                }
            }
        }
    }
    else if (Level == 2) {
        int x, y;
        EV()->Cursor().get_raw(&x, &y);
        if ((Odesc = select_inductor(x, y, cursde)) != 0) {
            DSP()->ShowCurrentObject(DISPLAY, Odesc);
            SetLevel3();
        }
    }
    else if (Level == 3) {
        CDc *odesc1 = Odesc, *odesc2;
        int x, y;
        EV()->Cursor().get_raw(&x, &y);
        if ((odesc2 = select_inductor(x, y, cursde)) != 0 && odesc2 != odesc1)
            DSP()->ShowCurrentObject(DISPLAY, odesc2);
        else
            return;

        char coefstr[128], devn[128];
        double kv;
        do {
            char *s = PL()->EditPrompt(
                "Enter SPICE coupling factor: k = ", 0);
            if (!s) {
                if (MutCmd) {
                    DSP()->ShowCurrentObject(ERASE, 0);
                    show_mut(DISPLAY);
                    SetLevel1(true);
                }
                else
                    PL()->ErasePrompt();
                return;
            }
            SCD()->mutParseName(s, devn, coefstr);
        }
        // don't allow an out of range numeric value
        while (sscanf(coefstr, "%le", &kv) == 1 && (kv < -1 || kv > 1));

        Ulist()->ListCheck("mutadd", cursde, false);

        // see if this mutual already exists
        CDp *pdesc = cursde->prptyList();
        for (; pdesc; pdesc = pdesc->next_prp()) {
            if (pdesc->value() == P_MUT) {
                CDp_mut *pm = (CDp_mut*)pdesc;
                int l1x, l1y, l2x, l2y;
                pm->get_coords(&l1x, &l1y, &l2x, &l2y);
                odesc1 = CDp_mut::find(l1x, l1y, cursde);
                odesc2 = CDp_mut::find(l2x, l2y, cursde);
                if (odesc1 && odesc2) {
                    pm->set_coeff(kv);
                    DSPmainWbag(PopUpMessage(umsg, false))
                    Ulist()->CommitChanges();
                    SetLevel1(true);
                    show_mut(DISPLAY);
                    return;
                }
            }
            else if (pdesc->value() == P_NEWMUT) {
                CDp_nmut *pm = (CDp_nmut*)pdesc;
                if ((odesc1 == pm->l1_dev() && odesc2 == pm->l2_dev()) ||
                        (odesc2 == pm->l1_dev() && odesc1 == pm->l2_dev())) {
                    pm->set_coeff_str(coefstr);
                    Errs()->init_error();
                    if (*devn && !pm->rename(cursde, devn))
                        Log()->ErrorLog(mh::EditOperation,
                            Errs()->get_error());
                    if (!SCD()->setMutLabel(cursde, pm, pm)) {
                        Errs()->add_error("SetMutLabel failed");
                        Log()->ErrorLog(mh::EditOperation,
                            Errs()->get_error());
                    }
                    DSPmainWbag(PopUpMessage(umsg, false))
                    Ulist()->CommitChanges();
                    SetLevel1(true);
                    show_mut(DISPLAY);
                    return;
                }
            }
        }
        // add a new one
        Errs()->init_error();
        if (!cursde->prptyMutualAdd(odesc1, odesc2, coefstr, devn)) {
            Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
            show_mut(DISPLAY);
            return;
        }
        sSel *sd = new sSel(odesc1, odesc2, cursde->prptyList());
        if (Scur) {
            sd->prev = Scur;
            sd->next = Scur->next;
            if (sd->next)
                sd->next->prev = sd;
            Scur->next = sd;
            Scur = sd;
        }
        else {
            sd->prev = 0;
            sd->next = 0;
            Shead = Scur = sd;
        }
        Ulist()->CommitChanges();
        SetLevel1(true);
    }
}


void
MutState::esc()
{
    DSP()->ShowCurrentObject(ERASE, 0);
    PL()->ErasePrompt();
    for (sSel *s = Shead; s; s = Shead) {
        Shead = s->next;
        delete [] s;
    }
    EV()->PopCallback(this);
    if (Caller)
        Menu()->Deselect(Caller);
    SCD()->DevsEscCallback();
    delete this;
}


bool
MutState::key(int code, const char *text, int mstate)
{
    if (Level != 1)
        return (false);
    if (code == LEFT_KEY || code == DOWN_KEY) {
        if (mstate & (GR_SHIFT_MASK | GR_CONTROL_MASK))
            return (false);
        if (Scur) {
            if (Scur->prev) {
                Scur = Scur->prev;
                show_mut(ERASE);
                show_mut(DISPLAY);
            }
            else if (Scur->next) {
                while (Scur->next)
                    Scur = Scur->next;
                show_mut(ERASE);
                show_mut(DISPLAY);
            }
        }
        return (true);
    }
    if (code == RIGHT_KEY || code == UP_KEY) {
        if (mstate & (GR_SHIFT_MASK | GR_CONTROL_MASK))
            return (false);
        if (Scur) {
            if (Scur->next) {
                Scur = Scur->next;
                show_mut(ERASE);
                show_mut(DISPLAY);
            }
            else if (Scur->prev) {
                while (Scur->prev)
                    Scur = Scur->prev;
                show_mut(ERASE);
                show_mut(DISPLAY);
            }
        }
        return (true);
    }
    if (code == DELETE_KEY) {
        delete_mutual();
        Uflag = false;
        return (true);
    }
    if (text) {
        switch (*text) {
            case 'a':
            case 'A':
                show_mut(ERASE);
                SetLevel2(true);
                Uflag = false;
                return (true);
            case 'd':
            case 'D':
                delete_mutual();
                Uflag = false;
                return (true);
            case 'k':
            case 'K':
                change_mutual();
                Uflag = false;
                return (true);
        }
    }
    return (false);
}


void
MutState::undo()
{
    if (Level == 1) {
        if (Ulist()->HasUndo()) {
            show_mut(ERASE);
            CDp *oldp = (Scur ? Scur->pdesc : 0);
            Ulist()->UndoOperation();
            mut_init();
            Scur = 0;
            if (oldp) {
                for (sSel *s = Shead; s; s = s->next) {
                    if (s->pdesc == oldp) {
                        Scur = s;
                        break;
                    }
                }
            }
            if (!Scur)
                Scur = Shead;
            show_mut(DISPLAY);
        }
        else
            PL()->ShowPrompt("Nothing else to undo.");
    }
    else if (Level == 2) {
        SetLevel1(true);
        show_mut(DISPLAY);
        Uflag = true;
    }
    else if (Level == 3) {
        DSP()->ShowCurrentObject(ERASE, 0);
        SetLevel2(true);
    }
}


void
MutState::redo()
{
    if (Level == 2) {
        if (Odesc && !Ulist()->HasRedo()) {
            DSP()->ShowCurrentObject(ERASE, 0);
            DSP()->ShowCurrentObject(DISPLAY, Odesc);
            SetLevel3();
            return;
        }
    }
    if (Uflag) {
        show_mut(ERASE);
        SetLevel2(true);
        Uflag = false;
        return;
    }
    if (Ulist()->HasRedo()) {
        show_mut(ERASE);
        CDp *oldp = (Scur ? Scur->pdesc : 0);
        Ulist()->RedoOperation();
        mut_init();
        Scur = 0;
        if (oldp) {
            for (sSel *s = Shead; s; s = s->next) {
                if (s->pdesc == oldp) {
                    Scur = s;
                    break;
                }
            }
        }
        if (!Scur)
            Scur = Shead;
        show_mut(DISPLAY);
    }
    else
        PL()->ShowPrompt("Nothing else to redo.");
}


void
MutState::show_mut(int draw)
{
    if (!Scur)
        return;
    if (draw == DISPLAY) {
        DSP()->ShowCurrentObject(DISPLAY, Scur->pointer1);
        DSP()->ShowCurrentObject(DISPLAY, Scur->pointer2);
    }
    else
        DSP()->ShowCurrentObject(ERASE, 0);
}


void
MutState::delete_mutual()
{
    if (!Scur) {
        Log()->PopUpErr("No mutual inductor to delete.");
        return;
    }
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;
    Ulist()->ListCheck("mutdel", cursde, false);
    CDp *pdesc = Scur->pdesc;
    if (pdesc->value() == P_NEWMUT) {
        CDc *odesc1, *odesc2;
        if (!PNMU(pdesc)->get_descs(&odesc1, &odesc2))
            return;
        CDp_mutlrf *pml = (CDp_mutlrf*)odesc1->prpty(P_MUTLRF);
        Ulist()->RecordPrptyChange(cursde, odesc1, pml, 0);
        pml = (CDp_mutlrf*)odesc2->prpty(P_MUTLRF);
        Ulist()->RecordPrptyChange(cursde, odesc2, pml, 0);
        CDla *olabel = pdesc->bound();
        if (olabel) {
            Ulist()->RecordObjectChange(cursde, olabel, 0);
            DSP()->RedisplayArea(&olabel->oBB());
        }
    }
    cursde->prptyUnlink(pdesc);
    delete pdesc;
    DSPmainWbag(PopUpMessage("Mutual inductance deleted.", false))
    show_mut(ERASE);
    sSel *sd = Scur;
    if (sd->prev)
        sd->prev->next = sd->next;
    if (sd->next) {
        sd->next->prev = sd->prev;
        Scur = sd->next;
    }
    else
        Scur = sd->prev;
    if (Shead == sd)
        Shead = sd->next;
    delete sd;
    show_mut(DISPLAY);
    Ulist()->CommitChanges();
}


void
MutState::change_mutual()
{
    if (!Scur)
        return;
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;
    CDp *pdesc = Scur->pdesc;
    double kv;
    char coefstr[128], devn[128];
    if (pdesc->value() == P_NEWMUT) {
        CDp_nmut *pm = PNMU(pdesc);
        do {
            bool copied;
            hyList *lt = pm->label_text(&copied);
            char *s = hyList::string(lt, HYcvPlain, false);
            if (copied)
                hyList::destroy(lt);
            strcpy(devn, s);
            delete [] s;
            s = PL()->EditPrompt("Enter new label: ", devn);
            if (!s) {
                if (MutCmd)
                    message();
                else
                    PL()->ErasePrompt();
                return;
            }
            SCD()->mutParseName(s, devn, coefstr);
        }
        while (sscanf(coefstr, "%le", &kv) == 1 && (kv < -1 || kv > 1));
        Ulist()->ListCheck("mutchg", cursde, false);
        pm->set_coeff_str(coefstr);
        Errs()->init_error();
        if (!pm->rename(cursde, devn))
            Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
        if (!SCD()->setMutLabel(cursde, pm, pm)) {
            Errs()->add_error("SetMutLabel failed");
            Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
        }
    }
    else {
        sprintf(coefstr, "%f", PMUT(pdesc)->coeff());
        char *s;
        do {
            s = PL()->EditPrompt("Enter SPICE coupling factor: k = ", coefstr);
            if (!s) {
                if (MutCmd)
                    message();
                else
                    PL()->ErasePrompt();
                return;
            }
        }
        while (sscanf(s, "%le", &kv) < 1 || kv < -1 || kv > 1);
        Ulist()->ListCheck("mutchg", cursde, false);
        PMUT(pdesc)->set_coeff(kv);
    }
    Ulist()->CommitChanges();
    message();
}
// End of MutState methods


// Change all of the old format mutual inductors in sdesc to the new
// format, and add labels.
//
void
cSced::mutToNewMut(CDs *sdesc)
{
    char buf[64];
    Errs()->init_error();
    CDp *pd, *pnext;
    for (pd = sdesc->prptyList(); pd; pd = pnext) {
        pnext = pd->next_prp();
        if (pd->value() == P_MUT) {
            // Referenced by lower left corner
            CDc *odesc1, *odesc2;
            int l1x, l1y, l2x, l2y;
            PMUT(pd)->get_coords(&l1x, &l1y, &l2x, &l2y);
            odesc1 = CDp_mut::find(l1x, l1y, sdesc);
            odesc2 = CDp_mut::find(l2x, l2y, sdesc);
            if (odesc1 == 0 || odesc2 == 0) {
                sdesc->prptyUnlink(pd);
                delete pd;
                continue;
            }
            sprintf(buf, "%g", PMUT(pd)->coeff());
            if (!sdesc->prptyMutualAdd(odesc1, odesc2, buf, 0))
                Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
            sdesc->prptyUnlink(pd);
            delete pd;
        }
    }
}


// The mutual inductor label has the form "devname=coefstr" or just
// "value", where devname, if given and not the default name, becomes
// an alias for the mutual inductor.  This function splits a supplied
// name string into the devname and coefstr buffers.
//
void
cSced::mutParseName(const char *name, char *devname, char *coefstr)
{
    const char *s = name;
    char *t = devname;
    *devname = '\0';
    *coefstr = '\0';
    while (isspace(*s))
        s++;
    while (*s && !isspace(*s) && *s != '=')
        *t++ = *s++;
    *t = '\0';
    while (isspace(*s))
        s++;
    if (*s == '=') {
        s++;
        while (isspace(*s))
            s++;
        t = coefstr;
        while (*s && !isspace(*s))
           *t++ = *s++;
        *t = '\0';
    }
    else {
        strcpy(coefstr, devname);
        *devname = '\0';
    }
}

namespace {
    // Return an "inductor" odesc at x,y.  An "inductor" is a device
    // with name key starting with 'L' or 'l' and has a branch
    // property.
    //
    CDc *
    select_inductor(int x, int y, CDs *sdesc)
    {
        BBox BB;
        BB.left = BB.right = x;
        BB.bottom = BB.top = y;
        bool nameok = false;
        bool branchok = false;
        CDg gdesc;
        gdesc.init_gen(sdesc, CellLayer(), &BB);
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {
            CDp *pd;
            for (pd = cdesc->prpty_list(); pd; pd = pd->next_prp()) {
                if (pd->value() == P_NAME) {
                    if (((CDp_name*)pd)->key() == 'l')
                        nameok = true;
                }
                else if (pd->value() == P_BRANCH)
                    branchok = true;
                if (nameok && branchok)
                    return (cdesc);
            }
        }
        return (0);
    }


    CDla *
    select_mlabel(int x, int y, CDs *sdesc)
    {
        BBox BB;
        BB.left = BB.right = x;
        BB.bottom = BB.top = y;
        CDg gdesc;
        CDl *ld;
        CDsLgen lgen(sdesc);
        while ((ld = lgen.next()) != 0) {
            gdesc.init_gen(sdesc, ld, &BB);
            CDla *odesc;
            while ((odesc = (CDla*)gdesc.next()) != 0) {
                if (odesc->type() != CDLABEL)
                    continue;
                CDp_lref *prf = (CDp_lref*)odesc->prpty(P_LABRF);
                if (prf && prf->cellref() == sdesc)
                    return (odesc);
            }
        }
        return (0);
    }
}

