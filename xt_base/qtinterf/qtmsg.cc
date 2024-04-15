
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qtinterf.h"
#include "qtmsg.h"
#include "qtfont.h"

#include <QApplication>
#include <QAction>
#include <QGroupBox>
#include <QLayout>
#include <QTextEdit>
#include <QPushButton>


QTmsgDlg::QTmsgDlg(QTbag *owner, const char *message_str,
    bool err, STYtype sty) : QDialog(owner ? owner->Shell() : 0)
{
    p_parent = owner;
    tx_display_style = sty;
    tx_desens = false;

    if (owner)
        owner->MonitorAdd(this);
    setWindowTitle(err ? tr("ERROR") : tr("Message"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QGroupBox *gb = new QGroupBox();
    vbox->addWidget(gb);
    QVBoxLayout *vb = new QVBoxLayout(gb);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    tx_tbox = new QTextEdit();
    tx_tbox->setReadOnly(true);
    vb->addWidget(tx_tbox);

    if (sty == STY_FIXED) {
        QFont *f;
        if (Fnt()->getFont(&f, FNT_FIXED)) {
            tx_tbox->setCurrentFont(*f);
            tx_tbox->setFont(*f);
        }
    }
    setText(message_str);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Dismiss");
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));
}


QTmsgDlg::~QTmsgDlg()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (owner)
            owner->ClearPopup(this);
    }
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        QTdev::Deselect(p_caller);
}


#ifdef Q_OS_MACOS

bool
QTmsgDlg::event(QEvent *ev)
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


QSize
QTmsgDlg::sizeHint() const
{
    return (QSize(400, 120));
}


// GRpopup override
//
void
QTmsgDlg::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


void
QTmsgDlg::setTitle(const char *title)
{
    setWindowTitle(title);
}


void
QTmsgDlg::setText(const char *message_str)
{
    if (tx_display_style == STY_HTML)
        tx_tbox->setHtml(message_str);
    else
        tx_tbox->setPlainText(message_str);
}


void
QTmsgDlg::dismiss_btn_slot()
{
    delete this;
}

