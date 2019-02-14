
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
#include "sced.h"
#include "sced_spiceipc.h"
#include "edit.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "ebtn_menu.h"
#include "pbtn_menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "events.h"
#include "select.h"
#include "miscutil/pathlist.h"
#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "bitmaps/xform.xpm"
#include "bitmaps/place.xpm"
#include "bitmaps/devs.xpm"
#include "bitmaps/shapes.xpm"
#include "bitmaps/wire.xpm"
#include "bitmaps/label.xpm"
#include "bitmaps/erase.xpm"
#include "bitmaps/break.xpm"
#include "bitmaps/symbl.xpm"
#include "bitmaps/nodmp.xpm"
#include "bitmaps/subct.xpm"
#include "bitmaps/terms.xpm"
#include "bitmaps/spcmd.xpm"
#include "bitmaps/run.xpm"
#include "bitmaps/deck.xpm"
#include "bitmaps/plot.xpm"
#include "bitmaps/iplot.xpm"


namespace {
    namespace ebtn_menu {
        MenuFunc  M_Xform;
        MenuFunc  M_Place;
        MenuFunc  M_Devs;
        MenuFunc  M_MakeWires;
        MenuFunc  M_MakeLabels;
        MenuFunc  M_Erase;
        MenuFunc  M_Break;
        MenuFunc  M_Symbolic;
        MenuFunc  M_NodeMap;
        MenuFunc  M_Subcircuit;
        MenuFunc  M_ShowTerms;
        MenuFunc  M_SpiceCmd;
        MenuFunc  M_RunSpice;
        MenuFunc  M_SpiceDeck;
        MenuFunc  M_ShowOutput;
        MenuFunc  M_SetDoIplot;
    }
}

using namespace ebtn_menu;

namespace {
    MenuEnt EbtnMenu[btnElecMenu_END + 1];
    MenuBox EbtnMenuBox;

    void
    esd_pre_switch_proc(int, MenuEnt *menu)
    {
        if (!menu || !DSP()->MainWdesc())
            return;
        MenuEnt *ent = &menu[btnElecMenuTerms];
        Menu()->Deselect(ent->cmd.caller);
        ent = &menu[btnElecMenuIplot];
        Menu()->Deselect(ent->cmd.caller);

        ent = &menu[btnElecMenuDevs];
        SCD()->setShowDevs(Menu()->GetStatus(ent->cmd.caller));
        SCD()->PopUpDevs(0, MODE_OFF);

        SCD()->clearPlots();
        SCD()->setDoingIplot(false);
        SCD()->setIplotStatusChanged(false);
        ED()->PopUpPlace(MODE_OFF, false);
        ED()->PopUpTransform(0, MODE_OFF, 0, 0);
    }


    void
    esd_post_switch_proc(int, MenuEnt *menu)
    {
        if (!menu || !DSP()->MainWdesc())
            return;
        if (SCD()->showDevs()) {
            MenuEnt *ent = &menu[btnElecMenuDevs];
            SCD()->PopUpDevs(ent->cmd.caller, MODE_ON);
        }
        if (SCD()->spif()->SimulationActive()) {
            MenuEnt *ent = &menu[btnElecMenuRun];
            Menu()->SetStatus(ent->cmd.caller, true);
        }
        SCD()->setDoingIplot(false);
        SCD()->setIplotStatusChanged(false);
    }
}


MenuBox *
cSced::createEbtnMenu()
{
    EbtnMenu[btnElecMenu] =
        MenuEnt(&M_NoOp,     "",        ME_MENU,     CMD_SAFE,
        0);
    EbtnMenu[btnElecMenuXform] =
        MenuEnt(&M_Xform,    MenuXFORM, ME_TOGGLE,   CMD_SAFE,
        MenuXFORM": Set current transform parameters.",         xform_xpm);
    EbtnMenu[btnElecMenuPlace] =
        MenuEnt(&M_Place,    MenuPLACE, ME_TOGGLE,   CMD_NOTSAFE,
        MenuPLACE": Pop up cell placement panel.",              place_xpm);
    EbtnMenu[btnElecMenuDevs] =
        MenuEnt(&M_Devs,     MenuDEVS,  ME_TOGGLE,   CMD_SAFE,
        MenuDEVS": Pop-up device selection menu.",              devs_xpm);
    EbtnMenu[btnElecMenuShape] =
        MenuEnt(&M_NoOp,     MenuSHAPE, ME_VANILLA,  CMD_SAFE,
        MenuSHAPE": Create an outline object.",                 shapes_xpm);
    EbtnMenu[btnElecMenuWire] =
        MenuEnt(&M_MakeWires,MenuWIRE,  ME_TOGGLE,   CMD_NOTSAFE,
        MenuWIRE": Create or modify wires.",                    wire_xpm);
    EbtnMenu[btnElecMenuLabel] =
        MenuEnt(&M_MakeLabels,MenuLABEL,ME_TOGGLE,   CMD_NOTSAFE,
        MenuLABEL": Create or modify labels.",                  label_xpm);
    EbtnMenu[btnElecMenuErase] =
        MenuEnt(&M_Erase,    MenuERASE, ME_TOGGLE,   CMD_NOTSAFE,
        MenuERASE": Erase geometry.",                           erase_xpm);
    EbtnMenu[btnElecMenuBreak] =
        MenuEnt(&M_Break,    MenuBREAK, ME_TOGGLE,   CMD_NOTSAFE,
        MenuBREAK": Divide objects along horizontal or vertical.", break_xpm);
    EbtnMenu[btnElecMenuSymbl] =
        MenuEnt(&M_Symbolic, MenuSYMBL, ME_TOGGLE,   CMD_SAFE,
        MenuSYMBL": Activate symbolic mode for current cell.",  symbl_xpm);
    EbtnMenu[btnElecMenuNodmp] =
        MenuEnt(&M_NodeMap,  MenuNODMP, ME_TOGGLE,   CMD_SAFE,
        MenuNODMP": Pop up node name mapping editor.",          nodmp_xpm);
    EbtnMenu[btnElecMenuSubct] =
        MenuEnt(&M_Subcircuit,MenuSUBCT,ME_TOGGLE,   CMD_NOTSAFE,
        MenuSUBCT": Create or edit subcircuit formal terminals.", subct_xpm);
    EbtnMenu[btnElecMenuTerms] =
        MenuEnt(&M_ShowTerms,MenuTERMS, ME_TOGGLE,   CMD_SAFE,
        MenuTERMS": Show terminal locations.",                  terms_xpm);
    EbtnMenu[btnElecMenuSpCmd] =
        MenuEnt(&M_SpiceCmd, MenuSPCMD, ME_TOGGLE,   CMD_NOTSAFE,
        MenuSPCMD": Send command to WRspice.",                  spcmd_xpm);
    EbtnMenu[btnElecMenuRun] =
        MenuEnt(&M_RunSpice, MenuRUN,   ME_VANILLA,  CMD_NOTSAFE,
        MenuRUN": Run the WRspice simulator on the current circuit.", run_xpm);
    EbtnMenu[btnElecMenuDeck] =
        MenuEnt(&M_SpiceDeck,MenuDECK,  ME_VANILLA,  CMD_NOTSAFE,
        MenuDECK": Dump a SPICE file of the current circuit.",  deck_xpm);
    EbtnMenu[btnElecMenuPlot] =
        MenuEnt(&M_ShowOutput,MenuPLOT, ME_TOGGLE,   CMD_NOTSAFE,
        MenuPLOT": Plot results from WRspice simulation.",        plot_xpm);
    EbtnMenu[btnElecMenuIplot] =
        MenuEnt(&M_SetDoIplot,MenuIPLOT,ME_TOGGLE,   CMD_NOTSAFE,
        MenuIPLOT": Enable WRspice plotting during simulations.", iplot_xpm);
    EbtnMenu[btnElecMenu_END] =
        MenuEnt();

    EbtnMenuBox.name = "ElecButtons";
    EbtnMenuBox.menu = EbtnMenu;
    EbtnMenuBox.registerPreSwitchProc(&esd_pre_switch_proc);
    EbtnMenuBox.registerPostSwitchProc(&esd_post_switch_proc);
    return (&EbtnMenuBox);
}


// Called from menu dispatch code, special command handling is done
// here.
//
bool
cSced::setupCommand(MenuEnt *ent, bool *retval, bool *call_on_up)
{
    *retval = true;
    *call_on_up = false;

    if (!strcmp(ent->entry, MenuRUN)) {
        if (ScedIf()->simulationActive()) {
            if (ent->action)
                (*ent->action)(&ent->cmd);
            *retval = false;
        }
        return (true);
    }
    if (!strcmp(ent->entry, MenuIPLOT)) {
        *call_on_up = true;
        return (true);
    }

    return (false);
}


//-----------------------------------------------------------------------------
// The XFORM command.
//
// Pop up the Current Transform panel.
//
void
ebtn_menu::M_Xform(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        ED()->PopUpTransform(cmd->caller, MODE_ON, cEdit::cur_tf_cb, 0);
    else
        ED()->PopUpTransform(0, MODE_OFF, 0, 0);
}


//-----------------------------------------------------------------------------
// The PLACE command.
//
void
ebtn_menu::M_Place(CmdDesc *cmd)
{
    ED()->placeExec(cmd);
}


//-----------------------------------------------------------------------------
// The DEVS command.
//
// Pop up/down the devices popup menu (electrical mode only).
//
void
ebtn_menu::M_Devs(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller)) {
        SCD()->PopUpDevs(cmd->caller, MODE_ON);
        SCD()->setShowDevs(true);
    }
    else {
        SCD()->PopUpDevs(cmd->caller, MODE_OFF);
        SCD()->setShowDevs(false);
    }
}


//-----------------------------------------------------------------------------
// The WIRE command
//
void
ebtn_menu::M_MakeWires(CmdDesc *cmd)
{
    ED()->makeWiresExec(cmd);
}


//-----------------------------------------------------------------------------
// The LABEL command
//
void
ebtn_menu::M_MakeLabels(CmdDesc *cmd)
{
    ED()->makeLabelsExec(cmd);
}


//-----------------------------------------------------------------------------
// The ERASE command
//
void
ebtn_menu::M_Erase(CmdDesc *cmd)
{
    ED()->eraseExec(cmd);
}


//-----------------------------------------------------------------------------
// The BREAK command
//
void
ebtn_menu::M_Break(CmdDesc *cmd)
{
    ED()->breakExec(cmd);
}


//-----------------------------------------------------------------------------
// The SYMBL command.
//
void
ebtn_menu::M_Symbolic(CmdDesc *cmd)
{
    SCD()->symbolicExec(cmd);
}


//-----------------------------------------------------------------------------
// The NODMP command.
//
// Menu command to enable user-defined node names
//
void
ebtn_menu::M_NodeMap(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(true, false, Electrical))
        return;
    ds.clear();

    if (cmd && Menu()->GetStatus(cmd->caller))
        SCD()->PopUpNodeMap(cmd->caller, MODE_ON);
    else
        SCD()->PopUpNodeMap(cmd->caller, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The SUBCT command.
//
void
ebtn_menu::M_Subcircuit(CmdDesc *cmd)
{
    SCD()->subcircuitExec(cmd);
}


//-----------------------------------------------------------------------------
// The TERMS command.
//
void
ebtn_menu::M_ShowTerms(CmdDesc *cmd)
{
    SCD()->showTermsExec(cmd);
}


//-----------------------------------------------------------------------------
// The SPCMD command.
//

// In binary data returns, integers and doubles are sent in "network
// byte order", which may not be the same as the machine order.  Most
// C libraries have htonl, ntohl, etc.  functions for dealing with
// this, however there does not appear to be support for float/double. 
// The function below does the trick for doubles, both to and from
// network byte order.  It simply reverses byte order if machine order
// does not match network byte order.

// If your application will always run WRspice on the same machine, or
// the same type of machine (e.g., Intel CPU), then you don't need to
// worry about byte order.  In particular, Intel x86 will reorder
// bytes, which has some overhead.  If all machines are Intel, then
// this overhead can be eliminated by skipping the byte ordering
// functions and read/write the values directly.

namespace {
    // Reverse the byte order if the MSB's are at the top address, i.e.,
    // switch to/from "network byte order".  This will reverse bytes on
    // Intel x86, but is a no-op on Sun SPARC (for example).
    // 
    // IEEE floating point is assumed here!
    //
    double net_byte_reorder(double d)
    {
        static double d1p0 = 1.0;

        if ( ((unsigned char*)&d1p0)[7] ) {
            // This means MSB's are at top address, reverse bytes.
            double dr;
            unsigned char *t = (unsigned char*)&dr;
            unsigned char *s = (unsigned char*)&d;
            t[0] = s[7];
            t[1] = s[6];
            t[2] = s[5];
            t[3] = s[4];
            t[4] = s[3];
            t[5] = s[2];
            t[6] = s[1];
            t[7] = s[0];
            return (dr);
        }
        else
            return (d);
    }
}


// Electrical menu command prompt for a command to send to WRspice.
//
void
ebtn_menu::M_SpiceCmd(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(true, true, Electrical))
        return;

    EV()->InitCallback();

    if (cmd && Menu()->GetStatus(cmd->caller)) {
        char *in = PL()->EditPrompt(
            "Enter WRspice command, or press Enter for setup: ", "");
        if (!in)
            return;

        if (!strcmp(in, "") || !strcmp(in, "setup")) {
            SCD()->PopUpSpiceIf(0, MODE_ON);
            PL()->ErasePrompt();
            return;
        }

        char *s = lstring::copy(in);
        GCarray<char*> gc_s(s);

        char *retbuf;           // Message returned.
        char *outbuf;           // Stdout/stderr returned.
        char *errbuf;           // Error message.
        unsigned char *databuf; // Command data.
        if (!SCD()->spif()->DoCmd(s, &retbuf, &outbuf, &errbuf, &databuf)) {
            // No connection to simulator.
            if (retbuf) {
                PL()->ShowPrompt(retbuf);
                delete [] retbuf;
            }
            if (errbuf) {
                Log()->ErrorLog("spice ipc", errbuf);
                delete [] errbuf;
            }
            return;
        }
        if (retbuf) {
            PL()->ShowPrompt(retbuf);
            delete [] retbuf;
        }
        if (outbuf) {
            fputs(outbuf, stdout);
            delete [] outbuf;
        }
        if (errbuf) {
            Log()->ErrorLog("spice ipc", errbuf);
            delete [] errbuf;
        }
        if (databuf) {
            // This is returned only by the eval command at present. 
            // The format is:
            //
            // databuf[0]      'o'
            // databuf[1]      'k'
            // databuf[2]      'd'  (datatype double, other types may be
            //                       added in future)
            // databuf[3]      'r' or 'c' (real or complex)
            // databuf[4-7]    array size, network byte order
            // ...             array of data values, network byte order

            printf("\n");
            if (databuf[0] != 'o' || databuf[1] != 'k' || databuf[2] != 'd') {
                // error (shouldn't happen)
                delete [] databuf;
                return;
            }

            // We'll just print the first 10 values of the return.
            unsigned int size = ntohl(*(int*)(databuf+4));
            double *dp = (double*)(databuf + 8);
            for (unsigned int i = 0; i < size; i++) {
                if (i == 10) {
                    printf("...\n");
                    break;
                }
                if (databuf[3] == 'r')      // real values
                    printf("%d   %g\n", i, net_byte_reorder(dp[i]));
                else if (databuf[3] == 'c') //complex values
                    printf("%d   %g,%g\n", i, net_byte_reorder(dp[2*i]),
                        net_byte_reorder(dp[2*i + 1]));
                else
                    // wacky error, can't happen
                    break;
            }
            delete [] databuf;
        }
    }
}


//-----------------------------------------------------------------------------
// The RUN command.
//
// Menu command for running Spice.  In asynchronous mode, the run button
// is left active while simulation continues.
//
void
ebtn_menu::M_RunSpice(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(false, false, Electrical))
        return;

    if (cmd && Menu()->GetStatus(cmd->caller)) {
        if (!SCD()->spif()->RunSpice(cmd))
            return;
        ds.clear();
    }
}


//-----------------------------------------------------------------------------
// The DECK command.
//
// Menu command to create a Spice input file.
//
void
ebtn_menu::M_SpiceDeck(CmdDesc*)
{
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(false, false, Electrical))
        return;
    if (CurCell()->isEmpty()) {
        PL()->ShowPrompt("No electrical data found in current cell.");
        return;
    }
    if (DSP()->CurCellName() == DSP()->TopCellName()) {
        char *s = SCD()->getAnalysis(false);
        if (!s || strcmp(s, "run")) {
            char *in =
                PL()->EditPrompt("Enter optional analysis command: ", s);
            if (!in) {
                PL()->ErasePrompt();
                delete [] s;
                return;
            }
            while (isspace(*in) || *in == '.')
                in++;
            if (*in)
                SCD()->setAnalysis(in);
        }
        delete [] s;
    }

    char tbuf[256];
    strncpy(tbuf, Tstring(DSP()->CurCellName()), 251);
    tbuf[251] = 0;
    strcat(tbuf, ".cir");

    char *in = XM()->SaveFileDlg("New SPICE file name? ", tbuf);
    if (!in || !*in) {
        PL()->ErasePrompt();
        return;
    }
    char *filename = pathlist::expand_path(in, false, true);
    if (SCD()->dumpSpiceFile(filename)) {
        sprintf(tbuf, "Spice listing saved in file %s, view file? ",
            filename);
        in = PL()->EditPrompt(tbuf, "n");
        in = lstring::strip_space(in);
        if (in && (*in == 'y' || *in == 'Y'))
            DSPmainWbag(PopUpFileBrowser(filename))
        PL()->ErasePrompt();
    }
    else
        PL()->ShowPromptV("Can't open %s for writing, permission denied.",
            filename);
    delete [] filename;
}


//-----------------------------------------------------------------------------
// The PLOT command.
//
void
ebtn_menu::M_ShowOutput(CmdDesc *cmd)
{
    SCD()->showOutputExec(cmd);
}


//-----------------------------------------------------------------------------
// The IPLOT command.
//
void
ebtn_menu::M_SetDoIplot(CmdDesc *cmd)
{
    SCD()->setDoIplotExec(cmd);
}

