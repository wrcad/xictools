
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

//#define XXX_OPT

#include "main.h"
#include "edit.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkspinbtn.h"


//--------------------------------------------------------------------------
// Pop up for setting the current transform
//
// Help system keywords used:
//  xic:xform

namespace {
    namespace gtkxform {
        struct sTfm
        {
            sTfm(GRobject c,
                bool (*)(const char*, bool, const char*, void*), void*);
            ~sTfm();

            GtkWidget *shell() { return (tf_popup); }

            void update();

        private:
            static void tf_cancel_proc(GtkWidget*, void*);
            static void tf_action_proc(GtkWidget*, void*);
            static void tf_ang_proc(GtkWidget*, void*);
            static void tf_val_changed(GtkWidget*, void*);

            GRobject tf_caller;
            GtkWidget *tf_popup;
            GtkWidget *tf_rflx;
            GtkWidget *tf_rfly;
            GtkWidget *tf_ang;
            GtkWidget *tf_id;
            GtkWidget *tf_last;
            GtkWidget *tf_cancel;
            bool (*tf_callback)(const char*, bool, const char*, void*);
            void *tf_arg;

            GTKspinBtn sb_mag;
        };

        sTfm *Tfm;
    }
}

using namespace gtkxform;

// Magn spin button parameters
//
#define TFM_NUMD 5
#define TFM_MIND CDMAGMIN
#define TFM_MAXD CDMAGMAX
#define TFM_DEL  CDMAGMIN


// Pop-up to allow setting the mirror, rotation angle, and
// magnification parameters.  Also provides five storage registers for
// store/recall, an "ident" button to clear everything, and a "last"
// button to recall previous values.  Call with MODE_UPD to refresh
// after mode switch.
//
void
cEdit::PopUpTransform(GRobject caller, ShowMode mode,
    bool (*callback)(const char*, bool, const char*, void*), void *arg)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Tfm;
        return;
    }
    if (mode == MODE_UPD) {
        if (Tfm)
            Tfm->update();
        return;
    }
    if (Tfm)
        return;

    new sTfm(caller, callback, arg);
    if (!Tfm->shell()) {
        delete Tfm;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Tfm->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UL), Tfm->shell(), mainBag()->Viewport());
    gtk_widget_show(Tfm->shell());
}


sTfm::sTfm(GRobject c,
    bool (*callback)(const char*, bool, const char*, void*), void *arg)
{
    Tfm = this;
    tf_caller = c;
    tf_popup = 0;
    tf_rflx = 0;
    tf_rfly = 0;
    tf_ang = 0;
    tf_id = 0;
    tf_last = 0;
    tf_cancel = 0;
    tf_callback = callback;
    tf_arg = arg;

    tf_popup = gtk_NewPopup(0, "Current Transform", tf_cancel_proc, 0);
    if (!tf_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(tf_popup), false);

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(tf_popup), form);

    //
    // Label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new(
        "Set transform for new cells\nand move/copy.");
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
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    int rowcnt = 0;
    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // Rotation entry and mirror buttons
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    label = gtk_label_new("Angle");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);
#ifdef XXX_OPT
    GtkWidget *entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "rotat");
    gtk_widget_show(entry);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "rotat");

    int da = DSP()->CurMode() == Physical ? 45 : 90;
    int d = 0;
    while (d < 360) {
        char buf[16];
        sprintf(buf, "%d", d);
        GtkWidget *mi = gtk_menu_item_new_with_label(buf);
        gtk_widget_set_name(mi, buf);
        gtk_widget_show(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
        g_signal_connect(G_OBJECT(mi), "activate",
            G_CALLBACK(tf_ang_proc), 0);
        d += da;
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
#else
    GtkWidget *entry = gtk_combo_box_text_new();
    gtk_widget_set_name(entry, "rotat");
    gtk_widget_show(entry);

    /*
    int da = DSP()->CurMode() == Physical ? 45 : 90;
    int d = 0;
    while (d < 360) {
        char buf[16];
        sprintf(buf, "%d", d);
        d += da;
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry), buf);
    }
    */
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(tf_ang_proc), this);
#endif
    tf_ang = entry;
    gtk_box_pack_start(GTK_BOX(row), entry, false, false, 0);

    button = gtk_check_button_new_with_label("Reflect Y");
    gtk_widget_set_name(button, "rfly");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    tf_rfly = button;

    button = gtk_check_button_new_with_label("Reflect X");
    gtk_widget_set_name(button, "rflx");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    tf_rflx = button;

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Magnification label and spin button
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    label = gtk_label_new("Magnification");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

    GtkWidget *sb = sb_mag.init(1.0, TFM_MIND, TFM_MAXD, TFM_NUMD);
    sb_mag.connect_changed((GCallback)tf_val_changed, 0);

    char buf[64];
    sprintf(buf, "%.*f", TFM_NUMD, TFM_MAXD);
    int wid = sb_mag.width_for_string(buf);
    sprintf(buf, "%.*f", TFM_NUMD, TFM_MIND);
    int wid1 = sb_mag.width_for_string(buf);
    if (wid1 > wid)
        wid = wid1;
    gtk_widget_set_size_request(sb, wid, -1);

    gtk_box_pack_start(GTK_BOX(row), sb, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Identity and Last buttons
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    button = gtk_button_new_with_label("Identity Transform");
    gtk_widget_set_name(button, "ident");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    tf_id = button;
    button = gtk_button_new_with_label("Last Transform");
    gtk_widget_set_name(button, "last");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    tf_last = button;
    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Store buttons
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    button = gtk_button_new_with_label("Sto 1");
    gtk_widget_set_name(button, "sto1");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    button = gtk_button_new_with_label("Sto 2");
    gtk_widget_set_name(button, "sto2");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    button = gtk_button_new_with_label("Sto 3");
    gtk_widget_set_name(button, "sto3");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    button = gtk_button_new_with_label("Sto 4");
    gtk_widget_set_name(button, "sto4");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    button = gtk_button_new_with_label("Sto 5");
    gtk_widget_set_name(button, "sto5");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Recall buttons
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    button = gtk_button_new_with_label("Rcl 1");
    gtk_widget_set_name(button, "rcl1");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    button = gtk_button_new_with_label("Rcl 2");
    gtk_widget_set_name(button, "rcl2");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    button = gtk_button_new_with_label("Rcl 3");
    gtk_widget_set_name(button, "rcl3");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    button = gtk_button_new_with_label("Rcl 4");
    gtk_widget_set_name(button, "rcl4");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    button = gtk_button_new_with_label("Rcl 5");
    gtk_widget_set_name(button, "rcl5");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tf_cancel_proc), 0);
    tf_cancel = button;
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    update();
}


sTfm::~sTfm()
{
    Tfm = 0;
    if (tf_caller)
        GRX->Deselect(tf_caller);
    if (tf_callback)
        (*tf_callback)(0, false, 0, tf_arg);
    if (tf_popup)
        gtk_widget_destroy(tf_popup);
}


void
sTfm::update()
{
    GRX->SetStatus(tf_rflx, GEO()->curTx()->reflectX());
    GRX->SetStatus(tf_rfly, GEO()->curTx()->reflectY());
    bool has_tf = GEO()->curTx()->reflectX();
    has_tf |= GEO()->curTx()->reflectY();
    char buf[32];
    int da = DSP()->CurMode() == Physical ? 45 : 90;
    int d = 0;
#ifdef XXX_OPT
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "rotat");
    while (d < 360) {
        sprintf(buf, "%d", d);
        GtkWidget *mi = gtk_menu_item_new_with_label(buf);
        gtk_widget_set_name(mi, buf);
        gtk_widget_show(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
        g_signal_connect(G_OBJECT(mi), "activate",
            G_CALLBACK(tf_ang_proc), 0);
        d += da;
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(tf_ang), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(tf_ang),
        GEO()->curTx()->angle()/da);
#else
    // Clear the combo box, note how the empty list is detected.
    GtkTreeModel *mdl =
        gtk_combo_box_get_model(GTK_COMBO_BOX(tf_ang));
    GtkTreeIter iter;
    while (gtk_tree_model_get_iter_first(mdl, &iter))
        gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(tf_ang), 0);

    while (d < 360) {
        sprintf(buf, "%d", d);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tf_ang), buf);
        d += da;
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(tf_ang),
        GEO()->curTx()->angle()/da);
#endif
    has_tf |= GEO()->curTx()->angle();
    if (DSP()->CurMode() == Physical) {
        sb_mag.set_value(GEO()->curTx()->magn());
        sb_mag.set_sensitive(true);
        has_tf |= (GEO()->curTx()->magn() != 1.0);
    }
    else {
        sb_mag.set_value(1.0);
        sb_mag.set_sensitive(false);
    }

    if (has_tf)
        gtk_window_set_focus(GTK_WINDOW(tf_popup), tf_id);
    else
        gtk_window_set_focus(GTK_WINDOW(tf_popup), tf_last);
}


// Static function.
void
sTfm::tf_cancel_proc(GtkWidget*, void*)
{
    ED()->PopUpTransform(0, MODE_OFF, 0, 0);
}


// Static function.
void
sTfm::tf_action_proc(GtkWidget *widget, void*)
{
    if (Tfm && Tfm->tf_callback) {
        const char *name = gtk_widget_get_name(widget);
        if (!strcmp(name, "Help")) {
            DSPmainWbag(PopUpHelp("xic:xform"))
            return;
        }
        if (!strcmp(name, "ident")) {
            ED()->saveCurTransform(0);
            ED()->clearCurTransform();
            gtk_window_set_focus(GTK_WINDOW(Tfm->tf_popup), Tfm->tf_cancel);
            return;
        }
        if (!strcmp(name, "last")) {
            ED()->recallCurTransform(0);
            gtk_window_set_focus(GTK_WINDOW(Tfm->tf_popup), Tfm->tf_cancel);
            return;
        }
        if (!strcmp(name, "sto1")) {
            ED()->saveCurTransform(1);
            return;
        }
        if (!strcmp(name, "sto2")) {
            ED()->saveCurTransform(2);
            return;
        }
        if (!strcmp(name, "sto3")) {
            ED()->saveCurTransform(3);
            return;
        }
        if (!strcmp(name, "sto4")) {
            ED()->saveCurTransform(4);
            return;
        }
        if (!strcmp(name, "sto5")) {
            ED()->saveCurTransform(5);
            return;
        }
        if (!strcmp(name, "rcl1")) {
            ED()->saveCurTransform(0);
            ED()->recallCurTransform(1);
            return;
        }
        if (!strcmp(name, "rcl2")) {
            ED()->saveCurTransform(0);
            ED()->recallCurTransform(2);
            return;
        }
        if (!strcmp(name, "rcl3")) {
            ED()->saveCurTransform(0);
            ED()->recallCurTransform(3);
            return;
        }
        if (!strcmp(name, "rcl4")) {
            ED()->saveCurTransform(0);
            ED()->recallCurTransform(4);
            return;
        }
        if (!strcmp(name, "rcl5")) {
            ED()->saveCurTransform(0);
            ED()->recallCurTransform(5);
            return;
        }
        (*Tfm->tf_callback)(name, GRX->GetStatus(widget), 0, Tfm->tf_arg);
    }
}


// Static function.
void
sTfm::tf_ang_proc(GtkWidget *widget, void*)
{
    if (Tfm && Tfm->tf_callback) {
#ifdef XXX_OPT
        (*Tfm->tf_callback)("ang", true, gtk_widget_get_name(widget),
            Tfm->tf_arg);
#else
        char *t = gtk_combo_box_text_get_active_text(
            GTK_COMBO_BOX_TEXT(Tfm->tf_ang));
        if (t) {
            (*Tfm->tf_callback)("ang", true, t, Tfm->tf_arg);
            g_free(t);
        }
#endif
    }
}


// Static function.
void
sTfm::tf_val_changed(GtkWidget*, void*)
{
    if (Tfm && Tfm->tf_callback) {
        const char *s = Tfm->sb_mag.get_string();
        char *endp;
        double d = strtod(s, &endp);
        if (endp > s && d >= TFM_MIND && d <= TFM_MAXD)
            (*Tfm->tf_callback)("magn", true, s, Tfm->tf_arg);
    }
}

