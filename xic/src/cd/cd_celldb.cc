
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
 $Id: cd_celldb.cc,v 5.7 2017/04/18 03:13:47 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "cd_celldb.h"


//
// cCDcdb:  The main database for cells.
//

cCDcdb *cCDcdb::instancePtr = 0;

cCDcdb::cCDcdb()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cCDcdb already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    c_tables = 0;
    c_subm_table = 0;
    c_no_insert_new = 0;
    c_no_cell_callback = false;
}


// Private static error exit.
//
void
cCDcdb::on_null_ptr()
{
    fprintf(stderr, "Singleton class cCDcdb used before instantiated.\n");
    exit(1);
}


// If the named cell exists, return it.  Otherwise, create and link a
// new cell, and return it.  If the no_insert_new integer is nonzero,
// new cells will not be created.
//
CDs *
cCDcdb::insertCell(const char *name, DisplayMode mode)
{
    if (!name || !*name)
        return (0);
    if (!c_tables)
        c_tables = new CDtables(0);
    CDs *sd;
    if (mode == Physical) {
        sd = c_tables->findPhysCell(name);
        if (!sd && !c_no_insert_new) {
            sd = new CDs(CD()->CellNameTableAdd(name), Physical);
            c_tables->linkPhysCell(sd);
        }
    }
    else {
        sd = c_tables->findElecCell(name);
        if (!sd && !c_no_insert_new) {
            sd = new CDs(CD()->CellNameTableAdd(name), Electrical);
            c_tables->linkElecCell(sd);
        }
    }
    return (sd);
}


CDs *
cCDcdb::removeCell(CDcellName name, DisplayMode mode)
{
    if (!c_tables)
        return (0);
    CDs *sd;
    if (mode == Physical)
        sd = c_tables->removePhysCell(Tstring(name));
    else
        sd = c_tables->removeElecCell(Tstring(name));
    return (sd);
}


// If a cell of the same name exists in the table, it will be returned.
// Otherwise, link in the cell passed.
//
CDs *
cCDcdb::linkCell(CDs *sdesc)
{
    if (!sdesc)
        return (0);
    if (!c_tables)
        c_tables = new CDtables(0);
    CDs *sd;
    if (sdesc->isElectrical())
        sd = c_tables->linkElecCell(sdesc);
    else
        sd = c_tables->linkPhysCell(sdesc);
    return (sd);
}


CDs *
cCDcdb::unlinkCell(CDs *sdesc)
{
    if (!sdesc)
        return (0);
    if (!c_tables)
        return (sdesc);
    CDs *sd;
    if (sdesc->isElectrical())
        sd = c_tables->unlinkElecCell(sdesc);
    else
        sd = c_tables->unlinkPhysCell(sdesc);
    return (sd);
}


bool
cCDcdb::findSymbol(CDcellName name, CDcbin *cbret)
{
    if (cbret)
        cbret->reset();
    CDs *sdphys = findCell(name, Physical);
    if (!cbret && sdphys)
        return (true);
    CDs *sdelec = findCell(name, Electrical);
    if (sdphys || sdelec) {
        if (cbret) {
            cbret->setPhys(sdphys);
            cbret->setElec(sdelec);
        }
        return (true);
    }
    return (false);
}


bool
cCDcdb::findSymbol(const char *name, CDcbin *cbret)
{
    if (cbret)
        cbret->reset();
    CDcellName cn = CD()->CellNameTableFind(name);
    CDs *sdphys = findCell(cn, Physical);
    if (!cbret && sdphys)
        return (true);
    CDs *sdelec = findCell(cn, Electrical);
    if (sdphys || sdelec) {
        if (cbret) {
            cbret->setPhys(sdphys);
            cbret->setElec(sdelec);
        }
        return (true);
    }
    return (false);
}


void
cCDcdb::removeSymbol(CDcellName name, CDcbin *cbret)
{
    CDs *sdphys = removeCell(name, Physical);
    CDs *sdelec = removeCell(name, Electrical);
    if (cbret) {
        cbret->setPhys(sdphys);
        cbret->setElec(sdelec);
    }
}


//
// The next few functions enable symbol table switching.
//

// Return true if the named table exists.
//
bool
cCDcdb::findTable(const char *name)
{
    if (!name || !*name || !strcmp(name, CD_MAIN_ST_NAME))
        // The main table always exists.
        return (true);
    for (CDtables *t = c_tables; t; t = t->nextTable()) {
        if (!strcmp(name, t->tableName()))
            return (true);
    }
    return (false);
}


// Return the name of the current table.
//
const char *
cCDcdb::tableName()
{
    if (c_tables)
        return (c_tables->tableName());
    return (0);
}


// Switch to the given symbol table.  If name is null, empty or
// MAIN_ST_NAME, the main table is restored, otherwise a new table is
// created if it doesn't exist, and the database is hooked to the
// named table.
//
void
cCDcdb::switchTable(const char *name)
{
    if (!name || !*name)
        name = CD_MAIN_ST_NAME;
    CDtables *t;
    for (t = c_tables; t; t = t->nextTable()) {
        if (!strcmp(name, t->tableName()))
            break;
    }
    if (t) {
        if (t != c_tables) {
            CDtables *tp = c_tables, *tn;
            for (CDtables *tx = tp->nextTable(); tx; tx = tn) {
                tn = tx->nextTable();
                if (tx == t) {
                    tp->setNextTable(tn);
                    break;
                }
                tp = tx;
            }
            t->setNextTable(c_tables);
            c_tables = t;
        }
        return;
    }
    t = new CDtables(name);
    t->setNextTable(c_tables);
    c_tables = t;
}


void
cCDcdb::rotateTable()
{
    if (!c_tables || !c_tables->nextTable())
        return;
    CDtables *t = c_tables;
    c_tables = c_tables->nextTable();
    t->setNextTable(0);
    CDtables *te = c_tables;
    while (te->nextTable())
        te = te->nextTable();
    te->setNextTable(t);
}


// Return a stringlist containing the symbol table names.  The first
// entry is the current table.
//
stringlist *
cCDcdb::listTables()
{
    stringlist *s0 = 0, *se = 0;
    for (CDtables *t = c_tables; t; t = t->nextTable()) {
        if (s0) {
            se->next = new stringlist(lstring::copy(t->tableName()), 0);
            se = se->next;
        }
        else
            s0 = se = new stringlist(lstring::copy(t->tableName()), 0);
    }
    return (s0);
}


// Return the name of the "current" cell for the present table.  This
// is used to remember the cell being displayed when tables are
// switched.
//
const char *
cCDcdb::tableCellName()
{
    if (c_tables)
        return (c_tables->cellname());
    return (0);
}


// Set the cellname saved in the current table.
//
void
cCDcdb::setTableCellname(CDcellName cn)
{
    if (c_tables)
        c_tables->setCellname(cn);
}


// Remove and destroy the contents of the table, cleared for reuse.
//
void
cCDcdb::clearTable()
{
    if (c_tables)
        c_tables->clear();
}


// Destroy and remove the current table and its contents.  The
// argument is true if we are destroying the main table in use by the
// application, false if the table is a subsidiary or temporary table.
//
void
cCDcdb::destroyTable(bool is_main_table)
{
    c_no_cell_callback = !is_main_table;
    // This suppresses callbacks when cells are destroyed in a temporary
    // symbol table.

    CDtables *tx = c_tables->nextTable();
    delete c_tables;
    c_tables = tx;
    c_no_cell_callback = false;
}


// Clear all tables and cells.
//
void
cCDcdb::destroyAllTables()
{
    while (c_tables)
        destroyTable(true);
    delete c_subm_table;
}
// End of cCDcdb functions.


//
// CDtables:  Hash tables for cells.
//

CDtables::CDtables(const char *n)
{
    if (!n)
        n = CD_MAIN_ST_NAME;
    // Use the cell name table to allocate the table names, too.
    t_table_name = CD()->CellNameTableAdd(n);
    t_cell_name = 0;
    t_next = 0;
    t_phys_table = 0;
    t_elec_table = 0;
    t_prev_phys = 0;
    t_prev_elec = 0;
    t_aux_cell_tab = new CDcellTab();
}


void
CDtables::clear()
{
    CDgenTab_s egen(t_elec_table);
    CDs *sd;
    while ((sd = egen.next()) != 0)
        delete sd;
    delete t_elec_table;
    t_elec_table = 0;
    t_prev_elec = 0;

    CDgenTab_s pgen(t_phys_table);
    while ((sd = pgen.next()) != 0)
        delete sd;
    delete t_phys_table;
    t_phys_table = 0;
    t_prev_phys = 0;

    delete t_aux_cell_tab;
    t_aux_cell_tab = 0;
}


CDs *
CDtables::findPhysCell(const char *name)
{
    if (!t_prev_phys || t_prev_phys->cellname() != (CDcellName)name) {
        if (!t_phys_table || !name)
            return (0);
        t_prev_phys = t_phys_table->find(name);
    }
    return (t_prev_phys);
}


CDs *
CDtables::removePhysCell(const char *name)
{
    if (!t_phys_table || !name)
        return (0);
    CDs *sd = t_phys_table->remove(name);
    if (sd) {
        t_phys_table = t_phys_table->check_rehash();
        if (sd == t_prev_phys)
            t_prev_phys = 0;
    }
    return (sd);
}


CDs *
CDtables::linkPhysCell(CDs *sdesc)
{
    if (!sdesc)
        return (0);
    if (!t_phys_table)
        t_phys_table = new itable_t<CDs>;
    CDs *sd = t_phys_table->link(sdesc);
    t_phys_table = t_phys_table->check_rehash();
    t_prev_phys = sd;
    return (sd);
}


CDs *
CDtables::unlinkPhysCell(CDs *sdesc)
{
    if (!sdesc)
        return (0);
    if (!t_phys_table)
        return (sdesc);
    CDs *sd = t_phys_table->unlink(sdesc);
    if (sd) {
        t_phys_table = t_phys_table->check_rehash();
        if (sd == t_prev_phys)
            t_prev_phys = 0;
    }
    return (sd);
}


CDs *
CDtables::findElecCell(const char *name)
{
    if (!t_prev_elec || t_prev_elec->cellname() != (CDcellName)name) {
        if (!t_elec_table || !name)
            return (0);
        t_prev_elec = t_elec_table->find(name);
    }
    return (t_prev_elec);
}


CDs *
CDtables::removeElecCell(const char *name)
{
    if (!t_elec_table || !name)
        return (0);
    CDs *sd = t_elec_table->remove(name);
    if (sd) {
        t_elec_table = t_elec_table->check_rehash();
        if (sd == t_prev_elec)
            t_prev_elec = 0;
    }
    return (sd);
}


CDs *
CDtables::linkElecCell(CDs *sdesc)
{
    if (!sdesc)
        return (0);
    if (sdesc->isSymbolic()) {
        CD()->DbgError("symbolic", "linkElecCell");
        return (0);
    }
    if (!t_elec_table)
        t_elec_table = new itable_t<CDs>;
    CDs *sd = t_elec_table->link(sdesc);
    t_elec_table = t_elec_table->check_rehash();
    t_prev_elec = sd;
    return (sd);
}


CDs *
CDtables::unlinkElecCell(CDs *sdesc)
{
    if (!sdesc)
        return (0);
    if (!t_elec_table)
        return (sdesc);
    CDs *sd = t_elec_table->unlink(sdesc);
    if (sd) {
        t_elec_table = t_elec_table->check_rehash();
        if (sd == t_prev_elec)
            t_prev_elec = 0;
    }
    return (sd);
}
// End of CDtables functions.


//
// CDcellTab:  Auxiliary cell name table.
//

// Add a cell name to the table.  If recurse, add the cell and all
// cell names in its hierarchy (as currently in the database).
// Otherwise, or in recurse mode and the cell is not found in the
// database, add the name.  In any case, the name must exist in the
// global string table.
//
bool
CDcellTab::add(const char *cellname, bool recurse)
{
    if (recurse) {
        CDs *sd = CDcdb()->findCell(cellname, Physical);
        if (sd) {
            if (!ct_table)
                ct_table = new itable_t<ct_elt>;
            CDgenHierDn_s gen(sd);
            CDs *tsd;
            while ((tsd = gen.next()) != 0) {
                if (ct_table->find(tsd->cellname()) == 0) {
                    ct_elt *elt = ct_eltab.new_element();
                    elt->set(tsd->cellname());
                    ct_table->link(elt, false);
                    ct_table = ct_table->check_rehash();
                }
            }
            return (true);
        }
    }

    CDcellName name = CD()->CellNameTableFind(cellname);
    if (!name)
        return (false);
    if (!ct_table)
        ct_table = new itable_t<ct_elt>;
    if (ct_table->find(name) == 0) {
        ct_elt *elt = ct_eltab.new_element();
        elt->set(name);
        ct_table->link(elt, false);
        ct_table = ct_table->check_rehash();
    }
    return (true);
}


stringlist *
CDcellTab::list()
{
    if (!ct_table)
        return (0);
    tgen_t<ct_elt> gen(ct_table);
    ct_elt *el;
    stringlist *s0 = 0;
    while ((el = gen.next()) != 0)
        s0 = new stringlist(lstring::copy((char*)el->tab_key()), s0);
    stringlist::sort(s0);
    return (s0);
}


void
CDcellTab::clear_written()
{
    if (!ct_table)
        return;
    tgen_t<ct_elt> gen(ct_table);
    ct_elt *el;
    while ((el = gen.next()) != 0)
        el->set_written(false);
}


namespace {
    void
    test_ptrs(CDs *sd)
    {
        if (!sd)
            return;
        const char *str = sd->isElectrical() ? "elec" : "phys";
        const char *name = Tstring(sd->cellname());

        CDm_gen rgen(sd, GEN_MASTER_REFS);
        for (CDm *mstr = rgen.m_first(); mstr; mstr = rgen.m_next()) {
            if (mstr->celldesc() != sd)
                printf("Error: in %s %s, master ref backpointer inconsistent\n",
                    str, name);
            CDc_gen cgen(mstr);
            for (CDc *cd = cgen.c_first(); cd; cd = cgen.c_next()) {
                if (cd->master() != mstr)
                    printf(
                "Error: in %s %s, refs ObjRefs instance inconsistent, %s\n",
                        str, name, Tstring(mstr->cellname()));
            }
            CDc_gen cgen_u(mstr, true);
            for (CDc *cd = cgen_u.c_first(); cd; cd = cgen_u.c_next()) {
                if (cd->master() != mstr)
                    printf(
                "Error: in %s %s, refs Unlinked instance inconsistent, %s\n",
                        str, name, Tstring(mstr->cellname()));
            }
            if (!mstr->parent())
                printf("Error: in %s %s, ref no parent, %s\n",
                    str, name, Tstring(mstr->cellname()));
            if (mstr != mstr->parent()->findMaster(mstr->cellname()))
                printf("Error: in %s %s, ref master not in parent table, %s\n",
                    str, name, Tstring(mstr->cellname()));

        }
        CDm_gen mgen(sd, GEN_MASTERS);
        for (CDm *mstr = mgen.m_first(); mstr; mstr = mgen.m_next()) {
            CDc_gen cgen(mstr);
            for (CDc *cd = cgen.c_first(); cd; cd = cgen.c_next()) {
                if (cd->master() != mstr)
                    printf(
                "Error: in %s %s, list ObjRefs instance inconsistent, %s\n",
                        str, name, Tstring(mstr->cellname()));
            }
            CDc_gen cgen_u(mstr, true);
            for (CDc *cd = cgen_u.c_first(); cd; cd = cgen_u.c_next()) {
                if (cd->master() != mstr)
                    printf(
                "Error: in %s %s, list Unlinked instance inconsistent, %s\n",
                        str, name, Tstring(mstr->cellname()));
            }
            if (mstr->parent() != sd)
                printf("Error: in %s %s, no parent, %s\n", str, name,
                    Tstring(mstr->cellname()));
            if (!mstr->celldesc())
                printf("Error: in %s %s, no cell ptr, %s\n", str, name,
                    Tstring(mstr->cellname()));
            bool found = false;
            CDm_gen tgen(mstr->celldesc(), GEN_MASTER_REFS);
            for (CDm *m = tgen.m_first(); m; m = tgen.m_next()) {
                if (m == mstr) {
                    found = true;
                    break;
                }
            }
            if (!found)
                printf("Error: in %s %s, no back ptr, %s\n",
                    str, name, Tstring(mstr->cellname()));
        }
    }
}


// Static function.
// Diagnostic test.
//
void
CDcellTab::do_test_ptrs()
{
    CDgenTab_cbin sgen;
    CDcbin cbin;
    while (sgen.next(&cbin)) {
        if (cbin.phys()) {
            if (cbin.phys()->isElectrical())
                printf("Error: in %s, phys is electrical\n",
                    Tstring(cbin.cellname()));
            test_ptrs(cbin.phys());
        }
        if (cbin.elec()) {
            if (!cbin.elec()->isElectrical())
                printf("Error: in %s, elec is physical\n",
                    Tstring(cbin.cellname()));
            test_ptrs(cbin.elec());
        }
    }
    printf("done\n");
}
// End of CDcellTab functions.


bool
CDgenTab_cbin::next(CDcbin *cbin)
{
    if (!cbin)
        return (false);

    // Initially, go through the physical cells, and add a link
    // to the electrical cell (if it exists) to the return.

    if (state == 0) {
        if (!pgen)
            pgen = new CDgenTab_s(Physical);
        cbin->setPhys(pgen->next());
        if (cbin->phys()) {
            cbin->setElec(CDcdb()->findCell(
                cbin->phys()->cellname(), Electrical));
            return (true);
        }
        state = 1;
    }

    // After processing all physical cells, look through the
    // electrical cells.  Return only those that don't have
    // a physical counterpart, since these would be redundant.
    // Note that egen is not initialized until after the physical
    // looping is finished, since we may be freeing cells.

    if (!egen)
        egen = new CDgenTab_s(Electrical);
    for (;;) {
        cbin->setElec(egen->next());
        if (cbin->elec()) {
            if (CDcdb()->findCell(cbin->elec()->cellname(), Physical))
                continue;
            cbin->setPhys(0);
            return (true);
        }
        break;
    }
    return (false);
}
// End of CDgenTab_cbin functions.

