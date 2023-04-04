
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

// This enables use of pango font rendering in GTK2, must be defined
// ahead of pango includes.
#define PANGO_ENABLE_BACKEND

#include "gtkinterf/gtkinterf.h"
#include "gtkviewer.h"
#include "gtkinterf/gtkfile.h"
#include "gtkinterf/gtkfont.h"
#include "help/help_startup.h"
#include "htm/htm_font.h"
#include "htm/htm_image.h"
#include "htm/htm_form.h"
#include "htm/htm_format.h"
#include "miscutil/pathlist.h"
#include "miscutil/randval.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <gdk/gdkprivate.h>
#include <gdk/gdkkeysyms.h>

#define NEW_OPT
#define NEW_LIS

// When set, we paint the entire area of the HTML page on a pixmap,
// and copy from this when scrolling.  This is to provide smooth
// scrolling for form widgets in GTK-3, which otherwise seems
// impossible since the GtkFixed will always change size when a form
// widget child is shown or hidden, making a mess of the display. 
// Unfortunately this method doesn't work either, as the
// GtkScrolledWindow refuses to change its idea of the size of the
// scrollable area after this is changed.  If works great is this size
// is set initially by a known value (before the value is actually
// known).
//
//#define NEW_SC

//
// HTML viewer component for the help viewer.  This contains the
// htmWidget, scroll bars, and drawing area.  The v_form (GtkTable)
// field is the top-level widget.
//

namespace {
    enum
    {
        TARGET_STRING,
        TARGET_TEXT,
        TARGET_COMPOUND_TEXT
    };

    const GtkTargetEntry targets[] =
    {
        { (char*)"STRING", 0, TARGET_STRING },
        { (char*)"text/plain", 0, TARGET_STRING },
        { (char*)"TEXT",   0, TARGET_TEXT },
        { (char*)"COMPOUND_TEXT", 0, TARGET_COMPOUND_TEXT }
    };
    const gint n_targets = sizeof(targets) / sizeof(targets[0]);
}


namespace {
    // Silence the warnings from Pango when a font isn't found.  These
    // can be numerous and annoying.

    void pango_warning_hdlr(const char*, GLogLevelFlags, const char *msg,
        void*)
    {
        if (lstring::prefix("couldn't load font", msg))
            return;

        printf("Pango-WARNING: %s\n", msg);
    }
}


gtk_viewer::gtk_viewer(int wid, int hei, htmDataInterface *d) :
    htmWidget(this, d)
{
    v_form = 0;
    v_fixed = 0;
#ifdef NEW_SC
    v_scrwin = 0;
#endif
    v_draw_area = 0;
    v_hsba = 0;
    v_vsba = 0;
    v_hsb = 0;
    v_vsb = 0;
    v_pixmap = 0;
    v_pixmap_bak = 0;
    v_gc = 0;
    v_font = 0;

    v_transact = 0;
    v_cursor = 0;
    v_timers = 0;
    v_width = wid;
    v_height = hei;
#ifdef NEW_SC
    v_da_width = 0;
    v_da_height = 0;
#endif
    v_timer_id_cnt = 1000;
    v_btn_x = 0;
    v_btn_y = 0;
    v_last_x = 0;
    v_last_y = 0;
    v_text_yoff = 0;
    v_hpolicy = GTK_POLICY_AUTOMATIC;
    v_vpolicy = GTK_POLICY_AUTOMATIC;
    v_btn_dn = false;
    v_seen_expose = false;
    v_iso8859 = false;

    v_form = gtk_table_new(2, 2, false);
    gtk_widget_show(v_form);

    // Use a GtkFixed container to support placement of the form
    // widgets.
    // The GtkLayout seems like it should be perfect for the job,
    // however the placed widgets were never visible.  Never found
    // the problem, reverted to GtkFixed + GtkDrawingArea.`
    v_fixed = gtk_fixed_new();
    gtk_widget_show(v_fixed);
    // Without this, form widgets will render outside of the drawing
    // area.  This clips them to the drawing area.
    gtk_widget_set_has_window(v_fixed, true);

    v_draw_area = gtk_drawing_area_new();
    gtk_widget_set_double_buffered(v_draw_area, false);  // we handle it
    gtk_widget_show(v_draw_area);
    gtk_widget_set_size_request(v_draw_area, v_width, v_height);
    gtk_fixed_put(GTK_FIXED(v_fixed), v_draw_area, 0, 0);
//gtk_container_set_resize_mode(GTK_CONTAINER(v_fixed), GTK_RESIZE_PARENT);

    v_hsba = (GtkAdjustment*)gtk_adjustment_new(0.0, 0.0, v_width, 8.0,
        v_width/2, v_width);
    v_hsb = gtk_hscrollbar_new(GTK_ADJUSTMENT(v_hsba));
    gtk_widget_hide(v_hsb);
    v_vsba = (GtkAdjustment*)gtk_adjustment_new(0.0, 0.0, v_height, 8.0,
        v_height/2, v_height);
    v_vsb = gtk_vscrollbar_new(GTK_ADJUSTMENT(v_vsba));
    gtk_widget_hide(v_vsb);

#if GTK_CHECK_VERSION(3,0,0)
    v_pixmap = new ndkPixmap(gtk_widget_get_window(v_draw_area),
        v_width, v_height);
    v_gc = new ndkGC(gtk_widget_get_window(v_draw_area));
    htmColor c;
    tk_parse_color("white", &c);
    tk_set_foreground(c.pixel);
    v_gc->draw_rectangle(v_pixmap, true, 0, 0, v_width, v_height);
#else
    v_pixmap = gdk_pixmap_new(gtk_widget_get_window(v_draw_area),
        v_width, v_height, tk_visual_depth());
    v_gc = gdk_gc_new(v_pixmap);
    htmColor c;
    tk_parse_color("white", &c);
    tk_set_foreground(c.pixel);
    gdk_draw_rectangle(v_pixmap, v_gc, true, 0, 0, v_width, v_height);
#endif

    int events = gtk_widget_get_events(v_draw_area);
    gtk_widget_set_events(v_draw_area, events
        | GDK_SCROLL_MASK
        | GDK_EXPOSURE_MASK
        | GDK_STRUCTURE_MASK
        | GDK_FOCUS_CHANGE_MASK
        | GDK_POINTER_MOTION_MASK
        | GDK_LEAVE_NOTIFY_MASK
        | GDK_ENTER_NOTIFY_MASK
        | GDK_BUTTON_RELEASE_MASK
        | GDK_BUTTON_PRESS_MASK
        | GDK_KEY_PRESS_MASK
        | GDK_KEY_RELEASE_MASK);

    g_signal_connect(G_OBJECT(v_fixed), "size-allocate",
        G_CALLBACK(v_resize_hdlr), this);
#ifdef NEW_SC
//    g_signal_connect(G_OBJECT(v_form), "size-allocate",
//        G_CALLBACK(v_resize_hdlr), this);
#endif

    g_signal_connect(G_OBJECT(v_vsba), "value-changed",
        G_CALLBACK(v_vsb_change_hdlr), this);
    g_signal_connect(G_OBJECT(v_hsba), "value-changed",
        G_CALLBACK(v_hsb_change_hdlr), this);

    gtk_selection_add_targets(GTK_WIDGET(v_draw_area),
        GDK_SELECTION_PRIMARY, targets, n_targets);
    g_signal_connect(G_OBJECT(v_draw_area), "selection-clear-event",
        G_CALLBACK(v_selection_clear_hdlr), this);
    g_signal_connect(G_OBJECT(v_draw_area), "selection-get",
        G_CALLBACK(v_selection_get_hdlr), this);

#if GTK_CHECK_VERSION(3,0,0)
    g_signal_connect(G_OBJECT(v_draw_area), "draw",
        G_CALLBACK(v_expose_event_hdlr), this);
#else
    g_signal_connect(G_OBJECT(v_draw_area), "expose-event",
        G_CALLBACK(v_expose_event_hdlr), this);
#endif
    g_signal_connect(G_OBJECT(v_draw_area), "motion-notify-event",
        G_CALLBACK(v_motion_event_hdlr), this);
    g_signal_connect(G_OBJECT(v_draw_area), "button-press-event",
        G_CALLBACK(v_button_dn_hdlr), this);
    g_signal_connect(G_OBJECT(v_draw_area), "button-release-event",
        G_CALLBACK(v_button_up_hdlr), this);

    g_signal_connect_after(G_OBJECT(v_draw_area), "key-press-event",
        G_CALLBACK(v_key_dn_hdlr), this);

    g_signal_connect(G_OBJECT(v_draw_area), "focus-in-event",
        G_CALLBACK(v_focus_event_hdlr), this);
    g_signal_connect(G_OBJECT(v_draw_area), "focus-out-event",
        G_CALLBACK(v_focus_event_hdlr), this);
    g_signal_connect(G_OBJECT(v_draw_area), "leave-notify-event",
        G_CALLBACK(v_focus_event_hdlr), this);
    g_signal_connect(G_OBJECT(v_draw_area), "enter-notify-event",
        G_CALLBACK(v_focus_event_hdlr), this);
    g_signal_connect(G_OBJECT(v_draw_area), "scroll-event",
        G_CALLBACK(v_scroll_event_hdlr), this);

#ifdef NEW_SC
    GtkWidget *v_scrwin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(v_scrwin);
    gtk_container_add(GTK_CONTAINER(v_scrwin), v_fixed);
    gtk_table_attach(GTK_TABLE(v_form), v_scrwin, 0, 1, 0, 1,
#else
    gtk_table_attach(GTK_TABLE(v_form), v_fixed, 0, 1, 0, 1,
#endif
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 0, 0);

    gtk_table_attach(GTK_TABLE(v_form), v_vsb, 1, 2, 0, 1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 0, 0);

    gtk_table_attach(GTK_TABLE(v_form), v_hsb, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 0, 0);

    // Font tracking
    GTKfont::trackFontChange(v_draw_area, FNT_MOZY);
    GTKfont::trackFontChange(v_draw_area, FNT_MOZY_FIXED);
    g_signal_connect(G_OBJECT(v_draw_area), "style-set",
        G_CALLBACK(v_font_change_hdlr), this);

    set_font(FC.getName(FNT_MOZY));
    set_fixed_font(FC.getName(FNT_MOZY_FIXED));

    g_log_set_handler("Pango", G_LOG_LEVEL_WARNING, pango_warning_hdlr, NULL);
}


gtk_viewer::~gtk_viewer()
{
    g_signal_handlers_disconnect_by_func(G_OBJECT(v_draw_area),
        (gpointer)v_focus_event_hdlr, this);
#if GTK_CHECK_VERSION(3,0,0)
    v_pixmap->dec_ref();
#else
    gdk_pixmap_unref(v_pixmap);
#endif
}


//-----------------------------------------------------------------------------
// General interface (ViewerWidget function implementations)

void
gtk_viewer::set_transaction(Transaction *t, const char*)
{
    v_transact = t;
}


Transaction *
gtk_viewer::get_transaction()
{
    return (v_transact);
}


bool
gtk_viewer::check_halt_processing(bool)
{
    return (false);
}


void
gtk_viewer::set_halt_proc_sens(bool)
{
}


void
gtk_viewer::set_status_line(const char*)
{
}


htmImageInfo *
gtk_viewer::new_image_info(const char *url, bool progressive)
{
    htmImageInfo *image = new htmImageInfo();
    if (progressive)
        image->options = IMAGE_PROGRESSIVE | IMAGE_ALLOW_SCALE;
    else
        image->options = IMAGE_DELAYED;
    image->url = strdup(url);
    return (image);
}


const char *
gtk_viewer::get_url()
{
    return (0);
}


bool
gtk_viewer::no_url_cache()
{
    return (true);
}


int
gtk_viewer::image_load_mode()
{
    return (HLPparams::LoadSync);
}


int
gtk_viewer::image_debug_mode()
{
    return (HLPparams::LInormal);
}


GRwbag *
gtk_viewer::get_widget_bag()
{
    return (0);
}


//-----------------------------------------------------------------------------
// Misc. public functions


// Show updated text, try to keep present scroll positions.
//
void
gtk_viewer::set_source(const char *string)
{
    int x = htm_viewarea.x;
    int y = htm_viewarea.y;
    setSource(string);
    set_scroll_position(x, true);
    set_scroll_position(y, false);
    if (!htm_in_layout && htm_initialized)
        trySync();
}


// Set the proportional font used.  The name is in the form
// "face [style keywords] [size]".  The style keywords are ignored.
//
void
gtk_viewer::set_font(const char *fontspec)
{
    char *family;
    int sz;
    FC.parse_freeform_font_string(fontspec, &family, 0, &sz, 0);
    setFontFamily(family, sz);
    delete [] family;
}


void
gtk_viewer::set_fixed_font(const char *fontspec)
{
    char *family;
    int sz;
    FC.parse_freeform_font_string(fontspec, &family, 0, &sz, 0);
    setFixedFontFamily(family, sz);
    delete [] family;
}


// Hide or restore the drawing area.  The drawing area is hidden when
// forms are displayed.
//
void
gtk_viewer::hide_drawing_area(bool hide)
{
    if (hide)
        gtk_widget_hide(v_draw_area);
    else
        gtk_widget_show(v_draw_area);
}


// Add a widget to the viewing area, exported for forms support.
//
void
gtk_viewer::add_widget(GtkWidget *w)
{
    gtk_fixed_put(GTK_FIXED(v_fixed), w, 0, 0);
}


int
gtk_viewer::scroll_position(bool horiz)
{
    if (horiz) {
        if (gtk_widget_get_mapped(v_hsb) && v_hsba)
            return ((int)gtk_adjustment_get_value(GTK_ADJUSTMENT(v_hsba)));
    }
    else {
        if (gtk_widget_get_mapped(v_vsb) && v_vsba)
            return ((int)gtk_adjustment_get_value(GTK_ADJUSTMENT(v_vsba)));
    }
    return (0);
}


void
gtk_viewer::set_scroll_position(int value, bool horiz)
{
    if (horiz) {
        if (gtk_widget_get_mapped(v_hsb) && v_hsba) {
            if (value < 0)
                value = 0;
            int v = (int)(gtk_adjustment_get_upper(v_hsba) -
                gtk_adjustment_get_page_size(v_hsba));
            if (value > v)
                value = v;
            int oldv = (int)gtk_adjustment_get_value(v_hsba);
            if (value == oldv && value != htm_viewarea.x) {
                // Make sure handler is called after (e.g.) font change.
                hsb_change_handler(value);
                return;
            }
            gtk_adjustment_set_value(v_hsba, value);
        }
    }
    else {
        if (gtk_widget_get_mapped(v_vsb) && v_vsba) {
            if (value < 0)
                value = 0;
            int v = (int)(gtk_adjustment_get_upper(v_vsba) -
                gtk_adjustment_get_page_size(v_vsba));
            if (value > v)
                value = v;
            int oldv = (int)gtk_adjustment_get_value(v_vsba);
            if (value == oldv && value != htm_viewarea.y) {
                // Make sure handler is called after (e.g.) font change.
                vsb_change_handler(value);
                return;
            }
            gtk_adjustment_set_value(v_vsba, value);
        }
    }
}


void
gtk_viewer::set_scroll_policy(GtkPolicyType hpolicy, GtkPolicyType vpolicy)
{
    v_hpolicy = hpolicy;
    v_vpolicy = vpolicy;
}


// Return the Y coordinate of the named anchor.
//
int
gtk_viewer::anchor_position(const char *name)
{
    return (anchorPosByName(name));
}


// Ensure that the bounding box specified is visible.
//
void
gtk_viewer::scroll_visible(int l, int t, int r, int b)
{
    if (gtk_widget_get_mapped(v_hsb) && v_hsba) {
        int xmin = l < r ? l : r - 50;
        int xmax = l > r ? l : r + 50;
        gtk_adjustment_clamp_page(GTK_ADJUSTMENT(v_hsba), xmin, xmax);
    }
    if (gtk_widget_get_mapped(v_vsb) && v_vsba) {
        int ymin = t < b ? t : b - 50;
        int ymax = t > b ? t : b + 50;
        gtk_adjustment_clamp_page(GTK_ADJUSTMENT(v_vsba), ymin, ymax);
    }
}


//-----------------------------------------------------------------------------
// Rendering interface (htmInterface function implementations)

void
gtk_viewer::tk_resize_area(int w, int h)
{
#ifdef NEW_SC
//XXX
fprintf(stderr, "resize_area %d %d\n", w, h);
v_da_width = w;
v_da_height = h;
#endif
    // This is called after formatting with the document size.
    int wid, hei;
    if (gtk_widget_get_mapped(v_draw_area)) {
        GtkAllocation a;
        gtk_widget_get_allocation(v_form, &a);
        wid = a.width;
        hei = a.height;
    }
    else {
        wid = v_width;
        hei = v_height;
    }

    htm_viewarea.x = 0;

    if (h > hei) {
        double val = gtk_adjustment_get_value(v_vsba);
        gtk_adjustment_set_upper(v_vsba, h);
        gtk_adjustment_set_page_size(v_vsba, v_height);
        gtk_adjustment_set_page_increment(v_vsba, v_height/2);
        gtk_adjustment_changed(v_vsba);
        if (v_vpolicy != GTK_POLICY_NEVER)
            gtk_widget_show(v_vsb);
        else
            gtk_widget_hide(v_vsb);
        if (val > h - v_height)
            gtk_adjustment_set_value(v_vsba, h - v_height);
    }
    else {
        if (v_vpolicy != GTK_POLICY_ALWAYS)
            gtk_widget_hide(v_vsb);
        else
            gtk_widget_show(v_vsb);

        if (v_width <= 1 || v_height <= 1) {
            GtkAllocation a;
            gtk_widget_get_allocation(v_form, &a);
            canvas_resize_handler(&a);
        }
    }

    if (w > wid+8) {
        // We can get by without a horizontal scrollbar if w is only
        // slightly larger than wid.  The scrollbar can be annoying.

        double val = gtk_adjustment_get_value(v_hsba);
        gtk_adjustment_set_upper(v_hsba, w);
        gtk_adjustment_set_page_size(v_hsba, v_width);
        gtk_adjustment_set_page_increment(v_hsba, v_width/2);
        gtk_adjustment_changed(v_hsba);
        if (v_hpolicy != GTK_POLICY_NEVER)
            gtk_widget_show(v_hsb);
        else
            gtk_widget_hide(v_hsb);
        if (val > w - v_width)
            gtk_adjustment_set_value(v_hsba, w - v_width);
    }
    else {
        if (v_hpolicy != GTK_POLICY_ALWAYS)
            gtk_widget_hide(v_hsb);
        else
            gtk_widget_show(v_hsb);
    }
}


void
gtk_viewer::tk_refresh_area(int x, int y, int w, int h)
{
//XXX    if (gtk_widget_get_window(v_draw_area))
//XXX        gtk_widget_queue_draw_area(v_draw_area, x, y, w, h);
    GdkWindow *win = gtk_widget_get_window(v_draw_area);
    if (win)
#if GTK_CHECK_VERSION(3,0,0)
        v_pixmap->copy_to_window(win, v_gc, x, y, x, y, w, h);
#else
        gdk_window_copy_area(win, v_gc, x, y, v_pixmap, x, y, w, h);
#endif
}


void
gtk_viewer::tk_window_size(htmInterface::WinRetMode mode,
    unsigned int *wp, unsigned int *hp)
{
    if (mode == htmInterface::VIEWABLE) {
        // Return the total available viewable size, assuming no
        // scrollbars.
        if (gtk_widget_get_mapped(v_draw_area)) {
            GtkAllocation a;
            gtk_widget_get_allocation(v_form, &a);
            *wp = a.width;
            *hp = a.height;
        }
        else {
            *wp = v_width;
            *hp = v_height;
        }
    }
    else {
        // Return the size of the drawing pixmap.
        *wp = v_width;
        *hp = v_height;
    }
}


unsigned int
gtk_viewer::tk_scrollbar_width()
{
    GtkAllocation a;
    gtk_widget_get_allocation(v_vsb, &a);
    return (a.width);
}


namespace {
    // The cursor for hovering over links
#define fingers_width 16
#define fingers_height 16
#define fingers_x_hot 4
#define fingers_y_hot 0
    const char fingers_bits[] = {
        char(0xcf), char(0xff), char(0xb7), char(0xff),
        char(0xb7), char(0xff), char(0xb7), char(0xff),
        char(0x37), char(0x92), char(0xb7), char(0x6d),
        char(0xb1), char(0x6d), char(0xb6), char(0x6d),
        char(0xb6), char(0x6d), char(0xf6), char(0x7f),
        char(0xf6), char(0x7f), char(0xfd), char(0x7f),
        char(0xfd), char(0xbf), char(0xfb), char(0xbf),
        char(0xfb), char(0xbf), char(0x03), char(0x80)};

#define fingers_m_width 16
#define fingers_m_height 16
    const char fingers_m_bits[] = {
        char(0x30), char(0x00), char(0x78), char(0x00),
        char(0x78), char(0x00), char(0x78), char(0x00),
        char(0xf8), char(0x6d), char(0xf8), char(0xff),
        char(0xfe), char(0xff), char(0xff), char(0xff),
        char(0xff), char(0xff), char(0xff), char(0xff),
        char(0xff), char(0xff), char(0xfe), char(0xff),
        char(0xfe), char(0x7f), char(0xfc), char(0x7f),
        char(0xfc), char(0x7f), char(0xfc), char(0x7f)};
}


void
gtk_viewer::tk_set_anchor_cursor(bool set)
{
    GdkWindow *window = gtk_widget_get_window(v_draw_area);
    if (!window)
        return;
    if (!v_cursor) {
#if GTK_CHECK_VERSION(3,0,0)
        GdkColor white, black;
        white.pixel = 0xffffff;
        ndkGC::query_rgb(&white, GRX->Visual());
        black.pixel = 0;
        ndkGC::query_rgb(&black, GRX->Visual());
        v_cursor = new ndkCursor(window, fingers_bits, fingers_m_bits,
            fingers_width, fingers_height, fingers_x_hot, fingers_y_hot,
            &white, &black);
#else
            
        GdkColor fg, bg;
        fg.pixel = 1;
        bg.pixel = 0;
        GdkPixmap *shape = gdk_pixmap_create_from_data(window, fingers_bits,
            fingers_width, fingers_height, 1, &fg, &bg);
        GdkPixmap *mask = gdk_pixmap_create_from_data(window, fingers_m_bits,
            fingers_m_width, fingers_m_height, 1, &fg, &bg);
        if (!shape || !mask)
            return;

        GdkColormap *colormap = gtk_widget_get_colormap(v_draw_area);
        GdkColor white, black;
        gdk_color_white(colormap, &white);
        gdk_color_black(colormap, &black);
        v_cursor = gdk_cursor_new_from_pixmap(shape, mask,
            &white, &black, fingers_x_hot, fingers_y_hot);
#endif
    }
#if GTK_CHECK_VERSION(3,0,0)
    if (set)
        v_cursor->set_in_window(window);
    else
        v_cursor->revert_in_window(window);
#else
    if (set)
        gdk_window_set_cursor(gtk_widget_get_window(v_draw_area), v_cursor);
    else
        gdk_window_set_cursor(gtk_widget_get_window(v_draw_area), 0);
#endif
}


unsigned int
gtk_viewer::tk_add_timer(int(*cb)(void*), void *arg)
{
    v_timers = new gtk_timer(cb, arg, v_timers);
    v_timers->id = v_timer_id_cnt++;
    return (v_timers->id);
}


void
gtk_viewer::tk_remove_timer(int id)
{
    gtk_timer *tp = 0, *tn;
    for (gtk_timer *t = v_timers; t; t = tn) {
        tn = t->next;
        if (t->id == id) {
            if (tp)
                tp->next = tn;
            else
                v_timers = tn;
            delete t;
            return;
        }
        tp = t;
    }
}


void
gtk_viewer::tk_start_timer(unsigned int id, int msec)
{
    for (gtk_timer *t = v_timers; t; t = t->next) {
        if (t->id == (int)id) {
            t->real_id = g_timeout_add(msec, (GSourceFunc)t->callback,
                t->arg);
            return;
        }
    }
}


// Let the world know about the current text selection.
//
void
gtk_viewer::tk_claim_selection(const char *seltext)
{
    if (seltext)
        gtk_selection_owner_set(v_draw_area, GDK_SELECTION_PRIMARY,
            GDK_CURRENT_TIME);
    else if (gdk_selection_owner_get(GDK_SELECTION_PRIMARY) ==
                gtk_widget_get_window(v_draw_area))
        gtk_selection_owner_set(0, GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
}


// Return a font allocated from the system.  The family can be an XFD,
// a face name, or a comma-separated list of face names to try.  The
// size is the requested pixel size.  This should always return some
// font.
//
htmFont *
gtk_viewer::tk_alloc_font(const char *family, int size, unsigned char style)
{
    char buf[256];
    if (xfd_t::is_xfd(family)) {
        xfd_t f(family);
        strcpy(buf, f.get_family());
        family = buf;
    }

    htmFont *font = new htmFont(this, family, size, style);
    PangoFontDescription *pfd = pango_font_description_from_string(family);

    if (style & FONT_BOLD)
        pango_font_description_set_weight(pfd, PANGO_WEIGHT_BOLD);
    if (style & FONT_ITALIC)
        pango_font_description_set_style(pfd, PANGO_STYLE_ITALIC);

    pango_font_description_set_size(pfd, size*PANGO_SCALE);
    font->xfont = pfd;

    PangoContext *pc = gtk_widget_get_pango_context(v_draw_area);
    PangoFont *pf = pango_context_load_font(pc, pfd);
    PangoFontMetrics *pm = pango_font_get_metrics(pf, 0);

    font->ascent = pm->ascent/PANGO_SCALE;
    font->descent = pm->descent/PANGO_SCALE;
    font->height = (pm->ascent + pm->descent)/PANGO_SCALE;
    font->lineheight = font->height + 1;
    font->width = pm->approximate_char_width/PANGO_SCALE;
    font->lbearing = 0;
    font->rbearing = 0;

    // normal interword spacing
    font->isp = font->width;

    // additional end-of-line spacing
    font->eol_sp = 0;

    // superscript x-offset, set later.
    font->sup_xoffset = 0;

    // superscript y-offset, set later.
    font->sup_yoffset = 0;

    // subscript x-offset
    font->sub_xoffset = 0;

    // subscript y-offset
    font->sub_yoffset = (int)(font->descent * 0.8);

    // underline offset
    font->ul_offset = pm->underline_position/PANGO_SCALE;
    font->ul_offset += font->descent;

    // underline thickness
    font->ul_thickness = pm->underline_thickness/PANGO_SCALE;

    // strikeout offset
    font->st_offset = pm->strikethrough_position/PANGO_SCALE;

    // strikeout descent
    font->st_thickness = pm->strikethrough_thickness/PANGO_SCALE;

    pango_font_metrics_unref(pm);
    return (font);
}


void
gtk_viewer::tk_release_font(void *f)
{
    pango_font_description_free((PangoFontDescription*)f);
}


void
gtk_viewer::tk_set_font(htmFont *font)
{
    v_font = (PangoFontDescription*)font->xfont;
    v_text_yoff = font->ascent;
}


namespace {
    // Check if the given unsigned char * is a valid utf-8 sequence.
    //
    // Return value :
    // If the string is valid utf-8, 0 is returned.
    // Else the position, starting from 1, is returned.
    //
    // Valid utf-8 sequences look like this :
    // 0xxxxxxx
    // 110xxxxx 10xxxxxx
    // 1110xxxx 10xxxxxx 10xxxxxx
    // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    // 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
    // 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
    //
    int is_utf8(const unsigned char *str, size_t len)
    {
        size_t i = 0;
        size_t continuation_bytes = 0;

        while (i < len)
        {
            if (str[i] <= 0x7F)
                continuation_bytes = 0;
            else if (str[i] == 0xc0 || str[i] == 0xc1)
                return (i + 1);  // These two are invalid.
            else if (str[i] >= 0xc2 && str[i] <= 0xdf) // 11000010 - 11011111
                continuation_bytes = 1;
            else if (str[i] >= 0xe0 && str[i] <= 0xef) // 11100000 - 11101111
                continuation_bytes = 2;
            else if (str[i] >= 0xf0 && str[i] <= 0xf4) // 11110000 - 11110100
                continuation_bytes = 3;

            /* RFC 3629 restricts sequences to end at U+10FFFF.
            else if (str[i] >= 0xf0 && str[i] <= 0xf7) // 11110000 - 11110111
                continuation_bytes = 3;
            else if (str[i] >= 0xf8 && str[i] <= 0xfb) // 11111000 - 11111011
                continuation_bytes = 4;
            else if (str[i] >= 0xfc && str[i] <= 0xfd) // 11111100 - 11111101
                continuation_bytes = 5;
            */
            else
                return (i + 1);
            i += 1;
            while (i < len && continuation_bytes > 0 &&
                str[i] >= 0x80 && str[i] <= 0xbf) { // 10000000 - 10111111
                i += 1;
                continuation_bytes -= 1;
            }
            if (continuation_bytes != 0)
                return (i + 1);
        }
        return (0);
    }
}


int
gtk_viewer::tk_text_width(htmFont *font, const char *text, int len)
{
    gsize xlen;
    if (v_iso8859 || is_utf8((const unsigned char*)text, len)) {
        // Must convert to UTF-8.
        text = g_convert(text, len, "UTF-8", "ISO-8859-1", 0, &xlen, 0);
    }
    else
        xlen = strlen(text);

    PangoContext *pc = gtk_widget_get_pango_context(v_draw_area);
    PangoLayout *pl = pango_layout_new(pc);
    PangoFontDescription *pfd = (PangoFontDescription*)font->xfont;
    pango_layout_set_font_description(pl, pfd);
    pango_layout_set_text(pl, text, xlen);
    if (v_iso8859) {
        g_free((void*)text);
    }

    int tw, th;
    pango_layout_get_pixel_size(pl, &tw, &th);
    g_object_unref(pl);
    return (tw);
}


CCXmode
gtk_viewer::tk_visual_mode()
{
    GdkVisual *vs = GRX->Visual();
    switch (gdk_visual_get_visual_type(vs)) {
    case GDK_VISUAL_STATIC_GRAY:
        return (MODE_BW);
    case GDK_VISUAL_GRAYSCALE:
        return (MODE_MY_GRAY);
    case GDK_VISUAL_STATIC_COLOR:
        return (MODE_STD_CMAP);
    case GDK_VISUAL_PSEUDO_COLOR:
        return (MODE_PALETTE);
    case GDK_VISUAL_TRUE_COLOR:
    case GDK_VISUAL_DIRECT_COLOR:
        return (MODE_TRUE);
    }
    return (MODE_UNDEFINED);
}


int
gtk_viewer::tk_visual_depth()
{
    return (gdk_visual_get_depth(GRX->Visual()));
}


htmPixmap *
gtk_viewer::tk_new_pixmap(int w, int h)
{
#if GTK_CHECK_VERSION(3,0,0)
    ndkPixmap *pm = new ndkPixmap(gtk_widget_get_window(v_draw_area), w, h);
#else
    GdkPixmap *pm = gdk_pixmap_new(0, w, h, tk_visual_depth());
#endif
    htmColor c;
    tk_parse_color("white", &c);
    tk_set_foreground(c.pixel);
#if GTK_CHECK_VERSION(3,0,0)
    v_gc->draw_rectangle(pm, true, 0, 0, w, h);
#else
    gdk_draw_rectangle(pm, v_gc, true, 0, 0, w, h);
#endif
    return ((htmPixmap*)pm);
}


void
gtk_viewer::tk_release_pixmap(htmPixmap *pm)
{
    if (pm)
#if GTK_CHECK_VERSION(3,0,0)
        ((ndkPixmap*)pm)->dec_ref();
#else
        gdk_pixmap_unref((GdkPixmap*)pm);
#endif
}


htmPixmap *
gtk_viewer::tk_pixmap_from_info(htmImage *image, htmImageInfo *info,
    unsigned int *color_map)
{
    unsigned int size = info->width * info->height;

    (void)image;
#if GTK_CHECK_VERSION(3,0,0)
    ndkImage *img = new ndkImage(ndkIMAGE_FASTEST,
        GRX->Visual(), info->width, info->height);
#else
    GdkImage *img = gdk_image_new(GDK_IMAGE_FASTEST,
        GRX->Visual(), info->width, info->height);
#endif

    int wid = info->width;
    for (unsigned int i = 0; i < size; i++) {
        int y = i/wid;
        int x = i%wid;
        unsigned int px = color_map[info->data[i]];
#ifdef WITH_QUARTZ
        // Hmmmm, seems that Quartz requires byte reversal.
        unsigned int qpx;
        unsigned char *c1 = ((unsigned char*)&px) + 3;
        unsigned char *c2 = (unsigned char*)&qpx;
        *c2++ = *c1--;
        *c2++ = *c1--;
        *c2++ = *c1--;
        *c2++ = *c1--;
        gdk_image_put_pixel(img, x, y, qpx);
#else
#if GTK_CHECK_VERSION(3,0,0)
        img->put_pixel(x, y, px);
#else
        gdk_image_put_pixel(img, x, y, px);
#endif
#endif
    }

#if GTK_CHECK_VERSION(3,0,0)
    ndkPixmap *pmap = new ndkPixmap(gtk_widget_get_window(v_draw_area),
        info->width, info->height);
    img->copy_to_pixmap(pmap, v_gc, 0, 0, 0, 0, info->width, info->height);
    delete img;
#else
    GdkPixmap *pmap = gdk_pixmap_new(0, info->width, info->height,
        tk_visual_depth());
    gdk_draw_image(pmap, v_gc, img, 0, 0, 0, 0, info->width, info->height);
    gdk_image_destroy(img);
#endif
    return (pmap);
}


void
gtk_viewer::tk_set_draw_to_pixmap(htmPixmap *pm)
{
    if (pm) {
        if (!v_pixmap_bak) {
            v_pixmap_bak = v_pixmap;
#if GTK_CHECK_VERSION(3,0,0)
            v_pixmap = (ndkPixmap*)pm;
#else
            v_pixmap = (GdkPixmap*)pm;
#endif
        }
    }
    else {
        if (v_pixmap_bak) {
            v_pixmap = v_pixmap_bak;
            v_pixmap_bak = 0;
        }
    }
}


htmBitmap *
gtk_viewer::tk_bitmap_from_data(int width, int height, unsigned char *data)
{
    if (!data)
        return (0);
    GdkColor fg, bg;
    fg.pixel = 1;
    bg.pixel = 0;
#if GTK_CHECK_VERSION(3,0,0)
    ndkPixmap *pm = new ndkPixmap(gtk_widget_get_window(v_draw_area),
        (char*)data, width, height);
#else
    GdkPixmap *pm = gdk_pixmap_create_from_data(v_draw_area->window,
        (char*)data, width, height, 1, &fg, &bg);
#endif
    return (pm);
}


void
gtk_viewer::tk_release_bitmap(htmBitmap *bmap)
{
    if (bmap)
#if GTK_CHECK_VERSION(3,0,0)
        ((ndkPixmap*)bmap)->dec_ref();
#else
        gdk_pixmap_unref((GdkPixmap*)bmap);
#endif
}


htmXImage *
gtk_viewer::tk_new_image(int w, int h)
{
#if GTK_CHECK_VERSION(3,0,0)
    return ((htmXImage*)new ndkImage(ndkIMAGE_FASTEST, GRX->Visual(), w, h));
#else
    return ((htmXImage*)gdk_image_new(GDK_IMAGE_FASTEST, GRX->Visual(), w, h));
#endif
}


void
gtk_viewer::tk_fill_image(htmXImage *ximage, unsigned char *data,
    unsigned int *color_map, int lo, int hi)
{
#if GTK_CHECK_VERSION(3,0,0)
    ndkImage *image = (ndkImage*)ximage;
    int wid = image->get_width();
#else
    GdkImage *image = (GdkImage*)ximage;
    int wid = gdk_image_get_width(image);
#endif

    for (int i = lo; i < hi; i++) {
        int y = i/wid;
        int x = i%wid;
        unsigned int px = color_map[data[i]];
#ifdef WITH_QUARTZ
        // Hmmmm, seems that Quartz requires byte reversal.
        unsigned int qpx;
        unsigned char *c1 = ((unsigned char*)&px) + 3;
        unsigned char *c2 = (unsigned char*)&qpx;
        *c2++ = *c1--;
        *c2++ = *c1--;
        *c2++ = *c1--;
        *c2++ = *c1--;
        gdk_image_put_pixel(image, x, y, qpx);
#else
#if GTK_CHECK_VERSION(3,0,0)
        image->put_pixel(x, y, px);
#else
        gdk_image_put_pixel(image, x, y, px);
#endif
#endif
    }
}


void
gtk_viewer::tk_draw_image(int xs, int ys, htmXImage *image,
    int x, int y, int w, int h)
{
#if GTK_CHECK_VERSION(3,0,0)
    ((ndkImage*)image)->copy_to_pixmap(v_pixmap, v_gc, x, y, xs, ys, w, h);
#else
    gdk_draw_image(v_pixmap, v_gc, (GdkImage*)image, xs, ys, x, y, w, h);
#endif
}


void
gtk_viewer::tk_release_image(htmXImage *image)
{
#if GTK_CHECK_VERSION(3,0,0)
    delete (ndkImage*)image;
#else
    gdk_image_destroy((GdkImage*)image);
#endif
}


void
gtk_viewer::tk_set_foreground(unsigned int pix)
{
    GdkColor c;
    c.pixel = pix;
#if GTK_CHECK_VERSION(3,0,0)
    v_gc->set_foreground(&c);
#else
    gdk_gc_set_foreground(v_gc, &c);
#endif
}


void
gtk_viewer::tk_set_background(unsigned int pix)
{
    GdkColor c;
    c.pixel = pix;
#if GTK_CHECK_VERSION(3,0,0)
    v_gc->set_background(&c);
#else
    gdk_gc_set_background(v_gc, &c);
#endif
}


bool
gtk_viewer::tk_parse_color(const char *spec, htmColor *clr)
{
    GdkColor c;
    if (gtk_ColorSet(&c, spec)) {
        clr->red = c.red >> 8;
        clr->green = c.green >> 8;
        clr->blue = c.blue >> 8;
        clr->pixel = c.pixel;
        return (true);
    }
    return (false);
}


bool
gtk_viewer::tk_alloc_color(htmColor *clr)
{
    GdkColor c;
    c.red = clr->red << 8;
    c.green = clr->green << 8;
    c.blue = clr->blue << 8;
#if GTK_CHECK_VERSION(3,0,0)
    ndkGC::query_pixel(&c, GRX->Visual());
    clr->pixel = c.pixel;
    return (true);
#else
    bool ret = gdk_colormap_alloc_color(gtk_widget_get_colormap(v_draw_area),
        &c, false, true);
    if (ret) {
        clr->pixel = c.pixel;
        return (true);
    }
    return (false);
#endif
}


int
gtk_viewer::tk_query_colors(htmColor *clrs, unsigned int sz)
{
    GdkColor *colors = new GdkColor[sz];
    for (unsigned int i = 0; i < sz; i++)
        colors[i].pixel = clrs[i].pixel;
#if GTK_CHECK_VERSION(3,0,0)
    for (unsigned int i = 0; i < sz; i++) {
        ndkGC::query_rgb(colors + i, GRX->Visual());
        clrs[i].red   = colors[i].red >> 8;
        clrs[i].green = colors[i].green >> 8;
        clrs[i].blue  = colors[i].blue >> 8;
    }
#else
    GdkColormap *colormap = gtk_widget_get_colormap(v_draw_area);
    for (unsigned int i = 0; i < sz; i++) {
        gdk_colormap_query_color(colormap, colors[i].pixel, colors + i);
        clrs[i].red   = colors[i].red >> 8;
        clrs[i].green = colors[i].green >> 8;
        clrs[i].blue  = colors[i].blue >> 8;
    }
#endif
    delete [] colors;
    return (sz);
}


void
gtk_viewer::tk_free_colors(unsigned int *pixels, unsigned int sz)
{
#if GTK_CHECK_VERSION(3,0,0)
    (void)pixels;
    (void)sz;
#else
    if (!sz)
        return;
    GdkColor *c = new GdkColor[sz];
    for (unsigned int i = 0; i < sz; i++)
        c[i].pixel = pixels[i];
    gdk_colormap_free_colors(gtk_widget_get_colormap(v_draw_area), c, sz);
    delete [] c;
#endif
}


bool
gtk_viewer::tk_get_pixels(unsigned short *reds, unsigned short *greens,
    unsigned short *blues, unsigned int sz, unsigned int *pixels)
{
    for (unsigned int i = 0; i < sz; i++) {
        GdkColor c;
        c.red = reds[i] << 8;
        c.green = greens[i] << 8;
        c.blue = blues[i] << 8;
#if GTK_CHECK_VERSION(3,0,0)
        ndkGC::query_pixel(&c, GRX->Visual());
        pixels[i] = c.pixel;
#else
        bool ret =
            gdk_colormap_alloc_color(gtk_widget_get_colormap(v_draw_area),
                &c, false, true);
        if (ret)
            pixels[i] = c.pixel;
        else
            // can't happen
            pixels[i] = 0;
#endif
    }
    return (true);
}


void
gtk_viewer::tk_set_clip_mask(htmPixmap*, htmBitmap *bmap)
{
#if GTK_CHECK_VERSION(3,0,0)
    v_gc->set_clip_mask((ndkPixmap*)bmap);
#else
    gdk_gc_set_clip_mask(v_gc, (GdkBitmap*)bmap);
#endif
}


void
gtk_viewer::tk_set_clip_origin(int x, int y)
{
#if GTK_CHECK_VERSION(3,0,0)
    v_gc->set_clip_origin(x, y);
#else
    gdk_gc_set_clip_origin(v_gc, x, y);
#endif
}


void
gtk_viewer::tk_set_clip_rectangle(htmRect *r)
{
#if GTK_CHECK_VERSION(3,0,0)
    if (!r) {
        v_gc->set_clip_rectangle(0);
        return;
    }
    GdkRectangle rect;
    rect.x = r->x;
    rect.y = r->y;
    rect.width = r->width;
    rect.height = r->height;
    v_gc->set_clip_origin(0, 0);
    v_gc->set_clip_rectangle(&rect);
#else
    if (!r) {
        gdk_gc_set_clip_rectangle(v_gc, 0);
        return;
    }
    GdkRectangle rect;
    rect.x = r->x;
    rect.y = r->y;
    rect.width = r->width;
    rect.height = r->height;
    gdk_gc_set_clip_origin(v_gc, 0, 0);
    gdk_gc_set_clip_rectangle(v_gc, &rect);
#endif
}


void
gtk_viewer::tk_draw_pixmap(int xs, int ys, htmPixmap *pm,
    int x, int y, int w, int h)
{
#if GTK_CHECK_VERSION(3,0,0)
    ((ndkPixmap*)pm)->copy_to_pixmap(v_pixmap, v_gc, x, y, xs, ys, w, h);
#else
    gdk_window_copy_area(v_pixmap, v_gc, xs, ys, (GdkPixmap*)pm, x, y, w, h);
#endif
}


void
gtk_viewer::tk_tile_draw_pixmap(int org_x, int org_y, htmPixmap *pm,
    int x, int y, int w, int h)
{
#if GTK_CHECK_VERSION(3,0,0)
    if (!pm)
#else
    if (!GDK_IS_PIXMAP(pm))
#endif
        return;
#ifdef WIN32
    // The GC tiling feature is not supported in the Windows GTK2
    // port.

    int pw, ph;
    gdk_drawable_get_size(GDK_DRAWABLE(pm), &pw, &ph);
    int n = (x - org_x)/pw;
    int x0 = org_x + n*pw;
    if (x0 > x)
        x0 -= pw;
    n = (y - org_y)/ph;
    int y0 = org_y + n*ph;
    if (y0 > y)
        y0 -= ph;
    int ix = x0;
    int iy = y0;
    for (;;) {
        if (ix >= x + w) {
            ix = x0;
            iy += ph;
        }
        if (iy >= y + h)
            break;
        int left = ix;
        if (left < x)
            left = x;
        int top = iy;
        if (top < y)
            top = y;
        int right = ix + pw;
        if (right > x + w)
            right = x + w;
        int bottom = iy + ph;
        if (bottom > y + h)
            bottom = y + h;
        gdk_window_copy_area(v_pixmap, v_gc, left, top, (GdkPixmap*)pm,
            left - ix, top - iy, right - left, bottom - top);
        ix += pw;
    }
#else
#if GTK_CHECK_VERSION(3,0,0)
    v_gc->set_fill(ndkGC_TILED);
    v_gc->set_tile((ndkPixmap*)pm);
    v_gc->set_ts_origin(org_x, org_y);
    v_gc->draw_rectangle(v_pixmap, true, x, y, w, h);
    v_gc->set_fill(ndkGC_SOLID);
#else
    gdk_gc_set_fill(v_gc, GDK_TILED);
    gdk_gc_set_tile(v_gc, GDK_DRAWABLE(pm));
    gdk_gc_set_ts_origin(v_gc, org_x, org_y);
    gdk_draw_rectangle(v_pixmap, v_gc, true, x, y, w, h);
    gdk_gc_set_fill(v_gc, GDK_SOLID);
#endif
#endif
}


void
gtk_viewer::tk_draw_rectangle(bool fill, int x, int y, int w, int h)
{
#if GTK_CHECK_VERSION(3,0,0)
    v_gc->draw_rectangle(v_pixmap, fill, x, y, w, h);
#else
    gdk_draw_rectangle(v_pixmap, v_gc, fill, x, y, w, h);
#endif
}


void
gtk_viewer::tk_set_line_style(FillMode m)
{
#if GTK_CHECK_VERSION(3,0,0)
    if (m == TILED)
        v_gc->set_line_attributes(1, ndkGC_LINE_DOUBLE_DASH,
            ndkGC_CAP_BUTT, ndkGC_JOIN_BEVEL);
    else
        v_gc->set_line_attributes(1, ndkGC_LINE_SOLID,
            ndkGC_CAP_BUTT, ndkGC_JOIN_BEVEL);
#else
    if (m == TILED)
        gdk_gc_set_line_attributes(v_gc, 1, GDK_LINE_DOUBLE_DASH,
            GDK_CAP_BUTT, GDK_JOIN_BEVEL);
    else
        gdk_gc_set_line_attributes(v_gc, 1, GDK_LINE_SOLID,
            GDK_CAP_BUTT, GDK_JOIN_BEVEL);
#endif
}


void
gtk_viewer::tk_draw_line(int x1, int y1, int x2, int y2)
{
#if GTK_CHECK_VERSION(3,0,0)
    v_gc->draw_line(v_pixmap, x1, y1, x2, y2);
#else
    gdk_draw_line(v_pixmap, v_gc, x1, y1, x2, y2);
#endif
}


void
gtk_viewer::tk_draw_text(int x, int y, const char *text, int len)
{
    if (v_iso8859 || is_utf8((const unsigned char*)text, len)) {
        // Must convert to UTF-8.
        gsize xlen;
        text = g_convert(text, len, "UTF-8", "ISO-8859-1", 0, &xlen, 0);
    }

#if GTK_CHECK_VERSION(3,0,0)
    PangoContext *pcx = gtk_widget_get_pango_context(v_draw_area);
    pango_context_set_font_description(pcx, v_font);
    PangoLayout *lout = pango_layout_new(pcx);
    pango_layout_set_text(lout, text, -1);
#else
    gtk_widget_modify_font(v_draw_area, v_font);
    PangoLayout *lout = gtk_widget_create_pango_layout(v_draw_area, text);
#endif
    if (v_iso8859) {
        g_free((void*)text);
    }
    y -= v_text_yoff;
#if GTK_CHECK_VERSION(3,0,0)
    v_gc->draw_pango_layout(v_pixmap, x, y, lout);
#else
    gdk_draw_layout(v_pixmap, v_gc, x, y, lout);
#endif
    g_object_unref(lout);
}


void
gtk_viewer::tk_draw_polygon(bool fill, htmPoint *points, int numpts)
{
    GdkPoint *gpts = new GdkPoint[numpts];
    for (int i = 0; i < numpts; i++) {
        gpts[i].x = points[i].x;
        gpts[i].y = points[i].y;
    }
#if GTK_CHECK_VERSION(3,0,0)
    v_gc->draw_polygon(v_pixmap, fill, gpts, numpts);
#else
    gdk_draw_polygon(v_pixmap, v_gc, fill, gpts, numpts);
#endif
    delete [] gpts;
}


void
gtk_viewer::tk_draw_arc(bool fill, int x, int y, int w, int h, int as, int ae)
{
#if GTK_CHECK_VERSION(3,0,0)
    v_gc->draw_arc(v_pixmap, fill, x, y, w, h, as, ae);
#else
    gdk_draw_arc(v_pixmap, v_gc, fill, x, y, w, h, as, ae);
#endif
}


//
// Forms handling
//

// Handler callbacks for form widgets

namespace {
    // Handle Enter keypress in text input.
    //
    int
    input_key_hdlr(GtkWidget *widget, GdkEvent *event, htmForm *entry)
    {
        htmFormData *form = entry->parent;
        if (event->key.keyval == GDK_KEY_Return) {
            for (htmForm *e = entry->next; e; e = e->next) {
                if (e->type == FORM_TEXT) {
                    // found next text entry, give it the focus
                    GtkWidget *w = GTK_WIDGET(e->widget);
                    while (gtk_widget_get_parent(w)) {
                        gtk_container_set_focus_child(
                            GTK_CONTAINER(gtk_widget_get_parent(w)), w);
                        w = gtk_widget_get_parent(w);
                    }
                    if (GTK_IS_WINDOW(w))
                        gtk_window_set_focus(GTK_WINDOW(w),
                            GTK_WIDGET(e->widget));
                    return (true);
                }
            }
            for (htmForm *e = form->components; e; e = e->next) {
                if (e->type == FORM_SUBMIT)
                    // form has a submit button, nothing to do
                    return (true);
            }
            gtk_viewer *viewer =
                (gtk_viewer*)g_object_get_data(G_OBJECT(widget), "viewer");
            if (viewer) {
                viewer->formActivate(0, entry);
                return (true);
            }
        }
        return (false);
    }


    // We use checkbox widgets for radio buttons, and handle the logic here.
    //
    void
    radio_change_hdlr(GtkWidget *w, htmForm *entry)
    {
        bool active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

        htmForm *e;
        for (e = entry->parent->components; e; e = e->next) {
            if (e->type == FORM_RADIO && !strcasecmp(e->name, e->name))
                break;
        }
        if (!e)
            return;

        for ( ; e; e = e->next) {
            if (e->type == FORM_RADIO && e != entry) {
                // same group, unset it
                if (!strcasecmp(e->name, entry->name)) {
                    if (active)
                        gtk_toggle_button_set_active(
                            GTK_TOGGLE_BUTTON(e->widget), false);
                    else
                        return;
                }
                else
                    // Not a member of this group, we processed all
                    // elements in this radio box, break out.
                    break;
            }
        }
        if (!active)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(e->widget), true);
    }


    void
    file_selection(const char *fname, void *arg)
    {
        if (fname && *fname && arg) {
            GtkWidget *edit = (GtkWidget*)g_object_get_data(G_OBJECT(arg),
                "lineedit");
            if (edit)
                gtk_entry_set_text(GTK_ENTRY(edit), fname);
        }
    }


    void
    file_selection_destroy(GRfilePopup*, void *arg)
    {
        if (arg) {
            GtkWidget *button =
                (GtkWidget*)g_object_get_data(G_OBJECT(arg),
                "browse");
            if (button)
                g_object_set_data(G_OBJECT(button), "filesel", 0);
        }
    }


    void
    button_press_hdlr(GtkWidget *widget, htmForm *entry)
    {
        gtk_viewer *viewer =
            (gtk_viewer*)g_object_get_data(G_OBJECT(widget), "viewer");
        if (!viewer)
            return;

        if (entry->type == FORM_SUBMIT)
            viewer->formActivate(0, entry);
        else if (entry->type == FORM_RESET)
            viewer->formReset(entry);
        else if (entry->type == FORM_FILE) {
            // pop up/down a file selection window
            GRfilePopup *fs =
                (GRfilePopup*)g_object_get_data(G_OBJECT(widget),
                "filesel");
            if (!fs) {
                fs = new GTKfilePopup(0, fsSEL, entry->widget, 0);
                fs->register_callback(file_selection);
                fs->register_quit_callback(file_selection_destroy);
                g_object_set_data(G_OBJECT(widget), "filesel", fs);
            }
            fs->set_visible(true);
        }
    }
}
// End of form element callbacks


#define PADVAL 16


// Create the appropriate widget(s) for the entry.  The accurate
// width and height must be set/determined here.
//
void
gtk_viewer::tk_add_widget(htmForm *entry, htmForm *parent)
{
    if (entry->type == FORM_IMAGE || entry->type == FORM_HIDDEN)
        return;

    switch (entry->type) {
    // text field, set args and create it
    case FORM_TEXT:
    case FORM_PASSWD:
        {
            GtkWidget *ed = gtk_entry_new();
            if (entry->maxlength != -1)
                gtk_entry_set_max_length(GTK_ENTRY(ed), entry->maxlength);

            entry->width = entry->size * GTKfont::stringWidth(ed, 0) + PADVAL;
            entry->height = GTKfont::stringHeight(ed, 0) + PADVAL;
            gtk_widget_set_size_request(ed, entry->width, entry->height);

            if (entry->type == FORM_TEXT) {
                if (entry->value)
                    // enter key terminates with action
                    gtk_entry_set_text(GTK_ENTRY(ed), entry->value);

                g_object_set_data(G_OBJECT(ed), "viewer", this);
                g_signal_connect(G_OBJECT(ed), "key-press-event",
                    G_CALLBACK(input_key_hdlr), entry);

            }
            // These pass mouse wheel scroll event to main window.
            g_signal_connect(G_OBJECT(ed), "scroll-event",
                G_CALLBACK(v_scroll_event_hdlr), this);
            if (entry->type == FORM_PASSWD)
                gtk_entry_set_visibility(GTK_ENTRY(ed), false);
            entry->widget = ed;
            gtk_widget_show(ed);
            if (!gtk_widget_get_has_window(ed)) {
                GtkWidget *b = gtk_event_box_new();
                gtk_widget_show(b);
                gtk_container_add(GTK_CONTAINER(b), ed);
                add_widget(b);
            }
            else
                add_widget(ed);
        }
        break;

    case FORM_CHECK:
        {
            GtkWidget *cb = gtk_toggle_button_new();
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb),
                entry->checked);

            entry->width = 16;
            entry->height = 16;
            gtk_widget_set_size_request(cb, entry->width, entry->height);

            entry->widget = cb;
            gtk_widget_show(cb);
            if (!gtk_widget_get_has_window(cb)) {
                GtkWidget *b = gtk_event_box_new();
                gtk_widget_show(b);
                gtk_container_add(GTK_CONTAINER(b), cb);
                add_widget(b);
            }
            else
                add_widget(cb);
        }
        break;

    case FORM_RADIO:
        {
            GtkWidget *cb = gtk_toggle_button_new();
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb),
                entry->checked);

            entry->width = 16;
            entry->height = 16;
            gtk_widget_set_size_request(cb, entry->width, entry->height);

            g_object_set_data(G_OBJECT(cb), "viewer", this);
            g_signal_connect(G_OBJECT(cb), "toggled",
                G_CALLBACK(radio_change_hdlr), entry);

            entry->widget = cb;
            gtk_widget_show(cb);
            if (!gtk_widget_get_has_window(cb)) {
                GtkWidget *b = gtk_event_box_new();
                gtk_widget_show(b);
                gtk_container_add(GTK_CONTAINER(b), cb);
                add_widget(b);
            }
            else
                add_widget(cb);
        }
        break;

    case FORM_FILE:
        {
            GtkWidget *hbox = gtk_hbox_new(0, 0);
            gtk_widget_show(hbox);

            GtkWidget *ed = gtk_entry_new();
            if (entry->maxlength != -1)
                gtk_entry_set_max_length(GTK_ENTRY(ed), entry->maxlength);
            gtk_widget_show(ed);
            g_object_set_data(G_OBJECT(hbox), "lineedit", ed);

            int ew = entry->size * GTKfont::stringWidth(ed, 0) + PADVAL;
            int ht = GTKfont::stringHeight(ed, 0) + PADVAL;
            gtk_widget_set_size_request(ed, ew, ht);

            const char *lab = entry->value ? entry->value : "Browse...";
            GtkWidget *button = gtk_button_new_with_label(entry->value ?
                entry->value : "Browse...");
            gtk_widget_show(button);
            int bw = GTKfont::stringWidth(button, lab) + PADVAL;
            gtk_widget_set_size_request(button, bw, ht);
            g_object_set_data(G_OBJECT(hbox), "browse", button);

            entry->width = ew + bw + PADVAL;
            entry->height = ht;

            // Pack and connect stuff
            gtk_box_pack_start(GTK_BOX(hbox), ed, 0, 0, 0);
            gtk_box_pack_start(GTK_BOX(hbox), button, 0, 0, 0);

            g_object_set_data(G_OBJECT(button), "viewer", this);
            g_signal_connect(G_OBJECT(button), "clicked",
                G_CALLBACK(button_press_hdlr), entry);

            entry->widget = hbox;
            if (!gtk_widget_get_has_window(hbox)) {
                GtkWidget *b = gtk_event_box_new();
                gtk_widget_show(b);
                gtk_container_add(GTK_CONTAINER(b), hbox);
                add_widget(b);
            }
            else
                add_widget(hbox);
        }
        break;

    case FORM_RESET:
    case FORM_SUBMIT:
        {
            const char *lab = entry->value ? entry->value : entry->name;
            if (!lab)
                lab = " ";
            GtkWidget *cb = gtk_button_new_with_label(lab);
            entry->width = GTKfont::stringWidth(cb, lab) + PADVAL;
            entry->height = GTKfont::stringHeight(cb, 0) + PADVAL;
            gtk_widget_set_size_request(cb, entry->width, entry->height);

            g_object_set_data(G_OBJECT(cb), "viewer", this);
            g_signal_connect(G_OBJECT(cb), "clicked",
                G_CALLBACK(button_press_hdlr), entry);

            entry->widget = cb;
            gtk_widget_show(cb);
            if (!gtk_widget_get_has_window(cb)) {
                GtkWidget *b = gtk_event_box_new();
                gtk_widget_show(b);
                gtk_container_add(GTK_CONTAINER(b), cb);
                add_widget(b);
            }
            else
                add_widget(cb);
        }
        break;

    case FORM_SELECT:
        if (entry->multiple || entry->size > 1) {
            // multiple select or more than one item visible: it's a listbox
            GtkWidget *scrolled_win = gtk_scrolled_window_new(0, 0);
            gtk_widget_show(scrolled_win);
#ifdef NEW_LIS
            GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
            GtkWidget *list =
                gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
            gtk_tree_view_set_enable_search(GTK_TREE_VIEW(list), false);
            GtkTreeViewColumn *tvcol = gtk_tree_view_column_new();
            gtk_tree_view_append_column(GTK_TREE_VIEW(list), tvcol);
            GtkCellRenderer *rnd = gtk_cell_renderer_text_new();
            gtk_tree_view_column_pack_start(tvcol, rnd, true);
            gtk_tree_view_column_add_attribute(tvcol, rnd, "text", 0);
            GtkTreeSelection *sel =
                gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
            gtk_scrolled_window_add_with_viewport(
                GTK_SCROLLED_WINDOW(scrolled_win), list);
#else
#ifdef XXX_DEPREC
            GtkWidget *list = gtk_list_new();
            gtk_scrolled_window_add_with_viewport(
                GTK_SCROLLED_WINDOW(scrolled_win), list);
#endif
#endif
            gtk_widget_show(GTK_WIDGET(list));
            g_object_set_data(G_OBJECT(scrolled_win), "user", list);
            gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
            if (entry->multiple)
#ifdef NEW_LIS
#else
#ifdef XXX_DEPREC
                gtk_list_set_selection_mode(GTK_LIST(list),
                    GTK_SELECTION_MULTIPLE);
#endif
#endif

            entry->widget = scrolled_win;
            gtk_widget_show(scrolled_win);
            if (!gtk_widget_get_has_window(scrolled_win)) {
                GtkWidget *b = gtk_event_box_new();
                gtk_widget_show(b);
                gtk_container_add(GTK_CONTAINER(b), scrolled_win);
                add_widget(b);
            }
            else
                add_widget(scrolled_win);
        }
        else {
#ifdef NEW_OPT
            GtkWidget *option_menu = gtk_combo_box_text_new();
            entry->widget = option_menu;
            gtk_widget_show(option_menu);
            add_widget(option_menu);
#else
#ifdef XXX_DEPREC
            GtkWidget *option_menu = gtk_option_menu_new();
            GtkWidget *menu = gtk_menu_new();
            gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
            entry->widget = option_menu;
            gtk_widget_show(option_menu);
            add_widget(option_menu);
#endif
#endif
        }
        break;

    case FORM_OPTION:
        if (parent->multiple || parent->size > 1) {
            // scrolled_win/viewport/list
#ifdef NEW_LIS
#else
#ifdef XXX_DEPREC
            GtkList *list =
                GTK_LIST(GTK_BIN(GTK_BIN(parent->widget)->child)->child);
            // append item to bottom of list
            GtkWidget *listitem =
                gtk_list_item_new_with_label(entry->name);
            gtk_widget_show(listitem);
            gtk_container_add(GTK_CONTAINER(list), listitem);
            entry->widget = listitem;
#endif
#endif
        }
        else {
#ifdef NEW_OPT
            GtkComboBoxText *txw = GTK_COMBO_BOX_TEXT(parent->widget);
            if (txw)
                gtk_combo_box_text_append_text(txw, entry->name);
#else
#ifdef XXX_DEPREC
            GtkWidget *menu = gtk_option_menu_get_menu(
                GTK_OPTION_MENU(parent->widget));
            GtkWidget *menu_entry =
                gtk_menu_item_new_with_label(entry->name);
            gtk_widget_show(menu_entry);
            gtk_menu_append(GTK_MENU(menu), menu_entry);
            entry->widget = menu_entry;
#endif
#endif
        }
        break;

    case FORM_TEXTAREA:
        {

            GtkWidget *frame = 0;
            frame = gtk_frame_new(0);
            gtk_widget_show(frame);

            GtkWidget *textw = gtk_text_view_new();
            gtk_widget_show(textw);
            gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textw), GTK_WRAP_NONE);
            gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textw), 4);
            gtk_text_view_set_editable(GTK_TEXT_VIEW(textw), TRUE);
            gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textw), TRUE);

            gtk_container_add(GTK_CONTAINER(frame), textw);

            if (entry->value)
                text_set_chars(textw, entry->value);

            entry->width = entry->size * GTKfont::stringWidth(textw, 0) +
                PADVAL;
            entry->height =
                entry->maxlength * GTKfont::stringHeight(textw, 0) + PADVAL;
            gtk_widget_set_size_request(frame ? frame : textw, entry->width,
                entry->height);

            entry->widget = textw;
            GtkWidget *w = frame ? frame : textw;
            if (!gtk_widget_get_has_window(w)) {
                GtkWidget *b = gtk_event_box_new();
                gtk_widget_show(b);
                gtk_container_add(GTK_CONTAINER(b), w);
                add_widget(b);
            }
            else
                add_widget(w);
        }
        break;

    default:
        break;
    }
}


// Take care of sizing FORM_SELECT menus.
//
void
gtk_viewer::tk_select_close(htmForm *entry)
{
    if (entry->multiple || entry->size > 1) {
        // scrolled_win/viewport/list
#ifdef NEW_LIS
#else
#ifdef XXX_DEPREC
        GtkList *list =
            GTK_LIST(GTK_BIN(GTK_BIN(entry->widget)->child)->child);
        int ht = 0;
        GtkRequisition req;
        if (list->children) {
            gtk_widget_size_request(GTK_WIDGET(list->children->data), &req);
            ht = req.height * entry->size + PADVAL;
        }
        if (ht < 40)
            ht = 40;
        gtk_widget_size_request(GTK_BIN(GTK_BIN(entry->widget)->child)->child,
            &req);
        if (ht < req.height)
            req.width += 22;  // scrollbar compensation
        entry->width = req.width + PADVAL;
        entry->height = ht;
        gtk_widget_set_size_request(GTK_WIDGET(entry->widget), entry->width,
            entry->height);
#endif
#endif

        // set initial selections
        int cnt = 0;
        for (htmForm *e = entry->options; e; e = e->next, cnt++) {
#ifdef NEW_LIS
#else
#ifdef XXX_DEPREC
            if (e->selected)
                gtk_list_select_item(list, cnt);
#endif
#endif
        }
    }
    else {
        GtkWidget *option_menu = GTK_WIDGET(entry->widget);
        // the option menu always has a selection
        int cnt = 0;
        htmForm *e;
        for (e = entry->options; e; e = e->next) {
            if (e->selected)
                break;
            cnt++;
        }
        if (!e && entry->options) {
            entry->options->selected = true;
            cnt = 0;
        }
#ifdef NEW_OPT
#else
#ifdef XXX_DEPREC
        GtkWidget *menu =
            gtk_option_menu_get_menu(GTK_OPTION_MENU(option_menu));
        GtkRequisition req;
        gtk_widget_size_request(menu, &req);
        entry->width = req.width + 24;
        entry->height = GTKfont::stringHeight(option_menu, 0) + PADVAL;
        gtk_widget_set_size_request(GTK_WIDGET(entry->widget), entry->width,
            entry->height);
        gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), cnt);
#endif
#endif
    }
}


// Return the text entered into the text entry area.
//
char *
gtk_viewer::tk_get_text(htmForm *entry)
{
    switch (entry->type) {
    case FORM_PASSWD:
    case FORM_TEXT:
        return (lstring::copy(gtk_entry_get_text(GTK_ENTRY(entry->widget))));
        break;

    case FORM_FILE:
        {
            GtkWidget *form = GTK_WIDGET(entry->widget);
            if (form) {
                GtkWidget *ed = (GtkWidget*)
                    g_object_get_data(G_OBJECT(form), "lineedit");
                if (ed)
                    return (lstring::copy(gtk_entry_get_text(GTK_ENTRY(ed))));
            }
        }
        break;

    case FORM_TEXTAREA:
        return (text_get_chars(GTK_WIDGET(entry->widget), 0, -1));

    default:
        break;
    }
    return (0);
}


// Set the text in the text entry area.
//
void
gtk_viewer::tk_set_text(htmForm *entry, const char *string)
{
    switch (entry->type) {
    case FORM_PASSWD:
    case FORM_TEXT:
        gtk_entry_set_text(GTK_ENTRY(entry->widget), string);
        break;

    case FORM_FILE:
        {
            GtkWidget *form = GTK_WIDGET(entry->widget);
            if (form) {
                GtkWidget *ed = (GtkWidget*)
                    g_object_get_data(G_OBJECT(form), "lineedit");
                if (ed)
                    gtk_entry_set_text(GTK_ENTRY(ed), string);
            }
        }
        break;

    case FORM_TEXTAREA:
        text_set_chars(GTK_WIDGET(entry->widget), string);
        break;

    default:
        break;
    }
}


// Return the state of the check box, or for a FORM_SELECT, set the
// checked field in the entries of seleted items.
//
bool
gtk_viewer::tk_get_checked(htmForm *entry)
{
    switch (entry->type) {
    case FORM_CHECK:
    case FORM_RADIO:
        return (gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(entry->widget)));
        break;
    case FORM_SELECT:
        if (entry->multiple || entry->size > 1) {
            for (htmForm *e = entry->options; e; e = e->next) {
                e->checked =
                    (gtk_widget_get_state(GTK_WIDGET(e->widget)) ==
                    GTK_STATE_SELECTED);
            }
        }
        else {
#ifdef NEW_OPT
#else
#ifdef XXX_DEPREC
            GtkWidget *menu =
                gtk_option_menu_get_menu(GTK_OPTION_MENU(entry->widget));
            GtkWidget *active = gtk_menu_get_active(GTK_MENU(menu));
            for (htmForm *e = entry->options; e; e = e->next)
                e->checked = (GTK_WIDGET(e->widget) == active);
#endif
#endif
        }
        break;
    default:
        break;
    }
    return (false);
}


// Set the state of the check box, or for a FORM_SELECT, set the
// state of each entry according to the selected field.
//
void
gtk_viewer::tk_set_checked(htmForm *entry)
{
    switch (entry->type) {
    case FORM_CHECK:
    case FORM_RADIO:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(entry->widget),
            entry->checked);
        break;
    case FORM_SELECT:
        if (entry->multiple || entry->size > 1) {
            for (htmForm *e = entry->options; e; e = e->next) {
#ifdef NEW_LIS
#else
#ifdef XXX_DEPREC
                if (e->selected)
                    gtk_list_item_select(GTK_LIST_ITEM(e->widget));
                else
                    gtk_list_item_deselect(GTK_LIST_ITEM(e->widget));
#endif
#endif
            }
        }
        else {
            int cnt = 0;
            for (htmForm *e = entry->options; e; e = e->next) {
                if (e->selected) {
#ifdef NEW_OPT
                    gtk_combo_box_set_active(GTK_COMBO_BOX(entry->widget),
                       cnt);
#else
#ifdef XXX_DEPREC
                    gtk_option_menu_set_history(GTK_OPTION_MENU(entry->widget),
                        cnt);
#endif
#endif
                    break;
                }
            }
        }
        break;
    default:
        break;
    }
}


// Destroy the form widget, this removes it from the screen.
//
void
gtk_viewer::tk_form_destroy(htmForm *entry)
{
    if (entry->widget) {
        GtkWidget *w = GTK_WIDGET(entry->widget);
        entry->widget = 0;
        if (entry->type == FORM_FILE) {
            GRfilePopup *fs =
                (GRfilePopup*)g_object_get_data(G_OBJECT(w), "filesel");
            if (fs)
                // The popdown method won't work since there is no parent
                // container.
                delete fs;
        }
        // Find the ancestor parented by GtkFixed.
        while (gtk_widget_get_parent(w) &&
                !GTK_IS_FIXED(gtk_widget_get_parent(w)))
            w = gtk_widget_get_parent(w);
        if (!gtk_widget_get_parent(w))
            // sanity
            return;
        gtk_widget_destroy(w);
    }
}


// Set the widget locations and visibility.
//
void
gtk_viewer::tk_position_and_show(htmForm *entry, bool show)
{
    if (entry->widget) {
        GtkWidget *w = GTK_WIDGET(entry->widget);

        // Find the ancestor parented by GtkFixed.
        while (gtk_widget_get_parent(w) &&
                !GTK_IS_FIXED(gtk_widget_get_parent(w)))
            w = gtk_widget_get_parent(w);
        if (!gtk_widget_get_parent(w))
            // sanity
            return;

        if (show) {
            if (entry->mapped)
                gtk_widget_show(w);
        }
        else {
            gtk_widget_hide(w);
            if (htm_viewarea.intersect(entry->data->area)) {
                int xv = viewportX(entry->data->area.x);
                int yv = viewportY(entry->data->area.y);
                gtk_fixed_move(GTK_FIXED(v_fixed), w, xv, yv);
                entry->mapped = true;
            }
            else
                entry->mapped = false;
        }
    }
}
// End of htmInterface functions


//-----------------------------------------------------------------------------
// Event handlers

// Erase/draw the bounding rectangle for selecting text, called from the
// motion handler.
//
void
gtk_viewer::show_selection_box(int x, int y, bool nodraw)
{
    if (!v_btn_dn)
        return;

    // erase box sides
    int xx = v_last_x < v_btn_x ? v_last_x : v_btn_x;
    int yy = v_last_y < v_btn_y ? v_last_y : v_btn_y;
    int w = abs(v_last_x - v_btn_x);
    int h = abs(v_last_y - v_btn_y);
    tk_refresh_area(xx-1, yy - 1, 2, h + 2);
    tk_refresh_area(xx + w-1, yy - 1, 2, h + 2);
    tk_refresh_area(xx, yy-1, w, 2);
    tk_refresh_area(xx, yy + h-1, w, 2);

    if (!nodraw) {
        xx = x < v_btn_x ? x : v_btn_x;
        yy = y < v_btn_y ? y : v_btn_y;
        w = abs(x - v_btn_x);
        h = abs(y - v_btn_y);
        v_last_x = x;
        v_last_y = y;

        // random color
        randval rnd;
        rnd.rand_seed(time(0));
        int j = rnd.rand_value();
        tk_set_foreground(j);
#if GTK_CHECK_VERSION(3,0,0)
        v_gc->draw_rectangle(gtk_widget_get_window(v_draw_area),
            false, xx, yy, w, h);
#else
        gdk_draw_rectangle(gtk_widget_get_window(v_draw_area),
            v_gc, false, xx, yy, w, h);
#endif
    }
}


#if GTK_CHECK_VERSION(3,0,0)

bool
gtk_viewer::expose_event_handler(cairo_t *cr)
{
    cairo_rectangle_int_t rect;
    ndkDrawable::redraw_area(cr, &rect);
    setReady();
    v_pixmap->copy_to_window(gtk_widget_get_window(v_draw_area), v_gc,
        rect.x, rect.y, rect.x, rect.y, rect.width, rect.height);
    v_seen_expose = true;
    return (true);
}


#else

bool
gtk_viewer::expose_event_handler(GdkEventExpose *event)
{
    setReady();
    gdk_window_copy_area(gtk_widget_get_window(v_draw_area), v_gc,
        event->area.x, event->area.y, v_pixmap, event->area.x, event->area.y,
        event->area.width, event->area.height);
    v_seen_expose = true;
    return (true);
}

#endif


bool
gtk_viewer::canvas_resize_handler(GtkAllocation *a)
{
#ifdef NEW_SC
fprintf(stderr,  "%d %d %d %d %d %d\n", v_da_width, v_da_height,
htm_viewarea.width, htm_viewarea.height, a->width, a->height);
//if (a->width < v_da_width)
//    a->width = v_da_width;
if (a->height < v_da_height)
//    a->height = v_da_height > 1500 ? v_da_height : 1500;
a->height = v_da_height;
//a->height = 1900;
#endif
    if (a->width != v_width || a->height != v_height) {
        v_width = a->width;
        v_height = a->height;
#ifdef NEW_SC
fprintf(stderr, "new %d %d\n", v_width, v_height);
#endif

/*XXXYYY
        if (!gtk_widget_get_window(v_draw_area)) {
            gtk_widget_set_size_request(v_draw_area, v_width, v_height);
            return true;
        }
*/

#if GTK_CHECK_VERSION(3,0,0)
        ndkPixmap *pm = (ndkPixmap*)tk_new_pixmap(v_width, v_height);
        v_pixmap->dec_ref();
        v_pixmap = pm;
#else
        GdkPixmap *pm = (GdkPixmap*)tk_new_pixmap(v_width, v_height);
        gdk_pixmap_unref(v_pixmap);
        v_pixmap = pm;
#endif
        // GtkFixed doesn't resize children so have to do it ourselves.
// XXX Check this!  the allocation branch screws up in GTK2 causing a
// screen mess when expanding the widget.  The set_size_request call,
// if I remember correctly, becomes a minimum, and the window can't be
// resized smaller in GTK3.
#if GTK_CHECK_VERSION(3,0,0)
        GtkAllocation a;
        a.x = 0;
        a.y = 0;
        a.width = v_width;
        a.height = v_height;
        gtk_widget_size_allocate(v_draw_area, &a);
#else
        gtk_widget_set_size_request(v_draw_area, v_width, v_height);
#endif

#ifdef NEW_SC
gtk_widget_size_allocate(v_fixed, &a);
gtk_widget_size_allocate(gtk_widget_get_parent(v_fixed), &a);

fprintf(stderr, "sz %d %d\n",
gdk_window_get_width(gtk_widget_get_window(v_fixed)),
gdk_window_get_height(gtk_widget_get_window(v_fixed)));

fprintf(stderr, "adj %g %g\n", gtk_adjustment_get_upper(ah),
gtk_adjustment_get_upper(av));

//gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(v_scrwin),
//    gdk_window_get_height(gtk_widget_get_window(v_fixed)));
//gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(v_scrwin),
//    gdk_window_get_height(gtk_widget_get_window(v_fixed)));
gtk_adjustment_set_upper(av, 
    gdk_window_get_height(gtk_widget_get_window(v_fixed)));
fprintf(stderr, "adj %g %g\n", gtk_adjustment_get_upper(ah),
gtk_adjustment_get_upper(av));

GtkWidget *vsb = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(v_scrwin));
gtk_range_set_range(GTK_RANGE(vsb), 0.0, 
    gdk_window_get_height(gtk_widget_get_window(v_fixed)));
#endif

        int x = htm_viewarea.x;
        int y = htm_viewarea.y;
        htmWidget::resize();
#ifndef WIN32
        // This hangs up badly in MSW for some reason.
        if (gtk_widget_get_realized(v_draw_area)) {
            // Flush the redraw queue.  If this isn't done, scrollbars
            // sometimes leave gray areas.
            do {
                gtk_main_iteration_do(false);
            } while (gtk_events_pending());
        }
#endif
        set_scroll_position(x, true);
        set_scroll_position(y, false);
        if (!htm_in_layout && htm_initialized)
            trySync();
    }
    return (true);
}


#ifdef NEW_SC

bool
gtk_viewer::form_resize_handler(GtkAllocation*)
{
    int x = htm_viewarea.x;
    int y = htm_viewarea.y;
    htmWidget::resize();
#ifndef WIN32
    // This hangs up badly in MSW for some reason.
    if (gtk_widget_get_realized(v_draw_area)) {
        // Flush the redraw queue.  If this isn't done, scrollbars
        // sometimes leave gray areas.
        do {
            gtk_main_iteration_do(false);
        } while (gtk_events_pending());
    }
#endif
    set_scroll_position(x, true);
    set_scroll_position(y, false);
    if (!htm_in_layout && htm_initialized)
        trySync();
    return (true);
}

#endif


bool
gtk_viewer::motion_event_handler(GdkEventMotion *event)
{
    int x = (int)event->x + scroll_position(true);
    int y = (int)event->y + scroll_position(false);
    anchorTrack(event, x, y);
    show_selection_box((int)event->x, (int)event->y, false);
    return (true);
}


bool
gtk_viewer::button_down_handler(GdkEventButton *event)
{
    if (event->button == 1 || event->button == 2 || event->button == 3) {
        v_btn_dn = true;
        v_btn_x = (int)event->x;
        v_btn_y = (int)event->y;
        v_last_x = v_btn_x;
        v_last_y = v_btn_y;
        int x = v_btn_x + scroll_position(true);
        int y = v_btn_y + scroll_position(false);
        extendStart((htmEvent*)event, event->button, x, y);
        return (true);
    }
    return (false);
}


bool
gtk_viewer::button_up_handler(GdkEventButton *event)
{
    if (event->button == 1 || event->button == 2 || event->button == 3) {
        int x = (int)event->x + scroll_position(true);
        int y = (int)event->y + scroll_position(false);
        show_selection_box(0, 0, true);
        v_btn_dn = false;
        extendEnd((htmEvent*)event, event->button, true, x, y);
        return (true);
    }
    return (false);
}


void
gtk_viewer::vsb_change_handler(int newval)
{
    int oldval = htm_viewarea.y;
    htm_viewarea.y = newval;

    // move form components
    for (htmFormData *form = htm_form_data; form; form = form->next) {
        for (htmForm *entry = form->components; entry; entry = entry->next)
            tk_position_and_show(entry, false);
    }
    if (newval > oldval) {
        // scroll down
        int delta = newval - oldval;
        if (delta < v_height) {
#if GTK_CHECK_VERSION(3,0,0)
            v_pixmap->copy_to_pixmap(v_pixmap, v_gc, 0, delta, 0, 0,
                v_width, v_height - delta);
#else
            gdk_window_copy_area(v_pixmap, v_gc, 0, 0, v_pixmap,
                0, delta, v_width, v_height - delta);
#endif
            htmWidget::repaint(htm_viewarea.x, newval + v_height - delta - 1,
                v_width, delta + 2);
            tk_refresh_area(0, 0, v_width, v_height);
        }
        else
            htmWidget::repaint(htm_viewarea.x, newval, v_width, v_height);
    }
    else if (newval < oldval) {
        // scroll up
        int delta = oldval - newval;
        if (delta < v_height) {
#if GTK_CHECK_VERSION(3,0,0)
            v_pixmap->copy_to_pixmap(v_pixmap, v_gc, 0, 0, 0, delta,
                v_width, v_height - delta);
#else
            gdk_window_copy_area(v_pixmap, v_gc, 0, delta, v_pixmap,
                0, 0, v_width, v_height - delta);
#endif
            htmWidget::repaint(htm_viewarea.x, newval - 1, v_width,
                delta + 2);
            tk_refresh_area(0, 0, v_width, v_height);
        }
        else
            htmWidget::repaint(htm_viewarea.x, newval, v_width, v_height);
    }
    for (htmFormData *form = htm_form_data; form; form = form->next) {
        for (htmForm *entry = form->components; entry; entry = entry->next)
            tk_position_and_show(entry, true);
    }
}


void
gtk_viewer::hsb_change_handler(int newval)
{
    int oldval = htm_viewarea.x;
    htm_viewarea.x = newval;

    // move form components
    for (htmFormData *form = htm_form_data; form; form = form->next) {
        for (htmForm *entry = form->components; entry; entry = entry->next)
            tk_position_and_show(entry, false);
    }
    if (newval > oldval) {
        // scroll right
        int delta = newval - oldval;
        if (delta < v_width) {
#if GTK_CHECK_VERSION(3,0,0)
            v_pixmap->copy_to_pixmap(v_pixmap, v_gc, delta, 0, 0, 0,
                v_width - delta, v_height);
#else
            gdk_window_copy_area(v_pixmap, v_gc, 0, 0, v_pixmap,
                delta, 0, v_width - delta, v_height);
#endif
            htmWidget::repaint(newval + v_width - delta - 1, htm_viewarea.y,
                delta + 2, v_height);
            tk_refresh_area(0, 0, v_width, v_height);
        }
        else
            htmWidget::repaint(newval, htm_viewarea.y, v_width, v_height);
    }
    else if (newval < oldval) {
        // scroll left
        int delta = oldval - newval;
        if (delta < v_width) {
#if GTK_CHECK_VERSION(3,0,0)
            v_pixmap->copy_to_pixmap(v_pixmap, v_gc, 0, 0, delta, 0,
                v_width - delta, v_height);
#else
            gdk_window_copy_area(v_pixmap, v_gc, delta, 0, v_pixmap,
                0, 0, v_width - delta, v_height);
#endif
            htmWidget::repaint(newval - 1, htm_viewarea.y, delta + 2,
                v_height);
            tk_refresh_area(0, 0, v_width, v_height);
        }
        else
            htmWidget::repaint(newval, htm_viewarea.y, v_width, v_height);
    }
    for (htmFormData *form = htm_form_data; form; form = form->next) {
        for (htmForm *entry = form->components; entry; entry = entry->next)
            tk_position_and_show(entry, true);
    }
}


bool
gtk_viewer::selection_clear_handler(GtkWidget *widget,
    GdkEventSelection *event)
{
    if (event->selection == GDK_SELECTION_PRIMARY) {
        if (htm_text_selection)
            selectRegion(0, 0, 0, 0);
    }
    return (false);
}


void
gtk_viewer::selection_get_handler(GtkWidget*,
    GtkSelectionData *data, unsigned int info, unsigned int)
{
    if (gtk_selection_data_get_selection(data) != GDK_SELECTION_PRIMARY)
        return;

    const char *str = htm_text_selection;
    if (!str)
        return;  // refuse
    int length = strlen(str);

    if (info == TARGET_STRING) {
        gtk_selection_data_set(data, GDK_SELECTION_TYPE_STRING,
            8*sizeof(char), (unsigned char*)str, length);
    }
#ifdef HAVE_X11
    else if (info == TARGET_TEXT || info == TARGET_COMPOUND_TEXT) {

        GdkAtom encoding;
        int fmtnum;
        unsigned char *text;
        int new_length;
        gdk_x11_display_string_to_compound_text(gdk_display_get_default(),
            str, &encoding, &fmtnum, &text, &new_length);
        // Strange problem here in gtk2 on RHEL5, some text returns 0
        // here that causes error message and erroneous transfer.
        if (fmtnum == 0)
            fmtnum = 8;
        gtk_selection_data_set(data, encoding, fmtnum, text,
            new_length);
        gdk_x11_free_compound_text(text);
    }
#endif
}


void
gtk_viewer::font_change_handler(int indx)
{
    if (indx == FNT_MOZY) {

        const char *fontname = FC.getName(FNT_MOZY);
        int position = scroll_position(false);
        set_font(fontname);
        set_scroll_position(position, false);

        const char *fixedname = FC.getName(FNT_MOZY_FIXED);
        char *family;
        int sz1, sz2;
        FC.parse_freeform_font_string(fontname, &family, 0, &sz1, 0);
        delete [] family;
        FC.parse_freeform_font_string(fixedname, &family, 0, &sz2, 0);
        if (sz1 != sz2) {
            char buf[256];
            sprintf(buf, "%s %d", family, sz1);
            FC.setName(buf, FNT_MOZY_FIXED);
            GTKfontPopup::update_all(FNT_MOZY_FIXED);
        }
        delete [] family;
    }
    else if (indx == FNT_MOZY_FIXED) {

        int position = scroll_position(false);
        set_fixed_font(FC.getName(FNT_MOZY_FIXED));
        set_scroll_position(position, false);
    }
}


// Gtk2 mouse wheel handler, send the event to the vertical scroll
// bar.
//
int
gtk_viewer::v_scroll_event_hdlr(GtkWidget*, GdkEvent *event, void *vp)
{
#if GTK_CHECK_VERSION(3,0,0)
#ifdef __APPLE__
    GdkEventScroll *sev = (GdkEventScroll*)event;
    if (sev->direction != GDK_SCROLL_SMOOTH)
        return (false);
#endif
#endif
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v && gtk_widget_get_mapped(v->v_vsb))
        gtk_propagate_event(v->v_vsb, event);
    return (true);
}


//-----------------------------------------------------------------------------
// Event callbacks

// Private static GTK signal handler.
//
#if GTK_CHECK_VERSION(3,0,0)
int
gtk_viewer::v_expose_event_hdlr(GtkWidget*, cairo_t *cr, void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v)
        return (v->expose_event_handler(cr));
    return (0);
}

#else

int
gtk_viewer::v_expose_event_hdlr(GtkWidget*, GdkEvent *event, void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v)
        return (v->expose_event_handler((GdkEventExpose*)event));
    return (0);
}
#endif


// Private static GTK signal handler.
//
void
gtk_viewer::v_resize_hdlr(GtkWidget *w, GtkAllocation *a, void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v) {
#ifdef NEW_SC
        if (v->v_fixed == w)
            v->canvas_resize_handler(a);
        else if (v->v_form == w)
            v->form_resize_handler(a);
#else
        v->canvas_resize_handler(a);
#endif
    }
}


// Private static GTK signal handler.
//
int
gtk_viewer::v_motion_event_hdlr(GtkWidget*, GdkEvent *event, void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v)
        return (v->motion_event_handler((GdkEventMotion*)event));
    return (0);
}


// Private static GTK signal handler.
//
int
gtk_viewer::v_button_dn_hdlr(GtkWidget*, GdkEvent *event, void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v)
        return (v->button_down_handler((GdkEventButton*)event));
    return (0);
}


// Private static GTK signal handler.
//
int
gtk_viewer::v_button_up_hdlr(GtkWidget*, GdkEvent *event, void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v)
        return (v->button_up_handler((GdkEventButton*)event));
    return (0);
}


// Private static GTK signal handler.
//
int
gtk_viewer::v_key_dn_hdlr(GtkWidget*, GdkEvent *event, void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (!v)
        return (0);
    GdkEventKey *kev = (GdkEventKey*)event;
    if (kev->keyval == GDK_KEY_Down || kev->keyval == GDK_KEY_Up) {
        if (v->v_vsb && v->v_vsba) {
            GtkAdjustment *adj = (GtkAdjustment*)v->v_vsba;
            double nval = gtk_adjustment_get_value(adj) +
                ((kev->keyval == GDK_KEY_Up) ?
                    -gtk_adjustment_get_page_increment(adj) / 4 :
                    gtk_adjustment_get_page_increment(adj) / 4);
            if (nval < gtk_adjustment_get_lower(adj))
                nval = gtk_adjustment_get_lower(adj);
            else if (nval > gtk_adjustment_get_upper(adj) -
                    gtk_adjustment_get_page_size(adj)) {
                nval = gtk_adjustment_get_upper(adj) -
                    gtk_adjustment_get_page_size(adj);
                    }
            gtk_adjustment_set_value(adj, nval);
        }
        return (1);
    }
    if (kev->keyval == GDK_KEY_Next || kev->keyval == GDK_KEY_Prior) {
        if (v->v_vsb && v->v_vsba) {
            GtkAdjustment *adj = (GtkAdjustment*)v->v_vsba;
            double nval = gtk_adjustment_get_value(adj) +
                ((kev->keyval == GDK_KEY_Prior) ?
                    -gtk_adjustment_get_page_increment(adj) :
                    gtk_adjustment_get_page_increment(adj));
            if (nval < gtk_adjustment_get_lower(adj))
                nval = gtk_adjustment_get_lower(adj);
            else if (nval > gtk_adjustment_get_upper(adj) -
                    gtk_adjustment_get_page_size(adj)) {
                nval = gtk_adjustment_get_upper(adj) -
                    gtk_adjustment_get_page_size(adj);
            }
            gtk_adjustment_set_value(adj, nval);
        }
        return (1);
    }
    if (kev->keyval == GDK_KEY_Home) {
        if (v->v_vsb && v->v_vsba) {
            GtkAdjustment *adj = (GtkAdjustment*)v->v_vsba;
            gtk_adjustment_set_value(adj, gtk_adjustment_get_lower(adj));
        }
        return (1);
    }
    if (kev->keyval == GDK_KEY_End) {
        if (v->v_vsb && v->v_vsba) {
            GtkAdjustment *adj = (GtkAdjustment*)v->v_vsba;
            gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj) -
                gtk_adjustment_get_page_size(adj));
        }
        return (1);
    }
    if (kev->keyval == GDK_KEY_Left || kev->keyval == GDK_KEY_Right) {
        if (v->v_hsb && v->v_hsba) {
            GtkAdjustment *adj = (GtkAdjustment*)v->v_hsba;
            double nval = gtk_adjustment_get_value(adj) +
                ((kev->keyval == GDK_KEY_Left) ?
                    -gtk_adjustment_get_page_increment(adj) / 4 :
                    gtk_adjustment_get_page_increment(adj) / 4);
            if (nval < gtk_adjustment_get_lower(adj))
                nval = gtk_adjustment_get_lower(adj);
            else if (nval > gtk_adjustment_get_upper(adj) -
                    gtk_adjustment_get_page_size(adj)) {
                nval = gtk_adjustment_get_upper(adj) -
                    gtk_adjustment_get_page_size(adj);
            }
            gtk_adjustment_set_value(adj, nval);
        }
        return (1);
    }
    return (0);
}


// Private static GTK signal handler.
// Handle focus_in, focus_out, leave_notify, enter_notify.
//
int
gtk_viewer::v_focus_event_hdlr(GtkWidget *widget, GdkEvent *event, void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (!v)
        return (0);
    if (event->any.window != gtk_widget_get_window(widget))
        return (0);

    if (event->type == GDK_ENTER_NOTIFY) {
        gtk_widget_set_can_focus(v->v_draw_area, true);
        GtkWidget *w = v->v_draw_area;
        while (gtk_widget_get_parent(w))
            w = gtk_widget_get_parent(w);
        gtk_window_set_focus(GTK_WINDOW(w), v->v_draw_area);
        return (1);
    }

    if (event->type == GDK_FOCUS_CHANGE) {
        if (gtk_widget_get_window(widget) ==
                gtk_widget_get_window(v->v_draw_area))
            // focussing in
            return (1);
    }

    // losing focus
    v->anchorTrack(event, 0, 0);
    if (v->htm_highlight_on_enter && v->htm_armed_anchor)
        v->leaveAnchor();
    v->htm_armed_anchor = 0;
    v->htm_anchor_current_cursor_element = 0;
    return (1);
}


// Private static GTK signal handler.
//
void
gtk_viewer::v_vsb_change_hdlr(GtkAdjustment *a, void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v)
        v->vsb_change_handler((int)gtk_adjustment_get_value(a));
}


// Private static GTK signal handler.
//
void
gtk_viewer::v_hsb_change_hdlr(GtkAdjustment *a, void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v)
        v->hsb_change_handler((int)gtk_adjustment_get_value(a));
}


// Private static GTK signal handler.
//
int
gtk_viewer::v_selection_clear_hdlr(GtkWidget *widget, GdkEventSelection *event,
    void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v)
        return (v->selection_clear_handler(widget, event));
    return (0);
}


// Private static GTK signal handler.
//
void
gtk_viewer::v_selection_get_hdlr(GtkWidget *widget,
    GtkSelectionData *selection_data, unsigned int info, unsigned int time,
    void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v)
        v->selection_get_handler(widget, selection_data, info, time);
}


// Static function.
// This can be called recursively from the handler, so beware.  The
// last_index_for_update() provides actionable output only on the
// first call following a font change.
//
void
gtk_viewer::v_font_change_hdlr(GtkWidget*, void*, void *a2)
{
    gtk_viewer *w = (gtk_viewer*)a2;
    if (w) {
        GTKfont *fc = dynamic_cast<GTKfont*>(&FC);
        if (fc)
            w->font_change_handler(fc->last_index_for_update());
    }
}

