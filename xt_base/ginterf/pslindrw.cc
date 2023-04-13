
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

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "pslindrw.h"
#include "miscutil/texttf.h"


#ifndef M_PI
#define	M_PI  3.14159265358979323846  // pi
#endif

#define SOLID (GRlineType*)0

// font height in points=1/72 inch
#define FONTPTS 10

#define PS_BW '0'
#define PS_COLOR '1'

namespace {
    // Limits and specified parameters
    const char *PSresols[] =
        {"72", "75", "100", "150", "200", "300", "400", 0};
}

namespace ginterf
{
    HCdesc PSdesc_m(
        "PS",                           // drname
        "PostScript line draw, mono",   // descr
        "postscript_line_draw",         // keyword
        "psld",                         // alias
        "-f %s -r %d -t 0 -w %f -h %f -x %f -y %f", // format
        HClimits(
            0.0, 22.0,  // minxoff, maxxoff
            0.0, 22.0,  // minyoff, maxyoff
            1.0, 23.5,  // minwidth, maxwidth
            1.0, 23.5,  // minheight, maxheight
            0,          // flags
            PSresols    // resols
        ),
        HCdefaults(
            0.25, 0.25, // defxoff, defyoff
            7.8, 10.3,  // defwidth, defheight
            0,          // command string to print file
            0,          // defresol (index into resols)
            HClegOn,    // initially use legend
            HCbest      // initially set best orientation
        ),
        true, true);    // line_draw, line width can be set

    HCdesc PSdesc_c(
        "PS",                           // drname
        "PostScript line draw, color",  // descr
        "postscript_line_draw_color",   // keyword
        "psldc",                        // alias
        "-f %s -r %d -t 1 -w %f -h %f -x %f -y %f", // format
        HClimits(
            0.0, 22.0,  // minxoff, maxxoff
            0.0, 22.0,  // minyoff, maxyoff
            1.0, 23.5,  // minwidth, maxwidth
            1.0, 23.5,  // minheight, maxheight
            0,          // flags
            PSresols    // resols
        ),
        HCdefaults(
            0.25, 0.25, // defxoff, defyoff
            7.8, 10.3,  // defwidth, defheight
            0,          // command string to print file
            0,          // defresol (index into resols)
            HClegOn,    // initially use legend
            HCbest      // initially set best orientation
        ),
        true, true);    // line_draw, line width can be set
}


bool
PSdev::Init(int *ac, char **av)
{
    if (!ac || !av)
        return (true);
    HCdata *hd = new HCdata;
    hd->filename = 0;
    hd->width = 8.0;
    hd->height = 10.5;
    hd->hctype = PS_BW;
    hd->resol = 144;

    if (HCdevParse(hd, ac, av)) {
        delete hd;
        return (true);
    }

    HCdesc *desc;
    if (hd->hctype == PS_BW)
        desc = &PSdesc_m;
    else if (hd->hctype == PS_COLOR)
        desc = &PSdesc_c;
    else {
        delete hd;
        return (true);
    }
    if (!hd->filename || !*hd->filename) {
        delete hd;
        return (true);
    }
    HCcheckEntries(hd, desc);

    delete data;
    data = hd;
    numlinestyles = 0;
    if (hd->hctype == PS_BW)
        numcolors = 2;
    else
        numcolors = 32;
    return (false);
}


GRdraw *
PSdev::NewDraw(int)
{
    FILE *plotfp;
    if (data->filename && *data->filename)
        plotfp = fopen(data->filename, "wb");
    else
        return (0);
    if (!plotfp) {
        GRpkgIf()->Perror(data->filename);
        return (0);
    }
    PSparams *ps = new PSparams;
    ps->fp = plotfp;
    ps->dev = this;

    ps->linestyle = SOLID;
    ps->strokecnt = 0;
    ps->lastx = -1;
    ps->lasty = -1;
    ps->curfpat = -1;
    if (data->hctype == PS_COLOR)
        ps->usecolor = true;

    // In auto-height, the application will rotate the cell.  The legend
    // is always on the bottom.  Nothing to do here
    if (data->height == 0)
        data->landscape = false;

    // drawable area
    if (data->landscape) {
        height = (int)(data->resol*data->width);
        width = (int)(data->resol*data->height);
        xoff = (int)(data->resol*data->yoff);
        yoff = (int)(data->resol*data->xoff);
    }
    else {
        width = (int)(data->resol*data->width);
        height = (int)(data->resol*data->height);
        xoff = (int)(data->resol*data->xoff);
        yoff = (int)(data->resol*data->yoff);
    }
    return (ps);
}


void
PSparams::Halt()
{
    fprintf(fp, "stroke\n");
    fprintf(fp, "showpage\n");
    fclose(fp);
    delete this;
}


void
PSparams::ResetViewport(int wid, int hei)
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


// Output the initializing lines of the file.  This has to be done after
// the viewport width/height are known, ahead of any geometry.
//
void
PSparams::DefineViewport()
{
    time_t secs;
    time(&secs);
    tm *t = localtime(&secs);
    fprintf(fp, "%%!PS-Adobe-3.0 EPSF-3.0\n");

#ifndef PS_DEBUG
    fprintf(fp, "%%%%BoundingBox: %d %d %d %d\n",
        (int)(72*dev->data->xoff)-1,
        (int)(72*dev->data->yoff)-1,
        (int)(72*(dev->data->xoff + dev->data->width))+1,
        (int)(72*(dev->data->yoff + dev->data->height))+1);
#endif

    fprintf(fp, "%%!Xic-postsc %02d-%02d-%02d %02d:%02d\n", t->tm_mon+1,
        t->tm_mday, t->tm_year - 100, t->tm_hour, t->tm_min);
    fprintf(fp, "/Helvetica findfont %g scalefont setfont\n",
        FONTPTS*dev->data->resol/72.0);

    // Elliptical arc rendering from PostScript Language Tutorial and
    // Cookbook, Adobe Inc.
    //
    fprintf(fp, "/ellipsedict 8 dict def\n");
    fprintf(fp, "ellipsedict /mtrx matrix put\n");
    fprintf(fp, "/ellipse\n");
    fprintf(fp, "{ ellipsedict begin\n");
    fprintf(fp, "/endangle exch def\n");
    fprintf(fp, "/startangle exch def\n");
    fprintf(fp, "/yrad exch def\n");
    fprintf(fp, "/xrad exch def\n");
    fprintf(fp, "/y exch def\n");
    fprintf(fp, "/x exch def\n");
    fprintf(fp, "/savematrix mtrx currentmatrix def\n");
    fprintf(fp, "x y translate\n");
    fprintf(fp, "xrad yrad scale\n");
    fprintf(fp, "0 0 1 startangle endangle arc\n");
    fprintf(fp, "savematrix setmatrix\n");
    fprintf(fp, "end\n");
    fprintf(fp, "} def\n");

#ifdef PS_DEBUG
    int ww = (int)(8.5*72);
    int hh = 11*72;
    fprintf(fp, "%d %d moveto\n", 0, 0);
    fprintf(fp, "%d %d lineto\n", 0, hh);
    fprintf(fp, "%d %d lineto\n", ww, hh);
    fprintf(fp, "%d %d lineto\n", ww, 0);
    fprintf(fp, "%d %d lineto\n", 0, 0);
#endif

    if (dev->data->linewidth > 0.0) {
        fprintf(fp, "%g setlinewidth\n",
            dev->data->linewidth*dev->data->resol/72.0);
        // round ends
        fprintf(fp, "1 setlinecap\n");
        fprintf(fp, "1 setlinejoin\n");
    }
    // scale to native coordinates
    fprintf(fp, "%g %g scale\n", 72.0/dev->data->resol, 72.0/dev->data->resol);

    if (dev->data->landscape) {
        fprintf(fp, "%d %d translate\n", 2*dev->yoff + dev->height, 0);
        fprintf(fp, "90 rotate\n");
    }

    fprintf(fp, "newpath\n");
    fprintf(fp, "[] 0 setdash\n");
}

void
PSparams::Pixel(int x, int y)
{
    x += dev->xoff;
    y += dev->yoff;

    y = invert(y);
    fprintf(fp, "gsave\n");
    setclr(-1);
    fprintf(fp, "[] 0 setdash\n");
    fprintf(fp, "%d %d moveto\n", x, y);
    fprintf(fp, "%d %d lineto\n", x, y+1);
    fprintf(fp, "stroke\n");
    fprintf(fp, "grestore\n");
}


void
PSparams::Pixels(GRmultiPt *data, int num)
{
    if (num < 1)
        return;
    fprintf(fp, "gsave\n");
    setclr(-1);
    fprintf(fp, "[] 0 setdash\n");
    data->data_ptr_init();
    while (num--) {
        int x = data->data_ptr_x() + dev->xoff;
        int y = invert(data->data_ptr_y() + dev->yoff);
        data->data_ptr_inc();
        fprintf(fp, "%d %d moveto\n", x, y);
        fprintf(fp, "%d %d lineto\n", x, y+1);
        fprintf(fp, "stroke\n");
    }
    fprintf(fp, "grestore\n");
}


void
PSparams::Line(int x1, int y1, int x2, int y2)
{
    x1 += dev->xoff;
    y1 += dev->yoff;
    x2 += dev->xoff;
    y2 += dev->yoff;

    // clip out unchanged points
    if (x1 == x2 && y1 == y2 && x1 == lastx && y1 == lasty)
        return;

    if (strokecnt > 499) {
        fprintf(fp, "stroke\n");
        strokecnt = 0;
        lastx = -1;
        lasty = -1;
    }
    int xt, yt;
    if (x2 == lastx && y2 == lastx) {
        xt = x1;
        x1 = x2;
        x2 = xt;
        yt = y1;
        y1 = y2;
        y2 = yt;
    }
    if ((x1 != lastx) || (y1 != lasty) || !strokecnt) {
        xt = x1;
        yt = y1;
        yt = invert(yt);
        fprintf(fp, "%d %d moveto\n", xt, yt);
    }
    if ((x2 != x1) || (y2 != y1)) {
        xt = x2;
        yt = y2;
        yt = invert(yt);
        fprintf(fp, "%d %d lineto\n", xt, yt);
    }

    // validate cache
    lastx = x2;
    lasty = y2;
    strokecnt++;
}


void
PSparams::PolyLine(GRmultiPt *data, int num)
{
    if (num < 2)
        return;
    data->data_ptr_init();
    num--;
    while (num--) {
        int x = data->data_ptr_x();
        int y = data->data_ptr_y();
        data->data_ptr_inc();
        Line(x, y, data->data_ptr_x(), data->data_ptr_y());
    }
}


void
PSparams::Lines(GRmultiPt *data, int num)
{
    if (num < 1)
        return;
    data->data_ptr_init();
    while (num--) {
        int x = data->data_ptr_x();
        int y = data->data_ptr_y();
        data->data_ptr_inc();
        Line(x, y, data->data_ptr_x(), data->data_ptr_y());
        data->data_ptr_inc();
    }
}


void
PSparams::Box(int x1, int y1, int x2, int y2)
{
    x1 += dev->xoff;
    y1 += dev->yoff;
    x2 += dev->xoff;
    y2 += dev->yoff;

    if (strokecnt) {
        fprintf(fp, "stroke\n");
        strokecnt = 0;
        lastx = -1;
        lasty = -1;
    }
    y1 = invert(y1);
    y2 = invert(y2);
    fprintf(fp, "%d %d moveto\n", x1, y1);
    fprintf(fp, "%d %d lineto\n", x2, y1);
    fprintf(fp, "%d %d lineto\n", x2, y2);
    fprintf(fp, "%d %d lineto\n", x1, y2);
    fprintf(fp, "%d %d lineto\n", x1, y1);
    fprintf(fp, "closepath fill\n");
}


void
PSparams::Boxes(GRmultiPt *data, int num)
{
    if (num < 1)
        return;
    data->data_ptr_init();
    while (num--) {
        int x = data->data_ptr_x();
        int y = data->data_ptr_y();
        data->data_ptr_inc();
        int w = data->data_ptr_x();
        int h = data->data_ptr_y();
        data->data_ptr_inc();
        Box(x, y, x + w - 1, y + h - 1);
    }
}


void
PSparams::Arc(int x0, int y0, int rx, int ry, double theta1, double theta2)
{
    if (rx <= 0 || ry <= 0)
        return;
    x0 += dev->xoff;
    y0 += dev->yoff;

    while (theta1 >= theta2)
        theta2 = 2 * M_PI + theta2;
    double a1 = (180.0/M_PI)*theta1;
    double a2 = (180.0/M_PI)*theta2;
    y0 = invert(y0);

    if (strokecnt) {
        fprintf(fp, "stroke\n");
        strokecnt = 0;
        lastx = -1;
        lasty = -1;
    }
    fprintf(fp, "gsave\n");
    setclr(-1);
    fprintf(fp, "[] 0 setdash\n");
    fprintf(fp, "%d %d %d %d %g %g ellipse\n", x0, y0, rx, ry, a1, a2);
    fprintf(fp, "stroke\n");
    fprintf(fp, "grestore\n");
}


void
PSparams::Polygon(GRmultiPt *data, int n)
{
    if (n < 4)
        return;
    if (strokecnt) {
        fprintf(fp, "stroke\n");
        strokecnt = 0;
        lastx = -1;
        lasty = -1;
    }
    data->data_ptr_init();
    fprintf(fp, "%d %d moveto\n", data->data_ptr_x() + dev->xoff,
        invert(data->data_ptr_y() + dev->yoff));
    n--;
    while (n--) {
        data->data_ptr_inc();
        fprintf(fp, "%d %d lineto\n", data->data_ptr_x() + dev->xoff,
            invert(data->data_ptr_y() + dev->yoff));
    }
    fprintf(fp, "closepath fill\n");
}


void
PSparams::Zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
{
    yl += dev->yoff;
    yu += dev->yoff;
    xll += dev->xoff;
    xul += dev->xoff;
    xlr += dev->xoff;
    xur += dev->xoff;

    if (strokecnt) {
        fprintf(fp, "stroke\n");
        strokecnt = 0;
        lastx = -1;
        lasty = -1;
    }
    if (yl >= yu)
        return;
    fprintf(fp, "%d %d moveto\n", xll, invert(yl));
    fprintf(fp, "%d %d lineto\n", xul, invert(yu));
    if (xur > xul)
        fprintf(fp, "%d %d lineto\n", xur, invert(yu));
    fprintf(fp, "%d %d lineto\n", xlr, invert(yl));
    if (xll < xlr)
        fprintf(fp, "%d %d lineto\n", xll, invert(yl));
    fprintf(fp, "closepath fill\n");
}


void
PSparams::Text(const char *text, int x, int y, int xform, int width,
    int height)
{
    x += dev->xoff;
    y += dev->yoff;

    if (text == 0)
        return;
    if (!width || !height)
        return;
    char tbuf[512];
    char *s = tbuf;
    while (*text) {
        if (*text == '(' || *text == ')' || *text == '\\')
            *s++ = '\\';
        *s++ = *text++;
    }
    *s = '\0';
    y = invert(y);

    fprintf(fp, "gsave\n");
    setclr(-1);
    fprintf(fp, "[] 0 setdash\n");
    if (xform & TXTF_HJR)
        fprintf(fp, "(%s) stringwidth pop neg %d add %d translate\n",
            tbuf, x, y);
    else if (xform & TXTF_HJC)
        fprintf(fp, "(%s) stringwidth pop -2 div %d add %d translate\n",
            tbuf, x, y);
    else
        fprintf(fp, "%d %d translate\n", x, y);
    if (xform & TXTF_VJT)
        fprintf(fp, "0 %g translate\n",
            -FONTPTS*(double)dev->data->resol/72);
    else if (xform & TXTF_VJC)
        fprintf(fp, "0 %g translate\n",
            -FONTPTS*(double)dev->data->resol/(2*72));
    if (xform & (TXTF_MX | TXTF_MY))
        fprintf(fp, "%d %d scale\n",
            (xform & TXTF_MX) ? -1 : 1, (xform & TXTF_MY) ? -1 : 1);
    int deg = (xform & TXTF_ROT) * 90;
    if (xform & TXTF_45)
        deg += 45;
    if (deg)
        fprintf(fp, "%d rotate\n", deg);
    if (width > 0 && height > 0) {
        double sy = height/(FONTPTS*(double)dev->data->resol/72);
        fprintf(fp, "%d (%s) stringwidth pop div %g scale\n", width,
            tbuf, sy);
    }
    fprintf(fp, "0 0 moveto\n");
    fprintf(fp, "(%s) show\n", tbuf);
    fprintf(fp, "grestore\n");
}


void
PSparams::TextExtent(const char *string, int *x, int *y)
{
    // This will be inacurate
    *y = (FONTPTS*dev->data->resol)/72;
    *x = (6*(*y))/10;  // approximate
    if (string)
        *x *= strlen(string);
}


void
PSparams::SetColor(int pixel)
{
    if (!usecolor) {
        if (pixel == GRappIf()->BackgroundPixel()) {
            if (backg)
                return;
            backg = true;
        }
        else {
            if (!backg)
                return;
            backg = false;
        }
        if (strokecnt) {
            fprintf(fp, "stroke\n");
            strokecnt = 0;
            lastx = -1;
            lasty = -1;
        }
        fprintf(fp, "%d setgray\n", backg ? 1 : 0);
        return;
    }

    int r, g, b;
    if (pixel == GRappIf()->BackgroundPixel())
        r = g = b = 255;
    else {
        GRpkgIf()->RGBofPixel(pixel, &r, &g, &b);
        // map pure white to black
        if (r == 255 && g == 255 && b == 255)
            r = g = b = 0;
    }
    if (r != red || g != green || b != blue) {
        if (strokecnt) {
            fprintf(fp, "stroke\n");
            strokecnt = 0;
            lastx = -1;
            lasty = -1;
        }
        red = r;
        green = g;
        blue = b;
        setclr(curfpat);
    }
}


void
PSparams::SetLinestyle(const GRlineType *lineptr)
{
    if (lineptr == SOLID || !lineptr->mask || lineptr->mask == -1) {
        if (linestyle) {
            if (strokecnt) {
                fprintf(fp, "stroke\n");
                strokecnt = 0;
                lastx = -1;
                lasty = -1;
            }
            fprintf(fp, "[] 0 setdash\n");
            linestyle = SOLID;
        }
        return;
    }
    if (linestyle != lineptr) {
        if (strokecnt) {
            fprintf(fp, "stroke\n");
            strokecnt = 0;
            lastx = -1;
            lasty = -1;
        }
        char buf[256];
        buf[0] = '[';
        buf[1] = '\0';
        char *s = buf+1;

        for (int i = 0; i < lineptr->length; i++) {
            snprintf(s, 8, " %d", (int)lineptr->dashes[i]);
            while (*s) s++;
        }
        while (*s) s++;
        *s++ = ' ';
        *s++ = ']';
        *s = '\0';
        fprintf(fp, "%s 0 setdash\n", buf);
        linestyle = lineptr;
    }
}


namespace {
    char hexc[16] =
        { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
}

void
PSparams::SetFillpattern(const GRfillType *fillp)
{
    if (strokecnt) {
        fprintf(fp, "stroke\n");
        strokecnt = 0;
        lastx = -1;
        lasty = -1;
    }
    if (fillp && fillp->hasMap()) {
        int num;
        for (num = 0; fpats[num]; num++)
            if (fpats[num] == fillp) {
                if (curfpat != num) {
                    curfpat = num;
                    setclr(num);
                }
                return;
            }
        if (num >= PS_MAX_FPs)
            return;
        fprintf(fp, "<<\n/PaintType 2\n/PatternType 1 /TilingType 1\n");
        fprintf(fp, "/BBox [0 0 %d %d]\n/XStep %d /YStep %d\n",
            fillp->nX(), fillp->nY(), fillp->nX(), fillp->nY());
        fprintf(fp, "/PaintProc {\npop\n");
        fprintf(fp, "%d %d true\n", fillp->nX(), fillp->nY());
        fprintf(fp, "matrix\n{<");
        int bpl = (fillp->nX() + 7) >> 3;
        unsigned char *map = fillp->newBitmap();
        unsigned char *ptr = map + bpl*(fillp->nY() - 1);
        for (int j = fillp->nY(); j >= 0; j--) {
            for (int i = 0; i < bpl; i++) {
                unsigned char c = *(unsigned char*)ptr;
                putc(hexc[c>>4], fp);
                putc(hexc[c&0xf], fp);
                ptr++;
            }
            ptr -= bpl << 1;
        }
        delete [] map;
        fprintf(fp, ">}\nimagemask\n}\n>>\nmatrix\nmakepattern\n");
        fprintf(fp, "/Fpat%d exch def\n", num);
        fpats[num] = fillp;
        curfpat = num;
        setclr(num);
        return;
    }
    curfpat = -1;
    setclr(-1);
}


void
PSparams::DisplayImage(const GRimage*, int, int, int, int)
{
    GRpkgIf()->HCabort("PS line-draw driver doesn't suppport image mode.");
}


// Return the resolution, relative to the on-screen resolution, needed
// for size thresholding.
//
double
PSparams::Resolution()
{
    return (dev->data->resol/GR_SCREEN_RESOL);
}


void
PSparams::setclr(int num)
{
    if (usecolor) {
        if (num >= 0) {
            fprintf(fp, "[/Pattern [/DeviceRGB]] setcolorspace\n");
            fprintf(fp, "%g %g %g Fpat%d setcolor\n",
                (double)red/255, (double)green/255,
                (double)blue/255, num);
        }
        else {
            fprintf(fp, "%g %g %g setrgbcolor\n",
                (double)red/255, (double)green/255,
                (double)blue/255);
        }
    }
    else {
        if (num >= 0) {
            fprintf(fp, "[/Pattern /DeviceGray] setcolorspace\n");
            fprintf(fp, "%d Fpat%d setcolor\n", backg ? 1 : 0, num);
        }
        else
            fprintf(fp, "%d setgray\n", backg ? 1 : 0);
    }
}

