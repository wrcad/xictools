
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

#ifndef DRAW_X_W_H
#define DRAW_X_W_H

#include "qtdraw.h"
#include <QWidget>
#include <QResizeEvent>
#include <math.h>
#include <X11/Xlib.h>

class QEvent;
class QMouseEvent;
class QResizeEvent;
class QPaintEvent;
class QKeyEvent;
class QDragEnterEvent;
class QDragEvent;

namespace qtinterf
{
    class draw_x_w : public QWidget, public draw_if
    {
        Q_OBJECT

    public:
        draw_x_w(bool, QWidget *parent);

        QWidget *widget() { return (this); }

        void draw_direct(bool);
        void update();
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
        int text_width(QFont*, const char*, int);
        void text_extent(const char*, int*, int*);
        void draw_text(int, int, const char*, int);

        void set_xor_mode(bool);
        void set_ghost_color(unsigned int);

    signals:
        void resize_event(QResizeEvent*);
        void paint_event(QPaintEvent*);
        void press_event(QMouseEvent*);
        void release_event(QMouseEvent*);
        void move_event(QMouseEvent*);
        void key_press_event(QKeyEvent*);
        void key_release_event(QKeyEvent*);
        void enter_event(QEvent*);
        void leave_event(QEvent*);
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
        void dragEnterEvent(QDragEnterEvent*);
        void dropEvent(QDropEvent*);

    private:
        QColor *da_fg;              // foreground color pointer
        QColor *da_bg;              // background color pointer
        QColor *da_ghost;           // ghost color pointer
        QColor da_ghost_fg;         // ghost color ^ background

        QColor da_fg_bak;           // foreground color
        QColor da_bg_bak;           // background color
        QColor da_ghost_bak;        // ghost color

        static QColor com_fg;       // common foreground color
        static QColor com_bg;       // common background color
        static QColor com_ghost;    // common ghost color

        QPixmap *da_pixmap;         // pixmap for drawing
        Drawable da_fore;           // current drawable
        Display *da_display;        // the display
        GC da_gc;                   // GC
        bool da_direct;             // using direct draw

        static GC com_gc;           // common GC
    };
}

#endif

