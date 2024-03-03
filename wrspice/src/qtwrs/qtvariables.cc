
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

#include "config.h"
#include "qtvariables.h"
#include "simulator.h"
#include "graph.h"
#include "output.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "qttoolb.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qttextw.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#endif

#include <QLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMouseEvent>
#include <QAction>


//===========================================================================
// Pop-up to display the current shell variables.
// Buttons:
//  cancel: pop down
//  help:   bring up help panel

// Keywords referenced in help database:
//  variablespanel

void
QTtoolbar::PopUpVariables(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        delete QTvarListDlg::self();
        return;
    }
    if (QTvarListDlg::self())
        return;

    sLstr lstr;
    Sp.VarPrint(&lstr);
    new QTvarListDlg(x, y, lstr.string());
    QTvarListDlg::self()->show();
}


// Update the variables list.  Called when variables are added or deleted.
//
void
QTtoolbar::UpdateVariables()
{
    if (tb_suppress_update)
        return;
    if (!QTvarListDlg::self())
        return;
    sLstr lstr;
    Sp.VarPrint(&lstr);
    QTvarListDlg::self()->update(lstr.string());
}
// End of QTtoolbar functions.


const char *QTvarListDlg::vl_btns[] = { 0 };
QTvarListDlg *QTvarListDlg::instPtr;

QTvarListDlg::QTvarListDlg(int xx, int yy, const char *s) : QTbag(this)
{
    instPtr = this;

    setWindowTitle(tr("Variables"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // title label and help button
    //
    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(0);

    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);

    QLabel *label = new QLabel(tr("Shell variables"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("help"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // scrolled text area
    //
    wb_textarea = new QTtextEdit();
    vbox->addWidget(wb_textarea);
    wb_textarea->setReadOnly(true);
    wb_textarea->setMouseTracking(true);
    wb_textarea->setAcceptDrops(false);
    connect(wb_textarea, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));
    connect(wb_textarea, SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(mouse_release_slot(QMouseEvent*)));
    connect(wb_textarea, SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(mouse_motion_slot(QMouseEvent*)));
    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_FIXED))
        wb_textarea->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    wb_textarea->set_chars(s);

    // dismiss button row
    //
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(0);

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    TB()->FixLoc(&xx, &yy);
    TB()->SetActiveDlg(tid_variables, this);
    move(xx, yy);
}


QTvarListDlg::~QTvarListDlg()
{
    instPtr = 0;
    TB()->SetLoc(tid_variables, this);
    TB()->SetActiveDlg(tid_variables, 0);
    QTtoolbar::entries(tid_variables)->action()->setChecked(false);
}


QSize
QTvarListDlg::sizeHint() const
{
    // 80X16 text area.
    int wid = 80*QTfont::stringWidth("X", FNT_FIXED) + 4;
    int hei = height() - wb_textarea->height() +
        16*QTfont::lineHeight(FNT_FIXED);
    return (QSize(wid, hei));
}


void
QTvarListDlg::help_btn_slot()
{
    HLP()->word("variablespanel");
}


void
QTvarListDlg::update(const char *s)
{
    int val = wb_textarea->get_scroll_value();
    wb_textarea->set_chars(s);
    wb_textarea->set_scroll_value(val);
}


// The default selection and drag/drop behavior is obtained by catching
// and explicitly ignoring the mouse press, release, and motion signals.

void
QTvarListDlg::mouse_press_slot(QMouseEvent *ev)
{
    ev->ignore();
}


void
QTvarListDlg::mouse_release_slot(QMouseEvent *ev)
{
    ev->ignore();
}


void
QTvarListDlg::mouse_motion_slot(QMouseEvent *ev)
{
    ev->ignore();
}


void
QTvarListDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, fnum))
            wb_textarea->setFont(*fnt);
    }
}


void
QTvarListDlg::dismiss_btn_slot()
{
    delete instPtr;
}

