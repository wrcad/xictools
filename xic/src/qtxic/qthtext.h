
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

#ifndef QTHTEXT_H
#define QTHTEXT_H

#include "qtmain.h"
#include "cd_hypertext.h"
#include "promptline.h"
#include "qtinterf/qtinterf.h"
#include <QWidget>


//
// Header for the "hypertext" editor and related.
//

class QEnterEvent;
class QToolButton;

// String storage registers, 0 is "last", 1-5 are general.
#define PE_NUMSTORES 6

class QTedit : public QWidget, public cPromptEdit, public QTdraw
{
    Q_OBJECT

public:
    QTedit(bool);
    ~QTedit();

    // virtual overrides
    void flash_msg(const char*, ...);
    void flash_msg_here(int, int, const char*, ...);
    void save_line();
    int win_width(bool = false);
    int win_height();
    void set_focus();
    void set_indicate();
    void show_lt_button(bool);
    void get_selection(bool);
    void *setup_backing(bool);
    void restore_backing(void*);
    void init_window();
    bool check_pixmap();
    void init_selection(bool);
    void warp_pointer();

    cKeys *keys()           { return (pe_keys); }
    int xpos()              { return (pe_colmin*pe_fntwid); }

    static QTedit *self()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

private slots:
    void font_changed_slot(int);
    void recall_menu_slot(QAction*);
    void store_menu_slot(QAction*);
    void long_text_slot();
    void resize_slot(QResizeEvent*);
    void press_slot(QMouseEvent*);
    void release_slot(QMouseEvent*);
    void enter_slot(QEnterEvent*);
    void leave_slot(QEvent*);
    void motion_slot(QMouseEvent*);
    void drag_enter_slot(QDragEnterEvent*);
    void drop_slot(QDropEvent*);
    void keys_press_slot(QMouseEvent*);

private:
    static void on_null_ptr();

    cKeys *pe_keys;
    QToolButton *pe_rcl_btn;
    QToolButton *pe_sto_btn;
    QToolButton *pe_ltx_btn;

    static hyList *pe_stores[PE_NUMSTORES]; // Editor text string registers.
    static QTedit *instancePtr;
};

#endif

