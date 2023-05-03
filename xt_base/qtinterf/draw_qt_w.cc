
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

#include "draw_qt_w.h"
#include <QResizeEvent>
#include <QPaintEvent>
#include <QBitmap>
#include <QPainter>


using namespace qtinterf;

draw_qt_w::draw_qt_w(bool, QWidget *prnt) : QWidget(prnt)
{
    setMouseTracking(true);
    setMinimumWidth(50);
    setMinimumHeight(50);

    da_pixmap = new QPixmap(50, 50);
    da_painter = new QPainter(da_pixmap);
    da_bg.setNamedColor(QString("white"));
    da_fg.setNamedColor(QString("black"));
    da_brush.setStyle(Qt::SolidPattern);
    da_painter_temp = 0;
    da_tile_pixmap = 0;
    da_tile_x = 0;
    da_tile_y = 0;
    da_fill_mode = false;
    da_line_mode = 0;
    da_pen.setStyle(Qt::NoPen);
    da_line_style = 0;
    initialize();
}


draw_qt_w::~draw_qt_w()
{
    delete da_painter;
    delete da_pixmap;
}


//
// The drawing interface.
//

void
draw_qt_w::draw_direct(bool direct)
{
    if (direct) {
        da_painter_temp = da_painter;
        da_painter = new QPainter(widget());
        initialize();
    }
    else {
        delete da_painter;
        da_painter = da_painter_temp;
    }
}


// Paint the screen window from the pixmap.
//
void
draw_qt_w::update()
{
    repaint(0, 0, width(), height());
}


// Flood with the background color.
//
void
draw_qt_w::clear()
{
    da_brush.setColor(da_bg);
    da_brush.setStyle(Qt::SolidPattern);
    da_painter->setBrush(da_brush);
    da_painter->drawRect(0, 0, width(), height());
    da_brush.setColor(da_fg);
}


void
draw_qt_w::clear_area(int x0, int y0, int w, int h)
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
draw_qt_w::set_foreground(unsigned int pix)
{
    da_fg.setRgb(pix);
    da_brush.setColor(da_fg);
    da_pen.setColor(da_fg);
    da_painter->setPen(da_pen);
    da_painter->setBrush(da_brush);
}


// Set the background rendering color.
//
void
draw_qt_w::set_background(unsigned int pix)
{
    da_bg.setRgb(pix);
}


// Draw one pixel.
//
void
draw_qt_w::draw_pixel(int x0, int y0)
{
    da_pen.setStyle(Qt::SolidLine);
    da_painter->drawPoint(x0, y0);
}


// Draw multiple pixels.
//
void
draw_qt_w::draw_pixels(GRmultiPt *p, int n)
{
    QPolygon poly;
    poly.setPoints(n, (int*)p->data());
    da_pen.setStyle(Qt::SolidLine);
    da_painter->drawPoints(poly);
}


// Set the current line texture, used when da_line_mode = 0;
//
void
draw_qt_w::set_linestyle(const GRlineType *lstyle)
{
    da_line_style = lstyle;
}


// Draw line function (private).  Arbitrary pattterned lines are not
// supported in QT.  This function implements patterned line drawing,
// one pixel at a time.  It is faster than I expected.
//
void
draw_qt_w::draw_line_prv(int x1, int y1, int x2, int y2)
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
draw_qt_w::draw_line(int x1, int y1, int x2, int y2)
{
    if (da_line_mode)
        da_pen.setStyle((Qt::PenStyle)(da_line_mode + 1));
    else if (!da_line_style || da_line_style->length <= 1)
        da_pen.setStyle(Qt::SolidLine);
    else {
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
draw_qt_w::draw_polyline(GRmultiPt *p, int n)
{
    if (n < 2)
        return;
    if (da_line_mode)
        da_pen.setStyle((Qt::PenStyle)(da_line_mode + 1));
    else if (!da_line_style || da_line_style->length <= 1)
        da_pen.setStyle(Qt::SolidLine);
    else {
        p->data_ptr_init();
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
draw_qt_w::draw_lines(GRmultiPt *p, int n)
{
    if (n < 1)
        return;
    if (da_line_mode)
        da_pen.setStyle((Qt::PenStyle)(da_line_mode + 1));
    else if (!da_line_style || da_line_style->length <= 1)
        da_pen.setStyle(Qt::SolidLine);
    else {
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
draw_qt_w::define_fillpattern(GRfillType *fillp)
{
    if (fillp && fillp->hasMap()) {
        QBitmap bm = QBitmap::fromData(QSize(fillp->nX(), fillp->nY()),
            (const unsigned char*)fillp->map(), QImage::Format_Mono);
        fillp->setXpixmap(new QBitmap(bm));
    }
}


void
draw_qt_w::set_fillpattern(const GRfillType *fillp)
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
draw_qt_w::draw_box(int x1, int y1, int x2, int y2)
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
    da_painter->drawRect(x1, y1, x2-x1 + 1, y2-y1 + 1);
}


// Draw multiple boxes, using current fill.
//
void
draw_qt_w::draw_boxes(GRmultiPt *p, int n)
{
    if (n < 1)
        return;
    p->data_ptr_init();
    while (n--) {
        int xx = p->data_ptr_x();
        int yy = p->data_ptr_y();
        p->data_ptr_inc();
        da_painter->drawRect(xx, yy, p->data_ptr_x() - 1, p->data_ptr_y() - 1);
        p->data_ptr_inc();
    }
}


// Draw a filled arc.
//
void
draw_qt_w::draw_arc(int x0, int y0, int r, int, double a1, double a2)
{
    if (a1 >= a2)
        a2 = 2 * M_PI + a2;
    int t1 = (int)(16 * (180.0 / M_PI) * a1);
    int t2 = (int)(16 * (180.0 / M_PI) * a2 - t1);
    if (t2 == 0)
        return;
    int dim = 2*r;
    da_painter->drawPie(x0 - r, y0 - r, dim, dim, t1, t2);
}


// Draw a filled polygon.
//
void
draw_qt_w::draw_polygon(GRmultiPt *p, int n)
{
    da_painter->drawPolygon((QPoint*)p->data(), n, Qt::WindingFill);
}


void
draw_qt_w::draw_zoid(int, int, int, int, int, int)
// draw_gl_w::draw_zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
{
}


void
draw_qt_w::draw_image(const GRimage*, int, int, int, int)
//draw_gl_w::draw_image(const GRimage *image, int x, int y,
//    int width, int height)
{
}


// Set the font used for rendering in the drawing area.
//
void
draw_qt_w::set_font(QFont *fnt)
{
    widget()->setFont(*fnt);
}


// Return the pixel width of len characters in str when rendered with
// the given font.
//
int
draw_qt_w::text_width(QFont *fnt, const char *str, int len)
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
draw_qt_w::text_extent(const char *str, int *w, int *h)
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
draw_qt_w::draw_text(int x0, int y0, const char *str, int len)
{
    QString qs(str);
    if (len >= 0)
        qs.truncate(len);

    // This changed between 4.0.1 and 4.1.2, the pen style can't be
    // Qt::NoPen or text will not be rendered.
    da_painter->setFont(font());
    da_pen.setStyle(Qt::SolidLine);
    da_painter->setPen(da_pen);
    da_painter->drawText(x0, y0, qs);
    da_pen.setStyle(Qt::NoPen);
    da_painter->setPen(da_pen);
}


void
draw_qt_w::set_xor_mode(bool set)
{
    if (set) {
        da_painter->setCompositionMode(QPainter::RasterOp_SourceXorDestination);
    }
    else {
        update();
        da_painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    }
}


void
draw_qt_w::set_ghost_color(unsigned int pixel)
{
    da_ghost.setRgb(pixel);
    da_ghost_fg.setRgb(pixel ^ da_bg.rgb());
}
// End of exported virtual interface


// Switch the drawing context to the supplied pixmap, or back to the
// main pixmap if 0 is passed.
//
void
draw_qt_w::set_draw_to_pixmap(QPixmap *pixmap)
{
    if (pixmap) {
        if (da_painter_temp)
            // already drawing to pixmap
            return;
        QPainter *p = new QPainter(pixmap);
        da_painter_temp = da_painter;
        da_painter = p;
    }
    else {
        if (!da_painter_temp)
            return;
        delete da_painter;
        da_painter = da_painter_temp;
        da_painter_temp = 0;
    }
    initialize();
}


// Set to internal line pattern (argument nonzero).  If set to zero,
// line will be solid or use da_line_style if set.
//
void
draw_qt_w::set_line_mode(int pattern_index)
{
    da_line_mode = pattern_index;
}


// Switch between solid fill and pixmap tiling, both are accomplished
// with draw_rectangle(true, ...).
//
void
draw_qt_w::set_fill(bool tiling)
{
    da_fill_mode = tiling;
}


// Set the pixmap for use in tiling (see set_fill).
//
void
draw_qt_w::set_tile(QPixmap *pixmap)
{
    da_tile_pixmap = pixmap;
}


// Set the origin used when drawing tiled pixmaps.
//
void
draw_qt_w::set_tile_origin(int x0, int y0)
{
    da_tile_x = x0;
    da_tile_y = y0;
}


// Draw a filled or open rectangle, or pixmap tiles.
//
void
draw_qt_w::draw_rectangle(bool filled, int x0, int y0, int w, int h)
{
    if (filled) {
        if (da_fill_mode) {
            if (da_tile_pixmap)
                da_painter->drawTiledPixmap(x0, y0, w, h, *da_tile_pixmap,
                    da_tile_x, da_tile_y);
        }
        else
            da_painter->drawRect(x0, y0, w, h);
    }
    else {
        Qt::BrushStyle st = da_brush.style();
        da_brush.setStyle(Qt::NoBrush);
        da_painter->setBrush(da_brush);
        da_painter->drawRect(x0, y0, w, h);
        da_brush.setStyle(st);
        da_painter->setBrush(da_brush);
    }
}


// Draw an arc or pie-slice.
//
void
draw_qt_w::draw_arc(bool filled, int x0, int y0, int w, int h, int st, int sp)
{
    if (filled)
        da_painter->drawPie(x0, y0, w, h, st/4, sp/4);
    else
        da_painter->drawArc(x0, y0, w, h, st/4, sp/4);
}


// Draw a filled or open polygon.
//
void
draw_qt_w::draw_polygon(bool filled, QPoint *points, int numpts)
{
    if (filled)
        da_painter->drawPolygon(points, numpts);
    else {
        Qt::BrushStyle st = da_brush.style();
        da_brush.setStyle(Qt::NoBrush);
        da_painter->setBrush(da_brush);
        da_painter->drawPolygon(points, numpts);
        da_brush.setStyle(st);
        da_painter->setBrush(da_brush);
    }
}


// Copy out the part of a pixmap (xp,yp,wp,hp) to xw,yw.
//
void
draw_qt_w::draw_pixmap(int xw, int yw, QPixmap *pmap,
    int xp, int yp, int wp, int hp)
{
    da_painter->drawPixmap(xw, yw, wp, hp, *pmap, xp, yp, wp, hp);
}


// Copy out the part of a pixmap (xp,yp,wp,hp) to xw,yw.
//
void
draw_qt_w::draw_image(int xw, int yw, QImage *image,
    int xp, int yp, int wp, int hp)
{
    da_painter->drawImage(xw, yw, *image, xp, yp, wp, hp);
}
// End of extra drawing functions


void
draw_qt_w::resizeEvent(QResizeEvent *ev)
{
    if (da_painter)
        da_painter->end();
    QFont fnt = da_painter->font();
    delete da_painter;
    delete da_pixmap;
    da_pixmap = new QPixmap(ev->size());
    da_painter = new QPainter(da_pixmap);
    da_painter->setFont(fnt);
    initialize();
    emit new_painter(da_painter);
    emit resize_event(ev);
}


void
draw_qt_w::paintEvent(QPaintEvent *ev)
{
    if (!da_pixmap)
        return;
    QPainter p(this);
    QVector<QRect> rects = ev->region().rects();
    for (int i  = 0; i < rects.size(); i++) {
        QRect r = rects.at(i);
        p.drawPixmap(r, *da_pixmap, r);
    }

    // The application can handle this to draw highlighting that is
    // not in the pixmap.  In some applications, it is desirable to
    // keep highlighting out of the pixmap, since it can be very
    // expensive to erase the highlighting.
    //
    emit paint_event(ev);
}


void
draw_qt_w::mousePressEvent(QMouseEvent *ev)
{
    emit press_event(ev);
}


void
draw_qt_w::mouseReleaseEvent(QMouseEvent *ev)
{
    emit release_event(ev);
}


void
draw_qt_w::mouseMoveEvent(QMouseEvent *ev)
{
    emit move_event(ev);
}


void
draw_qt_w::keyPressEvent(QKeyEvent *ev)
{
    emit key_press_event(ev);
}


void
draw_qt_w::keyReleaseEvent(QKeyEvent *ev)
{
    emit key_release_event(ev);
}


void
draw_qt_w::enterEvent(QEvent *ev)
{
    emit enter_event(ev);
}


void
draw_qt_w::leaveEvent(QEvent *ev)
{
    emit leave_event(ev);
}


void
draw_qt_w::dragEnterEvent(QDragEnterEvent *ev)
{
    emit drag_enter_event(ev);
}


void
draw_qt_w::dropEvent(QDropEvent *ev)
{
    emit drop_event(ev);
}


void
draw_qt_w::initialize()
{
    da_painter->setBrush(da_brush);
    da_painter->setPen(da_pen);
    clear();
}

