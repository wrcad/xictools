
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
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "graphics.h"
#include "hcimlib.h"
#include "miscutil/texttf.h"
#ifdef HAVE_MOZY
#include "imsave/imsave.h"
#endif

#ifdef WIN32
#include "mswdraw.h"
#else
#ifdef WITH_X11
#include "x11draw.h"
#endif
#endif
#include <stdio.h>


//
// Hard-copy driver which makes use of the Imlib library for producing
// image files in many different formats.
//

namespace {
    const char *IMresols[] = { "100", "200", 0 };
}

namespace ginterf
{
    // Hard-copy driver - uses imsave to dump to a supported file
    // type.  The file type is determined from the suffix of the given
    // file name.
    //
    HCdesc IMdesc(
        "IM",                // drname
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
            IMresols    // resolutions
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
IMdev::Init(int *ac, char **av)
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
    HCcheckEntries(hd, &IMdesc);

    width = (int)(hd->width * hd->resol);
    height = (int)(hd->height * hd->resol);
    numlinestyles = 16;
    numcolors = 32;

    delete im_data;
    im_data = hd;
    return (false);
}


#ifdef WIN32

namespace ginterf
{
    struct IMparams : public MSWdraw
    {
        IMparams() { pm_dev = 0; pm_lcx = 0; }
        virtual ~IMparams() { }

        void Halt();
        void ResetViewport(int, int);
        void DefineViewport();

        void set_dev(IMdev *d)      { pm_dev = d; }
        void set_lcx(void *p)       { pm_lcx = p; }

private:
        IMdev *pm_dev;
        void *pm_lcx;
    };
}


// New viewport function.
//
GRdraw *
IMdev::NewDraw(int)
{
    if (!im_data)
        return (0);
    IMparams *px = new IMparams();
    px->set_dev(this);
    px->set_lcx(GRappIf()->SetupLayers(0, px, 0));
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
IMparams::Halt()
{
    HDC dcMem = SetMemDC(0);
    if (dcMem) {
        if (pm_dev->data()->filename &&
                !strcasecmp(pm_dev->data()->filename, "clipboard")) {

            int width = pm_dev->width;
            int height = pm_dev->height;
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
                0, 0, pm_dev->width, pm_dev->height);
            HBITMAP bm = (HBITMAP)GetCurrentObject(dcMem, OBJ_BITMAP);
            HPEN pen = (HPEN)GetCurrentObject(dcMem, OBJ_PEN);
            HBRUSH brush = (HBRUSH)GetCurrentObject(dcMem, OBJ_BRUSH);
            DeleteDC(dcMem);
            DeleteBitmap(bm);
            DeletePen(pen);
            DeleteBrush(brush);
            if (im) {
                ImErrType ret = im->save_image(pm_dev->data()->filename, 0);
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
                GRappIf()->SetupLayers(0, this, pm_lcx);
                delete this;
                return;
            }
        }
    }
    GRpkg::self()->HCabort("Internal error");
    GRappIf()->SetupLayers(0, this, pm_lcx);
    delete this;
}


void
IMparams::ResetViewport(int wid, int hei)
{
    if (wid == pm_dev->width && hei == pm_dev->height)
        return;
    if (wid <= 0 || hei <= 0)
        return;
    pm_dev->width = wid;
    pm_dev->data()->width = ((double)pm_dev->width)/pm_dev->data()->resol;
    pm_dev->height = hei;
    pm_dev->data()->height = ((double)pm_dev->height)/pm_dev->data()->resol;
}


void
IMparams::DefineViewport()
{
    HDC dcRoot = GetDC(0);
    InitDC(dcRoot);
    HDC dcMem = CreateCompatibleDC(dcRoot);
    if (!dcMem) {
        GRpkg::self()->HCabort("Internal error");
        return;
    }
    HBITMAP pm = CreateCompatibleBitmap(dcRoot, pm_dev->width, pm_dev->height);
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
    Box(0, 0, pm_dev->width, pm_dev->height);
}
// End of IMparams functions


#else
#ifdef WITH_X11

namespace ginterf
{
    struct IMparams : public X11draw
    {
        IMparams(Display*, Window);
        virtual ~IMparams();

        void Halt();
        void ResetViewport(int, int);
        void DefineViewport();

        void set_dev(IMdev *d)      { pm_dev = d; }
        void set_lcx(void *p)       { pm_lcx = p; }

private:
        IMdev       *pm_dev;
        void        *pm_lcx;
        GRlineType  pm_linetypes[XD_NUM_LINESTYLES];
        GRfillType  pm_filltypes[XD_NUM_FILLPATTS];
    };
}

// New viewport function
//
GRdraw *
IMdev::NewDraw(int)
{
    if (!im_data)
        return (0);

    const char *dname = getenv("DISPLAY");
    if (!dname)
        dname = ":0";
    Display *display = XOpenDisplay(dname);
    if (!display)
        return (0);
    Window window = RootWindow(display, DefaultScreen(display));

    IMparams *px = new IMparams(display, window);
    px->set_dev(this);

    // This will save current and swap in the appropriate new pixmap
    // cached in layer descs.   *** WE CAN'T USE THEM with application's
    // graphics until we reset (in Halt).
    px->set_lcx(GRappIf()->SetupLayers(display, px, 0));

    if (width && height) {
        Pixmap pm = XCreatePixmap(display, px->window(), width, height,
            DefaultDepth(display, DefaultScreen(display)));
        if (!pm) {
            delete px;
            return (0);
        }
        px->set_window(pm);
        px->SetFillpattern(0);
        px->SetColor(GRappIf()->BackgroundPixel());
        px->Box(0, 0, width, height);
    }
    return (px);
}
// End of IMdev functions


IMparams::IMparams(Display *d, Window w) : X11draw(d, w)
{
    pm_dev = 0;
    pm_lcx = 0;
    defineLinestyle(&pm_linetypes[1], 0xa);
    defineLinestyle(&pm_linetypes[2], 0xc);
    defineLinestyle(&pm_linetypes[3], 0x924);
    defineLinestyle(&pm_linetypes[4], 0xe38);
    defineLinestyle(&pm_linetypes[5], 0x8);
    defineLinestyle(&pm_linetypes[6], 0xc3f0);
    defineLinestyle(&pm_linetypes[7], 0xfe03f80);
    defineLinestyle(&pm_linetypes[8], 0x1040);
    defineLinestyle(&pm_linetypes[9], 0xe3f8);
    defineLinestyle(&pm_linetypes[10], 0x3c48);
}


IMparams::~IMparams()
{
    for (int i = 0; i < XD_NUM_FILLPATTS; i++) {
        if (pm_filltypes[i].xPixmap())
            XFreePixmap(display(), (Pixmap)pm_filltypes[i].xPixmap());
    }
}


// Halt driver, dump image.
//
void
IMparams::Halt()
{
#ifdef HAVE_MOZY
    Image *im = create_image_from_drawable(display(), window(), 0, 0,
        pm_dev->width, pm_dev->height);
    XFreePixmap(display(), window());
    if (im) {
        ImErrType ret = im->save_image(pm_dev->data()->filename, 0);
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
    XFreePixmap(display(), window());
    fprintf(stderr, "Image creation failed, package not available.\n");
    GRpkg::self()->HCabort("Image format unsupported");
#endif

    // Put back original pixmaps in layer descs.
    GRappIf()->SetupLayers(display(), this, pm_lcx);

    pm_lcx = 0;
    delete this;
}


void
IMparams::ResetViewport(int wid, int hei)
{
    if (wid == pm_dev->width && hei == pm_dev->height)
        return;
    if (wid <= 0 || hei <= 0)
        return;
    pm_dev->width = wid;
    pm_dev->data()->width = ((double)pm_dev->width)/pm_dev->data()->resol;
    pm_dev->height = hei;
    pm_dev->data()->height = ((double)pm_dev->height)/pm_dev->data()->resol;
}


void
IMparams::DefineViewport()
{
    Pixmap pm = XCreatePixmap(display(), window(), pm_dev->width,
        pm_dev->height, DefaultDepth(display(), DefaultScreen(display())));
    if (!pm) {
        GRpkg::self()->HCabort("Internal error");
        return;
    }
    set_window(pm);
    SetFillpattern(0);
    SetColor(GRappIf()->BackgroundPixel());
    Box(0, 0, pm_dev->width, pm_dev->height);
}

// End of IMparams functions


#else
#endif
#endif
