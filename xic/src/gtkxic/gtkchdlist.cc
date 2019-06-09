
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

#include "config.h"
#include "main.h"
#include "cvrt.h"
#include "editif.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_alias.h"
#include "fio_library.h"
#include "fio_chd.h"
#include "cd_digest.h"
#include "events.h"
#include "menu.h"
#include "promptline.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkmcol.h"
#include "gtkinterf/gtkfont.h"
#include "miscutil/filestat.h"
#include "miscutil/pathlist.h"


//----------------------------------------------------------------------
//  Cell Hierarchy Digests Listing
//
// Help system keywords used:
//  xic:hier

namespace {
    namespace gtkchdlist {
        struct sCHL : public gtk_bag
        {
            sCHL(GRobject);
            ~sCHL();

            void update();

        private:
            void recolor();
            void action_hdlr(GtkWidget*, void*);
            void err_message(const char*);

            static void chl_cancel(GtkWidget*, void*);
            static void chl_action_proc(GtkWidget*, void*);
            static void chl_geom_proc(GtkWidget*, void*);
            static int chl_selection_proc(GtkTreeSelection*, GtkTreeModel*,
                GtkTreePath*, int, void*);
            static bool chl_focus_proc(GtkWidget*, GdkEvent*, void*);
            static bool chl_add_cb(const char*, const char*, int, void*);
            static bool chl_sav_cb(const char*, bool, void*);
            static void chl_del_cb(bool, void*);
            static bool chl_display_cb(bool, const BBox*, void*);
            static void chl_cnt_cb(const char*, void*);
            static ESret chl_cel_cb(const char*, void*);

            GRobject chl_caller;
            GtkWidget *chl_addbtn;
            GtkWidget *chl_savbtn;
            GtkWidget *chl_delbtn;
            GtkWidget *chl_cfgbtn;
            GtkWidget *chl_dspbtn;
            GtkWidget *chl_cntbtn;
            GtkWidget *chl_celbtn;
            GtkWidget *chl_infbtn;
            GtkWidget *chl_qinfbtn;
            GtkWidget *chl_list;
            GtkWidget *chl_loadtop;
            GtkWidget *chl_rename;
            GtkWidget *chl_usetab;
            GtkWidget *chl_showtab;
            GtkWidget *chl_failres;
            GtkWidget *chl_geomenu;
            GRledPopup *chl_cel_pop;
            GRmcolPopup *chl_cnt_pop;
            GRaffirmPopup *chl_del_pop;
            char *chl_selection;
            char *chl_contlib;
            bool chl_no_select;     // treeview focus hack
        };

        sCHL *CHL;

        enum { CHLnil, CHLadd, CHLsav, CHLdel, CHLcfg, CHLdsp, CHLcnt,
               CHLcel, CHLinf, CHLqinf, CHLhlp, CHLrenam, CHLltop, CHLutab,
               CHLetab, CHLfail };
    }
}

using namespace gtkchdlist;

// Contests pop-up buttons.
#define INFO_BTN "Info"
#define OPEN_BTN "Open"
#define PLACE_BTN "Place"


void
cConvert::PopUpHierarchies(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete CHL;
        return;
    }
    if (mode == MODE_UPD) {
        if (CHL)
            CHL->update();
        return;
    }
    if (CHL)
        return;

    new sCHL(caller);
    if (!CHL->Shell()) {
        delete CHL;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(CHL->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UL), CHL->Shell(), mainBag()->Viewport());
    gtk_widget_show(CHL->Shell());
}


sCHL::sCHL(GRobject c)
{
    CHL = this;
    chl_caller = c;
    chl_addbtn = 0;
    chl_savbtn = 0;
    chl_delbtn = 0;
    chl_cfgbtn = 0;
    chl_dspbtn = 0;
    chl_cntbtn = 0;
    chl_celbtn = 0;
    chl_infbtn = 0;
    chl_qinfbtn = 0;
    chl_list = 0;
    chl_loadtop = 0;
    chl_rename = 0;
    chl_usetab = 0;
    chl_showtab = 0;
    chl_failres = 0;
    chl_geomenu = 0;
    chl_cel_pop = 0;
    chl_cnt_pop = 0;
    chl_del_pop = 0;
    chl_selection = 0;
    chl_contlib = 0;
    chl_no_select = false;

    wb_shell = gtk_NewPopup(0, "Cell Hierarchy Digests", chl_cancel, 0);
    if (!wb_shell)
        return;
    gtk_window_set_default_size(GTK_WINDOW(wb_shell), 350, 150);

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    //
    // upper buttons
    //
    chl_addbtn = gtk_toggle_button_new_with_label("Add");
    gtk_widget_set_name(chl_addbtn, "Add");
    gtk_widget_show(chl_addbtn);
    gtk_signal_connect(GTK_OBJECT(chl_addbtn), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLadd);
    gtk_box_pack_start(GTK_BOX(hbox), chl_addbtn, true, true, 0);

    chl_savbtn = gtk_toggle_button_new_with_label("Save");
    gtk_widget_set_name(chl_savbtn, "Save");
    gtk_widget_show(chl_savbtn);
    gtk_signal_connect(GTK_OBJECT(chl_savbtn), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLsav);
    gtk_box_pack_start(GTK_BOX(hbox), chl_savbtn, true, true, 0);

    chl_delbtn = gtk_toggle_button_new_with_label("Delete");
    gtk_widget_set_name(chl_delbtn, "Remove");
    gtk_widget_show(chl_delbtn);
    gtk_signal_connect(GTK_OBJECT(chl_delbtn), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLdel);
    gtk_box_pack_start(GTK_BOX(hbox), chl_delbtn, true, true, 0);

    chl_cfgbtn = gtk_toggle_button_new_with_label("Config");
    gtk_widget_set_name(chl_cfgbtn, "Config");
    gtk_widget_show(chl_cfgbtn);
    gtk_signal_connect(GTK_OBJECT(chl_cfgbtn), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLcfg);
    gtk_box_pack_start(GTK_BOX(hbox), chl_cfgbtn, true, true, 0);

    chl_dspbtn = gtk_toggle_button_new_with_label("Display");
    gtk_widget_set_name(chl_dspbtn, "Display");
    gtk_widget_show(chl_dspbtn);
    gtk_signal_connect(GTK_OBJECT(chl_dspbtn), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLdsp);
    gtk_box_pack_start(GTK_BOX(hbox), chl_dspbtn, true, true, 0);

    chl_cntbtn = gtk_button_new_with_label("Contents");
    gtk_widget_set_name(chl_cntbtn, "Contents");
    gtk_widget_show(chl_cntbtn);
    gtk_signal_connect(GTK_OBJECT(chl_cntbtn), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLcnt);
    gtk_box_pack_start(GTK_BOX(hbox), chl_cntbtn, true, true, 0);

    chl_celbtn = gtk_toggle_button_new_with_label("Cell");
    gtk_widget_set_name(chl_celbtn, "Cell");
    gtk_widget_show(chl_celbtn);
    gtk_signal_connect(GTK_OBJECT(chl_celbtn), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLcel);
    gtk_box_pack_start(GTK_BOX(hbox), chl_celbtn, true, true, 0);

    chl_infbtn = gtk_button_new_with_label("Info");
    gtk_widget_set_name(chl_infbtn, "Info");
    gtk_widget_show(chl_infbtn);
    gtk_signal_connect(GTK_OBJECT(chl_infbtn), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLinf);
    gtk_box_pack_start(GTK_BOX(hbox), chl_infbtn, true, true, 0);

    chl_qinfbtn = gtk_button_new_with_label("?");
    gtk_widget_set_name(chl_qinfbtn, "Qinfo");
    gtk_widget_show(chl_qinfbtn);
    gtk_signal_connect(GTK_OBJECT(chl_qinfbtn), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLqinf);
    gtk_box_pack_start(GTK_BOX(hbox), chl_qinfbtn, true, true, 0);

    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLhlp);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    int rowcnt = 0;
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    if (DSP()->MainWdesc()->DbType() == WDchd)
        GRX->SetStatus(chl_dspbtn, true);

    //
    // scrolled list
    //
    GtkWidget *swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_set_border_width(GTK_CONTAINER(swin), 2);

    const char *title[3];
    title[0] = "Db Name";
    title[1] = "Linked CGD";
    title[2] = "Source, Default Cell - Click to select";
    GtkListStore *store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    chl_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_show(chl_list);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(chl_list), false);
    GtkCellRenderer *rnd = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *tvcol = gtk_tree_view_column_new_with_attributes(
        title[0], rnd, "text", 0, "background", 3, "foreground", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(chl_list), tvcol);
    tvcol = gtk_tree_view_column_new_with_attributes(
        title[1], rnd, "text", 1, "background", 3, "foreground", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(chl_list), tvcol);
    tvcol = gtk_tree_view_column_new_with_attributes(
        title[2], rnd, "text", 2, "background", 3, "foreground", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(chl_list), tvcol);

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(chl_list));
    gtk_tree_selection_set_select_function(sel, chl_selection_proc, 0, 0);
    // TreeView bug hack, see note with handlers.   
    gtk_signal_connect(GTK_OBJECT(chl_list), "focus",
        GTK_SIGNAL_FUNC(chl_focus_proc), this);

    gtk_container_add(GTK_CONTAINER(swin), chl_list);
    gtk_widget_set_usize(swin, 380, 120);

    // Set up font and tracking.
    GTKfont::setupFont(chl_list, FNT_PROP, true);

    gtk_table_attach(GTK_TABLE(form), swin, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // lower buttons
    //

    chl_rename = gtk_check_button_new_with_label(
          "Use auto-rename when writing CHD reference cells");
    gtk_widget_set_name(chl_rename, "rename");
    gtk_widget_show(chl_rename);
    gtk_signal_connect(GTK_OBJECT(chl_rename), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLrenam);
    gtk_table_attach(GTK_TABLE(form), chl_rename, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    chl_loadtop = gtk_check_button_new_with_label("Load top cell only");
    gtk_widget_set_name(chl_loadtop, "LoadTopCell");
    gtk_widget_show(chl_loadtop);
    gtk_signal_connect(GTK_OBJECT(chl_loadtop), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLltop);
    gtk_table_attach(GTK_TABLE(form), chl_loadtop, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    chl_failres = gtk_check_button_new_with_label("Fail on unresolved");
    gtk_widget_set_name(chl_failres, "FailOnUnresolved");
    gtk_widget_show(chl_failres);
    gtk_signal_connect(GTK_OBJECT(chl_failres), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLfail);
    gtk_table_attach(GTK_TABLE(form), chl_failres, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    chl_usetab = gtk_check_button_new_with_label("Use cell table");
    gtk_widget_set_name(chl_usetab, "UseCellTab");
    gtk_widget_show(chl_usetab);
    gtk_signal_connect(GTK_OBJECT(chl_usetab), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLutab);
    gtk_table_attach(GTK_TABLE(form), chl_usetab, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    chl_showtab = gtk_toggle_button_new_with_label("Edit Cell Table");
    gtk_widget_set_name(chl_showtab, "EditCellTab");
    gtk_widget_show(chl_showtab);
    gtk_signal_connect(GTK_OBJECT(chl_showtab), "clicked",
        GTK_SIGNAL_FUNC(chl_action_proc), (void*)CHLetab);
    gtk_table_attach(GTK_TABLE(form), chl_showtab, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Geom handling memu
    //
    GtkWidget *label = gtk_label_new("Default geometry handling");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    chl_geomenu = gtk_option_menu_new();
    gtk_widget_show(chl_geomenu);

    GtkWidget *menu = gtk_menu_new();
    gtk_widget_show(menu);
    GtkWidget *mi = gtk_menu_item_new_with_label("Create new MEMORY CGD");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(chl_geom_proc), (void*)(long)CHD_CGDmemory);
    mi = gtk_menu_item_new_with_label("Create new FILE CHD");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(chl_geom_proc), (void*)(long)CHD_CGDfile);
    mi = gtk_menu_item_new_with_label("Ignore geometry records");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(chl_geom_proc), (void*)(long)CHD_CGDnone);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(chl_geomenu), menu);

    gtk_table_attach(GTK_TABLE(form), chl_geomenu, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(chl_cancel), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    update();
}


sCHL::~sCHL()
{
    CHL = 0;
    delete [] chl_selection;
    delete [] chl_contlib;

    if (chl_caller)
        GRX->Deselect(chl_caller);
    Cvt()->PopUpChdOpen(0, MODE_OFF, 0, 0, 0, 0, 0, 0);
    Cvt()->PopUpChdSave(0, MODE_OFF, 0, 0, 0, 0, 0);
    Cvt()->PopUpChdConfig(0, MODE_OFF, 0, 0, 0);
    if (GRX->GetStatus(chl_showtab))
        Cvt()->PopUpAuxTab(0, MODE_OFF);
    if (chl_cnt_pop)
        chl_cnt_pop->popdown();
    if (chl_del_pop)
        chl_del_pop->popdown();
    Cvt()->PopUpDisplayWindow(0, MODE_OFF, 0, 0, 0);

    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(chl_cancel), wb_shell);
}


// Update the listing.
//
void
sCHL::update()
{
    if (!CHL)
        return;

    GRX->SetStatus(chl_loadtop, CDvdb()->getVariable(VA_ChdLoadTopOnly));
    GRX->SetStatus(chl_rename, CDvdb()->getVariable(VA_RefCellAutoRename));
    GRX->SetStatus(chl_usetab, CDvdb()->getVariable(VA_UseCellTab));
    GRX->SetStatus(chl_failres, CDvdb()->getVariable(VA_ChdFailOnUnresolved));
    gtk_option_menu_set_history(GTK_OPTION_MENU(chl_geomenu),
        sCHDin::get_default_cgd_type());

    if (chl_selection && !CDchd()->chdRecall(chl_selection, false)) {
        delete [] chl_selection;
        chl_selection = 0;
    }

    if (!chl_selection) {
        if (DSP()->MainWdesc()->DbType() != WDchd)
            gtk_widget_set_sensitive(chl_dspbtn, false);
        gtk_widget_set_sensitive(chl_savbtn, false);
        gtk_widget_set_sensitive(chl_delbtn, false);
        gtk_widget_set_sensitive(chl_cfgbtn, false);
        gtk_widget_set_sensitive(chl_cntbtn, false);
        gtk_widget_set_sensitive(chl_celbtn, false);
        gtk_widget_set_sensitive(chl_infbtn, false);
        gtk_widget_set_sensitive(chl_qinfbtn, false);
        if (chl_del_pop)
            chl_del_pop->popdown();
        Cvt()->PopUpChdSave(0, MODE_OFF, 0, 0, 0, 0, 0);
    }

    stringlist *names = CDchd()->chdList();
    stringlist::sort(names);

    int rowcnt = 0;
    int rowsel = -1;
    GtkListStore *store =
        GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(chl_list)));
    gtk_list_store_clear(store);
    GtkTreeIter iter;
    for (stringlist *l = names; l; l = l->next) {
        cCHD *chd = CDchd()->chdRecall(l->string, false);
        if (chd) {
            // List only CHDs that have a default physical cell.
            symref_t *p = chd->findSymref(0, Physical, true);
            if (p) {
                char *strings[3];
                strings[0] = l->string;
                strings[1] = lstring::copy(
                    chd->getCgdName() ? chd->getCgdName() : "");
                const char *fn = chd->filename() ?
                    lstring::strip_path(chd->filename()) : "";
                const char *cn = chd->defaultCell(Physical);
                if (!cn)
                    cn = "";
                strings[2] = new char[strlen(fn) + strlen(cn) + 10];
                sprintf(strings[2], "%s  %s", fn, cn);
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, 0, strings[0], 1, strings[1],
                    2, strings[2], 3, NULL, 4, NULL, -1);
                delete [] strings[1];
                delete [] strings[2];
                if (chl_selection && rowsel < 0 &&
                        !strcmp(chl_selection, l->string))
                    rowsel = rowcnt;
                rowcnt++;
            }
            else {
                Errs()->get_error();
                PopUpMessage(
                    "Error: CHD has no default physical cell, deleting.",
                    true);
                chd = CDchd()->chdRecall(l->string, true);
                delete chd;
            }
        }
    }
    // This resizes columns and the widget.
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(chl_list));
    if (rowsel >= 0) {
        GtkTreePath *p = gtk_tree_path_new_from_indices(rowsel, -1);
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(chl_list));
        gtk_tree_selection_select_path(sel, p);
        gtk_tree_path_free(p);
    }

    stringlist::destroy(names);
    recolor();
}


// Color the background of entry being displayed.
//
void
sCHL::recolor()
{
    const char *sclr = GRpkgIf()->GetAttrColor(GRattrColorLocSel);
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        GtkTreeIter iter;
        GtkListStore *store =
            GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(chl_list)));
        if (!store)
            return;
        if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
            return;
        for (int i = 0; ; i++) {
            char *name;
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &name, -1);
            int sc = strcmp(name, DSP()->MainWdesc()->DbName());
            free(name);
            if (!sc) {
                gtk_list_store_set(store, &iter, 3, sclr, 4, "black", -1);
                break;
            }
            if (!gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter))
                break;
        }
    }
    else {
        GtkTreeIter iter;
        GtkListStore *store =
            GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(chl_list)));
        if (!store)
            return;
        if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
            return;
        for (int i = 0; ; i++) {
            gtk_list_store_set(store, &iter, 3, NULL, 4, NULL, -1);
            if (!gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter))
                break;
        }
    }
    if (chl_cnt_pop) {
        if (DSP()->MainWdesc()->DbType() == WDchd)
            chl_cnt_pop->set_button_sens(0x1);
        else
            chl_cnt_pop->set_button_sens(0xff);
    }
}


// Handle the buttons.
//
void
sCHL::action_hdlr(GtkWidget *caller, void *client_data)
{
    if (client_data == (void*)CHLhlp) {
        DSPmainWbag(PopUpHelp("xic:hier"))
        return;
    }
    if (client_data == (void*)CHLinf) {
        if (chl_selection) {
            cCHD *chd = CDchd()->chdRecall(chl_selection, false);
            if (chd) {
                dspPkgIf()->SetWorking(true);
                char *string = chd->prInfo(0, DSP()->CurMode(), 0);

                char buf[256];
                sprintf(buf, "CHD name: %s\n", chl_selection);
                char *tmp = new char[strlen(buf) + strlen(string) + 1];
                char *e = lstring::stpcpy(tmp, buf);
                strcpy(e, string);
                delete [] string;
                string = tmp;

                PopUpInfo(MODE_ON, string, STY_FIXED);
                delete [] string;
                dspPkgIf()->SetWorking(false);
            }
        }
        return;
    }
    if (client_data == (void*)CHLqinf) {
        if (chl_selection) {
            cCHD *chd = CDchd()->chdRecall(chl_selection, false);
            if (chd) {
                sLstr lstr;
                lstr.add("CHD name       ");
                lstr.add(chl_selection);
                lstr.add_c('\n');
                lstr.add("Source File    ");
                lstr.add(chd->filename());
                lstr.add_c('\n');
                lstr.add("Source Type    ");
                lstr.add(FIO()->TypeName(chd->filetype()));
                lstr.add_c('\n');
                lstr.add("Def Phys Cell  ");
                lstr.add(chd->defaultCell(Physical));
                lstr.add_c('\n');
                if (chd->getCgdName()) {
                    lstr.add("Linked CGD     ");
                    lstr.add(chd->getCgdName());
                    lstr.add_c('\n');
                }
                PopUpInfo(MODE_ON, lstr.string(), STY_FIXED);
            }
        }
        return;
    }
    if (client_data == (void*)CHLcnt) {
        if (!chl_selection)
            return;
        delete [] chl_contlib;
        chl_contlib = 0;

        cCHD *chd = CDchd()->chdRecall(chl_selection, false);
        if (chd) {
            chl_contlib = lstring::copy(chl_selection);

            // The top-level cells are marked with an '*'.
            syrlist_t *allcells = chd->listing(DSP()->CurMode(), false);
            syrlist_t *topcells = chd->topCells(DSP()->CurMode());
            stringlist *s0 = 0, *se = 0;
            for (syrlist_t *x = allcells; x; x = x->next) {
                bool istop = false;
                for (syrlist_t *y = topcells; y; y = y->next) {
                    if (y->symref == x->symref) {
                        istop = true;
                        break;
                    }
                }
                char *t;
                if (!istop)
                    t = lstring::copy(Tstring(x->symref->get_name()));
                else {
                    t = new char[
                        strlen(Tstring(x->symref->get_name())) + 2];
                    *t = '*';
                    strcpy(t+1, Tstring(x->symref->get_name()));
                }
                if (!s0)
                    s0 = se = new stringlist(t, 0);
                else {
                    se->next = new stringlist(t, 0);
                    se = se->next;
                }
            }
            syrlist_t::destroy(allcells);
            syrlist_t::destroy(topcells);

            sLstr lstr;
            lstr.add(DisplayModeName(DSP()->CurMode()));
            lstr.add(" cells found in saved cell hierarchy\n");
            lstr.add(chl_selection);

            if (chl_cnt_pop)
                chl_cnt_pop->update(s0, lstr.string());
            else {
                const char *buttons[4];
                buttons[0] = INFO_BTN;
                buttons[1] = OPEN_BTN;
                buttons[2] = EditIf()->hasEdit() ? PLACE_BTN : 0;
                buttons[3] = 0;

                int pagesz = 0;
                const char *s = CDvdb()->getVariable(VA_ListPageEntries);
                if (s) {
                    pagesz = atoi(s);
                    if (pagesz < 100 || pagesz > 50000)
                        pagesz = 0;
                }
                chl_cnt_pop = DSPmainWbagRet(PopUpMultiCol(s0, lstr.string(),
                    chl_cnt_cb, 0, buttons, pagesz));
                if (chl_cnt_pop) {
                    chl_cnt_pop->register_usrptr((void**)&chl_cnt_pop);
                    chl_cnt_pop->register_caller(chl_cntbtn);
                    if (DSP()->MainWdesc()->DbType() == WDchd)
                        chl_cnt_pop->set_button_sens(0x1);
                    else
                        chl_cnt_pop->set_button_sens(0xff);
                }
            }
            stringlist::destroy(s0);
        }
        else
            err_message("Content scan failed:\n%s");
        return;
    }

    bool state = GRX->GetStatus(caller);

    if (client_data == (void*)CHLadd) {
        if (state) {
            int xo, yo;
            gdk_window_get_root_origin(Shell()->window, &xo, &yo);
            char *cn = CDchd()->newChdName();
            Cvt()->PopUpChdOpen(chl_addbtn, MODE_ON, cn, 0,
                xo + 20, yo + 100, chl_add_cb, 0);
            delete [] cn;
        }
        else
            Cvt()->PopUpChdOpen(0, MODE_OFF, 0, 0, 0, 0, 0, 0);
        return;
    }
    if (client_data == (void*)CHLsav) {
        if (state) {
            if (chl_del_pop)
                chl_del_pop->popdown();
            GRX->Deselect(chl_delbtn);
            if (chl_selection) {
                int xo, yo;
                gdk_window_get_root_origin(Shell()->window, &xo, &yo);
                Cvt()->PopUpChdSave(chl_savbtn, MODE_ON, chl_selection,
                    xo + 20, yo + 100, chl_sav_cb, 0);
            }
            else
                GRX->Deselect(chl_savbtn);
        }
        else
            Cvt()->PopUpChdSave(0, MODE_OFF, 0, 0, 0, 0, 0);
        return;
    }
    if (client_data == (void*)CHLdel) {
        if (chl_del_pop)
            chl_del_pop->popdown();
        if (state) {
            Cvt()->PopUpChdSave(0, MODE_OFF, 0, 0, 0, 0, 0);
            GRX->Deselect(chl_savbtn);
            if (chl_selection) {
                chl_del_pop = PopUpAffirm(chl_delbtn, GRloc(),
                    "Confirm - delete selected digest?", chl_del_cb,
                    lstring::copy(chl_selection));
                if (chl_del_pop)
                    chl_del_pop->register_usrptr((void**)&chl_del_pop);
            }
            else
                GRX->Deselect(chl_delbtn);
        }
        return;
    }
    if (client_data == (void*)CHLcfg) {
        if (state) {
            int xo, yo;
            gdk_window_get_root_origin(Shell()->window, &xo, &yo);
            Cvt()->PopUpChdConfig(chl_cfgbtn, MODE_ON, chl_selection,
                xo + 20, yo + 100);
        }
        else
            Cvt()->PopUpChdConfig(0, MODE_OFF, 0, 0, 0);
        return;
    }
    if (client_data == (void*)CHLdsp) {
        if (state) {
            if (!chl_selection) {
                GRX->Deselect(chl_dspbtn);
                return;
            }
            cCHD *chd = CDchd()->chdRecall(chl_selection, false);
            if (!chd) {
                GRX->Deselect(chl_dspbtn);
                return;
            }
            symref_t *p = chd->findSymref(0, Physical, true);
            if (!p) {
                PopUpMessage("CHD has no default cell!", true);
                return;
            }
            if (!chd->setBoundaries(p)) {
                CHL->err_message("Bounding box computation failed:\n%s");
                return;
            }
            BBox BB = *p->get_bb();
            BB.scale(1.1);
            Cvt()->PopUpDisplayWindow(0, MODE_ON, &BB, chl_display_cb, 0);
            gtk_widget_set_sensitive(wb_shell, false);
        }
        else {
            XM()->SetHierDisplayMode(0, 0, 0);
            if (!chl_selection)
                gtk_widget_set_sensitive(chl_dspbtn, false);
            Cvt()->PopUpDisplayWindow(0, MODE_OFF, 0, 0, 0);
            CHL->recolor();
        }
        return;
    }
    if (client_data == (void*)CHLcel) {
        if (chl_cel_pop)
            chl_cel_pop->popdown();
        if (state) {
            if (!chl_selection) {
                GRX->Deselect(chl_celbtn);
                return;
            }
            const char *cname = 0;
            cCHD *chd = CDchd()->chdRecall(chl_selection, false);
            if (chd)
                cname = chd->defaultCell(Physical);
            else {
                GRX->Deselect(chl_celbtn);
                return;
            }
            chl_cel_pop = PopUpEditString(chl_celbtn, GRloc(),
                "Reference cell name:", cname, chl_cel_cb, 0, 250, 0,
                false, 0);
            if (chl_cel_pop)
                chl_cel_pop->register_usrptr((void**)&chl_cel_pop);
        }
        return;
    }
    if (client_data == (void*)CHLltop) {
        if (state)
            CDvdb()->setVariable(VA_ChdLoadTopOnly, "");
        else
            CDvdb()->clearVariable(VA_ChdLoadTopOnly);
        return;
    }
    if (client_data == (void*)CHLrenam) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_RefCellAutoRename, "");
        else
            CDvdb()->clearVariable(VA_RefCellAutoRename);
        return;
    }
    if (client_data == (void*)CHLutab) {
        if (state)
            CDvdb()->setVariable(VA_UseCellTab, "");
        else
            CDvdb()->clearVariable(VA_UseCellTab);
        return;
    }
    if (client_data == (void*)CHLetab) {
        if (state) {
            Cvt()->PopUpAuxTab(0, MODE_OFF);
            Cvt()->PopUpAuxTab(chl_showtab, MODE_ON);
        }
        else
            Cvt()->PopUpAuxTab(0, MODE_OFF);
        return;
    }
    if (client_data == (void*)CHLfail) {
        if (state)
            CDvdb()->setVariable(VA_ChdFailOnUnresolved, "");
        else
            CDvdb()->clearVariable(VA_ChdFailOnUnresolved);
    }
}


void
sCHL::err_message(const char *fmt)
{
    const char *s = Errs()->get_error();
    int len = strlen(fmt) + (s ? strlen(s) : 0) + 10;
    char *t = new char[len];
    sprintf(t, fmt, s);
    PopUpMessage(t, true);
    delete [] t;
}


// Static function.
// Dismissal callback.
//
void
sCHL::chl_cancel(GtkWidget*, void*)
{
    Cvt()->PopUpHierarchies(0, MODE_OFF);
}


// Static function.
// Consolidated handler for the buttons.
//
void
sCHL::chl_action_proc(GtkWidget *caller, void *client_data)
{
    if (CHL)
        CHL->action_hdlr(caller, client_data);
}


// Static function.
// Handler for the geom handling menu.
//
void
sCHL::chl_geom_proc(GtkWidget*, void *client_data)
{
    if (!CHL)
        return;
    ChdCgdType tp = (ChdCgdType)(long)client_data;
    sCHDin::set_default_cgd_type(tp);
}


// Static function.
// Selection callback for the list.  This is called when a new selection
// is made, but not when the selection disappears, which happens when the
// list is updated.
//
int
sCHL::chl_selection_proc(GtkTreeSelection*, GtkTreeModel *store,
    GtkTreePath *path, int issel, void*)
{
    if (CHL) {
        if (issel)
            return (true);
        if (CHL->chl_no_select) {
            CHL->chl_no_select = false;
            return (false);
        }
        char *text = 0;
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(store, &iter, path))
            gtk_tree_model_get(store, &iter, 0, &text, -1);
        if  (text) {
            delete [] CHL->chl_selection;
            CHL->chl_selection = lstring::copy(text);
            gtk_widget_set_sensitive(CHL->chl_savbtn, true);
            gtk_widget_set_sensitive(CHL->chl_delbtn, true);
            gtk_widget_set_sensitive(CHL->chl_cfgbtn, true);
            if (DSP()->CurMode() == Physical)
                gtk_widget_set_sensitive(CHL->chl_dspbtn, true);
            gtk_widget_set_sensitive(CHL->chl_cntbtn, true);
            gtk_widget_set_sensitive(CHL->chl_celbtn, true);
            gtk_widget_set_sensitive(CHL->chl_infbtn, true);
            gtk_widget_set_sensitive(CHL->chl_qinfbtn, true);

            Cvt()->PopUpChdConfig(0, MODE_UPD, text, 0, 0);
            Cvt()->PopUpChdSave(0, MODE_UPD, text, 0, 0, 0, 0);
            free(text);
        }
        return (true);
    }
    return (false);
}


// Static function.
// This handler is a hack to avoid a GtkTreeWidget defect:  when focus
// is taken and there are no selections, the 0'th row will be
// selected.  There seems to be no way to avoid this other than a hack
// like this one.  We set a flag to lock out selection changes in this
// case.
//
bool
sCHL::chl_focus_proc(GtkWidget*, GdkEvent*, void*)
{
    if (CHL) {
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(CHL->chl_list));
        // If nothing selected set the flag.
        if (!gtk_tree_selection_get_selected(sel, 0, 0))
            CHL->chl_no_select = true;
    }
    return (false);
}
        

// Static function.
// Callback for the Add dialog.
//
bool
sCHL::chl_add_cb(const char *idname, const char *fname, int mode, void*)
{
    char buf[256];
    char *realname;
    FILE *fp = FIO()->POpen(fname, "r", &realname);
    if (!fp) {
        Errs()->sys_error("open");
        CHL->err_message("Open failed:\n%s");
        return (false);
    }
    if (idname && *idname && CDchd()->chdRecall(idname, false)) {
        sprintf(buf, "Access name %s is already in use.", idname);
        CHL->PopUpMessage(buf, true);
        return (false);
    }

    cCHD *chd = 0;
    FileType ft = FIO()->GetFileType(fp);
    fclose(fp);
    if (!FIO()->IsSupportedArchiveFormat(ft)) {

        // The file might be a saved digest file, if so, read it.
        sCHDin chd_in;
        if (!chd_in.check(realname)) {
            CHL->PopUpMessage("Not a supported file format.", true);
            delete [] realname;
            return (false);
        }
        dspPkgIf()->SetWorking(true);
        ChdCgdType tp = CHD_CGDmemory;
        if (mode == 1)
            tp = CHD_CGDfile;
        else if (mode == 2)
            tp = CHD_CGDnone;
        chd = chd_in.read(realname, tp);
        dspPkgIf()->SetWorking(false);
        if (!chd) {
            CHL->err_message("Read digest file failed:\n%s");
            delete [] realname;
            return (false);
        }
    }
    else {
        unsigned int alias_mask = (CVAL_CASE | CVAL_PFSF | CVAL_FILE);
        FIOaliasTab *tab = FIO()->NewReadingAlias(alias_mask);
        if (tab)
            tab->read_alias(realname);
        chd = FIO()->NewCHD(realname, ft, Electrical, tab, FIO()->CvtInfo());
        delete tab;
        delete [] realname;

        if (!chd) {
            CHL->err_message("Read failed:\n%s");
            return (false);
        }
    }

    if (!idname || !*idname)
        idname = CDchd()->newChdName();
    else
        idname = lstring::copy(idname);
    if (!CDchd()->chdStore(idname, chd)) {
        // Should never happen.
        char *n = CDchd()->newChdName();
        sprintf(buf, "Access name %s is in use, using new name %s.",
            idname, n);
        CHL->PopUpMessage(buf, false);
        CDchd()->chdStore(n, chd);
        delete [] n;
    }
    delete [] idname;

    CHL->update();
    return (true);
}


// Static function.
// Callback for the Save dialog.
//
bool
sCHL::chl_sav_cb(const char *fname, bool with_geom, void*)
{
    if (!CHL->chl_selection) {
        CHL->err_message("No selection, select a CHD in the listing.");
        return (false);
    }
    cCHD *chd = CDchd()->chdRecall(CHL->chl_selection, false);
    if (!chd) {
        Errs()->add_error("unresolved CHD name %s.", CHL->chl_selection);
        CHL->err_message("Error occurred:\n%s");
        return (false);
    }

    dspPkgIf()->SetWorking(true);
    sCHDout chd_out(chd);
    bool ok = chd_out.write(fname, with_geom ? CHD_WITH_GEOM : 0);
    dspPkgIf()->SetWorking(false);
    if (!ok) {
        CHL->err_message("Error occurred when writing digest file:\n%s");
        return (false);
    }
    CHL->PopUpMessage("Digest file saved successfully.", false);
    return (true);
}


// Static function.
// Callback for confirmation pop-up.
//
void
sCHL::chl_del_cb(bool yn, void *arg)
{
    if (arg && yn && CHL) {
        char *dbname = (char*)arg;
        cCHD *chd = CDchd()->chdRecall(dbname, true);
        delete chd;
        if (DSP()->MainWdesc()->DbType() == WDchd &&
                !strcmp(dbname, DSP()->MainWdesc()->DbName())) {
            GRX->Deselect(CHL->chl_dspbtn);
            XM()->SetHierDisplayMode(0, 0, 0);
        }
        CHL->update();
        delete [] dbname;
    }
}


// Static function.
// Callback for the display window pop-up.
//
bool
sCHL::chl_display_cb(bool working, const BBox *BB, void*)
{
    if (!CHL)
        return (true);
    if (working) {
        if (!CHL->chl_selection)
            return (true);
        cCHD *chd = CDchd()->chdRecall(CHL->chl_selection, false);
        if (!chd)
            return (true);

        XM()->SetHierDisplayMode(CHL->chl_selection,
            chd->defaultCell(Physical), BB);
        return (true);
    }

    // Strangeness here in GTK-2 for Red Hat 5.  Unless Deselect is
    // called before set_sensitive, the Display button will appear as
    // selected, but will revert to proper colors if, e.g., the mouse
    // cursor crosses the button area.

    GRX->Deselect(CHL->chl_dspbtn);
    gtk_widget_set_sensitive(CHL->wb_shell, true);
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        CHL->recolor();
        GRX->Select(CHL->chl_dspbtn);
    }
    return (true);
}


// Static function.
void
sCHL::chl_cnt_cb(const char *cellname, void*)
{
    if (!CHL)
        return;
    if (!CHL->chl_contlib || !CHL->chl_cnt_pop)
        return;
    if (!cellname)
        return;
    if (*cellname != '/')
        return;

    // Callback from button press.
    cellname++;
    if (!strcmp(cellname, INFO_BTN)) {
        cCHD *chd = CDchd()->chdRecall(CHL->chl_contlib, false);
        if (!chd)
            return;
        char *listsel = CHL->chl_cnt_pop->get_selection();
        symref_t *p = chd->findSymref(listsel, DSP()->CurMode(), true);
        delete [] listsel;
        if (!p)
            return;
        dspPkgIf()->SetWorking(true);
        stringlist *sl = new stringlist(
            lstring::copy(Tstring(p->get_name())), 0);
        int flgs = FIO_INFO_OFFSET | FIO_INFO_INSTANCES |
            FIO_INFO_BBS | FIO_INFO_FLAGS;
        char *str = chd->prCells(0, DSP()->CurMode(), flgs, sl);
        stringlist::destroy(sl);
        CHL->PopUpInfo(MODE_ON, str, STY_FIXED);
        dspPkgIf()->SetWorking(false);
    }
    else if (!strcmp(cellname, OPEN_BTN)) {
        char *sel = CHL->chl_cnt_pop->get_selection();
        if (sel) {
            EV()->InitCallback();
            XM()->EditCell(CHL->chl_contlib, false, FIO()->DefReadPrms(), sel);
            delete [] sel;
        }
    }
    else if (!strcmp(cellname, PLACE_BTN)) {
        char *sel = CHL->chl_cnt_pop->get_selection();
        if (sel) {
            EV()->InitCallback();
            EditIf()->addMaster(CHL->chl_contlib, sel);
            delete [] sel;
        }
    }
}


// Static function.
// Callback for the Cell dialog.
//
ESret
sCHL::chl_cel_cb(const char *cname, void*)
{
    if (!CHL->chl_selection)
        return (ESTR_DN);
    if (cname) {
        cCHD *chd = CDchd()->chdRecall(CHL->chl_selection, false);
        if (!chd)
            return (ESTR_DN);
        if (!chd->findSymref(cname, Physical)) {
            CHL->chl_cel_pop->update(
                "Cell name not found in CHD, try again: ", 0);
            return (ESTR_IGN);
        }
        if (!chd->createReferenceCell(cname)) {
            CHL->chl_cel_pop->update(
                "Error creating cell, try again:", 0);
            CHL->err_message("Error creating cell:\n%s");
            return (ESTR_IGN);
        }
        char buf[256];
        sprintf(buf, "Reference cell named %s created in memory.", cname);
        CHL->PopUpMessage(buf, false);
    }
    return (ESTR_DN);
}

