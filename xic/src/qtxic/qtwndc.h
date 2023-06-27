
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

#ifndef QTWNDC_H
#define QTWNDC_H

#include "main.h"
#include "qtmain.h"

#include <QGroupBox>

class QCheckBox;
class QLabel;
class QPushButton;
class QMenu;
class QDoubleSpinBox;


//-------------------------------------------------------------------------
// Subwidget group for window control
//

// Sensitivity logic.
enum WndSensMode {
    WndSensAllModes,
    WndSensFlatten,
    WndSensNone
};

// Set which function.
enum WndFuncMode {
    WndFuncCvt,     // Conversion
    WndFuncOut,     // Export
    WndFuncIn       // Import
};

class cWindowCfg : public QGroupBox
{
    Q_OBJECT

public:
    cWindowCfg(WndSensMode(*)(), WndFuncMode);
    ~cWindowCfg();

    void update();
    void set_sens();
    bool get_bb(BBox*);
    void set_bb(const BBox*);

private slots:
    void usewin_btn_slot(int);
    void clip_btn_slot(int);
    void flatten_btn_slot(int);
    void s_menu_slot(QAction*);
    void r_menu_slot(QAction*);
    void left_value_slot(double);
    void right_value_slot(double);
    void bottom_value_slot(double);
    void top_value_slot(double);
    void ecf_pre_btn_slot(int);
    void ecf_post_btn_slot(int);

private:
    QCheckBox   *wnd_use_win;
    QCheckBox   *wnd_clip;
    QCheckBox   *wnd_flatten;
    QLabel      *wnd_ecf_label;
    QCheckBox   *wnd_ecf_pre;
    QCheckBox   *wnd_ecf_post;
    QLabel      *wnd_l_label;
    QLabel      *wnd_b_label;
    QLabel      *wnd_r_label;
    QLabel      *wnd_t_label;
    QPushButton *wnd_sbutton;
    QPushButton *wnd_rbutton;
    QMenu       *wnd_s_menu;
    QMenu       *wnd_r_menu;
    QDoubleSpinBox *wnd_sb_left;
    QDoubleSpinBox *wnd_sb_bottom;
    QDoubleSpinBox *wnd_sb_right;
    QDoubleSpinBox *wnd_sb_top;

    WndSensMode (*wnd_sens_test)();
    WndFuncMode wnd_func_mode;
};

#endif

