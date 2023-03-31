
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
 * GtkInterf Graphical Interface Library, New Drawing Kit (ndk)           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

// An ndkDrawable is a container for a GdkWindow and a compatible
// ndkPixmap.  It manages the drawing target in the drawing interface. 
// This is not really the same as the old GdkDrawable and not derived
// from that object.

#include "config.h"
#include "gtkinterf.h"

#ifdef NDKDRAWABLE_H

ndkDrawable::ndkDrawable()
{
    d_window = 0;
    d_pixmap = 0;
    d_state = DW_NONE;
#ifdef WITH_X11
    d_xid = None;
#endif
}


ndkDrawable::~ndkDrawable()
{
    if (d_pixmap)
        d_pixmap->dec_ref();
}


// The primary way to use the interface is to use this function to
// define a window, then use set_draw_tp_pixmap to create a pixmap if
// necessary and set xid to the pixmap, then after drawing call
// set_draw_to_window to return the xid to the window, and call
// copy_pixmap_to_window to update the window.  The assumption here is
// that the pixmap is used for only one window, so can be used for
// expose event refresn=hing.
//
void
ndkDrawable::set_window(GdkWindow *window)
{
    if (d_pixmap) {
        d_pixmap->dec_ref();
        d_pixmap = 0;
    }
    if (window && GDK_IS_WINDOW(window)) {
        d_window = window;
        d_state = DW_WINDOW;
#ifdef WITH_X11
#if GTK_CHECK_VERSION(3,0,0)
        d_xid = gdk_x11_window_get_xid(d_window);
#else
        d_xid = gdk_x11_drawable_get_xid(d_window);
#endif
#endif
    }
    else {
        d_window = 0;
        d_state = DW_NONE;
#ifdef WITH_X11
        d_xid = None;
#endif
    }
}


#if GTK_CHECK_VERSION(3,0,0)

// Static function.
// This is for use in a draw signal handler.  It obtains the pixel area
// to redraw from the cairo_t* provided to the handler.
//
void
ndkDrawable::redraw_area(cairo_t *cr, cairo_rectangle_int_t *rect)
{
    double x1, y1, x2, y2;
    cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
    int ix1 = x1;
    int iy1 = y1;
    int ix2 = x2;
    int iy2 = y2;
    if (ix2 < ix1) {
        int t = ix1; ix1 = ix2; ix2 = t;
    }
    if (iy2 < iy1) {
        int t = iy1; iy1 = iy2; iy2 = t;
    }
    rect->x = ix1;
    rect->y = iy1;
    rect->width = ix2 - ix1;
    rect->height = iy2 - iy1;
}

#endif


// Another way to use the interface, which is useful when you have a
// lot of similar windows to manage, is to call this function before
// drawing, then explicitly copy into the window with
// copy_pixmap_to_window.  The pixmap only changs when it needs to be
// enlarged for different windows.
//
void
ndkDrawable::set_pixmap(GdkWindow *window)
{
    if (!window || !GDK_IS_WINDOW(window))
        return;
    int wid = gdk_window_get_width(window);
    int hei = gdk_window_get_height(window);
    if (!d_pixmap ||
            wid > d_pixmap->get_width() || hei > d_pixmap->get_height()) {
        d_pixmap = new ndkPixmap(window, wid, hei);
    }
    d_window = window;
    d_xid = d_pixmap->get_xid();
    d_state = DW_PIXMAP;
}


void
ndkDrawable::set_pixmap(ndkPixmap *pixmap)
{
    if (d_pixmap)
        d_pixmap->dec_ref();
    d_pixmap = pixmap;
    if (pixmap) {
        d_xid = d_pixmap->get_xid();
        d_state = DW_PIXMAP;
    }
    else if (d_window) {
#if GTK_CHECK_VERSION(3,0,0)
        d_xid = gdk_x11_window_get_xid(d_window);
#else
        d_xid = gdk_x11_drawable_get_xid(d_window);
#endif
        d_state = DW_WINDOW;
    }
    else {
        d_xid = None;
        d_state = DW_NONE;
    }
}


void
ndkDrawable::set_draw_to_window()
{
#ifdef WITH_X11
    if (d_window) {
#if GTK_CHECK_VERSION(3,0,0)
        d_xid = gdk_x11_window_get_xid(d_window);
#else
        d_xid = gdk_x11_drawable_get_xid(d_window);
#endif
        d_state = DW_WINDOW;
    }
    else {
        d_xid = None;
        d_state = DW_NONE;
    }
#endif
}


bool
ndkDrawable::set_draw_to_pixmap()
{
    bool dirty = check_compatible_pixmap();
#ifdef WITH_X11
    if (d_pixmap) {
        d_xid = d_pixmap->get_xid();
        d_state = DW_PIXMAP;
    }
    else {
        d_xid = None;
        d_state = DW_NONE;
    }
#endif
    return (dirty);
}


bool
ndkDrawable::check_compatible_pixmap()
{
    if (!d_window)
        return (false);
    bool dirty = false;
    int wid = gdk_window_get_width(d_window);
    int hei = gdk_window_get_height(d_window);
    if (!d_pixmap ||
            wid != d_pixmap->get_width() || hei != d_pixmap->get_height()) {
        if (d_pixmap)
            d_pixmap->dec_ref();
        d_pixmap = new ndkPixmap(d_window, wid, hei);
        dirty = true;
    }
    if (d_state == DW_PIXMAP)
        d_xid = d_pixmap->get_xid();
    return (dirty);
}
 

void
ndkDrawable::copy_pixmap_to_window(ndkGC *gc, int x, int y, int w, int h)
{
    if (d_window && d_pixmap) {
        int wp = d_pixmap->get_width();
        if (w < 0 || w + x > wp)
            w = wp - x;
        int hp = d_pixmap->get_height();
        if (h < 0 || h + y > hp)
            h = hp - y;
        d_pixmap->copy_to_window(d_window, gc, x, y, x, y, w, h);
    }
}


#if GTK_CHECK_VERSION(3,0,0)

void
ndkDrawable::refresh(ndkGC *gc, cairo_t *cr)
{
    if (d_window && d_pixmap) {
        if (!cr) {
            d_pixmap->copy_to_window(d_window, gc, 0, 0, 0, 0,
                d_pixmap->get_width(), d_pixmap->get_height());
            return;
        }

        cairo_rectangle_list_t *rlist = cairo_copy_clip_rectangle_list(cr);
        if (rlist->status != CAIRO_STATUS_SUCCESS) {
            cairo_rectangle_list_destroy(rlist);
            return;
        }
        for (int i = 0; i < rlist->num_rectangles; i++) {
            cairo_rectangle_int_t r;
            r.x = rlist->rectangles[i].x;
            r.y = rlist->rectangles[i].y;
            r.width = rlist->rectangles[i].width;
            r.height = rlist->rectangles[i].height;
            d_pixmap->copy_to_window(d_window, gc, r.x, r.y, r.x, r.y,
                r.width, r.height);
        }
        cairo_rectangle_list_destroy(rlist);
    }
}

#else

void
ndkDrawable::refresh(ndkGC *gc, GdkEventExpose *pev)
{
    if (d_window && d_pixmap) {
        if (!pev) {
            d_pixmap->copy_to_window(d_window, gc, 0, 0, 0, 0,
                d_pixmap->get_width(), d_pixmap->get_height());
            return;
        }
        GdkRectangle *rects;
        int nrects;
        gdk_region_get_rectangles(pev->region, &rects, &nrects);
        GdkRectangle *r = rects;
        if (nrects <= 0)
            return;
        while (nrects--) {
            d_pixmap->copy_to_window(d_window, gc, r->x, r->y, r->x, r->y,
                r->width, r->height);
            r++;
        }
        g_free(rects);
    }
}

#endif


GdkScreen *
ndkDrawable::get_screen()
{
    if (d_state == DW_WINDOW && d_window)
        return (gdk_window_get_screen(d_window));
    if (d_state == DW_PIXMAP && d_pixmap)
        return (d_pixmap->get_screen());
    if (d_window)
        return (gdk_window_get_screen(d_window));
    if (d_pixmap)
        return (d_pixmap->get_screen());
    return (0);
}


GdkVisual *
ndkDrawable::get_visual()
{
    if (d_state == DW_WINDOW && d_window)
        return (gdk_window_get_visual(d_window));
    if (d_state == DW_PIXMAP && d_pixmap)
        return (d_pixmap->get_visual());
    if (d_window)
        return (gdk_window_get_visual(d_window));
    if (d_pixmap)
        return (d_pixmap->get_visual());
    return (0);
}


int
ndkDrawable::get_width()
{
    if (d_state == DW_WINDOW && d_window)
        return (gdk_window_get_width(d_window));
    if (d_state == DW_PIXMAP && d_pixmap)
        return (d_pixmap->get_width());
    if (d_window)
        return (gdk_window_get_width(d_window));
    if (d_pixmap)
        return (d_pixmap->get_width());
    return (-1);
}


int
ndkDrawable::get_height()
{
    if (d_state == DW_WINDOW && d_window)
        return (gdk_window_get_height(d_window));
    if (d_state == DW_PIXMAP && d_pixmap)
        return (d_pixmap->get_height());
    if (d_window)
        return (gdk_window_get_height(d_window));
    if (d_pixmap)
        return (d_pixmap->get_height());
    return (-1);
}


int
ndkDrawable::get_depth()
{
    if (d_state == DW_WINDOW && d_window)
        return (gdk_visual_get_depth(gdk_window_get_visual(d_window)));
    if (d_state == DW_PIXMAP && d_pixmap)
        return (d_pixmap->get_depth());
    if (d_window)
        return (gdk_visual_get_depth(gdk_window_get_visual(d_window)));
    if (d_pixmap)
        return (d_pixmap->get_depth());
    return (-1);
}

#endif

