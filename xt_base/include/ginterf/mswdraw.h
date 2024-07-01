
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

#ifndef MSWDRAW_H
#define MSWDRAW_H
#ifdef WIN32

#include <windows.h>
#include <windowsx.h>

//
// A drawing class for native MS Windows, for use stand-alone in the
// xdraw interface and print driver under MS Windows.
//

namespace ginterf
{
    struct sGbagMsw
    {
        sGbagMsw() {
            gb_linestyle = 0;
            gb_fillpattern = 0;
        }


        void set_linestyle(const GRlineType *ls) { gb_linestyle = ls; }
        const GRlineType *get_linestyle()       { return (gb_linestyle); }
        void set_fillpattern(const GRfillType *fp) { gb_fillpattern = fp; }
        const GRfillType *get_fillpattern()     { return (gb_fillpattern); }

    private:
        const GRlineType *gb_linestyle;
        const GRfillType *gb_fillpattern;
    };

    struct MSWdraw : public GRdraw
    {
        MSWdraw();
        virtual ~MSWdraw();

        void *WindowID()                            { return (md_window); }
        virtual void Halt()                                     { }
        void Clear();
        virtual void ResetViewport(int, int)                    { }
        virtual void DefineViewport()                           { }
        void Dump(int)                                          { }
        void Pixel(int, int);
        void Pixels(GRmultiPt*, int);
        void Line(int, int, int, int);
        void LinePrv(int, int, int, int);
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
        void ShowGhost(bool) { }
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
        void SetXOR(int)                                        { }
        void ShowGlyph(int, int, int);
        GRobject GetRegion(int, int, int, int);
        void PutRegion(GRobject, int, int, int, int);
        void FreeRegion(GRobject);
        void DisplayImage(const GRimage*, int, int, int, int);
        double Resolution()                                 { return (1.0); }

        // non-overrides

        HDC check_dc();
        HDC InitDC(HDC = 0);
        HDC SetMemDC(HDC);
        void ReleaseDC();

        void set_monochrome(bool b) { md_blackout = b; }

        HWND Window()               { return (md_window); }
        void SetWindow(HWND w)      { md_window = w; }
        COLORREF GhostForeground()  { return (md_ghost_foreg); }

        void SetBackgPixel(COLORREF p)    { md_backg = p; }
        COLORREF GetBackgPixel()          { return (md_backg); }
        void SetForegPixel(COLORREF p)    { md_foreg = p; }
        COLORREF GetForegPixel()          { return (md_foreg); }

    protected:
        static HPEN new_pen(COLORREF, const GRlineType*);

        sGbagMsw *md_gbag;          // graphics rendering context
        HWND md_window;             // drawing window
        HDC md_memDC;               // backing bitmap DC
        COLORREF md_foreg;
        COLORREF md_backg;
        COLORREF md_ghost_foreg;    // ghost drawing color, this is actually
                                    //  ~backg in 256-color mode
        bool md_blackout;           // enforce monochrome output

        static HDC md_lastDC;
        static HWND md_lastHWND;
    };
}

#endif
#endif

