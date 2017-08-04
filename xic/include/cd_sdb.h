
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

#ifndef CD_SDB_H
#define CD_SDB_H

#include "geo_zlist.h"
#include "geo_alloc.h"


class cSDB;

inline class cCDsdbDB *CDsdb();

class cCDsdbDB
{
    static cCDsdbDB *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cCDsdbDB *CDsdb() { return (cCDsdbDB::ptr()); }

    cCDsdbDB();

    bool saveDB(cSDB*);
    cSDB *findDB(const char*);
    stringlist *listDB();
    void destroyDB(const char*);

private:
    SymTab *sdb_table;              // Table of cSDB databases

    static cCDsdbDB *instancePtr;
};

enum sdbType { sdbBdb, sdbOdb, sdbZdb, sdbZldb, sdbZbdb };

// Polymorphic database saved by name in cCDsdbDB.
//
class cSDB
{
public:
    cSDB(const char*, SymTab*, sdbType);
    ~cSDB();

    stringlist *layers();

    const char *name()      { return (dbname); }
    SymTab *table()         { return (db); }
    const BBox *BB()        { return (&dbBB); }
    sdbType type()          { return (dbtype); }
    int owner()             { return (dbowner); }
    void set_owner(int w)   { dbowner = w; }

protected:
    char *dbname;           // Tag name for the database.
    SymTab *db;             // Table of entries.
    BBox dbBB;              // Bounding box of entries.
    sdbType dbtype;         // Type of database.
    int dbowner;            // Owning window index, can't be freed if
                            // this is nonzero.
};


// Base class for database types.
//
struct base_db_t
{
    friend class cSDB;

    base_db_t() {
        db_num_objects = 0;
        db_list_size = 0;
        db_xmin = db_xmax = 0;
        db_ymin = db_ymax = 0;
        db_nx0 = db_nx = 0;
        db_ny0 = db_ny = 0;
        db_nx_valid = 0;
        db_ny_valid = 0;
        db_scan_cols = false;
        db_x_dsc = false;
        db_y_dsc = false;
    }

    unsigned int num_objects() { return (db_num_objects); }

protected:
    BBox db_BB;                         // area of content
    unsigned int db_num_objects;        // number of objects
    unsigned int db_list_size;          // array size

    int db_xmin, db_xmax;               // current x range
    int db_ymin, db_ymax;               // curernt y range
    unsigned int db_nx0, db_nx;         // current x offsets
    unsigned int db_ny0, db_ny;         // current y offsets
    unsigned char db_nx_valid;          // nx0, nx valid, ordering code
    unsigned char db_ny_valid;          // ny0, ny valid, ordering code
    bool db_scan_cols;                  // true when row-major
    bool db_x_dsc;                      // true if scanning left
    bool db_y_dsc;                      // true if scanning down
};


// A simple and efficient flat read-only database for boxes.
//
struct bdb_t : public base_db_t
{
    bdb_t() { bdb_objects = 0; }
    ~bdb_t();

    BBox **objects() { return (bdb_objects); }

    void add(const BBox*);
    Blist *getBlist(Blist*, XIrt*);
    Zlist *getZlist(const Zlist*, XIrt*);
    unsigned int find_objects(const BBox*);

private:
    unsigned int setup_row(unsigned int, int, int);
    unsigned int setup_col(unsigned int, int, int);

    unsigned int order_bl(unsigned int num, int ymax)
        {
            unsigned int n1 = 0;
            BBox **objs = bdb_objects;
            while (n1 < num && objs[n1]->bottom < ymax)
                n1++;
            for (unsigned int n2 = n1 + 1; n2 < num; n2++) {
                if (objs[n2]->bottom < ymax) {
                    BBox *t = objs[n1];
                    objs[n1] = objs[n2];
                    objs[n2] = t;
                    n1++;
                }
            }
            return (n1);
        }

    unsigned int order_tg(unsigned int num, int ymin)
        {
            unsigned int n1 = 0;
            BBox **objs = bdb_objects;
            while (n1 < num && objs[n1]->top > ymin)
                n1++;
            for (unsigned int n2 = n1 + 1; n2 < num; n2++) {
                if (objs[n2]->top > ymin) {
                    BBox *t = objs[n1];
                    objs[n1] = objs[n2];
                    objs[n2] = t;
                    n1++;
                }
            }
            return (n1);
        }

    unsigned int order_ll(unsigned int num, int xmax)
        {
            unsigned int n1 = 0;
            BBox **objs = bdb_objects;
            while (n1 < num && objs[n1]->left < xmax)
                n1++;
            for (unsigned int n2 = n1 + 1; n2 < num; n2++) {
                if (objs[n2]->left < xmax) {
                    BBox *t = objs[n1];
                    objs[n1] = objs[n2];
                    objs[n2] = t;
                    n1++;
                }
            }
            return (n1);
        }

    unsigned int order_rg(unsigned int num, int xmin)
        {
            unsigned int n1 = 0;
            BBox **objs = bdb_objects;
            while (n1 < num && objs[n1]->right > xmin)
                n1++;
            for (unsigned int n2 = n1 + 1; n2 < num; n2++) {
                if (objs[n2]->right > xmin) {
                    BBox *t = objs[n1];
                    objs[n1] = objs[n2];
                    objs[n2] = t;
                    n1++;
                }
            }
            return (n1);
        }

    BBox **bdb_objects;                 // array of objects
    tGEOfact<BBox> allocator;           // object memory management
};


// A simple and efficient flat read-only database for objects.
//
struct odb_t : public base_db_t
{
    odb_t() { odb_objects = 0; }
    ~odb_t();

    CDo **objects() { return (odb_objects); }

    void add(CDo*);
    Zlist *getZlist(const Zlist*, XIrt*);
    unsigned int find_objects(const BBox*);

private:
    unsigned int setup_row(unsigned int, int, int);
    unsigned int setup_col(unsigned int, int, int);

    unsigned int order_bl(unsigned int num, int ymax)
        {
            unsigned int n1 = 0;
            CDo **objs = odb_objects;
            while (n1 < num && objs[n1]->oBB().bottom < ymax)
                n1++;
            for (unsigned int n2 = n1 + 1; n2 < num; n2++) {
                if (objs[n2]->oBB().bottom < ymax) {
                    CDo *t = objs[n1];
                    objs[n1] = objs[n2];
                    objs[n2] = t;
                    n1++;
                }
            }
            return (n1);
        }

    unsigned int order_tg(unsigned int num, int ymin)
        {
            unsigned int n1 = 0;
            CDo **objs = odb_objects;
            while (n1 < num && objs[n1]->oBB().top > ymin)
                n1++;
            for (unsigned int n2 = n1 + 1; n2 < num; n2++) {
                if (objs[n2]->oBB().top > ymin) {
                    CDo *t = objs[n1];
                    objs[n1] = objs[n2];
                    objs[n2] = t;
                    n1++;
                }
            }
            return (n1);
        }

    unsigned int order_ll(unsigned int num, int xmax)
        {
            unsigned int n1 = 0;
            CDo **objs = odb_objects;
            while (n1 < num && objs[n1]->oBB().left < xmax)
                n1++;
            for (unsigned int n2 = n1 + 1; n2 < num; n2++) {
                if (objs[n2]->oBB().left < xmax) {
                    CDo *t = objs[n1];
                    objs[n1] = objs[n2];
                    objs[n2] = t;
                    n1++;
                }
            }
            return (n1);
        }

    unsigned int order_rg(unsigned int num, int xmin)
        {
            unsigned int n1 = 0;
            CDo **objs = odb_objects;
            while (n1 < num && objs[n1]->oBB().right > xmin)
                n1++;
            for (unsigned int n2 = n1 + 1; n2 < num; n2++) {
                if (objs[n2]->oBB().right > xmin) {
                    CDo *t = objs[n1];
                    objs[n1] = objs[n2];
                    objs[n2] = t;
                    n1++;
                }
            }
            return (n1);
        }

    CDo **odb_objects;                  // array of objects
};


// A simple and efficient read-only database for trapezoids.
//
struct zdb_t : public base_db_t
{
    zdb_t() { zdb_objects = 0; }
    ~zdb_t();

    Zoid **objects() { return (zdb_objects); }

    void add(Zoid*);
    Zlist *getZlist(const Zlist* = 0, XIrt* = 0);
    unsigned int find_objects(const BBox*);

private:
    unsigned int setup_row(unsigned int, int, int);
    unsigned int setup_col(unsigned int, int, int);

    unsigned int order_bl(unsigned int num, int ymax)
        {
            unsigned int n1 = 0;
            Zoid **objs = zdb_objects;
            while (n1 < num && objs[n1]->yl < ymax)
                n1++;
            for (unsigned int n2 = n1 + 1; n2 < num; n2++) {
                if (objs[n2]->yl < ymax) {
                    Zoid *t = objs[n1];
                    objs[n1] = objs[n2];
                    objs[n2] = t;
                    n1++;
                }
            }
            return (n1);
        }

    unsigned int order_tg(unsigned int num, int ymin)
        {
            unsigned int n1 = 0;
            Zoid **objs = zdb_objects;
            while (n1 < num && objs[n1]->yu > ymin)
                n1++;
            for (unsigned int n2 = n1 + 1; n2 < num; n2++) {
                if (objs[n2]->yu > ymin) {
                    Zoid *t = objs[n1];
                    objs[n1] = objs[n2];
                    objs[n2] = t;
                    n1++;
                }
            }
            return (n1);
        }

    unsigned int order_ll(unsigned int num, int xmax)
        {
            unsigned int n1 = 0;
            Zoid **objs = zdb_objects;
            while (n1 < num && objs[n1]->minleft() < xmax)
                n1++;
            for (unsigned int n2 = n1 + 1; n2 < num; n2++) {
                if (objs[n2]->minleft() < xmax) {
                    Zoid *t = objs[n1];
                    objs[n1] = objs[n2];
                    objs[n2] = t;
                    n1++;
                }
            }
            return (n1);
        }

    unsigned int order_rg(unsigned int num, int xmin)
        {
            unsigned int n1 = 0;
            Zoid **objs = zdb_objects;
            while (n1 < num && objs[n1]->maxright() > xmin)
                n1++;
            for (unsigned int n2 = n1 + 1; n2 < num; n2++) {
                if (objs[n2]->maxright() > xmin) {
                    Zoid *t = objs[n1];
                    objs[n1] = objs[n2];
                    objs[n2] = t;
                    n1++;
                }
            }
            return (n1);
        }

    Zoid **zdb_objects;                 // array of objects
    tGEOfact<Zoid> allocator;           // object memory management
};


// A spatially-binned database for trapezoids.  Each grid cell contains
// a Zlist clipped to that cell plus boundary.
//
struct zbins_t
{
    zbins_t(int x, int y, unsigned int nx, unsigned int ny,
            unsigned int dx, unsigned int dy,
            unsigned int bx, unsigned int by)
        {
            b_x = x;
            b_y = y;
            b_nx = nx > 0 ? nx : 1;
            b_ny = ny > 0 ? ny : 1;
            b_dx = dx;
            b_dy = dy;
            b_bx = bx;
            b_by = by;
            b_BB.left = b_x - b_bx;
            b_BB.bottom = b_y - b_by;
            b_BB.right = b_x + b_nx*b_dx + b_bx;
            b_BB.top = b_y + b_ny*b_dy + b_by;

            b_array = new Zlist*[b_nx*b_ny];
            memset(b_array, 0, b_nx*b_ny*sizeof(Zoid*));
        }

    zbins_t(zdb_t*, int, int, unsigned int, unsigned int,
            unsigned int, unsigned int, unsigned int, unsigned int);

    ~zbins_t();

    Zlist *getZlist(unsigned int nx, unsigned int ny)
        {
            if (nx >= b_nx || ny >= b_ny)
                return (0);
            return (b_array[ny*b_nx + nx]);
        }

    const BBox *BB() { return (&b_BB); }

    bool overlap(Zoid *z, unsigned int *xmin, unsigned int *xmax,
        unsigned int *ymin, unsigned int *ymax)
        {
            BBox tBB;
            z->BB(&tBB);
            return (overlap(&tBB, xmin, xmax, ymin, ymax));
        }

    void add(Zoid*);
    void add(Zlist*);
    void merge();
    Zlist *getZlist(const Zlist* = 0, XIrt* = 0);
    bool overlap(const BBox*, unsigned int*, unsigned int*,
        unsigned int*, unsigned int*);

private:
    Zlist **b_array;
    int b_x, b_y;               // Grid origin, lower-left corner
                                // excluding boundary.
    unsigned int b_nx, b_ny;    // Number x, y cells.
    unsigned int b_dx, b_dy;    // Grid size.
    unsigned int b_bx, b_by;    // Boundary width, grid cells are
                                // effectively bloated by this amount
                                // (so there will be overlap).
    BBox b_BB;                  // area of content
};

#endif

