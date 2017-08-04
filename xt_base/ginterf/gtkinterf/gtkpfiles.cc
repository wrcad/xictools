
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

#include "gtkinterf.h"
#include "gtkfont.h"
#include "gtkpfiles.h"
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>


//------------------------------------------------------------------------
//  Generic Search Path Files Listing Popup
//------------------------------------------------------------------------

namespace {
    // DND support
    GtkTargetEntry target_table[] = {
        { (char*)"STRING",      0, 0 },
        { (char*)"text/plain",  0, 1 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);
}

// Default window size, assumes 6X13 chars, 80 cols, 16 rows
// with a 2-pixel margin
#define DEF_WIDTH 484
#define DEF_HEIGHT 212

// The directories in the path are monitored for changes.
//
files_bag *files_bag::f_instptr = 0;
sPathList *files_bag::f_path_list = 0;
char *files_bag::f_cwd = 0;
int files_bag::f_timer_tag = 0;


// Constructor for files path listing.  The display consists of a
// notebook with a page for each directory in the search path, each
// page contains a text widget listing the files.  There is provision
// for up to MAX_BTNS command buttons.
//
// w                    the "parent" widget bag
// buttons              char array of button labels
// numbuttons           size of the array (<= MAX_BTNS)
// files_msg            printed in a label at the top of the composite
// action_proc          handler for button presses, arg is the index of
//                       the button, starting at 1
// btn_hdlr             event handler for button presses in the text
// files_listing        function to obtain the listing
// destroy              destroy procedure
// desel                deselection notification
//
// The user may explicitly set:
// caller               the initiating toggle button or menu item
//
files_bag::files_bag(gtk_bag *w, const char **buttons, int numbuttons,
    const char *files_msg, void(*action_proc)(GtkWidget*, void*),
    int(*btn_hdlr)(GtkWidget*, GdkEvent*, void*),
    sPathList *(*files_listing)(int), void(*destroy)(GtkWidget*, void*),
    void(*desel)(void*))
{
    if (f_instptr)
        return;
    f_instptr = this;
    for (int i = 0; i < MAX_BTNS; i++)
        f_buttons[i] = 0;
    f_menu = 0;
    f_notebook = 0;
    f_start = 0;
    f_end = 0;
    f_drag_start = false;
    f_drag_btn = 0;
    f_drag_x = f_drag_y = 0;
    f_btn_hdlr = btn_hdlr;
    f_destroy = destroy;
    f_desel = desel;
    f_directory = 0;

    wb_shell = gtk_NewPopup(w, "Path Files Listing", f_destroy, f_instptr);

    GtkWidget *form = gtk_table_new(1, 5, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    f_button_box = hbox;

    if (numbuttons > 0 && buttons) {
        gtk_widget_show(hbox);
        // button row
        //
        if (numbuttons > MAX_BTNS)
            numbuttons = MAX_BTNS;
        for (int i = 0; i < numbuttons; i++) {
            GtkWidget *button = gtk_toggle_button_new_with_label(buttons[i]);
            gtk_widget_set_name(button, buttons[i]);
            gtk_widget_show(button);
            f_buttons[i] = button;
            gtk_signal_connect(GTK_OBJECT(button), "clicked",
                GTK_SIGNAL_FUNC(action_proc), this);
            gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
        }
    }

    int rowcnt = 0;
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // title label
    //
    GtkWidget *label = gtk_label_new(files_msg);
    gtk_widget_show(label);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    int fw, fh;
    if (!GTKfont::stringBounds(FNT_FIXED, 0, &fw, &fh))
        fw = 8;
    int cols = DEF_WIDTH/fw - 2;
    if (!f_path_list) {
        f_path_list = files_listing(cols);
        f_monitor_setup();
    }
    else if (cols != f_path_list->columns()) {
        for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next())
            dl->set_dirty(true);
        f_path_list->set_columns(cols);
        f_idle_proc(0);
    }

    f_menu = gtk_option_menu_new();
    gtk_widget_show(f_menu);
    gtk_table_attach(GTK_TABLE(form), f_menu, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    rowcnt++;

    // creates notebook
    viewing_area(DEF_WIDTH, DEF_HEIGHT);

    if (f_notebook) {
        gtk_table_attach(GTK_TABLE(form), f_notebook, 0, 1, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);
        rowcnt++;
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(f_notebook), false);
    }

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // dismiss button
    //
    GtkWidget *button = gtk_toggle_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(destroy), this);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);
}


files_bag::~files_bag()
{
    if (f_destroy)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(f_destroy), f_instptr);
    f_instptr = 0;
    if (f_path_list) {
        for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next())
            dl->set_dataptr(0);
    }
    delete [] f_directory;
    // shell destroyed in gtk_bag destructor
}


// Update the directory listings (static function).
//
void
files_bag::update(const char *path, const char **buttons, int numbuttons,
    void(*action_proc)(GtkWidget*, void*))
{
    if (path && f_path_list) {
        stringlist *s0 = 0, *se = 0;
        for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next()) {
            if (!s0)
                s0 = se = new stringlist(lstring::copy(dl->dirname()), 0);
            else {
                se->next = new stringlist(lstring::copy(dl->dirname()), 0);
                se = se->next;
            }
        }
        if (f_check_path_and_update(path))
            relist(s0);
        stringlist::destroy(s0);
    }

    if (numbuttons > 0 && buttons) {
        if (numbuttons > MAX_BTNS)
            numbuttons = MAX_BTNS;
        GtkWidget *hbox = f_button_box;
        gtk_widget_hide(hbox);

        for (int i = 0; f_buttons[i]; i++) {
            gtk_widget_destroy(f_buttons[i]);
            f_buttons[i] = 0;
        }
        for (int i = 0; i < numbuttons; i++) {
            GtkWidget *button = gtk_toggle_button_new_with_label(buttons[i]);
            gtk_widget_set_name(button, buttons[i]);
            gtk_widget_show(button);
            f_buttons[i] = button;
            gtk_signal_connect(GTK_OBJECT(button), "clicked",
                GTK_SIGNAL_FUNC(action_proc), this);
            gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
        }
        gtk_widget_show(hbox);
    }
}


// Initialize the notebook of file listings.
//
void
files_bag::viewing_area(int width, int height)
{
    wb_textarea = 0;

    if (f_path_list) {
        for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next())
            dl->set_dataptr(0);
    }
    if (f_notebook == 0) {
        f_notebook = gtk_notebook_new();
        gtk_widget_show(f_notebook);
    }
    else {
        gtk_signal_disconnect_by_func(GTK_OBJECT(f_notebook),
            GTK_SIGNAL_FUNC(f_page_proc), this);
        while (GTK_NOTEBOOK(f_notebook)->children)
            gtk_notebook_remove_page(GTK_NOTEBOOK(f_notebook), 0);
    }
    if (!f_path_list)
        return;

    GtkWidget *menu = gtk_menu_new();
    gtk_widget_show(menu);

    int init_page = 0;
    int maxchars = 120;
    if (f_path_list && f_directory) {
        int i = 0;
        for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next()) {
            if (!strcmp(f_directory, dl->dirname())) {
                init_page = i;
                break;
            }
            i++;
        }
    }
    delete [] f_directory;
    f_directory = 0;

    char buf[256];
    int i = 0;
    for (sDirList *dl = f_path_list->dirs(); dl; i++, dl = dl->next()) {

        if (i == init_page)
            f_directory = lstring::copy(dl->dirname());

        int len = strlen(dl->dirname());
        if (len <= maxchars)
            strcpy(buf, dl->dirname());
        else {
            int partchars = maxchars/2 - 2;
            strncpy(buf, dl->dirname(), partchars);
            strcpy(buf + partchars, " ... ");
            strcat(buf, dl->dirname() + len - partchars);
        }

        GtkWidget *mi = gtk_menu_item_new_with_label(buf);
        gtk_widget_show(mi);
        gtk_object_set_data(GTK_OBJECT(mi), "index", (void*)(long)i);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(f_menu_proc), this);

        GtkWidget *page = create_page(dl);
        gtk_notebook_append_page(GTK_NOTEBOOK(f_notebook), page, 0);
        GtkWidget *nbtext = (GtkWidget*)dl->dataptr();
        if (i == init_page) {
            wb_textarea = nbtext;
            gtk_widget_set_usize(nbtext, width, height);  // Just set one.
        }

    }
    gtk_signal_connect(GTK_OBJECT(f_notebook), "switch-page",
        GTK_SIGNAL_FUNC(f_page_proc), this);
    gtk_notebook_set_page(GTK_NOTEBOOK(f_notebook), init_page);
    gtk_option_menu_remove_menu(GTK_OPTION_MENU(f_menu));
    gtk_option_menu_set_menu(GTK_OPTION_MENU(f_menu), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(f_menu), init_page);
}


namespace {
    // Return the index of str, -1 if not in list.
    //
    int findstr(const char *str, const char **ary, int len)
    {
        if (str && ary && len > 0) {
            for (int i = 0; i < len; i++) {
                if (ary[i] && !strcmp(str, ary[i]))
                    return (i);
            }
        }
        return (-1);
    }

    // Gtk doesn't seem to have this.
    //
    GtkWidget *get_nth_item(GtkMenu *menu, int n)
    {
        GList *list = gtk_container_get_children(GTK_CONTAINER(menu));
        int i = 0;
        for (GList *g = list; g; g = g->next) {
            if (i == n) {
                GtkWidget *mi = (GtkWidget*)g->data;
                g_list_free(list);
                return (mi);
            }
            i++;
        }
        g_list_free(list);
        return (0);
    }
}


// Reset the notebook listings.  The f_path_list has already been set. 
// The argument is the old path list, which still represents the state
// of the menu and pages.
//
void
files_bag::relist(stringlist *oldlist)
{
    if (!oldlist) {
        // This is the original way of doing things, just rebuild
        // everything.

        int width = DEF_WIDTH;
        int height = DEF_HEIGHT;
        GtkWidget *text1 =
            (GtkWidget*)gtk_object_get_data(GTK_OBJECT(wb_shell), "text1");
        if (text1 && text1->window)
            gdk_window_get_size(text1->window, &width, &height);
        viewing_area(width, height);
        return;
    }

    // New way:  keep what we already have, just add new directories. 
    // This is faster, and retains the selection.

    // Put the list in an array, easier to work with.
    int len = stringlist::length(oldlist);
    const char **ary = new const char*[len];
    stringlist *stmp = oldlist;
    for (int i = 0; i < len; i++) {
        ary[i] = stmp->string;
        stmp = stmp->next;
    }

    GtkWidget *menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(f_menu));

    int n = 0;
    for (sDirList *dl = f_path_list->dirs(); dl; n++, dl = dl->next()) {
        int oldn = findstr(dl->dirname(), ary, len);
        if (oldn == n)
            continue;
        if (oldn > 0) {
            // Directory moved to a new location in the path.  Make
            // the corresponding change to the notebook, menu, and the
            // array.

            GtkWidget *pg = gtk_notebook_get_nth_page(
                GTK_NOTEBOOK(f_notebook), oldn);
            gtk_notebook_reorder_child(GTK_NOTEBOOK(f_notebook), pg, n);

            GtkWidget *itm = get_nth_item(GTK_MENU(menu), oldn);
            gtk_menu_reorder_child(GTK_MENU(menu), itm, n);

            const char *t = ary[oldn];
            // We know that oldn is larger than n.
            for (int i = oldn; i > n; i--)
                ary[i] = ary[i-1];
            ary[n] = t;
            continue;
        }
        // Directory wasn't found in the old list, insert a new page
        // and menu entry, and add to the array.

        GtkWidget *pg = create_page(dl);
        gtk_notebook_insert_page(GTK_NOTEBOOK(f_notebook), pg, 0, n);

        char buf[256];
        int maxchars = 120;
        int dlen = strlen(dl->dirname());
        if (dlen <= maxchars)
            strcpy(buf, dl->dirname());
        else {
            int partchars = maxchars/2 - 2;
            strncpy(buf, dl->dirname(), partchars);
            strcpy(buf + partchars, " ... ");
            strcat(buf, dl->dirname() + dlen - partchars);
        }

        GtkWidget *mi = gtk_menu_item_new_with_label(buf);
        gtk_widget_show(mi);
        gtk_menu_insert(GTK_MENU(menu), mi, n);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(f_menu_proc), this);

        const char **nary = new const char*[len+1];
        for (int i = 0; i < n; i++)
            nary[i] = ary[i];
        nary[n] = dl->dirname();
        for (int i = n; i < len; i++)
            nary[i+1] = ary[i];
        len++;
        delete [] ary;
        ary = nary;
    }
    delete [] ary;

    // Anything at position n or above is not in the list so should be
    // deleted.

    while (gtk_notebook_get_nth_page(GTK_NOTEBOOK(f_notebook), n) != 0)
        gtk_notebook_remove_page(GTK_NOTEBOOK(f_notebook), n);
    GtkWidget *itm;
    while ((itm = get_nth_item(GTK_MENU(menu), n)) != 0)
        gtk_container_remove(GTK_CONTAINER(menu), itm);
}


// Return the full path name of the selected file, or 0 if no
// selection.
//
char *
files_bag::get_selection()
{
    if (!f_directory || !*f_directory)
        return (0);
    int start, end;
    text_get_selection_pos(wb_textarea, &start, &end);
    if (start == end)
        return (0);
    char *s = text_get_chars(wb_textarea, start, end);
    if (s) {
        if (*s) {
            sLstr lstr;
            lstr.add(f_directory);
            lstr.add_c('/');
            lstr.add(s);
            delete [] s;
            return (lstr.string_trim());
        }
        delete [] s;
    }
    return (0);
}


// Handle a resize event.
//
void
files_bag::resize(GtkAllocation *a)
{
    int fw, fh;
    if (!GTKfont::stringBounds(FNT_FIXED, 0, &fw, &fh))
        fw = 8;
    int cols = a->width/fw - 2;
    if (cols != f_path_list->columns()) {
        for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next())
            dl->set_dirty(true);
        f_path_list->set_columns(cols);
        f_idle_proc(0);
    }
}


// Select the chars in the range, start=end deselects existing.  In
// GTK-1, selecting gives blue inverse, which turns gray if
// unselected, retaining an indication for the buttons.  GTK-2
// doesn't do this automatically so we provide something similar here.
//
void
files_bag::select_range(GtkWidget *caller, int start, int end)
{
    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(caller));
    GtkTextIter istart, iend;
    if (f_end != f_start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, f_start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, f_end);
        gtk_text_buffer_remove_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    text_select_range(caller, start, end);
    if (end != start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, end);
        gtk_text_buffer_apply_tag_by_name(textbuf, "primary", &istart, &iend);
        gtk_selection_owner_set(caller, GDK_SELECTION_PRIMARY,
            GDK_CURRENT_TIME);
    }
    f_start = start;
    f_end = end;
    if (f_desel)
        (*f_desel)(this);
}


GtkWidget *
files_bag::create_page(sDirList *dl)
{

    GtkWidget *vtab = gtk_table_new(1, 2, false);
    gtk_widget_show(vtab);
    int rowcnt = 0;

    //
    // scrolled text area
    //
    GtkWidget *contr, *nbtext;
    text_scrollable_new(&contr, &nbtext, FNT_FIXED);
    dl->set_dataptr(nbtext);

    text_set_chars(nbtext, dl->dirfiles());

    if (f_btn_hdlr) {
        gtk_widget_add_events(nbtext, GDK_BUTTON_PRESS_MASK);
        gtk_signal_connect(GTK_OBJECT(nbtext), "button-press-event",
            GTK_SIGNAL_FUNC(f_btn_hdlr), this);
    }
    gtk_signal_connect(GTK_OBJECT(nbtext), "button-release-event",
        GTK_SIGNAL_FUNC(f_btn_release_hdlr), this);
    gtk_signal_connect(GTK_OBJECT(nbtext), "motion-notify-event",
        GTK_SIGNAL_FUNC(f_motion), this);
    gtk_signal_connect_after(GTK_OBJECT(nbtext), "realize",
        GTK_SIGNAL_FUNC(f_realize_proc), this);

    gtk_signal_connect(GTK_OBJECT(nbtext), "unrealize",
        GTK_SIGNAL_FUNC(f_unrealize_proc), this);

    // Gtk-2 is tricky to overcome internal selection handling.
    // Must remove clipboard (in f_realize_proc), and explicitly
    // call gtk_selection_owner_set after selecting text.  Note
    // also that explicit clear-event handling is needed.

    gtk_selection_add_targets(nbtext, GDK_SELECTION_PRIMARY,
        target_table, n_targets);
    gtk_signal_connect(GTK_OBJECT(nbtext), "selection-clear-event",
        GTK_SIGNAL_FUNC(f_selection_clear), 0);
    gtk_signal_connect(GTK_OBJECT(nbtext), "selection-get",
        GTK_SIGNAL_FUNC(f_selection_get), 0);

    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(nbtext));
    const char *bclr = GRpkgIf()->GetAttrColor(GRattrColorLocSel);
    gtk_text_buffer_create_tag(textbuf, "primary",
        "background", bclr, NULL);
    gtk_table_attach(GTK_TABLE(vtab), contr, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);

    // drag source (starts explicitly)
    gtk_signal_connect(GTK_OBJECT(nbtext), "drag-data-get",
        GTK_SIGNAL_FUNC(f_source_drag_data_get), this);

    // drop site
    gtk_drag_dest_set(nbtext, GTK_DEST_DEFAULT_ALL, target_table,
        n_targets,
        (GdkDragAction)(GDK_ACTION_COPY | GDK_ACTION_MOVE |
         GDK_ACTION_LINK | GDK_ACTION_ASK));
    gtk_signal_connect_after(GTK_OBJECT(nbtext), "drag-data-received",
        GTK_SIGNAL_FUNC(f_drag_data_received), this);
    return (vtab);
}


// Static private function.
void
files_bag::f_resize_hdlr(GtkWidget *widget, GtkAllocation *a, void *arg)
{
    if (GTK_WIDGET_REALIZED(widget)) {
        files_bag *f = static_cast<files_bag*>(arg);
        if (f)
            f->resize(a);
    }
}


// Static private function.
void
files_bag::f_menu_proc(GtkWidget *caller, void *arg)
{
    files_bag *f = (files_bag*)arg;
    GtkWidget *menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(f->f_menu));
    GList *list = gtk_container_get_children(GTK_CONTAINER(menu));
    int n = -1;
    int i = 0;
    for (GList *g = list; g; g = g->next) {
        if (g->data == caller) {
            n = i;
            break;
        }
        i++;
    }
    g_list_free(list);
    gtk_notebook_set_page(GTK_NOTEBOOK(f->f_notebook), n);
}


// Static private function.
void
files_bag::f_page_proc(GtkWidget*, GtkWidget*, int pagenum, void *arg)
{
    files_bag *f = (files_bag*)arg;
    int i = 0;
    for (sDirList *dl = f_path_list->dirs(); dl; i++, dl = dl->next()) {
        if (i == pagenum && dl->dataptr()) {
            delete [] f->f_directory;
            f->f_directory = lstring::copy(dl->dirname());
            if (f->wb_textarea)
                f->select_range(f->wb_textarea, 0, 0);
            f->wb_textarea = GTK_WIDGET(dl->dataptr());
            f->select_range(f->wb_textarea, 0, 0);
            break;
        }
    }
}


//
// The following functions implement polling to keep the directory
// listing current.
//

// Static private function.
// Idle procedure to update the file list.
//
int
files_bag::f_idle_proc(void*)
{
    for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next()) {
        if (dl->dirty()) {
            sFileList fl(dl->dirname());
            fl.read_list(f_path_list->checkfunc(), f_path_list->incldirs());
            int *colwid;
            char *txt = fl.get_formatted_list(f_path_list->columns(),
                false, f_path_list->no_files_msg(), &colwid);
            dl->set_dirfiles(txt, colwid);
            dl->set_dirty(false);
            if (f_instptr) {
                f_update_text((GtkWidget*)dl->dataptr(), dl->dirfiles());
                if ((GtkWidget*)dl->dataptr() == f_instptr->wb_textarea) {
                    if (f_instptr->f_desel)
                        (*f_instptr->f_desel)(f_instptr);
                }
            }
        }
    }
    return (false);
}


// Static private function.
// Check if directory has been modified, and set dirty flag if so.
//
int
files_bag::f_timer(void*)
{
    // If the cwd changes, update everything
    char *cwd = getcwd(0, 0);
    if (cwd) {
        if (!f_cwd || strcmp(f_cwd, cwd)) {
            delete [] f_cwd;
            f_cwd = lstring::tocpp(cwd);
            if (f_instptr)
                f_instptr->update(f_path_list->path_string());
            return (true);
        }
        else
            free(cwd);
    }

    bool dirtyone = false;
    for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next()) {
        if (dl->mtime() != 0) {
            struct stat st;
            if (stat(dl->dirname(), &st) == 0 && dl->mtime() != st.st_mtime) {
                dl->set_dirty(true);
                dl->set_mtime(st.st_mtime);
                dirtyone = true;
            }
        }
    }
    if (dirtyone)
        gtk_idle_add((GtkFunction)f_idle_proc, 0);

    return (true);
}


// Static private function.
// Set up monitoring of the directories in the path.
//
void
files_bag::f_monitor_setup()
{
    if (!f_cwd)
        f_cwd = lstring::tocpp(getcwd(0, 0));

    for (sDirList *d = f_path_list->dirs(); d; d = d->next()) {
        struct stat st;
        if (stat(d->dirname(), &st) == 0)
            d->set_mtime(st.st_mtime);
    }
    if (!f_timer_tag)
        f_timer_tag = gtk_timeout_add(1000, (GtkFunction)f_timer, 0);
}


//
// The following functions implement drag/drop capability.
//

// Static private function.
// Receive drop data (a path name).
//
void
files_bag::f_drag_data_received(GtkWidget*, GdkDragContext *context,
    gint, gint, GtkSelectionData *data, guint, guint time)
{
    char *src = (char*)data->data;
    if (src && *src && f_instptr->wb_textarea) {
        const char *dst = f_instptr->f_directory;
        if (dst && *dst && strcmp(src, dst)) {
            gtk_DoFileAction(f_instptr->Shell(), src, dst, context, true);
            gtk_drag_finish(context, true, false, time);
            return;
        }
    }
    gtk_drag_finish(context, false, false, time);
}


// Static private function.
// Set the drag data to the selected file path.
//
void
files_bag::f_source_drag_data_get(GtkWidget *caller, GdkDragContext*,
    GtkSelectionData *selection_data, guint, guint, gpointer)
{
    if (GTK_IS_TEXT_VIEW(caller))
    // stop text view native handler
    gtk_signal_emit_stop_by_name(GTK_OBJECT(caller), "drag-data-get");

    (void)caller;
    char *s = f_instptr->get_selection();
    gtk_selection_data_set(selection_data, selection_data->target,
        8, (unsigned char*)s, s ? strlen(s) + 1 : 0);
    delete [] s;
}


// Static private function.
// A release handler for the mouse button, signals the end of dragging.
//
int
files_bag::f_btn_release_hdlr(GtkWidget*, GdkEvent*, void*)
{
    if (f_instptr)
        f_instptr->f_drag_start = false;
    return (false);
}


// Static private function.
// Start the drag, after a selection, if the pointer moves.
//
int
files_bag::f_motion(GtkWidget *widget, GdkEvent *event, void*)
{
    if (f_instptr) {
        if (f_instptr->f_drag_start) {
#if GTK_CHECK_VERSION(2,12,0)
            if (event->motion.is_hint)
                gdk_event_request_motions((GdkEventMotion*)event);
#else
            // Strange voodoo to "turn on" motion events, that are
            // otherwise suppressed since GDK_POINTER_MOTION_HINT_MASK
            // is set.  See GdkEventMask doc.
            gdk_window_get_pointer(widget->window, 0, 0, 0);
#endif
            if ((abs((int)event->motion.x - f_instptr->f_drag_x) > 4 ||
                    abs((int)event->motion.y - f_instptr->f_drag_y) > 4)) {
                f_instptr->f_drag_start = false;
                GtkTargetList *targets = gtk_target_list_new(target_table,
                    n_targets);
                GdkDragContext *context = gtk_drag_begin(widget,
                    targets, (GdkDragAction)(GDK_ACTION_COPY |
                        GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_ASK),
                    f_instptr->f_drag_btn, event);
                gtk_drag_set_icon_default(context);
                return (true);
            }
        }
    }
    return (false);
}


// Private static GTK signal handler.
void
files_bag::f_realize_proc(GtkWidget *widget, void *arg)
{
    // Remove the primary selection clipboard before selection. 
    // In order to return the full path, we have to handle
    // selections ourselves.
    //
    gtk_text_buffer_remove_selection_clipboard(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)),
        gtk_widget_get_clipboard(widget, GDK_SELECTION_PRIMARY));
    text_realize_proc(widget, arg);
}


// Private static GTK signal handler.
void
files_bag::f_unrealize_proc(GtkWidget *widget, void*)
{
    // Stupid alert:  put the clipboard back into the buffer to
    // avoid a whine when remove_selection_clipboard is called in
    // unrealize.

    gtk_text_buffer_add_selection_clipboard(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)),
        gtk_widget_get_clipboard(widget, GDK_SELECTION_PRIMARY));
}


// Static private function.
// Return true if the given path does not match the path stored in
// the files list, and at the same time update the files list.
//
bool
files_bag::f_check_path_and_update(const char *path)
{
    int cnt = 0;
    sDirList *dl;
    for (dl = f_path_list->dirs(); dl; cnt++, dl = dl->next()) ;
    sDirList **array = new sDirList*[cnt];
    cnt = 0;
    for (dl = f_path_list->dirs(); dl; cnt++, dl = dl->next())
        array[cnt] = dl;
    // cnt is number of old path elements

    sDirList *d0 = 0, *de = 0;
    if (pathlist::is_empty_path(path))
        path = ".";
    bool changed = false;
    int newcnt = 0;
    pathgen pg(path);
    char *p;
    while ((p = pg.nextpath(true)) != 0) {
        if (newcnt < cnt && array[newcnt] &&
                !strcmp(p, array[newcnt]->dirname())) {
            if (!d0)
                de = d0 = array[newcnt];
            else {
                de->set_next(array[newcnt]);
                de = de->next();
            }
            array[newcnt] = 0;
            de->set_next(0);
        }
        else {
            changed = true;
            int i;
            for (i = 0; i < cnt; i++) {
                if (array[i] && !strcmp(p, array[i]->dirname())) {
                    if (!d0)
                        de = d0 = array[i];
                    else {
                        de->set_next(array[i]);
                        de = de->next();
                    }
                    array[i] = 0;
                    de->set_next(0);
                    break;
                }
            }
            if (i == cnt) {
                // not already there, create new element
                sDirList *d = new sDirList(p);
                sFileList fl(p);
                fl.read_list(f_path_list->checkfunc(),
                    f_path_list->incldirs());
                int *colwid;
                char *txt = fl.get_formatted_list(f_path_list->columns(),
                    false, f_path_list->no_files_msg(), &colwid);
                d->set_dirfiles(txt, colwid);
                if (!d0)
                    de = d0 = d;
                else {
                    de->set_next(d);
                    de = de->next();
                }
            }
        }
        newcnt++;
        delete [] p;
    }

    for (int i = 0; i < cnt; i++) {
        if (array[i]) {
            changed = true;
            delete array[i];
        }
    }
    delete [] array;
    if (changed)
        f_path_list->set_dirs(d0);
    return (changed);
}


// Static private function.
// Refresh the text while keeping current top location.
//
void
files_bag::f_update_text(GtkWidget *text, const char *newtext)
{
    if (f_instptr == 0 || text == 0 || newtext == 0)
        return;
    double val = text_get_scroll_value(text);
    text_set_chars(text, newtext);
    text_set_scroll_value(text, val);
}


// Private static GTK signal handler.
// In GTK-2, this overrides the built-in selection-get function.
//
void
files_bag::f_selection_get(GtkWidget *widget,
    GtkSelectionData *selection_data, guint, guint, void*)
{
    if (selection_data->selection != GDK_SELECTION_PRIMARY)
        return;  

    // stop native handler
    gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "selection-get");

    char *s = f_instptr->get_selection();
    gtk_selection_data_set(selection_data, selection_data->target,
        8, (unsigned char*)s, s ? strlen(s) + 1 : 0);
    delete [] s;
}


// Private static GTK signal handler.
// Selection clear handler, need to do this ourselves in GTK-2.
//
int
files_bag::f_selection_clear(GtkWidget *widget, GdkEventSelection*, void*)  
{
    text_select_range(widget, 0, 0);
    return (true);
}

