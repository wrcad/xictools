
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

#include "qtextsel.h"
#include "main.h"
#include "ext.h"
#include "sced.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "ext_menu.h"
#include "ext_pathfinder.h"
#include "promptline.h"

#include <QApplication>
#include <QLayout>
#include <QToolButton>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>


//-----------------------------------------------------------------------------
// QTextNetSelDlg:  Dialog to control group/node/path selections.
// Called from main menu: Extract/Net Selections.
//
// Help system keywords used:
//  xic:exsel

void
cExt::PopUpSelections(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTextNetSelDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTextNetSelDlg::self())
            QTextNetSelDlg::self()->update();
        return;
    }
    if (QTextNetSelDlg::self())
        return;

    new QTextNetSelDlg(caller);

    QTextNetSelDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_UL), QTextNetSelDlg::self(),
        QTmainwin::self()->Viewport());
    QTextNetSelDlg::self()->show();
}
// End of cExt functions.


QTextNetSelDlg *QTextNetSelDlg::instPtr;

QTextNetSelDlg::QTextNetSelDlg(GRobject caller) : QTbag(this)
{
    instPtr = this;
    es_caller = caller;
    es_gnsel = 0;
    es_paths = 0;
    es_qpath = 0;
    es_gpmnu = 0;
    es_sb_depth = 0;
    es_qpconn = 0;
    es_blink = 0;
    es_subpath = 0;
    es_antenna = 0;
    es_zoid = 0;
    es_tofile = 0;
    es_vias = 0;
    es_vtree = 0;
    es_rlab = 0;
    es_terms = 0;
    es_meas = 0;

    setWindowTitle(tr("Path Selection Control"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);
    vbox->setSizeConstraint(QLayout::SetFixedSize);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // top button row
    //
    es_gnsel = new QToolButton();
    es_gnsel->setText(tr("Select Group/Node"));
    hbox->addWidget(es_gnsel);
    es_gnsel->setCheckable(true);
    connect(es_gnsel, &QAbstractButton::toggled,
        this, &QTextNetSelDlg::gnsel_btn_slot);

    es_paths = new QToolButton();
    es_paths->setText(tr("Select Path"));
    hbox->addWidget(es_paths);
    es_paths->setCheckable(true);
    connect(es_paths, &QAbstractButton::toggled,
        this, &QTextNetSelDlg::pathsel_btn_slot);

    es_qpath = new QToolButton();
    es_qpath->setText(tr("\"Quick\" Path"));
    hbox->addWidget(es_qpath);
    es_qpath->setCheckable(true);
    connect(es_qpath, &QAbstractButton::toggled,
        this, &QTextNetSelDlg::qpath_btn_slot);

    hbox->addStretch(1);
    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTextNetSelDlg::help_btn_slot);

    // quick paths ground plane method
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QLabel *label = new QLabel(tr("\"Quick\" Path ground plane handling"));
    hbox->addWidget(label);

    es_gpmnu = new QComboBox();
    hbox->addWidget(es_gpmnu);
    es_gpmnu->addItem(tr("Use ground plane if available"));
    es_gpmnu->addItem(tr("Create and use ground plane"));
    es_gpmnu->addItem(tr("Never use ground plane"));
    connect(es_gpmnu, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &QTextNetSelDlg::gplane_menu_slot);

    // depth control line
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    label = new QLabel(tr("Path search depth"));
    hbox->addWidget(label);

    es_sb_depth = new QSpinBox();
    hbox->addWidget(es_sb_depth);
    es_sb_depth->setRange(0, CDMAXCALLDEPTH);
    es_sb_depth->setValue(CDMAXCALLDEPTH);
    connect(es_sb_depth, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &QTextNetSelDlg::depth_changed_slot);

    tbtn = new QToolButton();
    tbtn->setText("0");
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTextNetSelDlg::zbtn_slot);

    tbtn = new QToolButton();
    tbtn->setText(tr("all"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTextNetSelDlg::allbtn_slot);

    hbox->addSpacing(4);
    es_qpconn = new QCheckBox(tr("\"Quick\" Path use Conductor"));
    hbox->addWidget(es_qpconn);
    connect(es_qpconn, &QCheckBox::toggled,
        this, &QTextNetSelDlg::qp_usec_btn_slot);

    // check boxes
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    es_blink = new QCheckBox(tr("Blink highlighting"));
    hbox->addWidget(es_blink);
    connect(es_blink, &QCheckBox::toggled,
        this, &QTextNetSelDlg::blink_btn_slot);

    es_subpath = new QCheckBox(tr("Enable sub-path selection"));
    hbox->addWidget(es_subpath);
    connect(es_subpath, &QCheckBox::toggled,
        this, &QTextNetSelDlg::subpath_btn_slot);

    // botton button row
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    es_antenna = new QToolButton();
    es_antenna->setText(tr("Load Antenna file"));
    hbox->addWidget(es_antenna);
    connect(es_antenna, &QAbstractButton::clicked,
        this, &QTextNetSelDlg::ldant_btn_slot);

    es_zoid = new QToolButton();
    es_zoid->setText(tr("To trapezoids"));
    hbox->addWidget(es_zoid);
    connect(es_zoid, &QAbstractButton::clicked,
        this, &QTextNetSelDlg::zoid_btn_slot);

    es_tofile = new QToolButton();
    es_tofile->setText(tr("Save path to file"));
    hbox->addWidget(es_tofile);
    connect(es_tofile, &QAbstractButton::clicked,
        this, &QTextNetSelDlg::tofile_btn_slot);

    // Path file vias check boxes.
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

#if QT_VERSION >= QT_VERSION_CHECK(6,8,0)
#define CHECK_BOX_STATE_CHANGED &QCheckBox::checkStateChanged
#else
#define CHECK_BOX_STATE_CHANGED &QCheckBox::stateChanged
#endif

    es_vias = new QCheckBox(tr("Path file contains vias"));
    hbox->addWidget(es_vias);
    connect(es_vias, CHECK_BOX_STATE_CHANGED,
        this, &QTextNetSelDlg::pathvias_btn_slot);

    es_vtree = new QCheckBox(tr("Path file contains via check layers"));
    hbox->addWidget(es_vtree);
    connect(es_vtree, CHECK_BOX_STATE_CHANGED,
        this, &QTextNetSelDlg::vcheck_btn_slot);

    // resistance measurememt
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    es_rlab = new QLabel(tr("Resistance measurement"));
    hbox->addWidget(es_rlab);

    es_terms = new QToolButton();
    es_terms->setText(tr("Define terminals"));
    hbox->addWidget(es_terms);
    es_terms->setCheckable(true);
    connect(es_terms, &QAbstractButton::toggled,
        this, &QTextNetSelDlg::def_terms_slot);

    es_meas = new QToolButton();
    es_meas->setText(tr("Measure"));
    hbox->addWidget(es_meas);
    connect(es_meas, &QAbstractButton::clicked,
        this, &QTextNetSelDlg::meas_btn_slot);

    // dismiss button
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    vbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QTextNetSelDlg::dismiss_btn_slot);

    update();
}


QTextNetSelDlg::~QTextNetSelDlg()
{
    if (QTdev::GetStatus(es_gnsel))
        QTdev::CallCallback(es_gnsel);
    else if (QTdev::GetStatus(es_paths))
        QTdev::CallCallback(es_paths);
    else if (QTdev::GetStatus(es_qpath))
        QTdev::CallCallback(es_qpath);

    instPtr = 0;
    if (es_caller)
        QTdev::Deselect(es_caller);
    // Needed to unset the Click-Select Mode button.
    SCD()->PopUpNodeMap(0, MODE_UPD);
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTextNetSelDlg
#include "qtinterf/qtmacos_event.h"
#endif


void
QTextNetSelDlg::update()
{
    es_gpmnu->setCurrentIndex(EX()->quickPathMode());
    QTdev::SetStatus(es_qpconn, EX()->isQuickPathUseConductor());
    QTdev::SetStatus(es_blink, EX()->isBlinkSelections());
    if (QTdev::GetStatus(es_gnsel))
        QTdev::SetStatus(es_paths, EX()->isGNShowPath());
    else
        QTdev::SetStatus(es_subpath, EX()->isSubpathEnabled());
    es_sb_depth->setValue(EX()->pathDepth());
    set_sens();

    const char *vstr = CDvdb()->getVariable(VA_PathFileVias);
    if (!vstr) {
        QTdev::SetStatus(es_vias, false);
        QTdev::SetStatus(es_vtree, false);
        es_vtree->setEnabled(false);
    }
    else if (!*vstr) {
        QTdev::SetStatus(es_vias, true);
        QTdev::SetStatus(es_vtree, false);
        es_vtree->setEnabled(true);
    }
    else {
        QTdev::SetStatus(es_vias, true);
        QTdev::SetStatus(es_vtree, true);
        es_vtree->setEnabled(true);
    }
}


void
QTextNetSelDlg::set_sens()
{
    pathfinder *pf = EX()->pathFinder(cExt::PFget);
    bool has_path = (pf && !pf->is_empty());

    if (has_path && (QTdev::GetStatus(es_gnsel) ||
            QTdev::GetStatus(es_paths) ||
            QTdev::GetStatus(es_qpath))) {
        es_zoid->setEnabled(true);
        es_tofile->setEnabled(true);
        es_rlab->setEnabled(true);
        es_terms->setEnabled(true);

        Blist *t = EX()->terminals();
        if (t && t->next)
            es_meas->setEnabled(true);
        else
            es_meas->setEnabled(false);
    }
    else {
        es_zoid->setEnabled(false);
        es_tofile->setEnabled(false);
        es_rlab->setEnabled(false);
        es_terms->setEnabled(false);
        es_meas->setEnabled(false);
    }
    if (!QTdev::GetStatus(es_gnsel) &&
            QTdev::GetStatus(es_paths))
        es_antenna->setEnabled(true);
    else
        es_antenna->setEnabled(false);

    if (DSP()->CurMode() == Electrical) {
        es_qpath->setEnabled(false);
        if (!QTdev::GetStatus(es_gnsel))
            es_paths->setEnabled(false);

    }
    else {
        es_qpath->setEnabled(true);
        es_paths->setEnabled(true);
    }
}


// Static function.
int
QTextNetSelDlg::es_redraw_idle(void *)
{
    if (instPtr) {
        if (QTdev::GetStatus(instPtr->es_gnsel))
            EX()->selectRedrawPath();
        else if (QTdev::GetStatus(instPtr->es_paths) ||
                QTdev::GetStatus(instPtr->es_qpath))
            EX()->redrawPath();
    }
    return (0);
}


void
QTextNetSelDlg::gnsel_btn_slot(bool state)
{
    EX()->selectGroupNode(es_gnsel);
    if (state) {
        QTdev::Deselect(es_subpath);
        es_subpath->setEnabled(false);
        es_paths->setEnabled(true);
    }
    else {
        QTdev::SetStatus(es_subpath, EX()->isSubpathEnabled());
        es_subpath->setEnabled(true);
        QTdev::Deselect(es_paths);
        if (DSP()->CurMode() == Electrical)
            es_paths->setEnabled(false);
    }
    set_sens();
}


void
QTextNetSelDlg::pathsel_btn_slot(bool state)
{
    if (QTdev::GetStatus(es_gnsel))
        EX()->selectShowPath(state);
    else
        EX()->selectPath(es_paths);
    set_sens();
}


void
QTextNetSelDlg::qpath_btn_slot(bool)
{
    EX()->selectPathQuick(es_qpath);
    set_sens();
}


void
QTextNetSelDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:exsel"))
}


void
QTextNetSelDlg::gplane_menu_slot(int code)
{
    if (code == 0)
        CDvdb()->clearVariable(VA_QpathGroundPlane);
    else if (code == 1)
        CDvdb()->setVariable(VA_QpathGroundPlane, "1");
    else if (code == 2)
        CDvdb()->setVariable(VA_QpathGroundPlane, "2");
}


void
QTextNetSelDlg::depth_changed_slot(int d)
{
    EX()->setPathDepth(d);

    // Redraw the path in an idle proc, otherwise spin button
    // behaves strangely.
    QTpkg::self()->RegisterIdleProc(es_redraw_idle, 0);
}


void
QTextNetSelDlg::zbtn_slot()
{
    es_sb_depth->setValue(0);
}


void
QTextNetSelDlg::allbtn_slot()
{
    es_sb_depth->setValue(CDMAXCALLDEPTH);
}


void
QTextNetSelDlg::qp_usec_btn_slot(bool state)
{
    if (state)
        CDvdb()->setVariable(VA_QpathUseConductor, "");
    else
        CDvdb()->clearVariable(VA_QpathUseConductor);
}


void
QTextNetSelDlg::blink_btn_slot(bool state)
{
    EX()->setBlinkSelections(state);
    pathfinder *pf = EX()->pathFinder(cExt::PFget);
    if (pf) {
        pf->show_path(0, ERASE);
        pf->show_path(0, DISPLAY);
    }
}


void
QTextNetSelDlg::subpath_btn_slot(bool state)
{
    EX()->setSubpathEnabled(state);
}


void
QTextNetSelDlg::ldant_btn_slot()
{
    EX()->getAntennaPath();
}


void
QTextNetSelDlg::zoid_btn_slot()
{
    pathfinder *pf = EX()->pathFinder(cExt::PFget);
    if (pf) {
        pf->show_path(0, ERASE);
        pf->atomize_path();
        pf->show_path(0, DISPLAY);
    }
    es_zoid->setEnabled(false);
}


namespace {
    void tof_cb(const char *name, void*)
    {
        if (name && *name) {
            // Maybe write out the vias, too.
            const char *vstr = CDvdb()->getVariable(VA_PathFileVias);
            if (EX()->saveCurrentPathToFile(name, (vstr != 0))) {
                const char *msg = "Path saved to native cell file.";
                if (QTextNetSelDlg::self())
                    QTextNetSelDlg::self()->PopUpMessage(msg, false);
                else
                    PL()->ShowPrompt(msg);
            }
            else {
                const char *msg = "Operation failed.";
                if (QTextNetSelDlg::self())
                    QTextNetSelDlg::self()->PopUpMessage(msg, false);
                else
                    PL()->ShowPrompt(msg);
            }
            if (QTextNetSelDlg::self() &&
                    QTextNetSelDlg::self()->ActiveInput())
                QTextNetSelDlg::self()->ActiveInput()->popdown();
        }
    }
}


void
QTextNetSelDlg::tofile_btn_slot()
{
    pathfinder *pf = EX()->pathFinder(cExt::PFget);
    if (pf) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s_grp_%s",
            Tstring(DSP()->CurCellName()), pf->pathname());
        PopUpInput("Enter native cell name", buf, "Save Cell",
            tof_cb, 0);
    }
    else
        PL()->ShowPrompt("No current path!");
}


void
QTextNetSelDlg::pathvias_btn_slot(int state)
{
    if (state) {
        CDvdb()->setVariable(VA_PathFileVias, "");
        es_vtree->setEnabled(true);
    }
    else {
        CDvdb()->clearVariable(VA_PathFileVias);
        QTdev::SetStatus(es_vtree, false);
        es_vtree->setEnabled(false);
    }
}


void
QTextNetSelDlg::vcheck_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_PathFileVias, "check");
    else
        CDvdb()->setVariable(VA_PathFileVias, "");
}


void
QTextNetSelDlg::def_terms_slot(bool)
{
    EX()->editTerminals(es_terms);
}


void
QTextNetSelDlg::meas_btn_slot()
{
    double *vals;
    int sz;
    if (EX()->extractNetResistance(&vals, &sz, 0))
        PL()->ShowPromptV("Measured resistance: %g", vals[0]);
    else
        PL()->ShowPromptV("Failed: %s", Errs()->get_error());
}


void
QTextNetSelDlg::dismiss_btn_slot()
{
    EX()->PopUpSelections(0, MODE_OFF);
}


