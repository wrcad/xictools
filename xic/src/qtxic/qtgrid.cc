
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

#include "qtgrid.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "errorlog.h"
#include "tech.h"
#include "qtinterf/qtcanvas.h"

#include <QLayout>
#include <QAction>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QRadioButton>
#include <QMenu>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QGuiApplication>
#include <QDrag>
#include <QMimeData>
#include <QKeyEvent>


// The Grid Setup panel, used to control the grid in each drawing
// window.
//
// Help keyword used: xic:grid


namespace {
    enum { LstSolid, LstDots, LstStip };

    const char *edgevals[] =
    {
        "DISABLED",
        "Enabled in some commands",
        "Enabled always",
        0
    };
}

#define REVERT "revert"
#define LASTAPPL "last appl"


void
QTsubwin::PopUpGrid(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (sw_gridpop)
            sw_gridpop->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (sw_gridpop)
            sw_gridpop->update();
        return;
    }
    if (sw_gridpop)
        return;
    if (!QTmainwin::self())
        return;

    sw_gridpop = new QTgridDlg(this, sw_windesc);
    sw_gridpop->register_usrptr((void**)&sw_gridpop);
    sw_gridpop->register_caller(caller);
    sw_gridpop->initialize();
    sw_gridpop->set_visible(true);
}
// End of QTsubwin functions.


QTgridDlg *QTgridDlg::grid_pops[DSP_NUMWINS];

QTgridDlg::QTgridDlg(QTbag *owner, WindowDesc *wd) : QTdraw(XW_TEXT),
    QTbag(this)
{
    p_parent = owner;
    gd_snapbox = 0;
    gd_resol = 0;
    gd_mfglabel = 0;
    gd_snap = 0;
    gd_snapbtn = 0;
    gd_edgegrp = 0;
    gd_edge = 0;
    gd_off_grid = 0;
    gd_use_nm_edge = 0;
    gd_wire_edge = 0;
    gd_wire_path = 0;

    gd_showbtn = 0;
    gd_topbtn = 0;
    gd_noaxesbtn = 0;
    gd_plaxesbtn = 0;
    gd_oraxesbtn = 0;
    gd_cmult = 0;
    gd_nocoarse = 0;
    gd_sample = 0;
    gd_solidbtn = 0;
    gd_dotsbtn = 0;
    gd_stipbtn = 0;
    gd_crs_frame = 0;
    gd_crs = 0;
    gd_thresh = 0;

    gd_apply = 0;
    gd_cancel = 0;

    if (wd)
        gd_grid = *wd->Attrib()->grid(wd->Mode());
    gd_mask_bak = 0;
    gd_win_num = wd ? wd->WinNumber() : -1;
    gd_last_n = 0;
    gd_drag_x = 0;
    gd_drag_y = 0;
    gd_dragging = false;

    if (gd_win_num < 0) {
        // Bail out if we don't have a valid window.
//XXX
        wb_shell = 0;
        return;
    }

    grid_pops[gd_win_num] = this;

    if (owner)
        owner->MonitorAdd(this);

    setWindowTitle(tr("Grid Setup"));
    setAttribute(Qt::WA_DeleteOnClose);
//    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);

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
    QLabel *label = new QLabel(tr("Set grid parameters"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    QTabWidget *nbook = new QTabWidget();
    vbox->addWidget(nbook);

    // Snapping Page
    //
    QWidget *page = new QWidget();
    nbook->addTab(page, tr("Snapping"));

    QVBoxLayout *pvbox = new QVBoxLayout(page);
    pvbox->setContentsMargins(qmtop);
    pvbox->setSpacing(2);

    QHBoxLayout *phbox = new QHBoxLayout();
    pvbox->addLayout(phbox);
    phbox->setContentsMargins(qm);
    phbox->setSpacing(2);

    // Grid Snapping controls
    gb = new QGroupBox(tr("Snap Spacing"));
    phbox->addWidget(gb);
    QVBoxLayout *vb = new QVBoxLayout(gb);
    vb->setContentsMargins(qm);
    vb->setSpacing(2);

    gd_resol = new QDoubleSpinBox();
    vb->addWidget(gd_resol);
    gd_resol->setRange(0.0, 10000.0);
    gd_resol->setDecimals(4);
    gd_resol->setValue(1.0);
    connect(gd_resol, SIGNAL(valueChanged(double)),
        this, SLOT(resol_changed_slot(double)));

    gd_mfglabel = new QLabel();
    vb->addWidget(gd_mfglabel);

    gd_snapbox = new QGroupBox(tr("SnapPerGrid"));
    phbox->addWidget(gd_snapbox);
    vb = new QVBoxLayout(gd_snapbox);
    vb->setContentsMargins(qm);
    vb->setSpacing(2);

    gd_snap = new QSpinBox();
    vb->addWidget(gd_snap);
    gd_snap->setRange(1, 10);
    gd_snap->setValue(1);
    connect(gd_snap, SIGNAL(valueChanged(int)),
        this, SLOT(snap_changed_slot(int)));

    gd_snapbtn = new QCheckBox(tr("GridPerSnap"));
    vb->addWidget(gd_snapbtn);
    connect(gd_snapbtn, SIGNAL(stateChanged(int)),
        this, SLOT(gridpersnap_btn_slot(int)));

    // Edge Snapping group
    gd_edgegrp = new QGroupBox("Edge Snapping");
    pvbox->addWidget(gd_edgegrp);

    vb = new QVBoxLayout(gd_edgegrp);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    gd_edge = new QComboBox();
    vb->addWidget(gd_edge);
    for (int i = 0; edgevals[i]; i++) {
        gd_edge->addItem(edgevals[i], i);
    }
    connect(gd_edge, SIGNAL(currentIndexChanged(int)),
        this, SLOT(edge_menu_slot(int)));

    gd_off_grid = new QCheckBox(tr("Allow off-grid edge snapping"));
    vb->addWidget(gd_off_grid);
    connect(gd_off_grid, SIGNAL(stateChanged(int)),
        this, SLOT(off_grid_btn_slot(int)));

    gd_use_nm_edge = new QCheckBox(tr("Include non-Manhattan edges"));
    vb->addWidget(gd_use_nm_edge);
    connect(gd_use_nm_edge, SIGNAL(stateChanged(int)),
        this, SLOT(use_nm_btn_slot(int)));

    gd_wire_edge = new QCheckBox(tr("Include wire edges"));
    vb->addWidget(gd_wire_edge);
    connect(gd_wire_edge, SIGNAL(stateChanged(int)),
        this, SLOT(wire_edge_btn_slot(int)));

    gd_wire_path = new QCheckBox(tr("Include wire path"));
    vb->addWidget(gd_wire_path);
    connect(gd_wire_path, SIGNAL(stateChanged(int)),
        this, SLOT(wire_path_btn_slot(int)));

    // Style page
    //
    page = new QWidget();
    nbook->addTab(page, tr("Style"));

    pvbox = new QVBoxLayout(page);
    pvbox->setContentsMargins(qmtop);
    pvbox->setSpacing(2);

    hb = new QHBoxLayout();
    pvbox->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    // Top buttons
    gd_showbtn = new QPushButton(tr("Show"));
    hb->addWidget(gd_showbtn);
    gd_showbtn->setCheckable(true);
    gd_showbtn->setAutoDefault(false);
    connect(gd_showbtn, SIGNAL(toggled(bool)),
        this, SLOT(show_btn_slot(bool)));
    QTdev::SetStatus(gd_showbtn, gd_grid.displayed());

    gd_topbtn = new QPushButton(tr("On Top"));
    hb->addWidget(gd_topbtn);
    gd_topbtn->setCheckable(true);
    gd_topbtn->setAutoDefault(false);
    connect(gd_topbtn, SIGNAL(toggled(bool)),
        this, SLOT(top_btn_slot(bool)));
    QTdev::SetStatus(gd_topbtn, gd_grid.show_on_top());

    btn = new QPushButton(tr("Store"));
    hb->addWidget(btn);
    btn->setAutoDefault(false);
    QMenu *menu = new QMenu();
    btn->setMenu(menu);
    {
        char buf[64];
        for (int i = 1; i < TECH_NUM_GRIDS; i++) {
            snprintf(buf, sizeof(buf), "reg%d", i);
            menu->addAction(buf);
        }
        connect(menu, SIGNAL(triggered(QAction*)),
            this, SLOT(store_menu_slot(QAction*)));
    }

    btn = new QPushButton(tr("Recall"));
    hb->addWidget(btn);
    btn->setAutoDefault(false);
    menu = new QMenu();
    btn->setMenu(menu);
    {
        char buf[64];

        strcpy(buf, REVERT);
        menu->addAction(buf);
        strcpy(buf, LASTAPPL);
        menu->addAction(buf);
        for (int i = 1; i < TECH_NUM_GRIDS; i++) {
            snprintf(buf, sizeof(buf), "reg%d", i);
            menu->addAction(buf);
        }
        connect(menu, SIGNAL(triggered(QAction*)),
            this, SLOT(rcl_menu_slot(QAction*)));
    }

    // Axes and Coarse Mult
    hb = new QHBoxLayout();
    pvbox->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    QVBoxLayout *col = new QVBoxLayout();
    hb->addLayout(col);
    col->setContentsMargins(qm);

    gd_noaxesbtn = new QRadioButton(tr("No Axes"));
    col->addWidget(gd_noaxesbtn);
    connect(gd_noaxesbtn, SIGNAL(toggled(bool)),
        this, SLOT(noaxes_btn_slot(bool)));

    gd_plaxesbtn = new QRadioButton(tr("Plain Axes"));;
    col->addWidget(gd_plaxesbtn);
    connect(gd_plaxesbtn, SIGNAL(toggled(bool)),
        this, SLOT(plaxes_btn_slot(bool)));

    gd_oraxesbtn = new QRadioButton(tr("Mark Origin"));;
    col->addWidget(gd_oraxesbtn);
    connect(gd_oraxesbtn, SIGNAL(toggled(bool)),
        this, SLOT(oraxes_btn_slot(bool)));

    if (gd_grid.axes() == AxesNone) {
        gd_noaxesbtn->setChecked(true);
    }
    else if (gd_grid.axes() == AxesPlain) {
        gd_plaxesbtn->setChecked(true);
    }
    else if (gd_grid.axes() == AxesMark) {
        gd_oraxesbtn->setChecked(true);
    }

    gb = new QGroupBox(tr("Coarse Mult"));
    hb->addWidget(gb);
    col = new QVBoxLayout(gb);
    col->setContentsMargins(qm);
    col->setSpacing(2);

    gd_cmult = new QSpinBox();
    col->addWidget(gd_cmult);
    gd_cmult->setRange(1, 50);
    gd_cmult->setValue(1);
    connect(gd_cmult, SIGNAL(valueChanged(int)),
        this, SLOT(cmult_changed_slot(int)));

    // Line Style Editor
    gb = new QGroupBox(tr("Line Style Editor"));
    pvbox->addWidget(gb);
    vb = new QVBoxLayout(gb);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);
    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    gd_solidbtn = new QRadioButton(tr("Solid"));
    hb->addWidget(gd_solidbtn);
    connect(gd_solidbtn, SIGNAL(toggled(bool)),
        this, SLOT(solid_btn_slot(bool)));
    gd_dotsbtn = new QRadioButton(tr("Dots"));
    hb->addWidget(gd_dotsbtn);;
    connect(gd_dotsbtn, SIGNAL(toggled(bool)),
        this, SLOT(dots_btn_slot(bool)));

    gd_stipbtn = new QRadioButton(tr("Textured"));;
    hb->addWidget(gd_stipbtn);
    connect(gd_stipbtn, SIGNAL(toggled(bool)),
        this, SLOT(stip_btn_slot(bool)));

    if (gd_grid.linestyle().mask == 0) {
        gd_solidbtn->setChecked(true);
    }
    if (gd_grid.linestyle().mask != 0 && gd_grid.linestyle().mask != -1) {
        gd_stipbtn->setChecked(true);
    }

    QGridLayout *grd = new QGridLayout();
    vb->addLayout(grd);
    grd->setContentsMargins(qm);
    grd->setSpacing(2);
    grd->setColumnStretch(0, 1);
    grd->setColumnStretch(2, 1);

    gd_crs_frame = new QGroupBox(tr("Cross Size"));
    grd->addWidget(gd_crs_frame, 0, 1);
    QVBoxLayout *vb2 = new QVBoxLayout(gd_crs_frame);
    vb2->setContentsMargins(qmtop);
    vb2->setSpacing(2);

    // start out hidden
    gd_crs_frame->hide();

    gd_crs = new QSpinBox();
    vb2->addWidget(gd_crs);
    gd_crs->setRange(0, 6);
    gd_crs->setValue(0);
    connect(gd_crs, SIGNAL(valueChanged(int)),
        this, SLOT(cross_size_changed(int)));

    gd_sample = new QTcanvas();
    grd->addWidget(gd_sample, 1, 1);
    gd_sample->setFixedSize(QSize(200, 10));
    // The sample is a drag source of a piece of plain text which is
    // the line style mask in "0xhhhh" format.  This might be useful
    // for exporting the line style.

    connect(gd_sample, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(smp_btn_press_slot(QMouseEvent*)));
    connect(gd_sample, SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(smp_btn_release_slot(QMouseEvent*)));
    connect(gd_sample, SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(smp_motion_slot(QMouseEvent*)));

    gd_viewport = QTdrawIf::new_draw_interface(DrawNative, false, this);
    grd->addWidget(gd_viewport, 2, 1);
    gd_viewport->setFixedSize(QSize(200, 10));

    connect(gd_viewport, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(vp_btn_press_slot(QMouseEvent*)));
    connect(gd_viewport, SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(vp_btn_release_slot(QMouseEvent*)));
    connect(gd_viewport, SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(vp_resize_slot(QResizeEvent*)));

    // Visibility controls (global)
    gb = new QGroupBox(tr("All Windows"));
    vb->addWidget(gb);
    vb = new QVBoxLayout(gb);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);
    if (gd_win_num != 0)
        gb->hide();

    gd_nocoarse = new QCheckBox(tr("No coarse when fine invisible"));
    vb->addWidget(gd_nocoarse);
    connect(gd_nocoarse, SIGNAL(stateChanged(int)),
        this, SLOT(nocoarse_btn_slot(int)));

    hb = new QHBoxLayout();
    vb->addLayout(hb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    label = new QLabel(tr("Visibility Threshold"));
    hb->addWidget(label);

    gd_thresh = new QSpinBox();
    hb->addWidget(gd_thresh);
    gd_thresh->setRange(DSP_MIN_GRID_THRESHOLD, DSP_MAX_GRID_THRESHOLD);
    gd_thresh->setValue(DSP_DEF_GRID_THRESHOLD);
    connect(gd_thresh, SIGNAL(valueChanged(int)),
        this, SLOT(thresh_changed_slot(int)));

    // Apply and Dismiss buttons
    //

    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    gd_apply = new QPushButton(tr("Apply"));;
    hbox->addWidget(gd_apply);
    connect(gd_apply, SIGNAL(clicked()), this, SLOT(apply_slot()));

    gd_cancel = new QPushButton(tr("Dismiss"));
    hbox->addWidget(gd_cancel);
    connect(gd_cancel, SIGNAL(clicked()), this, SLOT(dismiss_slot()));

    update();
}


QTgridDlg::~QTgridDlg()
{
    if (gd_win_num >= 0)
        grid_pops[gd_win_num] = 0;

    if (p_parent) {
        QTsubwin *owner = dynamic_cast<QTsubwin*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        QTdev::Deselect(p_caller);
}


// GRpopup override
//
void
QTgridDlg::popdown()
{
    if (!p_parent)
        return;
    QTbag *owner = dynamic_cast<QTbag*>(p_parent);
    if (!owner || !owner->MonitorActive(this))
        return;
    deleteLater();
}


void
QTgridDlg::update(bool skip_init)
{
    WindowDesc *wd = DSP()->Window(gd_win_num);
    if (!wd)
        return;
    if (!skip_init)
        gd_grid = *wd->Attrib()->grid(wd->Mode());

    // Snapping page

    double del = GridDesc::mfg_grid(wd->Mode());
    if (del <= 0.0)
        del = wd->Mode() == Physical ? MICRONS(1) : ELEC_MICRONS(100);

    gd_resol->setMinimum(del);
    gd_resol->setSingleStep(del);
    gd_resol->setValue(gd_grid.spacing(wd->Mode()));
    gd_snap->setValue(abs(gd_grid.snap()));

    char buf[64];
    del = GridDesc::mfg_grid(wd->Mode());
    if (del > 0.0)
        snprintf(buf, sizeof(buf), "MfgGrid: %.4f", del);
    else
        strcpy(buf, "MfgGrid: unset");
    gd_mfglabel->setText(buf);
    if (gd_grid.snap() < 0) {
        QTdev::SetStatus(gd_snapbtn, true);
        gd_snapbox->setTitle("GridPerSnap");
    }
    else {
        QTdev::SetStatus(gd_snapbtn, false);
        gd_snapbox->setTitle("SnapPerGrid");
    }

    gd_edge->setCurrentIndex(wd->Attrib()->edge_snapping());
    QTdev::SetStatus(gd_off_grid, wd->Attrib()->edge_off_grid());
    QTdev::SetStatus(gd_use_nm_edge, wd->Attrib()->edge_non_manh());
    QTdev::SetStatus(gd_wire_edge, wd->Attrib()->edge_wire_edge());
    QTdev::SetStatus(gd_wire_path, wd->Attrib()->edge_wire_path());

    // Style page

    QTdev::SetStatus(gd_showbtn, gd_grid.displayed());
    QTdev::SetStatus(gd_topbtn, gd_grid.show_on_top());
    if (gd_grid.axes() == AxesNone)
        gd_noaxesbtn->setChecked(true);
    else if (gd_grid.axes() == AxesPlain)
        gd_plaxesbtn->setChecked(true);
    else if (gd_grid.axes() == AxesMark)
        gd_oraxesbtn->setChecked(true);
    if (wd->Mode() == Electrical) {
        gd_noaxesbtn->setEnabled(false);
        gd_plaxesbtn->setEnabled(false);
        gd_oraxesbtn->setEnabled(false);
        gd_nocoarse->setEnabled(false);
        gd_edgegrp->setEnabled(false);
    }
    else {
        gd_noaxesbtn->setEnabled(true);
        gd_plaxesbtn->setEnabled(true);
        gd_oraxesbtn->setEnabled(true);
        gd_nocoarse->setEnabled(true);
        gd_edgegrp->setEnabled(true);
    }

    gd_cmult->setValue(gd_grid.coarse_mult());

    QTdev::SetStatus(gd_nocoarse, DSP()->GridNoCoarseOnly());
    gd_thresh->setValue(DSP()->GridThreshold());

    gd_mask_bak = 0;
    if (gd_grid.linestyle().mask == -1)
        gd_solidbtn->setChecked(true);
    else if (gd_grid.linestyle().mask == 0) {
        gd_dotsbtn->setChecked(true);
        gd_crs->setValue(gd_grid.dotsize());
    }
    else
        gd_stipbtn->setChecked(true);

    redraw();
}


void
QTgridDlg::initialize()
{
    QTsubwin *w = dynamic_cast<QTsubwin*>(p_parent);
    if (!w)
        return;
    QTdev::self()->SetPopupLocation(GRloc(), this, w->Viewport());
}


void
QTgridDlg::redraw()
{
    if (!gd_viewport)
        return;

    if (gd_stipbtn->isChecked()) {
        gd_crs_frame->hide();
        gd_sample->show();
        gd_viewport->show();

        SetWindowBackground(QTdev::self()->NameColor("white"));
        Clear();
        SetFillpattern(0);
        SetColor(QTdev::self()->NameColor("blue"));
        int wid = gd_viewport->width();
        int hei = gd_viewport->height();
        int w = wid/32 - 1;
        if (w < 2)
            w = 2;
        int tw = 32*w;
        int os = 0;
        if (tw < wid)
            os = (wid - tw)/2;
        unsigned msk = ~((~(unsigned)0) >> 1);
        int xx = os;
        int xs = -1;
        for (int i = 0; i < 32; i++) {
            if (msk & gd_grid.linestyle().mask) {
                Box(xx, 1, xx+w, hei-2);
                if (xs < 0)
                    xs = xx;
            }
            xx += w;
            msk >>= 1;
        }
        if (xs < 0)
            xs = tw + os;
        QPalette pal = QGuiApplication::palette();
        QColor clr = pal.color(QPalette::Inactive, QPalette::Window);
        if (os) {
            SetColor(clr.rgb());
            Box(0, 0, os, hei);
            Box(os + tw, 0, wid, hei);
        }
        clr = pal.color(QPalette::Normal, QPalette::Window);
//XXX        SetColor(style->bg[GTK_STATE_ACTIVE].pixel);
SetColor(clr.rgb());
        Box(os, 0, xs, hei);
Update();

        QTdrawIf *tmp = gd_viewport;
        gd_viewport = gd_sample;
        SetWindowBackground(QTdev::self()->NameColor("black"));
        Clear();
        if (gd_grid.linestyle().mask) {
            // DefineLinestyle will convert 1,3,7,... to -1.
            unsigned ltmp = gd_grid.linestyle().mask;
            defineLinestyle(&gd_grid.linestyle(), ltmp);
            gd_grid.linestyle().mask = ltmp;
            SetColor(QTdev::self()->NameColor("white"));
            SetFillpattern(0);
            Line(os+2, 5, os+tw-2, 5);
        }
        if (os) {
            SetColor(clr.rgb());
            Box(0, 0, os, hei);
            Box(os + tw, 0, wid, hei);
        }
        gd_viewport = tmp;
        gd_sample->repaint(0, 0, gd_sample->width(), gd_sample->height());
    }
    else if (gd_dotsbtn->isChecked()) {
        gd_sample->hide();
        gd_viewport->hide();
        gd_crs_frame->show();
    }
    else if (gd_solidbtn->isChecked()) {
        gd_crs_frame->hide();
        gd_sample->show();
        gd_viewport->show();

        QPalette pal = QGuiApplication::palette();
        QColor clr = pal.color(QPalette::Inactive, QPalette::Window);
        SetWindowBackground(clr.rgb());
        Clear();
        Update();

        QTdrawIf *tmp = gd_viewport;
        gd_viewport = gd_sample;
        SetWindowBackground(clr.rgb());
        Clear();
        gd_viewport = tmp;
        gd_sample->repaint(0, 0, gd_sample->width(), gd_sample->height());
    }
}


void
QTgridDlg::keyPressEvent(QKeyEvent *ev)
{
    // Look only at Enter key events.  If not focussed on the Dismiss
    // button, run the Apply callback and set focus to the Dismiss
    // button.  A second Enter press will then dismiss the pop-up.

    if (ev->type() == QEvent::KeyPress && ev->key() == Qt::Key_Return) {
        QWidget *w = focusWidget();
        if (w != gd_cancel) {
            apply_slot();
            gd_cancel->setFocus();
            return;
        }
    }
    QDialog::keyPressEvent(ev);
}


void
QTgridDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:grid"));
}


void
QTgridDlg::resol_changed_slot(double val)
{
    WindowDesc *wd = DSP()->Window(gd_win_num);
    if (!wd)
        return;

    gd_grid.set_spacing(val);

    // Reset the value, as it might have been reset to the mfg grid.
    gd_resol->setValue(gd_grid.spacing(wd->Mode()));
}


void
QTgridDlg::snap_changed_slot(int n)
{
    bool neg = QTdev::GetStatus(gd_snapbtn);
    gd_grid.set_snap(neg ? -n : n);
}


void
QTgridDlg::gridpersnap_btn_slot(int state)
{
    if (state) {
        gd_snapbox->setTitle("GridPerSnap");
        int sn = gd_snap->value();
        gd_grid.set_snap(-sn);
    }
    else {
        gd_snapbox->setTitle("SnapPerGrid");
        int sn = gd_snap->value();
        gd_grid.set_snap(sn);
    }
}


void
QTgridDlg::edge_menu_slot(int i)
{
    WindowDesc *wd = DSP()->Window(gd_win_num);
    if (!wd)
        return;
    switch (i) {
    case EdgeSnapNone:
        wd->Attrib()->set_edge_snapping(EdgeSnapNone);
        break;
    case EdgeSnapSome:
        wd->Attrib()->set_edge_snapping(EdgeSnapSome);
        break;
    case EdgeSnapAll:
        wd->Attrib()->set_edge_snapping(EdgeSnapAll);
        break;
    }
}


void
QTgridDlg::off_grid_btn_slot(int state)
{
    WindowDesc *wd = DSP()->Window(gd_win_num);
    if (!wd)
        return;
    wd->Attrib()->set_edge_off_grid(state);
}


void
QTgridDlg::use_nm_btn_slot(int state)
{
    WindowDesc *wd = DSP()->Window(gd_win_num);
    if (!wd)
        return;
    wd->Attrib()->set_edge_non_manh(state);
}


void
QTgridDlg::wire_edge_btn_slot(int state)
{
    WindowDesc *wd = DSP()->Window(gd_win_num);
    if (!wd)
        return;
    wd->Attrib()->set_edge_wire_edge(state);
}


void
QTgridDlg::wire_path_btn_slot(int state)
{
    WindowDesc *wd = DSP()->Window(gd_win_num);
    if (!wd)
        return;
    wd->Attrib()->set_edge_wire_path(state);
}


void
QTgridDlg::show_btn_slot(bool state)
{
    gd_grid.set_displayed(state);
}


void
QTgridDlg::top_btn_slot(bool state)
{
    gd_grid.set_show_on_top(state);
}


void
QTgridDlg::store_menu_slot(QAction *a)
{
    WindowDesc *wd = DSP()->Window(gd_win_num);
    if (!wd)
        return;

    QByteArray ba = a->text().toLatin1();
    const char *string = ba.constData();
    DisplayMode mode = wd->Mode();
    if (string) {
        while (isalpha(*string))
            string++;
        int indx = atoi(string);
        if (indx >= 0 && indx < TECH_NUM_GRIDS)
            Tech()->SetGridReg(indx, gd_grid, mode);
    }
}


void
QTgridDlg::rcl_menu_slot(QAction *a)
{
    WindowDesc *wd = DSP()->Window(gd_win_num);
    if (!wd)
        return;

    QByteArray ba = a->text().toLatin1();
    const char *string = ba.constData();
    if (string) {
        if (!strcmp(string, REVERT)) {
            // Revert to the current Attributes grid.
            update();
            return;
        }
        if (!strcmp(string, LASTAPPL)) {
            // Return to the last grid applied (reg 0);
            GridDesc gref;
            DisplayMode mode = wd->Mode();
            GridDesc *g = Tech()->GridReg(0, mode);
            if (*g != gref) {
                gd_grid.set(*g);
                update(true);
            }
            return;
        }
        while (isalpha(*string))
            string++;
        int indx = atoi(string);
        if (indx >= 0 && indx < TECH_NUM_GRIDS) {
            GridDesc gref;
            DisplayMode mode = wd->Mode();
            GridDesc *g = Tech()->GridReg(indx, mode);
            if (*g != gref) {
                gd_grid.set(*g);
                update(true);
            }
        }
    }
}


void
QTgridDlg::noaxes_btn_slot(bool state)
{
    if (!state)
        return;
    WindowDesc *wd = DSP()->Window(gd_win_num);
    if (!wd || (wd->Mode() != Physical))
        return;
    gd_grid.set_axes(AxesNone);
}


void
QTgridDlg::plaxes_btn_slot(bool state)
{
    if (!state)
        return;
    WindowDesc *wd = DSP()->Window(gd_win_num);
    if (!wd || (wd->Mode() != Physical))
        return;
    gd_grid.set_axes(AxesPlain);
}


void
QTgridDlg::oraxes_btn_slot(bool state)
{
    if (!state)
        return;
    WindowDesc *wd = DSP()->Window(gd_win_num);
    if (!wd || (wd->Mode() != Physical))
        return;
    gd_grid.set_axes(AxesMark);
}


void
QTgridDlg::cmult_changed_slot(int i)
{
    gd_grid.set_coarse_mult(i);
}


void
QTgridDlg::solid_btn_slot(bool state)
{
    if (!state)
        return;
    if (!gd_mask_bak && gd_grid.linestyle().mask != 0 &&
            gd_grid.linestyle().mask != -1)
        gd_mask_bak = gd_grid.linestyle().mask;
    gd_grid.linestyle().mask = -1;
    redraw();
}


void
QTgridDlg::dots_btn_slot(bool state)
{
    if (!state)
        return;
    if (!gd_mask_bak && gd_grid.linestyle().mask != 0 &&
            gd_grid.linestyle().mask != -1)
        gd_mask_bak = gd_grid.linestyle().mask;
    gd_grid.linestyle().mask = 0;
    redraw();
}


void
QTgridDlg::stip_btn_slot(bool state)
{
    if (!state)
        return;
    if (gd_mask_bak) {
        gd_grid.linestyle().mask = gd_mask_bak;
        gd_mask_bak = 0;
    }
    redraw();
}


void
QTgridDlg::cross_size_changed(int i)
{
    gd_grid.set_dotsize(i);
}


void
QTgridDlg::nocoarse_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_GridNoCoarseOnly, "");
    else
        CDvdb()->clearVariable(VA_GridNoCoarseOnly);
}


void
QTgridDlg::thresh_changed_slot(int n)
{
    char buf[32];
    if (n < DSP_MIN_GRID_THRESHOLD || n > DSP_MAX_GRID_THRESHOLD)
        return;
    snprintf(buf, sizeof(buf), "%d", n);
    if (n != DSP_DEF_GRID_THRESHOLD)
        CDvdb()->setVariable(VA_GridThreshold, buf);
    else
        CDvdb()->clearVariable(VA_GridThreshold);
}


void
QTgridDlg::apply_slot()
{
    WindowDesc *wd = DSP()->Window(gd_win_num);
    if (!wd)
        return;

    DisplayMode mode = wd->Mode();
    if (mode == Electrical) {
        double spa = gd_grid.spacing(mode);
        if (10*ELEC_INTERNAL_UNITS(spa) % CDelecResolution) {
            // "can't happen"
            Log()->ErrorLog(mh::Initialization,
                "Error: electrical mode snap points must be "
                "0.1 micron multiples.");
            return;
        }
        if (ELEC_INTERNAL_UNITS(spa) % CDelecResolution) {
            Log()->WarningLog(mh::Initialization,
                "Sub-micron snap points are NOT RECOMMENDED in\n"
                "electrical mode.  Connection points should be on\n"
                "a one micron grid.  The present grid is accepted\n"
                "to allow repair of old files.");
        }
    }
    DSPattrib *a = wd->Attrib();
    GridDesc lastgrid = *a->grid(mode);
    Tech()->SetGridReg(0, lastgrid, mode);
    a->grid(mode)->set(gd_grid);
    bool axes_changed =
        (mode == Physical && a->grid(mode)->axes() != lastgrid.axes());
    if (lastgrid.visually_differ(a->grid(mode)))
        wd->Redisplay(0);
    else if (axes_changed) {
        if (lastgrid.axes() != AxesNone)
            wd->ShowAxes(ERASE);
        wd->ShowAxes(DISPLAY);
    }
    XM()->ShowParameters();
}


void
QTgridDlg::dismiss_slot()
{
    popdown();
}


void
QTgridDlg::smp_btn_press_slot(QMouseEvent *ev)
{
    if (ev->button() != Qt::LeftButton)
        return;
    gd_dragging = true;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    gd_drag_x = ev->position().x();
    gd_drag_y = ev->position().y();
#else
    gd_drag_x = ev->x();
    gd_drag_y = ev->y();
#endif
}


void
QTgridDlg::smp_btn_release_slot(QMouseEvent*)
{
    gd_dragging = false;
    gd_drag_x = 0;
    gd_drag_y = 0;
}


void
QTgridDlg::smp_motion_slot(QMouseEvent *ev)
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    if (gd_dragging && (abs(xx - gd_drag_x) > 2 || abs(yy - gd_drag_y) > 2)) {
        gd_dragging = false;
        char buf[64];
        snprintf(buf, sizeof(buf), "0x%x", gd_grid.linestyle().mask);
        QDrag *drag = new QDrag(gd_sample);
        QMimeData *mimedata = new QMimeData();
        QByteArray qdata(buf, strlen(buf)+1);
        mimedata->setData("text/plain", qdata);
        drag->setMimeData(mimedata);
        drag->exec(Qt::CopyAction);
    }
}


void
QTgridDlg::vp_btn_press_slot(QMouseEvent *ev)
{
    if (gd_stipbtn->isChecked()) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        int xx = ev->position().x();
#else
        int xx = ev->x();
#endif
        int wid = gd_viewport->width();
        int w = wid/32 - 1;
        if (w < 2)
            w = 2;
        int tw = 32*w;
        int os = 0;
        if (tw < wid)
            os = (wid - tw)/2;
        int n = (xx - os)/w;
        if (n < 0 || n > 31)
            return;
        n = 31 - n;
        if (ev->button() == Qt::LeftButton)
            gd_grid.linestyle().mask ^= (1 << n);
        else if (ev->button() == Qt::MiddleButton)
            gd_grid.linestyle().mask &= ~(1 << n);
        else if (ev->button() == Qt::RightButton)
            gd_grid.linestyle().mask |= (1 << n);
        else
            return;
        gd_last_n = n;
        redraw();
    }
}


void
QTgridDlg::vp_btn_release_slot(QMouseEvent *ev)
{
    if (gd_stipbtn->isChecked()) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        int xx = ev->position().x();
#else
        int xx = ev->x();
#endif
        int wid = gd_viewport->width();
        int w = wid/32 - 1;
        if (w < 2)
            w = 2;
        int tw = 32*w;
        int os = 0;
        if (tw < wid)
            os = (wid - tw)/2;
        int n = (xx - os)/w;
        if (n < 0 || n > 31)
            return;
        n = 31 - n;
        if (n > gd_last_n) {
            for (int i = gd_last_n + 1; i <= n; i++) {
                if (ev->button() == Qt::LeftButton)
                    gd_grid.linestyle().mask ^= (1 << i);
                else if (ev->button() == Qt::MiddleButton)
                    gd_grid.linestyle().mask &= ~(1 << i);
                else if (ev->button() == Qt::RightButton)
                    gd_grid.linestyle().mask |= (1 << i);
                else
                    return;
            }
            redraw();
        }
        else if (n < gd_last_n) {
            for (int i = gd_last_n - 1; i >= n; i--) {
                if (ev->button() == Qt::LeftButton)
                    gd_grid.linestyle().mask ^= (1 << i);
                else if (ev->button() == Qt::MiddleButton)
                    gd_grid.linestyle().mask &= ~(1 << i);
                else if (ev->button() == Qt::RightButton)
                    gd_grid.linestyle().mask |= (1 << i);
                else
                    return;
            }
            redraw();
        }
    }
}


void
QTgridDlg::vp_resize_slot(QResizeEvent*)
{
    redraw();
}

