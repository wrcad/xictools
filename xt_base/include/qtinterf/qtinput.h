
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

#ifndef QTINPUT_H
#define QTINPUT_H

#include "qtinterf.h"

#include <QVariant>
#include <QDialog>

class QLabel;
class QPushButton;
class QGroupBox;

namespace qtinterf {
    class QTledDlg;
}

class qtinterf::QTledDlg : public QDialog, public GRledPopup
{
    Q_OBJECT

public:
    QTledDlg(QTbag*, const char*, const char*, const char*, bool);
    ~QTledDlg();

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
    void register_caller(GRobject, bool=false, bool=false);
    void popdown();

    // GRledPopup override
    void update(const char*, const char*);

    void set_ignore_return(bool set)    { ed_ign_ret = set; }
    void set_message(const char*);
    void set_text(const char*);

signals:
    void action_call(const char*, void*);

private slots:
    void action_slot();
    void cancel_btn_slot();

private:
    QLabel  *ed_label;
    QWidget *ed_edit;           // QLineEdit or QTextEdit
    bool    ed_multiline;       // true when multiline
    bool    ed_quit_flag;       // set true if Apply pressed
    bool    ed_ign_ret;         // true if ignoring callback return
};

#endif

