
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

#include "qtmclchg.h"
#include "edit.h"
#include "edit_variables.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "modf_menu.h"

#include <QApplication>
#include <QLayout>
#include <QLabel>
#include <QGroupBox>
#include <QRadioButton>
#include <QToolButton>
#include <QPushButton>


//-----------------------------------------------------------------------------
// QTmclChangeDlg:  Dialog to control the layer change option in move/copy.
// Called from main menu: Modify/Set Lyr Chg Mode.
//
// Help system keywords used:
//  xic:mclcg

void
cEdit::PopUpLayerChangeMode(ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        MainMenu()->MenuButtonSet(MMmain, MenuMCLCG, false);
        delete QTmclChangeDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTmclChangeDlg::self())
            QTmclChangeDlg::self()->update();
        return;
    }
    if (QTmclChangeDlg::self())
        return;

    new QTmclChangeDlg;

    QTmclChangeDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTmclChangeDlg::self(),
        QTmainwin::self()->Viewport());
    QTmclChangeDlg::self()->show();
}
// End of cEdit functions.


QTmclChangeDlg *QTmclChangeDlg::instPtr;

QTmclChangeDlg::QTmclChangeDlg()
{
    instPtr = this;
    lcg_none = 0;
    lcg_cur = 0;
    lcg_all = 0;

    setWindowTitle(tr("Layer Change Mode"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(4, 4, 4, 4);
    vbox->setSpacing(4);
    vbox->setSizeConstraint(QLayout::SetFixedSize);

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
    QLabel *label = new QLabel(tr("Set layer change option for Move/Copy"));
    hb->addWidget(label);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTmclChangeDlg::help_btn_slot);

    lcg_none = new QRadioButton(tr("Don't allow layer change"));
    vbox->addWidget(lcg_none);
    connect(lcg_none, &QRadioButton::toggled,
        this, &QTmclChangeDlg::none_btn_slot);

    lcg_cur = new QRadioButton(tr(
        "Allow layer change for objects on current layer"));
    vbox->addWidget(lcg_cur);
    connect(lcg_cur, &QRadioButton::toggled,
        this, &QTmclChangeDlg::cur_btn_slot);

    lcg_all = new QRadioButton(tr(
        "Allow layer change for all objects"));
    vbox->addWidget(lcg_all);
    connect(lcg_all, &QRadioButton::toggled,
        this, &QTmclChangeDlg::all_btn_slot);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    vbox->addSpacing(4);
    vbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QTmclChangeDlg::dismiss_btn_slot);

    update();
}


QTmclChangeDlg::~QTmclChangeDlg()
{
    instPtr = 0;
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTmclChangeDlg
#include "qtinterf/qtmacos_event.h"
#endif


void
QTmclChangeDlg::update()
{
    const char *v = CDvdb()->getVariable(VA_LayerChangeMode);
    if (!v) {
        QTdev::SetStatus(lcg_none, true);
        QTdev::SetStatus(lcg_cur, false);
        QTdev::SetStatus(lcg_all, false);
    }
    else if (*v == 'a' || *v == 'A') {
        QTdev::SetStatus(lcg_none, false);
        QTdev::SetStatus(lcg_cur, false);
        QTdev::SetStatus(lcg_all, true);
    }
    else {
        QTdev::SetStatus(lcg_none, false);
        QTdev::SetStatus(lcg_cur, true);
        QTdev::SetStatus(lcg_all, false);
    }
}


void
QTmclChangeDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:mclcg"))
}


void
QTmclChangeDlg::none_btn_slot(bool status)
{
    if (status)
        CDvdb()->clearVariable(VA_LayerChangeMode);
}


void
QTmclChangeDlg::cur_btn_slot(bool status)
{
    if (status)
        CDvdb()->setVariable(VA_LayerChangeMode, "");
}


void
QTmclChangeDlg::all_btn_slot(bool status)
{
    if (status)
        CDvdb()->setVariable(VA_LayerChangeMode, "all");
}


void
QTmclChangeDlg::dismiss_btn_slot()
{
    ED()->PopUpLayerChangeMode(MODE_OFF);
}

