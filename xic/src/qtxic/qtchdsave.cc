
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

#include "qtchdsave.h"
#include "cvrt.h"
#include "dsp_inlines.h"
#include "qtllist.h"

#include <QApplication>
#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QToolButton>
#include <QPushButton>
#include <QLineEdit>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>


//-----------------------------------------------------------------------------
// QTchdSaveDlg: Dialog to save a cell hierarchy digest (CHD) file.
// Called from the Cell Hierarchy Digests list dialog (QTchdListDlg).
//
// Help system keywords used:
//  xic:chdsav

void
cConvert::PopUpChdSave(GRobject caller, ShowMode mode,
    const char *chdname, int x, int y,
    bool(*callback)(const char*, bool, void*), void *arg)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTchdSaveDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTchdSaveDlg::self())
            QTchdSaveDlg::self()->update(chdname);
        return;
    }
    if (QTchdSaveDlg::self())
        return;

    new QTchdSaveDlg(caller, callback, arg, chdname);

    QDialog *prnt = QTdev::DlgOf(caller);
    if (!prnt)
        prnt = QTmainwin::self();
    QTchdSaveDlg::self()->set_transient_for(prnt);
    QTdev::self()->SetPopupLocation(GRloc(LW_XYA, x, y), QTchdSaveDlg::self(),
        prnt);
    QTchdSaveDlg::self()->show();
}
// End of cConvert functions.


class QTchdSavePathEdit : public QLineEdit
{
public:
    QTchdSavePathEdit(QWidget *prnt = 0) : QLineEdit(prnt) { }

    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
};


void
QTchdSavePathEdit::dragEnterEvent(QDragEnterEvent *ev)
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
QTchdSavePathEdit::dropEvent(QDropEvent *ev)
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


QTchdSaveDlg *QTchdSaveDlg::instPtr;

QTchdSaveDlg::QTchdSaveDlg(GRobject caller,
    bool(*callback)(const char*, bool, void*), void *arg, const char *chdname)
{
    instPtr = this;
    cs_caller = caller;
    cs_label = 0;
    cs_text = 0;
    cs_geom = 0;
    cs_apply = 0;
    cs_llist = 0;
    cs_callback = callback;
    cs_arg = arg;

    setWindowTitle(tr("Save Hierarchy Digest File"));
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
    char buf[256];
    snprintf(buf, sizeof(buf), "Saving %s, enter pathname for file:",
        chdname ? chdname : "");
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    cs_label = new QLabel(tr(buf));
    hb->addWidget(cs_label);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTchdSaveDlg::help_btn_slot);

    cs_text = new QTchdSavePathEdit();
    vbox->addWidget(cs_text);
    cs_text->setReadOnly(false);
    cs_text->setAcceptDrops(true);

#if QT_VERSION >= QT_VERSION_CHECK(6,8,0)
#define CHECK_BOX_STATE_CHANGED &QCheckBox::checkStateChanged
#else
#define CHECK_BOX_STATE_CHANGED &QCheckBox::stateChanged
#endif

    cs_geom = new QCheckBox(tr("Include geometry records in file"));
    vbox->addWidget(cs_geom);
    connect(cs_geom, CHECK_BOX_STATE_CHANGED,
        this, &QTchdSaveDlg::geom_btn_slot);

    // Layer list
    //
    cs_llist = new QTlayerList;
    cs_llist->setEnabled(false);
    vbox->addWidget(cs_llist);

    // Apply/Dismiss buttons
    //
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    cs_apply = new QToolButton();
    cs_apply->setText(tr("Apply"));
    hbox->addWidget(cs_apply);
    connect(cs_apply, &QAbstractButton::clicked,
        this, &QTchdSaveDlg::apply_btn_slot);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    hbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QTchdSaveDlg::dismiss_btn_slot);
}


QTchdSaveDlg::~QTchdSaveDlg()
{
    instPtr = 0;
    if (cs_caller)
        QTdev::Deselect(cs_caller);
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTchdSaveDlg
#include "qtinterf/qtmacos_event.h"
#endif


void
QTchdSaveDlg::update(const char *chdname)
{
    if (!chdname)
        return;
    char buf[256];
    snprintf(buf, sizeof(buf), "Saving %s, enter pathname for file:",
        chdname);
    cs_label->setText(buf);
    cs_llist->update();
}


void
QTchdSaveDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:chdsav"))
}


void
QTchdSaveDlg::geom_btn_slot(int state)
{
    cs_llist->setEnabled(state);
}


void
QTchdSaveDlg::apply_btn_slot()
{
    int ret = true;
    if (cs_callback) {
        const char *string =
            lstring::copy(cs_text->text().toLatin1().constData());
        ret = (*cs_callback)(string, QTdev::GetStatus(cs_geom),
            cs_arg);
        delete [] string;
    }
    if (ret)
        delete this;
}


void
QTchdSaveDlg::dismiss_btn_slot()
{
    Cvt()->PopUpChdSave(0, MODE_OFF, 0, 0, 0, 0, 0);
}

