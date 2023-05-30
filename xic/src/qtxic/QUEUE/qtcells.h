
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

#ifndef QTCELLS_H
#define QTCELLS_H

#include "main.h"
#include "qtmain.h"


cCells *cCells::instPtr;

class cCells : public QDialog, public QTbag
{
    Q_OBJECT

public:
    cCells(GRobject);
    ~cCells();

    void update();

    char *get_selection() { return (text_get_selection(wb_textarea)); }

    void end_search() { GTKdev::Deselect(c_searchbtn); }
    
    static cCells *self()           { return (instPtr); }

private:
    void clear(const char*);
    void copy_cell(const char*);
    void replace_instances(const char*);
    void rename_cell(const char*);
    int button_hdlr(bool, GtkWidget*, GdkEvent*);
    void select_range(int, int);
    void action_hdlr(GtkWidget*, void*);
    int motion_hdlr(GtkWidget*, GdkEvent*);
    void resize_hdlr(GtkAllocation*);
    stringlist *raw_cell_list(int*, int*, bool);
    char *cell_list(int);
    void update_text(char*);
    void check_sens();

    /*
    static void c_clear_cb(bool, void*);
    static ESret c_copy_cb(const char*, void*);
    static void c_repl_cb(bool, void*);
    static ESret c_rename_cb(const char*, void*);

    static void c_page_proc(GtkWidget*, void*);
    static void c_mode_proc(GtkWidget*, void*);
    static void c_filter_cb(cfilter_t*, void*);
    static int c_highlight_idle(void*);
    static void c_drag_data_get(GtkWidget *widget, GdkDragContext*,
        GtkSelectionData*, guint, guint, void*);
    static int c_btn_hdlr(GtkWidget*, GdkEvent*, void*);
    static int c_btn_release_hdlr(GtkWidget*, GdkEvent*, void*);
    static int c_motion_hdlr(GtkWidget*, GdkEvent*, void*);
    static void c_resize_hdlr(GtkWidget*, GtkAllocation*);
    static void c_realize_hdlr(GtkWidget*, void*);
    static void c_action_proc(GtkWidget*, void*);
    static void c_help_proc(GtkWidget*, void*);
    static void c_cancel(GtkWidget*, void*);
    static void c_save_btn_hdlr(GtkWidget*, void*);
    static ESret c_save_cb(const char*, void*);
    static int c_timeout(void*);
    */

    GRobject c_caller;
    GRaffirmPopup *c_clear_pop;
    GRaffirmPopup *c_repl_pop;
    GRledPopup *c_copy_pop;
    GRledPopup *c_rename_pop;
    GTKledPopup *c_save_pop;
    GTKmsgPopup *c_msg_pop;
    GtkWidget *c_label;
    GtkWidget *c_clearbtn;
    GtkWidget *c_treebtn;
    GtkWidget *c_openbtn;
    GtkWidget *c_placebtn;
    GtkWidget *c_copybtn;
    GtkWidget *c_replbtn;
    GtkWidget *c_renamebtn;
    GtkWidget *c_searchbtn;
    GtkWidget *c_flagbtn;
    GtkWidget *c_infobtn;
    GtkWidget *c_showbtn;
    GtkWidget *c_fltrbtn;
    GtkWidget *c_page_combo;
    GtkWidget *c_mode_combo;
    cfilter_t *c_pfilter;
    cfilter_t *c_efilter;
    char *c_copyname;
    char *c_replname;
    char *c_newname;
    bool c_no_select;
    bool c_no_update;
    bool c_dragging;
    int c_drag_x, c_drag_y;
    int c_cols;
    int c_page;
    DisplayMode c_mode;
    int c_start;
    int c_end;

    static cCells *instPtr;
};

#endif

