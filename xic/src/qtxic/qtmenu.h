
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

#ifndef QTMENU_H
#define QTMENU_H

#include "main.h"
#include "menu.h"

#include <QPushButton>
#include <QToolButton>

class QAction;


class QTmenu : public MenuMain
{
public:
    friend class QTmenuConfig;

    QTmenu()
    {
        modalShell = 0;
    }

    void InitMainMenu();
    void InitTopButtonMenu();
    void InitSideButtonMenus();

    // Virtual functions from MenuMain.
    void SetSensGlobal(bool);
    void Deselect(GRobject);
    void Select(GRobject);
    bool GetStatus(GRobject);
    void SetStatus(GRobject, bool);
    void CallCallback(GRobject);
    void Location(GRobject, int*, int*);
    void PointerRootLoc(int*, int*);
    const char *GetLabel(GRobject);
    void SetLabel(GRobject, const char*);
    void SetSensitive(GRobject, bool);
    bool IsSensitive(GRobject);
    void SetVisible(GRobject, bool);
    bool IsVisible(GRobject);
    void DestroyButton(GRobject);
    void SwitchMenu();
    void SwitchSubwMenu(int, DisplayMode);
    GRobject NewSubwMenu(int);
    void SetDDentry(GRobject, int, const char*);
    void NewDDentry(GRobject, const char*);
    void NewDDmenu(GRobject, const char*const*);
    void UpdateUserMenu();
    void HideButtonMenu(bool);
    void DisableMainMenuItem(const char*, const char*, bool);

    // Non-virtual.

    void SetModal(QDialog *w)   { modalShell = w; }
    QDialog *GetModal()         { return (modalShell); }

    static QTmenu *self() { return (dynamic_cast<QTmenu*>(MainMenu())); }

private:
    QDialog     *modalShell;
};

class QTmenuButton : public QToolButton
//XXXclass QTmenuButton : public QPushButton
{
    Q_OBJECT

public:
    QTmenuButton(MenuEnt*, QWidget*);

signals:
    void button_pressed(MenuEnt*);

private slots:
    void pressed_slot() { emit button_pressed(entry); }

private:
    MenuEnt *entry;
};


inline QAction *
action(MenuEnt *ent)
{
    return (static_cast<QAction*>(ent->user_action));
}

#endif

