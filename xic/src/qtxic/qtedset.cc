
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

#include "qtedset.h"
#include "edit.h"
#include "undolist.h"
#include "dsp_inlines.h"
#include "tech.h"

#include <QApplication>
#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QToolButton>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>


//-----------------------------------------------------------------------------
// QTeditSetupDlg:  Dialog to set edit defaults.
// Called from main menu: Edit/Editing Setup.
//
// Help system keywords used:
//  xic:edset

void
cEdit::PopUpEditSetup(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTeditSetupDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTeditSetupDlg::self())
            QTeditSetupDlg::self()->update();
        return;
    }
    if (QTeditSetupDlg::self())
        return;

    new QTeditSetupDlg(caller);

    QTeditSetupDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTeditSetupDlg::self(),
        QTmainwin::self()->Viewport());
    QTeditSetupDlg::self()->show();
}
// End of cEdit functions.


const char *QTeditSetupDlg::ed_depthvals[] =
{
    "as expanded",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    0
};

QTeditSetupDlg *QTeditSetupDlg::instPtr;

QTeditSetupDlg::QTeditSetupDlg(GRobject c)
{
    instPtr = this;
    ed_caller = c;
    ed_cons45 = 0;
    ed_merge = 0;
    ed_noply = 0;
    ed_prompt = 0;
    ed_noww = 0;
    ed_crcovr = 0;
    ed_depth = 0;
    ed_sb_ulen = 0;
    ed_sb_maxgobjs = 0;

    setWindowTitle(tr("Editing Setup"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);
    vbox->setSizeConstraint(QLayout::SetFixedSize);

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
    QLabel *label = new QLabel(tr("Set editing flags and parameters"));
    hb->addWidget(label);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // check boxes
    //
    ed_cons45 = new QCheckBox(tr("Constrain angles to 45 degree multiples"));
    vbox->addWidget(ed_cons45);
    connect(ed_cons45, SIGNAL(stateChanged(int)),
        this, SLOT(cons45_btn_slot(int)));

    ed_merge = new QCheckBox(tr(
        "Merge new boxes and polys with existing boxes/polys"));
    vbox->addWidget(ed_merge);
    connect(ed_merge, SIGNAL(stateChanged(int)),
        this, SLOT(merge_btn_slot(int)));

    ed_noply = new QCheckBox(tr("Clip and merge new boxes only, not polys"));
    vbox->addWidget(ed_noply);
    connect(ed_noply, SIGNAL(stateChanged(int)),
        this, SLOT(noply_btn_slot(int)));

    ed_prompt = new QCheckBox(tr("Prompt to save modified native cells"));
    vbox->addWidget(ed_prompt);
    connect(ed_prompt, SIGNAL(stateChanged(int)),
        this, SLOT(prompt_btn_slot(int)));

    ed_noww = new QCheckBox(tr("No wire width change in magnification"));
    vbox->addWidget(ed_noww);
    connect(ed_noww, SIGNAL(stateChanged(int)),
        this, SLOT(noww_btn_slot(int)));

    ed_crcovr = new QCheckBox(tr(
        "Allow Create Cell to overwrite existing cell"));
    vbox->addWidget(ed_crcovr);
    vbox->addSpacing(10);
    connect(ed_crcovr, SIGNAL(stateChanged(int)),
        this, SLOT(crcovr_btn_slot(int)));

    // integer parameters
    //
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QVBoxLayout *col1 = new QVBoxLayout();
    hbox->addLayout(col1);
    col1->setContentsMargins(qm);
    col1->setSpacing(2);

    QVBoxLayout *col2 = new QVBoxLayout();
    hbox->addLayout(col2);
    col2->setContentsMargins(qm);
    col2->setSpacing(2);

    label = new QLabel(tr("Maximum undo list length"));
    col1->addWidget(label);

    ed_sb_ulen = new QSpinBox();
    ed_sb_ulen->setRange(0, 1000);
    ed_sb_ulen->setValue(DEF_MAX_UNDO_LEN);
    col2->addWidget(ed_sb_ulen);
    connect(ed_sb_ulen, SIGNAL(valueChanged(int)),
        this, SLOT(ulen_changed_slot(int)));

    label = new QLabel(tr("Maximum number of ghost-drawn objects"));
    col1->addWidget(label);

    ed_sb_maxgobjs = new QSpinBox();
    ed_sb_maxgobjs->setRange(50, 50000);
    ed_sb_maxgobjs->setValue(DEF_MAX_GHOST_OBJECTS);
    col2->addWidget(ed_sb_maxgobjs);
    connect(ed_sb_maxgobjs, SIGNAL(valueChanged(int)),
        this, SLOT(maxgobs_changed_slot(int)));

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    label = new QLabel(tr("Maximum subcell depth in ghosting"));
    hbox->addWidget(label);

    ed_depth = new QComboBox();
    hbox->addWidget(ed_depth);
    for (int i = 0; ed_depthvals[i]; i++)
        ed_depth->addItem(ed_depthvals[i]);
    connect(ed_depth, SIGNAL(currentIndexChanged(int)),
        this, SLOT(depth_changed_slot(int)));

    // Dismiss button
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Dismiss");
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTeditSetupDlg::~QTeditSetupDlg()
{
    instPtr = 0;
    if (ed_caller)
        QTdev::Deselect(ed_caller);
}


#ifdef Q_OS_MACOS

bool
QTeditSetupDlg::event(QEvent *ev)
{
    // Fix for QT BUG 116674, text becomes invisible on autodefault
    // button when the main window has focus.

    if (ev->type() == QEvent::ActivationChange) {
        QPushButton *dsm = findChild<QPushButton*>("Dismiss",
            Qt::FindDirectChildrenOnly);
        if (dsm) {
            QWidget *top = this;
            while (top->parentWidget())
                top = top->parentWidget();
            if (QApplication::activeWindow() == top)
                dsm->setDefault(false);
            else if (QApplication::activeWindow() == this)
                dsm->setDefault(true);
        }
    }
    return (QDialog::event(ev));
}

#endif


void
QTeditSetupDlg::update()
{
    QTdev::SetStatus(ed_cons45,
        CDvdb()->getVariable(VA_Constrain45));
    QTdev::SetStatus(ed_merge,
        !CDvdb()->getVariable(VA_NoMergeObjects));
    QTdev::SetStatus(ed_noply,
        CDvdb()->getVariable(VA_NoMergePolys));
    QTdev::SetStatus(ed_prompt,
        CDvdb()->getVariable(VA_AskSaveNative));

    int n = ed_sb_ulen->value();
    if (n != Ulist()->UndoLength())
        ed_sb_ulen->setValue(Ulist()->UndoLength());

    n = ed_sb_maxgobjs->value();
    if ((unsigned int)n != EGst()->maxGhostObjects())
        ed_sb_maxgobjs->setValue(EGst()->maxGhostObjects());

    QTdev::SetStatus(ed_noww,
        CDvdb()->getVariable(VA_NoWireWidthMag));
    QTdev::SetStatus(ed_crcovr,
        CDvdb()->getVariable(VA_CrCellOverwrite));

    ed_noply->setEnabled(QTdev::GetStatus(ed_merge));

    int hst = 0;
    const char *str = CDvdb()->getVariable(VA_MaxGhostDepth);
    if (str)
        hst = atoi(str) + 1;
    ed_depth->setCurrentIndex(hst);
}


void
QTeditSetupDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:edset"))
}


void
QTeditSetupDlg::cons45_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_Constrain45, "");
    else
        CDvdb()->clearVariable(VA_Constrain45);
}


void
QTeditSetupDlg::merge_btn_slot(int state)
{
    if (state)
        CDvdb()->clearVariable(VA_NoMergeObjects);
    else
        CDvdb()->setVariable(VA_NoMergeObjects, "");
}


void
QTeditSetupDlg::noply_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoMergePolys, "");
    else
        CDvdb()->clearVariable(VA_NoMergePolys);
}


void
QTeditSetupDlg::prompt_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_AskSaveNative, "");
    else
        CDvdb()->clearVariable(VA_AskSaveNative);
}


void
QTeditSetupDlg::noww_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoWireWidthMag, "");
    else
        CDvdb()->clearVariable(VA_NoWireWidthMag);
}


void
QTeditSetupDlg::crcovr_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_CrCellOverwrite, "");
    else
        CDvdb()->clearVariable(VA_CrCellOverwrite);
}


void
QTeditSetupDlg::ulen_changed_slot(int n)
{
    if (n == DEF_MAX_UNDO_LEN)
        CDvdb()->clearVariable(VA_UndoListLength);
    else {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", n);
        CDvdb()->setVariable(VA_UndoListLength, buf);
    }
}


void
QTeditSetupDlg::maxgobs_changed_slot(int n)
{
    if (n == DEF_MAX_GHOST_OBJECTS)
        CDvdb()->clearVariable(VA_MaxGhostObjects);
    else {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", n);
        CDvdb()->setVariable(VA_MaxGhostObjects, buf);
    }
}


void
QTeditSetupDlg::depth_changed_slot(int n)
{
    const char *s = ed_depthvals[n];
    if (isdigit(*s))
        CDvdb()->setVariable(VA_MaxGhostDepth, s);
    else
        CDvdb()->clearVariable(VA_MaxGhostDepth);
}


void
QTeditSetupDlg::dismiss_btn_slot()
{
    ED()->PopUpEditSetup(0, MODE_OFF);
}

