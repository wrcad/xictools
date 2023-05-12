
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
#include "dsp_color.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_gdsii.h"
#include "keymap.h"
#include "tech.h"
#include "menu.h"
#include "attr_menu.h"
#include "gtkmain.h"
#include "gtklpal.h"
#include "gtkltab.h"
#include "gtkinterf/gtkfont.h"


//-----------------------------------------------------------------------------
// The Layer Palette.  The top third is a text field that displays
// information about a layer under the pointer, or the current layer.
// The middle third contains icons for the last few current layer
// choices.  The bottom third is a user-configurable palette.  The
// user can drag their favorite layers into this area.  Clicking on
// any of the layer icons will set the current layer.
//
// Help system keywords used:
//  xic:ltpal

// gtkfillp.cc
extern const char *fillpattern_xpm[];

namespace {
    // The layer table is a dnd source for fill patterns, and a receiver for
    // fillpatterns and colors.
    //
    GtkTargetEntry lp_targets[] = {
        { (char*)"fillpattern", 0, 0 },
        { (char*)"application/x-color", 0, 1 }
    };
    guint n_lp_targets = sizeof(lp_targets) / sizeof(lp_targets[0]);

    // These give the widget areas distinctive backgrounds.

    unsigned int text_backg()
    {
        return (DSP()->Color(PromptEditFocusBackgColor));
    }

    unsigned int hist_backg()
    {
        return (DSP()->Color(PromptEditBackgColor));
    }

    unsigned int user_backg()
    {
        return (DSP()->Color(PromptBackgroundColor));
    }
}

namespace { sLpalette *Lpal; }


void
cMain::PopUpLayerPalette(GRobject caller, ShowMode mode, bool showinfo,
    CDl *ldesc)
{
    if (!GTKdev::exists() || !GTKmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete Lpal;
        return;
    }
    if (mode == MODE_UPD) {
        if (Lpal) {
            if (showinfo)
                Lpal->update_info(ldesc);
            else
                Lpal->update_layer(ldesc);
        }
        return;
    }
    if (Lpal)
        return;

    new sLpalette(caller);
    if (!Lpal->shell()) {
        delete Lpal;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Lpal->shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GTKdev::self()->SetPopupLocation(GRloc(), Lpal->shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(Lpal->shell());
}


sLpalette::sLpalette(GRobject caller) : GTKdraw(XW_LPAL)
{
    Lpal = this;
    lp_caller = caller;
    lp_shell = 0;
    lp_remove = 0;
    memset(lp_history, 0, LP_PALETTE_COLS * sizeof(CDl*));
    memset(lp_user, 0, LP_PALETTE_COLS * LP_PALETTE_ROWS * sizeof(CDl*));
#if GTK_CHECK_VERSION(3,0,0)
#else
    lp_pixmap = 0;
#endif
    lp_pmap_width = 0;
    lp_pmap_height = 0;
    lp_pmap_dirty = false;

    lp_drag_x = 0;
    lp_drag_y = 0;
    lp_dragging = false;

    lp_hist_y = 0;
    lp_user_y = 0;
    lp_line_height = 0;
    lp_box_dimension = 0;
    lp_box_text_spacing = 0;
    lp_entry_width = 0;

    lp_shell = gtk_NewPopup(0, "Layer Palette", lp_cancel_proc, 0);
    if (!lp_shell)
        return;
    gtk_window_set_resizable(GTK_WINDOW(lp_shell), false);

    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(lp_shell), form);

    GtkWidget *row = gtk_hbox_new(false, 0);
    gtk_widget_show(row);

    GtkWidget *recall_btn = gtk_button_new_with_label("Recall");
    gtk_widget_set_name(recall_btn, "Recall");
    gtk_widget_show(recall_btn);

    GtkWidget *recall_menu = gtk_menu_new();
    gtk_widget_set_name(recall_menu, "Recall");
    for (int i = 1; i < 8; i++) {
        char buf[16];
        sprintf(buf, "Reg %d", i);
        GtkWidget *mi = gtk_menu_item_new_with_label(buf);
        gtk_widget_set_name(mi, buf);
        gtk_widget_show(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(recall_menu), mi);
        g_signal_connect(G_OBJECT(mi), "activate",
            G_CALLBACK(lp_recall_proc), (void*)(intptr_t)i);
    }
    g_signal_connect(G_OBJECT(recall_btn), "button-press-event",
        G_CALLBACK(lp_popup_menu), recall_menu);
    gtk_box_pack_start(GTK_BOX(row), recall_btn, true, true, 2);

    GtkWidget *save_btn = gtk_button_new_with_label("Save");
    gtk_widget_set_name(save_btn, "Save");
    gtk_widget_show(save_btn);

    GtkWidget *save_menu = gtk_menu_new();
    gtk_widget_set_name(save_menu, "Save");
    for (int i = 1; i < 8; i++) {
        char buf[16];
        sprintf(buf, "Reg %d", i);
        GtkWidget *mi = gtk_menu_item_new_with_label(buf);
        gtk_widget_set_name(mi, buf);
        gtk_widget_show(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(save_menu), mi);
        g_signal_connect(G_OBJECT(mi), "activate",
            G_CALLBACK(lp_save_proc), (void*)(intptr_t)i);
    }
    g_signal_connect(G_OBJECT(save_btn), "button-press-event",
        G_CALLBACK(lp_popup_menu), save_menu);
    gtk_box_pack_start(GTK_BOX(row), save_btn, true, true, 2);

    lp_remove = gtk_toggle_button_new_with_label("Remove");
    gtk_widget_set_name(lp_remove, "Remove");
    gtk_widget_show(lp_remove);
    gtk_box_pack_start(GTK_BOX(row), lp_remove, true, true, 2);

    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(lp_help_proc), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);

    int rowcnt = 0;
    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 4, 2);
    rowcnt++;

    gd_viewport = gtk_drawing_area_new();
    gtk_widget_set_double_buffered(gd_viewport, false);
    gtk_widget_set_name(gd_viewport, "LayerPalette");
    gtk_widget_show(gd_viewport);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), gd_viewport);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 4, 2);
    rowcnt++;

    gtk_widget_add_events(gd_viewport, GDK_STRUCTURE_MASK);
    g_signal_connect(G_OBJECT(gd_viewport), "configure-event",
        G_CALLBACK(lp_resize_hdlr), 0);
#if GTK_CHECK_VERSION(3,0,0)
    g_signal_connect(G_OBJECT(gd_viewport), "draw",
        G_CALLBACK(lp_redraw_hdlr), 0);
#else
    gtk_widget_add_events(gd_viewport, GDK_EXPOSURE_MASK);
    g_signal_connect(G_OBJECT(gd_viewport), "expose-event",
        G_CALLBACK(lp_redraw_hdlr), 0);
#endif
    gtk_widget_add_events(gd_viewport, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(gd_viewport), "button-press-event",
        G_CALLBACK(lp_button_down_hdlr), 0);
    gtk_widget_add_events(gd_viewport, GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(G_OBJECT(gd_viewport), "button-release-event",
        G_CALLBACK(lp_button_up_hdlr), 0);
    gtk_widget_add_events(gd_viewport, GDK_POINTER_MOTION_MASK);
    g_signal_connect(G_OBJECT(gd_viewport), "motion-notify-event",
        G_CALLBACK(lp_motion_hdlr), 0);
    g_signal_connect(G_OBJECT(gd_viewport), "drag-begin",
        G_CALLBACK(lp_drag_begin), 0);
    g_signal_connect(G_OBJECT(gd_viewport), "drag-end",
        G_CALLBACK(lp_drag_end), 0);
    g_signal_connect(G_OBJECT(gd_viewport), "drag-data-get",
        G_CALLBACK(lp_drag_data_get), 0);
    gtk_drag_dest_set(gd_viewport, GTK_DEST_DEFAULT_ALL, lp_targets,
        n_lp_targets, GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(gd_viewport), "drag-data-received",
        G_CALLBACK(lp_drag_data_received), 0);
    g_signal_connect(G_OBJECT(gd_viewport), "style-set",
        G_CALLBACK(lp_font_change_hdlr), 0);

    GTKfont::setupFont(gd_viewport, FNT_SCREEN, true);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(lp_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    init_size();
    gtk_window_set_focus(GTK_WINDOW(lp_shell), button);
    lp_recall_proc(0, 0);
}


sLpalette::~sLpalette()
{
    lp_save_proc(0, 0);

    Lpal = 0;
    SetGbag(0);
    if (lp_caller)
        GTKdev::Deselect(lp_caller);
    if (lp_shell)
        gtk_widget_destroy(lp_shell);
#if GTK_CHECK_VERSION(3,0,0)
#else
    if (lp_pixmap)
        gdk_pixmap_unref(lp_pixmap);
#endif
}


// Update the info text.
//
void
sLpalette::update_info(CDl *ldesc)
{
#if GTK_CHECK_VERSION(3,0,0)
    if (!GetDrawable()->get_window())
        GetDrawable()->set_window(gtk_widget_get_window(gd_viewport));
    if (!GetDrawable()->get_window())
        return;
    int win_width = GetDrawable()->get_width();
    GetDrawable()->set_draw_to_pixmap();
#else
    if (!gd_window)
        gd_window = gtk_widget_get_window(gd_viewport);
    if (!gd_window)
        return;

    int win_width = gdk_window_get_width(gd_window);
    int win_height = gdk_window_get_height(gd_window);
    if (!lp_pixmap || lp_pmap_width != win_width ||
            lp_pmap_height != win_height) {
        if (lp_pixmap)
            gdk_pixmap_unref(lp_pixmap);
        lp_pmap_width = win_width;
        lp_pixmap = gdk_pixmap_new(gd_window, lp_pmap_width, lp_pmap_height,
            gdk_visual_get_depth(GTKdev::self()->Visual()));
        lp_pmap_dirty = true;
    }

    GdkWindow *win = gd_window;
    gd_window = lp_pixmap;
#endif

    int fwid, fhei;
    TextExtent(0, &fwid, &fhei);
    int x = 2;
    int y = fhei;

    SetColor(text_backg());
    SetFillpattern(0);
    SetBackground(text_backg());
    Box(0, 0, win_width, LP_TEXT_LINES*fhei + 2);

    unsigned long c1 = DSP()->Color(PromptTextColor);
    unsigned long c2 = DSP()->Color(PromptEditTextColor);
    const char *str;

    char buf[64];
    const char *lname = CDldb()->getOAlayerName(ldesc->oaLayerNum());
    if (!lname)
        lname = "";
    snprintf(buf, 32, " (%d)", ldesc->oaLayerNum());
    SetColor(c1);
    str = "name (num): ";
    Text(str, x, y, 0);
    x += GTKfont::stringWidth(gd_viewport, str);
    SetColor(c2);
    Text(lname, x, y, 0);
    x += GTKfont::stringWidth(gd_viewport, lname);
    Text(buf, x, y, 0);

    y += fhei;
    x = 2;

    const char *pname = CDldb()->getOApurposeName(ldesc->oaPurposeNum());
    if (!pname)
        pname = CDL_PRP_DRAWING;
    snprintf(buf, 32, " (%d)", ldesc->oaPurposeNum());
    SetColor(c1);
    str = "purpose (num): ";
    Text(str, x, y, 0);
    x += GTKfont::stringWidth(gd_viewport, str);
    SetColor(c2);
    Text(pname, x, y, 0);
    x += GTKfont::stringWidth(gd_viewport, pname);
    Text(buf, x, y, 0);

    y += fhei;
    x = 2;

    SetColor(c1);
    str = "alias: ";
    Text(str, x, y, 0);
    x += GTKfont::stringWidth(gd_viewport, str);
    if (ldesc->lppName()) {
        SetColor(c2);
        Text(ldesc->lppName(), x, y, 0);
    }

    y += fhei;
    x = 2;

    SetColor(c1);
    str = "descr: ";
    Text(str, x, y, 0);
    x += GTKfont::stringWidth(gd_viewport, str);
    if (ldesc->description()) {
        SetColor(c2);
        Text(ldesc->description(), x, y, 0);
    }

    y += fhei;
    x = 2;

    bool hasmap = false;
    int l = -1, d = -1;
    if (ldesc->strmOut()) {
        l = ldesc->strmOut()->layer();
        d = ldesc->strmOut()->dtype();
        hasmap = true;
    }
    else if (strmdata::hextrn(ldesc->name(), &l, &d))
        hasmap = true;
    if (hasmap) {
        SetColor(c1);
        str = "GDSII layer/dtype: ";
        Text(str, x, y, 0);
        x += GTKfont::stringWidth(gd_viewport, str);

        if (l > 255 || d > 255)
            sprintf(buf, "%d (%04Xh) / ", l, l);
        else
            sprintf(buf, "%d (%02Xh) / ", l, l);
        SetColor(c2);
        Text(buf, x, y, 0);
        x += GTKfont::stringWidth(gd_viewport, buf);
        if (l > 255 || d > 255)
            sprintf(buf, "%d (%04Xh)", d, d);
        else
            sprintf(buf, "%d (%02Xh)", d, d);
        Text(buf, x, y, 0);
    }
#if GTK_CHECK_VERSION(3,0,0)
    GetDrawable()->set_draw_to_window();
    GetDrawable()->copy_pixmap_to_window(GC(), 0, 0, win_width, 5*fhei);
#else
    gdk_window_copy_area(win, GC(), 0, 0, gd_window, 0, 0, win_width, 5*fhei);
    gd_window = win;
#endif
}


// Update the layer history list.
//
void
sLpalette::update_layer(CDl *ldesc)
{
    if (ldesc) {
        if (!CDldb()->findLayer(ldesc->name(), DSP()->CurMode())) {
            // The ldesc was removed from the layer table, purge it
            // from the lists.
            for (int i = 0; i < LP_PALETTE_COLS; i++) {
                if (ldesc == lp_history[i]) {
                    for (int j = i+1; j < LP_PALETTE_COLS; j++)
                        lp_history[j-1] = lp_history[j];
                    lp_history[LP_PALETTE_COLS-1] = 0;
                    break;
                }
            }
            int usz = LP_PALETTE_COLS * LP_PALETTE_ROWS;
            for (int i = 0; i < usz; i++) {
                if (ldesc == lp_user[i]) {
                    for (int j = i+1; j < usz; j++)
                        lp_user[j-1] = lp_user[j];
                    lp_user[usz-1] = 0;
                    break;
                }
            }
        }
        else {
            for (int i = 0; i < LP_PALETTE_COLS; i++) {
                if (ldesc == lp_history[i]) {
                    if (i == 0)
                        return;
                    for (int j = i; j > 0; j--)
                        lp_history[j] = lp_history[j-1];
                    lp_history[0] = ldesc;
                    lp_pmap_dirty = true;
                    refresh(0, 0, 0, 0);
                    return;
                }
            }
            for (int i = LP_PALETTE_COLS - 1; i > 0; i--)
                lp_history[i] = lp_history[i-1];
            lp_history[0] = ldesc;
        }
    }
    lp_pmap_dirty = true;
    refresh(0, 0, 0, 0);
}


void
sLpalette::update_user(CDl *ldesc, int x, int y)
{
    int col = (x-2)/lp_entry_width;
    if (col < 0)
        col = 0;
    else if (col >= LP_PALETTE_COLS)
        col = LP_PALETTE_COLS - 1;
    int row = (y - lp_user_y)/(lp_user_y - lp_hist_y);
    if (row < 0)
        row = 0;
    else if (row >= LP_PALETTE_ROWS)
        row = LP_PALETTE_ROWS-1;
    int indx = row*LP_PALETTE_COLS + col;

    int usz = LP_PALETTE_COLS * LP_PALETTE_ROWS;
    for (int i = 0; i < usz; i++) {
        if (!lp_user[i]) {
            if (indx > i)
                indx = i;
            break;
        }
    }

    int sindx = -1;
    for (int i = 0; i < usz; i++) {
        if (ldesc == lp_user[i]) {
            sindx = i;
            break;
        }
    }
    if (sindx < 0) {
        for (int i = usz - 1; i > indx; i--)
            lp_user[i] = lp_user[i-1];
        lp_user[indx] = ldesc;
    }
    else if (sindx < indx) {
        if (!lp_user[indx]) {
            indx--;
            if (indx == sindx)
                return;
        }
        for (int i = sindx; i < indx; i++)
            lp_user[i] = lp_user[i+1];
        lp_user[indx] = ldesc;
    }
    else if (sindx > indx) {
        for (int i = sindx; i > indx; i--)
            lp_user[i] = lp_user[i-1];
        lp_user[indx] = ldesc;
    }
    else
        return;
    lp_pmap_dirty = true;
    refresh(0, 0, 0, 0);
}


void
sLpalette::init_size()
{
    int fwid, fhei;
    TextExtent(0, &fwid, &fhei);
    lp_hist_y = LP_TEXT_LINES*fhei + fhei/2 + 4;
    lp_user_y = lp_hist_y + 2*fhei + fhei/2;

    int y_offset = 4;
    lp_line_height = 2*fhei + 2;
    lp_box_dimension = lp_line_height - 2*y_offset;
    lp_box_text_spacing = lp_box_dimension/4;
    lp_entry_width = lp_box_dimension + 3*lp_box_text_spacing + 3*fwid + 4;

    int wid = LP_PALETTE_COLS*lp_entry_width + 4;
    int hei = lp_user_y + (2*fhei + fhei/2)*LP_PALETTE_ROWS;
    gtk_widget_set_size_request(Lpal->gd_viewport, wid, hei);
}


// Redraw the two sample areas, not the text area.
//
void
sLpalette::redraw()
{
#if GTK_CHECK_VERSION(3,0,0)
    int win_width = GetDrawable()->get_width();
    int win_height = GetDrawable()->get_height();
#else
    int win_width = gdk_window_get_width(gd_window);
    int win_height = gdk_window_get_height(gd_window);
#endif
    SetFillpattern(0);
    SetColor(user_backg());
    Box(0, lp_hist_y - 8, win_width, lp_hist_y - 6);
    SetColor(hist_backg());
    Box(0, lp_hist_y - 6, win_width, lp_user_y - 6);
    SetColor(user_backg());
    Box(0, lp_user_y - 6, win_width, win_height);

    int backg = DSP()->Color(BackgroundColor);
    char buf[128];
    for (int xx = 0; xx < 2; xx++) {
        CDl **lary = xx ? lp_user : lp_history;
        int y = xx ? lp_user_y : lp_hist_y;
        int x = 4;
        int sz = LP_PALETTE_COLS;
        if (xx)
            sz *= LP_PALETTE_ROWS;
        for (int i = 0; i < sz; i++) {
            CDl *ld = lary[i];
            if (!ld)
                continue;

            if (i && i % LP_PALETTE_COLS == 0) {
                y += (lp_user_y - lp_hist_y);
                x = 4;
            }
            SetFillpattern(0);
            SetColor(backg);
            Box(x-4, y-4, x + lp_box_dimension + 4, y + lp_box_dimension + 4);

            SetColor(dsp_prm(ld)->pixel());
            if (!ld->isInvisible()) {
                if (ld->isFilled()) {
                    SetFillpattern(dsp_prm(ld)->fill());
                    Box(x, y, x + lp_box_dimension, y + lp_box_dimension);
                    if (dsp_prm(ld)->fill()) {
                        SetFillpattern(0);
                        if (ld->isOutlined()) {
                            SetLinestyle(0);
                            GRmultiPt xp(5);
                            if (ld->isOutlinedFat()) {
                                xp.assign(0, x+1, y+1);
                                xp.assign(1, x+1, y + lp_box_dimension-1);
                                xp.assign(2, x + lp_box_dimension-1, y +
                                    lp_box_dimension-1);
                                xp.assign(3, x + lp_box_dimension-1, y+1);
                                xp.assign(4, x+1, y+1);
                                PolyLine(&xp, 5);
                            }
                            xp.assign(0, x, y);
                            xp.assign(1, x, y + lp_box_dimension);
                            xp.assign(2, x + lp_box_dimension,
                                y + lp_box_dimension);
                            xp.assign(3, x + lp_box_dimension, y);
                            xp.assign(4, x, y);
                            PolyLine(&xp, 5);
                        }
                        if (ld->isCut()) {
                            SetLinestyle(0);
                            Line(x, y, x + lp_box_dimension,
                                y + lp_box_dimension);
                            Line(x, y + lp_box_dimension,
                                x + lp_box_dimension, y);
                        }
                    }
                }
                else {
                    GRmultiPt xp(5);
                    SetFillpattern(0);
                    if (ld->isOutlined()) {
                        if (ld->isOutlinedFat()) {
                            xp.assign(0, x+1, y+1);
                            xp.assign(1, x+1, y + lp_box_dimension-1);
                            xp.assign(2, x + lp_box_dimension-1,
                                y + lp_box_dimension-1);
                            xp.assign(3, x + lp_box_dimension-1, y+1);
                            xp.assign(4, x+1, y+1);
                            PolyLine(&xp, 5);
                        }
                        else
                            SetLinestyle(DSP()->BoxLinestyle());
                    }
                    else
                        SetLinestyle(0);
                    xp.assign(0, x, y);
                    xp.assign(1, x, y + lp_box_dimension);
                    xp.assign(2, x + lp_box_dimension, y + lp_box_dimension);
                    xp.assign(3, x + lp_box_dimension, y);
                    xp.assign(4, x, y);
                    PolyLine(&xp, 5);
                    if (ld->isCut()) {
                        Line(x, y, x + lp_box_dimension, y + lp_box_dimension);
                        Line(x, y + lp_box_dimension, x + lp_box_dimension, y);
                    }
                }
            }
            SetFillpattern(0);
            SetLinestyle(0);
            if (ld->isNoSelect())
                SetColor(DSP()->Color(GUIcolorDvSl));
            else if (xx)
                SetColor(user_backg());
            else
                SetColor(hist_backg());
            Box(x + lp_box_dimension + lp_box_text_spacing, y,
                x + lp_entry_width - lp_box_text_spacing - 4,
                y + lp_box_dimension);

            SetColor(DSP()->Color(PromptEditTextColor));
            int y_text_fudge = 2;
            int yt = y + lp_box_dimension + y_text_fudge;
            int xt = x + lp_box_dimension + lp_box_text_spacing + 2;
            sprintf(buf, "%d", ld->index(DSP()->CurMode()));
            Text(buf, xt, yt, 0);

            SetColor(DSP()->Color(PromptCursorColor));
            if (ld == LT()->CurLayer()) {
                SetLinestyle(0);
                GRmultiPt xp(5);
                xp.assign(0, x - 2, y - 2);
                xp.assign(1,
                    xp.at(0).x, xp.at(0).y + lp_box_dimension + 4);
                xp.assign(2,
                    xp.at(1).x + lp_entry_width - lp_box_text_spacing,
                    xp.at(1).y);
                xp.assign(3,
                    xp.at(2).x, xp.at(2).y - lp_box_dimension - 4);
                xp.assign(4,
                    xp.at(3).x - lp_entry_width + lp_box_text_spacing,
                    xp.at(3).y);
                PolyLine(&xp, 5);
            }

            x += lp_entry_width;
        }
    }
    lp_pmap_dirty = true;
}


// Exposure redraw, avoids flicker.
//
void
sLpalette::refresh(int x, int y, int w, int h)
{
#if GTK_CHECK_VERSION(3,0,0)
    if (!GetDrawable()->get_window())
        GetDrawable()->set_window(gtk_widget_get_window(gd_viewport));
    if (!GetDrawable()->get_window())
        return;
    int win_width = GetDrawable()->get_width();
    int win_height = GetDrawable()->get_height();
    lp_pmap_dirty = GetDrawable()->set_draw_to_pixmap();
    if (w <= 0)
        w = win_width;
    if (h <= 0)
        h = win_height;
#else
    if (!gd_window)
        gd_window = gtk_widget_get_window(gd_viewport);
    if (!gd_window)
        return;

    int win_width = gdk_window_get_width(gd_window);
    int win_height = gdk_window_get_height(gd_window);
    if (w <= 0)
        w = win_width;
    if (h <= 0)
        h = win_height;

    if (!lp_pixmap || lp_pmap_width != win_width ||
            lp_pmap_height != win_height) {
        // Widget is not currently resizable.
        if (lp_pixmap)
            gdk_pixmap_unref(lp_pixmap);
        lp_pmap_width = win_width;
        lp_pmap_height = win_height;
        lp_pixmap = gdk_pixmap_new(gd_window, lp_pmap_width, lp_pmap_height,
            gdk_visual_get_depth(GTKdev::self()->Visual()));
        lp_pmap_dirty = true;
    }

    GdkWindow *win = gd_window;
    gd_window = lp_pixmap;
#endif

    if (lp_pmap_dirty) {
        redraw();
        lp_pmap_dirty = false;
    }

#if GTK_CHECK_VERSION(3,0,0)
    GetDrawable()->set_draw_to_window();
    GetDrawable()->copy_pixmap_to_window(GC(), x, y, w, h);
#else
    gdk_window_copy_area(win, GC(), x, y, gd_window, x, y, w, h);
    gd_window = win;
#endif
}


// Button1 press/release action.  Reset the current layer to the one
// pointed to, and perform necessary actions.
//
void
sLpalette::b1_handler(int x, int y, int state, bool down)
{
    if (down) {
        if (remove(x, y)) {
            lp_pmap_dirty = true;
            refresh(0, 0, 0, 0);
            return;
        }
        CDl *ld = ldesc_at(x, y);
        if (!ld)
            return;
        if (state & (GR_SHIFT_MASK | GR_CONTROL_MASK)) {
            if (state & GR_SHIFT_MASK)
                LT()->SetLayerVisibility(LTtoggle, ld, 
                    (DSP()->CurMode() == Electrical) || !LT()->NoPhysRedraw());
            if (state & GR_CONTROL_MASK)
                LT()->SetLayerSelectability(LTtoggle, ld);
            lp_pmap_dirty = true;
            refresh(0, 0, 0, 0);
            return;
        }

        lp_dragging = true;
        lp_drag_x = x;
        lp_drag_y = y;
        LT()->HandleLayerSelect(ld);
    }
    else
        lp_dragging = false;
}


// Button2 action.  This toggles the visibility of the layers.  The
// sample box is not shown for invisible layers.  The screen is not
// redrawn after this operation unless the shift key is pressed while
// making the selection.
//
// In electrical mode, the drawing layer is always visible.  Instead
// the action on this layer is to turn the fill attribute on/off,
// changing (for now) whether dots are filled or outlined only.
//
void
sLpalette::b2_handler(int x, int y, int state, bool down)
{
    if (down) {
        CDl *ld = ldesc_at(x, y);
        if (!ld)
            return;

        LT()->SetLayerVisibility(LTtoggle, ld,
            ((DSP()->CurMode() == Electrical) ||
            (LT()->NoPhysRedraw() && (state & GR_SHIFT_MASK)) ||
            (!LT()->NoPhysRedraw() && !(state & GR_SHIFT_MASK))));
        lp_pmap_dirty = true;
        refresh(0, 0, 0, 0);
    }
}


// Button3 action.  This toggles blinking of the layer.  When activated,
// a timer function periodically alters the color table entry used by the
// selected layer(s).
//
void
sLpalette::b3_handler(int x, int y, int state, bool down)
{
    if (down) {
        CDl *ld = ldesc_at(x, y);
        if (!ld)
            return;

        bool ctrl = (state & GR_CONTROL_MASK);
        bool shft = (state & GR_SHIFT_MASK);
        if (ctrl || shft) {
            LT()->HandleLayerSelect(ld);

            if (ctrl && !shft)
                Menu()->MenuButtonPress(MMmain, MenuCOLOR);
            else if (!ctrl && shft)
                Menu()->MenuButtonPress(MMmain, MenuFILL);
            else if (ctrl && shft)
                Menu()->MenuButtonPress(MMmain, MenuLPEDT);
        }
        else if (GTKpkg::self()->IsTrueColor())
            gtkLtab()->blink(ld);
        else {
            if (ld->isBlink()) {
                ld->setBlink(false);
                DspLayerParams *lp = dsp_prm(ld);
                int pix;
                DefineColor(&pix, lp->red(), lp->green(), lp->blue());
                lp->set_pixel(pix);
            }
            else
                ld->setBlink(true);
        }
    }
}


CDl *
sLpalette::ldesc_at(int x, int y)
{
    if (y < lp_hist_y)
        return (0);
    int col = (x - 2)/lp_entry_width;
    if (col < 0)
        col = 0;
    else if (col >= LP_PALETTE_COLS)
        col = LP_PALETTE_COLS - 1;

    if (y < lp_user_y)
        return (lp_history[col]);

    int row = (y - lp_user_y)/(lp_user_y - lp_hist_y);
    if (row < 0)
        row = 0;
    else if (row > LP_PALETTE_ROWS-1)
        row = LP_PALETTE_ROWS-1;
    return (lp_user[row*LP_PALETTE_COLS + col]);
}


// If clicked on an entry in the user area with the Remove button
// active, remove the entry and reset the button.
//
bool
sLpalette::remove(int x, int y)
{
    if (!GTKdev::GetStatus(lp_remove))
        return (false);
    if (y < lp_user_y)
        return (false);
    int row = (y - lp_user_y)/(lp_user_y - lp_hist_y);
    if (row < 0)
        row = 0;
    else if (row > LP_PALETTE_ROWS-1)
        row = LP_PALETTE_ROWS-1;
    int col = (x - 2)/lp_entry_width;
    if (col < 0)
        col = 0;
    else if (col >= LP_PALETTE_COLS)
        col = LP_PALETTE_COLS - 1;
    int ix = row*LP_PALETTE_COLS + col;
    if (lp_user[ix]) {
        int usz = LP_PALETTE_COLS * LP_PALETTE_ROWS;
        usz--;
        for (int i = ix; i < usz; i++)
            lp_user[i] = lp_user[i+1];
        lp_user[usz] = 0;
        GTKdev::SetStatus(lp_remove, false);
        return (true);
    }
    return (false);
}


// Static function.
void
sLpalette::lp_cancel_proc(GtkWidget*, void*)
{
    XM()->PopUpLayerPalette(0, MODE_OFF, false, 0);
}


// Static function.
void
sLpalette::lp_help_proc(GtkWidget*, void*)
{
    DSPmainWbag(PopUpHelp("xic:lpal"))
}


// Static function.
// Widget is not resizable.
int
sLpalette::lp_resize_hdlr(GtkWidget*, GdkEvent*, void*)
{
    if (!Lpal)
        return (0);
#if GTK_CHECK_VERSION(3,0,0)
    if (!Lpal->GetDrawable()->get_window()) {
        GdkWindow *window = gtk_widget_get_window(Lpal->gd_viewport);
        Lpal->GetDrawable()->set_window(window);
    }
#else
    if (!Lpal->gd_window)
        Lpal->gd_window = gtk_widget_get_window(Lpal->gd_viewport);
#endif
    Lpal->update_layer(LT()->CurLayer());
    Lpal->update_info(LT()->CurLayer());
    return (true);
}


// Static function.
// Redraw the drawing area.
//
#if GTK_CHECK_VERSION(3,0,0)
int
sLpalette::lp_redraw_hdlr(GtkWidget*, cairo_t *cr, void*)
#else
int
sLpalette::lp_redraw_hdlr(GtkWidget*, GdkEvent *event, void*)
#endif
{
#if GTK_CHECK_VERSION(3,0,0)
    cairo_rectangle_int_t rect;
    ndkDrawable::redraw_area(cr, &rect);
    Lpal->refresh(rect.x, rect.y, rect.width, rect.height);
#else
    GdkEventExpose *pev = (GdkEventExpose*)event;
    Lpal->refresh(pev->area.x, pev->area.y,
        pev->area.width, pev->area.height);
#endif
    return (true);
}


// Static function.
// Dispatch mouse button presses in the drawing area.
//
int
sLpalette::lp_button_down_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    if (event->type == GDK_2BUTTON_PRESS ||
            event->type == GDK_3BUTTON_PRESS)
        return (true);
    GdkEventButton *bev = (GdkEventButton*)event;
    int button = Kmap()->ButtonMap(bev->button);

    if (XM()->IsDoingHelp() && !GTKmainwin::is_shift_down()) {
        DSPmainWbag(PopUpHelp("xic:ltpal"))
        return (true);
    }

    switch (button) {
    case 1:
        if (Lpal)
            Lpal->b1_handler((int)bev->x, (int)bev->y, bev->state, true);
        break;
    case 2:
        if (Lpal)
            Lpal->b2_handler((int)bev->x, (int)bev->y, bev->state, true);
        break;
    case 3:
        if (Lpal)
            Lpal->b3_handler((int)bev->x, (int)bev->y, bev->state, true);
        break;
    }
    return (true);
}


// Static function.
int
sLpalette::lp_button_up_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    GdkEventButton *bev = (GdkEventButton*)event;
    int button = Kmap()->ButtonMap(bev->button);

    if (XM()->IsDoingHelp() && !GTKmainwin::is_shift_down())
        return (true);

    switch (button) {
    case 1:
        if (Lpal)
            Lpal->b1_handler((int)bev->x, (int)bev->y, bev->state, false);
        break;
    case 2:
        if (Lpal)
            Lpal->b2_handler((int)bev->x, (int)bev->y, bev->state, false);
        break;
    case 3:
        if (Lpal)
            Lpal->b3_handler((int)bev->x, (int)bev->y, bev->state, false);
        break;
    }
    return (true);
}


// Static function.
int
sLpalette::lp_motion_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (!Lpal)
        return (false);
    int x = (int)event->motion.x;
    int y = (int)event->motion.y;
    if (Lpal->lp_dragging &&
            (abs(x - Lpal->lp_drag_x) > 4 || abs(y - Lpal->lp_drag_y) > 4)) {
        Lpal->lp_dragging = false;
        // fillpattern only
        GtkTargetList *targets = gtk_target_list_new(lp_targets, 1);
        gtk_drag_begin(caller, targets, (GdkDragAction)GDK_ACTION_COPY,
            1, event);
    }

    CDl *ld = Lpal->ldesc_at(x, y);
    if (ld)
        Lpal->update_info(ld);

    return (true);
}


// Static function.
// Set the pixmap.
//
void
sLpalette::lp_drag_begin(GtkWidget*, GdkDragContext *context, gpointer)
{
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_xpm_data(fillpattern_xpm);
    gtk_drag_set_icon_pixbuf(context, pixbuf, -2, -2);
    GTKsubwin::HaveDrag = true;
}


// Static function.
void
sLpalette::lp_drag_end(GtkWidget*, GdkDragContext*, gpointer)
{
    GTKsubwin::HaveDrag = false;
}


// Static function.
// Initialize data for drag/drop transfer from 'this'.
//
void
sLpalette::lp_drag_data_get(GtkWidget*, GdkDragContext*,
    GtkSelectionData *data, guint, guint, void*)
{
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return;
    LayerFillData dd(ld);
    gtk_selection_data_set(data, gtk_selection_data_get_target(data),
        8, (unsigned char*)&dd, sizeof(LayerFillData));
}


// Static function.
// Drag data received from layer table, or from 'this'.  The layer is added
// to the user palette line.
//
void
sLpalette::lp_drag_data_received(GtkWidget*, GdkDragContext *context,
    gint x, gint y, GtkSelectionData *data, guint, guint time)
{
    // datum is a guint16 array of the format:
    //  R G B opacity
    GdkAtom a = gdk_atom_intern("fillpattern", true);
    if (gtk_selection_data_get_target(data) == a) {
        LayerFillData *dd = (LayerFillData*)gtk_selection_data_get_data(data);
        if (dd->d_from_layer) {
            if (dd->d_layernum > 0) {
                CDl *ld = CDldb()->layer(dd->d_layernum, DSP()->CurMode());
                if (ld && Lpal)
                    Lpal->update_user(ld, x, y);
            }
        }
        else {
            CDl *ld = Lpal->ldesc_at(x, y);
            XM()->FillLoadCallback(
                (LayerFillData*)gtk_selection_data_get_data(data), ld);
            if (GTKpkg::self()->IsTrueColor()) {
                // update the colors
                Lpal->update_layer(0);
                LT()->ShowLayerTable(ld);
                XM()->PopUpFillEditor(0, MODE_UPD);
            }
        }
    }
    else {
        if (gtk_selection_data_get_length(data) < 0) {
            gtk_drag_finish(context, false, false, time);
            return;
        }
        if (gtk_selection_data_get_format(data) != 16 ||
                gtk_selection_data_get_length(data) != 8) {
            fprintf(stderr, "Received invalid color data\n");
            gtk_drag_finish(context, false, false, time);
            return;
        }
        guint16 *vals = (guint16*)gtk_selection_data_get_data(data);

        CDl *ld = Lpal->ldesc_at(x, y);

        LT()->SetLayerColor(ld, vals[0] >> 8, vals[1] >> 8, vals[2] >> 8);
        if (GTKpkg::self()->IsTrueColor()) {
            // update the colors
            Lpal->update_layer(0);
            LT()->ShowLayerTable(ld);
            XM()->PopUpFillEditor(0, MODE_UPD);
        }
    }
    gtk_drag_finish(context, true, false, time);
    if (DSP()->CurMode() == Electrical || !LT()->NoPhysRedraw())
        DSP()->RedisplayAll();
}


// Static function.
void
sLpalette::lp_font_change_hdlr(GtkWidget*, void*, void*)
{
    if (Lpal)
        Lpal->init_size();
}


namespace {
    // Positioning function for pop-up menus.
    //
    void
    pos_func(GtkMenu*, int *x, int *y, gboolean *pushin, void *data)
    {
        *pushin = true;
        GtkWidget *btn = GTK_WIDGET(data);
        GTKdev::self()->Location(btn, x, y);
        GtkAllocation a;
        gtk_widget_get_allocation(btn, &a);
        (*x) -= a.width;
        (*y) += a.height;
    }
}


// Static function.
// Button-press handler to produce pop-up menu.
//
int
sLpalette::lp_popup_menu(GtkWidget *widget, GdkEvent *event, void *arg)
{
    gtk_menu_popup(GTK_MENU(arg), 0, 0, pos_func, widget, event->button.button,
        event->button.time);
    return (true);
}


// Static function.
void
sLpalette::lp_recall_proc(GtkWidget*, void *arg)
{
    if (!Lpal)
        return;
    int ix = (intptr_t)arg;

    int usz = LP_PALETTE_COLS * LP_PALETTE_ROWS;
    for (int i = 0; i < usz; i++)
        Lpal->lp_user[i] = 0;

    const char *s = Tech()->LayerPaletteReg(ix, DSP()->CurMode());
    int cnt = 0;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        CDl *ld = CDldb()->findLayer(tok, DSP()->CurMode());
        delete [] tok;
        if (!ld)
            continue;
        Lpal->lp_user[cnt] = ld;
        cnt++;
    }
    Lpal->lp_pmap_dirty = true;
    Lpal->refresh(0, 0, 0, 0);
}


// Static function.
void
sLpalette::lp_save_proc(GtkWidget*, void *arg)
{
    if (!Lpal)
        return;
    int ix = (intptr_t)arg;

    sLstr lstr;
    int usz = LP_PALETTE_COLS * LP_PALETTE_ROWS;
    for (int i = 0; i < usz; i++) {
        CDl *ld = Lpal->lp_user[i];
        if (!ld)
            break;
        if (i)
            lstr.add_c(' ');
        lstr.add(ld->name());
    }
    Tech()->SetLayerPaletteReg(ix, DSP()->CurMode(), lstr.string());
}

