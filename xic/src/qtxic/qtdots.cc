
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qtdots.h"
#include "sced.h"
#include "menu.h"
#include "attr_menu.h"

#include <QLayout>
#include <QRadioButton>
#include <QPushButton>


//-------------------------------------------------------------------------
// Pop-up to control electrical connection point display.

void
cSced::PopUpDots(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTdotsDlg::self())
            QTdotsDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTdotsDlg::self())
            QTdotsDlg::self()->update();
        return;
    }
    if (QTdotsDlg::self())
        return;

    new QTdotsDlg(caller);

    QTdev::self()->SetPopupLocation(GRloc(), QTdotsDlg::self(),
        QTmainwin::self()->Viewport());
    QTdotsDlg::self()->show();
}
//End of cSced functions.


QTdotsDlg *QTdotsDlg::instPtr;

QTdotsDlg::QTdotsDlg(GRobject caller)
{
    instPtr = this;
    dt_caller = caller;
    dt_none = 0;
    dt_norm = 0;
    dt_all = 0;

    setWindowTitle(tr("Connection Points"));
    setWindowFlags(Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_ShowWithoutActivating);
//    gtk_window_set_resizable(GTK_WINDOW(dt_popup), false);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    dt_none = new QRadioButton(tr("Don't show dots"));
    vbox->addWidget(dt_none);
    connect(dt_none, SIGNAL(toggled(bool)), this, SLOT(none_slot(bool)));

    dt_norm = new QRadioButton(tr("Show dots normally"));
    vbox->addWidget(dt_norm);
    connect(dt_norm, SIGNAL(toggled(bool)), this, SLOT(norm_slot(bool)));

    dt_all = new QRadioButton(tr("Show dot at every connection"));
    vbox->addWidget(dt_all);
    connect(dt_all, SIGNAL(toggled(bool)), this, SLOT(all_slot(bool)));

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_slot()));

    update();
}


QTdotsDlg::~QTdotsDlg()
{
    instPtr = 0;
    if (dt_caller)
        QTdev::SetStatus(dt_caller, false);
}


void
QTdotsDlg::update()
{
    const char *v = CDvdb()->getVariable(VA_ShowDots);
    if (!v) {
        QTdev::SetStatus(dt_none, true);
        QTdev::SetStatus(dt_norm, false);
        QTdev::SetStatus(dt_all, false);
    }
    else if (*v == 'a' || *v == 'A') {
        QTdev::SetStatus(dt_none, false);
        QTdev::SetStatus(dt_norm, false);
        QTdev::SetStatus(dt_all, true);
    }
    else {
        QTdev::SetStatus(dt_none, false);
        QTdev::SetStatus(dt_norm, true);
        QTdev::SetStatus(dt_all, false);
    }
}


void
QTdotsDlg::none_slot(bool state)
{
    if (state)
        CDvdb()->clearVariable(VA_ShowDots);
}


void
QTdotsDlg::norm_slot(bool state)
{
    if (state)
        CDvdb()->setVariable(VA_ShowDots, "");
}


void
QTdotsDlg::all_slot(bool state)
{
    if (state)
        CDvdb()->setVariable(VA_ShowDots, "all");
}


void
QTdotsDlg::dismiss_slot()
{
    SCD()->PopUpDots(0, MODE_OFF);
}

