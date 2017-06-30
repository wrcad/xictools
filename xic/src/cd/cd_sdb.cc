
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
 $Id: cd_sdb.cc,v 5.36 2015/10/18 01:50:55 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "cd_sdb.h"
#include "geo_ylist.h"


// Print the bin dimensions before calling merge.
//#define ZBINS_DBG

cCDsdbDB *cCDsdbDB::instancePtr = 0;

cCDsdbDB::cCDsdbDB()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cCDsdbDB already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    sdb_table = 0;
}


// Private static error exit.
//
void
cCDsdbDB::on_null_ptr()
{
    fprintf(stderr, "Singleton class cCDsdbDB used before instantiated.\n");
    exit(1);
}


// Save db under the given name.  If the name is already in use, and
// has no owner, the existing data are destroyed and replaced by the
// given dbtab.  If existing data have an owner, no replacement is
// done and false is returned.
//
bool
cCDsdbDB::saveDB(cSDB *db)
{
    if (!db)
        return (true);
    if (!sdb_table)
        sdb_table = new SymTab(false, false);
    SymTabEnt *h = sdb_table->get_ent(db->name());
    if (h) {
        if (((cSDB*)h->stData)->owner())
            return (false);
        delete (cSDB*)h->stData;
        h->stTag = db->name();
        h->stData = db;
        return (true);
    }
    sdb_table->add(db->name(), db, false);
    return (true);
}


// Return the database element saved under name, or nil if not found.
//
cSDB *
cCDsdbDB::findDB(const char *name)
{
    if (!sdb_table || !name)
        return (0);
    cSDB *db = (cSDB*)sdb_table->get(name);
    if (db != (cSDB*)ST_NIL)
        return (db);
    return (0);
}


// Return a list of the database names and types, reasonable formatted
// for printing.
//
stringlist *
cCDsdbDB::listDB()
{
    if (!sdb_table)
        return (0);
    char buf[256];
    stringlist *s0 = 0;
    SymTabGen gen(sdb_table);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        cSDB *db = (cSDB*)h->stData;
        if (!db)
            continue;
        const char *dbt;
        if (db->type() == sdbBdb)
            dbt = "(Bdb)";
        else if (db->type() == sdbOdb)
            dbt = "(Odb)";
        else if (db->type() == sdbZdb)
            dbt = "(Zdb)";
        else if (db->type() == sdbZldb)
            dbt = "(Zldb)";
        else if (db->type() == sdbZbdb)
            dbt = "(Zbdb)";
        else
            continue;
        snprintf(buf, 256, "%-20s %s", db->name(), dbt);
        s0 = new stringlist(lstring::copy(buf), s0);
    }
    return (s0);
}


// Destroy the named element.  If a null pointer is passed, the entire
// database is cleared.  Only unowned databases will be deleted.
//
void
cCDsdbDB::destroyDB(const char *name)
{
    if (!sdb_table)
        return;
    if (name) {
        cSDB *db = (cSDB*)sdb_table->get(name);
        if (db != (cSDB*)ST_NIL) {
            if (!db->owner()) {
                sdb_table->remove(name);
                delete db;
            }
        }
    }
    else {
        SymTabGen gen(sdb_table, true);
        SymTabEnt *h, *h0 = 0;
        while ((h = gen.next()) != 0) {
            if (((cSDB*)h->stData)->owner()) {
                h->stNext = h0;
                h0 = h;
                continue;
            }
            delete (cSDB*)h->stData;
            delete h;
        }
        delete sdb_table;
        sdb_table = 0;

        if (h0) {
            // Rebuild the table, put these back.
            sdb_table = new SymTab(false, false);
            SymTabEnt *hn;
            for (h = h0; h; h = hn) {
                hn = h->stNext;
                cSDB *db = (cSDB*)h->stData;
                sdb_table->add(db->name(), db, false);
                delete h;
            }
        }
    }
}
// End of cCDsdbDB functions.


// The "special" database: cSDB
// This is a polymorphic symbol table, stored by name in cCDsdbDB. 
// The element can be odb_t, zdb_t, etc., according to the type field. 
// All have in common that the elements are referenced by the layer
// descriptor pointers, and represent geometry.

cSDB::cSDB(const char *n, SymTab *d, sdbType t)
{
    dbname = lstring::copy(n);
    db = d;
    dbtype = t;
    dbowner = 0;
    if (db) {
        if (dbtype == sdbBdb || dbtype == sdbOdb || dbtype == sdbZdb) {
            SymTabGen gen(db);
            SymTabEnt *h;
            bool first = true;
            while ((h = gen.next()) != 0) {
                base_db_t *bdb = (base_db_t*)h->stData;
                if (first) {
                    dbBB = bdb->db_BB;
                    first = false;
                }
                else
                    dbBB.add(&bdb->db_BB);
            }
        }
        else if (dbtype == sdbZbdb) {
            SymTabGen gen(db);
            SymTabEnt *h;
            bool first = true;
            while ((h = gen.next()) != 0) {
                zbins_t *bdb = (zbins_t*)h->stData;
                if (first) {
                    dbBB = *bdb->BB();
                    first = false;
                }
                else
                    dbBB.add(bdb->BB());
            }
        }
    }
}


cSDB::~cSDB()
{
    delete [] dbname;
    SymTabGen gen(db, true);
    if (dbtype == sdbBdb) {
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            delete (bdb_t*)h->stData;
            delete h;
        }
    }
    else if (dbtype == sdbOdb) {
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            delete (odb_t*)h->stData;
            delete h;
        }
    }
    else if (dbtype == sdbZdb) {
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            delete (zdb_t*)h->stData;
            delete h;
        }
    }
    else if (dbtype == sdbZldb) {
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            Zlist::destroy(((Zlist*)h->stData));
            delete h;
        }
    }
    else if (dbtype == sdbZbdb) {
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            delete (zbins_t*)h->stData;
            delete h;
        }
    }
    delete db;
}


// Return a sorted list of the layer names used in the database.
//
stringlist *
cSDB::layers()
{
    stringlist *s0 = 0;
    SymTabGen gen(db);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        CDl *ld = (CDl*)h->stTag;
        s0 = new stringlist(lstring::copy(ld->name()), s0);
    }
    s0->sort(0);
    return (s0);
}
// End of cSDB functions.


//----------------------------------------------------------------------------
// Struct bdb_t
// This is a list of boxes, with provision for spatial sorting.
//----------------------------------------------------------------------------

bdb_t::~bdb_t()
{
    // Objects are locally block-allocated and automatically freed.
    delete [] bdb_objects;
}


// Add an object to the list.
//
void
bdb_t::add(const BBox *o)
{
    if (!bdb_objects) {
        db_list_size = 16;
        bdb_objects = new BBox*[db_list_size];
    }
    else if (db_num_objects >= db_list_size) {
        BBox **t = new BBox*[db_list_size + db_list_size];
        memcpy(t, bdb_objects, db_list_size*sizeof(BBox*));
        delete [] bdb_objects;
        bdb_objects = t;
        db_list_size += db_list_size;
    }
    if (!db_num_objects)
        db_BB = *o;
    else
        db_BB.add(o);
    BBox *newbox = allocator.new_obj();
    *newbox = *o;
    bdb_objects[db_num_objects++] = newbox;
}


// Return a list of the objects from objects clipped to bref.
//
Blist *
bdb_t::getBlist(Blist *bref, XIrt *retp)
{
    Blist *b0 = 0;
    *retp = XIok;
    for (Blist *bl = bref; bl; bl = bl->next) {
        unsigned int n = find_objects(&bl->BB);
        for (unsigned int i = 0; i < n; i++) {
            Blist *bx = new Blist(bdb_objects[i], 0);
            if (!(bx->BB < bl->BB))
                bx = Blist::clip_to(bx, &bl->BB);
            if (bx) {
                Blist *bn = bx;
                while (bn->next)
                    bn = bn->next;
                bn->next = b0;
                b0 = bx;
            }
        }
    }
    try {
        b0 = Blist::merge(b0);
        return (b0);
    }
    catch (XIrt ret) {
        *retp = ret;
        return (0);
    }
}


// Return a list of the objects from objects clipped to zref.
//
Zlist *
bdb_t::getZlist(const Zlist *zref, XIrt *retp)
{
    Zlist *z0 = 0;
    *retp = XIok;
    for (const Zlist *zl = zref; zl; zl = zl->next) {
        BBox BB;
        zl->Z.BB(&BB);
        bool manh = zl->Z.is_rect();

        unsigned int n = find_objects(&BB);
        for (unsigned int i = 0; i < n; i++) {
            Zlist *zx = new Zlist(bdb_objects[i], 0);
            BBox tBB;
            zx->Z.BB(&tBB);
            if (!manh || !(tBB <= BB))
                Zlist::zl_and(&zx, &zl->Z);
            if (zx) {
                Zlist *zn = zx;
                while (zn->next)
                    zn = zn->next;
                zn->next = z0;
                z0 = zx;
            }
        }
    }
    try {
        z0 = Zlist::repartition(z0);
        return (z0);
    }
    catch (XIrt ret) {
        *retp = ret;
        return (0);
    }
}


// Move the objects that overlap BB to the front of the list, and
// return the number of such elements.
//
unsigned int
bdb_t::find_objects(const BBox *BB)
{
    if (BB->left == db_xmin && BB->right == db_xmax && db_nx_valid) {
        if (db_nx_valid == 1)
            return (setup_row(db_nx, BB->bottom, BB->top));
        db_scan_cols = true;
        db_nx_valid = 0;
        db_ny_valid = 0;
    }
    else if (BB->bottom == db_ymin && BB->top == db_ymax && db_ny_valid) {
        if (db_ny_valid == 1)
            return (setup_col(db_ny, BB->left, BB->right));
        db_scan_cols = false;
        db_nx_valid = 0;
        db_ny_valid = 0;
    }
    if (db_scan_cols) {
        unsigned int num = setup_col(db_num_objects, BB->left, BB->right);
        return (setup_row(num, BB->bottom, BB->top));
    }
    else {
        unsigned int num = setup_row(db_num_objects, BB->bottom, BB->top);
        return (setup_col(num, BB->left, BB->right));
    }
}


unsigned int
bdb_t::setup_row(unsigned int num, int ymin, int ymax)
{
    if (db_ny_valid) {
        if (ymin == db_ymin && ymax == db_ymax)
            return (db_ny);
        if (ymin > db_ymin) {
            if (!db_y_dsc)
                num = db_ny0;
            else
                db_y_dsc = false;
        }
        else if (ymax < db_ymax) {
            if (db_y_dsc)
                num = db_ny0;
            else
                db_y_dsc = true;
        }
    }
    if (db_y_dsc) {
        db_ny0 = order_bl(num, ymax);
        db_ny = order_tg(db_ny0, ymin);
    }
    else {
        db_ny0 = order_tg(num, ymin);
        db_ny = order_bl(db_ny0, ymax);
    }
    db_ymin = ymin;
    db_ymax = ymax;
    db_ny_valid = db_nx_valid + 1;
    return (db_ny);
}


unsigned int
bdb_t::setup_col(unsigned int num, int xmin, int xmax)
{

    if (db_nx_valid) {
        if (xmin == db_xmin && xmax == db_xmax)
            return (db_nx);
        if (xmin > db_xmin) {
            if (!db_x_dsc)
                num = db_nx0;
            else
                db_x_dsc = false;
        }
        else if (xmax < db_xmax) {
            if (db_x_dsc)
                num = db_nx0;
            else
                db_x_dsc = true;
        }
    }
    if (db_x_dsc) {
        db_nx0 = order_ll(num, xmax);
        db_nx = order_rg(db_nx0, xmin);
    }
    else {
        db_nx0 = order_rg(num, xmin);
        db_nx = order_ll(db_nx0, xmax);
    }
    db_xmin = xmin;
    db_xmax = xmax;
    db_nx_valid = db_ny_valid + 1;
    return (db_nx);
}


//----------------------------------------------------------------------------
// Struct odb_t
// This is a list of object descriptors, with provision for spatial
// sorting.
//----------------------------------------------------------------------------

odb_t::~odb_t()
{
    // The objects are deleted, so they must not be referenced elsewhere.
    if (odb_objects) {
        for (unsigned int i = 0; i < db_num_objects; i++)
            delete odb_objects[i];
        delete [] odb_objects;
    }
}


// Add an object to the list.
//
void
odb_t::add(CDo *o)
{
    if (!odb_objects) {
        db_list_size = 16;
        odb_objects = new CDo*[db_list_size];
    }
    else if (db_num_objects >= db_list_size) {
        CDo **t = new CDo*[db_list_size + db_list_size];
        memcpy(t, odb_objects, db_list_size*sizeof(CDo*));
        delete [] odb_objects;
        odb_objects = t;
        db_list_size += db_list_size;
    }
    if (!db_num_objects)
        db_BB = o->oBB();
    else
        db_BB.add(&o->oBB());
    odb_objects[db_num_objects++] = o;
}


// Return a list of the zoids from objects clipped to zref.
//
Zlist *
odb_t::getZlist(const Zlist *zref, XIrt *retp)
{
    Zlist *z0 = 0;
    *retp = XIok;
    for (const Zlist *zl = zref; zl; zl = zl->next) {
        BBox BB;
        zl->Z.BB(&BB);
        bool manh = zl->Z.is_rect();

        unsigned int n = find_objects(&BB);
        for (unsigned int i = 0; i < n; i++) {
            CDo *odesc = odb_objects[i];
            Zlist *zx = odesc->toZlist();
            if (!manh || !(odesc->oBB() <= BB))
                Zlist::zl_and(&zx, &zl->Z);
            if (zx) {
                Zlist *zn = zx;
                while (zn->next)
                    zn = zn->next;
                zn->next = z0;
                z0 = zx;
            }
        }
    }
    try {
        z0 = Zlist::repartition(z0);
        return (z0);
    }
    catch (XIrt ret) {
        *retp = ret;
        return (0);
    }
}


// Move the objects that overlap BB to the front of the list, and
// return the number of such elements.
//
unsigned int
odb_t::find_objects(const BBox *BB)
{
    if (BB->left == db_xmin && BB->right == db_xmax && db_nx_valid) {
        if (db_nx_valid == 1)
            return (setup_row(db_nx, BB->bottom, BB->top));
        db_scan_cols = true;
        db_nx_valid = 0;
        db_ny_valid = 0;
    }
    else if (BB->bottom == db_ymin && BB->top == db_ymax && db_ny_valid) {
        if (db_ny_valid == 1)
            return (setup_col(db_ny, BB->left, BB->right));
        db_scan_cols = false;
        db_nx_valid = 0;
        db_ny_valid = 0;
    }
    if (db_scan_cols) {
        unsigned int num = setup_col(db_num_objects, BB->left, BB->right);
        return (setup_row(num, BB->bottom, BB->top));
    }
    else {
        unsigned int num = setup_row(db_num_objects, BB->bottom, BB->top);
        return (setup_col(num, BB->left, BB->right));
    }
}


unsigned int
odb_t::setup_row(unsigned int num, int ymin, int ymax)
{
    if (db_ny_valid) {
        if (ymin == db_ymin && ymax == db_ymax)
            return (db_ny);
        if (ymin > db_ymin) {
            if (!db_y_dsc)
                num = db_ny0;
            else
                db_y_dsc = false;
        }
        else if (ymax < db_ymax) {
            if (db_y_dsc)
                num = db_ny0;
            else
                db_y_dsc = true;
        }
    }
    if (db_y_dsc) {
        db_ny0 = order_bl(num, ymax);
        db_ny = order_tg(db_ny0, ymin);
    }
    else {
        db_ny0 = order_tg(num, ymin);
        db_ny = order_bl(db_ny0, ymax);
    }
    db_ymin = ymin;
    db_ymax = ymax;
    db_ny_valid = db_nx_valid + 1;
    return (db_ny);
}


unsigned int
odb_t::setup_col(unsigned int num, int xmin, int xmax)
{

    if (db_nx_valid) {
        if (xmin == db_xmin && xmax == db_xmax)
            return (db_nx);
        if (xmin > db_xmin) {
            if (!db_x_dsc)
                num = db_nx0;
            else
                db_x_dsc = false;
        }
        else if (xmax < db_xmax) {
            if (db_x_dsc)
                num = db_nx0;
            else
                db_x_dsc = true;
        }
    }
    if (db_x_dsc) {
        db_nx0 = order_ll(num, xmax);
        db_nx = order_rg(db_nx0, xmin);
    }
    else {
        db_nx0 = order_rg(num, xmin);
        db_nx = order_ll(db_nx0, xmax);
    }
    db_xmin = xmin;
    db_xmax = xmax;
    db_nx_valid = db_ny_valid + 1;
    return (db_nx);
}


//----------------------------------------------------------------------------
// Struct zdb_t
// This is a list of trapezoids, with provision for spatial sorting.
//----------------------------------------------------------------------------

zdb_t::~zdb_t()
{
    // Objects are locally block-allocated and automatically freed.
    delete [] zdb_objects;
}


// Add an object to the list.
//
void
zdb_t::add(Zoid *z)
{
    if (!zdb_objects) {
        db_list_size = 16;
        zdb_objects = new Zoid*[db_list_size];
    }
    else if (db_num_objects >= db_list_size) {
        Zoid **t = new Zoid*[db_list_size + db_list_size];
        memcpy(t, zdb_objects, db_list_size*sizeof(Zoid*));
        delete [] zdb_objects;
        zdb_objects = t;
        db_list_size += db_list_size;
    }
    BBox BB;
    z->BB(&BB);
    if (!db_num_objects)
        db_BB = BB;
    else
        db_BB.add(&BB);
    Zoid *newzoid = allocator.new_obj();
    *newzoid = *z;
    zdb_objects[db_num_objects++] = newzoid;
}


// Return a list of the zoids from objects clipped to zref.
//
Zlist *
zdb_t::getZlist(const Zlist *zref, XIrt *retp)
{
    Zlist *z0 = 0;
    if (!zref) {
        for (unsigned int i = 0; i < db_num_objects; i++)
            z0 = new Zlist(zdb_objects[i], z0);
        return (z0);
    }

    if (retp)
        *retp = XIok;
    for (const Zlist *zl = zref; zl; zl = zl->next) {
        BBox BB;
        zl->Z.BB(&BB);
        bool manh = zl->Z.is_rect();

        unsigned int n = find_objects(&BB);
        for (unsigned int i = 0; i < n; i++) {
            Zlist *zx = new Zlist(zdb_objects[i], 0);
            BBox tBB;
            zx->Z.BB(&tBB);
            if (!manh || !(tBB <= BB))
                Zlist::zl_and(&zx, &zl->Z);
            if (zx) {
                Zlist *zn = zx;
                while (zn->next)
                    zn = zn->next;
                zn->next = z0;
                z0 = zx;
            }
        }
    }
    try {
        z0 = Zlist::repartition(z0);
        return (z0);
    }
    catch (XIrt ret) {
        if (retp)
            *retp = ret;
        return (0);
    }
}


// Move the objects that overlap BB to the front of the list, and
// return the number of such elements.
//
unsigned int
zdb_t::find_objects(const BBox *BB)
{
    if (BB->left == db_xmin && BB->right == db_xmax && db_nx_valid) {
        if (db_nx_valid == 1)
            return (setup_row(db_nx, BB->bottom, BB->top));
        db_scan_cols = true;
        db_nx_valid = 0;
        db_ny_valid = 0;
    }
    else if (BB->bottom == db_ymin && BB->top == db_ymax && db_ny_valid) {
        if (db_ny_valid == 1)
            return (setup_col(db_ny, BB->left, BB->right));
        db_scan_cols = false;
        db_nx_valid = 0;
        db_ny_valid = 0;
    }
    if (db_scan_cols) {
        unsigned int num = setup_col(db_num_objects, BB->left, BB->right);
        return (setup_row(num, BB->bottom, BB->top));
    }
    else {
        unsigned int num = setup_row(db_num_objects, BB->bottom, BB->top);
        return (setup_col(num, BB->left, BB->right));
    }
}


unsigned int
zdb_t::setup_row(unsigned int num, int ymin, int ymax)
{
    if (db_ny_valid) {
        if (ymin == db_ymin && ymax == db_ymax)
            return (db_ny);
        if (ymin > db_ymin) {
            if (!db_y_dsc)
                num = db_ny0;
            else
                db_y_dsc = false;
        }
        else if (ymax < db_ymax) {
            if (db_y_dsc)
                num = db_ny0;
            else
                db_y_dsc = true;
        }
    }
    if (db_y_dsc) {
        db_ny0 = order_bl(num, ymax);
        db_ny = order_tg(db_ny0, ymin);
    }
    else {
        db_ny0 = order_tg(num, ymin);
        db_ny = order_bl(db_ny0, ymax);
    }
    db_ymin = ymin;
    db_ymax = ymax;
    db_ny_valid = db_nx_valid + 1;
    return (db_ny);
}


unsigned int
zdb_t::setup_col(unsigned int num, int xmin, int xmax)
{

    if (db_nx_valid) {
        if (xmin == db_xmin && xmax == db_xmax)
            return (db_nx);
        if (xmin > db_xmin) {
            if (!db_x_dsc)
                num = db_nx0;
            else
                db_x_dsc = false;
        }
        else if (xmax < db_xmax) {
            if (db_x_dsc)
                num = db_nx0;
            else
                db_x_dsc = true;
        }
    }
    if (db_x_dsc) {
        db_nx0 = order_ll(num, xmax);
        db_nx = order_rg(db_nx0, xmin);
    }
    else {
        db_nx0 = order_rg(num, xmin);
        db_nx = order_ll(db_nx0, xmax);
    }
    db_xmin = xmin;
    db_xmax = xmax;
    db_nx_valid = db_ny_valid + 1;
    return (db_nx);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

zbins_t::zbins_t(zdb_t *zdb, int x, int y, unsigned int nx, unsigned int ny,
    unsigned int dx, unsigned int dy, unsigned int bx, unsigned int by)
{
    b_x = x;
    b_y = y;
    b_nx = nx > 0 ? nx : 1;
    b_ny = ny > 0 ? ny : 1;
    b_dx = dx;
    b_dy = dy;
    b_bx = bx;
    b_by = by;

    b_array = new Zlist*[b_nx*b_ny];
    memset(b_array, 0, b_nx*b_ny*sizeof(Zoid*));

    for (unsigned int i = 0; i < zdb->num_objects(); i++)
        add(zdb->objects()[i]);
}


zbins_t::~zbins_t()
{
    unsigned int n = b_nx*b_ny;
    for (unsigned int i = 0; i < n; i++)
        Zlist::destroy(b_array[i]);
    delete [] b_array;
}


void
zbins_t::add(Zoid *z)
{
    if (z->is_bad())   
        return;

    unsigned int xmin, xmax, ymin, ymax;
    if (!overlap(z, &xmin, &xmax, &ymin, &ymax))
        return;

    if (z->is_rect()) {
        for (unsigned int i = ymin; i <= ymax; i++) {
            int yb = b_y + i*b_dy;
            int yt = yb + b_dy + b_by;
            yb -= b_by;
            if (z->yl > yb)
                yb = z->yl;
            if (z->yu < yt)
                yt = z->yu;
            if (yt <= yb)
                continue;
            for (unsigned int j = xmin; j <= xmax; j++) {
                int xl = b_x + j*b_dx;
                int xr = xl + b_dx + b_bx;
                xl -= b_bx;
                if (z->xll > xl)
                    xl = z->xll;
                if (z->xlr < xr)
                    xr = z->xlr;
                if (xr <= xl)
                    continue;
                int os = i*b_nx + j;
                b_array[os] = new Zlist(xl, yb, xr, yt, b_array[os]);
            }
        }
    }
    else {
        for (unsigned int i = ymin; i <= ymax; i++) {
            int y = b_y + i*b_dy;
            for (unsigned int j = xmin; j <= xmax; j++) {
                int x = b_x + j*b_dx;
                BBox tBB(x - b_bx, y - b_by, x + b_dx + b_bx, y + b_dy + b_by);
                Zlist *zx = z->clip_to(&tBB);
                if (zx) {
                    Zlist *zn = zx;
                    while (zn->next)
                        zn = zn->next;
                    int os = i*b_nx + j;
                    zn->next = b_array[os];
                    b_array[os] = zx;
                }
            }
        }
    }
}


void
zbins_t::add(Zlist *z0)
{
    Zlist *znxt;
    for (Zlist *zl = z0; zl; zl = znxt) {
        znxt = zl->next;

        unsigned int xmin, xmax, ymin, ymax;
        if (!overlap(&zl->Z, &xmin, &xmax, &ymin, &ymax)) {
            delete zl;
            continue;
        }

        Zoid Z(zl->Z);
        delete zl;

        if (Z.is_rect()) {
            for (unsigned int i = ymin; i <= ymax; i++) {
                int yb = b_y + i*b_dy;
                int yt = yb + b_dy + b_by;
                yb -= b_by;
                if (Z.yl > yb)
                    yb = Z.yl;
                if (Z.yu < yt)
                    yt = Z.yu;
                if (yt <= yb)
                    continue;
                for (unsigned int j = xmin; j <= xmax; j++) {
                    int xl = b_x + j*b_dx;
                    int xr = xl + b_dx + b_bx;
                    xl -= b_bx;
                    if (Z.xll > xl)
                        xl = Z.xll;
                    if (Z.xlr < xr)
                        xr = Z.xlr;
                    if (xr <= xl)
                        continue;
                    int os = i*b_nx + j;
                    b_array[os] = new Zlist(xl, yb, xr, yt, b_array[os]);
                }
            }
        }
        else {
            for (unsigned int i = ymin; i <= ymax; i++) {
                int y = b_y + i*b_dy;
                for (unsigned int j = xmin; j <= xmax; j++) {
                    int x = b_x + j*b_dx;
                    BBox tBB(x - b_bx, y - b_by,
                        x + b_dx + b_bx, y + b_dy + b_by);
                    Zlist *zx = Z.clip_to(&tBB);
                    if (zx) {
                        Zlist *zn = zx;
                        while (zn->next)
                            zn = zn->next;
                        int os = i*b_nx + j;
                        zn->next = b_array[os];
                        b_array[os] = zx;
                    }
                }
            }
        }
    }
}


void
zbins_t::merge()
{
    if (b_array) {
        int sz = b_nx*b_ny;
        for (int i = 0; i < sz; i++) {
#ifdef ZBINS_DBG
            int y = i/b_nx;
            int x = i%b_nx;
            printf("%g,%g -- %g,%g\n", MICRONS(b_x + x*b_dx),
                MICRONS(b_y + y*b_dy), MICRONS(b_x + x*b_dx + b_dx),
                MICRONS(b_y + y*b_dy + b_dy));
#endif
            b_array[i] = Zlist::repartition_ni(b_array[i]);
        }
    }
}


// This returns the zoids from the bin containing the bounding box
// center of the first zoid in zref (or the 0,0 bin if zref is null).
// There is no clipping, and only one bin is returned.
//
Zlist *
zbins_t::getZlist(const Zlist *zref, XIrt *retp)
{
    if (retp)
        *retp = XIok;
    int nx = 0;
    int ny = 0;
    if (zref) {
        int midx = (zref->Z.minleft() + zref->Z.maxright())/2;
        nx = (midx - b_x)/(int)b_dx;
        if (nx < 0 || (unsigned int)nx >= b_nx)
            return (0);
        int midy = (zref->Z.yl + zref->Z.yu)/2;
        ny = (midy - b_y)/(int)b_dy;
        if (ny < 0 || (unsigned int)ny >= b_ny)
            return (0);
    }
    return (Zlist::copy(getZlist(nx, ny)));
}


bool
zbins_t::overlap(const BBox *AOI, unsigned int *xmin, unsigned int *xmax,
    unsigned int *ymin, unsigned int *ymax)
{
    int xn = (AOI->left - (b_x + (int)b_bx));
    if (xn < 0)
        xn = 0;
    else {
        if ((unsigned int)xn >= b_nx*b_dx)
            return (false);
        xn /= b_dx;
    }

    int xm = (AOI->right - (b_x - (int)b_bx) - 1);
    if (xm < 0)
        return (false);
    else {
        xm /= b_dx;
        if ((unsigned int)xm >= b_nx)
            xm = b_nx - 1;
    }

    int yn = (AOI->bottom - (b_y + (int)b_by));
    if (yn < 0)
        yn = 0;
    else {
        if ((unsigned int)yn >= b_ny*b_dy)
            return (false);
        yn /= b_dy;
    }

    int ym = (AOI->top - (b_y - (int)b_by) - 1);
    if (ym < 0)
        return (false);
    else {
        ym /= b_dy;
        if ((unsigned int)ym >= b_ny)
            ym = b_ny - 1;
    }

    *xmin = xn;
    *xmax = xm;
    *ymin = yn;
    *ymax = ym;
    return (true);
}

