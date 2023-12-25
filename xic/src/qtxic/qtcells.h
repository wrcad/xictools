
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

#ifndef QTCELLS_H
#define QTCELLS_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class QLabel;
class QPushButton;
class QComboBox;
class QMouseEvent;
class QResizeEvent;
class QMimeData;

class QTcellsDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    QTcellsDlg(GRobject);
    ~QTcellsDlg();

    void update();
    char *get_selection();

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef __APPLE__
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    void end_search()               { QTdev::Deselect(c_searchbtn); }
    static QTcellsDlg *self()       { return (instPtr); }

private slots:
    void clear_btn_slot();
    void tree_btn_slot();
    void open_btn_slot();
    void place_btn_slot();
    void copy_btn_slot();
    void repl_btn_slot();
    void rename_btn_slot();
    void search_btn_slot(bool);
    void flag_btn_slot();
    void info_btn_slot();
    void show_btn_slot(bool);
    void fltr_btn_slot(bool);
    void help_btn_slot();
    void resize_slot(QResizeEvent*);
    void mouse_press_slot(QMouseEvent*);
    void mouse_motion_slot(QMouseEvent*);
    void font_changed_slot(int);
    void save_btn_slot(bool);
    void page_menu_slot(int);
    void dismiss_btn_slot();
    void mode_changed_slot(int);

private:
    void clear(const char*);
    void copy_cell(const char*);
    void replace_instances(const char*);
    void rename_cell(const char*);
    void select_range(int, int);
    stringlist *raw_cell_list(int*, int*, bool);
    char *cell_list(int);
    void update_text(char*);
    void check_sens();

    static void c_clear_cb(bool, void*);
    static ESret c_copy_cb(const char*, void*);
    static void c_repl_cb(bool, void*);
    static ESret c_rename_cb(const char*, void*);
    static void c_filter_cb(cfilter_t*, void*);
    static int c_highlight_idle(void*);
    static ESret c_save_cb(const char*, void*);
    static int c_timeout(void*);

    GRobject    c_caller;
    GRaffirmPopup *c_clear_pop;
    GRaffirmPopup *c_repl_pop;
    GRledPopup  *c_copy_pop;
    GRledPopup  *c_rename_pop;
    QTledDlg    *c_save_pop;
    QTmsgDlg    *c_msg_pop;
    QLabel      *c_label;
    QPushButton *c_clearbtn;
    QPushButton *c_treebtn;
    QPushButton *c_openbtn;
    QPushButton *c_placebtn;
    QPushButton *c_copybtn;
    QPushButton *c_replbtn;
    QPushButton *c_renamebtn;
    QPushButton *c_searchbtn;
    QPushButton *c_flagbtn;
    QPushButton *c_infobtn;
    QPushButton *c_showbtn;
    QPushButton *c_fltrbtn;
    QComboBox   *c_page_combo;
    QComboBox   *c_mode_combo;
    cfilter_t   *c_pfilter;
    cfilter_t   *c_efilter;
    char        *c_copyname;
    char        *c_replname;
    char        *c_newname;
    bool        c_no_select;
    bool        c_no_update;
    bool        c_dragging;
    int         c_drag_x;
    int         c_drag_y;
    int         c_cols;
    int         c_page;
    DisplayMode c_mode;
    int         c_start;
    int         c_end;

    static QTcellsDlg *instPtr;
};

#endif

