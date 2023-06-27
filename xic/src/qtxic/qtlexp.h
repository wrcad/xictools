
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

#ifndef QTLEXP_H
#define QTLEXP_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>

class QRadioButton;
class QComboBox;
class QCheckBox;
class QPushButton;
class QLineEdit;
class QMenu;
class QAction;
class QSpinBox;
class QDoubleSpinBox;

class cLayerExp : public QDialog
{
    Q_OBJECT

public:
    cLayerExp(void*);
    ~cLayerExp();

    void update();

    static cLayerExp *self()            { return (instPtr); }

private slots:
    void help_btn_slot();
    void depth_changed_slot(int);
    void recurse_btn_slot(int);
    void none_btn_slot(bool);
    void part_changed_slot(double);
    void threads_changed_slot(int);
    void deflt_btn_slot(bool);
    void join_btn_slot(bool);
    void split_h_btn_slot(bool);
    void split_v_btn_slot(bool);
    void recall_menu_slot(QAction*);
    void save_menu_slot(QAction*);
    void noclear_btn_slot(int);
    void merge_btn_slot(int);
    void fast_btn_slot(int);
    void eval_btn_slot();
    void dismiss_btn_slot();

private:
    GRobject        lx_caller;
    QRadioButton    *lx_deflt;
    QRadioButton    *lx_join;
    QRadioButton    *lx_split_h;
    QRadioButton    *lx_split_v;
    QComboBox       *lx_depth;
    QCheckBox       *lx_recurse;
    QCheckBox       *lx_merge;
    QCheckBox       *lx_fast;
    QPushButton     *lx_none;
    QLineEdit       *lx_tolayer;
    QCheckBox       *lx_noclear;
    QLineEdit       *lx_lexpr;
    QPushButton     *lx_save;
    QPushButton     *lx_recall;
    QMenu           *lx_save_menu;
    QMenu           *lx_recall_menu;
    QDoubleSpinBox  *lx_sb_part;
    QSpinBox        *lx_sb_thread;

    double lx_last_part_size;

    static char *last_lexpr;
    static int depth_hst;
    static int create_mode;
    static bool fast_mode;
    static bool use_merge;
    static bool do_recurse;
    static bool noclear;

    static cLayerExp *instPtr;
};

#endif

