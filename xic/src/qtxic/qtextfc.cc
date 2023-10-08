
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

#include "qtextfc.h"
#include "ext.h"
#include "ext_fc.h"
#include "ext_fxunits.h"
#include "ext_fxjob.h"
#include "dsp_inlines.h"
#include "dsp_color.h"
#include "menu.h"
#include "select.h"
#include "tech.h"
#include "tech_extract.h"
#include "qtinterf/qttextw.h"
#include "qtinterf/qtinput.h"

#include <QLayout>
#include <QTabWidget>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QCheckBox>
#include <QMouseEvent>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>


//-----------------------------------------------------------------------------
// Pop-up to control FasterCap/FastCap-WR interface
//
// Help system keywords used:
//  fcpanel


// Pop up a panel to control the fastcap/fasthenry interface.
//
void
cFC::PopUpExtIf(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTfastCapDlg::self())
            QTfastCapDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTfastCapDlg::self())
            QTfastCapDlg::self()->update();
        return;
    }
    if (QTfastCapDlg::self())
        return;

    new QTfastCapDlg(caller);

    QTdev::self()->SetPopupLocation(GRloc(LW_LR), QTfastCapDlg::self(),
        QTmainwin::self()->Viewport());
    QTfastCapDlg::self()->show();
    setPopUpVisible(true);
}


void
cFC::updateString()
{
    if (QTfastCapDlg::self()) {
        char *s = statusString();
        QTfastCapDlg::self()->update_label(s);
        delete [] s;
    }
}


void
cFC::updateMarks()
{
    DSP()->EraseCrossMarks(Physical, CROSS_MARK_FC);
    if (QTfastCapDlg::self())
        QTfastCapDlg::self()->update_numbers();
}


void
cFC::clearMarks()
{
    DSP()->EraseCrossMarks(Physical, CROSS_MARK_FC);
    delete [] fc_groups;
    fc_groups = 0;
    fc_ngroups = 0;
    if (QTfastCapDlg::self())
        QTfastCapDlg::self()->clear_numbers();
}
// End of cFC functions.


QTfastCapDlg *QTfastCapDlg::instPtr;

QTfastCapDlg::QTfastCapDlg(GRobject c)
{
    instPtr = this;
    fc_caller = c;
    fc_label = 0;
    fc_foreg = 0;
    fc_out = 0;
    fc_shownum = 0;
    fc_file = 0;
    fc_args = 0;
    fc_path = 0;
    fc_units = 0;
    fc_enab = 0;
    fc_sb_plane_bloat = 0;
    fc_sb_substrate_thickness = 0;
    fc_sb_substrate_eps = 0;
    fc_sb_panel_target = 0;
    fc_dbg_zoids = 0;
    fc_dbg_vrbo = 0;
    fc_dbg_nm = 0;
    fc_dbg_czbot = 0;
    fc_dbg_dzbot = 0;
    fc_dbg_cztop = 0;
    fc_dbg_dztop = 0;
    fc_dbg_cyl = 0;
    fc_dbg_dyl = 0;
    fc_dbg_cyu = 0;
    fc_dbg_dyu = 0;
    fc_dbg_cleft = 0;
    fc_dbg_dleft = 0;
    fc_dbg_cright = 0;
    fc_dbg_dright = 0;
    fc_jobs = 0;
    fc_kill = 0;
    fc_no_reset = false;
    fc_frozen = false;
    fc_start = 0;
    fc_end = 0;
    fc_line_selected = -1;

    setWindowTitle(tr("Cap. Extraction"));
    setAttribute(Qt::WA_DeleteOnClose);

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
    QLabel *label = new QLabel(tr("Fast[er]Cap Interface"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    QTabWidget *nbook = new QTabWidget();
    vbox->addWidget(nbook);
    connect(nbook, SIGNAL(currentIndexChanged(int)),
        this, SLOT(page_changed_slot(int)));

    // Run page.
    //
    QWidget *page = new QWidget();
    nbook->addTab(page, tr("Run"));
    QVBoxLayout *vb = new QVBoxLayout(page);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);
    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    fc_foreg = new QCheckBox(tr("Run in foreground"));
    hb->addWidget(fc_foreg);
    connect(fc_foreg, SIGNAL(stateChanged(int)),
        this, SLOT(foreg_btn_slot(int)));

    fc_out = new QCheckBox(tr("Out to console"));
    hb->addWidget(fc_out);
    connect(fc_out, SIGNAL(stateChanged(int)),
        this, SLOT(console_btn_slot(int)));

    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    fc_shownum = new QCheckBox(tr("Show Numbers"));
    hb->addWidget(fc_shownum);
    connect(fc_shownum, SIGNAL(stateChanged(int)),
        this, SLOT(shownum_btn_slot(int)));

    vb->addStretch(1);

    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    btn = new QPushButton(tr("Run File"));
    hb->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(runfile_btn_slot()));

    fc_file = new QLineEdit();
    hb->addWidget(fc_file);;

    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    btn = new QPushButton("Run Extraction");
    hb->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(runext_btn_slot()));

    btn = new QPushButton(tr("Dump Unified List File"));
    hb->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dumplist_btn_slot()));

    gb = new QGroupBox(tr("FcArgs"));
    vb->addWidget(gb);
    hb = new QHBoxLayout(gb);

    fc_args = new QLineEdit();
    fc_args->setText(fc_def_string(FcArgs));
    hb->addWidget(fc_args);
    connect(fc_args, SIGNAL(textChanged(const QString&)),
        this, SLOT(args_changed_slot(const QString&)));

    gb = new QGroupBox(tr("Path to FasterCap or FastCap-WR"));
    vb->addWidget(gb);
    hb = new QHBoxLayout(gb);

    fc_path = new QLineEdit();;
    fc_path->setText(fc_def_string(FcPath));
    hb->addWidget(fc_path);
    connect(fc_path, SIGNAL(textChanged(const QString&)),
        this, SLOT(path_changed_slot(const QString&)));

    // Params page.
    //
    page = new QWidget();
    nbook->addTab(page, tr("Parms"));
    QGridLayout *grid = new QGridLayout(page);

    gb = new QGroupBox("FcPlaneBloat");
    grid->addWidget(gb, 0, 0);
    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);

    int ndgt = CD()->numDigits();

    fc_sb_plane_bloat = new QDoubleSpinBox();
    fc_sb_plane_bloat->setMinimum(FC_PLANE_BLOAT_MIN);
    fc_sb_plane_bloat->setMaximum(FC_PLANE_BLOAT_MAX);
    fc_sb_plane_bloat->setDecimals(ndgt);
    fc_sb_plane_bloat->setValue(FC_PLANE_BLOAT_DEF);
    hb->addWidget(fc_sb_plane_bloat);
    connect(fc_sb_plane_bloat, SIGNAL(valueChanged(double)),
        this, SLOT(plane_bloat_changed_slot(double)));

    gb = new QGroupBox("SubstrateThickness");
    grid->addWidget(gb, 0, 1);
    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);

    fc_sb_substrate_thickness = new QDoubleSpinBox();
    fc_sb_substrate_thickness->setMinimum(SUBSTRATE_THICKNESS_MIN);
    fc_sb_substrate_thickness->setMaximum(SUBSTRATE_THICKNESS_MAX);
    fc_sb_substrate_thickness->setDecimals(ndgt);
    fc_sb_substrate_thickness->setValue(SUBSTRATE_THICKNESS);
    hb->addWidget(fc_sb_substrate_thickness);
    connect(fc_sb_substrate_thickness, SIGNAL(valueChanged(double)),
        this, SLOT(subthick_changed_slot(double)));

    gb = new QGroupBox("FcUnits");
    grid->addWidget(gb, 1, 0);
    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);

    fc_units = new QComboBox();;
    hb->addWidget(fc_units);
    for (int i = 0; unit_t::units_strings[i]; i++)
        fc_units->addItem(unit_t::units_strings[i]);
    fc_units->setCurrentIndex(FCif()->getUnitsIndex(0));
    connect(fc_units, SIGNAL(currentIndexChanged(int)),
        this, SLOT(units_changed_slot(int)));

    gb = new QGroupBox("SubstrateEps");
    grid->addWidget(gb, 1, 1);
    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);

    fc_sb_substrate_eps = new QDoubleSpinBox();
    fc_sb_substrate_eps->setMinimum(SUBSTRATE_EPS_MIN);
    fc_sb_substrate_eps->setMaximum(SUBSTRATE_EPS_MAX);
    fc_sb_substrate_eps->setDecimals(3);
    fc_sb_substrate_eps->setValue(SUBSTRATE_EPS);
    hb->addWidget(fc_sb_substrate_eps);
    connect(fc_sb_substrate_eps, SIGNAL(valueChanged(double)),
        this, SLOT(subeps_changed_slot(double)));

    vb->addStretch(1);

    gb = new QGroupBox();
    vb->addWidget(gb);
    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);

    label = new QLabel(tr("FastCap Panel Refinement"));
    hb->addWidget(label);

    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);
    vb->addLayout(hb);

    fc_enab = new QCheckBox(tr("Enable"));
    hb->addWidget(fc_enab);
    connect(fc_enab, SIGNAL(stteChanged(int)),
        this, SLOT(enable_btn_slot(int)));

    gb = new QGroupBox("FcPanelTarget");
    hb->addWidget(gb);
    QHBoxLayout *hb1 = new QHBoxLayout(gb);
    hb1->setContentsMargins(qmtop);
    hb1->setSpacing(2);


//XXX This is exponential display, can QT do this?
    fc_sb_panel_target = new QDoubleSpinBox();
    fc_sb_panel_target->setMinimum(FC_MIN_TARG_PANELS);
    fc_sb_panel_target->setMaximum(FC_MAX_TARG_PANELS);
    fc_sb_panel_target->setDecimals(1);
    fc_sb_panel_target->setValue(FC_DEF_TARG_PANELS);
    hb1->addWidget(fc_sb_panel_target);
    connect(fc_sb_panel_target, SIGNAL(valueChanged(double)),
        this, SLOT(panels_changed_slot(double)));

    // Debug page
    // This is invisible unless the variable FcDebug is set.
    //
    page = new QWidget();
    nbook->addTab(page, tr("Debug"));
    grid = new QGridLayout(page);
    nbook->setTabVisible(2, CDvdb()->getVariable(VA_FcDebug));

    fc_dbg_zoids = new QCheckBox("Zoids");
    grid->addWidget(fc_dbg_zoids, 0, 0);
    connect(fc_dbg_zoids, SIGNAL(stateChanged(int)),
        this, SLOT(zoid_dbg_btn_slot(int)));

    fc_dbg_vrbo = new QCheckBox(tr("Verbose Out"));
    grid->addWidget(fc_dbg_vrbo, 0, 1);
    connect(fc_dbg_vrbo, SIGNAL(stateChanged(int)),
        this, SLOT(vrbo_dbg_btn_slot(int)));

    fc_dbg_nm = new QCheckBox(tr("No Merge"));
    grid->addWidget(fc_dbg_nm, 1, 0);
    connect(fc_dbg_nm, SIGNAL(stateChanged(int)),
        this, SLOT(nm_dbg_btn_slot(int)));

    fc_dbg_czbot = new QCheckBox("C zbot");
    grid->addWidget(fc_dbg_czbot, 2, 0);
    connect(fc_dbg_czbot, SIGNAL(stateChanged(int)),
        this, SLOT(czbot_dbg_btn_slot(int)));

    fc_dbg_dzbot = new QCheckBox("D zbot");
    grid->addWidget(fc_dbg_dzbot, 2, 1);
    connect(fc_dbg_dzbot, SIGNAL(stateChanged(int)),
        this, SLOT(dzbot_dbg_btn_slot(int)));

    fc_dbg_cztop = new QCheckBox("C ztop");
    grid->addWidget(fc_dbg_cztop, 3, 0);
    connect(fc_dbg_cztop, SIGNAL(stateChanged(int)),
        this, SLOT(cztop_dbg_btn_slot(int)));

    fc_dbg_dztop = new QCheckBox("D ztop");
    grid->addWidget(fc_dbg_dztop, 3, 1);
    connect(fc_dbg_dztop, SIGNAL(stateChanged(int)),
        this, SLOT(dztop_dbg_btn_slot(int)));

    fc_dbg_cyl = new QCheckBox("C yl");
    grid->addWidget(fc_dbg_cyl, 4, 0);
    connect(fc_dbg_cyl, SIGNAL(stateChanged(int)),
        this, SLOT(cyl_dbg_btn_slot(int)));

    fc_dbg_dyl = new QCheckBox("D yl");
    grid->addWidget(fc_dbg_dyl, 4, 1);
    connect(fc_dbg_dyl, SIGNAL(stateChanged(int)),
        this, SLOT(dyl_dbg_btn_slot(int)));

    fc_dbg_cyu = new QCheckBox("C yu");
    grid->addWidget(fc_dbg_cyu, 5, 0);
    connect(fc_dbg_cyu, SIGNAL(stateChanged(int)),
        this, SLOT(cyu_dbg_btn_slot(int)));

    fc_dbg_dyu = new QCheckBox("D yu");
    grid->addWidget(fc_dbg_dyu, 5, 1);
    connect(fc_dbg_dyu, SIGNAL(stateChanged(int)),
        this, SLOT(dyu_dbg_btn_slot(int)));

    fc_dbg_cleft = new QCheckBox("C left");
    grid->addWidget(fc_dbg_cleft, 6, 0);
    connect(fc_dbg_cleft, SIGNAL(stateChanged(int)),
        this, SLOT(cleft_dbg_btn_slot(int)));

    fc_dbg_dleft = new QCheckBox("D left");
    grid->addWidget(fc_dbg_dleft, 6, 1);
    connect(fc_dbg_dleft, SIGNAL(stateChanged(int)),
        this, SLOT(dleft_dbg_btn_slot(int)));

    fc_dbg_cright = new QCheckBox("C right");
    grid->addWidget(fc_dbg_cright, 7, 0);
    connect(fc_dbg_cright, SIGNAL(stateChanged(int)),
        this, SLOT(cright_dbg_btn_slot(int)));

    fc_dbg_dright = new QCheckBox("D right");
    grid->addWidget(fc_dbg_dright, 7, 1);
    connect(fc_dbg_dright, SIGNAL(stateChanged(int)),
        this, SLOT(dright_dbg_btn_slot(int)));

    // Jobs page
    //
    page = new QWidget();
    nbook->addTab(page, tr("Jobs"));
    vb = new QVBoxLayout(page);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    fc_jobs = new QTtextEdit();
    vb->addWidget(fc_jobs);
    fc_jobs->setReadOnly(true);
    fc_jobs->setMouseTracking(true);
    connect(fc_jobs, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_FIXED))
        fc_jobs->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    fc_kill = new QPushButton(tr("Abort job"));
    vb->addWidget(fc_kill);
    connect(fc_kill, SIGNAL(clicked()), this, SLOT(abort_btn_slot()));

    // End of pages.
    // Status line and Dismiss button.
    //
    gb = new QGroupBox();
    vbox->addWidget(gb);
    hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    char *s = FCif()->statusString();
    fc_label = new QLabel(s);
    fc_label->setAlignment(Qt::AlignCenter);
    delete [] s;
    hbox->addWidget(fc_label);

    btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTfastCapDlg::~QTfastCapDlg()
{
    FCif()->showMarks(false);
    instPtr = 0;
    if (fc_caller)
        QTdev::Deselect(fc_caller);
    FCif()->setPopUpVisible(false);
}


void
QTfastCapDlg::update()
{
    const char *var, *cur;
    fc_no_reset = true;

    // Run page
    QTdev::SetStatus(fc_foreg, CDvdb()->getVariable(VA_FcForeg));
    QTdev::SetStatus(fc_out, CDvdb()->getVariable(VA_FcMonitor));

    var = CDvdb()->getVariable(VA_FcArgs);
    if (!var)
        var = fc_def_string(FcArgs);
    cur = fc_args->text().toLatin1().constData();
    if (!cur)
        cur = "";
    if (strcmp(var, cur))
        fc_args->setText(var);

    var = CDvdb()->getVariable(VA_FcPath);
    if (!var)
        var = fc_def_string(FcPath);
    cur = fc_path->text().toLatin1().constData();
    if (!cur)
        cur = "";
    if (strcmp(var, cur))
        fc_path->setText(var);

    // Params page
    var = CDvdb()->getVariable(VA_FcPlaneBloat);
    QString qs(var);
    int z = 0;
    if (fc_sb_plane_bloat->validate(qs, z) == QValidator::Acceptable)
        fc_sb_plane_bloat->setValue(atof(var));
    else {
        if (var)
            CDvdb()->clearVariable(VA_FcPlaneBloat);
        fc_sb_plane_bloat->setValue(FC_PLANE_BLOAT_DEF);
    }

    var = CDvdb()->getVariable(VA_SubstrateThickness);
    qs = QString(var);
    if (fc_sb_substrate_thickness->validate(qs, z) == QValidator::Acceptable)
        fc_sb_substrate_thickness->setValue(atof(var));
    else {
        if (var)
            CDvdb()->clearVariable(VA_SubstrateThickness);
        fc_sb_substrate_thickness->setValue(SUBSTRATE_THICKNESS);
    }

    var = CDvdb()->getVariable(VA_SubstrateEps);
    qs = QString(var);
    if (fc_sb_substrate_eps->validate(qs, z) == QValidator::Acceptable)
        fc_sb_substrate_eps->setValue(atof(var));
    else {
        if (var)
            CDvdb()->clearVariable(VA_SubstrateEps);
        fc_sb_substrate_eps->setValue(SUBSTRATE_EPS);
    }

    var = CDvdb()->getVariable(VA_FcUnits);
    if (!var)
        var = "";
    int uoff = FCif()->getUnitsIndex(var);
    int ucur = fc_units->currentIndex();
    if (uoff != ucur)
        fc_units->setCurrentIndex(uoff);

    static double fcpt_bak;
    var = CDvdb()->getVariable(VA_FcPanelTarget);
    qs = QString(var);
    if (fc_sb_panel_target->validate(qs, z) == QValidator::Acceptable) {
        fc_sb_panel_target->setValue(atof(var));
        fc_sb_panel_target->setEnabled(true);
        QTdev::SetStatus(fc_enab, true);
        fcpt_bak = fc_sb_panel_target->value();
    }
    else {
        if (var)
            CDvdb()->clearVariable(VA_FcPanelTarget);
        if (fcpt_bak > 0.0)
            fc_sb_panel_target->setValue(fcpt_bak);
        fc_sb_panel_target->setEnabled(false);
        QTdev::SetStatus(fc_enab, false);
    }

    // Debug page
    QTdev::SetStatus(fc_dbg_zoids, CDvdb()->getVariable(VA_FcZoids));
    QTdev::SetStatus(fc_dbg_vrbo, CDvdb()->getVariable(VA_FcVerboseOut));
    QTdev::SetStatus(fc_dbg_nm, CDvdb()->getVariable(VA_FcNoMerge));

    unsigned int flgs = MRG_ALL;
    const char *s = CDvdb()->getVariable(VA_FcMergeFlags);
    if (s)
        flgs = Tech()->GetInt(s) & MRG_ALL;
    QTdev::SetStatus(fc_dbg_czbot, (flgs & MRG_C_ZBOT));
    QTdev::SetStatus(fc_dbg_dzbot, (flgs & MRG_D_ZBOT));
    QTdev::SetStatus(fc_dbg_cztop, (flgs & MRG_C_ZTOP));
    QTdev::SetStatus(fc_dbg_dztop, (flgs & MRG_D_ZTOP));
    QTdev::SetStatus(fc_dbg_cyl, (flgs & MRG_C_YL));
    QTdev::SetStatus(fc_dbg_dyl, (flgs & MRG_D_YL));
    QTdev::SetStatus(fc_dbg_cyu, (flgs & MRG_C_YU));
    QTdev::SetStatus(fc_dbg_dyu, (flgs & MRG_D_YU));
    QTdev::SetStatus(fc_dbg_cleft, (flgs & MRG_C_LEFT));
    QTdev::SetStatus(fc_dbg_dleft, (flgs & MRG_D_LEFT));
    QTdev::SetStatus(fc_dbg_cright, (flgs & MRG_C_RIGHT));
    QTdev::SetStatus(fc_dbg_dright, (flgs & MRG_D_RIGHT));

    // Jobs page
    update_jobs_list();

    fc_no_reset = false;
}


void
QTfastCapDlg::update_jobs_list()
{
    if (!fc_jobs)
        return;
    QColor c1 = QTbag::PopupColor(GRattrColorHl4);

    int pid = get_pid();
    if (pid <= 0)
        fc_kill->setEnabled(false);

    double val = fc_jobs->get_scroll_value();
    fc_jobs->set_chars("");

    char *list = FCif()->jobList();
    if (!list) {
        fc_jobs->setTextColor(c1);
        fc_jobs->set_chars("No background jobs running.");
    }
    else {
        fc_jobs->setTextColor(QColor("black"));
        fc_jobs->set_chars(list);
    }
    fc_jobs->set_scroll_value(val);
    if (pid > 0)
        select_pid(pid);
    fc_kill->setEnabled(get_pid() > 0);
}


void
QTfastCapDlg::update_label(const char *s)
{
    fc_label->setText(s);
}


void
QTfastCapDlg::update_numbers()
{
    if (QTdev::GetStatus(fc_shownum))
        FCif()->showMarks(true);
}


void
QTfastCapDlg::clear_numbers()
{
    QTdev::SetStatus(fc_shownum, false);
}


// Select the chars in the range, start=end deselects existing.
//
void
QTfastCapDlg::select_range(int start, int end)
{
    fc_jobs->select_range(start, end);
    fc_start = start;
    fc_end = end;
}


// Return the PID value of the selected line, or -1.
//
int
QTfastCapDlg::get_pid()
{
    if (fc_line_selected < 0)
        return (-1);
    char *string = fc_jobs->get_chars();
    int line = 0;
    for (const char *s = string; *s; s++) {
        if (line == fc_line_selected) {
            while (isspace(*s))
                s++;
            int pid;
            int r = sscanf(s, "%d", &pid);
            delete [] string;
            return (r == 1 ? pid : -1);
        }
        if (*s == '\n')
            line++;
    }
    delete [] string;
    return (-1);
}


void
QTfastCapDlg::select_pid(int p)
{
    char *string = fc_jobs->get_chars();
    bool nl = true;
    int line = 0;
    const char *cs = 0;
    for (const char *s = string; *s; s++) {
        if (nl) {
            cs = s;
            while (isspace(*s))
                s++;
            nl = false;
            int pid;
            int r = sscanf(s, "%d", &pid);
            if (r == 1 && p == pid) {
                const char *ce = cs;
                while (*ce && *ce != 'n')
                    ce++;
                select_range(cs - string, ce - string);
                delete [] string;
                fc_line_selected = line;
                fc_kill->setEnabled(true);
                return;
            }
        }
        if (*s == '\n') {
            nl = true;
            line++;
        }
    }
    delete [] string;
}


// Static function.
// Return the default text field text.
//
const char *
QTfastCapDlg::fc_def_string(int id)
{
    int ndgt = CD()->numDigits();
    static char tbuf[16];
    switch (id) {
    case FcPath:
        return (fxJob::fc_default_path());
    case FcArgs:
        return ("");
    case FcPlaneBloat:
        snprintf(tbuf, sizeof(tbuf), "%.*f", ndgt, FC_PLANE_BLOAT_DEF);
        return (tbuf);
    case SubstrateThickness:
        snprintf(tbuf, sizeof(tbuf), "%.*f", ndgt, SUBSTRATE_THICKNESS);
        return (tbuf);
    case SubstrateEps:
        snprintf(tbuf, sizeof(tbuf), "%.*f", 3, SUBSTRATE_EPS);
        return (tbuf);
    case FcPanelTarget:
        snprintf(tbuf, sizeof(tbuf), "%.*e", 1, FC_DEF_TARG_PANELS);
        return (tbuf);
    }
    return ("");
}


void
QTfastCapDlg::debug_btn_hdlr(int state, int val)
{
    unsigned int mrgflgs = MRG_ALL;
    const char *s = CDvdb()->getVariable(VA_FcMergeFlags);
    if (s)
        mrgflgs = Tech()->GetInt(s) & MRG_ALL;
    unsigned bkflgs = mrgflgs;
    if (state)
        mrgflgs |= val;
    else
        mrgflgs &= ~val;
    if (mrgflgs != bkflgs) {
        if (mrgflgs == MRG_ALL)
            CDvdb()->clearVariable(VA_FcMergeFlags);
        else {
            char buf[32];
            snprintf(buf, sizeof(buf), "0x%x", mrgflgs);
            CDvdb()->setVariable(VA_FcMergeFlags, buf);
        }
    }
}


// Static function.
void
QTfastCapDlg::fc_p_cb(bool ok, void *arg)
{
    char *fname = (char*)arg;
    if (ok)
        DSPmainWbag(PopUpFileBrowser(fname))
    delete [] fname;
}


// Static function.
void
QTfastCapDlg::fc_dump_cb(const char *fname, void *client_data)
{
    switch ((intptr_t)client_data) {
    case FcDump:
        if (FCif()->fcDump(fname)) {
            if (!QTfastCapDlg::self())
                return;
            const char *fn = lstring::strip_path(fname);
            char tbuf[256];
            snprintf(tbuf, sizeof(tbuf),
                "Input is in file %s.  View file? ", fn);
            QTfastCapDlg::self()->PopUpAffirm(0, GRloc(LW_UL), tbuf, fc_p_cb,
                lstring::copy(fname));
        }
        break;
    }
    if (QTfastCapDlg::self() && QTfastCapDlg::self()->wb_input)
        QTfastCapDlg::self()->wb_input->popdown();
}


void
QTfastCapDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("fcpanel"))
}


void
QTfastCapDlg::page_changed_slot(int)
{
}


void
QTfastCapDlg::foreg_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_FcForeg, "");
    else
        CDvdb()->clearVariable(VA_FcForeg);
}


void
QTfastCapDlg::console_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_FcMonitor, "");
    else
        CDvdb()->clearVariable(VA_FcMonitor);
}


void
QTfastCapDlg::shownum_btn_slot(int state)
{
    FCif()->showMarks(state);
}


void
QTfastCapDlg::runfile_btn_slot()
{
    const char *s = fc_file->text().toLatin1().constData();
    char *tok = lstring::getqtok(&s);
    if (tok) {
        FCif()->fcRun(tok, 0, 0, true);
        delete [] tok;
    }
    else
        PopUpErr(MODE_ON, "No file name given!");
}


void
QTfastCapDlg::runext_btn_slot()
{
    FCif()->fcRun(0, 0, 0);
}


void
QTfastCapDlg::dumplist_btn_slot()
{
    char *s = FCif()->getFileName(FC_LST_SFX);
    PopUpInput(0, s, "Dump", fc_dump_cb, (void*)FcDump);
    delete [] s;
}


void
QTfastCapDlg::args_changed_slot(const QString &qs)
{
    if (fc_no_reset)
        return;
    if (!strcmp(qs.toLatin1().constData(), fc_def_string(FcArgs)))
        CDvdb()->clearVariable(VA_FcArgs);
    else
        CDvdb()->setVariable(VA_FcArgs, qs.toLatin1().constData());
}


void
QTfastCapDlg::path_changed_slot(const QString &qs)
{
    if (fc_no_reset)
        return;
    if (!strcmp(qs.toLatin1().constData(), fc_def_string(FcPath)))
        CDvdb()->clearVariable(VA_FcPath);
    else
        CDvdb()->setVariable(VA_FcPath, qs.toLatin1().constData());
}


void
QTfastCapDlg::plane_bloat_changed_slot(double)
{
    if (fc_no_reset)
        return;
    if (fc_sb_plane_bloat->cleanText() == fc_def_string(FcPlaneBloat))
        CDvdb()->clearVariable(VA_FcPlaneBloat);
    else {
        CDvdb()->setVariable(VA_FcPlaneBloat,
            fc_sb_plane_bloat->cleanText().toLatin1().constData());
    }
}


void
QTfastCapDlg::subthick_changed_slot(double)
{
    if (fc_no_reset)
        return;
    if (fc_sb_substrate_thickness->cleanText() ==
            fc_def_string(SubstrateThickness))
        CDvdb()->clearVariable(VA_SubstrateThickness);
    else {
        CDvdb()->setVariable(VA_SubstrateThickness,
            fc_sb_substrate_thickness->cleanText().toLatin1().constData());
    }
}


void
QTfastCapDlg::units_changed_slot(int i)
{
    const char *str = FCif()->getUnitsString(unit_t::units_strings[i]);
    if (str)
        CDvdb()->setVariable(VA_FcUnits, str);
    else
        CDvdb()->clearVariable(VA_FcUnits);
}


void
QTfastCapDlg::subeps_changed_slot(double)
{
    if (fc_no_reset)
        return;
    if (fc_sb_substrate_eps->cleanText() == fc_def_string(SubstrateEps))
        CDvdb()->clearVariable(VA_SubstrateEps);
    else {
        CDvdb()->setVariable(VA_SubstrateEps,
            fc_sb_substrate_eps->cleanText().toLatin1().constData());
    }
}


void
QTfastCapDlg::enable_btn_slot(int state)
{
    if (state) {
        QString qs = fc_sb_panel_target->cleanText();
        int z = 0;
        if (fc_sb_panel_target->validate(qs, z) == QValidator::Acceptable)
            CDvdb()->setVariable(VA_FcPanelTarget, qs.toLatin1().constData());
        else {
            char tbf[32];
            snprintf(tbf, sizeof(tbf), "%.1e", FC_DEF_TARG_PANELS);
            CDvdb()->setVariable(VA_FcPanelTarget, tbf);
        }
        fc_sb_panel_target->setEnabled(true);
    }
    else {
        CDvdb()->clearVariable(VA_FcPanelTarget);
        fc_sb_panel_target->setEnabled(false);
    }
}


void
QTfastCapDlg::panels_changed_slot(double)
{
    if (fc_no_reset)
        return;
    if (fc_sb_panel_target->cleanText() == fc_def_string(FcPanelTarget))
        CDvdb()->clearVariable(VA_FcPanelTarget);
    else {
        CDvdb()->setVariable(VA_FcPanelTarget,
            fc_sb_panel_target->cleanText().toLatin1().constData());
    }
}


void
QTfastCapDlg::abort_btn_slot()
{
    int pid = get_pid();
    if (pid > 0)
        FCif()->killProcess(pid);
}


void
QTfastCapDlg::dismiss_btn_slot()
{
    FCif()->PopUpExtIf(0, MODE_OFF);
}


void
QTfastCapDlg::zoid_dbg_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_FcZoids, "");
    else
        CDvdb()->clearVariable(VA_FcZoids);
}


void
QTfastCapDlg::vrbo_dbg_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_FcVerboseOut, "");
    else
        CDvdb()->clearVariable(VA_FcVerboseOut);
}


void
QTfastCapDlg::nm_dbg_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_FcNoMerge, "");
    else
        CDvdb()->clearVariable(VA_FcNoMerge);
}


void
QTfastCapDlg::czbot_dbg_btn_slot(int state)
{
    debug_btn_hdlr(state, MRG_C_ZBOT);
}


void
QTfastCapDlg::dzbot_dbg_btn_slot(int state)
{
    debug_btn_hdlr(state, MRG_D_ZBOT);
}


void
QTfastCapDlg::cztop_dbg_btn_slot(int state)
{
    debug_btn_hdlr(state, MRG_C_ZTOP);
}


void
QTfastCapDlg::dztop_dbg_btn_slot(int state)
{
    debug_btn_hdlr(state, MRG_D_ZTOP);
}


void
QTfastCapDlg::cyl_dbg_btn_slot(int state)
{
    debug_btn_hdlr(state, MRG_C_YL);
}


void
QTfastCapDlg::dyl_dbg_btn_slot(int state)
{
    debug_btn_hdlr(state, MRG_D_YL);
}


void
QTfastCapDlg::cyu_dbg_btn_slot(int state)
{
    debug_btn_hdlr(state, MRG_C_YU);
}


void
QTfastCapDlg::dyu_dbg_btn_slot(int state)
{
    debug_btn_hdlr(state, MRG_D_YU);
}


void
QTfastCapDlg::cleft_dbg_btn_slot(int state)
{
    debug_btn_hdlr(state, MRG_C_LEFT);
}


void
QTfastCapDlg::dleft_dbg_btn_slot(int state)
{
    debug_btn_hdlr(state, MRG_D_LEFT);
}


void
QTfastCapDlg::cright_dbg_btn_slot(int state)
{
    debug_btn_hdlr(state, MRG_C_RIGHT);
}


void
QTfastCapDlg::dright_dbg_btn_slot(int state)
{
    debug_btn_hdlr(state, MRG_D_RIGHT);
}


void
QTfastCapDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    const char *str = lstring::copy(
        (const char*)fc_jobs->toPlainText().toLatin1());
    int x = ev->x();
    int y = ev->y();
    QTextCursor cur = fc_jobs->cursorForPosition(QPoint(x, y));
    int pos = cur.position();
    
    if (isspace(str[pos])) {
        // Clicked on white space.
        delete [] str;
        return;
    }

    const char *lineptr = str;
    int line = 0;
    for (int i = 0; i <= pos; i++) {
        if (str[i] == '\n') {
            if (i == pos) {
                // Clicked to right of line.
                delete [] str;
                return;
            }
            line++;
            lineptr = str + i+1;
        }
    }
    if (lineptr && *lineptr != '\n') {

        int start = lineptr - str;
        int end = start;
        while (str[end] && str[end] != '\n')
            end++;

        fc_line_selected = line;
        select_range(start, end);
        delete [] str;
        fc_kill->setEnabled(get_pid() > 0);
    }

    fc_line_selected = -1;
    delete [] str;
    select_range(0, 0);
    fc_kill->setEnabled(false);
}


void
QTfastCapDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_FIXED))
            fc_jobs->setFont(*fnt);
        update_jobs_list();
    }
}

