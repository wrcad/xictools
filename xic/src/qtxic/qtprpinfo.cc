
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

#include <QLayout>
#include <QPushButton>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>


//--------------------------------------------------------------------------
// Pop up to view object properties
//

namespace {
    /*
    GtkTargetEntry target_table[] = {
        { (char*)"property",     0, 0 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);
    */
}


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
        if (QTprpInfoDlg::self())
            QTprpInfoDlg::self()->deleteLater();
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

QTprpInfoDlg::QTprpInfoDlg(CDo *odesc)
{
    instPtr = this;

    setWindowTitle(tr("Properties"));
    setWindowFlags(Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
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
    connect(wb_textarea, SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(mouse_motion_slot(QMouseEvent*)));
    connect(wb_textarea, SIGNAL(mime_data_received(const QMimeData*)),
        this, SLOT(mime_data_received_slot(const QMimeData*)));

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_FIXED))
        wb_textarea->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

        /*
    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
    const char *bclr = GTKpkg::self()->GetAttrColor(GRattrColorLocSel);
    gtk_text_buffer_create_tag(textbuf, "primary", "background", bclr,
        "paragraph-background", bclr, NULL);

    // for passing hypertext via selections, see gtkhtext.cc
    g_object_set_data(G_OBJECT(wb_textarea), "hyexport", (void*)2);
    */

    // dismiss button
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
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
QTprpInfoDlg::mime_data_received_slot(const QMimeData *d)
{
    handle_mime_data_received(d);
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
        if (FC.getFont(&fnt, FNT_FIXED))
            wb_textarea->setFont(*fnt);
    }
}

