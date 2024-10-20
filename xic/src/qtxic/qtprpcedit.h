
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

#ifndef QTPRPCEDIT_H
#define QTPRPCEDIT_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTcellPrpDlg:  Dialog to modify proerties of the current cell.

class QAction;
class QMouseEvent;
class QMimeData;
class QToolButton;
class QMenu;

class QTcellPrpDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    struct sAddEnt
    {
        sAddEnt(const char *n, int v) : name(n), value(v) { }

        const char *name;
        int value;
    };

    QTcellPrpDlg();
    ~QTcellPrpDlg();

#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif

    void update();

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef Q_OS_MACOS
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    // Don't pop down from Esc press.
    void keyPressEvent(QKeyEvent *ev)
        {
            if (ev->key() != Qt::Key_Escape)
                QDialog::keyPressEvent(ev);
        }

    static QTcellPrpDlg *self()         { return (instPtr); }

private slots:
    void edit_btn_slot(bool);
    void del_btn_slot(bool);
    void add_menu_slot(QAction*);
    void help_btn_slot();
    void mouse_press_slot(QMouseEvent*);
    void mouse_release_slot(QMouseEvent*);
    void dismiss_btn_slot();
    void font_changed_slot(int);

private:
    PrptyText *get_selection();
    void update_display();
    void select_range(int, int);

    QToolButton *pc_edit;
    QToolButton *pc_del;
    QToolButton *pc_add;
    QMenu       *pc_addmenu;
    PrptyText   *pc_list;

    int pc_line_selected;
    int pc_action_calls;
    int pc_start;
    int pc_end;
    int pc_dspmode;

    static sAddEnt pc_elec_addmenu[];
    static sAddEnt pc_phys_addmenu[];
    static QTcellPrpDlg *instPtr;
};

#endif

