
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

#include "qtcflags.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "cd_celldb.h"
#include "editif.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qttextw.h"

#include <QLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>


//-----------------------------------------------------------------------------
// QTcflagsDlg:  Dialog to set cell flags (immutable, library).
// Called from the Cells Listing dialog (QTcellsDlg).

// Pop up a list of cell names, and enable changing the Immutable and
// Library flags.  The dmode specifies the display mode:  Physical or
// Electrical.
//
void
cMain::PopUpCellFlags(GRobject caller, ShowMode mode, const stringlist *list,
    int dmode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTcflagsDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTcflagsDlg::self())
            QTcflagsDlg::self()->update(list, dmode);
        return;
    }
    if (QTcflagsDlg::self()) {
        QTcflagsDlg::self()->update(list, dmode);
        return;
    }

    new QTcflagsDlg(caller, list, dmode);

    QTcflagsDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTcflagsDlg::self(),
        QTmainwin::self()->Viewport());
    QTcflagsDlg::self()->show();
}
// End of cMain functions.


QTcflagsDlg *QTcflagsDlg::instPtr;

QTcflagsDlg::QTcflagsDlg(GRobject caller, const stringlist *sl, int dmode)
    : QTbag(this)
{
    instPtr = this;
    cf_caller = caller;
    cf_label = 0;
    cf_list = 0;
    cf_field = 0;
    cf_dmode = dmode;

    setWindowTitle(tr("Set Cell Flags"));
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

    QLabel *label = new QLabel(tr("IMMUTABLE"));
    label->setAlignment(Qt::AlignCenter);
    hbox->addWidget(label);
    label = new QLabel(tr("LIBRARY"));
    label->setAlignment(Qt::AlignCenter);
    hbox->addWidget(label);

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QPushButton *btn = new QPushButton(tr("None"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(imm_none_btn_slot()));

    btn = new QPushButton(tr("All"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(imm_all_btn_slot()));

    btn = new QPushButton(tr("None"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(lib_none_btn_slot()));

    btn = new QPushButton(tr("All"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(lib_all_btn_slot()));

    QGroupBox *gb = new QGroupBox();
    vbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    cf_label = new QLabel("");
    hb->addWidget(cf_label);

    wb_textarea = new QTtextEdit();
    vbox->addWidget(wb_textarea);
    wb_textarea->setReadOnly(true);
    wb_textarea->setMouseTracking(true);
    connect(wb_textarea, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));

    // Use a fixed font in the label, same as the text area, so can
    // match columns.
    QFont *fnt;
    if (FC.getFont(&fnt, FNT_FIXED)) {
        wb_textarea->setFont(*fnt);
        cf_label->setFont(*fnt);
    }
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    btn = new QPushButton(tr("Apply"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(apply_btn_slot()));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update(sl, dmode);
}


QTcflagsDlg::~QTcflagsDlg()
{
    instPtr = 0;
    cf_elt::destroy(cf_list);
    if (cf_caller)
        QTdev::Deselect(cf_caller);
}


void
QTcflagsDlg::update(const stringlist *sl, int dmode)
{
    if (!sl) {
        refresh(true);
        return;
    }

    cf_dmode = dmode;
    cf_elt::destroy(cf_list);
    cf_list = 0;
    cf_elt *ce = 0;

    while (sl) {
        if (cf_dmode == Physical) {
            CDs *sd = CDcdb()->findCell(sl->string, Physical);
            if (sd) {
                if (!cf_list)
                    cf_list = ce = new cf_elt(sd->cellname(),
                        sd->isImmutable(), sd->isLibrary());
                else {
                    ce->next = new cf_elt(sd->cellname(),
                        sd->isImmutable(), sd->isLibrary());
                    ce = ce->next;
                }
            }
        }
        else {
            CDs *sd = CDcdb()->findCell(sl->string, Electrical);
            if (sd) {
                if (!cf_list)
                    cf_list = ce = new cf_elt(sd->cellname(),
                        sd->isImmutable(), sd->isLibrary());
                else {
                    ce->next = new cf_elt(sd->cellname(),
                        sd->isImmutable(), sd->isLibrary());
                    ce = ce->next;
                }
            }
        }
        int w = strlen(sl->string);
        if (w > cf_field)
            cf_field = w;
        sl = sl->next;
    }

    char lab[256];
    strcpy(lab, "Cells in memory - set flags (click on yes/no)\n");
    char *t = lab + strlen(lab);
    for (int i = 0; i <= cf_field + 1; i++)
        *t++ = ' ';
    strcpy(t, "Immut Libr");
    cf_label->setText(lab);
    refresh();
}


// Set a flag for all cells in list.
//
void
QTcflagsDlg::set(int im, int lb)
{
    for (cf_elt *cf = cf_list; cf; cf = cf->next) {
        if (im >= 0)
            cf->immutable = im;
        if (lb >= 0)
            cf->library = lb;
    }
}


// Redraw the text area.
//
void
QTcflagsDlg::refresh(bool check_cc)
{
    CDs *cursdesc = check_cc && DSP()->CurMode() == cf_dmode ?
        CurCell(DSP()->CurMode()) : 0;
    char buf[256];
    QColor nc = QTbag::PopupColor(GRattrColorNo);
    QColor yc = QTbag::PopupColor(GRattrColorYes);
    double val = wb_textarea->get_scroll_value();
    wb_textarea->set_chars("");
    for (cf_elt *cf = cf_list; cf; cf = cf->next) {
        if (cursdesc && cf->name == cursdesc->cellname()) {
            cf->immutable = cursdesc->isImmutable();
            cf->library = cursdesc->isLibrary();
        }
        snprintf(buf, sizeof(buf), "%-*s  ", cf_field, Tstring(cf->name));
        wb_textarea->setTextColor(QColor("black"));
        wb_textarea->insertPlainText(buf);
        const char *yn = cf->immutable ? "yes" : "no";
        snprintf(buf, sizeof(buf), "%-3s  ", yn);
        wb_textarea->setTextColor(*yn == 'y' ? yc : nc);
        wb_textarea->insertPlainText(buf);
        yn = cf->library ? "yes" : "no";
        snprintf(buf, sizeof(buf), "%-3s\n", yn);
        wb_textarea->setTextColor(*yn == 'y' ? yc : nc);
        wb_textarea->insertPlainText(buf);
    }
    wb_textarea->set_scroll_value(val);
}


void
QTcflagsDlg::imm_none_btn_slot()
{
    set(0, -1);
    refresh();
}


void
QTcflagsDlg::imm_all_btn_slot()
{
    set(1, -1);
    refresh();
}


void
QTcflagsDlg::lib_none_btn_slot()
{
    set(-1, 0);
    refresh();
}


void
QTcflagsDlg::lib_all_btn_slot()
{
    set(-1, 1);
    refresh();
}


void
QTcflagsDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    char *str = wb_textarea->get_chars(0, -1);
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
    delete [] str;
}


void
QTcflagsDlg::apply_btn_slot()
{
    CDs *cursd = CurCell(DSP()->CurMode());
    bool cc_changed = false;
    for (cf_elt *cf = cf_list; cf; cf = cf->next) {
        if (cf_dmode == Physical) {
            CDs *sd = CDcdb()->findCell(cf->name, Physical);
            if (sd) {
                sd->setImmutable(cf->immutable);
                sd->setLibrary(cf->library);
                if (sd == cursd)
                    cc_changed = true;
            }
        }
        else {
            CDs *sd = CDcdb()->findCell(cf->name, Electrical);
            if (sd) {
                sd->setImmutable(cf->immutable);
                sd->setLibrary(cf->library);
                if (sd == cursd)
                    cc_changed = true;
            }
        }
    }
    XM()->PopUpCellFlags(0, MODE_OFF, 0, 0);
    XM()->ShowParameters(0);
    if (cc_changed)
        EditIf()->setEditingMode(!cursd || !cursd->isImmutable());
}


void
QTcflagsDlg::dismiss_btn_slot()
{
    XM()->PopUpCellFlags(0, MODE_OFF, 0, 0);
}


void
QTcflagsDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_FIXED)) {
            wb_textarea->setFont(*fnt);
            cf_label->setFont(*fnt);
        }
        refresh();
    }
}

