
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

#ifndef GTKLPAL_H
#define GTKLPAL_H

// Number of layer entries.
#define LP_PALETTE_COLS 5
#define LP_PALETTE_ROWS 3

// Number of text lines at top.
#define LP_TEXT_LINES 5

struct sLpalette : public gtk_draw
{
    sLpalette(GRobject);
    ~sLpalette();

    GtkWidget *shell() { return (lp_shell); }

    void update_info(CDl*);
    void update_layer(CDl*);

private:
    void update_user(CDl*, int, int);
    void init_size();
    void redraw();
    void refresh(int, int, int, int);
    void b1_handler(int, int, int, bool);
    void b2_handler(int, int, int, bool);
    void b3_handler(int, int, int, bool);
    CDl *ldesc_at(int, int);
    bool remove(int, int);

    static void lp_cancel_proc(GtkWidget*, void*);
    static void lp_help_proc(GtkWidget*, void*);
    static int lp_resize_hdlr(GtkWidget*, GdkEvent*, void*);
    static int lp_redraw_hdlr(GtkWidget*, GdkEvent*, void*);
    static int lp_button_down_hdlr(GtkWidget*, GdkEvent*, void*);
    static int lp_button_up_hdlr(GtkWidget*, GdkEvent*, void*);
    static int lp_motion_hdlr(GtkWidget*, GdkEvent*, void*);
    static void lp_drag_begin(GtkWidget*, GdkDragContext*, gpointer);
    static void lp_drag_end(GtkWidget*, GdkDragContext*, gpointer);
    static void lp_drag_data_get(GtkWidget*, GdkDragContext*,
        GtkSelectionData*, guint, guint, void*);
    static void lp_drag_data_received(GtkWidget*, GdkDragContext*, gint, gint,
        GtkSelectionData*, guint, guint);
    static void lp_font_change_hdlr(GtkWidget*, void*, void*);
    static void lp_recall_proc(GtkWidget*, void*);
    static void lp_save_proc(GtkWidget*, void*);
    static int lp_popup_menu(GtkWidget*, GdkEvent*, void*);

    GRobject lp_caller;
    GtkWidget *lp_shell;
    GtkWidget *lp_remove;

    CDl *lp_history[LP_PALETTE_COLS];
    CDl *lp_user[LP_PALETTE_COLS * LP_PALETTE_ROWS];

    sGbag lp_gbag;              // private drawing context

    GdkPixmap *lp_pixmap;
    int lp_pmap_width;  
    int lp_pmap_height;
    bool lp_pmap_dirty;

    int lp_drag_x;
    int lp_drag_y;
    bool lp_dragging;

    int lp_hist_y;
    int lp_user_y;
    int lp_line_height;
    int lp_box_dimension;
    int lp_box_text_spacing;
    int lp_entry_width;
};

#endif

