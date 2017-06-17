
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id: rgbzimg.cc,v 2.6 2013/12/06 05:11:28 stevew Exp $
 *========================================================================*/

#include "config.h"
#include <stdio.h>
#include <math.h>
#include "rgbzimg.h"
#include "grfont.h"
#include "polydecomp.h"

#ifndef M_PI
#define	M_PI  3.14159265358979323846  // pi
#endif

#define swap(a, b) {int t=a; a=b; b=t;}


// Draw pixel function.
//
void
RGBzimg::Pixel(int x, int y)
{
    if (y < 0 || y >= rz_dev->height || x < 0 || x >= rz_dev->width)
        return;
    DrawPixel(y*rz_dev->width + x);
}


// Draw multiple pixels.
//
void
RGBzimg::Pixels(GRmultiPt *data, int n)
{
    if (n < 1)
        return;
    data->data_ptr_init();
    while (n--) {
        int x = data->data_ptr_x();
        int y = data->data_ptr_y();
        data->data_ptr_inc();
        if (y < 0 || y >= rz_dev->height || x < 0 || x >= rz_dev->width)
            continue;
        DrawPixel(y*rz_dev->width + x);
    }
}


#define ror(b) b >>= 1; if (!b) b = 0x80
#define rorl(b, p) b >>= 1; if (!b) b = 1 << (p-1)

// Draw line function.
//
void
RGBzimg::Line(int x1, int y1, int x2, int y2)
{
    if (rz_dev->LineClip(&x1, &y1, &x2, &y2))
        return;
    if (x2 < x1) {
        swap(x1, x2);
        swap(y1, y2);
    }
    int dx = x2 - x1;
    int dy = y2 - y1;

    int lcnt = y1;
    int next = rz_dev->width;
    int offs = lcnt*next + x1;

    if (y1 > y2) {
        next = -next;
        dy = -dy;
    }
    int dy2 = dy;

    unsigned right = rz_cur_ls;
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
            if (left & right)
                DrawPixel(offs);
            offs += next;
            rorl(left, cnt);
            continue;
        }
        while (errterm > 0 && x1 != x2) {
            if (left & right)
                DrawPixel(offs);
            offs++;
            rorl(left, cnt);
            x1++;
            errterm -= dy2;
        }
        offs += next;
    }
}


// Draw a path.
//
void
RGBzimg::PolyLine(GRmultiPt *data, int n)
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
RGBzimg::Lines(GRmultiPt *data, int n)
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
RGBzimg::Box(int xl, int yl, int xu, int yu)
{
    if (yu < yl) swap(yu, yl);
    if (xu < xl) swap(xu, xl);

    if (xl < 0)
        xl = 0;
    else if (xl >= rz_dev->width)
        return;

    if (xu > rz_dev->width - 1)
        xu = rz_dev->width - 1;
    else if (xu < 0)
        return;

    if (yl < 0)
        yl = 0;
    else if (yl >= rz_dev->height)
        return;

    if (yu > rz_dev->height - 1)
        yu = rz_dev->height - 1;
    else if (yu < 0)
        return;

    int lnum = yl;
    // The box includes the boundary points, so will cover a line
    // box with the same points.
    int dy = yu - yl + 1;
    int dx = xu - xl + 1;

    unsigned int offs = lnum*rz_dev->width + xl;
    int next = rz_dev->width - dx;

    if (!rz_cur_fp) {
        while (dy--) {
            int tx = dx;
            while (tx--) {
                DrawPixel(offs);
                offs++;
            }
            offs += next;
        }
    }
    else {
        const GRfillType *fpat = rz_cur_fp;
        while (dy--) {
            int tx = dx;
            unsigned int xo = xl;
            unsigned int yo = lnum++;
            fpat->initPixelScan(&xo, &yo);
            while (tx--) {
                if (fpat->getNextColPixel(&xo, yo))
                    DrawPixel(offs);
                offs++;
            }
            offs += next;
        }
    }
}


// Draw multiple boxes.
//
void
RGBzimg::Boxes(GRmultiPt *data, int n)
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
RGBzimg::Arc(int x, int y, int rx, int ry, double theta1, double theta2)
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
RGBzimg::Polygon(GRmultiPt *data, int num)
{
    poly_decomp::instance().polygon(this, data, num);
}


// Draw a trapezoid.
//
void
RGBzimg::Zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
{
    if (yl >= rz_dev->height || yu < 0)
        return;

    int dy = yu - yl;
    int offs0 = yl*rz_dev->width;

    int dx1 = xul - xll;
    int dx2 = xlr - xur;
    int s1 = 1;
    int s2 = -1;
    if (xll >= xul) { dx1 = -dx1; s1 = -1; }
    if (xur >= xlr) { dx2 = -dx2; s2 = 1; }
    int errterm1 = -dx1 >> 1;
    int errterm2 = -dx2 >> 1;

    if (!rz_cur_fp) {
        for (int y = yl; y <= yu; y++) {
            if (y >= 0 && y < rz_dev->height) {
                int xstrt = xll >= 0 ? xll : 0;
                int offs = offs0 + xstrt;
                int xend = xlr + 1;
                if (xend > rz_dev->width)
                    xend = rz_dev->width;
                for (int x = xstrt; x < xend; x++) {
                    DrawPixel(offs);
                    offs++;
                }
            }
            offs0 += rz_dev->width;
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
        const GRfillType *fpat = rz_cur_fp;
        for (int y = yl; y <= yu; y++) {
            if (y >= 0 && y < rz_dev->height) {
                int xstrt = xll >= 0 ? xll : 0;
                int offs = offs0 + xstrt;
                int xend = xlr + 1;
                if (xend > rz_dev->width)
                    xend = rz_dev->width;
                unsigned int xo = xstrt;
                unsigned int yo = y;
                fpat->initPixelScan(&xo, &yo);
                for (int x = xstrt; x < xend; x++) {
                    if (fpat->getNextColPixel(&xo, yo))
                        DrawPixel(offs);
                    offs++;
                }
            }
            offs0 += rz_dev->width;
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
RGBzimg::Text(const char *text, int x, int y, int xform, int width,
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
        w = (int)(lwid*rz_text_scale);
        h = (int)(lhei*rz_text_scale);
    }
    FT.renderText(this, text, x, y, xform, w, h);
}


// Get the pixel extent of string.
//
void
RGBzimg::TextExtent(const char *text, int *x, int *y)
{
    int lwid, lhei, lnum;
    FT.textExtent(text, &lwid, &lhei, &lnum);
    *x = (int)(lwid*rz_text_scale);
    *y = (int)(lhei*rz_text_scale);
}

