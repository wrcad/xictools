
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

#ifndef QTFLATTEN_H
#define QTFLATTEN_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class QCheckBox;
class QPushButton;
class QAction;

class QTflattenDlg : public QDialog
{
    Q_OBJECT

public:
    QTflattenDlg(GRobject, bool(*)(const char*, bool, const char*, void*),
        void*, int, bool);
    ~QTflattenDlg();

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

    static QTflattenDlg *self()         { return (instPtr); }

private slots:
    void help_btn_slot();
    void depth_menu_slot(const QString&);
    void novias_btn_slot(int);
    void nopcells_btn_slot(int);
    void nolabels_btn_slot(int);
    void fastmode_btn_slot(int);
    void merge_btn_slot(int);
    void go_btn_slot();
    void dismiss_btn_slot();

private:
    GRobject fl_caller;
    QCheckBox *fl_novias;
    QCheckBox *fl_nopcells;
    QCheckBox *fl_nolabels;
    QCheckBox *fl_merge;
    QPushButton *fl_go;

    bool (*fl_callback)(const char*, bool, const char*, void*);
    void *fl_arg;

    static QTflattenDlg *instPtr;
};

#endif

