
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
#include "qtspmsg.h"
#include "qttoolb.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>


// Pop up a message at x, y.
//
void
QTtoolbar::PopUpSpiceMessage(const char *string, int x, int y)
{
    if (!CP.Display())
        return;
    if (!GRpkg::self()->CurDev())
        return;
    if (!string || !*string)
        return;
    if (!QTspmsgDlg::self()) {
        new QTspmsgDlg(string);
        FixLoc(&x, &y);

        QTspmsgDlg::self()->move(x, y);
        QTspmsgDlg::self()->show();
    }
}


QTspmsgDlg *QTspmsgDlg::instPtr;

QTspmsgDlg::QTspmsgDlg(const char *string)
{
    instPtr = this;
    setWindowTitle(tr("Message"));
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(2, 2, 2, 2);
    vbox->setSpacing(2);

    // the label, in a frame
    //
    QGroupBox *gb = new QGroupBox();
    vbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(2, 2, 2, 2);
    hb->setSpacing(2);

    QLabel *label = new QLabel(tr(string));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    // the dismiss button
    //
    QPushButton *cancel = new QPushButton(tr("Dismiss"));
    vbox->addWidget(cancel);
    connect(cancel, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

//    QTdev::self()->SetDoubleClickExit(popup, cancel);

}


QTspmsgDlg::~QTspmsgDlg()
{
    instPtr = 0;
}


void
QTspmsgDlg::dismiss_btn_slot()
{
    delete this;
}

