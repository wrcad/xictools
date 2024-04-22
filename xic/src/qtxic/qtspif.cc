
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

#include "qtspif.h"
#include "sced.h"
#include "edit_variables.h"
#include "cvrt_variables.h"
#include "dsp_inlines.h"

#include <QApplication>
#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>


//-----------------------------------------------------------------------------
// QTspiceIfDlg:  Dialog to control SPICE interface.
// Called from the spcmd button in the Electrical side menu if the user
// does not give a command at the prompt.
//
// Help system keywords used:
//  xic:spif

void
cSced::PopUpSpiceIf(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTspiceIfDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTspiceIfDlg::self())
            QTspiceIfDlg::self()->update();
        return;
    }
    if (QTspiceIfDlg::self())
        return;

    new QTspiceIfDlg(caller);

    QTspiceIfDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_UL), QTspiceIfDlg::self(),
        QTmainwin::self()->Viewport());
    QTspiceIfDlg::self()->show();
}
// End of cSced functions.


QTspiceIfDlg *QTspiceIfDlg::instPtr;

QTspiceIfDlg::QTspiceIfDlg(GRobject c)
{
    instPtr = this;
    sc_caller = c;
    sc_listall = 0;
    sc_checksol = 0;
    sc_notools = 0;
    sc_alias = 0;
    sc_alias = 0;
    sc_hostname = 0;
    sc_hostname_b = 0;
    sc_dispname = 0;
    sc_dispname_b = 0;
    sc_progname = 0;
    sc_progname_b = 0;
    sc_execdir = 0;
    sc_execdir_b = 0;
    sc_execname = 0;
    sc_execname_b = 0;
    sc_catchar = 0;
    sc_catchar_b = 0;
    sc_catmode = 0;
    sc_catmode_b = 0;

    setWindowTitle(tr("WRspice Interface Control Panel"));
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
    QLabel *label = new QLabel(tr("WRspice Interface Options"));
    hb->addWidget(label);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // WRspice interface controls
    //
    QGridLayout *grid = new QGridLayout();
    vbox->addLayout(grid);

    sc_listall = new QCheckBox(tr("List all devices and subcircuits."));
    grid->addWidget(sc_listall, 0, 0, 1, 2);
    connect(sc_listall, SIGNAL(stateChanged(int)),
        this, SLOT(listall_btn_slot(int)));

    sc_checksol = new QCheckBox(tr("Check and report solitary connections."));
    grid->addWidget(sc_checksol, 1, 0, 1, 2);
    connect(sc_checksol, SIGNAL(stateChanged(int)),
        this, SLOT(checksol_btn_slot(int)));

    sc_notools = new QCheckBox(tr("Don't show WRspice Tool Control panel."));
    grid->addWidget(sc_notools, 2, 0, 1, 2);
    connect(sc_checksol, SIGNAL(stateChanged(int)),
        this, SLOT(notools_btn_slot(int)));

    sc_alias_b = new QCheckBox(tr("Spice device prefix aliases"));
    grid->addWidget(sc_alias_b, 3, 0);
    connect(sc_alias_b, SIGNAL(stateChanged(int)),
        this, SLOT(alias_btn_slot(int)));

    sc_alias = new QLineEdit();
    grid->addWidget(sc_alias, 3, 1);

    sc_hostname_b = new QCheckBox(tr("Remote WRspice server host\nname"));
    grid->addWidget(sc_hostname_b, 4, 0);
    connect(sc_hostname_b, SIGNAL(stateChanged(int)),
        this, SLOT(hostname_btn_slot(int)));

    sc_hostname = new QLineEdit();
    grid->addWidget(sc_hostname, 4, 1);

    sc_dispname_b = new QCheckBox(tr(
        "Remote WRspice server host\ndisplay name"));
    grid->addWidget(sc_dispname_b, 5, 0);
    connect(sc_dispname_b, SIGNAL(stateChanged(int)),
        this, SLOT(dispname_btn_slot(int)));

    sc_dispname = new QLineEdit();
    grid->addWidget(sc_dispname, 5, 1);

    sc_progname_b = new QCheckBox(tr("Path to local WRspice\nexecutable"));
    grid->addWidget(sc_progname_b, 6, 0);
    connect(sc_progname_b, SIGNAL(stateChanged(int)),
        this, SLOT(progname_btn_slot(int)));

    sc_progname = new QLineEdit();
    grid->addWidget(sc_progname, 6, 1);

    sc_execdir_b = new QCheckBox(tr(
        "Path to local directory\ncontaining WRspice executable"));
    grid->addWidget(sc_execdir_b, 7, 0);
    connect(sc_execdir_b, SIGNAL(stateChanged(int)),
        this, SLOT(execdir_btn_slot(int)));

    sc_execdir = new QLineEdit();
    grid->addWidget(sc_execdir, 7, 1);

    sc_execname_b = new QCheckBox(tr(
        "Assumed WRspice program\nexecutable name"));
    grid->addWidget(sc_execname_b, 8, 0);
    connect(sc_execname_b, SIGNAL(stateChanged(int)),
        this, SLOT(execname_btn_slot(int)));

    sc_execname = new QLineEdit();
    grid->addWidget(sc_execname, 8, 1);

    sc_catchar_b = new QCheckBox(tr(
        "Assumed WRspice subcircuit\nconcatenation character"));
    grid->addWidget(sc_catchar_b, 9, 0);
    connect(sc_catchar_b, SIGNAL(stateChanged(int)),
        this, SLOT(catchar_btn_slot(int)));

    sc_catchar = new QLineEdit();
    grid->addWidget(sc_catchar, 9, 1);

    sc_catmode_b = new QCheckBox(tr(
        "Assumed WRspice subcircuit\nexpansion mode"));
    grid->addWidget(sc_catmode_b, 10, 0);
    connect(sc_catmode_b, SIGNAL(stateChanged(int)),
        this, SLOT(catmode_btn_slot(int)));

    sc_catmode = new QComboBox();
    grid->addWidget(sc_catmode, 10, 1);
    sc_catmode->addItem("WRspice");
    sc_catmode->addItem("Spice3");

    // Dismiss button
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    grid->addWidget(btn, 11, 0, 1, 2);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTspiceIfDlg::~QTspiceIfDlg()
{
    instPtr = 0;
    if (sc_caller)
        QTdev::Deselect(sc_caller);
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTspiceIfDlg
#include "qtinterf/qtmacos_event.h"
#endif


void
QTspiceIfDlg::update()
{
    QTdev::SetStatus(sc_listall,
        CDvdb()->getVariable(VA_SpiceListAll));
    QTdev::SetStatus(sc_checksol,
        CDvdb()->getVariable(VA_CheckSolitary));
    QTdev::SetStatus(sc_notools,
        CDvdb()->getVariable(VA_NoSpiceTools));

    const char *s = CDvdb()->getVariable(VA_SpiceAlias);
    const char *t = lstring::copy(sc_alias->text().toLatin1().constData());
    if (s && t && strcmp(s, t))
        sc_alias->setText(s);
    bool set = (s != 0);
    sc_alias->setEnabled(!set);
    QTdev::SetStatus(sc_alias_b, set);
    delete [] t;

    s = CDvdb()->getVariable(VA_SpiceHost);
    t = lstring::copy(sc_hostname->text().toLatin1().constData());
    if (s && t && strcmp(s, t))
        sc_hostname->setText(s);
    set = (s != 0);
    sc_hostname->setEnabled(!set);
    QTdev::SetStatus(sc_hostname_b, set);
    delete [] t;

    s = CDvdb()->getVariable(VA_SpiceHostDisplay);
    t = lstring::copy(sc_dispname->text().toLatin1().constData());
    if (s && t && strcmp(s, t))
        sc_dispname->setText(s);
    set = (s != 0);
    sc_dispname->setEnabled(!set);
    QTdev::SetStatus(sc_dispname_b, set);
    delete [] t;

    s = CDvdb()->getVariable(VA_SpiceProg);
    t = lstring::copy(sc_progname->text().toLatin1().constData());
    if (s && t && strcmp(s, t))
        sc_progname->setText(s);
    set = (s != 0);
    sc_progname->setEnabled(!set);
    QTdev::SetStatus(sc_progname_b, set);
    delete [] t;

    s = CDvdb()->getVariable(VA_SpiceExecDir);
    t = lstring::copy(sc_execdir->text().toLatin1().constData());
    if (s) {
        if (t && strcmp(s, t))
            sc_execdir->setText(s);
    }
    else
        sc_execdir->setText(XM()->ExecDirectory());
    set = (s != 0);
    sc_execdir->setEnabled(!set);
    QTdev::SetStatus(sc_execdir_b, set);
    delete [] t;

    s = CDvdb()->getVariable(VA_SpiceExecName);
    t = lstring::copy(sc_execname->text().toLatin1().constData());
    if (s) {
        if (t && strcmp(s, t))
            sc_execname->setText(s);
    }
    else
        sc_execname->setText(XM()->SpiceExecName());
    set = (s != 0);
    sc_execname->setEnabled(!set);
    QTdev::SetStatus(sc_execname_b, set);
    delete [] t;

    s = CDvdb()->getVariable(VA_SpiceSubcCatchar);
    t = lstring::copy(sc_catchar->text().toLatin1().constData());
    if (s) {
        if (t && strcmp(s, t))
            sc_catchar->setText(s);
    }
    else {
        char tmp[2];
        tmp[0] = CD()->GetSubcCatchar();
        tmp[1] = 0;
        sc_catchar->setText(tmp);
    }
    set = (s != 0);
    sc_catchar->setEnabled(!set);
    QTdev::SetStatus(sc_catchar_b, set);

    s = CDvdb()->getVariable(VA_SpiceSubcCatmode);
    int h = sc_catmode->currentIndex();
    set = (s != 0);
    if (s) {
        if (*s == 'w' || *s == 'W') {
            if (h != 0)
                sc_catmode->setCurrentIndex(0);
        }
        else if (*s == 's' || *s == 'S') {
            if (h != 1)
                sc_catmode->setCurrentIndex(1);
        }
        else {
            CDvdb()->clearVariable(VA_SpiceSubcCatmode);
            set = false;
        }
    }
    else {
        if (CD()->GetSubcCatmode() == cCD::SUBC_CATMODE_WR && h != 0)
            sc_catmode->setCurrentIndex(0);
        else if (CD()->GetSubcCatmode() == cCD::SUBC_CATMODE_SPICE3 && h != 1)
            sc_catmode->setCurrentIndex(1);
    }
    sc_catmode->setEnabled(!set);
    QTdev::SetStatus(sc_catmode_b, set);
}


void
QTspiceIfDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:spif"))
}


void
QTspiceIfDlg::listall_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_SpiceListAll, "");
    else
        CDvdb()->clearVariable(VA_SpiceListAll);
}


void
QTspiceIfDlg::checksol_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_CheckSolitary, "");
    else
        CDvdb()->clearVariable(VA_CheckSolitary);
}


void
QTspiceIfDlg::notools_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoSpiceTools, "");
    else
        CDvdb()->clearVariable(VA_NoSpiceTools);
}


void
QTspiceIfDlg::alias_btn_slot(int state)
{
    if (state) {
        const char *text =
            lstring::copy(sc_alias->text().toLatin1().constData());
        CDvdb()->setVariable(VA_SpiceAlias, text);
        delete [] text;
    }
    else
        CDvdb()->clearVariable(VA_SpiceAlias);
}


void
QTspiceIfDlg::hostname_btn_slot(int state)
{
    if (state) {
        const char *text =
            lstring::copy(sc_hostname->text().toLatin1().constData());
        CDvdb()->setVariable(VA_SpiceHost, text);
        delete [] text;
    }
    else
        CDvdb()->clearVariable(VA_SpiceHost);
}


void
QTspiceIfDlg::dispname_btn_slot(int state)
{
    if (state) {
        const char *text =
            lstring::copy(sc_dispname->text().toLatin1().constData());
        CDvdb()->setVariable(VA_SpiceHostDisplay, text);
        delete [] text;
    }
    else
        CDvdb()->clearVariable(VA_SpiceHostDisplay);
}


void
QTspiceIfDlg::progname_btn_slot(int state)
{
    if (state) {
        const char *text =
            lstring::copy(sc_progname->text().toLatin1().constData());
        CDvdb()->setVariable(VA_SpiceProg, text);
        delete [] text;
    }
    else
        CDvdb()->clearVariable(VA_SpiceProg);
}


void
QTspiceIfDlg::execdir_btn_slot(int state)
{
    if (state) {
        const char *text =
            lstring::copy(sc_execdir->text().toLatin1().constData());
        CDvdb()->setVariable(VA_SpiceExecDir, text);
        delete [] text;
    }
    else
        CDvdb()->clearVariable(VA_SpiceExecDir);
}


void
QTspiceIfDlg::execname_btn_slot(int state)
{
    if (state) {
        const char *text =
            lstring::copy(sc_execname->text().toLatin1().constData());
        CDvdb()->setVariable(VA_SpiceExecName, text);
        delete [] text;
    }
    else
        CDvdb()->clearVariable(VA_SpiceExecName);
}


void
QTspiceIfDlg::catchar_btn_slot(int state)
{
    if (state) {
        const char *text =
            lstring::copy(sc_catchar->text().toLatin1().constData());
        CDvdb()->setVariable(VA_SpiceSubcCatchar, text);
        delete [] text;
    }
    else
        CDvdb()->clearVariable(VA_SpiceSubcCatchar);
}


void
QTspiceIfDlg::catmode_btn_slot(int state)
{
    if (state) {
        int h = sc_catmode->currentIndex();
        CDvdb()->setVariable(VA_SpiceSubcCatmode,
            h ? "Spice3" : "WRspice");
    }
    else
        CDvdb()->clearVariable(VA_SpiceSubcCatmode);
}


void
QTspiceIfDlg::dismiss_btn_slot()
{
    SCD()->PopUpSpiceIf(0, MODE_OFF);
}

