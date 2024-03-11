
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

#include "qtxform.h"
#include "qtinterf/qtdblsb.h"
#include "edit.h"
#include "dsp_inlines.h"

#include <QDialog>
#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QToolButton>


//-----------------------------------------------------------------------------
// QTxformDlg:  Dialog for setting the current transform.
// Called from the xform button in the side menu.
//
// Help system keywords used:
//  xic:xform

// Magn spin button parameters.
//
#define TFM_NUMD 5
#define TFM_MIND CDMAGMIN
#define TFM_MAXD CDMAGMAX
#define TFM_DEL  CDMAGMIN

// Pop-up to allow setting the mirror, rotation angle, and
// magnification parameters.  Also provides five storage registers for
// store/recall, an "ident" button to clear everything, and a "last"
// button to recall previous values.  Call with MODE_UPD to refresh
// after mode switch.
//
void
cEdit::PopUpTransform(GRobject caller, ShowMode mode,
    bool (*callback)(const char*, bool, const char*, void*), void *arg)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTxformDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTxformDlg::self()) {
            QTxformDlg::self()->update();
            QTmainwin::self()->activateWindow();
        }
        return;
    }
    if (QTxformDlg::self())
        return;

    new QTxformDlg(caller, callback, arg);
    QTxformDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_UL), QTxformDlg::self(),
        QTmainwin::self()->Viewport());
    QTxformDlg::self()->show();
}
// End of cEdit functions.


QTxformDlg *QTxformDlg::instPtr;

QTxformDlg::QTxformDlg(GRobject c,
    bool (*callback)(const char*, bool, const char*, void*), void *arg)
{
    instPtr = this;
    tf_caller = c;
    tf_rflx = 0;
    tf_rfly = 0;
    tf_ang = 0;
    tf_mag = 0;
    tf_id = 0;
    tf_last = 0;
    tf_cancel = 0;

    tf_callback = callback;
    tf_arg = arg;

    setWindowTitle(tr("Current Transform"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;

    // Label in frame plus help btn
    //
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);
    vbox->setSizeConstraint(QLayout::SetFixedSize);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QGroupBox *gb = new QGroupBox(this);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    QLabel *lbl =
        new QLabel(tr("Set transform for new cells\nand move/copy."));
    hb->addWidget(lbl);
    hbox->addWidget(gb);

    QPushButton *hbtn = new QPushButton(tr("Help"));
    hbox->addWidget(hbtn);
    hbtn->setAutoDefault(false);
    connect(hbtn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // Rotation entry and mirror buttons
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    vbox->addLayout(hbox);

    lbl = new QLabel(tr("Angle"));
    hbox->addWidget(lbl);

    tf_ang = new QComboBox();
    hbox->addWidget(tf_ang);
    connect(tf_ang, SIGNAL(currentIndexChanged(int)),
        this, SLOT(angle_change_slot(int)));

    tf_rflx = new QCheckBox(tr("Reflect X"));
    hbox->addWidget(tf_rflx);
    connect(tf_rflx, SIGNAL(stateChanged(int)),
        this, SLOT(reflect_x_slot(int)));

    hbox->addSpacing(12);
    tf_rfly = new QCheckBox(tr("Reflect Y"));
    hbox->addWidget(tf_rfly);
    connect(tf_rfly, SIGNAL(stateChanged(int)),
        this, SLOT(reflect_y_slot(int)));


    // Magnification label and spin button.
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    vbox->addLayout(hbox);

    lbl = new QLabel(tr("Magnification"));
    hbox->addWidget(lbl);

    tf_mag = new QTdoubleSpinBox();
    tf_mag->setValue(1.0);
    tf_mag->setRange(TFM_MIND, TFM_MAXD);
    tf_mag->setDecimals(TFM_NUMD);
    hbox->addWidget(tf_mag);
    connect(tf_mag, SIGNAL(valueChanged(double)),
        this, SLOT(magnification_change_slot(double)));

    // Identity and Last buttons
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    vbox->addLayout(hbox);

    tf_id = new QPushButton(tr("Identity Transform"));
    hbox->addWidget(tf_id);
    tf_id->setAutoDefault(false);
    connect(tf_id, SIGNAL(clicked()), this, SLOT(identity_btn_slot()));

    tf_last = new QPushButton(tr("Last Transform"));
    hbox->addWidget(tf_last);
    tf_last->setAutoDefault(false);
    connect(tf_last, SIGNAL(clicked()), this, SLOT(last_btn_slot()));

    // Store buttons
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    vbox->addLayout(hbox);

    QToolButton *btn = new QToolButton();
    btn->setText(tr("Sto 1"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(store_btn_slot()));
    btn = new QToolButton();
    btn->setText(tr("Sto 2"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(store_btn_slot()));
    btn = new QToolButton();
    btn->setText(tr("Sto 3"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(store_btn_slot()));
    btn = new QToolButton();
    btn->setText(tr("Sto 4"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(store_btn_slot()));
    btn = new QToolButton();
    btn->setText(tr("Sto 5"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(store_btn_slot()));

    // Recall buttons
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    vbox->addLayout(hbox);

    btn = new QToolButton();
    btn->setText(tr("Rcl 1"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(rcl_btn_slot()));
    btn = new QToolButton();
    btn->setText(tr("Rcl 2"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(rcl_btn_slot()));
    btn = new QToolButton();
    btn->setText(tr("Rcl 3"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(rcl_btn_slot()));
    btn = new QToolButton();
    btn->setText(tr("Rcl 4"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(rcl_btn_slot()));
    btn = new QToolButton();
    btn->setText(tr("Rcl 5"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(rcl_btn_slot()));

    // Dismiss button
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    vbox->addLayout(hbox);

    tf_cancel = new QPushButton(tr("Dismiss"));
    hbox->addWidget(tf_cancel);
    connect(tf_cancel, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTxformDlg::~QTxformDlg()
{
    instPtr = 0;
    if (tf_caller)
        QTdev::Deselect(tf_caller);
    if (tf_callback)
        (*tf_callback)(0, false, 0, tf_arg);
}


void
QTxformDlg::update()
{
    QTdev::SetStatus(tf_rflx, GEO()->curTx()->reflectX());
    QTdev::SetStatus(tf_rfly, GEO()->curTx()->reflectY());
    bool has_tf = GEO()->curTx()->reflectX();
    has_tf |= GEO()->curTx()->reflectY();
    char buf[32];
    int da = DSP()->CurMode() == Physical ? 45 : 90;
    int d = 0;

    disconnect(tf_ang, SIGNAL(currentIndexChanged(int)),
        this, SLOT(angle_change_slot(int)));
    tf_ang->clear();
    while (d < 360) {
        snprintf(buf, sizeof(buf), "%d", d);
        tf_ang->addItem(tr(buf));
        d += da;
    }
    connect(tf_ang, SIGNAL(currentIndexChanged(int)),
        this, SLOT(angle_change_slot(int)));
    tf_ang->setCurrentIndex(0);
    tf_ang->setCurrentIndex(GEO()->curTx()->angle()/da);

    has_tf |= GEO()->curTx()->angle();
    if (DSP()->CurMode() == Physical) {
        tf_mag->setValue(GEO()->curTx()->magn());
        tf_mag->setEnabled(true);
        has_tf |= (GEO()->curTx()->magn() != 1.0);
    }
    else {
        tf_mag->setValue(1.0);
        tf_mag->setEnabled(false);
    }

    if (has_tf)
        tf_id->setFocus();
    else
        tf_last->setFocus();
}

void
QTxformDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:xform"))
}


void
QTxformDlg::angle_change_slot(int)
{
    if (tf_callback) {
        QByteArray ba = tf_ang->currentText().toLatin1();
        const char *t = ba.constData();
        if (t) {
            (*tf_callback)("ang", true, t, tf_arg);
        }
    }
}


void
QTxformDlg::reflect_x_slot(int state)
{
    if (tf_callback)
        (*tf_callback)("rflx", state, 0, tf_arg);
}


void
QTxformDlg::reflect_y_slot(int state)
{
    if (tf_callback)
        (*tf_callback)("rfly", state, 0, tf_arg);
}


void
QTxformDlg::magnification_change_slot(double val)
{
    if (tf_callback) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*e", TFM_NUMD, val);
            (*tf_callback)("magn", true, buf, tf_arg);
    }
}


void
QTxformDlg::identity_btn_slot()
{
    ED()->saveCurTransform(0);
    ED()->clearCurTransform();
    tf_cancel->setFocus();
}


void
QTxformDlg::last_btn_slot()
{
    ED()->recallCurTransform(0);
    tf_cancel->setFocus();
}


void
QTxformDlg::store_btn_slot()
{
    QToolButton *tb = dynamic_cast<QToolButton*>(sender());
    if (!tb)
        return;
    QString qs = tb->text();
    if (qs == "Sto 1") {
        ED()->saveCurTransform(1);
        return;
    }
    if (qs == "Sto 2") {
        ED()->saveCurTransform(2);
        return;
    }
    if (qs == "Sto 3") {
        ED()->saveCurTransform(3);
        return;
    }
    if (qs == "Sto 4") {
        ED()->saveCurTransform(4);
        return;
    }
    if (qs == "Sto 5") {
        ED()->saveCurTransform(5);
        return;
    }
}


void
QTxformDlg::rcl_btn_slot()
{
    QToolButton *tb = dynamic_cast<QToolButton*>(sender());
    if (!tb)
        return;
    QString qs = tb->text();
    if (qs == "Rcl 1") {
        ED()->saveCurTransform(0);
        ED()->recallCurTransform(1);
        return;
    }
    if (qs == "Rcl 2") {
        ED()->saveCurTransform(0);
        ED()->recallCurTransform(2);
        return;
    }
    if (qs == "Rcl 3") {
        ED()->saveCurTransform(0);
        ED()->recallCurTransform(3);
        return;
    }
    if (qs == "Rcl 4") {
        ED()->saveCurTransform(0);
        ED()->recallCurTransform(4);
        return;
    }
    if (qs == "Rcl 5") {
        ED()->saveCurTransform(0);
        ED()->recallCurTransform(5);
        return;
    }
}


void
QTxformDlg::dismiss_btn_slot()
{
    ED()->PopUpTransform(0, MODE_OFF, 0, 0);
}

