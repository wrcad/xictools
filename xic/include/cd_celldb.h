
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2012 Whiteley Research Inc, all rights reserved.        *
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
 $Id: cd_celldb.h,v 5.5 2015/03/13 06:37:08 stevew Exp $
 *========================================================================*/

#ifndef CD_CELLDB_H
#define CD_CELLDB_H

#include "symtab.h"


// Reserved name for main symbol table.
#define CD_MAIN_ST_NAME "main"


// Accessory Cell Table
//
// This is a hash table for saving cell names.  It can automatically
// expand all names in a hierarchy.  This ia an accessory and not used
// in the main cell database.

struct ct_elt
{
    unsigned long tab_key()         { return (unsigned long)name; }
    ct_elt *tab_next()              { return (next); }
    void set_tab_next(ct_elt *n)    { next = n; }
    ct_elt *tgen_next(bool)         { return (next); }

    void set(CDcellName k)          { name = k; next = 0; written = false; }
    bool is_written()               { return (written); }
    void set_written(bool b)        { written = b; }

private:
    CDcellName name;
    ct_elt *next;
    bool written;
};

// When expanding, the entire hierarchy of cell names is added in the
// add method.
//
struct CDcellTab
{
    CDcellTab()                     { ct_table = 0; }
    ~CDcellTab()                    { delete ct_table; }

    ct_elt *find(CDcellName name)
        { return (ct_table ? ct_table->find(name) : 0); }
    ct_elt *remove(CDcellName name)
        { return (ct_table ? ct_table->remove(name) : 0); }

    void clear()
        {
            delete ct_table;
            ct_table = 0;
            ct_eltab.clear();
        }

    bool add(const char*, bool);
    stringlist *list();
    void clear_written();

    static void do_test_ptrs();

private:
    itable_t<ct_elt> *ct_table;
    eltab_t<ct_elt> ct_eltab;
};


// This is the cell table struct.  It contains separate hash tables
// for electrical and physical cells.  There is provision for giving
// the table a name, and for linking tables together for context
// switching.
//
// The cells are indexed by name.  The cell names are obtained from a
// string table and are never null.  Thus, names can be value compared
// for identity.

struct CDs;

struct CDtables
{
    CDtables(const char*);
    ~CDtables();

    CDs *findPhysCell(const char*);
    CDs *removePhysCell(const char*);
    CDs *linkPhysCell(CDs*);
    CDs *unlinkPhysCell(CDs*);

    CDs *findElecCell(const char*);
    CDs *removeElecCell(const char*);
    CDs *linkElecCell(CDs*);
    CDs *unlinkElecCell(CDs*);

    const char *tableName() { return (table_name->string()); }

    CDtables *nextTable() { return (next); }
    void setNextTable(CDtables *t) { next = t; }

    itable_t<CDs> *cellTable(DisplayMode m)
        { return (m == Physical ? phys_table : elec_table); }

    unsigned int allocated(DisplayMode m)
        {
            if (m == Physical && phys_table)
                return (phys_table->allocated());
            if (m == Electrical && elec_table)
                return (elec_table->allocated());
            return (0);
        }

    const char *cellname()      const { return (cell_name->string()); }
    void setCellname(CDcellName n)  { cell_name = n; }

    CDcellTab *auxCellTab()     { return (aux_cell_tab); }

private:
    CDcellName table_name;      // identifier
    CDcellName cell_name;       // remember current cell
    CDtables *next;             // next available table
    itable_t<CDs> *phys_table;  // physical cells
    itable_t<CDs> *elec_table;  // electrical cells
    CDs *prev_phys;             // cached last find
    CDs *prev_elec;             // cached last find
    CDcellTab *aux_cell_tab;    // auxiliary cell name table
};


// Below is the cell table container main class, which provides the
// application interface to the cell tables.

inline class cCDcdb *CDcdb();

class cCDcdb
{
    static cCDcdb *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cCDcdb *CDcdb() { return (cCDcdb::ptr()); }

    cCDcdb();

    CDs *findCell(CDcellName, DisplayMode);
    CDs *findCell(const char*, DisplayMode);
    CDs *insertCell(const char*, DisplayMode);
    CDs *removeCell(CDcellName, DisplayMode);
    CDs *linkCell(CDs*);
    CDs *unlinkCell(CDs*);
    bool findSymbol(CDcellName, CDcbin* = 0);
    bool findSymbol(const char*, CDcbin* = 0);
    void removeSymbol(CDcellName, CDcbin* = 0);
    bool findTable(const char*);
    const char *tableName();
    void switchTable(const char*);
    void rotateTable();
    stringlist *listTables();
    const char *tableCellName();
    void setTableCellname(CDcellName);
    void clearTable(bool);
    void clearAllTables();

    bool isNoCellCallback()     { return (c_no_cell_callback); }
    bool isEmpty()              { return (c_tables == 0); }

    void incNoInsertNew()       { c_no_insert_new++; }
    void decNoInsertNew()       { c_no_insert_new--; }

    unsigned int cellCount(DisplayMode m)
        {
            return (c_tables ? c_tables->allocated(m) : 0);
        }

    itable_t<CDs> *cellTable(DisplayMode m)
        {
            return (c_tables ? c_tables->cellTable(m) : 0);
        }

    CDcellTab *auxCellTab()
        {
            return (c_tables ? c_tables->auxCellTab() : 0);
        }

private:
    CDtables *c_tables;
    int c_no_insert_new;
    bool c_no_cell_callback;

    static cCDcdb *instancePtr;
};


// Generator to iterate over cells in the table.
//
struct CDgenTab_s : public tgen_t<CDs>
{
    CDgenTab_s(itable_t<CDs> *t) : tgen_t<CDs>(t) { }
    CDgenTab_s(DisplayMode m) : tgen_t<CDs>(CDcdb()->cellTable(m)) { }

    CDs *s_first() { return (next()); }
    CDs *s_next() { return (next()); }
};


// Generator to iterate over both physical and electrical cells in the
// tables in a unified manner.  Each return contains a cell pair with
// the same name, at least one of which is nonzero.
//
struct CDgenTab_cbin
{
    CDgenTab_cbin() { pgen = 0; egen = 0; state = 0; }
    ~CDgenTab_cbin() { delete pgen; delete egen; }

    bool next(CDcbin*);

private:
    CDgenTab_s *pgen;
    CDgenTab_s *egen;
    int state;
};

#endif

