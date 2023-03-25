
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

//
// Header for the layer table composite
//

#ifndef GTKLTAB_H
#define GTKLTAB_H

#include "layertab.h"
#include "events.h"

inline class GTKltab *gtkLtab();

class GTKltab : public cLtab, public GTKdraw
{
    static GTKltab *ptr() { return (instancePtr); }

public:
    friend inline GTKltab *gtkLtab() { return (GTKltab::ptr()); }

    GTKltab(bool);

    void setup_drawable();
    void blink(CDl*);
    void show(const CDl* = 0);
    void refresh(int, int, int, int);
    void win_size(int*, int*);
    void update();
    void update_scrollbar();
    void hide_layer_table(bool);
    void set_layer();

    GtkWidget *container()      { return (ltab_container); }
    GtkWidget *scrollbar()      { return (ltab_scrollbar); }
    GtkWidget *searcher()       { return (ltab_search_container); }

private:
    static int ltab_resize_hdlr(GtkWidget*, GdkEvent*, void*);
#if GTK_CHECK_VERSION(3,0,0)
    static int ltab_redraw_hdlr(GtkWidget*, cairo_t*, void*);
#else
    static int ltab_redraw_hdlr(GtkWidget*, GdkEvent*, void*);
#endif
    static int ltab_button_down_hdlr(GtkWidget*, GdkEvent*, void*);
    static int ltab_button_up_hdlr(GtkWidget*, GdkEvent*, void*);
    static int ltab_motion_hdlr(GtkWidget*, GdkEvent*, void*);
    static void ltab_drag_begin(GtkWidget*, GdkDragContext*, gpointer);
    static void ltab_drag_end(GtkWidget*, GdkDragContext*, gpointer);
    static void ltab_drag_data_get(GtkWidget*, GdkDragContext*,
        GtkSelectionData*, guint, guint, void*);
    static void ltab_drag_data_received(GtkWidget*, GdkDragContext*, gint, gint,
        GtkSelectionData*, guint, guint, void*);
    static void ltab_font_change_hdlr(GtkWidget*, void*, void*);
    static void ltab_scroll_change_proc(GtkAdjustment*, void*);
    static int ltab_scroll_event_hdlr(GtkWidget*, GdkEvent*, void*);
    static int ltab_ent_timer(void*);
    static void ltab_search_hdlr(GtkWidget*, void*);
    static void ltab_activate_proc(GtkWidget*, void*);

    GtkWidget *ltab_container;  // top-level subwidget for export
    GtkWidget *ltab_scrollbar;  // scroll bar

    GtkWidget *ltab_search_container;
                                // a text entry and search button
    GtkWidget *ltab_entry;
    GtkWidget *ltab_sbtn;
    GtkWidget *ltab_lsearch;
    GtkWidget *ltab_lsearchn;

#ifdef NEW_NDK
#else
    GdkPixmap *ltab_pixmap;
#endif
    int ltab_pmap_width;
    int ltab_pmap_height;
    bool ltab_pmap_dirty;
    bool ltab_hidden;
    int ltab_last_index;
    char *ltab_search_str;
    int ltab_timer_id;
    int ltab_last_mode;
    double ltab_sbvals[2];

    static GTKltab *instancePtr;
};

#endif

