
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef QTMSG_H
#define QTMSG_H

#include "ginterf/graphics.h"

#include <QVariant>
#include <QDialog>

class QGroupBox;
class QPushButton;
class QTextEdit;

namespace qtinterf {
    class QTbag;
    class QTmsgDlg;
}

class qtinterf::QTmsgDlg : public QDialog, public GRtextPopup
{
    Q_OBJECT

public:
    QTmsgDlg(QTbag*, const char*, bool err=false, STYtype style=STY_NORM);
    ~QTmsgDlg();

    QSize sizeHint() const;

    // GRpopup overrides
    void set_visible(bool visib)
    {
        if (visib) {
            show();
            raise();
            activateWindow();
        }
        else
            hide();
    }
    void set_desens()               { tx_desens = true; }
    bool is_desens()                { return (tx_desens); }

    // GRtextPopup overrides
    bool get_btn2_state()           { return (false); }
    void set_btn2_state(bool)       { }

    void popdown();
    void setTitle(const char*);
    void setText(const char*);

private slots:
    void dismiss_btn_slot();

private:
    QGroupBox   *tx_gbox;
    QTextEdit   *tx_tbox;
    QPushButton *tx_cancel;
    STYtype     tx_display_style;
    bool        tx_desens;      // If true, parent->wb_inout is disabled.
};

#endif

