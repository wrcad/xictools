
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
#include "cd_digest.h"
#include "fio.h"
#include "fio_alias.h"
#include "fio_library.h"
#include "fio_chd.h"
#include "fio_oasis.h"
#include "fio_cgd.h"
#include "events.h"
#include "menu.h"
#include "cvrt_menu.h"
#include "promptline.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtklist.h"
#include "gtkinterf/gtkfont.h"
#include "gtkinterf/gtkutil.h"
#include "miscutil/filestat.h"
#include "miscutil/pathlist.h"


//----------------------------------------------------------------------
//  Geometry Digests Popup
//
// Help system keywords used:
//  xic:geom

namespace {
    namespace gtkcgdlist {
        struct sCGL : public gtk_bag
        {
            sCGL(GRobject);
            ~sCGL();

            void update();

        private:
            void action_hdlr(GtkWidget*, void*);
            void err_message(const char*);

            static bool cgl_selection_proc(GtkTreeSelection*, GtkTreeModel*,
                GtkTreePath*, bool, void*);
            static bool cgl_focus_proc(GtkWidget*, GdkEvent*, void*);
            static bool cgl_add_cb(const char*, const char*, int, void*);
            static ESret cgl_sav_cb(const char*, void*);
            static void cgl_del_cb(bool, void*);
            static void cgl_cnt_cb(const char*, void*);
            static void cgl_action_proc(GtkWidget*, void*);
            static void cgl_cancel(GtkWidget*, void*);

            GRobject cgl_caller;
            GtkWidget *cgl_addbtn;
            GtkWidget *cgl_savbtn;
            GtkWidget *cgl_delbtn;
            GtkWidget *cgl_cntbtn;
            GtkWidget *cgl_infbtn;
            GtkWidget *cgl_list;
            GRledPopup *cgl_sav_pop;
            GRaffirmPopup *cgl_del_pop;
            GRmcolPopup *cgl_cnt_pop;
            GRmcolPopup *cgl_inf_pop;
            char *cgl_selection;
            char *cgl_contlib;
            bool cgl_no_select;     // treeview focus hack
        };

        sCGL *CGL;

        enum { CGLnil, CGLadd, CGLsav, CGLdel, CGLcnt, CGLinf, CGLhlp };
    }
}

using namespace gtkcgdlist;

// Contests pop-up button.
#define INFO_BTN "Info"


void
cConvert::PopUpGeometries(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete CGL;
        return;
    }
    if (mode == MODE_UPD) {
        if (CGL)
            CGL->update();
        return;
    }
    if (CGL)
        return;

    new sCGL(caller);
    if (!CGL->Shell()) {
        delete CGL;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(CGL->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UL), CGL->Shell(), mainBag()->Viewport());
    gtk_widget_show(CGL->Shell());
}


sCGL::sCGL(GRobject c)
{
    CGL = this;
    cgl_caller = c;
    cgl_addbtn = 0;
    cgl_savbtn = 0;
    cgl_delbtn = 0;
    cgl_cntbtn = 0;
    cgl_infbtn = 0;
    cgl_list = 0;
    cgl_sav_pop = 0;
    cgl_del_pop = 0;
    cgl_cnt_pop = 0;
    cgl_inf_pop = 0;
    cgl_selection = 0;
    cgl_contlib = 0;
    cgl_no_select = false;

    wb_shell = gtk_NewPopup(0, "Cell Geometry Digests", cgl_cancel, 0);
    if (!wb_shell)
        return;
    gtk_window_set_default_size(GTK_WINDOW(wb_shell), 380, 150);

    GtkWidget *form = gtk_table_new(1, 5, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    cgl_addbtn = gtk_toggle_button_new_with_label("Add");
    gtk_widget_set_name(cgl_addbtn, "Add");
    gtk_widget_show(cgl_addbtn);
    gtk_signal_connect(GTK_OBJECT(cgl_addbtn), "clicked",
        GTK_SIGNAL_FUNC(cgl_action_proc), (void*)CGLadd);
    gtk_box_pack_start(GTK_BOX(hbox), cgl_addbtn, true, true, 0);

    cgl_savbtn = gtk_toggle_button_new_with_label("Save");
    gtk_widget_set_name(cgl_savbtn, "Save");
    gtk_widget_show(cgl_savbtn);
    gtk_signal_connect(GTK_OBJECT(cgl_savbtn), "clicked",
        GTK_SIGNAL_FUNC(cgl_action_proc), (void*)CGLsav);
    gtk_box_pack_start(GTK_BOX(hbox), cgl_savbtn, true, true, 0);

    cgl_delbtn = gtk_toggle_button_new_with_label("Delete");
    gtk_widget_set_name(cgl_delbtn, "Delete");
    gtk_widget_show(cgl_delbtn);
    gtk_signal_connect(GTK_OBJECT(cgl_delbtn), "clicked",
        GTK_SIGNAL_FUNC(cgl_action_proc), (void*)CGLdel);
    gtk_box_pack_start(GTK_BOX(hbox), cgl_delbtn, true, true, 0);

    cgl_cntbtn = gtk_button_new_with_label("Contents");
    gtk_widget_set_name(cgl_cntbtn, "Contents");
    gtk_widget_show(cgl_cntbtn);
    gtk_signal_connect(GTK_OBJECT(cgl_cntbtn), "clicked",
        GTK_SIGNAL_FUNC(cgl_action_proc), (void*)CGLcnt);
    gtk_box_pack_start(GTK_BOX(hbox), cgl_cntbtn, true, true, 0);

    cgl_infbtn = gtk_toggle_button_new_with_label("Info");
    gtk_widget_set_name(cgl_infbtn, "Info");
    gtk_widget_show(cgl_infbtn);
    gtk_signal_connect(GTK_OBJECT(cgl_infbtn), "clicked",
        GTK_SIGNAL_FUNC(cgl_action_proc), (void*)CGLinf);
    gtk_box_pack_start(GTK_BOX(hbox), cgl_infbtn, true, true, 0);

    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cgl_action_proc), (void*)CGLhlp);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

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
    title[1] = "Type, Linked";
    title[2] = "Source  -  Click to select";
    GtkListStore *store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING);
    cgl_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_show(cgl_list);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(cgl_list), false);
    GtkCellRenderer *rnd = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *tvcol =
        gtk_tree_view_column_new_with_attributes(title[0], rnd,
        "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(cgl_list), tvcol);
    tvcol = gtk_tree_view_column_new_with_attributes(title[1], rnd,
        "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(cgl_list), tvcol);
    tvcol = gtk_tree_view_column_new_with_attributes(title[2], rnd,
        "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(cgl_list), tvcol);

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(cgl_list));
    gtk_tree_selection_set_select_function(sel,
        (GtkTreeSelectionFunc)cgl_selection_proc, 0, 0);
    // TreeView bug hack, see note with handlers.   
    gtk_signal_connect(GTK_OBJECT(cgl_list), "focus",
        GTK_SIGNAL_FUNC(cgl_focus_proc), this);

    gtk_container_add(GTK_CONTAINER(swin), cgl_list);
    gtk_widget_set_usize(swin, -1, 120);

    // Set up font and tracking.
    GTKfont::setupFont(cgl_list, FNT_PROP, true);

    gtk_table_attach(GTK_TABLE(form), swin, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cgl_cancel), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    update();
}


sCGL::~sCGL()
{
    CGL = 0;
    delete [] cgl_selection;
    delete [] cgl_contlib;

    if (cgl_caller)
        GRX->Deselect(cgl_caller);
    Cvt()->PopUpCgdOpen(0, MODE_OFF, 0, 0, 0, 0, 0, 0);
    PopUpInfo(MODE_OFF, 0);
    if (cgl_sav_pop)
        cgl_sav_pop->popdown();
    if (cgl_del_pop)
        cgl_del_pop->popdown();
    if (cgl_cnt_pop)
        cgl_cnt_pop->popdown();
    if (cgl_inf_pop)
        cgl_inf_pop->popdown();

    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(cgl_cancel), wb_shell);
}


// Update the listing.
//
void
sCGL::update()
{
    if (!CGL)
        return;

    if (cgl_selection && !CDcgd()->cgdRecall(cgl_selection, false)) {
        delete [] cgl_selection;
        cgl_selection = 0;
    }

    if (!cgl_selection) {
        gtk_widget_set_sensitive(cgl_savbtn, false);
        gtk_widget_set_sensitive(cgl_delbtn, false);
        gtk_widget_set_sensitive(cgl_cntbtn, false);
        gtk_widget_set_sensitive(cgl_infbtn, false);
        if (cgl_sav_pop)
            cgl_sav_pop->popdown();
        if (cgl_del_pop)
            cgl_del_pop->popdown();
    }

    stringlist *names = CDcgd()->cgdList();
    stringlist::sort(names);

    int rowcnt = 0;
    int rowsel = -1;
    GtkListStore *store =
        GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(cgl_list)));
    gtk_list_store_clear(store);
    GtkTreeIter iter;
    for (stringlist *l = names; l; l = l->next) {
        cCGD *cgd = CDcgd()->cgdRecall(l->string, false);
        if (cgd) {
            char *strings[3];
            strings[0] = l->string;
            strings[1] = cgd->info(true);
            strings[2] = lstring::copy(cgd->sourceName() ?
                lstring::strip_path(cgd->sourceName()) : "");
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, strings[0], 1, strings[1],
                2, strings[2], -1);
            delete [] strings[1];
            delete [] strings[2];
            if (cgl_selection && rowsel < 0 &&
                    !strcmp(cgl_selection, l->string))
                rowsel = rowcnt;
            rowcnt++;
        }
    }
    // This resizes columns and the widget.
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(cgl_list));
    if (rowsel >= 0) {
        GtkTreePath *p = gtk_tree_path_new_from_indices(rowsel, -1);
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(cgl_list));
        gtk_tree_selection_select_path(sel, p);
        gtk_tree_path_free(p);
    }

    stringlist::destroy(names);
}


// Handle the buttons.
//
void
sCGL::action_hdlr(GtkWidget *caller, void *client_data)
{
    if (client_data == (void*)CGLhlp) {
        DSPmainWbag(PopUpHelp("xic:geom"))
        return;
    }
    if (client_data == (void*)CGLcnt) {
        if (!cgl_selection)
            return;
        delete [] cgl_contlib;
        cgl_contlib = 0;

        cCGD *cgd = CDcgd()->cgdRecall(cgl_selection, false);
        if (cgd) {
            cgl_contlib = lstring::copy(cgl_selection);

            stringlist *s0 = cgd->cells_list();

            sLstr lstr;
            lstr.add("Cells found in geometry digest\n");
            lstr.add(cgl_selection);

            if (cgl_cnt_pop)
                cgl_cnt_pop->update(s0, lstr.string());
            else {
                const char *buttons[2];
                buttons[0] = INFO_BTN;
                buttons[1] = 0;

                int pagesz = 0;
                const char *s = CDvdb()->getVariable(VA_ListPageEntries);
                if (s) {
                    pagesz = atoi(s);
                    if (pagesz < 100 || pagesz > 50000)
                        pagesz = 0;
                }
                cgl_cnt_pop = DSPmainWbagRet(PopUpMultiCol(s0, lstr.string(),
                    cgl_cnt_cb, 0, buttons, pagesz, true));
                if (cgl_cnt_pop) {
                    cgl_cnt_pop->register_usrptr((void**)&cgl_cnt_pop);
                    cgl_cnt_pop->register_caller(cgl_cntbtn);
                }
            }
            stringlist::destroy(s0);
        }
        else
            err_message("Content scan failed:\n%s");
        return;
    }

    bool state = GRX->GetStatus(caller);

    if (client_data == (void*)CGLadd) {
        if (state) {
            int xo, yo;
            gdk_window_get_root_origin(Shell()->window, &xo, &yo);
            char *cn = CDcgd()->newCgdName();
            // Pop down first, panel used elsewhere.
            Cvt()->PopUpCgdOpen(0, MODE_OFF, 0, 0, 0, 0, 0, 0);
            Cvt()->PopUpCgdOpen(caller, MODE_ON, cn, 0, xo + 40, yo + 100,
                cgl_add_cb, 0);
            delete [] cn;
        }
        else
            Cvt()->PopUpCgdOpen(0, MODE_OFF, 0, 0, 0, 0, 0, 0);
        return;
    }
    if (client_data == (void*)CGLsav) {
        if (cgl_sav_pop)
            cgl_sav_pop->popdown();
        if (cgl_del_pop)
            cgl_del_pop->popdown();
        if (cgl_selection && state) {
            cgl_sav_pop = PopUpEditString((GRobject)cgl_savbtn,
                GRloc(), "Enter path to new digest file: ", 0, cgl_sav_cb,
                0, 250, 0, false, 0);
            if (cgl_sav_pop)
                cgl_sav_pop->register_usrptr((void**)&cgl_sav_pop);
        }
        else
            GRX->Deselect(cgl_savbtn);
        return;
    }
    if (client_data == (void*)CGLdel) {
        if (cgl_sav_pop)
            cgl_sav_pop->popdown();
        if (cgl_del_pop)
            cgl_del_pop->popdown();
        if (cgl_selection && state) {
            cCGD *cgd = CDcgd()->cgdRecall(CGL->cgl_selection, false);
            if (cgd && !cgd->refcnt()) {
                // should always be true
                cgl_del_pop = PopUpAffirm(cgl_delbtn, GRloc(),
                    "Confirm - delete selected digest?", cgl_del_cb,
                    lstring::copy(cgl_selection));
                if (cgl_del_pop)
                    cgl_del_pop->register_usrptr((void**)&cgl_del_pop);
            }
            else {
                GRX->Deselect(cgl_delbtn);
                GRX->SetSensitive(cgl_delbtn, false);
            }
        }
        else
            GRX->Deselect(cgl_delbtn);
        return;
    }
    if (client_data == (void*)CGLinf) {
        if (state) {
            cCGD *cgd = CDcgd()->cgdRecall(cgl_selection, false);
            if (cgd) {
                char *infostr = cgd->info(false);
                PopUpInfo(MODE_ON, infostr, STY_FIXED);
                delete [] infostr;
                if (wb_info)
                    wb_info->register_caller(cgl_infbtn);
            }
        }
        else
            PopUpInfo(MODE_OFF, 0);
        return;
    }
}


void
sCGL::err_message(const char *fmt)
{
    const char *s = Errs()->get_error();
    int len = strlen(fmt) + (s ? strlen(s) : 0) + 10;
    char *t = new char[len];
    sprintf(t, fmt, s);
    PopUpMessage(t, true);
    delete [] t;
}


// Static function.
// Selection callback for the list.  This is called when a new selection
// is made, but not when the selection disappears, which happens when the
// list is updated.
//
bool
sCGL::cgl_selection_proc(GtkTreeSelection*, GtkTreeModel *store,
    GtkTreePath *path, bool issel, void *)
{
    if (CGL) {
        if (issel)
            return (true);
        if (CGL->cgl_no_select) {
            CGL->cgl_no_select = false;
            return (false);
        }
        char *text = 0;
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(store, &iter, path))
            gtk_tree_model_get(store, &iter, 0, &text, -1);
        if (text) {
            delete [] CGL->cgl_selection;
            CGL->cgl_selection = lstring::copy(text);
            gtk_widget_set_sensitive(CGL->cgl_savbtn, true);
            gtk_widget_set_sensitive(CGL->cgl_cntbtn, true);
            gtk_widget_set_sensitive(CGL->cgl_infbtn, true);
            cCGD *cgd = CDcgd()->cgdRecall(CGL->cgl_selection, false);
            gtk_widget_set_sensitive(CGL->cgl_delbtn, cgd && !cgd->refcnt());
            if (cgd && CGL->wb_info) {
                char *infostr = cgd->info(false);
                CGL->wb_info->update(infostr);
                delete [] infostr;
            }
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
sCGL::cgl_focus_proc(GtkWidget*, GdkEvent*, void*)
{
    if (CGL) {
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(CGL->cgl_list));
        // If nothing selected set the flag.
        if (!gtk_tree_selection_get_selected(sel, 0, 0))
            CGL->cgl_no_select = true;
    }
    return (false);
}
        

// Static function.
// Callback for the Open dialog.
//
bool
sCGL::cgl_add_cb(const char *idname, const char *string, int mode, void*)
{
    if (!idname || !*idname)
        return (false);
    if (!string || !*string)
        return (false);
    CgdType tp = CGDremote;
    if (mode == 0)
        tp = CGDmemory;
    else if (mode == 1)
        tp = CGDfile;
    cCGD *cgd = FIO()->NewCGD(idname, string, tp);
    if (!cgd) {
        CGL->err_message("Failed to create new Geometry Digest:\n%s");
        return (false);
    }
    return (true);
}


// Static function.
// Callback for the Save dialog.
//
ESret
sCGL::cgl_sav_cb(const char *fname, void*)
{
    if (!CGL->cgl_selection) {
        CGL->err_message("No selection, select a CGD in the listing.");
        return (ESTR_IGN);
    }
    cCGD *cgd = CDcgd()->cgdRecall(CGL->cgl_selection, false);
    if (!cgd) {
        Errs()->add_error("unresolved CGD name %s.", CGL->cgl_selection);
        CGL->err_message("Error occurred:\n%s");
        return (ESTR_IGN);
    }

    dspPkgIf()->SetWorking(true);
    bool ok = cgd->write(fname);
    dspPkgIf()->SetWorking(false);
    if (!ok) {
        CGL->err_message("Error occurred when writing digest file:\n%s");
        return (ESTR_IGN);
    }
    CGL->PopUpMessage("Digest file saved successfully.", false);
    return (ESTR_DN);
}


// Static function.
// Callback for confirmation pop-up.
//
void
sCGL::cgl_del_cb(bool yn, void *arg)
{
    if (arg && yn && CGL) {
        char *dbname = (char*)arg;
        cCGD *cgd = CDcgd()->cgdRecall(dbname, true);
        if (cgd && !cgd->refcnt()) {
            // This will call update().
            delete cgd;
        }
        delete [] dbname;
    }
}


// Static function.
// When the user clicks on a cell name in the contents list, show a listing
// of the layers used.
//
void
sCGL::cgl_cnt_cb(const char *cellname, void*)
{
    if (!CGL)
        return;
    if (!CGL->cgl_contlib || !CGL->cgl_cnt_pop)
        return;
    if (!cellname)
        return;
    if (*cellname != '/')
        return;

    // Callback from button press.
    cellname++;
    if (!strcmp(cellname, INFO_BTN)) {
        cCGD *cgd = CDcgd()->cgdRecall(CGL->cgl_contlib, false);
        if (!cgd)
            return;
        char *listsel = CGL->cgl_cnt_pop->get_selection();
        stringlist *s0 = cgd->layer_info_list(listsel);
        char buf[256];
        sprintf(buf, "Layers found in %s", listsel);
        delete [] listsel;
        if (CGL->cgl_inf_pop)
            CGL->cgl_inf_pop->update(s0, buf);
        else {
            CGL->cgl_inf_pop = DSPmainWbagRet(PopUpMultiCol(s0, buf, 0, 0,
                0, 0, true));
            if (CGL->cgl_inf_pop)
                CGL->cgl_inf_pop->register_usrptr((void**)&CGL->cgl_inf_pop);
        }
        stringlist::destroy(s0);
    }
}


// Static function.
// Consolidated handler for the buttons.
//
void
sCGL::cgl_action_proc(GtkWidget *caller, void *client_data)
{
    if (CGL)
        CGL->action_hdlr(caller, client_data);
}


// Static function.
// Dismissal callback.
//
void
sCGL::cgl_cancel(GtkWidget*, void*)
{
    Cvt()->PopUpGeometries(0, MODE_OFF);
}

