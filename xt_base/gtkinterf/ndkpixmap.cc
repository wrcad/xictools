
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

#ifdef NDKPIXMAP_H


#ifdef WITH_X11
namespace {
    int xid_hash(XID *xid)          { return (*xid); }
    bool xid_equal(XID *a, XID *b)  { return (*a == *b); }

    GHashTable *pixmap_xid_tab;
}
#endif


ndkPixmap::ndkPixmap(GdkWindow *window, int width, int height, bool bitmap)
{
#if GTK_CHECK_VERSION(3,0,0)
    if (!window || gdk_window_is_destroyed(window))
#else
    if (!window || GDK_WINDOW_DESTROYED(window))
#endif
        window = gdk_screen_get_root_window(gdk_screen_get_default());

    pm_refcnt = 1;
    pm_width = width;
    pm_height = height;
    pm_screen = gdk_window_get_screen(window);
    if (bitmap) {
        pm_visual = 0;
        pm_depth = 1;
    }
    else {
        pm_visual = gdk_window_get_visual(window);
        pm_depth = gdk_visual_get_depth(pm_visual);
    }

#ifdef WITH_X11
#if GTK_CHECK_VERSION(3,0,0)
    pm_xid = XCreatePixmap(
        gdk_x11_display_get_xdisplay(gdk_window_get_display(window)),
        gdk_x11_window_get_xid(window), width, height, pm_depth);
#else
    pm_xid = XCreatePixmap(gdk_x11_drawable_get_xdisplay(window),
        gdk_x11_drawable_get_xid(window), width, height, pm_depth);
#endif

    if (!pixmap_xid_tab) {
        pixmap_xid_tab = g_hash_table_new((GHashFunc)xid_hash,
            (GEqualFunc)xid_equal);
    }
    if (g_hash_table_lookup(pixmap_xid_tab, &pm_xid)) {
        g_warning("XID collision detected!");
    }
    g_hash_table_insert(pixmap_xid_tab, &pm_xid, this);
#endif
}


//_gdk_bitmap_create_from_data
ndkPixmap::ndkPixmap(GdkWindow *window, const char *data,
    int width, int height)
{
#if GTK_CHECK_VERSION(3,0,0)
    if (!window || gdk_window_is_destroyed(window))
#else
    if (!window || GDK_WINDOW_DESTROYED(window))
#endif
        window = gdk_screen_get_root_window(gdk_screen_get_default());

    pm_refcnt = 1;
    pm_width = width;
    pm_height = height;
    pm_screen = gdk_window_get_screen(window);
    pm_visual = 0;
    pm_depth = 1;

#ifdef WITH_X11
#if GTK_CHECK_VERSION(3,0,0)
    pm_xid = XCreateBitmapFromData(
        gdk_x11_display_get_xdisplay(gdk_window_get_display(window)),
        gdk_x11_window_get_xid(window), (char *)data, width, height);
#else
    pm_xid = XCreateBitmapFromData(gdk_x11_drawable_get_xdisplay(window),
        gdk_x11_drawable_get_xid(window), (char *)data, width, height);
#endif

    if (!pixmap_xid_tab) {
        pixmap_xid_tab = g_hash_table_new((GHashFunc)xid_hash,
            (GEqualFunc)xid_equal);
    }
    if (g_hash_table_lookup(pixmap_xid_tab, &pm_xid)) {
        g_warning("XID collision detected!");
    }
    g_hash_table_insert(pixmap_xid_tab, &pm_xid, this);
#endif
}


// _gdk_pixmap_create_from_data
ndkPixmap::ndkPixmap(GdkWindow *window, const char *data,
    int width, int height, const GdkColor *fg, const GdkColor *bg)
{
#if GTK_CHECK_VERSION(3,0,0)
    if (!window || gdk_window_is_destroyed(window))
#else
    if (!window || GDK_WINDOW_DESTROYED(window))
#endif
        window = gdk_screen_get_root_window(gdk_screen_get_default());
  
    pm_refcnt = 1;
    pm_width = width;
    pm_height = height;
    pm_screen = gdk_window_get_screen(window);
    pm_visual = gdk_window_get_visual(window);
    pm_depth = gdk_visual_get_depth(pm_visual);

#ifdef WITH_X11
#if GTK_CHECK_VERSION(3,0,0)
    pm_xid = XCreatePixmapFromBitmapData(
        gdk_x11_display_get_xdisplay(gdk_window_get_display(window)),
        gdk_x11_window_get_xid(window), (char *)data, width, height,
#else
    pm_xid = XCreatePixmapFromBitmapData(gdk_x11_drawable_get_xdisplay(window),
        gdk_x11_drawable_get_xid(window), (char *)data, width, height,
#endif
        fg->pixel, bg->pixel, pm_depth);

#if GTK_CHECK_VERSION(3,0,0)
    pm_xid = XCreateBitmapFromData(
        gdk_x11_display_get_xdisplay(gdk_window_get_display(window)),
        gdk_x11_window_get_xid(window), (char *)data, width, height);
#else
    pm_xid = XCreateBitmapFromData(gdk_x11_drawable_get_xdisplay(window),
        gdk_x11_drawable_get_xid(window), (char *)data, width, height);
#endif

    if (!pixmap_xid_tab) {
        pixmap_xid_tab = g_hash_table_new((GHashFunc)xid_hash,
            (GEqualFunc)xid_equal);
    }
    if (g_hash_table_lookup(pixmap_xid_tab, &pm_xid)) {
        g_warning("XID collision detected!");
    }
    g_hash_table_insert(pixmap_xid_tab, &pm_xid, this);
#endif
}


ndkPixmap::ndkPixmap(ndkPixmap *pm, int width, int height, bool bitmap)
{
    if (!pm) {
        pm_refcnt = 0;
        pm_width = 0;
        pm_height = 0;
        pm_depth = 0;
        pm_screen = 0;
#ifdef WITH_X11
        pm_xid = None;
#endif
        return;
    }
    pm_refcnt = 1;
    pm_width = width >= 0 ? width : pm->get_width();
    pm_height = height >= 0 ? height : pm->get_height();
    pm_screen = pm->get_screen();
    if (bitmap) {
        pm_visual = 0;
        pm_depth = 1;
    }
    else {
        pm_visual = pm->get_visual();
        pm_depth = pm->get_depth();
    }
#ifdef WITH_X11
    pm_xid = XCreatePixmap(
        gdk_x11_display_get_xdisplay(gdk_screen_get_display(pm_screen)),
        pm->get_xid(), pm_width, pm_height, pm_depth);

    if (!pixmap_xid_tab) {
        pixmap_xid_tab = g_hash_table_new((GHashFunc)xid_hash,
            (GEqualFunc)xid_equal);
    }
    if (g_hash_table_lookup(pixmap_xid_tab, &pm_xid)) {
        g_warning("XID collision detected!");
    }
    g_hash_table_insert(pixmap_xid_tab, &pm_xid, this);
#endif
}


ndkPixmap::~ndkPixmap()
{
    GdkDisplay *display = gdk_screen_get_display(pm_screen);
    if (!gdk_display_is_closed(display))
        XFreePixmap(gdk_x11_display_get_xdisplay(display), pm_xid);
    if (pixmap_xid_tab)
        g_hash_table_remove(pixmap_xid_tab, &pm_xid);
}


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
#if GTK_CHECK_VERSION(3,0,0)
        XCopyArea(gc->get_xdisplay(), pm_xid, gdk_x11_window_get_xid(window),
#else
        XCopyArea(gc->get_xdisplay(), pm_xid, gdk_x11_drawable_get_xid(window),
#endif
            gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
        return;
    }
#if GTK_CHECK_VERSION(3,0,0)
    int dest_depth = gdk_visual_get_depth(gdk_window_get_visual(window));
#else
    int dest_depth = gdk_drawable_get_depth(window);
#endif
    if (dest_depth != 0 && pm_depth == dest_depth) {
#if GTK_CHECK_VERSION(3,0,0)
        XCopyArea(gc->get_xdisplay(), pm_xid, gdk_x11_window_get_xid(window),
#else
        XCopyArea(gc->get_xdisplay(), pm_xid, gdk_x11_drawable_get_xid(window),
#endif
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
  
    int src_depth = gdk_visual_get_depth(gdk_window_get_visual(window));
    if (src_depth == 1) {
#if GTK_CHECK_VERSION(3,0,0)
        XCopyArea(gc->get_xdisplay(), gdk_x11_window_get_xid(window),
#else
        XCopyArea(gc->get_xdisplay(), gdk_x11_drawable_get_xid(window),
#endif
            pm_xid, gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
        return;
    }
    int dest_depth = pm_depth;
    if (dest_depth != 0 && src_depth == dest_depth) {
#if GTK_CHECK_VERSION(3,0,0)
        XCopyArea(gc->get_xdisplay(), gdk_x11_window_get_xid(window),
#else
        XCopyArea(gc->get_xdisplay(), gdk_x11_drawable_get_xid(window),
#endif
            pm_xid, gc->get_xgc(), xsrc, ysrc, width, height, xdest, ydest);
    }
}


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


void
ndkPixmap::copy_to_drawable(ndkDrawable *dw, ndkGC *gc, int xsrc, int ysrc,
    int xdest, int ydest, int width, int height)
{
    if (!dw || dw->get_state() == DW_NONE)
        return;
    if (dw->get_state() == DW_WINDOW) {
        GdkWindow *w = dw->get_window();
        if (w)
            copy_to_window(w, gc, xsrc, ysrc, xdest, ydest, width, height);
    }
    else if (dw->get_state() == DW_PIXMAP) {
        ndkPixmap *p = dw->get_pixmap();
        if (p)
            copy_to_pixmap(p, gc, xsrc, ysrc, xdest, ydest, width, height);
    }
}


void
ndkPixmap::copy_from_drawable(ndkDrawable *dw, ndkGC *gc, int xsrc, int ysrc,
    int xdest, int ydest, int width, int height)
{
    if (!dw || dw->get_state() == DW_NONE)
        return;
    if (dw->get_state() == DW_WINDOW) {
        GdkWindow *w = dw->get_window();
        if (w)
            copy_from_window(w, gc, xsrc, ysrc, xdest, ydest, width, height);
    }
    else if (dw->get_state() == DW_PIXMAP) {
        ndkPixmap *p = dw->get_pixmap();
        if (p)
            copy_from_pixmap(p, gc, xsrc, ysrc, xdest, ydest, width, height);
    }
}


void
ndkPixmap::copy_from_pango_layout(ndkGC *gc, int x, int y, PangoLayout *lout)
{
    if (pm_depth == 1 || !pm_visual)
        return;
    cairo_surface_t *sfc = cairo_xlib_surface_create(gc->get_xdisplay(),
        pm_xid, gdk_x11_visual_get_xvisual(pm_visual), pm_width, pm_height);
    cairo_t *cr = cairo_create(sfc);
    cairo_surface_destroy(sfc);
    GdkColor clr;
    clr.pixel = gc->get_bg_pixel();
    gtk_QueryColor(&clr);
    gdk_cairo_set_source_color(cr, &clr);
    cairo_paint(cr);
    clr.pixel = gc->get_fg_pixel();
    gtk_QueryColor(&clr);
    gdk_cairo_set_source_color(cr, &clr);
    cairo_move_to(cr, x, y);
    pango_cairo_show_layout(cr, lout);
    cairo_fill(cr);
    cairo_destroy(cr);
}


// Static function.
ndkPixmap *
ndkPixmap::lookup(unsigned long id)
{
    if (pixmap_xid_tab)
        return ((ndkPixmap*)g_hash_table_lookup(pixmap_xid_tab, &id));
    return (0);
}

#endif  // NDKPIXMAP_H

