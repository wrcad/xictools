
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
#include "ginterf/xdraw.h"
#ifdef WITH_X11
#include <X11/Xlib.h>
#endif


//-----------------------------------------------------------------------------
// Exports to the xdraw graphics package.

namespace {
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
        phys_alloc = CDldb()->layersUsed(Physical);
        phys_pms = new void*[phys_alloc];
        memset(phys_pms, 0, phys_alloc);
        elec_alloc = CDldb()->layersUsed(Electrical);
        elec_pms = new void*[elec_alloc];
        memset(elec_pms, 0, elec_alloc);
    }
}


// Swap the cached pixmap in layer descs for one consistent with the
// present graphics package, and restore when finished.  For example,
// the pixmap may be a GdkPixmap under GTK, but we need a native X
// Pixmap within xdraw.  After setup, we can use our high-level
// drawing functions with the xdraw drawable.
//
void *
cMain::SetupLayers(void *dp, GRdraw *cx, void *scxptr)
{
    if (!scxptr) {
        // Save/set pixmaps
        FixupColors(dp);
        scx *c = new scx;
        for (int i = 0; i < c->phys_alloc; i++) {
            CDl *ld = CDldb()->layer(i, Physical);
            if (ld) {
                DspLayerParams *lp = dsp_prm(ld);
                c->phys_pms[i] = lp->fill()->xPixmap();
                lp->fill()->setXpixmap(0);
                cx->DefineFillpattern(lp->fill());
            }
        }
        for (int i = 0; i < c->elec_alloc; i++) {
            // Note that layers can be in both lists, avoid the ones
            // we've already seen.

            CDl *ld = CDldb()->layer(i, Electrical);
            if (ld && ld->physIndex() < 0) {
                DspLayerParams *lp = dsp_prm(ld);
                c->elec_pms[i] = lp->fill()->xPixmap();
                lp->fill()->setXpixmap(0);
                cx->DefineFillpattern(lp->fill());
            }
        }
        return (c);
    }
    else {
        // Put everything back
        scx *c = (scx*)scxptr;
        for (int i = 0; i < c->phys_alloc; i++) {
            CDl *ld = CDldb()->layer(i, Physical);
            if (ld) {
                DspLayerParams *lp = dsp_prm(ld);
#ifdef WITH_X11
                if (lp->fill()->xPixmap())
                    XFreePixmap((Display*)dp, (Pixmap)lp->fill()->xPixmap());
#else
#ifdef WIN32
                if (lp->fill()->xPixmap())
                    DeleteBitmap((HBITMAP)lp->fill()->xPixmap());
#endif
#ifdef WITH_QUARTZ
//XXX Need equiv. code for Quartz.
#endif
#endif
                lp->fill()->setXpixmap(c->phys_pms[i]);
            }
        }
        for (int i = 0; i < c->elec_alloc; i++) {
            // Note that layers can be in both lists, avoid the ones
            // we've already seen.

            CDl *ld = CDldb()->layer(i, Electrical);
            if (ld && ld->physIndex() < 0) {
                DspLayerParams *lp = dsp_prm(ld);
#ifdef WITH_X11
                if (lp->fill()->xPixmap())
                    XFreePixmap((Display*)dp, (Pixmap)lp->fill()->xPixmap());
#else
#ifdef WITH_QUARTZ
//XXX Need equiv. code for Quartz.
#endif
#ifdef WIN32
                if (lp->fill()->xPixmap())
                    DeleteBitmap((HBITMAP)lp->fill()->xPixmap());
#endif
#endif
                lp->fill()->setXpixmap(c->elec_pms[i]);
            }
        }
        delete c;
        return (0);
    }
}


// Rendering callback for the xdraw package
//
bool
cMain::DrawCallback(void *dp, GRdraw *cx, int l, int b, int r, int t,
    int w, int h)
{
    // So we can use this when another graphics package is active, a lot of
    // state is saved - the viewport/window, and the pixmaps in the fill
    // patterns.

    // Set attribute colors.
    FixupColors(dp);

    // init the viewport and window
    int wid = DSP()->MainWdesc()->ViewportWidth();
    int hei = DSP()->MainWdesc()->ViewportHeight();
    BBox wd = *DSP()->MainWdesc()->Window();
    double ratio = DSP()->MainWdesc()->Ratio();
    GRdraw *tcx = DSP()->MainWdesc()->Wdraw();
    DSP()->MainWdesc()->SetWdraw(cx);
    DSP()->MainWdesc()->InitViewport(w, h);
    DSP()->MainWdesc()->InitWindow((l + r)/2, (b + t)/2, r - l);

    // init the layers
    void *c = SetupLayers(dp, cx, 0);

    DSP()->MainWdesc()->RedisplayDirect();

    // return layers to prev state
    SetupLayers(dp, cx, c);

    DSP()->MainWdesc()->InitViewport(wid, hei);
    *DSP()->MainWdesc()->Window() = wd;
    DSP()->MainWdesc()->SetRatio(ratio);
    DSP()->MainWdesc()->SetWdraw(tcx);
    return (true);
}

