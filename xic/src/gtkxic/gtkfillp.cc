
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
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "layertab.h"
#include "menu.h"
#include "errorlog.h"
#include "keymap.h"
#include "tech.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkspinbtn.h"
#include <gdk/gdkkeysyms.h>


//-----------------------------------------------------------------------------
// Fill pattern editing widget.
//
// Help system keywords used:
//  fillpanel

// This is used for the layer table also.
// XPM
const char *fillpattern_xpm[] = {
    // width height ncolors chars_per_pixel
    "24 24 3 1",
    // colors
    " 	c None",
    ".	c #ffffff",
    "+  c #000000",
    // pixels
    "++++++++++++++++++++++++",
    "+......................+",
    "+..+     ..+     ..+  .+",
    "+...+     ..+     ..+ .+",
    "+. ..+     ..+     ..+.+",
    "+.  ..+     ..+     ...+",
    "+.   ..+     ..+     ..+",
    "+.    ..+     ..+     .+",
    "+.     ..+     ..+    .+",
    "+.+     ..+     ..+   .+",
    "+..+     ..+     ..+  .+",
    "+...+     ..+     ..+ .+",
    "+. ..+     ..+     ..+.+",
    "+.  ..+     ..+     ...+",
    "+.   ..+     ..+     ..+",
    "+.    ..+     ..+     .+",
    "+.     ..+     ..+    .+",
    "+.+     ..+     ..+   .+",
    "+..+     ..+     ..+  .+",
    "+...+     ..+     ..+ .+",
    "+. ..+     ..+     ..+.+",
    "+.  ..+     ..+     ...+",
    "+......................+",
    "++++++++++++++++++++++++"};

namespace {
    GtkTargetEntry target_table[] = {
        { (char*)"fillpattern", 0, 0 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

    namespace gtkfillp {
        // Pixel operations
        enum FPSETtype { FPSEToff, FPSETon, FPSETflip };

        struct sFpe : public GTKbag, public GTKdraw
        {
            sFpe(GRobject);
            ~sFpe();

            void update();
            void drag_load(LayerFillData*, CDl*);

        private:
            void redraw_edit();
            void redraw_sample();
            void redraw_store(int);
            void show_pixel(int, int);
            void set_fp(unsigned char*, int, int);
            bool getij(int*, int*);
            void set_pixel(int, int, FPSETtype);
            FPSETtype get_pixel(int, int);
            void line(int, int, int, int, FPSETtype);
            void box(int, int, int, int, FPSETtype);
            void def_to_sample(LayerFillData*);
            void sample_to_def(LayerFillData*, int);
            void layer_to_def_or_sample(LayerFillData*, int);
            void pattern_to_layer(LayerFillData*, CDl*);

            static void fp_connect_sigs(GtkWidget*, bool, bool);
            static void fp_source_drag_data_get(GtkWidget*, GdkDragContext*,
                GtkSelectionData*, guint, guint, void*);
            static void fp_source_drag_begin(GtkWidget*, GdkDragContext*,
                gpointer);
            static void fp_source_drag_end(GtkWidget*, GdkDragContext*,
                gpointer);
            static void fp_target_drag_data_received(GtkWidget*,
                GdkDragContext*, gint, gint, GtkSelectionData*, guint, guint);
            static gboolean fp_target_drag_motion(GtkWidget*, GdkDragContext*,
                gint, gint, guint);
            static void fp_target_drag_leave(GtkWidget*, GdkDragContext*,
                guint);
            static int fp_config_hdlr(GtkWidget*, GdkEvent*, void*);
            static int fp_redraw_edit_hdlr(GtkWidget*, GdkEvent*, void*);
            static int fp_redraw_sample_hdlr(GtkWidget*, GdkEvent*, void*);
            static int fp_redraw_store_hdlr(GtkWidget*, GdkEvent*, void*);
            static int fp_button_press_hdlr(GtkWidget*, GdkEvent*, void*);
            static int fp_button_rel_hdlr(GtkWidget*, GdkEvent*, void*);
            static int fp_key_press_hdlr(GtkWidget*, GdkEvent*, void*);
            static int fp_motion_hdlr(GtkWidget*, GdkEvent*, void*);
            static int fp_enter_hdlr(GtkWidget*, GdkEvent*, void*);
            static void fp_cancel_proc(GtkWidget*, void*);
            static void fp_mode_proc(GtkWidget*, void*);
            static void fp_outline_proc(GtkWidget*, void*);
            static void fp_btn_proc(GtkWidget*, void*);
            static void fp_nxy_proc(GtkWidget*, void*);
            static void fp_rot90_proc(GtkWidget*, void*);
            static void fp_refl_proc(GtkWidget*, void*);
            static void fp_bank_proc(GtkWidget*, void*);
            static void drawghost(int, int, int, int, bool = false);

            GRobject fp_caller;
            GdkPixmap *fp_pixmap;
            GtkWidget *fp_pm_widget;
            GtkWidget *fp_outl;
            GtkWidget *fp_fat;
            GtkWidget *fp_cut;
            GtkWidget *fp_editor;
            GtkWidget *fp_sample;
            GtkWidget *fp_editframe;
            GtkWidget *fp_editctrl;
            GtkWidget *fp_stoframe;
            GtkWidget *fp_stoctrl;
            GtkWidget *fp_stores[18];

            GRfillType *fp_fp;
            int fp_pattern_bank;
            int fp_width, fp_height;
            long fp_foreg, fp_pixbg;
            unsigned char fp_array[128];  // 32x32 max
            int fp_nx, fp_ny;
            int fp_margin;
            int fp_def_box_w, fp_def_box_h;
            int fp_pat_box_h;
            int fp_edt_box_dim;
            int fp_spa;
            int fp_epsz;
            int fp_ii, fp_jj;
            int fp_drag_btn, fp_drag_x, fp_drag_y;
            int fp_pm_w, fp_pm_h;
            int fp_downbtn;
            bool fp_dragging;
            bool fp_editing;
            GTKspinBtn sb_nx;
            GTKspinBtn sb_ny;
            GTKspinBtn sb_defpats;
        };

        sFpe *Fpe;
    }
}

using namespace gtkfillp;


// Menu callback for fill editor popup.
//
void
cMain::PopUpFillEditor(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Fpe;
        return;
    }
    if (mode == MODE_UPD) {
        if (Fpe)
            Fpe->update();
        return;
    }
    if (Fpe)
        return;

    if (!XM()->CheckCurLayer()) {
        GRX->Deselect(caller);
        return;
    }

    new sFpe(caller);
    if (!Fpe->Shell()) {
        delete Fpe;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Fpe->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_LR), Fpe->Shell(), mainBag()->Viewport());
    gtk_widget_show(Fpe->Shell());
}


// Callback used for drag/drop to layer table.
//
void
cMain::FillLoadCallback(LayerFillData *dd, CDl *ld)
{
    if (Fpe)
        Fpe->drag_load(dd, ld);
}


sFpe::sFpe(GRobject c)
{
    Fpe = this;
    fp_caller = c;
    fp_pixmap = 0;
    fp_pm_widget = 0;
    fp_outl = 0;
    fp_fat = 0;
    fp_cut = 0;
    fp_editor = 0;
    fp_sample = 0;
    fp_editframe = 0;
    fp_editctrl = 0;
    fp_stoframe = 0;
    fp_stoctrl = 0;
    memset(fp_stores, 0, sizeof(fp_stores));

    fp_fp = 0;
    fp_pattern_bank = 0;
    fp_width = fp_height = 0;
    fp_foreg = fp_pixbg = 0;
    memset(fp_array, 0, sizeof(fp_array));
    fp_nx = fp_ny = 0;
    fp_margin = 0;
    fp_def_box_w = fp_def_box_h = 0;
    fp_pat_box_h = 0;
    fp_edt_box_dim = 0;
    fp_spa = 0;
    fp_epsz = 0;
    fp_ii = fp_jj = 0;
    fp_drag_btn = 0;
    fp_drag_x = fp_drag_y = 0;
    fp_pm_w = fp_pm_h = 0;
    fp_downbtn = 0;
    fp_dragging = false;
    fp_editing = false;

    fp_nx = 8;
    fp_ny = 8;
    wb_shell = gtk_NewPopup(0, "Fill Pattern Editor", fp_cancel_proc, 0);
    if (!wb_shell)
        return;
    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);

    g_signal_connect(G_OBJECT(wb_shell), "configure-event",
        G_CALLBACK(fp_config_hdlr), 0);

    fp_width = 466;
//    fp_height = 300;
    fp_height = 264;
    fp_spa = fp_height/36;
    fp_margin = 2*fp_spa;
    fp_def_box_h = 11*fp_spa - 2;
    fp_def_box_w = 5*fp_spa + 1;
    fp_edt_box_dim = 34*fp_spa;
    fp_pat_box_h = 12*fp_spa;
    fp_foreg = dsp_prm(LT()->CurLayer())->pixel();
    fp_pixbg = DSP()->Color(BackgroundColor);

    GtkWidget *form = gtk_table_new(1, 3, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);

    GtkWidget *row = gtk_hbox_new(false, 0);
    gtk_widget_show(row);

    GtkWidget *vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);

    //
    // Pixel editor
    //
    GtkWidget *frame = gtk_frame_new("Pixel Editor");
    gtk_widget_show(frame);
    fp_editframe = frame;

    GtkWidget *darea = gtk_drawing_area_new();
    gtk_widget_set_double_buffered(darea, false);
    gtk_widget_show(darea);
    fp_editor = darea;
    gtk_widget_set_size_request(darea, fp_edt_box_dim, fp_edt_box_dim);
    gtk_widget_add_events(darea, GDK_ENTER_NOTIFY_MASK);
    gtk_container_add(GTK_CONTAINER(frame), darea);
    fp_connect_sigs(darea, false, true);
    g_signal_connect(G_OBJECT(darea), "enter-notify-event",
        G_CALLBACK(fp_enter_hdlr), 0);
    g_signal_connect(G_OBJECT(darea), "expose-event",
        G_CALLBACK(fp_redraw_edit_hdlr), 0);

    //
    // Pixel editor controls
    //
    GtkWidget *ecvbox = gtk_vbox_new(false, 0);
    gtk_widget_show(ecvbox);
    fp_editctrl = ecvbox;

    GtkWidget *echbox = gtk_hbox_new(false, 0);
    gtk_widget_show(echbox);

    frame = gtk_frame_new("NX x NY");
    gtk_widget_show(frame);

    GtkWidget *sb = sb_nx.init(8.0, 2.0, 32.0, 0);
    sb_nx.set_wrap(false);
    sb_nx.set_editable(true);
    sb_nx.connect_changed(G_CALLBACK(fp_nxy_proc), (void*)(long)1, "nx");
    gtk_widget_set_size_request(sb, 50, -1);
    gtk_box_pack_start(GTK_BOX(echbox), sb, false, false, 0);

    sb = sb_ny.init(8.0, 2.0, 32.0, 0);
    sb_ny.set_wrap(false);
    sb_ny.set_editable(true);
    sb_ny.connect_changed(G_CALLBACK(fp_nxy_proc), (void*)(long)2, "ny");
    gtk_widget_set_size_request(sb, 50, -1);
    gtk_box_pack_start(GTK_BOX(echbox), sb, false, false, 0);
    gtk_container_add(GTK_CONTAINER(frame), echbox);
    gtk_box_pack_start(GTK_BOX(ecvbox), frame, false, false, 0);

    GtkWidget *hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    GtkWidget *button = gtk_button_new_with_label("Rot90");
    gtk_widget_set_name(button, "Rot90");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fp_rot90_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    button = gtk_button_new_with_label("X");
    gtk_widget_set_name(button, "MX");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fp_refl_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    button = gtk_button_new_with_label("Y");
    gtk_widget_set_name(button, "MY");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fp_refl_proc), (void*)1L);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
    gtk_box_pack_start(GTK_BOX(ecvbox), hbox, false, false, 0);

    GtkWidget *hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);
    gtk_box_pack_start(GTK_BOX(ecvbox), hsep, true, true, 0);

    button = gtk_button_new_with_label("Stores");
    gtk_widget_set_name(button, "ShowStores");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fp_mode_proc), 0);
    gtk_box_pack_start(GTK_BOX(ecvbox), button, false, false, 0);

    hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);
    gtk_box_pack_start(GTK_BOX(ecvbox), hsep, true, true, 0);

    //
    // Stores display controls
    //
    GtkWidget *scvbox = gtk_vbox_new(false, 0);
    gtk_widget_show(scvbox);
    fp_stoctrl = scvbox;

    frame = gtk_frame_new("Page");
    gtk_widget_show(frame);

    sb = sb_defpats.init(1.0, 1.0, 4.0, 0);
    sb_defpats.set_wrap(true);
    sb_defpats.set_editable(false);
    sb_defpats.connect_changed(G_CALLBACK(fp_bank_proc), 0, "Defpats");
    gtk_widget_set_size_request(sb, 50, -1);
    gtk_container_add(GTK_CONTAINER(frame), sb);
    gtk_box_pack_start(GTK_BOX(scvbox), frame, false, false, 0);

    hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);
    gtk_box_pack_start(GTK_BOX(scvbox), hsep, true, true, 0);

    button = gtk_button_new_with_label("Dump Defs");
    gtk_widget_set_name(button, "Dumpdefs");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fp_btn_proc), 0);
    gtk_box_pack_start(GTK_BOX(scvbox), button, false, false, 0);

    hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);
    gtk_box_pack_start(GTK_BOX(scvbox), hsep, true, true, 0);

    button = gtk_button_new_with_label("Pixel Editor");
    gtk_widget_set_name(button, "PexEd");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fp_mode_proc), (void*)(long)1);
    gtk_box_pack_start(GTK_BOX(scvbox), button, false, false, 0);

    hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);
    gtk_box_pack_start(GTK_BOX(scvbox), hsep, true, true, 0);

    //
    // Sample area
    //
    frame = gtk_frame_new("Sample");
    gtk_widget_show(frame);

    darea = gtk_drawing_area_new();
    gtk_widget_set_double_buffered(darea, false);
    gtk_widget_show(darea);
    fp_sample = darea;
    gtk_widget_set_size_request(darea, fp_def_box_h, fp_pat_box_h);
    gtk_container_add(GTK_CONTAINER(frame), darea);
    fp_connect_sigs(darea, true, true);
    gtk_box_pack_start(GTK_BOX(vbox), fp_editctrl, true, true, 0);
    gtk_box_pack_start(GTK_BOX(vbox), fp_stoctrl, true, true, 0);
    gtk_box_pack_start(GTK_BOX(vbox), frame, true, true, 0);
    gtk_box_pack_start(GTK_BOX(row), vbox, true, true, 0);
    g_signal_connect(G_OBJECT(darea), "expose-event",
        G_CALLBACK(fp_redraw_sample_hdlr), 0);

    //
    // Patterns
    //
    frame = gtk_frame_new("Default Patterns - Pattern Storage");
    gtk_widget_show(frame);
    fp_stoframe = frame;

    vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);
    for (int i = 0; i < 3; i++) {
        hbox = gtk_hbox_new(false, 0);
        gtk_widget_show(hbox);
        for (int j = 0; j < 6; j++) {
            GtkWidget *iframe = gtk_frame_new(0);
            gtk_widget_show(iframe);
            darea = gtk_drawing_area_new();
            gtk_widget_set_double_buffered(darea, false);
            gtk_widget_show(darea);
            fp_stores[i + j*3] = darea;
            gtk_widget_set_size_request(darea, fp_def_box_w, fp_def_box_h);
            gtk_container_add(GTK_CONTAINER(iframe), darea);
            if (j == 0 && i <= 1)
                fp_connect_sigs(darea, true, false);
            else
                fp_connect_sigs(darea, true, true);
            gtk_box_pack_start(GTK_BOX(hbox), iframe, true, true, 0);
            g_signal_connect(G_OBJECT(darea), "expose-event",
                G_CALLBACK(fp_redraw_store_hdlr), (void*)(long)(i + j*3));
        }
        gtk_box_pack_start(GTK_BOX(vbox), hbox, true, true, 0);
    }
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(row), fp_editframe, true, true, 0);
    gtk_box_pack_start(GTK_BOX(row), fp_stoframe, true, true, 0);

    int rowcnt = 0;
    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Button line
    //
    row = gtk_hbox_new(false, 0);
    gtk_widget_show(row);

    button = gtk_button_new_with_label("Load");
    gtk_widget_set_name(button, "Load");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fp_btn_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(button, "Apply");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fp_btn_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fp_btn_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_toggle_button_new_with_label("Outline");
    gtk_widget_set_name(button, "Outline");
    gtk_widget_show(button);
    fp_outl = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fp_outline_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_toggle_button_new_with_label("Fat");
    gtk_widget_set_name(button, "Fat");
    gtk_widget_show(button);
    fp_fat = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fp_outline_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_toggle_button_new_with_label("Cut");
    gtk_widget_set_name(button, "Cut");
    gtk_widget_show(button);
    fp_cut = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fp_outline_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fp_cancel_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    DspLayerParams *lp = dsp_prm(LT()->CurLayer());
    bool solid = LT()->CurLayer()->isFilled() && !lp->fill()->hasMap();
    if (solid) {
        fp_nx = 8;
        fp_ny = 8;
        memset(fp_array, 0xff, 8);
    }
    else {
        if (!LT()->CurLayer()->isFilled()) {
            fp_nx = 8;
            fp_ny = 8;
            memset(fp_array, 0, 8);
        }
        else {
            fp_nx = lp->fill()->nX();
            fp_ny = lp->fill()->nY();
            unsigned char *map = lp->fill()->newBitmap();
            memcpy(fp_array, map, fp_ny*((fp_nx + 7)/8));
            delete [] map;
        }
        if (LT()->CurLayer()->isOutlined()) {
            GRX->SetStatus(fp_outl, true);
            if (LT()->CurLayer()->isOutlinedFat())
                GRX->SetStatus(fp_fat, true);
        }
        else
            gtk_widget_set_sensitive(fp_fat, false);
        GRX->SetStatus(fp_cut, LT()->CurLayer()->isCut());
    }
    fp_mode_proc(0, 0);
}


sFpe::~sFpe()
{
    Fpe = 0;
    GRX->Deselect(fp_caller);
    if (fp_fp) {
        SetFillpattern(0);          // Remove pointer to saved pixmap.
        fp_fp->newMap(0, 0, 0);     // Clear pixel map.
        DefineFillpattern(fp_fp);   // Destroy pixmap.
        delete fp_fp;
    }
    if (wb_shell) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(wb_shell),
            (gpointer)fp_cancel_proc, wb_shell);
    }
}


void
sFpe::update()
{
    if (!LT()->CurLayer())
        return;
    fp_foreg = dsp_prm(LT()->CurLayer())->pixel();
    fp_pixbg = DSP()->Color(BackgroundColor);
    for (int i = 0; i < 18; i++)
        redraw_store(i);
    redraw_edit();
    redraw_sample();
}


void
sFpe::drag_load(LayerFillData *dd, CDl *ld)
{
    if (dd->d_from_layer)
        return;
    pattern_to_layer(dd, ld);
}


void
sFpe::redraw_edit()
{
    if (!fp_editing)
        return;
    int wid = gdk_window_get_width(gtk_widget_get_window(fp_editor));
    int hei = gdk_window_get_height(gtk_widget_get_window(fp_editor));
#ifdef NEW_DRW
    GetDrawable()->set_pixmap(gtk_widget_get_window(fp_editor));
#else
    fp_pm_widget = 0;

    if (!fp_pixmap || wid > fp_pm_w || hei > fp_pm_h) {
        if (fp_pixmap)
            gdk_pixmap_unref(fp_pixmap);
        fp_pixmap = gdk_pixmap_new(gtk_widget_get_window(fp_editor), wid, hei,
            gdk_visual_get_depth(GRX->Visual()));
        if (fp_pixmap) {
            fp_pm_w = wid;
            fp_pm_h = hei;
        }
    }

    if (fp_pixmap)
        gd_window = fp_pixmap;
    else
        gd_window = gtk_widget_get_window(fp_editor);
#endif

    int mind = mmMin(wid, hei);
    fp_spa = 2;
    fp_epsz = (mind - 2*fp_spa)/mmMax(fp_nx, fp_ny);

    SetFillpattern(0);
    GtkStyle *style = gtk_widget_get_style(Shell());
    SetColor(style->bg[0].pixel);
    Box(0, 0, wid, hei);
    SetColor(fp_pixbg);
    Box(fp_spa - 1, fp_spa - 1,
        fp_spa + fp_nx*fp_epsz + 2, fp_spa + fp_ny*fp_epsz + 2);

    int bpl = (fp_nx + 7)/8;
    unsigned char *a = fp_array;
    for (int i = 0; i < fp_ny; i++) {
        unsigned int d = *a++;
        for (int j = 1; j < bpl; j++)
            d |= *a++ << j*8;

        unsigned int mask = 1;
        for (int j = 0; j < fp_nx; j++) {
            bool lit = mask & d;
            SetColor(lit ? fp_foreg : fp_pixbg);
            show_pixel(i, j);
            mask <<= 1;
        }
    }
#ifdef NEW_DRW
    GetDrawable()->copy_pixmap_to_window(GC(), 0, 0, wid, hei);
#else
    if (fp_pixmap) {
        gdk_window_copy_area(gtk_widget_get_window(fp_editor), GC(), 0, 0,
            gd_window, 0, 0, wid, hei);
        fp_pm_widget = fp_editor;
    }
#endif
}


void
sFpe::redraw_sample()
{
    int wid = gdk_window_get_width(gtk_widget_get_window(fp_sample));
    int hei = gdk_window_get_height(gtk_widget_get_window(fp_sample));
#ifdef NEW_DRW
    GetDrawable()->set_pixmap(gtk_widget_get_window(fp_sample));
#else
    fp_pm_widget = 0;

    if (!fp_pixmap || wid > fp_pm_w || hei > fp_pm_h) {
        if (fp_pixmap)
            gdk_pixmap_unref(fp_pixmap);
        fp_pixmap = gdk_pixmap_new(gtk_widget_get_window(fp_sample), wid, hei,
            gdk_visual_get_depth(GRX->Visual()));
        if (fp_pixmap) {
            fp_pm_w = wid;
            fp_pm_h = hei;
        }
    }
    if (fp_pixmap)
        gd_window = fp_pixmap;
    else {
        gd_window = gtk_widget_get_window(fp_sample);
    }
#endif

    SetColor(fp_pixbg);
    SetFillpattern(0);
    Box(0, 0, wid, hei);

    SetColor(fp_foreg);
    set_fp(fp_array, fp_nx, fp_ny);
    Box(0, 0, wid, hei);
    SetFillpattern(0);

    bool nonff = false;
    int sz = fp_ny*((fp_nx + 7)/8);
    for (int i = 0; i < sz; i++) {
        if (fp_array[i] != 0xff) {
            nonff = true;
            break;
        }
    }
    if (!nonff) {
        GRX->SetStatus(fp_cut, false);
        GRX->SetStatus(fp_outl, false);
        GRX->SetStatus(fp_fat, false);
        gtk_widget_set_sensitive(fp_cut, false);
        gtk_widget_set_sensitive(fp_outl, false);
        gtk_widget_set_sensitive(fp_fat, false);
    }
    else {
        gtk_widget_set_sensitive(fp_cut, true);
        gtk_widget_set_sensitive(fp_outl, true);
        gtk_widget_set_sensitive(fp_fat, GRX->GetStatus(fp_outl));
    }
    if (GRX->GetStatus(fp_outl)) {
        int x1 = 0;
        int y1 = 0;
        int x2 = wid - 1;
        int y2 = hei - 1;
        Line(x1, y1, x2, y1);
        Line(x2, y1, x2, y2);
        Line(x2, y2, x1, y2);
        Line(x1, y2, x1, y1);
        if (GRX->GetStatus(fp_fat)) {
            x1++;
            y1++;
            x2--;
            y2--;
            Line(x1, y1, x2, y1);
            Line(x2, y1, x2, y2);
            Line(x2, y2, x1, y2);
            Line(x1, y2, x1, y1);
            x1++;
            y1++;
            x2--;
            y2--;
            Line(x1, y1, x2, y1);
            Line(x2, y1, x2, y2);
            Line(x2, y2, x1, y2);
            Line(x1, y2, x1, y1);
        }
    }
    if (GRX->GetStatus(fp_cut)) {
        int x1 = 0;
        int y1 = 0;
        int x2 = wid - 1;
        int y2 = hei - 1;
        Line(x1, y1, x2, y2);
        Line(x1, y2, x2, y1);
    }
#ifdef NEW_DRW
    GetDrawable()->copy_pixmap_to_window(GC(), 0, 0, wid, hei);
#else
    if (fp_pixmap) {
        gdk_window_copy_area(gtk_widget_get_window(fp_sample), GC(), 0, 0,
            gd_window, 0, 0, wid, hei);
        fp_pm_widget = fp_sample;
    }
#endif
}


void
sFpe::redraw_store(int i)
{
    if (fp_editing)
        return;

    int wid = gdk_window_get_width(gtk_widget_get_window(fp_stores[i]));
    int hei = gdk_window_get_height(gtk_widget_get_window(fp_stores[i]));
#ifdef NEW_DRW
    GetDrawable()->set_pixmap(gtk_widget_get_window(fp_stores[i]));
#else
    fp_pm_widget = 0;

    if (!fp_pixmap || wid > fp_pm_w || hei > fp_pm_h) {
        if (fp_pixmap)
            gdk_pixmap_unref(fp_pixmap);
        fp_pixmap = gdk_pixmap_new(
            gtk_widget_get_window(fp_stores[i]), wid, hei,
            gdk_visual_get_depth(GRX->Visual()));
        if (fp_pixmap) {
            fp_pm_w = wid;
            fp_pm_h = hei;
        }
    }
    if (fp_pixmap)
        gd_window = fp_pixmap;
    else
        gd_window = gtk_widget_get_window(fp_stores[i]);
#endif

    SetColor(fp_pixbg);
    SetFillpattern(0);
    Box(0, 0, wid, hei);

    SetColor(fp_foreg);
    if (i == 1) {
        SetFillpattern(0);
        Box(0, 0, wid, hei);
    }
    else if (i > 1) {
        int indx = i - 2;
        sTpmap *p = Tech()->GetDefaultMap(indx + 16*fp_pattern_bank);
        if (p && p->map) {
            set_fp(p->map, p->nx, p->ny);
            Box(0, 0, wid, hei);
            SetFillpattern(0);
        }
    }
#ifdef NEW_DRW
    GetDrawable()->copy_pixmap_to_window(GC(), 0, 0, wid, hei);
#else
    if (fp_pixmap) {
        gdk_window_copy_area(gtk_widget_get_window(fp_stores[i]), GC(), 0, 0,
            gd_window, 0, 0, wid, hei);
        fp_pm_widget = fp_stores[i];
    }
#endif
}


// Show a box representing a pixel of the fillpattern.
//
void
sFpe::show_pixel(int i, int j)
{
    int x = fp_spa + j*fp_epsz + 1;
    int y = fp_spa + i*fp_epsz + 1;
    SetFillpattern(0);
    Box(x, y, x + fp_epsz - 2, y + fp_epsz - 2);
}


// Create the pixmap for the fillpattern given in map, and set
// the GC to use this pixmap.
//
void
sFpe::set_fp(unsigned char *pmap, int x, int y)
{
    if (!fp_fp)
        fp_fp = new GRfillType;
    fp_fp->newMap(x, y, pmap);      // Destroy/create new pixel map.
    DefineFillpattern(fp_fp);       // Destroy/create new pixmap.
    SetFillpattern(fp_fp);          // Save the pixmap for rendering.
}


// Given window coordinates x, y, return the pixel placement in the
// array j, i.
//
bool
sFpe::getij(int *x, int *y)
{
    int xx = *x;
    int yy = *y;
    xx -= fp_spa;
    xx /= fp_epsz;
    yy -= fp_spa;
    yy /= fp_epsz;
    if (xx >= 0 && xx < fp_nx && yy >= 0 && yy < fp_ny) {
        *x = xx;
        *y = yy;
        return (true);
    }
    return (false);
}


// Set the j, i pixel according to the mode.
//
void
sFpe::set_pixel(int i, int j, FPSETtype mode)
{
    int bpl = (fp_nx + 7)/8;
    unsigned char *a = fp_array + i*bpl;
    unsigned int d = *a++;
    for (int k = 1; k < bpl; k++)
        d |= *a++ << k*8;
    unsigned int mask = 1 << j;
    
    if (mode == FPSETflip) {
        if (d & mask) {
            SetColor(fp_pixbg);
            d &= ~mask;
        }
        else {
            SetColor(fp_foreg);
            d |= mask;
        }
    }
    else if (mode == FPSETon) {
        SetColor(fp_foreg);
        d |= mask;
    }
    else {
        SetColor(fp_pixbg);
        d &= ~mask;
    }
    j /= 8;
    fp_array[i*bpl + j] = d >> j*8;
}


// Return the polarity of the pixel j, i.
//
FPSETtype
sFpe::get_pixel(int i, int j)
{
    int bpl = (fp_nx + 7)/8;
    unsigned char *a = fp_array + i*bpl;
    unsigned int d = *a++;
    for (int k = 1; k < bpl; k++)
        d |= *a++ << k*8;
    unsigned int mask = 1 << j;
    return ((d & mask) ? FPSETon : FPSEToff);
}


// Set the pixels in the array in a line from j1, i1 to j2, i2.
//
void
sFpe::line(int x1, int y1, int x2, int y2, FPSETtype mode)
{
    int i;
    double r = 0.0;
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (!dy) {
        if (x1 > x2) {
            i = x2;
            x2 = x1;
            x1 = i;
        }
        for (i = x1; i <= x2; i++)
            set_pixel(y1, i, mode);
    }
    else if (!dx) {
        if (y1 > y2) {
            i = y2;
            y2 = y1;
            y1 = i;
        }
        for (i = y1; i < y2; i++)
            set_pixel(i, x1, mode);
    }
    else if (abs(dx) > abs(dy)) {
        if (x1 > x2) {
            i = x2;
            x2 = x1;
            x1 = i;
            i = y2;
            y2 = y1;
            y1 = i;
        }
        for (i = x1; i <= x2; i++) {
            set_pixel((int)(y1 + r + .5), i, mode);
            r += (double)dy/dx;
        }
    }
    else {
        if (y1 > y2) {
            i = y2;
            y2 = y1;
            y1 = i;
            i = x2;
            x2 = x1;
            x1 = i;
        }
        for (i = y1; i <= y2; i++) {
            set_pixel(i, (int)(x1 + r + .5), mode);
            r += (double)dx/dy;
        }
    }
}


// Set the pixels in the array which are on the boundary of the
// box defined by jmin, imin to jmax, imax.
//
void
sFpe::box(int imin, int imax, int jmin, int jmax, FPSETtype mode)
{
    int i;
    for (i = jmin; i <= jmax; i++)
        set_pixel(imax, i, mode);
    if (imin != imax) {
        for (i = jmin; i <= jmax; i++)
            set_pixel(imin, i, mode);
        imin++;
        for (i = imin; i < imax; i++)
            set_pixel(i, jmin, mode);
        for (i = imin; i < imax; i++)
            set_pixel(i, jmax, mode);
    }
}


// Load the default fillpattern at x, y into the editor.
//
void
sFpe::def_to_sample(LayerFillData *dd)
{
    fp_nx = dd->d_nx;
    fp_ny = dd->d_ny;
    int bpl = (fp_nx + 7)/8;
    int sz = fp_ny*bpl;
    memset(fp_array, 0, sizeof(fp_array));
    memcpy(fp_array, dd->d_data, fp_ny*bpl);
    sb_nx.set_value(fp_nx);
    sb_ny.set_value(fp_ny);

    bool nonz = false;
    bool nonff = false;
    for (int i = 0; i < sz; i++) {
        if (fp_array[i])
            nonz = true;
        if (fp_array[i] != 0xff)
            nonff = true;
    }
    if (!nonz || !nonff) {
        GRX->SetStatus(fp_outl, false);
        GRX->SetStatus(fp_fat, false);
    }

    redraw_edit();
    redraw_sample();
}


// Store the present pattern map into the default pattern storage area
// pointed to, and redisplay.
//
void
sFpe::sample_to_def(LayerFillData *dd, int indx)
{
    if (indx <= 1) {
        // can't reset solid or open
        return;
    }

    indx -= 2;
    sTpmap *p = Tech()->GetDefaultMap(indx + 16*fp_pattern_bank);
    if (p) {
        delete [] p->map;
        p->nx = dd->d_nx;
        p->ny = dd->d_ny;
        int sz = fp_ny*((fp_nx + 7)/8);
        p->map = new unsigned char[sz];
        bool nonz = false;
        bool nonff = false;
        for (int i = 0; i < sz; i++) {
            p->map[i] = dd->d_data[i];
            if (dd->d_data[i])
                nonz = true;
            if (fp_array[i] != 0xff)
                nonff = true;
        }
        if (!nonz) {
            delete [] p->map;
            p->map = 0;
            p->nx = p->ny = 0;
        }
        if (!nonff)
            p->nx = p->ny = 8;
        redraw_store(indx+2);
    }
    SetFillpattern(0);
}


// Load the current layer pattern into the sample or default areas.
//
void
sFpe::layer_to_def_or_sample(LayerFillData *dd, int indx)
{
    if (indx < 0) {
        fp_nx = dd->d_nx;
        fp_ny = dd->d_ny;
        memset(fp_array, 0, sizeof(fp_array));
        memcpy(fp_array, dd->d_data, fp_ny*((fp_nx + 7)/8));
        sb_nx.set_value(fp_nx);
        sb_ny.set_value(fp_ny);

        if (dd->d_flags & LFD_OUTLINE) {
            GRX->SetStatus(fp_outl, true);
            gtk_widget_set_sensitive(fp_fat, true);
            GRX->SetStatus(fp_fat, (dd->d_flags & LFD_FAT));
        }
        else {
            GRX->SetStatus(fp_outl, false);
            GRX->SetStatus(fp_fat, false);
            gtk_widget_set_sensitive(fp_fat, false);
        }
        GRX->SetStatus(fp_cut, (dd->d_flags & LFD_CUT));

        if (LT()->CurLayer())
            fp_foreg = dsp_prm(LT()->CurLayer())->pixel();
        fp_pixbg = DSP()->Color(BackgroundColor);
        for (int i = 0; i < 18; i++)
            redraw_store(i);
        redraw_edit();
        redraw_sample();
    }
    else {
        if (indx <= 1) {
            // can't reset solid or open
            return;
        }
        indx -= 2;
        sTpmap *p = Tech()->GetDefaultMap(indx + 16*fp_pattern_bank);
        if (p) {
            delete [] p->map;
            p->nx = dd->d_nx;
            p->ny = dd->d_ny;
            int sz = p->ny*((p->nx + 7)/8);
            p->map = new unsigned char[sz];
            memcpy(p->map, dd->d_data, sz);
            redraw_store(indx+2);
        }
        SetFillpattern(0);
    }
}


// Set the pattern for the layer.
//
void
sFpe::pattern_to_layer(LayerFillData *dd, CDl *ld)
{
    if (!ld)
        return;
    if (ld->isInvisible()) {
        Log()->ErrorLog(mh::Processing,
            "Can't transfer fill pattern to invisible layer.");
        return;
    }
    bool nonz = false;
    bool nonff = false;

    int sz = dd->d_ny*((dd->d_nx + 7)/8);
    for (int i = 0; i < sz; i++) {
        if (dd->d_data[i])
            nonz = true;
        if (dd->d_data[i] != 0xff)
            nonff = true;
    }
    if (!nonff) {
        // solid fill
        ld->setFilled(true);
        ld->setOutlined(false);
        ld->setOutlinedFat(false);
        ld->setCut(false);
        defineFillpattern(dsp_prm(ld)->fill(), 8, 8, 0);
    }
    else {
        if (!nonz) {
            // empty fill
            ld->setFilled(false);
            defineFillpattern(dsp_prm(ld)->fill(), 8, 8, 0);
        }
        else {
            // stippled
            defineFillpattern(dsp_prm(ld)->fill(), dd->d_nx, dd->d_ny,
                dd->d_data);
            ld->setFilled(true);
        }
        if (GRX->GetStatus(fp_outl)) {
            ld->setOutlined(true);
            ld->setOutlinedFat(GRX->GetStatus(fp_fat));
        }
        else {
            ld->setOutlined(false);
            ld->setOutlinedFat(false);
        }
        ld->setCut(GRX->GetStatus(fp_cut));
    }
    SetFillpattern(0);
    LT()->ShowLayerTable();
    XM()->PopUpLayerPalette(0, MODE_UPD, false, 0);
}


// Static function.
// Set the handlers for a drawing area.
//
void
sFpe::fp_connect_sigs(GtkWidget *darea, bool dnd_src, bool dnd_rcvr)
{
    gtk_widget_add_events(darea, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(darea), "button-press-event",
        G_CALLBACK(fp_button_press_hdlr), 0);
    gtk_widget_add_events(darea, GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(G_OBJECT(darea), "button-release-event",
        G_CALLBACK(fp_button_rel_hdlr), 0);
    gtk_widget_add_events(darea, GDK_KEY_PRESS_MASK);
    g_signal_connect_after(G_OBJECT(darea), "key-press-event",
        G_CALLBACK(fp_key_press_hdlr), 0);
    gtk_widget_add_events(darea, GDK_POINTER_MOTION_MASK);
    g_signal_connect(G_OBJECT(darea), "motion-notify-event",
        G_CALLBACK(fp_motion_hdlr), 0);
    if (dnd_src) {
        // source
        g_signal_connect(G_OBJECT(darea), "drag-data-get",
            G_CALLBACK(fp_source_drag_data_get), 0);
        g_signal_connect(G_OBJECT(darea), "drag-begin",
            G_CALLBACK(fp_source_drag_begin), 0);
        g_signal_connect(G_OBJECT(darea), "drag-end",
            G_CALLBACK(fp_source_drag_end), 0);
    }
    if (dnd_rcvr) {
        // destination
        GtkDestDefaults DD = (GtkDestDefaults)
            (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP);
        gtk_drag_dest_set(gtk_widget_get_parent(darea), DD, target_table,
            n_targets, GDK_ACTION_COPY);
        g_signal_connect_after(G_OBJECT(gtk_widget_get_parent(darea)),
            "drag-data-received", G_CALLBACK(fp_target_drag_data_received), 0);
        g_signal_connect(G_OBJECT(gtk_widget_get_parent(darea)),
            "drag-leave", G_CALLBACK(fp_target_drag_leave), 0);
        g_signal_connect(G_OBJECT(gtk_widget_get_parent(darea)),
            "drag-motion", G_CALLBACK(fp_target_drag_motion), 0);
    }
}


// Static function.
// Set drag data.
//
void
sFpe::fp_source_drag_data_get(GtkWidget *widget, GdkDragContext*,
    GtkSelectionData *data, guint, guint, void*)
{
    if (!Fpe)
        return;
    LayerFillData dd;
    bool isset = false;
    if (widget == Fpe->fp_sample) {
        dd.d_from_sample = true;
        isset = true;
        dd.d_nx = Fpe->fp_nx;
        dd.d_ny = Fpe->fp_ny;
        memcpy(dd.d_data, Fpe->fp_array, dd.d_ny*((dd.d_nx + 7)/8));
    }
    else {
        for (int i = 0; i < 18; i++) {
            if (widget == Fpe->fp_stores[i]) {
                dd.d_layernum = i;
                isset = true;
                if (i == 0) {
                    dd.d_nx = 8;
                    dd.d_ny = 8;
                    memset(dd.d_data, 0, 8);
                }
                else if (i == 1) {
                    dd.d_nx = 8;
                    dd.d_ny = 8;
                    memset(dd.d_data, 0xff, 8);
                }
                else {
                    sTpmap *p = Tech()->GetDefaultMap(
                        i-2 + 16*Fpe->fp_pattern_bank);
                    if (p && p->map) {
                        dd.d_nx = p->nx;
                        dd.d_ny = p->ny;
                        memcpy(dd.d_data, p->map, dd.d_ny*((dd.d_nx + 7)/8));
                    }
                    else {
                        dd.d_nx = 8;
                        dd.d_ny = 8;
                        memset(dd.d_data, 0, 8);
                    }
                }
                break;
            }
        }
    }
    if (isset) {
        gtk_selection_data_set(data, gtk_selection_data_get_target(data),
            8, (unsigned char*)&dd, sizeof(LayerFillData));
    }
}


// Static function.
// Set the pixmap.
//
void
sFpe::fp_source_drag_begin(GtkWidget*, GdkDragContext *context, gpointer)
{
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_xpm_data(fillpattern_xpm);
    gtk_drag_set_icon_pixbuf(context, pixbuf, -2, -2);
    win_bag::HaveDrag = true;
}


// Static function.
void
sFpe::fp_source_drag_end(GtkWidget*, GdkDragContext*, gpointer)
{
    win_bag::HaveDrag = false;
}


// Static function.
// Drop-data handler.
//
void
sFpe::fp_target_drag_data_received(GtkWidget *widget, GdkDragContext *context,
    gint, gint, GtkSelectionData *data, guint, guint time)
{
    if (Fpe && gtk_selection_data_get_length(data) >= 0 &&
            gtk_selection_data_get_format(data) == 8) {
        LayerFillData *dd = (LayerFillData*)gtk_selection_data_get_data(data);
        if (widget == gtk_widget_get_parent(Fpe->fp_sample) ||
                widget == gtk_widget_get_parent(Fpe->fp_editor)) {
            if (dd->d_from_layer)
                Fpe->layer_to_def_or_sample(dd, -1);
            else if (dd->d_from_sample) {
                if (gdk_drag_context_get_source_window(context) !=
                        gdk_drag_context_get_dest_window(context))
                    // from another process
                    Fpe->def_to_sample(dd);
            }
            else
                Fpe->def_to_sample(dd);
        }
        else {
            for (int i = 0; i < 18; i++) {
                if (widget == gtk_widget_get_parent(Fpe->fp_stores[i])) {
                    if (dd->d_from_layer)
                        Fpe->layer_to_def_or_sample(dd, i);
                    else if (dd->d_from_sample)
                        Fpe->sample_to_def(dd, i);
                    else {
                        // no drag between def boxes
                        gtk_drag_finish(context, false, false, time);
                        return;
                    }
                    break;
                }
            }
        }
        gtk_drag_finish(context, true, false, time);
        return;
    }
    gtk_drag_finish(context, false, false, time);
}


// The default highlighting action causes a redraw of the main viewport.
// Avoid this by handling highlighting here.

// Static function.
gboolean
sFpe::fp_target_drag_motion(GtkWidget *widget, GdkDragContext*, gint, gint,
    guint)
{
    if (!g_object_get_data(G_OBJECT(widget), "drag_hlite")) {
        gtk_drag_highlight(widget);
        g_object_set_data(G_OBJECT(widget), "drag_hlite", (void*)1);
    }
    return (true);
}


// Static function.
void
sFpe::fp_target_drag_leave(GtkWidget *widget, GdkDragContext*, guint)
{
    // called on drop, too
    if (g_object_get_data(G_OBJECT(widget), "drag_hlite")) {
        gtk_drag_unhighlight(widget);
//XXX        g_object_remove_data(G_OBJECT(widget), "drag_hlite");
        int i = g_object_replace_data(G_OBJECT(widget), "drag_hlite", 0, 0, 0, 0);
if (i == 0)
printf("g_object_replace_data didn't work in gtkfillp.cc\n");
    }
}


// Static function.
int
sFpe::fp_config_hdlr(GtkWidget*, GdkEvent*, void*)
{
    Fpe->fp_pm_widget = 0;
    // Can't call these before we have a window!
    Fpe->sb_nx.set_value(Fpe->fp_nx);
    Fpe->sb_ny.set_value(Fpe->fp_ny);
    return (false);
}


// Static function.
// Redraw handler, editing window.
//
int
sFpe::fp_redraw_edit_hdlr(GtkWidget*, GdkEvent *event, void*)
{
#ifdef NEW_DRW
#else
    if (Fpe->fp_pm_widget == Fpe->fp_editor) {
        GdkEventExpose *pev = (GdkEventExpose*)event;
        gdk_window_copy_area(gtk_widget_get_window(Fpe->fp_editor), Fpe->GC(),
            pev->area.x, pev->area.y, Fpe->fp_pixmap,
            pev->area.x, pev->area.y, pev->area.width, pev->area.height);
    }
    else
#endif
        Fpe->redraw_edit();
    return (true);
}


// Static function.
// Redraw handler, sample window.
//
int
sFpe::fp_redraw_sample_hdlr(GtkWidget*, GdkEvent *event, void*)
{
#ifdef NEW_DRW
#else
    if (Fpe->fp_pm_widget == Fpe->fp_sample) {
        GdkEventExpose *pev = (GdkEventExpose*)event;
        gdk_window_copy_area(gtk_widget_get_window(Fpe->fp_sample), Fpe->GC(),
            pev->area.x, pev->area.y, Fpe->fp_pixmap,
            pev->area.x, pev->area.y, pev->area.width, pev->area.height);
    }
    else
#endif
        Fpe->redraw_sample();
    return (true);
}


// Static function.
// Redraw handler, store windows.
//
int
sFpe::fp_redraw_store_hdlr(GtkWidget*, GdkEvent *event, void *arg)
{
    int i = (intptr_t)arg;
#ifdef NEW_DRW
#else
    if (Fpe->fp_pm_widget == Fpe->fp_stores[i]) {
        GdkEventExpose *pev = (GdkEventExpose*)event;
        gdk_window_copy_area(gtk_widget_get_window(Fpe->fp_stores[i]),
            Fpe->GC(),
            pev->area.x, pev->area.y, Fpe->fp_pixmap,
            pev->area.x, pev->area.y, pev->area.width, pev->area.height);
    }
    else
#endif
        Fpe->redraw_store(i);
    return (true);
}


// Static function.
// Button press handler, handles the pixel editor and drag/drop detection.
//
int
sFpe::fp_button_press_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (event->type != GDK_BUTTON_PRESS)
        return (true);
    if (!Fpe)
        return (false);
    GdkEventButton *bev = (GdkEventButton*)event;
    if (caller == Fpe->fp_editor) {
        // pixel editor
        Fpe->fp_jj = (int)bev->x;
        Fpe->fp_ii = (int)bev->y;
        Fpe->fp_downbtn = 0;
        if (Fpe->getij(&Fpe->fp_jj, &Fpe->fp_ii)) {
            Fpe->fp_downbtn = Kmap()->ButtonMap(bev->button);
            if (Fpe->fp_downbtn == 1 &&
                    bev->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
                Fpe->fp_downbtn = 2;
            int refx = Fpe->fp_spa + Fpe->fp_jj*Fpe->fp_epsz + Fpe->fp_epsz/2;
            int refy = Fpe->fp_spa + Fpe->fp_ii*Fpe->fp_epsz + Fpe->fp_epsz/2;
            Fpe->SetGhost(&drawghost, refx, refy);
        }
        return (true);
    }

    Fpe->fp_dragging = true;
    Fpe->fp_drag_btn = Kmap()->ButtonMap(bev->button);
    Fpe->fp_drag_x = (int)bev->x;
    Fpe->fp_drag_y = (int)bev->y;
    return (true);
}


// Static function.
// Button release callback.  The pixel editor has several modes,
// depending on the button used, whether it is clicked or held
// and dragged, and whether the shift key is down.  If the buttons
// are clicked, the target pixel is acted on.  If held and dragged,
// a region of pixels is acted on, indicated by a "ghost" cursor
// box.
//
//    button  figure       none  Shift   Ctrl
//    1       solid rect   flip  set     unset
//    2       outl rect    flip  set     unset
//    3       line         flip  set     unset
//
// If Shift or Ctrl is down *before* button 1 is prssed, the action
// will be as for button 2.
//
int
sFpe::fp_button_rel_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    GdkEventButton *bev = (GdkEventButton*)event;
    if (!Fpe)
        return (false);
    int btn = Fpe->fp_downbtn;
    Fpe->fp_downbtn = 0;
    if (caller == Fpe->fp_editor) {
        if (!btn)
            return (true);

        Fpe->SetGhost(0, 0, 0);

        int jo = Fpe->fp_jj;
        int io = Fpe->fp_ii;
        Fpe->fp_jj = (int)bev->x;
        Fpe->fp_ii = (int)bev->y;
        if (!Fpe->getij(&Fpe->fp_jj, &Fpe->fp_ii))
            return (true);
        int imin = (io < Fpe->fp_ii ? io : Fpe->fp_ii);
        int imax = (io > Fpe->fp_ii ? io : Fpe->fp_ii);
        int jmin = (jo < Fpe->fp_jj ? jo : Fpe->fp_jj);
        int jmax = (jo > Fpe->fp_jj ? jo : Fpe->fp_jj);
        switch (btn) {
        case 1:
            for (io = imin; io <= imax; io++) {
                for (jo = jmin; jo <= jmax; jo++) {
                    if (bev->state & GDK_SHIFT_MASK)
                        Fpe->set_pixel(io, jo, FPSETon);
                    else if (bev->state & GDK_CONTROL_MASK)
                        Fpe->set_pixel(io, jo, FPSEToff);
                    else
                        Fpe->set_pixel(io, jo, FPSETflip);
                }
            }
            Fpe->redraw_edit();
            Fpe->redraw_sample();
            break;
        case 2:
            if (bev->state & GDK_SHIFT_MASK)
                Fpe->box(imin, imax, jmin, jmax, FPSETon);
            else if (bev->state & GDK_CONTROL_MASK)
                Fpe->box(imin, imax, jmin, jmax, FPSEToff);
            else
                Fpe->box(imin, imax, jmin, jmax, FPSETflip);
            Fpe->redraw_edit();
            Fpe->redraw_sample();
            break;
        case 3:
            if (bev->state & GDK_SHIFT_MASK)
                Fpe->line(jo, io, Fpe->fp_jj, Fpe->fp_ii, FPSETon);
            else if (bev->state & GDK_CONTROL_MASK)
                Fpe->line(jo, io, Fpe->fp_jj, Fpe->fp_ii, FPSEToff);
            else
                Fpe->line(jo, io, Fpe->fp_jj, Fpe->fp_ii, FPSETflip);
            Fpe->redraw_edit();
            Fpe->redraw_sample();
            break;
        }
    }
    else
        Fpe->fp_dragging = false;
    return (true);
}


// Static function.
// Key press callback.  The arrow keys rotate the pixel array in
// the direction of the arrow.
//
int
sFpe::fp_key_press_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    int i, j;
    FPSETtype tmp;
    switch (event->key.keyval) {
    case GDK_Right:
        for (i = 0; i < Fpe->fp_ny; i++) {
            tmp = Fpe->get_pixel(i, 0);
            for (j = 1; j < Fpe->fp_nx; j++)
                Fpe->set_pixel(i, j - 1, Fpe->get_pixel(i, j));
            Fpe->set_pixel(i, Fpe->fp_nx - 1, tmp);
        }
        Fpe->redraw_edit();
        Fpe->redraw_sample();
        break;
    case GDK_Left:
        for (i = 0; i < Fpe->fp_ny; i++) {
            tmp = Fpe->get_pixel(i, Fpe->fp_nx - 1);
            for (j = Fpe->fp_nx - 1; j >= 1; j--)
                Fpe->set_pixel(i, j, Fpe->get_pixel(i, j - 1));
            Fpe->set_pixel(i, 0, tmp);
        }
        Fpe->redraw_edit();
        Fpe->redraw_sample();
        break;
    case GDK_Up:
        for (j = 0; j < Fpe->fp_nx; j++) {
            tmp = Fpe->get_pixel(Fpe->fp_ny - 1, j);
            for (i = Fpe->fp_ny - 1; i >= 1; i--)
                Fpe->set_pixel(i, j, Fpe->get_pixel(i - 1, j));
            Fpe->set_pixel(0, j, tmp);
        }
        Fpe->redraw_edit();
        Fpe->redraw_sample();
        break;
    case GDK_Down:
        for (j = 0; j < Fpe->fp_nx; j++) {
            tmp = Fpe->get_pixel(0, j);
            for (i = 1; i < Fpe->fp_ny; i++)
                Fpe->set_pixel(i - 1, j, Fpe->get_pixel(i, j));
            Fpe->set_pixel(Fpe->fp_ny - 1, j, tmp);
        }
        Fpe->redraw_edit();
        Fpe->redraw_sample();
        break;
    }
    return (true);
}


// Static function.
// Set focus so we can see arrow keys.
//
int
sFpe::fp_enter_hdlr(GtkWidget *caller, GdkEvent*, void*)
{
    // pointer entered the fill editor
    gtk_widget_set_can_focus(caller, true);
    gtk_window_set_focus(GTK_WINDOW(Fpe->wb_shell), caller);
    return (true);
}


// Static function.
// Pointer motion handler.  This simply draws an XOR'ed opon box when the
// pointer button is held down, and the pointer is in the pixel editor.
// This is also used for drag/drop detection.
//
int
sFpe::fp_motion_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    GdkEventMotion *mev = (GdkEventMotion*)event;
    if (!Fpe)
        return (false);
    if (caller == Fpe->fp_editor) {
        if (!Fpe->fp_downbtn)
            return (true);
        int x = (int)mev->x;
        int y = (int)mev->y;
        if (!Fpe->getij(&x, &y))
            return (true);

        x = Fpe->fp_spa + x*Fpe->fp_epsz + Fpe->fp_epsz/2;
        y = Fpe->fp_spa + y*Fpe->fp_epsz + Fpe->fp_epsz/2;

#ifdef NEW_DRW
        Fpe->GetDrawable()->set_pixmap(gtk_widget_get_window(Fpe->fp_editor));
        Fpe->GetDrawable()->set_draw_to_window();
#else
        Fpe->gd_window = gtk_widget_get_window(Fpe->fp_editor);
#endif
        Fpe->UndrawGhost();
        Fpe->DrawGhost(x, y);
    }
    else if (Fpe->fp_dragging &&
            (abs((int)event->motion.x - Fpe->fp_drag_x) > 4 ||
            abs((int)event->motion.y - Fpe->fp_drag_y) > 4)) {
        Fpe->fp_dragging = false;
        GtkTargetList *targets = gtk_target_list_new(target_table, n_targets);
        gtk_drag_begin(caller, targets, (GdkDragAction)GDK_ACTION_COPY,
            Fpe->fp_drag_btn, event);
    }
    return (true);
}


// Static function.
// Callback for the Cancel button.
//
void
sFpe::fp_cancel_proc(GtkWidget*, void*)
{
    XM()->PopUpFillEditor(0, MODE_OFF);
}


void
sFpe::fp_mode_proc(GtkWidget*, void *arg)
{
    if (arg) {
        Fpe->fp_editing = true;
        gtk_widget_hide(Fpe->fp_stoctrl);
        gtk_widget_hide(Fpe->fp_stoframe);
        gtk_widget_show(Fpe->fp_editctrl);
        gtk_widget_show(Fpe->fp_editframe);
    }
    else {
        Fpe->fp_editing = false;
        gtk_widget_hide(Fpe->fp_editctrl);
        gtk_widget_hide(Fpe->fp_editframe);
        gtk_widget_show(Fpe->fp_stoctrl);
        gtk_widget_show(Fpe->fp_stoframe);
    }
}


// Static function.
// Callback for the outline button.  When selected, new patterns
// will have the OUTLINED attribute set.
///
void
sFpe::fp_outline_proc(GtkWidget *caller, void*)
{
    if (!Fpe)
        return;
    if (caller == Fpe->fp_outl) {
        bool state = GRX->GetStatus(caller);
        if (state) {
            int sz = Fpe->fp_ny*((Fpe->fp_nx + 7)/8);
            for (int i = 0; i < sz; i++) {
                if (Fpe->fp_array[i]) {
                    state = false;
                    break;
                }
            }
        }
        gtk_widget_set_sensitive(Fpe->fp_fat, state);
    }
    Fpe->redraw_sample();
}


// Static function.
// Callback for pushbuttons.
//
void
sFpe::fp_btn_proc(GtkWidget *widget, void*)
{
    if (!Fpe)
        return;
    const char *name = gtk_widget_get_name(widget);
    if (!name)
        return;
    if (!strcmp(name, "Load")) {
        CDl *ld = LT()->CurLayer();
        if (!ld)
            return;
        LayerFillData dd(ld);
        Fpe->layer_to_def_or_sample(&dd, -1);
    }
    else if (!strcmp(name, "Apply")) {
        CDl *ld = LT()->CurLayer();
        if (!ld)
            return;
        LayerFillData dd;
        dd.d_from_sample = true;
        dd.d_nx = Fpe->fp_nx;
        dd.d_ny = Fpe->fp_ny;
        memcpy(dd.d_data, Fpe->fp_array, dd.d_ny*((dd.d_nx + 7)/8));
        Fpe->pattern_to_layer(&dd, ld);
    }
    else if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("fillpanel"))
    }
    else if (!strcmp(name, "Dumpdefs")) {
        const char *err = Tech()->DumpDefaultStipples();
        if (err)
            Fpe->PopUpMessage(err, true);
        else
            Fpe->PopUpMessage(
                "Created xic_stipples file in current directory.",
                false);
    }
}


void
sFpe::fp_nxy_proc(GtkWidget*, void *arg)
{
    if (Fpe) {
        if ((intptr_t)arg == 1) {
            // Reconfigure the pixel map so that the pattern doesn't
            // turn to crap when the bpl changes.

            int oldx = Fpe->fp_nx;
            Fpe->fp_nx = Fpe->sb_nx.get_value_as_int();
            int oldbpl = (oldx + 7)/8;
            int newbpl = (Fpe->fp_nx + 7)/8;
            if (oldbpl != newbpl) {
                unsigned char ary[128];
                memcpy(ary, Fpe->fp_array, Fpe->fp_ny*oldbpl);
                unsigned char *t = Fpe->fp_array;
                unsigned char *f = ary;
                for (int i = 0; i < Fpe->fp_ny; i++) {
                    if (oldbpl < newbpl) {
                        for (int j = 0; j < newbpl; j++) {
                            if (j < oldbpl)
                                *t++ = *f++;
                            else
                                *t++ = 0;
                        }
                    }
                    else {
                        for (int j = 0; j < oldbpl; j++) {
                            if (j < newbpl)
                                *t++ = *f++;
                            else
                                f++;
                        }
                    }
                }
            }
        }
        else if ((intptr_t)arg == 2)
            Fpe->fp_ny = Fpe->sb_ny.get_value_as_int();
        Fpe->set_fp(Fpe->fp_array, Fpe->fp_nx, Fpe->fp_ny);
        Fpe->SetFillpattern(0);
        Fpe->redraw_edit();
        Fpe->redraw_sample();
    }
}


namespace {
    void setpix(int x, int nx, int y, unsigned char *ary)
    {
        int bpl = (nx + 7)/8;
        unsigned char *a = ary + y*bpl;
        unsigned int d = *a++;
        for (int j = 1; j < bpl; j++)
            d |= *a++ << j*8;
        unsigned int mask = 1 << x;
        d |= mask;
        a = ary + y*bpl;
        *a++ = d;
        for (int j = 1; j < bpl; j++)
            *a++ = d >> (j*8);
    }
}


// Static function.
// Rotate the pixel map by 90 degrees.
//
void
sFpe::fp_rot90_proc(GtkWidget*, void*)
{
    if (!Fpe)
        return;
    int nx = Fpe->fp_nx;
    int ny = Fpe->fp_ny;
    int bpl = (nx + 7)/8;

    unsigned char ary[128];
    memset(ary, 0, 128*sizeof(unsigned char));
    for (int i = 0; i < ny; i++) {
        unsigned char *a = Fpe->fp_array + i*bpl;
        unsigned int d = *a++;
        for (int j = 1; j < bpl; j++)
            d |= *a++ << j*8;
        unsigned int mask = 1;
        for (int j = 0; j < Fpe->fp_nx; j++) {
            bool lit = mask & d;
            mask <<= 1;
            if (lit)
                setpix(i, ny, nx - j - 1, ary);
        }
    }
    Fpe->sb_nx.set_value(ny);
    Fpe->sb_ny.set_value(nx);
    Fpe->fp_nx = ny;
    Fpe->fp_ny = nx;
    memcpy(Fpe->fp_array, ary, 128);
    Fpe->set_fp(ary, ny, nx);
    Fpe->SetFillpattern(0);
    Fpe->redraw_edit();
    Fpe->redraw_sample();
}


// Static function.
// Reflect the pixel map.
//
void
sFpe::fp_refl_proc(GtkWidget*, void *dir)
{
    if (!Fpe)
        return;
    int nx = Fpe->fp_nx;
    int ny = Fpe->fp_ny;
    int bpl = (nx + 7)/8;

    bool flipy = (dir != 0);

    unsigned char ary[128];
    memset(ary, 0, 128*sizeof(unsigned char));
    for (int i = 0; i < ny; i++) {
        unsigned char *a = Fpe->fp_array + i*bpl;
        unsigned int d = *a++;
        for (int j = 1; j < bpl; j++)
            d |= *a++ << j*8;
        unsigned int mask = 1;
        for (int j = 0; j < Fpe->fp_nx; j++) {
            bool lit = mask & d;
            mask <<= 1;
            if (lit) {
                if (flipy)
                    setpix(j, nx, ny - i - 1, ary);
                else
                    setpix(nx - j - 1, nx, i, ary);
            }
        }
    }
    memcpy(Fpe->fp_array, ary, 128);
    Fpe->set_fp(ary, nx, ny);
    Fpe->SetFillpattern(0);
    Fpe->redraw_edit();
    Fpe->redraw_sample();
}


// Static function.
// Callback for the default pattern bank menu.
//
void
sFpe::fp_bank_proc(GtkWidget *caller, void*)
{
    if (Fpe) {
        const char *s = gtk_entry_get_text(GTK_ENTRY(caller));
        int bank = atoi(s);
        if (bank > 0 && bank <= TECH_MAP_SIZE/16) {
            Fpe->fp_pattern_bank = bank - 1;
            for (int i = 0; i < 18; i++)
                Fpe->redraw_store(i);
        }
    }
}


// Static function.
// Draw an open box or line, using the XOR GC.
//
void
sFpe::drawghost(int x0, int y0, int x1, int y1, bool)
{
    if (Fpe) {
        if (Fpe->fp_downbtn == 3)
            Fpe->Line(x0, y0, x1, y1);
        else {
            GRmultiPt p(5);
            p.assign(0, x0, y0);
            p.assign(1, x1, y0);
            p.assign(2, x1, y1);
            p.assign(3, x0, y1);
            p.assign(4, x0, y0);
            Fpe->PolyLine(&p, 5);
        }
    }
}

