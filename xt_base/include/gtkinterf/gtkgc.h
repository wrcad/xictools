
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

// gtkgc.h:  This derives from the gdkgc.h from GTK-2.0.  It defines a
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

#ifndef _GTKGC_H_
#define _GTKGC_H_


// GC cap styles.
enum GcCapStyle
{
    GC_CAP_NOT_LAST,
    GC_CAP_BUTT,
    GC_CAP_ROUND,
    GC_CAP_PROJECTING
};

// GC fill types.
enum GcFill
{
    GC_SOLID,
    GC_TILED,
    GC_STIPPLED,
    GC_OPAQUE_STIPPLED
};

// GC function types.
//   Copy:          Overwrites destination pixels with the source pixels.
//   Invert:        Inverts the destination pixels.
//   Xor:           XORs the destination pixels with the source pixels.
//   Clear:         Set pixels to 0.
//   And:           Source AND destination.
//   And Reverse:   Source AND (NOT destination).
//   And Invert:    (NOT source) AND destination.
//   Noop:          Destination.
//   Or:            Source OR destination.
//   Nor:           (NOT source) AND (NOT destination).
//   Equiv:         (NOT source) XOR destination.
//   Xor Reverse:   Source OR (NOT destination).
//   Copy Inverted: NOT source.
//   Xor Inverted:  (NOT source) OR destination.
//   Nand:          (NOT source) OR (NOT destination).
//   Set:           Set pixels to 1.
//
enum GcFunction
{
    GC_COPY,
    GC_INVERT,
    GC_XOR,
    GC_CLEAR,
    GC_AND,
    GC_AND_REVERSE,
    GC_AND_INVERT,
    GC_NOOP,
    GC_OR,
    GC_EQUIV,
    GC_OR_REVERSE,
    GC_COPY_INVERT,
    GC_OR_INVERT,
    GC_NAND,
    GC_NOR,
    GC_SET
};

// GC join styles.
enum GcJoinStyle
{
    GC_JOIN_MITER,
    GC_JOIN_ROUND,
    GC_JOIN_BEVEL
};

// GC line styles.
enum GcLineStyle
{
    GC_LINE_SOLID,
    GC_LINE_ON_OFF_DASH,
    GC_LINE_DOUBLE_DASH
};

enum GcSubwindowMode
{
    GC_CLIP_BY_CHILDREN  = 0,
    GC_INCLUDE_INFERIORS = 1
};

//XXX
typedef void GcPixmap;
typedef void GcBitmap;

enum GcValuesMask
{
    GC_FOREGROUND    = 1 << 0,
    GC_BACKGROUND    = 1 << 1,
    GC_FONT          = 1 << 2,
    GC_FUNCTION      = 1 << 3,
    GC_FILL          = 1 << 4,
    GC_TILE          = 1 << 5,
    GC_STIPPLE       = 1 << 6,
    GC_CLIP_MASK     = 1 << 7,
    GC_SUBWINDOW     = 1 << 8,
    GC_TS_X_ORIGIN   = 1 << 9,
    GC_TS_Y_ORIGIN   = 1 << 10,
    GC_CLIP_X_ORIGIN = 1 << 11,
    GC_CLIP_Y_ORIGIN = 1 << 12,
    GC_EXPOSURES     = 1 << 13,
    GC_LINE_WIDTH    = 1 << 14,
    GC_LINE_STYLE    = 1 << 15,
    GC_CAP_STYLE     = 1 << 16,
    GC_JOIN_STYLE    = 1 << 17
};

#ifdef WITH_X11
enum GcDirtyValues
{
    GC_DIRTY_CLIP = 1 << 0,
    GC_DIRTY_TS = 1 << 1
};
#endif

struct GcValues
{
    GdkColor        v_foreground;
    GdkColor        v_background;
    GcFunction      v_function;
    GcFill          v_fill;
    GcPixmap        *v_tile;
    GcPixmap        *v_stipple;
    GcPixmap        *v_clip_mask;
    GcSubwindowMode v_subwindow_mode;
    int             v_ts_x_origin;
    int             v_ts_y_origin;
    int             v_clip_x_origin;
    int             v_clip_y_origin;
    int             v_graphics_exposures;
    int             v_line_width;
    GcLineStyle     v_line_style;
    GcCapStyle      v_cap_style;
    GcJoinStyle     v_join_style;
};

struct Gc
{
    Gc(GdkDrawable*, GcValues*, GcValuesMask);
    ~Gc();

    void set_values(GcValues*, GcValuesMask);
    void get_values(GcValues*);

    // Sets the colormap for the GC to the given colormap.  The depth of
    // the colormap's visual must match the depth of the drawable for
    // which the GC was created.
    //
    void set_colormap(GdkColormap *cmap)
    {
        if (gc_colormap != cmap) {
            if (gc_colormap)
                g_object_unref(gc_colormap);
            gc_colormap = cmap;
            g_object_ref(gc_colormap);
        }
    }

    GdkColormap *get_colormap()     { return (gc_colormap); }


    void set_foreground(const GdkColor *color)
    {
        GcValues values;
        values.v_foreground = *color;
        set_values(&values, GC_FOREGROUND);
    }

    void get_foreground(GdkColor *color)
    {
        if (!gc_colormap)
            g_warning("No colormap in Gc::get_foreground");
        else {
            color->pixel = gc_bg_pixel;
            gdk_colormap_query_color(gc_colormap, gc_fg_pixel, color);
        }
    }

/* XXX
    // Set the foreground color of a GC using an unallocated color.  The
    // pixel value for the color will be determined using GdkRGB.  If the
    // colormap for the GC has not previously been initialized for GdkRGB,
    // then for pseudo-color colormaps (colormaps with a small modifiable
    // number of colors), a colorcube will be allocated in the colormap.
    // 
    // Calling this function for a GC without a colormap is an error.
    //
    void set_rgb_fg_color(const GdkColor *color)
    {
        GdkColormap *cmap = get_colormap_warn();
        if (!cmap)
            return;
        GdkColor tmp_color = *color;
        gdk_rgb_find_color(cmap, &tmp_color);
        set_foreground(&tmp_color);
    }
*/

    unsigned int get_fg_pixel()     { return (gc_fg_pixel); }


    void set_background(const GdkColor *color)
    {
        GcValues values;
        values.v_background = *color;
        set_values(&values, GC_BACKGROUND);
    }

    void get_background(GdkColor *color)
    {
        if (!gc_colormap)
            g_warning("No colormap in Gc::get_background");
        else {
            color->pixel = gc_bg_pixel;
            gdk_colormap_query_color(gc_colormap, gc_bg_pixel, color);
        }
    }

/*
    // Set the background color of a GC using an unallocated color.  The
    // pixel value for the color will be determined using GdkRGB.  If the
    // colormap for the GC has not previously been initialized for GdkRGB,
    // then for pseudo-color colormaps (colormaps with a small modifiable
    // number of colors), a colorcube will be allocated in the colormap.
    // 
    // Calling this function for a GC without a colormap is an error.
    //
    void set_rgb_bg_color(const GdkColor *color)
    {
        GdkColormap *cmap = get_colormap_warn();
        if (!cmap)
            return;
        GdkColor tmp_color = *color;
        gdk_rgb_find_color(cmap, &tmp_color);
        set_background(&tmp_color);
    }
*/

    unsigned int get_bg_pixel()     { return (gc_bg_pixel); }


    void set_function(GcFunction function)
    {
        GcValues values;
        values.v_function = function;
        set_values(&values, GC_FUNCTION);
    }


    void set_fill(GcFill fill)
    {
        GcValues values;
        values.v_fill = fill;
        set_values(&values, GC_FILL);
    }

    GcFill get_fill()               { return ((GcFill)gc_fill); }


    void set_tile(GcPixmap *tile)
    {
        GcValues values;
        values.v_tile = tile;
        set_values(&values, GC_TILE);
    }

    GcPixmap *get_tile()            { return (gc_tile); }


    void set_stipple(GcPixmap *stipple)
    {
        GcValues values;
        values.v_stipple = stipple;
        set_values(&values, GC_STIPPLE);
    }

    GcBitmap *get_stipple()         { return (gc_stipple); }


    // Set the origin when using tiles or stipples with the GC.  The tile
    // or stipple will be aligned such that the upper left corner of the
    // tile or stipple will coincide with this point.
    //
    void set_ts_origin(int x, int y)
    {
        GcValues values;
        values.v_ts_x_origin = x;
        values.v_ts_y_origin = y;
        set_values(&values, (GcValuesMask)(GC_TS_X_ORIGIN | GC_TS_Y_ORIGIN));
    }


    // Sets the origin of the clip mask.  The coordinates are interpreted
    // relative to the upper-left corner of the destination drawable of
    // the current operation.
    //
    void set_clip_origin(int x, int y)
    {
        GcValues values;
        values.v_clip_x_origin = x;
        values.v_clip_y_origin = y;
        set_values(&values, (GcValuesMask)(GC_CLIP_X_ORIGIN |
            GC_CLIP_Y_ORIGIN));
    }

    // Sets the clip mask for a graphics context from a bitmap.  The clip
    // mask is interpreted relative to the clip origin.  (See
    // set_clip_origin()).
    //
    void set_clip_mask(GcBitmap *mask)
    {
        GcValues values;
        values.v_clip_mask = mask;
        set_values(&values, GC_CLIP_MASK);
    }

    GcBitmap *get_clip_mask()       { return (gc_clip_mask); }

    // Sets the clip mask for a graphics context from a rectangle.  The
    // clip mask is interpreted relative to the clip origin.  (See
    // set_clip_origin()).
    //
    void set_clip_rectangle(const GdkRectangle *rectangle)
    {
        gc_remove_drawable_clip();
        GdkRegion *region;
        if (rectangle)
            region = gdk_region_rectangle(rectangle);
        else
            region = 0;
        gc_set_clip_region_real(region, true);
    }

    // Sets the clip mask for a graphics context from a region structure. 
    // The clip mask is interpreted relative to the clip origin.  (See
    // set_clip_origin()).
    //
    void set_clip_region(const GdkRegion *region)
    {
        gc_remove_drawable_clip();
        GdkRegion *copy;
        if (region)
            copy = gdk_region_copy(region);
        else
            copy = 0;
        gc_set_clip_region_real(copy, true);
    }

    GdkRegion *get_clip_region()    { return (gc_clip_region); }


    // Sets how drawing with this GC on a window will affect child windows
    // of that window. 
    //
    void set_subwindow(GcSubwindowMode mode)
    {
        // This could get called a lot to reset the subwindow mode in the
        // client side clipping, so bail out early.
        if (gc_subwindow_mode == mode)
            return;
      
        GcValues values;
        values.v_subwindow_mode = mode;
        set_values(&values, GC_SUBWINDOW);
    }

    GcSubwindowMode get_subwindow()  {
        return ((GcSubwindowMode)gc_subwindow_mode); }


    // Sets whether copying non-visible portions of a drawable using this
    // graphics context generate exposure events for the corresponding
    // regions of the destination drawable.  (See gdk_draw_drawable()).
    //
    void set_exposures(bool exposures)
    {
        GcValues values;
        values.v_graphics_exposures = exposures;
        set_values(&values, GC_EXPOSURES);
    }

    bool get_exposures()            { return gc_exposures; }


    // line_width: the width of lines.
    // line_style: the dash-style for lines.
    // cap_style:  the manner in which the ends of lines are drawn.
    // join_style: the in which lines are joined together.
    // 
    // Sets various attributes of how lines are drawn.  See the
    // corresponding members of GcValues for full explanations of the
    // arguments.
    //
    void set_line_attributes(int line_width, GcLineStyle line_style,
        GcCapStyle cap_style, GcJoinStyle join_style)
    {
        GcValues values;
        values.v_line_width = line_width;
        values.v_line_style = line_style;
        values.v_cap_style = cap_style;
        values.v_join_style = join_style;

        set_values(&values, (GcValuesMask)(GC_LINE_WIDTH | GC_LINE_STYLE |
            GC_CAP_STYLE | GC_JOIN_STYLE));
    }

    // dash_offset: the phase of the dash pattern.
    // dash_list:   an array of dash lengths.
    // n:           the number of elements in @dash_list.
    // 
    // Sets the way dashed-lines are drawn.  Lines will be drawn with
    // alternating on and off segments of the lengths specified in
    // dash_list.  The manner in which the on and off segments are drawn
    // is determined by the @line_style value of the GC.  (This can be
    // changed with gdk_gc_set_line_attributes().)
    //
    // The dash_offset defines the phase of the pattern, specifying how
    // many pixels into the dash-list the pattern should actually begin.
    //
    void set_dashes(int dash_offset, unsigned char dash_list[], int n) {
#ifdef WITH_X11
        XSetDashes(get_xdisplay(), gc_gc, dash_offset, (char *)dash_list, n);
#endif
    }


#ifdef WITH_X11
    GC get_xgc() {
        if (gc_dirty_mask)
            gc_x11_flush();
        return (gc_gc);
    }

    Display *get_xdisplay()         { return GDK_SCREEN_XDISPLAY(gc_screen); }

    GdkScreen *get_screen()         { return (gc_screen);}
#endif

    void offset(int, int);
    static void copy(Gc*, Gc*);

private:
    void gc_set_clip_region_real(GdkRegion*, bool);
    void gc_set_clip_region_internal(GdkRegion*, bool);
    void gc_add_drawable_clip(unsigned int, GdkRegion*, int, int);
    void gc_remove_drawable_clip();
    GdkColormap *get_colormap_warn();
    void gc_update_context(cairo_t*, const GdkColor*, GcBitmap*, bool,
        GdkDrawable*);
#ifdef WITH_X11
    void gc_x11_flush();
    void gc_x11_set_values(GcValues*, GcValuesMask);
    void gc_x11_get_values(GcValues*);
    static void gc_values_to_xvalues(GcValues*, GcValuesMask, XGCValues*,
        unsigned long*);
    void gc_windowing_set_clip_region(const GdkRegion*, bool);
    static void gc_windowing_copy(Gc*, Gc*);
#endif

    int             gc_clip_x_origin;
    int             gc_clip_y_origin;
    int             gc_ts_x_origin;
    int             gc_ts_y_origin;

    GdkRegion       *gc_clip_region;
    GdkRegion       *gc_old_clip_region;
    GcPixmap        *gc_old_clip_mask;

    unsigned int    gc_region_tag_applied;
    int             gc_region_tag_offset_x;
    int             gc_region_tag_offset_y;

    GcBitmap        *gc_stipple;
    GcPixmap        *gc_tile;
    GcPixmap        *gc_clip_mask;

    GdkColormap     *gc_colormap;
    unsigned int    gc_fg_pixel;
    unsigned int    gc_bg_pixel;

#ifdef WITH_X11
    GC              gc_gc;
    GdkScreen       *gc_screen;
    unsigned short  gc_dirty_mask;
    bool            gc_have_clip_region;
    bool            gc_have_clip_mask;
    unsigned char   gc_depth;
#endif
    bool            gc_subwindow_mode;
    unsigned char   gc_fill;
    bool            gc_exposures;
};

#endif

