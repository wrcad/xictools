
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
#include "sced.h"
#include "sced_nodemap.h"
#include "extif.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "events.h"
#include "menu.h"
#include "ebtn_menu.h"
#include "ext_menu.h"
#include "select.h"
#include "promptline.h"
#include "errorlog.h"
#include "gtkmain.h"
#include "gtkinterf/gtkutil.h"

#include "bitmaps/lsearch.xpm"
#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#include "regex/regex.h"
#endif


namespace {
    // Some utilities for GtkTreeView.

    // Return the text item at row,col, needs to be freed.
    //
    char *list_get_text(GtkWidget *list, int row, int col)
    {
        GtkTreePath *p = gtk_tree_path_new_from_indices(row, -1);
        GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
        GtkTreeIter iter;
        if (!gtk_tree_model_get_iter(store, &iter, p))
            return (0);
        char *text;
        gtk_tree_model_get(store, &iter, col, &text, -1);
        gtk_tree_path_free(p);
        return (lstring::tocpp(text));
    }

    void list_move_to(GtkWidget *list, int row)
    {
        // Run events, this is important:  If the cells have been
        // recently updated, the get_background_area call will return
        // a bogus value!  (GTK bug)
        //
        gtk_DoEvents(1000);

        // Don't scroll unless we have to, the scroll_to_cell function
        // doesn't do the right thing, so we attempt to handle this
        // ourselves.  We can do this since to cell height is fixed.

        GtkAdjustment *adj = gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(list));
        if (!adj)
            return;

        GtkTreePath *p = gtk_tree_path_new_from_indices(row, -1);
        GdkRectangle r;
        gtk_tree_view_get_background_area(GTK_TREE_VIEW(list), p, 0, &r);

        int last_row = (int)(gtk_adjustment_get_value(adj)/r.height);
        int vis_rows = (int)(gtk_adjustment_get_page_size(adj)/r.height);
        if (row < last_row || row > last_row + vis_rows) {
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(list), p, 0,
                false, 0.0, 0.0);
        }
        gtk_tree_path_free(p);
    }

    void list_select_row(GtkWidget *list, int row)
    {
        GtkTreePath *p = gtk_tree_path_new_from_indices(row, -1);
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
        gtk_tree_selection_select_path(sel, p);
        gtk_tree_path_free(p);
    }
}


//-------------------------------------------------------------------------
// Node (Net) Name Mapping
//
// Help system keywords used:
//  xic:nodmp

namespace {
    namespace gtknodmp {
        struct NmpState : public CmdState
        {
            NmpState(const char *nm, const char *hk) : CmdState(nm, hk) { }
            virtual ~NmpState();

            void b1down() { cEventHdlr::sel_b1down(); }
            void b1up();
            void b1up_altw();
            void esc();
        };

        struct sNM : public GTKbag
        {
            sNM(GRobject, int);
            ~sNM();

            bool update(int);
            void show_node_terms(int);
            void update_map();

            void clear_cmd() { nm_cmd = 0; }
            void desel_point_btn() { GTKdev::Deselect(nm_point_btn); }

        private:
            void enable_point(bool);

            static int nm_node_of_row(int);
            static int nm_select_nlist_proc(GtkTreeSelection*, GtkTreeModel*,
                GtkTreePath*, int, void*);
            static bool nm_n_focus_proc(GtkWidget*, GdkEvent*, void*);
            static int nm_select_tlist_proc(GtkTreeSelection*, GtkTreeModel*,
                GtkTreePath*, int, void*);
            static bool nm_t_focus_proc(GtkWidget*, GdkEvent*, void*);
            static void nm_cancel_proc(GtkWidget*, void*);
            static void nm_desel_proc(GtkWidget*, void*);
            static void nm_use_np_proc(GtkWidget*, void*);
            static void nm_name_cb(const char*, void*);
            static void nm_join_cb(bool, void*);
            static void nm_set_name(const char*);
            static void nm_rename_proc(GtkWidget*, void*);
            static void nm_rm_cb(bool, void*);
            static void nm_remove_proc(GtkWidget*, void*);
            static void nm_point_proc(GtkWidget*, void*);
            static void nm_usex_proc(GtkWidget*, void*);
            static void nm_find_proc(GtkWidget*, void*);
            static void nm_help_proc(GtkWidget*, void*);
            static void nm_search_hdlr(GtkWidget*, void*);
            static void nm_activate_proc(GtkWidget*, void*);

            void do_search(int*, int*);
            int find_row(const char*);

            struct NmpState *nm_cmd;
            GRobject nm_caller;
            GtkWidget *nm_use_np;;
            GtkWidget *nm_rename;
            GtkWidget *nm_remove;
            GtkWidget *nm_point_btn;
            GtkWidget *nm_srch_btn;
            GtkWidget *nm_srch_entry;
            GtkWidget *nm_srch_nodes;
            GtkWidget *nm_node_list;
            GtkWidget *nm_term_list;
            GtkWidget *nm_paned;
            GtkWidget *nm_usex_btn;
            GtkWidget *nm_find_btn;
            int nm_showing_node;
            int nm_showing_row;
            int nm_showing_term_row;
            bool nm_noupdating;
            bool nm_n_no_select;        // treeview focus hack
            bool nm_t_no_select;        // treeview focus hack

            CDp_node *nm_node;
            CDc *nm_cdesc;

            GRaffirmPopup *nm_rm_affirm;
            GRaffirmPopup *nm_join_affirm;

            static bool nm_use_extract;

            static short int nm_win_width;
            static short int nm_win_height;
            static short int nm_grip_pos;
        };

        sNM *NM;

        bool sNM::nm_use_extract;
        short int sNM::nm_win_width;
        short int sNM::nm_win_height;
        short int sNM::nm_grip_pos;
    }
}

using namespace gtknodmp;


// The return is true when MODE_UPD, and an update was done.  This is
// used by the extraction system.  True is also returned on successful
// MODE_ON.
//
bool
cSced::PopUpNodeMap(GRobject caller, ShowMode mode, int node)
{
    if (!GTKdev::exists() || !GTKmainwin::exists())
        return (false);
    if (mode == MODE_OFF) {
        delete NM;
        return (false);
    }
    if (mode == MODE_UPD) {
        if (NM)
            return (NM->update(node));
        return (false);
    }
    if (NM)
        return (true);
    if (!CurCell(Electrical, true))
        return (false);

    new sNM(caller, node);
    if (!NM->Shell()) {
        delete NM;
        return (false);
    }
    gtk_window_set_transient_for(GTK_WINDOW(NM->Shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GTKdev::self()->SetPopupLocation(GRloc(LW_LL), NM->Shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(NM->Shell());
    return (true);
}
// End of cSced functions.


sNM::sNM(GRobject caller, int node)
{
    NM = this;
    nm_caller = caller;
    nm_cmd = 0;
    nm_use_np = 0;
    nm_rename = 0;
    nm_remove = 0;
    nm_point_btn = 0;
    nm_srch_btn = 0;
    nm_srch_entry = 0;
    nm_srch_nodes = 0;
    nm_node_list = 0;
    nm_term_list = 0;
    nm_paned = 0;
    nm_usex_btn = 0;
    nm_find_btn = 0;
    nm_showing_node = -1;
    nm_showing_row = -1;
    nm_showing_term_row = -1;
    nm_noupdating = false;
    nm_node = 0;
    nm_cdesc = 0;
    nm_rm_affirm = 0;
    nm_join_affirm = 0;
    nm_n_no_select = false;
    nm_t_no_select = false;

    wb_shell = gtk_NewPopup(0, "Node (Net) Name Mapping", nm_cancel_proc, 0);
    if (!wb_shell)
        return;

    GtkWidget *form = gtk_table_new(1, 4, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    //
    // button line
    //
    GtkWidget *button = gtk_toggle_button_new_with_label("Use nophys");
    gtk_widget_set_name(button, "UseNoPhys");
    gtk_widget_show(button);
    nm_use_np = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(nm_use_np_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_toggle_button_new_with_label("Map Name");
    gtk_widget_set_name(button, "Rename");
    gtk_widget_show(button);
    nm_rename = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(nm_rename_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_toggle_button_new_with_label("Unmap");
    gtk_widget_set_name(button, "Remove");
    gtk_widget_show(button);
    nm_remove = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(nm_remove_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_toggle_button_new_with_label("Click-Select Mode");
    gtk_widget_set_name(button, "Click");
    gtk_widget_show(button);
    nm_point_btn = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(nm_point_proc), wb_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(nm_help_proc), wb_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    int rowcnt = 0;
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 0, 0);
    rowcnt++;

    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    //
    // second button line
    //
    GtkWidget *label = gtk_label_new("Search");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);

    button = gtk_NewPixmapButton(lsearch_xpm, 0, false);
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(nm_search_hdlr), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
    nm_srch_btn = button;

    GtkWidget *entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 0);
    gtk_widget_set_size_request(entry, 80, -1);
    g_signal_connect(G_OBJECT(entry), "activate",
        G_CALLBACK(nm_activate_proc), 0);
    nm_srch_entry = entry;

    button = gtk_radio_button_new_with_label(0, "Nodes");
    gtk_widget_set_name(button, "Nodes");
    gtk_widget_show(button);
    GSList *group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
    nm_srch_nodes = button;

    button = gtk_radio_button_new_with_label(group, "Terminals");
    gtk_widget_set_name(button, "Terminals");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 0, 0);
    rowcnt++;

    GtkWidget *paned = gtk_hpaned_new();
    gtk_widget_show(paned);
    nm_paned = paned;

    //
    // node listing text
    //
    GtkWidget *swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(swin);
    gtk_widget_set_size_request(swin, 250, 200);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_set_border_width(GTK_CONTAINER(swin), 2);

    // three columns
    const char *ntitles[3];
    ntitles[0] = "Internal";
    ntitles[1] = "Mapped";
    ntitles[2] = "Set";
    GtkListStore *store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING);
    nm_node_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_show(nm_node_list);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(nm_node_list), false);
    GtkCellRenderer *rnd = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *tvcol =
        gtk_tree_view_column_new_with_attributes(ntitles[0], rnd,
        "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(nm_node_list), tvcol);
    tvcol = gtk_tree_view_column_new_with_attributes(ntitles[1], rnd,
        "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(nm_node_list), tvcol);
    tvcol = gtk_tree_view_column_new_with_attributes(ntitles[2], rnd,
        "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(nm_node_list), tvcol);

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(nm_node_list));
    gtk_tree_selection_set_select_function(sel, nm_select_nlist_proc, 0, 0);
    // TreeView bug hack, see note with handler.   
    g_signal_connect(G_OBJECT(nm_node_list), "focus",
        G_CALLBACK(nm_n_focus_proc), this);

    gtk_container_add(GTK_CONTAINER(swin), nm_node_list);
    gtk_paned_pack1(GTK_PANED(paned), swin, true, true);

    //
    // terminal listing text
    //
    swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(swin);
    gtk_widget_set_size_request(swin, 110, 200);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_set_border_width(GTK_CONTAINER(swin), 2);

    const char *ttitles[1];
    ttitles[0] = "Terminals";
    store = gtk_list_store_new(1, G_TYPE_STRING);
    nm_term_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(nm_term_list), false);
    gtk_widget_show(nm_term_list);
    rnd = gtk_cell_renderer_text_new();
    tvcol =
        gtk_tree_view_column_new_with_attributes(ttitles[0], rnd,
        "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(nm_term_list), tvcol);

    sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(nm_term_list));
    gtk_tree_selection_set_select_function(sel, nm_select_tlist_proc, 0, 0);
    // TreeView bug hack, see note with handler.   
    g_signal_connect(G_OBJECT(nm_term_list), "focus",
        G_CALLBACK(nm_t_focus_proc), this);

    gtk_container_add(GTK_CONTAINER(swin), nm_term_list);
    gtk_paned_pack2(GTK_PANED(paned), swin, true, true);

    gtk_table_attach(GTK_TABLE(form), paned, 0, 1, rowcnt, rowcnt+1,
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
    // cancel button row
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    button = gtk_button_new_with_label(" Deselect ");
    gtk_widget_set_name(button, "Deselect");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(nm_desel_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    button = gtk_button_new_with_label("Dismiss");
    GtkWidget *dismiss_btn = button;
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(nm_cancel_proc), wb_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_check_button_new_with_label("Use Extract");
    gtk_widget_set_name(button, "UsEx");
    if (ExtIf()->hasExtract())
        gtk_widget_show(button);
    else
        gtk_widget_hide(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(nm_usex_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
    nm_usex_btn = button;

    button = gtk_button_new_with_label("Find");
    gtk_widget_set_name(button, "Find");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(nm_find_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
    nm_find_btn = button;

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), dismiss_btn);

    // If the group/node selection mode in extraction is enabled with
    // a selection, configure the pop-up to have that node selected.
    //
    if (node < 0 && DSP()->ShowingNode() >= 0 && ExtIf()->selectShowNode(-1))
        node = DSP()->ShowingNode();

    if (nm_win_width > 0 && nm_win_height > 0) {
        gtk_window_set_default_size(GTK_WINDOW(wb_shell),
            nm_win_width, nm_win_height);
        gtk_paned_set_position(GTK_PANED(nm_paned), nm_grip_pos);
    }
    if (DSP()->CurMode() == Physical)
        nm_use_extract = true;
    update(node);
    ExtIf()->PopUpExtSetup(0, MODE_UPD);
}


sNM::~sNM()
{
    NM = 0;
    if (nm_cmd)
        nm_cmd->esc();
    bool state = GTKdev::GetStatus(nm_rename);
    if (state)
        EV()->InitCallback();

    // Keep node selected if extraction group/node selection is active.
    if (!ExtIf()->selectShowNode(-1))
        DSP()->ShowNode(ERASE, nm_showing_node);

    if (nm_rm_affirm)
        nm_rm_affirm->popdown();
    if (nm_join_affirm)
        nm_join_affirm->popdown();
    if (nm_caller)
        GTKdev::SetStatus(nm_caller, false);
    if (wb_shell) {
        int wid = gdk_window_get_width(gtk_widget_get_window(wb_shell));
        int hei = gdk_window_get_height(gtk_widget_get_window(wb_shell));
        nm_win_width = wid;
        nm_win_height = hei;
        nm_grip_pos = gtk_paned_get_position(GTK_PANED(nm_paned));
        g_signal_handlers_disconnect_by_func(G_OBJECT(wb_shell),
            (gpointer)nm_cancel_proc, wb_shell);
    }
    if (nm_node) {
        DSP()->HliteElecTerm(ERASE, nm_node, nm_cdesc, 0);
        CDterm *t;
        if (nm_cdesc)
            t = ((CDp_cnode*)nm_node)->inst_terminal();
        else
            t = ((CDp_snode*)nm_node)->cell_terminal();
        DSP()->HlitePhysTerm(ERASE, t);
    }
    ExtIf()->PopUpExtSetup(0, MODE_UPD);
}


bool
sNM::update(int node)
{
    if (nm_noupdating)
        return (true);

    bool iselec = (DSP()->CurMode() == Electrical);
    gtk_widget_set_sensitive(nm_use_np, iselec);
    gtk_widget_set_sensitive(nm_rename, false);
    gtk_widget_set_sensitive(nm_remove, false);
    if (nm_rm_affirm)
        nm_rm_affirm->popdown();
    if (nm_join_affirm)
        nm_join_affirm->popdown();
    if (wb_input)
        wb_input->popdown();

    CDs *cursde = CurCell(Electrical);  // allow symbolic for check below
    if (!cursde) {
        SCD()->PopUpNodeMap(0, MODE_OFF);
        return (false);
    }

    if (node >= 0) {
        // This update is coming from group/node selection in the
        // extraction system.  Make sure that net we're not in
        // use-nophys mode.
        SCD()->setIncludeNoPhys(false);
        SCD()->connect(cursde);
    }

    DSP()->ShowNode(ERASE, nm_showing_node);
    enable_point(!cursde->isSymbolic());
    nm_showing_row = -1;

    if (node >= 0) {
        for (int row = 0; ; row++) {
            int n = nm_node_of_row(row);
            if (n < 0)
                break;
            if (n == node) {
                nm_showing_node = node;
                break;
            }
        }
    }
    update_map();

    GTKdev::SetStatus(nm_usex_btn, nm_use_extract);
    GTKdev::SetStatus(nm_use_np, SCD()->includeNoPhys());
    if (nm_use_extract) {
        GTKdev::SetStatus(nm_point_btn,
            MainMenu()->MenuButtonStatus(MMext, MenuEXSEL));
    }
    else
        GTKdev::SetStatus(nm_point_btn, (nm_cmd != 0));
    return (true);
}


void
sNM::show_node_terms(int node)
{
    DSP()->ShowNode(ERASE, nm_showing_node);
    nm_showing_node = -1;

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(NM->nm_term_list));
    gtk_tree_selection_unselect_all(sel);
    GtkListStore *store =
        GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(nm_term_list)));
    gtk_list_store_clear(store);
    gtk_widget_set_sensitive(nm_find_btn, false);
    if (nm_node) {
        DSP()->HliteElecTerm(ERASE, nm_node, nm_cdesc, 0);
        CDterm *t;
        if (nm_cdesc)
            t = ((CDp_cnode*)nm_node)->inst_terminal();
        else
            t = ((CDp_snode*)nm_node)->cell_terminal();
        DSP()->HlitePhysTerm(ERASE, t);
        nm_node = 0;
    }
    nm_cdesc = 0;

    GtkTreeIter iter;
    const char *strings[1];
    const char *msg = "no terminals found";
    CDs *cursde = CurCell(Electrical, true);
    if (!cursde)
        return;
    if (node < 0) {
        strings[0] = "bad node";
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, strings[0], -1);
    }
    else if (node == 0) {
        strings[0] = "ground node";
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, strings[0], -1);
        nm_showing_node = node;
    }
    else {
        bool didone = false;
        stringlist *sl = SCD()->getElecNodeContactNames(cursde, node);
        for (stringlist *s = sl; s; s = s->next) {
            strings[0] = s->string;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, strings[0], -1);
            didone = true;
        }
        stringlist::destroy(sl);
        if (!didone) {
            strings[0] = msg;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, strings[0], -1);
        }
        nm_showing_node = node;
    }
    DSP()->ShowNode(DISPLAY, nm_showing_node);
}


void
sNM::update_map()
{
    CDcbin cbin(DSP()->CurCellName());
    if (!cbin.elec())
        return;

    // Grab a list of the node properties connected to the node being
    // shown, if any, before the connectivity update.
    CDpl *pl = SCD()->getElecNodeProps(cbin.elec(), nm_showing_node);

    cNodeMap *map = cbin.elec()->nodes();
    if (!map) {
        map = new cNodeMap(cbin.elec());
        cbin.elec()->setNodes(map);
    }
    // this updates the node map
    SCD()->connect(cbin.elec());

    int last_row = 0;
    int vis_rows = 10;
    {
        GtkTreePath *p = gtk_tree_path_new_from_indices(0, -1);
        GdkRectangle r;
        gtk_tree_view_get_background_area(GTK_TREE_VIEW(nm_node_list),
            p, 0, &r);
        gtk_tree_path_free(p);
        if (r.height > 0) {
            GtkAdjustment *adj =
                gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(nm_node_list));
            if (adj) {
                last_row = (int)(gtk_adjustment_get_value(adj)/r.height);
                vis_rows = (int)(gtk_adjustment_get_page_size(adj)/r.height);
            }
        }
    }

    gtk_widget_set_sensitive(nm_rename, false);
    gtk_widget_set_sensitive(nm_remove, false);

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(NM->nm_node_list));
    gtk_tree_selection_unselect_all(sel);
    GtkListStore *store =
        GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(nm_node_list)));
    gtk_list_store_clear(store);

    GtkTreeIter iter;
    int sz = map->countNodes();
    for (int i = 0; i < sz; i++) {
        char buf1[32], buf2[4];
        snprintf(buf1, sizeof(buf1), "%d", i);
        buf2[0] = 0;
        buf2[1] = 0;
        buf2[2] = 0;
        char *strings[3];
        strings[0] = buf1;
        strings[1] = (char*)map->mapName(i);
        strings[2] = buf2;
        char *t = buf2;
        if (map->isGlobal(i) || i == 0)
            *t++ = 'G';
        if (map->isSet(i))
            *t++ = 'Y';
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, strings[0], 1, strings[1],
            2, strings[2], -1);
    }

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(NM->nm_term_list));
    gtk_tree_selection_unselect_all(sel);
    store =
        GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(nm_term_list)));
    gtk_list_store_clear(store);
    gtk_widget_set_sensitive(nm_find_btn, false);
    if (nm_node) {
        DSP()->HliteElecTerm(ERASE, nm_node, nm_cdesc, 0);
        CDterm *t;
        if (nm_cdesc)
            t = ((CDp_cnode*)nm_node)->inst_terminal();
        else
            t = ((CDp_snode*)nm_node)->cell_terminal();
        DSP()->HlitePhysTerm(ERASE, t);
        nm_node = 0;
    }
    nm_cdesc = 0;

    // The node number may have changed, try to find the new one, but
    // it may have disappeared.
    //
    if (nm_showing_node > 0 && pl) {
        bool found = false;
        for (CDpl *p = pl; p; p = p->next) {
            int node = ((CDp_cnode*)p->pdesc)->enode();
            CDpl *plx = SCD()->getElecNodeProps(cbin.elec(), node);
            for (CDpl *px = plx; px; px = px->next) {
                if (px->pdesc == p->pdesc) {
                    found = true;
                    break;
                }
            }
            CDpl::destroy(plx);
            if (found) {
                nm_showing_node = node;
                break;
            }
        }
        if (!found)
            nm_showing_node = -1;
    }
    CDpl::destroy(pl);

    if (nm_showing_node >= 0) {
        // select row
        for (int row = 0; ; row++) {
            int node = nm_node_of_row(row);
            if (node < 0)
                break;
            if (node == nm_showing_node) {
                list_select_row(NM->nm_node_list, row);
                // Make sure selection is visible.
                if (row < last_row || row >= last_row + vis_rows)
                    last_row = row;
                break;
            }
        }

        list_move_to(nm_node_list, last_row);
    }
}


void
sNM::enable_point(bool on)
{
    if (on)
        gtk_widget_set_sensitive(nm_point_btn, true);
    else {
        if (nm_cmd)
            nm_cmd->esc();
        gtk_widget_set_sensitive(nm_point_btn, false);
    }
}


// Static function.
int
sNM::nm_node_of_row(int row)
{
    char *text = list_get_text(NM->nm_node_list, row, 0);
    if (!text)
        return (-1);
    int node;
    if (sscanf(text, "%d", &node) < 1 || node < 0)
        node = -1;
    delete [] text;
    return (node);
}


// Static function.
int
sNM::nm_select_nlist_proc(GtkTreeSelection*, GtkTreeModel*,
    GtkTreePath *path, int issel, void*)
{
    if (NM) {
        if (issel) {
            NM->nm_showing_row = -1;
            gtk_widget_set_sensitive(NM->nm_rename, false);
            gtk_widget_set_sensitive(NM->nm_remove, false);
            if (NM->nm_rm_affirm)
                NM->nm_rm_affirm->popdown();
            if (NM->nm_join_affirm)
                NM->nm_join_affirm->popdown();
            if (NM->wb_input)
                NM->wb_input->popdown();
            return (true);
        }
        if (NM->nm_n_no_select) {
            // Don't let a focus event select anything!
            NM->nm_n_no_select = false;
            return (false);
        }

        // See note in tlist_proc.
        if (NM->nm_showing_row >= 0)
            return (true);

        int *indices = gtk_tree_path_get_indices(path);
        if (!indices)
            return (false);
        NM->nm_showing_row = *indices;
        int node = nm_node_of_row(*indices);
        if (node < 0)
            return (true);

        // Lock out the call back to sNM::update, select the node in
        // physical if command is active.
        NM->nm_noupdating = true;
        ExtIf()->selectShowNode(node);
        NM->nm_noupdating = false;

        NM->show_node_terms(node);
        if (DSP()->CurMode() == Electrical) {

            char *text = list_get_text(NM->nm_node_list, NM->nm_showing_row, 2);
            if (text && (text[0] == 'Y' || text[0] == 'Y')) {
                // Y, with or without G.  If G, the global name must
                // have been assigned, so editing is allowed.

                gtk_widget_set_sensitive(NM->nm_rename, true);
                gtk_widget_set_sensitive(NM->nm_remove, true);
            }
            else if (text && text[0] == 'G') {
                gtk_widget_set_sensitive(NM->nm_rename, false);
                gtk_widget_set_sensitive(NM->nm_remove, false);
            }
            else
                gtk_widget_set_sensitive(NM->nm_rename, true);
            delete [] text;
        }
        return (true);
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
sNM::nm_n_focus_proc(GtkWidget*, GdkEvent*, void*)
{
    if (NM) {
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(NM->nm_node_list));
        // If nothing selected set the flag.
        if (!gtk_tree_selection_get_selected(sel, 0, 0))
            NM->nm_n_no_select = true;
    }
    return (false);
}
        

// Static function.
int
sNM::nm_select_tlist_proc(GtkTreeSelection*, GtkTreeModel *store,
    GtkTreePath *path, int issel, void*)
{
    if (NM) {
        // The calling sequence from selections in the GtkTreeView is
        // a little screwy.  Suppose that you initially select row 1. 
        // That event will call this function once with issel false as
        // expected.  Then click row 2 and find that this function is
        // called three times:  isset=false.  row=2.  isset=true,
        // row=1, isset=false, row=2.
        //
        // int *ii = gtk_tree_path_get_indices(path);
        // printf("tlist %d %d\n", issel, *ii);

        if (issel) {
            NM->nm_showing_term_row = -1;
            gtk_widget_set_sensitive(NM->nm_find_btn, false);
            if (NM->nm_node) {
                DSP()->HliteElecTerm(ERASE, NM->nm_node, NM->nm_cdesc, 0);
                CDterm *t;
                if (NM->nm_cdesc)
                    t = ((CDp_cnode*)NM->nm_node)->inst_terminal();
                else
                    t = ((CDp_snode*)NM->nm_node)->cell_terminal();
                DSP()->HlitePhysTerm(ERASE, t);
                NM->nm_node = 0;
            }
            NM->nm_cdesc = 0;
            return (true);
        }
        if (NM->nm_t_no_select) {
            // Don't let a focus event select anything!
            NM->nm_t_no_select = false;
            return (false);
        }

        // Here's the fix for above: ignore the first call.
        if (NM->nm_showing_term_row >= 0)
            return (true);

        int *indices = gtk_tree_path_get_indices(path);
        if (!indices)
            return (false);
        NM->nm_showing_term_row = *indices;
        GtkTreeIter iter;
        if (!gtk_tree_model_get_iter(store, &iter, path))
            return (true);
        char *text;
        gtk_tree_model_get(store, &iter, 0, &text, -1);
        if (text) {
            bool torw = ((text[0] == 'T' || text[0] == 'W') && text[1] == ' ');
            // True for wire or terminal device indicators, which mean
            // nothing to the find terminal function.
            gtk_widget_set_sensitive(NM->nm_find_btn, !torw);

            if (DSP()->CurMode() == Physical) {
                CDs *psd = CurCell(Physical);
                if (psd) {
                    if (!ExtIf()->associate(psd)) {
                        Log()->ErrorLogV(mh::Processing,
                            "Association failed!\n%s", Errs()->get_error());
                    }
                }
            }
            if (!torw) {
                CDc *cdesc;
                CDp_node *pn;
                int vecix;  // unused
                if (DSP()->FindTerminal(text, &cdesc, &vecix, &pn)) {
                    DSP()->HliteElecTerm(DISPLAY, pn, cdesc, -1);
                    CDterm *t;
                    if (cdesc)
                        t = ((CDp_cnode*)pn)->inst_terminal();
                    else
                        t = ((CDp_snode*)pn)->cell_terminal();
                    DSP()->HlitePhysTerm(DISPLAY, t);
                    NM->nm_node = pn;
                    NM->nm_cdesc = cdesc;
                }
            }
            free(text);
        }
        return (true);
    }
    return (false);
}


// Static function.
// Focus handler.
//
bool
sNM::nm_t_focus_proc(GtkWidget*, GdkEvent*, void*)
{
    if (NM) {
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(NM->nm_term_list));
        // If nothing selected set the flag.
        if (!gtk_tree_selection_get_selected(sel, 0, 0))
            NM->nm_t_no_select = true;
    }
    return (false);
}
        

// Static function.
// Cancel callback, pop the widget down.
//
void
sNM::nm_cancel_proc(GtkWidget*, void*)
{
    SCD()->PopUpNodeMap(0, MODE_OFF);
}


// Static function.
// Deselect button callback, deselect window selections.
//
void
sNM::nm_desel_proc(GtkWidget*, void*)
{
    if (!NM)
        return;
    DSP()->ShowNode(ERASE, NM->nm_showing_node);
    NM->nm_showing_node = -1;

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(NM->nm_node_list));
    gtk_tree_selection_unselect_all(sel);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(NM->nm_term_list));
    gtk_tree_selection_unselect_all(sel);
    GtkListStore *store = GTK_LIST_STORE(
        gtk_tree_view_get_model(GTK_TREE_VIEW(NM->nm_term_list)));
    gtk_list_store_clear(store);

    gtk_widget_set_sensitive(NM->nm_find_btn, false);
    gtk_widget_set_sensitive(NM->nm_rename, false);
    gtk_widget_set_sensitive(NM->nm_remove, false);
    if (NM->nm_rm_affirm)
        NM->nm_rm_affirm->popdown();
    if (NM->nm_join_affirm)
        NM->nm_join_affirm->popdown();
    if (NM->wb_input)
        NM->wb_input->popdown();
    if (NM->nm_node) {
        DSP()->HliteElecTerm(ERASE, NM->nm_node, NM->nm_cdesc, 0);
        CDterm *t;
        if (NM->nm_cdesc)
            t = ((CDp_cnode*)NM->nm_node)->inst_terminal();
        else
            t = ((CDp_snode*)NM->nm_node)->cell_terminal();
        DSP()->HlitePhysTerm(ERASE, t);
        NM->nm_node = 0;
    }
    NM->nm_cdesc = 0;
}


// Static function.
// Handler for the use nophys button.
//
void
sNM::nm_use_np_proc(GtkWidget*, void*)
{
    if (!NM)
        return;
    int state = GTKdev::GetStatus(NM->nm_use_np);
    SCD()->setIncludeNoPhys(state);
    SCD()->connect(CurCell(Electrical, true));
}


// Static function.
// Handler for the name entry pop-up.
//
void
sNM::nm_name_cb(const char *name, void*)
{
    if (!NM)
        return;
    char *nametok = lstring::clip_space(name);
    if (!nametok)
        return;
    cNodeMap *map = CurCell(Electrical)->nodes();
    if (!map)
        // "can't happen"
        return;
    int node = nm_node_of_row(NM->nm_showing_row);
    if (node < 0)
        // "can't happen"
        return;

    // Check giving a name already in use.
    int nx = map->findNode(nametok);
    if (nx >= 0 && nx != node) {
        char buf[256];
        snprintf(buf, 256,
        " A net named \"%s\" already exists, and will be joined with\n"
        " the selected net.  This can make schematics difficult to trace.\n"
        " Do you wish to join the nets?", nametok);

        // Callback frees nametok.
        NM->nm_join_affirm = NM->PopUpAffirm(0, LW_CENTER, buf,
            NM->nm_join_cb, nametok);
        NM->nm_join_affirm->register_usrptr((void**)&NM->nm_join_affirm);
        return;
    }
    nm_set_name(nametok);
    delete [] nametok;
}


// Static function.
// Handler for the join nets affirmation pop-up.  This pop-up appears
// if the user gives a net name already in use.  The arg is a malloc'ed
// name token which needs to be freed.
//
void
sNM::nm_join_cb(bool yes, void *arg)
{
    char *name = (char*)arg;
    if (NM && yes)
        nm_set_name(name);
    delete [] name;
}


// Static function.
// Apply the name to the currently selected node.
//
void
sNM::nm_set_name(const char *name)
{
    cNodeMap *map = CurCell(Electrical)->nodes();
    if (!map)
        // "can't happen"
        return;
    int node = nm_node_of_row(NM->nm_showing_row);
    if (node < 0)
        // "can't happen"
        return;

    if (!map->newEntry(name, node)) {
        sLstr lstr;
        lstr.add("Operation failed: ");
        lstr.add(Errs()->get_error());
        NM->PopUpMessage(lstr.string(), true);
        return;
    }
    DSP()->ShowNode(ERASE, NM->nm_showing_node);
    NM->nm_showing_node = -1;
    NM->update_map();
    int indx = NM->find_row(name);
    if (indx >= 0) {
        list_select_row(NM->nm_node_list, indx);
        list_move_to(NM->nm_node_list, indx);
        // Row-select causes the input widget to pop-down.
        char buf[256];
        snprintf(buf, 256,
            " Name %s is linked to internal node %d. ",
            name, NM->nm_node_of_row(indx));
        NM->PopUpMessage(buf, false);
    }
    else
        NM->PopUpMessage("Operation failed, unknown error.", true);
}


// Static function.
// Handler for the rename button.
//
void
sNM::nm_rename_proc(GtkWidget *caller, void*)
{
    if (!NM)
        return;
    int state = GTKdev::GetStatus(NM->nm_rename);
    if (!state) {
        if (NM->wb_input)
            NM->wb_input->popdown();
        return;
    }
    if (NM->nm_showing_row < 0) {
        NM->PopUpMessage("No selected node to name/rename.", true);
        GTKdev::Deselect(caller);
        return;
    }

    EV()->InitCallback();
    int node = nm_node_of_row(NM->nm_showing_row);
    if (node < 0) {
        NM->PopUpMessage("Error, unknown or bad node.", true);
        GTKdev::Deselect(caller);
        return;
    }
    if (node == 0) {
        NM->PopUpMessage("Can't rename the ground node.", true);
        GTKdev::Deselect(caller);
        return;
    }

    CDs *cursde = CurCell(Electrical, true);
    if (!cursde) {
        NM->PopUpMessage("No currrent cell.", true);
        GTKdev::Deselect(caller);
        return;
    }

    cNodeMap *map = cursde->nodes();
    if (!map) {
        NM->PopUpMessage("Internal error: no map.", true);
        GTKdev::Deselect(caller);
        return;
    }
    if (map->isGlobal(node)) {
        NM->PopUpMessage("Node is global, can't set name.", true);
        GTKdev::Deselect(caller);
        return;
    }

    char buf[256];
    snprintf(buf, 256, " Name for internal node %d ? ", node);
    char *text = list_get_text(NM->nm_node_list, NM->nm_showing_row, 1);
    NM->PopUpInput(buf, text, "Apply", NM->nm_name_cb, 0);
    if (NM->wb_input)
        NM->wb_input->register_caller(NM->nm_rename);
    delete [] text;
}


// Static function.
// Handler for the remove mapping confirmation pop-up.
//
void
sNM::nm_rm_cb(bool yes, void*)
{
    char buf[256];
    if (yes && NM && NM->nm_showing_row >= 0) {
        int node = nm_node_of_row(NM->nm_showing_row);
        if (node > 0) {
            CDs *cursde = CurCell(Electrical, true);
            if (cursde) {
                cNodeMap *map = cursde->nodes();
                if (map) {
                    char *text = list_get_text(NM->nm_node_list,
                        NM->nm_showing_row, 1);
                    if (text && *text) {
                        snprintf(buf, 256,
                            "Name %s link with internal node %d removed.",
                            text, node);
                        map->delEntry(node);
                        NM->update_map();
                        NM->PopUpMessage(buf, false);
                        delete [] text;
                        return;
                    }
                    delete [] text;
                }
            }
        }
    }
    if (yes && NM)
        NM->PopUpMessage("No name/node link found.", true);
}


// Static function.
// Handler for the remove button.
//
void
sNM::nm_remove_proc(GtkWidget *caller, void*)
{
    if (!GTKdev::GetStatus(caller)) {
        if (NM && NM->nm_rm_affirm)
            NM->nm_rm_affirm->popdown();
        return;
    }

    char buf[256];
    if (NM && NM->nm_showing_row >= 0) {
        int node = nm_node_of_row(NM->nm_showing_row);
        if (node > 0) {
            CDs *cursde = CurCell(Electrical, true);
            if (cursde) {
                cNodeMap *map = cursde->nodes();
                if (map) {
                    char *text = list_get_text(NM->nm_node_list,
                        NM->nm_showing_row, 1);
                    if (text && *text) {
                        snprintf(buf, 256,
                            " Remove name %s link with internal node %d ? ",
                            text, node);
                        NM->nm_rm_affirm = NM->PopUpAffirm(caller, LW_CENTER,
                            buf, NM->nm_rm_cb, 0);
                        NM->nm_rm_affirm->register_usrptr(
                            (void**)&NM->nm_rm_affirm);
                        delete [] text;
                        return;
                    }
                    delete [] text;
                }
            }
        }
    }
    NM->PopUpMessage("No name/node link found.", false);
    GTKdev::Deselect(caller);
}


// Static function.
// Click-Select Mode button handler.
//
void
sNM::nm_point_proc(GtkWidget *caller, void*)
{
    if (!NM)
        return;
    int state = GTKdev::GetStatus(caller);
    if (sNM::nm_use_extract) {
        bool st = MainMenu()->MenuButtonStatus(MMext, MenuEXSEL);
        if (st != state)
            MainMenu()->MenuButtonPress(MMext, MenuEXSEL);
    }
    else {
        if (state) {
            if (!NM->nm_cmd) {
                EV()->InitCallback();
                NM->nm_cmd = new NmpState("NODESEL", "xic:nodmp#click");
                if (!EV()->PushCallback(NM->nm_cmd)) {
                    delete NM->nm_cmd;
                    NM->nm_cmd = 0;
                    GTKdev::Deselect(caller);
                    return;
                }
            }
            PL()->ShowPrompt("Selection of nodes in drawing is enabled.");
        }
        else {
            if (NM->nm_cmd)
                NM->nm_cmd->esc();
            PL()->ShowPrompt("Selection of nodes in drawing is disabled.");
        }
    }
}


// Static function.
//
void
sNM::nm_usex_proc(GtkWidget *caller, void*)
{
    if (!NM)
        return;
    sNM::nm_use_extract = GTKdev::GetStatus(caller);
    if (NM->nm_cmd)
        NM->nm_cmd->esc();
    NM->update(0);
}


// Static function.
//
void
sNM::nm_find_proc(GtkWidget*, void*)
{
    if (!NM)
        return;
    int row = NM->nm_showing_term_row;
    if (row < 0)
        return;
    if (sNM::nm_use_extract) {
        CDs *psd = CurCell(Physical);
        if (psd) {
            if (!ExtIf()->associate(psd)) {
                Log()->ErrorLogV(mh::Processing, "Association failed!\n%s",
                    Errs()->get_error());
            }
        }
    }

    char *text = list_get_text(NM->nm_term_list, row, 0);
    if (text && *text) {
        CDc *cdesc;
        CDp_node *pn;
        int vecix;
        if (DSP()->FindTerminal(text, &cdesc, &vecix, &pn))
            DSP()->ShowTerminal(cdesc, vecix, pn);
    }
    delete [] text;
}


// Static function.
// Click-Select Mode button handler.
//
void
sNM::nm_help_proc(GtkWidget*, void*)
{
    if (!NM)
        return;
    DSPmainWbag(PopUpHelp("xic:nodmp"))
}


// Static function.
void
sNM::nm_search_hdlr(GtkWidget*, void*)
{
    if (!NM)
        return;
    int indx, tindx;
    NM->do_search(&indx, &tindx);
    if (indx >= 0) {
        list_select_row(NM->nm_node_list, indx);
        list_move_to(NM->nm_node_list, indx);
    }
    if (tindx >= 0) {
        list_select_row(NM->nm_term_list, tindx);
        list_move_to(NM->nm_term_list, tindx);
    }
}


// Static function.
// Handler for pressing Enter in the text entry, will press the search
// button.
//
void
sNM::nm_activate_proc(GtkWidget*, void*)
{
    if (!NM)
        return;
    GTKdev::CallCallback(NM->nm_srch_btn);
}


// Function to actually perform the search.
//
void
sNM::do_search(int *pindx, int *ptindx)
{
    if (pindx)
        *pindx = -1;
    if (ptindx)
        *ptindx = -1;
    const char *str = gtk_entry_get_text(GTK_ENTRY(nm_srch_entry));
    if (!str || !*str)
        return;

    regex_t preg;
    if (regcomp(&preg, str, REG_EXTENDED | REG_ICASE | REG_NOSUB)) {
        NM->PopUpMessage("Regular expression syntax error.", true);
        return;
    }

    if (GTKdev::GetStatus(nm_srch_nodes)) {
        for (int i = nm_showing_row + 1; ; i++) {
            char *text = list_get_text(nm_node_list, i, 1);
            if (!text)
                break;
            int r = regexec(&preg, text, 0, 0, 0);
            delete [] text;
            if (!r) {
                regfree(&preg);
                if (pindx)
                    *pindx = i;
                regfree(&preg);
                return;
            }
        }
    }
    else {
        CDs *cursde = CurCell(Electrical, true);
        if (!cursde) {
            regfree(&preg);
            return;
        }
        int start = nm_showing_row;
        if (start < 0)
            start = 0;
        for (int i = start; ; i++) {
            int n = nm_node_of_row(i);
            if (n == 0)
                continue;
            if (n < 0)
                break;
            stringlist *sl = SCD()->getElecNodeContactNames(cursde, n);
            int trow = 0;
            for (stringlist *s = sl; s; s = s->next) {
                if (i == nm_showing_row && trow <= nm_showing_term_row) {
                    trow++;
                    continue;
                }
                if (!regexec(&preg, s->string, 0, 0, 0)) {
                    regfree(&preg);
                    if (pindx)
                        *pindx = i;
                    if (ptindx)
                        *ptindx = trow;
                    stringlist::destroy(sl);
                    regfree(&preg);
                    return;
                }
                trow++;
            }
            stringlist::destroy(sl);
        }
    }

    regfree(&preg);
}


// Find the row with the name given.
//
int
sNM::find_row(const char *str)
{
    if (!str || !*str)
        return (-1);
    CDnetName nn = CDnetex::name_tab_find(str);
    if (nn) {
        for (int i = 1; ; i++) {
            char *text = list_get_text(nm_node_list, i, 1);
            if (!text)
                break;
            if (!strcmp(Tstring(nn), text)) {
                delete [] text;
                return (i);
            }
            delete [] text;
        }
    }
    return (-1);
}
// End of sNM functions.


NmpState::~NmpState()
{
    if (NM)
        NM->clear_cmd();
}


void
NmpState::b1up()
{
    if (ExtIf()->hasExtract()) {
        int node = -1;
        if (DSP()->CurMode() == Electrical)
            node = ExtIf()->netSelB1Up();
        else {
            int grp = ExtIf()->netSelB1Up();
            node = ExtIf()->nodeOfGroup(CurCell(Physical), grp);
        }
        if (node >= 0) {
            NM->show_node_terms(node);
            NM->update_map();
        }
    }
    else if (DSP()->CurMode() == Electrical) {
        BBox AOI;
        CDol *sels;
        if (!cEventHdlr::sel_b1up(&AOI, 0, &sels))
            return;

        WindowDesc *wdesc = EV()->ButtonWin();
        if (!wdesc)
            wdesc = DSP()->MainWdesc();
        if (!wdesc->IsSimilar(Electrical, DSP()->MainWdesc()))
            return;
        int delta = 1 + (int)(DSP()->PixelDelta()/wdesc->Ratio());
        PSELmode ptsel = PSELpoint;
        if (AOI.width() < delta && AOI.height() < delta) {
            ptsel = PSELstrict_area;
            AOI.bloat(delta);
        }

        // check wires
        for (CDol *sl = sels; sl; sl = sl->next) {
            if (sl->odesc->type() != CDWIRE)
                continue;
            CDw *wrdesc = (CDw*)sl->odesc;
            if (!cSelections::processSelect(wrdesc, &AOI, ptsel, ASELnormal,
                    delta))
                continue;
            CDp_node *pn = (CDp_node*)wrdesc->prpty(P_NODE);
            if (pn) {
                NM->show_node_terms(pn->enode());
                NM->update_map();
                CDol::destroy(sels);
                return;
            }
        }
        CDol::destroy(sels);

        // Check subcells, can't use sels bacause only one subcell is
        // returned.
        CDs *cursde = CurCell(Electrical, true);
        if (cursde) {
            CDg gdesc;
            gdesc.init_gen(cursde, CellLayer(), &AOI);
            CDc *cdesc;
            while ((cdesc = (CDc*)gdesc.next()) != 0) {
                CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
                for ( ; pn; pn = pn->next()) {
                    for (unsigned int ix = 0; ; ix++) {
                        int x, y;
                        if (!pn->get_pos(ix, &x, &y))
                            break;
                        if (AOI.intersect(x, y, true)) {
                            NM->show_node_terms(pn->enode());
                            NM->update_map();
                            return;
                        }
                    }
                }
            }
        }
    }
}


// This allows a user to select nodes from the layout, if extraction
// has been run.  There is no visual group selection indication. 
// Better to set the Use Extraction check box and use the net
// selection panel from the extraction system.
//
void
NmpState::b1up_altw()
{
    if (EV()->CurrentWin()->CurCellName() != DSP()->MainWdesc()->CurCellName())
        return;
    if (ExtIf()->hasExtract()) {
        int node;
        if (DSP()->CurMode() == Electrical) {
            int grp = ExtIf()->netSelB1Up_altw();
            node = ExtIf()->nodeOfGroup(CurCell(Physical), grp);
        }
        else
            node = ExtIf()->netSelB1Up_altw();
        if (node > 0) {
            NM->show_node_terms(node);
            NM->update_map();
        }
    }
}


void
NmpState::esc()
{
    if (NM)
        NM->desel_point_btn();
    EV()->PopCallback(this);
    delete this;
}

