
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

#include <QAction>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTextEdit>

#include "qtinterf.h"
#include "progress_d.h"
#include "activity_w.h"


progress_d::progress_d(QTbag *owner, prgMode mode) :
    QDialog(owner ? owner->shell : 0)
{
    p_parent = owner;
    gb_in = 0;
    label_in = 0;
    gb_out = 0;
    label_out = 0;
    gb_info = 0;
    te_info = 0;
    gb_etc = 0;
    label_etc = 0;
    b_abort = 0;
    b_cancel = 0;
    pbar = 0;
    info_limit = 0;
    info_count = 0;

    if (owner)
        owner->monitor.add(this);

    setWindowTitle(tr("Progress"));
    QVBoxLayout *form = new QVBoxLayout(this);
    form->setMargin(4);
    form->setSpacing(2);

    if (mode == prgFileop) {
        QHBoxLayout *hbox = new QHBoxLayout(0);
        hbox->setSpacing(2);
        form->addLayout(hbox);
        gb_in = new QGroupBox(QString(tr("Input")), this);
        label_in = new QLabel("", gb_in);
        QVBoxLayout *vbox = new QVBoxLayout(gb_in);
        vbox->setMargin(4);
        vbox->setSpacing(2);
        vbox->addSpacing(10);
        vbox->addWidget(label_in);
        hbox->addWidget(gb_in);

        gb_out = new QGroupBox(QString(tr("Output")), this);
        label_out = new QLabel("", gb_out);
        vbox = new QVBoxLayout(gb_out);
        vbox->setMargin(4);
        vbox->setSpacing(2);
        vbox->addSpacing(10);
        vbox->addWidget(label_out);
        hbox->addWidget(gb_out);

        gb_info = new QGroupBox(QString(tr("Info")), this);
        te_info = new QTextEdit(gb_info);
        te_info->setReadOnly(true);
        vbox = new QVBoxLayout(gb_info);
        vbox->setMargin(4);
        vbox->setSpacing(2);
        vbox->addSpacing(10);
        vbox->addWidget(te_info);
        form->addWidget(gb_info);
    }

    gb_etc = new QGroupBox(this);
    label_etc = new QLabel("", gb_etc);
    QVBoxLayout *vbox = new QVBoxLayout(gb_etc);
    vbox->setMargin(4);
    vbox->setSpacing(2);
    vbox->addWidget(label_etc);
    form->addWidget(gb_etc);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setSpacing(2);
    form->addLayout(hbox);
    b_abort = new QPushButton("Abort", this);
    hbox->addWidget(b_abort);
    b_cancel = new QPushButton("Dismiss", this);
    hbox->addWidget(b_cancel);
    pbar = new activity_w(this);
    hbox->addWidget(pbar);

    connect(b_abort, SIGNAL(clicked()), this, SLOT(abort_slot()));
    connect(b_cancel, SIGNAL(clicked()), this, SLOT(quit_slot()));
}


progress_d::~progress_d()
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
            owner->monitor.remove(this);
    }
}


// GRpopup override
//
void
progress_d::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->monitor.is_active(this))
            return;
    }
    delete this;
}


void
progress_d::set_input(const char *str)
{
    if (label_in)
        label_in->setText(str);
}


void
progress_d::set_output(const char *str)
{
    if (label_out)
        label_out->setText(str);
}


void
progress_d::set_info(const char *str)
{
    if (te_info && str && *str) {
        if (info_limit > 0) {
            if (info_count == info_limit) {
                te_info->append(QString("More messages, see log file."));
                info_count++;
                return;
            }
            if (info_count > info_limit)
                return;
        }
        // Add a bullet and embolden "Warning:"
        int len = strlen(str) + 30;
        char *tbf = new char[len];
        if (!strncmp(str, "Warning:", 8))
            snprintf(tbf, len, "%c <b>Warning:</b>%s", 176, str+8);
        else
            snprintf(tbf, len, "%c %s", 176, str);
        te_info->append(QString(tbf));
        delete [] tbf;
        info_count++;
    }
}


void
progress_d::set_etc(const char *str)
{
    if (label_etc)
        label_etc->setText(str);
}


void
progress_d::start()
{
    if (pbar)
        pbar->start();
}


void
progress_d::finished()
{
    if (pbar)
        pbar->stop();
}


void
progress_d::quit_slot()
{
    delete this;
}


void
progress_d::abort_slot()
{
    emit abort();
}

