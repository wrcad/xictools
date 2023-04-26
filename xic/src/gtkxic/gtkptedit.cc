
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
#include "sced.h"
#include "cd_lgen.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "dsp_inlines.h"
#include "dsp_layer.h"
#include "menu.h"
#include "promptline.h"
#include "gtkmain.h"
#include "gtkinterf/gtkspinbtn.h"


//---------------------------------------------------------------------------
// Pop-up interface for terminal/property editing.  This handles
// physical mode (TEDIT command).

// Help system keywords used:
//  xic:edtrm

namespace {
    namespace gtkptedit {
        struct sTE
        {
            sTE(GRobject, TermEditInfo*, void(*)(TermEditInfo*, CDsterm*), CDsterm*);
            ~sTE();

            GtkWidget *shell() { return (te_popup); }

            void update(TermEditInfo*, CDsterm*);

        private:
            void set_layername(const char *n)
                {
                    const char *nn = lstring::copy(n);
                    delete [] te_lname;
                    te_lname = nn;
                }

            static void te_cancel_proc(GtkWidget*, void*);
            static void te_action_proc(GtkWidget*, void*);
            static void te_menu_proc(GtkWidget*, void*);

            GRobject te_caller;
            GtkWidget *te_popup;
            GtkWidget *te_name;
            GtkWidget *te_layer;
            GtkWidget *te_flags;
            GtkWidget *te_fixed;
            GtkWidget *te_physgrp;
            GtkWidget *te_toindex_sb;
            void (*te_action)(TermEditInfo*, CDsterm*);
            CDsterm *te_term;
            const char *te_lname;

            GTKspinBtn te_toindex;
        };

        sTE *TE;
    }
}

using namespace gtkptedit;


void
cExt::PopUpPhysTermEdit(GRobject caller, ShowMode mode, TermEditInfo *tinfo,
    void(*action)(TermEditInfo*, CDsterm*), CDsterm *term, int x, int y)
{
    if (!GRX || !GTKmainwin::self())
        return;
    if (mode == MODE_OFF) {
        delete TE;
        return;
    }
    if (mode == MODE_UPD) {
        if (TE && tinfo)
            TE->update(tinfo, term);
        return;
    }
    if (TE) {
        // Once the pop-up is visible, reuse it when clicking on other
        // terminals.
        if (tinfo) {
            TE->update(tinfo, term);
            return;
        }
        PopUpPhysTermEdit(0, MODE_OFF, 0, 0, 0, 0, 0);
    }

    if (!tinfo)
        return;

    new sTE(caller, tinfo, action, term);
    if (!TE->shell()) {
        delete TE;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(TE->shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    int mwid, mhei;
    gtk_MonitorGeom(GTKmainwin::self()->Shell(), 0, 0, &mwid, &mhei);
    GtkRequisition req;
    gtk_widget_get_requisition(TE->shell(), &req);
    if (x + req.width > mwid)
        x = mwid - req.width;
    if (y + req.height > mhei)
        y = mhei - req.height;
    gtk_window_move(GTK_WINDOW(TE->shell()), x, y);
    gtk_widget_show(TE->shell());

    // OpenSuse-13.1 gtk-2.24.23 bug
    gtk_window_move(GTK_WINDOW(TE->shell()), x, y);
}
// End of cSced functions.


sTE::sTE(GRobject caller, TermEditInfo *tinfo, void(*action)(TermEditInfo*, CDsterm*),
    CDsterm *term)
{
    TE = this;
    te_caller = caller;
    te_popup = 0;
    te_name = 0;
    te_layer = 0;
    te_flags = 0;
    te_fixed = 0;
    te_physgrp = 0;
    te_action = action;
    te_term = term;
    te_lname = 0;
    te_toindex_sb = 0;

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
    GtkWidget *label = gtk_label_new("Edit Terminal Properties");
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
        G_CALLBACK(te_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Name Entry
    //
    frame = gtk_frame_new("Terminal Name");
    gtk_widget_show(frame);

    label = gtk_label_new("");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_container_add(GTK_CONTAINER(frame), label);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    rowcnt++;
    te_name = label;

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

    GtkWidget *entry = gtk_combo_box_text_new();
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

    label = gtk_label_new("Current Flags");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);

    te_flags = gtk_label_new("");
    gtk_widget_show(te_flags);
    gtk_misc_set_padding(GTK_MISC(te_flags), 2, 2);
    gtk_table_attach(GTK_TABLE(form), te_flags, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
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
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(te_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_button_new_with_label("Next");
    gtk_widget_set_name(button, "Next");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(te_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    GtkWidget *vsep = gtk_vseparator_new();
    gtk_widget_show(vsep);
    gtk_box_pack_start(GTK_BOX(row), vsep, true, true, 0);

    te_toindex_sb = te_toindex.init(0, 0, P_NODE_MAX_INDEX, 0);
    gtk_box_pack_end(GTK_BOX(row), te_toindex_sb, false, false, 0);

    button = gtk_button_new_with_label("To Index");
    gtk_widget_set_name(button, "ToIndex");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(te_action_proc), 0);
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
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(te_action_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // Pressing Enter in the entry presses Apply.
    gtk_widget_set_can_default(button, true);
    gtk_window_set_default(GTK_WINDOW(te_popup), button);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(te_cancel_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    update(tinfo, term);
}


sTE::~sTE()
{
    TE = 0;
    if (te_caller)
        GRX->SetStatus(te_caller, false);
    if (te_action)
        (*te_action)(0, te_term);
    if (te_popup)
        gtk_widget_destroy(te_popup);
    delete [] te_lname;
}


void
sTE::update(TermEditInfo *tinfo, CDsterm *term)
{
    const char *name = tinfo->name();
    if (!name)
        name = "";
    gtk_label_set_text(GTK_LABEL(te_name), name);

    gtk_widget_set_sensitive(te_popup, (term != 0));

    gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(
        GTK_COMBO_BOX(te_layer))));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(te_layer), "any layer");
    CDl *ld;
    CDlgen lgen(Physical);
    while ((ld = lgen.next()) != 0) {
        if (!ld->isRouting())
            continue;
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(te_layer), 
            ld->name());
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(te_layer), 0);
    g_signal_connect(G_OBJECT(te_layer), "changed",
        G_CALLBACK(te_menu_proc), 0);

    bool hset = false;
    if (tinfo->layer_name() && *tinfo->layer_name()) {
        int cnt = 1;
        lgen = CDlgen(Physical);
        while ((ld = lgen.next()) != 0) {
            if (!ld->isRouting())
                continue;
            if (!strcmp(ld->name(), tinfo->layer_name())) {
                gtk_combo_box_set_active(GTK_COMBO_BOX(te_layer), cnt);
                set_layername(ld->name());
                hset = true;
                break;
            }
            cnt++;
        }
    }
    if (!hset) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(te_layer), 0);
        set_layername(0);
    }

    sLstr lstr;
    unsigned int mask = TE_UNINIT | TE_FIXED;
    for (FlagDef *f = TermFlags; f->name; f++) {
        if (tinfo->flags() & f->value & mask) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add(f->name);
        }
    }
    if (!lstr.string())
        lstr.add("none");
    gtk_label_set_text(GTK_LABEL(te_flags), lstr.string());

    GRX->SetStatus(te_fixed, tinfo->has_flag(TE_FIXED));
    CDp_snode *ps0 = (CDp_snode*)CurCell(Electrical)->prpty(P_NODE);
    for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
        if (ps->cell_terminal() == term) {
            te_toindex.set_value(ps->index());
            break;
        }
    }
    te_term = term;
}


// Static function.
void
sTE::te_cancel_proc(GtkWidget*, void*)
{
    EX()->PopUpPhysTermEdit(0, MODE_OFF, 0, 0, 0, 0, 0);
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
            unsigned int f = 0;
            if (GRX->GetStatus(TE->te_fixed))
                f |= TE_FIXED;
            CDp_snode *ps = TE->te_term->node_prpty();
            if (!ps)
                return;
            TermEditInfo tinfo(Tstring(ps->term_name()), TE->te_lname,
                ps->index(), f, true);
            (*TE->te_action)(&tinfo, TE->te_term);
            EX()->PopUpPhysTermEdit(0, MODE_UPD, &tinfo, 0, TE->te_term,
                0, 0);
        }
        else
            EX()->PopUpPhysTermEdit(0, MODE_OFF, 0, 0, 0, 0, 0);
        return;
    }
    if (!strcmp(name, "Prev")) {
        CDp_snode *ps0 = (CDp_snode*)CurCell(Electrical)->prpty(P_NODE);
        int topix = -1;
        int curix = -1;
        for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
            if (ps->cell_terminal() == TE->te_term) 
                curix = ps->index();
            if ((int)ps->index() > topix)
                topix = ps->index();
        }
        if (topix < 0 || curix < 0)
            return;
        int nix = curix - 1;
        while (nix >= 0) {
            for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
                if ((int)ps->index() == nix) {
                    CDsterm *term = ps->cell_terminal();
                    if (term) {
                        EX()->editTermsPush(term);
                        return;
                    }
                    break;
                }
            }
            nix--;
        }
        nix = topix;
        while (nix >= curix) {
            for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
                if ((int)ps->index() == nix) {
                    CDsterm *term = ps->cell_terminal();
                    if (term) {
                        EX()->editTermsPush(term);
                        return;
                    }
                    break;
                }
            }
            nix--;
        }
        return;
    }
    if (!strcmp(name, "Next")) {
        CDp_snode *ps0 = (CDp_snode*)CurCell(Electrical)->prpty(P_NODE);
        int topix = -1;
        int curix = -1;
        for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
            if (ps->cell_terminal() == TE->te_term) 
                curix = ps->index();
            if ((int)ps->index() > topix)
                topix = ps->index();
        }
        if (topix < 0 || curix < 0)
            return;
        int nix = curix + 1;
        while (nix <= topix) {
            for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
                if ((int)ps->index() == nix) {
                    CDsterm *term = ps->cell_terminal();
                    if (term) {
                        EX()->editTermsPush(term);
                        return;
                    }
                    break;
                }
            }
            nix++;
        }
        nix = 0;
        while (nix <= curix) {
            for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
                if ((int)ps->index() == nix) {
                    CDsterm *term = ps->cell_terminal();
                    if (term) {
                        EX()->editTermsPush(term);
                        return;
                    }
                    break;
                }
            }
            nix++;
        }
        return;
    }
    if (!strcmp(name, "ToIndex")) {
        int indx = TE->te_toindex.get_value_as_int();
        CDp_snode *ps0 = (CDp_snode*)CurCell(Electrical)->prpty(P_NODE);
        int topix = -1;
        int curix = -1;
        for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
            if (ps->cell_terminal() == TE->te_term) 
                curix = ps->index();
            if ((int)ps->index() > topix)
                topix = ps->index();
        }
        if (topix < 0 || curix < 0)
            return;
        if (indx == curix)
            return;
        for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
            if ((int)ps->index() == indx) {
                CDsterm *term = ps->cell_terminal();
                if (term) {
                    EX()->editTermsPush(term);
                    return;
                }
                break;
            }
        }
        int xx, yy;
        Menu()->PointerRootLoc(&xx, &yy);
        PL()->FlashMessageHereV(xx, yy, "No terminal for index %d", indx);
    }
}


// Static function.
void
sTE::te_menu_proc(GtkWidget *caller, void*)
{
    if (TE) {
        if (gtk_combo_box_get_active(GTK_COMBO_BOX(caller)) == 0) {
            // any_layer
            TE->set_layername(0);
            return;
        }
        char *lname = gtk_combo_box_text_get_active_text(
            GTK_COMBO_BOX_TEXT(caller));
        TE->set_layername(lname);
        g_free(lname);
    }
}

