
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
#include "hpgl.h"

#ifndef M_PI
#define	M_PI  3.14159265358979323846  // pi
#endif

// Driver code for HPGL

// Font width/height (cm.)
#define FCMW 0.22
#define FCMH 0.32

namespace ginterf
{
    HCdesc HPdesc(
        "HP",                       // drname
        "HPGL line draw, color",    // descr
        "hpgl_line_draw_color",     // keyword
        "hpgl",                     // alias
        "-f %s -r %d -t 1 -w %f -h %f -x %f -y %f", // format
        HClimits(
            0.0, 22.0,  // minxoff, maxxoff
            0.0, 22.0,  // minyoff, maxyoff
            1.0, 23.5,  // minwidth, maxwidth
            1.0, 23.5,  // minheight, maxheight
                        // flags
            HCnoCanRotate | HCfixedResol | HCnoBestOrient,
            0           // resols
        ),
        HCdefaults(
            0.25, 0.25, // defxoff, defyoff
            7.8, 10.3,  // defwidth, defheight
            0,          // command string to print file
            0,          // defresol (index into resols)
            HClegOn,    // initially use legend
            HCbest      // initially set best orientation
        ),
        true);      // line draw


    bool
    HPdev::Init(int *ac, char **av)
    {
        if (!ac || !av)
            return (true);
        HCdata *hd = new HCdata;
        hd->filename = 0;
        hd->width = 8.0;
        hd->height = 10.5;
        hd->resol = 1016;

        if (HCdevParse(hd, ac, av)) {
            delete hd;
            return (true);
        }

        if (!hd->filename || !*hd->filename) {
            delete hd;
            return (true);
        }
        HCcheckEntries(hd, &HPdesc);

        // fixed resolution
        hd->resol = 1016;

        delete data;
        data = hd;
        numlinestyles = 0;
        numcolors = 32;

        return (false);
    }
}


GRdraw *
HPdev::NewDraw(int)
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
    HPparams *hpgl = new HPparams;
    hpgl->fp = plotfp;
    hpgl->dev = this;

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
    hpgl->landscape = data->landscape;

    return (hpgl);
}


void
HPparams::Halt()
{
    fprintf(fp, "PU;SP0;PG\n");
    fclose(fp);
    delete this;
}


void
HPparams::ResetViewport(int wid, int hei)
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
HPparams::DefineViewport()
{
    // init line
    fprintf(fp, "\033%%-1BBPIN\n");
    // rotate to reasonable coordinate system (LL origin)
    if (landscape)
        fprintf(fp, "RO270IP\n");
    else
        fprintf(fp, "IP\n");
    // plot size
    fprintf(fp, "PS%d,%d\n", dev->xoff + dev->width, dev->yoff + dev->height);
    // palette initialization
    fprintf(fp, "PW0.15\n");
    fprintf(fp, "TR1CR0,255,0,255,0,255NP8\n");
    fprintf(fp, "PC1,0,0,0\n");
    fprintf(fp, "PC2,255,0,0\n");
    fprintf(fp, "PC3,0,255,0\n");
    fprintf(fp, "PC4,255,255,0\n");
    fprintf(fp, "PC5,0,0,255\n");
    fprintf(fp, "PC6,255,0,255\n");
    fprintf(fp, "PC7,0,255,255\n");
    // character size
    fprintf(fp, "SI%g,%g;\n", FCMW, FCMH);
}


void
HPparams::Pixel(int x, int y)
{
    x += dev->xoff;
    y += dev->yoff;

    fprintf(fp, "PU%d,%dPD\n", x, invert(y));
}


void
HPparams::Pixels(GRmultiPt *data, int num)
{
    if (num < 1)
        return;
    data->data_ptr_init();
    while (num--) {
        fprintf(fp, "PU%d,%dPD\n", data->data_ptr_x() + dev->xoff,
            invert(data->data_ptr_y() + dev->yoff));
        data->data_ptr_inc();
    }
}


void
HPparams::Line(int x1, int y1, int x2, int y2)
{
    x1 += dev->xoff;
    y1 += dev->yoff;
    x2 += dev->xoff;
    y2 += dev->yoff;

    // clip out unchanged points
    if (x1 == x2 && y1 == y2 && x1 == lastx && y1 == lasty)
        return;

    if (x2 == lastx && y2 == lasty) {
        int xt = x1;
        x1 = x2;
        x2 = xt;
        int yt = y1;
        y1 = y2;
        y2 = yt;
    }
    fprintf(fp, "PU%d,%dPD%d,%d\n", x1, invert(y1), x2, invert(y2));

    // validate cache
    lastx = x2;
    lasty = y2;
}


void
HPparams::PolyLine(GRmultiPt *data, int num)
{
    /*
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
    */

    if (num < 2)
        return;
    data->data_ptr_init();
    int x = data->data_ptr_x() + dev->xoff;
    int y = data->data_ptr_y() + dev->yoff;
    data->data_ptr_inc();
    fprintf(fp, "PU%d,%dPD%d,%d\n", x, invert(y),
        data->data_ptr_x() + dev->xoff,
        invert(data->data_ptr_y() + dev->yoff));
    num -= 2;
    while (num--) {
        data->data_ptr_inc();
        fprintf(fp, ",%d,%d\n", data->data_ptr_x() + dev->xoff,
            invert(data->data_ptr_y() + dev->yoff));
    }
}


void
HPparams::Lines(GRmultiPt *data, int num)
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
HPparams::Box(int x1, int y1, int x2, int y2)
{
    x1 += dev->xoff;
    y1 += dev->yoff;
    x2 += dev->xoff;
    y2 += dev->yoff;

    if (nofill)
        fprintf(fp, "PU%d,%dEA%d,%d\n", x1, invert(y1), x2, invert(y2));
    else
        fprintf(fp, "PU%d,%dRA%d,%d\n", x1, invert(y1), x2, invert(y2));
}


void
HPparams::Boxes(GRmultiPt *data, int num)
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
HPparams::Arc(int x0, int y0, int rx, int ry, double theta1, double theta2)
{
    if (rx <= 0 || ry <= 0)
        return;
    // No easy HPGL ellipse support.
    int radius = (rx + ry)/2;
    x0 += dev->xoff;
    y0 += dev->yoff;

    if (theta1 >= theta2)
        theta2 = 2 * M_PI + theta2;
    int dang = (int)((180.0 / M_PI) * (theta2 - theta1));
    int x = x0 + (int)(radius*cos(theta1));
    int y = y0 + (int)(radius*sin(theta1));

    fprintf(fp, "PU%d,%dPDAA%d,%d,%d\n",
        x0, invert(y0), x, invert(y), dang);
}


void
HPparams::Polygon(GRmultiPt *data, int n)
{
    if (n < 4)
        return;
    data->data_ptr_init();
    int x = data->data_ptr_x() + dev->xoff;
    int y = data->data_ptr_y() + dev->yoff;
    data->data_ptr_inc();
    fprintf(fp, "PU%d,%dPMPD%d,%d\n",
        x, invert(y), data->data_ptr_x() + dev->xoff,
            invert(data->data_ptr_y() + dev->yoff));
    n -= 2;
    while (n--) {
        data->data_ptr_inc();
        fprintf(fp, ",%d,%d\n", data->data_ptr_x() + dev->xoff,
            invert(data->data_ptr_y() + dev->yoff));
    }
    if (nofill)
        fprintf(fp, "PM2EP\n");
    else
        fprintf(fp, "PM2FP\n");
}


void
HPparams::Zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
{
    yl += dev->yoff;
    yu += dev->yoff;
    xll += dev->xoff;
    xul += dev->xoff;
    xlr += dev->xoff;
    xur += dev->xoff;

    if (yl >= yu)
        return;
    fprintf(fp, "PU%d,%dPMPD%d,%d\n", xll, invert(yl), xul, invert(yu));
    if (xur > xul)
        fprintf(fp, ",%d,%d\n", xur, invert(yu));
    fprintf(fp, ",%d,%d\n", xlr, invert(yl));
    if (xll < xlr)
        fprintf(fp, ",%d,%d\n", xll, invert(yl));
    if (nofill)
        fprintf(fp, "PM2EP\n");
    else
        fprintf(fp, "PM2FP\n");
}


void
HPparams::Text(const char *text, int x, int y, int xform, int width,
    int height)
{
    x += dev->xoff;
    y += dev->yoff;

    if (!width || !height)
        return;
    // how to do mirror?
    if (text == 0)
        return;
    fprintf(fp, "PU%d,%d", x, invert(y));
    if (xform & TXTF_HJR) {
        if (xform & TXTF_VJT)
            fprintf(fp, "LO9");
        else if (xform & TXTF_VJC)
            fprintf(fp, "LO8");
        else
            fprintf(fp, "LO7");
    }
    else if (xform & TXTF_HJC) {
        if (xform & TXTF_VJT)
            fprintf(fp, "LO6");
        else if (xform & TXTF_VJC)
            fprintf(fp, "LO5");
        else
            fprintf(fp, "LO4");
    }
    else {
        if (xform & TXTF_VJT)
            fprintf(fp, "LO3");
        else if (xform & TXTF_VJC)
            fprintf(fp, "LO2");
        else
            fprintf(fp, "LO1");
    }

    switch (xform & TXTF_ROT) {
    case 0:
        break;
    case 1:
        fprintf(fp, "DI0,1");
        break;
    case 2:
        fprintf(fp, "DI-1,0");
        break;
    case 3:
        fprintf(fp, "DI0,-1");
        break;
    }
    fprintf(fp, "LB%s%c\n", text, '\003');
}


void
HPparams::TextExtent(const char *text, int *x, int *y)
{
    *y = (int)(1016*FCMH/2.54);
    *x = (int)(1.5*1016*FCMW/2.54);
    // Above, 1.5 is a fudge factor determined with hp2xx.
    if (text)
        *x *= strlen(text);
}


void
HPparams::SetColor(int pixel)
{
    int pen = GRappIf()->PixelIndex(pixel);
    if (!pen)
        pen = 1;
    if (curpen != pen) {
        curpen = pen;
        fprintf(fp, "SP%d\n", pen);
    }
}


void
HPparams::SetLinestyle(const GRlineType *lineptr)
{
    if (!lineptr) {
        if (curline) {
            fprintf(fp, "LT;\n");
            curline = 0;
        }
    }
    else {
        if (!curline) {
            // 50% dash, 4 millimeter period
            fprintf(fp, "LT2,4,1;\n");
            curline = 1;
        }
    }
}


void
HPparams::SetFillpattern(const GRfillType *fillp)
{
    if (!fillp) {
        nofill = true;
        return;
    }
    int lindex = GRappIf()->FillTypeIndex(fillp);
    if (lindex <= 0) {
        nofill = true;
        return;
    }
    int opt1, opt2;
    int type = GRappIf()->FillStyle(_devHP_, lindex, &opt1, &opt2);
    if (type == 0) {
        nofill = true;
        return;
    }
    switch (type) {
    case 1:
    case 2:
        nofill = false;
        fprintf(fp, "FT%d\n", type);
        break;
    case 3:
    case 4:
    case 11:
        nofill = false;
        if (opt1 == 0)
            fprintf(fp, "FT%d\n", type);
        else if (opt2 == 0)
            fprintf(fp, "FT%d,%d\n", type, opt1);
        else
            fprintf(fp, "FT%d,%d,%d;\n", type, opt1, opt2);
        break;
    case 10:
        nofill = false;
        if (opt1 == 0)
            fprintf(fp, "FT%d\n", type);
        else
            fprintf(fp, "FT%d,%d\n", type, opt1);
        break;
    }
}


void
HPparams::DisplayImage(const GRimage*, int, int, int, int)
{
    GRpkgIf()->HCabort("HPGL driver doesn't suppport image mode.");
}


// Return the resolution, relative to the on-screen resolution, needed
// for size thresholding.
//
double
HPparams::Resolution()
{
    return (dev->data->resol/GR_SCREEN_RESOL);
}

