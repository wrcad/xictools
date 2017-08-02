
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
#include "menu.h"
#include "misc_menu.h"
#include "layertab.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "errorlog.h"

#include "bitmaps/wr.xpm"
#include "bitmaps/ltvis.xpm"
#include "bitmaps/lpal.xpm"
#include "bitmaps/selcp.xpm"
#include "bitmaps/desel.xpm"
#include "bitmaps/setcl.xpm"
#include "bitmaps/rdraw.xpm"


// This is the "WR" button and the button menu arrayed horizontally
// across the top of the main window.

namespace {
    namespace misc_menu {
        MenuFunc  M_Mail;
        MenuFunc  M_LtabVis;
        MenuFunc  M_Palette;
        MenuFunc  M_Setcl;
        MenuFunc  M_SelPanel;
        MenuFunc  M_Deselect;
        MenuFunc  M_Redraw;
    }
}

using namespace misc_menu;

namespace {
    MenuEnt MiscMenu[miscMenu_END + 1];
    MenuBox MiscMenuBox;
}

MenuBox *
cMain::createMiscMenu()
{
    MiscMenu[miscMenu] =
        MenuEnt(&M_NoOp,     "",        ME_MENU,    CMD_SAFE,
        0);
    MiscMenu[miscMenuMail] =
        MenuEnt(&M_Mail,    MenuMAIL,   ME_VANILLA, CMD_SAFE,
        MenuMAIL": Send email.",                                wr_xpm);
    MiscMenu[miscMenuLtvis] =
        MenuEnt(&M_LtabVis, MenuLTVIS,  ME_TOGGLE,  CMD_SAFE,
        MenuLTVIS": Show/hide layer table.",                    ltvis_xpm);
    MiscMenu[miscMenuLpal] =
        MenuEnt(&M_Palette, MenuLPAL,   ME_TOGGLE,  CMD_SAFE,
        MenuLPAL": Show/hide layer palette.",                   lpal_xpm);
    MiscMenu[miscMenuSetcl] =
        MenuEnt(&M_Setcl,   MenuSETCL,  ME_TOGGLE,  CMD_SAFE,
        MenuSETCL": Set current layer from clicked-on object.", setcl_xpm);
    MiscMenu[miscMenuSelcp] =
        MenuEnt(&M_SelPanel,MenuSELCP,  ME_TOGGLE,  CMD_SAFE,
        MenuSELCP": Show selection control panel.",             selcp_xpm);
    MiscMenu[miscMenuDesel] =
        MenuEnt(&M_Deselect,MenuDESEL,  ME_VANILLA, CMD_SAFE,
        MenuDESEL": Deselect all objects.",                     desel_xpm);
    MiscMenu[miscMenuRdraw] =
        MenuEnt(&M_Redraw,  MenuRDRAW,  ME_VANILLA, CMD_SAFE,
        MenuRDRAW": Redraw main and similar drawing windows.",  rdraw_xpm);
    MiscMenu[miscMenu_END] =
        MenuEnt();

    MiscMenuBox.name = "Misc";
    MiscMenuBox.menu = MiscMenu;
    return (&MiscMenuBox);
}


//-----------------------------------------------------------------------------
// The MAIL command
//
// Pop up an email client.
//
void
misc_menu::M_Mail(CmdDesc*)
{
    char buf[128];
    sprintf(buf, "%s-%s bug", XM()->Product(), XM()->VersionString());
    DSPmainWbag(PopUpMail(buf, Log()->MailAddress()))
}


//-----------------------------------------------------------------------------
// The LTVIS command
// 
// Command to show/hide the layer table.
//
void
misc_menu::M_LtabVis(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        LT()->HideLayerTable(true);
    else
        LT()->HideLayerTable(false);
}


//-----------------------------------------------------------------------------
// The LPAL command
// 
// Command to pop up/down the Layer Palette.
//
void
misc_menu::M_Palette(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        XM()->PopUpLayerPalette(cmd->caller, MODE_ON, false, 0);
    else
        XM()->PopUpLayerPalette(0, MODE_OFF, false, 0);

}


//-----------------------------------------------------------------------------
// The SETCL command
// 
// Set the current layer by clicking on an object.
//
void
misc_menu::M_Setcl(CmdDesc *cmd)
{
    LT()->SetCurLayerFromClick(cmd);
}


//-----------------------------------------------------------------------------
// The SELCP command
//
// Command to bring up the Selection Control panel.
//
void
misc_menu::M_SelPanel(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        XM()->PopUpSelectControl(cmd->caller, MODE_ON);
    else
        XM()->PopUpSelectControl(0, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The DESEL command
// 
// Command to deselect everything.
//
void
misc_menu::M_Deselect(CmdDesc*)
{
    XM()->DeselectExec(0);
}


//-----------------------------------------------------------------------------
// The RDRAW command
// 
// Command to redraw the drawing windows that are of the same mode as
// the main drawing window.
//
void
misc_menu::M_Redraw(CmdDesc*)
{
    DSP()->RedisplayAll(DSP()->CurMode());
}

