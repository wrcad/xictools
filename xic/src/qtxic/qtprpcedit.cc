
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

#include "qtprpcedit.h"
#include "edit.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "menu.h"
#include "edit_menu.h"
#include "events.h"
#include "promptline.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qttextw.h"

#include <QLayout>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QMouseEvent>
#include <QMimeData>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>


//-----------------------------------------------------------------------------
// QTcellPrpDlg:  Dialog to modify proerties of the current cell.
// Called from the main menu: Edit/Cell Properties.
//
// Help system keywords used:
//  xic:cprop

// Pop up the cell properties editor.
//
void
cEdit::PopUpCellProperties(ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTcellPrpDlg::self();
        return;
    }
    if (QTcellPrpDlg::self()) {
        QTcellPrpDlg::self()->update();
        return;
    }
    if (mode == MODE_UPD)
        return;

    new QTcellPrpDlg();

    QTcellPrpDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTcellPrpDlg::self(),
        QTmainwin::self()->Viewport());
    QTcellPrpDlg::self()->show();
}
// End of cEdit functions.


// For possible future drag/drop support.  The definitions will need to
// be added to the header file.
//#define PRPC_DD

QTcellPrpDlg::sAddEnt QTcellPrpDlg::pc_elec_addmenu[] = {
    sAddEnt("param", P_PARAM),
    sAddEnt("other", P_OTHER),
    sAddEnt("virtual", P_VIRTUAL),
    sAddEnt("flatten", P_FLATTEN),
    sAddEnt(0, 0)
};

QTcellPrpDlg::sAddEnt QTcellPrpDlg::pc_phys_addmenu[] = {
    sAddEnt("any", -1),
    sAddEnt("flags", XICP_FLAGS),
    sAddEnt("flatten", XICP_EXT_FLATTEN),
    sAddEnt("pc_script", XICP_PC_SCRIPT),
    sAddEnt("pc_params", XICP_PC_PARAMS),
    sAddEnt(0, 0)
};

QTcellPrpDlg *QTcellPrpDlg::instPtr;

QTcellPrpDlg::QTcellPrpDlg() : QTbag(this)
{
    instPtr = this;
    pc_edit = 0;
    pc_del = 0;
    pc_add = 0;
    pc_addmenu = 0;
    pc_list = 0;

    pc_line_selected = -1;
    pc_action_calls = 0;
    pc_start = 0;
    pc_end = 0;
    pc_dspmode = -1;

    setWindowTitle(tr("Cell Property Editor"));
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

    // top row buttons
    //
    pc_edit = new QPushButton(tr("Edit"));
    hbox->addWidget(pc_edit);
    pc_edit->setCheckable(true);
    pc_edit->setAutoDefault(false);
    connect(pc_edit, SIGNAL(toggled(bool)), this, SLOT(edit_btn_slot(bool)));

    pc_del = new QPushButton(tr("Delete"));
    hbox->addWidget(pc_del);
    pc_del->setAutoDefault(false);
    connect(pc_del, SIGNAL(toggled(bool)), this, SLOT(del_btn_slot(bool)));

    pc_add = new QPushButton(tr("Add"));
    pc_add->setCheckable(true);
    pc_add->setAutoDefault(false);
    hbox->addWidget(pc_add);

    pc_addmenu = new QMenu();
    pc_add->setMenu(pc_addmenu);
    connect(pc_addmenu, SIGNAL(triggered(QAction*)),
        this, SLOT(add_menu_slot(QAction*)));

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // scrolled text area
    //
    wb_textarea = new QTtextEdit();
    wb_textarea->setReadOnly(true);
    wb_textarea->setMouseTracking(true);
    vbox->addWidget(wb_textarea);
    connect(wb_textarea, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));
    connect(wb_textarea, SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(mouse_release_slot(QMouseEvent*)));

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_FIXED))
        wb_textarea->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    // dismiss button
    //
    btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTcellPrpDlg::~QTcellPrpDlg()
{
    if (pc_action_calls) {
        EV()->InitCallback();
        pc_action_calls = 0;
    }
    instPtr = 0;
    PrptyText::destroy(pc_list);
    MainMenu()->MenuButtonSet(0, MenuCPROP, false);
    PL()->AbortLongText();
}


void
QTcellPrpDlg::update()
{
    PrptyText::destroy(pc_list);
    CDs *cursd = CurCell();
    pc_list = cursd ? XM()->PrptyStrings(cursd) : 0;
    update_display();

    if (DSP()->CurMode() == pc_dspmode)
        return;
    pc_dspmode = DSP()->CurMode();

    // Set up the add menu.
    pc_addmenu->clear();
    if (DSP()->CurMode() == Physical) {
        for (int i = 0; ; i++) {
            const char *s = pc_phys_addmenu[i].name;
            if (!s)
                break;
            QAction *a = pc_addmenu->addAction(s);
            a->setData(i);
        }
    }
    else {
        for (int i = 0; ; i++) {
            const char *s = pc_elec_addmenu[i].name;
            if (!s)
                break;
            QAction *a = pc_addmenu->addAction(s);
            a->setData(i);
        }
    }
}


// Return the PrptyText element corresponding to the selected line, or 0 if
// there is no selection.
//
PrptyText *
QTcellPrpDlg::get_selection()
{
    int start, end;
    start = pc_start;
    end = pc_end;
    if (start == end)
        return (0);
    for (PrptyText *p = pc_list; p; p = p->next()) {
        if (start >= p->start() && start < p->end())
            return (p);
    }
    return (0);
}


void
QTcellPrpDlg::update_display()
{
    QColor c1 = QTbag::PopupColor(GRattrColorHl4);
    QColor c2 = QTbag::PopupColor(GRattrColorHl2);
    QScrollBar *vsb = wb_textarea->verticalScrollBar();
    double val = 0.0;
    if (vsb)
        val = vsb->value();
    wb_textarea->clear();

    if (!pc_list) {
        wb_textarea->setTextColor(c1);
        wb_textarea->insertPlainText(tr("Current cell has no properties."));
    }
    else {
        int cnt = 0;
        QColor blk("black");
        for (PrptyText *p = pc_list; p; p = p->next()) {
            p->set_start(cnt);

            QColor *c;
            const char *s = p->head();
            if (*s == '(')
                s++;
            int num = atoi(s);
            if (DSP()->CurMode() == Physical) {
                if (prpty_gdsii(num) || prpty_global(num) ||
                        prpty_reserved(num))
                    c = &blk;
                else if (prpty_pseudo(num))
                    c = &c2;
                else
                    c = &c1;
            }
            else {
                switch (num) {
                case P_PARAM:
                case P_OTHER:
                case P_VIRTUAL:
                case P_FLATTEN:
                case P_MACRO:
                    c = &c1;
                    break;
                default:
                    c = &blk;
                    break;
                }
            }
            wb_textarea->setTextColor(*c);
            wb_textarea->insertPlainText(p->head());
            cnt += strlen(p->head());
            wb_textarea->setTextColor(blk);
            wb_textarea->insertPlainText(p->string());
            cnt += strlen(p->string());
            wb_textarea->insertPlainText("\n");
            p->set_end(cnt);
            cnt++;
        }
    }
    if (vsb)
        vsb->setValue(val);
    pc_line_selected = -1;
}


// Select the chars in the range, start=end deselects existing.
//
void
QTcellPrpDlg::select_range(int start, int end)
{
    wb_textarea->select_range(start, end);
    pc_start = start;
    pc_end = end;
}


void
QTcellPrpDlg::edit_btn_slot(bool state)
{
    if (!state) {
        EV()->InitCallback();
        return;
    }
    QTdev::Deselect(pc_del);

    pc_action_calls++;
    PrptyText *p = get_selection();
    if (p)
        ED()->cellPrptyEdit(p);
    QTdev::Deselect(pc_edit);
    pc_action_calls--;
}


void
QTcellPrpDlg::del_btn_slot(bool state)
{
    if (!state)
        return;
    QTdev::Deselect(pc_edit);

    pc_action_calls++;
    PrptyText *p = get_selection();
    if (p)
        ED()->cellPrptyRemove(p);
    QTdev::Deselect(pc_del);
    pc_action_calls--;
}


void
QTcellPrpDlg::add_menu_slot(QAction *a)
{
    pc_action_calls++;
    int ix = a->data().toInt();
    sAddEnt *ae;
    if (DSP()->CurMode() == Electrical)
        ae = &pc_elec_addmenu[ix];
    else
        ae = &pc_elec_addmenu[ix];
    ED()->cellPrptyAdd(ae->value);
    pc_action_calls--;
}


void
QTcellPrpDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:cprop"))
}


void
QTcellPrpDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    int vsv = wb_textarea->verticalScrollBar()->value();
    int hsv = wb_textarea->horizontalScrollBar()->value();

#ifdef PRPC_DD
    pc_dragging = false;
#endif
    QByteArray qba = wb_textarea->toPlainText().toLatin1();
    const char *str = qba.constData();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    int posn = wb_textarea->document()->documentLayout()->hitTest(
        QPointF(xx + hsv, yy + vsv), Qt::ExactHit);
    const char *line_start = str;
    int linesel = 0;
    for (int i = 0; i <= posn; i++) {
        if (str[i] == '\n') {
            if (i == posn) {
                // Clicked to  right of line.
                return;
            }
            linesel++;
            line_start = str + i+1;
        }
    }
    if (line_start && *line_start != '\n') {
        PrptyText *p = pc_list;
        posn = line_start - str;
        for ( ; p; p = p->next()) {
            if (posn >= p->start() && posn < p->end())
                break;
        }
        if (p && pc_line_selected != linesel) {
            pc_line_selected = linesel;
            select_range(p->start() + strlen(p->head()), p->end());
            // Don't let the scroll position change.
            wb_textarea->verticalScrollBar()->setValue(vsv);
            wb_textarea->horizontalScrollBar()->setValue(hsv);
#ifdef PRPC_DD
            pc_drag_x = xx;
            pc_drag_y = yy;
            pc_dragging = true;
#endif
            return;
        }
    }
    pc_line_selected = -1;
    select_range(0, 0);
}


void
QTcellPrpDlg::mouse_release_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonRelease) {
        ev->ignore();
        return;
    }
    ev->accept();
#ifdef PRPC_DD
    pc_dragging = false;
#endif
}


#ifdef PRPC_DD

void
QTcellPrpDlg::mouse_motion_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseMove) {
        ev->ignore();
        return;
    }
    ev->accept();

    if (!pc_dragging)
        return;
    if (abs(ev->x() - pc_drag_x) < 5 && abs(ev->y() - pc_drag_y) < 5)
        return;
    PrptyText *p = get_selection();
    if (!p)
        return;
    pc_dragging = false;

    int sz = 0;
    char *bf = 0;
    if (p->prpty()) {
        CDs *cursd =  CurCell(true);
        if (cursd) {
            hyList *hp = cursd->hyPrpList(pc_odesc, p->prpty());
            char *s = hyList::string(hp, HYcvAscii, true);
            hyList::destroy(hp);
            sz = sizeof(int) + strlen(s) + 1;
            bf = new char[sz];
            *(int*)bf = p->prpty()->value();
            strcpy(bf + sizeof(int), s);
            delete [] s;
        }
    }
    else {
        QString qs = wb_textarea->toPlainText();
        QByteArray qba = qs.toLatin1();
        sz = p->end() - (p->start() + strlen(p->head())) + sizeof(int) + 1;
        bf = new char[sz];
        const char *q = p->head();
        if (!isdigit(*q))
            q++;
        *(int*)bf = atoi(q);
        int i = sizeof(int);
        for (int j = p->start() + strlen(p->head()); j < p->end(); j++)
            bf[i++] = qba[j];
        bf[i] = 0;
    }

    QDrag *drag = new QDrag(wb_textarea);
    QMimeData *mimedata = new QMimeData();
    QByteArray qdata((const char*)bf, sz);
    mimedata->setData("text/property", qdata);
    delete [] bf;
    drag->setMimeData(mimedata);
    drag->exec(Qt::CopyAction);
}


void
QTcellPrpDlg::mime_data_handled_slot(const QMimeData *dta, bool *accpt) const
{
    if (dta->hasFormat("text/property"))
        *accpt = true;
}


void
QTcellPrpDlg::mime_data_delivered_slot(const QMimeData *dta, bool *accpt)
{
    if (dta->hasFormat("text/property")) {
        *accpt = true;
        if (!pc_odesc) {
            QTpkg::self()->RegisterTimeoutProc(3000, pc_bad_cb, this);
            PopUpMessage("Can't add property, no object selected.", false,
                false, false, GRloc(LW_LR));
        }
        else {
            QByteArray bary = dta->data("text/property");
            int num = *(int*)bary.data();
            const char *val = (const char*)bary.data() + sizeof(int);
            bool accept = false;
            // Note: the window text is updated by call to PrptyRelist() in
            // CommitChangges()
            if (DSP()->CurMode() == Electrical) {
                if (pc_odesc->type() == CDINSTANCE) {
                    if (num == P_MODEL || num == P_VALUE || num == P_PARAM ||
                            num == P_OTHER || num == P_NOPHYS ||
                            num == P_FLATTEN || num == P_SYMBLC ||
                            num == P_RANGE || num == P_DEVREF) {
                        CDs *cursde = CurCell(Electrical, true);
                        if (cursde) {
                            Ulist()->ListCheck("addprp", cursde, false);
                            CDp *pdesc =
                                num != P_OTHER ? OCALL(pc_odesc)->prpty(num)
                                : 0;
                            hyList *hp = new hyList(cursde, (char*)val,
                                HYcvAscii);
                            ED()->prptyModify(OCALL(pc_odesc), pdesc, num,
                                0, hp);
                            hyList::destroy(hp);
                            Ulist()->CommitChanges(true);
                            accept = true;
                        }
                    }
                }
            }
            else {
                CDs *cursdp = CurCell(Physical);
                if (cursdp) {
                    Ulist()->ListCheck("ddprp", cursdp, false);
                    DSP()->ShowOdescPhysProperties(pc_odesc, ERASE);

                    CDp *newp = new CDp((char*)val, num);
                    Ulist()->RecordPrptyChange(cursdp, pc_odesc, 0, newp);

                    Ulist()->CommitChanges(true);
                    DSP()->ShowOdescPhysProperties(pc_odesc, DISPLAY);
                    accept = true;
                }
            }
            if (!accept) {
                QTpkg::self()->RegisterTimeoutProc(3000, pc_bad_cb, this);
                PopUpMessage("Can't add property, incorrect type.", false,
                    false, false, GRloc(LW_LR));
            }
        }
    }
}

#endif


void
QTcellPrpDlg::dismiss_btn_slot()
{
    ED()->PopUpCellProperties(MODE_OFF);
}


void
QTcellPrpDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_FIXED)) {
            wb_textarea->setFont(*fnt);
            update_display();
        }
    }
}

