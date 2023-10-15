
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

#ifndef QTCANVAS_H
#define QTCANVAS_H

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

namespace qtinterf {
    class QTcanvas;
}

class qtinterf::QTcanvas : public QTdrawIf
{
    Q_OBJECT

public:
    QTcanvas(QWidget *parent = nullptr);
    ~QTcanvas();

    QPixmap *pixmap()           { return (da_pixmap); }

    void switch_to_pixmap2();
    void switch_from_pixmap2(int, int, int, int, int, int);
    void set_overlay_mode(bool);
    void create_overlay_backg();
    void erase_last_overlay();
    void set_draw_to_pixmap(QPixmap*);
    void set_clipping(int, int, int, int);
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
    void draw_glyph(int, int, const unsigned char*, int);

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

    // Ghost drawing.
    void set_ghost(GhostDrawFunc, int, int);
    void show_ghost(bool);
    void undraw_ghost(bool);
    void draw_ghost(int, int);

    void set_ghost_mode(bool);
    void set_ghost_color(unsigned int);
    bool has_ghost()        { return (gd_ghost_draw_func != 0); }
    bool showing_ghost()    { return (gd_show_ghost && has_ghost()); }
    GRlineDb *linedb()      { return (gd_linedb); }

    GhostDrawFunc get_ghost_func()  { return (gd_ghost_draw_func); }
    void set_ghost_func(GhostDrawFunc f)
    {
        gd_ghost_draw_func = f;
        gd_first_ghost = true;
    }

signals:
    void resize_event(QResizeEvent*);
    void new_painter(QPainter*);
    void paint_event(QPaintEvent*);
    void press_event(QMouseEvent*);
    void release_event(QMouseEvent*);
    void motion_event(QMouseEvent*);
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
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    void enterEvent(QEnterEvent*);
#else
    void enterEvent(QEvent*);
#endif
    void leaveEvent(QEvent*);
    void focusInEvent(QFocusEvent*);
    void focusOutEvent(QFocusEvent*);
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);

private:
    void set_color(const QColor &qc)
    {
        da_brush.setColor(qc);
        da_pen.setColor(qc);
        da_painter->setPen(da_pen);
        da_painter->setBrush(da_brush);
    }

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

    QPixmap     *da_pixmap;         // Main pixmap.
    QPixmap     *da_overlay_bg;     // Background pixmap for overlay.
    QPixmap     *da_pixmap2;        // Backing pixmap.
    QPixmap     *da_pixmap_bak;     // Mode_swap_backing.
    QPixmap     *da_tile_pixmap;    // Tiling pixmap.
    QPainter    *da_painter;        // Main painter, paints da_pixmap.
    QPainter    *da_painter2;       // Backing painter, paints da_pixmap2.
    QPainter    *da_painter_bak;    // Mode swap backing.
    QColor      da_fg;              // Foreground color.
    QColor      da_bg;              // Background color.
    QColor      da_ghost;           // Ghost color.
    QColor      da_ghost_fg;        // Ghost color ^ background.
    QBrush      da_brush;           // Solid fill brush.
    QPen        da_pen;             // Min width pen.
    int         da_tile_x;          // Tile origin x.
    int         da_tile_y;          // Tile origin y.
    bool        da_fill_mode;       // True when tiling.
    bool        da_ghost_bg_set;    // True when the ghost bg is overlay_bg
#define DA_MAX_OVERLAY 32
    char        da_overlay_count;   // Overlay nesting count.
    int         da_line_mode;       // Nonzero when using internal textured
                                    //  lines (Qt::PenStyle - 1).
                                    //  1: dashes separated by a few pixels
                                    //  2: dots separated by a few pixels
                                    //  3: alternate dots and dashes
                                    //  4: one dash, two dots, one dash,
                                    //     two dots
    const
    GRlineType *da_line_style;      // Set for user-defined texture,
                                    //  da_line_mode = 0 in this case.

    int         da_xb1, da_yb1;     // Accumulated bounding box for
    int         da_xb2, da_yb2;     //  drawing overlay.

    int         da_olx, da_oly;     // Previous accumulated overlay
    int         da_olw, da_olh;     //  bounding box.

    // Ghost drawing.
    QPixmap     *gd_overlay_bg;
    GhostDrawFunc gd_ghost_draw_func;
    GRlineDb    *gd_linedb;
    int         gd_ref_x;
    int         gd_ref_y;
    int         gd_last_x;
    int         gd_last_y;
    int         gd_ghost_cx_cnt;
    bool        gd_first_ghost;
    bool        gd_show_ghost;
    bool        gd_undraw;
    bool        gd_xor_mode;        // XOR drawing, not supported presently
};

#endif

