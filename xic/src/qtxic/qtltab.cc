
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

#include "config.h"
#include "qtmain.h"
#include "qtltab.h"
#include "qtmenu.h"
#include "qtinterf/qtfont.h"
#include "dsp_inlines.h"
#include "dsp_layer.h"
#include "select.h"
#include "events.h"
#include "keymap.h"
#include "miscutil/timer.h"
#include "bitmaps/lsearch.xpm"
#include "bitmaps/lsearchn.xpm"

#include <unistd.h>
#include <errno.h>

#include <QApplication>
#include <QWidget>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDrag>
#include <QMimeData>
#include <QLayout>
#include <QToolButton>
#include <QScrollBar>
#include <QLineEdit>
#include <QSplitter>
#include <QWheelEvent>


//-----------------------------------------------------------------------------
// QTltab:  The Layer Table composite, used in the main window.
//
// help keywords used:
//  layertab
//  layerchange

// Drag and draw things exported for the fill pattern editor.
const char *QTltab::ltab_mime_type = "application/xic-fillpattern";

const char *QTltab::ltab_fillpattern_xpm[] = {
    // width height ncolors chars_per_pixel
    "24 24 3 1",
    // colors
    " 	c None",
    ".	c #ffffff",
    "+  c #000000",
    // pixels
    "++++++++++++++++++++++++",
    "+......................+",
    "+..+     ..+     ..+  .+",
    "+...+     ..+     ..+ .+",
    "+. ..+     ..+     ..+.+",
    "+.  ..+     ..+     ...+",
    "+.   ..+     ..+     ..+",
    "+.    ..+     ..+     .+",
    "+.     ..+     ..+    .+",
    "+.+     ..+     ..+   .+",
    "+..+     ..+     ..+  .+",
    "+...+     ..+     ..+ .+",
    "+. ..+     ..+     ..+.+",
    "+.  ..+     ..+     ...+",
    "+.   ..+     ..+     ..+",
    "+.    ..+     ..+     .+",
    "+.     ..+     ..+    .+",
    "+.+     ..+     ..+   .+",
    "+..+     ..+     ..+  .+",
    "+...+     ..+     ..+ .+",
    "+. ..+     ..+     ..+.+",
    "+.  ..+     ..+     ...+",
    "+......................+",
    "++++++++++++++++++++++++"};

QTltab *QTltab::instancePtr = 0;

QTltab::QTltab(bool nogr) : QTdraw(XW_LTAB)
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class QTltab already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    ltab_scrollbar = 0;
    ltab_entry = 0;
    ltab_sbtn = 0;
    ltab_lsearch = 0;
    ltab_lsearchn = 0;

    ltab_search_str = 0;
    ltab_last_index = 0;
    ltab_last_mode = -1;
    ltab_timer_id = 0;
    ltab_hidden = false;

    lt_drag_x = 0;
    lt_drag_y = 0;
    lt_dragging = false;
    lt_disabled = nogr;
    if (nogr)
        return;

    QMargins qm;
    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->setContentsMargins(qm);
    vb->setSpacing(0);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(0);

    ltab_scrollbar = new QScrollBar(Qt::Vertical, this);
    hbox->addWidget(ltab_scrollbar);

    gd_viewport = new QTcanvas();
    gd_viewport->setAcceptDrops(true);
    hbox->addWidget(gd_viewport);

    vb->addLayout(hbox);

    QFont *scfont;
    if (Fnt()->getFont(&scfont, FNT_SCREEN))
        gd_viewport->set_font(scfont);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed(int)), Qt::QueuedConnection);

    connect(gd_viewport, SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));
    connect(gd_viewport, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(button_press_slot(QMouseEvent*)));
    connect(gd_viewport, SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(button_release_slot(QMouseEvent*)));
    connect(gd_viewport, SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(motion_slot(QMouseEvent*)));
    connect(gd_viewport, SIGNAL(mouse_wheel_event(QWheelEvent*)),
        this, SLOT(mouse_wheel_slot(QWheelEvent*)));
    connect(ltab_scrollbar, SIGNAL(valueChanged(int)),
        this, SLOT(ltab_scroll_value_changed_slot(int)));
    connect(gd_viewport, SIGNAL(drag_enter_event(QDragEnterEvent*)),
        this, SLOT(drag_enter_slot(QDragEnterEvent*)));
    connect(gd_viewport, SIGNAL(drop_event(QDropEvent*)),
        this, SLOT(drop_event_slot(QDropEvent*)));
}


QTltab::~QTltab()
{
    instancePtr = 0;
}


void
QTltab::on_null_ptr()
{
    fprintf(stderr, "Singleton class QTltab used before instantiated.\n");
    exit(1);
}


void
QTltab::setup_drawable()
{
    // no action necessary
}


namespace {

    struct blinker
    {
        blinker(QTsubwin*, const CDl*);
        ~blinker();

        bool is_ok()
        {
            return (b_wid > 0 && b_hei > 0 && b_dim_pm && b_norm_pm);
        }

        void show_dim()
        {
            if (!b_dim_pm)
                return;
            b_win->Viewport()->draw_pixmap(0, 0, b_dim_pm, 0, 0, b_wid, b_hei);
            b_win->Viewport()->update();
        }

        void show_norm()
        {
            if (!b_norm_pm)
                return;
            b_win->Viewport()->draw_pixmap(0, 0, b_norm_pm, 0, 0, b_wid, b_hei);
            b_win->Viewport()->update();
        }

        static bool button3_down()
        {
            return (QApplication::mouseButtons() &
                (Qt::RightButton | Qt::LeftButton));
        }

        static int blink_idle(void*);

    private:
        QTsubwin *b_win;
        QPixmap *b_dim_pm;
        QPixmap *b_norm_pm;
        int b_wid;
        int b_hei;

        static blinker *blinkers[DSP_NUMWINS];
        static int blink_timer_id;
        static bool blink_state;
    };

    blinker *blinker::blinkers[DSP_NUMWINS];
    int blinker::blink_timer_id;
    bool blinker::blink_state;
}


void
QTltab::blink(CDl *ld)
{
    if (lt_disabled)
        return;

    for (int i = 0; i < DSP_NUMWINS; i++) {
        WindowDesc *wd = DSP()->Window(i);
        if (!wd)
            continue;
        QTsubwin *w = dynamic_cast<QTsubwin*>(wd->Wbag());
        if (!w)
            continue;
        blinker *b = new blinker(w, ld);
        if (!b->is_ok()) {
            delete b;
            continue;
        }
    }
}


void
QTltab::show(const CDl *ld)
{
    show_direct(ld);
}


// Exposure redraw.
//
void
QTltab::refresh(int xx, int yy, int w, int h)
{
    gd_viewport->repaint(xx, yy, w, h);
}


// Return the drawing area size.
//
void
QTltab::win_size(int *w, int *h)
{
    QSize qs = gd_viewport->size();
    *w = qs.width();
    *h = qs.height();
}


// Update the display.
//
void
QTltab::update()
{
    Update();
}


void
QTltab::update_scrollbar()
{
    if (ltab_scrollbar) {
        int numents = CDldb()->layersUsed(DSP()->CurMode()) - 1;
        int nm = numents - lt_vis_entries;
        if (nm < 0)
            nm = 0;
        ltab_scrollbar->setMaximum(nm);
        ltab_scrollbar->setSingleStep(1);
        ltab_scrollbar->setPageStep(nm);
        emit valueChanged(ltab_scrollbar->value());
    }
}


void
QTltab::hide_layer_table(bool hidelt)
{
    if (hidelt)
        QTmainwin::self()->splitter()->widget(0)->setVisible(false);
    else
        QTmainwin::self()->splitter()->widget(0)->setVisible(true);
}


void
QTltab::set_layer()
{
    CDl *ld = LT()->CurLayer();
    if (ld)
        ltab_entry->setText(tr(ld->name()));
    else {
        ltab_entry->setText("");
        return;
    }

    int posn = ld->index(DSP()->CurMode()) - 1;
    if (posn >= first_visible() && posn < first_visible() + vis_entries())
        show();
    else {
        int nt = CDldb()->layersUsed(DSP()->CurMode()) - 1;
        nt -= vis_entries();
        posn -= vis_entries()/2;
        if (posn < 0)
            posn = 0;
        else if (posn > nt)
            posn = nt;
        lt_first_visible = posn;
        update_scrollbar();
        show();
    }
}


void
QTltab::set_search_widgets(QToolButton *p, QLineEdit *e)
{
    if (!p || !e)
        return;
    ltab_sbtn = p;
    ltab_entry = e;

    ltab_sbtn->setToolTip(tr("Search layer table for layer."));
    ltab_lsearch = new QPixmap(lsearch_xpm);
    ltab_sbtn->setIcon(*ltab_lsearch);
    connect(ltab_sbtn, SIGNAL(clicked()), this, SLOT(pressed_slot()));
}


void
QTltab::resize_slot(QResizeEvent*)
{
    init();
    show();
}


// Dispatch button presses in the layer table area.
//
void
QTltab::button_press_slot(QMouseEvent *ev)
{
    int button = 0;
    if (ev->button() == Qt::LeftButton)
        button = 1;
    else if (ev->button() == Qt::MiddleButton)
        button = 2;
    else if (ev->button() == Qt::RightButton)
        button = 3;

    button = Kmap()->ButtonMap(button);
    int state = mod_state(ev->modifiers());

    if (XM()->IsDoingHelp() && (state & GR_SHIFT_MASK)) {
        DSPmainWbag(PopUpHelp("layertab"))
        return;
    }

    switch (button) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    case 1:
        b1_handler(ev->position().x(), ev->position().y(), state, true);
        break;
    case 2:
        b2_handler(ev->position().x(), ev->position().y(), state, true);
        break;
    case 3:
        b3_handler(ev->position().x(), ev->position().y(), state, true);
        break;
#else
    case 1:
        b1_handler(ev->x(), ev->y(), state, true);
        break;
    case 2:
        b2_handler(ev->x(), ev->y(), state, true);
        break;
    case 3:
        b3_handler(ev->x(), ev->y(), state, true);
        break;
#endif
    }
    update();
}


void
QTltab::button_release_slot(QMouseEvent *ev)
{
    int button = 0;
    if (ev->button() == Qt::LeftButton)
        button = 1;
    else if (ev->button() == Qt::MiddleButton)
        button = 2;
    else if (ev->button() == Qt::RightButton)
        button = 3;

    button = Kmap()->ButtonMap(button);
    int state = mod_state(ev->modifiers());

    if (XM()->IsDoingHelp() && (state & GR_SHIFT_MASK)) {
        DSPmainWbag(PopUpHelp("layertab"))
        return;
    }

    switch (button) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    case 1:
        b1_handler(ev->position().x(), ev->position().y(), state, false);
        break;
    case 2:
        b2_handler(ev->position().x(), ev->position().y(), state, false);
        break;
    case 3:
        b3_handler(ev->position().x(), ev->position().y(), state, false);
        break;
#else
    case 1:
        b1_handler(ev->x(), ev->y(), state, false);
        break;
    case 2:
        b2_handler(ev->x(), ev->y(), state, false);
        break;
    case 3:
        b3_handler(ev->x(), ev->y(), state, false);
        break;
#endif
    }
}


void
QTltab::motion_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseMove) {
        ev->ignore();
        return;
    }
    ev->accept();

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    if (drag_check(xx, yy)) {
        // fillpattern only

        int entry = entry_of_xy(xx, yy);
        int last_ent = last_entry();
        if (entry <= last_ent) {
            CDl *ld =
                CDldb()->layer(entry + first_visible() + 1, DSP()->CurMode());
            XM()->PopUpLayerPalette(0, MODE_UPD, true, ld);

            LayerFillData dd(ld);
            QDrag *drag = new QDrag(sender());
            drag->setPixmap(QPixmap(fillpattern_xpm()));
            QMimeData *mimedata = new QMimeData();
            QByteArray qdata((const char*)&dd, sizeof(LayerFillData));
            mimedata->setData(mime_type(), qdata);
            drag->setMimeData(mimedata);
            drag->exec(Qt::CopyAction);
        }
    }
}


void
QTltab::drag_enter_slot(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasFormat(QTltab::mime_type()) ||
            ev->mimeData()->hasColor())
        ev->accept();
    else
        ev->ignore();
}


void
QTltab::drop_event_slot(QDropEvent *ev)
{
    if (ev->mimeData()->hasFormat(QTltab::mime_type())) {
        QByteArray bary = ev->mimeData()->data(QTltab::mime_type());
        LayerFillData *dd = (LayerFillData*)bary.data();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        XM()->FillLoadCallback(dd,
            LT()->LayerAt(ev->position().x(), ev->position().y()));
#else
        XM()->FillLoadCallback(dd, LT()->LayerAt(ev->pos().x(), ev->pos().y()));
#endif
        if (DSP()->CurMode() == Electrical || !LT()->NoPhysRedraw())
            DSP()->RedisplayAll();
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasColor()) {
        QColor color = qvariant_cast<QColor>(ev->mimeData()->colorData());
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        int entry = entry_of_xy(ev->position().x(), ev->position().y());
#else
        int entry = entry_of_xy(ev->pos().x(), ev->pos().y());
#endif

        if (entry > last_entry())
            return;
        CDl *layer =
            CDldb()->layer(entry + first_visible() + 1, DSP()->CurMode());

        LT()->SetLayerColor(layer, color.red(), color.green(), color.blue());
        // update the colors
        LT()->ShowLayerTable(layer);
        XM()->PopUpFillEditor(0, MODE_UPD);
        if (DSP()->CurMode() == Electrical || !LT()->NoPhysRedraw())
            DSP()->RedisplayAll();
        ev->accept();
        return;
    }
    ev->ignore();
}


void
QTltab::mouse_wheel_slot(QWheelEvent *ev)
{
    ltab_scrollbar->event(ev);
}


void
QTltab::ltab_scroll_value_changed_slot(int val)
{
    if (val != first_visible()) {
        set_first_visible(val);
        show();
    }
}


void
QTltab::font_changed(int fnum)
{
    if (fnum == FNT_SCREEN) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_SCREEN))
            gd_viewport->set_font(fnt);
        init();
    }
}


void
QTltab::pressed_slot()
{
    if (!ltab_search_str)
        ltab_search_str =
            lstring::copy(ltab_entry->text().toLatin1());
    if (!ltab_search_str)
        return;
    CDl *ld = CDldb()->search(DSP()->CurMode(), ltab_last_index + 1,
        ltab_search_str);
    ltab_last_index = 0;
    if (!ld) {
        delete [] ltab_search_str;
        ltab_search_str = 0;
        return;
    }

    ltab_last_index = ld->index(DSP()->CurMode());
    LT()->SetCurLayer(ld);

    if (!ltab_lsearchn)
        ltab_lsearchn = new QPixmap(lsearchn_xpm);
    ltab_sbtn->setIcon(*ltab_lsearchn);

    if (ltab_timer_id)
        QTdev::self()->RemoveTimer(ltab_timer_id);
    ltab_timer_id = QTdev::self()->AddTimer(3000, ltab_ent_timer, this);
}


// Static function.
int
QTltab::ltab_ent_timer(void *arg)
{
    QTltab *lt = static_cast<QTltab*>(arg);
    if (lt) {
        lt->ltab_last_index = 0;
        delete [] lt->ltab_search_str;
        lt->ltab_search_str = 0;
        lt->ltab_sbtn->setIcon(*lt->ltab_lsearch);
        if (LT()->CurLayer())
            lt->ltab_entry->setText(tr(LT()->CurLayer()->name()));
        lt->ltab_timer_id = 0;
    }
    return (0);
}
// End of QTltab functions.


#define DIMPIXVAL 5

namespace {
    blinker::blinker(QTsubwin *wb, const CDl *ld)
    {
        b_win = wb;
        b_dim_pm = 0;
        b_norm_pm = wb ? new QPixmap(*wb->Viewport()->pixmap()) : 0;
        b_wid = b_norm_pm ? b_norm_pm->width() : 0;
        b_hei = b_norm_pm ? b_norm_pm->height() : 0;
        if (!wb || !ld)
            return;

        QImage qimg(b_wid, b_hei, QImage::Format_RGB32);
        QPainter p(&qimg);
        p.drawPixmap(0, 0, *b_norm_pm);
        QRgb rgb(dsp_prm(ld)->pixel());
        QColor qcref(rgb);
        QColor qcdim(qRgb((qRed(rgb)*DIMPIXVAL)/10,
            (qGreen(rgb)*DIMPIXVAL)/10, (qBlue(rgb)*DIMPIXVAL)/10));
        for (int c = 0; c < b_wid; c++) {
            for (int r = 0; r < b_hei; r++) {
                if (qimg.pixelColor(c, r) == qcref)
                    qimg.setPixelColor(c, r, qcdim);
            }
        }
        b_dim_pm = new QPixmap(b_wid, b_hei);
        QPainter pd(b_dim_pm);
        pd.drawImage(0, 0, qimg);

        if (is_ok()) {
            blinker *oldb = blinkers[wb->win_number()];
            blinkers[wb->win_number()] = this;
            delete oldb;

            if (!blink_timer_id) {
                blink_timer_id =
                    QTpkg::self()->RegisterIdleProc(blink_idle, 0);
            }
        }
    }


    blinker::~blinker()
    {
        delete b_dim_pm;
        delete b_norm_pm;
    }


    // Static function.
    // Idle proc for blinking layers.
    //
    int blinker::blink_idle(void*) {
        int cnt = 0;
        for (int i = 0; i < DSP_NUMWINS; i++) {
            if (!blinkers[i])
                continue;
            cnt++;
            if (!blink_state)
                blinkers[i]->show_dim();
            else
                blinkers[i]->show_norm();
        }
        cTimer::milli_sleep(400);

        if (!button3_down()) {
            for (int i = 0; i < DSP_NUMWINS; i++) {
                if (!blinkers[i])
                    continue;
                blinkers[i]->show_norm();
                delete blinkers[i];
                blinkers[i] = 0;
            }
            cnt = 0;
        }
        if (!cnt) {
            blink_timer_id = 0;
            blink_state = false;
            return (0);
        }
        blink_state = !blink_state;
        return (1);
    }
}

