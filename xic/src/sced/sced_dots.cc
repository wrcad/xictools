
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

#include "main.h"
#include "sced.h"
#include "dsp_inlines.h"
#include "cd_celldb.h"
#include "cd_lgen.h"
#include "miscutil/symtab.h"


/**************************************************************************
 *
 * Dot command.  Place dots at connections.
 *
 **************************************************************************/

enum VerType {VerTerminal, VerWireVertex, VerCellTerm, VerDot};

// Table element for vertices.
//
namespace {
    struct dvtx_t
    {
        int tab_x()                     { return (vt_x); }
        int tab_y()                     { return (vt_y); }
        VerType tab_type()              { return (vt_type); }
        void set_tab_type(VerType t)    { vt_type = t; }
        void set_vals(int x, int y, VerType t)
                                        { vt_x = x; vt_y = y; vt_type = t; }
        void dup_vals(const dvtx_t *v)  { vt_x = v->vt_x; vt_y = v->vt_y;
                                          vt_type = v->vt_type; vt_next = 0; }
        dvtx_t *tab_next()              { return (vt_next); }
        void set_tab_next(dvtx_t *n)    { vt_next = n; }
        dvtx_t *tgen_next(bool)         { return (tab_next()); }

    private:
        dvtx_t *vt_next;
        int vt_x;
        int vt_y;
        VerType vt_type;
    };

    // Table element for a cell, holds a vertex table.
    //
    struct dotc_t
    {
        uintptr_t tab_key()             { return (dc_key); }
        void set_tab_key(CDcellName  nm) { dc_key = (uintptr_t)nm; }

        dotc_t *tab_next()              { return (dc_next); }
        void set_tab_next(dotc_t *n)    { dc_next = n; }

        xytable_t<dvtx_t> *dot_tab()    { return (dc_dots); }
        void set_dot_tab(xytable_t<dvtx_t> *t)
                                        { dc_dots = t; }
        dotc_t *tgen_next(bool)         { return (tab_next()); }

        void set_dirty(bool b)          { dc_dirty = b; }
        bool is_dirty()                 { return (dc_dirty); }

    private:
        dotc_t *dc_next;
        uintptr_t dc_key;
        xytable_t<dvtx_t> *dc_dots;
        bool dc_dirty;
    };


    // Main class for dot generation.
    struct DotGen
    {
        DotGen()
            {
                dg_table = 0;
                dg_tmpvtx_table = 0;
                dg_gc_cnt = 0;
            }

        void recompute_dots();
        void update_dots(CDs*, CDo*);
        bool update_dot(CDs*, int, int);
        void redisplay_dot(int, int);
        void clear_dots();
        void rename_cell(CDcellName, CDcellName);
        void set_dirty(const CDs*);
        void update_dirty();

    private:
        void create_dots(CDs*);
        void save_vertex(int, int, VerType);
        void add_dots(CDs*);

        itable_t<dotc_t>   *dg_table;
        xytable_t<dvtx_t>  *dg_tmpvtx_table;

        eltab_t<dvtx_t>     dg_vtx_fct;
        eltab_t<dvtx_t>     dg_tmpvtx_fct;
        eltab_t<dotc_t>     dg_dotc_fct;
        int                 dg_gc_cnt;

        static short DotP[];
    };

    DotGen Dgen;

    const short DotDelta = (30*CDelecResolution)/100;
    const short DD23 = (2*DotDelta)/3;
}

short DotGen::DotP[] = {
    short(0),           short(DotDelta),
    short(DD23),        short(DD23),
    short(DotDelta),    short(0),
    short(DD23),        short(-DD23),
    short(0),           short(-DotDelta),
    short(-DD23),       short(-DD23),
    short(-DotDelta),   short(0),
    short(-DD23),       short(DD23),
    short(0),           short(DotDelta)
};


// Clear and recompute the dot locations.  No redisplay is done.  This
// is called after a new cell is opened for editing (not push/pop)
// after the new XM()->top_symbol has been set, and before the initial
// redisplay.  Will be called in either mode.
//
void
cSced::recomputeDots()
{
    Dgen.recompute_dots();
}


// Update the dots associated with odesc.  Called for every new object
// after creation, and for deleted objects after the Info field has
// been set to CDDeleted.  Called only in Electrical mode for
// electrical odescs.
//
void
cSced::updateDots(CDs *sdesc, CDo *odesc)
{
    if (sc_show_dots != DotsNone)
        Dgen.update_dots(sdesc, odesc);
}


// Set a flag to indicate that the connections in the cell have changed,
// and dots need to be recomputed.
//
void
cSced::dotsSetDirty(const CDs *sdesc)
{
    if (sc_show_dots != DotsNone)
        Dgen.set_dirty(sdesc);
}


// Update the cells that have been marked as dirty.
//
void
cSced::dotsUpdateDirty()
{
    if (sc_show_dots != DotsNone)
        Dgen.update_dirty();
}


// Clear the dot list, and delete the "dots" from the database.
//
void
cSced::clearDots()
{
    Dgen.clear_dots();
}


void
cSced::updateDotsCellName(CDcellName oldname, CDcellName newname)
{
    Dgen.rename_cell(oldname, newname);
}
// End od cSced functions.


// Clear and recompute the dot locations, if there are any electrical
// windows open.  This is called after a new cell is opened for
// editing (not push/pop) after the new XM()->top_symbol has been set,
// and before the initial redisplay, and when ShowDots is set.  Will
// be called in either mode.
//
void
DotGen::recompute_dots()
{
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    WindowDesc *wd;
    bool cleared = false; 
    while ((wd = wgen.next()) != 0) {
        if (wd->Mode() != Electrical)
            continue;
        if (!cleared) {
            clear_dots();
            cleared = true;
        }
        else if (dg_table && dg_table->find(wd->TopCellName()))
            continue;
        CDs *topc = CDcdb()->findCell(wd->TopCellName(), Electrical);
        if (topc) {
            CDgenHierDn_s gen(topc);
            CDs *sd;
            while ((sd = gen.next()) != 0)
                create_dots(sd);
        }
    }
}


// Update dots associated with odesc.
//
void
DotGen::update_dots(CDs *sdesc, CDo *odesc)
{
    if (!sdesc || !odesc || sdesc->isSymbolic())
        return;
    bool redraw = (sdesc == CurCell(Electrical));
    if (odesc->type() == CDWIRE) {
        const Point *pts = OWIRE(odesc)->points();
        int num = OWIRE(odesc)->numpts();
        for (int i = 0; i < num; i++) {
            if (update_dot(sdesc, pts[i].x, pts[i].y) && redraw)
                redisplay_dot(pts[i].x, pts[i].y);
        }
    }
    else if (odesc->type() == CDINSTANCE) {
        CDp_cnode *pn = (CDp_cnode*)odesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int x, y;
                if (!pn->get_pos(ix, &x, &y))
                    break;
                if (update_dot(sdesc, x, y) && redraw)
                    redisplay_dot(x, y);
            }
        }
        if (!dg_table || !dg_table->find(((CDc*)odesc)->cellname())) {
            // Haven't seen this cell before, create dots.
            CDs *sd = CDcdb()->findCell(((CDc*)odesc)->cellname(),
                Electrical);
            if (sd)
                create_dots(sd);
        }
    }
}


// See if a dot should be shown at x, y.  Update the dot list
// accordingly.  Called after topological modification, in Electrical
// mode.  True is returned if a dot was added or removed.
//
bool
DotGen::update_dot(CDs *sdesc, int x, int y)
{
    if (!sdesc)
        return (false);

    // Add a little slop.
    BBox BB(x, y, x, y);
    BB.bloat(10);

    // Process wires.
    CDg gdesc;
    CDsLgen gen(sdesc);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        gdesc.init_gen(sdesc, ld, &BB);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (!odesc->is_normal())
                continue;
            if (odesc->type() == CDWIRE) {
                const Point *pts = OWIRE(odesc)->points();
                int num = OWIRE(odesc)->numpts();
                for (int i = 0; i < num; i++) {
                    if (x == pts[i].x && y == pts[i].y) {
                        save_vertex(x, y, (i == 0 || i == num-1) ?
                            VerTerminal : VerWireVertex);
                        break;
                    }
                }
            }
        }
    }

    // Process devices and subcircuits.
    gdesc.init_gen(sdesc, CellLayer(), &BB);
    CDo *odesc;
    while ((odesc = gdesc.next()) != 0) {
        if (!odesc->is_normal())
            continue;
        CDp_cnode *pn = (CDp_cnode*)odesc->prpty(P_NODE);
        bool saved = false;
        for ( ; pn; pn = pn->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int xx, yy;
                if (!pn->get_pos(ix, &xx, &yy))
                    break;
                if (x == xx && y == yy) {
                    save_vertex(x, y, VerTerminal);
                    saved = true;
                    break;
                }
            }
            if (saved)
                break;
        }
    }

    // The tmpvtx table now has one entry max.  If there is no entry,
    // or if is not a VerDot entry, delete x, y from the visible dots
    // list.  If it is a VerDot entry, create a new dot, if it is not
    // already there.

    bool ret = false;
    tgen_t<dvtx_t> vgen(dg_tmpvtx_table);
    dvtx_t *v = vgen.next();
    if (!v || v->tab_type() != VerDot) {
        // Remove this dot.
        if (dg_table) {
            dotc_t *dc = dg_table->find(sdesc->cellname());
            if (dc) {
                xytable_t<dvtx_t> *vt = dc->dot_tab();
                if (vt)
                    vt->remove(x, y);
            }
        }

        // Clear the dot object.
        CDsLgen xgen(sdesc);
        while ((ld = xgen.next()) != 0) {
            if (!ld->isWireActive())
                continue;
            gdesc.init_gen(sdesc, ld, &BB);
            while ((odesc = gdesc.next()) != 0) {
                if (odesc->type() != CDPOLYGON)
                    continue;
                if (odesc->state() != CDobjInternal)
                    continue;
                // Someday may need a flag to indicate dots uniquely.
                if (!sdesc->unlink(odesc, false))
                    Errs()->get_error();
                ret = true;
                break;
            }
        }
    }
    else {
        // Add dot at x, y if not already present.
        if (!dg_table)
            dg_table = new itable_t<dotc_t>;
        dotc_t *dc = dg_table->find(sdesc->cellname());
        if (!dc) {
            dc = dg_dotc_fct.new_element();
            dc->set_tab_next(0);
            dc->set_tab_key(sdesc->cellname());
            dc->set_dot_tab(0);
            dc->set_dirty(false);
            dg_table->link(dc, false);
            dg_table = dg_table->check_rehash();
        }
        xytable_t<dvtx_t> *vtab = dc->dot_tab();
        if (!vtab)
            vtab = new xytable_t<dvtx_t>;

        v = vtab->find(x, y);
        if (!v) {
            v = dg_vtx_fct.new_element();
            v->set_tab_next(0);
            v->set_vals(x, y, VerDot);
            vtab->link(v, false);

            Poly poly(9, new Point[9]);
            for (int i = 0; i < 9; i++)
                poly.points[i].set(x + DotP[2*i], y + DotP[2*i+1]);

            CDlgen xgen(Electrical);
            while ((ld = xgen.next()) != 0) {
                if (ld->isWireActive())
                    break;
            }
            if (ld)
                sdesc->makePolygon(ld, &poly, 0, 0, true);
            vtab = vtab->check_rehash();
            ret = true;
        }
        dc->set_dot_tab(vtab);
    }

    delete dg_tmpvtx_table;
    dg_tmpvtx_table = 0;

    // We garbage collect this periodically.
    dg_gc_cnt++;
    if (!(dg_gc_cnt & 255))
        dg_tmpvtx_fct.clear();
    return (ret);
}


// Redisplay a dot-sized area of the current cell centered at x, y.
//
void
DotGen::redisplay_dot(int x, int y)
{
    BBox BB(x, y, x, y);
    BB.bloat(DotDelta + 10);
    DSP()->RedisplayArea(&BB);
}


void
DotGen::clear_dots()
{
    tgen_t<dotc_t> cgen(dg_table);
    dotc_t *dc;
    while ((dc = cgen.next()) != 0) {
        CDcellName cellname = (CDcellName)dc->tab_key();
        CDs *sdesc = CDcdb()->findCell(cellname, Electrical);
        if (sdesc) {
            tgen_t<dvtx_t> vgen(dc->dot_tab());
            dvtx_t *v;
            while ((v = vgen.next()) != 0) {
                BBox BB(v->tab_x(), v->tab_y(), v->tab_x(), v->tab_y());
                BB.bloat(10);
                CDg gdesc;
                CDsLgen gen(sdesc);
                CDl *ld;
                while ((ld = gen.next()) != 0) {
                    if (!ld->isWireActive())
                        continue;
                    gdesc.init_gen(sdesc, ld, &BB);
                    CDo *odesc;
                    while ((odesc = gdesc.next()) != 0) {
                        if (odesc->type() != CDPOLYGON)
                            continue;
                        if (odesc->state() != CDobjInternal)
                            continue;
                        // Someday may need a flag to indicate dots uniquely.
                        if (!sdesc->unlink(odesc, false))
                            Errs()->get_error();
                        break;
                    }
                }
            }
        }
        delete dc->dot_tab();
    }
    delete dg_table;
    dg_table = 0;
    dg_vtx_fct.clear();
    dg_dotc_fct.clear();
}


// The name of a cell has changed, deal with it.
//
void
DotGen::rename_cell(CDcellName oldname, CDcellName newname)
{
    if (!dg_table)
        return;
    dotc_t *dotc = dg_table->remove(oldname);
    if (dotc) {
        dotc->set_tab_key(newname);
        dg_table->link(dotc);
    }
}


// Set the dirty flag for sdesc, this indicates that the dots need to
// be recomputed.
//
void
DotGen::set_dirty(const CDs *sdesc)
{
    if (!sdesc || !dg_table)
        return;
    dotc_t *dotc = dg_table->find(sdesc->cellname());
    if (dotc)
        dotc->set_dirty(true);
}


// Update the cells marked as dirty.
//
void
DotGen::update_dirty()
{
    tgen_t<dotc_t> cgen(dg_table);
    dotc_t *dc;
    while ((dc = cgen.next()) != 0) {
        if (dc->is_dirty()) {
            CDcellName cellname = (CDcellName)dc->tab_key();
            CDs *sdesc = CDcdb()->findCell(cellname, Electrical);
            if (sdesc)
                create_dots(sdesc);
        }
    }
}


// Create a list of all wire vertices and subcell/device terminals. 
// Then call add_dots() to look through the list and potentially
// create dots at the connection points.
//
void
DotGen::create_dots(CDs *sdesc)
{
    if (!sdesc)
        return;
    SCD()->fixPaths(sdesc);

    // Process wires
    CDg gdesc;
    CDsLgen gen(sdesc);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        gdesc.init_gen(sdesc, ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (!odesc->is_normal())
                continue;
            if (odesc->type() == CDWIRE) {
                const Point *pts = OWIRE(odesc)->points();
                int num = OWIRE(odesc)->numpts();
                save_vertex(pts[0].x, pts[0].y, VerTerminal);
                for (int i = 1; i < num-1; i++)
                    save_vertex(pts[i].x, pts[i].y, VerWireVertex);
                save_vertex(pts[num-1].x, pts[num-1].y, VerTerminal);
            }
        }
    }

    // Process devices and subcircuits
    gdesc.init_gen(sdesc, CellLayer());
    CDo *odesc;
    while ((odesc = gdesc.next()) != 0) {
        if (!odesc->is_normal())
            continue;
        CDp_cnode *pn = (CDp_cnode*)odesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int x, y;
                if (!pn->get_pos(ix, &x, &y))
                    break;
                save_vertex(x, y, VerTerminal);
            }
        }
    }

    // Mark VerWireVertex at cell connection points with a dot.
    CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {
        int x, y;
        pn->get_schem_pos(&x, &y);
        save_vertex(x, y, VerCellTerm);
    }

    add_dots(sdesc);
}


// Save point in the tmpvtx table.  If it is already there, update the
// type field or mark it for inclusion in the dot list.
//
void
DotGen::save_vertex(int x, int y, VerType type)
{
    if (!dg_tmpvtx_table)
        dg_tmpvtx_table = new xytable_t<dvtx_t>;
    dvtx_t *v = dg_tmpvtx_table->find(x, y);
    if (v) {
        // Already a vertex at this location.
        if (v->tab_type() != VerDot) {
            // If both types are VerTerminal, update the stored entry
            // to type VerWireVertex (so as not to mark two abutting
            // line segments or device terminals).
            //
            // Draw a dot at a cell terminal over a VerWireVertex.
            // This marks the location, and a dot might not appear
            // there in subcircuits otherwise.
            //
            if (SCD()->showingDots() != DotsAll &&
                    (v->tab_type() == VerTerminal && type == VerTerminal))
                v->set_tab_type(VerWireVertex);
            else if (type != VerCellTerm || v->tab_type() == VerWireVertex)
                v->set_tab_type(VerDot);
        }
        return;
    }

    v = dg_tmpvtx_fct.new_element();
    v->set_tab_next(0);
    v->set_vals(x, y, type);
    dg_tmpvtx_table->link(v, false);
    dg_tmpvtx_table = dg_tmpvtx_table->check_rehash();
}


// Process the tmpvtx table, using this to update or create the table
// saved with the cell pointer.  Create the database dots.  It is
// probably not good to use database polygons, but it simplifies
// redraw handling.
//
// If a table for the cell exists, it will be updated, otherwise it will
// be created.
//
// Note that discarded table elements can't be freed, and can accumulate
// as dead storage.  They will be gc'ed when the entire table is rebuilt.
//
// This clears the tmpvtx table.
//
void
DotGen::add_dots(CDs *sdesc)
{
    if (!sdesc)
        return;
    dotc_t *dc = 0;
    if (dg_table)
        dc = dg_table->find(sdesc->cellname());
    if (!dg_tmpvtx_table) {
        if (dc) {
            dg_table->unlink(dc);
            delete dc->dot_tab();
            dc->set_dot_tab(0);
        }
        return;
    }

    if (dc) {
        tgen_t<dvtx_t> gen(dc->dot_tab());
        dvtx_t *v;
        while ((v = gen.next()) != 0) {
            dvtx_t *vx = dg_tmpvtx_table->find(v->tab_x(), v->tab_y());
            if (vx && vx->tab_type() == VerDot)
                dg_tmpvtx_table->unlink(vx);
            else {
                dc->dot_tab()->unlink(v);
                BBox BB(v->tab_x(), v->tab_y(), v->tab_x(), v->tab_y());
                BB.bloat(10);
                CDg gdesc;
                CDsLgen xgen(sdesc);
                CDl *ld;
                while ((ld = xgen.next()) != 0) {
                    if (!ld->isWireActive())
                        continue;
                    gdesc.init_gen(sdesc, ld, &BB);
                    CDo *odesc;
                    while ((odesc = gdesc.next()) != 0) {
                        if (odesc->type() != CDPOLYGON)
                            continue;
                        if (odesc->state() != CDobjInternal)
                            continue;
                        // Someday may need a flag to indicate dots uniquely.
                        if (!sdesc->unlink(odesc, false))
                            Errs()->get_error();
                        break;
                    }
                }
            }
        }
    }

    CDlgen lgen(Electrical);
    CDl *lddot;
    while ((lddot = lgen.next()) != 0) {
        if (lddot->isWireActive())
            break;
    }

    Poly poly;
    poly.numpts = 9;
    dvtx_t *v;
    tgen_t<dvtx_t> gen(dg_tmpvtx_table);
    while ((v = gen.next()) != 0) {
        if (v->tab_type() == VerDot) {
            dvtx_t *vn = dg_vtx_fct.new_element();
            vn->dup_vals(v);
            if (!dc) {
                if (!dg_table)
                    dg_table = new itable_t<dotc_t>;
                dc = dg_dotc_fct.new_element();
                dc->set_tab_next(0);
                dc->set_tab_key(sdesc->cellname());
                dc->set_dot_tab(new xytable_t<dvtx_t>);
                dc->set_dirty(false);
                dg_table->link(dc, false);
                dg_table = dg_table->check_rehash();
            }
            dc->dot_tab()->link(vn, false);
            dc->set_dot_tab(dc->dot_tab()->check_rehash());

            // Create visible dot as database polygon.
            poly.points = new Point[9];
            for (int i = 0; i < 9; i++) {
                poly.points[i].set(
                    v->tab_x() + DotP[2*i], v->tab_y() + DotP[2*i+1]);
            }
            if (lddot)
                sdesc->makePolygon(lddot, &poly, 0, 0, true);
        }
    }

    if (dc)
        dc->set_dirty(false);

    // Were done with this.
    delete dg_tmpvtx_table;
    dg_tmpvtx_table = 0;
    dg_tmpvtx_fct.clear();
    dg_gc_cnt = 0;
}

