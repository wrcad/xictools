
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
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinterf/gtkspinbtn.h"


//--------------------------------------------------------------------
// Pop-up to control cursor modes
//
// Help system keywords used:
//  xic:attr

namespace {
    namespace gtkattri {
        struct sAttr
        {
            sAttr(GRobject);
            ~sAttr();

            void update();

            GtkWidget *shell() { return (at_popup); }

        private:
            static void at_cancel_proc(GtkWidget*, void*);
            static void at_action(GtkWidget*, void*);
            static void at_curs_menu_proc(GtkWidget*, void*);
            static void at_menuproc(GtkWidget*, void*);
            static void at_val_changed(GtkWidget*, void*);
            static void at_ebt_proc(GtkWidget*, void*);

            GRobject at_caller;
            GtkWidget *at_popup;
            GtkWidget *at_cursor;
            GtkWidget *at_fullscr;
            GtkWidget *at_minst;
            GtkWidget *at_mcntr;
            GtkWidget *at_ebprop;
            GtkWidget *at_ebterms;
            GtkWidget *at_hdn;
            int at_ebthst;

            GTKspinBtn sb_tsize;
            GTKspinBtn sb_ttsize;
            GTKspinBtn sb_tmsize;
            GTKspinBtn sb_cellthr;
            GTKspinBtn sb_cxpct;
            GTKspinBtn sb_offset;
            GTKspinBtn sb_lheight;
            GTKspinBtn sb_llen;
            GTKspinBtn sb_llines;
        };

        sAttr *Attr;
    }

    const char *const hdn_menu[] = {
        "all labels",
        "all electrical labels",
        "electrical property labels",
        "no labels",
        0
    };

    const char *const cursvals[] = {
        "default",
        "cross",
        "left arrow",
        "right arrow",
        0
    };

    // Small cross-hair cursor
#define cursor_width 16
#define cursor_height 16
#define cursor_x_hot 8
#define cursor_y_hot 7

    char cursorCross_bits[] = {
        char(0x00), char(0x01), char(0x00), char(0x01),
        char(0x00), char(0x01), char(0x00), char(0x01),
        char(0x00), char(0x01), char(0x00), char(0x01),
        char(0x00), char(0x00), char(0x7e), char(0xfc),
        char(0x00), char(0x00), char(0x00), char(0x01),
        char(0x00), char(0x01), char(0x00), char(0x01),
        char(0x00), char(0x01), char(0x00), char(0x01),
        char(0x00), char(0x01), char(0x00), char(0x00) };
    char cursorCross_mask[] = {
        char(0x80), char(0x03), char(0x80), char(0x03),
        char(0x80), char(0x03), char(0x80), char(0x03),
        char(0x80), char(0x03), char(0x80), char(0x03),
        char(0x7e), char(0xfc), char(0x7e), char(0xfc),
        char(0x7e), char(0xfc), char(0x80), char(0x03),
        char(0x80), char(0x03), char(0x80), char(0x03),
        char(0x80), char(0x03), char(0x80), char(0x03),
        char(0x80), char(0x03), char(0x00), char(0x00) };

    GdkCursor *left_cursor;
    GdkCursor *right_cursor;
    GdkCursor *busy_cursor;
#if GTK_CHECK_VERSION(3,0,0)
    ndkCursor *cross_cursor;
#endif
}

using namespace gtkattri;


// Return the cursur type currently in force.
//
CursorType
cMain::GetCursor()
{
    if (!GTKmainwin::self())
        return (CursorDefault);
    return ((CursorType)GTKmainwin::self()->Gbag()->get_cursor_type());
}


// Change or update the cursor for wd, or all drawing windows if wd is
// null.  This should be called after a window mode change, since the
// cross cursor will need to be changed if the background color
// changes.
//
// Call with force true for new windows.
//
void
cMain::UpdateCursor(WindowDesc *wd, CursorType t, bool force)
{
    if (!force && t ==
            (CursorType)GTKmainwin::self()->Gbag()->get_cursor_type() &&
            t != CursorCross)
        return;
    if (!wd && GTKmainwin::self())
        GTKmainwin::self()->Gbag()->set_cursor_type(t);

    // The cross cursor makes things complicated, as it requires
    // background and foreground colors per window, i.e., each window
    // is given a separate instantiation of the cursor.

    if (t == CursorDefault) {
        // GTK default cursor.
        if (wd) {
            GTKsubwin *w = dynamic_cast<GTKsubwin*>(wd->Wbag());
#if GTK_CHECK_VERSION(3,0,0)
            if (w && w->GetDrawable()->get_window()) {
                if (cross_cursor) {
                    cross_cursor->revert_in_window(
                        w->GetDrawable()->get_window());
                }
                gdk_window_set_cursor(w->GetDrawable()->get_window(), 0);
            }
#else
            if (w && w->Window()) {
                if (!GDK_IS_PIXMAP(w->Window()))
                    gdk_window_set_cursor(w->Window(), 0);
            }
#endif
            return;
        }
        WDgen wgen(WDgen::MAIN, WDgen::ALL);
        while ((wd = wgen.next()) != 0) {
            GTKsubwin *w = dynamic_cast<GTKsubwin*>(wd->Wbag());
#if GTK_CHECK_VERSION(3,0,0)
            if (w && w->GetDrawable()->get_window()) {
                if (cross_cursor) {
                    cross_cursor->revert_in_window(
                        w->GetDrawable()->get_window());
                }
                gdk_window_set_cursor(w->GetDrawable()->get_window(), 0);
            }
#else
            if (w && w->Window()) {
                if (!GDK_IS_PIXMAP(w->Window()))
                    gdk_window_set_cursor(w->Window(), 0);
            }
#endif
        }
    }
    else if (t == CursorCross) {
        // Legacy cross cursor.
        if (wd) {
            GTKsubwin *w = dynamic_cast<GTKsubwin*>(wd->Wbag());
#if GTK_CHECK_VERSION(3,0,0)
            if (w && w->GetDrawable()->get_window()) {
                if (!cross_cursor) {
                    GdkColor foreg, backg;
                    backg.pixel = w->GetBackgPixel();
                    foreg.pixel = w->GetForegPixel();
                    gtk_QueryColor(&backg);
                    gtk_QueryColor(&foreg);
                    cross_cursor = new ndkCursor(w->GetDrawable()->get_window(),
                        cursorCross_bits, cursorCross_mask,
                        cursor_width, cursor_height, cursor_x_hot, cursor_y_hot,
                        &foreg, &backg);
                }
                cross_cursor->set_in_window(w->GetDrawable()->get_window());
            }
#else
            if (w && w->Window()) {
                if (!GDK_IS_PIXMAP(w->Window())) {
                    GdkWindow *win = w->Window();
                    GdkPixmap *data = gdk_bitmap_create_from_data(win,
                        cursorCross_bits, cursor_width, cursor_height);
                    GdkPixmap *mask = gdk_bitmap_create_from_data(win,
                        cursorCross_mask, cursor_width, cursor_height);

                    GdkColor foreg, backg;
                    backg.pixel = w->GetBackgPixel();
                    foreg.pixel = w->GetForegPixel();
                    gtk_QueryColor(&backg);
                    gtk_QueryColor(&foreg);
                    GdkCursor *cursor = gdk_cursor_new_from_pixmap(data, mask,
                        &foreg, &backg, cursor_x_hot, cursor_y_hot);
                    g_object_unref(data);
                    g_object_unref(mask);
                    gdk_window_set_cursor(win, cursor);
                    gdk_cursor_unref(cursor);
                }
            }
#endif
            return;
        }
        WDgen wgen(WDgen::MAIN, WDgen::ALL);
        while ((wd = wgen.next()) != 0) {
            GTKsubwin *w = dynamic_cast<GTKsubwin*>(wd->Wbag());
#if GTK_CHECK_VERSION(3,0,0)
            if (w && w->GetDrawable()->get_window()) {
                if (!cross_cursor) {
                    GdkColor foreg, backg;
                    backg.pixel = w->GetBackgPixel();
                    foreg.pixel = w->GetForegPixel();
                    gtk_QueryColor(&backg);
                    gtk_QueryColor(&foreg);
                    cross_cursor = new ndkCursor(w->GetDrawable()->get_window(),
                        cursorCross_bits, cursorCross_mask,
                        cursor_width, cursor_height, cursor_x_hot, cursor_y_hot,
                        &foreg, &backg);
                }
                cross_cursor->set_in_window(w->GetDrawable()->get_window());
            }
#else
            if (w && w->Window()) {
                if (!GDK_IS_PIXMAP(w->Window())) {
                    GdkWindow *win = w->Window();
                    GdkPixmap *data = gdk_bitmap_create_from_data(win,
                        cursorCross_bits, cursor_width, cursor_height);
                    GdkPixmap *mask = gdk_bitmap_create_from_data(win,
                        cursorCross_mask, cursor_width, cursor_height);

                    GdkColor foreg, backg;
                    backg.pixel = w->GetBackgPixel();
                    foreg.pixel = w->GetForegPixel();
                    gtk_QueryColor(&backg);
                    gtk_QueryColor(&foreg);
                    GdkCursor *cursor = gdk_cursor_new_from_pixmap(data, mask,
                        &foreg, &backg, cursor_x_hot, cursor_y_hot);
                    g_object_unref(data);
                    g_object_unref(mask);
                    gdk_window_set_cursor(win, cursor);
                    gdk_cursor_unref(cursor);
                }
            }
#endif
        }
    }
    else if (t == CursorLeftArrow) {
        // Stock left pointer cursor.
        if (!left_cursor)
            left_cursor = gdk_cursor_new(GDK_LEFT_PTR);
        if (wd) {
            GTKsubwin *w = dynamic_cast<GTKsubwin*>(wd->Wbag());
#if GTK_CHECK_VERSION(3,0,0)
            if (w && w->GetDrawable()->get_window()) {
                if (cross_cursor) {
                    cross_cursor->revert_in_window(
                        w->GetDrawable()->get_window());
                }
                gdk_window_set_cursor(w->GetDrawable()->get_window(),
                    left_cursor);
            }
#else
            if (w && w->Window()) {
                if (!GDK_IS_PIXMAP(w->Window()))
                    gdk_window_set_cursor(w->Window(), left_cursor);
            }
#endif
            return;
        }
        WDgen wgen(WDgen::MAIN, WDgen::ALL);
        while ((wd = wgen.next()) != 0) {
            GTKsubwin *w = dynamic_cast<GTKsubwin*>(wd->Wbag());
#if GTK_CHECK_VERSION(3,0,0)
            if (w && w->GetDrawable()->get_window()) {
                if (cross_cursor) {
                    cross_cursor->revert_in_window(
                        w->GetDrawable()->get_window());
                }
                gdk_window_set_cursor(w->GetDrawable()->get_window(),
                    left_cursor);
            }
#else
            if (w && w->Window()) {
                if (!GDK_IS_PIXMAP(w->Window()))
                    gdk_window_set_cursor(w->Window(), left_cursor);
            }
#endif
        }
    }
    else if (t == CursorRightArrow) {
        // Stock right pointer cursor.
        if (!right_cursor)
            right_cursor = gdk_cursor_new(GDK_RIGHT_PTR);
        if (wd) {
            GTKsubwin *w = dynamic_cast<GTKsubwin*>(wd->Wbag());
#if GTK_CHECK_VERSION(3,0,0)
            if (w && w->GetDrawable()->get_window()) {
                if (cross_cursor) {
                    cross_cursor->revert_in_window(
                        w->GetDrawable()->get_window());
                }
                gdk_window_set_cursor(w->GetDrawable()->get_window(),
                    right_cursor);
            }
#else
            if (w && w->Window()) {
                if (!GDK_IS_PIXMAP(w->Window()))
                    gdk_window_set_cursor(w->Window(), right_cursor);
            }
#endif
            return;
        }
        WDgen wgen(WDgen::MAIN, WDgen::ALL);
        while ((wd = wgen.next()) != 0) {
            GTKsubwin *w = dynamic_cast<GTKsubwin*>(wd->Wbag());
#if GTK_CHECK_VERSION(3,0,0)
            if (w && w->GetDrawable()->get_window()) {
                if (cross_cursor) {
                    cross_cursor->revert_in_window(
                        w->GetDrawable()->get_window());
                }
                gdk_window_set_cursor(w->GetDrawable()->get_window(),
                    right_cursor);
            }
#else
            if (w && w->Window()) {
                if (!GDK_IS_PIXMAP(w->Window()))
                    gdk_window_set_cursor(w->Window(), right_cursor);
            }
#endif
        }
    }
    else if (t == CursorBusy) {
        // Stock watch (busy) cursor.
        if (!busy_cursor)
            busy_cursor = gdk_cursor_new(GDK_WATCH);
        if (wd) {
            GTKsubwin *w = dynamic_cast<GTKsubwin*>(wd->Wbag());
#if GTK_CHECK_VERSION(3,0,0)
            if (w && w->GetDrawable()->get_window()) {
                if (cross_cursor) {
                    cross_cursor->revert_in_window(
                        w->GetDrawable()->get_window());
                }
                gdk_window_set_cursor(w->GetDrawable()->get_window(),
                    busy_cursor);
            }
#else
            if (w && w->Window()) {
                if (!GDK_IS_PIXMAP(w->Window()))
                    gdk_window_set_cursor(w->Window(), busy_cursor);
            }
#endif
            return;
        }
        WDgen wgen(WDgen::MAIN, WDgen::ALL);
        while ((wd = wgen.next()) != 0) {
            GTKsubwin *w = dynamic_cast<GTKsubwin*>(wd->Wbag());
#if GTK_CHECK_VERSION(3,0,0)
            if (w && w->GetDrawable()->get_window()) {
                if (cross_cursor) {
                    cross_cursor->revert_in_window(
                        w->GetDrawable()->get_window());
                }
                gdk_window_set_cursor(w->GetDrawable()->get_window(),
                    busy_cursor);
                // Force immediate display of busy cursor.
                GTKpkg::self()->CheckForInterrupt();
            }
#else
            if (w && w->Window()) {
                if (!GDK_IS_PIXMAP(w->Window())) {
                    gdk_window_set_cursor(w->Window(), busy_cursor);
                    // Force immediate display of busy cursor.
                    GTKpkg::self()->CheckForInterrupt();
                }
            }
#endif
        }
    }
}


void
cMain::PopUpAttributes(GRobject caller, ShowMode mode)
{
    if (!GTKdev::exists() || !GTKmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete Attr;
        return;
    }
    if (mode == MODE_UPD) {
        if (Attr)
            Attr->update();
        return;
    }
    if (Attr)
        return;

    new sAttr(caller);
    if (!Attr->shell()) {
        delete Attr;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Attr->shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GTKdev::self()->SetPopupLocation(GRloc(), Attr->shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(Attr->shell());
}


sAttr::sAttr(GRobject c)
{
    Attr = this;
    at_caller = c;
    at_minst = 0;
    at_mcntr = 0;
    at_ebprop = 0;
    at_ebterms = 0;
    at_hdn = 0;
    at_ebthst = 0;

    at_popup = gtk_NewPopup(0, "Window Attributes", at_cancel_proc, 0);
    if (!at_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(at_popup), false);

    GtkWidget *topform = gtk_table_new(1, 1, false);
    gtk_widget_show(topform);
    gtk_container_set_border_width(GTK_CONTAINER(topform), 2);
    gtk_container_add(GTK_CONTAINER(at_popup), topform);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new(
        "Set misc. window attributes");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(row), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(at_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(topform), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *nbook = gtk_notebook_new();
    gtk_widget_show(nbook);
    gtk_table_attach(GTK_TABLE(topform), nbook, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // General page
    //
    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    int rcnt = 0;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Cursor:");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    GtkWidget *entry = gtk_combo_box_text_new();
    gtk_widget_set_name(entry, "cursmenu");
    gtk_widget_show(entry);
    for (int i = 0; cursvals[i]; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry), cursvals[i]);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(entry), XM()->GetCursor());
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(at_curs_menu_proc), 0);
    at_cursor = entry;
    gtk_box_pack_start(GTK_BOX(row), entry, false, false, 0);

    button = gtk_check_button_new_with_label(
        "Use full-window cursor");
    gtk_widget_set_name(button, "fullscr");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(at_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    at_fullscr = button;

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Subcell visibility threshold (pixels)");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    GtkWidget *sb = sb_cellthr.init(DSP_DEF_CELL_THRESHOLD,
        DSP_MIN_CELL_THRESHOLD, DSP_MAX_CELL_THRESHOLD, 0);
    sb_cellthr.connect_changed(G_CALLBACK(at_val_changed), 0, "cellthr");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Push context display illumination percent");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    sb = sb_cxpct.init(DSP_DEF_CX_DARK_PCNT, DSP_MIN_CX_DARK_PCNT, 100.0, 0);
    sb_cxpct.connect_changed(G_CALLBACK(at_val_changed), 0, "cxpct");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Pixels between pop-ups and prompt line");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    sb = sb_offset.init(0, -16, 16, 0);
    sb_offset.connect_changed(G_CALLBACK(at_val_changed), 0, "offset");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    label = gtk_label_new("General");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), form, label);

    //
    // Selections page
    //
    form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    rcnt = 0;

    button = gtk_check_button_new_with_label(
        "Show origin of selected physical instances");
    gtk_widget_set_name(button, "mark");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(at_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;
    at_minst = button;

    button = gtk_check_button_new_with_label(
        "Show centroids of selected physical objects");
    gtk_widget_set_name(button, "centr");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(at_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;
    at_mcntr = button;

    label = gtk_label_new("Selections");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), form, label);

    //
    // Phys Props page
    //
    form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    rcnt = 0;

    button = gtk_check_button_new_with_label(
        "Erase behind physical properties text");
    gtk_widget_set_name(button, "ebprop");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(at_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;
    at_ebprop = button;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Physical property text size (pixels)");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    sb = sb_tsize.init(DSP_DEF_PTRM_TXTHT, DSP_MIN_PTRM_TXTHT,
        DSP_MAX_PTRM_TXTHT, 0);
    sb_tsize.connect_changed(G_CALLBACK(at_val_changed), 0, "tsize");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    label = gtk_label_new("Phys Props");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), form, label);

    //
    // Terminals page
    //
    form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    rcnt = 0;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Erase behind physical terminals");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    at_ebterms = gtk_combo_box_text_new();
    gtk_widget_set_name(at_ebterms, "EBTerms");
    gtk_widget_show(at_ebterms);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(at_ebterms),
        "Don't erase");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(at_ebterms),
        "Cell terminals only");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(at_ebterms),
        "All terminals");
    gtk_combo_box_set_active(GTK_COMBO_BOX(at_ebterms), at_ebthst);
    g_signal_connect(G_OBJECT(at_ebterms), "changed",
        G_CALLBACK(at_ebt_proc), 0);
    gtk_box_pack_end(GTK_BOX(row), at_ebterms, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Terminal text pixel size");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    sb = sb_ttsize.init(DSP()->TermTextSize(), DSP_MIN_PTRM_TXTHT,
        DSP_MAX_PTRM_TXTHT, 0);
    sb_ttsize.connect_changed(G_CALLBACK(at_val_changed), 0, "TTSize");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Terminal mark size");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    sb = sb_tmsize.init(DSP()->TermMarkSize(), DSP_MIN_PTRM_DELTA,
        DSP_MAX_PTRM_DELTA, 0);
    sb_tmsize.connect_changed(G_CALLBACK(at_val_changed), 0, "TMSize");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    label = gtk_label_new("Terminals");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), form, label);

    //
    // Labels page
    //
    form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    rcnt = 0;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Hidden label scope");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    entry = gtk_combo_box_text_new();
    gtk_widget_set_name(entry, "hidn");
    gtk_widget_show(entry);
    for (int i = 0; hdn_menu[i]; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry), hdn_menu[i]);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(entry), 0);
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(at_menuproc), 0);
    gtk_box_pack_end(GTK_BOX(row), entry, false, false, 0);
    at_hdn = entry;

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Default minimum label height");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    sb = sb_lheight.init(CD_DEF_TEXT_HEI, CD_MIN_TEXT_HEI,
        CD_MAX_TEXT_HEI, 2);
    sb_lheight.connect_changed(G_CALLBACK(at_val_changed), 0, "lheight");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Maximum displayed label length");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    sb = sb_llen.init(DSP_DEF_MAX_LABEL_LEN, DSP_MIN_MAX_LABEL_LEN,
        DSP_MAX_MAX_LABEL_LEN, 0);
    sb_llen.connect_changed(G_CALLBACK(at_val_changed), 0, "llen");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Label optional displayed line limit");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    sb = sb_llines.init(DSP_DEF_MAX_LABEL_LINES, DSP_MIN_MAX_LABEL_LINES,
        DSP_MAX_MAX_LABEL_LINES, 0);
    sb_llines.connect_changed(G_CALLBACK(at_val_changed), 0, "llines");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    label = gtk_label_new("Labels");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), form, label);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(topform), sep, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(at_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(topform), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(at_popup), button);

    update();
}


sAttr::~sAttr()
{
    Attr = 0;
    if (at_caller)
        GTKdev::Deselect(at_caller);
    if (at_popup)
        gtk_widget_destroy(at_popup);
}


void
sAttr::update()
{
    GTKdev::SetStatus(at_minst,
        CDvdb()->getVariable(VA_MarkInstanceOrigin));
    GTKdev::SetStatus(at_mcntr,
        CDvdb()->getVariable(VA_MarkObjectCentroid));
    GTKdev::SetStatus(at_ebprop,
        CDvdb()->getVariable(VA_EraseBehindProps));

    int d;
    const char *str = CDvdb()->getVariable(VA_PhysPropTextSize);
    if (str && sscanf(str, "%d", &d) == 1 && d >= DSP_MIN_PTRM_TXTHT &&
            d <= DSP_MAX_PTRM_TXTHT)
        ;
    else
        d = DSP_DEF_PTRM_TXTHT;
    sb_tsize.set_value(d);

    if (at_ebthst != (int)DSP()->EraseBehindTerms()) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(at_ebterms),
            DSP()->EraseBehindTerms());
        at_ebthst = DSP()->EraseBehindTerms();
    }

    str = CDvdb()->getVariable(VA_TermTextSize);
    if (str && sscanf(str, "%d", &d) == 1 && d >= DSP_MIN_PTRM_TXTHT &&
            d <= DSP_MAX_PTRM_TXTHT)
        ;
    else
        d = DSP_DEF_PTRM_TXTHT;
    sb_ttsize.set_value(d);

    str = CDvdb()->getVariable(VA_TermMarkSize);
    if (str && sscanf(str, "%d", &d) == 1 && d >= DSP_MIN_PTRM_DELTA &&
            d <= DSP_MAX_PTRM_DELTA)
        ;
    else
        d = DSP_DEF_PTRM_DELTA;
    sb_tmsize.set_value(d);

    str = CDvdb()->getVariable(VA_CellThreshold);
    if (str && sscanf(str, "%d", &d) == 1 && d >= DSP_MIN_CELL_THRESHOLD &&
            d <= DSP_MAX_CELL_THRESHOLD)
        ;
    else
        d = DSP_DEF_CELL_THRESHOLD;
    sb_cellthr.set_value(d);

    str = CDvdb()->getVariable(VA_ContextDarkPcnt);
    if (str && sscanf(str, "%d", &d) == 1 && d >= DSP_MIN_CX_DARK_PCNT &&
            d <= 100)
        ;
    else
        d = DSP_DEF_CX_DARK_PCNT;
    sb_cxpct.set_value(d);

    str = CDvdb()->getVariable(VA_LowerWinOffset);
    if (str && sscanf(str, "%d", &d) == 1 && d >= -16 && d <= 16)
        ;
    else
        d = 0;
    sb_offset.set_value(d);

    str = CDvdb()->getVariable(VA_LabelHiddenMode);
    d = str ? atoi(str) : 0;
    gtk_combo_box_set_active(GTK_COMBO_BOX(at_hdn), d);

    double dd;
    str = CDvdb()->getVariable(VA_LabelDefHeight);
    if (str && sscanf(str, "%lf", &dd) == 1 && dd >= CD_MIN_TEXT_HEI &&
            dd <= CD_MAX_TEXT_HEI)
        ;
    else
        dd = CD_DEF_TEXT_HEI;
    sb_lheight.set_value(dd);

    str = CDvdb()->getVariable(VA_LabelMaxLen);
    if (str && sscanf(str, "%d", &d) == 1 && d >= DSP_MIN_MAX_LABEL_LEN &&
            d <= DSP_MAX_MAX_LABEL_LEN)
        ;
    else
        d = DSP_DEF_MAX_LABEL_LEN;
    sb_llen.set_value(d);

    str = CDvdb()->getVariable(VA_LabelMaxLines);
    if (str && sscanf(str, "%d", &d) == 1 && d >= DSP_MIN_MAX_LABEL_LINES &&
            d <= DSP_MAX_MAX_LABEL_LINES)
        ;
    else
        d = DSP_DEF_MAX_LABEL_LINES;
    sb_llines.set_value(d);
}


// Static function.
void
sAttr::at_cancel_proc(GtkWidget*, void*)
{
    XM()->PopUpAttributes(0, MODE_OFF);
}


// Static function.
void
sAttr::at_action(GtkWidget *caller, void*)
{
    if (!Attr)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:attr"))
        return;
    }

    if (!strcmp(name, "fullscr")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_FullWinCursor, 0);
        else
            CDvdb()->clearVariable(VA_FullWinCursor);
        return;
    }
    if (!strcmp(name, "mark")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_MarkInstanceOrigin, "");
        else
            CDvdb()->clearVariable(VA_MarkInstanceOrigin);
        return;
    }
    if (!strcmp(name, "centr")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_MarkObjectCentroid, "");
        else
            CDvdb()->clearVariable(VA_MarkObjectCentroid);
        return;
    }
    if (!strcmp(name, "ebprop")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_EraseBehindProps, "");
        else
            CDvdb()->clearVariable(VA_EraseBehindProps);
        return;
    }
}


// Static function.
void
sAttr::at_curs_menu_proc(GtkWidget *caller, void*)
{
    int i = gtk_combo_box_get_active(GTK_COMBO_BOX(caller));
    if (i < 0)
        return;
    CursorType ct = (CursorType)i;
    XM()->UpdateCursor(0, ct);
}


// Static function.
void
sAttr::at_menuproc(GtkWidget *caller, void*)
{
    if (!Attr)
        return;
    int i = gtk_combo_box_get_active(GTK_COMBO_BOX(caller));
    if (i == 0)
        CDvdb()->clearVariable(VA_LabelHiddenMode);
    else if (i > 0) {
        char bf[8];
        bf[0] = '0' + i;
        bf[1] = 0;
        CDvdb()->setVariable(VA_LabelHiddenMode, bf);
    }
}


// Static function.
void
sAttr::at_val_changed(GtkWidget *caller, void*)
{
    if (!Attr)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "tsize")) {
        const char *s = Attr->sb_tsize.get_string();
        char *endp;
        int d = (int)strtod(s, &endp);
        if (endp > s && d >= DSP_MIN_PTRM_TXTHT && d <= DSP_MAX_PTRM_TXTHT) {
            if (d == DSP_DEF_PTRM_TXTHT)
                CDvdb()->clearVariable(VA_PhysPropTextSize);
            else {
                char buf[32];
                sprintf(buf, "%d", d);
                CDvdb()->setVariable(VA_PhysPropTextSize, buf);
            }
        }
    }
    else if (!strcmp(name, "TTSize")) {
        const char *s = Attr->sb_ttsize.get_string();
        int n;
        if (sscanf(s, "%d", &n) == 1 && n >= DSP_MIN_PTRM_TXTHT &&
                n <= DSP_MAX_PTRM_TXTHT) {
            if (n == DSP_DEF_PTRM_TXTHT)
                CDvdb()->clearVariable(VA_TermTextSize);
            else
                CDvdb()->setVariable(VA_TermTextSize, s);
        }
    }
    else if (!strcmp(name, "TMSize")) {
        const char *s = Attr->sb_tmsize.get_string();
        int n;
        if (sscanf(s, "%d", &n) == 1 && n >= DSP_MIN_PTRM_DELTA &&
                n <= DSP_MAX_PTRM_DELTA) {
            if (n == DSP_DEF_PTRM_DELTA)
                CDvdb()->clearVariable(VA_TermMarkSize);
            else
                CDvdb()->setVariable(VA_TermMarkSize, s);
        }
    }
    else if (!strcmp(name, "cellthr")) {
        const char *s = Attr->sb_cellthr.get_string();
        char *endp;
        int d = (int)strtod(s, &endp);
        if (endp > s && d >= DSP_MIN_CELL_THRESHOLD &&
                d <= DSP_MAX_CELL_THRESHOLD) {
            if (d == DSP_DEF_CELL_THRESHOLD)
                CDvdb()->clearVariable(VA_CellThreshold);
            else {
                char buf[32];
                sprintf(buf, "%d", d);
                CDvdb()->setVariable(VA_CellThreshold, buf);
            }
        }
    }
    else if (!strcmp(name, "cxpct")) {
        const char *s = Attr->sb_cxpct.get_string();
        char *endp;
        int d = (int)strtod(s, &endp);
        if (endp > s && d >= DSP_MIN_CX_DARK_PCNT && d <= 100) {
            if (d == DSP_DEF_CX_DARK_PCNT)
                CDvdb()->clearVariable(VA_ContextDarkPcnt);
            else {
                char buf[32];
                sprintf(buf, "%d", d);
                CDvdb()->setVariable(VA_ContextDarkPcnt, buf);
            }
        }
    }
    else if (!strcmp(name, "offset")) {
        const char *s = Attr->sb_offset.get_string();
        char *endp;
        int d = (int)strtod(s, &endp);
        if (endp > s && d >= -16 && d <= 16) {
            if (d == 0)
                CDvdb()->clearVariable(VA_LowerWinOffset);
            else {
                char buf[32];
                sprintf(buf, "%d", d);
                CDvdb()->setVariable(VA_LowerWinOffset, buf);
            }
        }
    }
    else if (!strcmp(name, "lheight")) {
        const char *s = Attr->sb_lheight.get_string();
        char *endp;
        double d = strtod(s, &endp);
        if (endp > s && d >= CD_MIN_TEXT_HEI && d <= CD_MAX_TEXT_HEI) {
            if (d == CD_DEF_TEXT_HEI)
                CDvdb()->clearVariable(VA_LabelDefHeight);
            else {
                char buf[32];
                sprintf(buf, "%.2f", d);
                CDvdb()->setVariable(VA_LabelDefHeight, buf);
            }
        }
    }
    else if (!strcmp(name, "llen")) {
        const char *s = Attr->sb_llen.get_string();
        char *endp;
        int d = (int)strtod(s, &endp);
        if (endp > s && d >= DSP_MIN_MAX_LABEL_LEN &&
                d <= DSP_MAX_MAX_LABEL_LEN) {
            if (d == DSP_DEF_MAX_LABEL_LEN)
                CDvdb()->clearVariable(VA_LabelMaxLen);
            else {
                char buf[32];
                sprintf(buf, "%d", d);
                CDvdb()->setVariable(VA_LabelMaxLen, buf);
            }
        }
    }
    else if (!strcmp(name, "llines")) {
        const char *s = Attr->sb_llines.get_string();
        char *endp;
        int d = (int)strtod(s, &endp);
        if (endp > s && d >= DSP_MIN_MAX_LABEL_LINES) {
            if (d == DSP_DEF_MAX_LABEL_LINES)
                CDvdb()->clearVariable(VA_LabelMaxLines);
            else {
                if (d > DSP_MAX_MAX_LABEL_LINES)
                    d = DSP_MAX_MAX_LABEL_LINES;
                char buf[32];
                sprintf(buf, "%d", d);
                CDvdb()->setVariable(VA_LabelMaxLines, buf);
            }
        }
    }
}


// Static function.
void
sAttr::at_ebt_proc(GtkWidget *caller, void*)
{
    int i = gtk_combo_box_get_active(GTK_COMBO_BOX(caller));
    if (i < 0)
        return;
    if (i == 0) {
        CDvdb()->clearVariable(VA_EraseBehindTerms);
        Attr->at_ebthst = 0;
    }
    else if (i == 1) {
        CDvdb()->setVariable(VA_EraseBehindTerms, "");
        Attr->at_ebthst = 1;
    }
    else if (i == 2) {
        CDvdb()->setVariable(VA_EraseBehindTerms, "all");
        Attr->at_ebthst = 2;
    }
}

