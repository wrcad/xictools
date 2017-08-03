
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

#ifndef FIO_CHD_FLAT_PRV_H
#define FIO_CHD_FLAT_PRV_H

#include "cd_sdb.h"

// The output processing struct for cCHD::readFlat.
//
struct rf_out : public cv_out
{
    rf_out(CDs *sd, const BBox *bb, cTfmStack *tstk, bool clip)
        {
            out_stk = tstk;
            rf_clip = bb ? clip : false;
            rf_noAOI = !bb;
            if (bb)
                rf_AOI = *bb;
            rf_backend = 0;
            rf_targcell = sd;
            rf_targld = 0;
            rf_targdt = 0;
        }

    bool set_destination(const char*) { return (true); }
    bool set_destination(FILE*, void**, void**) { return (true); }
    bool open_library(DisplayMode, double) { return (true); }
    bool queue_property(int, const char*) { return (true); }
    bool write_library(int, double, double, tm*, tm*, const char*)
        { return (true); }
    bool write_struct(const char*, tm*, tm*) { return (true); }
    bool write_end_struct(bool = false) { return (true); }

    bool queue_layer(const Layer *layer, bool*)
        {
            if (rf_backend) {
                if (!rf_backend->queue_layer(layer)) {
                    out_interrupted = rf_backend->aborted();
                    return (false);
                }
            }
            else {
                rf_targld = CDldb()->findLayer(layer->name, out_mode);
                if (!rf_targld)
                    rf_targld = CDldb()->newLayer(layer->name, out_mode);
                rf_targdt = layer->datatype;
            }
            return (true);
        }

    bool write_box(const BBox*);
    bool write_poly(const Poly*);
    bool write_wire(const Wire*);
    bool write_text(const Text*);
    bool write_sref(const Instance*) { return (true); }
    bool write_endlib(const char*) { return (true); }
    bool write_info(Attribute*, const char*) { return (true); }

    bool check_for_interrupt() { return (true); }
    bool flush_cache() { return (true); }
    bool write_header(const CDs*) { return (false); }
    bool write_object(const CDo*, cvLchk*);

    void set_backend(cv_backend *bk) { rf_backend = bk; }

    bool rf_clip;
    bool rf_noAOI;
    BBox rf_AOI;
    cv_backend *rf_backend;

    CDs *rf_targcell;
    CDl *rf_targld;
    int rf_targdt;
};

// Back end for cCHD::readFlat_odb.
//
struct cv_backend_odb : cv_backend
{
    cv_backend_odb()
        {
            table = 0;
            ldesc = 0;
            entry = 0;
        }

    bool queue_layer(const Layer *layer, bool*)
        {
            ldesc = CDldb()->findLayer(layer->name, Physical);
            if (!ldesc)
                ldesc = CDldb()->newLayer(layer->name, Physical);

            if (!table)
                table = new SymTab(false, false);
            odb_t *db = (odb_t*)SymTab::get(table, (unsigned long)ldesc);
            if (db == (odb_t*)ST_NIL) {
                db = new odb_t;
                table->add((unsigned long)ldesc, db, false);
            }
            entry = db;
            return (true);
        }

    bool write_box(const BBox *BB)
        {
            if (entry && ldesc) {
                CDo *newo = new CDo(ldesc, BB);
                entry->add(newo);
            }
            return (true);
        }

    bool write_poly(const Poly *po)
        {
            if (entry && ldesc) {
                Poly poly(*po);
                poly.points = new Point[po->numpts];
                memcpy(poly.points, po->points, po->numpts*sizeof(Point));
                CDpo *newo = new CDpo(ldesc, &poly);
                entry->add(newo);
            }
            return (true);
        }

    bool write_wire(const Wire *w)
        {
            if (entry && ldesc) {
                Wire wire(*w);
                wire.points = new Point[w->numpts];
                memcpy(wire.points, w->points, w->numpts*sizeof(Point));
                CDw *newo = new CDw(ldesc, &wire);
                entry->add(newo);
            }
            return (true);
        }

    bool write_text(const Text*) { return (true); }

    void print_report() { }

    SymTab *table;      // output table
private:
    CDl *ldesc;         // target layer desc
    odb_t *entry;       // output object list for current layer
};

// Back end for cCHD::readFlat_zdb.
//
struct cv_backend_zdb : cv_backend
{
    cv_backend_zdb()
        {
            table = 0;
            ldesc = 0;
            entry = 0;
            set_wire_to_poly(true);
        }

    bool queue_layer(const Layer *layer, bool*)
        {
            ldesc = CDldb()->findLayer(layer->name, Physical);
            if (!ldesc)
                ldesc = CDldb()->newLayer(layer->name, Physical);

            if (!table)
                table = new SymTab(false, false);
            zdb_t *db = (zdb_t*)SymTab::get(table, (unsigned long)ldesc);
            if (db == (zdb_t*)ST_NIL) {
                db = new zdb_t;
                table->add((unsigned long)ldesc, db, false);
            }
            entry = db;
            return (true);
        }

    bool write_box(const BBox *BB)
        {
            if (entry && ldesc &&
                    BB->right > BB->left && BB->top > BB->bottom) {
                Zoid z(BB);
                entry->add(&z);
            }
            return (true);
        }

    bool write_poly(const Poly *po)
        {
            if (entry && ldesc) {
                Zlist *z0 = po->toZlist();
                for (Zlist *z = z0; z; z = z->next)
                    entry->add(&z->Z);
                Zlist::destroy(z0);
            }
            return (true);
        }

    bool write_wire(const Wire *w)
        {
            if (entry && ldesc) {
                Poly po;
                w->toPoly(&po.points, &po.numpts);
                Zlist *z0 = po.toZlist();
                delete [] po.points;
                for (Zlist *z = z0; z; z = z->next)
                    entry->add(&z->Z);
                Zlist::destroy(z0);
            }
            return (true);
        }

    bool write_text(const Text*) { return (true); }

    void print_report() { }

    SymTab *table;      // output table
private:
    CDl *ldesc;         // target layer desc
    zdb_t *entry;       // output object list for current layer
};

// Back end for cCHD::readFlat_zl.
//
struct cv_backend_zl : cv_backend
{
    cv_backend_zl()
        {
            table = 0;
            ldesc = 0;
            entry = 0;
            set_wire_to_poly(true);
        }

    bool queue_layer(const Layer *layer, bool*)
        {
            ldesc = CDldb()->findLayer(layer->name, Physical);
            if (!ldesc)
                ldesc = CDldb()->newLayer(layer->name, Physical);
            if (!table)
                table = new SymTab(false, false);
            SymTabEnt *h = SymTab::get_ent(table, (unsigned long)ldesc);
            if (!h) {
                table->add((unsigned long)ldesc, 0, false);
                h = SymTab::get_ent(table, (unsigned long)ldesc);
            }
            entry = h;
            return (true);
        }

    bool write_box(const BBox *BB)
        {
            if (entry && ldesc &&
                    BB->right > BB->left && BB->top > BB->bottom)
                entry->stData = new Zlist(BB, (Zlist*)entry->stData);
            return (true);
        }

    bool write_poly(const Poly *po)
        {
            if (entry && ldesc) {
                Zlist *zl = po->toZlist();
                if (zl) {
                    Zlist *zn = zl;
                    while (zn->next)
                        zn = zn->next;
                    zn->next = (Zlist*)entry->stData;
                    entry->stData = zl;
                }
            }
            return (true);
        }

    bool write_wire(const Wire *w)
        {
            if (entry && ldesc) {
                Zlist *zl = w->toZlist();
                if (zl) {
                    Zlist *zn = zl;
                    while (zn->next)
                        zn = zn->next;
                    zn->next = (Zlist*)entry->stData;;
                    entry->stData = zl;
                }
            }
            return (true);
        }

    bool write_text(const Text*) { return (true); }

    void print_report() { }

    SymTab *table;          // symbol tab, keyed by layer desc
private:
    CDl *ldesc;             // current layer
    SymTabEnt *entry;       // current tab entry
};

// Back end for cCHD::readFlat_zbdb.
//
struct cv_backend_zbdb : cv_backend
{
    cv_backend_zbdb(SymTab *tab, int x, int y,
            unsigned int nx, unsigned int ny,
            unsigned int dx, unsigned int dy,
            unsigned int bx, unsigned int by)
        {
            table = tab;
            ldesc = 0;
            entry = 0;

            b_x = x;
            b_y = y;
            b_nx = nx;
            b_ny = ny;
            b_dx = dx;
            b_dy = dy;
            b_bx = bx;
            b_by = by;
            set_wire_to_poly(true);
        }

    bool queue_layer(const Layer *layer, bool*)
        {
            ldesc = CDldb()->findLayer(layer->name, Physical);
            if (!ldesc)
                ldesc = CDldb()->newLayer(layer->name, Physical);

            if (!table)
                table = new SymTab(false, false);
            zbins_t *db = (zbins_t*)SymTab::get(table, (unsigned long)ldesc);
            if (db == (zbins_t*)ST_NIL) {
                db = new zbins_t(b_x, b_y, b_nx, b_ny, b_dx, b_dy, b_bx, b_by);
                table->add((unsigned long)ldesc, db, false);
            }
            entry = db;
            return (true);
        }

    bool write_box(const BBox *BB)
        {
            if (entry && ldesc &&
                    BB->right > BB->left && BB->top > BB->bottom) {
                Zoid z(BB);
                entry->add(&z);
            }
            return (true);
        }

    bool write_poly(const Poly *po)
        {
            if (entry && ldesc)
                entry->add(po->toZlist());
            return (true);
        }

    bool write_wire(const Wire *w)
        {
            if (entry && ldesc) {
                Poly po;
                w->toPoly(&po.points, &po.numpts);
                entry->add(po.toZlist());
                delete [] po.points;
            }
            return (true);
        }

    bool write_text(const Text*) { return (true); }

    void print_report() { }

    SymTab *output_table() { return (table); }

private:
    SymTab *table;      // output table
    CDl *ldesc;         // target layer desc
    zbins_t *entry;     // output object list for current layer

    int b_x, b_y;
    unsigned int b_nx, b_ny;
    unsigned int b_dx, b_dy;
    unsigned int b_bx, b_by;
};

#endif

