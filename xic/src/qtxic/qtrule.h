
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

#ifndef QTRULE_H
#define QTRULE_H

#include "main.h"
#include "drc.h"
#include "qtmain.h"

#include <QDialog>


//--------------------------------------------------------------------
// Pop-up to control input parameters
//

class QLabel;
class QLineEdit;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;

class QTruleDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    QTruleDlg(GRobject, DRCtype, const char*, bool(*)(const char*, void*),
        void*, const DRCtestDesc*);
    ~QTruleDlg();

    void update(DRCtype, const char*, const DRCtestDesc*);

    void set_transient_for(QWidget *prnt)
    {
        Qt::WindowFlags f = windowFlags();
        setParent(prnt);
#ifdef __APPLE__
        f |= Qt::Tool;
#endif
        setWindowFlags(f);
    }

    static QTruleDlg *self()            { return (instPtr); }

private slots:
    void help_btn_slot();
    void edit_table_slot();
    void apply_btn_slot();
    void dismiss_btn_slot();

private:
    void alloff();
    void apply();
    static bool ru_edit_cb(const char*, void*, XEtype);

    GRobject    ru_caller;
    bool        (*ru_callback)(const char*, void*);
    void        *ru_arg;
    char        *ru_username;
    char        *ru_stabstr;
    DRCtype     ru_rule;

    QLabel      *ru_label;
    QLabel      *ru_region_la;
    QLineEdit   *ru_region_ent;
    QLabel      *ru_inside_la;
    QLineEdit   *ru_inside_ent;
    QLabel      *ru_outside_la;
    QLineEdit   *ru_outside_ent;
    QLabel      *ru_target_la;
    QLineEdit   *ru_target_ent;
    QLabel      *ru_dimen_la;
    QDoubleSpinBox *ru_dimen_sb;
    QDoubleSpinBox *ru_area_sb;
    QLabel      *ru_diag_la;
    QDoubleSpinBox *ru_diag_sb;
    QLabel      *ru_net_la;
    QDoubleSpinBox *ru_net_sb;
    QCheckBox   *ru_use_st;
    QPushButton *ru_edit_st;
    QLabel      *ru_enc_la;
    QDoubleSpinBox *ru_enc_sb;
    QLabel      *ru_opp_la;
    QDoubleSpinBox *ru_opp_sb1;
    QDoubleSpinBox *ru_opp_sb2;
    QLabel      *ru_user_la;
    QLineEdit   *ru_user_ent;
    QLabel      *ru_descr_la;
    QLineEdit   *ru_descr_ent;

    static QTruleDlg *instPtr;
};

#endif

