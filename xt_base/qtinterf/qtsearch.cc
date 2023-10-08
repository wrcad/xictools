
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

QTsearchDlg::QTsearchDlg(QTbag *owner, const char *initstr) : se_timer(this)
{
    p_parent = owner;

    if (owner)
        owner->MonitorAdd(this);

    setWindowTitle(QString(tr("Search")));
    setWindowFlags(Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QGroupBox *gb = new QGroupBox();
    vbox->addWidget(gb);
    QVBoxLayout *vb = new QVBoxLayout(gb);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);
    se_label = new QLabel("");
    vb->addWidget(se_label);

    se_edit = new QLineEdit(this);
    se_edit->setText(initstr);
    vbox->addWidget(se_edit);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    QPushButton *btn = new QPushButton();
    btn->setAutoDefault(false);
    btn->setIcon(QIcon(QPixmap(down_xpm)));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(down_btn_slot()));

    btn = new QPushButton();
    btn->setAutoDefault(false);
    btn->setIcon(QIcon(QPixmap(up_xpm)));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(up_btn_slot()));

    se_nc = new QCheckBox(this);
    se_nc->setText(tr("No Case"));
    hbox->addWidget(se_nc);
    connect(se_nc, SIGNAL(toggled(bool)), this, SLOT(icase_btn_slot(bool)));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    se_timer.setInterval(1000);
    set_message("Enter search text:");
    connect(&se_timer, SIGNAL(timeout()), this, SLOT(timeout_slot()));
}


QTsearchDlg::~QTsearchDlg()
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
    delete [] se_label_string;
}


// GRpopup override
//
void
QTsearchDlg::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


void
QTsearchDlg::set_ign_case(bool set)
{
    se_nc->setChecked(set);
}


void
QTsearchDlg::set_message(const char *msg)
{
    if (msg) {
        delete [] se_label_string;
        se_label_string = lstring::copy(msg);
        se_label->setText(msg);
    }
}


void
QTsearchDlg::set_transient_message(const char *msg)
{
    if (msg) {
        se_label->setText(QString(msg));
        se_timer.start();
    }
}


QString
QTsearchDlg::get_target()
{
    return (se_edit->text());
}


void
QTsearchDlg::down_btn_slot()
{
    emit search_down();
}


void
QTsearchDlg::up_btn_slot()
{
    emit search_up();
}


void
QTsearchDlg::icase_btn_slot(bool set)
{
    emit ignore_case(set);
}


void
QTsearchDlg::dismiss_btn_slot()
{
    delete this;
}


void
QTsearchDlg::timeout_slot()
{
    se_timer.stop();
    se_label->setText(se_label_string);
}

