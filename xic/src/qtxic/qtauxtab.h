
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

#ifndef QTAUXTAB_H
#define QTAUXTAB_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTauxTabDlg:  Dialog to manage Override Cell Table.

class QPushButton;
class QRadioButton;
class QResizeEvent;
class QMouseEvent;
class QMimeData;

class QTauxTabDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    QTauxTabDlg(GRobject);
    ~QTauxTabDlg();

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

    // Don't pop down from Esc press.
    void keyPressEvent(QKeyEvent *ev)
        {
            if (ev->key() != Qt::Key_Escape)
                QDialog::keyPressEvent(ev);
        }

    static QTauxTabDlg *self()          { return (instPtr); }

private slots:
    void add_btn_slot(bool);
    void rem_btn_slot(bool);
    void clear_btn_slot(bool);
    void help_btn_slot();
    void over_btn_slot(bool);
    void skip_btn_slot(bool);
    void dismiss_btn_slot();
    void resize_slot(QResizeEvent*);
    void mouse_press_slot(QMouseEvent*);
    void mouse_motion_slot(QMouseEvent*);
    void mime_data_handled_slot(const QMimeData*, int*) const;
    void mime_data_delivered_slot(const QMimeData*, int*);
    void font_changed_slot(int);

private:
    void select_range(int, int);
    void check_sens();
    static ESret at_add_cb(const char*, void*);
    static void at_remove_cb(bool, void*);
    static void at_clear_cb(bool, void*);

    GRobject    at_caller;
    QPushButton *at_addbtn;
    QPushButton *at_rembtn;
    QPushButton *at_clearbtn;
    QRadioButton *at_over;
    QRadioButton *at_skip;
    GRledPopup  *at_add_pop;
    GRaffirmPopup *at_remove_pop;
    GRaffirmPopup *at_clear_pop;

    int at_start;
    int at_end;
    int at_drag_x;
    int at_drag_y;
    bool at_dragging;

    static QTauxTabDlg *instPtr;
};

#endif

