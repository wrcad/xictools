
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
#include <QLayout>
#include <QPushButton>
#include <QMenu>
#include <QMouseEvent>

// Help keywords:
//  promptline
//

/*
static GtkTargetEntry pl_targets[] = {
  { "STRING",     0, 0 },
  { "text/plain", 0, 1 },
  { "CELLNAME",   0, 2 },
  { "property",   0, 3 }
};
static guint n_pl_targets = 4;

*/


//-------------------------------

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
    if (nogr)
        return;

    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    hbox->setSpacing(2);

    pe_keys = new cKeys(0, 0);
    pe_keys->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    hbox->addWidget(pe_keys);

    int w = QTfont::stringWidth(0, FNT_SCREEN);

    // Recall button and menu.
    pe_rcl_btn = new QPushButton("R");
    pe_rcl_btn->setAutoDefault(false);
    pe_rcl_btn->setMaximumWidth(7*w + 4);
    pe_rcl_btn->setToolTip(tr("Recall edit string from a register."));
    hbox->addWidget(pe_rcl_btn);

    char buf[4];
    QMenu *rcl_menu = new QMenu();
    for (int i = 0; i < PE_NUMSTORES; i++) {
        buf[0] = '0' + i;
        buf[1] = 0;
        rcl_menu->addAction(buf);
    }
    pe_rcl_btn->setMenu(rcl_menu);
    pe_rcl_btn->hide();
    connect(rcl_menu, SIGNAL(triggered(QAction*)),
        this, SLOT(recall_menu_slot(QAction*)));

    // Store button and menu.
    pe_sto_btn = new QPushButton("S");
    pe_sto_btn->setAutoDefault(false);
    pe_sto_btn->setMaximumWidth(7*w + 4);
    pe_sto_btn->setToolTip(tr("Save edit string to a register."));
    hbox->addWidget(pe_sto_btn);

    QMenu *sto_menu = new QMenu();
    for (int i = 0; i < PE_NUMSTORES; i++) {
        buf[0] = '0' + i;
        buf[1] = 0;
        sto_menu->addAction(buf);
    }
    pe_sto_btn->setMenu(sto_menu);
    pe_sto_btn->hide();
    connect(sto_menu, SIGNAL(triggered(QAction*)),
        this, SLOT(store_menu_slot(QAction*)));

    // Long Text button.
    pe_ltx_btn = new QPushButton("L");
    pe_ltx_btn->setAutoDefault(false);
    pe_ltx_btn->setMaximumWidth(6*w + 4);
    pe_ltx_btn->setToolTip(tr(
        "Associate a block of text with the label - pop up an editor."));
    hbox->addWidget(pe_ltx_btn);
    connect(pe_ltx_btn, SIGNAL(clicked()),
        this, SLOT(long_text_slot()));

    gd_viewport = draw_if::new_draw_interface(DrawNative, false, this);
    hbox->addWidget(Viewport());

    QFont *font;
    if (FC.getFont(&font, FNT_SCREEN)) {
        gd_viewport->set_font(font);
        pe_rcl_btn->setFont(*font);
        pe_sto_btn->setFont(*font);
        pe_ltx_btn->setFont(*font);
    }

    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed(int)), Qt::QueuedConnection);

    connect(Viewport(), SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)), Qt::QueuedConnection);
    connect(Viewport(), SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(press_slot(QMouseEvent*)), Qt::QueuedConnection);
    connect(Viewport(), SIGNAL(enter_event(QEvent*)),
        this, SLOT(enter_slot(QEvent*)), Qt::QueuedConnection);
    connect(Viewport(), SIGNAL(leave_event(QEvent*)),
        this, SLOT(leave_slot(QEvent*)), Qt::QueuedConnection);
    connect(Viewport(), SIGNAL(drag_enter_event(QDragEnterEvent*)),
        this, SLOT(drag_enter_slot(QDragEnterEvent*)), Qt::QueuedConnection);
    connect(Viewport(), SIGNAL(drop_event(QDropEvent*)),
        this, SLOT(drop_slot(QDropEvent*)), Qt::QueuedConnection);

    connect(pe_keys, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(keys_press_slot(QMouseEvent*)), Qt::QueuedConnection);
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

    /*XXX
    GtkWidget *popup = gtk_window_new(GTK_WINDOW_POPUP);
    if (!popup)
        return;
    GtkWidget *label = gtk_label_new(buf);
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_container_add(GTK_CONTAINER(popup), label);

    QTdev::self()->SetPopupLocation(GRloc(LW_LL), popup,
        QTmainwin::self()->Viewport());
    gtk_window_set_transient_for(GTK_WINDOW(popup),
        GTK_WINDOW(QTmainwin::self()->Shell()));

    gtk_widget_show(popup);

    QTdev::self()->AddTimer(2000, fm_timeout, popup);
    */
}


// As above, but user passes the location.
//
void
QTedit::flash_msg_here(int x, int y, const char *msg, ...)
{
    (void)x;
    (void)y;
    va_list args;

    char buf[256];
    va_start(args, msg);
    vsnprintf(buf, 256, msg, args);
    va_end(args);
    puts(buf);

    /*XXX
    GtkWidget *popup = gtk_window_new(GTK_WINDOW_POPUP);
    if (!popup)
        return;
    GtkWidget *label = gtk_label_new(buf);
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_container_add(GTK_CONTAINER(popup), label);

    int mwid, mhei;
    gtk_MonitorGeom(QTmainwin::self()->Shell(), 0, 0, &mwid, &mhei);
    GtkRequisition req;
    gtk_widget_get_requisition(popup, &req);
    if (x + req.width > mwid)
        x = mwid - req.width;
    if (y + req.height > mhei)
        y = mhei - req.height;
    gtk_window_move(GTK_WINDOW(popup), x, y);
    gtk_window_set_transient_for(GTK_WINDOW(popup),
        GTK_WINDOW(QTmainwin::self()->Shell()));

    gtk_widget_show(popup);

    QTdev::self()->AddTimer(2000, fm_timeout, popup);
    */
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
    return (gd_viewport->widget()->width());
}


int
QTedit::win_height()
{
//    if (!QTdev::exists() || !QTmainwin::exists())
        return (14);
//    return (pe_hei);
}


// Set the keyboard focus to the main drawing window.
//
void
QTedit::set_focus()
{
//    QTdev::self()->RevertFocus();
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
QTedit::show_lt_button(bool show)
{
    if (!pe_disabled) {
        if (show)
            pe_ltx_btn->show();
        else
            pe_ltx_btn->hide();
    }
}


void
QTedit::get_selection(bool)
{
}


void *
QTedit::setup_backing(bool use_pm)
{
    (void)use_pm;
    return (0);
}


void
QTedit::restore_backing(void *tw)
{
    (void)tw;
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
QTedit::init_selection(bool)
{
}


void
QTedit::warp_pointer()
{
    // The pointer move must be in an idle proc, so it runs after
    // prompt line reconfiguration.

//    g_idle_add(warp_ptr, this);
}


void
QTedit::font_changed(int fnum)
{
    if (fnum == FNT_SCREEN) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_SCREEN)) {
            gd_viewport->set_font(fnt);
        }
        //XXX redraw
        init();
    }
    /*XXX
    if (ptr()) {
#if GTK_CHECK_VERSION(3,0,0)
        //XXX probably need something here.
#else
        if (ptr()->gd_viewport) {
            int fw, fh;
            ptr()->TextExtent(0, &fw, &fh);
            GtkRequisition r;
            gtk_widget_size_request(ptr()->pe_r_button, &r);
            int ht = fh + 4;
            if (ht < r.height)
                ht = r.height;
            gtk_widget_set_size_request(ptr()->gd_viewport, -1, ht);
        }
#endif
        ptr()->init();
    }
    */
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

    if (ev->type() == QEvent::MouseButtonPress) {
        if (ev->button() == Qt::LeftButton)
            button_press_handler(1, ev->x(), ev->y());
        else if (ev->button() == Qt::MiddleButton)
            button_press_handler(2, ev->x(), ev->y());
        else if (ev->button() == Qt::RightButton)
            button_press_handler(3, ev->x(), ev->y());
    }
    else if (ev->type() == QEvent::MouseButtonRelease) {
        if (ev->button() == Qt::LeftButton)
            button_release_handler(1, ev->x(), ev->y());
        else if (ev->button() == Qt::MiddleButton)
            button_release_handler(2, ev->x(), ev->y());
        else if (ev->button() == Qt::RightButton)
            button_release_handler(3, ev->x(), ev->y());
    }
    ev->accept();
}


void
QTedit::enter_slot(QEvent*)
{
    pe_entered = true;
    redraw();
}


void
QTedit::leave_slot(QEvent*)
{
    pe_entered = false;
    redraw();
}


void
QTedit::drag_enter_slot(QDragEnterEvent*)
{
}


void
QTedit::drop_slot(QDropEvent*)
{
}


void
QTedit::keys_press_slot(QMouseEvent *ev)
{
    // Pop up info about the keys pressed area in help mode.

    if (XM()->IsDoingHelp() && ev->button() == Qt::LeftButton &&
            !(ev->modifiers() & Qt::ShiftModifier))
        DSPmainWbag(PopUpHelp("keyspresd"))
}


#ifdef notdef


namespace {
    // Copy the string, encoding unicode excapes to UTF-8 characters.
    //
    char *uni_decode(const char *str)
    {
        sLstr lstr;
        if (str) {
            for (const char *s = str; *s; s++) {
                sUni uc;
                if (*s == '\\') {
                    if (*(s+1) == 'u') {
                        // This should be followed by exactly 4 hex digits.
                        const char *t = s+2;
                        if (uc.addc(t[0]) && uc.addc(t[1]) && uc.addc(t[2]) &&
                                uc.addc(t[3])) {
                            lstr.add(uc.utf8_encode());
                            s += 5;
                            continue;
                        }
                    }
                    if (*(s+1) == 'U') {
                        // This should be followed by exactly 8 hex digits.
                        const char *t = s+2;
                        if (uc.addc(t[0]) && uc.addc(t[1]) && uc.addc(t[2]) &&
                                uc.addc(t[3]) && uc.addc(t[4]) && uc.addc(t[5]) &&
                                uc.addc(t[6]) && uc.addc(t[7])) {
                            lstr.add(uc.utf8_encode());
                            s += 9;
                            continue;
                        }
                    }
                }
                lstr.add_c(*s);
            }
        }
        return (lstr.string_trim());
    }
}


// Selection handler, supports hypertext transfer
//
void
pe_selection_proc(GtkWidget*, GtkSelectionData *sdata, void*)
{
    if (gtk_selection_data_get_length(data) < 0) {
        ptr()->draw_cursor(DRAW);
        return;
    }
    if (gtk_selection_data_get_data_type(data) != GDK_SELECTION_TYPE_STRING) {
        Log()->ErrorLog(mh::Internal,
            "Selection conversion failed. not string data.");
        ptr()->draw_cursor(DRAW);
        return;
    }

    hyList *hpl = 0;
    GdkWindow *wnd = gdk_selection_owner_get(GDK_SELECTION_PRIMARY);
    if (wnd) {
        GtkWidget *widget;
        gdk_window_get_user_data(wnd, (void**)&widget);
        if (widget && GTK_IS_TEXT_VIEW(widget)) {
            int code = (intptr_t)g_object_get_data(G_OBJECT(widget),
                "hyexport");
            if (code) {
                GtkTextBuffer *gb =
                    gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
                GtkTextIter siter, eiter;
                gtk_text_buffer_get_selection_bounds(gb, &siter, &eiter);
                int start = gtk_text_iter_get_offset(&siter);

                // The text is coming from the Property Editor or
                // Property Info pop-up, fetch the original
                // hypertext to insert.
                CDo *odesc;
                PrptyText *p = EditIf()->PropertyResolve(code, start, &odesc);
                if (p && p->prpty()) {
                    CDs *cursd = CurCell(true);
                    if (cursd)
                        hpl = cursd->hyPrpList(odesc, p->prpty());
                }
            }
        }
    }
    if (!hpl) {
        // Might not be 0-terminated?
        int len = gtk_selection_data_get_length(data);
        char *s = new char[len + 1];
        memcpy(s, gtk_selection_data_get_data(data), len);
        s[len] = 0;
        char *d = uni_decode(s);
        delete [] s;
        ptr()->insert(d);
        delete [] d;
    }
    else {
        ptr()->insert(hpl);
        hyList::destroy(hpl);
    }
}


// Drag data received in main window - open the cell
//
static void
pe_drag_data_received(GtkWidget*, GdkDragContext *context, gint, gint,
    GtkSelectionData *data, guint, guint time)
{
    bool success = false;
    if (!ptr() || !gtk_selection_data_get_data(data))
        ;
    else if (gtk_selection_data_get_target(data) ==
            gdk_atom_intern("property", true)) {
        if (ptr()->is_active()) {
            unsigned char *val =
                (unsigned char*)gtk_selection_data_get_data(data) + sizeof(int);
            CDs *cursd = CurCell(true);
            hyList *hp = new hyList(cursd, (char*)val, HYcvAscii);
            ptr()->insert(hp);
            hyList::destroy(hp);
            success = true;
        }
    }
    else {
        if (gtk_selection_data_get_length(data) >= 0 &&
                gtk_selection_data_get_format(data) == 8) {
            char *src = (char*)gtk_selection_data_get_data(data);
            char *t = 0;
            if (gtk_selection_data_get_target(data) ==
                    gdk_atom_intern("TWOSTRING", true)) {
                // Drops from content lists may be in the form
                // "fname_or_chd\ncellname".
                t = strchr(src, '\n');
            }
            if (ptr()->is_active()) {
                // Keep the cellname.
                if (t)
                    src = t+1;
                // If editing, push into prompt line.
                ptr()->insert(src);
            }
            else {
                if (t) {
                    *t++ = 0;
                    XM()->Load(EV()->CurrentWin(), src, 0, t);
                }
                else
                    XM()->Load(EV()->CurrentWin(), src);
            }
            success = true;
        }
    }
    gtk_drag_finish(context, success, false, time);
}

// Static functions.
// Pointer motion handler.
//
int
GTKedit::pe_motion_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    if (!ptr())
        return (false);
    if (ptr()->pe_has_drag) {
        ptr()->pointer_motion_handler(
            (int)event->motion.x, (int)event->motion.y);
        return (true);
    }   
    return (false);
}


// Static function.
// Selection clear handler.
//
int
GTKedit::pe_selection_clear(GtkWidget*, GdkEventSelection*, void*)
{
    if (ptr())
        ptr()->deselect(true);
    return (true);
}


void
GTKedit::pe_selection_get(GtkWidget*, GtkSelectionData *data,
    guint, guint, void*)
{
    if (gtk_selection_data_get_selection(data) != GDK_SELECTION_PRIMARY)
        return;
    if (!ptr())
        return;
        
    unsigned char *s = (unsigned char*)ptr()->get_sel();
    if (!s)
        return;  // refuse
    int length = strlen((const char*)s);

    // The UTF8_STRING atom was discovered by looking at GTK source.
    // GDK_SELECTION_TYPE_STRING doesn't work for UTF-8 when target
    // is a GTK window.

    gtk_selection_data_set(data,
        gdk_atom_intern_static_string("UTF8_STRING"), 8*sizeof(char),
        s, length);
    delete [] s;
}

// End of handlers

#endif
