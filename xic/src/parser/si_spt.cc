
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
 $Id: si_spt.cc,v 5.23 2015/08/29 15:40:00 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "fio.h"
#include "si_spt.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "lstring.h"
#include "filestat.h"

#include <ctype.h>

//
// A "spatial parameter table" is a 2-d array of floating point
// values, which can be accessed via x/y coordinate pairs.  The user
// can define any number of such tables, each of which is given a
// unique identifying keyword.
//

namespace {
    // Hash table for spatial parameter table storage.
    SymTab *spt_tab;

    // Hash table for string table storage.
    SymTab *name_tab;
}


// The tables are input through a file, which uses the following format:
//
// keyword X DX NX Y DY NY
// X Y value
// ...
//
// Blank lines and lines that begin with punctuation are ignored.
// There is one "header" line with the following entries:
//   keyword:       Arbitrary word for identification.  An existing
//                  database with the same identifier will be replaced.
//   X              Reference coordinate in microns.
//   DX             Grid spacing in X direction, in microns, must be > 0.
//   NX             Number of grid cells in X direction, must be > 0.
//   Y              Reference coordinate in microns.
//   DY             Grid spacing in Y direction, in microns, must be > 0.
//   NY             Number of grid cells in Y direction, must be > 0.
//
// The header line is followed by data lines that supply a value to
// the cells.  The X,Y given in microns specifies the cell.  A second
// access to a cell will simply overwrite the data value for that
// cell.  Unwritten cells will have a zero value.


// Static function
//
// Swap the table of SP tables.
//
SymTab *
spt_t::swapSPtableCx(SymTab *t)
{
    SymTab *old = spt_tab;
    spt_tab = t;
    return (old);
}


// Static function.
// Parse the indicated file, and save the resulting table.  On failure,
// false is returned, with an error message in Errs.
//
bool
spt_t::readSpatialParameterTable(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        Errs()->add_error(
            "Can't open spatial parameter file %s for reading", filename);
        return (false);
    }

    int lcnt = 0;
    spt_t *f = 0;
    bool header_read = false;
    char *s, buf[256];
    while ((s = fgets(buf, 256, fp)) != 0) {
        lcnt++;
        while (isspace(*s))
            s++;
        if (!*s || ispunct(*s))
            continue;
        if (!header_read) {
            char *keyword = lstring::gettok(&s);
            int nx, ny;
            double x, y;
            double dx, dy;
            int n = sscanf(s, "%lf %lf %d %lf %lf %d", &x, &dx, &nx,
                &y, &dy, &ny);
            if (!keyword || n != 6) {
                Errs()->add_error(
                    "Syntax error in spatial parameter file line %d",
                    lcnt);
                delete [] keyword;
                fclose(fp);
                return (false);
            }
            int idx = INTERNAL_UNITS(dx);
            int idy = INTERNAL_UNITS(dy);
            if (idx <= 0 || idy <= 0 || nx <= 0 || nx <= 0) {
                Errs()->add_error(
        "Bad data, value zero or negative in spatial parameter file line %d",
                    lcnt);
                delete [] keyword;
                fclose(fp);
                return (false);
            }

            f = new spt_t(keyword, INTERNAL_UNITS(x), idx, nx,
                INTERNAL_UNITS(y), idy, ny);
            header_read = true;
            continue;
        }
        double x, y;
        float val;
        int n = sscanf(s, "%lf %lf %f", &x, &y, &val);
        if (n != 3) {
            Errs()->add_error(
                "Syntax error in spatial parameter file line %d", lcnt);
            delete f;
            fclose(fp);
            return (false);
        }
        if (!f->save_item(INTERNAL_UNITS(x), INTERNAL_UNITS(y), val)) {
            Errs()->add_error(
                "Data error in spatial parameter file line %d", lcnt);
            delete f;
            fclose(fp);
            return (false);
        }
    }
    fclose(fp);

    if (!f)
        return (false);
    if (!spt_tab)
        spt_tab = new SymTab(false, false);
    SymTabEnt *h = spt_tab->get_ent(f->keyword());
    if (h) {
        delete (spt_t*)h->stData;
        h->stData = f;
        h->stTag = f->keyword();
    }
    else
        spt_tab->add(f->keyword(), f, false);
    return (true);
}


// Static function.
// Create a new empty table.
//
bool
spt_t::newSpatialParameterTable(const char *name, double x0, double dx,
    int nx, double y0, double dy, int ny)
{
    if (!name || !*name) {
        Errs()->add_error("null or empty table name.");
        return (false);
    }
    if (dx <= 0.0) {
        Errs()->add_error("zero or negative DX.");
        return (false);
    }
    if (dy <= 0.0) {
        Errs()->add_error("zero or negative DY.");
        return (false);
    }
    if (nx <= 0) {
        Errs()->add_error("zero or negative NX.");
        return (false);
    }
    if (ny <= 0) {
        Errs()->add_error("zero or negative NY.");
        return (false);
    }

    spt_t *f = new spt_t(lstring::copy(name), INTERNAL_UNITS(x0),
        INTERNAL_UNITS(dx), nx, INTERNAL_UNITS(y0), INTERNAL_UNITS(dy), ny);

    if (!spt_tab)
        spt_tab = new SymTab(false, false);
    SymTabEnt *h = spt_tab->get_ent(name);
    if (h) {
        delete (spt_t*)h->stData;
        h->stData = f;
        h->stTag = f->keyword();
    }
    else
        spt_tab->add(f->keyword(), f, false);
    return (true);
}


// Static function.
// Write the namef table to a file, return true on success.
//
bool
spt_t::writeSpatialParameterTable(const char *name, const char *fname)
{
    if (!spt_tab) {
        Errs()->add_error("no spatial parameter tables in memory.");
        return (false);
    }
    if (!name || !*name) {
        Errs()->add_error("null or empty table name.");
        return (false);
    }
    if (!fname || !*fname) {
        Errs()->add_error("null or empty file name.");
        return (false);
    }
    spt_t *f = (spt_t*)spt_tab->get(name);
    if (f == (spt_t*)ST_NIL) {
        Errs()->add_error("table %s not found.", name);
        return (false);
    }
    if (!filestat::create_bak(fname, 0)) {
        Errs()->add_error(filestat::error_msg());
        return (false);
    }
    FILE *fp = large_fopen(fname, "w");
    if (!fp) {
        Errs()->add_error("failed to open %s for writing.", fname);
        return (false);
    }
    if (f->matrix) {
        int ndgt = CD()->numDigits();
        double x0 = MICRONS(f->org_x);
        double y0 = MICRONS(f->org_y);
        double dx = MICRONS(f->del_x);
        double dy = MICRONS(f->del_y);
        if (fprintf(fp, "%s %.*f %.*f %d %.*f %.*f %d\n",
                name, ndgt, x0, ndgt, dx, f->num_x,
                ndgt, y0, ndgt, dy, f->num_y) < 0) {
            fclose(fp);
            Errs()->add_error("error writing file.");
            return (false);
        }
        for (unsigned int i = 0; i < f->num_y; i++) {
            for (unsigned int j = 0; j < f->num_x; j++) {
                float val = f->matrix[i*f->num_x + j];
                if (val != 0.0) {
                    double x = x0 + (j + 0.5)*dx;
                    double y = y0 + (i + 0.5)*dy;
                    if (fprintf(fp, "%.*f %.*f %.6e\n", ndgt, x, ndgt, y,
                            val) < 0) {
                        fclose(fp);
                        Errs()->add_error("error writing file.");
                        return (false);
                    }
                }
            }
        }
    }
    fclose (fp);
    return (true);
}


// Static function.
// Destroy the table corresponding to the passed keyword, in not null.
// If null is passed, all tables will be destroyed.  The return value
// is the number of tables destroyed.
//
int
spt_t::clearSpatialParameterTable(const char *name)
{
    if (spt_tab) {
        if (name) {
            spt_t *f = (spt_t*)spt_tab->get(name);
            if (f != (spt_t*)ST_NIL) {
                spt_tab->remove(name);
                delete f;
                return (1);
            }
        }
        else {
            int cnt = 0;
            SymTabGen gen(spt_tab, true);
            SymTabEnt *h;
            while ((h = gen.next()) != 0) {
                delete (spt_t*)h->stData;
                delete h;
                cnt++;
            }
            delete spt_tab;
            spt_tab = 0;
            return (cnt);
        }
    }
    return (0);
}


// Static function.
// Return the table corresponding to the name passed.  The table
// methods can be used to extract or modify the data.
//
spt_t *
spt_t::findSpatialParameterTable(const char *name)
{
    if (!spt_tab || !name)
        return (0);
    spt_t *f = (spt_t*)spt_tab->get(name);
    if (f == (spt_t*)ST_NIL)
        return (0);
    return (f);
}
// End of spt_t functions.


// Swap the nametab context.
//
SymTab*
nametab::swapNametabCx(SymTab *cx)
{
    SymTab *ocx = name_tab;
    name_tab = cx;
    return (ocx);
}


// Find the nametab for tabname.  If create is true, create it if it
// doesn't exist.
//
SymTab *
nametab::findNametab(const char *tabname, bool create)
{
    if (!tabname || !*tabname)
        return (0);
    if (!name_tab) {
        if (!create)
            return (0);
        name_tab = new SymTab(true, false);
    }
    SymTab *st = (SymTab*)name_tab->get(tabname);
    if (st == (SymTab*)ST_NIL) {
        if (create) {
            st = new SymTab(true, false);
            name_tab->add(lstring::copy(tabname), st, false);
            return (st);
        }
        return (0);
    }
    return (st);
}


// Remove and destroy the nametab tabname.
//
bool
nametab::removeNametab(const char *tabname)
{
    SymTabEnt *h = name_tab->get_ent(tabname);
    if (h) {
        delete (SymTab*)h->stData;
        h->stData = 0;
        name_tab->remove(tabname);
        return (true);
    }
    return (false);
}


// Return a list of names of saved nametabs.
//
stringlist *
nametab::listNametabs()
{
    return (name_tab->names());
}


// Clear and destroy all nametabs.
//
void
nametab::clearNametabs()
{
    SymTabGen gen(name_tab, true);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        delete (SymTab*)h->stData;
        h->stData = 0;
        delete [] h->stTag;
        h->stTag = 0;
        delete h;
    }
    delete name_tab;
    name_tab = 0;
}

