
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

#include "qtextdev.h"
#include "ext.h"
#include "ext_extract.h"
#include "dsp_inlines.h"
#include "promptline.h"
#include "qtinterf/qtfont.h"

#include <QLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTreeWidget>
#include <QHeaderView>
#include <QGroupBox>
#include <QCheckBox>


//-------------------------------------------------------------------------
// Dialog to control device highlighting and selection
//
// Help system keywords used:
//  xic:dvsel

void
cExt::PopUpDevices(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTextDevDlg::self())
            QTextDevDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTextDevDlg::self())
            QTextDevDlg::self()->update();
        return;
    }
    if (QTextDevDlg::self())
        return;

    new QTextDevDlg(caller);

    QTextDevDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_UR), QTextDevDlg::self(),
        QTmainwin::self()->Viewport());
    QTextDevDlg::self()->show();
}
// End of cExt functions.


const char *QTextDevDlg::nodevmsg = "No devices found.";
QTextDevDlg *QTextDevDlg::instPtr;

QTextDevDlg::QTextDevDlg(GRobject caller)
{
    instPtr = this;
    ed_caller = caller;
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

    setWindowTitle(tr("Show/Select Devices"));
    setAttribute(Qt::WA_DeleteOnClose);
//    gtk_window_set_resizable(GTK_WINDOW(ed_popup), false);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QGridLayout *grid = new QGridLayout(this);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    ed_update = new QPushButton(tr("Update\nList"));
    grid->addWidget(ed_update, 0, 0, 2, 1);
    connect(ed_update, SIGNAL(clicked()), this, SLOT(update_btn_slot()));

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    grid->addLayout(hbox, 0, 1);

    ed_show_all = new QPushButton(tr("Show All"));
    hbox->addWidget(ed_show_all);
    connect(ed_show_all, SIGNAL(clicked()), this, SLOT(showall_btn_slot()));

    ed_erase_all = new QPushButton(tr("Erase All"));
    hbox->addWidget(ed_erase_all);
    connect(ed_erase_all, SIGNAL(clicked()), this, SLOT(eraseall_btn_slot()));

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    grid->addLayout(hbox, 1, 1);

    ed_show = new QPushButton(tr("Show"));
    hbox->addWidget(ed_show);
    connect(ed_show, SIGNAL(clicked()), this, SLOT(show_btn_slot()));

    ed_erase = new QPushButton(tr("Erase"));
    hbox->addWidget(ed_erase);
    connect(ed_erase, SIGNAL(clicked()), this, SLOT(erase_btn_slot()));

    QLabel *label = new QLabel(tr("Indices"));
    hbox->addWidget(label);

    ed_indices = new QLineEdit();
    hbox->addWidget(ed_indices);

    // scrolled list
    //
    ed_list = new QTreeWidget();
    grid->addWidget(ed_list, 2, 0, 1, 2);
    ed_list->setHeaderLabels(QStringList(QList<QString>() << tr("Name") <<
        tr("Prefix") << tr("Index Range")));
    ed_list->header()->setMinimumSectionSize(25);
    ed_list->header()->resizeSection(0, 50);

    connect(ed_list,
        SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
        this,
        SLOT(current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(ed_list, SIGNAL(itemActivated(QTreeWidgetItem*, int)),
        this, SLOT(item_activated_slot(QTreeWidgetItem*, int)));
    connect(ed_list, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
        this, SLOT(item_clicked_slot(QTreeWidgetItem*, int)));
    connect(ed_list, SIGNAL(itemSelectionChanged()),
        this, SLOT(item_selection_changed_slot()));

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_FIXED))
        ed_list->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    // Frame and select devices group.
    //
    QGroupBox *gb = new QGroupBox();
    grid->addWidget(gb, 3, 0, 1, 2);
    QGridLayout *gr = new QGridLayout(gb);

    ed_select = new QPushButton(tr("Enable\nSelect"));
    ed_select->setCheckable(true);
    gr->addWidget(ed_select, 0, 0, 2, 1);
    connect(ed_select, SIGNAL(toggled(bool)),
        this, SLOT(enablesel_btn_slot(bool)));

    ed_compute = new QCheckBox(tr(
        "Show computed parameters of selected device"));
    gr->addWidget(ed_compute, 0, 1);
    connect(ed_compute, SIGNAL(stateChanged(int)),
        this, SLOT(compute_btn_slot(int)));

    ed_compare = new QCheckBox(tr(
        "Show elec/phys comparison of selected device"));
    gr->addWidget(ed_compare, 1, 1);
    connect(ed_compare, SIGNAL(stateChanged(int)),
        this, SLOT(compare_btn_slot(int)));

    // Frame and measure box group.
    //
    gb = new QGroupBox();
    grid->addWidget(gb, 4, 0, 1, 2);
    hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qmtop);
    hbox->setSpacing(2);

    ed_measbox = new QPushButton(tr("Enable Measure Box"));
    ed_measbox->setCheckable(true);
    hbox->addWidget(ed_measbox);
    connect(ed_measbox, SIGNAL(toggled(bool)),
        this, SLOT(measbox_btn_slot(bool)));

    ed_paint = new QPushButton(tr("Paint Box (use current layer)"));
    hbox->addWidget(ed_paint);
    connect(ed_paint, SIGNAL(clicked()), this, SLOT(paint_btn_slot()));

    // Dismiss button.
    //
    btn = new QPushButton(tr("Dismiss"));
    grid->addWidget(btn, 5, 0, 1, 2);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTextDevDlg::~QTextDevDlg()
{
    // Must do this before zeroing pointer!
    if (QTdev::GetStatus(ed_measbox))  
        QTdev::CallCallback(ed_measbox);

    instPtr = 0;
    if (ed_caller)
        QTdev::Deselect(ed_caller);

    CDs *cursdp = CurCell(Physical);
    if (cursdp) {
        cGroupDesc *gd = cursdp->groups();
        if (gd)
            gd->parse_find_dev(0, false);
    }

    if (ed_select && QTdev::GetStatus(ed_select)) {
        QTdev::SetStatus(ed_select, false);
        EX()->selectDevices(ed_select);
    }
}


// Call when 1) current cell changes, 2) selection mode termination.
//
void
QTextDevDlg::update()
{

    if (!QTdev::GetStatus(ed_select)) {
        ed_compute->setEnabled(false);
        ed_compare->setEnabled(false);
    }

    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        PL()->ShowPrompt("No current physical cell!");
        return;
    }
    if (!cursdp || cursdp != ed_sdesc || !cursdp->isAssociated()) {
        ed_list->clear();
        ed_devs_listed = false;
    }

    if (!ed_devs_listed) {
        ed_show->setEnabled(false);
        ed_erase->setEnabled(false);
        ed_show_all->setEnabled(false);
        ed_erase_all->setEnabled(false);
    }
    if (!QTdev::GetStatus(ed_measbox))
        ed_paint->setEnabled(false);
    if (DSP()->CurMode() == Electrical)
        ed_measbox->setEnabled(false);
    else
        ed_measbox->setEnabled(true);
}


void
QTextDevDlg::relist()
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
        ed_show_all->setEnabled(false);
        ed_erase_all->setEnabled(false);
        ed_devs_listed = false;
    }
    else {
        ed_show_all->setEnabled(true);
        ed_erase_all->setEnabled(true);
        ed_devs_listed = true;
    }
/*XXX
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
*/
    stringlist::destroy(devlist);
    delete [] ed_selection;
    ed_selection = 0;

    ed_show->setEnabled(false);
    ed_erase->setEnabled(false);
}


void
QTextDevDlg::update_btn_slot()
{
    relist();
}


void
QTextDevDlg::showall_btn_slot()
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    cGroupDesc *gd = cursdp->groups();
    if (!gd)
        return;
    if (gd)
        gd->parse_find_dev(0, true);
}


void
QTextDevDlg::eraseall_btn_slot()
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    cGroupDesc *gd = cursdp->groups();
    if (!gd)
        return;
    if (gd)
        gd->parse_find_dev(0, false);
}


void
QTextDevDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:dvsel"))
}


void
QTextDevDlg::show_btn_slot()
{
    if (!ed_selection)
        return;
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    cGroupDesc *gd = cursdp->groups();
    if (!gd)
        return;

    char *s = ed_selection;
    char *dname = lstring::gettok(&s);
    char *pref = lstring::gettok(&s);
    QByteArray ind_ba = ed_indices->text().toLatin1();
    const char *ind = ind_ba.constData();
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


void
QTextDevDlg::erase_btn_slot()
{
    if (!ed_selection)
        return;
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    cGroupDesc *gd = cursdp->groups();
    if (!gd)
        return;

    char *s = ed_selection;
    char *dname = lstring::gettok(&s);
    char *pref = lstring::gettok(&s);
    QByteArray ind_ba = ed_indices->text().toLatin1();
    const char *ind = ind_ba.constData();
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


void
QTextDevDlg::current_item_changed_slot(QTreeWidgetItem*, QTreeWidgetItem*)
{
}


void
QTextDevDlg::item_activated_slot(QTreeWidgetItem*, int)
{
}


void
QTextDevDlg::item_clicked_slot(QTreeWidgetItem*, int)
{
}


void
QTextDevDlg::item_selection_changed_slot()
{
}


void
QTextDevDlg::enablesel_btn_slot(bool state)
{
    if (state) {
        if (!EX()->selectDevices(ed_select)) {
            QTdev::Deselect(ed_select);
            return;
        }
        ed_compute->setEnabled(true);
        ed_compare->setEnabled(true);
    }
    else {
        EX()->selectDevices(ed_select);
        QTdev::SetStatus(ed_compute, false);
        QTdev::SetStatus(ed_compare, false);
        ed_compute->setEnabled(false);
        ed_compare->setEnabled(false);
        EX()->setDevselCompute(false);
        EX()->setDevselCompare(false);
    }
}


void
QTextDevDlg::compute_btn_slot(int state)
{
    EX()->setDevselCompute(state);
}


void
QTextDevDlg::compare_btn_slot(int state)
{
    EX()->setDevselCompare(state);
}


void
QTextDevDlg::measbox_btn_slot(bool state)
{
    if (state) {
        if (!EX()->measureLayerElectrical(ed_measbox)) {
            QTdev::Deselect(ed_measbox);
            return;
        }
        ed_paint->setEnabled(true);
    }
    else {
        EX()->measureLayerElectrical(ed_measbox);
        ed_paint->setEnabled(false);
    }
}


void
QTextDevDlg::paint_btn_slot()
{
    EX()->paintMeasureBox();
}


void
QTextDevDlg::dismiss_btn_slot()
{
    EX()->PopUpDevices(0, MODE_OFF);
}


void
QTextDevDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (FC.getFont(&fnt, fnum))
            ed_list->setFont(*fnt);
        update();
    }
}


#ifdef notdef


// Static function.
// Selection callback for the list.  This is called when a new selection
// is made, but not when the selection disappears, which happens when the
// list is updated.
//
int
QTextDevDlg::ed_selection_proc(GtkTreeSelection*, GtkTreeModel *store,
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

#endif
