
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
#include "qtmsg.h"
#include "qtfont.h"

#include <QAction>
#include <QGroupBox>
#include <QLayout>
#include <QTextEdit>
#include <QPushButton>


namespace qtinterf
{
    class text_box : public QTextEdit
    {
    public:
        text_box(int w, int h, QWidget *prnt) :
            QTextEdit(prnt), qs(w, h), qsmin(w/2, h/2) { }

        QSize sizeHint() const { return (qs); }
        QSize minimumSizeHint() const { return (qsmin); }

    private:
        QSize qs;
        QSize qsmin;
    };
}


QTmsgPopup::QTmsgPopup(QTbag *owner, const char *message_str, STYtype sty,
    int w, int h) : QDialog(owner ? owner->Shell() : 0)
{
    p_parent = owner;
    display_style = sty;
    pw_desens = false;

    if (owner)
        owner->MonitorAdd(this);
    setAttribute(Qt::WA_DeleteOnClose);

    gbox = new QGroupBox(this);
    tx = new text_box(w, h, gbox);
    tx->setReadOnly(true);

    if (sty == STY_FIXED) {
        QFont *f;
        if (FC.getFont(&f, FNT_FIXED)) {
            tx->setCurrentFont(*f);
            tx->setFont(*f);
        }
    }
    setText(message_str);

    QVBoxLayout *vbox = new QVBoxLayout(gbox);
    vbox->setMargin(4);
    vbox->setSpacing(2);
    vbox->addWidget(tx);

    b_cancel = new QPushButton(tr("Dismiss"), this);
    connect(b_cancel, SIGNAL(clicked()), this, SLOT(quit_slot()));

    vbox = new QVBoxLayout(this);
    vbox->setMargin(4);
    vbox->setSpacing(2);
    vbox->addWidget(gbox);
    vbox->addWidget(b_cancel);
}


QTmsgPopup::~QTmsgPopup()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (owner)
            owner->ClearPopup(this);
    }
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        QTdev::Deselect(p_caller);
}


// GRpopup override
//
void
QTmsgPopup::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


void
QTmsgPopup::setTitle(const char *title)
{
    setWindowTitle(title);
}


void
QTmsgPopup::setText(const char *message_str)
{
    if (display_style == STY_HTML)
        tx->setHtml(message_str);
    else
        tx->setPlainText(message_str);
}


void
QTmsgPopup::quit_slot()
{
    delete this;
}

