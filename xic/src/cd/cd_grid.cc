
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

#include "cd.h"
#include "cd_types.h"
#include "cd_lgen.h"
#include "cd_chkintr.h"
#include "geo.h"
#include "geo_alloc.h"
#include <algorithm>


//-----------------------------------------------------------------------------
// Global test for off-grid vertices.
//-----------------------------------------------------------------------------

namespace {
    // Vertex element.
    struct pt_t
    {
        int tab_x()       const { return (pt_x); }
        int tab_y()       const { return (pt_y); }
        pt_t *tab_next()  const { return (pt_next); }
        void set_tab_next(pt_t *p) { pt_next = p; }
        pt_t *tgen_next(bool) const { return (pt_next); }

        void init(int x, int y)
            {
                pt_x = x;
                pt_y = y;
                pt_next = 0;
            }

    private:
        int pt_x;               // this vertex X
        int pt_y;               // this vertex Y
        pt_t *pt_next;          // table link
    };

    // Main struct for grid test.
    struct grid_check_t
    {
        grid_check_t(int s) { xytab = 0; spacing = s; }
        ~grid_check_t() { delete xytab; }

        void check_vertex(int, int);
        void dump(FILE*, const char*);

        xytable_t<pt_t> *xytab;
        int spacing;
        tGEOfact<pt_t> allocator;
    };


    void
    grid_check_t::check_vertex(int x, int y)
    {
        if (x % spacing || y % spacing) {
            if (!xytab)
                xytab = new xytable_t<pt_t>;
            pt_t *e = xytab->find(x, y);
            if (!e) {
                e = allocator.new_obj();
                e->init(x, y);
                xytab->link(e, false);
                xytab = xytab->check_rehash();
            }
        }
    }


    // Sort comparison for Point, sort descending in Y, ascending in X.
    //
    inline bool
    pt_cmp(const pt_t *p1, const pt_t *p2)
    {
        if (p1->tab_y() > p2->tab_y())
            return (true);
        if (p1->tab_y() < p2->tab_y())
            return (false);
        return (p1->tab_x() < p2->tab_x());
    }
}


void
grid_check_t::dump(FILE *fp, const char *msg)
{
    fprintf(fp, "%s\n", msg);
    if (!xytab) {
        fprintf(fp, "none found\n");
        return;
    }
    pt_t **ary = new pt_t*[xytab->allocated()];
    tgen_t<pt_t> gen(xytab);
    pt_t *e;
    int cnt = 0;
    while ((e = gen.next()) != 0)
        ary[cnt++] = e;
    std::sort(ary, ary + cnt, pt_cmp);
    for (int i = 0; i < cnt; i++)
        fprintf(fp, "x=%11.4f y=%11.4f\n",
            MICRONS(ary[i]->tab_x()), MICRONS(ary[i]->tab_y()));
    delete [] ary;
}


// This uses the pseudo-flat generator to find vertices of objects
// that are off grid.
//  sdesc       top-level cell
//  spacing     grid spacing
//  BB          area to check or null to check everywhere
//  layer_list  space-separated list of layer names, null to check all
//  skip        if true, skip lauers in layer_list
//  types       string containing 'b', 'p', 'w' for object types, or null
//  depth       hierarchy depth to search
//  fp          file pointer for output
//
bool
cCD::CheckGrid(CDs *sdesc, int spacing, const BBox *BB, const char *layer_list,
    bool skip, const char *types, int depth, FILE *fp)
{
    if (!sdesc)
        return (false);
    if (spacing < 1)
        return (false);
    if (depth < 0)
        return (false);
    if (!fp)
        return (false);

    if (!BB)
        BB = sdesc->BB();

    bool dobox =  (!types || !*types || strchr(types, 'b'));
    bool dopoly = (!types || !*types || strchr(types, 'p'));
    bool dowire = (!types || !*types || strchr(types, 'w'));

    fprintf(fp, "Off-Grid Vertex Check, %s\n", ifIdString());
    fprintf(fp, "Cell: %s\n", Tstring(sdesc->cellname()));
    fprintf(fp, "Grid: %.4f\n", MICRONS(spacing));
    if (BB)
        fprintf(fp, "BBox: %.4f,%.4f %.4f,%.4f\n", MICRONS(BB->left),
            MICRONS(BB->bottom), MICRONS(BB->right), MICRONS(BB->top));
    else
        fprintf(fp, "BBox: none\n");
    if (layer_list && *layer_list) {
        if (skip)
            fprintf(fp, "Layers (skip): %s\n", layer_list);
        else
            fprintf(fp, "Layers (only): %s\n", layer_list);
    }
    else
        fprintf(fp, "Layers: all\n");
    fprintf(fp, "Types: %s\n", types && *types ? types : "bpw");

    char buf[256];
    CDl *ld;
    CDlgen lgen(Physical);
    while ((ld = lgen.next()) != 0) {
        if (layer_list) {
            bool found = false;
            const char *ll = layer_list;
            char *tok;
            while (!found && (tok = lstring::gettok(&ll)) != 0) {
                if (ld == CDldb()->findLayer(tok))
                    found = true;
                delete [] tok;
            }
            if (found) {
                if (skip)
                    continue;
            }
            else {
                if (!skip)
                    continue;
            }
        }

        grid_check_t gc(spacing);
        sPF gen(sdesc, BB, ld, depth);

        CDo *od;
        while ((od = gen.next(false, true)) != 0) {
            if (od->type() == CDBOX) {
                if (dobox) {
                    gc.check_vertex(od->oBB().left, od->oBB().bottom);
                    gc.check_vertex(od->oBB().right, od->oBB().top);
                }
            }
            else if (od->type() == CDPOLYGON) {
                if (dopoly) {
                    const Point *pts = ((CDpo*)od)->points();
                    int num = ((CDpo*)od)->numpts();
                    for (int i = 1; i < num; i++)
                        gc.check_vertex(pts[i].x, pts[i].y);
                }
            }
            else if (od->type() == CDWIRE) {
                if (dowire) {
                    const Point *pts = ((CDw*)od)->points();
                    int num = ((CDw*)od)->numpts();
                    for (int i = 0; i < num; i++)
                        gc.check_vertex(pts[i].x, pts[i].y);
                }
            }
            delete od;
            if (checkInterrupt()) {
                fclose(fp);
                return (false);
            }
        }
        sprintf(buf, "Off grid vertices on layer %s:", ld->name());
        gc.dump(fp, buf);
    }
    return (true);
}

