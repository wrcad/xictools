
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

#ifndef QTJOIN_H
#define QTJOIN_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class QCheckBox;
class QPushButton;
class QLabel;
class QSpinBox;

class QTjoinDlg : public QDialog
{
    Q_OBJECT

public:
    QTjoinDlg(void*);
    ~QTjoinDlg();

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

    static QTjoinDlg *self()            { return (instPtr); }

private slots:
    void help_btn_slot();
    void nolimit_btn_slot(int);
    void mverts_change_slot(int);
    void mgroup_change_slot(int);
    void mqueue_change_slot(int);
    void clean_btn_slot(int);
    void wires_btn_slot(int);
    void join_btn_slot();
    void join_lyr_btn_slot();
    void join_all_btn_slot();
    void split_h_btn_slot();
    void split_v_btn_slot();
    void delete_btn_slot();

private:
    void set_sens(bool);

    GRobject    jn_caller;
    QCheckBox   *jn_nolimit;
    QCheckBox   *jn_clean;
    QCheckBox   *jn_wires;
    QPushButton *jn_join;
    QPushButton *jn_join_lyr;
    QPushButton *jn_join_all;
    QPushButton *jn_split_h;
    QPushButton *jn_split_v;
    QLabel      *jn_mverts_label;
    QLabel      *jn_mgroup_label;
    QLabel      *jn_mqueue_label;
    QSpinBox    *jn_mverts;
    QSpinBox    *jn_mgroup;
    QSpinBox    *jn_mqueue;

    int jn_last_mverts;
    int jn_last_mgroup;
    int jn_last_mqueue;
    int jn_last;

    static QTjoinDlg *instPtr;
};

#endif

