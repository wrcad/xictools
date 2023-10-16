
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

#include "qttree.h"
#include "editif.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_chd.h"
#include "events.h"
#include "errorlog.h"
#include "qtinterf/qtfont.h"
#include "miscutil/timer.h"
#include <algorithm>

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>


//----------------------------------------------------------------------
//  Tree Popup
//
// Help system keywords used:
//  xic:tree

namespace {
    /*
    GtkTargetEntry target_table[] = {
        { (char*)"CELLNAME",    0, 0 },
        { (char*)"STRING",      0, 1 },
        { (char*)"text/plain",  0, 2 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);
    */
}

#define TREE_WIDTH 400
#define TREE_HEIGHT 300


// Static function.
//
char *
QTmainwin::get_tree_selection()
{
    if (QTtreeDlg::self())
        return (QTtreeDlg::self()->get_selection());
    return (0);
}


// Static function.
// Called on crash to prevent updates.
//
void
QTmainwin::tree_panic()
{
    QTtreeDlg::set_panic();
}


// Pop up a window displaying a tree diagram of the hierarchy of the
// cell named in root, which should exist in memory for the given
// display mode.
//
void
cMain::PopUpTree(GRobject caller, ShowMode mode, const char *root,
    TreeUpdMode dmode, const char *oldroot)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTtreeDlg::self())
            QTtreeDlg::self()->deleteLater();
        return;
    }

    if (mode == MODE_UPD || (mode == MODE_ON && QTtreeDlg::self())) {
        if (QTtreeDlg::self())
            QTtreeDlg::self()->update(root, oldroot, dmode);
        return;
    }

    if ((!root || !*root) && DSP()->MainWdesc()->DbType() == WDcddb)
        return;

    new QTtreeDlg(caller, root, dmode);

    QTdev::self()->SetPopupLocation(GRloc(), QTtreeDlg::self(),
        QTmainwin::self()->Viewport());
    QTtreeDlg::self()->show();
}
// End of cMain functions.


#define TR_INFO_BTN     "Info"
#define TR_OPEN_BTN     "Open"
#define TR_PLACE_BTN    "Place"
#define TR_UPD_BTN      "Update"

QTtreeDlg *QTtreeDlg::instPtr;

QTtreeDlg::QTtreeDlg(GRobject c, const char *root, TreeUpdMode dmode)
{
    instPtr = this;
    t_caller = c;
    t_label = 0;
    t_info = 0;
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

    setWindowTitle(tr("Cell Hierarchy Tree"));
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_ShowWithoutActivating);

    if (DSP()->MainWdesc()->DbType() == WDchd)
        t_root_db = lstring::copy(root);
    else
        t_root_cd = lstring::copy(root);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    t_label = new QLabel("");
    hb->addWidget(t_label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // scrolled tree
    //
    t_tree = new QTreeWidget();
    vbox->addWidget(t_tree);

    connect(t_tree,
        SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgretItem*)),
        this,
        SLOT(current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(t_tree, SIGNAL(itemActivated(QTreeWidgetItem*, int)),
        this, SLOT(item_activated(QTreeWidgetItem*, int)));
    connect(t_tree, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
        this, SLOT(item_changed(QTreeWidgetItem*, int)));
    connect(t_tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
        this, SLOT(item_clicked(QTreeWidgetItem*, int)));
    connect(t_tree, SIGNAL(itemCollapsed(QTreeWidgetItem*, int)),
        this, SLOT(item_collapsed(QTreeWidgetItem*, int)));
    connect(t_tree, SIGNAL(itemExpanded(QTreeWidgetItem*, int)),
        this, SLOT(item_expanded(QTreeWidgetItem*, int)));

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_PROP))
        t_tree->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    /*
    GtkTreeStore *store = gtk_tree_store_new(1, G_TYPE_STRING);
    t_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_show(t_tree);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(t_tree), false);
    GtkCellRenderer *rnd = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *tvcol =
            gtk_tree_view_column_new_with_attributes(0, rnd,
        "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(t_tree), tvcol);
    gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(t_tree), true);
    gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(t_tree), true);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(t_tree), false);

    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(t_tree));
    gtk_tree_selection_set_select_function(sel, t_select_proc, 0, 0);
    // TreeView bug hack, see note with handlers.   
    g_signal_connect(G_OBJECT(t_tree), "focus",
        G_CALLBACK(t_focus_proc), this);
    */


/*
    // Set up font and tracking.
    GTKfont::setupFont(t_tree, FNT_PROP, true);
*/

/*
    g_signal_connect(G_OBJECT(t_tree), "test_collapse_row",
        (GCallback)t_collapse_proc, 0);
    // init for drag/drop
    g_signal_connect(G_OBJECT(t_tree), "button-press-event",
        G_CALLBACK(t_btn_hdlr), 0);
    g_signal_connect(G_OBJECT(t_tree), "button-release-event",
        G_CALLBACK(t_btn_release_hdlr), 0);
    g_signal_connect(G_OBJECT(t_tree), "motion-notify-event",
        G_CALLBACK(t_motion_hdlr), 0);
    g_signal_connect(G_OBJECT(t_tree), "drag-data-get",
        G_CALLBACK(t_drag_data_get), 0);

    gtk_selection_add_targets(t_tree, GDK_SELECTION_PRIMARY, target_table,
        n_targets);
    g_signal_connect(G_OBJECT(t_tree), "selection-clear-event",
        G_CALLBACK(t_selection_clear), 0);
    g_signal_connect(G_OBJECT(t_tree), "selection-get",
        G_CALLBACK(t_selection_get), 0);
*/
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    gb = new QGroupBox();
    hbox->addWidget(gb);
    hb = new QHBoxLayout(gb);

    t_info = new QLabel("");
    hb->addWidget(t_info);

    // dismiss button line
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

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
        btn = new QPushButton(tr(buttons[i]));
        connect(btn, SIGNAL(clicked()), this, SLOT(user_btn_slot()));
        t_buttons[i] = btn;
        hbox->addWidget(btn);
    }

/*
    if (t_caller) {
        g_signal_connect(G_OBJECT(t_caller), "toggled",
            G_CALLBACK(t_cancel), 0);
    }
*/

    update(0, 0, dmode);
}


QTtreeDlg::~QTtreeDlg()
{
    instPtr = 0;
    XM()->SetTreeCaptive(false);
    delete [] t_root_cd;
    delete [] t_root_db;
    delete [] t_selection;
//    if (t_curnode)
//        gtk_tree_path_free(t_curnode);
    if (t_caller)
        QTdev::Deselect(t_caller);
}


void
QTtreeDlg::update(const char *root, const char *oldroot, TreeUpdMode dmode)
{
    if (!instPtr)
        return;
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
        QTdev::Select(t_caller);
    delete [] t_selection;
    t_selection = 0;
//    if (Tree->t_curnode)
//        gtk_tree_path_free(Tree->t_curnode);
    t_curnode = 0;

    if (dmode == TU_PHYS)
        t_mode = Physical;
    else if (dmode == TU_ELEC)
        t_mode = Electrical;
    // Else TU_CUR, keep the present display mode.

    char buf[128];
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        snprintf(buf, sizeof(buf), "%s cells from saved hierarchy named %s",
            DisplayModeName(t_mode), DSP()->MainWdesc()->DbName());
    }
    else {
        snprintf(buf, sizeof(buf), "%s cells from memory",
            DisplayModeName(t_mode));
    }
    t_label->setText(buf);

    t_info->setText(tr("Adding nodes, please wait..."));
    QTpkg::self()->RegisterTimeoutProc(200, t_build_proc, 0);
    check_sens();
}


char *
QTtreeDlg::get_selection()
{
    if (t_selection && *t_selection)
        return (lstring::copy(t_selection));
    return (0);
}


// Check for interrupts and print update message while building tree.
//
bool
QTtreeDlg::check_fb()
{
    if (!instPtr)
        return (true);
    t_ucount++;
    if (!(t_ucount & t_udel)) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s:  %u", "Nodes processed", t_ucount);
        t_info->setText(buf);
    }
    if (Timer()->check_interval(t_check_time)) {
        if (DSP()->MainWdesc() && DSP()->MainWdesc()->Wdraw())
            QTpkg::self()->CheckForInterrupt();
        return (XM()->ConfirmAbort());
    }
    return (false);
}


void
QTtreeDlg::check_sens()
{
    bool has_sel = (t_selection != 0);
    for (int i = 0; i < TR_MAXBTNS && t_buttons[i]; i++) {
        QString name = t_buttons[i]->text();
        if (name == TR_INFO_BTN)
            t_buttons[i]->setEnabled(has_sel);
        else if (name == TR_OPEN_BTN) {
            if (DSP()->MainWdesc()->DbType() == WDchd)
                t_buttons[i]->setEnabled(false);
            else
                t_buttons[i]->setEnabled(has_sel);
        }
        else if (name == TR_PLACE_BTN) {
            if (DSP()->MainWdesc()->DbType() == WDchd)
                t_buttons[i]->setEnabled(false);
            else
                t_buttons[i]->setEnabled(has_sel);
        }
    }
}


// Main entry to build the tree, using CDs objects.
//
void
QTtreeDlg::build_tree(CDs *sdesc)
{
    QTpkg::self()->SetWorking(true);
    t_ucount = 0;
    t_udel = (1 << 10) - 1;
    t_mdepth = 0;

    /*
    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(t_tree)));
    gtk_tree_store_clear(store);
    */
    t_tree->clear();

    bool ret = build_tree_rc(sdesc, 0, 0);
    if (instPtr) {
        char buf[256];
        if (ret)
            snprintf(buf, sizeof(buf), "Total Nodes: %u   Max Depth: %d",
                t_ucount, t_mdepth+1);
        else
            strcpy(buf, "Aborted, content incomplete.");
        t_info->setText(buf);
/*
        GtkTreePath *p = gtk_tree_path_new_from_indices(0, -1);
        gtk_tree_view_expand_row(GTK_TREE_VIEW(t_tree), p, false);
        gtk_tree_path_free(p);
*/
    }
    QTpkg::self()->SetWorking(false);
}


// Main entry to build the tree, using symref_t objects.
//
void
QTtreeDlg::build_tree(cCHD *chd, symref_t *p)
{
    QTpkg::self()->SetWorking(true);
    t_ucount = 0;
    t_udel = (1 << 10) - 1;
    t_mdepth = 0;

    /*
    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(t_tree)));
    gtk_tree_store_clear(store);
    */
    t_tree->clear();

    bool ret = build_tree_rc(chd, p, 0, 0);
    if (instPtr) {
        char buf[256];
        if (ret)
            snprintf(buf, sizeof(buf), "Total Nodes: %u   Max Depth: %d",
                t_ucount, t_mdepth+1);
        else
            strcpy(buf, "Aborted, content incomplete.");
        t_info->setText(buf);
    }
    QTpkg::self()->SetWorking(false);
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
QTtreeDlg::build_tree_rc(CDs *sdesc, QTreeWidgetItem *prnt, int dpt)
{
    if (!sdesc || !instPtr)
        return (false);

/*
    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(t_tree)));
    GtkTreeIter iter;
    gtk_tree_store_append(store, &iter, prnt);
    gtk_tree_store_set(store, &iter, 0, Tstring(sdesc->cellname()), -1);
*/
    QTreeWidgetItem *item = new QTreeWidgetItem(prnt);
    item->setText(0, Tstring(sdesc->cellname()));
    if (!prnt)
        t_tree->addTopLevelItem(item);

    if (dpt > t_mdepth)
        t_mdepth = dpt;
    if (check_fb())
        return (false);
    if (!instPtr)
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
        if (!build_tree_rc(ary[i]->celldesc(), item, dpt+1)) {
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
QTtreeDlg::build_tree_rc(cCHD *chd, symref_t *p, QTreeWidgetItem *prnt, int dpt)
{
    if (!p || !instPtr)
        return (false);

    /*
    GtkTreeStore *store =
        GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(t_tree)));
    GtkTreeIter iter;
    gtk_tree_store_append(store, &iter, prnt);
    gtk_tree_store_set(store, &iter, 0, Tstring(p->get_name()), -1);
    */
    QTreeWidgetItem *item = new QTreeWidgetItem(prnt);
    item->setText(0, Tstring(p->get_name()));
    if (!prnt)
        t_tree->addTopLevelItem(item);

    if (dpt > t_mdepth)
        t_mdepth = dpt;
    if (check_fb())
        return (false);
    if (!instPtr)
        return (false);

    SymTab *xtab = new SymTab(false, false);
    syrlist_t *s0 = 0;
    nametab_t *ntab = chd->nameTab(t_mode);
    crgen_t cgen(ntab, p);
    const cref_o_t *c;
    while ((c = cgen.next()) != 0) {
        symref_t *cp = ntab->find_symref(c->srfptr);
        if (cp && SymTab::get(xtab, (uintptr_t)cp) == ST_NIL) {
            s0 = new syrlist_t(cp, s0);
            xtab->add((uintptr_t)cp, 0, false);
        }
    }
    delete xtab;
    syrlist_t::sort(s0, false);

    for (syrlist_t *sr = s0; sr; sr = sr->next) {
        if (!build_tree_rc(chd, sr->symref, item, dpt+1)) {
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
QTtreeDlg::t_build_proc(void*)
{
    bool ok = false;
    if (instPtr) {
        if (DSP()->MainWdesc()->DbType() == WDcddb) {
            if (!instPtr->t_root_cd)
                // "can't happen"
                Log()->ErrorLog(mh::Processing, "Null cell name encountered.");
            else {
                CDcbin cbin;
                if (CDcdb()->findSymbol(instPtr->t_root_cd, &cbin)) {
                    CDs *sdesc = cbin.celldesc(instPtr->t_mode);
                    if (sdesc) {
                        instPtr->build_tree(sdesc);
                        ok = true;
                    }
                    else
                        Log()->ErrorLogV(mh::Processing,
                            "Null cell pointer encountered in cell %s.",
                            instPtr->t_root_cd);
                }
                else
                    Log()->ErrorLogV(mh::Processing,
                        "Can't find cell %s for tree display.",
                        instPtr->t_root_cd);
            }
        }
        else if (DSP()->MainWdesc()->DbType() == WDchd) {
            const char *dbname = DSP()->MainWdesc()->DbName();
            if (dbname) {
                cCHD *chd = CDchd()->chdRecall(dbname, false);
                if (chd) {
                    symref_t *p = chd->findSymref(instPtr->t_root_db,
                        instPtr->t_mode, true);
                    if (p) {
                        instPtr->build_tree(chd, p);
                        ok = true;
                    }
                    else
                        Log()->ErrorLogV(mh::Processing,
                            "Can't find name %s in database.",
                            instPtr->t_root_db ? instPtr->t_root_db : "<null>");
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
        instPtr->check_sens();
    }
    if (!ok)
        XM()->PopUpTree(0, MODE_OFF, 0, TU_CUR);
    return (false);
}


void
QTtreeDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:tree"))
}


void
QTtreeDlg::dismiss_btn_slot()
{
    XM()->PopUpTree(0, MODE_OFF, 0, TU_CUR);
}


void
QTtreeDlg::user_btn_slot()
{
    QString name = qobject_cast<QPushButton*>(sender())->text();
    if (name == TR_INFO_BTN) {
        if (!t_selection)
            return;
        if (DSP()->MainWdesc()->DbType() == WDchd) {
            cCHD *chd = CDchd()->chdRecall(DSP()->MainWdesc()->DbName(),
                false);
            if (!chd)
                return;
            symref_t *p = chd->findSymref(t_selection, t_mode, true);
            if (!p)
                return;
            stringlist *sl = new stringlist(
                lstring::copy(Tstring(p->get_name())), 0);
            int flgs = FIO_INFO_OFFSET | FIO_INFO_INSTANCES |
                FIO_INFO_BBS | FIO_INFO_FLAGS;
            char *str = chd->prCells(0, t_mode, flgs, sl);
            stringlist::destroy(sl);
            PopUpInfo(MODE_ON, str, STY_FIXED);
            delete [] str;
        }
        else
            XM()->ShowCellInfo(t_selection, true, t_mode);
    }
    else if (name == TR_OPEN_BTN) {
        if (!t_selection)
            return;
        XM()->Load(DSP()->MainWdesc(), t_selection);
    }
    else if (name == TR_PLACE_BTN) {
        if (!t_selection)
            return;
        if (DSP()->MainWdesc()->DbType() == WDcddb) {
            EV()->InitCallback();
            EditIf()->addMaster(t_selection, 0);
        }
    }
    else if (name == TR_UPD_BTN) {
        XM()->PopUpTree(0, MODE_UPD, 0, TU_CUR);
    }
}


void
QTtreeDlg::current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*)
{
}


void
QTtreeDlg::item_activated(QTreeWidgetItem*, int)
{
}


void
QTtreeDlg::item_changed(QTreeWidgetItem*, int)
{
}


void
QTtreeDlg::item_clicked(QTreeWidgetItem*, int)
{
}


void
QTtreeDlg::item_collapsed(QTreeWidgetItem*, int)
{
}


void
QTtreeDlg::item_expanded(QTreeWidgetItem*, int)
{
}


void
QTtreeDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_PROP) {
        QFont *fnt;
        if (FC.getFont(&fnt, fnum))
            t_tree->setFont(*fnt);
//XXX needs redraw        update();
    }
}


#ifdef notdef

// Static function.
//
int
QTtreeDlg::t_select_proc(GtkTreeSelection*, GtkTreeModel *store,
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
QTtreeDlg::t_focus_proc(GtkWidget*, GdkEvent*, void*)
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
QTtreeDlg::t_collapse_proc(GtkTreeView *tv, GtkTreeIter*, GtkTreePath *path, void*)
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
//
void
QTtreeDlg::t_action(GtkWidget *widget, void*)
{
    if (!Tree)
        return;
    const char *wname = gtk_widget_get_name(widget);
    if (!wname)
        return;
    if (!strcmp("Help", wname)) {
        return;
    }
}


// Static function.
//
int
QTtreeDlg::t_btn_hdlr(GtkWidget*, GdkEvent *event, void*)
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
QTtreeDlg::t_btn_release_hdlr(GtkWidget*, GdkEvent*, void*)
{
    if (Tree)
        Tree->t_dragging = false;
    return (false);
}


// Static function.
// Motion handler, begin drag.
//
int
QTtreeDlg::t_motion_hdlr(GtkWidget *caller, GdkEvent *event, void*)
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
QTtreeDlg::t_drag_data_get(GtkWidget*, GdkDragContext*,
    GtkSelectionData *data, guint, guint, void*)
{
    if (!Tree || !Tree->t_curnode || !Tree->t_selection)
        return;
    gtk_selection_data_set(data, gtk_selection_data_get_target(data),
        8, (unsigned char*)Tree->t_selection, strlen(Tree->t_selection)+1);
}


// Static function.
// Selection clear handler.
//
int
QTtreeDlg::t_selection_clear(GtkWidget*, GdkEventSelection*, void*)
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
QTtreeDlg::t_selection_get(GtkWidget*, GtkSelectionData *data,
    guint, guint, void*)
{
    if (gtk_selection_data_get_selection(data) != GDK_SELECTION_PRIMARY)
        return;
    if (!Tree || !Tree->t_curnode || !Tree->t_selection)
        return;
    gtk_selection_data_set(data, gtk_selection_data_get_target(data),
        8, (unsigned char*)Tree->t_selection, strlen(Tree->t_selection)+1);
}

#endif

