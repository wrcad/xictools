
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

QTcircuitListDlg::QTcircuitListDlg(int x, int y, const char *s)
{
    instPtr = this;
    cl_affirm = 0;

    setWindowTitle(tr("Circuits"));
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    // title label and help button
    //
    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setMargin(0);
    hbox->setSpacing(0);

    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setMargin(2);
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
    hbox->setMargin(0);
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

    TB()->FixLoc(&x, &y);
    TB()->SetActive(tid_circuits, true);
    move(x, y);
}


QTcircuitListDlg::~QTcircuitListDlg()
{
    instPtr = 0;
    TB()->SetLoc(tid_circuits, this);
    TB()->SetActive(tid_circuits, false);
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
    /*XXX
    sTextPop *pop = (sTextPop*)arg;
    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button.button == 1) {
            pop->tp_lx = (int)event->button.x;
            pop->tp_ly = (int)event->button.y;
            return (false);
        }
        return (true);
    }
    if (event->type == GDK_BUTTON_RELEASE) {
        if (event->button.button == 1) {
            int x = (int)event->button.x;
            int y = (int)event->button.y;
            if (abs(x - pop->tp_lx) <= 4 && abs(y - pop->tp_ly) <= 4) {
                if (pop->tp_callback)
                    (*pop->tp_callback)(caller, pop->tp_lx, pop->tp_ly);
            }
            return (false);
        }
    }
    */

    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    if (!instPtr)
        return;

    QByteArray ba = wb_textarea->toPlainText().toLatin1();
    const char *str = ba.constData();
    int x = ev->x();
    int y = ev->y();
    QTextCursor cur = wb_textarea->cursorForPosition(QPoint(x, y));
    int pos = cur.position();

    const char *lineptr = str;
    int line = 0;
    for (int i = 0; i <= pos; i++) {
        if (str[i] == '\n') {
            line++;
            if (i == pos) {
                // Clicked to  right of line.
                break;
            }
            lineptr = str + i+1;
        }
    }
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
                        this, SLOT(delete_plot_slot(bool, void*)));
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


#ifdef notdef


// Static function.
// 'Delete' button pressed, ask for confirmation.
//
void
QTcircuitListDlg::ci_actions(GtkWidget *caller, void*)
{
}


// Static function.
// Callback to delete the current circuit.
//
void
QTcircuitListDlg::ci_dfunc()
{
}


// Static function.
// Handle button presses in the text area.
//
void
QTcircuitListDlg::ci_btn_hdlr(GtkWidget *caller, int x, int y)
{
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter iline;
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    y = gtk_text_iter_get_line(&iline);

    sFtCirc *p;
    int i;
    for (i = 0, p = Sp.CircuitList(); p; i++, p = p->next()) {
        if (i == y)
            break;
    }
    if (p)
        Sp.SetCircuit(p->name());
}
// End of QTcircuitListDlg functions
#endif
