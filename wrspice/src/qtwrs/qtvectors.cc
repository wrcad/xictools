
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
#include "qtvectors.h"
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
#include <QToolButton>
#include <QPushButton>
#include <QMouseEvent>
#include <QAction>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>


//===========================================================================
// Pop up to display the list of vectors for the current plot.  Point at
// entries to select them (indicated by '>' in the first column.
// Buttons:
//  cancel: pop down
//  help:   bring up help panel
//  print:  print the selected vectors
//  plot    plot the selected vectors
//  delete: delete the selected vectors

// Keywords referenced in help database:
//  vectorspanel

void
QTtoolbar::PopUpVectors(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        delete QTvectorListDlg::self();
        return;
    }
    if (QTvectorListDlg::self())
        return;

    sLstr lstr;
    OP.vecPrintList(0, &lstr);
    new QTvectorListDlg(x, y, lstr.string());
    QTvectorListDlg::self()->show();
}


// Update the vector list.  Called when a new vector is created or a
// vector is destroyed.  The 'lev' if not 0 will prevent updating
// between successive calls with lev set.  Thus, if several vectors are
// changed, the update can be deferred.
//
void
QTtoolbar::UpdateVectors(int lev)
{
    if (tb_suppress_update)
        return;
    static int level;
    if (!QTvectorListDlg::self())
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

    sLstr lstr;
    OP.vecPrintList(0, &lstr);
    QTvectorListDlg::self()->update(lstr.string());
}
// End of QTtoolbar functions.


const char *QTvectorListDlg::vl_btns[] =
    { "Desel All", "Print", "Plot", "Delete", 0 };
QTvectorListDlg *QTvectorListDlg::instPtr;

QTvectorListDlg::QTvectorListDlg(int xx, int yy, const char *s) : QTbag(this)
{
    instPtr = this;
    vl_x = 0;
    vl_y = 0;
    vl_affirm = 0;

    setWindowTitle(tr("Vectors"));
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

    QLabel *label = new QLabel(tr("Vectors in current plot"));
    hb->addWidget(label);

    for (int n = 0; vl_btns[n]; n++) {
        QToolButton *tbtn = new QToolButton();
        tbtn->setText(tr(vl_btns[n]));
        tbtn->setCheckable(true);
        hbox->addWidget(tbtn);
        connect(tbtn, &QAbstractButton::toggled,
            this, &QTvectorListDlg::button_slot);
    }

    hbox->addStretch(1);
    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("help"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTvectorListDlg::help_btn_slot);

    // scrolled text area
    //
    wb_textarea = new QTtextEdit();
    vbox->addWidget(wb_textarea);
    wb_textarea->setReadOnly(true);
    connect(wb_textarea, &QTtextEdit::press_event,
        this, &QTvectorListDlg::mouse_press_slot);
    connect(wb_textarea, &QTtextEdit::release_event,
        this, &QTvectorListDlg::mouse_release_slot);
    connect(wb_textarea, &QTtextEdit::motion_event,
        this, &QTvectorListDlg::mouse_motion_slot);
    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_FIXED))
        wb_textarea->setFont(*fnt);
    connect(QTfont::self(), &QTfont::fontChanged,
        this, &QTvectorListDlg::font_changed_slot, Qt::QueuedConnection);

    wb_textarea->set_chars(s);
    recolor();

    // dismiss button row
    //
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(0);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QTvectorListDlg::dismiss_btn_slot);

    TB()->FixLoc(&xx, &yy);
    TB()->SetActiveDlg(tid_vectors, this);
    move(xx, yy);
}


QTvectorListDlg::~QTvectorListDlg()
{
    instPtr = 0;
    TB()->SetLoc(tid_vectors, this);
    TB()->SetActiveDlg(tid_vectors, 0);
    QTtoolbar::entries(tid_vectors)->action()->setChecked(false);
}


QSize
QTvectorListDlg::sizeHint() const
{
    // 80X16 text area.
    int wid = 80*QTfont::stringWidth("X", FNT_FIXED) + 4;
    int hei = height() - wb_textarea->height() +
        16*QTfont::lineHeight(FNT_FIXED);
    return (QSize(wid, hei));
}


void
QTvectorListDlg::help_btn_slot()
{
    HLP()->word("vectorspanel");
}


void
QTvectorListDlg::update(const char *s)
{
    int val = wb_textarea->get_scroll_value();
    wb_textarea->set_chars(s);
    recolor();
    wb_textarea->set_scroll_value(val);
}


// Return a wordlist containing the selected vector names.
//
wordlist *
QTvectorListDlg::selections()
{
    char buf[128];
    wordlist *wl=0, *wl0=0;
    QByteArray ba = wb_textarea->toPlainText().toLatin1();
    const char *str = ba.constData();
    const char *t = str;
    int i;
    for (i = 0; *t; t++) {
        if (*t == '\n') {
            i++;
            if (i < 3)
                continue;
            while (*t == '\n')
                t++;
            if (*t == '>') {
                t++;
                while (isspace(*t))
                    t++;
                char *s = buf;
                // Double quote each entry, otherwise vector names
                // that include net expression indexing and similar
                // may not parse correctly.
                *s++ = '"';
                while (*t && !isspace(*t))
                    *s++ = *t++;
                *s++ = '"';
                *s = '\0';
                if (!wl0)
                    wl = wl0 = new wordlist(buf, 0);
                else {
                    wl->wl_next = new wordlist(buf, wl);
                    wl = wl->wl_next;
                }
            }
            if (!*t)
                break;
        }
    }
    if (!wl0)
        GRpkg::self()->ErrPrintf(ET_ERROR, "no vectors are selected.\n");
    return (wl0);
}


void
QTvectorListDlg::recolor()
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
            if (*s == '>')
                wb_textarea->replace_chars(&red, ">", n, n+1);
        }
        if (*s == '\n')
            wasret = true;
        n++;
    }
    wb_textarea->set_editable(false);
}


// Clicking wil select/deselect a line (red caret in leftmost column
// when selected).  Dragging will select text, then initiate drag/drop
// the next time.

void
QTvectorListDlg::mouse_press_slot(QMouseEvent *ev)
{
    ev->ignore();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    vl_x = ev->position().x();
    vl_y = ev->position().y();
#else
    vl_x = ev->x();
    vl_y = ev->y();
#endif
}


void
QTvectorListDlg::mouse_release_slot(QMouseEvent *ev)
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    if (fabs(xx - vl_x) < 5 && fabs(yy - vl_y) < 5) {
        // Clicked, accept to prevent selection or drag/drop.
        ev->accept();
        int vsv = wb_textarea->verticalScrollBar()->value();
        int hsv = wb_textarea->horizontalScrollBar()->value();
        QByteArray ba = wb_textarea->toPlainText().toLatin1();
        const char *str = ba.constData();
        int posn = wb_textarea->document()->documentLayout()->hitTest(
            QPointF(xx + hsv, yy + vsv), Qt::ExactHit);

        const char *lineptr = str;
        int line = 0;
        for (int i = 0; i <= posn; i++) {
            if (str[i] == '\n') {
                line++;
                if (i == posn) {
                    // Clicked to  right of line.
                    break;
                }
                lineptr = str + i+1;
            }
        }
        if (line <= 3)
            return;

        // is it selected? set to opposite
        bool select = (*lineptr == ' ');
        const char *t = lineptr + 1;
        // get vector's name
        while (isspace(*t))
            t++;
        char buf[128];
        char *s = buf;
        while (*t && !isspace(*t))
            *s++ = *t++;
        *s = '\0';

        // grab the dvec from storage
        sDataVec *dv = (sDataVec*)OP.curPlot()->get_perm_vec(buf);
        if (!dv)
            return;
        // The flags save the selected status persistently.
        if (select)
            dv->set_flags(dv->flags() | VF_SELECTED);
        else
            dv->set_flags(dv->flags() & ~VF_SELECTED);

        int pn = lineptr - str;
        QColor red("red");
        wb_textarea->set_editable(true);
        wb_textarea->replace_chars(&red, select ? ">" : " ", pn, pn+1);
        wb_textarea->set_editable(false);
        return;
    }
    ev->ignore();
}


void
QTvectorListDlg::mouse_motion_slot(QMouseEvent *ev)
{
    ev->ignore();
}


void
QTvectorListDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, fnum))
            wb_textarea->setFont(*fnt);
    }
}


void
QTvectorListDlg::button_slot(bool state)
{
    QAbstractButton *btn = dynamic_cast<QAbstractButton*>(sender());
    if (!btn)
        return;
    QString btxt = btn->text();

    for (int n = 0; vl_btns[n]; n++) {
        if (btxt == vl_btns[n]) {
            if (n == 0) {
                // 'Desel All' button pressed, deselect all selections.
                if (!state)
                    return;
                QByteArray ba = wb_textarea->toPlainText().toLatin1();
                const char *string = ba.constData();
                QColor red("red");
                bool wasret = true;

                if (string) {
                    double val = wb_textarea->get_scroll_value();
                    int nc = 0;
                    wb_textarea->set_editable(true);
                    for (const char *s = string; *s; s++) {
                        if (wasret) {
                            wasret = false;
                            if (*s == '>') {
                                wb_textarea->replace_chars(&red, " ", nc,
                                    nc + 1);
                            }
                        }
                        if (*s == '\n')
                            wasret = true;
                        nc++;
                    }
                    wb_textarea->set_editable(false);
                    wb_textarea->set_scroll_value(val);
                    OP.curPlot()->clear_selected();
                }
            }
            else if (n == 1) {
                // 'Print' button pressed, print selected vectors.
                if (!state)
                    return;
                wordlist *wl = selections();
                if (wl) {
                    // Deselect now, if the print is aborted at "more", the
                    // button won't be deselected otherwise.

                    QTdev::Deselect(btn);
                    CommandTab::com_print(wl);
                    wordlist::destroy(wl);
                    CP.Prompt();
                    return;
                }
            }
            else if (n == 2) {
                // 'Plot' button pressed, plot selected vectors.
                if (!state)
                    return;
                wordlist *wl = selections();
                if (wl) {
                    CommandTab::com_plot(wl);
                    wordlist::destroy(wl);
                }
            }
            else if (n == 3) {
                // 'Delete' button pressed, ask for confirmation.
                if (!state) {
                    if (vl_affirm)
                        vl_affirm->popdown();
                    return;
                }
                vl_affirm = PopUpAffirm(this, GRloc(),
                    "Delete Selected Vectors?", 0, 0);
                if (vl_affirm) {
                    vl_affirm->register_usrptr((void**)&vl_affirm);
                    vl_affirm->register_caller(btn, false, false);
                    QTaffirmDlg *af = dynamic_cast<QTaffirmDlg*>(vl_affirm);
                    connect(af, &QTaffirmDlg::affirm,
                        this, &QTvectorListDlg::delete_vecs_slot);
                    return;
                }
            }
            QTdev::Deselect(btn);
        }
    }
}


void
QTvectorListDlg::dismiss_btn_slot()
{
    delete instPtr;
}


void
QTvectorListDlg::delete_vecs_slot(bool yn, void*)
{
    if (yn) {
        wordlist *wl = selections();
        if (wl) {
            CommandTab::com_unlet(wl);
            wordlist::destroy(wl);
        }
    }
}

