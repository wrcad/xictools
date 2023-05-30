
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
#include "cd_celldb.h"
#include "dsp_inlines.h"
#include "promptline.h"
#include "events.h"
#include "gtkmain.h"
#include "gtkcv.h"


//--------------------------------------------------------------------
// Pop-up to set current symbol table
//
// Help system keywords used:
//  xic:stabs

namespace {
    namespace gtkstab {
        struct sTb : public GTKbag
        {
            sTb(GRobject);
            ~sTb();

            void update();

        private:
            void action_proc(GtkWidget*);

            static void tb_cancel_proc(GtkWidget*, void*);
            static void tb_action(GtkWidget*, void*);
            static ESret tb_add_cb(const char*, void*);
            static void tb_clr_cb(bool, void*);
            static void tb_del_cb(bool, void*);
            static void tb_menu_proc(GtkWidget*, void*);

            GRobject tb_caller;
            GtkWidget *tb_tables;
            GtkWidget *tb_add;
            GtkWidget *tb_clr;
            GtkWidget *tb_del;

            GRledPopup *tb_add_pop;
            GRaffirmPopup *tb_clr_pop;
            GRaffirmPopup *tb_del_pop;
            stringlist *tb_namelist;
        };

        sTb *Tb;
    }
}

using namespace gtkstab;


void
cMain::PopUpSymTabs(GRobject caller, ShowMode mode)
{
    if (!GTKdev::exists() || !GTKmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete Tb;
        return;
    }
    if (mode == MODE_UPD) {
        if (Tb)
            Tb->update();
        return;
    }
    if (Tb)
        return;

    new sTb(caller);
    if (!Tb->Shell()) {
        delete Tb;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Tb->Shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GTKdev::self()->SetPopupLocation(GRloc(), Tb->Shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(Tb->Shell());
}


sTb::sTb(GRobject c)
{
    Tb = this;
    tb_caller = c;
    tb_tables = 0;
    tb_add = 0;
    tb_clr = 0;
    tb_del = 0;
    tb_add_pop = 0;
    tb_clr_pop = 0;
    tb_del_pop = 0;
    tb_namelist = 0;

    wb_shell = gtk_NewPopup(0, "Symbol Tables", tb_cancel_proc, 0);
    if (!wb_shell)
        return;
    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);

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
    GtkWidget *label = gtk_label_new(
        "Choose symbol table");
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
        G_CALLBACK(tb_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *entry = gtk_combo_box_text_new();
    gtk_widget_set_name(entry, "tables");
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(row), entry, true, true, 0);
    gtk_widget_set_size_request(entry, 100, -1);
    tb_tables = entry;

    stringlist *list = CDcdb()->listTables();
    for (stringlist *s = list; s; s = s->next)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry), s->string);
    tb_namelist = list;
    gtk_combo_box_set_active(GTK_COMBO_BOX(entry), 0);
    g_signal_connect(G_OBJECT(entry), "changed",
        G_CALLBACK(tb_menu_proc), 0);

    tb_add = gtk_toggle_button_new_with_label("Add");
    gtk_widget_set_name(tb_add, "add");
    gtk_widget_show(tb_add);
    g_signal_connect(G_OBJECT(tb_add), "clicked",
        G_CALLBACK(tb_action), 0);
    gtk_box_pack_start(GTK_BOX(row), tb_add, true, true, 0);

    tb_clr = gtk_toggle_button_new_with_label("Clear");
    gtk_widget_set_name(tb_clr, "clr");
    gtk_widget_show(tb_clr);
    g_signal_connect(G_OBJECT(tb_clr), "clicked",
        G_CALLBACK(tb_action), 0);
    gtk_box_pack_start(GTK_BOX(row), tb_clr, true, true, 0);

    tb_del = gtk_toggle_button_new_with_label("Destroy");
    gtk_widget_set_name(tb_del, "del");
    gtk_widget_show(tb_del);
    g_signal_connect(G_OBJECT(tb_del), "clicked",
        G_CALLBACK(tb_action), 0);
    gtk_box_pack_start(GTK_BOX(row), tb_del, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    if (!tb_namelist || !strcmp(tb_namelist->string, CD_MAIN_ST_NAME))
        gtk_widget_set_sensitive(tb_del, false);

    //
    // Dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tb_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);
}


sTb::~sTb()
{
    Tb = 0;
    stringlist::destroy(tb_namelist);
    if (tb_caller)
        GTKdev::Deselect(tb_caller);
}


void
sTb::update()
{
    stringlist *list = CDcdb()->listTables();

    bool changed = false;
    stringlist *s1 = list, *s2 = tb_namelist;
    while (s1 && s2) {
        if (strcmp(s1->string, s2->string)) {
            changed = true;
            break;
        }
        s1 = s1->next;
        s2 = s2->next;
    }
    if (s1 || s2)
        changed = true;

    if (changed) {
        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(
            GTK_COMBO_BOX(tb_tables))));
        for (stringlist *s = list; s; s = s->next) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(tb_tables),
                s->string);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(tb_tables), 0);
        stringlist::destroy(tb_namelist);
        tb_namelist = list;
    }
    else
        stringlist::destroy(list);
    if (!tb_namelist || !strcmp(tb_namelist->string, CD_MAIN_ST_NAME))
        gtk_widget_set_sensitive(tb_del, false);
    else
        gtk_widget_set_sensitive(tb_del, true);
}


void
sTb::action_proc(GtkWidget *caller)
{
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "add")) {
        if (tb_del_pop)
            tb_del_pop->popdown();
        if (tb_clr_pop)
            tb_clr_pop->popdown();
        if (tb_add_pop)
            tb_add_pop->popdown();
        tb_add_pop = PopUpEditString((GRobject)tb_add,
            GRloc(), "Enter name for new symbol table", 0, tb_add_cb, 0,
            250, 0, false, 0);
        if (tb_add_pop)
            tb_add_pop->register_usrptr((void**)&tb_add_pop);
    }
    else if (!strcmp(name, "clr")) {
        if (tb_add_pop)
            tb_add_pop->popdown();
        if (tb_clr_pop)
            tb_clr_pop->popdown();
        if (tb_del_pop)
            tb_del_pop->popdown();
        tb_clr_pop = PopUpAffirm(tb_clr, GRloc(),
            "Delete contents of current table?", tb_clr_cb, 0);
        if (tb_clr_pop)
            tb_clr_pop->register_usrptr((void**)&tb_clr_pop);
    }
    else if (!strcmp(name, "del")) {
        if (tb_add_pop)
            tb_add_pop->popdown();
        if (tb_clr_pop)
            tb_clr_pop->popdown();
        if (tb_del_pop)
            tb_del_pop->popdown();
        tb_del_pop = PopUpAffirm(tb_del, GRloc(),
            "Delete current table and contents?", tb_del_cb, 0);
        if (tb_del_pop)
            tb_del_pop->register_usrptr((void**)&tb_del_pop);
    }
    else if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:stabs"))
        return;
    }
}


// Static function.
void
sTb::tb_cancel_proc(GtkWidget*, void*)
{
    XM()->PopUpSymTabs(0, MODE_OFF);
}


// Static function.
void
sTb::tb_action(GtkWidget *caller, void*)
{
    if (Tb)
        Tb->action_proc(caller);
}


// Static function.
// Callback for the Add dialog.
//
ESret
sTb::tb_add_cb(const char *name, void*)
{
    if (name && Tb)
        XM()->SetSymbolTable(name);
    return (ESTR_DN);
}


// Static function.
// Callback for Clear confirmation pop-up.
//
void
sTb::tb_clr_cb(bool yn, void*)
{
    if (yn && Tb)
        XM()->Clear(0);
}


// Static function.
// Callback for Destroy confirmation pop-up.
//
void
sTb::tb_del_cb(bool yn, void*)
{
    if (yn && Tb)
        XM()->ClearSymbolTable();
}


// Static function.
void
sTb::tb_menu_proc(GtkWidget *caller, void*)
{
    if (Tb) {
        char *name = gtk_combo_box_text_get_active_text(
            GTK_COMBO_BOX_TEXT(caller));
        if (name) {
            XM()->SetSymbolTable(name);
            g_free(name);
        }
    }
}

