
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


inline int
encode(int r, int g, int b)
{
    return ( ((b & 0xff) << 16) | ((g & 0xff) << 8) | (r & 0xff) );
}


// If interactive graphics is disabled, all pixels in the color table will
// be zero.  This fills in the correct value
//
void
cMain::FixupColors(void *dp)
{
/*
    if (XM()->RunMode == ModeNormal)
        return;
    static enum { enc_none, enc_x, enc_triples } encoding;
    Display *display = (Display*)dp;
    // If a display was passed, the pixels will be X values, otherwise the
    // pixels are encoded as RGB triples
    bool use_triples = false;
    if (!display) {
        display = XOpenDisplay(":0");
        if (!display)
            return;
        use_triples = true;
    }
    if ((encoding == enc_x && !use_triples) ||
            (encoding == enc_triples && use_triples))
        return;
    encoding = (use_triples ? enc_triples : enc_x);
    for (unsigned i = 0; i < ColorTableEnd; i++) {
        const char *colorname = ColorTab[i].get_defclr();
        if (colorname) {
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
            else if (!XParseColor(display, DefaultColormap(display, 0),
                    colorname, &rgb))
                continue;
            colors[i].red = rgb.red/256;
            colors[i].green = rgb.green/256;
            colors[i].blue = rgb.blue/256;
            if (use_triples)
                colors[i].pixel = encode(colors[i].red, colors[i].green,
                    colors[i].blue);
            else if (XAllocColor(display, DefaultColormap(display, 0), &rgb))
                colors[i].pixel = rgb.pixel;
        }
    }

    CDl *ld;
    CDlgen lgen(Physical, 0);
    while ((ld = lgen.next()) != 0) {
        if (use_triples)
            ld->pixel = encode(ld->red, ld->green, ld->blue);
        else {
            XColor rgb;
            rgb.red = ld->red * 256;
            rgb.green = ld->green * 256;
            rgb.blue = ld->blue * 256;
            if (XAllocColor(display, DefaultColormap(display, 0), &rgb))
                ld->pixel = rgb.pixel;
        }
    }
    lgen = CDlgen(Electrical, 0);
    while ((ld = lgen.next()) != 0) {
        if (use_triples)
            ld->pixel = encode(ld->red, ld->green, ld->blue);
        else {
            XColor rgb;
            rgb.red = ld->red * 256;
            rgb.green = ld->green * 256;
            rgb.blue = ld->blue * 256;
            if (XAllocColor(display, DefaultColormap(display, 0), &rgb))
                ld->pixel = rgb.pixel;
        }
    }

    if (use_triples)
        XCloseDisplay(display);
*/
(void)dp;
}

