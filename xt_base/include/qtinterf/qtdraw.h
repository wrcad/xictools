
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
#include "qtcanvas.h"


class QFont;
class QCursor;
class QPixmap;
class QImage;
class QPaintEvent;

namespace qtinterf {
    struct sGbag;
    class QTdraw;
    class QTcanvas;
}

using namespace qtinterf;

class qtinterf::QTdraw : virtual public GRdraw
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
    void Text(const char*, int, int, int, int = -1, int = -1);
    void TextExtent(const char *str, int *w, int *h)
        { if (gd_viewport) gd_viewport->text_extent(str, w, h); }

    void SetGhost(GhostDrawFunc cb, int x, int y)
        { if (gd_viewport) gd_viewport->set_ghost(cb, x, y); }
    void ShowGhost(bool show)
        { if(gd_viewport) gd_viewport->show_ghost(show); }
    bool ShowingGhost()
        { return (gd_viewport ? gd_viewport->showing_ghost() : false); }
    void UndrawGhost(bool rst=false)
        { if (gd_viewport) gd_viewport->undraw_ghost(rst); }
    void DrawGhost(int x, int y)
        { if (gd_viewport) gd_viewport->draw_ghost(x, y); }
    void DrawGhost();

    void QueryPointer(int*, int*, unsigned*);
    void DefineColor(int*, int, int, int);
    void SetBackground(int pix)
        { if (gd_viewport) gd_viewport->set_background(pix); }
    void SetWindowBackground(int pix)
        { if (gd_viewport) gd_viewport->set_background(pix); }
    void SetGhostColor(int pix)
        { if (gd_viewport) gd_viewport->set_ghost_color(pix); }
    void SetColor(int pix)
        { if (gd_viewport) gd_viewport->set_foreground(pix); }  
    void DefineLinestyle(GRlineType*)       { }
    void SetLinestyle(const GRlineType *lt)
        { if (gd_viewport) gd_viewport->set_linestyle(lt); }
    void DefineFillpattern(GRfillType *fp)
        { if (gd_viewport) gd_viewport->define_fillpattern(fp); }
    void SetFillpattern(const GRfillType *fp)
        { if (gd_viewport) gd_viewport->set_fillpattern(fp); }

    void Refresh(int x, int y, int w, int h)
        { if (gd_viewport) gd_viewport->refresh(x, y, w, h); }
    void Refresh()
        { if (gd_viewport) gd_viewport->refresh(); }
    void Update(int x, int y, int w, int h)
        { if (gd_viewport) gd_viewport->update(x, y, w, h); }
    void Update()
        { if (gd_viewport) gd_viewport->update(); }

    void Input(int*, int*, int*, int*);
    void SetOverlayMode(bool set)
        { if (gd_viewport) gd_viewport->set_overlay_mode(set); }
    void CreateOverlayBackg()
        { if (gd_viewport) gd_viewport->create_overlay_backg(); }
    void SetXOR(int);
    void ShowGlyph(int, int, int);
    GRobject GetRegion(int, int, int, int);
    void PutRegion(GRobject, int, int, int, int);
    void FreeRegion(GRobject);
    void DisplayImage(const GRimage *im, int x, int y, int w, int h)
        { if (gd_viewport) gd_viewport->draw_image(im, x, y, w, h); }
    double Resolution()     { return (1.0); }

    qtinterf::QTcanvas *Viewport() { return (gd_viewport); }

protected:
    qtinterf::QTcanvas *gd_viewport;
};

#endif

