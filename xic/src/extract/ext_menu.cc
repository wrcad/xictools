
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
#include "ext.h"
#include "ext_fc.h"
#include "ext_fh.h"
#include "sced.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "events.h"
#include "layertab.h"
#include "promptline.h"
#include "errorlog.h"
#include "menu.h"
#include "ext_menu.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"


namespace {
    namespace ext_menu {
        MenuFunc  M_Config;
        MenuFunc  M_ExtSel;
        MenuFunc  M_DevSelect;
        MenuFunc  M_Source;
        MenuFunc  M_Exset;
        MenuFunc  M_DumpPhysNet;
        MenuFunc  M_DumpElecNet;
        MenuFunc  M_LVS;
        MenuFunc  M_ExtractC;
        MenuFunc  M_ExtractLR;
    }
}

using namespace ext_menu;

namespace {
    MenuEnt ExtractMenu[extMenu_END + 1];
    MenuBox ExtractMenuBox;

    void
    ext_post_switch_proc(int, MenuEnt *menu)
    {
        if (!menu || !DSP()->MainWdesc())
            return;
        for (int i = 1; i < extMenu_END; i++) {
            MenuEnt *ent = &menu[i];
            if (i == extMenuExC) {
                if (DSP()->CurMode() == Electrical) {
                    if (Menu()->MenuButtonStatus(MenuEXTRC, MenuEXC) == 1)
                        Menu()->MenuButtonPress(MenuEXTRC, MenuEXC);
                }
            }
            else if (i == extMenuExLR) {
                if (DSP()->CurMode() == Electrical) {
                    if (Menu()->MenuButtonStatus(MenuEXTRC, MenuEXLR) == 1)
                        Menu()->MenuButtonPress(MenuEXTRC, MenuEXLR);
                }
            }

            switch (i) {
            case extMenuExC:
            case extMenuExLR:
                Menu()->SetSensitive(ent->cmd.caller,
                    (DSP()->CurMode() != Electrical));
                break;
            default:
                break;
            }
        }
        DSP()->SetTerminalsVisible(false);
        DSP()->SetContactsVisible(false);
        EX()->PopUpSelections(0, MODE_UPD);
        EX()->PopUpExtSetup(0, MODE_UPD);
    }
}


MenuBox *
cExt::createMenu()
{
    ExtractMenu[extMenu] =
        MenuEnt(M_NoOp,     "",         ME_MENU,    CMD_SAFE,
        0);
    ExtractMenu[extMenuExcfg] =
        MenuEnt(M_Config,   MenuEXCFG,  ME_TOGGLE,  CMD_SAFE,
        MenuEXCFG": Set up and control extraction system.");
    ExtractMenu[extMenuSel] =
        MenuEnt(M_ExtSel,   MenuEXSEL,  ME_TOGGLE,  CMD_SAFE,
        MenuEXSEL": Select and show groups/nodes/paths.");
    ExtractMenu[extMenuDvsel] =
        MenuEnt(M_DevSelect,MenuDVSEL,  ME_TOGGLE | ME_SEP,   CMD_SAFE,
        MenuDVSEL": Show, select, compute, compare devices.");
    ExtractMenu[extMenuSourc] =
        MenuEnt(M_Source,   MenuSOURC,  ME_TOGGLE,  CMD_NOTSAFE,
        MenuSOURC": Extract electrical info from SPICE file.");
    ExtractMenu[extMenuExset] =
        MenuEnt(M_Exset,    MenuEXSET,  ME_TOGGLE,  CMD_NOTSAFE,
        MenuEXSET": Extract electrical info from physical layout.");
    ExtractMenu[extMenuPnet] =
        MenuEnt(M_DumpPhysNet,MenuPNET, ME_TOGGLE,  CMD_NOTSAFE,
        MenuPNET": Write a netlist obtained from physical data.");
    ExtractMenu[extMenuEnet] =
        MenuEnt(M_DumpElecNet,MenuENET, ME_TOGGLE,  CMD_NOTSAFE,
        MenuENET": Write a netlist obtained from electrical data.");
    ExtractMenu[extMenuLvs] =
        MenuEnt(M_LVS,      MenuLVS,    ME_TOGGLE | ME_SEP, CMD_NOTSAFE,
        MenuLVS": Compare physical and electrical netlists and values.");
    ExtractMenu[extMenuExC] =
        MenuEnt(M_ExtractC, MenuEXC,    ME_TOGGLE,  CMD_SAFE,
        MenuEXC": Extract capacitance with Fast[er]Cap.");
    ExtractMenu[extMenuExLR] =
        MenuEnt(M_ExtractLR,MenuEXLR,   ME_TOGGLE,  CMD_SAFE,
        MenuEXLR": Extract inductance/resistance with FastHenry.");
    ExtractMenu[extMenu_END] =
        MenuEnt();

    ExtractMenuBox.name = "Extract";
    ExtractMenuBox.menu = ExtractMenu;
    ExtractMenuBox.registerPostSwitchProc(&ext_post_switch_proc);
    return (&ExtractMenuBox);
}


// Called from menu dispatch code, special command handling is done
// here.
//
bool
cExt::setupCommand(MenuEnt *ent, bool *retval, bool *call_on_up)
{
    *retval = true;
    *call_on_up = false;

    // These commands are CMD_UNSAFE, but have an initial pop-up that
    // we want to pop-down from the extract menu, meaning that we need
    // to handle the button-up events.

    if (!strcmp(ent->entry, MenuSOURC)) {
        *call_on_up = true;
        return (true);
    }
    if (!strcmp(ent->entry, MenuEXSET)) {
        *call_on_up = true;
        return (true);
    }
    if (!strcmp(ent->entry, MenuPNET)) {
        *call_on_up = true;
        return (true);
    }
    if (!strcmp(ent->entry, MenuENET)) {
        *call_on_up = true;
        return (true);
    }
    if (!strcmp(ent->entry, MenuLVS)) {
        *call_on_up = true;
        return (true);
    }

    return (false);
}


//-----------------------------------------------------------------------------
// The EXCFG command.
//
void
ext_menu::M_Config(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        EX()->PopUpExtSetup(cmd->caller, MODE_ON);
    else
        EX()->PopUpExtSetup(cmd->caller, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The EXSEL command.
//
void
ext_menu::M_ExtSel(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        EX()->PopUpSelections(cmd->caller, MODE_ON);
    else
        EX()->PopUpSelections(cmd->caller, MODE_OFF);
    SCD()->PopUpNodeMap(0, MODE_UPD);
}


//-----------------------------------------------------------------------------
// The DVSEL command.
//
void
ext_menu::M_DevSelect(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        EX()->PopUpDevices(cmd->caller, MODE_ON);
    else
        EX()->PopUpDevices(cmd->caller, MODE_OFF);
}


//-----------------------------------------------------------------------------
//The SOURC command.
//
// The source button pops up a window which enables setting some defaults,
// as well as the file to extract from.  Pressing the Go button extracts
// electrical info from the given file.

namespace {
    namespace ext_menu {
        struct SOURCcmd
        {
            void source_exec(CmdDesc*);

        private:
            static bool sourc_cb(const char*, void*, bool, const char*,
                int, int);

            static sExtCmdBtn sourc_btns[];
            static sExtCmd sourc_cmd;
        };

        SOURCcmd SRC;
    }
}

sExtCmdBtn SOURCcmd::sourc_btns[] =
{
    sExtCmdBtn("all_devs", VA_SourceAllDevs,     0, 0, 0, 0),
    sExtCmdBtn("create",   VA_SourceCreate,      1, 0, 0, 0),
    sExtCmdBtn("clear",    VA_SourceClear,       2, 0, 0, 0)
};

sExtCmd SOURCcmd::sourc_cmd(ExtSource, "Enter name of SPICE file to source",
    0, "Go", "Source SPICE File", "Select Options", false, 3, sourc_btns);


void
ext_menu::M_Source(CmdDesc *cmd)
{
    SRC.source_exec(cmd);
}


void
SOURCcmd::source_exec(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurCell(false, true, Physical))
        return;
    if (!XM()->CheckCurCell(true, true, Electrical))
        return;
    ds.clear();

    sourc_cmd.button(0)->set_active(CDvdb()->getVariable(VA_SourceAllDevs));
    sourc_cmd.button(1)->set_active(CDvdb()->getVariable(VA_SourceCreate));
    sourc_cmd.button(2)->set_active(CDvdb()->getVariable(VA_SourceClear));

    if (!cmd || !cmd->caller)
        return;
    EX()->PopUpExtCmd(cmd->caller, MODE_ON, &sourc_cmd, &sourc_cb, 0);
}


// Callback from source pop-up.
//
bool
SOURCcmd::sourc_cb(const char *name, void*, bool state, const char *fpath,
    int, int)
{
    if (name) {
        if (!strcmp(name, sourc_cmd.gotext())) {
            char *fname = pathlist::expand_path(fpath, false, true);
            FILE *fp = filestat::open_file(fname, "r");
            if (!fp) {
                Log()->ErrorLog(mh::Initialization, filestat::error_msg());
                delete [] fname;
                return (true);
            }

            int mode = 0;
            if (sourc_cmd.button(0)->is_active())
                mode |= EFS_ALLDEVS;
            if (sourc_cmd.button(1)->is_active())
                mode |= EFS_CREATE;
            if (sourc_cmd.button(2)->is_active())
                mode |= EFS_CLEAR;

            EV()->InitCallback();
            CDs *cursde = CurCell(Electrical, true);
            if (!cursde)
                return (false);
            SCD()->extractFromSpice(cursde, fp, mode);
            PL()->ShowPrompt("Done.");
            fclose(fp);
            delete [] fname;
            return (false);
        }
        else if (!strcmp(name, sourc_cmd.button(0)->name())) {
            sourc_cmd.button(0)->set_active(state);
            if (state)
                CDvdb()->setVariable(VA_SourceAllDevs, 0);
            else
                CDvdb()->clearVariable(VA_SourceAllDevs);
        }
        else if (!strcmp(name, sourc_cmd.button(1)->name())) {
            sourc_cmd.button(1)->set_active(state);
            if (state)
                CDvdb()->setVariable(VA_SourceCreate, 0);
            else
                CDvdb()->clearVariable(VA_SourceCreate);
        }
        else if (!strcmp(name, sourc_cmd.button(2)->name())) {
            sourc_cmd.button(2)->set_active(state);
            if (state)
                CDvdb()->setVariable(VA_SourceClear, 0);
            else
                CDvdb()->clearVariable(VA_SourceClear);
        }
    }
    return (true);
}


//-----------------------------------------------------------------------------
// The EXSET command.
//
// The exset button pops up a window which enables setting some defaults.
// Pressing the Go button will extract exectrical info from the physical
// layout.

namespace {
    namespace ext_menu {
        struct EXSETcmd
        {
            void exset_exec(CmdDesc*);

        private:
            static bool exset_cb(const char*, void*, bool, const char*,
                int, int);

            static sExtCmdBtn exset_btns[];
            static sExtCmd exset_cmd;
        };

        EXSETcmd EXSET;
    }
}

sExtCmdBtn EXSETcmd::exset_btns[] =
{
    sExtCmdBtn("all_devs",           VA_NoExsetAllDevs,      0, 0, 0, 0),
    sExtCmdBtn("create",             VA_NoExsetCreate,       1, 0, 0, 0),
    sExtCmdBtn("clear",              VA_ExsetClear,          2, 0, 0, 0),
    sExtCmdBtn("include wire cap",   VA_ExsetIncludeWireCap, 0, 1, 0, 0),
    sExtCmdBtn("ignore labels",      VA_ExsetNoLabels,       1, 1, 0, 0)
};

sExtCmd EXSETcmd::exset_cmd(ExtSet, 0, 0, "Go", "Source Physical",
    "Select Options", true, 5, exset_btns);


void
ext_menu::M_Exset(CmdDesc *cmd)
{
    EXSET.exset_exec(cmd);
}


void
EXSETcmd::exset_exec(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurCell(false, false, Physical))
        return;
    if (!XM()->CheckCurCell(true, true, Electrical))
        return;

    CDs *cursdp = CurCell(Physical);
    if (cursdp->isEmpty()) {
        PL()->ShowPrompt("Current cell contains no physical data!");
        return;
    }
    if (!EX()->associate(cursdp)) {
        PL()->ShowPrompt("Association failed!");
        return;
    }
    ds.clear();

    exset_cmd.button(0)->set_active(!CDvdb()->getVariable(VA_NoExsetAllDevs));
    exset_cmd.button(1)->set_active(!CDvdb()->getVariable(VA_NoExsetCreate));
    exset_cmd.button(2)->set_active(CDvdb()->getVariable(VA_ExsetClear));
    exset_cmd.button(3)->set_active(CDvdb()->getVariable(VA_ExsetIncludeWireCap));
    exset_cmd.button(4)->set_active(CDvdb()->getVariable(VA_ExsetNoLabels));

    if (!cmd || !cmd->caller)
        return;
    static int depth;
    EX()->PopUpExtCmd(cmd->caller, MODE_ON, &exset_cmd, &exset_cb,
        &depth, depth);
}


// Callback from exset pop-up.
//
bool
EXSETcmd::exset_cb(const char *name, void *arg, bool state, const char *fpath,
    int, int)
{
    if (name) {
        int *depth = (int*)arg;
        if (!depth)
            return (false);
        if (!strcmp(name, "depth")) {
            if (fpath) {
                if (isdigit(*fpath))
                    *depth = *fpath - '0';
                else if (*fpath == 'a')
                    *depth = CDMAXCALLDEPTH;
            }
            return (true);
        }
        if (!strcmp(name, exset_cmd.gotext())) {
            // 'go' button
            if (!DSP()->CurCellName())
                return (false);
            CDs *cursdp = CurCell(Physical);
            if (!cursdp)
                return (false);

            int mode = 0;
            if (exset_cmd.button(0)->is_active())
                mode |= EFS_ALLDEVS;
            if (exset_cmd.button(1)->is_active())
                mode |= EFS_CREATE;
            if (exset_cmd.button(2)->is_active())
                mode |= EFS_CLEAR;
            if (exset_cmd.button(3)->is_active())
                mode |= EFS_WIRECAP;
            if (exset_cmd.button(4)->is_active())
                mode |= MEL_NOLABELS;

            EV()->InitCallback();
            bool ret = EX()->makeElec(cursdp, *depth, mode);
            if (!ret)
                Log()->ErrorLog(mh::Processing,
                    "Extract failed: can not open temporary file?");
            PL()->ShowPrompt("Done.");
            return (false);
        }
        else if (!strcmp(name, exset_cmd.button(0)->name())) {
            exset_cmd.button(0)->set_active(state);
            if (state)
                CDvdb()->clearVariable(VA_NoExsetAllDevs);
            else
                CDvdb()->setVariable(VA_NoExsetAllDevs, 0);
        }
        else if (!strcmp(name, exset_cmd.button(1)->name())) {
            exset_cmd.button(1)->set_active(state);
            if (state)
                CDvdb()->clearVariable(VA_NoExsetCreate);
            else
                CDvdb()->setVariable(VA_NoExsetCreate, 0);
        }
        else if (!strcmp(name, exset_cmd.button(2)->name())) {
            exset_cmd.button(2)->set_active(state);
            if (state)
                CDvdb()->setVariable(VA_ExsetClear, 0);
            else
                CDvdb()->clearVariable(VA_ExsetClear);
        }
        else if (!strcmp(name, exset_cmd.button(3)->name())) {
            exset_cmd.button(3)->set_active(state);
            if (state)
                CDvdb()->setVariable(VA_ExsetIncludeWireCap, 0);
            else
                CDvdb()->clearVariable(VA_ExsetIncludeWireCap);
        }
        else if (!strcmp(name, exset_cmd.button(4)->name())) {
            exset_cmd.button(4)->set_active(state);
            if (state)
                CDvdb()->setVariable(VA_ExsetNoLabels, 0);
            else
                CDvdb()->clearVariable(VA_ExsetNoLabels);
        }
    }
    return (true);
}


//-----------------------------------------------------------------------------
// The PNET command.
//
void
ext_menu::M_DumpPhysNet(CmdDesc *cmd)
{
    EX()->dumpPhysNetExec(cmd);
}


//-----------------------------------------------------------------------------
// The ENET command.
//
void
ext_menu::M_DumpElecNet(CmdDesc *cmd)
{
    EX()->dumpElecNetExec(cmd);
}


//-----------------------------------------------------------------------------
// The LVS command.
//
void
ext_menu::M_LVS(CmdDesc *cmd)
{
    EX()->lvsExec(cmd);
}


//-----------------------------------------------------------------------------
// The EXC command.
//
// This is the new graphical front-end for Fast[er]Cap.
//
void
ext_menu::M_ExtractC(CmdDesc *cmd)
{
    if (cmd && !Menu()->GetStatus(cmd->caller)) {
        FC()->PopUpExtIf(0, MODE_OFF);
        return;
    }
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Physical))
        return;
    if (!XM()->CheckCurCell(false, false, Physical))
        return;
    ds.clear();

    FC()->PopUpExtIf(cmd ? cmd->caller : 0, MODE_ON);
}


//-----------------------------------------------------------------------------
// The EXLP command.
//
// This is the graphical front-end for FastCap/Fasthenry.
//
void
ext_menu::M_ExtractLR(CmdDesc *cmd)
{
    if (cmd && !Menu()->GetStatus(cmd->caller)) {
        FH()->PopUpExtIf(0, MODE_OFF);
        return;
    }
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Physical))
        return;
    if (!XM()->CheckCurCell(false, false, Physical))
        return;
    ds.clear();

    FH()->PopUpExtIf(cmd ? cmd->caller : 0, MODE_ON);
}

