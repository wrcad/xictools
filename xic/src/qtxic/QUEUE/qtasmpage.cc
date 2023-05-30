
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
#include "cvrt.h"
#include "fio.h"
#include "fio_alias.h"
#include "menu.h"
#include "gtkmain.h"
#include "gtkasm.h"


//-----------------------------------------------------------------------------
// The cAsmPage (notebook page) class

cAsmPage::cAsmPage(cAsm *mt)
{
    pg_owner = mt;
    pg_numtlcells = 0;
    pg_curtlcell = -1;
    pg_infosize = 20;
    pg_cellinfo = new tlinfo*[pg_infosize];
    for (unsigned int i = 0; i < pg_infosize; i++)
        pg_cellinfo[i] = 0;

    pg_tx = 0;
    pg_tablabel = 0;
    pg_form = gtk_table_new(3, 1, false);
    pg_no_select = false;
    gtk_widget_show(pg_form);
    int row = 0;

    GtkWidget *table = gtk_table_new(3, 1, false);
    gtk_widget_show(table);

    //
    // path to source
    //
    GtkWidget *label = gtk_label_new(cAsm::path_to_source_string);
    gtk_widget_show(label);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

    gtk_table_attach(GTK_TABLE(table), label, 0, 3, row, row + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    pg_path = gtk_entry_new();
    gtk_widget_show(pg_path);

    // drop site
    GtkDestDefaults DD = (GtkDestDefaults)
        (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT);
    gtk_drag_dest_set(pg_path, DD, cAsm::target_table, cAsm::n_targets,
        GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(pg_path), "drag-data-received",
        G_CALLBACK(cAsm::asm_drag_data_received), 0);

    gtk_table_attach(GTK_TABLE(table), pg_path, 0, 3, row, row + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    //
    // scale factor and cell name aliasing
    //
    label = gtk_label_new("Conversion Scale Factor");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *sb = sb_scale.init(1.0, CDSCALEMIN, CDSCALEMAX, ASM_NUMD);
    gtk_widget_set_size_request(sb, ASM_NFW, -1);

    gtk_table_attach(GTK_TABLE(table), sb, 0, 1, row+1, row+2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    label = gtk_label_new("Cell Name Change");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(table), label, 1, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);
    label = gtk_label_new("Prefix");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 2);
    pg_prefix_lab = label;

    pg_prefix = gtk_entry_new();
    gtk_widget_show(pg_prefix);
    gtk_widget_set_size_request(pg_prefix, 60, -1);
    gtk_box_pack_start(GTK_BOX(hbox), pg_prefix, false, false, 2);

    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, row+1, row+2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);
    label = gtk_label_new("Suffix");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 2);
    pg_suffix_lab = label;

    pg_suffix = gtk_entry_new();
    gtk_widget_show(pg_suffix);
    gtk_widget_set_size_request(pg_suffix, 60, -1);
    gtk_box_pack_start(GTK_BOX(hbox), pg_suffix, false, false, 2);

    pg_to_lower = gtk_check_button_new_with_label("To Lower");
    gtk_widget_show(pg_to_lower);
    gtk_box_pack_start(GTK_BOX(hbox), pg_to_lower, false, false, 2);

    pg_to_upper = gtk_check_button_new_with_label("To Upper");
    gtk_widget_show(pg_to_upper);
    gtk_box_pack_start(GTK_BOX(hbox), pg_to_upper, false, false, 2);

    gtk_table_attach(GTK_TABLE(table), hbox, 2, 3, row+1, row+3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row += 2;

    //
    // frame around all above
    //
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), table);

    row = 0;
    gtk_table_attach(GTK_TABLE(pg_form), frame, 0, 3, row, row + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    GtkWidget *hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);
    gtk_table_attach(GTK_TABLE(pg_form), hsep, 0, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    //
    // layer list and associated
    //
    label = gtk_label_new("Layer List");
    gtk_widget_show(label);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

    gtk_table_attach(GTK_TABLE(pg_form), label, 0, 1, row, row + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    pg_layers_only = gtk_check_button_new_with_label("Layers Only");
    gtk_widget_set_name(pg_layers_only, "luse");
    gtk_widget_show(pg_layers_only);

    gtk_table_attach(GTK_TABLE(pg_form), pg_layers_only, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    pg_skip_layers = gtk_check_button_new_with_label("Skip Layers");
    gtk_widget_set_name(pg_skip_layers, "lskip");
    gtk_widget_show(pg_skip_layers);

    gtk_table_attach(GTK_TABLE(pg_form), pg_skip_layers, 2, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    pg_layer_list = gtk_entry_new();
    gtk_widget_show(pg_layer_list);

    gtk_table_attach(GTK_TABLE(pg_form), pg_layer_list, 0, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    label = gtk_label_new("Layer Aliases");
    gtk_widget_show(label);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

    gtk_table_attach(GTK_TABLE(pg_form), label, 0, 1, row, row + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    pg_layer_aliases = gtk_entry_new();
    gtk_widget_show(pg_layer_aliases);

    gtk_table_attach(GTK_TABLE(pg_form), pg_layer_aliases, 0, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);
    gtk_table_attach(GTK_TABLE(pg_form), hsep, 0, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    //
    // top-level cells list and transform entry
    //
    table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);
    gtk_table_attach(GTK_TABLE(pg_form), table, 0, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 2, 2);

    GtkWidget *vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);
    label = gtk_label_new("Top-Level Cells");
    gtk_box_pack_start(GTK_BOX(vbox), label, false, false, 2);
    gtk_widget_show(label);

    GtkWidget *swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
    pg_toplevels = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_show(pg_toplevels);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(pg_toplevels), false);
    GtkTreeViewColumn *tvcol = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(pg_toplevels), tvcol);
    GtkCellRenderer *rnd = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(tvcol, rnd, true);
    gtk_tree_view_column_add_attribute(tvcol, rnd, "text", 0);

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(pg_toplevels));
    gtk_tree_selection_set_select_function(sel, pg_selection_proc, this, 0);
    // TreeView bug hack, see note with handlers.   
    g_signal_connect(G_OBJECT(pg_toplevels), "focus",
        G_CALLBACK(pg_focus_proc), this);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin),
        pg_toplevels);
    gtk_box_pack_start(GTK_BOX(vbox), swin, true, true, 2);
    gtk_table_attach(GTK_TABLE(table), vbox, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 2, 2);

    // drop site
    gtk_drag_dest_set(pg_toplevels, GTK_DEST_DEFAULT_ALL,
        cAsm::target_table, cAsm::n_targets, GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(pg_toplevels), "drag-data-received",
        G_CALLBACK(pg_drag_data_received), this);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    pg_tx = new cAsmTf(this);
    gtk_container_add(GTK_CONTAINER(frame), pg_tx->shell());

    gtk_table_attach(GTK_TABLE(table), frame, 1, 2, 0, 1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)0, 2, 2);
    row++;

    upd_sens();
}


cAsmPage::~cAsmPage()
{
    delete pg_tx;
    for (unsigned int i = 0; i < pg_numtlcells; i++)
        delete pg_cellinfo[i];
    delete [] pg_cellinfo;
}


void
cAsmPage::upd_sens()
{
    const char *topc = pg_owner->top_level_cell();
    pg_tx->set_sens((pg_curtlcell >= 0), (topc && *topc));
}


void
cAsmPage::reset()
{
    gtk_entry_set_text(GTK_ENTRY(pg_path), "");
    GTKdev::Deselect(pg_layers_only);
    GTKdev::Deselect(pg_skip_layers);
    gtk_entry_set_text(GTK_ENTRY(pg_layer_list), "");
    gtk_entry_set_text(GTK_ENTRY(pg_layer_aliases), "");
    sb_scale.set_value(1.0);
    gtk_entry_set_text(GTK_ENTRY(pg_prefix), "");
    gtk_entry_set_text(GTK_ENTRY(pg_suffix), "");

    gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(
        GTK_TREE_VIEW(pg_toplevels))));

    for (unsigned int j = 0; j < pg_numtlcells; j++) {
        delete pg_cellinfo[j];
        pg_cellinfo[j] = 0;
    }
    pg_numtlcells = 0;
    pg_curtlcell = -1;
    pg_tx->reset();
    upd_sens();
}


// Append a new instance to the "top level" cell list.
//
tlinfo *
cAsmPage::add_instance(const char *cname)
{
    pg_owner->store_tx_params();
    if (pg_numtlcells >= pg_infosize) {
        tlinfo **tmp = new tlinfo*[pg_infosize + pg_infosize];
        unsigned int i;
        for (i = 0; i < pg_infosize; i++)
            tmp[i] = pg_cellinfo[i];
        pg_infosize += pg_infosize;
        for ( ; i < pg_infosize; i++)
            tmp[i] = 0;
        delete [] pg_cellinfo;
        pg_cellinfo = tmp;
    }
    tlinfo *tl = new tlinfo(cname);
    pg_cellinfo[pg_numtlcells] = tl;
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(
        GTK_TREE_VIEW(pg_toplevels)));
    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, cname ? cname : ASM_TOPC, -1);
    pg_numtlcells++;
    pg_curtlcell = -1;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(
        GTK_TREE_VIEW(pg_toplevels));
    gtk_tree_selection_select_iter(sel, &iter);
    upd_sens();
    return (tl);
}


//
// Handler functions
//

// Static function.
// Handle selection change in the "top-level" cell list.
//
int
cAsmPage::pg_selection_proc(GtkTreeSelection*, GtkTreeModel*,
    GtkTreePath *tp, int issel, void *srcp)
{
    cAsmPage *src = static_cast<cAsmPage*>(srcp);
    if (src->pg_no_select && !issel)
        return (false);
    if (!issel) {
        src->pg_owner->store_tx_params();
        int n = gtk_tree_path_get_indices(tp)[0];
        src->pg_owner->show_tx_params(n);
    }
    else {
        src->pg_curtlcell = -1;
        src->pg_tx->reset();
        src->upd_sens();
    }
    return (true);
}


// Static function.
// This handler is a hack to avoid a GtkTreeWidget defect:  when focus
// is taken and there are no selections, the 0'th row will be
// selected.  There seems to be no way to avoid this other than a hack
// like this one.  We set a flag to lock out selection changes in this
// case.
//
int
cAsmPage::pg_focus_proc(GtkWidget*, GdkEvent*, void *srcp)
{
    cAsmPage *src = static_cast<cAsmPage*>(srcp);
    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(src->pg_toplevels));
    // If nothing selected set the flag.
    if (!gtk_tree_selection_get_selected(sel, 0, 0))
        src->pg_no_select = true;
    return (false);
}


// Static function.
// Drag data received in entry window, grab it.
//
void
cAsmPage::pg_drag_data_received(GtkWidget*, GdkDragContext *context,
    gint, gint, GtkSelectionData *data, guint, guint time, void *srcp)
{
    cAsmPage *src = static_cast<cAsmPage*>(srcp);
    if (gtk_selection_data_get_length(data) >= 0 &&
            gtk_selection_data_get_format(data) == 8 &&
            gtk_selection_data_get_data(data)) {
        char *s = (char*)gtk_selection_data_get_data(data);
        if (gtk_selection_data_get_target(data) ==
                gdk_atom_intern("TWOSTRING", true)) {
            // Drops from content lists may be in the form
            // "fname_or_chd\ncellname".  Keep the cellname.
            char *t = strchr(s, '\n');
            if (t)
                s = t+1;
        }
        src->add_instance(s);
        gtk_drag_finish(context, true, false, time);
        return;
    }
    gtk_drag_finish(context, false, false, time);
}

