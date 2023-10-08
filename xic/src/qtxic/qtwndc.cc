
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

#include <qtwndc.h>
#include "fio.h"

#include <QLayout>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QDoubleSpinBox>


//-------------------------------------------------------------------------
// Subwidget group for window control.

QTwindowCfg::QTwindowCfg(WndSensMode(sens_test)(), WndFuncMode fmode)
{
    wnd_sens_test = sens_test;
    wnd_func_mode = fmode;

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(16);

    wnd_use_win = new QCheckBox(tr("Use Window"));
    hbox->addWidget(wnd_use_win);
    connect(wnd_use_win, SIGNAL(stateChanged(int)),
        this, SLOT(usewin_btn_slot(int)));

    wnd_clip = new QCheckBox(tr("Clip to Window"));
    hbox->addWidget(wnd_clip);
    connect(wnd_clip, SIGNAL(stateChanged(int)),
        this, SLOT(clip_btn_slot(int)));

    wnd_flatten = new QCheckBox(tr("Flatten Hierarchy"));
    hbox->addWidget(wnd_flatten);
    connect(wnd_flatten, SIGNAL(stateChanged(int)),
        this, SLOT(flatten_btn_slot(int)));

    wnd_ecf_label = new QLabel(tr("Empty Cell Filter"));
    hbox->addWidget(wnd_ecf_label);
    if (wnd_func_mode == WndFuncOut || wnd_func_mode == WndFuncIn)
        wnd_ecf_label->hide();;

    // two rows, six cols
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    QVBoxLayout *col = new QVBoxLayout();
    hbox->addLayout(col);

    wnd_sbutton = new QPushButton("S");
    col->addWidget(wnd_sbutton);

    wnd_s_menu = new QMenu();
    wnd_sbutton->setMenu(wnd_s_menu);

    char buf[64];
    for (int i = 0; i < FIO_NUM_BB_STORE; i++) {
        snprintf(buf, sizeof(buf), "Reg %d", i);
        QAction *a = wnd_s_menu->addAction(buf);
        a->setData(i);
    }
    connect(wnd_s_menu, SIGNAL(triggered(QAction*)),
        this, SLOT(s_menu_slot(QAction*)));

    wnd_rbutton = new QPushButton("R");
    col->addWidget(wnd_rbutton);

    wnd_r_menu = new QMenu();
    wnd_rbutton->setMenu(wnd_s_menu);

    wnd_r_menu = new QMenu();
    for (int i = 0; i < FIO_NUM_BB_STORE; i++) {
        snprintf(buf, sizeof(buf), "Reg %d", i);
        QAction *a = wnd_r_menu->addAction(buf);
        a->setData(i);
    }
    connect(wnd_r_menu, SIGNAL(triggered(QAction*)),
        this, SLOT(r_menu_slot(QAction*)));

    col = new QVBoxLayout();
    hbox->addLayout(col);

    wnd_l_label = new QLabel(tr("Left"));
    col->addWidget(wnd_l_label);

    wnd_r_label = new QLabel(tr("Right"));
    col->addWidget(wnd_r_label);

    col = new QVBoxLayout();
    hbox->addLayout(col);

    wnd_sb_left = new QDoubleSpinBox();
    col->addWidget(wnd_sb_left);
    wnd_sb_left->setMinimum(1e-6);
    wnd_sb_left->setMaximum(1e6);

    int ndgt = CD()->numDigits();
    wnd_sb_left->setDecimals(ndgt);

    double initd = 0;
    if (wnd_func_mode == WndFuncCvt)
        initd = MICRONS(FIO()->CvtWindow()->left);
    else if (wnd_func_mode == WndFuncOut)
        initd = MICRONS(FIO()->OutWindow()->left);
    else if (wnd_func_mode == WndFuncIn)
        initd = MICRONS(FIO()->InWindow()->left);
    wnd_sb_left->setValue(initd);
    connect(wnd_sb_left, SIGNAL(valueChanged(double)),
        this, SLOT(left_value_slot(double)));

    wnd_sb_right = new QDoubleSpinBox();
    col->addWidget(wnd_sb_right);
    wnd_sb_right->setMinimum(1e-6);
    wnd_sb_right->setMaximum(1e6);
    wnd_sb_right->setDecimals(ndgt);

    initd = 0;
    if (wnd_func_mode == WndFuncCvt)
        initd = MICRONS(FIO()->CvtWindow()->right);
    else if (wnd_func_mode == WndFuncOut)
        initd = MICRONS(FIO()->OutWindow()->right);
    else if (wnd_func_mode == WndFuncIn)
        initd = MICRONS(FIO()->InWindow()->right);
    wnd_sb_right->setValue(initd);
    connect(wnd_sb_right, SIGNAL(valueChanged(double)),
        this, SLOT(right_value_slot(double)));

    col = new QVBoxLayout();
    hbox->addLayout(col);

    wnd_b_label = new QLabel(tr("Bottom"));
    col->addWidget(wnd_b_label);

    wnd_t_label = new QLabel(tr("Top"));
    col->addWidget(wnd_t_label);

    col = new QVBoxLayout();
    hbox->addLayout(col);

    wnd_sb_bottom = new QDoubleSpinBox();
    col->addWidget(wnd_sb_bottom);
    wnd_sb_bottom->setMinimum(1e-6);
    wnd_sb_bottom->setMaximum(1e6);
    wnd_sb_bottom->setDecimals(ndgt);

    initd = 0;
    if (wnd_func_mode == WndFuncCvt)
        initd = MICRONS(FIO()->CvtWindow()->bottom);
    else if (wnd_func_mode == WndFuncOut)
        initd = MICRONS(FIO()->OutWindow()->bottom);
    else if (wnd_func_mode == WndFuncIn)
        initd = MICRONS(FIO()->InWindow()->bottom);
    wnd_sb_bottom->setValue(initd);
    connect(wnd_sb_bottom, SIGNAL(valueChanged(double)),
        this, SLOT(bottom_value_slot(double)));

    wnd_sb_top = new QDoubleSpinBox();
    col->addWidget(wnd_sb_top);
    wnd_sb_top->setMinimum(1e-6);
    wnd_sb_top->setMaximum(1e6);
    wnd_sb_top->setDecimals(ndgt);

    initd = 0;
    if (wnd_func_mode == WndFuncCvt)
        initd = MICRONS(FIO()->CvtWindow()->top);
    else if (wnd_func_mode == WndFuncOut)
        initd = MICRONS(FIO()->OutWindow()->top);
    else if (wnd_func_mode == WndFuncIn)
        initd = MICRONS(FIO()->InWindow()->top);
    wnd_sb_top->setValue(initd);
    connect(wnd_sb_top, SIGNAL(valueChanged(double)),
        this, SLOT(top_value_slot(double)));

    col = new QVBoxLayout();
    hbox->addLayout(col);

    wnd_ecf_pre = new QCheckBox(tr("pre-filter"));
    col->addWidget(wnd_ecf_pre);
    connect(wnd_ecf_pre, SIGNAL(stateChanged(int)),
        this, SLOT(ecf_pre_btn_slot(int)));
    if (wnd_func_mode == WndFuncOut || wnd_func_mode == WndFuncIn)
        wnd_ecf_pre->hide();

    wnd_ecf_post = new QCheckBox(tr("post-filter"));
    col->addWidget(wnd_ecf_post);
    connect(wnd_ecf_post, SIGNAL(stateChanged(int)),
        this, SLOT(ecf_post_btn_slot(int)));
    if (wnd_func_mode == WndFuncOut || wnd_func_mode == WndFuncIn)
        wnd_ecf_post->hide();

    update();
}


QTwindowCfg::~QTwindowCfg()
{
}


void
QTwindowCfg::update()
{
    if (wnd_func_mode == WndFuncCvt) {
        wnd_sb_left->setValue(MICRONS(FIO()->CvtWindow()->left));
        wnd_sb_bottom->setValue(MICRONS(FIO()->CvtWindow()->bottom));
        wnd_sb_right->setValue(MICRONS(FIO()->CvtWindow()->right));
        wnd_sb_top->setValue(MICRONS(FIO()->CvtWindow()->top));
        QTdev::SetStatus(wnd_use_win, FIO()->CvtUseWindow());
        QTdev::SetStatus(wnd_clip, FIO()->CvtClip());
        QTdev::SetStatus(wnd_flatten, FIO()->CvtFlatten());
        QTdev::SetStatus(wnd_ecf_pre,
            FIO()->CvtECFlevel() == ECFall || FIO()->CvtECFlevel() == ECFpre);
        QTdev::SetStatus(wnd_ecf_post,
            FIO()->CvtECFlevel() == ECFall || FIO()->CvtECFlevel() == ECFpost);
    }
    else if (wnd_func_mode == WndFuncOut) {
        wnd_sb_left->setValue(MICRONS(FIO()->OutWindow()->left));
        wnd_sb_bottom->setValue(MICRONS(FIO()->OutWindow()->bottom));
        wnd_sb_right->setValue(MICRONS(FIO()->OutWindow()->right));
        wnd_sb_top->setValue(MICRONS(FIO()->OutWindow()->top));
        QTdev::SetStatus(wnd_use_win, FIO()->OutUseWindow());
        QTdev::SetStatus(wnd_clip, FIO()->OutClip());
        QTdev::SetStatus(wnd_flatten, FIO()->OutFlatten());
        QTdev::SetStatus(wnd_ecf_pre,
            FIO()->OutECFlevel() == ECFall || FIO()->OutECFlevel() == ECFpre);
        QTdev::SetStatus(wnd_ecf_post,
            FIO()->OutECFlevel() == ECFall || FIO()->OutECFlevel() == ECFpost);
    }
    else if (wnd_func_mode == WndFuncIn) {
        wnd_sb_left->setValue(MICRONS(FIO()->InWindow()->left));
        wnd_sb_bottom->setValue(MICRONS(FIO()->InWindow()->bottom));
        wnd_sb_right->setValue(MICRONS(FIO()->InWindow()->right));
        wnd_sb_top->setValue(MICRONS(FIO()->InWindow()->top));
        QTdev::SetStatus(wnd_use_win, FIO()->InUseWindow());
        QTdev::SetStatus(wnd_clip, FIO()->InClip());
        QTdev::SetStatus(wnd_flatten, FIO()->InFlatten());
        QTdev::SetStatus(wnd_ecf_pre,
            FIO()->InECFlevel() == ECFall || FIO()->InECFlevel() == ECFpre);
        QTdev::SetStatus(wnd_ecf_post,
            FIO()->InECFlevel() == ECFall || FIO()->InECFlevel() == ECFpost);
    }
    set_sens();
}


void
QTwindowCfg::set_sens()
{
    WndSensMode mode = (wnd_sens_test ? (*wnd_sens_test)() : WndSensAllModes);

    // No Empties is visible only when translating.

    if (mode == WndSensAllModes) {
        // Basic mode when translating archive to archive.
        // Flattten, UseWindow are independent.
        // NoEmpties available when not flattening.
        // Clip is enabled by UseWindow.

        bool u = QTdev::GetStatus(wnd_use_win);
        if (!u)
            QTdev::SetStatus(wnd_clip, false);
        bool f = QTdev::GetStatus(wnd_flatten);
        if (f) {
            QTdev::SetStatus(wnd_ecf_pre, false);
            QTdev::SetStatus(wnd_ecf_post, false);
        }
        wnd_use_win->setEnabled(true);
        wnd_flatten->setEnabled(true);
        wnd_ecf_label->setEnabled(!f);
        wnd_ecf_pre->setEnabled(!f);
        wnd_ecf_post->setEnabled(!f);
        wnd_clip->setEnabled(u);
        wnd_sb_left->setEnabled(u);
        wnd_sb_bottom->setEnabled(u);
        wnd_sb_right->setEnabled(u);
        wnd_sb_top->setEnabled(u);
        wnd_l_label->setEnabled(u);
        wnd_b_label->setEnabled(u);
        wnd_r_label->setEnabled(u);
        wnd_t_label->setEnabled(u);
        wnd_rbutton->setEnabled(u);
        wnd_sbutton->setEnabled(u);
    }
    else if (mode == WndSensFlatten) {
        // Basic mode when writing database to archive or native, and
        // writing CHD to native.
        // UseWindow is available only when flattening.
        // NoEmpties available when not flattening.
        // Clip is enabled by UseWindow.

        bool f = QTdev::GetStatus(wnd_flatten);
        if (!f)
            QTdev::SetStatus(wnd_use_win, false);
        else {
            QTdev::SetStatus(wnd_ecf_pre, false);
            QTdev::SetStatus(wnd_ecf_post, false);
        }
        bool u = QTdev::GetStatus(wnd_use_win);
        if (!u)
            QTdev::SetStatus(wnd_clip, false);
        wnd_flatten->setEnabled(true);
        wnd_ecf_label->setEnabled(!f);
        wnd_ecf_pre->setEnabled(!f);
        wnd_ecf_post->setEnabled(!f);
        wnd_use_win->setEnabled(f);
        wnd_clip->setEnabled(f && u);
        wnd_sb_left->setEnabled(f && u);
        wnd_sb_bottom->setEnabled(f && u);
        wnd_sb_right->setEnabled(f && u);
        wnd_sb_top->setEnabled(f && u);
        wnd_l_label->setEnabled(f && u);
        wnd_b_label->setEnabled(f && u);
        wnd_r_label->setEnabled(f && u);
        wnd_t_label->setEnabled(f && u);
        wnd_rbutton->setEnabled(f && u);
        wnd_sbutton->setEnabled(f && u);
    }
    else if (mode == WndSensNone) {
        // If source is from text, nothing is enabled.

        wnd_flatten->setEnabled(false);
        wnd_ecf_label->setEnabled(false);
        wnd_ecf_pre->setEnabled(false);
        wnd_ecf_post->setEnabled(false);
        wnd_use_win->setEnabled(false);
        wnd_clip->setEnabled(false);
        wnd_sb_left->setEnabled(false);
        wnd_sb_bottom->setEnabled(false);
        wnd_sb_right->setEnabled(false);
        wnd_sb_top->setEnabled(false);
        wnd_l_label->setEnabled(false);
        wnd_b_label->setEnabled(false);
        wnd_r_label->setEnabled(false);
        wnd_t_label->setEnabled(false);
        wnd_rbutton->setEnabled(false);
        wnd_sbutton->setEnabled(false);
    }
}


bool
QTwindowCfg::get_bb(BBox *BB)
{
    if (!BB)
        return (false);
    double l = wnd_sb_left->value();
    double b = wnd_sb_bottom->value();
    double r = wnd_sb_right->value();
    double t = wnd_sb_top->value();
    BB->left = INTERNAL_UNITS(l);
    BB->bottom = INTERNAL_UNITS(b);
    BB->right = INTERNAL_UNITS(r);
    BB->top = INTERNAL_UNITS(t);
    if (BB->right < BB->left) {
        int tmp = BB->left;
        BB->left = BB->right;
        BB->right = tmp;
    }
    if (BB->top < BB->bottom) {
        int tmp = BB->bottom;
        BB->bottom = BB->top;
        BB->top = tmp;
    }
    if (BB->left == BB->right || BB->bottom == BB->top)
        return (false);;
    return (true);
}


void
QTwindowCfg::set_bb(const BBox *BB)
{
    if (!BB)
        return;
    wnd_sb_left->setValue(MICRONS(BB->left));
    wnd_sb_bottom->setValue(MICRONS(BB->bottom));
    wnd_sb_right->setValue(MICRONS(BB->right));
    wnd_sb_top->setValue(MICRONS(BB->top));
}


void
QTwindowCfg::usewin_btn_slot(int state)
{
    if (wnd_func_mode == WndFuncCvt)
        FIO()->SetCvtUseWindow(state);
    else if (wnd_func_mode == WndFuncOut)
        FIO()->SetOutUseWindow(state);
    else if (wnd_func_mode == WndFuncIn)
        FIO()->SetInUseWindow(state);
    set_sens();
}


void
QTwindowCfg::clip_btn_slot(int state)
{
    if (wnd_func_mode == WndFuncCvt)
        FIO()->SetCvtClip(state);
    else if (wnd_func_mode == WndFuncOut)
        FIO()->SetOutClip(state);
    else if (wnd_func_mode == WndFuncIn)
        FIO()->SetInClip(state);
}


void
QTwindowCfg::flatten_btn_slot(int state)
{
    if (wnd_func_mode == WndFuncCvt)
        FIO()->SetCvtFlatten(state);
    else if (wnd_func_mode == WndFuncOut)
        FIO()->SetOutFlatten(state);
    else if (wnd_func_mode == WndFuncIn)
        FIO()->SetInFlatten(state);
    set_sens();
}


void
QTwindowCfg::s_menu_slot(QAction *a)
{
    get_bb(FIO()->savedBB(a->data().toInt()));
}


void
QTwindowCfg::r_menu_slot(QAction *a)
{
    set_bb(FIO()->savedBB(a->data().toInt()));
}


void
QTwindowCfg::left_value_slot(double d)
{
    if (wnd_func_mode == WndFuncCvt)
        FIO()->SetCvtWindowLeft(INTERNAL_UNITS(d));
    else if (wnd_func_mode == WndFuncOut)
        FIO()->SetOutWindowLeft(INTERNAL_UNITS(d));
    else if (wnd_func_mode == WndFuncIn)
        FIO()->SetInWindowLeft(INTERNAL_UNITS(d));
}


void
QTwindowCfg::right_value_slot(double d)
{
    if (wnd_func_mode == WndFuncCvt)
        FIO()->SetCvtWindowRight(INTERNAL_UNITS(d));
    else if (wnd_func_mode == WndFuncOut)
        FIO()->SetOutWindowRight(INTERNAL_UNITS(d));
    else if (wnd_func_mode == WndFuncIn)
        FIO()->SetInWindowRight(INTERNAL_UNITS(d));
}


void
QTwindowCfg::bottom_value_slot(double d)
{
    if (wnd_func_mode == WndFuncCvt)
        FIO()->SetCvtWindowBottom(INTERNAL_UNITS(d));
    else if (wnd_func_mode == WndFuncOut)
        FIO()->SetOutWindowBottom(INTERNAL_UNITS(d));
    else if (wnd_func_mode == WndFuncIn)
        FIO()->SetInWindowBottom(INTERNAL_UNITS(d));
}


void
QTwindowCfg::top_value_slot(double d)
{
    if (wnd_func_mode == WndFuncCvt)
        FIO()->SetCvtWindowTop(INTERNAL_UNITS(d));
    else if (wnd_func_mode == WndFuncOut)
        FIO()->SetOutWindowTop(INTERNAL_UNITS(d));
    else if (wnd_func_mode == WndFuncIn)
        FIO()->SetInWindowTop(INTERNAL_UNITS(d));
}


void
QTwindowCfg::ecf_pre_btn_slot(int state)
{
    if (wnd_func_mode == WndFuncCvt) {
        if (state) {
            switch (FIO()->CvtECFlevel()) {
            case ECFnone:
                FIO()->SetCvtECFlevel(ECFpre);
                break;
            case ECFall:
                break;
            case ECFpre:
                break;
            case ECFpost:
                FIO()->SetCvtECFlevel(ECFall);
                break;
            }
        }
        else {
            switch (FIO()->CvtECFlevel()) {
            case ECFnone:
                break;
            case ECFall:
                FIO()->SetCvtECFlevel(ECFpost);
                break;
            case ECFpre:
                FIO()->SetCvtECFlevel(ECFnone);
                break;
            case ECFpost:
                break;
            }
        }
    }
    else if (wnd_func_mode == WndFuncOut) {
        if (state) {
            switch (FIO()->OutECFlevel()) {
            case ECFnone:
                FIO()->SetOutECFlevel(ECFpre);
                break;
            case ECFall:
                break;
            case ECFpre:
                break;
            case ECFpost:
                FIO()->SetOutECFlevel(ECFall);
                break;
            }
        }
        else {
            switch (FIO()->OutECFlevel()) {
            case ECFnone:
                break;
            case ECFall:
                FIO()->SetOutECFlevel(ECFpost);
                break;
            case ECFpre:
                FIO()->SetOutECFlevel(ECFnone);
                break;
            case ECFpost:
                break;
            }
        }
    }
    else if (wnd_func_mode == WndFuncIn) {
        if (state) {
            switch (FIO()->InECFlevel()) {
            case ECFnone:
                FIO()->SetInECFlevel(ECFpre);
                break;
            case ECFall:
                break;
            case ECFpre:
                break;
            case ECFpost:
                FIO()->SetInECFlevel(ECFall);
                break;
            }
        }
        else {
            switch (FIO()->InECFlevel()) {
            case ECFnone:
                break;
            case ECFall:
                FIO()->SetInECFlevel(ECFpost);
                break;
            case ECFpre:
                FIO()->SetInECFlevel(ECFnone);
                break;
            case ECFpost:
                break;
            }
        }
    }
}


void
QTwindowCfg::ecf_post_btn_slot(int state)
{
    if (wnd_func_mode == WndFuncCvt) {
        if (state) {
            switch (FIO()->CvtECFlevel()) {
            case ECFnone:
                FIO()->SetCvtECFlevel(ECFpost);
                break;
            case ECFall:
                break;
            case ECFpre:
                FIO()->SetCvtECFlevel(ECFall);
                break;
            case ECFpost:
                break;
            }
        }
        else {
            switch (FIO()->CvtECFlevel()) {
            case ECFnone:
                break;
            case ECFall:
                FIO()->SetCvtECFlevel(ECFpre);
                break;
            case ECFpre:
                break;
            case ECFpost:
                FIO()->SetCvtECFlevel(ECFnone);
                break;
            }
        }
    }
    else if (wnd_func_mode == WndFuncOut) {
        if (state) {
            switch (FIO()->OutECFlevel()) {
            case ECFnone:
                FIO()->SetOutECFlevel(ECFpost);
                break;
            case ECFall:
                break;
            case ECFpre:
                FIO()->SetOutECFlevel(ECFall);
                break;
            case ECFpost:
                break;
            }
        }
        else {
            switch (FIO()->OutECFlevel()) {
            case ECFnone:
                break;
            case ECFall:
                FIO()->SetOutECFlevel(ECFpre);
                break;
            case ECFpre:
                break;
            case ECFpost:
                FIO()->SetOutECFlevel(ECFnone);
                break;
            }
        }
    }
    else if (wnd_func_mode == WndFuncIn) {
        if (state) {
            switch (FIO()->InECFlevel()) {
            case ECFnone:
                FIO()->SetInECFlevel(ECFpost);
                break;
            case ECFall:
                break;
            case ECFpre:
                FIO()->SetInECFlevel(ECFall);
                break;
            case ECFpost:
                break;
            }
        }
        else {
            switch (FIO()->InECFlevel()) {
            case ECFnone:
                break;
            case ECFall:
                FIO()->SetInECFlevel(ECFpre);
                break;
            case ECFpre:
                break;
            case ECFpost:
                FIO()->SetInECFlevel(ECFnone);
                break;
            }
        }
    }
}

