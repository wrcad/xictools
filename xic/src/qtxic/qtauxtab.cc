
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

#include "qtauxtab.h"
#include "cvrt.h"
#include "fio.h"
#include "cd_celldb.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qttextw.h"

#include <QApplication>
#include <QLayout>
#include <QToolButton>
#include <QPushButton>
#include <QRadioButton>
#include <QGroupBox>
#include <QLabel>
#include <QMouseEvent>
#include <QMimeData>
#include <QResizeEvent>
#include <QDrag>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>


//-----------------------------------------------------------------------------
// QTauxTabDlg:  Dialog to manage Override Cell Table.
// Called from the Edit Cell Table pushbutton in the Cell Hierarchy Digests
// list panel (QTchdListDlg).
//
// Help system keywords used:
//  xic:overtab

void
cConvert::PopUpAuxTab(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTauxTabDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTauxTabDlg::self())
            QTauxTabDlg::self()->update();
        return;
    }
    if (QTauxTabDlg::self())
        return;

    new QTauxTabDlg(caller);

    QTauxTabDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTauxTabDlg::self(),
        QTmainwin::self()->Viewport());
    QTauxTabDlg::self()->show();
}
// End of cConvert functions.


QTauxTabDlg *QTauxTabDlg::instPtr;

QTauxTabDlg::QTauxTabDlg(GRobject c) : QTbag(this)
{
    instPtr = this;
    at_caller = c;
    at_addbtn = 0;
    at_rembtn = 0;
    at_clearbtn = 0;
    at_over = 0;
    at_skip = 0;
    at_add_pop = 0;
    at_remove_pop = 0;
    at_clear_pop = 0;

    at_start = 0;
    at_end = 0;
    at_drag_x = at_drag_y = 0;
    at_dragging = false;

    setWindowTitle(tr("Cell Table Listing"));
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

    // Button row
    //
    at_addbtn = new QToolButton();
    at_addbtn->setText(tr("Add"));
    hbox->addWidget(at_addbtn);
    at_addbtn->setCheckable(true);
    connect(at_addbtn, SIGNAL(toggled(bool)),
        this, SLOT(add_btn_slot(bool)));

    at_rembtn = new QToolButton();
    at_rembtn->setText(tr("Remove"));
    hbox->addWidget(at_rembtn);
    at_rembtn->setCheckable(true);
    connect(at_rembtn, SIGNAL(toggled(bool)),
        this, SLOT(rem_btn_slot(bool)));

    at_clearbtn = new QToolButton();
    at_clearbtn->setText(tr("Clear"));
    hbox->addWidget(at_clearbtn);
    at_clearbtn->setCheckable(true);
    connect(at_clearbtn, SIGNAL(toggled(bool)),
        this, SLOT(clear_btn_slot(bool)));

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    at_over = new QRadioButton(tr("Override"));;
    hbox->addWidget(at_over);
    connect(at_over, SIGNAL(toggled(bool)),
        this, SLOT(over_btn_slot(bool)));

    at_skip = new QRadioButton(tr("Skip"));;
    hbox->addWidget(at_skip);
    connect(at_skip, SIGNAL(toggled(bool)),
        this, SLOT(skip_btn_slot(bool)));

    // Title label
    //
    QGroupBox *gb = new QGroupBox();
    vbox->addWidget(gb);
    hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    
    QLabel *label = new QLabel(tr("Cells in Override Table"));
    hbox->addWidget(label);

    // Scrolled text area
    //
    wb_textarea = new QTtextEdit();
    vbox->addWidget(wb_textarea);

    wb_textarea->setReadOnly(true);
    wb_textarea->setMouseTracking(true);
    wb_textarea->setAcceptDrops(true);
    vbox->addWidget(wb_textarea);
    connect(wb_textarea, SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));
    connect(wb_textarea, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));
    connect(wb_textarea, SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(mouse_motion_slot(QMouseEvent*)));
    connect(wb_textarea,
        SIGNAL(mime_data_handled(const QMimeData*, int*)),
        this, SLOT(mime_data_handled_slot(const QMimeData*, int*)));
    connect(wb_textarea, SIGNAL(mime_data_delivered(const QMimeData*, int*)),
        this, SLOT(mime_data_delivered_slot(const QMimeData*, int*)));

    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_FIXED))
        wb_textarea->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    // Dismiss button
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Dismiss");
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTauxTabDlg::~QTauxTabDlg()
{
    instPtr = 0;
    if (at_caller)
        QTdev::Deselect(at_caller);
    if (at_add_pop)
        at_add_pop->popdown();
    if (at_remove_pop)
        at_remove_pop->popdown();
    if (at_clear_pop)
        at_clear_pop->popdown();
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTauxTabDlg
#include "qtinterf/qtmacos_event.h"
#endif


void
QTauxTabDlg::update()
{
    select_range(0, 0);
    CDcellTab *ct = CDcdb()->auxCellTab();
    if (ct) {
        int wid = wb_textarea->width();
        int cols = (wid-4)/QTfont::stringWidth(0, wb_textarea);
        stringlist *s0 = ct->list();
        char *newtext = stringlist::col_format(s0, cols);
        stringlist::destroy(s0);
        double val = wb_textarea->get_scroll_value();
        wb_textarea->set_chars(newtext);
        wb_textarea->set_scroll_value(val);
    }
    else
        wb_textarea->set_chars("");

    if (CDvdb()->getVariable(VA_SkipOverrideCells)) {
        QTdev::SetStatus(at_over, false);
        QTdev::SetStatus(at_skip, true);
    }
    else {
        QTdev::SetStatus(at_over, true);
        QTdev::SetStatus(at_skip, false);
    }
    check_sens();
}


// Select the chars in the range, start=end deselects existing.
//
void
QTauxTabDlg::select_range(int start, int end)
{
    wb_textarea->select_range(start, end);
    at_start = start;
    at_end = end;
    check_sens();
}


void
QTauxTabDlg::check_sens()
{
    if (wb_textarea->has_selection())
        at_rembtn->setEnabled(true);
    else
        at_rembtn->setEnabled(false);
}


// Static function.
// Callback for the Add dialog.
//
ESret
QTauxTabDlg::at_add_cb(const char *name, void*)
{
    if (name) {
        if (CDcdb()->auxCellTab()) {
            if (!CDcdb()->auxCellTab()->add(name, false)) {
                QTauxTabDlg::self()->at_add_pop->update(
                    "Name not found, try again: ", 0);
                return (ESTR_IGN);
            }
            QTauxTabDlg::self()->update();
        }
    }
    return (ESTR_DN);
}


// Static function.
// Callback for the Remove dialog.
//
void
QTauxTabDlg::at_remove_cb(bool state, void *arg)
{
    char *name = (char*)arg;
    if (state && name && CDcdb()->auxCellTab()) {
        CDcdb()->auxCellTab()->remove(CD()->CellNameTableAdd(name));
        QTauxTabDlg::self()->update();
    }
    delete [] name;
}


// Static function.
// Callback for the Clear dialog.
//
void
QTauxTabDlg::at_clear_cb(bool state, void*)
{
    if (state && CDcdb()->auxCellTab()) {
        CDcdb()->auxCellTab()->clear();
        QTauxTabDlg::self()->update();
    }
}


void
QTauxTabDlg::add_btn_slot(bool state)
{
    if (at_add_pop)
        at_add_pop->popdown();
    if (at_remove_pop)
        at_remove_pop->popdown();
    if (at_clear_pop)
        at_clear_pop->popdown();

    if (state) {
        at_add_pop = PopUpEditString((GRobject)at_addbtn,
            GRloc(), "Enter cellname: ", 0,
            at_add_cb, 0, 250, 0, false, 0);
        if (at_add_pop)
            at_add_pop->register_usrptr((void**)&at_add_pop);
    }
}


void
QTauxTabDlg::rem_btn_slot(bool state)
{
    if (at_add_pop)
        at_add_pop->popdown();
    if (at_remove_pop)
        at_remove_pop->popdown();
    if (at_clear_pop)
        at_clear_pop->popdown();

    if (state) {
        char *string = at_start != at_end ?
            wb_textarea->get_chars(at_start, at_end) : 0;
        if (string) {
            at_remove_pop = PopUpAffirm(at_rembtn, GRloc(),
                "Confirm - remove selected cell from table?", at_remove_cb,
                lstring::copy(string));
            if (at_remove_pop)
                at_remove_pop->register_usrptr((void**)&at_remove_pop);
            delete [] string;
        }
        else
            QTdev::Deselect(at_rembtn);
    }
}


void
QTauxTabDlg::clear_btn_slot(bool state)
{
    if (at_add_pop)
        at_add_pop->popdown();
    if (at_remove_pop)
        at_remove_pop->popdown();
    if (at_clear_pop)
        at_clear_pop->popdown();

    if (state) {
        at_clear_pop = PopUpAffirm(at_clearbtn, GRloc(),
            "Confirm - remove all names from table?", at_clear_cb, 0);
        if (at_clear_pop)
            at_clear_pop->register_usrptr((void**)&at_clear_pop);
    }
}


void
QTauxTabDlg::help_btn_slot()
{
    PopUpHelp("xic:overtab");
}


void
QTauxTabDlg::over_btn_slot(bool state)
{
    if (state)
        CDvdb()->clearVariable(VA_SkipOverrideCells);
}


void
QTauxTabDlg::skip_btn_slot(bool state)
{
    if (state)
        CDvdb()->setVariable(VA_SkipOverrideCells, "");
}


void
QTauxTabDlg::dismiss_btn_slot()
{
    Cvt()->PopUpAuxTab(0, MODE_OFF);
}


void
QTauxTabDlg::resize_slot(QResizeEvent*)
{
    update();
}


void
QTauxTabDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonRelease) {
        ev->accept();
        at_dragging = false;
        return;
    }
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    int vsv = wb_textarea->verticalScrollBar()->value();
    int hsv = wb_textarea->horizontalScrollBar()->value();
    select_range(0, 0);

    char *str = wb_textarea->get_chars();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    int posn = wb_textarea->document()->documentLayout()->hitTest(
        QPointF(xx + hsv, yy + vsv), Qt::ExactHit);

    if (isspace(str[posn])) {
        // Clicked on white space.
        delete [] str;
        return;
    }

    char *start = str + posn;
    char *end = start;
    while (!isspace(*start) && start > str)
        start--;
    if (isspace(*start))
        start++;
    while (!isspace(*end) && *end)
        end++;
    *end = 0;

    // The top level cells are listed with an '*'.
    // Modified cells are listed  with a '+'.
    while ((*start == '+' || *start == '*') && !CDcdb()->findSymbol(start))
        start++;

    if (start == end) {
        delete [] str;
        return;
    }
    select_range(start - str, end - str);
    delete [] str;
    // Don't let the scroll position change.
    wb_textarea->verticalScrollBar()->setValue(vsv);
    wb_textarea->horizontalScrollBar()->setValue(hsv);

    at_dragging = true;
    at_drag_x = xx;
    at_drag_y = yy;
}


void
QTauxTabDlg::mouse_motion_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseMove) {
        ev->ignore();
        return;
    }
    ev->accept();

    if (!at_dragging)
        return;

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    if (abs(ev->position().x() - at_drag_x) < 5 &&
            abs(ev->position().y() - at_drag_y) < 5)
#else
    if (abs(ev->x() - at_drag_x) < 5 && abs(ev->y() - at_drag_y) < 5)
#endif
        return;

    char *sel = wb_textarea->get_selection();
    if (!sel)
        return;

    at_dragging = false;
    QDrag *drag = new QDrag(wb_textarea);
    QMimeData *mimedata = new QMimeData();
    QByteArray qdata((const char*)sel, strlen(sel)+1);
    mimedata->setData("text/plain", qdata);
    delete [] sel;
    drag->setMimeData(mimedata);
    drag->exec(Qt::CopyAction);
}


void
QTauxTabDlg::mime_data_handled_slot(const QMimeData *dta, int *accpt) const
{
    if (dta->hasFormat("text/twostring") || dta->hasFormat("text/plain"))
        *accpt = 1;
    else
        *accpt = -1;
}


void
QTauxTabDlg::mime_data_delivered_slot(const QMimeData *dta, int *accpt)
{
    *accpt = -1;
    if (!CDcdb()->auxCellTab())
        return;

    QByteArray data_ba;
    if (dta->hasFormat("text/twostring"))
        data_ba = dta->data("text/twostring");
    else if (dta->hasFormat("text/plain"))
        data_ba = dta->data("text/plain");
    else
        return;
    char *src = lstring::copy(data_ba.constData());
    if (!src)
        return;

    // Drops from content lists may be in the form
    // "fname_or_chd\ncellname".  Keep the cellname.
    char *t = strchr(src, '\n');
    if (t) {
        t++;
        if (*t) {
            if (CDcdb()->auxCellTab()->add(t, false))
                update();
        }
    }
    else {
        if (CDcdb()->auxCellTab()->add(src, false))
            update();
    }
    delete [] src;
    *accpt = 1;
}


void
QTauxTabDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_FIXED))
            wb_textarea->setFont(*fnt);
        update();
    }
}

