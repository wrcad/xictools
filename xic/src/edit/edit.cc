
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
#include "fio.h"
#include "edit.h"
#include "edit_menu.h"
#include "extif.h"
#include "undolist.h"
#include "dsp_inlines.h"
#include "ghost.h"
#include "tech.h"
#include "promptline.h"
#include "pushpop.h"
#include "menu.h"
#include "edit_menu.h"
#include "events.h"
#include "grip.h"
#include "errorlog.h"


namespace {
    // This is called when the standard via table is created, on
    // addition of the first standard via.  This will un-gray the menu
    // entry.
    //
    void via_notify(bool b)
    {
        if (DSP()->CurMode() == Physical) {
            MenuBox *mb = MainMenu()->FindMainMenu(MMedit);
            if (mb)
                MainMenu()->SetSensitive(mb->menu[editMenuCrvia].cmd.caller, b);
        }
    }
}

cEdit::cEdit()
{
    ed_object_list          = 0;
    ed_press_layer          = 0;
    ed_popup                = 0;
    ed_menu_head            = 0;
    ed_push_data            = 0;
    ed_pcsuper              = 0;
    ed_pcparams             = 0;
    ed_cur_grip1            = 0;
    ed_cur_grip2            = 0;
    ed_gripdb               = 0;
    ed_wire_style           = CDWIRE_EXTEND;
    ed_move_or_copy         = CDmove;
    ed_lchange_mode         = LCHGnone;
    ed_label_x              = 0;
    ed_label_y              = 0;
    ed_label_width          = 0;
    ed_label_height         = 0;
    ed_label_xform          = 0;
    ed_place_ref            = PL_ORIGIN;
    ed_stretch_box_code     = 0;
    ed_h_justify            = 0;
    ed_v_justify            = 0;
    ed_menu_len             = 0;
    ed_auto_abut_mode       = AbutMode1;
    ed_hide_grips           = false;
    ed_replacing            = false;
    ed_use_array            = false;
    ed_no_wire_width_mag    = false;

    for (int i = 0; i < ED_YANK_DEPTH; i++)
        ed_yank_buffer[i] = 0;
    for (int i = 0; i < ED_LEXPR_STORES; i++)
        ed_lexpr_stores[i] = 0;

    // Instantiate ghosting.
    new cEditGhost;

    // Instantiate undo processing.
    new cUndoList;

    setupBangCmds();
    setupVariables();
    initMainState();
    loadScriptFuncs();

    Tech()->RegisterHasStdVia(&via_notify);
}


#ifndef WITH_GRFXTK

// Stubs for functions defined in graphical toolkit, include when
// not building with toolkit.
void cEdit::PopUpEditSetup( GRobject, ShowMode) { }
void cEdit::PopUpFlatten(   GRobject, ShowMode,
    bool (*)(const char*, bool, const char*, void*), void*, int,
    bool, bool) { }
void cEdit::PopUpJoin(      GRobject, ShowMode) { }
void cEdit::PopUpLayerExp(  GRobject, ShowMode) { }
void cEdit::PopUpLogo(      GRobject, ShowMode) { }
void cEdit::PopUpPolytextFont(GRobject, ShowMode) { }
void cEdit::polytextExtent(const char*, int*, int*, int*) { }
PolyList *cEdit::polytext(const char*, int, int, int) { return (0); }
void cEdit::PopUpLayerChangeMode(ShowMode) { }
PMretType cEdit::PopUpModified(stringlist*, bool(*)(const char*))
    { return (PMok); }
void cEdit::PopUpPCellCtrl( GRobject, ShowMode) { }
bool cEdit::PopUpPCellParams(GRobject, ShowMode, PCellParam*, const char*,
    pcpMode) { return (false); }
void cEdit::PopUpPlace(ShowMode, bool) { }
void cEdit::PopUpCellProperties(ShowMode) { }
void cEdit::PopUpProperties(CDo*, ShowMode, PRPmode) { }
PrptyText *cEdit::PropertyResolve(int, int, CDo**) { return (0); }
void cEdit::PropertyPurge(CDo*, CDo*) { }
PrptyText *cEdit::PropertySelect(int) { return (0); }
PrptyText *cEdit::PropertyCycle(CDp*, bool(*)(const CDp*), bool)
    { return (0); }
void cEdit::RegisterPrptyBtnCallback(int(*)(PrptyText*)) { }
void cEdit::PopUpPropertyInfo(CDo*, ShowMode) { }
void cEdit::PropertyInfoPurge(CDo*, CDo*) { }
void cEdit::PopUpStdVia(    GRobject, ShowMode, CDc*) { }
void cEdit::PopUpTransform( GRobject, ShowMode,
    bool (*)(const char*, bool, const char*, void*), void*) { }

#endif


// Set the editing capability on/off in accord with the immutable flag
// of the current cell.  If there is no current cell, the passed
// argument state is used.
//
void
cEdit::setEditingMode(bool edit)
{
    CDs *sd = CurCell(DSP()->CurMode());
    if (sd)
        edit = !sd->isImmutable();

    if (DSP()->MainWdesc()->DbType() != WDcddb)
        return;

    if (edit) {
        MainMenu()->HideButtonMenu(false);

        MainMenu()->DisableMainMenuItem(MMedit, MenuFLATN, false);
        MainMenu()->DisableMainMenuItem(MMedit, MenuJOIN, false);
        MainMenu()->DisableMainMenuItem(MMedit, MenuLEXPR, false);
        MainMenu()->DisableMainMenuItem(MMedit, MenuPRPTY, false);
        MainMenu()->DisableMainMenuItem(MMedit, MenuCPROP, false);

        MainMenu()->DisableMainMenuItem(MMmod, 0, false);

        MainMenu()->MenuButtonSet(MMedit, MenuCEDIT, true);
    }
    else {
        EV()->InitCallback();
        MainMenu()->HideButtonMenu(true);

        EditIf()->ulListFinalize(false);

        MainMenu()->DisableMainMenuItem(MMedit, MenuFLATN, true);
        MainMenu()->DisableMainMenuItem(MMedit, MenuJOIN, true);
        MainMenu()->DisableMainMenuItem(MMedit, MenuLEXPR, true);
        MainMenu()->DisableMainMenuItem(MMedit, MenuPRPTY, true);
        MainMenu()->DisableMainMenuItem(MMedit, MenuCPROP, true);

        MainMenu()->DisableMainMenuItem(MMmod, 0, true);

        MainMenu()->MenuButtonSet(MMedit, MenuCEDIT, false);
    }
    XM()->PopUpCells(0, MODE_UPD);
}


// This should be called on entering editing commands, and the command
// should abort on a true return.  Program modes that prohibit editing
// should be checked here.
//
bool
cEdit::noEditing()
{
    if (DSP()->CurMode() == Physical && ExtIf()->isExtractionView()) {
        PL()->ShowPrompt("No editing in Extraction View!");
        return (true);
    }
    return (false);
}


//-----------------------------------------------------------------------------
// Misc wrappers

void
cEdit::invalidateLayer(CDs *sd, CDl *ld)
{
    Ulist()->InvalidateLayer(sd, ld);
    PP()->InvalidateLayer(sd, ld);

    sModeSave::EditState *es = XM()->hist().editState();
    if (es) {
        if (es->PhysUL) {
            Oper::purge(es->PhysUL->operations, sd, ld);
            Oper::purge(es->PhysUL->redo_list, sd, ld);
        }
        if (es->ElecUL) {
            Oper::purge(es->ElecUL->operations, sd, ld);
            Oper::purge(es->ElecUL->redo_list, sd, ld);
        }
        if (es->PhysCX) {
            ContextDesc::purge(es->PhysCX->context(), sd, ld);
            ContextDesc::purge(es->PhysCX->context_history(), sd, ld);
        }
        if (es->ElecCX) {
            ContextDesc::purge(es->ElecCX->context(), sd, ld);
            ContextDesc::purge(es->ElecCX->context_history(), sd, ld);
        }
    }
}


void
cEdit::invalidateObject(CDs *sd, CDo *od, bool save)
{
    Ulist()->InvalidateObject(sd, od, save);
    PP()->InvalidateObject(sd, od, save);

    if (!sd)
        return;
    sModeSave::EditState *es = XM()->hist().editState();
    if (es) {
        if (es->PhysUL) {
            Oper::purge(es->PhysUL->operations, sd, od);
            Oper::purge(es->PhysUL->redo_list, sd, od);
        }
        if (es->ElecUL) {
            Oper::purge(es->ElecUL->operations, sd, od);
            Oper::purge(es->ElecUL->redo_list, sd, od);
        }
        if (es->PhysCX) {
            es->PhysCX->set_context(
                ContextDesc::purge(es->PhysCX->context(), sd, od));
            es->PhysCX->set_context_history(
                ContextDesc::purge(es->PhysCX->context_history(), sd, od));
        }
        if (es->ElecCX) {
            es->ElecCX->set_context(
                ContextDesc::purge(es->ElecCX->context(), sd, od));
            es->ElecCX->set_context_history(
                ContextDesc::purge(es->ElecCX->context_history(), sd, od));
        }
    }
}


void
cEdit::clearSaveState()
{
    delete XM()->hist().editState();
    XM()->hist().setEditState(0);
}


void
cEdit::popState(DisplayMode mode)
{
    sModeSave::EditState *es = XM()->hist().editState();
    if (!es) {
        es = new sModeSave::EditState;
        XM()->hist().setEditState(es);
    }

    if (mode == Physical) {
        es->PhysCX = PP()->PopState();
        es->PhysUL = Ulist()->PopState();
    }
    else {
        es->ElecCX = PP()->PopState();
        es->ElecUL = Ulist()->PopState();
    }
}


void
cEdit::pushState(DisplayMode mode)
{
    sModeSave::EditState *es = XM()->hist().editState();
    if (es) {
        if (mode == Physical) {
            PP()->PushState(es->PhysCX);
            es->PhysCX = 0;
            Ulist()->PushState(es->PhysUL);
            es->PhysUL = 0;
        }
        else {
            PP()->PushState(es->ElecCX);
            es->ElecCX = 0;
            Ulist()->PushState(es->ElecUL);
            es->ElecUL = 0;
        }
    }
    else {
        if (mode == Physical) {
            PP()->PushState(0);
            Ulist()->PushState(0);
        }
        else {
            PP()->PushState(0);
            Ulist()->PushState(0);
        }
    }
}


//-----------------------------------------------------------------------------
// UndoList interface

void
cEdit::ulUndoOperation()
{
    Ulist()->UndoOperation();
}


void
cEdit::ulRedoOperation()
{
    Ulist()->RedoOperation();
}


bool
cEdit::ulHasRedo()
{
    return (Ulist()->HasRedo());
}


bool
cEdit::ulHasChanged()
{
    return (Ulist()->HasChanged());
}


bool
cEdit::ulCommitChanges(bool redisplay, bool nodrc)
{
    return (Ulist()->CommitChanges(redisplay, nodrc));
}


void
cEdit::ulListFinalize(bool keep_undo)
{
    Ulist()->ListFinalize(keep_undo);
}


void
cEdit::ulListBegin(bool noreinit, bool nocheck)
{
    Ulist()->ListBegin(noreinit, nocheck);
}


void
cEdit::ulListCheck(const char *cmd, CDs *sdesc, bool selected)
{
    Ulist()->ListCheck(cmd, sdesc, selected);
}


op_change_t *
cEdit::ulFindNext(const op_change_t *c)
{
    const Ochg *o = static_cast<const Ochg*>(c);
    return (o ? o->next_chg() : 0);
}


// Set up undo list state for pcell evaluation.
//
void
cEdit::ulPCevalSet(CDs *sdesc, ulPCstate *pcstate)
{
    *pcstate = (ulPCstate)Ulist()->RotateUndo(0);
    Ulist()->ListCheckPush("pceval", sdesc, false, true);
}


// Revert undo state after pcell evaluation.
//
void
cEdit::ulPCevalReset(ulPCstate *pcstate)
{
    Ulist()->ListPop();

    Oper *cur = Ulist()->RotateUndo(0);
    Oper::destroy(cur);
    Ulist()->RotateUndo((Oper*)*pcstate);
    Ulist()->RotateUndo((Oper*)*pcstate);
}


// When the current cell is a pcell sub-master and we apply a new
// parameter set as a property change, the undo list is freed when the
// master is cleared.  Thus, any previous property application will be
// lost from the undo list.  Here we provide some hackery around this. 
// These calls should surround the sub-master update operation.

// Call this before a sub-master reparameterization when the
// sub-master is the curremt cell.  This extracts and returns the cell
// property changes associated with the current cell, and frees other
// operations.  The operations list will be cleared anyway when the
// sub-master is cleared before evaluation.
//
void
cEdit::ulPCreparamSet(ulPCstate *pcstate)
{
    *pcstate = Ulist()->GetPcPrmChanges();
}


// Call this after the pcell sub-master current cell has been
// reparameterized.  This puts the reparameterization operations back
// on the operations list, allowing undo/redo of these operations.
//
void
cEdit::ulPCreparamReset(ulPCstate *pcstate)
{
    Ulist()->ResetPcPrmChanges(pcstate);
}


//-----------------------------------------------------------------------------
// Context Push/Pop interface

namespace {
    // State storage for context push/pop.
    struct PPstate : public pp_state_t
    {
        PPstate()
            {
                ppUndoList = Ulist()->RotateUndo(0);
                ppRedoList = Ulist()->RotateRedo(0);
            }

        ~PPstate()
            {
                Oper::destroy(ppUndoList);
                Oper::destroy(ppRedoList);
            }

        void purge(const CDs *sd, const CDl *ld)
            {
                Oper::purge(ppUndoList, sd, ld);    
                Oper::purge(ppRedoList, sd, ld);    
            }

        void purge(const CDs *sd, const CDo *od)
            {
                Oper::purge(ppUndoList, sd, od);
                Oper::purge(ppRedoList, sd, od);
            }

        void rotate()
            {
                Ulist()->RotateUndo(ppUndoList);
                ppUndoList = 0;
                Ulist()->RotateRedo(ppRedoList);
                ppRedoList = 0;
            }

    private:
        Oper *ppUndoList;        // operations
        Oper *ppRedoList;        // redo list
    };
}


pp_state_t *
cEdit::newPPstate()
{
    return (new PPstate());
}


//
// The code below will save and restore the current transform and
// other settings.  It is intended for saving and reverting state
// during PCell evaluation.  We would like for PCell evaluation to not
// make permanent state changes.
//
// What about variables?  PCell scripts should use PushVar/PopVar
// exclusively.  During PCell evaluation, a flag is set in SIlcx,
// which can be used to lock out certain script functions such as Set
// and Unset.

namespace {
    struct sReverter
    {
        sReverter()
            {
                rv_tx                   = *GEO()->curTx();
                rv_array_params         = ED()->arrayParams();
                rv_45                   = ED()->state45();
                rv_wire_style           = ED()->getWireStyle();
                rv_move_or_copy         = ED()->moveOrCopy();
                rv_lchange_mode         = ED()->getLchgMode();
                rv_place_ref            = ED()->instanceRef();
                rv_stretch_box_code     = ED()->stretchBoxCode();
                rv_h_justify            = ED()->horzJustify();
                rv_v_justify            = ED()->vertJustify();
                rv_auto_abut_mode       = ED()->pcellAbutMode();
                rv_hide_grips           = ED()->hideGrips();
                rv_replacing            = ED()->replacing();
                rv_use_array            = ED()->useArray();
                rv_no_wire_width_mag    = ED()->noWireWidthMag();
            }

        ~sReverter()
            {
                GEO()->setCurTx         (rv_tx);
                ED()->setArrayParams    (rv_array_params);
                ED()->setState45        (rv_45);
                ED()->setWireStyle      (rv_wire_style);
                ED()->setMoveOrCopy     (rv_move_or_copy);
                ED()->setLchgMode       (rv_lchange_mode);
                ED()->setInstanceRef    (rv_place_ref);
                ED()->setStretchBoxCode (rv_stretch_box_code);
                ED()->setHorzJustify    (rv_h_justify);
                ED()->setVertJustify    (rv_v_justify);
                ED()->setPCellAbutMode  (rv_auto_abut_mode);
                ED()->setHideGrips      (rv_hide_grips);
                ED()->setReplacing      (rv_replacing);
                ED()->setUseArray       (rv_use_array);
                ED()->setNoWireWidthMag (rv_no_wire_width_mag);
            }

    private:
        sCurTx      rv_tx;
        iap_t       rv_array_params;
        sEdit45     rv_45;
        WireStyle   rv_wire_style;
        CDmcType    rv_move_or_copy;
        LCHGmode    rv_lchange_mode;
        PLref       rv_place_ref;
        int         rv_stretch_box_code;
        short       rv_h_justify;
        short       rv_v_justify;
        AbutMode    rv_auto_abut_mode;
        bool        rv_hide_grips;
        bool        rv_replacing;
        bool        rv_use_array;
        bool        rv_no_wire_width_mag;
    };
}


void *
cEdit::saveState()
{
    return (new sReverter);
}


void
cEdit::revertState(void *arg)
{
    delete ((sReverter*)arg);
}

