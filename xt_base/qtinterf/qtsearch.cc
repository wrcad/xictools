
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
#include "qtsearch.h"
#include "miscutil/lstring.h"

#include <QAction>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>

// XPM
static const char * const up_xpm[] = {
"32 16 2 1",
" 	c none",
".	c blue",
"                                ",
"               .                ",
"              ...               ",
"             .....              ",
"            .......             ",
"           .........            ",
"          ...........           ",
"         .............          ",
"        ...............         ",
"       .................        ",
"      ...................       ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                "};

// XPM
static const char * const down_xpm[] = {
"32 16 2 1",
" 	c none",
".	c blue",
"                                ",
"                                ",
"                                ",
"                                ",
"      ...................       ",
"       .................        ",
"        ...............         ",
"         .............          ",
"          ...........           ",
"           .........            ",
"            .......             ",
"             .....              ",
"              ...               ",
"               .                ",
"                                ",
"                                "};

QTsearch::QTsearch(QTbag *owner, const char *initstr) :
    QDialog(owner ? owner->Shell() : 0), timer(this)
{
    p_parent = owner;

    if (owner)
        owner->MonitorAdd(this);

    setWindowTitle(QString(tr("Search")));
    setWindowFlags(Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(4);
    vbox->setSpacing(2);

    QGroupBox *gb = new QGroupBox(this);
    label = new QLabel(gb);
    QVBoxLayout *vb = new QVBoxLayout(gb);
    vb->setMargin(4);
    vb->addWidget(label);
    vbox->addWidget(gb);

    edit = new QLineEdit(this);
    edit->setText(QString(initstr));
    vbox->addWidget(edit);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);

    b_cancel = new QPushButton(this);
    b_dn = new QPushButton(this);
    b_up = new QPushButton(this);

    b_dn->setAutoDefault(false);
    b_dn->setIcon(QIcon(QPixmap(down_xpm)));
    hbox->addWidget(b_dn);
    b_up->setAutoDefault(false);
    b_up->setIcon(QIcon(QPixmap(up_xpm)));
    hbox->addWidget(b_up);
    b_nc = new QCheckBox(this);
    b_nc->setText(QString(tr("No Case")));
    hbox->addWidget(b_nc);
    b_cancel->setText(QString(tr("Dismiss")));
    hbox->addWidget(b_cancel);
    vbox->addLayout(hbox);

    timer.setInterval(1000);
    set_message("Enter search text:");

    connect(b_dn, SIGNAL(clicked()), this, SLOT(down_slot()));
    connect(b_up, SIGNAL(clicked()), this, SLOT(up_slot()));
    connect(b_nc, SIGNAL(toggled(bool)), this, SLOT(ign_case_slot(bool)));
    connect(b_cancel, SIGNAL(clicked()), this, SLOT(quit_slot()));
    connect(&timer, SIGNAL(timeout()), this, SLOT(timeout_slot()));
}


QTsearch::~QTsearch()
{
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller) {
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
    delete [] label_string;
}


// GRpopup override
//
void
QTsearch::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


void
QTsearch::set_ign_case(bool set)
{
    b_nc->setChecked(set);
}


void
QTsearch::set_message(const char *msg)
{
    if (msg) {
        delete [] label_string;
        label_string = lstring::copy(msg);
        label->setText(QString(msg));
    }
}


void
QTsearch::set_transient_message(const char *msg)
{
    if (msg) {
        label->setText(QString(msg));
        timer.start();
    }
}


QString
QTsearch::get_target()
{
    return (edit->text());
}


void
QTsearch::quit_slot()
{
    delete this;
}


void
QTsearch::down_slot()
{
    emit search_down();
}


void
QTsearch::up_slot()
{
    emit search_up();
}


void
QTsearch::ign_case_slot(bool set)
{
    emit ignore_case(set);
}


void
QTsearch::timeout_slot()
{
    timer.stop();
    label->setText(QString(label_string));
}

