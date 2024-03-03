
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

#include "qtextfh.h"
#include "ext.h"
#include "ext_fh.h"
#include "ext_fxunits.h"
#include "ext_fxjob.h"
#include "tech_layer.h"
#include "dsp_inlines.h"
#include "dsp_color.h"
#include "menu.h"
#include "select.h"
#include "qtinterf/qttextw.h"
#include "qtinterf/qtinput.h"
#include "qtinterf/qtdblsb.h"

#include <QLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>


//-----------------------------------------------------------------------------
// QTfastHenryDlg:  Dialog to control FastHenry interface.
// Called from main menu: Extract/ExtrCT RL.
//
// Help system keywords used:
//  fhpanel

// Pop up a panel to control the fastcap/fasthenry interface.
//
void
cFH::PopUpExtIf(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTfastHenryDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTfastHenryDlg::self())
            QTfastHenryDlg::self()->update();
        return;
    }
    if (QTfastHenryDlg::self())
        return;

    new QTfastHenryDlg(caller);

    QTfastHenryDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_LR), QTfastHenryDlg::self(),
        QTmainwin::self()->Viewport());
    QTfastHenryDlg::self()->show();
    setPopUpVisible(true);
}


void
cFH::updateString()
{
    if (QTfastHenryDlg::self()) {
        char *s = statusString();
        QTfastHenryDlg::self()->update_label(s);
        delete [] s;
    }
}
// End of cFH functions.


QTfastHenryDlg *QTfastHenryDlg::instPtr;

QTfastHenryDlg::QTfastHenryDlg(GRobject c) : QTbag(this)
{
    instPtr = this;
    fh_caller = c;
    fh_label = 0;

    fh_foreg = 0;
    fh_out = 0;
    fh_file = 0;
    fh_args = 0;
    fh_defs = 0;
    fh_fmin = 0;
    fh_fmax = 0;
    fh_ndec = 0;
    fh_path = 0;

    fh_units = 0;
    fh_nhinc_ovr = 0;
    fh_nhinc_fh = 0;
    fh_enab = 0;
    fh_sb_manh_grid_cnt = 0;
    fh_sb_nhinc = 0;
    fh_sb_rh = 0;
    fh_sb_volel_min = 0;
    fh_sb_volel_target = 0;

    fh_jobs = 0;
    fh_kill = 0;

    fh_no_reset = false;
    fh_start = 0;
    fh_end = 0;
    fh_line_selected = -1;

    setWindowTitle(tr("LR Extraction"));
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
    QLabel *label = new QLabel(tr("FastHenry Interface"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    QTabWidget *nbook = new QTabWidget();
    vbox->addWidget(nbook);

    // Run page
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

    fh_foreg = new QCheckBox(tr("Run in foreground"));
    hb->addWidget(fh_foreg);
    connect(fh_foreg, SIGNAL(stateChanged(int)),
        this, SLOT(foreg_btn_slot(int)));

    fh_out = new QCheckBox(tr("Out to console"));
    hb->addWidget(fh_out);
    connect(fh_out, SIGNAL(stateChanged(int)),
        this, SLOT(console_btn_slot(int)));

    vb->addStretch(1);

    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    btn = new QPushButton(tr("Run File"));
    hb->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(runfile_btn_slot()));

    fh_file = new QLineEdit();
    hb->addWidget(fh_file);

    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    btn = new QPushButton(tr("Run FastHenry"));
    hb->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(runext_btn_slot()));

    btn = new QPushButton(tr("Dump FastHenry File"));
    hb->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(dumplist_btn_slot()));

    gb = new QGroupBox("FhArgs");
    vb->addWidget(gb);
    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    fh_args = new QLineEdit();
    hb->addWidget(fh_args);
    connect(fh_args, SIGNAL(textChanged(const QString&)),
        this, SLOT(args_changed_slot(const QString&)));

    gb = new QGroupBox("FhFreq");
    vb->addWidget(gb);
    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    label = new QLabel(" fmin=");
    hb->addWidget(label);
    fh_fmin = new QLineEdit();
    fh_fmin->setMaximumWidth(70);
    hb->addWidget(fh_fmin);
    connect(fh_fmin, SIGNAL(textChanged(const QString&)),
        this, SLOT(fmin_changed_slot(const QString&)));

    label = new QLabel(" fmax=");
    hb->addWidget(label);
    fh_fmax = new QLineEdit();
    fh_fmax->setMaximumWidth(70);
    hb->addWidget(fh_fmax);
    connect(fh_fmax, SIGNAL(textChanged(const QString&)),
        this, SLOT(fmax_changed_slot(const QString&)));

    label = new QLabel(" ndec=");
    hb->addWidget(label);
    fh_ndec = new QLineEdit();
    hb->addWidget(fh_ndec);
    fh_ndec->setMaximumWidth(50);
    connect(fh_ndec, SIGNAL(textChanged(const QString&)),
        this, SLOT(ndec_changed_slot(const QString&)));

    gb = new QGroupBox(tr("Path to FastHenry"));
    vb->addWidget(gb);
    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    fh_path = new QLineEdit();
    hb->addWidget(fh_path);
    connect(fh_path, SIGNAL(textChanged(const QString&)),
        this, SLOT(path_changed_slot(const QString&)));

    // Params page
    //
    page = new QWidget();
    nbook->addTab(page, tr("Params"));
    vb = new QVBoxLayout(page);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);
    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    gb = new QGroupBox("FhUnits");
    hb->addWidget(gb);
    QHBoxLayout *hb1 = new QHBoxLayout(gb);

    fh_units = new QComboBox();
    hb1->addWidget(fh_units);
    for (int i = 0; unit_t::units_strings[i]; i++)
        fh_units->addItem(unit_t::units_strings[i]);
    fh_units->setCurrentIndex(FHif()->getUnitsIndex(0));
    connect(fh_units, SIGNAL(currentIndexChanged(int)),
        this, SLOT(units_changed_slot(int)));

    gb = new QGroupBox("FhManhGridCnt");
    hb->addWidget(gb);
    hb1 = new QHBoxLayout(gb);
    hb1->setContentsMargins(qmtop);
    hb1->setSpacing(2);

    fh_sb_manh_grid_cnt = new QSpinBox();
    hb1->addWidget(fh_sb_manh_grid_cnt);
    fh_sb_manh_grid_cnt->setRange(FH_MIN_MANH_GRID_CNT, FH_MAX_MANH_GRID_CNT);
    fh_sb_manh_grid_cnt->setValue(FH_DEF_MANH_GRID_CNT);
    connect(fh_sb_manh_grid_cnt, SIGNAL(valueChanged(int)),
        this, SLOT(manh_grid_changed_slot(int)));

    gb = new QGroupBox("FhDefaults");
    vb->addWidget(gb);
    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);

    fh_defs = new QLineEdit();
    hb->addWidget(fh_defs);
    connect(fh_defs, SIGNAL(textChanged(const QString&)),
        this, SLOT(defaults_changed_slot(const QString&)));

    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    gb = new QGroupBox("FhDefNhinc");
    vb->addWidget(gb);
    hb1 = new QHBoxLayout(gb);
    hb1->setContentsMargins(qm);
    hb1->setSpacing(0);

    fh_sb_nhinc = new QSpinBox();
    hb1->addWidget(fh_sb_nhinc);
    fh_sb_nhinc->setRange(FH_MIN_DEF_NHINC, FH_MAX_DEF_NHINC);
    fh_sb_nhinc->setValue(DEF_FH_NHINC); 
    connect(fh_sb_nhinc, SIGNAL(valueChanged(int)),
        this, SLOT(nhinc_changed_slot(int)));

    gb = new QGroupBox("FhDefRh");
    hb->addWidget(gb);
    hb1 = new QHBoxLayout(gb);
    hb1->setContentsMargins(qm);
    hb1->setSpacing(0);

    fh_sb_rh = new QTdoubleSpinBox();
    hb1->addWidget(fh_sb_rh);
    fh_sb_rh->setRange(FH_MIN_DEF_RH, FH_MAX_DEF_RH);
    fh_sb_rh->setDecimals(3);
    fh_sb_rh->setValue(DEF_FH_RH);
    connect(fh_sb_rh, SIGNAL(valueChanged(double)),
        this, SLOT(rh_changed_slot(double)));

    fh_nhinc_ovr = new QCheckBox(tr("Override Layer NHINC, RH"));
    vb->addWidget(fh_nhinc_ovr);
    connect(fh_nhinc_ovr, SIGNAL(stateChanged(int)),
        this, SLOT(override_btn_slot(int)));

    fh_nhinc_fh = new QCheckBox(tr("Use FastHenry Internal NHINC, RH"));
    vb->addWidget(fh_nhinc_fh);
    connect(fh_nhinc_fh, SIGNAL(stateChanged(int)),
        this, SLOT(internal_btn_slot(int)));

    gb = new QGroupBox();
    vb->addWidget(gb);
    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(0);

    label = new QLabel(tr("FastHenry Volume Element Refinement"));
    label->setAlignment(Qt::AlignCenter);
    hb->addWidget(label);

    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    fh_enab = new QCheckBox(tr("Enable"));
    hb->addWidget(fh_enab);
    connect(fh_enab, SIGNAL(stateChanged(int)),
        this, SLOT(enable_btn_slot(int)));

    gb = new QGroupBox("FhVolElMin");
    hb->addWidget(gb);
    hb1 = new QHBoxLayout(gb);
    hb1->setContentsMargins(qm);
    hb1->setSpacing(0);

    fh_sb_volel_min = new QTdoubleSpinBox();
    hb1->addWidget(fh_sb_volel_min);
    fh_sb_volel_min->setRange(FH_MIN_VOLEL_MIN, FH_MAX_VOLEL_MIN);
    fh_sb_volel_min->setDecimals(2);
    fh_sb_volel_min->setValue(FH_DEF_VOLEL_MIN);
    connect(fh_sb_volel_min, SIGNAL(valueChanged(double)),
        this, SLOT(volel_min_changed_slot(double)));
    fh_sb_volel_min->setEnabled(false);

    gb = new QGroupBox("FhVolElTarget");
    hb->addWidget(gb);
    hb1 = new QHBoxLayout(gb);
    hb1->setContentsMargins(qm);
    hb1->setSpacing(0);

    fh_sb_volel_target = new QSpinBox();
    hb1->addWidget(fh_sb_volel_target);
    fh_sb_volel_target->setRange(FH_MIN_VOLEL_TARG, FH_MAX_VOLEL_TARG);
    fh_sb_volel_target->setValue(FH_DEF_VOLEL_TARG);
    connect(fh_sb_volel_target, SIGNAL(valueChanged(int)),
        this, SLOT(volel_target_changed_slot(int)));
    fh_sb_volel_target->setEnabled(false);

    // Jobs page
    //
    page = new QWidget();
    nbook->addTab(page, tr("Jobs"));
    vb = new QVBoxLayout(page);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    fh_jobs = new QTtextEdit();
    vb->addWidget(fh_jobs);
    fh_jobs->setReadOnly(true);
    fh_jobs->setMouseTracking(true);
    connect(fh_jobs, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));

    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_FIXED))
        fh_jobs->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    fh_kill = new QPushButton(tr("Abort job"));
    vb->addWidget(fh_kill);
    fh_kill->setAutoDefault(false);
    connect(fh_kill, SIGNAL(clicked()), this, SLOT(abort_btn_slot()));


    // End of pages.
    // Status line and Dismiss button
    //
    gb = new QGroupBox();
    vbox->addWidget(gb);
    hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    const char *s = FHif()->statusString();
    fh_label = new QLabel(s);
    fh_label->setAlignment(Qt::AlignCenter);
    delete [] s;
    hbox->addWidget(fh_label);

    btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTfastHenryDlg::~QTfastHenryDlg()
{
    instPtr = 0;
    if (fh_caller)
        QTdev::Deselect(fh_caller);
    FHif()->setPopUpVisible(false);
}


void
QTfastHenryDlg::update()
{
    const char *var, *cur;
    fh_no_reset = true;

    // Run page
    QTdev::SetStatus(fh_foreg, CDvdb()->getVariable(VA_FhForeg));
    QTdev::SetStatus(fh_out, CDvdb()->getVariable(VA_FhMonitor));

    var = CDvdb()->getVariable(VA_FhArgs);
    if (!var)
        var = fh_def_string(fhArgs);
    cur = fh_args->text().toLatin1().constData();
    if (!cur)
        cur = "";
    if (strcmp(var, cur))
        fh_args->setText(var);

    var = CDvdb()->getVariable(VA_FhDefaults);
    if (!var)
        var = fh_def_string(fhDefaults);
    cur = fh_defs->text().toLatin1().constData();
    if (!cur)
        cur = "";
    if (strcmp(var, cur))
        fh_defs->setText(var);

    var = CDvdb()->getVariable(VA_FhPath);
    if (!var)
        var = fh_def_string(fhPath);
    cur = fh_path->text().toLatin1().constData();
    if (!cur)
        cur = "";
    if (strcmp(var, cur))
        fh_path->setText(var);

    update_fh_freq_widgets();

    // Params page
    var = CDvdb()->getVariable(VA_FhUnits);
    if (!var)
        var = "";
    int uoff = FHif()->getUnitsIndex(var);
    int ucur = fh_units->currentIndex();
    if (uoff != ucur)
        fh_units->setCurrentIndex(uoff);

    // Unlike QDoubleSpinBox, QSpinBox prevents calling validate (private).
    var = CDvdb()->getVariable(VA_FhManhGridCnt);
    QString qs(var);
    bool ok;
    int i = qs.toInt(&ok);
    if (ok && i >= fh_sb_manh_grid_cnt->minimum() &&
            i <= fh_sb_manh_grid_cnt->maximum())
        fh_sb_manh_grid_cnt->setValue(i);
    else {
        if (var)
            CDvdb()->clearVariable(VA_FhManhGridCnt);
        fh_sb_manh_grid_cnt->setValue(FH_DEF_MANH_GRID_CNT);
    }

    var = CDvdb()->getVariable(VA_FhDefNhinc);
    qs = QString(var);
    i = qs.toInt(&ok);
    if (ok && i >= fh_sb_nhinc->minimum() && i <= fh_sb_nhinc->maximum())
        fh_sb_nhinc->setValue(i);
    else {
        if (var)
            CDvdb()->clearVariable(VA_FhDefNhinc);
        fh_sb_nhinc->setValue(DEF_FH_NHINC);
    }

    var = CDvdb()->getVariable(VA_FhDefRh);
    qs = QString(var);
    int z = 0;
    if (fh_sb_rh->validate(qs, z) == QValidator::Acceptable)
        fh_sb_rh->setValue(atof(var));
    else {
        if (var)
            CDvdb()->clearVariable(VA_FhDefRh);
        fh_sb_rh->setValue(DEF_FH_RH);
    }

    QTdev::SetStatus(fh_nhinc_ovr,
        CDvdb()->getVariable(VA_FhOverride));
    QTdev::SetStatus(fh_nhinc_fh,
        CDvdb()->getVariable(VA_FhUseFilament));

    var = CDvdb()->getVariable(VA_FhVolElMin);
    qs = QString(var);
    if (fh_sb_volel_min->validate(qs, z) == QValidator::Acceptable)
        fh_sb_volel_min->setValue(atof(var));
    else {
        if (var)
            CDvdb()->clearVariable(VA_FhVolElMin);
        fh_sb_volel_min->setValue(FH_DEF_VOLEL_MIN);
    }

    var = CDvdb()->getVariable(VA_FhVolElTarget);
    qs = QString(var);
    i = qs.toInt(&ok);
    if (ok && i >= fh_sb_volel_target->minimum() &&
            i <= fh_sb_volel_target->maximum())
        fh_sb_volel_target->setValue(i);
    else {
        if (var)
            CDvdb()->clearVariable(VA_FhVolElTarget);
        fh_sb_volel_target->setValue(FH_DEF_VOLEL_TARG);
    }

    // Jobs page
    update_jobs_list();

    fh_no_reset = false;
}


void
QTfastHenryDlg::update_jobs_list()
{
    if (!fh_jobs)
        return;
    QColor c1 = QTbag::PopupColor(GRattrColorHl4);

    int pid = get_pid();
    if (pid <= 0)
        fh_kill->setEnabled(false);

    double val = fh_jobs->get_scroll_value();
    fh_jobs->set_chars("");

    char *list = FHif()->jobList();
    if (!list) {
        fh_jobs->setTextColor(c1);
        fh_jobs->set_chars("No background jobs running.");
    }
    else {
        fh_jobs->setTextColor(QColor("black"));
        fh_jobs->set_chars(list);
    }
    fh_jobs->set_scroll_value(val);
    if (pid > 0)
        select_pid(pid);
    fh_kill->setEnabled(get_pid() > 0);
}


void
QTfastHenryDlg::update_label(const char *s)
{
    fh_label->setText(s);
}


namespace {
    char *getword(const char *kw, const char *str)
    {
        const char *s = strstr(str, kw);
        if (!s)
            return (lstring::copy(""));
        s += strlen(kw);
        const char *t = s;
        while (*t && !isspace(*t))
            t++;
        char *r = new char[t-s + 1];
        strncpy(r, s, t-s);
        r[t-s] = 0;
        return (r);
    }
}


// Update frequency entries from variable.
//
void
QTfastHenryDlg::update_fh_freq_widgets()
{
    const char *str = CDvdb()->getVariable(VA_FhFreq);
    char *smin = str ? getword("fmin=", str) :
        lstring::copy(fh_def_string(fhFreq));
    char *smax = str ? getword("fmax=", str) :
        lstring::copy(fh_def_string(fhFreq));
    char *sdec = str ? getword("ndec=", str) : lstring::copy("");
    fh_fmin->setText(smin);
    fh_fmax->setText(smax);
    fh_ndec->setText(sdec);
    delete [] smin;
    delete [] smax;
    delete [] sdec;
}


// Update variable from frequency entries.
//
void
QTfastHenryDlg::update_fh_freq()
{
    char buf[128];
    QByteArray qbmin = fh_fmin->text().toLatin1();
    QByteArray qbmax = fh_fmax->text().toLatin1();
    QByteArray qbdec = fh_ndec->text().toLatin1();
    const char *smin = qbmin.constData();
    const char *smax = qbmax.constData();
    const char *sdec = qbdec.constData();

    smin = lstring::gettok(&smin);
    if (!smin)
        smin = lstring::copy("");
    smax = lstring::gettok(&smax);
    if (!smax)
        smax = lstring::copy("");
    sdec = lstring::gettok(&sdec);
    if (sdec) {
        snprintf(buf, sizeof(buf), "fmin=%s fmax=%s ndec=%s",
            smin, smax, sdec);
    }
    else {
        if (!strcmp(smin, fh_def_string(fhFreq)) &&
                !strcmp(smax, fh_def_string(fhFreq))) {
            CDvdb()->clearVariable(VA_FhFreq);
            goto done;
        }
        snprintf(buf, sizeof(buf), "fmin=%s fmax=%s", smin, smax);
    }
    CDvdb()->setVariable(VA_FhFreq, buf);
done:
    delete [] smin;
    delete [] smax;
    delete [] sdec;
}


// Select the chars in the range, start=end deselects existing.  In
// GTK-1, selecting gives blue inverse, which turns gray if
// unselected, retaining an indication for the buttons.  GTK-2
// doesn't do this automatically so we provide something similar here.
//
void
QTfastHenryDlg::select_range(int start, int end)
{
    fh_jobs->select_range(start, end);
    fh_start = start;
    fh_end = end;
}


// Return the PID value of the selected line, or -1.
//
int
QTfastHenryDlg::get_pid()
{
    if (fh_line_selected < 0)
        return (-1);
    char *string = fh_jobs->get_chars();
    int line = 0;
    for (const char *s = string; *s; s++) {
        if (line == fh_line_selected) {
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
QTfastHenryDlg::select_pid(int p)
{
    char *string = fh_jobs->get_chars();
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
                fh_line_selected = line;
                fh_kill->setEnabled(true);
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
QTfastHenryDlg::fh_def_string(int id)
{
    static char tbuf[16];
    switch (id) {
    case fhManhGridCnt:
        snprintf(tbuf, sizeof(tbuf), "%.*f", 0, FH_DEF_MANH_GRID_CNT);
        return (tbuf);
    case fhNhinc:
        snprintf(tbuf, sizeof(tbuf), "%d", DEF_FH_NHINC);
        return (tbuf);
    case fhRh:
        snprintf(tbuf, sizeof(tbuf), "%.*f", 3, DEF_FH_RH);
        return (tbuf);
    case fhVolElMin:
        snprintf(tbuf, sizeof(tbuf), "%.*f", 2, FH_DEF_VOLEL_MIN);
        return (tbuf);
    case fhVolElTarg:
        snprintf(tbuf, sizeof(tbuf), "%.*f", 0, FH_DEF_VOLEL_TARG);
        return (tbuf);
    case fhPath:
        return (fxJob::fh_default_path());
    case fhArgs:
    case fhDefaults:
        return ("");
    case fhFreq:
        return ("1e3");
    }
    return ("");
}


// Static function.
void
QTfastHenryDlg::fh_p_cb(bool ok, void *arg)
{
    char *fname = (char*)arg;
    if (ok)
        DSPmainWbag(PopUpFileBrowser(fname))
    delete [] fname;
}


// Static function.
void
QTfastHenryDlg::fh_dump_cb(const char *fname, void *client_data)
{
    switch ((intptr_t)client_data) {
    case fhDump:
        if (FHif()->fhDump(fname)) {
            if (!QTfastHenryDlg::self())
                return;
            const char *fn = lstring::strip_path(fname);
            if (fn && *fn) {
                char tbuf[256];
                snprintf(tbuf, sizeof(tbuf),
                    "FastHenry input is in file %s.  View file? ", fn);
                QTfastHenryDlg::self()->PopUpAffirm(0, GRloc(LW_UL), tbuf,
                    fh_p_cb, lstring::copy(fname));
            }
        }
        break;
    }
    if (QTfastHenryDlg::self() && QTfastHenryDlg::self()->wb_input)
        QTfastHenryDlg::self()->wb_input->popdown();
}


void
QTfastHenryDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("fhpanel"))
}


void
QTfastHenryDlg::foreg_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_FhForeg, "");
    else
        CDvdb()->clearVariable(VA_FhForeg);
}


void
QTfastHenryDlg::console_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_FhMonitor, "");
    else
        CDvdb()->clearVariable(VA_FhMonitor);
}


void
QTfastHenryDlg::runfile_btn_slot()
{
    QByteArray qba = fh_file->text().toLatin1();
    const char *s = qba.constData();
    char *tok = lstring::getqtok(&s);
    if (tok) {
        FHif()->fhRun(tok, 0, 0, true);
        delete [] tok;
    }
    else
        PopUpErr(MODE_ON, "No file name given!");
}


void
QTfastHenryDlg::runext_btn_slot()
{
    FHif()->fhRun(0, 0, 0);
}


void
QTfastHenryDlg::dumplist_btn_slot()
{
    const char *s = FHif()->getFileName(FH_INP_SFX);
    PopUpInput(0, s, "Dump", fh_dump_cb, (void*)fhDump);
    delete [] s;
}


void
QTfastHenryDlg::args_changed_slot(const QString &qs)
{
    if (fh_no_reset)
        return;
    if (qs == fh_def_string(fhArgs))
        CDvdb()->clearVariable(VA_FhArgs);
    else
        CDvdb()->setVariable(VA_FhArgs, qs.toLatin1().constData());
}


void
QTfastHenryDlg::fmin_changed_slot(const QString&)
{
    if (fh_no_reset)
        return;
    update_fh_freq();
}


void
QTfastHenryDlg::fmax_changed_slot(const QString&)
{
    if (fh_no_reset)
        return;
    update_fh_freq();
}


void
QTfastHenryDlg::ndec_changed_slot(const QString&)
{
    if (fh_no_reset)
        return;
    update_fh_freq();
}


void
QTfastHenryDlg::path_changed_slot(const QString &qs)
{
    if (fh_no_reset)
        return;
    if (qs ==  fh_def_string(fhPath))
        CDvdb()->clearVariable(VA_FhPath);
    else
        CDvdb()->setVariable(VA_FhPath, qs.toLatin1().constData());
}


void
QTfastHenryDlg::units_changed_slot(int i)
{
    const char *str = unit_t::units_strings[i];
    str = FHif()->getUnitsString(str);
    if (str)
        CDvdb()->setVariable(VA_FhUnits, str);
    else
        CDvdb()->clearVariable(VA_FhUnits);
}


void
QTfastHenryDlg::manh_grid_changed_slot(int)
{
    if (fh_no_reset)
        return;
    if (fh_sb_manh_grid_cnt->cleanText() == fh_def_string(fhManhGridCnt))
        CDvdb()->clearVariable(VA_FhManhGridCnt);
    else {
        CDvdb()->setVariable(VA_FhManhGridCnt,
            fh_sb_manh_grid_cnt->cleanText().toLatin1().constData());
    }
}


void
QTfastHenryDlg::defaults_changed_slot(const QString &qs)
{
    if (fh_no_reset)
        return;
    if (qs == fh_def_string(fhDefaults))
        CDvdb()->clearVariable(VA_FhDefaults);
    else
        CDvdb()->setVariable(VA_FhDefaults, qs.toLatin1().constData());
}


void
QTfastHenryDlg::nhinc_changed_slot(int)
{
    if (fh_no_reset)
        return;
    if (fh_sb_nhinc->cleanText() ==  fh_def_string(fhNhinc))
        CDvdb()->clearVariable(VA_FhDefNhinc);
    else {
        CDvdb()->setVariable(VA_FhDefNhinc,
            fh_sb_nhinc->cleanText().toLatin1().constData());
    }
}


void
QTfastHenryDlg::rh_changed_slot(double)
{
    if (fh_no_reset)
        return;
    if (fh_sb_rh->cleanText() == fh_def_string(fhRh))
        CDvdb()->clearVariable(VA_FhDefRh);
    else {
        CDvdb()->setVariable(VA_FhDefRh,
            fh_sb_rh->cleanText().toLatin1().constData());
    }
}


void
QTfastHenryDlg::override_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_FhOverride, "");
    else
        CDvdb()->clearVariable(VA_FhOverride);
}


void
QTfastHenryDlg::internal_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_FhUseFilament, "");
    else
        CDvdb()->clearVariable(VA_FhUseFilament);
}


void
QTfastHenryDlg::enable_btn_slot(int state)
{
    if (state) {
        QString qs = fh_sb_volel_target->cleanText();
        bool ok;
        int i = qs.toInt(&ok);
        if (ok && i >= fh_sb_volel_target->minimum() &&
                i <= fh_sb_volel_target->maximum())
            CDvdb()->setVariable(VA_FhVolElTarget, qs.toLatin1().constData());
        else {
            char tbf[32];
            snprintf(tbf, sizeof(tbf), "%.1e", FH_DEF_VOLEL_TARG);
            CDvdb()->setVariable(VA_FhVolElTarget, tbf);
        }
        fh_sb_volel_target->setEnabled(true);

        qs = fh_sb_volel_min->cleanText();
        int z = 0;
        if (fh_sb_volel_min->validate(qs, z) == QValidator::Acceptable)
            CDvdb()->setVariable(VA_FhVolElMin, qs.toLatin1().constData());
        else {
            char tbf[32];
            snprintf(tbf,sizeof(tbf), "%.1e", FH_DEF_VOLEL_MIN);
            CDvdb()->setVariable(VA_FhVolElMin, tbf);
        }
        fh_sb_volel_min->setEnabled(true);
        CDvdb()->setVariable(VA_FhVolElEnable, "");
    }
    else {
        CDvdb()->clearVariable(VA_FhVolElTarget);
        CDvdb()->clearVariable(VA_FhVolElMin);
        CDvdb()->clearVariable(VA_FhVolElEnable);
        fh_sb_volel_target->setEnabled(false);
        fh_sb_volel_min->setEnabled(false);
    }
}


void
QTfastHenryDlg::volel_min_changed_slot(double)
{
    if (fh_no_reset)
        return;
    if (fh_sb_volel_min->cleanText() == fh_def_string(fhVolElMin))
        CDvdb()->clearVariable(VA_FhVolElMin);
    else  {
        CDvdb()->setVariable(VA_FhVolElMin,
            fh_sb_volel_min->cleanText().toLatin1().constData());
    }
}


void
QTfastHenryDlg::volel_target_changed_slot(int)
{
    if (fh_no_reset)
        return;
    if (fh_sb_volel_target->cleanText() == fh_def_string(fhVolElTarg))
        CDvdb()->clearVariable(VA_FhVolElTarget);
    else {
        CDvdb()->setVariable(VA_FhVolElTarget,
            fh_sb_volel_target->cleanText().toLatin1().constData());
    }
}


void
QTfastHenryDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    int vsv = fh_jobs->verticalScrollBar()->value();
    int hsv = fh_jobs->horizontalScrollBar()->value();

    const char *str = fh_jobs->get_chars();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    int posn = fh_jobs->document()->documentLayout()->hitTest(
        QPointF(xx + hsv, yy + vsv), Qt::ExactHit);
    
    if (isspace(str[posn])) {
        // Clicked on white space.
        delete [] str;
        return;
    }

    const char *lineptr = str;
    int line = 0;
    for (int i = 0; i <= posn; i++) {
        if (str[i] == '\n') {
            if (i == posn) {
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

        fh_line_selected = line;
        select_range(start, end);
        delete [] str;
        fh_kill->setEnabled(get_pid() > 0);
    }

    fh_line_selected = -1;
    delete [] str;
    select_range(0, 0);
    fh_kill->setEnabled(false);
}


void
QTfastHenryDlg::abort_btn_slot()
{
    int pid = get_pid();
    if (pid > 0)
        FHif()->killProcess(pid);
}


void
QTfastHenryDlg::dismiss_btn_slot()
{
    FHif()->PopUpExtIf(0, MODE_OFF);
}


void
QTfastHenryDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_FIXED))
            fh_jobs->setFont(*fnt);
        update_jobs_list();
    }
}

