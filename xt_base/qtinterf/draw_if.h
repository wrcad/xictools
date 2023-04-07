
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

#ifndef DRAW_IF_H
#define DRAW_IF_H

#include "ginterf/graphics.h"

class QWidget;
class QFont;

// This is an abstract class for a simple but efficient drawing area.
// New instances are intended to be obtained from new_draw_interface().
// Note that this is not a widget, but the widget pointer is available
// through the widget() method.

namespace qtinterf
{
    enum DrawType
    {
        DrawGL,         // use OpenGL
        DrawX,          // use X-Windows direct calls
        DrawNative      // use QPainter
    };

    class draw_if
    {
    public:
        static draw_if *new_draw_interface(DrawType, bool, QWidget*);
        virtual ~draw_if() { }

        virtual QWidget *widget() = 0;

        virtual void draw_direct(bool) = 0;
        virtual void update() = 0;
        virtual void clear() = 0;
        virtual void clear_area(int, int, int, int) = 0;

        virtual void set_foreground(unsigned int) = 0;
        virtual void set_background(unsigned int) = 0;

        virtual void draw_pixel(int, int) = 0;
        virtual void draw_pixels(GRmultiPt*, int) = 0;

        virtual void set_linestyle(const GRlineType*) = 0;
        virtual void draw_line(int, int, int, int) = 0;
        virtual void draw_polyline(GRmultiPt*, int) = 0;
        virtual void draw_lines(GRmultiPt*, int) = 0;

        virtual void define_fillpattern(GRfillType*) = 0;
        virtual void set_fillpattern(const GRfillType*) = 0;
        virtual void draw_box(int, int, int, int) = 0;
        virtual void draw_boxes(GRmultiPt*, int) = 0;
        virtual void draw_arc(int, int, int, double, double) = 0;
        virtual void draw_polygon(GRmultiPt*, int) = 0;
        virtual void draw_zoid(int, int, int, int, int, int) = 0;
        virtual void draw_image(const GRimage*, int, int, int, int) = 0;

        virtual void set_font(QFont*) = 0;
        virtual int text_width(QFont*, const char*, int) = 0;
        virtual void text_extent(const char*, int*, int*) = 0;
        virtual void draw_text(int, int, const char*, int) = 0;

        virtual void set_xor_mode(bool) = 0;
        virtual void set_ghost_color(unsigned int) = 0;
    };
}

#endif

