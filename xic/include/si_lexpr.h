
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
 $Id: si_lexpr.h,v 5.26 2015/10/14 04:57:40 stevew Exp $
 *========================================================================*/

#ifndef SI_LEXPR_H
#define SI_LEXPR_H

#include "si_scrfunc.h"
#include "geo_zlist.h"


// In layer expressions, the layer tokens have the form
// layer[.stname][.cellname].  If there is no stname, and the cellname
// starts with the char below, then the rest of the cellname is taken
// as the name of a "special" database.
//
#define SI_DBCHAR '@'

struct sLspec;

// This supports a handle table for layer expressions.  The lspec passed
// to add() is already inited, so we keep a pointer to the source string
// in lspec.lname for fast comparison.  Since we need to save and swap
// the layer expression parse trees during context switching, this is
// separate from the main handle stuff.
//
struct SIlexp_list
{
    SIlexp_list()
        {
            ll_lspec = 0;
            ll_indx = 0;
            ll_size = 0;
        }

    ~SIlexp_list();

    int check(const char*);
    int add(const sLspec&);
    const sLspec *find(int);

private:
    sLspec *ll_lspec;
    int ll_indx;
    int ll_size;
};

struct grd_t;


// This stores some context for geometric operations. An instantiation
// is passed to the top-level evaluation function.
//
struct SIlexprCx
{
    SIlexprCx()
        {
            cx_sdesc = 0;
            cx_db_name = 0;
            cx_refZlist = 0;
            cx_zlSaved = 0;
            cx_gridCx = 0;
            cx_depth = CDMAXCALLDEPTH;
            cx_throw = false;
            cx_verbose = false;
            cx_source_sq = false;
        }

    SIlexprCx(const CDs *sdesc, int depth, const Zlist *zl)
        {
            cx_sdesc = sdesc;
            cx_db_name = 0;
            cx_refZlist = zl;
            cx_zlSaved = 0;
            cx_gridCx = 0;
            cx_depth = depth;
            cx_throw = false;
            cx_verbose = false;
            cx_source_sq = false;
        }

    SIlexprCx(const CDs *sdesc, int depth, const BBox *bb)
        {
            cx_sdesc = sdesc;
            cx_db_name = 0;
            cx_refZlist = 0;
            cx_zlSaved = 0;
            cx_gridCx = 0;
            cx_depth = depth;
            cx_throw = false;
            cx_verbose = false;
            cx_source_sq = false;
            setZref(bb);
        }

    SIlexprCx(const CDs *sdesc, int depth, const Zoid *z)
        {
            cx_sdesc = sdesc;
            cx_db_name = 0;
            cx_refZlist = 0;
            cx_zlSaved = 0;
            cx_gridCx = 0;
            cx_depth = depth;
            cx_throw = false;
            cx_verbose = false;
            cx_source_sq = false;
            setZref(z);
        }

    SIlexprCx(const char *dbname, const Zlist *zl)
        {
            cx_sdesc = 0;
            cx_db_name = lstring::copy(dbname);
            cx_refZlist = zl;
            cx_zlSaved = 0;
            cx_gridCx = 0;
            cx_depth = 0;
            cx_throw = false;
            cx_verbose = false;
            cx_source_sq = false;
        }

    ~SIlexprCx();

    const Zlist *getZref();

    // Source must exist as long as needed!
    void setZref(const Zlist *zr)     { cx_refZlist = zr; }

    void setZref(const BBox *bb)
        {
            if (bb) {
                cx_defZref.Z.set(bb);
                cx_refZlist = &cx_defZref;
            }
        }

    void setZref(const Zoid *z)
        {
            if (z) {
                cx_defZref.Z = *z;
                cx_refZlist = &cx_defZref;
            }
        }

    // Take ownership of zl.
    void setZrefSaved(Zlist *zl)
        {
            cx_zlSaved->free();
            cx_zlSaved = zl;
            cx_refZlist = cx_zlSaved;
        }

    XIrt getZlist(const LDorig*, Zlist**);
    XIrt getZlist(const CDl*, Zlist**);
    XIrt getDbZlist(const CDl*, const char*, Zlist**);

    void reset();
    void clearGridCx();
    void handleInterrupt() const;

    const CDs *celldesc()       const { return (cx_sdesc); }
    const Zlist *refZlist()     const { return (cx_refZlist); }
    grd_t *gridCx()             const { return (cx_gridCx); }
    void setGridCx(grd_t *gcx)        { cx_gridCx = gcx; }

    int hierDepth()             const { return (cx_depth); }
    void setHierDepth(int d)
        {
            if (d < 0)
                d = 0;
            else if (d > CDMAXCALLDEPTH)
                d = CDMAXCALLDEPTH;
            cx_depth = d;
        }

    void enableExceptions(bool b)     { cx_throw = b; }
    bool verbose()              const { return (cx_verbose); }
    bool setSourceSelQueue(bool b)
        {
            bool r = cx_source_sq;
            cx_source_sq = b;
            return (r);
        }

private:
    const CDs *cx_sdesc;        // If given, the root cell to use.  If not
                                // given and db_name is null, the current
                                // physical cell will be used.  This will
                                // be ignored if db_name is not null.

    const char *cx_db_name;     // Name of database to use as a geometry
                                // source, instead of cell.

    const Zlist *cx_refZlist;   // The clipping area for operations.  If
                                // not set, the source BB is used.

    Zlist *cx_zlSaved;          // A refZlist copy to be freed in
                                // destructor.

    Zlist cx_defZref;           // Default layer expr reference area.

    grd_t *cx_gridCx;           // Cell area partitioning context.

    int cx_depth;               // Hierarchy depth under sdesc to consider
                                // if using a cell.

    bool cx_throw;              // True if try/catch in stack.

    bool cx_verbose;            // Print messages along the way.

    bool cx_source_sq;          // Source the selections list.
};


namespace zlist_funcs {
    // Zlist evaluation functions (funcs_math.cc)
    bool PTminusZ(Variable*, Variable*, void*);
    bool PTandZ(Variable*, Variable*, void*);
    bool PTorZ(Variable*, Variable*, void*);
    bool PTxorZ(Variable*, Variable*, void*);
    bool PTnotZ(Variable*, Variable*, void*);

    // Need to export the sqz() "pseudo" function implementation back
    // to the evaluator.
    extern SIscriptFunc SIlexp_sqz_func;

    extern SIscriptFunc PTbloatZ;
    extern SIscriptFunc PTedgesZ;
}

#endif

