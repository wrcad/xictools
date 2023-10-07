
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "cshell.h"
#include "simulator.h"
#include "ttyio.h"
#include "qterrmsg.h"
#include "qttoolb.h"
#include "qtinterf/qttextw.h"

#include <QLayout>
#include <QPushButton>
#include <QScreen>

// Error message default window size.
#define ER_WIDTH    400
#define ER_HEIGHT   120

// Instantiate the message database.
namespace {
    QTmsgDb msgDB;
}


// Handle message printing, takes care of the dialog, log file, and
// stdout printing.
//
void
QTtoolbar::PopUpSpiceErr(bool to_stdout, const char *string)
{
    if (!string || !*string)
        return;
    if (!CP.Display() || !GRpkg::self()->CurDev())
        to_stdout = true;
    if (Sp.GetFlag(FT_NOERRWIN))
        to_stdout = true;

    if (to_stdout) {
        if (TTY.is_tty() && CP.GetFlag(CP_WAITING)) {
            TTY.err_printf("\n");
            CP.SetFlag(CP_WAITING, false);
        }
        TTY.err_printf("%s", string);
        msgDB.ToLog(string);
        return;
    }
    msgDB.PopUpErr(string);
}


void
QTmsgDb::PopUpErr(const char *string)
{
    if (QTerrmsgDlg::self()) {
        // The message is not immediately loaded into the window,
        // instead messages are queued and handled in an idle
        // procedure.  This is for efficiency in handling a huge
        // cascade of messages that can occur.
        //
        append_message(string);
        return;
    }
    new QTerrmsgDlg(string);
    first_message(string);
}


void
QTmsgDb::stuff_msg(const char *str)
{
    if (QTerrmsgDlg::self())
        QTerrmsgDlg::self()->stuff_msg(str);
}
// End of QTmsgDb functions.


QTerrmsgDlg *QTerrmsgDlg::instPtr;

QTerrmsgDlg::QTerrmsgDlg(const char *string)
{
    instPtr = this;
    setWindowTitle(tr("ERROR"));
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    // scrolled text area
    //
    er_text = new QTtextEdit();
    vbox->addWidget(er_text);
    er_text->setReadOnly(true);

    // the wrap and dismiss buttons
    //
    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setMargin(0);
    hbox->setSpacing(2);

    QPushButton *wrap = new QPushButton(tr("Wrap Lines"));;
    wrap->setCheckable(true);
    hbox->addWidget(wrap);
    wrap->setAutoDefault(false);
    connect(wrap, SIGNAL(toggled(bool)),
        this, SLOT(wrap_btn_slot(bool)));

    QPushButton *cancel = new QPushButton(tr("Dismiss"));
    hbox->addWidget(cancel);
    connect(cancel, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));
    cancel->setAutoDefault(false);
    cancel->setDefault(true);

    QTdev::SetStatus(wrap, msgDB.get_wrap());
    er_text->setLineWrapMode(
        msgDB.get_wrap() ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);

    if (msgDB.get_x() == 0 && msgDB.get_y() == 0) {
        QSize screen_sz = screen()->size();
        msgDB.set_x((screen_sz.width() - ER_WIDTH)/2);
        msgDB.set_y(0);
    }
    if (string && *string)
        er_text->set_chars(string);
    move(msgDB.get_x(), msgDB.get_y());
    show();
}


QTerrmsgDlg::~QTerrmsgDlg()
{
    instPtr = 0;
    QPoint pt = mapToGlobal(QPoint(0, 0));
    msgDB.set_x(pt.x());
    msgDB.set_y(pt.y());
}


QSize
QTerrmsgDlg::sizeHint() const
{
    return (QSize(ER_WIDTH, ER_HEIGHT));
}


// Add the string to the display, at the end.  We keep only
// MAX_ERR_LINES lines in the display.  Oldest lines are deleted to
// maintain the limit.
//
void
QTerrmsgDlg::stuff_msg(const char *string)
{
    if (!er_text)
        return;

    char *t;
    char *s = er_text->get_chars();
    int cnt = 0;
    for (t = s; *t; t++) {
        if (*t == '\n')
            cnt++;
    }
    er_text->set_editable(true);
    if (cnt > MAX_ERR_LINES) {
        cnt = 0;
        for ( ; t > s; t--) {
            if (*t == '\n') {
                cnt++;
                if (cnt == MAX_ERR_LINES) {
                    er_text->delete_chars(0, t + 1 - s);
                    break;
                }
            }
        }
    }
    delete [] s;

    er_text->insert_chars_at_point(0, string, -1, -1);
    er_text->set_editable(false);
}


void
QTerrmsgDlg::wrap_btn_slot(bool state)
{
    msgDB.set_wrap(state);
    er_text->setLineWrapMode(
        state ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
}


void
QTerrmsgDlg::dismiss_btn_slot()
{
    delete this;
}

