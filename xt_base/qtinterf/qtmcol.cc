
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
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qtmcol.h"
#include "qtmsg.h"
#include "qttextw.h"
#include "qtinput.h"
#include "qtfont.h"
#include "miscutil/filestat.h"

#include <QLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDrag>


// Dialog to display a list.  title is the title label text, callback
// is called with the word pointed to when the user points in the
// window, and 0 when the popup is destroyed.
//
// The list is column-formatted, and a text widget is used for the
// display.  If buttons is given, it is a 0-terminated list of
// auxiliary toggle button button names.
//
GRmcolPopup *
QTbag::PopUpMultiCol(stringlist *symlist, const char *title,
    void (*callback)(const char*, void*), void *arg,
    const char **buttons, int pgsize, bool no_dd)
{
    static int mcol_count;

    QTmcolDlg *mcol = new QTmcolDlg(this, symlist, title, buttons, pgsize);
    if (wb_shell)
       mcol->set_transient_for(wb_shell);
    mcol->register_callback(callback);
    mcol->set_callback_arg(arg);
    mcol->set_no_dragdrop(no_dd);

    int x, y;
    QTdev::self()->ComputePopupLocation(GRloc(), mcol->Shell(), wb_shell,
        &x, &y);
    x += mcol_count*50 - 150;
    y += mcol_count*50 - 150;
    mcol_count++;
    if (mcol_count == 6)
        mcol_count = 0;
    mcol->move(x, y);

    mcol->set_visible(true);
    return (mcol);
}


QTmcolDlg::QTmcolDlg(QTbag *owner, stringlist *symlist,
    const char *title, const char **buttons, int pgsize) : QTbag(this)
{
    p_parent = owner;
    mc_pagesel = 0;
    for (int i = 0; i < MC_MAXBTNS; i++)
        mc_buttons[i] = 0;
    mc_save_pop = 0;
    mc_msg_pop = 0;
    mc_label = 0;
    mc_strings = stringlist::dup(symlist);
    stringlist::sort(mc_strings);

    mc_alloc_width = 0;
    mc_drag_x = mc_drag_y = 0;
    mc_page = 0;

    mc_pagesize = pgsize;
    if (mc_pagesize <= 0)
        mc_pagesize = DEF_LIST_MAX_PER_PAGE;
    else if (mc_pagesize < 100)
        mc_pagesize = 100;
    else if (mc_pagesize > 50000)
        mc_pagesize = 50000;

    mc_btnmask = 0;
    mc_start = 0;
    mc_end = 0;
    mc_dragging = false;

    if (owner)
        owner->MonitorAdd(this);

    setWindowTitle(tr("Listing"));
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(2, 2, 2, 2);
    vbox->setSpacing(2);

    // Title label.
    //
    mc_label = new QLabel(tr(title));
    vbox->addWidget(mc_label);

    // Scrolled text area.
    //
    wb_textarea = new QTtextEdit();
    vbox->addWidget(wb_textarea);
    wb_textarea->setReadOnly(true);
    wb_textarea->setMouseTracking(true);
    wb_textarea->setAcceptDrops(false);
    connect(wb_textarea, SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));
    connect(wb_textarea, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));
    connect(wb_textarea, SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(mouse_motion_slot(QMouseEvent*)));

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_FIXED))
        wb_textarea->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(2);

    QPushButton *btn = new QPushButton(tr("Save Text "));
    hbox->addWidget(btn);
    btn->setCheckable(true);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(toggled(bool)), this, SLOT(save_btn_slot(bool)));

    mc_pagesel = new QComboBox();
    hbox->addWidget(mc_pagesel);

    // Dismiss button.
    //
    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    if (buttons) {
        for (int i = 0; i < MC_MAXBTNS && buttons[i]; i++) {
            btn = new QPushButton(buttons[i]);
            btn->setAutoDefault(false);
            btn->setEnabled(false);
            mc_buttons[i] = btn;
            hbox->addWidget(btn);
            connect(btn, SIGNAL(clicked()), this, SLOT(user_btn_slot()));
        }
    }

    relist();
}


QTmcolDlg::~QTmcolDlg()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }
    if (p_callback)
        (*p_callback)(0, p_cb_arg);
    if (mc_save_pop)
        mc_save_pop->popdown();
    if (mc_msg_pop)
        mc_msg_pop->popdown();
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        QTdev::Deselect(p_caller);
    stringlist::destroy(mc_strings);
}


// GRpopup override
//
void
QTmcolDlg::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    deleteLater();
}


// GRmcolPopup override
//
void
QTmcolDlg::update(stringlist *symlist, const char *title)
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }

    stringlist::destroy(mc_strings);
    mc_strings = stringlist::dup(symlist);
    stringlist::sort(mc_strings);

    mc_page = 0;
    relist();

    if (title)
        mc_label->setText(tr(title));
}


// GRmcolPopup override
//
// Return the selected text, null if no selection.
//
char *
QTmcolDlg::get_selection()
{
    if (mc_end != mc_start)
        return (wb_textarea->get_chars(mc_start, mc_end));
    return (0);
}


// GRmcolPopup override
//
// Set sensitivity of optional buttons.
// Bit == 0:  button always insensitive
// Bit == 1:  button sensitive when selection.
//
void
QTmcolDlg::set_button_sens(int msk)
{
    int bm = 1;
    mc_btnmask = ~msk;
    bool has_sel = (mc_end != mc_start);
    for (int i = 0; i < MC_MAXBTNS && mc_buttons[i]; i++) {
        mc_buttons[i]->setEnabled((bm & msk) && has_sel);
        bm <<= 1;
    }
}


// Handle the relisting, the display is paged.
//
void
QTmcolDlg::relist()
{
    int min = mc_page * mc_pagesize;
    int max = min + mc_pagesize;

    stringlist *s0 = 0, *se = 0;
    int cnt = 0;
    for (stringlist *s = mc_strings; s; s = s->next) {
        if (cnt >= min && cnt < max) {
            if (!s0)
                se = s0 = new stringlist(lstring::copy(s->string), 0);
            else {
                se->next = new stringlist(lstring::copy(s->string), 0);
                se = se->next;
            }
        }
        cnt++;
    }

    if (cnt <= mc_pagesize)
        mc_pagesel->hide();
    else {
        char buf[128];
        mc_pagesel->clear();

        for (int i = 0; i*mc_pagesize < cnt; i++) {
            int tmpmax = (i+1)*mc_pagesize;
            if (tmpmax > cnt)
                tmpmax = cnt;
            snprintf(buf, sizeof(buf), "%d - %d", i*mc_pagesize + 1, tmpmax);
            mc_pagesel->addItem(buf);
        }
        mc_pagesel->setCurrentIndex(mc_page);
        connect(mc_pagesel, SIGNAL(currentIndex(int)),
            this, SLOT(page_size_slot(int)));
        mc_pagesel->show();
    }

    int tw = wb_textarea->width();
    int cols = (tw - 4)/QTfont::stringWidth(0, wb_textarea);
    char *s = stringlist::col_format(s0, cols);
    stringlist::destroy(s0);
    wb_textarea->setPlainText(s);
    delete [] s;
}


// Select the chars in the range, start=end deselects existing.
//
void
QTmcolDlg::select_range(int start, int end)
{
    wb_textarea->select_range(start, end);
    mc_start = start;
    mc_end = end;
    if (start == end) {
        for (int i = 0; i < MC_MAXBTNS && mc_buttons[i]; i++)
            mc_buttons[i]->setEnabled(false);
    }
    else {
        int bm = 1;
        for (int i = 0; i < MC_MAXBTNS && mc_buttons[i]; i++) {
            mc_buttons[i]->setEnabled(bm & ~mc_btnmask);
            bm <<= 1;
        }
    }
}


// Private static handler.
// Callback for the save file name pop-up.
//
ESret
QTmcolDlg::mc_save_cb(const char *string, void *arg)
{
    QTmcolDlg *mcp = (QTmcolDlg*)arg;
    if (string) {
        if (!filestat::create_bak(string)) {
            mcp->mc_save_pop->update(
                "Error backing up existing file, try again", 0);
            return (ESTR_IGN);
        }
        FILE *fp = fopen(string, "w");
        if (!fp) {
            mcp->mc_save_pop->update("Error opening file, try again", 0);
            return (ESTR_IGN);
        }
        QByteArray txt_ba = mcp->wb_textarea->toPlainText().toLatin1();
        const char *txt = txt_ba.constData();
        if (txt) {
            unsigned int len = strlen(txt);
            if (len) {
                if (fwrite(txt, 1, len, fp) != len) {
                    mcp->mc_save_pop->update("Write failed, try again", 0);
                    delete [] txt;
                    fclose(fp);
                    return (ESTR_IGN);
                }
            }
            delete [] txt;
        }
        fclose(fp);

        if (mcp->mc_msg_pop)
            mcp->mc_msg_pop->popdown();
        mcp->mc_msg_pop = new QTmsgDlg(0, "Text saved in file.");
        mcp->mc_msg_pop->register_usrptr((void**)&mcp->mc_msg_pop);
        QTdev::self()->SetPopupLocation(GRloc(), mcp->mc_msg_pop,
            mcp->wb_shell);
        mcp->mc_msg_pop->set_visible(true);
        QTdev::self()->AddTimer(2000, mc_timeout, mcp);
    }
    return (ESTR_DN);
}


int
QTmcolDlg::mc_timeout(void *arg)
{
    QTmcolDlg *mcp = (QTmcolDlg*)arg;
    if (mcp->mc_msg_pop)
        mcp->mc_msg_pop->popdown();
    return (0);
}


void
QTmcolDlg::save_btn_slot(bool state)
{
    if (!state)
        return;
    if (mc_save_pop)
        return;
    mc_save_pop = new QTledDlg(0,
        "Enter path to file for saved text:", "", "Save", false);
    mc_save_pop->register_caller(sender(), false, true);
    mc_save_pop->set_callback_arg(this);
    mc_save_pop->register_callback(
        (GRledPopup::GRledCallback)&mc_save_cb);
    mc_save_pop->register_usrptr((void**)&mc_save_pop);

    QTdev::self()->SetPopupLocation(GRloc(), mc_save_pop, this);
    mc_save_pop->set_visible(true);
}


void
QTmcolDlg::dismiss_btn_slot()
{
    popdown();
}


void
QTmcolDlg::user_btn_slot()
{
    // Handle the auxiliary buttons:  call the callback with '/'
    // followed by button text.

    QPushButton *b = qobject_cast<QPushButton*>(sender());
    if (b) {
        sLstr lstr;
        lstr.add_c('/');
        lstr.add(b->text().toLatin1().constData());
        (*p_callback)(lstr.string(), p_cb_arg);
    }
}


void
QTmcolDlg::page_size_slot(int i)
{
    mc_page = i;
    relist();
}


void
QTmcolDlg::resize_slot(QResizeEvent *ev)
{
    // Reformat the listing when window size changes.
    int wid = ev->size().width();
    if (wb_textarea && mc_strings && wid > 0 && wid != mc_alloc_width) {
        mc_alloc_width = wid;
        relist();
    }
}


void
QTmcolDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonRelease) {
        if (ev->button() == Qt::LeftButton) {
            mc_dragging = false;
            ev->accept();
        }
        else
            ev->ignore();
        return;
    }
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    if (ev->button() != Qt::LeftButton) {
        ev->ignore();
        return;
    }
    ev->accept();

    const char *str = lstring::copy(
        wb_textarea->toPlainText().toLatin1().constData());
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();;
#else
    int xx = ev->x();
    int yy = ev->y();;
#endif
    QTextCursor cur = wb_textarea->cursorForPosition(QPoint(xx, yy));
    int posn = cur.position();

    select_range(0, 0);

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
    int start = posn - (lineptr - str);;
    int end = start;
    while (start > 0 && !isspace(lineptr[start]))
        start--;
    if (isspace(lineptr[start]))
        start++;
    while (lineptr[end] && !isspace(lineptr[end]))
        end++;
    // The top level cells are listed with an '*'.
    // Modified cells are listed  with a '+'.
    if (lineptr[start] == '+' || lineptr[start] == '*')
        start++;
    if (start == end)
        return;

    char *tbuf = new char[end - start + 1];
    char *t = tbuf;
    for (int i = start; i < end; i++)
        *t++ = lineptr[i];
    *t = 0;

    delete [] str;
    start += (lineptr - str);
    end += (lineptr - str);
    select_range(start, end);

    if (p_callback)
        (*p_callback)(tbuf, p_cb_arg);
    delete [] tbuf;

    mc_drag_x = xx;
    mc_drag_y = yy;
    mc_dragging = true;
}


void
QTmcolDlg::mouse_motion_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseMove) {
        ev->ignore();
        return;
    }
    ev->accept();

    // Start the drag, after a selection, if the pointer moves.
    if (p_no_dd)
        return;
    if (!mc_dragging)
        return;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    if (abs(ev->position().x() - mc_drag_x) < 5 &&
            abs(ev->position().y() - mc_drag_y) < 5)
#else
    if (abs(ev->x() - mc_drag_x) < 5 && abs(ev->y() - mc_drag_y) < 5)
#endif
        return;
    if (!wb_textarea->has_selection())
        return;
    mc_dragging = false;

    // The payload is a string which has two substrings separated by
    // a newline.  We call this "text/twostring" locally.

    QByteArray label_ba = mc_label->text().toLatin1();
    const char *ltext = label_ba.constData();
    const char *t = strchr(ltext, '\n');
    if (t)
        ltext = t+1;
    sLstr lstr;
    lstr.add(ltext);
    lstr.add_c('\n');
    const char *sel = wb_textarea->get_selection();
    lstr.add(sel);
    delete [] sel;

    QDrag *drag = new QDrag(wb_textarea);
    QMimeData *mimedata = new QMimeData();
    QByteArray qdata(lstr.string(), strlen(lstr.string())+1);
    mimedata->setData("text/twostring", qdata);
    delete [] sel;
    drag->setMimeData(mimedata);
    drag->exec(Qt::CopyAction);
}


void
QTmcolDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (FC.getFont(&fnt, fnum)) {
            wb_textarea->setFont(*fnt);
            relist();
        }
    }
}

