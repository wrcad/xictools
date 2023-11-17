
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

#include "qtdspwin.h"
#include "cvrt.h"
#include "qtinterf/qtdblsb.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>


//-----------------------------------------------------------------------------
//  Pop-up for the CHD Display command
//

void
cConvert::PopUpDisplayWindow(GRobject caller, ShowMode mode, const BBox *BB,
    bool(*cb)(bool, const BBox*, void*), void *arg)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTdisplayWinDlg::self())
            QTdisplayWinDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTdisplayWinDlg::self())
            QTdisplayWinDlg::self()->update(BB);
        return;
    }
    if (QTdisplayWinDlg::self())
        return;

    new QTdisplayWinDlg(caller, BB, cb, arg);

    QTdisplayWinDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTdisplayWinDlg::self(),
        QTmainwin::self()->Viewport());
    QTdisplayWinDlg::self()->show();
}
// End of cConvert functions.


QTdisplayWinDlg *QTdisplayWinDlg::instPtr;

QTdisplayWinDlg::QTdisplayWinDlg(GRobject caller, const BBox *BB,
    bool(*cb)(bool, const BBox*, void*), void *arg)
{
    instPtr = this;
    dw_caller = caller;
    dw_apply = 0;
    dw_center = 0;
    dw_sb_x = 0;
    dw_sb_y = 0;
    dw_sb_wid = 0;
    dw_window = DSP()->MainWdesc();
    dw_callback = cb;
    dw_arg = arg;

    setWindowTitle(tr("Set Display Window"));
    setAttribute(Qt::WA_DeleteOnClose);
//    gtk_window_set_resizable(GTK_WINDOW(dw_popup), false);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // label in frame
    //
    QGroupBox *gb = new QGroupBox();
    vbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    QLabel *label = new QLabel(tr("Set area to display"));
    hb->addWidget(label);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QVBoxLayout *col1 = new QVBoxLayout(this);
    hbox->addLayout(col1);
    col1->setContentsMargins(qm);
    col1->setSpacing(2);
    QVBoxLayout *col2 = new QVBoxLayout(this);
    hbox->addLayout(col2);
    col2->setContentsMargins(qm);
    col2->setSpacing(2);
    QVBoxLayout *col3 = new QVBoxLayout(this);
    hbox->addLayout(col3);
    col3->setContentsMargins(qm);
    col3->setSpacing(2);

    label = new QLabel(tr("Center X,Y"));
    col1->addWidget(label);

    int ndgt = CD()->numDigits();
    dw_sb_x = new QTdoubleSpinBox();
    dw_sb_x->setMinimum(-1e6);
    dw_sb_x->setMaximum(1e6);
    dw_sb_x->setDecimals(ndgt);
    dw_sb_x->setValue(0.0);
    col2->addWidget(dw_sb_x);
    connect(dw_sb_x, SIGNAL(valueChanged(double)),
        this, SLOT(x_value_changed(double)));

    dw_sb_y = new QTdoubleSpinBox();
    dw_sb_y->setMinimum(-1e6);
    dw_sb_y->setMaximum(1e6);
    dw_sb_y->setDecimals(ndgt);
    dw_sb_y->setValue(0.0);
    col3->addWidget(dw_sb_y);
    connect(dw_sb_y, SIGNAL(valueChanged(double)),
        this, SLOT(y_value_changed(double)));

    label = new QLabel(tr("Window Width"));
    col1->addWidget(label);

    dw_sb_wid = new QTdoubleSpinBox();
    dw_sb_wid->setMinimum(0.1);
    dw_sb_wid->setMaximum(1e6);
    dw_sb_wid->setDecimals(2);
    dw_sb_wid->setValue(100.0);
    col2->addWidget(dw_sb_wid);
    connect(dw_sb_wid, SIGNAL(valueChanged(double)),
        this, SLOT(wid_value_changed(double)));

    dw_apply = new QPushButton(tr("Apply"));
    col3->addWidget(dw_apply);
    dw_apply->setAutoDefault(false);
    connect(dw_apply, SIGNAL(clicked()), this, SLOT(apply_btn_slot()));

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    dw_center = new QPushButton(tr("Center Full View"));
    hbox->addWidget(dw_center);
    dw_center->setAutoDefault(false);
    connect(dw_center, SIGNAL(clicked()), this, SLOT(center_btn_slot()));

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update(BB);
}


QTdisplayWinDlg::~QTdisplayWinDlg()
{
    instPtr = 0;
    if (dw_caller)
        QTdev::Deselect(dw_caller);
    if (dw_callback)
        (*dw_callback)(false, 0, dw_arg);
}


void
QTdisplayWinDlg::update(const BBox *BB)
{
    if (!BB)
        return;
    int xx = (BB->left + BB->right)/2;
    int yy = (BB->bottom + BB->top)/2;
    int w = BB->width();
    dw_sb_x->setValue(MICRONS(xx));
    dw_sb_y->setValue(MICRONS(yy));
    dw_sb_wid->setValue(MICRONS(w));
}

void
QTdisplayWinDlg::apply_btn_slot()
{
    double dx = dw_sb_x->value();
    double dy = dw_sb_y->value();
    double dw = dw_sb_wid->value();

    int wid2 = abs(INTERNAL_UNITS(dw)/2);
    int xx = INTERNAL_UNITS(dx);
    int yy = INTERNAL_UNITS(dy);

    BBox BB(xx - wid2, yy, xx + wid2, yy);
    if (dw_callback && !(*dw_callback)(true, &BB, dw_arg))
        return;
    Cvt()->PopUpDisplayWindow(0, MODE_OFF, 0, 0, 0);
}


void
QTdisplayWinDlg::center_btn_slot()
{
    if (dw_callback && !(*dw_callback)(true, 0, dw_arg))
        return;
    Cvt()->PopUpDisplayWindow(0, MODE_OFF, 0, 0, 0);
}


void
QTdisplayWinDlg::dismiss_btn_slot()
{
    Cvt()->PopUpDisplayWindow(0, MODE_OFF, 0, 0, 0);
}

