
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

#include <QLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QPushButton>

//-----------------------------------------------------------------------------
// Pop-up to control selections
//
// help system keywords used:
//  xic:layer


void
cMain::PopUpSelectControl(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTselectDlg::self())
            QTselectDlg::self()->deleteLater();
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

    QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTselectDlg::self(),
        QTmainwin::self()->Viewport());
    QTselectDlg::self()->show();
}
// End of cMain functions.


QTselectDlg *QTselectDlg::instPtr;

QTselectDlg::QTselectDlg(GRobject c)
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
    setWindowFlags(Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);
//    setAttribute(Qt::WA_ShowWithoutActivating);
//    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);


    // pointer mode radio group
    //
    QGroupBox *pmgb = new QGroupBox(tr("Pointer Mode"));
    QVBoxLayout *vb = new QVBoxLayout(pmgb);
    vb->setMargin(0);
    vb->setSpacing(2);
    sl_pm_norm = new QRadioButton(tr("Normal"));
    vb->addWidget(sl_pm_norm);
    connect(sl_pm_norm, SIGNAL(toggled(bool)),
        this, SLOT(pm_norm_slot(bool)));
    sl_pm_sel = new QRadioButton(tr("Select"));
    vb->addWidget(sl_pm_sel);
    connect(sl_pm_sel, SIGNAL(toggled(bool)),
        this, SLOT(pm_sel_slot(bool)));
    sl_pm_mod = new QRadioButton(tr("Modify"));
    vb->addWidget(sl_pm_mod);
    connect(sl_pm_mod, SIGNAL(toggled(bool)),
        this, SLOT(pm_mod_slot(bool)));

    // area mode radio group
    //
    QGroupBox *amgb = new QGroupBox(tr("Area Mode"));
    vb = new QVBoxLayout(amgb);
    vb->setMargin(0);
    vb->setSpacing(2);
    sl_am_norm = new QRadioButton(tr("Normal"));
    vb->addWidget(sl_am_norm);
    connect(sl_am_norm, SIGNAL(toggled(bool)),
        this, SLOT(am_norm_slot(bool)));
    sl_am_enc = new QRadioButton(tr("Enclosed"));
    vb->addWidget(sl_am_enc);
    connect(sl_am_enc, SIGNAL(toggled(bool)),
        this, SLOT(am_enc_slot(bool)));
    sl_am_all = new QRadioButton(tr("All"));
    vb->addWidget(sl_am_all);
    connect(sl_am_all, SIGNAL(toggled(bool)),
        this, SLOT(am_all_slot(bool)));

    // addition mode radio group
    //
    QGroupBox *sgb = new QGroupBox(tr("Selections"));
    vb = new QVBoxLayout(sgb);
    vb->setMargin(0);
    vb->setSpacing(2);
    sl_sel_norm = new QRadioButton(tr("Normal"));
    vb->addWidget(sl_sel_norm);
    connect(sl_sel_norm, SIGNAL(toggled(bool)),
        this, SLOT(sl_norm_slot(bool)));
    sl_sel_togl = new QRadioButton(tr("Toggle"));
    vb->addWidget(sl_sel_togl);
    connect(sl_sel_togl, SIGNAL(toggled(bool)),
        this, SLOT(sl_togl_slot(bool)));
    sl_sel_add = new QRadioButton(tr("Add"));
    vb->addWidget(sl_sel_add);
    connect(sl_sel_add, SIGNAL(toggled(bool)),
        this, SLOT(sl_add_slot(bool)));
    sl_sel_rem = new QRadioButton(tr("Remove"));
    vb->addWidget(sl_sel_rem);
    connect(sl_sel_rem, SIGNAL(toggled(bool)),
        this, SLOT(sl_rem_slot(bool)));

    // objects group
    //
    QGroupBox *ogb = new QGroupBox(tr("Objects"));
    vb = new QVBoxLayout(ogb);
    vb->setMargin(0);
    vb->setSpacing(2);
    sl_cell = new QCheckBox(tr("Cells"));
    vb->addWidget(sl_cell);
    connect(sl_cell, SIGNAL(stateChanged(int)),
        this, SLOT(ob_cell_slot(int)));
    sl_box = new QCheckBox(tr("Boxes"));
    vb->addWidget(sl_box);
    connect(sl_box, SIGNAL(stateChanged(int)),
        this, SLOT(ob_box_slot(int)));
    sl_poly = new QCheckBox(tr("Polys"));
    vb->addWidget(sl_poly);
    connect(sl_poly, SIGNAL(stateChanged(int)),
        this, SLOT(ob_poly_slot(int)));
    sl_wire = new QCheckBox(tr("Wires"));
    vb->addWidget(sl_wire);
    connect(sl_wire, SIGNAL(stateChanged(int)),
        this, SLOT(ob_wire_slot(int)));
    sl_label = new QCheckBox(tr("Labels"));
    vb->addWidget(sl_label);
    connect(sl_label, SIGNAL(stateChanged(int)),
        this, SLOT(ob_label_slot(int)));

    // buttons
    //
    sl_upbtn = new QPushButton(tr("Search Bottom to Top"));
    sl_upbtn->setCheckable(true);
    sl_upbtn->setAutoDefault(false);
    connect(sl_upbtn, SIGNAL(toggled(bool)), this, SLOT(up_btn_slot(bool)));

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    hbox->addWidget(pmgb);
    hbox->addWidget(amgb);

    QVBoxLayout *vbox = new QVBoxLayout();
    vbox->setMargin(0);
    vbox->setSpacing(2);
    vbox->addLayout(hbox);
    vbox->addWidget(sl_upbtn);

    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    hbox->addLayout(vbox);
    hbox->addWidget(sgb);

    vbox = new QVBoxLayout();
    vbox->setMargin(0);
    vbox->setSpacing(2);
    vbox->addLayout(hbox);

    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    QPushButton *btn = new QPushButton(tr("Help"));
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));
    hbox->addWidget(btn);
    btn = new QPushButton(tr("Dismiss"));
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));
    hbox->addWidget(btn);
    vbox->addLayout(hbox);

    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    hbox->addLayout(vbox);
    hbox->addWidget(ogb);
    setLayout(hbox);
    update();
}


QTselectDlg::~QTselectDlg()
{
    instPtr = 0;
    if (sl_caller)
        QTdev::Deselect(sl_caller);
}


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

