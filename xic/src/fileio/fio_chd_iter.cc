
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

#include "fio.h"
#include "fio_chd_iter.h"
#include "fio_chd.h"
#include "cd_sdb.h"
#include "cd_compare.h"
#include "cd_lgen.h"
#include "cd_digest.h"
#include "cd_chkintr.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_lexpr.h"
#include "si_spt.h"


//
// Facility for iterating over a grid, where a ZBDB database is created,
// and iteration over the fine grid of the ZBDB.
//

// Access name for ZBDB.
#define DBNAME "_IterZBDB_"

XIrt
cv_chd_iter::run(cCHD *chd, const char *cellname, const BBox *AOI)
{
    if (!chd) {
        Errs()->add_error("run: null CHD pointer given.");
        return (XIbad);
    }
    if (ci_coarse_grid <= 0) {
        Errs()->add_error("run: non-positive coarse grid spacing.");
        return (XIbad);
    }
    if (ci_fine_grid <= 0) {
        Errs()->add_error("run: non-positive fine grid spacing.");
        return (XIbad);
    }
    if (ci_bloat_val < 0) {
        Errs()->add_error("run: negative bloat value.");
        return (XIbad);
    }

    ci_chd1 = chd;
    ci_symref1 = ci_chd1->findSymref(cellname, Physical, true);
    if (!ci_symref1) {
        Errs()->add_error("run: unresolved top cell.");
        return (XIbad);
    }
    ci_chd1->setBoundaries(ci_symref1);

    ci_aoiBB = AOI ? *AOI : *ci_symref1->get_bb();

    int nxc = ci_aoiBB.width()/ci_coarse_grid +
       (ci_aoiBB.width()%ci_coarse_grid != 0);
    int nyc = ci_aoiBB.height()/ci_coarse_grid +
       (ci_aoiBB.height()%ci_coarse_grid != 0);
    int nvals = nxc*nyc;

    XIrt ret = XIok;
    FIO()->ifSetWorking(true);
    for (int ic = 0; ic < nyc; ic++) {
        for (int jc = 0; jc < nxc; jc++) {
            int cur_cx = ci_aoiBB.left + jc*ci_coarse_grid;
            int cur_cy = ci_aoiBB.bottom + ic*ci_coarse_grid;
            BBox cBB(cur_cx, cur_cy,
                cur_cx + ci_coarse_grid, cur_cy + ci_coarse_grid);
            if (cBB.right > ci_aoiBB.right)
                cBB.right = ci_aoiBB.right;
            if (cBB.top > ci_aoiBB.top)
                cBB.top = ci_aoiBB.top;

            FIOcvtPrms prms;
            prms.set_allow_layer_mapping(true);
            prms.set_use_window(true);
            prms.set_window(&cBB);

            SymTab *tab = 0;  // must pass 0 to create new tab
            OItype oiret = ci_chd1->readFlat_zbdb(&tab, cellname, &prms,
                    ci_fine_grid, ci_fine_grid, ci_bloat_val,
                    ci_bloat_val);
            if (oiret != OIok) {
                if (oiret == OIaborted) {
                    ret = XIintr;
                    goto done;
                }
                Errs()->add_error("run: geometry read failed.");
                ret = XIbad;
                goto done;
            }

            ci_db1 = new cSDB(DBNAME, tab, sdbZbdb);
            if (!CDsdb()->saveDB(ci_db1)) {
                Errs()->add_error("run: failed to save ZBDB.");
                delete ci_db1;
                ci_db1 = 0;
                ret = XIbad;
                goto done;
            }

            ret = iterFunc(&cBB);
            CDsdb()->destroyDB(DBNAME);
            ci_db1 = 0;
            if (ret != XIok) {
                Errs()->add_error("run: iteration %s at %d,%d.",
                    ret == XIintr ? "aborted" : "failed", cur_cx, cur_cy);
                goto done;
            }
            if (ci_endit)
                goto done;
            FIO()->ifInfoMessage(IFMSG_INFO, "Completed %d of %d.",
                ic*nxc + jc + 1, nvals);

            if (checkInterrupt()) {
                ret = XIintr;
                goto done;
            }
        }
    }
done:
    FIO()->ifSetWorking(false);
    return (ret);
}


// Access names for ZBDB1,2.
#define DBNAME1 "_IterZBDB_1"
#define DBNAME2 "_IterZBDB_2"

// Similar to above, but iterate through two different CHDs in parallel,
// for comparison purposes.
//
XIrt
cv_chd_iter::run2(cCHD *chd1, const char *cname1, cCHD *chd2,
    const char *cname2,  const BBox *AOI)
{
    if (!chd1) {
        Errs()->add_error("run2: null CHD-1 pointer given.");
        return (XIbad);
    }
    if (!chd2) {
        Errs()->add_error("run2: null CHD-2 pointer given.");
        return (XIbad);
    }
    if (ci_coarse_grid <= 0) {
        Errs()->add_error("run2: non-positive coarse grid spacing.");
        return (XIbad);
    }
    if (ci_fine_grid <= 0) {
        Errs()->add_error("run2: non-positive fine grid spacing.");
        return (XIbad);
    }
    if (ci_bloat_val < 0) {
        Errs()->add_error("run2: negative bloat value.");
        return (XIbad);
    }

    ci_chd1 = chd1;
    ci_symref1 = ci_chd1->findSymref(cname1, Physical, true);
    if (!ci_symref1) {
        Errs()->add_error("run2: unresolved top-1 cell.");
        return (XIbad);
    }
    ci_chd1->setBoundaries(ci_symref1);

    ci_chd2 = chd2;
    ci_symref2 = ci_chd2->findSymref(cname2, Physical, true);
    if (!ci_symref2) {
        Errs()->add_error("run2: unresolved top-2 cell.");
        return (XIbad);
    }
    ci_chd2->setBoundaries(ci_symref2);

    if (AOI)
        ci_aoiBB = *AOI;
    else {
        ci_aoiBB = *ci_symref1->get_bb();
        ci_aoiBB.add(ci_symref2->get_bb());
    }

    int nxc = ci_aoiBB.width()/ci_coarse_grid +
       (ci_aoiBB.width()%ci_coarse_grid != 0);
    int nyc = ci_aoiBB.height()/ci_coarse_grid +
       (ci_aoiBB.height()%ci_coarse_grid != 0);
    int nvals = nxc*nyc;

    XIrt ret = XIok;
    FIO()->ifSetWorking(true);
    for (int ic = 0; ic < nyc; ic++) {
        for (int jc = 0; jc < nxc; jc++) {
            int cur_cx = ci_aoiBB.left + jc*ci_coarse_grid;
            int cur_cy = ci_aoiBB.bottom + ic*ci_coarse_grid;
            BBox cBB(cur_cx, cur_cy,
                cur_cx + ci_coarse_grid, cur_cy + ci_coarse_grid);
            if (cBB.right > ci_aoiBB.right)
                cBB.right = ci_aoiBB.right;
            if (cBB.top > ci_aoiBB.top)
                cBB.top = ci_aoiBB.top;

            FIOcvtPrms prms;
            prms.set_allow_layer_mapping(true);
            prms.set_use_window(true);
            prms.set_window(&cBB);

            SymTab *tab = 0;  // must pass 0 to create new tab
            OItype oiret = ci_chd1->readFlat_zbdb(&tab, cname1, &prms,
                ci_fine_grid, ci_fine_grid, ci_bloat_val, ci_bloat_val);
            if (oiret != OIok) {
                if (oiret == OIaborted)
                    ret = XIintr;
                else {
                    Errs()->add_error("run: geometry-1 read failed.");
                    ret = XIbad;
                }
                goto done;
            }
            ci_db1 = new cSDB(DBNAME1, tab, sdbZbdb);
            if (!CDsdb()->saveDB(ci_db1)) {
                Errs()->add_error("run2: failed to save ZBDB-1.");
                delete ci_db1;
                ci_db1 = 0;
                ret = XIbad;
                goto done;
            }

            tab = 0;  // must pass 0 to create new tab
            oiret = ci_chd2->readFlat_zbdb(&tab, cname2, &prms,
                ci_fine_grid, ci_fine_grid, ci_bloat_val, ci_bloat_val);
            if (oiret != OIok) {
                if (oiret == OIaborted)
                    ret = XIintr;
                else {
                    Errs()->add_error("run: geometry-2 read failed.");
                    ret = XIbad;
                }
                CDsdb()->destroyDB(DBNAME1);
                ci_db1 = 0;
                goto done;
            }
            ci_db2 = new cSDB(DBNAME2, tab, sdbZbdb);
            if (!CDsdb()->saveDB(ci_db2)) {
                Errs()->add_error("run2: failed to save ZBDB-2.");
                delete ci_db2;
                ci_db2 = 0;
                CDsdb()->destroyDB(DBNAME1);
                ci_db1 = 0;
                ret = XIbad;
                goto done;
            }

            ret = iterFunc(&cBB);
            CDsdb()->destroyDB(DBNAME1);
            CDsdb()->destroyDB(DBNAME2);
            ci_db1 = 0;
            ci_db2 = 0;

            if (ret != XIok) {
                Errs()->add_error("run2: iteration %s at %d,%d.",
                    ret == XIintr ? "aborted" : "failed", cur_cx, cur_cy);
                goto done;
            }
            if (ci_endit)
                goto done;
            FIO()->ifInfoMessage(IFMSG_INFO, "Completed %d of %d.",
                ic*nxc + jc + 1, nvals);

            if (checkInterrupt()) {
                ret = XIintr;
                goto done;
            }
        }
    }
done:
    FIO()->ifSetWorking(false);
    return (ret);
}
// End of cv_chd_iter functions.


//-----------------------------------------------------------------------------
// Derivative to iterate over the regions, calling a user-supplied
// script function for each fine-grid cell.

namespace {
    struct sUserIter : public cv_chd_iter
    {
        sUserIter(const char *fn, int cgm, int fg, int bv) :
            cv_chd_iter(cgm, fg, bv)
            {
                funcname = lstring::copy(fn);
                block = 0;
                tmpstr1 = 0;
                tmpstr2 = 0;
                tmpstr3 = 0;
                adata.allocate(20);
                adata.dims()[0] = 20;
                data = adata.values();
                initialized = false;
            }

        ~sUserIter()
            {
                delete [] funcname;
                delete [] tmpstr1;
                delete [] tmpstr2;
                delete [] tmpstr3;
            }

    private:
        XIrt iterFunc(const BBox*);

        char *funcname;
        SIfunc *block;
        siVariable vars[10];
        char *tmpstr1;
        char *tmpstr2;
        char *tmpstr3;
        siAryData adata;
        double *data;
        bool initialized;
    };


    XIrt
    sUserIter::iterFunc(const BBox *cBB)
    {
        if (!ci_db1) {
            Errs()->add_error("iterFunc: null database pointer.");
            return (XIbad);
        }
        if (!ci_db1->table())
            return (XIok);

        if (!initialized) {
            if (!funcname || !*funcname) {
                Errs()->add_error("iterFunc: null or empty callback name.");
                return (XIbad);
            }
            int argc;
            block = SI()->GetSubfunc(funcname, &argc);
            if (!block) {
                Errs()->add_error("iterFunc: no function named %s found.",
                    funcname);
                return (XIbad);
            }

            vars[0].type = TYP_STRING;      // database name
            vars[1].type = TYP_SCALAR;      // j
            vars[2].type = TYP_SCALAR;      // i
            vars[3].type = TYP_SCALAR;      // SPT x (microns)
            vars[4].type = TYP_SCALAR;      // SPT y (microns)
            vars[5].type = TYP_ARRAY;       // lots of stuff
            vars[5].content.a = &adata;
            vars[6].type = TYP_STRING;      // cell name
            vars[7].type = TYP_STRING;      // CHD name

            // data[0]  : spt cols
            // data[1]  : spt rows 
            // data[2]  : fine grid (microns)
            // data[3]  : coarse grid (microns)
            // data[4]  : bloat value (microns)
            // data[5]  : AOI left (microns)
            // data[6]  : AOI bottom (microns)
            // data[7]  : AOI right (microns)
            // data[8]  : AOI top (microns)
            // data[9]  : coarse grid cell left (microns)
            // data[10] : coarse grid cell bottom (microns)
            // data[11] : coarse grid cell right (microns)
            // data[12] : coarse grid cell top (microns)
            // data[13] : fine grid cell left (microns)
            // data[14] : fine grid cell bottom (microns)
            // data[15] : fine grid cell right (microns)
            // data[16] : fine grid cell top (microns)

            data[0] = ci_aoiBB.width()/ci_fine_grid +
               (ci_aoiBB.width()%ci_fine_grid != 0);
            data[1] = ci_aoiBB.height()/ci_fine_grid +
               (ci_aoiBB.height()%ci_fine_grid != 0);
            data[2] = MICRONS(ci_fine_grid);
            data[3] = MICRONS(ci_coarse_grid);
            data[4] = MICRONS(ci_bloat_val);
            data[5] = MICRONS(ci_aoiBB.left);
            data[6] = MICRONS(ci_aoiBB.bottom);
            data[7] = MICRONS(ci_aoiBB.right);
            data[8] = MICRONS(ci_aoiBB.top);

            tmpstr1 = lstring::copy(DBNAME);
            tmpstr2 = lstring::copy(Tstring(ci_symref1->get_name()));
            tmpstr3 = lstring::copy(CDchd()->chdFind(ci_chd1));

            vars[0].content.string = tmpstr1;
            vars[6].content.string = tmpstr2;
            vars[7].content.string = tmpstr3;

            initialized = true;
        }

        data[9] = MICRONS(cBB->left);
        data[10] = MICRONS(cBB->bottom);
        data[11] = MICRONS(cBB->right);
        data[12] = MICRONS(cBB->top);


        int nxf = cBB->width()/ci_fine_grid +
            (cBB->width()%ci_fine_grid != 0);
        int nyf = cBB->height()/ci_fine_grid +
            (cBB->height()%ci_fine_grid != 0);

        SIlexprCx cx;
        for (int i = 0; i < nyf; i++) {
            vars[2].content.value = i;
            data[14] = data[10] + i*data[2];
            vars[4].content.value = data[14] + data[2]/2;
            data[16] = data[14] + data[2];
            if (data[16] > data[12])
                data[16] = data[12];

            for (int j = 0; j < nxf; j++) {
                vars[1].content.value = j;
                data[13] = data[9] + j*data[2];
                vars[3].content.value = data[13] + data[2]/2;
                data[15] = data[13] + data[2];
                if (data[15] > data[11])
                    data[15] = data[11];

                siVariable res;
                XIrt ret = SI()->EvalFunc(block, &cx, vars, &res);
                if (ret != XIok)
                    return (ret);
                if (res.type == TYP_SCALAR && (int)res.content.value != 0) {
                    if ((int)res.content.value == (int)XIintr)
                        return (XIintr);
                    return (XIbad);
                }
            }
        }
        return (XIok);
    }
    // End of sUserIter functions.
}


bool
cCHD::iterateOverRegion(const char *cellname, const char *fname,
    const BBox *AOI, int cgm, int fg, int bv)
{
    if (MICRONS(fg) < 0.01) {
        Errs()->add_error("iterateOverRegion: fine grid too small.");
        return (false);
    }
    if (abs(bv) >= fg/2) {
        Errs()->add_error("iterateOverRegion: bad bloat value.");
        return (false);
    }
    if (cgm < 1) {
        Errs()->add_error("iterateOverRegion: bad coarse grid multiple.");
        return (false);
    }
    if (!fname || !*fname) {
        Errs()->add_error("iterateOverRegion: null or empty function name.");
        return (false);
    }

    sUserIter iter(fname, cgm, fg, bv);
    return (iter.run(this, cellname, AOI) == XIok);
}


//-----------------------------------------------------------------------------
// The following class uses the cv_chd_iter framework to compute the
// density.  The density values are saved in spatial parameter tables
// in memory, which are written as files in the finalize method.

namespace {
    struct sDensIter : public cv_chd_iter
    {
        sDensIter(int cgm, int fg, int bv) : cv_chd_iter(cgm, fg, bv)
            { spt_tables = 0; }
        ~sDensIter();

        stringlist *info();
        bool finalize(unsigned int);

    private:
        XIrt iterFunc(const BBox*);

        SymTab *spt_tables;  // SPTs for density values.
    };


    sDensIter::~sDensIter()
    {
        SymTabGen gen(spt_tables, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            delete (spt_t*)h->stData;
            delete h;
        }
        delete spt_tables;
    }


    // Return a list of SPT names saved in memory.  These will be retained
    // in memory if true is passed to finalize.
    //
    stringlist*
    sDensIter::info()
    {
        stringlist *s0 = 0;
        SymTabGen gen(spt_tables);
        SymTabEnt *h;
        while ((h = gen.next()) != 0)
            s0 = new stringlist(lstring::copy(h->stTag), s0);
        stringlist::sort(s0);
        return (s0);
    }


    // If flags is false, write out the density files and clear the
    // tables.  If flags is true, merge the SPTs so they will persist in
    // memory after the sDensIter object is destroyed.
    //
    bool
    sDensIter::finalize(unsigned int flags)
    {
        if (flags) {
            // Swap in local SP table context.
            SymTab *old_tab = spt_t::swapSPtableCx(spt_tables);

            if (!old_tab) {
                spt_tables = 0;
                return (true);
            }

            SymTabGen gen(spt_tables, true);
            SymTabEnt *h;
            while ((h = gen.next()) != 0) {
                SymTabEnt *ho = SymTab::get_ent(old_tab, h->stTag);
                if (!ho) {
                    spt_t *tab = (spt_t*)h->stData;
                    old_tab->add(tab->keyword(), tab, false);
                }
                else {
                    delete (spt_t*) ho->stData;
                    ho->stTag = h->stTag;
                    ho->stData = h->stData;
                }
                delete h;
            }
            spt_t::swapSPtableCx(old_tab);
            FIO()->ifInfoMessage(IFMSG_INFO, "Done.");
            return (true);
        }
        else {
            // Swap in local SP table context.
            SymTab *old_tab = spt_t::swapSPtableCx(spt_tables);

            bool ret = true;
            char buf[256];
            SymTabGen gen(spt_tables);
            SymTabEnt *h;
            while ((h = gen.next()) != 0) {
                sprintf(buf, "%s.spt", h->stTag);
                if (ret && !spt_t::writeSpatialParameterTable(h->stTag, buf)) {
                    Errs()->add_error(
                        "finalize: error writing parameter table file %s.",
                        buf);
                    ret = false;
                }
                FIO()->ifInfoMessage(IFMSG_INFO, "Wrote %s.", buf);
                spt_t::clearSpatialParameterTable(h->stTag);
            }

            spt_t::swapSPtableCx(old_tab);
            FIO()->ifInfoMessage(IFMSG_INFO, "Done.");
            return (ret);
        }
    }


    // Perform the density calculation and save values.
    //
    XIrt
    sDensIter::iterFunc(const BBox *cBB)
    {
        if (!ci_db1) {
            Errs()->add_error("iterFunc: null database pointer.");
            return (XIbad);
        }
        if (!ci_db1->table())
            return (XIok);

        int nxf = cBB->width()/ci_fine_grid +
            (cBB->width()%ci_fine_grid != 0);
        int nyf = cBB->height()/ci_fine_grid +
            (cBB->height()%ci_fine_grid != 0);

        char buf[256];
        char *lnp = lstring::stpcpy(buf, Tstring(ci_symref1->get_name()));
        *lnp++ = '.';

        // Swap in local SP table context.
        SymTab *old_tab = spt_t::swapSPtableCx(spt_tables);

        stringlist *layers = ci_db1->layers();
        for (stringlist *s = layers; s; s = s->next) {
            const char *lname = s->string;
            strcpy(lnp, lname);

            // These layers were added to the layer table during database
            // creation.
            CDl *ld = CDldb()->findLayer(lname, Physical);
            if (!ld) {
                Errs()->add_error("iterFunc: layer %s unknown (impossible!).",
                    lname);
                spt_t::swapSPtableCx(old_tab);
                stringlist::destroy(layers);
                return (XIbad);
            }
            zbins_t *zdb =
                (zbins_t*)SymTab::get(ci_db1->table(), (unsigned long)ld);
            if (zdb == (zbins_t*)ST_NIL)
                continue;

            spt_t *f = spt_t::findSpatialParameterTable(buf);
            for (int i = 0; i < nyf; i++) {
                int y = cBB->bottom + i*ci_fine_grid;
                for (int j = 0; j < nxf; j++) {

                    Zlist *zl = zdb->getZlist(j, i);
                    if (zl) {
                        int x = cBB->left + j*ci_fine_grid;

                        BBox fBB(x, y, x + ci_fine_grid, y + ci_fine_grid);
                        if (fBB.right > cBB->right)
                            fBB.right = cBB->right;
                        if (fBB.top > cBB->top)
                            fBB.top = cBB->top;

                        float dens = Zlist::area(zl)/fBB.area();

                        if (!f) {
                            int tnxf = ci_aoiBB.width()/ci_fine_grid +
                                (ci_aoiBB.width()%ci_fine_grid != 0);
                            int tnyf = ci_aoiBB.height()/ci_fine_grid +
                                (ci_aoiBB.height()%ci_fine_grid != 0);
                            spt_t::newSpatialParameterTable(buf,
                                MICRONS(ci_aoiBB.left),
                                MICRONS(ci_fine_grid), tnxf,
                                MICRONS(ci_aoiBB.bottom),
                                MICRONS(ci_fine_grid), tnyf);
                            f = spt_t::findSpatialParameterTable(buf);
                            if (!f) {
                                Errs()->add_error(
                                "iterFunc: param table %s creation failed.",
                                    buf);
                                spt_t::swapSPtableCx(old_tab);
                                stringlist::destroy(layers);
                                return (XIbad);
                            }
                        }
                        f->save_item(x, y, dens);
                    }
                }
            }
        }
        stringlist::destroy(layers);
        spt_tables = spt_t::swapSPtableCx(old_tab);
        return (XIok);
    }
    // End of sDensIter functions.
}


// cCHD function to create density maps.  If spts is given, a list of
// SPT names is returned, and these SPTs are retained in memory and
// are not written out.  Otherwise, the SPTs are written to disk files
// and destroyed.
//
bool
cCHD::createDensityMaps(const char *cellname, const BBox *AOI,
    int cgm, int fg, int bv, stringlist **spts)
{
    if (MICRONS(fg) < 0.01) {
        Errs()->add_error("createDensityMaps: fine grid too small.");
        return (false);
    }
    if (cgm < 1) {
        Errs()->add_error("createDensityMaps: bad coarse grid multiple.");
        return (false);
    }

    sDensIter iter(cgm, fg, bv);
    bool ret = (iter.run(this, cellname, AOI) == XIok);
    if (ret) {
        if (spts) {
            *spts = iter.info();
            ret = iter.finalize(true);
        }
        else
            ret = iter.finalize(false);
    }
    return (ret);
}


//-----------------------------------------------------------------------------
// This iterates over two assumed similar hierarchies, comparing the
// geometry and recording differences.

namespace {
    struct sCmpIter : public cv_chd_iter
    {
        sCmpIter(FILE *fp, unsigned int dm, int cgm, int fg) :
            cv_chd_iter(cgm, fg, 0)
            {
                ci_fp = fp;
                ci_ltab = 0;
                ci_ldiffs = 0;
                ci_last_ld = 0;
                ci_diffmax = dm;
                ci_diffcnt = 0;
                ci_last_state = 0;
                ci_lskip = false;
            }

        ~sCmpIter()
            {
                delete ci_ltab;
                delete getDiffs();
            }

        unsigned int diffCount() { return (ci_diffcnt); }
        void setupLdiffs() { ci_ldiffs = new SymTab(false, false); }

        void setupLayers(const char*, bool);
        Sdiff *getDiffs();

    private:
        XIrt iterFunc(const BBox*);

        FILE *ci_fp;
        SymTab *ci_ltab;
        SymTab *ci_ldiffs;
        CDl *ci_last_ld;
        unsigned int ci_diffmax;
        unsigned int ci_diffcnt;
        short ci_last_state;
        bool ci_lskip;
    };


    // Set up layer filtering.
    //
    void
    sCmpIter::setupLayers(const char *layer_list, bool skip)
    {
        delete ci_ltab;
        if (layer_list) {
            ci_ltab = new SymTab(true, false);
            const char *s = layer_list;
            char *tok;
            while ((tok = lstring::gettok(&s)) != 0) {
                if (SymTab::get(ci_ltab, tok) == ST_NIL)
                    ci_ltab->add(tok, 0, false);
                else
                    delete [] tok;
            }
        }
        else
            ci_ltab = 0;
        ci_lskip = skip;
    }


    // Return the recorded differences, if any are saved.  This clears the
    // table.
    //
    Sdiff *
    sCmpIter::getDiffs()
    {
        if (!ci_ldiffs)
            return (0);

        Ldiff *l0 = 0, *le = 0;
        CDlgen lgen(Physical);
        CDl *ld;
        while ((ld = lgen.next()) != 0) {
            SymTabEnt *h = SymTab::get_ent(ci_ldiffs, (unsigned long)ld);
            if (h) {
                Ldiff *lx = (Ldiff*)h->stData;
                h->stData = 0;
                if (!l0)
                    l0 = le = lx;
                else {
                    le->set_next_ldiff(lx);
                    le = lx;
                }
            }
        }
        delete ci_ldiffs;
        ci_ldiffs = 0;

        return (new Sdiff(l0, 0));
    }


    namespace {
        inline const char *
        morediffs()
        {
            return ("    more differences found");
        }
    }


    XIrt
    sCmpIter::iterFunc(const BBox *cBB)
    {
        if (!ci_db1) {
            Errs()->add_error("iterFunc: null database-1 pointer.");
            return (XIbad);
        }

        if (!ci_db2) {
            Errs()->add_error("iterFunc: null database-2 pointer.");
            return (XIbad);
        }

        int nxf = cBB->width()/ci_fine_grid +
            (cBB->width()%ci_fine_grid != 0);
        int nyf = cBB->height()/ci_fine_grid +
            (cBB->height()%ci_fine_grid != 0);

        SymTab *ltab = new SymTab(true, false);
        stringlist *layers = ci_db1->layers();
        for (stringlist *s = layers; s; s = s->next) {
            ltab->add(s->string, 0, false);
            s->string = 0;
        }
        stringlist::destroy(layers);
        layers = ci_db2->layers();
        for (stringlist *s = layers; s; s = s->next) {
            if (SymTab::get(ltab, s->string) == ST_NIL)
                ltab->add(s->string, 0, false);
            else
                delete [] s->string;
            s->string = 0;
        }
        stringlist::destroy(layers);
        layers = SymTab::names(ltab);
        delete ltab;

        XIrt ret = XIok;
        for (stringlist *s = layers; s; s = s->next) {
            const char *lname = s->string;
            if (ci_ltab) {
                if (ci_lskip) {
                    if (SymTab::get(ci_ltab, lname) != ST_NIL)
                        continue;
                }
                else {
                    if (SymTab::get(ci_ltab, lname) == ST_NIL)
                        continue;
                }
            }

            // These layers were added to the layer table during database
            // creation.
            CDl *ld = CDldb()->findLayer(lname, Physical);
            if (!ld) {
                Errs()->add_error("iterFunc: layer %s unknown (impossible!).",
                    lname);
                ret = XIbad;
                break;
            }
            zbins_t *zdb1 =
                (zbins_t*)SymTab::get(ci_db1->table(), (unsigned long)ld);
            zbins_t *zdb2 =
                (zbins_t*)SymTab::get(ci_db2->table(), (unsigned long)ld);

            if (zdb1 == (zbins_t*)ST_NIL)
                zdb1 = 0;
            if (zdb2 == (zbins_t*)ST_NIL)
                zdb2 = 0;
            if (!zdb1 && !zdb2)
                continue;

            for (int i = 0; i < nyf; i++) {
                for (int j = 0; j < nxf; j++) {

                    Zlist *z1 = zdb1 ? zdb1->getZlist(j, i) : 0;
                    Zlist *z2 = zdb2 ? zdb2->getZlist(j, i) : 0;
                    z1 = Zlist::copy(z1);
                    z2 = Zlist::copy(z2);

                    Ldiff *ld0 = 0;
                    if (!ci_fp && ci_ldiffs) {
                        ld0 = (Ldiff*)SymTab::get(ci_ldiffs, (unsigned long)ld);
                        if (ld0 == (Ldiff*)ST_NIL)
                            ld0 = 0;
                    }
                    bool local = (ld0 == 0);
                    int state = 0;

                    ret = Zlist::zl_andnot2(&z1, &z2);
                    if (ret == XIintr) {
                        if (local)
                            delete ld0;
                        goto done;
                    }
                    else if (ret == XIbad) {
                        Errs()->add_error("iterFunc: zl_andnot2 failed.");
                        if (local)
                            delete ld0;
                        goto done;
                    }

                    unsigned int diffcnt = 0;
                    bool skip = false;
                    CDo *o1 = Zlist::to_obj_list(z1, ld);
                    const CDo *on;
                    for (const CDo *o = o1; o; o = on) {
                        on = o->next_odesc();
                        if (!skip) {
                            if (ci_diffmax > 0 && diffcnt >= ci_diffmax) {
                                ld0 = Ldiff::save(ld0, ld, morediffs(), 0);
                                skip = true;
                            }
                            else {
                                char *str = o->cif_string(0, 0);
                                ld0 = Ldiff::save(ld0, ld, str, 0);
                                delete [] str;
                            }
                            state = 1;
                            diffcnt++;
                        }
                        delete o;
                    }
                    ci_diffcnt += diffcnt;

                    diffcnt = 0;
                    skip = false;
                    CDo *o2 = Zlist::to_obj_list(z2, ld);
                    for (const CDo *o = o2; o; o = on) {
                        on = o->next_odesc();
                        if (!skip) {
                            if (ci_diffmax > 0 && diffcnt >= ci_diffmax) {
                                ld0 = Ldiff::save(ld0, ld, 0, morediffs());
                                skip = true;
                            }
                            else {
                                char *str = o->cif_string(0, 0);
                                ld0 = Ldiff::save(ld0, ld, 0, str);
                                delete [] str;
                            }
                            if (state == 1)
                                state = 4;
                            else
                                state = 2;
                            diffcnt++;
                        }
                        delete o;
                    }
                    ci_diffcnt += diffcnt;

                    if (ci_fp) {
                        if (ld0) {
                            // Some stuff to avoid repeating header string.
                            bool nohdr =
                                (ci_last_ld == ld && ci_last_state == state);
                            ld0->print(ci_fp, nohdr);
                            ci_last_ld = ld;
                            ci_last_state = (state & 0x3);
                            delete ld0;
                        }
                    }
                    else {
                        if (ci_ldiffs) {
                            if (SymTab::get(ci_ldiffs, (unsigned long)ld) ==
                                    ST_NIL)
                                ci_ldiffs->add((unsigned long)ld, ld0, false);
                        }
                        else
                            delete ld0;
                    }
                    if (ci_diffmax > 0 && ci_diffcnt >= ci_diffmax) {
                        if (ci_fp) {
                            fprintf(ci_fp,
                                "*** Max difference count %d reached.\n",
                                ci_diffmax);
                        }
                        ci_endit = true;
                        goto done;
                    }
                }
            }
        }
    done:
        stringlist::destroy(layers);
        return (ret);
    }
    // End of sCmpIter functions.
}

//XXX make next two funcs accept const CHD*

// Static function.
// Run the comparison, dumping output to a file.
//
XIrt
cCHD::compareCHDs_fp(cCHD *chd1, const char *cname1, cCHD *chd2,
    const char *cname2, const BBox *AOI, const char *layer_list, bool skip,
    FILE *fp, unsigned int maxerrs, unsigned int *errcnt, int cgm, int fg)
{
    if (!chd1 || !chd2) {
        Errs()->add_error("compareCHDs: null CHD pointer.");
        return (XIbad);
    }
    if (MICRONS(fg) < 1.0 || MICRONS(fg) > 100.0) {
        Errs()->add_error(
            "compareCHDs: fine grid out of range (1.0 - 100.0).");
        return (XIbad);
    }
    if (cgm < 1 || cgm > 100) {
        Errs()->add_error(
            "compareCHDs: coarse grid multiple out of range (1 - 100).");
        return (XIbad);
    }
    if (errcnt)
        *errcnt = 0;

    sCmpIter iter(fp, maxerrs, cgm, fg);
    iter.setupLayers(layer_list, skip);
    XIrt ret = iter.run2(chd1, cname1, chd2, cname2, AOI);
    if (ret != XIbad) {
        if (errcnt)
            *errcnt = iter.diffCount();
    }
    return (ret);
}


// Static function.
// Run the comparison, saving output in the returned Sdiff.
//
XIrt
cCHD::compareCHDs_sd(cCHD *chd1, const char *cname1, cCHD *chd2,
    const char *cname2, const BBox *AOI, const char *layer_list, bool skip,
    Sdiff **sdiff, unsigned int maxerrs, unsigned int *errcnt, int cgm, int fg)
{
    if (!chd1 || !chd2) {
        Errs()->add_error("compareCHDs: null CHD pointer.");
        return (XIbad);
    }
    if (MICRONS(fg) < 1.0 || MICRONS(fg) > 100.0) {
        Errs()->add_error(
            "compareCHDs: fine grid out of range (1.0 - 100.0).");
        return (XIbad);
    }
    if (cgm < 1 || cgm > 100) {
        Errs()->add_error(
            "compareCHDs: coarse grid multiple out of range (1 - 100).");
        return (XIbad);
    }
    if (errcnt)
        *errcnt = 0;

    sCmpIter iter(0, maxerrs, cgm, fg);
    iter.setupLayers(layer_list, skip);
    if (sdiff) {
        *sdiff = 0;
        iter.setupLdiffs();
    }
    XIrt ret = iter.run2(chd1, cname1, chd2, cname2, AOI);
    if (ret != XIbad) {
        if (sdiff)
            *sdiff = iter.getDiffs();
        if (errcnt)
            *errcnt = iter.diffCount();
    }
    return (ret);
}

