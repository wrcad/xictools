
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

#include "qtoatech.h"
#include "main.h"
#include "oa_if.h"
#include "dsp_inlines.h"
#include "errorlog.h"

#include <QApplication>
#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QLineEdit>


//-----------------------------------------------------------------------------
// QToaTechAttachDlg:  Dialog for setting the tech attachment for an
// OpenAccess library.
// Called from the OpenAccess Libraries list (QTodLibsDlg) Tech
// button.
//
// Help keyword:
// xic:oatech

void
cOAif::PopUpOAtech(GRobject caller, ShowMode mode, int x, int y)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QToaTechAttachDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QToaTechAttachDlg::self())
            QToaTechAttachDlg::self()->update();
        return;
    }
    if (QToaTechAttachDlg::self()) {
        QToaTechAttachDlg::self()->update();
        return;
    }

    new QToaTechAttachDlg(caller);

    QToaTechAttachDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_XYA, x, y),
        QToaTechAttachDlg::self(), QTmainwin::self()->Viewport());
    QToaTechAttachDlg::self()->show();
}
// End of cOAif functions.


QToaTechAttachDlg *QToaTechAttachDlg::instPtr;

QToaTechAttachDlg::QToaTechAttachDlg(GRobject c)
{
    instPtr = this;
    ot_caller = c;
    ot_label = 0;
    ot_unat = 0;
    ot_at = 0;
    ot_tech = 0;
    ot_def = 0;
    ot_dest = 0;
    ot_crt = 0;
    ot_status = 0;
    ot_attachment = 0;

    setWindowTitle(tr("OpenAccess Tech"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // label in frame plus help btn
    //
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    ot_label = new QLabel(tr("Set technology for library"));
    hb->addWidget(ot_label);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QToaTechAttachDlg::help_btn_slot);

    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    ot_unat = new QToolButton();
    ot_unat->setText(tr("Unattach"));
    hbox->addWidget(ot_unat);
    connect(ot_unat, &QAbstractButton::clicked,
        this, &QToaTechAttachDlg::unat_btn_slot);

    ot_at = new QToolButton();
    ot_at->setText(tr("Attach"));
    hbox->addWidget(ot_at);
    connect(ot_at, &QAbstractButton::clicked,
        this, &QToaTechAttachDlg::at_btn_slot);

    ot_tech = new QLineEdit();
    hbox->addWidget(ot_tech);

    ot_def = new QToolButton();
    ot_def->setText(tr("Default"));
    hbox->addWidget(ot_def);
    connect(ot_def, &QAbstractButton::clicked,
        this, &QToaTechAttachDlg::def_btn_slot);

    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    ot_dest = new QToolButton();
    ot_dest->setText(tr("Destroy Tech"));
    hbox->addWidget(ot_dest);
    connect(ot_dest, &QAbstractButton::clicked,
        this, &QToaTechAttachDlg::dest_btn_slot);

    ot_crt = new QToolButton();
    ot_crt->setText(tr("Create New Tech"));
    hbox->addWidget(ot_crt);
    connect(ot_crt, &QAbstractButton::clicked,
        this, &QToaTechAttachDlg::crt_btn_slot);

    gb = new QGroupBox();
    vbox->addWidget(gb);
    hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qmtop);
    hbox->setSpacing(2);

    ot_status = new QLabel(tr("Tech status:"));
    hbox->addWidget(ot_status);

    // Dismiss button
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    vbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QToaTechAttachDlg::dismiss_btn_slot);

    update();
}


QToaTechAttachDlg::~QToaTechAttachDlg()
{
    instPtr = 0;
    if (ot_caller)
        QTdev::Deselect(ot_caller);
    delete [] ot_attachment;
}


#ifdef Q_OS_MACOS
#define DLGTYPE QToaTechAttachDlg
#include "qtinterf/qtmacos_event.h"
#endif


void
QToaTechAttachDlg::update()
{
    char buf[256];
    const char *libname;
    OAif()->GetSelection(&libname, 0);
    if (!libname)
        return;
    bool branded;
    if (!OAif()->is_lib_branded(libname, &branded)) {
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        return;
    }
    if (branded)
        snprintf(buf, sizeof(buf), "Set technology for library %s", libname);
    else
        snprintf(buf, sizeof(buf), "Technology for library %s", libname);
    ot_label->setText(tr(buf));
    char *attachlib;
    if (!OAif()->has_attached_tech(libname, &attachlib)) {
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
        return;
    }
    if (attachlib) {
        delete [] ot_attachment;
        ot_attachment = lstring::copy(attachlib);
        snprintf(buf, sizeof(buf), "Tech status: attached library %s",
            attachlib);
        ot_status->setText(tr(buf));
        delete [] attachlib;
        ot_unat->setEnabled(branded);
        ot_at->setEnabled(false);
        ot_tech->setEnabled(false);
        ot_def->setEnabled(false);
        ot_dest->setEnabled(false);
        ot_crt->setEnabled(false);
    }
    else {
        bool hastech;
        if (!OAif()->has_local_tech(libname, &hastech)) {
            Log()->ErrorLog(mh::Processing, Errs()->get_error());
            return;
        }
        if (hastech) {
            ot_status->setText(tr("Tech status: local database"));
            ot_unat->setEnabled(false);
            ot_at->setEnabled(false);
            ot_tech->setEnabled(false);
            ot_def->setEnabled(false);
            ot_dest->setEnabled(branded);
            ot_crt->setEnabled(false);
        }
        else {
            ot_status->setText(tr("Tech status: no database (not good!)"));
            ot_unat->setEnabled(false);
            ot_at->setEnabled(branded);
            ot_tech->setEnabled(branded);
            ot_def->setEnabled(branded);
            ot_dest->setEnabled(false);
            ot_crt->setEnabled(branded);
        }
    }
}


void
QToaTechAttachDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:oatech"))
}


void
QToaTechAttachDlg::unat_btn_slot()
{
    const char *libname;
    OAif()->GetSelection(&libname, 0);
    if (!libname)
        return;
    if (!OAif()->destroy_tech(libname, true))
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
    update();
}


void
QToaTechAttachDlg::at_btn_slot()
{
    const char *libname;
    OAif()->GetSelection(&libname, 0);
    if (!libname)
        return;
    QByteArray alib_ba = ot_tech->text().toLatin1();
    const char *alib = alib_ba.constData();
    char *tok = lstring::gettok(&alib);
    if (!tok) {
        Log()->PopUpErr("No tech library name given!");
        return;
    }
    bool ret = OAif()->attach_tech(libname, tok);
    delete [] tok;
    if (!ret)
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
    update();
}


void
QToaTechAttachDlg::def_btn_slot()
{
    const char *def = CDvdb()->getVariable(VA_OaDefTechLibrary);
    if (!def)
        def = ot_attachment;
    if (!def)
        def = "";
    ot_tech->setText(def);
}


void
QToaTechAttachDlg::dest_btn_slot()
{
    const char *libname;
    OAif()->GetSelection(&libname, 0);
    if (!libname)
        return;
    if (!OAif()->destroy_tech(libname, false))
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
    update();
}


void
QToaTechAttachDlg::crt_btn_slot()
{
    const char *libname;
    OAif()->GetSelection(&libname, 0);
    if (!libname)
        return;
    if (!OAif()->create_local_tech(libname))
        Log()->ErrorLog(mh::Processing, Errs()->get_error());
    update();
}


void
QToaTechAttachDlg::dismiss_btn_slot()
{
    OAif()->PopUpOAtech(0, MODE_OFF, 0, 0);
}

