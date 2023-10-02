
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
#include "qtrunops.h"
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
// Dialog to display the current "runops" (traces, iplots, etc.).
// Buttons:
//  cancel: pop down
//  help:   bring up help panel
//  delete: delete entry

// Keywords referenced in help database:
//  tracepanel

void
QTtoolbar::PopUpRunops(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        if (QTrunopListDlg::self())
            QTrunopListDlg::self()->deleteLater();
        return;
    }
    if (QTrunopListDlg::self())
        return;

    sLstr lstr;
    OP.statusCmd(&lstr);
    new QTrunopListDlg(x, y, lstr.string());
    QTrunopListDlg::self()->show();
}


// Update the runop list.  Called when runops are added or deleted.
//
void
QTtoolbar::UpdateRunops()
{
    if (tb_suppress_update)
        return;
    if (!QTrunopListDlg::self())
        return;

    sLstr lstr;
    OP.statusCmd(&lstr);
    QTrunopListDlg::self()->update(lstr.string());
}
// End of QTtoolbar functions.


const char *QTrunopListDlg::tl_btns[] = { "Delete Inactive", 0 };
QTrunopListDlg *QTrunopListDlg::instPtr;

QTrunopListDlg::QTrunopListDlg(int x, int y, const char *s)
{
    instPtr = this;
    tl_affirm = 0;

    setWindowTitle(tr("Runops"));
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

    QLabel *label = new QLabel(tr("Active Runops (traces, iplots, debugs)"));
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

    for (int n = 0; tl_btns[n]; n++) {
        btn = new QPushButton(tr(tl_btns[n]));
        btn->setCheckable(true);
        hbox->addWidget(btn);
        connect(btn, SIGNAL(toggled(bool)),
            this, SLOT(button_slot(bool)));
    }

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    TB()->FixLoc(&x, &y);
    TB()->SetActiveDlg(tid_runops, this);
    move(x, y);
}


QTrunopListDlg::~QTrunopListDlg()
{
    instPtr = 0;
    TB()->SetLoc(tid_runops, this);
    TB()->SetActiveDlg(tid_runops, 0);
    QTtoolbar::entries(tid_runops)->action()->setChecked(false);
}


QSize
QTrunopListDlg::sizeHint() const
{
    // 80X16 text area.
    int wid = 80*QTfont::stringWidth("X", FNT_FIXED) + 4;
    int hei = height() - wb_textarea->height() +
        16*QTfont::lineHeight(FNT_FIXED);
    return (QSize(wid, hei));
}


void
QTrunopListDlg::help_btn_slot()
{
    HLP()->word("plotspanel");
}


void
QTrunopListDlg::update(const char *s)
{
    int val = wb_textarea->get_scroll_value();
    wb_textarea->set_chars(s);
    recolor();
    wb_textarea->set_scroll_value(val);
}


void
QTrunopListDlg::recolor()
{
    QByteArray ba = wb_textarea->toPlainText().toLatin1();
    const char *string = ba.constData();
    if (!string)
        return;
    QColor red("red");
    bool wasret = true;
    int n = 0;

    wb_textarea->set_editable(true);
    for (const char *s = string; *s; s++) {
        if (wasret) {
            wasret = false;
            if (*s == 'I')
                wb_textarea->replace_chars(&red, "I", n, n+1);
        }
        if (*s == '\n')
            wasret = true;
        n++;
    }
    wb_textarea->set_editable(false);
}


void
QTrunopListDlg::mouse_press_slot(QMouseEvent *ev)
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
    int x = ev->x();
    int y = ev->y();
    QTextCursor cur = wb_textarea->cursorForPosition(QPoint(x, y));
    int pos = cur.position();

    const char *lineptr = str;
    for (int i = 0; i <= pos; i++) {
        if (str[i] == '\n') {
            if (i == pos) {
                // Clicked to  right of line.
                break;
            }
            lineptr = str + i+1;
        }
    }

    // The first column should start with ' ' for active debugs, 'I'
    // for inactive ones.  Anything else is a 'no debugs in effect'
    // message.
    //
    if (*lineptr != ' ' && *lineptr != 'I')
        return;

    // Is it active? Set to opposite.
    bool active = (*lineptr == ' ') ? false : true;

    int dnum;
    if (sscanf(lineptr+2, "%d", &dnum) != 1)
        return;
    OP.setRunopActive(dnum, active);

    QColor red("red");
    int posn = lineptr - str;
    wb_textarea->set_editable(true);
    wb_textarea->replace_chars(&red, active ? " " : "I", posn, posn+1);
    wb_textarea->set_editable(false);
}


void
QTrunopListDlg::mouse_motion_slot(QMouseEvent*)
{
}


void
QTrunopListDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (FC.getFont(&fnt, fnum))
            wb_textarea->setFont(*fnt);
    }
}


void
QTrunopListDlg::button_slot(bool state)
{
    QPushButton *btn = dynamic_cast<QPushButton*>(sender());
    if (!btn)
        return;
    QString btxt = btn->text();

    for (int n = 0; tl_btns[n]; n++) {
        if (btxt == tl_btns[n]) {
            if (n == 0) {
                // 'Delete Inactive' button pressed, ask for confirmation.
                if (!state) {
                    if (tl_affirm)
                        tl_affirm->popdown();
                    return;
                }
                QByteArray ba = wb_textarea->toPlainText().toLatin1();
                const char *s = ba.constData();
                if (*s != 'I') {
                    const char *t;
                    for (t = s; *t; t++)
                        if (*t == '\n') {
                            if (*(t+1) == 'I')
                                break;
                    }
                    if (!*t) {
                        GRpkg::self()->ErrPrintf(ET_ERROR,
                            "no inactive debugs.\n");
                        QTdev::SetStatus(btn, false);
                        return;
                    }
                }
                tl_affirm = PopUpAffirm(this, GRloc(),
                    "Delete Inactive Runops?", 0, 0);
                if (tl_affirm) {
                    tl_affirm->register_usrptr((void**)&tl_affirm);
                    tl_affirm->register_caller(btn, false, false);
                    QTaffirmDlg *af = dynamic_cast<QTaffirmDlg*>(tl_affirm);
                    connect(af, SIGNAL(affirm(bool, void*)),
                        this, SLOT(delete_runop_slot(bool, void*)));
                }
            }
        }
    }

}


void
QTrunopListDlg::dismiss_btn_slot()
{
    delete instPtr;
}


void
QTrunopListDlg::delete_runop_slot(bool yn, void*)
{
    if (yn) {
        OP.deleteRunop(DF_ALL, true, -1);
        TB()->UpdateRunops();
    }
}

