
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef GTKX11_H
#define GTKX11_H

#include <gdk/gdkx.h>

// Some X info functions, encapsulate gtk1/2 differences.


namespace gtkinterf {
    inline Display *
    gr_x_display()
    {
        return (gdk_x11_get_default_xdisplay());
    }


    inline int
    gr_x_screen()
    {
        return (gdk_x11_get_default_screen());
    }


    inline Window
    gr_x_window(GdkWindow *window)
    {
        return (GDK_WINDOW_XWINDOW(window));
    }

    inline Visual *
    gr_x_visual(GdkVisual *visual)
    {
        return (GDK_VISUAL_XVISUAL(visual));
    }

    inline GC
    gr_x_gc(GdkGC *gc)
    {
        return (gdk_x11_gc_get_xgc(gc));
    }


    inline Colormap
    gr_x_colormap(GdkColormap *cmap)
    {
        return (gdk_x11_colormap_get_xcolormap(cmap));
    }


    inline Atom
    gr_x_atom_intern(const char *atom_name, bool only_if_exists)
    {
        return (gdk_x11_atom_to_xatom(gdk_atom_intern(atom_name,
            only_if_exists)));
    }
}
// End of encapsulation

#endif

