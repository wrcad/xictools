
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
        text_edit(QWidget *prnt) : QTextEdit(prnt)
        {
            QSizePolicy policy = sizePolicy();
            policy.setVerticalPolicy(QSizePolicy::Preferred);
            setSizePolicy(policy);
        }

        QSize sizeHint() const { return (QSize(200, 50)); }
    };
}

QTnumPopup::QTnumPopup(qt_bag *owner, const char *prompt_str, double initd,
    double mind, double maxd, double del, int numd, void *arg) :
    QDialog(owner ? owner->shell : 0)
{
    p_parent = owner;
    p_cb_arg = arg;
    pw_affirmed = false;

    if (owner)
        owner->monitor.add(this);

    setWindowTitle(QString(tr("Numeric Entry")));

    QSizePolicy policy = sizePolicy();
    policy.setVerticalPolicy(QSizePolicy::Preferred);
    setSizePolicy(policy);

    label = new text_edit(this);
    label->setPlainText(QString(prompt_str));
    label->setReadOnly(true);

    spinbtn = new QDoubleSpinBox(this);
    spinbtn->setMinimum(mind);
    spinbtn->setMaximum(maxd);
    spinbtn->setDecimals(numd);
    spinbtn->setValue(initd);
    spinbtn->setSingleStep(del);
    spinbtn->setMaximumWidth(100);
    spinbtn->setMinimumWidth(100);

    yesbtn = new QPushButton(this);
    yesbtn->setText(QString(tr("Apply")));
    nobtn = new QPushButton(this);
    nobtn->setText(QString(tr("Dismiss")));
    yesbtn->setDefault(true);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(4);
    vbox->setSpacing(2);
    vbox->addWidget(label);
    vbox->addWidget(spinbtn);
    vbox->setAlignment(spinbtn, Qt::AlignCenter);
    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setSpacing(2);
    hbox->addWidget(yesbtn);
    hbox->addWidget(nobtn);
    vbox->addLayout(hbox);

    connect(yesbtn, SIGNAL(clicked()), this, SLOT(action_slot()));
    connect(nobtn, SIGNAL(clicked()), this, SLOT(quit_slot()));
}


QTnumPopup::~QTnumPopup()
{
    if (p_callback)
        (*p_callback)(spinbtn->value(), pw_affirmed, p_cb_arg);
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
        qt_bag *owner = dynamic_cast<qt_bag*>(p_parent);
        if (owner)
            owner->monitor.remove(this);
    }
}


// GRpopup override
// Register the calling button, and set up:
//  1.  whether or not the caller is deselected on popdwon.
//  2.  whether or not deselecting the caller causes popdown.
//
void
QTnumPopup::register_caller(GRobject c, bool no_dsl, bool handle_popdn)
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
QTnumPopup::popdown()
{
    if (p_parent) {
        qt_bag *owner = dynamic_cast<qt_bag*>(p_parent);
        if (!owner || !owner->monitor.is_active(this))
            return;
    }
    delete this;
}


void
QTnumPopup::action_slot()
{
    pw_affirmed = true;
    emit affirm(spinbtn->value(), p_cb_arg);
    delete this;
}


void
QTnumPopup::quit_slot()
{
    delete this;
}

