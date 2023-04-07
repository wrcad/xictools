
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

#include "main.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "qtmain.h"
#include "xdraw.h"


//-----------------------------------------------------------------------------
// Exports to the xdraw graphics package.

// Struct to hold layer pixmaps, for SetupLayers()
struct scx
{
    scx();

    void **phys_pms;
    void **elec_pms;
    int phys_alloc;
    int elec_alloc;
};

scx::scx()
{
    phys_alloc = CD()->PhysLayers()->used();
    phys_pms = new void*[phys_alloc];
    memset(phys_pms, 0, phys_alloc);
    elec_alloc = CD()->ElecLayers()->used();
    elec_pms = new void*[elec_alloc];
    memset(elec_pms, 0, elec_alloc);
}


void *
cMain::SetupLayers(void *dp, GRdraw *cx, void *dptr)
{
/*
    if (!dptr) {
        // Save/set pixmaps
        FixupColors(dp);
        scx *c = new scx;
        for (int i = 0; i < c->phys_alloc; i++) {
            CDl *ld = CD()->PhysLayers.layer(i);
            if (ld) {
                c->phys_pms[i] = ld->fill.xpixmap;
                ld->fill.xpixmap = 0;
                cx->DefineFillpattern(&ld->fill);
            }
        }
        for (int i = 0; i < c->elec_alloc; i++) {
            CDl *ld = CD()->ElecLayers.layer(i);
            if (ld) {
                c->elec_pms[i] = ld->fill.xpixmap;
                ld->fill.xpixmap = 0;
                cx->DefineFillpattern(&ld->fill);
            }
        }
        return (c);
    }
    else {
        // Put everything back
        scx *c = (scx*)dptr;
        Display *display = (Display*)dp;
        for (int i = 0; i < c->phys_alloc; i++) {
            CDl *ld = CD()->PhysLayers.layer(i);
            if (ld) {
                if (ld->fill.xpixmap)
                    XFreePixmap(display, (Pixmap)ld->fill.xpixmap);
                ld->fill.xpixmap = c->phys_pms[i];
            }
        }
        for (int i = 0; i < c->elec_alloc; i++) {
            CDl *ld = CD()->ElecLayers.layer(i);
            if (ld) {
                if (ld->fill.xpixmap)
                    XFreePixmap(display, (Pixmap)ld->fill.xpixmap);
                ld->fill.xpixmap = c->elec_pms[i];
            }
        }
        delete c;
        return (0);
    }
*/
(void)dp;
(void)cx;
(void)dptr;
return (0);
}


// Rendering callback for the xdraw package
//
bool
cMain::DrawCallback(void *dp, GRdraw *cx, int l, int b, int r, int t,
    int w, int h)
{
/*
    Display *display = (Display*)dp;
    // Se we can use this when another graphics package is active, a lot of
    // state is saved - the viewport/window, and the pixmaps in the fill
    // patterns

    // Set attribute colors
    FixupColors(display);

    // init the viewport and window
    BBox vp = DSP()->windows[0]->w_viewport;
    BBox wd = DSP()->windows[0]->w_window;
    double ratio = DSP()->windows[0]->w_ratio;
    GRdraw *tcx = DSP()->windows[0]->w_draw;
    DSP()->windows[0]->w_draw = cx;
    DSP()->windows[0]->w_viewport.left = 0;
    DSP()->windows[0]->w_viewport.top = 0;
    DSP()->windows[0]->w_viewport.right = w;
    DSP()->windows[0]->w_viewport.bottom = h;
    DSP()->windows[0]->InitWindow((l + r)/2, (b + t)/2, r - l);

    // init the layers
    void *c = SetupLayers(display, cx, 0);

    DSP()->windows[0]->RedisplayDirect();

    // return layers to prev state
    SetupLayers(display, cx, c);

    DSP()->windows[0]->w_viewport = vp;
    DSP()->windows[0]->w_window = wd;
    DSP()->windows[0]->w_ratio = ratio;
    DSP()->windows[0]->w_draw = tcx;
*/
(void)dp;
(void)cx;
(void)l;
(void)b;
(void)r;
(void)t;
(void)w;
(void)h;

    return (true);
}

