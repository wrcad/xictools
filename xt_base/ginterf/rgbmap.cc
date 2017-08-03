
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

// Routines for rendering on an in-core RGB bitmap

#include <stdio.h>
#include <math.h>
#include "rgbmap.h"
#include "grfont.h"
#include "polydecomp.h"


#ifndef M_PI
#define	M_PI  3.14159265358979323846  // pi
#endif

#define swap(a, b) {int t=a; a=b; b=t;}


// Draw pixel function.
//
void
RGBparams::Pixel(int x, int y)
{
    if (y < 0 || y >= dev->height || x < 0 || x >= dev->width)
        return;
    unsigned char *bs = base + y*bytpline + 3*x;
    *bs++ = red;
    *bs++ = green;
    *bs = blue;
}


// Draw multiple pixels.
//
void
RGBparams::Pixels(GRmultiPt *data, int n)
{
    if (n < 1)
        return;
    data->data_ptr_init();
    while (n--) {
        int x = data->data_ptr_x();
        int y = data->data_ptr_y();
        data->data_ptr_inc();
        if (y < 0 || y >= dev->height || x < 0 || x >= dev->width)
            continue;
        unsigned char *bs = base + y*bytpline + 3*x;
        *bs++ = red;
        *bs++ = green;
        *bs = blue;
    }
}


#define ror(b) b >>= 1; if (!b) b = 0x80
#define rorl(b, p) b >>= 1; if (!b) b = 1 << (p-1)

// Draw line function.
//
void
RGBparams::Line(int x1, int y1, int x2, int y2)
{
    if (dev->LineClip(&x1, &y1, &x2, &y2))
        return;
    if (x2 < x1) {
        swap(x1, x2);
        swap(y1, y2);
    }
    int dx = x2 - x1;
    int dy = y2 - y1;

    int lcnt = y1;
    int next = bytpline;
    unsigned char *bs = base + lcnt*next + 3*x1;

    if (y1 > y2) {
        next = -next;
        dy = -dy;
    }
    int dy2 = dy;

    unsigned right = linestyle;
    if (!right)
        right = 0xff;
    unsigned left;
    int cnt;
    for (left = right, cnt = 0; left; left >>= 1, cnt++) ;
    left = 1 << (cnt - 1 - (lcnt + x1)%cnt);

    int errterm = -dx >> 1;
    for (dy++; dy; dy--) {
        errterm += dx;
        if (errterm <= 0) {
            if (left & right) {
                *bs++ = red;
                *bs++ = green;
                *bs = blue;
                bs -= 2;
            }
            bs += next;
            rorl(left, cnt);
            continue;
        }
        while (errterm > 0 && x1 != x2) {
            if (left & right) {
                *bs++ = red;
                *bs++ = green;
                *bs++ = blue;
            }
            else
                bs += 3;
            rorl(left, cnt);
            x1++;
            errterm -= dy2;
        }
        bs += next;
    }
}


// Draw a path.
//
void
RGBparams::PolyLine(GRmultiPt *data, int n)
{
    if (n < 2)
        return;
    data->data_ptr_init();
    n--;
    while (n--) {
        int x = data->data_ptr_x();
        int y = data->data_ptr_y();
        data->data_ptr_inc();
        Line(x, y, data->data_ptr_x(), data->data_ptr_y());
    }
}


// Draw multiple lines.
//
void
RGBparams::Lines(GRmultiPt *data, int n)
{
    if (n < 1)
        return;
    data->data_ptr_init();
    while (n--) {
        int x = data->data_ptr_x();
        int y = data->data_ptr_y();
        data->data_ptr_inc();
        Line(x, y, data->data_ptr_x(), data->data_ptr_y());
        data->data_ptr_inc();
    }
}


// Draw a rectangle, possibly filled.
//
void
RGBparams::Box(int xl, int yl, int xu, int yu)
{
    if (yu < yl) swap(yu, yl);
    if (xu < xl) swap(xu, xl);

    if (xl < 0)
        xl = 0;
    else if (xl >= dev->width)
        return;

    if (xu > dev->width - 1)
        xu = dev->width - 1;
    else if (xu < 0)
        return;

    if (yl < 0)
        yl = 0;
    else if (yl >= dev->height)
        return;

    if (yu > dev->height - 1)
        yu = dev->height - 1;
    else if (yu < 0)
        return;

    int lnum = yl;
    // The box includes the boundary points, so will cover a line
    // box with the same points.
    int dy = yu - yl + 1;
    int dx = xu - xl + 1;

    unsigned char *bs = base + lnum*bytpline + 3*xl;
    int next = bytpline - 3*dx;

    if (!curfillpatt) {
        while (dy--) {
            int tx = dx;
            while (tx--) {
                *bs++ = red;
                *bs++ = green;
                *bs++ = blue;
            }
            bs += next;
        }
    }
    else {
        const GRfillType *fpat = curfillpatt;
        while (dy--) {
            int tx = dx;
            unsigned int xo = xl;
            unsigned int yo = lnum++;
            fpat->initPixelScan(&xo, &yo);
            while (tx--) {
                if (fpat->getNextColPixel(&xo, yo)) {
                    *bs++ = red;
                    *bs++ = green;
                    *bs++ = blue;
                }
                else
                    bs += 3;
            }
            bs += next;
        }
    }
}


// Draw multiple boxes.
//
void
RGBparams::Boxes(GRmultiPt *data, int n)
{
    if (n < 1)
        return;
    data->data_ptr_init();
    while (n--) {
        int x = data->data_ptr_x();
        int y = data->data_ptr_y();
        data->data_ptr_inc();
        int w = data->data_ptr_x();
        int h = data->data_ptr_y();
        data->data_ptr_inc();
        Box(x, y, x + w - 1, y + h - 1);
    }
}


// Draw an arc.
//
void
RGBparams::Arc(int x, int y, int rx, int ry, double theta1, double theta2)
{
    if (rx <= 0 || ry <= 0)
        return;
    while (theta1 >= theta2)
        theta2 = 2*M_PI + theta2;
    double dt = M_PI/100;
    int x0 = (int)(rx*cos(theta1));
    int y0 = -(int)(ry*sin(theta1));
    for (double ang = theta1 + dt; ang < theta2; ang += dt) {
        int x1 = (int)(rx*cos(ang));
        int y1 = -(int)(ry*sin(ang));
        Line(x+x0, y+y0, x+x1, y+y1);
        x0 = x1;
        y0 = y1;
    }
    Line(x+x0, y+y0, x+(int)(rx*cos(theta2)), y-(int)(ry*sin(theta2)));
}


// Draw a polygon, possibly filled.
//
void
RGBparams::Polygon(GRmultiPt *data, int num)
{
    poly_decomp::instance().polygon(this, data, num);
}


// Draw a trapezoid.
//
void
RGBparams::Zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
{
    if (yl >= dev->height || yu < 0)
        return;

    int dy = yu - yl;
    unsigned char *base0 = base + yl*bytpline;

    int dx1 = xul - xll;
    int dx2 = xlr - xur;
    int s1 = 1;
    int s2 = -1;
    if (xll >= xul) { dx1 = -dx1; s1 = -1; }
    if (xur >= xlr) { dx2 = -dx2; s2 = 1; }
    int errterm1 = -dx1 >> 1;
    int errterm2 = -dx2 >> 1;

    if (!curfillpatt) {
        for (int y = yl; y <= yu; y++) {
            if (y >= 0 && y < dev->height) {
                int xstrt = xll >= 0 ? xll : 0;
                unsigned char *bs = base0 + 3*xstrt;
                int xend = xlr + 1;
                if (xend > dev->width)
                    xend = dev->width;
                for (int x = xstrt; x < xend; x++) {
                    *bs++ = red;
                    *bs++ = green;
                    *bs++ = blue;
                }
            }
            base0 += bytpline;
            errterm1 += dx1;
            errterm2 += dx2;
            while (errterm1 > 0 && xul != xll) {
                errterm1 -= dy;
                xll += s1;
            }
            while (errterm2 > 0 && xlr != xur) {
                errterm2 -= dy;
                xlr += s2;
            }
        }
    }
    else {
        const GRfillType *fpat = curfillpatt;
        for (int y = yl; y <= yu; y++) {
            if (y >= 0 && y < dev->height) {
                int xstrt = xll >= 0 ? xll : 0;
                unsigned char *bs = base0 + 3*xstrt;
                int xend = xlr + 1;
                if (xend > dev->width)
                    xend = dev->width;
                unsigned int xo = xstrt;
                unsigned int yo = y;
                fpat->initPixelScan(&xo, &yo);
                for (int x = xstrt; x < xend; x++) {
                    if (fpat->getNextColPixel(&xo, yo)) {
                        *bs++ = red;
                        *bs++ = green;
                        *bs++ = blue;
                    }
                    else
                        bs += 3;
                }
            }
            base0 += bytpline;
            errterm1 += dx1;
            errterm2 += dx2;
            while (errterm1 > 0 && xul != xll) {
                errterm1 -= dy;
                xll += s1;
            }
            while (errterm2 > 0 && xlr != xur) {
                errterm2 -= dy;
                xlr += s2;
            }
        }
    }
}


// Render a text string.  We don't handle transforms here.
//
void
RGBparams::Text(const char *text, int x, int y, int xform, int width,
    int height)
{
    if (!width || !height)
        return;
    int lwid, lhei, numlines;
    FT.textExtent(text, &lwid, &lhei, &numlines);
    int w, h;
    if (width > 0 && height > 0) {
        w = width/lwid;
        h = height/lhei;
    }
    else {
        w = (int)(lwid*text_scale);
        h = (int)(lhei*text_scale);
    }
    FT.renderText(this, text, x, y, xform, w, h);
}


// Get the pixel extent of string.
//
void
RGBparams::TextExtent(const char *text, int *x, int *y)
{
    int lwid, lhei, lnum;
    FT.textExtent(text, &lwid, &lhei, &lnum);
    *x = (int)(lwid*text_scale);
    *y = (int)(lhei*text_scale);
}


// Set linestyle function.
//
void
RGBparams::SetLinestyle(const GRlineType *lineptr)
{
    if (!lineptr || !lineptr->mask || lineptr->mask == -1)
        linestyle = 0;
    else
        linestyle = lineptr->mask;
}


// Set fillpattern function.
//
void
RGBparams::SetFillpattern(const GRfillType *fillp)
{
    if (!fillp || !fillp->hasMap())
        curfillpatt = 0;
    else
        curfillpatt = fillp;
}


void
RGBparams::SetColor(int pixel)
{
    int r, g, b;
    if (pixel == GRappIf()->BackgroundPixel())
        r = g = b = 0;
    else
        GRpkgIf()->RGBofPixel(pixel, &r, &g, &b);
    if (invert_colors) {
        red = 255 - r;
        green = 255 - g;
        blue = 255 - b;
    }
    else {
        red = r;
        green = g;
        blue = b;
    }
}


void
RGBparams::DisplayImage(const GRimage *image, int x, int y,
    int width, int height)
{
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
            unsigned int pix = image->data()[xd + yd*image->width()];
            put_pixel(pix, xd, yd);
        }
    }
}


void
RGBparams::put_pixel(unsigned int pixel, int x, int y)
{
    if (y < 0 || y >= dev->height || x < 0 || x >= dev->width)
        return;

    int r, g, b;
    if (pixel == (unsigned int)GRappIf()->BackgroundPixel())
        r = g = b = 255;
    else {
        GRpkgIf()->RGBofPixel(pixel, &r, &g, &b);
        // map pure white to black & vice-versa
        if (r == 255 && g == 255 && b == 255)
            r = g = b = 0;
        else if (r == 0 && g == 0 && b == 0)
            r = g = b = 255;
    }
    // assume that the color components will be inverted
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;

    unsigned char *bs = base + y*bytpline + 3*x;
    *bs++ = r;
    *bs++ = g;
    *bs = b;
}

