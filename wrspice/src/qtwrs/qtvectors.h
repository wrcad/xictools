
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef QTVECTORS_H
#define QTVECTORS_H

#include "qtinterf/qtinterf.h"

#include <QDialog>
#include <QKeyEvent>


//-----------------------------------------------------------------------------
// QTvectorListDlg:  dialog to display the list of vectors for the
// current plot.  Click on entries to select them, as indicated by '>'
// in the first column.

struct wordlist;

class QTvectorListDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    QTvectorListDlg(int, int, const char*);
    ~QTvectorListDlg();

    // Don't pop down from Esc press.
    void keyPressEvent(QKeyEvent *ev)
        {
            if (ev->key() != Qt::Key_Escape)
                QDialog::keyPressEvent(ev);
        }

    QSize sizeHint() const;
    void update(const char *s);
    wordlist *selections();

    static QTvectorListDlg *self()          { return (instPtr); }

private slots:
    void help_btn_slot();
    void mouse_press_slot(QMouseEvent*);
    void mouse_release_slot(QMouseEvent*);
    void mouse_motion_slot(QMouseEvent*);
    void font_changed_slot(int);
    void button_slot(bool);
    void dismiss_btn_slot();
    void delete_vecs_slot(bool, void*);

private:
    void recolor();

    int vl_x;
    int vl_y;
    GRaffirmPopup *vl_affirm;

    static const char *vl_btns[];
    static QTvectorListDlg *instPtr;
};

#endif

