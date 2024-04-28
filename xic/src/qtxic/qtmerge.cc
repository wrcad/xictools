
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

#include "qtmerge.h"
#include "main.h"
#include "fio.h"
#include "fio_cvt_base.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cvrt.h"
#include "qtmain.h"
#include "qtmenu.h"

#include <QLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>

//-----------------------------------------------------------------------------
// QTmergeDlg:  Controls whether newly read cells will overwrite
// existing cells with the same name already in memory.  This appears
// whenever a potential conflict is detected, when reading in a new
// cell or hierarchy.
//
// This handles the cells one at a time.  The return value is
// 1<<Physical | 1<<Electrical where a set flag indicates overwrite. 
// Call with mode = MODE_OFF after all processing to finish up.

namespace {
    void start_modal(QDialog *w)
    {
        QTmenu::self()->SetSensGlobal(false);
        QTmenu::self()->SetModal(w);
        QTpkg::self()->SetOverrideBusy(true);
        DSPmainDraw(ShowGhost(ERASE))
    }


    void end_modal()
    {
        QTmenu::self()->SetModal(0);
        QTmenu::self()->SetSensGlobal(true);
        QTpkg::self()->SetOverrideBusy(false);
        DSPmainDraw(ShowGhost(DISPLAY))
    }
}


// MODE_ON:   Pop up the dialog, and desensitize application.
// MODE_UPD:  Delete the dialog, and sensitize application, but
//            keep MC.
// MODE_OFF:  Destroy MC, and do the MODE_UPD stuff if not done.
//
bool
cConvert::PopUpMergeControl(ShowMode mode, mitem_t *mi)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return (true);
    if (mode == MODE_OFF) {
        if (QTmergeDlg::self() && !QTmergeDlg::self()->is_hidden()) {
            if (QTdev::self()->LoopLevel() > 1)
                QTdev::self()->BreakLoop();
            end_modal();
        }
        delete QTmergeDlg::self();
        return (true);
    }
    if (mode == MODE_UPD) {
        // We get here when the user presses "apply to all".  Hide the
        // widget and break out of modality.

        if (QTmergeDlg::self() && QTmergeDlg::self()->set_apply_to_all()) {
            if (QTdev::self()->LoopLevel() > 1)
                QTdev::self()->BreakLoop();
            end_modal();
        }
        return (true);
    }
    if (QTmergeDlg::self()) {
        if (QTmergeDlg::self()->refresh(mi))
            return (true);
    }
    else {
        if (!FIO()->IsMergeControlEnabled())
            // If the popup is not enabled, return the default.
            return (true);

        new QTmergeDlg(mi);
        if (QTmergeDlg::self()->is_hidden())
            return (true);

        QTmergeDlg::self()->set_transient_for(QTmainwin::self());
        QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTmergeDlg::self(),
            QTmainwin::self()->Viewport());
        start_modal(QTmergeDlg::self());
        QTmergeDlg::self()->show();
    }

    QTdev::self()->MainLoop();  // Wait for user's response.

    if (QTmergeDlg::self())
        QTmergeDlg::self()->query(mi);
    return (true);
}
// End of cConvert functions.


QTmergeDlg *QTmergeDlg::instPtr;

QTmergeDlg::QTmergeDlg(mitem_t *mi)
{
    instPtr = this;
    mc_label = 0;
    mc_ophys = 0;
    mc_oelec = 0;
    mc_names = new SymTab(true, false);
    mc_allflag = false;
    mc_do_phys = false;
    mc_do_elec = false;

    setWindowTitle(tr("Symbol Merge"));
    setAttribute(Qt::WA_DeleteOnClose);

    mc_names->add(lstring::copy(mi->name), (void*)1, false);

    mc_do_phys = !FIO()->IsNoOverwritePhys();
    mc_do_elec = !FIO()->IsNoOverwriteElec();
    if (CDvdb()->getVariable(VA_NoAskOverwrite) ||
            XM()->RunMode() != ModeNormal) {
        mc_allflag = true;
        query(mi);
        return;
    }

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);
    vbox->setSizeConstraint(QLayout::SetFixedSize);

    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    mc_label = new QLabel();
    hb->addWidget(mc_label);
    char buf[256];
    snprintf(buf, sizeof(buf), "Cell: %s", mi->name);
    mc_label->setText(tr(buf));

    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    
    QCheckBox *cb = new QCheckBox(tr("Overwrite Physical"));
    hbox->addWidget(cb);
    connect(cb, SIGNAL(stateChanged(int)),
        this, SLOT(phys_check_box_slot(int)));
    cb->setChecked(mc_do_elec);

    hbox = new QHBoxLayout(0);
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    cb = new QCheckBox(tr("Overwrite Electrical"));
    hbox->addWidget(cb);
    connect(cb, SIGNAL(stateChanged(int)),
        this, SLOT(elec_check_box_slot(int)));
    cb->setChecked(mc_do_elec);

    hbox = new QHBoxLayout(0);
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    QPushButton *btn = new QPushButton(tr("Apply"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(apply_btn_slot()));

    btn = new QPushButton(tr("Apply To Rest"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(apply_to_all_btn_slot()));
}


QTmergeDlg::~QTmergeDlg()
{
    instPtr = 0;
    delete mc_names;
}


void
QTmergeDlg::closeEvent(QCloseEvent *ev)
{
    // Closing the window is the same as pressing ...
    Cvt()->PopUpMergeControl(MODE_UPD, 0);
    ev->ignore();  // equiv. to Apply to All
    //ev->accept(); // equiv. to Apply
}


void
QTmergeDlg::query(mitem_t *mi)
{
    mi->overwrite_phys = mc_do_phys;
    mi->overwrite_elec = mc_do_elec;
}


bool
QTmergeDlg::set_apply_to_all()
{
    if (!mc_allflag) {
        mc_allflag = true;
        hide();
        return (true);
    }
    return (false);
}


bool
QTmergeDlg::refresh(mitem_t *mi)
{
    if (SymTab::get(mc_names, mi->name) != ST_NIL) {
        // We've seen this cell before.
        mi->overwrite_phys = false;
        mi->overwrite_elec = false;
        return (true);
    }
    mc_names->add(lstring::copy(mi->name), (void*)1, false);
    if (mc_allflag) {
        query(mi);
        return (true);
    }
    char buf[256];
    snprintf(buf, sizeof(buf), "Cell: %s", mi->name);
    mc_label->setText(tr(buf));
    return (false);
}


void
QTmergeDlg::apply_btn_slot()
{
    if (QTdev::self()->LoopLevel() > 1)
        QTdev::self()->BreakLoop();
}


void
QTmergeDlg::apply_to_all_btn_slot()
{
    Cvt()->PopUpMergeControl(MODE_UPD, 0);
}


void
QTmergeDlg::phys_check_box_slot(int checked)
{
    mc_do_phys = checked;
}


void
QTmergeDlg::elec_check_box_slot(int checked)
{
    mc_do_elec = checked;
}

