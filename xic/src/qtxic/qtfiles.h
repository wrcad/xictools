
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

#ifndef QTFILES_H
#define QTFILES_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


//----------------------------------------------------------------------
//  Files Listing Popup
//

#define MAX_BTNS 5

class QPushButton;
class QHBoxLayout;
class QComboBox;
class QStackedWidget;
class QMimeData;
struct sPathList;
struct sDirList;

class QTfilesListDlg : public QDialog, public QTbag
{
    Q_OBJECT

public:
    QTfilesListDlg(GRobject);
    ~QTfilesListDlg();

    QSize sizeHint()                const { return (QSize(500, 400)); }

    void update();
    void update(const char*, const char** = 0, int = 0);
    char *get_selection();

    const char *get_directory()         { return (f_directory); }
    static void panic()                 { instPtr = 0; }
    static QTfilesListDlg *self()       { return (instPtr); }

private slots:
    void button_slot(bool);
    void page_change_slot(int);
    void menu_change_slot(int);
    void font_changed_slot(int);
    void resize_slot(QResizeEvent*);
    void mouse_press_slot(QMouseEvent*);
    void mouse_motion_slot(QMouseEvent*);
    void mime_data_received_slot(const QMimeData*);
    void dismiss_btn_slot();

private:
    void init_viewing_area();
    void relist(stringlist*);
    void select_range(QTtextEdit*, int, int);
    QWidget *create_page(sDirList*);
    bool show_content();
    void set_sensitive(const char*, bool);

    static int f_idle_proc(void*);
    static int f_timer(void*);
    static void f_monitor_setup();
    static bool f_check_path_and_update(const char*);
    static void f_update_text(QTtextEdit*, const char*);

    static sPathList *fl_listing(int);
    static char *fl_is_symfile(const char*);
    static void fl_content_cb(const char*, void*);
    static void fl_down_cb(void*);
    static void fl_desel(void*);

    GRobject    fl_caller;
    QPushButton *f_buttons[MAX_BTNS];
    QHBoxLayout *f_button_box;
    QComboBox   *f_menu;
    QStackedWidget *f_notebook;

    int         f_start;
    int         f_end;
    bool        f_drag_start;   // used for drag/drop
    int         f_drag_btn;     // drag button
    int         f_drag_x;       // drag start location
    int         f_drag_y;
    char        *f_directory;   // visible directory

    char        *fl_selection;
    char        *fl_contlib;
    GRmcolPopup *fl_content_pop;
    cCHD        *fl_chd;
    int         fl_noupdate;

//        void (*f_desel)(void*); // deselection notification
//        int (*f_btn_hdlr)(GtkWidget*, GdkEvent*, void*);
//        void (*f_destroy)(GtkWidget*, void*);

    static sPathList *f_path_list;  // the search path struct
    static char *f_cwd;             // the current directory
    static int f_timer_tag;         // timer id for file monitor
    static const char *nofiles_msg;
    static const char *files_msg;
    static QTfilesListDlg *instPtr;
};

#endif

