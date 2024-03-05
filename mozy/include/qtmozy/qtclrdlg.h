
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
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef QTCLRDLG_H
#define QTCLRDLG_H

#include "qtinterf/qtinterf.h"

#include <QDialog>

class QLineEdit;

// Default color selection pop-up.
class QTmozyClrDlg : public QDialog, public GRpopup, public QTbag
{
    Q_OBJECT

public:
    QTmozyClrDlg(QWidget* = 0);
    ~QTmozyClrDlg();

    void update();

    void set_visible(bool b)    { setVisible(b); }
    void popdown()              { delete this; }

private slots:
    void colors_btn_slot(bool);
    void apply_btn_slot();
    void quit_btn_slot();

private:
    static void clr_list_callback(const char*, void*);

    QLineEdit *clr_bgclr;
    QLineEdit *clr_bgimg;
    QLineEdit *clr_text;
    QLineEdit *clr_link;
    QLineEdit *clr_vislink;
    QLineEdit *clr_actfglink;
    QLineEdit *clr_imgmap;
    QLineEdit *clr_sel;
    QPushButton *clr_listbtn;
    GRlistPopup *clr_listpop;

    static QTmozyClrDlg *instPtr;
};

#endif

