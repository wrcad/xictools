
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
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef GRLINEDB_H
#define GRLINEDB_H

#include "symtab.h"


// Line clipper for XOR drawing.
//
// When using XOR mode for drawing ghost objects, if all or part of a
// line is drawn an even number of times, it will disappear, which is
// surprising and confusing to the user.
//
// This is a database that is used when XOR drawing to ensure that
// lines are drawn once only.


namespace ginterf {
    // List element of a horizontal or vertical line segment.
    //
    struct llist_t
    {
        void set(int v1, int v2)
            {
                if (v1 < v2) {
                    l_vmin = v1;
                    l_vmax = v2;
                }
                else {
                    l_vmin = v2;
                    l_vmax = v1;
                }
                l_next = 0;
            }

        void set_next(llist_t *n)             { l_next = n; }
        llist_t *next()                 const { return (l_next); }

        int vmin()                      const { return (l_vmin); }
        void set_vmin(int v)                  { l_vmin = v; }
        int vmax()                      const { return (l_vmax); }
        void set_vmax(int v)                  { l_vmax = v; }

    private:
        llist_t *l_next;
        int l_vmin;
        int l_vmax;
    };

    // List element for non-Manhattan line segment.
    //
    struct nmllist_t
    {
        void set(int xx1, int yy1, int xx2, int yy2)
            {
                nm_x1 = xx1;
                nm_y1 = yy1;
                nm_x2 = xx2;
                nm_y2 = yy2;
            }

        void set_next(nmllist_t *n)           { nm_next = n; }
        nmllist_t *next()               const { return (nm_next); }

        int x1()                        const { return (nm_x1); }
        int y1()                        const { return (nm_y1); }
        int x2()                        const { return (nm_x2); }
        int y2()                        const { return (nm_y2); }

    private:
        int nm_x1;
        int nm_y1;
        int nm_x2;
        int nm_y2;
        nmllist_t *nm_next;
    };

    // Table element.
    //
    struct lelt_t
    {
        unsigned long tab_key()         { return (key); }
        lelt_t *tab_next()              { return (next); }
        void set_tab_next(lelt_t *n)    { next = n; }

        unsigned long key;
        lelt_t *next;
        llist_t *list;
    };


    // Line database.  The add calls return the part of the line that
    // in not in the database, which is also added to the database.
    //
    struct GRlineDb
    {
        GRlineDb()
            {
                ldb_nm_lines = 0;
                ldb_hline_tab = 0;
                ldb_vline_tab = 0;
                ldb_nm_line_store = 0;
            }

        ~GRlineDb()
            {
                delete ldb_hline_tab;
                delete ldb_vline_tab;
            }

        const llist_t *add_vert(int, int, int);
        const llist_t *add_horz(int, int, int);
        const nmllist_t *add_nm(int, int, int, int);

    private:
        llist_t *clip_out(llist_t*, const llist_t*);
        llist_t *merge(llist_t*, llist_t*);
        llist_t *copy(const llist_t*);

        nmllist_t           *ldb_nm_lines;
        itable_t<lelt_t>    *ldb_hline_tab;
        itable_t<lelt_t>    *ldb_vline_tab;
        nmllist_t           *ldb_nm_line_store;
        eltab_t<lelt_t>     ldb_elt_factory;
        eltab_t<llist_t>    ldb_list_factory;
        eltab_t<nmllist_t>  ldb_nmlist_factory;
    };
}

#endif

