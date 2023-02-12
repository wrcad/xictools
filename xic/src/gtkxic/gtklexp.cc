
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

#define XXX_OPT

#include "main.h"
#include "edit.h"
#include "dsp_inlines.h"
#include "geo_grid.h"
#include "promptline.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkspinbtn.h"


//--------------------------------------------------------------------
// Pop-up to control layer expression evaluation.
//
// Help system keywords used:
//  xic:lexpr

namespace {
    namespace gtklexp {
        struct sLx
        {
            sLx(void*);
            ~sLx();

            void update();

            GtkWidget *shell() { return (lx_popup); }

        private:
            static void lx_cancel_proc(GtkWidget*, void*);
            static void lx_action(GtkWidget*, void*);
            static void lx_val_changed(GtkWidget*, void*);
            static void lx_depth_proc(GtkWidget*, void*);
            static int lx_popup_menu(GtkWidget*, GdkEvent*, void*);
            static void lx_save_proc(GtkWidget*, void*);
            static void lx_recall_proc(GtkWidget*, void*);

            GRobject lx_caller;
            GtkWidget *lx_popup;
            GtkWidget *lx_deflt;
            GtkWidget *lx_join;
            GtkWidget *lx_split_h;
            GtkWidget *lx_split_v;
            GtkWidget *lx_depth;
            GtkWidget *lx_recurse;
            GtkWidget *lx_merge;
            GtkWidget *lx_fast;
            GtkWidget *lx_none;
            GtkWidget *lx_tolayer;
            GtkWidget *lx_noclear;
            GtkWidget *lx_lexpr;
            GtkWidget *lx_save;
            GtkWidget *lx_recall;
            GtkWidget *lx_save_menu;
            GtkWidget *lx_recall_menu;

            GTKspinBtn sb_part;
            GTKspinBtn sb_thread;

            double lx_last_part_size;

            static char *last_lexpr;
            static int depth_hst;
            static int create_mode;
            static bool fast_mode;
            static bool use_merge;
            static bool do_recurse;
            static bool noclear;
        };

        sLx *Lx;
    }
}

using namespace gtklexp;

char *sLx::last_lexpr = 0;
int sLx::depth_hst = 0;
int sLx::create_mode = CLdefault;
bool sLx::fast_mode = false;
bool sLx::use_merge = false;
bool sLx::do_recurse = false;
bool sLx::noclear = false;


void
cEdit::PopUpLayerExp(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Lx;
        return;
    }
    if (mode == MODE_UPD) {
        if (Lx)
            Lx->update();
        return;
    }
    if (Lx)
        return;

    new sLx(caller);
    if (!Lx->shell()) {
        delete Lx;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Lx->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UL), Lx->shell(), mainBag()->Viewport());
    gtk_widget_show(Lx->shell());
}


sLx::sLx(GRobject c)
{
    Lx = this;
    lx_caller = c;
    lx_deflt = 0;
    lx_join = 0;
    lx_split_h = 0;
    lx_split_v = 0;
    lx_depth = 0;
    lx_recurse = 0;
    lx_merge = 0;
    lx_fast = 0;
    lx_none = 0;
    lx_tolayer = 0;
    lx_noclear = 0;
    lx_lexpr = 0;
    lx_save = 0;
    lx_recall = 0;
    lx_save_menu = 0;
    lx_recall_menu = 0;
    lx_last_part_size = DEF_GRD_PART_SIZE;

    lx_popup = gtk_NewPopup(0, "Exaluate Layer Expression", lx_cancel_proc, 0);
    if (!lx_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(lx_popup), false);

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(lx_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new(
        "Set parameters, evaluate layer expression");
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
        G_CALLBACK(lx_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // depth option, recurse check box
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Depth to process");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);
#define DMAX 6
#ifdef XXX_OPT
    lx_depth = gtk_option_menu_new();
    gtk_widget_set_name(lx_depth, "Depth");
    gtk_widget_show(lx_depth);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "Depth");

    for (int i = 0; i <= DMAX; i++) {
        char buf[16];
        if (i == DMAX)
            strcpy(buf, "all");
        else
            sprintf(buf, "%d", i);
        GtkWidget *mi = gtk_menu_item_new_with_label(buf);
        gtk_widget_set_name(mi, buf);
        gtk_widget_show(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
        g_signal_connect(G_OBJECT(mi), "activate",
            G_CALLBACK(lx_depth_proc), (void*)(long)i);
    }

    gtk_option_menu_set_menu(GTK_OPTION_MENU(lx_depth), menu);
#else
    lx_depth = gtk_combo_box_text_new();
    gtk_widget_set_name(lx_depth, "Depth");
    gtk_widget_show(lx_depth);
#endif
    gtk_box_pack_start(GTK_BOX(row), lx_depth, false, false, 0);

    lx_recurse = gtk_check_button_new_with_label(
        "Recursively create in subcells");
    gtk_widget_set_name(lx_recurse, "Recurse");
    gtk_widget_show(lx_recurse);
    g_signal_connect(G_OBJECT(lx_recurse), "clicked",
        G_CALLBACK(lx_action), 0);
    gtk_box_pack_start(GTK_BOX(row), lx_recurse, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // target layer entry
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("To layer");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    lx_tolayer = gtk_entry_new();
    gtk_widget_show(lx_tolayer);
    gtk_widget_set_size_request(lx_tolayer, 40, -1);
    gtk_box_pack_start(GTK_BOX(row), lx_tolayer, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // partition size entry
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Partition size");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    lx_none = gtk_toggle_button_new_with_label("None");
    gtk_widget_set_name(lx_none, "None");
    gtk_widget_show(lx_none);
    g_signal_connect(G_OBJECT(lx_none), "clicked",
        G_CALLBACK(lx_action), 0);
    gtk_box_pack_start(GTK_BOX(row), lx_none, false, false, 0);

    GtkWidget *sb = sb_part.init(0, GRD_PART_MIN, GRD_PART_MAX, 2);
    sb_part.connect_changed(G_CALLBACK(lx_val_changed), 0, "PartSize");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_start(GTK_BOX(row), sb, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // helper threads entry
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    label = gtk_label_new("Number of helper threads");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    sb = sb_thread.init(DSP_DEF_THREADS, DSP_MIN_THREADS, DSP_MAX_THREADS, 0);
    sb_thread.connect_changed(G_CALLBACK(lx_val_changed), 0, "thread");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(row), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // join/split radio group
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    frame = gtk_frame_new("New object format");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), row);

    lx_deflt = gtk_radio_button_new_with_label(0, "Default");
    gtk_widget_set_name(lx_deflt, "Default");
    gtk_widget_show(lx_deflt);
    GSList *group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(lx_deflt));
    g_signal_connect(G_OBJECT(lx_deflt), "clicked",
        G_CALLBACK(lx_action), 0);
    gtk_box_pack_start(GTK_BOX(row), lx_deflt, true, true, 0);

    lx_join = gtk_radio_button_new_with_label(group, "Joined");
    gtk_widget_set_name(lx_join, "Join");
    gtk_widget_show(lx_join);
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(lx_join));
    g_signal_connect(G_OBJECT(lx_join), "clicked",
        G_CALLBACK(lx_action), 0);
    gtk_box_pack_start(GTK_BOX(row), lx_join, true, true, 0);

    lx_split_h = gtk_radio_button_new_with_label(group, "Horiz Split");
    gtk_widget_set_name(lx_split_h, "SplitH");
    gtk_widget_show(lx_split_h);
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(lx_split_h));
    g_signal_connect(G_OBJECT(lx_split_h), "clicked",
        G_CALLBACK(lx_action), 0);
    gtk_box_pack_start(GTK_BOX(row), lx_split_h, true, true, 0);

    lx_split_v = gtk_radio_button_new_with_label(group, "Vert Split");
    gtk_widget_set_name(lx_split_v, "SplitV");
    gtk_widget_show(lx_split_v);
    g_signal_connect(G_OBJECT(lx_split_v), "clicked",
        G_CALLBACK(lx_action), 0);
    gtk_box_pack_start(GTK_BOX(row), lx_split_v, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // layer expression entry, store/recall buttons
    //
    lx_lexpr = gtk_entry_new();
    gtk_widget_show(lx_lexpr);

    frame = gtk_frame_new("Expression");
    gtk_widget_show(frame);

    GtkWidget *vbox = gtk_vbox_new(false, 4);
    gtk_widget_show(vbox);
    gtk_box_pack_start(GTK_BOX(vbox), lx_lexpr, false, false, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    row = gtk_hbox_new(false, 80);
    gtk_widget_show(row);

    lx_recall = gtk_button_new_with_label("Recall");
    gtk_widget_set_name(lx_recall, "Recall");
    gtk_widget_show(lx_recall);

    lx_recall_menu = gtk_menu_new();
    gtk_widget_set_name(lx_recall_menu, "Recall");
    for (int i = 0; i < ED_LEXPR_STORES; i++) {
        char buf[16];
        sprintf(buf, "Reg %d", i);
        GtkWidget *mi = gtk_menu_item_new_with_label(buf);
        gtk_widget_set_name(mi, buf);
        gtk_widget_show(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(lx_recall_menu), mi);
        g_signal_connect(G_OBJECT(mi), "activate",
            G_CALLBACK(lx_recall_proc), (void*)(long)i);
    }
    g_signal_connect(G_OBJECT(lx_recall), "button-press-event",
        G_CALLBACK(lx_popup_menu), lx_recall_menu);
    gtk_box_pack_start(GTK_BOX(row), lx_recall, false, false, 0);

    lx_save = gtk_button_new_with_label("Save");
    gtk_widget_set_name(lx_save, "Save");
    gtk_widget_show(lx_save);

    lx_save_menu = gtk_menu_new();
    gtk_widget_set_name(lx_save_menu, "Save");
    for (int i = 0; i < ED_LEXPR_STORES; i++) {
        char buf[16];
        sprintf(buf, "Reg %d", i);
        GtkWidget *mi = gtk_menu_item_new_with_label(buf);
        gtk_widget_set_name(mi, buf);
        gtk_widget_show(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(lx_save_menu), mi);
        g_signal_connect(G_OBJECT(mi), "activate",
            G_CALLBACK(lx_save_proc), (void*)(long)i);
    }
    g_signal_connect(G_OBJECT(lx_save), "button-press-event",
        G_CALLBACK(lx_popup_menu), lx_save_menu);
    gtk_box_pack_start(GTK_BOX(row), lx_save, false, false, 0);
    gtk_box_pack_start(GTK_BOX(vbox), row, false, false, 0);

    //
    // no clear check box
    //
    lx_noclear = gtk_check_button_new_with_label(
        "Don't clear layer before evaluation");
    gtk_widget_set_name(lx_noclear, "NoClear");
    gtk_widget_show(lx_noclear);
    g_signal_connect(G_OBJECT(lx_noclear), "clicked",
        G_CALLBACK(lx_action), 0);
    gtk_table_attach(GTK_TABLE(form), lx_noclear, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // merge and fast mode check boxes
    //
    lx_merge = gtk_check_button_new_with_label(
        "Use object merging while processing");
    gtk_widget_set_name(lx_merge, "Merge");
    gtk_widget_show(lx_merge);
    g_signal_connect(G_OBJECT(lx_merge), "clicked",
        G_CALLBACK(lx_action), 0);
    gtk_table_attach(GTK_TABLE(form), lx_merge, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    lx_fast = gtk_check_button_new_with_label(
        "Fast mode, NOT UNDOABLE");
    gtk_widget_set_name(lx_fast, "Fast");
    gtk_widget_show(lx_fast);
    g_signal_connect(G_OBJECT(lx_fast), "clicked",
        G_CALLBACK(lx_action), 0);
    gtk_table_attach(GTK_TABLE(form), lx_fast, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // evaluate and dismiss buttons
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    button = gtk_button_new_with_label("Evaluate");
    gtk_widget_set_name(button, "Evaluate");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(lx_action), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(lx_cancel_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(lx_popup), button);

    if (last_lexpr)
        gtk_entry_set_text(GTK_ENTRY(lx_lexpr), last_lexpr);
#ifdef XXX_OPT
    gtk_option_menu_set_history(GTK_OPTION_MENU(lx_depth), depth_hst);
#endif
    if (create_mode == CLdefault) {
        GRX->SetStatus(lx_deflt, true);
        GRX->SetStatus(lx_join, false);
        GRX->SetStatus(lx_split_h, false);
        GRX->SetStatus(lx_split_v, false);
    }
    else if (create_mode == CLsplitH) {
        GRX->SetStatus(lx_deflt, false);
        GRX->SetStatus(lx_join, false);
        GRX->SetStatus(lx_split_h, true);
        GRX->SetStatus(lx_split_v, false);
    }
    else if (create_mode == CLsplitV) {
        GRX->SetStatus(lx_deflt, false);
        GRX->SetStatus(lx_join, false);
        GRX->SetStatus(lx_split_h, false);
        GRX->SetStatus(lx_split_v, true);
    }
    else {
        GRX->SetStatus(lx_deflt, false);
        GRX->SetStatus(lx_join, true);
        GRX->SetStatus(lx_split_h, false);
        GRX->SetStatus(lx_split_v, false);
    }
    GRX->SetStatus(lx_fast, fast_mode);
    GRX->SetStatus(lx_merge, use_merge);
    GRX->SetStatus(lx_recurse, do_recurse);
    GRX->SetStatus(lx_noclear, noclear);
    update();
}


sLx::~sLx()
{
    Lx = 0;
    const char *s = gtk_entry_get_text(GTK_ENTRY(lx_lexpr));
    if (!*s)
        s = 0;
    delete [] sLx::last_lexpr;
    sLx::last_lexpr = lstring::copy(s);
    if (lx_caller)
        GRX->Deselect(lx_caller);
    if (lx_save_menu) {
        g_object_ref(lx_save_menu);
//XXX        gtk_object_ref(GTK_OBJECT(lx_save_menu));
        gtk_widget_destroy(lx_save_menu);
        g_object_unref(lx_save_menu);
    }
    if (lx_recall_menu) {
        g_object_ref(lx_recall_menu);
//XXX        gtk_object_ref(GTK_OBJECT(lx_recall_menu));
        gtk_widget_destroy(lx_recall_menu);
        g_object_unref(lx_recall_menu);
    }

    if (lx_popup)
        gtk_widget_destroy(lx_popup);
}


void
sLx::update()
{
    const char *s = CDvdb()->getVariable(VA_PartitionSize);
    if (s) {
        double d = atof(s);
        if (d == 0.0) {
            GRX->SetStatus(lx_none, true);
            sb_part.set_sensitive(false, true);
        }
        else {
            GRX->SetStatus(lx_none, false);
            sb_part.set_sensitive(true);
            sb_part.set_value(d);
        }
    }
    else {
        GRX->SetStatus(lx_none, false);
        sb_part.set_sensitive(true);
        sb_part.set_value(DEF_GRD_PART_SIZE);
    }

    s = CDvdb()->getVariable(VA_Threads);
    int n;
    if (s && sscanf(s, "%d", &n) == 1 && n >= DSP_MIN_THREADS &&
            n <= DSP_MAX_THREADS)
        ;
    else
        n = DSP_DEF_THREADS;
    sb_thread.set_value(n);
}


// Static function.
void
sLx::lx_cancel_proc(GtkWidget*, void*)
{
    ED()->PopUpLayerExp(0, MODE_OFF);
}


// Static function.
void
sLx::lx_action(GtkWidget *caller, void*)
{
    if (!Lx)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "Help"))
        DSPmainWbag(PopUpHelp("xic:lexpr"))
    else if (!strcmp(name, "None")) {
        if (GRX->GetStatus(caller)) {
            Lx->lx_last_part_size = Lx->sb_part.get_value();
            CDvdb()->setVariable(VA_PartitionSize, "0");
        }
        else
            Lx->sb_part.set_value(Lx->lx_last_part_size);
    }
    else if (!strcmp(name, "Recurse"))
        do_recurse = GRX->GetStatus(caller);
    else if (!strcmp(name, "NoClear"))
        noclear = GRX->GetStatus(caller);
    else if (!strcmp(name, "Default")) {
        if (GRX->GetStatus(caller))
            create_mode = CLdefault;
    }
    else if (!strcmp(name, "Join")) {
        if (GRX->GetStatus(caller))
            create_mode = CLjoin;
    }
    else if (!strcmp(name, "SplitH")) {
        if (GRX->GetStatus(caller))
            create_mode = CLsplitH;
    }
    else if (!strcmp(name, "SplitV")) {
        if (GRX->GetStatus(caller))
            create_mode = CLsplitV;
    }
    else if (!strcmp(name, "Merge"))
        use_merge = GRX->GetStatus(caller);
    else if (!strcmp(name, "Fast"))
        fast_mode = GRX->GetStatus(caller);
    else if (!strcmp(name, "Evaluate")) {
        const char *s = gtk_entry_get_text(GTK_ENTRY(Lx->lx_tolayer));
        char *lname = lstring::gettok(&s);
        if (!lname) {
            PL()->ShowPrompt("No target layer name given!");
            return;
        }
        sLstr lstr;
        lstr.add(lname);
        delete [] lname;
        lstr.add_c(' ');
        lstr.add(gtk_entry_get_text(GTK_ENTRY(Lx->lx_lexpr)));

        int depth = depth_hst;
        if (depth == DMAX)
            depth = CDMAXCALLDEPTH;

        int flags = create_mode;
        if (do_recurse && depth > 0)
            flags |= CLrecurse;
        if (noclear)
            flags |= CLnoClear;
        if (use_merge)
            flags |= CLmerge;
        if (fast_mode)
            flags |= CLnoUndo;

        ED()->createLayerCmd(lstr.string(), depth, flags);
    }
}


// Static function.
void
sLx::lx_val_changed(GtkWidget *caller, void*)
{
    if (!Lx)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "PartSize")) {
        const char *s = Lx->sb_part.get_string();
        double n;
        if (sscanf(s, "%lf", &n) == 1 && n >= GRD_PART_MIN &&
                n <= GRD_PART_MAX) {
            int nint = INTERNAL_UNITS(n);
            if (nint == INTERNAL_UNITS(DEF_GRD_PART_SIZE))
                CDvdb()->clearVariable(VA_PartitionSize);
            else
                CDvdb()->setVariable(VA_PartitionSize, s);
        }
    }
    else if (!strcmp(name, "thread")) {
        const char *s = Lx->sb_thread.get_string();
        char *endp;
        int d = (int)strtod(s, &endp);
        if (endp > s && d >= DSP_MIN_THREADS && d <= DSP_MAX_THREADS) {
            if (d == DSP_DEF_THREADS)
                CDvdb()->clearVariable(VA_Threads);
            else {
                char buf[32];
                sprintf(buf, "%d", d);
                CDvdb()->setVariable(VA_Threads, buf);
            }
        }
    }
}


// Static function.
void
sLx::lx_depth_proc(GtkWidget*, void *arg)
{
    depth_hst = (intptr_t)arg;
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
sLx::lx_popup_menu(GtkWidget *widget, GdkEvent *event, void *arg)
{
    gtk_menu_popup(GTK_MENU(arg), 0, 0, pos_func, widget, event->button.button,
        event->button.time);
    return (true);
}


// Static function.
void
sLx::lx_save_proc(GtkWidget*, void *arg)
{
    if (!Lx)
        return;
    int ix = (intptr_t)arg;
    const char *s = gtk_entry_get_text(GTK_ENTRY(Lx->lx_lexpr));
    if (*s)
        ED()->setLayerExpString(s, ix);
}


// Static function.
void
sLx::lx_recall_proc(GtkWidget*, void *arg)
{
    if (!Lx)
        return;
    int ix = (intptr_t)arg;
    const char *s = ED()->layerExpString(ix);
    if (!s)
        s = "";
    gtk_entry_set_text(GTK_ENTRY(Lx->lx_lexpr), s);
}

