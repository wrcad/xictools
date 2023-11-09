
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
#include "select.h"
#include "events.h"
#include "keymap.h"
#include "bitmaps/lsearch.xpm"
#include "bitmaps/lsearchn.xpm"

#include <unistd.h>
#include <errno.h>

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

// help keywords used:
//  layertab
//  layerchange


//--------------------------------------------------------------------
//
// The Layer Table
//
//--------------------------------------------------------------------

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
    if (FC.getFont(&scfont, FNT_SCREEN))
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


void
QTltab::blink(CDl *ld)
{
    //XXX FIXME
(void)ld;
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
    if (hidelt) {
        QTmainwin::self()->splitter()->widget(0)->hide();
    }
    else {
//XXX fixme: pages of Painter not active messages.
        QTmainwin::self()->splitter()->widget(0)->show();
    }
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
    if (ev->mimeData()->hasFormat(QTltab::mime_type()) || ev->mimeData()->hasColor())
        ev->accept();
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
        if (FC.getFont(&fnt, FNT_SCREEN))
            gd_viewport->set_font(fnt);
        //XXX redraw
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


#ifdef notdef

/*
static unsigned int
revbytes(unsigned bits, int nb)
{
    unsigned int tmp = bits;
    bits = 0;
    unsigned char *a = (unsigned char*)&tmp;
    unsigned char *b = (unsigned char*)&bits;
    nb--;
    for (int i = 0; i <= nb; i++)
        b[i] = a[nb-i];
    return (bits);
}
*/


#define DIMPIXVAL 5

#ifdef WIN32

// Since the gtk implementation doesn't work as of 2.24.10, do it
// in Win32.  Some pretty ugly stuff here.

namespace {
    GdkGCValuesMask Win32GCvalues =
        (GdkGCValuesMask)(GDK_GC_FOREGROUND | GDK_GC_BACKGROUND);

    struct blinker
    {
        blinker(GTKsubwin*, const CDl*);

        ~blinker()
            {
                if (b_dcMemNorm)
                    DeleteDC(b_dcMemNorm);
                if (b_bmapNorm)
                    DeleteBitmap(b_bmapNorm);
                if (b_dcMemDim)
                    DeleteDC(b_dcMemDim);
                if (b_bmapDim)
                    DeleteBitmap(b_bmapDim);
            }

        bool is_ok()
            {
                return (b_wid > 0 && b_hei > 0 && b_dcMemNorm && b_bmapNorm &&
                    b_dcMemDim && b_bmapDim);
            }

        void show_dim()
            {
                if (!b_dcMemDim || !b_bmapDim)
                    return;
                HDC dc = gdk_win32_hdc_get(b_wbag->Window(), b_wbag->GC(),
                    Win32GCvalues);
                BitBlt(dc, b_xoff, b_yoff, b_wid + 1, b_hei + 1, b_dcMemDim,
                    0, 0, SRCCOPY);
                gdk_win32_hdc_release(b_wbag->Window(), b_wbag->GC(),
                    Win32GCvalues);
            }

        void show_norm()
            {
                if (!b_dcMemNorm || !b_bmapNorm)
                    return;
                HDC dc = gdk_win32_hdc_get(b_wbag->Window(), b_wbag->GC(),
                    Win32GCvalues);
                BitBlt(dc, b_xoff, b_yoff, b_wid + 1, b_hei + 1, b_dcMemNorm,
                    0, 0, SRCCOPY);
                gdk_win32_hdc_release(b_wbag->Window(), b_wbag->GC(),
                    Win32GCvalues);
            }

    private:
        GTKsubwin *b_wbag;
        HDC b_dcMemDim;
        HDC b_dcMemNorm;
        HBITMAP b_bmapDim;
        HBITMAP b_bmapNorm;
        int b_wid;
        int b_hei;
        int b_xoff;
        int b_yoff;
    };


    blinker::blinker(GTKsubwin *wb, const CDl *ld)
    {
        b_wbag = wb;
        b_dcMemDim = 0;
        b_dcMemNorm = 0;
        b_bmapDim = 0;
        b_bmapNorm = 0;
        b_xoff = 0;
        b_yoff = 0;

        gdk_window_get_size(wb->Window(), &b_wid, &b_hei);
        if (b_wid < 0 || b_hei < 0 || (!b_wid && !b_hei))
            return;

        // Note the ordering of the COLORREFs relative to the GDK pixels.

        // The dim pixel that alternates with the regular color.
        DspLayerParams *lp = dsp_prm(ld);
        COLORREF dimpix = RGB(
            (GetBValue(lp->pixel())*DIMPIXVAL)/10,
            (GetGValue(lp->pixel())*DIMPIXVAL)/10,
            (GetRValue(lp->pixel())*DIMPIXVAL)/10);

        if (GDK_IS_WINDOW(wb->Window())) {
            // Need this correction for windows without implementation.

            gdk_window_get_internal_paint_info(wb->Window(), 0,
                &b_xoff, &b_yoff);
            b_xoff = -b_xoff;
            b_yoff = -b_yoff;
        }

        HDC dc = gdk_win32_hdc_get(wb->Window(), wb->GC(), Win32GCvalues);
        struct dc_releaser
        {
            dc_releaser(GTKsubwin *wbg)  { wbag = wbg; }
            ~dc_releaser()
            {
                gdk_win32_hdc_release(wbag->Window(), wbag->GC(),
                    Win32GCvalues);
            }
            GTKsubwin *wbag;
        } releaser(wb);

        // Create a pixmap backing the entire window.
        b_dcMemNorm = CreateCompatibleDC(dc);
        if (!b_dcMemNorm)
            return;
        b_bmapNorm = CreateCompatibleBitmap(dc, b_wid, b_hei);
        if (!b_bmapNorm)
            return;
        HBITMAP obm = SelectBitmap(b_dcMemNorm, b_bmapNorm);
        BitBlt(b_dcMemNorm, 0, 0, b_wid + 1, b_hei + 1, dc, b_xoff, b_yoff,
            SRCCOPY);
        SelectBitmap(b_dcMemNorm, obm);
        // The bimap must not be selected into a dc when passed to the
        // image map add function (M$ headache #19305).
        DeleteDC(b_dcMemNorm);
        b_dcMemNorm = 0;

        // Create am image list, which gives us the masking function.
        HIMAGELIST h = ImageList_Create(b_wid, b_hei, ILC_COLORDDB | ILC_MASK,
            0, 0);
        if (!h)
            return;

        ImageList_SetBkColor(h, dimpix);
        // in 256 color mode, the mask is always for the background,
        // whatever pixel is passed.

        COLORREF pix = RGB(GetBValue(lp->pixel()),
            GetGValue(lp->pixel()), GetRValue(lp->pixel()));
        if (ImageList_AddMasked(h, b_bmapNorm, pix) < 0) {
            ImageList_Destroy(h);
            return;
        }
        DeleteBitmap(b_bmapNorm);
        b_bmapNorm = 0;

        // Create a bitmap and draw the image,  The dim-color background
        // of the image list replaces ld->pixel.  Destroy the image list
        // since we're done with it.
        b_dcMemDim = CreateCompatibleDC(dc);
        if (!b_dcMemDim) {
            ImageList_Destroy(h);
            return;
        }
        b_bmapDim = CreateCompatibleBitmap(dc, b_wid, b_hei);
        if (!b_bmapDim) {
            ImageList_Destroy(h);
            return;
        }
        SelectBitmap(b_dcMemDim, b_bmapDim);
        ImageList_Draw(h, 0, b_dcMemDim, 0, 0, ILD_NORMAL);
        ImageList_Destroy(h);

        // Recreate the reference bitmap.  It gets trashed by the image
        // list creation function, so it doesn't work to keep the
        // original.
        b_dcMemNorm = CreateCompatibleDC(dc);
        if (!b_dcMemNorm)
            return;
        b_bmapNorm = CreateCompatibleBitmap(dc, b_wid, b_hei);
        if (!b_bmapNorm)
            return;
        SelectBitmap(b_dcMemNorm, b_bmapNorm);
        BitBlt(b_dcMemNorm, 0, 0, b_wid + 1, b_hei + 1, dc, b_xoff, b_yoff,
            SRCCOPY);
    }


    // Return true if button 3 is pressed.
    //
    bool button3_down()
    {
        return (GetAsyncKeyState(VK_RBUTTON) < 0);
    }
}


#else

namespace {
    struct blinker
    {
        blinker(GTKsubwin*, const CDl*);

        ~blinker()
            {
#if GTK_CHECK_VERSION(3,0,0)
                if (b_dim_pm)
                    b_dim_pm->dec_ref();
                if (b_norm_pm)
                    b_norm_pm->dec_ref();
#else
                if (b_dim_pm)
                    gdk_pixmap_unref(b_dim_pm);
                if (b_norm_pm)
                    gdk_pixmap_unref(b_norm_pm);
#endif
            }

        bool is_ok()
            {
                return (b_wid > 0 && b_hei > 0 && b_dim_pm && b_norm_pm);
            }

        void show_dim()
            {
                if (!b_dim_pm)
                    return;
#if GTK_CHECK_VERSION(3,0,0)
                b_dim_pm->copy_to_window(b_wbag->GetDrawable()->get_window(),
                    b_wbag->GC(), 0, 0, 0, 0, b_wid, b_hei);
#else
                gdk_window_copy_area(b_wbag->Window(), b_wbag->GC(), 0, 0,
                    b_dim_pm, 0, 0, b_wid, b_hei);
                gdk_flush();
#endif
            }

        void show_norm()
            {
                if (!b_norm_pm)
                    return;
#if GTK_CHECK_VERSION(3,0,0)
                b_norm_pm->copy_to_window(b_wbag->GetDrawable()->get_window(),
                    b_wbag->GC(), 0, 0, 0, 0, b_wid, b_hei);
#else
                gdk_window_copy_area(b_wbag->Window(), b_wbag->GC(), 0, 0,
                    b_norm_pm, 0, 0, b_wid, b_hei);
#endif
                gdk_flush();
            }

    private:
        GTKsubwin *b_wbag;
#if GTK_CHECK_VERSION(3,0,0)
        ndkPixmap *b_dim_pm;
        ndkPixmap *b_norm_pm;
#else
        GdkPixmap *b_dim_pm;
        GdkPixmap *b_norm_pm;
#endif
        int b_wid;
        int b_hei;
    };


    unsigned int revbytes(unsigned bits, int nb)
    {
        unsigned int tmp = bits;
        bits = 0;
        unsigned char *a = (unsigned char*)&tmp;
        unsigned char *b = (unsigned char*)&bits;
        nb--;
        for (int i = 0; i <= nb; i++)
            b[i] = a[nb-i];
        return (bits);
    }


    blinker::blinker(GTKsubwin *wb, const CDl *ld)
    {
        b_wbag = wb;
        b_dim_pm = 0;
        b_norm_pm = 0;
        b_wid = 0;
        b_hei = 0;

#if GTK_CHECK_VERSION(3,0,0)
        if (!wb)
            return;

        GdkWindow *window = wb->GetDrawable()->get_window();
        ndkPixmap *pm = new ndkPixmap(window, 1, 1);
        if (!pm)
            return;
        wb->GetDrawable()->set_pixmap(pm);
        wb->SetColor(dsp_prm(ld)->pixel());
        wb->Pixel(0, 0);
        wb->GetDrawable()->set_window(window);  // frees pm
        ndkImage *im = new ndkImage(pm, 0, 0, 1, 1);
        if (!im)
            return;

        int bpp = im->get_bytes_per_pixel();

        int im_order = im->get_byte_order();
        int order = GDK_LSB_FIRST;
        unsigned int pix = 1;
        if (!(*(unsigned char*)&pix))
            order = GDK_MSB_FIRST;

        pix = 0;
        memcpy(&pix, im->get_pixels(), bpp);
        if (order != im_order)
            pix = revbytes(pix, bpp);
        if (order == GDK_MSB_FIRST)
            pix >>= 8*(sizeof(int) - bpp);
        delete im;

        // im->visual = 0 from pixmap
        unsigned int red_mask, green_mask, blue_mask;
        gdk_visual_get_red_pixel_details(GTKdev::self()->Visual(),
            &red_mask, 0, 0);
        gdk_visual_get_green_pixel_details(GTKdev::self()->Visual(),
            &green_mask, 0, 0);
        gdk_visual_get_blue_pixel_details(GTKdev::self()->Visual(),
            &blue_mask, 0, 0);

        int r = (pix & red_mask);
        r = (((r * DIMPIXVAL)/10) & red_mask);
        int g = (pix & green_mask);
        g = (((g * DIMPIXVAL)/10) & green_mask);
        int b = (pix & blue_mask);
        b = (((b * DIMPIXVAL)/10) & blue_mask);
        unsigned int dimpix = r | g | b;

        if (order == GDK_MSB_FIRST) {
            pix <<= 8*(sizeof(int) - bpp);
            dimpix <<= 8*(sizeof(int) - bpp);
        }
        if (order != im_order) {
            pix = revbytes(pix, bpp);
            dimpix = revbytes(dimpix, bpp);
        }

        b_wid = wb->GetDrawable()->get_width();
        b_hei = wb->GetDrawable()->get_height();
        if (b_wid < 0 || b_hei < 0 || (!b_wid && !b_hei))
            return;

        pm = new ndkPixmap(window, b_wid, b_hei,
            gdk_visual_get_depth(GTKdev::self()->Visual()));
        if (!pm)
            return;
        pm->copy_from_window(window, wb->GC(), 0, 0, 0, 0, b_wid, b_hei);
        b_norm_pm = pm;

        // There is a bug in 2.24.10, using the window rather than the
        // pixmap in gdk_image_get doesn't work.  The image has offsets
        // that seem to require compensation with
        // gdk_window_get_internal_paint_info() or similar, but I could
        // never get this to work properly.

        im = new ndkImage(pm, 0, 0, b_wid, b_hei);
        if (!im) {
            delete pm;
            b_norm_pm = 0;
            return;
        }

        // There may be some alpha info that should be ignored.
        unsigned int mask = red_mask | green_mask | blue_mask;

        char *z = (char*)im->get_pixels();
        int i = b_wid*b_hei;
        while (i--) {
            unsigned f = 0;
            unsigned char *a = (unsigned char*)&f;
            for (int j = 0; j < bpp; j++)
                a[j] = *z++;
            if ((f & mask) == (pix & mask)) {
                z -= bpp;
                a = (unsigned char*)&dimpix;
                for (int j = 0; j < bpp; j++)
                    *z++ = a[j];
            }
        }
        pm = new ndkPixmap(window, b_wid, b_hei);
        if (!pm) {
            delete im;
            b_norm_pm->dec_ref();
            b_norm_pm = 0;
            return;
        }
        im->copy_to_pixmap(pm, wb->GC(), 0, 0, 0, 0, b_wid, b_hei);
        delete im;
        b_dim_pm = pm;

#else
        if (!wb)
            return;

        GdkPixmap *pm = gdk_pixmap_new(wb->Window(), 1, 1,
            gdk_visual_get_depth(GTKdev::self()->Visual()));
        if (!pm)
            return;
        GdkWindow *bk = wb->Window();
        wb->SetWindow(pm);
        wb->SetColor(dsp_prm(ld)->pixel());
        wb->Pixel(0, 0);
        wb->SetWindow(bk);
        GdkImage *im = gdk_image_get(pm, 0, 0, 1, 1);
        gdk_pixmap_unref(pm);
        if (!im)
            return;

        int bpp = im->bpp;

        int im_order = im->byte_order;
        int order = GDK_LSB_FIRST;
        unsigned int pix = 1;
        if (!(*(unsigned char*)&pix))
            order = GDK_MSB_FIRST;

        pix = 0;
        memcpy(&pix, im->mem, bpp);
        if (order != im_order)
            pix = revbytes(pix, bpp);
        if (order == GDK_MSB_FIRST)
            pix >>= 8*(sizeof(int) - bpp);
        gdk_image_destroy(im);

        // im->visual = 0 from pixmap
        unsigned red_mask = GTKdev::self()->Visual()->red_mask;
        unsigned green_mask = GTKdev::self()->Visual()->green_mask;
        unsigned blue_mask = GTKdev::self()->Visual()->blue_mask;
        int r = (pix & red_mask);
        r = (((r * DIMPIXVAL)/10) & red_mask);
        int g = (pix & green_mask);
        g = (((g * DIMPIXVAL)/10) & green_mask);
        int b = (pix & blue_mask);
        b = (((b * DIMPIXVAL)/10) & blue_mask);
        unsigned int dimpix = r | g | b;

        if (order == GDK_MSB_FIRST) {
            pix <<= 8*(sizeof(int) - bpp);
            dimpix <<= 8*(sizeof(int) - bpp);
        }
        if (order != im_order) {
            pix = revbytes(pix, bpp);
            dimpix = revbytes(dimpix, bpp);
        }

        gdk_window_get_size(wb->Window(), &b_wid, &b_hei);
        if (b_wid < 0 || b_hei < 0 || (!b_wid && !b_hei))
            return;

        pm = gdk_pixmap_new(wb->Window(), b_wid, b_hei,
            GTKdev::self()->Visual()->depth);
        if (!pm)
            return;
        gdk_window_copy_area(pm, wb->GC(), 0, 0, wb->Window(), 0, 0,
            b_wid, b_hei);
        b_norm_pm = pm;

        // There is a bug in 2.24.10, using the window rather than the
        // pixmap in gdk_image_get doesn't work.  The image has offsets
        // that seem to require compensation with
        // gdk_window_get_internal_paint_info() or similar, but I could
        // never get this to work properly.

        im = gdk_image_get(pm, 0, 0, b_wid, b_hei);
        if (!im) {
            gdk_pixmap_unref(pm);
            b_norm_pm = 0;
            return;
        }

        // There may be some alpha info that should be ignored.
        unsigned int mask = red_mask | green_mask | blue_mask;

        char *z = (char*)im->mem;
        int i = b_wid*b_hei;
        while (i--) {
            unsigned f = 0;
            unsigned char *a = (unsigned char*)&f;
            for (int j = 0; j < bpp; j++)
                a[j] = *z++;
            if ((f & mask) == (pix & mask)) {
                z -= bpp;
                a = (unsigned char*)&dimpix;
                for (int j = 0; j < bpp; j++)
                    *z++ = a[j];
            }
        }
        pm = gdk_pixmap_new(wb->Window(), b_wid, b_hei,
            gdk_visual_get_depth(GTKdev::self()->Visual()));
        if (!pm) {
            gdk_image_destroy(im);
            gdk_pixmap_unref(b_norm_pm);
            b_norm_pm = 0;
            return;
        }
        gdk_draw_image(pm, wb->GC(), im, 0, 0, 0, 0, b_wid, b_hei);
        gdk_image_destroy(im);
        b_dim_pm = pm;
#endif
    }


    // Return true if button 3 is pressed.
    //
    bool button3_down()
    {
        unsigned state;
        GTKmainwin::self()->QueryPointer(0, 0, &state);
        return (state & GDK_BUTTON3_MASK);
    }
}

#endif


namespace {
    blinker *blinkers[DSP_NUMWINS];
    int blink_timer_id;
    bool blink_state;


    // Idle proc for blinking layers.
    //
    int blink_idle(void*) {
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
        cTimer::milli_sleep(200);

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


void
GTKltab::blink(CDl *ld)
{
    if (lt_disabled)
        return;

    for (int i = 0; i < DSP_NUMWINS; i++) {
        WindowDesc *wd = DSP()->Window(i);
        if (!wd)
            continue;
        GTKsubwin *w = dynamic_cast<GTKsubwin*>(wd->Wbag());
        if (!w)
            continue;
        blinker *b = new blinker(w, ld);
        if (!b->is_ok()) {
            delete b;
            continue;
        }
        blinker *oldb = blinkers[i];
        blinkers[i] = b;
        delete oldb;
    }
    if (!blink_timer_id)
        blink_timer_id = g_idle_add(blink_idle, 0);
}

#endif
