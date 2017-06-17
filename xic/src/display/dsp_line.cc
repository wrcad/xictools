
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
 $Id: dsp_line.cc,v 1.16 2016/05/28 06:24:18 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_inlines.h"


//
// Functions to display lines.
//

// Draw a line, clipped to the window's clip_rect.
//
void
WindowDesc::ShowLine(int x1, int y1, int x2, int y2)
{
    if (w_draw) {
        DSP()->TPoint(&x1, &y1);
        DSP()->TPoint(&x2, &y2);
        LToP(x1, y1, x1, y1);
        LToP(x2, y2, x2, y2);
        if (!cGEO::line_clip(&x1, &y1, &x2, &y2, &w_clip_rect))
            w_draw->Line(x1, y1, x2, y2);
    }
}


// Draw a line, clipped to the window's viewport.
//
void
WindowDesc::ShowLineW(int x1, int y1, int x2, int y2)
{
    if (w_draw) {
        DSP()->TPoint(&x1, &y1);
        DSP()->TPoint(&x2, &y2);
        LToP(x1, y1, x1, y1);
        LToP(x2, y2, x2, y2);
        if (!cGEO::line_clip(&x1, &y1, &x2, &y2, &Viewport()))
            w_draw->Line(x1, y1, x2, y2);
    }
}


// Draw a line, clipped to the window's viewport, passing viewport
// coordinates.
//
void
WindowDesc::ShowLineV(int x1, int y1, int x2, int y2)
{
    if (w_draw) {
        if (!cGEO::line_clip(&x1, &y1, &x2, &y2, &Viewport()))
            w_draw->Line(x1, y1, x2, y2);
    }
}


// Ghost draw a cross mark, d is the length of the arms in pixels.  If
// d is negative, use the absolute value as the base for log scaling.
//
void
WindowDesc::ShowCross(int x, int y, int d, bool do45)
{
    if (!w_draw || !d)
        return;
    if (d < 0)
        d = LogScaleToPix(-d);
    DSP()->TPoint(&x, &y);
    LToP(x, y, x, y);
    if (do45) {
        ShowLineV(x-d, y-d, x+d, y+d);
        ShowLineV(x-d, y+d, x+d, y-d);
    }
    else {
        ShowLineV(x-d, y, x+d, y);
        ShowLineV(x, y-d, x, y+d);
    }
}

