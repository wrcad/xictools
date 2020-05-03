
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
#include "gtkmain.h"
#include "gtkinlines.h"
#include <gdk/gdkkeysyms.h>


//-----------------------------------------------------------------------------
//  Pop-up for the Expand command
//
// Help system keywords used:
//  xic:expnd

namespace gtkexpand {
    struct sExp : public GRpopup
    {
        typedef bool(*ExpandCallback)(const char*, void*);

        sExp(gtk_bag*, const char*, bool, void*);
        ~sExp();

        GtkWidget *shell() { return (exp_popup); }

        // GRpopup overrides
        void set_visible(bool visib)
            {
                if (visib) {
                    gtk_widget_show(exp_popup);
                    gtk_window_set_focus(GTK_WINDOW(exp_popup), exp_text);
                }
                else
                    gtk_widget_hide(exp_popup);
            }

        void register_callback(ExpandCallback cb) { exp_callback = cb; }

        void popdown();
        void initialize();
        void update(const char*);

    private:
        void action_proc(GtkWidget*);

        // GTK signal handlers
        static void exp_cancel_proc(GtkWidget*, void*);
        static void exp_action(GtkWidget*, void*);
        static void exp_change_proc(GtkWidget*, void*);
        static int exp_key_hdlr(GtkWidget*, GdkEvent*, void*);

        GtkWidget *exp_popup;
        GtkWidget *exp_text;
        bool (*exp_callback)(const char*, void*);
        void *exp_cb_arg;
    };
}

using namespace gtkexpand;


void
win_bag::PopUpExpand(GRobject caller, ShowMode mode,
    bool (*callback)(const char*, void*), void *arg,
    const char *string, bool nopeek)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        if (wib_expandpop)
            wib_expandpop->popdown();
        return;
    }
    if (mode == MODE_UPD) {
        if (wib_expandpop)
            wib_expandpop->update(string);
        return;
    }
    if (wib_expandpop)
        return;

    wib_expandpop = new sExp(this, string, nopeek, arg);
    wib_expandpop->register_usrptr((void**)&wib_expandpop);
    if (!wib_expandpop->shell()) {
        delete wib_expandpop;
        wib_expandpop = 0;
        return;
    }
    wib_expandpop->register_caller(caller);
    wib_expandpop->register_callback(callback);
    wib_expandpop->initialize();
    wib_expandpop->set_visible(true);
}


sExp::sExp(gtk_bag *owner, const char *string, bool nopeek, void *arg)
{
    p_parent = owner;
    exp_popup = 0;
    exp_text = 0;
    exp_callback = 0;
    exp_cb_arg = arg;

    if (owner)
        owner->MonitorAdd(this);

    exp_popup = gtk_NewPopup(owner, "Expand", exp_cancel_proc, this);
    if (!exp_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(exp_popup), false);
    BlackHoleFix(exp_popup);

    gtk_signal_connect(GTK_OBJECT(exp_popup), "key-press-event",
        GTK_SIGNAL_FUNC(exp_key_hdlr), this);

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(exp_popup), form);
    int rowcnt = 0;

    // Label in frame plus help button
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("Set expansion control string");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(row), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(exp_action), this);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Buttons
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    exp_text = gtk_entry_new();
    gtk_widget_show(exp_text);
    if (!string)
        string = "";
    gtk_entry_set_text(GTK_ENTRY(exp_text), string);
    gtk_entry_set_editable(GTK_ENTRY(exp_text), true);
    gtk_box_pack_start(GTK_BOX(row), exp_text, false, false, 0);
    gtk_widget_set_size_request(exp_text, 60, -1);
    gtk_entry_set_position(GTK_ENTRY(exp_text), 0);
    gtk_signal_connect_after(GTK_OBJECT(exp_text), "changed",
        GTK_SIGNAL_FUNC(exp_change_proc), this);

    button = gtk_button_new_with_label("+");
    gtk_widget_set_name(button, "plus");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(exp_action), this);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);

    button = gtk_button_new_with_label("-");
    gtk_widget_set_name(button, "minus");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(exp_action), this);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);

    button = gtk_button_new_with_label("All");
    gtk_widget_set_name(button, "all");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(exp_action), this);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);

    button = gtk_button_new_with_label("0");
    gtk_widget_set_name(button, "0");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(exp_action), this);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);

    button = gtk_button_new_with_label("1");
    gtk_widget_set_name(button, "1");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(exp_action), this);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);

    button = gtk_button_new_with_label("2");
    gtk_widget_set_name(button, "2");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(exp_action), this);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);

    button = gtk_button_new_with_label("3");
    gtk_widget_set_name(button, "3");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(exp_action), this);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);

    button = gtk_button_new_with_label("4");
    gtk_widget_set_name(button, "4");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(exp_action), this);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);

    button = gtk_button_new_with_label("5");
    gtk_widget_set_name(button, "5");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(exp_action), this);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);

    if (!nopeek) {
        button = gtk_button_new_with_label("Peek Mode");
        gtk_widget_set_name(button, "peek");
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(exp_action), this);
        gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);
    }

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Go and Dismiss buttons
    //
    button = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(button, "apply");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(exp_action), this);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(exp_cancel_proc), this);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;
}


sExp::~sExp()
{
    if (p_parent) {
        win_bag *owner = dynamic_cast<win_bag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller && !p_no_desel)
        GRX->Deselect(p_caller);
    if (exp_popup) {
        gtk_signal_disconnect_by_func(GTK_OBJECT(exp_popup),
            GTK_SIGNAL_FUNC(exp_cancel_proc), this);
        // Destroy widget first, so "black hole" is filled in.
        gtk_widget_destroy(exp_popup);
    }
    if (p_caller)
        (*exp_callback)(0, exp_cb_arg);
}


// GRpopup override
//
void
sExp::popdown()
{
    if (!p_parent)
        return;
    gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
    if (owner)
        GRX->SetFocus(owner->Shell());
    if (!owner || !owner->MonitorActive(this))
        return;

    delete this;
}


// Set positioning and transient-for property, call before setting
// visible after creation.
//
void
sExp::initialize()
{
    win_bag *w = dynamic_cast<win_bag*>(p_parent);
    if (w && w->Shell()) {
        gtk_window_set_transient_for(GTK_WINDOW(exp_popup),
            GTK_WINDOW(w->Shell()));
        GRX->SetPopupLocation(GRloc(), exp_popup, w->Viewport());
    }
}


// Update the text input area.
//
void
sExp::update(const char *string)
{
    if (!p_parent)
        return;
    gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
    if (!owner || !owner->MonitorActive(this))
        return;

    if (!string)
        string = "";
    gtk_entry_set_text(GTK_ENTRY(exp_text), string);
}


// Event dispatch (private).
//
void
sExp::action_proc(GtkWidget *widget)
{
    const char *name = gtk_widget_get_name(widget);
    if (!strcmp(name, "plus")) {
        const char *t = gtk_entry_get_text(GTK_ENTRY(exp_text));
        if (t && *t == '+')
            gtk_entry_append_text(GTK_ENTRY(exp_text), "+");
        else
            gtk_entry_set_text(GTK_ENTRY(exp_text), "+");
        return;
    }
    if (!strcmp(name, "minus")) {
        const char *t = gtk_entry_get_text(GTK_ENTRY(exp_text));
        if (t && *t == '-')
            gtk_entry_append_text(GTK_ENTRY(exp_text), "-");
        else
            gtk_entry_set_text(GTK_ENTRY(exp_text), "-");
        return;
    }
    if (!strcmp(name, "all")) {
        gtk_entry_set_text(GTK_ENTRY(exp_text), name);
        return;
    }
    if (!strcmp(name, "0")) {
        gtk_entry_set_text(GTK_ENTRY(exp_text), name);
        return;
    }
    if (!strcmp(name, "1")) {
        gtk_entry_set_text(GTK_ENTRY(exp_text), name);
        return;
    }
    if (!strcmp(name, "2")) {
        gtk_entry_set_text(GTK_ENTRY(exp_text), name);
        return;
    }
    if (!strcmp(name, "3")) {
        gtk_entry_set_text(GTK_ENTRY(exp_text), name);
        return;
    }
    if (!strcmp(name, "4")) {
        gtk_entry_set_text(GTK_ENTRY(exp_text), name);
        return;
    }
    if (!strcmp(name, "5")) {
        gtk_entry_set_text(GTK_ENTRY(exp_text), name);
        return;
    }
    if (!strcmp(name, "peek")) {
        gtk_entry_set_text(GTK_ENTRY(exp_text), "p");
        return;
    }
    if (!strcmp(name, "Help")) {
        if (mainBag())
            mainBag()->PopUpHelp("xic:expnd");
        return;
    }
    if (!strcmp(name, "apply")) {
        const char *string = gtk_entry_get_text(GTK_ENTRY(exp_text));
        if (!widget || (exp_callback && !(*exp_callback)(string, exp_cb_arg)))
            popdown();
    }
}


// Private static GTK signal handler.
//
void
sExp::exp_cancel_proc(GtkWidget*, void *client_data)
{
    sExp *exp = static_cast<sExp*>(client_data);
    if (exp)
        exp->popdown();
}


// Private static GTK signal handler.
//
void
sExp::exp_action(GtkWidget *widget, void *client_data)
{
    sExp *exp = static_cast<sExp*>(client_data);
    if (exp)
        exp->action_proc(widget);
}


// Private static GTK signal handler.
// The text position is initially 0, on the first text change delete
// any existing text and unlink this handler.
//
void
sExp::exp_change_proc(GtkWidget*, void *client_data)
{
    sExp *exp = static_cast<sExp*>(client_data);
    if (exp) {
        gtk_signal_disconnect_by_func(GTK_OBJECT(exp->exp_text),
            GTK_SIGNAL_FUNC(exp_change_proc), exp);
        int pos = gtk_editable_get_position(GTK_EDITABLE(exp->exp_text));
        gtk_editable_delete_text(GTK_EDITABLE(exp->exp_text), pos+1, -1);
    }
}


// Private static GTK signal handler.
// Handle keypresses to the pop-up.  The pop-up has the focus when it
// appears.  Return is taken as an "Apply" termination.
//
int
sExp::exp_key_hdlr(GtkWidget *widget, GdkEvent *event, void *client_data)
{
    sExp *exp = static_cast<sExp*>(client_data);
    if (exp && event->key.keyval == GDK_Return) {
        gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "key_press_event");
        const char *string = gtk_entry_get_text(GTK_ENTRY(exp->exp_text));
        if (!exp->p_caller || (exp->exp_callback &&
                !(*exp->exp_callback)(string, exp->exp_cb_arg)))
            exp->popdown();
        return (true);
    }
    return (false);
}

