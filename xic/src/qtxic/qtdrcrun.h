
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

#ifndef QTDRCRUN_H
#define QTDRCRUN_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//------------------------------------------------------------------------
// DRC Run dialog.
//
// Allowe initiation and control of batch DRC runs.
//

class QPushButon;
class QLineEdit;
class QDoubleSpinBox;
class QCheckBox;
class QLabel;

class QTdrcRunDlg : public QDialog
{
    Q_OBJECT

public:
    QTdrcRunDlg(GRobject);
    ~QTdrcRunDlg();

    void update();
    void update_jobs_list();
    void set_window(const BBox*);
    void unset_set();

    void set_transient_for(QWidget *prnt)
    {
        Qt::WindowFlags f = windowFlags();
        setParent(prnt);
        setWindowFlags(f);
    }

    static QTdrcRunDlg *self()          { return (instPtr); }

private slots:
    void help_btn_slot();
    void use_btn_slot(bool);
    void chd_name_slot(const QString&);
    void cname_slot(const QString&);
    void none_btn_slot(bool);
    void part_changed_slot(double);
    void win_btn_slot(int);
    void flat_btn_slot(int);
    void set_btn_slot(bool);
    void left_changed_slot(double);
    void botm_changed_slot(double);
    void right_changed_slot(double);
    void top_changed_slot(double);
    void check_btn_slot(bool);
    void checkbg_btn_slot(bool);
    void mouse_press_slot(QMouseEvent*);
    void font_changed_slot(int);
    void abort_btn_slot();
    void dismiss_btn_slot();

private:
    void select_range(int, int);
    int get_pid();
    void select_pid(int);
    void run_drc(bool, bool);
    void dc_region();
    void dc_region_quit();

    GRobject    dc_caller;
    QPushButton *dc_use;
    QLineEdit   *dc_chdname;
    QLineEdit   *dc_cname;
    QPushButton *dc_none;
    QDoubleSpinBox *dc_sb_part;
    QCheckBox   *dc_wind;
    QCheckBox   *dc_flat;
    QPushButton *dc_set;
    QLabel      *dc_l_label;
    QLabel      *dc_b_label;
    QLabel      *dc_r_label;
    QLabel      *dc_t_label;
    QDoubleSpinBox *dc_sb_left;
    QDoubleSpinBox *dc_sb_bottom;
    QDoubleSpinBox *dc_sb_right;
    QDoubleSpinBox *dc_sb_top;
    QPushButton *dc_check;
    QPushButton *dc_checkbg;
    QTtextEdit  *dc_jobs;
    QPushButton *dc_kill;

    int         dc_start;
    int         dc_end;
    int         dc_line_selected;

    static double dc_last_part_size;
    static double dc_l_val;
    static double dc_b_val;
    static double dc_r_val;
    static double dc_t_val;
    static bool dc_use_win;
    static bool dc_flatten;
    static bool dc_use_chd;

    static QTdrcRunDlg *instPtr;
};

#endif

