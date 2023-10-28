
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

#ifndef QTXFORM_H
#define QTXFORM_H

#include "main.h"
#include "qtmain.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QPushButton;


class QTxformDlg : public QDialog
{
    Q_OBJECT

public:
    QTxformDlg(GRobject c,
        bool (*)(const char*, bool, const char*, void*), void*);
    ~QTxformDlg();

    void update();

    void set_transient_for(QWidget *prnt)
    {
        Qt::WindowFlags f = windowFlags();
        setParent(prnt);
        setWindowFlags(f);
    }

    static QTxformDlg *self()       { return (instPtr); }

private slots:
    void help_btn_slot();
    void angle_change_slot(int);
    void reflect_x_slot(int);
    void reflect_y_slot(int);
    void magnification_change_slot(double);
    void identity_btn_slot();
    void last_btn_slot();
    void dismiss_btn_slot();

private:
    GRobject tf_caller;
    QCheckBox *tf_rflx;
    QCheckBox *tf_rfly;
    QComboBox *tf_ang;
    QDoubleSpinBox *tf_mag;
    QPushButton *tf_id;
    QPushButton *tf_last;
    QPushButton *tf_cancel;

    bool (*tf_callback)(const char*, bool, const char*, void*);
    void *tf_arg;
    
    static QTxformDlg* instPtr;
};

#endif

