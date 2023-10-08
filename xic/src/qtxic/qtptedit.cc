
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

#include "qtptedit.h"
#include "ext.h"
#include "cd_lgen.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "dsp_inlines.h"
#include "dsp_layer.h"
#include "menu.h"
#include "promptline.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>


//---------------------------------------------------------------------------
// Pop-up interface for terminal/property editing.  This handles
// physical mode (TEDIT command).
//
// Help system keywords used:
//  xic:edtrm

void
cExt::PopUpPhysTermEdit(GRobject caller, ShowMode mode, TermEditInfo *tinfo,
    void(*action)(TermEditInfo*, CDsterm*), CDsterm *term, int x, int y)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTphysTermDlg::self())
            QTphysTermDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTphysTermDlg::self() && tinfo)
            QTphysTermDlg::self()->update(tinfo, term);
        return;
    }
    if (QTphysTermDlg::self()) {
        // Once the pop-up is visible, reuse it when clicking on other
        // terminals.
        if (tinfo) {
            QTphysTermDlg::self()->update(tinfo, term);
            return;
        }
        PopUpPhysTermEdit(0, MODE_OFF, 0, 0, 0, 0, 0);
    }

    if (!tinfo)
        return;

    new QTphysTermDlg(caller, tinfo, action, term);

    QTdev::self()->SetPopupLocation(GRloc(LW_XYA, x, y),
        QTphysTermDlg::self(), QTmainwin::self()->Viewport());
    QTphysTermDlg::self()->show();
}
// End of cSced functions.


QTphysTermDlg *QTphysTermDlg::instPtr;

QTphysTermDlg::QTphysTermDlg(GRobject caller, TermEditInfo *tinfo,
    void(*action)(TermEditInfo*, CDsterm*), CDsterm *term)
{
    instPtr = this;
    te_caller = caller;
    te_name = 0;
    te_layer = 0;
    te_flags = 0;
    te_fixed = 0;
    te_physgrp = 0;
    te_action = action;
    te_term = term;
    te_lname = 0;
    te_sb_toindex= 0;

    setWindowTitle(tr("Terminal Edit"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // label in frame plus help btn
    //
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    QLabel *label = new QLabel(tr("Edit Terminal Properties"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // Name Entry
    //
    gb = new QGroupBox(tr("Terminal Name"));
    vbox->addWidget(gb);
    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    te_name = new QLabel("");
    hb->addWidget(te_name);

    // Physical Group
    //
    te_physgrp = new QGroupBox(tr("Physical"));
    vbox->addWidget(te_physgrp);
    QGridLayout *grid = new QGridLayout(gb);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    // Layer Binding
    //
    label = new QLabel(tr("Layer Binding"));
    grid->addWidget(label, 0, 0);

    te_layer = new QComboBox();
    grid->addWidget(te_layer, 0, 1);

    te_fixed = new QCheckBox(tr("Location locked by user placement"));
    grid->addWidget(te_fixed, 1, 0, 1, 2);

    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    label = new QLabel(tr("Current Flags"));
    hbox->addWidget(label);

    te_flags = new QLabel("");
    hbox->addWidget(te_flags);

    // Prev, Next buttons.
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
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
    connect(btn, SIGNAL(clicked()), this, SLOT(toindedx_btn_slot()));

    te_sb_toindex = new QSpinBox();
    hbox->addWidget(te_sb_toindex);
    te_sb_toindex->setRange(0, P_NODE_MAX_INDEX);
    te_sb_toindex->setValue(0);

    // Apply, Dismiss buttons
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    btn = new QPushButton(tr("Apply"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(apply_btn_slot()));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update(tinfo, term);
}


QTphysTermDlg::~QTphysTermDlg()
{
    instPtr = 0;
    if (te_caller)
        QTdev::SetStatus(te_caller, false);
    if (te_action)
        (*te_action)(0, te_term);
    delete [] te_lname;
}


void
QTphysTermDlg::update(TermEditInfo *tinfo, CDsterm *term)
{
    const char *name = tinfo->name();
    if (!name)
        name = "";
    te_name->setText(name);

    setEnabled(term != 0);

    te_layer->clear();
    te_layer->addItem("any layer");
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

    sLstr lstr;
    unsigned int mask = TE_UNINIT | TE_FIXED;
    for (FlagDef *f = TermFlags; f->name; f++) {
        if (tinfo->flags() & f->value & mask) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add(f->name);
        }
    }
    if (!lstr.string())
        lstr.add("none");
    te_flags->setText(lstr.string());

    QTdev::SetStatus(te_fixed, tinfo->has_flag(TE_FIXED));
    CDp_snode *ps0 = (CDp_snode*)CurCell(Electrical)->prpty(P_NODE);
    for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
        if (ps->cell_terminal() == term) {
            te_sb_toindex->setValue(ps->index());
            break;
        }
    }
    te_term = term;
}


void
QTphysTermDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:edtrm"))
}


void
QTphysTermDlg::prev_btn_slot()
{
    CDp_snode *ps0 = (CDp_snode*)CurCell(Electrical)->prpty(P_NODE);
    int topix = -1;
    int curix = -1;
    for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
        if (ps->cell_terminal() == te_term) 
            curix = ps->index();
        if ((int)ps->index() > topix)
            topix = ps->index();
    }
    if (topix < 0 || curix < 0)
        return;
    int nix = curix - 1;
    while (nix >= 0) {
        for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
            if ((int)ps->index() == nix) {
                CDsterm *term = ps->cell_terminal();
                if (term) {
                    EX()->editTermsPush(term);
                    return;
                }
                break;
            }
        }
        nix--;
    }
    nix = topix;
    while (nix >= curix) {
        for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
            if ((int)ps->index() == nix) {
                CDsterm *term = ps->cell_terminal();
                if (term) {
                    EX()->editTermsPush(term);
                    return;
                }
                break;
            }
        }
        nix--;
    }
}


void
QTphysTermDlg::next_btn_slot()
{
    CDp_snode *ps0 = (CDp_snode*)CurCell(Electrical)->prpty(P_NODE);
    int topix = -1;
    int curix = -1;
    for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
        if (ps->cell_terminal() == te_term) 
            curix = ps->index();
        if ((int)ps->index() > topix)
            topix = ps->index();
    }
    if (topix < 0 || curix < 0)
        return;
    int nix = curix + 1;
    while (nix <= topix) {
        for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
            if ((int)ps->index() == nix) {
                CDsterm *term = ps->cell_terminal();
                if (term) {
                    EX()->editTermsPush(term);
                    return;
                }
                break;
            }
        }
        nix++;
    }
    nix = 0;
    while (nix <= curix) {
        for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
            if ((int)ps->index() == nix) {
                CDsterm *term = ps->cell_terminal();
                if (term) {
                    EX()->editTermsPush(term);
                    return;
                }
                break;
            }
        }
        nix++;
    }
}


void
QTphysTermDlg::toindex_btn_slot()
{
    int indx = te_sb_toindex->value();
    CDp_snode *ps0 = (CDp_snode*)CurCell(Electrical)->prpty(P_NODE);
    int topix = -1;
    int curix = -1;
    for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
        if (ps->cell_terminal() == te_term) 
            curix = ps->index();
        if ((int)ps->index() > topix)
            topix = ps->index();
    }
    if (topix < 0 || curix < 0)
        return;
    if (indx == curix)
        return;
    for (CDp_snode *ps = ps0; ps; ps = ps->next()) {
        if ((int)ps->index() == indx) {
            CDsterm *term = ps->cell_terminal();
            if (term) {
                EX()->editTermsPush(term);
                return;
            }
            break;
        }
    }
    int xx, yy;
    Menu()->PointerRootLoc(&xx, &yy);
    PL()->FlashMessageHereV(xx, yy, "No terminal for index %d", indx);
}


void
QTphysTermDlg::apply_btn_slot()
{
    if (te_action) {
        unsigned int f = 0;
        if (QTdev::GetStatus(te_fixed))
            f |= TE_FIXED;
        CDp_snode *ps = te_term->node_prpty();
        if (!ps)
            return;
        TermEditInfo tinfo(Tstring(ps->term_name()), te_lname,
            ps->index(), f, true);
        (*te_action)(&tinfo, te_term);
        EX()->PopUpPhysTermEdit(0, MODE_UPD, &tinfo, 0, te_term, 0, 0);
    }
    else
        EX()->PopUpPhysTermEdit(0, MODE_OFF, 0, 0, 0, 0, 0);
}


void
QTphysTermDlg::dismiss_btn_slot()
{
    EX()->PopUpPhysTermEdit(0, MODE_OFF, 0, 0, 0, 0, 0);
}


void
QTphysTermDlg::layer_menu_slot(int i)
{
    if (i == 0) {
        // any_layer
        set_layername(0);
        return;
    }
    QByteArray lname_ba = te_layer->currentText().toLatin1();
    const char *lname = lname_ba.constData();
    set_layername(lname);
}

