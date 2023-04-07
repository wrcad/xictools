
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
#include "lstring.h"

#include <QAction>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>


QTledPopup::QTledPopup(qt_bag *owner, const char *label_str,
    const char *initial_str, const char *action_str, void *arg, bool mult) :
    QDialog(owner ? owner->shell : 0)
{
    p_parent = owner;
    p_cb_arg = arg;
    multiline = mult;
    quit_flag = false;

    if (owner)
        owner->monitor.add(this);

    setWindowTitle(tr("Text Entry"));

    if (!label_str)
        label_str = "Enter Filename";
    gbox = new QGroupBox(this);
    label = new QLabel(label_str, gbox);
    QVBoxLayout *vbox = new QVBoxLayout(gbox);
    vbox->setMargin(4);
    vbox->setSpacing(2);
    vbox->addWidget(label);

    if (multiline) {
        edit = new QTextEdit(this);
        dynamic_cast<QTextEdit*>(edit)->setPlainText(initial_str);
    }
    else {
        edit = new QLineEdit(this);
        dynamic_cast<QLineEdit*>(edit)->setText(initial_str);
    }

    b_ok = new QPushButton(action_str, this);
    connect(b_ok, SIGNAL(clicked()), this, SLOT(action_slot()));
    b_cancel = new QPushButton(tr("Cancel"), this);
    connect(b_cancel, SIGNAL(clicked()), this, SLOT(quit_slot()));

    vbox = new QVBoxLayout(this);
    vbox->setMargin(4);
    vbox->setSpacing(2);
    vbox->addWidget(gbox);
    vbox->addWidget(edit);
    QHBoxLayout *hbox = new QHBoxLayout(0);
    vbox->addLayout(hbox);
    hbox->addWidget(b_ok);
    hbox->addWidget(b_cancel);
}


QTledPopup::~QTledPopup()
{
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_cancel)
        (*p_cancel)(quit_flag);
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
        if (owner) {
            owner->monitor.remove(this);
            if (owner->input == this) {
                owner->input = 0;
                if (owner && owner->sens_set)
                    (*owner->sens_set)(owner, true);
            }
        }
    }
}


// GRpopup override
// Register the calling button, and set up:
//  1.  whether or not the caller is deselected on popdwon.
//  2.  whether or not deselecting the caller causes popdown.
//
void
QTledPopup::register_caller(GRobject c, bool no_dsl, bool handle_popdn)
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
QTledPopup::popdown()
{
    if (p_parent) {
        qt_bag *owner = dynamic_cast<qt_bag*>(p_parent);
        if (!owner || !owner->monitor.is_active(this))
            return;
    }
    delete this;
}


// GRledPopup override
//
void
QTledPopup::update(const char *prompt_str, const char *init_str)
{
    if (p_parent) {
        qt_bag *owner = dynamic_cast<qt_bag*>(p_parent);
        if (!owner || !owner->monitor.is_active(this))
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
QTledPopup::set_message(const char *msg)
{
    label->setText(QString(msg));
}


// Replace the edit text.
//
void
QTledPopup::set_text(const char *msg)
{
    if (multiline)
        dynamic_cast<QTextEdit*>(edit)->setPlainText(QString(msg));
    else
        dynamic_cast<QLineEdit*>(edit)->setText(QString(msg));
}


void
QTledPopup::action_slot()
{
    char *text;
    if (multiline)
        text = lstring::copy(dynamic_cast<QTextEdit*>(edit)->
            toPlainText().toAscii().constData());
    else
        text = lstring::copy(dynamic_cast<QLineEdit*>(edit)->
            text().toAscii().constData());

    if (ign_ret) {
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
            quit_flag = true;
            delete this;
        }
    }
    else {
        emit action_call(text, p_cb_arg);
        delete [] text;
    }
}


void
QTledPopup::quit_slot()
{
    delete this;
}

