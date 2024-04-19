
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

#include "qtprpinfo.h"
#include "undolist.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "cd_hypertext.h"
#include "cd_propnum.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qttextw.h"

#include <QApplication>
#include <QLayout>
#include <QPushButton>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>


//-----------------------------------------------------------------------------
// QTprpInfoDlg:  Dialog to view object properties.
// Called when the Property Editor (QTprpEditorDlg) is active and has
// the Info button pressed and the user clicks on an object.  This
// allows copy/paste of properties from the object to the current
// selection in the Property Editor.

// Static function.
QTprpBase *
QTprpBase::prptyInfoPtr()
{
    return (QTprpInfoDlg::self());
}


// Pop up the property panel, list the properties of odesc.
//
void
cEdit::PopUpPropertyInfo(CDo *odesc, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTprpInfoDlg::self();
        return;
    }
    if (QTprpInfoDlg::self()) {
        QTprpInfoDlg::self()->update(odesc);
        return;
    }
    if (mode == MODE_UPD)
        return;
    if (!odesc)
        return;

    new QTprpInfoDlg(odesc);

    QTprpInfoDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_UR), QTprpInfoDlg::self(),
        QTmainwin::self()->Viewport());
    QTprpInfoDlg::self()->show();
}


// Called when odold is being deleted.
//
void
cEdit::PropertyInfoPurge(CDo *odold, CDo *odnew)
{
    if (QTprpInfoDlg::self())
        QTprpInfoDlg::self()->purge(odold, odnew);
}
// End of cEdit functions.


QTprpInfoDlg *QTprpInfoDlg::instPtr;

QTprpInfoDlg::QTprpInfoDlg(CDo *odesc) : QTprpBase(this)
{
    instPtr = this;

    setWindowTitle(tr("Properties"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // scrolled text area
    //
    wb_textarea = new QTtextEdit();
    wb_textarea->setReadOnly(true);
    wb_textarea->setMouseTracking(true);
    wb_textarea->setAcceptDrops(true);
    vbox->addWidget(wb_textarea);
    connect(wb_textarea, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));
    connect(wb_textarea, SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(mouse_release_slot(QMouseEvent*)));
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

    // dismiss button
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Dismiss");
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update(odesc);
}


QTprpInfoDlg::~QTprpInfoDlg()
{
    instPtr = 0;
    if (pb_odesc)
        DSP()->ShowCurrentObject(ERASE, pb_odesc, HighlightingColor);
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTprpInfoDlg
#include "qtinterf/qtmacos_event.h"
#endif


void
QTprpInfoDlg::update(CDo *odesc)
{
    if (pb_odesc)
        DSP()->ShowCurrentObject(ERASE, pb_odesc, HighlightingColor);
    PrptyText::destroy(pb_list);
    pb_list = 0;
    if (odesc)
        pb_odesc = odesc;
    if (pb_odesc) {
        CDs *cursd = CurCell();
        pb_list = (cursd ? XM()->PrptyStrings(pb_odesc, cursd) : 0);
    }
    update_display();
    if (pb_odesc)
        DSP()->ShowCurrentObject(DISPLAY, pb_odesc, HighlightingColor);
}


void
QTprpInfoDlg::purge(CDo *odold, CDo *odnew)
{
    if (odold == pb_odesc) {
        if (odnew)
            ED()->PopUpPropertyInfo(odnew, MODE_UPD);
        else
            ED()->PopUpPropertyInfo(0, MODE_OFF);
    }
}


void
QTprpInfoDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonPress) {
        ev->accept();
        handle_button_down(ev);
        return;
    }
    ev->ignore();
}


void
QTprpInfoDlg::mouse_release_slot(QMouseEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonRelease) {
        ev->accept();
        handle_button_up(ev);
        return;
    }
    ev->ignore();
}


void
QTprpInfoDlg::mouse_motion_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseMove) {
        ev->ignore();
        return;
    }
    ev->accept();
    handle_mouse_motion(ev);
}


void
QTprpInfoDlg::mime_data_handled_slot(const QMimeData *d, int *accpt) const
{
    *accpt = is_mime_data_handled(d) ? 1 : -1;
}


void
QTprpInfoDlg::mime_data_delivered_slot(const QMimeData *d, int *accpt)
{
    *accpt = is_mime_data_delivered(d) ? 1 : -1;
}


void
QTprpInfoDlg::dismiss_btn_slot()
{
    ED()->PopUpPropertyInfo(0, MODE_OFF);
}


void
QTprpInfoDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_FIXED))
            wb_textarea->setFont(*fnt);
    }
}

