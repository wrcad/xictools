
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
#include "cd_celldb.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "promptline.h"
#include "events.h"
#include "menu.h"
#include "view_menu.h"
#include "miscutil/pathlist.h"

// The View menu for the main and subwindows.

namespace {
    namespace view_menu {
        MenuFunc  M_SwitchMode;
        MenuFunc  M_Expand;
        MenuFunc  M_Zoom;
        MenuFunc  M_SubWindow;
        MenuFunc  M_Peek;
        MenuFunc  M_Profile;
        MenuFunc  M_Ruler;
        MenuFunc  M_Info;
        MenuFunc  M_Alloc;
    }
}

using namespace view_menu;

namespace {
    // menu entries for view pull-down
    const char *viewList[] = {
        "full ",
        "prev ",
        "next ",
        0
    };
}


const char *const *
cMain::ViewList()
{
    return (viewList);
}


namespace {
    MenuEnt ViewMenu[viewMenu_END + 1];
    MenuBox ViewMenuBox;

    void
    view_pre_switch_proc(int, MenuEnt *menu)
    {
        if (!menu || !DSP()->MainWdesc())
            return;
        MenuEnt *ent = &menu[viewMenuExpnd];
        if (Menu()->GetStatus(ent->cmd.caller))
            // pop down expnd popup, or reset peek mode
            Menu()->CallCallback(ent->cmd.caller);
    }


    void
    view_post_switch_proc(int, MenuEnt *menu)
    {
        if (!menu || !DSP()->MainWdesc())
            return;
        for (int i = 1; i < viewMenu_END; i++) {
            MenuEnt *ent = &menu[i];
            if (i == viewMenuCsect) {
                if (DSP()->CurMode() == Electrical)
                    Menu()->SetSensitive(ent->cmd.caller, false);
                else
                    Menu()->SetSensitive(ent->cmd.caller, true);
            }
            else if (i == viewMenuRuler) {
                if (DSP()->CurMode() == Electrical)
                    Menu()->SetSensitive(ent->cmd.caller, false);
                else
                    Menu()->SetSensitive(ent->cmd.caller, true);
            }
        }
    }
}


MenuBox *
cMain::createViewMenu()
{
    ViewMenu[viewMenu] =
        MenuEnt(&M_NoOp,     "",        ME_MENU,     CMD_SAFE,
        0);
    ViewMenu[viewMenuView] =
        MenuEnt(&M_NoOp,     MenuVIEW,  ME_VANILLA,  CMD_SAFE,
        MenuVIEW": Set the displayed cell view.");
    ViewMenu[viewMenuSced] =
        MenuEnt(&M_SwitchMode,MenuSCED, ME_VANILLA,  CMD_NOTSAFE,
        MenuSCED": Switch to electrical mode." );
    ViewMenu[viewMenuPhys] =
        MenuEnt(&M_SwitchMode,MenuPHYS, ME_ALT | ME_SEP, CMD_NOTSAFE,
        MenuPHYS": Switch to physical mode." );
    ViewMenu[viewMenuExpnd] =
        MenuEnt(&M_Expand,   MenuEXPND, ME_TOGGLE,   CMD_SAFE,
        MenuEXPND": Change expanded status of subcells.");
    ViewMenu[viewMenuZoom] =
        MenuEnt(&M_Zoom,     MenuZOOM,  ME_TOGGLE,   CMD_SAFE,
        MenuZOOM": Zoom, or set display window.");
    ViewMenu[viewMenuVport] =
        MenuEnt(&M_SubWindow,MenuVPORT, ME_VANILLA,  CMD_SAFE,
        MenuVPORT": Pop up another drawing window.");
    ViewMenu[viewMenuPeek] =
        MenuEnt(&M_Peek,     MenuPEEK,  ME_TOGGLE,   CMD_SAFE,
        MenuPEEK": Show underlying layers sequentially.");
    ViewMenu[viewMenuCsect] =
        MenuEnt(&M_Profile, MenuCSECT,  ME_TOGGLE,   CMD_SAFE,
        MenuCSECT": Show underlying layers in cross section.");
    ViewMenu[viewMenuRuler] =
        MenuEnt(&M_Ruler,    MenuRULER, ME_TOGGLE,   CMD_SAFE,
        MenuRULER": Create gradations for length measurement.");
    ViewMenu[viewMenuInfo] =
        MenuEnt(&M_Info,     MenuINFO,  ME_TOGGLE | ME_SEP,  CMD_NOTSAFE,
        MenuINFO": Pop up info window, info about objects.");
    ViewMenu[viewMenuAlloc] =
        MenuEnt(&M_Alloc,    MenuALLOC, ME_TOGGLE,   CMD_SAFE,
        MenuALLOC": Pop up memory statistics.");
    ViewMenu[viewMenu_END] =
        MenuEnt();

    ViewMenuBox.name = "View";
    ViewMenuBox.menu = ViewMenu;
    ViewMenuBox.registerPreSwitchProc(&view_pre_switch_proc);
    ViewMenuBox.registerPostSwitchProc(&view_post_switch_proc);
    return (&ViewMenuBox);
}


namespace {
    namespace subw_view_menu {
        MenuFunc  M_WinDump;
        MenuFunc  M_LocShow;
        MenuFunc  M_SwapMain;
        MenuFunc  M_LoadNew;
        MenuFunc  M_Cancel;
    }
}

using namespace subw_view_menu;

namespace {
    MenuEnt SubwViewMenu[subwViewMenu_END + 1];
    MenuBox SubwViewMenuBox;

    void
    subw_view_post_switch_proc(int wnum, MenuEnt *menu)
    {
        if (!menu || wnum < 1 || wnum >= DSP_NUMWINS)
            return;
        WindowDesc *wdesc = DSP()->Window(wnum);
        if (!wdesc)
            return;

        if (wdesc->DbType() != WDcddb) {
            MenuEnt *ent = &menu[subwViewMenuSwap];
            Menu()->SetSensitive(ent->cmd.caller, false);
        }
        if (wdesc->DbType() == WDblist || wdesc->DbType() == WDsdb) {
            // cross section or special database display
            MenuEnt *ent = &menu[subwViewMenuSced];
            Menu()->SetSensitive(ent->cmd.caller, false);
            ent = &menu[subwViewMenuExpnd];
            Menu()->SetSensitive(ent->cmd.caller, false);
            ent = &menu[subwViewMenuLshow];
            Menu()->SetSensitive(ent->cmd.caller, false);
            ent = &menu[subwViewMenuLoad];
            Menu()->SetSensitive(ent->cmd.caller, false);
            return;
        }
        MenuEnt *ent = &menu[subwViewMenuLshow];
        Menu()->SetSensitive(ent->cmd.caller, (wdesc->Mode() == Physical));
    }
}


MenuBox *
cMain::createSubwViewMenu()
{
    SubwViewMenu[subwViewMenu] =
        MenuEnt(M_NoOp,     "",        ME_MENU,     CMD_SAFE,
        0);
    SubwViewMenu[subwViewMenuView] =
        MenuEnt(M_NoOp,     MenuVIEW,  ME_VANILLA,  CMD_SAFE,
        "Set the displayed cell view.");
    SubwViewMenu[subwViewMenuSced] =
        MenuEnt(M_SwitchMode,MenuSCED, ME_VANILLA,  CMD_SAFE,
        MenuSCED": Switch to electrical mode.");
    SubwViewMenu[subwViewMenuPhys] =
        MenuEnt(M_SwitchMode,MenuPHYS, ME_ALT | ME_SEP, CMD_SAFE,
        MenuPHYS": Switch to physical mode.");
    SubwViewMenu[subwViewMenuExpnd] =
        MenuEnt(M_Expand,   MenuEXPND, ME_TOGGLE,   CMD_SAFE,
        MenuEXPND": Change expanded status of subcells.");
    SubwViewMenu[subwViewMenuZoom] =
        MenuEnt(M_Zoom,     MenuZOOM,  ME_TOGGLE,   CMD_SAFE,
        MenuZOOM": Zoom, or set display window.");
    SubwViewMenu[subwViewMenuWdump] =
        MenuEnt(M_WinDump,  MenuWDUMP, ME_VANILLA,  CMD_SAFE,
        MenuWDUMP": Dump window graphics to a file.");
    SubwViewMenu[subwViewMenuLshow] =
        MenuEnt(M_LocShow,  MenuLSHOW, ME_TOGGLE,   CMD_SAFE,
        MenuLSHOW": Show window location in main window.");
    SubwViewMenu[subwViewMenuSwap] =
        MenuEnt(M_SwapMain, MenuSWAP,  ME_VANILLA,  CMD_SAFE,
        MenuSWAP": swap contents with main window.");
    SubwViewMenu[subwViewMenuLoad] =
        MenuEnt(M_LoadNew, MenuLOAD,   ME_SEP,      CMD_SAFE,
        MenuLOAD": load cell or file for viewing.");
    SubwViewMenu[subwViewMenuCancl] =
        MenuEnt(M_Cancel,   MenuCANCL, ME_VANILLA,  CMD_SAFE,
        MenuCANCL": Dismiss this window.");
    SubwViewMenu[subwViewMenu_END] =
        MenuEnt();

    SubwViewMenuBox.name = "View";
    SubwViewMenuBox.menu = SubwViewMenu;
    SubwViewMenuBox.registerPostSwitchProc(&subw_view_post_switch_proc);
    return (&SubwViewMenuBox);
}


//-----------------------------------------------------------------------------
// The SCED and PHYS mode switching command
//
// Switch the display mode of a window.  Switching the main window causes
// a general mode switch.
//
void
view_menu::M_SwitchMode(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    int wnum = -1;
    for (wnum = 0; wnum < DSP_NUMWINS; wnum++)
        if (DSP()->Window(wnum) == cmd->wdesc)
            break;
    if (wnum == DSP_NUMWINS)
        return;
    const char *label = Menu()->GetLabel(cmd->caller);
    while (isspace(*label))
        label++;
    if (*label == 'p' || *label == 'P')
        XM()->SetMode(Physical, wnum);
    else
        XM()->SetMode(Electrical, wnum);
}


//-----------------------------------------------------------------------------
// The EXPND command.
//
// Set display of geometry in subcells.
//
void
view_menu::M_Expand(CmdDesc *cmd)
{
    XM()->ExpandExec(cmd);
}


//-----------------------------------------------------------------------------
// The ZOOM command.
//
// The zoom command from the main menu.  Enables changing the window
// magnification.
//
void
view_menu::M_Zoom(CmdDesc *cmd)
{
    if (cmd) {
        if (Menu()->GetStatus(cmd->caller))
            cmd->wdesc->Wbag()->PopUpZoom(cmd->caller, MODE_ON);
        else
            cmd->wdesc->Wbag()->PopUpZoom(0, MODE_OFF);
    }
}


//-----------------------------------------------------------------------------
// The VPORT command.
//
void
view_menu::M_SubWindow(CmdDesc *cmd)
{
    XM()->SubWindowExec(cmd);
}


//-----------------------------------------------------------------------------
// The PEEK command.
//
void
view_menu::M_Peek(CmdDesc *cmd)
{
    XM()->PeekExec(cmd);
}


//-----------------------------------------------------------------------------
// The CSECT command.
//
void
view_menu::M_Profile(CmdDesc *cmd)
{
    XM()->ProfileExec(cmd);
}


//-----------------------------------------------------------------------------
// The RULER command.
//
void
view_menu::M_Ruler(CmdDesc *cmd)
{
    XM()->RulerExec(cmd);
}


//-----------------------------------------------------------------------------
// The INFO command.
//
void
view_menu::M_Info(CmdDesc *cmd)
{
    XM()->InfoExec(cmd);
}


//-----------------------------------------------------------------------------
// The ALLOC command.
//
// Pop up/down memory monitor.
//
void
view_menu::M_Alloc(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        XM()->PopUpMemory(MODE_ON);
    else
        XM()->PopUpMemory(MODE_OFF);
}


//-----------------------------------------------------------------------------
// The WDUMP command (subwindows only)
//
namespace {
    void
    wd_cb(const char *s, void *client_data)
    {
        WindowDesc *wd = (WindowDesc*)client_data;
        if (!wd)
            return;
        char *tok = 0;
        if (s)
            tok = lstring::getqtok(&s);
        if (!tok) {
            wd->Wbag()->PopUpMessage("No file name given!", true);
            return;
        }
        if (!wd->DumpWindow(tok, 0))
            wd->Wbag()->PopUpMessage(
                "Oops, an error, image file creation failed.", true);
        else
            wd->Wbag()->PopUpMessage("Image file created.", false);
        delete [] tok;
        GRledPopup *ledpop = wd->Wbag()->ActiveInput();
        if (ledpop)
            ledpop->popdown();
    }
}


// Dump subwindow to graphics file.
//
void
subw_view_menu::M_WinDump(CmdDesc *cmd)
{
    if (!cmd->wdesc)
        return;
    cmd->wdesc->Wbag()->PopUpInput(
        "Enter filename (extension implies format): ", 0, "Dump", wd_cb,
            cmd->wdesc);
}


//-----------------------------------------------------------------------------
// The LSHOW command (subwindows only)
//
// Show subwindow location as dotted box in main window.
//
void
subw_view_menu::M_LocShow(CmdDesc *cmd)
{
    if (cmd && cmd->wdesc && cmd->wdesc != DSP()->MainWdesc()
            && cmd->wdesc->Mode() == Physical && cmd->caller) {
        cmd->wdesc->SetShowLoc(Menu()->GetStatus(cmd->caller));
        DSP()->MainWdesc()->Refresh(cmd->wdesc->Window());
    }
}


//-----------------------------------------------------------------------------
// The SWAP command (subwindows only)
//
// Swap contents with the main window.
//
void
subw_view_menu::M_SwapMain(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc)
        return;
    if (!DSP()->CurCellName()) {
        PL()->ShowPrompt("Can't swap, no current cell.");
        return;
    }
    if (cmd->wdesc->DbType() != WDcddb ||
            DSP()->MainWdesc()->DbType() != WDcddb) {
        PL()->ShowPrompt("Can't swap, invalid window.");
        return;
    }
    if (!cmd->wdesc->CurCellName()) {
        PL()->ShowPrompt("Can't swap, no cell for focus window.");
        return;
    }

    int win = -1;
    for (int i = 1; i < DSP_NUMWINS; i++) {
        if (DSP()->Window(i) == cmd->wdesc) {
            win = i;
            break;
        }
    }
    if (win < 0)
        return;

    DisplayMode nmode = cmd->wdesc->Mode();
    const char *newname = Tstring(cmd->wdesc->CurCellName());
    int nx = (cmd->wdesc->Window()->left + cmd->wdesc->Window()->right)/2;
    int ny = (cmd->wdesc->Window()->bottom + cmd->wdesc->Window()->top)/2;
    double nw = cmd->wdesc->Window()->width();

    bool similar = cmd->wdesc->IsSimilar(DSP()->MainWdesc(), WDsimXsymb);
    // Don't swap if only difference is symbolic view.
    if (similar && !cmd->wdesc->IsSimilar(DSP()->MainWdesc()))
        return;

    DSP()->SetNoRedisplay(true);
    if (!similar) {
        EV()->InitCallback();

        CDcbin cbin;
        CDcdb()->findSymbol(DSP()->CurCellName(), &cbin);
        cmd->wdesc->SetSymbol(&cbin);
        if (cmd->wdesc->Mode() != DSP()->CurMode())
            XM()->SetMode(DSP()->CurMode(), win);
    }
    cmd->wdesc->InitWindow(
        (DSP()->MainWdesc()->Window()->left +
            DSP()->MainWdesc()->Window()->right)/2,
        (DSP()->MainWdesc()->Window()->bottom +
            DSP()->MainWdesc()->Window()->top)/2,
        DSP()->MainWdesc()->Window()->width());

    if (!similar) {
        XM()->Load(DSP()->MainWdesc(), newname);
        if (nmode != DSP()->CurMode())
            XM()->SetMode(nmode);
    }
    DSP()->MainWdesc()->InitWindow(nx, ny, nw);
    DSP()->SetNoRedisplay(false);
    cmd->wdesc->Redisplay(0);
    DSP()->MainWdesc()->Redisplay(0);
}


//-----------------------------------------------------------------------------
// The LOAD command (subwindows only)
//
void
subw_view_menu::M_LoadNew(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc)
        return;
    if (cmd->wdesc->DbType() != WDcddb && cmd->wdesc->DbType() != WDchd) {
        PL()->ShowPrompt("Can't load, invalid window.");
        return;
    }

    int win = -1;
    for (int i = 1; i < DSP_NUMWINS; i++) {
        if (DSP()->Window(i) == cmd->wdesc) {
            win = i;
            break;
        }
    }
    if (win < 0)
        return;

    if (cmd->wdesc->DbType() == WDcddb) {
        char *s = XM()->OpenFileDlg("File, CHD and/or cell? ",
            Tstring(DSP()->Window(win)->CurCellName()));
        char *cn = lstring::getqtok(&s);
        if (cn) {
            char *p = pathlist::expand_path(cn, false, false);
            delete [] cn;
            cn = lstring::getqtok(&s);
            XM()->Load(cmd->wdesc, p, 0, cn);
            delete [] p;
            delete [] cn;
        }
    }
    else if (cmd->wdesc->DbType() == WDchd) {
        char *s = PL()->EditPrompt("Root cell? ", "");
        char *dbname = lstring::getqtok(&s);
        char *cn = lstring::getqtok(&s);
        if (!cn) {
           cn = dbname;
           dbname = 0;
        }
        if (cn) {
            if (dbname && strcmp(dbname, cmd->wdesc->DbName())) {
                PL()->ShowPrompt("Can't load, invalid database name.");
                delete [] dbname;
                delete [] cn;
                return;
            }
            delete [] dbname;
            cmd->wdesc->SetHierDisplayMode(cmd->wdesc->DbName(), cn, 0);
            delete [] cn;
        }
    }
}


//-----------------------------------------------------------------------------
// The CANCL command (subwindows only)
//
// Dismiss a subwindow.
//
void
subw_view_menu::M_Cancel(CmdDesc *cmd)
{
    if (cmd->wdesc && cmd->wdesc != DSP()->MainWdesc())
        delete cmd->wdesc;
    // cmd is trashed
}

