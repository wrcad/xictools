
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: gtkextset.cc,v 5.41 2017/03/15 00:49:17 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_rlsolver.h"
#include "sced.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkspinbtn.h"
#include "events.h"
#include "promptline.h"
#include "errorlog.h"
#include "menu.h"
#include "ebtn_menu.h"
#include "tech.h"
#include "tech_extract.h"
#include "filestat.h"


//--------------------------------------------------------------------
// Pop-up to control misc. extraction variables.
//
// Help system keywords used:
//  xic:excfg

namespace {
    namespace gtkextset {
        struct sEs
        {
            sEs(void*);
            ~sEs();

            void update();

            GtkWidget *shell() { return (es_popup); }

        private:
            void views_and_ops_page();
            void net_and_cell_page();
            void devs_page();
            void misc_page();
            void set_sens();

            static void es_cancel_proc(GtkWidget*, void*);
            static void es_show_grp_node(GtkWidget*);
            static void es_action(GtkWidget*, void*);
            static void es_val_changed(GtkWidget*, void*);
            static void es_gpinv_proc(GtkWidget*, void*);
            void dev_menu_upd();
            static bool es_editsave(const char*, void*, XEtype);
            static void es_dev_menu_proc(GtkWidget*, void*, unsigned int);

            GRobject es_caller;
            GtkWidget *es_popup;
            GtkWidget *es_notebook;
            GtkWidget *es_clrex;
            GtkWidget *es_doex;
            GtkWidget *es_p1_extview;
            GtkWidget *es_p1_groups;
            GtkWidget *es_p1_nodes;
            GtkWidget *es_p1_terms;
            GtkWidget *es_p1_cterms;
            GtkWidget *es_p1_recurs;
            GtkWidget *es_p1_tedit;
            GtkWidget *es_p1_tfind;

            GtkWidget *es_p2_menu;
            GtkWidget *es_p2_delblk;
            GtkWidget *es_p2_undblk;

            GtkWidget *es_p2_nlprpset;
            GtkWidget *es_p2_nlprp;
            GtkWidget *es_p2_nllset;
            GtkWidget *es_p2_nll;
            GtkWidget *es_p2_ignlab;
            GtkWidget *es_p2_oldlab;
            GtkWidget *es_p2_updlab;
            GtkWidget *es_p2_merge;
            GtkWidget *es_p2_vcvx;
            GtkWidget *es_p2_vsubs;
            GtkWidget *es_p2_gpglob;
            GtkWidget *es_p2_gpmulti;
            GtkWidget *es_p2_gpmthd;

            GtkWidget *es_p3_noseries;
            GtkWidget *es_p3_nopara;
            GtkWidget *es_p3_keepshrt;
            GtkWidget *es_p3_nomrgshrt;
            GtkWidget *es_p3_nomeas;
            GtkWidget *es_p3_usecache;
            GtkWidget *es_p3_nordcache;
            GtkWidget *es_p3_deltaset;
            GtkWidget *es_p3_trytile;
            GtkWidget *es_p3_lmax;
            GtkWidget *es_p3_lgrid;

            GtkWidget *es_p4_flkeyset;
            GtkWidget *es_p4_flkeys;
            GtkWidget *es_p4_exopq;
            GtkWidget *es_p4_vrbos;
            GtkWidget *es_p4_glbexset;
            GtkWidget *es_p4_glbex;
            GtkWidget *es_p4_noperm;
            GtkWidget *es_p4_apmrg;
            GtkWidget *es_p4_apfix;

            GtkItemFactory *es_item_factory;
            int es_gpmhst;
            sDevDesc *es_devdesc;

            GTKspinBtn sb_vdepth;
            GTKspinBtn sb_delta;
            GTKspinBtn sb_maxpts;
            GTKspinBtn sb_gridpts;
            GTKspinBtn sb_loop;
            GTKspinBtn sb_iters;
        };

        sEs *Es;
    }
}

using namespace gtkextset;


void
cExt::PopUpExtSetup(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Es;
        return;
    }
    if (mode == MODE_UPD) {
        if (Es)
            Es->update();
        return;
    }
    if (Es)
        return;

    new sEs(caller);
    if (!Es->shell()) {
        delete Es;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Es->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_LR), Es->shell(), mainBag()->Viewport());
    gtk_widget_show(Es->shell());
}


sEs::sEs(GRobject c)
{
    Es = this;
    es_caller = c;
    es_popup = 0;
    es_notebook = 0;
    es_clrex = 0;
    es_doex = 0;
    es_p1_extview = 0;
    es_p1_groups = 0;
    es_p1_nodes = 0;
    es_p1_terms = 0;
    es_p1_cterms = 0;
    es_p1_recurs = 0;
    es_p1_tedit = 0;
    es_p1_tfind = 0;
    es_p2_menu = 0;
    es_p2_delblk = 0;
    es_p2_undblk = 0;
    es_p2_nlprpset = 0;
    es_p2_nlprp = 0;
    es_p2_nllset = 0;
    es_p2_nll = 0;
    es_p2_ignlab = 0;
    es_p2_oldlab = 0;
    es_p2_updlab = 0;
    es_p2_merge = 0;
    es_p2_vcvx = 0;
    es_p2_vsubs = 0;
    es_p2_gpglob = 0;
    es_p2_gpmulti = 0;
    es_p2_gpmthd = 0;
    es_p3_noseries = 0;
    es_p3_nopara = 0;
    es_p3_keepshrt = 0;
    es_p3_nomrgshrt = 0;
    es_p3_nomeas = 0;
    es_p3_usecache = 0;
    es_p3_nordcache = 0;
    es_p3_deltaset = 0;
    es_p3_trytile = 0;
    es_p3_lmax = 0;
    es_p3_lgrid = 0;
    es_p4_flkeyset = 0;
    es_p4_flkeys = 0;
    es_p4_exopq = 0;
    es_p4_vrbos = 0;
    es_p4_glbexset = 0;
    es_p4_glbex = 0;
    es_p4_noperm = 0;
    es_p4_apmrg = 0;
    es_p4_apfix = 0;
    es_item_factory = 0;
    es_gpmhst = 0;
    es_devdesc = 0;

    es_popup = gtk_NewPopup(0, "Extraction Setup", es_cancel_proc,
        0);
    if (!es_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(es_popup), false);

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(es_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("Set parameters for extraction");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(row), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    es_notebook = gtk_notebook_new();
    gtk_widget_show(es_notebook);
    gtk_table_attach(GTK_TABLE(form), es_notebook, 0, 2, rowcnt,
        rowcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    views_and_ops_page();
    net_and_cell_page();
    devs_page();
    misc_page();

    //
    // set/clear extraction buttons, dismiss button
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    button = gtk_button_new_with_label("Clear Extraction");
    gtk_widget_set_name(button, "ClearEx");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);
    es_clrex = button;

    button = gtk_button_new_with_label("Do Extraction");
    gtk_widget_set_name(button, "DoEx");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);
    es_doex = button;

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_cancel_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(es_popup), button);

    update();
}


sEs::~sEs()
{
    // Must do this before zeroing Es.
    if (es_p1_tedit && GRX->GetStatus(es_p1_tedit))
        GRX->CallCallback(es_p1_tedit);
    Es = 0;
    delete es_devdesc;
    SCD()->PopUpNodeMap(0, MODE_OFF);
    if (es_caller)
        GRX->Deselect(es_caller);
    if (es_item_factory)
        g_object_unref(es_item_factory);
    if (es_popup)
        gtk_widget_destroy(es_popup);
}


void
sEs::views_and_ops_page()
{
    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    int rowcnt = 0;

    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 2, 2);
    rowcnt++;

    //
    // The Show group
    //
    GtkWidget *frame = gtk_frame_new("Show");
    gtk_widget_show(frame);

    GtkWidget *tform = gtk_table_new(4, 1, false);
    gtk_widget_show(tform);
    gtk_container_set_border_width(GTK_CONTAINER(tform), 2);
    gtk_container_add(GTK_CONTAINER(frame), tform);

    GtkWidget *button = gtk_check_button_new_with_label("Extraction View");
    gtk_widget_set_name(button, "ExtView");
    gtk_widget_show(button);
    es_p1_extview = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(tform), button, 0, 2, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(0), 2, 2);

    button = gtk_check_button_new_with_label("Groups");
    gtk_widget_set_name(button, "Groups");
    gtk_widget_show(button);
    es_p1_groups = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(tform), button, 2, 3, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(0), 2, 2);

    button = gtk_check_button_new_with_label("Nodes");
    gtk_widget_set_name(button, "Nodes");
    gtk_widget_show(button);
    es_p1_nodes = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(tform), button, 3, 4, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(0), 2, 2);

    button = gtk_check_button_new_with_label("All Terminals");
    gtk_widget_set_name(button, "Terms");
    gtk_widget_show(button);
    es_p1_terms = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(tform), button, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(0), 2, 2);

    button = gtk_check_button_new_with_label("Cell Terminals Only");
    gtk_widget_set_name(button, "CTOnly");
    gtk_widget_show(button);
    es_p1_cterms = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(tform), button, 2, 4, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(0), 2, 2);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 2, 2);
    rowcnt++;

    //
    // The Terminals group
    //
    frame = gtk_frame_new("Terminals");
    gtk_widget_show(frame);

    tform = gtk_table_new(2, 2, false);
    gtk_widget_show(tform);
    gtk_container_set_border_width(GTK_CONTAINER(tform), 2);
    gtk_container_add(GTK_CONTAINER(frame), tform);

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    button = gtk_button_new_with_label("Reset Terms");
    gtk_widget_set_name(button, "ResetTerms");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_button_new_with_label("Reset Subckts");
    gtk_widget_set_name(button, "ResetSubs");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_check_button_new_with_label("Recursive");
    gtk_widget_set_name(button, "Recurse");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    es_p1_recurs = button;

    gtk_table_attach(GTK_TABLE(tform), row, 0, 2, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(0), 2, 2);

    button = gtk_toggle_button_new_with_label("Edit Terminals");
    gtk_widget_set_name(button, "EditTerms");
    gtk_widget_show(button);
    es_p1_tedit = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(tform), button, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(0), 2, 2);

    button = gtk_toggle_button_new_with_label("Find Terminal");
    gtk_widget_set_name(button, "FindTerm");
    gtk_widget_show(button);
    es_p1_tfind = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(tform), button, 1, 2, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(0), 2, 2);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 2, 2);
    rowcnt++;

    //
    // Selection of unassociated objects.
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 2, 2);
    rowcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    GtkWidget *label = gtk_label_new("Select Unassociated");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    button = gtk_button_new_with_label("Groups/Nodes");
    gtk_widget_set_name(button, "GrpNodSel");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_button_new_with_label("Devices");
    gtk_widget_set_name(button, "DevsSel");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_button_new_with_label("Subckts");
    gtk_widget_set_name(button, "SubcSel");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(0), 2, 2);
    rowcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 2, 2);
    rowcnt++;

    GtkWidget *tab_label = gtk_label_new("Views and\nOperations");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(es_notebook), form, tab_label);
}


void
sEs::net_and_cell_page()
{
    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    int rowcnt = 0;

    //
    // Nets
    //
    GtkWidget *frame = gtk_frame_new("Net label purpose name");
    gtk_widget_show(frame);
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    gtk_container_add(GTK_CONTAINER(frame), row);

    GtkWidget *button = gtk_toggle_button_new_with_label("Apply");
    gtk_widget_set_name(button, "NlpSet");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);
    es_p2_nlprpset = button;

    es_p2_nlprp = gtk_entry_new();
    gtk_widget_show(es_p2_nlprp);
    gtk_box_pack_start(GTK_BOX(row), es_p2_nlprp, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    frame = gtk_frame_new("Net label layer");
    gtk_widget_show(frame);
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    gtk_container_add(GTK_CONTAINER(frame), row);

    button = gtk_toggle_button_new_with_label("Apply");
    gtk_widget_set_name(button, "NllSet");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);
    es_p2_nllset = button;

    es_p2_nll = gtk_entry_new();
    gtk_widget_show(es_p2_nll);
    gtk_box_pack_start(GTK_BOX(row), es_p2_nll, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), frame, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label(
        "Ignore net name labels");
    gtk_widget_set_name(button, "IgnName");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    es_p2_ignlab = button;

    button = gtk_check_button_new_with_label(
        "Find old-style net (term name) labels");
    gtk_widget_set_name(button, "OldName");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    es_p2_oldlab = button;

    button = gtk_check_button_new_with_label(
        "Update net name labels after association");
    gtk_widget_set_name(button, "UpdName");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    es_p2_updlab = button;

    button = gtk_check_button_new_with_label(
        "Merge groups with matching net names");
    gtk_widget_set_name(button, "Merge");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    es_p2_merge = button;

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Via detection group
    //
    frame = gtk_frame_new("Via Detection");
    gtk_widget_show(frame);

    GtkWidget *vform = gtk_table_new(2, 1, false);
    gtk_widget_show(vform);
    gtk_container_set_border_width(GTK_CONTAINER(vform), 2);
    gtk_container_add(GTK_CONTAINER(frame), vform);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    int vcnt = 0;

    button = gtk_check_button_new_with_label("Assume convex vias");
    gtk_widget_set_name(button, "ViaCvx");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(vform), button, 0, 1, vcnt, vcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    es_p2_vcvx = button;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    GtkWidget *label = gtk_label_new("Via search depth");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);
    es_p3_lmax = label;

    GtkWidget *vsb = sb_vdepth.init(EXT_DEF_VIA_SEARCH_DEPTH, 0,
        CDMAXCALLDEPTH, 0);
    sb_vdepth.connect_changed(GTK_SIGNAL_FUNC(es_val_changed), 0, "ViaDepth");
    gtk_widget_set_usize(vsb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), vsb, false, false, 0);

    gtk_table_attach(GTK_TABLE(vform), row, 1, 2, vcnt, vcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    vcnt++;

    button = gtk_check_button_new_with_label(
        "Check for via connections between subcells");
    gtk_widget_set_name(button, "ViaSubs");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(vform), button, 0, 2, vcnt, vcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    es_p2_vsubs = button;

    //
    // Ground plane group.
    //
    frame = gtk_frame_new("Ground Plane Handling");
    gtk_widget_show(frame);

    GtkWidget *tform = gtk_table_new(2, 1, false);
    gtk_widget_show(tform);
    gtk_container_set_border_width(GTK_CONTAINER(tform), 2);
    gtk_container_add(GTK_CONTAINER(frame), tform);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    int tcnt = 0;

    button = gtk_check_button_new_with_label(
        "Assume clear-field ground plane is global");
    gtk_widget_set_name(button, "GPGlob");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(tform), button, 0, 2, tcnt, tcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    tcnt++;
    es_p2_gpglob = button;

    button = gtk_check_button_new_with_label(
        "Invert dark-field ground plane for multi-nets");
    gtk_widget_set_name(button, "GPMulti");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(tform), button, 0, 2, tcnt, tcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    tcnt++;
    es_p2_gpmulti = button;

    es_p2_gpmthd = gtk_option_menu_new();
    gtk_widget_set_name(es_p2_gpmthd, "GPMthd");
    gtk_widget_show(es_p2_gpmthd);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "GPMthd");

    GtkWidget *mi = gtk_menu_item_new_with_label(
        "Invert in each cell, clip out subcells");
    gtk_widget_set_name(mi, "0");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(es_gpinv_proc), 0);
    mi = gtk_menu_item_new_with_label("Invert flat in top-level cell");
    gtk_widget_set_name(mi, "1");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(es_gpinv_proc), 0);
    mi = gtk_menu_item_new_with_label("Invert flat in all cells");
    gtk_widget_set_name(mi, "2");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(es_gpinv_proc), 0);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(es_p2_gpmthd), menu);
    gtk_table_attach(GTK_TABLE(tform), es_p2_gpmthd, 0, 2, tcnt, tcnt+1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)0, 2, 2);
    tcnt++;

    GtkWidget *tab_label = gtk_label_new("Net\nConfig");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(es_notebook), form, tab_label);
}


#define IFINIT(i, a, b, c, d, e) { \
    menu_items[i].path = (char*)a; \
    menu_items[i].accelerator = (char*)b; \
    menu_items[i].callback = (GtkItemFactoryCallback)c; \
    menu_items[i].callback_action = d; \
    menu_items[i].item_type = (char*)e; \
    i++; }

// Page 3, Device Config.
//
void
sEs::devs_page()
{
    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    int rowcnt = 0;

    //
    // Menu bar.
    //
    GtkItemFactoryEntry menu_items[50];
    int nitems = 0;

    IFINIT(nitems, "/_Device Block", 0, 0, 0, "<Branch>");
    IFINIT(nitems, "/Device Block/New", 0, es_dev_menu_proc,
        0, 0);
    IFINIT(nitems, "/Device Block/Delete", 0, es_dev_menu_proc,
        1, "<CheckItem>");
    IFINIT(nitems, "/Device Block/Undelete", 0, es_dev_menu_proc,
        2, 0);
    IFINIT(nitems, "/Device Block/sep1", 0, 0, 0, "<Separator>");
    GtkAccelGroup *accel_group = gtk_accel_group_new();
    es_item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<extrc>",
        accel_group);
    for (int i = 0; i < nitems; i++)
        gtk_item_factory_create_item(es_item_factory, menu_items + i, 0, 2);
    gtk_window_add_accel_group(GTK_WINDOW(es_popup), accel_group);

    GtkWidget *menubar = gtk_item_factory_get_widget(es_item_factory,
        "<extrc>");
    gtk_widget_show(menubar);

    es_p2_menu = gtk_item_factory_get_item(es_item_factory, "/Device Block");
    if (es_p2_menu)
        es_p2_menu = GTK_MENU_ITEM(es_p2_menu)->submenu;
    if (es_p2_menu) {
        stringlist *dnames = EX()->listDevices();
        for (stringlist *sl = dnames; sl; sl = sl->next) {
            const char *t = sl->string;
            char *nm = lstring::gettok(&t);
            char *pf = lstring::gettok(&t);
            sDevDesc *d = EX()->findDevice(nm, pf);
            delete [] nm;
            delete [] pf;

            if (d) {
                GtkWidget *mi = gtk_menu_item_new_with_label(sl->string);
                gtk_widget_show(mi);
                gtk_signal_connect(GTK_OBJECT(mi), "activate",
                    GTK_SIGNAL_FUNC(es_dev_menu_proc), d);
                gtk_menu_append(GTK_MENU(es_p2_menu), mi);
            }
        }
        dnames->free();
    }

    es_p2_delblk =
        gtk_item_factory_get_widget(es_item_factory, "/Device Block/Delete");
    es_p2_undblk =
        gtk_item_factory_get_widget(es_item_factory, "/Device Block/Undelete");
    gtk_widget_set_sensitive(es_p2_undblk, false);

    gtk_table_attach(GTK_TABLE(form), menubar, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Check boxes.
    //
    GtkWidget *button = gtk_check_button_new_with_label(
        "Don't merge series devices");
    gtk_widget_set_name(button, "NoSeries");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    es_p3_noseries = button;

    button = gtk_check_button_new_with_label("Don't merge parallel devices");
    gtk_widget_set_name(button, "NoPara");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    es_p3_nopara = button;

    button = gtk_check_button_new_with_label(
        "Include devices with terminals shorted");
    gtk_widget_set_name(button, "KeepShrt");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    es_p3_keepshrt = button;

    button = gtk_check_button_new_with_label(
        "Don't merge devices with terminals shorted");
    gtk_widget_set_name(button, "NoMrgShrt");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    es_p3_nomrgshrt = button;

    button = gtk_check_button_new_with_label(
        "Skip device parameter measurement");
    gtk_widget_set_name(button, "NoMeas");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    es_p3_nomeas = button;

    button = gtk_check_button_new_with_label(
        "Use measurement results cache property");
    gtk_widget_set_name(button, "UseCache");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    es_p3_usecache = button;

    button = gtk_check_button_new_with_label(
        "Don't read measurement results from property");
    gtk_widget_set_name(button, "NoRdCache");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    es_p3_nordcache = button;

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // RL solver group.
    //
    GtkWidget *frame = gtk_frame_new("Resistor/Inductor Extraction");
    gtk_widget_show(frame);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 2, 2);
    rowcnt++;
    int rlcnt = 0;

    GtkWidget *rlform = gtk_table_new(2, 1, false);
    gtk_widget_show(rlform);
    gtk_container_set_border_width(GTK_CONTAINER(rlform), 2);
    gtk_container_add(GTK_CONTAINER(frame), rlform);

    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    button = gtk_check_button_new_with_label("Set/use fixed grid size");
    gtk_widget_set_name(button, "DeltaSet");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    es_p3_deltaset = button;

    int ndgt = CD()->numDigits();
    GtkWidget *sb = sb_delta.init(0.01, 0.01, 1000.0, ndgt);
    sb_delta.connect_changed(GTK_SIGNAL_FUNC(es_val_changed), 0, "Delta");
    gtk_widget_set_usize(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(rlform), row, 0, 2, rlcnt, rlcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rlcnt++;

    button = gtk_check_button_new_with_label("Try to tile");
    gtk_widget_set_name(button, "TryTile");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(rlform), button, 0, 1, rlcnt, rlcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    es_p3_trytile = button;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    GtkWidget *label = gtk_label_new("Maximum tile count per device");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);
    es_p3_lmax = label;

    sb = sb_maxpts.init(1000.0, 1000.0, 100000.0, 0);
    sb_maxpts.connect_changed(GTK_SIGNAL_FUNC(es_val_changed), 0, "MaxPts");
    gtk_widget_set_usize(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(rlform), row, 1, 2, rlcnt, rlcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rlcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Set fixed per-device grid cell count");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);
    es_p3_lgrid = label;

    sb = sb_gridpts.init(10.0, 10.0, 100000.0, 0);
    sb_gridpts.connect_changed(GTK_SIGNAL_FUNC(es_val_changed), 0, "GridPts");
    gtk_widget_set_usize(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(rlform), row, 0, 2, rlcnt, rlcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *tab_label = gtk_label_new("Device\nConfig");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(es_notebook), form, tab_label);
}


// Page 4, Misc Config.
//
void
sEs::misc_page()
{
    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    int rowcnt = 0;

    //
    // Flatten token entry group, check box.
    //
    GtkWidget *frame = gtk_frame_new("Cell flattening name keys");
    gtk_widget_show(frame);
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    gtk_container_add(GTK_CONTAINER(frame), row);

    GtkWidget *button = gtk_toggle_button_new_with_label("Apply");
    gtk_widget_set_name(button, "UseKeys");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);
    es_p4_flkeyset = button;

    es_p4_flkeys = gtk_entry_new();
    gtk_widget_show(es_p4_flkeys);
    gtk_box_pack_start(GTK_BOX(row), es_p4_flkeys, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label(
        "Extract opaque cells, ignore OPAQUE flag");
    gtk_widget_set_name(button, "Opaque");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    es_p4_exopq = button;

    button = gtk_check_button_new_with_label(
        "Be very verbose on prompt line during extraction.");
    gtk_widget_set_name(button, "Verbose");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    es_p4_vrbos = button;

    frame = gtk_frame_new("Global exclude layer expression");
    gtk_widget_show(frame);
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    gtk_container_add(GTK_CONTAINER(frame), row);

    button = gtk_toggle_button_new_with_label("Apply");
    gtk_widget_set_name(button, "GlbSet");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);
    es_p4_glbexset = button;

    es_p4_glbex = gtk_entry_new();
    gtk_widget_show(es_p4_glbex);
    gtk_box_pack_start(GTK_BOX(row), es_p4_glbex, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Association group.
    //
    frame = gtk_frame_new("Association");
    gtk_widget_show(frame);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 2, 2);
    rowcnt++;
    int arowcnt = 0;

    GtkWidget *aform = gtk_table_new(2, 1, false);
    gtk_widget_show(aform);
    gtk_container_set_border_width(GTK_CONTAINER(aform), 2);
    gtk_container_add(GTK_CONTAINER(frame), aform);

    button = gtk_check_button_new_with_label(
        "Don't run symmetry trials in association");
    gtk_widget_set_name(button, "NoPerm");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(aform), button, 0, 2, arowcnt, arowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    arowcnt++;
    es_p4_noperm = button;

    button = gtk_check_button_new_with_label(
        "Logically merge physical contacts for split net handling");
    gtk_widget_set_name(button, "MergePC");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(aform), button, 0, 2, arowcnt, arowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    arowcnt++;
    es_p4_apmrg = button;

    button = gtk_check_button_new_with_label(
        "Apply post-association permutation fix");
    gtk_widget_set_name(button, "PrmFix");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(es_action), 0);
    gtk_table_attach(GTK_TABLE(aform), button, 0, 2, arowcnt, arowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    arowcnt++;
    es_p4_apfix = button;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    GtkWidget *label = gtk_label_new("Maximum association loop count");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    GtkWidget *sb = sb_loop.init(EXT_DEF_LVS_LOOP_MAX, 0.0, 1000000.0, 0);
    sb_loop.connect_changed(GTK_SIGNAL_FUNC(es_val_changed), 0, "Loop");
    gtk_widget_set_usize(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(aform), row, 1, 2, arowcnt, arowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    arowcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Maximum association iterations");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    sb = sb_iters.init(EXT_DEF_LVS_ITER_MAX, 10.0, 1000000.0, 0);
    sb_iters.connect_changed(GTK_SIGNAL_FUNC(es_val_changed), 0, "Iters");
    gtk_widget_set_usize(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(aform), row, 1, 2, arowcnt, arowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    arowcnt++;

    GtkWidget *tab_label = gtk_label_new("Misc\nConfig");
    gtk_widget_show(tab_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(es_notebook), form, tab_label);
}


void
sEs::update()
{
    // page 1
    bool physmode = DSP()->CurMode() == Physical;
    gtk_widget_set_sensitive(es_p1_terms, physmode);
    gtk_widget_set_sensitive(es_p1_cterms, physmode);

    if (DSP()->CurMode() == Electrical) {
        GRX->SetStatus(es_p1_tedit,
            Menu()->MenuButtonStatus("side", MenuSUBCT));
        GRX->SetStatus(es_p1_tfind,
            Menu()->MenuButtonStatus("side", MenuNODMP));
    }

    GRX->SetStatus(es_p1_extview, EX()->isExtractionView());
    GRX->SetStatus(es_p1_groups,
        EX()->isShowingGroups() && !EX()->isShowingNodes());
    GRX->SetStatus(es_p1_nodes,
        EX()->isShowingGroups() && EX()->isShowingNodes());
    GRX->SetStatus(es_p1_terms,
        DSP()->ShowTerminals() && DSP()->TerminalsVisible());
    GRX->SetStatus(es_p1_cterms,
        DSP()->ShowTerminals() && DSP()->ContactsVisible());

    // page 2
    const char *s = CDvdb()->getVariable(VA_PinPurpose);
    if (s) {
        gtk_entry_set_text(GTK_ENTRY(es_p2_nlprp), s);
        GRX->SetStatus(es_p2_nlprpset, true);
    }
    else
        GRX->SetStatus(es_p2_nlprpset, false);

    s = CDvdb()->getVariable(VA_PinLayer);
    if (s) {
        gtk_entry_set_text(GTK_ENTRY(es_p2_nll), s);
        GRX->SetStatus(es_p2_nllset, true);
    }
    else
        GRX->SetStatus(es_p2_nllset, false);

    GRX->SetStatus(es_p2_ignlab,
        CDvdb()->getVariable(VA_IgnoreNetLabels) != 0);
    GRX->SetStatus(es_p2_oldlab,
        CDvdb()->getVariable(VA_FindOldTermLabels) != 0);
    GRX->SetStatus(es_p2_updlab,
        CDvdb()->getVariable(VA_UpdateNetLabels) != 0);
    GRX->SetStatus(es_p2_merge,
        CDvdb()->getVariable(VA_MergeMatchingNamed) != 0);
    GRX->SetStatus(es_p2_vcvx,
        CDvdb()->getVariable(VA_ViaConvex) != 0);
    GRX->SetStatus(es_p2_vsubs,
        CDvdb()->getVariable(VA_ViaCheckBtwnSubs) != 0);

    s = sb_vdepth.get_string();
    int n;
    if (sscanf(s, "%d", &n) != EXT_DEF_VIA_SEARCH_DEPTH ||
            n != EX()->viaSearchDepth())
        sb_vdepth.set_value(EX()->viaSearchDepth());

    GRX->SetStatus(es_p2_gpglob,
        CDvdb()->getVariable(VA_GroundPlaneGlobal) != 0);
    GRX->SetStatus(es_p2_gpmulti,
        CDvdb()->getVariable(VA_GroundPlaneMulti) != 0);

    if (es_gpmhst != (int)Tech()->GroundPlaneMode())
        gtk_option_menu_set_history(GTK_OPTION_MENU(es_p2_gpmthd),
            Tech()->GroundPlaneMode());

    // page 3
    GRX->SetStatus(es_p3_noseries,
        CDvdb()->getVariable(VA_NoMergeSeries) != 0);
    GRX->SetStatus(es_p3_nopara,
        CDvdb()->getVariable(VA_NoMergeParallel) != 0);
    GRX->SetStatus(es_p3_keepshrt,
        CDvdb()->getVariable(VA_KeepShortedDevs) != 0);
    GRX->SetStatus(es_p3_nomrgshrt,
        CDvdb()->getVariable(VA_NoMergeShorted) != 0);
    GRX->SetStatus(es_p3_nomeas,
        CDvdb()->getVariable(VA_NoMeasure) != 0);
    GRX->SetStatus(es_p3_usecache,
        CDvdb()->getVariable(VA_UseMeasurePrpty) != 0);
    GRX->SetStatus(es_p3_nordcache,
        CDvdb()->getVariable(VA_NoReadMeasurePrpty) != 0);
    GRX->SetStatus(es_p3_trytile,
        CDvdb()->getVariable(VA_RLSolverTryTile) != 0);
    GRX->SetStatus(es_p3_deltaset,
        CDvdb()->getVariable(VA_RLSolverDelta) != 0);

    s = sb_maxpts.get_string();
    if (sscanf(s, "%d", &n) != 1 || n != RLsolver::rl_maxgrid)
        sb_maxpts.set_value(RLsolver::rl_maxgrid);

    s = sb_gridpts.get_string();
    if (sscanf(s, "%d", &n) != 1 || n != RLsolver::rl_numgrid)
        sb_gridpts.set_value(RLsolver::rl_numgrid);

    if (GRX->GetStatus(es_p3_deltaset)) {
        s = sb_delta.get_string();
        double d;
        if (sscanf(s, "%lf", &d) != 1 ||
                INTERNAL_UNITS(d) != RLsolver::rl_given_delta) {
            int ndgt = CD()->numDigits();
            sb_delta.set_digits(ndgt);
            sb_delta.set_value(MICRONS(RLsolver::rl_given_delta));
        }
    }

    // page 4
    s = CDvdb()->getVariable(VA_FlattenPrefix);
    if (s) {
        gtk_entry_set_text(GTK_ENTRY(es_p4_flkeys), s);
        GRX->SetStatus(es_p4_flkeyset, true);
    }
    else
        GRX->SetStatus(es_p4_flkeyset, false);

    GRX->SetStatus(es_p4_exopq,
        CDvdb()->getVariable(VA_ExtractOpaque) != 0);

    GRX->SetStatus(es_p4_vrbos,
        CDvdb()->getVariable(VA_VerbosePromptline) != 0);

    s = CDvdb()->getVariable(VA_GlobalExclude);
    if (s) {
        gtk_entry_set_text(GTK_ENTRY(es_p4_glbex), s);
        GRX->SetStatus(es_p4_glbexset, true);
    }
    else
        GRX->SetStatus(es_p4_glbexset, false);

    GRX->SetStatus(es_p4_noperm,
        CDvdb()->getVariable(VA_NoPermute) != 0);
    GRX->SetStatus(es_p4_apmrg,
        CDvdb()->getVariable(VA_MergePhysContacts) != 0);
    GRX->SetStatus(es_p4_apfix,
        CDvdb()->getVariable(VA_SubcPermutationFix) != 0);

    s = sb_loop.get_string();
    if (sscanf(s, "%d", &n) != 1 || n != cGroupDesc::assoc_loop_max())
        sb_maxpts.set_value(cGroupDesc::assoc_loop_max());
    s = sb_iters.get_string();
    if (sscanf(s, "%d", &n) != 1 || n != cGroupDesc::assoc_iter_max())
        sb_maxpts.set_value(cGroupDesc::assoc_iter_max());

    set_sens();
}


void
sEs::set_sens()
{
    if (GRX->GetStatus(es_p2_nlprpset))
        gtk_widget_set_sensitive(es_p2_nlprp, false);
    else
        gtk_widget_set_sensitive(es_p2_nlprp, true);
    if (GRX->GetStatus(es_p2_nllset))
        gtk_widget_set_sensitive(es_p2_nll, false);
    else
        gtk_widget_set_sensitive(es_p2_nll, true);
    if (GRX->GetStatus(es_p2_gpmulti))
        gtk_widget_set_sensitive(es_p2_gpmthd, true);
    else
        gtk_widget_set_sensitive(es_p2_gpmthd, false);

    if (GRX->GetStatus(es_p3_deltaset)) {
        sb_delta.set_sensitive(false);
        gtk_widget_set_sensitive(es_p3_trytile, false);
        gtk_widget_set_sensitive(es_p3_lmax, false);
        sb_maxpts.set_sensitive(false);
        gtk_widget_set_sensitive(es_p3_lgrid, false);
        sb_gridpts.set_sensitive(false);
    }
    else {
        sb_delta.set_sensitive(true);
        gtk_widget_set_sensitive(es_p3_trytile, true);

        if (GRX->GetStatus(es_p3_trytile)) {
            gtk_widget_set_sensitive(es_p3_lmax, true);
            sb_maxpts.set_sensitive(true);
            gtk_widget_set_sensitive(es_p3_lgrid, false);
            sb_gridpts.set_sensitive(false);
        }
        else {
            gtk_widget_set_sensitive(es_p3_lmax, false);
            sb_maxpts.set_sensitive(false);
            gtk_widget_set_sensitive(es_p3_lgrid, true);
            sb_gridpts.set_sensitive(true);
        }
    }

    if (GRX->GetStatus(es_p4_flkeyset))
        gtk_widget_set_sensitive(es_p4_flkeys, false);
    else
        gtk_widget_set_sensitive(es_p4_flkeys, true);
    if (GRX->GetStatus(es_p4_glbexset))
        gtk_widget_set_sensitive(es_p4_glbex, false);
    else
        gtk_widget_set_sensitive(es_p4_glbex, true);

}


// Static function.
void
sEs::es_cancel_proc(GtkWidget*, void*)
{
    EX()->PopUpExtSetup(0, MODE_OFF);
}


// Static function.
void
sEs::es_show_grp_node(GtkWidget *caller)
{
    CDs *cursdp = CurCell(Physical);
    if (caller && cursdp && Menu()->GetStatus(caller)) {
        if (!EX()->associate(cursdp)) {
            GRX->Deselect(caller);
            return;
        }
        cGroupDesc *gd = cursdp->groups();
        if (!gd || gd->isempty()) {
            GRX->Deselect(caller);
            return;
        }
        bool shownodes = (caller == Es->es_p1_nodes);
        WindowDesc *wd;
        if (EX()->isShowingGroups()) {
            if (shownodes)
                GRX->Deselect(Es->es_p1_groups);
            else
                GRX->Deselect(Es->es_p1_nodes);
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wd = wgen.next()) != 0)
                gd->show_groups(wd, ERASE);
        }
        EX()->setShowingNodes(shownodes);
        EX()->setShowingGroups(true);
        gd->set_group_display(true);
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wd = wgen.next()) != 0)
            gd->show_groups(wd, DISPLAY);
    }
    else {
        cGroupDesc *gd = cursdp ? cursdp->groups() : 0;
        if (gd) {
            WindowDesc *wd;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wd = wgen.next()) != 0)
                gd->show_groups(wd, ERASE);
            gd->set_group_display(false);
        }
        EX()->setShowingGroups(false);
        EX()->setShowingNodes(false);
    }
}


// Static function.
void
sEs::es_action(GtkWidget *caller, void*)
{
    if (!Es)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "Help"))
        DSPmainWbag(PopUpHelp("xic:excfg"))
    else if (!strcmp(name, "ClearEx")) {
        EX()->invalidateGroups();
        PL()->ShowPrompt("Extraction state invalidated.");
    }
    else if (!strcmp(name, "DoEx")) {
        EV()->InitCallback();
        if (!EX()->associate(CurCell(Physical)))
            PL()->ShowPrompt("Extraction/Association failed!");
        else
            PL()->ShowPrompt("Extraction/Association complete.");
    }

    // Page 1.
    else if (!strcmp(name, "ExtView")) {
        EX()->showExtractionView(Es->es_p1_extview);
    }
    else if (!strcmp(name, "Groups")) {
        if (!CurCell(Physical)) {
            Menu()->Deselect(caller);
            PL()->ShowPrompt("No physical data for current cell!");
            return;
        }
        es_show_grp_node(Es->es_p1_groups);
    }
    else if (!strcmp(name, "Nodes")) {
        if (!CurCell(Electrical)) {
            Menu()->Deselect(caller);
            PL()->ShowPrompt("No electrical data for current cell!");
            return;
        }
        es_show_grp_node(Es->es_p1_nodes);
    }
    else if (!strcmp(name, "Terms")) {
        if (GRX->GetStatus(Es->es_p1_terms))
            GRX->SetStatus(Es->es_p1_cterms, false);
        EX()->showPhysTermsExec(Es->es_p1_terms, false);
    }
    else if (!strcmp(name, "CTOnly")) {
        if (GRX->GetStatus(Es->es_p1_cterms))
            GRX->SetStatus(Es->es_p1_terms, false);
        EX()->showPhysTermsExec(Es->es_p1_cterms, true);
    }
    else if (!strcmp(name, "ResetTerms")) {
        bool tmpt = DSP()->ShowTerminals();
        if (tmpt)
            DSP()->ShowTerminals(ERASE);
        CDcbin cbin(DSP()->CurCellName());
        EX()->reset(&cbin, false, true, GRX->GetStatus(Es->es_p1_recurs));
        if (tmpt)
            DSP()->ShowTerminals(DISPLAY);
    }
    else if (!strcmp(name, "ResetSubs")) {
        bool tmpt = DSP()->ShowTerminals();
        if (tmpt)
            DSP()->ShowTerminals(ERASE);
        CDcbin cbin(DSP()->CurCellName());
        EX()->reset(&cbin, true, false, GRX->GetStatus(Es->es_p1_recurs));
        EX()->arrangeInstLabels(&cbin);
        if (tmpt)
            DSP()->ShowTerminals(DISPLAY);
    }
    else if (!strcmp(name, "Recurse")) {
        // nothing to do
    }
    else if (!strcmp(name, "GrpNodSel")) {
        CDs *cursdp = CurCell(Physical);
        if (!cursdp) {
            return;
        }
        EX()->associate(cursdp);
        cGroupDesc *gd = cursdp->groups();
        gd->select_unassoc_groups();
        gd->select_unassoc_nodes();
    }
    else if (!strcmp(name, "DevsSel")) {
        CDs *cursdp = CurCell(Physical);
        if (!cursdp) {
            return;
        }
        EX()->associate(cursdp);
        cGroupDesc *gd = cursdp->groups();
        gd->select_unassoc_pdevs();
        gd->select_unassoc_edevs();
    }
    else if (!strcmp(name, "SubcSel")) {
        CDs *cursdp = CurCell(Physical);
        if (!cursdp) {
            return;
        }
        EX()->associate(cursdp);
        cGroupDesc *gd = cursdp->groups();
        gd->select_unassoc_psubs();
        gd->select_unassoc_esubs();
    }
    else if (!strcmp(name, "EditTerms")) {
        if (DSP()->CurMode() == Physical)
            EX()->editTermsExec(Es->es_p1_tedit, Es->es_p1_cterms);
        else {
            bool state = GRX->GetStatus(caller);
            bool st = Menu()->MenuButtonStatus("side", MenuSUBCT);
            if (st != state)
                Menu()->MenuButtonPress("side", MenuSUBCT);
        }
    }
    else if (!strcmp(name, "FindTerm")) {
        bool state = GRX->GetStatus(caller);
        if (DSP()->CurMode() == Physical) {
            if (state)
                SCD()->PopUpNodeMap(caller, MODE_ON);
            else
                SCD()->PopUpNodeMap(0, MODE_OFF);
        }
        else {
            bool st = Menu()->MenuButtonStatus("side", MenuNODMP);
            if (st != state)
                Menu()->MenuButtonPress("side", MenuNODMP);
        }
    }

    // Page 2.
    else if (!strcmp(name, "NlpSet")) {
        if (GRX->GetStatus(caller)) {
            const char *s = gtk_entry_get_text(GTK_ENTRY(Es->es_p2_nlprp));
            CDvdb()->setVariable(VA_PinPurpose, s);
        }
        else
            CDvdb()->clearVariable(VA_PinPurpose);
    }
    else if (!strcmp(name, "NllSet")) {
        if (GRX->GetStatus(caller)) {
            const char *s = gtk_entry_get_text(GTK_ENTRY(Es->es_p2_nll));
            CDvdb()->setVariable(VA_PinLayer, s);
        }
        else
            CDvdb()->clearVariable(VA_PinLayer);
    }
    else if (!strcmp(name, "IgnName")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_IgnoreNetLabels, "");
        else
            CDvdb()->clearVariable(VA_IgnoreNetLabels);
    }
    else if (!strcmp(name, "OldName")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_FindOldTermLabels, "");
        else
            CDvdb()->clearVariable(VA_FindOldTermLabels);
    }
    else if (!strcmp(name, "UpdName")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_UpdateNetLabels, "");
        else
            CDvdb()->clearVariable(VA_UpdateNetLabels);
    }
    else if (!strcmp(name, "Merge")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_MergeMatchingNamed, "");
        else
            CDvdb()->clearVariable(VA_MergeMatchingNamed);
    }
    else if (!strcmp(name, "ViaCvx")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_ViaConvex, "");
        else
            CDvdb()->clearVariable(VA_ViaConvex);
    }
    else if (!strcmp(name, "ViaSubs")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_ViaCheckBtwnSubs, "");
        else
            CDvdb()->clearVariable(VA_ViaCheckBtwnSubs);
    }
    else if (!strcmp(name, "GPGlob")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_GroundPlaneGlobal, "");
        else
            CDvdb()->clearVariable(VA_GroundPlaneGlobal);
    }
    else if (!strcmp(name, "GPMulti")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_GroundPlaneMulti, "");
        else
            CDvdb()->clearVariable(VA_GroundPlaneMulti);
    }

    // Page 3.
    else if (!strcmp(name, "NoSeries")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoMergeSeries, "");
        else
            CDvdb()->clearVariable(VA_NoMergeSeries);
    }
    else if (!strcmp(name, "NoPara")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoMergeParallel, "");
        else
            CDvdb()->clearVariable(VA_NoMergeParallel);
    }
    else if (!strcmp(name, "KeepShrt")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_KeepShortedDevs, "");
        else
            CDvdb()->clearVariable(VA_KeepShortedDevs);
    }
    else if (!strcmp(name, "NoMrgShrt")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoMergeShorted, "");
        else
            CDvdb()->clearVariable(VA_NoMergeShorted);
    }
    else if (!strcmp(name, "NoMeas")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoMeasure, "");
        else
            CDvdb()->clearVariable(VA_NoMeasure);
    }
    else if (!strcmp(name, "UseCache")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_UseMeasurePrpty, "");
        else
            CDvdb()->clearVariable(VA_UseMeasurePrpty);
    }
    else if (!strcmp(name, "NoRdCache")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoReadMeasurePrpty, "");
        else
            CDvdb()->clearVariable(VA_NoReadMeasurePrpty);
    }
    else if (!strcmp(name, "DeltaSet")) {
        if (GRX->GetStatus(caller)) {
            const char *s = Es->sb_delta.get_string();
            double d;
            if (sscanf(s, "%lf", &d) == 1 && d >= 0.01)
                CDvdb()->setVariable(VA_RLSolverDelta, s);
            else
                GRX->Deselect(caller);
        }
        else
            CDvdb()->clearVariable(VA_RLSolverDelta);
    }
    else if (!strcmp(name, "TryTile")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_RLSolverTryTile, "");
        else
            CDvdb()->clearVariable(VA_RLSolverTryTile);
    }

    // Page 4.
    else if (!strcmp(name, "UseKeys")) {
        if (GRX->GetStatus(caller)) {
            const char *s = gtk_entry_get_text(GTK_ENTRY(Es->es_p4_flkeys));
            CDvdb()->setVariable(VA_FlattenPrefix, s);
        }
        else
            CDvdb()->clearVariable(VA_FlattenPrefix);
    }
    else if (!strcmp(name, "Opaque")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_ExtractOpaque, "");
        else
            CDvdb()->clearVariable(VA_ExtractOpaque);
    }
    else if (!strcmp(name, "Verbose")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_VerbosePromptline, "");
        else
            CDvdb()->clearVariable(VA_VerbosePromptline);
    }
    else if (!strcmp(name, "GlbSet")) {
        if (GRX->GetStatus(caller)) {
            const char *s = gtk_entry_get_text(GTK_ENTRY(Es->es_p4_glbex));
            CDvdb()->setVariable(VA_GlobalExclude, s);
        }
        else
            CDvdb()->clearVariable(VA_GlobalExclude);
    }
    else if (!strcmp(name, "NoPerm")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoPermute, "");
        else
            CDvdb()->clearVariable(VA_NoPermute);
    }
    else if (!strcmp(name, "MergePC")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_MergePhysContacts, "");
        else
            CDvdb()->clearVariable(VA_MergePhysContacts);
    }
    else if (!strcmp(name, "PrmFix")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_SubcPermutationFix, "");
        else
            CDvdb()->clearVariable(VA_SubcPermutationFix);
    }
}


// Static function.
void
sEs::es_val_changed(GtkWidget *caller, void*)
{
    if (!Es)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "ViaDepth")) {
        const char *s = Es->sb_vdepth.get_string();
        int n;
        if (sscanf(s, "%d", &n) == 1 && n >= 0) {
            if (n == EXT_DEF_VIA_SEARCH_DEPTH)
                CDvdb()->clearVariable(VA_ViaSearchDepth);
            else
                CDvdb()->setVariable(VA_ViaSearchDepth, s);
        }
    }
    else if (!strcmp(name, "Delta")) {
        ;
    }
    else if (!strcmp(name, "MaxPts")) {
        const char *s = Es->sb_maxpts.get_string();
        int n;
        if (sscanf(s, "%d", &n) == 1 && n >= 1000 && n <= 100000) {
            if (n == RLS_DEF_MAX_GRID)
                CDvdb()->clearVariable(VA_RLSolverMaxPoints);
            else
                CDvdb()->setVariable(VA_RLSolverMaxPoints, s);
        }
    }
    else if (!strcmp(name, "GridPts")) {
        const char *s = Es->sb_gridpts.get_string();
        int n;
        if (sscanf(s, "%d", &n) == 1 && n >= 10 && n <= 100000) {
            if (n == RLS_DEF_NUM_GRID)
                CDvdb()->clearVariable(VA_RLSolverGridPoints);
            else
                CDvdb()->setVariable(VA_RLSolverGridPoints, s);
        }
    }
    else if (!strcmp(name, "Loop")) {
        const char *s = Es->sb_loop.get_string();
        int n;
        if (sscanf(s, "%d", &n) == 1 && n >= 0 && n <= 1000000) {
            if (n == EXT_DEF_LVS_LOOP_MAX)
                CDvdb()->clearVariable(VA_MaxAssocLoops);
            else
                CDvdb()->setVariable(VA_MaxAssocLoops, s);
        }
    }
    else if (!strcmp(name, "Iters")) {
        const char *s = Es->sb_iters.get_string();
        int n;
        if (sscanf(s, "%d", &n) == 1 && n >= 10 && n <= 100000) {
            if (n == EXT_DEF_LVS_ITER_MAX)
                CDvdb()->clearVariable(VA_MaxAssocIters);
            else
                CDvdb()->setVariable(VA_MaxAssocIters, s);
        }
    }
}


// Static function.
void
sEs::es_gpinv_proc(GtkWidget *caller, void*)
{
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "0")) {
        CDvdb()->clearVariable(VA_GroundPlaneMethod);
        Es->es_gpmhst = 0;
    }
    else if (!strcmp(name, "1")) {
        CDvdb()->setVariable(VA_GroundPlaneMethod, name);
        Es->es_gpmhst = 1;
    }
    else if (!strcmp(name, "2")) {
        CDvdb()->setVariable(VA_GroundPlaneMethod, name);
        Es->es_gpmhst = 2;
    }
}


// Update the Device Block menu.
//
void
sEs::dev_menu_upd()
{
    GList *gl = gtk_container_children(GTK_CONTAINER(es_p2_menu));
    int cnt = 0;
    for (GList *l = gl; l; l = l->next, cnt++) {
        if (cnt > 3)  // ** skip first four entries **
            gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(gl);

    stringlist *dnames = EX()->listDevices();
    for (stringlist *sl = dnames; sl; sl = sl->next) {
        const char *t = sl->string;
        char *nm = lstring::gettok(&t);
        char *pf = lstring::gettok(&t);
        sDevDesc *d = EX()->findDevice(nm, pf);
        delete [] nm;
        delete [] pf;

        if (d) {
            GtkWidget *mi = gtk_menu_item_new_with_label(sl->string);
            gtk_widget_show(mi);
            gtk_signal_connect(GTK_OBJECT(mi), "activate",
                GTK_SIGNAL_FUNC(es_dev_menu_proc), d);
            gtk_menu_append(GTK_MENU(es_p2_menu), mi);
        }
    }
    dnames->free();
}


// Static function.
// Callback from the text editor popup.
//
bool
sEs::es_editsave(const char *fname, void*, XEtype type)
{
    if (type == XE_QUIT)
        unlink(fname);
    else if (type == XE_SAVE) {
        FILE *fp = filestat::open_file(fname, "r");
        if (!fp) {
            Log()->ErrorLog(mh::Initialization, filestat::error_msg());
            return (true);
        }
        bool ret = EX()->parseDevice(fp, true);
        fclose(fp);
        if (ret && Es)
            Es->dev_menu_upd();
    }
    return (true);
}


// Static function.
// Edit a Device Block.
//
void
sEs::es_dev_menu_proc(GtkWidget*, void *client_data, unsigned int type)
{
    if (type == 2) {
        // Undelete button
        EX()->addDevice(Es->es_devdesc);
        EX()->invalidateGroups();
        Es->es_devdesc = 0;
        gtk_widget_set_sensitive(Es->es_p2_undblk, false);
        Es->dev_menu_upd();
        return;
    }
    if (type == 1 && !client_data) {
        // delete button;
        return;
    }
    sDevDesc *d = (sDevDesc*)client_data;
    if (GRX->GetStatus(Es->es_p2_delblk)) {
        GRX->Deselect(Es->es_p2_delblk);
        if (d && EX()->removeDevice(d->name()->string(), d->prefix())) {
            d->set_next(0);
            delete Es->es_devdesc;
            Es->es_devdesc = d;
            gtk_widget_set_sensitive(Es->es_p2_undblk, true);
            Es->dev_menu_upd();
            EX()->invalidateGroups();
        }
        return;
    }
    char *fname = filestat::make_temp("xi");
    FILE *fp = filestat::open_file(fname, "w");
    if (!fp) {
        Log()->ErrorLog(mh::Initialization, filestat::error_msg());
        return;
    }
    if (d)
        d->print(fp);
    fclose(fp);
    DSPmainWbag(PopUpTextEditor(fname, es_editsave, d, false))
}

