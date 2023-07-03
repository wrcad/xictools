
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
#include "qtchdlist.h"
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
#include "qtinterf/qtmcol.h"
#include "qtinterf/qtfont.h"
#include "miscutil/filestat.h"
#include "miscutil/pathlist.h"

#include <QLayout>
#include <QPushButton>
#include <QLabel>
#include <QTreeWidget>
#include <QHeaderView>
#include <QCheckBox>
#include <QComboBox>


//----------------------------------------------------------------------
//  Cell Hierarchy Digests Listing
//
// Help system keywords used:
//  xic:hier

void
cConvert::PopUpHierarchies(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTchdListDlg::self())
            QTchdListDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTchdListDlg::self())
            QTchdListDlg::self()->update();
        return;
    }
    if (QTchdListDlg::self())
        return;

    new QTchdListDlg(caller);

    QTdev::self()->SetPopupLocation(GRloc(LW_UL), QTchdListDlg::self(),
        QTmainwin::self()->Viewport());
    QTchdListDlg::self()->show();
}
// End of cConvert functions.


// Contexts pop-up buttons.
#define INFO_BTN    "Info"
#define OPEN_BTN    "Open"
#define PLACE_BTN   "Place"

QTchdListDlg *QTchdListDlg::instPtr;

QTchdListDlg::QTchdListDlg(GRobject c)
{
    instPtr = this;
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

    setWindowTitle(tr("Cell Hierarchy Digests"));
    setAttribute(Qt::WA_DeleteOnClose);
//    gtk_window_set_default_size(QT_WINDOW(wb_shell), 350, 150);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // upper buttons
    //
    chl_addbtn = new QPushButton(tr("Add"));
    chl_addbtn->setCheckable(true);
    hbox->addWidget(chl_addbtn);
    connect(chl_addbtn, SIGNAL(toggled(bool)),
        this, SLOT(add_btn_slot(bool)));

    chl_savbtn = new QPushButton(tr("Save"));
    chl_savbtn->setCheckable(true);
    hbox->addWidget(chl_savbtn);
    connect(chl_savbtn, SIGNAL(toggled(bool)),
        this, SLOT(sav_btn_slot(bool)));

    chl_delbtn = new QPushButton(tr("Delete"));
    chl_delbtn->setCheckable(true);
    hbox->addWidget(chl_delbtn);
    connect(chl_delbtn, SIGNAL(toggled(bool)),
        this, SLOT(del_btn_slot(bool)));

    chl_cfgbtn = new QPushButton(tr("Config"));
    chl_cfgbtn->setCheckable(true);
    hbox->addWidget(chl_cfgbtn);
    connect(chl_cfgbtn, SIGNAL(toggled(bool)),
        this, SLOT(cfg_btn_slot(bool)));

    chl_dspbtn = new QPushButton(tr("Display"));
    chl_dspbtn->setCheckable(true);
    hbox->addWidget(chl_dspbtn);
    connect(chl_dspbtn, SIGNAL(toggled(bool)),
        this, SLOT(dsp_btn_slot(bool)));

    chl_cntbtn = new QPushButton(tr("Contents"));
    hbox->addWidget(chl_cntbtn);
    connect(chl_cntbtn, SIGNAL(clicked()),
        this, SLOT(cnt_btn_slot()));

    chl_celbtn = new QPushButton(tr("Cell"));
    chl_celbtn->setCheckable(true);
    hbox->addWidget(chl_celbtn);
    connect(chl_celbtn, SIGNAL(toggled(bool)),
        this, SLOT(cel_btn_slot(bool)));

    chl_infbtn = new QPushButton(tr("Info"));
    hbox->addWidget(chl_infbtn);
    connect(chl_infbtn, SIGNAL(clicked()), this, SLOT(inf_btn_slot()));

    chl_qinfbtn = new QPushButton("?");
    hbox->addWidget(chl_qinfbtn);
    connect(chl_qinfbtn, SIGNAL(clicked()), this, SLOT(qinf_btn_slot()));

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    if (DSP()->MainWdesc()->DbType() == WDchd)
        QTdev::SetStatus(chl_dspbtn, true);

    // scrolled list
    //
    chl_list = new QTreeWidget();
    vbox->addWidget(chl_list);
    chl_list->setHeaderLabels(QStringList(QList<QString>() << tr("Db Name") <<
        tr("Linked CGD") <<
        tr("Source, Default Cell - click to select")));
    chl_list->header()->setMinimumSectionSize(25);
    chl_list->header()->resizeSection(0, 100);
    chl_list->header()->resizeSection(1, 100);

    connect(chl_list,
        SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
        this,
        SLOT(current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(chl_list, SIGNAL(itemActivated(QTreeWidgetItem*, int)),
        this, SLOT(item_activated_slot(QTreeWidgetItem*, int)));
    connect(chl_list, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
        this, SLOT(item_clicked_slot(QTreeWidgetItem*, int)));
    connect(chl_list, SIGNAL(itemSelectionChanged()),
        this, SLOT(item_selection_changed()));
//    g_signal_connect(G_OBJECT(chl_list), "button-press-event",
//        G_CALLBACK(lb_button_press_proc), this);

    // Set up font and tracking.
//    QTfont::setupFont(lb_list, FNT_PROP, true);

    // lower buttons
    //
    chl_rename = new QCheckBox(tr(
        "Use auto-rename when writing CHD reference cells"));
    vbox->addWidget(chl_rename);
    connect(chl_rename, SIGNAL(stateChanged(int)),
        this, SLOT(rename_btn_slot(int)));

    hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QVBoxLayout *col1 = new QVBoxLayout();
    col1->setMargin(0);
    col1->setSpacing(2);
    hbox->addLayout(col1);

    QVBoxLayout *col2 = new QVBoxLayout();
    col2->setMargin(0);
    col2->setSpacing(2);
    hbox->addLayout(col2);

    chl_loadtop = new QCheckBox(tr("Load top cell only"));
    col1->addWidget(chl_loadtop);
    connect(chl_loadtop, SIGNAL(stateChanged(int)),
        this, SLOT(loadtop_btn_slot(int)));

    chl_failres = new QCheckBox(tr("Fail on unresolved"));
    col2->addWidget(chl_failres);
    connect(chl_failres, SIGNAL(stateChanged(int)),
        this, SLOT(failres_btn_slot(int)));

    chl_usetab = new QCheckBox(tr("Use cell table"));
    col1->addWidget(chl_usetab);
    connect(chl_usetab, SIGNAL(stateChanged(int)),
        this, SLOT(usetab_btn_slot(int)));

    chl_showtab = new QPushButton(tr("Edit Cell Table"));
    chl_showtab->setCheckable(true);
    col2->addWidget(chl_showtab);
    connect(chl_showtab, SIGNAL(toggled(bool)),
        this, SLOT(showtab_btn_slot(bool)));

    // Geom handling menu
    //
    QLabel *label = new QLabel(tr("Default geometry handling"));
    col1->addWidget(label);

    chl_geomenu = new QComboBox();
    col2->addWidget(chl_geomenu);

    chl_geomenu->addItem(tr("Create new MEMORY CGD"));
    chl_geomenu->addItem(tr("Create new FILE CHD"));
    chl_geomenu->addItem(tr("Ignore geometry records"));
    connect(chl_geomenu, SIGNAL(currentIndexChanged(int)),
        this, SLOT(geom_change_slot(int)));

    // dismiss button
    //
    btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTchdListDlg::~QTchdListDlg()
{
    instPtr = 0;
    delete [] chl_selection;
    delete [] chl_contlib;

    if (chl_caller)
        QTdev::Deselect(chl_caller);
    Cvt()->PopUpChdOpen(0, MODE_OFF, 0, 0, 0, 0, 0, 0);
    Cvt()->PopUpChdSave(0, MODE_OFF, 0, 0, 0, 0, 0);
    Cvt()->PopUpChdConfig(0, MODE_OFF, 0, 0, 0);
    if (QTdev::GetStatus(chl_showtab))
        Cvt()->PopUpAuxTab(0, MODE_OFF);
    if (chl_cnt_pop)
        chl_cnt_pop->popdown();
    if (chl_del_pop)
        chl_del_pop->popdown();
    Cvt()->PopUpDisplayWindow(0, MODE_OFF, 0, 0, 0);
}


// Update the listing.
//
void
QTchdListDlg::update()
{
    QTdev::SetStatus(chl_loadtop, CDvdb()->getVariable(VA_ChdLoadTopOnly));
    QTdev::SetStatus(chl_rename, CDvdb()->getVariable(VA_RefCellAutoRename));
    QTdev::SetStatus(chl_usetab, CDvdb()->getVariable(VA_UseCellTab));
    QTdev::SetStatus(chl_failres, CDvdb()->getVariable(VA_ChdFailOnUnresolved));

    // Since the enum is defined elsewhere, don't assume that the values
    // are the same as the menu order.
    switch (sCHDin::get_default_cgd_type()) {
    case CHD_CGDmemory:
        chl_geomenu->setCurrentIndex(0);
        break;
    case CHD_CGDfile:
        chl_geomenu->setCurrentIndex(1);
        break;
    case CHD_CGDnone:
        chl_geomenu->setCurrentIndex(2);
        break;
    }

    if (chl_selection && !CDchd()->chdRecall(chl_selection, false)) {
        delete [] chl_selection;
        chl_selection = 0;
    }

    if (!chl_selection) {
        if (DSP()->MainWdesc()->DbType() != WDchd)
            chl_dspbtn->setEnabled(false);
        chl_savbtn->setEnabled(false);
        chl_delbtn->setEnabled(false);
        chl_cfgbtn->setEnabled(false);
        chl_cntbtn->setEnabled(false);
        chl_celbtn->setEnabled(false);
        chl_infbtn->setEnabled(false);
        chl_qinfbtn->setEnabled(false);
        if (chl_del_pop)
            chl_del_pop->popdown();
        Cvt()->PopUpChdSave(0, MODE_OFF, 0, 0, 0, 0, 0);
    }

    stringlist *names = CDchd()->chdList();
    stringlist::sort(names);

    chl_list->clear();
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
                int s2len = strlen(fn) + strlen(cn) + 10;
                strings[2] = new char[s2len];
                snprintf(strings[2], s2len, "%s  %s", fn, cn);

                QTreeWidgetItem *item = new QTreeWidgetItem(
                    QStringList(QList<QString>() << strings[0] <<
                    strings[1] << strings[2]));
                chl_list->addTopLevelItem(item);

                delete [] strings[1];
                delete [] strings[2];
             /*
                if (chl_selection && rowsel < 0 &&
                        !strcmp(chl_selection, l->string))
                    rowsel = rowcnt;
                rowcnt++;
              */
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
    /*
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(chl_list));
    if (rowsel >= 0) {
        GtkTreePath *p = gtk_tree_path_new_from_indices(rowsel, -1);
        GtkTreeSelection *sel =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(chl_list));
        gtk_tree_selection_select_path(sel, p);
        gtk_tree_path_free(p);
    }
    */

    stringlist::destroy(names);
    recolor();
}


// Color the background of entry being displayed.
//
void
QTchdListDlg::recolor()
{
    const char *sclr = QTpkg::self()->GetAttrColor(GRattrColorLocSel);
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        /*
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
        */
    }
    else {
        /*
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
        */
    }
    if (chl_cnt_pop) {
        if (DSP()->MainWdesc()->DbType() == WDchd)
            chl_cnt_pop->set_button_sens(0x1);
        else
            chl_cnt_pop->set_button_sens(0xff);
    }
}


void
QTchdListDlg::err_message(const char *fmt)
{
    const char *s = Errs()->get_error();
    int len = strlen(fmt) + (s ? strlen(s) : 0) + 10;
    char *t = new char[len];
    snprintf(t, len, fmt, s);
    PopUpMessage(t, true);
    delete [] t;
}


// Static function.
// Callback for the Add dialog.
//
bool
QTchdListDlg::chl_add_cb(const char *idname, const char *fname, int mode, void*)
{
    char buf[256];
    char *realname;
    FILE *fp = FIO()->POpen(fname, "r", &realname);
    if (!fp) {
        Errs()->sys_error("open");
        instPtr->err_message("Open failed:\n%s");
        return (false);
    }
    if (idname && *idname && CDchd()->chdRecall(idname, false)) {
        snprintf(buf, sizeof(buf), "Access name %s is already in use.",
            idname);
        instPtr->PopUpMessage(buf, true);
        return (false);
    }

    cCHD *chd = 0;
    FileType ft = FIO()->GetFileType(fp);
    fclose(fp);
    if (!FIO()->IsSupportedArchiveFormat(ft)) {

        // The file might be a saved digest file, if so, read it.
        sCHDin chd_in;
        if (!chd_in.check(realname)) {
            instPtr->PopUpMessage("Not a supported file format.", true);
            delete [] realname;
            return (false);
        }
        QTpkg::self()->SetWorking(true);
        ChdCgdType tp = CHD_CGDmemory;
        if (mode == 1)
            tp = CHD_CGDfile;
        else if (mode == 2)
            tp = CHD_CGDnone;
        chd = chd_in.read(realname, tp);
        QTpkg::self()->SetWorking(false);
        if (!chd) {
            instPtr->err_message("Read digest file failed:\n%s");
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
            instPtr->err_message("Read failed:\n%s");
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
        snprintf(buf, sizeof(buf),
            "Access name %s is in use, using new name %s.", idname, n);
        instPtr->PopUpMessage(buf, false);
        CDchd()->chdStore(n, chd);
        delete [] n;
    }
    delete [] idname;

    instPtr->update();
    return (true);
}


// Static function.
// Callback for the Save dialog.
//
bool
QTchdListDlg::chl_sav_cb(const char *fname, bool with_geom, void*)
{
    if (!instPtr->chl_selection) {
        instPtr->err_message("No selection, select a CHD in the listing.");
        return (false);
    }
    cCHD *chd = CDchd()->chdRecall(instPtr->chl_selection, false);
    if (!chd) {
        Errs()->add_error("unresolved CHD name %s.", instPtr->chl_selection);
        instPtr->err_message("Error occurred:\n%s");
        return (false);
    }

    QTpkg::self()->SetWorking(true);
    sCHDout chd_out(chd);
    bool ok = chd_out.write(fname, with_geom ? CHD_WITH_GEOM : 0);
    QTpkg::self()->SetWorking(false);
    if (!ok) {
        instPtr->err_message("Error occurred when writing digest file:\n%s");
        return (false);
    }
    instPtr->PopUpMessage("Digest file saved successfully.", false);
    return (true);
}


// Static function.
// Callback for confirmation pop-up.
//
void
QTchdListDlg::chl_del_cb(bool yn, void *arg)
{
    if (arg && yn && instPtr) {
        char *dbname = (char*)arg;
        cCHD *chd = CDchd()->chdRecall(dbname, true);
        delete chd;
        if (DSP()->MainWdesc()->DbType() == WDchd &&
                !strcmp(dbname, DSP()->MainWdesc()->DbName())) {
            QTdev::Deselect(instPtr->chl_dspbtn);
            XM()->SetHierDisplayMode(0, 0, 0);
        }
        instPtr->update();
        delete [] dbname;
    }
}


// Static function.
// Callback for the display window pop-up.
//
bool
QTchdListDlg::chl_display_cb(bool working, const BBox *BB, void*)
{
    if (!instPtr)
        return (true);
    if (working) {
        if (!instPtr->chl_selection)
            return (true);
        cCHD *chd = CDchd()->chdRecall(instPtr->chl_selection, false);
        if (!chd)
            return (true);

        XM()->SetHierDisplayMode(instPtr->chl_selection,
            chd->defaultCell(Physical), BB);
        return (true);
    }

    // Strangeness here in GTK-2 for Red Hat 5.  Unless Deselect is
    // called before set_sensitive, the Display button will appear as
    // selected, but will revert to proper colors if, e.g., the mouse
    // cursor crosses the button area.

    QTdev::Deselect(instPtr->chl_dspbtn);
    instPtr->setEnabled(true);
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        instPtr->recolor();
        QTdev::Select(instPtr->chl_dspbtn);
    }
    return (true);
}


// Static function.
void
QTchdListDlg::chl_cnt_cb(const char *cellname, void*)
{
    if (!instPtr)
        return;
    if (!instPtr->chl_contlib || !instPtr->chl_cnt_pop)
        return;
    if (!cellname)
        return;
    if (*cellname != '/')
        return;

    // Callback from button press.
    cellname++;
    if (!strcmp(cellname, INFO_BTN)) {
        cCHD *chd = CDchd()->chdRecall(instPtr->chl_contlib, false);
        if (!chd)
            return;
        char *listsel = instPtr->chl_cnt_pop->get_selection();
        symref_t *p = chd->findSymref(listsel, DSP()->CurMode(), true);
        delete [] listsel;
        if (!p)
            return;
        QTpkg::self()->SetWorking(true);
        stringlist *sl = new stringlist(
            lstring::copy(Tstring(p->get_name())), 0);
        int flgs = FIO_INFO_OFFSET | FIO_INFO_INSTANCES |
            FIO_INFO_BBS | FIO_INFO_FLAGS;
        char *str = chd->prCells(0, DSP()->CurMode(), flgs, sl);
        stringlist::destroy(sl);
        instPtr->PopUpInfo(MODE_ON, str, STY_FIXED);
        QTpkg::self()->SetWorking(false);
    }
    else if (!strcmp(cellname, OPEN_BTN)) {
        char *sel = instPtr->chl_cnt_pop->get_selection();
        if (sel) {
            EV()->InitCallback();
            XM()->EditCell(instPtr->chl_contlib, false, FIO()->DefReadPrms(), sel);
            delete [] sel;
        }
    }
    else if (!strcmp(cellname, PLACE_BTN)) {
        char *sel = instPtr->chl_cnt_pop->get_selection();
        if (sel) {
            EV()->InitCallback();
            EditIf()->addMaster(instPtr->chl_contlib, sel);
            delete [] sel;
        }
    }
}


// Static function.
// Callback for the Cell dialog.
//
ESret
QTchdListDlg::chl_cel_cb(const char *cname, void*)
{
    if (!instPtr->chl_selection)
        return (ESTR_DN);
    if (cname) {
        cCHD *chd = CDchd()->chdRecall(instPtr->chl_selection, false);
        if (!chd)
            return (ESTR_DN);
        if (!chd->findSymref(cname, Physical)) {
            instPtr->chl_cel_pop->update(
                "Cell name not found in CHD, try again: ", 0);
            return (ESTR_IGN);
        }
        if (!chd->createReferenceCell(cname)) {
            instPtr->chl_cel_pop->update(
                "Error creating cell, try again:", 0);
            instPtr->err_message("Error creating cell:\n%s");
            return (ESTR_IGN);
        }
        char buf[256];
        snprintf(buf, sizeof(buf),
            "Reference cell named %s created in memory.", cname);
        instPtr->PopUpMessage(buf, false);
    }
    return (ESTR_DN);
}


void
QTchdListDlg::add_btn_slot(bool state)
{
    if (state) {
        QPoint pg = mapToGlobal(QPoint(0, 0));
        char *cn = CDchd()->newChdName();
        Cvt()->PopUpChdOpen(chl_addbtn, MODE_ON, cn, 0,
            pg.x() + 20, pg.y() + 100, chl_add_cb, 0);
        delete [] cn;
    }
    else
        Cvt()->PopUpChdOpen(0, MODE_OFF, 0, 0, 0, 0, 0, 0);
}


void
QTchdListDlg::sav_btn_slot(bool state)
{
    if (state) {
        if (chl_del_pop)
            chl_del_pop->popdown();
        QTdev::Deselect(chl_delbtn);
        if (chl_selection) {
            QPoint pg = mapToGlobal(QPoint(0, 0));
            Cvt()->PopUpChdSave(chl_savbtn, MODE_ON, chl_selection,
                pg.x() + 20, pg.y() + 100, chl_sav_cb, 0);
        }
        else
            QTdev::Deselect(chl_savbtn);
    }
    else
        Cvt()->PopUpChdSave(0, MODE_OFF, 0, 0, 0, 0, 0);
}


void
QTchdListDlg::del_btn_slot(bool state)
{
    if (chl_del_pop)
        chl_del_pop->popdown();
    if (state) {
        Cvt()->PopUpChdSave(0, MODE_OFF, 0, 0, 0, 0, 0);
        QTdev::Deselect(chl_savbtn);
        if (chl_selection) {
            chl_del_pop = PopUpAffirm(chl_delbtn, GRloc(),
                "Confirm - delete selected digest?", chl_del_cb,
                lstring::copy(chl_selection));
            if (chl_del_pop)
                chl_del_pop->register_usrptr((void**)&chl_del_pop);
        }
        else
            QTdev::Deselect(chl_delbtn);
    }
}


void
QTchdListDlg::cfg_btn_slot(bool state)
{
    if (state) {
        QPoint pg = mapToGlobal(QPoint(0, 0));
        Cvt()->PopUpChdConfig(chl_cfgbtn, MODE_ON, chl_selection,
            pg.x() + 20, pg.y() + 100);
    }
    else
        Cvt()->PopUpChdConfig(0, MODE_OFF, 0, 0, 0);
}


void
QTchdListDlg::dsp_btn_slot(bool state)
{
    if (state) {
        if (!chl_selection) {
            QTdev::Deselect(chl_dspbtn);
            return;
        }
        cCHD *chd = CDchd()->chdRecall(chl_selection, false);
        if (!chd) {
            QTdev::Deselect(chl_dspbtn);
            return;
        }
        symref_t *p = chd->findSymref(0, Physical, true);
        if (!p) {
            PopUpMessage("CHD has no default cell!", true);
            return;
        }
        if (!chd->setBoundaries(p)) {
            err_message("Bounding box computation failed:\n%s");
            return;
        }
        BBox BB = *p->get_bb();
        BB.scale(1.1);
        Cvt()->PopUpDisplayWindow(0, MODE_ON, &BB, chl_display_cb, 0);
        setEnabled(false);
    }
    else {
        XM()->SetHierDisplayMode(0, 0, 0);
        if (!chl_selection)
            chl_dspbtn->setEnabled(false);
        Cvt()->PopUpDisplayWindow(0, MODE_OFF, 0, 0, 0);
        recolor();
    }
}


void
QTchdListDlg::cnt_btn_slot()
{
    if (!chl_selection)
        return;
    delete [] chl_contlib;
    chl_contlib = 0;

    cCHD *chd = CDchd()->chdRecall(chl_selection, false);
    if (!chd) {
        err_message("Content scan failed:\n%s");
        return;
    }

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


void
QTchdListDlg::cel_btn_slot(bool state)
{
    if (chl_cel_pop)
        chl_cel_pop->popdown();
    if (state) {
        if (!chl_selection) {
            QTdev::Deselect(chl_celbtn);
            return;
        }
        const char *cname = 0;
        cCHD *chd = CDchd()->chdRecall(chl_selection, false);
        if (chd)
            cname = chd->defaultCell(Physical);
        else {
            QTdev::Deselect(chl_celbtn);
            return;
        }
        chl_cel_pop = PopUpEditString(chl_celbtn, GRloc(),
            "Reference cell name:", cname, chl_cel_cb, 0, 250, 0,
            false, 0);
        if (chl_cel_pop)
            chl_cel_pop->register_usrptr((void**)&chl_cel_pop);
    }
}


void
QTchdListDlg::inf_btn_slot()
{
    if (!chl_selection)
        return;
    cCHD *chd = CDchd()->chdRecall(chl_selection, false);
    if (!chd)
        return;

    QTpkg::self()->SetWorking(true);
    char *string = chd->prInfo(0, DSP()->CurMode(), 0);

    char buf[256];
    snprintf(buf, sizeof(buf), "CHD name: %s\n", chl_selection);
    char *tmp = new char[strlen(buf) + strlen(string) + 1];
    char *e = lstring::stpcpy(tmp, buf);
    strcpy(e, string);
    delete [] string;
    string = tmp;

    PopUpInfo(MODE_ON, string, STY_FIXED);
    delete [] string;
    QTpkg::self()->SetWorking(false);
}


void
QTchdListDlg::qinf_btn_slot()
{
    if (!chl_selection)
        return;
    cCHD *chd = CDchd()->chdRecall(chl_selection, false);
    if (!chd)
        return;

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


void
QTchdListDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:hier"))
}


void
QTchdListDlg::current_item_changed_slot(QTreeWidgetItem *cur, QTreeWidgetItem*)
{
    if (!cur) {
        delete [] chl_selection;
        chl_selection = 0;
        return;
    }
    char *text = lstring::copy(cur->text(0).toLatin1().constData());
    if  (text) {
        delete [] chl_selection;
        chl_selection = text;
        chl_savbtn->setEnabled(true);
        chl_delbtn->setEnabled(true);
        chl_cfgbtn->setEnabled(true);
        if (DSP()->CurMode() == Physical)
            chl_dspbtn->setEnabled(true);
        chl_cntbtn->setEnabled(true);
        chl_celbtn->setEnabled(true);
        chl_infbtn->setEnabled(true);
        chl_qinfbtn->setEnabled(true);

        Cvt()->PopUpChdConfig(0, MODE_UPD, text, 0, 0);
        Cvt()->PopUpChdSave(0, MODE_UPD, text, 0, 0, 0, 0);
    }
}


void
QTchdListDlg::item_activated_slot(QTreeWidgetItem*, int)
{
}


void
QTchdListDlg::item_clicked_slot(QTreeWidgetItem*, int)
{
}


void
QTchdListDlg::item_selection_changed()
{
}


void
QTchdListDlg::rename_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_RefCellAutoRename, "");
    else
        CDvdb()->clearVariable(VA_RefCellAutoRename);
}


void
QTchdListDlg::loadtop_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_ChdLoadTopOnly, "");
    else
        CDvdb()->clearVariable(VA_ChdLoadTopOnly);
}


void
QTchdListDlg::failres_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_ChdFailOnUnresolved, "");
    else
        CDvdb()->clearVariable(VA_ChdFailOnUnresolved);
}


void
QTchdListDlg::usetab_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_UseCellTab, "");
    else
        CDvdb()->clearVariable(VA_UseCellTab);
}


void
QTchdListDlg::showtab_btn_slot(bool state)
{
    if (state) {
        Cvt()->PopUpAuxTab(0, MODE_OFF);
        Cvt()->PopUpAuxTab(chl_showtab, MODE_ON);
    }
    else
        Cvt()->PopUpAuxTab(0, MODE_OFF);
}


void
QTchdListDlg::geom_change_slot(int index)
{
    if (index == 0)
        sCHDin::set_default_cgd_type(CHD_CGDmemory);
    else if (index == 1)
        sCHDin::set_default_cgd_type(CHD_CGDfile);
    else 
        sCHDin::set_default_cgd_type(CHD_CGDnone);
}


void
QTchdListDlg::dismiss_btn_slot()
{
    Cvt()->PopUpHierarchies(0, MODE_OFF);
}

