
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

#include "qtmodif.h"
#include "qtmenu.h"
#include "edit.h"
#include "cvrt_variables.h"
#include "fio.h"
#include "cd_celldb.h"
#include "dsp_color.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qttextw.h"

#include <QWidget>
#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMouseEvent>
#include <QScrollBar>


//--------------------------------------------------------------------------
// Pop up to question user about saving modified cells
//
// Help system keywords used:
//  xic:sv

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


// Pop-up to list modified cells, allowing the user to choose which to
// save.  This is a modal pop-up.  The return value is true unless
// saveproc returns false for some file.
//
PMretType
cEdit::PopUpModified(stringlist *list, bool(*saveproc)(const char*))
{
    if (!QTdev::self() || !QTmainwin::self())
        return (PMok);
    if (QTmodifDlg::self())
        return (PMok);
    if (!list)
        return (PMok);

    new QTmodifDlg(list, saveproc);
    if (QTmodifDlg::self()->is_empty()) {
        QTmodifDlg::self()->deleteLater();
        return (PMok);
    }
    if (!QTmodifDlg::self()->Shell()) {
        QTmodifDlg::self()->deleteLater();
        return (PMerr);
    }
    QTmodifDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTmodifDlg::self(),
        QTmainwin::self()->Viewport());

    QTmodifDlg::self()-> show();
    start_modal(QTmodifDlg::self());
    QTdev::self()->MainLoop();  // wait for user's response
    end_modal();

    return (QTmodifDlg::retval());
}
// End of cEdit functions.


namespace {
    const char *file_type_str(FileType ft)
    {
        if (ft == Fnative || ft == Fnone)
            return ("X");
        if (ft == Fgds)
            return ("G");
        if (ft == Fcgx)
            return ("B");
        if (ft == Foas)
            return ("O");
        if (ft == Fcif)
            return ("C");
        if (ft == Foa)
            return ("A");
        return ("");
    }
}


PMretType QTmodifDlg::m_retval;
QTmodifDlg *QTmodifDlg::instPtr;

QTmodifDlg::QTmodifDlg(stringlist *l, bool(*s)(const char*))
{
    instPtr = this;
    wb_shell = this;
    stringlist::sort(l);
    int sz = stringlist::length(l);
    m_list = new s_item[sz+1];
    m_field = 0;
    m_width = 0;
    s_item *itm = m_list;
    bool ptoo = FIO()->IsListPCellSubMasters();
    bool vtoo = FIO()->IsListViaSubMasters();
    while (l) {
        CDcbin cbin;
        if (CDcdb()->findSymbol(l->string, &cbin)) {
            if (!ptoo && cbin.phys() && cbin.phys()->isPCellSubMaster()) {
                l = l->next;
                continue;
            }
            if (!vtoo && cbin.phys() && cbin.phys()->isViaSubMaster()) {
                l = l->next;
                continue;
            }

            itm->name = lstring::copy(l->string);
            if (cbin.fileType() == Fnative && cbin.fileName()) {
                int len = strlen(cbin.fileName()) +
                    strlen(Tstring(cbin.cellname())) + 2;
                itm->path = new char[len];
                snprintf(itm->path, len, "%s/%s", cbin.fileName(),
                    Tstring(cbin.cellname()));
            }
            else if (FIO()->IsSupportedArchiveFormat(cbin.fileType()))
                itm->path = FIO()->DefaultFilename(&cbin, cbin.fileType());
            else
                itm->path = lstring::copy(cbin.fileName());
            if (!itm->path)
                itm->path = lstring::copy("(in current directory)");
            strcpy(itm->ft, file_type_str(cbin.fileType()));
            int w = strlen(itm->name);
            if (w > m_field)
                m_field = w;
            w += strlen(itm->path);
            w += 9;
            if (w > m_width)
                m_width = w;
            if (cbin.phys() && cbin.phys()->isPCellSubMaster()) {
                itm->save = false;
                itm->ft[0] = 'P';
                itm->ft[1] = 0;
            }
            if (cbin.phys() && cbin.phys()->isViaSubMaster()) {
                itm->save = false;
                itm->ft[0] = 'V';
                itm->ft[1] = 0;
            }
            itm++;
        }
        l = l->next;
    }
    m_saveproc = s;
    m_retval = PMok;
    if (!m_field || !m_width) {
        wb_shell = 0;
        return;
    }

    setWindowTitle(tr("Modified Cells"));
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

    QPushButton *btn = new QPushButton(tr("Save All"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(save_all_slot()));

    btn = new QPushButton(tr("Skip All"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(skip_all_slot()));

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);

    QGroupBox *gb = new QGroupBox(this);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    m_label = new QLabel(gb);

    hb->addWidget(m_label);
    hbox->addWidget(gb);

    btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_slot()));
    vbox->addLayout(hbox);

    m_text = new QTtextEdit();
    m_text->setReadOnly(true);
    m_text->setMouseTracking(true);
    vbox->addWidget(m_text);
    connect(m_text, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    btn = new QPushButton(tr("Apply - Continue"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(apply_slot()));
    btn = new QPushButton(tr("ABORT"));
    hbox->addWidget(btn);
    vbox->addLayout(hbox);
    connect(btn, SIGNAL(clicked()), this, SLOT(abort_slot()));

    // Use a fixed font in the label, same as the text area, so can
    // match columns.
    QFont *fnt;
    if (FC.getFont(&fnt, FNT_FIXED)) {
        m_text->setFont(*fnt);
        m_label->setFont(*fnt);
    }

    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    refresh();
}


QTmodifDlg::~QTmodifDlg()
{
    instPtr = 0;
    delete [] m_list;
}


QSize
QTmodifDlg::sizeHint() const
{
    int ww = m_width*QTfont::stringWidth(0, m_text);
    if (ww < 200)
        ww = 200;
    else if (ww > 600)
        ww = 600;
    ww += 15;  // scrollbar
    int hh = 8*QTfont::lineHeight(m_text);
    return (QSize(ww, hh));
}


// Redraw the text area.
//
void
QTmodifDlg::refresh()
{
    char buf[256];
    QColor nc = QTbag::PopupColor(GRattrColorNo);
    QColor yc = QTbag::PopupColor(GRattrColorYes);
    QColor c1 = QTbag::PopupColor(GRattrColorHl2);
    QColor c2 = QTbag::PopupColor(GRattrColorHl3);
    int sval = m_text->get_scroll_value();
    m_text->clear();
    for (s_item *s = m_list; s->name; s++) {
        snprintf(buf, sizeof(buf), "%-*s  ", m_field, s->name);
        m_text->setTextColor(QColor("black"));
        m_text->insertPlainText(tr(buf));

        snprintf(buf, sizeof(buf), "%-3s  ", s->save ? "yes" : "no");
        m_text->setTextColor(s->save ? yc : nc);
        m_text->insertPlainText(tr(buf));

        snprintf(buf, sizeof(buf), "%c ", *s->ft);
        m_text->setTextColor(
            *buf == 'X' || *buf == 'A' || *buf == 'P' ? c1 : c2);
        m_text->insertPlainText(tr(buf));

        snprintf(buf, sizeof(buf), "%s\n", s->path);
        m_text->setTextColor(QColor("black"));
        m_text->insertPlainText(tr(buf));
    }
    m_text->set_scroll_value(sval);

    char lab[256];
    strcpy(lab, "Modified Cells            Click on yes/no \n");
    char *t = lab + strlen(lab);
    for (int i = 0; i <= m_field + 1; i++)
        *t++ = ' ';
    strcpy(t, "Save?");
    m_label->setText(tr(lab));
}


void
QTmodifDlg::save_all_slot()
{
    for (s_item *s = instPtr->m_list; s->name; s++)
        s->save = true;
    refresh();
}


void
QTmodifDlg::skip_all_slot()
{
    for (s_item *s = instPtr->m_list; s->name; s++)
        s->save = false;
    refresh();
}


void
QTmodifDlg::help_slot()
{
    DSPmainWbag(PopUpHelp("xic:sv"))
}


void
QTmodifDlg::apply_slot()
{
    if (m_saveproc) {
        for (s_item *s = m_list; s->name; s++) {
            if (s->save && !(*m_saveproc)(s->name))
                    m_retval = PMerr;
        }
        m_saveproc = 0;
    }
    if (QTdev::self()->LoopLevel() > 1)
        QTdev::self()->BreakLoop();
    instPtr->deleteLater();
}


void
QTmodifDlg::abort_slot()
{
    m_retval = PMabort;
    if (QTdev::self()->LoopLevel() > 1)
        QTdev::self()->BreakLoop();
    instPtr->deleteLater();
}


void
QTmodifDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_FIXED)) {
            m_text->setFont(*fnt);
            m_label->setFont(*fnt);
        }
        refresh();
    }
}


void
QTmodifDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    const char *str = lstring::copy(
        m_text->toPlainText().toLatin1().constData());
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    QTextCursor cur = m_text->cursorForPosition(QPoint(xx, yy));
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

    m_text->setTextCursor(cur);
    m_text->moveCursor(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    m_text->moveCursor(QTextCursor::EndOfWord,QTextCursor::KeepAnchor);
    if (!strncmp(start, "yes", 3)) {
        m_text->setTextColor(QTbag::PopupColor(GRattrColorNo));
        m_text->textCursor().insertText("no ");
    }
    else if (!strncmp(start, "no ", 3)) {
        m_text->moveCursor(QTextCursor::Right,QTextCursor::KeepAnchor);
        m_text->setTextColor(QTbag::PopupColor(GRattrColorYes));
        m_text->textCursor().insertText("yes");
    }
    delete [] str;
}

