
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
#include "scedif.h"
#include "dsp_layer.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "promptline.h"
#include "tech.h"
#include "tech_kwords.h"
#include "select.h"
#include "events.h"
#include "menu.h"
#include "layertab.h"
#include "keymacro.h"
#include "attr_menu.h"
#include "miscutil/filestat.h"
#include "ginterf/grfont.h"


// The Attributes menu for the main and subwindows.

namespace {
    namespace attr_menu {
        MenuFunc  M_SaveTech;
        MenuFunc  M_KeyMap;
        MenuFunc  M_Macro;
        MenuFunc  M_Freeze;
        MenuFunc  M_Attr;
        MenuFunc  M_Dots;
        MenuFunc  M_Font;
        MenuFunc  M_Color;
        MenuFunc  M_FillEdit;
        MenuFunc  M_EditLtab;
        MenuFunc  M_LyrPrmEdit;
    }

    namespace subw_attr_menu {
        MenuFunc  M_Context;
        MenuFunc  M_ShowProps;
        MenuFunc  M_SetLabels;
        MenuFunc  M_SetLabelRot;
        MenuFunc  M_SetCnames;
        MenuFunc  M_SetCnameRot;
        MenuFunc  M_SetNoUnexp;
        MenuFunc  M_TinyBB;
        MenuFunc  M_NoSymbl;
        MenuFunc  M_SetGrid;
    }

    namespace obj_menu {
        MenuFunc  M_Boxes;
        MenuFunc  M_Polys;
        MenuFunc  M_Wires;
        MenuFunc  M_Labels;
    }
}

using namespace attr_menu;
using namespace subw_attr_menu;
using namespace obj_menu;

namespace {
    MenuEnt AttrMenu[attrMenu_END + 1];
    MenuBox AttrMenuBox;
}


MenuBox *
cMain::createAttrMenu()
{
    AttrMenu[attrMenu] =
        MenuEnt(&M_NoOp,     "",        ME_MENU,     CMD_SAFE,
        0);
    AttrMenu[attrMenuUpdat] =
        MenuEnt(&M_SaveTech, MenuUPDAT, ME_TOGGLE,   CMD_SAFE,
        MenuUPDAT": Save updated technology file in the current directory.");
    AttrMenu[attrMenuKeymp] =
        MenuEnt(&M_KeyMap,   MenuKEYMP, ME_VANILLA,  CMD_NOTSAFE,
        MenuKEYMP": Create keyboard mapping file.");
    AttrMenu[attrMenuMacro] =
        MenuEnt(&M_Macro,    MenuMACRO, ME_VANILLA | ME_SEP,  CMD_NOTSAFE,
        MenuMACRO": Define macro.");
    AttrMenu[attrMenuMainWin] =
        MenuEnt(&M_NoOp,     MenuMAINW, ME_MENU | ME_SEP, CMD_SAFE,
        MenuMAINW": Set main window display attributes.");
    AttrMenu[attrMenuAttr] =
        MenuEnt(&M_Attr,     MenuATTR,  ME_TOGGLE,   CMD_SAFE,
        MenuATTR": Set global display attributes.");
    AttrMenu[attrMenuDots] =
        MenuEnt(&M_Dots,     MenuDOTS,  ME_TOGGLE,   CMD_SAFE,
        MenuDOTS": Show connection dots in electrical windows.");
    AttrMenu[attrMenuFont] =
        MenuEnt(&M_Font,     MenuFONT,  ME_TOGGLE,   CMD_SAFE,
        MenuFONT": Pop up font selection panel.");
    AttrMenu[attrMenuColor] =
        MenuEnt(&M_Color,    MenuCOLOR, ME_TOGGLE,   CMD_SAFE,
        MenuCOLOR": Pop up color selection panel.");
    AttrMenu[attrMenuFill] =
        MenuEnt(&M_FillEdit, MenuFILL,  ME_TOGGLE,   CMD_SAFE,
        MenuFILL": Pop up fill pattern editor.");
    AttrMenu[attrMenuEdlyr] =
        MenuEnt(&M_EditLtab, MenuEDLYR, ME_TOGGLE,   CMD_NOTSAFE,
        MenuEDLYR": Add or remove layers in the layer table.");
    AttrMenu[attrMenuLpedt] =
        MenuEnt(&M_LyrPrmEdit,MenuLPEDT,ME_TOGGLE,   CMD_SAFE,
        MenuLPEDT": Edit layer parameter keywords.");
    AttrMenu[attrMenu_END] =
        MenuEnt();

    AttrMenuBox.name = "Attributes";
    AttrMenuBox.menu = AttrMenu;
    return (&AttrMenuBox);
}


// The SubwAttrMenu is used in the sub-windows, and in the main window
// as a sub-menu of the main Attribtues menu.

namespace {
    MenuEnt SubwAttrMenu[subwAttrMenu_END + 1];
    MenuBox SubwAttrMenuBox;

    void
    subw_attr_post_switch_proc(int wnum, MenuEnt *menu)
    {
        if (!menu || wnum < 0 || wnum >= DSP_NUMWINS)
            return;
        WindowDesc *wdesc = DSP()->Window(wnum);
        if (!wdesc)
            return;
        DSPattrib *a = wdesc->Attrib();
        if (!a)
            return;
        DisplayMode mode = wdesc->Mode();

        if (wdesc->DbType() == WDblist || wdesc->DbType() == WDsdb) {
            // cross section or special database display
            for (int i = 1; i < subwAttrMenu_END; i++) {
                MenuEnt *ent = &menu[i];
                switch (i) {
                case subwAttrMenuFreez:
                case subwAttrMenuCntxt:
                case subwAttrMenuProps:
                case subwAttrMenuLabls:
                case subwAttrMenuLarot:
                case subwAttrMenuCnams:
                case subwAttrMenuCnrot:
                case subwAttrMenuNouxp:
                case subwAttrMenuTinyb:
                case subwAttrMenuNosym:
                    Menu()->SetStatus(ent->cmd.caller, ent->is_set());
                    Menu()->SetSensitive(ent->cmd.caller, false);
                    break;
                default:
                    break;
                }
            }
            return;
        }

        for (int i = 1; i < subwAttrMenu_END; i++) {
            MenuEnt *ent = &menu[i];
            if (i == subwAttrMenuFreez)
                ent->set_state(wdesc->IsFrozen());
            else if (i == subwAttrMenuCntxt) {
                ent->set_state(a->show_context(mode));
                if (wdesc->DbType() == WDchd)
                    Menu()->SetSensitive(ent->cmd.caller, false);
                else
                    Menu()->SetSensitive(ent->cmd.caller, true);
            }
            else if (i == subwAttrMenuProps) {
                if (wdesc->DbType() == WDchd)
                    Menu()->SetSensitive(ent->cmd.caller, false);
                else if (mode == Physical) {
                    ent->set_state(a->show_phys_props());
                    Menu()->SetSensitive(ent->cmd.caller, true);
                }
                else {
                    ent->set_state(false);
                    Menu()->SetSensitive(ent->cmd.caller, false);
                }
            }
            else if (i == subwAttrMenuLabls)
                ent->set_state(a->display_labels(mode));
            else if (i == subwAttrMenuLarot) {
                ent->set_state(a->display_labels(mode) == SLtrueOrient);
                Menu()->SetSensitive(ent->cmd.caller, a->display_labels(mode));
            }
            else if (i == subwAttrMenuCnams) {
                ent->set_state(a->label_instances(mode));
                Menu()->SetSensitive(ent->cmd.caller,
                    !a->no_show_unexpand(mode));
            }
            else if (i == subwAttrMenuCnrot) {
                ent->set_state(a->label_instances(mode) == SLtrueOrient);
                Menu()->SetSensitive(ent->cmd.caller,
                    a->label_instances(mode) && !a->no_show_unexpand(mode));
            }
            else if (i == subwAttrMenuNouxp)
                ent->set_state(a->no_show_unexpand(mode));
            else if (i == subwAttrMenuObjs)
                continue;
            else if (i == subwAttrMenuTinyb)
                ent->set_state(a->show_tiny_bb(mode));
            else if (i == subwAttrMenuNosym) {
                ent->set_state(a->no_elec_symbolic());
                if (wdesc->DbType() == WDchd)
                    Menu()->SetSensitive(ent->cmd.caller, false);
                else
                    Menu()->SetSensitive(ent->cmd.caller, true);
            }
            else
                ent->set_state(false);

            Menu()->SetStatus(ent->cmd.caller, ent->is_set());
        }
    }
}


MenuBox *
cMain::createSubwAttrMenu()
{
    SubwAttrMenu[subwAttrMenu] =
        MenuEnt(&M_NoOp,     "",        ME_MENU,     CMD_SAFE,
        0);
    SubwAttrMenu[subwAttrMenuFreez] =
        MenuEnt(M_Freeze, MenuFREEZ,   ME_TOGGLE | ME_SEP, CMD_SAFE,
        MenuFREEZ": Suppress drawing geometry in window.");
    SubwAttrMenu[subwAttrMenuCntxt] =
        MenuEnt(M_Context,  MenuCNTXT, ME_TOGGLE,   CMD_SAFE,
        MenuCNTXT": Show surrounding context in subedit (push).");
    SubwAttrMenu[subwAttrMenuProps] =
        MenuEnt(M_ShowProps,MenuPROPS, ME_TOGGLE,   CMD_SAFE,
        MenuPROPS": Show physical properties in drawing windows.");
    SubwAttrMenu[subwAttrMenuLabls] =
        MenuEnt(M_SetLabels,MenuLABLS, ME_TOGGLE,   CMD_SAFE,
        MenuLABLS": Show labels in drawing window.");
    SubwAttrMenu[subwAttrMenuLarot] =
        MenuEnt(M_SetLabelRot,MenuLAROT,ME_TOGGLE,  CMD_SAFE,
        MenuLAROT": Show labels in true orientation.");
    SubwAttrMenu[subwAttrMenuCnams] =
        MenuEnt(M_SetCnames,MenuCNAMS, ME_TOGGLE,   CMD_SAFE,
        MenuCNAMS": Show cell names in unexpanded subcells.");
    SubwAttrMenu[subwAttrMenuCnrot] =
        MenuEnt(M_SetCnameRot,MenuCNROT,ME_TOGGLE,  CMD_SAFE,
        MenuCNROT": Show unexpanded cell text in true orientation.");
    SubwAttrMenu[subwAttrMenuNouxp] =
        MenuEnt(&M_SetNoUnexp,MenuNOUXP,ME_TOGGLE,  CMD_SAFE,
        MenuNOUXP": Do not show unexpanded subcells.");
    SubwAttrMenu[subwAttrMenuObjs] =
        MenuEnt(&M_NoOp,    MenuOBJS,  ME_MENU,     CMD_SAFE,
        MenuOBJS": Object display control.");
    SubwAttrMenu[subwAttrMenuTinyb] =
        MenuEnt(M_TinyBB,   MenuTINYB, ME_TOGGLE,   CMD_SAFE,
        MenuTINYB": Show outline of subthreshold subcells.");
    SubwAttrMenu[subwAttrMenuNosym] =
        MenuEnt(M_NoSymbl,  MenuNOSYM, ME_TOGGLE | ME_SEP, CMD_SAFE,
        MenuNOSYM": Always show top cell as non-symbolic.");
    SubwAttrMenu[subwAttrMenuGrid] =
        MenuEnt(M_SetGrid,  MenuGRID,  ME_TOGGLE,   CMD_SAFE,
        MenuGRID": Change the grid.");
    SubwAttrMenu[subwAttrMenu_END] =
        MenuEnt();

    SubwAttrMenuBox.name = "Attributes";
    SubwAttrMenuBox.menu = SubwAttrMenu;
    SubwAttrMenuBox.registerPostSwitchProc(&subw_attr_post_switch_proc);
    return (&SubwAttrMenuBox);
}


// The Objects menu is a sub-menu of the subwindow attributes menu, used
// to select object types to display.

namespace {
    MenuBox ObjMenuBox;
    MenuEnt ObjMenu[objSubMenu_END + 1];

    void
    obj_post_switch_proc(int wnum, MenuEnt *menu)
    {
        if (!menu || wnum < 0 || wnum >= DSP_NUMWINS)
            return;
        WindowDesc *wdesc = DSP()->Window(wnum);
        if (!wdesc)
            return;
        DSPattrib *a = wdesc->Attrib();
        if (!a)
            return;

        MenuEnt *ent = &menu[objSubMenuBoxes];
        ent->set_state(a->showing_boxes());
        Menu()->SetStatus(ent->cmd.caller, ent->is_set());
        ent = &menu[objSubMenuPolys];
        ent->set_state(a->showing_polys());
        Menu()->SetStatus(ent->cmd.caller, ent->is_set());
        ent = &menu[objSubMenuWires];
        ent->set_state(a->showing_wires());
        Menu()->SetStatus(ent->cmd.caller, ent->is_set());
        ent = &menu[objSubMenuLabels];
        ent->set_state(a->showing_labels());
        Menu()->SetStatus(ent->cmd.caller, ent->is_set());
    }
}


MenuBox *
cMain::createObjSubMenu()
{
    ObjMenu[objSubMenu] =
        MenuEnt(&M_NoOp,    "",         ME_MENU,    CMD_SAFE,
        0);
    ObjMenu[objSubMenuBoxes] =
        MenuEnt(&M_Boxes,   MenuDPYB,   ME_TOGGLE,  CMD_SAFE,
        MenuDPYB": Show boxes in window display.");
    ObjMenu[objSubMenuPolys] =
        MenuEnt(&M_Polys,   MenuDPYP,   ME_TOGGLE,  CMD_SAFE,
        MenuDPYP": Show polygons in window display.");
    ObjMenu[objSubMenuWires] =
        MenuEnt(&M_Wires,   MenuDPYW,   ME_TOGGLE,  CMD_SAFE,
        MenuDPYW": Show wires in window display.");
    ObjMenu[objSubMenuLabels] =
        MenuEnt(&M_Labels,  MenuDPYL,   ME_TOGGLE,  CMD_SAFE,
        MenuDPYL": Show labels in window display.");
    ObjMenu[objSubMenu_END] =
        MenuEnt();

    ObjMenuBox.name = "Objects";
    ObjMenuBox.menu = ObjMenu;
    ObjMenuBox.registerPostSwitchProc(&obj_post_switch_proc);

    return (&ObjMenuBox);
}


//-----------------------------------------------------------------------------
// The UPDAT command.
//
// Menu command to save an updated version of the technology file
// in the current directory.
//
void
attr_menu::M_SaveTech(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        XM()->PopUpTechWrite(cmd->caller, MODE_ON);
    else
        XM()->PopUpTechWrite(cmd->caller, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The KEYMP command.
//
void
attr_menu::M_KeyMap(CmdDesc *cmd)
{
    XM()->KeyMapExec(cmd);
}


//-----------------------------------------------------------------------------
// The MACRO command.
//
void
attr_menu::M_Macro(CmdDesc*)
{
    KbMac()->GetMacro();
}


//-----------------------------------------------------------------------------
// The FREEZ command.
//
// Don't draw geometry in the drawing window
//
void
attr_menu::M_Freeze(CmdDesc *cmd)
{
    if (cmd && cmd->wdesc && cmd->caller) {
        cmd->wdesc->SetFrozen(Menu()->GetStatus(cmd->caller));
        cmd->wdesc->Redisplay(0);
    }
}


//-----------------------------------------------------------------------------
// The ATTR command.
//
// Menu command to show tiny subcells as bounding box, or detail.
//
void
attr_menu::M_Attr(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        XM()->PopUpAttributes(cmd->caller, MODE_ON);
    else
        XM()->PopUpAttributes(cmd->caller, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The DOTS command.
//
// Menu command to show tiny subcells as bounding box, or detail.
//
void
attr_menu::M_Dots(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        ScedIf()->PopUpDots(cmd->caller, MODE_ON);
    else
        ScedIf()->PopUpDots(cmd->caller, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The FONT command.
//

namespace {
    const char *btn_names[] = {
        "+",  // This magic adds button to default control set.
        "Dump Vector Font ",
        0
    };


    // Callback to save vector font to a file.
    //
    void
    vf_cb(const char *fname, void*)
    {
        if (!fname)
            return;
        char *tok = lstring::getqtok(&fname);
        if (!tok)
            return;

        if (filestat::create_bak(tok)) {
            FILE *fp = fopen(tok, "w");
            if (fp) {
                FT.dumpFont(fp);
                fclose(fp);
                DSPmainWbag(PopUpMessage("Label font dumped to file.", false));
            }
            else
                DSPpkg::self()->Perror(lstring::strip_path(tok));
        }
        else
            DSPpkg::self()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        delete [] tok;
        GRledPopup *ledpop = DSPmainWbagRet(ActiveInput());
        if (ledpop)
            ledpop->popdown();
    }


    void
    font_cb(const char *name, const char *fname, void *arg)
    {
        if (name && !strcmp(name, btn_names[1])) {
            DSPmainWbag(PopUpInput("Enter path for saved vector font",
                XM()->FontFileName(), "Dump Vector Font", vf_cb, 0))
            return;
        }
        if (name == 0 && fname == 0)
            Menu()->Deselect(arg);
    }
}


// Bring up a panel to set the font used in pop-up text windows.
//
void
attr_menu::M_Font(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        DSPmainWbag(PopUpFontSel(cmd->caller, GRloc(), MODE_ON, font_cb,
            cmd->caller, FNT_FIXED, btn_names))
    else
        DSPmainWbag(PopUpFontSel(cmd->caller, GRloc(), MODE_OFF, 0, 0, 0))

/*XXX for GTK, why is panel hidden not destroyed?
    if (cmd && Menu()->GetStatus(cmd->caller))
        DSPmainWbag(PopUpFontSel(0, GRloc(), MODE_ON, font_cb,
            cmd->caller, FNT_FIXED, btn_names))
    else
        DSPmainWbag(PopUpFontSel(0, GRloc(), MODE_OFF, 0, 0, 0))
*/
}


//-----------------------------------------------------------------------------
// The COLOR command.
//
// Bring up the color-setting panel.
//
void
attr_menu::M_Color(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        XM()->PopUpColor(cmd->caller, MODE_ON);
    else
        XM()->PopUpColor(cmd->caller, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The FILL command.
//
// Bring up a panel for editing fill patterns.
//
void
attr_menu::M_FillEdit(CmdDesc *cmd)
{
    if (cmd && Menu()->GetStatus(cmd->caller))
        XM()->PopUpFillEditor(cmd->caller, MODE_ON);
    else
        XM()->PopUpFillEditor(cmd->caller, MODE_OFF);
}


//-----------------------------------------------------------------------------
// The EDLYR command.
//
void
attr_menu::M_EditLtab(CmdDesc *cmd)
{
    XM()->EditLtabExec(cmd);
}


//-----------------------------------------------------------------------------
// The CNTXT command.
//
// Toggle display of surrounding structure during a subedit using
// the push command.
//
void
subw_attr_menu::M_Context(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    cmd->wdesc->Attrib()->set_show_context(cmd->wdesc->Mode(),
        Menu()->GetStatus(cmd->caller));
    if (cmd->wdesc == DSP()->MainWdesc()) {
        if (cmd->wdesc->Attrib()->show_context(cmd->wdesc->Mode()))
            PL()->ShowPrompt("Will show context in subedit.");
        else
            PL()->ShowPrompt("Context will not be shown in subedit.");
    }
    if (DSP()->CurCellName() != DSP()->TopCellName())
        cmd->wdesc->Redisplay(0);
}


//-----------------------------------------------------------------------------
// The PROPS command.
//
// Menu command to enable display of physical properties on-screen.
//
void
subw_attr_menu::M_ShowProps(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    DSPattrib *a = cmd->wdesc->Attrib();
    if (cmd->wdesc->Mode() != Physical) {
        Menu()->Deselect(cmd->caller);
        return;
    }
    if (!a->show_phys_props()) {
        Menu()->Select(cmd->caller);
        cmd->wdesc->ShowPhysProperties(0, ERASE);
        a->set_show_phys_props(true);
        cmd->wdesc->ShowPhysProperties(0, DISPLAY);
    }
    else {
        Menu()->Deselect(cmd->caller);
        cmd->wdesc->ShowPhysProperties(0, ERASE);
        a->set_show_phys_props(false);
        cmd->wdesc->ShowPhysProperties(0, DISPLAY);
    }
}


//-----------------------------------------------------------------------------
// The LABLS command.
//
// Menu command to toggle display of labels in a window.
//
void
subw_attr_menu::M_SetLabels(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    bool state = Menu()->GetStatus(cmd->caller);
    MenuEnt *ent = Menu()->FindEntOfWin(cmd->wdesc, MenuLAROT);
    if (state) {
        if (ent && ent->cmd.caller)
            Menu()->SetSensitive(ent->cmd.caller, true);
        if (ent && ent->cmd.caller && Menu()->GetStatus(ent->cmd.caller)) {
            cmd->wdesc->Attrib()->set_display_labels(cmd->wdesc->Mode(),
                SLtrueOrient);
            PL()->ShowPrompt(
                "Labels will always be displayed, in true orientation.");
        }
        else {
            cmd->wdesc->Attrib()->set_display_labels(cmd->wdesc->Mode(),
                SLupright);
            PL()->ShowPrompt("Labels will always be displayed.");
        }
    }
    else {
        if (ent && ent->cmd.caller)
            Menu()->SetSensitive(ent->cmd.caller, false);
        cmd->wdesc->Attrib()->set_display_labels(cmd->wdesc->Mode(),
            SLnone);
        PL()->ShowPrompt("Labels will not be shown.");
    }
}


//-----------------------------------------------------------------------------
// The LAROT command.
//
// Menu command to toggle true orientation display of labels.
//
void
subw_attr_menu::M_SetLabelRot(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    bool state = Menu()->GetStatus(cmd->caller);
    if (state) {
        cmd->wdesc->Attrib()->set_display_labels(cmd->wdesc->Mode(),
            SLtrueOrient);
        PL()->ShowPrompt(
            "Labels will always be displayed, in true orientation.");
    }
    else {
        cmd->wdesc->Attrib()->set_display_labels(cmd->wdesc->Mode(),
            SLupright);
        PL()->ShowPrompt("Labels will always be displayed.");
    }
}


//-----------------------------------------------------------------------------
// The CNAMS command.
//
// Menu command to control whether the name is printed in unexpanded
// instances.
//
void
subw_attr_menu::M_SetCnames(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    bool state = Menu()->GetStatus(cmd->caller);
    MenuEnt *ent = Menu()->FindEntOfWin(cmd->wdesc, MenuCNROT);
    if (state) {
        if (ent && ent->cmd.caller)
            Menu()->SetSensitive(ent->cmd.caller, true);
        if (ent && ent->cmd.caller && Menu()->GetStatus(ent->cmd.caller)) {
            cmd->wdesc->Attrib()->set_label_instances(
                cmd->wdesc->Mode(), SLtrueOrient);
            PL()->ShowPrompt(
                "Instances will be labeled, using true orientation.");
        }
        else {
            cmd->wdesc->Attrib()->set_label_instances(
                cmd->wdesc->Mode(), SLupright);
            PL()->ShowPrompt("Instances will be labeled.");
        }
    }
    else {
        if (ent && ent->cmd.caller)
            Menu()->SetSensitive(ent->cmd.caller, false);
        cmd->wdesc->Attrib()->set_label_instances(
            cmd->wdesc->Mode(), SLnone);
        PL()->ShowPrompt("Instances will not be labeled.");
    }
}


//-----------------------------------------------------------------------------
// The CNROT command.
//
// Menu command to control whether unexpanded cell names are shown in
// true orientation.
//
void
subw_attr_menu::M_SetCnameRot(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    bool state = Menu()->GetStatus(cmd->caller);
    if (state) {
        cmd->wdesc->Attrib()->set_label_instances(
            cmd->wdesc->Mode(), SLtrueOrient);
        PL()->ShowPrompt("Instances will be labeled, using true orientation.");
    }
    else {
        cmd->wdesc->Attrib()->set_label_instances(
            cmd->wdesc->Mode(), SLupright);
        PL()->ShowPrompt("Instances will be labeled.");
    }
}


//-----------------------------------------------------------------------------
// The NOUXP command.
//
// Menu command to toggle display of unexpanded subcells.
//
void
subw_attr_menu::M_SetNoUnexp(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    bool state = Menu()->GetStatus(cmd->caller);
    MenuEnt *entn = Menu()->FindEntOfWin(cmd->wdesc, MenuCNAMS);
    MenuEnt *entr = Menu()->FindEntOfWin(cmd->wdesc, MenuCNROT);
    if (state) {
        cmd->wdesc->Attrib()->set_no_show_unexpand(cmd->wdesc->Mode(), true);
        if (entn && entn->cmd.caller)
            Menu()->SetSensitive(entn->cmd.caller, false);
        if (entr && entr->cmd.caller)
            Menu()->SetSensitive(entr->cmd.caller, false);
        PL()->ShowPrompt("Unexpanded subcells will NOT be shown.");
    }
    else {
        cmd->wdesc->Attrib()->set_no_show_unexpand(cmd->wdesc->Mode(), false);
        if (entn && entn->cmd.caller)
            Menu()->SetSensitive(entn->cmd.caller, true);
        if (entr && entr->cmd.caller) {
            if (entn && entn->cmd.caller &&
                    Menu()->GetStatus(entn->cmd.caller))
                Menu()->SetSensitive(entr->cmd.caller, true);
            else
                Menu()->SetSensitive(entr->cmd.caller, false);
        }
        PL()->ShowPrompt("Unexpanded subcells will be shown.");
    }
}

//-----------------------------------------------------------------------------
// The OBJS command.
//
// Sub-menu toggles visibility of object types.
//
void
obj_menu::M_Boxes(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    bool state = Menu()->GetStatus(cmd->caller);
    cmd->wdesc->Attrib()->set_showing_boxes(state);
}


void
obj_menu::M_Polys(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    bool state = Menu()->GetStatus(cmd->caller);
    cmd->wdesc->Attrib()->set_showing_polys(state);
}


void
obj_menu::M_Wires(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    bool state = Menu()->GetStatus(cmd->caller);
    cmd->wdesc->Attrib()->set_showing_wires(state);
}


void
obj_menu::M_Labels(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    bool state = Menu()->GetStatus(cmd->caller);
    cmd->wdesc->Attrib()->set_showing_labels(state);
}


//-----------------------------------------------------------------------------
// The TINYB command.
//
// Menu command to show tiny subcells as bounding box, or detail.
//
void
subw_attr_menu::M_TinyBB(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    DSPattrib *a = cmd->wdesc->Attrib();
    if (Menu()->GetStatus(cmd->caller)) {
        PL()->ShowPrompt("Tiny cells will show bounding box only.");
        a->set_show_tiny_bb(cmd->wdesc->Mode(), true);
    }
    else {
        PL()->ShowPrompt("Tiny cells will show detail.");
        a->set_show_tiny_bb(cmd->wdesc->Mode(), false);
    }
}


//-----------------------------------------------------------------------------
// The NOSYM command.
//
// Set/unset use of symbolic representation for top-level cell, applies
// to electrical mode only.
//
void
subw_attr_menu::M_NoSymbl(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    DSPattrib *a = cmd->wdesc->Attrib();
    if (cmd->wdesc == DSP()->MainWdesc()) {
        EV()->InitCallback();
        Selections.deselectTypes(CurCell(), 0);
    }
    if (Menu()->GetStatus(cmd->caller)) {
        PL()->ShowPrompt("Window will NOT show symbolic form of top cell.");
        a->set_no_elec_symbolic(true);
        if (cmd->wdesc == DSP()->MainWdesc())
            ScedIf()->assertSymbolic(true);
    }
    else {
        if (cmd->wdesc->CurCellName() != cmd->wdesc->TopCellName() &&
                cmd->wdesc->Mode() == Electrical &&
                cmd->wdesc->TopCellDesc(Electrical, true)->symbolicRep(0)) {
            // We are in a Push, and the top cell would revert to showing
            // as symbolic.  Don't want to do that.
            PL()->ShowPrompt(
        "Cannot display top level of context as symbolic while in Push.\n");
            Menu()->SetStatus(cmd->caller, true);
            return;
        }
        PL()->ShowPrompt("Window will show symbolic form of top cell.");
        a->set_no_elec_symbolic(false);
        if (cmd->wdesc == DSP()->MainWdesc())
            ScedIf()->assertSymbolic(true);
    }
    if (cmd->wdesc->Mode() == Electrical) {
        cmd->wdesc->ClearViews();
        cmd->wdesc->CenterFullView();
        cmd->wdesc->Redisplay(0);
    }
}


//-----------------------------------------------------------------------------
// The GRID command.
//
// Pop up/down the Grid Parameter panel.
//
void
subw_attr_menu::M_SetGrid(CmdDesc *cmd)
{
    if (!cmd || !cmd->wdesc || !cmd->caller)
        return;
    if (cmd->wdesc->Wbag())
        cmd->wdesc->Wbag()->PopUpGrid(cmd->caller,
            Menu()->GetStatus(cmd->caller) ? MODE_ON : MODE_OFF);
}


//-----------------------------------------------------------------------------
// The LPEDT command.

void
attr_menu::M_LyrPrmEdit(CmdDesc *cmd)
{  
    if (cmd && !Menu()->GetStatus(cmd->caller)) {
        XM()->PopUpLayerParamEditor(0, MODE_OFF, 0, 0);
        return;
    }
    if (!XM()->CheckCurLayer()) {
        if (cmd)
            Menu()->Deselect(cmd->caller);
        return;
    }
    XM()->PopUpLayerParamEditor(cmd ? cmd->caller : 0, MODE_ON, 0, 0);
} 

