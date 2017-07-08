
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
 $Id: cd_digest.cc,v 5.4 2014/11/05 05:43:33 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_propnum.h"
#include "cd_digest.h"
#include "fio.h"
#include "fio_chd.h"
#include "fio_alias.h"
#include "fio_oasis.h"
#include "fio_cgd.h"


//
// The Cell Hierarchy Digests (CHDs) database.  These are
// highly-compressed representations of cell hierarchies.
//

cCDchdDB *cCDchdDB::instancePtr = 0;

cCDchdDB::cCDchdDB()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cCDchdDB already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    chd_table = 0;
}


// Private static error exit.
//
void
cCDchdDB::on_null_ptr()
{
    fprintf(stderr, "Singleton class cCDchdDB used before instantiated.\n");
    exit(1);
}


// Store a CHD under name in a symbol table.
//
bool
cCDchdDB::chdStore(const char *name, cCHD *chd)
{
    if (chd->isStored()) {
        Errs()->add_error("ChdStore: digest already in storage");
        return (false);
    }
    if (!chd_table)
        chd_table = new SymTab(true, false);
    name = lstring::copy(name);
    if (!chd_table->add(name, chd, true)) {
        Errs()->add_error("chdStore: \"%s\" already in use", name);
        delete [] name;
        return (false);
    }
    chd->setStored(true);
    CD()->ifChdDbChange();
    return (true);
}


// Recall a CHD by name, if remove is true the digest is also
// removed from the table.
//
cCHD *
cCDchdDB::chdRecall(const char *name, bool remove)
{
    if (!chd_table)
        return (0);
    cCHD *chd = (cCHD*)SymTab::get(chd_table, name);
    if (chd == (cCHD*)ST_NIL)
        return (0);
    if (remove) {
        chd_table->remove(name);
        chd->setStored(false);
        CD()->ifChdDbChange();
    }
    return (chd);
}


// Given a CHD pointer, return its database name, if found.
//
const char *
cCDchdDB::chdFind(const cCHD *chd)
{
    SymTabGen gen(chd_table);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        if (chd == (cCHD*)h->stData)
            return (h->stTag);
    }
    return (0);
}


// Hunt for an existing CHD that would match prp.
//
const char *
cCDchdDB::chdFind(const sChdPrp *prp)
{
    SymTabGen gen(chd_table);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        cCHD *chd = (cCHD*)h->stData;
        if (chd->match(prp))
            return (h->stTag);
    }
    return (0);
}


// Hunt for an existing CHD with matching parameters:  full pathname,
// scale factor, and aliasing.  If with_bb, the chd must have the bb
// and srf flags set.  The file must exist.  The return value is the
// database name, or null if not found.
//
const char *
cCDchdDB::chdFind(const char *path, cv_alias_info *ainfo)
{
    char *realname;
    FILE *fp = FIO()->POpen(path, "r", &realname);
    if (!fp)
        return (0);
    fclose(fp);
    GCarray<char*> gc_realname(realname);

    SymTabGen gen(chd_table);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        cCHD *chd = (cCHD*)h->stData;
        if (strcmp(realname, chd->filename()))
            continue;
        if (*ainfo == *chd->aliasInfo())
            return (h->stTag);
    }
    return (0);
}


// Return a list of saved digest names.
//
stringlist *
cCDchdDB::chdList()
{
    if (!chd_table)
        return (0);
    return (SymTab::names(chd_table));
}


// Clear the table and delete all saved digests.
//
void
cCDchdDB::chdClear()
{
    SymTabGen gen(chd_table, true);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        delete [] h->stTag;
        delete (cCHD*)h->stData;
        delete h;
    }
    delete chd_table;
    chd_table = 0;
    CD()->ifChdDbChange();
}


// Create a new unique CHD database name token.
//
char *
cCDchdDB::newChdName()
{
    static int dbix = 1;
    char buf[64];
    char *s = lstring::stpcpy(buf, "CellHier");

    for (;;) {
        mmItoA(s, dbix);
        if (!chdRecall(buf, false))
            return (lstring::copy(buf));
        dbix++;
    }
}


// The Cell Geometry Digest (CGD) geometry database.  This saves
// highly compressed geometry data per cell/per layer.  Separate
// databases are available by name.

cCDcgdDB *cCDcgdDB::instancePtr = 0;

cCDcgdDB::cCDcgdDB()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cCDcgdDB already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    cgd_table = 0;
}


// Private static error exit.
//
void
cCDcgdDB::on_null_ptr()
{
    fprintf(stderr, "Singleton class cCDcgdDB used before instantiated.\n");
    exit(1);
}


// Store a cCGD under name in a symbol table.  It is an error if
// the name is already in use and has nonzero ref count, otherwise
// any existing CGD under name is freed.
//
bool
cCDcgdDB::cgdStore(const char *name, cCGD *cgd)
{
    if (!cgd) {
        Errs()->add_error("cgdStore: null database pointer.");
        return (false);
    }
    if (!name || !*name) {
        Errs()->add_error("cgdStore: null or empty access name.");
        return (false);
    }
    if (cgd->id_name()) {
        Errs()->add_error("cgdStore: database already in storage");
        return (false);
    }
    if (!cgd_table)
        cgd_table = new SymTab(true, false);
    cCGD *cgdold = (cCGD*)SymTab::get(cgd_table, name);
    if (cgdold != (cCGD*)ST_NIL) {
        if (cgdold->refcnt()) {
            Errs()->add_error(
                "cgdStore: existing database %s is in use.", name);
            return (false);
        }
        cgdold->set_id_name(0);
        cgd_table->remove(name);
        delete cgdold;
    }

    cgd->set_id_name(lstring::copy(name));
    cgd_table->add(cgd->id_name(), cgd, false);
    CD()->ifCgdDbChange();
    return (true);
}


// Recall a cCGD by name, if remove is true the database is also
// removed from the table.
//
cCGD *
cCDcgdDB::cgdRecall(const char *name, bool remove)
{
    if (!cgd_table)
        return (0);
    cCGD *cgd = (cCGD*)SymTab::get(cgd_table, name);
    if (cgd == (cCGD*)ST_NIL)
        return (0);
    if (remove) {
        cgd->set_id_name(0);
        cgd_table->remove(name);
        CD()->ifCgdDbChange();
    }
    return (cgd);
}


// Return a list of saved CGD database names.
//
stringlist *
cCDcgdDB::cgdList()
{
    if (!cgd_table)
        return (0);
    return (SymTab::names(cgd_table));
}


// Clear the table and delete all saved databases with zero ref
// count.  This should be called after ChdClear to make sure that
// ref counts of linked CGDs are zero.
//
void
cCDcgdDB::cgdClear()
{
    SymTabGen gen(cgd_table, true);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        cCGD *cgd = (cCGD*)h->stData;
        cgd->set_id_name(0);
        delete [] h->stTag;
        delete h;
        if (!cgd->refcnt())
            delete cgd;
    }
    delete cgd_table;
    cgd_table = 0;
    CD()->ifCgdDbChange();
}


// Create a new unique cCGD database name token.
//
char *
cCDcgdDB::newCgdName()
{
    static int dbix = 1;
    char buf[64];
    char *s = lstring::stpcpy(buf, "CellGeom");

    for (;;) {
        mmItoA(s, dbix);
        if (!cgdRecall(buf, false))
            return (lstring::copy(buf));
        dbix++;
    }
}

