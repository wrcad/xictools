
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

#ifdef NEW_DRW
ndkGC::ndkGC(ndkDrawable *drawable, ndkGCvalues *values,
    ndkGCvaluesMask values_mask)
#else
ndkGC::ndkGC(GdkDrawable *drawable, ndkGCvalues *values,
    ndkGCvaluesMask values_mask)
#endif
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
#ifdef NEW_PIX
            if (gc_stipple)
                gc_stipple->inc_ref();
#else
            if (gc_stipple)
                gdk_pixmap_ref(gc_stipple);
#endif
        }
        if (values_mask & ndkGC_TILE) {
            gc_tile = values->v_tile;
#ifdef NEW_PIX
            if (gc_tile)
                gc_tile->inc_ref();
#else
            if (gc_tile)
                gdk_pixmap_ref(gc_tile);
#endif
        }
        if (values_mask & ndkGC_CLIP_MASK) {
            gc_clip_mask = values->v_clip_mask;
#ifdef NEW_PIX
            if (gc_clip_mask)
                gc_clip_mask->inc_ref();
#else
            if (gc_clip_mask)
                gdk_pixmap_ref(gc_clip_mask);
#endif
        }

        if (values_mask & ndkGC_FOREGROUND)
            gc_fg_pixel = values->v_foreground.pixel;
        if (values_mask & ndkGC_BACKGROUND)
            gc_bg_pixel = values->v_background.pixel;

        if (values_mask & ndkGC_SUBWINDOW)
            gc_subwindow_mode = values->v_subwindow_mode;
        if (values_mask & ndkGC_FILL)
            gc_fill = values->v_fill;
        if (values_mask & ndkGC_EXPOSURES)
            gc_exposures = values->v_graphics_exposures;
        else
            gc_exposures = true;
    }

#ifdef WITH_X11
    gc_screen = gdk_window_get_screen(drawable);
    gc_depth = gdk_drawable_get_depth(drawable);

    gc_dirty_mask = 0;
    if (values_mask & (ndkGC_CLIP_X_ORIGIN | ndkGC_CLIP_Y_ORIGIN)) {
        unsigned int foo = values_mask;
        foo &= ~(ndkGC_CLIP_X_ORIGIN | ndkGC_CLIP_Y_ORIGIN);
        values_mask = (ndkGCvaluesMask)foo;
        gc_dirty_mask |= ndkGC_DIRTY_CLIP;
    }

    if (values_mask & (ndkGC_TS_X_ORIGIN | ndkGC_TS_Y_ORIGIN)) {
        unsigned int foo = values_mask;
        foo &= ~(ndkGC_TS_X_ORIGIN | ndkGC_TS_Y_ORIGIN);
        values_mask = (ndkGCvaluesMask)foo;
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
        gdk_x11_drawable_get_xid(drawable), xvalues_mask, &xvalues);
#endif
}


ndkGC::~ndkGC()
{
#ifdef HAVE_X11
    if (gc_gc)
        XFreeGC(get_xdisplay(), gc_gc);
#endif

    if (gc_clip_region)
        gdk_region_destroy(gc_clip_region);
    if (gc_old_clip_region)
        gdk_region_destroy(gc_old_clip_region);
#ifdef NEW_PIX
    if (gc_clip_mask)
        gc_clip_mask->dec_ref();
    if (gc_old_clip_mask)
        gc_old_clip_mask->dec_ref();
    if (gc_tile)
        gc_tile->dec_ref();
    if (gc_stipple)
        gc_stipple->dec_ref();
#else
    if (gc_clip_mask)
        gdk_pixmap_unref(gc_clip_mask);
    if (gc_old_clip_mask)
        gdk_pixmap_unref(gc_old_clip_mask);
    if (gc_tile)
        gdk_pixmap_unref(gc_tile);
    if (gc_stipple)
        gdk_pixmap_unref(gc_stipple);
#endif
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
#ifdef NEW_PIX
            if (gc_clip_mask) {
                gc_clip_mask->dec_ref();
                gc_clip_mask = 0;
            }
            if (values->v_clip_mask) {
                gc_clip_mask = values->v_clip_mask;
                gc_clip_mask->inc_ref();
            }
#else
            if (gc_clip_mask) {
                gdk_pixmap_unref(gc_clip_mask);
                gc_clip_mask = 0;
            }
            if (values->v_clip_mask) {
                gc_clip_mask = values->v_clip_mask;
                gdk_pixmap_ref(gc_clip_mask);
            }
#endif
        }
      
        if (gc_clip_region) {
            gdk_region_destroy(gc_clip_region);
            gc_clip_region = 0;
        }
    }
    if (values_mask & ndkGC_FILL)
        gc_fill = values->v_fill;
    if (values_mask & ndkGC_STIPPLE) {
        if (gc_stipple != values->v_stipple) {
#ifdef NEW_PIX
            if (gc_stipple) {
                gc_stipple->dec_ref();
                gc_stipple = 0;
            }
            if (values->v_stipple) {
                gc_stipple = values->v_stipple;
                gc_stipple->inc_ref();
            }
#else
            if (gc_stipple) {
                gdk_pixmap_unref(gc_stipple);
                gc_stipple = 0;
            }
            if (values->v_stipple) {
                gc_stipple = values->v_stipple;
                gdk_pixmap_ref(gc_stipple);
            }
#endif
        }
    }
    if (values_mask & ndkGC_TILE) {
        if (gc_tile != values->v_tile) {
#ifdef NEW_PIX
            if (gc_tile) {
                gc_tile->dec_ref();
                gc_tile = 0;
            }
            if (values->v_tile) {
                gc_tile = values->v_tile;
                gc_tile->inc_ref();
            }
#else
            if (gc_tile) {
                gdk_pixmap_unref(gc_tile);
                gc_tile = 0;
            }
            if (values->v_tile) {
                gc_tile = values->v_tile;
                gdk_pixmap_ref(gc_tile);
            }
#endif
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
      
        set_values(&values, (ndkGCvaluesMask)(ndkGC_CLIP_X_ORIGIN |
             ndkGC_CLIP_Y_ORIGIN | ndkGC_TS_X_ORIGIN | ndkGC_TS_Y_ORIGIN));
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

    if (src_gc->gc_clip_mask != dst_gc->gc_clip_mask) {
#ifdef NEW_PIX
        if (dst_gc->gc_clip_mask) {
            dst_gc->gc_clip_mask->dec_ref();
            dst_gc->gc_clip_mask = 0;
        }
        if (src_gc->gc_clip_mask) {
            dst_gc->gc_clip_mask = src_gc->gc_clip_mask;
            dst_gc->gc_clip_mask->inc_ref();
        }
#else
        if (dst_gc->gc_clip_mask) {
            gdk_pixmap_unref(dst_gc->gc_clip_mask);
            dst_gc->gc_clip_mask = 0;
        }
        if (src_gc->gc_clip_mask) {
            dst_gc->gc_clip_mask = src_gc->gc_clip_mask;
            gdk_pixmap_ref(dst_gc->gc_clip_mask);
        }
#endif
    }
  
    if (src_gc->gc_old_clip_mask != dst_gc->gc_old_clip_mask) {
#ifdef NEW_PIX
        if (dst_gc->gc_old_clip_mask) {
            dst_gc->gc_old_clip_mask->dec_ref();
            dst_gc->gc_old_clip_mask = 0;
        }
        if (src_gc->gc_old_clip_mask) {
            dst_gc->gc_old_clip_mask = src_gc->gc_old_clip_mask;
            dst_gc->gc_old_clip_mask->inc_ref();
        }
#else
        if (dst_gc->gc_old_clip_mask) {
            gdk_pixmap_unref(dst_gc->gc_old_clip_mask);
            dst_gc->gc_old_clip_mask = 0;
        }
        if (src_gc->gc_old_clip_mask) {
            dst_gc->gc_old_clip_mask = src_gc->gc_old_clip_mask;
            gdk_pixmap_ref(dst_gc->gc_old_clip_mask);
        }
#endif
    }

    dst_gc->gc_fill = src_gc->gc_fill;
  
    if (src_gc->gc_stipple != dst_gc->gc_stipple) {
#ifdef NEW_PIX
        if (dst_gc->gc_stipple) {
            dst_gc->gc_stipple->dec_ref();
            dst_gc->gc_stipple = 0;
        }
        if (src_gc->gc_stipple) {
            dst_gc->gc_stipple = src_gc->gc_stipple;
            dst_gc->gc_stipple->inc_ref();
        }
#else
        if (dst_gc->gc_stipple) {
            gdk_pixmap_unref(dst_gc->gc_stipple);
            dst_gc->gc_stipple = 0;
        }
        if (src_gc->gc_stipple) {
            dst_gc->gc_stipple = src_gc->gc_stipple;
            gdk_pixmap_ref(dst_gc->gc_stipple);
        }
#endif
    }
  
    if (src_gc->gc_tile != dst_gc->gc_tile) {
#ifdef NEW_PIX
        if (dst_gc->gc_tile) {
            dst_gc->gc_tile->dec_ref();
            dst_gc->gc_tile = 0;
        }
        if (src_gc->gc_tile) {
            dst_gc->gc_tile = src_gc->gc_tile;
            dst_gc->gc_tile->inc_ref();
        }
#else
        if (dst_gc->gc_tile) {
            gdk_pixmap_unref(dst_gc->gc_tile);
            dst_gc->gc_tile = 0;
        }
        if (src_gc->gc_tile) {
            dst_gc->gc_tile = src_gc->gc_tile;
            gdk_pixmap_ref(dst_gc->gc_tile);
        }
#endif
    }

    dst_gc->gc_fg_pixel = src_gc->gc_fg_pixel;
    dst_gc->gc_bg_pixel = src_gc->gc_bg_pixel;
    dst_gc->gc_subwindow_mode = src_gc->gc_subwindow_mode;
    dst_gc->gc_exposures = src_gc->gc_exposures;
}


// END of public methods.


// Takes ownership of passed-in region.
//
void
ndkGC::gc_set_clip_region_real(GdkRegion *region, bool reset_origin)
{
    if (gc_clip_mask) {
#ifdef NEW_PIX
        gc_clip_mask->dec_ref();
#else
        gdk_pixmap_unref(gc_clip_mask);
#endif
        gc_clip_mask = 0;
    }
  
    if (gc_clip_region)
        gdk_region_destroy(gc_clip_region);
    gc_clip_region = region;

    gc_windowing_set_clip_region(region, reset_origin);
}


// Doesn't copy region, allows not to reset origin.
//
void
ndkGC::gc_set_clip_region_internal(GdkRegion *region, bool reset_origin)
{
    gc_remove_drawable_clip();
    gc_set_clip_region_real(region, reset_origin);
}


/*****
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
        GcPixmap *new_mask;
        Gc *tmp_gc;
        GdkColor black = {0, 0, 0, 0};
        GdkRectangle r;
        GdkOverlapType overlap;

        int w = cairo_xlib_surface_get_width(gc_clip_mask);
        int h = cairo_xlib_surface_get_height(gc_clip_mask);
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
            gc_old_clip_mask = cairo_surface_reference(gc_clip_mask);
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
#ifdef NEW_PIX
            gc_old_clip_mask->dec_ref();
#else
            gdk_pixmap_unref(gc_old_clip_mask);
#endif
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
#ifdef NEW_PIX
    cairo_surface_t *make_stipple_tile_surface(cairo_t *cr,
        ndkPixmap *stipple, GdkColor *foreground, GdkColor *background)
#else
    cairo_surface_t *make_stipple_tile_surface(cairo_t *cr,
        GdkBitmap *stipple, GdkColor *foreground, GdkColor *background)
#endif
    {
#ifdef NEW_PIX
        int width = stipple->get_width();
        int height = stipple->get_height();
#else
        int width, height;
        gdk_drawable_get_size(stipple, &width, &height);
#endif

#ifdef NEW_PIX
        cairo_surface_t *sfc = cairo_get_target(cr);
        cairo_surface_t *alpha_surface = cairo_xlib_surface_create(
            cairo_xlib_surface_get_display(sfc), stipple->get_xid(),
            cairo_xlib_surface_get_visual(sfc), width, height);
        cairo_surface_destroy(sfc);
#else
        cairo_surface_t *alpha_surface =
            GDK_DRAWABLE_GET_CLASS(stipple)->ref_cairo_surface(stipple);
#endif
      
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
#ifdef NEW_PIX
ndkGC::gc_update_context(cairo_t *cr, const GdkColor *override_foreground,
    ndkPixmap  *override_stipple, bool gc_changed,
    GdkDrawable *target_drawable)
#else
ndkGC::gc_update_context(cairo_t *cr, const GdkColor *override_foreground,
    GdkBitmap  *override_stipple, bool gc_changed,
    GdkDrawable *target_drawable)
#endif
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

#ifdef NEW_PIX
    ndkPixmap *stipple = 0;
#else
    GdkBitmap *stipple = 0;
#endif
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
#ifdef NEW_PIX
//XXXXX FIXME        tile_surface = gc_tile;
#else
        tile_surface = 
            GDK_DRAWABLE_GET_CLASS(gc_tile)->ref_cairo_surface(gc_tile);
#endif
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


#ifdef WITH_X11

extern void _gdk_region_get_xrectangles (const GdkRegion*, gint, gint,
    XRectangle**, gint*);


void
ndkGC::gc_x11_flush()
{
    Display *xdisplay = get_xdisplay();
    /*XXX implement _gdk_region...
    if (gc_dirty_mask & ndkGC_DIRTY_CLIP) {
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
    */

    if (gc_dirty_mask & ndkGC_DIRTY_TS) {
        XSetTSOrigin(xdisplay, gc_gc, gc_ts_x_origin, gc_ts_y_origin);
    }
    gc_dirty_mask = 0;
}


void
ndkGC::gc_x11_set_values(ndkGCvalues *values, ndkGCvaluesMask values_mask)
{
    if (values_mask & (ndkGC_CLIP_X_ORIGIN | ndkGC_CLIP_Y_ORIGIN)) {
        int foo = values_mask;
        foo &= ~(ndkGC_CLIP_X_ORIGIN | ndkGC_CLIP_Y_ORIGIN);
        values_mask = (ndkGCvaluesMask)foo;
        gc_dirty_mask |= ndkGC_DIRTY_CLIP;
    }

    if (values_mask & (ndkGC_TS_X_ORIGIN | ndkGC_TS_Y_ORIGIN)) {
        int foo = values_mask;
        foo &= ~(ndkGC_TS_X_ORIGIN | ndkGC_TS_Y_ORIGIN);
        values_mask = (ndkGCvaluesMask)foo;
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

/* XXX Problem here, need surface of drawable
GdkWindow *wd = gdk_window_lookup_for_display(xvalues.tile);
        values->v_tile = xvalues.tile;
        values->v_stipple = xvalues.stipple;
*/
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
            break;
        case ndkGC_INVERT:
            xvalues->function = GXinvert;
            break;
        case ndkGC_XOR:
            xvalues->function = GXxor;
            break;
        case ndkGC_CLEAR:
            xvalues->function = GXclear;
            break;
        case ndkGC_AND:
            xvalues->function = GXand;
            break;
        case ndkGC_AND_REVERSE:
            xvalues->function = GXandReverse;
            break;
        case ndkGC_AND_INVERT:
            xvalues->function = GXandInverted;
            break;
        case ndkGC_NOOP:
            xvalues->function = GXnoop;
            break;
        case ndkGC_OR:
            xvalues->function = GXor;
            break;
        case ndkGC_EQUIV:
            xvalues->function = GXequiv;
            break;
        case ndkGC_OR_REVERSE:
            xvalues->function = GXorReverse;
            break;
        case ndkGC_COPY_INVERT:
            xvalues->function = GXcopyInverted;
            break;
        case ndkGC_OR_INVERT:
            xvalues->function = GXorInverted;
            break;
        case ndkGC_NAND:
            xvalues->function = GXnand;
            break;
        case ndkGC_SET:
            xvalues->function = GXset;
            break;
        case ndkGC_NOR:
            xvalues->function = GXnor;
        break;
        }
        *xvalues_mask |= GCFunction;
    }
    if (mask & ndkGC_FILL) {
        switch (values->v_fill) {
        case ndkGC_SOLID:
            xvalues->fill_style = FillSolid;
            break;
        case ndkGC_TILED:
            xvalues->fill_style = FillTiled;
            break;
        case ndkGC_STIPPLED:
            xvalues->fill_style = FillStippled;
            break;
        case ndkGC_OPAQUE_STIPPLED:
            xvalues->fill_style = FillOpaqueStippled;
            break;
        }
        *xvalues_mask |= GCFillStyle;
    }
    if (mask & ndkGC_TILE) {
        if (values->v_tile)
#ifdef NEW_PIX
            xvalues->tile = values->v_tile->get_xid();
#else
            xvalues->tile = GDK_DRAWABLE_XID(values->v_tile);
#endif
        else
            xvalues->tile = None;
        *xvalues_mask |= GCTile;
    }
    if (mask & ndkGC_STIPPLE) {
        if (values->v_stipple) {
#ifdef NEW_PIX
            xvalues->stipple = values->v_stipple->get_xid();
#else
            xvalues->stipple = GDK_DRAWABLE_XID(values->v_stipple);
#endif
        }
        else
            xvalues->stipple = None;
        *xvalues_mask |= GCStipple;
    }
    if (mask & ndkGC_CLIP_MASK) {
        if (values->v_clip_mask) {
#ifdef NEW_PIX
            xvalues->clip_mask = values->v_clip_mask->get_xid();
#else
            xvalues->clip_mask = GDK_DRAWABLE_XID(values->v_clip_mask);
#endif
        }
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
            break;
        case ndkGC_LINE_ON_OFF_DASH:
            xvalues->line_style = LineOnOffDash;
            break;
        case ndkGC_LINE_DOUBLE_DASH:
            xvalues->line_style = LineDoubleDash;
            break;
        }
        *xvalues_mask |= GCLineStyle;
    }
    if (mask & ndkGC_CAP_STYLE) {
        switch (values->v_cap_style) {
        case ndkGC_CAP_NOT_LAST:
            xvalues->cap_style = CapNotLast;
            break;
        case ndkGC_CAP_BUTT:
            xvalues->cap_style = CapButt;
            break;
        case ndkGC_CAP_ROUND:
            xvalues->cap_style = CapRound;
            break;
        case ndkGC_CAP_PROJECTING:
            xvalues->cap_style = CapProjecting;
            break;
        }
        *xvalues_mask |= GCCapStyle;
    }
    if (mask & ndkGC_JOIN_STYLE) {
        switch (values->v_join_style) {
        case ndkGC_JOIN_MITER:
            xvalues->join_style = JoinMiter;
            break;
        case ndkGC_JOIN_ROUND:
            xvalues->join_style = JoinRound;
            break;
        case ndkGC_JOIN_BEVEL:
            xvalues->join_style = JoinBevel;
            break;
        }
        *xvalues_mask |= GCJoinStyle;
    }
}


void
ndkGC::gc_windowing_set_clip_region(const GdkRegion *region, bool reset_origin)
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
    XCopyGC(src_gc->get_xdisplay(), src_gc->get_xgc(), ~((~1) << GCLastBit),
        dst_gc->get_xgc());

    dst_gc->gc_dirty_mask = src_gc->gc_dirty_mask;
    dst_gc->gc_have_clip_region = src_gc->gc_have_clip_region;
    dst_gc->gc_have_clip_mask = src_gc->gc_have_clip_mask;
}

#endif  // WITH_X11

#endif  // NDKGC_H

