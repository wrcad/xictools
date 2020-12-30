
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


#ifndef FIO_INFO_H
#define FIO_INFO_H

struct cv_info;
struct symref_t;

//-----------------------------------------------------------------------------
// Per-layer data counts.
//
struct pl_data
{
    // NOTE:  Since this object is bulk-allocated in cv_info, there is
    // no constructor/destructor.  There is nothing to free in a
    // destructor, so it is safe to use outside of the cv_info
    // framework, but be advised that it will not be initialized when
    // created.

    // itable_t necessities
    uintptr_t tab_key()                 { return ((uintptr_t)lname); }
    pl_data *tab_next()                 { return (next); }
    void set_tab_next(pl_data *n)       { next = n; }
    pl_data *tgen_next(bool)            { return (next); }

    // initializer
    void set_name(const char *n)
        {
            lname = n;
            next = 0;
            box_cnt = 0;
            poly_cnt = 0;
            wire_cnt = 0;
            vtex_cnt = 0;
        }

    void totalize(pl_data *pld)
        {
            if (pld) {
                box_cnt += pld->box_cnt;
                poly_cnt += pld->poly_cnt;
                wire_cnt += pld->wire_cnt;
                vtex_cnt += pld->vtex_cnt;
            }
        }

    // increment counts, called during reading
    void add_box(int n)                 { box_cnt += n; }
    void add_poly(int nverts)           { poly_cnt++; vtex_cnt += nverts; }
    void add_wire(int nverts)           { wire_cnt++; vtex_cnt += nverts; }

    // data access
    int64_t box_count()           const { return (box_cnt); }
    int64_t poly_count()          const { return (poly_cnt); }
    int64_t wire_count()          const { return (wire_cnt); }
    int64_t vertex_count()        const { return (vtex_cnt); }
    const char *layer_name()      const { return (lname); }
    void print_counts(char*, const char*) const;

    // set data
    void set_box_count(int64_t n)       { box_cnt = n; }
    void set_poly_count(int64_t n)      { poly_cnt = n; }
    void set_wire_count(int64_t n)      { wire_cnt = n; }
    void set_vertex_count(int64_t n)    { vtex_cnt = n; }

private:
    const char *lname;                  // layer name, string tab pointer
    pl_data *next;                      // table link

    int64_t box_cnt;                    // number of boxes read
    int64_t poly_cnt;                   // number of polygons read
    int64_t wire_cnt;                   // number of wires read
    int64_t vtex_cnt;                   // number of poly/wire vertices read
};


//-----------------------------------------------------------------------------
// Per cell data counts.
//
struct pc_data
{
    // NOTE:  Since this object is bulk-allocated in cv_info, there is
    // no constructor/destructor.  The item_tab must be destroyed when
    // the object is destroyed.  This cleanup is done in the cv_info
    // destructor.  Use outside of the cv_info framework will require
    // explicit deletion of this field through subclassing or
    // otherwise.

    uintptr_t tab_key()                 { return ((uintptr_t)cname); }
    pc_data *tab_next()                 { return (next); }
    void set_tab_next(pc_data *n)       { next = n; }
    pc_data *tgen_next(bool)            { return (next); }

    // initializer
    void set_name(CDcellName n)
        {
            cname = n;
            next = 0;
            item_tab = 0;
        }

    // increment counts, called when reading
    void add_box(cv_info*, int);
    void add_poly(cv_info*, int);
    void add_wire(cv_info*, int);

    // data access, totals
    int64_t total_boxes() const;
    int64_t total_polys() const;
    int64_t total_wires() const;
    int64_t total_vertices() const;
    void print_totals(char*) const;

    inline pl_data *get_pldata(const char*, cv_info*);

    const char *cellname()              { return ((const char*)cname); }

    // data access, per-layer
    itable_t<pl_data> *data_tab()       { return (item_tab); }

private:
    CDcellName cname;                   // cell name, string tab pointer
    pc_data *next;                      // table link
    itable_t<pl_data> *item_tab;        // per-layer data
};


//-----------------------------------------------------------------------------

// String-table resolved layer name list.
struct info_lnames_t
{
    info_lnames_t(stringlist *s) { il_names = s; }
    ~info_lnames_t()
        {
            while (il_names) {
                stringlist *nx = il_names;
                il_names = il_names->next;
                nx->string = 0;
                delete nx;
            }
        }

    stringlist *names() { return (il_names); }

private:
    stringlist *il_names;
};

// Base struct for file info recording.
//
struct cv_info
{
    cv_info(bool per_layer, bool per_cell)
        {
            pcdata = 0;
            pldata = 0;

            rec_cnt = 0;
            cell_cnt = 0;
            text_cnt = 0;
            sref_cnt = 0;
            aref_cnt = 0;

            cur_lname = 0;
            cur_pcdata = 0;

            enable_per_layer = per_layer;
            enable_per_cell = per_cell;
        }

    virtual ~cv_info();

    virtual void initialize();
    virtual char *pr_records(FILE*)     { return (0); }
    virtual void add_record(int)        { rec_cnt++; }

    // empty cell filtering
    info_lnames_t *get_info_lnames(stringlist*);
    bool has_geom(symref_t*, info_lnames_t*, bool);

    // increment counts, called when reading
    void add_cell(const symref_t*);
    void add_layer(const char*);
    void add_box(int);
    void add_poly(int);
    void add_wire(int);
    void add_text()             { text_cnt++; }
    void add_inst(bool ary)     { if (ary) aref_cnt++; else sref_cnt++; }

    // data access methods

    // Return the save level enumeration.
    static cvINFO savemode(const cv_info *cvi)
        {
            if (!cvi)
                return (cvINFOnone);
            return (cvi->savemode_prv());
        }
private:
    cvINFO savemode_prv() const
        {
            if (enable_per_layer) {
                if (enable_per_cell)
                    return (cvINFOplpc);
                else
                    return (cvINFOpl);
            }
            else {
                if (enable_per_cell)
                    return (cvINFOpc);
                else
                    return (cvINFOtotals);
            }
        }
public:
    stringlist *layers(const char* = 0);
    stringlist *cells();
    pl_data *info(const char*, const char*);
    char *format_totals();
    char *format_counts(const symref_t*);
    int64_t total_records()     { return (rec_cnt); }
    int64_t total_cells()       { return (cell_cnt); }
    int64_t total_boxes();
    int64_t total_polys();
    int64_t total_wires();
    int64_t total_vertices();
    int64_t total_labels()      { return (text_cnt); }
    int64_t total_srefs()       { return (sref_cnt); }
    int64_t total_arefs()       { return (aref_cnt); }

    // data access, per-cell and per-layer
    itable_t<pc_data> *pcdata_tab() { return (pcdata); }
    itable_t<pl_data> *pldata_tab() { return (pldata); }

    // set data
    void set_records(int64_t n) { rec_cnt = n; }
    void set_cells(int64_t n)   { cell_cnt = n; }
    void set_boxes(int64_t);
    void set_polys(int64_t);
    void set_wires(int64_t);
    void set_vertices(int64_t);
    void set_labels(int64_t n)  { text_cnt = n; }
    void set_srefs(int64_t n)   { sref_cnt = n; }
    void set_arefs(int64_t n)   { aref_cnt = n; }

    // local allocators for table elements
    pc_data *new_pcdata(const symref_t*);
    pl_data *new_pldata();

    unsigned int memuse();

    const char *cur_layer()     { return (cur_lname); }

    const char *tabname(const char *lname)
        {
            return (lname ? lname_tab.find(lname) : 0);
        }

protected:
    itable_t<pc_data> *pcdata;  // per-cell data table
    itable_t<pl_data> *pldata;  // per-layer data table

    int64_t    rec_cnt;         // number of records read
    int64_t    cell_cnt;        // number of cells read
    int64_t    text_cnt;        // number of labels read
    int64_t    sref_cnt;        // number of structure references read
    int64_t    aref_cnt;        // number of array references read

    const char *cur_lname;      // current layer name, from lname_tab
    pc_data    *cur_pcdata;     // current cell pointer

    bool enable_per_layer;      // enable recording per-layer stats
    bool enable_per_cell;       // enable recording per-cell stats

    strtab_t   lname_tab;       // layer names found
    eltab_t<pc_data> pc_fct;    // pc_data factory
    eltab_t<pl_data> pl_fct;    // pl_data factory
};


//-----------------------------------------------------------------------------
// Deferred inline.

// This only works if cvINFOplpc.
//
pl_data *
pc_data::get_pldata(const char *layername, cv_info *info)
{
    if (!item_tab || !layername || !info)
        return (0);
    const char *ln = info->tabname(layername);
    if (!ln)
        return (0);
    return (item_tab->find(ln));
}

#endif

