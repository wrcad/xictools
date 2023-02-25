
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

// gtkgc.cc:  This derives from the gdkgc.c from GTK-2.0.  It supports a
// Gc class which is a replacement for the GdkGC but which fits into the
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

#include <gtkinterf.h>
#ifdef WITH_X11
#include "gtkx11.h"
#endif
#include "gtkgc.h"
//#include <string.h>


Gc::Gc(GdkWindow *window, GcValues *values, GcValuesMask values_mask)
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

    gc_stipple              = 0;;
    gc_tile                 = 0;
    gc_clip_mask            = 0;

    gc_colormap             = 0;
    // These are the default X11 value, which we match.  They are
    // clearly wrong for TrueColor displays, so apps have to change
    // them.
    //
    gc_fg_pixel             = 0;
    gc_bg_pixel             = 1;

#ifdef WITH_X11
    gc_gc                   = 0;
    gc_screen               = 0;
    gc_dirty_mask           = 0;
    gc_have_clip_region     = false;
    gc_have_clip_mask       = false;
    gc_depth                = 0;
#endif
    gc_subwindow_mode       = false;
    gc_fill                 = GC_SOLID;
    gc_exposures            = false;

    if (values) {
        if (values_mask & GC_CLIP_X_ORIGIN)
            gc_clip_x_origin = values->v_clip_x_origin;
        if (values_mask & GC_CLIP_Y_ORIGIN)
            gc_clip_y_origin = values->v_clip_y_origin;
        if (values_mask & GC_TS_X_ORIGIN)
            gc_ts_x_origin = values->v_ts_x_origin;
        if (values_mask & GC_TS_Y_ORIGIN)
            gc_ts_y_origin = values->v_ts_y_origin;

        if (values_mask & GC_STIPPLE) {
            gc_stipple = values->v_stipple;
            if (gc_stipple)
                g_object_ref(gc_stipple);
        }
        if (values_mask & GC_TILE) {
            gc_tile = values->v_tile;
            if (gc_tile)
                g_object_ref(gc_tile);
        }
        if ((values_mask & GC_CLIP_MASK) && values->v_clip_mask)
            gc_clip_mask = g_object_ref(values->v_clip_mask);

        if (values_mask & GC_FOREGROUND)
            gc_fg_pixel = values->v_foreground.pixel;
        if (values_mask & GC_BACKGROUND)
            gc_bg_pixel = values->v_background.pixel;

        if (values_mask & GC_SUBWINDOW)
            gc_subwindow_mode = values->v_subwindow_mode;
        if (values_mask & GC_FILL)
            gc_fill = values->v_fill;
        if (values_mask & GC_EXPOSURES)
            gc_exposures = values->v_graphics_exposures;
        else
            gc_exposures = true;

        gc_colormap = gdk_drawable_get_colormap(window);
        if (gc_colormap)
            g_object_ref(gc_colormap);
    }

#ifdef WITH_X11
    gc_screen = gdk_window_get_screen(window);

    gc_depth = gdk_drawable_get_depth(window);

    gc_dirty_mask = 0;
    if (values_mask & (GC_CLIP_X_ORIGIN | GC_CLIP_Y_ORIGIN)) {
        unsigned int foo = values_mask;
        foo &= ~(GC_CLIP_X_ORIGIN | GC_CLIP_Y_ORIGIN);
        values_mask = (GcValuesMask)foo;
        gc_dirty_mask |= GC_DIRTY_CLIP;
    }

    if (values_mask & (GC_TS_X_ORIGIN | GC_TS_Y_ORIGIN)) {
        unsigned int foo = values_mask;
        foo &= ~(GC_TS_X_ORIGIN | GC_TS_Y_ORIGIN);
        values_mask = (GcValuesMask)foo;
        gc_dirty_mask |= GC_DIRTY_TS;
    }

    gc_have_clip_mask = false;
    if ((values_mask & GC_CLIP_MASK) && values->v_clip_mask)
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
        GDK_DRAWABLE_IMPL_X11(window)->xid, xvalues_mask, &xvalues);
#endif
}


Gc::~Gc()
{
#ifdef HAVE_X11
    if (gc_gc)
        XFreeGC(get_xdisplay(), gc_gc);
#endif

    if (gc_clip_region)
        gdk_region_destroy(gc_clip_region);
    if (gc_old_clip_region)
        gdk_region_destroy(gc_old_clip_region);
    if (gc_clip_mask)
        g_object_unref(gc_clip_mask);
    if (gc_old_clip_mask)
        g_object_unref(gc_old_clip_mask);
    if (gc_colormap)
        g_object_unref(gc_colormap);
    if (gc_tile)
        g_object_unref(gc_tile);
    if (gc_stipple)
        g_object_unref(gc_stipple);
}


void
Gc::set_values(GcValues *values, GcValuesMask values_mask)
{
    if ((values_mask & GC_CLIP_X_ORIGIN) ||
            (values_mask & GC_CLIP_Y_ORIGIN) ||
            (values_mask & GC_CLIP_MASK) ||
            (values_mask & GC_SUBWINDOW))
        gc_remove_drawable_clip();
  
    if (values_mask & GC_CLIP_X_ORIGIN)
        gc_clip_x_origin = values->v_clip_x_origin;
    if (values_mask & GC_CLIP_Y_ORIGIN)
        gc_clip_y_origin = values->v_clip_y_origin;
    if (values_mask & GC_TS_X_ORIGIN)
        gc_ts_x_origin = values->v_ts_x_origin;
    if (values_mask & GC_TS_Y_ORIGIN)
        gc_ts_y_origin = values->v_ts_y_origin;

    if (values_mask & GC_CLIP_MASK) {
        if (gc_clip_mask) {
            g_object_unref(gc_clip_mask);
            gc_clip_mask = 0;
        }
        if (values->v_clip_mask)
            gc_clip_mask = g_object_ref(values->v_clip_mask);
      
        if (gc_clip_region) {
            gdk_region_destroy(gc_clip_region);
            gc_clip_region = 0;
        }
    }
    if (values_mask & GC_FILL)
        gc_fill = values->v_fill;
    if (values_mask & GC_STIPPLE) {
        if (gc_stipple != values->v_stipple) {
            if (gc_stipple)
                g_object_unref(gc_stipple);
            gc_stipple = values->v_stipple;
            if (gc_stipple)
                g_object_ref(gc_stipple);
        }
    }
    if (values_mask & GC_TILE) {
        if (gc_tile != values->v_tile) {
            if (gc_tile)
                g_object_unref(gc_tile);
            gc_tile = values->v_tile;
            if (gc_tile)
                g_object_ref(gc_tile);
        }
    }
    if (values_mask & GC_FOREGROUND)
        gc_fg_pixel = values->v_foreground.pixel;
    if (values_mask & GC_BACKGROUND)
        gc_bg_pixel = values->v_background.pixel;
    if (values_mask & GC_SUBWINDOW)
        gc_subwindow_mode = values->v_subwindow_mode;
    if (values_mask & GC_EXPOSURES)
        gc_exposures = values->v_graphics_exposures;
  
#ifdef WITH_X11
    gc_x11_set_values(values, values_mask);
#endif
}


void
Gc::get_values(GcValues *values)
{
#ifdef WITH_X11
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
Gc::offset(int x_offset, int y_offset)
{
    if (x_offset != 0 || y_offset != 0) {
        GcValues values;
        values.v_clip_x_origin = gc_clip_x_origin - x_offset;
        values.v_clip_y_origin = gc_clip_y_origin - y_offset;
        values.v_ts_x_origin = gc_ts_x_origin - x_offset;
        values.v_ts_y_origin = gc_ts_y_origin - y_offset;
      
        set_values(&values, (GcValuesMask)(GC_CLIP_X_ORIGIN |
             GC_CLIP_Y_ORIGIN | GC_TS_X_ORIGIN | GC_TS_Y_ORIGIN));
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
Gc::copy(Gc *dst_gc, Gc *src_gc)
{
//XXX    _gdk_windowing_gc_copy(dst_gc, src_gc);

    dst_gc->gc_clip_x_origin = src_gc->gc_clip_x_origin;
    dst_gc->gc_clip_y_origin = src_gc->gc_clip_y_origin;
    dst_gc->gc_ts_x_origin = src_gc->gc_ts_x_origin;
    dst_gc->gc_ts_y_origin = src_gc->gc_ts_y_origin;

    if (src_gc->gc_colormap)
        g_object_ref(src_gc->gc_colormap);
    if (dst_gc->gc_colormap)
        g_object_unref(dst_gc->gc_colormap);
    dst_gc->gc_colormap = src_gc->gc_colormap;

    if (dst_gc->gc_clip_region)
        gdk_region_destroy(dst_gc->gc_clip_region);

    if (src_gc->gc_clip_region)
        dst_gc->gc_clip_region = gdk_region_copy(src_gc->gc_clip_region);
    else
        dst_gc->gc_clip_region = 0;

    dst_gc->gc_region_tag_applied = src_gc->gc_region_tag_applied;
  
    if (dst_gc->gc_old_clip_region)
        gdk_region_destroy(dst_gc->gc_old_clip_region);

    if (src_gc->gc_old_clip_region) {
        dst_gc->gc_old_clip_region =
            gdk_region_copy(src_gc->gc_old_clip_region);
    }
    else
        dst_gc->gc_old_clip_region = 0;

    if (src_gc->gc_clip_mask)
        dst_gc->gc_clip_mask = g_object_ref(src_gc->gc_clip_mask);
    else
        dst_gc->gc_clip_mask = 0;
  
    if (src_gc->gc_old_clip_mask)
        dst_gc->gc_old_clip_mask = g_object_ref(src_gc->gc_old_clip_mask);
    else
        dst_gc->gc_old_clip_mask = 0;
  
    dst_gc->gc_fill = src_gc->gc_fill;
  
    if (dst_gc->gc_stipple)
        g_object_unref(dst_gc->gc_stipple);
    dst_gc->gc_stipple = src_gc->gc_stipple;
    if (dst_gc->gc_stipple)
        g_object_ref(dst_gc->gc_stipple);
  
    if (dst_gc->gc_tile)
        g_object_unref(dst_gc->gc_tile);
    dst_gc->gc_tile = src_gc->gc_tile;
    if (dst_gc->gc_tile)
        g_object_ref(dst_gc->gc_tile);

    dst_gc->gc_fg_pixel = src_gc->gc_fg_pixel;
    dst_gc->gc_bg_pixel = src_gc->gc_bg_pixel;
    dst_gc->gc_subwindow_mode = src_gc->gc_subwindow_mode;
    dst_gc->gc_exposures = src_gc->gc_exposures;
}


// END of public methods.


// Takes ownership of passed-in region.
//
void
Gc::gc_set_clip_region_real(GdkRegion *region, bool reset_origin)
{
    if (gc_clip_mask) {
        g_object_unref(gc_clip_mask);
        gc_clip_mask = 0;
    }
  
    if (gc_clip_region)
        gdk_region_destroy(gc_clip_region);
    gc_clip_region = region;

//XXX    gc_gdk_windowing_gc_set_clip_region(region, reset_origin);
}


// Doesn't copy region, allows not to reset origin.
//
void
Gc::gc_set_clip_region_internal(GdkRegion *region, bool reset_origin)
{
    gc_remove_drawable_clip();
    gc_set_clip_region_real(region, reset_origin);
}


void
Gc::gc_add_drawable_clip(unsigned int region_tag, GdkRegion *region,
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
        int w, h;
        GcPixmap *new_mask;
        Gc *tmp_gc;
        GdkColor black = {0, 0, 0, 0};
        GdkRectangle r;
        GdkOverlapType overlap;

//XXX        gdk_drawable_get_size (gc_clip_mask, &w, &h);

        r.x = 0;
        r.y = 0;
        r.width = w;
        r.height = h;

        // Its quite common to expose areas that are completely in or
        // outside the region, so we try to avoid allocating bitmaps
        // that are just fully set or completely unset.
        //
        overlap = gdk_region_rect_in(region, &r);
        if (overlap == GDK_OVERLAP_RECTANGLE_PART) {
            // The region and the mask intersect, create a new clip
            // mask that includes both areas.
            gc_old_clip_mask = g_object_ref(gc_clip_mask);
//XXX            new_mask = gdk_pixmap_new(gc_old_clip_mask, w, h, -1);
//XXX            tmp_gc = _gdk_drawable_get_scratch_gc((GdkDrawable *)new_mask, false);

            tmp_gc->set_foreground(&black);
//XXX            gdk_draw_rectangle(new_mask, tmp_gc, true, 0, 0, -1, -1);
            tmp_gc->gc_set_clip_region_internal(region, true);
            // Takes ownership of region.
//XXX            gdk_draw_drawable(new_mask, tmp_gc, gc_old_clip_mask, 0, 0, 0, 0,
//XXX                -1, -1);
            tmp_gc->set_clip_region(NULL);
            set_clip_mask(new_mask);
            g_object_unref(new_mask);
        }
        else if (overlap == GDK_OVERLAP_RECTANGLE_OUT) {
            // No intersection, set empty clip region.
            GdkRegion *empty = gdk_region_new();

            gdk_region_destroy(region);
            gc_old_clip_mask = g_object_ref(gc_clip_mask);
            gc_clip_region = empty;
//XXX            _gdk_windowing_gc_set_clip_region(gc, empty, false);
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

//XXX        _gdk_windowing_gc_set_clip_region(gc, gc_clip_region, false);
    }

    gc_region_tag_applied = region_tag;
    gc_region_tag_offset_x = offset_x;
    gc_region_tag_offset_y = offset_y;
}


void
Gc::gc_remove_drawable_clip()
{
    if (gc_region_tag_applied) {
        gc_region_tag_applied = 0;
        if (gc_old_clip_mask) {
            set_clip_mask(gc_old_clip_mask);
            g_object_unref(gc_old_clip_mask);
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


GdkColormap *
Gc::get_colormap_warn()
{
    if (!gc_colormap) {
        g_warning (
    "Gc::set_rgb_fg_color() Gc::set_rgb_bg_color() can only be used on\n"
    "GC's with a colormap. A GC will have a colormap if it is created for\n"
    "a drawable with a colormap, or if a colormap has been set explicitly\n"
    "with Gc::set_colormap.\n");
        return (0);
    }

    return (gc_colormap);
}


namespace {
    cairo_surface_t *make_stipple_tile_surface(cairo_t *cr, GcBitmap *stipple,
        GdkColor *foreground, GdkColor *background)
    {

        int width, height;
//XXX        gdk_drawable_get_size(stipple, &width, &height);
      
        cairo_surface_t *alpha_surface = 
//XXX            _gdk_drawable_ref_cairo_surface(stipple);
0;
      
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
Gc::gc_update_context(cairo_t *cr, const GdkColor *override_foreground,
    GcBitmap *override_stipple, bool gc_changed, GdkDrawable *target_drawable)
{
    gc_remove_drawable_clip();

    GcFill fill = (GcFill)gc_fill;
    if (override_stipple && fill != GC_OPAQUE_STIPPLED)
        fill = GC_STIPPLED;

    GdkColor foreground;
    if (fill != GC_TILED) {
        if (override_foreground)
            foreground = *override_foreground;
        else
            get_foreground(&foreground);
    }

    GdkColor background;
    if (fill == GC_OPAQUE_STIPPLED)
        get_background(&background);

    GcBitmap *stipple = 0;
    switch (fill) {
    case GC_SOLID:
        break;
    case GC_TILED:
        if (!gc_tile)
            fill = GC_SOLID;
        break;
    case GC_STIPPLED:
    case GC_OPAQUE_STIPPLED:
        if (override_stipple)
            stipple = override_stipple;
        else
            stipple = gc_stipple;
      
        if (!stipple)
            fill = GC_SOLID;
        break;
    }
  
    cairo_surface_t *tile_surface = 0;
    switch (fill) {
    case GC_SOLID:
        gdk_cairo_set_source_color(cr, &foreground);
        break;
    case GC_TILED:
//XXX        tile_surface = gc_gdk_drawable_ref_cairo_surface(gc_tile);
        break;
    case GC_STIPPLED:
        tile_surface = make_stipple_tile_surface(cr, stipple, &foreground, 0);
        break;
    case GC_OPAQUE_STIPPLED:
        tile_surface = make_stipple_tile_surface (cr, stipple, &foreground,
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
    // The reset above resets the window clip rect, so we want to re-set
    // that.
    if (target_drawable &&
            GDK_DRAWABLE_GET_CLASS(target_drawable)->set_cairo_clip)
        GDK_DRAWABLE_GET_CLASS(target_drawable)->set_cairo_clip(
            target_drawable, cr);

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

//////////////////////////

#ifdef WITH_X11


void
Gc::gc_x11_flush()
{
    Display *xdisplay = get_xdisplay();
    if (gc_dirty_mask & GC_DIRTY_CLIP) {
        GdkRegion *clip_region = get_clip_region();
      
        if (!clip_region)
            XSetClipOrigin(xdisplay, gc_gc, gc_clip_x_origin, gc_clip_y_origin);
        else {
            XRectangle *rectangles;
            int n_rects;
            _gdk_region_get_xrectangles(clip_region, gc_clip_x_origin,
                gc_clip_y_origin, &rectangles, &n_rects);
      
            XSetClipRectangles(xdisplay, gc_gc, 0, 0, rectangles, n_rects,
                YXBanded);
            g_free (rectangles);
        }
    }

    if (gc_dirty_mask & GC_DIRTY_TS) {
        XSetTSOrigin(xdisplay, gc_gc, gc_ts_x_origin, gc_ts_y_origin);
    }
    gc_dirty_mask = 0;
}


void
Gc::gc_x11_set_values(GcValues *values, GcValuesMask values_mask)
{
    if (values_mask & (GC_CLIP_X_ORIGIN | GC_CLIP_Y_ORIGIN)) {
        int foo = values_mask;
        foo &= ~(GC_CLIP_X_ORIGIN | GC_CLIP_Y_ORIGIN);
        values_mask = (GcValuesMask)foo;
        gc_dirty_mask |= GC_DIRTY_CLIP;
    }

    if (values_mask & (GC_TS_X_ORIGIN | GC_TS_Y_ORIGIN)) {
        int foo = values_mask;
        foo &= ~(GC_TS_X_ORIGIN | GC_TS_Y_ORIGIN);
        values_mask = (GcValuesMask)foo;
        gc_dirty_mask |= GC_DIRTY_TS;
    }

    if (values_mask & GC_CLIP_MASK) {
        gc_have_clip_region = FALSE;
        gc_have_clip_mask = (values->v_clip_mask != 0);
    }

    XGCValues xvalues;
    unsigned long xvalues_mask = 0;
    gc_values_to_xvalues(values, values_mask, &xvalues, &xvalues_mask);

    XChangeGC(get_xdisplay(), gc_gc, xvalues_mask, &xvalues);
}


void
Gc::gc_x11_get_values(GcValues *values)
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
//        values->font = gdk_font_lookup_for_display(get_xdisplay(),
//            xvalues.font);

        switch (xvalues.function) {
        case GXcopy:
            values->v_function = GC_COPY;
            break;
        case GXinvert:
            values->v_function = GC_INVERT;
            break;
        case GXxor:
            values->v_function = GC_XOR;
            break;
        case GXclear:
            values->v_function = GC_CLEAR;
            break;
        case GXand:
            values->v_function = GC_AND;
            break;
        case GXandReverse:
            values->v_function = GC_AND_REVERSE;
            break;
        case GXandInverted:
            values->v_function = GC_AND_INVERT;
            break;
        case GXnoop:
            values->v_function = GC_NOOP;
            break;
        case GXor:
            values->v_function = GC_OR;
            break;
        case GXequiv:
            values->v_function = GC_EQUIV;
            break;
        case GXorReverse:
            values->v_function = GC_OR_REVERSE;
            break;
        case GXcopyInverted:
            values->v_function = GC_COPY_INVERT;
            break;
        case GXorInverted:
            values->v_function = GC_OR_INVERT;
            break;
        case GXnand:
            values->v_function = GC_NAND;
            break;
        case GXset:
            values->v_function = GC_SET;
            break;
        case GXnor:
            values->v_function = GC_NOR;
            break;
        }

        switch (xvalues.fill_style) {
        case FillSolid:
            values->v_fill = GC_SOLID;
            break;
        case FillTiled:
            values->v_fill = GC_TILED;
            break;
        case FillStippled:
            values->v_fill = GC_STIPPLED;
            break;
        case FillOpaqueStippled:
            values->v_fill = GC_OPAQUE_STIPPLED;
            break;
        }

        values->v_tile = gdk_pixmap_lookup_for_display(
            gdk_screen_get_display(gc_screen), xvalues.tile);
        values->v_stipple = gdk_pixmap_lookup_for_display(
            gdk_screen_get_display(gc_screen), xvalues.stipple);
        values->v_clip_mask = 0;
        values->v_subwindow_mode = (GcSubwindowMode)xvalues.subwindow_mode;
        values->v_ts_x_origin = xvalues.ts_x_origin;
        values->v_ts_y_origin = xvalues.ts_y_origin;
        values->v_clip_x_origin = xvalues.clip_x_origin;
        values->v_clip_y_origin = xvalues.clip_y_origin;
        values->v_graphics_exposures = xvalues.graphics_exposures;
        values->v_line_width = xvalues.line_width;

        switch (xvalues.line_style) {
        case LineSolid:
            values->v_line_style = GC_LINE_SOLID;
            break;
        case LineOnOffDash:
            values->v_line_style = GC_LINE_ON_OFF_DASH;
            break;
        case LineDoubleDash:
            values->v_line_style = GC_LINE_DOUBLE_DASH;
            break;
        }

        switch (xvalues.cap_style) {
        case CapNotLast:
            values->v_cap_style = GC_CAP_NOT_LAST;
            break;
        case CapButt:
            values->v_cap_style = GC_CAP_BUTT;
            break;
        case CapRound:
            values->v_cap_style = GC_CAP_ROUND;
            break;
        case CapProjecting:
            values->v_cap_style = GC_CAP_PROJECTING;
            break;
        }

        switch (xvalues.join_style) {
        case JoinMiter:
            values->v_join_style = GC_JOIN_MITER;
            break;
        case JoinRound:
            values->v_join_style = GC_JOIN_ROUND;
            break;
        case JoinBevel:
            values->v_join_style = GC_JOIN_BEVEL;
            break;
        }
    }
    else {
        memset(values, 0, sizeof(GcValues));
    }
}


// Static function.
void
Gc::gc_values_to_xvalues(GcValues *values, GcValuesMask mask,
    XGCValues *xvalues, unsigned long *xvalues_mask)
{
    if (!values || mask == 0)
        return;
  
    if (mask & GC_FOREGROUND) {
        xvalues->foreground = values->v_foreground.pixel;
        *xvalues_mask |= GCForeground;
    }
    if (mask & GC_BACKGROUND) {
        xvalues->background = values->v_background.pixel;
        *xvalues_mask |= GCBackground;
    }
    /*
    if ((mask & GC_FONT) && (values->font->type == GDK_FONT_FONT))
    {
      xvalues->font = ((XFontStruct *) (GDK_FONT_XFONT (values->font)))->fid;
      *xvalues_mask |= GCFont;
    }
    */
    if (mask & GC_FUNCTION) {
        switch (values->v_function) {
        case GC_COPY:
            xvalues->function = GXcopy;
            break;
        case GC_INVERT:
            xvalues->function = GXinvert;
            break;
        case GC_XOR:
            xvalues->function = GXxor;
            break;
        case GC_CLEAR:
            xvalues->function = GXclear;
            break;
        case GC_AND:
            xvalues->function = GXand;
            break;
        case GC_AND_REVERSE:
            xvalues->function = GXandReverse;
            break;
        case GC_AND_INVERT:
            xvalues->function = GXandInverted;
            break;
        case GC_NOOP:
            xvalues->function = GXnoop;
            break;
        case GC_OR:
            xvalues->function = GXor;
            break;
        case GC_EQUIV:
            xvalues->function = GXequiv;
            break;
        case GC_OR_REVERSE:
            xvalues->function = GXorReverse;
            break;
        case GC_COPY_INVERT:
            xvalues->function = GXcopyInverted;
            break;
        case GC_OR_INVERT:
            xvalues->function = GXorInverted;
            break;
        case GC_NAND:
            xvalues->function = GXnand;
            break;
        case GC_SET:
            xvalues->function = GXset;
            break;
        case GC_NOR:
            xvalues->function = GXnor;
        break;
        }
        *xvalues_mask |= GCFunction;
    }
    if (mask & GC_FILL) {
        switch (values->v_fill) {
        case GC_SOLID:
            xvalues->fill_style = FillSolid;
            break;
        case GC_TILED:
            xvalues->fill_style = FillTiled;
            break;
        case GC_STIPPLED:
            xvalues->fill_style = FillStippled;
            break;
        case GC_OPAQUE_STIPPLED:
            xvalues->fill_style = FillOpaqueStippled;
            break;
        }
        *xvalues_mask |= GCFillStyle;
    }
    if (mask & GC_TILE) {
        if (values->v_tile)
            xvalues->tile = GDK_DRAWABLE_XID (values->v_tile);
        else
            xvalues->tile = None;
      
        *xvalues_mask |= GCTile;
    }
    if (mask & GC_STIPPLE) {
        if (values->v_stipple)
            xvalues->stipple = GDK_DRAWABLE_XID (values->v_stipple);
        else
            xvalues->stipple = None;
      
        *xvalues_mask |= GCStipple;
    }
    if (mask & GC_CLIP_MASK) {
        if (values->v_clip_mask)
            xvalues->clip_mask = GDK_DRAWABLE_XID (values->v_clip_mask);
        else
            xvalues->clip_mask = None;

        *xvalues_mask |= GCClipMask;
      
    }
    if (mask & GC_SUBWINDOW) {
        xvalues->subwindow_mode = values->v_subwindow_mode;
        *xvalues_mask |= GCSubwindowMode;
    }
    if (mask & GC_TS_X_ORIGIN) {
        xvalues->ts_x_origin = values->v_ts_x_origin;
        *xvalues_mask |= GCTileStipXOrigin;
    }
    if (mask & GC_TS_Y_ORIGIN) {
        xvalues->ts_y_origin = values->v_ts_y_origin;
        *xvalues_mask |= GCTileStipYOrigin;
    }
    if (mask & GC_CLIP_X_ORIGIN) {
        xvalues->clip_x_origin = values->v_clip_x_origin;
        *xvalues_mask |= GCClipXOrigin;
    }
    if (mask & GC_CLIP_Y_ORIGIN) {
        xvalues->clip_y_origin = values->v_clip_y_origin;
        *xvalues_mask |= GCClipYOrigin;
    }

    if (mask & GC_EXPOSURES) {
        xvalues->graphics_exposures = values->v_graphics_exposures;
        *xvalues_mask |= GCGraphicsExposures;
    }

    if (mask & GC_LINE_WIDTH) {
        xvalues->line_width = values->v_line_width;
        *xvalues_mask |= GCLineWidth;
    }
    if (mask & GC_LINE_STYLE) {
        switch (values->v_line_style) {
        case GC_LINE_SOLID:
            xvalues->line_style = LineSolid;
            break;
        case GC_LINE_ON_OFF_DASH:
            xvalues->line_style = LineOnOffDash;
            break;
        case GC_LINE_DOUBLE_DASH:
            xvalues->line_style = LineDoubleDash;
            break;
        }
        *xvalues_mask |= GCLineStyle;
    }
    if (mask & GC_CAP_STYLE) {
        switch (values->v_cap_style) {
        case GC_CAP_NOT_LAST:
            xvalues->cap_style = CapNotLast;
            break;
        case GC_CAP_BUTT:
            xvalues->cap_style = CapButt;
            break;
        case GC_CAP_ROUND:
            xvalues->cap_style = CapRound;
            break;
        case GC_CAP_PROJECTING:
            xvalues->cap_style = CapProjecting;
            break;
        }
        *xvalues_mask |= GCCapStyle;
    }
    if (mask & GC_JOIN_STYLE) {
        switch (values->v_join_style) {
        case GC_JOIN_MITER:
            xvalues->join_style = JoinMiter;
            break;
        case GC_JOIN_ROUND:
            xvalues->join_style = JoinRound;
            break;
        case GC_JOIN_BEVEL:
            xvalues->join_style = JoinBevel;
            break;
        }
        *xvalues_mask |= GCJoinStyle;
    }
}


void
Gc::gc_windowing_set_clip_region(const GdkRegion *region, bool reset_origin)
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

    gc_dirty_mask |= GC_DIRTY_CLIP;
}


// Static function.
void
Gc::gc_windowing_copy(Gc *dst_gc, Gc *src_gc)
{
    XCopyGC(src_gc->get_xdisplay(), src_gc->get_xgc(), ~((~1) << GCLastBit),
        dst_gc->get_xgc());

    dst_gc->gc_dirty_mask = src_gc->gc_dirty_mask;
    dst_gc->gc_have_clip_region = src_gc->gc_have_clip_region;
    dst_gc->gc_have_clip_mask = src_gc->gc_have_clip_mask;
}

#endif

