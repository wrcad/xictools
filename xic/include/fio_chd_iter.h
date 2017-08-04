
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

#ifndef FIO_CHD_ITER_H
#define FIO_CHD_ITER_H


class cSDB;

// This is a base class for iterating over the area of a CHD cell, to
// perform some action.  The exact action would be implemented in the
// derived class, primarily in the iterFunc method.
//
// The area of interest can be specified explicitly (in the run
// method), or if not specified defaults to the area of the cell given
// (also in run).  This is split into a coarse grid.  For each coarse
// grid cell, a Zbdb special database is read from the chd, which
// subdivides the coarse grid into a find grid, with an optional bloat
// value.  The sizes of the grids, and bloat value, are specified to
// the constructor.
//
// The iterFunc is called for each coarse grid cell.  This will do
// some kind of processing on the fine grid zoid lists.
//
// The info method can return information about the data that is saved
// in memory, before finalize.
//
// The finalize method is intended to perform final actions and
// clean-up.  It takes an integer argument which can be used for mode
// flags.

struct cv_chd_iter
{
    cv_chd_iter(int cgm, int fg, int bv)
        {
            ci_chd1 = 0;
            ci_chd2 = 0;
            ci_symref1 = 0;
            ci_symref2 = 0;
            ci_db1 = 0;
            ci_db2 = 0;

            ci_coarse_grid = cgm*fg;
            ci_fine_grid = fg;
            ci_bloat_val = bv;
            ci_endit = false;
        }

    virtual ~cv_chd_iter() { }

    XIrt run(cCHD*, const char*, const BBox*);
    XIrt run2(cCHD*, const char*, cCHD*, const char*, const BBox*);

protected:
    virtual XIrt iterFunc(const BBox*) = 0;

    cCHD *ci_chd1;
    cCHD *ci_chd2;
    symref_t *ci_symref1;
    symref_t *ci_symref2;
    cSDB *ci_db1;
    cSDB *ci_db2;
    BBox ci_aoiBB;
    int ci_coarse_grid;
    int ci_fine_grid;
    int ci_bloat_val;
    bool ci_endit;
};

#endif

