
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

#include "config.h"
#include "gtkinterf.h"
#include "gtkfont.h"
#include "ginterf/grlinedb.h"
#include "miscutil/texttf.h"
#include "miscutil/lstring.h"
#ifdef WIN32
#include <winsock2.h>
#include <windowsx.h>
#include <gdk/gdkwin32.h>
#include "mswdraw.h"
#include "mswpdev.h"
using namespace mswinterf;
#endif


//#define USE_XSHM

#include <gdk/gdkkeysyms.h>
#ifdef WITH_X11
#include <X11/Xproto.h>
#ifdef USE_XSHM
#ifdef HAVE_SHMGET
#include <X11/extensions/XShm.h>  
#endif
#endif
#endif


#ifdef NEW_NDK
#ifdef WITH_X11
// XPoint, XSegment, XRectangle all use shorts.  This sets GRmultiPt
// to use shorts as well.

namespace {
struct GRsetupDummy
    {
        GRsetupDummy()  { GRmultiPt::set_short_data(true); }
    };
    GRsetupDummy __gt_dummy;
}
#endif
#endif

// Use our own Win32 drawing.  This is necessary, as currently gdk
// does not handle stippled drawing.
#define DIRECT_TO_GDI

// Graphics context storage.  The 0 element is the default.
//
sGbag *sGbag::app_gbags[NUMGCS];

sGbag::sGbag()
{
#ifdef NEW_NDK
    // Create GCs.  Use the root window as the drawable, hope that's ok.
    gb_gc = new ndkGC(0);
    gb_xorgc = new ndkGC(0);
    GdkColor clr;
    clr.pixel = 0;  // black
    gb_gc->set_background(&clr);
    gb_xorgc->set_background(&clr);
    clr.pixel = 0xffffff;  // white
    gb_gc->set_foreground(&clr);
    gb_xorgc->set_foreground(&clr);
    gb_xorgc->set_function(ndkGC_XOR);
    ndkGCvalues gcv;
    gcv.v_cap_style = ndkGC_CAP_NOT_LAST;
    gb_gc->set_values(&gcv, ndkGC_CAP_STYLE);
    gb_xorgc->set_values(&gcv, ndkGC_CAP_STYLE);
#else
    gb_gc = gdk_gc_new(gdk_get_default_root_window());
    gb_xorgc = gdk_gc_new(gdk_get_default_root_window());
    GdkColor clr;
    clr.pixel = 0;  // black
    gdk_gc_set_background(gb_gc, & clr);
    gdk_gc_set_background(gb_xorgc, & clr);
    clr.pixel = 0xffffff;  // white
    gdk_gc_set_foreground(gb_gc, & clr);
    gdk_gc_set_foreground(gb_xorgc, & clr);
    GdkGCValues gcvalues;
    gcvalues.function = GDK_XOR;
    gdk_gc_set_values(gb_xorgc, &gcvalues, GDK_GC_FUNCTION);
    gcvalues.cap_style = GDK_CAP_NOT_LAST;
    gdk_gc_set_values(gb_gc, &gcvalues, GDK_GC_CAP_STYLE);
    gdk_gc_set_values(gb_xorgc, &gcvalues, GDK_GC_CAP_STYLE);
#endif
    gb_gcbak = 0;
#ifdef WIN32
    gb_fillpattern = 0;
#endif
    gb_cursor_type = 0;
}


sGbag::~sGbag()
{
    // Don't destroy the GCs here.  WRspice will copy the sGbag but
    // keep the same GCs, so plot windows can have the same drawing
    // context except for XOR mode.  The Gbag copy will be freed,
    // obviously this would be bad news if the GCs were destroyed.
}


// Static method to create/return the default graphics context.  Note that
// this does not create the GCs.
//
sGbag *
sGbag::default_gbag(int type)
{
    if (type < 0 || type >= NUMGCS)
        type = 0;
    if (!app_gbags[type])
        app_gbags[type] = new sGbag;
    return (app_gbags[type]);
}


//-----------------------------------------------------------------------------
// GTKdraw functions

GTKdraw::GTKdraw(int type)
{
    gd_viewport = 0;
#ifdef NEW_NDK
#else
    gd_window = 0;
#endif
    gd_gbag = sGbag::default_gbag(type);
    gd_backg = 0;
    gd_foreg = 0xffffff;
    gd_xor_fg = gd_foreg;
}


GTKdraw::~GTKdraw()
{
    if (gd_gbag) {
        // Unless the gbag is in the array, free it.
        int i;
        for (i = 0; i < NUMGCS; i++) {
            if (sGbag::app_gbags[i] == gd_gbag)
                break;
        }
        if (i == NUMGCS)
            delete gd_gbag;
    }
}


#ifdef NEW_NDK
void
GTKdraw::SetViewport(GtkWidget *w)
{
    if (w && GTK_IS_WIDGET(w)) {
        // We handle our own double-buffering.
        gtk_widget_set_double_buffered(w, false);
        gd_viewport = w;
    }
    else
        gd_viewport = 0;
    gd_dw.set_window(0);
}


void *
GTKdraw::WindowID()
{
    return (gd_dw.get_window());
}
#endif


void
GTKdraw::Halt()
{
    // Applies to hard copy drivers only.
}


// Clear the drawing window.
//
void
GTKdraw::Clear()
{
#ifdef NEW_NDK
    if (gd_dw.get_state() == DW_NONE)
        return;
    SetColor(gd_backg);
    SetFillpattern(0);
    Box(0, 0, gd_dw.get_width(), gd_dw.get_height());
#else
    if (!GDK_IS_PIXMAP(gd_window)) {
        // doesn't work for pixmaps
        gdk_window_clear(gd_window);
    }
    else {
        int w, h;
        gdk_drawable_get_size(gd_window, &w, &h);
        gdk_draw_rectangle(gd_window, GC(), true, 0, 0, w, h);
    }
#endif
}


#if defined(WIN32) && defined(DIRECT_TO_GDI)
// The current gdk-win32 does not handle stippled rendering.  Thus, we
// do our own rendering using native Win32 functions.  At least for
// now, we leave line and text rendering to gdk.

namespace {
    inline HBITMAP get_bitmap(sGbag *gb)
    {
        const GRfillType *fp = gb->get_fillpattern();
        if (fp)
            return ((HBITMAP)fp->xPixmap());
        return (0);
    }

    GdkGCValuesMask Win32GCvalues = (GdkGCValuesMask)(
        GDK_GC_FOREGROUND |
        GDK_GC_BACKGROUND);

    POINT *PTbuf;
    int PTbufSz;
}
#endif


// Draw a pixel at x, y.
//
void
GTKdraw::Pixel(int x, int y)
{
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        x -= xoff;
        y -= yoff;
    }

    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);
    HBITMAP bmap = get_bitmap(gd_gbag);
    unsigned rop = Gbag()->get_gc() == Gbag()->get_xorgc() ?
        PATINVERT : PATCOPY;
    if (bmap) {
        HBRUSH brush = CreatePatternBrush(bmap);
        brush = SelectBrush(dc, brush);

        PatBlt(dc, x, y, 1, 1, rop);

        brush = SelectBrush(dc, brush);
        if (brush)
            DeleteBrush(brush);
    }
    else
        PatBlt(dc, x, y, 1, 1, rop);
    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);

#else

#ifdef NEW_NDK
    Drawable xid = gd_dw.get_xid();
    if (xid == None)
        return;
    XDrawPoint(GC()->get_xdisplay(), xid, GC()->get_xgc(), x, y);
#else
    gdk_draw_point(gd_window, GC(), x, y);
#endif

#endif
}


// Draw multiple pixels from list.
//
void
GTKdraw::Pixels(GRmultiPt *data, int n)
{
    if (n < 1)
        return;
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    POINT *p = (POINT*)data->data();
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        if (xoff | yoff) {
            if (n > PTbufSz) {
                delete [] PTbuf;
                PTbufSz = n + 32;
                PTbuf = new POINT[PTbufSz];
            }
            for (int i = 0; i < n; i++) {
                PTbuf[i].x = p[i].x - xoff;
                PTbuf[i].y = p[i].y - yoff;
            }
            p = PTbuf;
        }
    }
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);
    HBITMAP bmap = get_bitmap(gd_gbag);

    // This seems a bit faster than SetPixelV.
    unsigned rop = Gbag()->get_gc() == Gbag()->get_xorgc() ?
        PATINVERT : PATCOPY;
    if (bmap) {
        HBRUSH brush = CreatePatternBrush(bmap);
        brush = SelectBrush(dc, brush);

        for (int i = 0; i < n; i++) {
            if (!i || p->x != (p-1)->x || p->y != (p-1)->y)
                PatBlt(dc, p->x, p->y, 1, 1, rop);
            p++;
        }

        brush = SelectBrush(dc, brush);
        if (brush)
            DeleteBrush(brush);
    }
    else {
        for (int i = 0; i < n; i++) {
            if (!i || p->x != (p-1)->x || p->y != (p-1)->y)
                PatBlt(dc, p->x, p->y, 1, 1, rop);
            p++;
        }
    }
    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);

#else

#ifdef NEW_NDK
    Drawable xid = gd_dw.get_xid();
    if (xid == None)
        return;

    if (GRmultiPt::is_short_data()) {
        XDrawPoints(GC()->get_xdisplay(), xid, GC()->get_xgc(),
            (XPoint*)data->data(), n, CoordModeOrigin);
    }
    else {
        data->data_ptr_init();
        while (n--) {
            int x = data->data_ptr_x();
            int y = data->data_ptr_y();
            Pixel(x, y);
            data->data_ptr_inc();
        }
    }
#else
    gdk_draw_points(gd_window, GC(), (GdkPoint*)data->data(), n);
#endif

#endif
}


// Draw a line.
//
void
GTKdraw::Line(int x1, int y1, int x2, int y2)
{
    /***** Example native line draw
    static GdkGCValuesMask LineGCvalues = (GdkGCValuesMask)(
        GDK_GC_FOREGROUND |
        GDK_GC_BACKGROUND |
        GDK_GC_LINE_WIDTH |
        GDK_GC_LINE_STYLE |
        GDK_GC_CAP_STYLE |
        GDK_GC_JOIN_STYLE);

    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        if (xoff | yoff) {
            x1 -= xoff;
            y1 -= yoff;
            x2 -= xoff;
            y2 -= yoff;
        }
    }
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), LineGCvalues);
    MoveToEx(dc, x1, y1, 0);
    LineTo(dc, x2, y2);
    gdk_win32_hdc_release(gd_window, GC(), LineGCvalues);
    return;
    *****/

// XXX WIN32 needs support

#ifdef NEW_NDK
    Drawable xid = gd_dw.get_xid();
#else
    Drawable xid = gdk_x11_drawable_get_xid(gd_window);
#endif
    if (xid == None)
        return;
    if (XorLineDb()) {
        // We are drawing in XOR mode, filter the Manhattan lines so
        // none is overdrawn.

        // Must draw vertical lines first to properly handle single-pixel
        // "lines".
        if (x1 == x2) {
            const llist_t *ll = XorLineDb()->add_vert(x1, y1, y2);
            while (ll) {
#ifdef NEW_NDK
                XDrawLine(GC()->get_xdisplay(), xid, GC()->get_xgc(),
                    x1, ll->vmin(), x1, ll->vmax());
#else
                gdk_draw_line(gd_window, GC(), x1, ll->vmin(), x1, ll->vmax());
#endif
                ll = ll->next();
            }
        }
        else if (y1 == y2) {
            const llist_t *ll = XorLineDb()->add_horz(y1, x1, x2);
            while (ll) {
#ifdef NEW_NDK
                XDrawLine(GC()->get_xdisplay(), xid, GC()->get_xgc(),
                    ll->vmin(), y1, ll->vmax(), y1);
#else
                gdk_draw_line(gd_window, GC(), ll->vmin(), y1, ll->vmax(), y1);
#endif
                ll = ll->next();
            }
        }
        else {
            const nmllist_t *ll = XorLineDb()->add_nm(x1, y1, x2, y2);
            while (ll) {
#ifdef NEW_NDK
                XDrawLine(GC()->get_xdisplay(), xid, GC()->get_xgc(),
                    ll->x1(), ll->y1(), ll->x2(), ll->y2());
#else
                gdk_draw_line(gd_window, GC(),
                    ll->x1(), ll->y1(), ll->x2(), ll->y2());
#endif
                ll = ll->next();
            }
        }
        return;
    }

#ifdef NEW_NDK
    XDrawLine(GC()->get_xdisplay(), xid, GC()->get_xgc(), x1, y1, x2, y2);
#else
    gdk_draw_line(gd_window, GC(), x1, y1, x2, y2);
#endif
}


// Draw a path.
//
void
GTKdraw::PolyLine(GRmultiPt *p, int n)
{
    if (n < 2)
        return;
    if (XorLineDb()) {
        n--;
        p->data_ptr_init();
        while (n--) {
            int x = p->data_ptr_x();
            int y = p->data_ptr_y();
            p->data_ptr_inc();
            if (abs(p->data_ptr_x() - x) <= 1 &&
                    abs(p->data_ptr_y() - y) <= 1) {
                // Keep tiny features from disappearing.
                Line(p->data_ptr_x(), p->data_ptr_y(),
                    p->data_ptr_x(), p->data_ptr_y());
            }
            else
                Line(x, y, p->data_ptr_x(), p->data_ptr_y());
        }
        return;
    }

#ifdef NEW_NDK
    Drawable xid = gd_dw.get_xid();
    if (xid == None)
        return;
    if (GRmultiPt::is_short_data()) {
        XDrawLines(GC()->get_xdisplay(), xid, GC()->get_xgc(),
            (XPoint*)p->data(), n, CoordModeOrigin);
    }
    else {
        n--;
        p->data_ptr_init();
        while (n--) {
            int x = p->data_ptr_x();
            int y = p->data_ptr_y();
            p->data_ptr_inc();
            if (abs(p->data_ptr_x() - x) <= 1 &&
                    abs(p->data_ptr_y() - y) <= 1) {
                // Keep tiny features from disappearing.
                Line(p->data_ptr_x(), p->data_ptr_y(),
                    p->data_ptr_x(), p->data_ptr_y());
            }
            else
                Line(x, y, p->data_ptr_x(), p->data_ptr_y());
        }
    }
#else
    gdk_draw_lines(gd_window, GC(), (GdkPoint*)p->data(), n);
#endif
}


// Draw a collection of lines.
//
void
GTKdraw::Lines(GRmultiPt *p, int n)
{
    if (n < 1)
        return;
    if (XorLineDb()) {
        p->data_ptr_init();
        while (n--) {
            int x = p->data_ptr_x();
            int y = p->data_ptr_y();
            p->data_ptr_inc();
            Line(x, y, p->data_ptr_x(), p->data_ptr_y());
            p->data_ptr_inc();
        }
        return;
    }

#ifdef NEW_NDK
    Drawable xid = gd_dw.get_xid();
    if (xid == None)
        return;
    if (GRmultiPt::is_short_data()) {
        XDrawSegments(GC()->get_xdisplay(), xid, GC()->get_xgc(),
            (XSegment*)p->data(), n);
    }
    else {
        p->data_ptr_init();
        while (n--) {
            int x = p->data_ptr_x();
            int y = p->data_ptr_y();
            p->data_ptr_inc();
            Line(x, y, p->data_ptr_x(), p->data_ptr_y());
            p->data_ptr_inc();
        }
    }
#else
    gdk_draw_segments(gd_window, GC(), (GdkSegment*)p->data(), n);
#endif
}


// Draw a filled box.
//
void
GTKdraw::Box(int x1, int y1, int x2, int y2)
{
    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
    }
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        x1 -= xoff;
        x2 -= xoff;
        y1 -= yoff;
        y2 -= yoff;
    }

    x2++;
    y2++;
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);
    HBITMAP bmap = get_bitmap(gd_gbag);
    if (bmap) {
        HBRUSH brush = CreatePatternBrush(bmap);
        brush = SelectBrush(dc, brush);

        COLORREF fg = SetTextColor(dc, 0);
        COLORREF bg = SetBkColor(dc, RGB(255, 255, 255));
        // D <- P & D
        PatBlt(dc, x1, y1, x2-x1, y2-y1, 0xa000c9);
        SetBkColor(dc, 0);
        SetTextColor(dc, fg);
        // D <- P | D
        PatBlt(dc, x1, y1, x2-x1, y2-y1, 0xfa0089);
        SetBkColor(dc, bg);

        brush = SelectBrush(dc, brush);
        if (brush)
            DeleteBrush(brush);
    }
    else
        PatBlt(dc, x1, y1, x2-x1, y2-y1, PATCOPY);
    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);

#else

#ifdef NEW_NDK
    Drawable xid = gd_dw.get_xid();
    if (xid == None)
        return;
    XFillRectangle(GC()->get_xdisplay(), xid, GC()->get_xgc(),
        x1, y1, x2-x1 + 1, y2-y1 + 1);
#else
    gdk_draw_rectangle(gd_window, GC(), true, x1, y1, x2-x1 + 1, y2-y1 + 1);
#endif

#endif
}


// Draw a collection of filled boxes.
//
void
GTKdraw::Boxes(GRmultiPt *data, int n)
{
    if (n < 1)
        return;
    // order: x, y, width, height
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    POINT *points = (POINT*)data->data();
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        if (xoff | yoff) {
            int two_n = n + n;
            if (two_n > PTbufSz) {
                delete [] PTbuf;
                PTbufSz = two_n + 32;
                PTbuf = new POINT[PTbufSz];
            }
            for (int i = 0; i < two_n; i += 2) {
                PTbuf[i].x = points[i].x - xoff;
                PTbuf[i].y = points[i].y - yoff;
            }
            points = PTbuf;
        }
    }
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);
    HBITMAP bmap = get_bitmap(gd_gbag);
    if (bmap) {
        HBRUSH brush = CreatePatternBrush(bmap);
        brush = SelectBrush(dc, brush);

        COLORREF fg = SetTextColor(dc, 0);
        COLORREF bg = SetBkColor(dc, RGB(255, 255, 255));
        POINT *p = points;
        for (int i = 0; i < n; i++) {
            // D <- P & D
            PatBlt(dc, p->x, p->y, (p+1)->x, (p+1)->y, 0xa000c9);
            p += 2;
        }
        SetBkColor(dc, 0);
        SetTextColor(dc, fg);
        p = points;
        for (int i = 0; i < n; i++) {
            // D <- P | D
            PatBlt(dc, p->x, p->y, (p+1)->x, (p+1)->y, 0xfa0089);
            p += 2;
        }
        SetBkColor(dc, bg);

        brush = SelectBrush(dc, brush);
        if (brush)
            DeleteBrush(brush);
    }
    else {
        POINT *p = points;
        for (int i = 0; i < n; i++) {
            PatBlt(dc, p->x, p->y, (p+1)->x, (p+1)->y, PATCOPY);
            p += 2;
        }
    }
    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);

#else

#ifdef NEW_NDK
    Drawable xid = gd_dw.get_xid();
    if (xid == None)
        return;
    if (GRmultiPt::is_short_data()) {
        XFillRectangles(GC()->get_xdisplay(), xid, GC()->get_xgc(),
            (XRectangle*)data->data(), n);
    }
    else {
        data->data_ptr_init();
        while (n--) {
            int x = data->data_ptr_x();
            int y = data->data_ptr_y();
            data->data_ptr_inc();
            Box(x, y, x + data->data_ptr_x() - 1, y + data->data_ptr_y() - 1);
            data->data_ptr_inc();
        }
    }
#else
    data->data_ptr_init();
    while (n--) {
        int x = data->data_ptr_x();
        int y = data->data_ptr_y();
        data->data_ptr_inc();
        Box(x, y, x + data->data_ptr_x() - 1, y + data->data_ptr_y() - 1);
        data->data_ptr_inc();
    }
#endif

#endif
}


// Draw an arc.
//
void
GTKdraw::Arc(int x0, int y0, int rx, int ry, double theta1, double theta2)
{
    if (theta1 >= theta2)
        theta2 = 2 * M_PI + theta2;
    int t1 = (int)(64 * (180.0 / M_PI) * theta1);
    int t2 = (int)(64 * (180.0 / M_PI) * theta2 - t1);
    if (t2 == 0)
        return;
    if (rx <= 0 || ry <= 0)
        return;
    int dx = 2*rx;
    int dy = 2*ry;
#ifdef NEW_NDK
    Drawable xid = gd_dw.get_xid();
    if (xid == None)
        return;
    XDrawArc(GC()->get_xdisplay(), xid, GC()->get_xgc(),
        x0 - rx, y0 - ry, dx, dy, t1, t2);
#else
    gdk_draw_arc(gd_window, GC(), false, x0 - rx, y0 - ry, dx, dy, t1, t2);
#endif
}


// Draw a filled polygon.
//
void
GTKdraw::Polygon(GRmultiPt *data, int numv)
{
    if (numv < 4)
        return;
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    POINT *points = (POINT*)data->data();
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        if (xoff | yoff) {
            if (numv > PTbufSz) {
                delete [] PTbuf;
                PTbufSz = numv + 32;
                PTbuf = new POINT[PTbufSz];
            }
            for (int i = 0; i < numv; i++) {
                PTbuf[i].x = points[i].x - xoff;
                PTbuf[i].y = points[i].y - yoff;
            }
            points = PTbuf;
        }
    }
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);
    HBITMAP bmap = get_bitmap(gd_gbag);
    HRGN rgn = CreatePolygonRgn(points, numv, WINDING);
    if (bmap) {
        HBRUSH brush = CreatePatternBrush(bmap);
        brush = SelectBrush(dc, brush);

        COLORREF fg = SetTextColor(dc, 0);
        COLORREF bg = SetBkColor(dc, RGB(255, 255, 255));
        // D <- P & D
        SetROP2(dc, R2_MASKPEN);
        PaintRgn(dc, rgn);
        SetBkColor(dc, 0);
        SetTextColor(dc, fg);
        // D <- P | D
        SetROP2(dc, R2_MERGEPEN);
        PaintRgn(dc, rgn);
        SetBkColor(dc, bg);
        SetROP2(dc, R2_COPYPEN);

        brush = SelectBrush(dc, brush);
        if (brush)
            DeleteBrush(brush);
    }
    else
        PaintRgn(dc, rgn);
    DeleteRgn(rgn);
    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);

#else

#ifdef NEW_NDK
    Drawable xid = gd_dw.get_xid();
    if (xid == None)
        return;
    if (GRmultiPt::is_short_data()) {
        XFillPolygon(GC()->get_xdisplay(), xid, GC()->get_xgc(),
            (XPoint*)data->data(), numv, Complex, CoordModeOrigin);
    }
    else {
//XXX copy int to short
    }
#else
    gdk_draw_polygon(gd_window, GC(), true, (GdkPoint*)data->data(), numv);
#endif

#endif
}


// Draw a trapezoid.
void
GTKdraw::Zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
{
    if (yl >= yu)
        return;
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    POINT points[5];
#else
#ifdef NEW_NDK
    XPoint points[5];
#else
    GdkPoint points[5];
#endif
#endif
    int n = 0;
    points[n].x = xll;
    points[n].y = yl;
    n++;
    points[n].x = xul;
    points[n].y = yu;
    n++;
    if (xur > xul) {
        points[n].x = xur;
        points[n].y = yu;
        n++;
    }
    points[n].x = xlr;
    points[n].y = yl;
    n++;
    if (xll < xlr) {
        points[n].x = xll;
        points[n].y = yl;
        n++;
    }
    if (n < 4)
        return;

#if defined(WIN32) && defined(DIRECT_TO_GDI)
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        if (xoff | yoff) {
            for (int i = 0; i < n; i++) {
                points[i].x -= xoff;
                points[i].y -= yoff;
            }
        }
    }
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);
    HBITMAP bmap = get_bitmap(gd_gbag);
    HRGN rgn = CreatePolygonRgn(points, n, WINDING);
    if (bmap) {
        HBRUSH brush = CreatePatternBrush(bmap);
        brush = SelectBrush(dc, brush);

        COLORREF fg = SetTextColor(dc, 0);
        COLORREF bg = SetBkColor(dc, RGB(255, 255, 255));
        // D <- P & D
        SetROP2(dc, R2_MASKPEN);
        PaintRgn(dc, rgn);
        SetBkColor(dc, 0);
        SetTextColor(dc, fg);
        // D <- P | D
        SetROP2(dc, R2_MERGEPEN);
        PaintRgn(dc, rgn);
        SetBkColor(dc, bg);
        SetROP2(dc, R2_COPYPEN);

        brush = SelectBrush(dc, brush);
        if (brush)
            DeleteBrush(brush);
    }
    else
        PaintRgn(dc, rgn);
    DeleteRgn(rgn);
    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);

#else

#ifdef NEW_NDK
    Drawable xid = gd_dw.get_xid();
    if (xid == None)
        return;
    XFillPolygon(GC()->get_xdisplay(), xid, GC()->get_xgc(),
        points, n, Convex, CoordModeOrigin);
#else
    gdk_draw_polygon(gd_window, GC(), true, points, n);
#endif

#endif
}


namespace {
#ifdef NEW_NDK
    struct fixgc
    {
        fixgc(ndkGC *gc, ndkGC *xgc, unsigned int oldfg, unsigned int newfg,
            unsigned int oldfunc, unsigned int newfunc)
        {
            if (gc == xgc) {
                GdkColor clr;
                clr.pixel = newfg;
                gc->set_foreground(&clr);
            }
            gc->set_function((ndkGCfunction)newfunc);
            f_gc = gc;
            f_xgc = xgc;
            f_oldfg = oldfg;
            f_oldfunc = oldfunc;
        }

        ~fixgc()
        {
            if (f_gc == f_xgc) {
                GdkColor clr;
                clr.pixel = f_oldfg;
                f_gc->set_foreground(&clr);
            }
            f_gc->set_function((ndkGCfunction)f_oldfunc);
        }

        ndkGC *f_gc;
        ndkGC *f_xgc;
        unsigned int f_oldfg;
        unsigned int f_oldfunc;
    };

#else
    struct fixgc
    {
        fixgc(GdkGC *gc, GdkGC *xgc, unsigned int oldfg, unsigned int newfg,
            unsigned int oldfunc, unsigned int newfunc)
        {
            if (gc == xgc) {
                GdkColor clr;
                clr.pixel = newfg;
                gdk_gc_set_foreground(gc, &clr);
            }
            gdk_gc_set_function(gc, (GdkFunction)newfunc);
            f_gc = gc;
            f_xgc = xgc;
            f_oldfg = oldfg;
            f_oldfunc = oldfunc;
        }

        ~fixgc()
        {
            if (f_gc == f_xgc) {
                GdkColor clr;
                clr.pixel = f_oldfg;
                gdk_gc_set_foreground(f_gc, &clr);
            }
            gdk_gc_set_function(f_gc, (GdkFunction)f_oldfunc);
        }

        GdkGC *f_gc;
        GdkGC *f_xgc;
        unsigned int f_oldfg;
        unsigned int f_oldfunc;
    };
#endif
}


// Render text.  Go through hoops to provide rotated/mirrored
// rendering.  The x and y are the LOWER left corner of untransformed
// text.
//
// Note: Pango now handles rotated text, update this some day.
//
// This function DOES NOT support XOR drawing.  Native X drawing would
// work, Pango does not, and the rotation code manifestly requires
// straight copy.  We have to back the GC out of XOR drawing mode for
// the duration.
//
void
GTKdraw::Text(const char *text, int x, int y, int xform, int, int)
{
    if (!text || !*text)
        return;

#ifdef NEW_NDK
    if (gd_dw.get_state() == DW_NONE)
        return;
    fixgc gcfix(GC(), XorGC(), GC()->get_fg_pixel(), gd_foreg,
        GC()->get_function(), ndkGC_COPY);
#else
    GdkGCValues vals;
    gdk_gc_get_values(GC(), &vals);
    fixgc gcfix(GC(), XorGC(), vals.foreground.pixel, gd_foreg,
        vals.function, GDK_COPY);
#endif

    // We need to handle strings with embedded newlines on a single
    // line.  GTK-1 does this naturally.  With Pango, one can set
    // "single paragraph" mode to achieve this.  However, the glyph
    // used as a separator is much wider than the (monospace)
    // characters, badly breaking positioning.  We don't use this
    // mode, but instead map newlines to a unicode special character
    // that will be displayed with the correct width.

    PangoLayout *lout = gtk_widget_create_pango_layout(gd_viewport, 0);
    if (strchr(text, '\n')) {
        sLstr lstr;
        const char *t = text;
        while (*t) {
            if (*t == '\n') {
                // This is the "section sign" UTF-8 character.
                lstr.add_c(0xc2);
                lstr.add_c(0xa7);
            }
            else
                lstr.add_c(*t);
            t++;
        }
        pango_layout_set_text(lout, lstr.string(), -1);
    }
    else
        pango_layout_set_text(lout, text, -1);

    int wid, hei;
    pango_layout_get_pixel_size(lout, &wid, &hei);
    if (wid <= 0 || hei <= 0) {
        g_object_unref(lout);
        return;
    }
    y -= hei;

    if (xform & (TXTF_HJC | TXTF_HJR)) {
        if (xform & TXTF_HJR)
            x -= wid;
        else
            x -= wid/2;
    }
    if (xform & (TXTF_VJC | TXTF_VJT)) {
        if (xform & TXTF_VJT)
            y += hei;
        else
            y += hei/2;
    }

    xform &= (TXTF_ROT | TXTF_MX | TXTF_MY);


#ifdef NEW_NDK
    unsigned int bg_pixel = GC()->get_bg_pixel();
    ndkPixmap *p = new ndkPixmap(gd_dw.get_window(), wid, hei);
    p->copy_from_pango_layout(GC(), 0, 0, lout);
#else
    /*
    gdk_draw_layout(gd_window, GC(), x, y, lout);
    g_object_unref(lout);
    return;
    */

    GdkColor clr;
    GdkPixmap *p = gdk_pixmap_new(gd_window, wid, hei,
        gdk_visual_get_depth(GRX->Visual()));
    if (!p)
        return;
    unsigned int bg_pixel = vals.background.pixel;
    clr.pixel = bg_pixel;
    GdkColor tclr;
    tclr.pixel = vals.foreground.pixel;
    gdk_gc_set_foreground(GC(), &clr);
    gdk_draw_rectangle(p, GC(), true, 0, 0, wid, hei);
    gdk_gc_set_foreground(GC(), &tclr);
    gdk_draw_layout(p, GC(), 0, 0, lout);
#endif
    g_object_unref(lout);

/* XXX force to use image as test
    if (!xform || xform == 14) {
        // 0 no rotation, 14 MX MY R180
#ifdef NEW_NDK
        p->copy_to_drawable(&gd_dw, GC(), 0, 0, x, y, wid, hei);
        p->dec_ref();
#else
        copy_x11_pixmap_to_drawable(gd_window, GC(), p, 0, 0, x, y,
            wid, hei);
        gdk_pixmap_unref(p);
#endif
        return;
    }
    */

#ifdef NEW_NDK
    ndkImage *im = new ndkImage(p, 0, 0, wid, hei);
    p->dec_ref();

    // Create a second image for the transformed copy.  This will contain
    // the background pixels from the rendering area.
    //
    ndkImage *im1;
    if (xform & 1) {
        // rotation
        p = (ndkPixmap*)GetRegion(x, y, hei, wid);
        im1 = new ndkImage(p, 0, 0, hei, wid);
        p->dec_ref();
    }
    else {
        p = (ndkPixmap*)GetRegion(x, y, wid, hei);
        im1 = new ndkImage(p, 0, 0, wid, hei);
        p->dec_ref();
    }

    // Transform and copy the pixels, only those that are non-background.

    int i, j;
    unsigned long px;
    switch (xform) {
    case 0:  // R0
    case 14: // MX MY R180
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = im->get_pixel(i, j);
                if (px != bg_pixel)
                    im1->put_pixel(i, j, px);
            }
        }
        break;

    case 1:  // R90
    case 15: // MX MY R270
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = im->get_pixel(i, j);
                if (px != bg_pixel)
                    im1->put_pixel(j, wid-i-1, px);
            }
        }
        y += wid;
        break;

    case 2:  // R180
    case 12: // MX MY
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = im->get_pixel(i, j);
                if (px != bg_pixel)
                    im1->put_pixel(wid-i-1, hei-j-1, px);
            }
        }
        y += hei;
        break;

    case 3:  // R270
    case 13: // MX MY R90
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = im->get_pixel(i, j);
                if (px != bg_pixel)
                    im1->put_pixel(hei-j-1, i, px);
            }
        }
        y += wid;
        break;

    case 4:  // MY
    case 10: // MX R180
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = im->get_pixel(i, j);
                if (px != bg_pixel)
                    im1->put_pixel(i, hei-j-1, px);
            }
        }
        y += hei;
        break;

    case 5:  // MY R90
    case 11: // MX R270
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = im->get_pixel(i, j);
                if (px != bg_pixel)
                    im1->put_pixel(j, i, px);
            }
        }
        y += wid;
        break;

    case 6:  // MY R180
    case 8:  // MX
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = im->get_pixel(i, j);
                if (px != bg_pixel)
                    im1->put_pixel(wid-i-1, j, px);
            }
        }
        y += hei;
        break;

    case 7:  // MY R270
    case 9:  // MX R90
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = im->get_pixel(i, j);
                if (px != bg_pixel)
                    im1->put_pixel(hei-j-1, wid-i-1, px);
            }
        }
        y += wid;
        break;
    }
    delete im;

    if (xform & 1)
        // rotation
        im1->copy_to_drawable(&gd_dw, GC(), 0, 0, x, y, hei, wid);
    else
        im1->copy_to_drawable(&gd_dw, GC(), 0, 0, x, y, wid, hei);
    delete im1;

#else
    GdkImage *im = gdk_image_get(p, 0, 0, wid, hei);
    gdk_pixmap_unref(p);

    // Create a second image for the transformed copy.  This will contain
    // the background pixels from the rendering area.
    //
    GdkImage *im1;
    if (xform & 1) {
        // rotation
        p = (GdkPixmap*)GetRegion(x, y, hei, wid);
        im1 = gdk_image_get(p, 0, 0, hei, wid);
        gdk_pixmap_unref(p);
    }
    else {
        p = (GdkPixmap*)GetRegion(x, y, wid, hei);
        im1 = gdk_image_get(p, 0, 0, wid, hei);
        gdk_pixmap_unref(p);
    }

    // Transform and copy the pixels, only those that are non-background.

    int i, j;
    unsigned long px;
    switch (xform) {
    case 0:  // R0
    case 14: // MX MY R180
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, i, j, px);
            }
        }
        break;

    case 1:  // R90
    case 15: // MX MY R270
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, j, wid-i-1, px);
            }
        }
        y += wid;
        break;

    case 2:  // R180
    case 12: // MX MY
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, wid-i-1, hei-j-1, px);
            }
        }
        y += hei;
        break;

    case 3:  // R270
    case 13: // MX MY R90
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, hei-j-1, i, px);
            }
        }
        y += wid;
        break;

    case 4:  // MY
    case 10: // MX R180
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, i, hei-j-1, px);
            }
        }
        y += hei;
        break;

    case 5:  // MY R90
    case 11: // MX R270
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, j, i, px);
            }
        }
        y += wid;
        break;

    case 6:  // MY R180
    case 8:  // MX
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, wid-i-1, j, px);
            }
        }
        y += hei;
        break;

    case 7:  // MY R270
    case 9:  // MX R90
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, hei-j-1, wid-i-1, px);
            }
        }
        y += wid;
        break;
    }

    gdk_image_destroy(im);
    if (xform & 1)
        // rotation
        gdk_draw_image(gd_window, GC(), im1, 0, 0, x, y, hei, wid);
    else
        gdk_draw_image(gd_window, GC(), im1, 0, 0, x, y, wid, hei);
    gdk_image_destroy(im1);

#endif
}


// Return the width/height of text.  If text is 0, return
// the max bounds of any character.
//
void
GTKdraw::TextExtent(const char *text, int *wid, int *hei)
{
    if (!text || !*text)
        text = "M";
    PangoLayout *pl;
    PangoContext *pc = 0;
    if (GTK_IS_WIDGET(gd_viewport)) {
        pl = gtk_widget_create_pango_layout(gd_viewport, text);
        GtkRcStyle *rc = gtk_widget_get_modifier_style(gd_viewport);
        if (rc->font_desc)
            pango_layout_set_font_description(pl, rc->font_desc);
    }
    else {
        pc = gdk_pango_context_get();
        pl = pango_layout_new(pc);
        PangoFontDescription *pfd =
            pango_font_description_from_string(FC.getName(FNT_SCREEN));
        pango_layout_set_font_description(pl, pfd);
        pango_font_description_free(pfd);
        pango_layout_set_text(pl, text, -1);
    }

    int tw, th;
    pango_layout_get_pixel_size(pl, &tw, &th);
    g_object_unref(pl);
    if (pc)
        g_object_unref(pc);
    if (wid)
        *wid = tw;
    if (hei)
        *hei = th;
}


// Move the pointer by x, y relative to current position, if absolute
// is false.  If true, move to given location.
//
void
GTKdraw::MovePointer(int x, int y, bool absolute)
{
    // Called with 0,0 this redraws ghost objects.
#ifdef NEW_NDK
    GdkWindow *window = gd_dw.get_window();
#else
    GdkWindow *window = gd_window;
#endif
    if (window) {
        int x0, y0;
        GdkScreen *screen;
        GdkDisplay *display = gdk_display_get_default();
        gdk_display_get_pointer(display, &screen, &x0, &y0, 0);
        if (absolute)
            gdk_window_get_root_origin(window, &x0, &y0);
        x += x0;
        y += y0;
        gdk_display_warp_pointer(display, screen, x, y);
    }
}


// Set x, y, and state of pointer referenced to window of context.
//
void
GTKdraw::QueryPointer(int *x, int *y, unsigned *state)
{
#ifdef NEW_NDK
    GdkWindow *window = gd_dw.get_window();
#else
    GdkWindow *window = gd_window;
#endif
    int tx = 0, ty = 0;
    unsigned int ts = 0;
    if (window)
        gdk_window_get_pointer(window, &tx, &ty, (GdkModifierType*)&ts);
    if (x)
        *x = tx;
    if (y)
        *y = ty;
    if (state)
        *state = ts;
}


// Define a new rgb value for pixel (if read/write cell) or return a
// new pixel with a matching color.
//
void
GTKdraw::DefineColor(int *pixel, int red, int green, int blue)
{
    if (!GC())
        return;
    GdkColor newcolor;
    newcolor.red   = (red   << 8);
    newcolor.green = (green << 8);
    newcolor.blue  = (blue  << 8);
    newcolor.pixel = 0;
#ifdef NEW_NDK
    GdkWindow *window = gd_dw.get_window();
    if (!window)
        window = gdk_get_default_root_window();
    GdkVisual *visual = gdk_window_get_visual(window);
    GC()->query_pixel(&newcolor, visual);
    GC()->set_foreground(&newcolor);
    *pixel = newcolor.pixel;
#else
    if (gdk_colormap_alloc_color(GRX->Colormap(), &newcolor, false, true))
        *pixel = newcolor.pixel;
    else
        *pixel = 0;
    gdk_gc_set_foreground(GC(), &newcolor);
#endif
}


// Set the window background in the GC's.
//
void
GTKdraw::SetBackground(int pixel)
{
    gd_backg = pixel;
    GdkColor clr;
    clr.pixel = pixel;
#ifdef NEW_NDK
    GC()->set_background(&clr);
    XorGC()->set_background(&clr);
    clr.pixel = gd_xor_fg ^ pixel;
    XorGC()->set_foreground(&clr);
#else
    gdk_gc_set_background(GC(), &clr);
    gdk_gc_set_background(XorGC(), &clr);
    clr.pixel = gd_xor_fg ^ pixel;
    gdk_gc_set_foreground(XorGC(), &clr);
#endif
}


// Change the drawing window background color.
//
void
GTKdraw::SetWindowBackground(int pixel)
{
#ifdef NEW_NDK
    GdkWindow *window = gd_dw.get_window();
    if (window) {
        GdkColor clr;
        clr.pixel = pixel;
        gdk_window_set_background(window, &clr);
    }
#else
    if (GDK_IS_WINDOW(gd_window)) {
        GdkColor clr;
        clr.pixel = pixel;
        gdk_window_set_background(gd_window, &clr);
    }
#endif
}


// Set the color used for ghost drawing.
//
void
GTKdraw::SetGhostColor(int pixel)
{
    gd_xor_fg = pixel;
    GdkColor clr;
    clr.pixel = gd_xor_fg ^ gd_backg;
#ifdef NEW_NDK
    XorGC()->set_foreground(&clr);
#else
    gdk_gc_set_foreground(XorGC(), &clr);
#endif
}


// Set the current foreground color.
//
void
GTKdraw::SetColor(int pixel)
{
    if (GC() == XorGC())
        return;
    GdkColor clr;
    clr.pixel = pixel;
#ifdef NEW_NDK
    GC()->set_foreground(&clr);
#else
    gdk_gc_set_foreground(GC(), &clr);
#endif
}


// Set the current linestyle.
//
void
GTKdraw::SetLinestyle(const GRlineType *lineptr)
{
#ifdef NEW_NDK
    if (!lineptr || !lineptr->mask || lineptr->mask == -1) {
        GC()->set_line_attributes(0, ndkGC_LINE_SOLID, ndkGC_CAP_BUTT,
            ndkGC_JOIN_MITER);
        return;
    }
    GC()->set_line_attributes(0, ndkGC_LINE_ON_OFF_DASH, ndkGC_CAP_BUTT,
        ndkGC_JOIN_MITER);

    GC()->set_dashes(lineptr->offset, (unsigned char*)lineptr->dashes,
        lineptr->length);
#else
    if (!lineptr || !lineptr->mask || lineptr->mask == -1) {
        gdk_gc_set_line_attributes(GC(), 0, GDK_LINE_SOLID,
            GDK_CAP_BUTT, GDK_JOIN_MITER);
        return;
    }
    gdk_gc_set_line_attributes(GC(), 0, GDK_LINE_ON_OFF_DASH,
         GDK_CAP_BUTT, GDK_JOIN_MITER);

    gdk_gc_set_dashes(GC(), lineptr->offset,
        (signed char*)lineptr->dashes, lineptr->length);
#endif
}


#ifdef WIN32
namespace {
    // Reverse bit order and complement.
    //
    unsigned int
    revnotbits(unsigned char c)
    {
        unsigned char out = 0;
        for (int i = 0;;) {
            if (!(c & 1))
                out |= 1;
            i++;
            if (i == 8)
                break;
            c >>= 1;
            out <<= 1;
        }
        return (out);
    }
}
#endif


// Create a new pixmap for the fill pattern.
//
void
GTKdraw::DefineFillpattern(GRfillType *fillp)
{
    if (!fillp)
        return;
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    if (fillp->xPixmap()) {
        DeleteBitmap((HBITMAP)fillp->xPixmap());
        fillp->setXpixmap(0);
    }
    if (fillp->hasMap()) {
        // 0 -> text fg color, 1 -> text bg color, so invert map pixels
        int bpl = (fillp->nX() + 7)/8;
        unsigned char *map = fillp->newBitmap();
        unsigned short *invmap =
            new unsigned short[fillp->nY()*(bpl > 2 ? 2:1)];
        if (fillp->nX() <= 16) {
            for (unsigned int i = 0; i < fillp->nY(); i++) {
                unsigned short c = revnotbits(map[i*bpl]);
                if (bpl > 1)
                    c |= revnotbits(map[i*bpl + 1]) << 8;
                invmap[i] = c;
            }
        }
        else {
            for (unsigned int i = 0; i < fillp->nY(); i++) {
                unsigned short c = revnotbits(map[i*bpl]);
                c |= revnotbits(map[i*bpl + 1]) << 8;
                invmap[2*i] = c;
                c = revnotbits(map[i*bpl + 2]);
                if (bpl > 3)
                    c |= revnotbits(map[i*bpl + 3]) << 8;
                invmap[2*i + 1] = c;
            }
        }
        fillp->setXpixmap((GRfillData)CreateBitmap(fillp->nX(), fillp->nY(),
            1, 1, invmap));
        delete [] invmap;
        delete [] map;
    }
#else

#ifdef NEW_NDK
    if (fillp->xPixmap()) {
        ((ndkPixmap*)fillp->xPixmap())->dec_ref();
        fillp->setXpixmap(0);
    }
    if (fillp->hasMap()) {
        unsigned char *map = fillp->newBitmap();
        GdkWindow *window = gd_dw.get_window();
        fillp->setXpixmap((GRfillData)new ndkPixmap(window,
            (char*)map, fillp->nX(), fillp->nY()));
        delete [] map;
    }
#else
    if (fillp->xPixmap()) {
        gdk_pixmap_unref((GdkPixmap*)fillp->xPixmap());
        fillp->setXpixmap(0);
    }
    if (fillp->hasMap()) {
        unsigned char *map = fillp->newBitmap();
        fillp->setXpixmap((GRfillData)gdk_bitmap_create_from_data(gd_window,
            (char*)map, fillp->nX(), fillp->nY()));
        delete [] map;
    }
#endif

#endif
}


// Set the current fill pattern.
//
void
GTKdraw::SetFillpattern(const GRfillType *fillp)
{
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    // Alas, gdk-win32 currently does not support stippled drawing,
    // which apparently got lost in the switch to cairo.  So, we roll
    // our own.

    gd_gbag->set_fillpattern(fillp);

#else

#ifdef NEW_NDK
    if (!fillp || !fillp->xPixmap())
        GC()->set_fill(ndkGC_SOLID);
    else {
        GC()->set_stipple((ndkPixmap*)fillp->xPixmap());
        GC()->set_fill(ndkGC_STIPPLED);
    }
#else
    if (!fillp || !fillp->xPixmap())
        gdk_gc_set_fill(GC(), GDK_SOLID);
    else {
        gdk_gc_set_stipple(GC(), (GdkPixmap*)fillp->xPixmap());
        gdk_gc_set_fill(GC(), GDK_STIPPLED);
    }
#endif

#endif
}


// Update the display.
//
void
GTKdraw::Update()
{
#ifdef WITH_QUARTZ
    // This is a horrible thing that forces the broken Quartz back end
    // to actually draw something.

    struct myrect : public GdkRectangle
    {
        myrect()
            {
                x = y = 0;
                width = height = 1; 
            }
    };
    static myrect onepixrect;

    if (gd_viewport && GDK_IS_WINDOW(gd_viewport->window))
        gdk_window_invalidate_rect(gd_viewport->window, &onepixrect, false);

    // Equivalent to above but slightly less efficient.
    // if (gd_viewport && GDK_IS_WINDOW(gd_viewport->window))
    //     gtk_widget_queue_draw_area(gd_viewport, 0, 0, 1, 1);
#endif
    gdk_flush();
}


void
GTKdraw::Input(int *keyret, int *butret, int *xret, int *yret)
{
    *keyret = *butret = 0;
    *xret = *yret = 0;
#ifdef NEW_NDK
    GdkWindow *window = gd_dw.get_window();
#else
    GdkWindow *window = gd_window;
#endif
    if (!window)
        return;
    for (;;) {
        GdkEvent *ev = gdk_event_get();
        if (ev) {
            if (ev->type == GDK_BUTTON_PRESS && ev->button.window == window) {
                *butret = ev->button.button;
                *xret = (int)ev->button.x;
                *yret = (int)ev->button.y;
                gdk_event_free(ev);
                break;
            }
            if (ev->type == GDK_KEY_PRESS && ev->key.window == window) {
                if (ev->key.string) {
                    *keyret = *ev->key.string;
                    *xret = 0;
                    *yret = 0;
                    gdk_event_free(ev);
                    break;
                }
            }
            gtk_main_do_event(ev);
            gdk_event_free(ev);
        }
    }
    UndrawGhost(true);
}


// Switch to/from ghost (xor) or highlight/unhighlight drawing context.
// Highlight/unhighlight works only with dual-plane color cells.  It is
// essential to call this with GRxNone after each mode change, due to
// the static storage.
//
void
GTKdraw::SetXOR(int val)
{
#ifdef NEW_NDK
    switch (val) {
    case GRxNone:
        XorGC()->set_function(ndkGC_XOR);
        gd_gbag->set_xor(false);
        break;
    case GRxXor:
        gd_gbag->set_xor(true);
        break;
    case GRxHlite:
        XorGC()->set_function(ndkGC_OR);
        gd_gbag->set_xor(true);
        break;
    case GRxUnhlite:
        XorGC()->set_function(ndkGC_AND_INVERT);
        gd_gbag->set_xor(true);
        break;
    }
#else
    switch (val) {
    case GRxNone:
        gdk_gc_set_function(XorGC(), GDK_XOR);
        gd_gbag->set_xor(false);
        break;
    case GRxXor:
        gd_gbag->set_xor(true);
        break;
    case GRxHlite:
        gdk_gc_set_function(XorGC(), GDK_OR);
        gd_gbag->set_xor(true);
        break;
    case GRxUnhlite:
        gdk_gc_set_function(XorGC(), GDK_AND_INVERT);
        gd_gbag->set_xor(true);
        break;
    }
#endif
}


// Show a glyph (from the list).
//

#define GlyphWidth 7

namespace {
    struct stmap
    {
#ifdef WIN32
        HBITMAP pmap;
#else
#ifdef NEW_NDK
        ndkPixmap *pmap;
#else
        GdkPixmap *pmap;
#endif
#endif
        char bits[GlyphWidth];
    };
    stmap glyphs[] =
    {
       { 0, {0x00, 0x1c, 0x22, 0x22, 0x22, 0x1c, 0x00} }, // circle
       { 0, {0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41} }, // cross (x)
       { 0, {0x08, 0x14, 0x22, 0x41, 0x22, 0x14, 0x08} }, // diamond
       { 0, {0x08, 0x14, 0x14, 0x22, 0x22, 0x41, 0x7f} }, // triangle
       { 0, {0x7f, 0x41, 0x22, 0x22, 0x14, 0x14, 0x08} }, // inverted triangle
       { 0, {0x7f, 0x41, 0x41, 0x41, 0x41, 0x41, 0x7f} }, // square
       { 0, {0x00, 0x00, 0x1c, 0x14, 0x1c, 0x00, 0x00} }, // dot
       { 0, {0x08, 0x08, 0x08, 0x7f, 0x08, 0x08, 0x08} }, // cross (+)
    };
}


void
GTKdraw::ShowGlyph(int gnum, int x, int y)
{
    x -= GlyphWidth/2;
    y -= GlyphWidth/2;
    gnum = gnum % (sizeof(glyphs)/sizeof(stmap));
    stmap *st = &glyphs[gnum];
#ifdef WIN32
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        x -= xoff;
        y -= yoff;
    }
    if (!st->pmap) {
        unsigned short invmap[8];
        for (int i = 0; i < GlyphWidth; i++)
            invmap[i] = revnotbits(st->bits[i]);
        invmap[7] = 0xffff;
        st->pmap = CreateBitmap(8, 8, 1, 1, (char*)invmap);
    }
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);

    HBRUSH brush = CreatePatternBrush(st->pmap);
    brush = SelectBrush(dc, brush);
    SetBrushOrgEx(dc, x, y, 0);

    COLORREF fg = SetTextColor(dc, 0);
    COLORREF bg = SetBkColor(dc, RGB(255, 255, 255));
    // D <- P & D
    PatBlt(dc, x, y, 8, 8, 0xa000c9);
    SetBkColor(dc, 0);
    SetTextColor(dc, fg);
    // D <- P | D
    PatBlt(dc, x, y, 8, 8, 0xfa0089);
    SetBkColor(dc, bg);

    brush = SelectBrush(dc, brush);
    if (brush)
        DeleteBrush(brush);

    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);

#else

#ifdef NEW_NDK
    if (!st->pmap) {
        GdkWindow *window = gd_dw.get_window();
        st->pmap = new ndkPixmap(window, (char*)st->bits,
            GlyphWidth, GlyphWidth);
    }
    GC()->set_stipple(st->pmap);
    GC()->set_fill(ndkGC_STIPPLED);
    GC()->set_ts_origin(x, y);
    Box(x, y, x + GlyphWidth - 1, y + GlyphWidth - 1);
    GC()->set_ts_origin(0, 0);
    GC()->set_fill(ndkGC_SOLID);
#else
    if (!st->pmap) {
        st->pmap = gdk_bitmap_create_from_data(gd_window, (char*)st->bits,
            GlyphWidth, GlyphWidth);
    }
    gdk_gc_set_stipple(GC(), st->pmap);
    gdk_gc_set_fill(GC(), GDK_STIPPLED);
    gdk_gc_set_ts_origin(GC(), x, y);
    gdk_draw_rectangle(gd_window, GC(), true, x, y, GlyphWidth, GlyphWidth);
    gdk_gc_set_ts_origin(GC(), 0, 0);
    gdk_gc_set_fill(GC(), GDK_SOLID);
#endif

#endif
}


GRobject
GTKdraw::GetRegion(int x, int y, int wid, int hei)
{
#ifdef NEW_NDK
    GdkWindow *window = gd_dw.get_window();
    ndkPixmap *pm = new ndkPixmap(window, wid, hei);
    pm->copy_from_drawable(&gd_dw, CpyGC(), x, y, 0, 0, wid, hei);
    return ((GRobject)pm);

#else
    GdkPixmap *pm = gdk_pixmap_new(gd_window, wid, hei,
        gdk_visual_get_depth(GRX->Visual()));
    gdk_window_copy_area(pm, CpyGC(), 0, 0, gd_window, x, y, wid, hei);
    return ((GRobject)pm);
#endif
}


void
GTKdraw::PutRegion(GRobject pm, int x, int y, int wid, int hei)
{
#ifdef NEW_NDK
    ((ndkPixmap*)pm)->copy_to_drawable(&gd_dw, CpyGC(), 0, 0, x, y, wid, hei);
#else
    gdk_window_copy_area(gd_window, CpyGC(), x, y, (GdkPixmap*)pm, 0, 0,
        wid, hei);
#endif
}


void
GTKdraw::FreeRegion(GRobject pm)
{
#ifdef NEW_NDK
    ((ndkPixmap*)pm)->dec_ref();
#else
    gdk_pixmap_unref((GdkPixmap*)pm);
#endif
}


void
GTKdraw::DisplayImage(const GRimage *image, int x, int y,
    int width, int height)
{
#ifdef NEW_NDK
    ndkImage *im = new ndkImage(ndkIMAGE_FASTEST, GRX->Visual(),
        width, height);

    for (int i = 0; i < height; i++) {
        int yd = i + y;
        if (yd < 0)
            continue;
        if ((unsigned int)yd >= image->height())
            break;
        for (int j = 0; j < width; j++) {
            int xd = j + x;
            if (xd < 0)
                continue;
            if ((unsigned int)xd >= image->width())
                break;
            unsigned int px = image->data()[xd + yd*image->width()];
#ifdef WITH_QUARTZ
            // Hmmmm, seems that Quartz requires byte reversal.
            unsigned int qpx;
            unsigned char *c1 = ((unsigned char*)&px) + 3;
            unsigned char *c2 = (unsigned char*)&qpx;
            *c2++ = *c1--;
            *c2++ = *c1--;
            *c2++ = *c1--;
            *c2++ = *c1--;
            im->put_pixel(j, i, qpx);
#else
            im->put_pixel(j, i, px);
#endif
        }
    }
    im->copy_to_drawable(&gd_dw, CpyGC(), 0, 0, x, y, width, height);
    delete im;

#endif
}
// End of GTKdraw functions.

