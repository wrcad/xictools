
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

#include "qtjoin.h"
#include "edit.h"
#include "dsp_inlines.h"
#include "geo_zlist.h"
#include "errorlog.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>


//--------------------------------------------------------------------
// Pop-up to control box/poly join operations.
//
// Help system keywords used:
//  xic:join

void
cEdit::PopUpJoin(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTjoinDlg::self())
            QTjoinDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTjoinDlg::self())
            QTjoinDlg::self()->update();
        return;
    }
    if (QTjoinDlg::self())
        return;

    new QTjoinDlg(caller);

    QTdev::self()->SetPopupLocation(GRloc(LW_UL), QTjoinDlg::self(),
        QTmainwin::self()->Viewport());
    QTjoinDlg::self()->show();
}
// End of cEdit functions.


QTjoinDlg *QTjoinDlg::instPtr;

QTjoinDlg::QTjoinDlg(GRobject c)
{
    instPtr = this;
    jn_caller = c;
    jn_nolimit = 0;
    jn_clean = 0;
    jn_wires = 0;
    jn_join = 0;
    jn_join_lyr = 0;
    jn_join_all = 0;
    jn_split_h = 0;
    jn_split_v = 0;
    jn_mverts_label = 0;
    jn_mgroup_label = 0;
    jn_mqueue_label = 0;
    jn_last_mverts = DEF_JoinMaxVerts;
    jn_last_mgroup = DEF_JoinMaxGroup;
    jn_last_mqueue = DEF_JoinMaxQueue;
    jn_last = 0;

    setWindowTitle(tr("Join or Split Objects"));
    setWindowFlags(Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_ShowWithoutActivating);
//    gtk_window_set_resizable(GTK_WINDOW(jn_popup), false);

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
    hb->setMargin(0);
    hb->setSpacing(2);
    QLabel *label = new QLabel(tr(
        "Set parameters or initiate join/split operation"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // no limit check box
    //
    jn_nolimit = new QCheckBox(tr("No limits in join operation"));
    vbox->addWidget(jn_nolimit);
    connect(jn_nolimit, SIGNAL(stateChanged(int)),
        this, SLOT(nolimit_btn_slot(int)));

    // Max Verts spin button
    //
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    jn_mverts_label = new QLabel(tr("Maximum vertices in joined polygon"));
    hbox->addWidget(jn_mverts_label);

    jn_mverts = new QSpinBox();
    hbox->addWidget(jn_mverts);
    jn_mverts->setMinimum(0);
    jn_mverts->setMaximum(8000);
    jn_mverts->setValue(0);
    jn_mverts->setMaximumWidth(140);
    connect(jn_mverts, SIGNAL(valueChanged(int)),
        this, SLOT(mverts_change_slot(int)));

    // Max Group spin button
    //
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    jn_mgroup_label = new QLabel(tr("Maximum trapezoids per poly for join"));
    hbox->addWidget(jn_mgroup_label);

    jn_mgroup = new QSpinBox();
    hbox->addWidget(jn_mgroup);
    jn_mgroup->setMinimum(0);
    jn_mgroup->setMaximum(1000000);
    jn_mgroup->setValue(0);
    jn_mgroup->setMaximumWidth(140);
    connect(jn_mgroup, SIGNAL(valueChanged(int)),
        this, SLOT(mgroup_change_slot(int)));

    // Max Queue spin button
    //
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    jn_mqueue_label = new QLabel(tr("Trapezoid queue size for join"));
    hbox->addWidget(jn_mqueue_label);

    jn_mqueue = new QSpinBox();
    hbox->addWidget(jn_mqueue);
    jn_mqueue->setMinimum(0);
    jn_mqueue->setMaximum(1000000);
    jn_mqueue->setValue(0);
    jn_mqueue->setMaximumWidth(140);
    connect(jn_mqueue, SIGNAL(valueChanged(int)),
        this, SLOT(mqueue_change_slot(int)));

    // break clean check box
    //
    jn_clean = new QCheckBox(tr("Clean break in join operation limiting"));
    vbox->addWidget(jn_clean);
    connect(jn_clean, SIGNAL(stateChanged(int)),
        this, SLOT(clean_btn_slot(int)));

    // include wires check box
    //
    jn_wires = new QCheckBox(tr("Include wires (as polygons) in join/split"));
    vbox->addWidget(jn_wires);
    connect(jn_wires, SIGNAL(stateChanged(int)),
        this, SLOT(wires_btn_slot(int)));

    // Command buttons
    //
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    jn_join = new QPushButton(tr("Join"));
    hbox->addWidget(jn_join);
    connect(jn_join, SIGNAL(clicked()), this, SLOT(join_btn_slot()));

    jn_join_lyr = new QPushButton(tr("Join Lyr"));
    hbox->addWidget(jn_join_lyr);
    connect(jn_join_lyr, SIGNAL(clicked()), this, SLOT(join_lyr_btn_slot()));

    jn_join_all = new QPushButton(tr("Join All"));
    hbox->addWidget(jn_join_all);
    connect(jn_join_all, SIGNAL(clicked()), this, SLOT(join_all_btn_slot()));

    jn_split_h = new QPushButton(tr("Split Horiz"));
    hbox->addWidget(jn_split_h);
    connect(jn_split_h, SIGNAL(clicked()), this, SLOT(split_h_btn_slot()));

    jn_split_v = new QPushButton(tr("Split Vert"));
    hbox->addWidget(jn_split_v);
    connect(jn_split_v, SIGNAL(clicked()), this, SLOT(split_v_btn_slot()));

    // Dismiss button
    //
    btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(delete_btn_slot()));

    update();
}


QTjoinDlg::~QTjoinDlg()
{
    instPtr = 0;
    if (jn_caller)
        QTdev::Deselect(jn_caller);
}


void
QTjoinDlg::update()
{
    QTdev::SetStatus(jn_clean,
        CDvdb()->getVariable(VA_JoinBreakClean) != 0);
    QTdev::SetStatus(jn_wires,
        CDvdb()->getVariable(VA_JoinSplitWires) != 0);

    if (Zlist::JoinMaxVerts || Zlist::JoinMaxGroup || Zlist::JoinMaxQueue) {
        QTdev::SetStatus(jn_nolimit, false);
        set_sens(true);

        if (jn_mverts->value() != Zlist::JoinMaxVerts)
            jn_mverts->setValue(Zlist::JoinMaxVerts);
        if (jn_mgroup->value() != Zlist::JoinMaxGroup)
            jn_mgroup->setValue(Zlist::JoinMaxGroup);
        if (jn_mqueue->value() != Zlist::JoinMaxQueue)
            jn_mqueue->setValue(Zlist::JoinMaxQueue);
    }
    else {
        QTdev::SetStatus(jn_nolimit, true);
        jn_mverts->setValue(0);
        jn_mgroup->setValue(0);
        jn_mqueue->setValue(0);
        set_sens(false);
    }
}


void
QTjoinDlg::set_sens(bool sens)
{
    jn_mverts->setEnabled(sens);
    jn_mgroup->setEnabled(sens);
    jn_mqueue->setEnabled(sens);
    jn_mverts_label->setEnabled(sens);
    jn_mgroup_label->setEnabled(sens);
    jn_mqueue_label->setEnabled(sens);
    jn_clean->setEnabled(sens);
}


void
QTjoinDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:join"))
}


void
QTjoinDlg::nolimit_btn_slot(int state)
{
    if (state) {
        jn_last_mverts = jn_mverts->value();
        jn_last_mgroup = jn_mgroup->value();
        jn_last_mqueue = jn_mqueue->value();
        if (DEF_JoinMaxVerts == 0)
            CDvdb()->clearVariable(VA_JoinMaxPolyVerts);
        else
            CDvdb()->setVariable(VA_JoinMaxPolyVerts, "0");
        if (DEF_JoinMaxGroup == 0)
            CDvdb()->clearVariable(VA_JoinMaxPolyGroup);
        else
            CDvdb()->setVariable(VA_JoinMaxPolyGroup, "0");
        if (DEF_JoinMaxQueue == 0)
            CDvdb()->clearVariable(VA_JoinMaxPolyQueue);
        else
            CDvdb()->setVariable(VA_JoinMaxPolyQueue, "0");
    }
    else {
        jn_mverts->setValue(jn_last_mverts);
        jn_mgroup->setValue(jn_last_mgroup);
        jn_mqueue->setValue(jn_last_mqueue);
    }
}


void
QTjoinDlg::mverts_change_slot(int n)
{
    // Skip over 1-19 in spin button display, these values are
    // no good.
    if (n >= 1 && n < 20) {
        if (jn_last > n)
            jn_mverts->setValue(0);
        else
            jn_mverts->setValue(20);
        return;
    }
    jn_last = n;

    if (n == DEF_JoinMaxVerts)
        CDvdb()->clearVariable(VA_JoinMaxPolyVerts);
    else {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", n);
        CDvdb()->setVariable(VA_JoinMaxPolyVerts, buf);
    }
}


void
QTjoinDlg::mgroup_change_slot(int n)
{
    if (n == DEF_JoinMaxGroup)
        CDvdb()->clearVariable(VA_JoinMaxPolyGroup);
    else {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", n);
        CDvdb()->setVariable(VA_JoinMaxPolyGroup, buf);
    }
}


void
QTjoinDlg::mqueue_change_slot(int n)
{
    if (n == DEF_JoinMaxQueue)
        CDvdb()->clearVariable(VA_JoinMaxPolyQueue);
    else {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", n);
        CDvdb()->setVariable(VA_JoinMaxPolyQueue, buf);
    }
}


void
QTjoinDlg::clean_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_JoinBreakClean, "");
    else
        CDvdb()->clearVariable(VA_JoinBreakClean);
}


void
QTjoinDlg::wires_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_JoinSplitWires, "");
    else
        CDvdb()->clearVariable(VA_JoinSplitWires);
}


void
QTjoinDlg::join_btn_slot()
{
    bool ret = ED()->joinCmd();
    if (!ret)
        Log()->PopUpErr(Errs()->get_error());
}


void
QTjoinDlg::join_lyr_btn_slot()
{
    bool ret = ED()->joinLyrCmd();
    if (!ret)
        Log()->PopUpErr(Errs()->get_error());
}


void
QTjoinDlg::join_all_btn_slot()
{
    bool ret = ED()->joinAllCmd();
    if (!ret)
        Log()->PopUpErr(Errs()->get_error());
}


void
QTjoinDlg::split_h_btn_slot()
{
    bool ret = ED()->splitCmd(false);
    if (!ret)
        Log()->PopUpErr(Errs()->get_error());
}


void
QTjoinDlg::split_v_btn_slot()
{
    bool ret = ED()->splitCmd(true);
    if (!ret)
        Log()->PopUpErr(Errs()->get_error());
}


void
QTjoinDlg::delete_btn_slot()
{
    ED()->PopUpJoin(0, MODE_OFF);
}

