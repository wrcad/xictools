
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

#include "qtcvout.h"
#include "cvrt.h"
#include "fio.h"
#include "dsp_inlines.h"
#include "qtcnmap.h"
#include "qtwndc.h"
#include "qtcvofmt.h"
#include "qtinterf/qtdblsb.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTabWidget>
#include <QPushButton>
#include <QCheckBox>


//-----------------------------------------------------------------------------
// QTconvertOutDlg:  Dialog to write cell data files.
// Called from main menu: Convert/Export Cell Data.
//
// Help system keywords used:
//  xic:exprt

void
cConvert::PopUpExport(GRobject caller, ShowMode mode,
    bool (*callback)(FileType, bool, void*), void *arg)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTconvertOutDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTconvertOutDlg::self())
            QTconvertOutDlg::self()->update();
        return;
    }
    if (QTconvertOutDlg::self())
        return;

    new QTconvertOutDlg(caller, callback, arg);

    QTconvertOutDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_UR), QTconvertOutDlg::self(),
        QTmainwin::self()->Viewport());
    QTconvertOutDlg::self()->show();
}
// End of cConvert functions.


namespace {
    WndSensMode wnd_sens_test()     { return (WndSensFlatten); }
}


QTconvertOutDlg::fmtval_t QTconvertOutDlg::cvo_fmtvals[] =
{
    fmtval_t("GDSII", Fgds),
    fmtval_t("OASIS", Foas),
    fmtval_t("CIF", Fcif),
    fmtval_t("CGX", Fcgx),
    fmtval_t("Xic Cell Files", Fnative),
    fmtval_t(0, Fnone)
};
int QTconvertOutDlg::cvo_fmt_type = cConvert::cvGds;
QTconvertOutDlg *QTconvertOutDlg::instPtr;

QTconvertOutDlg::QTconvertOutDlg(GRobject c, CvoCallback callback, void *arg)
{
    instPtr = this;
    cvo_caller = c;
    cvo_label = 0;
    cvo_nbook = 0;
    cvo_strip = 0;
    cvo_libsub = 0;
    cvo_pcsub = 0;
    cvo_viasub = 0;
    cvo_allcells = 0;
    cvo_noflvias = 0;
    cvo_noflpcs = 0;
    cvo_nofllbs = 0;
    cvo_keepbad = 0;
    cvo_invis_p = 0;
    cvo_invis_e = 0;
    cvo_sb_scale = 0;
    cvo_fmt = 0;
    cvo_wnd = 0;
    cvo_cnmap = 0;
    cvo_callback = callback;
    cvo_arg = arg;
    cvo_useallcells = false;

    // Dangerous to leave this in effect, force user to turn in on
    // when needed.
    FIO()->SetOutFlatten(false);

    setWindowTitle(tr("Export Control"));
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
    cvo_label = new QLabel("");
    cvo_label->setAlignment(Qt::AlignCenter);
    hb->addWidget(cvo_label);

    hbox->addStretch(1);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // Format selection notebook.
    //
    cvo_fmt = new QTconvOutFmt(cvo_format_proc, cvo_fmt_type,
        QTconvOutFmt::cvofmt_asm);
    vbox->addWidget(cvo_fmt);

    // Mode notebook.
    cvo_nbook = new QTabWidget();
    vbox->addWidget(cvo_nbook);
    connect(cvo_nbook, SIGNAL(currentChanged(int)),
        this, SLOT(nbook_page_slot(int)));

    // The Setup Page
    //
    QWidget *page = new QWidget();
    cvo_nbook->addTab(page, tr("Setup"));

    QVBoxLayout *pvbox = new QVBoxLayout(page);
    pvbox->setContentsMargins(qmtop);
    pvbox->setSpacing(2);

    QHBoxLayout *phbox = new QHBoxLayout(0);
    phbox->setContentsMargins(qm);
    phbox->setSpacing(2);
    pvbox->addLayout(phbox);

    // Invisible layer conversion
    //
    QLabel *label = new QLabel(tr("Don't convert invisible layers:"));
    phbox->addWidget(label);

    phbox->addStretch(1);

    cvo_invis_p = new QCheckBox(tr("Physical"));
    phbox->addWidget(cvo_invis_p);
    connect(cvo_invis_p, SIGNAL(stateChanged(int)),
        this, SLOT(invis_p_btn_slot(int)));

    phbox->addSpacing(24);

    cvo_invis_e = new QCheckBox(tr("Electrical"));
    phbox->addWidget(cvo_invis_e);
    connect(cvo_invis_e, SIGNAL(stateChanged(int)),
        this, SLOT(invis_e_btn_slot(int)));

    const char *s = CDvdb()->getVariable(VA_SkipInvisible);
    if (!s) {
        QTdev::SetStatus(cvo_invis_p, false);
        QTdev::SetStatus(cvo_invis_e, false);
    }
    else {
        if (*s != 'e' && *s != 'E')
            QTdev::SetStatus(cvo_invis_p, true);
        else
            QTdev::SetStatus(cvo_invis_p, false);
        if (*s != 'p' && *s != 'P')
            QTdev::SetStatus(cvo_invis_e, true);
        else
            QTdev::SetStatus(cvo_invis_e, false);
    }

    // Check boxes.
    //
    cvo_strip = new QCheckBox(tr(
        "Strip For Export - (convert physical data only)"));
    pvbox->addWidget(cvo_strip);
    connect(cvo_strip, SIGNAL(stateChanged(int)),
        this, SLOT(strip_btn_slot(int)));

    cvo_libsub = new QCheckBox(tr("Include library cell masters"));
    pvbox->addWidget(cvo_libsub);
    connect(cvo_libsub, SIGNAL(stateChanged(int)),
        this, SLOT(libsub_btn_slot(int)));

    cvo_pcsub = new QCheckBox(tr(
        "Include parameterized cell sub-masters"));
    pvbox->addWidget(cvo_pcsub);
    connect(cvo_pcsub, SIGNAL(stateChanged(int)),
        this, SLOT(pcsub_btn_slot(int)));

    cvo_viasub = new QCheckBox(tr(
        "Include standard via cell sub-masters"));
    pvbox->addWidget(cvo_viasub);
    connect(cvo_viasub, SIGNAL(stateChanged(int)),
        this, SLOT(viasub_btn_slot(int)));

    cvo_allcells = new QCheckBox(tr(
        "Consider ALL cells in current symbol table for output"));
    pvbox->addWidget(cvo_allcells);
    connect(cvo_allcells, SIGNAL(stateChanged(int)),
        this, SLOT(allcells_btn_slot(int)));

    cvo_noflvias = new QCheckBox(tr(
        "Don't flatten standard vias, keep as instance at top level"));
    pvbox->addWidget(cvo_noflvias);
    connect(cvo_noflvias, SIGNAL(stateChanged(int)),
        this, SLOT(noflvias_btn_slot(int)));

    cvo_noflpcs = new QCheckBox(tr(
        "Don't flatten pcells, keep as instance at top level"));
    pvbox->addWidget(cvo_noflpcs);
    connect(cvo_noflpcs, SIGNAL(stateChanged(int)),
        this, SLOT(noflpcs_btn_slot(int)));

    cvo_nofllbs = new QCheckBox(tr(
        "Ignore labels in subcells when flattening"));
    pvbox->addWidget(cvo_nofllbs);
    connect(cvo_nofllbs, SIGNAL(stateChanged(int)),
        this, SLOT(nofllbs_btn_slot(int)));

    cvo_keepbad = new QCheckBox(tr("Keep bad output (for debugging)"));
    pvbox->addWidget(cvo_keepbad);
    connect(cvo_keepbad, SIGNAL(stateChanged(int)),
        this, SLOT(keepbad_btn_slot(int)));

    // The Read File page
    //
    page = new QWidget();
    cvo_nbook->addTab(page, tr("Write File"));

    pvbox = new QVBoxLayout(page);
    pvbox->setContentsMargins(qmtop);
    pvbox->setSpacing(2);

    // Cell name mapping
    //
    cvo_cnmap = new QTcnameMap(true);
    pvbox->addWidget(cvo_cnmap);

    // Window
    //
    cvo_wnd = new QTwindowCfg(wnd_sens_test, WndFuncOut);
    pvbox->addWidget(cvo_wnd);

    // Scale spin button and label
    //
    phbox = new QHBoxLayout();
    pvbox->addLayout(phbox);
    phbox->setContentsMargins(qm);
    phbox->setSpacing(2);

    label = new QLabel(tr("Writing Scale Factor"));
    phbox->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    cvo_sb_scale = new QTdoubleSpinBox();;
    phbox->addWidget(cvo_sb_scale);
    cvo_sb_scale->setMinimum(CDSCALEMIN);
    cvo_sb_scale->setMaximum(CDSCALEMAX);
    cvo_sb_scale->setDecimals(5);
    cvo_sb_scale->setValue(FIO()->WriteScale());
    connect(cvo_sb_scale, SIGNAL(valueChanged(double)),
        this, SLOT(scale_changed_slot(double)));

    // Write File button
    //
    btn = new QPushButton(tr("Write File"));
    pvbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(write_btn_slot()));
    btn->setMaximumWidth(140);

    // Dismiss button
    //
    btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTconvertOutDlg::~QTconvertOutDlg()
{
    instPtr = 0;
    if (cvo_caller)
        QTdev::Deselect(cvo_caller);
    if (cvo_callback)
        (*cvo_callback)(Fnone, false, cvo_arg);
}


void
QTconvertOutDlg::update()
{
    cvo_fmt->update();
    QTdev::SetStatus(cvo_strip,
        CDvdb()->getVariable(VA_StripForExport));
    QTdev::SetStatus(cvo_libsub,
        CDvdb()->getVariable(VA_KeepLibMasters));
    QTdev::SetStatus(cvo_pcsub,
        CDvdb()->getVariable(VA_PCellKeepSubMasters));
    QTdev::SetStatus(cvo_viasub,
        CDvdb()->getVariable(VA_ViaKeepSubMasters));
    QTdev::SetStatus(cvo_noflvias,
        CDvdb()->getVariable(VA_NoFlattenStdVias));
    QTdev::SetStatus(cvo_noflpcs,
        CDvdb()->getVariable(VA_NoFlattenPCells));
    QTdev::SetStatus(cvo_nofllbs,
        CDvdb()->getVariable(VA_NoFlattenLabels));
    QTdev::SetStatus(cvo_keepbad,
        CDvdb()->getVariable(VA_KeepBadArchive));

    const char *s = CDvdb()->getVariable(VA_SkipInvisible);
    if (!s) {
        QTdev::SetStatus(cvo_invis_p, false);
        QTdev::SetStatus(cvo_invis_e, false);
    }
    else {
        if (*s != 'e' && *s != 'E')
            QTdev::SetStatus(cvo_invis_p, true);
        else
            QTdev::SetStatus(cvo_invis_p, false);
        if (*s != 'p' && *s != 'P')
            QTdev::SetStatus(cvo_invis_e, true);
        else
            QTdev::SetStatus(cvo_invis_e, false);
    }

    cvo_sb_scale->setValue(FIO()->WriteScale());
    cvo_cnmap->update();
    cvo_wnd->update();
    cvo_wnd->set_sens();
}


// Static function.
void
QTconvertOutDlg::cvo_format_proc(int type)
{
    cvo_fmt_type = type;
    instPtr->cvo_wnd->set_sens();
}


void
QTconvertOutDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:exprt"))
}


void
QTconvertOutDlg::nbook_page_slot(int pg)
{
    const char *lb;
    if (pg == 0)
        lb = "Set parameters for writing cell data";
    else
        lb = "Write cell data file";
    cvo_label->setText(tr(lb));
}


void
QTconvertOutDlg::invis_p_btn_slot(int state)
{
    bool ske = QTdev::GetStatus(cvo_invis_e);
    if (state) {
        if (ske)
            CDvdb()->setVariable(VA_SkipInvisible, 0);
        else
            CDvdb()->setVariable(VA_SkipInvisible, "p");
    }
    else {
        if (ske)
            CDvdb()->setVariable(VA_SkipInvisible, "e");
        else
            CDvdb()->clearVariable(VA_SkipInvisible);
    }
}


void
QTconvertOutDlg::invis_e_btn_slot(int state)
{
    bool skp = QTdev::GetStatus(cvo_invis_p);
    if (state) {
        if (skp)
            CDvdb()->setVariable(VA_SkipInvisible, 0);
        else
            CDvdb()->setVariable(VA_SkipInvisible, "e");
    }
    else {
        if (skp)
            CDvdb()->setVariable(VA_SkipInvisible, "p");
        else
            CDvdb()->clearVariable(VA_SkipInvisible);
    }
}


void
QTconvertOutDlg::strip_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_StripForExport, 0);
    else
        CDvdb()->clearVariable(VA_StripForExport);
}


void
QTconvertOutDlg::libsub_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_KeepLibMasters, 0);
    else
        CDvdb()->clearVariable(VA_KeepLibMasters);
}


void
QTconvertOutDlg::pcsub_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_PCellKeepSubMasters, "");
    else
        CDvdb()->clearVariable(VA_PCellKeepSubMasters);
}


void
QTconvertOutDlg::viasub_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_ViaKeepSubMasters, "");
    else
        CDvdb()->clearVariable(VA_ViaKeepSubMasters);
}


void
QTconvertOutDlg::allcells_btn_slot(int state)
{
    cvo_useallcells = state;
}


void
QTconvertOutDlg::noflvias_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoFlattenStdVias, 0);
    else
        CDvdb()->clearVariable(VA_NoFlattenStdVias);
}


void
QTconvertOutDlg::noflpcs_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoFlattenPCells, 0);
    else
        CDvdb()->clearVariable(VA_NoFlattenPCells);
}


void
QTconvertOutDlg::nofllbs_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoFlattenLabels, 0);
    else
        CDvdb()->clearVariable(VA_NoFlattenLabels);
}


void
QTconvertOutDlg::keepbad_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_KeepBadArchive, 0);
    else
        CDvdb()->clearVariable(VA_KeepBadArchive);
}


void
QTconvertOutDlg::scale_changed_slot(double d)
{
    if (d >= CDSCALEMIN && d <= CDSCALEMAX)
        FIO()->SetWriteScale(d);
}


void
QTconvertOutDlg::write_btn_slot()
{
    if (!cvo_callback ||
            !(*cvo_callback)(cvo_fmtvals[cvo_fmt_type].filetype,
                cvo_useallcells, cvo_arg))
        Cvt()->PopUpExport(0, MODE_OFF, 0, 0);
}


void
QTconvertOutDlg::dismiss_btn_slot()
{
    Cvt()->PopUpExport(0, MODE_OFF, 0, 0);
}

