
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "gtkinterf.h"
#include "gtklist.h"
#include "gtkfont.h"
#include "miscutil/pathlist.h"


// Popup to display a list.  title is the title label text, header
// is the first line displayed or 0.  callback is called with
// the word pointed to when the user clicks in the window, and 0
// when the popup is destroyed.
//
// If usepix is set, a two-column clist is used, with the first column
// an open or closed icon.  This is controlled by the first two chars
// of each string (which are stripped):  if one is non-space, the open
// icon is used.
//
// If use_apply is set, the list will have an Apply button, which will
// call the callback with the selection and dismiss.
//
GRlistPopup *
gtk_bag::PopUpList(stringlist *symlist, const char *title,
    const char *header, void (*callback)(const char*, void*), void *arg,
    bool usepix, bool use_apply)
{
    static int list_count;

    GTKlistPopup *list = new GTKlistPopup(this, symlist, title, header,
        usepix, use_apply, arg);
    list->register_callback(callback);

    gtk_window_set_transient_for(GTK_WINDOW(list->Shell()),
        GTK_WINDOW(wb_shell));
    int x, y;
    GRX->ComputePopupLocation(GRloc(), list->Shell(), wb_shell, &x, &y);
    x += list_count*50 - 150;
    y += list_count*50 - 150;
    list_count++;
    if (list_count == 6)
        list_count = 0;
    gtk_widget_set_uposition(list->Shell(), x, y);

    list->set_visible(true);
    return (list);
}


GTKlistPopup::GTKlistPopup(gtk_bag *owner, stringlist *symlist,
    const char *title, const char *header, bool usepix, bool use_apply,
    void *arg)
{
    p_parent = owner;
    p_cb_arg = arg;
    ls_list = 0;
    ls_open_pb = 0;
    ls_close_pb = 0;
    ls_row = -1;
    ls_no_select = false;
    ls_use_pix = usepix;
    ls_use_apply = use_apply;

    if (owner)
        owner->MonitorAdd(this);

    wb_shell = gtk_NewPopup(owner, "Listing", ls_quit_proc, this);
    gtk_window_set_default_size(GTK_WINDOW(wb_shell), 350, 250);

    GtkWidget *form = gtk_table_new(1, 3, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    // Scrolled list.
    //
    GtkWidget *swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_set_border_width(GTK_CONTAINER(swin), 2);

    GtkListStore *store;
    if (ls_use_pix)
        store = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
    else
        store = gtk_list_store_new(1, G_TYPE_STRING);
    ls_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(ls_list), false);
    gtk_widget_show(ls_list);  
    if (ls_use_pix) {
        GtkCellRenderer *rnd = gtk_cell_renderer_pixbuf_new();
        GtkTreeViewColumn *tvcol =
            gtk_tree_view_column_new_with_attributes(0, rnd,  
            "pixbuf", 0, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(ls_list), tvcol);

        rnd = gtk_cell_renderer_text_new(); 
        tvcol =
            gtk_tree_view_column_new_with_attributes(0, rnd,  
            "text", 1, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(ls_list), tvcol);
    }
    else {
        GtkCellRenderer *rnd = gtk_cell_renderer_text_new(); 
        GtkTreeViewColumn *tvcol =
            gtk_tree_view_column_new_with_attributes(0, rnd,  
            "text", 0, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(ls_list), tvcol);
    }

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(ls_list));
    // With SINGLE, one can deselect with Ctrl-click, which doesn't
    // happen with (the default) BROWSE.
    gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
    gtk_tree_selection_set_select_function(sel,
        (GtkTreeSelectionFunc)ls_selection_proc, this, 0);
    // TreeView bug hack, see note with handler.
    gtk_signal_connect(GTK_OBJECT(ls_list), "focus",
        GTK_SIGNAL_FUNC(ls_focus_proc), this);

    gtk_object_set_data(GTK_OBJECT(ls_list), "arg", arg);
    gtk_container_add(GTK_CONTAINER(swin), ls_list);

    // Use a fixed font.
    GTKfont::setupFont(ls_list, FNT_FIXED, true);

    gtk_table_attach(GTK_TABLE(form), swin, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    if (ls_use_pix) {
        ls_open_pb = gdk_pixbuf_new_from_xpm_data(wb_open_folder_xpm);
        ls_close_pb = gdk_pixbuf_new_from_xpm_data(wb_closed_folder_xpm);
    }

    // Dismiss button line.
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    if (ls_use_apply) {
        GtkWidget *button = gtk_button_new_with_label("Apply");
        gtk_widget_set_name(button, "Apply");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(ls_apply_proc), this);
        gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);
    }

    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ls_quit_proc), this);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    update(symlist, title, header);
}


GTKlistPopup::~GTKlistPopup()
{
    if (p_parent) {
        gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }
    if (p_callback)
        (*p_callback)(0, p_cb_arg);
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        GRX->Deselect(p_caller);

    gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
        GTK_SIGNAL_FUNC(ls_quit_proc), this);

    if (ls_open_pb)
        g_object_unref(ls_open_pb);
    if (ls_close_pb)
        g_object_unref(ls_close_pb);
}


// GRpopup override.
//
void
GTKlistPopup::popdown()
{
    if (p_parent) {
        gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// GRlistPopup override.
//
void
GTKlistPopup::update(stringlist *symlist, const char *title,
    const char *header)
{
    if (p_parent) {
        gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }

    char buf[256];
    const char *t;
    if (title && header) {
        snprintf(buf, 256, "%s\n%s", title, header);
        t = buf;
    }
    else
        t = header ? header : title;

    GtkListStore *store =
        GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ls_list)));
    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(ls_list));
    gtk_tree_selection_unselect_all(sel);
    gtk_list_store_clear(store);  
    ls_row = -1;

    GtkTreeIter iter;
    for (stringlist *l = symlist; l; l = l->next) {
        gtk_list_store_append(store, &iter);
        if (ls_use_pix) {
            if (isspace(l->string[0]) && isspace(l->string[1])) {
                gtk_list_store_set(store, &iter, 0, ls_close_pb, 1,
                    l->string+2, -1);
            }
            else {
                gtk_list_store_set(store, &iter, 0, ls_open_pb, 1,
                    l->string+2, -1);
            }
        }
        else
            gtk_list_store_set(store, &iter, 0, l->string, -1);
    }
    if (t) {
        GtkTreeViewColumn *col;
        if (ls_use_pix)
            col = gtk_tree_view_get_column(GTK_TREE_VIEW(ls_list), 1);
        else
            col = gtk_tree_view_get_column(GTK_TREE_VIEW(ls_list), 0);
        gtk_tree_view_column_set_title(col, t);
    }
}


// GRlistPopup override.
//
// In use_pix mode, reset the status of the pixmaps.  A true return
// from the callback is "open".
//
void
GTKlistPopup::update(bool(*cb)(const char*))
{
    if (ls_use_pix && cb) {
        GtkListStore *store =
            GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ls_list)));
        GtkTreeIter iter;
        if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
            return;
        do {
            char *text;
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 1, &text, -1);
            if ((*cb)(text))
                gtk_list_store_set(store, &iter, 0, ls_open_pb, -1);
            else
                gtk_list_store_set(store, &iter, 0, ls_close_pb, -1);
            free(text);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
    }
}


// GRlistPopup override.
//
void
GTKlistPopup::unselect_all()
{
    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(ls_list));
    gtk_tree_selection_unselect_all(sel);
    ls_row = -1;
}



// Selection callback for the list.
//
void
GTKlistPopup::selection_handler(int row)
{
    ls_row = row;
    if (p_callback && !ls_use_apply) {
        GtkListStore *store = GTK_LIST_STORE(
            gtk_tree_view_get_model(GTK_TREE_VIEW(ls_list)));
        GtkTreePath *p = gtk_tree_path_new_from_indices(ls_row, -1);
        GtkTreeIter iter;
        if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, p)) {
            gtk_tree_path_free(p);
            return;
        }
        gtk_tree_path_free(p);
        char *text;
        if (ls_use_pix)
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 1, &text, -1);
        else
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &text, -1);
        if  (text) {
            (*p_callback)(text, p_cb_arg);
            free(text);
        }
    }
}


// Private static GTK signal handler.
//
void
GTKlistPopup::ls_quit_proc(GtkWidget*, void *client_data)
{
    GTKlistPopup *list = static_cast<GTKlistPopup*>(client_data);
    if (list)
        list->popdown();
}


// Private static GTK signal handler.
//
void
GTKlistPopup::ls_apply_proc(GtkWidget*, void *client_data)
{
    GTKlistPopup *list = static_cast<GTKlistPopup*>(client_data);
    if (list && list->ls_list && list->p_callback && list->ls_row >= 0) {
        GtkListStore *store = GTK_LIST_STORE(
            gtk_tree_view_get_model(GTK_TREE_VIEW(list->ls_list)));
        GtkTreePath *p = gtk_tree_path_new_from_indices(list->ls_row, -1);
        GtkTreeIter iter;
        if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, p)) {
            gtk_tree_path_free(p);
            return;
        }
        gtk_tree_path_free(p);
        char *text;
        if (list->ls_use_pix)
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 1, &text, -1);
        else
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &text, -1);
        if  (text) {
            (*list->p_callback)(text, list->p_cb_arg);
            free(text);
            list->popdown();
        }
    }
}


// Private static GTK signal handler.
// List selection callback.
//
bool
GTKlistPopup::ls_selection_proc(GtkTreeSelection*, GtkTreeModel*,
    GtkTreePath *path, bool issel, void *data)
{
    GTKlistPopup *list = static_cast<GTKlistPopup*>(data);
    if (!list)
        return (false);
    if (issel)
        return (true);
    if (list->ls_no_select) {
        // Don't let a focus event select anything!
        list->ls_no_select = false;
        return (false);
    }
    int *indices = gtk_tree_path_get_indices(path);
    if (indices)
        list->selection_handler(*indices);
    return (true);
}


// Static function.
// This handler is a hack to avoid a GtkTreeWidget defect:  when focus
// is taken and there are no selections, the 0'th row will be
// selected.  There seems to be no way to avoid this other than a hack
// like this one.  We set a flag to lock out selection changes in this
// case.
//
bool
GTKlistPopup::ls_focus_proc(GtkWidget*, GdkEvent*, void *data)
{
    GTKlistPopup *list = static_cast<GTKlistPopup*>(data);
    if (list) {
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(list->ls_list));
        // If nothing selected set the flag.
        if (!gtk_tree_selection_get_selected(sel, 0, 0))
            list->ls_no_select = true;
    }
    return (false);
}

