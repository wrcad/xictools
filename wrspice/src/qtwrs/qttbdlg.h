
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

#ifndef QTTBDLG_H
#define QTTBDLG_H

#include "qttoolb.h"

#include <QDialog>
#include <QKeyEvent>


//-----------------------------------------------------------------------------
// QTtbDlg: the 'tool bar' application dialog, contains main menus, etc.

class QMenu;
class QAction;
class QLineEdit;
class QGroupBox;
class QDragEnterEvent;
class QDropEvent;


class QTtbDlg : public QDialog, public QTdraw
{
    Q_OBJECT

public:
    QTtbDlg(int, int);
    ~QTtbDlg();

    // Menu codes.
    enum {
        // File menu.
        MA_file_sel, MA_source, MA_load, MA_save_tools, MA_save_fonts,
        MA_dismiss,
        // Edit menu.
        MA_txt_edit, MA_xic,
        // Help menu.
        MA_help, MA_about, MA_notes };

    void update(ResUpdType = RES_UPD);

    // Don't pop down from Esc press.
    void keyPressEvent(QKeyEvent *ev)
        {
            if (ev->key() != Qt::Key_Escape)
                QDialog::keyPressEvent(ev);
        }

    static QTtbDlg *self()          { return (instPtr); }

protected:
    void closeEvent(QCloseEvent*);

private slots:
    void file_menu_slot(QAction*);
    void edit_menu_slot(QAction*);
    void tools_menu_slot(QAction*);
    void help_menu_slot(QAction*);
    void wr_btn_slot();
    void run_btn_slot();
    void stop_btn_slot();
    void font_changed_slot(int);
    void drag_enter_slot(QDragEnterEvent*);
    void drop_slot(QDropEvent*);

private:
    void revert_focus();
    static int tb_res_timeout(void*);

    QMenu   *tb_file_menu;
    QAction *tb_source_btn;
    QAction *tb_load_btn;
    QMenu   *tb_edit_menu;
    QMenu   *tb_tools_menu;
    QMenu   *tb_help_menu;

    char    *tb_dropfile;
    double  tb_elapsed_start;
    int     tb_clr_1;
    int     tb_clr_2;
    int     tb_clr_3;
    int     tb_clr_4;

    static QTtbDlg *instPtr;
};

#endif

