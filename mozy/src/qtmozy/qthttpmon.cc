
/*========================================================================
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
 * Qt MOZY help viewer.
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include <QApplication>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTimer>

#include "qthttpmon.h"
#include "httpget/transact.h"
#include "miscutil/lstring.h"

//
// The monitor widget and functions for the httpget library and
// program.
//


using namespace qtinterf;

/*
//-----------------------------------------------------------------------------
// Static functions for stand-alone application

bool
http_monitor::graphics_enabled()
{
    return (true);
}


void
http_monitor::initialize(int &argc, char **argv)
{
    new QApplication(argc, argv);
}


void
http_monitor::setup_comm(sComm*)
{
}


void
http_monitor::start(Transaction *t)
{
    QThttpmon *dl = new QThttpmon(0);
    t->t_gcontext = dl;
    dl->set_transaction(t);

    if (setjmp(dl->g_jbuf) == -2) {
        t->t_err_return = HTTPAborted;
        return;
    }

#if (QT_VERSION >> 16 < 4)
    qApp->setMainWidget(dl);
#endif
    dl->show();
    qApp->exec();
}
*/


//-----------------------------------------------------------------------------
// The download monitor widget

QThttpmon::QThttpmon(QWidget *prnt) : QDialog(prnt)
{
    g_transaction = 0;
    g_textbuf = 0;
    g_jbuf_set = false;

#if (QT_VERSION >> 16 < 4)
    setCaption("httpget - download file");
#else
    setWindowTitle("httpget - download file");
#endif

    g_gb = new QGroupBox(this);
    g_label = new QLabel("", g_gb);
    g_label->setMinimumWidth(350);
    QVBoxLayout *vbox = new QVBoxLayout(g_gb);
    vbox->setContentsMargins(2, 2, 2, 2);
    vbox->setSpacing(2);
    vbox->addWidget(g_label);

    g_cancel = new QPushButton(tr("Cancel Download"), this);
    vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(2, 2, 2, 2);
    vbox->setSpacing(2);
    vbox->addWidget(g_gb);
    vbox->addWidget(g_cancel);

    connect(g_cancel, SIGNAL(clicked()), this, SLOT(abort_slot()));

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(run_slot()));
#if (QT_VERSION >> 16 < 4)
    timer->start(0, true);
#else
    timer->setSingleShot(true);
    timer->start(0);
#endif

}


QThttpmon::~QThttpmon()
{
    if (g_transaction)
        g_transaction->set_gr(0);
    delete [] g_textbuf;
}


// Function to print a message in the text area.  If buf is 0, reprint
// from the t_textbuf.  This also services events, if the widget is
// realized, and returns true.  False is returned if the widget is not
// realized.
//
bool
QThttpmon::widget_print(const char *buf)
{
    char *str = lstring::copy(buf ? buf : g_textbuf);
    if (str) {
        char *e = str + strlen(str) - 1;
        while (e >= str && isspace(*e))
            *e-- = 0;
        char *s = str;
        while (*s == '\r' || *s == '\n')
            s++;
        if (*s) {
            if (buf) {
                delete [] g_textbuf;
                g_textbuf = lstring::copy(s);  // for expose
            }
            g_label->setText(s);
        }
        delete [] str;
    }
    if (qApp)
        qApp->processEvents();
    return (true);
}


void
QThttpmon::abort()
{
    // Have to use setjmp/longjmp, since exceptions won't propagate
    // through qt for some reason.
    if (g_jbuf_set) {
        g_jbuf_set = false;
        longjmp(g_jbuf, -2);
    }
}


void
QThttpmon::run(Transaction *t)
{
    g_jbuf_set = true;
    http_monitor::run(t);
    g_jbuf_set = false;
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(quit_slot()));
#if (QT_VERSION >> 16 < 4)
    timer->start(2000, true);
#else
    timer->setSingleShot(true);
    timer->start(2000);
#endif
}


void
QThttpmon::run_slot()
{
    if (g_transaction)
        run (g_transaction);
}


void
QThttpmon::abort_slot()
{
    abort();
}


void
QThttpmon::quit_slot()
{
    delete this;
}


bool
QThttpmon::event(QEvent *ev)
{
    if (ev->type() == QEvent::Close || ev->type() == QEvent::Destroy) {
        // If the user closes the window, hide the display and
        // finish downloading.
        hide();
        return (true);
    }
    return (QDialog::event(ev));
}

