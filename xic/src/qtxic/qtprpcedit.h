
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


class QAction;
class QMouseEvent;
class QMimeData;
class QPushButton;
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

    void update();

    void set_transient_for(QWidget *prnt)
    {
        Qt::WindowFlags f = windowFlags();
        setParent(prnt);
#ifdef __APPLE__
        f |= Qt::Tool;
#endif
        setWindowFlags(f);
    }

    static QTcellPrpDlg *self()         { return (instPtr); }

private slots:
    void edit_btn_slot(bool);
    void del_btn_slot(bool);
    void add_menu_slot(QAction*);
    void help_btn_slot();
    void mouse_press_slot(QMouseEvent*);
//    void mouse_motion_slot(QMouseEvent*);
//    void mime_data_received_slot(const QMimeData*);
    void dismiss_btn_slot();
    void font_changed_slot(int);

private:
    PrptyText *get_selection();
    void update_display();
    void select_range(int, int);
    void handle_button_down(QMouseEvent*);
    void handle_button_up(QMouseEvent*);
//    void handle_mouse_motion(QMouseEvent*);
//    void handle_mime_data_received(const QMimeData*);

    QPushButton *pc_edit;
    QPushButton *pc_del;
    QPushButton *pc_add;
    QMenu *pc_addmenu;
    PrptyText *pc_list;
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

