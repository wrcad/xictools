
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
 $Id: gtkpfiles.h,v 2.28 2016/06/29 23:38:34 stevew Exp $
 *========================================================================*/

#ifndef GTKPFILES_H
#define GTKPFILES_H

#include "pathlist.h"

//
//  Header for the path file listing composite.
//

#define MAX_BTNS 5


namespace gtkinterf {
    // The core of a path file listing widget.  The widget contains a
    // notebook, each page provides a list of files for a directory in
    // the search path.

    // The static part maintains the listings, even after the widget
    // is destroyed.  There can be only one listing window, since it
    // is referenced by a static pointer.

    struct files_bag : public gtk_bag
    {
        files_bag(gtk_bag*, const char**, int, const char*,
            void(*)(GtkWidget*, void*), int(*)(GtkWidget*, GdkEvent*, void*),
            sPathList*(*)(int), void(*)(GtkWidget*, void*), void(*)(void*));
        ~files_bag();

        void update(const char*, const char** = 0, int = 0,
            void(*)(GtkWidget*, void*) = 0);
        void viewing_area(int, int);
        void relist(stringlist*);
        char *get_selection();
        void resize(GtkAllocation*);
        void select_range(GtkWidget*, int, int);

        const char *get_directory() { return (f_directory); }

        static files_bag *instance() { return (files_bag::f_instptr); }
        static void panic() { files_bag::f_instptr = 0; }

    protected:
        GtkWidget *create_page(sDirList*);

        static void f_resize_hdlr(GtkWidget*, GtkAllocation*, void*);
        static void f_menu_proc(GtkWidget*, void*);
        static void f_page_proc(GtkWidget*, GtkWidget*, int, void*);
        static int f_idle_proc(void*);
        static int f_timer(void*);
        static void f_monitor_setup();
        static void f_drag_data_received(GtkWidget*, GdkDragContext*,
            gint, gint, GtkSelectionData*, guint, guint);
        static void f_source_drag_data_get(GtkWidget*, GdkDragContext*,
            GtkSelectionData*, guint, guint, gpointer);
        static int f_btn_release_hdlr(GtkWidget*, GdkEvent*, void*);
        static int f_motion(GtkWidget*, GdkEvent*, void*);
        static void f_realize_proc(GtkWidget*, void*);
        static void f_unrealize_proc(GtkWidget*, void*);
        static bool f_check_path_and_update(const char*);
        static void f_update_text(GtkWidget*, const char*);
        static void f_selection_get(GtkWidget*, GtkSelectionData*, guint,
            guint, gpointer);
        static int f_selection_clear(GtkWidget*, GdkEventSelection*, void*);

        GtkWidget *f_buttons[MAX_BTNS];
        GtkWidget *f_button_box;
        GtkWidget *f_menu;
        GtkWidget *f_notebook;
        int f_start;
        int f_end;
        bool f_drag_start;      // used for drag/drop
        int f_drag_btn;         // drag button
        int f_drag_x, f_drag_y; // drag start location
        int (*f_btn_hdlr)(GtkWidget*, GdkEvent*, void*);
        void (*f_destroy)(GtkWidget*, void*);
        void (*f_desel)(void*); // deselection notification
        char *f_directory;      // visible directory

        static sPathList *f_path_list;  // the search path struct
        static char *f_cwd;             // the current directory
        static int f_timer_tag;         // timer id for file monitor

        static files_bag *f_instptr;    // instantiation
    };
}

#endif

