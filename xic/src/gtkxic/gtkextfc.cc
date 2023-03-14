
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
#include "ext.h"
#include "ext_fc.h"
#include "ext_fxunits.h"
#include "ext_fxjob.h"
#include "dsp_inlines.h"
#include "dsp_color.h"
#include "menu.h"
#include "select.h"
#include "tech.h"
#include "tech_extract.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkutil.h"
#include "gtkinterf/gtkspinbtn.h"


//-----------------------------------------------------------------------------
// Pop-up to control FasterCap/FastCap-WR interface
//
// Help system keywords used:
//  fcpanel

namespace {
    namespace gtkextfc {
        struct sFc : public GTKbag
        {
            sFc(GRobject);
            ~sFc();

            void update();
            void update_jobs_list();
            void update_label(const char*);
            void update_numbers();
            void clear_numbers();

        private:
            void select_range(int, int);
            int get_pid();
            void select_pid(int);

            static const char *fc_def_string(int);
            static void fc_cancel_proc(GtkWidget*, void*);
            static void fc_help_proc(GtkWidget*, void*);
            static void fc_page_change_proc(GtkWidget*, void*, int, void*);
            static void fc_change_proc(GtkWidget*, void*);
            static void fc_units_proc(GtkWidget*, void*);
            static void fc_p_cb(bool, void*);
            static void fc_dump_cb(const char*, void*);
            static void fc_btn_proc(GtkWidget*, void*);
            static int fc_button_dn(GtkWidget*, GdkEvent*, void*);
            static void fc_font_changed();
            static void fc_dbg_btn_proc(GtkWidget*, void*);

            GRobject fc_caller;
            GtkWidget *fc_label;
            GtkWidget *fc_foreg;
            GtkWidget *fc_out;
            GtkWidget *fc_shownum;
            GtkWidget *fc_file;
            GtkWidget *fc_args;
            GtkWidget *fc_path;

            GtkWidget *fc_units;
            GtkWidget *fc_enab;

            GtkWidget *fc_jobs;
            GtkWidget *fc_kill;

            GtkWidget *fc_dbg_zoids;
            GtkWidget *fc_dbg_vrbo;
            GtkWidget *fc_dbg_nm;
            GtkWidget *fc_dbg_czbot;
            GtkWidget *fc_dbg_dzbot;
            GtkWidget *fc_dbg_cztop;
            GtkWidget *fc_dbg_dztop;
            GtkWidget *fc_dbg_cyl;
            GtkWidget *fc_dbg_dyl;
            GtkWidget *fc_dbg_cyu;
            GtkWidget *fc_dbg_dyu;
            GtkWidget *fc_dbg_cleft;
            GtkWidget *fc_dbg_dleft;
            GtkWidget *fc_dbg_cright;
            GtkWidget *fc_dbg_dright;

            bool fc_no_reset;
            bool fc_frozen;
            int fc_start;
            int fc_end;
            int fc_line_selected;

            GTKspinBtn sb_fc_plane_bloat;
            GTKspinBtn sb_substrate_thickness;
            GTKspinBtn sb_substrate_eps;
            GTKspinBtnExp sb_fc_panel_target;
        };

        sFc *Fc;

        enum { FcRun, FcRunFile, FcDump, FcPath, FcArgs, ShowNums, 
            Foreg, ToCons, FcPlaneBloat, SubstrateThickness, 
            SubstrateEps, Enable, FcPanelTarget, Kill };
    }

    // FastHenry units menu, must have same order and length as used in
    // ext_fxunits.cc.
    //
    const char *units_strings[] =
    {
        "meters",
        "centimeters",
        "millimeters",
        "microns",
        "inches",
        "mils",
        0
    };
}

using namespace gtkextfc;


// Pop up a panel to control the fastcap/fasthenry interface.
//
void
cFC::PopUpExtIf(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Fc;
        return;
    }
    if (mode == MODE_UPD) {
        if (Fc)
            Fc->update();
        return;
    }
    if (Fc)
        return;

    new sFc(caller);
    if (!Fc->Shell()) {
        delete Fc;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Fc->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_LR), Fc->Shell(), mainBag()->Viewport());
    gtk_widget_show(Fc->Shell());
    setPopUpVisible(true);
}


void
cFC::updateString()
{
    if (Fc) {
        char *s = statusString();
        Fc->update_label(s);
        delete [] s;
    }
}


void
cFC::updateMarks()
{
    DSP()->EraseCrossMarks(Physical, CROSS_MARK_FC);
    if (Fc)
        Fc->update_numbers();
}


void
cFC::clearMarks()
{
    DSP()->EraseCrossMarks(Physical, CROSS_MARK_FC);
    delete [] fc_groups;
    fc_groups = 0;
    fc_ngroups = 0;
    if (Fc)
        Fc->clear_numbers();
}
// End of cFC functions.


sFc::sFc(GRobject c)
{
    Fc = this;
    fc_caller = c;
    fc_label = 0;
    fc_foreg = 0;
    fc_out = 0;
    fc_shownum = 0;
    fc_file = 0;
    fc_args = 0;
    fc_path = 0;
    fc_units = 0;
    fc_enab = 0;
    fc_jobs = 0;
    fc_kill = 0;
    fc_no_reset = false;
    fc_frozen = false;
    fc_start = 0;
    fc_end = 0;
    fc_line_selected = -1;

    wb_shell = gtk_NewPopup(0, "Cap. Extraction", fc_cancel_proc, 0);
    if (!wb_shell)
        return;

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    int rowcnt = 0;

    //
    // Label in frame plus help btn
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    GtkWidget *label = gtk_label_new("Fast[er]Cap Interface");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(hbox), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_help_proc), 0);
    gtk_box_pack_end(GTK_BOX(hbox), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *nbook = gtk_notebook_new();
    gtk_widget_show(nbook);
    g_signal_connect(G_OBJECT(nbook), "switch-page",
        G_CALLBACK(fc_page_change_proc), 0);
    gtk_table_attach(GTK_TABLE(form), nbook, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Run page
    //
    GtkWidget *table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);
    int row = 0;

    hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    button = gtk_check_button_new_with_label("Run in foreground");
    gtk_widget_set_name(button, "FcForeg");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_btn_proc), (void*)Foreg);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
    fc_foreg = button;

    button = gtk_check_button_new_with_label("Out to console");
    gtk_widget_set_name(button, "FcCons");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_btn_proc), (void*)ToCons);
    gtk_box_pack_end(GTK_BOX(hbox), button, false, false, 0);
    fc_out = button;

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    button = gtk_check_button_new_with_label("Show Numbers");
    gtk_widget_set_name(button, "ShowNums");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_btn_proc), (void*)ShowNums);
    fc_shownum = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, row, row+1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    row++;

    GtkWidget *hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);

    gtk_table_attach(GTK_TABLE(table), hsep, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    row++;

    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    button = gtk_button_new_with_label("Run File");
    gtk_widget_set_name(button, "FcRunFile");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_btn_proc), (void*)FcRunFile);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    GtkWidget *entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 0);
    fc_file = entry;

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    button = gtk_button_new_with_label("Run Extraction");
    gtk_widget_set_name(button, "FcRun");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_btn_proc), (void*)FcRun);

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label("Dump Unified List File");
    gtk_widget_set_name(button, "FcDump");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_btn_proc), (void*)FcDump);

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    frame = gtk_frame_new("FcArgs");
    gtk_widget_show(frame);
    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_container_add(GTK_CONTAINER(frame), entry);
    gtk_entry_set_text(GTK_ENTRY(entry), fc_def_string(FcArgs));
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(fc_change_proc), (void*)FcArgs);
    fc_args = entry;

    gtk_table_attach(GTK_TABLE(table), frame, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    frame = gtk_frame_new("Path to FasterCap or FastCap-WR");
    gtk_widget_show(frame);
    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_container_add(GTK_CONTAINER(frame), entry);
    gtk_entry_set_text(GTK_ENTRY(entry), fc_def_string(FcPath));
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(fc_change_proc), (void*)FcPath);
    fc_path = entry;

    gtk_table_attach(GTK_TABLE(table), frame, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    label = gtk_label_new("Run");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), table, label);

    //
    // Params page
    //
    table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);
    row = 0;

    int ndgt = CD()->numDigits();

    GtkWidget *sb = sb_fc_plane_bloat.init(FC_PLANE_BLOAT_DEF,
        FC_PLANE_BLOAT_MIN, FC_PLANE_BLOAT_MAX, ndgt);
    gtk_widget_set_size_request(sb, 100, -1);
    sb_fc_plane_bloat.connect_changed(G_CALLBACK(fc_change_proc),
        (void*)FcPlaneBloat, "FcPlaneBloat");
    frame = gtk_frame_new("FcPlaneBloat");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), sb);

    gtk_table_attach(GTK_TABLE(table), frame, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_substrate_thickness.init(SUBSTRATE_THICKNESS,
        SUBSTRATE_THICKNESS_MIN, SUBSTRATE_THICKNESS_MAX, ndgt);
    gtk_widget_set_size_request(sb, 100, -1);
    sb_substrate_thickness.connect_changed(G_CALLBACK(fc_change_proc),
        (void*)SubstrateThickness, "SubstrateThickness");
    frame = gtk_frame_new("SubstrateThickness");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), sb);

    gtk_table_attach(GTK_TABLE(table), frame, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    frame = gtk_frame_new("FcUnits");
    gtk_widget_show(frame);
    entry = gtk_combo_box_text_new();
    gtk_widget_set_name(entry, "FcUnits");
    gtk_widget_show(entry);
    for (int i = 0; units_strings[i]; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry),
            units_strings[i]);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(entry), FC()->getUnitsIndex(0));
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(fc_units_proc), 0);
    gtk_container_add(GTK_CONTAINER(frame), entry);
    fc_units = entry;

    gtk_table_attach(GTK_TABLE(table), frame, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_substrate_eps.init(SUBSTRATE_EPS, SUBSTRATE_EPS_MIN,
        SUBSTRATE_EPS_MAX, 3);
    gtk_widget_set_size_request(sb, 100, -1);
    sb_substrate_eps.connect_changed(G_CALLBACK(fc_change_proc),
        (void*)SubstrateEps, "SubstrateEps");
    frame = gtk_frame_new("SubstrateEps");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), sb);

    gtk_table_attach(GTK_TABLE(table), frame, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);

    gtk_table_attach(GTK_TABLE(table), hsep, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    row++;

    label = gtk_label_new("FastCap Panel Refinement");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    gtk_table_attach(GTK_TABLE(table), frame, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    button = gtk_check_button_new_with_label("Enable");
    gtk_widget_set_name(button, "Enable");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_btn_proc), (void*)Enable);
    fc_enab = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    sb = sb_fc_panel_target.init(FC_DEF_TARG_PANELS, FC_MIN_TARG_PANELS,
        FC_MAX_TARG_PANELS, 1);
    gtk_widget_set_size_request(sb, 100, -1);
    sb_fc_panel_target.connect_changed(G_CALLBACK(fc_change_proc),
        (void*)FcPanelTarget, "FcPanelTarget");
    frame = gtk_frame_new("FcPanelTarget");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), sb);

    gtk_table_attach(GTK_TABLE(table), frame, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    label = gtk_label_new("Params");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), table, label);

    //
    // Debug page
    //
    table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);
    row = 0;

    button = gtk_check_button_new_with_label("Zoids");
    gtk_widget_set_name(button, "Zoids");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_zoids = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_check_button_new_with_label("Verbose Out");
    gtk_widget_set_name(button, "VrbO");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_vrbo = button;

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    row++;

    button = gtk_check_button_new_with_label("No Merge");
    gtk_widget_set_name(button, "NoMerge");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_nm = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    row++;

    button = gtk_check_button_new_with_label("C zbot");
    gtk_widget_set_name(button, "czbot");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_czbot = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_check_button_new_with_label("D zbot");
    gtk_widget_set_name(button, "dzbot");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_dzbot = button;

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    row++;

    button = gtk_check_button_new_with_label("C ztop");
    gtk_widget_set_name(button, "cztop");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_cztop = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_check_button_new_with_label("D ztop");
    gtk_widget_set_name(button, "dztop");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_dztop = button;

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    row++;

    button = gtk_check_button_new_with_label("C yl");
    gtk_widget_set_name(button, "cyl");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_cyl = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_check_button_new_with_label("D yl");
    gtk_widget_set_name(button, "dyl");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_dyl = button;

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    row++;

    button = gtk_check_button_new_with_label("C yu");
    gtk_widget_set_name(button, "cyu");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_cyu = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_check_button_new_with_label("D yu");
    gtk_widget_set_name(button, "dyu");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_dyu = button;

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    row++;

    button = gtk_check_button_new_with_label("C left");
    gtk_widget_set_name(button, "cleft");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_cleft = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_check_button_new_with_label("D left");
    gtk_widget_set_name(button, "dleft");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_dleft = button;

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    row++;

    button = gtk_check_button_new_with_label("C right");
    gtk_widget_set_name(button, "cright");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_cright = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_check_button_new_with_label("D right");
    gtk_widget_set_name(button, "dright");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_dbg_btn_proc), 0);
    fc_dbg_dright = button;

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    row++;


    label = gtk_label_new("Debug");
    gtk_widget_show(label);
    int pg = gtk_notebook_append_page(GTK_NOTEBOOK(nbook), table, label);
    if (pg >= 0 && !CDvdb()->getVariable(VA_FcDebug))
        gtk_widget_hide(gtk_notebook_get_nth_page(GTK_NOTEBOOK(nbook), pg));

    //
    // Jobs page
    //
    table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);
    row = 0;

    GtkWidget *contr;
    text_scrollable_new(&contr, &fc_jobs, FNT_FIXED);

    gtk_widget_add_events(fc_jobs, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(fc_jobs), "button-press-event",
        G_CALLBACK(fc_button_dn), 0);

    // The font change pop-up uses this to redraw the widget
    g_object_set_data(G_OBJECT(fc_jobs), "font_changed",
        (void*)fc_font_changed);

    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(fc_jobs));
    const char *bclr = GRpkgIf()->GetAttrColor(GRattrColorLocSel);
    gtk_text_buffer_create_tag(textbuf, "primary", "background", bclr,
        "paragraph-background", bclr, NULL);

    gtk_widget_set_size_request(fc_jobs, 200, 200);

    gtk_table_attach(GTK_TABLE(table), contr, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    button = gtk_button_new_with_label("Abort job");
    gtk_widget_set_name(button, "Abort");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_btn_proc), (void*)Kill);
    fc_kill = button;

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    label = gtk_label_new("Jobs");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), table, label);

    //
    // Status line and Dismiss button
    //
    char *s = FC()->statusString();
    fc_label = gtk_label_new(s);
    delete [] s;
    gtk_widget_show(fc_label);
    gtk_misc_set_alignment(GTK_MISC(fc_label), 0.5, 0.5);
    gtk_misc_set_padding(GTK_MISC(fc_label), 2, 2);
    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), fc_label);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fc_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    update();
}


sFc::~sFc()
{
    FC()->showMarks(false);
    Fc = 0;
    if (fc_caller)
        GRX->Deselect(fc_caller);
    FC()->setPopUpVisible(false);
    if (wb_shell) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(wb_shell),
            (gpointer)sFc::fc_cancel_proc, wb_shell);
    }
}


void
sFc::update()
{
    const char *var, *cur;
    fc_no_reset = true;

    // Run page
    GRX->SetStatus(fc_foreg, CDvdb()->getVariable(VA_FcForeg));
    GRX->SetStatus(fc_out, CDvdb()->getVariable(VA_FcMonitor));

    var = CDvdb()->getVariable(VA_FcArgs);
    if (!var)
        var = fc_def_string(FcArgs);
    cur = gtk_entry_get_text(GTK_ENTRY(fc_args));
    if (!cur)
        cur = "";
    if (strcmp(var, cur))
        gtk_entry_set_text(GTK_ENTRY(fc_args), var);

    var = CDvdb()->getVariable(VA_FcPath);
    if (!var)
        var = fc_def_string(FcPath);
    cur = gtk_entry_get_text(GTK_ENTRY(fc_path));
    if (!cur)
        cur = "";
    if (strcmp(var, cur))
        gtk_entry_set_text(GTK_ENTRY(fc_path), var);

    // Params page
    var = CDvdb()->getVariable(VA_FcPlaneBloat);
    if (sb_fc_plane_bloat.is_valid(var))
        sb_fc_plane_bloat.set_value(atof(var));
    else {
        if (var)
            CDvdb()->clearVariable(VA_FcPlaneBloat);
        sb_fc_plane_bloat.set_value(FC_PLANE_BLOAT_DEF);
    }

    var = CDvdb()->getVariable(VA_SubstrateThickness);
    if (sb_substrate_thickness.is_valid(var))
        sb_substrate_thickness.set_value(atof(var));
    else {
        if (var)
            CDvdb()->clearVariable(VA_SubstrateThickness);
        sb_substrate_thickness.set_value(SUBSTRATE_THICKNESS);
    }

    var = CDvdb()->getVariable(VA_SubstrateEps);
    if (sb_substrate_eps.is_valid(var))
        sb_substrate_eps.set_value(atof(var));
    else {
        if (var)
            CDvdb()->clearVariable(VA_SubstrateEps);
        sb_substrate_eps.set_value(SUBSTRATE_EPS);
    }

    var = CDvdb()->getVariable(VA_FcUnits);
    if (!var)
        var = "";
    int uoff = FC()->getUnitsIndex(var);
    int ucur = gtk_combo_box_get_active(GTK_COMBO_BOX(fc_units));
    if (uoff != ucur)
        gtk_combo_box_set_active(GTK_COMBO_BOX(fc_units), uoff);

    static double fcpt_bak;
    var = CDvdb()->getVariable(VA_FcPanelTarget);
    if (sb_fc_panel_target.is_valid(var)) {
        sb_fc_panel_target.set_value(atof(var));
        sb_fc_panel_target.set_sensitive(true);
        GRX->SetStatus(fc_enab, true);
        fcpt_bak = sb_fc_panel_target.get_value();
    }
    else {
        if (var)
            CDvdb()->clearVariable(VA_FcPanelTarget);
        if (fcpt_bak > 0.0)
            sb_fc_panel_target.set_value(fcpt_bak);
        sb_fc_panel_target.set_sensitive(false);
        GRX->SetStatus(fc_enab, false);
    }

    // Debug page
    GRX->SetStatus(fc_dbg_zoids, CDvdb()->getVariable(VA_FcZoids));
    GRX->SetStatus(fc_dbg_vrbo, CDvdb()->getVariable(VA_FcVerboseOut));
    GRX->SetStatus(fc_dbg_nm, CDvdb()->getVariable(VA_FcNoMerge));

    unsigned int flgs = MRG_ALL;
    const char *s = CDvdb()->getVariable(VA_FcMergeFlags);
    if (s)
        flgs = Tech()->GetInt(s) & MRG_ALL;
    GRX->SetStatus(fc_dbg_czbot, (flgs & MRG_C_ZBOT));
    GRX->SetStatus(fc_dbg_dzbot, (flgs & MRG_D_ZBOT));
    GRX->SetStatus(fc_dbg_cztop, (flgs & MRG_C_ZTOP));
    GRX->SetStatus(fc_dbg_dztop, (flgs & MRG_D_ZTOP));
    GRX->SetStatus(fc_dbg_cyl, (flgs & MRG_C_YL));
    GRX->SetStatus(fc_dbg_dyl, (flgs & MRG_D_YL));
    GRX->SetStatus(fc_dbg_cyu, (flgs & MRG_C_YU));
    GRX->SetStatus(fc_dbg_dyu, (flgs & MRG_D_YU));
    GRX->SetStatus(fc_dbg_cleft, (flgs & MRG_C_LEFT));
    GRX->SetStatus(fc_dbg_dleft, (flgs & MRG_D_LEFT));
    GRX->SetStatus(fc_dbg_cright, (flgs & MRG_C_RIGHT));
    GRX->SetStatus(fc_dbg_dright, (flgs & MRG_D_RIGHT));

    // Jobs page
    update_jobs_list();

    fc_no_reset = false;
}


void
sFc::update_jobs_list()
{
    if (!fc_jobs)
        return;
    GdkColor *c1 = gtk_PopupColor(GRattrColorHl4);

    int pid = get_pid();
    if (pid <= 0)
        gtk_widget_set_sensitive(fc_kill, false);

    double val = text_get_scroll_value(fc_jobs);
    text_set_chars(fc_jobs, "");

    char *list = FC()->jobList();
    if (!list)
        text_insert_chars_at_point(fc_jobs, c1,
            "No background jobs running.", -1, -1);

    else
        text_insert_chars_at_point(fc_jobs, 0, list, -1, -1);
    text_set_scroll_value(fc_jobs, val);
    if (pid > 0)
        select_pid(pid);
    gtk_widget_set_sensitive(fc_kill, get_pid() > 0);
}


void
sFc::update_label(const char *s)
{
    gtk_label_set_text(GTK_LABEL(fc_label), s);
}


void
sFc::update_numbers()
{
    if (GRX->GetStatus(fc_shownum))
        FC()->showMarks(true);
}


void
sFc::clear_numbers()
{
    GRX->SetStatus(fc_shownum, false);
}


// Select the chars in the range, start=end deselects existing.  In
// GTK-1, selecting gives blue inverse, which turns gray if
// unselected, retaining an indication for the buttons.  GTK-2
// doesn't do this automatically so we provide something similar here.
//
void
sFc::select_range(int start, int end)
{
    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(fc_jobs));
    GtkTextIter istart, iend;
    if (fc_end != fc_start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, fc_start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, fc_end);
        gtk_text_buffer_remove_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    text_select_range(fc_jobs, start, end);
    if (end != start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, end);
        gtk_text_buffer_apply_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    fc_start = start;
    fc_end = end;
}


// Return the PID value of the selected line, or -1.
//
int
sFc::get_pid()
{
    if (fc_line_selected < 0)
        return (-1);
    char *string = text_get_chars(fc_jobs, 0, -1);
    int line = 0;
    for (const char *s = string; *s; s++) {
        if (line == fc_line_selected) {
            while (isspace(*s))
                s++;
            int pid;
            int r = sscanf(s, "%d", &pid);
            delete [] string;
            return (r == 1 ? pid : -1);
        }
        if (*s == '\n')
            line++;
    }
    delete [] string;
    return (-1);
}


void
sFc::select_pid(int p)
{
    char *string = text_get_chars(fc_jobs, 0, -1);
    bool nl = true;
    int line = 0;
    const char *cs = 0;
    for (const char *s = string; *s; s++) {
        if (nl) {
            cs = s;
            while (isspace(*s))
                s++;
            nl = false;
            int pid;
            int r = sscanf(s, "%d", &pid);
            if (r == 1 && p == pid) {
                const char *ce = cs;
                while (*ce && *ce != 'n')
                    ce++;
                select_range(cs - string, ce - string);
                delete [] string;
                fc_line_selected = line;
                gtk_widget_set_sensitive(fc_kill, true);
                return;
            }
        }
        if (*s == '\n') {
            nl = true;
            line++;
        }
    }
    delete [] string;
}


// Static function.
// Return the default text field text.
//
const char *
sFc::fc_def_string(int id)
{
    int ndgt = CD()->numDigits();
    static char tbuf[16];
    switch (id) {
    case FcPath:
        return (fxJob::fc_default_path());
    case FcArgs:
        return ("");
    case FcPlaneBloat:
        sprintf(tbuf, "%.*f", ndgt, FC_PLANE_BLOAT_DEF);
        return (tbuf);
    case SubstrateThickness:
        sprintf(tbuf, "%.*f", ndgt, SUBSTRATE_THICKNESS);
        return (tbuf);
    case SubstrateEps:
        sprintf(tbuf, "%.*f", 3, SUBSTRATE_EPS);
        return (tbuf);
    case FcPanelTarget:
        sprintf(tbuf, "%.*e", 1, FC_DEF_TARG_PANELS);
        return (tbuf);
    }
    return ("");
}


// Static function.
void
sFc::fc_cancel_proc(GtkWidget*, void*)
{
    FC()->PopUpExtIf(0, MODE_OFF);
}


// Static function.
void
sFc::fc_help_proc(GtkWidget*, void*)
{
    DSPmainWbag(PopUpHelp("fcpanel"))
}


void
sFc::fc_page_change_proc(GtkWidget*, void*, int, void*)
{
}


namespace {
    bool check_num(const char *s, double minv, double maxv)
    {
        if (!s || !*s)
            return (true);
        double d;
        if (sscanf(s, "%lf", &d) != 1)
            return (true);
        if (d < minv || d > maxv)
            return (true);
        return (false);
    }
}


// Static function.
void
sFc::fc_change_proc(GtkWidget *widget, void *arg)
{
    if (!Fc || Fc->fc_no_reset)
        return;
    const char *s = gtk_entry_get_text(GTK_ENTRY(widget));
    if (!s)
        return;
    int id = (intptr_t)arg;
    switch (id) {
    case FcPath:
        if (!strcmp(s, fc_def_string(id)))
            CDvdb()->clearVariable(VA_FcPath);
        else
            CDvdb()->setVariable(VA_FcPath, s);
        break;
    case FcArgs:
        if (!strcmp(s, fc_def_string(id)))
            CDvdb()->clearVariable(VA_FcArgs);
        else
            CDvdb()->setVariable(VA_FcArgs, s);
        break;
    case FcPlaneBloat:
        if (check_num(s, FC_PLANE_BLOAT_MIN, FC_PLANE_BLOAT_MAX))
            break;
        if (!strcmp(s, fc_def_string(id)))
            CDvdb()->clearVariable(VA_FcPlaneBloat);
        else
            CDvdb()->setVariable(VA_FcPlaneBloat, s);
        break;
    case SubstrateThickness:
        if (check_num(s, SUBSTRATE_THICKNESS_MIN, SUBSTRATE_THICKNESS_MAX))
            break;
        if (!strcmp(s, fc_def_string(id)))
            CDvdb()->clearVariable(VA_SubstrateThickness);
        else
            CDvdb()->setVariable(VA_SubstrateThickness, s);
        break;
    case SubstrateEps:
        if (check_num(s, SUBSTRATE_EPS_MIN, SUBSTRATE_EPS_MAX))
            break;
        if (!strcmp(s, fc_def_string(id)))
            CDvdb()->clearVariable(VA_SubstrateEps);
        else
            CDvdb()->setVariable(VA_SubstrateEps, s);
        break;
    case FcPanelTarget:
        if (check_num(s, FC_MIN_TARG_PANELS, FC_MAX_TARG_PANELS))
            break;
        if (!strcmp(s, fc_def_string(id)))
            CDvdb()->clearVariable(VA_FcPanelTarget);
        else
            CDvdb()->setVariable(VA_FcPanelTarget, s);
        break;
    }
}


// Static function.
void
sFc::fc_units_proc(GtkWidget *caller, void*)
{
    int i = gtk_combo_box_get_active(GTK_COMBO_BOX(caller));
    if (i < 0)
        return;
    const char *str = FC()->getUnitsString(units_strings[i]);
    if (str)
        CDvdb()->setVariable(VA_FcUnits, str);
    else
        CDvdb()->clearVariable(VA_FcUnits);
}


// Static function.
void
sFc::fc_p_cb(bool ok, void *arg)
{
    char *fname = (char*)arg;
    if (ok)
        DSPmainWbag(PopUpFileBrowser(fname))
    delete [] fname;
}


// Static function.
void
sFc::fc_dump_cb(const char *fname, void *client_data)
{
    switch ((intptr_t)client_data) {
    case FcDump:
        if (FC()->fcDump(fname)) {
            if (!Fc)
                return;
            const char *fn = lstring::strip_path(fname);
            char tbuf[256];
            sprintf(tbuf, "Input is in file %s.  View file? ", fn);
            Fc->PopUpAffirm(0, GRloc(LW_UL), tbuf, fc_p_cb,
                lstring::copy(fname));
        }
        break;
    }
    if (Fc && Fc->wb_input)
        Fc->wb_input->popdown();
}


// Static function.
void
sFc::fc_btn_proc(GtkWidget *widget, void *arg)
{
    if (!Fc)
        return;
    switch ((intptr_t)arg) {
    case FcRun:
        FC()->fcRun(0, 0, 0);
        break;
    case FcRunFile:
        {
            const char *s = gtk_entry_get_text(GTK_ENTRY(Fc->fc_file));
            char *tok = lstring::getqtok(&s);
            if (tok) {
                FC()->fcRun(tok, 0, 0, true);
                delete [] tok;
            }
            else {
                Fc->PopUpErr(MODE_ON, "No file name given!");
                return;
            }
        }
        break;
    case FcDump:
        {
            char *s = FC()->getFileName(FC_LST_SFX);
            Fc->PopUpInput(0, s, "Dump", fc_dump_cb, (void*)FcDump);
            delete [] s;
        }
        break;
    case ShowNums:
        FC()->showMarks(GRX->GetStatus(widget));
        break;
    case Foreg:
        if (GRX->GetStatus(widget))
            CDvdb()->setVariable(VA_FcForeg, "");
        else
            CDvdb()->clearVariable(VA_FcForeg);
        break;
    case ToCons:
        if (GRX->GetStatus(widget))
            CDvdb()->setVariable(VA_FcMonitor, "");
        else
            CDvdb()->clearVariable(VA_FcMonitor);
        break;
    case Enable:
        if (GRX->GetStatus(widget)) {
            const char *s = Fc->sb_fc_panel_target.get_string();
            if (!check_num(s, FC_MIN_TARG_PANELS, FC_MAX_TARG_PANELS))
                CDvdb()->setVariable(VA_FcPanelTarget, s);
            else {
                char tbf[32];
                sprintf(tbf, "%.1e", FC_DEF_TARG_PANELS);
                CDvdb()->setVariable(VA_FcPanelTarget, tbf);
            }
            Fc->sb_fc_panel_target.set_sensitive(true);
        }
        else {
            CDvdb()->clearVariable(VA_FcPanelTarget);
            Fc->sb_fc_panel_target.set_sensitive(false);
        }
        break;
    case Kill:
        {
            int pid = Fc->get_pid();
            if (pid > 0)
                FC()->killProcess(pid);
        }
    }
}


// Static function.
int
sFc::fc_button_dn(GtkWidget *caller, GdkEvent *event, void*)
{
    if (!Fc)
        return (true);
    if (event->type != GDK_BUTTON_PRESS)
        return (true);

    char *string = text_get_chars(caller, 0, -1);
    int x = (int)event->button.x;
    int y = (int)event->button.y;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(caller), &ihere, x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    char *line_start = string + gtk_text_iter_get_offset(&iline);
    x = gtk_text_iter_get_offset(&ihere) - gtk_text_iter_get_offset(&iline);
    y = gtk_text_iter_get_line(&iline);

    // line_start points to start of selected line, or 0
    if (line_start && *line_start != '\n') {

        int start = line_start - string;
        int end = start;
        while (string[end] && string[end] != '\n')
            end++;

        Fc->fc_line_selected = y;
        Fc->select_range(start, end);
        delete [] string;
        gtk_widget_set_sensitive(Fc->fc_kill, Fc->get_pid() > 0);
        return (true);
    }
    Fc->fc_line_selected = -1;
    delete [] string;
    Fc->select_range(0, 0);
    gtk_widget_set_sensitive(Fc->fc_kill, false);
    return (true);
}


// Static function.
void
sFc::fc_font_changed()
{
    if (Fc)
        Fc->update_jobs_list();
}


// Static function.
void
sFc::fc_dbg_btn_proc(GtkWidget *widget, void*)
{
    if (!Fc)
        return;
    const char *name = gtk_widget_get_name(widget);
    if (!name)
        return;
    bool state = GRX->GetStatus(widget);
        
    if (!strcmp(name, "NoMerge")) {
        if (state)
            CDvdb()->setVariable(VA_FcNoMerge, "");
        else
            CDvdb()->clearVariable(VA_FcNoMerge);
        return;
    }
    if (!strcmp(name, "Zoids")) {
        if (state)
            CDvdb()->setVariable(VA_FcZoids, "");
        else
            CDvdb()->clearVariable(VA_FcZoids);
        return;
    }
    if (!strcmp(name, "VrbO")) {
        if (state)
            CDvdb()->setVariable(VA_FcVerboseOut, "");
        else
            CDvdb()->clearVariable(VA_FcVerboseOut);
        return;
    }
    unsigned int mrgflgs = MRG_ALL;
    const char *s = CDvdb()->getVariable(VA_FcMergeFlags);
    if (s)
        mrgflgs = Tech()->GetInt(s) & MRG_ALL;
    unsigned bkflgs = mrgflgs;

    if (!strcmp(name, "czbot")) {
        if (state)
            mrgflgs |= MRG_C_ZBOT;
        else
            mrgflgs &= ~MRG_C_ZBOT;
    }
    else if (!strcmp(name, "dzbot")) {
        if (state)
            mrgflgs |= MRG_D_ZBOT;
        else
            mrgflgs &= ~MRG_D_ZBOT;
    }
    else if (!strcmp(name, "cztop")) {
        if (state)
            mrgflgs |= MRG_C_ZTOP;
        else
            mrgflgs &= ~MRG_C_ZTOP;
    }
    else if (!strcmp(name, "dztop")) {
        if (state)
            mrgflgs |= MRG_D_ZTOP;
        else
            mrgflgs &= ~MRG_D_ZTOP;
    }
    else if (!strcmp(name, "cyl")) {
        if (state)
            mrgflgs |= MRG_C_YL;
        else
            mrgflgs &= ~MRG_C_YL;
    }
    else if (!strcmp(name, "dyl")) {
        if (state)
            mrgflgs |= MRG_D_YL;
        else
            mrgflgs &= ~MRG_D_YL;
    }
    else if (!strcmp(name, "cyu")) {
        if (state)
            mrgflgs |= MRG_C_YU;
        else
            mrgflgs &= ~MRG_C_YU;
    }
    else if (!strcmp(name, "dyu")) {
        if (state)
            mrgflgs |= MRG_D_YU;
        else
            mrgflgs &= ~MRG_D_YU;
    }
    else if (!strcmp(name, "cleft")) {
        if (state)
            mrgflgs |= MRG_C_LEFT;
        else
            mrgflgs &= ~MRG_C_LEFT;
    }
    else if (!strcmp(name, "dleft")) {
        if (state)
            mrgflgs |= MRG_D_LEFT;
        else
            mrgflgs &= ~MRG_D_LEFT;
    }
    else if (!strcmp(name, "cright")) {
        if (state)
            mrgflgs |= MRG_C_RIGHT;
        else
            mrgflgs &= ~MRG_C_RIGHT;
    }
    else if (!strcmp(name, "dright")) {
        if (state)
            mrgflgs |= MRG_D_RIGHT;
        else
            mrgflgs &= ~MRG_D_RIGHT;
    }
    if (mrgflgs != bkflgs) {
        if (mrgflgs == MRG_ALL)
            CDvdb()->clearVariable(VA_FcMergeFlags);
        else {
            char buf[32];
            sprintf(buf, "0x%x", mrgflgs);
            CDvdb()->setVariable(VA_FcMergeFlags, buf);
        }
    }
}
