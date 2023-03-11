
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

#ifndef GTKMCOL_H
#define GTKMCOL_H

// Max number of optional buttons.
#define MC_MAXBTNS 3

namespace gtkinterf {
    struct GTKmcolPopup : public GRmcolPopup, public GTKbag
    {
        GTKmcolPopup(GTKbag*, stringlist*, const char*, const char**,
            int, void*);
        ~GTKmcolPopup();

        // GRpopup overrides
        void set_visible(bool visib)
            {
                if (visib)
                    gtk_widget_show(wb_shell);
                else
                    gtk_widget_hide(wb_shell);
            }
        void popdown();

        // GRmcolPopup override
        void update(stringlist*, const char*);
        char *get_selection();
        void set_button_sens(int);

    private:
        void relist();
        void resize_handler(int);
        int button_handler(int, int);
        void select_range(int, int);

        // GTK signal handlers
        static void mc_menu_proc(GtkWidget*, void*);
        static void mc_action_proc(GtkWidget*, void*);
        static void mc_quit_proc(GtkWidget*, void*);
        static int mc_map_hdlr(GtkWidget*, GdkEvent*, void*);
        static void mc_resize_proc(GtkWidget*, GtkAllocation*, void*);
        static int mc_btn_proc(GtkWidget*, GdkEvent*, void*);
        static int mc_btn_release_proc(GtkWidget*, GdkEvent*, void*);
        static int mc_motion_proc(GtkWidget*, GdkEvent*, void*);
        static void mc_source_drag_data_get(GtkWidget*, GdkDragContext*,
            GtkSelectionData*, guint, guint, gpointer);
        static void mc_save_btn_hdlr(GtkWidget*, void*);
        static ESret mc_save_cb(const char*, void*);
        static int mc_timeout(void*);

        GtkWidget *mc_pagesel;      // page selection menu if multicol
        GtkWidget *mc_buttons[MC_MAXBTNS];

        GTKledPopup *mc_save_pop;
        GTKmsgPopup *mc_msg_pop;
        stringlist *mc_strings;     // list contents
        int mc_alloc_width;         // visible width
        int mc_drag_x;              // drag start coord
        int mc_drag_y;              // drag start coord
        int mc_page;                // multicol page
        int mc_pagesize;            // entries per page
        unsigned int mc_btnmask;    // prevent btn selection mask
        int mc_start;               // GTK-2 selection hack
        int mc_end;
        bool mc_dragging;           // possible start of drag/drop
    };
}

#endif

