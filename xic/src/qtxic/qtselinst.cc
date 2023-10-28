
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

#include "qtselinst.h"
#include "select.h"
#include "cd_celldb.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "qtmenu.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qttextw.h"

#include <QLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>


//--------------------------------------------------------------------------
//
// Pop up to allow the user to select/deselect cell instances.
//

namespace {
    void start_modal(QDialog *w)
    {
        QTmenu::self()->SetSensGlobal(false);
        QTmenu::self()->SetModal(w);
        QTpkg::self()->SetOverrideBusy(true);
        DSPmainDraw(ShowGhost(ERASE))
    }


    void end_modal()
    {
        QTmenu::self()->SetModal(0);
        QTmenu::self()->SetSensGlobal(true);
        QTpkg::self()->SetOverrideBusy(false);
        DSPmainDraw(ShowGhost(DISPLAY))
    }
}


// Modal pop-up allows selection/deselection of the instances in list.
//
void
cMain::PopUpSelectInstances(CDol *list)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (RunMode() != ModeNormal)
        return;
    if (QTcellInstSelectDlg::self()) {
        QTcellInstSelectDlg::self()->update(list);
        return;
    }
    if (!list)
        return;

    new QTcellInstSelectDlg(list);

    QTcellInstSelectDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTcellInstSelectDlg::self(),
        QTmainwin::self()->Viewport());
    QTcellInstSelectDlg::self()->show();

    start_modal(QTcellInstSelectDlg::self());
    QTdev::self()->MainLoop();  // wait for user's response
}


// Modal pop-up returns a new list of instances from those passed,
// user can control whether to keep or ignore.
//
// The argument is consumed, do not use after calling this function.
//
CDol *
cMain::PopUpFilterInstances(CDol *list)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return (list);
    if (RunMode() != ModeNormal)
        return (list);
    if (QTcellInstSelectDlg::self()) {
        QTcellInstSelectDlg::self()->update(list);
        return (list);
    }
    if (!list)
        return (list);

    new QTcellInstSelectDlg(list, true);
    CDol::destroy(list);

    QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTcellInstSelectDlg::self(),
        QTmainwin::self()->Viewport());
    QTcellInstSelectDlg::self()->show();

    start_modal(QTcellInstSelectDlg::self());
    QTdev::self()->MainLoop();  // wait for user's response
    return (QTcellInstSelectDlg::instances());
}
// End of cMain functions.


CDol *QTcellInstSelectDlg::ci_return;
QTcellInstSelectDlg *QTcellInstSelectDlg::instPtr;

QTcellInstSelectDlg::QTcellInstSelectDlg(CDol *l, bool filtmode)
{
    instPtr = this;
    ci_list = 0;
    ci_label = 0;
    ci_field = 0;
    ci_filt = filtmode;
    if (filtmode)
        ci_return = 0;

    setWindowTitle(tr(ci_filt ? "Choose Instances" : "Select Instances"));
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

    QPushButton *btn = new QPushButton(tr(
        ci_filt ? "Choose All" : "Select All"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(sel_btn_slot()));

    btn = new QPushButton(tr(
        ci_filt ? "Ignore All" : "Desel All"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(desel_btn_slot()));

    QGroupBox *gb = new QGroupBox();
    vbox->addWidget(gb);
    hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qmtop);

    ci_label = new QLabel("");
    hbox->addWidget(ci_label);

    wb_textarea = new QTtextEdit;
    vbox->addWidget(wb_textarea);
    wb_textarea->setReadOnly(true);
    wb_textarea->setMouseTracking(true);
    connect(wb_textarea, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));

    btn = new QPushButton(tr(
        ci_filt ? "Continue" : "Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    // Use a fixed font in the label, same as the text area, so can
    // match columns.
    QFont *fnt;
    if (FC.getFont(&fnt, FNT_FIXED)) {
        wb_textarea->setFont(*fnt);
        ci_label->setFont(*fnt);
    }
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    update(l);
}


QTcellInstSelectDlg::~QTcellInstSelectDlg()
{
    instPtr = 0;
    if (ci_filt) {
        CDol *c0 = 0, *cn = 0;
        for (ci_item *s = ci_list; s->name; s++) {
            if (s->sel) {
                if (!c0)
                    c0 = cn = new CDol(s->cdesc, 0);
                else {
                    cn->next = new CDol(s->cdesc, 0);
                    cn = cn->next;
                }
            }
        }
        ci_return = c0;
    }
    delete [] ci_list;
    if (QTdev::self()->LoopLevel() > 1)
        QTdev::self()->BreakLoop();
    end_modal();
}


QSize
QTcellInstSelectDlg::sizeHint() const
{
    int ww = (ci_field + 6)*QTfont::stringWidth(0, wb_textarea);
    if (ww < 200)
        ww = 200;
    else if (ww > 600)
        ww = 600;
    ww += 15;  // scrollbar
    int hh = 8*QTfont::lineHeight(wb_textarea);
    return (QSize(ww, hh));
}


void
QTcellInstSelectDlg::update(CDol *ol)
{
    if (ol) {
        int sz = 0;
        for (CDol *o = ol; o; o = o->next)
            sz++;
        ci_list = new ci_item[sz+1];
        ci_field = 0;
        ci_item *itm = ci_list;
        for (CDol *o = ol; o; o = o->next) {
            if (o->odesc->type() != CDINSTANCE)
                continue;
            CDc *cd = (CDc*)o->odesc;
            CDs *prnt = cd->parent();
            if (!prnt)
                continue;
            if (!prnt->isInstNumValid())
                prnt->numberInstances();

            itm->cdesc = cd;
            itm->name = Tstring(cd->cellname());
            itm->index = cd->index();
            itm->sel = false;
            int w = strlen(itm->name);
            if (w > ci_field)
                ci_field = w;
            itm++;
        }
        ci_field += 5;

        char lab[256];
        strcpy(lab, "Cell Instances        Click on yes/no\n");
        char *t = lab + strlen(lab);
        for (int i = 0; i <= ci_field; i++)
            *t++ = ' ';
        strcpy(t, ci_filt ? "Choose? " : "Select?");

        ci_label->setText(lab);
    }
    refresh();
}


// Redraw the text area.
//
void
QTcellInstSelectDlg::refresh()
{
    if (!ci_list)
        return;

    char buf[256];
    QColor nc = QTbag::PopupColor(GRattrColorNo);
    QColor yc = QTbag::PopupColor(GRattrColorYes);

    double val = wb_textarea->get_scroll_value();
    wb_textarea->clear();

    for (ci_item *s = ci_list; s->name; s++) {
        snprintf(buf, sizeof(buf), "%s%c%d", s->name, CD_INST_NAME_SEP,
            s->index);
        int len = strlen(buf);
        char *e = buf + len;
        while (len <= ci_field) {
            *e++ = ' ';
            len++;
        }
        *e = 0;
        wb_textarea->setTextColor(QColor("black"));
        wb_textarea->insertPlainText(buf);

        if (!ci_filt)
            s->sel = (s->cdesc->state() == CDobjSelected);
        snprintf(buf, sizeof(buf), "%-3s\n", s->sel ? "yes" : "no");
        wb_textarea->setTextColor(s->sel ? yc : nc);
        wb_textarea->insertPlainText(buf);
    }
    wb_textarea->set_scroll_value(val);
}


// Static function
void
QTcellInstSelectDlg::apply_selection(ci_item *s)
{
    CDs *sd = CurCell();
    if (s->cdesc->state() == CDobjVanilla && s->sel) {
        if (XM()->IsBoundaryVisible(sd, s->cdesc)) {
            Selections.insertObject(sd, s->cdesc);
        }
    }
    else if (s->cdesc->state() == CDobjSelected && !s->sel) {
        if (XM()->IsBoundaryVisible(sd, s->cdesc)) {
            Selections.removeObject(sd, s->cdesc);
        }
    }
}


void
QTcellInstSelectDlg::sel_btn_slot()
{
    for (ci_item *s = ci_list; s->name; s++) {
        s->sel = true;
        if (!ci_filt)
            apply_selection(s);
    }
    refresh();
}


void
QTcellInstSelectDlg::desel_btn_slot()
{
    for (ci_item *s = ci_list; s->name; s++) {
        s->sel = false;
        if (!ci_filt)
            apply_selection(s);
    }
    refresh();
}


void
QTcellInstSelectDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    char *str = wb_textarea->get_chars();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    QTextCursor cur = wb_textarea->cursorForPosition(QPoint(xx, yy));
    int posn = cur.position();

    if (isspace(str[posn])) {
        // Clicked on white space.
        delete [] str;
        return;
    }

    const char *lineptr = str;
    for (int i = 0; i <= posn; i++) {
        if (str[i] == '\n') {
            if (i == posn) {
                // Clicked to right of line.
                delete [] str;
                return;
            }
            lineptr = str + i+1;
        }
    }

    const char *start = str + posn;
    const char *end = start;
    while (start > lineptr && !isspace(*start))
        start--;
    if (isspace(*start))
        start++;
    if (start == lineptr) {
        // Pointing at first word (cell name).
        delete [] str;
        return;
    }
    while (*end && !isspace(*end))
        end++;

    if (end - start > 3) {
        delete [] str;
        return;
    }

    wb_textarea->setTextCursor(cur);
    wb_textarea->moveCursor(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    wb_textarea->moveCursor(QTextCursor::EndOfWord,QTextCursor::KeepAnchor);
    if (!strncmp(start, "yes", 3)) {
        wb_textarea->setTextColor(QTbag::PopupColor(GRattrColorNo));
        wb_textarea->textCursor().insertText("no ");
    }
    else if (!strncmp(start, "no ", 3)) {
        wb_textarea->moveCursor(QTextCursor::Right,QTextCursor::KeepAnchor);
        wb_textarea->setTextColor(QTbag::PopupColor(GRattrColorYes));
        wb_textarea->textCursor().insertText("yes");
    }

    char *cname = lstring::gettok(&lineptr);
    if (cname) {
        char *e = strrchr(cname, CD_INST_NAME_SEP);
        if (e) {
            *e++ = 0;
            int ix = atoi(e);
            for (ci_item *s = ci_list; s->name; s++) {
                if (!strcmp(s->name, cname) && ix == s->index) {
                    s->sel = (*start == 'n');
                    if (!ci_filt)
                        apply_selection(s);
                    break;
                }
            }
        }
        delete cname;
    }
    delete [] str;
}


void
QTcellInstSelectDlg::dismiss_btn_slot()
{
    deleteLater();
}


void
QTcellInstSelectDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_FIXED)) {
            wb_textarea->setFont(*fnt);
            ci_label->setFont(*fnt);
        }
        refresh();
    }
}

