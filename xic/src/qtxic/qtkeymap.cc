
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

#include "qtmain.h"
#include "dsp_inlines.h"
#include "events.h"
#include "keymap.h"
#include "drc.h"
#include <unistd.h>
//#include <pwd.h>
#include <sys/stat.h>

#include <QKeyEvent>
#include <QApplication>


//
// Functions for keyboard mapping and macros
//

//  Mapping for key-down
//
static keymap kmap_dn[] = {
    { Qt::Key_Return,       RETURN_KEY,     0 },
    { Qt::Key_Escape,       ESCAPE_KEY,     0 },
    { Qt::Key_Tab,          TAB_KEY,        0 },
    { Qt::Key_Pause,        BREAK_KEY,      0 },
    { Qt::Key_Delete,       DELETE_KEY,     0 },
    { Qt::Key_Backspace,    BSP_KEY,        0 },
    { Qt::Key_Left,         LEFT_KEY,       0 },
    { Qt::Key_Up,           UP_KEY,         0 },
    { Qt::Key_Right,        RIGHT_KEY,      0 },
    { Qt::Key_Down,         DOWN_KEY,       0 },
    { Qt::Key_Shift,        SHIFTDN_KEY,    0 },
    { Qt::Key_Control,      CTRLDN_KEY,     0 },
    { Qt::Key_Home,         HOME_KEY,       0 },
/* XXX how to get *numeric* +/-? */
    { Qt::Key_Plus,         NUPLUS_KEY,     0 },
    { Qt::Key_Minus,        NUMINUS_KEY,    0 },
/*
#ifdef __APPLE__
        // Mappings for MacBook Pro keyboard: fn-Enter and fn-Right.
        { GDK_KEY_KP_Enter,     NUPLUS_KEY,     0 },
        { GDK_KEY_End,          NUMINUS_KEY,    0 },
#endif
*/

    { Qt::Key_PageDown,     PAGEDN_KEY,     0 },
    { Qt::Key_PageUp,       PAGEUP_KEY,     0 },
    { Qt::Key_F1,           FUNC_KEY,       0 },
    { Qt::Key_F2,           FUNC_KEY,       1 },
    { Qt::Key_F3,           FUNC_KEY,       2 },
    { Qt::Key_F4,           FUNC_KEY,       3 },
    { Qt::Key_F5,           FUNC_KEY,       4 },
    { Qt::Key_F6,           FUNC_KEY,       5 },
    { Qt::Key_F7,           FUNC_KEY,       6 },
    { Qt::Key_F8,           FUNC_KEY,       7 },
    { Qt::Key_F9,           FUNC_KEY,       8 },
    { Qt::Key_F10,          FUNC_KEY,       9 },
    { Qt::Key_F11,          FUNC_KEY,       10 },
    { 0,                    NO_KEY,         0 }
};

//  Mapping for key-up
//
static keymap kmap_up[] = {
    { Qt::Key_Shift,        SHIFTUP_KEY,    0 },
    { Qt::Key_Control,      CTRLUP_KEY,     0 },
    { 0,                    NO_KEY,         0 }
};

// Actions done before passing the keypress to the current command
//
static keyaction actions_pre[] = {
    { ESCAPE_KEY,  0,                               Escape_action },
    { PAGEDN_KEY,  0,                               DRCf_action },
    { PAGEUP_KEY,  0,                               DRCb_action },
    { TAB_KEY,     GR_SHIFT_MASK,                   CodeRedo_action },
    { TAB_KEY,     0,                               CodeUndo_action },
    { 'c',         GR_CONTROL_MASK,                 Interrupt_action },
    { 'd',         GR_CONTROL_MASK,                 Delete_action },
    { 'g',         GR_CONTROL_MASK,                 Grid_action },
    { 'r',         GR_CONTROL_MASK,                 Redisplay_action },
    { 'x',         GR_CONTROL_MASK,                 Expand_action },
    { 'z',         GR_CONTROL_MASK,                 Iconify_action },
    { 0,           0,                               No_action }
};

// Actions performed if keypress was not handled by current command
//
static keyaction actions_post[] = {
    { BSP_KEY,     0,                               Bsp_action },
    { DELETE_KEY,  0,                               No_action },
    { UNDO_KEY,    0,                               Undo_action },
    { REDO_KEY,    0,                               Redo_action },
    { 'A',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'B',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'C',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'D',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'E',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'F',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'G',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'H',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'I',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'J',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'K',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'L',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'M',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'N',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'O',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'P',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'Q',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'R',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'S',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'T',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'U',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'V',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'W',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'X',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'Y',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'Z',         GR_SHIFT_MASK | GR_CONTROL_MASK, NameView_action },
    { 'b',         GR_CONTROL_MASK,                 DRCb_action },
    { 'e',         GR_CONTROL_MASK,                 Coord_action },
    { 'f',         GR_CONTROL_MASK,                 DRCf_action },
    { 'n',         GR_CONTROL_MASK,                 SaveView_action },
    { 'p',         GR_CONTROL_MASK,                 DRCp_action },
    { 'u',         GR_CONTROL_MASK,                 ClearKeys_action },
    { 'v',         GR_CONTROL_MASK,                 Version_action },
    { '!',         0,                               Command_action },
    { '?',         0,                               Help_action },
    { LEFT_KEY,    GR_SHIFT_MASK | GR_CONTROL_MASK, SetNextView_action },
    { LEFT_KEY,    GR_CONTROL_MASK,                 DecRot_action },
    { LEFT_KEY,    GR_SHIFT_MASK,                   PanLeftFine_action },
    { LEFT_KEY,    0,                               PanLeft_action },
    { DOWN_KEY,    GR_SHIFT_MASK | GR_CONTROL_MASK, SetNextView_action },
    { DOWN_KEY,    GR_CONTROL_MASK,                 DecRot_action },
    { DOWN_KEY,    GR_SHIFT_MASK,                   PanDownFine_action },
    { DOWN_KEY,    0,                               PanDown_action },
    { RIGHT_KEY,   GR_SHIFT_MASK | GR_CONTROL_MASK, SetPrevView_action },
    { RIGHT_KEY,   GR_CONTROL_MASK,                 IncRot_action },
    { RIGHT_KEY,   GR_SHIFT_MASK,                   PanRightFine_action },
    { RIGHT_KEY,   0,                               PanRight_action },
    { UP_KEY,      GR_SHIFT_MASK | GR_CONTROL_MASK, SetPrevView_action },
    { UP_KEY,      GR_CONTROL_MASK,                 IncRot_action },
    { UP_KEY,      GR_SHIFT_MASK,                   PanUpFine_action },
    { UP_KEY,      0,                               PanUp_action },
    { HOME_KEY,    0,                               FullView_action },
    { NUPLUS_KEY,  GR_SHIFT_MASK,                   ZoomInFine_action },
    { NUMINUS_KEY, GR_SHIFT_MASK,                   ZoomOutFine_action },
    { NUPLUS_KEY,  0,                               ZoomIn_action },
    { NUMINUS_KEY, 0,                               ZoomOut_action },
    { 0,           0,                               No_action }
};


void
cKsMap::SetMap(int code, unsigned keyval)
{
    for (keymap *k = KeymapDown(); k->keyval; k++) {
        if (k->code == code)  {
            k->keyval = keyval;
            break;
        }
    }
}


char *
cKsMap::KeyvalToString(unsigned int keyval)
{
//    return (gdk_keyval_name(keyval));
(void)keyval;
return (0);
}


unsigned int
cKsMap::StringToKeyval(const char *string)
{
//    return (gdk_keyval_from_name(string));
(void)string;
return (0);
}


/*========================================================================
  Keyboard Mapping
 ========================================================================*/

void
cKsMap::init()
{
    kmKeyMapDn = kmap_dn;
    kmKeyMapUp = kmap_up;
    kmActionsPre = actions_pre;
    kmActionsPost = actions_post;
    kmSuppressChar = '`';

    kmKeyTab[0] = sKsMapElt("Home", HOME_KEY, Qt::Key_Home);
    kmKeyTab[1] = sKsMapElt("Page Up", PAGEUP_KEY, Qt::Key_PageUp);
    kmKeyTab[2] = sKsMapElt("Page Down", PAGEDN_KEY, Qt::Key_PageDown);
#ifdef GDK_KP_Subtract
    kmKeyTab[3] = sKsMapElt("Numeric Minus", NUMINUS_KEY, GDK_KP_Subtract);
#else
    kmKeyTab[3] = sKsMapElt("Numeric Minus", NUMINUS_KEY, 0xffff);
#endif
#ifdef GDK_KP_Add
    kmKeyTab[4] = sKsMapElt("Numeric Plus", NUPLUS_KEY, GDK_KP_Add);
#else
    kmKeyTab[4] = sKsMapElt("Numeric Plus", NUPLUS_KEY, 0xffff);
#endif
    kmKeyTab[5] = sKsMapElt(0, 0, 0);
}


int
cKsMap::filter_key(unsigned keysym)
{
    switch (keysym) {
    case Qt::Key_Escape:
        return (-1);
    case Qt::Key_Return:
        return (1);
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_CapsLock:
    case Qt::Key_Meta:
    case Qt::Key_Alt:
    case Qt::Key_Super_L:
    case Qt::Key_Super_R:
    case Qt::Key_Hyper_L:
    case Qt::Key_Hyper_R:
    case Qt::Key_Tab:
    case Qt::Key_Pause:
    case Qt::Key_Delete:
    case Qt::Key_Left:
    case Qt::Key_Up:
    case Qt::Key_Right:
    case Qt::Key_Down:
    case Qt::Key_F1:
    case Qt::Key_F2:
    case Qt::Key_F3:
    case Qt::Key_F4:
    case Qt::Key_F5:
    case Qt::Key_F6:
    case Qt::Key_F7:
    case Qt::Key_F8:
    case Qt::Key_F9:
    case Qt::Key_F10:
    case Qt::Key_F11:
    case Qt::Key_F12:
        return (0);
    default:
        if (keysym >= 0xf000)
            return (2);
    }
    return (0);
}

