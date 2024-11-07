
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

#include "qtasm.h"
#include "cvrt.h"
#include "qtinterf/qtdblsb.h"

#include <QLayout>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>


//-----------------------------------------------------------------------------
// QTasmTf: Instance transform entry class for the QTasmDlg.

QTasmTf::QTasmTf(QTasmPage *src)
{
    tf_owner = src;
    tf_pxy_label = 0;
    tf_sb_placement_x = 0;
    tf_sb_placement_y = 0;
    tf_ang_label = 0;
    tf_angle = 0;
    tf_mirror = 0;
    tf_mag_label = 0;
    tf_sb_magnification = 0;
    tf_name_label = 0;
    tf_name = 0;
    tf_use_win = 0;
    tf_do_clip = 0;
    tf_do_flatn = 0;
    tf_ecf_label = 0;
    tf_ecf_pre = 0;
    tf_ecf_post = 0;
    tf_lb_label = 0;
    tf_sb_win_l = 0;
    tf_sb_win_b = 0;
    tf_rt_label = 0;
    tf_sb_win_r = 0;
    tf_sb_win_t = 0;
    tf_sc_label = 0;
    tf_sb_scale = 0;
    tf_no_hier = 0;
    tf_angle_ix = 0;

    // the "Basic" page
    //
    QWidget *page = new QWidget();
    QGridLayout *grid = new QGridLayout(page);
    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);
    insertTab(0, page, tr("Basic"));

    // placement x, y
    //
    int ndgt = CD()->numDigits();

    tf_pxy_label = new QLabel(tr("Placement X,Y"));
    tf_pxy_label->setAlignment(Qt::AlignCenter);
    grid->addWidget(tf_pxy_label, 0, 0);

    tf_sb_placement_x = new QTdoubleSpinBox();
    tf_sb_placement_x->setRange(-1e6, 1e6);
    tf_sb_placement_x->setDecimals(ndgt);
    tf_sb_placement_x->setValue(0.0);
    grid->addWidget(tf_sb_placement_x, 0, 1);

    tf_sb_placement_y = new QTdoubleSpinBox();
    tf_sb_placement_y->setRange(-1e6, 1e6);
    tf_sb_placement_y->setDecimals(ndgt);
    tf_sb_placement_y->setValue(0.0);
    grid->addWidget(tf_sb_placement_y, 0, 2);

    // rotation and mirror
    //
    tf_ang_label = new QLabel(tr("Rotation Angle"));
    tf_ang_label->setAlignment(Qt::AlignCenter);
    grid->addWidget(tf_ang_label, 1, 0);

    tf_angle = new QComboBox();
    grid->addWidget(tf_angle, 1, 1);

    tf_angle_ix = 0;
    for (int i = 0; i < 8; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", i*45);
        tf_angle->addItem(buf);
    }
    tf_angle->setCurrentIndex(tf_angle_ix);
    connect(tf_angle, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &QTasmTf::angle_changed_slot);

    tf_mirror = new QCheckBox(tr("Mirror-Y"));
    grid->addWidget(tf_mirror, 1, 2);

    // magnification
    //
    tf_mag_label = new QLabel(tr("Magnification"));
    tf_mag_label->setAlignment(Qt::AlignCenter);
    grid->addWidget(tf_mag_label, 2, 0);

    tf_sb_magnification = new QTdoubleSpinBox();
    tf_sb_magnification->setRange(CDMAGMIN, CDMAGMAX);
    tf_sb_magnification->setDecimals(ASM_NUMD);
    tf_sb_magnification->setValue(1.0);
    grid->addWidget(tf_sb_magnification, 2, 1);

    // placement name
    //
    tf_name_label = new QLabel(tr("Placement Name"));
    tf_name_label->setAlignment(Qt::AlignCenter);
    grid->addWidget(tf_name_label, 3, 0);

    tf_name = new QLineEdit();
    grid->addWidget(tf_name, 3, 1, 1, 2);

    // the "Advanced" page
    //
    page = new QWidget();
    grid = new QGridLayout(page);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);
    insertTab(1, page, tr("Advanced"));

    // window, clip, flatten check boxes
    //
    QHBoxLayout *hbox = new QHBoxLayout();
    grid->addLayout(hbox, 0, 0, 1, 4);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

#if QT_VERSION >= QT_VERSION_CHECK(6,8,0)
#define CHECK_BOX_STATE_CHANGED &QCheckBox::checkStateChanged
#else
#define CHECK_BOX_STATE_CHANGED &QCheckBox::stateChanged
#endif

    tf_use_win = new QCheckBox(tr("Use Window"));
    hbox->addWidget(tf_use_win);
    connect(tf_use_win, CHECK_BOX_STATE_CHANGED,
        this, &QTasmTf::usew_btn_slot);

    tf_do_clip = new QCheckBox(tr("Clip"));
    hbox->addWidget(tf_do_clip);

    tf_do_flatn = new QCheckBox(tr("Flatten"));
    hbox->addWidget(tf_do_flatn);

    tf_ecf_label = new QLabel(tr("Empty Cell Filter"));
    tf_ecf_label->setAlignment(Qt::AlignCenter);
    hbox->addWidget(tf_ecf_label);

    // window coordinate entries
    //
    tf_lb_label = new QLabel(tr("Left, Bottom"));
    tf_lb_label->setAlignment(Qt::AlignCenter);
    grid->addWidget(tf_lb_label, 1, 0);

    tf_sb_win_l = new QTdoubleSpinBox();
    tf_sb_win_l->setRange(-1e6, 1e6);
    tf_sb_win_l->setDecimals(ndgt);
    tf_sb_win_l->setValue(0.0);
    grid->addWidget(tf_sb_win_l, 1, 1);

    tf_sb_win_b = new QTdoubleSpinBox();
    tf_sb_win_b->setRange(-1e6, 1e6);
    tf_sb_win_b->setDecimals(ndgt);
    tf_sb_win_b->setValue(0.0);
    grid->addWidget(tf_sb_win_b, 1, 2);

    tf_ecf_pre = new QCheckBox(tr("pre-filt"));
    grid->addWidget(tf_ecf_pre, 1, 3);

    tf_rt_label = new QLabel(tr("Right, Top"));
    tf_rt_label->setAlignment(Qt::AlignRight);
    grid->addWidget(tf_rt_label, 2, 0);

    tf_sb_win_r = new QTdoubleSpinBox();
    tf_sb_win_r->setRange(-1e6, 1e6);
    tf_sb_win_r->setDecimals(ndgt);
    tf_sb_win_r->setValue(0.0);
    grid->addWidget(tf_sb_win_r, 2, 1);

    tf_sb_win_t = new QTdoubleSpinBox();
    tf_sb_win_t->setRange(-1e6, 1e6);
    tf_sb_win_t->setDecimals(ndgt);
    tf_sb_win_t->setValue(0.0);
    grid->addWidget(tf_sb_win_t, 2, 2);

    tf_ecf_post = new QCheckBox(tr("post-filt"));
    grid->addWidget(tf_ecf_post, 2, 3);

    // scale factor and no hierarchy entries
    //
    tf_sc_label = new QLabel(tr("Scale Fct"));
    tf_sc_label->setAlignment(Qt::AlignCenter);
    grid->addWidget(tf_sc_label, 3, 0);

    tf_sb_scale = new QTdoubleSpinBox();
    tf_sb_scale->setRange(CDSCALEMIN, CDSCALEMAX);
    tf_sb_scale->setDecimals(ASM_NUMD);
    tf_sb_scale->setValue(1.0);
    grid->addWidget(tf_sb_scale, 3, 1);

    tf_no_hier = new QCheckBox(tr("No Hierarchy"));
    grid->addWidget(tf_no_hier, 3, 2);
}


QTasmTf::~QTasmTf()
{
}


// Enable or gray the transformation entries.
//
void
QTasmTf::set_sens(bool has_selection, bool has_toplev)
{
    bool sens = has_selection && has_toplev;
    tf_sb_placement_x->setEnabled(sens);
    tf_sb_placement_y->setEnabled(sens);
    tf_pxy_label->setEnabled(sens);
    tf_angle->setEnabled(sens);
    tf_ang_label->setEnabled(sens);
    tf_mirror->setEnabled(sens);
    tf_sb_magnification->setEnabled(sens);
    tf_mag_label->setEnabled(sens);

    sens = has_selection;
    tf_name_label->setEnabled(sens);
    tf_name->setEnabled(sens);
    tf_use_win->setEnabled(sens);
    tf_do_flatn->setEnabled(sens);
    tf_ecf_label->setEnabled(sens);
    tf_ecf_pre->setEnabled(sens);
    tf_ecf_post->setEnabled(sens);
    tf_no_hier->setEnabled(sens);
    tf_sc_label->setEnabled(sens);
    tf_sb_scale->setEnabled(sens);
    if (sens)
        sens = QTdev::GetStatus(tf_use_win);
    tf_do_clip->setEnabled(sens);
    tf_sb_win_l->setEnabled(sens);
    tf_sb_win_b->setEnabled(sens);
    tf_sb_win_r->setEnabled(sens);
    tf_sb_win_t->setEnabled(sens);
    tf_lb_label->setEnabled(sens);
    tf_rt_label->setEnabled(sens);
}


// Reset the values of the transformation entries to defaults.
//
void
QTasmTf::reset()
{
    tf_sb_placement_x->setValue(0.0);
    tf_sb_placement_y->setValue(0.0);
    tf_angle->setCurrentIndex(0);
    QTdev::Deselect(tf_mirror);
    tf_sb_magnification->setValue(1.0);
    QTdev::Deselect(tf_ecf_pre);
    QTdev::Deselect(tf_ecf_post);
    QTdev::Deselect(tf_use_win);
    QTdev::Deselect(tf_do_clip);
    QTdev::Deselect(tf_do_flatn);
    QTdev::Deselect(tf_no_hier);
    tf_sb_win_l->setValue(0.0);
    tf_sb_win_b->setValue(0.0);
    tf_sb_win_r->setValue(0.0);
    tf_sb_win_t->setValue(0.0);
    tf_sb_scale->setValue(1.0);
    tf_name->clear();
}


void
QTasmTf::get_tx_params(tlinfo *tl)
{
    if (!tl)
        return;
    tl->x = INTERNAL_UNITS(tf_sb_placement_x->value());
    tl->y = INTERNAL_UNITS(tf_sb_placement_y->value());
    tl->angle = 45*tf_angle_ix;
    tl->magn = tf_sb_magnification->value();
    if (tl->magn < CDSCALEMIN || tl->magn > CDSCALEMAX)
        tl->magn = 1.0;
    tl->scale = tf_sb_scale->value();
    if (tl->scale < CDSCALEMIN || tl->scale > CDSCALEMAX)
        tl->scale = 1.0;
    tl->mirror_y = QTdev::GetStatus(tf_mirror);
    tl->ecf_level = ECFnone;
    if (QTdev::GetStatus(tf_ecf_pre)) {
        if (QTdev::GetStatus(tf_ecf_post))
            tl->ecf_level = ECFall;
        else
            tl->ecf_level = ECFpre;
    }
    else if (QTdev::GetStatus(tf_ecf_post))
        tl->ecf_level = ECFpost;
    tl->use_win = QTdev::GetStatus(tf_use_win);
    tl->clip = QTdev::GetStatus(tf_do_clip);
    tl->flatten = QTdev::GetStatus(tf_do_flatn);
    tl->no_hier = QTdev::GetStatus(tf_no_hier);
    tl->winBB.left = INTERNAL_UNITS(tf_sb_win_l->value());
    tl->winBB.bottom = INTERNAL_UNITS(tf_sb_win_b->value());
    tl->winBB.right = INTERNAL_UNITS(tf_sb_win_r->value());
    tl->winBB.top = INTERNAL_UNITS(tf_sb_win_t->value());
    delete [] tl->placename;
    tl->placename = 0;
    QByteArray name_ba = tf_name->text().trimmed().toLatin1();
    const char *s = name_ba.constData();
    if (s)
        tl->placename = lstring::copy(s);
}


void
QTasmTf::set_tx_params(tlinfo *tl)
{
    if (!tl)
        return;
    int ndgt = CD()->numDigits();
    tf_sb_placement_x->setDecimals(ndgt);
    tf_sb_placement_x->setValue(MICRONS(tl->x));
    tf_sb_placement_y->setDecimals(ndgt);
    tf_sb_placement_y->setValue(MICRONS(tl->y));
    tf_angle_ix = tl->angle/45;
    tf_angle->setCurrentIndex(tf_angle_ix);
    tf_sb_magnification->setValue(tl->magn);
    tf_sb_scale->setValue(tl->scale);
    QTdev::SetStatus(tf_mirror, tl->mirror_y);
    QTdev::SetStatus(tf_ecf_pre, tl->ecf_level == ECFall ||
        tl->ecf_level == ECFpre);
    QTdev::SetStatus(tf_ecf_post, tl->ecf_level == ECFall ||
        tl->ecf_level == ECFpost);
    QTdev::SetStatus(tf_use_win, tl->use_win);
    QTdev::SetStatus(tf_do_clip, tl->clip);
    QTdev::SetStatus(tf_do_flatn, tl->flatten);
    QTdev::SetStatus(tf_no_hier, tl->no_hier);
    tf_sb_win_l->setDecimals(ndgt);
    tf_sb_win_l->setValue(MICRONS(tl->winBB.left));
    tf_sb_win_b->setDecimals(ndgt);
    tf_sb_win_b->setValue(MICRONS(tl->winBB.bottom));
    tf_sb_win_r->setDecimals(ndgt);
    tf_sb_win_r->setValue(MICRONS(tl->winBB.right));
    tf_sb_win_t->setDecimals(ndgt);
    tf_sb_win_t->setValue(MICRONS(tl->winBB.top));
    tf_name->setText(tl->placename ? tl->placename : "");
}


void
QTasmTf::angle_changed_slot(int ix)
{
    tf_angle_ix = ix;
}


void
QTasmTf::usew_btn_slot(int)
{
    if (tf_owner)
        tf_owner->upd_sens();
}

