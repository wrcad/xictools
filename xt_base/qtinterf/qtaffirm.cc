
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
#include "qtaffirm.h"

#include <QApplication>
#include <QAction>
#include <QLayout>
#include <QToolButton>
#include <QPushButton>
#include <QTextEdit>


namespace qtinterf
{
    // Change the default sizing behavior.  Would like a single line
    // that is scrollable.
    //
    class text_edit : public QTextEdit
    {
    public:
        text_edit(QWidget *prnt = 0) : QTextEdit(prnt)
        {
            QSizePolicy policy = sizePolicy();
            policy.setVerticalPolicy(QSizePolicy::Preferred);
            setSizePolicy(policy);
        }

        QSize sizeHint() const { return (QSize(200, 50)); }
    };
}


QTaffirmDlg::QTaffirmDlg(QTbag *owner, const char *question_str)
{
    p_parent = owner;
    af_affirmed = false;

    if (owner)
        owner->MonitorAdd(this);

    setWindowTitle(QString(tr("Affirm?")));
    setWindowFlags(Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);

    QSizePolicy policy = sizePolicy();
    policy.setVerticalPolicy(QSizePolicy::Preferred);
    setSizePolicy(policy);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(2, 2, 2, 2);
    vbox->setSpacing(2);

    text_edit *tarea = new text_edit();
    vbox->addWidget(tarea);
    tarea->setReadOnly(true);
    tarea->setPlainText(question_str);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    vbox->addLayout(hbox);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(2);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Affirm"));
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(affirm_btn_slot()));

    QPushButton *btn = new QPushButton(tr("Cancel"));
    btn->setObjectName("Default");
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(cancel_btn_slot()));
}


QTaffirmDlg::~QTaffirmDlg()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_callback)
        (*p_callback)(af_affirmed, p_cb_arg);
    if (p_caller && !p_no_desel)
        QTdev::Deselect(p_caller);
    // OK to emit a signal and die? Seems so.
    emit affirm(af_affirmed, p_cb_arg);
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTaffirmDlg
#include "qtmacos_event.h"
#endif


// GRpopup override
// Register the calling button, and set up:
//  1.  whether or not the caller is deselected on popdwon.
//  2.  whether or not deselecting the caller causes popdown.
//
void
QTaffirmDlg::register_caller(GRobject c, bool no_dsl, bool handle_popdn)
{
    p_caller = c;
    p_no_desel = no_dsl;
    if (handle_popdn) {
        QObject *o = (QObject*)c;
        if (o) {
            if (o->isWidgetType()) {
                QAbstractButton *btn = dynamic_cast<QAbstractButton*>(o);
                if (btn) {
                    if (btn->isCheckable()) {
                        connect(btn, SIGNAL(toggled(bool)),
                            this, SLOT(cancel_action_slot(bool)));
                    }
                    else {
                        connect(btn, SIGNAL(clicked()),
                            this, SLOT(cancel_btn_slot()));
                    }
                }
            }
            else {
                QAction *a = dynamic_cast<QAction*>(o);
                if (a) {
                    if (a->isCheckable()) {
                        connect(a, SIGNAL(triggered(bool)),
                            this, SLOT(cancel_action_slot(bool)));
                    }
                    else {
                        connect(a, SIGNAL(triggered()),
                            this, SLOT(cancel_btn_slot()));
                    }
                }
            }
        }
    }
}


// GRpopup override
//
void
QTaffirmDlg::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


void
QTaffirmDlg::affirm_btn_slot()
{
    af_affirmed = true;
    delete this;
}


void
QTaffirmDlg::cancel_btn_slot()
{
    delete this;
}


void
QTaffirmDlg::cancel_action_slot(bool state)
{
    if (!state)
        delete this;
}

