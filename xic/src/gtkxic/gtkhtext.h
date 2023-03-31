
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
// Header for the "hypertext" editor and related
//

#ifndef GTKHTEXT_H
#define GTKHTEXT_H

#include "cd_hypertext.h"
#include "promptline.h"

// String storage registers, 0 is "last", 1-5 are general.
#define PE_NUMSTORES 6

inline class GTKedit *gtkEdit();

class GTKedit : public cPromptEdit, public GTKdraw
{
    static GTKedit *ptr() { return (instancePtr); }

public:
    friend inline GTKedit *gtkEdit() { return (GTKedit::ptr()); }

    GTKedit(bool);

    // virtual overrides
    void flash_msg(const char*, ...);
    void flash_msg_here(int, int, const char*, ...);
    void save_line();
    int win_width(bool = false);
    int win_height();
    void set_focus();
    void set_indicate();
    void show_lt_button(bool);
    void get_selection(bool);
    void *setup_backing(bool);
    void restore_backing(void*);
    void init_window();
    bool check_pixmap();
    void init_selection(bool);
    void warp_pointer();

    GtkWidget *container()  { return (pe_container); }
    GtkWidget *keys()       { return (pe_keys); }
    int xpos()              { return (pe_colmin*pe_fntwid); }

private:
    static void pe_l_btn_hdlr(GtkWidget*, void*, void*);
    static int pe_popup_btn_proc(GtkWidget*, GdkEvent*, void*);
    static void pe_r_menu_proc(GtkWidget*, void*);
    static void pe_s_menu_proc(GtkWidget*, void*);
    static int pe_keys_hdlr(GtkWidget*, GdkEvent*, void*);
    static int pe_redraw_idle(void*);
    static int pe_map_hdlr(GtkWidget*, GdkEvent*, void*);
    static int pe_resize_hdlr(GtkWidget*, GdkEvent*, void*);
#if GTK_CHECK_VERSION(3,0,0)
    static int pe_redraw_hdlr(GtkWidget*, cairo_t*, void*);
#else
    static int pe_redraw_hdlr(GtkWidget*, GdkEvent*, void*);
#endif
    static int pe_btn_hdlr(GtkWidget*, GdkEvent*, void*);
    static int pe_enter_hdlr(GtkWidget*, GdkEvent*, void*);
    static int pe_leave_hdlr(GtkWidget*, GdkEvent*, void*);
    static void pe_selection_proc(GtkWidget*, GtkSelectionData*, guint, void*);
    static void pe_font_change_hdlr(GtkWidget*, void*, void*);
    static void pe_drag_data_received(GtkWidget*, GdkDragContext*, gint, gint,
        GtkSelectionData*, guint, guint, void*);
    static int pe_motion_hdlr(GtkWidget*, GdkEvent*, void*);
    static int pe_selection_clear(GtkWidget*, GdkEventSelection*, void*);
    static void pe_selection_get(GtkWidget*, GtkSelectionData*, guint, guint,
        gpointer);

    GtkWidget *pe_container;
    GtkWidget *pe_keys;
    GtkWidget *pe_keyframe;
    GtkWidget *pe_r_button;
    GtkWidget *pe_s_button;
    GtkWidget *pe_l_button;
    GtkWidget *pe_r_menu;
    GtkWidget *pe_s_menu;

    int pe_id;             // redraw idle function id
    int pe_wid, pe_hei;    // drawing area size
#if GTK_CHECK_VERSION(3,0,0)
#else
    GdkPixmap *pe_pixmap;  // backing pixmap
#endif

    static hyList *pe_stores[PE_NUMSTORES]; // editor text string registers

    static GTKedit *instancePtr;
};

#endif

