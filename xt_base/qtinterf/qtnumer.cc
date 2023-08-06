
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
#include "qtnumer.h"

#include <QAction>
#include <QDoubleSpinBox>
#include <QLayout>
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

QTnumDlg::QTnumDlg(QTbag *owner, const char *prompt_str, double initd,
    double mind, double maxd, double del, int numd, void *arg) :
    QDialog(owner ? owner->Shell() : 0)
{
    p_parent = owner;
    p_cb_arg = arg;
    nu_affirmed = false;

    if (owner)
        owner->MonitorAdd(this);

    setWindowTitle(QString(tr("Numeric Entry")));
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::WindowStaysOnTopHint);

    QSizePolicy policy = sizePolicy();
    policy.setVerticalPolicy(QSizePolicy::Preferred);
    setSizePolicy(policy);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    nu_label = new text_edit();
    nu_label->setReadOnly(true);
    nu_label->setPlainText(tr(prompt_str));
    vbox->addWidget(nu_label);

    nu_spinbtn = new QDoubleSpinBox(this);
    nu_spinbtn->setRange(mind, maxd);
    nu_spinbtn->setDecimals(numd);
    nu_spinbtn->setValue(initd);
    nu_spinbtn->setSingleStep(del);
    vbox->addWidget(nu_spinbtn);
    vbox->setAlignment(nu_spinbtn, Qt::AlignCenter);

    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setMargin(0);
    hbox->setSpacing(2);

    nu_yesbtn = new QPushButton(tr("Apply"));
    nu_yesbtn->setDefault(true);
    hbox->addWidget(nu_yesbtn);
    connect(nu_yesbtn, SIGNAL(clicked()), this, SLOT(action_slot()));

    nu_nobtn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(nu_nobtn);
    connect(nu_nobtn, SIGNAL(clicked()), this, SLOT(quit_slot()));
}


QTnumDlg::~QTnumDlg()
{
    if (p_callback)
        (*p_callback)(nu_spinbtn->value(), nu_affirmed, p_cb_arg);
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller && !p_no_desel) {
        QObject *o = (QObject*)p_caller;
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn)
                btn->setChecked(false);
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a)
                a->setChecked(false);
        }
    }
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }
}


// GRpopup override
// Register the calling button, and set up:
//  1.  whether or not the caller is deselected on popdwon.
//  2.  whether or not deselecting the caller causes popdown.
//
void
QTnumDlg::register_caller(GRobject c, bool no_dsl, bool handle_popdn)
{
    p_caller = c;
    p_no_desel = no_dsl;
    if (handle_popdn) {
        QObject *o = (QObject*)c;
        if (o) {
            if (o->isWidgetType()) {
                QPushButton *btn = dynamic_cast<QPushButton*>(o);
                if (btn)
                    connect(btn, SIGNAL(clicked()),
                        this, SLOT(quit_slot()));
            }
            else {
                QAction *a = dynamic_cast<QAction*>(o);
                if (a)
                    connect(a, SIGNAL(triggered()),
                        this, SLOT(quit_slot()));
            }
        }
    }
}


// GRpopup override
//
void
QTnumDlg::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


void
QTnumDlg::action_slot()
{
    nu_affirmed = true;
    emit affirm(nu_spinbtn->value(), p_cb_arg);
    delete this;
}


void
QTnumDlg::quit_slot()
{
    deleteLater();
}

