
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
#include "cvrt_variables.h"
#include "fio_layermap.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkfont.h"
#include <errno.h>


//--------------------------------------------------------------------
// Layer Alias Table Editor pop-up
//
// Help system keywords used:
//  layerchange

namespace {
    namespace gtkltalias {
        struct sLA : public gtk_bag
        {
            sLA(GRobject);
            ~sLA();

            void update();

            GRobject call_btn() { return (la_calling_btn); }

        private:
            static void la_cancel_proc(GtkWidget*, void*);
            static ESret str_cb(const char*, void*);
            static void yn_cb(bool, void*);
            static void la_action_proc(GtkWidget*, void*, unsigned);
            static bool la_select_proc(GtkTreeSelection*, GtkTreeModel*,
                GtkTreePath*, bool, void*);
            static bool la_focus_proc(GtkWidget*, GdkEvent*, void*);

            GRaffirmPopup *la_affirm;
            GRledPopup *la_line_edit;
            GtkWidget *la_list;
            GtkItemFactory *la_factory;
            GRobject la_calling_btn;
            int la_row;
            bool la_show_dec;
            bool la_no_select;      // treeview focus hack
        };

        sLA *LA;

        // function codes
        enum {NoCode, CancelCode, OpenCode, SaveCode, NewCode, DeleteCode,
            EditCode, DecCode, HelpCode };
    }
}

using namespace gtkltalias;


void
cMain::PopUpLayerAliases(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete LA;
        return;
    }
    if (mode == MODE_UPD) {
        if (LA)
            LA->update();
        return;
    }
    if (LA) {
        if (caller && caller != LA->call_btn())
            GRX->Deselect(caller);
        return;
    }

    new sLA(caller);
    if (!LA->Shell()) {
        delete LA;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(LA->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), LA->Shell(), mainBag()->Viewport());
    gtk_widget_show(LA->Shell());
}


#define IFINIT(i, a, b, c, d, e) { \
    menu_items[i].path = (char*)a; \
    menu_items[i].accelerator = (char*)b; \
    menu_items[i].callback = (GtkItemFactoryCallback)c; \
    menu_items[i].callback_action = d; \
    menu_items[i].item_type = (char*)e; \
    i++; }

sLA::sLA(GRobject c)
{
    LA = this;
    la_affirm = 0;
    la_line_edit = 0;
    la_list = 0;
    la_factory = 0;
    la_calling_btn = c;
    la_row = -1;
    la_show_dec = false;
    la_no_select = false;

    wb_shell = gtk_NewPopup(0, "Layer Aliases", la_cancel_proc, 0);
    if (!wb_shell)
        return;
    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    GtkItemFactoryEntry menu_items[20];
    int nitems = 0;

    IFINIT(nitems, "/_File", 0, 0, 0, "<Branch>")
    IFINIT(nitems, "/File/_Open", "<control>O", la_action_proc,
        OpenCode, 0);
    IFINIT(nitems, "/File/_Save", "<control>S", la_action_proc,
        SaveCode, 0);
    IFINIT(nitems, "/File/sep1", 0, 0, 0, "<Separator>");
    IFINIT(nitems, "/File/_Quit", "<control>Q", la_action_proc,
        CancelCode, 0);

    IFINIT(nitems, "/_Edit", 0, 0, 0, "<Branch>")
    IFINIT(nitems, "/Edit/_New", "<control>N", la_action_proc,
        NewCode, 0);
    IFINIT(nitems, "/Edit/_Delete", "<control>D", la_action_proc,
        DeleteCode, 0);
    IFINIT(nitems, "/Edit/_Edit", "<control>E", la_action_proc,
        EditCode, 0);
    IFINIT(nitems, "/Edit/Decimal _Form", "<control>F", la_action_proc,
        DecCode, "<CheckItem>");

    IFINIT(nitems, "/_Help", 0, 0, 0, "<LastBranch>");
    IFINIT(nitems, "/Help/_Help", "<control>H", la_action_proc,
        HelpCode, 0);

    GtkAccelGroup *accel_group = gtk_accel_group_new();
    la_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<aliases>",
        accel_group);
    for (int i = 0; i < nitems; i++)
        gtk_item_factory_create_item(la_factory, menu_items + i, 0, 2);
    gtk_window_add_accel_group(GTK_WINDOW(wb_shell), accel_group);

    GtkWidget *menubar = gtk_item_factory_get_widget(la_factory, "<aliases>");
    gtk_widget_show(menubar);

    // name the menubar objects
    GtkWidget *widget = gtk_item_factory_get_item(la_factory, "/File");
    if (widget)
        gtk_widget_set_name(widget, "File");
    widget = gtk_item_factory_get_item(la_factory, "/Edit");
    if (widget)
        gtk_widget_set_name(widget, "Edit");
    widget = gtk_item_factory_get_item(la_factory, "/Help");
    if (widget)
        gtk_widget_set_name(widget, "Help");

    widget = gtk_item_factory_get_item(la_factory, "/Edit/Delete");
    if (widget)
        gtk_widget_set_sensitive(widget, false);
    widget = gtk_item_factory_get_item(la_factory, "/Edit/Edit");
    if (widget)
        gtk_widget_set_sensitive(widget, false);

    gtk_table_attach(GTK_TABLE(form), menubar, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_set_border_width(GTK_CONTAINER(swin), 2);

    const char *title[2];
    title[0] = "Layer Name";
    title[1] = "Alias";
    GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    la_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_show(la_list);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(la_list), false);
    GtkCellRenderer *rnd = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *tvcol = gtk_tree_view_column_new_with_attributes(
        title[0], rnd, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(la_list), tvcol);
    tvcol = gtk_tree_view_column_new_with_attributes(
        title[1], rnd, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(la_list), tvcol);

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(la_list));
    gtk_tree_selection_set_select_function(sel,
        (GtkTreeSelectionFunc)la_select_proc, 0, 0);
    // TreeView bug hack, see note with handlers.   
    gtk_signal_connect(GTK_OBJECT(la_list), "focus",
        GTK_SIGNAL_FUNC(la_focus_proc), this);

    gtk_container_add(GTK_CONTAINER(swin), la_list);

    // Set up font and tracking.
    GTKfont::setupFont(la_list, FNT_PROP, true);

    gtk_table_attach(GTK_TABLE(form), swin, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(la_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    // Constrain overall widget width so title text isn't truncated.
    gtk_widget_set_usize(wb_shell, 220, 200);

    update();
}


sLA::~sLA()
{
    LA = 0;
    if (la_calling_btn)
        GRX->Deselect(la_calling_btn);
    if (la_factory)
        g_object_unref(la_factory);
}


void
sLA::update()
{
    FIOlayerAliasTab latab;
    latab.parse(CDvdb()->getVariable(VA_LayerAlias));
    char *s0 = latab.toString(la_show_dec);
    const char *str = s0;

    // We need to deselect before clearing, so that the deselection
    // signal is generated.
    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(la_list));
    gtk_tree_selection_unselect_all(sel);
    GtkListStore *store =
        GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(la_list)));
    gtk_list_store_clear(store);

    GtkTreeIter iter;
    char *stok;
    while ((stok = lstring::gettok(&str)) != 0) {
        char *t = strchr(stok, '=');
        if (t) {
            *t++ = 0;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, stok, 1, t, -1);
        }
        delete [] stok;
    }
    delete [] s0;
}


// Static function.
void
sLA::la_cancel_proc(GtkWidget*, void*)
{
    XM()->PopUpLayerAliases(0, MODE_OFF);
}


// Static function.
ESret
sLA::str_cb(const char *str, void *arg)
{
    if (arg == (void*)OpenCode) {
        if (str && *str) {
            char buf[256];
            FILE *fp = fopen(str, "r");
            if (!fp) {
                sprintf(buf, "Failed to open file\n%s", strerror(errno));
                LA->PopUpMessage(buf, true, false, true);
            }
            else {
                FIOlayerAliasTab latab;
                latab.readFile(fp);
                char *t = latab.toString(false);
                CDvdb()->setVariable(VA_LayerAlias, t);
                delete [] t;
                fclose(fp);
                LA->update();
            }
        }
        return (ESTR_DN);
    }
    if (arg == (void*)SaveCode) {
        if (str && *str) {
            char buf[256];
            FILE *fp = fopen(str, "w");
            if (!fp) {
                sprintf(buf, "Failed to open file\n%s", strerror(errno));
                LA->PopUpMessage(buf, true, false, true);
            }
            else {
                FIOlayerAliasTab latab;
                latab.parse(CDvdb()->getVariable(VA_LayerAlias));
                latab.dumpFile(fp);
                fclose(fp);
                LA->PopUpMessage("Layer alias data saved.", true, false, true);
            }
        }
        return (ESTR_DN);
    }
    if (arg == (void*)NewCode) {
        if (str && *str) {
            FIOlayerAliasTab latab;
            latab.parse(CDvdb()->getVariable(VA_LayerAlias));
            latab.parse(str);
            char *t = latab.toString(false);
            CDvdb()->setVariable(VA_LayerAlias, t);
            delete [] t;
            LA->update();
        }
        return (ESTR_IGN);
    }
    if (arg == (void*)EditCode) {
        if (str && *str) {
            const char *s0 = str;
            char *s = CDl::get_layer_tok(&str);
            FIOlayerAliasTab latab;
            latab.parse(CDvdb()->getVariable(VA_LayerAlias));
            latab.remove(s);
            latab.parse(s0);
            char *t = latab.toString(false);
            CDvdb()->setVariable(VA_LayerAlias, t);
            delete [] t;
            LA->update();
        }
        return (ESTR_IGN);
    }
    return (ESTR_IGN);
}


// Static function.
void
sLA::yn_cb(bool yn, void*)
{
    if (yn && LA->la_row >= 0) {
        GtkTreePath *p = gtk_tree_path_new_from_indices(LA->la_row, -1);
        GtkTreeModel *store = 
            gtk_tree_view_get_model(GTK_TREE_VIEW(LA->la_list));
        GtkTreeIter iter;
        gtk_tree_model_get_iter(store, &iter, p);
        char *text;
        gtk_tree_model_get(store, &iter, 0, &text, -1);
        gtk_tree_path_free(p);

        if (text) {
            FIOlayerAliasTab latab;
            latab.parse(CDvdb()->getVariable(VA_LayerAlias));
            latab.remove(text);
            free(text);
            char *t = latab.toString(false);
            if (t && *t)
                CDvdb()->setVariable(VA_LayerAlias, t);
            else {
                CDvdb()->clearVariable(VA_LayerAlias);
                CDvdb()->clearVariable(VA_UseLayerAlias);
            }
            delete [] t;
            LA->update();
        }
    }
}


// Static function.
void
sLA::la_action_proc(GtkWidget *caller, void*, unsigned code)
{
    int w, h;
    gdk_window_get_size(LA->wb_shell->window, &w, &h);
    GRloc loc(LW_XYR, w + 4, 0);
    if (code == CancelCode)
        XM()->PopUpLayerAliases(0, MODE_OFF);
    else if (code == OpenCode) {
        if (LA->la_line_edit)
            LA->la_line_edit->popdown();
        LA->la_line_edit = LA->PopUpEditString(0, loc,
            "Enter file name to read aliases", 0,
            str_cb, (void*)OpenCode, 200, 0, false, 0);
        if (LA->la_line_edit)
            LA->la_line_edit->register_usrptr((void**)&LA->la_line_edit);
    }
    else if (code == SaveCode) {
        if (LA->la_line_edit)
            LA->la_line_edit->popdown();
        LA->la_line_edit = LA->PopUpEditString(0, loc,
            "Enter file name to save aliases", 0,
            str_cb, (void*)SaveCode, 200, 0, false, 0);
        if (LA->la_line_edit)
            LA->la_line_edit->register_usrptr((void**)&LA->la_line_edit);
    }
    else if (code == NewCode) {
        if (LA->la_line_edit)
            LA->la_line_edit->popdown();
        LA->la_line_edit = LA->PopUpEditString(0, loc,
            "Enter layer name and alias", 0,
            str_cb, (void*)NewCode, 200, 0, false, 0);
        if (LA->la_line_edit)
            LA->la_line_edit->register_usrptr((void**)&LA->la_line_edit);
    }
    else if (code == DeleteCode) {
        if (LA->la_affirm)
            LA->la_affirm->popdown();
        LA->la_affirm = LA->PopUpAffirm(0, loc, "Delete selected entry?",
            yn_cb, 0);
        if (LA->la_affirm)
            LA->la_affirm->register_usrptr((void**)&LA->la_affirm);
    }
    else if (code == EditCode) {
        if (LA->la_row >= 0) {
            GtkTreePath *p = gtk_tree_path_new_from_indices(LA->la_row, -1);
            GtkTreeModel *store = 
                gtk_tree_view_get_model(GTK_TREE_VIEW(LA->la_list));
            GtkTreeIter iter;
            gtk_tree_model_get_iter(store, &iter, p);
            char *n, *a;
            gtk_tree_model_get(store, &iter, 0, &n, 1, &a, -1);
            gtk_tree_path_free(p);

            if (n && a) {
                char buf[128];
                sprintf(buf, "%s %s", n, a);
                if (LA->la_line_edit)
                    LA->la_line_edit->popdown();
                LA->la_line_edit = LA->PopUpEditString(0, loc,
                    "Enter layer name and alias", buf,
                    str_cb, (void*)EditCode, 200, 0, false, 0);
                if (LA->la_line_edit) {
                    LA->la_line_edit->register_usrptr(
                        (void**)&LA->la_line_edit);
                }
            }
            free(n);
            free(a);
        }
    }
    else if (code == DecCode) {
        LA->la_show_dec = GRX->GetStatus(caller);
        LA->update();
    }
    else if (code == HelpCode)
        LA->PopUpHelp("layerchange");
}


// Static function.
// Selection callback for the list.
//
bool
sLA::la_select_proc(GtkTreeSelection*, GtkTreeModel*, GtkTreePath *path,
    bool issel, void*)
{
    if (LA) {
        if (issel) {
            LA->la_row = -1;
            GtkWidget *widget =
                gtk_item_factory_get_item(LA->la_factory, "/Edit/Delete");
            if (widget)
                gtk_widget_set_sensitive(widget, false);
            widget = gtk_item_factory_get_item(LA->la_factory, "/Edit/Edit");
            if (widget)
                gtk_widget_set_sensitive(widget, false);
            return (true);
        }
        if (LA->la_no_select)
            return (false);
        int *indices = gtk_tree_path_get_indices(path);
        if (indices)
            LA->la_row = *indices;
        GtkWidget *widget =
            gtk_item_factory_get_item(LA->la_factory, "/Edit/Delete");
        if (widget)
            gtk_widget_set_sensitive(widget, true);
        widget = gtk_item_factory_get_item(LA->la_factory, "/Edit/Edit");
        if (widget)
            gtk_widget_set_sensitive(widget, true);
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
sLA::la_focus_proc(GtkWidget*, GdkEvent*, void*)
{
    if (LA) {
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(LA->la_list));
        // If nothing selected set the flag.
        if (!gtk_tree_selection_get_selected(sel, 0, 0))
            LA->la_no_select = true;
    }
    return (false);
}

