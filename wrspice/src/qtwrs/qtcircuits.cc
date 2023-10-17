
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
#include "qtcircuits.h"
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


//===========================================================================
// Dialog that displays a list of the circuits that have been loaded.
// Clicking on an entry will make it the 'current' circuit.
// Buttons:
//  cancel: pop down
//  help:   bring up help panel
//  delete: delete the current circuit

// Keywords referenced in help database:
//  circuitspanel

void
QTtoolbar::PopUpCircuits(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        if (QTcircuitListDlg::self())
            QTcircuitListDlg::self()->deleteLater();
        return;
    }
    if (QTcircuitListDlg::self())
        return;

    char *s = QTcircuitListDlg::circuit_list();
    new QTcircuitListDlg(x, y, s);
    delete [] s;
    QTcircuitListDlg::self()->show();
}


// Update the circuit list.  Called when circuit list changes.
//
void
QTtoolbar::UpdateCircuits()
{
    if (tb_suppress_update)
        return;
    if (!QTcircuitListDlg::self())
        return;

    char *s = QTcircuitListDlg::circuit_list();
    QTcircuitListDlg::self()->update(s);
    delete [] s;
}
// End of QTtoolbar functions.


const char *QTcircuitListDlg::cl_btns[] = { "Delete Current", 0 };
QTcircuitListDlg *QTcircuitListDlg::instPtr;

QTcircuitListDlg::QTcircuitListDlg(int xx, int yy, const char *s)
{
    instPtr = this;
    cl_affirm = 0;

    setWindowTitle(tr("Circuits"));
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

    QLabel *label = new QLabel(tr("Currently active circuits"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("help"));
    hbox->addWidget(btn);
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
    connect(wb_textarea, SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(mouse_motion_slot(QMouseEvent*)));
//    connect(wb_textarea, SIGNAL(mime_data_recieved(const QMimeData*)),
//        this, SLOT(mime_data_received_slot(const QMimeData*)));
    QFont *fnt;
    if (FC.getFont(&fnt, FNT_FIXED))
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

    for (int n = 0; cl_btns[n]; n++) {
        btn = new QPushButton(tr(cl_btns[n]));
        btn->setCheckable(true);
        hbox->addWidget(btn);
        connect(btn, SIGNAL(toggled(bool)),
            this, SLOT(button_slot(bool)));
    }

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    TB()->FixLoc(&xx, &yy);
    TB()->SetActiveDlg(tid_circuits, this);
    move(xx, yy);
}


QTcircuitListDlg::~QTcircuitListDlg()
{
    instPtr = 0;
    TB()->SetLoc(tid_circuits, this);
    TB()->SetActiveDlg(tid_circuits, 0);
    QTtoolbar::entries(tid_circuits)->action()->setChecked(false);
}


QSize
QTcircuitListDlg::sizeHint() const
{
    // 80X16 text area.
    int wid = 80*QTfont::stringWidth("X", FNT_FIXED) + 4;
    int hei = height() - wb_textarea->height() +
        16*QTfont::lineHeight(FNT_FIXED);
    return (QSize(wid, hei));
}


void
QTcircuitListDlg::help_btn_slot()
{
    HLP()->word("circuitspanel");
}


void
QTcircuitListDlg::update(const char *s)
{
    int val = wb_textarea->get_scroll_value();
    wb_textarea->set_chars(s);
    wb_textarea->set_scroll_value(val);
}


// Static function.
// Create a string containing the circuit list.
//
char *
QTcircuitListDlg::circuit_list()
{
    if (!Sp.CircuitList())
        return (lstring::copy("There are no circuits loaded."));

    sLstr lstr;
    for (sFtCirc *p = Sp.CircuitList(); p; p = p->next()) {
        char buf[512];
        if (Sp.CurCircuit() == p) {
            snprintf(buf, sizeof(buf), "Current %-6s %s\n", p->name(),
                p->descr());
        }
        else {
            snprintf(buf, sizeof(buf), "        %-6s %s\n", p->name(),
                p->descr());
        }
        lstr.add(buf);
    }
    return (lstr.string_trim());
}


void
QTcircuitListDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    if (!instPtr)
        return;

    QByteArray ba = wb_textarea->toPlainText().toLatin1();
    const char *str = ba.constData();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    QTextCursor cur = wb_textarea->cursorForPosition(QPoint(xx, yy));
    int posn = cur.position();

//    const char *lineptr = str;
    int line = 0;
    for (int i = 0; i <= posn; i++) {
        if (str[i] == '\n') {
            if (i == posn) {
                // Clicked to  right of line.
                break;
            }
            line++;
//            lineptr = str + i+1;
        }
    }
    sFtCirc *p;
    int i = 0;
    for (p = Sp.CircuitList(); p; i++, p = p->next()) {
        if (i == line)
            break;
    }
    if (p)
        Sp.SetCircuit(p->name());
         
}


void
QTcircuitListDlg::mouse_motion_slot(QMouseEvent*)
{
}


void
QTcircuitListDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (FC.getFont(&fnt, fnum))
            wb_textarea->setFont(*fnt);
    }
}


void
QTcircuitListDlg::button_slot(bool state)
{
    QPushButton *btn = dynamic_cast<QPushButton*>(sender());
    if (!btn)
        return;
    QString btxt = btn->text();

    for (int n = 0; cl_btns[n]; n++) {
        if (btxt == cl_btns[n]) {
            if (Sp.CurCircuit()) {
                if (!state) {
                    if (cl_affirm)
                        cl_affirm->popdown();
                    return;
                }
                cl_affirm = PopUpAffirm(this, GRloc(),
                    "Delete Current Circuit?", 0, 0);
                if (cl_affirm) {
                    cl_affirm->register_usrptr((void**)&cl_affirm);
                    cl_affirm->register_caller(btn, false, false);
                    QTaffirmDlg *af = dynamic_cast<QTaffirmDlg*>(cl_affirm);
                    connect(af, SIGNAL(affirm(bool, void*)),
                        this, SLOT(delete_ckt_slot(bool, void*)));
                    return;
                }
            }
            else
                GRpkg::self()->ErrPrintf(ET_ERROR, "no current circuit.\n");
            QTdev::SetStatus(btn, false);
        }
    }
}


void
QTcircuitListDlg::dismiss_btn_slot()
{
    delete instPtr;
}


void
QTcircuitListDlg::delete_ckt_slot(bool yn, void*)
{
    if (yn)
        delete Sp.CurCircuit();
}

