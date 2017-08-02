
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
#include "dsp_layer.h"
#include "dsp_tkif.h"
#include "si_parsenode.h"


//
// Application-specific container for layers.
//

int DspLayerParams::internal_pixel;
int DspLayerParams::alloc_count;

DspLayerParams::DspLayerParams(CDl *ld)
{
    if (ld->layerType() == CDLcellInstance) {
        lp_red = 0;
        lp_green = 0;
        lp_blue = 0;
        lp_pixel = 0;
    }
    else if (ld->layerType() == CDLinternal) {
        defaultColor();
        // internal layer, these share a colormap pixel
        if (internal_pixel == 0 || !dspPkgIf()->IsDualPlane())
            GRpkgIf()->AllocateColor(&internal_pixel,
                lp_red, lp_green, lp_blue);
        lp_pixel = internal_pixel;
    }
    else {
        defaultColor();
        GRpkgIf()->AllocateColor((int*)&lp_pixel, lp_red, lp_green, lp_blue);
    }
    lp_dim_pixel = 0;
    lp_xsect_thickness = 0;
    lp_thickness = 0.0;
    lp_wire_width = 0;
}


DspLayerParams::~DspLayerParams()
{
    // Nothing to do currently.
}


// Assign a color for a new layer.
//
void
DspLayerParams::defaultColor()
{
    double h = fmod(alloc_count * 111.0, 360.0);
    double s = 0.6;
    double v = 0.85;
    double r = 0.0, g = 0.0, b = 0.0;
    hsv_to_rgb(h, s, v, &r, &g, &b);
    lp_red = mmRnd(r*255);
    lp_green = mmRnd(g*255);
    lp_blue = mmRnd(b*255);
    alloc_count++;
}


void
DspLayerParams::setColor(int r, int g, int b)
{
    lp_red = r;
    lp_green = g;
    lp_blue = b;
    GRpkgIf()->AllocateColor((int*)&lp_pixel, lp_red, lp_green, lp_blue);
}


// Static function.
//
void
DspLayerParams::hsv_to_rgb(double h, double s, double v,
    double *r, double *g, double *b)
{
    if (s < 0.0)
        s = 0.0;
    else if (s > 1.0)
        s = 1.0;
    if (v < 0.0)
        v = 0.0;
    else if (v > 1.0)
        v = 1.0;

    if (s == 0.0)
        s = 0.000001;

    if (h < 0 || h >= 360.0)
        h = 0.0;
    h = h / 60.0;
    int i = (int)h;
    double f = h - i;
    double w = v * (1.0 - s);
    double q = v * (1.0 - (s * f));
    double t = v * (1.0 - (s * (1.0 - f)));

    switch (i) {
    case 0:
        *r = v;
        *g = t;
        *b = w;
        break;
    case 1:
        *r = q;
        *g = v;
        *b = w;
        break;
    case 2:
        *r = w;
        *g = v;
        *b = t;
        break;
    case 3:
        *r = w;
        *g = q;
        *b = v;
        break;
    case 4:
        *r = t;
        *g = w;
        *b = v;
        break;
    case 5:
        *r = v;
        *g = w;
        *b = q;
        break;
    }
}
// End of DspLayerParams functions

