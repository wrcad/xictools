
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

#ifndef QTDRAW_H
#define QTDRAW_H

#include "ginterf/graphics.h"
#include "ginterf/grlinedb.h"

class QWidget;
class QFont;
class QCursor;
class QPixmap;
class QImage;

// This is an abstract class for a simple but efficient drawing area.
// New instances are intended to be obtained from new_draw_interface().
// Note that this is not a widget, but the widget pointer is available
// through the widget() method.


// The X11Extras don't seem to be available on MacPorts.
#ifndef __APPLE__
//#define WITH_X11
#endif
#ifdef WITH_X11
//#include <X11/Xlib.h>
#endif

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
        virtual QPixmap *pixmap() = 0;

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
        virtual void draw_arc(int, int, int, int, double, double) = 0;
        virtual void draw_polygon(GRmultiPt*, int) = 0;
        virtual void draw_zoid(int, int, int, int, int, int) = 0;
        virtual void draw_image(const GRimage*, int, int, int, int) = 0;

        virtual void set_font(QFont*) = 0;
        virtual int text_width(QFont*, const char*, int) = 0;
        virtual void text_extent(const char*, int*, int*) = 0;
        virtual void draw_text(int, int, const char*, int) = 0;

        virtual void set_xor_mode(bool) = 0;
        virtual void set_ghost_color(unsigned int) = 0;

        virtual void set_draw_to_pixmap(QPixmap*) = 0;
        virtual void draw_pixmap(int, int, QPixmap*, int, int, int, int) = 0;
        virtual void draw_image(int, int, QImage*, int, int, int, int) = 0;
    };

    // Encapsulation of the window ghost-drawing capability.
    //
    struct sGdraw
    {
        sGdraw() {
            gd_draw_ghost = 0;
            gd_linedb = 0;
            gd_ref_x = 0;
            gd_ref_y = 0;
            gd_last_x = 0;
            gd_last_y = 0;
            gd_ghost_cx_cnt = 0;
            gd_first_ghost = false;
            gd_show_ghost = false;
            gd_undraw = false;
        }

        void set_ghost(GhostDrawFunc, int, int);
        void show_ghost(bool);
        void undraw_ghost(bool);
        void draw_ghost(int, int);

        bool has_ghost()    { return (gd_draw_ghost != 0); }
        bool showing()      { return (gd_show_ghost); }
        GRlineDb *linedb()  { return (gd_linedb); }

        GhostDrawFunc get_ghost_func()  { return (gd_draw_ghost); }
        void set_ghost_func(GhostDrawFunc f)
            {
                gd_draw_ghost = f;
                gd_first_ghost = true;
            }

    private:
        GhostDrawFunc gd_draw_ghost;
        GRlineDb *gd_linedb;        // line clipper for XOR mode
        int gd_ref_x;
        int gd_ref_y;
        int gd_last_x;
        int gd_last_y;
        int gd_ghost_cx_cnt;
        bool gd_first_ghost;
        bool gd_show_ghost;
        bool gd_undraw;
    };

    // Graphical context, may be used by multiple windows.
    struct sGbag
    {
        sGbag()
        {
            gb_draw_if = 0;
            gb_cursor_type = 0;
        }

        void set_draw_if(draw_if *bkptr)
        {
            gb_draw_if = bkptr;
        }

        void set_xor(bool xormode)
        {
            if (gb_draw_if)
                gb_draw_if->set_xor_mode(xormode);
        }

        void set_ghost(GhostDrawFunc cb, int x, int y)
        {
            set_xor(true);
            gb_gdraw.set_ghost(cb, x, y);
            set_xor(false);
        }

        void show_ghost(bool show)
        {
            set_xor(true);
            gb_gdraw.show_ghost(show);
            set_xor(false);
        }

        void undraw_ghost(bool rst)
        {
            set_xor(true);
            gb_gdraw.undraw_ghost(rst);
            set_xor(false);
        }

        void draw_ghost(int x, int y)
        {
            set_xor(true);
            gb_gdraw.draw_ghost(x, y);
            set_xor(false);
        }

        bool has_ghost()
        {
            return (gb_gdraw.has_ghost());
        }

        bool showing_ghost()
        {
            return (gb_gdraw.showing());
        }

        GhostDrawFunc get_ghost_func()
        {
            return (gb_gdraw.get_ghost_func());
        }

        void set_ghost_func(GhostDrawFunc f)
        {
            gb_gdraw.set_ghost_func(f);
        }

        void set_cursor_type(unsigned int t)    { gb_cursor_type = t; }
        unsigned int get_cursor_type()          { return (gb_cursor_type); }
        GRlineDb *linedb()                      { return (gb_gdraw.linedb()); }

        static sGbag *default_gbag(int = 0);

#define NUMGCS 10
        static sGbag *app_gbags[NUMGCS];

        draw_if *gb_draw_if;
        sGdraw gb_gdraw;
        unsigned int gb_cursor_type;
    };

    class QTdraw : virtual public GRdraw
    {
    public:
        QTdraw(int);
        virtual ~QTdraw() { }

        // GRdraw virtual overrides
        void *WindowID()                { return (0); }
        void Halt();
        void Clear()
            { if (gd_viewport) gd_viewport->clear(); }
        void ResetViewport(int, int)    { }
        void DefineViewport()           { }
        void Dump(int)                  { }

        void Pixel(int x, int y)
            { if (gd_viewport) gd_viewport->draw_pixel(x, y); }
        void Pixels(GRmultiPt *p, int n)
            { if (gd_viewport) gd_viewport->draw_pixels(p, n); }
        void Line(int x1, int y1, int x2, int y2)
            { if (gd_viewport) gd_viewport->draw_line(x1, y1, x2, y2); }
        void PolyLine(GRmultiPt *p, int n)
            { if (gd_viewport) gd_viewport->draw_polyline(p, n); }
        void Lines(GRmultiPt *p, int n)
            { if (gd_viewport) gd_viewport->draw_lines(p, n); }
        void Box(int x1, int y1, int x2, int y2)
            { if (gd_viewport) gd_viewport->draw_box(x1, y1, x2, y2); }
        void Boxes(GRmultiPt *p, int n)
            { if (gd_viewport) gd_viewport->draw_boxes(p, n); }
        void Arc(int x, int y, int rx, int ry, double a1, double a2)
            { if (gd_viewport) gd_viewport->draw_arc(x, y, rx, ry, a1, a2); }
        void Polygon(GRmultiPt *p, int n)
            { if (gd_viewport) gd_viewport->draw_polygon(p, n); }
        void Zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
            { if (gd_viewport)
                gd_viewport->draw_zoid(yl, yu, xll, xul, xlr, xur); }
        void Text(const char *str, int x, int y, int, int = -1, int = -1)
            { if (gd_viewport) gd_viewport->draw_text(x, y, str, -1); }
        void TextExtent(const char *str, int *w, int *h)
            { if (gd_viewport) gd_viewport->text_extent(str, w, h); }

        void SetGhost(GhostDrawFunc cb, int x, int y)
                                            { gd_gbag->set_ghost(cb, x, y); }
        void ShowGhost(bool show)           { gd_gbag->show_ghost(show); }
        void UndrawGhost(bool reset = false)
                                            { gd_gbag->undraw_ghost(reset); }
        void DrawGhost(int x, int y)        { gd_gbag->draw_ghost(x, y); }

        void MovePointer(int, int, bool);
        void QueryPointer(int*, int*, unsigned*);
        void DefineColor(int*, int, int, int);
        void SetBackground(int pix)
            { if (gd_viewport) gd_viewport->set_background(pix); }
        void SetWindowBackground(int pix)
            { if (gd_viewport) gd_viewport->set_background(pix); }
        void SetGhostColor(int); 
        void SetColor(int pix)
            { if (gd_viewport) gd_viewport->set_foreground(pix); }  
        void DefineLinestyle(GRlineType*)       { }
        void SetLinestyle(const GRlineType *lt)
            { if (gd_viewport) gd_viewport->set_linestyle(lt); }
        void DefineFillpattern(GRfillType *fp)
            { if (gd_viewport) gd_viewport->define_fillpattern(fp); }
        void SetFillpattern(const GRfillType *fp)
            { if (gd_viewport) gd_viewport->set_fillpattern(fp); }
        void Update()
            { if (gd_viewport) gd_viewport->update(); }

        void Input(int*, int*, int*, int*);
        void SetXOR(int);
        void ShowGlyph(int, int, int);
        GRobject GetRegion(int, int, int, int);
        void PutRegion(GRobject, int, int, int, int);
        void FreeRegion(GRobject);
        void DisplayImage(const GRimage *im, int x, int y, int w, int h)
            { if (gd_viewport) gd_viewport->draw_image(im, x, y, w, h); }
        double Resolution() { return (1.0); }

        sGbag *Gbag()           { return (gd_gbag); }
        void SetGbag(sGbag *b)  { gd_gbag = b; }
        QWidget *Viewport()     { return (gd_viewport->widget()); }

protected:
        sGbag   *gd_gbag;       // graphics rendering context
        draw_if *gd_viewport;   // drawing area
    };
}

#endif

