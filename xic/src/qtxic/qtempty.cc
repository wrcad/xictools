
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

#include "qtmain.h"
#include "qtempty.h"
#include "cvrt.h"
#include "cd_celldb.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "qtinterf/qtfont.h"

#include <QApplication>
#include <QLayout>
#include <QGroupBox>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QScrollBar>
#include <QMouseEvent>
#include <QAbstractTextDocumentLayout>


//-----------------------------------------------------------------------------
// QTemptyDlg:  Dialog to allow the user to delete empty cells from a
// hierarchy.
// This appears after a new hierarchy is opened.

void
cConvert::PopUpEmpties(stringlist *list)
{
    if (!QTdev::exists() || !QTmainwin::self())
        return;
    if (QTemptyDlg::self())
        return;
    if (XM()->RunMode() != ModeNormal)
        return;
    if (!list)
        return;

    new QTemptyDlg(list);

    QTemptyDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTemptyDlg::self(),
        QTmainwin::self()->Viewport());
    QTemptyDlg::self()->show();
}
// End of cConvert functions.


QTemptyDlg *QTemptyDlg::instPtr;

QTemptyDlg::QTemptyDlg(stringlist *l) : QTbag(this)
{
    instPtr = this;
    ec_list = 0;
    ec_tab = 0;
    ec_label = 0;
    ec_text = 0;
    ec_field = 0;
    ec_changed = false;

    setWindowTitle(tr("Empty Cells"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);
    vbox->setSizeConstraint(QLayout::SetFixedSize);

    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Delete All"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTemptyDlg::delete_btn_slot);

    tbtn = new QToolButton();
    tbtn->setText(tr("Skip All"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTemptyDlg::skip_btn_slot);

    QGroupBox *gb = new QGroupBox();
    vbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    ec_label = new QLabel(gb);
    hb->addWidget(ec_label);

    ec_text = new QTtextEdit();
    ec_text->setReadOnly(true);
    ec_text->setMouseTracking(true);
    vbox->addWidget(ec_text);
    connect(ec_text, &QTtextEdit::press_event,
        this, &QTemptyDlg::mouse_press_slot);

    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    tbtn = new QToolButton();
    tbtn->setText(tr("Apply"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTemptyDlg::apply_btn_slot);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    hbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QTemptyDlg::dismiss_btn_slot);

    // Use a fixed font in the label, same as the text area, so can
    // match columns.
    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_FIXED)) {
        ec_text->setFont(*fnt);
        ec_label->setFont(*fnt);
    }
    connect(QTfont::self(), &QTfont::fontChanged,
        this, &QTemptyDlg::font_changed_slot, Qt::QueuedConnection);
    update(l);
}


QTemptyDlg::~QTemptyDlg()
{
    instPtr = 0;
    if (ec_changed) {
        CDcbin cbin(DSP()->CurCellName());
        cbin.fixBBs();
        XM()->PopUpCells(0, MODE_UPD);
        XM()->PopUpTree(0, MODE_UPD, 0, TU_CUR);
        DSP()->RedisplayAll();
    }
    delete [] ec_list;
    delete ec_tab;
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTemptyDlg
#include "qtinterf/qtmacos_event.h"
#endif


QSize
QTemptyDlg::sizeHint() const
{
    int ww = (ec_field + 6)*QTfont::stringWidth(0, ec_text);
    if (ww < 200)
        ww = 200;
    else if (ww > 600)
        ww = 600;
    ww += 15;  // scrollbar
    int hh = 8*QTfont::lineHeight(ec_text);
    return (QSize(ww, hh));
}


void
QTemptyDlg::update(stringlist *sl)
{
    stringlist *s0 = 0;
    if (!ec_tab)
        ec_tab = new SymTab(false, false);
    if (!sl) {
        if (!DSP()->CurCellName())
            return;
        CDcbin cbin(DSP()->CurCellName());
        sl = cbin.listEmpties();
        stringlist::sort(sl);
        s0 = sl;
    }
    if (!sl) {
        delete this;
        return;
    }
    delete [] ec_list;

    int sz = stringlist::length(sl);
    ec_list = new e_item[sz+1];
    ec_field = 0;
    e_item *itm = ec_list;
    while (sl) {
        // Save only names not seen before.
        if (SymTab::get(ec_tab, sl->string) == ST_NIL) {
            itm->name = lstring::copy(sl->string);
            ec_tab->add(itm->name, 0, false);
            int w = strlen(sl->string);
            if (w > ec_field)
                ec_field = w;
            itm++;
        }
        sl = sl->next;
    }
    stringlist::destroy(s0);

    if (itm == ec_list) {
        // No new items.
        delete this;
        return;
    }

    refresh();
}


// Redraw the text area.
//
void
QTemptyDlg::refresh()
{
    char buf[256];
    QColor nc = QTbag::PopupColor(GRattrColorNo);
    QColor yc = QTbag::PopupColor(GRattrColorYes);
    QScrollBar *vsb = ec_text->verticalScrollBar();
    double val = 0.0;
    if (vsb)
        val = vsb->value();
    ec_text->clear();
    for (e_item *s = ec_list; s->name; s++) {
        snprintf(buf, sizeof(buf), "%-*s  ", ec_field, s->name);
        ec_text->setTextColor(QColor("black"));
        ec_text->insertPlainText(tr(buf));

        snprintf(buf, sizeof(buf), "%-3s\n", s->del ? "yes" : "no");
        ec_text->setTextColor(s->del ? yc : nc);
        ec_text->insertPlainText(tr(buf));
    }
    if (vsb)
        vsb->setValue(val);

    char lab[256];
    strcpy(lab, "Empty Cells            Click on yes/no\n");
    char *t = lab + strlen(lab);
    for (int i = 0; i <= ec_field; i++)
        *t++ = ' ';
    strcpy(t, "Delete?");
    ec_label->setText(lab);
}


void
QTemptyDlg::delete_btn_slot()
{
    for (e_item *s = ec_list; s->name; s++)
        s->del = true;
    refresh();
}


void
QTemptyDlg::skip_btn_slot()
{
    for (e_item *s = ec_list; s->name; s++)
        s->del = false;
    refresh();
}


void
QTemptyDlg::apply_btn_slot()
{
    bool didone = false;
    bool leftone = false;
    for (e_item *s = ec_list; s->name; s++) {
        if (s->del) {
            CDcbin cbin;
            if (CDcdb()->findSymbol(s->name, &cbin)) {
                if (cbin.deleteCells())
                    didone = true;
                continue;
            }
        }
        leftone = true;
    }
    if (didone)
        ec_changed = true;
    if (didone && !leftone)
        update(0);
    else
        delete this;
}


void
QTemptyDlg::dismiss_btn_slot()
{
    delete this;
}


void
QTemptyDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_FIXED)) {
            ec_text->setFont(*fnt);
            ec_label->setFont(*fnt);
        }
        refresh();
    }
}


void
QTemptyDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    int vsv = ec_text->verticalScrollBar()->value();
    int hsv = ec_text->horizontalScrollBar()->value();

    if (!instPtr)
        return;

    const char *str = ec_text->get_chars();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    int posn = ec_text->document()->documentLayout()->hitTest(
        QPointF(xx + hsv, yy + vsv), Qt::ExactHit);
    
    if (isspace(str[posn])) {
        // Clicked on white space.
        delete [] str;
        return;
    }

    const char *lineptr = str;
    for (int i = 0; i <= posn; i++) {
        if (str[i] == '\n') {
            if (i == posn) {
                // Clicked to  right of line.
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

    ec_text->set_insertion_point(posn);
    ec_text->moveCursor(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    ec_text->moveCursor(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    ec_text->moveCursor(QTextCursor::EndOfWord,QTextCursor::KeepAnchor);
    e_item *scur = 0;
    char *cname = lstring::gettok(&lineptr);
    if (cname) {
        for (e_item *s = ec_list; s->name; s++) {
            if (!strcmp(s->name, cname)) {
                scur = s;
                break;
            }
        }
    }
    if (!strncmp(start, "yes", 3)) {
        ec_text->setTextColor(QTbag::PopupColor(GRattrColorNo));
        ec_text->textCursor().insertText("no ");
        if (scur)
            scur->del = false;
    }
    else if (!strncmp(start, "no ", 3)) {
        ec_text->moveCursor(QTextCursor::Right,QTextCursor::KeepAnchor);
        ec_text->setTextColor(QTbag::PopupColor(GRattrColorYes));
        ec_text->textCursor().insertText("yes");
        if (scur)
            scur->del = true;
    }
    delete [] cname;
    delete [] str;
}

