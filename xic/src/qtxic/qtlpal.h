
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

#ifndef QTLPAL_H
#define QTLPAL_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//-----------------------------------------------------------------------------
// QTlayerPaletteDlg:  The Layer Palette.

class QPushButton;
class QAction;
class QResizeEvent;
class QMouseEvent;
class QDragEntrEvent;
class QDropEvent;

// Number of layer entries.
#define LP_PALETTE_COLS 5
#define LP_PALETTE_ROWS 3

// Number of text lines at top.
#define LP_TEXT_LINES 5

class QTlayerPaletteDlg : public QDialog, public QTdraw
{
    Q_OBJECT

public:
    QTlayerPaletteDlg(GRobject);
    ~QTlayerPaletteDlg();

    void update_info(CDl*);
    void update_layer(CDl*);

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

    static QTlayerPaletteDlg *self()            { return (instPtr); }

private slots:
    void recall_menu_slot(QAction*);
    void save_menu_slot(QAction*);
    void help_btn_slot();
    void font_changed_slot(int);
    void resize_slot(QResizeEvent*);
    void button_down_slot(QMouseEvent*);
    void button_up_slot(QMouseEvent*);
    void motion_slot(QMouseEvent*);
    void drag_enter_slot(QDragEnterEvent*);
    void drop_slot(QDropEvent*);
    void dismiss_btn_slot();

private:
    void update_user(CDl*, int, int);
    void init_size();
    void redraw();
    void b1_handler(int, int, int, bool);
    void b2_handler(int, int, int, bool);
    void b3_handler(int, int, int, bool);
    CDl *ldesc_at(int, int);
    bool remove(int, int);

    GRobject lp_caller;
    QPushButton *lp_remove;
    QMenu *lp_recall_menu;
    QMenu *lp_save_menu;

    CDl *lp_history[LP_PALETTE_COLS];
    CDl *lp_user[LP_PALETTE_COLS * LP_PALETTE_ROWS];

    int lp_drag_x;
    int lp_drag_y;
    bool lp_dragging;

    int lp_hist_y;
    int lp_user_y;
    int lp_line_height;
    int lp_box_dimension;
    int lp_box_text_spacing;
    int lp_entry_width;

    static QTlayerPaletteDlg *instPtr;
};

#endif

