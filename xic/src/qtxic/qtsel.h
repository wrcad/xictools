
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

#ifndef QTSEL_H
#define QTSEL_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class QRadioButton;
class QCheckBox;
class QPushButton;

class QTselectDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    QTselectDlg(GRobject);
    ~QTselectDlg();

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

    static QTselectDlg *self()      { return (instPtr); }

private slots:
    void pm_norm_slot(bool);
    void pm_sel_slot(bool);
    void pm_mod_slot(bool);
    void am_norm_slot(bool);
    void am_enc_slot(bool);
    void am_all_slot(bool);
    void sl_norm_slot(bool);
    void sl_togl_slot(bool);
    void sl_add_slot(bool);
    void sl_rem_slot(bool);
    void ob_cell_slot(int);
    void ob_box_slot(int);
    void ob_poly_slot(int);
    void ob_wire_slot(int);
    void ob_label_slot(int);

    void up_btn_slot(bool);
    void help_btn_slot();
    void dismiss_btn_slot();

private:
    GRobject sl_caller;
    QRadioButton *sl_pm_norm;
    QRadioButton *sl_pm_sel;
    QRadioButton *sl_pm_mod;
    QRadioButton *sl_am_norm;
    QRadioButton *sl_am_enc;
    QRadioButton *sl_am_all;
    QRadioButton *sl_sel_norm;
    QRadioButton *sl_sel_togl;
    QRadioButton *sl_sel_add;
    QRadioButton *sl_sel_rem;
    QCheckBox *sl_cell;
    QCheckBox *sl_box;
    QCheckBox *sl_poly;
    QCheckBox *sl_wire;
    QCheckBox *sl_label;
    QPushButton *sl_upbtn;

    static QTselectDlg *instPtr;
};

#endif

