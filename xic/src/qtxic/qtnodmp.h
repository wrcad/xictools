
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

#ifndef QTNODMP_H
#define QTNODMP_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTnodeMapDlg::  Dialog which lists nodes in the current cell and
// the associated net names, and allows the ned names to be defined.

namespace ns_nodmp {
    struct NmpState;
}
class QPushButton;
class QLineEdit;
class QRadioButton;
class QTreeWidget;
class QCheckBox;
class QTreeWidgetItem;

using namespace ns_nodmp;

class QTnodeMapDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    QTnodeMapDlg(GRobject, int);
    ~QTnodeMapDlg();

    bool update(int);
    void show_node_terms(int);
    void update_map();

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef __APPLE__
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

    void clear_cmd()                    { nm_cmd = 0; }
    void desel_point_btn()              { QTdev::Deselect(nm_point_btn); }

    static QTnodeMapDlg *self()         { return (instPtr); }

private slots:
    void nophys_btn_slot(bool);
    void mapname_btn_slot(bool);
    void unmap_btn_slot(bool);
    void click_btn_slot(bool);
    void help_btn_slot();
    void srch_btn_slot();
    void srch_text_changed_slot(const QString&);
    void nodesel_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*);
    void termsel_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*);
    void deselect_btn_slot();
    void dismiss_btn_slot();
    void usex_btn_slot(int);
    void find_btn_slot();
    void font_changed_slot(int);

private:
    void enable_point(bool);
    int node_of_row(int);
    void set_name(const char*);
    void do_search(int*, int*);
    int find_row(const char*);
    static void nm_name_cb(const char*, void*);
    static void nm_join_cb(bool, void*);
    static void nm_rm_cb(bool, void*);

    GRobject    nm_caller;
    NmpState    *nm_cmd;
    QPushButton *nm_use_np;;
    QPushButton *nm_rename;
    QPushButton *nm_remove;
    QPushButton *nm_point_btn;
    QPushButton *nm_srch_btn;
    QLineEdit   *nm_srch_entry;
    QRadioButton *nm_srch_nodes;
    QTreeWidget *nm_node_list;
    QTreeWidget *nm_term_list;
    QCheckBox   *nm_usex_btn;
    QPushButton *nm_find_btn;

    int         nm_showing_node;
    int         nm_showing_row;
    int         nm_showing_term_row;
    bool        nm_noupdating;

    CDp_node    *nm_node;
    CDc         *nm_cdesc;

    GRaffirmPopup *nm_rm_affirm;
    GRaffirmPopup *nm_join_affirm;

    static bool nm_use_extract;
    static QTnodeMapDlg *instPtr;
};

#endif

