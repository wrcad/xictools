
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
#include "qtplots.h"
#include "simulator.h"
#include "graph.h"
#include "output.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "qttoolb.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qttextw.h"
#include "qtinterf/qtaffirm.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#endif

#include <QLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMouseEvent>
#include <QAction>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>


//===========================================================================
// Dialog to display a list of the plots.  Clicking in the list selects the
// 'current' plot.
// Buttons:
//  cancel: pop down
//  help:   bring up help panel
//  new:    create and add a 'new' plot, making it current
//  delete: delete the current plot

// Keywords referenced in help database:
//  plotspanel

void
QTtoolbar::PopUpPlots(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        delete QTplotListDlg::self();
        return;
    }
    if (QTplotListDlg::self())
        return;

    sLstr lstr;
    for (sPlot *p = OP.plotList(); p; p = p->next_plot()) {
        char buf[256];
        if (OP.curPlot() == p)
            snprintf(buf, sizeof(buf), "Current %-11s%-20s (%s)\n",
            p->type_name(), p->title(), p->name());
        else
            snprintf(buf, sizeof(buf), "        %-11s%-20s (%s)\n",
                p->type_name(), p->title(), p->name());
        lstr.add(buf);
    }
    new QTplotListDlg(x, y, lstr.string());
    QTplotListDlg::self()->show();
}


// Update the list of plots.  This should be called whenever the internal
// plot list changes.  If called with 'lev' nonzero, defer updates until
// second call with same lev value.
//
void
QTtoolbar::UpdatePlots(int lev)
{
    static int level;
    if (tb_suppress_update)
        return;
    UpdateVectors(lev);
    if (!QTplotListDlg::self())
        return;
    if (lev) {
        if (level < lev) {
            level = lev;
            return;
        }
        else if (level == lev)
            level = 0;
        else
            return;
    }
    else if (level)
        return;

    char buf[512];
    sLstr lstr;
    for (sPlot *p = OP.plotList(); p; p = p->next_plot()) {
        if (OP.curPlot() == p) {
            snprintf(buf, sizeof(buf), "Current %-11s%-20s (%s)\n",
            p->type_name(), p->title(), p->name());
        }
        else {
            snprintf(buf, sizeof(buf), "        %-11s%-20s (%s)\n",
                p->type_name(), p->title(), p->name());
        }
        lstr.add(buf);
    }
    QTplotListDlg::self()->update(lstr.string());
}
// End of QTtoolbar functions.


const char *QTplotListDlg::pl_btns[] = { "New Plot", "Delete Current", 0 };
QTplotListDlg *QTplotListDlg::instPtr;

QTplotListDlg::QTplotListDlg(int xx, int yy, const char *s) : QTbag(this)
{
    instPtr = this;
    pl_x = 0;
    pl_y = 0;
    pl_affirm = 0;

    setWindowTitle(tr("Plots"));
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

    QLabel *label = new QLabel(tr("Currently active plots"));
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

    for (int n = 0; pl_btns[n]; n++) {
        btn = new QPushButton(tr(pl_btns[n]));
        hbox->addWidget(btn);
        btn->setCheckable(true);
        btn->setAutoDefault(false);
        connect(btn, SIGNAL(toggled(bool)),
            this, SLOT(button_slot(bool)));
    }

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    TB()->FixLoc(&xx, &yy);
    TB()->SetActiveDlg(tid_plots, this);
    move(xx, yy);
}


QTplotListDlg::~QTplotListDlg()
{
    instPtr = 0;
    TB()->SetLoc(tid_plots, this);
    TB()->SetActiveDlg(tid_plots, 0);
    QTtoolbar::entries(tid_plots)->action()->setChecked(false);
}


QSize
QTplotListDlg::sizeHint() const
{
    // 80X16 text area.
    int wid = 80*QTfont::stringWidth("X", FNT_FIXED) + 4;
    int hei = height() - wb_textarea->height() +
        16*QTfont::lineHeight(FNT_FIXED);
    return (QSize(wid, hei));
}


void
QTplotListDlg::help_btn_slot()
{
    HLP()->word("plotspanel");
}


void
QTplotListDlg::update(const char *s)
{
    int val = wb_textarea->get_scroll_value();
    wb_textarea->set_chars(s);
    wb_textarea->set_scroll_value(val);
}


void
QTplotListDlg::mouse_press_slot(QMouseEvent *ev)
{
    ev->ignore();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    pl_x = ev->position().x();
    pl_y = ev->position().y();
#else
    pl_x = ev->x();
    pl_y = ev->y();
#endif
}


void
QTplotListDlg::mouse_release_slot(QMouseEvent *ev)
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    if (fabs(xx - pl_x) < 5 && fabs(yy - pl_y) < 5) {
        // Clicked, accept to prevent selection or drag/drop.
        ev->accept();
        int vsv = wb_textarea->verticalScrollBar()->value();
        int hsv = wb_textarea->horizontalScrollBar()->value();
        QByteArray ba = wb_textarea->toPlainText().toLatin1();
        const char *str = ba.constData();
        int posn = wb_textarea->document()->documentLayout()->hitTest(
            QPointF(xx + hsv, yy + vsv), Qt::ExactHit);

        int line = 0;
        for (int i = 0; i <= posn; i++) {
            if (str[i] == '\n') {
                line++;
                if (i == posn) {
                    // Clicked to  right of line.
                    break;
                }
            }
        }

        sPlot *p;
        int i;
        for (i = 0, p = OP.plotList(); p; i++, p = p->next_plot()) {
            if (i == line)
                break;
        }
        if (p) {
            OP.setCurPlot(p);
            OP.curPlot()->run_commands();
            TB()->UpdatePlots(0);
            if (p->circuit()) {
                Sp.OptUpdate();
                Sp.SetCircuit(p->circuit());
            }
        }
        return;
    }
    ev->ignore();
}


void
QTplotListDlg::mouse_motion_slot(QMouseEvent *ev)
{
    ev->ignore();
}


void
QTplotListDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, fnum))
            wb_textarea->setFont(*fnt);
    }
}


void
QTplotListDlg::button_slot(bool state)
{
    QPushButton *btn = dynamic_cast<QPushButton*>(sender());
    if (!btn)
        return;
    QString btxt = btn->text();

    for (int n = 0; pl_btns[n]; n++) {
        if (btxt == pl_btns[n]) {
            if (n == 0) {
                if (state) {
                    // 'New' button pressed, create a new plot and make it
                    // current.
                    OP.setCurPlot("new");
                    QTdev::Deselect(btn);
                }
                return;
            }
            if (n == 1) {
                // 'Delete' button pressed, ask for confirmation.
                if (!state) {
                    if (pl_affirm)
                        pl_affirm->popdown();
                    return;
                }
                if (OP.curPlot() == OP.constants()) {
                    GRpkg::self()->ErrPrintf(ET_ERROR,
                        "can't destroy constants plot.\n");
                    QTdev::Deselect(btn);
                    return;
                }
                pl_affirm = PopUpAffirm(this, GRloc(),
                    "Delete Current Plot?", 0, 0);
                if (pl_affirm) {
                    pl_affirm->register_usrptr((void**)&pl_affirm);
                    pl_affirm->register_caller(btn, false, false);
                    QTaffirmDlg *af = dynamic_cast<QTaffirmDlg*>(pl_affirm);
                    connect(af, SIGNAL(affirm(bool, void*)),
                        this, SLOT(delete_plot_slot(bool, void*)));
                }
            }
        }
    }
}


void
QTplotListDlg::dismiss_btn_slot()
{
    delete instPtr;
}


void
QTplotListDlg::delete_plot_slot(bool yn, void*)
{
    if (yn)
        OP.removePlot(OP.curPlot());
}

