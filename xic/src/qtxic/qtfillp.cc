
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

#include "qtfillp.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "layertab.h"
#include "menu.h"
#include "errorlog.h"
#include "keymap.h"
#include "tech.h"
#include "qtinterf/qtcanvas.h"
#include "qtltab.h"
#include <iostream>

#include <QApplication>
#include <QLayout>
#include <QSpinBox>
#include <QGroupBox>
#include <QToolButton>
#include <QPushButton>
#include <QToolButton>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>


//-----------------------------------------------------------------------------
// QTfillPatDlg:  Fill pattern editing dialog.
// Called from main menu: Attributes/Set Fill.
//
// Help system keywords used:
//  fillpanel

// Menu callback for fill editor popup.
//
void
cMain::PopUpFillEditor(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTfillPatDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTfillPatDlg::self())
            QTfillPatDlg::self()->update();
        return;
    }
    if (QTfillPatDlg::self())
        return;

    if (!XM()->CheckCurLayer()) {
        QTdev::Deselect(caller);
        return;
    }

    new QTfillPatDlg(caller);

    QTfillPatDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_LR), QTfillPatDlg::self(),
        QTmainwin::self()->Viewport());
    QTfillPatDlg::self()->show();
}


// Callback used for drag/drop to layer table.
//
void
cMain::FillLoadCallback(LayerFillData *dd, CDl *ld)
{
    if (QTfillPatDlg::self())
        QTfillPatDlg::self()->drag_load(dd, ld);
}
// End of cMain functions.


QTfillPatDlg *QTfillPatDlg::instPtr;

QTfillPatDlg::QTfillPatDlg(GRobject c) : QTbag(this), QTdraw(XW_DRAWING)
{
    instPtr = this;
    fp_caller = c;
    fp_outl = 0;
    fp_fat = 0;
    fp_cut = 0;
    fp_editor = 0;
    fp_sample = 0;
    fp_editframe = 0;
    fp_editctrl = 0;
    fp_stoframe = 0;
    fp_stoctrl = 0;
    memset(fp_stores, 0, sizeof(fp_stores));
    fp_spnx = 0;
    fp_spny = 0;
    fp_defpats = 0;

    fp_fp = 0;
    fp_pattern_bank = 0;
    fp_width = fp_height = 0;
    fp_foreg = fp_pixbg = 0;
    memset(fp_array, 0, sizeof(fp_array));
    fp_nx = 8;
    fp_ny = 8;
    fp_margin = 0;
    fp_def_box_w = fp_def_box_h = 0;
    fp_pat_box_h = 0;
    fp_edt_box_dim = 0;
    fp_spa = 0;
    fp_epsz = 0;
    fp_ii = fp_jj = 0;
    fp_drag_btn = 0;
    fp_drag_x = fp_drag_y = 0;
    fp_pm_w = fp_pm_h = 0;
    fp_downbtn = 0;
    fp_dragging = false;
    fp_editing = false;

    setWindowTitle(tr("Fill Pattern Editor"));
    setAttribute(Qt::WA_DeleteOnClose);

    fp_width = 466;
    fp_height = 264;
    fp_spa = fp_height/36;
    fp_margin = 2*fp_spa;
    fp_def_box_h = 11*fp_spa - 2;
    fp_def_box_w = 5*fp_spa + 1;
    fp_edt_box_dim = 34*fp_spa;
    fp_pat_box_h = 12*fp_spa;
    fp_foreg = dsp_prm(LT()->CurLayer())->pixel();
    fp_pixbg = DSP()->Color(BackgroundColor);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;

    // Mainrow + button row
    //
    QVBoxLayout *top_vbox = new QVBoxLayout(this);
    top_vbox->setContentsMargins(qm);
    top_vbox->setSpacing(2);
    top_vbox->setSizeConstraint(QLayout::SetFixedSize);

    // Leftcol + pixel editor + stores
    //
    QHBoxLayout *mainrow = new QHBoxLayout();
    mainrow->setContentsMargins(qmtop);
    mainrow->setSpacing(2);
    top_vbox->addLayout(mainrow);

    QGroupBox *bb = new QGroupBox();
    mainrow->addWidget(bb);

    // Pixel editor controls + stores controls + sample area
    //
    QVBoxLayout *leftcol = new QVBoxLayout();
    leftcol->setContentsMargins(qmtop);
    leftcol->setSpacing(2);
    bb->setLayout(leftcol);

    // Pixel editor controls
    //
    fp_editctrl = new QWidget();
    leftcol->addWidget(fp_editctrl);

    QVBoxLayout *vbox = new QVBoxLayout();
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);
    fp_editctrl->setLayout(vbox);

    QGroupBox *gb = new QGroupBox(tr("NX x NY"));
    QHBoxLayout *hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qmtop);
    vbox->addWidget(gb);

    fp_spnx = new QSpinBox();
    hbox->addWidget(fp_spnx);
    fp_spnx->setValue(8);
    fp_spnx->setMinimum(2);
    fp_spnx->setMaximum(32);
    connect(fp_spnx, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &QTfillPatDlg::nx_change_slot);
    fp_spny = new QSpinBox();
    fp_spny->setMinimum(2);
    fp_spny->setMaximum(32);
    hbox->addWidget(fp_spny);
    fp_spny->setValue(8);
    connect(fp_spny, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &QTfillPatDlg::ny_change_slot);

    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qmtop);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Rot90"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTfillPatDlg::rot90_btn_slot);

    tbtn = new QToolButton();
    tbtn->setText("X");
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTfillPatDlg::x_btn_slot);
    tbtn = new QToolButton();
    tbtn->setText("Y");
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTfillPatDlg::y_btn_slot);

    tbtn = new QToolButton();
    tbtn->setText(tr("Stores"));
    vbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTfillPatDlg::stores_btn_slot);

    // Stores display controls
    //
    fp_stoctrl = new QWidget();
    leftcol->addWidget(fp_stoctrl);

    vbox = new QVBoxLayout;
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);
    fp_stoctrl->setLayout(vbox);

    gb = new QGroupBox(tr("Page"));
    vbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    fp_defpats = new QSpinBox();
    fp_defpats->setValue(1);
    fp_defpats->setMinimum(1);
    fp_defpats->setMaximum(4);
    fp_defpats->setWrapping(true);
    hb->addWidget(fp_defpats);
    connect(fp_defpats, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &QTfillPatDlg::defpats_change_slot);

    tbtn = new QToolButton();
    tbtn->setText(tr("Dump Defs"));
    vbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTfillPatDlg::dump_btn_slot);

    tbtn = new QToolButton();
    tbtn->setText(tr("Pixel Editor"));
    vbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTfillPatDlg::pixed_btn_slot);

    // Sample area
    //
    gb = new QGroupBox(tr("Sample"));
    leftcol->addWidget(gb);
    hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);

    fp_sample = new QTcanvas();
    hb->addWidget(fp_sample);
    connect_sigs(fp_sample, true);

    // Pixel editor
    //
    fp_editframe = new QGroupBox(tr("Pixel Editor"));
    mainrow->addWidget(fp_editframe);
    hb = new QHBoxLayout(fp_editframe);
    hb->setContentsMargins(qmtop);

    fp_editor = new QTcanvas();
    fp_editor->setMinimumWidth(fp_edt_box_dim);
    fp_editor->setMaximumWidth(fp_edt_box_dim);
    fp_editor->setMinimumHeight(fp_edt_box_dim);
    fp_editor->setMaximumHeight(fp_edt_box_dim);
    hb->addWidget(fp_editor);
    connect_sigs(fp_editor, true);
    fp_editor->setFocusPolicy(Qt::StrongFocus);
    connect(fp_editor, &QTcanvas::enter_event,
        this, &QTfillPatDlg::enter_slot);

    // Patterns
    //
    fp_stoframe = new QGroupBox(tr("Default Patterns - Pattern Storage"));
    mainrow->addWidget(fp_stoframe);

    vbox = new QVBoxLayout();
    vbox->setContentsMargins(qm);
    vbox->setSpacing(0);
    fp_stoframe->setLayout(vbox);
    for (int i = 0; i < 3; i++) {
        hbox = new QHBoxLayout();
        hbox->setContentsMargins(qm);
        hbox->setSpacing(0);
        vbox->addLayout(hbox);
        for (int j = 0; j < 6; j++) {
            QGroupBox *iframe = new QGroupBox();
            QTcanvas *darea = new QTcanvas();
            int k = i + j*3;
            fp_stores[k] = darea;
            darea->setMinimumWidth(fp_def_box_w);
            darea->setMaximumWidth(fp_def_box_w);
            darea->setMinimumHeight(fp_def_box_h);
            darea->setMaximumHeight(fp_def_box_h);

            hb = new QHBoxLayout(iframe);
            hb->setContentsMargins(qm);
            hb->addWidget(darea);
            connect_sigs(darea, (k > 1));
            hbox->addWidget(iframe);
        }
    }

    // Button line
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qmtop);
    hbox->setSpacing(2);
    top_vbox->addLayout(hbox);

    fp_outl = new QToolButton();
    fp_outl->setText(tr("Outline"));
    hbox->addWidget(fp_outl);
    fp_outl->setCheckable(true);
    connect(fp_outl, &QAbstractButton::toggled,
        this, &QTfillPatDlg::outline_btn_slot);

    fp_fat = new QToolButton();
    fp_fat->setText(tr("Fat"));
    hbox->addWidget(fp_fat);
    fp_fat->setCheckable(true);
    connect(fp_fat, &QAbstractButton::toggled,
        this, &QTfillPatDlg::fat_btn_slot);

    fp_cut = new QToolButton();
    fp_cut->setText(tr("Cut"));
    hbox->addWidget(fp_cut);
    fp_cut->setCheckable(true);
    connect(fp_cut, &QAbstractButton::toggled,
        this, &QTfillPatDlg::cut_btn_slot);

    tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTfillPatDlg::help_btn_slot);

    tbtn = new QToolButton();
    tbtn->setText(tr("Load"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTfillPatDlg::load_btn_slot);

    tbtn = new QToolButton();
    tbtn->setText(tr("Apply"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTfillPatDlg::apply_btn_slot);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    hbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QTfillPatDlg::dismiss_btn_slot);

    DspLayerParams *lp = dsp_prm(LT()->CurLayer());
    bool solid = LT()->CurLayer()->isFilled() && !lp->fill()->hasMap();
    if (solid) {
        fp_nx = 8;
        fp_ny = 8;
        memset(fp_array, 0xff, 8);
    }
    else {
        if (!LT()->CurLayer()->isFilled()) {
            fp_nx = 8;
            fp_ny = 8;
            memset(fp_array, 0, 8);
        }
        else {
            fp_nx = lp->fill()->nX();
            fp_ny = lp->fill()->nY();
            unsigned char *map = lp->fill()->newBitmap();
            memcpy(fp_array, map, fp_ny*((fp_nx + 7)/8));
            delete [] map;
        }
        if (LT()->CurLayer()->isOutlined()) {
            QTdev::SetStatus(fp_outl, true);
            if (LT()->CurLayer()->isOutlinedFat())
                QTdev::SetStatus(fp_fat, true);
        }
        else
            fp_fat->setEnabled(false);
        QTdev::SetStatus(fp_cut, LT()->CurLayer()->isCut());
    }
    fp_mode_proc(false);
    // The call to update() needs to be delayed or the sample and
    // store patterns don't appear. Putting the call in an idle
    // proc fixes this.
    QTpkg::self()->RegisterIdleProc(fp_update_idle_proc, (void*)0);
}


QTfillPatDlg::~QTfillPatDlg()
{
    instPtr = 0;
    if (fp_caller)
        QTdev::Deselect(fp_caller);
    if (fp_fp) {
        SetFillpattern(0);          // Remove pointer to saved pixmap.
        fp_fp->newMap(0, 0, 0);     // Clear pixel map.
        DefineFillpattern(fp_fp);   // Destroy pixmap.
        delete fp_fp;
    }
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTfillPatDlg
#include "qtinterf/qtmacos_event.h"
#endif


void
QTfillPatDlg::update()
{
    if (!LT()->CurLayer())
        return;
    fp_foreg = dsp_prm(LT()->CurLayer())->pixel();
    fp_pixbg = DSP()->Color(BackgroundColor);
    for (int i = 0; i < 18; i++)
        redraw_store(i);
    redraw_edit();
    redraw_sample();
}


void
QTfillPatDlg::drag_load(LayerFillData *dd, CDl *ld)
{
    if (dd->d_from_layer)
        return;
    pattern_to_layer(dd, ld);
}


void
QTfillPatDlg::nx_change_slot(int nx)
{
    // Reconfigure the pixel map so that the pattern doesn't
    // turn to crap when the bpl changes.

    int oldx = fp_nx;
    fp_nx = nx;
    int oldbpl = (oldx + 7)/8;
    int newbpl = (fp_nx + 7)/8;
    if (oldbpl != newbpl) {
        unsigned char ary[128];
        memcpy(ary, fp_array, fp_ny*oldbpl);
        unsigned char *t = fp_array;
        unsigned char *f = ary;
        for (int i = 0; i < fp_ny; i++) {
            if (oldbpl < newbpl) {
                for (int j = 0; j < newbpl; j++) {
                    if (j < oldbpl)
                        *t++ = *f++;
                    else
                        *t++ = 0;
                }
            }
            else {
                for (int j = 0; j < oldbpl; j++) {
                    if (j < newbpl)
                        *t++ = *f++;
                    else
                        f++;
                }
            }
        }
    }
    set_fp(fp_array, fp_nx, fp_ny);
    SetFillpattern(0);
    redraw_edit();
    redraw_sample();
}


void
QTfillPatDlg::ny_change_slot(int ny)
{
    fp_ny = ny;
    set_fp(fp_array, fp_nx, fp_ny);
    SetFillpattern(0);
    redraw_edit();
    redraw_sample();
}


namespace {
    void setpix(int x, int nx, int y, unsigned char *ary)
    {
        int bpl = (nx + 7)/8;
        unsigned char *a = ary + y*bpl;
        unsigned int d = *a++;
        for (int j = 1; j < bpl; j++)
            d |= *a++ << j*8;
        unsigned int mask = 1 << x;
        d |= mask;
        a = ary + y*bpl;
        *a++ = d;
        for (int j = 1; j < bpl; j++)
            *a++ = d >> (j*8);
    }
}


void
QTfillPatDlg::rot90_btn_slot()
{
    int nx = fp_nx;
    int ny = fp_ny;
    int bpl = (nx + 7)/8;

    unsigned char ary[128];
    memset(ary, 0, 128*sizeof(unsigned char));
    for (int i = 0; i < ny; i++) {
        unsigned char *a = fp_array + i*bpl;
        unsigned int d = *a++;
        for (int j = 1; j < bpl; j++)
            d |= *a++ << j*8;
        unsigned int msk = 1;
        for (int j = 0; j < fp_nx; j++) {
            bool lit = msk & d;
            msk <<= 1;
            if (lit)
                setpix(i, ny, nx - j - 1, ary);
        }
    }
    fp_spnx->setValue(ny);
    fp_spny->setValue(nx);
    fp_nx = ny;
    fp_ny = nx;
    memcpy(fp_array, ary, 128);
    set_fp(ary, ny, nx);
    SetFillpattern(0);
    redraw_edit();
    redraw_sample();
}


void
QTfillPatDlg::x_btn_slot()
{
    int nx = fp_nx;
    int ny = fp_ny;
    int bpl = (nx + 7)/8;

    bool flipy = false;

    unsigned char ary[128];
    memset(ary, 0, 128*sizeof(unsigned char));
    for (int i = 0; i < ny; i++) {
        unsigned char *a = fp_array + i*bpl;
        unsigned int d = *a++;
        for (int j = 1; j < bpl; j++)
            d |= *a++ << j*8;
        unsigned int msk = 1;
        for (int j = 0; j < fp_nx; j++) {
            bool lit = msk & d;
            msk <<= 1;
            if (lit) {
                if (flipy)
                    setpix(j, nx, ny - i - 1, ary);
                else
                    setpix(nx - j - 1, nx, i, ary);
            }
        }
    }
    memcpy(fp_array, ary, 128);
    set_fp(ary, nx, ny);
    SetFillpattern(0);
    redraw_edit();
    redraw_sample();
}


void
QTfillPatDlg::y_btn_slot()
{
    int nx = fp_nx;
    int ny = fp_ny;
    int bpl = (nx + 7)/8;

    bool flipy = true;

    unsigned char ary[128];
    memset(ary, 0, 128*sizeof(unsigned char));
    for (int i = 0; i < ny; i++) {
        unsigned char *a = fp_array + i*bpl;
        unsigned int d = *a++;
        for (int j = 1; j < bpl; j++)
            d |= *a++ << j*8;
        unsigned int msk = 1;
        for (int j = 0; j < fp_nx; j++) {
            bool lit = msk & d;
            msk <<= 1;
            if (lit) {
                if (flipy)
                    setpix(j, nx, ny - i - 1, ary);
                else
                    setpix(nx - j - 1, nx, i, ary);
            }
        }
    }
    memcpy(fp_array, ary, 128);
    set_fp(ary, nx, ny);
    SetFillpattern(0);
    redraw_edit();
    redraw_sample();
}


void
QTfillPatDlg::stores_btn_slot()
{
    fp_mode_proc(false);
}


void
QTfillPatDlg::defpats_change_slot(int bank)
{
    if (bank > 0 && bank <= TECH_MAP_SIZE/16) {
        fp_pattern_bank = bank - 1;
        for (int i = 0; i < 18; i++)
            redraw_store(i);
    }
}


void
QTfillPatDlg::dump_btn_slot()
{
    const char *err = Tech()->DumpDefaultStipples();
    if (err)
        PopUpMessage(err, true);
    else {
        PopUpMessage("Created xic_stipples file in current directory.",
            false);
    }
}


void
QTfillPatDlg::pixed_btn_slot()
{
    fp_mode_proc(true);
}


void
QTfillPatDlg::load_btn_slot()
{
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return;
    LayerFillData dd(ld);
    layer_to_def_or_sample(&dd, -1);
}


void
QTfillPatDlg::apply_btn_slot()
{
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return;
    LayerFillData dd;
    dd.d_from_sample = true;
    dd.d_nx = fp_nx;
    dd.d_ny = fp_ny;
    memcpy(dd.d_data, fp_array, dd.d_ny*((dd.d_nx + 7)/8));
    pattern_to_layer(&dd, ld);
}


void
QTfillPatDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("fillpanel"))
}


void
QTfillPatDlg::outline_btn_slot(bool state)
{
    // Callback for the outline button.  When selected, new patterns
    // will have the OUTLINED attribute set.

    if (state) {
        int sz = fp_ny*((fp_nx + 7)/8);
        for (int i = 0; i < sz; i++) {
            if (fp_array[i]) {
                state = false;
                break;
            }
        }
    }
    fp_fat->setEnabled(state);
    redraw_sample();
}


void
QTfillPatDlg::fat_btn_slot(bool)
{
    redraw_sample();
}


void
QTfillPatDlg::cut_btn_slot(bool)
{
    redraw_sample();
}


void
QTfillPatDlg::dismiss_btn_slot()
{
    XM()->PopUpFillEditor(0, MODE_OFF);
}


void
QTfillPatDlg::button_down_slot(QMouseEvent *ev)
{
    // Button press, handles the pixel editor and drag/drop detection.

    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();
    int button = 0;
    if (ev->button() == Qt::LeftButton)
        button = 1;
    else if (ev->button() == Qt::MiddleButton)
        button = 2;
    else if (ev->button() == Qt::RightButton)
        button = 3;
    button = Kmap()->ButtonMap(button);

    if (sender() == fp_editor) {
        // pixel editor
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        fp_jj = ev->position().x();
        fp_ii = ev->position().y();
#else
        fp_jj = ev->x();
        fp_ii = ev->y();
#endif
        fp_downbtn = 0;
        if (getij(&fp_jj, &fp_ii)) {
            fp_downbtn = button;
            if (fp_downbtn == 1 && (ev->modifiers() &
                    (Qt::ShiftModifier | Qt::ControlModifier)))
                fp_downbtn = 2;
            int refx = fp_spa + fp_jj*fp_epsz + fp_epsz/2;
            int refy = fp_spa + fp_ii*fp_epsz + fp_epsz/2;
            gd_viewport = fp_editor;
            SetGhost(&fp_drawghost, refx, refy);
        }
        return;
    }

    fp_dragging = true;
    fp_drag_btn = button;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    fp_drag_x = ev->position().x();
    fp_drag_y = ev->position().y();
#else
    fp_drag_x = ev->x();
    fp_drag_y = ev->y();
#endif
}


void
QTfillPatDlg::button_up_slot(QMouseEvent *ev)
{
    // Button release.  The pixel editor has several modes, depending
    // on the button used, whether it is clicked or held and dragged,
    // and whether the shift key is down.  If the buttons are clicked,
    // the target pixel is acted on.  If held and dragged, a region of
    // pixels is acted on, indicated by a "ghost" cursor box.
    //
    //    button  figure       none  Shift   Ctrl
    //    1       solid rect   flip  set     unset
    //    2       outl rect    flip  set     unset
    //    3       line         flip  set     unset
    //
    // If Shift or Ctrl is down *before* button 1 is prssed, the action
    // will be as for button 2.

    if (ev->type() != QEvent::MouseButtonRelease) {
        ev->ignore();
        return;
    }
    ev->accept();

    int btn = fp_downbtn;
    fp_downbtn = 0;
    if (sender() == fp_editor) {
        if (!btn)
            return;

        gd_viewport = fp_editor;
        SetGhost(0, 0, 0);

        int jo = fp_jj;
        int io = fp_ii;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        fp_jj = ev->position().x();
        fp_ii = ev->position().y();
#else
        fp_jj = ev->x();
        fp_ii = ev->y();
#endif
        if (!getij(&fp_jj, &fp_ii))
            return;
        int imin = (io < fp_ii ? io : fp_ii);
        int imax = (io > fp_ii ? io : fp_ii);
        int jmin = (jo < fp_jj ? jo : fp_jj);
        int jmax = (jo > fp_jj ? jo : fp_jj);
        switch (btn) {
        case 1:
            for (io = imin; io <= imax; io++) {
                for (jo = jmin; jo <= jmax; jo++) {
                    if (ev->modifiers() & Qt::ShiftModifier)
                        set_pixel(io, jo, FPSETon);
                    else if (ev->modifiers() & Qt::ControlModifier)
                        set_pixel(io, jo, FPSEToff);
                    else
                        set_pixel(io, jo, FPSETflip);
                }
            }
            redraw_edit();
            redraw_sample();
            break;
        case 2:
            if (ev->modifiers() & Qt::ShiftModifier)
                box(imin, imax, jmin, jmax, FPSETon);
            else if (ev->modifiers() & Qt::ControlModifier)
                box(imin, imax, jmin, jmax, FPSEToff);
            else
                box(imin, imax, jmin, jmax, FPSETflip);
            redraw_edit();
            redraw_sample();
            break;
        case 3:
            if (ev->modifiers() & Qt::ShiftModifier)
                line(jo, io, fp_jj, fp_ii, FPSETon);
            else if (ev->modifiers() & Qt::ControlModifier)
                line(jo, io, fp_jj, fp_ii, FPSEToff);
            else
                line(jo, io, fp_jj, fp_ii, FPSETflip);
            redraw_edit();
            redraw_sample();
            break;
        }
    }
    else
        fp_dragging = false;
}


void
QTfillPatDlg::key_down_slot(QKeyEvent *ev)
{
    // Key press.  The arrow keys rotate the pixel array in the
    // direction of the arrow.

    if (ev->type() != QEvent::KeyPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    int i, j;
    FPSETtype tmp;
    switch (ev->key()) {
    case Qt::Key_Right:
        for (i = 0; i < fp_ny; i++) {
            tmp = get_pixel(i, 0);
            for (j = 1; j < fp_nx; j++)
                set_pixel(i, j - 1, get_pixel(i, j));
            set_pixel(i, fp_nx - 1, tmp);
        }
        redraw_edit();
        redraw_sample();
        break;
    case Qt::Key_Left:
        for (i = 0; i < fp_ny; i++) {
            tmp = get_pixel(i, fp_nx - 1);
            for (j = fp_nx - 1; j >= 1; j--)
                set_pixel(i, j, get_pixel(i, j - 1));
            set_pixel(i, 0, tmp);
        }
        redraw_edit();
        redraw_sample();
        break;
    case Qt::Key_Up:
        for (j = 0; j < fp_nx; j++) {
            tmp = get_pixel(fp_ny - 1, j);
            for (i = fp_ny - 1; i >= 1; i--)
                set_pixel(i, j, get_pixel(i - 1, j));
            set_pixel(0, j, tmp);
        }
        redraw_edit();
        redraw_sample();
        break;
    case Qt::Key_Down:
        for (j = 0; j < fp_nx; j++) {
            tmp = get_pixel(0, j);
            for (i = 1; i < fp_ny; i++)
                set_pixel(i - 1, j, get_pixel(i, j));
            set_pixel(fp_ny - 1, j, tmp);
        }
        redraw_edit();
        redraw_sample();
        break;
    }
}


void
QTfillPatDlg::motion_slot(QMouseEvent *ev)
{
    // Pointer motion.  This simply draws an XOR'ed opon box when the
    // pointer button is held down, and the pointer is in the pixel
    // editor.  This is also used for drag/drop detection.

    if (ev->type() != QEvent::MouseMove) {
        ev->ignore();
        return;
    }
    ev->accept();

    if (sender() == fp_editor) {
        if (!fp_downbtn)
            return;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        int xx = ev->position().x();
        int yy = ev->position().y();
#else
        int xx = ev->x();
        int yy = ev->y();
#endif
        if (!getij(&xx, &yy))
            return;

        xx = fp_spa + xx*fp_epsz + fp_epsz/2;
        yy = fp_spa + yy*fp_epsz + fp_epsz/2;

        gd_viewport = fp_editor;
        SetColor(0xffffff);
        UndrawGhost();
        DrawGhost(xx, yy);
    }
    else if (fp_dragging &&
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
            (abs(ev->position().x() - fp_drag_x) > 4 ||
            abs(ev->position().y() - fp_drag_y) > 4)) {
#else
            (abs(ev->x() - fp_drag_x) > 4 || abs(ev->y() - fp_drag_y) > 4)) {
#endif
        fp_dragging = false;

        LayerFillData dd;
        bool isset = false;
        if (sender() == fp_sample) {
            dd.d_from_sample = true;
            isset = true;
            dd.d_nx = fp_nx;
            dd.d_ny = fp_ny;
            memcpy(dd.d_data, fp_array, dd.d_ny*((dd.d_nx + 7)/8));
        }
        else {
            for (int i = 0; i < 18; i++) {
                if (sender() == fp_stores[i]) {
                    dd.d_layernum = i;
                    isset = true;
                    if (i == 0) {
                        dd.d_nx = 8;
                        dd.d_ny = 8;
                        memset(dd.d_data, 0, 8);
                    }
                    else if (i == 1) {
                        dd.d_nx = 8;
                        dd.d_ny = 8;
                        memset(dd.d_data, 0xff, 8);
                    }
                    else {
                        sTpmap *p = Tech()->GetDefaultMap(
                            i-2 + 16*fp_pattern_bank);
                        if (p && p->map) {
                            dd.d_nx = p->nx;
                            dd.d_ny = p->ny;
                            memcpy(dd.d_data, p->map,
                                dd.d_ny*((dd.d_nx + 7)/8));
                        }
                        else {
                            dd.d_nx = 8;
                            dd.d_ny = 8;
                            memset(dd.d_data, 0, 8);
                        }
                    }
                    break;
                }
            }
        }
        if (isset) {
            QDrag *drag = new QDrag(sender());
            drag->setPixmap(QPixmap(QTltab::fillpattern_xpm()));
            QMimeData *mimedata = new QMimeData();
            QByteArray qdata((const char*)&dd, sizeof(LayerFillData));
            mimedata->setData(QTltab::mime_type(), qdata);
            drag->setMimeData(mimedata);
            drag->exec(Qt::CopyAction);
        }
    }
}


void
QTfillPatDlg::enter_slot(QEnterEvent*)
{
    // Pointer entered the fill editor.
    // Set focus so we can see arrow keys.

    fp_editor->setFocus(Qt::MouseFocusReason);
}


void
QTfillPatDlg::drag_enter_slot(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasFormat(QTltab::mime_type())) {
        // The "sender()" is the widget that recieved the event.
        if (sender() == fp_sample || sender() == fp_editor) {
            ev->accept();
            return;
        }

        // Sender must be a store, prevent direct drag/drop between
        // stores.`

        QByteArray bary = ev->mimeData()->data(QTltab::mime_type());
        LayerFillData *dd = (LayerFillData*)bary.data();
        if (dd->d_from_sample || dd->d_from_layer) {
            ev->accept();
            return;
        }
    }
    ev->ignore();
}


void
QTfillPatDlg::drop_event_slot(QDropEvent *ev)
{
    if (ev->mimeData()->hasFormat(QTltab::mime_type())) {
        QByteArray bary = ev->mimeData()->data(QTltab::mime_type());
        LayerFillData *dd = (LayerFillData*)bary.data();
        // The "sender()" is the widget that recieved the event.
        if (sender() == fp_sample || sender() == fp_editor) {
            if (dd->d_from_layer)
                layer_to_def_or_sample(dd, -1);
            else if (dd->d_from_sample) {
                if (ev->source() != fp_sample) {
                    // from another process
                    def_to_sample(dd);
                }
            }
            else
                def_to_sample(dd);
        }
        else {
            for (int i = 0; i < 18; i++) {
                if (sender() == fp_stores[i]) {
                    if (dd->d_from_layer)
                        layer_to_def_or_sample(dd, i);
                    else if (dd->d_from_sample)
                        sample_to_def(dd, i);
                    else {
                        // no drag between def boxes
                        return;
                    }
                    break;
                }
            }
        }
        ev->accept();
        return;
    }
    ev->ignore();
}
// End of slots.


void
QTfillPatDlg::fp_mode_proc(bool editing)
{
    if (editing) {
        fp_editing = true;
        fp_stoctrl->hide();
        fp_stoframe->hide();
        fp_editctrl->show();
        fp_editframe->show();
        redraw_edit();
    }
    else {
        fp_editing = false;
        fp_editctrl->hide();
        fp_editframe->hide();
        fp_stoctrl->show();
        fp_stoframe->show();
    }
    redraw_sample();
}


void
QTfillPatDlg::redraw_edit()
{
    if (!fp_editing)
        return;
    gd_viewport = fp_editor;
    QSize qs = fp_editor->size();
    int wid = qs.width();
    int hei = qs.height();

    int mind = mmMin(wid, hei);
    fp_spa = 2;
    fp_epsz = (mind - 2*fp_spa)/mmMax(fp_nx, fp_ny);

    SetFillpattern(0);
    SetColor(0xf0f0f0);
    Box(0, 0, wid, hei);
    SetBackground(fp_pixbg);
    SetColor(fp_pixbg);
    Box(fp_spa - 1, fp_spa - 1,
        fp_spa + fp_nx*fp_epsz + 2, fp_spa + fp_ny*fp_epsz + 2);

    int bpl = (fp_nx + 7)/8;
    unsigned char *a = fp_array;
    for (int i = 0; i < fp_ny; i++) {
        unsigned int d = *a++;
        for (int j = 1; j < bpl; j++)
            d |= *a++ << j*8;

        unsigned int msk = 1;
        for (int j = 0; j < fp_nx; j++) {
            bool lit = msk & d;
            SetColor(lit ? fp_foreg : fp_pixbg);
            show_pixel(i, j);
            msk <<= 1;
        }
    }
    fp_editor->update();
}


void
QTfillPatDlg::redraw_sample()
{
    gd_viewport = fp_sample;
    QSize qs = fp_sample->size();
    int wid = qs.width();
    int hei = qs.height();

    SetColor(fp_pixbg);
    SetFillpattern(0);
    Box(0, 0, wid, hei);

    SetColor(fp_foreg);
    set_fp(fp_array, fp_nx, fp_ny);
    Box(0, 0, wid, hei);
    SetFillpattern(0);

    bool nonff = false;
    int sz = fp_ny*((fp_nx + 7)/8);
    for (int i = 0; i < sz; i++) {
        if (fp_array[i] != 0xff) {
            nonff = true;
            break;
        }
    }
    if (!nonff) {
        QTdev::SetStatus(fp_cut, false);
        QTdev::SetStatus(fp_outl, false);
        QTdev::SetStatus(fp_fat, false);
        fp_cut->setEnabled(false);
        fp_outl->setEnabled(false);
        fp_fat->setEnabled(false);
    }
    else {
        fp_cut->setEnabled(true);
        fp_outl->setEnabled(true);
        fp_fat->setEnabled(QTdev::GetStatus(fp_outl));
    }
    if (QTdev::GetStatus(fp_outl)) {
        int x1 = 0;
        int y1 = 0;
        int x2 = wid - 1;
        int y2 = hei - 1;
        Line(x1, y1, x2, y1);
        Line(x2, y1, x2, y2);
        Line(x2, y2, x1, y2);
        Line(x1, y2, x1, y1);
        if (QTdev::GetStatus(fp_fat)) {
            x1++;
            y1++;
            x2--;
            y2--;
            Line(x1, y1, x2, y1);
            Line(x2, y1, x2, y2);
            Line(x2, y2, x1, y2);
            Line(x1, y2, x1, y1);
            x1++;
            y1++;
            x2--;
            y2--;
            Line(x1, y1, x2, y1);
            Line(x2, y1, x2, y2);
            Line(x2, y2, x1, y2);
            Line(x1, y2, x1, y1);
        }
    }
    if (QTdev::GetStatus(fp_cut)) {
        int x1 = 0;
        int y1 = 0;
        int x2 = wid - 1;
        int y2 = hei - 1;
        Line(x1, y1, x2, y2);
        Line(x1, y2, x2, y1);
    }
    fp_sample->update();
}


void
QTfillPatDlg::redraw_store(int i)
{
    if (fp_editing)
        return;

    gd_viewport = fp_stores[i];
    QSize sz = fp_stores[i]->size();
    int wid = sz.width();
    int hei = sz.height();

    SetColor(fp_pixbg);
    SetFillpattern(0);
    Box(0, 0, wid, hei);

    SetColor(fp_foreg);
    if (i == 1) {
        SetFillpattern(0);
        Box(0, 0, wid, hei);
    }
    else if (i > 1) {
        int indx = i - 2;
        sTpmap *p = Tech()->GetDefaultMap(indx + 16*fp_pattern_bank);
        if (p && p->map) {
            set_fp(p->map, p->nx, p->ny);
            Box(0, 0, wid, hei);
            SetFillpattern(0);
        }
    }
    fp_stores[i]->update();
}


// Show a box representing a pixel of the fillpattern.
//
void
QTfillPatDlg::show_pixel(int i, int j)
{
    int xx = fp_spa + j*fp_epsz + 1;
    int yy = fp_spa + i*fp_epsz + 1;
    SetFillpattern(0);
    Box(xx, yy, xx + fp_epsz - 2, yy + fp_epsz - 2);
}


// Create the pixmap for the fillpattern given in map, and set
// the GC to use this pixmap.
//
void
QTfillPatDlg::set_fp(unsigned char *pmap, int xx, int yy)
{
    if (!fp_fp)
        fp_fp = new GRfillType;
    fp_fp->newMap(xx, yy, pmap);    // Destroy/create new pixel map.
    DefineFillpattern(fp_fp);       // Destroy/create new pixmap.
    SetFillpattern(fp_fp);          // Save the pixmap for rendering.
}


// Given window coordinates x, y, return the pixel placement in the
// array j, i.
//
bool
QTfillPatDlg::getij(int *x1, int *y1)
{
    int xx = *x1;
    int yy = *y1;
    xx -= fp_spa;
    xx /= fp_epsz;
    yy -= fp_spa;
    yy /= fp_epsz;
    if (xx >= 0 && xx < fp_nx && yy >= 0 && yy < fp_ny) {
        *x1 = xx;
        *y1 = yy;
        return (true);
    }
    return (false);
}


// Set the j, i pixel according to the mode.
//
void
QTfillPatDlg::set_pixel(int i, int j, FPSETtype mode)
{
    int bpl = (fp_nx + 7)/8;
    unsigned char *a = fp_array + i*bpl;
    unsigned int d = *a++;
    for (int k = 1; k < bpl; k++)
        d |= *a++ << k*8;
    unsigned int msk = 1 << j;
    
    if (mode == FPSETflip) {
        if (d & msk) {
            SetColor(fp_pixbg);
            d &= ~msk;
        }
        else {
            SetColor(fp_foreg);
            d |= msk;
        }
    }
    else if (mode == FPSETon) {
        SetColor(fp_foreg);
        d |= msk;
    }
    else {
        SetColor(fp_pixbg);
        d &= ~msk;
    }
    j /= 8;
    fp_array[i*bpl + j] = d >> j*8;
}


// Return the polarity of the pixel j, i.
//
QTfillPatDlg::FPSETtype
QTfillPatDlg::get_pixel(int i, int j)
{
    int bpl = (fp_nx + 7)/8;
    unsigned char *a = fp_array + i*bpl;
    unsigned int d = *a++;
    for (int k = 1; k < bpl; k++)
        d |= *a++ << k*8;
    unsigned int msk = 1 << j;
    return ((d & msk) ? FPSETon : FPSEToff);
}


// Set the pixels in the array in a line from j1, i1 to j2, i2.
//
void
QTfillPatDlg::line(int x1, int y1, int x2, int y2, FPSETtype mode)
{
    int i;
    double r = 0.0;
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (!dy) {
        if (x1 > x2) {
            i = x2;
            x2 = x1;
            x1 = i;
        }
        for (i = x1; i <= x2; i++)
            set_pixel(y1, i, mode);
    }
    else if (!dx) {
        if (y1 > y2) {
            i = y2;
            y2 = y1;
            y1 = i;
        }
        for (i = y1; i < y2; i++)
            set_pixel(i, x1, mode);
    }
    else if (abs(dx) > abs(dy)) {
        if (x1 > x2) {
            i = x2;
            x2 = x1;
            x1 = i;
            i = y2;
            y2 = y1;
            y1 = i;
        }
        for (i = x1; i <= x2; i++) {
            set_pixel((int)(y1 + r + .5), i, mode);
            r += (double)dy/dx;
        }
    }
    else {
        if (y1 > y2) {
            i = y2;
            y2 = y1;
            y1 = i;
            i = x2;
            x2 = x1;
            x1 = i;
        }
        for (i = y1; i <= y2; i++) {
            set_pixel(i, (int)(x1 + r + .5), mode);
            r += (double)dx/dy;
        }
    }
}


// Set the pixels in the array which are on the boundary of the
// box defined by jmin, imin to jmax, imax.
//
void
QTfillPatDlg::box(int imin, int imax, int jmin, int jmax, FPSETtype mode)
{
    int i;
    for (i = jmin; i <= jmax; i++)
        set_pixel(imax, i, mode);
    if (imin != imax) {
        for (i = jmin; i <= jmax; i++)
            set_pixel(imin, i, mode);
        imin++;
        for (i = imin; i < imax; i++)
            set_pixel(i, jmin, mode);
        for (i = imin; i < imax; i++)
            set_pixel(i, jmax, mode);
    }
}


// Load the default fillpattern at x, y into the editor.
//
void
QTfillPatDlg::def_to_sample(LayerFillData *dd)
{
    fp_nx = dd->d_nx;
    fp_ny = dd->d_ny;
    int bpl = (fp_nx + 7)/8;
    int sz = fp_ny*bpl;
    memset(fp_array, 0, sizeof(fp_array));
    memcpy(fp_array, dd->d_data, fp_ny*bpl);
    fp_spnx->setValue(fp_nx);
    fp_spny->setValue(fp_ny);

    bool nonz = false;
    bool nonff = false;
    for (int i = 0; i < sz; i++) {
        if (fp_array[i])
            nonz = true;
        if (fp_array[i] != 0xff)
            nonff = true;
    }
    if (!nonz || !nonff) {
        QTdev::SetStatus(fp_outl, false);
        QTdev::SetStatus(fp_fat, false);
    }

    redraw_edit();
    redraw_sample();
}


// Store the present pattern map into the default pattern storage area
// pointed to, and redisplay.
//
void
QTfillPatDlg::sample_to_def(LayerFillData *dd, int indx)
{
    if (indx <= 1) {
        // can't reset solid or open
        return;
    }

    indx -= 2;
    sTpmap *p = Tech()->GetDefaultMap(indx + 16*fp_pattern_bank);
    if (p) {
        delete [] p->map;
        p->nx = dd->d_nx;
        p->ny = dd->d_ny;
        int sz = fp_ny*((fp_nx + 7)/8);
        p->map = new unsigned char[sz];
        bool nonz = false;
        bool nonff = false;
        for (int i = 0; i < sz; i++) {
            p->map[i] = dd->d_data[i];
            if (dd->d_data[i])
                nonz = true;
            if (fp_array[i] != 0xff)
                nonff = true;
        }
        if (!nonz) {
            delete [] p->map;
            p->map = 0;
            p->nx = p->ny = 0;
        }
        if (!nonff)
            p->nx = p->ny = 8;
        redraw_store(indx+2);
    }
    SetFillpattern(0);
}


// Load the current layer pattern into the sample or default areas.
//
void
QTfillPatDlg::layer_to_def_or_sample(LayerFillData *dd, int indx)
{
    if (indx < 0) {
        fp_nx = dd->d_nx;
        fp_ny = dd->d_ny;
        memset(fp_array, 0, sizeof(fp_array));
        memcpy(fp_array, dd->d_data, fp_ny*((fp_nx + 7)/8));
        fp_spnx->setValue(fp_nx);
        fp_spny->setValue(fp_ny);

        if (dd->d_flags & LFD_OUTLINE) {
            QTdev::SetStatus(fp_outl, true);
            fp_fat->setEnabled(true);
            QTdev::SetStatus(fp_fat, (dd->d_flags & LFD_FAT));
        }
        else {
            QTdev::SetStatus(fp_outl, false);
            QTdev::SetStatus(fp_fat, false);
            fp_fat->setEnabled(false);
        }
        QTdev::SetStatus(fp_cut, (dd->d_flags & LFD_CUT));

        if (LT()->CurLayer())
            fp_foreg = dsp_prm(LT()->CurLayer())->pixel();
        fp_pixbg = DSP()->Color(BackgroundColor);
        for (int i = 0; i < 18; i++)
            redraw_store(i);
        redraw_edit();
        redraw_sample();
    }
    else {
        if (indx <= 1) {
            // can't reset solid or open
            return;
        }
        indx -= 2;
        sTpmap *p = Tech()->GetDefaultMap(indx + 16*fp_pattern_bank);
        if (p) {
            delete [] p->map;
            p->nx = dd->d_nx;
            p->ny = dd->d_ny;
            int sz = p->ny*((p->nx + 7)/8);
            p->map = new unsigned char[sz];
            memcpy(p->map, dd->d_data, sz);
            redraw_store(indx+2);
        }
        SetFillpattern(0);
    }
}


// Set the pattern for the layer.
//
void
QTfillPatDlg::pattern_to_layer(LayerFillData *dd, CDl *ld)
{
    if (!ld)
        return;
    if (ld->isInvisible()) {
        Log()->ErrorLog(mh::Processing,
            "Can't transfer fill pattern to invisible layer.");
        return;
    }
    bool nonz = false;
    bool nonff = false;

    int sz = dd->d_ny*((dd->d_nx + 7)/8);
    for (int i = 0; i < sz; i++) {
        if (dd->d_data[i])
            nonz = true;
        if (dd->d_data[i] != 0xff)
            nonff = true;
    }
    if (!nonff) {
        // solid fill
        ld->setFilled(true);
        ld->setOutlined(false);
        ld->setOutlinedFat(false);
        ld->setCut(false);
        defineFillpattern(dsp_prm(ld)->fill(), 8, 8, 0);
    }
    else {
        if (!nonz) {
            // empty fill
            ld->setFilled(false);
            defineFillpattern(dsp_prm(ld)->fill(), 8, 8, 0);
        }
        else {
            // stippled
            defineFillpattern(dsp_prm(ld)->fill(), dd->d_nx, dd->d_ny,
                dd->d_data);
            ld->setFilled(true);
        }
        if (QTdev::GetStatus(fp_outl)) {
            ld->setOutlined(true);
            ld->setOutlinedFat(QTdev::GetStatus(fp_fat));
        }
        else {
            ld->setOutlined(false);
            ld->setOutlinedFat(false);
        }
        ld->setCut(QTdev::GetStatus(fp_cut));
    }
    SetFillpattern(0);
    LT()->ShowLayerTable();
    XM()->PopUpLayerPalette(0, MODE_UPD, false, 0);
}


// Set the handlers for a drawing area.
//
void
QTfillPatDlg::connect_sigs(QTcanvas *darea, bool dnd_rcvr)
{
    connect(darea, &QTcanvas::press_event,
        this, &QTfillPatDlg::button_down_slot);
    connect(darea, &QTcanvas::release_event,
        this, &QTfillPatDlg::button_up_slot);
    connect(darea, &QTcanvas::key_press_event,
        this, &QTfillPatDlg::key_down_slot);
    connect(darea, &QTcanvas::motion_event,
        this, &QTfillPatDlg::motion_slot);

    if (dnd_rcvr) {
        // destination
        darea->setAcceptDrops(true);
        connect(darea, &QTcanvas::drag_enter_event,
            this, &QTfillPatDlg::drag_enter_slot);
        connect(darea, &QTcanvas::drop_event,
            this, &QTfillPatDlg::drop_event_slot);
    }
}


// Static function.
// Draw an open box or line, using the XOR GC.
//
void
QTfillPatDlg::fp_drawghost(int x0, int y0, int x1, int y1, bool)
{
    if (QTfillPatDlg::self()) {
        if (QTfillPatDlg::self()->fp_downbtn == 3)
            QTfillPatDlg::self()->Line(x0, y0, x1, y1);
        else {
            GRmultiPt p(5);
            p.assign(0, x0, y0);
            p.assign(1, x1, y0);
            p.assign(2, x1, y1);
            p.assign(3, x0, y1);
            p.assign(4, x0, y0);
            QTfillPatDlg::self()->PolyLine(&p, 5);
        }
    }
}


// Static function.
int
QTfillPatDlg::fp_update_idle_proc(void*)
{
    if (instPtr)
        instPtr->update();
    return (0);
}

