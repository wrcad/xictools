
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

#ifndef QTOASIS_H
#define QTOASIS_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class QCheckBox;
class QComboBox;
class QPushButton;
class QSpinBox;

class QToasisDlg : public QDialog
{
    Q_OBJECT

public:
    QToasisDlg(GRobject);
    ~QToasisDlg();

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

    GRobject call_btn()                 { return (oas_caller); }
    static QToasisDlg *self()           { return (instPtr); }

private slots:
    void nozoid_btn_slot(int);
    void help_btn_slot();
    void wtob_btn_slot(int);
    void rwtop_btn_slot(int);
    void nogcd_btn_slot(int);
    void oldsort_btn_slot(int);
    void pmask_menu_slot(int);
    void def_btn_slot();
    void cells_btn_slot(int);
    void boxes_btn_slot(int);
    void polys_btn_slot(int);
    void wires_btn_slot(int);
    void labels_btn_slot(int);
    void noruns_btn_slot(bool);
    void entm_changed_slot(int);
    void noarrs_btn_slot(bool);
    void enta_changed_slot(int);
    void entx_changed_slot(int);
    void nosim_btn_slot(bool);
    void entt_changed_slot(int);
    void dismiss_btn_slot();

private:
    void restore_defaults();
    void set_repvar();

    GRobject    oas_caller;
    QCheckBox   *oas_notrap;
    QCheckBox   *oas_wtob;
    QCheckBox   *oas_rwtop;
    QCheckBox   *oas_nogcd;
    QCheckBox   *oas_oldsort;
    QComboBox   *oas_pmask;
    QCheckBox   *oas_objc;
    QCheckBox   *oas_objb;
    QCheckBox   *oas_objp;
    QCheckBox   *oas_objw;
    QCheckBox   *oas_objl;
    QPushButton *oas_def;
    QPushButton *oas_noruns;
    QPushButton *oas_noarrs;
    QPushButton *oas_nosim;
    QSpinBox    *oas_sb_entm;
    QSpinBox    *oas_sb_enta;
    QSpinBox    *oas_sb_entx;
    QSpinBox    *oas_sb_entt;

    int oas_lastm;
    int oas_lasta;
    int oas_lastt;

    static const char *pmaskvals[];
    static QToasisDlg *instPtr;
};

#endif

