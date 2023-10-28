
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

#ifndef QTEDSET_H
#define QTEDSET_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class QCheckBox;
class QComboBox;
class QSpinBox;

class QTeditSetupDlg : public QDialog
{
    Q_OBJECT

public:
    QTeditSetupDlg(GRobject);
    ~QTeditSetupDlg();

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

    static QTeditSetupDlg *self()            { return (instPtr); }

private slots:
    void help_btn_slot();
    void cons45_btn_slot(int);
    void merge_btn_slot(int);
    void noply_btn_slot(int);
    void prompt_btn_slot(int);
    void noww_btn_slot(int);
    void crcovr_btn_slot(int);
    void ulen_changed_slot(int);
    void maxgobs_changed_slot(int);
    void depth_changed_slot(int);
    void dismiss_btn_slot();

private:
    GRobject    ed_caller;
    QCheckBox   *ed_cons45;
    QCheckBox   *ed_merge;
    QCheckBox   *ed_noply;
    QCheckBox   *ed_prompt;
    QCheckBox   *ed_noww;
    QCheckBox   *ed_crcovr;
    QComboBox   *ed_depth;
    QSpinBox    *ed_sb_ulen;
    QSpinBox    *ed_sb_maxgobjs;

    static const char *ed_depthvals[];
    static QTeditSetupDlg *instPtr;
};

#endif

