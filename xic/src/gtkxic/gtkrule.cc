
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
#include "drc_kwords.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkspinbtn.h"


//--------------------------------------------------------------------
// Pop-up to control input parameters
//
// Help system keywords used:
//  xic:ruleedit

namespace {
    namespace gtkrule {
        struct sRu : public GTKbag
        {
            sRu(GRobject, DRCtype, const char*, bool(*)(const char*, void*),
                void*, const DRCtestDesc*);
            ~sRu();

            void update(DRCtype, const char*, const DRCtestDesc*);

        private:
            void alloff();
            void apply();

            static void ru_cancel_proc(GtkWidget*, void*);
            static void ru_action_proc(GtkWidget*, void*);
            static bool ru_edit_cb(const char*, void*, XEtype);

            GRobject ru_caller;
            bool (*ru_callback)(const char*, void*);
            void *ru_arg;
            char *ru_username;
            char *ru_stabstr;
            DRCtype ru_rule;

            GtkWidget *ru_label;
            GtkWidget *ru_region_la;
            GtkWidget *ru_region_ent;
            GtkWidget *ru_inside_la;
            GtkWidget *ru_inside_ent;
            GtkWidget *ru_outside_la;
            GtkWidget *ru_outside_ent;
            GtkWidget *ru_target_la;
            GtkWidget *ru_target_ent;
            GtkWidget *ru_dimen_la;
            GtkWidget *ru_dimen_sb;
            GtkWidget *ru_area_sb;
            GtkWidget *ru_diag_la;
            GtkWidget *ru_diag_sb;
            GtkWidget *ru_net_la;
            GtkWidget *ru_net_sb;
            GtkWidget *ru_use_st;
            GtkWidget *ru_edit_st;
            GtkWidget *ru_enc_la;
            GtkWidget *ru_enc_sb;
            GtkWidget *ru_opp_la;
            GtkWidget *ru_opp_sb1;
            GtkWidget *ru_opp_sb2;
            GtkWidget *ru_user_la;
            GtkWidget *ru_user_ent;
            GtkWidget *ru_descr_la;
            GtkWidget *ru_descr_ent;

            GTKspinBtn sb_dimen;
            GTKspinBtn sb_area;
            GTKspinBtn sb_diag;
            GTKspinBtn sb_net;
            GTKspinBtn sb_enc;
            GTKspinBtn sb_opp1;
            GTKspinBtn sb_opp2;
        };

        sRu *Ru;
    }
}

using namespace gtkrule;


// Pop up or update a design rule parameter input panel.  The username
// is the name of a user-defined rule, when type is drUserDefinedRule,
// otherwise it should be null.  The optional rule argument provides
// initial values, its type must match or it is ignored.
//
void
cDRC::PopUpRuleEdit(GRobject caller, ShowMode mode, DRCtype type,
    const char *username, bool (*callback)(const char*, void*),
    void *arg, const DRCtestDesc *rule)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Ru;
        return;
    }
    if (mode == MODE_UPD) {
        if (Ru)
            Ru->update(type, username, rule);
        return;
    }
    if (Ru) {
        Ru->update(type, username, rule);
        return;
    }

    new sRu(caller, type, username, callback, arg, rule);
    if (!Ru->Shell()) {
        delete Ru;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Ru->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_LL), Ru->Shell(), mainBag()->Viewport());
    gtk_widget_show(Ru->Shell());
}


sRu::sRu(GRobject c, DRCtype type, const char *username,
    bool (*callback)(const char*, void*), void *arg, const DRCtestDesc *rule)
{
    Ru = this;
    ru_caller = c;
    ru_callback = callback;
    ru_arg = arg;
    ru_username = 0;
    ru_rule = drNoRule;
    ru_stabstr = 0;
    ru_label = 0;
    ru_region_la = 0;
    ru_region_ent = 0;
    ru_inside_la = 0;
    ru_inside_ent = 0;
    ru_outside_la = 0;
    ru_outside_ent = 0;
    ru_target_la = 0;
    ru_target_ent = 0;
    ru_dimen_la = 0;
    ru_dimen_sb = 0;
    ru_area_sb = 0;
    ru_diag_la = 0;
    ru_diag_sb = 0;
    ru_net_la = 0;
    ru_net_sb = 0;
    ru_use_st = 0;
    ru_edit_st = 0;
    ru_enc_la = 0;
    ru_enc_sb = 0;
    ru_opp_la = 0;
    ru_opp_sb1 = 0;
    ru_opp_sb2 = 0;
    ru_user_la = 0;
    ru_user_ent = 0;
    ru_descr_la = 0;
    ru_descr_ent = 0;

    wb_shell = gtk_NewPopup(0, "Design Rule Parameters", ru_cancel_proc, 0);
    if (!wb_shell)
        return;

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("Set Design Rule parameters");
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
        G_CALLBACK(ru_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_label = label;
    rowcnt++;

    // Region.
    //
    label = gtk_label_new("Layer expression to AND with source\n"
        "figures on current layer (optional)");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_region_la = label;

    GtkWidget *entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_region_ent = entry;
    rowcnt++;

    // Inside edge layer or expression.
    //
    label = gtk_label_new(
        "Layer expression to AND at inside edges\n"
        "when forming test areas (optional)");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_inside_la = label;

    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_inside_ent = entry;
    rowcnt++;

    // Outside edge layer or expression.
    //
    label = gtk_label_new(
        "Layer expression to AND at outside edges\n"
        "when forming test areas (optional)");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_outside_la = label;

    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_outside_ent = entry;
    rowcnt++;

    // Target object layer or expression.
    //
    label = gtk_label_new("Target layer name or expression");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_target_la = label;

    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_target_ent = entry;
    rowcnt++;

    // Dimension.
    //
    label = gtk_label_new("Dimension (microns)");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 0, 1);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_dimen_la = label;

    int ndgt = CD()->numDigits();

#define SBSIZE 120
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    GtkWidget *sb = sb_dimen.init(0.0, 0.0, 1e6, ndgt);
    gtk_widget_set_size_request(sb, SBSIZE, -1);
    ru_dimen_sb = sb;
    gtk_box_pack_start(GTK_BOX(hbox), sb, true, true, 0);

    sb = sb_area.init(0.0, 0.0, 1e6, 6);
    gtk_widget_set_size_request(sb, SBSIZE, -1);
    ru_area_sb = sb;
    gtk_box_pack_start(GTK_BOX(hbox), sb, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Non-Manhattan "Diagional" dimension.
    //
    label = gtk_label_new("Non-Manhattan \"diagonal\" dimension");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 0, 1);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_diag_la = label;

    sb = sb_diag.init(0.0, 0.0, 1e6, ndgt);
    gtk_widget_set_size_request(sb, SBSIZE, -1);
    ru_diag_sb = sb;

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Same-Net dimension.
    //
    label = gtk_label_new("Same-Net spacing");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 0, 1);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_net_la = label;

    sb = sb_net.init(0.0, 0.0, 1e6, ndgt);
    gtk_widget_set_size_request(sb, SBSIZE, -1);
    ru_net_sb = sb;

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label("Use spacing table");
    gtk_widget_show(button);
    ru_use_st = button;

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label("Edit Table");
    gtk_widget_show(button);
    gtk_widget_set_name(button, "Edit");
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(ru_action_proc), 0);
    ru_edit_st = button;

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Dimension when target objects are fully enclosed.
    //
    label = gtk_label_new("Dimension when target objects\n"
        "are fully enclosed");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 0, 1);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_enc_la = label;

    sb = sb_enc.init(0.0, 0.0, 1e6, ndgt);
    gtk_widget_set_size_request(sb, SBSIZE, -1);
    ru_enc_sb = sb;

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Opposite-side dimensions.
    //
    label = gtk_label_new("Opposite side dimensions");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 0, 1);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_opp_la = label;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    sb = sb_opp1.init(0.0, 0.0, 1e6, ndgt);
    gtk_widget_set_size_request(sb, SBSIZE, -1);
    gtk_box_pack_start(GTK_BOX(row), sb, true, true, 0);
    ru_opp_sb1 = sb;

    sb = sb_opp2.init(0.0, 0.0, 1e6, ndgt);
    gtk_widget_set_size_request(sb, SBSIZE, -1);
    gtk_box_pack_start(GTK_BOX(row), sb, true, true, 0);
    ru_opp_sb2 = sb;

    gtk_table_attach(GTK_TABLE(form), row, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // User-defined rule arguments.
    //
    label = gtk_label_new("User-defined rule arguments");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_user_la = label;
    rowcnt++;

    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_user_ent = entry;
    rowcnt++;

    // Description string.
    //
    label = gtk_label_new("Description string");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), label, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_descr_la = label;
    rowcnt++;

    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_table_attach(GTK_TABLE(form), entry, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    ru_descr_ent = entry;
    rowcnt++;

    // Apply and Dismiss buttons.
    //
    button = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(button, "Apply");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(ru_action_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(ru_cancel_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    update(type, username, rule);
}


sRu::~sRu()
{
    Ru = 0;
    delete [] ru_username;
    delete [] ru_stabstr;
    if (ru_caller)
        GRX->Deselect(ru_caller);
}


namespace {
    inline void show_widget(GtkWidget *w, bool show)
    {
        if (show)
            gtk_widget_show(w);
        else
            gtk_widget_hide(w);
    }
}


void
sRu::update(DRCtype type, const char *username, const DRCtestDesc *rule)
{
    if (rule && rule->type() != type)
        rule = 0;
    delete [] ru_username;
    ru_username = (type == drUserDefinedRule ? lstring::copy(username) : 0);
    if (type == drUserDefinedRule && !ru_username) {
        PopUpMessage("Internal error, no rule name given.", true);
        return;
    }
    delete [] ru_stabstr;
    ru_stabstr = 0;
    if (rule && rule->spaceTab()) {
        sLstr lstr;
        sTspaceTable::tech_print(rule->spaceTab(), 0, &lstr);
        ru_stabstr = lstr.string_trim();
    }

    // Passing type == drNoRule when updating keeps existing rule.
    if (type != drNoRule)
        ru_rule = type;
    type = ru_rule;

    char buf[256];
    sprintf(buf, "Set Design Rule parameters for %s",
        type == drUserDefinedRule ? ru_username : DRCtestDesc::ruleName(type));
    gtk_label_set_text(GTK_LABEL(ru_label), buf);

    gtk_entry_set_text(GTK_ENTRY(ru_region_ent), "");
    gtk_entry_set_text(GTK_ENTRY(ru_descr_ent), "");
    sb_dimen.set_value(0);
    sb_area.set_value(0);
    if (rule) {
        char *s = rule->regionString();
        if (s) {
            gtk_entry_set_text(GTK_ENTRY(ru_region_ent), s);
            delete [] s;
        }
        sLstr lstr;
        rule->printComment(0, &lstr);
        if (lstr.length() > 0)
            gtk_entry_set_text(GTK_ENTRY(ru_descr_ent), lstr.string());
        sb_dimen.set_value(MICRONS(rule->dimen()));
        sb_area.set_value(rule->area());
    }

    alloff();
    switch (type) {
    case drNoRule:
        // Should not see this.
        break;

    case drExist:
        show_widget(ru_descr_la,    true);
        show_widget(ru_descr_ent,   true);
        break;

    case drConnected:
        show_widget(ru_region_la,   true);
        show_widget(ru_region_ent,  true);
        show_widget(ru_descr_la,    true);
        show_widget(ru_descr_ent,   true);
        break;

    case drNoHoles:
        gtk_label_set_text(GTK_LABEL(ru_dimen_la),
            "Minimum area (square microns)");
        gtk_label_set_text(GTK_LABEL(ru_diag_la),
            "Minimum width (microns)");
        if (rule && rule->value(0) > 0)
            sb_diag.set_value(MICRONS(rule->value(0)));
        show_widget(ru_region_la,   true);
        show_widget(ru_region_ent,  true);
        show_widget(ru_dimen_la,    true);
        show_widget(ru_area_sb,     true);
        show_widget(ru_diag_la,     true);
        show_widget(ru_diag_sb,     true);
        show_widget(ru_descr_la,    true);
        show_widget(ru_descr_ent,   true);
        break;

    case drOverlap:
    case drIfOverlap:
    case drNoOverlap:
    case drAnyOverlap:
    case drPartOverlap:
    case drAnyNoOverlap:
        gtk_entry_set_text(GTK_ENTRY(ru_target_ent), "");
        if (rule) {
            char *tstr = rule->targetString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_target_ent), tstr);
                delete [] tstr;
            }
        }
        show_widget(ru_region_la,   true);
        show_widget(ru_region_ent,  true);
        show_widget(ru_target_la,   true);
        show_widget(ru_target_ent,  true);
        show_widget(ru_descr_la,    true);
        show_widget(ru_descr_ent,   true);
        break;

    case drMinArea:
    case drMaxArea:
        gtk_label_set_text(GTK_LABEL(ru_dimen_la), type == drMinArea ?
                "Minimum area (square microns)" :
                "Maximum area (square microns)");
        show_widget(ru_region_la,   true);
        show_widget(ru_region_ent,  true);
        show_widget(ru_dimen_la,    true);
        show_widget(ru_area_sb,     true);
        show_widget(ru_descr_la,    true);
        show_widget(ru_descr_ent,   true);
        break;

    case drMinEdgeLength:
        gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), "");
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), "");
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_label_set_text(GTK_LABEL(ru_dimen_la),
            "Minimum edge length (microns)");
        gtk_entry_set_text(GTK_ENTRY(ru_target_ent), "");
        if (rule) {
            char *tstr = rule->targetString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_target_ent), tstr);
                delete [] tstr;
            }
        }
        show_widget(ru_region_la,   true);
        show_widget(ru_region_ent,  true);
        show_widget(ru_inside_la,   true);
        show_widget(ru_inside_ent,  true);
        show_widget(ru_outside_la,  true);
        show_widget(ru_outside_ent, true);
        show_widget(ru_target_la,   true);
        show_widget(ru_target_ent,  true);
        show_widget(ru_dimen_la,    true);
        show_widget(ru_dimen_sb,    true);
        show_widget(ru_descr_la,    true);
        show_widget(ru_descr_ent,   true);
        break;

    case drMaxWidth:
        gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), "");
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), "");
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_label_set_text(GTK_LABEL(ru_dimen_la), "Maximum width (microns)");
        show_widget(ru_region_la,   true);
        show_widget(ru_region_ent,  true);
        show_widget(ru_inside_la,   true);
        show_widget(ru_inside_ent,  true);
        show_widget(ru_outside_la,  true);
        show_widget(ru_outside_ent, true);
        show_widget(ru_dimen_la,    true);
        show_widget(ru_dimen_sb,    true);
        show_widget(ru_descr_la,    true);
        show_widget(ru_descr_ent,   true);
        break;

    case drMinWidth:
        gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), "");
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), "");
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_label_set_text(GTK_LABEL(ru_dimen_la), "Minimum width (microns)");
        gtk_label_set_text(GTK_LABEL(ru_diag_la),
            "Non-Manhattan \"diagonal\" width");
        sb_diag.set_value(0);
        if (rule) {
            if (rule->value(0) > 0)
                sb_diag.set_value(MICRONS(rule->value(0)));
        }
        show_widget(ru_region_la,   true);
        show_widget(ru_region_ent,  true);
        show_widget(ru_inside_la,   true);
        show_widget(ru_inside_ent,  true);
        show_widget(ru_outside_la,  true);
        show_widget(ru_outside_ent, true);
        show_widget(ru_dimen_la,    true);
        show_widget(ru_dimen_sb,    true);
        show_widget(ru_diag_la,     true);
        show_widget(ru_diag_sb,     true);
        show_widget(ru_descr_la,    true);
        show_widget(ru_descr_ent,   true);
        break;

    case drMinSpace:
        gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), "");
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), "");
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_label_set_text(GTK_LABEL(ru_dimen_la),
            "Default minimum spacing (microns)");
        gtk_label_set_text(GTK_LABEL(ru_diag_la),
            "Non-Manhattan \"diagonal\" spacing");
        sb_diag.set_value(0);
        sb_net.set_value(0);
        if (rule) {
            if (rule->value(0) > 0)
                sb_diag.set_value(MICRONS(rule->value(0)));
            if (rule->value(1) > 0)
                sb_net.set_value(MICRONS(rule->value(1)));
        }
        GRX->SetStatus(ru_use_st, (rule && rule->spaceTab() &&
            !(rule->spaceTab()->length & STF_IGNORE)));
        show_widget(ru_region_la,   true);
        show_widget(ru_region_ent,  true);
        show_widget(ru_inside_la,   true);
        show_widget(ru_inside_ent,  true);
        show_widget(ru_outside_la,  true);
        show_widget(ru_outside_ent, true);
        show_widget(ru_dimen_la,    true);
        show_widget(ru_dimen_sb,    true);
        show_widget(ru_diag_la,     true);
        show_widget(ru_diag_sb,     true);
        show_widget(ru_net_la,      true);
        show_widget(ru_net_sb,      true);
        show_widget(ru_use_st,      true);
        show_widget(ru_edit_st,     true);
        show_widget(ru_descr_la,    true);
        show_widget(ru_descr_ent,   true);
        break;

    case drMinSpaceTo:
        gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), "");
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), "");
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_label_set_text(GTK_LABEL(ru_dimen_la),
            "Default minimum spacing (microns)");
        gtk_label_set_text(GTK_LABEL(ru_diag_la),
            "Non-Manhattan \"diagonal\" spacing");
        gtk_entry_set_text(GTK_ENTRY(ru_target_ent), "");
        sb_diag.set_value(0);
        sb_net.set_value(0);
        if (rule) {
            char *tstr = rule->targetString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_target_ent), tstr);
                delete [] tstr;
            }
            if (rule->value(0) > 0)
                sb_diag.set_value(MICRONS(rule->value(0)));
            if (rule->value(1) > 0)
                sb_net.set_value(MICRONS(rule->value(1)));
        }
        GRX->SetStatus(ru_use_st, (rule && rule->spaceTab() &&
            !(rule->spaceTab()->length & STF_IGNORE)));
        show_widget(ru_region_la,   true);
        show_widget(ru_region_ent,  true);
        show_widget(ru_inside_la,   true);
        show_widget(ru_inside_ent,  true);
        show_widget(ru_outside_la,  true);
        show_widget(ru_outside_ent, true);
        show_widget(ru_target_la,   true);
        show_widget(ru_target_ent,  true);
        show_widget(ru_dimen_la,    true);
        show_widget(ru_dimen_sb,    true);
        show_widget(ru_diag_la,     true);
        show_widget(ru_diag_sb,     true);
        show_widget(ru_net_la,      true);
        show_widget(ru_net_sb,      true);
        show_widget(ru_use_st,      true);
        show_widget(ru_edit_st,     true);
        show_widget(ru_descr_la,    true);
        show_widget(ru_descr_ent,   true);
        break;

    case drMinSpaceFrom:
        gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), "");
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), "");
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_label_set_text(GTK_LABEL(ru_dimen_la),
            "Minimum dimension (microns)");
        gtk_entry_set_text(GTK_ENTRY(ru_target_ent), "");
        sb_enc.set_value(0);
        sb_opp1.set_value(0);
        sb_opp2.set_value(0);
        if (rule) {
            char *tstr = rule->targetString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_target_ent), tstr);
                delete [] tstr;
            }
            if (rule->value(0) > 0)
                sb_enc.set_value(MICRONS(rule->value(0)));
            if (rule->value(1) > 0 || rule->value(2) > 0) {
                sb_opp1.set_value(MICRONS(rule->value(1)));
                sb_opp2.set_value(MICRONS(rule->value(2)));
            }
        }
        show_widget(ru_region_la,   true);
        show_widget(ru_region_ent,  true);
        show_widget(ru_inside_la,   true);
        show_widget(ru_inside_ent,  true);
        show_widget(ru_outside_la,  true);
        show_widget(ru_outside_ent, true);
        show_widget(ru_target_la,   true);
        show_widget(ru_target_ent,  true);
        show_widget(ru_dimen_la,    true);
        show_widget(ru_dimen_sb,    true);
        show_widget(ru_enc_la,      true);
        show_widget(ru_enc_sb,      true);
        show_widget(ru_opp_la,      true);
        show_widget(ru_opp_sb1,     true);
        show_widget(ru_opp_sb2,     true);
        show_widget(ru_descr_la,    true);
        show_widget(ru_descr_ent,   true);
        break;

    case drMinOverlap:
    case drMinNoOverlap:
        gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), "");
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), "");
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_label_set_text(GTK_LABEL(ru_dimen_la),
            "Minimum dimension (microns)");
        gtk_entry_set_text(GTK_ENTRY(ru_target_ent), "");
        if (rule) {
            char *tstr = rule->targetString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_target_ent), tstr);
                delete [] tstr;
            }
        }
        show_widget(ru_region_la,   true);
        show_widget(ru_region_ent,  true);
        show_widget(ru_inside_la,   true);
        show_widget(ru_inside_ent,  true);
        show_widget(ru_outside_la,  true);
        show_widget(ru_outside_ent, true);
        show_widget(ru_target_la,   true);
        show_widget(ru_target_ent,  true);
        show_widget(ru_dimen_la,    true);
        show_widget(ru_dimen_sb,    true);
        show_widget(ru_descr_la,    true);
        show_widget(ru_descr_ent,   true);
        break;

    case drUserDefinedRule:
        gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), "");
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_inside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), "");
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                gtk_entry_set_text(GTK_ENTRY(ru_outside_ent), tstr);
                delete [] tstr;
            }
        }
        gtk_entry_set_text(GTK_ENTRY(ru_user_ent), "");
        if (rule) {
            const DRCtest *ur = rule->userRule();
            char *astr = ur ? ur->argString() : 0;
            if (astr) {
                gtk_entry_set_text(GTK_ENTRY(ru_user_ent), astr);
                delete [] astr;
            }
        }
        DRCtest *tst = DRC()->userTests();
        for ( ; tst; tst = tst->next()) {
            if (lstring::cieq(tst->name(), ru_username))
                break;
        }
        if (tst)
            sprintf(buf, "User-defined rule arguments (%d required)",
                tst->argc());
        else
            sprintf(buf,
                "User-defined rule arguments (warning: unknown rule)");
        gtk_label_set_text(GTK_LABEL(ru_user_la), buf);
        show_widget(ru_region_la,   true);
        show_widget(ru_region_ent,  true);
        show_widget(ru_inside_la,   true);
        show_widget(ru_inside_ent,  true);
        show_widget(ru_outside_la,  true);
        show_widget(ru_outside_ent, true);
        show_widget(ru_user_la,     true);
        show_widget(ru_user_ent,    true);
        show_widget(ru_descr_la,    true);
        show_widget(ru_descr_ent,   true);
        break;
    }
}


void
sRu::alloff()
{
    show_widget(ru_region_la,   false);
    show_widget(ru_region_ent,  false);
    show_widget(ru_target_la,   false);
    show_widget(ru_target_ent,  false);
    show_widget(ru_inside_la,   false);
    show_widget(ru_inside_ent,  false);
    show_widget(ru_outside_la,  false);
    show_widget(ru_outside_ent, false);
    show_widget(ru_dimen_la,    false);
    show_widget(ru_dimen_sb,    false);
    show_widget(ru_area_sb,     false);
    show_widget(ru_diag_la,     false);
    show_widget(ru_diag_sb,     false);
    show_widget(ru_net_la,      false);
    show_widget(ru_net_sb,      false);
    show_widget(ru_use_st,      false);
    show_widget(ru_edit_st,     false);
    show_widget(ru_enc_la,      false);
    show_widget(ru_enc_sb,      false);
    show_widget(ru_opp_la,      false);
    show_widget(ru_opp_sb1,     false);
    show_widget(ru_opp_sb2,     false);
    show_widget(ru_user_la,     false);
    show_widget(ru_user_ent,    false);
    show_widget(ru_descr_la,    false);
    show_widget(ru_descr_ent,   false);
}


namespace {
    char *get_string(GtkWidget *w)
    {
        const char *s = gtk_entry_get_text(GTK_ENTRY(w));
        if (s) {
            while (isspace(*s))
                s++;
            if (*s) {
                char *str = lstring::copy(s);
                char *e = str + strlen(str) - 1;
                while (e >= str && isspace(*e))
                    *e-- = 0;
                if (!*str) {
                    delete [] str;
                    return (0);
                }
                return (str);
            }
        }
        return (0);
    }


    // Add the SpaceTable record to lstr, but fix the IGNORE flag.  We
    // know the the string syntax is ok.
    //
    void add_stab_fix(const char *stabstr, bool ignore, sLstr &lstr)
    {
        const char *s = stabstr;
        char *tok = lstring::gettok(&s); // "SpacingTable"
        lstr.add_c(' ');
        lstr.add(tok);
        delete [] tok;
        tok = lstring::gettok(&s); // default space
        lstr.add_c(' ');
        lstr.add(tok);
        delete [] tok;
        tok = lstring::gettok(&s); // table dimensions
        lstr.add_c(' ');
        lstr.add(tok);
        delete [] tok;
        tok = lstring::gettok(&s); // flags
        unsigned long f = strtoul(tok, 0, 0);
        delete [] tok;
        if (ignore)
            f |= STF_IGNORE;
        else
            f &= ~STF_IGNORE;
        lstr.add_c(' ');
        lstr.add_h(f, true);
        lstr.add_c(' ');
        lstr.add(s);
    }
}


void
sRu::apply()
{
    const char *msg_target = "Error: the target layer must be provided.";
    const char *msg_dimen = "Error: the Dimension must be nonzero.";

    sLstr lstr;
    if (ru_rule == drUserDefinedRule)
        lstr.add(ru_username);
    else
        lstr.add(DRCtestDesc::ruleName(ru_rule));

    if (ru_rule != drExist) {
        char *rstr = get_string(ru_region_ent);
        if (rstr) {
            lstr.add_c(' ');
            lstr.add("Region ");
            lstr.add(rstr);
            delete [] rstr;
        }
    }

    switch (ru_rule) {
    case drNoRule:
        // Should not see this.
        return;

    case drExist:
    case drConnected:
        break;

    case drNoHoles:
        {
            if (sb_area.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(Dkw.MinArea());
                lstr.add_c(' ');
                lstr.add(sb_area.get_string());
            }
            if (sb_diag.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(Dkw.MinWidth());
                lstr.add_c(' ');
                lstr.add(sb_diag.get_string());
            }
        }
        break;

    case drOverlap:
    case drIfOverlap:
    case drNoOverlap:
    case drAnyOverlap:
    case drPartOverlap:
    case drAnyNoOverlap:
        {
            char *tstr = get_string(ru_target_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            else {
                PopUpMessage(msg_target, true);
                return;
            }
        }
        break;

    case drMinArea:
    case drMaxArea:
        {
            if (sb_area.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(sb_area.get_string());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
        }
        break;

    case drMinEdgeLength:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_target_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            else {
                PopUpMessage(msg_target, true);
                return;
            }
            if (sb_dimen.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(sb_dimen.get_string());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
        }
        break;

    case drMaxWidth:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            if (sb_dimen.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(sb_dimen.get_string());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
        }
        break;

    case drMinWidth:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            if (sb_dimen.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(sb_dimen.get_string());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
            if (sb_diag.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(Dkw.Diagonal());
                lstr.add_c(' ');
                lstr.add(sb_diag.get_string());
            }
        }
        break;

    case drMinSpace:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            if (ru_stabstr) {
                bool ignore = !GRX->GetStatus(ru_use_st);
                add_stab_fix(ru_stabstr, ignore, lstr);
            }
            else if (sb_dimen.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(sb_dimen.get_string());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
            if (sb_diag.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(Dkw.Diagonal());
                lstr.add_c(' ');
                lstr.add(sb_diag.get_string());
            }
            if (sb_net.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(Dkw.SameNet());
                lstr.add_c(' ');
                lstr.add(sb_net.get_string());
            }
        }
        break;

    case drMinSpaceTo:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_target_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            else {
                PopUpMessage(msg_target, true);
                return;
            }
            if (ru_stabstr) {
                bool ignore = !GRX->GetStatus(ru_use_st);
                add_stab_fix(ru_stabstr, ignore, lstr);
            }
            else if (sb_dimen.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(sb_dimen.get_string());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
            if (sb_diag.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(Dkw.Diagonal());
                lstr.add_c(' ');
                lstr.add(sb_diag.get_string());
            }
        }
        break;

    case drMinSpaceFrom:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_target_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            else {
                PopUpMessage(msg_target, true);
                return;
            }
            if (sb_dimen.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(sb_dimen.get_string());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
            if (sb_enc.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(Dkw.Enclosed());
                lstr.add_c(' ');
                lstr.add(sb_enc.get_string());
            }
            if (sb_opp1.get_value() > 0.0 || sb_opp2.get_value() > 0.0) {
                lstr.add_c(' ');
#undef Opposite
// Stupid thing in X.h.
                lstr.add(Dkw.Opposite());
                lstr.add_c(' ');
                lstr.add(sb_opp1.get_string());
                lstr.add_c(' ');
                lstr.add(sb_opp2.get_string());
            }
        }
        break;

    case drMinOverlap:
    case drMinNoOverlap:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_target_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            else {
                PopUpMessage(msg_target, true);
                return;
            }
            if (sb_dimen.get_value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(sb_dimen.get_string());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
        }
        break;

    case drUserDefinedRule:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            DRCtest *tst = DRC()->userTests();
            for ( ; tst; tst = tst->next()) {
                if (lstring::cieq(tst->name(), ru_username))
                    break;
            }
            if (!tst) {
                sLstr tlstr;
                tlstr.add("Internal error:  ");
                tlstr.add(ru_username);
                tlstr.add("is not found.");
                PopUpMessage(tlstr.string(), true);
                return;
            }
            char *astr = get_string(ru_user_ent);
            if (astr) {
                int ac = 0;
                char *t = astr;
                while ((lstring::advtok(&t)) != false) {
                    if (ac == tst->argc()) {
                        // Truncate any extra args.
                        *t = 0;
                        break;
                    }
                    ac++;
                }
                if (ac != tst->argc()) {
                    sLstr tlstr;
                    tlstr.add("Error:  too few arguments given for ");
                    tlstr.add(ru_username);
                    tlstr.add_c('.');
                    PopUpMessage(tlstr.string(), true);
                    delete [] astr;
                    return;
                }
                lstr.add_c(' ');
                lstr.add(astr);
                delete [] astr;
            }
        }
        break;
    }

    char *dstr = get_string(ru_descr_ent);
    if (dstr) {
        lstr.add_c(' ');
        lstr.add(dstr);
        delete [] dstr;
    }

    if (ru_callback) {
        if (!(*ru_callback)(lstr.string(), ru_arg))
            return;
    }
    DRC()->PopUpRuleEdit(0, MODE_OFF, drNoRule, 0, 0, 0, 0);
}


// Static function.
void
sRu::ru_cancel_proc(GtkWidget*, void*)
{
    DRC()->PopUpRuleEdit(0, MODE_OFF, drNoRule, 0, 0, 0, 0);
}


// Static function.
void
sRu::ru_action_proc(GtkWidget *caller, void*)
{
    if (!Ru)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:ruleedit"))
        return;
    }
    if (!strcmp(name, "Edit")) {
        Ru->PopUpStringEditor(Ru->ru_stabstr, sRu::ru_edit_cb, 0);
        return;
    }
    if (!strcmp(name, "Apply")) {
        Ru->apply();
        return;
    }
}


// Static function.
bool
sRu::ru_edit_cb(const char *string, void*, XEtype xet)
{
    if (!Ru)
        return (false);
    if (xet == XE_SAVE) {
        if (string) {
            const char *s = string;
            const char *err;
            sTspaceTable *t = sTspaceTable::tech_parse(&s, &err);
            if (!t) {
                if (err) {
                    Ru->PopUpErr(MODE_ON, err);
                    return (false);
                }
                // Empty return is ok.
                delete [] string;
                string = 0;
            }
            delete [] t;
        }
        delete [] Ru->ru_stabstr;
        Ru->ru_stabstr = (char*)string;  // Yes, this is a copy.
        return (true);
    }
    return (false);
}

