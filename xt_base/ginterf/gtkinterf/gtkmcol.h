
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id: gtkmcol.h,v 2.9 2015/07/31 22:37:01 stevew Exp $
 *========================================================================*/

#ifndef GTKMCOL_H
#define GTKMCOL_H

// Max number of optional buttons.
#define MC_MAXBTNS 3

namespace gtkinterf {
    struct GTKmcolPopup : public GRmcolPopup, public gtk_bag
    {
        GTKmcolPopup(gtk_bag*, stringlist*, const char*, const char**,
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

