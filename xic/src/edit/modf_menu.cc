
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
#include "edit.h"
#include "undolist.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "layertab.h"
#include "select.h"
#include "events.h"
#include "menu.h"
#include "modf_menu.h"
#include "promptline.h"


namespace {
    namespace modf_menu {
        MenuFunc  M_Undo;
        MenuFunc  M_Redo;
        MenuFunc  M_Delete;
        MenuFunc  M_EraseUnder;
        MenuFunc  M_Move;
        MenuFunc  M_Copy;
        MenuFunc  M_Stretch;
        MenuFunc  M_ChangeLayer;
        MenuFunc  M_MClayerChg;
    }
}

using namespace modf_menu;

namespace {
    MenuEnt ModfMenu[modfMenu_END + 1];
    MenuBox ModfMenuBox;
}


MenuBox *
cEdit::createModfMenu()
{
    ModfMenu[modfMenu] =
        MenuEnt(&M_NoOp,     "",        ME_MENU,     CMD_SAFE,
        0),
    ModfMenu[modfMenuUndo] =
        MenuEnt(&M_Undo,     MenuUNDO,  ME_VANILLA,  CMD_UNDO,
        MenuUNDO": Undo the last operation.");
    ModfMenu[modfMenuRedo] =
        MenuEnt(&M_Redo,     MenuREDO,  ME_VANILLA,  CMD_REDO,
        MenuREDO": Re-do the last undone operation.");
    ModfMenu[modfMenuDelet] =
        MenuEnt(&M_Delete,   MenuDELET, ME_VANILLA,  CMD_NOTSAFE,
        MenuDELET": Delete the selected objects.");
    ModfMenu[modfMenuEundr] =
        MenuEnt(&M_EraseUnder,MenuEUNDR,ME_VANILLA,  CMD_NOTSAFE,
        MenuEUNDR": Erase intersection area with selected objects.");
    ModfMenu[modfMenuMove] =
        MenuEnt(&M_Move,     MenuMOVE,  ME_TOGGLE,   CMD_NOTSAFE,
        MenuMOVE": Move the selected objects.");
    ModfMenu[modfMenuCopy] =
        MenuEnt(&M_Copy,     MenuCOPY,  ME_TOGGLE,   CMD_NOTSAFE,
        MenuCOPY": Copy the selected objects.");
    ModfMenu[modfMenuStrch] =
        MenuEnt(&M_Stretch,  MenuSTRCH, ME_TOGGLE,   CMD_NOTSAFE,
        MenuSTRCH": Stretch the selected objects.");
    ModfMenu[modfMenuChlyr] =
        MenuEnt(&M_ChangeLayer,MenuCHLYR,ME_VANILLA, CMD_NOTSAFE,
        MenuCHLYR": Change layer of selected objects to current layer.");
    ModfMenu[modfMenuMClcg] =
        MenuEnt(&M_MClayerChg,MenuMCLCG,ME_TOGGLE,   CMD_SAFE,
        MenuMCLCG": Set layer change mode for move/copy.");

    ModfMenu[modfMenu_END] =
        MenuEnt();

    ModfMenuBox.name = "Modify";
    ModfMenuBox.menu = ModfMenu;
    return (&ModfMenuBox);
}

//-----------------------------------------------------------------------------
// The UNDO command.
//
// Undo the last database-modifying command.
//
void
modf_menu::M_Undo(CmdDesc*)
{
    if (EV()->CurCmd())
        EV()->CurCmd()->undo();
    else
        Ulist()->UndoOperation();
}


//-----------------------------------------------------------------------------
// The REDO command.
//
// Redo the last operation undone by Undo.
//
void
modf_menu::M_Redo(CmdDesc*)
{
    if (EV()->CurCmd())
        EV()->CurCmd()->redo();
    else
        Ulist()->RedoOperation();
}


//-----------------------------------------------------------------------------
// The DELET command.
//
// Menu command to delete all selected objects.  Also called by the
// front end pointer handler when the Delete key is pressed.
//
void
modf_menu::M_Delete(CmdDesc *cmd)
{
    ED()->deleteExec(cmd);
}


//-----------------------------------------------------------------------------
// The EUNDR command.
//
// Menu command to erase intersection of non-selected objects with
// selected objects
//
void
modf_menu::M_EraseUnder(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!Selections.hasTypes(CurCell(), 0)) {
        PL()->ShowPrompt("You must first select objects to erase under.");
        return;
    }
    if (ED()->noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, false, DSP()->CurMode()))
        return;

    Ulist()->ListCheck("eunder", CurCell(), false);
    ED()->eraseUnder();
    Ulist()->CommitChanges(true);
}


//-----------------------------------------------------------------------------
// The MOVE command.
//
void
modf_menu::M_Move(CmdDesc *cmd)
{
    ED()->moveExec(cmd);
}


//-----------------------------------------------------------------------------
// The COPY command.
//
void
modf_menu::M_Copy(CmdDesc *cmd)
{
    ED()->copyExec(cmd);
}


//-----------------------------------------------------------------------------
// The STRCH command.
//
void
modf_menu::M_Stretch(CmdDesc *cmd)
{
    ED()->stretchExec(cmd);
}


//-----------------------------------------------------------------------------
// The CHLYR command.
//
// Menu command to change the layer of the selected objects to the
// current layer.
//
void
modf_menu::M_ChangeLayer(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (ED()->noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, false, DSP()->CurMode()))
        return;

    int found = 0;
    sSelGen sg(Selections, CurCell(), "bpwl");
    CDo *od;
    while ((od = sg.next()) != 0) {
        found = 1;
        if (od->ldesc() != LT()->CurLayer()) {
            found = 2;
            break;
        }
    }
    if (!found) {
        PL()->ShowPrompt("You haven't selected anything to change.");
        return;
    }
    if (found == 1) {
        PL()->ShowPrompt("First select a new layer to change to.");
        return;
    }
    Ulist()->ListCheck("chlyr", CurCell(), true);
    ED()->changeLayer();
    if (Ulist()->CommitChanges(true))
        PL()->ShowPrompt("Selected objects changed to current layer.");
    XM()->ShowParameters();
}


//-----------------------------------------------------------------------------
// The MCLCG command.
//
// Menu command to bring up a panel for setting the layer change mode
// for move/copy and some other commands.
//
void
modf_menu::M_MClayerChg(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        ED()->PopUpLayerChangeMode(MODE_ON);
    else
        ED()->PopUpLayerChangeMode(MODE_OFF);
}

