
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
#include "drc.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkspinbtn.h"


//------------------------------------------------------------------------
// DRC Limits Pop-Up
//
// This provides entry fields for the various limit parameters and
// the error recording level.
//
// Help system keywords used:
//  xic:limit

namespace {
    namespace gtkdrclim {
        struct sDL
        {
            sDL(GRobject);
            ~sDL();

            GtkWidget *shell() { return (dl_popup); }

            void update();

        private:
            static void dl_cancel_proc(GtkWidget*, void*);
            static void dl_action_proc(GtkWidget*, void*);
            static void dl_val_changed(GtkWidget*, void*);
            static void dl_text_changed(GtkWidget*, void*);

            GRobject dl_caller;
            GtkWidget *dl_popup;
            GtkWidget *dl_luse;
            GtkWidget *dl_lskip;
            GtkWidget *dl_llist;
            GtkWidget *dl_ruse;
            GtkWidget *dl_rskip;
            GtkWidget *dl_rlist;
            GtkWidget *dl_skip;
            GtkWidget *dl_b1;
            GtkWidget *dl_b2;
            GtkWidget *dl_b3;

            GTKspinBtn sb_max_errs;
            GTKspinBtn sb_imax_objs;
            GTKspinBtn sb_imax_time;
            GTKspinBtn sb_imax_errs;
        };

        sDL *DL;
    }
}

using namespace gtkdrclim;


void
cDRC::PopUpDrcLimits(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete DL;
        return;
    }
    if (mode == MODE_UPD) {
        if (DL)
            DL->update();
        return;
    }
    if (DL)
        return;

    new sDL(caller);
    if (!DL->shell()) {
        delete DL;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(DL->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UR), DL->shell(), mainBag()->Viewport());
    gtk_widget_show(DL->shell());
}
// End of cDRC functions.


sDL::sDL(GRobject c)
{
    DL = this;
    dl_caller = c;
    dl_popup = 0;
    dl_luse = 0;
    dl_lskip = 0;
    dl_llist = 0;
    dl_ruse = 0;
    dl_rskip = 0;
    dl_rlist = 0;
    dl_skip = 0;
    dl_b1 = 0;
    dl_b2 = 0;
    dl_b3 = 0;

    dl_popup = gtk_NewPopup(0, "DRC Parameter Setup", dl_cancel_proc, 0);
    if (!dl_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(dl_popup), false);

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(dl_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("Set DRC parameters and limits");
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
        G_CALLBACK(dl_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Layer list
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    label = gtk_label_new("Layer list");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    dl_luse = gtk_check_button_new_with_label("Check listed layers only");
    gtk_widget_set_name(dl_luse, "luse");
    gtk_widget_show(dl_luse);
    g_signal_connect(G_OBJECT(dl_luse), "clicked",
        G_CALLBACK(dl_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), dl_luse, true, true, 0);

    dl_lskip = gtk_check_button_new_with_label("Skip listed layers");
    gtk_widget_set_name(dl_lskip, "lskip");
    gtk_widget_show(dl_lskip);
    g_signal_connect(G_OBJECT(dl_lskip), "clicked",
        G_CALLBACK(dl_action_proc), this);
    gtk_box_pack_start(GTK_BOX(row), dl_lskip, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    dl_llist = gtk_entry_new();
    gtk_widget_show(dl_llist);
    gtk_table_attach(GTK_TABLE(form), dl_llist, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_FILL),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Rule list
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    label = gtk_label_new("Rule list");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(row), label, true, true, 0);

    dl_ruse = gtk_check_button_new_with_label("Check listed rules only");
    gtk_widget_set_name(dl_ruse, "ruse");
    gtk_widget_show(dl_ruse);
    g_signal_connect(G_OBJECT(dl_ruse), "clicked",
        G_CALLBACK(dl_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), dl_ruse, true, true, 0);

    dl_rskip = gtk_check_button_new_with_label("Skip listed rules");
    gtk_widget_set_name(dl_rskip, "rskip");
    gtk_widget_show(dl_rskip);
    g_signal_connect(G_OBJECT(dl_rskip), "clicked",
        G_CALLBACK(dl_action_proc), this);
    gtk_box_pack_start(GTK_BOX(row), dl_rskip, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    dl_rlist = gtk_entry_new();
    gtk_widget_show(dl_rlist);
    gtk_table_attach(GTK_TABLE(form), dl_rlist, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_FILL),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Limits group.
    //
    frame = gtk_frame_new("DRC limit values, set to 0 for no limit");
    gtk_widget_show(frame);
    GtkWidget *tform = gtk_table_new(2, 1, false);
    gtk_widget_show(tform);
    gtk_container_add(GTK_CONTAINER(frame), tform);
    int rcnt = 0;

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Batch Mode error limit
    //
    label = gtk_label_new("Batch mode error count limit");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(tform), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *sb = sb_max_errs.init(DRC()->maxErrors(),
        DRC_MAX_ERRS_MIN, DRC_MAX_ERRS_MAX, 0);
    sb_max_errs.connect_changed(G_CALLBACK(dl_val_changed), 0, "MaxErrs");
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(tform), sb, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    //
    // Interactive Mode object limit
    //
    label = gtk_label_new("Interactive checking object count limit");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(tform), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_imax_objs.init(DRC()->intrMaxObjs(),
        DRC_INTR_MAX_OBJS_MIN, DRC_INTR_MAX_OBJS_MAX, 0);
    sb_imax_objs.connect_changed(G_CALLBACK(dl_val_changed), 0,
        "IMaxObjs");
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(tform), sb, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    //
    // Interactive Mode time limit
    //
    label = gtk_label_new("Interactive check time limit (milliseconds)");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(tform), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_imax_time.init(DRC()->intrMaxTime(),
        DRC_INTR_MAX_TIME_MIN, DRC_INTR_MAX_TIME_MAX, 0);
    sb_imax_time.connect_changed(G_CALLBACK(dl_val_changed), 0,
        "IMaxTime");
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(tform), sb, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    //
    // Interactive Mode error limit
    //
    label = gtk_label_new("Interactive mode error count limit");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(tform), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_imax_errs.init(DRC()->intrMaxErrors(),
        DRC_INTR_MAX_ERRS_MIN, DRC_INTR_MAX_ERRS_MAX, 0);
    sb_imax_errs.connect_changed(G_CALLBACK(dl_val_changed), 0,
        "IMaxErrs");
    gtk_widget_set_size_request(sb, 100, -1);

    gtk_table_attach(GTK_TABLE(tform), sb, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    //
    // Skip cells check box
    //
    button = gtk_check_button_new_with_label(
        "Skip interactive test of moved/copied/placed cells");
    gtk_widget_set_name(button, "skip_cells_intr");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(dl_action_proc), 0);
    gtk_table_attach(GTK_TABLE(tform), button, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;
    dl_skip = button;

    //
    // DRC error level
    //
    frame = gtk_frame_new("DRC error recording");
    gtk_widget_show(frame);
    GtkWidget *col = gtk_vbox_new(false, 2);
    gtk_widget_show(col);
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    gtk_box_pack_start(GTK_BOX(row), col, false, false, 0);
    gtk_container_add(GTK_CONTAINER(frame), row);

    button = gtk_radio_button_new_with_label(0, "One error per object");
    gtk_widget_set_name(button, "one_err");
    gtk_widget_show(button);
    GSList *group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    gtk_box_pack_start(GTK_BOX(col), button, false, false, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(dl_action_proc), 0);
    dl_b1 = button;

    button = gtk_radio_button_new_with_label(group,
        "One error of each type per object");
    gtk_widget_set_name(button, "one_type");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(col), button, false, false, 0);
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(dl_action_proc), 0);
    dl_b2 = button;

    button = gtk_radio_button_new_with_label(group, "Record all errors");
    gtk_widget_set_name(button, "all_errs");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(col), button, false, false, 0);
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(dl_action_proc), 0);
    dl_b3 = button;

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(dl_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(dl_popup), button);

    update();

    // must be done after entry text set
    g_signal_connect(G_OBJECT(dl_llist), "changed",
        G_CALLBACK(dl_text_changed), 0);
    g_signal_connect(G_OBJECT(dl_rlist), "changed",
        G_CALLBACK(dl_text_changed), 0);
}


sDL::~sDL()
{
    DL = 0;
    if (dl_caller)
        GRX->Deselect(dl_caller);
    if (dl_popup)
        gtk_widget_destroy(dl_popup);
}


void
sDL::update()
{
    const char *s = CDvdb()->getVariable(VA_DrcUseLayerList);
    if (s) {
        if (*s == 'n' || *s == 'N') {
            GRX->SetStatus(dl_lskip, true);
            GRX->SetStatus(dl_luse, false);
        }
        else {
            GRX->SetStatus(dl_lskip, false);
            GRX->SetStatus(dl_luse, true);
        }
    }
    else {
        GRX->SetStatus(dl_lskip, false);
        GRX->SetStatus(dl_luse, false);
    }

    s = CDvdb()->getVariable(VA_DrcLayerList);
    if (!s)
        s = "";
    const char *l = gtk_entry_get_text(GTK_ENTRY(dl_llist));
    if (!l)
        l = "";
    if (strcmp(s, l))
        gtk_entry_set_text(GTK_ENTRY(dl_llist), s);
    if (GRX->GetStatus(dl_luse) || GRX->GetStatus(dl_lskip))
        gtk_widget_set_sensitive(dl_llist, true);
    else
        gtk_widget_set_sensitive(dl_llist, false);

    s = CDvdb()->getVariable(VA_DrcUseRuleList);
    if (s) {
        if (*s == 'n' || *s == 'N') {
            GRX->SetStatus(dl_rskip, true);
            GRX->SetStatus(dl_ruse, false);
        }
        else {
            GRX->SetStatus(dl_rskip, false);
            GRX->SetStatus(dl_ruse, true);
        }
    }
    else {
        GRX->SetStatus(dl_rskip, false);
        GRX->SetStatus(dl_ruse, false);
    }

    s = CDvdb()->getVariable(VA_DrcRuleList);
    if (!s)
        s = "";
    l = gtk_entry_get_text(GTK_ENTRY(dl_rlist));
    if (!l)
        l = "";
    if (strcmp(s, l))
        gtk_entry_set_text(GTK_ENTRY(dl_rlist), s);
    if (GRX->GetStatus(dl_ruse) || GRX->GetStatus(dl_rskip))
        gtk_widget_set_sensitive(dl_rlist, true);
    else
        gtk_widget_set_sensitive(dl_rlist, false);

    if (DRC()->maxErrors() > DRC_MAX_ERRS_MAX)
        DRC()->setMaxErrors(DRC_MAX_ERRS_MAX);
    sb_max_errs.set_value(DRC()->maxErrors());
    if (DRC()->intrMaxObjs() > DRC_INTR_MAX_OBJS_MAX)
        DRC()->setIntrMaxObjs(DRC_INTR_MAX_OBJS_MAX);
    sb_imax_objs.set_value(DRC()->intrMaxObjs());
    if (DRC()->intrMaxTime() > DRC_INTR_MAX_TIME_MAX)
        DRC()->setIntrMaxTime(DRC_INTR_MAX_TIME_MAX);
    sb_imax_time.set_value(DRC()->intrMaxTime());
    if (DRC()->intrMaxErrors() > DRC_INTR_MAX_ERRS_MAX)
        DRC()->setIntrMaxErrors(DRC_INTR_MAX_ERRS_MAX);
    sb_imax_errs.set_value(DRC()->intrMaxErrors());
    GRX->SetStatus(dl_skip, DRC()->isIntrSkipInst());
    if (DRC()->errorLevel() == DRCone_err) {
        GRX->SetStatus(dl_b1, true);
        GTK_TOGGLE_BUTTON(dl_b1)->active = true;
        GRX->SetStatus(dl_b2, false);
        GTK_TOGGLE_BUTTON(dl_b2)->active = false;
        GRX->SetStatus(dl_b3, false);
        GTK_TOGGLE_BUTTON(dl_b3)->active = false;
    }
    else if (DRC()->errorLevel() == DRCone_type) {
        GRX->SetStatus(dl_b1, false);
        GTK_TOGGLE_BUTTON(dl_b1)->active = false;
        GRX->SetStatus(dl_b2, true);
        GTK_TOGGLE_BUTTON(dl_b2)->active = true;
        GRX->SetStatus(dl_b3, false);
        GTK_TOGGLE_BUTTON(dl_b3)->active = false;
    }
    else {
        GRX->SetStatus(dl_b1, false);
        GTK_TOGGLE_BUTTON(dl_b1)->active = false;
        GRX->SetStatus(dl_b2, false);
        GTK_TOGGLE_BUTTON(dl_b2)->active = false;
        GRX->SetStatus(dl_b3, true);
        GTK_TOGGLE_BUTTON(dl_b3)->active = true;
    }
}


// Static functions.
void
sDL::dl_cancel_proc(GtkWidget*, void*)
{
    DRC()->PopUpDrcLimits(0, MODE_OFF);
}


// Static functions.
void
sDL::dl_action_proc(GtkWidget *caller, void*)
{
    if (!DL)
        return;

    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help"))
        DSPmainWbag(PopUpHelp("xic:limit"))
    else if (!strcmp(name, "luse")) {
        if (GRX->GetStatus(caller)) {
            CDvdb()->setVariable(VA_DrcUseLayerList, 0);
            // Give the list entry the focus.
            gtk_window_set_focus(GTK_WINDOW(DL->dl_popup), DL->dl_llist);
        }
        else
            CDvdb()->clearVariable(VA_DrcUseLayerList);
    }
    else if (!strcmp(name, "lskip")) {
        if (GRX->GetStatus(caller)) {
            CDvdb()->setVariable(VA_DrcUseLayerList, "n");
            // Give the list entry the focus.
            gtk_window_set_focus(GTK_WINDOW(DL->dl_popup), DL->dl_llist);
        }
        else
            CDvdb()->clearVariable(VA_DrcUseLayerList);
    }
    else if (!strcmp(name, "ruse")) {
        if (GRX->GetStatus(caller)) {
            CDvdb()->setVariable(VA_DrcUseRuleList, 0);
            // Give the list entry the focus.
            gtk_window_set_focus(GTK_WINDOW(DL->dl_popup), DL->dl_rlist);
        }
        else
            CDvdb()->clearVariable(VA_DrcUseRuleList);
    }
    else if (!strcmp(name, "rskip")) {
        if (GRX->GetStatus(caller)) {
            CDvdb()->setVariable(VA_DrcUseRuleList, "n");
            // Give the list entry the focus.
            gtk_window_set_focus(GTK_WINDOW(DL->dl_popup), DL->dl_rlist);
        }
        else
            CDvdb()->clearVariable(VA_DrcUseRuleList);
    }
    else if (!strcmp(name, "skip_cells_intr")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_DrcInterSkipInst, "");
        else
            CDvdb()->clearVariable(VA_DrcInterSkipInst);
    }
    else if (!strcmp(name, "one_err")) {
        if (GRX->GetStatus(caller))
            CDvdb()->clearVariable(VA_DrcLevel);
    }
    else if (!strcmp(name, "one_type")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_DrcLevel, "1");
    }
    else if (!strcmp(name, "all_errs")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_DrcLevel, "2");
    }
}


void
sDL::dl_text_changed(GtkWidget *caller, void*)
{
    if (!DL)
        return;
    if (caller == DL->dl_llist) {
        const char *s = gtk_entry_get_text(GTK_ENTRY(caller));
        const char *ss = CDvdb()->getVariable(VA_DrcLayerList);
        if (s && *s) {
            if (!ss || strcmp(ss, s))
                CDvdb()->setVariable(VA_DrcLayerList, s);
        }
        else
            CDvdb()->clearVariable(VA_DrcLayerList);
    }
    else if (caller == DL->dl_rlist) {
        const char *s = gtk_entry_get_text(GTK_ENTRY(caller));
        const char *ss = CDvdb()->getVariable(VA_DrcRuleList);
        if (s && *s) {
            if (!ss || strcmp(ss, s))
                CDvdb()->setVariable(VA_DrcRuleList, s);
        }
        else
            CDvdb()->clearVariable(VA_DrcRuleList);
    }
}


// Static functions.
void
sDL::dl_val_changed(GtkWidget *caller, void*)
{
    if (!DL)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "MaxErrs")) {
        const char *s = DL->sb_max_errs.get_string();
        int i;
        if (sscanf(s, "%d", &i) == 1 && i >= DRC_MAX_ERRS_MIN &&
                i <= DRC_MAX_ERRS_MAX) {
            if (i == DRC_MAX_ERRS_DEF)
                CDvdb()->clearVariable(VA_DrcMaxErrors);
            else
                CDvdb()->setVariable(VA_DrcMaxErrors, s);
        }
    }
    else if (!strcmp(name, "IMaxObjs")) {
        const char *s = DL->sb_imax_objs.get_string();
        int i;
        if (sscanf(s, "%d", &i) == 1 && i >= DRC_INTR_MAX_OBJS_MIN &&
                i <= DRC_INTR_MAX_OBJS_MAX) {
            if (i == DRC_INTR_MAX_OBJS_DEF)
                CDvdb()->clearVariable(VA_DrcInterMaxObjs);
            else
                CDvdb()->setVariable(VA_DrcInterMaxObjs, s);
        }
    }
    else if (!strcmp(name, "IMaxTime")) {
        const char *s = DL->sb_imax_time.get_string();
        int i;
        if (sscanf(s, "%d", &i) == 1 && i >= DRC_INTR_MAX_TIME_MIN &&
                i <= DRC_INTR_MAX_TIME_MAX) {
            if (i == DRC_INTR_MAX_TIME_DEF)
                CDvdb()->clearVariable(VA_DrcInterMaxTime);
            else
                CDvdb()->setVariable(VA_DrcInterMaxTime, s);
        }
    }
    else if (!strcmp(name, "IMaxErrs")) {
        const char *s = DL->sb_imax_errs.get_string();
        int i;
        if (sscanf(s, "%d", &i) == 1 && i >= DRC_INTR_MAX_ERRS_MIN &&
                i <= DRC_INTR_MAX_ERRS_MAX) {
            if (i == DRC_INTR_MAX_ERRS_DEF)
                CDvdb()->clearVariable(VA_DrcInterMaxErrors);
            else
                CDvdb()->setVariable(VA_DrcInterMaxErrors, s);
        }
    }
}

