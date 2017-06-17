
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: drc_menu.cc,v 5.83 2017/03/14 01:26:30 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "fio.h"
#include "drc.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_layer.h"
#include "cd_lgen.h"
#include "events.h"
#include "errorlog.h"
#include "promptline.h"
#include "select.h"
#include "ghost.h"
#include "menu.h"
#include "tech.h"
#include "drc_menu.h"
#include "filestat.h"
#include "pathlist.h"
#include "tvals.h"
#include "miscutil.h"
#include "timedbg.h"
#include "timer.h"

#include <ctype.h>
#include <dirent.h>
#ifdef WIN32
#include "msw.h"
#endif


namespace {
    namespace drc_menu {
        MenuFunc  M_DrcLimits;
        MenuFunc  M_DrcFlag;
        MenuFunc  M_DrcOn;
        MenuFunc  M_DrcNoPop;
        MenuFunc  M_DrcCheck;
        MenuFunc  M_DrcPoint;
        MenuFunc  M_DrcClear;
        MenuFunc  M_DrcQuery;
        MenuFunc  M_DrcDumpErrs;
        MenuFunc  M_DrcUpdate;
        MenuFunc  M_DrcShowErr;
        MenuFunc  M_DrcLayer;
        MenuFunc  M_DrcRules;
    }
}

using namespace drc_menu;

namespace {
    MenuEnt DrcMenu[drcMenu_END + 1];
    MenuBox DrcMenuBox;

    void
    drc_post_switch_proc(int, MenuEnt *menu)
    {
        if (!menu || !DSP()->MainWdesc())
            return;
        for (int i = 1; i < drcMenu_END; i++) {
            MenuEnt *ent = &menu[i];
            if (i == drcMenuIntr)
                ent->set_state(DRC()->isInteractive());
            else if (i == drcMenuNopop) {
                ent->set_state(DRC()->isIntrNoErrMsg());
                Menu()->SetSensitive(ent->cmd.caller,
                    DRC()->isInteractive());
            }
            else if (i == drcMenuLimit) {
                if (DSP()->CurMode() == Electrical) {
                    if (Menu()->MenuButtonStatus(MenuDRC, MenuLIMIT) == 1)
                        Menu()->MenuButtonPress(MenuDRC, MenuLIMIT);
                }
            }
            else if (i == drcMenuCheck) {
                if (DSP()->CurMode() == Electrical) {
                    if (Menu()->MenuButtonStatus(MenuDRC, MenuCHECK) == 1)
                        Menu()->MenuButtonPress(MenuDRC, MenuCHECK);
                }
            }
            else
                continue;
            Menu()->SetStatus(ent->cmd.caller, ent->is_set());
        }
    }
}


MenuBox *
cDRC::createMenu()
{
    DrcMenu[drcMenu] =
        MenuEnt(M_NoOp,     "",        ME_MENU,     CMD_SAFE,
        0);
    DrcMenu[drcMenuLimit] =
        MenuEnt(M_DrcLimits,MenuLIMIT, ME_TOGGLE,   CMD_SAFE,
        MenuLIMIT": Set limits and other parameters for DRC runs.");
    DrcMenu[drcMenuSflag] =
        MenuEnt(M_DrcFlag,  MenuSFLAG, ME_TOGGLE,   CMD_NOTSAFE,
        MenuSFLAG": Set skip-DRC flags in objects.");
    DrcMenu[drcMenuIntr] =
        MenuEnt(M_DrcOn,    MenuINTR,  ME_TOGGLE,   CMD_SAFE,
        MenuINTR": Enable interactive DRC testing.");
    DrcMenu[drcMenuNopop] =
        MenuEnt(M_DrcNoPop, MenuNOPOP, ME_TOGGLE | ME_SEP, CMD_SAFE,
        MenuNOPOP": Suppress pop-up errors listing in interactive DRC.");
    DrcMenu[drcMenuCheck] =
        MenuEnt(M_DrcCheck, MenuCHECK, ME_TOGGLE,   CMD_SAFE,
        MenuCHECK": Initiate batch DRC run.");
    DrcMenu[drcMenuPoint] =
        MenuEnt(M_DrcPoint, MenuPOINT, ME_TOGGLE | ME_SEP, CMD_NOTSAFE,
        MenuPOINT": Perform quick DRC test in an area.");
    DrcMenu[drcMenuClear] =
        MenuEnt(M_DrcClear, MenuCLEAR, ME_VANILLA,  CMD_SAFE,
        MenuCLEAR": Clear DRC errors displayed.");
    DrcMenu[drcMenuQuery] =
        MenuEnt(M_DrcQuery, MenuQUERY, ME_TOGGLE,   CMD_NOTSAFE,
        MenuQUERY": Enable DRC error explanations by clicking.");
    DrcMenu[drcMenuErdmp] =
        MenuEnt(M_DrcDumpErrs,MenuERDMP,ME_VANILLA, CMD_PROMPT,
        MenuERDMP": Dump highlighted errors to file.");
    DrcMenu[drcMenuErupd] =
        MenuEnt(M_DrcUpdate,MenuERUPD, ME_VANILLA,  CMD_PROMPT,
        MenuERUPD": Update error highlighting from file.");
    DrcMenu[drcMenuNext] =
        MenuEnt(M_DrcShowErr,MenuNEXT, ME_TOGGLE,   CMD_SAFE,
        MenuNEXT": Sequentially display DRC errors from file.");
    DrcMenu[drcMenuErlyr] =
        MenuEnt(M_DrcLayer, MenuERLYR, ME_VANILLA,  CMD_PROMPT,
        MenuERLYR": Create layer objects from highlighting.");
    DrcMenu[drcMenuDredt] =
        MenuEnt(M_DrcRules, MenuDREDT, ME_TOGGLE,   CMD_SAFE,
        MenuDREDT": Pop up the DRC rule editor.");
    DrcMenu[drcMenu_END] =
        MenuEnt();

    DrcMenuBox.name = "DRC";
    DrcMenuBox.menu = DrcMenu;
    DrcMenuBox.registerPostSwitchProc(&drc_post_switch_proc);
    return (&DrcMenuBox);
}


//-----------------------------------------------------------------------------
// The LIMITS command.
//
// Menu function to set certain limits for DRC.
//
void
drc_menu::M_DrcLimits(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        DRC()->PopUpDrcLimits(cmd->caller, MODE_ON);
    else
        DRC()->PopUpDrcLimits(0, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The SFLAG command.
//
// Command to allow setting/resetting of the CDnoDRC flag through
// the selection mechanism.
//
namespace {
    namespace drc_menu {
        struct FlagState : public CmdState
        {
            FlagState(const char*, const char*);
            virtual ~FlagState();

            void setCaller(GRobject c)  { Caller = c; }

            void b1down() { cEventHdlr::sel_b1down(); }
            void b1up();
            void esc();
            void undo() { cEventHdlr::sel_undo(); }
            void redo() { cEventHdlr::sel_redo(); }

        private:
            GRobject Caller;
        };

        FlagState *FlagCmd;
    }
}


void
drc_menu::M_DrcFlag(CmdDesc *cmd)
{
    if (FlagCmd) {
        FlagCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Physical))
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(false, false, Physical))
        return;

    FlagCmd = new FlagState("DRCFLAGS", "xic:sflag");
    FlagCmd->setCaller(cmd ? cmd->caller : 0);
    Selections.deselectTypes(CurCell(Physical), 0);
    CDl *ld;
    CDlgen gen(Physical);
    while ((ld = gen.next()) != 0) {
        CDg gdesc;
        gdesc.init_gen(CurCell(Physical), ld);
        CDo *pointer;
        while ((pointer = gdesc.next()) != 0) {
            if (!pointer->is_normal())
                continue;
            if (pointer->type() == CDBOX || pointer->type() == CDWIRE ||
                    pointer->type() == CDPOLYGON) {
                if (pointer->has_flag(CDnoDRC)) {
                    pointer->set_state(CDVanilla);
                    Selections.insertObject(CurCell(Physical), pointer);
                }
            }
        }
    }
    if (!EV()->PushCallback(FlagCmd)) {
        Selections.deselectTypes(CurCell(Physical), 0);
        delete FlagCmd;
        return;
    }
    PL()->ShowPrompt("Selected objects will be ignored in DRC.");
    ds.clear();
}


FlagState::FlagState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
}


FlagState::~FlagState()
{
    FlagCmd = 0;
}


// Process the selections.
//
void
FlagState::b1up()
{
    char types[4];
    types[0] = CDBOX;
    types[1] = CDWIRE;
    types[2] = CDPOLYGON;
    types[3] = '\0';
    CDol *stmp = 0;
    if (!cEventHdlr::sel_b1up(0, types, &stmp))
        return;
    for (CDol *s = stmp; s; s = s->next) {
        if (s->odesc->state() == CDSelected) {
            Selections.removeObject(CurCell(Physical), s->odesc);
            s->odesc->unset_flag(CDnoDRC);
        }
        else {
            s->odesc->set_state(CDVanilla);
            Selections.insertObject(CurCell(Physical), s->odesc);
            s->odesc->set_flag(CDnoDRC);
        }
    }
    stmp->free();
    return;
}


// Esc entered, clean up and abort.
//
void
FlagState::esc()
{
    cEventHdlr::sel_esc();
    Selections.deselectTypes(CurCell(Physical), 0);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


//-----------------------------------------------------------------------------
// The INTR command.
//
// Turn on/off interactive design rule checking.
//
void
drc_menu::M_DrcOn(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc)
        return;
    MenuEnt *ent = Menu()->FindEntOfWin(cmd->wdesc, MenuNOPOP);
    if (Menu()->GetStatus(cmd->caller)) {
        CDvdb()->setVariable(VA_Drc, "");
        if (ent && ent->cmd.caller)
            Menu()->SetSensitive(ent->cmd.caller, true);
        PL()->ShowPrompt("Will perform DRC when objects change.");
    }
    else {
        CDvdb()->clearVariable(VA_Drc);
        if (ent && ent->cmd.caller)
            Menu()->SetSensitive(ent->cmd.caller, false);
        PL()->ShowPrompt("Interactive DRC not active.");
    }
}


//-----------------------------------------------------------------------------
// The NOPOP command.
//
// Turn on/off the error listing popup in interactive mode.
//
void
drc_menu::M_DrcNoPop(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller)) {
        CDvdb()->setVariable(VA_DrcNoPopup, "");
        PL()->ShowPrompt("No pop-up list of interactive DRC error messages.");
    }
    else {
        CDvdb()->clearVariable(VA_DrcNoPopup);
        PL()->ShowPrompt(
            "Interactive DRC errors will be listed in a pop-up error window.");
    }
}


//-----------------------------------------------------------------------------
// The CHECK command.
//
// Bring up the DRC Run Control panel, from which foreground and
// backaground runs can be initiated.
//
void
drc_menu::M_DrcCheck(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        DRC()->PopUpDrcRun(cmd->caller, MODE_ON);
    else
        DRC()->PopUpDrcRun(0, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The POINT command.
//
// Run DRC in an area, interactively.

namespace {
    namespace drc_menu {

        struct DrcState : public CmdState
        {
            DrcState(const char*, const char*);
            virtual ~DrcState();

            void setCaller(GRobject c)  { Caller = c; }

            void b1down();
            void b1up();
            void esc();
            bool key(int, const char*, int);
            void undo() { cEventHdlr::sel_undo(); }
            void redo() { cEventHdlr::sel_redo(); }

            void message() { PL()->ShowPrompt(msg1p); }

        private:
            GRobject Caller;
            int State;
            int Lastx, Lasty;

            static const char *msg1p;
        };

        DrcState *DrcCmd;
    }
}

const char *DrcState::msg1p = "Point to endpoints of area to check.";


// Menu command for performing design rule checking on the current
// cell and its hierarchy.  The errors are printed on-screen, no
// file is generated.  The command will abort after DRC_PTMAX errors.
// The Enter key is ignored.  The command remains in effect until
// escaped.
//
void
drc_menu::M_DrcPoint(CmdDesc *cmd)
{
    if (DrcCmd) {
        DrcCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Physical))
        return;
    if (!XM()->CheckCurCell(false, false, Physical))
        return;

    PL()->ShowPrompt("Click on violations for explanation.");

    DrcCmd = new DrcState("DRC", "xic:point");
    DrcCmd->setCaller(cmd ? cmd->caller : 0);
    if (!EV()->PushCallback(DrcCmd)) {
        delete DrcCmd;
        return;
    }
    DrcCmd->message();
    ds.clear();
}


DrcState::DrcState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    Lastx = Lasty = 0;
    State = 0;
}


DrcState::~DrcState()
{
    DrcCmd = 0;
}


void
DrcState::b1down()
{
    cEventHdlr::sel_b1down();
}


// Drc the box, if the pointer has moved, and the box has area.
// Otherwise set the state for next button 1 press.
//
void
DrcState::b1up()
{
    BBox AOI;
    if (!cEventHdlr::sel_b1up(&AOI, 0, B1UP_NOSEL))
        return;
    DRC()->runDRCregion(&AOI);
    State = 0;
}


// Esc entered, clean up and abort.
//
void
DrcState::esc()
{
    cEventHdlr::sel_esc();
    if (State != 2)
        PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


bool
DrcState::key(int, const char*, int)
{
    return (false);
}
// End of DrcState methods


//-----------------------------------------------------------------------------
// The CLEAR command.
//
// Clear and erase the current drc errors.
//
void
drc_menu::M_DrcClear(CmdDesc *cmd)
{
    WindowDesc *wd;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wd = wgen.next()) != 0)
        DRC()->showCurError(wd, false);
    DRC()->clearCurError();
    if (cmd)
        Menu()->Deselect(cmd->caller);
}


//-----------------------------------------------------------------------------
// The QUERY command.
//
// Command to print the error message when the user clicks in an error
// region.
//
namespace {
    namespace drc_menu {
        struct QuState : public CmdState
        {
            QuState(const char*, const char*);
            virtual ~QuState();

            void setCaller(GRobject c)  { Caller = c; }

            void b1down() { cEventHdlr::sel_b1down(); }
            void b1up();
            void esc();
            void message()
                {
                    PL()->ShowPrompt(
                        "Click on error regions for explanation.");
                }

        private:
            GRobject Caller;
        };

        QuState *QuCmd;
    }
}


void
drc_menu::M_DrcQuery(CmdDesc *cmd)
{
    if (QuCmd) {
        QuCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Physical))
        return;
    if (!XM()->CheckCurCell(false, false, Physical))
        return;

    QuCmd = new QuState("DRCQUERY", "xic:query");
    QuCmd->setCaller(cmd ? cmd->caller : 0);
    if (!EV()->PushCallback(QuCmd)) {
        delete QuCmd;
        return;
    }
    QuCmd->message();
    ds.clear();
}


QuState::QuState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
}


QuState::~QuState()
{
    QuCmd = 0;
}


void
QuState::b1up()
{
    BBox AOI;
    if (!cEventHdlr::sel_b1up(&AOI, 0, B1UP_NOSEL))
        return;
    DRCerrList *el = DRC()->findError(&AOI);
    if (!el) {
        message();
        return;
    }
    if (el->message())
        PL()->ShowPrompt(el->message());
    else
        PL()->ShowPrompt("Unknown error, no message recorded.");
}


void
QuState::esc()
{
    cEventHdlr::sel_esc();
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    Menu()->Deselect(Caller);
    delete this;
}


//-----------------------------------------------------------------------------
// The ERDMP command.
//
// Dump an errors file.

void
drc_menu::M_DrcDumpErrs(CmdDesc*)
{
    DRC()->dumpResultsFile();
}


//-----------------------------------------------------------------------------
// The ERUPD command.
//
// Recreate highlight list from errors file.

void
drc_menu::M_DrcUpdate(CmdDesc*)
{
    DRC()->setErrlist();
}


//-----------------------------------------------------------------------------
// The NEXT command.
//
// The Next command, reads in an error file, and allows sequential
// visitation of the errors in a pop-up subwindow.
//
void
drc_menu::M_DrcShowErr(CmdDesc *cmd)
{
    DRC()->viewErrsSequentiallyExec(cmd);
}


//-----------------------------------------------------------------------------
// The ERLYR command.
//
// Create objects on layer from highlighting.

void
drc_menu::M_DrcLayer(CmdDesc*)
{
    if (!DRC()->isError()) {
        PL()->ShowPrompt(
            "Highlighted errors list is empty, run DRC or update "
            "highlighting first.");
        return;
    }
    const char *in = PL()->EditPrompt(
        "Highlighted polys saved as objects on this layer name: ", 0);
    if (!in)
        return;
    char *layer = lstring::gettok(&in);
    in = PL()->EditPrompt(
        "Optional positive integer property number for error text: ", "1");
    if (!in) {
        delete [] layer;
        return;
    }
    int n = atoi(in);
    if (n < 0)
        n = 0;
    bool ret = DRC()->errLayer(layer, n);
    delete [] layer;
    if (!ret)
        PL()->ShowPrompt("Command failed: internal error.");
    else
        PL()->ShowPrompt("Done.");
}


//-----------------------------------------------------------------------------
// The DREDT command.
//
// Menu command to bring up a panel to edit design rule parameters.
//
void
drc_menu::M_DrcRules(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller)) {
        if (!XM()->CheckCurLayer()) {
            Menu()->Deselect(cmd->caller);
            return;
        }
        DRC()->PopUpRules(cmd->caller, MODE_ON);
    }
    else
        DRC()->PopUpRules(0, MODE_OFF);
}

