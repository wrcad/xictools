
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
#include "qtinlines.h"
#include "dsp_inlines.h"
#include "events.h"
#include "keymap.h"
#include "drc.h"
#include <unistd.h>
#include <pwd.h>
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
cKsMap::setmap(int code, unsigned keyval)
{
    for (keymap *k = keymap_down(); k->keyval; k++) {
        if (k->code == code)  {
            k->keyval = keyval;
            break;
        }
    }
}


char *
cKsMap::keyval_to_string(unsigned int keyval)
{
//    return (gdk_keyval_name(keyval));
(void)keyval;
return (0);
}


unsigned int
cKsMap::string_to_keyval(const char *string)
{
//    return (gdk_keyval_from_name(string));
(void)string;
return (0);
}


/*========================================================================
  Keyboard Mapping
 ========================================================================*/

cKsMap::cKsMap()
{
    suppress_char = '`';
    kmKeyList = 0;
    kmKeyMapDn = kmap_dn;
    kmKeyMapUp = kmap_up;
    kmActionsPre = actions_pre;
    kmActionsPost = actions_post;
    kmLastBtn = 0;
    kmLastMenu = 0;

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


/*========================================================================
  Keyboard Macros
 ========================================================================*/

/*
static void macro_event_handler(GdkEvent*, void*);
static GtkWidget *find_wname(GdkWindow*, char**);
static char *widget_path(GtkWidget*);
static GtkWidget *name_to_widget(const char*);
*/


// Global interface for the script reader
//
bool
cMain::SendKeyEvent(const char *widget_name, int keysym, int state,
    bool up)
{
    sKeyEvent ev(lstring::copy(widget_name), state,
        up ? KEY_RELEASE : KEY_PRESS, keysym);
    if (!up)
        ev.set_text();
    return (ev.exec());
}


// Global interface for the script reader
//
bool
cMain::SendButtonEvent(const char *widget_name, int state, int num, int x,
    int y, bool up)
{
    sBtnEvent ev(lstring::copy(widget_name), state,
        up ? BUTTON_RELEASE : BUTTON_PRESS, num, x, y);
    return (ev.exec());
}


// Convert the state flags to the Xic representation
//
inline unsigned
convert_state(unsigned qtstate)
{
    int state = 0;
    if (qtstate & Qt::ShiftModifier)
        state |= GR_SHIFT_MASK;
    if (qtstate & Qt::ControlModifier)
        state |= GR_CONTROL_MASK;
    if (qtstate & Qt::AltModifier)
        state |= GR_ALT_MASK;
    return (state);
}
// XXX also one of these in subwin_d.cc


// Main function to obtain a macro mapping for a keypress.
//
sKeyMap *
cKsMap::get_key_to_map()
{
    /*
    bool done = false;
    sKeyMap *nkey = 0;
    int prcnt = 0;
    XM()->ShowPrompt("Enter keypress to map: ");
    for (;;) {
        GdkEvent *ev = gdk_event_get();
        if (!ev)
            continue;
        if (ev->type == GDK_KEY_PRESS) {
            prcnt++;
            if (ev->key.keyval == 0) {
                // may happen if key is unknown, e.g. the Microsoft Windows
                // key
                gdk_event_free(ev);
                continue;
            }
            if (Kmap.isModifier(ev->key.keyval)) {
                gdk_event_free(ev);
                continue;
            }
            if ((ev->key.string && ev->key.string[1] == 0 &&
                    (*ev->key.string == 27 || *ev->key.string == 13)) &&
                    !(ev->key.state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))) {
                // Esc or CR
                XM()->ErasePrompt();
                gdk_event_free(ev);
                return (0);
            }
            if (!done) {
                if (not_mappable(ev->key.keyval, ev->key.state)) {
                    XM()->ShowPrompt("Not mappable, try another: ");
                    gdk_event_free(ev);
                    continue;
                }
                nkey = already_mapped(ev->key.keyval,
                    convert_state(ev->key.state));
                if (!nkey)
                    nkey = new sKeyMap(ev->key.keyval,
                        convert_state(ev->key.state), ev->key.string);
                done = true;
            }
            gdk_event_free(ev);
            continue;
        }
        if (ev->type == GDK_KEY_RELEASE) {
            if (prcnt)
                prcnt--;
            if (!prcnt && done)
                break;
            gdk_event_free(ev);
            continue;
        }
        if (ev->type == GDK_BUTTON_PRESS ||
                ev->type == GDK_2BUTTON_PRESS ||
                ev->type == GDK_3BUTTON_PRESS) {
            gdk_event_free(ev);
            return (0);
        }
        gtk_main_do_event(ev);
        gdk_event_free(ev);
    }
    return (nkey);
    */
    return (0);
}


bool
cKsMap::isModifier(unsigned key)
{
    return (
        key == Qt::Key_Shift    ||
        key == Qt::Key_Control  ||
        key == Qt::Key_Meta     ||
        key == Qt::Key_Alt      ||
        key == Qt::Key_CapsLock);
}
// also one of these in subwin_d.cc


bool
cKsMap::isControl(unsigned key)
{
    return (key == Qt::Key_Control);
}


bool
cKsMap::isShift(unsigned key)
{
    return (key == Qt::Key_Shift);
}


bool
cKsMap::isAlt(unsigned key)
{
    return (key == Qt::Key_Alt);
}


char *
cKsMap::key_text(unsigned key, unsigned state)
{
    static char text[4];
    text[0] = 0;

/*  XXX This doesn't work.  F'n QT doesn't seem to have a way to
 * do this.
    Qt::KeyboardModifiers mod = Qt::NoModifier;
    if (state & GR_SHIFT_MASK)
        mod |= Qt::ShiftModifier;
    if (state & GR_CONTROL_MASK)
        mod |= Qt::ControlModifier;
    if (state & GR_ALT_MASK)
        mod |= Qt::AltModifier;

    QKeySequence ks(key + mod);
    const char *tt = ks.toString().toAscii().constFata();
 printf("*%s*\n", tt);
*/

    if (key == Qt::Key_Return) {
        text[0] = '\n';
        text[1] = 0;
    }
    return (text);
}


void
cKsMap::key_name(unsigned key, char *buf)
{
    QKeySequence ks(key);
    strncpy(buf, ks.toString().toAscii().constData(), 32);
}


bool
cKsMap::isModifierDown()
{
    int mstate = QApplication::keyboardModifiers();
    if (mstate & Qt::ShiftModifier)
        return (true);
    if (mstate & Qt::ControlModifier)
        return (true);
    if (mstate & Qt::AltModifier)
        return (true);
    return (false);
}


// Return true if the given key event can not be mapped.
//
bool
cKsMap::not_mappable(unsigned key, unsigned state)
{
    if (isModifier(key))
        return (true);
    if (state)
        return (false);
    switch (key) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Escape:
        return (true);
    }
    return (false);
}


// Send a key event, return false if the window can't be found
//
bool
cKsMap::exec_key(sKeyEvent *k)
{
    if (!mainBag())
        return (false);
    if (!k->widget_name) {
        mainBag()->send_key_event(k);
        return (true);
    }

    /*
    dspPkgIf()->CheckForInterrupt();
    GtkWidget *w = gtk_keyb::name_to_widget(k->widget_name);
    if (w && !w->window)
        gtk_widget_realize(w);
    if (!w || !w->window)
        return (false);

    GdkEventKey event;
    event.window = w->window;
    event.send_event = true;
    event.time = GDK_CURRENT_TIME;
    event.state = k->state;
    event.keyval = k->key;
    event.string = k->text;
    event.length = strlen(k->text);
    event.type = (k->type == KEY_PRESS ? GDK_KEY_PRESS : GDK_KEY_RELEASE);

    gtk_propagate_event(w, (GdkEvent*)&event);
    return (true);
    */
    return (true);
}


// Send a button event, return false if the window can't be found
//
bool
cKsMap::exec_btn(sBtnEvent *b)
{
    /*
    if (b->type == BUTTON_PRESS)
        dspPkgIf()->CheckForInterrupt();
    GtkWidget *w = gtk_keyb::name_to_widget(b->widget_name);
    if (w && !w->window)
        gtk_widget_realize(w);
    if (!w || !w->window)
        return (false);

    if (GTK_IS_BUTTON(w)) {
        if (b->type == BUTTON_RELEASE) {
            if (w == (GtkWidget*)kmLastBtn)
                gtk_button_clicked(GTK_BUTTON(w));
            kmLastBtn = 0;
        }
        else
            kmLastBtn = w;
        kmLastMenu = 0;
    }
    else if (GTK_IS_MENU_ITEM(w)) {
        if (b->type == BUTTON_RELEASE) {
            if (kmLastMenu) {
                gtk_menu_item_activate(GTK_MENU_ITEM(w));
                gtk_menu_shell_deactivate(GTK_MENU_SHELL(w->parent));
            }
            kmLastMenu = 0;
        }
        else
            kmLastMenu = w;
        kmLastBtn = 0;
    }
    else {
        kmLastMenu = w;
        kmLastBtn = 0;
        int xo, yo;
        gdk_window_get_root_origin(w->window, &xo, &yo);

        GdkEventButton event;
        event.window = w->window;
        event.send_event = true;
        event.time = GDK_CURRENT_TIME;
        event.x = b->x;
        event.y = b->y;
        event.x_root = xo + b->x;
        event.y_root = yo + b->y;
#if GTK_CHECK_VERSION(1,3,15)
#else
        event.pressure = 0.5;
        event.xtilt = 0;
        event.ytilt = 0;
        event.source = GDK_SOURCE_MOUSE;
        event.deviceid = GDK_CORE_POINTER;
#endif
        event.state = b->state;
        event.button = b->button;

        event.type = (b->type == BUTTON_PRESS ?
            GDK_BUTTON_PRESS : GDK_BUTTON_RELEASE);
        gtk_propagate_event(w, (GdkEvent*)&event);
    }
    return (true);
    */
    (void)b;
    return (true);
}
// End of cKsMap functions


// Start recording events for macro definition
//
void
sKeyMap::begin_recording(char *str)
{
    /*
    forstr = copy(str);
    for (end = response; end && end->next; end = end->next) ;
    show();

    lastkey = 0;
    grabber = 0;
    GApp->RegisterEventHandler(macro_event_handler, this);
    */
    (void)str;
}
// end of sKeyMap functions


/*
// Event handler in effect while processing macro definition input
//
static void
macro_event_handler(GdkEvent *ev, void *arg)
{
    sKeyMap *km = (sKeyMap*)arg;
    if (ev->type == GDK_KEY_RELEASE) {
        // the string field of the key up event is empty
        if (km->lastkey == 13 &&
                !(ev->key.state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))) {
            km->fix_modif();
            GApp->RegisterEventHandler(0, 0);
            Kmap.SaveMacro(km, true);
            return;
        }
        if (km->lastkey == 8 &&
                !(ev->key.state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
            return;

        char *wname;
        if (find_wname(ev->key.window, &wname)) {
            sEvent *nev = new sKeyEvent(wname,
                convert_state(ev->key.state), KEY_RELEASE, ev->key.keyval);
            if (ev->key.string && ev->key.string[1] == 0)
                *((sKeyEvent*)nev)->text = *ev->key.string;
            km->add_response(nev);
        }
        return;
    }
    if (ev->type == GDK_KEY_PRESS) {
        km->lastkey = (ev->key.string && ev->key.string[1] == 0) ?
            *ev->key.string : 0;
        if (km->lastkey == 13 &&
                !(ev->key.state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
            return;
        if (ev->key.keyval == GDK_Escape &&
                !(ev->key.state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))) {
            km->clear_response();
            GApp->RegisterEventHandler(0, 0);
            Kmap.SaveMacro(km, false);
            return;
        }
        if (km->lastkey == 8 &&
                !(ev->key.state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))) {
            if (!km->response)
                return;
            km->clear_last_event();
            km->show();
            return;
        }

        char *wname;
        if (find_wname(ev->key.window, &wname)) {
            sEvent *nev = new sKeyEvent(wname,
                convert_state(ev->key.state), KEY_PRESS, ev->key.keyval);
            if (ev->key.string && ev->key.string[1] == 0)
                *((sKeyEvent*)nev)->text = *ev->key.string;
            km->add_response(nev);
            km->show();
        }
        return;
    }
    if (ev->type == GDK_BUTTON_PRESS || ev->type == GDK_BUTTON_RELEASE) {
        if (ev->type == GDK_BUTTON_RELEASE && km->grabber) {
//                gtk_menu_item_deselect(GTK_MENU_ITEM(km->grabber));
            km->grabber = 0;
            gtk_main_do_event(ev);
        }

        char *wname;
        GtkWidget *widg = find_wname(ev->button.window, &wname);
        if (widg) {
            sEvent *nev = new sBtnEvent(wname,
                convert_state(ev->button.state),
                ev->button.type == GDK_BUTTON_PRESS ?
                BUTTON_PRESS : BUTTON_RELEASE, ev->button.button,
                (int)ev->button.x, (int)ev->button.y);
            if (ev->type == GDK_BUTTON_PRESS && GTK_IS_MENU_ITEM(widg) &&
                    GTK_MENU_ITEM(widg)->submenu) {
                // show the menu
                km->grabber = widg;
                gtk_main_do_event(ev);
            }
            km->add_response(nev);
            if (ev->type == GDK_BUTTON_PRESS)
                km->show();
        }
        return;
    }
    if (ev->type == GDK_2BUTTON_PRESS || ev->type == GDK_3BUTTON_PRESS)
        return;
    gtk_main_do_event(ev);
}
*/


/*
// Return the widget and its name from the window.  Return 0 if the widget
// isn't recognized
//
static GtkWidget *
find_wname(GdkWindow *window, char **wname)
{
    // this is the magic to get a widget from a window
    GtkWidget *widget = 0;
    gdk_window_get_user_data(window, (void**)&widget);

    if (widget) {
        char *name = widget_path(widget);
        if (name) {
            *wname = name;
            return (widget);
        }
    }
    return (0);
}
*/


/*
// Return the path name for the widget.  Yes, there is a gtk function that
// does the same thing, but it has a massive core leak (2.4.6)
//
static char *
widget_path(GtkWidget *widget)
{
    if (!widget || !GTK_IS_WIDGET(widget)
#if GTK_CHECK_VERSION(1,3,15)
            )
#else
            || GTK_OBJECT_DESTROYED(GTK_OBJECT(widget)))
#endif
        return (0);
    char *aa[256];
    int cnt = 0, len = 0;
    while (widget) {
        aa[cnt] = (char*)gtk_widget_get_name(widget);
        len += strlen(aa[cnt]) + 1;
        widget = widget->parent;
        cnt++;
    }
    cnt--;
    char *path = new char[len+1];
    char *t = path;
    while (cnt >= 0) {
        strcpy(t, aa[cnt]);
        while (*t)
            t++;
        if (cnt)
            *t++ = '.';
        cnt--;
    }
    *t = 0;
    return (path);
}
*/


/*
// List element for widget name matches
//
struct wlist
{
    wlist(GtkWidget *w, wlist *n) { widget = w; next = n; }

    GtkWidget *widget;
    wlist *next;
};
*/


/*
// Return a list of widgets that match the path given, relative to w
//
static wlist *
find_widget(GtkWidget *w, const char *path)
{
    wlist *w0 = 0;
    const char *t = strchr(path, '.');
    if (!t)
        t = path + strlen(path);
    char name[128];
    strncpy(name, path, t-path);
    name[t-path] = 0;
    GList *g = gtk_container_children(GTK_CONTAINER(w));
    for (GList *gt = g; gt; gt = gt->next) {
        GtkWidget *ww = GTK_WIDGET(gt->data);
        if (!strcmp(name, gtk_widget_get_name(ww))) {
            if (*t == '.') {
                wlist *wx = find_widget(ww, t+1);
                if (wx) {
                    wlist *wn = wx;
                    while (wn->next)
                        wn = wn->next;
                    wn->next = w0;
                    w0 = wx;
                }
            }
            else
                w0 = new wlist(ww, w0);
        }
    }
    if (g)
        g_list_free(g);
    return (w0);
}
*/


/*
// If the path identifies a widget uniquely, return it.  The path is rooted
// in a top level (GtkWindow) widget
//
static GtkWidget *
name_to_widget(const char *path)
{
    if (!path)
        return (0);
    wlist *w0 = 0;
    const char *t = strchr(path, '.');
    if (!t)
        t = path + strlen(path);
    char name[128];
    strncpy(name, path, t-path);
    name[t-path] = 0;
#if GTK_CHECK_VERSION(1,3,15)
    GList *g = gtk_window_list_toplevels();
#else
    GList *g = gtk_container_get_toplevels();
#endif
    for ( ; g; g = g->next) {
        GtkWidget *ww = GTK_WIDGET(g->data);
#if GTK_CHECK_VERSION(1,3,15)
        gtk_widget_unref(ww);
#endif
        if (!strcmp(name, gtk_widget_get_name(ww))) {
            if (*t == '.') {
                wlist *wx = find_widget(ww, t+1);
                if (wx) {
                    wlist *wn = wx;
                    while (wn->next)
                        wn = wn->next;
                    wn->next = w0;
                    w0 = wx;
                }
            }
            else
                w0 = new wlist(ww, w0);
        }
    }
#if GTK_CHECK_VERSION(1,3,15)
    g_list_free(g);
#endif
    GtkWidget *wfound = 0;
    if (w0 && !w0->next)
        wfound = w0->widget;
    while (w0) {
        wlist *wx = w0->next;
        delete w0;
        w0 = wx;
    }
    return (wfound);
}
*/


/*========================================================================
  Event History Logging
 ========================================================================*/


/*
// Print the first few lines of the run log file.
//
static void
log_init(FILE *fp)
{
    fprintf(fp, "# %s\n", Global.IdString());

    char buf[256];
    if (CD()->PGetCWD(buf, 256))
        fprintf(fp, "Cwd(\"%s\")\n", buf);

    fprintf(fp, "Techfile(\"%s\")\n",
        Global.TechFile ? Global.TechFile : "<none>");
    fprintf(fp, "Edit(\"%s\", 0)\n",
        GetCurSymbol() ? GetCurSymbol()->cellname() : "<none>");
    fprintf(fp, "Mode(%d)\n", GetCurMode());
    fprintf(fp, "Window(%g, %g, %g, 0)\n",
        (double)(XM()->main_win->w_window.left +
            XM()->main_win->w_window.right)/(2*CDresolution),
        (double)(XM()->main_win->w_window.bottom +
            XM()->main_win->w_window.top)/(2*CDresolution),
        (double)(XM()->main_win->w_window.right -
            XM()->main_win->w_window.left)/CDresolution);
}
*/


/*
// Print a line in the run log file describing the event.  The print format
// is readable by the function parser.
//
void
LogEvent(GdkEvent *ev)
{
    static int ccnt;
    char *msg = "Warning: can't open %s file, history won't be logged.";
    if (ccnt < 0)
        return;

    if (ev->type == GDK_KEY_PRESS || ev->type == GDK_KEY_RELEASE) {
        FILE *fp = fopen_log(Global.LogName, ccnt ? "a" : "w", true);
        if (fp) {
            if (!ccnt)
                log_init(fp);
            if (ev->key.string && isprint(*ev->key.string))
                fprintf(fp, "# %s\n", ev->key.string);
            else
                fprintf(fp, "# %s\n", gdk_keyval_name(ev->key.keyval));
            if (ev->key.send_event) {
                fprintf(fp, "# XSendEvent\n");
                fprintf(fp, "# ");
            }

            GtkWidget *widget = gtk_get_event_widget(ev);
            char *wname = widget_path(widget);
            if (!wname)
                wname = copy("unknown");
            fprintf(fp, "%s(%d, %d, \"%s\")\n",
                ev->type == GDK_KEY_PRESS ? "KeyDown" : "KeyUp",
                ev->key.keyval, ev->key.state, wname);
            delete [] wname;
            fclose(fp);
        }
        else if (!ccnt) {
            XM()->WarningLogV(mh::Initialization, msg, Global.LogName);
            ccnt = -1;
            return;
        }
        ccnt++;
    }
    else if (ev->type == GDK_BUTTON_PRESS || ev->type == GDK_BUTTON_RELEASE) {
        FILE *fp = fopen_log(Global.LogName, ccnt ? "a" : "w", true);
        if (fp) {
            if (!ccnt)
                log_init(fp);
            if (ev->button.send_event)
                // suppress all send events
                fprintf(fp, "# XSendEvent\n# ");
            GtkWidget *widget = gtk_get_event_widget(ev);
            char *wname = widget_path(widget);
            if (!wname)
                wname = copy("unknown");
            fprintf(fp, "%s(%d, %d, %d, %d, \"%s\")\n",
                ev->type == GDK_BUTTON_PRESS ? "BtnDown" : "BtnUp",
                ev->button.button, ev->button.state, (int)ev->button.x,
                (int)ev->button.y, wname);
            delete [] wname;
            fclose(fp);
        }
        else if (!ccnt) {
            XM()->WarningLogV(mh::Initialization, msg, Global.LogName);
            ccnt = -1;
            return;
        }
        ccnt++;
    }
}
*/

