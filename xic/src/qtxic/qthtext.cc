
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

#include "qthtext.h"
#include "qtinterf/qtfont.h"
#include "cd_property.h"
#include "dsp_inlines.h"
#include "select.h"
#include "menu.h"
#include "events.h"
#include "keymap.h"
#include "sced.h"

#include <QApplication>
#include <QClipboard>
#include <QLayout>
#include <QLabel>
#include <QToolButton>
#include <QMenu>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>


// QTedit:  The QT-specific prompt line functions, used in the main
// window.
//
// Help keywords:
//  promptline

hyList *QTedit::pe_stores[PE_NUMSTORES];

QTedit *QTedit::instancePtr = 0;

QTedit::QTedit(bool nogr) : QTdraw(XW_TEXT)
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class QTedit already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    pe_disabled = nogr;

    pe_firstinsert = false;
    pe_indicating = false;
    pe_rcl_btn = 0;
    pe_sto_btn = 0;
    pe_ltx_btn = 0;
    if (nogr)
        return;

    QMargins qm;
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    pe_keys = new cKeys(0, 0);
    pe_keys->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    hbox->addWidget(pe_keys);

    // Recall button and menu.
    pe_rcl_btn = new QToolButton();
    pe_rcl_btn->setText("R");
    pe_rcl_btn->setToolTip(tr("Recall edit string from a register."));
    hbox->addWidget(pe_rcl_btn);

    // Prevent height change of row when buttons come ang go, to avoid main
    // window resize/redraw.
    hbox->addStrut(pe_rcl_btn->sizeHint().height() + 2);

    char buf[4];
    QMenu *rcl_menu = new QMenu();
    for (int i = 0; i < PE_NUMSTORES; i++) {
        buf[0] = '0' + i;
        buf[1] = 0;
        rcl_menu->addAction(buf);
    }
    pe_rcl_btn->setMenu(rcl_menu);
    pe_rcl_btn->setPopupMode(QToolButton::InstantPopup);
    pe_rcl_btn->hide();
    connect(rcl_menu, SIGNAL(triggered(QAction*)),
        this, SLOT(recall_menu_slot(QAction*)));

    // Store button and menu.
    pe_sto_btn = new QToolButton();
    pe_sto_btn->setText("S");
    pe_sto_btn->setToolTip(tr("Save edit string to a register."));
    hbox->addWidget(pe_sto_btn);

    QMenu *sto_menu = new QMenu();
    for (int i = 0; i < PE_NUMSTORES; i++) {
        buf[0] = '0' + i;
        buf[1] = 0;
        sto_menu->addAction(buf);
    }
    pe_sto_btn->setMenu(sto_menu);
    pe_sto_btn->setPopupMode(QToolButton::InstantPopup);
    pe_sto_btn->hide();
    connect(sto_menu, SIGNAL(triggered(QAction*)),
        this, SLOT(store_menu_slot(QAction*)));

    // Long Text button.
    pe_ltx_btn = new QToolButton();
    pe_ltx_btn->setText("L");
    pe_ltx_btn->setToolTip(tr(
        "Associate a block of text with the label - pop up an editor."));
    hbox->addWidget(pe_ltx_btn);
    connect(pe_ltx_btn, SIGNAL(clicked()),
        this, SLOT(long_text_slot()));

    gd_viewport = new QTcanvas();
    hbox->addWidget(Viewport());
    Viewport()->setAcceptDrops(true);

    QFont *tfont;
    if (FC.getFont(&tfont, FNT_SCREEN)) {
        gd_viewport->set_font(tfont);
        pe_rcl_btn->setFont(*tfont);
        pe_sto_btn->setFont(*tfont);
        pe_ltx_btn->setFont(*tfont);
    }

    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)));

    connect(Viewport(), SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));
    connect(Viewport(), SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(press_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(release_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(enter_event(QEnterEvent*)),
        this, SLOT(enter_slot(QEnterEvent*)));
    connect(Viewport(), SIGNAL(leave_event(QEvent*)),
        this, SLOT(leave_slot(QEvent*)));
    connect(Viewport(), SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(motion_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(drag_enter_event(QDragEnterEvent*)),
        this, SLOT(drag_enter_slot(QDragEnterEvent*)));
    connect(Viewport(), SIGNAL(drop_event(QDropEvent*)),
        this, SLOT(drop_slot(QDropEvent*)));

    connect(pe_keys, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(keys_press_slot(QMouseEvent*)));
}


QTedit::~QTedit()
{
    instancePtr = 0;
}


// Private static error exit.
//
void
QTedit::on_null_ptr()
{
    fprintf(stderr, "Singleton class QTedit used before instantiated.\n");
    exit(1);
}


namespace {
    class QTflashPop : public QDialog
    {
    public:
        QTflashPop(const char *msg, QWidget *prnt = 0) : QDialog(prnt)
        {
            setAttribute(Qt::WA_DeleteOnClose);
            setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
            QVBoxLayout *vbox = new QVBoxLayout(this);
            vbox->setContentsMargins(0, 0, 0, 0);
            vbox->setSpacing(2);
            QLabel *label = new QLabel(msg);
            vbox->addWidget(label);

        }       

        static int timeout(void *arg)
        {
            QTflashPop *fp = static_cast<QTflashPop*>(arg);
            delete fp;
            return (0);
        }
    };

}


// Flash a message just above the prompt line for a couple of seconds.
//
void
QTedit::flash_msg(const char *msg, ...)
{
    va_list args;

    char buf[256];
    va_start(args, msg);
    vsnprintf(buf, 256, msg, args);
    va_end(args);
    puts(buf);

    QTflashPop *pop = new QTflashPop(buf, QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_LL), pop,
        QTmainwin::self()->Viewport());
    pop->show();
    QTdev::self()->AddTimer(2000, &QTflashPop::timeout, pop);
}


// As above, but user passes the location.
//
void
QTedit::flash_msg_here(int xx, int yy, const char *msg, ...)
{
    va_list args;

    char buf[256];
    va_start(args, msg);
    vsnprintf(buf, 256, msg, args);
    va_end(args);
    puts(buf);

    QTflashPop *pop = new QTflashPop(buf, QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_XYA, xx, yy),
        pop, QTmainwin::self()->Viewport());
    pop->show();
    QTdev::self()->AddTimer(2000, &QTflashPop::timeout, pop);
}


// Save text in register 0, called when editing finished.
//
void
QTedit::save_line()
{
    hyList::destroy(pe_stores[0]);
    pe_stores[0] = get_hyList(false);
}


// Return the pixel width of the drawing area.
//
int
QTedit::win_width(bool)
{
    return (gd_viewport->width());
}


int
QTedit::win_height()
{
    return (gd_viewport->height());
}


// Set the keyboard focus to the main drawing window.
//
void
QTedit::set_focus()
{
    QTmainwin::self()->activateWindow();
}


// Display the R/S/L buttons, hide the keys area while editing.
//
void
QTedit::set_indicate()
{
    if (pe_indicating) {
        pe_keys->hide();
        pe_rcl_btn->show();
        pe_sto_btn->show();
        if (pe_long_text_mode)
            pe_ltx_btn->show();
    }
    else {
        pe_keys->show();
        pe_rcl_btn->hide();
        pe_sto_btn->hide();
        pe_ltx_btn->hide();
    }
}


void
QTedit::show_lt_button(bool show_btn)
{
    if (!pe_disabled) {
        if (show_btn)
            pe_ltx_btn->show();
        else
            pe_ltx_btn->hide();
    }
}


void
QTedit::get_selection(bool pri)
{
    if (pri) {
        // Insert the clipboard.
        QByteArray ba = QApplication::clipboard()->text().toLatin1();
        const char *sel = ba.constData();
        if (sel && *sel && is_active()) {
            CDs *cursd = CurCell(true);
            hyList *hp = new hyList(cursd, sel, HYcvAscii);
            insert(hp);
            hyList::destroy(hp);
        }
    }
    else {
        // Insert the "primary" selection.
        // X only
    }
}


void *
QTedit::setup_backing(bool)
{
    return (0);
}


void
QTedit::restore_backing(void*)
{
}


void
QTedit::init_window()
{
    SetWindowBackground(bg_pixel());
    SetBackground(bg_pixel());
    Clear();
}


bool
QTedit::check_pixmap()
{
    return (true);
}


void
QTedit::init_selection(bool selected)
{
    if (selected) {
        // Copy highlighted text to clipboard.
        char *str = get_sel();
        if (str) {
            QApplication::clipboard()->setText(str);
            delete [] str;
        }
    }
}


void
QTedit::warp_pointer()
{
    // The pointer move must be in an idle proc, so it runs after
    // prompt line reconfiguration.

//    g_idle_add(warp_ptr, this);
}


void
QTedit::font_changed_slot(int fnum)
{
    if (fnum == FNT_SCREEN) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_SCREEN)) {
            gd_viewport->set_font(fnt);
            init();
            redraw();
        }
    }
}


void
QTedit::recall_menu_slot(QAction *a)
{
    const char *name = (const char*)a->text().constData();
    if (!name)
        return;
    while (*name && !isdigit(*name))
        name++;
    if (!isdigit(*name))
        return;
    int ix = atoi(name);
    if (ix >= PE_NUMSTORES)
        return;

    QTmainwin::self()->Viewport()->setFocus();

    for (hyList *hl = pe_stores[ix]; hl; hl = hl->next()) {
        if (hl->ref_type() == HLrefText || hl->ref_type() == HLrefLongText ||
                hl->ref_type() == HLrefEnd)
            continue;
        const hyEnt *ent = hl->hent();
        if (!ent) {
            flash_msg("Can't recall, mising element.");
            return;
        }
        if (!ent->owner()) {
            flash_msg("Can't recall, bad address.");
            return;
        }
        if (ent->owner() != CurCell(true)) {
            flash_msg("Can't recall, incompatible reference.");
            return;
        }
    }

    clear_cols_to_end(pe_colmin);
    insert(pe_stores[ix]);
}
 

void
QTedit::store_menu_slot(QAction *a)
{
    const char *name = (const char*)a->text().constData();
    if (!name)
        return;
    while (*name && !isdigit(*name))
        name++;
    if (!isdigit(*name))
        return;
    int ix = atoi(name);
    if (ix < 1 || ix >= PE_NUMSTORES)
        return;
    hyList::destroy(pe_stores[ix]);
    pe_stores[ix] = get_hyList(false);

    flash_msg("Edit line saved in register %d.", ix);

    QTmainwin::self()->Viewport()->setFocus();
}
 

void
QTedit::long_text_slot()
{
    lt_btn_press_handler();
}
 

void
QTedit::resize_slot(QResizeEvent*)
{
    redraw();
}


void
QTedit::press_slot(QMouseEvent *ev)
{
    // Pop up info about the prompt/reply area if the user points
    // there in help mode.  Otherwise, handle button presses, in
    // particular selection insertions using button 2.

    if (ev->type() == QEvent::MouseButtonDblClick) {
        // Double-clicking in the prompt area with button 1 sends
        // Enter.

        if (ev->button() == Qt::LeftButton)
            finish(false);
        ev->accept();
        return;
    }
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    if (ev->button() == Qt::LeftButton)
        button_press_handler(1, ev->position().x(), ev->position().y());
    else if (ev->button() == Qt::MiddleButton)
        button_press_handler(2, ev->position().x(), ev->position().y());
    else if (ev->button() == Qt::RightButton)
        button_press_handler(3, ev->position().x(), ev->position().y());
#else
    if (ev->button() == Qt::LeftButton)
        button_press_handler(1, ev->x(), ev->y());
    else if (ev->button() == Qt::MiddleButton)
        button_press_handler(2, ev->x(), ev->y());
    else if (ev->button() == Qt::RightButton)
        button_press_handler(3, ev->x(), ev->y());
#endif
    ev->accept();
}


void
QTedit::release_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonRelease) {
        ev->ignore();
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    if (ev->button() == Qt::LeftButton)
        button_release_handler(1, ev->position().x(), ev->position().y());
    else if (ev->button() == Qt::MiddleButton)
        button_release_handler(2, ev->position().x(), ev->position().y());
    else if (ev->button() == Qt::RightButton)
        button_release_handler(3, ev->position().x(), ev->position().y());
#else
    if (ev->button() == Qt::LeftButton)
        button_release_handler(1, ev->x(), ev->y());
    else if (ev->button() == Qt::MiddleButton)
        button_release_handler(2, ev->x(), ev->y());
    else if (ev->button() == Qt::RightButton)
        button_release_handler(3, ev->x(), ev->y());
#endif
    ev->accept();
}


void
QTedit::enter_slot(QEnterEvent*)
{
    if (QTmainwin::self()->Viewport()->hasFocus()) {
        pe_entered = true;
        redraw();
    }
}


void
QTedit::leave_slot(QEvent*)
{
    if (QTmainwin::self()->Viewport()->hasFocus()) {
        pe_entered = false;
        redraw();
    }
}


void
QTedit::motion_slot(QMouseEvent *ev)
{
    if (pe_has_drag)
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        pointer_motion_handler(ev->position().x(), ev->position().y());
#else
        pointer_motion_handler(ev->x(), ev->y());
#endif
}


namespace {
    struct load_file_data
    {
        load_file_data(const char *f, const char *c)
            {
                filename = lstring::copy(f);
                cellname = lstring::copy(c);
            }

        ~load_file_data()
            {
                delete [] filename;
                delete [] cellname;
            }

        char *filename;
        char *cellname;
    };

    // Idle procedure to load a file, called from the drop handler.
    //
    int load_file_idle(void *arg)
    {
        load_file_data *data = (load_file_data*)arg;
        XM()->Load(EV()->CurrentWin(), data->filename, 0, data->cellname);
        delete data;
        return (0);
    }

    void load_file_proc(const char *fmt, const char *s)
    {
        char *src = lstring::copy(s);
        char *t = 0;
        if (!strcmp(fmt, "text/twostring")) {
            // Drops from content lists may be in the form
            // "fname_or_chd\ncellname".
            t = strchr(src, '\n');
            if (t)
                *t++ = 0;
        }
        load_file_data *lfd = new load_file_data(src, t);
        delete [] src;

        bool didit = false;
        if (QTedit::self() && QTedit::self()->is_active()) {
            if (ScedIf()->doingPlot()) {
                // Plot/Iplot edit line is active, break out.
                QTedit::self()->abort();
            }
            else {
                // If editing, push into prompt line.
                // Keep the cellname.
                if (lfd->cellname)
                    QTedit::self()->insert(lfd->cellname);
                else
                    QTedit::self()->insert(lfd->filename);
                delete lfd;
                didit = true;
            }
        }
        if (!didit)
            QTpkg::self()->RegisterIdleProc(load_file_idle, lfd);
    }
}


void
QTedit::drag_enter_slot(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasUrls() ||
            ev->mimeData()->hasFormat("text/property") ||
            ev->mimeData()->hasFormat("text/twostring") ||
            ev->mimeData()->hasFormat("text/cellname") ||
            ev->mimeData()->hasFormat("text/string") ||
            ev->mimeData()->hasFormat("text/plain")) {
        ev->accept();
    }
    ev->ignore();
}


void
QTedit::drop_slot(QDropEvent *ev)
{
    if (ev->mimeData()->hasUrls()) {
        QByteArray ba = ev->mimeData()->data("text/plain");
        const char *str = ba.constData() + strlen("File://");
        load_file_proc("", str);
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/property")) {
        if (is_active()) {
            QByteArray bary = ev->mimeData()->data("text/property");
            const char *val = (const char*)bary.data() + sizeof(int);
            CDs *cursd = CurCell(true);
            hyList *hp = new hyList(cursd, val, HYcvAscii);
            insert(hp);
            hyList::destroy(hp);
        }
        ev->accept();
        return;
    }
    const char *fmt = 0;
    if (ev->mimeData()->hasFormat("text/twostring"))
        fmt = "text/twostring";
    else if (ev->mimeData()->hasFormat("text/cellname"))
        fmt = "text/cellname";
    else if (ev->mimeData()->hasFormat("text/string"))
        fmt = "text/string";
    else if (ev->mimeData()->hasFormat("text/plain"))
        fmt = "text/plain";
    if (fmt) {
        QByteArray bary = ev->mimeData()->data(fmt);
        load_file_proc(fmt, bary.constData());
        ev->accept();
        return;
    }
    ev->ignore();
}


void
QTedit::keys_press_slot(QMouseEvent *ev)
{
    // Pop up info about the keys pressed area in help mode.

    if (XM()->IsDoingHelp() && ev->button() == Qt::LeftButton &&
            !(ev->modifiers() & Qt::ShiftModifier))
        DSPmainWbag(PopUpHelp("keyspresd"))
}

