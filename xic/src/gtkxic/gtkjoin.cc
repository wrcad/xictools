
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
#include "edit.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkspinbtn.h"
#include "geo_zlist.h"
#include "errorlog.h"


//--------------------------------------------------------------------
// Pop-up to control box/poly join operations.
//
// Help system keywords used:
//  xic:join

namespace {
    namespace gtkjoin {
        struct sJn
        {
            sJn(void*);
            ~sJn();

            void update();

            GtkWidget *shell() { return (jn_popup); }

        private:
            void set_sens(bool);
            static void jn_cancel_proc(GtkWidget*, void*);
            static void jn_action(GtkWidget*, void*);
            static void jn_val_changed(GtkWidget*, void*);

            GRobject jn_caller;
            GtkWidget *jn_popup;
            GtkWidget *jn_nolimit;
            GtkWidget *jn_clean;
            GtkWidget *jn_wires;
            GtkWidget *jn_join;
            GtkWidget *jn_join_lyr;
            GtkWidget *jn_join_all;
            GtkWidget *jn_split_h;
            GtkWidget *jn_split_v;
            GtkWidget *jn_mverts_label;
            GtkWidget *jn_mgroup_label;
            GtkWidget *jn_mqueue_label;

            GTKspinBtn sb_mverts;
            GTKspinBtn sb_mgroup;
            GTKspinBtn sb_mqueue;

            int jn_last_mverts;
            int jn_last_mgroup;
            int jn_last_mqueue;
            int jn_last;
        };

        sJn *Jn;
    }
}

using namespace gtkjoin;


void
cEdit::PopUpJoin(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Jn;
        return;
    }
    if (mode == MODE_UPD) {
        if (Jn)
            Jn->update();
        return;
    }
    if (Jn)
        return;

    new sJn(caller);
    if (!Jn->shell()) {
        delete Jn;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Jn->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UL), Jn->shell(), mainBag()->Viewport());
    gtk_widget_show(Jn->shell());
}


sJn::sJn(GRobject c)
{
    Jn = this;
    jn_caller = c;
    jn_popup = 0;
    jn_nolimit = 0;
    jn_clean = 0;
    jn_wires = 0;
    jn_join = 0;
    jn_join_lyr = 0;
    jn_join_all = 0;
    jn_split_h = 0;
    jn_split_v = 0;
    jn_mverts_label = 0;
    jn_mgroup_label = 0;
    jn_mqueue_label = 0;
    jn_last_mverts = DEF_JoinMaxVerts;
    jn_last_mgroup = DEF_JoinMaxGroup;
    jn_last_mqueue = DEF_JoinMaxQueue;
    jn_last = 0;

    jn_popup = gtk_NewPopup(0, "Join or Split Objects", jn_cancel_proc, 0);
    if (!jn_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(jn_popup), false);

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(jn_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new(
        "Set parameters or initiate join/split operation");
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
        GTK_SIGNAL_FUNC(jn_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // no limit check box
    //
    button = gtk_check_button_new_with_label(
        "No limits in join operation");
    gtk_widget_set_name(button, "NoLimit");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(jn_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    jn_nolimit = button;

    //
    // Max Verts spin button
    //
    jn_mverts_label = gtk_label_new("Maximum vertices in joined polygon");
    gtk_widget_show(jn_mverts_label);
    gtk_misc_set_padding(GTK_MISC(jn_mverts_label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), jn_mverts_label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *sb = sb_mverts.init(Zlist::JoinMaxVerts, 0, 8000, 0);
    sb_mverts.connect_changed(GTK_SIGNAL_FUNC(jn_val_changed), 0, "MaxVerts");
    gtk_widget_set_usize(sb, 60, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Max Group spin button
    //
    jn_mgroup_label = gtk_label_new("Maximum trapezoids per poly for join");
    gtk_widget_show(jn_mgroup_label);
    gtk_misc_set_padding(GTK_MISC(jn_mgroup_label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), jn_mgroup_label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_mgroup.init(Zlist::JoinMaxGroup, 0, 1e6, 0);
    sb_mgroup.connect_changed(GTK_SIGNAL_FUNC(jn_val_changed), 0, "MaxGroup");
    gtk_widget_set_usize(sb, 60, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Max Queue spin button
    //
    jn_mqueue_label = gtk_label_new("Trapezoid queue size for join");
    gtk_widget_show(jn_mqueue_label);
    gtk_misc_set_padding(GTK_MISC(jn_mqueue_label), 2, 2);
    gtk_table_attach(GTK_TABLE(form), jn_mqueue_label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_mqueue.init(Zlist::JoinMaxQueue, 0, 1e6, 0);
    sb_mqueue.connect_changed(GTK_SIGNAL_FUNC(jn_val_changed), 0, "MaxQueue");
    gtk_widget_set_usize(sb, 60, -1);

    gtk_table_attach(GTK_TABLE(form), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // break clean check box
    //
    button = gtk_check_button_new_with_label(
        "Clean break in join operation limiting");
    gtk_widget_set_name(button, "BreakClean");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(jn_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    jn_clean = button;

    //
    // include wires check box
    //
    button = gtk_check_button_new_with_label(
        "Include wires (as polygons) in join/split");
    gtk_widget_set_name(button, "Wires");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(jn_action), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    jn_wires = button;

    //
    // Command buttons
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    jn_join = gtk_button_new_with_label("Join");
    gtk_widget_set_name(jn_join, "Join");
    gtk_widget_show(jn_join);
    gtk_signal_connect(GTK_OBJECT(jn_join), "clicked",
        GTK_SIGNAL_FUNC(jn_action), 0);
    gtk_box_pack_start(GTK_BOX(row), jn_join, true, true, 0);

    jn_join_lyr = gtk_button_new_with_label("Join Lyr");
    gtk_widget_set_name(jn_join_lyr, "JoinLyr");
    gtk_widget_show(jn_join_lyr);
    gtk_signal_connect(GTK_OBJECT(jn_join_lyr), "clicked",
        GTK_SIGNAL_FUNC(jn_action), 0);
    gtk_box_pack_start(GTK_BOX(row), jn_join_lyr, true, true, 0);

    jn_join_all = gtk_button_new_with_label("Join All");
    gtk_widget_set_name(jn_join_all, "JoinAll");
    gtk_widget_show(jn_join_all);
    gtk_signal_connect(GTK_OBJECT(jn_join_all), "clicked",
        GTK_SIGNAL_FUNC(jn_action), 0);
    gtk_box_pack_start(GTK_BOX(row), jn_join_all, true, true, 0);

    jn_split_h = gtk_button_new_with_label("Split Horiz");
    gtk_widget_set_name(jn_split_h, "SplitH");
    gtk_widget_show(jn_split_h);
    gtk_signal_connect(GTK_OBJECT(jn_split_h), "clicked",
        GTK_SIGNAL_FUNC(jn_action), 0);
    gtk_box_pack_start(GTK_BOX(row), jn_split_h, true, true, 0);

    jn_split_v = gtk_button_new_with_label("Split Vert");
    gtk_widget_set_name(jn_split_v, "SplitV");
    gtk_widget_show(jn_split_v);
    gtk_signal_connect(GTK_OBJECT(jn_split_v), "clicked",
        GTK_SIGNAL_FUNC(jn_action), 0);
    gtk_box_pack_start(GTK_BOX(row), jn_split_v, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(jn_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(jn_popup), button);

    update();
}


sJn::~sJn()
{
    Jn = 0;
    if (jn_caller)
        GRX->Deselect(jn_caller);
    if (jn_popup)
        gtk_widget_destroy(jn_popup);
}


void
sJn::update()
{
    GRX->SetStatus(jn_clean, CDvdb()->getVariable(VA_JoinBreakClean) != 0);
    GRX->SetStatus(jn_wires, CDvdb()->getVariable(VA_JoinSplitWires) != 0);

    if (Zlist::JoinMaxVerts || Zlist::JoinMaxGroup || Zlist::JoinMaxQueue) {
        GRX->SetStatus(jn_nolimit, false);
        set_sens(true);

        const char *s = sb_mverts.get_string();
        int n;
        if (sscanf(s, "%d", &n) != 1 || n != Zlist::JoinMaxVerts)
            sb_mverts.set_value(Zlist::JoinMaxVerts);

        s = sb_mgroup.get_string();
        if (sscanf(s, "%d", &n) != 1 || n != Zlist::JoinMaxGroup)
            sb_mgroup.set_value(Zlist::JoinMaxGroup);

        s = sb_mqueue.get_string();
        if (sscanf(s, "%d", &n) != 1 || n != Zlist::JoinMaxQueue)
            sb_mqueue.set_value(Zlist::JoinMaxQueue);

    }
    else {
        GRX->SetStatus(jn_nolimit, true);
        sb_mverts.set_value(0);
        sb_mgroup.set_value(0);
        sb_mqueue.set_value(0);
        set_sens(false);
    }
}


void
sJn::set_sens(bool sens)
{
    sb_mverts.set_sensitive(sens, true);
    sb_mgroup.set_sensitive(sens, true);
    sb_mqueue.set_sensitive(sens, true);
    gtk_widget_set_sensitive(jn_mverts_label, sens);
    gtk_widget_set_sensitive(jn_mgroup_label, sens);
    gtk_widget_set_sensitive(jn_mqueue_label, sens);
    gtk_widget_set_sensitive(jn_clean, sens);
}


// Static function.
void
sJn::jn_cancel_proc(GtkWidget*, void*)
{
    ED()->PopUpJoin(0, MODE_OFF);
}


// Static function.
void
sJn::jn_action(GtkWidget *caller, void*)
{
    if (!Jn)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "Help"))
        DSPmainWbag(PopUpHelp("xic:join"))
    else if (!strcmp(name, "NoLimit")) {
        if (GRX->GetStatus(caller)) {
            Jn->jn_last_mverts = Jn->sb_mverts.get_value_as_int();
            Jn->jn_last_mgroup = Jn->sb_mgroup.get_value_as_int();
            Jn->jn_last_mqueue = Jn->sb_mqueue.get_value_as_int();
            if (DEF_JoinMaxVerts == 0)
                CDvdb()->clearVariable(VA_JoinMaxPolyVerts);
            else
                CDvdb()->setVariable(VA_JoinMaxPolyVerts, "0");
            if (DEF_JoinMaxGroup == 0)
                CDvdb()->clearVariable(VA_JoinMaxPolyGroup);
            else
                CDvdb()->setVariable(VA_JoinMaxPolyGroup, "0");
            if (DEF_JoinMaxQueue == 0)
                CDvdb()->clearVariable(VA_JoinMaxPolyQueue);
            else
                CDvdb()->setVariable(VA_JoinMaxPolyQueue, "0");
        }
        else {
            Jn->sb_mverts.set_value(Jn->jn_last_mverts);
            Jn->sb_mgroup.set_value(Jn->jn_last_mgroup);
            Jn->sb_mqueue.set_value(Jn->jn_last_mqueue);
        }
    }
    else if (!strcmp(name, "BreakClean")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_JoinBreakClean, "");
        else
            CDvdb()->clearVariable(VA_JoinBreakClean);
    }
    else if (!strcmp(name, "Wires")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_JoinSplitWires, "");
        else
            CDvdb()->clearVariable(VA_JoinSplitWires);
    }
    else {
        bool ret = true;
        if (!strcmp(name, "Join"))
            ret = ED()->joinCmd();
        else if (!strcmp(name, "JoinLyr"))
            ret = ED()->joinLyrCmd();
        else if (!strcmp(name, "JoinAll"))
            ret = ED()->joinAllCmd();
        else if (!strcmp(name, "SplitH"))
            ret = ED()->splitCmd(false);
        else if (!strcmp(name, "SplitV"))
            ret = ED()->splitCmd(true);
        if (!ret)
            Log()->PopUpErr(Errs()->get_error());
    }
}


// Static function.
void
sJn::jn_val_changed(GtkWidget *caller, void*)
{
    if (!Jn)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "MaxVerts")) {
        // Skip over 1-19 in spin button display, these values are
        // no good.
        int n = Jn->sb_mverts.get_value_as_int();
        if (n >= 1 && n < 20) {
            if (Jn->jn_last > n)
                Jn->sb_mverts.set_value(0.0);
            else
                Jn->sb_mverts.set_value(20.0);
            return;
        }
        Jn->jn_last = n;

        const char *s = Jn->sb_mverts.get_string();
        if (sscanf(s, "%d", &n) == 1 && (n == 0 || (n >= 20 && n <= 8000))) {
            if (n == DEF_JoinMaxVerts)
                CDvdb()->clearVariable(VA_JoinMaxPolyVerts);
            else
                CDvdb()->setVariable(VA_JoinMaxPolyVerts, s);
        }
    }
    else if (!strcmp(name, "MaxGroup")) {
        const char *s = Jn->sb_mgroup.get_string();
        int n;
        if (sscanf(s, "%d", &n) == 1 && n >= 0) {
            if (n == DEF_JoinMaxGroup)
                CDvdb()->clearVariable(VA_JoinMaxPolyGroup);
            else
                CDvdb()->setVariable(VA_JoinMaxPolyGroup, s);
        }
    }
    else if (!strcmp(name, "MaxQueue")) {
        const char *s = Jn->sb_mqueue.get_string();
        int n;
        if (sscanf(s, "%d", &n) == 1 && n >= 0) {
            if (n == DEF_JoinMaxQueue)
                CDvdb()->clearVariable(VA_JoinMaxPolyQueue);
            else
                CDvdb()->setVariable(VA_JoinMaxPolyQueue, s);
        }
    }
}

