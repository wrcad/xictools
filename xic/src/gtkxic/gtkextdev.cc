
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
#include "ext_extract.h"
#include "dsp_inlines.h"
#include "promptline.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkfont.h"


//-------------------------------------------------------------------------
// Pop-up to control device highlighting and selection
//
// Help system keywords used:
//  xic:dvsel

namespace {
    namespace gtkextdev {
        struct sED
        {
            sED(GRobject);
            ~sED();

            GtkWidget *shell() { return (ed_popup); }

            void update();
            void relist();

        private:
            static void ed_cancel_proc(GtkWidget*, void*);
            static void ed_action_proc(GtkWidget*, void*);
            static int ed_selection_proc(GtkTreeSelection*, GtkTreeModel*,
                GtkTreePath*, int, void*);
            static bool ed_focus_proc(GtkWidget*, GdkEvent*, void*);

            GRobject ed_caller;
            GtkWidget *ed_popup;
            GtkWidget *ed_update;
            GtkWidget *ed_show;
            GtkWidget *ed_erase;
            GtkWidget *ed_show_all;
            GtkWidget *ed_erase_all;
            GtkWidget *ed_indices;
            GtkWidget *ed_list;
            GtkWidget *ed_select;
            GtkWidget *ed_compute;
            GtkWidget *ed_compare;
            GtkWidget *ed_measbox;
            GtkWidget *ed_paint;
            char *ed_selection;
            CDs *ed_sdesc;
            bool ed_devs_listed;
            bool ed_no_select;      // treeview focus hack

            static const char *nodevmsg;
        };

        sED *ED;
    }
}

using namespace gtkextdev;

const char *sED::nodevmsg = "No devices found.";


void
cExt::PopUpDevices(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete ED;
        return;
    }
    if (mode == MODE_UPD) {
        if (ED)
            ED->update();
        return;
    }
    if (ED)
        return;

    new sED(caller);
    if (!ED->shell()) {
        delete ED;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(ED->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UR), ED->shell(), mainBag()->Viewport());
    gtk_widget_show(ED->shell());
}


sED::sED(GRobject caller)
{
    ED = this;
    ed_caller = caller;
    ed_popup = 0;
    ed_update = 0;
    ed_show = 0;
    ed_erase = 0;
    ed_show_all = 0;
    ed_erase_all = 0;
    ed_indices = 0;
    ed_list = 0;
    ed_select = 0;
    ed_compute = 0;
    ed_compare = 0;
    ed_selection = 0;
    ed_sdesc = 0;
    ed_measbox = 0;
    ed_paint = 0;
    ed_devs_listed = false;
    ed_no_select = false;

    ed_popup = gtk_NewPopup(0, "Show/Select Devices", ed_cancel_proc, 0);
    if (!ed_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(ed_popup), false);

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(ed_popup), form);
    int rowcnt = 0;

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    ed_update = gtk_button_new_with_label("Update\nList");
    gtk_widget_set_name(ed_update, "Update");
    gtk_widget_show(ed_update);
    gtk_signal_connect(GTK_OBJECT(ed_update), "clicked",
        GTK_SIGNAL_FUNC(ed_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), ed_update, false, false, 0);

    GtkWidget *vbox = gtk_vbox_new(false, 2);
    gtk_widget_show(vbox);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, true, true, 0);

    GtkWidget *hbox1 = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox1);

    ed_show_all = gtk_button_new_with_label("Show All");
    gtk_widget_set_name(ed_show_all, "ShowAll");
    gtk_widget_show(ed_show_all);
    gtk_signal_connect(GTK_OBJECT(ed_show_all), "clicked",
        GTK_SIGNAL_FUNC(ed_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox1), ed_show_all, true, true, 0);

    ed_erase_all = gtk_button_new_with_label("Erase All");
    gtk_widget_set_name(ed_erase_all, "EraseAll");
    gtk_widget_show(ed_erase_all);
    gtk_signal_connect(GTK_OBJECT(ed_erase_all), "clicked",
        GTK_SIGNAL_FUNC(ed_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox1), ed_erase_all, true, true, 0);

    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ed_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox1), button, false, false, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox1, false, false, 0);

    hbox1 = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox1);

    ed_show = gtk_button_new_with_label("Show");
    gtk_widget_set_name(ed_show, "Show");
    gtk_widget_show(ed_show);
    gtk_signal_connect(GTK_OBJECT(ed_show), "clicked",
        GTK_SIGNAL_FUNC(ed_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox1), ed_show, true, true, 0);

    ed_erase = gtk_button_new_with_label("Erase");
    gtk_widget_set_name(ed_erase, "Erase");
    gtk_widget_show(ed_erase);
    gtk_signal_connect(GTK_OBJECT(ed_erase), "clicked",
        GTK_SIGNAL_FUNC(ed_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox1), ed_erase, true, true, 0);
    GtkWidget *label = gtk_label_new("Indices");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(hbox1), label, false, false, 0);

    ed_indices = gtk_entry_new();
    gtk_widget_set_name(ed_indices, "Indices");
    gtk_widget_show(ed_indices);
    gtk_box_pack_start(GTK_BOX(hbox1), ed_indices, true, true, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox1, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // scrolled list
    //
    GtkWidget *swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_set_border_width(GTK_CONTAINER(swin), 2);

    const char *title[3];
    title[0] = "Name";
    title[1] = "Prefix";
    title[2] = "Index Range";
    GtkListStore *store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING);
    ed_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_show(ed_list);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(ed_list), false);
    GtkCellRenderer *rnd = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *tvcol = gtk_tree_view_column_new_with_attributes(
        title[0], rnd, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ed_list), tvcol);
    tvcol = gtk_tree_view_column_new_with_attributes(
        title[1], rnd, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ed_list), tvcol);
    tvcol = gtk_tree_view_column_new_with_attributes(
        title[2], rnd, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ed_list), tvcol);

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(ed_list));
    gtk_tree_selection_set_select_function(sel, ed_selection_proc, 0, 0);
    // TreeView bug hack, see note with handlers.   
    gtk_signal_connect(GTK_OBJECT(ed_list), "focus",
        GTK_SIGNAL_FUNC(ed_focus_proc), this);

    gtk_container_add(GTK_CONTAINER(swin), ed_list);
    gtk_widget_set_size_request(ed_list, -1, 100);

    // Set up font and tracking.
    GTKfont::setupFont(ed_list, FNT_PROP, true);

    gtk_table_attach(GTK_TABLE(form), swin, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    // Frame and select devices group.
    //
    GtkWidget *tform = gtk_table_new(1, 1, false);
    gtk_widget_show(tform);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), tform);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    int ncnt = 0;

    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    ed_select = gtk_toggle_button_new_with_label("Enable\nSelect");
    gtk_widget_set_name(ed_select, "Select");
    gtk_widget_show(ed_select);
    gtk_signal_connect(GTK_OBJECT(ed_select), "clicked",
        GTK_SIGNAL_FUNC(ed_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), ed_select, true, true, 0);

    vbox = gtk_vbox_new(false, 2);
    gtk_widget_show(vbox);
    ed_compute = gtk_check_button_new_with_label(
        "Show computed parameters of selected device");
    gtk_widget_set_name(ed_compute, "Compute");
    gtk_widget_show(ed_compute);
    gtk_signal_connect(GTK_OBJECT(ed_compute), "clicked",
        GTK_SIGNAL_FUNC(ed_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(vbox), ed_compute, true, true, 0);

    ed_compare = gtk_check_button_new_with_label(
        "Show elec/phys comparison of selected device");
    gtk_widget_set_name(ed_compare, "Compare");
    gtk_widget_show(ed_compare);
    gtk_signal_connect(GTK_OBJECT(ed_compare), "clicked",
        GTK_SIGNAL_FUNC(ed_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(vbox), ed_compare, true, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, true, true, 0);

    gtk_table_attach(GTK_TABLE(tform), hbox, 0, 1, ncnt, ncnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // Frame and measure box group.
    //
    tform = gtk_table_new(1, 1, false);
    gtk_widget_show(tform);
    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), tform);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    ed_measbox = gtk_toggle_button_new_with_label("Enable Measure Box");
    gtk_widget_set_name(ed_measbox, "MeasBox");
    gtk_widget_show(ed_measbox);
    gtk_signal_connect(GTK_OBJECT(ed_measbox), "clicked",
        GTK_SIGNAL_FUNC(ed_action_proc), 0);
    gtk_table_attach(GTK_TABLE(tform), ed_measbox, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    ed_paint = gtk_button_new_with_label("Paint Box (use current layer)");
    gtk_widget_set_name(ed_paint, "Paint");
    gtk_widget_show(ed_paint);
    gtk_signal_connect(GTK_OBJECT(ed_paint), "clicked",
        GTK_SIGNAL_FUNC(ed_action_proc), 0);
    gtk_table_attach(GTK_TABLE(tform), ed_paint, 1, 2, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // Dismiss button.
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ed_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(ed_popup), button);

    update();
}


sED::~sED()
{
    // Must do this before zeroing pointer!
    if (GRX->GetStatus(ed_measbox))  
        GRX->CallCallback(ed_measbox);

    ED = 0;
    if (ed_caller)
        GRX->Deselect(ed_caller);

    CDs *cursdp = CurCell(Physical);
    if (cursdp) {
        cGroupDesc *gd = cursdp->groups();
        if (gd)
            gd->parse_find_dev(0, false);
    }

    if (ed_select && GRX->GetStatus(ed_select)) {
        GRX->SetStatus(ed_select, false);
        EX()->selectDevices(ed_select);
    }

    if (ed_popup) {
        gtk_signal_disconnect_by_func(GTK_OBJECT(ed_popup),
            GTK_SIGNAL_FUNC(ed_cancel_proc), ed_popup);
        gtk_widget_destroy(ed_popup);
    }
}


// Call when 1) current cell changes, 2) selection mode termination.
//
void
sED::update()
{

    if (!GRX->GetStatus(ed_select)) {
        gtk_widget_set_sensitive(ed_compute, false);
        gtk_widget_set_sensitive(ed_compare, false);
    }

    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        PL()->ShowPrompt("No current physical cell!");
        return;
    }
    if (!cursdp || cursdp != ed_sdesc || !cursdp->isAssociated()) {
        GtkListStore *store = GTK_LIST_STORE(
            gtk_tree_view_get_model(GTK_TREE_VIEW(ed_list)));
        gtk_list_store_clear(store);
        ed_devs_listed = false;
    }

    if (!ed_devs_listed) {
        gtk_widget_set_sensitive(ed_show, false);
        gtk_widget_set_sensitive(ed_erase, false);
        gtk_widget_set_sensitive(ed_show_all, false);
        gtk_widget_set_sensitive(ed_erase_all, false);
    }
    if (!GRX->GetStatus(ed_measbox))
        gtk_widget_set_sensitive(ed_paint, false);
    if (DSP()->CurMode() == Electrical)
        gtk_widget_set_sensitive(ed_measbox, false);
    else
        gtk_widget_set_sensitive(ed_measbox, true);
}


void
sED::relist()
{
    ed_sdesc = CurCell(Physical);
    if (!ed_sdesc) {
        PL()->ShowPrompt("No current physical cell!");
        return;
    }
    if (!EX()->extract(ed_sdesc)) {
        PL()->ShowPrompt("Extraction failed!");
        return;
    }

    // Run association, too, for device highlighting in electrical windows.
    // Not an error if this fails.
    EX()->associate(ed_sdesc);

    stringlist *devlist = 0;
    cGroupDesc *gd = ed_sdesc->groups();
    if (gd)
        devlist = gd->list_devs();
    if (!devlist) {
        devlist = new stringlist(lstring::copy(nodevmsg), 0);
        gtk_widget_set_sensitive(ed_show_all, false);
        gtk_widget_set_sensitive(ed_erase_all, false);
        ed_devs_listed = false;
    }
    else {
        gtk_widget_set_sensitive(ed_show_all, true);
        gtk_widget_set_sensitive(ed_erase_all, true);
        ed_devs_listed = true;
    }
    GtkListStore *store =
        GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ed_list)));
    gtk_list_store_clear(store);
    GtkTreeIter iter;
    for (stringlist *l = devlist; l; l = l->next) {
        gtk_list_store_append(store, &iter);

        char *ls[3];
        char *s = l->string;
        ls[0] = lstring::gettok(&s);
        ls[1] = lstring::gettok(&s);
        ls[2] = lstring::gettok(&s);
        gtk_list_store_set(store, &iter, 0, ls[0], 1, ls[1], 2, ls[2], -1);
        delete [] ls[0];
        delete [] ls[1];
        delete [] ls[2];
    }
    stringlist::destroy(devlist);
    delete [] ed_selection;
    ed_selection = 0;

    gtk_widget_set_sensitive(ed_show, false);
    gtk_widget_set_sensitive(ed_erase, false);
}


// Static function.
void
sED::ed_cancel_proc(GtkWidget*, void*)
{
    EX()->PopUpDevices(0, MODE_OFF);
}


// Static function.
void
sED::ed_action_proc(GtkWidget *caller, void*)
{
    if (!ED)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;

    if (!strcmp(name, "Update"))
        ED->relist();
    else if (!strcmp(name, "Show")) {
        if (!ED->ed_selection)
            return;
        CDs *cursdp = CurCell(Physical);
        if (!cursdp)
            return;
        cGroupDesc *gd = cursdp->groups();
        if (!gd)
            return;

        char *s = ED->ed_selection;
        char *dname = lstring::gettok(&s);
        char *pref = lstring::gettok(&s);
        const char *ind = gtk_entry_get_text(GTK_ENTRY(ED->ed_indices));
        sLstr lstr;
        if (dname) {
            lstr.add(dname);
            delete [] dname;
        }
        lstr.add_c('.');
        if (pref && strcmp(pref, EXT_NONE_TOK))
            lstr.add(pref);
        delete [] pref;
        lstr.add_c('.');
        if (ind) {
            while (isspace(*ind))
                ind++;
            if (*ind)
                lstr.add(ind);
        }

        gd->parse_find_dev(lstr.string(), true);
    }
    else if (!strcmp(name, "Erase")) {
        if (!ED->ed_selection)
            return;
        CDs *cursdp = CurCell(Physical);
        if (!cursdp)
            return;
        cGroupDesc *gd = cursdp->groups();
        if (!gd)
            return;

        char *s = ED->ed_selection;
        char *dname = lstring::gettok(&s);
        char *pref = lstring::gettok(&s);
        const char *ind = gtk_entry_get_text(GTK_ENTRY(ED->ed_indices));
        sLstr lstr;
        if (dname) {
            lstr.add(dname);
            delete [] dname;
        }
        lstr.add_c('.');
        if (pref && strcmp(pref, EXT_NONE_TOK))
            lstr.add(pref);
        delete [] pref;
        lstr.add_c('.');
        if (ind) {
            while (isspace(*ind))
                ind++;
            if (*ind)
                lstr.add(ind);
        }

        gd->parse_find_dev(lstr.string(), false);
    }
    else if (!strcmp(name, "ShowAll")) {
        CDs *cursdp = CurCell(Physical);
        if (!cursdp)
            return;
        cGroupDesc *gd = cursdp->groups();
        if (!gd)
            return;
        if (gd)
            gd->parse_find_dev(0, true);
    }
    else if (!strcmp(name, "EraseAll")) {
        CDs *cursdp = CurCell(Physical);
        if (!cursdp)
            return;
        cGroupDesc *gd = cursdp->groups();
        if (!gd)
            return;
        if (gd)
            gd->parse_find_dev(0, false);
    }
    else if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:dvsel"))
    }
    else if (!strcmp(name, "Select")) {
        if (GRX->GetStatus(ED->ed_select)) {
            if (!EX()->selectDevices(caller)) {
                GRX->Deselect(caller);
                return;
            }
            gtk_widget_set_sensitive(ED->ed_compute, true);
            gtk_widget_set_sensitive(ED->ed_compare, true);
        }
        else {
            EX()->selectDevices(caller);
            GRX->SetStatus(ED->ed_compute, false);
            GRX->SetStatus(ED->ed_compare, false);
            gtk_widget_set_sensitive(ED->ed_compute, false);
            gtk_widget_set_sensitive(ED->ed_compare, false);
            EX()->setDevselCompute(false);
            EX()->setDevselCompare(false);
        }
    }
    else if (!strcmp(name, "Compute"))
        EX()->setDevselCompute(GRX->GetStatus(caller));
    else if (!strcmp(name, "Compare"))
        EX()->setDevselCompare(GRX->GetStatus(caller));
    else if (!strcmp(name, "MeasBox")) {
        if (GRX->GetStatus(ED->ed_measbox)) {
            if (!EX()->measureLayerElectrical(caller)) {
                GRX->Deselect(caller);
                return;
            }
            gtk_widget_set_sensitive(ED->ed_paint, true);
        }
        else {
            EX()->measureLayerElectrical(caller);
            gtk_widget_set_sensitive(ED->ed_paint, false);
        }
    }
    else if (!strcmp(name, "Paint"))
        EX()->paintMeasureBox();
}


// Static function.
// Selection callback for the list.  This is called when a new selection
// is made, but not when the selection disappears, which happens when the
// list is updated.
//
int
sED::ed_selection_proc(GtkTreeSelection*, GtkTreeModel *store,
    GtkTreePath *path, int issel, void *)
{
    if (ED) {
        if (!ED->ed_devs_listed)
            return (false);
        if (ED->ed_no_select && !issel)
            return (false);
        char *name = 0, *pref = 0;
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(store, &iter, path))
            gtk_tree_model_get(store, &iter, 0, &name, 1, &pref, -1);
        if (!name || !pref) {
            free(name);
            free(pref);
            return (false);
        }
        if (!strcmp(name, "no") && !strcmp(pref, "devices")) {
            // First two tokens of nodevmsg.
            gtk_widget_set_sensitive(ED->ed_show, false);
            gtk_widget_set_sensitive(ED->ed_erase, false);
            free(name);
            free(pref);
            return (false);
        }
        if (issel) {
            gtk_widget_set_sensitive(ED->ed_show, true);
            gtk_widget_set_sensitive(ED->ed_erase, true);
            free(name);
            free(pref);
            return (true);
        }
        delete [] ED->ed_selection;
        ED->ed_selection = new char[strlen(name) + strlen(pref) + 2];
        char *t = lstring::stpcpy(ED->ed_selection, name);
        *t++ = ' ';
        strcpy(t, pref);
        gtk_widget_set_sensitive(ED->ed_show, true);
        gtk_widget_set_sensitive(ED->ed_erase, true);
        free(name);
        free(pref);
    }
    return (true);
}


// Static function.
// This handler is a hack to avoid a GtkTreeWidget defect:  when focus
// is taken and there are no selections, the 0'th row will be
// selected.  There seems to be no way to avoid this other than a hack
// like this one.  We set a flag to lock out selection changes in this
// case.
//
bool
sED::ed_focus_proc(GtkWidget*, GdkEvent*, void*)
{
    if (ED) {
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(ED->ed_list));
        // If nothing selected set the flag.
        if (!gtk_tree_selection_get_selected(sel, 0, 0))
            ED->ed_no_select = true;
    }
    return (false);
}

