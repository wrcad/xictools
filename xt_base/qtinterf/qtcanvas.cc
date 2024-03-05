
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qtcanvas.h"
#include <QResizeEvent>
#include <QPaintEvent>
#include <QBitmap>
#include <QPainter>
#include <QEnterEvent>
#include <QFocusEvent>
#include <QWheelEvent>
#include <QGuiApplication>


using namespace qtinterf;

QTcanvas::QTcanvas(QWidget *prnt) : QWidget(prnt)
{
    setMouseTracking(true);

    da_pixmap = new QPixmap(1, 1);
    da_overlay_bg = 0;
    da_pixmap2 = 0;
    da_pixmap_bak = 0;
    da_tile_pixmap = 0;
    da_painter = new QPainter(da_pixmap);
    da_painter2 = 0;
    da_painter_bak = 0;
    da_tile_x = 0;
    da_tile_y = 0;
    da_fill_mode = false;
    da_ghost_bg_set = false;
    da_overlay_count = 0;
    da_line_mode = 0;
    da_line_style = 0;
    da_xb1 = 0;
    da_yb1 = 0;
    da_xb2 = 0;
    da_yb2 = 0;
    da_olx = 0;
    da_oly = 0;
    da_olw = 0;
    da_olh = 0;
    da_call_count = 0;

    // Ghost drawing.
    da_ghost_overlay_bg = 0;
    da_ghost_draw_ptr = &da_local;
    da_local.gd_windows[0] = this;

    da_bg.setNamedColor(QString("white"));
    da_fg.setNamedColor(QString("black"));
    da_brush.setStyle(Qt::SolidPattern);
    da_pen.setStyle(Qt::NoPen);
    initialize();
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAutoFillBackground(true);
}


QTcanvas::~QTcanvas()
{
    if (da_painter->isActive())
        da_painter->end();
    delete da_pixmap;
    delete da_painter;
    delete da_overlay_bg;
    delete da_ghost_overlay_bg;
    if (da_pixmap_bak) {
        if (da_painter_bak->isActive())
            da_painter_bak->end();
        delete da_pixmap_bak;
        delete da_painter_bak;
    }
    else if (da_pixmap2) {
        if (da_painter2->isActive())
            da_painter2->end();
        delete da_pixmap2;
        delete da_painter2;
    }
}


//
// The drawing interface.
//


// Switch the drawing context to the second pixmap.  If there is no
// second pixmap, it will be set to the argument, or created if the
// argument is null.  Once set, the pixmap2 is retained until
// explicitly freed with clear_pixmap2, after which pixmap2 can be
// reset with this function.
//
void
QTcanvas::switch_to_pixmap2(QPixmap *pixmp)
{
    if (da_pixmap_bak) {
        // Already switched.
        return;
    }
    if (!da_pixmap2) {
        if (pixmp)
            da_pixmap2 = pixmp;
        else
            da_pixmap2 = new QPixmap(da_pixmap->size());
        da_painter2 = new QPainter(da_pixmap2);
    }
    da_pixmap_bak = da_pixmap;
    da_pixmap = da_pixmap2;
    da_painter_bak = da_painter;
    da_painter = da_painter2;
    da_painter->setBrush(da_brush);
    da_painter->setPen(da_pen);
    bb_init();
    emit new_painter(da_painter);
}


// Switch back to the main pixmap and copy the given region from the
// second pixmap, update the screen.  The second pixmap is retained
// for future use.
//
void
QTcanvas::switch_from_pixmap2(int xd, int yd, int xs, int ys, int w, int h)
{
    if (!da_pixmap_bak) {
        // Already switched back.
        return;
    }
    da_pixmap = da_pixmap_bak;
    da_pixmap_bak = 0;
    da_painter = da_painter_bak;
    da_painter_bak = 0;
    da_painter->setBrush(da_brush);
    da_painter->setPen(da_pen);
    bb_init();
    emit new_painter(da_painter);
    da_painter->drawPixmap(xd, yd, *da_pixmap2, xs, ys, w, h);
    repaint(xd, yd, w, h);
}


// Reset to the main pixmap if necessary, then destroy and clear the
// second pixmap and its painter.  There is no pixmap copy here.
//
void
QTcanvas::clear_pixmap2()
{
    set_draw_to_pixmap(0);
    if (da_painter2) {
        da_painter2->end();
        delete da_painter2;
        da_painter2 = 0;
    }
    if (da_pixmap2) {
        delete da_pixmap2;
        da_pixmap2 = 0;
    }
}


// Switch the drawing context to the supplied pixmap, or back to the
// main pixmap if 0 is passed.  When 0 is passed the pixmap2 is
// cleared but not destroyed.  The caller is responsible for pixmp
// cleanup when using this function.
//
void
QTcanvas::set_draw_to_pixmap(QPixmap *pixmp)
{
    if (pixmp)
        switch_to_pixmap2(pixmp);
    else {
        if (!da_pixmap_bak) {
            // Already switched back.
            return;
        }
        da_pixmap = da_pixmap_bak;
        da_pixmap_bak = 0;
        da_painter = da_painter_bak;
        da_painter_bak = 0;
        da_painter->setBrush(da_brush);
        da_painter->setPen(da_pen);
        bb_init();
        emit new_painter(da_painter);
        if (da_painter2) {
            da_painter2->end();
            delete da_painter2;
            da_painter2 = 0;
        }
        da_pixmap2 = 0;
    }
}


/******************************************************************************

Theory of overlays.

There are three drawing levels, from the bottom up.  The lowest
"base" level contains the complex drawing, that is likely expensive
to render.  Above this is the "annotation" overlay which may
contain circles and arrows and text.  These come and go, and we can
erase these objects efficiently by copying up the base level
pixmap.  Above the annotation is an overlay for "ghost" objects,
which are animations, generally attached to the mouse cursor.  The
background generated from the annotation level is used to
efficiently erase the ghosts.

The main context element is the background pixmap.  This must be
switched between the three sources before drawing into an overlay. 
The logic for this may be a bit tricky.

When a new scene is generated, the base and annotation backgrounds are
generated:

compose base
create base background
draw annotation
create annotation background.

The ghost background is computed when a ghost-drawing mode is entered,
in graphics code, not in the application.

The base background is created whenever the window is redrawn from
scratch, after a mode change of some sort.  There is likely one place
in the application where this is done.  In this same location, after
default annotation is applied, the annotation background can be created.
The annotation backgound may subsequently change as the annotation
changes within the application.  This must be implementated in
application code.

******************************************************************************/

void
QTcanvas::set_overlay_mode(bool set)
{
    if (set) {
        if (da_overlay_count == DA_MAX_OVERLAY) {
            fprintf(stderr, "Error: QTcanvas::set_overlay_mode call overflow\n");
            return;
        }

        // This should have been called already, but if not do it here.
        if (!da_overlay_bg)
            create_overlay_backg();

        // Set up to accumulate the bounding box of rendered area
        // while in overlay mode.  This area will be used to update
        // the screen.

        if (da_overlay_count == 0)
            bb_init();
        da_overlay_count++;
    }
    else {
        if (!da_overlay_count)
            return;

        // Switch out of overlay mode and repaint the area modified
        // while in overlay mode.

        da_overlay_count--;
        if (da_overlay_count == 0) {
            da_olx = da_xb1;
            da_oly = da_yb1;
            da_olw = da_xb2-da_xb1+1;
            da_olh = da_yb2-da_yb1+1;
            if (da_olw > 0 && da_olh > 0) {
                repaint(da_olx, da_oly, da_olw, da_olh);
            }
            bb_init();
        }
    }
}


void
QTcanvas::create_overlay_backg()
{
    // Make sure that the overlay_bg is size consistent with the main
    // pixmap and copy the main pixmap into it.  The user should call
    // this before entering overlay mode.

    if (da_overlay_bg && (da_overlay_bg->size() != da_pixmap->size())) {
        delete da_overlay_bg;
        da_overlay_bg = 0;
    }
    if (!da_overlay_bg)
        da_overlay_bg = new QPixmap(da_pixmap->size());
    QPainter painter(da_overlay_bg);
    painter.drawPixmap(0, 0, *da_pixmap, 0, 0,
        da_pixmap->width(), da_pixmap->height());
}


// Erase the accumulated drawn area of the recently completed
// overlay drawing by copying in the backing pixmap.
//
void
QTcanvas::erase_last_overlay()
{
    if (da_overlay_count && da_overlay_bg && da_olw > 0 && da_olh > 0) {
        da_painter->drawPixmap(da_olx, da_oly, da_olw, da_olh,
            *da_overlay_bg, da_olx, da_oly, da_olw, da_olh);
        bb_add(da_olx, da_oly);
        bb_add(da_olx + da_olw, da_oly + da_olh);
    }
}


void
QTcanvas::set_clipping(int xx, int yy, int w, int h)
{
    // Set a clip rectangle in the main pixmap painter.  Call with
    // zero w and h to end clipping.

    if (w > 0 && h > 0) {
        da_painter->setClipRect(QRectF(xx, yy, w, h));
        da_painter->setClipping(true);
    }
    else
        da_painter->setClipping(false);
}


// Copy the pixmap2 area into the main pixmap.
//
void
QTcanvas::refresh(int xx, int yy, int w, int h)
{
    if (da_pixmap_bak || !da_pixmap2) {
        // Do this in main mode only.
        return;
    }
    da_painter->drawPixmap(xx, yy, *da_pixmap2, xx, yy, w, h);
}


// If not in overlay mode paint the screen window from the main
// pixmap.  When in overlay mode, instead copy the overlay background
// into the main pixmap, effecting erasure of the overlay drawing in
// the given area.
//
void
QTcanvas::update(int xx, int yy, int w, int h)
{
    if (da_overlay_count) {
        if (da_overlay_bg) {
            da_painter->drawPixmap(xx, yy, w, h, *da_overlay_bg, xx, yy, w, h);
            bb_add(xx, yy);
            bb_add(xx+w, yy+h);
        }
        return;
    }
    repaint(xx, yy, w, h);
}


// Flood with the background color.
//
void
QTcanvas::clear()
{
    da_brush.setColor(da_bg);
    da_brush.setStyle(Qt::SolidPattern);
    da_painter->setBrush(da_brush);
    da_painter->drawRect(0, 0, width(), height());
    da_brush.setColor(da_fg);
}


void
QTcanvas::clear_area(int x0, int y0, int w, int h)
{
    if (w <= 0)
        w = width() - x0;
    if (h <= 0)
        h = height() - y0;
    da_brush.setColor(da_bg);
    da_brush.setStyle(Qt::SolidPattern);
    da_painter->setBrush(da_brush);
    da_painter->drawRect(x0, y0, w, h);
    da_brush.setColor(da_fg);
}


// Set the foreground rendering color.
//
void
QTcanvas::set_foreground(unsigned int pix)
{
    da_fg.setRgb(pix);
    set_color(da_fg);
}


// Set the background rendering color.
//
void
QTcanvas::set_background(unsigned int pix)
{
    da_bg.setRgb(pix);
    if (da_ghost_draw_ptr->gd_xor_mode)
        da_ghost_fg.setRgb(da_ghost.rgb() ^ da_bg.rgb());
}


// Draw one pixel.
//
void
QTcanvas::draw_pixel(int x0, int y0)
{
    if (da_overlay_count)
        bb_add(x0, y0);
    da_pen.setStyle(Qt::SolidLine);
    da_painter->setPen(da_pen);
    da_painter->drawPoint(x0, y0);
}


// Draw multiple pixels.
//
void
QTcanvas::draw_pixels(GRmultiPt *p, int n)
{
    if (da_overlay_count) {
        p->data_ptr_init();
        int nn = n;
        while (nn--) {
            int xx = p->data_ptr_x();
            int yy = p->data_ptr_y();
            bb_add(xx, yy);
            p->data_ptr_inc();
        }
    }

    QPolygon poly;
    poly.setPoints(n, (int*)p->data());
    da_pen.setStyle(Qt::SolidLine);
    da_painter->setPen(da_pen);
    da_painter->drawPoints(poly);
}


// Set the current line texture, used when da_line_mode = 0;
//
void
QTcanvas::set_linestyle(const GRlineType *lstyle)
{
    da_line_style = lstyle;
}


// Draw line function (private).  Arbitrary pattterned lines are not
// supported in QT.  This function implements patterned line drawing,
// one pixel at a time.  It is faster than I expected.
//
void
QTcanvas::draw_line_prv(int x1, int y1, int x2, int y2)
{
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

    // Set up linestyle mask from dash data.
    unsigned int linestyle = 0;
    unsigned int bit = 1;
    bool on = true;
    int len = 0;
    for (int i = 0; i < da_line_style->length; i++) {
        for ( int j = 0; j < da_line_style->dashes[i]; j++) {
            if (on)
                linestyle |= bit;
            bit <<= 1;
            len++;
        }
        on = !on;
    }

    da_pen.setStyle(Qt::SolidLine);

    if (da_line_style->offset) {
        int os = da_line_style->offset;
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
    int errterm = 0;
    for (dy++; dy; dy--, y1 += sn) {
        errterm += dx;
        if (errterm <= 0) {
            if (bit & linestyle) {
                if (pcnt == 2048) {
                    QPolygon poly;
                    poly.setPoints(pcnt, (int*)pts.data());
                    da_painter->drawPoints(poly);
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
                    QPolygon poly;
                    poly.setPoints(pcnt, (int*)pts.data());
                    da_painter->drawPoints(poly);
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
        bit <<= 1;
        if (bit >= end)
            bit = 1;
    }
    if (pcnt) {
        QPolygon poly;
        poly.setPoints(pcnt, (int*)pts.data());
        da_painter->drawPoints(poly);
    }
}


// Draw a line.
//
void
QTcanvas::draw_line(int x1, int y1, int x2, int y2)
{
    if (da_overlay_count) {
        bb_add(x1, y1);
        bb_add(x2, y2);
    }
    da_call_count++;
    if (da_line_mode)
        da_pen.setStyle((Qt::PenStyle)(da_line_mode + 1));
    else if (!da_line_style || da_line_style->length <= 1)
        da_pen.setStyle(Qt::SolidLine);
    else {
        da_pen.setStyle(Qt::SolidLine);
        da_painter->setPen(da_pen);
        draw_line_prv(x1, y1, x2, y2);
        return;
    }
    da_painter->setPen(da_pen);
    da_painter->drawLine(x1, y1, x2, y2);
    da_pen.setStyle(Qt::NoPen);
    da_painter->setPen(da_pen);
}


// Draw a connected line set.
//
void
QTcanvas::draw_polyline(GRmultiPt *p, int n)
{
    if (n < 2)
        return;

    if (da_overlay_count) {
        p->data_ptr_init();
        int nn = n;
        while (nn--) {
            int xx = p->data_ptr_x();
            int yy = p->data_ptr_y();
            bb_add(xx, yy);
            p->data_ptr_inc();
        }
    }
    da_call_count++;

    if (da_line_mode)
        da_pen.setStyle((Qt::PenStyle)(da_line_mode + 1));
    else if (!da_line_style || da_line_style->length <= 1)
        da_pen.setStyle(Qt::SolidLine);
    else {
        da_pen.setStyle(Qt::SolidLine);
        da_painter->setPen(da_pen);
        p->data_ptr_init();
        n--;
        while (n--) {
            int xx = p->data_ptr_x();
            int yy = p->data_ptr_y();
            p->data_ptr_inc();
            draw_line_prv(xx, yy, p->data_ptr_x(), p->data_ptr_y());
        }
        return;
    }
    QPolygon poly;
    poly.setPoints(n, (int*)p->data());
    da_painter->setPen(da_pen);
    da_painter->drawPolyline(poly);
    da_pen.setStyle(Qt::NoPen);
    da_painter->setPen(da_pen);
}


// Draw multiple lines.
//
void
QTcanvas::draw_lines(GRmultiPt *p, int n)
{
    if (n < 1)
        return;

    if (da_overlay_count) {
        p->data_ptr_init();
        int nn = n;
        while (nn--) {
            int xx = p->data_ptr_x();
            int yy = p->data_ptr_y();
            bb_add(xx, yy);
            p->data_ptr_inc();
            xx = p->data_ptr_x();
            yy = p->data_ptr_y();
            bb_add(xx, yy);
            p->data_ptr_inc();
        }
    }
    da_call_count++;

    if (da_line_mode)
        da_pen.setStyle((Qt::PenStyle)(da_line_mode + 1));
    else if (!da_line_style || da_line_style->length <= 1)
        da_pen.setStyle(Qt::SolidLine);
    else {
        da_pen.setStyle(Qt::SolidLine);
        da_painter->setPen(da_pen);
        p->data_ptr_init();
        while (n--) {
            int xx = p->data_ptr_x();
            int yy = p->data_ptr_y();
            p->data_ptr_inc();
            draw_line_prv(xx, yy, p->data_ptr_x(), p->data_ptr_y());
            p->data_ptr_inc();
        }
        return;
    }
    da_painter->setPen(da_pen);
    da_painter->drawLines((QLine*)p->data(), n);
    da_pen.setStyle(Qt::NoPen);
    da_painter->setPen(da_pen);
}


void
QTcanvas::define_fillpattern(GRfillType *fillp)
{
    if (!fillp)
        return;
#if defined(WIN32) && defined(DIRECT_TO_GDI)
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
#else

    if (fillp->xPixmap()) {
        delete (QPixmap*)fillp->xPixmap();
        fillp->setXpixmap(0);
    }
    if (fillp->hasMap()) {
        unsigned char *map = fillp->newBitmap();
        QBitmap *bmp = new QBitmap(QBitmap::fromData(
            QSize(fillp->nX(), fillp->nY()),
            (unsigned char*)map, QImage::Format_Mono));
        fillp->setXpixmap((GRfillData)bmp);
        delete [] map;
    }

#endif
}


void
QTcanvas::set_fillpattern(const GRfillType *fillp)
{
    if (!fillp || !fillp->xPixmap())
        da_brush.setStyle(Qt::SolidPattern);
    else
        da_brush.setTexture(*(QBitmap*)fillp->xPixmap());
    da_painter->setBrush(da_brush);
}


// Draw a box, using current fill.
//
void
QTcanvas::draw_box(int x1, int y1, int x2, int y2)
{
    if (da_overlay_count) {
        bb_add(x1, y1);
        bb_add(x2, y2);
    }
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
    da_painter->drawRect(x1, y1, x2-x1 + 1, y2-y1 + 1);
}


// Draw multiple boxes, using current fill.
//
void
QTcanvas::draw_boxes(GRmultiPt *p, int n)
{
    if (n < 1)
        return;
    p->data_ptr_init();
    while (n--) {
        int xx = p->data_ptr_x();
        int yy = p->data_ptr_y();
        p->data_ptr_inc();
        da_painter->drawRect(xx, yy, p->data_ptr_x() - 1, p->data_ptr_y() - 1);
        if (da_overlay_count) {
            bb_add(xx, yy);
            xx += p->data_ptr_x() - 1;
            yy += p->data_ptr_y() - 1;
            bb_add(xx, yy);
        }
        p->data_ptr_inc();
    }
}


// Draw a filled arc.
//
void
QTcanvas::draw_arc(int x0, int y0, int r, int, double a1, double a2)
{
    if (r < 0)
        r = -r;
    if (da_overlay_count) {
        bb_add(x0-r, y0-r);
        bb_add(x0+r, y0+r);
    }
    if (a1 >= a2)
        a2 = 2 * M_PI + a2;
    int t1 = (int)(16 * (180.0 / M_PI) * a1);
    int t2 = (int)(16 * (180.0 / M_PI) * a2 - t1);
    if (t2 == 0)
        return;
    da_painter->drawPie(x0, y0, r, r, t1, t2);
}


// Draw a filled polygon.
//
void
QTcanvas::draw_polygon(GRmultiPt *p, int n)
{
    if (da_overlay_count) {
        p->data_ptr_init();
        while (n--) {
            int xx = p->data_ptr_x();
            int yy = p->data_ptr_y();
            bb_add(xx, yy);
            p->data_ptr_inc();
        }
    }
    da_painter->drawPolygon((QPoint*)p->data(), n, Qt::WindingFill);
}


void
QTcanvas::draw_zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
{
    if (da_overlay_count) {
        bb_add(xll, yl);
        bb_add(xul, yu);
        bb_add(xlr, yl);
        bb_add(xur, yu);
    }
    QPoint pts[5];
    int n = 0;
    pts[n].setX(xll);
    pts[n].setY(yl);
    n++;
    pts[n].setX(xul);
    pts[n].setY(yu);
    if (xul != xur) {
        n++;
        pts[n].setX(xur);
        pts[n].setY(yu);
    }
    n++;
    pts[n].setX(xlr);
    pts[n].setY(yl);
    if (xll != xlr) {
        n++;
        pts[n].setX(xll);
        pts[n].setY(yl);
    }
    da_painter->drawPolygon(pts, n+1, Qt::WindingFill);

}


void
QTcanvas::draw_image(const GRimage *im, int xx, int yy, int w, int h)
{
    QImage qimg((const unsigned char*)im->data(), im->width(), im->height(),
        QImage::Format_RGB32);
    da_painter->drawImage(QPoint(xx, yy), qimg, QRect(xx, yy, w, h));
}


// Set the font used for rendering in the drawing area.
//
void
QTcanvas::set_font(QFont *fnt)
{
    setFont(*fnt);
}


// Return the pixel width of len characters in str when rendered with
// the given font.
//
int
QTcanvas::text_width(QFont *fnt, const char *str, int len)
{
    QFontMetrics fm(*fnt);
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    return (fm.horizontalAdvance(QString(str), len));
#else
    return (fm.width(QString(str), len));
#endif
}


// Return the width and height of the string as rendered in the
// current font.
//
void
QTcanvas::text_extent(const char *str, int *w, int *h)
{
    if (!str)
        str = "x";
    if (w)
        *w = 0;
    if (h)
        *h = 0;
    const QFont &f = font();
    QFontMetrics fm(f);
    if (w)
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
        *w = fm.horizontalAdvance(QString(str));
#else
        *w = fm.width(QString(str));
#endif
    if (h)
        *h = fm.height();
}


// Draw text, at most len characters from str if len >= 0.
//
void
QTcanvas::draw_text(int x0, int y0, const char *str, int len)
{
    da_call_count++;
    QString qs(str);
    if (len >= 0)
        qs.truncate(len);

    y0 -= 2;
    QFontMetrics fm(font());
    if (da_overlay_count) {
        QRect r = fm.boundingRect(qs);
        bb_add(x0 + r.x(), y0);
        bb_add(x0 + r.x() + r.width(), y0 + r.height());
    }

    da_painter->setFont(font());
    da_pen.setStyle(Qt::SolidLine);
    da_painter->setPen(da_pen);
    da_painter->drawText(x0, y0, qs);
    da_pen.setStyle(Qt::NoPen);
    da_painter->setPen(da_pen);
}


// Draw a len X len glyph with bitmap data given in bits centered at
// x0, y0.
void
QTcanvas::draw_glyph(int x0, int y0, const unsigned char *bits, int len)
{
    x0 -= len/2;
    y0 -= len/2;
    if (da_overlay_count) {
        bb_add(x0, y0);
        bb_add(x0 + len, y0 + len);
    }
    Qt::BrushStyle st = da_brush.style();
    QBitmap bm(QBitmap::fromData(QSize(len, len), bits));
    da_brush.setTexture(bm);
    da_brush.setTransform(QTransform::fromTranslate(x0, y0));
    da_painter->setBrush(da_brush);
    da_painter->drawRect(x0, y0, len, len);
    da_brush.setStyle(st);
    da_brush.setTransform(QTransform());
    da_painter->setBrush(da_brush);
}


// Copy out the part of a pixmap (xp,yp,wp,hp) to xw,yw.
//
void
QTcanvas::draw_pixmap(int xw, int yw, QPixmap *pmap,
    int xp, int yp, int wp, int hp)
{
    da_painter->drawPixmap(xw, yw, wp, hp, *pmap, xp, yp, wp, hp);
}


// Copy out the part of a pixmap (xp,yp,wp,hp) to xw,yw.
//
void
QTcanvas::draw_image(int xw, int yw, QImage *image,
    int xp, int yp, int wp, int hp)
{
    da_painter->drawImage(xw, yw, *image, xp, yp, wp, hp);
}
// End of exported virtual interface


// Set to internal line pattern (argument nonzero).  If set to zero,
// line will be solid or use da_line_style if set.
//
void
QTcanvas::set_line_mode(int pattern_index)
{
    da_line_mode = pattern_index;
}


// Switch between solid fill and pixmap tiling, both are accomplished
// with draw_rectangle(true, ...).
//
void
QTcanvas::set_fill(bool tiling)
{
    da_fill_mode = tiling;
}


// Set the pixmap for use in tiling (see set_fill).
//
void
QTcanvas::set_tile(QPixmap *pixmp)
{
    da_tile_pixmap = pixmp;
}


// Set the origin used when drawing tiled pixmaps.
//
void
QTcanvas::set_tile_origin(int x0, int y0)
{
    da_tile_x = x0;
    da_tile_y = y0;
}


// Draw a tiled pixmap, self contained, overrides present settings.
//
void
QTcanvas::tile_draw_pixmap(int o_x, int o_y, QPixmap *pm, int xx, int yy,
    int w, int h)
{
    if (!pm)
        return;
    da_painter->drawTiledPixmap(xx, yy, w, h, *pm, o_x, o_y);
}


// Draw a filled or open rectangle, or pixmap tiles.
//
void
QTcanvas::draw_rectangle(bool filled, int x0, int y0, int w, int h)
{
    if (filled) {
        if (da_fill_mode) {
            if (da_tile_pixmap) {
                da_painter->drawTiledPixmap(x0, y0, w, h, *da_tile_pixmap,
                    da_tile_x, da_tile_y);
            }
        }
        else
            da_painter->drawRect(x0, y0, w, h);
    }
    else {
        Qt::BrushStyle st = da_brush.style();
        da_brush.setStyle(Qt::NoBrush);
        da_painter->setBrush(da_brush);
        da_pen.setStyle(Qt::SolidLine);
        da_pen.setColor(da_fg);
        da_painter->setPen(da_pen);
        da_painter->drawRect(x0, y0, w, h);
        da_brush.setStyle(st);
        da_painter->setBrush(da_brush);
    }
}


// Draw an arc or pie-slice.
//
void
QTcanvas::draw_arc(bool filled, int x0, int y0, int w, int h, int st, int sp)
{
    if (filled) {
        da_painter->drawPie(x0, y0, w, h, st, sp);
    }
    else {
        Qt::BrushStyle sty = da_brush.style();
        da_brush.setStyle(Qt::NoBrush);
        da_painter->setBrush(da_brush);
        da_pen.setStyle(Qt::SolidLine);
        da_pen.setColor(da_fg);
        da_painter->setPen(da_pen);
        da_painter->drawArc(x0, y0, w, h, st, sp);
        da_brush.setStyle(sty);
        da_painter->setBrush(da_brush);
    }
}


// Draw a filled or open polygon.
//
void
QTcanvas::draw_polygon(bool filled, QPoint *points, int numpts)
{
    if (filled)
        da_painter->drawPolygon(points, numpts);
    else {
        Qt::BrushStyle st = da_brush.style();
        da_brush.setStyle(Qt::NoBrush);
        da_painter->setBrush(da_brush);
        da_pen.setStyle(Qt::SolidLine);
        da_pen.setColor(da_fg);
        da_painter->setPen(da_pen);
        da_painter->drawPolygon(points, numpts);
        da_brush.setStyle(st);
        da_painter->setBrush(da_brush);
    }
}
// End of extra drawing functions


void
QTcanvas::resizeEvent(QResizeEvent *ev)
{
    QFont fnt = da_painter->font();
    da_painter->end();
    delete da_painter;
    delete da_pixmap;
    delete da_overlay_bg;
    da_overlay_bg = 0;

    // If a dimension is 0, lots of "Painter not active" messages.
    QSize sz = ev->size();
    if (sz.width() < 1)
        sz.setWidth(1);
    if (sz.height() < 1)
        sz.setHeight(1);

    da_pixmap = new QPixmap(sz);
    da_painter = new QPainter(da_pixmap);
    da_painter->setFont(fnt);
    initialize();
    if (da_pixmap_bak) {
        fnt = da_painter_bak->font();
        da_painter_bak->end();
        delete da_painter_bak;
        delete da_pixmap_bak;
        da_pixmap_bak = new QPixmap(sz);
        da_painter_bak = new QPainter(da_pixmap_bak);
        da_painter_bak->setFont(fnt);
    }
    else if (da_pixmap2) {
        fnt = da_painter2->font();
        da_painter2->end();
        delete da_painter2;
        delete da_pixmap2;
        da_pixmap2 = new QPixmap(sz);
        da_painter2 = new QPainter(da_pixmap2);
        da_painter2->setFont(fnt);
    }
    emit new_painter(da_painter);
    emit resize_event(ev);
}


void
QTcanvas::paintEvent(QPaintEvent *ev)
{
    if (!da_pixmap)
        return;
    QPainter p(this);
    QRegion::const_iterator i;
    for (i = ev->region().begin(); i != ev->region().end(); i++)
        p.drawPixmap(*i, *da_pixmap, *i);
}


void
QTcanvas::mousePressEvent(QMouseEvent *ev)
{
    emit press_event(ev);
}


void
QTcanvas::mouseReleaseEvent(QMouseEvent *ev)
{
    emit release_event(ev);
}


void
QTcanvas::mouseDoubleClickEvent(QMouseEvent *ev)
{
    emit double_click_event(ev);
}


void
QTcanvas::mouseMoveEvent(QMouseEvent *ev)
{
    emit motion_event(ev);
}


void
QTcanvas::keyPressEvent(QKeyEvent *ev)
{
    emit key_press_event(ev);
}


void
QTcanvas::keyReleaseEvent(QKeyEvent *ev)
{
    emit key_release_event(ev);
}


#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)

void
QTcanvas::enterEvent(QEnterEvent *ev)
{
    emit enter_event(ev);
}

#else

void
QTcanvas::enterEvent(QEvent *evp)
{
    QEnterEvent *ev = dynamic_cast<QEnterEvent*>(evp);
    emit enter_event(ev);
}

#endif


void
QTcanvas::leaveEvent(QEvent *ev)
{
    emit leave_event(ev);
}


void
QTcanvas::focusInEvent(QFocusEvent *ev)
{
    emit focus_in_event(ev);
}


void
QTcanvas::focusOutEvent(QFocusEvent *ev)
{
    emit focus_out_event(ev);
}


void
QTcanvas::wheelEvent(QWheelEvent *ev)
{
    emit mouse_wheel_event(ev);
}


void
QTcanvas::dragEnterEvent(QDragEnterEvent *ev)
{
    emit drag_enter_event(ev);
}


void
QTcanvas::dropEvent(QDropEvent *ev)
{
    emit drop_event(ev);
}


void
QTcanvas::initialize()
{
    da_painter->setBrush(da_brush);
    da_painter->setPen(da_pen);
    clear();
    bb_init();
}


// Ghost drawing.

cGhostDrawCommon::cGhostDrawCommon()
{
    gd_ghost_draw_func = 0;
    gd_linedb = 0;
    gd_ref_x = 0;
    gd_ref_y = 0;
    gd_last_x = 0;
    gd_last_y = 0;
    gd_ghost_cx_cnt = 0;
    gd_first_ghost = false;
    gd_show_ghost = false;
    gd_undraw = false;
    gd_xor_mode = false;
    for (int i = 0; i < 8; i++)
        gd_windows[i] = 0;
}


void
cGhostDrawCommon::set_ghost(GhostDrawFunc callback, int xx, int yy)
{
    if (gd_ghost_draw_func) {
        if (gd_undraw && !gd_first_ghost) {
            // undraw last
            if (gd_xor_mode) {
                gd_linedb = new GRlineDb;
                for (int i = 0; i < 5; i++) {
                    if (gd_windows[i]) {
                        gd_windows[i]->xordrw_beg();
                    }
                }
                (*gd_ghost_draw_func)(gd_last_x, gd_last_y,
                    gd_ref_x, gd_ref_y, gd_undraw);
                for (int i = 0; i < 5; i++) {
                    if (gd_windows[i]) {
                        gd_windows[i]->xordrw_end();
                    }
                }
                delete gd_linedb;
                gd_linedb = 0;
            }
            else {
                for (int i = 0; i < 5; i++) {
                    if (gd_windows[i]) {
                        gd_windows[i]->set_overlay_mode(true);
                        gd_windows[i]->erase_last_overlay();
                        gd_windows[i]->set_overlay_mode(false);
                    }
                }
            }
            gd_undraw = false;
        }
        gd_ghost_draw_func = 0;
    }

    if (callback) {
        for (int i = 0; i < 5; i++) {
            if (gd_windows[i]) {
                gd_windows[i]->gdrw_setbg();
                gd_windows[i]->create_overlay_backg();
            }
        }

        gd_ghost_draw_func = callback;
        gd_ref_x = xx;
        gd_ref_y = yy;
        gd_last_x = 0;
        gd_last_y = 0;
        gd_ghost_cx_cnt = 0;
        gd_first_ghost = true;
        gd_show_ghost = true;
        gd_undraw = false;
    }
    else if (gd_show_ghost) {
        gd_show_ghost = false;
        for (int i = 0; i < 5; i++) {
            if (gd_windows[i]) {
                gd_windows[i]->gdrw_unsetbg();
            }
        }
    }
}


// Turn on/off display of ghosting.  Keep track of calls in ghost_cx_cnt.
//
void
cGhostDrawCommon::show_ghost(bool showit)
{
    if (!showit) {
        if (!gd_ghost_cx_cnt && gd_show_ghost) {
            undraw_ghost(false);
            gd_show_ghost = false;
            gd_first_ghost = true;
            for (int i = 0; i < 5; i++) {
                if (gd_windows[i]) {
                    gd_windows[i]->gdrw_unsetbg();
                }
            }
            // This will redraw ghost (maybe).
            QPoint qp = QCursor::pos();
            QCursor::setPos(qp.x() + 0, qp.y() + 0);
        }
        gd_ghost_cx_cnt++;
    }
    else {
        if (gd_ghost_cx_cnt)
            gd_ghost_cx_cnt--;
        if (!gd_ghost_cx_cnt && !gd_show_ghost && gd_ghost_draw_func) {
            for (int i = 0; i < 5; i++) {
                if (gd_windows[i]) {
                    gd_windows[i]->gdrw_setbg();
                    gd_windows[i]->create_overlay_backg();
                }
            }
            gd_show_ghost = true;

            // This will redraw ghost.
            QPoint qp = QCursor::pos();
            if (gd_windows[0])
                qp = gd_windows[0]->mapFromGlobal(qp);
            draw_ghost(qp.x(), qp.y());
        }
    }
}


void
cGhostDrawCommon::undraw_ghost(bool reset)
{
    if (gd_ghost_draw_func && gd_show_ghost && gd_undraw &&
            !gd_first_ghost) {
        if (gd_xor_mode) {
            gd_linedb = new GRlineDb;
            for (int i = 0; i < 5; i++) {
                if (gd_windows[i]) {
                    gd_windows[i]->xordrw_beg();
                }
            }
            (*gd_ghost_draw_func)(gd_last_x, gd_last_y,
                gd_ref_x, gd_ref_y, gd_undraw);
            for (int i = 0; i < 5; i++) {
                if (gd_windows[i]) {
                    gd_windows[i]->xordrw_end();
                }
            }
            delete gd_linedb;
            gd_linedb = 0;
        }
        else {
            for (int i = 0; i < 5; i++) {
                if (gd_windows[i]) {
                    gd_windows[i]->set_overlay_mode(true);
                    gd_windows[i]->erase_last_overlay();
                    gd_windows[i]->set_overlay_mode(false);
                }
            }
        }
        gd_undraw = false;
        if (reset)
            gd_first_ghost = true;
    }
}


void
cGhostDrawCommon::draw_ghost(int xx, int yy)
{
    if (gd_ghost_draw_func && gd_show_ghost && !gd_undraw) {
        gd_last_x = xx;
        gd_last_y = yy;
        if (gd_xor_mode) {
            gd_linedb = new GRlineDb;
            for (int i = 0; i < 5; i++) {
                if (gd_windows[i]) {
                    gd_windows[i]->xordrw_beg();
                }
            }
            (*gd_ghost_draw_func)(xx, yy, gd_ref_x, gd_ref_y, gd_undraw);
            for (int i = 0; i < 5; i++) {
                if (gd_windows[i]) {
                    gd_windows[i]->xordrw_end();
                }
            }
            delete gd_linedb;
            gd_linedb = 0;
        }
        else {
            for (int i = 0; i < 5; i++) {
                QTcanvas *cv = gd_windows[i];
                if (cv) {
                    cv->set_overlay_mode(true);
                    cv->drw_beg();
                    // Need to update the overlay_backg here after
                    // drawing a mark (such as a plot mark) otherwise
                    // erasing the ghost will eat the mark.  The call
                    // count will be nonzero if a mark was just drawn,
                    // and is reset in the block below.

                    if (cv->call_count())
                        cv->create_overlay_backg();
                }
            }
            (*gd_ghost_draw_func)(xx, yy, gd_ref_x, gd_ref_y, gd_undraw);
            for (int i = 0; i < 5; i++) {
                QTcanvas *cv = gd_windows[i];
                if (cv) {
                    cv->drw_end();
                    cv->set_overlay_mode(false);
                    cv->set_call_count(0);
                }
            }
        }
        gd_undraw = true;
        gd_first_ghost = false;
    }
}

