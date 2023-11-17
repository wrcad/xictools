
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

#ifndef QTZOOM_H
#define QTZOOM_H

#include "main.h"
#include "qtmain.h"
#include <QDialog>

class QLayout;
class QLabel;
class QPushButton;
class QCheckBox;
namespace qtinterf {
    class QTdoubleSpinBox;
}

//-----------------------------------------------------------------------------
//  Pop-up for the Zoom command
//
// Help system keywords used:
//  xic:zoom

class QTzoomDlg : public QDialog, public GRpopup
{
    Q_OBJECT

public:
    QTzoomDlg(QTbag*, WindowDesc*);
    ~QTzoomDlg();

    // GRpopup overrides
    void set_visible(bool visib)
    {
        if (visib)
            show();
        else
            hide();
    }

    void set_transient_for(QWidget *prnt)
    {
        Qt::WindowFlags f = windowFlags();
        setParent(prnt);
#ifdef __APPLE__
        f |= Qt::Tool;
#endif
        setWindowFlags(f);
    }

    void popdown();
    void initialize();
    void update();

private slots:
    void help_btn_slot();
    void y_apply_btn_slot();
    void z_apply_btn_slot();
    void window_apply_btn_slot();
    void dismiss_btn_slot();

private:
    WindowDesc *zm_window;
    QCheckBox *zm_autoy;
    QTdoubleSpinBox *zm_yscale;
    QTdoubleSpinBox *zm_zoom;
    QTdoubleSpinBox *zm_x;
    QTdoubleSpinBox *zm_y;
    QTdoubleSpinBox *zm_wid;
};

#endif

