
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

#include <QApplication>
#include <QLayout>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTreeWidget>
#include <QHeaderView>
#include <QGroupBox>
#include <QCheckBox>


//-----------------------------------------------------------------------------
// QTextDevDlg:  Dialog to control device highlighting and selection.
// Called from the main menu: Extract/Device Selections.
//
// Help system keywords used:
//  xic:dvsel

void
cExt::PopUpDevices(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTextDevDlg::self();
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
    ed_measbox = 0;
    ed_paint = 0;

    ed_selection = 0;
    ed_sdesc = 0;
    ed_devs_listed = false;

    setWindowTitle(tr("Show/Select Devices"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QGridLayout *grid = new QGridLayout(this);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    ed_update = new QToolButton();
    ed_update->setText(tr("Update\nList"));
    grid->addWidget(ed_update, 0, 0, 2, 1);
    connect(ed_update, &QAbstractButton::clicked,
        this, &QTextDevDlg::update_btn_slot);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    grid->addLayout(hbox, 0, 1);

    ed_show_all = new QToolButton();
    ed_show_all->setText(tr("Show All"));
    hbox->addWidget(ed_show_all);
    connect(ed_show_all, &QAbstractButton::clicked,
        this, &QTextDevDlg::showall_btn_slot);

    ed_erase_all = new QToolButton();
    ed_erase_all->setText(tr("Erase All"));
    hbox->addWidget(ed_erase_all);
    connect(ed_erase_all, &QAbstractButton::clicked,
        this, &QTextDevDlg::eraseall_btn_slot);

    hbox->addStretch(1);
    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTextDevDlg::help_btn_slot);

    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    grid->addLayout(hbox, 1, 1);

    ed_show = new QToolButton();
    ed_show->setText(tr("Show"));
    hbox->addWidget(ed_show);
    connect(ed_show, &QAbstractButton::clicked,
        this, &QTextDevDlg::show_btn_slot);

    ed_erase = new QToolButton();
    ed_erase->setText(tr("Erase"));
    hbox->addWidget(ed_erase);
    connect(ed_erase, &QAbstractButton::clicked,
        this, &QTextDevDlg::erase_btn_slot);

    hbox->addSpacing(4);
    QLabel *label = new QLabel(tr("Indices"));
    hbox->addWidget(label);
    hbox->addSpacing(4);

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

    connect(ed_list, &QTreeWidget::currentItemChanged,
        this, &QTextDevDlg::current_item_changed_slot);
    connect(ed_list, &QTreeWidget::itemActivated,
        this, &QTextDevDlg::item_activated_slot);
    connect(ed_list, &QTreeWidget::itemClicked,
        this, &QTextDevDlg::item_clicked_slot);
    connect(ed_list, &QTreeWidget::itemSelectionChanged,
        this, &QTextDevDlg::item_selection_changed_slot);

    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_FIXED))
        ed_list->setFont(*fnt);
    connect(QTfont::self(), &QTfont::fontChanged,
        this, &QTextDevDlg::font_changed_slot, Qt::QueuedConnection);

    // Frame and select devices group.
    //
    QGroupBox *gb = new QGroupBox();
    grid->addWidget(gb, 3, 0, 1, 2);
    QGridLayout *gr = new QGridLayout(gb);

    ed_select = new QToolButton();
    ed_select->setText(tr("Enable\nSelect"));
    gr->addWidget(ed_select, 0, 0, 2, 1);
    ed_select->setCheckable(true);
    connect(ed_select, &QAbstractButton::toggled,
        this, &QTextDevDlg::enablesel_btn_slot);

#if QT_VERSION >= QT_VERSION_CHECK(6,8,0)
#define CHECK_BOX_STATE_CHANGED &QCheckBox::checkStateChanged
#else
#define CHECK_BOX_STATE_CHANGED &QCheckBox::stateChanged
#endif

    ed_compute = new QCheckBox(tr(
        "Show computed parameters of selected device"));
    gr->addWidget(ed_compute, 0, 1);
    connect(ed_compute, CHECK_BOX_STATE_CHANGED,
        this, &QTextDevDlg::compute_btn_slot);

    ed_compare = new QCheckBox(tr(
        "Show elec/phys comparison of selected device"));
    gr->addWidget(ed_compare, 1, 1);
    connect(ed_compare, CHECK_BOX_STATE_CHANGED,
        this, &QTextDevDlg::compare_btn_slot);

    // Frame and measure box group.
    //
    gb = new QGroupBox();
    grid->addWidget(gb, 4, 0, 1, 2);
    hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qmtop);
    hbox->setSpacing(2);

    ed_measbox = new QToolButton();
    ed_measbox->setText(tr("Enable Measure Box"));
    hbox->addWidget(ed_measbox);
    ed_measbox->setCheckable(true);
    connect(ed_measbox, &QAbstractButton::toggled,
        this, &QTextDevDlg::measbox_btn_slot);

    ed_paint = new QToolButton();
    ed_paint->setText(tr("Paint Box (use current layer)"));
    hbox->addWidget(ed_paint);
    connect(ed_paint, &QAbstractButton::clicked,
        this, &QTextDevDlg::paint_btn_slot);

    // Dismiss button.
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    grid->addWidget(btn, 5, 0, 1, 2);
    connect(btn, &QAbstractButton::clicked,
        this, &QTextDevDlg::dismiss_btn_slot);

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


#ifdef Q_OS_MACOS
#define DLGTYPE QTextDevDlg
#include "qtinterf/qtmacos_event.h"
#endif


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
    ed_list->clear();
    for (stringlist *l = devlist; l; l = l->next) {
        QTreeWidgetItem *itm = new QTreeWidgetItem();
        ed_list->addTopLevelItem(itm);

        char *s = l->string;
        char *t = lstring::gettok(&s);
        itm->setText(0, t);
        delete [] t;
        t = lstring::gettok(&s);
        itm->setText(1, t);
        delete [] t;
        t = lstring::gettok(&s);
        itm->setText(2, t);
        delete [] t;
    }
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
QTextDevDlg::current_item_changed_slot(QTreeWidgetItem *itm, QTreeWidgetItem*)
{
    if (!ed_devs_listed)
        return;
    if (!itm)
        return;
    QByteArray ba1 = itm->text(0).toLatin1();
    const char *name = ba1.constData();
    if (!name)
        return;
    QByteArray ba2 = itm->text(1).toLatin1();
    const char *pref = ba2.constData();
    if (!pref)
        return;
    if (!strcmp(name, "no") && !strcmp(pref, "devices")) {
        // First two tokens of nodevmsg.
        ed_show->setEnabled(false);
        ed_erase->setEnabled(false);
        return;
    }
    delete [] ed_selection;
    ed_selection = new char[strlen(name) + strlen(pref) + 2];
    char *t = lstring::stpcpy(ed_selection, name);
    *t++ = ' ';
    strcpy(t, pref);
    ed_show->setEnabled(true);
    ed_erase->setEnabled(true);
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
        if (Fnt()->getFont(&fnt, fnum))
            ed_list->setFont(*fnt);
        update();
    }
}

