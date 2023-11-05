
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

#include "qtpfilt.h"
#include "cvrt.h"
#include "cd_compare.h"
#include "dsp_inlines.h"

#include <QLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

//-----------------------------------------------------------------------------
// The Custom Property Filter Setup pop-up, called from the Compare
// Layouts panel.
//
// Help system keywords used:
//  xic:prpfilt

void
cConvert::PopUpPropertyFilter(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTcmpPrpFltDlg::self())
            QTcmpPrpFltDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTcmpPrpFltDlg::self())
            QTcmpPrpFltDlg::self()->update();
        return;
    }
    if (QTcmpPrpFltDlg::self())
        return;

    new QTcmpPrpFltDlg(caller);

    QTcmpPrpFltDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_UR), QTcmpPrpFltDlg::self(),
        QTmainwin::self()->Viewport());
    QTcmpPrpFltDlg::self()->show();
}
// End of cCompare functions.


QTcmpPrpFltDlg *QTcmpPrpFltDlg::instPtr;

QTcmpPrpFltDlg::QTcmpPrpFltDlg(GRobject c)
{
    instPtr = this;
    pf_caller = c;
    pf_phys_cell = 0;
    pf_phys_inst = 0;
    pf_phys_obj = 0;
    pf_elec_cell = 0;
    pf_elec_inst = 0;
    pf_elec_obj = 0;

    setWindowTitle(tr("Custom Property Filter Setup"));
    setAttribute(Qt::WA_DeleteOnClose);

    QGridLayout *grid = new QGridLayout(this);
    QHBoxLayout *hbox = new QHBoxLayout();
    grid->addLayout(hbox, 0, 1);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(2);

    // physical strings
    //
    QLabel *label = new QLabel(tr("Physical property filter strings"));
    hbox->addWidget(label);
    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    label = new QLabel(tr("Cell"));
    grid->addWidget(label, 1, 0);
    label = new QLabel(tr("Inst"));
    grid->addWidget(label, 2, 0);
    label = new QLabel(tr("Obj"));
    grid->addWidget(label, 3, 0);

    pf_phys_cell = new QLineEdit();
    grid->addWidget(pf_phys_cell, 1, 1);
    connect(pf_phys_cell, SIGNAL(textChanged(const QString&)),
        this, SLOT(phys_cellstr_slot(const QString&)));
    pf_phys_inst = new QLineEdit();
    grid->addWidget(pf_phys_inst, 2, 1);
    connect(pf_phys_inst, SIGNAL(textChanged(const QString&)),
        this, SLOT(phys_inststr_slot(const QString&)));
    pf_phys_obj = new QLineEdit();
    grid->addWidget(pf_phys_obj, 3, 1);
    connect(pf_phys_obj, SIGNAL(textChanged(const QString&)),
        this, SLOT(phys_objstr_slot(const QString&)));

    // electrical strings
    //
    label = new QLabel(tr("Electrical property filter strings"));
    grid->addWidget(label, 4, 1);

    label = new QLabel(tr("Cell"));
    grid->addWidget(label, 5, 0);
    label = new QLabel(tr("Inst"));
    grid->addWidget(label, 6, 0);
    label = new QLabel(tr("Obj"));
    grid->addWidget(label, 7, 0);

    pf_elec_cell = new QLineEdit();
    grid->addWidget(pf_elec_cell, 5, 1);
    connect(pf_elec_cell, SIGNAL(textChanged(const QString&)),
        this, SLOT(elec_cellstr_slot(const QString&)));
    pf_elec_inst = new QLineEdit();
    grid->addWidget(pf_elec_inst, 6, 1);
    connect(pf_elec_inst, SIGNAL(textChanged(const QString&)),
        this, SLOT(elec_inststr_slot(const QString&)));
    pf_elec_obj = new QLineEdit();
    grid->addWidget(pf_elec_obj, 7, 1);
    connect(pf_elec_obj, SIGNAL(textChanged(const QString&)),
        this, SLOT(elec_objstr_slot(const QString&)));

    // dismiss button
    //
    btn = new QPushButton(tr("Dismiss"));
    grid->addWidget(btn, 8, 0, 1, 2);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTcmpPrpFltDlg::~QTcmpPrpFltDlg()
{
    instPtr = 0;
    if (pf_caller)
        QTdev::Deselect(pf_caller);
}


void
QTcmpPrpFltDlg::update()
{
    const char *s1 = CDvdb()->getVariable(VA_PhysPrpFltCell);
    const char *s2 = lstring::copy(
        pf_phys_cell->text().toLatin1().constData());
    if (s1 && strcmp(s1, s2))
        pf_phys_cell->setText(s1);
    else if (!s1)
        pf_phys_cell->setText("");
    delete [] s2;

    s1 = CDvdb()->getVariable(VA_PhysPrpFltInst);
    s2 = lstring::copy(pf_phys_inst->text().toLatin1().constData());
    if (s1 && strcmp(s1, s2))
        pf_phys_inst->setText(s1);
    else if (!s1)
        pf_phys_inst->setText("");
    delete [] s2;

    s1 = CDvdb()->getVariable(VA_PhysPrpFltObj);
    s2 = lstring::copy(pf_phys_obj->text().toLatin1().constData());
    if (s1 && strcmp(s1, s2))
        pf_phys_obj->setText(s1);
    else if (!s1)
        pf_phys_obj->setText("");
    delete [] s2;

    s1 = CDvdb()->getVariable(VA_ElecPrpFltCell);
    s2 = lstring::copy(pf_elec_cell->text().toLatin1().constData());
    if (s1 && strcmp(s1, s2))
        pf_elec_cell->setText(s1);
    else if (!s1)
        pf_elec_cell->setText("");
    delete [] s2;

    s1 = CDvdb()->getVariable(VA_ElecPrpFltInst);
    s2 = lstring::copy(pf_elec_inst->text().toLatin1().constData());
    if (s1 && strcmp(s1, s2))
        pf_elec_inst->setText(s1);
    else if (!s1)
        pf_elec_inst->setText("");
    delete [] s2;

    s1 = CDvdb()->getVariable(VA_ElecPrpFltObj);
    s2 = lstring::copy(pf_elec_obj->text().toLatin1().constData());
    if (s1 && strcmp(s1, s2))
        pf_elec_obj->setText(s1);
    else if (!s1)
        pf_elec_obj->setText("");
    delete [] s2;
}

void
QTcmpPrpFltDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:prpfilt"))
}


void
QTcmpPrpFltDlg::phys_cellstr_slot(const QString &qs)
{
    if (qs.isNull() || qs.isEmpty())
        CDvdb()->clearVariable(VA_PhysPrpFltCell);
    else {
        const char *s = lstring::copy(qs.toLatin1().constData());
        CDvdb()->setVariable(VA_PhysPrpFltCell, s);
        delete [] s;
    }
}


void
QTcmpPrpFltDlg::phys_inststr_slot(const QString &qs)
{
    if (qs.isNull() || qs.isEmpty())
        CDvdb()->clearVariable(VA_PhysPrpFltInst);
    else {
        const char *s = lstring::copy(qs.toLatin1().constData());
        CDvdb()->setVariable(VA_PhysPrpFltInst, s);
        delete [] s;
    }
}


void
QTcmpPrpFltDlg::phys_objstr_slot(const QString &qs)
{
    if (qs.isNull() || qs.isEmpty())
        CDvdb()->clearVariable(VA_PhysPrpFltObj);
    else {
        const char *s = lstring::copy(qs.toLatin1().constData());
        CDvdb()->setVariable(VA_PhysPrpFltObj, s);
        delete [] s;
    }
}


void
QTcmpPrpFltDlg::elec_cellstr_slot(const QString &qs)
{
    if (qs.isNull() || qs.isEmpty())
        CDvdb()->clearVariable(VA_ElecPrpFltCell);
    else {
        const char *s = lstring::copy(qs.toLatin1().constData());
        CDvdb()->setVariable(VA_ElecPrpFltCell, s);
        delete [] s;
    }
}


void
QTcmpPrpFltDlg::elec_inststr_slot(const QString &qs)
{
    if (qs.isNull() || qs.isEmpty())
        CDvdb()->clearVariable(VA_ElecPrpFltInst);
    else {
        const char *s = lstring::copy(qs.toLatin1().constData());
        CDvdb()->setVariable(VA_ElecPrpFltInst, s);
        delete [] s;
    }
}


void
QTcmpPrpFltDlg::elec_objstr_slot(const QString &qs)
{
    if (qs.isNull() || qs.isEmpty())
        CDvdb()->clearVariable(VA_ElecPrpFltObj);
    else {
        const char *s = lstring::copy(qs.toLatin1().constData());
        CDvdb()->setVariable(VA_ElecPrpFltObj, s);
        delete [] s;
    }
}


void
QTcmpPrpFltDlg::dismiss_btn_slot()
{
    Cvt()->PopUpPropertyFilter(0, MODE_OFF);
}

