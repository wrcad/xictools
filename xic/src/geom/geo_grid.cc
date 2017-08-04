
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

#include "cd.h"
#include "cd_types.h"
#include "geo_grid.h"


int grd_t::g_defgsize = -1;

// Set g_BB to the next clipping region and return a pointer to it, or
// return 0 if done.
//
const BBox *
grd_t::advance()
{
    if (g_j < 0) {
        g_i = 0;
        g_j = 0;
        if (g_gsize <= 0) {
            g_BB = g_refBB;
            return (&g_BB);
        }
        if (g_refBB.left >= CDinfinity || g_refBB.bottom >= CDinfinity)
            return (0);
        g_BB.left = g_refBB.left;
        g_BB.bottom = g_refBB.bottom;
        g_BB.right = g_BB.left + g_gsize;
        g_BB.top = g_BB.bottom + g_gsize;
        if (g_BB.right > g_refBB.right)
            g_BB.right = g_refBB.right;
        if (g_BB.top > g_refBB.top)
            g_BB.top = g_refBB.top;
        return (&g_BB);
    }
    if (g_BB.right < g_refBB.right) {
        g_j++;
        g_BB.left = g_BB.right;
        g_BB.right = g_BB.left + g_gsize;
        if (g_BB.right > g_refBB.right)
            g_BB.right = g_refBB.right;
        return (&g_BB);
    }
    if (g_BB.top < g_refBB.top) {
        g_i++;
        g_j = 0;
        g_BB.left = g_refBB.left;
        g_BB.right = g_BB.left + g_gsize;
        if (g_BB.right > g_refBB.right)
            g_BB.right = g_refBB.right;
        g_BB.bottom = g_BB.top;
        g_BB.top = g_BB.bottom + g_gsize;
        if (g_BB.top > g_refBB.top)
            g_BB.top = g_refBB.top;
        return (&g_BB);
    }
    return (0);
}

