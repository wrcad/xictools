
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
 $Id: gtktedit.cc,v 5.20 2015/07/31 22:37:03 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "sced.h"
#include "cd_lgen.h"
#include "cd_terminal.h"
#include "dsp_inlines.h"
#include "dsp_layer.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkspinbtn.h"
#include "menu.h"
#include "errorlog.h"


//---------------------------------------------------------------------------
// Pop-up interface for terminal/property editing.  This handles
// electrical mode (SUBCT command).

// Help system keywords used:
//  xic:edtrm

namespace {
    namespace gtktedit {
        struct sTE
        {
            sTE(GRobject, te_info_t*, void(*)(te_info_t*, CDp*), CDp*);
            ~sTE();

            GtkWidget *shell() { return (te_popup); }

            void update(te_info_t*, CDp*);

        private:
            static void te_cancel_proc(GtkWidget*, void*);
            static void te_action_proc(GtkWidget*, void*);
            static void te_menu_proc(GtkWidget*, void*);

            GRobject te_caller;
            GtkWidget *te_popup;
            GtkWidget *te_top_lab;
            GtkWidget *te_index_lab;
            GtkWidget *te_index_sb;
            GtkWidget *te_name;
            GtkWidget *te_netex_lab;
            GtkWidget *te_netex;
            GtkWidget *te_layer;
            GtkWidget *te_fixed;
            GtkWidget *te_phys;
            GtkWidget *te_physgrp;
            GtkWidget *te_byname;
            GtkWidget *te_scinvis;
            GtkWidget *te_syinvis;
            GtkWidget *te_bitsgrp;
            GtkWidget *te_toindex_sb;
            GtkWidget *te_crtbits;
            GtkWidget *te_ordbits;
            void (*te_action)(te_info_t*, CDp*);
            CDp *te_prp;
            const char *te_lname;
            bool te_bterm;

            GTKspinBtn te_index;
            GTKspinBtn te_toindex;
        };

        sTE *TE;
    }
}

using namespace gtktedit;


void
cSced::PopUpTermEdit(GRobject caller, ShowMode mode, te_info_t *tinfo,
    void(*action)(te_info_t*, CDp*), CDp *prp, int x, int y)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete TE;
        return;
    }
    if (mode == MODE_UPD) {
        if (TE && tinfo)
            TE->update(tinfo, prp);
        return;
    }
    if (TE) {
        // Once the pop-up is visible, reuse it when clicking on other
        // terminals.
        if (tinfo) {
            TE->update(tinfo, prp);
            return;
        }
        PopUpTermEdit(0, MODE_OFF, 0, 0, 0, 0, 0);
    }

    if (!tinfo)
        return;

    new sTE(caller, tinfo, action, prp);
    if (!TE->shell()) {
        delete TE;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(TE->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    int mwid, mhei;
    MonitorGeom(mainBag()->Shell(), 0, 0, &mwid, &mhei);
    if (x + TE->shell()->requisition.width > mwid)
        x = mwid - TE->shell()->requisition.width;
    if (y + TE->shell()->requisition.height > mhei)
        y = mhei - TE->shell()->requisition.height;
    gtk_widget_set_uposition(TE->shell(), x, y);
    gtk_widget_show(TE->shell());

    // OpenSuse-13.1 gtk-2.24.23 bug
    gtk_widget_set_uposition(TE->shell(), x, y);
}
// End of cSced functions.


sTE::sTE(GRobject caller, te_info_t *tinfo, void(*action)(te_info_t*, CDp*),
    CDp *prp)
{
    TE = this;
    te_caller = caller;
    te_popup = 0;
    te_top_lab = 0;
    te_index_lab = 0;
    te_index_sb = 0;
    te_name = 0;
    te_netex_lab = 0;
    te_netex = 0;
    te_layer = 0;
    te_fixed = 0;
    te_phys = 0;
    te_physgrp = 0;
    te_byname = 0;
    te_scinvis = 0;
    te_syinvis = 0;
    te_action = action;
    te_prp = prp;
    te_lname = 0;
    te_bitsgrp = 0;
    te_toindex_sb = 0;
    te_crtbits = 0;
    te_ordbits = 0;
    te_bterm = false;

    te_popup = gtk_NewPopup(0, "Terminal Edit", te_cancel_proc, 0);
    if (!te_popup)
        return;

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(te_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("");
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
        GTK_SIGNAL_FUNC(te_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    te_top_lab = label;

    //
    // Bus Term Indices
    //
    te_index_lab = gtk_label_new("Term Index");
    gtk_widget_show(te_index_lab);
    gtk_misc_set_padding(GTK_MISC(te_index_lab), 2, 2);

    gtk_table_attach(GTK_TABLE(form), te_index_lab, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    te_index_sb = te_index.init(0, 0, P_NODE_MAX_INDEX, 0);

    gtk_table_attach(GTK_TABLE(form), te_index_sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Name Entry
    //
    label = gtk_label_new("Terminal Name");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);

    GtkWidget *entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    rowcnt++;
    te_name = entry;

    //
    // NetEx Entry
    //
    label = gtk_label_new("Net Expression");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    te_netex_lab = label;

    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    rowcnt++;
    te_netex = entry;

    // Pressing Enter in the entry presses Apply.
    gtk_window_set_focus(GTK_WINDOW(te_popup), te_name);
    gtk_entry_set_activates_default(GTK_ENTRY(te_name), true);

    //
    // Has Physical Term?
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    te_phys = gtk_check_button_new_with_label("Has physical terminal");
    gtk_widget_set_name(te_phys, "phys");
    gtk_widget_show(te_phys);
    gtk_signal_connect(GTK_OBJECT(te_phys), "clicked",
        GTK_SIGNAL_FUNC(te_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), te_phys, true, true, 0);

    button = gtk_button_new_with_label("Delete");
    gtk_widget_set_name(button, "Delete");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(te_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Physical Group
    //
    frame = gtk_frame_new("Physical");
    gtk_widget_show(frame);
    te_physgrp = frame;

    GtkWidget *table = gtk_table_new(1, 1, false);
    gtk_widget_show(table);
    gtk_container_add(GTK_CONTAINER(frame), table);
    int rc = 0;

    //
    // Layer Binding
    //
    label = gtk_label_new("Layer Binding");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, rc, rc+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "layer");
    gtk_widget_show(entry);
    te_layer = entry;

    gtk_table_attach(GTK_TABLE(table), entry, 1, 2, rc, rc+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rc++;

    te_fixed = gtk_check_button_new_with_label(
        "Location locked by user placement");
    gtk_widget_set_name(te_fixed, "LocSet");
    gtk_widget_show(te_fixed);
    gtk_table_attach(GTK_TABLE(table), te_fixed, 0, 2, rc, rc+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rc++;

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    te_byname = gtk_check_button_new_with_label(
        "Set connect by name only");
    gtk_widget_set_name(te_byname, "byname");
    gtk_widget_show(te_byname);
    gtk_table_attach(GTK_TABLE(form), te_byname, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    te_scinvis = gtk_check_button_new_with_label(
        "Set terminal invisible in schematic");
    gtk_widget_set_name(te_scinvis, "scinvis");
    gtk_widget_show(te_scinvis);
    gtk_table_attach(GTK_TABLE(form), te_scinvis, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    te_syinvis = gtk_check_button_new_with_label(
        "Set terminal invisible in symbol");
    gtk_widget_set_name(te_syinvis, "syinvis");
    gtk_widget_show(te_syinvis);
    gtk_table_attach(GTK_TABLE(form), te_syinvis, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    //
    // Bterm bits buttons
    //
    frame = gtk_frame_new("Bus Term Bits");
    gtk_widget_show(frame);
    te_bitsgrp = frame;

    GtkWidget *col = gtk_vbox_new(false, 0);
    gtk_widget_show(col);
    gtk_container_add(GTK_CONTAINER(frame), col);

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    gtk_box_pack_start(GTK_BOX(col), row, false, false, 0);

    button = gtk_button_new_with_label("Check/Create Bits");
    gtk_widget_set_name(button, "Bits");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(te_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    te_crtbits = button;

    button = gtk_button_new_with_label("Reorder to Index");
    gtk_widget_set_name(button, "Reord");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(te_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    te_ordbits = button;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    gtk_box_pack_start(GTK_BOX(col), row, false, false, 0);

    button = gtk_button_new_with_label("Schem Vis");
    gtk_widget_set_name(button, "ScVis");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(te_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_button_new_with_label("Invis");
    gtk_widget_set_name(button, "ScInvis");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(te_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_button_new_with_label("Symbol Vis");
    gtk_widget_set_name(button, "SyVis");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(te_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_button_new_with_label("Invis");
    gtk_widget_set_name(button, "SyInvis");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(te_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Prev, Next buttons
    //
    GtkWidget *hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);
    gtk_table_attach(GTK_TABLE(form), hsep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    button = gtk_button_new_with_label("Prev");
    gtk_widget_set_name(button, "Prev");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(te_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_button_new_with_label("Next");
    gtk_widget_set_name(button, "Next");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(te_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    GtkWidget *vsep = gtk_vseparator_new();
    gtk_widget_show(vsep);
    gtk_box_pack_start(GTK_BOX(row), vsep, true, true, 0);

    te_toindex_sb = te_toindex.init(0, 0, P_NODE_MAX_INDEX, 0);
    gtk_box_pack_end(GTK_BOX(row), te_toindex_sb, false, false, 0);

    button = gtk_button_new_with_label("To Index");
    gtk_widget_set_name(button, "ToIndex");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(te_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Apply, Dismiss buttons
    //
    button = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(button, "Apply");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(te_action_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // Pressing Enter in the entry presses Apply.
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_window_set_default(GTK_WINDOW(te_popup), button);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(te_cancel_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    update(tinfo, prp);
}


sTE::~sTE()
{
    TE = 0;
    if (te_caller)
        GRX->SetStatus(te_caller, false);
    if (te_action)
        (*te_action)(0, te_prp);
    if (te_popup)
        gtk_widget_destroy(te_popup);
}


void
sTE::update(te_info_t *tinfo, CDp *prp)
{
    const char *name = tinfo->name();
    if (!name)
        name = "";
    gtk_entry_set_text(GTK_ENTRY(te_name), name);
    const char *netex = tinfo->netex();
    if (!netex)
        netex = "";
    gtk_entry_set_text(GTK_ENTRY(te_netex), netex);

    te_bterm = tinfo->has_bterm();

    gtk_widget_set_sensitive(te_popup, (prp != 0));

    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "layer");
    const char *anyname = "any layer";
    GtkWidget *mi = gtk_menu_item_new_with_label(anyname);
    gtk_widget_set_name(mi, anyname);
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(te_menu_proc), 0);
    CDl *ld;
    CDlgen lgen(Physical);
    while ((ld = lgen.next()) != 0) {
        if (!ld->isRouting())
            continue;
        mi = gtk_menu_item_new_with_label(ld->name());
        gtk_widget_set_name(mi, ld->name());
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(te_menu_proc), (void*)ld->name());
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(te_layer), menu);

    te_index.set_value(tinfo->index());
    te_toindex.set_value(tinfo->index());
    if (te_bterm) {
        gtk_label_set_text(GTK_LABEL(te_top_lab),
            "Edit Bus Terminal Properties");
        gtk_widget_show(te_netex_lab);
        gtk_widget_show(te_netex);
        gtk_widget_hide(te_phys);
        gtk_widget_hide(te_physgrp);
        gtk_widget_hide(te_byname);
        gtk_widget_show(te_bitsgrp);

        // These requires schematic display and a terminal name.
        CDs *sd = CurCell(Electrical);
        if (sd && sd->isSymbolic()) {
            gtk_widget_set_sensitive(te_crtbits, false);
            gtk_widget_set_sensitive(te_ordbits, false);
        }
        else {
            if (prp && ((CDp_bsnode*)prp)->has_name()) {
                gtk_widget_set_sensitive(te_crtbits, true);
                gtk_widget_set_sensitive(te_ordbits, true);
            }
            else {
                gtk_widget_set_sensitive(te_crtbits, false);
                gtk_widget_set_sensitive(te_ordbits, false);
            }
        }
        GRX->SetStatus(te_scinvis, tinfo->has_flag(TE_SCINVIS));
        GRX->SetStatus(te_syinvis, tinfo->has_flag(TE_SYINVIS));
    }
    else {
        gtk_label_set_text(GTK_LABEL(te_top_lab),
            "Edit Terminal Properties");
        gtk_widget_hide(te_netex_lab);
        gtk_widget_hide(te_netex);
        gtk_widget_show(te_phys);
        gtk_widget_show(te_physgrp);
        gtk_widget_show(te_byname);
        gtk_widget_hide(te_bitsgrp);

        gtk_widget_set_sensitive(te_phys, DSP()->CurMode() == Electrical);

        bool hset = false;
        if (tinfo->layer_name() && *tinfo->layer_name()) {
            int cnt = 1;
            lgen = CDlgen(Physical);
            while ((ld = lgen.next()) != 0) {
                if (!ld->isRouting())
                    continue;
                if (!strcmp(ld->name(), tinfo->layer_name())) {
                    gtk_option_menu_set_history(GTK_OPTION_MENU(te_layer),
                        cnt);
                    hset = true;
                    te_lname = ld->name();
                    break;
                }
                cnt++;
            }
        }
        if (!hset) {
            gtk_option_menu_set_history(GTK_OPTION_MENU(te_layer), 0);
            te_lname = 0;
        }

        GRX->SetStatus(te_fixed, tinfo->has_flag(TE_FIXED));
        GRX->SetStatus(te_phys, tinfo->has_phys());
        GRX->SetStatus(te_byname, tinfo->has_flag(TE_BYNAME));
        GRX->SetStatus(te_scinvis, tinfo->has_flag(TE_SCINVIS));
        GRX->SetStatus(te_syinvis, tinfo->has_flag(TE_SYINVIS));
        gtk_widget_set_sensitive(te_physgrp, tinfo->has_phys());
    }
    te_prp = prp;
}


// Static function.
void
sTE::te_cancel_proc(GtkWidget*, void*)
{
    SCD()->PopUpTermEdit(0, MODE_OFF, 0, 0, 0, 0, 0);
}


// Static function.
void
sTE::te_action_proc(GtkWidget *caller, void*)
{
    if (!TE)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:edtrm"))
        return;
    }
    if (!strcmp(name, "Apply")) {
        if (TE->te_action) {
            name = gtk_entry_get_text(GTK_ENTRY(TE->te_name));
            unsigned int ix = TE->te_index.get_value_as_int();
            if (TE->te_bterm) {
                const char *netex = gtk_entry_get_text(GTK_ENTRY(TE->te_netex));
                unsigned int f = 0;
                if (GRX->GetStatus(TE->te_scinvis))
                    f |= TE_SCINVIS;
                if (GRX->GetStatus(TE->te_syinvis))
                    f |= TE_SYINVIS;

                te_info_t tinfo(name, ix, f, netex);
                (*TE->te_action)(&tinfo, TE->te_prp);
                SCD()->PopUpTermEdit(0, MODE_UPD, &tinfo, 0, TE->te_prp, 0, 0);
            }
            else {
                unsigned int f = 0;
                if (GRX->GetStatus(TE->te_fixed))
                    f |= TE_FIXED;
                if (GRX->GetStatus(TE->te_byname))
                    f |= TE_BYNAME;
                if (GRX->GetStatus(TE->te_scinvis))
                    f |= TE_SCINVIS;
                if (GRX->GetStatus(TE->te_syinvis))
                    f |= TE_SYINVIS;

                te_info_t tinfo(name, TE->te_lname, ix, f,
                    GRX->GetStatus(TE->te_phys));
                (*TE->te_action)(&tinfo, TE->te_prp);
                SCD()->PopUpTermEdit(0, MODE_UPD, &tinfo, 0, TE->te_prp, 0, 0);
            }
        }
        else
            SCD()->PopUpTermEdit(0, MODE_OFF, 0, 0, 0, 0, 0);
        return;
    }
    if (!strcmp(name, "phys")) {
        bool state = GRX->GetStatus(TE->te_phys);
        gtk_widget_set_sensitive(TE->te_physgrp, state);
    }
    if (!strcmp(name, "Prev")) {
        int indx;
        bool was_bt;
        if (SCD()->subcircuitEditTerm(&indx, &was_bt)) {
            if (!was_bt) {
                if (SCD()->subcircuitSetEditTerm(indx, true))
                    return;
            }
            indx--;
            if (SCD()->subcircuitSetEditTerm(indx, false))
                return;
            if (SCD()->subcircuitSetEditTerm(indx, true))
                return;
        }
    }
    if (!strcmp(name, "Next")) {
        int indx;
        bool was_bt;
        if (SCD()->subcircuitEditTerm(&indx, &was_bt)) {
            if (was_bt) {
                if (SCD()->subcircuitSetEditTerm(indx, false))
                    return;
            }
            indx++;
            if (SCD()->subcircuitSetEditTerm(indx, true))
                return;
            if (SCD()->subcircuitSetEditTerm(indx, false))
                return;
        }
    }
    if (!strcmp(name, "ToIndex")) {
        int indx = TE->te_toindex.get_value_as_int();
        if (SCD()->subcircuitSetEditTerm(indx, true))
            return;
        if (SCD()->subcircuitSetEditTerm(indx, false))
            return;
    }
    if (!strcmp(name, "Delete")) {
        SCD()->subcircuitDeleteTerm();
        return;
    }
    if (!strcmp(name, "Bits")) {
        if (!SCD()->subcircuitBits(false)) {
            Log()->ErrorLogV(mh::Processing,
                "Operation failed: %s.", Errs()->get_error());
        }
        return;
    }
    if (!strcmp(name, "Reord")) {
        if (!SCD()->subcircuitBits(true)) {
            Log()->ErrorLogV(mh::Processing,
                "Operation failed: %s.", Errs()->get_error());
        }
        return;
    }
    if (!strcmp(name, "ScVis")) {
        SCD()->subcircuitBitsVisible(TE_SCINVIS);
        return;
    }
    if (!strcmp(name, "ScInvis")) {
        SCD()->subcircuitBitsInvisible(TE_SCINVIS);
        return;
    }
    if (!strcmp(name, "SyVis")) {
        SCD()->subcircuitBitsVisible(TE_SYINVIS);
        return;
    }
    if (!strcmp(name, "SyInvis")) {
        SCD()->subcircuitBitsInvisible(TE_SYINVIS);
        return;
    }
}


// Static function.
void
sTE::te_menu_proc(GtkWidget*, void *client_data)
{
    if (TE)
        TE->te_lname = (char*)client_data;
}

