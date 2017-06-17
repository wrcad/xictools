
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
 $Id: geo_grid.cc,v 1.6 2009/03/08 05:42:07 stevew Exp $
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

