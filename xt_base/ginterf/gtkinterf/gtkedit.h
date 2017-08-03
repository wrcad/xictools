
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

#ifndef GTKEDIT_H
#define GTKEDIT_H

#include "gtksearch.h"


//
// Text editor pop-up.
//

namespace gtkinterf {

    // Undo list element.
    struct histlist
    {
        histlist(char *t, int p, bool d, histlist *n)
            {
                h_next = n;
                h_text = t;
                h_cpos = p;
                h_deletion = d;
            }

        ~histlist()
            {
                delete [] h_text;
            }

        static void destroy(histlist *l)
            {
                while (l) {
                    histlist *x = l;
                    l = l->h_next;
                    delete x;
                }
            }

        histlist *h_next;
        char *h_text;
        int h_cpos;
        bool h_deletion;
    };

    struct GTKeditPopup : public GReditPopup, public gtk_bag
    {
        // internal widget state
        enum EventType {QUIT, SAVE, SAVEAS, SOURCE, LOAD, TEXTMOD};

        // widget configuration
        enum WidgetType { Editor, Browser, StringEditor, Mailer };

        GTKeditPopup(gtk_bag*, WidgetType type, const char*, bool, void*);
        ~GTKeditPopup();

        // GRpopup overrides
        void set_visible(bool visib)
            {
                if (visib)
                    gtk_widget_show(wb_shell);
                else
                    gtk_widget_hide(wb_shell);
            }
        void popdown();

        WidgetType incarnation()    { return (ed_widget_type); }

        const char *title_label_string()
            {
                return (gtk_frame_get_label(GTK_FRAME(ed_title)));
            }

        void load_file(const char *fname)
            {
                ed_do_load_proc(fname, this);
            }

        // This can be set to provide a default transient-for window.
        static void set_transient_for(GtkWindow *win)
            {
                ed_transient_for = win;
            }

        static GtkWindow *transient_for()       { return (ed_transient_for); }

    private:
        bool register_edit(bool);
        void check_sens();
        bool write_file(const char*, int, int);
        bool read_file(const char*, bool);
        void pop_up_search(int);

        // Gtk signal handlers
        static int ed_btn_hdlr(GtkWidget*,  GdkEvent*, void*);
        static void ed_quit_proc(GtkWidget*, void*);
        static void ed_save_proc(GtkWidget*, void*);
        static void ed_save_as_proc(GtkWidget*, void*);
        static void ed_print_proc(GtkWidget*, void*);
        static void ed_undo_proc(GtkWidget*, void*);
        static void ed_redo_proc(GtkWidget*, void*);
        static void ed_cut_proc(GtkWidget*, void*);
        static void ed_copy_proc(GtkWidget*, void*);
        static void ed_paste_proc(GtkWidget*, void*);
        static void ed_paste_prim_proc(GtkWidget*, void*);
        static void ed_search_proc(GtkWidget*, void*);
        static void ed_font_proc(GtkWidget*, void*);
        static void ed_source_proc(GtkWidget*, void*);
        static void ed_attach_proc(GtkWidget*, void*);
        static void ed_unattach_proc(GtkWidget*, void*);
        static void ed_data_destr(void*);
        static void ed_mail_proc(GtkWidget*, void*);
        static void ed_open_proc(GtkWidget*, void*);
        static void ed_load_proc(GtkWidget*, void*);
        static void ed_read_proc(GtkWidget*, void*);
        static void ed_change_proc(GtkWidget*, void*);
        static void ed_help_proc(GtkWidget*, void*);
        static void ed_drag_data_received(GtkWidget*, GdkDragContext*,
            gint, gint, GtkSelectionData*, guint, guint, void*);
        static void ed_insert_text_proc(GtkTextBuffer*, GtkTextIter*,
            char*, int, void*);
        static void ed_delete_range_proc(GtkTextBuffer*, GtkTextIter*,
            GtkTextIter*, void*);

        // dialog callbacks
        static void ed_do_attach_proc(const char*, void*);
        static void ed_fsel_callback(const char*, void*);
        static void ed_do_saveas_proc(const char*, void*);
        static void ed_do_load_proc(const char*, void*);
        static void ed_do_read_proc(const char*, void*);
        static void ed_set_sens(gtk_bag*, bool, int);

        GtkWidget *ed_title;
        GtkWidget *ed_msg;
        GRfilePopup *ed_fsels[4];  // can have 4 up at once
        GtkItemFactory *ed_item_factory;
        const char *ed_string;
        char *ed_saved_as;
        char *ed_dropfile;
        GTKsearchPopup *ed_search_pop;
        histlist *ed_undo_list;
        histlist *ed_redo_list;
        WidgetType ed_widget_type;
        EventType ed_last_ev;
        bool ed_text_changed;
        bool ed_havesource;
        bool ed_ign_change;
        bool ed_in_undo;

        static GtkWindow *ed_transient_for;
    };
}

#endif

