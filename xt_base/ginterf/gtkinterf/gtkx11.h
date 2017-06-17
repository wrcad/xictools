
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id: gtkx11.h,v 2.10 2015/07/31 22:37:01 stevew Exp $
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

