
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
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef X11DRAW_H
#define X11DRAW_H


#define WITH_XDRAW
#define XD_NUM_LINESTYLES 16
#define XD_NUM_FILLPATTS  16

#ifdef WITH_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace ginterf
{
    struct X11draw : public GRdraw
    {
        X11draw(Display*, Window);
        virtual ~X11draw();

        void *WindowID()                                    { return (0); }
        virtual void Halt()                                     { }
        void Clear();
        virtual void ResetViewport(int, int)                    { }
        virtual void DefineViewport()                           { }
        void Dump(int)                                          { }
        void Pixel(int, int);
        void Pixels(GRmultiPt*, int);
        void Line(int, int, int, int);
        void PolyLine(GRmultiPt*, int);
        void Lines(GRmultiPt*, int);
        void Box(int, int, int, int);
        void Boxes(GRmultiPt*, int);
        void Arc(int, int, int, int, double, double);
        void Polygon(GRmultiPt*, int);
        void Zoid(int, int, int, int, int, int);
        void Text(const char*, int, int, int, int = -1, int = -1);
        void TextExtent(const char*, int*, int*);

        void SetGhost(GhostDrawFunc, int, int)                  { }
        void ShowGhost(bool)                                    { }
        bool ShowingGhost()                                 { return(false); }
        void UndrawGhost(bool = false)                          { }
        void DrawGhost(int, int)                                { }
        void DrawGhost()                                        { }

        void QueryPointer(int*, int*, unsigned*)                { }
        void DefineColor(int*, int, int, int);
        void SetBackground(int);
        void SetWindowBackground(int);
        void SetGhostColor(int)                                 { }
        void SetColor(int);
        void DefineLinestyle(GRlineType*)                       { }
        void SetLinestyle(const GRlineType*);
        void DefineFillpattern(GRfillType*);
        void SetFillpattern(const GRfillType*);
        void Refresh(int, int, int, int)                        { }
        void Refresh()                                          { }
        void Update(int, int, int, int)                         { }
        void Update();
        void Input(int*, int*, int*, int*)                      { }
        void SetOverlayMode(bool)                               { }
        void CreateOverlayBackg()                               { }
        void SetXOR(int);
        void ShowGlyph(int, int, int)                           { }
        GRobject GetRegion(int, int, int, int)              { return (0); }
        void PutRegion(GRobject, int, int, int, int)            { }
        void FreeRegion(GRobject)                               { }
        void DisplayImage(const GRimage*, int, int, int, int);
        double Resolution();

        Display *display()          { return (xd_display); }
        Window window()             { return (xd_window); }
        GC gc()                     { return (xd_gc); }
        GC xorgc()                  { return (xd_xorgc); }
        XFontStruct *font()         { return (xd_font); }

        void set_window(Window w)   { xd_window = w; }

    private:
        Display     *xd_display;
        Window      xd_window;
        GC          xd_gc;
        GC          xd_xorgc;
        XFontStruct *xd_font;
    };
}

#endif

// This definition is currently needed whether X11 is available or not.
namespace ginterf
{
    struct X11draw;

    struct Xdraw
    {
        Xdraw(const char*, unsigned long);
        ~Xdraw();

        static bool check_error();
        unsigned long create_pixmap(int, int);
        bool destroy_pixmap(unsigned long);
        bool copy_drawable(unsigned long, unsigned long, int, int, int, int,
            int, int);
        bool draw(int, int, int, int);
        bool get_drawable_size(unsigned long, int*, int*);
        unsigned long reset_drawable(unsigned long);

        void Clear();
        void Pixel(int, int);
        void Pixels(GRmultiPt*, int);
        void Line(int, int, int, int);
        void PolyLine(GRmultiPt*, int);
        void Lines(GRmultiPt*, int);
        void Box(int, int, int, int);
        void Boxes(GRmultiPt*, int);
        void Arc(int, int, int, int, double, double);
        void Polygon(GRmultiPt*, int);
        void Zoid(int, int, int, int, int, int);
        void Text(const char*, int, int, int);
        void TextExtent(const char*, int*, int*);
        void DefineColor(int*, int, int, int);
        void SetBackground(int);
        void SetWindowBackground(int);
        void SetColor(int);
        void DefineLinestyle(int, int);
        void SetLinestyle(int);
        void DefineFillpattern(int, int, int, unsigned char*);
        void SetFillpattern(int);
        void Update();
        void SetXOR(int);

    private:
        X11draw *xp;
        GRlineType  linetypes[XD_NUM_LINESTYLES];
        GRfillType  filltypes[XD_NUM_FILLPATTS];
    };
}

#endif

