
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

#include "qtstab.h"
#include "cvrt.h"
#include "fio.h"
#include "cd_celldb.h"
#include "dsp_inlines.h"
#include "promptline.h"
#include "events.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>


//-----------------------------------------------------------------------------
// QTstabDlg:  Dialog to set current symbol table.
// Called from the main menu: Cell/Symbol Tables.
//
// Help system keywords used:
//  xic:stabs

void
cMain::PopUpSymTabs(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTstabDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTstabDlg::self())
            QTstabDlg::self()->update();
        return;
    }
    if (QTstabDlg::self())
        return;

    new QTstabDlg(caller);

    QTstabDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTstabDlg::self(),
        QTmainwin::self()->Viewport());
    QTstabDlg::self()->show();
}
// End of cMain functions.


QTstabDlg *QTstabDlg::instPtr;

QTstabDlg::QTstabDlg(GRobject c) : QTbag(this)
{
    instPtr = this;
    tb_caller = c;
    tb_tables = 0;
    tb_add = 0;
    tb_clr = 0;
    tb_del = 0;

    tb_add_pop = 0;
    tb_clr_pop = 0;
    tb_del_pop = 0;
    tb_namelist = 0;

    setWindowTitle(tr("Symbol Tables"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);
    vbox->setSizeConstraint(QLayout::SetFixedSize);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // Label in frame plus help button.
    //
    QGroupBox *gb = new QGroupBox(this);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    QLabel *label = new QLabel(tr("Choose symbol table"));
    hb->addWidget(label);
    hbox->addWidget(gb);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // Symbol tables list.
    //
    tb_tables = new QComboBox();
    vbox->addWidget(tb_tables);
    tb_namelist = CDcdb()->listTables();
    for (stringlist *s = tb_namelist; s; s = s->next)
        tb_tables->addItem((const char*)s->string);
    tb_tables->setCurrentIndex(0);
    connect(tb_tables, SIGNAL(currentIndexChanged(int)),
        this, SLOT(table_change_slot(int)));

    // Button row.
    //
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    tb_add = new QPushButton(tr("Add"));
    hbox->addWidget(tb_add);
    tb_add->setCheckable(true);
    tb_add->setAutoDefault(false);
    connect(tb_add, SIGNAL(toggled(bool)), this, SLOT(add_btn_slot(bool)));

    tb_clr = new QPushButton(tr("Clear"));
    hbox->addWidget(tb_clr);
    tb_clr->setCheckable(true);
    tb_clr->setAutoDefault(false);
    connect(tb_clr, SIGNAL(toggled(bool)), this, SLOT(clear_btn_slot(bool)));

    tb_del = new QPushButton(tr("Destroy"));
    hbox->addWidget(tb_del);
    tb_del->setCheckable(true);
    tb_del->setAutoDefault(false);
    if (!tb_namelist || !strcmp(tb_namelist->string, CD_MAIN_ST_NAME))
        tb_del->setEnabled(false);
    connect(tb_del, SIGNAL(toggled(bool)), this, SLOT(destroy_btn_slot(bool)));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));
}


QTstabDlg::~QTstabDlg()
{
    instPtr = 0;
    if (tb_del_pop)
        tb_del_pop->popdown();
    if (tb_clr_pop)
        tb_clr_pop->popdown();
    if (tb_add_pop)
        tb_add_pop->popdown();
    stringlist::destroy(tb_namelist);
    if (tb_caller)
        QTdev::Deselect(tb_caller);
}


void
QTstabDlg::update()
{
    stringlist *list = CDcdb()->listTables();

    bool changed = false;
    stringlist *s1 = list, *s2 = tb_namelist;
    while (s1 && s2) {
        if (strcmp(s1->string, s2->string)) {
            changed = true;
            break;
        }
        s1 = s1->next;
        s2 = s2->next;
    }
    if (s1 || s2)
        changed = true;

    if (changed) {
        tb_tables->clear();
        for (stringlist *s = list; s; s = s->next)
            tb_tables->addItem(s->string);
        tb_tables->setCurrentIndex(0);
        stringlist::destroy(tb_namelist);
        tb_namelist = list;
    }
    else
        stringlist::destroy(list);
    if (!tb_namelist || !strcmp(tb_namelist->string, CD_MAIN_ST_NAME))
        tb_del->setEnabled(false);
    else
        tb_del->setEnabled(true);
}


void
QTstabDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:stabs"))
}


void
QTstabDlg::table_change_slot(int)
{
    QString qs = tb_tables->currentText();
    if (qs.length() > 0)
        XM()->SetSymbolTable(qs.toLatin1().constData());
}


void
QTstabDlg::add_btn_slot(bool state)
{
    if (tb_del_pop)
        tb_del_pop->popdown();
    if (tb_clr_pop)
        tb_clr_pop->popdown();
    if (tb_add_pop)
        tb_add_pop->popdown();
    if (state) {
        GRloc loc(LW_XYR, width() + 4, 0);
        tb_add_pop = PopUpEditString((GRobject)tb_add,
            loc, "Enter name for new symbol table", 0, tb_add_cb, 0,
            250, 0, false, 0);
        if (tb_add_pop)
            tb_add_pop->register_usrptr((void**)&tb_add_pop);
    }
}


void
QTstabDlg::clear_btn_slot(bool state)
{
    if (tb_add_pop)
        tb_add_pop->popdown();
    if (tb_clr_pop)
        tb_clr_pop->popdown();
    if (tb_del_pop)
        tb_del_pop->popdown();
    if (state) {
        GRloc loc(LW_XYR, width() + 4, 0);
        tb_clr_pop = PopUpAffirm(tb_clr, loc,
            "Delete contents of current table?", tb_clr_cb, 0);
        if (tb_clr_pop)
            tb_clr_pop->register_usrptr((void**)&tb_clr_pop);
    }
}


void
QTstabDlg::destroy_btn_slot(bool state)
{
    if (tb_add_pop)
        tb_add_pop->popdown();
    if (tb_clr_pop)
        tb_clr_pop->popdown();
    if (tb_del_pop)
        tb_del_pop->popdown();
    if (state) {
        GRloc loc(LW_XYR, width() + 4, 0);
        tb_del_pop = PopUpAffirm(tb_del, loc,
            "Delete current table and contents?", tb_del_cb, 0);
        if (tb_del_pop)
            tb_del_pop->register_usrptr((void**)&tb_del_pop);
    }
}


void
QTstabDlg::dismiss_btn_slot()
{
    XM()->PopUpSymTabs(0, MODE_OFF);
}
// End of alots.


// Static function.
// Callback for the Add dialog.
//
ESret
QTstabDlg::tb_add_cb(const char *name, void*)
{
    if (name && self())
        XM()->SetSymbolTable(name);
    return (ESTR_DN);
}


// Static function.
// Callback for Clear confirmation pop-up.
//
void
QTstabDlg::tb_clr_cb(bool yn, void*)
{
    if (yn && self())
        XM()->Clear(0);
}


// Static function.
// Callback for Destroy confirmation pop-up.
//
void
QTstabDlg::tb_del_cb(bool yn, void*)
{
    if (yn && self())
        XM()->ClearSymbolTable();
}

