
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

#include "qtprpbase.h"
#include "undolist.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "cd_hypertext.h"
#include "cd_propnum.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qtmsg.h"
#include "qtinterf/qttextw.h"

#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDrag>
#include <QMimeData>
#include <QScrollBar>
#include <QTextCursor>


//-----------------------------------------------------------------------------
// QTprpBase:  Base class methods for property listing dialogs.
// Used in QTprpEditorDlg and QTprpInfoDlg.

PrptyText *
QTprpBase::resolve(int offset, CDo **odp)
{
    if (odp)
        *odp = pb_odesc;
    for (PrptyText *p = pb_list; p; p = p->next()) {
        if (offset >= p->start() && offset < p->end())
            return (p);
    }
    return (0);
}


// Return the PrptyText element corresponding to the selected line, or 0 if
// there is no selection.
//
PrptyText *
QTprpBase::get_selection()
{
    int start, end;
    start = pb_start;
    end = pb_end;
    if (start == end)
        return (0);
    for (PrptyText *p = pb_list; p; p = p->next()) {
        if (start >= p->start() && start < p->end())
            return (p);
    }
    return (0);
}


void
QTprpBase::update_display()
{
    QColor c1 = QTbag::PopupColor(GRattrColorHl4);
    QColor c2 = QTbag::PopupColor(GRattrColorHl2);
    QColor c3 = QTbag::PopupColor(GRattrColorHl1);
    QScrollBar *vsb = wb_textarea->verticalScrollBar();
    double val = 0.0;
    if (vsb)
        val = vsb->value();
    wb_textarea->clear();

    if (!pb_list) {
        wb_textarea->setTextColor(c1);
        wb_textarea->insertPlainText(
            pb_odesc ? "No properties found." : "No selection.");
    }
    else {
        int cnt = 0;
        QColor blk("black");
        for (PrptyText *p = pb_list; p; p = p->next()) {
            p->set_start(cnt);

            QColor *c, *cx = &blk;
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
                case P_NAME:
                    // Indicate name property set
                    if (pb_odesc && pb_odesc->type() == CDINSTANCE) {
                        CDp_cname *pn =
                            (CDp_cname*)OCALL(pb_odesc)->prpty(P_NAME);
                        if (pn && pn->assigned_name())
                            cx = &c3;
                    }
                    // fallthrough
                case P_RANGE:
                case P_MODEL:
                case P_VALUE:
                case P_PARAM:
                case P_OTHER:
                case P_NOPHYS:
                case P_FLATTEN:
                case P_SYMBLC:
                case P_DEVREF:
                    c = &c1;
                    break;
                default:
                    c = 0;
                    break;
                }
            }
            wb_textarea->setTextColor(*c);
            wb_textarea->insertPlainText(p->head());
            cnt += strlen(p->head());
            wb_textarea->setTextColor(*cx);
            wb_textarea->insertPlainText(p->string());
            cnt += strlen(p->string());
            wb_textarea->insertPlainText("\n");
            p->set_end(cnt);
            cnt++;
        }
    }
    if (vsb)
        vsb->setValue(val);
    pb_line_selected = -1;
}


// Select the chars in the range, start=end deselects existing.
//
void
QTprpBase::select_range(int start, int end)
{
    wb_textarea->select_range(start, end);
    pb_start = start;
    pb_end = end;
}


void
QTprpBase::handle_button_down(QMouseEvent *ev)
{
    pb_dragging = false;
    QByteArray qba = wb_textarea->toPlainText().toLatin1();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int x = ev->position().x();
    int y = ev->position().y();
#else
    int x = ev->x();
    int y = ev->y();
#endif
    QTextCursor cur = wb_textarea->cursorForPosition(QPoint(x, y));
    int pos = cur.position();
    const char *str = lstring::copy((const char*)qba.constData());
    const char *line_start = str;
    int linesel = 0;
    for (int i = 0; i <= pos; i++) {
        if (str[i] == '\n') {
            if (i == pos) {
                // Clicked to  right of line.
                delete [] str;
                return;
            }
            linesel++;
            line_start = str + i+1;
        }
    }
    if (line_start && *line_start != '\n') {
        PrptyText *p = pb_list;
        pos = line_start - str;
        for ( ; p; p = p->next()) {
            if (pos >= p->start() && pos < p->end())
                break;
        }
        if (p && pb_line_selected != linesel) {
            pb_line_selected = linesel;
            select_range(p->start() + strlen(p->head()), p->end());
            if (pb_btn_callback)
                (*pb_btn_callback)(p);
            delete [] str;
            pb_drag_x = x;
            pb_drag_y = y;
            pb_dragging = true;
            return;
        }
    }
    pb_line_selected = -1;
    delete [] str;
    select_range(0, 0);
}


void
QTprpBase::handle_button_up(QMouseEvent*)
{
    pb_dragging = false;
}


void
QTprpBase::handle_mouse_motion(QMouseEvent *ev)
{
    if (!pb_dragging)
        return;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    if (abs(ev->position().x() - pb_drag_x) < 5 &&
            abs(ev->position().y() - pb_drag_y) < 5)
#else
    if (abs(ev->x() - pb_drag_x) < 5 && abs(ev->y() - pb_drag_y) < 5)
#endif
        return;
    PrptyText *p = get_selection();
    if (!p)
        return;
    pb_dragging = false;

    int sz = 0;
    char *bf = 0;
    if (p->prpty()) {
        CDs *cursd =  CurCell(true);
        if (cursd) {
            hyList *hp = cursd->hyPrpList(pb_odesc, p->prpty());
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


bool
QTprpBase::is_mime_data_handled(const QMimeData *data) const
{
    if (data->hasFormat("text/property"))
        return (true);
    return (false);
}


bool
QTprpBase::is_mime_data_delivered(const QMimeData *data)
{
    if (data->hasFormat("text/property")) {
        if (!pb_odesc) {
            QTpkg::self()->RegisterTimeoutProc(3000, pb_bad_cb, this);
            PopUpMessage("Can't add property, no object selected.", false,
                false, false, GRloc(LW_LR));
        }
        else {
            QByteArray bary = data->data("text/property");
            int num = *(int*)bary.data();
            const char *val = (const char*)bary.data() + sizeof(int);
            bool accept = false;
            // Note: the window text is updated by call to PrptyRelist() in
            // CommitChangges()
            if (DSP()->CurMode() == Electrical) {
                if (pb_odesc->type() == CDINSTANCE) {
                    if (num == P_MODEL || num == P_VALUE || num == P_PARAM ||
                            num == P_OTHER || num == P_NOPHYS ||
                            num == P_FLATTEN || num == P_SYMBLC ||
                            num == P_RANGE || num == P_DEVREF) {
                        CDs *cursde = CurCell(Electrical, true);
                        if (cursde) {
                            Ulist()->ListCheck("addprp", cursde, false);
                            CDp *pdesc =
                                num != P_OTHER ? OCALL(pb_odesc)->prpty(num)
                                : 0;
                            hyList *hp = new hyList(cursde, (char*)val,
                                HYcvAscii);
                            ED()->prptyModify(OCALL(pb_odesc), pdesc, num,
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
                    DSP()->ShowOdescPhysProperties(pb_odesc, ERASE);

                    CDp *newp = new CDp((char*)val, num);
                    Ulist()->RecordPrptyChange(cursdp, pb_odesc, 0, newp);

                    Ulist()->CommitChanges(true);
                    DSP()->ShowOdescPhysProperties(pb_odesc, DISPLAY);
                    accept = true;
                }
            }
            if (!accept) {
                QTpkg::self()->RegisterTimeoutProc(3000, pb_bad_cb, this);
                PopUpMessage("Can't add property, incorrect type.", false,
                    false, false, GRloc(LW_LR));
            }
        }
        return (true);
    }
    return (false);
}


// Static function.
int
QTprpBase::pb_bad_cb(void *arg)
{
    QTprpBase *pb = (QTprpBase*)arg;
    if (pb && pb->wb_message)
        pb->wb_message->popdown();
    return (false);
}

