
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
 * Ginterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifdef WITH_X11

#include "config.h"
#include "graphics.h"
#include "x11draw.h"
#include "miscutil/texttf.h"

#include <math.h>
#include <stdio.h>


//
// Exportable Graphics
// This provides the Xdraw functions for drawing into a foreign X window.
//


namespace {
    bool x_error;

    int handle_x_error(Display*, XErrorEvent*)
    {
        x_error = true;
        return (1);
    }
}


X11draw::X11draw(Display *d, Window w)
{
    xd_display = d;
    xd_window = w;

    x_error = false;
    XErrorHandler erh = XSetErrorHandler((XErrorHandler)handle_x_error);
    XGCValues gcv;
    gcv.cap_style = CapButt;
    gcv.fill_rule = WindingRule;
    xd_gc = XCreateGC(xd_display, xd_window, GCCapStyle|GCFillRule, &gcv);
    gcv.function = GXxor;
    xd_xorgc = XCreateGC(xd_display, xd_window,
        GCFunction|GCCapStyle|GCFillRule, &gcv);
    xd_font = XLoadQueryFont(xd_display, "fixed");
    XSetErrorHandler((XErrorHandler)erh);

}


X11draw::~X11draw()
{
    if (xd_gc)
        XFreeGC(xd_display, xd_gc);
    if (xd_xorgc)
        XFreeGC(xd_display, xd_xorgc);
    if (xd_font)
        XFreeFont(xd_display, xd_font);
    if (xd_display)
        XCloseDisplay(xd_display);
}


// Clear the drawing window.
//
void
X11draw::Clear()
{
    XClearWindow(xd_display, xd_window);
}


// Draw a pixel at x, y.
//
void
X11draw::Pixel(int x, int y)
{
    XDrawPoint(xd_display, xd_window, xd_gc, x, y);
}


// Draw multiple pixels from list.
//
void
X11draw::Pixels(GRmultiPt *p, int n)
{
    if (GRmultiPt::is_short_data())
        XDrawPoints(xd_display, xd_window, xd_gc, (XPoint*)p->data(), n,
            CoordModeOrigin);
    else {
        XPoint *pd = new XPoint[n];
        p->data_ptr_init();
        for (int i = 0; i < n; i++) {
            pd[i].x = p->data_ptr_x();
            pd[i].y = p->data_ptr_y();
            p->data_ptr_inc();
        }
        XDrawPoints(xd_display, xd_window, xd_gc, pd, n, CoordModeOrigin);
        delete [] pd;
    }
}


// Draw a line.
//
void
X11draw::Line(int x1, int y1, int x2, int y2)
{
    XDrawLine(xd_display, xd_window, xd_gc, x1, y1, x2, y2);
}


// Draw a path.
//
void
X11draw::PolyLine(GRmultiPt *p, int n)
{
    if (GRmultiPt::is_short_data())
        XDrawLines(xd_display, xd_window, xd_gc, (XPoint*)p->data(), n,
            CoordModeOrigin);
    else {
        XPoint *pd = new XPoint[n];
        p->data_ptr_init();
        for (int i = 0; i < n; i++) {
            pd[i].x = p->data_ptr_x();
            pd[i].y = p->data_ptr_y();
            p->data_ptr_inc();
        }
        XDrawLines(xd_display, xd_window, xd_gc, pd, n, CoordModeOrigin);
        delete [] pd;
    }
}


// Draw a collection of lines.
//
void
X11draw::Lines(GRmultiPt *p, int n)
{
    if (GRmultiPt::is_short_data())
        XDrawSegments(xd_display, xd_window, xd_gc, (XSegment*)p->data(), n);
    else {
        XSegment *pd = new XSegment[n];
        p->data_ptr_init();
        for (int i = 0; i < n; i++) {
            pd[i].x1 = p->data_ptr_x();
            pd[i].y1 = p->data_ptr_y();
            p->data_ptr_inc();
            pd[i].x2 = p->data_ptr_x();
            pd[i].y2 = p->data_ptr_y();
            p->data_ptr_inc();
        }
        XDrawSegments(xd_display, xd_window, xd_gc, pd, n);
        delete [] pd;
    }
}


// Draw a filled box.
//
void
X11draw::Box(int x1, int y1, int x2, int y2)
{
    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
    }
    XFillRectangle(xd_display, xd_window, xd_gc, x1, y1,
        x2 - x1 + 1, y2 - y1 + 1);
}


// Draw a collection of filled boxes.
//
void
X11draw::Boxes(GRmultiPt *p, int n)
{
    if (GRmultiPt::is_short_data()) {
        XFillRectangles(xd_display, xd_window, xd_gc,
            (XRectangle*)p->data(), n);
    }
    else {
        XRectangle *pd = new XRectangle[n];
        p->data_ptr_init();
        for (int i = 0; i < n; i++) {
            pd[i].x = p->data_ptr_x();
            pd[i].y = p->data_ptr_y();
            p->data_ptr_inc();
            pd[i].width = p->data_ptr_x();
            pd[i].height = p->data_ptr_y();
            p->data_ptr_inc();
        }
        XFillRectangles(xd_display, xd_window, xd_gc, pd, n);
        delete [] pd;
    }
}


// Draw an arc.
//
void
X11draw::Arc(int x0, int y0, int rx, int ry, double theta1, double theta2)
{
    if (rx <= 0 || ry <= 0)
        return;
    if (theta1 >= theta2)
        theta2 = 2 * M_PI + theta2;
    int t1 = (int)(64 * (180.0 / M_PI) * theta1);
    int t2 = (int)(64 * (180.0 / M_PI) * theta2 - t1);
    if (t2 == 0)
        return;
    XDrawArc(xd_display, xd_window, xd_gc, x0 - rx, y0 - ry, 2*rx, 2*ry,
        t1, t2);
}


// Draw a filled polygon.
//
void
X11draw::Polygon(GRmultiPt *p, int numv)
{
    if (GRmultiPt::is_short_data())
        XFillPolygon(xd_display, xd_window, xd_gc, (XPoint*)p->data(), numv,
            Complex, CoordModeOrigin);
    else {
        XPoint *pd = new XPoint[numv];
        p->data_ptr_init();
        for (int i = 0; i < numv; i++) {
            pd[i].x = p->data_ptr_x();
            pd[i].y = p->data_ptr_y();
            p->data_ptr_inc();
        }
        XFillPolygon(xd_display, xd_window, xd_gc, pd, numv, Complex,
            CoordModeOrigin);
        delete [] pd;
    }
}


void
X11draw::Zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
{
    if (yl >= yu)
        return;
    XPoint points[5];
    int n = 0;
    points[n].x = xll;
    points[n].y = yl;
    n++;
    points[n].x = xul;
    points[n].y = yu;
    n++;
    if (xur > xul) {
        points[n].x = xur;
        points[n].y = yu;
        n++;
    }
    points[n].x = xlr;
    points[n].y = yl;
    n++;
    if (xll < xlr) {
        points[n].x = xll;
        points[n].y = yl;
        n++;
    }
    if (n >= 4)
        XFillPolygon(xd_display, xd_window, xd_gc, points, n, Convex,
            CoordModeOrigin);
}


// Render text.  Go through hoops to provide rotated/mirrored
// rendering.
// note: x and y are the LOWER left corner of text
//
void
X11draw::Text(const char *text, int x, int y, int xform, int width,
    int height)
{
    if (!width || !height)
        return;
    // Handle rotated text, not simple in X!
    XCharStruct overall;
    overall.width = 0;
    int asc, dsc, dir;
    if (xform & (TXTF_HJC | TXTF_HJR)) {
        XTextExtents(xd_font, text, strlen(text), &dir, &asc, &dsc, &overall);
        if (xform & TXTF_HJR)
            x -= overall.width;
        else
            x -= overall.width/2;
    }
    xform &= (TXTF_ROT | TXTF_MX | TXTF_MY);
    if (!xform || xform == 14) {
        // 0 no rotation, 14 MX MY R180
        XDrawString(xd_display, xd_window, xd_gc, x, y, text, strlen(text));
        return;
    }

    if (!overall.width)
        XTextExtents(xd_font, text, strlen(text), &dir, &asc, &dsc, &overall);
    int wid = overall.width;
    int hei = asc + dsc;
    Pixmap p = XCreatePixmap(xd_display, xd_window, wid, hei,
        DefaultDepth(xd_display, DefaultScreen(xd_display)));

    XDrawImageString(xd_display, p, xd_gc, 0, asc, text, strlen(text));

    XImage *im = XGetImage(xd_display, p, 0, 0, wid, hei, 0xff, XYPixmap);
    XFreePixmap(xd_display, p);

    XImage *im1;
    char *data;
    if (xform & 1) {
        // rotation
        int nbpl = (hei/im->bitmap_unit + 1)*(im->bitmap_unit/8);
        data = new char [nbpl*wid*im->depth];
        im1 = new XImage;
        *im1 = *im;
        im1->data = data;
        im1->bytes_per_line = nbpl;
        im1->width = im->height;
        im1->height = im->width;
    }
    else {
        int nbpl = im->bytes_per_line;
        data = new char [nbpl*hei*im->depth];
        im1 = new XImage;
        *im1 = *im;
        im1->data = data;
    }
    int i, j;
    unsigned long px;
    switch (xform) {
    case 1:  // R90
    case 15: // MX MY R270
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = XGetPixel(im, i, j);
                XPutPixel(im1, j, wid-i-1, px);
            }
        }
        y += wid;
        break;

    case 2:  // R180
    case 12: // MX MY
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = XGetPixel(im, i, j);
                XPutPixel(im1, wid-i-1, hei-j-1, px);
            }
        }
        y += hei;
        break;

    case 3:  // R270
    case 13: // MX MY R90
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = XGetPixel(im, i, j);
                XPutPixel(im1, hei-j-1, i, px);
            }
        }
        y += wid;
        break;

    case 4:  // MY
    case 10: // MX R180
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = XGetPixel(im, i, j);
                XPutPixel(im1, i, hei-j-1, px);
            }
        }
        y += hei;
        break;

    case 5:  // MY R90
    case 11: // MX R270
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = XGetPixel(im, i, j);
                XPutPixel(im1, j, i, px);
            }
        }
        y += wid;
        break;

    case 6:  // MY R180
    case 8:  // MX
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = XGetPixel(im, i, j);
                XPutPixel(im1, wid-i-1, j, px);
            }
        }
        y += hei;
        break;

    case 7:  // MY R270
    case 9:  // MX R90
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = XGetPixel(im, i, j);
                XPutPixel(im1, hei-j-1, wid-i-1, px);
            }
        }
        y += wid;
        break;
    }

    XDestroyImage(im);
    if (xform & 1)
        // rotation
        XPutImage(xd_display, xd_window, xd_gc, im1, 0, 0, x, y, hei, wid);
    else
        XPutImage(xd_display, xd_window, xd_gc, im1, 0, 0, x, y, wid, hei);
    delete [] data;
    delete im1;
}


// Return the width/height of text.  If text is 0, return
// the max bounds of any character.
//
void
X11draw::TextExtent(const char *text, int *wid, int *hei)
{
    if (!text) {
        *wid = xd_font->max_bounds.width;
        *hei = xd_font->max_bounds.ascent + xd_font->max_bounds.descent;
    }
    else {
        XCharStruct overall;
        int asc, dsc, dir;
        XTextExtents(xd_font, text, strlen(text), &dir, &asc, &dsc, &overall);
        *wid = overall.width;
        *hei = asc + dsc;
    }
}


// Define a new rgb value for pixel (if read/write cell) or return a
// new pixel with a matching color.
//
void
X11draw::DefineColor(int *pixel, int red, int green, int blue)
{
    XColor newcolor;
    newcolor.red   = (red   * 256);
    newcolor.green = (green * 256);
    newcolor.blue  = (blue  * 256);
    newcolor.flags = -1;
    newcolor.pixel = *pixel;
    if (XAllocColor(xd_display, DefaultColormap(xd_display, 0), &newcolor))
        *pixel = newcolor.pixel;
    else
        *pixel = 0;
}


// Set the window background in the GC's.
//
void
X11draw::SetBackground(int pixel)
{
    XSetBackground(xd_display, xd_gc, pixel);
    XSetBackground(xd_display, xd_xorgc, pixel);
}


// Actually change the drwaing window background.
//
void
X11draw::SetWindowBackground(int pixel)
{
    XSetWindowBackground(xd_display, xd_window, pixel);
}


// Set the current foreground color.
//
void
X11draw::SetColor(int pixel)
{
    if (xd_gc == xd_xorgc)
        return;
    XSetForeground(xd_display, xd_gc, pixel);
}


// Set the current linestyle.
//
void
X11draw::SetLinestyle(const GRlineType *lineptr)
{
    XGCValues values;
    if (!lineptr || !lineptr->mask || lineptr->mask == -1) {
        values.line_style = LineSolid;
        XChangeGC(xd_display, xd_gc, GCLineStyle, &values);
        return;
    }
    values.line_style = LineOnOffDash;
    XChangeGC(xd_display, xd_gc, GCLineStyle, &values);

    XSetDashes(xd_display, xd_gc, lineptr->offset,
        (char*)lineptr->dashes, lineptr->length);
}


// Create a new pixmap for the fill pattern.
//
void
X11draw::DefineFillpattern(GRfillType *fillp)
{
    if (fillp->xPixmap()) {
        XFreePixmap(xd_display, (Pixmap)fillp->xPixmap());
        fillp->setXpixmap(0);
    }
    if (fillp->hasMap()) {
        unsigned char *map = fillp->newBitmap();
        fillp->setXpixmap((GRfillData)XCreateBitmapFromData(xd_display,
            xd_window, (char*)map, fillp->nX(), fillp->nY()));
        delete [] map;
    }
}



// Set the current fill pattern.
//
void
X11draw::SetFillpattern(const GRfillType *fillp)
{
    if (!fillp || !fillp->xPixmap())
        XSetFillStyle(xd_display, xd_gc, FillSolid);
    else {
        XSetStipple(xd_display, xd_gc, (Pixmap)fillp->xPixmap());
        XSetFillStyle(xd_display, xd_gc, FillStippled);
    }
}


// Update the display.
//
void
X11draw::Update()
{
    XSync(xd_display, 0);
    XFlush(xd_display);
}


// Switch to/from ghost (xor) or highlight/unhighlight drawing context.
// Highlight/unhighlight works only with dual-plane color cells.  It is
// essential to call this with GRxNone after each mode change, due to
// the static storage.
//
void
X11draw::SetXOR(int mode)
{
    static GC tempgc;
    switch (mode) {
    case GRxNone:
        XSetFunction(xd_display, xd_xorgc, GXxor);
        xd_gc = tempgc;
        break;
    case GRxXor:
        tempgc = xd_gc;
        xd_gc = xd_xorgc;
        break;
    case GRxHlite:
        XSetFunction(xd_display, xd_xorgc, GXor);
        tempgc = xd_gc;
        xd_gc = xd_xorgc;
        break;
    case GRxUnhlite:
        XSetFunction(xd_display, xd_xorgc, GXandInverted);
        tempgc = xd_gc;
        xd_gc = xd_xorgc;
        break;
    }
}


void
X11draw::DisplayImage(const GRimage *image, int x, int y,
    int width, int height)
{
    XImage *im = XGetImage(xd_display, xd_window, 0, 0, width, height,
        0xffffffff, XYPixmap);

    for (int i = 0; i < height; i++) {
        int yd = i + y;
        if (yd < 0)
            continue;
        if ((unsigned int)yd >= image->height())
            break;
        for (int j = 0; j < width; j++) {
            int xd = j + x;
            if (xd < 0)
                continue;
            if ((unsigned int)xd >= image->width())
                break;
            XPutPixel(im, j, i,
                image->data()[xd + yd*image->width()]);
        }
    }
    XPutImage(xd_display, xd_window, xd_gc, im, 0, 0, x, y, width, height);
    XDestroyImage(im);
}


double
X11draw::Resolution()
{
    return (1.0);
}
// End of X11draw functions.


Xdraw::Xdraw(const char *dname, unsigned long win)
{
    xp = 0;
    Display *display = XOpenDisplay(dname);
    if (display) {
        xp = new X11draw(display, win);
        if (x_error) {
            delete xp;
            xp = 0;
        }
        xp->defineLinestyle(&linetypes[1], 0xa);
        xp->defineLinestyle(&linetypes[2], 0xc);
        xp->defineLinestyle(&linetypes[3], 0x924);
        xp->defineLinestyle(&linetypes[4], 0xe38);
        xp->defineLinestyle(&linetypes[5], 0x8);
        xp->defineLinestyle(&linetypes[6], 0xc3f0);
        xp->defineLinestyle(&linetypes[7], 0xfe03f80);
        xp->defineLinestyle(&linetypes[8], 0x1040);
        xp->defineLinestyle(&linetypes[9], 0xe3f8);
        xp->defineLinestyle(&linetypes[10], 0x3c48);
    }
    else
        x_error = true;
}


Xdraw::~Xdraw()
{
    for (int i = 0; i < XD_NUM_FILLPATTS; i++) {
        if (filltypes[i].xPixmap())
            XFreePixmap(xp->display(), (Pixmap)filltypes[i].xPixmap());
    }
    delete xp;
}


bool
Xdraw::check_error()
{
    return (x_error);
}

unsigned long
Xdraw::create_pixmap(int width, int height)
{
    x_error = false;
    XErrorHandler erh = XSetErrorHandler((XErrorHandler)handle_x_error);
    Window root;
    int xr, yr;
    unsigned int wr, hr, bwr, dr;
    XGetGeometry(xp->display(), xp->window(), &root, &xr, &yr, &wr, &hr,
        &bwr, &dr);
    if (x_error) {
        XSetErrorHandler((XErrorHandler)erh);
        return (0);
    }
    if (width <= 0)
        width = wr;
    if (height <= 0)
        height = hr;
    Pixmap p = XCreatePixmap(xp->display(), xp->window(), width, height,
        DefaultDepth(xp->display(), DefaultScreen(xp->display())));
    XSetErrorHandler((XErrorHandler)erh);
    return (p);
}


bool
Xdraw::destroy_pixmap(Pixmap pixmap)
{
    x_error = false;
    XErrorHandler erh = XSetErrorHandler((XErrorHandler)handle_x_error);
    XFreePixmap(xp->display(), pixmap);
    XSetErrorHandler((XErrorHandler)erh);
    return (!x_error);
}


bool
Xdraw::copy_drawable(Drawable dest, Drawable src, int l, int b, int r,
    int t, int x, int y)
{
    x_error = false;
    XErrorHandler erh = XSetErrorHandler((XErrorHandler)handle_x_error);
    int xs, ys, ws, hs;
    if (l || b || r || t) {
        xs = (l < r ? l : r);
        ys = (t < b ? t : b);
        ws = abs(r - l);
        hs = abs(t - b);
    }
    else {
        Window root;
        int xr, yr;
        unsigned int wr, hr, bwr, dr;
        XGetGeometry(xp->display(), src, &root, &xr, &yr, &wr, &hr, &bwr, &dr);
        if (x_error) {
            XSetErrorHandler((XErrorHandler)erh);
            return (false);
        }
        xs = ys = 0;
        ws = wr;
        hs = hr;
    }
    XCopyArea(xp->display(), src, dest, xp->gc(), xs, ys, ws, hs, x, y);
    XSetErrorHandler((XErrorHandler)erh);
    return (!x_error);
}


// This will display the current cell in the window win, which is cached
// in the server indicated by the display string dstring.  The l, b, r, t
// is the field to render in internal coordinates.
//
bool
Xdraw::draw(int l, int b, int r, int t)
{
    x_error = false;
    XErrorHandler erh = XSetErrorHandler((XErrorHandler)handle_x_error);
    Window root;
    int x, y;
    unsigned int w, h, bw, d;
    XGetGeometry(xp->display(), xp->window(), &root, &x, &y, &w, &h, &bw, &d);
    if (x_error) {
        XSetErrorHandler((XErrorHandler)erh);
        XCloseDisplay(xp->display());
        return (false);
    }
    bool ret = GRappIf()->DrawCallback(xp->display(), xp, l, b, r, t, w, h);
    XSetErrorHandler((XErrorHandler)erh);
    return (ret);
}


bool
Xdraw::get_drawable_size(Drawable win, int *width, int *height)
{
    if (!win)
        win = xp->window();
    x_error = false;
    XErrorHandler erh = XSetErrorHandler((XErrorHandler)handle_x_error);
    Window root;
    int xr, yr;
    unsigned int wr, hr, bwr, dr;
    XGetGeometry(xp->display(), win, &root, &xr, &yr, &wr, &hr, &bwr, &dr);
    XSetErrorHandler((XErrorHandler)erh);
    if (x_error)
        return (false);
    *width = wr;
    *height = hr;
    return (true);
}


unsigned long
Xdraw::reset_drawable(unsigned long win)
{
    Window prev = xp->window();
    xp->set_window((Window)win);
    return (prev);
}


void
Xdraw::Clear()
{
    xp->Clear();
}


void
Xdraw::Pixel(int x, int y)
{
    xp->Pixel(x, y);
}


void
Xdraw::Pixels(GRmultiPt *p, int n)
{
    xp->Pixels(p, n);
}


void
Xdraw::Line(int x1, int y1, int x2, int y2)
{
    xp->Line(x1, y1, x2, y2);
}


void
Xdraw::PolyLine(GRmultiPt *p, int n)
{
    xp->PolyLine(p, n);
}


void
Xdraw::Lines(GRmultiPt *p, int n)
{
    xp->Lines(p, n);
}


void
Xdraw::Box(int x1, int y1, int x2, int y2)
{
    xp->Box(x1, y1, x2, y2);
}


void
Xdraw::Boxes(GRmultiPt *p, int n)
{
    xp->Boxes(p, n);
}


void
Xdraw::Arc(int x0, int y0, int rx, int ry, double theta1, double theta2)
{
    xp->Arc(x0, y0, rx, ry, theta1, theta2);
}


void
Xdraw::Polygon(GRmultiPt *p, int numv)
{
    xp->Polygon(p, numv);
}


void
Xdraw::Zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
{
    xp->Zoid(yl, yu, xll, xul, xlr, xur);
}


void
Xdraw::Text(const char *text, int x, int y, int xform)
{
    xp->Text(text, x, y, xform);
}


void
Xdraw::TextExtent(const char *text, int *wid, int *hei)
{
    xp->TextExtent(text, wid, hei);
}


void
Xdraw::DefineColor(int *pixel, int red, int green, int blue)
{
    xp->DefineColor(pixel, red, green, blue);
}


void
Xdraw::SetBackground(int pixel)
{
    xp->SetBackground(pixel);
}


void
Xdraw::SetWindowBackground(int pixel)
{
    xp->SetWindowBackground(pixel);
}


void
Xdraw::SetColor(int pixel)
{
    xp->SetColor(pixel);
}


void
Xdraw::DefineLinestyle(int indx, int mask)
{
    if (indx > 0 && indx < XD_NUM_LINESTYLES)
        xp->defineLinestyle(&linetypes[indx], mask);
}


void
Xdraw::SetLinestyle(int indx)
{
    if (indx >= 0 && indx < XD_NUM_LINESTYLES)
        xp->SetLinestyle(&linetypes[indx]);
}


void
Xdraw::DefineFillpattern(int indx, int nx, int ny, unsigned char *array)
{
    if (indx > 0 && indx < XD_NUM_FILLPATTS && (nx == 8 || nx == 16) &&
            (ny == 8 || ny == 16) && array)
        xp->defineFillpattern(&filltypes[indx], nx, ny, array);
}


void
Xdraw::SetFillpattern(int indx)
{
    if (indx > 0 && indx < XD_NUM_FILLPATTS)
        xp->SetFillpattern(&filltypes[indx]);
}


void
Xdraw::Update()
{
    xp->Update();
}


void
Xdraw::SetXOR(int mode)
{
    xp->SetXOR(mode);
}
// End of Xdraw functions

#else

// The xdraw interface is not supported yet
Xdraw::Xdraw(const char*, unsigned long) { xp = 0; }
Xdraw::~Xdraw() { }

bool Xdraw::check_error() { return (true); }
unsigned long Xdraw::create_pixmap(int, int) { return (0); }
bool Xdraw::destroy_pixmap(unsigned long) { return (false); }
bool Xdraw::copy_drawable(unsigned long, unsigned long, int, int, int, int,
    int, int) { return (false); }
bool Xdraw::draw(int, int, int, int) { return (false); }
bool Xdraw::get_drawable_size(unsigned long, int*, int*)
    { return (false); }
unsigned long Xdraw::reset_drawable(unsigned long) { return (0); }
void Xdraw::Clear() { }
void Xdraw::Pixel(int, int) { }
void Xdraw::Pixels(GRmultiPt*, int) { }
void Xdraw::Line(int, int, int, int) { }
void Xdraw::PolyLine(GRmultiPt*, int) { }
void Xdraw::Lines(GRmultiPt*, int) { }
void Xdraw::Box(int, int, int, int) { }
void Xdraw::Boxes(GRmultiPt*, int) { }
void Xdraw::Arc(int, int, int, int, double, double) { }
void Xdraw::Polygon(GRmultiPt*, int) { }
void Xdraw::Zoid(int, int, int, int, int, int) { }
void Xdraw::Text(const char*, int, int, int) { }
void Xdraw::TextExtent(const char*, int*, int*) { }
void Xdraw::DefineColor(int*, int, int, int) { }
void Xdraw::SetBackground(int) { }
void Xdraw::SetWindowBackground(int) { }
void Xdraw::SetColor(int) { }
void Xdraw::DefineLinestyle(int, int) { }
void Xdraw::SetLinestyle(int) { }
void Xdraw::DefineFillpattern(int, int, int, unsigned char*) { }
void Xdraw::SetFillpattern(int) { }
void Xdraw::Update() { }
void Xdraw::SetXOR(int) { }
// End of Xdraw functions

#endif

