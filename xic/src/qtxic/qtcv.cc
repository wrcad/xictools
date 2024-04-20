
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

#include "qtcv.h"
#include "fio.h"
#include "cvrt.h"
#include "dsp_inlines.h"
#include "qtllist.h"
#include "qtcvofmt.h"
#include "qtcnmap.h"
#include "qtwndc.h"
#include "qtinterf/qtdblsb.h"

#include <QApplication>
#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTabWidget>
#include <QCheckBox>
#include <QToolButton>
#include <QPushButton>
#include <QComboBox>


//-----------------------------------------------------------------------------
// QTconvertFmfDlg:  Dialog to control stand-alone layout file format
// conversions.
// Called from main menu: Convert/Format Conversion.
//
// Help system keywords used:
//  xic:convt

// The inp_type has two fields:
// Low short:  set input source
//    enum { cvDefault, cvLayoutFile, cvChdName, cvChdFile, cvNativeDir };
// High short: set page, if 0 no change, else subtract 1 for
//    enum { cvGds, cvOas, cvCif, cvCgx, cvXic, cvTxt, cvChd, cvCgd };

void
cConvert::PopUpConvert(GRobject caller, ShowMode mode, int inp_type,
    bool(*callback)(int, void*), void *arg)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTconvertFmtDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTconvertFmtDlg::self())
            QTconvertFmtDlg::self()->update(inp_type);
        return;
    }
    if (QTconvertFmtDlg::self())
        return;

    new QTconvertFmtDlg(caller, inp_type, callback, arg);

    QTconvertFmtDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTconvertFmtDlg::self(),
        QTmainwin::self()->Viewport());
    QTconvertFmtDlg::self()->show();
}
// End of cConvert functions.


int QTconvertFmtDlg::cv_fmt_type = cConvert::cvGds;
int QTconvertFmtDlg::cv_inp_type = cConvert::cvLayoutFile;
QTconvertFmtDlg *QTconvertFmtDlg::instPtr;

QTconvertFmtDlg::QTconvertFmtDlg(GRobject c, int inp_type,
    bool(*callback)(int, void*), void *arg)
{
    instPtr = this;
    cv_caller = c;
    cv_label = 0;
    cv_input = 0;
    cv_fmt = 0;
    cv_nbook = 0;

    cv_strip = 0;
    cv_libsub = 0;
    cv_pcsub = 0;
    cv_viasub = 0;
    cv_noflvias = 0;
    cv_noflpcs = 0;
    cv_nofllbs = 0;
    cv_nolabels = 0;
    cv_keepbad = 0;

    cv_llist = 0;
    cv_cnmap = 0;
    cv_wnd = 0;
    cv_tx_label = 0;
    cv_callback = callback;
    cv_arg = arg;

    setWindowTitle(tr("File Format Conversion"));
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
    cv_label = new QLabel("");
    hb->addWidget(cv_label);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // Input selection menu
    //
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QLabel *label = new QLabel(tr("Input Source"));
    hbox->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    cv_input = new QComboBox();
    hbox->addWidget(cv_input);
    cv_input->addItem(tr("Layout File"), 0);
    cv_input->addItem(tr("Cell Hierarchy Digest Name"), 1);
    cv_input->addItem(tr("Cell Hierarchy Digest File"), 2);
    cv_input->addItem(tr("Native Cell Directory"), 3);
    connect(cv_input, SIGNAL(currentIndexChanged(int)),
        this, SLOT(input_menu_slot(int)));

    // Output format selection notebook.
    //
    cv_fmt = new QTconvOutFmt(cv_format_proc, cv_fmt_type,
        QTconvOutFmt::cvofmt_file);
    vbox->addWidget(cv_fmt);

    // Lower half tab widget.
    //
    cv_nbook = new QTabWidget();
    vbox->addWidget(cv_nbook);
    connect(cv_nbook, SIGNAL(currentChanged(int)),
        this, SLOT(nbook_page_slot(int)));

    // The Setup page
    //
    QWidget *page = new QWidget();
    cv_nbook->addTab(page, tr("Setup"));

    QVBoxLayout *vb = new QVBoxLayout(page);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    // Strip for Export button
    //
    cv_strip = new QCheckBox(tr(
        "Strip For Export - (convert physical data only)"));
    vb->addWidget(cv_strip);
    connect(cv_strip, SIGNAL(stateChanged(int)),
        this, SLOT(strip_btn_slot(int)));
    QTdev::SetStatus(cv_strip, CDvdb()->getVariable(VA_StripForExport));

    cv_libsub = new QCheckBox(tr("Include library cell masters"));
    vb->addWidget(cv_libsub);
    connect(cv_libsub, SIGNAL(stateChanged(int)),
        this, SLOT(libsub_btn_slot(int)));
    QTdev::SetStatus(cv_libsub, CDvdb()->getVariable(VA_KeepLibMasters));

    cv_pcsub = new QCheckBox(tr("Include parameterized cell sub-masters"));
    vb->addWidget(cv_pcsub);
    connect(cv_pcsub, SIGNAL(stateChanged(int)),
        this, SLOT(pcsub_btn_slot(int)));

    cv_viasub = new QCheckBox(tr("Include standard via cell sub-masters"));
    vb->addWidget(cv_viasub);
    connect(cv_viasub, SIGNAL(stateChanged(int)),
        this, SLOT(viasub_btn_slot(int)));

    cv_noflvias = new QCheckBox(tr(
        "Don't flatten standard vias, keep as instance at top level"));
    vb->addWidget(cv_noflvias);
    connect(cv_noflvias, SIGNAL(stateChanged(int)),
        this, SLOT(noflvias_btn_slot(int)));

    cv_noflpcs = new QCheckBox(tr(
        "Don't flatten pcells, keep as instance at top level"));
    vb->addWidget(cv_noflpcs);
    connect(cv_noflpcs, SIGNAL(stateChanged(int)),
        this, SLOT(noflpcs_btn_slot(int)));

    cv_nofllbs = new QCheckBox(tr(
        "Ignore labels in subcells when flattening"));
    vb->addWidget(cv_nofllbs);
    connect(cv_nofllbs, SIGNAL(stateChanged(int)),
        this, SLOT(nofllbs_btn_slot(int)));

    cv_nolabels = new QCheckBox(tr(
        "Skip reading text labels from physical archives"));
    vb->addWidget(cv_nolabels);
    connect(cv_nolabels, SIGNAL(stateChanged(int)),
        this, SLOT(nolabels_btn_slot(int)));

    cv_keepbad = new QCheckBox(tr("Keep bad output (for debugging)"));
    vb->addWidget(cv_keepbad);
    connect(cv_keepbad, SIGNAL(stateChanged(int)),
        this, SLOT(keepbad_btn_slot(int)));

    // The Convert File page.
    //
    page = new QWidget();
    cv_nbook->addTab(page, tr("Convert File"));
    vb = new QVBoxLayout(page);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);


    // Layer list.
    //
    cv_llist = new QTlayerList();
    vb->addWidget(cv_llist);

    // Cell name mapping.
    //
    cv_cnmap = new QTcnameMap(false);
    vb->addWidget(cv_cnmap);

    // Window
    //
    cv_wnd = new QTwindowCfg((WndSensMode(*)())&wnd_sens_test, WndFuncCvt);
    vb->addWidget(cv_wnd);

    // Go button
    //
    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    tbtn = new QToolButton();
    tbtn->setText(tr("Convert"));
    hb->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(convert_btn_slot()));

    // Conversion scale
    //
    cv_sb_scale = new QTdoubleSpinBox();
    hb->addWidget(cv_sb_scale);
    cv_sb_scale->setMinimum(CDSCALEMIN);
    cv_sb_scale->setMaximum(CDSCALEMAX);
    cv_sb_scale->setDecimals(5);
    cv_sb_scale->setValue(FIO()->TransScale());
    connect(cv_sb_scale, SIGNAL(valueChanged(double)),
        this, SLOT(scale_changed_slot(double)));

    cv_tx_label = new QLabel(tr("Conversion Scale Factor"));
    hb->addWidget(cv_tx_label);

    // Dismiss button
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Dismiss");
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    if (inp_type == cConvert::cvDefault)
        inp_type = cv_inp_type;
    update(inp_type);
}


QTconvertFmtDlg::~QTconvertFmtDlg()
{
    instPtr = 0;
    if (cv_caller)
        QTdev::Deselect(cv_caller);
    if (cv_callback)
        (*cv_callback)(-1, cv_arg);
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTconvertFmtDlg
#include "qtinterf/qtmacos_event.h"
#endif


void
QTconvertFmtDlg::update(int inp_type)
{
    int op_type = inp_type >> 16;
    inp_type &= 0xffff;

    QTdev::SetStatus(cv_strip,
        CDvdb()->getVariable(VA_StripForExport));
    QTdev::SetStatus(cv_libsub,
        CDvdb()->getVariable(VA_KeepLibMasters));
    QTdev::SetStatus(cv_pcsub,
        CDvdb()->getVariable(VA_PCellKeepSubMasters));
    QTdev::SetStatus(cv_viasub,
        CDvdb()->getVariable(VA_ViaKeepSubMasters));
    QTdev::SetStatus(cv_noflvias,
        CDvdb()->getVariable(VA_NoFlattenStdVias));
    QTdev::SetStatus(cv_noflpcs,
        CDvdb()->getVariable(VA_NoFlattenPCells));
    QTdev::SetStatus(cv_nofllbs,
        CDvdb()->getVariable(VA_NoFlattenLabels));
    QTdev::SetStatus(cv_nolabels,
        CDvdb()->getVariable(VA_NoReadLabels));
    QTdev::SetStatus(cv_keepbad,
        CDvdb()->getVariable(VA_KeepBadArchive));
    cv_sb_scale->setValue(FIO()->TransScale());

    cv_fmt->update();
    cv_wnd->update();
    cv_llist->update();
    cv_cnmap->update();
    cv_sens_test();

    if (inp_type >= cConvert::cvLayoutFile &&
            inp_type <= cConvert::cvNativeDir)
        cv_input->setCurrentIndex(inp_type - 1);
    if (inp_type == cConvert::cvChdName)
        cv_fmt->configure(QTconvOutFmt::cvofmt_chd);
    else if (inp_type == cConvert::cvChdFile)
        cv_fmt->configure(QTconvOutFmt::cvofmt_chdfile);
    else if (inp_type == cConvert::cvNativeDir)
        cv_fmt->configure(QTconvOutFmt::cvofmt_native);

    if (op_type > 0) {
        op_type -= 1;
        cv_fmt->set_page(op_type);
    }
    cv_format_proc(cv_fmt_type);
}


// Static function.
void
QTconvertFmtDlg::cv_format_proc(int type)
{
    if (!instPtr)
        return;
    cv_fmt_type = type;
    if (type == cConvert::cvXic) {
        instPtr->cv_llist->setEnabled(true);
        instPtr->cv_cnmap->setEnabled(true);
        instPtr->cv_wnd->setEnabled(true);
        instPtr->cv_strip->setEnabled(false);
    }
    else if (type == cConvert::cvTxt) {
        instPtr->cv_llist->setEnabled(false);
        instPtr->cv_cnmap->setEnabled(false);
        instPtr->cv_wnd->setEnabled(false);
        instPtr->cv_strip->setEnabled(false);
    }
    else if (type == cConvert::cvChd) {
        bool cn = (cv_inp_type == cConvert::cvLayoutFile ||
            cv_inp_type == cConvert::cvNativeDir);
        instPtr->cv_llist->setEnabled(false);
        instPtr->cv_cnmap->setEnabled(cn);
        instPtr->cv_wnd->setEnabled(false);
        instPtr->cv_strip->setEnabled(false);
    }
    else if (type == cConvert::cvCgd) {
        bool cn = (cv_inp_type == cConvert::cvLayoutFile ||
            cv_inp_type == cConvert::cvNativeDir);
        instPtr->cv_llist->setEnabled(true);
        instPtr->cv_cnmap->setEnabled(cn);
        instPtr->cv_wnd->setEnabled(false);
        instPtr->cv_strip->setEnabled(false);
    }
    else {
        bool cn = (cv_inp_type == cConvert::cvLayoutFile ||
            cv_inp_type == cConvert::cvNativeDir);
        instPtr->cv_llist->setEnabled(true);
        instPtr->cv_cnmap->setEnabled(cn);
        instPtr->cv_wnd->setEnabled(true);
        instPtr->cv_strip->setEnabled(true);
    }
    cv_sens_test();
}


// Static function.
void
QTconvertFmtDlg::cv_sens_test()
{
    bool ns = cv_fmt_type == cConvert::cvGds &&
        instPtr->cv_fmt->gds_text_input();
    if (!ns && cv_fmt_type == cConvert::cvTxt)
        ns = true;
    if (!ns && (cv_fmt_type == cConvert::cvChd ||
            cv_fmt_type == cConvert::cvCgd))
        ns = true;

    instPtr->cv_sb_scale->setEnabled(!ns);
    instPtr->cv_tx_label->setEnabled(!ns);
    instPtr->cv_wnd->set_sens();
}


// Static function.
QTconvertFmtDlg::CvSensMode
QTconvertFmtDlg::wnd_sens_test()
{
    if (cv_inp_type == cConvert::cvNativeDir)
        return (CvSensNone);
    if (cv_fmt_type == cConvert::cvGds && instPtr->cv_fmt->gds_text_input())
        return (CvSensNone);
    if (cv_fmt_type == cConvert::cvTxt)
        return (CvSensNone);
    if (cv_fmt_type == cConvert::cvXic)
        return (CvSensFlatten);
    return (CvSensAllModes);
}


void
QTconvertFmtDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:convt"))
}


void
QTconvertFmtDlg::input_menu_slot(int ix)
{
    int a = 1 + ix;
    if (a == cConvert::cvLayoutFile) {
        cv_cnmap->setEnabled(true);
        cv_inp_type = cConvert::cvLayoutFile;
        cv_fmt->configure(QTconvOutFmt::cvofmt_file);
    }
    else if (a == cConvert::cvChdName) {
        cv_cnmap->setEnabled(false);
        cv_inp_type = cConvert::cvChdName;
        cv_fmt->configure(QTconvOutFmt::cvofmt_chd);
    }
    else if (a == cConvert::cvChdFile) {
        cv_cnmap->setEnabled(false);
        cv_inp_type = cConvert::cvChdFile;
        cv_fmt->configure(QTconvOutFmt::cvofmt_chdfile);
    }
    else if (a == cConvert::cvNativeDir) {
        cv_cnmap->setEnabled(true);
        cv_inp_type = cConvert::cvNativeDir;
        cv_fmt->configure(QTconvOutFmt::cvofmt_native);
    }
    cv_sens_test();
}


void
QTconvertFmtDlg::nbook_page_slot(int pg)
{
    const char *lb;
    if (pg == 0)
        lb = "Set parameters for converting cell data";
    else
        lb = "Convert cell data file";
    cv_label->setText(lb);
}


void
QTconvertFmtDlg::strip_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_StripForExport, 0);
    else
        CDvdb()->clearVariable(VA_StripForExport);
}


void
QTconvertFmtDlg::libsub_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_KeepLibMasters, 0);
    else
        CDvdb()->clearVariable(VA_KeepLibMasters);
}


void
QTconvertFmtDlg::pcsub_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_PCellKeepSubMasters, "");
    else
        CDvdb()->clearVariable(VA_PCellKeepSubMasters);
}


void
QTconvertFmtDlg::viasub_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_ViaKeepSubMasters, "");
    else
        CDvdb()->clearVariable(VA_ViaKeepSubMasters);
}


void
QTconvertFmtDlg::noflvias_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoFlattenStdVias, 0);
    else
        CDvdb()->clearVariable(VA_NoFlattenStdVias);
}


void
QTconvertFmtDlg::noflpcs_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoFlattenPCells, 0);
    else
        CDvdb()->clearVariable(VA_NoFlattenPCells);
}


void
QTconvertFmtDlg::nofllbs_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoFlattenLabels, 0);
    else
        CDvdb()->clearVariable(VA_NoFlattenLabels);
}


void
QTconvertFmtDlg::nolabels_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoReadLabels, 0);
    else
        CDvdb()->clearVariable(VA_NoReadLabels);
}


void
QTconvertFmtDlg::keepbad_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_KeepBadArchive, 0);
    else
        CDvdb()->clearVariable(VA_KeepBadArchive);
}


void
QTconvertFmtDlg::convert_btn_slot()
{
    int code = cv_fmt_type;
    if (cv_inp_type == cConvert::cvChdName)
        // Input is a CHD name from the database.
        code |= (cConvert::CVchdName << 16);
    else if (cv_inp_type == cConvert::cvChdFile)
        // Input is a saved CHD file from disk.
        code |= (cConvert::CVchdFile << 16);
    else if (cv_inp_type == cConvert::cvNativeDir)
        // Input is a directory containing native files.
        code |= (cConvert::CVnativeDir << 16);
    else if (code == cConvert::cvGds && cv_fmt->gds_text_input())
        // Input is a gds-text file.
        code |= (cConvert::CVgdsText << 16);
    else
        // Input is normal archive file.
        code |= (cConvert::CVlayoutFile << 16);
    if (!cv_callback || !(*cv_callback)(code, cv_arg))
        Cvt()->PopUpConvert(0, MODE_OFF, 0, 0, 0);
}


void
QTconvertFmtDlg::scale_changed_slot(double d)
{
    FIO()->SetTransScale(d);
}


void
QTconvertFmtDlg::dismiss_btn_slot()
{
    Cvt()->PopUpConvert(0, MODE_OFF, 0, 0, 0);
}

