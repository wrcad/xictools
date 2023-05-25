
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
#include <stdint.h>
#include <string.h>

#include "gtkinterf.h"
#include "gtkmcol.h"
#include "gtkfont.h"
#include "gtkutil.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"


// Popup to display a list.  title is the title label text, callback
// is called with the word pointed to when the user points in the
// window, and 0 when the popup is destroyed.
//
// The list is column-formatted, and a text widget is used for the
// display.  If buttons is given, it is a 0-terminated list of
// auxiliary toggle button button names.
//
GRmcolPopup *
GTKbag::PopUpMultiCol(stringlist *symlist, const char *title,
    void (*callback)(const char*, void*), void *arg,
    const char **buttons, int pgsize, bool no_dd)
{
    static int mcol_count;

    GTKmcolPopup *mcol = new GTKmcolPopup(this, symlist, title,
        buttons, pgsize, arg);
    mcol->register_callback(callback);
    mcol->set_no_dragdrop(no_dd);

    gtk_window_set_transient_for(GTK_WINDOW(mcol->Shell()),
        GTK_WINDOW(wb_shell));
    int x, y;
    GTKdev::self()->ComputePopupLocation(GRloc(), mcol->Shell(), wb_shell,
        &x, &y);
    x += mcol_count*50 - 150;
    y += mcol_count*50 - 150;
    mcol_count++;
    if (mcol_count == 6)
        mcol_count = 0;
    gtk_window_move(GTK_WINDOW(mcol->Shell()), x, y);

    mcol->set_visible(true);
    return (mcol);
}


namespace {
    // DND support.  The string is actually two strings separated by
    // a newline.  The TWOSTRING atom is local, so we can handle this
    // properly.
    GtkTargetEntry target_table[] = {
      { (char*)"TWOSTRING",  0, 0 },
      { (char*)"STRING",     0, 1 },
      { (char*)"text/plain", 0, 2 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);
}


GTKmcolPopup::GTKmcolPopup(GTKbag *owner, stringlist *symlist,
    const char *title, const char **buttons, int pgsize, void *arg)
{
    p_parent = owner;
    p_cb_arg = arg;
    mc_pagesel = 0;
    for (int i = 0; i < MC_MAXBTNS; i++)
        mc_buttons[i] = 0;
    mc_save_pop = 0;
    mc_msg_pop = 0;
    mc_strings = stringlist::dup(symlist);
    stringlist::sort(mc_strings);

    mc_alloc_width = 0;
    mc_drag_x = mc_drag_y = 0;
    mc_page = 0;

    mc_pagesize = pgsize;
    if (mc_pagesize <= 0)
        mc_pagesize = DEF_LIST_MAX_PER_PAGE;
    else if (mc_pagesize < 100)
        mc_pagesize = 100;
    else if (mc_pagesize > 50000)
        mc_pagesize = 50000;

    mc_btnmask = 0;
    mc_start = 0;
    mc_end = 0;
    mc_dragging = false;

    if (owner)
        owner->MonitorAdd(this);

    wb_shell = gtk_NewPopup(owner, "Listing", mc_quit_proc, this);
    gtk_window_set_default_size(GTK_WINDOW(wb_shell), 350, 250);

    GtkWidget *form = gtk_table_new(1, 3, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    // Title label.
    //
    GtkWidget *label = gtk_label_new(title);
    gtk_widget_show(label);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // Scrolled text area.
    //
    GtkWidget *contr;
    text_scrollable_new(&contr, &wb_textarea, FNT_FIXED);

    gtk_widget_add_events(wb_textarea, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(wb_textarea), "button-press-event",
        G_CALLBACK(mc_btn_proc), this);
    g_signal_connect(G_OBJECT(wb_textarea), "button-release-event",
        G_CALLBACK(mc_btn_release_proc), this);
    g_signal_connect(G_OBJECT(wb_textarea), "motion-notify-event",
        G_CALLBACK(mc_motion_proc), this);
    g_signal_connect(G_OBJECT(wb_textarea), "drag-data-get",
        G_CALLBACK(mc_source_drag_data_get), this);
    g_signal_connect(G_OBJECT(wb_textarea), "map-event",
        G_CALLBACK(mc_map_hdlr), this);
    g_signal_connect_after(G_OBJECT(wb_textarea), "realize",
        G_CALLBACK(text_realize_proc), this);

    // this callback formats the text
    g_signal_connect_after(G_OBJECT(wb_textarea), "size-allocate",
        G_CALLBACK(mc_resize_proc), this);

    // drop handler needs this
    g_object_set_data(G_OBJECT(wb_textarea), "label", label);

    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
    const char *bclr = GRpkg::self()->GetAttrColor(GRattrColorLocSel);
    gtk_text_buffer_create_tag(textbuf, "primary", "background", bclr, NULL);
    gtk_table_attach(GTK_TABLE(form), contr, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    GtkWidget*button = gtk_toggle_button_new_with_label("Save Text ");
    gtk_widget_set_name(button, "Save");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(mc_save_btn_hdlr), this);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    mc_pagesel = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(hbox), mc_pagesel, false, false, 0);

    // Dismiss button.
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(mc_quit_proc), this);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    if (buttons) {
        for (int i = 0; i < MC_MAXBTNS && buttons[i]; i++) {
            button = gtk_button_new_with_label(buttons[i]);
            gtk_widget_set_name(button, buttons[i]);
            gtk_widget_set_sensitive(button, false);
            gtk_widget_show(button);
            g_signal_connect(G_OBJECT(button), "clicked",
                G_CALLBACK(mc_action_proc), this);
            gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
            mc_buttons[i] = button;
        }
    }

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    relist();
}


GTKmcolPopup::~GTKmcolPopup()
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }
    if (p_callback)
        (*p_callback)(0, p_cb_arg);
    if (mc_save_pop)
        mc_save_pop->popdown();
    if (mc_msg_pop)
        mc_msg_pop->popdown();
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        GTKdev::Deselect(p_caller);
    stringlist::destroy(mc_strings);

    g_signal_handlers_disconnect_by_func(G_OBJECT(wb_shell),
        (gpointer)mc_quit_proc, this);
}


// GRpopup override
//
void
GTKmcolPopup::popdown()
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// GRmcolPopup override
//
void
GTKmcolPopup::update(stringlist *symlist, const char *title)
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }

    stringlist::destroy(mc_strings);
    mc_strings = stringlist::dup(symlist);
    stringlist::sort(mc_strings);

    mc_page = 0;
    relist();

    if (title) {
        GtkWidget *label = (GtkWidget*)
            g_object_get_data(G_OBJECT(wb_textarea), "label");
        if (label)
            gtk_label_set_text(GTK_LABEL(label), title);
    }
}


// GRmcolPopup override
//
// Return the selected text, null if no selection.
//
char *
GTKmcolPopup::get_selection()
{
    if (mc_end != mc_start)
        return (text_get_chars(wb_textarea, mc_start, mc_end));
    return (0);
}


// GRmcolPopup override
//
// Set sensitivity of optional buttons.
// Bit == 0:  button always insensitive
// Bit == 1:  button sensitive when selection.
//
void
GTKmcolPopup::set_button_sens(int mask)
{
    int bm = 1;
    mc_btnmask = ~mask;
    bool has_sel = (mc_end != mc_start);
    for (int i = 0; i < MC_MAXBTNS && mc_buttons[i]; i++) {
        gtk_widget_set_sensitive(mc_buttons[i], (bm & mask) && has_sel);
        bm <<= 1;
    }
}


// Handle the relisting, the display is paged.
//
void
GTKmcolPopup::relist()
{
    int min = mc_page * mc_pagesize;
    int max = min + mc_pagesize;

    stringlist *s0 = 0, *se = 0;
    int cnt = 0;
    for (stringlist *s = mc_strings; s; s = s->next) {
        if (cnt >= min && cnt < max) {
            if (!s0)
                se = s0 = new stringlist(lstring::copy(s->string), 0);
            else {
                se->next = new stringlist(lstring::copy(s->string), 0);
                se = se->next;
            }
        }
        cnt++;
    }

    if (cnt <= mc_pagesize)
        gtk_widget_hide(mc_pagesel);
    else {
        char buf[128];
        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(
            GTK_COMBO_BOX(mc_pagesel))));

        for (int i = 0; i*mc_pagesize < cnt; i++) {
            int tmpmax = (i+1)*mc_pagesize;
            if (tmpmax > cnt)
                tmpmax = cnt;
            snprintf(buf, sizeof(buf), "%d - %d", i*mc_pagesize + 1, tmpmax);
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mc_pagesel),
                buf);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(mc_pagesel), mc_page);
        g_signal_connect(G_OBJECT(mc_pagesel), "activate",
            G_CALLBACK(mc_menu_proc), this);
        gtk_widget_show(mc_pagesel);
    }

    GtkAllocation a;
    gtk_widget_get_allocation(wb_textarea, &a);
    int cols = (a.width - 4)/GTKfont::stringWidth(wb_textarea, 0);
    char *s = stringlist::col_format(s0, cols);
    stringlist::destroy(s0);
    text_set_chars(wb_textarea, s);
    delete [] s;
}


// Reformat the listing when window size changes.
//
void
GTKmcolPopup::resize_handler(int width)
{
    if (wb_textarea && gtk_widget_get_window(wb_textarea) && mc_strings) {
        if (width > 0 && width != mc_alloc_width) {
            mc_alloc_width = width;
            relist();
        }
    }
}


// Handle button presses in the text area.
//
int
GTKmcolPopup::button_handler(int x0, int y0)
{
    if (!wb_textarea)
        return (false);
    int x = x0;
    int y = y0;

    select_range(0, 0);
    char *string = text_get_chars(wb_textarea, 0, -1);
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(wb_textarea),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(wb_textarea), &ihere,
        x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(wb_textarea), &iline, y, 0);
    x = gtk_text_iter_get_offset(&ihere) - gtk_text_iter_get_offset(&iline);
    char *line_start = string + gtk_text_iter_get_offset(&iline);

    int start = 0;
    for ( ; start < x; start++) {
        if (line_start[start] == '\n' || line_start[start] == 0) {
            // pointing to right of line end
            delete [] string;
            return (true);
        }
    }
    if (isspace(line_start[start])) {
        // pointing at white space
        delete [] string;
        return (true);
    }
    int end = start;
    while (start > 0 && !isspace(line_start[start]))
        start--;
    if (isspace(line_start[start]))
        start++;
    while (line_start[end] && !isspace(line_start[end]))
        end++;

    char buf[256];
    char *t = buf;
    for (int i = start; i < end; i++)
        *t++ = line_start[i];
    *t = 0;

    t = buf;

    // The top level cells are listed with an '*'.
    // Modified cells are listed  with a '+'.
    while (*t == '+' || *t == '*') {
        start++;
        t++;
    }

    start += (line_start - string);
    end += (line_start - string);
    delete [] string;

    if (start == end)
        return (true);
    select_range(start, end);

    if (p_callback)
        (*p_callback)(t, p_cb_arg);

    mc_drag_x = x0;
    mc_drag_y = y0;
    mc_dragging = true;

    return (true);
}


// Select the chars in the range, start=end deselects existing.  In
// GTK-1, selecting gives blue inverse, which turns gray if
// unselected, retaining an indication for the buttons.  GTK-2
// doesn't do this automatically so we provide something similar here.
//
void
GTKmcolPopup::select_range(int start, int end)
{
    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
    GtkTextIter istart, iend;
    if (mc_end != mc_start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, mc_start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, mc_end);
        gtk_text_buffer_remove_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    text_select_range(wb_textarea, start, end);
    if (end != start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, end);
        gtk_text_buffer_apply_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    mc_start = start;
    mc_end = end;
    if (start == end) {
        for (int i = 0; i < MC_MAXBTNS && mc_buttons[i]; i++)
            gtk_widget_set_sensitive(mc_buttons[i], false);
    }
    else {
        int bm = 1;
        for (int i = 0; i < MC_MAXBTNS && mc_buttons[i]; i++) {
            gtk_widget_set_sensitive(mc_buttons[i], bm & ~mc_btnmask);
            bm <<= 1;
        }
    }
}


// Private static GTK signal handler.
//
void
GTKmcolPopup::mc_menu_proc(GtkWidget *widget, void *client_data)
{
    GTKmcolPopup *mcol = static_cast<GTKmcolPopup*>(client_data);
    if (mcol) {
        int i = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
        if (mcol->mc_page != i) {
            mcol->mc_page = i;
            mcol->relist();
        }
    }
}


// Private static GTK signal handler.
// Handle the auxilliary buttons: call the callback with '/' followed by
// button text.
//
void
GTKmcolPopup::mc_action_proc(GtkWidget *widget, void *client_data)
{
    GTKmcolPopup *mcol = static_cast<GTKmcolPopup*>(client_data);
    const char *wname = gtk_widget_get_name(widget);
    if (mcol && wname && mcol->p_callback) {
        char *tmp = new char[strlen(wname) + 2];
        tmp[0] = '/';
        strcpy(tmp+1, wname);
        (*mcol->p_callback)(tmp, mcol->p_cb_arg);
        delete [] tmp;
    }
}


// Private static GTK signal handler.
//
void
GTKmcolPopup::mc_quit_proc(GtkWidget*, void *client_data)
{
    GTKmcolPopup *mcol = static_cast<GTKmcolPopup*>(client_data);
    if (mcol)
        mcol->popdown();
}


// Private static GTK signal handler.
// Required by GTK2, load the text now that we know the size of the
// window (GRmulticol only).
//
int
GTKmcolPopup::mc_map_hdlr(GtkWidget*, GdkEvent*, void *client_data)
{
    GTKmcolPopup *mcol = static_cast<GTKmcolPopup*>(client_data);
    if (mcol && mcol->wb_textarea) {
        GtkAllocation a;
        gtk_widget_get_allocation(mcol->wb_textarea, &a);
        mcol->resize_handler(a.width);
    }
    return (false);
}


// Private static GTK signal handler.
// Window resize callback.
//
void
GTKmcolPopup::mc_resize_proc(GtkWidget*, GtkAllocation *alloc,
    void *client_data)
{
    GTKmcolPopup *mcol = static_cast<GTKmcolPopup*>(client_data);
    if (mcol)
        mcol->resize_handler(alloc->width);
}


// Private static GTK signal handler.
// Button press callback.
//
int
GTKmcolPopup::mc_btn_proc(GtkWidget*, GdkEvent *event, void *client_data)
{
    if (event->type != GDK_BUTTON_PRESS)
        return (true);
    if (event->button.button != 1)
        return (true);
    GTKmcolPopup *mcol = static_cast<GTKmcolPopup*>(client_data);
    if (mcol) {
        int x = (int)event->button.x;
        int y = (int)event->button.y;
        return (mcol->button_handler(x, y));
    }
    return (false);
}


// Private static GTK signal handler.
//
int
GTKmcolPopup::mc_btn_release_proc(GtkWidget*, GdkEvent *event,
    void *client_data)
{
    if (event->button.button != 1)
        return (true);
    GTKmcolPopup *mcol = static_cast<GTKmcolPopup*>(client_data);
    if (mcol)
        mcol->mc_dragging = false;
    return (true);
}


// Private static GTK signal handler.
// Start the drag, after a selection, if the pointer moves.
//
int
GTKmcolPopup::mc_motion_proc(GtkWidget *widget, GdkEvent *event, void *arg)
{
    GTKmcolPopup *mcol = static_cast<GTKmcolPopup*>(arg);
    if (mcol && !mcol->p_no_dd) {
        if (mcol->mc_dragging) {
            if (event->motion.is_hint)
                gdk_event_request_motions((GdkEventMotion*)event);
            (void)widget;
            if ((abs((int)event->motion.x - mcol->mc_drag_x) > 4 ||
                    abs((int)event->motion.y - mcol->mc_drag_y) > 4)) {
                mcol->mc_dragging = false;
                GtkTargetList *targets =
                    gtk_target_list_new(target_table, n_targets);
                GdkDragContext *context = gtk_drag_begin(mcol->wb_textarea,
                    targets, (GdkDragAction)GDK_ACTION_COPY, 1, event);
                gtk_drag_set_icon_default(context);
                return (true);
            }
        }
    }
    return (false);
}


// Private static GTK signal handler.
// Set the drag data to the selected file path
//
void
GTKmcolPopup::mc_source_drag_data_get(GtkWidget *widget, GdkDragContext*,
    GtkSelectionData *selection_data, guint, guint, gpointer)
{
    if (GTK_IS_TEXT_VIEW(widget)) {
        // stop text view native handler
        g_signal_stop_emission_by_name(G_OBJECT(widget), "drag-data-get");
    }

    GtkWidget *label = (GtkWidget*)g_object_get_data(G_OBJECT(widget),
        "label");
    char *s = text_get_selection(widget);
    if (!s)
        return;

    if (gtk_selection_data_get_target(selection_data) ==
            gdk_atom_intern("TWOSTRING", true)) {
        // We use a newline to separate the file and cell names, the
        // drop receiver must handle this.

        const char *ltext = 0;
        if (label)
            ltext = gtk_label_get_text(GTK_LABEL(label));
        if (ltext) {
            const char *t = strchr(ltext, '\n');
            if (t)
                ltext = t+1;
        }

        char *t = new char[strlen(s) + strlen(ltext) + 2];
        strcpy(t, ltext);
        strcat(t, "\n");
        strcat(t, s);
        delete [] s;
        gtk_selection_data_set(selection_data,
            gtk_selection_data_get_target(selection_data),
            8, (unsigned char*)t, strlen(t) + 1);
        delete [] t;
    }
    else {
        // Plain text format, just pass the listing text.
        gtk_selection_data_set(selection_data,
            gtk_selection_data_get_target(selection_data),
            8, (unsigned char*)s, strlen(s) + 1);
        delete [] s;
    }
}


// Private static GTK signal handler.
// Handle Save Text button presses.
//
void
GTKmcolPopup::mc_save_btn_hdlr(GtkWidget *widget, void *arg)
{
    if (GTKdev::GetStatus(widget)) {
        GTKmcolPopup *mcp = (GTKmcolPopup*)arg;
        if (mcp->mc_save_pop)
            return;
        mcp->mc_save_pop = new GTKledPopup(0,
            "Enter path to file for saved text:", "", 200, false, 0, arg);
        mcp->mc_save_pop->register_caller(widget, false, true);
        mcp->mc_save_pop->register_callback(
            (GRledPopup::GRledCallback)&mcp->mc_save_cb);
        mcp->mc_save_pop->register_usrptr((void**)&mcp->mc_save_pop);

        gtk_window_set_transient_for(GTK_WINDOW(mcp->mc_save_pop->pw_shell),
            GTK_WINDOW(mcp->wb_shell));
        GTKdev::self()->SetPopupLocation(GRloc(), mcp->mc_save_pop->pw_shell,
            mcp->wb_shell);
        mcp->mc_save_pop->set_visible(true);
    }
}


// Private static handler.
// Callback for the save file name pop-up.
//
ESret
GTKmcolPopup::mc_save_cb(const char *string, void *arg)
{
    GTKmcolPopup *mcp = (GTKmcolPopup*)arg;
    if (string) {
        if (!filestat::create_bak(string)) {
            mcp->mc_save_pop->update(
                "Error backing up existing file, try again", 0);
            return (ESTR_IGN);
        }
        FILE *fp = fopen(string, "w");
        if (!fp) {
            mcp->mc_save_pop->update("Error opening file, try again", 0);
            return (ESTR_IGN);
        }
        char *txt = text_get_chars(mcp->wb_textarea, 0, -1);
        if (txt) {
            unsigned int len = strlen(txt);
            if (len) {
                if (fwrite(txt, 1, len, fp) != len) {
                    mcp->mc_save_pop->update("Write failed, try again", 0);
                    delete [] txt;
                    fclose(fp);
                    return (ESTR_IGN);
                }
            }
            delete [] txt;
        }
        fclose(fp);

        if (mcp->mc_msg_pop)
            mcp->mc_msg_pop->popdown();
        mcp->mc_msg_pop = new GTKmsgPopup(0, "Text saved in file.", false);
        mcp->mc_msg_pop->register_usrptr((void**)&mcp->mc_msg_pop);
        gtk_window_set_transient_for(GTK_WINDOW(mcp->mc_msg_pop->pw_shell),
            GTK_WINDOW(mcp->wb_shell));
        GTKdev::self()->SetPopupLocation(GRloc(), mcp->mc_msg_pop->pw_shell,
            mcp->wb_shell);
        mcp->mc_msg_pop->set_visible(true);
        g_timeout_add(2000, mc_timeout, mcp);
    }
    return (ESTR_DN);
}


int
GTKmcolPopup::mc_timeout(void *arg)
{
    GTKmcolPopup *mcp = (GTKmcolPopup*)arg;
    if (mcp->mc_msg_pop)
        mcp->mc_msg_pop->popdown();
    return (0);
}

