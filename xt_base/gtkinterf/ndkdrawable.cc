
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
        d_xid = gdk_x11_drawable_get_xid(d_window);
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


void
ndkDrawable::set_draw_to_window()
{
#ifdef WITH_X11
    if (d_window) {
        d_xid = gdk_x11_drawable_get_xid(d_window);
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
        d_pixmap = new ndkPixmap(d_window, wid, hei,false);
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


int
ndkDrawable::get_width()
{
    if (d_state == DW_WINDOW && d_window)
        return (gdk_window_get_width(d_window));
    if (d_state == DW_PIXMAP && d_pixmap)
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
    return (-1);
}

#endif

