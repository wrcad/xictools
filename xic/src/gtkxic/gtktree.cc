
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
#include "editif.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_chd.h"
#include "events.h"
#include "errorlog.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkfont.h"
#include "miscutil/timer.h"
#include <algorithm>


//----------------------------------------------------------------------
//  Tree Popup
//
// Help system keywords used:
//  xic:tree

namespace {
    GtkTargetEntry target_table[] = {
        { (char*)"CELLNAME",    0, 0 },
        { (char*)"STRING",      0, 1 },
        { (char*)"text/plain",  0, 2 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

// Max number of user buttons.
#define TR_MAXBTNS 5

    namespace gtktree {
        // subclass gtk_bag so sTree can be passed to ToTop handler function
        //
        struct sTree : public gtk_bag
        {
            sTree(GRobject, const char*, TreeUpdMode);
            ~sTree();

            void update(const char*, const char*, TreeUpdMode);
            char *get_selection();

        private:
            bool check_fb();
            void check_sens();
            void build_tree(CDs*);
            void build_tree(cCHD*, symref_t*);
            bool build_tree_rc(CDs*, GtkTreeIter*, int);
            bool build_tree_rc(cCHD*, symref_t*, GtkTreeIter*, int);

            static int t_build_proc(void*);
            static int t_select_proc(GtkTreeSelection*, GtkTreeModel*,
                GtkTreePath*, int, void*);
            static bool t_focus_proc(GtkWidget*, GdkEvent*, void*);
            static int t_collapse_proc(GtkTreeView*, GtkTreeIter*,
                GtkTreePath*, void*);
            static void t_cancel(GtkWidget*, void*);
            static void t_action(GtkWidget*, void*);

            static int t_btn_hdlr(GtkWidget*, GdkEvent*, void*);
            static int t_btn_release_hdlr(GtkWidget*, GdkEvent*, void*);
            static int t_motion_hdlr(GtkWidget*, GdkEvent*, void*);
            static void t_drag_data_get(GtkWidget *widget, GdkDragContext*,
                GtkSelectionData*, guint, guint, void*);
            static int t_selection_clear(GtkWidget*, GdkEventSelection*,
                void*);
            static void t_selection_get(GtkWidget*, GtkSelectionData*, guint,
                guint, gpointer);

            GRobject t_caller;          // toggle button initiator
            GtkWidget *t_label;         // label above listing
            GtkWidget *t_tree;          // tree listing
            GtkWidget *t_buttons[TR_MAXBTNS]; // user buttons
            GtkTreePath *t_curnode;     // selected node
            char *t_selection;          // selected text
            char *t_root_cd;            // name of root cell
            char *t_root_db;            // name of root cell
            DisplayMode t_mode;         // display mode of cells
            bool t_dragging;            // drag/drop
            bool t_no_select;           // treeview focus hack
            int t_dragX, t_dragY;       // drag/drop
            unsigned int t_ucount;      // user feedback counter
            unsigned int t_udel;        // user feedback increment
            int t_mdepth;               // max depth
            unsigned long t_check_time; // interval test
        };

        sTree *Tree;
    }
}

using namespace gtktree;

#define TREE_WIDTH 400
#define TREE_HEIGHT 300


// Static function.
//
char *
main_bag::get_tree_selection()
{
    if (Tree)
        return (Tree->get_selection());
    return (0);
}


// Static function.
// Called on crash to prevent updates.
//
void
main_bag::tree_panic()
{
    Tree = 0;
}


// Pop up a window displaying a tree diagram of the hierarchy of the
// cell named in root, which should exist in memory for the given
// display mode.
//
void
cMain::PopUpTree(GRobject caller, ShowMode mode, const char *root,
    TreeUpdMode dmode, const char *oldroot)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Tree;
        return;
    }

    if (mode == MODE_UPD || (mode == MODE_ON && Tree)) {
        if (Tree)
            Tree->update(root, oldroot, dmode);
        return;
    }

    if ((!root || !*root) && DSP()->MainWdesc()->DbType() == WDcddb)
        return;

    new sTree(caller, root, dmode);
    if (!Tree->Shell()) {
        delete Tree;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Tree->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), Tree->Shell(), mainBag()->Viewport());
    gtk_widget_show(Tree->Shell());
}


#define TR_INFO_BTN     "Info"
#define TR_OPEN_BTN     "Open"
#define TR_PLACE_BTN    "Place"
#define TR_UPD_BTN      "Update"

sTree::sTree(GRobject c, const char *root, TreeUpdMode dmode)
{
    Tree = this;
    t_caller = c;
    t_label = 0;
    t_tree = 0;
    t_curnode = 0;
    t_selection = 0;
    t_root_cd = 0;
    t_root_db = 0;

    // The display mode of the hierarchy shown is set here.
    if (dmode == TU_CUR)
        t_mode = DSP()->CurMode();
    else if (dmode == TU_PHYS)
        t_mode = Physical;
    else
        t_mode = Electrical;

    t_dragging = false;
    t_no_select = false;
    t_dragX = t_dragY = 0;
    t_ucount = 0;
    t_udel = 0;
    t_mdepth = 0;
    t_check_time = 0;

    for (int i = 0; i < TR_MAXBTNS; i++)
        t_buttons[i] = 0;

    wb_shell = gtk_NewPopup(0, "Cell Hierarchy Tree", t_cancel, 0);
    if (!wb_shell)
        return;
    gtk_window_set_default_size(GTK_WINDOW(wb_shell), TREE_WIDTH,
        TREE_HEIGHT);
    if (DSP()->MainWdesc()->DbType() == WDchd)
        t_root_db = lstring::copy(root);
    else
        t_root_cd = lstring::copy(root);

    GtkWidget *form = gtk_table_new(1, 3, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    t_label = gtk_label_new("");
    gtk_widget_show(t_label);
    gtk_misc_set_padding(GTK_MISC(t_label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), t_label);
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    gtk_box_pack_start(GTK_BOX(row), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(t_action), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // scrolled tree
    //
    GtkWidget *swin = gtk_scrolled_window_new(0, 0);
    gtk_widget_show(swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_set_border_width(GTK_CONTAINER(swin), 2);

    GtkTreeStore *store = gtk_tree_store_new(1, G_TYPE_STRING);
    t_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_show(t_tree);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(t_tree), false);
    GtkCellRenderer *rnd = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *tvcol =
            gtk_tree_view_column_new_with_attributes(0, rnd,
        "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(t_tree), tvcol);
#if GTK_CHECK_VERSION(2,12,0)
    gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(t_tree), true);
#endif
    gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(t_tree), true);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(t_tree), false);

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(t_tree));
    gtk_tree_selection_set_select_function(sel, t_select_proc, 0, 0);
    // TreeView bug hack, see note with handlers.   
    gtk_signal_connect(GTK_OBJECT(t_tree), "focus",
        GTK_SIGNAL_FUNC(t_focus_proc), this);

    gtk_container_add(GTK_CONTAINER(swin), t_tree);

    // Set up font and tracking.
    GTKfont::setupFont(t_tree, FNT_PROP, true);

    gtk_table_attach(GTK_TABLE(form), swin, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    gtk_signal_connect(GTK_OBJECT(t_tree), "test_collapse_row",
        (GtkSignalFunc)t_collapse_proc, 0);
    // init for drag/drop
    gtk_signal_connect(GTK_OBJECT(t_tree), "button-press-event",
        GTK_SIGNAL_FUNC(t_btn_hdlr), 0);
    gtk_signal_connect(GTK_OBJECT(t_tree), "button-release-event",
        GTK_SIGNAL_FUNC(t_btn_release_hdlr), 0);
    gtk_signal_connect(GTK_OBJECT(t_tree), "motion-notify-event",
        GTK_SIGNAL_FUNC(t_motion_hdlr), 0);
    gtk_signal_connect(GTK_OBJECT(t_tree), "drag-data-get",
        GTK_SIGNAL_FUNC(t_drag_data_get), 0);

    gtk_selection_add_targets(t_tree, GDK_SELECTION_PRIMARY, target_table,
        n_targets);
    gtk_signal_connect(GTK_OBJECT(t_tree), "selection-clear-event",
        GTK_SIGNAL_FUNC(t_selection_clear), 0);
    gtk_signal_connect(GTK_OBJECT(t_tree), "selection-get",
        GTK_SIGNAL_FUNC(t_selection_get), 0);

    wb_textarea = gtk_label_new("");
    gtk_widget_show(wb_textarea);
    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), wb_textarea);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    //
    // dismiss button line
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(t_cancel), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
    GtkWidget *dismiss_btn = button;

    const char *buttons[5];
    buttons[0] = TR_INFO_BTN;
    buttons[1] = TR_OPEN_BTN;
    if (EditIf()->hasEdit()) {
        buttons[2] = TR_PLACE_BTN;
        buttons[3] = TR_UPD_BTN;
    }
    else {
        buttons[2] = 0;
        buttons[3] = 0;
    }
    buttons[4] = 0;

    for (int i = 0; i < TR_MAXBTNS && buttons[i]; i++) {
        button = gtk_button_new_with_label(buttons[i]);
        gtk_widget_set_name(button, buttons[i]);
        gtk_widget_show(button);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
            GTK_SIGNAL_FUNC(t_action), 0);
        gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
        t_buttons[i] = button;
    }

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), dismiss_btn);

    if (t_caller)
        gtk_signal_connect(GTK_OBJECT(t_caller), "toggled",
            GTK_SIGNAL_FUNC(t_cancel), 0);

    update(0, 0, dmode);
}


sTree::~sTree()
{
    Tree = 0;
    XM()->SetTreeCaptive(false);
    delete [] t_root_cd;
    delete [] t_root_db;
    delete [] t_selection;
    if (t_curnode)
        gtk_tree_path_free(t_curnode);
    if (t_caller) {
        gtk_signal_disconnect_by_func(GTK_OBJECT(t_caller),
            GTK_SIGNAL_FUNC(t_cancel), 0);
        GRX->Deselect(t_caller);
    }
    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(t_cancel), wb_shell);
}


void
sTree::update(const char *root, const char *oldroot, TreeUpdMode dmode)
{
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        if (root && (!t_root_db || strcmp(root, t_root_db))) {
            delete [] t_root_db;
            t_root_db = lstring::copy(root);
        }
    }
    else {
        // If root is nil, reuse same root name.  If oldroot is not nil
        // it is the old name of a cell undergoing name change, with
        // root the new name.
        if (oldroot) {
            if (root && t_root_cd && !strcmp(oldroot, t_root_cd)) {
                delete [] t_root_cd;
                t_root_cd = lstring::copy(root);
            }
        }
        else if (root && (!t_root_cd || strcmp(root, t_root_cd))) {
            delete [] t_root_cd;
            t_root_cd = lstring::copy(root);
        }
    }

    if (t_caller)
        GRX->Select(t_caller);
    delete [] t_selection;
    t_selection = 0;
    if (Tree->t_curnode)
        gtk_tree_path_free(Tree->t_curnode);
    t_curnode = 0;

    if (dmode == TU_PHYS)
        t_mode = Physical;
    else if (dmode == TU_ELEC)
        t_mode = Electrical;
    // Else TU_CUR, keep the present display mode.

    char buf[128];
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        sprintf(buf, "%s cells from saved hierarchy named %s",
            DisplayModeName(t_mode), DSP()->MainWdesc()->DbName());
    }
    else
        sprintf(buf, "%s cells from memory", DisplayModeName(t_mode));
    gtk_label_set_text(GTK_LABEL(t_label), buf);

    gtk_label_set_text(GTK_LABEL(wb_textarea),
        "Adding nodes, please wait...");
    dspPkgIf()->RegisterTimeoutProc(200, t_build_proc, 0);
    check_sens();
}


char *
sTree::get_selection()
{
    if (t_selection && *t_selection)
        return (lstring::copy(t_selection));
    return (0);
}


// Check for interrupts and print update message while building tree.
//
bool
sTree::check_fb()
{
    if (!Tree)
        return (true);
    t_ucount++;
    if (!(t_ucount & t_udel)) {
        char buf[256];
        sprintf(buf, "%s:  %u", "Nodes processed", t_ucount);
        gtk_label_set_text(GTK_LABEL(wb_textarea), buf);
    }
    if (Timer()->check_interval(t_check_time)) {
        if (DSP()->MainWdesc() && DSP()->MainWdesc()->Wdraw())
            dspPkgIf()->CheckForInterrupt();
        return (XM()->ConfirmAbort());
    }
    return (false);
}


void
sTree::check_sens()
{
    bool has_sel = (t_selection != 0);
    for (int i = 0; i < TR_MAXBTNS && t_buttons[i]; i++) {
        const char *name = gtk_widget_get_name(t_buttons[i]);
        if (!name)
            continue;
        if (!strcmp(TR_INFO_BTN, name))
            gtk_widget_set_sensitive(t_buttons[i], has_sel);
        else if (!strcmp(TR_OPEN_BTN, name)) {
            if (DSP()->MainWdesc()->DbType() == WDchd)
                gtk_widget_set_sensitive(t_buttons[i], false);
            else
                gtk_widget_set_sensitive(t_buttons[i], has_sel);
        }
        else if (!strcmp(TR_PLACE_BTN, name)) {
            if (DSP()->MainWdesc()->DbType() == WDchd)
                gtk_widget_set_sensitive(t_buttons[i], false);
            else
                gtk_widget_set_sensitive(t_buttons[i], has_sel);
        }
    }
}


// Main entry to build the tree, using CDs objects.
//
void
sTree::build_tree(CDs *sdesc)
{
    dspPkgIf()->SetWorking(true);
    t_ucount = 0;
    t_udel = (1 << 10) - 1;
    t_mdepth = 0;

    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(t_tree)));
    gtk_tree_store_clear(store);

    bool ret = build_tree_rc(sdesc, 0, 0);
    if (Tree && GTK_IS_TREE_VIEW(t_tree)) {
        char buf[256];
        if (ret)
            sprintf(buf, "Total Nodes: %u   Max Depth: %d", t_ucount,
                t_mdepth+1);
        else
            strcpy(buf, "Aborted, content incomplete.");
        gtk_label_set_text(GTK_LABEL(wb_textarea), buf);
        GtkTreePath *p = gtk_tree_path_new_from_indices(0, -1);
        gtk_tree_view_expand_row(GTK_TREE_VIEW(t_tree), p, false);
        gtk_tree_path_free(p);
    }
    dspPkgIf()->SetWorking(false);
}


// Main entry to build the tree, using symref_t objects.
//
void
sTree::build_tree(cCHD *chd, symref_t *p)
{
    dspPkgIf()->SetWorking(true);
    t_ucount = 0;
    t_udel = (1 << 10) - 1;
    t_mdepth = 0;

    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(t_tree)));
    gtk_tree_store_clear(store);

    bool ret = build_tree_rc(chd, p, 0, 0);
    if (Tree && GTK_IS_TREE_VIEW(t_tree)) {
        char buf[256];
        if (ret)
            sprintf(buf, "Total Nodes: %u   Max Depth: %d", t_ucount,
                t_mdepth+1);
        else
            strcpy(buf, "Aborted, content incomplete.");
        gtk_label_set_text(GTK_LABEL(wb_textarea), buf);
    }
    dspPkgIf()->SetWorking(false);
}


namespace {
    // Comparison function for master name sorting.
    //
    inline bool
    m_cmp(const CDm *m1, const CDm *m2)
    {
        return (strcmp(Tstring(m1->cellname()), Tstring(m2->cellname())) < 0);
    }
}


// Recursively add the hierarchy to the tree (using CDs objects).
//
bool
sTree::build_tree_rc(CDs *sdesc, GtkTreeIter *parent, int dpt)
{
    if (!sdesc || !Tree || !GTK_IS_TREE_VIEW(t_tree))
        return (false);

    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(t_tree)));
    GtkTreeIter iter;
    gtk_tree_store_append(store, &iter, parent);
    gtk_tree_store_set(store, &iter, 0, Tstring(sdesc->cellname()), -1);

    if (dpt > t_mdepth)
        t_mdepth = dpt;
    if (check_fb())
        return (false);
    if (!Tree)
        return (false);

    // The sorting function in gtk is too slow, so we do sorting
    // ourselves.
    CDm_gen mgen(sdesc, GEN_MASTERS);
    int icnt = 0;
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        if (!mdesc->hasInstances())
            continue;
        CDs *msd = mdesc->celldesc();
        if (!msd)
            continue;
        if (msd->isDevice() && msd->isLibrary()) {
            // don't include library devices
            continue;
        }
        if (msd->isViaSubMaster()) {
            // don't include via cells
            continue;
        }
        icnt++;
    }
    CDm **ary = new CDm*[icnt];
    icnt = 0;
    mgen = CDm_gen(sdesc, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        if (!mdesc->hasInstances())
            continue;
        CDs *msd = mdesc->celldesc();
        if (!msd)
            continue;
        if (msd->isDevice() && msd->isLibrary()) {
            // don't include library devices
            continue;
        }
        if (msd->isViaSubMaster()) {
            // don't include via cells
            continue;
        }
        ary[icnt] = mdesc;
        icnt++;
    }
    std::sort(ary, ary + icnt, m_cmp);
    for (int i = 0; i < icnt; i++) {
        if (!build_tree_rc(ary[i]->celldesc(), &iter, dpt+1)) {
            delete [] ary;
            return (false);
        }
    }
    delete [] ary;
    return (true);
}


// Recursively add the hierarchy to the tree (using symref_t objects).
//
bool
sTree::build_tree_rc(cCHD *chd, symref_t *p, GtkTreeIter *parent, int dpt)
{
    if (!p || !Tree || !GTK_IS_TREE_VIEW(t_tree))
        return (false);

    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(t_tree)));
    GtkTreeIter iter;
    gtk_tree_store_append(store, &iter, parent);
    gtk_tree_store_set(store, &iter, 0, Tstring(p->get_name()), -1);

    if (dpt > t_mdepth)
        t_mdepth = dpt;
    if (check_fb())
        return (false);
    if (!Tree)
        return (false);

    SymTab *xtab = new SymTab(false, false);
    syrlist_t *s0 = 0;
    nametab_t *ntab = chd->nameTab(Tree->t_mode);
    crgen_t cgen(ntab, p);
    const cref_o_t *c;
    while ((c = cgen.next()) != 0) {
        symref_t *cp = ntab->find_symref(c->srfptr);
        if (cp && SymTab::get(xtab, (unsigned long)cp) == ST_NIL) {
            s0 = new syrlist_t(cp, s0);
            xtab->add((unsigned long)cp, 0, false);
        }
    }
    delete xtab;
    syrlist_t::sort(s0, false);

    for (syrlist_t *sr = s0; sr; sr = sr->next) {
        if (!build_tree_rc(chd, sr->symref, &iter, dpt+1)) {
            syrlist_t::destroy(s0);
            return (false);
        }
    }
    syrlist_t::destroy(s0);
    return (true);
}


// Static function.
// Timer callback for initial build.
//
int
sTree::t_build_proc(void*)
{
    bool ok = false;
    if (Tree) {
        if (DSP()->MainWdesc()->DbType() == WDcddb) {
            if (!Tree->t_root_cd)
                // "can't happen"
                Log()->ErrorLog(mh::Processing, "Null cell name encountered.");
            else {
                CDcbin cbin;
                if (CDcdb()->findSymbol(Tree->t_root_cd, &cbin)) {
                    CDs *sdesc = cbin.celldesc(Tree->t_mode);
                    if (sdesc) {
                        Tree->build_tree(sdesc);
                        ok = true;
                    }
                    else
                        Log()->ErrorLogV(mh::Processing,
                            "Null cell pointer encountered in cell %s.",
                            Tree->t_root_cd);
                }
                else
                    Log()->ErrorLogV(mh::Processing,
                        "Can't find cell %s for tree display.",
                        Tree->t_root_cd);
            }
        }
        else if (DSP()->MainWdesc()->DbType() == WDchd) {
            const char *dbname = DSP()->MainWdesc()->DbName();
            if (dbname) {
                cCHD *chd = CDchd()->chdRecall(dbname, false);
                if (chd) {
                    symref_t *p = chd->findSymref(Tree->t_root_db,
                        Tree->t_mode, true);
                    if (p) {
                        Tree->build_tree(chd, p);
                        ok = true;
                    }
                    else
                        Log()->ErrorLogV(mh::Processing,
                            "Can't find name %s in database.",
                            Tree->t_root_db ? Tree->t_root_db : "<null>");
                }
                else {
                    Log()->ErrorLogV(mh::Processing,
                        "Can't find hierarchy named %s.", dbname);
                }
            }
            else {
                Log()->ErrorLog(mh::Processing,
                    "Null database name encountered.");
            }
        }
        Tree->check_sens();
    }
    if (!ok)
        XM()->PopUpTree(0, MODE_OFF, 0, TU_CUR);
    return (false);
}


// Static function.
//
int
sTree::t_select_proc(GtkTreeSelection*, GtkTreeModel *store,
    GtkTreePath *path, int issel, void*)
{
    if (!Tree)
        return (false);
    if (Tree->t_no_select && !issel)
        return (false);
    delete [] Tree->t_selection;
    Tree->t_selection = 0;
    if (Tree->t_curnode)
        gtk_tree_path_free(Tree->t_curnode);
    Tree->t_curnode = 0;
    if (!issel) {
        char *cname = 0;
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(store, &iter, path))
            gtk_tree_model_get(store, &iter, 0, &cname, -1);
        if (cname) {
            Tree->t_selection = lstring::tocpp(cname);
            Tree->t_curnode = gtk_tree_path_copy(path);

            gtk_selection_owner_set(Tree->t_tree, GDK_SELECTION_PRIMARY,
                GDK_CURRENT_TIME);
        }
    }
    Tree->check_sens();
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
sTree::t_focus_proc(GtkWidget*, GdkEvent*, void*)
{
    if (Tree) {
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(Tree->t_tree));
        // If nothing selected set the flag.
        if (!gtk_tree_selection_get_selected(sel, 0, 0))
            Tree->t_no_select = true;
    }
    return (false);
}


// Static function.
// If the selected row is collapsed, deselect.  This does not happen
// automatically.
//
int
sTree::t_collapse_proc(GtkTreeView *tv, GtkTreeIter*, GtkTreePath *path, void*)
{
    if (!Tree)
        return (true);
    if (Tree->t_curnode && gtk_tree_path_is_ancestor(path, Tree->t_curnode)) {
        GtkTreeSelection *sel = gtk_tree_view_get_selection(tv);
        gtk_tree_selection_unselect_path(sel, Tree->t_curnode);
    }
    return (false);
}


// Static function.
// Pop down the tree popup.
//
void
sTree::t_cancel(GtkWidget*, void*)
{
    XM()->PopUpTree(0, MODE_OFF, 0, TU_CUR);
}


// Static function.
//
void
sTree::t_action(GtkWidget *widget, void*)
{
    if (!Tree)
        return;
    const char *wname = gtk_widget_get_name(widget);
    if (!wname)
        return;
    if (!strcmp("Help", wname)) {
        DSPmainWbag(PopUpHelp("xic:tree"))
        return;
    }
    if (!strcmp(TR_INFO_BTN, wname)) {
        if (!Tree->t_selection)
            return;
        if (DSP()->MainWdesc()->DbType() == WDchd) {
            cCHD *chd = CDchd()->chdRecall(DSP()->MainWdesc()->DbName(),
                false);
            if (!chd)
                return;
            symref_t *p = chd->findSymref(Tree->t_selection,
                Tree->t_mode, true);
            if (!p)
                return;
            stringlist *sl = new stringlist(
                lstring::copy(Tstring(p->get_name())), 0);
            int flgs = FIO_INFO_OFFSET | FIO_INFO_INSTANCES |
                FIO_INFO_BBS | FIO_INFO_FLAGS;
            char *str = chd->prCells(0, Tree->t_mode, flgs, sl);
            stringlist::destroy(sl);
            Tree->PopUpInfo(MODE_ON, str, STY_FIXED);
            delete [] str;
        }
        else
            XM()->ShowCellInfo(Tree->t_selection, true, Tree->t_mode);
    }
    else if (!strcmp(TR_OPEN_BTN, wname)) {
        if (!Tree->t_selection)
            return;
        XM()->Load(DSP()->MainWdesc(), Tree->t_selection);
    }
    else if (!strcmp(TR_PLACE_BTN, wname)) {
        if (!Tree->t_selection)
            return;
        if (DSP()->MainWdesc()->DbType() == WDcddb) {
            EV()->InitCallback();
            EditIf()->addMaster(Tree->t_selection, 0);
        }
    }
    else if (!strcmp(TR_UPD_BTN, wname)) {
        XM()->PopUpTree(0, MODE_UPD, 0, TU_CUR);
    }
}


// Static function.
//
int
sTree::t_btn_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    if (Tree && event->type == GDK_BUTTON_PRESS) {
        Tree->t_dragging = true;
        Tree->t_dragX = (int)event->button.x;
        Tree->t_dragY = (int)event->button.y;
    }
    return (false);
}


// Static function.
//
int
sTree::t_btn_release_hdlr(GtkWidget*, GdkEvent*, void*)
{
    if (Tree)
        Tree->t_dragging = false;
    return (false);
}


// Static function.
// Motion handler, begin drag.
//
int
sTree::t_motion_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (Tree && Tree->t_dragging) {
        if ((abs((int)event->motion.x - Tree->t_dragX) > 4 ||
                abs((int)event->motion.y - Tree->t_dragY) > 4)) {
            Tree->t_dragging = false;
            GtkTargetList *targets = gtk_target_list_new(target_table,
                n_targets);
            GdkDragContext *drcx = gtk_drag_begin(caller, targets,
                (GdkDragAction)GDK_ACTION_COPY, 1, event);
            gtk_drag_set_icon_default(drcx);
            return (true);
        }
    }
    return (false);
}


// Static function.
// Data-get function, for drag/drop.
//
void
sTree::t_drag_data_get(GtkWidget*, GdkDragContext*,
    GtkSelectionData *selection_data, guint, guint, void*)
{
    if (!Tree || !Tree->t_curnode || !Tree->t_selection)
        return;
    gtk_selection_data_set(selection_data, selection_data->target,
        8, (unsigned char*)Tree->t_selection, strlen(Tree->t_selection)+1);
}


// Static function.
// Selection clear handler.
//
int
sTree::t_selection_clear(GtkWidget*, GdkEventSelection*, void*)
{
    if (Tree && Tree->t_curnode) {
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(Tree->t_tree));
        gtk_tree_selection_unselect_path(sel, Tree->t_curnode);
    }
    return (true);
}


// Static function.
//
void
sTree::t_selection_get(GtkWidget*, GtkSelectionData *selection_data,
    guint, guint, void*)
{
    if (selection_data->selection != GDK_SELECTION_PRIMARY)
        return;
    if (!Tree || !Tree->t_curnode || !Tree->t_selection)
        return;
    gtk_selection_data_set(selection_data, selection_data->target,
        8, (unsigned char*)Tree->t_selection, strlen(Tree->t_selection)+1);
}

