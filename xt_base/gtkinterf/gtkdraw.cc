
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

//#define XXX_GDK

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

// This all has to go to Cairo.  The gdk draw functions are not supported
// in gtk-3.  Ugh, much work ahead,

// Looks like Cairo and GTK don't support the X-windows shared memory
// extension.
//#define USE_XSHM

#include <gdk/gdkkeysyms.h>
#ifdef WITH_X11
#include "gtkx11.h"
#include <X11/Xproto.h>
#ifdef USE_XSHM
#ifdef HAVE_SHMGET
#include <X11/extensions/XShm.h>  
#endif
#endif
#endif

// Bypass gdk drawing functions for speed.
//
// Note:  Recent GTK (2.18.6) sets up a clipping context in the GC, so
// directly sharing the XGC causes display problems.
//
// We always use gdk drawing functions with GTK-2.  The direct to X
// functions work fine up to GTK-2.18 (probably), but since the GTK-2
// releases are dynamically linked, we can't count on the user having
// a compatible GTK-2 installation.
//
// #define DIRECT_TO_X

#ifdef DIRECT_TO_X
    // GRmultiPt uses short integers.
    GRmultiPt::set_short_data(true);
#endif

// Use our own Win32 drawing.  This is necessary, as currently gdk
// does not handle stippled drawing.
#define DIRECT_TO_GDI


//-----------------------------------------------------------------------------
// gtk_draw functions

gtk_draw::gtk_draw(int type)
{
    gd_viewport = 0;
    gd_window = 0;
    gd_gbag = sGbag::default_gbag(type);
    gd_backg = 0;
    gd_foreg = (unsigned int)-1;
#ifdef XXX_GDK
#else
    gd_surface = 0;
    gd_image = 0;
    gd_cr = 0;
#endif
}


gtk_draw::~gtk_draw()
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
#ifdef XXX_GDK
#else
    if (gd_cr)
        cairo_destroy(gd_cr);
    if (gd_image)
        cairo_surface_destroy(gd_image);
    if (gd_surface)
        cairo_surface_destroy(gd_surface);
#endif
}

void
gtk_draw::Halt()
{
    // Applies to hard copy drivers only.
}


// Clear the drawing window.
//
void
gtk_draw::Clear()
{
#ifdef XXX_GDK
    if (!GDK_IS_PIXMAP(gd_window)) {
        // doesn't work for pixmaps
        gdk_window_clear(gd_window);
    }
    else {
        int w, h;
        gdk_drawable_get_size(gd_window, &w, &h);
        gdk_draw_rectangle(gd_window, GC(), true, 0, 0, w, h);
    }
#else
    gdk_window_clear(gd_window);
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
gtk_draw::Pixel(int x, int y)
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
#ifdef DIRECT_TO_X
    XDrawPoint(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()), x, y);
#else
#ifdef XXX_GDK
    gdk_draw_point(gd_window, GC(), x, y);
#else
    cairo_rectangle(gd_cr, x, y, 1, 1);
    cairo_fill(gd_cr);
#endif
#endif
#endif
}


// Draw multiple pixels from list.
//
void
gtk_draw::Pixels(GRmultiPt *data, int n)
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
#ifdef DIRECT_TO_X
    XDrawPoints(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        (XPoint*)data->data(), n, CoordModeOrigin);
#else
#ifdef XXX_GDK
    gdk_draw_points(gd_window, GC(), (GdkPoint*)data->data(), n);
#else
    data->data_ptr_init();
    while (n--) {
        cairo_rectangle(gd_cr, data->data_ptr_x(), data->data_ptr_y(), 1, 1);
        data->data_ptr_inc();
    }
    cairo_fill(gd_cr);
#endif

#endif
#endif
}


// Draw a line.
//
void
gtk_draw::Line(int x1, int y1, int x2, int y2)
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

    if (XorLineDb()) {
        // We are drawing in XOR mode, filter the Manhattan lines so
        // none is overdrawn.

        // Must draw vertical lines first to properly handle single-pixel
        // "lines".
        if (x1 == x2) {
            const llist_t *ll = XorLineDb()->add_vert(x1, y1, y2);
            while (ll) {
#ifdef DIRECT_TO_X
                XDrawLine(gr_x_display(), gr_x_window(gd_window),
                    gr_x_gc(GC()), x1, ll->vmin(), x1, ll->vmax());
#else
#ifdef XXX_GDK
                gdk_draw_line(gd_window, GC(), x1, ll->vmin(), x1, ll->vmax());
#else
                cairo_move_to(gd_cr, x1, ll->vmin());
                cairo_line_to(gd_cr, x1, ll->vmax());
                cairo_stroke(gd_cr);
#endif
#endif
                ll = ll->next();
            }
        }
        else if (y1 == y2) {
            const llist_t *ll = XorLineDb()->add_horz(y1, x1, x2);
            while (ll) {
#ifdef DIRECT_TO_X
                XDrawLine(gr_x_display(), gr_x_window(gd_window),
                    gr_x_gc(GC()), ll->vmin(), y1, ll->vmax(), y1);
#else
#ifdef XXX_GDK
                gdk_draw_line(gd_window, GC(), ll->vmin(), y1, ll->vmax(), y1);
#else
                cairo_move_to(gd_cr, ll->vmin(), y1);
                cairo_line_to(gd_cr, ll->vmax(), y1);
                cairo_stroke(gd_cr);
#endif
#endif
                ll = ll->next();
            }
        }
        else {
            const nmllist_t *ll = XorLineDb()->add_nm(x1, y1, x2, y2);
            while (ll) {
#ifdef DIRECT_TO_X
                XDrawLine(gr_x_display(), gr_x_window(gd_window),
                    gr_x_gc(GC()), ll->x1(), ll->y1(), ll->x2(), ll->y2());
#else
#ifdef XXX_GDK
                gdk_draw_line(gd_window, GC(),
                    ll->x1(), ll->y1(), ll->x2(), ll->y2());
#else
                cairo_move_to(gd_cr, ll->x1(), ll->y1());
                cairo_line_to(gd_cr, ll->x2(), ll->y2());
                cairo_stroke(gd_cr);
#endif
#endif
                ll = ll->next();
            }
        }
        return;
    }

#ifdef DIRECT_TO_X
    XDrawLine(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        x1, y1, x2, y2);
#else
#ifdef XXX_GDK
    gdk_draw_line(gd_window, GC(), x1, y1, x2, y2);
#else
    cairo_move_to(gd_cr, x1, y1);
    cairo_line_to(gd_cr, x2, y2);
    cairo_stroke(gd_cr);
#endif
#endif
}


// Draw a path.
//
void
gtk_draw::PolyLine(GRmultiPt *p, int n)
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

#ifdef DIRECT_TO_X
    XDrawLines(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        (XPoint*)p->data(), n, CoordModeOrigin);
#else
#ifdef XXX_GDK
    gdk_draw_lines(gd_window, GC(), (GdkPoint*)p->data(), n);
#else
    n--;
    p->data_ptr_init();
    cairo_move_to(gd_cr, p->data_ptr_x(), p->data_ptr_y());
    p->data_ptr_inc();
    while (n--) {
        cairo_line_to(gd_cr, p->data_ptr_x(), p->data_ptr_y());
        p->data_ptr_inc();
    }
    cairo_stroke(gd_cr);

#endif
#endif
}


// Draw a collection of lines.
//
void
gtk_draw::Lines(GRmultiPt *p, int n)
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

#ifdef DIRECT_TO_X
    XDrawSegments(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        (XSegment*)p->data(), n);
#else
#ifdef XXX_GDK
    gdk_draw_segments(gd_window, GC(), (GdkSegment*)p->data(), n);
#else
    p->data_ptr_init();
    while (n--) {
        cairo_move_to(gd_cr, p->data_ptr_x(), p->data_ptr_y());
        p->data_ptr_inc();
        cairo_line_to(gd_cr, p->data_ptr_x(), p->data_ptr_y());
        p->data_ptr_inc();
    }
    cairo_stroke(gd_cr);
#endif
#endif
}


// Draw a filled box.
//
void
gtk_draw::Box(int x1, int y1, int x2, int y2)
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
#ifdef DIRECT_TO_X
    XFillRectangle(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        x1, y1, x2-x1 + 1, y2-y1 + 1);
#else
#ifdef XXX_GDK
    gdk_draw_rectangle(gd_window, GC(), true, x1, y1, x2-x1 + 1, y2-y1 + 1);
#else
    cairo_rectangle(gd_cr, x1, y1, x2-x1 + 1, y2-y1 + 1);
    cairo_fill(gd_cr);
#endif

#endif
#endif
}


// Draw a collection of filled boxes.
//
void
gtk_draw::Boxes(GRmultiPt *data, int n)
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
#ifdef DIRECT_TO_X
    XFillRectangles(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        (XRectangle*)data->data(), n);
#else
    data->data_ptr_init();
    while (n--) {
        int x = data->data_ptr_x();
        int y = data->data_ptr_y();
        data->data_ptr_inc();
        cairo_rectangle(gd_cr, x, y, data->data_ptr_x(), data->data_ptr_y());
        data->data_ptr_inc();
    }
    cairo_fill(gd_cr);
#endif
#endif
}


// Draw an arc.
//
void
gtk_draw::Arc(int x0, int y0, int rx, int ry, double theta1, double theta2)
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
#ifdef DIRECT_TO_X
    XDrawArc(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        x0 - rx, y0 - ry, dx, dy, t1, t2);
#else
#ifdef XXX_GDK
    gdk_draw_arc(gd_window, GC(), false, x0 - rx, y0 - ry, dx, dy, t1, t2);
#else
    if (rx == ry)
        cairo_arc(gd_cr, x0 - rx, y0 - ry, rx, t1, t2);
    else {
        cairo_save(gd_cr);
        cairo_translate(gd_cr, x0+dx, y0+dy);
        cairo_arc(gd_cr, x0 - rx, y0 - ry, rx, t1, t2);
        cairo_scale(gd_cr, dx, dy);
        cairo_restore(gd_cr);
    }
    cairo_stroke(gd_cr);
#endif
#endif
}


// Draw a filled polygon.
//
void
gtk_draw::Polygon(GRmultiPt *data, int numv)
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
#ifdef DIRECT_TO_X
    XFillPolygon(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        (XPoint*)data->data(), numv, Complex, CoordModeOrigin);
#else
#ifdef XXX_GDK
    gdk_draw_polygon(gd_window, GC(), true, (GdkPoint*)data->data(), numv);
#else
    data->data_ptr_init();
    cairo_move_to(gd_cr, data->data_ptr_x(), data->data_ptr_y());
    data->data_ptr_inc();
    numv--;
    while (numv--) {
        cairo_line_to(gd_cr, data->data_ptr_x(), data->data_ptr_y());
        data->data_ptr_inc();
    }
    cairo_fill(gd_cr);
#endif

#endif
#endif
}


// Draw a trapezoid.
void
gtk_draw::Zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
{
    if (yl >= yu)
        return;
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    POINT points[5];
#else
#ifdef DIRECT_TO_X
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
#ifdef DIRECT_TO_X
    XFillPolygon(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        points, n, Convex, CoordModeOrigin);
#else
#ifdef XXX_GDK
    gdk_draw_polygon(gd_window, GC(), true, points, n);
#else
    GdkPoint *p = points;
    cairo_move_to(gd_cr, p->x, p->y);
    p++;
    n--;
    while (n--) {
        cairo_line_to(gd_cr, p->x, p->y);
        p++;
    }
    cairo_fill(gd_cr);
#endif

#endif
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
gtk_draw::Text(const char *text, int x, int y, int xform, int, int)
{
    if (!text || !*text)
        return;

#ifdef XXX_GDK
    // Save the GC state.
    GdkGCValues vals;
    gdk_gc_get_values(GC(), &vals);

    if (GC() == XorGC()) {
        // Set the foreground to the true ghost-drawing color, this is
        // currently the true color xor'ed with the background.
        GdkColor clr;
        clr.pixel = gd_foreg;
        gdk_gc_set_foreground(GC(), &clr);
    }

    // Switch to copy function.
    gdk_gc_set_function(GC(), GDK_COPY);

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
        // Fix up the GC as it was.
        if (GC() == XorGC())
            gdk_gc_set_foreground(GC(), &vals.foreground);
        gdk_gc_set_function(GC(), vals.function);
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
    if (!xform || xform == 14) {
        // 0 no rotation, 14 MX MY R180

        gdk_draw_layout(gd_window, GC(), x, y, lout);
        g_object_unref(lout);

        // Fix up the GC as it was.
        if (GC() == XorGC())
            gdk_gc_set_foreground(GC(), &vals.foreground);
        gdk_gc_set_function(GC(), vals.function);
        return;
    }

    // We have to tranform.  Create a pixmap with a constant background,
    // and render the text into it.

    unsigned int bg_pixel;
    if (GC() == XorGC())
        bg_pixel = gd_foreg;
    else
        bg_pixel = vals.foreground.pixel;
    if (bg_pixel == 0) {
        GdkColor c;
        if (gdk_color_white(GRX->Colormap(), &c))
            bg_pixel = c.pixel;
        else
            bg_pixel = -1;
    }
    else {
        GdkColor c;
        if (gdk_color_black(GRX->Colormap(), &c))
            bg_pixel = c.pixel;
        else
            bg_pixel = 0;
    }

    GdkPixmap *p = gdk_pixmap_new(gd_window, wid, hei,
        gdk_visual_get_depth(GRX->Visual()));
    if (p) {
        GdkColor clr;
        clr.pixel = bg_pixel;
        gdk_gc_set_foreground(GC(), &clr);
        gdk_draw_rectangle(p, GC(), true, 0, 0, wid, hei);
        if (GC() == XorGC()) {
            clr.pixel = gd_foreg;
            gdk_gc_set_foreground(GC(), &clr);
        }
        else
            gdk_gc_set_foreground(GC(), &vals.foreground);
    }
    else {
        // Fix up the GC as it was.
        if (GC() == XorGC())
            gdk_gc_set_foreground(GC(), &vals.foreground);
        gdk_gc_set_function(GC(), vals.function);
        return;
    }

    gdk_draw_layout(p, GC(), 0, 0, lout);
    g_object_unref(lout);
    GdkImage *im = gdk_image_get(p, 0, 0, wid, hei);
    gdk_pixmap_unref(p);

    // Create a second image for the transformed copy.  This will cntain
    // the background pixels from the rendering area.

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

    // Fix up the GC as it was.
    if (GC() == XorGC())
        gdk_gc_set_foreground(GC(), &vals.foreground);
    gdk_gc_set_function(GC(), vals.function);

#else
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

    cairo_move_to(gd_cr, x, y);
    pango_cairo_show_layout(gd_cr, lout);
    cairo_fill(gd_cr);
    g_object_unref(lout);
#endif
}


// Return the width/height of text.  If text is 0, return
// the max bounds of any character.
//
void
gtk_draw::TextExtent(const char *text, int *wid, int *hei)
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
gtk_draw::MovePointer(int x, int y, bool absolute)
{
    // Called with 0,0 this redraws ghost objects
    if (gd_window) {
        int x0, y0;
        GdkScreen *screen;
        GdkDisplay *display = gdk_display_get_default();
        gdk_display_get_pointer(display, &screen, &x0, &y0, 0);
        if (absolute)
            gdk_window_get_root_origin(gd_window, &x0, &y0);
        x += x0;
        y += y0;
        gdk_display_warp_pointer(display, screen, x, y);
    }
}


// Set x, y, and state of pointer referenced to window of context.
//
void
gtk_draw::QueryPointer(int *x, int *y, unsigned *state)
{
    int tx, ty;
    unsigned int ts;
    gdk_window_get_pointer(gd_window, &tx, &ty, (GdkModifierType*)&ts);
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
gtk_draw::DefineColor(int *pixel, int red, int green, int blue)
{
    GdkColor newcolor;
    newcolor.red   = (red   << 8);
    newcolor.green = (green << 8);
    newcolor.blue  = (blue  << 8);
    newcolor.pixel = *pixel;
    if (GTKdev::ColorAlloc.no_alloc) {
        if (gdk_colormap_alloc_color(GRX->Colormap(), &newcolor, false, true))
            *pixel = newcolor.pixel;
        else
            *pixel = 0;
    }
    else {
        gdk_error_trap_push();
        GRX->SetSilenceErrs(true);
#ifdef XXX_GDK
        gdk_color_change(GRX->Colormap(), &newcolor);
#else
#endif
        gdk_flush();  // important!
        if (gdk_error_trap_pop()) {
            if (gdk_colormap_alloc_color(GRX->Colormap(), &newcolor,
                    false, true))
                *pixel = newcolor.pixel;
            else
                *pixel = 0;
        }
#ifdef XXX_GDK
        if (gd_gbag && gd_gbag->get_gc())
            gdk_gc_set_foreground(GC(), &newcolor);
#else
        gdk_cairo_set_source_color(gd_cr, &newcolor);
#endif
        GRX->SetSilenceErrs(false);
    }
}


// Set the window background in the GC's.
//
void
gtk_draw::SetBackground(int pixel)
{
    gd_backg = pixel;
    GdkColor clr;
    clr.pixel = pixel;
#ifdef XXX_GDK
    gdk_gc_set_background(GC(), &clr);
    gdk_gc_set_background(XorGC(), &clr);
    if (!GTKdev::ColorAlloc.num_mask_allocated) {
        clr.pixel = gd_foreg ^ pixel;
        gdk_gc_set_foreground(XorGC(), &clr);
    }
#else
#endif
}


// Actually change the drawing window background.
//
void
gtk_draw::SetWindowBackground(int pixel)
{
#ifdef XXX_GDK
    if (!GDK_IS_PIXMAP(gd_window)) {
        GdkColor clr;
        clr.pixel = pixel;
        gdk_window_set_background(gd_window, &clr);
    }
#else
    if (GDK_IS_WINDOW(gd_window)) {
        GdkColor clr;
        clr.pixel = pixel;
        gdk_window_set_background(gd_window, &clr);
    }
#endif
}


// Set the color used for ghost drawing.  pixel is an existing cell
// of the proper color.  If two-plane cells have been allocated,
// copy the pixel value to the upper plane half space, otherwise
// just set the xor foreground color.
//
void
gtk_draw::SetGhostColor(int pixel)
{
    if (GTKdev::ColorAlloc.num_mask_allocated) {
/* XXX
        GdkColor newcolor;
        newcolor.pixel = pixel;
        GdkColor *colors = gdk_colormat_get_colors(GRX->Colormap());
        newcolor.red = colors[pixel].red;
        newcolor.green = colors[pixel].green;
        newcolor.blue = colors[pixel].blue;
//XXX        newcolor.red = GRX->Colormap()->colors[pixel].red;
//XXX        newcolor.green = GRX->Colormap()->colors[pixel].green;
//XXX        newcolor.blue = GRX->Colormap()->colors[pixel].blue;
        for (int i = 0; i < GTKdev::ColorAlloc.num_mask_allocated; i++) {
            newcolor.pixel =
                GTKdev::ColorAlloc.plane_mask[0] |
                GTKdev::ColorAlloc.drawing_pixels[i];
            gdk_color_change(GRX->Colormap(), &newcolor);
        }
#ifdef WITH_X11
        XGCValues gcv;
        gcv.plane_mask = GTKdev::ColorAlloc.plane_mask[0];
        XChangeGC(gr_x_display(), gr_x_gc(XorGC()), GCPlaneMask, &gcv);
#endif
        gd_foreg = GTKdev::ColorAlloc.plane_mask[0] |
            GTKdev::ColorAlloc.drawing_pixels[0];
        newcolor.pixel = gd_foreg;
        gdk_gc_set_foreground(XorGC(), &newcolor);
*/
    }
    else {
        gd_foreg = pixel;
        GdkColor newcolor;
        newcolor.pixel = pixel ^ gd_backg;
#ifdef XXX_GDK
        gdk_gc_set_foreground(XorGC(), &newcolor);
#else
        gtk_QueryColor(&newcolor);
        gdk_cairo_set_source_color(gd_cr, &newcolor);
#endif
    }
}


// Set the current foreground color.
//
void
gtk_draw::SetColor(int pixel)
{
#ifdef XXX_GDK
    if (GC() == XorGC())
        return;
    GdkColor clr;
    clr.pixel = pixel;
    gdk_gc_set_foreground(GC(), &clr);
#else
    GdkColor clr;
    clr.pixel = pixel;
    gtk_QueryColor(&clr);
    gdk_cairo_set_source_color(gd_cr, &clr);
#endif
}


// Set the current linestyle.
//
void
gtk_draw::SetLinestyle(const GRlineType *lineptr)
{
#ifdef XXX_GDK
    if (!lineptr || !lineptr->mask || lineptr->mask == -1) {
        gdk_gc_set_line_attributes(GC(), 0, GDK_LINE_SOLID,
            GDK_CAP_BUTT, GDK_JOIN_MITER);
        return;
    }
    gdk_gc_set_line_attributes(GC(), 0, GDK_LINE_ON_OFF_DASH,
         GDK_CAP_BUTT, GDK_JOIN_MITER);

    gdk_gc_set_dashes(GC(), lineptr->offset,
        (signed char*)lineptr->dashes, lineptr->length);
#else
    if (!lineptr || lineptr->length == 0 || lineptr->length > 8) {
        cairo_set_dash(gd_cr, 0, 0, 0);
        return;
    }
    double dashes[8];
    for (int i = 0; i < lineptr->length; i++)
        dashes[i] = lineptr->dashes[i];
    cairo_set_dash(gd_cr, dashes, lineptr->length, lineptr->offset);
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
gtk_draw::DefineFillpattern(GRfillType *fillp)
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
#ifdef XXX_GDK
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
#else
#endif
#endif
}


// Set the current fill pattern.
//
void
gtk_draw::SetFillpattern(const GRfillType *fillp)
{
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    // Alas, gdk-win32 currently does not support stippled drawing,
    // which apparently got lost in the switch to cairo.  So, we roll
    // our own.

    gd_gbag->set_fillpattern(fillp);

#else
#ifdef XXX_GDK
    if (!fillp || !fillp->xPixmap())
        gdk_gc_set_fill(GC(), GDK_SOLID);
    else {
        gdk_gc_set_stipple(GC(), (GdkPixmap*)fillp->xPixmap());
        gdk_gc_set_fill(GC(), GDK_STIPPLED);
    }
#else
#endif
#endif
}


// Update the display.
//
void
gtk_draw::Update()
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
gtk_draw::Input(int *keyret, int *butret, int *xret, int *yret)
{
    *keyret = *butret = 0;
    *xret = *yret = 0;
    if (!gd_window)
        return;
    for (;;) {
        GdkEvent *ev = gdk_event_get();
        if (ev) {
            if (ev->type == GDK_BUTTON_PRESS && ev->button.window == gd_window) {
                *butret = ev->button.button;
                *xret = (int)ev->button.x;
                *yret = (int)ev->button.y;
                gdk_event_free(ev);
                break;
            }
            if (ev->type == GDK_KEY_PRESS && ev->key.window == gd_window) {
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
gtk_draw::SetXOR(int val)
{
#ifdef XXX_GDK
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
#else
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
        GdkPixmap *pmap;
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
gtk_draw::ShowGlyph(int gnum, int x, int y)
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
#ifdef XXX_GDK
    if (!st->pmap)
        st->pmap = gdk_bitmap_create_from_data(gd_window, (char*)st->bits,
            GlyphWidth, GlyphWidth);
    gdk_gc_set_stipple(GC(), st->pmap);
    gdk_gc_set_fill(GC(), GDK_STIPPLED);
    gdk_gc_set_ts_origin(GC(), x, y);
    gdk_draw_rectangle(gd_window, GC(), true, x, y, GlyphWidth, GlyphWidth);
    gdk_gc_set_ts_origin(GC(), 0, 0);
    gdk_gc_set_fill(GC(), GDK_SOLID);
#else
#endif
#endif
}


GRobject
gtk_draw::GetRegion(int x, int y, int wid, int hei)
{
#ifdef XXX_GDK
    GdkPixmap *pm = gdk_pixmap_new(gd_window, wid, hei,
        gdk_visual_get_depth(GRX->Visual()));
    gdk_window_copy_area(pm, GC(), 0, 0, gd_window, x, y, wid, hei);
    return ((GRobject)pm);
#else
    return (0);
#endif
}


void
gtk_draw::PutRegion(GRobject pm, int x, int y, int wid, int hei)
{
#ifdef XXX_GDK
    gdk_window_copy_area(gd_window, GC(), x, y, (GdkPixmap*)pm, 0, 0,
        wid, hei);
#else
#endif
}


void
gtk_draw::FreeRegion(GRobject pm)
{
#ifdef XXX_GDK
    gdk_pixmap_unref((GdkPixmap*)pm);
#else
#endif
}


void
gtk_draw::DisplayImage(const GRimage *image, int x, int y,
    int width, int height)
{
#ifdef WITH_X11
#ifdef USE_XSHM
#ifdef HAVE_SHMGET
    // Use MIT-SHM if available.  If an error occurs, fall through and
    // use normal processing.

    if (image->shmid()) {
        XShmSegmentInfo shminfo;
        XImage *im = XShmCreateImage(gr_x_display(),
            gr_x_visual(GRX->Visual()), GRX->Visual()->depth, ZPixmap,
            0, &shminfo, image->width(), image->height());
        if (!im)
            goto normal;
        shminfo.shmid = image->shmid();
        shminfo.shmaddr = im->data = (char*)image->data();
        shminfo.readOnly = False;
        if (XShmAttach(gr_x_display(), &shminfo) != True) {
            im->data = 0;
            XDestroyImage(im);
            goto normal;
        }
        XShmPutImage(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
            im, x, y, x, y, width, height, True);
        XShmDetach(gr_x_display(), &shminfo);
        im->data = 0;
        XDestroyImage(im);
        return;
    }
normal:
#endif
#endif

/** not used
    // Variation that uses normal memory in the GRimage and does all
    // SHM stuff here.

    if (GRpkgIf()->UseSHM() > 0) {
        XShmSegmentInfo shminfo;
        XImage *im = XShmCreateImage(gr_x_display(),
            gr_x_visual(GRX->Visual()), GRX->Visual()->depth, ZPixmap,
            0, &shminfo, width, height);
        if (!im)
            return;
        shminfo.shmid = shmget(IPC_PRIVATE, im->bytes_per_line * im->height,
            IPC_CREAT | 0777);
        if (shminfo.shmid == -1) {
            perror("shmget");
            return;
        }
        shminfo.shmaddr = im->data = (char*)shmat(shminfo.shmid, 0, 0);
        shminfo.readOnly = False;
        if (XShmAttach(gr_x_display(), &shminfo) != True)
            return;

        for (int i = 0; i < height; i++) {
            int yd = i + y;
            if (yd < 0)
                continue;
            if ((unsigned int)yd >= image->height())
                break;
            unsigned int *lptr = image->data() + yd*image->width();
            for (int j = 0; j < width; j++) {
                int xd = j + x;
                if (xd < 0)
                    continue;
                if ((unsigned int)xd >= image->width())
                    break;
                XPutPixel(im, j, i, lptr[xd]);
            }
        }
        XShmPutImage(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
            im, 0, 0, x, y, width, height, True);
        XShmDetach(gr_x_display(), &shminfo);
        XDestroyImage(im);
        shmdt(shminfo.shmaddr);
        shmctl(shminfo.shmid, IPC_RMID, 0);
    }
*/

    if (GRX->ImageCode() == 0) {
        // Pure-X version, may have slightly less overhead than the gtk
        // code, but does not use SHM.  I can't see any difference
        // with/without SHM anyway.
        //
        // Why allocate a new map and copy with XPutPixel?  The pixel
        // was obtained from X for the present dieplay/visual, so there
        // shouldn't need to be a format change.  The bit padding is 32,
        // same as ours.  Maybe there is an endian issue?  Anyway, this
        // code seems to work, and avoids the copy overhead.

        XImage *im = XCreateImage(gr_x_display(),
            gr_x_visual(GRX->Visual()), gdk_visual_get_depth(GRX->Visual()),
            ZPixmap, 0, 0, image->width(), image->height(), 32, 0);
        im->data = (char*)image->data();
#ifdef XXX_GDK
        XPutImage(gr_x_display(), gr_x_window(gd_window),
            gdk_x11_gc_get_xgc(GC()), im, x, y, x, y, width, height);
#else
#endif
        im->data = 0;
        XDestroyImage(im);
        return;
    }
    if (GRX->ImageCode() == 1) {
        // Pure-X version, may have slightly less overhead than the gtk
        // code, but does not use SHM.  I can't see any difference
        // with/without SHM anyway.

        XImage *im = XCreateImage(gr_x_display(),
            gr_x_visual(GRX->Visual()), gdk_visual_get_depth(GRX->Visual()),
            ZPixmap, 0, 0, width, height, 32, 0);
        im->data = (char*)malloc(im->bytes_per_line * im->height);

        for (int i = 0; i < height; i++) {
            int yd = i + y;
            if (yd < 0)
                continue;
            if ((unsigned int)yd >= image->height())
                break;
            unsigned int *lptr = image->data() + yd*image->width();
            for (int j = 0; j < width; j++) {
                int xd = j + x;
                if (xd < 0)
                    continue;
                if ((unsigned int)xd >= image->width())
                    break;
                XPutPixel(im, j, i, lptr[xd]);
            }
        }
#ifdef XXX_GDK
        XPutImage(gr_x_display(), gr_x_window(gd_window),
            gdk_x11_gc_get_xgc(GC()), im, 0, 0, x, y, width, height);
#else
#endif
        XDestroyImage(im);
        return;
    }
#endif

    // Gdk version.

#ifdef XXX_GDK
    GdkImage *im = gdk_image_new(GDK_IMAGE_FASTEST, GRX->Visual(),
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
            gdk_image_put_pixel(im, j, i, qpx);
#else
            gdk_image_put_pixel(im, j, i, px);
#endif
        }
    }

    gdk_draw_image(gd_window, GC(), im, 0, 0, x, y, width, height);
    gdk_image_destroy(im);
#else
#endif
}
// End of gtk_draw functions.

