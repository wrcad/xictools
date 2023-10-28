
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

#include "qtzoom.h"
#include "main.h"
#include "dsp_inlines.h"
#include "qtmain.h"
#include <math.h>

#include <QLayout>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QGroupBox>

//-----------------------------------------------------------------------------
//  Pop-up for the Zoom command
//
// Help system keywords used:
//  xic:zoom



QTzoomDlg::QTzoomDlg(QTbag *owner, WindowDesc *w)
{
    p_parent = owner;
    zm_window = w;
    zm_autoy = 0;
    zm_yscale = 0;
    zm_zoom = 0;
    zm_x = 0;
    zm_y = 0;
    zm_wid = 0;

    if (owner)
        owner->MonitorAdd(this);

    setWindowTitle(tr("Set Display Window"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QGroupBox *gb = new QGroupBox(this);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    QLabel *lbl = new QLabel(gb);
    lbl->setText(tr("Set zoom factor or display window"));
    hb->addWidget(lbl);
    hbox->addWidget(gb);

    QPushButton *btn = new QPushButton();
    btn->setText(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    if (w->IsXSect()) {
        // Showing cross-section, add a control set for the Y-scale.

        hbox = new QHBoxLayout(0);
        hbox->setContentsMargins(qm);
        hbox->setSpacing(2);
        vbox->addLayout(hbox);

        zm_autoy = new QCheckBox();
        hbox->addWidget(zm_autoy);
        lbl = new QLabel();
        lbl->setText(tr("Auto Y-Scale"));
        hbox->addWidget(lbl);

        hbox = new QHBoxLayout(0);
        hbox->setContentsMargins(qm);
        hbox->setSpacing(2);
        vbox->addLayout(hbox);

        lbl = new QLabel();
        lbl->setText(tr("Y-Scale"));
        hbox->addWidget(lbl);
        zm_yscale = new QDoubleSpinBox();
        zm_yscale->setMaximum(CDSCALEMAX);
        zm_yscale->setMinimum(CDSCALEMIN);
        zm_yscale->setValue(1.0);
        zm_yscale->setDecimals(5);
        hbox->addWidget(zm_yscale);
        btn = new QPushButton();
        btn->setText(tr("Apply"));
        hbox->addWidget(btn);
        connect(btn, SIGNAL(clicked()), this, SLOT(y_apply_btn_slot()));
    }

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    lbl = new QLabel();
    lbl->setText(tr("Zoom Factor"));
    hbox->addWidget(lbl);
    zm_zoom = new QDoubleSpinBox();
    zm_zoom->setMaximum(CDSCALEMAX);
    zm_zoom->setMinimum(CDSCALEMIN);
    zm_zoom->setValue(1.0);
    zm_zoom->setDecimals(5);
    hbox->addWidget(zm_zoom);
    btn = new QPushButton();
    btn->setText(tr("Apply"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(z_apply_btn_slot()));

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    lbl = new QLabel();
    lbl->setText(tr("Center X,Y"));
    hbox->addWidget(lbl);
    int ndgt = w->Mode() == Physical ? CD()->numDigits() : 3;
    zm_x = new QDoubleSpinBox();
    zm_x->setMaximum(1e6);
    zm_x->setMinimum(-1e6);
    zm_x->setValue(0.0);
    zm_x->setDecimals(ndgt);
    hbox->addWidget(zm_x);
    zm_y = new QDoubleSpinBox();
    zm_y->setMaximum(1e6);
    zm_y->setMinimum(-1e6);
    zm_y->setValue(0.0);
    zm_y->setDecimals(ndgt);
    hbox->addWidget(zm_y);

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    lbl = new QLabel();
    lbl->setText(tr("Window Width"));
    hbox->addWidget(lbl);
    zm_wid = new QDoubleSpinBox();
    zm_wid->setMaximum(1e6);
    zm_wid->setMinimum(0.1);
    zm_wid->setValue(100.0);
    zm_wid->setDecimals(ndgt);
    hbox->addWidget(zm_wid);
    btn = new QPushButton();
    btn->setText(tr("Apply"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(window_apply_btn_slot()));

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    btn = new QPushButton();
    btn->setText(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTzoomDlg::~QTzoomDlg()
{
    if (p_parent) {
        QTsubwin *owner = dynamic_cast<QTsubwin*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller && !p_no_desel)
        QTdev::Deselect(p_caller);
}


// GRpopup override
//
void
QTzoomDlg::popdown()
{
    if (!p_parent)
        return;
    QTbag *owner = dynamic_cast<QTbag*>(p_parent);
//XXX    if (owner)
//XXX        QTdev::SetFocus(owner->Shell());
    if (!owner || !owner->MonitorActive(this))
        return;

    deleteLater();
}


// Set positioning and transient-for property, call before setting
// visible after creation.
//
void
QTzoomDlg::initialize()
{
    QTsubwin *w = dynamic_cast<QTsubwin*>(p_parent);
    if (w)
        QTdev::self()->SetPopupLocation(GRloc(), this, w->Viewport());
}


void
QTzoomDlg::update()
{
    int xc = (zm_window->Window()->left + zm_window->Window()->right)/2;
    int yc = (zm_window->Window()->bottom + zm_window->Window()->top)/2;
    int w = zm_window->Window()->width();

    if (zm_window->Mode() == Physical) {
        double d = zm_x->value();
        if (INTERNAL_UNITS(d) != xc)
            zm_x->setValue(MICRONS(xc));

        d = zm_y->value();
        if (INTERNAL_UNITS(d) != yc)
            zm_y->setValue(MICRONS(yc));

        d = zm_wid->value();
        if (INTERNAL_UNITS(d) != w)
            zm_wid->setValue(MICRONS(w));
    }
    else {
        double d = zm_x->value();
        if (ELEC_INTERNAL_UNITS(d) != xc)
            zm_x->setValue(ELEC_MICRONS(xc));

        d = zm_y->value();
        if (ELEC_INTERNAL_UNITS(d) != yc)
            zm_y->setValue(ELEC_MICRONS(yc));

        d = zm_wid->value();
        if (ELEC_INTERNAL_UNITS(d) != w)
            zm_wid->setValue(ELEC_MICRONS(w));
    }
    if (zm_window->IsXSect()) {
        QTdev::SetStatus(zm_autoy, zm_window->IsXSectAutoY());
        zm_yscale->setValue(zm_window->XSectYScale());
    }
}


void
QTzoomDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:zoom"))
}


void
QTzoomDlg::y_apply_btn_slot()
{
    double ysc = zm_yscale->value();
    zm_window->SetXSectYScale(ysc);
    if (fabs(ysc - 1.0) < 1e-9)
        CDvdb()->clearVariable(VA_XSectYScale);
    else {
        const char *str = lstring::copy(
            zm_yscale->textFromValue(zm_yscale->value()).toLatin1());
        CDvdb()->setVariable(VA_XSectYScale, str);
        delete [] str;
    }

    bool autoy = QTdev::GetStatus(zm_autoy);
    zm_window->SetXSectAutoY(autoy);
    if (autoy)
        CDvdb()->clearVariable(VA_XSectNoAutoY);
    else
        CDvdb()->setVariable(VA_XSectNoAutoY, "");
    zm_window->Redisplay(0);
}


void
QTzoomDlg::z_apply_btn_slot()
{
    double d = zm_zoom->value();
    WindowDesc *wdesc = zm_window;
    d *= wdesc->Window()->width();
    wdesc->InitWindow(
        (wdesc->Window()->left + wdesc->Window()->right)/2,
        (wdesc->Window()->top + wdesc->Window()->bottom)/2, d);
    wdesc->Redisplay(0);
}


void
QTzoomDlg::window_apply_btn_slot()
{
    double dx = zm_x->value();
    double dy = zm_y->value();
    double dw = zm_wid->value();
    WindowDesc *wdesc = zm_window;
    if (wdesc->Mode() == Physical) {
        wdesc->InitWindow(INTERNAL_UNITS(dx), INTERNAL_UNITS(dy),
            dw*CDphysResolution);
    }
    else {
        wdesc->InitWindow(ELEC_INTERNAL_UNITS(dx), ELEC_INTERNAL_UNITS(dy),
            dw*CDelecResolution);
    }
    wdesc->Redisplay(0);
}


void
QTzoomDlg::dismiss_btn_slot()
{
    popdown();
}

