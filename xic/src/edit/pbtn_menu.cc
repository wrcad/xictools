
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
#include "edit.h"
#include "extif.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "pbtn_menu.h"
#include "select.h"
#include "promptline.h"

#include "bitmaps/xform.xpm"
#include "bitmaps/place.xpm"
#include "bitmaps/label.xpm"
#include "bitmaps/logo.xpm"
#include "bitmaps/box.xpm"
#include "bitmaps/polyg.xpm"
#include "bitmaps/wire.xpm"
#include "bitmaps/round.xpm"
#include "bitmaps/donut.xpm"
#include "bitmaps/arc.xpm"
#include "bitmaps/sides.xpm"
#include "bitmaps/xor.xpm"
#include "bitmaps/break.xpm"
#include "bitmaps/erase.xpm"
#include "bitmaps/put.xpm"
#include "bitmaps/spin.xpm"


namespace {
    namespace pbtn_menu {
        MenuFunc  M_Xform;
        MenuFunc  M_Place;
        MenuFunc  M_MakeLabels;
        MenuFunc  M_Logo;
        MenuFunc  M_MakeBoxes;
        MenuFunc  M_MakePolygons;
        MenuFunc  M_MakeWires;
        MenuFunc  M_MakeDisks;
        MenuFunc  M_MakeDonuts;
        MenuFunc  M_MakeArcs;
        MenuFunc  M_Sides;
        MenuFunc  M_XORbox;
        MenuFunc  M_Break;
        MenuFunc  M_Erase;
        MenuFunc  M_Put;
        MenuFunc  M_Rotation;
    }
}

using namespace pbtn_menu;

namespace {
    MenuEnt PbtnMenu[btnPhysMenu_END + 1];
    MenuBox PbtnMenuBox;

    // Order here must match WsType enum!
    const char * style_list[] = {
        "Wire Width",
        "Flush Ends",
        "Rounded Ends",
        "Extended Ends",
        0,
    };


    void
    pbtn_pre_switch_proc(int, MenuEnt*)
    {
        ED()->PopUpPlace(MODE_OFF, false);
        ED()->PopUpTransform(0, MODE_OFF, 0, 0);
    }
}


const char *const *
cEdit::styleList()
{
    return (style_list);
}


MenuBox *
cEdit::createPbtnMenu()
{
    PbtnMenu[btnPhysMenu] =
        MenuEnt(&M_NoOp,     "",        ME_MENU,     CMD_SAFE,
        0);
    PbtnMenu[btnPhysMenuXform] =
        MenuEnt(&M_Xform,    MenuXFORM, ME_TOGGLE,   CMD_SAFE,
        MenuXFORM": Set current transform parameters.",         xform_xpm);
    PbtnMenu[btnPhysMenuPlace] =
        MenuEnt(&M_Place,    MenuPLACE, ME_TOGGLE,   CMD_NOTSAFE,
        MenuPLACE": Pop up cell placement panel.",              place_xpm);
    PbtnMenu[btnPhysMenuLabel] =
        MenuEnt(&M_MakeLabels,MenuLABEL,ME_TOGGLE,   CMD_NOTSAFE,
        MenuLABEL": Create or modify a text label.",            label_xpm);
    PbtnMenu[btnPhysMenuLogo] =
        MenuEnt(&M_Logo,     MenuLOGO,  ME_TOGGLE,   CMD_NOTSAFE,
        MenuLOGO": Create physical text.",                      logo_xpm);
    PbtnMenu[btnPhysMenuBox] =
        MenuEnt(&M_MakeBoxes,MenuBOX,   ME_TOGGLE,   CMD_NOTSAFE,
        MenuBOX": Create rectangular objects.",                 box_xpm);
    PbtnMenu[btnPhysMenuPolyg] =
        MenuEnt(&M_MakePolygons,MenuPOLYG,ME_TOGGLE, CMD_NOTSAFE,
        MenuPOLYG": Create or modify polygons.",                polyg_xpm);
    PbtnMenu[btnPhysMenuWire] =
        MenuEnt(&M_MakeWires,MenuWIRE,  ME_TOGGLE,   CMD_NOTSAFE,
        MenuWIRE": Create or modify wires.",                    wire_xpm);
    PbtnMenu[btnPhysMenuStyle] =
        MenuEnt(&M_NoOp,     MenuSTYLE, ME_VANILLA,  CMD_SAFE,
        MenuSTYLE": Set wire width and end style.",             0);
    PbtnMenu[btnPhysMenuRound] =
        MenuEnt(&M_MakeDisks,MenuROUND, ME_TOGGLE,   CMD_NOTSAFE,
        MenuROUND": Create a round object.",                    round_xpm);
    PbtnMenu[btnPhysMenuDonut] =
        MenuEnt(&M_MakeDonuts,MenuDONUT,ME_TOGGLE,   CMD_NOTSAFE,
        MenuDONUT": Create a ring object.",                     donut_xpm);
    PbtnMenu[btnPhysMenuArc] =
        MenuEnt(&M_MakeArcs, MenuARC,   ME_TOGGLE,   CMD_NOTSAFE,
        MenuARC": Create an arc.",                              arc_xpm);
    PbtnMenu[btnPhysMenuSides] =
        MenuEnt(&M_Sides,    MenuSIDES, ME_TOGGLE,   CMD_SAFE,
        MenuSIDES": Set the number of sides in round objects.", sides_xpm);
    PbtnMenu[btnPhysMenuXor] =
        MenuEnt(&M_XORbox,   MenuXOR,   ME_TOGGLE,   CMD_NOTSAFE,
        MenuXOR": Exclusive-OR an area.",                       xor_xpm);
    PbtnMenu[btnPhysMenuBreak] =
        MenuEnt(&M_Break,    MenuBREAK, ME_TOGGLE,   CMD_NOTSAFE,
        MenuBREAK": Divide objects along horizontal or vertical.",  break_xpm);
    PbtnMenu[btnPhysMenuErase] =
        MenuEnt(&M_Erase,    MenuERASE, ME_TOGGLE,   CMD_NOTSAFE,
        MenuERASE": Erase or yank geometry.",                   erase_xpm);
    PbtnMenu[btnPhysMenuPut] =
        MenuEnt(&M_Put,      MenuPUT,   ME_TOGGLE,   CMD_NOTSAFE,
        MenuPUT": Put erased or yanked geometry.",              put_xpm);
    PbtnMenu[btnPhysMenuSpin] =
        MenuEnt(&M_Rotation, MenuSPIN,  ME_TOGGLE,   CMD_NOTSAFE,
        MenuSPIN": Rotate objects about a point.",              spin_xpm);
    PbtnMenu[btnPhysMenu_END] =
        MenuEnt();

    PbtnMenuBox.name = "PhysButtons";
    PbtnMenuBox.menu = PbtnMenu;
    PbtnMenuBox.registerPreSwitchProc(&pbtn_pre_switch_proc);
    return (&PbtnMenuBox);
}


//-----------------------------------------------------------------------------
// The XFORM command.
//
// Pop up the Current Transform panel.
//
void
pbtn_menu::M_Xform(CmdDesc *cmd)
{
    if (cmd && MainMenu()->GetStatus(cmd->caller))
        ED()->PopUpTransform(cmd->caller, MODE_ON, cEdit::cur_tf_cb, 0);
    else
        ED()->PopUpTransform(0, MODE_OFF, 0, 0);
}


//-----------------------------------------------------------------------------
// The PLACE command.
//
void
pbtn_menu::M_Place(CmdDesc *cmd)
{
    ED()->placeExec(cmd);
}


//-----------------------------------------------------------------------------
// The LABEL command
//
void
pbtn_menu::M_MakeLabels(CmdDesc *cmd)
{
    ED()->makeLabelsExec(cmd);
}


//-----------------------------------------------------------------------------
// The LOGO command
//
void
pbtn_menu::M_Logo(CmdDesc *cmd)
{
    ED()->makeLogoExec(cmd);
}


//-----------------------------------------------------------------------------
// The BOX command
//
// Menu command to create boxes.
//
void
pbtn_menu::M_MakeBoxes(CmdDesc *cmd)
{
    ED()->makeBoxesExec(cmd);
}


//-----------------------------------------------------------------------------
// The POLYG command
//
// Menu command to initiate the creation of a polygon.
//
void
pbtn_menu::M_MakePolygons(CmdDesc *cmd)
{
    ED()->makePolygonsExec(cmd);
}


//-----------------------------------------------------------------------------
// The WIRE command
//
void
pbtn_menu::M_MakeWires(CmdDesc *cmd)
{
    ED()->makeWiresExec(cmd);
}


//-----------------------------------------------------------------------------
// The ROUND command
//
void
pbtn_menu::M_MakeDisks(CmdDesc *cmd)
{
    ED()->makeDisksExec(cmd);
}


//-----------------------------------------------------------------------------
// The DONUT command
//
void
pbtn_menu::M_MakeDonuts(CmdDesc *cmd)
{
    ED()->makeDonutsExec(cmd);
}


//-----------------------------------------------------------------------------
// The ARC command
//
void
pbtn_menu::M_MakeArcs(CmdDesc *cmd)
{
    ED()->makeArcsExec(cmd);
}


//-----------------------------------------------------------------------------
// The SIDES command
//
// Menu command to change the number of segments per 360 degrees used
// to approximate round objects.
//
void
pbtn_menu::M_Sides(CmdDesc *cmd)
{
    ED()->sidesExec(cmd);
}


//-----------------------------------------------------------------------------
// The XOR command
//
void
pbtn_menu::M_XORbox(CmdDesc *cmd)
{
    ED()->makeXORboxExec(cmd);
}


//-----------------------------------------------------------------------------
// The BREAK command
//
void
pbtn_menu::M_Break(CmdDesc *cmd)
{
    ED()->breakExec(cmd);
}


//-----------------------------------------------------------------------------
// The ERASE command
//
void
pbtn_menu::M_Erase(CmdDesc *cmd)
{
    ED()->eraseExec(cmd);
}


//-----------------------------------------------------------------------------
// The PUT command
//
void
pbtn_menu::M_Put(CmdDesc *cmd)
{
    ED()->putExec(cmd);
}


//-----------------------------------------------------------------------------
// The SPIN command
//
void
pbtn_menu::M_Rotation(CmdDesc *cmd)
{
    ED()->rotationExec(cmd);
}

