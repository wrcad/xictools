
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

#ifndef QTEXTDEV_H
#define QTEXTDEV_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-------------------------------------------------------------------------
// Dialog to control device highlighting and selection.
//

class QTreeWidgetItem;
class QTreeWidget;
class QPushButton;
class QLineEdit;
class QCheckBox;

class QTextDevDlg : public QDialog
{
    Q_OBJECT

public:
    QTextDevDlg(GRobject);
    ~QTextDevDlg();

    void update();
    void relist();

    void set_transient_for(QWidget *prnt)
    {
        Qt::WindowFlags f = windowFlags();
        setParent(prnt);
#ifdef __APPLE__
        f |= Qt::Tool;
#endif
        setWindowFlags(f);
    }

    static QTextDevDlg *self()          { return (instPtr); }

private slots:
    void update_btn_slot();
    void showall_btn_slot();
    void eraseall_btn_slot();
    void help_btn_slot();
    void show_btn_slot();
    void erase_btn_slot();
    void current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*);
    void item_activated_slot(QTreeWidgetItem*, int);
    void item_clicked_slot(QTreeWidgetItem*, int);
    void item_selection_changed_slot();
    void enablesel_btn_slot(bool);
    void compute_btn_slot(int);
    void compare_btn_slot(int);
    void measbox_btn_slot(bool);
    void paint_btn_slot();
    void dismiss_btn_slot();
    void font_changed_slot(int);

private:
    GRobject    ed_caller;
    QPushButton *ed_update;
    QPushButton *ed_show;
    QPushButton *ed_erase;
    QPushButton *ed_show_all;
    QPushButton *ed_erase_all;
    QLineEdit   *ed_indices;
    QTreeWidget *ed_list;
    QPushButton *ed_select;
    QCheckBox   *ed_compute;
    QCheckBox   *ed_compare;
    QPushButton *ed_measbox;
    QPushButton *ed_paint;

    char        *ed_selection;
    CDs         *ed_sdesc;
    bool        ed_devs_listed;

    static const char *nodevmsg;
    static QTextDevDlg *instPtr;
};

#endif

