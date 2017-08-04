
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

#include "fio.h"
#include "fio_info.h"
#include "fio_symref.h"


//-----------------------------------------------------------------------------
// Saved hierarchy info.
//-----------------------------------------------------------------------------

namespace {
#ifdef WIN32
    const char *count_format = "%-8s %12I64d %12I64d %12I64d %10.3lf\n";
#else
    const char *count_format = "%-8s %12lld %12lld %12lld %10.3lf\n";
#endif

    void HEADER(char *buf)
        {
            sprintf(buf, "%-8s %12s %12s %12s %10s\n", "Layer", "Boxes",
                "Polys", "Wires", "Avg Verts");
        }
}


cv_info::~cv_info()
{
    // Have to cycle through the allocated pc_data structs and clear
    // the tables.  This is NOT done in the table destructor or the
    // factory destructor.
    //
    if (pcdata) {
        tgen_t<pc_data> gen(pcdata);
        pc_data *pcd;
        while ((pcd = gen.next()) != 0)
            delete pcd->data_tab();
    }

    delete pcdata;
    delete pldata;
}


// Virtual function.
// Initialize all fields.
//
void
cv_info::initialize()
{
    lname_tab.clear();
    delete pcdata;
    delete pldata;

    rec_cnt = 0;
    cell_cnt = 0;
    text_cnt = 0;
    sref_cnt = 0;
    aref_cnt = 0;

    cur_lname = 0;
    cur_pcdata = 0;
    pcdata = 0;
    pldata = 0;
}


// Allocate a list struct where the strings are replaced by name table
// entries.  The passed stringlist is consumed.  This is used with
// cv_info::has_geom.
//
info_lnames_t *
cv_info::get_info_lnames(stringlist *sl0)
{
    stringlist *sp = 0, *sn;
    for (stringlist *sl = sl0; sl; sl = sn) {
        sn = sl->next;
        const char *ln = lname_tab.find(sl->string);
        delete [] sl->string;
        sl->string = (char*)ln;
        if (!ln) {
            if (sp)
                sp->next = sn;
            else
                sl0 = sn;
            delete sl;
            continue;
        }
        sp = sl;
    }
    if (sl0)
        return (new info_lnames_t(sl0));
    return (0);
};


// Return true if p has a geometry entry for one of the layers in ifln
// if skip_layers is false, or if p has a geometry entry for a layer
// not in ifln if skip_layers is true.
//
bool
cv_info::has_geom(symref_t *p, info_lnames_t *ifln, bool skip_layers)
{
    if (!pcdata)
        // Hmmm, no tables, assume everything has geometry.
        return (true);
    pc_data *pcd = pcdata->find(p->get_name());
    if (!pcd || !pcd->data_tab()) {
        // Hmmm, no table for this symref, assume it is empty.
        return (false);
    }
    if (skip_layers) {
        tgen_t<pl_data> gen(pcd->data_tab());
        pl_data *lyr;
        while ((lyr = gen.next()) != 0) {
            bool found = false;
            for (stringlist *sl = ifln->names(); sl; sl = sl->next) {
                if (lyr->tab_key() == (unsigned long)sl->string) {
                    found = true;
                    break;
                }
            }
            if (!found)
                return (true);
        }
    }
    else {
        for (stringlist *sl = ifln->names(); sl; sl = sl->next) {
            if (pcd->data_tab()->find(sl->string))
                return (true);
        }
    }
    return (false);
}


// Record a new cell read.
//
void
cv_info::add_cell(const symref_t *p)
{
    cell_cnt++;
    if (p) {
        if (!pcdata) {
            if (enable_per_cell)
                pcdata = new itable_t<pc_data>;
            else
                return;
        }
        pc_data *pcd = pcdata->find(p->get_name());
        if (!pcd) {
            pcd = new_pcdata(p);
            pcdata->link(pcd);
            pcdata = pcdata->check_rehash();
        }
        cur_pcdata = pcd;
    }
}


// Add a layer, called for layer records.  Establishes the "current"
// layer.
//
void
cv_info::add_layer(const char *lname)
{
    cur_lname = lname_tab.add(lname);
    if (!enable_per_layer)
        cur_lname = 0;
}


// Record a box.
//
void
cv_info::add_box(int n)
{
    if (!pldata)
        pldata = new itable_t<pl_data>;
    pl_data *pld = pldata->find(cur_lname);
    if (!pld) {
        pld = new_pldata();
        pldata->link(pld);
        pldata = pldata->check_rehash();
    }
    pld->add_box(n);
    if (cur_pcdata)
        cur_pcdata->add_box(this, n);
}


// Record a polygon.
//
void
cv_info::add_poly(int nverts)
{
    if (!pldata)
        pldata = new itable_t<pl_data>;
    pl_data *pld = pldata->find(cur_lname);
    if (!pld) {
        pld = new_pldata();
        pldata->link(pld);
        pldata = pldata->check_rehash();
    }
    pld->add_poly(nverts);
    if (cur_pcdata)
        cur_pcdata->add_poly(this, nverts);
}


// Record a wire.
//
void
cv_info::add_wire(int nverts)
{
    if (!pldata)
        pldata = new itable_t<pl_data>;
    pl_data *pld = pldata->find(cur_lname);
    if (!pld) {
        pld = new_pldata();
        pldata->link(pld);
        pldata = pldata->check_rehash();
    }
    pld->add_wire(nverts);
    if (cur_pcdata)
        cur_pcdata->add_wire(this, nverts);
}


// If cname is null, return a list of all layers seen in the file.  If
// the save mode is cvINFOplpc, and a nonzero cname is passed, the
// return will be a list of the layers used in the named cell, or 0 if
// not found or the mode is not cvINFOplpc, or no layers exist in the
// cell.
//
stringlist *
cv_info::layers(const char *cname)
{
    if (!cname || !*cname)
        return (lname_tab.strings());
    if (!enable_per_cell || !enable_per_layer || !pcdata)
        return (0);

    CDcellName cn = CD()->CellNameTableFind(cname);
    if (!cn)
        return (0);
    pc_data *pcd  = pcdata->find(cn);
    if (!pcd || !pcd->data_tab())
        return (0);

    stringlist *s0 = 0;
    tgen_t<pl_data> gen(pcd->data_tab());
    pl_data *pld;
    while ((pld = gen.next()) != 0)
        s0 = new stringlist(lstring::copy(pld->layer_name()), s0);
    return (s0);
}


// Return a list of all cells seen in the file.  This returns nonzero
// only for cvINFOpc or cvINFOplpc.
//
stringlist *
cv_info::cells()
{
    if (!enable_per_cell || !pcdata)
        return (0);

    stringlist *s0 = 0;
    tgen_t<pc_data> gen(pcdata);
    pc_data *pcd;
    while ((pcd = gen.next()) != 0)
        s0 = new stringlist(lstring::copy(pcd->cellname()), s0);
    return (s0);
}


// Return the pl_data according to the arguments.  The return depends
// on the cvINFO mode.
//
// cvINFOtotals
// Both arguments are ignored, the return provides file totals.
//
// cvINFOpl
// The cname argument is ignored.  If lname is 0, the return provides
// file totals.  Otherwise, the return provides totals for lname, if
// found.  If given and not found, the return is 0.
//
// cvINFOpc
// The lname argument is ignored.  If cname is 0, the return
// represents file totals.  Otherwise, the return provides totals for
// cname, if found.  If not found, 0 is returned.
//
// cvINFOplpc
// If both arguments are 0, the return represents file totals.  If
// cname is 0, the return rpresents totals for the layer given, or 0
// if not found.  If lname is 0, the return provides totals for the
// cell name given, or 0 if not found.  If both names are given, the
// return provides totals for the given layer in the given cell.
//
// The returned object is a *copy*, caller must delete!
//
pl_data *
cv_info::info(const char *cname, const char *lname)
{
    cvINFO mode = savemode_prv();
    if (mode == cvINFOtotals) {
        if (!pldata)
            return (0);
        tgen_t<pl_data> gen(pldata);
        pl_data *pld = gen.next();
        if (!pld)
            return (0);
        pl_data *dn = new pl_data(*pld);
        dn->set_tab_next(0);
        return (dn);
    }
    if (mode == cvINFOpl) {
        if (!pldata)
            return (0);
        if (lname && *lname) {
            const char *ln = lname_tab.find(lname);
            if (!ln)
                return (0);
            pl_data *pld = pldata->find(ln);
            if (!pld)
                return (0);
            pl_data *dn = new pl_data(*pld);
            dn->set_tab_next(0);
            return (dn);
        }
        pl_data *dn = new pl_data;
        dn->set_name(0);

        tgen_t<pl_data> gen(pldata);
        pl_data *pld;
        while ((pld = gen.next()) != 0)
            dn->totalize(pld);
        return (dn);
    }
    if (mode == cvINFOpc) {
        if (cname && *cname) {
            if (!pcdata)
                return (0);
            CDcellName cn = CD()->CellNameTableFind(cname);
            if (!cn)
                return (0);
            pc_data *pcd = pcdata->find(cn);
            if (!pcd || !pcd->data_tab())
                return (0);
            tgen_t<pl_data> gen(pcd->data_tab());
            pl_data *pld = gen.next();
            if (!pld)
                return (0);
            pl_data *dn = new pl_data(*pld);
            dn->set_tab_next(0);
            return (dn);
        }
        if (!pldata)
            return (0);
        tgen_t<pl_data> gen(pldata);
        pl_data *pld = gen.next();
        if (!pld)
            return (0);
        pl_data *dn = new pl_data(*pld);
        dn->set_tab_next(0);
        return (dn);
    }
    if (mode == cvINFOplpc) {
        if (cname && *cname) {
            if (!pcdata)
                return (0);
            CDcellName cn = CD()->CellNameTableFind(cname);
            if (!cn)
                return (0);
            pc_data *pcd = pcdata->find(cn);
            if (!pcd || !pcd->data_tab())
                return (0);
            if (lname && *lname) {
                const char *ln = lname_tab.find(lname);
                if (!ln)
                    return (0);
                pl_data *pld = pcd->data_tab()->find(ln);
                if (!pld)
                    return (0);
                pl_data *dn = new pl_data(*pld);
                dn->set_tab_next(0);
                return (dn);
            }
            pl_data *dn = new pl_data;
            dn->set_name(0);
            tgen_t<pl_data> gen(pcd->data_tab());
            pl_data *pld;
            while ((pld = gen.next()) != 0)
                dn->totalize(pld);
            return (dn);
        }
        if (!pldata)
            return (0);
        if (lname && *lname) {
            const char *ln = lname_tab.find(lname);
            if (!ln)
                return (0);
            pl_data *pld = pldata->find(ln);
            if (!pld)
                return (0);
            pl_data *dn = new pl_data(*pld);
            dn->set_tab_next(0);
            return (dn);
        }
        pl_data *dn = new pl_data;
        dn->set_name(0);

        tgen_t<pl_data> gen(pldata);
        pl_data *pld;
        while ((pld = gen.next()) != 0)
            dn->totalize(pld);
        return (dn);
    }
    return (0);
}


// Return a string tabulation of the top-level counts.
//
char *
cv_info::format_totals()
{
    sLstr lstr;
    char buf[256];

#ifdef WIN32
    const char *format = "%-16s %I64d\n";
#else
    const char *format = "%-16s %lld\n";
#endif

    sprintf(buf, format, "Records", (long long)total_records());
    lstr.add(buf);
    sprintf(buf, format, "Cells", (long long)total_cells());
    lstr.add(buf);
    sprintf(buf, format, "Labels", (long long)total_labels());
    lstr.add(buf);
    sprintf(buf, format, "Srefs", (long long)total_srefs());
    lstr.add(buf);
    sprintf(buf, format, "Arefs", (long long)total_arefs());
    lstr.add(buf);

    if (enable_per_layer) {
        lstr.add_c('\n');
        HEADER(buf);
        lstr.add(buf);

        stringlist *s0 = layers();
        for (stringlist *s = s0; s; s = s->next) {
            pl_data *pld = pldata->find(lname_tab.find(s->string));
            if (pld) {
                pld->print_counts(buf, s->string);
                lstr.add(buf);
            }
        }
        stringlist::destroy(s0);

        lstr.add("\nTotals:\n");
    }

    sprintf(buf, format, "Boxes", (long long)total_boxes());
    lstr.add(buf);
    sprintf(buf, format, "Polygons", (long long)total_polys());
    lstr.add(buf);
    sprintf(buf, format, "Wires", (long long)total_wires());

    lstr.add(buf);
    double tv = (double)(total_polys() + total_wires());
    sprintf(buf, "%-16s %.3f\n", "Avg Verts",
        tv == 0.0 ? 0.0 : total_vertices()/tv);
    lstr.add(buf);

    return (lstr.string_trim());
}


// Return a string tabulation of the data for p.
//
char *
cv_info::format_counts(const symref_t *p)
{
    if (!pcdata)
        return (0);

    sLstr lstr;
    char buf[256];
    pc_data *pcd = pcdata->find(Tstring(p->get_name()));
    if (pcd && pcd->data_tab()) {
        HEADER(buf);
        lstr.add(buf);

        stringlist *s0 = layers();
        for (stringlist *s = s0; s; s = s->next) {
            pl_data *pld = pcd->data_tab()->find(lname_tab.find(s->string));
            if (pld) {
                pld->print_counts(buf, s->string);
                lstr.add(buf);
            }
        }
        stringlist::destroy(s0);

        pcd->print_totals(buf);
        lstr.add(buf);
    }
    return (lstr.string_trim());
}


int64_t
cv_info::total_boxes()
{
    int64_t boxes = 0;
    tgen_t<pl_data> gen(pldata);
    pl_data *pl;
    while ((pl = gen.next()) != 0)
        boxes += pl->box_count();
    return (boxes);
}


int64_t
cv_info::total_polys()
{
    int64_t polys = 0;
    tgen_t<pl_data> gen(pldata);
    pl_data *pl;
    while ((pl = gen.next()) != 0)
        polys += pl->poly_count();
    return (polys);
}


int64_t
cv_info::total_wires()
{
    int64_t wires = 0;
    tgen_t<pl_data> gen(pldata);
    pl_data *pl;
    while ((pl = gen.next()) != 0)
        wires += pl->wire_count();
    return (wires);
}


int64_t
cv_info::total_vertices()
{
    int64_t verts = 0;
    tgen_t<pl_data> gen(pldata);
    pl_data *pl;
    while ((pl = gen.next()) != 0)
        verts += pl->vertex_count();
    return (verts);
}


void
cv_info::set_boxes(int64_t n)
{
    if (!pldata)
        pldata = new itable_t<pl_data>;
    pl_data *pld = pldata->find(cur_lname);
    if (!pld) {
        pld = new_pldata();
        pldata->link(pld);
        pldata = pldata->check_rehash();
    }
    pld->set_box_count(n);
}


void
cv_info::set_polys(int64_t n)
{
    if (!pldata)
        pldata = new itable_t<pl_data>;
    pl_data *pld = pldata->find(cur_lname);
    if (!pld) {
        pld = new_pldata();
        pldata->link(pld);
        pldata = pldata->check_rehash();
    }
    pld->set_poly_count(n);
}


void
cv_info::set_wires(int64_t n)
{
    if (!pldata)
        pldata = new itable_t<pl_data>;
    pl_data *pld = pldata->find(cur_lname);
    if (!pld) {
        pld = new_pldata();
        pldata->link(pld);
        pldata = pldata->check_rehash();
    }
    pld->set_wire_count(n);
}

void
cv_info::set_vertices(int64_t n)
{
    if (!pldata)
        pldata = new itable_t<pl_data>;
    pl_data *pld = pldata->find(cur_lname);
    if (!pld) {
        pld = new_pldata();
        pldata->link(pld);
        pldata = pldata->check_rehash();
    }
    pld->set_vertex_count(n);
}


// Allocate a new pc_data struct.
//
pc_data *
cv_info::new_pcdata(const symref_t *p)
{
    pc_data *pcd = pc_fct.new_element();
    pcd->set_name(p->get_name());
    return (pcd);
}


// Allocate a new pl_info struct.
//
pl_data *
cv_info::new_pldata()
{
    pl_data *pld = pl_fct.new_element();
    pld->set_name(cur_lname);
    return (pld);
}


// Return bytes currently in use by this struct.
//
unsigned int
cv_info::memuse()
{
    unsigned int m = sizeof(cv_info);
    m += pc_fct.memuse() + pl_fct.memuse() + lname_tab.memuse();
    if (pcdata) {
        tgen_t<pc_data> gen(pcdata);
        pc_data *pcd;
        while ((pcd = gen.next()) != 0) {
            itable_t<pl_data> *t = pcd->data_tab();
            if (t) {
                m += sizeof(itable_t<pl_data>) +
                    (t->hashwidth()-1)*sizeof(pl_data*);
            }
        }
        m += sizeof(itable_t<pc_data>) +
            (pcdata->hashwidth()-1)*sizeof(pc_data*);
    }
    if (pldata)
        m += sizeof(itable_t<pl_data>) +
            (pldata->hashwidth()-1)*sizeof(pl_data*);
    return (m);
}
// End of cv_info functions.


void
pl_data::print_counts(char *buf, const char *lstring) const
{
    int64_t boxes = box_count();
    int64_t polys = poly_count();
    int64_t wires = wire_count();
    int64_t verts = vertex_count();
    double va = 0.0;
    if (polys + wires)
        va = ((double)verts)/(polys + wires);
    sprintf(buf, count_format, lstring, (long long)boxes, (long long)polys,
        (long long)wires, va);
}
// End of pl_data functions.


void
pc_data::add_box(cv_info *info, int n)
{
    if (!item_tab)
        item_tab = new itable_t<pl_data>;
    pl_data *pld = item_tab->find(info->cur_layer());
    if (!pld) {
        pld = info->new_pldata();
        item_tab->link(pld);
        item_tab = item_tab->check_rehash();
    }
    pld->add_box(n);
}


void
pc_data::add_poly(cv_info *info, int nverts)
{
    if (!item_tab)
        item_tab = new itable_t<pl_data>;
    pl_data *pld = item_tab->find(info->cur_layer());
    if (!pld) {
        pld = info->new_pldata();
        item_tab->link(pld);
        item_tab = item_tab->check_rehash();
    }
    pld->add_poly(nverts);
}


void
pc_data::add_wire(cv_info *info, int nverts)
{
    if (!item_tab)
        item_tab = new itable_t<pl_data>;
    pl_data *pld = item_tab->find(info->cur_layer());
    if (!pld) {
        pld = info->new_pldata();
        item_tab->link(pld);
        item_tab = item_tab->check_rehash();
    }
    pld->add_wire(nverts);
}


int64_t
pc_data::total_boxes() const
{
    if (!item_tab)
        return (0);
    int64_t boxes = 0;
    tgen_t<pl_data> gen(item_tab);
    pl_data *pl;
    while ((pl = gen.next()) != 0)
        boxes += pl->box_count();
    return (boxes);
}


int64_t
pc_data::total_polys() const
{
    if (!item_tab)
        return (0);
    int64_t polys = 0;
    tgen_t<pl_data> gen(item_tab);
    pl_data *pl;
    while ((pl = gen.next()) != 0)
        polys += pl->box_count();
    return (polys);
}


int64_t
pc_data::total_wires() const
{
    if (!item_tab)
        return (0);
    int64_t wires = 0;
    tgen_t<pl_data> gen(item_tab);
    pl_data *pl;
    while ((pl = gen.next()) != 0)
        wires += pl->box_count();
    return (wires);
}


int64_t
pc_data::total_vertices() const
{
    if (!item_tab)
        return (0);
    int64_t verts = 0;
    tgen_t<pl_data> gen(item_tab);
    pl_data *pl;
    while ((pl = gen.next()) != 0)
        verts += pl->box_count();
    return (verts);
}


void
pc_data::print_totals(char *buf) const
{
    if (!item_tab) {
        // note: sprintf bug work-around
        sprintf(buf, count_format, "Totals:", (long long)0, (long long)0,
            (long long)0, 0.0);
        return;
    }
    int64_t boxes = 0;
    int64_t polys = 0;
    int64_t wires = 0;
    int64_t verts = 0;
    tgen_t<pl_data> gen(item_tab);
    pl_data *pl;
    while ((pl = gen.next()) != 0) {
        boxes += pl->box_count();
        polys += pl->poly_count();
        wires += pl->wire_count();
        verts += pl->vertex_count();
    }

    double va = 0.0;
    if (polys + wires)
        va = ((double)verts)/(polys + wires);
    sprintf(buf, count_format, "Totals:", (long long)boxes, (long long)polys,
        (long long)wires, va);
}
// End of pc_data functions.

