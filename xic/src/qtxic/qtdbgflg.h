
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

#ifndef QTDBGFLG_H
#define QTDBGFLG_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class QCheckBox;
class QLineEdit;

class cDbgFlags : public QDialog
{
    Q_OBJECT

public:
    cDbgFlags(void*);
    ~cDbgFlags();

    void update();

    static cDbgFlags *self()            { return (instPtr); }

private slots:
    void help_btn_slot();
    void sel_btn_slot(int);
    void undo_btn_slot(int);
    void ldb3d_btn_slot(int);
    void rlsolv_btn_slot(int);
    void editing_finished_slot();
    void lisp_btn_slot(int);
    void connect_btn_slot(int);
    void rlsolvlog_btn_slot(int);
    void group_btn_slot(int);
    void extract_btn_slot(int);
    void assoc_btn_slot(int);
    void verbose_btn_slot(int);
    void load_btn_slot(int);
    void net_btn_slot(int);
    void pcell_btn_slot(int);
    void dismiss_btn_slot();

private:
    GRobject df_caller;
    QCheckBox *df_sel;
    QCheckBox *df_undo;
    QCheckBox *df_ldb3d;
    QCheckBox *df_rlsolv;
    QLineEdit *df_fname;

    QCheckBox *df_lisp;
    QCheckBox *df_connect;
    QCheckBox *df_rlsolvlog;
    QCheckBox *df_group;
    QCheckBox *df_extract;
    QCheckBox *df_assoc;
    QCheckBox *df_verbose;
    QCheckBox *df_load;
    QCheckBox *df_net;
    QCheckBox *df_pcell;

    static cDbgFlags *instPtr;
};

#endif
