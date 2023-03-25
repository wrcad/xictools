
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

#ifndef NDKDRAWABLE_H
#define NDKDRAWABLE_H


enum DW_STATE
{
    DW_NONE,
    DW_WINDOW,
    DW_PIXMAP
};

struct ndkDrawable
{
    ndkDrawable();
    ~ndkDrawable();

    GdkWindow *get_window()         { return (d_window); }
    ndkPixmap *get_pixmap()         { return (d_pixmap); }
    DW_STATE get_state()            { return (d_state); }
#ifdef WITH_X11
    Drawable get_xid()              { return (d_xid); }
#endif

    void set_window(GdkWindow*);
    void set_pixmap(GdkWindow*);
    void set_pixmap(ndkPixmap*);
    void set_draw_to_window();
    bool set_draw_to_pixmap();
    bool check_compatible_pixmap();
    void copy_pixmap_to_window(ndkGC*, int, int, int, int);
#if GTK_CHECK_VERSION(3,0,0)
    void refresh(ndkGC*, cairo_t*);
#else
    void refresh(ndkGC*, GdkEventExpose*);
#endif
    GdkScreen *get_screen();
    GdkVisual *get_visual();
    int get_width();
    int get_height();
    int get_depth();

private:
    GdkWindow *d_window;
    ndkPixmap *d_pixmap;

#ifdef WITH_X11
    Window d_xid;
#endif
    DW_STATE d_state;
};

#endif

