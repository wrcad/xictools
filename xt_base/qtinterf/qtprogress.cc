
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

#include <QApplication>
#include <QAction>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QToolButton>
#include <QPushButton>
#include <QTextEdit>

#include "qtinterf.h"
#include "qtprogress.h"
#include "qtactivity.h"


QTprogressDlg::QTprogressDlg(QTbag *owner, prgMode mode)
{
    p_parent = owner;
    pg_gb_in = 0;
    pg_label_in = 0;
    pg_gb_out = 0;
    pg_label_out = 0;
    pg_gb_info = 0;
    pg_te_info = 0;
    pg_gb_etc = 0;
    pg_label_etc = 0;
    pg_pbar = 0;
    pg_info_limit = 0;
    pg_info_count = 0;

    if (owner)
        owner->MonitorAdd(this);

    setWindowTitle(tr("Progress"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    if (mode == prgFileop) {
        QHBoxLayout *hbox = new QHBoxLayout();
        hbox->setContentsMargins(qm);
        hbox->setSpacing(2);
        vbox->addLayout(hbox);

        pg_gb_in = new QGroupBox(tr("Input"));
        hbox->addWidget(pg_gb_in);
        QVBoxLayout *vb = new QVBoxLayout(pg_gb_in);
        vb->setContentsMargins(qmtop);
        vb->setSpacing(2);
        pg_label_in = new QLabel("");
        vb->addWidget(pg_label_in);
        vb->addSpacing(10);

        pg_gb_out = new QGroupBox(tr("Output"));
        hbox->addWidget(pg_gb_out);
        vb = new QVBoxLayout(pg_gb_out);
        vb->setContentsMargins(qmtop);
        vb->setSpacing(2);
        pg_label_out = new QLabel("");
        vb->addWidget(pg_label_out);
        vb->addSpacing(10);

        pg_gb_info = new QGroupBox(tr("Info"));
        vbox->addWidget(pg_gb_info);
        vb = new QVBoxLayout(pg_gb_info);
        vb->setContentsMargins(qmtop);
        vb->setSpacing(2);
        vb->addSpacing(10);
        pg_te_info = new QTextEdit();
        pg_te_info->setReadOnly(true);
        vb->addWidget(pg_te_info);
    }

    pg_gb_etc = new QGroupBox(this);
    vbox->addWidget(pg_gb_etc);
    QVBoxLayout *vb = new QVBoxLayout(pg_gb_etc);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);
    pg_label_etc = new QLabel("");
    vb->addWidget(pg_label_etc);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Abort"));
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(abort_btn_slot()));

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    pg_pbar = new QTactivity();
    hbox->addWidget(pg_pbar);
}


QTprogressDlg::~QTprogressDlg()
{
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller) {
        QObject *o = (QObject*)p_caller;
        if (o->isWidgetType()) {
            QAbstractButton *btn = dynamic_cast<QAbstractButton*>(o);
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


#ifdef Q_OS_MACOS
#define DLGTYPE QTprogressDlg
#include "qtmacos_event.h"
#endif


// GRpopup override
//
void
QTprogressDlg::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


void
QTprogressDlg::set_input(const char *str)
{
    if (pg_label_in)
        pg_label_in->setText(str);
}


void
QTprogressDlg::set_output(const char *str)
{
    if (pg_label_out)
        pg_label_out->setText(str);
}


void
QTprogressDlg::set_info(const char *str)
{
    if (pg_te_info && str && *str) {
        if (pg_info_limit > 0) {
            if (pg_info_count == pg_info_limit) {
                pg_te_info->append(QString("More messages, see log file."));
                pg_info_count++;
                return;
            }
            if (pg_info_count > pg_info_limit)
                return;
        }
        // Add a bullet and embolden "Warning:"
        int len = strlen(str) + 30;
        char *tbf = new char[len];
        if (!strncmp(str, "Warning:", 8))
            snprintf(tbf, len, "%c <b>Warning:</b>%s", 176, str+8);
        else
            snprintf(tbf, len, "%c %s", 176, str);
        pg_te_info->append(QString(tbf));
        delete [] tbf;
        pg_info_count++;
    }
}


void
QTprogressDlg::set_etc(const char *str)
{
    if (pg_label_etc)
        pg_label_etc->setText(str);
}


void
QTprogressDlg::start()
{
    if (pg_pbar)
        pg_pbar->start();
}


void
QTprogressDlg::finished()
{
    if (pg_pbar)
        pg_pbar->stop();
}


void
QTprogressDlg::abort_btn_slot()
{
    emit abort();
}


void
QTprogressDlg::dismiss_btn_slot()
{
    delete this;
}

