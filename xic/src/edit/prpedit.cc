
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
#include "pcell.h"
#include "pcell_params.h"
#include "scedif.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "cd_lgen.h"
#include "fio.h"
#include "events.h"
#include "menu.h"
#include "ebtn_menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"


//----------------------------------------------------------------------
// The prpty command - modify properties of objects using
// PopUpProperties()
//----------------------------------------------------------------------

namespace {
    // Add/replace the property specified, do undo list processing.
    // Used for physical mode properties only.
    //
    bool
    prpty_replace(CDo *odesc, CDs *sdesc, CDp *pdesc, int value,
        const char *string)
    {
        if (!odesc || !string)
            return (true);
        if (value == XICP_NOMERGE) {
            // This property is restricted to boxes, polys, and wires.
            if (odesc->type() != CDBOX && odesc->type() != CDPOLYGON &&
                    odesc->type() != CDWIRE) {
                Log()->ErrorLog(mh::Properties,
                    "NoMerge applicable to boxes, polygons, and wires only.");
                return (false);
            }
        }
        else if (value == XICP_EXT_FLATTEN) {
            // Instances only.
            if (odesc->type() != CDINSTANCE) {
                Log()->ErrorLog(mh::Properties,
                    "Flatten applicable to instances only.");
                return (false);
            }
        }
        CDp *newp = new CDp(string, value);
        Ulist()->RecordPrptyChange(sdesc, odesc, pdesc, newp);
        return (true);
    }


    // This is passed to the "long text" editor and to its callback.
    //
    struct ltobj
    {
        ltobj(CDs *s, CDo *o, CDp *p, int v)
            { sdesc = s; odesc = o; pdesc = p ? p->dup() : 0; value = v; }
        ~ltobj() { delete pdesc; }

        CDs *sdesc;
        CDo *odesc;
        CDp *pdesc;
        int value;
    };

    // Doubly linked list of objects to view/process
    //
    struct sSel
    {
        sSel(CDo *p) { pointer = p; next = prev = 0; }

        CDo *pointer;
        sSel *next;
        sSel *prev;
    };

    inline CDo *od_of(sSel *sl) { return (sl ? sl->pointer : 0); }

    CDp *prpmatch(CDo*, CDp*, int);
    int maskof(int);

// for Vmask
#define NAME_MASK       0x1
#define MODEL_MASK      0x2
#define VALUE_MASK      0x4
#define PARAM_MASK      0x8
#define OTHER_MASK      0x10
#define NOPHYS_MASK     0x20
#define FLATTEN_MASK    0x40
#define NOSYMB_MASK     0x80
#define RANGE_MASK      0x100
#define DEVREF_MASK     0x200

    namespace ed_prpedit {
        enum { PRPquiescent, PRPadding, PRPdeleting };
        typedef unsigned char PRPmodif;

        struct PrptyState : public CmdState
        {
            PrptyState(const char*, const char*);
            virtual ~PrptyState();

            void setCaller(GRobject c)  { Caller = c; }

            // Control interface, wrapped into cEdit.
            bool pSetup();
            bool pCallback(CDo*);
            void pRelist();
            void pSetGlobal(bool);
            void pSetInfoMode(bool b) { InfoMode = b; }
            void pUpdateList(CDo*, CDo*);
            void pAdd(int);
            void pEdit(PrptyText*);
            void pRemove(PrptyText*);

            void b1down() { cEventHdlr::sel_b1down(); }
            void b1up();
            void desel();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();

            static void ltcallback(hyList*, void*);

        private:
            void prp_showselect(bool);
            bool prp_get_prompt(bool, CDo*, char*, int);
            void prp_updtext(sSel*);
            void prp_add_elec(PrptyText*, int, bool);
            void prp_add_elec_noglob();
            void prp_add_elec_glob_all();
            void prp_add_elec_glob_seq();
            void prp_add_phys(PrptyText*, int, bool);
            void prp_add_phys_noglob();
            void prp_add_phys_glob_all();
            void prp_add_phys_glob_seq();
            bool prp_get_add_type(bool);
            bool prp_get_string(bool, bool);
            void prp_rm(PrptyText*);
            bool prp_get_rm_type(bool);
            void prp_remove(sSel*, CDp*);

            static int btn_callback(PrptyText*);
            static void down_callback();
            static void up_callback();

            void message() { PL()->ShowPrompt(msg1); }
            void SetLevel1(bool show) { Level = 1; if (show) message(); }
            void SetLevel2() { Level = 2; }

            GRobject Caller;
            bool GotOne;
            PRPmodif Modifying;   // set when in adprp or rmprp
            bool Global;          // global mode add/remove
            bool InfoMode;        // use info window pop-up
            bool ShowAllSel;      // show all objects in list as selected
            sSel *Shead;
            sSel *Scur;
            char Types[8];
            BBox AOI;
            int Value;            // current property value for adprp, rmprp
            int Vmask;            // type vector for rmprp
            CDp *SelPrp;          // selected property
            struct {
                char *string;
                hyList *hyl;
            } Udata;              // property string for adprp, rmprp

            static const char *msg1;
            static const char *nosel;
        };

        PrptyState *PrptyCmd;
    }
}

using namespace ed_prpedit;

const char *PrptyState::msg1 = "Select objects";
const char *PrptyState::nosel = "No objects have been selected.";

namespace {
    // While the prpty command is active, this code is called from the
    // desel button.  It deselects all selected objects.
    //
    void
    prptyDesel(CmdDesc*)
    {
        // Nothing to do, the work is done in PrptyCmd::desel.
    }


    // Reset the desel button when active to property specific action.
    //
    void
    switchPropertiesMenu(bool state)
    {
        if (state)
            XM()->RegisterDeselOverride(&prptyDesel);
        else
            XM()->RegisterDeselOverride(0);
    }
}


// Menu function for prpty command.  If objects have been selected,
// link them into a new sSel list, and set Level2, where each object
// is highlighted and properties displayed.  The arrow keys cycle
// through this list.  Otherwise set Level1, which enables selection
// of objects.
//
void
cEdit::propertiesExec(CmdDesc *cmd)
{
    if (PrptyCmd) {
        PrptyCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurCell(true, false, DSP()->CurMode()))
        return;
    if (DSP()->CurMode() == Electrical &&
            MainMenu()->MenuButtonStatus(MMmain, MenuSYMBL) == 1) {
        PL()->ShowPrompt("Can't show properties in symbolic mode.");
        return;
    }
    switchPropertiesMenu(true);
    ED()->PopUpProperties(0, MODE_ON, PRPinactive);
    PrptyCmd = new PrptyState("PRPTY", "xic:prpty");
    PrptyCmd->setCaller(cmd ? cmd->caller : 0);
    if (!PrptyCmd->pSetup()) {
        delete PrptyCmd;
        return;
    }
    ds.clear();
}


// For graphics.
//
CmdState*
cEdit::prptyCmd()
{
    return (PrptyCmd);
}


bool
cEdit::prptyCallback(CDo *odesc)
{
    if (PrptyCmd)
        return (PrptyCmd->pCallback(odesc));
    return (false);
}


// Update the properties lists.
//
void
cEdit::prptyRelist()
{
    if (PrptyCmd)
        PrptyCmd->pRelist();
    PopUpCellProperties(MODE_UPD);
}


void
cEdit::prptySetGlobal(bool glob)
{
    if (PrptyCmd)
        PrptyCmd->pSetGlobal(glob);
}


// Set/unset info mode (from the pop-up).
//
void
cEdit::prptySetInfoMode(bool b)
{
    if (PrptyCmd)
        PrptyCmd->pSetInfoMode(b);
}


void
cEdit::prptyUpdateList(CDo *odesc, CDo *onew)
{
    if (PrptyCmd)
        PrptyCmd->pUpdateList(odesc, onew);
}


void
cEdit::prptyAdd(int which)
{
    if (PrptyCmd)
        PrptyCmd->pAdd(which);
}


void
cEdit::prptyEdit(PrptyText *line)
{
    if (PrptyCmd)
        PrptyCmd->pEdit(line);
}


void
cEdit::prptyRemove(PrptyText *line)
{
    if (PrptyCmd)
        PrptyCmd->pRemove(line);
}
// End of cEdit functions for PrptyState.


PrptyState::PrptyState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    if (DSP()->CurMode() == Electrical) {
        ScedIf()->connectAll(false);
        Types[0] = CDINSTANCE;
        Types[1] = CDWIRE;
        Types[2] = CDLABEL;
        Types[3] = '\0';
    }
    else
        strcpy(Types, Selections.selectTypes());
    GotOne = Selections.hasTypes(CurCell(), Types, false);
    if (!GotOne)
        SetLevel1(false);
    else
        SetLevel2();
    Modifying = PRPquiescent;
    Global = false;
    InfoMode = false;
    ShowAllSel = false;
    Shead = Scur = 0;
    Value = Vmask = 0;
    SelPrp = 0;
    Udata.string = 0;
    Udata.hyl = 0;
}


PrptyState::~PrptyState()
{
    PrptyCmd = 0;
}


bool
PrptyState::pSetup()
{
    if (!EV()->PushCallback(this)) {
        switchPropertiesMenu(false);
        return (false);
    }

    sSel *sd = 0, *sd0 = 0;
    sSelGen sg(Selections, CurCell());
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (!strchr(Types, od->type()))
            continue;
        if (!sd)
            sd = sd0 = new sSel(od);
        else {
            sd->next = new sSel(od);
            sd->next->prev = sd;
            sd = sd->next;
        }
    }
    if (!ShowAllSel)
        Selections.deselectTypes(CurCell(), 0);
    Shead = Scur = sd0;
    ED()->PopUpProperties(od_of(Scur), MODE_UPD, PRPactive);
    if (!Scur)
        message();
    return (true);
}


// This function is called by the editor if the user points at an
// object on screen.  This pops up a secondary window listing the
// properties of the "alternate" object, allowing cut/paste.  A
// true return suppresses hypertext return in editor.
//
bool
PrptyState::pCallback(CDo *odesc)
{
    if (InfoMode || (EV()->Cursor().get_downstate() &
            (GR_SHIFT_MASK | GR_CONTROL_MASK))) {
        if (odesc && XM()->IsBoundaryVisible(CurCell(), odesc))
            ED()->PopUpPropertyInfo(odesc, MODE_ON);
        return (true);
    }
    if (Modifying == PRPdeleting)
        return (true);
    return (false);
}


void
PrptyState::pRelist()
{
    CDo *od = Scur ? Scur->pointer : 0;
    ED()->PopUpProperties(od, MODE_UPD, PRPnochange);
    ED()->PopUpPropertyInfo(0, MODE_UPD);
}


// Set/unset global mode (from the pop-up).
//
void
PrptyState::pSetGlobal(bool glob)
{
    Global = glob;
    if (!ShowAllSel) {
        CDs *cursd = CurCell();
        if (glob) {
            for (sSel *s = Shead; s; s = s->next) {
                // Skip display, then revert to vanilla.
                Selections.insertObject(cursd, s->pointer, true);
                s->pointer->set_state(CDobjVanilla);
            }
        }
        else
            Selections.deselectTypes(cursd, 0);
        prp_showselect(true);
    }
}


// If an object is being deleted, remove it from the list or replace
// it with onew.  This is needed if a device label is updated, and the
// label is in the list, and for objects changing due to applied pseudo-
// properties.
//
void
PrptyState::pUpdateList(CDo *odesc, CDo *onew)
{
    for (sSel *sd = Shead; sd; sd = sd->next) {
        if (sd->pointer == odesc) {
            if (onew)
                sd->pointer = onew;
            else {
                if (sd == Scur) {
                    if (sd->next)
                        Scur = sd->next;
                    else if (sd->prev)
                        Scur = sd->prev;
                    else
                        Scur = 0;
                }
                if (sd->prev)
                    sd->prev->next = sd->next;
                else
                    Shead = sd->next;
                if (sd->next)
                    sd->next->prev = sd->prev;
                delete sd;
            }
            break;
        }
    }
    if (Global) {
        sSelGen sg(Selections, CurCell());
        CDo *od;
        while ((od = sg.next()) != 0) {
            if (od == odesc) {
                if (onew) {
                    // Keep state, replace sets to selected.
                    CDobjState st = onew->state();
                    sg.replace(onew);
                    onew->set_state(st);
                }
                else
                    sg.remove();
                break;
            }
        }
    }
    ED()->PropertyPurge(odesc, Scur ? Scur->pointer : 0);
    ED()->PropertyInfoPurge(odesc, onew);
}


void
PrptyState::pAdd(int which)
{
    PL()->RegisterArrowKeyCallbacks(0, 0);
    ED()->RegisterPrptyBtnCallback(0);
    if (DSP()->CurMode() == Physical)
        prp_add_phys(0, which, false);
    else
        prp_add_elec(0, which, false);
    if (PrptyCmd)
        pRelist();
}


// Edit the property given in line.
//
void
PrptyState::pEdit(PrptyText *line)
{
    PL()->RegisterArrowKeyCallbacks(0, 0);
    ED()->RegisterPrptyBtnCallback(0);
    if (DSP()->CurMode() == Physical)
        prp_add_phys(line, -1, true);
    else
        prp_add_elec(line, -1, true);
    if (PrptyCmd)
        pRelist();
}


// Remove the property whose string appears in line, or if in global
// mode remove properties from all objects in the object list.
//
void
PrptyState::pRemove(PrptyText *line)
{
    PL()->RegisterArrowKeyCallbacks(0, 0);
    ED()->RegisterPrptyBtnCallback(0);
    prp_rm(line);
}


void
PrptyState::b1up()
{
    if (InfoMode || (EV()->Cursor().get_downstate() &
            (GR_SHIFT_MASK | GR_CONTROL_MASK))) {
        CDol *sl0;
        if (!cEventHdlr::sel_b1up(&AOI, Types, &sl0))
            return;
        if (!sl0)
            return;
        CDo *od = 0;
        for (CDol *sl = sl0; sl; sl = sl->next) {
            if (XM()->IsBoundaryVisible(CurCell(), sl->odesc)) {
                od = sl->odesc;
                break;
            }
        }
        CDol::destroy(sl0);
        if (od)
            ED()->PopUpPropertyInfo(od, MODE_ON);
        return;
    }

    sSel *sd = 0, *sd0 = 0;
    if (Level == 1) {

        if (!Global && !ShowAllSel) {
            CDol *sl0;
            if (!cEventHdlr::sel_b1up(&AOI, Types, &sl0))
                return;
            if (!sl0)
                return;
            for (CDol *sl = sl0; sl; sl = sl->next) {
                if (!sd)
                    sd = sd0 = new sSel(sl->odesc);
                else {
                    sd->next = new sSel(sl->odesc);
                    sd->next->prev = sd;
                    sd = sd->next;
                }
            }
            CDol::destroy(sl0);
        }
        else {
            if (!cEventHdlr::sel_b1up(&AOI, Types, 0))
                return;
            sSelGen sg(Selections, CurCell());
            CDo *od;
            while ((od = sg.next()) != 0) {
                if (!sd)
                    sd = sd0 = new sSel(od);
                else {
                    sd->next = new sSel(od);
                    sd->next->prev = sd;
                    sd = sd->next;
                }
            }
            if (!sd)
                return;
        }
        Shead = Scur = sd0;
        ED()->PopUpProperties(od_of(Scur), MODE_UPD, PRPnochange);
        SetLevel2();
    }
    else {
        sSel *newsel = 0, *n0 = 0;
        CDol *sl0;
        if (!cEventHdlr::sel_b1up(&AOI, Types, &sl0))
            return;
        if (!sl0)
            return;
        for (CDol *sl = sl0; sl; sl = sl->next) {
            // is object already in list?
            for (sd = Shead ; sd; sd = sd->next)
                if (sd->pointer == sl->odesc)
                    break;
            if (sd) {
                // Object is already in list
                if (!sl->next && !newsel) {
                    // This is the last new object, and no other new objects
                    // were found to not be in the existing list.
                    //
                    if (sd == Scur && (sd->prev || sd->next)) {
                        // This is also the currently marked object,
                        // deselect and remove from list.
                        //
                        Selections.removeObject(CurCell(), sd->pointer);
                        if (sd->prev)
                            sd->prev->next = sd->next;
                        else
                            Shead = sd->next;
                        if (sd->next) {
                            sd->next->prev = sd->prev;
                            Scur = sd->next;
                        }
                        else
                            Scur = sd->prev;
                        delete sd;
                    }
                    else
                        // Make the new object the currently marked object
                        Scur = sd;
                    CDol::destroy(sl0);
                    ED()->PopUpProperties(od_of(Scur), MODE_UPD, PRPnochange);
                    return;
                }
                continue;
            }
            else {
                // Object not in list.  Select it, and add it to the "new"
                // list.
                //
                if (Global || ShowAllSel) {
                    sl->odesc->set_state(CDobjVanilla);
                    Selections.insertObject(CurCell(), sl->odesc);
                }
                if (newsel == 0)
                    newsel = n0 = new sSel(sl->odesc);
                else {
                    newsel->next = new sSel(sl->odesc);
                    newsel->next->prev = newsel;
                    newsel = newsel->next;
                }
            }
        }
        //
        // If we get here, a new object was found.  Link the new list
        // into the present list at the present position.  Advance the
        // present position to the first new object.  It is possible that
        // Scur/Shead is null, if objects have been merged away through
        // pseudo-props
        //
        CDol::destroy(sl0);
        n0->prev = Scur;
        if (Scur) {
            newsel->next = Scur->next;
            Scur->next = n0;
        }
        if (newsel->next)
            newsel->next->prev = newsel;
        Scur = n0;
        if (!Shead)
            Shead = Scur;
        ED()->PopUpProperties(od_of(Scur), MODE_UPD, PRPnochange);
    }
}


// While the prpty command is active, this code is called from the
// desel button.  It deselects all selected objects.
//
void
PrptyState::desel()
{
    if (!Scur)
        return;
    ED()->PopUpProperties(0, MODE_UPD, PRPnochange);
    while ((Scur = Shead) != 0) {
        Shead = Scur->next;
        Selections.removeObject(CurCell(), Scur->pointer);
        delete Scur;
    }
    Scur = Shead = 0;
    if (EV()->CurCmd() && EV()->CurCmd() != this)
        EV()->CurCmd()->esc();
    SetLevel1(true);
}


void
PrptyState::esc()
{
    PL()->AbortLongText(); 

    cEventHdlr::sel_esc();
    Selections.deselectTypes(CurCell(), 0);
    while ((Scur = Shead)) {
        Shead = Scur->next;
        delete Scur;
    }
    Scur = Shead = 0;
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    MainMenu()->Deselect(Caller);
    switchPropertiesMenu(false);
    ED()->PopUpPropertyInfo(0, MODE_OFF);
    ED()->PopUpProperties(0, MODE_UPD, PRPinactive);
    delete this;
}


// If the user points at an object, it is selected, added to the sSel
// list if not already there, and becomes the currently marked object.
// If it is already the currently marked object, deselect it and
// remove it from the list.


// Look for arrow keys, which cycle the currently marked object
// through the list.  Also respond to accelerators for the button
// commands.
//
bool
PrptyState::key(int code, const char*, int mstate)
{
    if (Level == 1)
        return (false);
    if (code == LEFT_KEY || code == DOWN_KEY) {
        if (mstate & (GR_SHIFT_MASK | GR_CONTROL_MASK))
            return (false);
        if (!Scur)
            return (true);
        if (Scur->prev)
            Scur = Scur->prev;
        else {
            while (Scur->next)
                Scur = Scur->next;
        }
        ED()->PopUpProperties(od_of(Scur), MODE_UPD, PRPnochange);
        return (true);
    }
    if (code == RIGHT_KEY || code == UP_KEY) {
        if (mstate & (GR_SHIFT_MASK | GR_CONTROL_MASK))
            return (false);
        if (!Scur)
            return (true);
        if (Scur->next)
            Scur = Scur->next;
        else {
            while (Scur->prev)
                Scur = Scur->prev;
        }
        ED()->PopUpProperties(od_of(Scur), MODE_UPD, PRPnochange);
        return (true);
    }
    return (false);
}


// Undo the last adprp or rmprp, set currently marked object to
// undone object, if not global.
//
void
PrptyState::undo()
{
    if (Level == 1)
        cEventHdlr::sel_undo();
    else {
        Oper *cur = Ulist()->UndoList();
        if (cur && (!strcmp(cur->cmd(), "adprp") ||
                !strcmp(cur->cmd(), "rmprp"))) {
            if (cur->prp_list()) {
                CDo *odesc = cur->prp_list()->odesc();
                bool global = (cur->prp_list()->next_chg() != 0);
                if (!cEventHdlr::sel_undo() && !global) {
                    sSel *sd;
                    for (sd = Shead; sd; sd = sd->next)
                        if (sd->pointer == odesc)
                            break;
                    if (sd) {
                        Scur = sd;
                        ED()->PopUpProperties(od_of(Scur), MODE_UPD,
                            PRPnochange);
                    }
                }
            }
            else if (cur->obj_list()) {
                if (!cEventHdlr::sel_undo() && Ulist()->HasRedo()) {
                    Ochg *xc = Ulist()->RedoList()->obj_list();
                    while (xc) {
                        for (sSel *sd = Shead; sd; sd = sd->next) {
                            if (xc->oadd() && xc->odel() &&
                                    sd->pointer == xc->odel()) {
                                sd->pointer = xc->oadd();
                                break;
                            }
                        }
                        xc = xc->next_chg();
                    }
                    ED()->PopUpProperties(od_of(Scur), MODE_UPD, PRPnochange);
                }
            }
            else
                cEventHdlr::sel_undo();
        }
        else {
            // Limit undo to property setting operations
            if (cur && (!strcmp(cur->cmd(), "set") ||
                    !strcmp(cur->cmd(), "ddprp")))
                // "set" comes from !set device@param
                Ulist()->UndoOperation();
            else {
                Ulist()->RotateUndo(0);
                cEventHdlr::sel_undo();
                Ulist()->RotateUndo(cur);
            }
        }
    }
}


// Redo the last adprp or rmprp, set currently marked object to
// object redone, if not global.
//
void
PrptyState::redo()
{
    if (Level == 1)
        cEventHdlr::sel_redo();
    else {
        Oper *cur = Ulist()->RedoList();
        if (cur && (!strcmp(cur->cmd(), "adprp") ||
                !strcmp(cur->cmd(), "rmprp"))) {
            if (cur->prp_list()) {
                CDo *odesc = cur->prp_list()->odesc();
                bool global = (cur->prp_list()->next_chg() != 0);
                Ulist()->RedoOperation();
                if (!global) {
                    sSel *sd;
                    for (sd = Shead; sd; sd = sd->next)
                        if (sd->pointer == odesc)
                            break;
                    if (sd) {
                        Scur = sd;
                        ED()->PopUpProperties(od_of(Scur), MODE_UPD,
                            PRPnochange);
                    }
                }
            }
            else if (cur->obj_list()) {
                Ulist()->RedoOperation();
                if (Ulist()->HasUndo()) {
                    Ochg *xc = Ulist()->UndoList()->obj_list();
                    while (xc) {
                        for (sSel *sd = Shead; sd; sd = sd->next) {
                            if (xc->oadd() && xc->odel() &&
                                    sd->pointer == xc->odel()) {
                                sd->pointer = xc->oadd();
                                break;
                            }
                        }
                        xc = xc->next_chg();
                    }
                    ED()->PopUpProperties(od_of(Scur), MODE_UPD, PRPnochange);
                }
            }
            else
                Ulist()->RedoOperation();
        }
        else {
            // Limit redo to property setting operations
            if (cur && (!strcmp(cur->cmd(), "set") ||
                    !strcmp(cur->cmd(), "ddprp")))
                Ulist()->RedoOperation();
            else {
                Ulist()->RotateRedo(0);
                cEventHdlr::sel_redo();
                Ulist()->RotateRedo(cur);
            }
        }
    }
}


// Static function.
// Asynchronous property change from text editor for physical
// mode.  Since physical cell properties are ascii strings, we
// have to explicitly change the property when done editing.
//
void
PrptyState::ltcallback(hyList *h, void *arg)
{
    ltobj *lt = (ltobj*)arg;
    if (PrptyCmd)
        PrptyCmd->prp_showselect(false);

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;

    Ulist()->ListCheck("adprp", cursdp, false);

    char *string;
    if (h->ref_type() == HLrefLongText)
        string = hyList::string(h, HYcvAscii, true);
    else
        string = hyList::string(h, HYcvPlain, true);
    hyList::destroy(h);

    if (PrptyCmd && PrptyCmd->Global) {

        for (sSel *sd = PrptyCmd->Shead; sd; sd = sd->next) {
            DSP()->ShowOdescPhysProperties(sd->pointer, ERASE);
            prpty_replace(sd->pointer, cursdp, prpmatch(sd->pointer,
                lt->pdesc, lt->value), lt->value, string);
        }
        Ulist()->CommitChanges(true);
        for (sSel *sd = PrptyCmd->Shead; sd; sd = sd->next)
            DSP()->ShowOdescPhysProperties(sd->pointer, DISPLAY);
    }
    else {
        DSP()->ShowOdescPhysProperties(lt->odesc, ERASE);
        prpty_replace(lt->odesc, cursdp, prpmatch(lt->odesc, lt->pdesc,
            lt->value), lt->value, string);

        Ulist()->CommitChanges(true);
        DSP()->ShowOdescPhysProperties(lt->odesc, DISPLAY);
    }

    delete [] string;
    delete lt;
    if (PrptyCmd)
        PrptyCmd->prp_showselect(true);
}


// Turn on/off the display of selected objects other than the currently
// marked object.
//
void
PrptyState::prp_showselect(bool show)
{
    if (Global || ShowAllSel) {
        if (!show)
            for (sSel *s = Shead; s; s = s->next)
                Selections.showUnselected(CurCell(), s->pointer);
        else
            for (sSel *s = Shead; s; s = s->next)
                Selections.showSelected(CurCell(), s->pointer);
    }
}


// Create the prompt used to solicit the property string.
// Return the existing property, if applicable.
// The prompt is returned in buf.
// The odesc argument is the source of the returned prpty desc.
// If true is returned, don't prompt but use string in buf.
//
bool
PrptyState::prp_get_prompt(bool global, CDo *odesc, char *buf, int szbuf)
{
    const char *glmsg = "Global %s? ";
    if (DSP()->CurMode() == Electrical) {
        // use the existing string as default
        switch (Value) {
        case P_NAME:
            if (global)
                snprintf(buf, szbuf, glmsg, "name");
            else
                strcpy(buf, "Name? ");
            break;
        case P_MODEL:
            if (global)
                snprintf(buf, szbuf, glmsg, "model");
            else
                strcpy(buf, "Model name? ");
            break;
        case P_VALUE:
            if (global)
                snprintf(buf, szbuf, glmsg, "value");
            else
                strcpy(buf, "Value? ");
            break;
        case P_PARAM:
            if (global)
                snprintf(buf, szbuf, glmsg, "parameter string");
            else
                strcpy(buf, "Parameter string? ");
            break;
        case P_OTHER:
            if (global)
                snprintf(buf, szbuf, glmsg, "string");
            else
                strcpy(buf, "Property string? ");
            break;
        case P_NOPHYS:
            strcpy(buf, "nophys");
            return (true);
        case P_FLATTEN:
            strcpy(buf, "flatten");
            return (true);
        case P_SYMBLC:
            strcpy(buf, "0");
            return (true);
        case P_RANGE:
            if (global)
                snprintf(buf, szbuf, glmsg, "range");
            else
                strcpy(buf, "Range? (2 unsigned integers, begin and end) ");
            break;
        case P_DEVREF:
            if (global)
                snprintf(buf, szbuf, glmsg, "reference device string");
            else
                strcpy(buf, "Reference device String? ");
            break;
        default:
            Value = P_MODEL;
            return (prp_get_prompt(global, odesc, buf, szbuf));
        }
    }
    else {
        if (global)
            snprintf(buf, szbuf, "Global property %d string? ", Value);
        else
            snprintf(buf, szbuf, "Property %d string? ", Value);
    }
    return (false);
}


// Push new text into executing input window.
//
void
PrptyState::prp_updtext(sSel *sl)
{
    char buf[256];
    if (!sl || !sl->pointer)
        return;
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    prp_get_prompt(false, sl->pointer, buf, sizeof(buf));
    CDp *pdesc = SelPrp;
    if (pdesc && pdesc->value() != Value)
        // shouldn't happen
        Value = pdesc->value();

    if (DSP()->CurMode() == Electrical) {
        bool use_lt = false;
        if (Value == P_VALUE || Value == P_PARAM || Value == P_OTHER)
            use_lt = true;
        if (pdesc) {
            if (Value == P_NAME) {
                // don't show default name
                if (((CDp_cname*)pdesc)->assigned_name()) {
                    hyList *h = new hyList(0,
                        ((CDp_cname*)pdesc)->assigned_name(), HYcvPlain);
                    PL()->EditHypertextPrompt(buf, h, false, PLedUpdate);
                    hyList::destroy(h);
                }
                else
                    PL()->EditHypertextPrompt(buf, 0, false, PLedUpdate);
            }
            else {
                switch (Value) {
                case P_MODEL:
                case P_VALUE:
                case P_PARAM:
                case P_OTHER:
                case P_DEVREF:
                    PL()->EditHypertextPrompt(buf, ((CDp_user*)pdesc)->data(),
                        use_lt, PLedUpdate);
                    break;
                case P_RANGE:
                    {
                        sLstr lstr;
                        ((CDp_range*)pdesc)->print(&lstr, 0, 0);
                        hyList *h = new hyList(0, lstr.string(), HYcvPlain);
                        PL()->EditHypertextPrompt(buf, h, false, PLedUpdate);
                        hyList::destroy(h);
                    }
                    break;

                default:
                    // shouldn't be here
                    return;
                }
            }
        }
        else
            PL()->EditHypertextPrompt(buf, 0, use_lt, PLedUpdate);
    }
    else {
        bool use_lt = true;
        hyList *hp = 0;
        if (!pdesc && Value >= XprpType && Value < XprpEnd) {
            char *s = XM()->GetPseudoProp(sl->pointer, Value);
            hp = new hyList(cursd, s, HYcvPlain);
            delete [] s;
            use_lt = false;
        }
        else if (pdesc && pdesc->string() && *pdesc->string()) {
            const char *lttok = HYtokPre HYtokLT HYtokSuf;
            if (lstring::prefix(lttok, pdesc->string())) {
                if (use_lt)
                    hp = new hyList(cursd, pdesc->string(), HYcvAscii);
                else
                    hp = new hyList(cursd,
                        pdesc->string() + strlen(lttok), HYcvPlain);
            }
            else
                hp = new hyList(cursd, pdesc->string(), HYcvPlain);
        }
        PL()->EditHypertextPrompt(buf, hp, use_lt, PLedUpdate);
        hyList::destroy(hp);
    }
}


namespace {
    // These are the properties cycled through with the arrow keys
    // when editing.  In physical mode, anything in the list is fine.
    //
    bool is_prp_editable(const CDp *pd)
    {
        if (DSP()->CurMode() == Electrical) {
            if (!pd)
                return (false);
            switch (pd->value()) {
            case P_NAME:
            case P_MODEL:
            case P_VALUE:
            case P_PARAM:
            case P_OTHER:
            case P_RANGE:
            case P_DEVREF:
                return (true);
            default:
                return (false);
            }
        }
        return (true);
    }
}


namespace {
    // Return true is the argument is a valid string for a P_NAME
    // property.  The first character must be alpha, and we can't
    // include any SPICE token separators.
    //
    bool nameprp_ok(const char *str)
    {
        if (!str)
            return (false);
        if (!isalpha(*str))
            return (false);
        str++;
        while (*str) {
            if (isspace(*str) || *str == '=' || *str == ',')
                return (false);
            str++;
        }
        return (true);
    }
}


// Add:  line = 0, which is always > 0, edit false
// Edit: line 0 or not, which is always -1, edit true
//
void
PrptyState::prp_add_elec(PrptyText *line, int which, bool edit)
{
    if (!Shead) {
        PL()->ShowPrompt(nosel);
        return;
    }
    if (!CurCell(Electrical) || DSP()->CurMode() != Electrical)
        return;

    if (!line) {
        switch (which) {
        case P_NAME:
        case P_MODEL:
        case P_VALUE:
        case P_PARAM:
        case P_RANGE:
        case P_DEVREF:
            line = ED()->PropertySelect(which);
            break;
        default:
            if (which < 0)
                line = ED()->PropertyCycle(0, &is_prp_editable, false);
            break;
        }
    }
    if (line && !line->prpty())
        line = 0;

    Value = which;
    SelPrp = 0;
    if (line) {
        SelPrp = line->prpty();
        Value = SelPrp->value();
    }
    if (!SelPrp && edit) {
        PL()->ShowPrompt("No editable property found - use Add.");
        Value = -1;
        return;
    }
    if (Value < 0)
        return;

    switch (Value) {
    case P_NOPHYS:
    case P_FLATTEN:
    case P_SYMBLC:
        if (!edit)
            break;
        // fallthrough
    default:
        PL()->ShowPrompt("This property can not be edited.");
        return;
    case P_NAME:
    case P_MODEL:
    case P_VALUE:
    case P_PARAM:
    case P_OTHER:
    case P_RANGE:
    case P_DEVREF:
    case XICP_PC_PARAMS:
        break;
    }

    if (Scur->pointer->type() != CDINSTANCE) {
        PL()->ShowPrompt("Can't modify properties of current object.");
        return;
    }
    CDc *cdesc = OCALL(Scur->pointer);
    CDs *msdesc = cdesc->masterCell();
    if (!msdesc) {
        PL()->ShowPrompt("Internal: instance has no cell pointer!");
        return;
    }
    if (!msdesc->isDevice()) {
        switch (Value) {
        case P_NAME:
        case P_PARAM:
        case P_OTHER:
        case P_NOPHYS:
        case P_FLATTEN:
        case P_SYMBLC:
        case P_RANGE:
            break;
        default:
            PL()->ShowPrompt("Can't add this property type to subcircuit.");
            return;
        }
    }
    else if (Value == P_FLATTEN || Value == P_SYMBLC) {
            PL()->ShowPrompt("Can't add this property type to device.");
            return;
    }
    if (Value == P_NAME && Global && !edit) {
        PL()->ShowPrompt("You don't want to set all names the same.");
        return;
    }

    Modifying = PRPadding;

    prp_showselect(false);
    if (edit) {
        PL()->RegisterArrowKeyCallbacks(down_callback, up_callback);
        ED()->RegisterPrptyBtnCallback(btn_callback);
    }
    bool ret = prp_get_string(false, edit);
    if (edit) {
        PL()->RegisterArrowKeyCallbacks(0, 0);
        ED()->RegisterPrptyBtnCallback(0);
    }
    if (!PrptyCmd)
        return;
    if (ret && Udata.hyl) {
        if (Value == P_NAME) {
            char *pstr = hyList::string(Udata.hyl, HYcvPlain, false);
            if (!nameprp_ok(pstr)) {
                delete [] pstr;
                PL()->ShowPrompt(
                    "Bad name, must start with alpha, exclude SPICE token "
                    "separators space, ',' and '='.");
                return;
            }
            delete [] pstr;
        }
        if (Global) {
            if (edit)
                prp_add_elec_glob_seq();
            else
                prp_add_elec_glob_all();
        }
        else
            prp_add_elec_noglob();
    }

    if (PrptyCmd) {
        if (Scur)
            prp_showselect(true);
        SelPrp = 0;
        Value = -1;
        Modifying = PRPquiescent;
    }
}


void
PrptyState::prp_add_elec_noglob()
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;
    Ulist()->ListCheck("adprp", cursde, false);
    CDc *cdesc = OCALL(Scur->pointer);
    CDp *pdesc = SelPrp;
    if (!pdesc && Value != P_OTHER)
        pdesc = cdesc->prpty(Value);
    ED()->prptyModify(cdesc, pdesc, Value, 0, Udata.hyl);
    Ulist()->CommitChanges(true);

    // Might have exited during modify.
    if (PrptyCmd) {
        hyList::destroy(Udata.hyl);
        Udata.hyl = 0;
    }
}


void
PrptyState::prp_add_elec_glob_all()
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;
    Ulist()->ListCheck("adprp", cursde, false);
    bool changemade = false;
    for (sSel *sd = Shead; sd; sd = sd->next) {
        if (sd->pointer->type() != CDINSTANCE)
            continue;
        CDc *cdesc = OCALL(sd->pointer);
        CDs *msdesc = cdesc->masterCell();
        if (!msdesc)
            continue;
        if (!msdesc->isDevice() && Value != P_NAME && Value != P_PARAM &&
                Value != P_RANGE && Value != P_FLATTEN)
            continue;
        ED()->prptyModify(cdesc, prpmatch(sd->pointer, 0, Value), Value, 0,
            Udata.hyl);
        changemade = true;
        // Might have exited during modify.
        if (!PrptyCmd)
            break;
    }
    if (changemade)
        Ulist()->CommitChanges(true);
    if (PrptyCmd) {
        hyList::destroy(Udata.hyl);
        Udata.hyl = 0;
    }
}


void
PrptyState::prp_add_elec_glob_seq()
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;

    // rotate the list so that Scur is head
    if (Scur != Shead) {
        sSel *s = Scur;
        while (s->next)
            s = s->next;
        s->next = Shead;
        Shead->prev = s;
        while (s->next != Scur)
            s = s->next;
        s->next = 0;
        Shead = Scur;
        Shead->prev = 0;
    }

    bool changemade = false;
    bool first = true;
    for (sSel *sd = Shead; sd; sd = sd->next) {
        bool ret = true;
        CDc *cdesc = OCALL(sd->pointer);
        if (!first) {
            if (sd->pointer->type() != CDINSTANCE)
                continue;
            CDs *msdesc = cdesc->masterCell();
            if (!msdesc)
                continue;
            if (!msdesc->isDevice() && Value != P_NAME && Value != P_PARAM &&
                    Value != P_RANGE && Value != P_FLATTEN)
                continue;
            Scur = sd;
            ED()->PopUpProperties(od_of(Scur), MODE_UPD, PRPnochange);
            ret = prp_get_string(false, false);
            if (!PrptyCmd)
                return;
        }
        if (!ret || !Udata.hyl)
            break;
        if (first) {
            Ulist()->ListCheck("adprp", cursde, false);
            first = false;
        }

        ED()->prptyModify(cdesc, prpmatch(cdesc, SelPrp, Value), Value, 0,
            Udata.hyl);
        changemade = true;
        // Might have exited during modify.
        if (PrptyCmd) {
            hyList::destroy(Udata.hyl);
            Udata.hyl = 0;
        }
        else
            break;
    }
    if (changemade)
        Ulist()->CommitChanges(true);
    if (PrptyCmd)
        Scur = Shead;
}


// Add:  line = 0, which is always -1, edit false
// Edit: line 0 or not, which is always -1, edit true
//
void
PrptyState::prp_add_phys(PrptyText *line, int which, bool edit)
{
    if (!Shead) {
        PL()->ShowPrompt(nosel);
        return;
    }
    if (!CurCell(Physical) || DSP()->CurMode() != Physical)
        return;

    if (!line && edit)
        line = ED()->PropertyCycle(0, &is_prp_editable, false);

    bool phony_add = false;
    if (line && !line->prpty()) {
        // If pseudo-property in edit mode, treat as Add...
        const char *s = strchr(line->head(), '(');
        if (s) {
            s++;
            Value = atoi(s);
            if (prpty_pseudo(Value)) {
                SelPrp = 0;
                edit = false;
                phony_add = true;
            }
        }
        // shouldn't get here
        if (!phony_add) {
            PL()->ShowPrompt("Property not editable.");
            Value = -1;
            return;
        }
    }

    if (!phony_add) {
        SelPrp = 0;
        Value = which;
        if (line) {
            SelPrp = line->prpty();
            Value = SelPrp->value();
        }
        if (Value == XICP_PC || prpty_reserved(Value) ||
                (edit && prpty_pseudo(Value))) {
            PL()->ShowPrompt("This property can not be edited.");
            SelPrp = 0;
            Value = -1;
            return;
        }
        if (!SelPrp && edit) {
            PL()->ShowPrompt(
                "No editable property selected - select first or use Add.");
            Value = -1;
            return;
        }
    }

    Modifying = PRPadding;

    prp_showselect(false);
    bool ret = true;
    if (Value < 0) {
        ret = prp_get_add_type(false);
        if (!PrptyCmd)
            return;
    }
    if (Value == XICP_PC || prpty_reserved(Value)) {
        PL()->ShowPrompt("This property can't be set by user.");
        SelPrp = 0;
        Value = -1;
        return;
    }

    if (ret && Scur && Value >= 0) {
        if (Value == XICP_NOMERGE) {
            if (edit) {
                PL()->ShowPrompt("Property not editable.");
                return;
            }
            delete [] Udata.string;
            Udata.string = lstring::copy("nomerge");
            if (Global)
                prp_add_phys_glob_all();
            else
                prp_add_phys_noglob();
        }
        else if (Value == XICP_EXT_FLATTEN) {
            if (edit) {
                PL()->ShowPrompt("Property not editable.");
                return;
            }
            delete [] Udata.string;
            Udata.string = lstring::copy("flatten");
            if (Global)
                prp_add_phys_glob_all();
            else
                prp_add_phys_noglob();
        }
        else {
            if (edit) {
                PL()->RegisterArrowKeyCallbacks(down_callback, up_callback);
                ED()->RegisterPrptyBtnCallback(btn_callback);
            }
            ret = prp_get_string(false, edit);
            if (edit) {
                PL()->RegisterArrowKeyCallbacks(0, 0);
                ED()->RegisterPrptyBtnCallback(0);
            }
            if (!PrptyCmd)
                return;
            if (!ret || !Udata.string) {
                delete [] Udata.string;
                Udata.string = 0;
            }
            else {
                if (Global) {
                    if (edit)
                        prp_add_phys_glob_seq();
                    else
                        prp_add_phys_glob_all();
                }
                else
                    prp_add_phys_noglob();
            }
        }
    }
    if (PrptyCmd) {
        if (Scur)
            prp_showselect(true);
        SelPrp = 0;
        Value = -1;
        Modifying = PRPquiescent;
    }
}


void
PrptyState::prp_add_phys_noglob()
{

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    Ulist()->ListCheck("adprp", cursdp, false);
    DSP()->ShowOdescPhysProperties(Scur->pointer, ERASE);
    prpty_replace(Scur->pointer, cursdp, SelPrp, Value, Udata.string);

    // Explicit redraw needed for pseudo-properties.
    // Might have exited during modify.
    Ulist()->CommitChanges(true);
    if (PrptyCmd) {
        delete [] Udata.string;
        Udata.string = 0;
        if (Scur)
            DSP()->ShowOdescPhysProperties(Scur->pointer, DISPLAY);
    }
}


void
PrptyState::prp_add_phys_glob_all()
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    Ulist()->ListCheck("adprp", cursdp, false);
    sSel *sn;
    for (sSel *sd = Shead; sd; sd = sn) {
        sn = sd->next;
        prpty_replace(sd->pointer, cursdp, 0, Value, Udata.string);
        // Might have exited during modify.
        if (!PrptyCmd)
            break;
    }
    if (PrptyCmd) {
        delete [] Udata.string;
        Udata.string = 0;
        for (sSel *sd = Shead; sd; sd = sd->next)
            DSP()->ShowOdescPhysProperties(sd->pointer, ERASE);
    }
    // Explicit redraw needed for pseudo-properties
    Ulist()->CommitChanges(true);
    if (PrptyCmd) {
        for (sSel *sd = Shead; sd; sd = sd->next)
            DSP()->ShowOdescPhysProperties(sd->pointer, DISPLAY);
    }
}


void
PrptyState::prp_add_phys_glob_seq()
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    // rotate the list so that Scur is head
    if (Scur != Shead) {
        sSel *s = Scur;
        while (s->next)
            s = s->next;
        s->next = Shead;
        Shead->prev = s;
        while (s->next != Scur)
            s = s->next;
        s->next = 0;
        Shead = Scur;
        Shead->prev = 0;
    }

    bool changemade = false;
    bool first = true;
    sSel *sn;
    for (sSel *sd = Shead; sd; sd = sn) {
        sn = sd->next;
        bool ret = true;
        if (!first) {
            Scur = sd;
            ED()->PopUpProperties(od_of(Scur), MODE_UPD, PRPnochange);
            ret = prp_get_string(false, false);
            if (!PrptyCmd)
                return;
        }
        if (!ret || !Udata.string)
            break;
        if (first) {
            Ulist()->ListCheck("adprp", cursdp, false);
            first = false;
        }

        prpty_replace(sd->pointer, cursdp,
                prpmatch(sd->pointer, SelPrp, Value), Value, Udata.string);
        changemade = true;
        // Might have exited during modify.
        if (PrptyCmd) {
            delete [] Udata.string;
            Udata.string = 0;
        }
        else
            break;
    }
    if (PrptyCmd)
        Scur = Shead;

    if (changemade) {
        if (PrptyCmd) {
            for (sSel *sd = Shead; sd; sd = sd->next)
                DSP()->ShowOdescPhysProperties(sd->pointer, ERASE);
        }
        // Explicit redraw needed for pseudo-properties
        Ulist()->CommitChanges(true);
        if (PrptyCmd) {
            for (sSel *sd = Shead; sd; sd = sd->next)
                DSP()->ShowOdescPhysProperties(sd->pointer, DISPLAY);
        }
    }
}


// Solicit from the user the type or value of a property being added.
// Return false if Esc entered, true otherwise.  The value is put into
// Value, which is -1 if no value was recognized.
//
bool
PrptyState::prp_get_add_type(bool global)
{
    const char *msg = global ?
        "Press Enter, or give property number to add to selected objects? " :
        "Property number? ";

    char tbuf[64];
    *tbuf = '\0';
    if (!global && Value >= 0)
        snprintf(tbuf, sizeof(tbuf), "%d", Value);
    Value = -1;
    char *in = PL()->EditPrompt(msg, (global || !*tbuf) ? 0 : tbuf);
    for (;;) {
        PL()->ErasePrompt();
        if (!in)
            return (false);
        int d;
        if (sscanf(in, "%d", &d) == 1 && d >= 0) {
            if (prpty_reserved(d))
                msg = "Given value is for internal use only, try again: ";
            else {
                Value = d;
                break;
            }
        }
        else
            msg = "Bad input, try again: ";
        Value = -1;
        in = PL()->EditPrompt(msg, (global || !*tbuf) ? 0 : tbuf);
    }
    return (true);
}


// Query the user for the property string to be used in added properties.
// Return false if Esc entered, true otherwise.
// It is assumed here that a null string is a valid return.
//
bool
PrptyState::prp_get_string(bool global, bool allow_switch)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);

    if (Value == XICP_PC_PARAMS && Scur->pointer->type() == CDINSTANCE) {
        char *pstr;
        bool ret = ED()->reparameterize((CDc*)Scur->pointer, &pstr);
        if (ret)
            Udata.string = pstr;
        else {
            Log()->ErrorLogV(mh::PCells,
                "Error, reparameterize failed:\n%s", Errs()->get_error());
        }
        return (ret);
    }

    char tbuf[256];
    bool immut = prp_get_prompt(global, Scur->pointer, tbuf, sizeof(tbuf));
    Udata.string = 0;
    Udata.hyl = 0;
    if (allow_switch && !immut) {
        const char *fs = "(Up/down arrows to select)  %s";
        if (strlen(tbuf) + strlen(fs) - 2  < 256) {
            char *tt = lstring::copy(tbuf);
            snprintf(tbuf, sizeof(tbuf), fs, tt);
            delete [] tt;
        }
    }
    if (DSP()->CurMode() == Electrical) {
        hyList *hnew;
        if (immut) {
            if (Value == P_NOPHYS) {
                char *in = PL()->EditPrompt("Device shorted in LVS? ", "n");
                in = lstring::strip_space(in);
                if (!in) {
                    PL()->ErasePrompt();
                    return (false);
                }
                if (in && (*in == 'y' || *in == 'Y'))
                    strcpy(tbuf, "shorted");
            }
            hnew = new hyList(cursd, tbuf, HYcvPlain);
        }
        else if (Value == P_NAME) {
            if (SelPrp && ((CDp_cname*)SelPrp)->assigned_name()) {
                hyList *h = new hyList(0, ((CDp_cname*)SelPrp)->assigned_name(),
                    HYcvPlain);
                hnew = PL()->EditHypertextPrompt(tbuf, h, false);
                hyList::destroy(h);
            }
            else
                hnew = PL()->EditHypertextPrompt(tbuf, 0, false);
        }
        else {
            bool use_lt = false;
            if (Value == P_VALUE || Value == P_PARAM || Value == P_OTHER)
                use_lt = true;
            if (SelPrp) {
                switch (Value) {
                case P_MODEL:
                case P_VALUE:
                case P_PARAM:
                case P_OTHER:
                case P_DEVREF:
                    hnew = PL()->EditHypertextPrompt(tbuf,
                        ((CDp_user*)SelPrp)->data(), use_lt);
                    break;
                case P_RANGE:
                    {
                        sLstr lstr;
                        ((CDp_range*)SelPrp)->print(&lstr, 0, 0);
                        hyList *h = new hyList(0, lstr.string(), HYcvPlain);
                        hnew = PL()->EditHypertextPrompt(tbuf, h, false);
                        hyList::destroy(h);
                    }
                    break;

                default:
                    // shouldn't be here
                    hnew = PL()->EditHypertextPrompt(tbuf, 0, use_lt);
                    break;
                }
            }
            else
                hnew = PL()->EditHypertextPrompt(tbuf, 0, use_lt);
        }
        PL()->ErasePrompt();
        if (!hnew)
            return (false);
        hnew->trim_white_space();
        if (hnew->ref_type() == HLrefText && !hnew->text()[0]) {
            hyList::destroy(hnew);
            hnew = 0;
        }
        Udata.hyl = hnew;
    }
    else {
        if (immut) {
            Udata.string = lstring::copy(tbuf);
            return (true);
        }
        bool use_lt = true;
        hyList *hp = 0;
        if (!SelPrp && (Value >= XprpType && Value < XprpEnd)) {
            char *s = XM()->GetPseudoProp(Scur->pointer, Value);
            hp = new hyList(cursd, s, HYcvPlain);
            delete [] s;
            use_lt = false;
        }
        else if (SelPrp && SelPrp->string() && *SelPrp->string()) {
            const char *lttok = HYtokPre HYtokLT HYtokSuf;
            if (lstring::prefix(lttok, SelPrp->string())) {
                if (use_lt)
                    hp = new hyList(cursd, SelPrp->string(), HYcvAscii);
                else
                    hp = new hyList(cursd,
                        SelPrp->string() + strlen(lttok), HYcvPlain);
            }
            else
                hp = new hyList(cursd, SelPrp->string(), HYcvPlain);
        }
        ltobj *lt = new ltobj(cursd, Scur->pointer, SelPrp, Value);
        hyList *hnew = PL()->EditHypertextPrompt(tbuf, hp, use_lt,
            PLedStart, PLedNormal, ltcallback, lt);
        hyList::destroy(hp);
        PL()->ErasePrompt();
        if (!hnew) {
            delete lt;
            return (false);
        }
        if (hnew->ref_type() == HLrefLongText)
            // text editor popped, calls ltcallback when done
            return (true);

        delete lt;
        if (hnew->ref_type() == HLrefText && !hnew->text()[0]) {
            hyList::destroy(hnew);
            hnew = 0;
        }
        if (hnew) {
            Udata.string = hyList::string(hnew, HYcvPlain, true);
            hyList::destroy(hnew);
        }
    }
    return (true);
}


void
PrptyState::prp_rm(PrptyText *line)
{
    if (!Shead) {
        PL()->ShowPrompt(nosel);
        return;
    }
    if (!line)
        return;
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    SelPrp = line->prpty();
    if (DSP()->CurMode() == Electrical) {
        if (SelPrp) {
            switch (SelPrp->value()) {
            case P_NAME:
            case P_MODEL:
            case P_VALUE:
            case P_PARAM:
            case P_OTHER:
            case P_NOPHYS:
            case P_FLATTEN:
            case P_SYMBLC:
            case P_RANGE:
            case P_DEVREF:
                break;
            default:
                SelPrp = 0;
                break;
            }
        }
    }
    if (!SelPrp) {
        PL()->ShowPrompt("This property can not be deleted.");
        return;
    }

    Modifying = PRPdeleting;

    prp_showselect(false);
    bool ret = true;
    if (SelPrp) {
        Value = SelPrp->value();
        Vmask = maskof(Value);
    }
    else {
        Value = -1;
        Vmask = 0;
        ret = prp_get_rm_type(Global);
        if (!PrptyCmd)
            return;
    }
    if (ret && Scur) {
        if (Global) {
            Ulist()->ListCheck("rmprp", cursd, false);
            for (sSel *sd = Shead; sd; sd = sd->next) {
                if (DSP()->CurMode() == Electrical &&
                        sd->pointer->type() != CDINSTANCE)
                    continue;
                prp_remove(sd, SelPrp);
            }
            Ulist()->CommitChanges(true);
        }
        else {
            if (DSP()->CurMode() == Electrical &&
                    Scur->pointer->type() != CDINSTANCE)
                PL()->ShowPrompt("Can't delete properties of this object.");
            else {
                Ulist()->ListCheck("rmprp", cursd, false);
                prp_remove(Scur, SelPrp);
                Ulist()->CommitChanges(true);
            }
        }
    }

    if (PrptyCmd) {
        if (Scur)
            prp_showselect(true);
        Value = -1;
        Vmask = 0;
        SelPrp = 0;
        Modifying = PRPquiescent;
    }
}


// Solicit the type(s) or number of a property to be removed.
// Return false if Esc entered, true otherwise.
//
bool
PrptyState::prp_get_rm_type(bool global)
{
    if (DSP()->CurMode() == Electrical) {
        const char *glmsg = global ?
            "Property types (nmvpoys) to remove from all selected devices? " :
            "Property types (nmvpoys) to be removed? ";

        Vmask = 0;
        for (;;) {
            char *in = PL()->EditPrompt(glmsg, 0);
            PL()->ErasePrompt();
            if (!in)
                return (false);
            for (char *s = in; *s; s++) {
                switch (*s) {
                case 'n':
                case 'N':
                    Vmask |= NAME_MASK;
                    break;
                case 'm':
                case 'M':
                    Vmask |= MODEL_MASK;
                    break;
                case 'v':
                case 'V':
                    Vmask |= VALUE_MASK;
                    break;
                case 'i':
                case 'I':
                case 'p':
                case 'P':
                    Vmask |= PARAM_MASK;
                    break;
                case 'o':
                case 'O':
                    Vmask |= OTHER_MASK;
                    break;
                case 'y':
                case 'Y':
                    Vmask |= NOPHYS_MASK;
                    break;
                case 'f':
                case 'F':
                    Vmask |= FLATTEN_MASK;
                    break;
                case 's':
                case 'S':
                    Vmask |= NOSYMB_MASK;
                    break;
                case 'r':
                case 'R':
                    Vmask |= RANGE_MASK;
                    break;
                case 'd':
                case 'D':
                    Vmask |= DEVREF_MASK;
                    break;
                }
            }
            if (Vmask)
                break;
            glmsg =
            "Bad input, try again.  You must give one or more of n,m,v,p,o. ";
        }
    }
    else {
        const char *msg2 = global ?
            "Property number to remove from all selected ojbects? " :
            "Property number to be removed? ";
        for (;;) {
            char *in = PL()->EditPrompt(msg2, 0);
            PL()->ErasePrompt();
            if (!in)
                return (false);
            int d;
            if (sscanf(in, "%d", &d) == 1 && d >= 0) {
                if (prpty_reserved(d))
                    msg2 = "Given value is for internal use only, try again: ";
                else {
                    Value = d;
                    break;
                }
            }
            else
                msg2 = "Bad input, try again: ";
        }
    }
    return (true);
}


// Remove properties from the object in sd.  The values to remove
// are in Value or Vmask.  Remove all of the properties of the given
// type.  If pdesc is given, remove properties that "match" pdesc.
//
void
PrptyState::prp_remove(sSel *sd, CDp *pdesc)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    if (DSP()->CurMode() == Electrical) {
        if (sd->pointer->type() != CDINSTANCE)
            return;
        if (pdesc) {
            pdesc = prpmatch(sd->pointer, pdesc, pdesc->value());
            if (!pdesc)
                return;
            if (pdesc->value() == P_NAME) {
                Udata.hyl = 0;
                // replace the property with default
                ED()->prptyModify(OCALL(sd->pointer), pdesc,  P_NAME, 0, 0);
            }
            else {
                Ulist()->RecordPrptyChange(cursd, sd->pointer, pdesc, 0);
                CDla *olabel = pdesc->bound();
                if (olabel) {
                    Ulist()->RecordObjectChange(cursd, olabel, 0);
                    DSP()->RedisplayArea(&olabel->oBB());
                }
            }
            return;
        }

        for (int i = 0; ; i++) {
            int value;
            if (i == 0) {
                if (!(Vmask & MODEL_MASK))
                    continue;
                value = P_MODEL;
            }
            else if (i == 1) {
                if (!(Vmask & VALUE_MASK))
                    continue;
                value = P_VALUE;
            }
            else if (i == 2) {
                if (!(Vmask & PARAM_MASK))
                    continue;
                value = P_PARAM;
            }
            else if (i == 3) {
                if (!(Vmask & OTHER_MASK))
                    continue;
                value = P_OTHER;
            }
            else if (i == 4) {
                if (!(Vmask & NOPHYS_MASK))
                    continue;
                value = P_NOPHYS;
            }
            else if (i == 5) {
                if (!(Vmask & FLATTEN_MASK))
                    continue;
                value = P_FLATTEN;
            }
            else if (i == 6) {
                if (!(Vmask & NOSYMB_MASK))
                    continue;
                value = P_SYMBLC;
            }
            else if (i == 7) {
                if (!(Vmask & RANGE_MASK))
                    continue;
                value = P_RANGE;
            }
            else if (i == 8) {
                if (!(Vmask & NAME_MASK))
                    continue;
                value = P_NAME;
            }
            else if (i == 9) {
                if (!(Vmask & DEVREF_MASK))
                    continue;
                value = P_DEVREF;
            }
            else
                break;
            while ((pdesc = sd->pointer->prpty(value)) != 0) {
                if (value == P_NAME) {
                    Udata.hyl = 0;
                    // replace the property with default
                    ED()->prptyModify(OCALL(sd->pointer), pdesc,  P_NAME,
                        0, 0);
                }
                else {
                    Ulist()->RecordPrptyChange(cursd, sd->pointer, pdesc, 0);
                    CDla *olabel = pdesc->bound();
                    if (olabel) {
                        Ulist()->RecordObjectChange(cursd, olabel, 0);
                        DSP()->RedisplayArea(&olabel->oBB());
                    }
                }
            }
        }
    }
    else {
        if (pdesc) {
            // Remove at most one matching property
            pdesc = prpmatch(sd->pointer, pdesc, pdesc->value());
            if (pdesc) {
                DSP()->ShowOdescPhysProperties(sd->pointer, ERASE);
                Ulist()->RecordPrptyChange(cursd, sd->pointer, pdesc, 0);
                DSP()->ShowOdescPhysProperties(sd->pointer, DISPLAY);
            }
        }
        else {
            // Remove all properties of value Value
            DSP()->ShowOdescPhysProperties(sd->pointer, ERASE);
            while ((pdesc = sd->pointer->prpty(Value)) != 0)
                Ulist()->RecordPrptyChange(cursd, sd->pointer, pdesc, 0);
            DSP()->ShowOdescPhysProperties(sd->pointer, DISPLAY);
        }
    }
}


// Static function.
// Called from the Properties panel, sets the current editing
// property to the one just clicked on.
//
int
PrptyState::btn_callback(PrptyText *pt)
{
    if (!pt->prpty())
        return (false);
    PrptyCmd->SelPrp = pt->prpty();
    PrptyCmd->Value = PrptyCmd->SelPrp->value();
    PrptyCmd->prp_updtext(PrptyCmd->Scur);
    return (true);
}


// Static function.
// Called from the hypertext editor, advances prompt to next
// property.
//
void
PrptyState::down_callback()
{
    if (!PrptyCmd || !PrptyCmd->Scur || !PrptyCmd->Scur->pointer->prpty_list())
        return;

    PrptyText *pt = ED()->PropertyCycle(PrptyCmd->SelPrp, &is_prp_editable, false);
    if (!pt)
        return;
    PrptyCmd->SelPrp = pt->prpty();
    PrptyCmd->Value = PrptyCmd->SelPrp->value();
    PrptyCmd->prp_updtext(PrptyCmd->Scur);
}


// Static function.
// Called from the hypertext editor, advances prompt to previous
// property.
//
void
PrptyState::up_callback()
{
    if (!PrptyCmd || !PrptyCmd->Scur || !PrptyCmd->Scur->pointer->prpty_list())
        return;

    PrptyText *pt = ED()->PropertyCycle(PrptyCmd->SelPrp, &is_prp_editable, true);
    if (!pt)
        return;
    PrptyCmd->SelPrp = pt->prpty();
    PrptyCmd->Value = PrptyCmd->SelPrp->value();
    PrptyCmd->prp_updtext(PrptyCmd->Scur);
}
// End of PrptyState functions.


namespace {
    // Return a property from odesc that "matches" pdesc, or val if
    // pdesc is nil.
    //
    CDp *
    prpmatch(CDo *odesc, CDp *pdesc, int val)
    {
        if (!odesc)
            return (0);
        if (DSP()->CurMode() == Electrical) {
            if (!pdesc) {
                if (val >= 0 && val != P_OTHER)
                    return (odesc->prpty(val));
            }
            else {
                if (pdesc->value() != P_OTHER)
                    return (odesc->prpty(pdesc->value()));
                // Return a P_OTHER property with matching text
                char *s1 = hyList::string(((CDp_user*)pdesc)->data(), HYcvPlain,
                    false);
                for (CDp *pd = odesc->prpty_list(); pd; pd = pd->next_prp()) {
                    if (pd->value() == P_OTHER) {
                        char *s2 = hyList::string(((CDp_user*)pd)->data(),
                            HYcvPlain, false);
                        int j = strcmp(s1, s2);
                        delete [] s2;
                        if (!j) {
                            delete [] s1;
                            return (pd);
                        }
                    }
                }
                delete [] s1;
            }
        }
        else if (pdesc && pdesc->string()) {
            for (CDp *pd = odesc->prpty_list(); pd; pd = pd->next_prp()) {
                if (pd->value() == pdesc->value() && pd->string() &&
                        !strcmp(pdesc->string(), pd->string()))
                    return (pd);
            }
        }
        return (0);
    }


    int
    maskof(int prpty)
    {
        if (DSP()->CurMode() == Physical)
            return(0);
        switch(prpty) {
        case P_NAME:
            return (NAME_MASK);
        case P_MODEL:
            return (MODEL_MASK);
        case P_VALUE:
            return (VALUE_MASK);
        case P_PARAM:
            return (PARAM_MASK);
        case P_OTHER:
            return (OTHER_MASK);
        case P_NOPHYS:
            return (NOPHYS_MASK);
        case P_FLATTEN:
            return (FLATTEN_MASK);
        case P_SYMBLC:
            return (NOSYMB_MASK);
        case P_RANGE:
            return (RANGE_MASK);
        case P_DEVREF:
            return (DEVREF_MASK);
        default:
            return (0);
        }
    }
}


//----------------------------------------------------------------------
// The props command - show physical properties on-screen, and allow
// editing
//----------------------------------------------------------------------

namespace {
    // We don't show internal properties.
    //
    inline bool is_showable(CDp *pdesc)
    {
        if (prpty_internal(pdesc->value()))
            return (false);
        return (true);
    }


    struct sPrpPointer
    {
        sPrpPointer() { ctrl_d_entered = false; }
        ~sPrpPointer() { ctrl_d_entered = false; }

        static void ctrl_d_cb();
        bool point_at_prpty(WindowDesc*, int, int, CDo**, CDp**);
        bool is_ctrl_d() { return (ctrl_d_entered); }

    private:
        void find_loc(CDo*, int*, int*, int);

        static bool ctrl_d_entered;
    };
}

bool sPrpPointer::ctrl_d_entered = false;


// If x,y fall on a physical property text area, allow editing of the
// property.  Return true if the editor was entered.
//
bool
cEdit::editPhysPrpty()
{
    WindowDesc *wdesc = EV()->CurrentWin();
    if (!wdesc || wdesc->Mode() != Physical ||
            !wdesc->Attrib()->show_phys_props())
        return (false);
    if (DSP()->CurMode() != Physical)
        return (false);
    CDs *cursd = CurCell();
    if (!cursd)
        return (false);
    int x, y;
    EV()->Cursor().get_raw(&x, &y);
    CDo *odesc = 0;
    CDp *pdesc = 0;
    sPrpPointer PP;
    if (PP.point_at_prpty(wdesc, x, y, &odesc, &pdesc)) {
        char tbuf[64];
        snprintf(tbuf, sizeof(tbuf), "%d", pdesc->value());
        PL()->RegisterCtrlDCallback(PP.ctrl_d_cb);
        char *in = PL()->EditPrompt("Edit number: ", tbuf);
        PL()->RegisterCtrlDCallback(0);
        if (in) {
            int val;
            if (sscanf(in, "%d", &val) < 1)
                val = pdesc->value();
            const char *lttok = HYtokPre HYtokLT HYtokSuf;
            hyList *hp;
            if (pdesc->string() && lstring::prefix(lttok, pdesc->string()))
                hp = new hyList(cursd, pdesc->string(), HYcvAscii);
            else
                hp = new hyList(cursd, pdesc->string(), HYcvPlain);
            ltobj *lt = new ltobj(cursd, odesc, pdesc, val);
            PL()->RegisterCtrlDCallback(PP.ctrl_d_cb);
            hyList *hnew = PL()->EditHypertextPrompt("Edit string: ", hp,
                true, PLedStart, PLedNormal, PrptyState::ltcallback, lt);
            PL()->RegisterCtrlDCallback(0);
            hyList::destroy(hp);

            if (!hnew) {
                PL()->ErasePrompt();
                return (true);
            }
            if (hnew->ref_type() == HLrefLongText) {
                // text editor popped, calls ltcallback when done
                PL()->ErasePrompt();
                return (true);
            }
            if (hnew->ref_type() == HLrefText && !hnew->text()[0]) {
                hyList::destroy(hnew);
                hnew = 0;
            }
            if (hnew) {
                char *string = hyList::string(hnew, HYcvPlain, true);
                hyList::destroy(hnew);
                DSP()->ShowOdescPhysProperties(odesc, ERASE);
                Ulist()->ListCheck("editpp", cursd, false);
                CDp *op = pdesc;
                pdesc = new CDp(string, val);
                delete [] string;
                Ulist()->RecordPrptyChange(cursd, odesc, op, pdesc);
                Ulist()->CommitChanges();
                DSP()->ShowOdescPhysProperties(odesc, DISPLAY);
                PL()->ErasePrompt();
                return (true);
            }
        }
        if (PP.is_ctrl_d()) {
            DSP()->ShowOdescPhysProperties(odesc, ERASE);
            Ulist()->ListCheck("delpp", cursd, false);
            Ulist()->RecordPrptyChange(cursd, odesc, pdesc, 0);
            Ulist()->CommitChanges();
            DSP()->ShowOdescPhysProperties(odesc, DISPLAY);
        }
        PL()->ErasePrompt();
        return (true);
    }
    return (false);
}


namespace {
    void
    sPrpPointer::ctrl_d_cb()
    {
        ctrl_d_entered = true;
    }


    // If x,y fall on a physical property text area, return true and set the
    // pointer args to the object and the property.
    //
    bool
    sPrpPointer::point_at_prpty(WindowDesc *wdesc, int xp, int yp, CDo **oret,
        CDp **pret)
    {
        CDs *cursdp = CurCell(Physical);
        if (!cursdp)
            return (false);
        int delta = wdesc->LogScale(DSP()->PhysPropSize());
        BBox BB;
        BB.left = xp - delta;
        BB.right = xp + delta;
        BB.bottom = yp - delta;
        BB.top = yp + delta;
        CDg gdesc;
        gdesc.init_gen(cursdp, CellLayer(), &BB);
        CDo *pointer;
        while ((pointer = gdesc.next()) != 0) {
            if (!pointer->is_normal())
                continue;
            CDp *pdesc = pointer->prpty_list();
            if (!pdesc)
                continue;
            bool locfound = false;
            int x = 0, y = 0;
            for ( ; pdesc; pdesc = pdesc->next_prp()) {
                if (is_showable(pdesc)) {
                    if (!locfound) {
                        find_loc(pointer, &x, &y, delta);
                        locfound = true;
                    }
                    int w, h, nl;
                    {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%d ", pdesc->value());
                        sLstr lstr;
                        lstr.add(buf);
                        lstr.add(pdesc->string());
                        nl = DSP()->DefaultLabelSize(lstr.string(),
                            Physical, &w, &h);
                    }
                    w = (w*delta*nl)/h;
                    h = delta*nl;
                    y -= h - delta;
                    if (xp >= x && xp <= x+w && yp >= y-delta && yp <= y) {
                        *oret = pointer;
                        *pret = pdesc;
                        return (true);
                    }
                    y -= delta;
                }
            }
        }

        CDsLgen lgen(cursdp);
        CDl *ld;
        while ((ld = lgen.next()) != 0) {
            gdesc.init_gen(cursdp, ld, &BB);
            while ((pointer = gdesc.next()) != 0) {
                if (!pointer->is_normal())
                    continue;
                CDp *pdesc = pointer->prpty_list();
                if (!pdesc)
                    continue;
                bool locfound = false;
                int x = 0, y = 0;
                for ( ; pdesc; pdesc = pdesc->next_prp()) {
                    if (is_showable(pdesc)) {
                        if (!locfound) {
                            find_loc(pointer, &x, &y, delta);
                            locfound = true;
                        }
                        int w, h, nl;
                        {
                            char buf[32];
                            snprintf(buf, sizeof(buf), "%d ", pdesc->value());
                            sLstr lstr;
                            lstr.add(buf);
                            lstr.add(pdesc->string());
                            nl = DSP()->DefaultLabelSize(lstr.string(),
                                Physical, &w, &h);
                        }
                        w = (w*delta*nl)/h;
                        h = delta*nl;
                        y -= h - delta;
                        if (xp >= x && xp <= x+w && yp >= y && yp <= y+h) {
                            *oret = pointer;
                            *pret = pdesc;
                            return (true);
                        }
                        y -= delta;
                    }
                }
            }
        }
        return (false);
    }


    // Return screen coordinates for physical properties of odesc.
    // Arg delta is approx. text height.
    //
    void
    sPrpPointer::find_loc(CDo *odesc, int *x, int *y, int delta)
    {
        if (odesc->type() == CDWIRE) {
            // leftmost end
            const Point *pts = OWIRE(odesc)->points();
            int num = OWIRE(odesc)->numpts();
            int wid = OWIRE(odesc)->wire_width()/2;
            if (pts[0].x < pts[num-1].x || (pts[0].x == pts[num-1].x &&
                    pts[0].y > pts[num-1].y)) {
                *x = pts[0].x - wid;
                *y = pts[0].y + wid - delta;
            }
            else {
                *x = pts[num-1].x - wid;
                *y = pts[num-1].y + wid - delta;
            }
        }
        else if (odesc->type() == CDPOLYGON) {
            // leftmost vertex with largest y
            const Point *pts = OPOLY(odesc)->points();
            int num = OPOLY(odesc)->numpts();
            int minx = CDinfinity;
            int maxy = -CDinfinity;
            int iref = 0;
            for (int i = 0; i < num; i++) {
                if (pts[i].x < minx || (pts[i].x == minx && pts[i].y > maxy)) {
                    minx = pts[i].x;
                    maxy = pts[i].y;
                    iref = i;
                }
            }
            *x = pts[iref].x;
            *y = pts[iref].y - delta;
        }
        else {
            // upper left corner
            *x = odesc->oBB().left;
            *y = odesc->oBB().top - delta;
        }
    }
    // End of sPrpPointer functions.
}


//----------------------------------------------------------------------
// Pseudo-property processing - modify objects according to text.
//----------------------------------------------------------------------

namespace {
    Point *scalepts(const BBox*, const BBox*, const Point*, int);
    Point *getpts(const char*, int*);
}


// Set attributes for pseudo-prooperty value.  If the object is a
// copy, it is modified directly.  Otherwise the database object is
// replaced.  We don't do merging here, since 1) the new box probably
// has to be moved, such as for XprpMagn, and 2) this confuses the
// Properties Editor.
//
bool
cEdit::acceptPseudoProp(CDo *odesc, CDs *sdesc, int val, const char *string)
{
    if (!odesc || !sdesc)
        return (false);
    if (!string) {
        Errs()->add_error("null string encountered");
        return (false);
    }
    if (val == XprpFlags) {
        char *tok;
        unsigned flags = 0;
        while ((tok = lstring::gettok(&string)) != 0) {
            if (isdigit(*tok)) {
                for (char *t = tok; *t; t++) {
                    if (!isdigit(*t))
                        break;
                    int n = *t - '0';
                    if (n >= DSP_NUMWINS)
                        break;
                    flags |= CDexpand << n;
                }
            }
            else {
                for (FlagDef *f = OdescFlags; f->name; f++) {
                    if (lstring::cieq(tok, f->name))
                        flags |= f->value;
                }
            }
            delete [] tok;
        }
        odesc->set_flags(flags);
        return (true);
    }
    if (val == XprpState) {
        if (lstring::cieq(string, "normal"))
            odesc->set_state(CDobjVanilla);
        else if (lstring::cieq(string, "selected"))
            odesc->set_state(CDobjSelected);
        else if (lstring::cieq(string, "deleted"))
            odesc->set_state(CDobjDeleted);
        else if (lstring::cieq(string, "incomplete"))
            odesc->set_state(CDobjIncomplete);
        else if (lstring::cieq(string, "internal"))
            odesc->set_state(CDobjInternal);
        else {
            Errs()->add_error("error, unknown state keyword");
            return (false);
        }
        return (true);
    }
    if (val == XprpGroup) {
        int d;
        if (sscanf(string, "%d", &d) != 1) {
            Errs()->add_error("syntax error, expecting integer");
            return (false);
        }
        odesc->set_group(d);
        return (true);
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
    return (acceptBoxPseudoProp(odesc, sdesc, val, string));
poly:
    return (acceptPolyPseudoProp((CDpo*)odesc, sdesc, val, string));
wire:
    return (acceptWirePseudoProp((CDw*)odesc, sdesc, val, string));
label:
    return (acceptLabelPseudoProp((CDla*)odesc, sdesc, val, string));
inst:
    return (acceptInstPseudoProp((CDc*)odesc, sdesc, val, string));
}


// Handle pseudo-properties applied to boxes.
//
bool
cEdit::acceptBoxPseudoProp(CDo *odesc, CDs *sdesc, int val,
    const char *string)
{
    // For boxes, accept all (physical or electrical, copy or not).  

    if (val == XprpBB) {
        BBox BB;
        if (sscanf(string, "%d,%d %d,%d", &BB.left, &BB.bottom,
                &BB.right, &BB.top) != 4) {
            Errs()->add_error(
                "syntax error, expecting l,b r,t coordinates");
            return (false);
        }
        BB.fix();
        if (odesc->is_copy())
            odesc->set_oBB(BB);
        else {
            CDo *newo = sdesc->newBox(odesc, &BB, odesc->ldesc(),
                odesc->prpty_list());
            if (!newo) {
                Errs()->add_error("newBox failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpLayer) {
        CDl *ld = CDldb()->findLayer(string, sdesc->displayMode());
        if (!ld) {
            Errs()->add_error("layer %s not found", string);
            return (false);
        }
        if (odesc->is_copy())
            odesc->set_ldesc(ld);
        else {
            CDo *newo = sdesc->newBox(odesc, &odesc->oBB(), ld,
                odesc->prpty_list());
            if (!newo) {
                Errs()->add_error("newBox failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpCoords) {
        Poly poly;
        poly.points = getpts(string, &poly.numpts);
        if (!poly.points)
            return (false);
        if (poly.is_rect()) {
            BBox BB(poly.points);
            delete [] poly.points;
            if (odesc->is_copy())
                odesc->set_oBB(BB);
            else {
                CDo *newo = sdesc->newBox(odesc, &BB, odesc->ldesc(),
                    odesc->prpty_list());
                if (!newo) {
                    Errs()->add_error("newBox failed");
                    return (false);
                }
            }
            return (true);
        }
        if (poly.numpts < 4) {
            delete [] poly.points;
            Errs()->add_error("poly/box has too few points");
            return (false);
        }
        if (odesc->is_copy()) {
            delete [] poly.points;
            Errs()->add_error("can't change box copy to polygon.");
            return (false);
        }
        else {
            CDo *newo = sdesc->newPoly(odesc, &poly, odesc->ldesc(),
                odesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newPoly failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpMagn) {
        double mag;
        if (sscanf(string, "%lf", &mag) != 1) {
            Errs()->add_error("syntax error, number expected");
            return (false);
        }
        if (mag < CDMAGMIN || mag > CDMAGMAX) {
            Errs()->add_error("error, number %g-%g expected",
                CDMAGMIN, CDMAGMAX);
            return (false);
        }
        // The box lower-left corner remains fixed.
        BBox BB(odesc->oBB().left, odesc->oBB().bottom,
            odesc->oBB().left + mmRnd(mag*odesc->oBB().width()),
            odesc->oBB().bottom + mmRnd(mag*odesc->oBB().height()));

        if (odesc->is_copy())
            odesc->set_oBB(BB);
        else {
            CDo *newo = sdesc->newBox(odesc, &BB, odesc->ldesc(),
                odesc->prpty_list());
            if (!newo) {
                Errs()->add_error("newBox failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpXY) {
        int n;
        Point *p = getpts(string, &n);
        if (!p)
            return (false);
        int px = p->x;
        int py = p->y;
        delete [] p;
        if (px != odesc->oBB().left || py != odesc->oBB().bottom) {
            BBox BB(px, py, px + odesc->oBB().right - odesc->oBB().left,
                py + odesc->oBB().top - odesc->oBB().bottom);
            if (odesc->is_copy())
                odesc->set_oBB(BB);
            else {
                CDo *newo = sdesc->newBox(odesc, &BB, odesc->ldesc(),
                    odesc->prpty_list());
                if (!newo) {
                    delete [] p;
                    Errs()->add_error("newBox failed");
                    return (false);
                }
            }
        }
        return (true);
    }
    if (val == XprpWidth) {
        int w;
        if (sscanf(string, "%d", &w) != 1 || w <= 0) {
            Errs()->add_error("error, bad width");
            return (false);
        } 
        BBox BB(odesc->oBB());
        BB.right = BB.left + w;

        if (odesc->is_copy())
            odesc->set_oBB(BB);
        else {
            CDo *newo = sdesc->newBox(odesc, &BB, odesc->ldesc(),
                odesc->prpty_list());
            if (!newo) {
                Errs()->add_error("newBox failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpHeight) {
        int h;
        if (sscanf(string, "%d", &h) != 1 || h <= 0) {
            Errs()->add_error("error, bad height");
            return (false);
        } 
        BBox BB(odesc->oBB());
        BB.top = BB.bottom + h;

        if (odesc->is_copy())
            odesc->set_oBB(BB);
        else {
            CDo *newo = sdesc->newBox(odesc, &BB, odesc->ldesc(),
                odesc->prpty_list());
            if (!newo) {
                Errs()->add_error("newBox failed");
                return (false);
            }
        }
        return (true);
    }
    Errs()->add_error("unknown pseudo-property %d", val);
    return (false);
}


// Handle pseudo-properties applied to polygons.
//
bool
cEdit::acceptPolyPseudoProp(CDpo *pdesc, CDs *sdesc, int val,
    const char *string)
{
    // For polys, accept all (physical or electrical, copy or not).  

    if (val == XprpBB) {
        BBox BB;
        if (sscanf(string, "%d,%d %d,%d", &BB.left, &BB.bottom,
                &BB.right, &BB.top) != 4) {
            Errs()->add_error(
                "syntax error, expecting l,b r,t coordinates");
            return (false);
        }
        BB.fix();
        int num = pdesc->numpts();
        Poly po(num, scalepts(&BB, &pdesc->oBB(), pdesc->points(), num));
        if (pdesc->is_copy()) {
            pdesc->set_oBB(BB);
            delete [] pdesc->points();
            pdesc->set_points(po.points);
        }
        else {
            CDo *newo = sdesc->newPoly(pdesc, &po, pdesc->ldesc(),
                pdesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newPoly failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpLayer) {
        CDl *ld = CDldb()->findLayer(string, sdesc->displayMode());
        if (!ld) {
            Errs()->add_error("layer %s not found", string);
            return (false);
        }
        if (pdesc->is_copy())
            pdesc->set_ldesc(ld);
        else {
            int num = pdesc->numpts();
            Poly po(num, Point::dup(pdesc->points(), num));
            CDo *newo = sdesc->newPoly(pdesc, &po, ld, pdesc->prpty_list(),
                false);
            if (!newo) {
                Errs()->add_error("newPoly failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpCoords) {
        Poly po;
        po.points = getpts(string, &po.numpts);
        if (!po.points)
            return (false);
        if (po.is_rect() && !pdesc->is_copy()) {
            BBox BB(po.points);
            delete [] po.points;
            CDo *newo = sdesc->newBox(pdesc, &BB, pdesc->ldesc(),
                pdesc->prpty_list());
            if (!newo) {
                Errs()->add_error("newBox failed");
                return (false);
            }
            return (true);
        }
        if (po.numpts < 4) {
            delete [] po.points;
            Errs()->add_error("poly has too few points");
            return (false);
        }
        if (pdesc->is_copy()) {
            delete [] pdesc->points();
            pdesc->set_points(po.points);
            pdesc->set_numpts(po.numpts);
            pdesc->computeBB();
        }
        else {
            CDo *newo = sdesc->newPoly(pdesc, &po, pdesc->ldesc(),
                pdesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newPoly failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpMagn) {
        double mag;
        if (sscanf(string, "%lf", &mag) != 1) {
            Errs()->add_error("syntax error, number expected");
            return (false);
        }
        if (mag < CDMAGMIN || mag > CDMAGMAX) {
            Errs()->add_error("error, number %g-%g expected",
                CDMAGMIN, CDMAGMAX);
            return (false);
        }

        // The first vertex remains fixed.
        int num = pdesc->numpts();
        Poly po(num, new Point[num]);
        const Point *pts = pdesc->points();
        int xr = pts[0].x;
        int yr = pts[0].y;
        po.points[0].set(xr, yr);
        for (int i = 1; i < po.numpts; i++) {
            po.points[i].set(xr + mmRnd(mag*(pts[i].x - xr)),
                yr + mmRnd(mag*(pts[i].y - yr)));
        }

        if (pdesc->is_copy()) {
            delete [] pdesc->points();
            pdesc->set_points(po.points);
            pdesc->computeBB();
        }
        else {
            CDo *newo = sdesc->newPoly(pdesc, &po, pdesc->ldesc(),
                pdesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newPoly failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpXY) {
        int n;
        Point *p = getpts(string, &n);
        if (!p)
            return (false);
        int px = p->x;
        int py = p->y;
        delete [] p;
        int num = pdesc->numpts();
        Poly po(num, 0);
        const Point *pts = pdesc->points();
        if (px != pts->x || py != pts->y) {
            po.points = new Point[num];
            int dx = px - pts->x;
            int dy = py - pts->y;
            for (int i = 0; i < po.numpts; i++) {
                po.points[i].x = pts[i].x + dx;
                po.points[i].y = pts[i].y + dy;
            }
            if (pdesc->is_copy()) {
                delete [] pdesc->points();
                pdesc->set_points(po.points);
                pdesc->computeBB();
            }
            else {
                CDo *newo = sdesc->newPoly(pdesc, &po, pdesc->ldesc(),
                    pdesc->prpty_list(), false);
                if (!newo) {
                    Errs()->add_error("newPoly failed");
                    return (false);
                }
            }
        }
        return (true);
    }
    if (val == XprpWidth) {
        int w;
        if (sscanf(string, "%d", &w) != 1 || w <= 0) {
            Errs()->add_error("error, bad width");
            return (false);
        }
        BBox BB(pdesc->oBB());
        BB.right = BB.left + w;
        int num = pdesc->numpts();
        Poly po(num, scalepts(&BB, &pdesc->oBB(), pdesc->points(), num));
        if (pdesc->is_copy()) {
            pdesc->set_oBB(BB);
            delete [] pdesc->points();
            pdesc->set_points(po.points);
        }
        else {
            CDo *newo = sdesc->newPoly(pdesc, &po, pdesc->ldesc(),
                pdesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newPoly failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpHeight) {
        int h;
        if (sscanf(string, "%d", &h) != 1 || h <= 0) {
            Errs()->add_error("error, bad height");
            return (false);
        }
        BBox BB(pdesc->oBB());
        BB.top = BB.bottom + h;
        int num = pdesc->numpts();
        Poly po(num, scalepts(&BB, &pdesc->oBB(), pdesc->points(), num));
        if (pdesc->is_copy()) {
            pdesc->set_oBB(BB);
            delete [] pdesc->points();
            pdesc->set_points(po.points);
        }
        else {
            CDo *newo = sdesc->newPoly(pdesc, &po, pdesc->ldesc(),
                pdesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newPoly failed");
                return (false);
            }
        }
        return (true);
    }
    Errs()->add_error("unknown pseudo-property %d", val);
    return (false);
}


// Handle pseudo-properties applied to wires.
//
bool
cEdit::acceptWirePseudoProp(CDw *wdesc, CDs *sdesc, int val,
    const char *string)
{
    // For wires, electrical wires on the active layer or that
    // would be moved to the active layer are rejected, all other
    // wires are accepted.

    if (sdesc->isElectrical() && wdesc->ldesc()->isWireActive()) {
        Errs()->add_error(
        "Can't modify wires on an active layer with pseudo-properties.");
        return (false);
    }

    if (val == XprpBB) {
        BBox BB;
        if (sscanf(string, "%d,%d %d,%d", &BB.left, &BB.bottom,
                &BB.right, &BB.top) != 4) {
            Errs()->add_error(
                "syntax error, expecting l,b r,t coordinates");
            return (false);
        }
        BB.fix();
        int num = wdesc->numpts();
        Wire wire(num, scalepts(&BB, &wdesc->oBB(), wdesc->points(), num),
            wdesc->attributes());
        if (wdesc->is_copy()) {
            delete [] wdesc->points();
            wdesc->set_points(wire.points);
            wdesc->computeBB();
        }
        else {
            CDo *newo = sdesc->newWire(wdesc, &wire, wdesc->ldesc(),
                wdesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newWire failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpLayer) {
        CDl *ld = CDldb()->findLayer(string, sdesc->displayMode());
        if (!ld) {
            Errs()->add_error("layer %s not found", string);
            return (false);
        }
        if (sdesc->isElectrical() && ld->isWireActive()) {
            Errs()->add_error(
                "Can't move wire to active layer with pseudo-properties");
            return (false);
        }
        if (wdesc->is_copy())
            wdesc->set_ldesc(ld);
        else {
            int num = wdesc->numpts();
            Wire wire(num, Point::dup(wdesc->points(), num),
                wdesc->attributes());
            CDo *newo = sdesc->newWire(wdesc, &wire, ld,
                wdesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newWire failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpCoords) {
        Wire wire(0, 0, wdesc->attributes());
        wire.points = getpts(string, &wire.numpts);
        if (!wire.points)
            return (false);
        if (wdesc->is_copy()) {
            delete [] wdesc->points();
            wdesc->set_points(wire.points);
            wdesc->set_numpts(wire.numpts);
            wdesc->computeBB();
        }
        else {
            CDo *newo = sdesc->newWire(wdesc, &wire, wdesc->ldesc(),
                wdesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newWire failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpWwidth) {
        int d;
        if (sscanf(string, "%d", &d) != 1 || d <= 0) {
            Errs()->add_error("error, positive integer expected");
            return (false);
        }
        if (wdesc->is_copy()) {
            wdesc->set_wire_width(d);
            wdesc->computeBB();
        }
        else {
            int num = wdesc->numpts();
            Wire wire(num, Point::dup(wdesc->points(), num),
                wdesc->attributes());
            wire.set_wire_width(d);
            if (wire.wire_width() != wdesc->wire_width()) {
                CDo *newo = sdesc->newWire(wdesc, &wire, wdesc->ldesc(),
                    wdesc->prpty_list(), false);
                if (!newo) {
                    Errs()->add_error("newWire failed");
                    return (false);
                }
            }
        }
        return (true);
    }
    if (val == XprpWstyle) {
        const char *s = string;
        while (isspace(*s))
            s++;
        int d = -1;
        switch (*s) {
        case '0':
        case 'f':
        case 'F':
            d = CDWIRE_FLUSH;
            break;
        case '1':
        case 'r':
        case 'R':
            d = CDWIRE_ROUND;
            break;
        case '2':
        case 'e':
        case 'E':
            d = CDWIRE_EXTEND;
            break;
        default:
            break;
        }
        if (d < 0) {
            Errs()->add_error("error, integer 0-2 or 'f', 'r', 'e' expected");
            return (false);
        }
        if (wdesc->is_copy()) {
            wdesc->set_wire_style((WireStyle)d);
            wdesc->computeBB();
        }
        else {
            int num = wdesc->numpts();
            Wire wire(num, Point::dup(wdesc->points(), num),
                wdesc->attributes());
            wire.set_wire_style((WireStyle)d);
            if (wire.wire_style() != wdesc->wire_style()) {
                CDo *newo = sdesc->newWire(wdesc, &wire, wdesc->ldesc(),
                    wdesc->prpty_list(), false);
                if (!newo) {
                    Errs()->add_error("newWire failed");
                    return (false);
                }
            }
        }
        return (true);
    }
    if (val == XprpMagn) {
        double mag;
        if (sscanf(string, "%lf", &mag) != 1) {
            Errs()->add_error("syntax error, number expected");
            return (false);
        }
        if (mag < CDMAGMIN || mag > CDMAGMAX) {
            Errs()->add_error("error, number %g-%g expected",
                CDMAGMIN, CDMAGMAX);
            return (false);
        }

        // The first vertex remains fixed.
        int num = wdesc->numpts();
        Wire wire(num, 0, wdesc->attributes());
        const Point *pts = wdesc->points();
        wire.points = new Point[wire.numpts];
        int xr = pts[0].x;
        int yr = pts[0].y;
        wire.points[0].set(xr, yr);
        for (int i = 1; i < wire.numpts; i++) {
            wire.points[i].set(xr + mmRnd(mag*(pts[i].x - xr)),
                yr + mmRnd(mag*(pts[i].y - yr)));
        }

        // Note that the Magn does not change wire width, only the
        // point list.  Use XprpWwidth.

        if (wdesc->is_copy()) {
            delete [] wdesc->points();
            wdesc->set_points(wire.points);
            wdesc->computeBB();
        }
        else {
            CDo *newo = sdesc->newWire(wdesc, &wire, wdesc->ldesc(),
                wdesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newWire failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpXY) {
        int n;
        Point *p = getpts(string, &n);
        if (!p)
            return (false);
        int px = p->x;
        int py = p->y;
        delete [] p;
        int num = wdesc->numpts();
        Wire wire(num, 0, wdesc->attributes());
        const Point *pts = wdesc->points();
        if (px != pts->x || py != pts->y) {
            wire.points = new Point[wire.numpts];
            int dx = px - pts->x;
            int dy = py - pts->y;
            for (int i = 0; i < wire.numpts; i++) {
                wire.points[i].x = pts[i].x + dx;
                wire.points[i].y = pts[i].y + dy;
            }
            if (wdesc->is_copy()) {
                delete [] wdesc->points();
                wdesc->set_points(wire.points);
                wdesc->computeBB();
            }
            else {
                CDo *newo = sdesc->newWire(wdesc, &wire, wdesc->ldesc(),
                    wdesc->prpty_list(), false);
                if (!newo) {
                    Errs()->add_error("newWire failed");
                    return (false);
                }
            }
        }
        return (true);
    }
    if (val == XprpWidth) {
        int w;
        if (sscanf(string, "%d", &w) != 1 || w <= 0) {
            Errs()->add_error("error, bad width");
            return (false);
        }
        BBox BB(wdesc->oBB());
        BB.right = BB.left + w;
        int num = wdesc->numpts();
        Wire wire(num, scalepts(&BB, &wdesc->oBB(), wdesc->points(), num),
            wdesc->attributes());
        if (wdesc->is_copy()) {
            delete [] wdesc->points();
            wdesc->set_points(wire.points);
            wdesc->computeBB();
        }
        else {
            CDo *newo = sdesc->newWire(wdesc, &wire, wdesc->ldesc(),
                wdesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newWire failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpHeight) {
        int h;
        if (sscanf(string, "%d", &h) != 1 || h <= 0) {
            Errs()->add_error("error, bad height");
            return (false);
        }
        BBox BB(wdesc->oBB());
        BB.top = BB.bottom + h;
        int num = wdesc->numpts();
        Wire wire(num, scalepts(&BB, &wdesc->oBB(), wdesc->points(), num),
            wdesc->attributes());
        if (wdesc->is_copy()) {
            delete [] wdesc->points();
            wdesc->set_points(wire.points);
            wdesc->computeBB();
        }
        else {
            CDo *newo = sdesc->newWire(wdesc, &wire, wdesc->ldesc(),
                wdesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newWire failed");
                return (false);
            }
        }
        return (true);
    }
    Errs()->add_error("unknown pseudo-property %d", val);
    return (false);
}


// Handle pseudo-properties applied to labels.
//
bool
cEdit::acceptLabelPseudoProp(CDla *ladesc, CDs *sdesc, int val,
    const char *string)
{
    // For labels, accept all (physical or electrical, copy or not).  
    // Have to be careful with electrical property labels.

    if (val == XprpBB) {
        BBox BB;
        if (sscanf(string, "%d,%d %d,%d", &BB.left, &BB.bottom,
                &BB.right, &BB.top) != 4) {
            Errs()->add_error(
                "syntax error, expecting l,b r,t coordinates");
            return (false);
        }
        BB.fix();
        if (ladesc->is_copy()) {
            ladesc->set_xpos(BB.left);
            ladesc->set_ypos(BB.bottom);
            ladesc->set_width(BB.width());
            ladesc->set_height(BB.height());
            ladesc->computeBB();
        }
        else {
            Label lab(ladesc->la_label());
            lab.x = BB.left;
            lab.y = BB.bottom;
            lab.width = BB.width();
            lab.height = BB.height();
            CDla *newo = sdesc->newLabel(ladesc, &lab, ladesc->ldesc(),
                ladesc->prpty_list(), true);
            if (!newo) {
                Errs()->add_error("newLabel failed");
                return (false);
            }
            if (sdesc->isElectrical())
                sdesc->prptyLabelUpdate(newo, ladesc);
        }
        return (true);
    }
    if (val == XprpLayer) {
        CDl *ld = CDldb()->findLayer(string, sdesc->displayMode());
        if (!ld) {
            Errs()->add_error("layer %s not found", string);
            return (false);
        }
        if (ladesc->is_copy())
            ladesc->set_ldesc(ld);
        else {
            Label ltmp(ladesc->la_label());
            CDla *newo = sdesc->newLabel(ladesc, &ltmp, ld,
                ladesc->prpty_list(), true);
            if (!newo) {
                Errs()->add_error("newLabel failed");
                return (false);
            }
            if (sdesc->isElectrical())
                sdesc->prptyLabelUpdate(newo, ladesc);
        }
        return (true);
    }
    if (val == XprpCoords) {
        int numpts;
        Point *p = getpts(string, &numpts);
        if (!p)
            return (false);
        Poly ptmp(numpts, p);
        if (ptmp.is_rect()) {
            BBox BB(p);
            delete [] p;
            if (ladesc->is_copy()) {
                ladesc->set_xpos(BB.left);
                ladesc->set_ypos(BB.bottom);
                ladesc->set_width(BB.width());
                ladesc->set_height(BB.height());
                ladesc->computeBB();
            }
            else {
                Label lab(ladesc->la_label());
                lab.x = BB.left;
                lab.y = BB.bottom;
                lab.width = BB.width();
                lab.height = BB.height();
                CDla *newo = sdesc->newLabel(ladesc, &lab, ladesc->ldesc(),
                    ladesc->prpty_list(), true);
                if (!newo) {
                    Errs()->add_error("newLabel failed");
                    return (false);
                }
                if (sdesc->isElectrical())
                    sdesc->prptyLabelUpdate(newo, ladesc);
            }
            return (true);
        }
        Errs()->add_error(
            "syntax error, expecting rectangular closed path");
        delete [] p;
        return (false);
    }
    if (val == XprpText) {
        if (!string || !*string) {
            Errs()->add_error("empty string not allowed in label");
            return (false);
        }
        hyList *hl = new hyList(sdesc, string, HYcvAscii);
        if (!hl) {
            Errs()->add_error("internal error: null hyperlist");
            return (false);
        }
        if (ladesc->is_copy()) {
            char *oldstr = hyList::string(ladesc->label(), HYcvPlain, false);
            if (!oldstr)
                oldstr = lstring::copy("X");
            double oldwidth, oldheight;
            int oldlines = CD()->DefaultLabelSize(oldstr, Physical,
                &oldwidth, &oldheight);
            int oldlineht = ladesc->height()/oldlines;
            delete [] oldstr;

            double newwidth, newheight;
            int newlines = CD()->DefaultLabelSize(string, Physical,
                &newwidth, &newheight);
            int newlineht = (int)(newheight/newlines);
         
            hyList::destroy(ladesc->label());
            ladesc->set_label(hl);

            ladesc->set_height(oldlineht * newlines);
            ladesc->set_width((int)((newwidth * oldlineht)/newlineht));
            ladesc->computeBB();
        }
        else {
            // This will handle electrical property changes.
            CDo *newo = changeLabel(ladesc, sdesc, hl);
            hyList::destroy(hl);
            if (!newo) {
                Errs()->add_error("changeLabel failed");
                return (false);
            }
        }
        return (true);
    }
    if (val == XprpXform) {
        // format:  [+|-]  0xhex | tok,tok,...
        bool had_p = false, had_m = false;
        const char *s = string;
        while (isspace(*s))
            s++;
        if (*s == '+') {
            had_p = true;
            s++;
        }
        else if (*s == '-') {
            had_m = true;
            s++;
        }
        unsigned int xf = string_to_xform(s);

        if (ladesc->is_copy()) {
            if (had_p)
                ladesc->set_xform(ladesc->xform() | xf);
            else if (had_m)
                ladesc->set_xform(ladesc->xform() & ~xf);
            else
                ladesc->set_xform(xf);
            ladesc->computeBB();
        }
        else {
            Label lab(ladesc->la_label());
            if (had_p)
                lab.xform |= xf;
            else if (had_m)
                lab.xform &= ~xf;
            else
                lab.xform = xf;
            CDla *newo = sdesc->newLabel(ladesc, &lab, ladesc->ldesc(),
                ladesc->prpty_list(), true);
            if (!newo) {
                Errs()->add_error("newLabel failed");
                return (false);
            }
            if (sdesc->isElectrical())
                sdesc->prptyLabelUpdate(newo, ladesc);
        }
        return (true);
    }
    if (val == XprpMagn) {
        double mag;
        if (sscanf(string, "%lf", &mag) != 1) {
            Errs()->add_error("syntax error, number expected");
            return (false);
        }
        if (mag < CDMAGMIN || mag > CDMAGMAX) {
            Errs()->add_error("error, number %g-%g expected",
                CDMAGMIN, CDMAGMAX);
            return (false);
        }
        if (ladesc->is_copy()) {
            ladesc->set_width(mmRnd(mag*ladesc->width()));
            ladesc->set_height(mmRnd(mag*ladesc->height()));
            ladesc->computeBB();
        }
        else {
            Label lab(ladesc->la_label());
            lab.width = mmRnd(mag*ladesc->width());
            lab.height = mmRnd(mag*ladesc->height());
            CDla *newo = sdesc->newLabel(ladesc, &lab, ladesc->ldesc(),
                ladesc->prpty_list(), true);
            if (!newo) {
                Errs()->add_error("newLabel failed");
                return (false);
            }
            if (sdesc->isElectrical())
                sdesc->prptyLabelUpdate(newo, ladesc);
        }
        return (true);
    }
    if (val == XprpXY) {
        int n;
        Point *p = getpts(string, &n);
        if (!p)
            return (false);
        int px = p->x;
        int py = p->y;
        delete [] p;
        if (px != ladesc->xpos() || py != ladesc->ypos()) {
            if (ladesc->is_copy()) {
                ladesc->set_xpos(px);
                ladesc->set_ypos(py);
                ladesc->computeBB();
            }
            else {
                Label lab(ladesc->la_label());
                lab.x = px;
                lab.y = py;
                CDla *newo = sdesc->newLabel(ladesc, &lab, ladesc->ldesc(),
                    ladesc->prpty_list(), true);
                if (!newo) {
                    Errs()->add_error("newLabel failed");
                    return (false);
                }
                if (sdesc->isElectrical())
                    sdesc->prptyLabelUpdate(newo, ladesc);
            }
        }
        return (true);
    }
    if (val == XprpWidth) {
        int w;
        if (sscanf(string, "%d", &w) != 1 || w <= 0) {
            Errs()->add_error("error, bad width");
            return (false);
        }
        if (ladesc->is_copy()) {
            ladesc->set_width(w);
            ladesc->computeBB();
        }
        else {
            Label lab(ladesc->la_label());
            lab.width = w;
            CDla *newo = sdesc->newLabel(ladesc, &lab, ladesc->ldesc(),
                ladesc->prpty_list(), true);
            if (!newo) {
                Errs()->add_error("newLabel failed");
                return (false);
            }
            if (sdesc->isElectrical())
                sdesc->prptyLabelUpdate(newo, ladesc);
        }
        return (true);
    }
    if (val == XprpHeight) {
        int h;
        if (sscanf(string, "%d", &h) != 1 || h <= 0) {
            Errs()->add_error("error, bad height");
            return (false);
        }
        if (ladesc->is_copy()) {
            ladesc->set_height(h);
            ladesc->computeBB();
        }
        else {
            Label lab(ladesc->la_label());
            lab.height = h;
            CDla *newo = sdesc->newLabel(ladesc, &lab, ladesc->ldesc(),
                ladesc->prpty_list(), true);
            if (!newo) {
                Errs()->add_error("newLabel failed");
                return (false);
            }
            if (sdesc->isElectrical())
                sdesc->prptyLabelUpdate(newo, ladesc);
        }
        return (true);
    }
    Errs()->add_error("unknown pseudo-property %d", val);
    return (false);
}


// Handle pseudo-properties applied to instances.
//
bool
cEdit::acceptInstPseudoProp(CDc *cdesc, CDs *sdesc, int val,
    const char *string)
{
    // For instances, don't accept electrical mode, copies are ok.
    if (sdesc->isElectrical()) {
        Errs()->add_error(
            "Can't change electrical instance with pseudo-properties.");
        return (false);
    }

    if (val == XprpArray) {
        int nx, ny, dx, dy;
        if (sscanf(string, "%d,%d %d,%d", &nx, &ny, &dx, &dy) != 4) {
            Errs()->add_error(
                "syntax error, expecting nx,dx ny,dy values");
            return (false);
        }
        if (nx < 1 || ny < 1) {
            Errs()->add_error("error, nx or ny less than 1");
            return (false);
        }
        if (cdesc->is_copy()) {
            cTfmStack stk;
            stk.TPush();
            stk.TApplyTransform(cdesc);
            CDtx tx;
            stk.TCurrent(&tx);
            stk.TPop();
            CDap ap(nx, ny, dx, dy);
            CDattr at(&tx, &ap);
            cdesc->setAttr(CD()->RecordAttr(&at));
        }
        else {
            CallDesc calldesc;
            cdesc->call(&calldesc);
            cTfmStack stk;
            stk.TPush();
            stk.TApplyTransform(cdesc);
            CDtx tx;
            stk.TCurrent(&tx);
            stk.TPop();
            CDap ap(nx, ny, dx, dy);
            CDc *newodesc;
            if (OIfailed(sdesc->makeCall(&calldesc, &tx, &ap, CDcallNone,
                    &newodesc))) {
                Errs()->add_error("makeCall failed");
                return (false);
            }
            newodesc->prptyAddCopyList(cdesc->prpty_list());
            Ulist()->RecordObjectChange(sdesc, cdesc, newodesc);
        }
        return (true);
    }
    if (val == XprpTransf) {
        cTfmStack stk;
        stk.TPush();
        for (const char *s = string; *s; s++) {
            if (isspace(*s))
                continue;
            if (*s == 'T') {
                s++;
                char *tok = lstring::gettok(&s);
                if (!tok) {
                    stk.TPop();
                    Errs()->add_error(
                        "syntax error, expecting translation x");
                    return (false);
                }
                int x = atoi(tok);
                delete [] tok;
                tok = lstring::gettok(&s);
                if (!tok) {
                    stk.TPop();
                    Errs()->add_error(
                        "syntax error, expecting translation y");
                    return (false);
                }
                int y = atoi(tok);
                delete [] tok;
                stk.TTranslate(x, y);
                s--;
            }
            else if (*s == 'R') {
                s++;
                char *tok = lstring::gettok(&s);
                if (!tok) {
                    stk.TPop();
                    Errs()->add_error(
                        "syntax error, expecting rotation angle");
                    return (false);
                }
                s--;
                int r = atoi(tok);
                delete [] tok;
                int x, y;
                if (r == 0)
                    x = 1,  y = 0;
                else if (r == 45)
                    x = 1, y = 1;
                else if (r == 90)
                    x = 0, y = 1;
                else if (r == 135)
                    x = -1, y = 1;
                else if (r == 180)
                    x = -1, y = 0;
                else if (r == 225)
                    x = -1, y = -1;
                else if (r == 270)
                    x = 0, y = -1;
                else if (r == 315)
                    x = 1, y = -1;
                else {
                    stk.TPop();
                    Errs()->add_error("error, bad angle");
                    return (false);
                }
                stk.TRotate(x, y);
            }
            else if (*s == 'M') {
                s++;
                if (*s == 'X')
                    stk.TMX();
                else if (*s == 'Y')
                    stk.TMY();
                else {
                    stk.TPop();
                    Errs()->add_error(
                        "syntax error, expecting MX or MY");
                    return (false);
                }
            }
            else {
                stk.TPop();
                Errs()->add_error(
                    "syntax error, unknown transformation");
                return (false);
            }
        }
        CDtx tx;
        stk.TCurrent(&tx);
        stk.TPop();

        if (cdesc->is_copy()) {
            CDap ap(cdesc);
            CDattr at(&tx, &ap);
            cdesc->setPosX(tx.tx);
            cdesc->setPosY(tx.ty);
            cdesc->setAttr(CD()->RecordAttr(&at));
        }
        else {
            CallDesc calldesc;
            cdesc->call(&calldesc);
            CDap ap(cdesc);
            CDc *newodesc;
            if (OIfailed(sdesc->makeCall(&calldesc, &tx, &ap, CDcallNone,
                    &newodesc))) {
                Errs()->add_error("makeCall failed");
                return (false);
            }
            newodesc->prptyAddCopyList(cdesc->prpty_list());
            Ulist()->RecordObjectChange(sdesc, cdesc, newodesc);
        }
        return (true);
    }
    if (val == XprpMagn) {
        double mag;
        if (sscanf(string, "%lf", &mag) != 1) {
            Errs()->add_error("syntax error, number expected");
            return (false);
        }
        if (mag < CDMAGMIN || mag > CDMAGMAX) {
            Errs()->add_error("error, number %g-%g expected",
                CDMAGMIN, CDMAGMAX);
            return (false);
        }
        if (cdesc->is_copy()) {
            cTfmStack stk;
            stk.TPush();
            stk.TApplyTransform(cdesc);
            CDtx tx;
            stk.TCurrent(&tx);
            stk.TPop();
            CDap ap(cdesc);
            tx.magn = mag;
            CDattr at(&tx, &ap);
            cdesc->setAttr(CD()->RecordAttr(&at));
        }
        else {
            CallDesc calldesc;
            cdesc->call(&calldesc);
            cTfmStack stk;
            stk.TPush();
            stk.TApplyTransform(cdesc);
            CDtx tx;
            stk.TCurrent(&tx);
            stk.TPop();
            CDap ap(cdesc);
            tx.magn = mag;
            CDc *newodesc;
            if (OIfailed(sdesc->makeCall(&calldesc, &tx, &ap, CDcallNone,
                    &newodesc))) {
                Errs()->add_error("makeCall failed");
                return (false);
            }
            newodesc->prptyAddCopyList(cdesc->prpty_list());
            Ulist()->RecordObjectChange(sdesc, cdesc, newodesc);
        }
        return (true);
    }
    if (val == XprpName) {
        if (cdesc->is_copy()) {
            Errs()->add_error("Can't re-master an instance copy.");
            return (false);
        }
        else {
            if (sdesc->cellname() == DSP()->CurCellName()) {
                CDcbin cbin;
                if (!openCell(string, &cbin, 0)) {
                    Errs()->add_error("OpenSymbol failed");
                    return (false);
                }
                if (!replaceInstance(cdesc, &cbin, true, false)) {
                    Errs()->add_error("ReplaceCell failed");
                    return (false);
                }
                return (true);
            }
            Errs()->add_error("instance parent not current cell");
        }
        return (false);
    }
    if (val == XprpXY) {
        int n;
        Point *p = getpts(string, &n);
        if (!p)
            return (false);
        int px = p->x;
        int py = p->y;
        delete [] p;
        if (px != cdesc->posX() || py != cdesc->posY()) {
            if (cdesc->is_copy()) {
                cTfmStack stk;
                stk.TPush();
                stk.TApplyTransform(cdesc);
                CDtx tx;
                stk.TCurrent(&tx);
                stk.TPop();
                CDap ap(cdesc);
                tx.tx = px;
                tx.ty = py;
                CDattr at(&tx, &ap);
                cdesc->setPosX(tx.tx);
                cdesc->setPosY(tx.ty);
                cdesc->setAttr(CD()->RecordAttr(&at));
            }
            else {
                CallDesc calldesc;
                cdesc->call(&calldesc);
                cTfmStack stk;
                stk.TPush();
                stk.TApplyTransform(cdesc);
                CDtx tx;
                stk.TCurrent(&tx);
                stk.TPop();
                CDap ap(cdesc);
                tx.tx = px;
                tx.ty = py;
                CDc *newodesc;
                if (OIfailed(sdesc->makeCall(&calldesc, &tx, &ap,
                        CDcallNone, &newodesc))) {
                    Errs()->add_error("makeCall failed");
                    return (false);
                }
                newodesc->prptyAddCopyList(cdesc->prpty_list());
                Ulist()->RecordObjectChange(sdesc, cdesc, newodesc);
            }
        }
        return (true);
    }
    Errs()->add_error("unknown pseudo-property %d", val);
    return (false);
}


namespace {
    // Scale points according to new BB
    //
    Point *
    scalepts(const BBox *BB, const BBox *oBB, const Point *pts, int numpts)
    {
        Point *npts = new Point[numpts];
        double rx = BB->width()/(double)oBB->width();
        double ry = BB->height()/(double)oBB->height();
        for (int i = 0; i < numpts; i++) {
            npts[i].x = mmRnd((pts[i].x - oBB->left)*rx + BB->left);
            npts[i].y = mmRnd((pts[i].y - oBB->bottom)*ry + BB->bottom);
        }
        return (npts);
    }


    // Parse a list of "%d,%d" coordinates
    //
    Point *
    getpts(const char *string, int *num)
    {
        const char *s = string;
        int cnt = 0;
        while (*s) {
            lstring::advtok(&s);
            cnt++;
        }
        if (!cnt) {
            Errs()->add_error("no coords found!");
            return (0);
        }
        Point *pts = new Point[cnt];
        cnt = 0;
        while ((s = lstring::gettok(&string)) != 0) {
            if (sscanf(s, "%d,%d", &pts[cnt].x, &pts[cnt].y) != 2) {
                delete [] s;
                delete [] pts;
                Errs()->add_error("parse error reading coords!");
                return (0);
            }
            delete [] s;
            cnt++;
        }
        *num = cnt;
        return (pts);
    }
}

