
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

#ifndef QTDRCLIM_H
#define QTDRCLIM_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//------------------------------------------------------------------------
// DRC Limits Pop-Up
//
// This provides entry fields for the various limit parameters and
// the error recording level.

class QCheckBox;
class QLineEdit;
class QRadioButton;
class QSpinBox;

class QTdrcLimitsDlg : public QDialog
{
    Q_OBJECT

public:
    QTdrcLimitsDlg(GRobject);
    ~QTdrcLimitsDlg();

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

    static QTdrcLimitsDlg *self()           { return (instPtr); }

private slots:
    void help_btn_slot();
    void luse_btn_slot(int);
    void lskip_btn_slot(int);
    void ruse_btn_slot(int);
    void rskip_btn_slot(int);
    void max_errs_changed_slot(int);
    void imax_objs_changed_slot(int);
    void imax_time_changed_slot(int);
    void imax_errs_changed_slot(int);
    void skip_btn_slot(int);
    void b1_btn_slot(bool);
    void b2_btn_slot(bool);
    void b3_btn_slot(bool);
    void dismiss_btn_slot();
    void llist_changed_slot(const QString&);
    void rlist_changed_slot(const QString&);

private:
    GRobject    dl_caller;
    QCheckBox   *dl_luse;
    QCheckBox   *dl_lskip;
    QLineEdit   *dl_llist;
    QCheckBox   *dl_ruse;
    QCheckBox   *dl_rskip;
    QLineEdit   *dl_rlist;
    QCheckBox   *dl_skip;
    QRadioButton *dl_b1;
    QRadioButton *dl_b2;
    QRadioButton *dl_b3;
    QSpinBox    *dl_sb_max_errs;
    QSpinBox    *dl_sb_imax_objs;
    QSpinBox    *dl_sb_imax_time;
    QSpinBox    *dl_sb_imax_errs;

    static QTdrcLimitsDlg *instPtr;
};

#endif

