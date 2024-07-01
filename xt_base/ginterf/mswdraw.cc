
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

#ifdef WIN32

#include "graphics.h"
#include "grfont.h"
#include "mswdraw.h"
#include "miscutil/texttf.h"
#include "miscutil/lstring.h"
#include <math.h>


//
// A drawing class for native MS Windows, for use stand-alone in the
// xdraw interface and print driver under MS Windows.
//
// This is a simplified set imported from the old MSW interface.
//
// These functions generally assume that each window have a CLASSDC or
// OWNDC, since the DC's are never released in the drawing functions.
// However, a regular DC can be used if each set of operations is enclosed
// in InitDC/ReleaseDC.
//
// Here we have the case where several windows have equivalent drawing
// attributes.  Using an OWNDC for each window requires having to set an
// attribute for each DC, so that something like changing colors becomes
// expensive.  Instead, we use a CLASSDC, but then the DC must be reset
// after each window change by calling GetDC().  This is handled by the
// function below, which is called every time a DC is used in rendering.
// GetDC does not appear to be too expensive, but I hate to call it for
// every rendered object.

HDC MSWdraw::md_lastDC = 0;
HWND MSWdraw::md_lastHWND = 0;


MSWdraw::MSWdraw()
{
    md_gbag = new sGbagMsw;
    md_window = 0;
    md_memDC = 0;
    md_ghost_foreg = RGB(255, 255, 255);
    md_blackout = false;
}


MSWdraw::~MSWdraw()
{
    delete md_gbag;
}


HDC
MSWdraw::check_dc()
{
    if (md_memDC)
        return (md_memDC);
    if (md_window && md_window != md_lastHWND) {
        md_lastHWND = md_window;
        md_lastDC = GetDC(md_window);
    }
    return (md_lastDC);
}


// This should be called once for each DC class, or ahead of a series of
// operations on an ordinary DC.
//
HDC
MSWdraw::InitDC(HDC ndc)
{
    if (!md_window && !ndc)
        return (0);
    if (md_memDC && ndc != md_memDC)
        return (0);

    HDC dc = ndc ? ndc : GetDC(md_window);
    md_lastHWND = md_window;
    md_lastDC = dc;
    SetBkColor(dc, RGB(0, 0, 0));
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkMode(dc, TRANSPARENT);
    HFONT hft;
    if (Fnt()->getFont(&hft, FNT_SCREEN))
        SelectFont(dc, hft);
    SetTextAlign(dc, TA_LEFT | TA_BOTTOM | TA_NOUPDATECP);
    return (md_lastDC);
}


HDC
MSWdraw::SetMemDC(HDC dcMem)
{
    md_lastDC = 0;
    md_lastHWND = 0;
    HDC tdc = md_memDC;
    md_memDC = dcMem;
    return (tdc);
}


// If the window does not have CS_OWNDC or CS_CLASSDC, one can still use
// these functions, by calling ReleaseDC after a series of operations
// to the *same* window.
//
void
MSWdraw::ReleaseDC()
{
    if (md_lastDC) {
        HPEN pen = SelectPen(md_lastDC, GetStockObject(BLACK_PEN));
        if (pen)
            DeletePen(pen);
        HBRUSH brush = SelectBrush(md_lastDC, GetStockObject(BLACK_BRUSH));
        if (brush)
            DeleteBrush(brush);
        ::ReleaseDC(md_lastHWND, md_lastDC);
    }
    md_lastDC = 0;
    md_lastHWND = 0;
}


// Clear the drawing window.
//
void
MSWdraw::Clear()
{
    RECT r;
    GetClientRect(md_window, &r);
    r.right++;
    r.bottom++;
    HDC dc = check_dc();
    HBRUSH brush = (HBRUSH)GetClassLongPtr(md_window, GCLP_HBRBACKGROUND);
    if (!brush) {
        brush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
        FillRect(dc, &r, brush);
        DeleteBrush(brush);
    }
    else
        FillRect(dc, &r, brush);
}


// Draw a pixel at x, y.
//
void
MSWdraw::Pixel(int x, int y)
{
    SetPixelV(check_dc(), x, y, md_foreg);
}


// Draw multiple pixels from list.
//
void
MSWdraw::Pixels(GRmultiPt *data, int n)
{
    // This seems a bit faster than SetPixelV
    HDC dc = check_dc();
    POINT *p = (POINT*)data->data();
    unsigned rop = PATCOPY;
    if (md_gbag->get_fillpattern() && md_gbag->get_fillpattern()->xPixmap()) {
        HBRUSH brush = CreateSolidBrush(md_foreg);
        brush = SelectBrush(dc, brush);
        for (int i = 0; i < n; i++) {
            if (!i || p->x != (p-1)->x || p->y != (p-1)->y)
                PatBlt(dc, p->x, p->y, 1, 1, rop);
            p++;
        }
        brush = SelectBrush(dc, brush);
        DeleteBrush(brush);
    }
    else {
        for (int i = 0; i < n; i++) {
            if (!i || p->x != (p-1)->x || p->y != (p-1)->y)
                PatBlt(dc, p->x, p->y, 1, 1, rop);
            p++;
        }
    }
}


// Define this to generate patterned lines here, rather than using
// Microsoft's pen attributes.
//
#define USE_MY_LINES

// Draw line function.
//
void
MSWdraw::Line(int x1, int y1, int x2, int y2)
{
    LinePrv(x1, y1, x2, y2);
}


// Actual line drawing code.  We draw our own patterned lines, since
// Microsoft makes this too difficult and slow.
//
void
MSWdraw::LinePrv(int x1, int y1, int x2, int y2)
{
    HDC dc = check_dc();
#ifdef USE_MY_LINES
    if (md_gbag->get_linestyle() && md_gbag->get_linestyle()->length > 1) {

        GRmultiPt pts(2048);
        int pcnt = 0;

        if (x2 < x1) {
            int t = x1; x1 = x2; x2 = t;
            t = y1; y1 = y2; y2 = t;
        }
        int dx = x2 - x1;
        int dy = y2 - y1;
        if (y1 > y2)
            dy = -dy;
        int dy2 = dy;

        // Set up linestyle mask from dash data
        unsigned int linestyle = 0;
        unsigned int bit = 1;
        bool on = true;
        int len = 0;
        for (int i = 0; i < md_gbag->get_linestyle()->length; i++) {
            for (int j = 0; j < md_gbag->get_linestyle()->dashes[i]; j++) {
                if (on)
                    linestyle |= bit;
                bit <<= 1;
                len++;
            }
            on = !on;
        }

        if (md_gbag->get_linestyle()->offset) {
            int os = md_gbag->get_linestyle()->offset;
            while (os--) {
                linestyle <<= 1;
                if (linestyle & bit) {
                    linestyle &= ~bit;
                    linestyle |= 1;
                }
            }
        }

        unsigned int end = bit;
        // set pattern origin of Manhattan lines
        if (y1 == y2)
            bit = 1 << (x1 % len);
        else if (x1 == x2)
            bit = 1 << (y1 % len);
        else
            bit = 1;

        // Spit out the pixels
        int sn = y2 > y1 ? 1 : -1;
        int errterm = -dx >> 1;
        for (dy++; dy; dy--, y1 += sn) {
            errterm += dx;
            if (errterm <= 0) {
                if (bit & linestyle) {
                    if (pcnt == 2048) {
                        Pixels(&pts, pcnt);
                        pcnt = 0;
                    }
                    pts.assign(pcnt, x1, y1);
                    pcnt++;
                }
                bit <<= 1;
                if (bit >= end)
                    bit = 1;
                continue;
            }
            while (errterm > 0 && x1 != x2) {
                if (bit & linestyle) {
                    if (pcnt == 2048) {
                        Pixels(&pts, pcnt);
                        pcnt = 0;
                    }
                    pts.assign(pcnt, x1, y1);
                    pcnt++;
                }
                x1++;
                errterm -= dy2;
                bit <<= 1;
                if (bit >= end)
                    bit = 1;
            }
        }
        if (pcnt)
            Pixels(&pts, pcnt);
        return;
    }
#endif

    MoveToEx(dc, x1, y1, 0);
    LineTo(dc, x2, y2);
}


// Draw a path.
//
void
MSWdraw::PolyLine(GRmultiPt *data, int n)
{
    if (n < 2)
        return;
#ifdef USE_MY_LINES
    if (md_gbag->get_linestyle() && md_gbag->get_linestyle()->length > 1) {
        n--;
        data->data_ptr_init();
        while (n--) {
            int x = data->data_ptr_x();
            int y = data->data_ptr_y();
            data->data_ptr_inc();
            Line(x, y, data->data_ptr_x(), data->data_ptr_y());
        }
        return;
    }
#endif
    ::Polyline(check_dc(), (POINT*)data->data(), n);
}


// Draw a collection of lines.
//
void
MSWdraw::Lines(GRmultiPt *data, int n)
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


// Draw a filled box.
//
void
MSWdraw::Box(int x1, int y1, int x2, int y2)
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

    x2++;
    y2++;

    if (md_gbag->get_fillpattern() && md_gbag->get_fillpattern()->xPixmap()) {
        HDC dc = check_dc();
        SetTextColor(dc, 0);
        COLORREF bg = SetBkColor(dc, RGB(255, 255, 255));
        // D <- P & D
        PatBlt(dc, x1, y1, x2-x1, y2-y1, 0xa000c9);
        SetBkColor(dc, 0);
        SetTextColor(dc, md_foreg);
        // D <- P | D
        PatBlt(dc, x1, y1, x2-x1, y2-y1, 0xfa0089);
        SetBkColor(dc, bg);
    }
    else
        PatBlt(check_dc(), x1, y1, x2-x1, y2-y1, PATCOPY);
}


// Draw a collection of filled boxes.
//
void
MSWdraw::Boxes(GRmultiPt *data, int n)
{
    // order: x, y, width, height
    HDC dc = check_dc();
    if (md_gbag->get_fillpattern() && md_gbag->get_fillpattern()->xPixmap()) {

        SetTextColor(dc, 0);
        COLORREF bg = SetBkColor(dc, RGB(255, 255, 255));
        POINT *p = (POINT*)data->data();
        for (int i = 0; i < n; i++) {
            // D <- P & D
            PatBlt(dc, p->x, p->y, (p+1)->x, (p+1)->y, 0xa000c9);
            p += 2;
        }
        SetBkColor(dc, 0);
        SetTextColor(dc, md_foreg);
        p = (POINT*)data->data();
        for (int i = 0; i < n; i++) {
            // D <- P | D
            PatBlt(dc, p->x, p->y, (p+1)->x, (p+1)->y, 0xfa0089);
            p += 2;
        }
        SetBkColor(dc, bg);
    }
    else {
        POINT *p = (POINT*)data->data();
        for (int i = 0; i < n; i++) {
            PatBlt(dc, p->x, p->y, (p+1)->x, (p+1)->y, PATCOPY);
            p += 2;
        }
    }
}


namespace {
    inline int
    rnd(double z)
    {
        return ((int)(z >= 0.0 ? z + 0.5 : z - 0.5));
    }
}


// Draw an arc.
//
void
MSWdraw::Arc(int x0, int y0, int rx, int ry, double theta1, double theta2)
{

    if (rx <= 0 || ry <= 0)
        return;
    if (theta1 >= theta2)
        theta2 = 2 * M_PI + theta2;
    int xe1 = x0 + rnd(rx*cos(theta1));
    int ye1 = y0 - rnd(ry*sin(theta1));
    int xe2 = x0 + rnd(rx*cos(theta2));
    int ye2 = y0 - rnd(ry*sin(theta2));
    ::Arc(check_dc(), x0 - rx, y0 - ry, x0 + rx, y0 + ry,
        xe1, ye1, xe2, ye2);
}


// Draw a filled polygon.
//
void
MSWdraw::Polygon(GRmultiPt *data, int numv)
{
    HRGN rgn = CreatePolygonRgn((POINT*)data->data(), numv, WINDING);
    if (md_gbag->get_fillpattern() && md_gbag->get_fillpattern()->xPixmap()) {
        HDC dc = check_dc();
        SetTextColor(dc, 0);
        COLORREF bg = SetBkColor(dc, RGB(255, 255, 255));
        // D <- P & D
        SetROP2(dc, R2_MASKPEN);
        PaintRgn(dc, rgn);
        SetBkColor(dc, 0);
        SetTextColor(dc, md_foreg);
        // D <- P | D
        SetROP2(dc, R2_MERGEPEN);
        PaintRgn(dc, rgn);
        SetBkColor(dc, bg);
        SetROP2(dc, R2_COPYPEN);
    }
    else
        PaintRgn(check_dc(), rgn);
    DeleteRgn(rgn);
}


// Draw a trapezoid.
void
MSWdraw::Zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
{
    if (yl >= yu)
        return;
    POINT points[5];
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
    if (n < 4)
        return;
    HRGN rgn = CreatePolygonRgn(points, n, WINDING);
    if (md_gbag->get_fillpattern() && md_gbag->get_fillpattern()->xPixmap()) {
        HDC dc = check_dc();
        SetTextColor(dc, 0);
        COLORREF bg = SetBkColor(dc, RGB(255, 255, 255));
        // D <- P & D
        SetROP2(dc, R2_MASKPEN);
        PaintRgn(dc, rgn);
        SetBkColor(dc, 0);
        SetTextColor(dc, md_foreg);
        // D <- P | D
        SetROP2(dc, R2_MERGEPEN);
        PaintRgn(dc, rgn);
        SetBkColor(dc, bg);
        SetROP2(dc, R2_COPYPEN);
    }
    else
        PaintRgn(check_dc(), rgn);
    DeleteRgn(rgn);
}


// Render text.  The x and y are the LOWER left corner of text
// Transformations are not supported.
//
void
MSWdraw::Text(const char *str, int x, int y, int xform, int, int)
{
    if (!str)
        return;
    // Replace non-printing control characters with a black dot,
    // replace '\n' with a paragraph symbol.
    char *text = lstring::copy(str);
    for (char *t = text; *t; t++) {
        if (*t < ' ') {
            if (*t == '\n')
                *t = 182;  // paragraph symbol
            else
                *t = 149;  // bullet char
        }
    }
    HDC dc = check_dc();
    if (xform & TXTF_HJR) {
        SIZE size;
        GetTextExtentPoint32(dc, text, strlen(text), &size);
        x -= size.cx;
    }
    else if (xform & TXTF_HJC) {
        SIZE size;
        GetTextExtentPoint32(dc, text, strlen(text), &size);
        x -= size.cx/2;
    }

    SetBkMode(dc, TRANSPARENT);
    TextOut(dc, x, y, text, strlen(text));
    delete [] text;
}


// Return the width/height of text.  If text is 0, return
// the max bounds of any character.
//
void
MSWdraw::TextExtent(const char *text, int *wid, int *hei)
{
    if (!text || !*text)
        text = "x";
    SIZE size;
    GetTextExtentPoint32(check_dc(), text, strlen(text), &size);
    if (wid)
        *wid = size.cx;
    if (hei)
        *hei = size.cy;
}


// Define a new rgb value for pixel (if read/write cell) or return a
// new pixel with a matching color.
//
void
MSWdraw::DefineColor(int *address, int red, int green, int blue)
{
    if (md_blackout) {
        *address = 0;
        return;
    }
    *address = RGB(red, green, blue);
    SetColor(*address);
}


// Set the text background.
//
void
MSWdraw::SetBackground(int pixel)
{
    md_backg = pixel;
    SetBkColor(check_dc(), pixel);
}


// Actually change the drwaing window background.
// THIS CHANGES ALL WINDOWS IN THE CLASS
//
void
MSWdraw::SetWindowBackground(int pixel)
{
    HBRUSH brush = CreateSolidBrush(pixel);
    brush = (HBRUSH)SetClassLongPtr(md_window, GCLP_HBRBACKGROUND,
        (long long)brush);
    if (brush)
        DeleteBrush(brush);
}


// Set the current foreground color.
//
void
MSWdraw::SetColor(int pixel)
{
    if (md_blackout)
        return;
    md_foreg = pixel;
    HDC dc = check_dc();
    SetTextColor(dc, pixel);
    HPEN pen = new_pen(pixel, md_gbag->get_linestyle());
    pen = SelectPen(dc, pen);
    if (pen)
        DeletePen(pen);
    if (!md_gbag->get_fillpattern() ||
            !md_gbag->get_fillpattern()->xPixmap()) {
        HBRUSH brush = CreateSolidBrush(pixel);
        brush = SelectBrush(dc, brush);
        if (brush)
            DeleteBrush(brush);
    }
}


// Set the current linestyle.
//
void
MSWdraw::SetLinestyle(const GRlineType *lineptr)
{
    md_gbag->set_linestyle(lineptr);
#ifndef USE_MY_LINES
    HPEN pen = new_pen(md_gbag->get_foreg_pixel(), lineptr);
    pen = SelectPen(check_dc(), pen);
    if (pen)
        DeletePen(pen);
#endif
}


namespace {
    // Reverse bit order and complement.
    //
    unsigned int
    revnotbits(unsigned char c)
    {
        unsigned char out = 0;
        for (int i = 0;;) {
            if (!(c & 1))
                out |= 1;
            i++;
            if (i == 8)
                break;
            c >>= 1;
            out <<= 1;
        }
        return (out);
    }
}


// Create a new pixmap for the fill pattern.
//
void
MSWdraw::DefineFillpattern(GRfillType *fillp)
{
    if (!fillp)
        return;
    if (fillp->xPixmap()) {
        DeleteBitmap((HBITMAP)fillp->xPixmap());
        fillp->setXpixmap(0);
    }
    if (fillp->hasMap()) {
        // 0 -> text fg color, 1 -> text bg color, so invert map pixels
        int bpl = (fillp->nX() + 7)/8;
        unsigned char *map = fillp->newBitmap();
        unsigned short *invmap =
            new unsigned short[fillp->nY()*(bpl > 2 ? 2:1)];
        if (fillp->nX() <= 16) {
            for (unsigned int i = 0; i < fillp->nY(); i++) {
                unsigned short c = revnotbits(map[i*bpl]);
                if (bpl > 1)
                    c |= revnotbits(map[i*bpl + 1]) << 8;
                invmap[i] = c;
            }
        }
        else {
            for (unsigned int i = 0; i < fillp->nY(); i++) {
                unsigned short c = revnotbits(map[i*bpl]);
                c |= revnotbits(map[i*bpl + 1]) << 8;
                invmap[2*i] = c;
                c = revnotbits(map[i*bpl + 2]);
                if (bpl > 3)
                    c |= revnotbits(map[i*bpl + 3]) << 8;
                invmap[2*i + 1] = c;
            }
        }
        fillp->setXpixmap((GRfillData)CreateBitmap(fillp->nX(), fillp->nY(),
            1, 1, invmap));
        delete [] invmap;
        delete [] map;
    }
}


// Set the current fill pattern.
//
void
MSWdraw::SetFillpattern(const GRfillType *fillp)
{
    md_gbag->set_fillpattern(fillp);
    HBRUSH brush;
    if (!fillp || !fillp->xPixmap())
        brush = CreateSolidBrush(md_foreg);
    else
        brush = CreatePatternBrush((HBITMAP)fillp->xPixmap());
    brush = SelectBrush(check_dc(), brush);
    if (brush)
        DeleteBrush(brush);
}


// Update the display.
//
void
MSWdraw::Update()
{
    GdiFlush();
}


// Show a glyph (from the list).
//

#define GlyphWidth 7

struct stmap
{
    HBITMAP pmap;
    char bits[GlyphWidth];
};

namespace {
    stmap glyphs[] =
    {
       { 0, {0x00, 0x1c, 0x22, 0x22, 0x22, 0x1c, 0x00} }, // circle
       { 0, {0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41} }, // cross (x)
       { 0, {0x08, 0x14, 0x22, 0x41, 0x22, 0x14, 0x08} }, // diamond
       { 0, {0x08, 0x14, 0x14, 0x22, 0x22, 0x41, 0x7f} }, // triangle
       { 0, {0x7f, 0x41, 0x22, 0x22, 0x14, 0x14, 0x08} }, // inverted triangle
       { 0, {0x7f, 0x41, 0x41, 0x41, 0x41, 0x41, 0x7f} }, // square
       { 0, {0x00, 0x00, 0x1c, 0x14, 0x1c, 0x00, 0x00} }, // dot
       { 0, {0x08, 0x08, 0x08, 0x7f, 0x08, 0x08, 0x08} }, // cross (+)
    };
}

// Some ROP3's
#define COPYFG 0x00ca0749  // dst = (pat & src) | (!pat & dst)
#define COPYBG 0x00ac0744  // dst = (!pat & src) | (pat & dst)


void
MSWdraw::ShowGlyph(int gnum, int x, int y)
{
    x -= GlyphWidth/2;
    y -= GlyphWidth/2;
    gnum = gnum % (sizeof(glyphs)/sizeof(stmap));
    stmap *st = &glyphs[gnum];
    if (!st->pmap) {
        unsigned short invmap[8];
        for (int i = 0; i < GlyphWidth; i++)
            invmap[i] = revnotbits(st->bits[i]);
        invmap[7] = 0xffff;
        st->pmap = CreateBitmap(8, 8, 1, 1, (char*)invmap);
    }
    HBRUSH brush = CreatePatternBrush(st->pmap);
    brush = SelectBrush(check_dc(), brush);

    HDC dcMem = CreateCompatibleDC(check_dc());
    HBITMAP bm = CreateCompatibleBitmap(check_dc(), 8, 8);
    bm = (HBITMAP)SelectObject(dcMem, bm);
    SetBkColor(dcMem, md_foreg);
    RECT r;
    r.left = r.top = 0;
    r.right = r.bottom = GlyphWidth;
    ExtTextOut(dcMem, 0, 0, ETO_OPAQUE, &r, 0, 0, 0);
    SetTextColor(check_dc(), RGB(255,255,255));
    SetBrushOrgEx(check_dc(), x, y, 0);
    BitBlt(check_dc(), x, y, 8, 8, dcMem, 0, 0, COPYFG);
    SetBrushOrgEx(check_dc(), 0, 0, 0);
    SetTextColor(check_dc(), md_foreg);
    bm = (HBITMAP)SelectObject(dcMem, bm);
    DeleteBitmap(bm);
    DeleteDC(dcMem);
    brush = SelectBrush(check_dc(), brush);
    DeleteBrush(brush);
}


GRobject
MSWdraw::GetRegion(int x, int y, int wid, int hei)
{
    HDC dcMem = CreateCompatibleDC(check_dc());
    HBITMAP bm = CreateCompatibleBitmap(check_dc(), wid, hei);
    SelectBitmap(dcMem, bm);
    BitBlt(dcMem, 0, 0, wid, hei, check_dc(), x, y, SRCCOPY);
    return (dcMem);
}


void
MSWdraw::PutRegion(GRobject pm, int x, int y, int wid, int hei)
{
    BitBlt(check_dc(), x, y, wid, hei, (HDC)pm, 0, 0, SRCCOPY);
}


void
MSWdraw::FreeRegion(GRobject pm)
{
    HDC dcMem = (HDC)pm;
    HBITMAP bm = (HBITMAP)GetCurrentObject(dcMem, OBJ_BITMAP);
    DeleteBitmap(bm);
    DeleteDC(dcMem);
}


void
MSWdraw::DisplayImage(const GRimage *image, int x, int y,
    int width, int height)
{
    // custom BITMAPINFO
    // We need to use BI_BITFIELDS and the three DWORD masks since our
    // bitmap is RGBA and BI_RGB assumes BGRA.
    struct bminfo
    {
        BITMAPINFOHEADER bmiHeader;
        unsigned long ct[3];
    };

    bminfo binfo;
    BITMAPINFOHEADER *h = &binfo.bmiHeader;
    h->biSize = sizeof(BITMAPINFOHEADER);
    h->biWidth = image->width();
    h->biHeight = -image->height();  // Negative to flip y coords.
    h->biPlanes = 1;
    h->biBitCount = 32;
    h->biCompression = BI_BITFIELDS;
    h->biSizeImage = 0;
    h->biXPelsPerMeter = 0;
    h->biYPelsPerMeter = 0;
    h->biClrUsed = 0;
    h->biClrImportant = 0;
    // These effectively swap the red and blue pixel values, as our source
    // is out of order for this function (Thanks Bill).
    binfo.ct[0] = 0x000000ff;
    binfo.ct[1] = 0x0000ff00;
    binfo.ct[2] = 0x00ff0000;

    int k = SetDIBitsToDevice(
        check_dc(),                     // destination DC
        x,                              // destination x
        y,                              // destination y
        width,                          // image width
        height,                         // image height
        x,                              // source x
        image->height() - y - height,   // source y
        0,                              // starting scanline of source data
        image->height(),                // source array height
        image->data(),                  // source data
        (BITMAPINFO*)&binfo,
        DIB_RGB_COLORS );

   if (k == 0)
       fprintf(stderr, "Error: SetDIBitsToDevice returned %d.\n",
           (int)GetLastError());


/*------ Wrong, it is possible to avoid copy.
    // It doesn't seem possible to use SetDIBitsForDevice directly from
    // the image data.  Create an intermediate bitmap, note that the y
    // ordering is reversed.

    unsigned int *ary = new unsigned int[width*height];
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
            unsigned int p = image->data()[xd + yd*image->width()];
            ary[(height - 1 - i)*width + j] = p;
        }
    }

    // custom BITMAPINFO
    // We need to use BI_BITFIELDS and the three DWORD masks since our
    // bitmap is RGBA and BI_RGB assumes BGRA.
    struct bminfo
    {
        BITMAPINFOHEADER bmiHeader;
        unsigned long ct[3];
    };

    bminfo binfo;
    BITMAPINFOHEADER *h = &binfo.bmiHeader;
    h->biSize = sizeof(BITMAPINFOHEADER);
    h->biWidth = width;
    h->biHeight = height;
    h->biPlanes = 1;
    h->biBitCount = 32;
    h->biCompression = BI_BITFIELDS;
    h->biSizeImage = 0;
    h->biXPelsPerMeter = 0;
    h->biYPelsPerMeter = 0;
    h->biClrUsed = 0;
    h->biClrImportant = 0;
    // These effectively swap the red and blue pixel values, as our source
    // is out of order for this function (Thanks Bill).
    binfo.ct[0] = 0x000000ff;
    binfo.ct[1] = 0x0000ff00;
    binfo.ct[2] = 0x00ff0000;

    int k = SetDIBitsToDevice(
        check_dc(),
        x,
        y,
        width,
        height,
        0, // XSrc
        0, // YSrc
        0, // StartScan
        height, // ScanLines
        ary,
        (BITMAPINFO*)&binfo,
        DIB_RGB_COLORS );

   delete [] ary;

   if (k == 0)
       fprintf(stderr, "Error: SetDIBitsToDevice returned %d.\n",
           (int)GetLastError());
------- */

/*------ Pathetic naive slow method.
    HDC dc = check_dc();
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
            unsigned int p = image->data()[xd + yd*image->width()];
            SetPixelV(dc, xd, yd,
                RGB(p & 0xff, (p >> 8) & 0xff, (p >> 16) & 0xff));
        }
    }
------- */

}


// Static function.
HPEN
MSWdraw::new_pen(COLORREF cref, const GRlineType *lineptr)
{
#ifdef USE_MY_LINES
    (void)lineptr;
    return (CreatePen(PS_SOLID, 0, cref));
#else
    HPEN pen;
    if (lineptr == 0 || lineptr->length < 2)
        pen = CreatePen(PS_SOLID, 0, cref);
    else {
        // More Microsoft Manure:  A cosmetic pen draws quidkly, but the
        // minimum dash unit is a "style unit" of three pixels.  A geometric
        // pen is 3-10 times slower, but the minimum unit seems to be one
        // pixel, but it is difficult to tell since horizontal and vertical
        // lines look different (the vertical lines appear solid on my
        // monitor for small gaps).  Here we try to be a little clever in
        // choosing the "best" line type.

        LOGBRUSH lb;
        lb.lbStyle = BS_SOLID;
        lb.lbColor = cref;
        DWORD st[8];
        for (int i = 0; i < lineptr->length; i++)
            st[i] = lineptr->dashes[i];
        int k;
        for (k = 0; k < lineptr->length; k++)
            if (st[k] != 1)
                break;
        if (k == lineptr->length)
            // use "alternate" mode
            return (ExtCreatePen(PS_COSMETIC | PS_ALTERNATE, 1, &lb, 0, 0));
        for (k = 0; k < lineptr->length; k++)
            if (st[k] % 3)
                break;
        if (k == lineptr->length) {
            // each dash is divisible by one style unit, rescale and use
            // a cosmetic pen
            for (k = 0; k < lineptr->length; k++)
                st[k] /= 3;
            return (ExtCreatePen(PS_COSMETIC | PS_USERSTYLE, 1, &lb,
                lineptr->length, st));
        }
        // use a (slow) geometric pen
        pen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 0, &lb,
            lineptr->length, st);
    }
    return (pen);
#endif
}
// End of MSWdraw functions

#endif

