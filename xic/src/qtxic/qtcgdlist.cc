
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
#include "qtcgdlist.h"
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
#include "qtinterf/qtlist.h"
#include "qtinterf/qttext.h"
#include "qtinterf/qtfont.h"
#include "miscutil/filestat.h"
#include "miscutil/pathlist.h"

#include <QApplication>
#include <QLayout>
#include <QToolButton>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>


//-----------------------------------------------------------------------------
// QTcgdListDlg::  Cell Geometry Digests list dialog.
// Called from main menu: File/Geometry Digests. 
//
// Help system keywords used:
//  xic:geom

void
cConvert::PopUpGeometries(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTcgdListDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTcgdListDlg::self())
            QTcgdListDlg::self()->update();
        return;
    }
    if (QTcgdListDlg::self())
        return;

    new QTcgdListDlg(caller);

    QTcgdListDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_UL), QTcgdListDlg::self(),
        QTmainwin::self()->Viewport());
    QTcgdListDlg::self()->show();
}
// End of cConvert functions.


// Contests pop-up button.
#define INFO_BTN "Info"

QTcgdListDlg *QTcgdListDlg::instPtr;

QTcgdListDlg::QTcgdListDlg(GRobject c) : QTbag(this)
{
    instPtr = this;
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

    setWindowTitle(tr("Cell Geometry Digests"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    cgl_addbtn = new QToolButton();
    cgl_addbtn->setText(tr("Add"));
    hbox->addWidget(cgl_addbtn);
    cgl_addbtn->setCheckable(true);
    connect(cgl_addbtn, &QAbstractButton::toggled,
        this, &QTcgdListDlg::add_btn_slot);

    cgl_savbtn = new QToolButton();
    cgl_savbtn->setText(tr("Save"));
    hbox->addWidget(cgl_savbtn);
    cgl_savbtn->setCheckable(true);
    connect(cgl_savbtn, &QAbstractButton::toggled,
        this, &QTcgdListDlg::sav_btn_slot);

    cgl_delbtn = new QToolButton();
    cgl_delbtn->setText(tr("Delete"));
    hbox->addWidget(cgl_delbtn);
    cgl_delbtn->setCheckable(true);
    connect(cgl_delbtn, &QAbstractButton::toggled,
        this, &QTcgdListDlg::del_btn_slot);

    cgl_cntbtn = new QToolButton();
    cgl_cntbtn->setText(tr("Contents"));
    hbox->addWidget(cgl_cntbtn);
    connect(cgl_cntbtn, &QAbstractButton::clicked,
        this, &QTcgdListDlg::cont_btn_slot);

    cgl_infbtn = new QToolButton();
    cgl_infbtn->setText(tr("Info"));
    hbox->addWidget(cgl_infbtn);
    cgl_infbtn->setCheckable(true);
    connect(cgl_infbtn, &QAbstractButton::toggled,
        this, &QTcgdListDlg::inf_btn_slot);

    hbox->addStretch(1);
    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTcgdListDlg::help_btn_slot);

    // scrolled list
    //
    cgl_list = new QTreeWidget();
    vbox->addWidget(cgl_list);
    cgl_list->setHeaderLabels(QStringList(QList<QString>() << tr("Db Name") <<
        tr("Type, Linked") << tr("Source ll - Click to select")));

    connect(cgl_list, &QTreeWidget::currentItemChanged,
        this, &QTcgdListDlg::current_item_changed_slot);
    connect(cgl_list, &QTreeWidget::itemActivated,
        this, &QTcgdListDlg::item_activated_slot);
    connect(cgl_list, &QTreeWidget::itemClicked,
        this, &QTcgdListDlg::item_clicked_slot);
    connect(cgl_list, &QTreeWidget::itemSelectionChanged,
        this, &QTcgdListDlg::item_selection_changed);

    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_PROP))
        cgl_list->setFont(*fnt);
    connect(QTfont::self(), &QTfont::fontChanged,
        this, &QTcgdListDlg::font_changed_slot, Qt::QueuedConnection);

    // dismiss button
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    vbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QTcgdListDlg::dismiss_btn_slot);

    update();
}


QTcgdListDlg::~QTcgdListDlg()
{
    instPtr = 0;
    delete [] cgl_selection;
    delete [] cgl_contlib;

    if (cgl_caller)
        QTdev::Deselect(cgl_caller);
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
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTcgdListDlg
#include "qtinterf/qtmacos_event.h"
#endif


// Update the listing.
//
void
QTcgdListDlg::update()
{
    if (cgl_selection && !CDcgd()->cgdRecall(cgl_selection, false)) {
        delete [] cgl_selection;
        cgl_selection = 0;
    }

    if (!cgl_selection) {
        cgl_savbtn->setEnabled(false);
        cgl_delbtn->setEnabled(false);
        cgl_cntbtn->setEnabled(false);
        cgl_infbtn->setEnabled(false);
        if (cgl_sav_pop)
            cgl_sav_pop->popdown();
        if (cgl_del_pop)
            cgl_del_pop->popdown();
    }

    stringlist *names = CDcgd()->cgdList();
    stringlist::sort(names);

    cgl_list->clear();

    for (stringlist *l = names; l; l = l->next) {
        cCGD *cgd = CDcgd()->cgdRecall(l->string, false);
        if (cgd) {
            char *strings[3];
            strings[0] = l->string;
            strings[1] = cgd->info(true);
            strings[2] = lstring::copy(cgd->sourceName() ?
                lstring::strip_path(cgd->sourceName()) : "");

            QTreeWidgetItem *item = new QTreeWidgetItem(
                QStringList(QList<QString>() << strings[0] <<
                strings[1] << strings[2]));
            cgl_list->addTopLevelItem(item);

            delete [] strings[1];
            delete [] strings[2];
        }
    }

    stringlist::destroy(names);
}


void
QTcgdListDlg::err_message(const char *fmt)
{
    const char *s = Errs()->get_error();
    int len = strlen(fmt) + (s ? strlen(s) : 0) + 10;
    char *t = new char[len];
    snprintf(t, len, fmt, s);
    PopUpMessage(t, true);
    delete [] t;
}


// Static function.
// Callback for the Open dialog.
//
bool
QTcgdListDlg::cgl_add_cb(const char *idname, const char *string, int mode, void*)
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
        instPtr->err_message("Failed to create new Geometry Digest:\n%s");
        return (false);
    }
    return (true);
}


// Static function.
// Callback for the Save dialog.
//
ESret
QTcgdListDlg::cgl_sav_cb(const char *fname, void*)
{
    if (!instPtr->cgl_selection) {
        instPtr->err_message("No selection, select a CGD in the listing.");
        return (ESTR_IGN);
    }
    cCGD *cgd = CDcgd()->cgdRecall(instPtr->cgl_selection, false);
    if (!cgd) {
        Errs()->add_error("unresolved CGD name %s.", instPtr->cgl_selection);
        instPtr->err_message("Error occurred:\n%s");
        return (ESTR_IGN);
    }

    QTpkg::self()->SetWorking(true);
    bool ok = cgd->write(fname);
    QTpkg::self()->SetWorking(false);
    if (!ok) {
        instPtr->err_message("Error occurred when writing digest file:\n%s");
        return (ESTR_IGN);
    }
    instPtr->PopUpMessage("Digest file saved successfully.", false);
    return (ESTR_DN);
}


// Static function.
// Callback for confirmation pop-up.
//
void
QTcgdListDlg::cgl_del_cb(bool yn, void *arg)
{
    if (arg && yn && instPtr) {
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
// When the user clicks on a cell name in the contents list, show a
// listing of the layers used.
//
void
QTcgdListDlg::cgl_cnt_cb(const char *cellname, void*)
{
    if (!instPtr)
        return;
    if (!instPtr->cgl_contlib || !instPtr->cgl_cnt_pop)
        return;
    if (!cellname)
        return;
    if (*cellname != '/')
        return;

    // Callback from button press.
    cellname++;
    if (!strcmp(cellname, INFO_BTN)) {
        cCGD *cgd = CDcgd()->cgdRecall(instPtr->cgl_contlib, false);
        if (!cgd)
            return;
        char *listsel = instPtr->cgl_cnt_pop->get_selection();
        stringlist *s0 = cgd->layer_info_list(listsel);
        char buf[256];
        snprintf(buf, sizeof(buf), "Layers found in %s", listsel);
        delete [] listsel;
        if (instPtr->cgl_inf_pop)
            instPtr->cgl_inf_pop->update(s0, buf);
        else {
            instPtr->cgl_inf_pop = DSPmainWbagRet(PopUpMultiCol(s0, buf,
                0, 0, 0, 0, true));
            if (instPtr->cgl_inf_pop) {
                instPtr->cgl_inf_pop->register_usrptr(
                    (void**)&instPtr->cgl_inf_pop);
            }
        }
        stringlist::destroy(s0);
    }
}


void
QTcgdListDlg::add_btn_slot(bool state)
{
    if (state) {
        QPoint pg = mapToGlobal(QPoint(0, 0));
        char *cn = CDcgd()->newCgdName();
        // Pop down first, panel used elsewhere.
        Cvt()->PopUpCgdOpen(0, MODE_OFF, 0, 0, 0, 0, 0, 0);
        Cvt()->PopUpCgdOpen(cgl_addbtn, MODE_ON, cn, 0,
            pg.x() + 40, pg.y() + 100, cgl_add_cb, 0);
        delete [] cn;
    }
    else
        Cvt()->PopUpCgdOpen(0, MODE_OFF, 0, 0, 0, 0, 0, 0);
}


void
QTcgdListDlg::sav_btn_slot(bool state)
{
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
        QTdev::Deselect(cgl_savbtn);
}


void
QTcgdListDlg::del_btn_slot(bool state)
{
    if (cgl_sav_pop)
        cgl_sav_pop->popdown();
    if (cgl_del_pop)
        cgl_del_pop->popdown();
    if (cgl_selection && state) {
        cCGD *cgd = CDcgd()->cgdRecall(cgl_selection, false);
        if (cgd && !cgd->refcnt()) {
            // should always be true
            cgl_del_pop = PopUpAffirm(cgl_delbtn, GRloc(),
                "Confirm - delete selected digest?", cgl_del_cb,
                lstring::copy(cgl_selection));
            if (cgl_del_pop)
                cgl_del_pop->register_usrptr((void**)&cgl_del_pop);
        }
        else {
            QTdev::Deselect(cgl_delbtn);
            QTdev::SetSensitive(cgl_delbtn, false);
        }
    }
    else
        QTdev::Deselect(cgl_delbtn);
}


void
QTcgdListDlg::cont_btn_slot()
{
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
}


void
QTcgdListDlg::inf_btn_slot(bool state)
{
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
}


void
QTcgdListDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:geom"))
}


void
QTcgdListDlg::current_item_changed_slot(QTreeWidgetItem *cur, QTreeWidgetItem*)
{
    if (!cur) {
        delete [] cgl_selection;
        cgl_selection = 0;
        return;
    }
    char *text = lstring::copy(cur->text(0).toLatin1().constData());
    if (text) {
        delete [] cgl_selection;
        cgl_selection = text;
        cgl_savbtn->setEnabled(true);
        cgl_cntbtn->setEnabled(true);
        cgl_infbtn->setEnabled(true);
        cCGD *cgd = CDcgd()->cgdRecall(cgl_selection, false);
        cgl_delbtn->setEnabled(cgd && !cgd->refcnt());
        if (cgd && wb_info) {
            char *infostr = cgd->info(false);
            wb_info->setText(infostr);
            delete [] infostr;
        }
    }
}


void
QTcgdListDlg::item_activated_slot(QTreeWidgetItem*, int)
{
}


void
QTcgdListDlg::item_clicked_slot(QTreeWidgetItem*, int)
{
}


void
QTcgdListDlg::item_selection_changed()
{
}


void
QTcgdListDlg::dismiss_btn_slot()
{
    Cvt()->PopUpGeometries(0, MODE_OFF);
}


void
QTcgdListDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_PROP) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, fnum))
            cgl_list->setFont(*fnt);
        update();
    }
}

