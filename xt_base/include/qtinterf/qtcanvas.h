
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

#ifndef DRAW_QT_W_H
#define DRAW_QT_W_H

#include "qtdraw.h"
#include <QWidget>
#include <QBrush>
#include <QPen>
#include <QPixmap>
#include <QPaintEngine>
#include <math.h>

class QEvent;
class QMouseEvent;
class QResizeEvent;
class QPaintEvent;
class QKeyEvent;
class QDragEnterEvent;
class QDragEvent;
class QEnterEvent;
class QFocusEvent;

namespace qtinterf
{
    class QTcanvas : public QWidget, public draw_if
    {
        Q_OBJECT

    public:
        QTcanvas(QWidget *parent = nullptr);
        ~QTcanvas();

        QWidget *widget()           { return (this); }
        QPixmap *pixmap()           { return (da_pixmap); }

        void draw_direct(bool);
        void switch_to_pixmap2();
        void switch_from_pixmap2(int, int, int, int);
        void set_draw_to_pixmap(QPixmap*);
        void refresh(int, int, int, int);
        void refresh()              { refresh(0, 0, width(), height()); }
        void update(int, int, int, int);
        void update()               { update(0, 0, width(), height()); }
        void clear();
        void clear_area(int, int, int, int);

        void set_foreground(unsigned int);
        void set_background(unsigned int);

        void draw_pixel(int, int);
        void draw_pixels(GRmultiPt*, int);

        void set_linestyle(const GRlineType*);
        void draw_line(int, int, int, int);
        void draw_polyline(GRmultiPt*, int);
        void draw_lines(GRmultiPt*, int);

        void define_fillpattern(GRfillType*);
        void set_fillpattern(const GRfillType*);
        void draw_box(int, int, int, int);
        void draw_boxes(GRmultiPt*, int);
        void draw_arc(int, int, int, int, double, double);
        void draw_polygon(GRmultiPt*, int);
        void draw_zoid(int, int, int, int, int, int);
        void draw_image(const GRimage*, int, int, int, int);

        void set_font(QFont*);
        int  text_width(QFont*, const char*, int);
        void text_extent(const char*, int*, int*);
        void draw_text(int, int, const char*, int);

        void set_xor_mode(bool);
        void set_ghost_color(unsigned int);

        void draw_pixmap(int, int, QPixmap*, int, int, int, int);
        void draw_image(int, int, QImage*, int, int, int, int);

        // extra functions
        void set_line_mode(int);
        void set_fill(bool);
        void set_tile(QPixmap*);
        void set_tile_origin(int, int);
        void draw_rectangle(bool, int, int, int, int);
        void draw_arc(bool, int, int, int, int, int, int);
        void draw_polygon(bool, QPoint*, int);

        QPainter *cur_painter() { return (da_painter); }

    signals:
        void resize_event(QResizeEvent*);
        void new_painter(QPainter*);
        void paint_event(QPaintEvent*);
        void press_event(QMouseEvent*);
        void release_event(QMouseEvent*);
        void move_event(QMouseEvent*);
        void key_press_event(QKeyEvent*);
        void key_release_event(QKeyEvent*);
        void enter_event(QEnterEvent*);
        void leave_event(QEvent*);
        void focus_in_event(QFocusEvent*);
        void focus_out_event(QFocusEvent*);
        void drag_enter_event(QDragEnterEvent*);
        void drop_event(QDropEvent*);

    protected:
        void resizeEvent(QResizeEvent*);
        void paintEvent(QPaintEvent*);
        void mousePressEvent(QMouseEvent*);
        void mouseReleaseEvent(QMouseEvent*);
        void mouseMoveEvent(QMouseEvent*);
        void keyPressEvent(QKeyEvent*);
        void keyReleaseEvent(QKeyEvent*);
        void enterEvent(QEvent*);
        void leaveEvent(QEvent*);
        void focusInEvent(QFocusEvent*);
        void focusOutEvent(QFocusEvent*);
        void dragEnterEvent(QDragEnterEvent*);
        void dropEvent(QDropEvent*);

    private:
        // Init a bounding box for refreshing.                                 
        void bb_init()
        {
            da_xb1 = size().width();
            da_yb1 = size().height();
            da_xb2 = 0;
            da_yb2 = 0;
        }

        // Add a vertex to the bounding box.
        void bb_add(int xx, int yy)
        {
            if (xx < da_xb1)
                da_xb1 = xx;
            if (yy < da_yb1)
                da_yb1 = yy;
            if (xx > da_xb2)
                da_xb2 = xx;
            if (yy > da_yb2)
                da_yb2 = yy;
        }

        void draw_line_prv(int, int, int, int);
        void initialize();

        QPixmap     *da_pixmap;         // main pixmap
        QPixmap     *da_pixmap2;        // backing pixmap
        QPixmap     *da_pixmap_bak;     // mode_swap_backing
        QPixmap     *da_tile_pixmap;    // tiling pixmap;
        QPainter    *da_painter;        // main painter, paints da_pixmap
        QPainter    *da_painter2;       // backing painter, paints da_pixmap2
        QPainter    *da_painter_bak;    // mode swap backing
        QColor      da_fg;              // foreground color
        QColor      da_bg;              // background color
        QColor      da_ghost;           // ghost color
        QColor      da_ghost_fg;        // ghost color ^ background
        QBrush      da_brush;           // solid fill brush 
        QPen        da_pen;             // min width pen
        int         da_tile_x;          // tile origin x
        int         da_tile_y;          // tile origin y
        bool        da_fill_mode;       // true when tiling
        bool        da_xor_mode;        // true in XOR mode
        bool        da_direct_mode;     // direct mode, see note in draw_direct
        int         da_line_mode;       // true when using internal textured
                                        //  lines (Qt::PenStyle - 1)
                                        //  1: dashes separated by a few pixels
                                        //  2: dots separated by a few pixels
                                        //  3: alternate dots and dashes
                                        //  4: one dash, two dots, one dash,
                                        //     two dots
        const
        GRlineType *da_line_style;      // set for user-defined texture,
                                        //  da_line_mode = 0 in this case

        // Keep a bounding box for refreshing.                                 
        int         da_xb1, da_yb1;
        int         da_xb2, da_yb2;
    };
}

#endif

