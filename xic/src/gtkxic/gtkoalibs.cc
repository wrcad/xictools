
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: gtkoalibs.cc,v 5.41 2017/04/13 17:06:22 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "oa_if.h"
#include "pcell.h"
#include "pcell_params.h"
#include "editif.h"
#include "events.h"
#include "errorlog.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkmcol.h"
#include "gtkfont.h"
#include "gtkutil.h"
#include "gtkinlines.h"


//----------------------------------------------------------------------
//  OpenAccess Libraries Popup
//
// Help system keywords used:
//  xic:oalib

namespace {
    namespace gtkoalibs {
        struct sLBoa : public gtk_bag
        {
            sLBoa(GRobject);
            ~sLBoa();

            void get_selection(const char**, const char**);
            void update();

            enum { LBhelp, LBopen, LBwrt, LBcont, LBcrt, LBdefs,
                LBtech, LBdest, LBboth, LBphys, LBelec };

        private:
            void pop_up_contents();
            void update_contents(bool);
            void set_sensitive(bool);

            static void lb_cancel(GtkWidget*, void*);
            static bool lb_selection_proc(GtkTreeSelection*, GtkTreeModel*,
                GtkTreePath*, bool, void*);
            static int lb_focus_proc(GtkWidget*, GdkEvent*, void*);
            static int lb_button_press_proc(GtkWidget*, GdkEvent*, void*);
            static void lb_action_proc(GtkWidget*, void*);
            static void lb_lib_cb(const char*, void*);
            static void lb_dest_cb(bool, void*);
            static void lb_content_cb(const char*, void*);

            GRobject lb_caller;
            GtkWidget *lb_openbtn;
            GtkWidget *lb_writbtn;
            GtkWidget *lb_contbtn;
            GtkWidget *lb_techbtn;
            GtkWidget *lb_destbtn;
            GtkWidget *lb_both;
            GtkWidget *lb_phys;
            GtkWidget *lb_elec;
            GtkWidget *lb_list;
            GRmcolPopup *lb_content_pop;
            char *lb_selection;
            char *lb_contlib;
            GdkPixbuf *lb_open_pb;
            GdkPixbuf *lb_close_pb;
            char *lb_tempstr;
            bool lb_no_select;      // treeview focus hack

            static const char *nolibmsg;
        };

        sLBoa *LB;
    }
}

using namespace gtkoalibs;

const char *sLBoa::nolibmsg = "There are no open libraries.";

// Contents pop-up buttons.
#define OPEN_BTN "Open"
#define PLACE_BTN "Place"


// Pop up a listing of the libraries which are currently visible
// through the OpenAccess interface.
//
// The popup contains the following buttons:
//  Open/Close:   Toggle the open/closed status of selected library.
//  Contents:     Pop up a listing of the selected library contents.
//  Help:         Pop Up help.
//
void
cOAif::PopUpOAlibraries(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete LB;
        return;
    }
    if (mode == MODE_UPD) {
        if (LB)
            LB->update();
        return;
    }
    if (LB)
        return;

    new sLBoa(caller);
    if (!LB->Shell()) {
        delete LB;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(LB->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UL), LB->Shell(), mainBag()->Viewport());
    gtk_widget_show(LB->Shell());
}


void
cOAif::GetSelection(const char **plibname, const char **pcellname)
{
    if (plibname)
        *plibname = 0;
    if (pcellname)
        *pcellname = 0;
    if (LB)
        LB->get_selection(plibname, pcellname);
}


sLBoa::sLBoa(GRobject c)
{
    LB = this;
    lb_caller = c;
    lb_openbtn = 0;
    lb_writbtn = 0;
    lb_contbtn = 0;
    lb_techbtn = 0;
    lb_destbtn = 0;
    lb_both = 0;
    lb_phys = 0;
    lb_elec = 0;
    lb_list = 0;
    lb_content_pop = 0;
    lb_selection = 0;
    lb_contlib = 0;
    lb_open_pb = 0;
    lb_close_pb = 0;
    lb_tempstr = 0;
    lb_no_select = false;

    wb_shell = gtk_NewPopup(0, "OpenAccess Libraries", lb_cancel, 0);
    if (!wb_shell)
        return;
    gtk_window_set_default_size(GTK_WINDOW(wb_shell), 320, 200);

    GtkWidget *form = gtk_table_new(1, 5, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    lb_openbtn = gtk_button_new_with_label("Open/Close");
    gtk_widget_set_name(lb_openbtn, "Open");
    gtk_widget_show(lb_openbtn);
    gtk_signal_connect(GTK_OBJECT(lb_openbtn), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBopen);
    gtk_box_pack_start(GTK_BOX(hbox), lb_openbtn, true, true, 0);

    lb_writbtn = gtk_button_new_with_label("Writable Y/N");
    gtk_widget_set_name(lb_writbtn, "Writble");
    gtk_widget_show(lb_writbtn);
    gtk_signal_connect(GTK_OBJECT(lb_writbtn), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBwrt);
    gtk_box_pack_start(GTK_BOX(hbox), lb_writbtn, true, true, 0);

    lb_contbtn = gtk_button_new_with_label("Contents");
    gtk_widget_set_name(lb_contbtn, "Contents");
    gtk_widget_show(lb_contbtn);
    gtk_signal_connect(GTK_OBJECT(lb_contbtn), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBcont);
    gtk_box_pack_start(GTK_BOX(hbox), lb_contbtn, true, true, 0);

    GtkWidget *button = gtk_button_new_with_label("Create");
    gtk_widget_set_name(button, "Create");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBcrt);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_toggle_button_new_with_label("Defaults");
    gtk_widget_set_name(button, "Defaults");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBdefs);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBhelp);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    int rowcnt = 0;
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    lb_techbtn = gtk_toggle_button_new_with_label("Tech");
    gtk_widget_set_name(lb_techbtn, "Tech");
    gtk_widget_show(lb_techbtn);
    gtk_signal_connect(GTK_OBJECT(lb_techbtn), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBtech);
    gtk_box_pack_start(GTK_BOX(hbox), lb_techbtn, false, false, 0);

    lb_destbtn = gtk_button_new_with_label("Destroy");
    gtk_widget_set_name(lb_destbtn, "Destroy");
    gtk_widget_show(lb_destbtn);
    gtk_signal_connect(GTK_OBJECT(lb_destbtn), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBdest);
    gtk_box_pack_start(GTK_BOX(hbox), lb_destbtn, false, false, 0);

    sLstr lstr;
    lstr.add("Using OpenAccess ");
    lstr.add(OAif()->version());
    GtkWidget *label = gtk_label_new(lstr.string());
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // read mode radio group
    //
    hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    label = gtk_label_new("Data to use from OA: ");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, true, false, 0);

    lb_both = gtk_radio_button_new_with_label(0, "All");
    gtk_widget_set_name(lb_both, "All");
    gtk_widget_show(lb_both);
    GSList *group = gtk_radio_button_group(GTK_RADIO_BUTTON(lb_both));
    gtk_box_pack_start(GTK_BOX(hbox), lb_both, true, false, 0);
    gtk_signal_connect(GTK_OBJECT(lb_both), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBboth);

    lb_phys = gtk_radio_button_new_with_label(group, "Physical");
    gtk_widget_set_name(lb_phys, "Phys");
    gtk_widget_show(lb_phys);
    gtk_box_pack_start(GTK_BOX(hbox), lb_phys, true, false, 0);
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(lb_phys));
    gtk_signal_connect(GTK_OBJECT(lb_phys), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBphys);

    lb_elec = gtk_radio_button_new_with_label(group, "Electrical");
    gtk_widget_set_name(lb_elec, "Elec");
    gtk_widget_show(lb_elec);
    gtk_box_pack_start(GTK_BOX(hbox), lb_elec, true, false, 0);
    gtk_signal_connect(GTK_OBJECT(lb_elec), "clicked",
        GTK_SIGNAL_FUNC(lb_action_proc), (void*)LBelec);

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
    title[0] = "Open?";
    title[1] = "Write?";
    title[2] = "OpenAccess libraries, click to select";
    GtkListStore *store = gtk_list_store_new(3, GDK_TYPE_PIXBUF,
        G_TYPE_STRING, G_TYPE_STRING);
    lb_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(lb_list), false);
    gtk_widget_show(lb_list);
    GtkTreeViewColumn *tvcol = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(tvcol, title[0]);
    gtk_tree_view_append_column(GTK_TREE_VIEW(lb_list), tvcol);
    GtkCellRenderer *rnd = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(tvcol, rnd, true);
    gtk_tree_view_column_add_attribute(tvcol, rnd, "pixbuf", 0);

    tvcol = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(tvcol, title[1]);
    gtk_tree_view_append_column(GTK_TREE_VIEW(lb_list), tvcol);
    rnd = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(tvcol, rnd, true);
    gtk_tree_view_column_add_attribute(tvcol, rnd, "text", 1);

    tvcol = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(tvcol, title[2]);
    gtk_tree_view_append_column(GTK_TREE_VIEW(lb_list), tvcol);
    rnd = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(tvcol, rnd, true);
    gtk_tree_view_column_add_attribute(tvcol, rnd, "text", 2);

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(lb_list));
    gtk_tree_selection_set_select_function(sel,
        (GtkTreeSelectionFunc)lb_selection_proc, 0, 0);
    // TreeView bug hack, see note with handlers.   
    gtk_signal_connect(GTK_OBJECT(lb_list), "focus",
        GTK_SIGNAL_FUNC(lb_focus_proc), this);

    gtk_container_add(GTK_CONTAINER(swin), lb_list);
    gtk_widget_set_usize(lb_list, -1, 100);

    // Set up font and tracking.
    GTKfont::setupFont(lb_list, FNT_PROP, true);

    gtk_signal_connect(GTK_OBJECT(lb_list), "button-press-event",
        GTK_SIGNAL_FUNC(lb_button_press_proc), this);

    gtk_table_attach(GTK_TABLE(form), swin, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rowcnt++;

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // dismiss button line
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(lb_cancel), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    // Create pixmaps.
    lb_open_pb = gdk_pixbuf_new_from_xpm_data(wb_open_folder_xpm);
    lb_close_pb = gdk_pixbuf_new_from_xpm_data(wb_closed_folder_xpm);

    update();
}


sLBoa::~sLBoa()
{
    LB = 0;
    OAif()->PopUpOAdefs(0, MODE_OFF, 0, 0);
    OAif()->PopUpOAtech(0, MODE_OFF, 0, 0);
    delete [] lb_selection;
    delete [] lb_contlib;
    delete [] lb_tempstr;

    if (lb_caller)
        GRX->Deselect(lb_caller);
    if (lb_content_pop)
        lb_content_pop->popdown();

    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(lb_cancel), wb_shell);

    if (lb_open_pb)
        g_object_unref(lb_open_pb);
    if (lb_close_pb)
        g_object_unref(lb_close_pb);
}


// For export, return the current selections, if any.  If we pass null
// for the second arg, we alws get the currently selected library,
// otherwise it will be the library that contains the file selection,
// if any.
//
void
sLBoa::get_selection(const char **plibname, const char **pcellname)
{
    if (pcellname && lb_content_pop && lb_contlib) {
        char *sel = lb_content_pop->get_selection();
        if (sel) {
            if (plibname)
                *plibname = lb_contlib;
            delete [] lb_tempstr;
            lb_tempstr = sel;
            *pcellname = sel;
            return;
        }
    }
    else {
        if (plibname)
            *plibname = lb_selection;
    }
}


// Update the listing of open directories in the main pop-up.
//
void
sLBoa::update()
{
    {
        sLBoa *lbt = this;
        if (!lbt)
            return;
    }

    const char *s = CDvdb()->getVariable(VA_OaUseOnly);
    if (s && ((s[0] == '1' && s[1] == 0) || s[0] == 'p' || s[0] == 'P')) {
        if (!GRX->GetStatus(lb_phys)) {
            GRX->SetStatus(lb_phys, true);
            GRX->SetStatus(lb_both, false);
            GRX->SetStatus(lb_elec, false);
        }
    }
    else if (s && ((s[0] == '2' && s[1] == 0) || s[0] == 'e' || s[0] == 'E')) {
        if (!GRX->GetStatus(lb_elec)) {
            GRX->SetStatus(lb_elec, true);
            GRX->SetStatus(lb_both, false);
            GRX->SetStatus(lb_phys, false);
        }
    }
    else {
        if (!GRX->GetStatus(lb_both)) {
            GRX->SetStatus(lb_both, true);
            GRX->SetStatus(lb_phys, false);
            GRX->SetStatus(lb_elec, false);
        }
    }

    stringlist *liblist;
    if (!OAif()->list_libraries(&liblist)) {
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        return;
    }
    if (!liblist)
        liblist = new stringlist(lstring::copy(nolibmsg), 0);
    else
        liblist->sort();
    GtkListStore *store =
        GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(lb_list)));
    gtk_list_store_clear(store);
    GtkTreeIter iter;
    for (stringlist *l = liblist; l; l = l->next) {
        gtk_list_store_append(store, &iter);
        bool isopen;
        if (!OAif()->is_lib_open(l->string, &isopen)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            continue;
        }
        bool branded;
        if (!OAif()->is_lib_branded(l->string, &branded)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            continue;
        }
        gtk_list_store_set(store, &iter, 0, isopen ? lb_open_pb : lb_close_pb,
            1, branded ? "Y" :"N", 2, l->string, -1);
    }
    liblist->free();
    char *oldsel = lb_selection;
    lb_selection = 0;
    set_sensitive(false);

    if (lb_content_pop) {
        if (lb_contlib) {
            bool islib;
            if (!OAif()->is_library(lb_contlib, &islib))
                Log()->ErrorLog(mh::Processing, Errs()->get_error());
            if (!islib)
                lb_content_pop->update(0, "No library selected");
        }
        if (DSP()->MainWdesc()->DbType() == WDchd)
            lb_content_pop->set_button_sens(0);
        else
            lb_content_pop->set_button_sens(-1);
    }
    if (oldsel) {
        // This re-selects the previously selected library.
        for (int i = 0; ; i++) {
            GtkTreePath *p = gtk_tree_path_new_from_indices(i, -1);
            if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, p)) {
                gtk_tree_path_free(p);
                break;
            }
            char *text;
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 2, &text, -1);
            int sc = strcmp(text, oldsel);
            free(text);
            if (!sc) {
                GtkTreeSelection *sel =
                    gtk_tree_view_get_selection(GTK_TREE_VIEW(lb_list));
                gtk_tree_selection_select_path(sel, p);
                gtk_tree_path_free(p);
                break;
            }
            gtk_tree_path_free(p);
        }
    }
    delete [] oldsel;

}


// Pop up a listing of the contents of the selected library.
//
void
sLBoa::pop_up_contents()
{
    {
        sLBoa *lbt = this;
        if (!lbt)
            return;
    }
    if (!lb_selection)
        return;

    delete [] lb_contlib;
    lb_contlib = lstring::copy(lb_selection);
    stringlist *list;
    if (!OAif()->list_lib_cells(lb_contlib, &list)) {
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        return;
    }
    sLstr lstr;
    lstr.add("Cells found in library - click to select\n");
    lstr.add(lb_contlib);

    if (lb_content_pop)
        lb_content_pop->update(list, lstr.string());
    else {
        const char *buttons[3];
        buttons[0] = OPEN_BTN;
        buttons[1] = EditIf()->hasEdit() ? PLACE_BTN : 0;
        buttons[2] = 0;

        int pagesz = 0;
        const char *s = CDvdb()->getVariable(VA_ListPageEntries);
        if (s) {
            pagesz = atoi(s);
            if (pagesz < 100 || pagesz > 50000)
                pagesz = 0;
        }
        lb_content_pop = DSPmainWbagRet(PopUpMultiCol(list, lstr.string(),
            lb_content_cb, 0, buttons, pagesz));
        if (lb_content_pop) {
            lb_content_pop->register_usrptr((void**)&lb_content_pop);
            if (DSP()->MainWdesc()->DbType() == WDchd)
                lb_content_pop->set_button_sens(0);
            else
                lb_content_pop->set_button_sens(-1);
        }
    }
    list->free();
}


void
sLBoa::update_contents(bool upd_dir)
{
    if (!lb_content_pop)
        return;
    if (!upd_dir) {
        if (!lb_contlib)
            return;
    }
    else {
        if (!lb_selection)
            return;
        delete [] lb_contlib;
        lb_contlib = lstring::copy(lb_selection);
    }

    stringlist *list;
    if (!OAif()->list_lib_cells(lb_contlib, &list)) {
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        return;
    }
    sLstr lstr;
    lstr.add("Cells found in library - click to select\n");
    lstr.add(lb_contlib);
    lb_content_pop->update(list, lstr.string());
    list->free();
}


void
sLBoa::set_sensitive(bool sens)
{
    gtk_widget_set_sensitive(lb_openbtn, sens);
    gtk_widget_set_sensitive(lb_writbtn, sens);
    gtk_widget_set_sensitive(lb_contbtn, sens);
    if (!sens) {
        gtk_widget_set_sensitive(lb_techbtn, false);
        gtk_widget_set_sensitive(lb_destbtn, false);
    }
    else {
        bool branded;
        if (!OAif()->is_lib_branded(LB->lb_selection, &branded)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            gtk_widget_set_sensitive(lb_techbtn, false);
            gtk_widget_set_sensitive(lb_destbtn, false);
        }
        else {
            gtk_widget_set_sensitive(lb_techbtn, branded);
            gtk_widget_set_sensitive(lb_destbtn, branded);
        }
    }
}


// Static function.
// Dismissal callback
//
void
sLBoa::lb_cancel(GtkWidget*, void*)
{
    OAif()->PopUpOAlibraries(0, MODE_OFF);
}


// Static function.
// Selection callback for the list.  This is called when a new selection
// is made, but not when the selection disappears, which happens when the
// list is updated.
//
bool
sLBoa::lb_selection_proc(GtkTreeSelection*, GtkTreeModel *store,
    GtkTreePath *path, bool issel, void*)
{
    if (LB) {
        if (LB->lb_no_select && !issel)
            return (false);
        char *text = 0;
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(store, &iter, path))
            gtk_tree_model_get(store, &iter, 2, &text, -1);
        if (!text || !strcmp(nolibmsg, text)) {
            LB->set_sensitive(false);
            free(text);
            return (false);
        }

        // Behavior is a bit strange.  When clicking on a new selection,
        // we get called three times:
        // 0  old  (initial click, integer is issel value)
        // 0  new  (the second click an a new library gives these three)
        // 1  old
        // 0  new
        // printf("%d %s\n", issel, text);

        if (issel) {
            LB->set_sensitive(false);
            free(text);
            return (true);
        }
        if (!LB->lb_selection || strcmp(text, LB->lb_selection)) {
            delete [] LB->lb_selection;
            LB->lb_selection = lstring::copy(text);
            OAif()->PopUpOAtech(0, MODE_UPD, 0, 0);
            LB->update_contents(true);
        }
        LB->set_sensitive(true);
        free(text);
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
int
sLBoa::lb_focus_proc(GtkWidget*, GdkEvent*, void*)
{
    if (LB) {
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(LB->lb_list));
        // If nothing selected set the flag.
        if (!gtk_tree_selection_get_selected(sel, 0, 0))
            LB->lb_no_select = true;
    }
    return (false);
}


// Toggle closed/open pr writable Y/N by clicking on the icon in the
// selected row.
//
int
sLBoa::lb_button_press_proc(GtkWidget*, GdkEvent *event, void*)
{
    if (LB) {
        GtkTreePath *p;
        GtkTreeView *tv = GTK_TREE_VIEW(LB->lb_list);
        GtkTreeViewColumn *col;
        if (!gtk_tree_view_get_path_at_pos(tv,
                (int)event->button.x, (int)event->button.y, &p, &col, 0, 0))
            return (false);
        if (!LB->lb_selection)
            return (false);
        GtkTreeModel *mod = gtk_tree_view_get_model(tv);
        GtkTreeIter iter;
        gtk_tree_model_get_iter(mod, &iter, p);
        gtk_tree_path_free(p);

        GtkTreeSelection *sel = gtk_tree_view_get_selection(tv);
        if (!gtk_tree_selection_iter_is_selected(sel, &iter))
            return (false);

        if (col == gtk_tree_view_get_column(tv, 0)) {
            bool isopen;
            if (!OAif()->is_lib_open(LB->lb_selection, &isopen)) {
                Log()->ErrorLog(mh::Processing, Errs()->get_error());
                OAif()->set_lib_open(LB->lb_selection, false);
            }
            else if (isopen)
                OAif()->set_lib_open(LB->lb_selection, false);
            else
                OAif()->set_lib_open(LB->lb_selection, true);
            LB->update();
        }
        else if (col == gtk_tree_view_get_column(tv, 1)) {
            bool branded;
            if (!OAif()->is_lib_branded(LB->lb_selection, &branded)) {
                Log()->ErrorLog(mh::Processing, Errs()->get_error());
                OAif()->brand_lib(LB->lb_selection, false);
            }
            else if (!OAif()->brand_lib(LB->lb_selection, !branded))
                Log()->ErrorLog(mh::Processing, Errs()->get_error());
            LB->update();
        }
    }
    return (false);
}


// Static function.
// Consolidated handler for the buttons.
//
void
sLBoa::lb_action_proc(GtkWidget *caller, void *client_data)
{
    if (!LB)
        return;
    if (client_data == (void*)LBhelp) {
        DSPmainWbag(PopUpHelp("xic:oalib"))
    }
    else if (client_data == (void*)LBopen) {
        if (LB->lb_selection) {
            bool isopen;
            if (!OAif()->is_lib_open(LB->lb_selection, &isopen)) {
                Log()->ErrorLog(mh::Processing, Errs()->get_error());
                OAif()->set_lib_open(LB->lb_selection, false);
            }
            else if (isopen)
                OAif()->set_lib_open(LB->lb_selection, false);
            else
                OAif()->set_lib_open(LB->lb_selection, true);
            LB->update();
        }
    }
    else if (client_data == (void*)LBwrt) {
        if (LB->lb_selection) {
            bool branded;
            if (!OAif()->is_lib_branded(LB->lb_selection, &branded)) {
                Log()->ErrorLog(mh::Processing, Errs()->get_error());
                OAif()->brand_lib(LB->lb_selection, false);
            }
            else if (!OAif()->brand_lib(LB->lb_selection, !branded))
                Log()->ErrorLog(mh::Processing, Errs()->get_error());
            LB->update();
        }
    }
    else if (client_data == (void*)LBcont) {
        LB->pop_up_contents();
    }
    else if (client_data == (void*)LBcrt) {
        LB->PopUpInput("New library name? ", "", "Create", LB->lb_lib_cb, 0);
    }
    else if (client_data == (void*)LBdefs) {
        if (GRX->GetStatus(caller)) {
            int x, y;
            GRX->Location(caller, &x, &y);
            OAif()->PopUpOAdefs(caller, MODE_ON, x - 100, y + 50);
        }
        else
            OAif()->PopUpOAdefs(0, MODE_OFF, 0, 0);
    }
    else if (client_data == (void*)LBtech) {
        if (GRX->GetStatus(caller)) {
            int x, y;
            GRX->Location(caller, &x, &y);
            OAif()->PopUpOAtech(caller, MODE_ON, x + 100, y + 50);
        }
        else
            OAif()->PopUpOAtech(0, MODE_OFF, 0, 0);
    }
    else if (client_data == (void*)LBdest) {
        if (!LB->lb_selection)
            return;
        sLstr lstr;
        char *sel = 0;
        if (LB->lb_content_pop) {
            sel = LB->lb_content_pop->get_selection();
            if (sel && LB->lb_contlib) {
                lstr.add(LB->lb_contlib);
                lstr.add_c('\n');
                lstr.add(sel);
            }
            else
                return;
        }
        else
            lstr.add(LB->lb_selection);

        sLstr tstr;
        tstr.add("Do you really want to irretrievalby\n");
        if (sel) {
            tstr.add("destroy cell ");
            tstr.add(sel);
            tstr.add(" in library ");
            tstr.add(LB->lb_contlib);
            tstr.add_c('?');
        }
        else {
            tstr.add("destroy library ");
            tstr.add(LB->lb_selection);
            tstr.add(" and its contents?");
        }

        LB->PopUpAffirm(0, LW_CENTER, tstr.string(),
            LB->lb_dest_cb, lstr.string_trim());
    }
    else if (client_data == (void*)LBboth) {
        if (GRX->GetStatus(caller))
            CDvdb()->clearVariable(VA_OaUseOnly);
    }
    else if (client_data == (void*)LBphys) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_OaUseOnly, "Physical");
    }
    else if (client_data == (void*)LBelec) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_OaUseOnly, "Electrical");
    }
}


void
sLBoa::lb_lib_cb(const char *lname, void*)
{
    if (!LB)
        return;
    char *nametok = lstring::clip_space(lname);
    if (!nametok)
        return;
    bool ret = OAif()->create_lib(nametok, 0);
    delete [] nametok;
    if (!ret) {
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        return;
    }
    if (LB->wb_input)
        LB->wb_input->popdown();
    LB->update();
}


void
sLBoa::lb_dest_cb(bool yes, void *arg)
{
    char *str = (char*)arg;
    if (LB && yes) {
        char *t = strchr(str, '\n');
        if (t)
            *t++ = 0;
        if (!OAif()->destroy(str, t, 0))
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
        if (t)
            LB->update_contents(false);
        else
            LB->update();
    }
    delete [] str;
}


// Static function.
// Callback for a content window.
//
void
sLBoa::lb_content_cb(const char *cellname, void*)
{
    if (!LB)
        return;
    if (!LB->lb_contlib || !LB->lb_content_pop)
        return;
    if (!cellname)
        return;
    if (*cellname != '/')
        return;

    // Callback from button press.
    cellname++;
    if (!strcmp(cellname, OPEN_BTN)) {
        char *sel = LB->lb_content_pop->get_selection();
        if (!sel)
            return;
        PCellParam *p0 = 0;
        EV()->InitCallback();

        // Clear persistent OA cells-loaded table.
        OAif()->clear_name_table();

        if (!OAif()->load_cell(LB->lb_contlib, sel, 0, CDMAXCALLDEPTH, true,
                &p0, 0)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            delete [] sel;
            return;
        }
        if (p0) {
            char *dbname = PC()->addSuperMaster(LB->lb_contlib, sel,
                DSP()->CurMode() == Physical ? "layout" : "schematic", p0);
            p0->free();
            if (EditIf()->hasEdit()) {
                if (!EditIf()->openPlacement(0, dbname)) {
                    Log()->ErrorLogV(mh::PCells,
                        "Failed to open sub-master:\n%s",
                        Errs()->get_error());
                }
            }
            else {
                Log()->ErrorLogV(mh::PCells,
                    "Support for PCell sub-master creation is not "
                    "available in\nthis feature set.");
            }
        }
        delete [] sel;
    }
    else if (!strcmp(cellname, PLACE_BTN)) {
        char *sel = LB->lb_content_pop->get_selection();
        if (!sel)
            return;
        EV()->InitCallback();
        EditIf()->addMaster(LB->lb_contlib, sel);
        delete [] sel;
    }
}

