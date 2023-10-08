
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

#include "qtcvin.h"
#include "cvrt.h"
#include "fio.h"
#include "dsp_inlines.h"
#include "oa_if.h"
#include "qtcnmap.h"
#include "qtllist.h"
#include "qtwndc.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTabWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>


//--------------------------------------------------------------------
// Pop-up to set input parameters and read cell files.
//
// Help system keywords used:
//  xic:imprt

void
cConvert::PopUpImport(GRobject caller, ShowMode mode,
    bool (*callback)(int, void*), void *arg)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTconvertInDlg::self())
            QTconvertInDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTconvertInDlg::self())
            QTconvertInDlg::self()->update();
        return;
    }
    if (QTconvertInDlg::self())
        return;

    new QTconvertInDlg(caller, callback, arg);

    QTdev::self()->SetPopupLocation(GRloc(LW_UL), QTconvertInDlg::self(),
        QTmainwin::self()->Viewport());
    QTconvertInDlg::self()->show();
}
// End of cConvert functions.


namespace {
    WndSensMode wnd_sens_test()     { return (WndSensFlatten); }
}


const char *QTconvertInDlg::cvi_mergvals[] =
{
    "No Merge Into Current",
    "Merge Cell Into Current",
    "Merge Into Current Recursively",
    0
};

const char *QTconvertInDlg::cvi_overvals[] =
{
    "Overwrite All",
    "Overwrite Phys",
    "Overwrite Elec",
    "Overwrite None",
    "Auto-Rename",
    0
};

const char *QTconvertInDlg::cvi_dupvals[] =
{
    "No Checking for Duplicates",
    "Put Warning in Log File",
    "Remove Duplicates",
    0
};

QTconvertInDlg::fmt_menu QTconvertInDlg::cvi_forcevals[] =
{
    { "Try Both",   EXTlreadDef },
    { "By Name",    EXTlreadName },
    { "By Index",   EXTlreadIndex },
    { 0,            0 }
};

int QTconvertInDlg::cvi_merg_val = 0;
QTconvertInDlg *QTconvertInDlg::instPtr;


QTconvertInDlg::QTconvertInDlg(GRobject c, bool (*callback)(int, void*), void *arg)
{
    instPtr = this;
    cvi_caller = c;
    cvi_label = 0;
    cvi_nbook = 0;
    cvi_nonpc = 0;
    cvi_yesoapc = 0;
    cvi_nolyr = 0;
    cvi_replace = 0;
    cvi_over = 0;
    cvi_merge = 0;
    cvi_polys = 0;
    cvi_dup = 0;
    cvi_empties = 0;
    cvi_dtypes = 0;
    cvi_force = 0;
    cvi_noflvias = 0;
    cvi_noflpcs = 0;
    cvi_nofllbs = 0;
    cvi_nolabels = 0;
    cvi_merg = 0;
    cvi_callback = callback;
    cvi_arg = arg;
    cvi_cnmap = 0;
    cvi_llist = 0;
    cvi_wnd = 0;

    // Dangerous to leave this in effect, force user to turn it on
    // when needed.
    FIO()->SetInFlatten(false);

    setWindowTitle(tr("Import Control"));
    setAttribute(Qt::WA_DeleteOnClose);
//    gtk_window_set_resizable(GTK_WINDOW(cvi_popup), false);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

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
    cvi_label = new QLabel(tr("Set parameters for reading input"));
    hb->addWidget(cvi_label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    cvi_nbook = new QTabWidget();
    vbox->addWidget(cvi_nbook);
    connect(cvi_nbook, SIGNAL(currentChanged(int)),
        this, SLOT(nbook_page_slot(int)));

    // The Setup page
    //
    QWidget *page = new QWidget();
    cvi_nbook->addTab(page, tr("Setup"));

    QVBoxLayout *pvbox = new QVBoxLayout(page);
    pvbox->setContentsMargins(qmtop);
    pvbox->setSpacing(2);

    QHBoxLayout *phbox = new QHBoxLayout();
    pvbox->addLayout(phbox);
    phbox->setContentsMargins(qm);
    phbox->setSpacing(2);

    QLabel *label = new QLabel(tr(" PCell evaluation:"));
    phbox->addWidget(label);

    cvi_nonpc = new QCheckBox(tr("Don't eval native"));
    phbox->addWidget(cvi_nonpc);
    connect(cvi_nonpc, SIGNAL(stateChanged(int)),
        this, SLOT(nonpc_btn_slot(int)));

    if (OAif()->hasOA()) {
        cvi_yesoapc = new QCheckBox(tr("Eval OpenAccess"));
        pvbox->addWidget(cvi_yesoapc);
        connect(cvi_yesoapc, SIGNAL(stateChanged(int)),
            this, SLOT(yesoapc_btn_slot(int)));
    }

    cvi_nolyr = new QCheckBox(tr(
        "Don't create new layers when reading, abort instead"));
    pvbox->addWidget(cvi_nolyr);
    connect(cvi_nolyr, SIGNAL(stateChanged(int)),
        this, SLOT(nolyr_btn_slot(int)));

    phbox = new QHBoxLayout();
    pvbox->addLayout(phbox);
    phbox->setContentsMargins(qm);
    phbox->setSpacing(2);

    cvi_over = new QComboBox();
    phbox->addWidget(cvi_over);
    for (int i = 0; cvi_overvals[i]; i++)
        cvi_over->addItem(cvi_overvals[i], i);
    connect(cvi_over, SIGNAL(currentIndexChanged(int)),
        this, SLOT(over_menu_slot(int)));

    label = new QLabel(tr("Default when new cells conflict"));
    phbox->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    cvi_replace = new QCheckBox(tr(
        "Don't prompt for overwrite instructions"));
    pvbox->addWidget(cvi_replace);
    connect(cvi_replace, SIGNAL(stateChanged(int)),
        this, SLOT(replace_btn_slot(int)));

    cvi_merge = new QCheckBox(tr(
        "Clip and merge overlapping boxes"));
    pvbox->addWidget(cvi_merge);
    connect(cvi_merge, SIGNAL(stateChanged(int)),
        this, SLOT(merge_btn_slot(int)));

    cvi_polys = new QCheckBox(tr(
        "Skip testing for badly formed polygons"));
    connect(cvi_polys, SIGNAL(stateChanged(int)),
        this, SLOT(polys_btn_slot(int)));

    phbox = new QHBoxLayout();
    pvbox->addLayout(phbox);
    phbox->setContentsMargins(qm);
    phbox->setSpacing(2);

    cvi_dup = new QComboBox();
    phbox->addWidget(cvi_dup);
    for (int i = 0; cvi_dupvals[i]; i++)
        cvi_dup->addItem(cvi_dupvals[i], i);
    connect(cvi_dup, SIGNAL(currentIndexChanged(int)),
        this, SLOT(dup_menu_slot(int)));

    label = new QLabel(tr("Duplicate item handling"));
    phbox->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    cvi_empties = new QCheckBox(tr(
        "Skip testing for empty cells"));
    pvbox->addWidget(cvi_empties);
    connect(cvi_empties, SIGNAL(stateChanged(int)),
        this, SLOT(empties_btn_slot(int)));

    cvi_dtypes = new QCheckBox(tr(
        "Map all unmapped GDSII datatypes to same Xic layer"));
    pvbox->addWidget(cvi_dtypes);
    connect(cvi_dtypes, SIGNAL(stateChanged(int)),
        this, SLOT(dtypes_btn_slot(int)));

    phbox = new QHBoxLayout();
    pvbox->addLayout(phbox);
    phbox->setContentsMargins(qm);
    phbox->setSpacing(2);

    cvi_force = new QComboBox();
    phbox->addWidget(cvi_force);;
    for (int i = 0; cvi_forcevals[i].name; i++)
        cvi_force->addItem(cvi_forcevals[i].name, i);
    connect(cvi_force, SIGNAL(currentIndexChanged(int)),
        this, SLOT(force_menu_slot(int)));

    label = new QLabel(tr("How to resolve CIF layers"));
    phbox->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    cvi_noflvias = new QCheckBox(tr(
        "Don't flatten standard vias, keep as instance at top level"));
    pvbox->addWidget(cvi_noflvias);
    connect(cvi_noflvias, SIGNAL(stateChanged(int)),
        this, SLOT(noflvias_btn_slot(int)));

    cvi_noflpcs = new QCheckBox(tr(
        "Don't flatten pcells, keep as instance at top level"));
    pvbox->addWidget(cvi_noflpcs);
    connect(cvi_noflpcs, SIGNAL(stateChanged(int)),
        this, SLOT(noflpcs_btn_slot(int)));

    cvi_nofllbs = new QCheckBox(tr(
        "Ignore labels in subcells when flattening"));
    pvbox->addWidget(cvi_nofllbs);
    connect(cvi_nofllbs, SIGNAL(stateChanged(int)),
        this, SLOT(nofllbs_btn_slot(int)));

    cvi_nolabels = new QCheckBox(tr(
        "Skip reading text labels from physical archives"));
    pvbox->addWidget(cvi_nolabels);
    connect(cvi_nolabels, SIGNAL(stateChanged(int)),
        this, SLOT(nolabels_btn_slot(int)));

    // The Read File page
    //
    page = new QWidget();
    cvi_nbook->addTab(page, tr("Read File"));

    pvbox = new QVBoxLayout(page);
    pvbox->setContentsMargins(qmtop);
    pvbox->setSpacing(2);

    phbox = new QHBoxLayout();
    pvbox->addLayout(phbox);
    phbox->setContentsMargins(qm);
    phbox->setSpacing(2);

    cvi_merg = new QComboBox();
    phbox->addWidget(cvi_merg);
    for (int i = 0; cvi_mergvals[i]; i++)
        cvi_merg->addItem(cvi_mergvals[i], i);
    cvi_merg->setCurrentIndex(cvi_merg_val);
    connect(cvi_merg, SIGNAL(currentIndexChanged(int)),
        this, SLOT(merg_menu_slot(int)));

    label = new QLabel(tr("Merge Into Current mode"));
    phbox->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    // Cell name mapping.
    //
    cvi_cnmap = new QTcnameMap(false);
    pvbox->addWidget(cvi_cnmap);

    // Layer filtering
    //
    cvi_llist = new QTlayerList();
    pvbox->addWidget(cvi_llist);

    // Window.
    //
    cvi_wnd = new QTwindowCfg(wnd_sens_test, WndFuncIn);
    pvbox->addWidget(cvi_wnd);

    // Scale spin button.
    //
    phbox = new QHBoxLayout();
    pvbox->addLayout(phbox);
    phbox->setContentsMargins(qm);
    phbox->setSpacing(2);

    label = new QLabel(tr("Reading Scale Factor"));
    phbox->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    cvi_sb_scale = new QDoubleSpinBox();
    phbox->addWidget(cvi_sb_scale);
    cvi_sb_scale->setMinimum(CDSCALEMIN);
    cvi_sb_scale->setMaximum(CDSCALEMAX);
    cvi_sb_scale->setDecimals(5);
    cvi_sb_scale->setValue(FIO()->ReadScale());
    connect(cvi_sb_scale, SIGNAL(valueChanged(double)),
        this, SLOT(scale_changed_slot(double)));

    // Read File button
    //
    btn = new QPushButton(tr("Read File"));
    btn->setMaximumWidth(140);
    pvbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(read_btn_slot()));

    // Dismiss button
    //
    btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTconvertInDlg::~QTconvertInDlg()
{
    instPtr = 0;
    if (cvi_caller)
        QTdev::Deselect(cvi_caller);
    if (cvi_callback)
        (*cvi_callback)(-1, cvi_arg);
}


void
QTconvertInDlg::update()
{
    QTdev::SetStatus(cvi_nonpc,
        CDvdb()->getVariable(VA_NoEvalNativePCells));
    QTdev::SetStatus(cvi_yesoapc,
        CDvdb()->getVariable(VA_EvalOaPCells));
    QTdev::SetStatus(cvi_nolyr,
        CDvdb()->getVariable(VA_NoCreateLayer));
    int overval = (FIO()->IsNoOverwritePhys() ? 2 : 0) +
        (FIO()->IsNoOverwriteElec() ? 1 : 0);
    if (CDvdb()->getVariable(VA_AutoRename))
        overval = 4;
    cvi_over->setCurrentIndex(overval);
    QTdev::SetStatus(cvi_replace,
        CDvdb()->getVariable(VA_NoAskOverwrite));
    QTdev::SetStatus(cvi_merge,
        CDvdb()->getVariable(VA_MergeInput));
    QTdev::SetStatus(cvi_polys,
        CDvdb()->getVariable(VA_NoPolyCheck));
    QTdev::SetStatus(cvi_empties,
        CDvdb()->getVariable(VA_NoCheckEmpties));
    QTdev::SetStatus(cvi_dtypes,
        CDvdb()->getVariable(VA_NoMapDatatypes));
    QTdev::SetStatus(cvi_noflvias,
        CDvdb()->getVariable(VA_NoFlattenStdVias));
    QTdev::SetStatus(cvi_noflpcs,
        CDvdb()->getVariable(VA_NoFlattenPCells));
    QTdev::SetStatus(cvi_nofllbs,
        CDvdb()->getVariable(VA_NoFlattenLabels));
    QTdev::SetStatus(cvi_nolabels,
        CDvdb()->getVariable(VA_NoReadLabels));

    int hst = 1;
    const char *str = CDvdb()->getVariable(VA_DupCheckMode);
    if (str) {
        if (*str == 'r' || *str == 'R')
            hst = 2;
        else if (*str == 'w' || *str == 'W')
            hst = 1;
        else
            hst = 0;
    }
    cvi_dup->setCurrentIndex(hst);

    for (int i = 0; cvi_forcevals[i].name; i++) {
        if (cvi_forcevals[i].code == FIO()->CifStyle().lread_type()) {
            cvi_force->setCurrentIndex(i);
            break;
        }
    }
    cvi_sb_scale->setValue(FIO()->ReadScale());
    cvi_cnmap->update();
    cvi_llist->update();
    cvi_wnd->update();
    cvi_wnd->set_sens();
}


void
QTconvertInDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:imprt"))
}


void
QTconvertInDlg::nbook_page_slot(int pg)
{
    const char *lb;
    if (pg == 0)
        lb = "Set parameters for reading cell data";
    else
        lb = "Read cell data file";
    cvi_label->setText(tr(lb));
}


void
QTconvertInDlg::nonpc_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoEvalNativePCells, 0);
    else
        CDvdb()->clearVariable(VA_NoEvalNativePCells);
}


void
QTconvertInDlg::yesoapc_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_EvalOaPCells, 0);
    else
        CDvdb()->clearVariable(VA_EvalOaPCells);
}


void
QTconvertInDlg::nolyr_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoCreateLayer, 0);
    else
        CDvdb()->clearVariable(VA_NoCreateLayer);
}


void
QTconvertInDlg::over_menu_slot(int i)
{
    if (i == 0) {
        // Overwrite All (default)
        CDvdb()->clearVariable(VA_NoOverwritePhys);
        CDvdb()->clearVariable(VA_NoOverwriteElec);
        CDvdb()->clearVariable(VA_AutoRename);
    }
    else if (i == 1) {
        // Overwrite Phys
        CDvdb()->clearVariable(VA_NoOverwritePhys);
        CDvdb()->setVariable(VA_NoOverwriteElec, 0);
        CDvdb()->clearVariable(VA_AutoRename);
    }
    else if (i == 2) {
        // Ovewrwrite Elec
        CDvdb()->setVariable(VA_NoOverwritePhys, 0);
        CDvdb()->clearVariable(VA_NoOverwriteElec);
        CDvdb()->clearVariable(VA_AutoRename);
    }
    else if (i == 3) {
        // Overwrite None
        CDvdb()->setVariable(VA_NoOverwritePhys, 0);
        CDvdb()->setVariable(VA_NoOverwriteElec, 0);
        CDvdb()->clearVariable(VA_AutoRename);
    }
    else if (i == 4) {
        // Auto Rename
        CDvdb()->setVariable(VA_AutoRename, 0);
        CDvdb()->clearVariable(VA_NoOverwritePhys);
        CDvdb()->clearVariable(VA_NoOverwriteElec);
    }
}


void
QTconvertInDlg::replace_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoAskOverwrite, 0);
    else
        CDvdb()->clearVariable(VA_NoAskOverwrite);
}


void
QTconvertInDlg::merge_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_MergeInput, 0);
    else
        CDvdb()->clearVariable(VA_MergeInput);
}


void
QTconvertInDlg::polys_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoPolyCheck, 0);
    else
        CDvdb()->clearVariable(VA_NoPolyCheck);
}


void
QTconvertInDlg::dup_menu_slot(int i)
{
    if (i == 0) {
        // Skip Test
        CDvdb()->setVariable(VA_DupCheckMode, "NoTest");
    }
    else if (i == 1) {
        // Warn
        CDvdb()->clearVariable(VA_DupCheckMode);
    }
    else if (i == 2) {
        // Remove Dups
        CDvdb()->setVariable(VA_DupCheckMode, "Remove");
    }
}


void
QTconvertInDlg::empties_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoCheckEmpties, 0);
    else
        CDvdb()->clearVariable(VA_NoCheckEmpties);
}


void
QTconvertInDlg::dtypes_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoMapDatatypes, 0);
    else
        CDvdb()->clearVariable(VA_NoMapDatatypes);
}


void
QTconvertInDlg::force_menu_slot(int i)
{
    FIO()->CifStyle().set_lread_type(cvi_forcevals[i].code);
    if (FIO()->CifStyle().lread_type() != EXTlreadDef) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", FIO()->CifStyle().lread_type());
        CDvdb()->setVariable(VA_CifLayerMode, buf);
    }
    else
        CDvdb()->clearVariable(VA_CifLayerMode);
}


void
QTconvertInDlg::noflvias_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoFlattenStdVias, 0);
    else
        CDvdb()->clearVariable(VA_NoFlattenStdVias);
}


void
QTconvertInDlg::noflpcs_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoFlattenPCells, 0);
    else
        CDvdb()->clearVariable(VA_NoFlattenPCells);
}


void
QTconvertInDlg::nofllbs_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoFlattenLabels, 0);
    else
        CDvdb()->clearVariable(VA_NoFlattenLabels);
}


void
QTconvertInDlg::nolabels_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoReadLabels, 0);
    else
        CDvdb()->clearVariable(VA_NoReadLabels);
}


void
QTconvertInDlg::merg_menu_slot(int i)
{
    if (i > 0) {
        cvi_merg_val = i;
        // Can't use window/flatten ro scale factor.
        cvi_wnd->setEnabled(false);
        cvi_sb_scale->setEnabled(false);
    }
    else if (i == 0) {
        cvi_merg_val = i;
        cvi_wnd->setEnabled(true);
        cvi_sb_scale->setEnabled(true);
    }
}


void
QTconvertInDlg::scale_changed_slot(double d)
{
    /*XXX
    const char *s = Cvi->sb_scale.get_string();
    char *endp;
    double d = strtod(s, &endp);
    if (endp > s && d >= CDSCALEMIN && d <= CDSCALEMAX)
    */
        FIO()->SetReadScale(d);
}


void
QTconvertInDlg::read_btn_slot()
{
    if (!cvi_callback ||
            !(*cvi_callback)(QTconvertInDlg::cvi_merg_val, cvi_arg))
        Cvt()->PopUpImport(0, MODE_OFF, 0, 0);
}


void
QTconvertInDlg::dismiss_btn_slot()
{
    Cvt()->PopUpImport(0, MODE_OFF, 0, 0);
}

