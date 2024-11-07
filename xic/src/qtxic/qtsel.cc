
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

#include "qtsel.h"
#include "dsp_inlines.h"
#include "layertab.h"
#include "menu.h"
#include "select.h"
#include "qtltab.h"

#include <QApplication>
#include <QLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QToolButton>
#include <QPushButton>


//-----------------------------------------------------------------------------
// QTselectDlg:  Selection Control panel, dialog to control selections.
// Called from the selcp button in the top button menu.
//
// help system keywords used:
//  xic:layer

void
cMain::PopUpSelectControl(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTselectDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTselectDlg::self())
            QTselectDlg::self()->update();
        return;
    }
    if (QTselectDlg::self())
        return;

    new QTselectDlg(caller);

    QTselectDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTselectDlg::self(),
        QTmainwin::self()->Viewport());
    QTselectDlg::self()->show();
}
// End of cMain functions.


QTselectDlg *QTselectDlg::instPtr;

QTselectDlg::QTselectDlg(GRobject c) : QTbag(this)
{
    instPtr = this;
    sl_caller = c;
    sl_pm_norm = 0;
    sl_pm_sel = 0;
    sl_pm_mod = 0;
    sl_am_norm = 0;
    sl_am_enc = 0;
    sl_am_all = 0;
    sl_sel_norm = 0;
    sl_sel_togl = 0;
    sl_sel_add = 0;
    sl_sel_rem = 0;
    sl_cell = 0;
    sl_box = 0;
    sl_poly = 0;
    sl_wire = 0;
    sl_label = 0;
    sl_upbtn = 0;

    setWindowTitle(tr("Selection Control"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qm;
    QGridLayout *grid = new QGridLayout(this);
    grid->setContentsMargins(4, 4, 4, 4);
    grid->setSpacing(2);
    grid->setSizeConstraint(QLayout::SetFixedSize);

    // pointer mode radio group
    //
    QGroupBox *pmgb = new QGroupBox(tr("Pointer Mode"));
    grid->addWidget(pmgb, 0, 0, 1, 2);
    QVBoxLayout *vb = new QVBoxLayout(pmgb);
    vb->setContentsMargins(qm);
    vb->setSpacing(2);
    sl_pm_norm = new QRadioButton(tr("Normal"));
    vb->addWidget(sl_pm_norm);
    connect(sl_pm_norm, &QRadioButton::toggled,
        this, &QTselectDlg::pm_norm_slot);
    sl_pm_sel = new QRadioButton(tr("Select"));
    vb->addWidget(sl_pm_sel);
    connect(sl_pm_sel, &QRadioButton::toggled,
        this, &QTselectDlg::pm_sel_slot);
    sl_pm_mod = new QRadioButton(tr("Modify"));
    vb->addWidget(sl_pm_mod);
    connect(sl_pm_mod, &QRadioButton::toggled,
        this, &QTselectDlg::pm_mod_slot);

    // area mode radio group
    //
    QGroupBox *amgb = new QGroupBox(tr("Area Mode"));
    grid->addWidget(amgb, 0, 2);
    vb = new QVBoxLayout(amgb);
    vb->setContentsMargins(qm);
    vb->setSpacing(2);
    sl_am_norm = new QRadioButton(tr("Normal"));
    vb->addWidget(sl_am_norm);
    connect(sl_am_norm, &QRadioButton::toggled,
        this, &QTselectDlg::am_norm_slot);
    sl_am_enc = new QRadioButton(tr("Enclosed"));
    vb->addWidget(sl_am_enc);
    connect(sl_am_enc, &QRadioButton::toggled,
        this, &QTselectDlg::am_enc_slot);
    sl_am_all = new QRadioButton(tr("All"));
    vb->addWidget(sl_am_all);
    connect(sl_am_all, &QRadioButton::toggled,
        this, &QTselectDlg::am_all_slot);

    sl_upbtn = new QToolButton();
    sl_upbtn->setText(tr("Search Bottom to Top"));
    grid->addWidget(sl_upbtn, 1, 0, 1, 3);
    sl_upbtn->setCheckable(true);
    connect(sl_upbtn, &QAbstractButton::toggled,
        this, &QTselectDlg::up_btn_slot);

    // addition mode radio group
    //
    QGroupBox *sgb = new QGroupBox(tr("Selections"));
    grid->addWidget(sgb, 0, 3, 2, 1);
    vb = new QVBoxLayout(sgb);
    vb->setContentsMargins(qm);
    vb->setSpacing(2);
    sl_sel_norm = new QRadioButton(tr("Normal"));
    vb->addWidget(sl_sel_norm);
    connect(sl_sel_norm, &QRadioButton::toggled,
        this, &QTselectDlg::sl_norm_slot);
    sl_sel_togl = new QRadioButton(tr("Toggle"));
    vb->addWidget(sl_sel_togl);
    connect(sl_sel_togl, &QRadioButton::toggled,
        this, &QTselectDlg::sl_togl_slot);
    sl_sel_add = new QRadioButton(tr("Add"));
    vb->addWidget(sl_sel_add);
    connect(sl_sel_add, &QRadioButton::toggled,
        this, &QTselectDlg::sl_add_slot);
    sl_sel_rem = new QRadioButton(tr("Remove"));
    vb->addWidget(sl_sel_rem);
    connect(sl_sel_rem, &QRadioButton::toggled,
        this, &QTselectDlg::sl_rem_slot);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    grid->addWidget(tbtn, 2, 0);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTselectDlg::help_btn_slot);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    grid->addWidget(btn, 2, 1, 1, 3);
    connect(btn, &QAbstractButton::clicked,
        this, &QTselectDlg::dismiss_btn_slot);

#if QT_VERSION >= QT_VERSION_CHECK(6,8,0)
#define CHECK_BOX_STATE_CHANGED &QCheckBox::checkStateChanged
#else
#define CHECK_BOX_STATE_CHANGED &QCheckBox::stateChanged
#endif

    // objects group
    //
    QGroupBox *ogb = new QGroupBox(tr("Objects"));
    grid->addWidget(ogb, 0, 4, 3, 1);
    vb = new QVBoxLayout(ogb);
    vb->setContentsMargins(qm);
    vb->setSpacing(2);
    sl_cell = new QCheckBox(tr("Cells"));
    vb->addWidget(sl_cell);
    connect(sl_cell, CHECK_BOX_STATE_CHANGED,
        this, &QTselectDlg::ob_cell_slot);
    sl_box = new QCheckBox(tr("Boxes"));
    vb->addWidget(sl_box);
    connect(sl_box, CHECK_BOX_STATE_CHANGED,
        this, &QTselectDlg::ob_box_slot);
    sl_poly = new QCheckBox(tr("Polys"));
    vb->addWidget(sl_poly);
    connect(sl_poly, CHECK_BOX_STATE_CHANGED,
        this, &QTselectDlg::ob_poly_slot);
    sl_wire = new QCheckBox(tr("Wires"));
    vb->addWidget(sl_wire);
    connect(sl_wire, CHECK_BOX_STATE_CHANGED,
        this, &QTselectDlg::ob_wire_slot);
    sl_label = new QCheckBox(tr("Labels"));
    vb->addWidget(sl_label);
    connect(sl_label, CHECK_BOX_STATE_CHANGED,
        this, &QTselectDlg::ob_label_slot);

    update();
}


QTselectDlg::~QTselectDlg()
{
    instPtr = 0;
    if (sl_caller)
        QTdev::Deselect(sl_caller);
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTselectDlg
#include "qtinterf/qtmacos_event.h"
#endif


void
QTselectDlg::update()
{
    if (Selections.ptrMode() == PTRnormal) {
        QTdev::Select(sl_pm_norm);
        QTdev::Deselect(sl_pm_sel);
        QTdev::Deselect(sl_pm_mod);
    }
    else if (Selections.ptrMode() == PTRselect) {
        QTdev::Deselect(sl_pm_norm);
        QTdev::Select(sl_pm_sel);
        QTdev::Deselect(sl_pm_mod);
    }
    else if (Selections.ptrMode() == PTRmodify) {
        QTdev::Deselect(sl_pm_norm);
        QTdev::Deselect(sl_pm_sel);
        QTdev::Select(sl_pm_mod);
    }

    if (Selections.areaMode() == ASELnormal) {
        QTdev::Select(sl_am_norm);
        QTdev::Deselect(sl_am_enc);
        QTdev::Deselect(sl_am_all);
    }
    else if (Selections.areaMode() == ASELenclosed) {
        QTdev::Deselect(sl_am_norm);
        QTdev::Select(sl_am_enc);
        QTdev::Deselect(sl_am_all);
    }
    else if (Selections.areaMode() == ASELall) {
        QTdev::Deselect(sl_am_norm);
        QTdev::Deselect(sl_am_enc);
        QTdev::Select(sl_am_all);
    }

    if (Selections.selMode() == SELnormal) {
        QTdev::Select(sl_sel_norm);
        QTdev::Deselect(sl_sel_togl);
        QTdev::Deselect(sl_sel_add);
        QTdev::Deselect(sl_sel_rem);
    }
    if (Selections.selMode() == SELtoggle) {
        QTdev::Deselect(sl_sel_norm);
        QTdev::Select(sl_sel_togl);
        QTdev::Deselect(sl_sel_add);
        QTdev::Deselect(sl_sel_rem);
    }
    if (Selections.selMode() == SELselect) {
        QTdev::Deselect(sl_sel_norm);
        QTdev::Deselect(sl_sel_togl);
        QTdev::Select(sl_sel_add);
        QTdev::Deselect(sl_sel_rem);
    }
    if (Selections.selMode() == SELdesel) {
        QTdev::Deselect(sl_sel_norm);
        QTdev::Deselect(sl_sel_togl);
        QTdev::Deselect(sl_sel_add);
        QTdev::Select(sl_sel_rem);
    }

    QTdev::SetStatus(sl_cell,
        (strchr(Selections.selectTypes(), CDINSTANCE) != 0));
    QTdev::SetStatus(sl_box,
        (strchr(Selections.selectTypes(), CDBOX) != 0));
    QTdev::SetStatus(sl_poly,
        (strchr(Selections.selectTypes(), CDPOLYGON) != 0));
    QTdev::SetStatus(sl_wire,
        (strchr(Selections.selectTypes(), CDWIRE) != 0));
    QTdev::SetStatus(sl_label,
        (strchr(Selections.selectTypes(), CDLABEL) != 0));

    QTdev::SetStatus(sl_upbtn, Selections.layerSearchUp());
}


void
QTselectDlg::pm_norm_slot(bool state)
{
    if (state)
        Selections.setPtrMode(PTRnormal);
}


void
QTselectDlg::pm_sel_slot(bool state)
{
    if (state)
        Selections.setPtrMode(PTRselect);
}


void
QTselectDlg::pm_mod_slot(bool state)
{
    if (state)
        Selections.setPtrMode(PTRmodify);
}


void
QTselectDlg::am_norm_slot(bool state)
{
    if (state)
        Selections.setAreaMode(ASELnormal);
}


void
QTselectDlg::am_enc_slot(bool state)
{
    if (state)
        Selections.setAreaMode(ASELenclosed);
}


void
QTselectDlg::am_all_slot(bool state)
{
    if (state)
        Selections.setAreaMode(ASELall);
}


void
QTselectDlg::sl_norm_slot(bool state)
{
    if (state)
        Selections.setSelMode(SELnormal);
}


void
QTselectDlg::sl_togl_slot(bool state)
{
    if (state)
        Selections.setSelMode(SELtoggle);
}


void
QTselectDlg::sl_add_slot(bool state)
{
    if (state)
        Selections.setSelMode(SELselect);
}


void
QTselectDlg::sl_rem_slot(bool state)
{
    if (state)
        Selections.setSelMode(SELdesel);
}


void
QTselectDlg::ob_cell_slot(int state)
{
    Selections.setSelectType(CDINSTANCE, state);
}


void
QTselectDlg::ob_box_slot(int state)
{
    Selections.setSelectType(CDBOX, state);
}


void
QTselectDlg::ob_poly_slot(int state)
{
    Selections.setSelectType(CDPOLYGON, state);
}


void
QTselectDlg::ob_wire_slot(int state)
{
    Selections.setSelectType(CDWIRE, state);
}


void
QTselectDlg::ob_label_slot(int state)
{
    Selections.setSelectType(CDLABEL, state);
}


void
QTselectDlg::up_btn_slot(bool state)
{
    XM()->SetLayerSearchUpSelections(state);
}


void
QTselectDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:layer"))
}


void
QTselectDlg::dismiss_btn_slot()
{
    XM()->PopUpSelectControl(0, MODE_OFF);
}

