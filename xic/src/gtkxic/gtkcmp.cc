
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
#include "fio.h"
#include "fio_compare.h"
#include "cvrt.h"
#include "dsp_inlines.h"
#include "promptline.h"
#include "errorlog.h"
#include "gtkmain.h"
#include "gtkinterf/gtkspinbtn.h"


//-----------------------------------------------------------------------------
// The Compare Layers pop-up
//
// Help system keywords used:
//  xic:diff

namespace {
    // Drag/drop stuff.
    //
    GtkTargetEntry target_table[] = {
        { (char*)"TWOSTRING",   0, 0 },
        { (char*)"CELLNAME",    0, 1 },
        { (char*)"STRING",      0, 2 },
        { (char*)"text/plain",  0, 3 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

    namespace gtkcmp {
        struct sCmp
        {
            friend struct sCmp_store;
            sCmp(GRobject);
            ~sCmp();

            void update();

            GtkWidget *shell() { return (cmp_popup); }

        private:
            void per_cell_obj_page();
            void per_cell_geom_page();
            void flat_geom_page();
            void p1_sens();
            char *compose_arglist();
            bool get_bb(BBox*);
            void set_bb(const BBox*);

            static void cmp_cancel_proc(GtkWidget*, void*);
            static void cmp_action(GtkWidget*, void*);
            static void cmp_p1_action(GtkWidget*, void*);
            static void cmp_p1_fltr_proc(GtkWidget*, void*);
            static int cmp_popup_btn_proc(GtkWidget*, GdkEvent*, void*);
            static void cmp_sto_menu_proc(GtkWidget*, void*);
            static void cmp_rcl_menu_proc(GtkWidget*, void*);
            static void cmp_drag_data_received(GtkWidget*, GdkDragContext*,
                gint, gint, GtkSelectionData*, guint, guint);

            GRobject cmp_caller;
            GtkWidget *cmp_popup;
            GtkWidget *cmp_mode;
            GtkWidget *cmp_fname1;
            GtkWidget *cmp_fname2;
            GtkWidget *cmp_cnames1;
            GtkWidget *cmp_cnames2;
            GtkWidget *cmp_diff_only;
            GtkWidget *cmp_layer_list;
            GtkWidget *cmp_layer_use;
            GtkWidget *cmp_layer_skip;
            GTKspinBtn sb_max_errs;

            GtkWidget *cmp_p1_expand_arrays;
            GtkWidget *cmp_p1_slop;
            GtkWidget *cmp_p1_dups;
            GtkWidget *cmp_p1_boxes;
            GtkWidget *cmp_p1_polys;
            GtkWidget *cmp_p1_wires;
            GtkWidget *cmp_p1_labels;
            GtkWidget *cmp_p1_insta;
            GtkWidget *cmp_p1_boxes_prp;
            GtkWidget *cmp_p1_polys_prp;
            GtkWidget *cmp_p1_wires_prp;
            GtkWidget *cmp_p1_labels_prp;
            GtkWidget *cmp_p1_insta_prp;
            GtkWidget *cmp_p1_recurse;
            GtkWidget *cmp_p1_phys;
            GtkWidget *cmp_p1_elec;
            GtkWidget *cmp_p1_cell_prp;
            GtkWidget *cmp_p1_fltr;
            GtkWidget *cmp_p1_setup;
            PrpFltMode cmp_p1_fltr_mode;

            GtkWidget *cmp_p2_expand_arrays;
            GtkWidget *cmp_p2_recurse;
            GtkWidget *cmp_p2_insta;

            GtkWidget *cmp_p3_aoi_use;
            GtkWidget *cmp_p3_s_btn;
            GtkWidget *cmp_p3_r_btn;
            GtkWidget *cmp_p3_s_menu;
            GtkWidget *cmp_p3_r_menu;
            GTKspinBtn sb_p3_aoi_left;
            GTKspinBtn sb_p3_aoi_bottom;
            GTKspinBtn sb_p3_aoi_right;
            GTKspinBtn sb_p3_aoi_top;
            GTKspinBtn sb_p3_fine_grid;
            GTKspinBtn sb_p3_coarse_mult;
        };

        sCmp *Cmp;

        // Entry storage, all entries are persistent.
        //
        struct sCmp_store
        {
            sCmp_store();
            ~sCmp_store();

            void save();
            void recall();
            void recall_p1();
            void recall_p2();
            void recall_p3();

            char *cs_file1;
            char *cs_file2;
            char *cs_cells1;
            char *cs_cells2;
            char *cs_layers;
            int cs_mode;
            bool cs_layer_only;
            bool cs_layer_skip;
            bool cs_differ;
            int cs_max_errs;

            bool cs_p1_recurse;
            bool cs_p1_expand_arrays;
            bool cs_p1_slop;
            bool cs_p1_dups;
            bool cs_p1_boxes;
            bool cs_p1_polys;
            bool cs_p1_wires;
            bool cs_p1_labels;
            bool cs_p1_insta;
            bool cs_p1_boxes_prp;
            bool cs_p1_polys_prp;
            bool cs_p1_wires_prp;
            bool cs_p1_labels_prp;
            bool cs_p1_insta_prp;
            bool cs_p1_elec;
            bool cs_p1_cell_prp;
            unsigned char cs_p1_fltr;

            bool cs_p2_recurse;
            bool cs_p2_expand_arrays;
            bool cs_p2_insta;

            bool cs_p3_use_window;
            double cs_p3_left;
            double cs_p3_bottom;
            double cs_p3_right;
            double cs_p3_top;
            int cs_p3_fine_grid;
            int cs_p3_coarse_mult;
        };

        sCmp_store cmp_storage;

    }
}

using namespace gtkcmp;


void
cConvert::PopUpCompare(GRobject caller, ShowMode mode)
{
    if (!GRX || !GTKmainwin::self())
        return;
    if (mode == MODE_OFF) {
        delete Cmp;
        return;
    }
    if (mode == MODE_UPD) {
        if (Cmp)
            Cmp->update();
        return;
    }
    if (Cmp)
        return;

    new sCmp(caller);
    if (!Cmp->shell()) {
        delete Cmp;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Cmp->shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UR), Cmp->shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(Cmp->shell());
}


sCmp::sCmp(GRobject c)
{
    Cmp = this;
    cmp_caller = c;
    cmp_popup = 0;
    cmp_mode = 0;
    cmp_fname1 = 0;
    cmp_fname2 = 0;
    cmp_cnames1 = 0;
    cmp_cnames2 = 0;
    cmp_diff_only = 0;
    cmp_layer_list = 0;
    cmp_layer_use = 0;
    cmp_layer_skip = 0;
    cmp_p1_expand_arrays = 0;
    cmp_p1_slop = 0;
    cmp_p1_dups = 0;
    cmp_p1_boxes = 0;
    cmp_p1_polys = 0;
    cmp_p1_wires = 0;
    cmp_p1_labels = 0;
    cmp_p1_insta = 0;
    cmp_p1_boxes_prp = 0;
    cmp_p1_polys_prp = 0;
    cmp_p1_wires_prp = 0;
    cmp_p1_labels_prp = 0;
    cmp_p1_insta_prp = 0;
    cmp_p1_recurse = 0;
    cmp_p1_phys = 0;
    cmp_p1_elec = 0;
    cmp_p1_cell_prp = 0;
    cmp_p1_fltr = 0;
    cmp_p1_setup = 0;
    cmp_p1_fltr_mode = PrpFltDflt;
    cmp_p2_expand_arrays = 0;
    cmp_p2_recurse = 0;
    cmp_p2_insta = 0;
    cmp_p3_aoi_use = 0;
    cmp_p3_s_btn = 0;
    cmp_p3_r_btn = 0;
    cmp_p3_s_menu = 0;
    cmp_p3_r_menu = 0;

    cmp_popup = gtk_NewPopup(0, "Compare Layouts", cmp_cancel_proc, 0);
    if (!cmp_popup)
        return;

    // Without this, spin entries sometimes freeze up for some reason.
    g_object_set_data(G_OBJECT(cmp_popup), "no_prop_key", (void*)1);

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(cmp_popup), form);
    int rowcnt = 0;

    //
    // Label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new(
        "Compare Cells/Geometry Between Layouts");
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
        G_CALLBACK(cmp_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Comparison mode selection notebook
    //
    cmp_mode = gtk_notebook_new();
    gtk_widget_show(cmp_mode);
    gtk_table_attach(GTK_TABLE(form), cmp_mode, 0, 2, rowcnt,
        rowcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    per_cell_obj_page();
    per_cell_geom_page();
    flat_geom_page();

    //
    // Left file/cell entries
    //
    GtkWidget *rc = gtk_table_new(2, 2, false);
    gtk_widget_show(rc);
    label = gtk_label_new("Source");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(rc), label, 0, 1, 0, 1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    cmp_fname1 = gtk_entry_new();
    gtk_widget_show(cmp_fname1);
    gtk_table_attach(GTK_TABLE(rc), cmp_fname1, 1, 2, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    label = gtk_label_new("Cells");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(rc), label, 0, 1, 1, 2,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    cmp_cnames1 = gtk_entry_new();
    gtk_widget_show(cmp_cnames1);
    gtk_table_attach(GTK_TABLE(rc), cmp_cnames1, 1, 2, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    frame = gtk_frame_new("<<<");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), rc);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkDestDefaults DD = (GtkDestDefaults)
        (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT);

    // drop site
    gtk_drag_dest_set(cmp_fname1, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(cmp_fname1), "drag-data-received",
        G_CALLBACK(cmp_drag_data_received), 0);
    gtk_drag_dest_set(cmp_cnames1, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    // Don't use connect_after here, see note in cmp_drag_data_received.
    g_signal_connect(G_OBJECT(cmp_cnames1), "drag-data-received",
        G_CALLBACK(cmp_drag_data_received), 0);

    //
    // Right file/cell entries
    //
    rc = gtk_table_new(2, 2, false);
    gtk_widget_show(rc);
    label = gtk_label_new("Source");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(rc), label, 0, 1, 0, 1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    cmp_fname2 = gtk_entry_new();
    gtk_widget_show(cmp_fname2);
    gtk_table_attach(GTK_TABLE(rc), cmp_fname2, 1, 2, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    label = gtk_label_new("Equiv");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(rc), label, 0, 1, 1, 2,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    cmp_cnames2 = gtk_entry_new();
    gtk_widget_show(cmp_cnames2);
    gtk_table_attach(GTK_TABLE(rc), cmp_cnames2, 1, 2, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    frame = gtk_frame_new(">>>");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), rc);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // drop site
    gtk_drag_dest_set(cmp_fname2, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(cmp_fname2), "drag-data-received",
        G_CALLBACK(cmp_drag_data_received), 0);
    gtk_drag_dest_set(cmp_cnames2, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    // Don't use connect_after here, see note in cmp_drag_data_received.
    g_signal_connect(G_OBJECT(cmp_cnames2), "drag-data-received",
        G_CALLBACK(cmp_drag_data_received), 0);

    //
    // Layer list and associated buttons
    //
    rc = gtk_table_new(2, 2, false);
    gtk_widget_show(rc);
    cmp_layer_use = gtk_check_button_new_with_label("Layers Only");
    gtk_widget_show(cmp_layer_use);
    g_signal_connect(G_OBJECT(cmp_layer_use), "clicked",
        G_CALLBACK(cmp_action), this);
    gtk_table_attach(GTK_TABLE(rc), cmp_layer_use, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    cmp_layer_skip = gtk_check_button_new_with_label("Skip Layers");
    gtk_widget_show(cmp_layer_skip);
    g_signal_connect(G_OBJECT(cmp_layer_skip), "clicked",
        G_CALLBACK(cmp_action), this);
    gtk_table_attach(GTK_TABLE(rc), cmp_layer_skip, 1, 2, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    cmp_layer_list = gtk_entry_new();
    gtk_widget_show(cmp_layer_list);
    gtk_table_attach(GTK_TABLE(rc), cmp_layer_list, 0, 2, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    frame = gtk_frame_new("Layer List");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), rc);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Diff only and max differences spin button
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    cmp_diff_only = gtk_check_button_new_with_label("Differ Only");
    gtk_widget_show(cmp_diff_only);
    gtk_box_pack_start(GTK_BOX(hbox), cmp_diff_only, true, true, 0);

    label = gtk_label_new("Maximum Differences");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);

    GtkWidget *sb = sb_max_errs.init(0.0, 0.0, 1e6, 0);
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_start(GTK_BOX(hbox), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);
    gtk_table_attach(GTK_TABLE(form), hsep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Go and Dismiss buttons
    //
    button = gtk_button_new_with_label("Go");
    gtk_widget_set_name(button, "Go");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cmp_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cmp_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(cmp_popup), button);

    cmp_storage.recall();
    p1_sens();
}


sCmp::~sCmp()
{
    if (GRX->GetStatus(cmp_p1_setup))
        GRX->CallCallback(cmp_p1_setup);
    cmp_storage.save();
    Cmp = 0;
    if (cmp_caller)
        GRX->Deselect(cmp_caller);
    if (cmp_p3_s_menu) {
        g_object_unref(cmp_p3_s_menu);
        gtk_widget_destroy(cmp_p3_s_menu);
    }
    if (cmp_p3_r_menu) {
        g_object_unref(cmp_p3_r_menu);
        gtk_widget_destroy(cmp_p3_r_menu);
    }

    if (cmp_popup)
        gtk_widget_destroy(cmp_popup);
}


void
sCmp::update()
{
}


void
sCmp::per_cell_obj_page()
{
    int rcnt = 0;
    GtkWidget *table = gtk_table_new(3, 1, false);
    gtk_widget_show(table);

    //
    // Expand arrays and Recurse buttons
    //
    cmp_p1_recurse = gtk_check_button_new_with_label(
        "Recurse Into Hierarchy");
    gtk_widget_show(cmp_p1_recurse);
    gtk_table_attach(GTK_TABLE(table), cmp_p1_recurse, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    cmp_p1_expand_arrays = gtk_check_button_new_with_label("Expand Arrays");
    gtk_widget_show(cmp_p1_expand_arrays);
    gtk_table_attach(GTK_TABLE(table), cmp_p1_expand_arrays, 2, 3, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;
    cmp_p1_slop = gtk_check_button_new_with_label("Box to Wire/Poly Check");
    gtk_widget_show(cmp_p1_slop);
    gtk_table_attach(GTK_TABLE(table), cmp_p1_slop, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    cmp_p1_dups = gtk_check_button_new_with_label("Ignore Duplicate Objects");
    gtk_widget_show(cmp_p1_dups);
    gtk_table_attach(GTK_TABLE(table), cmp_p1_dups, 2, 3, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    //
    // Object Types group
    //
    GtkWidget *vbox = gtk_vbox_new(false, 2);
    gtk_widget_show(vbox);
    cmp_p1_boxes = gtk_check_button_new_with_label("Boxes");
    gtk_widget_show(cmp_p1_boxes);
    g_signal_connect(G_OBJECT(cmp_p1_boxes), "clicked",
        G_CALLBACK(cmp_p1_action), 0);
    gtk_box_pack_start(GTK_BOX(vbox), cmp_p1_boxes, false, false, 0);
    cmp_p1_polys = gtk_check_button_new_with_label("Polygons");
    gtk_widget_show(cmp_p1_polys);
    g_signal_connect(G_OBJECT(cmp_p1_polys), "clicked",
        G_CALLBACK(cmp_p1_action), 0);
    gtk_box_pack_start(GTK_BOX(vbox), cmp_p1_polys, false, false, 0);
    cmp_p1_wires = gtk_check_button_new_with_label("Wires");
    gtk_widget_show(cmp_p1_wires);
    g_signal_connect(G_OBJECT(cmp_p1_wires), "clicked",
        G_CALLBACK(cmp_p1_action), 0);
    gtk_box_pack_start(GTK_BOX(vbox), cmp_p1_wires, false, false, 0);
    cmp_p1_labels = gtk_check_button_new_with_label("Labels");
    gtk_widget_show(cmp_p1_labels);
    g_signal_connect(G_OBJECT(cmp_p1_labels), "clicked",
        G_CALLBACK(cmp_p1_action), 0);
    gtk_box_pack_start(GTK_BOX(vbox), cmp_p1_labels, false, false, 0);
    cmp_p1_insta = gtk_check_button_new_with_label("Cell Instances");
    gtk_widget_show(cmp_p1_insta);
    g_signal_connect(G_OBJECT(cmp_p1_insta), "clicked",
        G_CALLBACK(cmp_p1_action), 0);
    gtk_box_pack_start(GTK_BOX(vbox), cmp_p1_insta, false, false, 0);
    GtkWidget *frame = gtk_frame_new("Object Types");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_table_attach(GTK_TABLE(table), frame, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // Properties group
    //
    vbox = gtk_vbox_new(false, 2);
    gtk_widget_show(vbox);
    cmp_p1_boxes_prp = gtk_check_button_new_with_label("");
    gtk_widget_show(cmp_p1_boxes_prp);
    gtk_box_pack_start(GTK_BOX(vbox), cmp_p1_boxes_prp, false, false, 0);
    cmp_p1_polys_prp = gtk_check_button_new_with_label("");
    gtk_widget_show(cmp_p1_polys_prp);
    gtk_box_pack_start(GTK_BOX(vbox), cmp_p1_polys_prp, false, false, 0);
    cmp_p1_wires_prp = gtk_check_button_new_with_label("");
    gtk_widget_show(cmp_p1_wires_prp);
    gtk_box_pack_start(GTK_BOX(vbox), cmp_p1_wires_prp, false, false, 0);
    cmp_p1_labels_prp = gtk_check_button_new_with_label("");
    gtk_widget_show(cmp_p1_labels_prp);
    gtk_box_pack_start(GTK_BOX(vbox), cmp_p1_labels_prp, false, false, 0);
    cmp_p1_insta_prp = gtk_check_button_new_with_label("");
    gtk_widget_show(cmp_p1_insta_prp);
    gtk_box_pack_start(GTK_BOX(vbox), cmp_p1_insta_prp, false, false, 0);
    frame = gtk_frame_new("Properties");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_table_attach(GTK_TABLE(table), frame, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    vbox = gtk_vbox_new(false, 2);
    gtk_widget_show(vbox);
    cmp_p1_phys = gtk_radio_button_new_with_label(0, "Physical");
    gtk_widget_show(cmp_p1_phys);
    g_signal_connect(G_OBJECT(cmp_p1_phys), "clicked",
        G_CALLBACK(cmp_p1_action), 0);
    gtk_box_pack_start(GTK_BOX(vbox), cmp_p1_phys, false, false, 0);
    GSList *group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(cmp_p1_phys));
    cmp_p1_elec = gtk_radio_button_new_with_label(group, "Electrical");
    gtk_widget_show(cmp_p1_elec);
    gtk_box_pack_start(GTK_BOX(vbox), cmp_p1_elec, false, false, 0);
    cmp_p1_cell_prp = gtk_check_button_new_with_label("Structure Properties");
    gtk_widget_show(cmp_p1_cell_prp);
    gtk_box_pack_start(GTK_BOX(vbox), cmp_p1_cell_prp, false, false, 0);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    GtkWidget *label = gtk_label_new("Property Filtering:");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(vbox), label, false, false, 0);

    cmp_p1_fltr = gtk_combo_box_text_new();
    gtk_widget_set_name(cmp_p1_fltr, "Filter");
    gtk_widget_show(cmp_p1_fltr);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmp_p1_fltr),
        "Default");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmp_p1_fltr),
        "None");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmp_p1_fltr),
        "Custrom");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cmp_p1_fltr), 0);
    g_signal_connect(G_OBJECT(cmp_p1_fltr), "changed",
        G_CALLBACK(cmp_p1_fltr_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), cmp_p1_fltr, true, true, 0);

    cmp_p1_setup = gtk_toggle_button_new_with_label("Setup");
    gtk_widget_show(cmp_p1_setup);
    gtk_box_pack_start(GTK_BOX(hbox), cmp_p1_setup, false, false, 0);
    g_signal_connect(G_OBJECT(cmp_p1_setup), "clicked",
        G_CALLBACK(cmp_p1_action), (void*)1L);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);

    gtk_table_attach(GTK_TABLE(table), vbox, 2, 3, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *tab_label = gtk_label_new("Per-Cell Objects");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(cmp_mode), table, tab_label);
    cmp_storage.recall_p1();
}


void
sCmp::per_cell_geom_page()
{
    int rcnt = 0;
    GtkWidget *table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);

    //
    // Expand arrays and Recurse buttons
    //
    cmp_p2_recurse = gtk_check_button_new_with_label(
        "Recurse Into Hierarchy");
    gtk_widget_show(cmp_p2_recurse);
    gtk_table_attach(GTK_TABLE(table), cmp_p2_recurse, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    cmp_p2_expand_arrays = gtk_check_button_new_with_label("Expand Arrays");
    gtk_widget_show(cmp_p2_expand_arrays);
    gtk_table_attach(GTK_TABLE(table), cmp_p2_expand_arrays, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    cmp_p2_insta = gtk_check_button_new_with_label("Compare Cell Instances");
    gtk_widget_show(cmp_p2_insta);
    gtk_table_attach(GTK_TABLE(table), cmp_p2_insta, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    GtkWidget *label = gtk_label_new("Physical Cells Only");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(table), label,
        0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *tab_label = gtk_label_new("Per-Cell Geometry");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(cmp_mode), table, tab_label);
    cmp_storage.recall_p2();
}


void
sCmp::flat_geom_page()
{
    int rcnt = 0;
    GtkWidget *table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);

    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    cmp_p3_aoi_use = gtk_check_button_new_with_label("Use Window");
    gtk_widget_set_name(cmp_p3_aoi_use, "Window");
    gtk_widget_show(cmp_p3_aoi_use);
    g_signal_connect(G_OBJECT(cmp_p3_aoi_use), "clicked",
        G_CALLBACK(cmp_action), this);
    gtk_box_pack_start(GTK_BOX(row), cmp_p3_aoi_use, false, false, 0);

    gtk_table_attach(GTK_TABLE(table), row, 0, 4, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    cmp_p3_s_btn = gtk_button_new_with_label("S");
    gtk_widget_set_name(cmp_p3_s_btn, "SaveWin");
    gtk_widget_show(cmp_p3_s_btn);
    gtk_widget_set_size_request(cmp_p3_s_btn, 30, -1);

    char buf[64];
    cmp_p3_s_menu = gtk_menu_new();
    gtk_widget_set_name(cmp_p3_s_menu, "Smenu");
    g_object_ref(cmp_p3_s_menu);
    for (int i = 0; i < FIO_NUM_BB_STORE; i++) {
        sprintf(buf, "Reg %d", i);
        GtkWidget *mi = gtk_menu_item_new_with_label(buf);
        gtk_widget_set_name(mi, buf);
        gtk_widget_show(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(cmp_p3_s_menu), mi);
        g_signal_connect(G_OBJECT(mi), "activate",
            G_CALLBACK(cmp_sto_menu_proc), this);
    }
    g_signal_connect(G_OBJECT(cmp_p3_s_btn), "button-press-event",
        G_CALLBACK(cmp_popup_btn_proc), cmp_p3_s_menu);

    GtkWidget *wnd_l_label = gtk_label_new("Left");
    gtk_widget_show(wnd_l_label);
    gtk_misc_set_padding(GTK_MISC(wnd_l_label), 2, 2);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(hbox), cmp_p3_s_btn, false, false, 0);
    gtk_box_pack_start(GTK_BOX(hbox), wnd_l_label, true, true, 0);

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    int ndgt = CD()->numDigits();

    GtkWidget *sb = sb_p3_aoi_left.init(MICRONS(FIO()->CvtWindow()->left),
        -1e6, 1e6, ndgt);
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(table), sb, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *wnd_b_label = gtk_label_new("Bottom");
    gtk_widget_show(wnd_b_label);
    gtk_misc_set_padding(GTK_MISC(wnd_b_label), 2, 2);
    gtk_table_attach(GTK_TABLE(table), wnd_b_label, 2, 3, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_p3_aoi_bottom.init(MICRONS(FIO()->CvtWindow()->bottom),
        -1e6, 1e6, ndgt);
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(table), sb, 3, 4, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    cmp_p3_r_btn = gtk_button_new_with_label("R");
    gtk_widget_set_name(cmp_p3_r_btn, "RclWin");
    gtk_widget_show(cmp_p3_r_btn);
    gtk_widget_set_size_request(cmp_p3_r_btn, 30, -1);

    cmp_p3_r_menu = gtk_menu_new();
    gtk_widget_set_name(cmp_p3_r_menu, "Rmenu");
    g_object_ref(cmp_p3_r_menu);
    for (int i = 0; i < FIO_NUM_BB_STORE; i++) {
        sprintf(buf, "Reg %d", i);
        GtkWidget *mi = gtk_menu_item_new_with_label(buf);
        gtk_widget_set_name(mi, buf);
        gtk_widget_show(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(cmp_p3_r_menu), mi);
        g_signal_connect(G_OBJECT(mi), "activate",
            G_CALLBACK(cmp_rcl_menu_proc), this);
    }
    g_signal_connect(G_OBJECT(cmp_p3_r_btn), "button-press-event",
        G_CALLBACK(cmp_popup_btn_proc), cmp_p3_r_menu);

    GtkWidget *wnd_r_label = gtk_label_new("Right");
    gtk_widget_show(wnd_r_label);
    gtk_misc_set_padding(GTK_MISC(wnd_r_label), 2, 2);

    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(hbox), cmp_p3_r_btn, false, false, 0);
    gtk_box_pack_start(GTK_BOX(hbox), wnd_r_label, true, true, 0);

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_p3_aoi_right.init(MICRONS(FIO()->CvtWindow()->right),
        -1e6, 1e6, ndgt);
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(table), sb, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *wnd_t_label = gtk_label_new("Top");
    gtk_widget_show(wnd_t_label);
    gtk_misc_set_padding(GTK_MISC(wnd_t_label), 2, 2);
    gtk_table_attach(GTK_TABLE(table), wnd_t_label, 2, 3, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_p3_aoi_top.init(MICRONS(FIO()->CvtWindow()->top),
        -1e6, 1e6, ndgt);
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(table), sb, 3, 4, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    GtkWidget *hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);
    gtk_widget_set_size_request(hsep, -1, 20);
    gtk_table_attach(GTK_TABLE(table), hsep, 0, 4, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    GtkWidget *label = gtk_label_new("Fine Grid");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_p3_fine_grid.init(20.0, 1.0, 100.0, 0);
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(table), sb, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    label = gtk_label_new("Coarse Mult");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(table), label, 2, 3, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_p3_coarse_mult.init(20.0, 1.0, 100.0, 0);
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(table), sb, 3, 4, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    GtkWidget *wnd_frame = gtk_frame_new(0);
    gtk_widget_show(wnd_frame);
    gtk_container_add(GTK_CONTAINER(wnd_frame), table);

    label = gtk_label_new("Physical Cells Only");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(table), label,
        0, 4, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *tab_label = gtk_label_new("Flat Geometry");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(cmp_mode), wnd_frame, tab_label);
    cmp_storage.recall_p3();
}


namespace {
    char *
    strip_sp(const char *str)
    {
        if (!str)
            return (0);
        while (isspace(*str))
            str++;
        if (!*str)
            return (0);
        bool qtd = false;
        if (*str == '"') {
            str++;
            qtd = true;
        }
        char *sret = lstring::copy(str);
        char *t = sret + strlen(sret) - 1;
        while (isspace(*t) && t >= sret)
            *t-- = 0;
        if (qtd && *t == '"')
            *t = 0;
        if (!*sret) {
            delete [] sret;
            return (0);
        }
        return (sret);
    }
}


// Sensitivity logic for the properties check boxes.
//
void
sCmp::p1_sens()
{
    bool b_ok = GRX->GetStatus(cmp_p1_boxes);
    bool p_ok = GRX->GetStatus(cmp_p1_polys);
    bool w_ok = GRX->GetStatus(cmp_p1_wires);
    bool l_ok = GRX->GetStatus(cmp_p1_labels);
    bool c_ok = GRX->GetStatus(cmp_p1_insta);

    gtk_widget_set_sensitive(cmp_p1_boxes_prp, b_ok);
    gtk_widget_set_sensitive(cmp_p1_polys_prp, p_ok);
    gtk_widget_set_sensitive(cmp_p1_wires_prp, w_ok);
    gtk_widget_set_sensitive(cmp_p1_labels_prp, l_ok);
    gtk_widget_set_sensitive(cmp_p1_insta_prp, c_ok);
}


char *
sCmp::compose_arglist()
{
    sLstr lstr;

    const char *s = gtk_entry_get_text(GTK_ENTRY(cmp_fname1));
    char *tok = strip_sp(s);
    if (tok) {
        lstr.add(" -f1 \"");
        lstr.add(tok);
        lstr.add_c('"');
        delete [] tok;
    }

    s = gtk_entry_get_text(GTK_ENTRY(cmp_fname2));
    tok = strip_sp(s);
    if (tok) {
        lstr.add(" -f2 \"");
        lstr.add(tok);
        lstr.add_c('"');
        delete [] tok;
    }

    s = gtk_entry_get_text(GTK_ENTRY(cmp_cnames1));
    tok = strip_sp(s);
    if (tok) {
        lstr.add(" -c1 \"");
        lstr.add(tok);
        lstr.add_c('"');
        delete [] tok;
    }

    s = gtk_entry_get_text(GTK_ENTRY(cmp_cnames2));
    tok = strip_sp(s);
    if (tok) {
        lstr.add(" -c2 \"");
        lstr.add(tok);
        lstr.add_c('"');
        delete [] tok;
    }

    if (GRX->GetStatus(cmp_layer_use) || GRX->GetStatus(cmp_layer_skip)) {
        s = gtk_entry_get_text(GTK_ENTRY(cmp_layer_list));
        tok = strip_sp(s);
        if (tok) {
            lstr.add(" -l \"");
            lstr.add(tok);
            lstr.add_c('"');
            if (GRX->GetStatus(cmp_layer_skip))
                lstr.add(" -s");
            delete [] tok;
        }
    }

    if (GRX->GetStatus(cmp_diff_only))
        lstr.add(" -d");

    s = sb_max_errs.get_string();
    tok = strip_sp(s);
    if (tok) {
        if (atoi(tok) > 0) {
            lstr.add(" -r ");
            lstr.add(tok);
        }
        delete [] tok;
    }

    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(cmp_mode));
    if (page == 0) {
        if (GRX->GetStatus(cmp_p1_recurse))
            lstr.add(" -h");
        if (GRX->GetStatus(cmp_p1_expand_arrays))
            lstr.add(" -x");
        if (GRX->GetStatus(cmp_p1_elec))
            lstr.add(" -e");
        if (GRX->GetStatus(cmp_p1_slop))
            lstr.add(" -b");
        if (GRX->GetStatus(cmp_p1_dups))
            lstr.add(" -n");

        bool has_b = false;
        bool has_p = false;
        bool has_w = false;
        bool has_l = false;
        bool has_c = false;

        char typ[8];
        int cnt = 0;
        if (GRX->GetStatus(cmp_p1_boxes)) {
            typ[cnt++] = 'b';
            has_b = true;
        }
        if (GRX->GetStatus(cmp_p1_polys)) {
            typ[cnt++] = 'p';
            has_p = true;
        }
        if (GRX->GetStatus(cmp_p1_wires)) {
            typ[cnt++] = 'w';
            has_w = true;
        }
        if (GRX->GetStatus(cmp_p1_labels)) {
            typ[cnt++] = 'l';
            has_l = true;
        }
        if (GRX->GetStatus(cmp_p1_insta)) {
            typ[cnt++] = 'c';
            has_c = true;
        }
        typ[cnt] = 0;

        if (typ[0] && strcmp(typ, "bpwc")) {
            lstr.add(" -t ");
            lstr.add(typ);
        }
        else {
            has_b = true;
            has_p = true;
            has_w = true;
            has_c = true;
        }

        cnt = 0;
        if (has_b && GRX->GetStatus(cmp_p1_boxes_prp))
            typ[cnt++] = 'b';
        if (has_p && GRX->GetStatus(cmp_p1_polys_prp))
            typ[cnt++] = 'p';
        if (has_w && GRX->GetStatus(cmp_p1_wires_prp))
            typ[cnt++] = 'w';
        if (has_l && GRX->GetStatus(cmp_p1_labels_prp))
            typ[cnt++] = 'l';
        if (has_c && GRX->GetStatus(cmp_p1_insta_prp))
            typ[cnt++] = 'c';
        if (GRX->GetStatus(cmp_p1_cell_prp))
            typ[cnt++] = 's';
        if (cmp_p1_fltr_mode == PrpFltCstm)
            typ[cnt++] = 'u';
        else if (cmp_p1_fltr_mode == PrpFltNone)
            typ[cnt++] = 'n';

        typ[cnt] = 0;

        if (typ[0]) {
            lstr.add(" -p ");
            lstr.add(typ);
        }
    }
    else if (page == 1) {
        lstr.add(" -g");
        if (GRX->GetStatus(cmp_p2_recurse))
            lstr.add(" -h");
        if (GRX->GetStatus(cmp_p2_expand_arrays))
            lstr.add(" -x");
        if (GRX->GetStatus(cmp_p2_insta))
            lstr.add(" -t c");
    }
    else if (page == 2) {
        lstr.add(" -f");
        if (GRX->GetStatus(cmp_p3_aoi_use)) {
            s = sb_p3_aoi_left.get_string();
            char *tokl = lstring::gettok(&s);
            s = sb_p3_aoi_bottom.get_string();
            char *tokb = lstring::gettok(&s);
            s = sb_p3_aoi_right.get_string();
            char *tokr = lstring::gettok(&s);
            s = sb_p3_aoi_top.get_string();
            char *tokt = lstring::gettok(&s);
            if (tokl && tokb && tokr && tokt) {
                lstr.add(" -a ");
                lstr.add(tokl);
                lstr.add_c(',');
                lstr.add(tokb);
                lstr.add_c(',');
                lstr.add(tokr);
                lstr.add_c(',');
                lstr.add(tokt);
            }
            delete [] tokl;
            delete [] tokb;
            delete [] tokr;
            delete [] tokt;
        }

        s = sb_p3_fine_grid.get_string();
        tok = strip_sp(s);
        if (tok) {
            lstr.add(" -i ");
            lstr.add(tok);
            delete [] tok;
        }

        s = sb_p3_coarse_mult.get_string();
        tok = strip_sp(s);
        if (tok) {
            lstr.add(" -m ");
            lstr.add(tok);
            delete [] tok;
        }
    }
    return (lstr.string_trim());
}


bool
sCmp::get_bb(BBox *BB)
{
    if (!BB)
        return (false);
    double l, b, r, t;
    int i = 0;
    i += sscanf(sb_p3_aoi_left.get_string(), "%lf", &l);
    i += sscanf(sb_p3_aoi_bottom.get_string(), "%lf", &b);
    i += sscanf(sb_p3_aoi_right.get_string(), "%lf", &r);
    i += sscanf(sb_p3_aoi_top.get_string(), "%lf", &t);
    if (i != 4)
        return (false);
    BB->left = INTERNAL_UNITS(l);
    BB->bottom = INTERNAL_UNITS(b);
    BB->right = INTERNAL_UNITS(r);
    BB->top = INTERNAL_UNITS(t);
    if (BB->right < BB->left) {
        int tmp = BB->left;
        BB->left = BB->right;
        BB->right = tmp;
    }
    if (BB->top < BB->bottom) {
        int tmp = BB->bottom;
        BB->bottom = BB->top;
        BB->top = tmp;
    }
    if (BB->left == BB->right || BB->bottom == BB->top)
        return (false);;
    return (true);
}


void
sCmp::set_bb(const BBox *BB)
{
    int ndgt = CD()->numDigits();
    if (!BB)
        return;
    sb_p3_aoi_left.set_digits(ndgt);
    sb_p3_aoi_left.set_value(MICRONS(BB->left));
    sb_p3_aoi_bottom.set_digits(ndgt);
    sb_p3_aoi_bottom.set_value(MICRONS(BB->bottom));
    sb_p3_aoi_right.set_digits(ndgt);
    sb_p3_aoi_right.set_value(MICRONS(BB->right));
    sb_p3_aoi_top.set_digits(ndgt);
    sb_p3_aoi_top.set_value(MICRONS(BB->top));
}


// Static function.
void
sCmp::cmp_cancel_proc(GtkWidget*, void*)
{
    Cvt()->PopUpCompare(0, MODE_OFF);
}


// Static function.
void
sCmp::cmp_action(GtkWidget *caller, void*)
{
    if (!Cmp)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:diff"))
        return;
    }
    if (!strcmp(name, "Go")) {
        // If the prompt to view output is still active, kill it.
        PL()->AbortEdit();

        char *str = Cmp->compose_arglist();

        cCompare cmp;
        if (!cmp.parse(str)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            delete [] str;
            return;
        }
        if (!cmp.setup()) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            delete [] str;
            return;
        }
        dspPkgIf()->SetWorking(true);
        DFtype df = cmp.compare();
        dspPkgIf()->SetWorking(false);

        if (df == DFabort)
            PL()->ShowPrompt("Comparison aborted.");
        else if (df == DFerror)
            PL()->ShowPromptV("Comparison failed: %s", Errs()->get_error());
        else {
            char buf[256];
            if (df == DFsame) {
                sprintf(buf, "No differences found, see file \"%s\".",
                    DIFF_LOG_FILE);
                PL()->ShowPrompt(buf);
            }
            else {
                sprintf(buf, "Differences found, data written to "
                    "file \"%s\", view file? [y] ", DIFF_LOG_FILE);
                char *in = PL()->EditPrompt(buf, "y");
                in = lstring::strip_space(in);
                if (in && (*in == 'y' || *in == 'Y'))
                    DSPmainWbag(PopUpFileBrowser(DIFF_LOG_FILE))
                PL()->ErasePrompt();
            }
        }
        delete [] str;
        return;
    }
    if (caller == Cmp->cmp_layer_use) {
        if (GRX->GetStatus(caller))
            GRX->SetStatus(Cmp->cmp_layer_skip, false);

        if (GRX->GetStatus(Cmp->cmp_layer_use) ||
                GRX->GetStatus(Cmp->cmp_layer_skip))
            gtk_widget_set_sensitive(Cmp->cmp_layer_list, true);
        else
            gtk_widget_set_sensitive(Cmp->cmp_layer_list, false);
        return;
    }
    if (caller == Cmp->cmp_layer_skip) {
        if (GRX->GetStatus(caller))
            GRX->SetStatus(Cmp->cmp_layer_use, false);
        if (GRX->GetStatus(Cmp->cmp_layer_use) ||
                GRX->GetStatus(Cmp->cmp_layer_skip))
            gtk_widget_set_sensitive(Cmp->cmp_layer_list, true);
        else
            gtk_widget_set_sensitive(Cmp->cmp_layer_list, false);
        return;
    }
    if (caller == Cmp->cmp_p3_aoi_use) {
        if (GRX->GetStatus(caller)) {
            Cmp->sb_p3_aoi_left.set_sensitive(true);
            Cmp->sb_p3_aoi_bottom.set_sensitive(true);
            Cmp->sb_p3_aoi_right.set_sensitive(true);
            Cmp->sb_p3_aoi_top.set_sensitive(true);
        }
        else {
            Cmp->sb_p3_aoi_left.set_sensitive(false);
            Cmp->sb_p3_aoi_bottom.set_sensitive(false);
            Cmp->sb_p3_aoi_right.set_sensitive(false);
            Cmp->sb_p3_aoi_top.set_sensitive(false);
        }
    }
}


// Static function.
void
sCmp::cmp_p1_action(GtkWidget *caller, void *arg)
{
    if (Cmp) {
        if (arg == (void*)1L) {
            if (GRX->GetStatus(caller))
                Cvt()->PopUpPropertyFilter(caller, MODE_ON);
            else
                Cvt()->PopUpPropertyFilter(caller, MODE_OFF);
            return;
        }
        Cmp->p1_sens();
    }
}


// Static function.
void
sCmp::cmp_p1_fltr_proc(GtkWidget *caller, void*)
{
    if (Cmp) {
        int n = gtk_combo_box_get_active(GTK_COMBO_BOX(caller));
        if (n == 1)
            Cmp->cmp_p1_fltr_mode = PrpFltNone;
        else if (n == 2)
            Cmp->cmp_p1_fltr_mode = PrpFltCstm;
        else
            Cmp->cmp_p1_fltr_mode = PrpFltDflt;
    }
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
sCmp::cmp_popup_btn_proc(GtkWidget *widget, GdkEvent *event, void *arg)
{
    gtk_menu_popup(GTK_MENU(arg), 0, 0, pos_func, widget, event->button.button,
        event->button.time);
    return (true);
}


// Static function.
void
sCmp::cmp_sto_menu_proc(GtkWidget *widget, void *arg)
{
    sCmp *cmp = (sCmp*)arg;
    const char *name = gtk_widget_get_name(widget);
    if (name && cmp) {
        while (*name) {
            if (isdigit(*name))
                break;
            name++;
        }
        if (isdigit(*name))
            cmp->get_bb(FIO()->savedBB(*name - '0'));
    }
}


// Static function.
void
sCmp::cmp_rcl_menu_proc(GtkWidget *widget, void *arg)
{
    sCmp *cmp = (sCmp*)arg;
    const char *name = gtk_widget_get_name(widget);
    if (name && cmp) {
        while (*name) {
            if (isdigit(*name))
                break;
            name++;
        }
        if (isdigit(*name))
            cmp->set_bb(FIO()->savedBB(*name - '0'));
    }
}


// Private static GTK signal handler.
// Drag data received in entry area, deal accordingly.
//
void
sCmp::cmp_drag_data_received(GtkWidget *entry,
    GdkDragContext *context, gint, gint, GtkSelectionData *data,
    guint, guint time)
{
    if (Cmp && gtk_selection_data_get_length(data) >= 0 &&
            gtk_selection_data_get_format(data) == 8 &&
            gtk_selection_data_get_data(data)) {
        char *src = (char*)gtk_selection_data_get_data(data);
        char *t = 0;
        if (gtk_selection_data_get_target(data) ==
                gdk_atom_intern("TWOSTRING", true)) {
            // Drops from content lists may be in the form
            // "fname_or_chd\ncellname".
            t = strchr(src, '\n');
        }
        if (entry == Cmp->cmp_fname1 || entry == Cmp->cmp_fname2) {
            if (t)
                *t = 0;
            gtk_entry_set_text(GTK_ENTRY(entry), src);
        }
        else if (entry == Cmp->cmp_cnames1 || entry == Cmp->cmp_cnames2) {
            if (t)
                src = t+1;
            // Add new cell name to end of existing text.
            sLstr lstr;
            lstr.add(gtk_entry_get_text(GTK_ENTRY(entry)));
            if (lstr.string() && *lstr.string())
                lstr.add_c(' ');
            lstr.add(src);
            gtk_entry_set_text(GTK_ENTRY(entry), lstr.string());
        }
        gtk_drag_finish(context, true, false, time);

        // The concatenation behavior above is unique, and opened a
        // can of worms in GTK-2.  The problem is that the GtkEntry
        // will call its own data-received handler before calling
        // this one, or call this one twice, if this handler is
        // connect-after.  Various changes were made to fight this: 
        //
        // 1.  Don't set GTK_DEST_DEFAULT_DROP.  This avoids a
        // double call of this handler for targets not handled by
        // the internal handler.  The internal handler takes only
        // the plain text targets.
        //
        // 2.  Use local targets preferentially by putting such
        // targets as TWOSTRING and CELLNAME at the beginning of
        // the targets lists.
        //
        // 3.  Still, a plain text drop would cause trouble.  This
        // was finally resolved by not using connect-after, and
        // adding the emit_stop call below.
        //
        g_signal_stop_emission_by_name(G_OBJECT(entry),
            "drag-data-received");
        return;
    }
    gtk_drag_finish(context, false, false, time);
}
// End of sCmp functions.


sCmp_store::sCmp_store()
{
    cs_file1 = 0;
    cs_file2 = 0;
    cs_cells1 = 0;
    cs_cells2 = 0;
    cs_layers = 0;
    cs_mode = 0;
    cs_layer_only = false;
    cs_layer_skip = false;
    cs_differ = false;
    cs_max_errs = 0;

    cs_p1_recurse = false;
    cs_p1_expand_arrays = false;
    cs_p1_slop = false;
    cs_p1_dups = false;
    cs_p1_boxes = true;
    cs_p1_polys = true;
    cs_p1_wires = true;
    cs_p1_labels = false;
    cs_p1_insta = true;
    cs_p1_boxes_prp = false;
    cs_p1_polys_prp = false;
    cs_p1_wires_prp = false;
    cs_p1_labels_prp = false;
    cs_p1_insta_prp = false;
    cs_p1_elec = false;
    cs_p1_cell_prp = false;
    cs_p1_fltr = 0;

    cs_p2_recurse = false;
    cs_p2_expand_arrays = false;
    cs_p2_insta = false;

    cs_p3_use_window = false;
    cs_p3_left = 0.0;
    cs_p3_bottom = 0.0;
    cs_p3_right = 0.0;
    cs_p3_top = 0.0;
    cs_p3_fine_grid = 20;
    cs_p3_coarse_mult = 20;
}


sCmp_store::~sCmp_store()
{
    delete [] cs_file1;
    delete [] cs_file2;
    delete [] cs_cells1;
    delete [] cs_cells2;
    delete [] cs_layers;
}


void
sCmp_store::save()
{
    if (!Cmp)
        return;
    delete [] cs_file1;
    cs_file1 = lstring::copy(gtk_entry_get_text(GTK_ENTRY(Cmp->cmp_fname1)));
    delete [] cs_file2;
    cs_file2 = lstring::copy(gtk_entry_get_text(GTK_ENTRY(Cmp->cmp_fname2)));
    delete [] cs_cells1;
    cs_cells1 = lstring::copy(gtk_entry_get_text(GTK_ENTRY(Cmp->cmp_cnames1)));
    delete [] cs_cells2;
    cs_cells2 = lstring::copy(gtk_entry_get_text(GTK_ENTRY(Cmp->cmp_cnames2)));
    delete [] cs_layers;
    cs_layers = lstring::copy(
        gtk_entry_get_text(GTK_ENTRY(Cmp->cmp_layer_list)));
    cs_mode = gtk_notebook_get_current_page(GTK_NOTEBOOK(Cmp->cmp_mode));
    cs_layer_only = GRX->GetStatus(Cmp->cmp_layer_use);
    cs_layer_skip = GRX->GetStatus(Cmp->cmp_layer_skip);
    cs_differ = GRX->GetStatus(Cmp->cmp_diff_only);
    cs_max_errs = Cmp->sb_max_errs.get_value_as_int();

    if (Cmp->cmp_p1_recurse) {
        cs_p1_recurse = GRX->GetStatus(Cmp->cmp_p1_recurse);
        cs_p1_expand_arrays = GRX->GetStatus(Cmp->cmp_p1_expand_arrays);
        cs_p1_slop = GRX->GetStatus(Cmp->cmp_p1_slop);
        cs_p1_dups = GRX->GetStatus(Cmp->cmp_p1_dups);
        cs_p1_boxes = GRX->GetStatus(Cmp->cmp_p1_boxes);
        cs_p1_polys = GRX->GetStatus(Cmp->cmp_p1_polys);
        cs_p1_wires = GRX->GetStatus(Cmp->cmp_p1_wires);
        cs_p1_labels = GRX->GetStatus(Cmp->cmp_p1_labels);
        cs_p1_insta = GRX->GetStatus(Cmp->cmp_p1_insta);
        cs_p1_boxes_prp = GRX->GetStatus(Cmp->cmp_p1_boxes_prp);
        cs_p1_polys_prp = GRX->GetStatus(Cmp->cmp_p1_polys_prp);
        cs_p1_wires_prp = GRX->GetStatus(Cmp->cmp_p1_wires_prp);
        cs_p1_labels_prp = GRX->GetStatus(Cmp->cmp_p1_labels_prp);
        cs_p1_insta_prp = GRX->GetStatus(Cmp->cmp_p1_insta_prp);
        cs_p1_elec = GRX->GetStatus(Cmp->cmp_p1_elec);
        cs_p1_cell_prp = GRX->GetStatus(Cmp->cmp_p1_cell_prp);
        cs_p1_fltr = Cmp->cmp_p1_fltr_mode;
    }

    if (Cmp->cmp_p2_recurse) {
        cs_p2_recurse = GRX->GetStatus(Cmp->cmp_p2_recurse);
        cs_p2_expand_arrays = GRX->GetStatus(Cmp->cmp_p2_expand_arrays);
        cs_p2_insta = GRX->GetStatus(Cmp->cmp_p2_insta);
    }

    if (Cmp->cmp_p3_aoi_use) {
        cs_p3_use_window = GRX->GetStatus(Cmp->cmp_p3_aoi_use);
        cs_p3_left = Cmp->sb_p3_aoi_left.get_value();
        cs_p3_bottom = Cmp->sb_p3_aoi_bottom.get_value();
        cs_p3_right = Cmp->sb_p3_aoi_right.get_value();
        cs_p3_top = Cmp->sb_p3_aoi_top.get_value();
        cs_p3_fine_grid = Cmp->sb_p3_fine_grid.get_value_as_int();
        cs_p3_coarse_mult = Cmp->sb_p3_coarse_mult.get_value_as_int();
    }
}


void
sCmp_store::recall()
{
    if (!Cmp)
        return;
    gtk_entry_set_text(GTK_ENTRY(Cmp->cmp_fname1),
        cs_file1 ? cs_file1 : "");
    gtk_entry_set_text(GTK_ENTRY(Cmp->cmp_fname2),
        cs_file2 ? cs_file2 : "");
    gtk_entry_set_text(GTK_ENTRY(Cmp->cmp_cnames1),
        cs_cells1 ? cs_cells1 : "");
    gtk_entry_set_text(GTK_ENTRY(Cmp->cmp_cnames2),
        cs_cells2 ? cs_cells2 : "");
    gtk_entry_set_text(GTK_ENTRY(Cmp->cmp_layer_list),
        cs_layers ? cs_layers : "");
    gtk_notebook_set_current_page(GTK_NOTEBOOK(Cmp->cmp_mode), cs_mode);
    GRX->SetStatus(Cmp->cmp_layer_use, cs_layer_only);
    GRX->SetStatus(Cmp->cmp_layer_skip, cs_layer_skip);
    GRX->SetStatus(Cmp->cmp_diff_only, cs_differ);
    Cmp->sb_max_errs.set_value(cs_max_errs);
    gtk_widget_set_sensitive(Cmp->cmp_layer_list,
        cs_layer_only || cs_layer_skip);
}


void
sCmp_store::recall_p1()
{
    if (!Cmp)
        return;
    if (Cmp->cmp_p1_recurse) {
        GRX->SetStatus(Cmp->cmp_p1_recurse, cs_p1_recurse);
        GRX->SetStatus(Cmp->cmp_p1_expand_arrays, cs_p1_expand_arrays);
        GRX->SetStatus(Cmp->cmp_p1_slop, cs_p1_slop);
        GRX->SetStatus(Cmp->cmp_p1_dups, cs_p1_dups);
        GRX->SetStatus(Cmp->cmp_p1_boxes, cs_p1_boxes);
        GRX->SetStatus(Cmp->cmp_p1_polys, cs_p1_polys);
        GRX->SetStatus(Cmp->cmp_p1_wires, cs_p1_wires);
        GRX->SetStatus(Cmp->cmp_p1_labels, cs_p1_labels);
        GRX->SetStatus(Cmp->cmp_p1_insta, cs_p1_insta);
        GRX->SetStatus(Cmp->cmp_p1_boxes_prp, cs_p1_boxes_prp);
        GRX->SetStatus(Cmp->cmp_p1_polys_prp, cs_p1_polys_prp);
        GRX->SetStatus(Cmp->cmp_p1_wires_prp, cs_p1_wires_prp);
        GRX->SetStatus(Cmp->cmp_p1_labels_prp, cs_p1_labels_prp);
        GRX->SetStatus(Cmp->cmp_p1_insta_prp, cs_p1_insta_prp);
        GRX->SetStatus(Cmp->cmp_p1_phys, !cs_p1_elec);
        GRX->SetStatus(Cmp->cmp_p1_elec, cs_p1_elec);
        GRX->SetStatus(Cmp->cmp_p1_cell_prp, cs_p1_cell_prp);
        gtk_combo_box_set_active(GTK_COMBO_BOX(Cmp->cmp_p1_fltr),
            cs_p1_fltr);
    }
}


void
sCmp_store::recall_p2()
{
    if (!Cmp)
        return;
    if (Cmp->cmp_p2_recurse) {
        GRX->SetStatus(Cmp->cmp_p2_recurse, cs_p2_recurse);
        GRX->SetStatus(Cmp->cmp_p2_expand_arrays, cs_p2_expand_arrays);
        GRX->SetStatus(Cmp->cmp_p2_insta, cs_p2_insta);
    }
}


void
sCmp_store::recall_p3()
{
    if (!Cmp)
        return;
    if (Cmp->cmp_p3_aoi_use) {
        GRX->SetStatus(Cmp->cmp_p3_aoi_use, cs_p3_use_window);
        Cmp->sb_p3_aoi_left.set_value(cs_p3_left);
        Cmp->sb_p3_aoi_bottom.set_value(cs_p3_bottom);
        Cmp->sb_p3_aoi_right.set_value(cs_p3_right);
        Cmp->sb_p3_aoi_top.set_value(cs_p3_top);
        Cmp->sb_p3_fine_grid.set_value(cs_p3_fine_grid);
        Cmp->sb_p3_coarse_mult.set_value(cs_p3_coarse_mult);
        Cmp->sb_p3_aoi_left.set_sensitive(cs_p3_use_window);
        Cmp->sb_p3_aoi_bottom.set_sensitive(cs_p3_use_window);
        Cmp->sb_p3_aoi_right.set_sensitive(cs_p3_use_window);
        Cmp->sb_p3_aoi_top.set_sensitive(cs_p3_use_window);
    }
}
// End of sCmp_store functions.

