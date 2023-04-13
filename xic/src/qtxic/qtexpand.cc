
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

#include "qtmain.h"
#include "qtexpand.h"
#include "dsp_inlines.h"

#include <QAction>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>


expand_d::expand_d(qt_bag *owner, const char *string, bool nopeek,
    void *arg) : QDialog(owner ? owner->shell : 0)
{
    p_parent = owner;
    p_cb_arg = arg;
    p_callback = 0;

    if (owner)
        owner->monitor.add(this);

    setWindowTitle(QString(tr("Expand")));

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);

    QGroupBox *gb = new QGroupBox(this);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setMargin(2);
    label = new QLabel(gb);
    label->setText(QString(tr("Set Expansion Control String")));
    hb->addWidget(label);
    hbox->addWidget(gb);

    b_help = new QPushButton(this);
    b_help->setText(QString(tr("Help")));
    b_help->setAutoDefault(false);
    b_help->setMaximumWidth(70);
    connect(b_help, SIGNAL(clicked()), this, SLOT(help_slot()));

    hbox->addWidget(b_help);
    vbox->addLayout(hbox);

    hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    
    edit = new QLineEdit(this);
    edit->setText(QString(string));
    edit->setMaximumWidth(60);
    hbox->addWidget(edit);
    b_plus = new QPushButton(this);
    b_plus->setText(QString("+"));
    b_plus->setAutoDefault(false);
    connect(b_plus, SIGNAL(clicked()), this, SLOT(plus_slot()));
    hbox->addWidget(b_plus);

    b_minus = new QPushButton(this);
    b_minus->setText(QString("-"));
    b_minus->setAutoDefault(false);
    connect(b_minus, SIGNAL(clicked()), this, SLOT(minus_slot()));
    hbox->addWidget(b_minus);

    b_all = new QPushButton(this);
    b_all->setText(QString(tr("all")));
    b_all->setAutoDefault(false);
    connect(b_all, SIGNAL(clicked()), this, SLOT(all_slot()));
    hbox->addWidget(b_all);

    b_0 = new QPushButton(this);
    b_0->setText(QString("0"));
    b_0->setAutoDefault(false);
    connect(b_0, SIGNAL(clicked()), this, SLOT(b0_slot()));
    hbox->addWidget(b_0);

    b_1 = new QPushButton(this);
    b_1->setText(QString("1"));
    b_1->setAutoDefault(false);
    connect(b_1, SIGNAL(clicked()), this, SLOT(b1_slot()));
    hbox->addWidget(b_1);

    b_2 = new QPushButton(this);
    b_2->setText(QString("2"));
    b_2->setAutoDefault(false);
    connect(b_2, SIGNAL(clicked()), this, SLOT(b2_slot()));
    hbox->addWidget(b_2);

    b_3 = new QPushButton(this);
    b_3->setText(QString("3"));
    b_3->setAutoDefault(false);
    connect(b_3, SIGNAL(clicked()), this, SLOT(b3_slot()));
    hbox->addWidget(b_3);

    b_4 = new QPushButton(this);
    b_4->setText(QString("4"));
    b_4->setAutoDefault(false);
    connect(b_4, SIGNAL(clicked()), this, SLOT(b4_slot()));
    hbox->addWidget(b_4);

    b_5 = new QPushButton(this);
    b_5->setText(QString("5"));
    b_5->setAutoDefault(false);
    connect(b_5, SIGNAL(clicked()), this, SLOT(b5_slot()));
    hbox->addWidget(b_5);

    if (!nopeek) {
        b_peek = new QPushButton(this);
        b_peek->setText(QString(tr("Peek Mode")));
        b_peek->setAutoDefault(false);
        hbox->addWidget(b_peek);
        connect(b_peek, SIGNAL(clicked()), this, SLOT(peek_slot()));
    }
    else
        b_peek = 0;
    vbox->addLayout(hbox);

    hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);

    b_apply = new QPushButton(this);
    b_apply->setText(QString(tr("Apply")));
    b_apply->setAutoDefault(true);
    connect(b_apply, SIGNAL(clicked()), this, SLOT(apply_slot()));
    hbox->addWidget(b_apply);

    b_dismiss = new QPushButton(this);
    b_dismiss->setText(QString(tr("Dismiss")));
    b_dismiss->setAutoDefault(false);
    connect(b_dismiss, SIGNAL(clicked()), this, SLOT(dismiss_slot()));
    hbox->addWidget(b_dismiss);
    vbox->addLayout(hbox);
}


expand_d::~expand_d()
{
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller && p_no_desel) {
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
        subwin_d *owner = dynamic_cast<subwin_d*>(p_parent);
        if (owner) {
            owner->monitor.remove(this);
            owner->expand = 0;
        }
    }
    if (p_callback)
        (*p_callback)(0, p_cb_arg);
}


// GRpopup override
//
void
expand_d::popdown()
{
    if (!p_parent)
        return;
    qt_bag *owner = dynamic_cast<qt_bag*>(p_parent);
    if (!owner || !owner->monitor.is_active(this))
        return;

    delete this;
}


void
expand_d::update(const char *string)
{
    edit->setText(QString(string));
}


void
expand_d::help_slot()
{
    DSPmainWbag(PopUpHelp("xic:expnd"))
}


void
expand_d::plus_slot()
{
    QString qs = edit->text();
    if (!qs.isNull()) {
        const char *t = qs.toLatin1().constData();
        if (*t == '+') {
            qs = qs + QString("+");
            edit->setText(qs);
            return;
        }
    }
    edit->setText(QString("+"));
}


void
expand_d::minus_slot()
{
    QString qs = edit->text();
    if (!qs.isNull()) {
        const char *t = qs.toLatin1().constData();
        if (*t == '-') {
            qs = qs + QString("-");
            edit->setText(qs);
            return;
        }
    }
    edit->setText(QString("-"));
}


void
expand_d::all_slot()
{
    edit->setText(QString(tr("all")));
}


void
expand_d::b0_slot()
{
    edit->setText(QString("0"));
}


void
expand_d::b1_slot()
{
    edit->setText(QString("1"));
}


void
expand_d::b2_slot()
{
    edit->setText(QString("2"));
}


void
expand_d::b3_slot()
{
    edit->setText(QString("3"));
}


void
expand_d::b4_slot()
{
    edit->setText(QString("4"));
}


void
expand_d::b5_slot()
{
    edit->setText(QString("5"));
}


void
expand_d::peek_slot()
{
    edit->setText(QString("p"));
}


void
expand_d::apply_slot()
{
    char *string = lstring::copy(edit->text().toLatin1().constData());
    bool ret = false;
    if (p_callback)
        ret = (*p_callback)(string, p_cb_arg);
    emit apply(string, p_cb_arg);
    if (!ret)
        popdown();
}


void
expand_d::dismiss_slot()
{
    delete this;
}

