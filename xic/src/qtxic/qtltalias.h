
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

#ifndef QTLTALIAS_H
#define QTLTALIAS_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QCheckBox;

class QTlayerAliasDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    QTlayerAliasDlg(GRobject);
    ~QTlayerAliasDlg();

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

    GRobject call_btn()                 { return (la_calling_btn); }
    static QTlayerAliasDlg *self()      { return (instPtr); }

private slots:
    void open_btn_slot();
    void save_btn_slot();
    void new_btn_slot();
    void del_btn_slot();
    void edit_btn_slot();
    void help_btn_slot();
    void current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*);
    void decimal_btn_slot(int);
    void dismiss_btn_slot();
    void font_changed_slot(int);

private:
    static ESret str_cb(const char*, void*);
    static void yn_cb(bool, void*);

    QPushButton *la_open;
    QPushButton *la_save;
    QPushButton *la_new;
    QPushButton *la_del;
    QPushButton *la_edit;
    QTreeWidget *la_list;
    QCheckBox   *la_decimal;

    GRaffirmPopup *la_affirm;
    GRledPopup  *la_line_edit;
    GRobject    la_calling_btn;
    int         la_row;
    bool        la_show_dec;

    static QTlayerAliasDlg *instPtr;
};

#endif

