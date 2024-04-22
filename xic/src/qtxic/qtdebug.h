
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

#ifndef QTDEBUG_H
#define QTDEBUG_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTscriptDebuggerDlg:  Dialog and supporting functions for script debugging.

// function codes
enum { NoCode, CancelCode, NewCode, LoadCode, PrintCode, SaveAsCode,
    CRLFcode, RunCode, StepCode, StartCode, MonitorCode, HelpCode };

// current status
enum Estatus { Equiescent, Eexecuting };

// configuration mode
enum DBmode { DBedit, DBrun }; 

// Code passed to refresh()
// locStart          scroll to top
// locPresent        keep present position
// locFollowCurrent  keep the current exec. line visible
//
enum locType { locStart, locPresent, locFollowCurrent };

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QToolButton;
class QMenu;
class QAction;
class QMouseEvent;
class QMimeData;
namespace qtinterf {
    class QTsearchDlg;
    class QTbag;
}

// Wariables listing dialog.
//
class QTdbgVarsDlg : public QDialog
{
    Q_OBJECT

public:
    QTdbgVarsDlg(void*, QWidget* = 0);
    ~QTdbgVarsDlg();

#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif

    void update(stringlist*);

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

    void popdown()                  { delete this; }

private slots:
    void current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*);
    void item_activated_slot(QTreeWidgetItem*, int);
    void item_clicked_slot(QTreeWidgetItem*, int);
    void item_selection_changed();
    void dismiss_btn_slot();
    void font_changed_slot(int);

private:
    QTreeWidget *dv_list;
    void        *dv_pointer;
};


// max number of active breakpoints
#define NUMBKPTS 5

// Debugger main window.
//
class QTscriptDebuggerDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    friend class QTdbgVarsDlg;

    // Undo list element.
    struct histlist
    {
        histlist(char *t, int p, bool d, histlist *n)
            {
                h_next = n;
                h_text = t;
                h_cpos = p;
                h_deletion = d;
            }

        ~histlist()
            {
                delete [] h_text;
            }

        static void destroy(const histlist *l)
            {
                while (l) {
                    const histlist *x = l;
                    l = l->h_next;
                    delete x;
                }
            }

        histlist    *h_next;
        char        *h_text;
        int         h_cpos;
        bool        h_deletion;
    };

    // breakpoints
    struct sBp
    {
        sBp() { line = 0; active = false; }

        int line;
        bool active;
    };

    QTscriptDebuggerDlg(GRobject);
    ~QTscriptDebuggerDlg();

    QSize sizeHint() const;

    bool load_from_menu(MenuEnt*);

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

    static QTscriptDebuggerDlg *self()          { return (instPtr); }

private slots:
    void file_menu_slot(QAction*);
    void edit_menu_slot(QAction*);
    void exec_menu_slot(QAction*);
    void options_menu_slot(QAction*);
    void help_slot();
    void mode_btn_slot();
    void mouse_press_slot(QMouseEvent*);
    void mouse_release_slot(QMouseEvent*);
    void key_press_slot(QKeyEvent*);
    void text_changed_slot();
    void text_change_slot(int, int, int);
    void mime_data_handled_slot(const QMimeData*, int*) const;
    void mime_data_delivered_slot(const QMimeData*, int*);
    void font_changed_slot(int);

private:
    void update_variables()
        {
            if (db_vars_pop)
                db_vars_pop->update(db_vlist);
        }

    void check_sens();
    void set_mode(DBmode);
    void set_line();
    bool is_last_line();
    void step();
    void run();
    void set_sens(bool);
    void start();
    void breakpoint(int);
    bool write_file(const char*);
    bool check_save(int);
    void refresh(bool, locType, bool = false);
    char *listing(bool);
    void monitor();
    void open_file();

    static stringlist *db_mklist(const char*);
    static const char *var_prompt(const char*, const char*, bool*);
    static int db_step_idle(void*);
    static ESret db_open_cb(const char*, void*);
    static int db_open_idle(void*);
    static void db_do_saveas_proc(const char*, void*);

    GRobject    db_caller;
    QLabel      *db_modelabel;
    QLabel      *db_title;
    QToolButton *db_modebtn;
    QAction     *db_saveas;
    QAction     *db_undo;
    QAction     *db_redo;
    QMenu       *db_filemenu;
    QMenu       *db_editmenu;
    QMenu       *db_execmenu;
    QAction     *db_load_btn;
    GRledPopup *db_load_pop;
    QTdbgVarsDlg *db_vars_pop;

    char        *db_file_path;
    const char  *db_line_ptr;
    char        *db_line_save;
    char        *db_dropfile;
    FILE        *db_file_ptr;
    stringlist  *db_vlist;
    QTsearchDlg *db_search_pop;

    int         db_line;
    int         db_last_code;
    Estatus     db_status;
    DBmode      db_mode;
    bool        db_text_changed;
    bool        db_row_cb_flag;
    bool        db_in_edit;
    bool        db_in_undo;
    histlist    *db_undo_list;
    histlist    *db_redo_list;
    struct      sBp db_breaks[NUMBKPTS];

    static QTscriptDebuggerDlg *instPtr;
};

#endif

