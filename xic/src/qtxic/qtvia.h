
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

#ifndef QTVIA_H
#define QTVIA_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>

//--------------------------------------------------------------------
// Dialog to create a standard via.
//

class QComboBox;
class QLineEdit;
class QPushButton;
class QDoubleSpinBox;
class QSpinBox;

class QTstdViaDlg : public QDialog
{
    Q_OBJECT

public:
    QTstdViaDlg(GRobject, CDc*);
    ~QTstdViaDlg();

    void update(GRobject, CDc*);

    void set_transient_for(QWidget *prnt)
    {
        Qt::WindowFlags f = windowFlags();
        setParent(prnt);
        setWindowFlags(f);
    }

    static QTstdViaDlg *self()          { return (instPtr); }

private slots:
    void help_btn_slot();
    void name_menu_slot(const QString&);
    void layerv_menu_slot(const QString&);
    void apply_btn_slot();
    void dismiss_btn_slot();

private:
    GRobject        stv_caller;
    QComboBox       *stv_name;
    QLineEdit       *stv_layer1;
    QLineEdit       *stv_layer2;
    QComboBox       *stv_layerv;
    QLineEdit       *stv_imp1;
    QLineEdit       *stv_imp2;
    QPushButton     *stv_apply;

    CDc             *stv_cdesc;

    QDoubleSpinBox  *stv_sb_wid;
    QDoubleSpinBox  *stv_sb_hei;
    QSpinBox        *stv_sb_rows;
    QSpinBox        *stv_sb_cols;
    QDoubleSpinBox  *stv_sb_spa_x;
    QDoubleSpinBox  *stv_sb_spa_y;
    QDoubleSpinBox  *stv_sb_enc1_x;
    QDoubleSpinBox  *stv_sb_enc1_y;
    QDoubleSpinBox  *stv_sb_off1_x;
    QDoubleSpinBox  *stv_sb_off1_y;
    QDoubleSpinBox  *stv_sb_enc2_x;
    QDoubleSpinBox  *stv_sb_enc2_y;
    QDoubleSpinBox  *stv_sb_off2_x;
    QDoubleSpinBox  *stv_sb_off2_y;
    QDoubleSpinBox  *stv_sb_org_x;
    QDoubleSpinBox  *stv_sb_org_y;
    QDoubleSpinBox  *stv_sb_imp1_x;
    QDoubleSpinBox  *stv_sb_imp1_y;
    QDoubleSpinBox  *stv_sb_imp2_x;
    QDoubleSpinBox  *stv_sb_imp2_y;

    static QTstdViaDlg *instPtr;
};

#endif

