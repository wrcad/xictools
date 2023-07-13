
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

#include "qttedit.h"
#include "sced.h"
#include "cd_lgen.h"
#include "cd_terminal.h"
#include "dsp_inlines.h"
#include "dsp_layer.h"
#include "menu.h"
#include "errorlog.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>

//---------------------------------------------------------------------------
// Pop-up interface for terminal/property editing.  This handles
// electrical mode (SUBCT command).
//
// Help system keywords used:
//  xic:edtrm

void
cSced::PopUpTermEdit(GRobject caller, ShowMode mode, TermEditInfo *tinfo,
    void(*action)(TermEditInfo*, CDp*), CDp *prp, int x, int y)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTelecTermEditDlg::self())
            QTelecTermEditDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTelecTermEditDlg::self() && tinfo)
            QTelecTermEditDlg::self()->update(tinfo, prp);
        return;
    }
    if (QTelecTermEditDlg::self()) {
        // Once the pop-up is visible, reuse it when clicking on other
        // terminals.
        if (tinfo) {
            QTelecTermEditDlg::self()->update(tinfo, prp);
            return;
        }
        PopUpTermEdit(0, MODE_OFF, 0, 0, 0, 0, 0);
    }

    if (!tinfo)
        return;

    new QTelecTermEditDlg(caller, tinfo, action, prp);

    /*XXX
    int mwid, mhei;
    gtk_MonitorGeom(GTKmainwin::self()->Shell(), 0, 0, &mwid, &mhei);
    GtkRequisition req;
    gtk_widget_get_requisition(TE->shell(), &req);
    if (x + req.width > mwid)
        x = mwid - req.width;
    if (y + req.height > mhei)
        y = mhei - req.height;
    gtk_window_move(GTK_WINDOW(TE->shell()), x, y);
    gtk_widget_show(TE->shell());

    // OpenSuse-13.1 gtk-2.24.23 bug
    gtk_window_move(GTK_WINDOW(TE->shell()), x, y);
    */
    QTelecTermEditDlg::self()->show();
}
// End of cSced functions.


QTelecTermEditDlg *QTelecTermEditDlg::instPtr;

QTelecTermEditDlg::QTelecTermEditDlg(GRobject caller, TermEditInfo *tinfo,
    void(*action)(TermEditInfo*, CDp*), CDp *prp)
{
    instPtr = this;
    te_caller = caller;
    te_lab_top = 0;
    te_lab_index = 0;
    te_sb_index = 0;
    te_name = 0;
    te_lab_netex = 0;
    te_netex = 0;
    te_layer = 0;
    te_fixed = 0;
    te_phys = 0;
    te_physgrp = 0;
    te_byname = 0;
    te_scinvis = 0;
    te_syinvis = 0;
    te_bitsgrp = 0;
    te_crtbits = 0;
    te_ordbits = 0;
    te_sb_toindex = 0;

    te_action = action;
    te_prp = prp;
    te_lname = 0;
    te_bterm = false;

    setWindowTitle(tr("Terminal Edit"));
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // label in frame plus help btn
    //
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setMargin(0);
    hb->setSpacing(2);
    te_lab_top = new QLabel("");
    hb->addWidget(te_lab_top);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // Bus Term Indices.
    //
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    te_lab_index = new QLabel(tr("Term Index"));
    hbox->addWidget(te_lab_index);

    te_sb_index = new QSpinBox();
    hbox->addWidget(te_sb_index);
    te_sb_index->setMinimum(0);
    te_sb_index->setMaximum(P_NODE_MAX_INDEX);
    te_sb_index->setValue(0);

    // Name Entry
    //
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QLabel *label = new QLabel(tr("Terminal Name"));
    hbox->addWidget(label);

    te_name = new QLineEdit();;
    hbox->addWidget(te_name);

    // NetEx Entry
    //
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    te_lab_netex = new QLabel(tr("Net Expression"));
    hbox->addWidget(te_lab_netex);

    te_netex = new QLineEdit();
    hbox->addWidget(te_netex);;
/*XXX
    // Pressing Enter in the entry presses Apply.
    gtk_window_set_focus(GTK_WINDOW(te_popup), te_name);
    gtk_entry_set_activates_default(GTK_ENTRY(te_name), true);
*/

    // Has Physical Term?
    //
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    te_phys = new QCheckBox(tr("Has physical terminal"));
    hbox->addWidget(te_phys);
    connect(te_phys, SIGNAL(stateChanged(int)),
        this, SLOT(has_phys_term_slot(int)));

    btn = new QPushButton("Delete");
    hbox->addWidget(te_phys);
    connect(btn, SIGNAL(clicked()), this, SLOT(destroy_btn_slot()));

    // Physical Group
    //
    te_physgrp = new QGroupBox(tr("Physical"));
    vbox->addWidget(te_physgrp);
    QVBoxLayout *vb = new QVBoxLayout(te_physgrp);
    vb->setMargin(2);
    vb->setSpacing(2);
    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setMargin(0);
    hb->setSpacing(2);

    // Layer Binding
    //
    label = new QLabel(tr("Layer Binding"));
    hb->addWidget(label);

    te_layer = new QComboBox();;
    hb->addWidget(te_layer);

    te_fixed = new QCheckBox(tr("Location locked by user placement"));
    vb->addWidget(te_fixed);

    // Check boxes
    //
    te_byname = new QCheckBox(tr("Set connect by name only"));
    vbox->addWidget(te_byname);

    te_scinvis = new QCheckBox(tr("Set terminal invisible in schematic"));
    vbox->addWidget(te_scinvis);

    te_syinvis = new QCheckBox(tr("Set terminal invisible in symbol"));
    vbox->addWidget(te_syinvis);

    // Bterm bits buttons
    //
    te_bitsgrp = new QGroupBox(tr("Bus Term Bits"));
    vbox->addWidget(te_bitsgrp);
    vb = new QVBoxLayout(te_bitsgrp);
    vb->setMargin(2);
    vb->setSpacing(2);
    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setMargin(0);
    hb->setSpacing(2);

    te_crtbits = new QPushButton(tr("Check/Create Bits"));
    hb->addWidget(te_crtbits);
    connect(te_crtbits, SIGNAL(clicked()), this, SLOT(crbits_btn_slot()));

    te_ordbits = new QPushButton(tr("Reorder to Index"));
    hb->addWidget(te_ordbits);
    connect(te_ordbits, SIGNAL(clicked()), this, SLOT(ordbits_btn_slot()));

    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setMargin(0);
    hb->setSpacing(2);

    btn = new QPushButton(tr("Schem Vis"));
    hb->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(scvis_btn_slot()));

    btn = new QPushButton(tr("Invis"));
    hb->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(scinvis_btn_slot()));

    hb->addStretch(1);

    btn = new QPushButton(tr("Symbol Vis"));
    hb->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(syvis_btn_slot()));

    btn = new QPushButton(tr("Invis"));
    hb->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(syinvis_btn_slot()));

    // Prev, Next buttons
    //
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    btn = new QPushButton(tr("Prev"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(prev_btn_slot()));

    btn = new QPushButton(tr("Next"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(next_btn_slot()));

    hbox->addStretch(1);

    btn = new QPushButton(tr("To Index"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(toindex_btn_slot()));

    te_sb_toindex = new QSpinBox();
    hbox->addWidget(te_sb_toindex);
    te_sb_toindex->setMinimum(0);
    te_sb_toindex->setMaximum(P_NODE_MAX_INDEX);
    te_sb_toindex->setValue(0);

    // Apply, Dismiss buttons
    //
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    btn = new QPushButton(tr("Apply"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(apply_btn_slot()));

/*XXX
    // Pressing Enter in the entry presses Apply.
    gtk_widget_set_can_default(button, true);
    gtk_window_set_default(GTK_WINDOW(te_popup), button);
*/

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update(tinfo, prp);
}


QTelecTermEditDlg::~QTelecTermEditDlg()
{
    instPtr = 0;
    if (te_caller)
        QTdev::SetStatus(te_caller, false);
    if (te_action)
        (*te_action)(0, te_prp);
    delete [] te_lname;
}


void
QTelecTermEditDlg::update(TermEditInfo *tinfo, CDp *prp)
{
    const char *name = tinfo->name();
    if (!name)
        name = "";
    te_name->setText(name);
    const char *netex = tinfo->netex();
    if (!netex)
        netex = "";
    te_netex->setText(netex);

    te_bterm = tinfo->has_bterm();

    setEnabled(prp != 0);

    te_layer->clear();
    te_layer->addItem("any_layer");

    CDl *ld;
    CDlgen lgen(Physical);
    while ((ld = lgen.next()) != 0) {
        if (!ld->isRouting())
            continue;
        te_layer->addItem(ld->name());
    }
    te_layer->setCurrentIndex(0);
    connect(te_layer, SIGNAL(currentIndexChanged(int)),
        this, SLOT(layer_menu_slot(int)));

    te_sb_index->setValue(tinfo->index());
    te_sb_toindex->setValue(tinfo->index());
    if (te_bterm) {
        te_lab_top->setText(tr("Edit Bus Terminal Properties"));
        te_lab_netex->show();
        te_netex->show();
        te_phys->hide();
        te_physgrp->hide();
        te_byname->hide();
        te_bitsgrp->show();

        // These requires schematic display and a terminal name.
        CDs *sd = CurCell(Electrical);
        if (sd && sd->isSymbolic()) {
            te_crtbits->setEnabled(false);
            te_ordbits->setEnabled(false);
        }
        else {
            if (prp && ((CDp_bsnode*)prp)->has_name()) {
                te_crtbits->setEnabled(true);
                te_ordbits->setEnabled(true);
            }
            else {
                te_crtbits->setEnabled(false);
                te_ordbits->setEnabled(false);
            }
        }
        QTdev::SetStatus(te_scinvis, tinfo->has_flag(TE_SCINVIS));
        QTdev::SetStatus(te_syinvis, tinfo->has_flag(TE_SYINVIS));
    }
    else {
        te_lab_top->setText(tr("Edit Terminal Properties"));
        te_lab_netex->hide();
        te_netex->hide();
        te_phys->show();
        te_physgrp->show();
        te_byname->show();
        te_bitsgrp->hide();

        te_phys->setEnabled(DSP()->CurMode() == Electrical);

        bool hset = false;
        if (tinfo->layer_name() && *tinfo->layer_name()) {
            int cnt = 1;
            lgen = CDlgen(Physical);
            while ((ld = lgen.next()) != 0) {
                if (!ld->isRouting())
                    continue;
                if (!strcmp(ld->name(), tinfo->layer_name())) {
                    te_layer->setCurrentIndex(cnt);
                    set_layername(ld->name());
                    hset = true;
                    break;
                }
                cnt++;
            }
        }
        if (!hset) {
            te_layer->setCurrentIndex(0);
            set_layername(0);
        }

        QTdev::SetStatus(te_fixed, tinfo->has_flag(TE_FIXED));
        QTdev::SetStatus(te_phys, tinfo->has_phys());
        QTdev::SetStatus(te_byname, tinfo->has_flag(TE_BYNAME));
        QTdev::SetStatus(te_scinvis, tinfo->has_flag(TE_SCINVIS));
        QTdev::SetStatus(te_syinvis, tinfo->has_flag(TE_SYINVIS));
        te_physgrp->setEnabled(tinfo->has_phys());
    }
    te_prp = prp;
}


void
QTelecTermEditDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:edtrm"))
}


void
QTelecTermEditDlg::has_phys_term_slot(int state)
{
    te_physgrp->setEnabled(state);
}


void
QTelecTermEditDlg::destroy_btn_slot()
{
    SCD()->subcircuitDeleteTerm();
}


void
QTelecTermEditDlg::crbits_btn_slot()
{
    if (!SCD()->subcircuitBits(false)) {
        Log()->ErrorLogV(mh::Processing,
            "Operation failed: %s.", Errs()->get_error());
    }
}


void
QTelecTermEditDlg::ordbits_btn_slot()
{
    if (!SCD()->subcircuitBits(true)) {
        Log()->ErrorLogV(mh::Processing,
            "Operation failed: %s.", Errs()->get_error());
    }
}


void
QTelecTermEditDlg::scvis_btn_slot()
{
    SCD()->subcircuitBitsVisible(TE_SCINVIS);
}


void
QTelecTermEditDlg::scinvis_btn_slot()
{
    SCD()->subcircuitBitsInvisible(TE_SCINVIS);
}


void
QTelecTermEditDlg::syvis_btn_slot()
{
    SCD()->subcircuitBitsVisible(TE_SYINVIS);
}


void
QTelecTermEditDlg::syinvis_btn_slot()
{
    SCD()->subcircuitBitsInvisible(TE_SYINVIS);
}


void
QTelecTermEditDlg::prev_btn_slot()
{
    int indx;
    bool was_bt;
    if (SCD()->subcircuitEditTerm(&indx, &was_bt)) {
        if (!was_bt) {
            if (SCD()->subcircuitSetEditTerm(indx, true))
                return;
        }
        indx--;
        if (SCD()->subcircuitSetEditTerm(indx, false))
            return;
        if (SCD()->subcircuitSetEditTerm(indx, true))
            return;
    }
}


void
QTelecTermEditDlg::next_btn_slot()
{
    int indx;
    bool was_bt;
    if (SCD()->subcircuitEditTerm(&indx, &was_bt)) {
        if (was_bt) {
            if (SCD()->subcircuitSetEditTerm(indx, false))
                return;
        }
        indx++;
        if (SCD()->subcircuitSetEditTerm(indx, true))
            return;
        if (SCD()->subcircuitSetEditTerm(indx, false))
            return;
    }
}


void
QTelecTermEditDlg::toindex_btn_slot()
{
    int indx = te_sb_toindex->value();
    if (SCD()->subcircuitSetEditTerm(indx, true))
        return;
    if (SCD()->subcircuitSetEditTerm(indx, false))
        return;
}


void
QTelecTermEditDlg::apply_btn_slot()
{
    if (te_action) {
        QByteArray name_ba = te_name->text().toLatin1();
        unsigned int ix = te_sb_index->value();
        if (te_bterm) {
            QByteArray netex_ba = te_netex->text().toLatin1();
            unsigned int f = 0;
            if (QTdev::GetStatus(te_scinvis))
                f |= TE_SCINVIS;
            if (QTdev::GetStatus(te_syinvis))
                f |= TE_SYINVIS;

            TermEditInfo tinfo(name_ba.constData(), ix, f,
                netex_ba.constData());
            (*te_action)(&tinfo, te_prp);
            SCD()->PopUpTermEdit(0, MODE_UPD, &tinfo, 0, te_prp, 0, 0);
        }
        else {
            unsigned int f = 0;
            if (QTdev::GetStatus(te_fixed))
                f |= TE_FIXED;
            if (QTdev::GetStatus(te_byname))
                f |= TE_BYNAME;
            if (QTdev::GetStatus(te_scinvis))
                f |= TE_SCINVIS;
            if (QTdev::GetStatus(te_syinvis))
                f |= TE_SYINVIS;

            TermEditInfo tinfo(name_ba.constData(), te_lname, ix, f,
                QTdev::GetStatus(te_phys));
            (*te_action)(&tinfo, te_prp);
            SCD()->PopUpTermEdit(0, MODE_UPD, &tinfo, 0, te_prp, 0, 0);
        }
    }
    else
        SCD()->PopUpTermEdit(0, MODE_OFF, 0, 0, 0, 0, 0);
}


void
QTelecTermEditDlg::layer_menu_slot(int)
{
    if (te_layer->currentIndex() == 0) {
        // any_layer
        set_layername(0);
        return;
    }
    QByteArray ba = te_layer->currentText().toLatin1();
    set_layername(ba.constData());
}


void
QTelecTermEditDlg::dismiss_btn_slot()
{
    SCD()->PopUpTermEdit(0, MODE_OFF, 0, 0, 0, 0, 0);
}

