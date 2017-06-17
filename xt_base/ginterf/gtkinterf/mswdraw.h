
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2012 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id: mswdraw.h,v 1.2 2013/04/13 02:34:21 stevew Exp $
 *========================================================================*/

#ifndef MSWDRAW_H
#define MSWDRAW_H

#include <windows.h>
#include <windowsx.h>

//
// A drawing class for native MS Windows, for use stand-alone in the
// xdraw interface and print driver under MS Windows.
//


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

struct msw_draw : virtual public GRdraw
{
    msw_draw();
    virtual ~msw_draw();

    void Halt();
    void Clear();
    void ResetViewport(int, int)                    { }
    void DefineViewport()                           { }
    void Dump(int)                                  { }
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

    void SetGhost(GhostDrawFunc, int, int)          { }
    void ShowGhost(bool) { }
    void UndrawGhost(bool = false)                  { }
    void DrawGhost(int, int)                        { }

    void MovePointer(int, int, bool)                { }
    void QueryPointer(int*, int*, unsigned*)        { }

    void DefineColor(int*, int, int, int);
    void SetBackground(int);
    void SetWindowBackground(int);
    void SetGhostColor(int)                         { }
    void SetColor(int);
    void DefineLinestyle(GRlineType*)               { }
    void SetLinestyle(const GRlineType*);
    void DefineFillpattern(GRfillType*);
    void SetFillpattern(const GRfillType*);
    void Update();
    void Input(int*, int*, int*, int*)              { }
    void SetXOR(int)                                { }
    void ShowGlyph(int, int, int);
    GRobject GetRegion(int, int, int, int);
    void PutRegion(GRobject, int, int, int, int);
    void FreeRegion(GRobject);
    void DisplayImage(const GRimage*, int, int, int, int);
    double Resolution()                             { return (1.0); }

    // non-overrides

    HDC check_dc();
    HDC InitDC(HDC = 0);
    HDC SetMemDC(HDC);
    void ReleaseDC();

    unsigned long WindowID()    { return ((unsigned long)md_window); }
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

#endif

