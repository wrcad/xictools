
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

#include "qttech.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "tech.h"
#include "promptline.h"
#include "errorlog.h"
#include "miscutil/filestat.h"

#include <QLayout>
#include <QRadioButton>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>


//-------------------------------------------------------------------------
// Pop-up to control writing of a technology file.
//
// Help system keywords used:
//  xic:updat

void
cMain::PopUpTechWrite(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTwriteTechDlg::self())
            QTwriteTechDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTwriteTechDlg::self())
            QTwriteTechDlg::self()->update();
        return;
    }
    if (QTwriteTechDlg::self())
        return;

    new QTwriteTechDlg(caller);

    QTdev::self()->SetPopupLocation(GRloc(), QTwriteTechDlg::self(),
        QTmainwin::self()->Viewport());
    QTwriteTechDlg::self()->show();
}
// End of cMain functions.


QTwriteTechDlg *QTwriteTechDlg::instPtr;

QTwriteTechDlg::QTwriteTechDlg(GRobject caller)
{
    instPtr = this;
    tc_caller = caller;
    tc_none = 0;
    tc_cmt = 0;
    tc_use = 0;
    tc_entry = 0;
    tc_write = 0;

    setWindowTitle(tr("Write Tech File"));
    setAttribute(Qt::WA_DeleteOnClose);
//    gtk_window_set_resizable(GTK_WINDOW(tc_popup), false);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    tc_none = new QRadioButton(tr("Omit default definitions"));
    vbox->addWidget(tc_none);
    connect(tc_none, SIGNAL(toggled(bool)),
        this, SLOT(none_btn_slot(bool)));

    tc_cmt = new QRadioButton(tr("Comment default definitions"));
    vbox->addWidget(tc_cmt);
    connect(tc_cmt, SIGNAL(toggled(bool)),
        this, SLOT(cmt_btn_slot(bool)));

    tc_use = new QRadioButton(tr("Include default definitions"));
    vbox->addWidget(tc_use);
    connect(tc_use, SIGNAL(toggled(bool)),
        this, SLOT(use_btn_slot(bool)));

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QLabel *label = new QLabel(tr("Technology File"));
    label->setAlignment(Qt::AlignCenter);
    hbox->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    char string[256];
    if (Tech()->TechExtension() && *Tech()->TechExtension())
        snprintf(string, sizeof(string), "./%s.%s",
            XM()->TechFileBase(), Tech()->TechExtension());
    else
        snprintf(string, sizeof(string), "./%s", XM()->TechFileBase());

    tc_entry = new QLineEdit();
    vbox->addWidget(tc_entry);
    tc_entry->setText(string);

    hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    tc_write = new QPushButton(tr("Write File"));
    hbox->addWidget(tc_write);
    connect(tc_write, SIGNAL(clicked()), this, SLOT(write_btn_slot()));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTwriteTechDlg::~QTwriteTechDlg()
{
    instPtr = 0;
    if (tc_caller)
        QTdev::Deselect(tc_caller);
}


void
QTwriteTechDlg::update()
{
    const char *v = CDvdb()->getVariable(VA_TechPrintDefaults);
    if (!v) {
        QTdev::SetStatus(tc_none, true);
        QTdev::SetStatus(tc_cmt, false);
        QTdev::SetStatus(tc_use, false);
    }
    else if (!*v) {
        QTdev::SetStatus(tc_none, false);
        QTdev::SetStatus(tc_cmt, true);
        QTdev::SetStatus(tc_use, false);
    }
    else {
        QTdev::SetStatus(tc_none, false);
        QTdev::SetStatus(tc_cmt, false);
        QTdev::SetStatus(tc_use, true);
    }
}


void
QTwriteTechDlg::none_btn_slot(bool state)
{
    if (state)
        CDvdb()->clearVariable(VA_TechPrintDefaults);
}


void
QTwriteTechDlg::cmt_btn_slot(bool state)
{
    if (state)
        CDvdb()->setVariable(VA_TechPrintDefaults, "");
}


void
QTwriteTechDlg::use_btn_slot(bool state)
{
    if (state)
        CDvdb()->setVariable(VA_TechPrintDefaults, "all");
}


void
QTwriteTechDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:updat"))
}


void
QTwriteTechDlg::write_btn_slot()
{
    const char *s = CDvdb()->getVariable(VA_TechPrintDefaults);
    if (s) {
        if (*s)
            Tech()->SetPrintMode(TCPRall);
        else
            Tech()->SetPrintMode(TCPRcmt);
    }
    else
        Tech()->SetPrintMode(TCPRnondef);

    const char *string = lstring::copy(tc_entry->text().toLatin1().constData());

    // make a backup of the present file
    if (!filestat::create_bak(string)) {
        QTpkg::self()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        PL()->ShowPromptV("Update of %s failed.", string);
        delete [] string;
        return;
    }

    FILE *techfp;
    if ((techfp = filestat::open_file(string, "w")) == 0) {
        Log()->ErrorLog(mh::Initialization, filestat::error_msg());
        delete [] string;
        return;
    }
    HCcb &cb = Tech()->HC();
    DSPmainWbag(HCupdate(&cb, 0))
    // If in hardcopy mode, switch back temporarily.
    int save_drvr = Tech()->HcopyDriver();
    if (DSP()->DoingHcopy())
        XM()->HCswitchMode(false, true, Tech()->HcopyDriver());

    fprintf(techfp, "# update from %s\n", XM()->IdString());

    Tech()->Print(techfp);

    fclose(techfp);
    if (DSP()->DoingHcopy())
        XM()->HCswitchMode(true, true, save_drvr);

    PL()->ShowPromptV("Current attributes written to new %s file.",
        string);
    delete [] string;
    XM()->PopUpTechWrite(0, MODE_OFF);
}


void
QTwriteTechDlg::dismiss_btn_slot()
{
    XM()->PopUpTechWrite(0, MODE_OFF);
}

