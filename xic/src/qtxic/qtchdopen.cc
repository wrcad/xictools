
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

#include "qtchdopen.h"
#include "cvrt.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_chd.h"
#include "fio_alias.h"
#include "qtcnmap.h"
#include "qtinterf/qtfont.h"

#include <QApplication>
#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>


//-----------------------------------------------------------------------------
// QTchdOpenDlg:  Dialog to create a cell hierarchy digest (CHD).
// Called from the Cell Hierarchy Digests list dialog (QRchdListDlg).
//
// Help system keywords used:
//  xic:chdadd

void
cConvert::PopUpChdOpen(GRobject caller, ShowMode mode,
    const char *init_idname, const char *init_str, int x, int y,
    bool(*callback)(const char*, const char*, int, void*), void *arg)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTchdOpenDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTchdOpenDlg::self())
            QTchdOpenDlg::self()->update(init_idname, init_str);
        return;
    }
    if (QTchdOpenDlg::self())
        return;

    new QTchdOpenDlg(caller, callback, arg, init_idname, init_str);

    QDialog *prnt = QTdev::DlgOf(caller);
    if (!prnt)
        prnt = QTmainwin::self();
    QTchdOpenDlg::self()->set_transient_for(prnt);
    QTdev::self()->SetPopupLocation(GRloc(LW_XYA, x, y), QTchdOpenDlg::self(),
        prnt);
    QTchdOpenDlg::self()->show();
}
// End of cConvert functions.


class QTchdOpenPathEdit : public QLineEdit
{
public:
    QTchdOpenPathEdit(QWidget *prnt = 0) : QLineEdit(prnt) { }

    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
};


void
QTchdOpenPathEdit::dragEnterEvent(QDragEnterEvent *ev)
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
QTchdOpenPathEdit::dropEvent(QDropEvent *ev)
{
    if (ev->mimeData()->hasUrls()) {
        QByteArray ba = ev->mimeData()->data("text/plain");
        setText(ba.constData());
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


QTchdOpenDlg *QTchdOpenDlg::instPtr;

QTchdOpenDlg::QTchdOpenDlg(GRobject caller,
    bool(*callback)(const char*, const char*, int, void*),
    void *arg, const char *init_idname, const char *init_str) : QTbag(this)
{
    instPtr = this;
    co_caller = caller;
    co_nbook = 0;
    co_p1_text = 0;
    co_p1_info = 0;
    co_p2_text = 0;
    co_p2_mem = 0;
    co_p2_file = 0;
    co_p2_none = 0;
    co_idname = 0;
    co_apply = 0;
    co_p1_cnmap = 0;
    co_callback = callback;
    co_arg = arg;

    setWindowTitle("Open Cell Hierarchy Digest");
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

    // Label in frame plus help button.
    //
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    QLabel *label = new QLabel( tr(
        "Enter path to layout or saved digest file"));
    hb->addWidget(label);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTchdOpenDlg::help_btn_slot);

    co_nbook = new QTabWidget();
    vbox->addWidget(co_nbook);

    // Layout file page.
    //
    QWidget *page = new QWidget();
    QVBoxLayout *p_vbox = new QVBoxLayout(page);
    p_vbox->setContentsMargins(qmtop);
    p_vbox->setSpacing(2);

    co_p1_text = new QTchdOpenPathEdit();
    p_vbox->addWidget(co_p1_text);
    co_p1_text->setReadOnly(false);
    co_p1_text->setAcceptDrops(true);

    co_p1_cnmap = new QTcnameMap(false);
    p_vbox->addWidget(co_p1_cnmap);

    QHBoxLayout *p_hbox = new QHBoxLayout();
    p_vbox->addLayout(p_hbox);
    p_hbox->setContentsMargins(qm);
    p_hbox->setSpacing(2);

    // Info options
    label = new QLabel(tr("Geometry Counts"));
    p_hbox->addWidget(label);

    co_p1_info = new QComboBox();
    p_hbox->addWidget(co_p1_info);
    co_p1_info->addItem(tr("no geometry info saved"));
    co_p1_info->addItem(tr("totals only"));
    co_p1_info->addItem(tr("per-layer counts"));
    co_p1_info->addItem(tr("per-cell counts"));
    co_p1_info->addItem(tr("per-cell and per-layer counts"));
    co_p1_info->setCurrentIndex(FIO()->CvtInfo());
    connect(co_p1_info, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &QTchdOpenDlg::p1_info_slot);

    co_nbook->addTab(page, tr("layout file"));

    // CHD file page.
    //
    page = new QWidget();
    p_vbox = new QVBoxLayout(page);
    p_vbox->setContentsMargins(qmtop);
    p_vbox->setSpacing(2);

    co_p2_text = new QTchdOpenPathEdit();
    p_vbox->addWidget(co_p2_text);
    co_p2_text->setReadOnly(false);
    co_p2_text->setAcceptDrops(true);

    p_vbox->addSpacing(20);

    label = new QLabel(tr("Handle geometry records in saved CHD file:"));
    p_vbox->addWidget(label);

    co_p2_mem = new QRadioButton(tr(
        "Read geometry records into new MEMORY CGD"));
    p_vbox->addWidget(co_p2_mem);

    co_p2_file = new QRadioButton(tr(
        "Read geometry records into new FILE CGD"));
    p_vbox->addWidget(co_p2_file);

    co_p2_none = new QRadioButton(tr("Ignore geometry records"));
    p_vbox->addWidget(co_p2_none);

    switch (sCHDin::get_default_cgd_type()) {
    case CHD_CGDmemory:
        QTdev::SetStatus(co_p2_mem, true);
        QTdev::SetStatus(co_p2_file, false);
        QTdev::SetStatus(co_p2_none, false);
        break;
    case CHD_CGDfile:
        QTdev::SetStatus(co_p2_mem, false);
        QTdev::SetStatus(co_p2_file, true);
        QTdev::SetStatus(co_p2_none, false);
        break;
    case CHD_CGDnone:
        QTdev::SetStatus(co_p2_mem, false);
        QTdev::SetStatus(co_p2_file, false);
        QTdev::SetStatus(co_p2_none, true);
        break;
    }
    p_vbox->addStretch(1);

    co_nbook->addTab(page, tr("CHD file"));

    // below pages
    //
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    label = new QLabel(tr("Access name:"));
    hbox->addWidget(label);

    co_idname = new QLineEdit();
    co_idname->setReadOnly(false);
    hbox->addWidget(co_idname);

    // Apply/Dismiss buttons
    //
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    co_apply = new QToolButton();
    co_apply->setText(tr("Apply"));
    hbox->addWidget(co_apply);
    connect(co_apply, &QAbstractButton::clicked,
        this, &QTchdOpenDlg::apply_btn_slot);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    hbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QTchdOpenDlg::dismiss_btn_slot);

    update(init_idname, init_str);
}


QTchdOpenDlg::~QTchdOpenDlg()
{
    instPtr = 0;
    if (co_caller)
        QTdev::Deselect(co_caller);
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTchdOpenDlg
#include "qtinterf/qtmacos_event.h"
#endif


void
QTchdOpenDlg::update(const char *init_idname, const char *init_str)
{
    if (init_str) {
        int pg = co_nbook->currentIndex();
        if (pg == 0)
            co_p1_text->setText(init_str);
        else
            co_p2_text->setText(init_str);
    }
    if (init_idname)
        co_idname->setText(init_idname);

    // Since the enum is defined elsewhere, don't assume that the
    // values are the same as the menu button order.
    switch (FIO()->CvtInfo()) {
    case cvINFOnone:
        co_p1_info->setCurrentIndex(0);
        break;
    case cvINFOtotals:
        co_p1_info->setCurrentIndex(1);
        break;
    case cvINFOpl:
        co_p1_info->setCurrentIndex(2);
        break;
    case cvINFOpc:
        co_p1_info->setCurrentIndex(3);
        break;
    case cvINFOplpc:
        co_p1_info->setCurrentIndex(4);
        break;
    }
    co_p1_cnmap->update();
}


void
QTchdOpenDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:chdadd"))
}


void
QTchdOpenDlg::p1_info_slot(int tp)
{
    cvINFO cv;
    switch (tp) {
    case cvINFOnone:
        cv = cvINFOnone;
        break;
    case cvINFOtotals:
        cv = cvINFOtotals;
        break;
    case cvINFOpl:
        cv = cvINFOpl;
        break;
    case cvINFOpc:
        cv = cvINFOpc;
        break;
    case cvINFOplpc:
        cv = cvINFOplpc;
        break;
    default:
        return;
    }
    if (cv != FIO()->CvtInfo()) {
        FIO()->SetCvtInfo(cv);
        Cvt()->PopUpConvert(0, MODE_UPD, 0, 0, 0);
    }
}


void
QTchdOpenDlg::apply_btn_slot()
{
    // Pop down and destroy, call callback.  Note that if the callback
    // returns nonzero but not one, the caller is not deselected.
    int ret = true;
    if (co_callback) {
        QTdev::Deselect(co_apply);
        QString string;
        int m = 0;
        int pg = co_nbook->currentIndex();
        if (pg == 0)
            string = co_p1_text->text();
        else {
            string = co_p2_text->text();
            if (QTdev::GetStatus(co_p2_file))
                m = 1;
            else if (QTdev::GetStatus(co_p2_none))
                m = 2;
        }
        if (string.isNull() || string.isEmpty()) {
            PopUpMessage("No input source entered.", true);
            return;
        }
        QString idname = co_idname->text();
        const char *str = lstring::copy(string.toLatin1().constData());
        const char *idn = lstring::copy(idname.toLatin1().constData());
        ret = (*co_callback)(idn, str, m, co_arg);
        delete [] str;
        delete [] idn;
    }
    if (ret)
        delete this;
}


void
QTchdOpenDlg::dismiss_btn_slot()
{
    delete this;
}

