 
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id: gtkviewer.cc,v 2.70 2016/02/07 20:28:33 stevew Exp $
 *========================================================================*/

// This enables use of pango font rendering in GTK2, must be defined
// ahead of pango includes.
#define PANGO_ENABLE_BACKEND

#include "gtkinterf.h"
#include "gtkviewer.h"
#include "gtkfile.h"
#include "gtkfont.h"
#include "help/help_startup.h"
#include "htm/htm_font.h"
#include "htm/htm_image.h"
#include "htm/htm_form.h"
#include "htm/htm_format.h"
#include "pathlist.h"
#include "randval.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <gdk/gdkprivate.h>
#include <gdk/gdkkeysyms.h>


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

    v_fixed = gtk_fixed_new();
    gtk_widget_show(v_fixed);
    // Without this, form widgets will render outside of the drawing
    // area.  This clips them to the drawing area.
    gtk_fixed_set_has_window(GTK_FIXED(v_fixed), true);

    v_draw_area = gtk_drawing_area_new();
    gtk_widget_show(v_draw_area);
    gtk_drawing_area_size(GTK_DRAWING_AREA(v_draw_area), v_width, v_height);
    gtk_fixed_put(GTK_FIXED(v_fixed), v_draw_area, 0, 0);

    v_hsba = (GtkAdjustment*)gtk_adjustment_new(0.0, 0.0, v_width, 8.0,
        v_width/2, v_width);
    v_hsb = gtk_hscrollbar_new(GTK_ADJUSTMENT(v_hsba));
    gtk_widget_hide(v_hsb);
    v_vsba = (GtkAdjustment*)gtk_adjustment_new(0.0, 0.0, v_height, 8.0,
        v_height/2, v_height);
    v_vsb = gtk_vscrollbar_new(GTK_ADJUSTMENT(v_vsba));
    gtk_widget_hide(v_vsb);
    v_pixmap = gdk_pixmap_new(0, v_width, v_height, tk_visual_depth());
    v_gc = gdk_gc_new(v_pixmap);
    htmColor c;
    tk_parse_color("white", &c);
    tk_set_foreground(c.pixel);
    gdk_draw_rectangle(v_pixmap, v_gc, true, 0, 0, v_width, v_height);

    v_pixmap_bak = 0;
    v_font = 0;

    int events = gtk_widget_get_events(v_draw_area);
    gtk_widget_set_events(v_draw_area, events
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

    gtk_signal_connect(GTK_OBJECT(v_fixed), "size-allocate",
        GTK_SIGNAL_FUNC(v_resize_hdlr), this);

    gtk_signal_connect(GTK_OBJECT(v_vsba), "value-changed",
        (GtkSignalFunc)v_vsb_change_hdlr, this);
    gtk_signal_connect(GTK_OBJECT(v_hsba), "value-changed",
        (GtkSignalFunc)v_hsb_change_hdlr, this);

    gtk_selection_add_targets(GTK_WIDGET(v_draw_area),
        GDK_SELECTION_PRIMARY, targets, n_targets);
    gtk_signal_connect(GTK_OBJECT(v_draw_area), "selection-clear-event",
        (GtkSignalFunc)v_selection_clear_hdlr, this);
    gtk_signal_connect(GTK_OBJECT(v_draw_area), "selection-get",
        (GtkSignalFunc)v_selection_get_hdlr, this);

    gtk_signal_connect(GTK_OBJECT(v_draw_area), "expose-event",
        (GtkSignalFunc)v_expose_event_hdlr, this);
    gtk_signal_connect(GTK_OBJECT(v_draw_area), "motion-notify-event",
        (GtkSignalFunc)v_motion_event_hdlr, this);
    gtk_signal_connect(GTK_OBJECT(v_draw_area), "button-press-event",
        (GtkSignalFunc)v_button_dn_hdlr, this);
    gtk_signal_connect(GTK_OBJECT(v_draw_area), "button-release-event",
        (GtkSignalFunc)v_button_up_hdlr, this);

    gtk_signal_connect_after(GTK_OBJECT(v_draw_area), "key-press-event",
        (GtkSignalFunc)v_key_dn_hdlr, this);

    gtk_signal_connect(GTK_OBJECT(v_draw_area), "focus-in-event",
        (GtkSignalFunc)v_focus_event_hdlr, this);
    gtk_signal_connect(GTK_OBJECT(v_draw_area), "focus-out-event",
        (GtkSignalFunc)v_focus_event_hdlr, this);
    gtk_signal_connect(GTK_OBJECT(v_draw_area), "leave-notify-event",
        (GtkSignalFunc)v_focus_event_hdlr, this);
    gtk_signal_connect(GTK_OBJECT(v_draw_area), "enter-notify-event",
        (GtkSignalFunc)v_focus_event_hdlr, this);
    gtk_signal_connect(GTK_OBJECT(v_draw_area), "scroll-event",
        (GtkSignalFunc)v_scroll_event_hdlr, this);

    gtk_table_attach(GTK_TABLE(v_form), v_fixed, 0, 1, 0, 1,
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
    gtk_signal_connect(GTK_OBJECT(v_draw_area), "style-set",
        GTK_SIGNAL_FUNC(v_font_change_hdlr), this);

    set_font(FC.getName(FNT_MOZY));
    set_fixed_font(FC.getName(FNT_MOZY_FIXED));

    g_log_set_handler("Pango", G_LOG_LEVEL_WARNING, pango_warning_hdlr, NULL);
}


gtk_viewer::~gtk_viewer()
{
    gtk_signal_disconnect_by_func(GTK_OBJECT(v_draw_area),
        (GtkSignalFunc)v_focus_event_hdlr, this);
    gdk_pixmap_unref(v_pixmap);
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
        if (GTK_WIDGET_MAPPED(v_hsb) && v_hsba)
            return ((int)GTK_ADJUSTMENT(v_hsba)->value);
    }
    else {
        if (GTK_WIDGET_MAPPED(v_vsb) && v_vsba)
            return ((int)GTK_ADJUSTMENT(v_vsba)->value);
    }
    return (0);
}


void
gtk_viewer::set_scroll_position(int value, bool horiz)
{
    if (horiz) {
        if (GTK_WIDGET_MAPPED(v_hsb) && v_hsba) {
            if (value < 0)
                value = 0;
            if (value > v_hsba->upper - v_hsba->page_size)
                value = (int)(v_hsba->upper - v_hsba->page_size);
            int oldv = (int)gtk_adjustment_get_value(GTK_ADJUSTMENT(v_hsba));
            if (value == oldv && value != htm_viewarea.x) {
                // Make sure handler is called after (e.g.) font change.
                hsb_change_handler(value);
                return;
            }
            gtk_adjustment_set_value(GTK_ADJUSTMENT(v_hsba), value);
        }
    }
    else {
        if (GTK_WIDGET_MAPPED(v_vsb) && v_vsba) {
            if (value < 0)
                value = 0;
            if (value > v_vsba->upper - v_vsba->page_size)
                value = (int)(v_vsba->upper - v_vsba->page_size);
            int oldv = (int)gtk_adjustment_get_value(GTK_ADJUSTMENT(v_vsba));
            if (value == oldv && value != htm_viewarea.y) {
                // Make sure handler is called after (e.g.) font change.
                vsb_change_handler(value);
                return;
            }
            gtk_adjustment_set_value(GTK_ADJUSTMENT(v_vsba), value);
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
    if (GTK_WIDGET_MAPPED(v_hsb) && v_hsba) {
        int xmin = l < r ? l : r - 50;
        int xmax = l > r ? l : r + 50;
        gtk_adjustment_clamp_page(GTK_ADJUSTMENT(v_hsba), xmin, xmax);
    }
    if (GTK_WIDGET_MAPPED(v_vsb) && v_vsba) {
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
    // This is called after formatting with the document size.
    int wid, hei;
    if (GTK_WIDGET_MAPPED(v_draw_area)) {
        wid = v_form->allocation.width;
        hei = v_form->allocation.height;
    }
    else {
        wid = v_width;
        hei = v_height;
    }

    htm_viewarea.x = 0;

    if (h > hei) {
        double val = gtk_adjustment_get_value(v_vsba);
        v_vsba->upper = h;
        v_vsba->page_size = v_height;
        v_vsba->page_increment = v_height/2;
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

        if (v_width <= 1 || v_height <= 1)
            resize_handler(&v_form->allocation);
    }

    if (w > wid) {
        double val = gtk_adjustment_get_value(v_hsba);
        v_hsba->upper = w;
        v_hsba->page_size = v_width;
        v_hsba->page_increment = v_width/2;
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
    if (v_draw_area->window)
        gdk_window_copy_area(v_draw_area->window, v_gc, x, y, v_pixmap,
            x, y, w, h);
}


void
gtk_viewer::tk_window_size(htmInterface::WinRetMode mode,
    unsigned int *wp, unsigned int *hp)
{
    if (mode == htmInterface::VIEWABLE) {
        // Return the total available viewable size, assuming no
        // scrollbars.
        if (GTK_WIDGET_MAPPED(v_draw_area)) {
            *wp = v_form->allocation.width;
            *hp = v_form->allocation.height;
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
    return (v_vsb->allocation.width);
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
    if (!v_cursor) {
        GdkColor fg, bg;
        fg.pixel = 1;
        bg.pixel = 0;
        GdkWindow *window = v_draw_area->window;
        if (!window)
            return;
        GdkPixmap *shape = gdk_pixmap_create_from_data(window, fingers_bits,
            fingers_width, fingers_height, 1, &fg, &bg);
        GdkPixmap *mask = gdk_pixmap_create_from_data(window, fingers_m_bits,
            fingers_m_width, fingers_m_height, 1, &fg, &bg);
//XXX   Calls above fail under Quartz.
        if (!shape || !mask)
            return;

        GdkColormap *colormap = gtk_widget_get_colormap(v_draw_area);
        GdkColor white, black;
        gdk_color_white(colormap, &white);
        gdk_color_black(colormap, &black);
        v_cursor = gdk_cursor_new_from_pixmap(shape, mask,
            &white, &black, fingers_x_hot, fingers_y_hot);
    }
    if (set)
        gdk_window_set_cursor(v_draw_area->window, v_cursor);
    else
        gdk_window_set_cursor(v_draw_area->window, 0);
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
            t->real_id = gtk_timeout_add(msec, (GtkFunction)t->callback,
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
                v_draw_area->window)
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

#if GTK_CHECK_VERSION(2,4,0)
    // underline offset
    font->ul_offset = pm->underline_position/PANGO_SCALE;
    font->ul_offset += font->descent;

    // underline thickness
    font->ul_thickness = pm->underline_thickness/PANGO_SCALE;

    // strikeout offset
    font->st_offset = pm->strikethrough_position/PANGO_SCALE;

    // strikeout descent
    font->st_thickness = pm->strikethrough_thickness/PANGO_SCALE;
#else
    font->ul_offset = font->descent;
    font->ul_thickness = 1;
    font->st_offset = font->ascent/2 - font->descent/2;
    font->st_thickness = 1;
#endif

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
    v_font = font->xfont;
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
    GdkVisual *vs = gdk_visual_get_system();
    switch (vs->type) {
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
    GdkVisual *vs = gdk_visual_get_system();
    return (vs->depth);
}


htmPixmap *
gtk_viewer::tk_new_pixmap(int w, int h)
{
    GdkPixmap *pm = gdk_pixmap_new(0, w, h, tk_visual_depth());
    htmColor c;
    tk_parse_color("white", &c);
    tk_set_foreground(c.pixel);
    gdk_draw_rectangle(pm, v_gc, true, 0, 0, w, h);
    return ((htmPixmap*)pm);
}


void
gtk_viewer::tk_release_pixmap(htmPixmap *pm)
{
    if (pm)
        gdk_pixmap_unref((GdkPixmap*)pm);
}


htmPixmap *
gtk_viewer::tk_pixmap_from_info(htmImage *image, htmImageInfo *info,
    unsigned int *color_map)
{
    unsigned int size = info->width * info->height;

    (void)image;
    GdkImage *img = gdk_image_new(GDK_IMAGE_FASTEST,
        gdk_visual_get_system(), info->width, info->height);

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
        gdk_image_put_pixel(img, x, y, px);
#endif
    }

    GdkPixmap *pmap = gdk_pixmap_new(0, info->width, info->height,
        tk_visual_depth());
    gdk_draw_image(pmap, v_gc, img, 0, 0, 0, 0, info->width, info->height);
    gdk_image_destroy(img);
    return (pmap);
}


void
gtk_viewer::tk_set_draw_to_pixmap(htmPixmap *pm)
{
    if (pm) {
        if (!v_pixmap_bak) {
            v_pixmap_bak = v_pixmap;
            v_pixmap = (GdkPixmap*)pm;
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
    GdkPixmap *pm = gdk_pixmap_create_from_data(v_draw_area->window,
        (char*)data, width, height, 1, &fg, &bg);
    return (pm);
}


void
gtk_viewer::tk_release_bitmap(htmBitmap *bmap)
{
    if (bmap)
        gdk_pixmap_unref((GdkPixmap*)bmap);
}


htmXImage *
gtk_viewer::tk_new_image(int w, int h)
{
    return ((htmXImage*)gdk_image_new(GDK_IMAGE_FASTEST,
        gdk_visual_get_system(), w, h));
}


void
gtk_viewer::tk_fill_image(htmXImage *ximage, unsigned char *data,
    unsigned int *color_map, int lo, int hi)
{
    GdkImage *image = (GdkImage*)ximage;
    int wid = image->width;

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
        gdk_image_put_pixel(image, x, y, px);
#endif
    }
}


void
gtk_viewer::tk_draw_image(int xs, int ys, htmXImage *image,
    int x, int y, int w, int h)
{
    gdk_draw_image(v_pixmap, v_gc, (GdkImage*)image, xs, ys, x, y, w, h);
}


void
gtk_viewer::tk_release_image(htmXImage *image)
{
    gdk_image_destroy((GdkImage*)image);
}


void
gtk_viewer::tk_set_foreground(unsigned int pix)
{
    GdkColor c;
    c.pixel = pix;
    gdk_gc_set_foreground(v_gc, &c);
}


void
gtk_viewer::tk_set_background(unsigned int pix)
{
    GdkColor c;
    c.pixel = pix;
    gdk_gc_set_background(v_gc, &c);
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
    bool ret = gdk_colormap_alloc_color(gtk_widget_get_colormap(v_draw_area),
        &c, false, true);
    if (ret) {
        clr->pixel = c.pixel;
        return (true);
    }
    return (false);
}


int
gtk_viewer::tk_query_colors(htmColor *clrs, unsigned int sz)
{
    GdkColor *colors = new GdkColor[sz];
    for (unsigned int i = 0; i < sz; i++)
        colors[i].pixel = clrs[i].pixel;
    GdkColormap *colormap = gtk_widget_get_colormap(v_draw_area);
    for (unsigned int i = 0; i < sz; i++) {
        gdk_colormap_query_color(colormap, colors[i].pixel, colors + i);
        clrs[i].red   = colors[i].red >> 8;
        clrs[i].green = colors[i].green >> 8;
        clrs[i].blue  = colors[i].blue >> 8;
    }
    delete [] colors;
    return (sz);
}


void
gtk_viewer::tk_free_colors(unsigned int *pixels, unsigned int sz)
{
    if (!sz)
        return;
    GdkColor *c = new GdkColor[sz];
    for (unsigned int i = 0; i < sz; i++)
        c[i].pixel = pixels[i];
    gdk_colormap_free_colors(gtk_widget_get_colormap(v_draw_area), c, sz);
    delete [] c;
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
        bool ret =
            gdk_colormap_alloc_color(gtk_widget_get_colormap(v_draw_area),
                &c, false, true);
        if (ret)
            pixels[i] = c.pixel;
        else
            // can't happen
            pixels[i] = 0;
    }
    return (true);
}


void
gtk_viewer::tk_set_clip_mask(htmPixmap*, htmBitmap *bmap)
{
    gdk_gc_set_clip_mask(v_gc, (GdkBitmap*)bmap);
}


void
gtk_viewer::tk_set_clip_origin(int x, int y)
{
    gdk_gc_set_clip_origin(v_gc, x, y);
}


void
gtk_viewer::tk_set_clip_rectangle(htmRect *r)
{
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
}


void
gtk_viewer::tk_draw_pixmap(int xs, int ys, htmPixmap *pm,
    int x, int y, int w, int h)
{
    gdk_window_copy_area(v_pixmap, v_gc, xs, ys, (GdkPixmap*)pm, x, y, w, h);
}


void
gtk_viewer::tk_tile_draw_pixmap(int org_x, int org_y, htmPixmap *pm,
    int x, int y, int w, int h)
{
    if (!GDK_IS_PIXMAP(pm))
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
    gdk_gc_set_fill(v_gc, GDK_TILED);
    gdk_gc_set_tile(v_gc, GDK_DRAWABLE(pm));
    gdk_gc_set_ts_origin(v_gc, org_x, org_y);
    gdk_draw_rectangle(v_pixmap, v_gc, true, x, y, w, h);
    gdk_gc_set_fill(v_gc, GDK_SOLID);
#endif
}


void
gtk_viewer::tk_draw_rectangle(bool fill, int x, int y, int w, int h)
{
    gdk_draw_rectangle(v_pixmap, v_gc, fill, x, y, w, h);
}


void
gtk_viewer::tk_set_line_style(FillMode m)
{
    if (m == TILED)
        gdk_gc_set_line_attributes(v_gc, 1, GDK_LINE_DOUBLE_DASH,
            GDK_CAP_BUTT, GDK_JOIN_BEVEL);
    else
        gdk_gc_set_line_attributes(v_gc, 1, GDK_LINE_SOLID,
            GDK_CAP_BUTT, GDK_JOIN_BEVEL);
}


void
gtk_viewer::tk_draw_line(int x1, int y1, int x2, int y2)
{
    gdk_draw_line(v_pixmap, v_gc, x1, y1, x2, y2);
}


void
gtk_viewer::tk_draw_text(int x, int y, const char *text, int len)
{
    if (v_iso8859 || is_utf8((const unsigned char*)text, len)) {
        // Must convert to UTF-8.
        gsize xlen;
        text = g_convert(text, len, "UTF-8", "ISO-8859-1", 0, &xlen, 0);
    }

    PangoFontDescription *pfd = (PangoFontDescription*)v_font;
    gtk_widget_modify_font(v_draw_area, pfd);
    PangoLayout *lout = gtk_widget_create_pango_layout(v_draw_area, text);
    if (v_iso8859) {
        g_free((void*)text);
    }
    y -= v_text_yoff;
    gdk_draw_layout(v_pixmap, v_gc, x, y, lout);
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
    gdk_draw_polygon(v_pixmap, v_gc, fill, gpts, numpts);
    delete [] gpts;
}


void
gtk_viewer::tk_draw_arc(bool fill, int x, int y, int w, int h, int as, int ae)
{
    gdk_draw_arc(v_pixmap, v_gc, fill, x, y, w, h, as, ae);
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
        if (event->key.keyval == GDK_Return) {
            for (htmForm *e = entry->next; e; e = e->next) {
                if (e->type == FORM_TEXT) {
                    // found next text entry, give it the focus
                    GtkWidget *w = GTK_WIDGET(e->widget);
                    while (w->parent) {
                        gtk_container_set_focus_child(
                            GTK_CONTAINER(w->parent), w);
                        w = w->parent;
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
                (gtk_viewer*)gtk_object_get_data(GTK_OBJECT(widget), "viewer");
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
            GtkWidget *edit = (GtkWidget*)gtk_object_get_data(GTK_OBJECT(arg),
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
                (GtkWidget*)gtk_object_get_data(GTK_OBJECT(arg),
                "browse");
            if (button)
                gtk_object_set_data(GTK_OBJECT(button), "filesel", 0);
        }
    }


    void
    button_press_hdlr(GtkWidget *widget, htmForm *entry)
    {
        gtk_viewer *viewer =
            (gtk_viewer*)gtk_object_get_data(GTK_OBJECT(widget), "viewer");
        if (!viewer)
            return;

        if (entry->type == FORM_SUBMIT)
            viewer->formActivate(0, entry);
        else if (entry->type == FORM_RESET)
            viewer->formReset(entry);
        else if (entry->type == FORM_FILE) {
            // pop up/down a file selection window
            GRfilePopup *fs =
                (GRfilePopup*)gtk_object_get_data(GTK_OBJECT(widget),
                "filesel");
            if (!fs) {
                fs = new GTKfilePopup(0, fsSEL, entry->widget, 0);
                fs->register_callback(file_selection);
                fs->register_quit_callback(file_selection_destroy);
                gtk_object_set_data(GTK_OBJECT(widget), "filesel", fs);
            }
            fs->set_visible(true);
        }
    }
}
// End of form element callbacks


#define PADVAL 10


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
            gtk_widget_set_usize(ed, entry->width, entry->height);

            if (entry->type == FORM_TEXT) {
                if (entry->value)
                    // enter key terminates with action
                    gtk_entry_set_text(GTK_ENTRY(ed), entry->value);

                gtk_object_set_data(GTK_OBJECT(ed), "viewer", this);
                gtk_signal_connect(GTK_OBJECT(ed), "key-press-event",
                    GTK_SIGNAL_FUNC(input_key_hdlr), entry);

            }
            // These pass mouse wheel scroll event to main window.
            gtk_signal_connect(GTK_OBJECT(ed), "scroll-event",
                (GtkSignalFunc)v_scroll_event_hdlr, this);
            if (entry->type == FORM_PASSWD)
                gtk_entry_set_visibility(GTK_ENTRY(ed), false);
            entry->widget = ed;
            gtk_widget_show(ed);
            if (GTK_WIDGET_NO_WINDOW(ed)) {
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
            gtk_widget_set_usize(cb, entry->width, entry->height);

            entry->widget = cb;
            gtk_widget_show(cb);
            if (GTK_WIDGET_NO_WINDOW(cb)) {
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
            gtk_widget_set_usize(cb, entry->width, entry->height);

            gtk_object_set_data(GTK_OBJECT(cb), "viewer", this);
            gtk_signal_connect(GTK_OBJECT(cb), "toggled",
                (GtkSignalFunc)radio_change_hdlr, entry);

            entry->widget = cb;
            gtk_widget_show(cb);
            if (GTK_WIDGET_NO_WINDOW(cb)) {
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
            gtk_object_set_data(GTK_OBJECT(hbox), "lineedit", ed);

            int ew = entry->size * GTKfont::stringWidth(ed, 0) + PADVAL;
            int ht = GTKfont::stringHeight(ed, 0) + PADVAL;
            gtk_widget_set_usize(ed, ew, ht);

            const char *lab = entry->value ? entry->value : "Browse...";
            GtkWidget *button = gtk_button_new_with_label(entry->value ?
                entry->value : "Browse...");
            gtk_widget_show(button);
            int bw = GTKfont::stringWidth(button, lab) + PADVAL;
            gtk_widget_set_usize(button, bw, ht);
            gtk_object_set_data(GTK_OBJECT(hbox), "browse", button);

            entry->width = ew + bw + PADVAL;
            entry->height = ht;

            // Pack and connect stuff
            gtk_box_pack_start(GTK_BOX(hbox), ed, 0, 0, 0);
            gtk_box_pack_start(GTK_BOX(hbox), button, 0, 0, 0);

            gtk_object_set_data(GTK_OBJECT(button), "viewer", this);
            gtk_signal_connect(GTK_OBJECT(button), "clicked",
                (GtkSignalFunc)button_press_hdlr, entry);

            entry->widget = hbox;
            if (GTK_WIDGET_NO_WINDOW(hbox)) {
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
            gtk_widget_set_usize(cb, entry->width, entry->height);

            gtk_object_set_data(GTK_OBJECT(cb), "viewer", this);
            gtk_signal_connect(GTK_OBJECT(cb), "clicked",
                (GtkSignalFunc)button_press_hdlr, entry);

            entry->widget = cb;
            gtk_widget_show(cb);
            if (GTK_WIDGET_NO_WINDOW(cb)) {
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
            GtkWidget *list = gtk_list_new();
            gtk_scrolled_window_add_with_viewport(
                GTK_SCROLLED_WINDOW(scrolled_win), list);
            gtk_widget_show(list);
            gtk_object_set_user_data(GTK_OBJECT(scrolled_win), list);
            gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
            if (entry->multiple)
                gtk_list_set_selection_mode(GTK_LIST(list),
                    GTK_SELECTION_MULTIPLE);

            entry->widget = scrolled_win;
            gtk_widget_show(scrolled_win);
            if (GTK_WIDGET_NO_WINDOW(scrolled_win)) {
                GtkWidget *b = gtk_event_box_new();
                gtk_widget_show(b);
                gtk_container_add(GTK_CONTAINER(b), scrolled_win);
                add_widget(b);
            }
            else
                add_widget(scrolled_win);
        }
        else {
            GtkWidget *option_menu = gtk_option_menu_new();
            GtkWidget *menu = gtk_menu_new();
            gtk_widget_show(option_menu);
            gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);

            entry->widget = option_menu;
            gtk_widget_show(option_menu);
            if (GTK_WIDGET_NO_WINDOW(option_menu)) {
                GtkWidget *b = gtk_event_box_new();
                gtk_widget_show(b);
                gtk_container_add(GTK_CONTAINER(b), option_menu);
                add_widget(b);
            }
            else
                add_widget(option_menu);
        }
        break;

    case FORM_OPTION:
        if (parent->multiple || parent->size > 1) {
            // scrolled_win/viewport/list
            GtkList *list =
                GTK_LIST(GTK_BIN(GTK_BIN(parent->widget)->child)->child);
            // append item to bottom of list
            GtkWidget *listitem =
                gtk_list_item_new_with_label(entry->name);
            gtk_widget_show(listitem);
            gtk_container_add(GTK_CONTAINER(list), listitem);
            entry->widget = listitem;
        }
        else {
            GtkWidget *menu = gtk_option_menu_get_menu(
                GTK_OPTION_MENU(parent->widget));
            GtkWidget *menu_entry =
                gtk_menu_item_new_with_label(entry->name);
            gtk_widget_show(menu_entry);
            gtk_menu_append(GTK_MENU(menu), menu_entry);
            entry->widget = menu_entry;
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
            gtk_widget_set_usize(frame ? frame : textw, entry->width,
                entry->height);

            entry->widget = textw;
            GtkWidget *w = frame ? frame : textw;
            if (GTK_WIDGET_NO_WINDOW(w)) {
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
        gtk_widget_set_usize(GTK_WIDGET(entry->widget), entry->width,
            entry->height);

        // set initial selections
        int cnt = 0;
        for (htmForm *e = entry->options; e; e = e->next, cnt++) {
            if (e->selected)
                gtk_list_select_item(list, cnt);
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
        GtkWidget *menu =
            gtk_option_menu_get_menu(GTK_OPTION_MENU(option_menu));
        GtkRequisition req;
        gtk_widget_size_request(menu, &req);
        entry->width = req.width + 24;
        entry->height = GTKfont::stringHeight(option_menu, 0) + PADVAL;
        gtk_widget_set_usize(GTK_WIDGET(entry->widget), entry->width,
            entry->height);
        gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), cnt);
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
                    gtk_object_get_data(GTK_OBJECT(form), "lineedit");
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
                    gtk_object_get_data(GTK_OBJECT(form), "lineedit");
                if (ed)
                    gtk_entry_set_text(GTK_ENTRY(ed), string);
            }
        }

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
            for (htmForm *e = entry->options; e; e = e->next)
                e->checked =
                    (GTK_WIDGET(e->widget)->state == GTK_STATE_SELECTED);
        }
        else {
            GtkWidget *menu =
                gtk_option_menu_get_menu(GTK_OPTION_MENU(entry->widget));
            GtkWidget *active = gtk_menu_get_active(GTK_MENU(menu));
            for (htmForm *e = entry->options; e; e = e->next)
                e->checked = (GTK_WIDGET(e->widget) == active);
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
                if (e->selected)
                    gtk_list_item_select(GTK_LIST_ITEM(e->widget));
                else
                    gtk_list_item_deselect(GTK_LIST_ITEM(e->widget));
            }
        }
        else {
            int cnt = 0;
            for (htmForm *e = entry->options; e; e = e->next) {
                if (e->selected) {
                    gtk_option_menu_set_history(GTK_OPTION_MENU(entry->widget),
                        cnt);
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
                (GRfilePopup*)gtk_object_get_data(GTK_OBJECT(w), "filesel");
            if (fs)
                // The popdown method won't work since there is no parent
                // container.
                delete fs;
        }
        // Find the ancestor parented by GtkFixed.
        while (w->parent && !GTK_IS_FIXED(w->parent))
            w = w->parent;
        if (!w->parent)
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
        while (w->parent && !GTK_IS_FIXED(w->parent))
            w = w->parent;
        if (!w->parent)
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
        gdk_draw_rectangle(v_draw_area->window, v_gc, false, xx, yy, w, h);
    }
}


bool
gtk_viewer::expose_event_handler(GdkEventExpose *event)
{
    setReady();
    gdk_window_copy_area(v_draw_area->window, v_gc,
        event->area.x, event->area.y, v_pixmap, event->area.x, event->area.y,
        event->area.width, event->area.height);
    v_seen_expose = true;
    return (true);
}


bool
gtk_viewer::resize_handler(GtkAllocation *a)
{
    // Ignore resize events until the initial expose event is fully
    // handled.
    if (!v_seen_expose)
        return (true);

    if (a->width != v_width || a->height != v_height) {
        v_width = a->width;
        v_height = a->height;
        GdkPixmap *pm = (GdkPixmap*)tk_new_pixmap(v_width, v_height);
        gdk_pixmap_unref(v_pixmap);
        v_pixmap = pm;
        gtk_drawing_area_size(GTK_DRAWING_AREA(v_draw_area),
            v_width, v_height);
        int x = htm_viewarea.x;
        int y = htm_viewarea.y;
        htmWidget::resize();
#ifndef WIN32
        // This hangs up badly in MSW for some reason.
        if (GTK_WIDGET_REALIZED(v_draw_area)) {
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
    // Handle mouse wheel events (gtk1 only)
    if (event->button == 4 || event->button == 5) {
        if (GTK_WIDGET_MAPPED(v_vsb) && v_vsba) {
            double nval = v_vsba->value + ((event->button == 4) ?
                -v_vsba->page_increment / 4 : v_vsba->page_increment / 4);
            if (nval < v_vsba->lower)
                nval = v_vsba->lower;
            else if (nval > v_vsba->upper - v_vsba->page_size)
                nval = v_vsba->upper - v_vsba->page_size;
            gtk_adjustment_set_value(v_vsba, nval);
        }
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
            gdk_window_copy_area(v_pixmap, v_gc, 0, 0, v_pixmap,
                0, delta, v_width, v_height - delta);
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
            gdk_window_copy_area(v_pixmap, v_gc, 0, delta, v_pixmap,
                0, 0, v_width, v_height - delta);
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
            gdk_window_copy_area(v_pixmap, v_gc, 0, 0, v_pixmap,
                delta, 0, v_width - delta, v_height);
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
            gdk_window_copy_area(v_pixmap, v_gc, delta, 0, v_pixmap,
                0, 0, v_width - delta, v_height);
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
    if (!gtk_selection_clear(widget, event))
        return (false);

    if (event->selection == GDK_SELECTION_PRIMARY) {
        if (htm_text_selection)
            selectRegion(0, 0, 0, 0);
    }
    return (true);
}


void
gtk_viewer::selection_get_handler(GtkWidget*,
    GtkSelectionData *selection_data, unsigned int info, unsigned int)
{
    if (selection_data->selection != GDK_SELECTION_PRIMARY)
        return;

    const char *str = htm_text_selection;
    if (!str)
        return;  // refuse
    int length = strlen(str);

    if (info == TARGET_STRING) {
        gtk_selection_data_set(selection_data, GDK_SELECTION_TYPE_STRING,
            8*sizeof(char), (unsigned char*)str, length);
    }
    else if (info == TARGET_TEXT || info == TARGET_COMPOUND_TEXT) {

        GdkAtom encoding;
        int fmtnum;
        unsigned char *text;
        int new_length;
        gdk_string_to_compound_text(str, &encoding, &fmtnum, &text,
            &new_length);
        // Strange problem here in gtk2 on RHEL5, some text returns 0
        // here that causes error message and erroneous transfer.
        if (fmtnum == 0)
            fmtnum = 8;
        gtk_selection_data_set(selection_data, encoding, fmtnum, text,
            new_length);
        gdk_free_compound_text(text);
    }
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
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v && GTK_WIDGET_MAPPED(v->v_vsb))
        gtk_propagate_event(v->v_vsb, event);
    return (true);
}


//-----------------------------------------------------------------------------
// Event callbacks

// Private static GTK signal handler.
//
int
gtk_viewer::v_expose_event_hdlr(GtkWidget*, GdkEvent *event, void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v)
        return (v->expose_event_handler((GdkEventExpose*)event));
    return (0);
}


// Private static GTK signal handler.
//
void
gtk_viewer::v_resize_hdlr(GtkWidget*, GtkAllocation *a, void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v)
        v->resize_handler(a);
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
    if (kev->keyval == GDK_Down || kev->keyval == GDK_Up) {
        if (v->v_vsb && v->v_vsba) {
            GtkAdjustment *adj = (GtkAdjustment*)v->v_vsba;
            double nval = adj->value + ((kev->keyval == GDK_Up) ?
                -adj->page_increment / 4 : adj->page_increment / 4);
            if (nval < adj->lower)
                nval = adj->lower;
            else if (nval > adj->upper - adj->page_size)
                nval = adj->upper - adj->page_size;
            gtk_adjustment_set_value(adj, nval);
        }
        return (1);
    }
    if (kev->keyval == GDK_Next || kev->keyval == GDK_Prior) {
        if (v->v_vsb && v->v_vsba) {
            GtkAdjustment *adj = (GtkAdjustment*)v->v_vsba;
            double nval = adj->value + ((kev->keyval == GDK_Prior) ?
                -adj->page_increment : adj->page_increment);
            if (nval < adj->lower)
                nval = adj->lower;
            else if (nval > adj->upper - adj->page_size)
                nval = adj->upper - adj->page_size;
            gtk_adjustment_set_value(adj, nval);
        }
        return (1);
    }
    if (kev->keyval == GDK_Home) {
        if (v->v_vsb && v->v_vsba) {
            GtkAdjustment *adj = (GtkAdjustment*)v->v_vsba;
            gtk_adjustment_set_value(adj, adj->lower);
        }
        return (1);
    }
    if (kev->keyval == GDK_End) {
        if (v->v_vsb && v->v_vsba) {
            GtkAdjustment *adj = (GtkAdjustment*)v->v_vsba;
            gtk_adjustment_set_value(adj, adj->upper - adj->page_size);
        }
        return (1);
    }
    if (kev->keyval == GDK_Left || kev->keyval == GDK_Right) {
        if (v->v_hsb && v->v_hsba) {
            GtkAdjustment *adj = (GtkAdjustment*)v->v_hsba;
            double nval = adj->value + ((kev->keyval == GDK_Left) ?
                -adj->page_increment / 4 : adj->page_increment / 4);
            if (nval < adj->lower)
                nval = adj->lower;
            else if (nval > adj->upper - adj->page_size)
                nval = adj->upper - adj->page_size;
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
    if (event->any.window != widget->window)
        return (0);

    if (event->type == GDK_ENTER_NOTIFY) {
        GTK_WIDGET_SET_FLAGS(v->v_draw_area, GTK_CAN_FOCUS);
        GtkWidget *w = v->v_draw_area;
        while (w->parent)
            w = w->parent;
        gtk_window_set_focus(GTK_WINDOW(w), v->v_draw_area);
        return (1);
    }

    if (event->type == GDK_FOCUS_CHANGE) {
        if (widget->window == v->v_draw_area->window)
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
        v->vsb_change_handler((int)a->value);
}


// Private static GTK signal handler.
//
void
gtk_viewer::v_hsb_change_hdlr(GtkAdjustment *a, void *vp)
{
    gtk_viewer *v = static_cast<gtk_viewer*>(vp);
    if (v)
        v->hsb_change_handler((int)a->value);
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

