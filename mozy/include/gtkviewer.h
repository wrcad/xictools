
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

#ifndef GTKVIEWER_H
#define GTKVIEWER_H

#include "htm_widget.h"
#include "help_context.h"

class Transaction;

//
// HTML viewer component for the help viewer.  This contains the
// htmWidget, scroll bars, and drawing area.  The v_form (GtkTable)
// field is the top-level widget.
//

namespace gtkinterf {
    // Timer object for animations.
    //
    struct gtk_timer
    {
        gtk_timer(int(*cb)(void*), void *a, gtk_timer *n)
            {
                callback = cb;
                arg = a;
                next = n;
                id = 0;
                real_id = 0;
            }

        ~gtk_timer()
            {
                if (real_id > 0)
                    gtk_timeout_remove(real_id);
            }

        int(*callback)(void*);
        void *arg;
        int id;                     // our id number
        int real_id;                // gtk's id number
        gtk_timer *next;
    };

    class gtk_viewer : public ViewerWidget, public htmInterface,
        protected htmWidget
    {
    public:
        gtk_viewer(int, int, htmDataInterface*);
        ~gtk_viewer();

        // ViewerWidget functions
        bool initialized() { return (htm_initialized); }
        void freeze() { htmWidget::freeze(); }
        void thaw() { htmWidget::thaw(); }
        void set_transaction(Transaction*, const char*);
        Transaction *get_transaction();
        bool check_halt_processing(bool);
        void set_halt_proc_sens(bool);
        void set_status_line(const char*);
        htmImageInfo *new_image_info(const char*, bool);
        bool call_plc(const char *url) { return (htm_im.callPLC(url)); }
        htmImageInfo *image_procedure(const char *url)
            { return (htmWidget::imageLoadProc(url)); }
        void image_replace(htmImageInfo *image, htmImageInfo *new_image)
            { htm_im.imageReplace(image, new_image); }
        bool is_body_image(const char *url)
            {
                return (htm_body_image_url && !htm_im.im_body_image &&
                    suffix(url, htm_body_image_url));
            }

        // Wrap these for export.
        void set_select_color(const char *c)
            { htmWidget::set_select_bg(c); }
        void set_imagemap_boundary_color(const char *c)
            { htmWidget::set_imagemap_fg(c); }
        void redisplay_view()
            { htmWidget::redisplay(); }

        const char *get_url();
        bool no_url_cache();
        int image_load_mode();
        int image_debug_mode();
        GRwbag *get_widget_bag();

        // Inlines to access needed protected members
        bool is_ready()
            { return (isReady()); }
        char *get_title()
            { return (getTitle()); }
        void set_mime_type(const char *mime_type)
            { setMimeType(mime_type); }
        char *get_postscript_text(int fontfamily, const char *url,
            const char *title, bool use_header, bool a4)
            { return (getPostscriptText(fontfamily, url, title,
            use_header, a4)); }
        char *get_plain_text()
            { return (getPlainText()); }
        char *get_html_text()
            { return (getString()); }
        bool find_words(const char *target, bool up, bool case_insens)
            { return (findWords(target, up, case_insens)); }
        void set_html_warnings(bool set)
            { setBadHtmlWarnings(set); }
        void set_iso8859_source(bool set)
            { v_iso8859 = set; }

        void progressive_kill()
            { htm_im.imageProgressiveKill(); }
        void set_freeze_animations(bool set)
            { setFreezeAnimations(set); }

        int anchor_pos_by_name(const char *aname)
            { return (anchorPosByName(aname)); }
        void set_anchor_style(AnchorStyle style)
            {
                setAnchorStyle(style);
                setAnchorVisitedStyle(style);
                setAnchorTargetStyle(style);
            }
        void set_anchor_highlighting(bool set)
            { setHighlightOnEnter(set); }

        // Return true if suf is a suffix of str
        //
        bool suffix(const char *str, const char *suf) const
            {
                int n1 = strlen(str);
                int n2 = strlen(suf);
                if (n1 >= n2 && !strcmp(suf, str + n1 - n2))
                    return (true);
                return (false);
            }

        void set_source(const char*);
        void set_font(const char*);
        void set_fixed_font(const char*);
        void hide_drawing_area(bool);
        void add_widget(GtkWidget*);
        int scroll_position(bool);
        void set_scroll_position(int, bool);
        void set_scroll_policy(GtkPolicyType, GtkPolicyType);
        int anchor_position(const char*);
        void scroll_visible(int, int, int, int);
        GtkWidget *top_widget() { return (v_form); }
        GtkWidget *fixed_widget() { return (v_fixed); }

        void formActivate(htmEvent *e, htmForm *f)
            { return (htmWidget::formActivate(e, f)); }
        void formReset(htmForm *f)
            { return (htmWidget::formReset(f)); }

        // htmInterface implementation
        void tk_resize_area(int, int);
        void tk_refresh_area(int, int, int, int);
        void tk_window_size(WinRetMode, unsigned int*, unsigned int*);
        unsigned int tk_scrollbar_width();
        void tk_set_anchor_cursor(bool);
        unsigned int tk_add_timer(int(*)(void*), void*);
        void tk_remove_timer(int);
        void tk_start_timer(unsigned int, int);
        void tk_claim_selection(const char*);

        htmFont *tk_alloc_font(const char*, int, unsigned char);
        void tk_release_font(void*);
        void tk_set_font(htmFont*);
        int tk_text_width(htmFont*, const char*, int);

        CCXmode tk_visual_mode();
        int tk_visual_depth();
        htmPixmap *tk_new_pixmap(int, int);
        void tk_release_pixmap(htmPixmap*);
        htmPixmap *tk_pixmap_from_info(htmImage*, htmImageInfo*,
            unsigned int*);
        void tk_set_draw_to_pixmap(htmPixmap*);
        htmBitmap *tk_bitmap_from_data(int, int, unsigned char*);
        void tk_release_bitmap(htmBitmap*);
        htmXImage *tk_new_image(int, int);
        void tk_fill_image(htmXImage*, unsigned char*, unsigned int*,
            int, int);
        void tk_draw_image(int, int, htmXImage*, int, int, int, int);
        void tk_release_image(htmXImage*);

        void tk_set_foreground(unsigned int);
        void tk_set_background(unsigned int);
        bool tk_parse_color(const char*, htmColor*);
        bool tk_alloc_color(htmColor*);
        int tk_query_colors(htmColor*, unsigned int);
        void tk_free_colors(unsigned int*, unsigned int);
        bool tk_get_pixels(unsigned short*, unsigned short*,
            unsigned short*, unsigned int, unsigned int*);

        void tk_set_clip_mask(htmPixmap*, htmBitmap*);
        void tk_set_clip_origin(int, int);
        void tk_set_clip_rectangle(htmRect*);
        void tk_draw_pixmap(int, int, htmPixmap*, int, int, int, int);
        void tk_tile_draw_pixmap(int, int, htmPixmap*, int, int, int, int);

        void tk_draw_rectangle(bool, int, int, int, int);
        void tk_set_line_style(FillMode);
        void tk_draw_line(int, int, int, int);
        void tk_draw_text(int, int, const char*, int);
        void tk_draw_polygon(bool, htmPoint*, int);
        void tk_draw_arc(bool, int, int, int, int, int, int);

        // forms handling
        void tk_add_widget(htmForm*, htmForm* = 0);
        void tk_select_close(htmForm*);
        char *tk_get_text(htmForm*);
        void tk_set_text(htmForm*, const char*);
        bool tk_get_checked(htmForm*);
        void tk_set_checked(htmForm*);
        void tk_position_and_show(htmForm*, bool);
        void tk_form_destroy(htmForm*);

    protected:
        // event handlers
        void show_selection_box(int, int, bool);
        bool expose_event_handler(GdkEventExpose*);
        bool resize_handler(GtkAllocation*);
        bool motion_event_handler(GdkEventMotion*);
        bool button_down_handler(GdkEventButton*);
        bool button_up_handler(GdkEventButton*);
        void vsb_change_handler(int);
        void hsb_change_handler(int);
        bool selection_clear_handler(GtkWidget*, GdkEventSelection*);
        void selection_get_handler(GtkWidget*, GtkSelectionData*,
            unsigned int, unsigned int);
        void font_change_handler(int);

        // GTK signal handlers
        static int v_expose_event_hdlr(GtkWidget*, GdkEvent*, void*);
        static void v_resize_hdlr(GtkWidget*, GtkAllocation*, void*);
        static int v_motion_event_hdlr(GtkWidget*, GdkEvent*, void*);
        static int v_button_dn_hdlr(GtkWidget*, GdkEvent*, void*);
        static int v_button_up_hdlr(GtkWidget*, GdkEvent*, void*);
        static int v_key_dn_hdlr(GtkWidget*, GdkEvent*, void*);
        static int v_focus_event_hdlr(GtkWidget*, GdkEvent*, void*);
        static void v_vsb_change_hdlr(GtkAdjustment*, void*);
        static void v_hsb_change_hdlr(GtkAdjustment*, void*);
        static int v_selection_clear_hdlr(GtkWidget*, GdkEventSelection*, void*);
        static void v_selection_get_hdlr(GtkWidget*, GtkSelectionData*,
            unsigned int, unsigned int, void*);
        static void v_font_change_hdlr(GtkWidget*, void*, void*);
        static int v_scroll_event_hdlr(GtkWidget*, GdkEvent*, void*);

        GtkWidget *v_form;
        GtkWidget *v_fixed;
        GtkWidget *v_draw_area;
        GtkAdjustment *v_hsba;
        GtkAdjustment *v_vsba;
        GtkWidget *v_hsb;
        GtkWidget *v_vsb;
        GdkPixmap *v_pixmap;
        GdkPixmap *v_pixmap_bak;
        GdkGC *v_gc;
        void *v_font;
            // This is a GdkFont when using X fonts (GTK-1.2), or a
            // PangoFontDescription when using Pango in GTK-2

        Transaction *v_transact;    // pointer to download context
        GdkCursor *v_cursor;        // transient anchor cursor
        gtk_timer *v_timers;        // list of animation timers
        int v_width;                // viewport width
        int v_height;               // viewport height
        int v_timer_id_cnt;         // timer id counter
        int v_btn_x;                // button down x
        int v_btn_y;                // button down y
        int v_last_x;               // last selection box x
        int v_last_y;               // last selection box y
        int v_text_yoff;            // text position fudge
        GtkPolicyType v_hpolicy;    // h scrollbar visibility policy
        GtkPolicyType v_vpolicy;    // v scrollbar visibility policy
        bool v_btn_dn;              // true when button pressed
        bool v_seen_expose;         // set true on forst expose event
        bool v_iso8859;             // if true, assume iso-8859-1
                                    //  text strings, else utf-8
    };
}

#endif

