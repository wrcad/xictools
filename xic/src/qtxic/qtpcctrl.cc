
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

#include "qtpcctrl.h"
#include "edit.h"
#include "edit_variables.h"
#include "cvrt_variables.h"
#include "dsp_inlines.h"
#include "qtmain.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>


//--------------------------------------------------------------------
// Pop-up to control pcell abutment and similar.
//
// Help system keywords used:
//  xic:pcctl


void
cEdit::PopUpPCellCtrl(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (cPCellCtrl::self())
            cPCellCtrl::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (cPCellCtrl::self())
            cPCellCtrl::self()->update();
        return;
    }
    if (cPCellCtrl::self())
        return;

    new cPCellCtrl(caller);

    QTdev::self()->SetPopupLocation(GRloc(), cPCellCtrl::self(),
        QTmainwin::self()->Viewport());
    cPCellCtrl::self()->show();
}


const char *cPCellCtrl::pcc_abutvals[] =
{
    "Mode 0 (no auto-abutment)",
    "Mode 1 (no contact)",
    "Mode 2 (with contact)",
    0
};

cPCellCtrl *cPCellCtrl::instPtr;

cPCellCtrl::cPCellCtrl(GRobject c)
{
    instPtr = this;
    pcc_caller = c;
    pcc_abut = 0;
    pcc_hidestr = 0;
    pcc_listsm = 0;
    pcc_allwarn = 0;

    setWindowTitle(tr("PCell Control"));
    setWindowFlags(Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);
//    gtk_window_set_resizable(GTK_WINDOW(pcc_popup), false);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // label in frame plus help btn
    //
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    QLabel *label = new QLabel(tr("Control Parameterized Cell Options"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    label = new QLabel(tr("Auto-abutment mode"));
    hbox->addWidget(label);

    pcc_abut = new QComboBox();;
    hbox->addWidget(pcc_abut);
    for (int i = 0; pcc_abutvals[i]; i++)
        pcc_abut->addItem(tr(pcc_abutvals[i]));
    connect(pcc_abut, SIGNAL(currrentIndexChanged(int)),
        this, SLOT(abut_mode_slot(int)));

    pcc_hidestr = new QCheckBox(tr("Hide and disable stretch handles"));
    vbox->addWidget(pcc_hidestr);
    connect(pcc_hidestr, SIGNAL(stateChanged(int)),
        this, SLOT(hidestr_btn_slot(int)));

    hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    label = new QLabel(tr("Instance min. pixel size for stretch handles"));
    hbox->addWidget(label);

    pcc_sb_psz = new QSpinBox();
    pcc_sb_psz->setMinimum(0);
    pcc_sb_psz->setMaximum(1000);
    pcc_sb_psz->setValue(DSP_MIN_FENCE_INST_PIXELS);
    hbox->addWidget(pcc_sb_psz);
    connect(pcc_sb_psz, SIGNAL(valueChanged(int)),
        this, SLOT(psz_change_slot(int)));

    pcc_listsm = new QCheckBox(tr("List sub-masters as modified cells"));
    vbox->addWidget(pcc_listsm);
    connect(pcc_listsm, SIGNAL(stateChanged(int)),
        this, SLOT(listsm_btn_slot(int)));

    pcc_allwarn = new QCheckBox(tr("Show all evaluation warnings"));
    vbox->addWidget(pcc_allwarn);
    connect(pcc_allwarn, SIGNAL(stateChanged(int)),
        this, SLOT(allwarn_btn_slot(int)));

    // Dismiss button
    //
    btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


cPCellCtrl::~cPCellCtrl()
{
    instPtr = 0;
    if (pcc_caller)
        QTdev::Deselect(pcc_caller);
}


void
cPCellCtrl::update()
{
    const char *s = CDvdb()->getVariable(VA_PCellAbutMode);
    if (s && atoi(s) == 2)
        pcc_abut->setCurrentIndex(2);
    else if (s && atoi(s) == 0)
        pcc_abut->setCurrentIndex(0);
    else
        pcc_abut->setCurrentIndex(1);

    QTdev::SetStatus(pcc_hidestr,
        CDvdb()->getVariable(VA_PCellHideGrips));
    QTdev::SetStatus(pcc_listsm,
        CDvdb()->getVariable(VA_PCellListSubMasters));
    QTdev::SetStatus(pcc_allwarn,
        CDvdb()->getVariable(VA_PCellShowAllWarnings));

    int d;
    const char *str = CDvdb()->getVariable(VA_PCellGripInstSize);
    if (str && sscanf(str, "%d", &d) == 1 && d >= 0 && d <= 1000)
        ;
    else
        d = DSP_MIN_FENCE_INST_PIXELS;
    pcc_sb_psz->setValue(d);
}


void
cPCellCtrl::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:pcctl"))
}


void
cPCellCtrl::abut_mode_slot(int i)
{
    if (i == 0)
        CDvdb()->setVariable(VA_PCellAbutMode, "0");
    else if (i == 1)
        CDvdb()->clearVariable(VA_PCellAbutMode);
    else if (i == 2)
        CDvdb()->setVariable(VA_PCellAbutMode, "2");
}


void
cPCellCtrl::hidestr_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_PCellHideGrips, "");
    else
        CDvdb()->clearVariable(VA_PCellHideGrips);
}


void
cPCellCtrl::psz_change_slot(int d)
{
    if (d == DSP_MIN_FENCE_INST_PIXELS)
        CDvdb()->clearVariable(VA_PCellGripInstSize);
    else {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", d);
        CDvdb()->setVariable(VA_PCellGripInstSize, buf);
    }
}


void
cPCellCtrl::listsm_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_PCellListSubMasters, "");
    else
        CDvdb()->clearVariable(VA_PCellListSubMasters);
}


void
cPCellCtrl::allwarn_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_PCellShowAllWarnings, "");
    else
        CDvdb()->clearVariable(VA_PCellShowAllWarnings);
}


void
cPCellCtrl::dismiss_btn_slot()
{
    ED()->PopUpPCellCtrl(0, MODE_OFF);
}

