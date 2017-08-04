
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

#include "cd.h"
#include "cd_types.h"
#include "cd_sdb.h"
#include "cd_celldb.h"
#include "geo_grid.h"
#include "si_parsenode.h"
#include "si_lexpr.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_lspec.h"


//
// Structures and functions for evaluating "layer expressions".  These
// are parse trees containing a limited number of data types, used for
// geometrical manipulation and testing.
//


SIlexprCx::~SIlexprCx()
{
    delete cx_gridCx;
    delete [] cx_db_name;
    Zlist::destroy(cx_zlSaved);
}


// Return the current reference Zlist.
//
const Zlist *
SIlexprCx::getZref()
{
    const Zlist *zr = cx_refZlist;
    if (!zr) {
        if (cx_db_name) {
            cSDB *sdb = CDsdb()->findDB(cx_db_name);
            if (sdb)
                cx_defZref = Zlist(sdb->BB());
            else
                cx_defZref = Zlist(&CDinfiniteBB);
        }
        else {
            const CDs *sd = cx_sdesc;
            if (!sd)
                sd = SIparse()->ifGetCurPhysCell();
            if (sd)
                cx_defZref = Zlist(sd->BB());
            else
                cx_defZref = Zlist(&CDinfiniteBB);
        }
        zr = &cx_defZref;
    }
    return (zr);
}


// Evaluation function for a layer name and providing an originating
// cell name and symbol table name.
//
XIrt
SIlexprCx::getZlist(const LDorig *ldorig, Zlist **zret)
{
    *zret = 0;
    if (!ldorig || !ldorig->ldesc())
        return (XIbad);

    if (ldorig->cellname() || ldorig->stab_name()) {
        if (!ldorig->stab_name() && ldorig->cellname() &&
                *Tstring(ldorig->cellname()) == SI_DBCHAR) {
            // This triggers use of a named database.

            const CDs *tsd = cx_sdesc;
            cx_sdesc = 0;
            const char *tn = cx_db_name;
            cx_db_name = Tstring(ldorig->cellname()) + 1;
            XIrt ret = getDbZlist(ldorig->ldesc(), ldorig->origname(), zret);
            cx_db_name = tn;
            cx_sdesc = tsd;
            return (ret);
        }
        CDs *targ = SIparse()->openReference(Tstring(ldorig->cellname()),
            Tstring(ldorig->stab_name()));
        if (!targ)
            return (XIbad);
        if (cx_verbose)
            SIparse()->ifSendMessage("Zoidifying %s ...", ldorig->origname());
        if (SIparse()->ifCheckInterrupt())
            return (XIintr);

        XIrt ret;
        *zret = targ->getZlist(cx_depth, ldorig->ldesc(), getZref(), &ret);
        return (ret);
    }
    return (getZlist(ldorig->ldesc(), zret));
}


XIrt
SIlexprCx::getZlist(const CDl *ld, Zlist **zret)
{
    if (!ld)
        return (XIbad);

    if (cx_db_name)
        return (getDbZlist(ld, ld->name(), zret));
    const CDs *targ = cx_sdesc ? cx_sdesc : SIparse()->ifGetCurPhysCell();
    if (!targ)
        return (XIbad);
    if (cx_verbose)
        SIparse()->ifSendMessage("Zoidifying %s ...", ld->name());
    if (SIparse()->ifCheckInterrupt())
        return (XIintr);

    XIrt ret;
    if (cx_source_sq && targ == SIparse()->ifGetCurPhysCell() &&
            SIparse()->ifGetCurMode() == Physical) {
        *zret = SIparse()->ifGetSqZlist(ld, getZref());
        ret = XIok;
    }
    else
        *zret = targ->getZlist(cx_depth, ld, getZref(), &ret);
    return (ret);
}


// Function to extract the zoids from a named database, used by
// getZlist.
//
XIrt
SIlexprCx::getDbZlist(const CDl *ld, const char *name, Zlist **zret)
{
    *zret = 0;
    if (!cx_db_name || !ld)
        return (XIbad);
    cSDB *sdb = CDsdb()->findDB(cx_db_name);
    if (!sdb)
        return (XIbad);
    if (sdb->type() == sdbBdb) {
        bdb_t *db = (bdb_t*)SymTab::get(sdb->table(), (unsigned long)ld);
        if (db != (bdb_t*)ST_NIL) {
            if (cx_verbose)
                SIparse()->ifSendMessage("Zoidifying %s ...", name);
            if (SIparse()->ifCheckInterrupt())
                return (XIintr);
            XIrt ret;
            *zret = db->getZlist(getZref(), &ret);
            return (ret);
        }
        return (XIok);
    }
    if (sdb->type() == sdbOdb) {
        odb_t *db = (odb_t*)SymTab::get(sdb->table(), (unsigned long)ld);
        if (db != (odb_t*)ST_NIL) {
            if (cx_verbose)
                SIparse()->ifSendMessage("Zoidifying %s ...", name);
            if (SIparse()->ifCheckInterrupt())
                return (XIintr);
            XIrt ret;
            *zret = db->getZlist(getZref(), &ret);
            return (ret);
        }
        return (XIok);
    }
    if (sdb->type() == sdbZdb) {
        zdb_t *db = (zdb_t*)SymTab::get(sdb->table(), (unsigned long)ld);
        if (db != (zdb_t*)ST_NIL) {
            if (cx_verbose)
                SIparse()->ifSendMessage("Zoidifying %s ...", name);
            if (SIparse()->ifCheckInterrupt())
                return (XIintr);
            XIrt ret;
            *zret = db->getZlist(getZref(), &ret);
            return (ret);
        }
        return (XIok);
    }
    if (sdb->type() == sdbZldb) {
        Zlist *db = (Zlist*)SymTab::get(sdb->table(), (unsigned long)ld);
        if (db != (Zlist*)ST_NIL) {
            if (cx_verbose)
                SIparse()->ifSendMessage("Zoidifying %s ...", name);
            if (SIparse()->ifCheckInterrupt())
                return (XIintr);

            Zlist *zl = Zlist::copy(db);
            XIrt ret = Zlist::zl_and(&zl, Zlist::copy(getZref()));
            if (ret == XIok)
                *zret = zl;
            return (ret);
        }
        return (XIok);
    }
    if (sdb->type() == sdbZbdb) {
        zbins_t *db = (zbins_t*)SymTab::get(sdb->table(), (unsigned long)ld);
        if (db != (zbins_t*)ST_NIL) {
            if (cx_verbose)
                SIparse()->ifSendMessage("Zoidifying %s ...", name);
            if (SIparse()->ifCheckInterrupt())
                return (XIintr);
            XIrt ret;
            *zret = db->getZlist(getZref(), &ret);
            return (ret);
        }
        return (XIok);
    }
    return (XIbad);
}


void
SIlexprCx::reset()
{
    cx_sdesc = 0;
    delete [] cx_db_name;
    cx_db_name = 0;
    cx_refZlist = 0;
    delete cx_gridCx;
    cx_gridCx = 0;
    cx_depth = CDMAXCALLDEPTH;
    cx_throw = false;
    cx_verbose = false;
    cx_source_sq = false;
}


void
SIlexprCx::clearGridCx()
{
    delete cx_gridCx;
    cx_gridCx = 0;
    cx_refZlist = 0;
}


void
SIlexprCx::handleInterrupt() const
{
    if (cx_throw)
        throw XIintr;
    else
        SI()->Halt();
}
// End of SIlexprCx functions.


LDorig::LDorig(const char *name)
{
    char *cname, *stname;
    char *lname = SIparse()->parseLayer(name, &cname, &stname);
    ldo_cellname = CD()->CellNameTableAdd(cname);
    delete [] cname;
    ldo_stname = CD()->CellNameTableAdd(stname);
    delete [] stname;
    ldo_ldesc = CDldb()->findLayer(lname, Physical);
    if (!ldo_ldesc)
        ldo_ldesc = CDldb()->findDerivedLayer(lname);
    delete [] lname;
    ldo_origname = lstring::copy(name);
}
// End of LDorig functions.


//----------------------------------------------------------------------------
// SIparser utilities

// Note:  the sLspec struct is used primarily by DRC, but is also used
// as a container in createLayer().  In the latter use, the layer name
// field can have the form "lname[.stname][.cellname]" to indicate
// that the source of the geometry is cellname.  This feature is not
// used in DRC, but must be supported in the functions used in
// CreateLayer().

#define L_SEPC '.'
#define L_QUOT '"'

// Parse the "layer name", which can be in the form
//   lname[.stname][.cname]
// Any of lname, stname, cname can be double-quoted, which must be true
// if the token contains the separation char '.'.  If there are two
// fields, the second field is cname.  All returns are dynamic storage,
// the cname return is null if the token would be empty, the stname
// return can be empty.
//
char *
SIparser::parseLayer(const char *name, char **cname, char **stname)
{
    if (cname)
        *cname = 0;
    if (stname)
        *stname = 0;

    // Note that embedded \" quotes are not recognized here.
    const char *sep1 = 0, *sep2 = 0;
    bool inq = false;
    for (const char *t = name; *t; t++) {
        if (*t == L_QUOT) {
            inq = !inq;
            continue;
        }
        if (inq)
            continue;
        if (*t == L_SEPC && t != name) {
            if (!sep1)
                sep1 = t;
            else {
                sep2 = t;
                break;
            }
        }
    }

    char *lname;
    if (!sep1) {
        const char *e = name + strlen(name) - 1;
        if (*name == L_QUOT && *e == L_QUOT && name != e) {
            name++;
            lname = new char[e - name + 1];
            strncpy(lname, name, e - name);
            lname[e - name] = 0;
        }
        else
            lname = lstring::copy(name);
        return (lname);
    }

    if (*name == L_QUOT && *(sep1-1) == L_QUOT && name != sep1-1) {
        name++;
        lname = new char[sep1 - name];
        strncpy(lname, name, sep1 - name - 1);
        lname[sep1 - name - 1] = 0;
    }
    else {
        lname = new char[sep1 - name + 1];
        strncpy(lname, name, sep1 - name);
        lname[sep1 - name] = 0;
    }

    if (!sep2) {
        if (cname) {
            char *cn = 0;
            sep1++;
            const char *e = sep1 + strlen(sep1) - 1;
            if (*sep1 == L_QUOT && *e == L_QUOT && sep1 != e) {
                sep1++;
                cn = new char[e - sep1 + 1];
                strncpy(cn, sep1, e - sep1);
                cn[e - sep1] = 0;
            }
            else if (*sep1)
                cn = lstring::copy(sep1);
            if (cn && *cn)
                *cname = cn;
            else
                delete [] cn;
        }
        return (lname);
    }

    if (stname) {
        char *sn = 0;
        sep1++;
        if (*sep1 == L_QUOT && *(sep2-1) == L_QUOT && sep1 != sep2-1) {
            sep1++;
            sn = new char[sep2 - sep1];
            strncpy(sn, sep1, sep2 - sep1 - 1);
            sn[sep2 - sep1 - 1] = 0;
        }
        else {
            sn = new char[sep2 - sep1 + 1];
            strncpy(sn, sep1, sep2 - sep1);
            sn[sep2 - sep1] = 0;
        }
        *stname = sn;
    }

    if (cname) {
        char *cn = 0;
        sep2++;
        const char *e = sep2 + strlen(sep2) - 1;
        if (*sep2 == L_QUOT && *e == L_QUOT && sep2 != e) {
            sep2++;
            cn = new char[e - sep2 + 1];
            strncpy(cn, sep2, e - sep2);
            cn[e - sep2] = 0;
        }
        else if (*sep2)
            cn = lstring::copy(sep2);
        if (cn && *cn)
            *cname = cn;
        else
            delete [] cn;
    }
    return (lname);
}


// Open the cname cell in the symbol table stname.  If cname is null
// or empty, the current cell name is assumed.  If stname is null, the
// current symbol table is assumed.  If stname is empty, the "main"
// table will be assumed.
//
CDs *
SIparser::openReference(const char *cname, const char *stname)
{
    if (!CDcdb()->findTable(stname))
        return (0);

    if (!cname || !*cname)
        cname = ifGetCurCellName();

    const char *stbak = CDcdb()->tableName();
    if (stname)
        CDcdb()->switchTable(stname);
    CDcbin cbin;
    OItype oiret = CD()->OpenExisting(cname, &cbin);
    if (stname)
        CDcdb()->switchTable(stbak);
    if (OIfailed(oiret))
        return (0);
    return (cbin.phys());
}
// End of SIparser functions.


SIlexp_list::~SIlexp_list()
{
    if (ll_lspec) {
        // must zero the lname first
        for (int i = 0; i < ll_indx; i++)
            ll_lspec[i].set_lname_pointer(0);
        delete [] ll_lspec;
    }
}


int
SIlexp_list::check(const char *e)
{
    {
        SIlexp_list *llt = this;
        if (!llt)
            return (0);
    }
    if (!ll_lspec)
        return (0);
    for (int i = 0; i < ll_indx; i++)
        if (ll_lspec[i].lname() == e)
            return (i+1);
    return (0);
}


#define LL_INCR 64

int
SIlexp_list::add(const sLspec &l)
{
    ll_indx++;
    if (ll_indx >= ll_size) {
        sLspec *lold = ll_lspec;
        ll_lspec = new sLspec[ll_size + LL_INCR];
        memcpy(ll_lspec, lold, ll_size * sizeof(sLspec));
        memset(lold, 0, ll_size * sizeof(sLspec));
        delete [] lold;
        ll_size += LL_INCR;
    }
    ll_lspec[ll_indx-1] = l;
    return (ll_indx);
}


const sLspec *
SIlexp_list::find(int i)
{
    {
        SIlexp_list *llt = this;
        if (!llt)
            return (0);
    }
    if (i <= 0 || i > ll_indx)
        return (0);
    return (ll_lspec + i - 1);
}

