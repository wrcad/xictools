
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: geo_grid.h,v 5.8 2010/01/26 01:31:01 stevew Exp $
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

