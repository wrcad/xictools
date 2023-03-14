
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

// ntkimage.cc:  This derives from the gdkimage.c from GTK-2.0.  It
// supports a ndkImage class which is a replacement for the GdkImage
// but which fits into the post-2.0 context.

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

#include "config.h"
#include "gtkinterf.h"

#ifdef NDKIMAGE_H

ndkImage::ndkImage(ndkImageType type, GdkVisual *visual,
    int width, int height)
{
    if (!visual)
        visual = GRX->Visual();
  
    im_screen = gdk_visual_get_screen(visual);
    im_type = type;
    im_visual = visual;
    im_width = width;
    im_height = height;
    im_depth = gdk_visual_get_depth(visual);

    Visual *xvisual = gdk_x11_visual_get_xvisual(visual);
    Display *xdisplay = gdk_x11_display_get_xdisplay(
        gdk_screen_get_display(im_screen));

    bool try_normal = false;
    if (type == ndkIMAGE_FASTEST) {
        try_normal = true;
        type = ndkIMAGE_SHARED;
    }
    if (type == ndkIMAGE_SHARED) {
#ifdef USE_SHM
        if (gdk_x11_display_get_use_xshm(gdk_screen_get_display));
            im_x_shm_info = g_new(XShmSegmentInfo, 1);
            XShmSegmentInfo *x_shm_info = im_x_shm_info;
            x_shm_info->shmid = -1;
            x_shm_info->shmaddr = (char*) -1;

            im_ximage = XShmCreateImage(xdisplay, xvisual, depth, ZPixmap,
                0, x_shm_info, width, height);
            if (!im_ximage) {
                g_warning ("XShmCreateImage failed");
                gdk_x11_display_set_use_xshm(gdk_screen_get_display), false);
                if (im_x_shm_info) {
                    x_shm_info = im_x_shm_info;
                  
                    if (x_shm_info->shmaddr != (char *)-1)
                        shmdt(x_shm_info->shmaddr);
                    if (x_shm_info->shmid != -1) 
                        shmctl(x_shm_info->shmid, IPC_RMID, NULL);
                  
                    g_free(x_shm_info);
                    im_x_shm_info = 0;
                }
                if (try_normal)
                    type = ndkIMAGE_NORMAL;
                goto shmerr;
            }
            x_shm_info->shmid = shmget(IPC_PRIVATE,
                  xi_ximage->im_bytes_per_line * xi_ximage->height,
                  IPC_CREAT | 0600);

            if (x_shm_info->shmid == -1) {
                // EINVAL indicates, most likely, that the segment
                // we asked for is bigger than SHMMAX, so we don't
                // treat it as a permanent error.  ENOSPC and
                // ENOMEM may also indicate this, but more likely
                // are permanent errors.

                if (errno != EINVAL) {
                    g_warning(
            "shmget failed: error %d (%s)", errno, g_strerror (errno));
                    gdk_x11_display_set_use_xshm(gdk_screen_get_display),
                        false);
                }
                XDestroyImage(im_ximage);
                im_ximage = 0;
                if (im_x_shm_info) {
                    x_shm_info = im_x_shm_info;
                  
                    if (x_shm_info->shmaddr != (char *)-1)
                        shmdt(x_shm_info->shmaddr);
                    if (x_shm_info->shmid != -1) 
                        shmctl(x_shm_info->shmid, IPC_RMID, NULL);
                  
                    g_free(x_shm_info);
                    im_x_shm_info = 0;
                }
                if (try_normal)
                    type = ndkIMAGE_NORMAL;
                goto shmerr;
            }

            x_shm_info->readOnly = False;
            x_shm_info->shmaddr = shmat(x_shm_info->shmid, NULL, 0);
            im_ximage->data = x_shm_info->shmaddr;

            if (x_shm_info->shmaddr == (char*) -1) {
                g_warning(
                "shmat failed: error %d (%s)", errno, g_strerror (errno));
                // Failure in shmat is almost certainly permanent. 
                // Most likely error is EMFILE, which would mean
                // that we've exceeded the per-process Shm segment
                // limit.
                gdk_x11_display_set_use_xshm(gdk_screen_get_display), false);
                XDestroyImage(im_ximage);
                im_ximage = 0;
                if (im_x_shm_info) {
                    x_shm_info = im_x_shm_info;
                  
                    if (x_shm_info->shmaddr != (char *)-1)
                        shmdt(x_shm_info->shmaddr);
                    if (x_shm_info->shmid != -1) 
                        shmctl(x_shm_info->shmid, IPC_RMID, NULL);
                  
                    g_free(x_shm_info);
                    im_x_shm_info = 0;
                }
                if (try_normal)
                    type = ndkIMAGE_NORMAL;
                goto shmerr;
            }
            gdk_error_trap_push ();

            XShmAttach(xdisplay, x_shm_info);
            XSync(xdisplay, False);

            if (gdk_error_trap_pop()) {
                // This is the common failure case so omit warning.
                gdk_x11_display_set_use_xshm(gdk_screen_get_display), false);
                XDestroyImage(im_ximage);
                im_ximage = 0;
                if (im_x_shm_info) {
                    x_shm_info = im_x_shm_info;
                  
                    if (x_shm_info->shmaddr != (char *)-1)
                        shmdt(x_shm_info->shmaddr);
                    if (x_shm_info->shmid != -1) 
                        shmctl(x_shm_info->shmid, IPC_RMID, NULL);
                  
                    g_free(x_shm_info);
                    im_x_shm_info = 0;
                }
                if (try_normal)
                    type = ndkIMAGE_NORMAL;
                goto shmerr;
            }
      
            // We mark the segment as destroyed so that when the
            // last process detaches, it will be deleted.  There
            // is a small possibility of leaking if we die in
            // XShmAttach.  In theory, a signal handler could be
            // set up.
            //
            shmctl(x_shm_info->shmid, IPC_RMID, NULL);             

            image_list = g_list_prepend(image_list, this);
        }
        else
#endif  // USE_SHM
            if (try_normal)
                type = ndkIMAGE_NORMAL;
    }
#ifdef USE_SHM
shmerr:
#endif

    if (type == ndkIMAGE_NORMAL) {
        im_ximage = XCreateImage(xdisplay, xvisual, im_depth, ZPixmap, 0, 0,
            width, height, 32, 0);

        // Use malloc, not g_malloc here, because X will call
        // free() on this data
        im_ximage->data = (char*)malloc(
            im_ximage->bytes_per_line * im_ximage->height);
        if (!im_ximage->data) {
            XDestroyImage(im_ximage);
            im_ximage = 0;
        }
    }

    if (!im_ximage) {
        // Somethng unfortunate happened.
        im_mem = 0;
        im_bpl = 0;
        im_bits_per_pixel = 0;
        im_bpp = 0;
        im_byte_order = GDK_LSB_FIRST;
        return;
    }
    im_byte_order = (im_ximage->byte_order == LSBFirst) ?
        GDK_LSB_FIRST : GDK_MSB_FIRST;
    im_mem = im_ximage->data;
    im_bpl = im_ximage->bytes_per_line;
    im_bpp = (im_ximage->bits_per_pixel + 7) / 8;
    im_bits_per_pixel = im_ximage->bits_per_pixel;
}


#ifdef XXX_DEPREC
ndkImage::ndkImage(GdkDrawable *drawable, int src_x, int src_y,
    int width, int height)
{
    im_type = ndkIMAGE_NORMAL;
    im_screen = gdk_drawable_get_screen(drawable);
    im_visual = gdk_drawable_get_visual(drawable); // could be NULL
    im_width = width;
    im_height = height;
    im_depth = gdk_drawable_get_depth(drawable);
  
    im_ximage = XGetImage(
        gdk_x11_display_get_xdisplay(gdk_screen_get_display(im_screen)),
        gdk_x11_drawable_get_xid(drawable),
        src_x, src_y, width, height, AllPlanes, ZPixmap);
  
    if (!im_ximage) {
        // Somethng unfortunate happened.
        im_mem = 0;
        im_bpl = 0;
        im_bits_per_pixel = 0;
        im_bpp = 0;
        im_byte_order = GDK_LSB_FIRST;
        return;
    }
    im_mem = im_ximage->data;
    im_bpl = im_ximage->bytes_per_line;
    im_bits_per_pixel = im_ximage->bits_per_pixel;
    im_bpp = (im_ximage->bits_per_pixel + 7) / 8;
    im_byte_order = (im_ximage->byte_order == LSBFirst) ?
        GDK_LSB_FIRST : GDK_MSB_FIRST;
}
#endif

#ifdef NEW_PIX

ndkImage::ndkImage(ndkPixmap *pixmap, int src_x, int src_y,
    int width, int height)
{
    im_type = ndkIMAGE_NORMAL;
    im_screen = pixmap->get_screen();
    im_visual = pixmap->get_visual();
    im_width = width;
    im_height = height;
    im_depth = pixmap->get_depth();
  
    im_ximage = XGetImage(
        gdk_x11_display_get_xdisplay(gdk_screen_get_display(im_screen)),
        pixmap->get_xid(), src_x, src_y, width, height, AllPlanes, ZPixmap);
  
    if (!im_ximage) {
        // Somethng unfortunate happened.
        im_mem = 0;
        im_bpl = 0;
        im_bits_per_pixel = 0;
        im_bpp = 0;
        im_byte_order = GDK_LSB_FIRST;
        return;
    }
    im_mem = im_ximage->data;
    im_bpl = im_ximage->bytes_per_line;
    im_bits_per_pixel = im_ximage->bits_per_pixel;
    im_bpp = (im_ximage->bits_per_pixel + 7) / 8;
    im_byte_order = (im_ximage->byte_order == LSBFirst) ?
        GDK_LSB_FIRST : GDK_MSB_FIRST;
}


/*
ndkImage::ndkImage(GdkWindow *window, int src_x, int src_y,
    int width, int height)
{
    im_type = ndkIMAGE_NORMAL;
    im_screen = gdk_window_get_screen(window);
    im_visual = gdk_drawable_get_visual(window); // could be NULL
    im_width = width;
    im_height = height;
    im_depth = gdk_visual_get_depth(im_visual);
  
    im_ximage = XGetImage(GDK_SCREEN_XDISPLAY(im_screen),
        gdk_x11_drawable_get_xid(window), src_x, src_y, width, height,
        AllPlanes, ZPixmap);
  
    if (!im_ximage) {
        // Somethng unfortunate happened.
        im_mem = 0;
        im_bpl = 0;
        im_bits_per_pixel = 0;
        im_bpp = 0;
        im_byte_order = GDK_LSB_FIRST;
        return;
    }
    im_mem = im_ximage->data;
    im_bpl = im_ximage->bytes_per_line;
    im_bits_per_pixel = im_ximage->bits_per_pixel;
    im_bpp = (im_ximage->bits_per_pixel + 7) / 8;
    im_byte_order = (im_ximage->byte_order == LSBFirst) ?
        GDK_LSB_FIRST : GDK_MSB_FIRST;
}
*/


#endif
#ifdef NEW_DRW

ndkImage::ndkImage(ndkDrawable *drawable, int src_x, int src_y,
    int width, int height)
{
    im_type = ndkIMAGE_NORMAL;
    im_screen = drawable->get_screen();
    im_visual = drawable->get_visual();
    im_width = width;
    im_height = height;
    im_depth = drawable->get_depth();
  
    im_ximage = XGetImage(GDK_SCREEN_XDISPLAY(im_screen),
        drawable->get_xid(), src_x, src_y, width, height, AllPlanes, ZPixmap);
  
    if (!im_ximage) {
        // Somethng unfortunate happened.
        im_mem = 0;
        im_bpl = 0;
        im_bits_per_pixel = 0;
        im_bpp = 0;
        im_byte_order = GDK_LSB_FIRST;
        return;
    }
    im_mem = im_ximage->data;
    im_bpl = im_ximage->bytes_per_line;
    im_bits_per_pixel = im_ximage->bits_per_pixel;
    im_bpp = (im_ximage->bits_per_pixel + 7) / 8;
    im_byte_order = (im_ximage->byte_order == LSBFirst) ?
        GDK_LSB_FIRST : GDK_MSB_FIRST;
}

#endif

ndkImage::~ndkImage()
{
#ifdef WITH_X11
    if (im_ximage) {
        // Deal with failure of creation.
        switch(im_type) {
        case ndkIMAGE_NORMAL:
            if (!gdk_display_is_closed(gdk_screen_get_display(im_screen)))
                XDestroyImage(im_ximage);
            break;
      
        case ndkIMAGE_SHARED:
#ifdef USE_SHM
            if (!gdk_screen_is_closed(im_screen)) {
                gdk_display_sync(GDK_SCREEN_DISPLAY(im_screen));

                if (im_shm_pixmap)
                    XFreePixmap(GDK_SCREEN_XDISPLAY(im_screen), im_shm_pixmap);
              
                XShmDetach(GDK_SCREEN_XDISPLAY(im_screen), im_x_shm_info);
                XDestroyImage(im_ximage);
            }
      
            image_list = g_list_remove(image_list, image);

            XShmSegmentInfo *x_shm_info = im_x_shm_info;
            shmdt(x_shm_info->shmaddr);
          
            g_free(im_x_shm_info);
            im_x_shm_info = 0;
#else
            g_error(
      "trying to destroy shared memory image when gdk was compiled without\n"
      "shared memory support");
#endif
            break;
      
        case ndkIMAGE_FASTEST:
            // not reached
            break;
        }
        im_ximage = 0;
    }
#endif  // With_X11
}


#ifdef XXX_NOTDEF
ndkImage::copy_from_drawable(GdkDrawable *drawable,
    int src_x, int src_y, int dest_x, int dest_y, int width, int height)
{

    /*
GdkImage*
_gdk_x11_copy_to_image(GdkDrawable *drawable, GdkImage *image,
    int src_x, int src_y, int dest_x, int dest_y, int width, int height)
    */
{
    GdkRectangle window_rect;
    bool success = true;

    GdkVisual *visual = gdk_drawable_get_visual(drawable);
    GdkDisplay *display = gdk_drawable_get_display(drawable);
    Display *xdisplay = gdk_x11_display_get_xdisplay(display);
    Drawable xid = gtk_x11_drawable_get_xid(drawable);
    GdkScreen = gdk_drawable_get_scfreen(drawable);

    if (gdk_display_is_closed(display))
        return;
  
    bool have_grab = false;

#define UNGRAB() G_STMT_START { \
    if (have_grab) { \
        gdk_x11_display_ungrab(display); \
        have_grab = false; } \
    } G_STMT_END

    Pixmap shm_pixmap = None;
    if (im_type == GDK_IMAGE_SHARED) {
        shm_pixmap = im_shm_pixmap;
        if (shm_pixmap) {
            XGCValues values;

            // Again easy, we can just XCopyArea, and don't have to
            // worry about clipping.
            values.subwindow_mode = IncludeInferiors;
            GC xgc = XCreateGC(xdisplay, xid, GCSubwindowMode, &values);
      
            XCopyArea(xdisplay, xid, shm_pixmap, xgc, src_x, src_y,
                width, height, dest_x, dest_y);
            XSync(xdisplay, false);
            XFreeGC(xdisplay, xgc);
            return;
        }
    }

    // Now the general case - we may have to worry about clipping to
    // the screen bounds, in which case we'll have to grab the server
    // and only get a piece of the window.
    //

    if (GDK_IS_WINDOW(drawable)) {

        have_grab = true;
        gdk_x11_display_grab(display);

        // Translate screen area into window coordinates.
        GdkRectangle screen_rect;
        Window child;
        XTranslateCoordinates(xdisplay, GDK_SCREEN_XROOTWIN(screen),
            xid, 0, 0, &screen_rect.x, &screen_rect.y, &child);

        screen_rect.width = gdk_screen_get_width(screen);
        screen_rect.height = gdk_screen_get_height(screen);
      
        gdk_error_trap_push ();

        window_rect.x = 0;
        window_rect.y = 0;
      
        gdk_window_get_geometry(GDK_WINDOW(drawable), NULL, NULL,
            &window_rect.width, &window_rect.height, NULL);
      
        // compute intersection of screen and window, in window
        // coordinates
        //

        if (gdk_error_trap_pop () || !gdk_rectangle_intersect(
                &window_rect, &screen_rect, &window_rect))
            goto out;
    }
    else {
        window_rect.x = 0;
        window_rect.y = 0;
        gdk_drawable_get_size(drawable,
            &window_rect.width, &window_rect.height);
    }
      
    GdkRectangle req;
    req.x = src_x;
    req.y = src_y;
    req.width = width;
    req.height = height;
  
    // window_rect specifies the part of drawable which we can get from
    // the server in window coordinates. 
    // For pixmaps this is all of the pixmap, for windows it is just 
    // the onscreen part.

    if (!gdk_rectangle_intersect(&req, &window_rect, &req))
        goto out;

    gdk_error_trap_push ();
  
    if (!image && req.x == src_x && req.y == src_y && req.width == width &&
            req.height == height) {
        image = get_full_image(drawable, src_x, src_y, width, height);
        if (!image)
            success = false;
    }
    else {
        bool created_image = false;
      
        if (!image) {
            image = _gdk_image_new_for_depth(impl->screen, GDK_IMAGE_NORMAL, 
                visual, width, height, gdk_drawable_get_depth (drawable));
            created_image = true;
        }

        // In the ShmImage but no ShmPixmap case, we could use
        // XShmGetImage when we are getting the entire image.

        if (XGetSubImage(xdisplay, impl->xid, req.x, req.y,
                req.width, req.height, AllPlanes, ZPixmap, image->im_ximage,
                dest_x + req.x - src_x, dest_y + req.y - src_y) == None) {
            if (created_image)
                g_object_unref(image);
            success = false;
        }
    }
    gdk_error_trap_pop ();

out:
  
    if (have_grab) {               
        gdk_x11_display_ungrab(display);
        have_grab = false;
    }
  
    if (success && !image) {
        // We "succeeded", but could get no content for the image so
        // return junk.
        image = _gdk_image_new_for_depth(impl->screen, GDK_IMAGE_NORMAL, 
            visual, width, height, gdk_drawable_get_depth(drawable));
    }
      
    return (image);
}
#endif  // XXX_NOTDEF

void
ndkImage::copy_to_drawable(GdkDrawable *drawable, ndkGC *gc,
    int xsrc, int ysrc, int xdest, int ydest, int width, int height)
{
#ifdef USE_SHM  
    if (im_type == ndkIMAGE_SHARED) {
        XShmPutImage(GDK_SCREEN_XDISPLAY(im_screen),
            gdk_x11_drawable_get_xid(drawable), gc->get_xgc(),
            im_ximage, xsrc, ysrc, xdest, ydest, width, height, False);
        return;
    }
#endif
    XPutImage(GDK_SCREEN_XDISPLAY(im_screen), 
        gdk_x11_drawable_get_xid(drawable), gc->get_xgc(), im_ximage,
        xsrc, ysrc, xdest, ydest, width, height);
}


#if defined(NEW_DRW) && defined(NEW_GC)

void
ndkImage::copy_to_drawable(ndkDrawable *drawable, ndkGC *gc,
    int xsrc, int ysrc, int xdest, int ydest, int width, int height)
{
#ifdef USE_SHM  
    if (im_type == ndkIMAGE_SHARED) {
        XShmPutImage(
            gc->get_xdisplay(), drawable->get_xid(), gc->get_xgc(),
            im_ximage, xsrc, ysrc, xdest, ydest, width, height, False);
        return;
    }
#endif
    XPutImage(
        gc->get_xdisplay(), drawable->get_xid(), gc->get_xgc(), im_ximage,
        xsrc, ysrc, xdest, ydest, width, height);
}

#endif


#if defined(NEW_PIX) && defined(NEW_GC)

void
ndkImage::copy_to_pixmap(ndkPixmap *pixmap, ndkGC *gc,
    int xsrc, int ysrc, int xdest, int ydest, int width, int height)
{
#ifdef USE_SHM  
    if (im_type == ndkIMAGE_SHARED) {
        XShmPutImage(
            gc->get_xdisplay(), pixmap->get_xid(), gc->get_xgc(),
            im_ximage, xsrc, ysrc, xdest, ydest, width, height, False);
        return;
    }
#endif
    XPutImage(
        gc->get_xdisplay(), pixmap->get_xid(), gc->get_xgc(), im_ximage,
        xsrc, ysrc, xdest, ydest, width, height);
}

#endif

void
ndkImage::put_pixel(int x, int y, unsigned int pixel)
{
    if (!gdk_display_is_closed(gdk_screen_get_display(im_screen)))
        XPutPixel(im_ximage, x, y, pixel);
}


unsigned int
ndkImage::get_pixel(int x, int y)
{
    unsigned int pixel = 0;
    if (!gdk_display_is_closed(gdk_screen_get_display(im_screen)))
        pixel = XGetPixel(im_ximage, x, y);
    return (pixel);
}

#ifdef XXX_NOTUSED

Display *
ndkImage::image_get_xdisplay()
{
    return GDK_SCREEN_XDISPLAY(im_screen);
}


XImage *
ndkImage::x11_image_get_ximage()
{
    if (gdk_screen_is_closed(im_screen))
        return (0);
    return (im_ximage);
}


namespace {
    int
    _gdk_windowing_get_bits_for_depth(GdkDisplay *display, int depth)
    {
        int count;
        XPixmapFormatValues *formats =
            XListPixmapFormats(GDK_DISPLAY_XDISPLAY(display), &count);
      
        for (int i = 0; i < count; i++) {
            if (formats[i].depth == depth) {
                int result = formats[i].bits_per_pixel;
                XFree(formats);
                return (result);
            }
        }
        return (-1);
    }
}

namespace {
    GList *image_list;

    void image_exit()
    {
        while (image_list) {
            image = image_list->data;
            gdk_x11_image_destroy (image);
        }
    }
}
#endif

#endif  // NDKIMAGE_H

