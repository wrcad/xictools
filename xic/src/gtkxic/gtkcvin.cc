
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
#include "dsp_inlines.h"
#include "oa_if.h"
#include "gtkmain.h"
#include "gtkcv.h"


//--------------------------------------------------------------------
// Pop-up to set input parameters and read cell files.
//
// Help system keywords used:
//  xic:imprt

namespace {
    namespace gtkcvin {
        struct sCvi
        {
            sCvi(GRobject, bool(*)(int, void*), void*);
            ~sCvi();

            void update();

            GtkWidget *shell() { return (cvi_popup); }

        private:
            static void cvi_cancel_proc(GtkWidget*, void*);
            static void cvi_page_chg_proc(GtkWidget*, void*, int, void*);
            static void cvi_action(GtkWidget*, void*);
            static void cvi_over_menu_proc(GtkWidget*, void*);
            static void cvi_dup_menu_proc(GtkWidget*, void*);
            static void cvi_force_menu_proc(GtkWidget*, void*);
            static void cvi_merg_menu_proc(GtkWidget*, void*);
            static void cvi_val_changed(GtkWidget*, void*);
            static WndSensMode wnd_sens_test();

            GRobject cvi_caller;
            GtkWidget *cvi_popup;
            GtkWidget *cvi_label;
            GtkWidget *cvi_nbook;
            GtkWidget *cvi_nonpc;
            GtkWidget *cvi_yesoapc;
            GtkWidget *cvi_nolyr;
            GtkWidget *cvi_replace;
            GtkWidget *cvi_over;
            GtkWidget *cvi_merge;
            GtkWidget *cvi_polys;
            GtkWidget *cvi_dup;
            GtkWidget *cvi_empties;
            GtkWidget *cvi_dtypes;
            GtkWidget *cvi_force;
            GtkWidget *cvi_noflvias;
            GtkWidget *cvi_noflpcs;
            GtkWidget *cvi_nofllbs;
            GtkWidget *cvi_nolabels;
            GtkWidget *cvi_merg;
            bool (*cvi_callback)(int, void*);
            void *cvi_arg;
            cnmap_t *cvi_cnmap;
            llist_t *cvi_llist;
            wnd_t *cvi_wnd;
            GTKspinBtn sb_scale;

            static int cvi_merg_val;
        };

        sCvi *Cvi;
    }

    const char *mergvals[] =
    {
        "No Merge Into Current",
        "Merge Cell Into Current",
        "Merge Into Current Recursively",
        0
    };

    const char *overvals[] =
    {
        "Overwrite All",
        "Overwrite Phys",
        "Overwrite Elec",
        "Overwrite None",
        "Auto-Rename",
        0
    };

    const char *dupvals[] =
    {
        "No Checking for Duplicates",
        "Put Warning in Log File",
        "Remove Duplicates",
        0
    };

    fmt_menu forcevals[] =
    {
        { "Try Both",   EXTlreadDef },
        { "By Name",    EXTlreadName },
        { "By Index",   EXTlreadIndex },
        { 0,            0 }
    };
}

using namespace gtkcvin;

int sCvi::cvi_merg_val = 0;


void
cConvert::PopUpImport(GRobject caller, ShowMode mode,
    bool (*callback)(int, void*), void *arg)
{
    if (!GTKdev::exists() || !GTKmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete Cvi;
        return;
    }
    if (mode == MODE_UPD) {
        if (Cvi)
            Cvi->update();
        return;
    }
    if (Cvi)
        return;

    new sCvi(caller, callback, arg);
    if (!Cvi->shell()) {
        delete Cvi;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Cvi->shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GTKdev::self()->SetPopupLocation(GRloc(LW_UL), Cvi->shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(Cvi->shell());
}


sCvi::sCvi(GRobject c, bool (*callback)(int, void*), void *arg)
{
    Cvi = this;
    cvi_caller = c;
    cvi_popup = 0;
    cvi_label = 0;
    cvi_nbook = 0;
    cvi_nonpc = 0;
    cvi_yesoapc = 0;
    cvi_nolyr = 0;
    cvi_replace = 0;
    cvi_over = 0;
    cvi_merge = 0;
    cvi_polys = 0;
    cvi_dup = 0;
    cvi_empties = 0;
    cvi_dtypes = 0;
    cvi_force = 0;
    cvi_noflvias = 0;
    cvi_noflpcs = 0;
    cvi_nofllbs = 0;
    cvi_nolabels = 0;
    cvi_merg = 0;
    cvi_callback = callback;
    cvi_arg = arg;
    cvi_cnmap = 0;
    cvi_llist = 0;
    cvi_wnd = 0;

    // Dangerous to leave this in effect, force user to turn in on
    // when needed.
    FIO()->SetInFlatten(false);

    cvi_popup = gtk_NewPopup(0, "Import Control", cvi_cancel_proc, 0);
    if (!cvi_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(cvi_popup), false);

    GtkWidget *topform = gtk_table_new(2, 1, false);
    gtk_widget_show(topform);
    gtk_container_set_border_width(GTK_CONTAINER(topform), 2);
    gtk_container_add(GTK_CONTAINER(cvi_popup), topform);
    int toprcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new(
        "Set parameters for reading input");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    cvi_label = label;
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(row), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cvi_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(topform), row, 0, 2, toprcnt, toprcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    toprcnt++;

    cvi_nbook = gtk_notebook_new();
    gtk_widget_show(cvi_nbook);
    g_signal_connect(G_OBJECT(cvi_nbook), "switch-page",
        G_CALLBACK(cvi_page_chg_proc), 0);
    gtk_table_attach(GTK_TABLE(topform), cvi_nbook, 0, 2, toprcnt, toprcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    toprcnt++;

    //
    // The Setup page
    //
    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    int rowcnt = 0;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    label = gtk_label_new(" PCell evaluation:");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);
    button = gtk_check_button_new_with_label("Don't eval native");
    gtk_widget_set_name(button, "nonpc");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cvi_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    cvi_nonpc = button;

    if (OAif()->hasOA()) {
        button = gtk_check_button_new_with_label("Eval OpenAccess");
        gtk_widget_set_name(button, "yesoapc");
        gtk_widget_show(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(cvi_action), 0);
        gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
        cvi_yesoapc = button;
    }

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label(
        "Don't create new layers when reading, abort instead");
    gtk_widget_set_name(button, "nolyr");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cvi_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvi_nolyr = button;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *entry = gtk_combo_box_text_new();
    gtk_widget_set_name(entry, "overmenu");
    gtk_widget_show(entry);
    for (int i = 0; overvals[i]; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry),
            overvals[i]);
    }
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(cvi_over_menu_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), entry, true, true, 0);
    cvi_over = entry;

    label = gtk_label_new("Default when new cells conflict");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label(
        "Don't prompt for overwrite instructions");
    gtk_widget_set_name(button, "replace");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cvi_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvi_replace = button;

    button = gtk_check_button_new_with_label(
        "Clip and merge overlapping boxes");
    gtk_widget_set_name(button, "merge");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cvi_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvi_merge = button;

    button = gtk_check_button_new_with_label(
        "Skip testing for badly formed polygons");
    gtk_widget_set_name(button, "polys");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cvi_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvi_polys = button;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    entry = gtk_combo_box_text_new();
    gtk_widget_set_name(entry, "dupmenu");
    gtk_widget_show(entry);
    for (int i = 0; dupvals[i]; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry),
            dupvals[i]);
    }
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(cvi_dup_menu_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), entry, true, true, 0);
    cvi_dup = entry;

    label = gtk_label_new("Duplicate item handling");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label(
        "Skip testing for empty cells");
    gtk_widget_set_name(button, "empties");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cvi_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvi_empties = button;

    button = gtk_check_button_new_with_label(
        "Map all unmapped GDSII datatypes to same Xic layer");
    gtk_widget_set_name(button, "dtypes");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cvi_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvi_dtypes = button;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    entry = gtk_combo_box_text_new();
    gtk_widget_set_name(entry, "forcemenu");
    gtk_widget_show(entry);
    for (int i = 0; forcevals[i].name; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry),
            forcevals[i].name);
    }
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(cvi_force_menu_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), entry, true, true, 0);
    cvi_force = entry;

    label = gtk_label_new("How to resolve CIF layers");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label(
        "Don't flatten standard vias, keep as instance at top level");
    gtk_widget_set_name(button, "noflvias");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cvi_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvi_noflvias = button;

    button = gtk_check_button_new_with_label(
        "Don't flatten pcells, keep as instance at top level");
    gtk_widget_set_name(button, "noflpcs");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cvi_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvi_noflpcs = button;

    button = gtk_check_button_new_with_label(
        "Ignore labels in subcells when flattening");
    gtk_widget_set_name(button, "nofllbs");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cvi_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvi_nofllbs = button;

    button = gtk_check_button_new_with_label(
        "Skip reading text labels from physical archives");
    gtk_widget_set_name(button, "nolabels");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cvi_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    cvi_nolabels = button;

    GtkWidget *tab_label = gtk_label_new("Setup");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(cvi_nbook), form, tab_label);

    //
    // The Read File page
    //
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    rowcnt = 0;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    entry = gtk_combo_box_text_new();
    gtk_widget_set_name(entry, "mergmenu");
    gtk_widget_show(entry);
    for (int i = 0; mergvals[i]; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry),
            mergvals[i]);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(entry), cvi_merg_val);
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(cvi_merg_menu_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), entry, true, true, 0);
    cvi_merg = entry;

    label = gtk_label_new("Merge Into Current mode");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Cell name mapping
    //
    cvi_cnmap = new cnmap_t(false);
    gtk_table_attach(GTK_TABLE(form), cvi_cnmap->frame(), 0, 2, rowcnt,
        rowcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Layer filtering
    //
    cvi_llist = new llist_t;
    gtk_table_attach(GTK_TABLE(form), cvi_llist->frame(), 0, 2, rowcnt,
        rowcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Window
    //
    cvi_wnd = new wnd_t(wnd_sens_test, WndFuncIn);
    gtk_table_attach(GTK_TABLE(form), cvi_wnd->frame(), 0, 2, rowcnt,
        rowcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Scale spin button
    //
    label = gtk_label_new("Reading Scale Factor");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *sb = sb_scale.init(FIO()->ReadScale(), CDSCALEMIN, CDSCALEMAX,
        5);
    sb_scale.connect_changed(G_CALLBACK(cvi_val_changed), 0);
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Read File button
    //
    button = gtk_button_new_with_label("Read File");
    gtk_widget_set_name(button, "ReadFile");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cvi_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    tab_label = gtk_label_new("Read File");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(cvi_nbook), form, tab_label);

    //
    // Dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cvi_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(topform), button, 0, 2, toprcnt, toprcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(cvi_popup), button);

    update();
}


sCvi::~sCvi()
{
    Cvi = 0;
    if (cvi_caller)
        GTKdev::Deselect(cvi_caller);
    delete cvi_cnmap;
    delete cvi_llist;
    delete cvi_wnd;
    if (cvi_callback)
        (*cvi_callback)(-1, cvi_arg);
    if (cvi_popup)
        gtk_widget_destroy(cvi_popup);
}


void
sCvi::update()
{
    GTKdev::SetStatus(cvi_nonpc,
        CDvdb()->getVariable(VA_NoEvalNativePCells));
    GTKdev::SetStatus(cvi_yesoapc,
        CDvdb()->getVariable(VA_EvalOaPCells));
    GTKdev::SetStatus(cvi_nolyr,
        CDvdb()->getVariable(VA_NoCreateLayer));
    int overval = (FIO()->IsNoOverwritePhys() ? 2 : 0) +
        (FIO()->IsNoOverwriteElec() ? 1 : 0);
    if (CDvdb()->getVariable(VA_AutoRename))
        overval = 4;
    gtk_combo_box_set_active(GTK_COMBO_BOX(cvi_over), overval);
    GTKdev::SetStatus(cvi_replace,
        CDvdb()->getVariable(VA_NoAskOverwrite));
    GTKdev::SetStatus(cvi_merge,
        CDvdb()->getVariable(VA_MergeInput));
    GTKdev::SetStatus(cvi_polys,
        CDvdb()->getVariable(VA_NoPolyCheck));
    GTKdev::SetStatus(cvi_empties,
        CDvdb()->getVariable(VA_NoCheckEmpties));
    GTKdev::SetStatus(cvi_dtypes,
        CDvdb()->getVariable(VA_NoMapDatatypes));
    GTKdev::SetStatus(cvi_noflvias,
        CDvdb()->getVariable(VA_NoFlattenStdVias));
    GTKdev::SetStatus(cvi_noflpcs,
        CDvdb()->getVariable(VA_NoFlattenPCells));
    GTKdev::SetStatus(cvi_nofllbs,
        CDvdb()->getVariable(VA_NoFlattenLabels));
    GTKdev::SetStatus(cvi_nolabels,
        CDvdb()->getVariable(VA_NoReadLabels));

    int hst = 1;
    const char *str = CDvdb()->getVariable(VA_DupCheckMode);
    if (str) {
        if (*str == 'r' || *str == 'R')
            hst = 2;
        else if (*str == 'w' || *str == 'W')
            hst = 1;
        else
            hst = 0;
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(cvi_dup), hst);

    for (int i = 0; forcevals[i].name; i++) {
        if (forcevals[i].code == FIO()->CifStyle().lread_type()) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(cvi_force), i);
            break;
        }
    }
    sb_scale.set_value(FIO()->ReadScale());
    cvi_cnmap->update();
    cvi_llist->update();
    cvi_wnd->update();
    cvi_wnd->set_sens();
}


// Static function.
void
sCvi::cvi_cancel_proc(GtkWidget*, void*)
{
    Cvt()->PopUpImport(0, MODE_OFF, 0, 0);
}


// Static function.
void
sCvi::cvi_page_chg_proc(GtkWidget*, void*, int pg, void*)
{
    if (!Cvi)
        return;
    const char *lb;
    if (pg == 0)
        lb = "Set parameters for reading cell data";
    else
        lb = "Read cell data file";
    gtk_label_set_text(GTK_LABEL(Cvi->cvi_label), lb);
}


// Static function.
void
sCvi::cvi_action(GtkWidget *caller, void*)
{
    if (!Cvi)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "nonpc")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_NoEvalNativePCells, 0);
        else
            CDvdb()->clearVariable(VA_NoEvalNativePCells);
        return;
    }
    if (!strcmp(name, "yesoapc")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_EvalOaPCells, 0);
        else
            CDvdb()->clearVariable(VA_EvalOaPCells);
        return;
    }
    if (!strcmp(name, "nolyr")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_NoCreateLayer, 0);
        else
            CDvdb()->clearVariable(VA_NoCreateLayer);
        return;
    }
    if (!strcmp(name, "replace")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_NoAskOverwrite, 0);
        else
            CDvdb()->clearVariable(VA_NoAskOverwrite);
        return;
    }
    if (!strcmp(name, "merge")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_MergeInput, 0);
        else
            CDvdb()->clearVariable(VA_MergeInput);
        return;
    }
    if (!strcmp(name, "polys")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_NoPolyCheck, 0);
        else
            CDvdb()->clearVariable(VA_NoPolyCheck);
        return;
    }
    if (!strcmp(name, "empties")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_NoCheckEmpties, 0);
        else
            CDvdb()->clearVariable(VA_NoCheckEmpties);
        return;
    }
    if (!strcmp(name, "dtypes")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_NoMapDatatypes, 0);
        else
            CDvdb()->clearVariable(VA_NoMapDatatypes);
        return;
    }
    if (!strcmp(name, "noflvias")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_NoFlattenStdVias, 0);
        else
            CDvdb()->clearVariable(VA_NoFlattenStdVias);
        return;
    }
    if (!strcmp(name, "noflpcs")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_NoFlattenPCells, 0);
        else
            CDvdb()->clearVariable(VA_NoFlattenPCells);
        return;
    }
    if (!strcmp(name, "nofllbs")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_NoFlattenLabels, 0);
        else
            CDvdb()->clearVariable(VA_NoFlattenLabels);
        return;
    }
    if (!strcmp(name, "nolabels")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_NoReadLabels, 0);
        else
            CDvdb()->clearVariable(VA_NoReadLabels);
        return;
    }
    if (!strcmp(name, "luse")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_UseLayerList, 0);
        else
            CDvdb()->clearVariable(VA_UseLayerList);
    }
    if (!strcmp(name, "lskip")) {
        if (GTKdev::GetStatus(caller))
            CDvdb()->setVariable(VA_UseLayerList, "n");
        else
            CDvdb()->clearVariable(VA_UseLayerList);
    }
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:imprt"))
        return;
    }
    if (!strcmp(name, "ReadFile")) {
        if (!Cvi->cvi_callback ||
                !(*Cvi->cvi_callback)(sCvi::cvi_merg_val, Cvi->cvi_arg))
            Cvt()->PopUpImport(0, MODE_OFF, 0, 0);
        return;
    }
}


// Static function.
void
sCvi::cvi_over_menu_proc(GtkWidget *caller, void*)
{
    int i = gtk_combo_box_get_active(GTK_COMBO_BOX(caller));
    if (i == 0) {
        // Overwrite All (default)
        CDvdb()->clearVariable(VA_NoOverwritePhys);
        CDvdb()->clearVariable(VA_NoOverwriteElec);
        CDvdb()->clearVariable(VA_AutoRename);
    }
    else if (i == 1) {
        // Overwrite Phys
        CDvdb()->clearVariable(VA_NoOverwritePhys);
        CDvdb()->setVariable(VA_NoOverwriteElec, 0);
        CDvdb()->clearVariable(VA_AutoRename);
    }
    else if (i == 2) {
        // Ovewrwrite Elec
        CDvdb()->setVariable(VA_NoOverwritePhys, 0);
        CDvdb()->clearVariable(VA_NoOverwriteElec);
        CDvdb()->clearVariable(VA_AutoRename);
    }
    else if (i == 3) {
        // Overwrite None
        CDvdb()->setVariable(VA_NoOverwritePhys, 0);
        CDvdb()->setVariable(VA_NoOverwriteElec, 0);
        CDvdb()->clearVariable(VA_AutoRename);
    }
    else if (i == 4) {
        // Auto Rename
        CDvdb()->setVariable(VA_AutoRename, 0);
        CDvdb()->clearVariable(VA_NoOverwritePhys);
        CDvdb()->clearVariable(VA_NoOverwriteElec);
    }
}


// Static function.
void
sCvi::cvi_dup_menu_proc(GtkWidget *caller, void*)
{
    int i = gtk_combo_box_get_active(GTK_COMBO_BOX(caller));
    if (i == 0) {
        // Skip Test
        CDvdb()->setVariable(VA_DupCheckMode, "NoTest");
    }
    else if (i == 1) {
        // Warn
        CDvdb()->clearVariable(VA_DupCheckMode);
    }
    else if (i == 2) {
        // Remove Dups
        CDvdb()->setVariable(VA_DupCheckMode, "Remove");
    }
}


// Static function.
void
sCvi::cvi_force_menu_proc(GtkWidget *caller, void*)
{
    int i = gtk_combo_box_get_active(GTK_COMBO_BOX(caller));
    FIO()->CifStyle().set_lread_type(forcevals[i].code);
    if (FIO()->CifStyle().lread_type() != EXTlreadDef) {
        char buf[32];
        sprintf(buf, "%d", FIO()->CifStyle().lread_type());
        CDvdb()->setVariable(VA_CifLayerMode, buf);
    }
    else
        CDvdb()->clearVariable(VA_CifLayerMode);
}


// Static function.
void
sCvi::cvi_merg_menu_proc(GtkWidget *caller, void*)
{
    if (!Cvi)
        return;
    int i = gtk_combo_box_get_active(GTK_COMBO_BOX(caller));
    if (i > 0) {
        cvi_merg_val = i;
        // Can't use window/flatten ro scale factor.
        gtk_widget_set_sensitive(Cvi->cvi_wnd->frame(), false);
        Cvi->sb_scale.set_sensitive(false);
    }
    else if (i == 0) {
        cvi_merg_val = i;
        gtk_widget_set_sensitive(Cvi->cvi_wnd->frame(), true);
        Cvi->sb_scale.set_sensitive(true);
    }
}


// Static function.
void
sCvi::cvi_val_changed(GtkWidget*, void*)
{
    if (!Cvi)
        return;
    const char *s = Cvi->sb_scale.get_string();
    char *endp;
    double d = strtod(s, &endp);
    if (endp > s && d >= CDSCALEMIN && d <= CDSCALEMAX)
        FIO()->SetReadScale(d);
}


// Static function.
WndSensMode
sCvi::wnd_sens_test()
{
    return (WndSensFlatten);
}

