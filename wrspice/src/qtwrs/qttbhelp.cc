
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
#include "qttbhelp.h"
#include "qtinterf/qttextw.h"
#include "qtinterf/qtfont.h"
#include "simulator.h"
#include "cshell.h"
#include "keywords.h"
#include "qttoolb.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#include "qtmozy/qthelp.h"
#endif

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>


// Dialog to display keyword help lists.  Clicking on the list entries
// calls the main help system.  This is called from the dialogs which
// contain lists of 'set' variables to modify.

void
QTtoolbar::PopUpTBhelp(ShowMode mode, GRobject parent, GRobject call_btn,
    TBH_type type)
{
    if (mode == MODE_OFF) {
        if (!tb_kw_help[type])
            return;
        delete tb_kw_help[type];
        return;
    }
    if (tb_kw_help[type])
        return;
    QTtbHelpDlg *th = new QTtbHelpDlg(parent, call_btn, type);
    tb_kw_help[type] = th;
    if (tb_kw_help_pos[type].x != 0 && tb_kw_help_pos[type].y != 0)
        th->move(tb_kw_help_pos[type].x, tb_kw_help_pos[type].y);
    th->show();
}


char *
QTtoolbar::KeywordsText(GRobject parent)
{
    sLstr lstr;
    if (parent == QTtoolbar::entries(tid_shell)->dialog()) {
        for (int i = 0; KW.shell(i)->word; i++)
            KW.shell(i)->print(&lstr);
    }
    else if (parent == QTtoolbar::entries(tid_simdefs)->dialog()) {
        for (int i = 0; KW.sim(i)->word; i++)
            KW.sim(i)->print(&lstr);
    }
    else if (parent == QTtoolbar::entries(tid_commands)->dialog()) {
        for (int i = 0; KW.cmds(i)->word; i++)
            KW.cmds(i)->print(&lstr);
    }
    else if (parent == QTtoolbar::entries(tid_plotdefs)->dialog()) {
        for (int i = 0; KW.plot(i)->word; i++)
            KW.plot(i)->print(&lstr);
    }
    else if (parent == QTtoolbar::entries(tid_debug)->dialog()) {
        for (int i = 0; KW.debug(i)->word; i++)
            KW.debug(i)->print(&lstr);
    }
    else
        lstr.add("Internal error.");
    return (lstr.string_trim());
}


void
QTtoolbar::KeywordsCleanup(QTtbHelpDlg *dlg)
{
    if (!dlg)
        return;
    TBH_type t = dlg->type();
    tb_kw_help[t] = 0;
    QPoint pt = dlg->mapToGlobal(QPoint(0, 0));
    tb_kw_help_pos[t].x = pt.x();
    tb_kw_help_pos[t].y = pt.y();
}
// End of QTtoolbar functions;


QTtbHelpDlg::QTtbHelpDlg(GRobject prnt, GRobject call_btn, TBH_type typ)
{
    th_text = 0;
    th_parent = prnt;
    th_caller = call_btn;
    th_type = typ;
    th_lx = 0;
    th_ly = 0;

    setWindowTitle(tr("Keyword Help"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // label in frame
    //
    QGroupBox *gb = new QGroupBox();
    vbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);

    QLabel *label = new QLabel(tr("Click on entries for more help: "));
    hb->addWidget(label);

    // text area
    //
    th_text = new QTtextEdit();
    vbox->addWidget(th_text);
    th_text->setReadOnly(true);
    th_text->setMouseTracking(true);
    th_text->setLineWrapMode(QTextEdit::NoWrap);
    connect(th_text, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));
    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_FIXED))
        th_text->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    char *s = TB()->KeywordsText(th_parent);
    th_text->set_chars(s);
    delete [] s;

    // buttons
    //
    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));
}


QTtbHelpDlg::~QTtbHelpDlg()
{
    if (th_caller)
        QTdev::Deselect(th_caller);
    TB()->KeywordsCleanup(this);
}


QSize
QTtbHelpDlg::sizeHint() const
{
    int wid = 80*QTfont::stringWidth(0, th_text);
    int hei = 12*QTfont::lineHeight(th_text);
    return (QSize(wid+8, hei+20));
}


void
QTtbHelpDlg::dismiss_btn_slot()
{
    delete this;
}


void
QTtbHelpDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    int vsv = th_text->verticalScrollBar()->value();
    int hsv = th_text->horizontalScrollBar()->value();

    const char *str = th_text->get_chars();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    int posn = th_text->document()->documentLayout()->hitTest(
        QPointF(xx + hsv, yy + vsv), Qt::ExactHit);

    const char *lineptr = str;
    for (int i = 0; i <= posn; i++) {
        if (str[i] == '\n') {
            if (i == posn) {
                // Clicked to right of line.
                th_text->select_range(0, 0);
                delete [] str;
                return;
            }
            lineptr = str + i+1;
        }
    }

    // find first word
    while (isspace(*lineptr) && *lineptr != '\n')
        lineptr++;
    if (*lineptr == 0 || *lineptr == '\n') {
        th_text->select_range(0, 0);
        delete [] str;
        return;
    }

    int start = lineptr - str;
    char buf[64];
    char *s = buf;
    while (!isspace(*lineptr))
        *s++ = *lineptr++;
    *s = 0;
    int end = lineptr - str;

    th_text->select_range(start, end);
    // Don't let the scroll position change.
    th_text->verticalScrollBar()->setValue(vsv);
    th_text->horizontalScrollBar()->setValue(hsv);
#ifdef HAVE_MOZY
    HLP()->word(buf);
#endif
    delete [] str;
}


void
QTtbHelpDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, fnum)) {
            th_text->setFont(*fnt);
        }
    }
}

