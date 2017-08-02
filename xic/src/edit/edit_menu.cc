
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
#include "pcell.h"
#include "undolist.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "layertab.h"
#include "select.h"
#include "events.h"
#include "tech.h"
#include "menu.h"
#include "edit_menu.h"
#include "promptline.h"


namespace {
    namespace edit_menu {
        MenuFunc  M_Edit;
        MenuFunc  M_Setup;
        MenuFunc  M_PcCtl;
        MenuFunc  M_CreateCell;
        MenuFunc  M_CreateVia;
        MenuFunc  M_Flatten;
        MenuFunc  M_Join;
        MenuFunc  M_LayerExp;
        MenuFunc  M_Properties;
        MenuFunc  M_CellPrpty;
    }
}

using namespace edit_menu;

namespace {
    MenuEnt EditMenu[editMenu_END + 1];
    MenuBox EditMenuBox;

    void
    edit_pre_switch_proc(int, MenuEnt*)
    {
        ED()->PopUpProperties(0, MODE_OFF, PRPnochange);
        ED()->PopUpStdVia(0, MODE_OFF);
    }

    void
    edit_post_switch_proc(int, MenuEnt *menu)
    {
        if (!menu || !DSP()->MainWdesc())
            return;
        bool nostdvia = (DSP()->CurMode() == Electrical ||
            !Tech()->StdViaTab() || !Tech()->StdViaTab()->allocated());

        Menu()->SetSensitive(menu[editMenuCrvia].cmd.caller, !nostdvia);
    }
}


MenuBox *
cEdit::createEditMenu()
{
    EditMenu[editMenu] =
        MenuEnt(&M_NoOp,     "",        ME_MENU,    CMD_SAFE,
        0),
    EditMenu[editMenuCedit] =
        MenuEnt(&M_Edit,    MenuCEDIT,  ME_TOGGLE,  CMD_SAFE,
        MenuCEDIT": Enable/disable editing of cell.");
    EditMenu[editMenuEdSet] =
        MenuEnt(&M_Setup,   MenuEDSET,  ME_TOGGLE | ME_SEP, CMD_SAFE,
        MenuEDSET": Show/hide editing setup panel.");
    EditMenu[editMenuPcctl] =
        MenuEnt(&M_PcCtl,   MenuPCCTL,  ME_TOGGLE,  CMD_SAFE,
        MenuPCCTL": Control pcell auto-abutment and other options.");
    EditMenu[editMenuCrcel] =
        MenuEnt(&M_CreateCell,MenuCRCEL,ME_VANILLA, CMD_NOTSAFE,
        MenuCRCEL": Create new cell from selected objects.");
    EditMenu[editMenuCrvia] =
        MenuEnt(&M_CreateVia,MenuCRVIA, ME_TOGGLE,  CMD_SAFE,
        MenuCRVIA": Create a new standard via.");
    EditMenu[editMenuFlatn] =
        MenuEnt(&M_Flatten, MenuFLATN,  ME_TOGGLE,  CMD_SAFE,
        MenuFLATN": Flatten the cell hierarchy.");
    EditMenu[editMenuJoin] =
        MenuEnt(&M_Join,    MenuJOIN,   ME_TOGGLE,  CMD_SAFE,
        MenuJOIN": Join or atomize objects.");
    EditMenu[editMenuLexpr] =
        MenuEnt(&M_LayerExp,MenuLEXPR,  ME_TOGGLE | ME_SEP, CMD_SAFE,
        MenuLEXPR": Evaluate layer expression.");
    EditMenu[editMenuPrpty] =
        MenuEnt(&M_Properties,MenuPRPTY,ME_TOGGLE,  CMD_NOTSAFE,
        MenuPRPTY": Pop up property editor.");
    EditMenu[editMenuCprop] =
        MenuEnt(&M_CellPrpty,MenuCPROP, ME_TOGGLE,  CMD_SAFE,
        MenuCPROP": Pop up cell property editor.");

    EditMenuBox.name = "Edit";
    EditMenuBox.menu = EditMenu;
    EditMenuBox.registerPreSwitchProc(&edit_pre_switch_proc);
    EditMenuBox.registerPostSwitchProc(&edit_post_switch_proc);
    return (&EditMenuBox);
}


//-----------------------------------------------------------------------------
// The CEDIT command.
//
// Menu command to switch editing capability on/off by setting the IMMUTABLE
// flag of the current cell.
//
void
edit_menu::M_Edit(CmdDesc *cmd)
{
    if (!cmd)
        return;
    CDs *sd = CurCell(DSP()->CurMode());
    if (Menu()->GetStatus(cmd->caller)) {
        if (sd)
            sd->setImmutable(false);
        ED()->setEditingMode(true);
    }
    else {
        if (sd)
            sd->setImmutable(true);
        ED()->setEditingMode(false);
    }
    if (sd)
        XM()->PopUpCellFlags(0, MODE_UPD, 0, 0);
}


//-----------------------------------------------------------------------------
// The EDSET command.
//
// Setup panel for editing.
//
void
edit_menu::M_Setup(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        ED()->PopUpEditSetup(cmd->caller, MODE_ON);
    else
        ED()->PopUpEditSetup(0, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The PCCTL command.
//
void
edit_menu::M_PcCtl(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        ED()->PopUpPCellCtrl(cmd->caller, MODE_ON);
    else
        ED()->PopUpPCellCtrl(0, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The CRCEL command.
//
void
edit_menu::M_CreateCell(CmdDesc*)
{
    ED()->createCellExec(0);
}


//-----------------------------------------------------------------------------
// The CRVIA command.
//
void
edit_menu::M_CreateVia(CmdDesc *cmd)
{
    if (cmd) {
        if (Menu()->GetStatus(cmd->caller))
            ED()->PopUpStdVia(cmd->caller, MODE_ON);
        else
            ED()->PopUpStdVia(cmd->caller, MODE_OFF);
    }
}


//-----------------------------------------------------------------------------
// The FLATN command.
//
void
edit_menu::M_Flatten(CmdDesc *cmd)
{
    ED()->flattenExec(cmd);
}


//-----------------------------------------------------------------------------
// The JOIN command.
//
void
edit_menu::M_Join(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        ED()->PopUpJoin(cmd->caller, MODE_ON);
    else
        ED()->PopUpJoin(0, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The LEXPR command.
//
void
edit_menu::M_LayerExp(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        ED()->PopUpLayerExp(cmd->caller, MODE_ON);
    else
        ED()->PopUpLayerExp(0, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The PRPTY command.
//
void
edit_menu::M_Properties(CmdDesc *cmd)
{
    ED()->propertiesExec(cmd);
}


//-----------------------------------------------------------------------------
// The CPROP command.
//
// Menu function to pop up the cell properties editor
//
void
edit_menu::M_CellPrpty(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;

    if (cmd && Menu()->GetStatus(cmd->caller))
        ED()->PopUpCellProperties(MODE_ON);
    else
        ED()->PopUpCellProperties(MODE_OFF);
    ds.clear();
}

