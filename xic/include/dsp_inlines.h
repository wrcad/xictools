
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: dsp_inlines.h,v 5.19 2013/09/10 06:53:03 stevew Exp $
 *========================================================================*/

#ifndef DSP_INLINES_H
#define DSP_INLINES_H

//
// Inlines and defines for display windows.
//

// Current display mode
//
inline DisplayMode
cDisplay::CurMode()
{
    return (MainWdesc() ? MainWdesc()->Mode() : Physical);
}

inline void
cDisplay::SetCurMode(DisplayMode m)
{
    if (MainWdesc())
        MainWdesc()->SetMode(m);
}


// Current cell name.
//
inline CDcellName
cDisplay::CurCellName()
{
    return (MainWdesc() ? MainWdesc()->CurCellName() : 0);
}

inline void
cDisplay::SetCurCellName(CDcellName name)
{
    if (MainWdesc())
        MainWdesc()->SetCurCellName(name);
}

// Top cell name.  This differs form the current cell name when the
// window is transiently displaying a subcell (as in the Xic Push
// command).
//
inline CDcellName
cDisplay::TopCellName()
{
    return (MainWdesc() ? MainWdesc()->TopCellName() : 0);
}

inline void
cDisplay::SetTopCellName(CDcellName name)
{
    if (MainWdesc())
        MainWdesc()->SetTopCellName(name);
}


// Expose GUI functions for main window.
//
inline DSPwbag *
cDisplay::MainWbag()
{
    return (MainWdesc() ? MainWdesc()->Wbag() : 0);
}

// Expose drawing functions for main window.
//
inline GRdraw *
cDisplay::MainDraw()
{
    return (MainWdesc() ? MainWdesc()->Wdraw() : 0);
}

// The "current cell", i.e., that shown in the main window.
//
inline CDs *
CurCell(bool no_symb = false)
{
    return (DSP()->MainWdesc() ?
        DSP()->MainWdesc()->CurCellDesc(DSP()->CurMode(), no_symb) : 0);
}

// The per-mode current cell.
//
inline CDs *
CurCell(DisplayMode m, bool no_symb = false)
{
    return (DSP()->MainWdesc() ?
        DSP()->MainWdesc()->CurCellDesc(m, no_symb) : 0);
}

// Top-cell for main window, same as current cell except when
// transiently displaying a subcell as in the Xic Push command.
//
inline CDs *
TopCell(bool no_symb = false)
{
    return (DSP()->MainWdesc() ?
        DSP()->MainWdesc()->TopCellDesc(DSP()->CurMode(), no_symb) : 0);
}

// The per-mode top cell.
//
inline CDs *
TopCell(DisplayMode m, bool no_symb = false)
{
    return (DSP()->MainWdesc() ?
        DSP()->MainWdesc()->TopCellDesc(m, no_symb) : 0);
}


// Call the passed method of the main window "draw_bag".
//
#define DSPmainDraw(x) { if (DSP()->MainDraw()) DSP()->MainDraw()->x; }

// Call the passed method of the main window "widget_bag".
//
#define DSPmainWbag(x) { if (DSP()->MainWbag()) DSP()->MainWbag()->x; }

// Call the passed method of the main window "widget_bag", and pass back
// the return value.
//
#define DSPmainWbagRet(x) (DSP()->MainWbag() ? DSP()->MainWbag()->x : 0)


// Viewport (screen) coordinate test functions.
// THESE ASSUME 0,0 IS UPPER LEFT CORNER.

// Return true if BB1 intersects BB2.
inline bool ViewportIntersect(const BBox &BB1, const BBox &BB2)
    { return (BB1.left <= BB2.right && BB1.right >= BB2.left &&
    BB1.bottom >= BB2.top && BB1.top <= BB2.bottom); }

// Reteurn true if integer outside of AOI.
inline bool ViewportClipLeft(int l, const BBox *AOI)
    { return (l < AOI->left); }
inline bool ViewportClipRight(int r, const BBox *AOI)
    { return (r > AOI->right); }
inline bool ViewportClipBottom(int b, const BBox *AOI)
    { return (b > AOI->bottom); }
inline bool ViewportClipTop(int t, const BBox *AOI)
    { return (t < AOI->top); }

// Clip BB to vp.
inline void ViewportClip(BBox &BB, const BBox &vp)
    { if (BB.left < vp.left) BB.left = vp.left;
    if (BB.bottom > vp.bottom) BB.bottom = vp.bottom;
    if (BB.right > vp.right) BB.right = vp.right;
    if (BB.top < vp.top) BB.top = vp.top; }

// Set BB1 from BB2 and d.
inline void ViewportBloat(BBox &BB1, const BBox &BB2, int d)
    { BB1.left = BB2.left - d; BB1.bottom = BB2.bottom + d;
    BB1.right = BB2.right + d; BB1.top = BB2.top - d; }

#endif

