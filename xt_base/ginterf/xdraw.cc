
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

#include "config.h"
#include "graphics.h"
#include "xdraw.h"
#include "miscutil/texttf.h"
#ifdef HAVE_MOZY
#include "imsave/imsave.h"
#endif

#ifdef WITH_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <math.h>
#else
#ifdef WIN32
#include "mswdraw.h"
#endif
#endif


//
// Exportable Graphics
// This provides the Xdraw functions for drawing into a foreign X window,
// and the Image print driver, plus support for that driver when graphics
// is not available in the main application.
//

namespace {
    const char *IMSresols[] = { "100", "200", 0 };
}

namespace ginterf
{
    // Hard-copy driver - uses imsave to dump to a supported file
    // type.  The file type is determined from the suffix of the given
    // file name.
    //
    HCdesc Xdesc(
        "X",                // drname
#ifdef WIN32
        "Image: jpeg, tiff, png, etc, or \"clipboard\"", // descr
#else
        "Image: jpeg, tiff, png, etc", // descr
#endif
        "image",            // keyword
        "imlib",            // alias
        "-f %s -r %d -w %f -h %f", // format
        HClimits(
            0.0, 15.0,  // minxoff, maxxoff
            0.0, 15.0,  // minyoff, maxyoff
            1.0, 16.5,  // minwidth, maxwidth
            1.0, 16.5,  // minheight, maxheight
                        // flags
            HCdontCareXoff | HCdontCareYoff | HCfileOnly,
            IMSresols   // resolutions
        ),
        HCdefaults(
            0.0, 0.0,   // defxoff, defyoff
            4.0, 4.0,   // defwidth, defheight
            0,          // command string to print file
            0,          // defresol (index into resols)
            HClegOff,   // initially no legend
            HCbest      // initially set best orientation
        ),
        false);     // line_draw
}


bool
Xdev::Init(int *ac, char **av)
{
    if (!ac || !av)
        return (true);
    HCdata *hd = new HCdata;
    hd->filename = 0;
    hd->resol = 100;
    hd->width = 8.0;
    hd->height = 10.5;
    hd->xoff = .25;
    hd->yoff = .25;
    hd->landscape = false;
    if (HCdevParse(hd, ac, av)) {
        delete hd;
        return (true);
    }

    if (!hd->filename || !*hd->filename) {
        delete hd;
        return (true);
    }
    HCcheckEntries(hd, &Xdesc);

    width = (int)(hd->width * hd->resol);
    height = (int)(hd->height * hd->resol);
    numlinestyles = 16;
    numcolors = 32;

    delete data;
    data = hd;
    return (false);
}


#ifdef WITH_X11

namespace ginterf
{
    struct Xparams : public GRdraw
    {
        Xparams(Display *d, Window w);
        virtual ~Xparams();

        unsigned long create_pixmap(int, int);
        bool destroy_pixmap(Pixmap);
        bool copy_drawable(Drawable, Drawable, int, int, int, int, int, int);
        bool draw(int, int, int, int);
        bool get_drawable_size(Drawable, int*, int*);

        Window reset_drawable(Drawable d)
            { Window prev = window; window = d; return (prev); }

        void *WindowID()                                        { return (0); }
        void Halt();
        void Title(const char*, const char*)                    { }
        void Clear();
        void ResetViewport(int, int);
        void DefineViewport();
        void Dump(int)                                          { }
        void Pixel(int, int);
        void Pixels(GRmultiPt*, int);
        void Line(int, int, int, int);
        void PolyLine(GRmultiPt*, int);
        void Lines(GRmultiPt*, int);
        void Box(int, int, int, int);
        void Boxes(GRmultiPt*, int);
        void Arc(int, int, int, int, double, double);
        void Polygon(GRmultiPt*, int);
        void Zoid(int, int, int, int, int, int);
        void Text(const char*, int, int, int, int = -1, int = -1);
        void TextExtent(const char*, int*, int*);
        void SetGhost(GhostDrawFunc, int, int)                  { }
        void ShowGhost(bool)                                    { }
//        bool ShowingGhost()                                 { return(false); }
        void UndrawGhost(bool = false)                          { }
        void DrawGhost(int, int)                                { }
        void DrawGhost()                                        { }
        void QueryPointer(int*, int*, unsigned*)                { }
        void DefineColor(int*, int, int, int);
        void SetBackground(int);
        void SetWindowBackground(int);
        void SetGhostColor(int)                                 { }
        void SetColor(int);
        void DefineLinestyle(GRlineType*)                       { }
        void SetLinestyle(const GRlineType*);
        void DefineFillpattern(GRfillType*);
        void SetFillpattern(const GRfillType*);
//        void Refresh(int, int, int, int)                        { }
//        void Refresh()                                          { }
//        void Update(int, int, int, int)                         { }
        void Update();
//        void SetOverlayMode(bool)                               { }
//        void CreateOverlayBackg()                               { }
        void Input(int*, int*, int*, int*)                      { }
        void SetXOR(int);

        void ShowGlyph(int, int, int)                           { }
        GRobject GetRegion(int, int, int, int)          { return (0); }
        void PutRegion(GRobject, int, int, int, int)            { }
        void FreeRegion(GRobject)                               { }
        void DisplayImage(const GRimage*, int, int, int, int);
        double Resolution();

        Display *display;
        Window window;
        GC gc;
        GC xorgc;
        XFontStruct *font;
        GRlineType linetypes[XD_NUM_LINESTYLES];
        GRfillType filltypes[XD_NUM_FILLPATTS];
        Xdev *dev;
        void *lcx;
    };
}


// New viewport function
//
GRdraw *
Xdev::NewDraw(int)
{
    if (!data)
        return (0);

    const char *dname = getenv("DISPLAY");
    if (!dname)
        dname = ":0";
    Display *display = XOpenDisplay(dname);
    if (!display)
        return (0);
    Window window = RootWindow(display, DefaultScreen(display));

    Xparams *px = new Xparams(display, window);
    px->dev = this;

    // This will save current and swap in the appropriate new pixmap
    // cached in layer descs.   *** WE CAN'T USE THEM with application's
    // graphics until we reset (in Halt).
    px->lcx = GRappIf()->SetupLayers(display, px, 0);

    if (width && height) {
        Pixmap pm = XCreatePixmap(display, px->window, width, height,
            DefaultDepth(display, DefaultScreen(display)));
        if (!pm) {
            delete px;
            return (0);
        }
        px->window = pm;
        px->SetFillpattern(0);
        px->SetColor(GRappIf()->BackgroundPixel());
        px->Box(0, 0, width, height);
    }
    return (px);
}
// End of Xdev functions


namespace {
    bool x_error;

    int
    handle_x_error(Display*, XErrorEvent*)
    {
        x_error = true;
        return (1);
    }
}


Xparams::Xparams(Display *d, Window w)
{
    display = d;
    window = w;
    dev = 0;
    lcx = 0;

    x_error = false;
    XErrorHandler erh = XSetErrorHandler((XErrorHandler)handle_x_error);
    XGCValues gcv;
    gcv.cap_style = CapButt;
    gcv.fill_rule = WindingRule;
    gc = XCreateGC(display, window, GCCapStyle | GCFillRule, &gcv);
    gcv.function = GXxor;
    xorgc = XCreateGC(display, window, GCFunction | GCCapStyle | GCFillRule,
        &gcv);
    font = XLoadQueryFont(display, "fixed");
    XSetErrorHandler((XErrorHandler)erh);

    defineLinestyle(&linetypes[1], 0xa);
    defineLinestyle(&linetypes[2], 0xc);
    defineLinestyle(&linetypes[3], 0x924);
    defineLinestyle(&linetypes[4], 0xe38);
    defineLinestyle(&linetypes[5], 0x8);
    defineLinestyle(&linetypes[6], 0xc3f0);
    defineLinestyle(&linetypes[7], 0xfe03f80);
    defineLinestyle(&linetypes[8], 0x1040);
    defineLinestyle(&linetypes[9], 0xe3f8);
    defineLinestyle(&linetypes[10], 0x3c48);
}


Xparams::~Xparams()
{
    if (gc)
        XFreeGC(display, gc);
    if (xorgc)
        XFreeGC(display, xorgc);
    if (font)
        XFreeFont(display, font);
    for (int i = 0; i < XD_NUM_FILLPATTS; i++) {
        if (filltypes[i].xPixmap())
            XFreePixmap(display, (Pixmap)filltypes[i].xPixmap());
    }
    if (display)
        XCloseDisplay(display);
}


unsigned long
Xparams::create_pixmap(int width, int height)
{
    x_error = false;
    XErrorHandler erh = XSetErrorHandler((XErrorHandler)handle_x_error);
    Window root;
    int xr, yr;
    unsigned int wr, hr, bwr, dr;
    XGetGeometry(display, window, &root, &xr, &yr, &wr, &hr, &bwr, &dr);
    if (x_error) {
        XSetErrorHandler((XErrorHandler)erh);
        return (0);
    }
    if (width <= 0)
        width = wr;
    if (height <= 0)
        height = hr;
    Pixmap p = XCreatePixmap(display, window, width, height,
        DefaultDepth(display, DefaultScreen(display)));
    XSetErrorHandler((XErrorHandler)erh);
    return (p);
}


bool
Xparams::destroy_pixmap(Pixmap pixmap)
{
    x_error = false;
    XErrorHandler erh = XSetErrorHandler((XErrorHandler)handle_x_error);
    XFreePixmap(display, pixmap);
    XSetErrorHandler((XErrorHandler)erh);
    return (!x_error);
}


bool
Xparams::copy_drawable(Drawable dest, Drawable src, int l, int b, int r,
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
        XGetGeometry(display, src, &root, &xr, &yr, &wr, &hr, &bwr, &dr);
        if (x_error) {
            XSetErrorHandler((XErrorHandler)erh);
            return (false);
        }
        xs = ys = 0;
        ws = wr;
        hs = hr;
    }
    XCopyArea(display, src, dest, gc, xs, ys, ws, hs, x, y);
    XSetErrorHandler((XErrorHandler)erh);
    return (!x_error);
}


// This will display the current cell in the window win, which is cached
// in the server indicated by the display string dstring.  The l, b, r, t
// is the field to render in internal coordinates.
//
bool
Xparams::draw(int l, int b, int r, int t)
{
    x_error = false;
    XErrorHandler erh = XSetErrorHandler((XErrorHandler)handle_x_error);
    Window root;
    int x, y;
    unsigned int w, h, bw, d;
    XGetGeometry(display, window, &root, &x, &y, &w, &h, &bw, &d);
    if (x_error) {
        XSetErrorHandler((XErrorHandler)erh);
        XCloseDisplay(display);
        return (false);
    }
    bool ret = GRappIf()->DrawCallback(display, this, l, b, r, t, w, h);
    XSetErrorHandler((XErrorHandler)erh);
    return (ret);
}


bool
Xparams::get_drawable_size(Drawable win, int *width, int *height)
{
    if (!win)
        win = window;
    x_error = false;
    XErrorHandler erh = XSetErrorHandler((XErrorHandler)handle_x_error);
    Window root;
    int xr, yr;
    unsigned int wr, hr, bwr, dr;
    XGetGeometry(display, win, &root, &xr, &yr, &wr, &hr, &bwr, &dr);
    XSetErrorHandler((XErrorHandler)erh);
    if (x_error)
        return (false);
    *width = wr;
    *height = hr;
    return (true);
}


//
// The next 4 functions are used for the Image driver only.
//

// Halt driver, dump image
//
void
Xparams::Halt()
{
#ifdef HAVE_MOZY
    Image *im = create_image_from_drawable(display, window, 0, 0, dev->width,
        dev->height);
    XFreePixmap(display, window);
    if (im) {
        ImErrType ret = im->save_image(dev->data->filename, 0);
        if (ret == ImError) {
            fprintf(stderr, "Image creation failed, internal error.\n");
            GRpkg::self()->HCabort("Image creation error");
        }
        else if (ret == ImNoSupport) {
            fprintf(stderr, "Image creation failed, file type unsupported.\n");
            GRpkg::self()->HCabort("Image format unsupported");
        }
        delete im;
    }
    else
        GRpkg::self()->HCabort("Internal error");
#else
    XFreePixmap(display, window);
    fprintf(stderr, "Image creation failed, package not available.\n");
    GRpkg::self()->HCabort("Image format unsupported");
#endif

    // Put back original pixmaps in layer descs.
    GRappIf()->SetupLayers(display, this, lcx);

    lcx = 0;
    delete this;
}


void
Xparams::ResetViewport(int wid, int hei)
{
    if (wid == dev->width && hei == dev->height)
        return;
    if (wid <= 0 || hei <= 0)
        return;
    dev->width = wid;
    dev->data->width = ((double)dev->width)/dev->data->resol;
    dev->height = hei;
    dev->data->height = ((double)dev->height)/dev->data->resol;
}


void
Xparams::DefineViewport()
{
    Pixmap pm = XCreatePixmap(display, window, dev->width, dev->height,
        DefaultDepth(display, DefaultScreen(display)));
    if (!pm) {
        GRpkg::self()->HCabort("Internal error");
        return;
    }
    window = pm;
    SetFillpattern(0);
    SetColor(GRappIf()->BackgroundPixel());
    Box(0, 0, dev->width, dev->height);
}


//
// The remaining functions handle general rendering
//

// Clear the drawing window.
//
void
Xparams::Clear()
{
    XClearWindow(display, window);
}


// Draw a pixel at x, y.
//
void
Xparams::Pixel(int x, int y)
{
    XDrawPoint(display, window, gc, x, y);
}


// Draw multiple pixels from list.
//
void
Xparams::Pixels(GRmultiPt *p, int n)
{
    if (GRmultiPt::is_short_data())
        XDrawPoints(display, window, gc, (XPoint*)p->data(), n,
            CoordModeOrigin);
    else {
        XPoint *pd = new XPoint[n];
        p->data_ptr_init();
        for (int i = 0; i < n; i++) {
            pd[i].x = p->data_ptr_x();
            pd[i].y = p->data_ptr_y();
            p->data_ptr_inc();
        }
        XDrawPoints(display, window, gc, pd, n, CoordModeOrigin);
        delete [] pd;
    }
}


// Draw a line.
//
void
Xparams::Line(int x1, int y1, int x2, int y2)
{
    XDrawLine(display, window, gc, x1, y1, x2, y2);
}


// Draw a path.
//
void
Xparams::PolyLine(GRmultiPt *p, int n)
{
    if (GRmultiPt::is_short_data())
        XDrawLines(display, window, gc, (XPoint*)p->data(), n,
            CoordModeOrigin);
    else {
        XPoint *pd = new XPoint[n];
        p->data_ptr_init();
        for (int i = 0; i < n; i++) {
            pd[i].x = p->data_ptr_x();
            pd[i].y = p->data_ptr_y();
            p->data_ptr_inc();
        }
        XDrawLines(display, window, gc, pd, n, CoordModeOrigin);
        delete [] pd;
    }
}


// Draw a collection of lines.
//
void
Xparams::Lines(GRmultiPt *p, int n)
{
    if (GRmultiPt::is_short_data())
        XDrawSegments(display, window, gc, (XSegment*)p->data(), n);
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
        XDrawSegments(display, window, gc, pd, n);
        delete [] pd;
    }
}


// Draw a filled box.
//
void
Xparams::Box(int x1, int y1, int x2, int y2)
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
    XFillRectangle(display, window, gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}


// Draw a collection of filled boxes.
//
void
Xparams::Boxes(GRmultiPt *p, int n)
{
    if (GRmultiPt::is_short_data())
        XFillRectangles(display, window, gc, (XRectangle*)p->data(), n);
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
        XFillRectangles(display, window, gc, pd, n);
        delete [] pd;
    }
}


// Draw an arc.
//
void
Xparams::Arc(int x0, int y0, int rx, int ry, double theta1, double theta2)
{
    if (rx <= 0 || ry <= 0)
        return;
    if (theta1 >= theta2)
        theta2 = 2 * M_PI + theta2;
    int t1 = (int)(64 * (180.0 / M_PI) * theta1);
    int t2 = (int)(64 * (180.0 / M_PI) * theta2 - t1);
    if (t2 == 0)
        return;
    XDrawArc(display, window, gc, x0 - rx, y0 - ry, 2*rx, 2*ry, t1, t2);
}


// Draw a filled polygon.
//
void
Xparams::Polygon(GRmultiPt *p, int numv)
{
    if (GRmultiPt::is_short_data())
        XFillPolygon(display, window, gc, (XPoint*)p->data(), numv,
            Complex, CoordModeOrigin);
    else {
        XPoint *pd = new XPoint[numv];
        p->data_ptr_init();
        for (int i = 0; i < numv; i++) {
            pd[i].x = p->data_ptr_x();
            pd[i].y = p->data_ptr_y();
            p->data_ptr_inc();
        }
        XFillPolygon(display, window, gc, pd, numv, Complex, CoordModeOrigin);
        delete [] pd;
    }
}


void
Xparams::Zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
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
        XFillPolygon(display, window, gc, points, n, Convex, CoordModeOrigin);
}


// Render text.  Go through hoops to provide rotated/mirrored
// rendering.
// note: x and y are the LOWER left corner of text
//
void
Xparams::Text(const char *text, int x, int y, int xform, int width,
    int height)
{
    if (!width || !height)
        return;
    // Handle rotated text, not simple in X!
    XCharStruct overall;
    overall.width = 0;
    int asc, dsc, dir;
    if (xform & (TXTF_HJC | TXTF_HJR)) {
        XTextExtents(font, text, strlen(text), &dir, &asc, &dsc, &overall);
        if (xform & TXTF_HJR)
            x -= overall.width;
        else
            x -= overall.width/2;
    }
    xform &= (TXTF_ROT | TXTF_MX | TXTF_MY);
    if (!xform || xform == 14) {
        // 0 no rotation, 14 MX MY R180
        XDrawString(display, window, gc, x, y, text, strlen(text));
        return;
    }

    if (!overall.width)
        XTextExtents(font, text, strlen(text), &dir, &asc, &dsc, &overall);
    int wid = overall.width;
    int hei = asc + dsc;
    Pixmap p = XCreatePixmap(display, window, wid, hei,
        DefaultDepth(display, DefaultScreen(display)));

    XDrawImageString(display, p, gc, 0, asc, text, strlen(text));

    XImage *im = XGetImage(display, p, 0, 0, wid, hei, 0xff, XYPixmap);
    XFreePixmap(display, p);

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
        XPutImage(display, window, gc, im1, 0, 0, x, y, hei, wid);
    else
        XPutImage(display, window, gc, im1, 0, 0, x, y, wid, hei);
    delete [] data;
    delete im1;
}


// Return the width/height of text.  If text is 0, return
// the max bounds of any character.
//
void
Xparams::TextExtent(const char *text, int *wid, int *hei)
{
    if (!text) {
        *wid = font->max_bounds.width;
        *hei = font->max_bounds.ascent + font->max_bounds.descent;
    }
    else {
        XCharStruct overall;
        int asc, dsc, dir;
        XTextExtents(font, text, strlen(text), &dir, &asc, &dsc, &overall);
        *wid = overall.width;
        *hei = asc + dsc;
    }
}


// Define a new rgb value for pixel (if read/write cell) or return a
// new pixel with a matching color.
//
void
Xparams::DefineColor(int *pixel, int red, int green, int blue)
{
    XColor newcolor;
    newcolor.red   = (red   * 256);
    newcolor.green = (green * 256);
    newcolor.blue  = (blue  * 256);
    newcolor.flags = -1;
    newcolor.pixel = *pixel;
    if (XAllocColor(display, DefaultColormap(display, 0), &newcolor))
        *pixel = newcolor.pixel;
    else
        *pixel = 0;
}


// Set the window background in the GC's.
//
void
Xparams::SetBackground(int pixel)
{
    XSetBackground(display, gc, pixel);
    XSetBackground(display, xorgc, pixel);
}


// Actually change the drwaing window background.
//
void
Xparams::SetWindowBackground(int pixel)
{
    XSetWindowBackground(display, window, pixel);
}


// Set the current foreground color.
//
void
Xparams::SetColor(int pixel)
{
    if (gc == xorgc)
        return;
    XSetForeground(display, gc, pixel);
}


// Set the current linestyle.
//
void
Xparams::SetLinestyle(const GRlineType *lineptr)
{
    XGCValues values;
    if (!lineptr || !lineptr->mask || lineptr->mask == -1) {
        values.line_style = LineSolid;
        XChangeGC(display, gc, GCLineStyle, &values);
        return;
    }
    values.line_style = LineOnOffDash;
    XChangeGC(display, gc, GCLineStyle, &values);

    XSetDashes(display, gc, lineptr->offset,
        (char*)lineptr->dashes, lineptr->length);
}


// Create a new pixmap for the fill pattern.
//
void
Xparams::DefineFillpattern(GRfillType *fillp)
{
    if (fillp->xPixmap()) {
        XFreePixmap(display, (Pixmap)fillp->xPixmap());
        fillp->setXpixmap(0);
    }
    if (fillp->hasMap()) {
        unsigned char *map = fillp->newBitmap();
        fillp->setXpixmap((GRfillData)XCreateBitmapFromData(display, window,
            (char*)map, fillp->nX(), fillp->nY()));
        delete [] map;
    }
}


// Set the current fill pattern.
//
void
Xparams::SetFillpattern(const GRfillType *fillp)
{
    if (!fillp || !fillp->xPixmap())
        XSetFillStyle(display, gc, FillSolid);
    else {
        XSetStipple(display, gc, (Pixmap)fillp->xPixmap());
        XSetFillStyle(display, gc, FillStippled);
    }
}


// Update the display.
//
void
Xparams::Update()
{
    XSync(display, 0);
    XFlush(display);
}


// Switch to/from ghost (xor) or highlight/unhighlight drawing context.
// Highlight/unhighlight works only with dual-plane color cells.  It is
// essential to call this with GRxNone after each mode change, due to
// the static storage.
//
void
Xparams::SetXOR(int mode)
{
    static GC tempgc;
    switch (mode) {
    case GRxNone:
        XSetFunction(display, xorgc, GXxor);
        gc = tempgc;
        break;
    case GRxXor:
        tempgc = gc;
        gc = xorgc;
        break;
    case GRxHlite:
        XSetFunction(display, xorgc, GXor);
        tempgc = gc;
        gc = xorgc;
        break;
    case GRxUnhlite:
        XSetFunction(display, xorgc, GXandInverted);
        tempgc = gc;
        gc = xorgc;
        break;
    }
}


void
Xparams::DisplayImage(const GRimage *image, int x, int y,
    int width, int height)
{
    XImage *im = XGetImage(display, window, 0, 0, width, height, 0xffffffff,
        XYPixmap);

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
    XPutImage(display, window, gc, im, 0, 0, x, y, width, height);
    XDestroyImage(im);
}


double
Xparams::Resolution()
{
    return (1.0);
}
// End of Xparams functions


Xdraw::Xdraw(const char *dname, unsigned long win)
{
    xp = 0;
    Display *display = XOpenDisplay(dname);
    if (display) {
        xp = new Xparams(display, win);
        if (x_error) {
            delete xp;
            xp = 0;
        }
    }
    else
        x_error = true;
}


Xdraw::~Xdraw()
{
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
    return (xp->create_pixmap(width, height));
}


bool
Xdraw::destroy_pixmap(unsigned long pixmap)
{
    return (xp->destroy_pixmap((Pixmap)pixmap));
}


bool
Xdraw::copy_drawable(unsigned long dest, unsigned long src, int l, int b,
    int r, int t, int x, int y)
{
    return (xp->copy_drawable((Drawable)dest, (Drawable)src, l, b, r, t,
        x, y));
}


bool
Xdraw::draw(int l, int b, int r, int t)
{
    return (xp->draw(l, b, r, t));
}


bool
Xdraw::get_drawable_size(unsigned long win, int *width, int *height)
{
    return (xp->get_drawable_size(win, width, height));
}


unsigned long
Xdraw::reset_drawable(unsigned long win)
{
    return (xp->reset_drawable((Drawable)win));
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
        xp->defineLinestyle(&xp->linetypes[indx], mask);
}


void
Xdraw::SetLinestyle(int indx)
{
    if (indx >= 0 && indx < XD_NUM_LINESTYLES)
        xp->SetLinestyle(&xp->linetypes[indx]);
}


void
Xdraw::DefineFillpattern(int indx, int nx, int ny, unsigned char *array)
{
    if (indx > 0 && indx < XD_NUM_FILLPATTS && (nx == 8 || nx == 16) &&
            (ny == 8 || ny == 16) && array)
        xp->defineFillpattern(&xp->filltypes[indx], nx, ny, array);
}


void
Xdraw::SetFillpattern(int indx)
{
    if (indx > 0 && indx < XD_NUM_FILLPATTS)
        xp->SetFillpattern(&xp->filltypes[indx]);
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
#ifdef WIN32

namespace ginterf
{
    struct Xparams : public msw_draw
    {
        Xparams() { dev = 0; lcx = 0; }
        virtual ~Xparams() { }

        void ResetViewport(int, int);
        void DefineViewport();
        void Halt();

        Xdev *dev;
        void *lcx;
    };
}


// New viewport function.
//
GRdraw *
Xdev::NewDraw(int)
{
    if (!data)
        return (0);
    Xparams *px = new Xparams;
    px->dev = this;
    px->lcx = GRappIf()->SetupLayers(0, px, 0);
    if (width && height) {
        HDC dcRoot = GetDC(0);
        px->InitDC(dcRoot);
        HDC dcMem = CreateCompatibleDC(dcRoot);
        if (!dcMem) {
            delete px;
            return (0);
        }
        HBITMAP pm = CreateCompatibleBitmap(dcRoot, width, height);
        if (!pm) {
            delete px;
            DeleteDC(dcMem);
            return (0);
        }
        SelectBitmap(dcMem, pm);
        px->SetMemDC(dcMem);
        px->InitDC(dcMem);

        px->SetFillpattern(0);
        px->SetColor(GRappIf()->BackgroundPixel());
        px->Box(0, 0, width, height);
    }
    return (px);
}


// Halt driver, dump image.
//
void
Xparams::Halt()
{
    HDC dcMem = SetMemDC(0);
    if (dcMem) {

        if (dev->data->filename &&
                !strcasecmp(dev->data->filename, "clipboard")) {

            int width = dev->width;
            int height = dev->height;
            unsigned int *ary = new unsigned int[width*height];
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j++) {
                    unsigned int pixel = GetPixel(dcMem, j, i);
                    unsigned int r = GetRValue(pixel);
                    unsigned int g = GetGValue(pixel);
                    unsigned int b = GetBValue(pixel);
                    unsigned int p = r | (g << 8) | (b << 16);
                    ary[(height - 1 - i)*width + j] = p;
                }
            }

            BITMAPINFOHEADER bmh;
            BITMAPINFOHEADER *h = &bmh;
            h->biSize = sizeof(BITMAPINFOHEADER);
            h->biWidth = width;
            h->biHeight = height;
            h->biPlanes = 1;
            h->biBitCount = 32;
            h->biCompression = BI_RGB;
            h->biSizeImage = 0;
            h->biXPelsPerMeter = 0;
            h->biYPelsPerMeter = 0;
            h->biClrUsed = 0;
            h->biClrImportant = 0;

            HBITMAP bitmap = CreateDIBitmap(dcMem, h, CBM_INIT, (void*)ary,
                (BITMAPINFO*)h, DIB_RGB_COLORS);
            delete [] ary;

            if (OpenClipboard(0)) {
                EmptyClipboard();
                SetClipboardData(CF_BITMAP, bitmap);
                CloseClipboard();
                delete this;
                return;
            }
            DeleteBitmap(bitmap);
        }
        else {
            Image *im = create_image_from_drawable(dcMem, 0,
                0, 0, dev->width, dev->height);
            HBITMAP bm = (HBITMAP)GetCurrentObject(dcMem, OBJ_BITMAP);
            HPEN pen = (HPEN)GetCurrentObject(dcMem, OBJ_PEN);
            HBRUSH brush = (HBRUSH)GetCurrentObject(dcMem, OBJ_BRUSH);
            DeleteDC(dcMem);
            DeleteBitmap(bm);
            DeletePen(pen);
            DeleteBrush(brush);
            if (im) {
                ImErrType ret = im->save_image(dev->data->filename, 0);
                if (ret == ImError) {
                    fprintf(stderr,
                        "Image creation failed, internal error.\n");
                    GRpkg::self()->HCabort("Image creation error");
                }
                else if (ret == ImNoSupport) {
                    fprintf(stderr,
                        "Image creation failed, file type unsupported.\n");
                    GRpkg::self()->HCabort("Image format unsupported");
                }
                delete im;
                GRappIf()->SetupLayers(0, this, lcx);
                delete this;
                return;
            }
        }
    }
    GRpkg::self()->HCabort("Internal error");
    GRappIf()->SetupLayers(0, this, lcx);
    delete this;
}


void
Xparams::ResetViewport(int wid, int hei)
{
    if (wid == dev->width && hei == dev->height)
        return;
    if (wid <= 0 || hei <= 0)
        return;
    dev->width = wid;
    dev->data->width = ((double)dev->width)/dev->data->resol;
    dev->height = hei;
    dev->data->height = ((double)dev->height)/dev->data->resol;
}


void
Xparams::DefineViewport()
{
    HDC dcRoot = GetDC(0);
    InitDC(dcRoot);
    HDC dcMem = CreateCompatibleDC(dcRoot);
    if (!dcMem) {
        GRpkg::self()->HCabort("Internal error");
        return;
    }
    HBITMAP pm = CreateCompatibleBitmap(dcRoot, dev->width, dev->height);
    if (!pm) {
        DeleteDC(dcMem);
        GRpkg::self()->HCabort("Internal error");
        return;
    }
    SelectBitmap(dcMem, pm);
    SetMemDC(dcMem);
    InitDC(dcMem);
    SetFillpattern(0);
    SetColor(GRappIf()->BackgroundPixel());
    Box(0, 0, dev->width, dev->height);
}
// End of Xparams functions

#else

GRdraw *
Xdev::NewDraw(int)
{
    return (0);
}

#endif

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
