
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

// ntkpixmap.cc:  This derives from the gdkpixmap.c from GTK-2.0. 
// It supports a ndkPixmap class which is a replacement for the
// GdkPixmap but which fits into the post-2.0 context.

/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "config.h"
#include "gtkinterf.h"
#include "ndkpixmap.h"

#ifdef XXX_GDK
ndkPixmap::ndkPixmap(GdkDrawable *drawable, int width, int height, bool bitmap)
{
    if (!drawable ||
            (GDK_IS_WINDOW(drawable) && GDK_WINDOW_DESTROYED(drawable)))
        drawable = gdk_screen_get_root_window(gdk_screen_get_default());

    pm_refcnt = 1;
    pm_width = width;
    pm_height = height;
    if (bitmap)
        pm_depth = 1;
    else
        pm_depth = gdk_visual_get_depth(gdk_drawable_get_visual(drawable));
    pm_screen = gdk_drawable_get_screen(drawable);

#ifdef WITH_X11
    pm_xid = XCreatePixmap(gdk_x11_drawable_get_xdisplay(drawable),
        gdk_x11_drawable_get_xid(drawable), width, height, pm_depth);
#endif

//  _gdk_xid_table_insert (GDK_WINDOW_DISPLAY (drawable), 
//			 &GDK_PIXMAP_XID (pixmap), pixmap);
}


//_gdk_bitmap_create_from_data
ndkPixmap::ndkPixmap(GdkDrawable *drawable, const char *data,
    int width, int height)
{
    if (!drawable ||
            (GDK_IS_WINDOW(drawable) && GDK_WINDOW_DESTROYED(drawable)))
        drawable = gdk_screen_get_root_window(gdk_screen_get_default());

    pm_refcnt = 1;
    pm_width = width;
    pm_height = height;
    pm_depth = 1;
    pm_screen = gdk_drawable_get_screen(drawable);
#ifdef WITH_X11
    pm_xid = XCreateBitmapFromData(gdk_x11_drawable_get_xdisplay(drawable),
        gdk_x11_drawable_get_xid(drawable), (char *)data, width, height);
#endif

//    _gdk_xid_table_insert(GDK_WINDOW_DISPLAY(drawable), 
//        &GDK_PIXMAP_XID(pixmap), pixmap);
}


// _gdk_pixmap_create_from_data
ndkPixmap::ndkPixmap(GdkDrawable *drawable, const char *data,
    int width, int height, const GdkColor *fg, const GdkColor *bg)
{
    if (!drawable ||
            (GDK_IS_WINDOW(drawable) && GDK_WINDOW_DESTROYED(drawable)))
        drawable = gdk_screen_get_root_window(gdk_screen_get_default());
  
    pm_refcnt = 1;
    pm_width = width;
    pm_height = height;
    pm_depth = gdk_visual_get_depth(gdk_drawable_get_visual(drawable));

    pm_screen = gdk_drawable_get_screen(drawable);
#ifdef WITH_X11
    pm_xid = XCreatePixmapFromBitmapData(gdk_x11_drawable_get_xdisplay(drawable),
        gdk_x11_drawable_get_xid(drawable), (char *)data, width, height,
        fg->pixel, bg->pixel, pm_depth);
#endif

//    _gdk_xid_table_insert (GDK_WINDOW_DISPLAY (drawable),
//    &GDK_PIXMAP_XID (pixmap), pixmap);
}

#else

ndkPixmap::ndkPixmap(GdkWindow *window, int width, int height, bool bitmap)
{
    if (!window || GDK_WINDOW_DESTROYED(window))
        window = gdk_screen_get_root_window(gdk_screen_get_default());

    pm_refcnt = 1;
    pm_width = width;
    pm_height = height;
    if (bitmap)
        pm_depth = 1;
    else
        pm_depth = gdk_visual_get_depth(gdk_drawable_get_visual(window));
    pm_screen = gdk_window_get_screen(window);

#ifdef WITH_X11
    pm_xid = XCreatePixmap(gdk_x11_drawable_get_xdisplay(window),
        gdk_x11_drawable_get_xid(window), width, height, pm_depth);
#endif

//  _gdk_xid_table_insert (GDK_WINDOW_DISPLAY (drawable), 
//			 &GDK_PIXMAP_XID (pixmap), pixmap);
}


//_gdk_bitmap_create_from_data
ndkPixmap::ndkPixmap(GdkWindow *window, const char *data,
    int width, int height)
{
    if (!window || GDK_WINDOW_DESTROYED(window))
        window = gdk_screen_get_root_window(gdk_screen_get_default());

    pm_refcnt = 1;
    pm_width = width;
    pm_height = height;
    pm_depth = 1;
    pm_screen = gdk_window_get_screen(window);
#ifdef WITH_X11
    pm_xid = XCreateBitmapFromData(gdk_x11_drawable_get_xdisplay(window),
        gdk_x11_drawable_get_xid(window), (char *)data, width, height);
#endif

//    _gdk_xid_table_insert(GDK_WINDOW_DISPLAY(drawable), 
//        &GDK_PIXMAP_XID(pixmap), pixmap);
}


// _gdk_pixmap_create_from_data
ndkPixmap::ndkPixmap(GdkWindow *window, const char *data,
    int width, int height, const GdkColor *fg, const GdkColor *bg)
{
    if (!window || GDK_WINDOW_DESTROYED(window))
        window = gdk_screen_get_root_window(gdk_screen_get_default());
  
    pm_refcnt = 1;
    pm_width = width;
    pm_height = height;
    pm_depth = gdk_visual_get_depth(gdk_drawable_get_visual(window));

    pm_screen = gdk_window_get_screen(window);
#ifdef WITH_X11
    pm_xid = XCreatePixmapFromBitmapData(gdk_x11_drawable_get_xdisplay(window),
        gdk_x11_drawable_get_xid(window), (char *)data, width, height,
        fg->pixel, bg->pixel, pm_depth);
#endif

//    _gdk_xid_table_insert (GDK_WINDOW_DISPLAY (drawable),
//    &GDK_PIXMAP_XID (pixmap), pixmap);
}

#endif

ndkPixmap::ndkPixmap(ndkPixmap *pm, int width, int height)
{
    pm_refcnt = 1;
    pm_width = pm->get_width();
    pm_height = pm->get_height();
    pm_depth = pm->get_depth();
    pm_screen = pm->get_screen();
#ifdef WITH_X11
    pm_xid = XCreatePixmap(
        gdk_x11_display_get_xdisplay(gdk_screen_get_display(pm_screen)),
        pm->get_xid(), pm_width, pm_height, pm_depth);
#endif
}

ndkPixmap::~ndkPixmap()
{
    GdkDisplay *display = gdk_screen_get_display(pm_screen);
    if (!display->closed) {
	    XFreePixmap(gdk_x11_display_get_xdisplay(display), pm_xid);
    }
//    _gdk_xid_table_remove(display, pm_xid);
}

#ifdef XXX_NEW

void
ndkPixmap::copy_to_window(GdkDrawable *drawable, ndkGC *gc, int xsrc, int ysrc,
    int xdest, int ydest, int width, int height)
{
    // Don't draw from outside of the window, this can trigger an X server
    // bug.
    //
    // See: 
    // http://lists.freedesktop.org/archives/xorg/2009-February/043318.html
    //

    if (xsrc < 0) {
        width += xsrc;
        xdest -= xsrc;
        xsrc = 0;
    }
  
    if (ysrc < 0) {
        height += ysrc;
        ydest -= ysrc;
        ysrc = 0;
    }

    if (xsrc + width > pm_width)
        width = pm_width - xsrc;
    if (ysrc + height > pm_height)
        height = pm_height - ysrc;
  
    if (pm_depth == 1) {
        XCopyArea(gc->get_xdisplay(), pm_xid, gdk_x11_drawable_get_xid(drawable),
            gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
        return;
    }
    int dest_depth = gdk_drawable_get_depth(drawable);
    if (dest_depth != 0 && pm_depth == dest_depth) {
        XCopyArea(gc->get_xdisplay(), pm_xid, gdk_x11_drawable_get_xid(drawable),
            gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
    }
}


void
ndkPixmap::copy_from_window(GdkDrawable *drawable, ndkGC *gc, int xsrc, int ysrc,
    int xdest, int ydest, int width, int height)
{
    // Don't draw from outside of the window, this can trigger an X server
    // bug.
    //
    // See: 
    // http://lists.freedesktop.org/archives/xorg/2009-February/043318.html
    //

    if (xsrc < 0) {
        width += xsrc;
        xdest -= xsrc;
        xsrc = 0;
    }
  
    if (ysrc < 0) {
        height += ysrc;
        ydest -= ysrc;
        ysrc = 0;
    }

    int swid = gdk_drawable_get_width(drawable);
    int shei = gdk_drawable_get_height(drawable);
    if (xsrc + width > swid)
        width = swid - xsrc;
    if (ysrc + height > shei)
        height = shei - ysrc;
  
    int src_depth = gdk_visual_get_depth(gdk_drawable_get_visual(drawable));
    if (src_depth == 1) {
        XCopyArea(gc->get_xdisplay(), gdk_x11_drawable_get_xid(drawable),
            pm_xid, gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
        return;
    }
    int dest_depth = pm_depth;
    if (dest_depth != 0 && src_depth == dest_depth) {
        XCopyArea(gc->get_xdisplay(), gdk_x11_drawable_get_xid(drawable),
            pm_xid, gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
    }
}

#else

void
ndkPixmap::copy_to_window(GdkWindow *window, ndkGC *gc, int xsrc, int ysrc,
    int xdest, int ydest, int width, int height)
{
    // Don't draw from outside of the window, this can trigger an X server
    // bug.
    //
    // See: 
    // http://lists.freedesktop.org/archives/xorg/2009-February/043318.html
    //

    if (xsrc < 0) {
        width += xsrc;
        xdest -= xsrc;
        xsrc = 0;
    }
  
    if (ysrc < 0) {
        height += ysrc;
        ydest -= ysrc;
        ysrc = 0;
    }

    if (xsrc + width > pm_width)
        width = pm_width - xsrc;
    if (ysrc + height > pm_height)
        height = pm_height - ysrc;
  
    if (pm_depth == 1) {
        XCopyArea(gc->get_xdisplay(), pm_xid, gdk_x11_drawable_get_xid(window),
            gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
        return;
    }
    int dest_depth = gdk_drawable_get_depth(window);
    if (dest_depth != 0 && pm_depth == dest_depth) {
        XCopyArea(gc->get_xdisplay(), pm_xid, gdk_x11_drawable_get_xid(window),
            gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
    }
}


void
ndkPixmap::copy_from_window(GdkWindow *window, ndkGC *gc, int xsrc, int ysrc,
    int xdest, int ydest, int width, int height)
{
    // Don't draw from outside of the window, this can trigger an X server
    // bug.
    //
    // See: 
    // http://lists.freedesktop.org/archives/xorg/2009-February/043318.html
    //

    if (xsrc < 0) {
        width += xsrc;
        xdest -= xsrc;
        xsrc = 0;
    }
  
    if (ysrc < 0) {
        height += ysrc;
        ydest -= ysrc;
        ysrc = 0;
    }

    int swid = gdk_window_get_width(window);
    int shei = gdk_window_get_height(window);
    if (xsrc + width > swid)
        width = swid - xsrc;
    if (ysrc + height > shei)
        height = shei - ysrc;
  
    int src_depth = gdk_visual_get_depth(gdk_drawable_get_visual(window));
    if (src_depth == 1) {
        XCopyArea(gc->get_xdisplay(), gdk_x11_drawable_get_xid(window),
            pm_xid, gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
        return;
    }
    int dest_depth = pm_depth;
    if (dest_depth != 0 && src_depth == dest_depth) {
        XCopyArea(gc->get_xdisplay(), gdk_x11_drawable_get_xid(window),
            pm_xid, gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
    }
}

#endif

void
ndkPixmap::copy_to_pixmap(ndkPixmap *pixmap, ndkGC *gc, int xsrc, int ysrc,
    int xdest, int ydest, int width, int height)
{
    // Don't draw from outside of the window, this can trigger an X server
    // bug.
    //
    // See: 
    // http://lists.freedesktop.org/archives/xorg/2009-February/043318.html
    //

    if (xsrc < 0) {
        width += xsrc;
        xdest -= xsrc;
        xsrc = 0;
    }
  
    if (ysrc < 0) {
        height += ysrc;
        ydest -= ysrc;
        ysrc = 0;
    }

    if (xsrc + width > pm_width)
        width = pm_width - xsrc;
    if (ysrc + height > pm_height)
        height = pm_height - ysrc;
  
    if (pm_depth == 1) {
        XCopyArea(gc->get_xdisplay(), pm_xid, pixmap->get_xid(),
            gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
        return;
    }
    int dest_depth = pixmap->get_depth();
    if (dest_depth != 0 && pm_depth == dest_depth) {
        XCopyArea(gc->get_xdisplay(), pm_xid, pixmap->get_xid(),
            gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
    }
}


void
ndkPixmap::copy_from_pixmap(ndkPixmap *pixmap, ndkGC *gc, int xsrc, int ysrc,
    int xdest, int ydest, int width, int height)
{
    // Don't draw from outside of the window, this can trigger an X server
    // bug.
    //
    // See: 
    // http://lists.freedesktop.org/archives/xorg/2009-February/043318.html
    //

    if (xsrc < 0) {
        width += xsrc;
        xdest -= xsrc;
        xsrc = 0;
    }
  
    if (ysrc < 0) {
        height += ysrc;
        ydest -= ysrc;
        ysrc = 0;
    }

    int swid = pixmap->get_width();
    int shei = pixmap->get_height();
    if (xsrc + width > swid)
        width = swid - xsrc;
    if (ysrc + height > shei)
        height = shei - ysrc;
  
    int src_depth = pixmap->get_depth();
    if (src_depth == 1) {
        XCopyArea(gc->get_xdisplay(), pixmap->get_xid(),
            pm_xid, gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
        return;
    }
    int dest_depth = pm_depth;
    if (dest_depth != 0 && src_depth == dest_depth) {
        XCopyArea(gc->get_xdisplay(), pixmap->get_xid(),
            pm_xid, gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
    }
}


// Static function.
// Looks up the ndkPixmap that wraps the given native pixmap handle.
//
// For example in the X backend, a native pixmap handle is an Xlib
// XID.
//
// Return value: the ndkPixmap wrapper for the native pixmap,
// or 0 if there is none.
//
ndkPixmap *
ndkPixmap::lookup(unsigned long anid)
{
//    return (ndkPixmap*) gdk_xid_table_lookup_for_display(
//        gdk_display_get_default(), anid);
//XXX
return (0);
}


// Static function.
// Looks up the ndkPixmap that wraps the given native pixmap handle.
//
// For example in the X backend, a native pixmap handle is an Xlib
// XID.
//
// Return value: the ndkPixmap wrapper for the native pixmap,
// or %NULL if there is none.
//
ndkPixmap*
ndkPixmap::lookup_for_display(GdkDisplay *display, unsigned long anid)
{
//    if (GDK_IS_DISPLAY(display))
//        return (ndkPixmap*)gdk_xid_table_lookup_for_display(display, anid);
//XXX
    return (0);
}

