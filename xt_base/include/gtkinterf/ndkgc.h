
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

// ndkgc.h:  This derives from the gdkgc.h from GTK-2.0.  It defines a
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

// Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
// file for a list of people on the GTK+ Team.  See the ChangeLog
// files for a list of changes.  These files are distributed with
// GTK+ at ftp://ftp.gtk.org/pub/gtk/.

#ifndef NDKGC_H
#define NDKGC_H

struct ndkPixmap;
struct ndkDrawable;

// ndkGC cap styles.
enum ndkGCcapStyle
{
    ndkGC_CAP_NOT_LAST,
    ndkGC_CAP_BUTT,
    ndkGC_CAP_ROUND,
    ndkGC_CAP_PROJECTING
};

// ndkGC fill types.
enum ndkGCfill
{
    ndkGC_SOLID,
    ndkGC_TILED,
    ndkGC_STIPPLED,
    ndkGC_OPAQUE_STIPPLED
};

// ndkGC fill rule
enum ndkGCfillRule
{
    ndkGC_EVEN_ODD_RULE,
    ndkGC_WINDING_RULE
};

// ndkGC function types.
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
enum ndkGCfunction
{
    ndkGC_COPY,
    ndkGC_INVERT,
    ndkGC_XOR,
    ndkGC_CLEAR,
    ndkGC_AND,
    ndkGC_AND_REVERSE,
    ndkGC_AND_INVERT,
    ndkGC_NOOP,
    ndkGC_OR,
    ndkGC_EQUIV,
    ndkGC_OR_REVERSE,
    ndkGC_COPY_INVERT,
    ndkGC_OR_INVERT,
    ndkGC_NAND,
    ndkGC_NOR,
    ndkGC_SET
};

// ndkGC join styles.
enum ndkGCjoinStyle
{
    ndkGC_JOIN_MITER,
    ndkGC_JOIN_ROUND,
    ndkGC_JOIN_BEVEL
};

// ndkGC line styles.
enum ndkGClineStyle
{
    ndkGC_LINE_SOLID,
    ndkGC_LINE_ON_OFF_DASH,
    ndkGC_LINE_DOUBLE_DASH
};

enum ndkGCsubwinMode
{
    ndkGC_CLIP_BY_CHILDREN  = 0,
    ndkGC_INCLUDE_INFERIORS = 1
};

typedef unsigned int ndkGCvaluesMask;
enum
{
    ndkGC_FOREGROUND    = 1 << 0,
    ndkGC_BACKGROUND    = 1 << 1,
    ndkGC_FONT          = 1 << 2,
    ndkGC_FUNCTION      = 1 << 3,
    ndkGC_FILL          = 1 << 4,
    ndkGC_FILL_RULE     = 1 << 5,
    ndkGC_TILE          = 1 << 6,
    ndkGC_STIPPLE       = 1 << 7,
    ndkGC_CLIP_MASK     = 1 << 8,
    ndkGC_SUBWINDOW     = 1 << 9,
    ndkGC_TS_X_ORIGIN   = 1 << 10,
    ndkGC_TS_Y_ORIGIN   = 1 << 11,
    ndkGC_CLIP_X_ORIGIN = 1 << 12,
    ndkGC_CLIP_Y_ORIGIN = 1 << 13,
    ndkGC_EXPOSURES     = 1 << 14,
    ndkGC_LINE_WIDTH    = 1 << 15,
    ndkGC_LINE_STYLE    = 1 << 16,
    ndkGC_CAP_STYLE     = 1 << 17,
    ndkGC_JOIN_STYLE    = 1 << 18
};

#ifdef WITH_X11
enum ndkGCdirtyValues
{
    ndkGC_DIRTY_CLIP = 1 << 0,
    ndkGC_DIRTY_TS = 1 << 1
};
#endif

struct ndkGCvalues
{
    GdkColor        v_foreground;
    GdkColor        v_background;
    ndkGCfunction   v_function;
    ndkGCfill       v_fill;
    ndkGCfillRule   v_fill_rule;
    ndkPixmap       *v_tile;
    ndkPixmap       *v_stipple;
    ndkPixmap       *v_clip_mask;
    ndkGCsubwinMode v_subwindow_mode;
    int             v_ts_x_origin;
    int             v_ts_y_origin;
    int             v_clip_x_origin;
    int             v_clip_y_origin;
    int             v_graphics_exposures;
    int             v_line_width;
    ndkGClineStyle  v_line_style;
    ndkGCcapStyle   v_cap_style;
    ndkGCjoinStyle  v_join_style;
};

struct ndkGC
{
    ndkGC(GdkWindow*, ndkGCvalues* =0, ndkGCvaluesMask =0);
    ~ndkGC();

    void set_values(ndkGCvalues*, ndkGCvaluesMask);
    void get_values(ndkGCvalues*);

    void set_foreground(const GdkColor *color)
    {
        ndkGCvalues values;
        values.v_foreground = *color;
        set_values(&values, ndkGC_FOREGROUND);
    }

    unsigned int get_fg_pixel()     { return (gc_fg_pixel); }


    void set_background(const GdkColor *color)
    {
        ndkGCvalues values;
        values.v_background = *color;
        set_values(&values, ndkGC_BACKGROUND);
    }

    unsigned int get_bg_pixel()     { return (gc_bg_pixel); }


    void set_function(ndkGCfunction function)
    {
        ndkGCvalues values;
        values.v_function = function;
        set_values(&values, ndkGC_FUNCTION);
        gc_function = function;
    }

    ndkGCfunction get_function()    { return (gc_function); }


    void set_fill(ndkGCfill fill)
    {
        ndkGCvalues values;
        values.v_fill = fill;
        set_values(&values, ndkGC_FILL);
    }

    ndkGCfill get_fill()            { return ((ndkGCfill)gc_fill); }

    void set_fill_rule(ndkGCfillRule rule)
    {
        ndkGCvalues values;
        values.v_fill_rule = rule;
        set_values(&values, ndkGC_FILL_RULE);
    }

    ndkGCfillRule get_fill_rule()   { return ((ndkGCfillRule)gc_fill_rule); }

    void set_tile(ndkPixmap *tile)
    {
        ndkGCvalues values;
        values.v_tile = tile;
        set_values(&values, ndkGC_TILE);
    }

    ndkPixmap *get_tile()           { return (gc_tile); }


    void set_stipple(ndkPixmap *stipple)
    {
        ndkGCvalues values;
        values.v_stipple = stipple;
        set_values(&values, ndkGC_STIPPLE);
    }

    ndkPixmap *get_stipple()        { return (gc_stipple); }


    // Set the origin when using tiles or stipples with the GC.  The tile
    // or stipple will be aligned such that the upper left corner of the
    // tile or stipple will coincide with this point.
    //
    void set_ts_origin(int x, int y)
    {
        ndkGCvalues values;
        values.v_ts_x_origin = x;
        values.v_ts_y_origin = y;
        set_values(&values, ndkGC_TS_X_ORIGIN | ndkGC_TS_Y_ORIGIN);
    }


    // Sets the origin of the clip mask.  The coordinates are interpreted
    // relative to the upper-left corner of the destination drawable of
    // the current operation.
    //
    void set_clip_origin(int x, int y)
    {
        ndkGCvalues values;
        values.v_clip_x_origin = x;
        values.v_clip_y_origin = y;
        set_values(&values, ndkGC_CLIP_X_ORIGIN | ndkGC_CLIP_Y_ORIGIN);
    }

    // Sets the clip mask for a graphics context from a bitmap.  The clip
    // mask is interpreted relative to the clip origin.  (See
    // set_clip_origin()).
    //
    void set_clip_mask(ndkPixmap *mask)
    {
        ndkGCvalues values;
        values.v_clip_mask = mask;
        set_values(&values, ndkGC_CLIP_MASK);
    }

    ndkPixmap *get_clip_mask()      { return (gc_clip_mask); }

    // Sets the clip mask for a graphics context from a rectangle.  The
    // clip mask is interpreted relative to the clip origin.  (See
    // set_clip_origin()).
    //
    void set_clip_rectangle(const GdkRectangle *rectangle)
    {
        gc_remove_drawable_clip();
#ifdef NOTGDK3
        GdkRegion *region = rectangle ? gdk_region_rectangle(rectangle) : 0;
#else
        cairo_region_t *region = rectangle ?
            cairo_region_create_rectangle(rectangle) : 0;
#endif
        gc_set_clip_region_real(region, true);
    }

    // Sets the clip mask for a graphics context from a region structure. 
    // The clip mask is interpreted relative to the clip origin.  (See
    // set_clip_origin()).
    //
#ifdef NOTGTK3
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
#else
    void set_clip_region(const cairo_region_t *region)
    {
        gc_remove_drawable_clip();
        cairo_region_t *copy = region ? cairo_region_copy(region) : 0;;
        gc_set_clip_region_real(copy, true);
    }

    cairo_region_t *get_clip_region()    { return (gc_clip_region); }
#endif


    // Sets how drawing with this GC on a window will affect child windows
    // of that window. 
    //
    void set_subwindow(ndkGCsubwinMode mode)
    {
        // This could get called a lot to reset the subwindow mode in the
        // client side clipping, so bail out early.
        if (gc_subwindow_mode == mode)
            return;
      
        ndkGCvalues values;
        values.v_subwindow_mode = mode;
        set_values(&values, ndkGC_SUBWINDOW);
    }

    ndkGCsubwinMode get_subwindow()  {
        return ((ndkGCsubwinMode)gc_subwindow_mode); }


    // Sets whether copying non-visible portions of a drawable using this
    // graphics context generate exposure events for the corresponding
    // regions of the destination drawable.  (See gdk_draw_drawable()).
    //
    void set_exposures(bool exposures)
    {
        ndkGCvalues values;
        values.v_graphics_exposures = exposures;
        set_values(&values, ndkGC_EXPOSURES);
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
    void set_line_attributes(int line_width, ndkGClineStyle line_style,
        ndkGCcapStyle cap_style, ndkGCjoinStyle join_style)
    {
        ndkGCvalues values;
        values.v_line_width = line_width;
        values.v_line_style = line_style;
        values.v_cap_style = cap_style;
        values.v_join_style = join_style;

        set_values(&values, ndkGC_LINE_WIDTH | ndkGC_LINE_STYLE |
            ndkGC_CAP_STYLE | ndkGC_JOIN_STYLE);
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
    static void copy(ndkGC*, ndkGC*);
    void draw_line(ndkDrawable *d, int x1, int y1, int x2, int y2)
        { draw_line(d->get_xid(), x1, y1, x2, y2); }
    void draw_arc(ndkDrawable *d, bool filled, int x, int y, int w, int h,
            int as, int ae)
        { draw_arc(d->get_xid(), filled, x, y, w, h, as, ae); }
    void draw_rectangle(ndkDrawable *d, bool filled, int x, int y, int w, int h)
        { draw_rectangle(d->get_xid(), filled, x, y, w, h); }
    void draw_polygon(ndkDrawable *d, bool filled, GdkPoint *pts, int npts)
        { draw_polygon(d->get_xid(), filled, pts, npts); }
    void draw_pango_layout(ndkDrawable *d, int x, int y, PangoLayout *lout)
        { draw_pango_layout(d->get_xid(), x, y, lout); }

    void draw_line(ndkPixmap *p, int x1, int y1, int x2, int y2)
        { draw_line(p->get_xid(), x1, y1, x2, y2); }
    void draw_arc(ndkPixmap *p, bool filled, int x, int y, int w, int h,
            int as, int ae)
        { draw_arc(p->get_xid(), filled, x, y, w, h, as, ae); }
    void draw_rectangle(ndkPixmap *p, bool filled, int x, int y, int w, int h)
        { draw_rectangle(p->get_xid(), filled, x, y, w, h); }
    void draw_polygon(ndkPixmap *p, bool filled, GdkPoint *pts, int npts)
        { draw_polygon(p->get_xid(), filled, pts, npts); }
    void draw_pango_layout(ndkPixmap *p, int x, int y, PangoLayout *lout)
        { draw_pango_layout(p->get_xid(), x, y, lout); }

#ifdef NOTGTK3
    void draw_line(GdkWindow *d, int x1, int y1, int x2, int y2)
        { draw_line(gdk_x11_drawable_get_xid(d), x1, y1, x2, y2); }
    void draw_arc(GdkWindow *d, bool filled, int x, int y, int w, int h,
            int as, int ae)
        { draw_arc(gdk_x11_drawable_get_xid(d), filled, x, y, w, h, as, ae); }
    void draw_rectangle(GdkWindow *d, bool filled, int x, int y, int w, int h)
        { draw_rectangle(gdk_x11_drawable_get_xid(d), filled, x, y, w, h); }
    void draw_polygon(GdkWindow *d, bool filled, GdkPoint *pts, int npts)
        { draw_polygon(gdk_x11_drawable_get_xid(d), filled, pts, npts); }
    void draw_pango_layout(GdkWindow *d, int x, int y, PangoLayout *lout)
        { draw_pango_layout(gdk_x11_drawable_get_xid(d), x, y, lout); }
#else
    void draw_line(GdkWindow *d, int x1, int y1, int x2, int y2)
        { draw_line(gdk_x11_window_get_xid(d), x1, y1, x2, y2); }
    void draw_arc(GdkWindow *d, bool filled, int x, int y, int w, int h,
            int as, int ae)
        { draw_arc(gdk_x11_window_get_xid(d), filled, x, y, w, h, as, ae); }
    void draw_rectangle(GdkWindow *d, bool filled, int x, int y, int w, int h)
        { draw_rectangle(gdk_x11_window_get_xid(d), filled, x, y, w, h); }
    void draw_polygon(GdkWindow *d, bool filled, GdkPoint *pts, int npts)
        { draw_polygon(gdk_x11_window_get_xid(d), filled, pts, npts); }
    void draw_pango_layout(GdkWindow *d, int x, int y, PangoLayout *lout)
        { draw_pango_layout(gdk_x11_window_get_xid(d), x, y, lout); }
#endif

private:
#ifdef WITH_X11
    void draw_line(XID, int, int, int, int);
    void draw_arc(XID, bool, int, int, int, int, int, int);
    void draw_rectangle(XID, bool, int, int, int, int);
    void draw_polygon(XID, bool, GdkPoint*, int);
    void draw_pango_layout(XID, int, int, PangoLayout*);

    void gc_x11_set_values(ndkGCvalues*, ndkGCvaluesMask);
    void gc_x11_get_values(ndkGCvalues*);
    static void gc_values_to_xvalues(ndkGCvalues*, ndkGCvaluesMask, XGCValues*,
        unsigned long*);
#endif

#define GDK_IMPORTS
#ifdef GDK_IMPORTS
#ifdef NOTGDK3
    void gc_set_clip_region_real(GdkRegion*, bool);
    void gc_set_clip_region_internal(GdkRegion*, bool);
//    void gc_add_drawable_clip(unsigned int, GdkRegion*, int, int);
#else
    void gc_set_clip_region_real(cairo_region_t*, bool);
    void gc_set_clip_region_internal(cairo_region_t*, bool);
//    void gc_add_drawable_clip(unsigned int, cairo_region_t*, int, int);
#endif
    void gc_remove_drawable_clip();
    void gc_update_context(cairo_t*, const GdkColor*, ndkPixmap*, bool,
        ndkDrawable*);
#ifdef WITH_X11
    void gc_x11_flush();
#ifdef NOTGDK3
    void gc_windowing_set_clip_region(const GdkRegion*, bool);
#else
    void gc_windowing_set_clip_region(const cairo_region_t*, bool);
#endif
    static void gc_windowing_copy(ndkGC*, ndkGC*);
#endif
#endif

    int             gc_clip_x_origin;
    int             gc_clip_y_origin;
    int             gc_ts_x_origin;
    int             gc_ts_y_origin;

#ifdef NOTGDK3
    GdkRegion       *gc_clip_region;
    GdkRegion       *gc_old_clip_region;
#else
    cairo_region_t  *gc_clip_region;
    cairo_region_t  *gc_old_clip_region;
#endif

    unsigned int    gc_region_tag_applied;
    int             gc_region_tag_offset_x;
    int             gc_region_tag_offset_y;

    ndkPixmap       *gc_stipple;
    ndkPixmap       *gc_tile;
    ndkPixmap       *gc_clip_mask;
    ndkPixmap       *gc_old_clip_mask;

    unsigned int    gc_fg_pixel;
    unsigned int    gc_bg_pixel;
    ndkGCfunction   gc_function;

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
    bool            gc_fill_rule;
    bool            gc_exposures;
};

#endif

