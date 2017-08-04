
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

#ifndef GEO_GRID_H
#define GEO_GRID_H

#include "geo_box.h"

// The default partition grid size in microns, and accepted range.
#define DEF_GRD_PART_SIZE 100.0
#define GRD_PART_MAX 10000.0
#define GRD_PART_MIN 1.0

// Advance a clipping box
//
struct grd_t
{
    grd_t(const BBox *fieldBB, int gridsize)
        {
            g_gsize = gridsize > 0 ? gridsize : 0;
            g_i = -1;
            g_j = -1;
            if (fieldBB)
                g_refBB = *fieldBB;
        }

    const BBox *advance();

    int cur_i()             const { return (g_i); }
    int cur_j()             const { return (g_j); }

    int numgrid() const
        {
            if (g_gsize <= 0)
                return (1);
            int nx = (g_refBB.right - g_refBB.left)/g_gsize;
            nx += ((g_refBB.right - g_refBB.left)%g_gsize != 0);
            int ny = (g_refBB.top - g_refBB.bottom)/g_gsize;
            ny += ((g_refBB.top - g_refBB.bottom)%g_gsize != 0);
            return (nx*ny);
        }

    static int def_gridsize()
        {
            // Careful! Don't want to set defgsize until the resolution
            // has been set.
            if (g_defgsize < 0)
                g_defgsize = INTERNAL_UNITS(DEF_GRD_PART_SIZE);
            return (g_defgsize);
        }

    static void set_def_gridsize(int gs) { g_defgsize = gs > 0 ? gs : 0; }

private:
    BBox g_BB;
    BBox g_refBB;
    int g_i, g_j;
    int g_gsize;

    static int g_defgsize;
};

#endif

