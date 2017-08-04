
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

#ifndef DSP_GRID_H
#define DSP_GRID_H


// The maximum value accepter for the manufacturing grid.
#define GRD_MFG_GRID_MAX 100.0

// Axes rendering styles.
//
enum AxesType { AxesMark, AxesPlain, AxesNone };

// Grid description.
//
struct GridDesc
{
    GridDesc()
        {
            g_spacing = 1.0;
            g_snap = 1;
            g_axes = AxesNone;
            g_displayed = false;
            g_on_top = false;
            g_cross_size = 0;
            g_coarse_mult = 5;
        }

    bool operator==(const GridDesc g) const
        {
            if (g_spacing != g.g_spacing || g_snap != g.g_snap || 
                    g_linestyle.mask != g.g_linestyle.mask ||
                    g_coarse_mult != g.g_coarse_mult)
                return (false);
            if (g_linestyle.mask == 0 && (g_cross_size != g.g_cross_size))
                return (false);
            return (true);
        }

    bool operator!=(const GridDesc g) const
        {
            if (g_spacing != g.g_spacing || g_snap != g.g_snap || 
                    g_linestyle.mask != g.g_linestyle.mask ||
                    g_coarse_mult != g.g_coarse_mult)
                return (true);
            if (g_linestyle.mask == 0 && (g_cross_size != g.g_cross_size))
                return (true);
            return (false);
        }

    // Return true if the window should be redrawn when changing grids.
    bool visually_differ(const GridDesc *g)
        {
            if (g_displayed != g->g_displayed)
                return (true);
            if (!g_displayed)
                return (false);
            if (g_on_top != g->g_on_top)
                return (true);
            return (*this != *g);
        }

    // This corrects for the manufacturing grid, if g_spacing is
    // off-grid.
    double spacing(DisplayMode m) const
        {
            double mgrd = mfg_grid(m);
            if (mgrd == 0.0)
                return (g_spacing);
            int r = mmRnd(g_spacing/mgrd);
            if (r < 1)
                r = 1;
            return (r*mgrd);
        }
    // This is the resolution given by the caller.
    //
    double base_spacing()     const { return (g_spacing); }
    void set_spacing(double r)      { if (r > 0.0) g_spacing = r; }

    GRlineType &linestyle()         { return (g_linestyle); }
    void set_linestyle(const GRlineType &t)
                                    { g_linestyle = t; }

    // If g_snap > 0, grid lines will be at g_snap*g_spacing
    // intervals.  Otherwise, grid lines will be at
    // g_spacing/abs(g_snap) intervals.
    int snap()                const { return (g_snap); }
    void set_snap(int s)
        {
            if (s >= -10 && s <= 10 && s != 0)
                g_snap = s;
        }

    AxesType axes()           const { return ((AxesType)g_axes); }
    void set_axes(AxesType a)       { g_axes = a; }

    bool displayed()          const { return (g_displayed); }
    void set_displayed(bool b)      { g_displayed = b; }

    bool show_on_top()        const { return (g_on_top); }
    void set_show_on_top(bool b)    { g_on_top = b; }

    // If the line mask is 0, the displayed grid will consist of dots
    // or crosses.  The size of the cross is given by g_cross_size,
    // which is the pixel count above and below, right and left, of
    // the grid point.  E.g., 1 will draw the five pixels -1,0, 0,0,
    // 1,0, 0,-1, and 0,1.

#define GRD_MAX_CRS 6

    int dotsize() const
        {
            return (g_linestyle.mask == 0 ? (int)g_cross_size : -1);
        }

    void set_dotsize(unsigned int n)
        {
            g_cross_size = (n <= GRD_MAX_CRS ? n : GRD_MAX_CRS);
        }

    int coarse_mult()         const { return (g_coarse_mult); }
    void set_coarse_mult(unsigned int m)
        {
            if (m && m <= 50)
                g_coarse_mult = m;
        }

    // dsp_grid.cc
    void set(const GridDesc&);
    char *desc_string();
    static bool parse(const char*, GridDesc&);

    // Snap points will always fall on a multiple of the manufacturing
    // grid, if it is nonzero.
    //
    static double mfg_grid(DisplayMode m)
        {
            return (m == Physical ? g_phys_mfg_grid : g_elec_mfg_grid);
        }
    static void set_mfg_grid(double d, DisplayMode m)
        {
            if (d >= 0.0 && d <= GRD_MFG_GRID_MAX) {
                if (m == Physical) {
                    double m1 = MICRONS(1);
                    g_phys_mfg_grid = d < m1 ? m1 : d;
                }
                else {
                    double m1 = ELEC_MICRONS(1);
                    g_elec_mfg_grid = d < m1 ? m1 : d;
                }
            }
        }

private:
    double g_spacing;       // Spacing between snap points, as set by the
                            // caller, may be off mfg_grid.
    GRlineType g_linestyle; // Storage for grid line style.
    signed char g_snap;     // The number of points per grid interval to which
                            //  a cursor input point is snapped.  If negative,
                            //  the number of grid lines per snap line.
    unsigned char g_axes;   // Axes presentation style.
    bool g_displayed;       // Grid is being displayed.
    bool g_on_top;          // Grid is shown above geometry.
    unsigned short g_cross_size; // Extension size for cross, when in dots mode.
    unsigned short g_coarse_mult;// Coarse grid multiple.

    static double g_phys_mfg_grid;
    static double g_elec_mfg_grid;
                            // The returned spacing is always a multiple
                            // of this value, if nonzero.
};

#endif

