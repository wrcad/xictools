
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
#include "qtkeymacro.h"

#include <QKeyEvent>
#include <QApplication>
#include <QAbstractButton>
#include <QAction>
#include <QMenuBar>


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


namespace {
    struct sGetKeyState
    {
        sGetKeyState() : nkey(0), prcnt(0), done(false) { }

        sKeyMap *nkey;
        int prcnt;
        bool done;
    };
}


// Main function to obtain a macro mapping for a keypress.
//
sKeyMap *
cKbMacro::getKeyToMap()
{
    PL()->ShowPrompt("Enter keypress to map: ");
    sGetKeyState st;
    QTpkg::self()->RegisterEventHandler(qt_keyb::getkey_event_handler, &st);

    QTdev::self()->MainLoop(false);
    return (st.nkey);
}


// Static function.
bool
cKbMacro::isModifier(unsigned int key)
{
    return (
        key == Qt::Key_Shift    ||
        key == Qt::Key_Control  ||
        key == Qt::Key_Meta     ||
        key == Qt::Key_Alt      ||
        key == Qt::Key_CapsLock);
}


// Static function.
bool
cKbMacro::isControl(unsigned int key)
{
    return (key == Qt::Key_Control);
}


// Static function.
bool
cKbMacro::isShift(unsigned int key)
{
    return (key == Qt::Key_Shift);
}


// Static function.
bool
cKbMacro::isAlt(unsigned int key)
{
    return (key == Qt::Key_Alt);
}


// Static function.
char *
cKbMacro::keyText(unsigned int key, unsigned int state)
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
        strncpy(text, tt, 3);
        text[3] = 0;
    }

    if (key == Qt::Key_Return) {
        text[0] = '\n';
        text[1] = 0;
    }
    return (text);
}


// Static function.
void
cKbMacro::keyName(unsigned int key, char *buf)
{
    QKeySequence ks(key);
    strncpy(buf, ks.toString().toLatin1().constData(), 32);
}


// Static function.
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


// Static function.
// Return true if the given key event can not be mapped.
//
bool
cKbMacro::notMappable(unsigned int key, unsigned int state)
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


namespace {
    Qt::KeyboardModifiers modifiers(unsigned int m)
    {
        Qt::KeyboardModifiers mod = Qt::NoModifier;
        if (m & GR_SHIFT_MASK)
            mod |= Qt::ShiftModifier;
        if (m & GR_CONTROL_MASK)
            mod |= Qt::ControlModifier;
        if (m & GR_ALT_MASK)
            mod |= Qt::AltModifier;
        if (m & GR_KEYPAD_MASK)
            mod |= Qt::KeypadModifier;
        return (mod);
    }

    Qt::MouseButton button(int b)
    {
        if (b == 1)
            return (Qt::LeftButton);
        if (b == 2)
            return (Qt::MiddleButton);
        if (b == 4)
            return (Qt::RightButton);
        return (Qt::NoButton);
    }
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
    QWidget *w = qt_keyb::name_to_widget(k->widget_name);
    if (!w)
        return (false);

    QKeyEvent event(
        k->type == KEY_PRESS ? QEvent::KeyPress : QEvent::KeyRelease,
        k->key, modifiers(k->state), QString(k->text));

    QCoreApplication::sendEvent(w, &event);
    return (true);
}


// Send a button event, return false if the window can't be found.
//
bool
cKbMacro::execBtn(sBtnEvent *b)
{
    if (b->type == BUTTON_PRESS)
        QTpkg::self()->CheckForInterrupt();
    QWidget *w = qt_keyb::name_to_widget(b->widget_name);
    if (!w)
        return (false);

    if (dynamic_cast<const QAbstractButton*>(w)) {
        if (b->type == BUTTON_RELEASE) {
            if (w == km_last_btn) {
                QAbstractButton *btn = dynamic_cast<QAbstractButton*>(w);
                if (btn)
                    btn->click();
            }
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
                if (a)
                    a->activate(QAction::Trigger);
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

        QWidget *widg = dynamic_cast<QWidget*>(w);
        if (widg) {
            QPoint localPos(b->x, b->y);
            QPoint globalPos = widg->mapToGlobal(localPos);
            QMouseEvent event(b->type == BUTTON_PRESS ?
                QEvent::MouseButtonPress : QEvent::MouseButtonRelease,
                localPos, globalPos, button(b->button), button(b->button),
                modifiers(b->state));

            QCoreApplication::sendEvent(w, &event);
        }
    }
    return (true);
}
// End of cKbMacro functions


// Event handler in effect while defining macro key..
//
bool
qt_keyb::getkey_event_handler(QObject *obj, QEvent *ev, void *arg)
{
    sGetKeyState *st = static_cast<sGetKeyState*>(arg);
    if (ev->type() == QEvent::KeyPress) {
        st->prcnt++;
        QKeyEvent *kev = static_cast<QKeyEvent*>(ev);
        if (kev->key() == 0) {
            // May happen if key is unknown, e.g. the Microsoft Windows
            // key.
            return (true);
        }
        if (cKbMacro::isModifier(kev->key())) {
            // Pure modifier press/release, ignore here.
            return (true);
        }
        QByteArray ba = kev->text().toLatin1();
        const char *kstr = ba.constData();
        if (kstr && kstr[0] && kstr[1] == 0 &&
                (kev->key() == Qt::Key_Escape ||
                    kev->key() == Qt::Key_Return) &&
                !(kev->modifiers() &
                    (Qt::ControlModifier | Qt::AltModifier))) {
            // Esc or CR
            // Finished.
            PL()->ErasePrompt();
            QTdev::self()->BreakLoop();
            return (true);
        }
        if (!st->done) {
            if (cKbMacro::notMappable(kev->key(),
                    mod_state(kev->modifiers()))) {
                PL()->ShowPrompt("Not mappable, try another: ");
                return (true);
            }
            st->nkey = KbMac()->AlreadyMapped(kev->key(),
                mod_state(kev->modifiers()));
            if (!st->nkey) {
                st->nkey = new sKeyMap(kev->key(),
                    mod_state(kev->modifiers()), kstr);
            }
            st->done = true;
        }
        return (true);
    }
    if (ev->type() == QEvent::KeyRelease) {
        if (st->prcnt)
            st->prcnt--;
        if (!st->prcnt && st->done)
            QTdev::self()->BreakLoop();
        return (true);
    }
    if (ev->type() == QEvent::MouseButtonPress ||
            ev->type() == QEvent::MouseButtonRelease ||
            ev->type() == QEvent::MouseButtonDblClick) {
        // Ignore mouse presses.
        return (true);
    }

    // Handle event.
    return (false);
}


// Event handler in effect while processing macro definition input.
//
bool
qt_keyb::macro_event_handler(QObject *obj, QEvent *ev, void *arg)
{
    sKeyMap *km = static_cast<sKeyMap*>(arg);
    if (ev->type() == QEvent::KeyRelease) {
        // The string field of the key up event is empty.
        QKeyEvent *kev = dynamic_cast<QKeyEvent*>(ev);
        if (kev->key() == Qt::Key_Return && !(mod_state(kev->modifiers()) &
                (GR_CONTROL_MASK | GR_ALT_MASK))) {
            km->fix_modif();
            QTpkg::self()->RegisterEventHandler(0, 0);
            KbMac()->SaveMacro(km, true);
            return (true);
        }
        if (kev->key() == Qt::Key_Backspace && !(mod_state(kev->modifiers()) &
                (GR_CONTROL_MASK | GR_ALT_MASK)))
            return (true);

        QWidget *widg = qobject_cast<QWidget*>(obj);
        if (!widg || widg->isHidden())
            return (false);
        char *wname = widget_path(widg);
        if (wname) {
            sEvent *nev = new sKeyEvent(wname,
                mod_state(kev->modifiers()), KEY_RELEASE, kev->key());
            if (!kev->text().isNull() && (kev->text().length() == 1))
                *((sKeyEvent*)nev)->text = kev->text().toLatin1()[0];
            km->add_response(nev);
        }
        return (true);
    }
    if (ev->type() == QEvent::KeyPress) {
        QKeyEvent *kev = dynamic_cast<QKeyEvent*>(ev);
        km->lastkey = (!kev->text().isNull() &&
            kev->text().length() == 1) ? kev->text().toLatin1()[0] : 0;
        if (kev->key() == Qt::Key_Return && !(mod_state(kev->modifiers()) &
                (GR_CONTROL_MASK | GR_ALT_MASK)))
            return (true);
        if (kev->key() == Qt::Key_Escape && !(mod_state(kev->modifiers()) &
                (GR_CONTROL_MASK | GR_ALT_MASK))) {
            km->clear_response();
            QTpkg::self()->RegisterEventHandler(0, 0);
            KbMac()->SaveMacro(km, false);
            return (true);
        }
        if (kev->key() == Qt::Key_Backspace && !(mod_state(kev->modifiers()) &
                (GR_CONTROL_MASK | GR_ALT_MASK))) {
            if (!km->response)
                return (true);
            km->clear_last_event();
            km->show();
            return (true);
        }

        QWidget *widg = qobject_cast<QWidget*>(obj);
        if (!widg || widg->isHidden())
            return (false);
        char *wname = widget_path(widg);
        if (wname) {
            sEvent *nev = new sKeyEvent(wname,
                mod_state(kev->modifiers()), KEY_PRESS, kev->key());
            if (!kev->text().isNull() && kev->text().length() == 1)
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
            return (false);
        }
        QMouseEvent *mev = dynamic_cast<QMouseEvent*>(ev);

        QWidget *widg = qobject_cast<QWidget*>(obj);
        if (!widg || widg->isHidden())
            return (false);
        char *wname = widget_path(widg);
        bool do_it = false;
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
            bool ismenu = qobject_cast<QMenuBar*>(obj) ||
                qobject_cast<QMenu*>(obj);
            if (ev->type() == QEvent::MouseButtonPress && ismenu) {
                // show the menu
                km->grabber = widg;
                do_it = true;
            }
            km->add_response(nev);
        }
        return (!do_it);
    }
    if (ev->type() == QEvent::MouseButtonDblClick)
        return (true);
    return (false);
}


// Return the path name for the widget.  Heap allocated.
//
char *
qt_keyb::widget_path(const QWidget *widg)
{
    if (!widg || widg->isHidden())
        return (0);
    char *aa[256];
    int cnt = 0, len = 0;
    while (widg) {
        QByteArray ba = widg->objectName().toLatin1();
        const char *nm = ba.constData();
        if (!nm || !*nm)
            nm = widg->metaObject()->className();
        aa[cnt] = lstring::copy(nm);
        len += strlen(aa[cnt]) + 1;
        widg = widg->parentWidget();
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


// Return a list of widgets that match the path given.
//
qt_keyb::wlist *
qt_keyb::find_widget(const QWidget *widg, const char *path)
{
    if (!widg || widg->isHidden() || !path)
        return (0);
    wlist *w0 = 0;
    const char *t = strchr(path, '.');
    if (!t)
        t = path + strlen(path);
    char name[128];
    strncpy(name, path, t-path);
    name[t-path] = 0;
    QList<QObject*> chl = widg->children();
    int sz = chl.size();
    for (int i = 0; i < sz; i++) {
        QWidget *ww = qobject_cast<QWidget*>(chl[i]);
        if (!ww)
            continue;
        QByteArray ba = ww->objectName().toLatin1();
        const char *nm = ba.constData();
        if (!nm || !*nm)
            nm = ww->metaObject()->className();

        if (!strcmp(nm, name)) {
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
    return (w0);
}


// If the path identifies a widget uniquely, return it.  The path is rooted
// in a top level widget.
//
QWidget *
qt_keyb::name_to_widget(const char *path)
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
    int sz = qwl.size();
    for (int i = 0; i < sz; i++) {
        QWidget *ww = qwl[i];
        if (ww->isHidden())
            continue;
        QByteArray ba = ww->objectName().toLatin1();
        const char *nm = ba.constData();
        if (!nm || !*nm)
            nm = ww->metaObject()->className();
        if (!strcmp(nm, name)) {
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
    QWidget *wfound = 0;
    if (w0 && !w0->next)
        wfound = w0->widget;
    while (w0) {
        wlist *wx = w0->next;
        delete w0;
        w0 = wx;
    }
    return (wfound);
}

