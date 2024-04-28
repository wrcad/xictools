
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

#ifndef QTMCOL_H
#define QTMCOL_H

#include "qtinterf.h"

#include <QDialog>
#include <QKeyEvent>


class QComboBox;
class QLabel;
class QToolButton;

// Max number of optional buttons.
#define MC_MAXBTNS 3

namespace qtinterf {
    class QTmcolDlg;
}

class qtinterf::QTmcolDlg : public QDialog, public GRmcolPopup, public QTbag
{
    Q_OBJECT

public:
    QTmcolDlg(QTbag*, stringlist*, const char*, const char**, int);
    ~QTmcolDlg();

#ifdef Q_OS_MACOS
    bool event(QEvent*);
#endif

    // GRpopup overrides
    void set_visible(bool visib)
        {
            if (visib)
                show();
            else
                hide();
        }
    void popdown();

    // GRmcolPopup override
    void update(stringlist*, const char*);
    char *get_selection();
    void set_button_sens(int);

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

private slots:
    void save_btn_slot(bool);
    void dismiss_btn_slot();
    void user_btn_slot();
    void page_size_slot(int);
    void resize_slot(QResizeEvent*);
    void mouse_press_slot(QMouseEvent*);
    void mouse_release_slot(QMouseEvent*);
    void mouse_motion_slot(QMouseEvent*);
    void font_changed_slot(int);

private:
    void relist();
    void select_range(int, int);
    static ESret mc_save_cb(const char*, void*);
    static int mc_timeout(void*);

    QComboBox   *mc_pagesel;        // page selection menu if multicol
    QToolButton *mc_buttons[MC_MAXBTNS];
    QTledDlg    *mc_save_pop;
    QTmsgDlg    *mc_msg_pop;
    QLabel      *mc_label;          // title label
    stringlist  *mc_strings;        // list contents
    int         mc_alloc_width;     // visible width
    int         mc_drag_x;          // drag start coord
    int         mc_drag_y;          // drag start coord
    int         mc_page;            // multicol page
    int         mc_pagesize;        // entries per page
    unsigned int mc_btnmask;        // prevent btn selection mask
    int         mc_start;           // selection extent
    int         mc_end;
    bool        mc_dragging;        // possible start of drag/drop
};

#endif

