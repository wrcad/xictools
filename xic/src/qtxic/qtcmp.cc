
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

#include "qtcmp.h"
#include "fio.h"
#include "fio_compare.h"
#include "cvrt.h"
#include "dsp_inlines.h"
#include "promptline.h"
#include "errorlog.h"
#include "qtinterf/qtdblsb.h"

#include <QLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QRadioButton>
#include <QMenu>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>


//-----------------------------------------------------------------------------
// The Compare Layers dialog.
//
// Help system keywords used:
//  xic:diff

void
cConvert::PopUpCompare(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTcompareDlg::self())
            QTcompareDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTcompareDlg::self())
            QTcompareDlg::self()->update();
        return;
    }
    if (QTcompareDlg::self())
        return;

    new QTcompareDlg(caller);

    QTcompareDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_UR), QTcompareDlg::self(),
        QTmainwin::self()->Viewport());
    QTcompareDlg::self()->show();
}
// End of cConvert functions.


class QTcomparePathEdit : public QLineEdit
{
public:
    QTcomparePathEdit(QWidget *prnt = 0) : QLineEdit(prnt) { }

    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
};


void
QTcomparePathEdit::dragEnterEvent(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasUrls()) {
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/plain")) {
        ev->accept();
        return;
    }
    ev->ignore();
}


void
QTcomparePathEdit::dropEvent(QDropEvent *ev)
{
    if (ev->mimeData()->hasUrls()) {
        QByteArray ba = ev->mimeData()->data("text/plain");
        const char *str = ba.constData() + strlen("File://");
        setText(str);
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/twostring")) {
        // Drops from content lists may be in the form
        // "fname_or_chd\ncellname".  Keep the filename.
        char *str = lstring::copy(ev->mimeData()->data("text/plain").constData());
        char *t = strchr(str, '\n');
        if (t)
            *t = 0;
        setText(str);
        delete [] str;
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/plain")) {
        // The default action will insert the text at the click location,
        // instead here we replace any existing text.
        QByteArray ba = ev->mimeData()->data("text/plain");
        setText(ba.constData());
        ev->accept();
        return;
    }
    ev->ignore();
}


class QTcompareCellEdit : public QLineEdit
{
public:
    QTcompareCellEdit(QWidget *prnt = 0) : QLineEdit(prnt) { }

    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
};


void
QTcompareCellEdit::dragEnterEvent(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasUrls()) {
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/plain")) {
        ev->accept();
        return;
    }
    ev->ignore();
}


void
QTcompareCellEdit::dropEvent(QDropEvent *ev)
{
    if (ev->mimeData()->hasUrls()) {
        QByteArray ba = ev->mimeData()->data("text/plain");
        const char *str = ba.constData() + strlen("File://");
        str = lstring::strip_path(str);
        setText(text() + QString(" ") + QString(str));
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/twostring")) {
        // Drops from content lists may be in the form
        // "fname_or_chd\ncellname".  Keep the cellname.
        char *str = lstring::copy(ev->mimeData()->data("text/plain").constData());
        const char *t = strchr(str, '\n');
        if (t)
            t = t+1;
        else
            t = str;
        setText(text() + QString(" ") + QString(t));
        delete [] str;
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/plain")) {
        // The default action will insert the text at the click location,
        // instead here we append to any existing text.
        QByteArray ba = ev->mimeData()->data("text/plain");
        setText(text() + QString(" ") + ba.constData());
        ev->accept();
        return;
    }
    ev->ignore();
}


QTcompareDlg::sCmp_store QTcompareDlg::cmp_store;
QTcompareDlg *QTcompareDlg::instPtr;

QTcompareDlg::QTcompareDlg(GRobject c)
{
    instPtr = this;
    cmp_caller = c;
    cmp_mode = 0;
    cmp_fname1 = 0;
    cmp_fname2 = 0;
    cmp_cnames1 = 0;
    cmp_cnames2 = 0;
    cmp_diff_only = 0;
    cmp_layer_list = 0;
    cmp_layer_use = 0;
    cmp_layer_skip = 0;
    cmp_sb_max_errs = 0;

    cmp_p1_recurse = 0;
    cmp_p1_expand_arrays = 0;
    cmp_p1_slop = 0;
    cmp_p1_dups = 0;
    cmp_p1_boxes = 0;
    cmp_p1_polys = 0;
    cmp_p1_wires = 0;
    cmp_p1_labels = 0;
    cmp_p1_insts = 0;
    cmp_p1_boxes_prp = 0;
    cmp_p1_polys_prp = 0;
    cmp_p1_wires_prp = 0;
    cmp_p1_labels_prp = 0;
    cmp_p1_insts_prp = 0;
    cmp_p1_phys = 0;
    cmp_p1_elec = 0;
    cmp_p1_cell_prp = 0;
    cmp_p1_fltr = 0;
    cmp_p1_setup = 0;
    cmp_p1_fltr_mode = PrpFltDflt;

    cmp_p2_expand_arrays = 0;
    cmp_p2_recurse = 0;
    cmp_p2_insts = 0;

    cmp_p3_aoi_use = 0;
    cmp_p3_s_btn = 0;
    cmp_p3_r_btn = 0;
    cmp_p3_s_menu = 0;
    cmp_p3_r_menu = 0;
    cmp_sb_p3_aoi_left = 0;
    cmp_sb_p3_aoi_bottom = 0;
    cmp_sb_p3_aoi_right = 0;
    cmp_sb_p3_aoi_top = 0;
    cmp_sb_p3_fine_grid = 0;
    cmp_sb_p3_coarse_mult = 0;

    setWindowTitle(tr("Compare Layouts"));
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
    QLabel *label = new QLabel(tr("Compare Cells/Geometry Between Layouts"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // Comparison mode selection notebook.
    //
    cmp_mode = new QTabWidget();
    vbox->addWidget(cmp_mode);

    per_cell_obj_page();
    per_cell_geom_page();
    flat_geom_page();

    // Left file/cell entries
    //
    gb = new QGroupBox("<<<");
    vbox->addWidget(gb);
    QGridLayout *grid = new QGridLayout(gb);

    label = new QLabel(tr("Source"));
    grid->addWidget(label, 0, 0);
    label = new QLabel(tr("Cells"));
    grid->addWidget(label, 1, 0);
    cmp_fname1 =  new QTcomparePathEdit();
    cmp_fname1->setReadOnly(false);
    cmp_fname1->setAcceptDrops(true);
    grid->addWidget(cmp_fname1, 0, 1);
    cmp_cnames1 = new QTcompareCellEdit();
    cmp_cnames1->setReadOnly(false);
    cmp_cnames1->setAcceptDrops(true);
    grid->addWidget(cmp_cnames1, 1, 1);

    // Right file/cell entries
    //
    gb = new QGroupBox(">>>");
    vbox->addWidget(gb);
    grid = new QGridLayout(gb);

    label = new QLabel(tr("Source"));
    grid->addWidget(label, 0, 0);
    label = new QLabel(tr("Equiv"));
    grid->addWidget(label, 1, 0);
    cmp_fname2 = new QTcomparePathEdit();
    cmp_fname2->setReadOnly(false);
    cmp_fname2->setAcceptDrops(true);
    grid->addWidget(cmp_fname2, 0, 1);
    cmp_cnames2 = new QTcompareCellEdit();
    cmp_cnames2->setReadOnly(false);
    cmp_cnames2->setAcceptDrops(true);
    grid->addWidget(cmp_cnames2, 1, 1);

    cmp_fname2->setAcceptDrops(true);
    cmp_cnames2->setAcceptDrops(true);

    // Layer list and associated buttons
    //
    gb = new QGroupBox(tr("Layer List"));
    vbox->addWidget(gb);
    grid = new QGridLayout(gb);

    cmp_layer_use = new QCheckBox(tr("Layers Only"));
    grid->addWidget(cmp_layer_use, 0, 0);
    connect(cmp_layer_use, SIGNAL(stateChanged(int)),
        this, SLOT(luse_btn_slot(int)));

    cmp_layer_skip = new QCheckBox(tr("Skip Layers"));
    grid->addWidget(cmp_layer_skip, 0, 1);
    connect(cmp_layer_skip, SIGNAL(stateChanged(int)),
        this, SLOT(lskip_btn_slot(int)));

    cmp_layer_list = new QLineEdit();
    grid->addWidget(cmp_layer_list, 1, 0, 1, 2);

    // Diff only and max differences spin button
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    cmp_diff_only = new QCheckBox(tr("Differ Only"));
    hbox->addWidget(cmp_diff_only);

    hbox->addStretch(1);

    label = new QLabel(tr("Maximum Differences"));
    hbox->addWidget(label);

    cmp_sb_max_errs = new QSpinBox;
    hbox->addWidget(cmp_sb_max_errs);
    cmp_sb_max_errs->setMinimum(0);
    cmp_sb_max_errs->setMaximum(1000000);
    cmp_sb_max_errs->setValue(0);

    // Go and Dismiss buttons
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    btn = new QPushButton(tr("Go"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(go_btn_slot()));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    recall();
    p1_sens();
}


QTcompareDlg::~QTcompareDlg()
{
    if (QTdev::GetStatus(cmp_p1_setup))
        QTdev::CallCallback(cmp_p1_setup);
    save();
    instPtr = 0;
    if (cmp_caller)
        QTdev::Deselect(cmp_caller);
}


void
QTcompareDlg::update()
{
}


void
QTcompareDlg::save()
{
    sCmp_store *s = &cmp_store;
    delete [] s->cs_file1;
    s->cs_file1 = lstring::copy(cmp_fname1->text().toLatin1().constData());
    delete [] s->cs_file2;
    s->cs_file2 = lstring::copy(cmp_fname2->text().toLatin1().constData());
    delete [] s->cs_cells1;
    s->cs_cells1 = lstring::copy(cmp_cnames1->text().toLatin1().constData());
    delete [] s->cs_cells2;
    s->cs_cells2 = lstring::copy(cmp_cnames2->text().toLatin1().constData());
    delete [] s->cs_layers;
    s->cs_layers = lstring::copy(cmp_layer_list->text().toLatin1().constData());
    s->cs_mode = cmp_mode->currentIndex();
    s->cs_layer_only = QTdev::GetStatus(cmp_layer_use);
    s->cs_layer_skip = QTdev::GetStatus(cmp_layer_skip);
    s->cs_differ = QTdev::GetStatus(cmp_diff_only);
    s->cs_max_errs = cmp_sb_max_errs->value();

    if (cmp_p1_recurse) {
        s->cs_p1_recurse = QTdev::GetStatus(cmp_p1_recurse);
        s->cs_p1_expand_arrays = QTdev::GetStatus(cmp_p1_expand_arrays);
        s->cs_p1_slop = QTdev::GetStatus(cmp_p1_slop);
        s->cs_p1_dups = QTdev::GetStatus(cmp_p1_dups);
        s->cs_p1_boxes = QTdev::GetStatus(cmp_p1_boxes);
        s->cs_p1_polys = QTdev::GetStatus(cmp_p1_polys);
        s->cs_p1_wires = QTdev::GetStatus(cmp_p1_wires);
        s->cs_p1_labels = QTdev::GetStatus(cmp_p1_labels);
        s->cs_p1_insts = QTdev::GetStatus(cmp_p1_insts);
        s->cs_p1_boxes_prp = QTdev::GetStatus(cmp_p1_boxes_prp);
        s->cs_p1_polys_prp = QTdev::GetStatus(cmp_p1_polys_prp);
        s->cs_p1_wires_prp = QTdev::GetStatus(cmp_p1_wires_prp);
        s->cs_p1_labels_prp = QTdev::GetStatus(cmp_p1_labels_prp);
        s->cs_p1_insts_prp = QTdev::GetStatus(cmp_p1_insts_prp);
        s->cs_p1_elec = QTdev::GetStatus(cmp_p1_elec);
        s->cs_p1_cell_prp = QTdev::GetStatus(cmp_p1_cell_prp);
        s->cs_p1_fltr = cmp_p1_fltr_mode;
    }

    if (cmp_p2_recurse) {
        s->cs_p2_recurse = QTdev::GetStatus(cmp_p2_recurse);
        s->cs_p2_expand_arrays = QTdev::GetStatus(cmp_p2_expand_arrays);
        s->cs_p2_insts = QTdev::GetStatus(cmp_p2_insts);
    }

    if (cmp_p3_aoi_use) {
        s->cs_p3_use_window = QTdev::GetStatus(cmp_p3_aoi_use);
        s->cs_p3_left = cmp_sb_p3_aoi_left->value();
        s->cs_p3_bottom = cmp_sb_p3_aoi_bottom->value();
        s->cs_p3_right = cmp_sb_p3_aoi_right->value();
        s->cs_p3_top = cmp_sb_p3_aoi_top->value();
        s->cs_p3_fine_grid = cmp_sb_p3_fine_grid->value();
        s->cs_p3_coarse_mult = cmp_sb_p3_coarse_mult->value();
    }
}


void
QTcompareDlg::recall()
{
    sCmp_store *s = &cmp_store;
    cmp_fname1->setText(s->cs_file1 ? s->cs_file1 : "");
    cmp_fname2->setText(s->cs_file2 ? s->cs_file2 : "");
    cmp_cnames1->setText(s->cs_cells1 ? s->cs_cells1 : "");
    cmp_cnames2->setText(s->cs_cells2 ? s->cs_cells2 : "");
    cmp_layer_list->setText(s->cs_layers ? s->cs_layers : "");
    cmp_mode->setCurrentIndex(s->cs_mode);
    QTdev::SetStatus(cmp_layer_use, s->cs_layer_only);
    QTdev::SetStatus(cmp_layer_skip, s->cs_layer_skip);
    QTdev::SetStatus(cmp_diff_only, s->cs_differ);
    cmp_sb_max_errs->setValue(s->cs_max_errs);
    cmp_layer_list->setEnabled(s->cs_layer_only || s->cs_layer_skip);
}


void
QTcompareDlg::recall_p1()
{
    sCmp_store *s = &cmp_store;
    QTdev::SetStatus(cmp_p1_recurse, s->cs_p1_recurse);
    QTdev::SetStatus(cmp_p1_expand_arrays, s->cs_p1_expand_arrays);
    QTdev::SetStatus(cmp_p1_slop, s->cs_p1_slop);
    QTdev::SetStatus(cmp_p1_dups, s->cs_p1_dups);
    QTdev::SetStatus(cmp_p1_boxes, s->cs_p1_boxes);
    QTdev::SetStatus(cmp_p1_polys, s->cs_p1_polys);
    QTdev::SetStatus(cmp_p1_wires, s->cs_p1_wires);
    QTdev::SetStatus(cmp_p1_labels, s->cs_p1_labels);
    QTdev::SetStatus(cmp_p1_insts, s->cs_p1_insts);
    QTdev::SetStatus(cmp_p1_boxes_prp, s->cs_p1_boxes_prp);
    QTdev::SetStatus(cmp_p1_polys_prp, s->cs_p1_polys_prp);
    QTdev::SetStatus(cmp_p1_wires_prp, s->cs_p1_wires_prp);
    QTdev::SetStatus(cmp_p1_labels_prp, s->cs_p1_labels_prp);
    QTdev::SetStatus(cmp_p1_insts_prp, s->cs_p1_insts_prp);
    QTdev::SetStatus(cmp_p1_phys, !s->cs_p1_elec);
    QTdev::SetStatus(cmp_p1_elec, s->cs_p1_elec);
    QTdev::SetStatus(cmp_p1_cell_prp, s->cs_p1_cell_prp);
    cmp_p1_fltr->setCurrentIndex(s->cs_p1_fltr);
}


void
QTcompareDlg::recall_p2()
{
    sCmp_store *s = &cmp_store;
    QTdev::SetStatus(cmp_p2_recurse, s->cs_p2_recurse);
    QTdev::SetStatus(cmp_p2_expand_arrays, s->cs_p2_expand_arrays);
    QTdev::SetStatus(cmp_p2_insts, s->cs_p2_insts);
}


void
QTcompareDlg::recall_p3()
{
    sCmp_store *s = &cmp_store;
    QTdev::SetStatus(cmp_p3_aoi_use, s->cs_p3_use_window);
    cmp_sb_p3_aoi_left->setValue(s->cs_p3_left);
    cmp_sb_p3_aoi_bottom->setValue(s->cs_p3_bottom);
    cmp_sb_p3_aoi_right->setValue(s->cs_p3_right);
    cmp_sb_p3_aoi_top->setValue(s->cs_p3_top);
    cmp_sb_p3_fine_grid->setValue(s->cs_p3_fine_grid);
    cmp_sb_p3_coarse_mult->setValue(s->cs_p3_coarse_mult);
    cmp_sb_p3_aoi_left->setEnabled(s->cs_p3_use_window);
    cmp_sb_p3_aoi_bottom->setEnabled(s->cs_p3_use_window);
    cmp_sb_p3_aoi_right->setEnabled(s->cs_p3_use_window);
    cmp_sb_p3_aoi_top->setEnabled(s->cs_p3_use_window);
}


void
QTcompareDlg::per_cell_obj_page()
{
    QWidget *page = new QWidget;
    cmp_mode->addTab(page, tr("Per-Cell Objrcts"));
    QGridLayout *grid = new QGridLayout(page);
    QMargins qmtop(2, 2, 2, 2);

    // Expand arrays and Recurse buttons
    //
    cmp_p1_recurse = new QCheckBox(tr("Recurse Into Hierarchy"));
    grid->addWidget(cmp_p1_recurse, 0, 0, 1, 2);

    cmp_p1_expand_arrays = new QCheckBox(tr("Expand Arrays"));
    grid->addWidget(cmp_p1_expand_arrays, 0, 2);

    cmp_p1_slop = new QCheckBox(tr("Box to Wire/Poly Check"));
    grid->addWidget(cmp_p1_slop, 1, 0, 1, 2);

    cmp_p1_dups = new QCheckBox(tr("Ignore Duplicate Objects"));
    grid->addWidget(cmp_p1_dups, 1, 2);

    // Object Types group
    //
    QGroupBox *gb = new QGroupBox(tr("Object Types"));
    grid->addWidget(gb, 2, 0, 5, 1);
    QVBoxLayout *vb = new QVBoxLayout(gb);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    cmp_p1_boxes = new QCheckBox(tr("Boxes"));
    vb->addWidget(cmp_p1_boxes);
    connect(cmp_p1_boxes, SIGNAL(stateChanged(int)),
        this, SLOT(p1_sens_set_slot(int)));

    cmp_p1_polys = new QCheckBox(tr("Polygons"));
    vb->addWidget(cmp_p1_polys);
    connect(cmp_p1_polys, SIGNAL(stateChanged(int)),
        this, SLOT(p1_sens_set_slot(int)));

    cmp_p1_wires = new QCheckBox(tr("Wires"));
    vb->addWidget(cmp_p1_wires);
    connect(cmp_p1_wires, SIGNAL(stateChanged(int)),
        this, SLOT(p1_sens_set_slot(int)));

    cmp_p1_labels = new QCheckBox(tr("Labels"));
    vb->addWidget(cmp_p1_labels);
    connect(cmp_p1_labels, SIGNAL(stateChanged(int)),
        this, SLOT(p1_sens_set_slot(int)));

    cmp_p1_insts = new QCheckBox(tr("Cell Instances"));
    vb->addWidget(cmp_p1_insts);
    connect(cmp_p1_insts, SIGNAL(stateChanged(int)),
        this, SLOT(p1_sens_set_slot(int)));

    // Properties group
    //
    gb = new QGroupBox(tr("Properties"));
    grid->addWidget(gb, 2, 1, 5, 1);
    vb = new QVBoxLayout(gb);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    cmp_p1_boxes_prp = new QCheckBox();
    vb->addWidget(cmp_p1_boxes_prp);

    cmp_p1_polys_prp = new QCheckBox();
    vb->addWidget(cmp_p1_polys_prp);

    cmp_p1_wires_prp = new QCheckBox();
    vb->addWidget(cmp_p1_wires_prp);

    cmp_p1_labels_prp = new QCheckBox();
    vb->addWidget(cmp_p1_labels_prp);

    cmp_p1_insts_prp = new QCheckBox();
    vb->addWidget(cmp_p1_insts_prp);

    cmp_p1_phys = new QRadioButton(tr("Physical"));
    grid->addWidget(cmp_p1_phys, 2, 2);
    connect(cmp_p1_phys, SIGNAL(toggled(bool)),
        this, SLOT(p1_mode_btn_slot(bool)));

    cmp_p1_elec = new QRadioButton(tr("Electrical"));
    grid->addWidget(cmp_p1_elec, 3, 2);

    cmp_p1_cell_prp = new QCheckBox(tr("Structure Properties"));
    grid->addWidget(cmp_p1_cell_prp, 4, 2);

    QLabel *label = new QLabel(tr("Property Filtering:"));
    label->setAlignment(Qt::AlignCenter);
    grid->addWidget(label, 5, 2);

    QHBoxLayout *hb = new QHBoxLayout();
    grid->addLayout(hb, 6, 2);

    cmp_p1_fltr = new QComboBox();
    hb->addWidget(cmp_p1_fltr);
    cmp_p1_fltr->addItem(tr("Default"));
    cmp_p1_fltr->addItem(tr("None"));
    cmp_p1_fltr->addItem(tr("Custrom"));
    connect(cmp_p1_fltr, SIGNAL(currentIndexChanged(int)),
        this, SLOT(p1_fltr_menu_slot(int)));

    cmp_p1_setup = new QPushButton(tr("Setup"));
    hb->addWidget(cmp_p1_setup);
    cmp_p1_setup->setCheckable(true);
    cmp_p1_setup->setAutoDefault(false);
    connect(cmp_p1_setup, SIGNAL(toggled(bool)),
        this, SLOT(p1_setup_btn_slot(bool)));

    recall_p1();
}


void
QTcompareDlg::per_cell_geom_page()
{
    QWidget *page = new QWidget;
    cmp_mode->addTab(page, tr("Per-Cell Geometry"));
    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    // Expand arrays and Recurse buttons
    //
    cmp_p2_recurse = new QCheckBox(tr("Recurse Into Hierarchy"));
    hbox->addWidget(cmp_p2_recurse);

    cmp_p2_expand_arrays = new QCheckBox(tr("Expand Arrays"));
    hbox->addWidget(cmp_p2_expand_arrays);

    cmp_p2_insts = new QCheckBox(tr("Compare Cell Instances"));
    vbox->addWidget(cmp_p2_insts);

    QLabel *label = new QLabel(tr("Physical Cells Only"));
    label->setAlignment(Qt::AlignCenter);
    vbox->addWidget(label);

    recall_p2();
}


void
QTcompareDlg::flat_geom_page()
{
    QWidget *page = new QWidget;
    cmp_mode->addTab(page, tr("Flat Geometry"));
    QGridLayout *grid = new QGridLayout(page);

    cmp_p3_aoi_use = new QCheckBox(tr("Use Window"));
    grid->addWidget(cmp_p3_aoi_use, 0, 0, 1, 3);
    connect(cmp_p3_aoi_use, SIGNAL(stateChanged(int)),
        this, SLOT(p3_usewin_btn_slot(int)));

    cmp_p3_s_btn = new QPushButton("S");
    grid->addWidget(cmp_p3_s_btn, 1, 0);
    cmp_p3_s_btn->setAutoDefault(false);
    cmp_p3_s_menu = new QMenu();
    char buf[64];
    cmp_p3_s_btn->setMenu(cmp_p3_s_menu);
    for (int i = 0; i < FIO_NUM_BB_STORE; i++) {
        snprintf(buf, sizeof(buf), "Reg %d", i);
        QAction *a = cmp_p3_s_menu->addAction(buf);
        a->setData(i);
    }
    connect(cmp_p3_s_menu, SIGNAL(triggered(QAction*)),
        this, SLOT(p3_s_menu_slot(QAction*)));

    cmp_p3_r_btn = new QPushButton("R");
    grid->addWidget(cmp_p3_r_btn, 2, 0);
    cmp_p3_r_btn->setAutoDefault(false);
    cmp_p3_r_menu = new QMenu();
    cmp_p3_r_btn->setMenu(cmp_p3_r_menu);
    for (int i = 0; i < FIO_NUM_BB_STORE; i++) {
        snprintf(buf, sizeof(buf), "Reg %d", i);
        QAction *a = cmp_p3_r_menu->addAction(buf);
        a->setData(i);
    }
    connect(cmp_p3_r_menu, SIGNAL(triggered(QAction*)),
        this, SLOT(p3_r_menu_slot(QAction*)));

    QLabel *wnd_l_label = new QLabel(tr("Left"));
    grid->addWidget(wnd_l_label, 1, 1);
    wnd_l_label->setAlignment(Qt::AlignCenter);

    QLabel *wnd_r_label = new QLabel("Right");
    grid->addWidget(wnd_r_label, 2, 1);
    wnd_r_label->setAlignment(Qt::AlignCenter);

    int ndgt = CD()->numDigits();
    cmp_sb_p3_aoi_left = new QTdoubleSpinBox();
    grid->addWidget(cmp_sb_p3_aoi_left, 1, 2);
    cmp_sb_p3_aoi_left->setMinimum(-1e6);
    cmp_sb_p3_aoi_left->setMaximum(1e6);
    cmp_sb_p3_aoi_left->setDecimals(ndgt);
    cmp_sb_p3_aoi_left->setValue(MICRONS(FIO()->CvtWindow()->left));

    cmp_sb_p3_aoi_right = new QTdoubleSpinBox();
    grid->addWidget(cmp_sb_p3_aoi_right, 2, 2);
    cmp_sb_p3_aoi_right->setMinimum(-1e6);
    cmp_sb_p3_aoi_right->setMaximum(1e6);
    cmp_sb_p3_aoi_right->setDecimals(ndgt);
    cmp_sb_p3_aoi_right->setValue(MICRONS(FIO()->CvtWindow()->right));

    QLabel *wnd_b_label = new QLabel(tr("Bottom"));
    grid->addWidget(wnd_b_label, 1, 3);
    wnd_b_label->setAlignment(Qt::AlignCenter);

    QLabel  *wnd_t_label = new QLabel(tr("Top"));
    grid->addWidget(wnd_t_label, 2, 3);
    wnd_t_label->setAlignment(Qt::AlignCenter);

    cmp_sb_p3_aoi_bottom = new QTdoubleSpinBox();
    grid->addWidget(cmp_sb_p3_aoi_bottom, 1, 4);
    cmp_sb_p3_aoi_bottom->setMinimum(-1e6);
    cmp_sb_p3_aoi_bottom->setMaximum(1e6);
    cmp_sb_p3_aoi_bottom->setDecimals(ndgt);
    cmp_sb_p3_aoi_bottom->setValue(MICRONS(FIO()->CvtWindow()->bottom));

    cmp_sb_p3_aoi_top = new QTdoubleSpinBox();
    grid->addWidget(cmp_sb_p3_aoi_top, 2, 4);
    cmp_sb_p3_aoi_top->setMinimum(-1e6);
    cmp_sb_p3_aoi_top->setMaximum(1e6);
    cmp_sb_p3_aoi_top->setDecimals(ndgt);
    cmp_sb_p3_aoi_top->setValue(MICRONS(FIO()->CvtWindow()->top));

    QLabel *label = new QLabel(tr("Fine Grid"));
    label->setAlignment(Qt::AlignCenter);
    grid->addWidget(label, 4, 0, 1, 2);

    cmp_sb_p3_fine_grid = new QSpinBox();
    grid->addWidget(cmp_sb_p3_fine_grid, 4, 2);
    cmp_sb_p3_fine_grid->setMinimum(1);
    cmp_sb_p3_fine_grid->setMaximum(100);
    cmp_sb_p3_fine_grid->setValue(20);

    label = new QLabel(tr("Coarse Mult"));
    label->setAlignment(Qt::AlignCenter);
    grid->addWidget(label, 4, 3);

    cmp_sb_p3_coarse_mult = new QSpinBox();
    grid->addWidget(cmp_sb_p3_coarse_mult, 4, 4);
    cmp_sb_p3_coarse_mult->setMinimum(1);
    cmp_sb_p3_coarse_mult->setMaximum(100);
    cmp_sb_p3_coarse_mult->setValue(20);

    label = new QLabel(tr("Physical Cells Only"));
    label->setAlignment(Qt::AlignCenter);
    grid->addWidget(label, 5, 0, 1, 4);

    recall_p3();
}


// Sensitivity logic for the properties check boxes.
//
void
QTcompareDlg::p1_sens()
{
    bool b_ok = QTdev::GetStatus(cmp_p1_boxes);
    bool p_ok = QTdev::GetStatus(cmp_p1_polys);
    bool w_ok = QTdev::GetStatus(cmp_p1_wires);
    bool l_ok = QTdev::GetStatus(cmp_p1_labels);
    bool c_ok = QTdev::GetStatus(cmp_p1_insts);

    cmp_p1_boxes_prp->setEnabled(b_ok);
    cmp_p1_polys_prp->setEnabled(p_ok);
    cmp_p1_wires_prp->setEnabled(w_ok);
    cmp_p1_labels_prp->setEnabled(l_ok);
    cmp_p1_insts_prp->setEnabled(c_ok);
}


namespace {
    char *strip_sp(const char *str)
    {
        if (!str)
            return (0);
        while (isspace(*str))
            str++;
        if (!*str)
            return (0);
        bool qtd = false;
        if (*str == '"') {
            str++;
            qtd = true;
        }
        char *sret = lstring::copy(str);
        char *t = sret + strlen(sret) - 1;
        while (isspace(*t) && t >= sret)
            *t-- = 0;
        if (qtd && *t == '"')
            *t = 0;
        if (!*sret) {
            delete [] sret;
            return (0);
        }
        return (sret);
    }
}


char *
QTcompareDlg::compose_arglist()
{
    sLstr lstr;
    char *str = lstring::copy(cmp_fname1->text().toLatin1().constData());
    const char *s = str;

    char *tok = strip_sp(s);
    if (tok) {
        lstr.add(" -f1 \"");
        lstr.add(tok);
        lstr.add_c('"');
        delete [] tok;
    }
    delete [] str;

    str = lstring::copy(cmp_fname2->text().toLatin1().constData());
    s = str;
    tok = strip_sp(s);
    if (tok) {
        lstr.add(" -f2 \"");
        lstr.add(tok);
        lstr.add_c('"');
        delete [] tok;
    }
    delete [] str;

    str = lstring::copy(cmp_cnames1->text().toLatin1().constData());
    s = str;
    tok = strip_sp(s);
    if (tok) {
        lstr.add(" -c1 \"");
        lstr.add(tok);
        lstr.add_c('"');
        delete [] tok;
    }
    delete [] str;

    str = lstring::copy(cmp_cnames2->text().toLatin1().constData());
    s = str;
    tok = strip_sp(s);
    if (tok) {
        lstr.add(" -c2 \"");
        lstr.add(tok);
        lstr.add_c('"');
        delete [] tok;
    }
    delete [] str;

    if (QTdev::GetStatus(cmp_layer_use) ||
            QTdev::GetStatus(cmp_layer_skip)) {
        str = lstring::copy(cmp_layer_list->text().toLatin1().constData());
        s = str;
        tok = strip_sp(s);
        if (tok) {
            lstr.add(" -l \"");
            lstr.add(tok);
            lstr.add_c('"');
            if (QTdev::GetStatus(cmp_layer_skip))
                lstr.add(" -s");
            delete [] tok;
        }
        delete [] str;
    }

    if (QTdev::GetStatus(cmp_diff_only))
        lstr.add(" -d");

    int val = cmp_sb_max_errs->value();
    lstr.add(" -r ");
    lstr.add_i(val);
/*XXX
    if (tok) {
        if (atoi(tok) > 0) {
            lstr.add(" -r ");
            lstr.add(tok);
        }
        delete [] tok;
    }
*/

    int page = cmp_mode->currentIndex();
    if (page == 0) {
        if (QTdev::GetStatus(cmp_p1_recurse))
            lstr.add(" -h");
        if (QTdev::GetStatus(cmp_p1_expand_arrays))
            lstr.add(" -x");
        if (QTdev::GetStatus(cmp_p1_elec))
            lstr.add(" -e");
        if (QTdev::GetStatus(cmp_p1_slop))
            lstr.add(" -b");
        if (QTdev::GetStatus(cmp_p1_dups))
            lstr.add(" -n");

        bool has_b = false;
        bool has_p = false;
        bool has_w = false;
        bool has_l = false;
        bool has_c = false;

        char typ[8];
        int cnt = 0;
        if (QTdev::GetStatus(cmp_p1_boxes)) {
            typ[cnt++] = 'b';
            has_b = true;
        }
        if (QTdev::GetStatus(cmp_p1_polys)) {
            typ[cnt++] = 'p';
            has_p = true;
        }
        if (QTdev::GetStatus(cmp_p1_wires)) {
            typ[cnt++] = 'w';
            has_w = true;
        }
        if (QTdev::GetStatus(cmp_p1_labels)) {
            typ[cnt++] = 'l';
            has_l = true;
        }
        if (QTdev::GetStatus(cmp_p1_insts)) {
            typ[cnt++] = 'c';
            has_c = true;
        }
        typ[cnt] = 0;

        if (typ[0] && strcmp(typ, "bpwc")) {
            lstr.add(" -t ");
            lstr.add(typ);
        }
        else {
            has_b = true;
            has_p = true;
            has_w = true;
            has_c = true;
        }

        cnt = 0;
        if (has_b && QTdev::GetStatus(cmp_p1_boxes_prp))
            typ[cnt++] = 'b';
        if (has_p && QTdev::GetStatus(cmp_p1_polys_prp))
            typ[cnt++] = 'p';
        if (has_w && QTdev::GetStatus(cmp_p1_wires_prp))
            typ[cnt++] = 'w';
        if (has_l && QTdev::GetStatus(cmp_p1_labels_prp))
            typ[cnt++] = 'l';
        if (has_c && QTdev::GetStatus(cmp_p1_insts_prp))
            typ[cnt++] = 'c';
        if (QTdev::GetStatus(cmp_p1_cell_prp))
            typ[cnt++] = 's';
        if (cmp_p1_fltr_mode == PrpFltCstm)
            typ[cnt++] = 'u';
        else if (cmp_p1_fltr_mode == PrpFltNone)
            typ[cnt++] = 'n';

        typ[cnt] = 0;

        if (typ[0]) {
            lstr.add(" -p ");
            lstr.add(typ);
        }
    }
    else if (page == 1) {
        lstr.add(" -g");
        if (QTdev::GetStatus(cmp_p2_recurse))
            lstr.add(" -h");
        if (QTdev::GetStatus(cmp_p2_expand_arrays))
            lstr.add(" -x");
        if (QTdev::GetStatus(cmp_p2_insts))
            lstr.add(" -t c");
    }
    else if (page == 2) {
        lstr.add(" -f");
/*XXX
        if (QTdev::GetStatus(cmp_p3_aoi_use)) {
            s = cmp_sb_p3_aoi_left.get_string();
            char *tokl = lstring::gettok(&s);
            s = cmp_sb_p3_aoi_bottom.get_string();
            char *tokb = lstring::gettok(&s);
            s = cmp_sb_p3_aoi_right.get_string();
            char *tokr = lstring::gettok(&s);
            s = cmp_sb_p3_aoi_top.get_string();
            char *tokt = lstring::gettok(&s);
            if (tokl && tokb && tokr && tokt) {
                lstr.add(" -a ");
                lstr.add(tokl);
                lstr.add_c(',');
                lstr.add(tokb);
                lstr.add_c(',');
                lstr.add(tokr);
                lstr.add_c(',');
                lstr.add(tokt);
            }
            delete [] tokl;
            delete [] tokb;
            delete [] tokr;
            delete [] tokt;
        }

        s = sb_p3_fine_grid.get_string();
        tok = strip_sp(s);
        if (tok) {
            lstr.add(" -i ");
            lstr.add(tok);
            delete [] tok;
        }

        s = sb_p3_coarse_mult.get_string();
        tok = strip_sp(s);
        if (tok) {
            lstr.add(" -m ");
            lstr.add(tok);
            delete [] tok;
        }
*/
    }
    return (lstr.string_trim());
}


bool
QTcompareDlg::get_bb(BBox *BB)
{
    if (!BB)
        return (false);
    double l = cmp_sb_p3_aoi_left->value();
    double b = cmp_sb_p3_aoi_bottom->value();
    double r = cmp_sb_p3_aoi_right->value();
    double t = cmp_sb_p3_aoi_top->value();
/*XXX
    double l, b, r, t;
    int i = 0;
    i += sscanf(sb_p3_aoi_left.get_string(), "%lf", &l);
    i += sscanf(sb_p3_aoi_bottom.get_string(), "%lf", &b);
    i += sscanf(sb_p3_aoi_right.get_string(), "%lf", &r);
    i += sscanf(sb_p3_aoi_top.get_string(), "%lf", &t);
    if (i != 4)
        return (false);
*/
    BB->left = INTERNAL_UNITS(l);
    BB->bottom = INTERNAL_UNITS(b);
    BB->right = INTERNAL_UNITS(r);
    BB->top = INTERNAL_UNITS(t);
    if (BB->right < BB->left) {
        int tmp = BB->left;
        BB->left = BB->right;
        BB->right = tmp;
    }
    if (BB->top < BB->bottom) {
        int tmp = BB->bottom;
        BB->bottom = BB->top;
        BB->top = tmp;
    }
    if (BB->left == BB->right || BB->bottom == BB->top)
        return (false);;
    return (true);
}


void
QTcompareDlg::set_bb(const BBox *BB)
{
    if (!BB)
        return;
    int ndgt = CD()->numDigits();
    cmp_sb_p3_aoi_left->setDecimals(ndgt);
    cmp_sb_p3_aoi_left->setValue(MICRONS(BB->left));
    cmp_sb_p3_aoi_bottom->setDecimals(ndgt);
    cmp_sb_p3_aoi_bottom->setValue(MICRONS(BB->bottom));
    cmp_sb_p3_aoi_right->setDecimals(ndgt);
    cmp_sb_p3_aoi_right->setValue(MICRONS(BB->right));
    cmp_sb_p3_aoi_top->setDecimals(ndgt);
    cmp_sb_p3_aoi_top->setValue(MICRONS(BB->top));
}


void
QTcompareDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:diff"))
}


void
QTcompareDlg::luse_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cmp_layer_skip, false);

    if (QTdev::GetStatus(cmp_layer_use) ||
            QTdev::GetStatus(cmp_layer_skip))
        cmp_layer_list->setEnabled(true);
    else
        cmp_layer_list->setEnabled(false);
}


void
QTcompareDlg::lskip_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cmp_layer_use, false);
    if (QTdev::GetStatus(cmp_layer_use) ||
            QTdev::GetStatus(cmp_layer_skip))
        cmp_layer_list->setEnabled(true);
    else
        cmp_layer_list->setEnabled(false);
}


void
QTcompareDlg::go_btn_slot()
{
    // If the prompt to view output is still active, kill it.
    PL()->AbortEdit();

    char *str = compose_arglist();

    cCompare cmp;
    if (!cmp.parse(str)) {
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        delete [] str;
        return;
    }
    if (!cmp.setup()) {
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        delete [] str;
        return;
    }
    QTpkg::self()->SetWorking(true);
    DFtype df = cmp.compare();
    QTpkg::self()->SetWorking(false);

    if (df == DFabort)
        PL()->ShowPrompt("Comparison aborted.");
    else if (df == DFerror)
        PL()->ShowPromptV("Comparison failed: %s", Errs()->get_error());
    else {
        char buf[256];
        if (df == DFsame) {
            snprintf(buf, sizeof(buf),
                "No differences found, see file \"%s\".",
                DIFF_LOG_FILE);
            PL()->ShowPrompt(buf);
        }
        else {
            snprintf(buf, sizeof(buf),
                "Differences found, data written to "
                "file \"%s\", view file? [y] ", DIFF_LOG_FILE);
            char *in = PL()->EditPrompt(buf, "y");
            in = lstring::strip_space(in);
            if (in && (*in == 'y' || *in == 'Y'))
                DSPmainWbag(PopUpFileBrowser(DIFF_LOG_FILE))
            PL()->ErasePrompt();
        }
    }
    delete [] str;
}


void
QTcompareDlg::dismiss_btn_slot()
{
    Cvt()->PopUpCompare(0, MODE_OFF);
}


void
QTcompareDlg::p1_sens_set_slot(int)
{
    p1_sens();
}


void
QTcompareDlg::p1_mode_btn_slot(bool)
{
    p1_sens();
}


void
QTcompareDlg::p1_fltr_menu_slot(int n)
{
    if (n == 1)
        cmp_p1_fltr_mode = PrpFltNone;
    else if (n == 2)
        cmp_p1_fltr_mode = PrpFltCstm;
    else
        cmp_p1_fltr_mode = PrpFltDflt;
}


void
QTcompareDlg::p1_setup_btn_slot(bool state)
{
    if (state)
        Cvt()->PopUpPropertyFilter(cmp_p1_setup, MODE_ON);
    else
        Cvt()->PopUpPropertyFilter(cmp_p1_setup, MODE_OFF);
}


void
QTcompareDlg::p3_usewin_btn_slot(int state)
{
    if (state) {
        cmp_sb_p3_aoi_left->setEnabled(true);
        cmp_sb_p3_aoi_bottom->setEnabled(true);
        cmp_sb_p3_aoi_right->setEnabled(true);
        cmp_sb_p3_aoi_top->setEnabled(true);
    }
    else {
        cmp_sb_p3_aoi_left->setEnabled(false);
        cmp_sb_p3_aoi_bottom->setEnabled(false);
        cmp_sb_p3_aoi_right->setEnabled(false);
        cmp_sb_p3_aoi_top->setEnabled(false);
    }
}


void
QTcompareDlg::p3_s_menu_slot(QAction *a)
{
    get_bb(FIO()->savedBB(a->data().toInt()));
}


void
QTcompareDlg::p3_r_menu_slot(QAction *a)
{
    set_bb(FIO()->savedBB(a->data().toInt()));
}
// End of QTcompareDlg slots and functions.


QTcompareDlg::sCmp_store::sCmp_store()
{
    cs_file1 = 0;
    cs_file2 = 0;
    cs_cells1 = 0;
    cs_cells2 = 0;
    cs_layers = 0;
    cs_mode = 0;
    cs_layer_only = false;
    cs_layer_skip = false;
    cs_differ = false;
    cs_max_errs = 0;

    cs_p1_recurse = false;
    cs_p1_expand_arrays = false;
    cs_p1_slop = false;
    cs_p1_dups = false;
    cs_p1_boxes = true;
    cs_p1_polys = true;
    cs_p1_wires = true;
    cs_p1_labels = false;
    cs_p1_insts = true;
    cs_p1_boxes_prp = false;
    cs_p1_polys_prp = false;
    cs_p1_wires_prp = false;
    cs_p1_labels_prp = false;
    cs_p1_insts_prp = false;
    cs_p1_elec = false;
    cs_p1_cell_prp = false;
    cs_p1_fltr = 0;

    cs_p2_recurse = false;
    cs_p2_expand_arrays = false;
    cs_p2_insts = false;

    cs_p3_use_window = false;
    cs_p3_left = 0.0;
    cs_p3_bottom = 0.0;
    cs_p3_right = 0.0;
    cs_p3_top = 0.0;
    cs_p3_fine_grid = 20;
    cs_p3_coarse_mult = 20;
}


QTcompareDlg::sCmp_store::~sCmp_store()
{
    delete [] cs_file1;
    delete [] cs_file2;
    delete [] cs_cells1;
    delete [] cs_cells2;
    delete [] cs_layers;
}
// End of sCmp_store functions.

