
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
#include "dsp_inlines.h"
#include "fio.h"
#include "keymap.h"
#include "keymacro.h"
#include "promptline.h"
#include "tech.h"
#include "errorlog.h"
#include "qtmain.h"

#include <QKeyEvent>
#include <QApplication>
#include <QAbstractButton>
#include <QAction>


//-----------------------------------------------------------------------------
//  Keyboard Macros

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
// End of cMain functions.


// Main function to obtain a macro mapping for a keypress.
//
sKeyMap *
cKbMacro::getKeyToMap()
{
    /*
    bool done = false;
    sKeyMap *nkey = 0;
    int prcnt = 0;
    XM()->ShowPrompt("Enter keypress to map: ");
    for (;;) {
        QEvent *ev = gdk_event_get();
        if (!ev)
            continue;
        if (ev->type() == QEvent::KeyPress) {
            prcnt++;
            QKeyEvent *kev = static_cast<QKeyEvent*>(ev);
            if (kev->key() == 0) {
                // may happen if key is unknown, e.g. the Microsoft Windows
                // key
                gdk_event_free(ev);
                continue;
            }
            if (isModifier(kev->key())) {
                gdk_event_free(ev);
                continue;
            }
            QByteArray ba = kev->text().toLatin1();
            const char *kstr = ba.constData();
            if ((kstr && kstr[1] == 0 &&
                    (*kstr == 27 || *kstr == 13)) &&
                    !(kev->modifiers & (Qt::ControlModifier | Qt::Mod1Modifier))) {
                // Esc or CR
                XM()->ErasePrompt();
                gdk_event_free(ev);
                return (0);
            }
            if (!done) {
                if (not_mappable(kev->key(), mod_state(kev->modifiers()))) {
                    XM()->ShowPrompt("Not mappable, try another: ");
                    gdk_event_free(ev);
                    continue;
                }
                nkey = already_mapped(kev->key(),
                    mod_state(kev->modifiers()));
                if (!nkey)
                    nkey = new sKeyMap(kev->key(),
                        mod_state(kev->modifiers()), kstr);
                done = true;
            }
            gdk_event_free(ev);
            continue;
        }
        if (ev->type() == QEvent::KeyRelease) {
            if (prcnt)
                prcnt--;
            if (!prcnt && done)
                break;
            gdk_event_free(ev);
            continue;
        }
        if (ev->type() == QEvent::MouseButtonPress ||
                ev->type == GDK_2BUTTON_PRESS) {
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
cKbMacro::isModifier(unsigned key)
{
    return (
        key == Qt::Key_Shift    ||
        key == Qt::Key_Control  ||
        key == Qt::Key_Meta     ||
        key == Qt::Key_Alt      ||
        key == Qt::Key_CapsLock);
}


bool
cKbMacro::isControl(unsigned key)
{
    return (key == Qt::Key_Control);
}


bool
cKbMacro::isShift(unsigned key)
{
    return (key == Qt::Key_Shift);
}


bool
cKbMacro::isAlt(unsigned key)
{
    return (key == Qt::Key_Alt);
}


char *
cKbMacro::keyText(unsigned key, unsigned state)
{
    static char text[4];
    text[0] = 0;

    Qt::KeyboardModifiers mod = Qt::NoModifier;
    if (state & GR_SHIFT_MASK)
        mod |= Qt::ShiftModifier;
    if (state & GR_CONTROL_MASK)
        mod |= Qt::ControlModifier;
    if (state & GR_ALT_MASK)
        mod |= Qt::AltModifier;

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QKeySequence ks(QKeyCombination(mod, (Qt::Key)key));
#else
    // No QKeyCombination in QT5.
    QKeySequence ks(mod | key);
#endif
    QByteArray ba = ks.toString().toLatin1();
    if (ba.size() > 0) {
        const char *tt = ba.constData();
        strncpy(text, tt, 4);
// printf("*%s*\n", tt);
    }

    if (key == Qt::Key_Return) {
        text[0] = '\n';
        text[1] = 0;
    }
    return (text);
}


void
cKbMacro::keyName(unsigned key, char *buf)
{
    QKeySequence ks(key);
    strncpy(buf, ks.toString().toLatin1().constData(), 32);
}


bool
cKbMacro::isModifierDown()
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
cKbMacro::notMappable(unsigned key, unsigned state)
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


// Send a key event, return false if the window can't be found.
//
bool
cKbMacro::execKey(sKeyEvent *k)
{
    if (!QTmainwin::self())
        return (false);
    if (!k->widget_name) {
        QTmainwin::self()->send_key_event(k);
        return (true);
    }

    QTpkg::self()->CheckForInterrupt();
    const QObject *w = qt_keyb::name_to_object(k->widget_name);
    if (!w)
        return (false);

/*XXX
    QKeyEvent event;
    event.state = k->state;
    event.keyval = k->key;
    event.string = k->text;
    event.length = strlen(k->text);
    event.type = (k->type == KEY_PRESS ? QEvent::KeyPress : QEvent::KeyRelease);

    w->event(&event);
*/
    return (true);
}


// Send a button event, return false if the window can't be found.
//
bool
cKbMacro::execBtn(sBtnEvent *b)
{
    if (b->type == BUTTON_PRESS)
        QTpkg::self()->CheckForInterrupt();
    QObject *w = qt_keyb::name_to_object(b->widget_name);
    if (!w)
        return (false);

    if (dynamic_cast<const QAbstractButton*>(w)) {
        if (b->type == BUTTON_RELEASE) {
            if (w == km_last_btn)
//                gtk_button_clicked(GTK_BUTTON(w));
;
            km_last_btn = 0;
        }
        else
            km_last_btn = w;
        km_last_menu = 0;
    }
    else if (dynamic_cast<const QAction*>(w)) {
        if (b->type == BUTTON_RELEASE) {
            if (km_last_menu) {
                QAction *a = dynamic_cast<QAction*>(w);
                a->activate(QAction::Trigger);
//                gtk_menu_shell_deactivate(GTK_MENU_SHELL(w->parent));
            }
            km_last_menu = 0;
        }
        else
            km_last_menu = w;
        km_last_btn = 0;
    }
    else {
        km_last_menu = w;
        km_last_btn = 0;
/*XXX
        int xo, yo;
        gdk_window_get_root_origin(w->window, &xo, &yo);

        eQMouseEvent event;
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
        w->event(&event);
*/
    }
    return (true);
}
// End of cKbMacro functions


// Start recording events for macro definition.
//
void
sKeyMap::begin_recording(char *str)
{
    forstr = lstring::copy(str);
    for (end = response; end && end->next; end = end->next) ;
    show();

    lastkey = 0;
    grabber = 0;
    QTpkg::self()->RegisterEventHandler(qt_keyb::macro_event_handler, this);
}
// end of sKeyMap functions


// Event handler in effect while processing macro definition input
//
bool
qt_keyb::macro_event_handler(QObject *obj, QEvent *ev, void *arg)
{
    sKeyMap *km = static_cast<sKeyMap*>(arg);
    if (ev->type() == QEvent::KeyRelease) {
        // The string field of the key up event is empty.
        QKeyEvent *kev = dynamic_cast<QKeyEvent*>(ev);
        if (km->lastkey == 13 && !(mod_state(kev->modifiers()) &
                (GR_CONTROL_MASK | GR_ALT_MASK))) {
            km->fix_modif();
            QTpkg::self()->RegisterEventHandler(0, 0);
            KbMac()->SaveMacro(km, true);
            return (true);
        }
        if (km->lastkey == 8 && !(mod_state(kev->modifiers()) &
                (GR_CONTROL_MASK | GR_ALT_MASK)))
            return (true);

        char *wname = find_wname(obj);
        if (wname) {
            sEvent *nev = new sKeyEvent(wname,
                mod_state(kev->modifiers()), KEY_RELEASE, kev->key());
            if (!kev->text().isNull() && kev->text().toLatin1()[1] == 0)
                *((sKeyEvent*)nev)->text = kev->text().toLatin1()[0];;
            km->add_response(nev);
        }
        return (true);
    }
    if (ev->type() == QEvent::KeyPress) {
        QKeyEvent *kev = dynamic_cast<QKeyEvent*>(ev);
        km->lastkey = (!kev->text().isNull() &&
            kev->text().toLatin1()[1] == 0) ? kev->text().toLatin1()[0] : 0;
        if (km->lastkey == 13 && !(mod_state(kev->modifiers()) &
                (GR_CONTROL_MASK | GR_ALT_MASK)))
            return (true);
        if (kev->key() == Qt::Key_Escape && !(mod_state(kev->modifiers()) &
                (GR_CONTROL_MASK | GR_ALT_MASK))) {
            km->clear_response();
            QTpkg::self()->RegisterEventHandler(0, 0);
            KbMac()->SaveMacro(km, false);
            return (true);
        }
        if (km->lastkey == 8 && !(mod_state(kev->modifiers()) &
                (GR_CONTROL_MASK | GR_ALT_MASK))) {
            if (!km->response)
                return (true);
            km->clear_last_event();
            km->show();
            return (true);
        }

        char *wname = find_wname(obj);
        if (wname) {
            sEvent *nev = new sKeyEvent(wname,
                mod_state(kev->modifiers()), KEY_PRESS, kev->key());
            if (!kev->text().isNull() && kev->text().toLatin1()[1] == 0)
                *((sKeyEvent*)nev)->text = kev->text().toLatin1()[0];
            km->add_response(nev);
            km->show();
        }
        return (true);
    }
    if (ev->type() == QEvent::MouseButtonPress ||
            ev->type() == QEvent::MouseButtonRelease) {
        if (ev->type() == QEvent::MouseButtonRelease && km->grabber) {
            QTdev::Deselect(km->grabber);
            km->grabber = 0;
//            gtk_main_do_event(ev);
        }
        QMouseEvent *mev = dynamic_cast<QMouseEvent*>(ev);

        char *wname = qt_keyb::find_wname(obj);
        if (wname) {
            sEvent *nev = new sBtnEvent(wname,
                mod_state(mev->modifiers()),
                ev->type() == QEvent::MouseButtonPress ?
                BUTTON_PRESS : BUTTON_RELEASE, mev->button(),
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
                (int)mev->position().x(), (int)mev->position().y());
#else
                (int)mev->x(), (int)mev->y());
#endif
            /*
            if (ev->type() == QEvent::MouseButtonPress &&
                    GTK_IS_MENU_ITEM(widg) &&
                    GTK_MENU_ITEM(widg)->submenu) {
                // show the menu
                km->grabber = widg;
//                gtk_main_do_event(ev);
            }
            */
            km->add_response(nev);
            if (ev->type() == QEvent::MouseButtonPress)
                km->show();
        }
        return (true);
    }
    if (ev->type() == QEvent::MouseButtonDblClick)
        return (true);
//    gtk_main_do_event(ev);
    return (false);
}


// Return the widget and its name from the window.  Return 0 if the widget
// isn't recognized.
//
char *
qt_keyb::find_wname(const QObject *obj)
{
    if (obj)
        return (object_path(obj));
    return (0);
}


// Return the path name for the object.  Heap allocated.
//
char *
qt_keyb::object_path(const QObject *object)
{
    if (!object)
        return (0);
    char *aa[256];
    int cnt = 0, len = 0;
    while (object) {
        QByteArray ba = object->objectName().toLatin1();
        const char *nm = ba.constData();
        if (!nm || !*nm)
            nm = "unknown";
        aa[cnt] = lstring::copy(nm);
        len += strlen(aa[cnt]) + 1;
        object = object->parent();
        cnt++;
    }
    cnt--;
    char *path = new char[len+1];
    char *t = path;
    while (cnt >= 0) {
        strcpy(t, aa[cnt]);
        delete [] aa[cnt];
        while (*t)
            t++;
        if (cnt)
            *t++ = '.';
        cnt--;
    }
    *t = 0;
    return (path);
}


// Return a list of widgets that match the path given, relative to w
//
qt_keyb::wlist *
qt_keyb::find_object(const QObject *obj, const char *path)
{
    wlist *w0 = 0;
    const char *t = strchr(path, '.');
    if (!t)
        t = path + strlen(path);
    char name[128];
    strncpy(name, path, t-path);
    name[t-path] = 0;
    QList<QObject*> chl = obj->children();
    for (int i = 0; chl[i]; i++) {
        QObject *ww = chl[i];
        if (ww->objectName() == name) {
            if (*t == '.') {
                wlist *wx = find_object(ww, t+1);
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
    return (w0);
}


// If the path identifies a widget uniquely, return it.  The path is rooted
// in a top level widget.
//
QObject *
qt_keyb::name_to_object(const char *path)
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
    QList<QWidget*> qwl = QApplication::topLevelWidgets();
    for (int i = 0; qwl[i]; i++) {
        QWidget *ww = qwl[i];
        if (ww->objectName() == name) {
            if (*t == '.') {
                wlist *wx = find_object(ww, t+1);
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
    QObject *wfound = 0;
    if (w0 && !w0->next)
        wfound = w0->obj;
    while (w0) {
        wlist *wx = w0->next;
        delete w0;
        w0 = wx;
    }
    return (wfound);
}

