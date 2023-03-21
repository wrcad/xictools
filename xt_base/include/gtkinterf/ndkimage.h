
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

#ifndef NDKIMAGE_H
#define NDKIMAGE_H


struct ndkGC;
struct ndkPixmap;
struct ndkDrawable;

// Types of images.
//   Normal: Normal X image type. These are slow as they involve passing
//       the entire image through the X connection each time a draw
//       request is required. On Win32, a bitmap.
//   Shared: Shared memory X image type. These are fast as the X server
//       and the program actually use the same piece of memory. They
//       should be used with care though as there is the possibility
//       for both the X server and the program to be reading/writing
//       the image simultaneously and producing undesired results.
//       On Win32, also a bitmap.
//
enum ndkImageType
{
    ndkIMAGE_NORMAL,
    ndkIMAGE_SHARED,
    ndkIMAGE_FASTEST
};

struct ndkImage
{
    ndkImage(ndkImageType, GdkVisual*, int, int);
    ndkImage(ndkPixmap*, int, int, int, int);
    ndkImage(ndkDrawable*, int, int, int, int);
    ndkImage(GdkWindow*, int, int, int, int);
    ~ndkImage();

    void copy_to_drawable(ndkDrawable*, ndkGC*, int, int, int, int, int, int);
    void copy_to_pixmap(ndkPixmap*, ndkGC*, int, int, int, int, int, int);
    void copy_to_window(GdkWindow*, ndkGC*, int, int, int, int, int, int);
    void put_pixel(int, int, unsigned int);
    unsigned int get_pixel(int, int);

#ifdef NOTGDK3
    void set_colormap(GdkColormap*);
    GdkColormap *get_colormap();
#endif

    ndkImageType    get_image_type()        { return (im_type); }
    GdkVisual       *get_visual()           { return (im_visual); }
    GdkByteOrder    get_byte_order()        { return (im_byte_order); }
    int             get_width()             { return (im_width); }
    int             get_height()            { return (im_height); }
    int             get_depth()             { return (im_depth); }
    int             get_bytes_per_pixel()   { return (im_bpp); }
    int             get_bytes_per_line()    { return (im_bpl); }
    int             get_bits_per_pixel()    { return (im_bits_per_pixel); }
    gpointer        get_pixels()            { return (im_mem); }

private:
    ndkImageType    im_type;
    GdkVisual       *im_visual;
    GdkByteOrder    im_byte_order;
    int             im_width;
    int             im_height;
    unsigned short  im_depth;
    unsigned short  im_bpp;
    unsigned short  im_bpl;
    unsigned short  im_bits_per_pixel;
    gpointer        im_mem;
#ifdef NOTGDK3
    GdkColormap     *im_colormap;
#endif

#ifdef WITH_X11
    XImage          *im_ximage;
    GdkScreen       *im_screen;
    gpointer        im_x_shm_info;
    Pixmap          im_shm_pixmap;
#endif
};

#endif  // NDKIMAGE_H

