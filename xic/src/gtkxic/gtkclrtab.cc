
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
#include "cd_lgen.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "gtkmain.h"
#ifdef WITH_X11
#include "gtkinterf/gtkx11.h"
#endif

#ifdef WIN32
#include <windows.h>
#else

namespace {
    inline unsigned int RGB(int r, int g, int b)
    {
        return ((r & 0xff) | ((g & 0xff) << 8) | ((b & 0xff) << 16));
    }
}
#endif


// If interactive graphics is disabled, all pixels in the color table will
// be zero.  This fills in the correct value.
//
void
cMain::FixupColors(void *dp)
{
    if (RunMode() == ModeNormal)
        return;
    static enum { enc_none, enc_x, enc_triples } encoding;

#ifdef WITH_X11
    bool use_triples = false;
    Display *display = (Display*)dp;
    // If a display was passed, the pixels will be X values, otherwise the
    // pixels are encoded as RGB triples.
    if (!display) {
        display = XOpenDisplay(":0");
        if (!display)
            return;
        use_triples = true;
    }
#else
    // The argument is ignored in this case.
    (void)dp;
    bool use_triples = true;
#endif

    if ((encoding == enc_x && !use_triples) ||
            (encoding == enc_triples && use_triples))
        return;
    encoding = (use_triples ? enc_triples : enc_x);

#ifdef WITH_X11
    for (unsigned i = 0; i < ColorTableEnd; i++) {
        sColorTab::sColorTabEnt *c = DSP()->ColorTab()->color_ent(i);
        if (!c)
            continue;
        const char *colorname = c->get_defclr();
        if (!colorname)
            continue;

        int r, g, b;
        XColor rgb;
        if (sscanf(colorname, "%d %d %d", &r, &g, &b) == 3) {
            if (r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
                    b >= 0 && b <= 255) {
                rgb.red = r*256;
                rgb.green = g*256;
                rgb.blue = b*256;
            }
            else if (r >= 0 && r <= 65535 && g >= 0 && g <= 65535 &&
                    b >= 0 && b <= 65535) {
                rgb.red = r;
                rgb.green = g;
                rgb.blue = b;
            }
            else
                continue;
        }
        else if (!XParseColor(display, DefaultColormap(display, 0), colorname,
                &rgb))
            continue;
        r = rgb.red/256;
        g = rgb.green/256;
        b = rgb.blue/256;
        c->set_rgb(r, g, b);
        if (use_triples)
            *c->pixel_addr() = RGB(r, g, b);
        else if (XAllocColor(display, DefaultColormap(display, 0), &rgb))
            *c->pixel_addr() = rgb.pixel;
    }

    CDl *ld;
    CDlgen lgen(Physical, CDlgen::BotToTopWithCells);
    while ((ld = lgen.next()) != 0) {
        DspLayerParams *lp = dsp_prm(ld);
        if (use_triples)
            lp->set_pixel(RGB(lp->red(), lp->green(), lp->blue()));
        else {
            XColor rgb;
            rgb.red = lp->red() * 256;
            rgb.green = lp->green() * 256;
            rgb.blue = lp->blue() * 256;
            if (XAllocColor(display, DefaultColormap(display, 0), &rgb))
                lp->set_pixel(rgb.pixel);
        }
    }
    lgen = CDlgen(Electrical, CDlgen::BotToTopWithCells);
    while ((ld = lgen.next()) != 0) {
        DspLayerParams *lp = dsp_prm(ld);
        if (use_triples)
            lp->set_pixel(RGB(lp->red(), lp->green(), lp->blue()));
        else {
            XColor rgb;
            rgb.red = lp->red() * 256;
            rgb.green = lp->green() * 256;
            rgb.blue = lp->blue() * 256;
            if (XAllocColor(display, DefaultColormap(display, 0), &rgb))
                lp->set_pixel(rgb.pixel);
        }
    }

    if (use_triples)
        XCloseDisplay(display);

#else
    for (unsigned i = 0; i < ColorTableEnd; i++) {
        sColorTab::sColorTabEnt *c = DSP()->ColorTab()->color_ent(i);
        if (!c)
            continue;
        const char *colorname = c->get_defclr();
        if (!colorname)
            continue;

        int r, g, b;
        if (sscanf(colorname, "%d %d %d", &r, &g, &b) == 3) {
            if (r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
                    b >= 0 && b <= 255) {
                ;
            }
            else if (r >= 0 && r <= 65535 && g >= 0 && g <= 65535 &&
                    b >= 0 && b <= 65535) {
                r /= 256;
                g /= 256;
                b /= 256;
            }
            else
                continue;
        }
        else if (!GRcolorList::lookupColor(colorname, &r, &g, &b))
            continue;
        c->set_rgb(r, g, b);
        *c->pixel_addr() = RGB(r, g, b);
    }

    CDl *ld;
    CDlgen lgen(Physical, CDlgen::BotToTopWithCells);
    while ((ld = lgen.next()) != 0) {
        DspLayerParams *lp = dsp_prm(ld);
        lp->set_pixel(RGB(lp->red(), lp->green(), lp->blue()));
    }
    lgen = CDlgen(Electrical, CDlgen::BotToTopWithCells);
    while ((ld = lgen.next()) != 0) {
        DspLayerParams *lp = dsp_prm(ld);
        lp->set_pixel(RGB(lp->red(), lp->green(), lp->blue()));
    }
#endif
}

