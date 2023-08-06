
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

#ifndef QTTREE_H
#define QTTREE_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QLabel;

// Max number of user buttons.
#define TR_MAXBTNS 5

class QTtreeDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    QTtreeDlg(GRobject, const char*, TreeUpdMode);
    ~QTtreeDlg();

    void update(const char*, const char*, TreeUpdMode);
    char *get_selection();

    // Program is exiting, disable updates.
    static void set_panic()             { instPtr = 0; }

    static QTtreeDlg *self()            { return (instPtr); }

private slots:
    void help_btn_slot();
    void dismiss_btn_slot();
    void user_btn_slot();
    void current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*);
    void item_activated(QTreeWidgetItem*, int);
    void item_changed(QTreeWidgetItem*, int);
    void item_clicked(QTreeWidgetItem*, int);
    void item_collapsed(QTreeWidgetItem*, int);
    void item_expanded(QTreeWidgetItem*, int);
    void font_changed_slot(int);

private:
    bool check_fb();
    void check_sens();
    void build_tree(CDs*);
    void build_tree(cCHD*, symref_t*);
    bool build_tree_rc(CDs*, QTreeWidgetItem*, int);
    bool build_tree_rc(cCHD*, symref_t*, QTreeWidgetItem*, int);

    static int t_build_proc(void*);
    /*
    static int t_select_proc(GtkTreeSelection*, GtkTreeModel*,
        GtkTreePath*, int, void*);
    static bool t_focus_proc(GtkWidget*, GdkEvent*, void*);
    static int t_collapse_proc(GtkTreeView*, GtkTreeIter*,
        GtkTreePath*, void*);
    static void t_cancel(GtkWidget*, void*);
    static void t_action(GtkWidget*, void*);

    static int t_btn_hdlr(GtkWidget*, GdkEvent*, void*);
    static int t_btn_release_hdlr(GtkWidget*, GdkEvent*, void*);
    static int t_motion_hdlr(GtkWidget*, GdkEvent*, void*);
    static void t_drag_data_get(GtkWidget *widget, GdkDragContext*,
        GtkSelectionData*, guint, guint, void*);
    static int t_selection_clear(GtkWidget*, GdkEventSelection*,
        void*);
    static void t_selection_get(GtkWidget*, GtkSelectionData*, guint,
        guint, gpointer);
    */

    GRobject t_caller;          // toggle button initiator
    QLabel *t_label;            // label above listing
    QLabel *t_info;             // label below listing
    QTreeWidget *t_tree;        // tree listing
    QPushButton *t_buttons[TR_MAXBTNS]; // user buttons
    QTreeWidgetItem *t_curnode; // selected node
    char *t_selection;          // selected text
    char *t_root_cd;            // name of root cell
    char *t_root_db;            // name of root cell
    DisplayMode t_mode;         // display mode of cells
    bool t_dragging;            // drag/drop
    bool t_no_select;           // treeview focus hack
    int t_dragX, t_dragY;       // drag/drop
    unsigned int t_ucount;      // user feedback counter
    unsigned int t_udel;        // user feedback increment
    int t_mdepth;               // max depth
    uint64_t t_check_time;      // interval test

    static QTtreeDlg *instPtr;
};

#endif

