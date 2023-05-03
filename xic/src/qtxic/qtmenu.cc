
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
#include "qtmenu.h"
#include "qtmenucfg.h"
#include "dsp_inlines.h"

#include <QAction>
#include <QPushButton>
#include <QMenu>


namespace {
    // Instantiate the menus.
    QTmenu _qt_menu_;
}


void
QTmenu::InitMainMenu()
{
    QTmenuConfig::self()->instantiateMainMenus();
}


void
QTmenu::InitTopButtonMenu()
{
    QTmenuConfig::self()->instantiateTopButtonMenu();
}


void
QTmenu::InitSideButtonMenus()
{
    QTmenuConfig::self()->instantiateSideButtonMenus();
}


void
QTmenu::SetSensGlobal(bool)
{
}


void
QTmenu::Deselect(GRobject obj)
{
    if (obj) {
        QObject *o = (QObject*)obj;
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn && btn->isCheckable())
                btn->setChecked(false);
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a && a->isCheckable())
                a->setChecked(false);
        }
    }
}


void
QTmenu::Select(GRobject obj)
{
    if (obj) {
        QObject *o = (QObject*)obj;
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn && btn->isCheckable())
                btn->setChecked(true);
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a && a->isCheckable())
                a->setChecked(true);
        }
    }
}


bool
QTmenu::GetStatus(GRobject obj)
{
    if (obj) {
        QObject *o = (QObject*)obj;
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn && btn->isCheckable())
                return (btn->isChecked());
            return (true);
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a && a->isCheckable())
                return (a->isChecked());
            return (true);
        }
    }
    return (false);
}


void
QTmenu::SetStatus(GRobject obj, bool set)
{
    if (obj) {
        QObject *o = (QObject*)obj;
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn && btn->isCheckable())
                btn->setChecked(set);
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a && a->isCheckable())
                a->setChecked(set);
        }
    }
}


void
QTmenu::CallCallback(GRobject obj)
{
    if (obj) {
        QObject *o = (QObject*)obj;
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn)
                btn->click();
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a)
                a->activate(QAction::Trigger);
        }
    }
}


// Return the root window coordinates x+width, y of obj.
//
void
QTmenu::Location(GRobject obj, int *x, int *y)
{
    if (obj) {
        QObject *o = (QObject*)obj;
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn) {
                QPoint pt = btn->mapToGlobal(QPoint(0, 0));
                *x = pt.x() + btn->width();
                *y = pt.y();
                return;
            }
        }
        else {
            /*
            QAction *a = dynamic_cast<QAction*>(o);
            if (a)
                a->activate(QAction::Trigger);
            */
            // How to get menu item position?
        }
    }
    *x = 0;
    *y = 0;
}


// Return the pointer position in root window coordinates
//
void
QTmenu::PointerRootLoc(int *x, int *y)
{
    QPoint pt = QCursor::pos();
    *x = pt.x();
    *y = pt.y();
}


const char *
QTmenu::GetLabel(GRobject obj)
{
    static char buf[32];
    if (obj) {
        QObject *o = (QObject*)obj;
        QString qs;
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn)
                qs = btn->text();
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a)
                qs = a->text();
        }
        int n = qs.size();
        if (n > 0) {
            // Need to strip the '&' if present.
            QByteArray b = qs.toLatin1();
            int i = 0;
            for (int j = 0; j < n; j++) {
                if (b[j] != '&') {
                    buf[i++] = b[j];
                    if (i == sizeof(buf)-1)
                        break;
                }
            }
            buf[i] = 0;
            return (buf);
        }
    }
    return (0);
}


void
QTmenu::SetLabel(GRobject obj, const char *label)
{
    if (obj) {
        QObject *o = (QObject*)obj;
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn)
                btn->setText(QString(label));
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a)
                a->setText(QString(label));
        }
    }
}


void
QTmenu::SetSensitive(GRobject obj, bool set)
{
    if (obj) {
        QObject *o = (QObject*)obj;
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn)
                btn->setEnabled(set);
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a)
                a->setEnabled(set);
        }
    }
}


bool
QTmenu::IsSensitive(GRobject obj)
{
    if (obj) {
        QObject *o = (QObject*)obj;
        if (o->isWidgetType()) {
            QWidget *btn = dynamic_cast<QWidget*>(o);
            if (btn)
                return (btn->isEnabled());
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a)
                return (a->isEnabled());
        }
    }
    return (false);
}


void
QTmenu::SetVisible(GRobject obj, bool vis_state)
{
    if (QTdev::exists())
        QTdev::self()->SetVisible(obj, vis_state);
}


bool
QTmenu::IsVisible(GRobject obj)
{
    if (!QTdev::exists())
        return (false);
    return (QTdev::self()->IsVisible(obj));
}


void
QTmenu::DestroyButton(GRobject obj)
{
    (void)obj;
}


void
QTmenu::SwitchMenu()
{
    QTmenuConfig::self()->switch_menu_mode(DSP()->CurMode(), 0);
}


void
QTmenu::SwitchSubwMenu(int wnum, DisplayMode mode)
{
    if (wnum > 0)   
        QTmenuConfig::self()->switch_menu_mode(mode, wnum);
}


GRobject
QTmenu::NewSubwMenu(int wnum)
{
    if (wnum < 1 || wnum >= DSP_NUMWINS)
        return (0);
    QTmenuConfig::self()->instantiateSubwMenus(wnum);
    return (0);
}


// Replace the label for the indx'th menu item with newlabel.
//
void
QTmenu::SetDDentry(GRobject button, int indx, const char *newlabel)
{
    if (!button)
        return;
    QAction *a = (QAction*)button;
    if (!a)
        return;
    QMenu *menu = a->menu();
    if (!menu)
        return;
    QList<QAction*> actions = menu->actions();
    QAction *ai = actions.value(indx);
    if (ai)
        ai->setText(newlabel);
}


// Add a menu entry for label to the end of the list.
//
void
QTmenu::NewDDentry(GRobject button, const char *label)
{
    if (!button || !label)
        return;
    QAction *a = (QAction*)button;
    if (!a)
        return;

    // Window number encoding.
    int i = a->data().toInt();
    int wnum = i >> 16;

    QMenu *menu = a->menu();
    if (menu) {
        a = menu->addAction(label);
        a->setData(wnum << 16);
    }
}


// Rebuild the menu for button.
//
// data set:
// button           "menu"              menu
//
void
QTmenu::NewDDmenu(GRobject button, const char* const *list)
{
    if (!button || !list)
        return;
    QAction *a = (QAction*)button;
    if (!a)
        return;

    // Window number encoding.
    int i = a->data().toInt();
    int wnum = i >> 16;

    QMenu *menu = a->menu();
    if (!menu) {
        menu = new QMenu();
        a->setMenu(menu);
    }

    menu->clear();
    for (const char *const *s = list; *s; s++) {
        if (!strcmp(*s, MENU_SEP_STRING))
            a = menu->addSeparator();
        else
            a = menu->addAction(*s);
        a->setData(wnum << 16);
    }
}


// Update or create the user menu.  This is the only menu that is
// dynamic.
//
void
QTmenu::UpdateUserMenu()
{
    QTmenuConfig::self()->updateDynamicMenus();
}


// Hide or show the side (button) menu.
//
void
QTmenu::HideButtonMenu(bool hide)
{
    if (!QTmainwin::exists())
        return;
    QTmainwin *main_win = QTmainwin::self();

    QWidget *phys_button_box = main_win->PhysButtonBox();
    QWidget *elec_button_box = main_win->ElecButtonBox();
    if (hide) {
        if (phys_button_box)
            phys_button_box->hide();
        if (elec_button_box)
            elec_button_box->hide();
    }
    else {
        if (DSP()->CurMode() == Physical) {
            if (phys_button_box)
                phys_button_box->show();
            if (elec_button_box)
                elec_button_box->hide();
        }
        else {
            if (phys_button_box)
                phys_button_box->hide();
            if (elec_button_box)
                elec_button_box->show();
        }
    }
}


// Disable/enable menus (item is null) or menu entries.
//
void
QTmenu::DisableMainMenuItem(const char *mname, const char *item, bool desens)
{
    MenuBox *mbox = FindMainMenu(mname);
    if (mbox && mbox->menu) {
        if (!item) {
            QWidget *btn = (QWidget*)mbox->menu[0].cmd.caller;
            if (btn)
                btn->setEnabled(!desens);
            return;
        }
        for (MenuEnt *m = mbox->menu; m->entry; m++) {
            if (!strcasecmp(m->entry, item)) {
                QAction *a = (QAction*)m->user_action;
                if (a)
                    a->setEnabled(!desens);
                return;
            }
        }
    }
}
// End of QTmenu functions.


QTmenuButton::QTmenuButton(MenuEnt *ent, QWidget *prnt) : QPushButton(prnt)
{
    setAutoDefault(false);
    if (ent->is_toggle())
        setCheckable(true);
    entry = ent;
    connect(this, SIGNAL(clicked()), this, SLOT(pressed_slot()));
}

