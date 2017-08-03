
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef GTKFILE_H
#define GTKFILE_H

struct stringlist;

namespace gtkinterf {
    struct sFsBmap;

    // Main container for the file selection pop-up.
    //
    struct GTKfilePopup : public GRfilePopup, public gtk_bag
    {
        friend struct sFsBmap;
        static char *any_selection();

        GTKfilePopup(gtk_bag*, FsMode, void*, const char*);
        ~GTKfilePopup();

        // GRpopup overrides
        void set_visible(bool visib)
            {
                if (visib)
                    gtk_widget_show(wb_shell);
                else
                    gtk_widget_hide(wb_shell);
            }
        void popdown();

        // GRfilePopup override
        char *get_selection();

    private:
        void init();
        void select_file(const char*);
        void select_dir(GtkTreeIter*);
        void monitor_setup();
        void check_update();
        char *get_path(GtkTreePath*, bool);
        char *get_path(GtkTreeIter*, bool);
        bool insert_node(char*, GtkTreeIter*, GtkTreeIter*);
        void add_dir(GtkTreeIter*, char*);
        char *get_newdir(const char*);
        stringlist *tokenize_filter();
        void list_files();
        char *file_name_at(int, int, int*, int*);
        void set_label();
        void set_bmap();
        void select_row_handler(GtkTreeIter*, bool);
        void menu_handler(GtkWidget*, int);
        bool button_handler(GtkWidget*, GdkEvent*, bool);
        void select_range(int, int);
        stringlist *fs_list_node_children(GtkTreeIter*);
        bool fs_has_child(GtkTreeIter*);
        bool fs_find_child(GtkTreeIter*, const char*, GtkTreeIter*);

        // GTK signal handlers
        static void fs_quit_proc(GtkWidget*, void*);
        static int fs_focus_hdlr(GtkWidget*, GdkEvent*, void*);
        static int fs_open_idle(void*);
        static bool fs_tree_select_proc(GtkTreeSelection*, GtkTreeModel*,
            GtkTreePath*, bool, void*);
        static int fs_sel_test_idle(void*);
        static int fs_tree_collapse_proc(GtkTreeView*, GtkTreeIter*,
            GtkTreePath*, void*);
        static int fs_tree_expand_proc(GtkTreeView*, GtkTreeIter*,
            GtkTreePath*, void*);
        static void fs_upmenu_proc(GtkWidget*, void*);
        static void fs_menu_proc(GtkWidget*, void*, unsigned int);
        static void fs_open_proc(GtkWidget*, void*);
        static void fs_filter_sel_proc(GtkList*, GtkWidget*, void*);
        static void fs_filter_unsel_proc(GtkList*, GtkWidget*, void*);
        static void fs_filter_activate_proc(GtkWidget*, void*);
        static void fs_up_btn_proc(GtkWidget*, void*);
        static void fs_realize_proc(GtkWidget*, void*);
        static void fs_unrealize_proc(GtkWidget*, void*);
        static void fs_resize_proc(GtkWidget*, GtkAllocation*, void*);
        static int fs_button_press_proc(GtkWidget*, GdkEvent*, void*);
        static int fs_button_release_proc(GtkWidget*, GdkEvent*, void*);
        static int fs_motion_proc(GtkWidget*, GdkEvent*, void*);
        static int fs_leave_proc(GtkWidget*, GdkEvent*, void*);
        static void fs_drag_begin(GtkWidget*, GdkDragContext*);
        static void fs_drag_data_received(GtkWidget*, GdkDragContext*, gint,
            gint, GtkSelectionData*, guint, guint);
        static void fs_source_drag_data_get(GtkWidget*, GdkDragContext*,
            GtkSelectionData*, guint, guint, gpointer);
        static int fs_dir_drag_motion(GtkWidget*, GdkDragContext*, gint, gint,
            guint);
        static void fs_dir_drag_leave(GtkWidget*, GdkDragContext*, guint);
        static void fs_selection_get(GtkWidget*, GtkSelectionData*, guint,
            guint, gpointer);
        static int fs_selection_clear(GtkWidget*, GdkEventSelection*, void*);
        static int fs_vertical_timeout(void*);
        static void fs_scroll_hdlr(GtkWidget*);

        // Misc. callbacks.
        static int fs_files_timer(void*);
        static void fs_new_cb(const char*, void*);
        static void fs_delete_cb(bool, void*);
        static void fs_rename_cb(const char*, void*);
        static void fs_root_cb(const char*, void*);
        static void fs_cwd_cb(const char*, void*);

        // Misc. functions
        static char *fs_make_dir(const char*, const char*);
        static void fs_fdiff(stringlist**, stringlist**);
        static stringlist *fs_list_subdirs(const char*);
        static unsigned long fs_dirtime(const char*);

        GtkWidget *fs_tree;
        GtkWidget *fs_label;
        GtkWidget *fs_label_frame;
        GtkWidget *fs_entry;
        GtkWidget *fs_up_btn;
        GtkWidget *fs_go_btn;
        GtkWidget *fs_open_btn;
        GtkWidget *fs_new_btn;
        GtkWidget *fs_delete_btn;
        GtkWidget *fs_rename_btn;
        GtkWidget *fs_anc_btn;
        GtkWidget *fs_filter;
        GtkWidget *fs_scrwin;
        GtkItemFactory *fs_item_factory;

        sFsBmap *fs_bmap;
        GtkTreePath *fs_curnode;
        GtkTreePath *fs_drag_node;
        GtkTreePath *fs_cset_node;
        char *fs_rootdir;
        char *fs_curfile;
        char *fs_cwd_bak;
        int *fs_colwid;

        FsMode fs_type;
        unsigned int fs_mtime;
        int fs_timer_tag;
        int fs_filter_index;
        int fs_alloc_width;
        int fs_drag_btn;
        int fs_drag_x, fs_drag_y;
        int fs_start;
        int fs_end;
        int fs_vtimer;
        int fs_tid;
        bool fs_dragging;
        bool fs_mtime_sort;

        // "show label" states for fsSEL and fsOPEN
        static bool fs_sel_show_label;
        static bool fs_open_show_label;

        // filter strings
        static const char *fs_filter_options[];
    };
}

#endif

