
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
 * IMSAVE Image Dump Facility
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/*------------------------------------------------------------------------*
 *
 * IMSAVE -- Screen Dump Utility
 *
 * S. R. Whiteley (stevew@wrcad.com)
 *------------------------------------------------------------------------*
 * Borrowed extensively from Imlib-1.9.8
 *
 * This software is Copyright (C) 1998 By The Rasterman (Carsten
 * Haitzler).  I accept no responsability for anything this software
 * may or may not do to your system - you use it completely at your
 * own risk.  This software comes under the LGPL and GPL licences.
 * The library itself is LGPL (all software in Imlib and gdk_imlib
 * directories) see the COPYING and COPYING.LIB files for full legal
 * details.
 *
 * (Rasterdude :-) <raster@redhat.com>)
 *------------------------------------------------------------------------*/

#include "imsave.h"

#ifdef WIN32

#include <windows.h>


Image *
create_image_from_drawable(void *drawable, unsigned long, int x, int y,
    int width, int height)
{
    // Since we don't have the window here, assume that x, y, width, height
    // have been clipped to the window
    //
    HDC dc = (HDC)drawable;
    HDC dcmem = CreateCompatibleDC(dc);
    HBITMAP hbm = CreateCompatibleBitmap(dc, width, height);
    SelectObject(dcmem, hbm);
    BitBlt(dcmem, 0, 0, width, height, dc, x, y, SRCCOPY);

    BITMAPINFO bmi;
    BITMAPINFOHEADER *h = &bmi.bmiHeader;
    h->biSize = sizeof(BITMAPINFOHEADER);
    h->biWidth = width;
    h->biHeight = -height;
    h->biPlanes = 1;
    h->biBitCount = 32;
    h->biCompression = BI_RGB;
    h->biSizeImage = 0;
    h->biXPelsPerMeter = 0;
    h->biYPelsPerMeter = 0;
    h->biClrUsed = 0;
    h->biClrImportant = 0;

    unsigned char *vals = new unsigned char[4*width*height];
    GetDIBits(dcmem, hbm, 0, height, vals, &bmi, DIB_RGB_COLORS);

    unsigned char *data = new unsigned char[width * height * 3];
    unsigned char *ptr = data;
    for (int yy = 0; yy < height; yy++) {
        unsigned char *p = vals + 4*yy*width;
        for (int xx = 0; xx < width; xx++) {
            *ptr++ = p[2];
            *ptr++ = p[1];
            *ptr++ = p[0];
            p += 4;
        }
    }
    delete [] vals;
    DeleteObject(hbm);
    DeleteDC(dcmem);

    return (new Image(width, height, data));
    
    /******
     ** old slow method
    unsigned char *data = new unsigned char[width * height * 3];
    unsigned char *ptr = data;
    for (int yy = y; yy < y + height; yy++) {
        for (int xx = x; xx < x + width; xx++) {
            unsigned long pixel = GetPixel(dc, xx, yy);
            *ptr++ = GetRValue(pixel);
            *ptr++ = GetGValue(pixel);
            *ptr++ = GetBValue(pixel);
        }
    }
    return (new Image(width, height, data));
    ******/
}

#else

#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace {
    struct colorcell
    {
        colorcell() { r = g = b = 0; pixel = 0; }

        int     r, g, b;
        int     pixel;
    };

    bool x_error;

    void
    handle_x_error(Display*, XErrorEvent*)
    {
        x_error = true;
    }


    // Struct to handle bitfield mapping of color planes
    //
    struct pix_d
    {
        pix_d(Visual*);

        unsigned rmask, gmask, bmask;
        int rshift, gshift, bshift;
    };


    pix_d::pix_d(Visual *v)
    {
        rmask = v->red_mask;
        gmask = v->green_mask;
        bmask = v->blue_mask;
        rshift = 0;
        gshift = 0;
        bshift = 0;
        unsigned m = 1;
        while (!(m & rmask)) {
            m <<= 1;
            rshift++;
        }
        unsigned msk = rmask >> rshift;
        m = 0x80;
        while (!(m & msk)) {
            rshift--;
            m >>= 1;
        }
        m = 1;
        while (!(m & gmask)) {
            m <<= 1;
            gshift++;
        }
        msk = gmask >> gshift;
        m = 0x80;
        while (!(m & msk)) {
            gshift--;
            m >>= 1;
        }
        m = 1;
        while (!(m & bmask)) {
            m <<= 1;
            bshift++;
        }
        msk = bmask >> bshift;
        m = 0x80;
        while (!(m & msk)) {
            bshift--;
            m >>= 1;
        }
    }
}


Image *
create_image_from_drawable(void *dp, unsigned long drawable, int x, int y,
    int width, int height)
{
    int w = width;
    int h = height;
    Window win = (Window)drawable;
    Display *display = (Display*)dp;
    if (!display || !win)
        return (0);

    XErrorHandler erh = XSetErrorHandler((XErrorHandler)handle_x_error);
    x_error = false;
    XWindowAttributes xatt;
    XGetWindowAttributes(display, win, &xatt);
    XFlush(display);
    char is_pixmap = 0;
    if (x_error) {
        x_error = 0;
        is_pixmap = 1;
        int tmp;
        Window chld;
        XGetGeometry(display, win, &chld, &tmp, &tmp,
            (unsigned int*)&xatt.width, (unsigned int*)&xatt.height,
            (unsigned int*)&tmp, (unsigned int*)&xatt.depth);
        xatt.visual = DefaultVisual(display, DefaultScreen(display));
        XFlush(display);
        if (x_error) {
            XFlush(display);
            XSetErrorHandler((XErrorHandler)erh);
            return (0);
        }
    }
    XSetErrorHandler((XErrorHandler) erh);
    int rx = 0, ry = 0;
    XWindowAttributes ratt;
    if (!is_pixmap) {
        XGetWindowAttributes(display, xatt.root, &ratt);
        Window chld;
        XTranslateCoordinates(display, win, xatt.root, 0, 0, &rx, &ry, &chld);
        if ((xatt.map_state != IsViewable) &&
                (xatt.backing_store == NotUseful)) {
            XFlush(display);
            return (0);
        }
    }
    //int clipx = 0;
    //int clipy = 0;

    width = xatt.width - x;
    height = xatt.height - y;
    if (width > w)
        width = w;
    if (height > h)
        height = h;

    if (!is_pixmap) {
        if ((rx + x + width) > ratt.width)
            width = ratt.width - (rx + x);
        if ((ry + y + height) > ratt.height)
            height = ratt.height - (ry + y);
    }
    if (x < 0) {
        //clipx = -x;
        width += x;
        x = 0;
    }
    if (y < 0) {
        //clipy = -y;
        height += y;
        y = 0;
    }
    if (!is_pixmap) {
        if ((rx + x) < 0) {
            //clipx -= (rx + x);
            width += (rx + x);
            x = -rx;
        }
        if ((ry + y) < 0) {
            //clipy -= (ry + y);
            height += (ry + y);
            y = -ry;
        }
    }
    if ((width <= 0) || (height <= 0)) {
        XSync(display, False);
        return (0);
    }
    XImage *xim = XGetImage(display, win, x, y, width, height, 0xffffffff,
        ZPixmap);
    XFlush(display);

    colorcell ctab[256];
    if (xatt.depth == 1) {
        ctab[0].r = 255;
        ctab[0].g = 255;
        ctab[0].b = 255;
        ctab[1].r = 0;
        ctab[1].g = 0;
        ctab[1].b = 0;
    }
    if (xatt.depth <= 8) {
        XColor cols[256];
        for (int i = 0; i < (1 << xatt.depth); i++) {
            cols[i].pixel = i;
            cols[i].flags = DoRed | DoGreen | DoBlue;
        }
        Colormap cmap = is_pixmap ?
            DefaultColormap(display, DefaultScreen(display)) : xatt.colormap;
        XQueryColors(display, cmap, cols, 1 << xatt.depth);
        for (int i = 0; i < (1 << xatt.depth); i++) {
            ctab[i].r = cols[i].red >> 8;
            ctab[i].g = cols[i].green >> 8;
            ctab[i].b = cols[i].blue >> 8;
            ctab[i].pixel = cols[i].pixel;
        }
    }
    unsigned char *data = new unsigned char[width * height * 3];
    unsigned char *ptr = data;
    switch (xatt.depth) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        for (int yy = 0; yy < height; yy++) {
            for (int xx = 0; xx < width; xx++) {
                unsigned long pixel = XGetPixel(xim, xx, yy);
                *ptr++ = ctab[pixel & 0xff].r;
                *ptr++ = ctab[pixel & 0xff].g;
                *ptr++ = ctab[pixel & 0xff].b;
            }
        }
        break;
    case 15:
    case 16:
    case 24:
    case 32:
        {
            pix_d p(xatt.visual);
            for (int yy = 0; yy < height; yy++) {
                for (int xx = 0; xx < width; xx++) {
                    unsigned long pixel = XGetPixel(xim, xx, yy);
                    if (p.rshift >= 0)
                        *ptr++ = (pixel & p.rmask) >> p.rshift;
                    else
                        *ptr++ = (pixel & p.rmask) << -p.rshift;
                    if (p.gshift >= 0)
                        *ptr++ = (pixel & p.gmask) >> p.gshift;
                    else
                        *ptr++ = (pixel & p.gmask) << -p.gshift;
                    if (p.bshift >= 0)
                        *ptr++ = (pixel & p.bmask) >> p.bshift;
                    else
                        *ptr++ = (pixel & p.bmask) << -p.bshift;
                }
            }
        }
        break;
    default:
        for (int yy = 0; yy < height; yy++) {
            for (int xx = 0; xx < width; xx++) {
                *ptr++ = 0;
                *ptr++ = 0;
                *ptr++ = 0;
            }
        }
        break;
    }
    XDestroyImage(xim);

    return (new Image(w, h, data));
}

#endif

