
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

#ifndef QTDSPWIN_H
#define QTDSPWIN_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTdisplayWinDlg:  Dialog for the CHD Display command.

class QToolButton;
namespace qtinterf {
    class QTdoubleSpinBox;
}

class QTdisplayWinDlg : public QDialog
{
    Q_OBJECT

public:
    QTdisplayWinDlg(GRobject, const BBox*,
        bool(*)(bool, const BBox*, void*), void*);
    ~QTdisplayWinDlg();

#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif

    void update(const BBox*);

    void set_transient_for(QWidget *prnt)
        {
            Qt::WindowFlags f = windowFlags();
            setParent(prnt);
#ifdef Q_OS_MACOS
            f |= Qt::Tool;
#endif
            setWindowFlags(f);
        }

    // Don't pop down from Esc press.
    void keyPressEvent(QKeyEvent *ev)
        {
            if (ev->key() != Qt::Key_Escape)
                QDialog::keyPressEvent(ev);
        }

    static QTdisplayWinDlg *self()          { return (instPtr); }

private slots:
    void apply_btn_slot();
    void center_btn_slot();
    void dismiss_btn_slot();

private:
    GRobject    dw_caller;
    QToolButton *dw_apply;
    QToolButton *dw_center;
    QTdoubleSpinBox *dw_sb_x;
    QTdoubleSpinBox *dw_sb_y;
    QTdoubleSpinBox *dw_sb_wid;

    WindowDesc *dw_window;
    bool (*dw_callback)(bool, const BBox*, void*);
    void *dw_arg;

    static QTdisplayWinDlg *instPtr;
};

#endif

