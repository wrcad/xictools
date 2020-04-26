
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
#include "fio_oasis.h"
#include "fio_oas_sort.h"
#include "fio_oas_reps.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkcv.h"
#include "gtkinterf/gtkspinbtn.h"


//-------------------------------------------------------------------------
// Pop-up for advanced/obscure OASIS writing features.
//
// Help system keywords used:
//  xic:oasadv

namespace {
    namespace gtkoasis {
        struct sOas
        {
            sOas(GRobject);
            ~sOas();

            void update();

            GtkWidget *shell() { return (oas_popup); }
            GRobject call_btn() { return (oas_caller); }

        private:
            void restore_defaults();
            void set_repvar();
            static void oas_cancel_proc(GtkWidget*, void*);
            static void oas_action(GtkWidget*, void*);
            static void oas_val_changed(GtkWidget*, void*);
            static void oas_pmask_menu_proc(GtkWidget*, void*);

            GRobject oas_caller;
            GtkWidget *oas_popup;
            GtkWidget *oas_notrap;
            GtkWidget *oas_wtob;
            GtkWidget *oas_rwtop;
            GtkWidget *oas_nogcd;
            GtkWidget *oas_oldsort;
            GtkWidget *oas_pmask;

            GtkWidget *oas_objc;
            GtkWidget *oas_objb;
            GtkWidget *oas_objp;
            GtkWidget *oas_objw;
            GtkWidget *oas_objl;
            GtkWidget *oas_def;
            GtkWidget *oas_noruns;
            GtkWidget *oas_noarrs;
            GtkWidget *oas_nosim;

            int oas_lastm;
            int oas_lasta;
            int oas_lastt;

            GTKspinBtn sb_entm;
            GTKspinBtn sb_enta;
            GTKspinBtn sb_entx;
            GTKspinBtn sb_entt;
        };

        sOas *Oas;
    }

    const char *pmaskvals[] =
    {
        "mask none",
        "mask XIC_PROPERTIES 7012",
        "mask XIC_LABEL",
        "mask 7012 and XIC_LABEL",
        "mask all properties",
        0
    };
}

using namespace gtkoasis;


void
cConvert::PopUpOasAdv(GRobject caller, ShowMode mode, int x, int y)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Oas;
        return;
    }
    if (mode == MODE_UPD) {
        if (Oas)
            Oas->update();
        return;
    }
    if (Oas) {
        if (caller && caller != Oas->call_btn())
            GRX->Deselect(caller);
        return;
    }

    new sOas(caller);
    if (!Oas->shell()) {
        delete Oas;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Oas->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    int mwid;
    MonitorGeom(mainBag()->Shell(), 0, 0, &mwid, 0);
    if (x + Oas->shell()->requisition.width > mwid)
        x = mwid - Oas->shell()->requisition.width;
    gtk_widget_set_uposition(Oas->shell(), x, y);
    gtk_widget_show(Oas->shell());

    // OpenSuse 13.1 gtk-2.24.23 bug
    gtk_widget_set_uposition(Oas->shell(), x, y);
}


sOas::sOas(GRobject c)
{
    Oas = this;
    oas_caller = c;
    oas_popup = 0;
    oas_notrap = 0;
    oas_wtob = 0;
    oas_rwtop = 0;
    oas_nogcd = 0;
    oas_oldsort = 0;
    oas_pmask = 0;
    oas_objc = 0;
    oas_objb = 0;
    oas_objp = 0;
    oas_objw = 0;
    oas_objl = 0;
    oas_def = 0;
    oas_noruns = 0;
    oas_noarrs = 0;
    oas_nosim = 0;
    oas_lastm = REP_RUN_MIN;
    oas_lasta = REP_ARRAY_MIN;
    oas_lastt = REP_MAX_REPS;

    oas_popup = gtk_NewPopup(0, "Advanced OASIS Export Parameters",
        oas_cancel_proc, 0);
    if (!oas_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(oas_popup), false);

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(oas_popup), form);
    int rowcnt = 0;

    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    GtkWidget *button = gtk_check_button_new_with_label(
        "Don't write trapezoid records");
    gtk_widget_set_name(button, "notrap");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    oas_notrap = button;

    button = gtk_button_new_with_label("Help");
    gtk_widget_show(button);
    gtk_widget_set_name(button, "Help");
    gtk_box_pack_start(GTK_BOX(row), button, false, false, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label(
        "Convert Wire to Box records when possible");
    gtk_widget_set_name(button, "wtob");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    oas_wtob = button;

    button = gtk_check_button_new_with_label(
        "Convert rounded-end Wire records to Poly records");
    gtk_widget_set_name(button, "rwtop");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    oas_rwtop = button;

    button = gtk_check_button_new_with_label(
        "Skip GCD check");
    gtk_widget_set_name(button, "nogcd");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    oas_nogcd = button;

    button = gtk_check_button_new_with_label(
        "Use alternate modal sort algorithm");
    gtk_widget_set_name(button, "oldsort");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    oas_oldsort = button;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    GtkWidget *label = gtk_label_new("Property masking");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    GtkWidget *entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "pmask");
    gtk_widget_show(entry);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "overmenu");
    for (int i = 0; pmaskvals[i]; i++) {
        GtkWidget *mi = gtk_menu_item_new_with_label(pmaskvals[i]);
        gtk_widget_set_name(mi, pmaskvals[i]);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(oas_pmask_menu_proc), (void*)pmaskvals[i]);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    gtk_box_pack_start(GTK_BOX(row), entry, true, true, 0);
    oas_pmask = entry;

    gtk_table_attach(GTK_TABLE(form), row, 0, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Repetition Finder Configuration");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label("Restore Defaults");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);
    oas_def = button;
    gtk_table_attach(GTK_TABLE(form), button, 2, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Repetition Finder Configuration

    GtkWidget *vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);
    button = gtk_check_button_new_with_label("Cells");
    gtk_widget_set_name(button, "Cells");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, false, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);
    oas_objc = button;

    button = gtk_check_button_new_with_label("Boxes");
    gtk_widget_set_name(button, "Boxes");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, false, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);
    oas_objb = button;

    button = gtk_check_button_new_with_label("Polys");
    gtk_widget_set_name(button, "Polys");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, false, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);
    oas_objp = button;

    button = gtk_check_button_new_with_label("Wires");
    gtk_widget_set_name(button, "Wires");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, false, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);
    oas_objw = button;

    button = gtk_check_button_new_with_label("Labels");
    gtk_widget_set_name(button, "Labels");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, false, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);
    oas_objl = button;

    GtkWidget *frame = gtk_frame_new("Objects");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, rowcnt, rowcnt+5,
        (GtkAttachOptions)0,
        (GtkAttachOptions)0, 2, 2);

    //
    // Entry areas
    //
    label = gtk_label_new("Run minimum");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_toggle_button_new_with_label("None");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);
    oas_noruns = button;
    gtk_table_attach(GTK_TABLE(form), button,
        2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *sb = sb_entm.init(REP_RUN_MIN, REP_RUN_MIN, REP_RUN_MAX, 0);
    sb_entm.connect_changed(GTK_SIGNAL_FUNC(oas_val_changed), 0);
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 3, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Array minimum");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_toggle_button_new_with_label("None");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);
    oas_noarrs = button;
    gtk_table_attach(GTK_TABLE(form), button,
        2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)0, 2, 2);

    sb = sb_enta.init(REP_ARRAY_MIN, REP_ARRAY_MIN, REP_ARRAY_MAX, 0);
    sb_enta.connect_changed(GTK_SIGNAL_FUNC(oas_val_changed), 0);
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 3, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Max different objects");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_entx.init(REP_MAX_ITEMS, REP_MAX_ITEMS_MIN, REP_MAX_ITEMS_MAX, 0);
    sb_entx.connect_changed(GTK_SIGNAL_FUNC(oas_val_changed), 0);
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 3, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Max identical objects");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_toggle_button_new_with_label("None");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_action), 0);
    oas_nosim = button;
    gtk_table_attach(GTK_TABLE(form), button,
        2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)0, 2, 2);

    sb = sb_entt.init(REP_MAX_REPS, REP_MAX_REPS_MIN,
        REP_MAX_REPS_MAX, 0);
    sb_entt.connect_changed(GTK_SIGNAL_FUNC(oas_val_changed), 0);
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 3, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(oas_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(oas_popup), button);

    // Constrain overall widget width so title text isn't truncated.
    gtk_widget_set_size_request(oas_popup, 360, -1);

    update();
}


sOas::~sOas()
{
    Oas = 0;
    if (oas_caller)
        GRX->Deselect(oas_caller);
    if (oas_popup)
        gtk_widget_destroy(oas_popup);
}


namespace {
    // Struct to hold repetition finder state while parsing property
    // string.
    //
    struct rp_state
    {
        rp_state()
        {
            cf = true;
            bf = true;
            pf = true;
            wf = true;
            lf = true;
            mval = REP_RUN_MIN;
            aval = REP_ARRAY_MIN;
            xval = REP_MAX_ITEMS;
            tval = REP_MAX_REPS;
            mnone = false;
            anone = false;
            tnone = false;
            oflgs = 0;
        }

        bool cf, bf, pf, wf, lf;
        int mval, aval, xval, tval;
        bool mnone, anone, tnone;
        int oflgs;
    };
}


void
sOas::update()
{
    GRX->SetStatus(oas_notrap, CDvdb()->getVariable(VA_OasWriteNoTrapezoids));
    GRX->SetStatus(oas_wtob, CDvdb()->getVariable(VA_OasWriteWireToBox));
    GRX->SetStatus(oas_rwtop, CDvdb()->getVariable(VA_OasWriteRndWireToPoly));
    GRX->SetStatus(oas_nogcd, CDvdb()->getVariable(VA_OasWriteNoGCDcheck));
    GRX->SetStatus(oas_oldsort, CDvdb()->getVariable(VA_OasWriteUseFastSort));

    const char *str = CDvdb()->getVariable(VA_OasWritePrptyMask);
    int nn;
    if (!str)
        nn = 0;
    else if (!*str)
        nn = 3;
    else if (isdigit(*str))
        nn = (atoi(str) & 0x3);
    else
        nn = 4;
    gtk_option_menu_set_history(GTK_OPTION_MENU(oas_pmask), nn);

    const char *s = CDvdb()->getVariable(VA_OasWriteRep);
    if (s) {
        // The property is set, update the backup string if necessary.
        if (!cvofmt_t::rep_string() || strcmp(s, cvofmt_t::rep_string()))
            cvofmt_t::set_rep_string(lstring::copy(s));
    }
    else
        s = cvofmt_t::rep_string();
    if (!s || !*s) {
        restore_defaults();
        return;
    }

    rp_state rp;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (*tok == 'r')
            rp.mnone = true;
        else if (*tok == 'm') {
            char *t = strchr(tok, '=');
            if (t) {
                int n = atoi(t+1);
                if (n >= REP_RUN_MIN && n <= REP_RUN_MAX)
                    rp.mval = n;
            }
        }
        else if (*tok == 'a') {
            char *t = strchr(tok, '=');
            if (t) {
                int n = atoi(t+1);
                if (n == 0) {
                    rp.anone = true;
                    rp.aval = 0;
                }
                else if (n >= REP_ARRAY_MIN && n <= REP_ARRAY_MAX)
                    rp.aval = n;
            }
        }
        else if (*tok == 't') {
            char *t = strchr(tok, '=');
            if (t) {
                int n = atoi(t+1);
                if (n == 0) {
                    rp.tnone = true;
                    rp.tval = 0;
                }
                else if (n >= REP_MAX_REPS_MIN && n <= REP_MAX_REPS_MAX)
                    rp.tval = n;
            }
        }
        else if (*tok == 'd') {
            ;
        }
        else if (*tok == 'x') {
            char *t = strchr(tok, '=');
            if (t) {
                int n = atoi(t+1);
                if (n >= REP_MAX_ITEMS_MIN && n <= REP_MAX_ITEMS_MAX)
                    rp.xval = n;
            }
        }
        else {
            if (strchr(tok, 'b'))
                rp.oflgs |= OAS_CR_BOX;
            if (strchr(tok, 'p'))
                rp.oflgs |= OAS_CR_POLY;
            if (strchr(tok, 'w'))
                rp.oflgs |= OAS_CR_WIRE;
            if (strchr(tok, 'l'))
                rp.oflgs |= OAS_CR_LAB;
            if (strchr(tok, 'c'))
                rp.oflgs |= OAS_CR_CELL;
        }
        delete [] tok;
    }
    if (rp.oflgs) {
        rp.cf = (rp.oflgs & OAS_CR_CELL);
        rp.bf = (rp.oflgs & OAS_CR_BOX);
        rp.pf = (rp.oflgs & OAS_CR_POLY);
        rp.wf = (rp.oflgs & OAS_CR_WIRE);
        rp.lf = (rp.oflgs & OAS_CR_LAB);
    }

    GRX->SetStatus(oas_objc, rp.cf);
    GRX->SetStatus(oas_objb, rp.bf);
    GRX->SetStatus(oas_objp, rp.pf);
    GRX->SetStatus(oas_objw, rp.wf);
    GRX->SetStatus(oas_objl, rp.lf);
    if (rp.mnone) {
        GRX->SetStatus(oas_noruns, true);
        sb_entm.set_sensitive(false, true);
        sb_enta.set_sensitive(false, true);
        GRX->SetStatus(oas_noarrs, true);
        gtk_widget_set_sensitive(oas_noarrs, false);
    }
    else {
        GRX->SetStatus(oas_noruns, false);
        sb_entm.set_sensitive(true);
        sb_entm.set_value(rp.mval);
        gtk_widget_set_sensitive(oas_noarrs, true);
        if (rp.anone) {
            GRX->SetStatus(oas_noarrs, true);
            sb_enta.set_sensitive(false, true);
        }
        else {
            GRX->SetStatus(oas_noarrs, false);
            sb_enta.set_sensitive(true);
            sb_enta.set_value(rp.aval);
        }
    }
    sb_entx.set_value(rp.xval);
    if (rp.tnone) {
        GRX->SetStatus(oas_nosim, true);
        sb_entt.set_sensitive(false, true);
    }
    else {
        GRX->SetStatus(oas_nosim, false);
        sb_entt.set_sensitive(true);
        sb_entt.set_value(rp.tval);
    }
}


void
sOas::restore_defaults()
{
    GRX->SetStatus(oas_objc, true);
    GRX->SetStatus(oas_objb, true);
    GRX->SetStatus(oas_objp, true);
    GRX->SetStatus(oas_objw, true);
    GRX->SetStatus(oas_objl, true);

    GRX->SetStatus(oas_noruns, false);
    GRX->SetStatus(oas_noarrs, false);
    GRX->SetStatus(oas_nosim, false);
    gtk_widget_set_sensitive(oas_noruns, true);
    gtk_widget_set_sensitive(oas_noarrs, true);
    gtk_widget_set_sensitive(oas_nosim, true);

    sb_entm.set_value(REP_RUN_MIN);
    sb_entm.set_sensitive(true);
    sb_enta.set_value(REP_ARRAY_MIN);
    sb_enta.set_sensitive(true);
    sb_entx.set_value(REP_MAX_ITEMS);
    sb_entt.set_value(REP_MAX_REPS);
    sb_entt.set_sensitive(true);

    oas_lastm = REP_RUN_MIN;
    oas_lasta = REP_ARRAY_MIN;
    oas_lastt = REP_MAX_REPS;
}


void
sOas::set_repvar()
{
    sLstr lstr;
    bool cv = GRX->GetStatus(oas_objc);
    bool bv = GRX->GetStatus(oas_objb);
    bool pv = GRX->GetStatus(oas_objp);
    bool wv = GRX->GetStatus(oas_objw);
    bool lv = GRX->GetStatus(oas_objl);
    if (cv && bv && pv && wv && lv)
        ;
    else {
        if (cv)
            lstr.add_c('c');
        if (bv)
            lstr.add_c('b');
        if (pv)
            lstr.add_c('p');
        if (wv)
            lstr.add_c('w');
        if (lv)
            lstr.add_c('l');
    }

    char buf[64];
    if (GRX->GetStatus(oas_noruns)) {
        if (lstr.length())
            lstr.add_c(' ');
        lstr.add("r");
    }
    else {
        const char *str = sb_entm.get_string();
        int mval;
        if (sscanf(str, "%d", &mval) == 1 &&
                mval > REP_RUN_MIN && mval <= REP_RUN_MAX) {
            sprintf(buf, "m=%d", mval);
            if (lstr.length())
                lstr.add_c(' ');
            lstr.add(buf);
        }
        if (GRX->GetStatus(oas_noarrs)) {
            if (lstr.length())
                lstr.add_c(' ');
            lstr.add("a=0");
        }
        else {
            str = sb_enta.get_string();
            int aval;
            if (sscanf(str, "%d", &aval) == 1 &&
                    aval > REP_ARRAY_MIN && aval <= REP_ARRAY_MAX) {
                sprintf(buf, "a=%d", aval);
                if (lstr.length())
                    lstr.add_c(' ');
                lstr.add(buf);
            }
        }
    }
    if (GRX->GetStatus(oas_nosim)) {
        if (lstr.length())
            lstr.add_c(' ');
        lstr.add("t=0");
    }
    else {
        const char *str = sb_entt.get_string();
        int tval;
        if (sscanf(str, "%d", &tval) == 1 &&
                tval >= REP_MAX_REPS_MIN && tval <= REP_MAX_REPS_MAX &&
                tval != REP_MAX_REPS) {
            sprintf(buf, "t=%d", tval);
            if (lstr.length())
                lstr.add_c(' ');
            lstr.add(buf);
        }
    }

    const char *str = sb_entx.get_string();
    int xval;
    if (sscanf(str, "%d", &xval) == 1 &&
            xval >= REP_MAX_ITEMS_MIN && xval <= REP_MAX_ITEMS_MAX &&
            xval != REP_MAX_ITEMS) {
        sprintf(buf, "x=%d", xval);
        if (lstr.length())
            lstr.add_c(' ');
        lstr.add(buf);
    }
    cvofmt_t::set_rep_string(lstr.string_trim());
    if (!cvofmt_t::rep_string())
        cvofmt_t::set_rep_string(lstring::copy(""));

    // If the property is already set, update it.
    str = CDvdb()->getVariable(VA_OasWriteRep);
    if (str && strcmp(str, cvofmt_t::rep_string()))
        CDvdb()->setVariable(VA_OasWriteRep, cvofmt_t::rep_string());
}


// Static function.
void
sOas::oas_cancel_proc(GtkWidget*, void*)
{
    Cvt()->PopUpOasAdv(0, MODE_OFF, 0, 0);
}


// Static function.
void
sOas::oas_action(GtkWidget *caller, void*)
{
    if (!Oas)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:oasadv"))
        return;
    }
    if (!strcmp(name, "notrap")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_OasWriteNoTrapezoids, 0);
        else
            CDvdb()->clearVariable(VA_OasWriteNoTrapezoids);
        return;
    }
    if (!strcmp(name, "wtob")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_OasWriteWireToBox, 0);
        else
            CDvdb()->clearVariable(VA_OasWriteWireToBox);
        return;
    }
    if (!strcmp(name, "rwtop")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_OasWriteRndWireToPoly, 0);
        else
            CDvdb()->clearVariable(VA_OasWriteRndWireToPoly);
        return;
    }
    if (!strcmp(name, "nogcd")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_OasWriteNoGCDcheck, 0);
        else
            CDvdb()->clearVariable(VA_OasWriteNoGCDcheck);
        return;
    }
    if (!strcmp(name, "oldsort")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_OasWriteUseFastSort, 0);
        else
            CDvdb()->clearVariable(VA_OasWriteUseFastSort);
        return;
    }

    if (caller == Oas->oas_def) {
        Oas->restore_defaults();
        Oas->set_repvar();
        return;
    }

    if (caller == Oas->oas_noruns) {
        if (GRX->GetStatus(caller)) {
            const char *str = Oas->sb_entm.get_string();
            int mval;
            if (sscanf(str, "%d", &mval) == 1 &&
                    mval >= REP_RUN_MIN && mval <= REP_RUN_MAX)
                Oas->oas_lastm = mval;
            str = Oas->sb_enta.get_string();
            int aval;
            if (sscanf(str, "%d", &aval) == 1 &&
                    aval > REP_ARRAY_MIN && aval <= REP_ARRAY_MAX)
                Oas->oas_lasta = aval;

            Oas->sb_entm.set_sensitive(false, true);
            Oas->sb_enta.set_sensitive(false, true);
            GRX->SetStatus(Oas->oas_noarrs, true);
            gtk_widget_set_sensitive(Oas->oas_noarrs, false);
        }
        else {
            Oas->sb_entm.set_sensitive(true);
            Oas->sb_entm.set_value(Oas->oas_lastm);
            gtk_widget_set_sensitive(Oas->oas_noarrs, true);
        }
        Oas->set_repvar();
        return;
    }
    if (caller == Oas->oas_noarrs) {
        if (GRX->GetStatus(caller)) {
            const char *str = Oas->sb_enta.get_string();
            int aval;
            if (sscanf(str, "%d", &aval) == 1 &&
                    aval > REP_ARRAY_MIN && aval <= REP_ARRAY_MAX)
                Oas->oas_lasta = aval;

            Oas->sb_enta.set_sensitive(false, true);
        }
        else {
            Oas->sb_enta.set_value(Oas->oas_lasta);
            Oas->sb_enta.set_sensitive(true);
        }
        Oas->set_repvar();
        return;
    }
    if (caller == Oas->oas_nosim) {

        if (GRX->GetStatus(caller)) {
            const char *str = Oas->sb_entt.get_string();
            int tval;
            if (sscanf(str, "%d", &tval) == 1 &&
                    tval >= REP_MAX_REPS_MIN && tval <= REP_MAX_REPS_MAX)
                Oas->oas_lastt = tval;
            Oas->sb_entt.set_sensitive(false, true);
        }
        else {
            Oas->sb_entt.set_value(Oas->oas_lastt);
            Oas->sb_entt.set_sensitive(true);
        }
        Oas->set_repvar();
        return;
    }
    if (caller == Oas->oas_objc || caller == Oas->oas_objb ||
            caller == Oas->oas_objp || caller == Oas->oas_objw ||
            caller == Oas->oas_objl) {
        Oas->set_repvar();
        return;
    }
}


// Static function.
void
sOas::oas_val_changed(GtkWidget*, void*)
{
    if (!Oas)
        return;
    Oas->set_repvar();
}


// Static function.
void
sOas::oas_pmask_menu_proc(GtkWidget*, void *client_data)
{
    char bf[2];
    bf[1] = 0;
    char *s = (char*)client_data;
    for (int i = 0; pmaskvals[i]; i++) {
        if (!strcmp(s, pmaskvals[i])) {
            if (i == 0)
                CDvdb()->clearVariable(VA_OasWritePrptyMask);
            else if (i == 1) {
                bf[0] = '1';
                CDvdb()->setVariable(VA_OasWritePrptyMask, bf);
            }
            else if (i == 2) {
                bf[0] = '2';
                CDvdb()->setVariable(VA_OasWritePrptyMask, bf);
            }
            else if (i == 3) {
                bf[0] = '3';
                CDvdb()->setVariable(VA_OasWritePrptyMask, bf);
            }
            else if (i == 4)
                CDvdb()->setVariable(VA_OasWritePrptyMask, "all");
            break;
        }
    }
}

