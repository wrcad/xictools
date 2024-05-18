
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

#include "qtprpedit.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "menu.h"
#include "edit_menu.h"
#include "pbtn_menu.h"
#include "events.h"
#include "undolist.h"
#include "promptline.h"
#include "qtinterf/qttextw.h"

#include <QApplication>
#include <QLayout>
#include <QToolButton>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>


//-----------------------------------------------------------------------------
// QTprpEditorDlg:  Dialog to modify proerties of objects.
// Called from the main menu: Edit/Properties.
//
// Help system keywords used:
//  prppanel

// Pop up the property panel, list the properties of odesc.  The activ
// argument sets the state of the Activate button and menu sensitivity.
//
void
cEdit::PopUpProperties(CDo *odesc, ShowMode mode, PRPmode activ)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTprpEditorDlg::self();
        return;
    }
    if (QTprpEditorDlg::self()) {
        QTprpEditorDlg::self()->update(odesc, activ);
        return;
    }
    if (mode == MODE_UPD)
        return;

    new QTprpEditorDlg(odesc, activ);

    QTprpEditorDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTprpEditorDlg::self(),
        QTmainwin::self()->Viewport());
    QTprpEditorDlg::self()->show();
}


// Given the offset, return the containing PrptyText element and object desc,
// called from selection processing code, supports Property Editor and
// Property Info pop-ups.
//
PrptyText *
cEdit::PropertyResolve(int code, int offset, CDo **odp)
{
    if (odp)
        *odp = 0;
    if (code == 1) {
        if (QTprpEditorDlg::self())
            return (QTprpEditorDlg::self()->resolve(offset, odp));
    }
    else if (code == 2) {
        if (QTprpBase::prptyInfoPtr())
            return (QTprpBase::prptyInfoPtr()->resolve(offset, odp));
    }
    return (0);
}


// Called when odold is being deleted.
//
void
cEdit::PropertyPurge(CDo *odold, CDo *odnew)
{
    if (QTprpEditorDlg::self())
        QTprpEditorDlg::self()->purge(odold, odnew);
}


// Select and return the first matching property.
//
PrptyText *
cEdit::PropertySelect(int which)
{
    if (QTprpEditorDlg::self())
        return (QTprpEditorDlg::self()->select(which));
    return (0);
}


PrptyText *
cEdit::PropertyCycle(CDp *pd, bool(*checkfunc)(const CDp*), bool rev)
{
    if (QTprpEditorDlg::self())
        return (QTprpEditorDlg::self()->cycle(pd, checkfunc, rev));
    return (0);
}


void
cEdit::RegisterPrptyBtnCallback(int(*cb)(PrptyText*))
{
    if (QTprpEditorDlg::self())
        QTprpEditorDlg::self()->set_btn_callback(cb);
}
// End of cEdit functions.


QTprpEditorDlg *QTprpEditorDlg::instPtr;

QTprpEditorDlg::sAddEnt QTprpEditorDlg::po_elec_addmenu[] = {
    sAddEnt("name", P_NAME),
    sAddEnt("model", P_MODEL),
    sAddEnt("value", P_VALUE),
    sAddEnt("param", P_PARAM),
    sAddEnt("devref", P_DEVREF),
    sAddEnt("other", P_OTHER),
    sAddEnt("nophys", P_NOPHYS),
    sAddEnt("flatten", P_FLATTEN),
    sAddEnt("nosymb", P_SYMBLC),
    sAddEnt("range", P_RANGE),
    sAddEnt(0, 0)
};

QTprpEditorDlg::sAddEnt QTprpEditorDlg::po_phys_addmenu[] = {
    sAddEnt("any", -1),
    sAddEnt("flatten", XICP_EXT_FLATTEN),
    sAddEnt("nomerge", XICP_NOMERGE),
    sAddEnt(0, 0)
};


QTprpEditorDlg::QTprpEditorDlg(CDo *odesc, PRPmode activ) : QTprpBase(this)
{
    instPtr = this;
    po_activ = 0;
    po_edit = 0;
    po_del = 0;
    po_add = 0;
    po_addmenu = 0;
    po_global = 0;
    po_info = 0;
    po_name_btn = 0;
    po_dspmode = -1;

    setWindowTitle(tr("Property Editor"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // top row buttons
    //
    po_edit = new QToolButton();
    po_edit->setText(tr("Edit"));
    hbox->addWidget(po_edit);
    po_edit->setCheckable(true);
    connect(po_edit, &QAbstractButton::toggled,
        this, &QTprpEditorDlg::edit_btn_slot);

    po_del = new QToolButton();
    po_del->setText(tr("Delete"));
    hbox->addWidget(po_del);
    po_del->setCheckable(true);
    connect(po_del, &QAbstractButton::toggled,
        this, &QTprpEditorDlg::del_btn_slot);

    po_add = new QToolButton();
    po_add->setText(tr("Add"));
    po_add->setCheckable(true);
    hbox->addWidget(po_add);

    po_addmenu = new QMenu();
    po_add->setMenu(po_addmenu);
    po_add->setPopupMode(QToolButton::InstantPopup);
    connect(po_addmenu, &QMenu::triggered,
        this, &QTprpEditorDlg::add_menu_slot);

    po_global = new QToolButton();
    po_global->setText(tr("Global"));
    hbox->addWidget(po_global);
    po_global->setCheckable(true);
    connect(po_global, &QAbstractButton::toggled,
        this, &QTprpEditorDlg::global_btn_slot);

    po_info = new QToolButton();
    po_info->setText(tr("Info"));
    hbox->addWidget(po_info);
    po_info->setCheckable(true);
    connect(po_info, &QAbstractButton::toggled,
        this, &QTprpEditorDlg::info_btn_slot);

    hbox->addStretch(1);
    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTprpEditorDlg::help_btn_slot);

    // scrolled text area
    //
    wb_textarea = new QTtextEdit();
    wb_textarea->setReadOnly(true);
    wb_textarea->setMouseTracking(true);
    wb_textarea->setAcceptDrops(true);
    vbox->addWidget(wb_textarea);
    connect(wb_textarea, &QTtextEdit::press_event,
        this, &QTprpEditorDlg::mouse_press_slot);
    connect(wb_textarea, &QTtextEdit::release_event,
        this, &QTprpEditorDlg::mouse_release_slot);
    connect(wb_textarea, &QTtextEdit::motion_event,
        this, &QTprpEditorDlg::mouse_motion_slot);
    connect(wb_textarea, &QTtextEdit::mime_data_handled,
        this, &QTprpEditorDlg::mime_data_handled_slot);
    connect(wb_textarea, &QTtextEdit::mime_data_delivered,
        this, &QTprpEditorDlg::mime_data_delivered_slot);

    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_FIXED))
        wb_textarea->setFont(*fnt);
    connect(QTfont::self(), &QTfont::fontChanged,
        this, &QTprpEditorDlg::font_changed_slot, Qt::QueuedConnection);

    // activate and dismiss buttons
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    po_activ = new QToolButton();
    po_activ->setText(tr("Activate"));
    po_activ->setCheckable(true);
    hbox->addWidget(po_activ);
    connect(po_activ, &QAbstractButton::toggled,
        this, &QTprpEditorDlg::activ_btn_slot);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    hbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QTprpEditorDlg::dismiss_btn_slot);

    update(odesc, activ);
}


QTprpEditorDlg::~QTprpEditorDlg()
{
    instPtr = 0;
    if (MainMenu()->MenuButtonStatus(MMedit, MenuPRPTY) == 1) {
        MainMenu()->MenuButtonPress(MMedit, MenuPRPTY);
        if (pb_odesc)
            DSP()->ShowCurrentObject(ERASE, pb_odesc, MarkerColor);
    }
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTprpEditorDlg
#include "qtinterf/qtmacos_event.h"
#endif


void
QTprpEditorDlg::update(CDo *odesc, PRPmode activ)
{
    if (pb_odesc)
        DSP()->ShowCurrentObject(ERASE, pb_odesc, MarkerColor);
    PrptyText::destroy(pb_list);
    pb_list = 0;
    pb_odesc = odesc;
    if (odesc) {
        CDs *cursd = CurCell();
        pb_list = (cursd ? XM()->PrptyStrings(odesc, cursd) : 0);
    }
    update_display();
    if (pb_odesc)
        DSP()->ShowCurrentObject(DISPLAY, pb_odesc, MarkerColor);

    if (activ == PRPnochange)
        return;
    if (activ == PRPinactive) {
        if (QTdev::GetStatus(po_activ) == true) {
            QTdev::SetStatus(po_activ, false);
            activate(false);
        }
        QTdev::SetStatus(po_info, false);
        QTdev::SetStatus(po_global, false);
    }
    else if (activ == PRPactive) {
        if (QTdev::GetStatus(po_activ) == false) {
            QTdev::SetStatus(po_activ, true);
            activate(true);
        }
    }

    if (DSP()->CurMode() == po_dspmode)
        return;
    po_dspmode = DSP()->CurMode();

    // Set up the add menu.
    po_addmenu->clear();
    if (DSP()->CurMode() == Physical) {
        for (int i = 0; ; i++) {
            const char *s = po_phys_addmenu[i].name;
            if (!s)
                break;
            QAction *a = po_addmenu->addAction(s);
            a->setData(i);
        }
    }
    else {
        for (int i = 0; ; i++) {
            const char *s = po_elec_addmenu[i].name;
            if (!s)
                break;
            QAction *a = po_addmenu->addAction(s);
            a->setData(i);
            // Hard-code name button, index 0
            if (!i)
                po_name_btn = a;;
        }
    }
}


void
QTprpEditorDlg::purge(CDo *odold, CDo *odnew)
{
    if (odold == pb_odesc)
        ED()->PopUpProperties(odnew, MODE_UPD, PRPnochange);
}


PrptyText *
QTprpEditorDlg::select(int which)
{
    for (PrptyText *p = pb_list; p; p = p->next()) {
        if (!p->prpty())
            continue;
        if (p->prpty()->value() == which) {
            select_range(p->start() + strlen(p->head()), p->end());
            return (p);
        }
    }
    return (0);
}


PrptyText *
QTprpEditorDlg::cycle(CDp *pd, bool(*checkfunc)(const CDp*), bool rev)
{
    int cnt = 0;
    for (PrptyText *p = pb_list; p; p = p->next(), cnt++) ;
    if (!cnt)
        return (0);
    PrptyText **ary = new PrptyText*[cnt];
    cnt = 0;
    for (PrptyText *p = pb_list; p; p = p->next(), cnt++)
        ary[cnt] = p;

    PrptyText *p0 = 0;
    int j = 0;
    if (pd) {
        for (j = 0; j < cnt; j++) {
            if (ary[j]->prpty() == pd)
                break;
        }
    }
    if (!pd || j == cnt) {
        for (int i = 0; i < cnt; i++) {
            if (!ary[i]->prpty())
                continue;
            if (!checkfunc || checkfunc(ary[i]->prpty())) {
                p0 = ary[i];
                break;
            }
        }
    }
    else {
        if (!rev) {
            for (int i = j+1; i < cnt; i++) {
                if (!ary[i]->prpty())
                    continue;
                if (!checkfunc || checkfunc(ary[i]->prpty())) {
                    p0 = ary[i];
                    break;
                }
            }
            if (!p0) {
                for (int i = 0; i <= j; i++) {
                    if (!ary[i]->prpty())
                        continue;
                    if (!checkfunc || checkfunc(ary[i]->prpty())) {
                        p0 = ary[i];
                        break;
                    }
                }
            }
        }
        else {
            for (int i = j-1; i >= 0; i--) {
                if (!ary[i]->prpty())
                    continue;
                if (!checkfunc || checkfunc(ary[i]->prpty())) {
                    p0 = ary[i];
                    break;
                }
            }
            if (!p0) {
                for (int i = cnt-1; i >= j; i--) {
                    if (!ary[i]->prpty())
                        continue;
                    if (!checkfunc || checkfunc(ary[i]->prpty())) {
                        p0 = ary[i];
                        break;
                    }
                }
            }
        }
    }
    delete [] ary;
    if (p0)
        select_range(p0->start() + strlen(p0->head()), p0->end());
    return (p0);
}


void
QTprpEditorDlg::set_btn_callback(int(*cb)(PrptyText*))
{
    pb_btn_callback = cb;
}


void
QTprpEditorDlg::activate(bool activ)
{
    po_edit->setEnabled(activ);
    po_del->setEnabled(activ);
    po_add->setEnabled(activ);
    po_global->setEnabled(activ);
    po_info->setEnabled(activ);
}


void
QTprpEditorDlg::call_prpty_add(int which)
{
    po_global->setEnabled(false);
    ED()->prptyAdd(which);
    if (QTprpEditorDlg::self())
        po_global->setEnabled(true);
}


void
QTprpEditorDlg::call_prpty_edit(PrptyText *line)
{
    po_global->setEnabled(false);
    ED()->prptyEdit(line);
    if (QTprpEditorDlg::self())
        po_global->setEnabled(true);
}


void
QTprpEditorDlg::call_prpty_del(PrptyText *line)
{
    po_global->setEnabled(false);
    ED()->prptyRemove(line);
    if (QTprpEditorDlg::self())
        po_global->setEnabled(true);
}


void
QTprpEditorDlg::edit_btn_slot(bool state)
{
    if (!state)
       return;
    QTdev::Deselect(po_del);
    PrptyText *p = get_selection();
    call_prpty_edit(p);
    if (QTprpEditorDlg::self())
        QTdev::Deselect(po_edit);
}


void
QTprpEditorDlg::del_btn_slot(bool state)
{
    if (!state)
       return;
    QTdev::Deselect(po_edit);
    PrptyText *p = get_selection();
    call_prpty_del(p);
    if (QTprpEditorDlg::self())
        QTdev::Deselect(po_del);
}


void
QTprpEditorDlg::add_menu_slot(QAction *a)
{
    int ix = a->data().toInt();
    sAddEnt *ae;
    if (DSP()->CurMode() == Electrical)
        ae = &po_elec_addmenu[ix];
    else
        ae = &po_elec_addmenu[ix];
    call_prpty_add(ae->value);
}


void
QTprpEditorDlg::global_btn_slot(bool state)
{
    ED()->prptySetGlobal(state);
    if (po_name_btn)
        po_name_btn->setEnabled(!state);
}


void
QTprpEditorDlg::info_btn_slot(bool state)
{
    if (EV()->CurCmd() && EV()->CurCmd() != ED()->prptyCmd())
        EV()->CurCmd()->esc();

    ED()->prptySetInfoMode(state);
}


void
QTprpEditorDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("prppanel"))
}


void
QTprpEditorDlg::activ_btn_slot(bool state)
{
    if (EV()->CurCmd() && EV()->CurCmd() != ED()->prptyCmd())
        EV()->CurCmd()->esc();

    if (state) {
        if (MainMenu()->MenuButtonStatus(MMedit, MenuPRPTY) == 0)
            MainMenu()->MenuButtonPress(MMedit, MenuPRPTY);
    }
    else {
        QTdev::SetStatus(po_info, false);
        QTdev::SetStatus(po_global, false);
        if (MainMenu()->MenuButtonStatus(MMedit, MenuPRPTY) == 1)
            MainMenu()->MenuButtonPress(MMedit, MenuPRPTY);
    }
    activate(state);
}


void
QTprpEditorDlg::dismiss_btn_slot()
{
    ED()->PopUpProperties(0, MODE_OFF, PRPnochange);
}


void
QTprpEditorDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonPress) {
        ev->accept();
        handle_button_down(ev);
        return;
    }
    ev->ignore();
}


void
QTprpEditorDlg::mouse_release_slot(QMouseEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonRelease) {
        ev->accept();
        handle_button_up(ev);
        return;
    }
    ev->ignore();
}


void
QTprpEditorDlg::mouse_motion_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseMove) {
        ev->ignore();
        return;
    }
    ev->accept();
    handle_mouse_motion(ev);
}
 

void
QTprpEditorDlg::mime_data_handled_slot(const QMimeData *d, int *accpt) const
{
    *accpt = is_mime_data_handled(d) ? 1 : -1;
}


void
QTprpEditorDlg::mime_data_delivered_slot(const QMimeData *d, int *accpt)
{
    *accpt = is_mime_data_delivered(d) ? 1 : -1;
}


void
QTprpEditorDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_FIXED))
            wb_textarea->setFont(*fnt);
    }
}

