
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

#include "main.h"
#include "cvrt.h"
#include "fio.h"
#include "cd_celldb.h"
#include "gtkmain.h"
#include "gtkfont.h"
#include "gtkinlines.h"


//-----------------------------------------------------------------------------
// Pop-up to manage Override Cell Table
//
// Help system keywords used:
//  xic:overtab

namespace {
    GtkTargetEntry target_table[] = {
        { (char*)"TWOSTRING",   0, 0 },
        { (char*)"CELLNAME",    0, 1 },
        { (char*)"STRING",      0, 2 },
        { (char*)"text/plain",  0, 3 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

    namespace gtkauxtab {
        struct sAT : public gtk_bag
        {
            sAT(GRobject);
            ~sAT();

            void update();

        private:
            void action_hdlr(GtkWidget*, void*);
            int button_hdlr(bool, GtkWidget*, GdkEvent*);
            int motion_hdlr(GtkWidget*, GdkEvent*);
            void select_range(int, int);
            void check_sens();

            static void at_cancel(GtkWidget*, void*);
            static void at_action_proc(GtkWidget*, void*);
            static ESret at_add_cb(const char*, void*);
            static void at_remove_cb(bool, void*);
            static void at_clear_cb(bool, void*);

            static void at_drag_data_get(GtkWidget *widget, GdkDragContext*,
                GtkSelectionData*, guint, guint, void*);
            static void at_drag_data_received(GtkWidget*, GdkDragContext*,
                gint, gint, GtkSelectionData*, guint, guint);
            static int at_btn_hdlr(GtkWidget*, GdkEvent*, void*);
            static int at_btn_release_hdlr(GtkWidget*, GdkEvent*, void*);
            static int at_motion_hdlr(GtkWidget*, GdkEvent*, void*);
            static void at_resize_hdlr(GtkWidget*, GtkAllocation*);
            static void at_realize_hdlr(GtkWidget*, void*);

            GRobject at_caller;
            GtkWidget *at_addbtn;
            GtkWidget *at_rembtn;
            GtkWidget *at_clearbtn;
            GtkWidget *at_over;
            GtkWidget *at_skip;
            GRledPopup *at_add_pop;
            GRaffirmPopup *at_remove_pop;
            GRaffirmPopup *at_clear_pop;

            int at_start;
            int at_end;
            int at_drag_x;
            int at_drag_y;
            bool at_dragging;
        };

        sAT *AT;
    }
}

using namespace gtkauxtab;


void
cConvert::PopUpAuxTab(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete AT;
        return;
    }
    if (mode == MODE_UPD) {
        if (AT)
            AT->update();
        return;
    }
    if (AT)
        return;

    new sAT(caller);
    if (!AT->Shell()) {
        delete AT;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(AT->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), AT->Shell(), mainBag()->Viewport());
    gtk_widget_show(AT->Shell());
}


sAT::sAT(GRobject c)
{
    AT = this;
    at_caller = c;
    at_addbtn = 0;
    at_rembtn = 0;
    at_clearbtn = 0;
    at_add_pop = 0;
    at_remove_pop = 0;
    at_clear_pop = 0;
    at_start = 0;
    at_end = 0;
    at_drag_x = at_drag_y = 0;
    at_dragging = false;

    wb_shell = gtk_NewPopup(0, "Cell Table Listing", at_cancel, 0);
    if (!wb_shell)
        return;

    GtkWidget *form = gtk_table_new(4, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    //
    // Button row
    //
    GtkWidget *button = gtk_toggle_button_new_with_label("Add");
    gtk_widget_set_name(button, "Add");
    gtk_widget_show(button);
    at_addbtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(at_action_proc), 0);

    int rowcnt = 0;
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_toggle_button_new_with_label("Remove");
    gtk_widget_set_name(button, "Remove");
    gtk_widget_show(button);
    at_rembtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(at_action_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_toggle_button_new_with_label("Clear");
    gtk_widget_set_name(button, "Clear");
    gtk_widget_show(button);
    at_clearbtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(at_action_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 2, 3, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_toggle_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(at_action_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 3, 4, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_radio_button_new_with_label(0, "Override");
    gtk_widget_set_name(button, "Override");
    gtk_widget_show(button);
    GSList *group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(at_action_proc), 0);
    at_over = button;

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_radio_button_new_with_label(group, "Skip");
    gtk_widget_set_name(button, "Skip");
    gtk_widget_show(button);
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(at_action_proc), 0);
    at_skip = button;

    gtk_table_attach(GTK_TABLE(form), button, 2, 4, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Title label
    //
    GtkWidget *label = gtk_label_new("Cells in Override Table");
    gtk_widget_show(label);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 4, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Scrolled text area
    //
    GtkWidget *contr;
    text_scrollable_new(&contr, &wb_textarea, FNT_FIXED);

    gtk_widget_add_events(wb_textarea, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "button-press-event",
        GTK_SIGNAL_FUNC(at_btn_hdlr), 0);
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "size-allocate",
        GTK_SIGNAL_FUNC(at_resize_hdlr), 0);
    // init for drag/drop
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "button-release-event",
        GTK_SIGNAL_FUNC(at_btn_release_hdlr), 0);
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "motion-notify-event",
        GTK_SIGNAL_FUNC(at_motion_hdlr), 0);
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "drag-data-get",
        GTK_SIGNAL_FUNC(at_drag_data_get), 0);
    gtk_signal_connect_after(GTK_OBJECT(wb_textarea), "realize",
        GTK_SIGNAL_FUNC(at_realize_hdlr), 0);

    // drop site
    gtk_drag_dest_set(wb_textarea, GTK_DEST_DEFAULT_ALL, target_table,
        n_targets, GDK_ACTION_COPY);
    gtk_signal_connect_after(GTK_OBJECT(wb_textarea), "drag-data-received",
        GTK_SIGNAL_FUNC(at_drag_data_received), 0);

    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
    const char *bclr = GRpkgIf()->GetAttrColor(GRattrColorLocSel);
    gtk_text_buffer_create_tag(textbuf, "primary", "background", bclr, NULL);

    gtk_table_attach(GTK_TABLE(form), contr, 0, 4, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);
    rowcnt++;

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 4, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Dismiss button
    //
    button = gtk_toggle_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(at_cancel), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 4, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    // Constrain overall widget width so title text isn't truncated.
    gtk_widget_set_usize(wb_shell, 240, -1);

    check_sens();
}


sAT::~sAT()
{
    AT = 0;
    if (at_caller)
        GRX->Deselect(at_caller);
    if (at_add_pop)
        at_add_pop->popdown();
    if (at_remove_pop)
        at_remove_pop->popdown();
    if (at_clear_pop)
        at_clear_pop->popdown();
    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(at_cancel), wb_shell);
}


void
sAT::update()
{
    if (!wb_textarea || !wb_textarea->window)
        return;
    select_range(0, 0);
    CDcellTab *ct = CDcdb()->auxCellTab();
    if (ct) {
        int width, height;
        gdk_window_get_size(wb_textarea->window, &width, &height);
        int cols = (width-4)/GTKfont::stringWidth(wb_textarea, 0);
        stringlist *s0 = ct->list();
        char *newtext = stringlist::col_format(s0, cols);
        stringlist::destroy(s0);
        double val = text_get_scroll_value(wb_textarea);
        text_set_chars(wb_textarea, newtext);
        text_set_scroll_value(wb_textarea, val);
    }
    else
        text_set_chars(wb_textarea, "");

    if (CDvdb()->getVariable(VA_SkipOverrideCells)) {
        GRX->SetStatus(at_over, false);
        GRX->SetStatus(at_skip, true);
    }
    else {
        GRX->SetStatus(at_over, true);
        GRX->SetStatus(at_skip, false);
    }
    check_sens();
}


void
sAT::action_hdlr(GtkWidget *caller, void*)
{
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help")) {
        PopUpHelp("xic:overtab");
        return;
    }

    bool state = GRX->GetStatus(caller);

    if (!strcmp(name, "Add")) {
        if (at_add_pop)
            at_add_pop->popdown();
        if (at_remove_pop)
            at_remove_pop->popdown();
        if (at_clear_pop)
            at_clear_pop->popdown();

        if (state) {
            at_add_pop = PopUpEditString((GRobject)at_addbtn,
                GRloc(), "Enter cellname: ", 0,
                at_add_cb, 0, 250, 0, false, 0);
            if (at_add_pop)
                at_add_pop->register_usrptr((void**)&at_add_pop);
        }
        return;
    }
    if (!strcmp(name, "Remove")) {
        if (at_add_pop)
            at_add_pop->popdown();
        if (at_remove_pop)
            at_remove_pop->popdown();
        if (at_clear_pop)
            at_clear_pop->popdown();

        if (state) {
            char *string = at_start != at_end ?
                text_get_chars(wb_textarea, at_start, at_end) : 0;
            if (string) {
                at_remove_pop = PopUpAffirm(at_rembtn, GRloc(),
                    "Confirm - remove selected cell from table?", at_remove_cb,
                    lstring::copy(string));
                if (at_remove_pop)
                    at_remove_pop->register_usrptr((void**)&at_remove_pop);
                delete [] string;
            }
            else
                GRX->Deselect(at_rembtn);
        }
        return;
    }
    if (!strcmp(name, "Clear")) {
        if (at_add_pop)
            at_add_pop->popdown();
        if (at_remove_pop)
            at_remove_pop->popdown();
        if (at_clear_pop)
            at_clear_pop->popdown();

        if (state) {
            at_clear_pop = PopUpAffirm(at_clearbtn, GRloc(),
                "Confirm - remove all names from table?", at_clear_cb, 0);
            if (at_clear_pop)
                at_clear_pop->register_usrptr((void**)&at_clear_pop);
        }
        return;
    }
    if (!strcmp(name, "Override")) {
        if (state)
            CDvdb()->clearVariable(VA_SkipOverrideCells);
        return;
    }
    if (!strcmp(name, "Skip")) {
        if (state)
            CDvdb()->setVariable(VA_SkipOverrideCells, "");
        return;
    }
}


int
sAT::button_hdlr(bool up, GtkWidget *caller, GdkEvent *event)
{
    if (up) {
        at_dragging = false;
        return (false);
    }

    if (event->type != GDK_BUTTON_PRESS)
        return (true);

    select_range(0, 0);
    char *string = text_get_chars(caller, 0, -1);
    int x = (int)event->button.x;
    int y = (int)event->button.y;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(caller), &ihere, x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    x = gtk_text_iter_get_offset(&ihere) - gtk_text_iter_get_offset(&iline);
    char *line_start = string + gtk_text_iter_get_offset(&iline);

    int start = 0;
    for ( ; start < x; start++) {
        if (line_start[start] == '\n' || line_start[start] == 0) {
            // pointing to right of line end
            select_range(0, 0);
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
    while ((*t == '+' || *t == '*') && !CDcdb()->findSymbol(t)) {
        start++;
        t++;
    }

    start += (line_start - string);
    end += (line_start - string);
    delete [] string;

    if (start == end)
        return (true);
    select_range(start, end);

    // init for drag/drop
    at_dragging = true;
    at_drag_x = (int)event->button.x;
    at_drag_y = (int)event->button.y;

    return (true);
}


int
sAT::motion_hdlr(GtkWidget *widget, GdkEvent *event)
{
    if (at_dragging) {
#if GTK_CHECK_VERSION(2,12,0)
        if (event->motion.is_hint)
            gdk_event_request_motions((GdkEventMotion*)event);
#else
        // Strange voodoo to "turn on" motion events, that are
        // otherwise suppressed since GDK_POINTER_MOTION_HINT_MASK
        // is set.  See GdkEventMask doc.
        gdk_window_get_pointer(widget->window, 0, 0, 0);
#endif
        if ((abs((int)event->motion.x - at_drag_x) > 4 ||
                abs((int)event->motion.y - at_drag_y) > 4)) {
            at_dragging = false;
            GtkTargetList *targets = gtk_target_list_new(target_table,
                n_targets);
            GdkDragContext *drcx = gtk_drag_begin(widget, targets,
                (GdkDragAction)GDK_ACTION_COPY, 1, event);
            gtk_drag_set_icon_default(drcx);
            return (true);
        }
    }
    return (false);
}


// Select the chars in the range, start=end deselects existing.  In
// GTK-1, selecting gives blue inverse, which turns gray if
// unselected, retaining an indication for the buttons.  GTK-2
// doesn't do this automatically so we provide something similar here.
//
void
sAT::select_range(int start, int end)
{
    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
    GtkTextIter istart, iend;
    if (at_end != at_start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, at_start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, at_end);
        gtk_text_buffer_remove_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    text_select_range(wb_textarea, start, end);
    if (end != start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, end);
        gtk_text_buffer_apply_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    at_start = start;
    at_end = end;
    check_sens();
}


void
sAT::check_sens()
{
    if (text_has_selection(wb_textarea))
        gtk_widget_set_sensitive(at_rembtn, true);
    else
        gtk_widget_set_sensitive(at_rembtn, false);
}


// Static function.
// Pop down the cells popup
//
void
sAT::at_cancel(GtkWidget*, void*)
{
    Cvt()->PopUpAuxTab(0, MODE_OFF);
}


// Static function.
//
void
sAT::at_action_proc(GtkWidget *caller, void *client_data)
{
    if (AT)
        AT->action_hdlr(caller, client_data);
}


// Static function.
// Callback for the Add dialog.
//
ESret
sAT::at_add_cb(const char *name, void*)
{
    if (AT && name) {
        if (CDcdb()->auxCellTab()) {
            if (!CDcdb()->auxCellTab()->add(name, false)) {
                AT->at_add_pop->update("Name not found, try again: ", 0);
                return (ESTR_IGN);
            }
            AT->update();
        }
    }
    return (ESTR_DN);
}


// Static function.
// Callback for the Remove dialog.
//
void
sAT::at_remove_cb(bool state, void *arg)
{
    char *name = (char*)arg;
    if (state && name && CDcdb()->auxCellTab()) {
        CDcdb()->auxCellTab()->remove(CD()->CellNameTableAdd(name));
        if (AT)
            AT->update();
    }
    delete [] name;
}


// Static function.
// Callback for the Clear dialog.
//
void
sAT::at_clear_cb(bool state, void*)
{
    if (state && CDcdb()->auxCellTab()) {
        CDcdb()->auxCellTab()->clear();
        if (AT)
            AT->update();
    }
}


// Static function.
// Data-get function, for drag/drop.
//
void
sAT::at_drag_data_get(GtkWidget *widget, GdkDragContext*,
    GtkSelectionData *selection_data, guint, guint, void*)
{
    if (GTK_IS_TEXT_VIEW(widget))
    // stop text view native handler
    gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "drag-data-get");

    char *string = text_get_selection(widget);
    if (string) {
        gtk_selection_data_set(selection_data, selection_data->target,
            8, (unsigned char*)string, strlen(string)+1);
        delete [] string;
    }
}


// Static function.
// Drag data received in editing window, grab it.
//
void
sAT::at_drag_data_received(GtkWidget*, GdkDragContext *context,
    gint, gint, GtkSelectionData *data, guint, guint time)
{
    if (data->length >= 0 && data->format == 8 && data->data) {
        char *src = (char*)data->data;
        if (data->target == gdk_atom_intern("TWOSTRING", true)) {
            // Drops from content lists may be in the form
            // "fname_or_chd\ncellname".  Keep the cellname.
            char *t = strchr(src, '\n');
            if (t)
                src = t+1;
        }
        if (CDcdb()->auxCellTab()) {
            if (CDcdb()->auxCellTab()->add(src, false))
                AT->update();
        }
        gtk_drag_finish(context, true, false, time);
        return;
    }
    gtk_drag_finish(context, false, false, time);
}


// Static function.
// Handle button presses in the text area.
//
int
sAT::at_btn_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (AT)
        return (AT->button_hdlr(false, caller, event));
    return (true);
}


// Static function.
int
sAT::at_btn_release_hdlr(GtkWidget*, GdkEvent*, void*)
{
    if (AT)
        AT->button_hdlr(true, 0, 0);
    return (false);
}


// Static function.
// Motion handler, begin drag.
//
int
sAT::at_motion_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (AT)
        return (AT->motion_hdlr(caller, event));
    return (false);
}


// Static function.
// Reformat listing after resize.
//
void
sAT::at_resize_hdlr(GtkWidget*, GtkAllocation*)
{
    if (AT)
        AT->update();
}


// Static function.
// Do initial text rendering and set up arrow cursor.
//
void
sAT::at_realize_hdlr(GtkWidget *w, void *arg)
{
    text_realize_proc(w, arg);
    if (AT)
        AT->update();
}

