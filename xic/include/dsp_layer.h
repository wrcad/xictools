
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

    int red()                           { return (lp_red); }
    void set_red(int c)                 { lp_red = c; }
    int green()                         { return (lp_green); }
    void set_green(int c)               { lp_green = c; }
    int blue()                          { return (lp_blue); }
    void set_blue(int c)                { lp_blue = c; }

    int pixel()                         { return (lp_pixel); }
    void set_pixel(int c)               { lp_pixel = c; }

    int dim_pixel()                     { return (lp_dim_pixel); }
    void set_dim_pixel(int c)           { lp_dim_pixel = c; }

    int xsect_thickness()               { return (lp_xsect_thickness); }
    void set_xsect_thickness(int d)     { lp_xsect_thickness = d; }
    double thickness()                  { return (lp_thickness); }
    void set_thickness(double d)        { lp_thickness = d; }
    int wire_width()                    { return (lp_wire_width); }
    void set_wire_width(int d)          { lp_wire_width = d; }

private:
    GRfillType lp_fill;                 // fill data
    int lp_red, lp_green, lp_blue;      // RGB color
    unsigned int lp_pixel;              // Colormap pixel
    unsigned int lp_dim_pixel;          // Dim pixel, for anti-highlighting
    int lp_xsect_thickness;             // thickness for cross-section display
    float lp_thickness;                 // film thickness, microns
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

