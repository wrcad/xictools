
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
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "miscutil/lstring.h"
#include "miscutil/filestat.h"
#include "gtkinterf.h"
#include "gtkutil.h"
#include "gtkfont.h"
#ifdef HAVE_MOZY
#include "gtkmozy/gtkviewer.h"
#include "htm/htm_callback.h"
#include "help/help_defs.h"
#include "help/help_context.h"
#endif

#include <gdk/gdkkeysyms.h>
#ifdef WITH_X11
#include <X11/Xproto.h>
#else
#ifdef WIN32
#include <gdk/gdkwin32.h>
#endif
#endif


// WARNING:  don't use gtk_window_move on a mapped window, also
// gdk_window_move and gtk_set_uposition.  These functions are f'ed up
// big-time presently (10/12/15) on El Capitan X11.  Windows are moved
// off screen instead of to the given location.  The calls are ok if
// the window is not presently mapped.  We used to set a handler for
// new pop-ups to readjust the positioning once the pop-up was mapped. 
// This is no longer done, we try and set the positioning once only. 
// This works well except for Windows, where the positioning is off by
// a bit, but acceptable.

// Locate the sub widget over the parent, positioned according to code.
//
void
GTKdev::SetPopupLocation(GRloc loc, GtkWidget *sub, GtkWidget *parent)
{
    int x, y;
    ComputePopupLocation(loc, sub, parent, &x, &y);
    // This computes the location assuming that the widget is not
    // decorated.  On the first call, find the top-level shell and
    // obtain the decoration dimensions for location compensation.

    static int dec_ht = 0;
    static int dec_wd = 0;
    if (dec_ht == 0) {
#ifdef WIN32
        // The code below gives 16,43 for the values, which are not
        // good visually.  The following values look about right for
        // Windows 10.

        dec_wd = 5;
        dec_ht = 28;
#else
        GtkWidget *p = parent;
        while (gtk_widget_get_parent(p) != 0)
            p = gtk_widget_get_parent(p);
        GdkRectangle rect, rect_d;
        ShellGeometry(p, &rect, &rect_d);
        dec_wd = rect_d.width - rect.width;
        dec_ht = rect_d.height - rect.height;
#endif
    }

    // The dv_lower_win_offset tweaks the vertical position of windows
    // that abut the lower edge of the viewport.  In "plasma"
    // displays, we may want to move this windows up to avoid the
    // "shadow" falling over a text input line (as in Xic).

#ifdef WITH_QUARTZ
    if (loc.code == LW_LL)
        y -= dv_lower_win_offset;
    else if (loc.code == LW_UL)
        y += dec_ht;
    else if (loc.code == LW_LR) {
        x -= dec_wd;
        y -= dv_lower_win_offset;
    }
    else if (loc.code == LW_UR) {
        x -= dec_wd;
        y += dec_ht;
    }
    else if (loc.code == LW_CENTER) {
        x -= dec_wd/2;
        y += dec_ht/2;
    }
#else
    if (loc.code == LW_LL)
        y -= (dec_ht + dv_lower_win_offset);
    else if (loc.code == LW_LR) {
        x -= dec_wd;
        y -= (dec_ht + dv_lower_win_offset);
    }
    else if (loc.code == LW_UR)
        x -= dec_wd;
    else if (loc.code == LW_CENTER) {
        x -= dec_wd/2;
        y -= dec_ht/2;
    }
#endif
    gtk_window_move(GTK_WINDOW(sub), x, y);
}


// Return screen coordinates xpos, ypos where the widget is to be
// located according to loc.
//
void
GTKdev::ComputePopupLocation(GRloc loc, GtkWidget *sub, GtkWidget *parent,
    int *xpos, int *ypos)
{
    int x, y;
    GdkWindow *win = gtk_widget_get_window(parent);
    gdk_window_get_origin(win, &x, &y);
    int mwidth = gdk_window_get_width(win);
    int mheight = gdk_window_get_height(win);

#ifdef WIN32
    // There seems to be a GTK bug that causes a difference in
    // placement of resizable vs non-resizable windows.
    
    if (gtk_window_get_resizable(GTK_WINDOW(sub))) {
        x -= 5;
        y -= 5;
    }
#endif

    int swidth = 0, sheight;
    GtkRequisition req;
    gtk_widget_size_request(sub, &req);
    swidth = req.width;
    sheight = req.height;

    switch (loc.code) {
    case LW_CENTER:
    default:
        x += (mwidth - swidth)/2;
        y += (mheight - sheight)/2;
        break;
    case LW_LL:
        y += mheight - sheight;
        break;
    case LW_LR:
        y += mheight - sheight;
        x += mwidth - swidth;
        break;
    case LW_UL:
        break;
    case LW_UR:
        x += mwidth - swidth;
        break;
    case LW_XYR:
        x += loc.xpos;
        y += loc.ypos;
        break;
    case LW_XYA:
        x = loc.xpos;
        y = loc.ypos;
        break;
    }
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    int mx, my, mwid, mhei;
    MonitorGeom(parent, &mx, &my, &mwid, &mhei);
    if (x + swidth > mx + mwid)
        x = mx + mwid - swidth;
    if (y + sheight > my + mhei)
        y = my + mhei - sheight;
    *xpos = x;
    *ypos = y;
}


// Find the position and size of the *decorated* widget.
//
void
GTKdev::WidgetLocation(GtkWidget *w, int *x, int *y, int *wid, int *hei)
{
    GdkRectangle rect;
    if (gtk_widget_get_window(w) && ShellGeometry(w, 0, &rect)) {
        *x = rect.x;
        *y = rect.y;
        *wid = rect.width;
        *hei = rect.height;
    }
    else {
        *x = 0;
        *y = 0;
        *wid = 0;
        *hei = 0;
    }
}


// Give the focus to the passed GtkWindow.
//
void
GTKdev::SetFocus(GtkWidget *window)
{
    if (window && gtk_widget_get_window(window)) {
#ifdef WITH_X11
        XSetInputFocus(gr_x_display(),
            gr_x_window(gtk_widget_get_window(window)),
            RevertToPointerRoot, CurrentTime);
#else
#ifdef WIN32
        ::SetFocus((HWND)GDK_WINDOW_HWND(gtk_widget_get_window(window)));
#endif
#endif
    }
}


// The following function enables a (button 2) double-click-to-cancel
// feature for pop-up widgets.  The widget arg is usually the shell,
// the cancel arg is the cancel button.

namespace {
    int dc_btn_hdlr(GtkWidget*, GdkEvent*, void*);
    int dc_leave_hdlr(GtkWidget*, GdkEvent*, void*);
    int dc_spinbtn_hdlr(GtkWidget*, GdkEvent*, void*);
}

void
GTKdev::SetDoubleClickExit(GtkWidget *widget, GtkWidget *cancel)
{
    gtk_widget_add_events(widget, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(widget), "button-press-event",
        G_CALLBACK(dc_btn_hdlr), cancel);
    gtk_widget_add_events(widget, GDK_LEAVE_NOTIFY_MASK);
    g_signal_connect(G_OBJECT(widget), "leave-notify-event",
        G_CALLBACK(dc_leave_hdlr), 0);
}


void
GTKdev::RegisterBigWindow(GtkWidget *window)
{
#ifdef WITH_X11
    dv_big_window_xid = 0;
    if (window && gtk_widget_get_window(window))
        dv_big_window_xid = gr_x_window(gtk_widget_get_window(window));
#else
    (void)window;
#endif
}


void
GTKdev::RegisterBigForeignWindow(unsigned int xid)
{
#ifdef WITH_X11
    dv_big_window_xid = xid;
#else
    (void)xid;
#endif
}
// End of GTKdev functions.


//
//  Utility Widgets
//
//  The following data keywords are set in each instance:
//    shell:        "bag"   (the calling widget bag)
//    cancel btn:   "shell" (the widget's shell)

// Idle function to attach the pop-up popdown function to the calling
// button, so that pressing the button pops down the pop-up.  This is
// done in an idle proc since if done in the main function, the initiating
// signal is still active, leading to a spurrious call to the popdown
// function.

namespace {
    void
    popdown_widget(GtkWidget*, void *arg)
    {
        GRpopup *p = static_cast<GRpopup*>(arg);
        p->popdown();
    }
}


void
GTKbag::ClearPopups()
{
    if (wb_message)
        wb_message->popdown();
    if (wb_input)
        wb_input->popdown();
    if (wb_warning)
        wb_warning->popdown();
    if (wb_error)
        wb_error->popdown();
    if (wb_info)
        wb_info->popdown();
    if (wb_info2)
        wb_info2->popdown();
    if (wb_htinfo)
        wb_htinfo->popdown();
    GRobject o;
    while ((o = wb_monitor.first_object()) != 0) {
        GRpopup *p = static_cast<GRpopup*>(o);
        p->popdown();
    }
}


// Clean up after a pop-up is destroyed, called from destructors.
//
void
GTKbag::ClearPopup(GRpopup *popup)
{
    MonitorRemove(popup);
    if (popup == wb_input) {
        wb_input = 0;
        if (wb_sens_set)
            (*wb_sens_set)(this, true, 0);
    }
    else if (popup == wb_message) {
        if (wb_message->is_desens() && wb_input)
            gtk_widget_set_sensitive(wb_input->pw_shell, true);
        wb_message = 0;
    }
    else if (popup == wb_info) {
        wb_info = 0;
        wb_info_cnt++;
    }
    else if (popup == wb_info2) {
        wb_info2 = 0;
        wb_info2_cnt++;
    }
    else if (popup == wb_htinfo) {
        wb_htinfo = 0;
        wb_htinfo_cnt++;
    }
    else if (popup == wb_warning) {
        wb_warning = 0;
        wb_warn_cnt++;
    }
    else if (popup == wb_error) {
        wb_error = 0;
        wb_err_cnt++;
    }
    else if (popup == wb_fontsel) {
        wb_fontsel = 0;
    }
}


// Note about signals - the caller can be one of:
// caller             signals
// command button     clicked
// toggle button      clicked
// radio button       clicked
// menu item          activate
// check menu item    toggled
// radio menu item    toggled


//-----------------------------------------------------------------------------
// Simple yes/no popup, not linked into bag, no instancing limit.
// Arg caller is button, if given the popup will be destroyed without
// calling callback is caller is pressed while popup is active.
// Args x,y are shell coordinates for popup.  Arg action_callback is
// called with a bool and action_arg when choice is made.

GTKaffirmPopup::GTKaffirmPopup(GTKbag *owner, const char *question_str,
    void *arg)
{
    p_parent = owner;
    p_cb_arg = arg;
    pw_yes = 0;             // yes button
    pw_label = 0;           // question label
    pw_affirmed = false;    // yes button presed

    if (owner)
        owner->MonitorAdd(this);

    pw_shell = gtk_NewPopup(owner, "Yes or No?", pw_affirm_popdown, this);
    if (!pw_shell)
        return;
    gtk_window_set_resizable(GTK_WINDOW(pw_shell), false);
    g_object_set_data(G_OBJECT(pw_shell), "affirm_w", this);

    GtkWidget *form = gtk_vbox_new(false, 2);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_widget_add_events(pw_shell, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(pw_shell), "key-press-event",
        G_CALLBACK(pw_affirm_key), this);
    gtk_container_add(GTK_CONTAINER(pw_shell), form);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    pw_label = gtk_label_new(question_str);
    gtk_widget_show(pw_label);
    gtk_misc_set_padding(GTK_MISC(pw_label), 2, 2);
    gtk_container_add(GTK_CONTAINER(frame), pw_label);
    gtk_box_pack_start(GTK_BOX(form), frame, true, true, 0);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    pw_yes = gtk_button_new_with_label("Yes");
    gtk_widget_set_name(pw_yes, "Yes");
    gtk_widget_show(pw_yes);
    gtk_box_pack_start(GTK_BOX(hbox), pw_yes, true, true, 0);
    g_signal_connect(G_OBJECT(pw_yes), "clicked",
        G_CALLBACK(pw_affirm_button), this);

    pw_cancel = gtk_button_new_with_label("No");
    gtk_widget_set_name(pw_cancel, "No");
    gtk_widget_show(pw_cancel);
    gtk_box_pack_start(GTK_BOX(hbox), pw_cancel, true, true, 0);
    g_signal_connect(G_OBJECT(pw_cancel), "clicked",
        G_CALLBACK(pw_affirm_button), this);
    gtk_window_set_focus(GTK_WINDOW(pw_shell), pw_cancel);

    gtk_box_pack_start(GTK_BOX(form), hbox, false, false, 0);
}


GTKaffirmPopup::~GTKaffirmPopup()
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }
    if (p_callback)
        (*p_callback)(pw_affirmed, p_cb_arg);
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller && !p_no_desel)
        GRX->Deselect(p_caller);
    if (p_caller_data)
        g_signal_handlers_disconnect_by_func(G_OBJECT(p_caller),
            (gpointer)popdown_widget, this);
    g_signal_handlers_disconnect_by_func(G_OBJECT(pw_shell),
        (gpointer)pw_affirm_popdown, this);

    gtk_widget_destroy(pw_shell);
}


// GRpopup override
// Register the calling button, and set up:
//  1.  whether or not the caller is deselected on popdwon.
//  2.  whether or not deselecting the caller causes popdown.
//
void
GTKaffirmPopup::register_caller(GRobject c, bool no_dsl, bool handle_popdn)
{
    p_caller = c;
    p_no_desel = no_dsl;
    if (handle_popdn) {
        GtkWidget *w = static_cast<GtkWidget*>(c);
        if (w) {
            // pop down if user unsets caller
            const char *sig = 0;
            if (GTK_IS_BUTTON(w) || GTK_IS_TOGGLE_BUTTON(w) ||
                    GTK_IS_RADIO_BUTTON(w))
                sig = "clicked";
            else if (GTK_IS_MENU_ITEM(w))
                sig = "activate";
            else if (GTK_IS_CHECK_MENU_ITEM(w) ||
                    GTK_IS_RADIO_MENU_ITEM(w))
                sig = "toggled";
            if (sig) {
                p_caller_data = (void*)sig;
                g_idle_add((GSourceFunc)pw_attach_idle_proc, this);
            }
        }
    }
}


// GRpopup override
//
void
GTKaffirmPopup::popdown()
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// Private static GTK signal handler.
// Respond to keypress: popup is destroyed, callback is called with
// true if 'y' or 'Y' pressed, false otherwise.
//
int
GTKaffirmPopup::pw_affirm_key(GtkWidget*, GdkEvent *ev, void *client_data)
{
    GTKaffirmPopup *p = static_cast<GTKaffirmPopup*>(client_data);
    if (p && ev->key.string) {
        if (*ev->key.string == 'y' || *ev->key.string == 'Y')
            p->pw_affirmed = true;
        p->popdown();
    }
    return (true);
}


// Private static GTK signal handler.
// Pop down and destroy.
//
void
GTKaffirmPopup::pw_affirm_button(GtkWidget *widget, void *client_data)
{
    GTKaffirmPopup *p = static_cast<GTKaffirmPopup*>(client_data);
    if (p) {
        if (widget == p->pw_yes)
            p->pw_affirmed = true;
        p->popdown();
    }
}


// Private static GTK signal handler.
//
void
GTKaffirmPopup::pw_affirm_popdown(GtkWidget*, void *client_data)
{
    GTKaffirmPopup *p = static_cast<GTKaffirmPopup*>(client_data);
    if (p)
        p->popdown();
}


// Private static GTK signal handler.
//
int
GTKaffirmPopup::pw_attach_idle_proc(void *arg)
{
    GTKaffirmPopup *p = static_cast<GTKaffirmPopup*>(arg);
    if (p->p_caller_data && p->p_caller) {
        g_signal_connect(G_OBJECT(p->p_caller),
            (const char*)p->p_caller_data,
            G_CALLBACK(popdown_widget), p);
    }
    return (false);
}
// End of GTKaffirm functions


//-----------------------------------------------------------------------------
// Popup to solicit a numeric value, consisting of a label and a spin
// button.

GTKnumPopup::GTKnumPopup(GTKbag *owner, const char *prompt_str,
    double initd, double mind, double maxd, double del, int numd, void *arg)
{
    p_parent = owner;
    p_cb_arg = arg;
    pw_yes = 0;
    pw_label = 0;
    pw_text = 0;
    pw_value = pw_tmp_value = 0.0;
    pw_mind = pw_maxd = 0.0;
    pw_setentr = false;
    pw_affirmed = false;

    if (owner)
        owner->MonitorAdd(this);

    pw_shell = gtk_NewPopup(owner, "Numeric Entry", pw_numer_popdown, this);
    if (!pw_shell)
        return;
    gtk_window_set_resizable(GTK_WINDOW(pw_shell), false);
    g_object_set_data(G_OBJECT(pw_shell), "numer_w", this);
    BlackHoleFix(pw_shell);

    GtkWidget *form = gtk_table_new(1, 4, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(pw_shell), form);

    // Label in frame.
    //
    pw_label = gtk_label_new(prompt_str);
    gtk_widget_show(pw_label);
    gtk_misc_set_padding(GTK_MISC(pw_label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), pw_label);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // Text area, spin button.
    //
    if (mind > maxd) { double tmp = mind; mind = maxd; maxd = tmp; }
    if (del <= 0 || del >= maxd - mind)
        del = 0.1*(maxd - mind);
    if (initd < mind)
        initd = mind;
    else if (initd > maxd)
        initd = maxd;
    gfloat pagesize = del;
    gfloat climb_rate = 0.0;
    if ((maxd - mind)/del > 100) {
        pagesize = 64*del;
        climb_rate = 4*del;
    }
    pw_mind = mind;
    pw_maxd = maxd;
    pw_value = pw_tmp_value = initd;
    GtkObject *adj = gtk_adjustment_new(initd, mind, maxd, del, pagesize, 0);
    pw_text = gtk_spin_button_new(GTK_ADJUSTMENT(adj), climb_rate, numd);
    gtk_widget_show(pw_text);
    g_signal_connect(G_OBJECT(pw_text), "changed",
        G_CALLBACK(pw_numer_val_changed), this);
    gtk_window_set_focus(GTK_WINDOW(pw_shell), pw_text);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(pw_text), true);

    gtk_table_attach(GTK_TABLE(form), DblClickSpinBtnContainer(pw_text),
        0, 1, 1, 2,
        (GtkAttachOptions)0,
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // Buttons.
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    pw_yes = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(pw_yes, "Apply");
    gtk_widget_show(pw_yes);
    g_signal_connect(G_OBJECT(pw_yes), "clicked",
        G_CALLBACK(pw_numer_button), this);
    gtk_box_pack_start(GTK_BOX(hbox), pw_yes, true, true, 0);

    pw_cancel = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(pw_cancel, "Dismiss");
    gtk_widget_show(pw_cancel);
    g_signal_connect(G_OBJECT(pw_cancel), "clicked",
        G_CALLBACK(pw_numer_button), this);
    gtk_box_pack_start(GTK_BOX(hbox), pw_cancel, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
}


GTKnumPopup::~GTKnumPopup()
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }
    if (p_callback)
        (*p_callback)(pw_value, pw_affirmed, p_cb_arg);
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller && !p_no_desel)
        GRX->Deselect(p_caller);
    if (p_caller_data)
        g_signal_handlers_disconnect_by_func(G_OBJECT(p_caller),
            (gpointer)popdown_widget, this);
    g_signal_handlers_disconnect_by_func(G_OBJECT(pw_shell),
        (gpointer)pw_numer_popdown, this);

    gtk_widget_destroy(pw_shell);
}


// GRpopup override
// Register the calling button, and set up:
//  1.  whether or not the caller is deselected on popdwon.
//  2.  whether or not deselecting the caller causes popdown.
//
void
GTKnumPopup::register_caller(GRobject c, bool no_dsl, bool handle_popdn)
{
    p_caller = c;
    p_no_desel = no_dsl;
    if (handle_popdn) {
        GtkWidget *w = static_cast<GtkWidget*>(c);
        if (w) {
            // pop down if user unsets caller
            const char *sig = 0;
            if (GTK_IS_BUTTON(w) || GTK_IS_TOGGLE_BUTTON(w) ||
                    GTK_IS_RADIO_BUTTON(w))
                sig = "clicked";
            else if (GTK_IS_MENU_ITEM(w))
                sig = "activate";
            else if (GTK_IS_CHECK_MENU_ITEM(w) ||
                    GTK_IS_RADIO_MENU_ITEM(w))
                sig = "toggled";
            if (sig) {
                p_caller_data = (void*)sig;
                g_idle_add((GSourceFunc)pw_attach_idle_proc, this);
            }
        }
    }
}


// GRpopup override
//
void
GTKnumPopup::popdown()
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// The spin button widget has some problems (gtk+ 1.2.4)
// 1. The value returned from get_value_as_float() is not exact, and
//    seems to contain the accumulated errors from scrolling.
// 2. If one tries to get the value from the string, the number of
//    digits reverts back to the default precision as soon as the
//    button is pressed, before the inputted value can be read.
// 3. Digit numbers greater than 5 are not allowed, and in fact cause
//    a program crash.
// Here, the value is obtained from the "changed" signal for the entry
// widget.  In the popdown callback, the second most recently set value
// is used, as the last value is truncated.  Thus, it is possible to
// enter arbitrary numbers, and the value reported is always the exact
// value as shown in the entry widget.

// Private static GTK signal handler.
//
void
GTKnumPopup::pw_numer_val_changed(GtkWidget*, void *client_data)
{
    GTKnumPopup *p = static_cast<GTKnumPopup*>(client_data);
    if (p) {
        if (!p->pw_setentr) {
            // After the text changes, Enter calls the Apply method
            g_signal_connect(G_OBJECT(p->pw_shell), "key-press-event",
                G_CALLBACK(pw_numer_key_hdlr), p);
            p->pw_setentr = true;
        }
        const char *s = gtk_entry_get_text(GTK_ENTRY(p->pw_text));
        char *error = 0;
        double d = strtod(s, &error);
        if (d < p->pw_mind)
            d = p->pw_mind;
        else if (d > p->pw_maxd)
            d = p->pw_maxd;
        p->pw_value = p->pw_tmp_value;
        p->pw_tmp_value = d;
    }
}


// Private static GTK signal handler.
// In single-line mode, Return is taken as an "OK" termination.
//
int
GTKnumPopup::pw_numer_key_hdlr(GtkWidget*, GdkEvent *ev, void *client_data)
{
    GTKnumPopup *p = static_cast<GTKnumPopup*>(client_data);
    if (p && ev->key.keyval == GDK_Return) {
        gtk_button_clicked(GTK_BUTTON(p->pw_yes));
        return (true);
    }
    return (false);
}


// Private static GTK signal handler.
//
void
GTKnumPopup::pw_numer_button(GtkWidget *widget, void *client_data)
{
    GTKnumPopup *p = static_cast<GTKnumPopup*>(client_data);
    if (p) {
        // If the last two values are the same to the precision, the
        // pw_tmp_value must be the truncated version.
        char b1[64], b2[64];
        sprintf(b1, "%0.*f",
            (int)gtk_spin_button_get_digits(GTK_SPIN_BUTTON(p->pw_text)),
            p->pw_value);
        sprintf(b2, "%0.*f",
            (int)gtk_spin_button_get_digits(GTK_SPIN_BUTTON(p->pw_text)),
            p->pw_tmp_value);
        double d;
        if (!strcmp(b1, b2))
            d = p->pw_value;
        else
            d = p->pw_tmp_value;

        p->pw_affirmed = (widget == p->pw_yes);
        p->pw_value = d;
        p->popdown();
    }
}


// Private static GTK signal handler.
//
void
GTKnumPopup::pw_numer_popdown(GtkWidget*, void *client_data)
{
    GTKnumPopup *p = static_cast<GTKnumPopup*>(client_data);
    if (p)
        p->popdown();
}


// Private static GTK signal handler.
//
int
GTKnumPopup::pw_attach_idle_proc(void *arg)
{
    GTKnumPopup *p = static_cast<GTKnumPopup*>(arg);
    if (p->p_caller_data && p->p_caller) {
        g_signal_connect(G_OBJECT(p->p_caller),
            (const char*)p->p_caller_data,
            G_CALLBACK(popdown_widget), p);
    }
    return (false);
}
// End of GTKnumPopup functions


//-----------------------------------------------------------------------------
// Simple popup to solicit a text string.  Arg caller, if given, is
// the initiating button, and will destroy the popup if pressed while
// the popup is active.  When finished, action_callback will be called
// with the string (malloced) and action_arg.  Arg textwidth is the
// width of the input field.  hold the cancel widget.  Arg downproc is
// a function executed when the widget is destroyed, its arg is true
// if popping down from OK button, false otherwise.

namespace {
    // Drag/drop stuf.
    //
    GtkTargetEntry target_table[] = {
        { (char*)"CELLNAME",    0, 0 },
        { (char*)"STRING",      0, 1 },
        { (char*)"text/plain",  0, 2 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);
}

GTKledPopup::GTKledPopup(GTKbag *owner, const char *prompt_str,
    const char *init_str, int textwidth, bool multiline, const char *btnstr,
    void *arg)
{
    p_parent = owner;
    p_cb_arg = arg;
    pw_vcallback = 0;
    pw_yes = 0;
    pw_label = 0;
    pw_text = 0;
    pw_applied = false;
    pw_ign_ret = false;
    pw_no_drops = false;

    if (init_str && strchr(init_str, '\n'))
        multiline = true;

    if (owner)
        owner->MonitorAdd(this);

    pw_shell = gtk_NewPopup(owner, "Text Entry", pw_editstr_popdown, this);
    g_object_set_data(G_OBJECT(pw_shell), "edit_w", this);

    GtkWidget *form = gtk_table_new(1, 4, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(pw_shell), form);

    // Label in frame.
    //
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    pw_label = gtk_label_new(prompt_str ? prompt_str : "Enter filename:");
    gtk_widget_show(pw_label);
    gtk_misc_set_padding(GTK_MISC(pw_label), 2, 2);
    gtk_container_add(GTK_CONTAINER(frame), pw_label);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // A bit o' hackery: if the prompt_str begins with "<tt>", use a fixed
    // font and left justify.
    if (prompt_str && lstring::ciprefix("<tt>", prompt_str)) {
        prompt_str += 4;
        gtk_label_set_text(GTK_LABEL(pw_label), prompt_str);
        gtk_label_set_justify(GTK_LABEL(pw_label), GTK_JUSTIFY_LEFT);

        GTKfont::setupFont(pw_label, FNT_FIXED, true);
    }

    // This allows user to change label text.
    g_object_set_data(G_OBJECT(pw_shell), "label", pw_label);

    // Text area, text widget if multi-line, entry widget if not.
    //
    if (!multiline) {
        pw_text = gtk_entry_new();
        gtk_widget_show(pw_text);
        if (init_str)
            gtk_entry_set_text(GTK_ENTRY(pw_text), init_str);
        gtk_editable_set_editable(GTK_EDITABLE(pw_text), true);
        gtk_table_attach(GTK_TABLE(form), pw_text, 0, 1, 1, 2,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 0);

        if (textwidth < 160)
            textwidth = 160;
        if (init_str) {
            int twid = GTKfont::stringWidth(pw_text, init_str);
            if (textwidth < twid)
                textwidth = twid;
        }
        gtk_widget_set_size_request(pw_text, textwidth + 10, -1);

        // drop site
        // For GtkEntry, in GTK-2, including GTK_DEST_DEFAULT_DROP will
        // set up a separate drop handler, so data_get/data_received is
        // actually done twice.  However GTK-1 needs this or the
        // transfer won't be done at all.
        GtkDestDefaults DD = (GtkDestDefaults)
            (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT);
        gtk_drag_dest_set(pw_text, DD, target_table, n_targets,
            GDK_ACTION_COPY);
        g_signal_connect_after(G_OBJECT(pw_text), "drag-data-received",
            G_CALLBACK(pw_editstr_drag_data_received), 0);

        // Handle Return, same as pressing pw_yes.
        g_signal_connect(G_OBJECT(pw_shell), "key-press-event",
            G_CALLBACK(pw_editstr_key), this);
    }
    else {
        GtkWidget *contr;
        text_scrollable_new(&contr, &pw_text, FNT_EDITOR);
        text_set_chars(pw_text, init_str);
        text_set_editable(pw_text, true);

        gtk_table_attach(GTK_TABLE(form), contr, 0, 1, 1, 2,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);
        int hh = 8*GTKfont::stringHeight(pw_text, 0);
        gtk_widget_set_size_request(GTK_WIDGET(pw_text), textwidth, hh);
    }
    gtk_window_set_focus(GTK_WINDOW(pw_shell), pw_text);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // Buttons.
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    if (!btnstr)
        btnstr = "Apply";
    pw_yes = gtk_button_new_with_label(btnstr);
    gtk_widget_set_name(pw_yes, btnstr);
    gtk_widget_show(pw_yes);
    g_signal_connect(G_OBJECT(pw_yes), "clicked",
        G_CALLBACK(pw_editstr_button), this);
    gtk_box_pack_start(GTK_BOX(hbox), pw_yes, true, true, 0);

    pw_cancel = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(pw_cancel, "Dismiss");
    gtk_widget_show(pw_cancel);
    g_signal_connect(G_OBJECT(pw_cancel), "clicked",
        G_CALLBACK(pw_editstr_button), this);
    gtk_box_pack_start(GTK_BOX(hbox), pw_cancel, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
}


GTKledPopup::~GTKledPopup()
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (owner)
            owner->ClearPopup(this);
    }
    if (p_cancel)
        (*p_cancel)(pw_applied);
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller && !p_no_desel)
        GRX->Deselect(p_caller);
    if (p_caller_data)
        g_signal_handlers_disconnect_by_func(G_OBJECT(p_caller),
            (gpointer)popdown_widget, this);
    g_signal_handlers_disconnect_by_func(G_OBJECT(pw_shell),
        (gpointer)pw_editstr_popdown, this);

    gtk_widget_destroy(pw_shell);
}


// GRpopup override
// Register the calling button, and set up:
//  1.  whether or not the caller is deselected on popdwon.
//  2.  whether or not deselecting the caller causes popdown.
//
void
GTKledPopup::register_caller(GRobject c, bool no_dsl, bool handle_popdn)
{
    p_caller = c;
    p_no_desel = no_dsl;
    if (handle_popdn) {
        GtkWidget *w = static_cast<GtkWidget*>(c);
        if (w) {
            // pop down if user unsets caller
            const char *sig = 0;
            if (GTK_IS_BUTTON(w) || GTK_IS_TOGGLE_BUTTON(w) ||
                    GTK_IS_RADIO_BUTTON(w))
                sig = "clicked";
            else if (GTK_IS_MENU_ITEM(w))
                sig = "activate";
            else if (GTK_IS_CHECK_MENU_ITEM(w) ||
                    GTK_IS_RADIO_MENU_ITEM(w))
                sig = "toggled";
            if (sig) {
                p_caller_data = (void*)sig;
                g_idle_add((GSourceFunc)pw_attach_idle_proc, this);
            }
        }
    }
}


// GRpopup override
//
void
GTKledPopup::popdown()
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// GRledPopup override
//
void
GTKledPopup::update(const char *prompt_str, const char *init_str)
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    if (prompt_str)
        gtk_label_set_text(GTK_LABEL(pw_label), prompt_str);
    if (init_str)
        gtk_entry_set_text(GTK_ENTRY(pw_text), init_str);
}


// Pop down and destroy, call callback.  Note that if the callback
// returns nonzero but not one, the caller is not deselected.
//
void
GTKledPopup::button_hdlr(GtkWidget *widget)
{
    // This component is used for both PopUpEditString and PopUpInput,
    // which are similar but have the following differences:
    // 1) PopUpInput hangs a pointer in the widget bag, so there can only
    //    be one editor per bag.
    // 2) The action callback returns void for PopUpInput, there is a
    //    separate function to remove the pop-up.
    // If created from PopUpInput, the pw_shell will have data "edit_w"
    // pointing to this.  This is otherwise unreferenced so must be
    // deleted when popping down.

    if (pw_ign_ret) {
        // We were created from PopUpInput.  Don't pop down after
        // action (ignore return).
        if (widget == pw_yes) {
            if (pw_vcallback) {
                char *string = gtk_editable_get_chars(GTK_EDITABLE(pw_text),
                    0, -1);
                (*pw_vcallback)(string, p_cb_arg);
                free(string);
            }
        }
        else {
            // The caller != pw_cancel here if handling window delete
            // from title bar.
            popdown();
        }
        return;
    }

    int ret = ESTR_DN;
    if (p_callback && widget == pw_yes) {
        GRX->Deselect(widget);
        char *string = gtk_editable_get_chars(GTK_EDITABLE(pw_text), 0, -1);
        ret = (*p_callback)(string, p_cb_arg);
        free(string);
    }
    if (ret != ESTR_IGN) {
        if (ret == ESTR_DN_NODESEL)
            p_no_desel = true;
        if (widget == pw_yes)
            pw_applied = true;
        popdown();
    }
}


// Private static GTK signal handler.
// In single-line mode, Return is taken as an "OK" termination.
//
int
GTKledPopup::pw_editstr_key(GtkWidget*, GdkEvent *ev, void *client_data)
{
    GTKledPopup *p = static_cast<GTKledPopup*>(client_data);
    if (p && ev->key.keyval == GDK_Return) {
        p->button_hdlr(p->pw_yes);
        return (true);
    }
    return (false);
}


// Private static GTK signal handler.
//
void
GTKledPopup::pw_editstr_button(GtkWidget *widget, void *client_data)
{
    GTKledPopup *p = static_cast<GTKledPopup*>(client_data);
    if (p)
        p->button_hdlr(widget);
}


// Private static GTK signal handler.
//
void
GTKledPopup::pw_editstr_popdown(GtkWidget*, void *client_data)
{
    GTKledPopup *p = static_cast<GTKledPopup*>(client_data);
    if (p)
        p->popdown();
}


// Private static GTK signal handler.
// Drag data received in editing window, grab it
//
void
GTKledPopup::pw_editstr_drag_data_received(GtkWidget *entry,
    GdkDragContext *context, gint, gint, GtkSelectionData *data,
    guint, guint time)
{
    if (gtk_selection_data_get_length(data) >= 0 &&
            gtk_selection_data_get_format(data) == 8 &&
            gtk_selection_data_get_data(data)) {
        char *src = (char*)gtk_selection_data_get_data(data);
        gtk_entry_set_text(GTK_ENTRY(entry), src);
        GdkWindow *win = gtk_widget_get_window(entry);
        int wid = gdk_window_get_width(win);
        int twid = GTKfont::stringWidth(entry, src);
        if (wid < twid) {
            GtkAllocation a;
            gtk_widget_get_allocation(entry, &a);
            a.width = twid;
            gtk_widget_size_allocate(entry, &a);
            gtk_widget_queue_resize(entry);
        }
        gtk_drag_finish(context, true, false, time);
    }
    gtk_drag_finish(context, false, false, time);
}


// Private static GTK signal handler.
//
int
GTKledPopup::pw_attach_idle_proc(void *arg)
{
    GTKledPopup *p = static_cast<GTKledPopup*>(arg);
    if (p->p_caller_data && p->p_caller) {
        g_signal_connect(G_OBJECT(p->p_caller),
            (const char*)p->p_caller_data,
            G_CALLBACK(popdown_widget), p);
    }
    return (false);
}
// End of GTKledPopup functions


//----------------------------------------------------------------------------
// Simple message box.

GTKmsgPopup::GTKmsgPopup(GTKbag *owner, const char *string, bool err)
{
    p_parent = owner;
    pw_label = 0;
    pw_desens = false;

    if (owner)
        owner->MonitorAdd(this);

    pw_shell = gtk_NewPopup(owner,
        err ? "ERROR" : "Message", pw_message_popdown, this);
    g_object_set_data(G_OBJECT(pw_shell), "message_w", this);

    // Hit any key to pop down.
    //
    gtk_widget_add_events(pw_shell, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(pw_shell), "key-press-event",
        G_CALLBACK(pw_message_popdown_ev), this);

    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(pw_shell), form);

    // The label, in a frame.
    //
    pw_label = gtk_label_new(string);
    gtk_widget_show(pw_label);
    gtk_misc_set_padding(GTK_MISC(pw_label), 2, 2);
    const char *s = strchr(string, '\n');
    if (s && *(s+1))
        gtk_label_set_justify(GTK_LABEL(pw_label), GTK_JUSTIFY_LEFT);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), pw_label);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 4, 2);

    // The dismiss button.
    //
    pw_cancel = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(pw_cancel, "Dismiss");
    gtk_widget_show(pw_cancel);
    g_signal_connect(G_OBJECT(pw_cancel), "clicked",
        G_CALLBACK(pw_message_popdown), this);
    gtk_window_set_focus(GTK_WINDOW(pw_shell), pw_cancel);
    gtk_table_attach(GTK_TABLE(form), pw_cancel, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_widget_set_size_request(pw_cancel, 150, -1);  // set min width
}


GTKmsgPopup::~GTKmsgPopup()
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (owner)
            owner->ClearPopup(this);
    }
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        GRX->Deselect(p_caller);
    g_signal_handlers_disconnect_by_func(G_OBJECT(pw_shell),
        (gpointer)pw_message_popdown, this);

    gtk_widget_destroy(pw_shell);
}


// GRpopup override
//
void
GTKmsgPopup::popdown()
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// Private static GTK signal handler.
// Pop down and destroy the message popup.
//
void
GTKmsgPopup::pw_message_popdown(GtkWidget*, void *client_data)
{
    GTKmsgPopup *p = static_cast<GTKmsgPopup*>(client_data);
    if (p)
        p->popdown();
}


// Private static GTK signal handler.
// Key press event handler.
//
int
GTKmsgPopup::pw_message_popdown_ev(GtkWidget*, GdkEvent*, void *client_data)
{
    GTKmsgPopup *p = static_cast<GTKmsgPopup*>(client_data);
    if (p)
        p->popdown();
    return (true);
}
// End of GTKmsgPopup functions


//-----------------------------------------------------------------------------
// Fancy message box (scroll bars, buttons, html, etc.)

#ifdef HAVE_MOZY

// Set up an anchor activate signal handler for the HTML info window.
// Anchor href text is sent to the help system.

struct gtkDataInterface : public htmDataInterface
{
    void emit_signal(SignalID id, void *payload)
    {
        switch (id) {
        case S_ARM:
            break;
        case S_ACTIVATE:
            {
                htmAnchorCallbackStruct *cbs =
                    static_cast<htmAnchorCallbackStruct*>(payload);
                if (cbs && cbs->href)
                    GRX->MainFrame()->PopUpHelp(cbs->href);
            }
            break;
        case S_ANCHOR_TRACK:
        case S_ANCHOR_VISITED:
        case S_DOCUMENT:
        case S_LINK:
        case S_FRAME:
        case S_FORM:
        case S_IMAGEMAP:
        case S_HTML_EVENT:
        default:
            break;
        }
    }

    void *event_proc(const char*)                           { return (0); }
    void panic_callback(const char*)                        { }
    htmImageInfo *image_resolve(const char *s)
        {
            if (!s || !if_viewer)
                return (0);
            return (HLP()->context()->imageResolve(s, if_viewer));
        }
    int get_image_data(htmPLCStream*, void*)                { return (0); }
    void end_image_data(htmPLCStream*, void*, int, bool)    { }
    void frame_rendering_area(htmRect*)                     { }
    const char *get_frame_name()                            { return (0); }
    void get_topic_keys(char**, char**)                     { }
    void scroll_visible(int, int, int, int)                 { }

    void set_viewer(ViewerWidget *v)                        { if_viewer = v; }
private:
    ViewerWidget *if_viewer;
};


// Background color used by HTML text display when not otherwise
// specified (in a <body> tab)
#define HTML_TEXT_BG "white"

namespace {
    // Return true if a <body> tag is found near the start of the text.
    bool has_body_tag(const char *str)
    {
        if (!str)
            return (false);
        const char *s = str;;
        int cnt = 5;
        while ((s = strchr(s, '<')) != 0) {
            s++;
            while (isspace(*s))
                s++;
            if (!strncasecmp(s, "body", 4) &&
                    (isspace(s[4]) || s[4] == '>')) {
                return (true);
            }
            cnt--;
            if (!cnt)
                break;
        }
        return (false);
    }
}

#endif

char *GTKtextPopup::pw_errlog = 0;

GTKtextPopup::GTKtextPopup(GTKbag *owner, const char *message_str,
    int which, STYtype sty, void *arg)
{
    p_parent = owner;
    p_cb_arg = arg;
    pw_which = which;
    pw_style = sty;
    pw_viewer = 0;
    pw_if = 0;
    pw_text = 0;
    pw_btn = 0;
    pw_upd_text = 0;
    pw_save_pop = 0;
    pw_msg_pop = 0;
    pw_idle_id = 0;
    pw_defer = true;

    // Pango will not print a string that is not valid UTF8, which can
    // happen if we get some non-ascii chars from somewhere.

    GCarray<char*> gc_tmpstr(0);
    if (lstring::is_utf8(message_str) != 0) {
        // NOT UTF8, filter out non-ascii chars.
        char *tmpstr = new char[strlen(message_str)];
        char *t = tmpstr;
        for (const char *s = message_str; *s; s++) {
            if (!(*s & 0x80))
                *t++ = *s;
        }
        *t = 0;
        gc_tmpstr.set(tmpstr);
        message_str = tmpstr;
    }

    if (owner)
        owner->MonitorAdd(this);

    if (pw_which == PuWarn)
        pw_shell = gtk_NewPopup(owner, "Warning", pw_text_popdown, this);
    else if (pw_which == PuErr || pw_which == PuErrAlso)
        pw_shell = gtk_NewPopup(owner, "ERROR", pw_text_popdown, this);
    else if (pw_which == PuInfo)
        pw_shell = gtk_NewPopup(owner, "Info", pw_text_popdown, this);
    else if (pw_which == PuInfo2)
        pw_shell = gtk_NewPopup(owner, "Info", pw_text_popdown, this);
    else if (pw_which == PuHTML) {
        pw_shell = gtk_NewPopup(owner, "Info", pw_text_popdown, this);
        gtk_window_set_wmclass(GTK_WINDOW(pw_shell), "Mozy", "mozy");
    }
    else
        pw_shell = gtk_NewPopup(owner, "ERROR", pw_text_popdown, this);
    g_object_set_data(G_OBJECT(pw_shell), "text_w", this);

    GtkWidget *form = gtk_table_new(1, 3, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(pw_shell), form);

    // Scrolled text area.
    //
    int width, height;
#ifdef HAVE_MOZY
    if (pw_style == STY_HTML) {

        width = 500;
        height = 300;
        pw_if = new gtkDataInterface;
        pw_viewer = new gtk_viewer(width, height, pw_if);
        pw_if->set_viewer(pw_viewer);

        GtkWidget *frame = gtk_frame_new(0);
        gtk_widget_show(frame);
        gtk_container_add(GTK_CONTAINER(frame), pw_viewer->top_widget());

        if (has_body_tag(message_str))
            pw_viewer->set_source(message_str);
        else {
            sLstr lstr;
            lstr.add("<body bgcolor=\"");
            lstr.add(HTML_TEXT_BG);
            lstr.add("\">");
            lstr.add(message_str);
            lstr.add("</body>");
            pw_viewer->set_source(lstr.string());
        }

        gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    }
#else
    if (false) { }
#endif
    else {
        GtkWidget *contr;
        text_scrollable_new(&contr, &pw_text,
            pw_style == STY_FIXED ? FNT_FIXED : FNT_PROP);
        if (pw_which == PuInfo || pw_which == PuInfo2)
            g_object_set_data(G_OBJECT(pw_text), "export", (void*)1);
        textbox(message_str, &width, &height);
        text_set_chars(pw_text, message_str);

        gtk_widget_add_events(pw_text, GDK_BUTTON_PRESS_MASK);
        g_signal_connect_after(G_OBJECT(pw_text), "realize",
            G_CALLBACK(text_realize_proc), this);

        gtk_table_attach(GTK_TABLE(form), contr, 0, 1, 0, 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
        gtk_widget_set_size_request(pw_text, width, height);
    }

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // Button(s).
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    if (pw_style != STY_HTML) {
        GtkWidget*button = gtk_toggle_button_new_with_label("Save Text ");
        gtk_widget_set_name(button, "Save");
        gtk_widget_show(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(pw_btn_hdlr), this);
        gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
    }
    if ((pw_which == PuErr || pw_which == PuErrAlso) &&
            pw_errlog && p_parent) {
        GtkWidget*button = gtk_button_new_with_label("Show Error Log");
        gtk_widget_set_name(button, "ErrLog");
        gtk_widget_show(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(pw_btn_hdlr), this);
        gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
    }
    if (pw_which == PuInfo2) {
        GtkWidget *button = gtk_button_new_with_label("Help");
        gtk_widget_set_name(button, "Help");
        gtk_widget_show(button);
        gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(pw_text_action), this);

        pw_btn = gtk_toggle_button_new_with_label("Activate");
        gtk_widget_set_name(pw_btn, "Activate");
        gtk_widget_show(pw_btn);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pw_btn), true);
        gtk_box_pack_start(GTK_BOX(hbox), pw_btn, true, true, 0);
        g_signal_connect(G_OBJECT(pw_btn), "clicked",
            G_CALLBACK(pw_text_action), this);
    }
    pw_cancel = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(pw_cancel, "Dismiss");
    gtk_widget_show(pw_cancel);
    g_signal_connect(G_OBJECT(pw_cancel), "clicked",
        G_CALLBACK(pw_text_popdown), this);

    gtk_box_pack_start(GTK_BOX(hbox), pw_cancel, true, true, 0);
    gtk_window_set_focus(GTK_WINDOW(pw_shell), pw_cancel);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
}


GTKtextPopup::~GTKtextPopup()
{
    if (pw_idle_id)
        g_source_remove(pw_idle_id);
    if (pw_btn && GRX->GetStatus(pw_btn))
        gtk_button_clicked(GTK_BUTTON(pw_btn));
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (owner)
            owner->ClearPopup(this);
    }
    if (pw_save_pop)
        pw_save_pop->popdown();
    if (pw_msg_pop)
        pw_msg_pop->popdown();
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        GRX->Deselect(p_caller);
    g_signal_handlers_disconnect_by_func(G_OBJECT(pw_shell),
        (gpointer)pw_text_popdown, this);

#ifdef HAVE_MOZY
    delete pw_if;
    delete pw_viewer;
#endif
    gtk_widget_destroy(pw_shell);
}


// GRpopup override
//
void
GTKtextPopup::popdown()
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// GRtextPopup override
//
bool
GTKtextPopup::get_btn2_state()
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return (false);
    }
    if (pw_btn)
        return (GRX->GetStatus(pw_btn));
    return (false);
}


// GRtextPopup override
//
void
GTKtextPopup::set_btn2_state(bool state)
{
    if (p_parent) {
        GTKbag *owner = dynamic_cast<GTKbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    if (pw_btn)
        GRX->SetStatus(pw_btn, state);
}


// Attempt to reuse an existing widget to show message_str.  If not
// possible, return false, otherwise return true;
//
bool
GTKtextPopup::update(const char *message_str)
{
    if (pw_style == STY_HTML) {
#ifdef HAVE_MOZY
        if (pw_viewer) {
            if (has_body_tag(message_str))
                pw_viewer->set_source(message_str);
            else {
                sLstr lstr;
                lstr.add("<body bgcolor=\"");
                lstr.add(HTML_TEXT_BG);
                lstr.add("\">");
                lstr.add(message_str);
                lstr.add("</body>");
                pw_viewer->set_source(lstr.string());
            }
            return (true);
        }
#endif
    }
    else if (pw_text && gtk_widget_get_window(pw_text)) {
        if (GTK_IS_TEXT_VIEW(pw_text)) {
            text_set_chars(pw_text, message_str);
            return (true);
        }
    }
    return (false);
}


// Return the height and width needed to display text, within limits.
// Return true if vertical scrolling needed.
//
bool
GTKtextPopup::textbox(const char *text, int *wd, int *ht)
{
    if (!text || !*text)
        text = "x";
    PangoLayout *pl = gtk_widget_create_pango_layout(pw_text, text);
    int x, y;
    pango_layout_get_pixel_size(pl, &x, &y);
    g_object_unref(pl);
    y /= 50;
    y *= 50;
    if (y < 100)
        y = 100;
    if (y > 300)
        y = 300;
    *wd = 450;
    *ht = y;
    return (false);  // scrolls when needed
}


// Private static GTK signal handler.
//
void
GTKtextPopup::pw_text_popdown(GtkWidget*, void *client_data)
{
    GTKtextPopup *p = static_cast<GTKtextPopup*>(client_data);
    if (p)
        p->popdown();
}


// Private static GTK signal handler.
//
void
GTKtextPopup::pw_text_action(GtkWidget *widget, void *client_data)
{
    GTKtextPopup *p = static_cast<GTKtextPopup*>(client_data);
    if (p && p->p_callback) {
        const char *nm = gtk_widget_get_name(widget);
        if (nm && !strcmp(nm, "Help")) {
            (*p->p_callback)(false, GRtextPopupHelp);
        }
        else {
            bool state = GRX->GetStatus(widget);
            if ((*p->p_callback)(state, p->p_cb_arg))
                p->popdown();
        }
    }
}


// Private static GTK signal handler.
// Handle Save Text and Show Error Log button presses.
//
void
GTKtextPopup::pw_btn_hdlr(GtkWidget *widget, void *arg)
{
    const char *name = gtk_widget_get_name(widget);
    if (!name)
        return;
    if (!strcmp(name, "Save")) {
        if (!GRX->GetStatus(widget))
            return;
        GTKtextPopup *txtp = (GTKtextPopup*)arg;
        if (txtp->pw_save_pop)
            return;
        txtp->pw_save_pop = new GTKledPopup(0,
            "Enter path to file for saved text:", "", 200, false, 0, arg);
        txtp->pw_save_pop->register_caller(widget, false, true);
        txtp->pw_save_pop->register_callback(&txtp->pw_save_cb);
        txtp->pw_save_pop->register_usrptr((void**)&txtp->pw_save_pop);

        gtk_window_set_transient_for(GTK_WINDOW(txtp->pw_save_pop->pw_shell),
            GTK_WINDOW(txtp->pw_shell));
        GRX->SetPopupLocation(GRloc(), txtp->pw_save_pop->pw_shell,
            txtp->pw_shell);
        txtp->pw_save_pop->set_visible(true);
    }
    else if (!strcmp(name, "ErrLog")) {
        GTKtextPopup *txtp = (GTKtextPopup*)arg;
        if (!txtp->p_parent || !txtp->pw_errlog)
            return;
        txtp->p_parent->PopUpFileBrowser(txtp->pw_errlog);
    }
}


// Private static handler.
// Callback for the save file name pop-up.
//
ESret
GTKtextPopup::pw_save_cb(const char *string, void *arg)
{
    GTKtextPopup *txtp = (GTKtextPopup*)arg;
    if (string) {
        if (!filestat::create_bak(string)) {
            txtp->pw_save_pop->update(
                "Error backing up existing file, try again", 0);
            return (ESTR_IGN);
        }
        FILE *fp = fopen(string, "w");
        if (!fp) {
            txtp->pw_save_pop->update("Error opening file, try again", 0);
            return (ESTR_IGN);
        }
        char *txt = text_get_chars(txtp->pw_text, 0, -1);
        if (txt) {
            unsigned int len = strlen(txt);
            if (len) {
                if (fwrite(txt, 1, len, fp) != len) {
                    txtp->pw_save_pop->update("Write failed, try again", 0);
                    delete [] txt;
                    fclose(fp);
                    return (ESTR_IGN);
                }
            }
            delete [] txt;
        }
        fclose(fp);

        if (txtp->pw_msg_pop)
            txtp->pw_msg_pop->popdown();
        txtp->pw_msg_pop = new GTKmsgPopup(0, "Text saved in file.", false);
        txtp->pw_msg_pop->register_usrptr((void**)&txtp->pw_msg_pop);
        gtk_window_set_transient_for(GTK_WINDOW(txtp->pw_msg_pop->pw_shell),
            GTK_WINDOW(txtp->pw_shell));
        GRX->SetPopupLocation(GRloc(), txtp->pw_msg_pop->pw_shell,
            txtp->pw_shell);
        txtp->pw_msg_pop->set_visible(true);
        g_timeout_add(2000, pw_timeout, txtp);
    }
    return (ESTR_DN);
}


int
GTKtextPopup::pw_timeout(void *arg)
{
    GTKtextPopup *txtp = (GTKtextPopup*)arg;
    if (txtp->pw_msg_pop)
        txtp->pw_msg_pop->popdown();
    return (0);
}


// Static function.
// Idle proc to actually update the text.
//
int
GTKtextPopup::pw_text_upd_idle(void *arg)
{
    GTKtextPopup *p = (GTKtextPopup*)arg;
    p->pw_defer = false;
    p->update(p->pw_upd_text);
    p->pw_defer = true;
    delete p->pw_upd_text;
    p->pw_upd_text = 0;
    p->pw_idle_id = 0;
    return (0);
}
// End of GTKtextPopup functions


//----------------------------------------------------------------------------
// GTKbag methods

GRaffirmPopup *
GTKbag::PopUpAffirm(GRobject caller, GRloc loc, const char *question_str,
    void (*action_callback)(bool, void*), void *action_arg)
{
    GTKaffirmPopup *p = new GTKaffirmPopup(this, question_str, action_arg);
    p->register_caller(caller, false, true);
    p->register_callback(action_callback);

    gtk_window_set_transient_for(GTK_WINDOW(p->pw_shell),
        GTK_WINDOW(wb_shell));
    GRX->SetPopupLocation(loc, p->pw_shell, PositionReferenceWidget());
    p->set_visible(true);
    return (p);
}


GRnumPopup *
GTKbag::PopUpNumeric(GRobject caller, GRloc loc,
    const char *prompt_str, double initd, double mind, double maxd, double del,
    int numd, void(*action_callback)(double, bool, void*), void *action_arg)
{
    GTKnumPopup *p = new GTKnumPopup(this, prompt_str, initd, mind, maxd,
        del, numd, action_arg);
    p->register_caller(caller, false, true);
    p->register_callback(action_callback);

    gtk_window_set_transient_for(GTK_WINDOW(p->pw_shell),
        GTK_WINDOW(wb_shell));
    GRX->SetPopupLocation(loc, p->pw_shell, PositionReferenceWidget());
    p->set_visible(true);
    return (p);
}


GRledPopup *
GTKbag::PopUpEditString(GRobject caller, GRloc loc,
    const char *prompt_str, const char *init_str,
    ESret (*action_callback)(const char *, void*),
    void *action_arg, int textwidth,
    void (*downproc)(bool), bool multiline, const char *btnstr)
{
    GTKledPopup *p = new GTKledPopup(this, prompt_str, init_str, textwidth,
        multiline, btnstr, action_arg);
    p->register_caller(caller, false, true);
    p->register_callback(action_callback);
    p->register_quit_callback(downproc);

    gtk_window_set_transient_for(GTK_WINDOW(p->pw_shell),
        GTK_WINDOW(wb_shell));
    GRX->SetPopupLocation(loc, p->pw_shell, PositionReferenceWidget());
    p->set_visible(true);
    return (p);
}


// PopUpInput and PopUpMessage implement an input solicitation
// widget with error reporting.  Only one of each widget is
// possible per parent GTKbag as they set fields in the
// GTKbag struct.
// char *prompt_str;     label string
// char *initial_str;    initial text
// char  *action_str;    action button string
// void (*action_callback)(char *string, void *arg)
// void *arg;            client_data for action_callback()
// int textwidth;        width of entry area
//
void
GTKbag::PopUpInput(const char *prompt_str, const char *initial_str,
    const char *action_str, void(*action_callback)(const char*, void*),
    void *arg, int textwidth)
{
    if (wb_input)
        wb_input->popdown();

    GTKledPopup *p = new GTKledPopup(this, prompt_str, initial_str, textwidth,
        false, action_str, arg);
    wb_input = p;

    p->register_void_callback(action_callback);

    gtk_window_set_transient_for(GTK_WINDOW(p->pw_shell),
        GTK_WINDOW(wb_shell));
    GRX->SetPopupLocation(GRloc(), p->pw_shell, PositionReferenceWidget());
    if (wb_sens_set)
        (*wb_sens_set)(this, false, 0);
    p->set_visible(true);
}


// Pop up a message box If err is true, the popup will get the
// resources of an error popup.  If multi is true, the widget will not
// use the GTKbag::message field, so that there can be arbitrarily
// many of these, but the user must keep track of them.  If desens is
// true, then any popup pointed to by GTKbag::wb_input is
// desensitized while the message is active.  This is done only if
// multi is false.
//
GRmsgPopup *
GTKbag::PopUpMessage(const char *string, bool err,
    bool desens, bool multi, GRloc loc)
{
    if (!multi && wb_message)
        wb_message->popdown();

    GTKmsgPopup *p = new GTKmsgPopup(this, string, err);
    if (!multi)
        wb_message = p;

    gtk_window_set_transient_for(GTK_WINDOW(p->pw_shell),
        GTK_WINDOW(wb_shell));
    GRX->SetPopupLocation(loc, p->pw_shell, PositionReferenceWidget());

    if (desens && !multi && wb_input) {
        p->set_desens();
        gtk_widget_set_sensitive(wb_input->pw_shell, false);
    }
    p->set_visible(true);
    return (multi ? p : 0);
}


//
// Error and Info Pop-ups
//

// The next few functions pop up and down error and info popups.  The
// shell and text widgets are stored in the GTKbag struct, so there
// can be only one of each type per GTKbag.  Additional calls to
// PopUpxxx() simply update the text.
//
// The functions return an integer index, which is incremented when the
// window is explicitly popped down.  Calling with mode == MODE_UPD also
// returns the index, and can be used to determine if the "same" window
// is still visible.

int
GTKbag::PopUpWarn(ShowMode mode, const char *message_str, STYtype sty,
    GRloc loc)
{
    if (sty == STY_HTML)
        return (-1);
    if (mode == MODE_OFF) {
        if (wb_warning)
            wb_warning->popdown();
        return (0);
    }
    if (mode == MODE_UPD) {
        if (wb_warning)
            return (wb_warn_cnt);
        return (0);
    }

    if (wb_warning) {
        if (wb_warning->update(message_str))
            return (wb_warn_cnt);
        wb_warning->popdown();
    }

    wb_warning = new GTKtextPopup(this, message_str, PuWarn, sty, 0);

    GRX->SetPopupLocation(loc, wb_warning->pw_shell, PositionReferenceWidget());
    gtk_window_set_transient_for(GTK_WINDOW(wb_warning->pw_shell),
        GTK_WINDOW(wb_shell));
    wb_warning->set_visible(true);

    return (wb_warn_cnt);
}


int
GTKbag::PopUpErr(ShowMode mode, const char *message_str, STYtype sty,
    GRloc loc)
{
    if (sty == STY_HTML)
        return (-1);
    if (mode == MODE_OFF) {
        if (wb_error)
            wb_error->popdown();
        return (0);
    }
    if (mode == MODE_UPD) {
        if (wb_error)
            return (wb_err_cnt);
        return (0);
    }

    if (wb_error) {
        if (wb_error->update(message_str))
            return (wb_err_cnt);
        wb_error->popdown();
    }

    wb_error = new GTKtextPopup(this, message_str, PuErr, sty, 0);

    GRX->SetPopupLocation(loc, wb_error->pw_shell, PositionReferenceWidget());
    gtk_window_set_transient_for(GTK_WINDOW(wb_error->pw_shell),
        GTK_WINDOW(wb_shell));
    wb_error->set_visible(true);

    return (wb_err_cnt);
}


// Pop up an error text window.  This is not linked to anything.
// Return the widget.
//
GRtextPopup *
GTKbag::PopUpErrText(const char *message_str, STYtype sty, GRloc loc)
{
    if (sty == STY_HTML)
        return (0);

    GTKtextPopup *p = new GTKtextPopup(this, message_str, PuErrAlso, sty, 0);

    gtk_window_set_transient_for(GTK_WINDOW(p->pw_shell),
        GTK_WINDOW(wb_shell));
    GRX->SetPopupLocation(loc, p->pw_shell, PositionReferenceWidget());
    p->set_visible(true);

    return (p);
}


int
GTKbag::PopUpInfo(ShowMode mode, const char *message_str, STYtype sty,
    GRloc loc)
{
    if (sty == STY_HTML)
        return (-1);
    if (mode == MODE_OFF) {
        if (wb_info)
            wb_info->popdown();
        return (0);
    }
    if (mode == MODE_UPD) {
        if (wb_info)
            return (wb_info_cnt);
        return (0);
    }

    if (wb_info) {
        if (wb_info->update(message_str))
            return (wb_info_cnt);
        wb_info->popdown();
    }

    wb_info = new GTKtextPopup(this, message_str, PuInfo, sty, 0);

    GRX->SetPopupLocation(loc, wb_info->pw_shell, PositionReferenceWidget());
    gtk_window_set_transient_for(GTK_WINDOW(wb_info->pw_shell),
        GTK_WINDOW(wb_shell));
    wb_info->set_visible(true);

    return (wb_info_cnt);
}


int
GTKbag::PopUpInfo2(ShowMode mode, const char *message_str,
    bool(*cb)(bool, void*), void *arg, STYtype sty, GRloc loc)
{
    if (sty == STY_HTML)
        return (-1);
    if (mode == MODE_OFF) {
        if (wb_info2)
            wb_info2->popdown();
        return (0);
    }
    if (mode == MODE_UPD) {
        if (wb_info2)
            return (wb_info2_cnt);
        return (0);
    }

    if (wb_info2) {
        if (wb_info2->update(message_str))
            return (wb_info2_cnt);
        wb_info2->popdown();
    }

    wb_info2 = new GTKtextPopup(this, message_str, PuInfo2, sty, arg);
    wb_info2->register_callback(cb);

    GRX->SetPopupLocation(loc, wb_info2->pw_shell, PositionReferenceWidget());
    gtk_window_set_transient_for(GTK_WINDOW(wb_info2->pw_shell),
        GTK_WINDOW(wb_shell));
    wb_info2->set_visible(true);

    return (wb_info2_cnt);
}


int
GTKbag::PopUpHTMLinfo(ShowMode mode, const char *message_str, GRloc loc)
{
    if (mode == MODE_OFF) {
        if (wb_htinfo)
            wb_htinfo->popdown();
        return (0);
    }
    if (mode == MODE_UPD) {
        if (wb_htinfo)
            return (wb_htinfo_cnt);
        return (0);
    }

    if (wb_htinfo) {
        if (wb_htinfo->update(message_str))
            return (wb_htinfo_cnt);
        wb_htinfo->popdown();
    }

    wb_htinfo = new GTKtextPopup(this, message_str, PuHTML, STY_HTML, 0);

    gtk_window_set_transient_for(GTK_WINDOW(wb_htinfo->pw_shell),
        GTK_WINDOW(wb_shell));
    GRX->SetPopupLocation(loc, wb_htinfo->pw_shell, PositionReferenceWidget());
    wb_htinfo->set_visible(true);

    return (wb_htinfo_cnt);
}


GRledPopup *GTKbag::ActiveInput()       { return (wb_input); }
GRmsgPopup *GTKbag::ActiveMessage()     { return (wb_message); }
GRtextPopup *GTKbag::ActiveInfo()       { return (wb_info); }
GRtextPopup *GTKbag::ActiveInfo2()      { return (wb_info2); }
GRtextPopup *GTKbag::ActiveHtinfo()     { return (wb_htinfo); }
GRtextPopup *GTKbag::ActiveWarn()       { return (wb_warning); }
GRtextPopup *GTKbag::ActiveError()      { return (wb_error); }
GRfontPopup *GTKbag::ActiveFontsel()    { return (wb_fontsel); }

void
GTKbag::SetErrorLogName(const char *fname)
{
    GTKtextPopup::set_error_log(fname);
}

#ifndef HAVE_MOZY

// Resolve help pop-up when mozy is not included.
//
bool
GTKbag::PopUpHelp(const char*)
{
    PopUpErr(MODE_ON, "Help system is not available in this executable.");
    return (false);
}

#endif
// End of GTKbag functions.


//-----------------------------------------------------------------------------
// Some Misc. Utilities
//-----------------------------------------------------------------------------

// Using GTKdev::SetDoubleClickExit, there is a *big problem* with
// spin buttons.  Button 2 is used to page-advance, but since the spin
// button (or adjustment?) propagates the button 2 press, clicking
// button 2 twice will destroy the widget tree while the spin button
// is being adjusted, which is likely to cause a program fault.
//
// The following function creates an event box that swallows the
// button press events, which is returned.  This should be placed
// in the widget hierarchy, instead of the spin button (argument).
//
GtkWidget *
gtkinterf::DblClickSpinBtnContainer(GtkWidget *spinbtn)
{
    // Need to grab and throw away button press events from the
    // spin button.
    GtkWidget *ebox = gtk_event_box_new();
    gtk_widget_show(ebox);
    gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(ebox), "button-press-event",
        G_CALLBACK(dc_spinbtn_hdlr), 0);
    gtk_container_add(GTK_CONTAINER(ebox), spinbtn);
    return (ebox);
}


namespace {
    void
    bh_proc()
    {
        gdk_flush();
        gtk_DoEvents(100);
    }
}


// When a widget is popped down and the application begins a long
// operation immediately, the screen is not updated until the
// operation completes, leaving a "black hole" for the duration.
// This sets a delete handler that should do the redraw.
//
// For this to work, the widget must be hidden or destroyed
// *before* the operation is started or put in the idle queue.
//
void
gtkinterf::BlackHoleFix(GtkWidget *widg)
{
    g_signal_connect(G_OBJECT(widg), "destroy",
        G_CALLBACK(bh_proc), 0);
    g_signal_connect(G_OBJECT(widg), "unmap",
        G_CALLBACK(bh_proc), 0);
}


// Returns the position and size of the widget, and the decorated widget
// top-level parent.
//
bool
gtkinterf::ShellGeometry(GtkWidget *widget, GdkRectangle *widget_box,
    GdkRectangle *wmparent_box)
{
    if (widget_box) {
        if (!gtk_widget_get_window(widget)) {
            widget_box->x = 0;
            widget_box->y = 0;
            widget_box->width = 0;
            widget_box->height = 0;
        }
        else {
            int x, y;
            gdk_window_get_origin(gtk_widget_get_window(widget), &x, &y);
            widget_box->x = x;
            widget_box->y = y;
            GdkWindow *win = gtk_widget_get_window(widget);
            widget_box->width = gdk_window_get_width(win);
            widget_box->height = gdk_window_get_height(win);
        }
    }
    if (wmparent_box) {
        if (!gtk_widget_get_window(widget)) {
            wmparent_box->x = 0;
            wmparent_box->y = 0;
            wmparent_box->width = 0;
            wmparent_box->height = 0;
        }
        else {
            gdk_window_get_frame_extents(gtk_widget_get_window(widget),
                wmparent_box);
        }
    }
    return (true);
}


// This is a Btn1Down handler for moving popups.  It is attached to the
// popup shell, and allows the application to move the popup anywhere
// on screen.  Activation is by pointing at the form area of the popup
// with button 1, and dragging.

namespace {
    GdkCursor *B1hand_cursor;
    GdkRectangle B1box;
    GdkRectangle B1cf;
#ifdef NEW_GC
    ndkGC *B1gc;
#else
    GdkGC *B1gc;
#endif

//XXX
// The box-drawing doesn't show in either gdk or cairo versions.  Maybe
// drawing on the root window is not allowed in OSX and elsewhere?

    inline GdkWindow *
    gr_default_root_window()
    {
        return (gdk_get_default_root_window());
    }


    int
    B1motion(GtkWidget *caller, GdkEvent *event, void*)
    {
        if (gtk_widget_get_window(caller) != event->motion.window)
            return (false);
#ifdef NEW_GC
#else
        gdk_draw_rectangle(gr_default_root_window(), B1gc, false,
            B1cf.x, B1cf.y, B1box.width, B1box.height);
        B1cf.x = (int)event->motion.x_root - B1box.x;
        B1cf.y = (int)event->motion.y_root - B1box.y;
        gdk_draw_rectangle(gr_default_root_window(), B1gc, false,
            B1cf.x, B1cf.y, B1box.width, B1box.height);
#endif
        return (true);
    }


    int
    B1button(GtkWidget *caller, GdkEvent *event, void*)
    {
        if (gtk_widget_get_window(caller) != event->button.window)
            return (false);
        if (event->button.button == 1) {
            gdk_pointer_ungrab(GDK_CURRENT_TIME);
#ifdef NEW_GC
#else
            gdk_draw_rectangle(gr_default_root_window(), B1gc, false,
                B1cf.x, B1cf.y, B1box.width, B1box.height);
#endif
            B1cf.x = (int)event->button.x_root - B1box.x;
            B1cf.y = (int)event->button.y_root - B1box.y;

            gdk_window_move(gtk_widget_get_window(caller), B1cf.x + B1cf.width,
                B1cf.y + B1cf.width);
            g_signal_handlers_disconnect_by_func(G_OBJECT(caller),
                (gpointer)B1motion, 0);
            g_signal_handlers_disconnect_by_func(G_OBJECT(caller),
                (gpointer)B1button, 0);
        }
        return (true);
    }
}


int
gtkinterf::Btn1MoveHdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (event->button.button != 1)
        return (false);
    if (gtk_widget_get_window(caller) != event->button.window)
        return (false);
    if (event->type != GDK_BUTTON_PRESS)
        return (false);
    if (!B1hand_cursor) {
        // first call, create cursor and GC
#ifdef NEW_GC
        B1hand_cursor = gdk_cursor_new(GDK_HAND2);
        B1gc = new ndkGC(gr_default_root_window(), 0, (ndkGCvaluesMask)0);
        B1gc->set_subwindow(ndkGC_INCLUDE_INFERIORS);

        GdkColor wp;
        gdk_color_parse("white", &wp);
        gdk_colormap_alloc_color(GRX->Colormap(), &wp, false, true);
        GdkColor bp;
        gdk_color_parse("black", &bp);
        gdk_colormap_alloc_color(GRX->Colormap(), &bp, false, true);
        GdkColor clr;
        clr.pixel = wp.pixel ^ bp.pixel;
        gtk_QueryColor(&clr);
        B1gc->set_foreground(&clr);
        B1gc->set_function(ndkGC_XOR);
#else
        B1hand_cursor = gdk_cursor_new(GDK_HAND2);
        B1gc = gdk_gc_new(gr_default_root_window());
        gdk_gc_set_subwindow(B1gc, GDK_INCLUDE_INFERIORS);

        GdkColor wp;
        gdk_color_white(GRX->Colormap(), &wp);
        GdkColor bp;
        gdk_color_black(GRX->Colormap(), &bp);
        GdkColor clr;
        clr.pixel = wp.pixel ^ bp.pixel;
        gtk_QueryColor(&clr);
        gdk_gc_set_foreground(B1gc, &clr);
        gdk_gc_set_function(B1gc, GDK_XOR);
#endif
    }

    GdkRectangle shell_box, parent_box;
    if (!ShellGeometry(caller, &shell_box, &parent_box))
        return (true);
    B1box.x = (int)event->button.x_root - parent_box.x;
    B1box.y = (int)event->button.y_root - parent_box.y;
    B1box.width = parent_box.width;
    B1box.height = parent_box.height;
    B1cf.x = parent_box.x;
    B1cf.y = parent_box.y;
    B1cf.width = shell_box.x - parent_box.x;
    B1cf.height = shell_box.y - parent_box.y;

#ifdef NEW_GC
    Drawable xid = gdk_x11_drawable_get_xid(gr_default_root_window());
    int x1 = (int)event->button.x_root - B1box.x;
    int y1 = (int)event->button.y_root - B1box.y;
    int x2 = x1 + B1box.width;
    int y2 = y1 + B1box.height;
    XDrawLine(B1gc->get_xdisplay(), xid, B1gc->get_xgc(), x1, y1, x2, y1);
    XDrawLine(B1gc->get_xdisplay(), xid, B1gc->get_xgc(), x2, y1, x2, y2);
    XDrawLine(B1gc->get_xdisplay(), xid, B1gc->get_xgc(), x2, y2, x1, y2);
    XDrawLine(B1gc->get_xdisplay(), xid, B1gc->get_xgc(), x1, y2, x1, y1);
#else
    gdk_draw_rectangle(gr_default_root_window(), B1gc, false,
        (int)event->button.x_root - B1box.x,
        (int)event->button.y_root - B1box.y, B1box.width, B1box.height);
#endif

    gtk_widget_add_events(caller, GDK_BUTTON1_MOTION_MASK);
    g_signal_connect(G_OBJECT(caller), "motion-notify-event",
        G_CALLBACK(B1motion), 0);
    gtk_widget_add_events(caller, GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(G_OBJECT(caller), "button-release-event",
        G_CALLBACK(B1button), 0);

    gdk_pointer_grab(gtk_widget_get_window(caller), false,
        (GdkEventMask)(GDK_POINTER_MOTION_MASK | GDK_BUTTON_MOTION_MASK |
        GDK_BUTTON_RELEASE_MASK), 0, B1hand_cursor, GDK_CURRENT_TIME);
    return (true);
}


// Add this callback to popups that should always be visible.
//
int
gtkinterf::ToTop(GtkWidget *caller, GdkEvent *event, void *client_data)
{
#ifdef WITH_X11
    // Warning, dragons here.  The parent widget (overshell) might have
    // just been deleted, and the pointer is trash.

    if (GRX->IsNoToTop())
        return (true);

    // Have to avoid a competition problem if there are identical
    // widgets at the same location
    static GdkRectangle tt_last;
    static time_t tt_ltime;

    GdkEventVisibility *vev = (GdkEventVisibility*)event;
    if (vev->state == GDK_VISIBILITY_FULLY_OBSCURED) {

        XWindowChanges xv;
        GtkWidget *overshell = (GtkWidget*)client_data;
        if (!overshell) {
            overshell = (GtkWidget*)g_object_get_data(G_OBJECT(caller),
                "parent");
        }
        if (overshell && gtk_widget_is_drawable(overshell) &&
                gr_x_window(gtk_widget_get_window(overshell)) !=
                GRX->BigWindowXid()) {

            GdkRectangle r;
            ShellGeometry(caller, 0, &r);
            time_t t;
            time(&t);
            if (abs(r.x - tt_last.x) <= 2 && abs(r.y - tt_last.y) <= 2 &&
                    abs(r.width - tt_last.width) <= 2 &&
                    abs(r.height - tt_last.height) <= 2 &&
                    (t - tt_ltime <= 1)) {
                tt_last = r;
                tt_ltime = t;
                return (true);
            }
            xv.sibling = gr_x_window(gtk_widget_get_window(overshell));
            xv.stack_mode = Above;
            XReconfigureWMWindow(gr_x_display(),
                gr_x_window(vev->window),
                DefaultScreen(gr_x_display()), CWSibling|CWStackMode, &xv);
            tt_last = r;
            tt_ltime = t;
            return (true);
        }
        if (GRX->BigWindowXid()) {

            // It used to be possible to keep plot windows from
            // WRspice on top of the main Xic window, by raising them
            // above the Xic window when fully obscured.  This doesn't
            // work anymore with most current window managers.

            // This would keep plot windows above Xic, but even when
            // there is only partial overlap, which is no help.  It
            // also produces BadMatch errors, need to understand and
            // fix to use this.
            // XSetTransientForHint(gr_x_display(), gr_x_window(vev->window),
            //     GRX->BigWindowXid());

            // This fixed some bug, in ancient OLWM (IIRC) where
            // multiple windows would interact causing a continuous
            // raising/lowering.
            //
            GdkRectangle r;
            ShellGeometry(caller, 0, &r);
            time_t t;
            time(&t);
            if (abs(r.x - tt_last.x) <= 2 && abs(r.y - tt_last.y) <= 2 &&
                    abs(r.width - tt_last.width) <= 2 &&
                    abs(r.height - tt_last.height) <= 2 &&
                    (t - tt_ltime <= 1)) {
                tt_last = r;
                tt_ltime = t;
                return (true);
            }

            // Most window managers apparently ignore this.
            xv.sibling = GRX->BigWindowXid();
            xv.stack_mode = Above;
            XReconfigureWMWindow(gr_x_display(), gr_x_window(vev->window),
                DefaultScreen(gr_x_display()), CWSibling|CWStackMode, &xv);

            tt_last = r;
            tt_ltime = t;

            // Below is code to implement the "new" window manager
            // protocol.  However, it doesn't work either, at least
            // with my current IceWM and whatever is used on
            // RHEL5/gnome.  Keep it commented for now.
            /****
            XEvent xev;
            xev.xclient.type = ClientMessage;
            xev.xclient.display = gr_x_display();
            xev.xclient.serial = 0;
            xev.xclient.send_event = True;
            xev.xclient.window = gr_x_window(vev->window);
            xev.xclient.message_type =
                gr_x_atom_intern("_NET_RESTACK_WINDOW", false);
            xev.xclient.format = 32;
            xev.xclient.data.l[0] = 2;
            xev.xclient.data.l[1] = GRX->BigWindowXid();
            xev.xclient.data.l[2] = Above;
            xev.xclient.data.l[3] = 0;
            xev.xclient.data.l[4] = 0;

            gdk_error_trap_push();

            Window root = gr_x_window(gr_default_root_window());
            XSendEvent(gr_x_display(), root, False,
                SubstructureRedirectMask | SubstructureNotifyMask, &xev);

            gdk_flush();
            gdk_error_trap_pop();
            ****/

            return (true);
        }
    }
#else
    (void)caller;
    (void)event;
    (void)client_data;
#endif
    return (true);
}


// Return true if the widget is currently iconic.
//
bool
gtkinterf::IsIconic(GtkWidget *w)
{
    return (gdk_window_get_state(gtk_widget_get_window(w)) &
        GDK_WINDOW_STATE_ICONIFIED);
}


// Return the monitor number and the geometry.  The widget is used to
// obtain the screen, if null the default screen is used.
//
int
gtkinterf::MonitorGeom(GtkWidget *w, int *px, int *py, int *pw, int *ph)
{
    GdkScreen *scrn = w ? gtk_widget_get_screen(w) : gdk_screen_get_default();
    int pmon;
    if (w && GDK_IS_WINDOW(gtk_widget_get_window(w)))
        pmon = gdk_screen_get_monitor_at_window(scrn, gtk_widget_get_window(w));
    else {
        int x, y;
        GRX->PointerRootLoc(&x, &y);
        pmon = gdk_screen_get_monitor_at_point(scrn, x, y);
    }
    GdkRectangle r;
    gdk_screen_get_monitor_geometry(scrn, pmon, &r);
    if (px)
        *px = r.x;
    if (py)
        *py = r.y;
    if (pw)
        *pw = r.width;
    if (ph)
        *ph = r.height;
    return (pmon);
}


// The following functions enable a double-click to cancel feature for
// pop-up widgets.  The widget arg is usually the shell, the cancel arg
// is the cancel button.

namespace {
    // This handler is called by button presses.  It prepares to double-click
    // exit, or calls the exit callback.
    //
    // data set:
    // caller           "pirate"            1
    //
    int
    dc_btn_hdlr(GtkWidget *caller, GdkEvent *event, void *client_data)
    {
        GtkWidget *cancel = (GtkWidget*)client_data;
        static GdkCursor *cursor;
        if (event->button.button == 2) {
            int dc_state =
                (intptr_t)g_object_get_data(G_OBJECT(caller), "pirate");
            if (dc_state) {
                g_object_set_data(G_OBJECT(caller), "pirate", (void*)0);
                if (GTK_IS_BUTTON(cancel))
                    gtk_button_clicked(GTK_BUTTON(cancel));
                else if (GTK_IS_MENU_ITEM(cancel))
                    gtk_menu_item_activate(GTK_MENU_ITEM(cancel));
            }
            else {
                if (!cursor)
                    cursor = gdk_cursor_new(GDK_PIRATE);
                gdk_window_set_cursor(gtk_widget_get_window(caller), cursor);
                g_object_set_data(G_OBJECT(caller), "pirate", (void*)1);
            }
            return (true);
        }
        return (false);
    }


    // On leaving, exit double click exit mode.
    //
    int
    dc_leave_hdlr(GtkWidget *caller, GdkEvent *event, void*)
    {
        int dc_state = (intptr_t)g_object_get_data(G_OBJECT(caller),
            "pirate");
        if (dc_state && event->crossing.mode == GDK_CROSSING_NORMAL) {
            g_object_set_data(G_OBJECT(caller), "pirate", (void*)0);
            gdk_window_set_cursor(gtk_widget_get_window(caller), 0);
            return (true);
        }
        return (false);
    }


    int
    dc_spinbtn_hdlr(GtkWidget*, GdkEvent*, void*)
    {
        return (true);
    }
}


//=========================================================================
//
// A collection of helper functions to simplify migration from gtk1 to
// gtk2.
//

// Create a new button with an XPM image if xpm is not null.  The old
// code mostly still works, except on Apple/Quartz, where the images
// are munged (looks bad on CentOS 6.4 Gnome desktop, too).
//
GtkWidget *
gtkinterf::new_pixmap_button(const char **xpm, const char *text, bool toggle)
{
    GtkWidget *button = 0;
    if (xpm) {
        button = toggle ? gtk_toggle_button_new() : gtk_button_new();
        GdkPixbuf *pb = gdk_pixbuf_new_from_xpm_data(xpm);
        GtkWidget *img = gtk_image_new_from_pixbuf(pb);
        g_object_unref(pb);
        gtk_widget_show(img);
        gtk_container_add(GTK_CONTAINER(button), img);
    }
    else if (text) 
        button = toggle ? gtk_toggle_button_new_with_label(text) :
            gtk_button_new_with_label(text);
    return (button);
}


//  ---  This group applies to text widgets (GtkText and GtkTextView).

namespace {
    bool
    tv_drag_motion(GtkWidget*, GdkDragContext*, int, int, unsigned int, void*)
    {
        return (true);
    }
}


// Create a new scrollable text window, reeturning the container and
// the text widget.  The third argument is a font index (FNT_???) to
// use for rendering, as per gtkfont.cc.
//
void
gtkinterf::text_scrollable_new(GtkWidget **container, GtkWidget **textp,
    int font_indx)
{
    GtkWidget *swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkWidget *text = gtk_text_view_new();
    gtk_widget_show(text);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_NONE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 4);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), false);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), false);
    gtk_container_add(GTK_CONTAINER(swin), text);

    // This prevents the normal drag motion handling, which is to move
    // the cursor and possibly scroll the text.  It is also needed to
    // make non-editable text views work as drop receivers.  The
    // handler simply returns 1.
    g_signal_connect(G_OBJECT(text), "drag-motion",
        G_CALLBACK(tv_drag_motion), 0);

    // This prevents drag_data_received from being emitted multiple
    // times.
    g_signal_connect(G_OBJECT(text), "drag-drop",
        G_CALLBACK(tv_drag_motion), 0);

    // This prevents the text view from processing a drop with its own
    // handler, which will insert the text.
    GtkTextViewClass *cls = GTK_TEXT_VIEW_GET_CLASS(text);
    GtkWidgetClass *wcls = GTK_WIDGET_CLASS(cls);
    wcls->drag_data_received = 0;

    *container = swin;
    *textp = text;
    if (font_indx > 0)
        GTKfont::setupFont(text, font_indx, true);
}


// Set the text in the text window, discarding any previous content.
//
void
gtkinterf::text_set_chars(GtkWidget *widget, const char *s)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    gtk_text_buffer_set_text(tbf, s, -1);
}


// Insert nc chars from string into the text window at the given
// position.  If nc is -1, string must be null terminated and will be
// inserted in its entirety.  If pos is -1, insertion will start at
// the end of existing text.
//
void
gtkinterf::text_insert_chars_at_point(GtkWidget *widget, GdkColor *clr,
    const char *string, int nc, int pos)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter istart;
    gtk_text_buffer_get_iter_at_offset(tbf, &istart, pos);
    pos = gtk_text_iter_get_offset(&istart);  // actual offset, not -1
    gtk_text_buffer_insert(tbf, &istart, string, nc);
    if (clr) {
        gtk_text_buffer_get_iter_at_offset(tbf, &istart, pos);
        int len = strlen(string);
        if (nc < 0 || nc > len)
            nc = len;
        GtkTextIter iend;
        gtk_text_buffer_get_iter_at_offset(tbf, &iend, pos + nc);
        GtkTextTag *tag = gtk_text_buffer_create_tag(tbf, 0,
            "foreground-gdk", clr, NULL);
        gtk_text_buffer_apply_tag(tbf, tag, &istart, &iend);
    }
}


// Return the cursor offset into text.
//
int
gtkinterf::text_get_insertion_point(GtkWidget *widget)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextMark *mark = gtk_text_buffer_get_insert(tbf);
    GtkTextIter ipos;
    gtk_text_buffer_get_iter_at_mark(tbf, &ipos, mark);
    return (gtk_text_iter_get_offset(&ipos));
}


// Set the cursor offset into text, -1 indicates end.
//
void
gtkinterf::text_set_insertion_point(GtkWidget *widget, int pos)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter ipos;
    gtk_text_buffer_get_iter_at_offset(tbf, &ipos, pos);
    gtk_text_buffer_place_cursor(tbf, &ipos);
}


// Delete the characters from start to end, -1 indicates end of text.
//
void
gtkinterf::text_delete_chars(GtkWidget *widget, int start, int end)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter istart, iend;
    gtk_text_buffer_get_iter_at_offset(tbf, &istart, start);
    gtk_text_buffer_get_iter_at_offset(tbf, &iend, end);
    gtk_text_buffer_delete(tbf, &istart, &iend);
}


// Replace the chars from start to end with the same number of chars
// from string, -1 indicates end of text.
//
void
gtkinterf::text_replace_chars(GtkWidget *widget, GdkColor *clr,
    const char *string, int start, int end)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter istart, iend;
    gtk_text_buffer_get_iter_at_offset(tbf, &istart, start);
    gtk_text_buffer_get_iter_at_offset(tbf, &iend, end);
    end = gtk_text_iter_get_offset(&iend);  // actual offset, not -1
    gtk_text_buffer_delete(tbf, &istart, &iend);
    gtk_text_buffer_get_iter_at_offset(tbf, &istart, start);
    gtk_text_buffer_insert(tbf, &istart, string, end - start);
    if (clr) {
        int nc = end - start;
        int len = strlen(string);
        if (len < nc)
            nc = len;
        gtk_text_buffer_get_iter_at_offset(tbf, &istart, start);
        gtk_text_buffer_get_iter_at_offset(tbf, &iend, start + nc);
        GtkTextTag *tag = gtk_text_buffer_create_tag(tbf, 0,
            "foreground-gdk", clr, NULL);
        gtk_text_buffer_apply_tag(tbf, tag, &istart, &iend);
    }
}


// Return the chars from start to end, -1 indicates end of text.
//
char *
gtkinterf::text_get_chars(GtkWidget *widget, int start, int end)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter istart, iend;
    gtk_text_buffer_get_iter_at_offset(tbf, &istart, start);
    gtk_text_buffer_get_iter_at_offset(tbf, &iend, end);
    char *s = lstring::tocpp(gtk_text_iter_get_text(&istart, &iend));
    if (!s)
        s = lstring::copy("");
    return (s);
}


// Select the text from start to end, -1 indicates end of text.
//
void
gtkinterf::text_select_range(GtkWidget *widget, int start, int end)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter istart, iend;
    gtk_text_buffer_get_iter_at_offset(tbf, &istart, start);
    gtk_text_buffer_get_iter_at_offset(tbf, &iend, end);
    gtk_text_buffer_select_range(tbf, &istart, &iend);
}


// Return true if the text window has a selection.
//
bool
gtkinterf::text_has_selection(GtkWidget *widget)
{
    if (!widget)
        return (false);
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter istart, iend;
    gtk_text_buffer_get_selection_bounds(tbf, &istart, &iend);
    int start = gtk_text_iter_get_offset(&istart);
    int end = gtk_text_iter_get_offset(&iend);
    return (start != end);
}


// Return the selected text.
//
char *
gtkinterf::text_get_selection(GtkWidget *widget)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter istart, iend;
    gtk_text_buffer_get_selection_bounds(tbf, &istart, &iend);
    int start = gtk_text_iter_get_offset(&istart);
    int end = gtk_text_iter_get_offset(&iend);
    if (start != end)
        return (lstring::tocpp(gtk_text_iter_get_text(&istart, &iend)));
    return (0);
}


// Return the offsets of selected text.
//
void
gtkinterf::text_get_selection_pos(GtkWidget *widget, int *startp, int *endp)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter istart, iend;
    gtk_text_buffer_get_selection_bounds(tbf, &istart, &iend);
    *startp = gtk_text_iter_get_offset(&istart);
    *endp = gtk_text_iter_get_offset(&iend);
}


// Return the current scroll position.
//
double
gtkinterf::text_get_scroll_value(GtkWidget *widget)
{
    GtkAdjustment *va = gtk_text_view_get_vadjustment(GTK_TEXT_VIEW(widget));
    return (gtk_adjustment_get_value(va));
    
}


// Change the val to keep line visible.
//
double
gtkinterf::text_get_scroll_to_line_value(GtkWidget *widget, int line,
    double val)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter inewpos;
    gtk_text_buffer_get_iter_at_line(tbf, &inewpos, line);
    GdkRectangle rect;
    gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(widget), &rect);
    int ly, lht;
    gtk_text_view_get_line_yrange(GTK_TEXT_VIEW(widget), &inewpos,
        &ly, &lht);
    if (ly < rect.y + 2 || ly + lht > rect.y + rect.height - 2) {
        int x, y;
        gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(widget),
        GTK_TEXT_WINDOW_WIDGET, 0, ly, &x, &y);
        val = y;
    }
    return (val);
}


// Set the scroll position.
//
void
gtkinterf::text_set_scroll_value(GtkWidget *widget, double val)
{
    GtkAdjustment *va = gtk_text_view_get_vadjustment(GTK_TEXT_VIEW(widget));
    gtk_adjustment_set_value(va, val);
    g_signal_emit_by_name(G_OBJECT(va), "value_changed");
}


// Scroll the window to make pos (character offset, -1 indicates end)
// visible.
//
void
gtkinterf::text_scroll_to_pos(GtkWidget *widget, int pos)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter ipos;
    gtk_text_buffer_get_iter_at_offset(tbf, &ipos, pos);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(widget), &ipos, .05, false,
        0, 0);
}


// Set/unset editable flag for text window.
//
void
gtkinterf::text_set_editable(GtkWidget *widget, bool able)
{
    gtk_text_view_set_editable(GTK_TEXT_VIEW(widget), able);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(widget), able);
}


// Get the number of characters in the text.
//
int
gtkinterf::text_get_length(GtkWidget *widget)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter iend;
    gtk_text_buffer_get_end_iter(tbf, &iend);
    return (gtk_text_iter_get_offset(&iend));
}


// Cut selected text to the clipboard.
//
void
gtkinterf::text_cut_clipboard(GtkWidget *widget)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkClipboard *cb = gtk_clipboard_get_for_display(
        gdk_display_get_default(), GDK_SELECTION_CLIPBOARD);
    gtk_text_buffer_cut_clipboard(tbf, cb, true);
}


// Copy selected text to the clipboard.
//
void
gtkinterf::text_copy_clipboard(GtkWidget *widget)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkClipboard *cb = gtk_clipboard_get_for_display(
        gdk_display_get_default(), GDK_SELECTION_CLIPBOARD);
    gtk_text_buffer_copy_clipboard(tbf, cb);
}


// Paste clipboard contents into text window.
//
void
gtkinterf::text_paste_clipboard(GtkWidget *widget)
{
    GtkTextBuffer *tbf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkClipboard *cb = gtk_clipboard_get_for_display(
        gdk_display_get_default(), GDK_SELECTION_CLIPBOARD);
    gtk_text_buffer_paste_clipboard(tbf, cb, 0, true);
}


// Connect to or disconnect from the "changed" signal.
//
void
gtkinterf::text_set_change_hdlr(GtkWidget *widget,
    void(*change_proc)(GtkWidget*, void*), void *arg, bool active)
{
    if (active) {
        GtkTextBuffer *tbf =
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
        g_signal_connect_after(G_OBJECT(tbf), "changed",
            (GCallback)change_proc, arg);
    }
    else {
        GtkTextBuffer *tbf =
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
        g_signal_handlers_disconnect_by_func(G_OBJECT(tbf),
            (void*)(GCallback)change_proc, arg);
    }
}


// Set an arrow cursor instead of the default I-beam.
//
void
gtkinterf::text_realize_proc(GtkWidget *w, void*)
{
    GdkWindow *wd = gtk_text_view_get_window(GTK_TEXT_VIEW(w),
        GTK_TEXT_WINDOW_TEXT);
    if (wd) {
        GdkCursor *c = gdk_cursor_new(GDK_TOP_LEFT_ARROW);
        gdk_window_set_cursor(wd, c);
        gdk_cursor_unref(c);
    }
}



/****************/
// Some translation code, based in GDK-2 source.

#ifdef WITH_X11

// Replacement for gdk_draw_drawable.
void
gtkinterf::copy_x11_pixmap_to_drawable(GdkDrawable *drawable, void *gcp,
    GdkPixmap *src, int xsrc, int ysrc, int xdest, int ydest,
    int width, int height)
{
    // Work around an Xserver bug where non-visible areas from a
    // pixmap to a window will clear the window background in
    // destination areas that are supposed to be clipped out. 
    // This is a problem with client side windows as this means
    // things may draw outside the virtual windows.  This could
    // also happen for window to window copies, but I don't think
    // we generate any calls like that.
    //
    // See: 
    // http://lists.freedesktop.org/archives/xorg/2009-February/043318.html
    //

    if (GDK_IS_WINDOW(drawable)) {
        if (xsrc < 0) {
            width += xsrc;
            xdest -= xsrc;
            xsrc = 0;
        }
      
        if (ysrc < 0) {
            height += ysrc;
            ydest -= ysrc;
            ysrc = 0;
        }

        int swid, shei;
        gdk_pixmap_get_size(src, &swid, &shei);
        if (xsrc + width > swid)
            width = swid - xsrc;
        if (ysrc + height > shei)
            height = shei - ysrc;
    }
  
    int src_depth = gdk_drawable_get_depth(src);
    if (src_depth == 1) {
#ifdef NEW_GC
        ndkGC *gc = (ndkGC*)gcp;
        XCopyArea(gc->get_xdisplay(), gdk_x11_drawable_get_xid(src),
            gdk_x11_drawable_get_xid(drawable), gc->get_xgc(), xsrc, ysrc,
            width, height, xdest, ydest);
#else
        GdkGC *gc = (GdkGC*)gcp;
        XCopyArea(gr_x_display(), gdk_x11_drawable_get_xid(src),
            gdk_x11_drawable_get_xid(drawable), gdk_x11_gc_get_xgc(gc),
            xsrc, ysrc, width, height, xdest, ydest);
#endif
        return;
    }
    int dest_depth = gdk_drawable_get_depth(drawable);
    if (dest_depth != 0 && src_depth == dest_depth) {
#ifdef NEW_GC
        ndkGC *gc = (ndkGC*)gcp;
        XCopyArea(gc->get_xdisplay(), gdk_x11_drawable_get_xid(src),
            gdk_x11_drawable_get_xid(drawable), gc->get_xgc(), xsrc, ysrc,
            width, height, xdest, ydest);
#else
        GdkGC *gc = (GdkGC*)gcp;
        XCopyArea(gr_x_display(), gdk_x11_drawable_get_xid(src),
            gdk_x11_drawable_get_xid(drawable), gdk_x11_gc_get_xgc(gc),
            xsrc, ysrc, width, height, xdest, ydest);
#endif
    }
    else
        g_warning(
    "Attempt to draw a drawable with depth %d to a drawable with depth %d",
           src_depth, dest_depth);
}
#endif

