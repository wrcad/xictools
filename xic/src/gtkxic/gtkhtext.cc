
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: gtkhtext.cc,v 5.145 2017/04/12 05:02:53 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "editif.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "gtkmain.h"
#include "gtkhtext.h"
#include "gtkfont.h"
#include "gtkinlines.h"
#include "events.h"
#include "keymap.h"
#include "menu.h"
#include "select.h"
#include "errorlog.h"

#include <gdk/gdkkeysyms.h>


//-----------------------------------------------------------------------------
// Prompt line hypertext editor
//
// Help system keywords used:
//  keyspresd

namespace {
    GtkTargetEntry pl_targets[] = {
        { (char*)"TWOSTRING",  0, 0 },
        { (char*)"CELLNAME",   0, 1 },
        { (char*)"property",   0, 2 },
        { (char*)"STRING",     0, 3 },
        { (char*)"text/plain", 0, 4 },
    };
    guint n_pl_targets = sizeof(pl_targets) / sizeof(pl_targets[0]);

    // The UTF8_STRING atom was discovered by looking at GTK source.
    // STRING doesn't work for UTF-8 when target is a KDE Konsole
    // window.

    GtkTargetEntry sel_targets[] = {
        { (char*)"UTF8_STRING",0, 0 },
        { (char*)"text/plain", 0, 1 }
    };
    guint n_sel_targets = sizeof(sel_targets) / sizeof(sel_targets[0]);
}

hyList *GTKedit::pe_stores[PE_NUMSTORES];

GTKedit *GTKedit::instancePtr = 0;


GTKedit::GTKedit(bool nogr)
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class GTKedit already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    pe_disabled = nogr;

    pe_container = 0;
    pe_keys = 0;
    pe_keyframe = 0;
    pe_r_button = 0;
    pe_s_button = 0;
    pe_l_button = 0;
    pe_r_menu = 0;
    pe_s_menu = 0;
    pe_id = 0;
    pe_wid = pe_hei = 0;
    pe_pixmap = 0;
    if (nogr)
        return;

    pe_container = gtk_hbox_new(false, 0);
    gtk_widget_show(pe_container);

    // key press display
    pe_keys = gtk_label_new("");
    gtk_widget_show(pe_keys);
    gtk_label_set_justify(GTK_LABEL(pe_keys), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(pe_keys), 0, 0.5);
    GtkWidget *ebox = gtk_event_box_new();
    gtk_widget_show(ebox);
    gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(ebox), "button-press-event",
        GTK_SIGNAL_FUNC(pe_keys_hdlr), 0);
    gtk_container_add(GTK_CONTAINER(ebox), pe_keys);

    pe_keyframe = gtk_frame_new(0);
    gtk_widget_show(pe_keyframe);
    gtk_container_add(GTK_CONTAINER(pe_keyframe), ebox);
    gtk_box_pack_start(GTK_BOX(pe_container), pe_keyframe, false, false, 0);

    // Set up font and tracking.
    GTKfont::setupFont(pe_keys, FNT_FIXED, true);

    // R (recall) button
    char buf[8];
    pe_r_menu = gtk_menu_new();
    gtk_widget_set_name(pe_r_menu, "Rmenu");
    for (int i = 0; i < PE_NUMSTORES; i++) {
        sprintf(buf, "%d", i);
        GtkWidget *mi = gtk_menu_item_new_with_label(buf);
        gtk_widget_set_name(mi, buf);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(pe_r_menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(pe_r_menu_proc), this);
    }
    pe_r_button = gtk_button_new_with_label("R");
    gtk_widget_set_usize(pe_r_button, -1, 20);
    gtk_widget_hide(pe_r_button);
    gtk_widget_set_name(pe_r_button, "Recall");
    GtkTooltips *tt = gtk_NewTooltip();
    gtk_tooltips_set_tip(tt, pe_r_button,
        "Recall edit string from a register.", "");
    gtk_signal_connect(GTK_OBJECT(pe_r_button), "button-press-event",
        GTK_SIGNAL_FUNC(pe_popup_btn_proc), this);
    gtk_box_pack_start(GTK_BOX(pe_container), pe_r_button, false, false, 0);

    // S (save) button
    pe_s_menu = gtk_menu_new();
    gtk_widget_set_name(pe_s_menu, "Smenu");
    for (int i = 1; i < PE_NUMSTORES; i++) {
        sprintf(buf, "%d", i);
        GtkWidget *mi = gtk_menu_item_new_with_label(buf);
        gtk_widget_set_name(mi, buf);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(pe_s_menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(pe_s_menu_proc), this);
    }
    pe_s_button = gtk_button_new_with_label("S");
    gtk_widget_set_usize(pe_s_button, -1, 20);
    gtk_widget_hide(pe_s_button);
    gtk_widget_set_name(pe_s_button, "Save");
    tt = gtk_NewTooltip();
    gtk_tooltips_set_tip(tt, pe_s_button,
        "Save edit string to a register.", "");
    gtk_signal_connect(GTK_OBJECT(pe_s_button), "button-press-event",
        GTK_SIGNAL_FUNC(pe_popup_btn_proc), this);
    gtk_box_pack_start(GTK_BOX(pe_container), pe_s_button, false, false, 0);

    // L (long text) button
    pe_l_button = gtk_button_new_with_label("L");
    gtk_widget_set_usize(pe_l_button, -1, 20);
    gtk_widget_hide(pe_l_button);
    gtk_widget_set_name(pe_l_button, "LongText");
    tt = gtk_NewTooltip();
    gtk_tooltips_set_tip(tt, pe_l_button,
        "Associate a block of text with the label - pop up an editor.", "");
    gtk_signal_connect(GTK_OBJECT(pe_l_button), "clicked",
        GTK_SIGNAL_FUNC(pe_l_btn_hdlr), this);
    gtk_box_pack_start(GTK_BOX(pe_container), pe_l_button, false, false, 0);

    // the prompt line
    gd_viewport = gtk_drawing_area_new();
    gtk_widget_set_name(gd_viewport, "PromptLine");
    gtk_widget_show(gd_viewport);

    GTKfont::setupFont(gd_viewport, FNT_SCREEN, true);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), gd_viewport);
    gtk_box_pack_start(GTK_BOX(pe_container), frame, true, true, 0);

    gtk_widget_add_events(gd_viewport, GDK_EXPOSURE_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "expose-event",
        GTK_SIGNAL_FUNC(pe_redraw_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "button-press-event",
        GTK_SIGNAL_FUNC(pe_btn_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_BUTTON_RELEASE_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "button-release-event",
        GTK_SIGNAL_FUNC(pe_btn_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_POINTER_MOTION_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "motion-notify-event",
        GTK_SIGNAL_FUNC(pe_motion_hdlr), 0);

    gtk_widget_add_events(gd_viewport, GDK_ENTER_NOTIFY_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "enter-notify-event",
        GTK_SIGNAL_FUNC(pe_enter_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_LEAVE_NOTIFY_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "leave-notify-event",
        GTK_SIGNAL_FUNC(pe_leave_hdlr), this);

    gtk_signal_connect(GTK_OBJECT(gd_viewport), "selection-received",
        GTK_SIGNAL_FUNC(pe_selection_proc), this);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "style-set",
        GTK_SIGNAL_FUNC(pe_font_change_hdlr), this);

    // prompt line drop site
    gtk_drag_dest_set(frame, GTK_DEST_DEFAULT_ALL, pl_targets,
        n_pl_targets, GDK_ACTION_COPY);
    gtk_signal_connect_after(GTK_OBJECT(frame), "drag-data-received",
        GTK_SIGNAL_FUNC(pe_drag_data_received), this);

    // selections
    gtk_selection_add_targets(gd_viewport, GDK_SELECTION_PRIMARY, sel_targets,
        n_sel_targets);
#ifndef WIN32
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "selection-clear-event",
        GTK_SIGNAL_FUNC(pe_selection_clear), 0);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "selection-get",
        GTK_SIGNAL_FUNC(pe_selection_get), 0);
#endif

    // Set sizes
    GtkRequisition r;
    gtk_widget_size_request(pe_r_button, &r);
    int height = GTKfont::stringHeight(gd_viewport, 0) + 4;
    if (height < r.height)
        height = r.height;
    int prm_wid = 800;
    int keys_wid = GTKfont::stringWidth(pe_keys, "INPUT") + 6;
    GtkAllocation a;
    a.height = 0;
    a.width = keys_wid;
    a.x = 0;
    a.y = 0;
    gtk_widget_size_allocate(pe_keys, &a);
    gtk_drawing_area_size(GTK_DRAWING_AREA(gd_viewport), prm_wid, height);
}


namespace {
    int fm_timeout(void *arg)
    {
        if (arg)
            gtk_widget_destroy(GTK_WIDGET(arg));
        return (0);
    }
}


// Flash a message just above the prompt line for a couple of seconds.
//
void
GTKedit::flash_msg(const char *msg, ...)
{
    va_list args;
    GtkWidget *popup = gtk_window_new(GTK_WINDOW_POPUP);
    if (!popup)
        return;

    char buf[256];
    va_start(args, msg);
    vsnprintf(buf, 256, msg, args);
    va_end(args);

    GtkWidget *label = gtk_label_new(buf);
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_container_add(GTK_CONTAINER(popup), label);

    GRX->SetPopupLocation(GRloc(LW_LL), popup, mainBag()->Viewport());
    gtk_window_set_transient_for(GTK_WINDOW(popup),
        GTK_WINDOW(mainBag()->Shell()));

    gtk_widget_show(popup);

    GRX->AddTimer(2000, fm_timeout, popup);
}


// As above, but user passes the location.
//
void
GTKedit::flash_msg_here(int x, int y, const char *msg, ...)
{
    va_list args;
    GtkWidget *popup = gtk_window_new(GTK_WINDOW_POPUP);
    if (!popup)
        return;

    char buf[256];
    va_start(args, msg);
    vsnprintf(buf, 256, msg, args);
    va_end(args);

    GtkWidget *label = gtk_label_new(buf);
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_container_add(GTK_CONTAINER(popup), label);

    int mwid, mhei;
    MonitorGeom(mainBag()->Shell(), 0, 0, &mwid, &mhei);
    if (x + popup->requisition.width > mwid)
        x = mwid - popup->requisition.width;
    if (y + popup->requisition.height > mhei)
        y = mhei - popup->requisition.height;
    gtk_widget_set_uposition(popup, x, y);
    gtk_window_set_transient_for(GTK_WINDOW(popup),
        GTK_WINDOW(mainBag()->Shell()));

    gtk_widget_show(popup);

    GRX->AddTimer(2000, fm_timeout, popup);
}


// Save text in register 0, called when editing finished.
//
void
GTKedit::save_line()
{
    pe_stores[0]->free();
    pe_stores[0] = get_hyList(false);
}


int
GTKedit::win_width(bool in_chars)
{
    if (!GRX || !mainBag())
        return (in_chars ? 80 : 800);
    if (in_chars) {
        int cw, ch;
        GTKfont::stringBounds(FNT_SCREEN, 0, &cw, &ch);
        if (!cw)
            return (0);
        return ((pe_wid - 4)/cw);
    }
    return (pe_wid);
}


// Set the keyboard focus to the main drawing window.
//
void
GTKedit::set_focus()
{
    if (mainBag()) {
        GRX->SetFocus(mainBag()->Shell());
        gtk_window_set_focus(GTK_WINDOW(mainBag()->Shell()),
            mainBag()->Viewport());
    }
}


// Display the R/S/L buttons, hide the keys area while editing.
//
void
GTKedit::set_indicate()
{
    if (pe_indicating) {
        gtk_widget_hide(pe_keyframe);
        gtk_widget_show(pe_r_button);
        gtk_widget_show(pe_s_button);
        if (pe_long_text_mode)
            gtk_widget_show(pe_l_button);
    }
    else {
        gtk_widget_show(pe_keyframe);
        gtk_widget_hide(pe_r_button);
        gtk_widget_hide(pe_s_button);
        gtk_widget_hide(pe_l_button);
    }
}


void
GTKedit::show_lt_button(bool show)
{
    if (!pe_disabled)
        gtk_widget_set_sensitive(pe_l_button, show);
}


void
GTKedit::get_selection(bool clipb)
{
    if (clipb)
        gtk_selection_convert(gd_viewport, gdk_atom_intern("CLIPBOARD", false),
            GDK_TARGET_STRING, GDK_CURRENT_TIME);
    else
        gtk_selection_convert(gd_viewport, GDK_SELECTION_PRIMARY,
            GDK_TARGET_STRING, GDK_CURRENT_TIME);
}


void *
GTKedit::setup_backing(bool clear)
{
    GdkWindow *tmp_window = 0;
    if (pe_pixmap && clear) {
        tmp_window = gd_window;
        gd_window = pe_pixmap;
    }
    return (tmp_window);
}


void
GTKedit::restore_backing(void *tw)
{
    GdkWindow *tmp_window = (GdkWindow*)tw;
    if (tmp_window) {
        gdk_window_copy_area(tmp_window, GC(), 0, 0, pe_pixmap,
            0, 0, pe_wid, pe_hei);
        gd_window = tmp_window;
    }
}


void
GTKedit::init_window()
{
    if (!gd_window)
        gd_window = gd_viewport->window;
    if (gd_window) {
        SetWindowBackground(bg_pixel());
        Clear();
    }
}


// If this returns true, use an explicit backing pixmap in redraw.
//
bool
GTKedit::check_pixmap()
{
    if (!gd_window)
        gd_window = gd_viewport->window;
    int w, h;
    gdk_window_get_size(gd_window, &w, &h);
    if (!pe_pixmap || w != pe_wid || h != pe_hei) {
        if (pe_pixmap)
            gdk_pixmap_unref(pe_pixmap);
        pe_pixmap = gdk_pixmap_new(gd_window, w, h, GRX->Visual()->depth);
        pe_wid = w;
        pe_hei = h;
    }
    return (true);
}


#ifdef WIN32
namespace {
    void primary_get_cb(GtkClipboard*, GtkSelectionData *data,
        unsigned int, void*)
    {
        if (gtkEdit()) {
            char *str = gtkEdit()->get_sel();
            if (str) {
                gtk_selection_data_set_text(data, str, -1);
                delete [] str;
            }
        }
    }


    void primary_clear_cb(GtkClipboard*, void*)
    {
        if (gtkEdit())
            gtkEdit()->deselect(true);
    }

}
#endif


void
GTKedit::init_selection(bool selected)
{
    if (selected) {
#ifdef WIN32
        // For some reaon the owner_set code doesn't work in
        // Windows.

        if (!ptr())
            return;
        char *str = ptr()->get_sel();
        if (str) {
            GtkClipboard *cb = gtk_clipboard_get_for_display(
                gdk_display_get_default(), GDK_SELECTION_PRIMARY);
            gtk_clipboard_set_with_owner(cb, sel_targets, n_sel_targets,
                primary_get_cb, primary_clear_cb, G_OBJECT(Viewport()));
            delete [] str;
        }
#else
        gtk_selection_owner_set(Viewport(), GDK_SELECTION_PRIMARY,
            GDK_CURRENT_TIME);
#endif
    }
    else
        gtk_selection_owner_set(0, GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
}


namespace {
    // Move the mouse pointer into the prompt line area.  When
    // editing, the arrow keys will move the text carat rather than
    // pan the main drawing window.
    //
    int warp_ptr(void *arg)
    {
        GTKedit *ed = (GTKedit*)arg;
        int x, y;
        gdk_window_get_origin(ed->Viewport()->window, &x, &y);
        GdkDisplay *display = gdk_display_get_default();
        GdkScreen *screen = gdk_display_get_default_screen(display);
        int h;
        ed->TextExtent(0, 0, &h);
        int xx = ed->xpos() - 4;
        if (xx < 0)
            xx = 0;

        gdk_display_warp_pointer(display, screen, x + xx, y + h/2 + 2);
        // Note: This does not send motion/enter/leave events in Quartz,
        // which is a butt-ugly.

        return (0);
    }
}


void
GTKedit::warp_pointer()
{
    // The pointer move must be in an idle proc, so it runs after
    // prompt line reconfiguration.

    gtk_idle_add(warp_ptr, this);
}


// Static function.
// Callback for 'L' (long text) button
//
void
GTKedit::pe_l_btn_hdlr(GtkWidget *w, void*, void*)
{
    if (ptr() && w == ptr()->pe_l_button)
        ptr()->lt_btn_press_handler();
}


namespace {
    // Positioning function for pop-up menus.
    //
    void
    pos_func(GtkMenu*, int *x, int *y, gboolean *pushin, void *data)
    {
        *pushin = true;
        GtkWidget *btn = GTK_WIDGET(data);
        GRX->Location(btn, x, y);
    }
}


// Static function.
// Button-press handler to produce pop-up menu.
//
int
GTKedit::pe_popup_btn_proc(GtkWidget *w, GdkEvent *event, void*)
{
    if (ptr() && w == ptr()->pe_r_button)
        gtk_menu_popup(GTK_MENU(ptr()->pe_r_menu), 0, 0, pos_func, w,
            event->button.button, event->button.time);
    else if (ptr() && w == ptr()->pe_s_button)
        gtk_menu_popup(GTK_MENU(ptr()->pe_s_menu), 0, 0, pos_func, w,
            event->button.button, event->button.time);
    gtk_window_set_focus(GTK_WINDOW(mainBag()->Shell()),
        mainBag()->Viewport());
    return (true);
}


// Static function.
void
GTKedit::pe_r_menu_proc(GtkWidget *w, void*)
{
    const char *name = gtk_widget_get_name(w);
    if (!name)
        return;
    while (*name && !isdigit(*name))
        name++;
    if (!isdigit(*name))
        return;
    int ix = atoi(name);
    if (ix >= PE_NUMSTORES)
        return;

    gtk_window_set_focus(GTK_WINDOW(mainBag()->Shell()),
        mainBag()->Viewport());
    for (hyList *hl = pe_stores[ix]; hl; hl = hl->next()) {
        if (hl->ref_type() == HLrefText || hl->ref_type() == HLrefLongText ||
                hl->ref_type() == HLrefEnd)
            continue;
        const hyEnt *ent = hl->hent();
        if (!ent) {
            ptr()->flash_msg("Can't recall, mising element.");
            return;
        }
        if (!ent->owner()) {
            ptr()->flash_msg("Can't recall, bad address.");
            return;
        }
        if (ent->owner() != CurCell(true)) {
            ptr()->flash_msg("Can't recall, incompatible reference.");
            return;
        }
    }

    ptr()->clear_cols_to_end(ptr()->pe_colmin);
    ptr()->insert(pe_stores[ix]);
}


// Static function.
void
GTKedit::pe_s_menu_proc(GtkWidget *w, void*)
{
    const char *name = gtk_widget_get_name(w);
    if (!name)
        return;
    while (*name && !isdigit(*name))
        name++;
    if (!isdigit(*name))
        return;
    int ix = atoi(name);
    if (ix < 1 || ix >= PE_NUMSTORES)
        return;
    pe_stores[ix]->free();
    pe_stores[ix] = ptr()->get_hyList(false);

    ptr()->flash_msg("Edit line saved in register %d.", ix);
    gtk_window_set_focus(GTK_WINDOW(mainBag()->Shell()),
        mainBag()->Viewport());
}


// Static function.
// Pop up info about the keys pressed area in help mode.
//
int
GTKedit::pe_keys_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    if (XM()->IsDoingHelp() && event->type == GDK_BUTTON_PRESS &&
            !is_shift_down())
        DSPmainWbag(PopUpHelp("keyspresd"))
    return (true);
}


#ifdef WITH_QUARTZ

// Static function.
// Redraw callback.
//
int
GTKedit::pe_redraw_hdlr(GtkWidget*, GdkEvent*, void*)
{
    if (ptr())
        ptr()->redraw();
    return (true);
}

#else

// Static function.
// Redraw idle proc, Windows needs this or the cursor disappears when
// the main window is resized.  Doesn't work with Quartz.
//
int
GTKedit::pe_redraw_idle(void *arg)
{
    GTKedit *e = (GTKedit*)arg;
    e->redraw();
    e->pe_id = 0;
    return (0);
}


// Static function.
// Redraw callback.
//
int
GTKedit::pe_redraw_hdlr(GtkWidget*, GdkEvent*, void*)
{
    if (ptr()) {
        if (ptr()->pe_id)
            gtk_idle_remove(ptr()->pe_id);
        ptr()->pe_id = gtk_idle_add(pe_redraw_idle, ptr());
    }
    return (true);
}

#endif


// Static function.
// Handle button press/release.
//
int
GTKedit::pe_btn_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    if (ptr()) {
        if (event->type == GDK_2BUTTON_PRESS) {
            // Double-clicking in the prompt area with button 1 sends
            // Enter.

            if (event->button.button == 1) {
                ptr()->finish(false);
                return (true);
            }
            return (false);
        }

        if (event->type == GDK_BUTTON_PRESS) {
            ptr()->button_press_handler(event->button.button,
                (int)event->button.x, (int)event->button.y);
            return (true);
        }
        if (event->type == GDK_BUTTON_RELEASE) {
            ptr()->button_release_handler(event->button.button,
                (int)event->button.x, (int)event->button.y);
            return (true);
        }
    }
    return (false);
}


int
GTKedit::pe_enter_hdlr(GtkWidget*, GdkEvent*, void*)
{
    if (ptr()) {
        ptr()->pe_entered = true;
        ptr()->redraw();
        return (true);
    }
    return (false);
}


int
GTKedit::pe_leave_hdlr(GtkWidget*, GdkEvent*, void*)
{
    if (ptr()) {
        ptr()->pe_entered = false;
        ptr()->redraw();
        return (true);
    }
    return (false);
}


namespace {
    // Copy the string, encoding unicode excapes to UTF-8 characters.
    //
    char *uni_decode(const char *str)
    {
        sLstr lstr;
        if (str) {
            for (const char *s = str; *s; s++) {
                sUni uc;
                if (*s == '\\') {
                    if (*(s+1) == 'u') {
                        // This should be followed by exactly 4 hex digits.
                        const char *t = s+2;
                        if (uc.addc(t[0]) && uc.addc(t[1]) && uc.addc(t[2]) &&
                                uc.addc(t[3])) {
                            lstr.add(uc.utf8_encode());
                            s += 5;
                            continue;
                        }
                    }
                    if (*(s+1) == 'U') {
                        // This should be followed by exactly 8 hex digits.
                        const char *t = s+2;
                        if (uc.addc(t[0]) && uc.addc(t[1]) && uc.addc(t[2]) &&
                                uc.addc(t[3]) && uc.addc(t[4]) && uc.addc(t[5]) &&
                                uc.addc(t[6]) && uc.addc(t[7])) {
                            lstr.add(uc.utf8_encode());
                            s += 9;
                            continue;
                        }
                    }
                }
                lstr.add_c(*s);
            }
        }
        return (lstr.string_trim());
    }
}


// Static function.
// Selection handler, supports hypertext transfer
//
void
GTKedit::pe_selection_proc(GtkWidget*, GtkSelectionData *sdata, guint, void*)
{
    if (sdata->length < 0) {
        ptr()->draw_cursor(DRAW);
        return;
    }
    if (sdata->type != GDK_SELECTION_TYPE_STRING) {
        Log()->ErrorLog(mh::Internal,
            "Selection conversion failed. not string data.");
        ptr()->draw_cursor(DRAW);
        return;
    }

    hyList *hpl = 0;
    GdkWindow *wnd = gdk_selection_owner_get(GDK_SELECTION_PRIMARY);
    if (wnd) {
        GtkWidget *widget;
        gdk_window_get_user_data(wnd, (void**)&widget);
        if (widget) {
            int code = (long)gtk_object_get_data(GTK_OBJECT(widget),
                "hyexport");
            if (code) {
                int start = GTK_OLD_EDITABLE(widget)->selection_start_pos;
                // The text is coming from the Property Editor or
                // Property Info pop-up, fetch the original
                // hypertext to insert.
                CDo *odesc;
                Ptxt *p = EditIf()->PropertyResolve(code, start, &odesc);
                if (p && p->prpty()) {
                    CDs *cursd = CurCell(true);
                    if (cursd)
                        hpl = cursd->hyPrpList(odesc, p->prpty());
                }
            }
        }
    }
    if (!hpl) {
        // Might not be 0-terminated?
        char *s = new char[sdata->length + 1];
        memcpy(s, sdata->data, sdata->length);
        s[sdata->length] = 0;
        char *d = uni_decode(s);
        delete [] s;
        ptr()->insert(d);
        delete [] d;
    }
    else {
        ptr()->insert(hpl);
        hpl->free();
    }
}


// Static function.
void
GTKedit::pe_font_change_hdlr(GtkWidget*, void*, void*)
{
    if (ptr()) {
        if (ptr()->gd_viewport) {
            int fw, fh;
            ptr()->TextExtent(0, &fw, &fh);
            GtkRequisition r;
            gtk_widget_size_request(ptr()->pe_r_button, &r);
            int ht = fh + 4;
            if (ht < r.height)
                ht = r.height;
            gtk_drawing_area_size(GTK_DRAWING_AREA(ptr()->gd_viewport), -1,
                ht);
        }
        ptr()->init();
    }
}


// Static function.
// Drag data received in main window - open the cell
//
void
GTKedit::pe_drag_data_received(GtkWidget*, GdkDragContext *context,
    gint, gint, GtkSelectionData *data, guint, guint time, void*)
{
    bool success = false;
    if (!ptr() || !data->data)
        ;
    else if (data->target == gdk_atom_intern("property", true)) {
        if (ptr()->is_active()) {
            unsigned char *val = data->data + sizeof(int);
            CDs *cursd = CurCell(true);
            hyList *hp = new hyList(cursd, (char*)val, HYcvAscii);
            ptr()->insert(hp);
            hp->free();
            success = true;
        }
    }
    else {
        if (data->length >= 0 && data->format == 8) {
            char *src = (char*)data->data;
            char *t = 0;
            if (data->target == gdk_atom_intern("TWOSTRING", true)) {
                // Drops from content lists may be in the form
                // "fname_or_chd\ncellname".
                t = strchr(src, '\n');
            }
            if (ptr()->is_active()) {
                // Keep the cellname.
                if (t)
                    src = t+1;
                // If editing, push into prompt line.
                ptr()->insert(src);
            }
            else {
                if (t) {
                    *t++ = 0;
                    XM()->Load(EV()->CurrentWin(), src, 0, t);
                }
                else
                    XM()->Load(EV()->CurrentWin(), src);
            }
            success = true;
        }
    }
    gtk_drag_finish(context, success, false, time);
}


// Static functions.
// Pointer motion handler.
//
int
GTKedit::pe_motion_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    if (!ptr())
        return (false);
    if (ptr()->pe_has_drag) {
        ptr()->pointer_motion_handler(
            (int)event->motion.x, (int)event->motion.y);
        return (true);
    }   
    return (false);
}


// Static function.
// Selection clear handler.
//
int
GTKedit::pe_selection_clear(GtkWidget*, GdkEventSelection*, void*)
{
    if (ptr())
        ptr()->deselect(true);
    return (true);
}


void
GTKedit::pe_selection_get(GtkWidget*, GtkSelectionData *selection_data,
    guint, guint, void*)
{
    if (selection_data->selection != GDK_SELECTION_PRIMARY)
        return;
    if (!ptr())
        return;
        
    unsigned char *s = (unsigned char*)ptr()->get_sel();
    if (!s)
        return;  // refuse
    int length = strlen((const char*)s);

    // The UTF8_STRING atom was discovered by looking at GTK source.
    // GDK_SELECTION_TYPE_STRING doesn't work for UTF-8 when target
    // is a GTK window.

    gtk_selection_data_set(selection_data,
        gdk_atom_intern_static_string("UTF8_STRING"), 8*sizeof(char),
        s, length);
    delete [] s;
}

