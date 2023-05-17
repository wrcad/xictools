
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
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "events.h"
#include "promptline.h"
#include "errorlog.h"
#include "menu.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"


// button text, used for PNET and ENET
opt_atom opt_atom_all = (opt_atom)"list all cells";
opt_atom opt_atom_btmup = (opt_atom)"list bottom-up";
opt_atom opt_atom_net = (opt_atom)"net";
opt_atom opt_atom_devs = (opt_atom)"devs";
opt_atom opt_atom_spice = (opt_atom)"spice";
opt_atom opt_atom_geom = (opt_atom)"include geometry";
opt_atom opt_atom_cap = (opt_atom)"show wire cap";
opt_atom opt_atom_labels = (opt_atom)"ignore labels";
opt_atom opt_atom_verbose = (opt_atom)"devs verbose";


//-----------------------------------------------------------------------------
// The PNET command.
//
// Dump a netlist derived from physical data.

namespace {
    struct PNETcmd
    {
        void init(CmdDesc*);
        sDumpOpts *pnet_new_opts(const char*, const char*);

    private:
        int pnet_build_preset(sExtCmdBtn*);
        sDumpOpts *pnet_init_btns();

        static void p_cb(bool, void*);
        static bool pnet_cb(const char*, void*, bool, const char*, int, int);

        sDumpOpts *pnet_opts;

        static sExtCmd pnet_cmd;
    };

    PNETcmd PNET;
}

sExtCmd PNETcmd::pnet_cmd(ExtDumpPhys, "Enter name of file to create",
    0, "Go", "Dump Phys Netlist", "Select Formats", true, 0, 0);


void
cExt::dumpPhysNetExec(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurCell(false, false, Physical))
        return;
    if (CurCell(Physical)->isEmpty()) {
        PL()->ShowPrompt("Physical part of current cell is empty.");
        return;
    }
    if (!EX()->associate(CurCell(Physical))) {
        PL()->ShowPrompt("Association failed!");
        return;
    }
    PNET.init(cmd);
    ds.clear();
}


// Export to support DumpPhysNetlist script function.
//
sDumpOpts *
cExt::pnetNetOpts(const char *optstr, const char *list)
{
    return (PNET.pnet_new_opts(optstr, list));
}


namespace {
    void
    PNETcmd::init(CmdDesc *cmd)
    {
        if (!cmd || !cmd->caller)
            return;
        sDumpOpts *opts = pnet_init_btns();
        for (int i = 0; i < opts->num_buttons(); i++) {
            if (opts->button(i)->atom() == opt_atom_all)
                opts->button(i)->set(CDvdb()->getVariable(VA_PnetListAll));
            else if (opts->button(i)->atom() == opt_atom_btmup)
                opts->button(i)->set(CDvdb()->getVariable(VA_PnetBottomUp));
            else if (opts->button(i)->atom() == opt_atom_net)
                opts->button(i)->set(CDvdb()->getVariable(VA_PnetNet));
            else if (opts->button(i)->atom() == opt_atom_devs)
                opts->button(i)->set(CDvdb()->getVariable(VA_PnetDevs));
            else if (opts->button(i)->atom() == opt_atom_spice)
                opts->button(i)->set(CDvdb()->getVariable(VA_PnetSpice));
            else if (opts->button(i)->atom() == opt_atom_geom)
                opts->button(i)->set(CDvdb()->getVariable(VA_PnetShowGeometry));
            else if (opts->button(i)->atom() == opt_atom_cap)
                opts->button(i)->set(CDvdb()->getVariable(VA_PnetIncludeWireCap));
            else if (opts->button(i)->atom() == opt_atom_labels)
                opts->button(i)->set(CDvdb()->getVariable(VA_PnetNoLabels));
            else if (opts->button(i)->atom() == opt_atom_verbose)
                opts->button(i)->set(CDvdb()->getVariable(VA_PnetVerbose));
            opts->button(i)->set_active(opts->button(i)->is_set());
        }

        char fname[256];
        snprintf(fname, sizeof(fname), "%s.physnet",
            Tstring(DSP()->CurCellName()));
        delete [] pnet_cmd.filename();
        pnet_cmd.set_filename(lstring::copy(fname));

        EX()->PopUpExtCmd(cmd->caller, MODE_ON, &pnet_cmd, &pnet_cb,
            opts, opts->depth());
    }


    sDumpOpts *
    PNETcmd::pnet_new_opts(const char *optstr, const char *list)
    {
        int ucnt = 0;
        if (list && *list) {
            const char *s = list;
            while (lstring::advtok(&s))
                ucnt++;
        }

        int len = ucnt + pnet_build_preset(0);
        sExtCmdBtn *btns = new sExtCmdBtn[len];
        int i = pnet_build_preset(btns);
        int user_start = i;

        if (list && *list) {
            const char *s = list;
            char *name = lstring::getqtok(&s);
            for (int k = 0; k < user_start; k++) {
                if (!strcmp(name, btns[k].name())) {
                    delete [] name;
                    name = 0;
                    btns[k].set(true);
                    break;
                }
            }
            if (name) {
                btns[i] = sExtCmdBtn(name, 0, 0, 0, 0, false);
                btns[i++].set(true);
            }
        }

        for (int k = 0; k < user_start; k++) {
            if (btns[k].atom() == opt_atom_all) {
                if (strchr(optstr, 'a'))
                    btns[k].set(true);
                else if (strchr(optstr, 'A'))
                    btns[k].set(CDvdb()->getVariable(VA_PnetListAll));
            }
            else if (btns[k].atom() == opt_atom_btmup) {
                if (strchr(optstr, 'b'))
                    btns[k].set(true);
                else if (strchr(optstr, 'B'))
                    btns[k].set(CDvdb()->getVariable(VA_PnetBottomUp));
            }
            else if (btns[k].atom() == opt_atom_net) {
                if (strchr(optstr, 'n'))
                    btns[k].set(true);
                else if (strchr(optstr, 'N'))
                    btns[k].set(!CDvdb()->getVariable(VA_PnetNet));
            }
            else if (btns[k].atom() == opt_atom_devs) {
                if (strchr(optstr, 'd'))
                    btns[k].set(true);
                else if (strchr(optstr, 'D'))
                    btns[k].set(!CDvdb()->getVariable(VA_PnetDevs));
            }
            else if (btns[k].atom() == opt_atom_spice) {
                if (strchr(optstr, 's'))
                    btns[k].set(true);
                else if (strchr(optstr, 'S'))
                    btns[k].set(!CDvdb()->getVariable(VA_PnetSpice));
            }
            else if (btns[k].atom() == opt_atom_geom) {
                if (strchr(optstr, 'g'))
                    btns[k].set(true);
                else if (strchr(optstr, 'G'))
                    btns[k].set(CDvdb()->getVariable(VA_PnetShowGeometry));
            }
            else if (btns[k].atom() == opt_atom_cap) {
                if (strchr(optstr, 'c'))
                    btns[k].set(true);
                else if (strchr(optstr, 'C'))
                    btns[k].set(CDvdb()->getVariable(VA_PnetIncludeWireCap));
            }
            else if (btns[k].atom() == opt_atom_labels) {
                if (strchr(optstr, 'l'))
                    btns[k].set(true);
                else if (strchr(optstr, 'L'))
                    btns[k].set(CDvdb()->getVariable(VA_PnetNoLabels));
            }
            else if (btns[k].atom() == opt_atom_verbose) {
                if (strchr(optstr, 'v'))
                    btns[k].set(true);
                else if (strchr(optstr, 'V'))
                    btns[k].set(CDvdb()->getVariable(VA_PnetVerbose));
            }
        }

        sDumpOpts *opts = new sDumpOpts(btns, i, user_start);
        return (opts);
    }


    int
    PNETcmd::pnet_build_preset(sExtCmdBtn *btns)
    {
        int sns[EXT_BSENS];
        memset(sns, 0, EXT_BSENS*sizeof(int));
        if (!btns)
            return (9);  // PRESET 9 BUTTONS
        int i = 0;
        btns[i++] = sExtCmdBtn(
            opt_atom_all, VA_PnetListAll, 0, 0, 0, false);
        btns[i++] = sExtCmdBtn(
            opt_atom_btmup, VA_PnetBottomUp, 1, 0, 0, false);
        btns[i++] = sExtCmdBtn(
            opt_atom_net, VA_PnetNet, 0, 1, 0, false);
        btns[i++] = sExtCmdBtn(
            opt_atom_devs, VA_PnetDevs, 0, 2, 0, false);
        btns[i++] = sExtCmdBtn(
            opt_atom_spice, VA_PnetSpice, 0, 3, 0, false);
        sns[0] = 3;
        sns[1] = 10;  // Map sens to all user buttons.
        sns[2] = 11;
        sns[3] = 12;
        sns[4] = 13;
        btns[i++] = sExtCmdBtn(
            opt_atom_geom, VA_PnetShowGeometry, 1, 1, sns, false);
        sns[0] = 5;
        sns[1] = 0;
        sns[2] = 0;
        sns[3] = 0;
        sns[4] = 0;
        btns[i++] = sExtCmdBtn(
            opt_atom_cap, VA_PnetIncludeWireCap, 1, 3, sns, false);
        btns[i++] = sExtCmdBtn(
            opt_atom_labels, VA_PnetNoLabels, 1, 2, 0, false);
        sns[0] = 4;
        btns[i++] = sExtCmdBtn(
            opt_atom_verbose, VA_PnetVerbose, 1, 4, sns, false);
        return (i);
    }


    sDumpOpts *
    PNETcmd::pnet_init_btns()
    {
        if (pnet_cmd.btns())
            return (pnet_opts);
        SymTab *ftab = XM()->GetFormatFuncTab(EX_PNET_FORMAT);
        if (!ftab) {
            XM()->OpenFormatLib(EX_PNET_FORMAT);
            ftab = XM()->GetFormatFuncTab(EX_PNET_FORMAT);
        }
        stringlist *names = 0;
        if (ftab) {
            names = SymTab::names(ftab);
            stringlist::sort(names);
        }
        int len = stringlist::length(names) + pnet_build_preset(0);
        sExtCmdBtn *btns = new sExtCmdBtn[len];
        int i = pnet_build_preset(btns);
        int user_start = i;
        int r = 0;
        for (int k = 0; k < i; k++) {
            if (r < btns[k].row())
                r = btns[k].row();
        }
        for (stringlist *s = names; s; s = s->next) {
            r++;
            btns[i] = sExtCmdBtn((opt_atom)s->string, 0, 0, r, 0, false);
            s->string = 0;
            i++;
        }
        stringlist::destroy(names);

        pnet_cmd.set_btns(btns, i);
        pnet_opts = new sDumpOpts(btns, i, user_start);
        return (pnet_opts);
    }


    // Static function.
    void
    PNETcmd::p_cb(bool ok, void *arg)
    {
        char *fname = (char*)arg;
        if (ok)
            DSPmainWbag(PopUpFileBrowser(fname))
        delete [] fname;
    }


    // Static function.
    bool
    PNETcmd::pnet_cb(const char *name, void *arg, bool state,
        const char *fpath, int x, int y)
    {
        if (name) {
            sDumpOpts *opts = (sDumpOpts*)arg;
            if (!opts)
                return (false);
            if (!strcmp(name, "depth")) {
                if (fpath) {
                    if (isdigit(*fpath))
                        opts->set_depth(*fpath - '0');
                    else if (*fpath == 'a')
                        opts->set_depth(CDMAXCALLDEPTH);
                }
                return (true);
            }
            if (!strcmp(name, pnet_cmd.gotext())) {
                // 'go' button

                if (!DSP()->CurCellName()) {
                    Log()->PopUpErr("No current cell!");
                    return (false);
                }
                CDs *cursdp = CurCell(Physical);
                if (!cursdp)
                    return (false);
                char *fname = pathlist::expand_path(fpath, false, true);

                if (!filestat::create_bak(fname)) {
                    Log()->ErrorLogV(mh::Initialization,
                        filestat::error_msg());
                    delete [] fname;
                    return (true);
                }
                FILE *fp = filestat::open_file(fname, "w");
                if (!fp) {
                    Log()->ErrorLogV(mh::Initialization,
                        filestat::error_msg());
                    delete [] fname;
                    return (true);
                }
                EV()->InitCallback();
                opts->set_spice_print_mode(PSPM_physical);
                if (EX()->dumpPhysNetlist(fp, cursdp, opts)) {
                    char *fn = lstring::strip_path(fname);
                    char tbuf[256];
                    snprintf(tbuf, sizeof(tbuf),
                        "Physical netlist is in file %s.  View file? ", fn);
                    DSPmainWbag(PopUpAffirm(0, GRloc(LW_XYA, x, y), tbuf, p_cb,
                        lstring::copy(fname)))
                }
                fclose(fp);
                PL()->ShowPrompt("Done.");
                delete [] fname;
                return (false);
            }
            else {
                for (int i = 0; i < pnet_cmd.num_buttons(); i++) {
                    if (strcmp(name, pnet_cmd.button(i)->name()))
                        continue;

                    opts->button(i)->set(state);
                    if (pnet_cmd.button(i)->atom() == opt_atom_all) {
                        if (state)
                            CDvdb()->setVariable(VA_PnetListAll, 0);
                        else
                            CDvdb()->clearVariable(VA_PnetListAll);
                    }
                    else if (pnet_cmd.button(i)->atom() == opt_atom_btmup) {
                        if (state)
                            CDvdb()->setVariable(VA_PnetBottomUp, 0);
                        else
                            CDvdb()->clearVariable(VA_PnetBottomUp);
                    }
                    else if (pnet_cmd.button(i)->atom() == opt_atom_net) {
                        if (state)
                            CDvdb()->setVariable(VA_PnetNet, 0);
                        else
                            CDvdb()->clearVariable(VA_PnetNet);
                    }
                    else if (pnet_cmd.button(i)->atom() == opt_atom_devs) {
                        if (state)
                            CDvdb()->setVariable(VA_PnetDevs, 0);
                        else
                            CDvdb()->clearVariable(VA_PnetDevs);
                    }
                    else if (pnet_cmd.button(i)->atom() == opt_atom_spice) {
                        if (state)
                            CDvdb()->setVariable(VA_PnetSpice, 0);
                        else
                            CDvdb()->clearVariable(VA_PnetSpice);
                    }
                    else if (pnet_cmd.button(i)->atom() == opt_atom_geom) {
                        if (state)
                            CDvdb()->setVariable(VA_PnetShowGeometry, 0);
                        else
                            CDvdb()->clearVariable(VA_PnetShowGeometry);
                    }
                    else if (pnet_cmd.button(i)->atom() == opt_atom_cap) {
                        if (state)
                            CDvdb()->setVariable(VA_PnetIncludeWireCap, 0);
                        else
                            CDvdb()->clearVariable(VA_PnetIncludeWireCap);
                    }
                    else if (pnet_cmd.button(i)->atom() == opt_atom_labels) {
                        if (state)
                            CDvdb()->setVariable(VA_PnetNoLabels, 0);
                        else
                            CDvdb()->clearVariable(VA_PnetNoLabels);
                    }
                    else if (pnet_cmd.button(i)->atom() == opt_atom_verbose) {
                        if (state)
                            CDvdb()->setVariable(VA_PnetVerbose, 0);
                        else
                            CDvdb()->clearVariable(VA_PnetVerbose);
                    }
                    break;
                }
            }
        }
        return (true);
    }
}


//-----------------------------------------------------------------------------
// The ENET command.
//
// Dump a netlist derived from electrical data.

namespace {
    struct ENETcmd
    {
        void init(CmdDesc*);
        sDumpOpts *enet_new_opts(const char*, const char*);

    private:
        int enet_build_preset(sExtCmdBtn*);
        sDumpOpts *enet_init_btns();
        static void e_cb(bool, void*);
        static bool enet_cb(const char*, void*, bool, const char*, int, int);

        sDumpOpts *enet_opts;

        static sExtCmd enet_cmd;
    };

    ENETcmd ENET;
}

sExtCmd ENETcmd::enet_cmd(ExtDumpElec, "Enter name of file to create",
    0, "Go", "Dump Elec Netlist", "Select Formats", true, 0, 0);


void
cExt::dumpElecNetExec(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurCell(false, false, Electrical))
        return;
    if (CurCell(Electrical)->isEmpty()) {
        PL()->ShowPrompt("Electrical part of current cell is empty.");
        return;
    }
    ENET.init(cmd);
    ds.clear();
}


// Export to support DumpElecNetlist script function.
//
sDumpOpts *
cExt::enetNewOpts(const char *optstr, const char *list)
{
    return (ENET.enet_new_opts(optstr, list));
}


namespace {
    void
    ENETcmd::init(CmdDesc *cmd)
    {
        if (!cmd || !cmd->caller)
            return;
        sDumpOpts *opts = enet_init_btns();
        for (int i = 0; i < opts->num_buttons(); i++) {
            if (opts->button(i)->atom() == opt_atom_btmup)
                opts->button(i)->set(CDvdb()->getVariable(VA_EnetBottomUp));
            else if (opts->button(i)->atom() == opt_atom_net)
                opts->button(i)->set(CDvdb()->getVariable(VA_EnetNet));
            else if (opts->button(i)->atom() == opt_atom_spice)
                opts->button(i)->set(CDvdb()->getVariable(VA_EnetSpice));
            opts->button(i)->set_active(opts->button(i)->is_set());
        }

        char fname[256];
        snprintf(fname, sizeof(fname), "%s.elecnet",
            Tstring(DSP()->CurCellName()));
        delete [] enet_cmd.filename();
        enet_cmd.set_filename(lstring::copy(fname));

        EX()->PopUpExtCmd(cmd->caller, MODE_ON, &enet_cmd, enet_cb,
            opts, opts->depth());
    }


    sDumpOpts *
    ENETcmd::enet_new_opts(const char *optstr, const char *list)
    {
        int ucnt = 0;
        if (list && *list) {
            const char *s = list;
            while (lstring::advtok(&s))
                ucnt++;
        }

        int len = ucnt + enet_build_preset(0);
        sExtCmdBtn *btns = new sExtCmdBtn[len];
        int i = enet_build_preset(btns);
        int user_start = i;

        if (list && *list) {
            const char *s = list;
            char *name = lstring::getqtok(&s);
            for (int k = 0; k < user_start; k++) {
                if (!strcmp(name, btns[k].name())) {
                    delete [] name;
                    name = 0;
                    btns[k].set(true);
                    break;
                }
            }
            if (name) {
                btns[i] = sExtCmdBtn(name, 0, 0, 0, 0, false);
                btns[i++].set(true);
            }
        }

        for (int k = 0; k < user_start; k++) {
            if (btns[k].atom() == opt_atom_btmup) {
                if (strchr(optstr, 'b'))
                    btns[k].set(true);
                else if (strchr(optstr, 'B'))
                    btns[k].set(CDvdb()->getVariable(VA_EnetBottomUp));
            }
            else if (btns[k].atom() == opt_atom_net) {
                if (strchr(optstr, 'n'))
                    btns[k].set(true);
                else if (strchr(optstr, 'N'))
                    btns[k].set(CDvdb()->getVariable(VA_EnetNet));
            }
            else if (btns[k].atom() == opt_atom_spice) {
                if (strchr(optstr, 's'))
                    btns[k].set(true);
                else if (strchr(optstr, 'S'))
                    btns[k].set(CDvdb()->getVariable(VA_EnetSpice));
            }
        }

        sDumpOpts *opts = new sDumpOpts(btns, i, user_start);
        return (opts);
    }


    int
    ENETcmd::enet_build_preset(sExtCmdBtn *btns)
    {
        if (!btns)
            return (3);  // PRESET 3 BUTTONS
        int i = 0;
        btns[i++] = sExtCmdBtn(
            opt_atom_btmup, VA_EnetBottomUp, 1, 0, 0, false);
        btns[i++] = sExtCmdBtn(
            opt_atom_net, VA_EnetNet, 0, 0, 0, false);
        btns[i++] = sExtCmdBtn(
            opt_atom_spice, VA_EnetSpice, 0, 1, 0, false);
        return (i);
    }


    sDumpOpts *
    ENETcmd::enet_init_btns()
    {
        if (enet_cmd.btns())
            return (enet_opts);
        SymTab *ftab = XM()->GetFormatFuncTab(EX_ENET_FORMAT);
        if (!ftab) {
            XM()->OpenFormatLib(EX_ENET_FORMAT);
            ftab = XM()->GetFormatFuncTab(EX_ENET_FORMAT);
        }
        stringlist *names = 0;
        if (ftab) {
            names = SymTab::names(ftab);
            stringlist::sort(names);
        }
        int len = stringlist::length(names) + enet_build_preset(0);
        sExtCmdBtn *btns = new sExtCmdBtn[len];
        int i = enet_build_preset(btns);
        int user_start = i;
        int r = 0;
        for (int k = 0; k < i; k++) {
            if (r < btns[k].row())
                r = btns[k].row();
        }
        for (stringlist *s = names; s; s = s->next) {
            r++;
            btns[i] = sExtCmdBtn((opt_atom)s->string, 0, 0, r, 0, false);
            s->string = 0;
            i++;
        }
        stringlist::destroy(names);

        enet_cmd.set_btns(btns, i);
        enet_opts = new sDumpOpts(btns, i, user_start);
        return (enet_opts);
    }


    // Static function.
    void
    ENETcmd::e_cb(bool ok, void *arg)
    {
        char *fname = (char*)arg;
        if (ok)
            DSPmainWbag(PopUpFileBrowser(fname))
        delete [] fname;
    }


    // Static function.
    bool
    ENETcmd::enet_cb(const char *name, void *arg, bool state,
        const char *fpath, int x, int y)
    {
        if (name) {
            sDumpOpts *opts = (sDumpOpts*)arg;
            if (!opts)
                return (false);
            if (!strcmp(name, "depth")) {
                if (fpath) {
                    if (isdigit(*fpath))
                        opts->set_depth(*fpath - '0');
                    else if (*fpath == 'a')
                        opts->set_depth(CDMAXCALLDEPTH);
                }
                return (true);
            }
            if (!strcmp(name, enet_cmd.gotext())) {
                // 'go' button

                if (!DSP()->CurCellName()) {
                    Log()->PopUpErr("No current cell!");
                    return (false);
                }
                CDs *cursde = CurCell(Electrical, true);
                if (!cursde)
                    return (false);
                char *fname = pathlist::expand_path(fpath, false, true);

                if (!filestat::create_bak(fname)) {
                    Log()->ErrorLogV(mh::Initialization,
                        filestat::error_msg());
                    delete [] fname;
                    return (true);
                }
                FILE *fp = filestat::open_file(fname, "w");
                if (!fp) {
                    Log()->ErrorLogV(mh::Initialization,
                        filestat::error_msg());
                    delete [] fname;
                    return (true);
                }
                EV()->InitCallback();
                if (EX()->dumpElecNetlist(fp, cursde, opts)) {
                    char *fn = lstring::strip_path(fname);
                    char tbuf[128];
                    snprintf(tbuf, sizeof(tbuf),
                        "Electrical netlist is in file %s.  View file? ", fn);
                    DSPmainWbag(PopUpAffirm(0, GRloc(LW_XYA, x, y), tbuf, e_cb,
                        lstring::copy(fname)))
                }
                fclose(fp);
                PL()->ShowPrompt("Done.");
                delete [] fname;
                return (false);
            }
            else {
                for (int i = 0; i < enet_cmd.num_buttons(); i++) {
                    if (strcmp(name, enet_cmd.button(i)->name()))
                        continue;

                    opts->button(i)->set(state);
                    if (enet_cmd.button(i)->atom() == opt_atom_btmup) {
                        if (state)
                            CDvdb()->setVariable(VA_EnetBottomUp, 0);
                        else
                            CDvdb()->clearVariable(VA_EnetBottomUp);
                    }
                    else if (enet_cmd.button(i)->atom() == opt_atom_net) {
                        if (state)
                            CDvdb()->setVariable(VA_EnetNet, 0);
                        else
                            CDvdb()->clearVariable(VA_EnetNet);
                    }
                    else if (enet_cmd.button(i)->atom() == opt_atom_spice) {
                        if (state)
                            CDvdb()->setVariable(VA_EnetSpice, 0);
                        else
                            CDvdb()->clearVariable(VA_EnetSpice);
                    }
                    break;
                }
            }
        }
        return (true);
    }
}


//-----------------------------------------------------------------------------
// The LVS command.
//
// Perform and report comparison between physical and electrical data.

namespace {
    struct LVScmd
    {
        void init(CmdDesc*);

    private:
        sDumpOpts *lvs_init_btns();

        static void l_cb(bool, void*);
        static bool lvs_cb(const char*, void*, bool, const char*, int, int);

        sDumpOpts *lvs_opts;

        static sExtCmd lvs_cmd;
    };

    LVScmd LVS;

    opt_atom opt_atom_fail_noconnect =
        (opt_atom)"fail if unconnected physical subcells";
}

sExtCmd LVScmd::lvs_cmd(ExtLVS, "Enter name of file to create",
    0, "Go", "LVS", "Set Options", true, 0, 0);


void
cExt::lvsExec(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurCell(false, false, Physical))
        return;
    if (CurCell(Physical)->isEmpty()) {
        PL()->ShowPrompt("Physical part of current cell is empty.");
        return;
    }
    if (!XM()->CheckCurCell(false, false, Electrical))
        return;
    if (CurCell(Electrical)->isEmpty()) {
        PL()->ShowPrompt("Electrical part of current cell is empty.");
        return;
    }
    LVS.init(cmd);
    ds.clear();
}


namespace {
    void
    LVScmd::init(CmdDesc *cmd)
    {
        if (!cmd || !cmd->caller)
            return;

        sDumpOpts *opts = lvs_init_btns();
        for (int i = 0; i < opts->num_buttons(); i++) {
            if (opts->button(i)->atom() == opt_atom_fail_noconnect)
                opts->button(i)->set(
                    CDvdb()->getVariable(VA_LvsFailNoConnect));
        }

        char fname[256];
        snprintf(fname, sizeof(fname), "%s.lvs",
            Tstring(DSP()->CurCellName()));
        delete [] lvs_cmd.filename();
        lvs_cmd.set_filename(lstring::copy(fname));

        EX()->PopUpExtCmd(cmd->caller, MODE_ON, &lvs_cmd, lvs_cb, opts,
            opts->depth());
    }


    sDumpOpts *
    LVScmd::lvs_init_btns()
    {
        if (lvs_cmd.btns())
            return (lvs_opts);

        sExtCmdBtn *btns = new sExtCmdBtn[1];
        btns[0] = sExtCmdBtn(
            opt_atom_fail_noconnect, VA_LvsFailNoConnect, 0, 0, 0, false);

        lvs_cmd.set_btns(btns, 1);
        lvs_opts = new sDumpOpts(btns, 1, 1);
        return (lvs_opts);
    }


    // Static function.
    void
    LVScmd::l_cb(bool ok, void *arg)
    {
        char *fname = (char*)arg;
        if (ok)
            DSPmainWbag(PopUpFileBrowser(fname))
        delete [] fname;
    }


    // Static function.
    bool
    LVScmd::lvs_cb(const char *name, void *arg, bool state, const char *fpath,
        int x, int y)
    {
        if (name) {
            sDumpOpts *opts = (sDumpOpts*)arg;
            if (!opts)
                return (false);
            if (!strcmp(name, "depth")) {
                if (fpath) {
                    if (isdigit(*fpath))
                        opts->set_depth(*fpath - '0');
                    else if (*fpath == 'a')
                        opts->set_depth(CDMAXCALLDEPTH);
                }
                return (true);
            }
            if (!strcmp(name, lvs_cmd.gotext())) {
                // 'go' button

                if (!DSP()->CurCellName()) {
                    Log()->PopUpErr("No current cell!");
                    return (false);
                }

                char *fname = pathlist::expand_path(fpath, false, true);

                if (!filestat::create_bak(fname)) {
                    Log()->ErrorLogV(mh::Initialization,
                        filestat::error_msg());
                    delete [] fname;
                    return (true);
                }
                FILE *fp = filestat::open_file(fname, "w");
                if (!fp) {
                    Log()->ErrorLogV(mh::Initialization,
                        filestat::error_msg());
                    delete [] fname;
                    return (true);
                }

                EV()->InitCallback();
                DSPpkg::self()->SetWorking(true);
                PL()->ShowPrompt("Working...");
                char *fn = lstring::strip_path(fname);
                CDcbin cbin(DSP()->CurCellName());
                LVSresult ret = EX()->lvs(fp, &cbin, opts->depth());
                if (ret == LVSerror) {
                    PL()->ShowPrompt("LVS not run, error in association.");
                    delete [] fname;
                    fclose(fp);
                    DSPpkg::self()->SetWorking(false);
                    return (true);
                }

                char tbuf[128];
                if (ret == LVSclean)
                    snprintf(tbuf, sizeof(tbuf),
                        "LVS CLEAN.  View file %s? ", fn);
                else if (ret == LVSap)
                    snprintf(tbuf, sizeof(tbuf),
                        "LVS PASSED - AMBIGUITY.  View file %s? ", fn);
                else if (ret == LVStopok)
                    snprintf(tbuf, sizeof(tbuf),
                        "LVS PASSED - PARAM DIFFS.  View file %s? ", fn);
                else
                    snprintf(tbuf, sizeof(tbuf),
                        "LVS FAILED.  View file %s? ", fn);
                DSPmainWbag(PopUpAffirm(0, GRloc(LW_XYA, x, y), tbuf, l_cb,
                    lstring::copy(fname)))
                PL()->ErasePrompt();

                delete [] fname;
                fclose(fp);
                DSPpkg::self()->SetWorking(false);
                return (false);
            }
            else {
                for (int i = 0; i < lvs_cmd.num_buttons(); i++) {
                    if (strcmp(name, lvs_cmd.button(i)->name()))
                        continue;

                    opts->button(i)->set(state);
                    if (lvs_cmd.button(i)->atom() == opt_atom_fail_noconnect) {
                        if (state)
                            CDvdb()->setVariable(VA_LvsFailNoConnect, 0);
                        else
                            CDvdb()->clearVariable(VA_LvsFailNoConnect);
                    }
                }
            }
        }
        return (true);
    }
}

