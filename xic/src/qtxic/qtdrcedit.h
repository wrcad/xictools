
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

#ifndef QTDRCEDIT_H
#define QTDRCEDIT_H

#include "main.h"
#include "drc.h"
#include "drc_edit.h"
#include "qtmain.h"

#include <QDialog>

//-----------------------------------------------------------------------------
// Pop up to display a listing of design rules for the current layer.
//

class QAction;
class QMouseEvent;
class QMenu;

class QTdrcRuleEditDlg : public QDialog, public DRCedit
{
    Q_OBJECT

public:
//    edit { emEdit, emInhibit, emDelete, emUndo, emQuit };

    QTdrcRuleEditDlg(GRobject);
    ~QTdrcRuleEditDlg();

    QSize sizeHint() const;
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

    static QTdrcRuleEditDlg *self()         { return (instPtr); }

private slots:
    void edit_menu_slot(QAction*);
    void user_menu_slot(QAction*);
    void rules_menu_slot(QAction*);
    void ruleblk_menu_slot(QAction*);
    void help_slot();
    void mouse_press_slot(QMouseEvent*);
    void font_changed_slot(int);

private:
    void rule_menu_upd();
    void save_last_op(DRCtestDesc*, DRCtestDesc*);
    void select_range(int, int);
    void check_sens();
    static bool dim_cb(const char*, void*);
    static void dim_show_msg(const char*);
    static bool dim_editsave(const char*, void*, XEtype);

    /*
    static void dim_rule_proc(GtkWidget*, void*);
    static void dim_rule_menu_proc(GtkWidget*, void*);
    static int dim_text_btn_hdlr(GtkWidget*, GdkEvent*, void*);
    */

    GRobject    dim_caller;         // initiating button
    QTtextEdit  *dim_text;          // text area
    QAction     *dim_edit;          // Edit/Edit action
    QAction     *dim_inhibit;       // Edit/Iihibit action
    QAction     *dim_del;           // Edit/Delete action
    QAction     *dim_undo;          // Edit/Undo action
    QMenu       *dim_menu;          // rules menu
    QMenu       *dim_umenu;         // user rules menu
    QMenu       *dim_rbmenu;        // rule block menu
    QAction     *dim_delblk;        // rule block delete
    QAction     *dim_undblk;        // rule block undelete
    DRCtestDesc *dim_editing_rule;  // rule selected for editing
    int         dim_start;
    int         dim_end;

    static QTdrcRuleEditDlg *instPtr;
};

#endif

