
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
#include "cvrt.h"
#include "editif.h"
#include "scedif.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "promptline.h"
#include "tech.h"
#include "events.h"
#include "oa_if.h"
#include "menu.h"
#include "file_menu.h"


namespace {
    namespace file_menu {
        MenuFunc  M_SelectFile;
        MenuFunc  M_Save;
        MenuFunc  M_SaveAs;
        MenuFunc  M_SaveAsDev;
        MenuFunc  M_Hcopy;
        MenuFunc  M_Files;
        MenuFunc  M_Hier;
        MenuFunc  M_Geom;
        MenuFunc  M_Libraries;
        MenuFunc  M_OAlibs;
        MenuFunc  M_Exit;
    }
}

using namespace file_menu;

#define MENU_SIZE 12

namespace {
    MenuEnt FileMenu[fileMenu_END + 1];
    MenuBox FileMenuBox;

    // Entries in Open pop-up menu.
    const char *menu_list[MENU_SIZE+2];
    int menu_list_offset;


    void
    file_post_switch_proc(int, MenuEnt *menu)
    {
        if (!menu || !DSP()->MainWdesc())
            return;
        Menu()->SetVisible(menu[fileMenuSaveAsDev].cmd.caller,
            DSP()->CurMode() == Electrical);
    }
}


MenuBox *
cMain::createFileMenu()
{
    FileMenu[fileMenu] =
        MenuEnt(M_NoOp,     "",        ME_MENU,     CMD_SAFE,
        0);
    FileMenu[fileMenuFsel] =
        MenuEnt(M_SelectFile, MenuFSEL,ME_VANILLA,  CMD_SAFE,
        MenuFSEL": Pop up a file browser panel.");
    FileMenu[fileMenuOpen] =
        MenuEnt(&M_NoOp,    MenuOPEN,  ME_VANILLA,  CMD_NOTSAFE,
        MenuOPEN": Open new cell or file.");
    FileMenu[fileMenuSave] =
        MenuEnt(M_Save,     MenuSV,    ME_VANILLA,  CMD_PROMPT,
        MenuSV": Pop up to save modified cells.");
    FileMenu[fileMenuSaveAs] =
        MenuEnt(M_SaveAs,   MenuSAVE,  ME_VANILLA,  CMD_PROMPT,
        MenuSAVE": Save current cell, with optional name change.");
    FileMenu[fileMenuSaveAsDev] =
        MenuEnt(M_SaveAsDev,MenuSADEV, ME_TOGGLE,   CMD_SAFE,
        MenuSADEV": Save current cell as a device.");
    FileMenu[fileMenuHcopy] =
        MenuEnt(M_Hcopy,    MenuHCOPY, ME_TOGGLE | ME_SEP, CMD_SAFE,
        MenuHCOPY": Create hard copy output.");
    FileMenu[fileMenuFiles] =
        MenuEnt(M_Files,    MenuFILES, ME_TOGGLE,   CMD_SAFE,
        MenuFILES": Pop up search path file listing panel.");
    FileMenu[fileMenuHier] =
        MenuEnt(M_Hier,     MenuHIER,  ME_TOGGLE,   CMD_SAFE,
        MenuHIER": Pop up cell hierarchies listing panel.");
    FileMenu[fileMenuGeom] =
        MenuEnt(M_Geom,     MenuGEOM,  ME_TOGGLE,   CMD_SAFE,
        MenuGEOM": Pop up cell geometries listing panel.");
    FileMenu[fileMenuLibs] =
        MenuEnt(M_Libraries, MenuLIBS, ME_TOGGLE,   CMD_SAFE,
        MenuLIBS": Pop up listing of open libraries.");
    FileMenu[fileMenuOAlib] =
        MenuEnt(M_OAlibs, MenuOALIB, ME_TOGGLE | ME_SEP, CMD_SAFE,
        MenuOALIB": Pop up listing of OpenAccess libraries.");
    FileMenu[fileMenuExit] =
        MenuEnt(M_Exit,      MenuEXIT, ME_VANILLA,  CMD_NOTSAFE,
        MenuEXIT": Exit program.");
    FileMenu[fileMenu_END] =
        MenuEnt();

    if (EditIf()->hasEdit()) {
        menu_list[0] = "new";
        menu_list[1] = "temporary";
        menu_list[2] = MENU_SEP_STRING;
        menu_list_offset = 3;
    }
    else {
        menu_list[0] = "new";
        menu_list[1] = MENU_SEP_STRING;
        menu_list_offset = 2;
    }

    FileMenuBox.name = "File";
    FileMenuBox.menu = FileMenu;
    FileMenuBox.registerPostSwitchProc(&file_post_switch_proc);
    return (&FileMenuBox);
}


const char *const *
cMain::OpenCellMenuList()
{
    return (menu_list);
}


// Add name to drop down menu.
//
void
cMain::PushOpenCellName(const char *name)
{
    MenuEnt *ent = Menu()->FindEntry(MMfile, MenuOPEN);
    if (!ent || !ent->cmd.caller)
        return;
    int i;
    for (i = menu_list_offset; i <= MENU_SIZE && menu_list[i]; i++) {
        if (!strcmp(name, menu_list[i])) {
            // found name, move to top
            const char *s = menu_list[i];
            while (i > menu_list_offset) {
                menu_list[i] = menu_list[i-1];
                Menu()->SetDDentry(ent->cmd.caller, i, menu_list[i]);
                i--;
            }
            menu_list[menu_list_offset] = s;
            Menu()->SetDDentry(ent->cmd.caller, menu_list_offset,
                menu_list[menu_list_offset]);
            return;
        }
    }
    for (i = menu_list_offset; i <= MENU_SIZE && menu_list[i]; i++) ;
    if (i <= MENU_SIZE)
        Menu()->NewDDentry(ent->cmd.caller, "");
    else
        i--;
    while (i > menu_list_offset) {
        menu_list[i] = menu_list[i-1];
        Menu()->SetDDentry(ent->cmd.caller, i, menu_list[i]);
        i--;
    }
    menu_list[menu_list_offset] = lstring::copy(name);
    Menu()->SetDDentry(ent->cmd.caller, menu_list_offset,
        menu_list[menu_list_offset]);
}


// Rename the drop pown menu entry for oldname to newname.
//
void
cMain::ReplaceOpenCellName(const char *newname, const char *oldname)
{
    for (int i = menu_list_offset; i <= MENU_SIZE && menu_list[i]; i++) {
        if (!strcmp(menu_list[i], oldname)) {
            if (newname && *newname) {
                delete [] menu_list[i];
                menu_list[i] = lstring::copy(newname);
                MenuEnt *ent = Menu()->FindEntry(MMfile, MenuOPEN);
                if (!ent || !ent->cmd.caller)
                    return;
                Menu()->SetDDentry(ent->cmd.caller, i, menu_list[i]);
            }
            break;
        }
    }
}


//-----------------------------------------------------------------------------
// The OPEN command.
//

namespace {
    // Not sure whether it is necessary to enclose the EditCell call
    // in an idle proc, but it doesn't hurt.
    //
    int
    edit_idle_proc(void*)
    {
        XM()->EditCell(0, false);
        return (false);
    }
}


// Callback to handle button presses in the Open pop-up menu.  The arg
// is one of the elments of menu_list.
//
void
cMain::HandleOpenCellMenu(const char *entry, bool shift)
{
    if (!entry)
        return;
    EV()->InitCallback();
    if (!strcmp(entry, menu_list[0])) {
        // new
        // Prompt to open a new cell.
        dspPkgIf()->RegisterIdleProc(&edit_idle_proc, 0);
    }
    else if (!strcmp(entry, menu_list[1])) {
        // temporary
        // Opan a unique new cell.
        char *s = XM()->NewCellName();
        EditCell(s, false);
        delete [] s;
    }
    else {
        if (shift && EditIf()->hasEdit())
            // Shift pressed, set master instead.
            EditIf()->addMaster(entry, 0);
        else
            EditCell(entry, false);
    }
}


//-----------------------------------------------------------------------------
// The FSEL command.
//

namespace {
    int
    load_idle(void *arg)
    {
        char *fname = (char*)arg;
        XM()->EditCell(fname, false);
        delete [] fname;
        return (0);
    }


    // ok button callback for the file manager.
    //
    void
    file_sel_cb(const char *fname, void*)
    {
        fname = lstring::copy(fname);
        bool didit = false;
        if (PL()->IsEditing()) {
            if (ScedIf()->doingPlot()) {
                // Plot/Iplot edit line is active, break out.
                PL()->AbortEdit();

                // Have to use an idle proce here, so that the prompt
                // line editor can finish.  It will hang otherwise if
                // Load pop up a "modal" (e.g., merge control) pop-up.

                dspPkgIf()->RegisterIdleProc(&load_idle, (void*)fname);
                return;
            }
            else {
                // If editing, insert fname at insertion point.
                PL()->EditPrompt(0, fname, PLedInsert);
                didit = true;
            }
        }
        if (!didit)
            XM()->EditCell(fname, false);
        delete [] fname;
    }
}


// Pop up the file manager.
//
void
file_menu::M_SelectFile(CmdDesc*)
{
    XM()->PopUpFileSel(0, file_sel_cb, 0);
}


//-----------------------------------------------------------------------------
// The SV command.
//
// Menu command to save current cell, and any modified cells in memory.
//
void
file_menu::M_Save(CmdDesc*)
{
    XM()->Save();
}


//-----------------------------------------------------------------------------
// The SAVE command.
//
// Menu command to save the current cell, perhaps under a new name.
//
void
file_menu::M_SaveAs(CmdDesc*)
{
    XM()->SaveCellAs(0);
}


//-----------------------------------------------------------------------------
// The SADEV command.
//
// Menu command to save the current cell as a device.
//
void
file_menu::M_SaveAsDev(CmdDesc *cmd)
{
    ScedIf()->PopUpDevEdit(cmd ? cmd->caller : 0,
        cmd && Menu()->GetStatus(cmd->caller) ? MODE_ON : MODE_OFF);
}


//-----------------------------------------------------------------------------
// The HCOPY command.
//
// Pop up/down the hardcopy form.
//
void
file_menu::M_Hcopy(CmdDesc *cmd)
{
    DSPmainWbag(PopUpPrint(cmd ? cmd->caller : 0, &Tech()->HC(), HCgraphical))
}


//-----------------------------------------------------------------------------
// The FILES command.
//
// Pop up/down the Files Listing.
//
void
file_menu::M_Files(CmdDesc *cmd)
{
    Cvt()->PopUpFiles(cmd ? cmd->caller : 0,
        cmd && Menu()->GetStatus(cmd->caller) ? MODE_ON : MODE_OFF);
}


//-----------------------------------------------------------------------------
// The HIER command.
//
// Pop up/down the listing of cell hierarchy digests.
//
void
file_menu::M_Hier(CmdDesc *cmd)
{
    Cvt()->PopUpHierarchies(cmd ? cmd->caller : 0,
        cmd && Menu()->GetStatus(cmd->caller) ? MODE_ON : MODE_OFF);
}


//-----------------------------------------------------------------------------
// The GEOM command.
//
// Pop up/down the listing of cell geometry digests.
//
void
file_menu::M_Geom(CmdDesc *cmd)
{
    Cvt()->PopUpGeometries(cmd ? cmd->caller : 0,
        cmd && Menu()->GetStatus(cmd->caller) ? MODE_ON : MODE_OFF);
}


//-----------------------------------------------------------------------------
// The LIBS command.
//
// Pop up/down the Libraries Listing.
//
void
file_menu::M_Libraries(CmdDesc *cmd)
{
    Cvt()->PopUpLibraries(cmd ? cmd->caller : 0,
        cmd && Menu()->GetStatus(cmd->caller) ? MODE_ON : MODE_OFF);
}


//-----------------------------------------------------------------------------
// The OALIB command.
//
// Pop up/down the OpenAccess Libraries Listing.
//
void
file_menu::M_OAlibs(CmdDesc *cmd)
{
    OAif()->PopUpOAlibraries(cmd ? cmd->caller : 0,
        cmd && Menu()->GetStatus(cmd->caller) ? MODE_ON : MODE_OFF);
}


//-----------------------------------------------------------------------------
// The EXIT command.
//
// Exit the application, after checking the user's intentions regarding
// unsaved cells.
//
void
file_menu::M_Exit(CmdDesc*)
{
    XM()->Exit(ExitCheckMod);
}

