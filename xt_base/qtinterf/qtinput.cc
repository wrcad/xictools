
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

#include "qtinput.h"
#include "miscutil/lstring.h"

#include <QAction>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>


QTledDlg::QTledDlg(QTbag *owner, const char *label_str,
    const char *initial_str, const char *action_str, bool mult) :
    QDialog(owner ? owner->Shell() : 0)
{
    p_parent = owner;
    ed_multiline = mult;
    ed_quit_flag = false;

    if (owner)
        owner->MonitorAdd(this);

    setWindowTitle(tr("Text Entry"));
    setWindowFlags(Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);

    if (!label_str)
        label_str = "Enter Filename";
    ed_gbox = new QGroupBox(this);
    ed_label = new QLabel(label_str, ed_gbox);
    QVBoxLayout *vbox = new QVBoxLayout(ed_gbox);
    vbox->setMargin(4);
    vbox->setSpacing(2);
    vbox->addWidget(ed_label);

    if (ed_multiline) {
        ed_edit = new QTextEdit(this);
        dynamic_cast<QTextEdit*>(ed_edit)->setPlainText(initial_str);
    }
    else {
        ed_edit = new QLineEdit(this);
        dynamic_cast<QLineEdit*>(ed_edit)->setText(initial_str);
    }

    if (!action_str)
        action_str = "Apply";
    ed_ok = new QPushButton(action_str, this);
    connect(ed_ok, SIGNAL(clicked()), this, SLOT(action_slot()));
    ed_cancel = new QPushButton(tr("Cancel"), this);
    connect(ed_cancel, SIGNAL(clicked()), this, SLOT(quit_slot()));

    vbox = new QVBoxLayout(this);
    vbox->setMargin(4);
    vbox->setSpacing(2);
    vbox->addWidget(ed_gbox);
    vbox->addWidget(ed_edit);
    QHBoxLayout *hbox = new QHBoxLayout(0);
    vbox->addLayout(hbox);
    hbox->addWidget(ed_ok);
    hbox->addWidget(ed_cancel);
}


QTledDlg::~QTledDlg()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (owner)
            owner->ClearPopup(this);
    }
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_cancel)
        (*p_cancel)(ed_quit_flag);
    if (p_caller && !p_no_desel)
        QTdev::Deselect(p_caller);
}


// GRpopup override
// Register the calling button, and set up:
//  1.  whether or not the caller is deselected on popdwon.
//  2.  whether or not deselecting the caller causes popdown.
//
void
QTledDlg::register_caller(GRobject c, bool no_dsl, bool handle_popdn)
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
QTledDlg::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// GRledPopup override
//
void
QTledDlg::update(const char *prompt_str, const char *init_str)
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    if (prompt_str)
        set_message(prompt_str);
    if (init_str)
        set_text(init_str);
}


// Replace the label text.
//
void
QTledDlg::set_message(const char *msg)
{
    ed_label->setText(QString(msg));
}


// Replace the edit text.
//
void
QTledDlg::set_text(const char *msg)
{
    if (ed_multiline)
        dynamic_cast<QTextEdit*>(ed_edit)->setPlainText(QString(msg));
    else
        dynamic_cast<QLineEdit*>(ed_edit)->setText(QString(msg));
}


void
QTledDlg::action_slot()
{
    char *text;
    if (ed_multiline)
        text = lstring::copy(dynamic_cast<QTextEdit*>(ed_edit)->
            toPlainText().toLatin1().constData());
    else
        text = lstring::copy(dynamic_cast<QLineEdit*>(ed_edit)->
            text().toLatin1().constData());

    if (ed_ign_ret) {
        if (p_callback)
            (*p_callback)(text, p_cb_arg);
        emit action_call(text, p_cb_arg);
        delete [] text;
    }
    else if (p_callback) {
        int ret = (*p_callback)(text, p_cb_arg);
        emit action_call(text, p_cb_arg);
        delete [] text;
        if (ret != ESTR_IGN) {
            ed_quit_flag = true;
            delete this;
        }
    }
    else {
        emit action_call(text, p_cb_arg);
        delete [] text;
    }
}


void
QTledDlg::quit_slot()
{
    deleteLater();
}

