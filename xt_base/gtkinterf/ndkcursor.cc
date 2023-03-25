
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

// A Cursor that can have transparency the old way, with a mask, which
// is no longer available from stock GTK.

#include "config.h"
#include "gtkinterf.h"

#ifdef NDKCURSOR_H

ndkCursor::ndkCursor(GdkWindow *window, const char *data, const char *mask,
    int width, int height, int xhot, int yhot, GdkColor *fg, GdkColor *bg)
{
    if (!window)
        return;
    Display *xdisplay = gdk_x11_display_get_xdisplay(
        gdk_window_get_display(window));
    c_datapm = new ndkPixmap(window, data, width, height);
    c_maskpm = new ndkPixmap(window, mask, width, height);
    XColor xfg, xbg;
    xfg.pixel = fg->pixel;
    xfg.red = fg->red;
    xfg.green = fg->green;
    xfg.blue = fg->blue;
    xbg.pixel = bg->pixel;
    xbg.red = bg->red;
    xbg.green = bg->green;
    xbg.blue = bg->blue;
    c_xcursor = XCreatePixmapCursor(xdisplay, c_datapm->get_xid(),
        c_maskpm->get_xid(), &xfg, &xbg, xhot, yhot);
}


ndkCursor::~ndkCursor()
{
    if (c_datapm)
        c_datapm->dec_ref();
    if (c_maskpm)
        c_maskpm->dec_ref();
}


void
ndkCursor::set_in_window(GdkWindow *window)
{
    if (!window)
        return;
    Display *xdisplay = gdk_x11_display_get_xdisplay(
        gdk_window_get_display(window));
#if GTK_CHECK_VERSION(3,0,0)
    XDefineCursor(xdisplay, gdk_x11_window_get_xid(window), c_xcursor);
#else
    XDefineCursor(xdisplay, gdk_x11_drawable_get_xid(window), c_xcursor);
#endif
}


void
ndkCursor::revert_in_window(GdkWindow *window)
{
    if (!window)
        return;
    Display *xdisplay = gdk_x11_display_get_xdisplay(
        gdk_window_get_display(window));
#if GTK_CHECK_VERSION(3,0,0)
    XDefineCursor(xdisplay, gdk_x11_window_get_xid(window), None);
#else
    XDefineCursor(xdisplay, gdk_x11_drawable_get_xid(window), None);
#endif
    gdk_window_set_cursor(window, 0);
}

#endif

