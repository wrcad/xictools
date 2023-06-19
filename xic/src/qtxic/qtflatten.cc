
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

#include "qtflatten.h"
#include "edit.h"
#include "dsp_inlines.h"
#include "cvrt_variables.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QAction>


//--------------------------------------------------------------------------
// Pop up for the Flatten command
//
// Help system keywords used:
//  xic:flatn

#define DMAX 6

// Pop-up for the Flatten command, consists of a depth option selector,
// fast-mode toggle, go and dismiss buttons.
//
void
cEdit::PopUpFlatten(GRobject caller, ShowMode mode,
    bool (*callback)(const char*, bool, const char*, void*),
    void *arg, int depth, bool fmode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (cFlatten::self())
            cFlatten::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (cFlatten::self())
            cFlatten::self()->update();
        return;
    }
    if (cFlatten::self())
        return;

    new cFlatten(caller, callback, arg, depth, fmode);

    QTdev::self()->SetPopupLocation(GRloc(), cFlatten::self(),
        QTmainwin::self()->Viewport());
    cFlatten::self()->show();
}
// End of cEdit functions.


cFlatten *cFlatten::instPtr;

cFlatten::cFlatten(
    GRobject c, bool(*callback)(const char*, bool, const char*, void*),
    void *arg, int depth, bool fmode)
{
    instPtr = this;
    fl_caller = c;
    fl_novias = 0;
    fl_nopcells = 0;
    fl_nolabels = 0;
    fl_merge = 0;
    fl_go = 0;
    fl_callback = callback;
    fl_arg = arg;

    setWindowTitle(tr("Flatten Hierarchy"));
    setWindowFlags(Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_ShowWithoutActivating);

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
    QLabel *label = new QLabel(tr("Selected subcells will be flattened"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // depth option
    //
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    label = new QLabel(tr("Depth to flatten"));
    hbox->addWidget(label);

    QComboBox *entry = new QComboBox();
    hbox->addWidget(entry);

    if (depth < 0)
        depth = 0;
    if (depth > DMAX)
        depth = DMAX;
    for (int i = 0; i <= DMAX; i++) {
        char buf[16];
        if (i == DMAX)
            strcpy(buf, "all");
        else
            snprintf(buf, sizeof(buf), "%d", i);
        entry->addItem(buf);
    }
    entry->setCurrentIndex(depth);
    connect(entry, SIGNAL(currentTextChanged(const QString&)),
        this, SLOT(depth_menu_slot(const QString&)));

    // check boxes
    //
    fl_novias = new QCheckBox(tr(
        "Don't flatten standard vias, move to top"));
    vbox->addWidget(fl_novias);
    connect(fl_novias, SIGNAL(stateChanged(int)),
        this, SLOT(novias_btn_slot(int)));

    fl_nopcells = new QCheckBox(tr(
        "Don't flatten param. cells, move to top"));
    vbox->addWidget(fl_nopcells);
    connect(fl_nopcells, SIGNAL(stateChanged(int)),
        this, SLOT(nopcells_btn_slot(int)));

    fl_nolabels = new QCheckBox(tr("Ignore labels in subcells"));
    vbox->addWidget(fl_nolabels);
    connect(fl_nolabels, SIGNAL(stateChanged(int)),
        this, SLOT(nolabels_btn_slot(int)));

    QCheckBox *cbox = new QCheckBox(tr("Use fast mode, NOT UNDOABLE"));
    vbox->addWidget(cbox);
    QTdev::SetStatus(cbox, fmode);
    connect(cbox, SIGNAL(stateChanged(int)),
        this, SLOT(fastmode_btn_slot(int)));

    fl_merge = new QCheckBox(tr("Use object merging when flattening"));
    vbox->addWidget(fl_merge);
    connect(fl_merge, SIGNAL(stateChanged(int)),
        this, SLOT(merge_btn_slot(int)));

    // flatten and dismiss buttons
    //
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    fl_go = new QPushButton(tr("Flatten"));
    hbox->addWidget(fl_go);
    connect(fl_go, SIGNAL(clicked()), this, SLOT(go_btn_slot()));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


cFlatten::~cFlatten()
{
    instPtr = 0;
    if (fl_caller)
        QTdev::Deselect(fl_caller);
    if (fl_callback)
        (*fl_callback)(0, false, 0, fl_arg);
}


void
cFlatten::update()
{
    QTdev::SetStatus(fl_novias,
        CDvdb()->getVariable(VA_NoFlattenStdVias));
    QTdev::SetStatus(fl_nopcells,
        CDvdb()->getVariable(VA_NoFlattenPCells));
    QTdev::SetStatus(fl_nolabels,
        CDvdb()->getVariable(VA_NoFlattenLabels));
}


void
cFlatten::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:flatn"))
}


void
cFlatten::depth_menu_slot(const QString &qs)
{
    if (fl_callback) {
        const char *str = lstring::copy(qs.toLatin1().constData());
        if (str) {
            (*fl_callback)("depth", true, str, fl_arg);
            delete [] str;
        }
    }
}


void
cFlatten::novias_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoFlattenStdVias, "");
    else
        CDvdb()->clearVariable(VA_NoFlattenStdVias);
}


void
cFlatten::nopcells_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoFlattenPCells, "");
    else
        CDvdb()->clearVariable(VA_NoFlattenPCells);
}


void
cFlatten::nolabels_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoFlattenLabels, "");
    else
        CDvdb()->clearVariable(VA_NoFlattenLabels);
}


void
cFlatten::fastmode_btn_slot(int state)
{
    if (fl_callback)
        (*fl_callback)("mode", state, 0, fl_arg);
}


void
cFlatten::merge_btn_slot(int state)
{
    if (fl_callback)
        (*fl_callback)("merge", state, 0, fl_arg);
}


void
cFlatten::go_btn_slot()
{
    if (fl_callback)
        (*fl_callback)("flatten", true, 0, fl_arg);
}


void
cFlatten::dismiss_btn_slot()
{
    ED()->PopUpFlatten(0, MODE_OFF, 0, 0);
}

