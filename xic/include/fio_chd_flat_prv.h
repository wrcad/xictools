
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
 $Id: fio_chd_flat_prv.h,v 5.9 2014/12/20 06:20:22 stevew Exp $
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
            rf_clip = bb ? clip : false;
            rf_noAOI = !bb;
            if (bb)
                rf_AOI = *bb;
            rf_backend = 0;
            rf_stack = tstk;
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

    cTfmStack *rf_stack;
    CDs *rf_targcell;
    CDl *rf_targld;
    int rf_targdt;
};

// Back end for cCHD::readFlat_odb.
//
struct cv_backend_odb : cv_backend
{
    cv_backend_odb() : cv_backend(false)
        {
            table = 0;
            ldesc = 0;
            entry = 0;
        }

    bool queue_layer(const Layer *layer)
        {
            ldesc = CDldb()->findLayer(layer->name, Physical);
            if (!ldesc)
                ldesc = CDldb()->newLayer(layer->name, Physical);

            if (!table)
                table = new SymTab(false, false);
            odb_t *db = (odb_t*)table->get((unsigned long)ldesc);
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
    cv_backend_zdb() : cv_backend(true)
        {
            table = 0;
            ldesc = 0;
            entry = 0;
        }

    bool queue_layer(const Layer *layer)
        {
            ldesc = CDldb()->findLayer(layer->name, Physical);
            if (!ldesc)
                ldesc = CDldb()->newLayer(layer->name, Physical);

            if (!table)
                table = new SymTab(false, false);
            zdb_t *db = (zdb_t*)table->get((unsigned long)ldesc);
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
                z0->free();
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
                z0->free();
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
    cv_backend_zl() : cv_backend(true)
        {
            table = 0;
            ldesc = 0;
            entry = 0;
        }

    bool queue_layer(const Layer *layer)
        {
            ldesc = CDldb()->findLayer(layer->name, Physical);
            if (!ldesc)
                ldesc = CDldb()->newLayer(layer->name, Physical);
            if (!table)
                table = new SymTab(false, false);
            SymTabEnt *h = table->get_ent((unsigned long)ldesc);
            if (!h) {
                table->add((unsigned long)ldesc, 0, false);
                h = table->get_ent((unsigned long)ldesc);
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
            unsigned int bx, unsigned int by) : cv_backend(true)
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
        }

    bool queue_layer(const Layer *layer)
        {
            ldesc = CDldb()->findLayer(layer->name, Physical);
            if (!ldesc)
                ldesc = CDldb()->newLayer(layer->name, Physical);

            if (!table)
                table = new SymTab(false, false);
            zbins_t *db = (zbins_t*)table->get((unsigned long)ldesc);
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

