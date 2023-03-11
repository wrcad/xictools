
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

#ifndef NDKPIXMAP_H
#define NDKPIXMAP_H


#ifdef NEW_GC
struct ndkGC;
#endif
#ifdef NEW_DRW
struct ndkDrawable;
#endif

struct ndkPixmap
{
#ifdef NEW_DRW
    ndkPixmap(GdkWindow*, int, int, bool=false);
    ndkPixmap(GdkWindow*, const char*, int, int);
    ndkPixmap(GdkWindow*, const char*, int, int,
        const GdkColor*, const GdkColor*);
#else
    ndkPixmap(GdkDrawable*, int, int, bool=false);
    ndkPixmap(GdkDrawable*, const char*, int, int);
    ndkPixmap(GdkDrawable*, const char*, int, int,
        const GdkColor*, const GdkColor*);
#endif
    ndkPixmap(ndkPixmap*, int, int, bool=false);
    ~ndkPixmap();

#ifdef NEW_DRW
    void copy_to_window(GdkWindow*, ndkGC*, int, int, int, int, int, int);
    void copy_from_window(GdkWindow*, ndkGC*, int, int, int, int, int, int);
#else
    void copy_to_window(GdkDrawable*, ndkGC*, int, int, int, int, int, int);
    void copy_from_window(GdkDrawable*, ndkGC*, int, int, int, int, int, int);
#endif
    void copy_to_pixmap(ndkPixmap*, ndkGC*, int, int, int, int, int, int);
    void copy_from_pixmap(ndkPixmap*, ndkGC*, int, int, int, int, int, int);
#ifdef NEW_DRW
    void copy_to_drawable(ndkDrawable*, ndkGC*, int, int, int, int, int, int);
    void copy_from_drawable(ndkDrawable*, ndkGC*, int, int, int, int, int, int);
#endif
    static ndkPixmap *lookup(unsigned long);

    void inc_ref()      { pm_refcnt++; }
    void dec_ref()      { pm_refcnt--; if (!pm_refcnt) delete this; }
    int get_width()     { return (pm_width); }
    int get_height()    { return (pm_height); }
    int get_depth()     { return (pm_depth); }
    GdkScreen *get_screen() { return (pm_screen); }
    GdkVisual *get_visual() { return (pm_visual); }
#ifdef WITH_X11
    Pixmap get_xid()    { return (pm_xid); }
#endif

private:
    int pm_width;
    int pm_height;
    int pm_depth;
    int pm_refcnt;
    GdkScreen *pm_screen;
    GdkVisual *pm_visual;
#ifdef WITH_X11
    Pixmap pm_xid;
#endif
};

#endif  // NDKPIXMAP_H

