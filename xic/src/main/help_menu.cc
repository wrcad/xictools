
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
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "filestat.h"
#include "events.h"
#include "help/help_defs.h"
#include "menu.h"
#include "help_menu.h"
#include "errorlog.h"
#include "pathlist.h"


// The Help menu for the main and subwindows.

namespace {
    namespace help_menu {
        MenuFunc  M_Help;
        MenuFunc  M_MultiWin;
        MenuFunc  M_About;
        MenuFunc  M_RelNotes;
        MenuFunc  M_Logs;
        MenuFunc  M_Debug;
    }
}

using namespace help_menu;

namespace {
    MenuEnt HelpMenu[helpMenu_END + 1];
    MenuBox HelpMenuBox;
}

MenuBox *
cMain::createHelpMenu()
{
    HelpMenu[helpMenu] =
        MenuEnt(M_NoOp,     "",        ME_MENU,     CMD_SAFE,
        0);
    HelpMenu[helpMenuHelp] =
        MenuEnt(M_Help,     MenuHELP,  ME_TOGGLE,   CMD_SAFE,
        MenuHELP": Pop up the help browser.");
    HelpMenu[helpMenuMultw] =
        MenuEnt(M_MultiWin, MenuMULTW, ME_TOGGLE | ME_SEP, CMD_SAFE,
        MenuMULTW": Use multiple windows in help mode.");
    HelpMenu[helpMenuAbout] =
        MenuEnt(M_About,    MenuABOUT, ME_VANILLA,  CMD_SAFE,
        MenuABOUT": Pop up the legal message and version info.");
    HelpMenu[helpMenuNotes] =
        MenuEnt(M_RelNotes, MenuNOTES, ME_VANILLA,  CMD_SAFE,
        MenuNOTES": Pop up the release document browser.");
    HelpMenu[helpMenuLogs] =
        MenuEnt(M_Logs,     MenuLOGS,  ME_VANILLA,  CMD_SAFE,
        MenuLOGS": access log files.");
    HelpMenu[helpMenuDebug] =
        MenuEnt(M_Debug,    MenuDBLOG, ME_TOGGLE,   CMD_SAFE,
        MenuDBLOG": set logging and debugging modes.");
    HelpMenu[helpMenu_END] =
        MenuEnt();

    HelpMenuBox.name = "Help";
    HelpMenuBox.menu = HelpMenu;
    return (&HelpMenuBox);
}


namespace {
    MenuEnt SubwHelpMenu[subwHelpMenu_END + 1];
    MenuBox SubwHelpMenuBox;
}

MenuBox *
cMain::createSubwHelpMenu()
{
    SubwHelpMenu[subwHelpMenu] =
        MenuEnt(M_NoOp,     "",        ME_MENU,     CMD_SAFE,
        0);
    SubwHelpMenu[subwHelpMenuHelp] =
        MenuEnt(M_Help,     MenuHELP,  ME_VANILLA,  CMD_SAFE,
        MenuHELP": Show help information about this window.");
    SubwHelpMenu[subwHelpMenu_END] =
        MenuEnt();

    SubwHelpMenuBox.name = "help";
    SubwHelpMenuBox.menu = SubwHelpMenu;
    return (&SubwHelpMenuBox);
}


//-----------------------------------------------------------------------------
// The HELP command.
//

namespace {
    namespace help_menu {
        struct HelpState : public CmdState
        {
            friend void help_menu::M_Help(CmdDesc*);
            friend void cMain::QuitHelp();

            HelpState(const char*, const char*);
            virtual ~HelpState();

            void setCaller(GRobject c)  { Caller = c; }

            void b1down();
            void esc();

        private:
            GRobject Caller;
        };

        HelpState *HelpCmd;
    }
}

using namespace help_menu;


void
cMain::QuitHelp()
{
    if (HelpCmd)
        HelpCmd->esc();
}


// The Help function.  Pointing at buttons or areas of the screen
// produces pop-up descriptions.
//
void
help_menu::M_Help(CmdDesc *cmd)
{
    if (cmd->wdesc != DSP()->MainWdesc()) {
        DSPmainWbag(PopUpHelp("xic:vport"))
        return;
    }
    if (Menu()->IsGlobalInsensitive()) {
        DSPmainWbag(PopUpHelp("xicinfo"))
        return;
    }
    XM()->SetDoingHelp(true);
    HelpCmd = new HelpState("HELP", 0);
    HelpCmd->setCaller(cmd ? cmd->caller : 0);
    if (!EV()->PushCallback(HelpCmd)) {
        if (cmd)
            Menu()->Deselect(cmd->caller);
        delete HelpCmd;
        HelpCmd = 0;
        return;
    }
    HLP()->set_init_x(400);
    const char *topkw = CDvdb()->getVariable(VA_HelpDefaultTopic);
    if (!topkw)
        topkw = "xicinfo";
    if (*topkw) {
        if (!DSP()->MainWdesc() ||
                !DSP()->MainWdesc()->Wbag() ||
                !DSP()->MainWdesc()->Wbag()->PopUpHelp(topkw))
            if (HelpCmd)
                HelpCmd->esc();
    }
}


HelpState::HelpState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
}


HelpState::~HelpState()
{
    HelpCmd = 0;
}


void
HelpState::b1down()
{
    if (EV()->CurrentWin()) {
        if (EV()->CurrentWin() == DSP()->MainWdesc())
            DSPmainWbag(PopUpHelp("mainwindow"))
        else
            DSPmainWbag(PopUpHelp("subwindow"))
    }
}


void
HelpState::esc()
{
    XM()->SetDoingHelp(false);
    Menu()->Deselect(Caller);
    EV()->PopCallback(this);
    delete this;
}


//-----------------------------------------------------------------------------
// The MULTW command.
//
// Set multi-window help mode.
//
void
help_menu::M_MultiWin(CmdDesc *cmd)
{
    if (!cmd || !cmd->caller)
        return;
    if (Menu()->GetStatus(cmd->caller))
        CDvdb()->setVariable(VA_HelpMultiWin, "");
    else
        CDvdb()->clearVariable(VA_HelpMultiWin);
}


//-----------------------------------------------------------------------------
// The ABOUT command.
//
// Pop-up the "about" window.
//
void
help_menu::M_About(CmdDesc*)
{
    XM()->LegalMsg();
}


//-----------------------------------------------------------------------------
// The NOTES command.
//
// Pop up the file browser loaded with the current release note.
//
void
help_menu::M_RelNotes(CmdDesc*)
{
    char *notefile = XM()->ReleaseNotePath();
    if (!notefile)
        return;

    check_t ret = filestat::check_file(notefile, R_OK);
    if (ret == NOGO) {
        DSPmainWbag(PopUpMessage(filestat::error_msg(), true))
        delete [] notefile;
        return;
    }
    if (ret == NO_EXIST) {
        char *tbuf = new char[strlen(notefile) + 100];
        sprintf(tbuf, "Can't find file %s.", notefile);
        DSPmainWbag(PopUpMessage(tbuf, true))
        delete [] tbuf;
        delete [] notefile;
        return;
    }
    DSPmainWbag(PopUpFileBrowser(notefile))
    delete [] notefile;
}


//-----------------------------------------------------------------------------
// The LOGS command.
//
// Pop up the file manager displaying the log files area.
//
namespace {
    void
    log_cb(const char *fname, void*)
    {
        if (fname)
            DSPmainWbag(PopUpFileBrowser(fname))
    }
}


void
help_menu::M_Logs(CmdDesc*)
{
    XM()->PopUpFileSel(Log()->LogDirectory(), log_cb, 0);
}


//-----------------------------------------------------------------------------
// The DBLOG command.
//
// Pop up the Logging and Debugging control panel.
//
void
help_menu::M_Debug(CmdDesc *cmd)
{
    if (cmd && cmd->caller) {
        if (Menu()->GetStatus(cmd->caller))
            XM()->PopUpDebugFlags(cmd->caller, MODE_ON);
        else
            XM()->PopUpDebugFlags(cmd->caller, MODE_OFF);
    }
}

