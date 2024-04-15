
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

#include "qtoadefs.h"
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
#include <QCheckBox>


//-----------------------------------------------------------------------------
// QToaDefsDlg:  Panel for setting misc. OpenAccess interface parameters.
// Called from the OpenAccess Libraries panel (QToaLibsDlg) Defaults
// button.
//
// Help keyword:
// xic:oadefs

void
cOAif::PopUpOAdefs(GRobject caller, ShowMode mode, int x, int y)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QToaDefsDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QToaDefsDlg::self())
            QToaDefsDlg::self()->update();
        return;
    }
    if (QToaDefsDlg::self()) {
        QToaDefsDlg::self()->update();
        return;
    }

    new QToaDefsDlg(caller);

    QToaDefsDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_XYA, x, y),
        QToaDefsDlg::self(), QTmainwin::self()->Viewport());
    QToaDefsDlg::self()->show();
}
// End of cOAif functions.


QToaDefsDlg *QToaDefsDlg::instPtr;

QToaDefsDlg::QToaDefsDlg(GRobject c)
{
    instPtr = this;
    od_caller = c;
    od_path = 0;
    od_lib = 0;
    od_techlib = 0;
    od_layout = 0;
    od_schem = 0;
    od_symb = 0;
    od_prop = 0;
    od_cdf = 0;

    setWindowTitle(tr("OpenAccess Defaults"));
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
    QLabel *label = new QLabel(tr("Set interface defaults and options"));
    hb->addWidget(label);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    gb = new QGroupBox(tr("Library Path"));
    vbox->addWidget(gb);
    hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qmtop);
    hbox->setSpacing(2);

    od_path = new QLineEdit();
    hbox->addWidget(od_path);
    connect(od_path, SIGNAL(textChanged(const QString&)),
        this, SLOT(path_text_changed(const QString&)));

    QGridLayout *grid = new QGridLayout();
    vbox->addLayout(grid);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    label = new QLabel(tr("Default Library"));
    grid->addWidget(label, 0, 0);

    od_lib = new QLineEdit();
    grid->addWidget(od_lib, 0, 1);
    connect(od_lib, SIGNAL(textChanged(const QString&)),
        this, SLOT(lib_text_changed(const QString&)));

    label = new QLabel(tr("Default Tech Library"));
    grid->addWidget(label, 1, 0);

    od_techlib = new QLineEdit();
    grid->addWidget(od_techlib, 1, 1);
    connect(od_techlib, SIGNAL(textChanged(const QString&)),
        this, SLOT(tech_text_changed(const QString&)));

    label = new QLabel(tr("Default Layout View"));
    grid->addWidget(label, 2, 0);

    od_layout = new QLineEdit();
    grid->addWidget(od_layout, 2, 1);
    connect(od_layout, SIGNAL(textChanged(const QString&)),
        this, SLOT(lview_text_changed(const QString&)));

    label = new QLabel(tr("Default Schematic View"));
    grid->addWidget(label, 3, 0);

    od_schem = new QLineEdit();
    grid->addWidget(od_schem, 3, 1);;
    connect(od_schem, SIGNAL(textChanged(const QString&)),
        this, SLOT(schview_text_changed(const QString&)));

    label = new QLabel(tr("Default Symbol View"));
    grid->addWidget(label, 4, 0);

    od_symb = new QLineEdit();
    grid->addWidget(od_symb, 4, 1);
    connect(od_symb, SIGNAL(textChanged(const QString&)),
        this, SLOT(symview_text_changed(const QString&)));
    
    label = new QLabel(tr("Default Properties View"));
    grid->addWidget(label, 5, 0);

    od_prop = new QLineEdit;
    grid->addWidget(od_prop, 5, 1);
    connect(od_prop, SIGNAL(textChanged(const QString&)),
        this, SLOT(prop_text_changed(const QString&)));

    od_cdf = new QCheckBox(tr("Dump CDF files when reading"));
    vbox->addWidget(od_cdf);
    connect(od_cdf, SIGNAL(stateChangred(int)),
        this, SLOT(cdf_btn_slot(int)));

    // Dismiss button
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Dismiss");
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QToaDefsDlg::~QToaDefsDlg()
{
    instPtr = 0;
    if (od_caller)
        QTdev::Deselect(od_caller);
}


#ifdef Q_OS_MACOS

bool
QToaDefsDlg::event(QEvent *ev)
{
    // Fix for QT BUG 116674, text becomes invisible on autodefault
    // button when the main window has focus.

    if (ev->type() == QEvent::ActivationChange) {
        QPushButton *dsm = findChild<QPushButton*>("Dismiss",
            Qt::FindDirectChildrenOnly);
        if (dsm) {
            QWidget *top = this;
            while (top->parentWidget())
                top = top->parentWidget();
            if (QApplication::activeWindow() == top)
                dsm->setDefault(false);
            else if (QApplication::activeWindow() == this)
                dsm->setDefault(true);
        }
    }
    return (QDialog::event(ev));
}

#endif


void
QToaDefsDlg::update()
{
    const char *s = CDvdb()->getVariable(VA_OaLibraryPath);
    if (!s)
        s = "";
    QByteArray path_ba = od_path->text().toLatin1();
    const char *t = path_ba.constData();
    if (!t)
        t = "";
    if (strcmp(s, t))
        od_path->setText(s);

    s = CDvdb()->getVariable(VA_OaDefLibrary);
    if (!s)
        s = "";
    QByteArray lib_ba = od_lib->text().toLatin1();
    t = lib_ba.constData();
    if (!t)
        t = "";
    if (strcmp(s, t))
        od_lib->setText(s);

    s = CDvdb()->getVariable(VA_OaDefTechLibrary);
    if (!s)
        s = "";
    QByteArray tech_ba = od_techlib->text().toLatin1();
    t = tech_ba.constData();
    if (!t)
        t = "";
    if (strcmp(s, t))
        od_techlib->setText(s);

    s = CDvdb()->getVariable(VA_OaDefLayoutView);
    if (!s)
        s = "";
    QByteArray layout_ba = od_layout->text().toLatin1();
    t = layout_ba.constData();
    if (!t)
        t = "";
    if (strcmp(s, t))
        od_layout->setText(s);

    s = CDvdb()->getVariable(VA_OaDefSchematicView);
    if (!s)
        s = "";
    QByteArray schem_ba = od_schem->text().toLatin1();
    t = schem_ba.constData();
    if (!t)
        t = "";
    if (strcmp(s, t))
        od_schem->setText(s);

    s = CDvdb()->getVariable(VA_OaDefSymbolView);
    if (!s)
        s = "";
    QByteArray symb_ba = od_symb->text().toLatin1();
    t = symb_ba.constData();
    if (!t)
        t = "";
    if (strcmp(s, t))
        od_symb->setText(s);

    s = CDvdb()->getVariable(VA_OaDefDevPropView);
    if (!s)
        s = "";
    QByteArray prop_ba = od_prop->text().toLatin1();
    t = prop_ba.constData();
    if (!t)
        t = "";
    if (strcmp(s, t))
        od_prop->setText(s);

    QTdev::SetStatus(od_cdf, CDvdb()->getVariable(VA_OaDumpCdfFiles));
}


void
QToaDefsDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:oadefs"))
}


void
QToaDefsDlg::path_text_changed(const QString &qs)
{
    QByteArray qba = qs.toLatin1();
    const char *t = qba.constData();
    if (!t || !*t)
        CDvdb()->clearVariable(VA_OaLibraryPath);
    else
        CDvdb()->setVariable(VA_OaLibraryPath, t);
}


void
QToaDefsDlg::lib_text_changed(const QString &qs)
{
    QByteArray qba = qs.toLatin1();
    const char *t = qba.constData();
    if (!t || !*t)
        CDvdb()->clearVariable(VA_OaDefLibrary);
    else
        CDvdb()->setVariable(VA_OaDefLibrary, t);
}


void
QToaDefsDlg::tech_text_changed(const QString &qs)
{
    QByteArray qba = qs.toLatin1();
    const char *t = qba.constData();
    if (!t || !*t)
        CDvdb()->clearVariable(VA_OaDefTechLibrary);
    else
        CDvdb()->setVariable(VA_OaDefTechLibrary, t);
}


void
QToaDefsDlg::lview_text_changed(const QString &qs)
{
    QByteArray qba = qs.toLatin1();
    const char *t = qba.constData();
    if (!t || !*t)
        CDvdb()->clearVariable(VA_OaDefLayoutView);
    else
        CDvdb()->setVariable(VA_OaDefLayoutView, t);
}


void
QToaDefsDlg::schview_text_changed(const QString &qs)
{
    QByteArray qba = qs.toLatin1();
    const char *t = qba.constData();
    if (!t || !*t)
        CDvdb()->clearVariable(VA_OaDefSchematicView);
    else
        CDvdb()->setVariable(VA_OaDefSchematicView, t);
}


void
QToaDefsDlg::symview_text_changed(const QString &qs)
{
    QByteArray qba = qs.toLatin1();
    const char *t = qba.constData();
    if (!t || !*t)
        CDvdb()->clearVariable(VA_OaDefSymbolView);
    else
        CDvdb()->setVariable(VA_OaDefSymbolView, t);
}


void
QToaDefsDlg::prop_text_changed(const QString &qs)
{
    QByteArray qba = qs.toLatin1();
    const char *t = qba.constData();
    if (!t || !*t)
        CDvdb()->clearVariable(VA_OaDefDevPropView);
    else
        CDvdb()->setVariable(VA_OaDefDevPropView, t);
}


void
QToaDefsDlg::cdf_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_OaDumpCdfFiles, "");
    else
        CDvdb()->clearVariable(VA_OaDumpCdfFiles);
}


void
QToaDefsDlg::dismiss_btn_slot()
{
    OAif()->PopUpOAdefs(0, MODE_OFF, 0, 0);
}

