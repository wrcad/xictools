
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

#include "qtasm.h"
#include "cvrt.h"
#include "qtinterf/qtactivity.h"

#include <QApplication>
#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>


//-----------------------------------------------------------------------------
// QTasmPrgDlg:: Progress monitor dialog for the QTasmDlg.

QTasmPrgDlg *QTasmPrgDlg::instPtr;

QTasmPrgDlg::QTasmPrgDlg()
{
    instPtr = this;
    prg_inp_label = 0;
    prg_out_label = 0;
    prg_info_label = 0;
    prg_cname_label = 0;
    prg_pbar = 0;
    prg_refptr = 0;
    prg_abort = false;

    setWindowTitle(tr("Progress"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QGridLayout *grid = new QGridLayout(this);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    QGroupBox *gb = new QGroupBox(tr("Input"));
    grid->addWidget(gb, 0, 0);
    QHBoxLayout *hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    prg_inp_label = new QLabel("");
    hbox->addWidget(prg_inp_label);

    gb = new QGroupBox(tr("Output"));
    grid->addWidget(gb, 0, 1);
    hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    prg_out_label = new QLabel("");
    hbox->addWidget(prg_out_label);

    gb = new QGroupBox(tr("Info"));
    grid->addWidget(gb, 1, 0, 1, 2);
    hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    prg_info_label = new QLabel("");
    hbox->addWidget(prg_info_label);

    gb = new QGroupBox();
    grid->addWidget(gb, 2, 0, 1, 2);
    hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    prg_cname_label = new QLabel("");
    hbox->addWidget(prg_cname_label);

    hbox = new QHBoxLayout(0);
    grid->addLayout(hbox, 3, 0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Abort"));
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(abort_btn_slot()));

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Dismiss");
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    prg_pbar = new QTactivity(this);
    grid->addWidget(prg_pbar, 4, 0, 1, 2);
}


QTasmPrgDlg::~QTasmPrgDlg()
{
    instPtr = 0;
    if (prg_refptr)
        *prg_refptr = 0;
}


#ifdef Q_OS_MACOS

bool
QTasmPrgDlg::event(QEvent *ev)
{
    // Fix for QT BUG 116674, text becomes invisible on autodefault
    // button when the main window has focus.

    if (ev->type() == QEvent::ActivationChange) {
        QPushButton *dsm = findChild<QPushButton*>("Dismiss",
            Qt::FindDirectChildrenOnly);
        if (dsm) {
            QWidget *top = this;
            while (top->parentWidget())
                top = top->parentWidget();
            if (QApplication::activeWindow() == top)
                dsm->setDefault(false);
            else if (QApplication::activeWindow() == this)
                dsm->setDefault(true);
        }
    }
    return (QDialog::event(ev));
}

#endif


void
QTasmPrgDlg::update(const char *msg, ASMcode code)
{
    char *str = lstring::copy(msg);
    char *s = str + strlen(str) - 1;
    while (s >= str && isspace(*s))
        *s-- = 0;
    if (code == ASM_INFO)
        prg_info_label->setText(str);
    else if (code == ASM_READ)
        prg_inp_label->setText(str);
    else if (code == ASM_WRITE)
        prg_out_label->setText(str);
    else if (code == ASM_CNAME)
        prg_cname_label->setText(str);
    delete [] str;
}


void
QTasmPrgDlg::abort_btn_slot()
{
    prg_abort = true;
}


void
QTasmPrgDlg::dismiss_btn_slot()
{
    delete this;
}

