
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

// ntkgc.cc:  This derives from the gdkgc.c from GTK-2.0.  It supports a
// ndkGC class which is a replacement for the GdkGC but which fits into the
// post-2.0 context.

// GDK - The GIMP Drawing Kit
// Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
//

//
// Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
// file for a list of people on the GTK+ Team.  See the ChangeLog
// files for a list of changes.  These files are distributed with
// GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
//

#include "config.h"
#include "gtkinterf.h"

#ifdef NDKGC_H

ndkGC::ndkGC(GdkWindow *window, ndkGCvalues *values,
    ndkGCvaluesMask values_mask)
{
    gc_clip_x_origin        = 0;
    gc_clip_y_origin        = 0;
    gc_ts_x_origin          = 0;
    gc_ts_y_origin          = 0;

    gc_clip_region          = 0;
    gc_old_clip_region      = 0;
    gc_old_clip_mask        = 0;

    gc_region_tag_applied   = 0;
    gc_region_tag_offset_x  = 0;
    gc_region_tag_offset_y  = 0;

    gc_stipple              = 0;
    gc_tile                 = 0;
    gc_clip_mask            = 0;

    // These are the default X11 value, which we match.  They are
    // clearly wrong for TrueColor displays, so apps have to change
    // them.
    //
    gc_fg_pixel             = 0;
    gc_bg_pixel             = 1;
    gc_function             = ndkGC_COPY;

#ifdef WITH_X11
    gc_gc                   = 0;
    gc_screen               = 0;
    gc_dirty_mask           = 0;
    gc_have_clip_region     = false;
    gc_have_clip_mask       = false;
    gc_depth                = 0;
#endif
    gc_subwindow_mode       = false;
    gc_fill                 = ndkGC_SOLID;
    gc_fill_rule            = ndkGC_EVEN_ODD_RULE;
    gc_exposures            = false;

    if (values) {
        if (values_mask & ndkGC_CLIP_X_ORIGIN)
            gc_clip_x_origin = values->v_clip_x_origin;
        if (values_mask & ndkGC_CLIP_Y_ORIGIN)
            gc_clip_y_origin = values->v_clip_y_origin;
        if (values_mask & ndkGC_TS_X_ORIGIN)
            gc_ts_x_origin = values->v_ts_x_origin;
        if (values_mask & ndkGC_TS_Y_ORIGIN)
            gc_ts_y_origin = values->v_ts_y_origin;

        if (values_mask & ndkGC_STIPPLE) {
            gc_stipple = values->v_stipple;
            if (gc_stipple)
                gc_stipple->inc_ref();
        }
        if (values_mask & ndkGC_TILE) {
            gc_tile = values->v_tile;
            if (gc_tile)
                gc_tile->inc_ref();
        }
        if (values_mask & ndkGC_CLIP_MASK) {
            gc_clip_mask = values->v_clip_mask;
            if (gc_clip_mask)
                gc_clip_mask->inc_ref();
        }

        if (values_mask & ndkGC_FOREGROUND)
            gc_fg_pixel = values->v_foreground.pixel;
        if (values_mask & ndkGC_BACKGROUND)
            gc_bg_pixel = values->v_background.pixel;

        if (values_mask & ndkGC_SUBWINDOW)
            gc_subwindow_mode = values->v_subwindow_mode;
        if (values_mask & ndkGC_FILL)
            gc_fill = values->v_fill;
        if (values_mask & ndkGC_FILL_RULE)
            gc_fill_rule = values->v_fill_rule;
        if (values_mask & ndkGC_EXPOSURES)
            gc_exposures = values->v_graphics_exposures;
        else
            gc_exposures = true;
    }

#ifdef WITH_X11
    if (!window)
        window = gdk_get_default_root_window();
    gc_screen = gdk_window_get_screen(window);
    gc_depth = gdk_visual_get_depth(gdk_window_get_visual(window));

    gc_dirty_mask = 0;
    if (values_mask & (ndkGC_CLIP_X_ORIGIN | ndkGC_CLIP_Y_ORIGIN)) {
        values_mask &= ~(ndkGC_CLIP_X_ORIGIN | ndkGC_CLIP_Y_ORIGIN);
        gc_dirty_mask |= ndkGC_DIRTY_CLIP;
    }

    if (values_mask & (ndkGC_TS_X_ORIGIN | ndkGC_TS_Y_ORIGIN)) {
        values_mask &= ~(ndkGC_TS_X_ORIGIN | ndkGC_TS_Y_ORIGIN);
        gc_dirty_mask |= ndkGC_DIRTY_TS;
    }

    gc_have_clip_mask = false;
    if ((values_mask & ndkGC_CLIP_MASK) && values->v_clip_mask)
        gc_have_clip_mask = true;

    XGCValues xvalues;
    xvalues.function = GXcopy;
    xvalues.fill_style = FillSolid;
    xvalues.arc_mode = ArcPieSlice;
    xvalues.subwindow_mode = ClipByChildren;
    xvalues.graphics_exposures = False;
    unsigned long xvalues_mask = GCFunction | GCFillStyle | GCArcMode |
        GCSubwindowMode | GCGraphicsExposures;

    gc_values_to_xvalues(values, values_mask, &xvalues, &xvalues_mask);

    gc_gc = XCreateGC(get_xdisplay(),
#if GTK_CHECK_VERSION(3,0,0)
        gdk_x11_window_get_xid(window), xvalues_mask, &xvalues);
#else
        gdk_x11_drawable_get_xid(window), xvalues_mask, &xvalues);
#endif
#endif
}


ndkGC::~ndkGC()
{
#ifdef HAVE_X11
    if (gc_gc)
        XFreeGC(get_xdisplay(), gc_gc);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    if (gc_clip_region)
        cairo_region_destroy(gc_clip_region);
    if (gc_old_clip_region)
        cairo_region_destroy(gc_old_clip_region);
#else
    if (gc_clip_region)
        gdk_region_destroy(gc_clip_region);
    if (gc_old_clip_region)
        gdk_region_destroy(gc_old_clip_region);
#endif
    if (gc_clip_mask)
        gc_clip_mask->dec_ref();
    if (gc_old_clip_mask)
        gc_old_clip_mask->dec_ref();
    if (gc_tile)
        gc_tile->dec_ref();
    if (gc_stipple)
        gc_stipple->dec_ref();
}


void
ndkGC::set_values(ndkGCvalues *values, ndkGCvaluesMask values_mask)
{
    if ((values_mask & ndkGC_CLIP_X_ORIGIN) ||
            (values_mask & ndkGC_CLIP_Y_ORIGIN) ||
            (values_mask & ndkGC_CLIP_MASK) ||
            (values_mask & ndkGC_SUBWINDOW))
        gc_remove_drawable_clip();
  
    if (values_mask & ndkGC_CLIP_X_ORIGIN)
        gc_clip_x_origin = values->v_clip_x_origin;
    if (values_mask & ndkGC_CLIP_Y_ORIGIN)
        gc_clip_y_origin = values->v_clip_y_origin;
    if (values_mask & ndkGC_TS_X_ORIGIN)
        gc_ts_x_origin = values->v_ts_x_origin;
    if (values_mask & ndkGC_TS_Y_ORIGIN)
        gc_ts_y_origin = values->v_ts_y_origin;

    if (values_mask & ndkGC_CLIP_MASK) {
        if (gc_clip_mask != values->v_clip_mask) {
            if (gc_clip_mask) {
                gc_clip_mask->dec_ref();
                gc_clip_mask = 0;
            }
            if (values->v_clip_mask) {
                gc_clip_mask = values->v_clip_mask;
                gc_clip_mask->inc_ref();
            }
        }
      
        if (gc_clip_region) {
#if GTK_CHECK_VERSION(3,0,0)
            cairo_region_destroy(gc_clip_region);
#else
            gdk_region_destroy(gc_clip_region);
#endif
            gc_clip_region = 0;
        }
    }
    if (values_mask & ndkGC_FILL)
        gc_fill = values->v_fill;
    if (values_mask & ndkGC_FILL_RULE)
        gc_fill_rule = values->v_fill_rule;
    if (values_mask & ndkGC_STIPPLE) {
        if (gc_stipple != values->v_stipple) {
            if (gc_stipple) {
                gc_stipple->dec_ref();
                gc_stipple = 0;
            }
            if (values->v_stipple) {
                gc_stipple = values->v_stipple;
                gc_stipple->inc_ref();
            }
        }
    }
    if (values_mask & ndkGC_TILE) {
        if (gc_tile != values->v_tile) {
            if (gc_tile) {
                gc_tile->dec_ref();
                gc_tile = 0;
            }
            if (values->v_tile) {
                gc_tile = values->v_tile;
                gc_tile->inc_ref();
            }
        }
    }
    if (values_mask & ndkGC_FOREGROUND)
        gc_fg_pixel = values->v_foreground.pixel;
    if (values_mask & ndkGC_BACKGROUND)
        gc_bg_pixel = values->v_background.pixel;
    if (values_mask & ndkGC_SUBWINDOW)
        gc_subwindow_mode = values->v_subwindow_mode;
    if (values_mask & ndkGC_EXPOSURES)
        gc_exposures = values->v_graphics_exposures;
  
#ifdef WITH_X11
    if (gc_gc)
        gc_x11_set_values(values, values_mask);
#endif
}


void
ndkGC::get_values(ndkGCvalues *values)
{
#ifdef WITH_X11
    if (gc_gc)
        gc_x11_get_values(values);
#endif
}


// x_offset: amount by which to offset the GC in the X direction.
// y_offset: amount by which to offset the GC in the Y direction.
// 
// Offset attributes such as the clip and tile-stipple origins of the
// GC so that drawing at x - x_offset, y - y_offset with the offset GC
// has the same effect as drawing at x, y with the original GC.
//
void
ndkGC::offset(int x_offset, int y_offset)
{
    if (x_offset != 0 || y_offset != 0) {
        ndkGCvalues values;
        values.v_clip_x_origin = gc_clip_x_origin - x_offset;
        values.v_clip_y_origin = gc_clip_y_origin - y_offset;
        values.v_ts_x_origin = gc_ts_x_origin - x_offset;
        values.v_ts_y_origin = gc_ts_y_origin - y_offset;
      
        set_values(&values, ndkGC_CLIP_X_ORIGIN | ndkGC_CLIP_Y_ORIGIN |
            ndkGC_TS_X_ORIGIN | ndkGC_TS_Y_ORIGIN);
    }
}


// Static function.
// dst_gc: the destination graphics context.
// src_gc: the source graphics context.
// 
// Copy the set of values from one graphics context onto another
// graphics context.
//
void
ndkGC::copy(ndkGC *dst_gc, ndkGC *src_gc)
{
    gc_windowing_copy(dst_gc, src_gc);

    dst_gc->gc_clip_x_origin = src_gc->gc_clip_x_origin;
    dst_gc->gc_clip_y_origin = src_gc->gc_clip_y_origin;
    dst_gc->gc_ts_x_origin = src_gc->gc_ts_x_origin;
    dst_gc->gc_ts_y_origin = src_gc->gc_ts_y_origin;

    if (dst_gc->gc_clip_region)
#if GTK_CHECK_VERSION(3,0,0)
        cairo_region_destroy(dst_gc->gc_clip_region);
#else
        gdk_region_destroy(dst_gc->gc_clip_region);
#endif

    if (src_gc->gc_clip_region)
#if GTK_CHECK_VERSION(3,0,0)
        dst_gc->gc_clip_region = cairo_region_copy(src_gc->gc_clip_region);
#else
        dst_gc->gc_clip_region = gdk_region_copy(src_gc->gc_clip_region);
#endif
    else
        dst_gc->gc_clip_region = 0;

    dst_gc->gc_region_tag_applied = src_gc->gc_region_tag_applied;
  
    if (dst_gc->gc_old_clip_region)
#if GTK_CHECK_VERSION(3,0,0)
        cairo_region_destroy(dst_gc->gc_old_clip_region);
#else
        gdk_region_destroy(dst_gc->gc_old_clip_region);
#endif

    if (src_gc->gc_old_clip_region) {
        dst_gc->gc_old_clip_region =
#if GTK_CHECK_VERSION(3,0,0)
            cairo_region_copy(src_gc->gc_old_clip_region);
#else
            gdk_region_copy(src_gc->gc_old_clip_region);
#endif
    }
    else
        dst_gc->gc_old_clip_region = 0;

    if (src_gc->gc_clip_mask != dst_gc->gc_clip_mask) {
        if (dst_gc->gc_clip_mask) {
            dst_gc->gc_clip_mask->dec_ref();
            dst_gc->gc_clip_mask = 0;
        }
        if (src_gc->gc_clip_mask) {
            dst_gc->gc_clip_mask = src_gc->gc_clip_mask;
            dst_gc->gc_clip_mask->inc_ref();
        }
    }
  
    if (src_gc->gc_old_clip_mask != dst_gc->gc_old_clip_mask) {
        if (dst_gc->gc_old_clip_mask) {
            dst_gc->gc_old_clip_mask->dec_ref();
            dst_gc->gc_old_clip_mask = 0;
        }
        if (src_gc->gc_old_clip_mask) {
            dst_gc->gc_old_clip_mask = src_gc->gc_old_clip_mask;
            dst_gc->gc_old_clip_mask->inc_ref();
        }
    }

    dst_gc->gc_fill = src_gc->gc_fill;
    dst_gc->gc_fill_rule = src_gc->gc_fill_rule;
  
    if (src_gc->gc_stipple != dst_gc->gc_stipple) {
        if (dst_gc->gc_stipple) {
            dst_gc->gc_stipple->dec_ref();
            dst_gc->gc_stipple = 0;
        }
        if (src_gc->gc_stipple) {
            dst_gc->gc_stipple = src_gc->gc_stipple;
            dst_gc->gc_stipple->inc_ref();
        }
    }
  
    if (src_gc->gc_tile != dst_gc->gc_tile) {
        if (dst_gc->gc_tile) {
            dst_gc->gc_tile->dec_ref();
            dst_gc->gc_tile = 0;
        }
        if (src_gc->gc_tile) {
            dst_gc->gc_tile = src_gc->gc_tile;
            dst_gc->gc_tile->inc_ref();
        }
    }

    dst_gc->gc_fg_pixel = src_gc->gc_fg_pixel;
    dst_gc->gc_bg_pixel = src_gc->gc_bg_pixel;
    dst_gc->gc_subwindow_mode = src_gc->gc_subwindow_mode;
    dst_gc->gc_exposures = src_gc->gc_exposures;
}


// Static function.
// Fill in the colors, given the pixel.
//
void
ndkGC::query_rgb(GdkColor *clr, GdkVisual *visual)
{
    int type = gdk_visual_get_visual_type(visual);
    if (type != GDK_VISUAL_TRUE_COLOR && type != GDK_VISUAL_DIRECT_COLOR) {
        clr->red = 0;
        clr->green = 0;
        clr->blue = 0;
        return;
    }

    unsigned int mask;
    int shift, prec;
    gdk_visual_get_red_pixel_details(visual, &mask, &shift, &prec);
    clr->red = 65535 * (double)((clr->pixel & mask) >> shift) /
        ((1 << prec) - 1);

    gdk_visual_get_green_pixel_details(visual, &mask, &shift, &prec);
    clr->green = 65535 * (double)((clr->pixel & mask) >> shift) /
        ((1 << prec) - 1);

    gdk_visual_get_blue_pixel_details(visual, &mask, &shift, &prec);
    clr->blue = 65535 * (double)((clr->pixel & mask) >> shift) /
        ((1 << prec) - 1);
}


// Static function.
// Find the pixel given the colors.
//
void
ndkGC::query_pixel(GdkColor *clr, GdkVisual *visual)
{
    int type = gdk_visual_get_visual_type(visual);
    if (type != GDK_VISUAL_TRUE_COLOR && type != GDK_VISUAL_DIRECT_COLOR) {
        clr->pixel = 0;
        return;
    }

    // Shifting by >= width-of-type isn't defined in C.
    int depth = gdk_visual_get_depth(visual);
    unsigned int padding = depth < 32 ? (~(guint32)0) << depth : 0;
  
    unsigned int mask;
    int shift, prec;
    gdk_visual_get_red_pixel_details(visual, &mask, &shift, &prec);
    clr->pixel = ((clr->red >> (16 - prec)) << shift);
    unsigned int unused = padding;
    unused |= mask;

    gdk_visual_get_green_pixel_details(visual, &mask, &shift, &prec);
    clr->pixel += ((clr->green >> (16 - prec)) << shift);
    unused |= mask;

    gdk_visual_get_blue_pixel_details(visual, &mask, &shift, &prec);
    clr->pixel += ((clr->blue >> (16 - prec)) << shift);
    unused |= mask;

    // If bits not used for color are used for something other than padding,
    // it's likely alpha, so we set them to 1s.
    //
    unused = ~unused;
    clr->pixel += unused;
}


void
ndkGC::clear(GdkWindow *window)
{
    GdkColor clr;
    clr.pixel = gc_bg_pixel;
    set_foreground(&clr);
    set_fill(ndkGC_SOLID);
    draw_rectangle(window, true, 0, 0, gdk_window_get_width(window),
        gdk_window_get_height(window));
}


void
ndkGC::clear(ndkPixmap *pixmap)
{
    GdkColor clr;
    clr.pixel = gc_bg_pixel;
    set_foreground(&clr);
    set_fill(ndkGC_SOLID);
    draw_rectangle(pixmap, true, 0, 0, pixmap->get_width(),
        pixmap->get_height());
}
// END of public methods.


#ifdef WITH_X11

void
ndkGC::draw_line(XID xid, int x1, int y1, int x2, int y2)
{
    XDrawLine(get_xdisplay(), xid, get_xgc(), x1, y1, x2, y2);
}


void
ndkGC::draw_arc(XID xid, bool filled, int x, int y, int w, int h,
    int as, int ae)
{
    if (filled)
        XFillArc(get_xdisplay(), xid, get_xgc(), x, y, w, h, as, ae);
    else
        XDrawArc(get_xdisplay(), xid, get_xgc(), x, y, w, h, as, ae);
}


void
ndkGC::draw_rectangle(XID xid, bool filled, int x, int y, int w, int h)
{
    if (xid == None)
        return;
    if (!filled) {
        int x1 = x;
        int y1 = y;
        int x2 = x1 + w;
        int y2 = y1 + h;
        XDrawLine(get_xdisplay(), xid, get_xgc(), x1, y1, x2, y1);
        XDrawLine(get_xdisplay(), xid, get_xgc(), x2, y1, x2, y2);
        XDrawLine(get_xdisplay(), xid, get_xgc(), x2, y2, x1, y2);
        XDrawLine(get_xdisplay(), xid, get_xgc(), x1, y2, x1, y1);
    }
    else {
        XFillRectangle(get_xdisplay(), xid, get_xgc(), x, y, w, h);
    }
}


void
ndkGC::draw_polygon(XID xid, bool filled, GdkPoint *pts, int npts)
{
    if (npts < 4)
        return;
    if (sizeof(GdkPoint) != sizeof(XPoint)) {
        XPoint *xpts = new XPoint[npts];
        int n = npts;
        while (n--) {
            xpts[n].x = pts[n].x;
            xpts[n].y = pts[n].y;
        }
        if (filled) {
            XFillPolygon(get_xdisplay(), xid, get_xgc(),
                xpts, npts, Complex, CoordModeOrigin);
        }
        else {
            XDrawLines(get_xdisplay(), xid, get_xgc(),
                xpts, npts, CoordModeOrigin);
        }
        delete [] xpts;
    }
    else {
        if (filled) {
            XFillPolygon(get_xdisplay(), xid, get_xgc(),
                (XPoint*)pts, npts, Complex, CoordModeOrigin);
        }
        else {
            XDrawLines(get_xdisplay(), xid, get_xgc(),
                (XPoint*)pts, npts, CoordModeOrigin);
        }
    }
}


void
ndkGC::draw_pango_layout(XID xid, int x, int y, PangoLayout *lout)
{
    int wid, hei;
    pango_layout_get_pixel_size(lout, &wid, &hei);
    if (wid <= 0 || hei <= 0)
        return;
    ndkPixmap *p = new ndkPixmap((GdkWindow*)0, wid, hei);
    p->copy_from_pango_layout(this, x, y, lout);
    XCopyArea(get_xdisplay(), p->get_xid(), xid, get_xgc(),
        0, 0, wid, hei, x, y);
    p->dec_ref();
}


void
ndkGC::gc_x11_set_values(ndkGCvalues *values, ndkGCvaluesMask values_mask)
{
    if (values_mask & (ndkGC_CLIP_X_ORIGIN | ndkGC_CLIP_Y_ORIGIN)) {
        values_mask &= ~(ndkGC_CLIP_X_ORIGIN | ndkGC_CLIP_Y_ORIGIN);
        gc_dirty_mask |= ndkGC_DIRTY_CLIP;
    }

    if (values_mask & (ndkGC_TS_X_ORIGIN | ndkGC_TS_Y_ORIGIN)) {
        values_mask &= ~(ndkGC_TS_X_ORIGIN | ndkGC_TS_Y_ORIGIN);
        gc_dirty_mask |= ndkGC_DIRTY_TS;
    }

    if (values_mask & ndkGC_CLIP_MASK) {
        gc_have_clip_region = FALSE;
        gc_have_clip_mask = (values->v_clip_mask != 0);
    }

    XGCValues xvalues;
    unsigned long xvalues_mask = 0;
    gc_values_to_xvalues(values, values_mask, &xvalues, &xvalues_mask);

    XChangeGC(get_xdisplay(), gc_gc, xvalues_mask, &xvalues);
}


void
ndkGC::gc_x11_get_values(ndkGCvalues *values)
{
    XGCValues xvalues;
    if (XGetGCValues(get_xdisplay(), gc_gc,
            GCForeground | GCBackground | GCFont |
            GCFunction | GCTile | GCStipple | /* GCClipMask | */
            GCSubwindowMode | GCGraphicsExposures |
            GCTileStipXOrigin | GCTileStipYOrigin |
            GCClipXOrigin | GCClipYOrigin |
            GCLineWidth | GCLineStyle | GCCapStyle |
            GCFillStyle | GCJoinStyle, &xvalues)) {
        values->v_foreground.pixel = xvalues.foreground;
        values->v_background.pixel = xvalues.background;

        switch (xvalues.function) {
        case GXcopy:
            values->v_function = ndkGC_COPY;
            break;
        case GXinvert:
            values->v_function = ndkGC_INVERT;
            break;
        case GXxor:
            values->v_function = ndkGC_XOR;
            break;
        case GXclear:
            values->v_function = ndkGC_CLEAR;
            break;
        case GXand:
            values->v_function = ndkGC_AND;
            break;
        case GXandReverse:
            values->v_function = ndkGC_AND_REVERSE;
            break;
        case GXandInverted:
            values->v_function = ndkGC_AND_INVERT;
            break;
        case GXnoop:
            values->v_function = ndkGC_NOOP;
            break;
        case GXor:
            values->v_function = ndkGC_OR;
            break;
        case GXequiv:
            values->v_function = ndkGC_EQUIV;
            break;
        case GXorReverse:
            values->v_function = ndkGC_OR_REVERSE;
            break;
        case GXcopyInverted:
            values->v_function = ndkGC_COPY_INVERT;
            break;
        case GXorInverted:
            values->v_function = ndkGC_OR_INVERT;
            break;
        case GXnand:
            values->v_function = ndkGC_NAND;
            break;
        case GXset:
            values->v_function = ndkGC_SET;
            break;
        case GXnor:
            values->v_function = ndkGC_NOR;
            break;
        }

        switch (xvalues.fill_style) {
        case FillSolid:
            values->v_fill = ndkGC_SOLID;
            break;
        case FillTiled:
            values->v_fill = ndkGC_TILED;
            break;
        case FillStippled:
            values->v_fill = ndkGC_STIPPLED;
            break;
        case FillOpaqueStippled:
            values->v_fill = ndkGC_OPAQUE_STIPPLED;
            break;
        }

        values->v_tile = ndkPixmap::lookup(xvalues.tile);
        values->v_stipple = ndkPixmap::lookup(xvalues.stipple);
        values->v_clip_mask = 0;
        values->v_subwindow_mode = (ndkGCsubwinMode)xvalues.subwindow_mode;
        values->v_ts_x_origin = xvalues.ts_x_origin;
        values->v_ts_y_origin = xvalues.ts_y_origin;
        values->v_clip_x_origin = xvalues.clip_x_origin;
        values->v_clip_y_origin = xvalues.clip_y_origin;
        values->v_graphics_exposures = xvalues.graphics_exposures;
        values->v_line_width = xvalues.line_width;

        switch (xvalues.line_style) {
        case LineSolid:
            values->v_line_style = ndkGC_LINE_SOLID;
            break;
        case LineOnOffDash:
            values->v_line_style = ndkGC_LINE_ON_OFF_DASH;
            break;
        case LineDoubleDash:
            values->v_line_style = ndkGC_LINE_DOUBLE_DASH;
            break;
        }

        switch (xvalues.cap_style) {
        case CapNotLast:
            values->v_cap_style = ndkGC_CAP_NOT_LAST;
            break;
        case CapButt:
            values->v_cap_style = ndkGC_CAP_BUTT;
            break;
        case CapRound:
            values->v_cap_style = ndkGC_CAP_ROUND;
            break;
        case CapProjecting:
            values->v_cap_style = ndkGC_CAP_PROJECTING;
            break;
        }

        switch (xvalues.join_style) {
        case JoinMiter:
            values->v_join_style = ndkGC_JOIN_MITER;
            break;
        case JoinRound:
            values->v_join_style = ndkGC_JOIN_ROUND;
            break;
        case JoinBevel:
            values->v_join_style = ndkGC_JOIN_BEVEL;
            break;
        }
    }
    else {
        memset(values, 0, sizeof(ndkGCvalues));
    }
}


// Static function.
void
ndkGC::gc_values_to_xvalues(ndkGCvalues *values, ndkGCvaluesMask mask,
    XGCValues *xvalues, unsigned long *xvalues_mask)
{
    if (!values || mask == 0)
        return;
  
    if (mask & ndkGC_FOREGROUND) {
        xvalues->foreground = values->v_foreground.pixel;
        *xvalues_mask |= GCForeground;
    }
    if (mask & ndkGC_BACKGROUND) {
        xvalues->background = values->v_background.pixel;
        *xvalues_mask |= GCBackground;
    }
    if (mask & ndkGC_FUNCTION) {
        switch (values->v_function) {
        case ndkGC_COPY:
            xvalues->function = GXcopy;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_INVERT:
            xvalues->function = GXinvert;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_XOR:
            xvalues->function = GXxor;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_CLEAR:
            xvalues->function = GXclear;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_AND:
            xvalues->function = GXand;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_AND_REVERSE:
            xvalues->function = GXandReverse;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_AND_INVERT:
            xvalues->function = GXandInverted;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_NOOP:
            xvalues->function = GXnoop;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_OR:
            xvalues->function = GXor;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_EQUIV:
            xvalues->function = GXequiv;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_OR_REVERSE:
            xvalues->function = GXorReverse;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_COPY_INVERT:
            xvalues->function = GXcopyInverted;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_OR_INVERT:
            xvalues->function = GXorInverted;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_NAND:
            xvalues->function = GXnand;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_SET:
            xvalues->function = GXset;
            *xvalues_mask |= GCFunction;
            break;
        case ndkGC_NOR:
            xvalues->function = GXnor;
            *xvalues_mask |= GCFunction;
        break;
        }
    }
    if (mask & ndkGC_FILL) {
        switch (values->v_fill) {
        case ndkGC_SOLID:
            xvalues->fill_style = FillSolid;
            *xvalues_mask |= GCFillStyle;
            break;
        case ndkGC_TILED:
            xvalues->fill_style = FillTiled;
            *xvalues_mask |= GCFillStyle;
            break;
        case ndkGC_STIPPLED:
            xvalues->fill_style = FillStippled;
            *xvalues_mask |= GCFillStyle;
            break;
        case ndkGC_OPAQUE_STIPPLED:
            xvalues->fill_style = FillOpaqueStippled;
            *xvalues_mask |= GCFillStyle;
            break;
        }
    }
    if (mask & ndkGC_FILL_RULE) {
        switch (values->v_fill_rule) {
        case ndkGC_EVEN_ODD_RULE:
            xvalues->fill_rule = EvenOddRule;
            *xvalues_mask |= GCFillRule;
            break;
        case ndkGC_WINDING_RULE:
            xvalues->fill_rule = WindingRule;
            *xvalues_mask |= GCFillRule;
            break;
        }
    }
    if (mask & ndkGC_TILE) {
        if (values->v_tile)
            xvalues->tile = values->v_tile->get_xid();
        else
            xvalues->tile = None;
        *xvalues_mask |= GCTile;
    }
    if (mask & ndkGC_STIPPLE) {
        if (values->v_stipple)
            xvalues->stipple = values->v_stipple->get_xid();
        else
            xvalues->stipple = None;
        *xvalues_mask |= GCStipple;
    }
    if (mask & ndkGC_CLIP_MASK) {
        if (values->v_clip_mask)
            xvalues->clip_mask = values->v_clip_mask->get_xid();
        else
            xvalues->clip_mask = None;
        *xvalues_mask |= GCClipMask;
      
    }
    if (mask & ndkGC_SUBWINDOW) {
        xvalues->subwindow_mode = values->v_subwindow_mode;
        *xvalues_mask |= GCSubwindowMode;
    }
    if (mask & ndkGC_TS_X_ORIGIN) {
        xvalues->ts_x_origin = values->v_ts_x_origin;
        *xvalues_mask |= GCTileStipXOrigin;
    }
    if (mask & ndkGC_TS_Y_ORIGIN) {
        xvalues->ts_y_origin = values->v_ts_y_origin;
        *xvalues_mask |= GCTileStipYOrigin;
    }
    if (mask & ndkGC_CLIP_X_ORIGIN) {
        xvalues->clip_x_origin = values->v_clip_x_origin;
        *xvalues_mask |= GCClipXOrigin;
    }
    if (mask & ndkGC_CLIP_Y_ORIGIN) {
        xvalues->clip_y_origin = values->v_clip_y_origin;
        *xvalues_mask |= GCClipYOrigin;
    }

    if (mask & ndkGC_EXPOSURES) {
        xvalues->graphics_exposures = values->v_graphics_exposures;
        *xvalues_mask |= GCGraphicsExposures;
    }

    if (mask & ndkGC_LINE_WIDTH) {
        xvalues->line_width = values->v_line_width;
        *xvalues_mask |= GCLineWidth;
    }
    if (mask & ndkGC_LINE_STYLE) {
        switch (values->v_line_style) {
        case ndkGC_LINE_SOLID:
            xvalues->line_style = LineSolid;
            *xvalues_mask |= GCLineStyle;
            break;
        case ndkGC_LINE_ON_OFF_DASH:
            xvalues->line_style = LineOnOffDash;
            *xvalues_mask |= GCLineStyle;
            break;
        case ndkGC_LINE_DOUBLE_DASH:
            xvalues->line_style = LineDoubleDash;
            *xvalues_mask |= GCLineStyle;
            break;
        }
    }
    if (mask & ndkGC_CAP_STYLE) {
        switch (values->v_cap_style) {
        case ndkGC_CAP_NOT_LAST:
            xvalues->cap_style = CapNotLast;
            *xvalues_mask |= GCCapStyle;
            break;
        case ndkGC_CAP_BUTT:
            xvalues->cap_style = CapButt;
            *xvalues_mask |= GCCapStyle;
            break;
        case ndkGC_CAP_ROUND:
            xvalues->cap_style = CapRound;
            *xvalues_mask |= GCCapStyle;
            break;
        case ndkGC_CAP_PROJECTING:
            xvalues->cap_style = CapProjecting;
            *xvalues_mask |= GCCapStyle;
            break;
        }
    }
    if (mask & ndkGC_JOIN_STYLE) {
        switch (values->v_join_style) {
        case ndkGC_JOIN_MITER:
            xvalues->join_style = JoinMiter;
            *xvalues_mask |= GCJoinStyle;
            break;
        case ndkGC_JOIN_ROUND:
            xvalues->join_style = JoinRound;
            *xvalues_mask |= GCJoinStyle;
            break;
        case ndkGC_JOIN_BEVEL:
            xvalues->join_style = JoinBevel;
            *xvalues_mask |= GCJoinStyle;
            break;
        }
    }
}

#endif


#ifdef GDK_IMPORTS
// Is any of this stuff needed?

// Takes ownership of passed-in region.
//
#if GTK_CHECK_VERSION(3,0,0)
void
ndkGC::gc_set_clip_region_real(cairo_region_t *region, bool reset_origin)
#else
void
ndkGC::gc_set_clip_region_real(GdkRegion *region, bool reset_origin)
#endif
{
    if (gc_clip_mask) {
        gc_clip_mask->dec_ref();
        gc_clip_mask = 0;
    }
  
    if (gc_clip_region)
#if GTK_CHECK_VERSION(3,0,0)
        cairo_region_destroy(gc_clip_region);
#else
        gdk_region_destroy(gc_clip_region);
#endif
    gc_clip_region = region;

    gc_windowing_set_clip_region(region, reset_origin);
}


// Doesn't copy region, allows not to reset origin.
//
#if GTK_CHECK_VERSION(3,0,0)
void
ndkGC::gc_set_clip_region_internal(cairo_region_t *region, bool reset_origin)
#else
void
ndkGC::gc_set_clip_region_internal(GdkRegion *region, bool reset_origin)
#endif
{
    gc_remove_drawable_clip();
    gc_set_clip_region_real(region, reset_origin);
}


/*****
void
ndkGC::gc_add_drawable_clip(unsigned int region_tag, GdkRegion *region,
    int offset_x, int offset_y)
{
    if (gc_region_tag_applied == region_tag &&
            offset_x == gc_region_tag_offset_x &&
            offset_y == gc_region_tag_offset_y)
        return; // Already appied this drawable region.
  
    if (gc_region_tag_applied)
        gc_remove_drawable_clip();

    region = gdk_region_copy(region);
    if (offset_x != 0 || offset_y != 0)
        gdk_region_offset(region, offset_x, offset_y);

    if (gc_clip_mask) {
        int w = gc_clip_mask->get_width();
        int h = gc_clip_mask->get_height();

        GdkRectangle r;
        r.x = 0;
        r.y = 0;
        r.width = w;
        r.height = h;

        // Its quite common to expose areas that are completely in or
        // outside the region, so we try to avoid allocating bitmaps
        // that are just fully set or completely unset.
        //
        GdkOverlapType overlap = gdk_region_rect_in(region, &r);
        if (overlap == GDK_OVERLAP_RECTANGLE_PART) {
            // The region and the mask intersect, create a new clip
            // mask that includes both areas.
            gc_old_clip_mask = gc_clip_mask;
            ndkPixmap *new_mask = new ndkPixmap(gc_old_clip_mask, w, h);
            ndkGC *tmp_gc = new ndkGC(new_mask, 0, 0);

            GdkColor black = {0, 0, 0, 0};
            tmp_gc->set_foreground(&black);
            tmp_gc->draw_rectangle(new_mask, true, 0, 0, w, h);
            tmp_gc->gc_set_clip_region_internal(region, true);
            // Takes ownership of region.
            new_mask->copy_from_pixmap(gc_old_clip_mask, tmp_gc, 0, 0, 0, 0,
                -1, -1);
            tmp_gc->set_clip_region(0);
            set_clip_mask(new_mask);
            g_object_unref(new_mask);
        }
        else if (overlap == GDK_OVERLAP_RECTANGLE_OUT) {
            // No intersection, set empty clip region.
            GdkRegion *empty = gdk_region_new();

            gdk_region_destroy(region);
            gc_old_clip_mask = cairo_surface_reference(gc_clip_mask);
            gc_clip_region = empty;
            gc_windowing_set_clip_region(empty, false);
        }
        else {
            // Completely inside region, don't set unnecessary clip.
            gdk_region_destroy(region);
            return;
        }
    }
    else {
        gc_old_clip_region = gc_clip_region;
        gc_clip_region = region;
        if (gc_old_clip_region)
            gdk_region_intersect(region, gc_old_clip_region);

        gc_windowing_set_clip_region(gc_clip_region, false);
    }

    gc_region_tag_applied = region_tag;
    gc_region_tag_offset_x = offset_x;
    gc_region_tag_offset_y = offset_y;
}
*****/


void
ndkGC::gc_remove_drawable_clip()
{
    if (gc_region_tag_applied) {
        gc_region_tag_applied = 0;
        if (gc_old_clip_mask) {
            set_clip_mask(gc_old_clip_mask);
            gc_old_clip_mask->dec_ref();
            gc_old_clip_mask = 0;

            if (gc_clip_region) {
                g_object_unref(gc_clip_region);
                gc_clip_region = 0;
            }
        }
        else {
            gc_set_clip_region_real(gc_old_clip_region, false);
            gc_old_clip_region = 0;
        }
    }
}


namespace {
    cairo_surface_t *make_stipple_tile_surface(cairo_t *cr,
        ndkPixmap *stipple, GdkColor *foreground, GdkColor *background)
    {
        int width = stipple->get_width();
        int height = stipple->get_height();

        cairo_surface_t *sfc = cairo_get_target(cr);
        cairo_surface_t *alpha_surface = cairo_xlib_surface_create(
            cairo_xlib_surface_get_display(sfc), stipple->get_xid(),
            cairo_xlib_surface_get_visual(sfc), width, height);
        cairo_surface_destroy(sfc);
      
        cairo_surface_t *surface = cairo_surface_create_similar(
            cairo_get_target(cr), CAIRO_CONTENT_COLOR_ALPHA, width, height);

        cairo_t *tmp_cr = cairo_create(surface);
        cairo_set_operator(tmp_cr, CAIRO_OPERATOR_SOURCE);
        if (background)
            gdk_cairo_set_source_color(tmp_cr, background);
        else
            cairo_set_source_rgba(tmp_cr, 0, 0, 0 ,0);
        cairo_paint(tmp_cr);

        cairo_set_operator(tmp_cr, CAIRO_OPERATOR_OVER);
        gdk_cairo_set_source_color(tmp_cr, foreground);
        cairo_mask_surface(tmp_cr, alpha_surface, 0, 0);
        cairo_destroy(tmp_cr);
        cairo_surface_destroy(alpha_surface);

        return (surface);
    }
}


// override_foreground:  a foreground color to use to override the
//   foreground color of the GC.
// override_stipple:  a stipple pattern to use to override the stipple
//   from the GC.  If this is present and the fill mode of the GC isn't
//   GC_STIPPLED or GC_OPAQUE_STIPPLED the fill mode will be forced to
//   GC_STIPPLED.
// gc_changed:  pass false if the GC has not changed since the last
//   call to this function.
// target_drawable:  The drawable you're drawing in.  If passed in
//   this is used for client side window clip emulation.
// 
// Set the attributes of a cairo context to match those of the GC as
// far as possible.  Some aspects of a GC, such as clip masks and
// functions other than GDK_COPY are not currently handled.
//
void
ndkGC::gc_update_context(cairo_t *cr, const GdkColor *override_foreground,
    ndkPixmap  *override_stipple, bool gc_changed,
    ndkDrawable *target_drawable)
{
    gc_remove_drawable_clip();

    ndkGCfill fill = (ndkGCfill)gc_fill;
    if (override_stipple && fill != ndkGC_OPAQUE_STIPPLED)
        fill = ndkGC_STIPPLED;

    GdkColor foreground;
    if (fill != ndkGC_TILED) {
        if (override_foreground)
            foreground = *override_foreground;
        else {
            foreground.pixel = gc_fg_pixel;
            gtk_QueryColor(&foreground);
        }
    }

    GdkColor background;
    if (fill == ndkGC_OPAQUE_STIPPLED) {
        background.pixel = gc_bg_pixel;
        gtk_QueryColor(&background);
    }

    ndkPixmap *stipple = 0;
    switch (fill) {
    case ndkGC_SOLID:
        break;
    case ndkGC_TILED:
        if (!gc_tile)
            fill = ndkGC_SOLID;
        break;
    case ndkGC_STIPPLED:
    case ndkGC_OPAQUE_STIPPLED:
        if (override_stipple)
            stipple = override_stipple;
        else
            stipple = gc_stipple;
      
        if (!stipple)
            fill = ndkGC_SOLID;
        break;
    }
  
    cairo_surface_t *tile_surface = 0;
    switch (fill) {
    case ndkGC_SOLID:
        gdk_cairo_set_source_color(cr, &foreground);
        break;
    case ndkGC_TILED:
        tile_surface = cairo_xlib_surface_create(get_xdisplay(),
            gc_tile->get_xid(), gdk_x11_visual_get_xvisual(GRX->Visual()),
            gc_tile->get_width(), gc_tile->get_height());
        break;
    case ndkGC_STIPPLED:
        tile_surface = make_stipple_tile_surface(cr, stipple, &foreground, 0);
        break;
    case ndkGC_OPAQUE_STIPPLED:
        tile_surface = make_stipple_tile_surface(cr, stipple, &foreground,
            &background);
        break;
    }

    // Tiles, stipples, and clip regions are all specified in device
    // space, not user space.  For the clip region, we can simply
    // change the matrix, clip, then clip back, but for the source
    // pattern, we need to compute the right matrix.
    //
    // What we want is:
    //
    //     CTM_inverse * Pattern_matrix = Translate(- ts_x, - ts_y)
    //
    // (So that ts_x, ts_y in device space is taken to 0,0 in pattern
    // space). So, pattern_matrix = CTM * Translate(- ts_x, - tx_y);

    if (tile_surface) {
        cairo_pattern_t *pattern =
            cairo_pattern_create_for_surface(tile_surface);

        cairo_matrix_t user_to_device;
        cairo_get_matrix(cr, &user_to_device);

        cairo_matrix_t device_to_pattern;
        cairo_matrix_init_translate(&device_to_pattern, -gc_ts_x_origin,
            -gc_ts_y_origin);

        cairo_matrix_t user_to_pattern;
        cairo_matrix_multiply(&user_to_pattern, &user_to_device,
            &device_to_pattern);
      
        cairo_pattern_set_matrix(pattern, &user_to_pattern);
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
        cairo_set_source(cr, pattern);
      
        cairo_surface_destroy(tile_surface);
        cairo_pattern_destroy(pattern);
    }

    if (!gc_changed)
        return;

    cairo_reset_clip(cr);
//XXX FIXME WTF?
    // The reset above resets the window clip rect, so we want to re-set
    // that.
    if (target_drawable && target_drawable->get_state() == DW_WINDOW) {
        GdkWindow *window = target_drawable->get_window();
#if GTK_CHECK_VERSION(3,0,0)
//        GdkWindowClass *wc = GDK_WINDOW_GET_CLASS(window);
//        gdk_window_class_set_cairo_clip(window, cr);
#else
        if (window && GDK_DRAWABLE_GET_CLASS(window)->set_cairo_clip)
            GDK_DRAWABLE_GET_CLASS(window)->set_cairo_clip(window, cr);
#endif
    }
    if (gc_clip_region) {
        cairo_save(cr);

        cairo_identity_matrix(cr);
        cairo_translate(cr, gc_clip_x_origin, gc_clip_y_origin);

        cairo_new_path(cr);
        gdk_cairo_region(cr, gc_clip_region);

        cairo_restore(cr);

        cairo_clip(cr);
    }
}


#ifdef WITH_X11

namespace {
    // Adapted this from _gdk_region_get_xrectangles from GDK source.
    //
#if GTK_CHECK_VERSION(3,0,0)
    void region_get_xrectangles(const cairo_region_t *region,
        int x_offset, int y_offset, XRectangle **rects, int *n_rects)
    {
        if (!region) {
            *n_rects = 0;
            *rects = 0;
            return;
        }
        int nrects = cairo_region_num_rectangles(region);
        XRectangle *rectangles = g_new(XRectangle, nrects);
        cairo_rectangle_int_t rect;
        for (int i = 0; i < nrects; i++) {
            cairo_region_get_rectangle(region, i, &rect);
            rectangles[i].x = CLAMP(rect.x + x_offset, G_MINSHORT, G_MAXSHORT);
            rectangles[i].y = CLAMP(rect.y + y_offset, G_MINSHORT, G_MAXSHORT);
            rectangles[i].width = CLAMP(rect.width, G_MINSHORT, G_MAXSHORT);
            rectangles[i].height = CLAMP(rect.height, G_MINSHORT, G_MAXSHORT);
        }
        *rects = rectangles;
        *n_rects = nrects;
    }
#else
    void region_get_xrectangles(const GdkRegion *region,
        int x_offset, int y_offset, XRectangle **rects, int *n_rects)
    {
        struct fakereg  // Must match GdkRegion.
        {
            long size;
            long numRects;
            GdkSegment *rects;
            GdkSegment extents;
        }; 
        fakereg *reg = (fakereg*)(void*)region;
        XRectangle *rectangles = g_new(XRectangle, reg->numRects);
        const GdkSegment *boxes = reg->rects;
        for (int i = 0; i < reg->numRects; i++) {
            rectangles[i].x = CLAMP(boxes[i].x1 + x_offset, G_MINSHORT,
                G_MAXSHORT);
            rectangles[i].y = CLAMP(boxes[i].y1 + y_offset, G_MINSHORT,
                G_MAXSHORT);
            rectangles[i].width = CLAMP(boxes[i].x2 + x_offset, G_MINSHORT,
                G_MAXSHORT) - rectangles[i].x;
            rectangles[i].height = CLAMP(boxes[i].y2 + y_offset, G_MINSHORT,
                G_MAXSHORT) - rectangles[i].y;
        }
        *rects = rectangles;
        *n_rects = reg->numRects;
    }
#endif
}


void
ndkGC::gc_x11_flush()
{
    Display *xdisplay = get_xdisplay();
    if (gc_dirty_mask & ndkGC_DIRTY_CLIP) {
#if GTK_CHECK_VERSION(3,0,0)
        cairo_region_t *clip_region = get_clip_region();
#else
        GdkRegion *clip_region = get_clip_region();
#endif
      
        if (!clip_region)
            XSetClipOrigin(xdisplay, gc_gc, gc_clip_x_origin, gc_clip_y_origin);
        else {
            XRectangle *rectangles;
            int n_rects;
            region_get_xrectangles(clip_region, gc_clip_x_origin,
                gc_clip_y_origin, &rectangles, &n_rects);
      
            XSetClipRectangles(xdisplay, gc_gc, 0, 0, rectangles, n_rects,
                YXBanded);
            g_free(rectangles);
        }
    }

    if (gc_dirty_mask & ndkGC_DIRTY_TS) {
        XSetTSOrigin(xdisplay, gc_gc, gc_ts_x_origin, gc_ts_y_origin);
    }
    gc_dirty_mask = 0;
}


#if GTK_CHECK_VERSION(3,0,0)
void
ndkGC::gc_windowing_set_clip_region(const cairo_region_t *region,
    bool reset_origin)
#else
void
ndkGC::gc_windowing_set_clip_region(const GdkRegion *region, bool reset_origin)
#endif
{
    // Unset immediately, to make sure Xlib doesn't keep the XID of an
    // old clip mask cached.
    //
    if ((gc_have_clip_region && !region) || gc_have_clip_mask) {
        XSetClipMask(get_xdisplay(), gc_gc, None);
        gc_have_clip_mask = false;
    }
    gc_have_clip_region = (region != 0);

    if (reset_origin) {
        gc_clip_x_origin = 0;
        gc_clip_y_origin = 0;
    }
    gc_dirty_mask |= ndkGC_DIRTY_CLIP;
}


// Static function.
void
ndkGC::gc_windowing_copy(ndkGC *dst_gc, ndkGC *src_gc)
{
    XCopyGC(src_gc->get_xdisplay(), src_gc->get_xgc(), ~((~1U) << GCLastBit),
        dst_gc->get_xgc());

    dst_gc->gc_dirty_mask = src_gc->gc_dirty_mask;
    dst_gc->gc_have_clip_region = src_gc->gc_have_clip_region;
    dst_gc->gc_have_clip_mask = src_gc->gc_have_clip_mask;
}

#endif  // WITH_X11

#endif  // GDK_IMPORTS

#endif  // NDKGC_H

