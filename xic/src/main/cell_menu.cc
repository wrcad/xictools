
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
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "pushpop.h"
#include "menu.h"
#include "cell_menu.h"

namespace {
    namespace cell_menu {
        MenuFunc  M_Push;
        MenuFunc  M_Pop;
        MenuFunc  M_SymTabs;
        MenuFunc  M_Cells;
        MenuFunc  M_Tree;
    }
}

using namespace cell_menu;

namespace {
    MenuEnt CellMenu[cellMenu_END + 1];
    MenuBox CellMenuBox;
}


MenuBox *
cMain::createCellMenu()
{
    CellMenu[cellMenu] =
        MenuEnt(M_NoOp,     "",        ME_MENU,     CMD_SAFE,
        0);
    CellMenu[cellMenuPush] =
        MenuEnt(&M_Push,    MenuPUSH,  ME_TOGGLE,   CMD_NOTSAFE,
        MenuPUSH": Push editing context to subcell.");
    CellMenu[cellMenuPop] =
        MenuEnt(&M_Pop,     MenuPOP,   ME_VANILLA,  CMD_NOTSAFE,
        MenuPOP": Pop editing context from subcell.");
    CellMenu[cellMenuStabs] =
        MenuEnt(M_SymTabs,  MenuSTABS, ME_TOGGLE,   CMD_SAFE,
        MenuSTABS": Create or select symbol table.");
    CellMenu[cellMenuCells] =
        MenuEnt(M_Cells,    MenuCELLS, ME_TOGGLE,   CMD_SAFE,
        MenuCELLS": Pop up cells listing panel.");
    CellMenu[cellMenuTree] =
        MenuEnt(M_Tree,     MenuTREE,  ME_TOGGLE,   CMD_SAFE,
        MenuTREE": Pop up tree listing of cells.");
    CellMenu[cellMenu_END] =
        MenuEnt();

    CellMenuBox.name = "Cell";
    CellMenuBox.menu = CellMenu;
    return (&CellMenuBox);
}


// Called from menu dispatch code, special command handling is done
// here.
//
bool
cMain::setupCommand(MenuEnt *ent, bool *retval, bool *call_on_up)
{
    *retval = true;
    *call_on_up = false;

    if (!strcmp(ent->entry, MenuPUSH)) {
        pushSetup();
        return (true);
    }

    return (false);
}


//-----------------------------------------------------------------------------
// The PUSH command.
//
void
cell_menu::M_Push(CmdDesc *cmd)
{
    XM()->pushExec(cmd);
}


//-----------------------------------------------------------------------------
// The POP command.
//
// Menu command to pop the editing context back to the parent.
//
void
cell_menu::M_Pop(CmdDesc*)
{
    PP()->PopContext();
}


//-----------------------------------------------------------------------------
// The STABS command.
//
void
cell_menu::M_SymTabs(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        XM()->PopUpSymTabs(cmd->caller, MODE_ON);
    else
        XM()->PopUpSymTabs(0, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The CELLS command.
//
// Pop up/down the Cells Listing.
//
void
cell_menu::M_Cells(CmdDesc *cmd)
{
    XM()->PopUpCells(cmd ? cmd->caller : 0,
        cmd && Menu()->GetStatus(cmd->caller) ? MODE_ON : MODE_OFF);
}


//-----------------------------------------------------------------------------
// The TREE command.
//
// Pop up/down the Tree Listing.
//
void
cell_menu::M_Tree(CmdDesc *cmd)
{
    if (DSP()->MainWdesc()->DbType() == WDcddb) {
        if (DSP()->CurCellName())
            XM()->PopUpTree(cmd ? cmd->caller : 0,
                cmd && Menu()->GetStatus(cmd->caller) ? MODE_ON : MODE_OFF,
                Tstring(DSP()->CurCellName()), TU_CUR);
    }
    else if (DSP()->MainWdesc()->DbType() == WDchd) {
        XM()->PopUpTree(cmd ? cmd->caller : 0,
            cmd && Menu()->GetStatus(cmd->caller) ? MODE_ON : MODE_OFF, 0,
            TU_CUR);
    }
}

