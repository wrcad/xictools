
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

#include "qtextset.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_rlsolver.h"
#include "sced.h"
#include "dsp_inlines.h"
#include "events.h"
#include "promptline.h"
#include "errorlog.h"
#include "menu.h"
#include "ebtn_menu.h"
#include "tech.h"
#include "tech_extract.h"
#include "miscutil/filestat.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>

//--------------------------------------------------------------------
// Pop-up to control misc. extraction variables.
//
// Help system keywords used:
//  xic:excfg

void
cExt::PopUpExtSetup(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTextSetupDlg::self())
            QTextSetupDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTextSetupDlg::self())
            QTextSetupDlg::self()->update();
        return;
    }
    if (QTextSetupDlg::self())
        return;

    new QTextSetupDlg(caller);

    QTdev::self()->SetPopupLocation(GRloc(LW_LR), QTextSetupDlg::self(),
        QTmainwin::self()->Viewport());
    QTextSetupDlg::self()->show();
}
// End of cExt functions


QTextSetupDlg *QTextSetupDlg::instPtr;

QTextSetupDlg::QTextSetupDlg(GRobject c)
{
    instPtr = this;
    es_caller = c;
    es_notebook = 0;
    es_clrex = 0;
    es_doex = 0;
    es_p1_extview = 0;
    es_p1_groups = 0;
    es_p1_nodes = 0;
    es_p1_terms = 0;
    es_p1_cterms = 0;
    es_p1_recurs = 0;
    es_p1_tedit = 0;
    es_p1_tfind = 0;
    es_p2_nlprpset = 0;
    es_p2_nlprp = 0;
    es_p2_nllset = 0;
    es_p2_nll = 0;
    es_p2_ignlab = 0;
    es_p2_oldlab = 0;
    es_p2_updlab = 0;
    es_p2_merge = 0;
    es_p2_vcvx = 0;
    es_p2_lmax = 0;
    es_p2_sb_vdepth = 0;
    es_p2_vsubs = 0;
    es_p2_gpglob = 0;
    es_p2_gpmulti = 0;
    es_p2_gpmthd = 0;
    es_p3_noseries = 0;
    es_p3_nopara = 0;
    es_p3_keepshrt = 0;
    es_p3_nomrgshrt = 0;
    es_p3_nomeas = 0;
    es_p3_usecache = 0;
    es_p3_nordcache = 0;
    es_p3_deltaset = 0;
    es_p3_sb_delta = 0;
    es_p3_trytile = 0;
    es_p3_lmax = 0;
    es_p3_lgrid = 0;
    es_p3_sb_maxpts = 0;
    es_p3_sb_gridpts = 0;
    es_p4_flkeyset = 0;
    es_p4_flkeys = 0;
    es_p4_exopq = 0;
    es_p4_vrbos = 0;
    es_p4_glbexset = 0;
    es_p4_glbex = 0;
    es_p4_noperm = 0;
    es_p4_apmrg = 0;
    es_p4_apfix = 0;
    es_p4_sb_loop = 0;
    es_p4_sb_iters = 0;
    es_gpmhst = 0;
    es_devdesc = 0;

    setWindowTitle(tr("Extraction Setup"));
    setAttribute(Qt::WA_DeleteOnClose);
//    gtk_window_set_resizable(GTK_WINDOW(es_popup), false);

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
    QLabel *label = new QLabel(tr("Set parameters for extraction"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    es_notebook = new QTabWidget();
    vbox->addWidget(es_notebook);

    views_and_ops_page();
    net_and_cell_page();
    devs_page();
    misc_page();

    // set/clear extraction buttons, dismiss button
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    es_clrex = new QPushButton(tr("Clear Extraction"));
    hbox->addWidget(es_clrex);
    connect(es_clrex, SIGNAL(clicked()), this, SLOT(clrex_btn_slot()));

    es_doex = new QPushButton(tr("Do Extraction"));
    hbox->addWidget(es_doex);
    connect(es_doex, SIGNAL(clicked()), this, SLOT(doex_btn_slot()));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTextSetupDlg::~QTextSetupDlg()
{
    // Must do this before zeroing Es.
    if (es_p1_tedit && QTdev::GetStatus(es_p1_tedit))
        QTdev::CallCallback(es_p1_tedit);
    instPtr = 0;
    delete es_devdesc;
    SCD()->PopUpNodeMap(0, MODE_OFF);
    if (es_caller)
        QTdev::Deselect(es_caller);
}


void
QTextSetupDlg::views_and_ops_page()
{
    QWidget *page = new QWidget();
    es_notebook->addTab(page, tr("Views and\nOperations"));
    QVBoxLayout *vbox = new QVBoxLayout(page);
    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // The Show group.
    //
    QGroupBox *gb = new QGroupBox(tr("Show"));
    QGridLayout *grid = new QGridLayout(gb);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    es_p1_extview = new QCheckBox(tr("Extraction View"));
    grid->addWidget(es_p1_extview, 0, 0);
    connect(es_p1_extview, SIGNAL(stateChanged(int)),
        this, SLOT(p1_extview_btn_slot(int)));

    es_p1_groups = new QCheckBox(tr("Groups"));
    grid->addWidget(es_p1_groups, 0, 1);
    connect(es_p1_groups, SIGNAL(stateChanged(int)),
        this, SLOT(p1_groups_btn_slot(int)));

    es_p1_nodes = new QCheckBox(tr("Nodes"));
    grid->addWidget(es_p1_nodes, 0, 2);
    connect(es_p1_nodes, SIGNAL(stateChanged(int)),
        this, SLOT(p1_nodes_btn_slot(int)));

    es_p1_terms = new QCheckBox(tr("All Terminals"));
    grid->addWidget(es_p1_terms, 1, 0);
    connect(es_p1_terms, SIGNAL(stateChanged(int)),
        this, SLOT(p1_terms_btn_slot(int)));

    es_p1_cterms = new QCheckBox(tr("Cell Terminals Only"));
    grid->addWidget(es_p1_cterms, 1, 1);
    connect(es_p1_cterms, SIGNAL(stateChanged(int)),
        this, SLOT(p1_cterms_btn_slot(int)));

    // The Terminals group.
    //
    gb = new QGroupBox(tr("Terminals"));
    vbox->addWidget(gb);
    QVBoxLayout *vb = new QVBoxLayout(gb);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);
    QHBoxLayout *hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    QPushButton *btn = new QPushButton(tr("Reset Terms"));
    hb->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(p1_rsterms_btn_slot()));

    btn = new QPushButton(tr("Reset Subckts"));
    hb->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(p1_rssubs_btn_slot()));

    es_p1_recurs = new QCheckBox(tr("Recursive"));
    hb->addWidget(es_p1_recurs);
    connect(es_p1_recurs, SIGNAL(stateChanged(int)),
        this, SLOT(p1_recurs_btn_slot(int)));

    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    es_p1_tedit = new QPushButton(tr("Edit Terminals"));
    es_p1_tedit->setCheckable(true);
    hb->addWidget(es_p1_tedit);
    connect(es_p1_tedit, SIGNAL(toggled(bool)),
        this, SLOT(p1_tedit_btn_slot(bool)));

    es_p1_tfind = new QPushButton(tr("Find Terminal"));
    es_p1_tfind->setCheckable(true);
    hb->addWidget(es_p1_tfind);
    connect(es_p1_tfind, SIGNAL(toggled(bool)),
        this, SLOT(p1_tfind_btn_slot(bool)));

    // Selection of unassociated objects.
    //
    hb = new QHBoxLayout();
    vbox->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    QLabel *label = new QLabel(tr("Select Unassociated"));
    hb->addWidget(label);

    btn = new QPushButton(tr("Groups/Nodes"));
    hb->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(p1_uagn_btn_slot()));

    btn = new QPushButton(tr("Devices"));
    hb->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(p1_uadev_btn_slot()));

    btn = new QPushButton(tr("Subckts"));
    hb->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(p1_uasub_btn_slot()));
}


void
QTextSetupDlg::net_and_cell_page()
{
    QWidget *page = new QWidget();
    es_notebook->addTab(page, tr("Net\nConfig"));
    QVBoxLayout *vbox = new QVBoxLayout(page);
    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);
    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    // Nets
    //
    QGroupBox *gb = new QGroupBox(tr("Net label purpose name"));
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);

    es_p2_nlprpset = new QPushButton(tr("Apply"));
    es_p2_nlprpset->setCheckable(true);
    hb->addWidget(es_p2_nlprpset);
    connect(es_p2_nlprpset, SIGNAL(toggled(bool)),
        this, SLOT(p2_papply_btn_slot(bool)));

    es_p2_nlprp = new QLineEdit();
    hb->addWidget(es_p2_nlprp);

    gb = new QGroupBox(tr("Net label layer"));
    hbox->addWidget(gb);
    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);

    es_p2_nllset = new QPushButton(tr("Apply"));
    es_p2_nllset->setCheckable(true);
    hb->addWidget(es_p2_nllset);
    connect(es_p2_nllset, SIGNAL(toggled(bool)),
        this, SLOT(p2_lapply_btn_slot(bool)));

    es_p2_nll = new QLineEdit();
    hb->addWidget(es_p2_nll);

    // Check boxes.
    //
    es_p2_ignlab = new QCheckBox(tr("Ignore net name labels"));
    vbox->addWidget(es_p2_ignlab);
    connect(es_p2_ignlab, SIGNAL(stateChanged(int)),
        this, SLOT(p2_ignnm_btn_slot(int)));

    es_p2_oldlab = new QCheckBox(tr(
        "Find old-style net (term name) labels"));
    vbox->addWidget(es_p2_oldlab);
    connect(es_p2_oldlab, SIGNAL(stateChanged(int)),
        this, SLOT(p2_oldlab_btn_slot(int)));

    es_p2_updlab = new QCheckBox(tr(
        "Update net name labels after association"));
    vbox->addWidget(es_p2_updlab);
    connect(es_p2_updlab, SIGNAL(stateChanged(int)),
        this, SLOT(p2_updlab_btn_slot(int)));

    es_p2_merge = new QCheckBox(tr(
        "Merge groups with matching net names"));
    vbox->addWidget(es_p2_merge);
    connect(es_p2_merge, SIGNAL(stateChanged(int)),
        this, SLOT(p2_merge_btn_slot(int)));

    // Via detection group.
    //
    gb = new QGroupBox(tr("Via Detection"));
    vbox->addWidget(gb);
    QVBoxLayout *vb = new QVBoxLayout(gb);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);
    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    es_p2_vcvx = new QCheckBox(tr("Assume convex vias"));
    hb->addWidget(es_p2_vcvx);
    connect(es_p2_vcvx, SIGNAL(stateChanged(int)),
        this, SLOT(p2_vcvx_btn_slot(int)));

    hb->addStretch(1);

    es_p2_lmax = new QLabel(tr("Via search depth"));
    hb->addWidget(es_p2_lmax);

    es_p2_sb_vdepth = new QSpinBox();
    es_p2_sb_vdepth->setRange(0, CDMAXCALLDEPTH);
    es_p2_sb_vdepth->setValue(EXT_DEF_VIA_SEARCH_DEPTH);
    hb->addWidget(es_p2_sb_vdepth);
    connect(es_p2_sb_vdepth, SIGNAL(valueChanged(int)),
        this, SLOT(p2_vdepth_slot(int)));

    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    es_p2_vsubs = new QCheckBox(tr(
        "Check for via connections between subcells"));
    hb->addWidget(es_p2_vsubs);
    connect(es_p2_vsubs, SIGNAL(stateChanged(int)),
        this, SLOT(p2_vsubs_btn_slot(int)));

    // Ground plane group.
    //
    gb = new QGroupBox(tr("Ground Plane Handling"));
    vbox->addWidget(gb);
    vb = new QVBoxLayout(gb);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    es_p2_gpglob = new QCheckBox(tr(
        "Assume clear-field ground plane is global"));
    vb->addWidget(es_p2_gpglob);
    connect(es_p2_gpglob, SIGNAL(stateChanged(int)),
        this, SLOT(p2_gpglob_btn_slot(int)));

    es_p2_gpmulti = new QCheckBox(tr(
        "Invert dark-field ground plane for multi-nets"));
    vb->addWidget(es_p2_gpmulti);
    connect(es_p2_gpmulti, SIGNAL(stateChanged(int)),
        this, SLOT(p2_gpmulti_btn_slot(int)));

    es_p2_gpmthd = new QComboBox();
    vb->addWidget(es_p2_gpmthd);
    es_p2_gpmthd->addItem(tr(
        "Invert in each cell, clip out subcells"));
    es_p2_gpmthd->addItem(tr(
        "Invert flat in top-level cell"));
    es_p2_gpmthd->addItem(tr(
        "Invert flat in all cells"));
    es_p2_gpmthd->setCurrentIndex(Tech()->GroundPlaneMode());
    connect(es_p2_gpmthd, SIGNAL(currentIndexChanged(int)),
        this, SLOT(p2_gpmthd_menu_slot(int)));
}


namespace {
    const char *MIDX = "midx";
}

// Page 3, Device Config.
//
void
QTextSetupDlg::devs_page()
{
    QWidget *page = new QWidget();
    es_notebook->addTab(page, tr("Device\nConfig"));
    QVBoxLayout *vbox = new QVBoxLayout(page);
    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // Menu bar.
    //
    QMenuBar *menubar = new QMenuBar();
    vbox->addWidget(menubar);

    // Device Block menu.
    QMenu *menu = menubar->addMenu(tr("&Device Block"));

    // New, 0, es_dev_menu_proc, 0, 0
    QAction *a = menu->addAction(tr("New"));

    // Delete, 0, es_dev_menu_proc, 1, <CheckItem>
    a = menu->addAction(tr("Delete"));
    a->setCheckable(true);

    // Undelete, 0, es_dev_menu_proc, 2, 0);
    a = menu->addAction(tr("Undelete"));
    a->setCheckable(true);

//    item = gtk_separator_menu_item_new();

    // Check boxes.
    //
    es_p3_noseries = new QCheckBox(tr("Don't merge series devices"));
    vbox->addWidget(es_p3_noseries);
    connect(es_p3_noseries, SIGNAL(stateChanged(int)),
        this, SLOT(p3_noseries_btn_slot(int)));

    es_p3_nopara = new QCheckBox(tr("Don't merge parallel devices"));
    vbox->addWidget(es_p3_nopara);
    connect(es_p3_nopara, SIGNAL(stateChanged(int)),
        this, SLOT(p3_nopara_btn_slot(int)));

    es_p3_keepshrt = new QCheckBox(tr(
        "Include devices with terminals shorted"));
    vbox->addWidget(es_p3_keepshrt);
    connect(es_p3_keepshrt, SIGNAL(stateChanged(int)),
        this, SLOT(p3_keepshrt_btn_slot(int)));

    es_p3_nomrgshrt = new QCheckBox(tr(
        "Don't merge devices with terminals shorted"));
    vbox->addWidget(es_p3_nomrgshrt);
    connect(es_p3_nomrgshrt, SIGNAL(stateChanged(int)),
        this, SLOT(p3_nomrgshrt_btn_slot(int)));

    es_p3_nomeas = new QCheckBox(tr(
        "Skip device parameter measurement"));
    vbox->addWidget(es_p3_nomeas);
    connect(es_p3_nomeas, SIGNAL(stateChanged(int)),
        this, SLOT(p3_nomeas_btn_slot(int)));

    es_p3_usecache = new QCheckBox(tr(
        "Use measurement results cache property"));
    vbox->addWidget(es_p3_usecache);
    connect(es_p3_usecache, SIGNAL(stateChanged(int)),
        this, SLOT(p3_usecache_btn_slot(int)));

    es_p3_nordcache = new QCheckBox(tr(
        "Don't read measurement results from property"));
    vbox->addWidget(es_p3_nordcache);
    connect(es_p3_nordcache, SIGNAL(stateChanged(int)),
        this, SLOT(p3_nordcache_btn_slot(int)));

    // RL solver group.
    //
    QGroupBox *gb = new QGroupBox(tr("Resistor/Inductor Extraction"));
    vbox->addWidget(gb);
    QGridLayout *grid = new QGridLayout(gb);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    es_p3_deltaset = new QCheckBox(tr("Set/use fixed grid size"));
    grid->addWidget(es_p3_deltaset, 0, 0);
    connect(es_p3_deltaset, SIGNAL(stateChanged(int)),
        this, SLOT(p3_deltaset_btn_slot(int)));

    int ndgt = CD()->numDigits();
    es_p3_sb_delta = new QDoubleSpinBox();
    es_p3_sb_delta->setRange(0.01, 1000.0);
    es_p3_sb_delta->setDecimals(ndgt);
    es_p3_sb_delta->setValue(0.01);
    grid->addWidget(es_p3_sb_delta, 0, 2);
    connect(es_p3_sb_delta, SIGNAL(valueChanged(double)),
        this, SLOT(p3_gridsize_slot(double)));

    es_p3_trytile = new QCheckBox(tr("Try to tile"));
    grid->addWidget(es_p3_trytile, 1, 0);
    connect(es_p3_trytile, SIGNAL(stateChanged(int)),
        this, SLOT(p3_trytile_btn_slot(int)));

    es_p3_lmax = new QLabel(tr("Maximum tile count per device"));
    grid->addWidget(es_p3_lmax, 1, 1);

    es_p3_sb_maxpts = new QSpinBox();
    es_p3_sb_maxpts->setRange(1000, 100000);
    es_p3_sb_maxpts->setValue(1000);
    grid->addWidget(es_p3_sb_maxpts, 1, 2);
    connect(es_p3_sb_maxpts, SIGNAL(valueChanged(int)),
        this, SLOT(p3_maxpts_slot(int)));

    es_p3_lgrid = new QLabel(tr("Set fixed per-device grid cell count"));
    grid->addWidget(es_p3_lgrid, 2, 1);

    es_p3_sb_gridpts = new QSpinBox();
    es_p3_sb_gridpts->setRange(10, 100000);
    es_p3_sb_gridpts->setValue(10);
    grid->addWidget(es_p3_sb_gridpts, 2, 2);
    connect(es_p3_sb_gridpts, SIGNAL(valueChanged(int)),
        this, SLOT(p3_gridpts_slot(int)));
}


// Page 4, Misc Config.
//
void
QTextSetupDlg::misc_page()
{
    QWidget *page = new QWidget();
    es_notebook->addTab(page, tr("Misc\nConfig"));
    QVBoxLayout *vbox = new QVBoxLayout(page);
    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // Flatten token entry group, check box.
    //
    QGroupBox *gb = new QGroupBox(tr("Cell flattening name keys"));
    vbox->addWidget(gb);
    QHBoxLayout *hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qmtop);
    hbox->setSpacing(2);

    es_p4_flkeyset = new QPushButton(tr("Apply"));
    es_p4_flkeyset->setCheckable(true);
    hbox->addWidget(es_p4_flkeyset);
    connect(es_p4_flkeyset, SIGNAL(toggled(bool)),
        this, SLOT(p4_flapply_btn_slot(bool)));

    es_p4_flkeys = new QLineEdit();
    hbox->addWidget(es_p4_flkeys);

    es_p4_exopq = new QCheckBox(tr(
        "Extract opaque cells, ignore OPAQUE flag"));
    vbox->addWidget(es_p4_exopq);
    connect(es_p4_exopq, SIGNAL(stateChanged(int)),
        this, SLOT(p4_exopq_btn_slot(int)));

    es_p4_vrbos = new QCheckBox(tr(
        "Be very verbose on prompt line during extraction."));
    vbox->addWidget(es_p4_vrbos);
    connect(es_p4_vrbos, SIGNAL(stateChanged(int)),
        this, SLOT(p4_vrbos_btn_slot(int)));


    gb = new QGroupBox(tr("Global exclude layer expression"));
    vbox->addWidget(gb);
    hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    es_p4_glbexset = new QPushButton(tr("Apply"));
    es_p4_glbexset->setCheckable(true);
    hbox->addWidget(es_p4_glbexset);
    connect(es_p4_glbexset, SIGNAL(toggled(bool)),
        this, SLOT(p4_glapply_btn_slot(bool)));

    es_p4_glbex = new QLineEdit();
    hbox->addWidget(es_p4_glbex);

    // Association group.
    //
    gb = new QGroupBox(tr("Association"));
    vbox->addWidget(gb);
    QGridLayout *grid = new QGridLayout(gb);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    es_p4_noperm = new QCheckBox(tr(
        "Don't run symmetry trials in association"));
    grid->addWidget(es_p4_noperm, 0, 0);
    connect(es_p4_noperm, SIGNAL(stateChanged(int)),
        this, SLOT(p4_noperm_btn_slot(int)));

    es_p4_apmrg = new QCheckBox(tr(
        "Logically merge physical contacts for split net handling"));
    grid->addWidget(es_p4_apmrg, 1, 0, 1, 2);
    connect(es_p4_apmrg, SIGNAL(stateChanged(int)),
        this, SLOT(p4_apmrg_btn_slot(int)));

    es_p4_apfix = new QCheckBox(tr(
        "Apply post-association permutation fix"));
    grid->addWidget(es_p4_apfix, 2, 0);
    connect(es_p4_apfix, SIGNAL(stateChanged(int)),
        this, SLOT(p4_apfix_btn_slot(int)));

    QLabel *label = new QLabel(tr("Maximum association loop count"));
    grid->addWidget(label, 3, 0);

    es_p4_sb_loop = new QSpinBox();
    es_p4_sb_loop->setRange(0, 1000000);
    es_p4_sb_loop->setValue(EXT_DEF_LVS_LOOP_MAX);
    grid->addWidget(es_p4_sb_loop, 3, 1);
    connect(es_p4_sb_loop, SIGNAL(valueChanged(int)),
        this, SLOT(p4_loop_count_slot(int)));

    label = new QLabel(tr("Maximum association iterations"));
    grid->addWidget(label, 4, 0);

    es_p4_sb_iters = new QSpinBox();
    es_p4_sb_iters->setRange(10, 1000000);
    es_p4_sb_iters->setValue(EXT_DEF_LVS_ITER_MAX);
    grid->addWidget(es_p4_sb_iters, 4, 1);
    connect(es_p4_sb_iters, SIGNAL(valueChanged(int)),
        this, SLOT(p4_iter_count_slot(int)));
}


void
QTextSetupDlg::update()
{
    // page 1
    bool physmode = DSP()->CurMode() == Physical;
    es_p1_terms->setEnabled(physmode);
    es_p1_cterms->setEnabled(physmode);

    if (DSP()->CurMode() == Electrical) {
        QTdev::SetStatus(es_p1_tedit,
            Menu()->MenuButtonStatus(MMside, MenuSUBCT));
        QTdev::SetStatus(es_p1_tfind,
            Menu()->MenuButtonStatus(MMside, MenuNODMP));
    }

    QTdev::SetStatus(es_p1_extview, EX()->isExtractionView());
    QTdev::SetStatus(es_p1_groups,
        EX()->isShowingGroups() && !EX()->isShowingNodes());
    QTdev::SetStatus(es_p1_nodes,
        EX()->isShowingGroups() && EX()->isShowingNodes());
    QTdev::SetStatus(es_p1_terms,
        DSP()->ShowTerminals() && DSP()->TerminalsVisible());
    QTdev::SetStatus(es_p1_cterms,
        DSP()->ShowTerminals() && DSP()->ContactsVisible());

    // page 2
    const char *s = CDvdb()->getVariable(VA_PinPurpose);
    if (s) {
        es_p2_nlprp->setText(s);
        QTdev::SetStatus(es_p2_nlprpset, true);
    }
    else
        QTdev::SetStatus(es_p2_nlprpset, false);

    s = CDvdb()->getVariable(VA_PinLayer);
    if (s) {
        es_p2_nll->setText(s);
        QTdev::SetStatus(es_p2_nllset, true);
    }
    else
        QTdev::SetStatus(es_p2_nllset, false);

    QTdev::SetStatus(es_p2_ignlab,
        CDvdb()->getVariable(VA_IgnoreNetLabels) != 0);
    QTdev::SetStatus(es_p2_oldlab,
        CDvdb()->getVariable(VA_FindOldTermLabels) != 0);
    QTdev::SetStatus(es_p2_updlab,
        CDvdb()->getVariable(VA_UpdateNetLabels) != 0);
    QTdev::SetStatus(es_p2_merge,
        CDvdb()->getVariable(VA_MergeMatchingNamed) != 0);
    QTdev::SetStatus(es_p2_vcvx,
        CDvdb()->getVariable(VA_ViaConvex) != 0);
    QTdev::SetStatus(es_p2_vsubs,
        CDvdb()->getVariable(VA_ViaCheckBtwnSubs) != 0);

    QByteArray vdepth_ba = es_p2_sb_vdepth->cleanText().toLatin1();
    s = vdepth_ba.constData();
    int n;
    if (sscanf(s, "%d", &n) != EXT_DEF_VIA_SEARCH_DEPTH ||
            n != EX()->viaSearchDepth())
        es_p2_sb_vdepth->setValue(EX()->viaSearchDepth());

    QTdev::SetStatus(es_p2_gpglob,
        CDvdb()->getVariable(VA_GroundPlaneGlobal) != 0);
    QTdev::SetStatus(es_p2_gpmulti,
        CDvdb()->getVariable(VA_GroundPlaneMulti) != 0);

    if (es_gpmhst != (int)Tech()->GroundPlaneMode())
        es_p2_gpmthd->setCurrentIndex(Tech()->GroundPlaneMode());

    // page 3
    QTdev::SetStatus(es_p3_noseries,
        CDvdb()->getVariable(VA_NoMergeSeries) != 0);
    QTdev::SetStatus(es_p3_nopara,
        CDvdb()->getVariable(VA_NoMergeParallel) != 0);
    QTdev::SetStatus(es_p3_keepshrt,
        CDvdb()->getVariable(VA_KeepShortedDevs) != 0);
    QTdev::SetStatus(es_p3_nomrgshrt,
        CDvdb()->getVariable(VA_NoMergeShorted) != 0);
    QTdev::SetStatus(es_p3_nomeas,
        CDvdb()->getVariable(VA_NoMeasure) != 0);
    QTdev::SetStatus(es_p3_usecache,
        CDvdb()->getVariable(VA_UseMeasurePrpty) != 0);
    QTdev::SetStatus(es_p3_nordcache,
        CDvdb()->getVariable(VA_NoReadMeasurePrpty) != 0);
    QTdev::SetStatus(es_p3_trytile,
        CDvdb()->getVariable(VA_RLSolverTryTile) != 0);
    QTdev::SetStatus(es_p3_deltaset,
        CDvdb()->getVariable(VA_RLSolverDelta) != 0);

    QByteArray maxpts_ba = es_p3_sb_maxpts->cleanText().toLatin1();
    s = maxpts_ba.constData();
    if (sscanf(s, "%d", &n) != 1 || n != RLsolver::rl_maxgrid)
        es_p3_sb_maxpts->setValue(RLsolver::rl_maxgrid);

    QByteArray gridpts_ba = es_p3_sb_gridpts->cleanText().toLatin1();
    s = gridpts_ba.constData();
    if (sscanf(s, "%d", &n) != 1 || n != RLsolver::rl_numgrid)
        es_p3_sb_gridpts->setValue(RLsolver::rl_numgrid);

    if (QTdev::GetStatus(es_p3_deltaset)) {
        QByteArray delta_ba = es_p3_sb_delta->cleanText().toLatin1();
        s = delta_ba.constData();
        double d;
        if (sscanf(s, "%lf", &d) != 1 ||
                INTERNAL_UNITS(d) != RLsolver::rl_given_delta) {
            int ndgt = CD()->numDigits();
            es_p3_sb_delta->setDecimals(ndgt);
            es_p3_sb_delta->setValue(MICRONS(RLsolver::rl_given_delta));
        }
    }

    // page 4
    s = CDvdb()->getVariable(VA_FlattenPrefix);
    if (s) {
        es_p4_flkeys->setText(s);
        QTdev::SetStatus(es_p4_flkeyset, true);
    }
    else
        QTdev::SetStatus(es_p4_flkeyset, false);

    QTdev::SetStatus(es_p4_exopq,
        CDvdb()->getVariable(VA_ExtractOpaque) != 0);

    QTdev::SetStatus(es_p4_vrbos,
        CDvdb()->getVariable(VA_VerbosePromptline) != 0);

    s = CDvdb()->getVariable(VA_GlobalExclude);
    if (s) {
        es_p4_glbex->setText(s);
        QTdev::SetStatus(es_p4_glbexset, true);
    }
    else
        QTdev::SetStatus(es_p4_glbexset, false);

    QTdev::SetStatus(es_p4_noperm,
        CDvdb()->getVariable(VA_NoPermute) != 0);
    QTdev::SetStatus(es_p4_apmrg,
        CDvdb()->getVariable(VA_MergePhysContacts) != 0);
    QTdev::SetStatus(es_p4_apfix,
        CDvdb()->getVariable(VA_SubcPermutationFix) != 0);

    QByteArray loop_ba = es_p4_sb_loop->cleanText().toLatin1();
    s = loop_ba.constData();
    if (sscanf(s, "%d", &n) != 1 || n != cGroupDesc::assoc_loop_max())
        es_p4_sb_loop->setValue(cGroupDesc::assoc_loop_max());

    QByteArray iters_ba = es_p4_sb_iters->cleanText().toLatin1();
    s = iters_ba.constData();
    if (sscanf(s, "%d", &n) != 1 || n != cGroupDesc::assoc_iter_max())
        es_p4_sb_iters->setValue(cGroupDesc::assoc_iter_max());

    set_sens();
}


void
QTextSetupDlg::set_sens()
{
    if (QTdev::GetStatus(es_p2_nlprpset))
        es_p2_nlprp->setEnabled(false);
    else
        es_p2_nlprp->setEnabled(true);
    if (QTdev::GetStatus(es_p2_nllset))
        es_p2_nll->setEnabled(false);
    else
        es_p2_nll->setEnabled(true);
    if (QTdev::GetStatus(es_p2_gpmulti))
        es_p2_gpmthd->setEnabled(true);
    else
        es_p2_gpmthd->setEnabled(false);

    if (QTdev::GetStatus(es_p3_deltaset)) {
        es_p3_sb_delta->setEnabled(false);
        es_p3_trytile->setEnabled(false);
        es_p3_lmax->setEnabled(false);
        es_p3_sb_maxpts->setEnabled(false);
        es_p3_lgrid->setEnabled(false);
        es_p3_sb_gridpts->setEnabled(false);
    }
    else {
        es_p3_sb_delta->setEnabled(true);
        es_p3_trytile->setEnabled(true);

        if (QTdev::GetStatus(es_p3_trytile)) {
            es_p3_lmax->setEnabled(true);
            es_p3_sb_maxpts->setEnabled(true);
            es_p3_lgrid->setEnabled(false);
            es_p3_sb_gridpts->setEnabled(false);
        }
        else {
            es_p3_lmax->setEnabled(false);
            es_p3_sb_maxpts->setEnabled(false);
            es_p3_lgrid->setEnabled(true);
            es_p3_sb_gridpts->setEnabled(true);
        }
    }

    if (QTdev::GetStatus(es_p4_flkeyset))
        es_p4_flkeys->setEnabled(false);
    else
        es_p4_flkeys->setEnabled(true);
    if (QTdev::GetStatus(es_p4_glbexset))
        es_p4_glbex->setEnabled(false);
    else
        es_p4_glbex->setEnabled(true);

}


void
QTextSetupDlg::show_grp_node(QCheckBox *caller)
{
    CDs *cursdp = CurCell(Physical);
    if (caller && cursdp && Menu()->GetStatus(caller)) {
        if (!EX()->associate(cursdp)) {
            QTdev::Deselect(caller);
            return;
        }
        cGroupDesc *gd = cursdp->groups();
        if (!gd || gd->isempty()) {
            QTdev::Deselect(caller);
            return;
        }
        bool shownodes = (caller == es_p1_nodes);
        WindowDesc *wd;
        if (EX()->isShowingGroups()) {
            if (shownodes)
                QTdev::Deselect(es_p1_groups);
            else
                QTdev::Deselect(es_p1_nodes);
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wd = wgen.next()) != 0)
                gd->show_groups(wd, ERASE);
        }
        EX()->setShowingNodes(shownodes);
        EX()->setShowingGroups(true);
        gd->set_group_display(true);
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wd = wgen.next()) != 0)
            gd->show_groups(wd, DISPLAY);
    }
    else {
        cGroupDesc *gd = cursdp ? cursdp->groups() : 0;
        if (gd) {
            WindowDesc *wd;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wd = wgen.next()) != 0)
                gd->show_groups(wd, ERASE);
            gd->set_group_display(false);
        }
        EX()->setShowingGroups(false);
        EX()->setShowingNodes(false);
    }
}


void
QTextSetupDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:excfg"))
}


void
QTextSetupDlg::clrex_btn_slot()
{
    EX()->invalidateGroups();
    PL()->ShowPrompt("Extraction state invalidated.");
}


void
QTextSetupDlg::doex_btn_slot()
{
    EV()->InitCallback();
    if (!EX()->associate(CurCell(Physical)))
        PL()->ShowPrompt("Extraction/Association failed!");
    else
        PL()->ShowPrompt("Extraction/Association complete.");
}


void
QTextSetupDlg::dismiss_btn_slot()
{
    EX()->PopUpExtSetup(0, MODE_OFF);
}



void
QTextSetupDlg::p1_extview_btn_slot(int)
{
    EX()->showExtractionView(es_p1_extview);
}


void
QTextSetupDlg::p1_groups_btn_slot(int)
{
    if (!CurCell(Physical)) {
        Menu()->Deselect(es_p1_groups);
        PL()->ShowPrompt("No physical data for current cell!");
        return;
    }
    show_grp_node(es_p1_groups);
}


void
QTextSetupDlg::p1_nodes_btn_slot(int)
{
    if (!CurCell(Electrical)) {
        Menu()->Deselect(es_p1_nodes);
        PL()->ShowPrompt("No electrical data for current cell!");
        return;
    }
    show_grp_node(es_p1_nodes);
}


void
QTextSetupDlg::p1_terms_btn_slot(int)
{
    if (QTdev::GetStatus(es_p1_terms))
        QTdev::SetStatus(es_p1_cterms, false);
    EX()->showPhysTermsExec(es_p1_terms, false);
}


void
QTextSetupDlg::p1_cterms_btn_slot(int)
{
    if (QTdev::GetStatus(es_p1_cterms))
        QTdev::SetStatus(es_p1_terms, false);
    EX()->showPhysTermsExec(es_p1_cterms, true);
}


void
QTextSetupDlg::p1_rsterms_btn_slot()
{
    bool tmpt = DSP()->ShowTerminals();
    if (tmpt)
        DSP()->ShowTerminals(ERASE);
    CDcbin cbin(DSP()->CurCellName());
    EX()->reset(&cbin, false, true,
        QTdev::GetStatus(es_p1_recurs));
    if (tmpt)
        DSP()->ShowTerminals(DISPLAY);
}


void
QTextSetupDlg::p1_rssubs_btn_slot()
{
    bool tmpt = DSP()->ShowTerminals();
    if (tmpt)
        DSP()->ShowTerminals(ERASE);
    CDcbin cbin(DSP()->CurCellName());
    EX()->reset(&cbin, true, false,
        QTdev::GetStatus(es_p1_recurs));
    EX()->arrangeInstLabels(&cbin);
    if (tmpt)
        DSP()->ShowTerminals(DISPLAY);
}


void
QTextSetupDlg::p1_recurs_btn_slot(int)
{
    // nothing to do
}


void
QTextSetupDlg::p1_tedit_btn_slot(bool state)
{
    if (DSP()->CurMode() == Physical)
        EX()->editTermsExec(es_p1_tedit, es_p1_cterms);
    else {
        bool st = Menu()->MenuButtonStatus(MMside, MenuSUBCT);
        if (st != state)
            Menu()->MenuButtonPress(MMside, MenuSUBCT);
    }
}


void
QTextSetupDlg::p1_tfind_btn_slot(bool state)
{
    if (DSP()->CurMode() == Physical) {
        if (state)
            SCD()->PopUpNodeMap(es_p1_tfind, MODE_ON);
        else
            SCD()->PopUpNodeMap(0, MODE_OFF);
    }
    else {
        bool st = Menu()->MenuButtonStatus(MMside, MenuNODMP);
        if (st != state)
            Menu()->MenuButtonPress(MMside, MenuNODMP);
    }
}


void
QTextSetupDlg::p1_uagn_btn_slot()
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        return;
    }
    EX()->associate(cursdp);
    cGroupDesc *gd = cursdp->groups();
    gd->select_unassoc_groups();
    gd->select_unassoc_nodes();
}


void
QTextSetupDlg::p1_uadev_btn_slot()
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        return;
    }
    EX()->associate(cursdp);
    cGroupDesc *gd = cursdp->groups();
    gd->select_unassoc_pdevs();
    gd->select_unassoc_edevs();
}


void
QTextSetupDlg::p1_uasub_btn_slot()
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        return;
    }
    EX()->associate(cursdp);
    cGroupDesc *gd = cursdp->groups();
    gd->select_unassoc_psubs();
    gd->select_unassoc_esubs();
}



void
QTextSetupDlg::p2_papply_btn_slot(bool state)
{
    if (state) {
        QByteArray nlprp_ba = es_p2_nlprp->text().toLatin1();
        const char *s = nlprp_ba.constData();
        CDvdb()->setVariable(VA_PinPurpose, s);
    }
    else
        CDvdb()->clearVariable(VA_PinPurpose);
}


void
QTextSetupDlg::p2_lapply_btn_slot(bool state)
{
    if (state) {
        QByteArray nll_ba = es_p2_nll->text().toLatin1();
        const char *s = nll_ba.constData();
        CDvdb()->setVariable(VA_PinLayer, s);
    }
    else
        CDvdb()->clearVariable(VA_PinLayer);
}


void
QTextSetupDlg::p2_ignnm_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_IgnoreNetLabels, "");
    else
        CDvdb()->clearVariable(VA_IgnoreNetLabels);
}


void
QTextSetupDlg::p2_oldlab_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_FindOldTermLabels, "");
    else
        CDvdb()->clearVariable(VA_FindOldTermLabels);
}


void
QTextSetupDlg::p2_updlab_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_UpdateNetLabels, "");
    else
        CDvdb()->clearVariable(VA_UpdateNetLabels);
}


void
QTextSetupDlg::p2_merge_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_MergeMatchingNamed, "");
    else
        CDvdb()->clearVariable(VA_MergeMatchingNamed);
}


void
QTextSetupDlg::p2_vcvx_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_ViaConvex, "");
    else
        CDvdb()->clearVariable(VA_ViaConvex);
}


void
QTextSetupDlg::p2_vdepth_slot(int)
{
    QByteArray val_ba = es_p2_sb_vdepth->cleanText().toLatin1();
    const char *s = val_ba.constData();
    int n;
    if (sscanf(s, "%d", &n) == 1 && n >= 0) {
        if (n == EXT_DEF_VIA_SEARCH_DEPTH)
            CDvdb()->clearVariable(VA_ViaSearchDepth);
        else
            CDvdb()->setVariable(VA_ViaSearchDepth, s);
    }
}


void
QTextSetupDlg::p2_vsubs_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_ViaCheckBtwnSubs, "");
    else
        CDvdb()->clearVariable(VA_ViaCheckBtwnSubs);
}


void
QTextSetupDlg::p2_gpglob_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_GroundPlaneGlobal, "");
    else
        CDvdb()->clearVariable(VA_GroundPlaneGlobal);
}


void
QTextSetupDlg::p2_gpmulti_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_GroundPlaneMulti, "");
    else
        CDvdb()->clearVariable(VA_GroundPlaneMulti);
}


void
QTextSetupDlg::p2_gpmthd_menu_slot(int i)
{
    if (i == 0)
        CDvdb()->clearVariable(VA_GroundPlaneMethod);
    else if (i == 1)
        CDvdb()->setVariable(VA_GroundPlaneMethod, "1");
    else if (i == 2)
        CDvdb()->setVariable(VA_GroundPlaneMethod, "2");
    else
        return;
    es_gpmhst = i;
}



void
QTextSetupDlg::p3_noseries_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoMergeSeries, "");
    else
        CDvdb()->clearVariable(VA_NoMergeSeries);
}


void
QTextSetupDlg::p3_nopara_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoMergeParallel, "");
    else
        CDvdb()->clearVariable(VA_NoMergeParallel);
}


void
QTextSetupDlg::p3_keepshrt_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_KeepShortedDevs, "");
    else
        CDvdb()->clearVariable(VA_KeepShortedDevs);
}


void
QTextSetupDlg::p3_nomrgshrt_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoMergeShorted, "");
    else
        CDvdb()->clearVariable(VA_NoMergeShorted);
}


void
QTextSetupDlg::p3_nomeas_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoMeasure, "");
    else
        CDvdb()->clearVariable(VA_NoMeasure);
}


void
QTextSetupDlg::p3_usecache_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_UseMeasurePrpty, "");
    else
        CDvdb()->clearVariable(VA_UseMeasurePrpty);
}


void
QTextSetupDlg::p3_nordcache_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoReadMeasurePrpty, "");
    else
        CDvdb()->clearVariable(VA_NoReadMeasurePrpty);
}


void
QTextSetupDlg::p3_deltaset_btn_slot(int state)
{
    if (state) {
        QByteArray val_ba = es_p3_sb_delta->cleanText().toLatin1();
        const char *s = val_ba.constData();
        double d;
        if (sscanf(s, "%lf", &d) == 1 && d >= 0.01)
            CDvdb()->setVariable(VA_RLSolverDelta, s);
        else
            QTdev::Deselect(es_p3_deltaset);
    }
    else
        CDvdb()->clearVariable(VA_RLSolverDelta);
}


void
QTextSetupDlg::p3_gridsize_slot(double)
{
}


void
QTextSetupDlg::p3_trytile_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_RLSolverTryTile, "");
    else
        CDvdb()->clearVariable(VA_RLSolverTryTile);
}


void
QTextSetupDlg::p3_maxpts_slot(int)
{
    QByteArray val_ba = es_p3_sb_maxpts->cleanText().toLatin1();
    const char *s = val_ba.constData();
    int n;
    if (sscanf(s, "%d", &n) == 1 && n >= 1000 && n <= 100000) {
        if (n == RLS_DEF_MAX_GRID)
            CDvdb()->clearVariable(VA_RLSolverMaxPoints);
        else
            CDvdb()->setVariable(VA_RLSolverMaxPoints, s);
    }
}


void
QTextSetupDlg::p3_gridpts_slot(int)
{
    QByteArray val_ba = es_p3_sb_gridpts->cleanText().toLatin1();
    const char *s = val_ba.constData();
    int n;
    if (sscanf(s, "%d", &n) == 1 && n >= 10 && n <= 100000) {
        if (n == RLS_DEF_NUM_GRID)
            CDvdb()->clearVariable(VA_RLSolverGridPoints);
        else
            CDvdb()->setVariable(VA_RLSolverGridPoints, s);
    }
}



void
QTextSetupDlg::p4_flapply_btn_slot(bool state)
{
    if (state) {
        QByteArray flkeys_ba = es_p4_flkeys->text().toLatin1();
        const char *s = flkeys_ba.constData();
        CDvdb()->setVariable(VA_FlattenPrefix, s);
    }
    else
        CDvdb()->clearVariable(VA_FlattenPrefix);
}


void
QTextSetupDlg::p4_exopq_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_ExtractOpaque, "");
    else
        CDvdb()->clearVariable(VA_ExtractOpaque);
}


void
QTextSetupDlg::p4_vrbos_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_VerbosePromptline, "");
    else
        CDvdb()->clearVariable(VA_VerbosePromptline);
}


void
QTextSetupDlg::p4_glapply_btn_slot(bool state)
{
    if (state) {
        QByteArray glbex_ba = es_p4_glbex->text().toLatin1();
        const char *s = glbex_ba.constData();
        CDvdb()->setVariable(VA_GlobalExclude, s);
    }
    else
        CDvdb()->clearVariable(VA_GlobalExclude);
}


void
QTextSetupDlg::p4_noperm_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoPermute, "");
    else
        CDvdb()->clearVariable(VA_NoPermute);
}


void
QTextSetupDlg::p4_apmrg_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_MergePhysContacts, "");
    else
        CDvdb()->clearVariable(VA_MergePhysContacts);
}


void
QTextSetupDlg::p4_apfix_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_SubcPermutationFix, "");
    else
        CDvdb()->clearVariable(VA_SubcPermutationFix);
}


void
QTextSetupDlg::p4_loop_count_slot(int)
{
    QByteArray val_ba = es_p4_sb_loop->cleanText().toLatin1();
    const char *s = val_ba.constData();
    int n;
    if (sscanf(s, "%d", &n) == 1 && n >= 0 && n <= 1000000) {
        if (n == EXT_DEF_LVS_LOOP_MAX)
            CDvdb()->clearVariable(VA_MaxAssocLoops);
        else
            CDvdb()->setVariable(VA_MaxAssocLoops, s);
    }
}


void
QTextSetupDlg::p4_iter_count_slot(int)
{
    QByteArray val_ba = es_p4_sb_iters->cleanText().toLatin1();
    const char *s = val_ba.constData();
    int n;
    if (sscanf(s, "%d", &n) == 1 && n >= 10 && n <= 100000) {
        if (n == EXT_DEF_LVS_ITER_MAX)
            CDvdb()->clearVariable(VA_MaxAssocIters);
        else
            CDvdb()->setVariable(VA_MaxAssocIters, s);
    }
}


#ifdef notdef

// Update the Device Block menu.
//
void
QTextSetupDlg::dev_menu_upd()
{
    GList *gl = gtk_container_get_children(GTK_CONTAINER(es_p2_menu));
    int cnt = 0;
    for (GList *l = gl; l; l = l->next, cnt++) {
        if (cnt > 3)  // ** skip first four entries **
            gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(gl);

    stringlist *dnames = EX()->listDevices();
    for (stringlist *sl = dnames; sl; sl = sl->next) {
        const char *t = sl->string;
        char *nm = lstring::gettok(&t);
        char *pf = lstring::gettok(&t);
        sDevDesc *d = EX()->findDevice(nm, pf);
        delete [] nm;
        delete [] pf;

        if (d) {
            GtkWidget *mi = gtk_menu_item_new_with_label(sl->string);
            gtk_widget_show(mi);
            g_signal_connect(G_OBJECT(mi), "activate",
                G_CALLBACK(es_dev_menu_proc), d);
            gtk_menu_shell_append(GTK_MENU_SHELL(es_p2_menu), mi);
        }
    }
    stringlist::destroy(dnames);
}


// Static function.
// Callback from the text editor popup.
//
bool
QTextSetupDlg::es_editsave(const char *fname, void*, XEtype type)
{
    if (type == XE_QUIT)
        unlink(fname);
    else if (type == XE_SAVE) {
        FILE *fp = filestat::open_file(fname, "r");
        if (!fp) {
            Log()->ErrorLog(mh::Initialization, filestat::error_msg());
            return (true);
        }
        bool ret = EX()->parseDevice(fp, true);
        fclose(fp);
        if (ret && Es)
            Es->dev_menu_upd();
    }
    return (true);
}


// Static function.
// Edit a Device Block.
//
void
QTextSetupDlg::es_dev_menu_proc(GtkWidget *caller, void *client_data)
{
    long type = (long)g_object_get_data(G_OBJECT(caller), MIDX);
    if (type == 2) {
        // Undelete button
        EX()->addDevice(Es->es_devdesc);
        EX()->invalidateGroups();
        Es->es_devdesc = 0;
        gtk_widget_set_sensitive(Es->es_p2_undblk, false);
        Es->dev_menu_upd();
        return;
    }
    if (type == 1 && !client_data) {
        // delete button;
        return;
    }
    sDevDesc *d = (sDevDesc*)client_data;
    if (GTKdev::GetStatus(Es->es_p2_delblk)) {
        GTKdev::Deselect(Es->es_p2_delblk);
        if (d && EX()->removeDevice(Tstring(d->name()), d->prefix())) {
            d->set_next(0);
            delete Es->es_devdesc;
            Es->es_devdesc = d;
            gtk_widget_set_sensitive(Es->es_p2_undblk, true);
            Es->dev_menu_upd();
            EX()->invalidateGroups();
        }
        return;
    }
    char *fname = filestat::make_temp("xi");
    FILE *fp = filestat::open_file(fname, "w");
    if (!fp) {
        Log()->ErrorLog(mh::Initialization, filestat::error_msg());
        return;
    }
    if (d)
        d->print(fp);
    fclose(fp);
    DSPmainWbag(PopUpTextEditor(fname, es_editsave, d, false))
}

#endif
