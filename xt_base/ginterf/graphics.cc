
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

#ifndef _GNU_SOURCE
// For vasprintf on RH Linux 7.2
#define _GNU_SOURCE
#endif

#include "config.h"
#include <errno.h>
#include <stdarg.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef WIN32
#include <windows.h>
#include <process.h>
#include <libiberty.h>
#endif
#include "graphics.h"
#include "lstring.h"
#include "filestat.h"
#ifdef HAVE_MOZY
#include "../../mozy/include/imsave.h"
#endif

#include "nulldev.h"
#include "xdraw.h"
#include "pslindrw.h"
#include "psbm.h"
#include "psbc.h"
#include "hpgl.h"
#include "pcl.h"
#include "xfig.h"


bool GRmultiPt::xshort;

bool GRwbag::no_ask_file_action = false;

// Default print command (printer name in Win32).
#ifdef WIN32
const char *HCdefaults::default_print_cmd = "default";
#else
const char *HCdefaults::default_print_cmd = "lpr";
#endif

//-----------------------------------------------------------------------------
// GRappCalls functions

GRappCalls *GRappCalls::instancePtr = 0;

GRappCalls::GRappCalls()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class GRappCalls already instantiated.\n");
        exit(1);
    }
    instancePtr = this;
}


// Private static error exit.
//
void
GRappCalls::on_null_ptr()
{
    fprintf(stderr, "Singleton class GRappCalls used before instantiated.\n");
    exit(1);
}


//-----------------------------------------------------------------------------
// GRimage functions

bool
GRimage::create_image_file(GRpkg *pkg, const char *filename)
{
    if (!im_width || !im_height || !im_data)
        return (false);
    if (!pkg || !filename)
        return (false);

#ifdef HAVE_MOZY
    unsigned char *rgbdata = new unsigned char[im_width*im_height * 3];
    Image img(im_width, im_height, rgbdata);
    // rgbdata freed im Image destructor.

    for (unsigned int i = 0; i < im_height; i++) {
        for (unsigned int j = 0; j < im_width; j++) {
            int r, g, b;
            pkg->RGBofPixel(im_data[j + i*im_width], &r, &g, &b);
            *rgbdata++ = r;
            *rgbdata++ = g;
            *rgbdata++ = b;
        }
    }
    SaveInfo sv;
    return (img.save_image(filename, &sv) == ImOK);
#else
    return (false);
#endif
}


//-----------------------------------------------------------------------------
// GRpkg functions

#define MAX_DRVRS 20

// Default "local selection" color.  This is used as a background for
// certain selections that are different from the widget's default.
//
#define GRattrColorLocSelDef    "#e1e1ff"

GRpkg *GRpkg::instancePtr = 0;

GRpkg::GRpkg()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class GRpkg already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    pkg_cur_dev = 0;
    pkg_main_dev = 0;
    pkg_hc_descs = new HCdesc*[MAX_DRVRS];
    memset(pkg_hc_descs, 0, MAX_DRVRS*sizeof(HCdesc*));

    pkg_last_dev = 0;
    pkg_devices = new GRdev*[MAX_DRVRS];
    memset(pkg_devices, 0, MAX_DRVRS*sizeof(GRdev*));
    pkg_hc_desc_count = 0;
    pkg_device_count = 0;
    pkg_max_count = MAX_DRVRS;
    pkg_abort = false;
    pkg_abort_msg = 0;

    pkg_error_callback = 0;

    pkg_main_wbag = 0;

    memset(pkg_colors, 0, sizeof(pkg_colors));
    pkg_colors[GRattrColorLocSel] = lstring::copy(GRattrColorLocSelDef);
}


// Private static error exit.
//
void
GRpkg::on_null_ptr()
{
    fprintf(stderr, "Singleton class GRpkg used before instantiated.\n");
    exit(1);
}


// The GRpkg::InitPkg method is instantiated in gr_pkg_setup.h, which
// must be included in the application.


// Shut down the screen graphics and establist the NULL package.
// Returns false on success.
//
bool
GRpkg::SetNullGraphics()
{
    ResetMain(new NULLdev);
    if (pkg_devices[0]->Init(0, 0))
        return (true);
    pkg_cur_dev = pkg_devices[0];
    pkg_main_dev = dynamic_cast<GRscreenDev*>(pkg_cur_dev);
    return (false);
}


// If given name of a hardcopy device, finds it and switches devices
// if given 0, switches back.
//
HCswitchErr
GRpkg::SwitchDev(const char *devname, int *argc, char **argv)
{
    if (devname != 0) {
        if (pkg_last_dev != 0) {
            // Dev switch w/o changing back.
            return (HCSinhc);
        }
        pkg_last_dev = pkg_cur_dev;
        pkg_cur_dev = FindDev(devname);
        if (pkg_cur_dev == 0) {
            // No hardcopy device found, revert.
            pkg_cur_dev = pkg_last_dev;
            pkg_last_dev = 0;
            return (HCSnotfnd);
        }
        if (pkg_cur_dev->Init(argc, argv))
            // Init failed.
            return (HCSinit);
    }
    else {
        pkg_cur_dev = pkg_last_dev;
        pkg_last_dev = 0;
    }
    return (HCSok);
}


// Find a package by name, and return a (GRdispDev*) pointer it.
//
GRdev *
GRpkg::FindDev(const char *name)
{
    for (int i = 0; pkg_devices[i]; i++) {
        if (!pkg_devices[i]->name)
            continue;
        if (!strcmp(name, pkg_devices[i]->name))
            return (pkg_devices[i]);
    }
    ErrPrintf(ET_INTERR, "can't find device %s.\n", name);
    return (0);
}


// Find the hard copy driver by name (case insensitive).
//
HCdesc *
GRpkg::FindHCdesc(const char *name)
{
    HCdesc *hcdesc;
    for (int i = 0; (hcdesc = HCof(i)); i++) {
        if (lstring::cieq(name, hcdesc->keyword))
            return (hcdesc);
        if (hcdesc->alias && lstring::cieq(name, hcdesc->alias))
            return (hcdesc);
    }
    return (0);
}


// Find the hard copy driver by name (case insensitive), return its
// index.
//
int
GRpkg::FindHCindex(const char *name)
{
    HCdesc *hcdesc;
    for (int i = 0; (hcdesc = HCof(i)); i++) {
        if (lstring::cieq(name, hcdesc->keyword))
            return (i);
        if (hcdesc->alias && lstring::cieq(name, hcdesc->alias))
            return (i);
    }
    return (-1);
}


#ifdef WIN32

namespace {
    long thread1;
    bool thread_got1;
    int  thread_c1;

    void __cdecl thcb1(void *arg)
    {
        thread_got1 = false;
        thread_c1 = GRpkgIf()->GetChar((int)arg);
        thread_got1 = true;
        thread1 = 0;
    }
}

#endif


int
GRpkg::Input(int fd, int socket, int *keyret)
{
    if (pkg_main_dev)
        return (pkg_main_dev->Input(fd, socket, keyret));

#ifdef WIN32

    for (;;) {
        if (socket > 0) {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(socket, &readfds);

            timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 50000;

            int i = select(1, &readfds, 0, 0, &timeout);
            if (i > 0) {
                char c;
                recv(socket, &c, 1, 0);
                *keyret = c;
                return (socket);
            }
        }

        if (fd >= 0 && thread_got1) {
            *keyret = thread_c1;
            thread_got1 = false;
            return (fd);
        }
        if (fd >= 0 && !thread1)
            thread1 = _beginthread(thcb1, 0, (void*)fd);
        SleepEx(0, true);
    }

#else

    for (;;) {
        int nfds = 0;
        if (fd > nfds)
            nfds = fd;
        if (socket > nfds)
            nfds = socket;
        fd_set readfds;
        FD_ZERO(&readfds);
        if (fd >= 0)
            FD_SET(fd, &readfds);
        if (socket >= 0)
            FD_SET(socket, &readfds);

        int i = select(nfds + 1, &readfds, 0, 0, 0);
        if (i > 0) {
            if (fd >= 0 && FD_ISSET(fd, &readfds)) {
                *keyret = GetChar(fd);
                return (fd);
            }
            else if (socket >= 0 && FD_ISSET(socket, &readfds)) {
                *keyret = GetChar(socket);
                return (socket);
            }
        }
    }

#endif

    return (-1);
}


// Special getc() to handle ^D cleanly.
//
int
GRpkg::GetChar(int fd)
{
    char c;
    int i;
    do {
        i = read(fd, &c, 1);
    } while (i == -1 && errno == EINTR);

    if (i == 0)
        return (EOF);
    else if (i == -1) {
        Perror("read");
        return (EOF);
    }
    return ((int) c);
}


// Replacement for perror() which supports diversion to error window.
//
void
GRpkg::Perror(const char *str)
{
    if (str && *str) {
#ifdef WIN32
        char buf[256];
        if (!strcmp(str, "send") || !strcmp(str, "recv") ||
                !strcmp(str, "socket") || !strcmp(str, "connect") ||
                !strcmp(str, "accept") || !strcmp(str, "bind") ||
                !strcmp(str, "listen") || !strcmp(str, "select") ||
                lstring::prefix("gethost", str)) {
            sprintf(buf, "%s: WSA error code %d.\n", str, WSAGetLastError());
            ErrPrintf(ET_MSG, "%s\n", buf);
            return;
        }
#endif
#ifdef HAVE_STRERROR
        ErrPrintf(ET_MSG, "%s: %s\n", str, strerror(errno));
    }
    else
        ErrPrintf(ET_MSG, "%s\n", strerror(errno));
#else
        ErrPrintf(ET_MSG, "%s: %s\n", str, sys_errlist[errno]);
    }
    else
        ErrPrintf(ET_MSG, "%s\n", sys_errlist[errno]);
#endif
}


// Register a callback to receive the error messages.
//
void
GRpkg::RegisterErrorCallback(GRerrorCallback cb)
{
    pkg_error_callback = cb;
}


#define GR_ERR_BUFSIZE 8192

// General error printing function, uses a window if possible.
//
void
GRpkg::ErrPrintf(Etype type, const char *fmt, ...)
{
#ifdef HAVE_VASPRINTF
    // This is provided by libiberty in MINGW.
    va_list args;
    const char *pfx = 0;
    bool to_stdout = false;
    switch (type) {
    default:
    case ET_MSGS:
        to_stdout = true;
        // fallthrough
    case ET_MSG:
        break;
    case ET_WARNS:
        to_stdout = true;
        // fallthrough
    case ET_WARN:
        pfx = "Warning: ";
        break;
    case ET_ERRORS:
        to_stdout = true;
        // fallthrough
    case ET_ERROR:
        pfx = "Error: ";
        break;
    case ET_INTERRS:
        to_stdout = true;
        // fallthrough
    case ET_INTERR:
        pfx = "Internal error: ";
        break;
    }
    va_start(args, fmt);
    char *outstr;
    if (vasprintf(&outstr, fmt, args) < 0)
        return;
    if (pfx && !lstring::prefix(pfx, outstr)) {
        char *tstr = (char*)malloc(strlen(pfx) + strlen(outstr) + 1);
        strcpy(tstr, pfx);
        strcat(tstr, outstr);
        free(outstr);
        outstr = tstr;
    }

    if (pkg_error_callback)
        (*pkg_error_callback)(type, to_stdout, outstr);
    else {
        if (!to_stdout && GRpkgIf()->MainWbag())
            GRpkgIf()->MainWbag()->PopUpErr(MODE_ON, outstr);
        else
            fputs(outstr, stderr);
    }
    free(outstr);

#else

    va_list args;
    char buf[GR_ERR_BUFSIZE];
    *buf = '\0';
    bool to_stdout = false;
    switch (type) {
    default:
    case ET_MSGS:
        to_stdout = true;
    case ET_MSG:
        break;
    case ET_WARNS:
        to_stdout = true;
    case ET_WARN:
        strcpy(buf, "Warning: ");
        break;
    case ET_ERRORS:
        to_stdout = true;
    case ET_ERROR:
        strcpy(buf, "Error: ");
        break;
    case ET_INTERRS:
        to_stdout = true;
    case ET_INTERR:
        strcpy(buf, "Internal error: ");
        break;
    }
    va_start(args, fmt);
    vsnprintf(buf + strlen(buf), GR_ERR_BUFSIZE - strlen(buf), fmt, args);
    va_end(args);

    if (pkg_error_callback)
        (*pkg_error_callback)(type, to_stdout, buf);
    else {
        if (!to_stdout && GRpkgIf()->MainWbag())
            GRpkgIf()->MainWbag()->PopUpErr(MODE_ON, buf);
        else
            fputs(buf, stderr);
    }
#endif
}


void
GRpkg::SetAttrColor(GRattrColor c, const char *string)
{
    if (c == GRattrColorLocSel) {
        char *s = lstring::copy(string);
        delete [] pkg_colors[GRattrColorLocSel];
        if (s)
            pkg_colors[GRattrColorLocSel] = s;
        else
            pkg_colors[GRattrColorLocSel] =
                lstring::copy(GRattrColorLocSelDef);
    }
    else if (c >= 0 && c < GRattrColorEnd) {
        char *s = lstring::copy(string);
        delete [] pkg_colors[c];
        pkg_colors[c] = s;
    }
}


const char *
GRpkg::GetAttrColor(GRattrColor c)
{
    const char *s = "black";
    if (c >= 0 && c < GRattrColorEnd) {
        if (pkg_colors[c])
            s = pkg_colors[c];
    }
    return (s);
}


// private functions

// Register a device for use
//
void
GRpkg::RegisterDevice(GRdev *dev)
{
    if (pkg_device_count < pkg_max_count) {
        if (dev)
            pkg_devices[pkg_device_count++] = dev;
    }
    else
        fprintf(stderr, "Driver reference not added: too many registered.\n");
}


// Register a hard copy descriptor for reference
//
void
GRpkg::RegisterHcopyDesc(HCdesc *desc)
{
    if (pkg_hc_desc_count < pkg_max_count) {
        if (desc)
            pkg_hc_descs[pkg_hc_desc_count++] = desc;
    }
    else
        fprintf(stderr, "Driver not added: too many registered.\n");
}


//-----------------------------------------------------------------------------
// GRdev functions

namespace {
    void
    shift(int i, int *acp, char **av)
    {
        (*acp)--;
        while (i < *acp) {
            av[i] = av[i+1];
            i++;
        }
    }
}


// Called by the hardcopy drivers to process args to Init(), false
// is returned if the parse succeeds.
//
bool
GRdev::HCdevParse(HCdata *hd, int *acp, char **av)
{
    for (int i = 0; i < *acp; ) {
        if (!av[i])
            return (true);
        if (!strcmp(av[i], "-f")) {
            shift(i, acp, av);
            if (i >= *acp || !av[i] || !*av[i])
                return (true);
            hd->filename = lstring::copy(av[i]);
            shift(i, acp, av);
            continue;
        }
        if (!strcmp(av[i], "-r")) {
            shift(i, acp, av);
            if (i >= *acp || !av[i] || !*av[i])
                return (true);
            hd->resol = atoi(av[i]);
            shift(i, acp, av);
            continue;
        }
        if (!strcmp(av[i], "-t")) {
            shift(i, acp, av);
            if (i >= *acp || !av[i] || !*av[i])
                return (true);
            hd->hctype = *av[i];
            shift(i, acp, av);
            continue;
        }
        if (!strcmp(av[i], "-w")) {
            shift(i, acp, av);
            if (i >= *acp || !av[i] || !*av[i])
                return (true);
            hd->width = atof(av[i]);
            shift(i, acp, av);
            continue;
        }
        if (!strcmp(av[i], "-h")) {
            shift(i, acp, av);
            if (i >= *acp || !av[i] || !*av[i])
                return (true);
            hd->height = atof(av[i]);
            shift(i, acp, av);
            continue;
        }
        if (!strcmp(av[i], "-x")) {
            shift(i, acp, av);
            if (i >= *acp || !av[i] || !*av[i])
                return (true);
            hd->xoff = atof(av[i]);
            shift(i, acp, av);
            continue;
        }
        if (!strcmp(av[i], "-y")) {
            shift(i, acp, av);
            if (i >= *acp || !av[i] || !*av[i])
                return (true);
            hd->yoff = atof(av[i]);
            shift(i, acp, av);
            continue;
        }
        if (!strcmp(av[i], "-e")) {
            shift(i, acp, av);
            hd->encode = true;
            continue;
        }
        if (!strcmp(av[i], "-l")) {
            shift(i, acp, av);
            hd->landscape = true;
            continue;
        }
        i++;
    }
    return (false);
}


// Once the string has been parsed by the driver init routine, the
// appropriate descriptor can be determined.  This routine checks the
// entries against the limits.
//
void
GRdev::HCcheckEntries(HCdata *hd, HCdesc *desc)
{
    if (!(desc->limits.flags & HCdontCareWidth)) {
        if (hd->width != 0 || (desc->limits.flags & HCnoAutoWid)) {
            if (hd->width < desc->limits.minwidth)
                hd->width = desc->limits.minwidth;
            if (hd->width > desc->limits.maxwidth)
                hd->width = desc->limits.maxwidth;
        }
    }
    else
        hd->width = 0.0;
    if (!(desc->limits.flags & HCdontCareHeight)) {
        if (hd->height != 0 || (desc->limits.flags & HCnoAutoHei)) {
            if (hd->height < desc->limits.minheight)
                hd->height = desc->limits.minheight;
            if (hd->height > desc->limits.maxheight)
                hd->height = desc->limits.maxheight;
        }
    }
    else
        hd->height = 0.0;
    if (!(desc->limits.flags & HCdontCareXoff)) {
        if (hd->xoff < desc->limits.minxoff)
            hd->xoff = desc->limits.minxoff;
        if (hd->xoff > desc->limits.maxxoff)
            hd->xoff = desc->limits.maxxoff;
    }
    else
        hd->xoff = 0.0;
    if (!(desc->limits.flags & HCdontCareYoff)) {
        if (hd->yoff < desc->limits.minyoff)
            hd->yoff = desc->limits.minyoff;
        if (hd->yoff > desc->limits.maxyoff)
            hd->yoff = desc->limits.maxyoff;
    }
    else
        hd->yoff = 0.0;
    if (desc->limits.resols) {
        int i;
        for (i = 0; desc->limits.resols[i]; i++)
            if (hd->resol == atoi(desc->limits.resols[i]))
                break;
        if (!desc->limits.resols[i])
            hd->resol = atoi(desc->limits.resols[desc->defaults.defresol]);
    }
}


#define CODEXMIN 1
#define CODEYMIN 2
#define CODEXMAX 4
#define CODEYMAX 8

#define CODE(x, y, c) \
    c = 0;\
    if (x < xmin) c = CODEXMIN; \
    else if (x > xmax) c = CODEXMAX; \
    if (y < ymin) c |= CODEYMIN; \
    else if (y > ymax) c |= CODEYMAX;

// This clips to the rectangle (0, 0, width-1, height-1).  The
// returned value is true if the line is out of the AOI (therefore
// does not need to be displayed) and false if the line is in the AOI.
//
bool
GRdev::LineClip(int *px1, int *py1, int *px2, int *py2)
{
    int x1 = *px1;
    int y1 = *py1;
    int x2 = *px2;
    int y2 = *py2;
    int x=0, y=0;
    int c, c1, c2;

    int xmin = 0;
    int xmax = width - 1;
    int ymin = 0;
    int ymax = height - 1;

    CODE(x1, y1, c1)
    CODE(x2, y2, c2)
    while (c1 || c2) {
        if (c1 & c2)
            return (true); // Line is invisible
        if ((c = c1) == 0)
            c = c2;
        if (c & CODEXMIN) {
            y = y1 + (y2 - y1)*(xmin - x1)/(x2 - x1);
            x = xmin;
        }
        else if (c & CODEXMAX) {
            y = y1 + (y2 - y1)*(xmax - x1)/(x2 - x1);
            x = xmax;
        }
        else if (c & CODEYMIN) {
            x = x1 + (x2 - x1)*(ymin - y1)/(y2 - y1);
            y = ymin;
        }
        else if (c & CODEYMAX) {
            x = x1 + (x2 - x1)*(ymax - y1)/(y2 - y1);
            y = ymax;
        }
        if (c == c1) {
            x1 = x;
            y1 = y;
            CODE(x, y, c1)
        }
        else {
            x2 = x;
            y2 = y;
            CODE(x, y, c2)
        }
    }
    *px1 = x1;
    *py1 = y1;
    *px2 = x2;
    *py2 = y2;
    return (false); // Line is at least partially visible.
}


//-----------------------------------------------------------------------------
// GRdraw functions

namespace {
    // predefined line styles
    int GRdefLinestyles[] =
    {
        0xa,        // dotted
        0xc,        // short dashes
        0x924,      // spaced dots
        0xe38,      // medium dashes
        0x8,        // spaced dots (wider)
        0xc3f0,     // short/long dashes
        0xfe03f80,  // long dashes
        0x1040,     // dot/dash
        0xe3f8,     // medium/long dashes
        0x3c48,     // dash dot dot
    };
}
#define GRnumDefLinestyles 10


void
GRdraw::defineLinestyle(GRlineType *lineptr, int mask)
{
    // The dash list has 8 bytes storage.
    {
        GRdraw *gdt = this;
        if (!gdt)
            return;
    }
    if (!lineptr)
        return;
    lineptr->mask = mask;
    lineptr->length = 0;
    if (!mask || mask == -1) {
        SetLinestyle(0);
        return;
    }
    unsigned imask = ~((~(unsigned)0) >> 1);
    while (!(imask & mask))
        imask >>= 1;
    unsigned char *dash = lineptr->dashes;
    for (int i = 0; i < 8; i++)
        dash[i] = 0;
    int n = 0;
    for (;;) {
        while (imask && (imask & mask)) {
            dash[n]++;
            imask >>= 1;
        }
        n++;
        if (!imask || n == 8) break;
        while (imask && !(imask & mask)) {
            dash[n]++;
            imask >>= 1;
        }
        n++;
        if (!imask || n == 8) break;
    }
    if (n > 1) {
        if (n & 1) {
            // odd length, wrap last segment into first
            n--;
            dash[0] += dash[n];
            dash[n] = 0;
        }
        lineptr->offset = 0;
        lineptr->length = n;
        DefineLinestyle(lineptr);
        SetLinestyle(lineptr);
    }
    else {
        // just a string of 1's, solid
        lineptr->mask = -1;
        SetLinestyle(0);
    }
}


void
GRdraw::setDefaultLinestyle(int index)
{
    static GRlineType lt;
    {
        GRdraw *gdt = this;
        if (!gdt)
            return;
    }
    if (!index)
        SetLinestyle(0);
    else {
        if (index > GRnumDefLinestyles)
            index = (index % GRnumDefLinestyles) + 1;
        index--;
        defineLinestyle(&lt, GRdefLinestyles[index]);
    }
}


void
GRdraw::defineFillpattern(GRfillType *fillp, int nx, int ny,
    const unsigned char *array)
{
    {
        GRdraw *gdt = this;
        if (!gdt)
            return;
    }
    if (!fillp)
        return;
    fillp->newMap(nx, ny, array);
    DefineFillpattern(fillp);
    SetFillpattern(fillp->hasMap() ? fillp : 0);
}

