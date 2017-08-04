
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
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

