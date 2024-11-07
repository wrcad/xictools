
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

#include "qtlexp.h"
#include "qtllist.h"
#include "edit.h"
#include "dsp_inlines.h"
#include "geo_grid.h"
#include "promptline.h"
#include "qtinterf/qtdblsb.h"

#include <QApplication>
#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QMenu>
#include <QAction>


//-----------------------------------------------------------------------------
// QTlayerExpDlg:  Dialog to control layer expression evaluation.
// Called from main menu: Edit/Layer Expression.
//
// Help system keywords used:
//  xic:lexpr

void
cEdit::PopUpLayerExp(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTlayerExpDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTlayerExpDlg::self())
            QTlayerExpDlg::self()->update();
        return;
    }
    if (QTlayerExpDlg::self())
        return;

    new QTlayerExpDlg(caller);

    QTlayerExpDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_UL), QTlayerExpDlg::self(),
        QTmainwin::self()->Viewport());
    QTlayerExpDlg::self()->show();
}
// End of cEdit functions.


char *QTlayerExpDlg::last_lexpr = 0;
int QTlayerExpDlg::depth_hst = 0;
int QTlayerExpDlg::create_mode = CLdefault;
bool QTlayerExpDlg::fast_mode = false;
bool QTlayerExpDlg::use_merge = false;
bool QTlayerExpDlg::do_recurse = false;
bool QTlayerExpDlg::noclear = false;
QTlayerExpDlg *QTlayerExpDlg::instPtr;

QTlayerExpDlg::QTlayerExpDlg(GRobject c)
{
    instPtr = this;
    lx_caller = c;
    lx_deflt = 0;
    lx_join = 0;
    lx_split_h = 0;
    lx_split_v = 0;
    lx_depth = 0;
    lx_recurse = 0;
    lx_merge = 0;
    lx_fast = 0;
    lx_none = 0;
    lx_tolayer = 0;
    lx_noclear = 0;
    lx_lexpr = 0;
    lx_save = 0;
    lx_recall = 0;
    lx_save_menu = 0;
    lx_recall_menu = 0;
    lx_sb_part = 0;
    lx_sb_thread = 0;

    lx_last_part_size = DEF_GRD_PART_SIZE;

    setWindowTitle(tr("Exaluate Layer Expression"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
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
    QLabel *label = new QLabel(tr(
        "Set parameters, evaluate layer expression"));
    hb->addWidget(label);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTlayerExpDlg::help_btn_slot);

    // depth option, recurse check box
    //
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    label = new QLabel(tr("Depth to process"));
    hbox->addWidget(label);

#define DMAX 6
    lx_depth = new QComboBox();
    hbox->addWidget(lx_depth);
    for (int i = 0; i <= DMAX; i++) {
        char buf[16];
        if (i == DMAX)
            strcpy(buf, "all");
        else
            snprintf(buf, sizeof(buf), "%d", i);
        lx_depth->addItem(tr(buf));
    }
    lx_depth->setCurrentIndex(0);
    connect(lx_depth, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &QTlayerExpDlg::depth_changed_slot);

#if QT_VERSION >= QT_VERSION_CHECK(6,8,0)
#define CHECK_BOX_STATE_CHANGED &QCheckBox::checkStateChanged
#else
#define CHECK_BOX_STATE_CHANGED &QCheckBox::stateChanged
#endif

    lx_recurse = new QCheckBox(tr("Recursively create in subcells"));
    hbox->addWidget(lx_recurse);
    connect(lx_recurse, CHECK_BOX_STATE_CHANGED,
        this, &QTlayerExpDlg::recurse_btn_slot);

    // target layer entry
    //
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(4);
    vbox->addLayout(hbox);

    label = new QLabel(tr("To layer"));
    hbox->addWidget(label);

    // Use a line edit that accepts layer drops.
    lx_tolayer = new QTlayerEdit();
    hbox->addWidget(lx_tolayer);

    // partition size entry
    label = new QLabel(tr("Partition size"));
    hbox->addSpacing(8);
    hbox->addWidget(label);

    lx_none = new QToolButton();
    lx_none->setText(tr("None"));
    hbox->addWidget(lx_none);
    lx_none->setCheckable(true);
    connect(lx_none, &QAbstractButton::toggled,
        this, &QTlayerExpDlg::none_btn_slot);

    lx_sb_part = new QTdoubleSpinBox();
    lx_sb_part->setMinimum(GRD_PART_MIN);
    lx_sb_part->setMaximum(GRD_PART_MAX);
    lx_sb_part->setDecimals(2);
    lx_sb_part->setValue(0.0);
    hbox->addWidget(lx_sb_part);
    connect(lx_sb_part, QOverload<double>::of(&QTdoubleSpinBox::valueChanged),
        this, &QTlayerExpDlg::part_changed_slot);

    // helper threads entry
    //
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    label = new QLabel(tr("Number of helper threads"));
    hbox->addWidget(label);

    lx_sb_thread = new QSpinBox();
    lx_sb_thread->setMinimum(DSP_MIN_THREADS);
    lx_sb_thread->setMaximum(DSP_MAX_THREADS);
    lx_sb_thread->setValue(DSP_DEF_THREADS);
    hbox->addWidget(lx_sb_thread);
    connect(lx_sb_thread, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &QTlayerExpDlg::threads_changed_slot);

    // join/split radio group
    //
    gb = new QGroupBox(tr("New object format"));
    vbox->addWidget(gb);
    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);

    lx_deflt = new QRadioButton(tr("Default"));
    hb->addWidget(lx_deflt);
    connect(lx_deflt, &QRadioButton::toggled,
        this, &QTlayerExpDlg::deflt_btn_slot);

    lx_join = new QRadioButton(tr("Joined"));
    hb->addWidget(lx_join);
    connect(lx_join, &QRadioButton::toggled,
        this, &QTlayerExpDlg::join_btn_slot);

    lx_split_h = new QRadioButton(tr("Horiz Split"));
    hb->addWidget(lx_split_h);
    connect(lx_split_h, &QRadioButton::toggled,
        this, &QTlayerExpDlg::split_h_btn_slot);

    lx_split_v = new QRadioButton(tr("Vert Split"));
    hb->addWidget(lx_split_v);
    connect(lx_split_v, &QRadioButton::toggled,
        this, &QTlayerExpDlg::split_v_btn_slot);

    // layer expression entry, store/recall buttons
    //
    gb = new QGroupBox(tr("Expression"));
    vbox->addWidget(gb);
    QVBoxLayout *vb = new QVBoxLayout(gb);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    lx_lexpr = new QLineEdit();
    vb->addWidget(lx_lexpr);

    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    lx_recall = new QToolButton();
    lx_recall->setText(tr("Recall"));
    hb->addWidget(lx_recall);
    lx_recall_menu = new QMenu();
    lx_recall->setMenu(lx_recall_menu);
    lx_recall->setPopupMode(QToolButton::InstantPopup);
    for (int i = 0; i < ED_LEXPR_STORES; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "Reg %d", i);
        QAction *a = lx_recall_menu->addAction(buf);
        a->setData(i);
    }
    connect(lx_recall_menu, &QMenu::triggered,
        this, &QTlayerExpDlg::recall_menu_slot);

    lx_save = new QToolButton();
    lx_save->setText(tr("Save"));
    hb->addWidget(lx_save);
    lx_save_menu = new QMenu();
    lx_save->setMenu(lx_save_menu);
    lx_save->setPopupMode(QToolButton::InstantPopup);
    for (int i = 0; i < ED_LEXPR_STORES; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "Reg %d", i);
        QAction *a = lx_save_menu->addAction(buf);
        a->setData(i);
    }
    connect(lx_save_menu, &QMenu::triggered,
        this, &QTlayerExpDlg::save_menu_slot);

    // no clear check box
    //
    lx_noclear = new QCheckBox(tr("Don't clear layer before evaluation"));
    vbox->addWidget(lx_noclear);
    connect(lx_noclear, CHECK_BOX_STATE_CHANGED,
        this, &QTlayerExpDlg::noclear_btn_slot);

    // merge and fast mode check boxes
    //
    lx_merge = new QCheckBox(tr("Use object merging while processing"));
    vbox->addWidget(lx_merge);
    connect(lx_merge, CHECK_BOX_STATE_CHANGED,
        this, &QTlayerExpDlg::merge_btn_slot);

    lx_fast = new QCheckBox(tr("Fast mode, NOT UNDOABLE"));
    vbox->addWidget(lx_fast);
    connect(lx_merge, CHECK_BOX_STATE_CHANGED,
        this, &QTlayerExpDlg::fast_btn_slot);

    // evaluate and dismiss buttons
    //
    hbox = new QHBoxLayout;
    vbox->addSpacing(8);
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    tbtn = new QToolButton();
    tbtn->setText(tr("Evaluate"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTlayerExpDlg::eval_btn_slot);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    hbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QTlayerExpDlg::dismiss_btn_slot);

    if (last_lexpr)
        lx_lexpr->setText(last_lexpr);
    lx_depth->setCurrentIndex(depth_hst);
    if (create_mode == CLdefault) {
        QTdev::SetStatus(lx_deflt, true);
        QTdev::SetStatus(lx_join, false);
        QTdev::SetStatus(lx_split_h, false);
        QTdev::SetStatus(lx_split_v, false);
    }
    else if (create_mode == CLsplitH) {
        QTdev::SetStatus(lx_deflt, false);
        QTdev::SetStatus(lx_join, false);
        QTdev::SetStatus(lx_split_h, true);
        QTdev::SetStatus(lx_split_v, false);
    }
    else if (create_mode == CLsplitV) {
        QTdev::SetStatus(lx_deflt, false);
        QTdev::SetStatus(lx_join, false);
        QTdev::SetStatus(lx_split_h, false);
        QTdev::SetStatus(lx_split_v, true);
    }
    else {
        QTdev::SetStatus(lx_deflt, false);
        QTdev::SetStatus(lx_join, true);
        QTdev::SetStatus(lx_split_h, false);
        QTdev::SetStatus(lx_split_v, false);
    }
    QTdev::SetStatus(lx_fast, fast_mode);
    QTdev::SetStatus(lx_merge, use_merge);
    QTdev::SetStatus(lx_recurse, do_recurse);
    QTdev::SetStatus(lx_noclear, noclear);
    update();
}


QTlayerExpDlg::~QTlayerExpDlg()
{
    instPtr = 0;
    char *s = lstring::copy(lx_lexpr->text().toLatin1().constData());
    if (s && !*s) {
        delete [] s;
        s = 0;
    }
    last_lexpr = s;

    if (lx_caller)
        QTdev::Deselect(lx_caller);
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTlayerExpDlg
#include "qtinterf/qtmacos_event.h"
#endif


void
QTlayerExpDlg::update()
{
    const char *s = CDvdb()->getVariable(VA_PartitionSize);
    if (s) {
        double d = atof(s);
        if (d == 0.0) {
            QTdev::SetStatus(lx_none, true);
            lx_sb_part->setEnabled(false);
        }
        else {
            QTdev::SetStatus(lx_none, false);
            lx_sb_part->setEnabled(true);
            lx_sb_part->setValue(d);
        }
    }
    else {
        QTdev::SetStatus(lx_none, false);
        lx_sb_part->setEnabled(true);
        lx_sb_part->setValue(DEF_GRD_PART_SIZE);
    }

    s = CDvdb()->getVariable(VA_Threads);
    int n;
    if (s && sscanf(s, "%d", &n) == 1 && n >= DSP_MIN_THREADS &&
            n <= DSP_MAX_THREADS)
        ;
    else
        n = DSP_DEF_THREADS;
    lx_sb_thread->setValue(n);
}


void
QTlayerExpDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:lexpr"))
}


void
QTlayerExpDlg::depth_changed_slot(int d)
{
    depth_hst = d;
}


void
QTlayerExpDlg::recurse_btn_slot(int state)
{
    do_recurse = state;
}


void
QTlayerExpDlg::none_btn_slot(bool state)
{
    if (state) {
        lx_last_part_size = lx_sb_part->value();
        CDvdb()->setVariable(VA_PartitionSize, "0");
    }
    else {
        lx_sb_part->setValue(lx_last_part_size);
        part_changed_slot(lx_last_part_size);
    }
}


void
QTlayerExpDlg::part_changed_slot(double val)
{
    if (val >= GRD_PART_MIN && val <= GRD_PART_MAX) {
        int nint = INTERNAL_UNITS(val);
        if (nint == INTERNAL_UNITS(DEF_GRD_PART_SIZE))
            CDvdb()->clearVariable(VA_PartitionSize);
        else {
            const char *s =
                lstring::copy(lx_sb_part->text().toLatin1().constData());
            CDvdb()->setVariable(VA_PartitionSize, s);
            delete [] s;
        }
    }
}


void
QTlayerExpDlg::threads_changed_slot(int d)
{
    if (d >= DSP_MIN_THREADS && d <= DSP_MAX_THREADS) {
        if (d == DSP_DEF_THREADS)
            CDvdb()->clearVariable(VA_Threads);
        else {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", d);
            CDvdb()->setVariable(VA_Threads, buf);
        }
    }
}


void
QTlayerExpDlg::deflt_btn_slot(bool state)
{
    if (state)
        create_mode = CLdefault;
}


void
QTlayerExpDlg::join_btn_slot(bool state)
{
    if (state)
        create_mode = CLjoin;
}


void
QTlayerExpDlg::split_h_btn_slot(bool state)
{
    if (state)
        create_mode = CLsplitH;
}


void
QTlayerExpDlg::split_v_btn_slot(bool state)
{
    if (state)
        create_mode = CLsplitV;
}


void
QTlayerExpDlg::recall_menu_slot(QAction *a)
{
    int ix = a->data().toInt();
    const char *s = ED()->layerExpString(ix);
    if (!s)
        s = "";
    lx_lexpr->setText(s);
}


void
QTlayerExpDlg::save_menu_slot(QAction *a)
{
    int ix = a->data().toInt();
    const char *s = lstring::copy(lx_lexpr->text().toLatin1().constData());
    if (*s)
        ED()->setLayerExpString(s, ix);
    delete [] s;
}


void
QTlayerExpDlg::noclear_btn_slot(int state)
{
    noclear = state;
}


void
QTlayerExpDlg::merge_btn_slot(int state)
{
    use_merge = state;
}


void
QTlayerExpDlg::fast_btn_slot(int state)
{
    fast_mode = state;
}


void
QTlayerExpDlg::eval_btn_slot()
{
    const char *s = lstring::copy(lx_tolayer->text().toLatin1().constData());
    const char *t = s;
    char *lname = lstring::gettok(&t);
    delete [] s;
    if (!lname) {
        PL()->ShowPrompt("No target layer name given!");
        return;
    }
    sLstr lstr;
    lstr.add(lname);
    delete [] lname;
    lstr.add_c(' ');
    s = lstring::copy(lx_lexpr->text().toLatin1().constData());
    lstr.add(s);
    delete [] s;

    int dep = depth_hst;
    if (dep == DMAX)
        dep = CDMAXCALLDEPTH;

    int flags = create_mode;
    if (do_recurse && dep > 0)
        flags |= CLrecurse;
    if (noclear)
        flags |= CLnoClear;
    if (use_merge)
        flags |= CLmerge;
    if (fast_mode)
        flags |= CLnoUndo;

    ED()->createLayerCmd(lstr.string(), dep, flags);
}


void
QTlayerExpDlg::dismiss_btn_slot()
{
    ED()->PopUpLayerExp(0, MODE_OFF);
}

