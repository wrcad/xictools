
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

#ifndef DSP_LAYER_H
#define DSP_LAYER_H


//
// Diaplay-related things to attach to a layer descriptor.
//

// Outline styles
//  FILLED set:  use fill pattern.
//      Solid thin line outline if OUTLINE set.
//      No outline if OUTLINE not set.
//  FILLED unset:  no fill pattern.
//      Solid thin line outline if OUTLINE not set.
//      Dashed thin line outline if OUTLINE set.
//      Solid fat line outline if OUTLINE and OUTLINE_FAT set.

struct DspLayerParams
{
    DspLayerParams(CDl*);
    ~DspLayerParams();

    void defaultColor();
    void setColor(int, int, int);
    static void hsv_to_rgb(double, double, double, double*, double*, double*);

    GRfillType *fill()                  { return (&lp_fill); }

    int red()                           const { return (lp_red); }
    int green()                         const { return (lp_green); }
    int blue()                          const { return (lp_blue); }
    void set_red(int c)                 { lp_red = c; }
    void set_green(int c)               { lp_green = c; }
    void set_blue(int c)                { lp_blue = c; }

    int pixel()                         const { return (lp_pixel); }
    void set_pixel(int c)               { lp_pixel = c; }

    int dim_pixel()                     const { return (lp_dim_pixel); }
    void set_dim_pixel(int c)           { lp_dim_pixel = c; }

    int xsect_thickness()               const { return (lp_xsect_thickness); }
    double thickness()                  const { return (lp_thickness); }
    int wire_width()                    const { return (lp_wire_width); }
    void set_xsect_thickness(int t)     { lp_xsect_thickness = t; }
    void set_thickness(double d)        { lp_thickness = d; }
    void set_wire_width(int w)          { lp_wire_width = w; }

private:
    GRfillType lp_fill;                 // fill data
    int lp_red, lp_green, lp_blue;      // RGB color
    unsigned int lp_pixel;              // Colormap pixel
    unsigned int lp_dim_pixel;          // Dim pixel, for anti-highlighting
    int lp_xsect_thickness;             // thickness for cross-section display
    double lp_thickness;                // film thickness, microns
    int lp_wire_width;                  // default wire width

    static int internal_pixel;      // common default internal lyr. pix.
    static int alloc_count;         // default color assignment counter
};


// Return the params struct.
//
inline DspLayerParams *
dsp_prm(const CDl *ld)
{
    return (static_cast<DspLayerParams*>(ld->dspData()));
}

#endif

