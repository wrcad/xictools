
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

#include "fio.h"
#include "fio_chd.h"
#include "fio_chd_flat.h"
#include "fio_cvt_base.h"
#include "fio_chd_cvtab.h"
#include "fio_chd_ecf.h"
#include "fio_oasis.h"
#include "fio_cgd.h"
#include "fio_chd_flat_prv.h"
#include "cd_strmdata.h"
#include "cd_hypertext.h"
#include "miscutil/filestat.h"
#include "miscutil/timedbg.h"
#include <algorithm>


//-----------------------------------------------------------------------------
// cCHD flattening functions
//-----------------------------------------------------------------------------

// Print the AOI when flattening (if using windowing).
//#define FLATTEN_DBG


// Write out each region in the box list to the output file as a flat
// cell with the cellname given in the list.  The output file becomes a
// library of the flat region cells.
//
OItype
cCHD::writeFlatRegions(const char *cellname, const FIOcvtPrms *prms,
    named_box_list *nblist)
{
    // Local struct for cleanup and timing info.
    struct wfr_cleanup : public TimeDbg
    {
        wfr_cleanup() : TimeDbg("chd_write_flat_regions")
            {
                ctab = 0;
                itab = 0;
                in = 0;
                out = 0;
            }

        ~wfr_cleanup()
            {
                delete ctab;
                delete itab;
                delete in;
                delete out;
            }
            
        cCVtab      *ctab;
        chd_intab   *itab;
        cv_in       *in;
        cv_out      *out;
    };
    wfr_cleanup wfc;

    if (!prms) {
        Errs()->add_error("cCHD::writeFlatRegions: null parameters pointer.");
        return (OIerror);
    }
    if (prms->scale() < .001 || prms->scale() > 1000.0) {
        Errs()->add_error("cCHD::writeFlatRegions: bad scale.");
        return (OIerror);
    }
    if (!cellname || !*cellname) {
        cellname = defaultCell(Physical);
        if (!cellname) {
            Errs()->add_error(
            "cCHD::writeFlatRegions: no default for null/empty cell name.");
            return (OIerror);
        }
    }

    symref_t *p = findSymref(cellname, Physical, false);
    if (!p) {
        Errs()->add_error("cCHD::writeFlatRegions: cell %s not found in CHD.",
            cellname);
        return (OIerror);
    }
    if (!setBoundaries(p)) {
        Errs()->add_error(
            "cCHD::writeFlatRegions: CHD cell boundary setup failed.");
        return (OIerror);
    }

    // If layer filtering, do the empty-cell pre-filtering, which sets
    // the CVemty flags in the symrefs in the hierarchy.  These are
    // cleared from the ecf destructor.
    //
    CVecFilt ecf;
    ecf.setup(this, p);

    // Create input for this CHD.
    //
    wfc.in = newInput(prms->allow_layer_mapping());
    if (!wfc.in) {
        Errs()->add_error(
            "cCHD::writeFlatRegions: main input channel creation failed.");
        return (OIerror);
    }

    // Set input aliasing to CHD aliasing.
    //
    wfc.in->assign_alias(new FIOaliasTab(true, false, c_alias_info));

    // Create output channel.
    //
    FileType ftype = prms->filetype();
    const char *fname = prms->destination();
    if (ftype == Fnone)
        ftype = cFIO::TypeExt(fname);
    if (ftype == Fnone &&
            (!fname || !*fname || filestat::is_directory(fname)))
        ftype = Fnative;
    if (!wfc.in->setup_destination(fname, ftype, prms->to_cgd())) {
        Errs()->add_error("cCHD::writeFlatRegions: destination setup failed.");
        return (OIerror);
    }

    // Set up flattening.
    //
    wfc.in->set_flatten(CDMAXCALLDEPTH, true);
    wfc.in->TPush();
    wfc.in->TLoad(CDtfRegI2);

    // Loop over regions, witing output.
    //
    bool ok = true;
    bool first = true;
    wfc.out = wfc.in->backend();
    int flatmax = CDMAXCALLDEPTH;
    time_t tloc = time(0);
    tm *date = gmtime(&tloc);
    for (named_box_list *nbl = nblist; nbl; nbl = nbl->next) {

        // Set up windowing/clipping.
        //
        wfc.in->set_area_filt(true, &nbl->BB);
        wfc.in->set_clip(true);

        // Build table of cells to write.
        //
        BBox tBB(nbl->BB);
        tBB.scale(1.0/prms->scale());

        wfc.ctab = new cCVtab(false, 1);
        if (!wfc.ctab->build_BB_table(this, p, 0, &tBB)) {
            Errs()->add_error(
                "cCHD::writeFlatRegions: failed to build cell table.");
            return (OIerror);
        }
        if (!wfc.ctab->build_TS_table(p, 0, flatmax)) {
            Errs()->add_error(
            "cCHD::writeFlatRegions: failed to build flat cell map table.");
            return (OIerror);
        }

        // Set up additional CHD input channels, for library references.
        //
        wfc.itab = new chd_intab;
        for (int i = 0; ; i++) {
            cCHD *tchd = wfc.ctab->get_chd(i);
            if (!tchd)
                break;
            if (tchd == this) {
                wfc.itab->insert(tchd, wfc.in);
                wfc.itab->set_no_free(wfc.in);
                continue;
            }
            cv_in *new_in = tchd->newInput(prms->allow_layer_mapping());
            if (!new_in) {
                Errs()->add_error(
                    "cCHD::writeFlatRegions: reference channel setup failed.");
                return (OIerror);
            }
            new_in->set_area_filt(true, &nbl->BB);
            new_in->set_clip(true);
            new_in->set_flatten(CDMAXCALLDEPTH, true);
            new_in->TPush();
            new_in->TLoad(CDtfRegI2);
            new_in->setup_backend(wfc.in->peek_backend());
            wfc.itab->insert(tchd, new_in);
        }

        // Write out the table contents.
        //
        CVtabGen cfgen(wfc.ctab, 0, p);
        cCHD *tchd;
        symref_t *tp;
        cv_in *cin = wfc.in;
        cCHD *cchd = this;
        if (!cin->chd_setup(cchd, wfc.ctab, 0, Physical, prms->scale())) {
            Errs()->add_error(
                "cCHD::writeFlatRegions: main channel setup failed.");
            ok = false;
        }
        if (ok && first && wfc.out) {
            first = false;
            if (!wfc.in->no_open_lib() &&
                    !wfc.out->open_library(Physical, 1.0)) {
                Errs()->add_error(
                "cCHD::writeFlatRegions: main channel header write failed.");
                ok = false;
            }
        }
        if (ok && wfc.out) {
            if (!wfc.out->write_struct(nbl->name, date, date)) {
                Errs()->add_error(
                    "cCHD::writeFlatRegions: structure write failed.");
                ok = false;
            }
        }
        if (ok) {
            cin->set_ignore_instances(true);
            unsigned char *tstream;
            cvtab_item_t *item;
            while ((item = cfgen.next()) != 0) {
                tchd = wfc.ctab->get_chd(item->get_chd_tkt());
                tp = item->symref();
                tstream = wfc.ctab->get_tstream(item);
                if (!tstream)
                    continue;
                if (tchd != cchd) {
                    cin->set_ignore_instances(false);
                    cin->set_transform_level(0);
                    cin->chd_finalize();
                    FIOaliasTab *at = cin->extract_alias();
                    cin = wfc.itab->find(tchd);
                    cin->assign_alias(at);
                    cchd = tchd;

                    if (!cin->chd_setup(cchd, wfc.ctab, 0, Physical,
                            prms->scale())) {
                        Errs()->add_error(
                    "cCHD::writeFlatRegions: reference channel setup failed.");
                        ok = false;
                        break;
                    }
                    cin->set_ignore_instances(true);
                }
                cin->set_transform_level(1);  // suppress cell headers
                cin->set_tf_list(tstream);
                bool ret = cin->chd_read_cell(tp, false);
                cin->set_tf_list(0);
                if (!ret) {
                    Errs()->add_error(
                        "cCHD::writeFlatRegions: cell read failed.");
                    ok = false;
                    break;
                }
            }
        }
        cin->set_ignore_instances(false);
        cin->set_transform_level(0);
        cin->set_tf_list(0);
        cin->chd_finalize();
        wfc.in->assign_alias(cin->extract_alias());
        if (ok && wfc.out) {
            if (!wfc.out->write_end_struct()) {
                Errs()->add_error(
                    "cCHD::writeFlatRegions: write end struct failed.");
                ok = false;
            }
        }

        delete wfc.itab;
        wfc.itab = 0;
        delete wfc.ctab;
        wfc.ctab = 0;

        if (!ok)
            break;
    }
    if (ok && wfc.out) {
        if (!wfc.in->no_end_lib() && !wfc.out->write_endlib(0)) {
            Errs()->add_error(
                "cCHD::writeFlatRegions: write end lib failed.");
            ok = false;
        }
    }

    OItype oiret = ok ? OIok : wfc.in->was_interrupted() ? OIaborted : OIerror;

    return (oiret);
}


// Read cell and its flattened hierarchy into the database in the cell
// named cellname.
//
// For each cell in the hierarchy, create a list of the transforms for
// each instance of the cell, relative to the top level.  Then, read
// each cell, without recursion.  For each object, loop through the
// transform list, saving the object under each transform (if it is in
// the reference area, if applicable).
//
// The TT_U1 transform register is applied to the top of the transform
// stack, and so will be applied to the output objects.  This enables
// setting up a pseudo "instance" transformation, which might be
// useful.
//
// Note that the windowing area must be in post-transform coordinates.
//
//  Return values:
//    OIerror       unspecified error
//    OIok          ok
//    OIaborted     user aborted
//
OItype
cCHD::readFlat(const char *cellname, const FIOcvtPrms *prms,
    cv_backend *backend, int maxdepth, int size_thr, bool keep_clipped_text)
{
    // Local struct for cleanup and timing info.
    struct rf_cleanup : public TimeDbg
    {
        rf_cleanup() : TimeDbg("chd_read_flat")
            {
                in = 0;
                out = 0;
            }

        ~rf_cleanup()
            {
                delete in;
                delete out;
            }
            
        cv_in           *in;
        rf_out          *out;
    };
    rf_cleanup rfc;

    if (!prms) {
        Errs()->add_error("cCHD::readFlat: null parameters pointer.");
        return (OIerror);
    }
    if (prms->scale() < .001 || prms->scale() > 1000.0) {
        Errs()->add_error("cCHD::readFlat: bad scale.");
        return (OIerror);
    }

    if (!cellname || !*cellname) {
        cellname = defaultCell(Physical);
        if (!cellname) {
            Errs()->add_error(
                "cCHD::readFlat: no default for null/empty cell name.");
            return (OIerror);
        }
    }

    symref_t *p = findSymref(cellname, Physical, false);
    if (!p) {
        Errs()->add_error("cCHD::readFlat: cell %s not found in CHD.",
            cellname);
        return (OIerror);
    }
    if (prms->use_window()) {
        if (!setBoundaries(p)) {
            Errs()->add_error(
                "cCHD::readFlat: CHD cell boundary setup failed.");
            return (OIerror);
        }
        if (!prms->window()->intersect(p->get_bb(), false)) {
            // We look for this text in WindowDesc::compose_chd_image.
            Errs()->add_error("No cell/window intersection area.");
            return (OIerror);
        }
    }

    // Create input for this CHD.
    //
    rfc.in = newInput(prms->allow_layer_mapping());
    if (!rfc.in) {
        Errs()->add_error(
            "cCHD::readFlat: main input channel creation failed.");
        return (OIerror);
    }
    rfc.in->set_keep_clipped_text(keep_clipped_text);

    // Set input aliasing to CHD aliasing.
    //
    rfc.in->assign_alias(new FIOaliasTab(true, false, c_alias_info));

    // Make sure all layers exist.
    if (!createLayers())
        return (OIerror);

    // Set up back end.
    //
    if (backend) {
        // Assume here that the main database is not used, so skip creating
        // top symbol.

        // Since windowing/clipping is done in the cv_in, we can skip
        // this in the back end.
        // out = new rf_out(0, prms->use_window() ? prms->regionBB() : 0,
        //     in, prms->clip());
        rfc.out = new rf_out(0, 0, rfc.in, false);
        rfc.out->set_backend(backend);
    }
    else {
        CDs *sd = CD()->ReopenCell(cellname, Physical);
        if (!sd) {
            Errs()->add_error("cCHD::readFlat: top cell reopen failed.");
            return (OIerror);
        }
        // Since windowing/clipping is done in the cv_in, we can skip
        // this in the back end.
        // out = new rf_out(sd, prms->use_window() ? prms->regionBB() : 0,
        //     in, prms->clip());
        rfc.out = new rf_out(sd, 0, rfc.in, false);
    }
    rfc.out->set_size_thr(size_thr);
    if (!rfc.in->setup_backend(rfc.out)) {
        Errs()->add_error("cCHD::readFlat: back-end setup failed.");
        return (OIerror);
    }

    OItype oiret = flatten(p, rfc.in, maxdepth, prms);
    return (oiret);
}


// Read cell and its flattened hierarchy into a special database.
//
// Special readFlat to read objects into a flat object database.
// This is a symbol table keyed by layer pointers, with the payload
// being an ordered list of objects (cd_sdb.cc/cd_sdb.h).
//
//  Return values:
//    OIerror       unspecified error
//    OIok          ok
//    OIaborted     user aborted
//
// WARNING: *tabptr must be 0, or a new Symtab(false, false)
//
OItype
cCHD::readFlat_odb(SymTab **tabptr, const char *cellname,
    const FIOcvtPrms *prms)
{
    cv_backend_odb bk;
    OItype oiret = readFlat(cellname, prms, &bk);
    if (oiret != OIok) {
        SymTabGen gen(bk.table, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            delete (odb_t*)h->stData;
            delete h;
        }
        delete bk.table;
        return (oiret);
    }
    *tabptr = bk.table;
    return (OIok);
}


// Read cell and its flattened hierarchy into a trapezoid database.
//
// Special readFlat to read objects as trapezoids into a database.
// This is a symbol table keyed by layer pointers, with the payload
// being an ordered list of trapezoids (cd_sdb.cc/cd_sdb.h).
//
//  Return values:
//    OIerror       unspecified error
//    OIok          ok
//    OIaborted     user aborted
//
// WARNING: *tabptr must be 0, or a new Symtab(false, false)
//
OItype
cCHD::readFlat_zdb(SymTab **tabptr, const char *cellname,
    const FIOcvtPrms *prms)
{
    cv_backend_zdb bk;
    OItype oiret = readFlat(cellname, prms, &bk);
    if (oiret != OIok) {
        SymTabGen gen(bk.table, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            delete (zdb_t*)h->stData;
            delete h;
        }
        delete bk.table;
        return (oiret);
    }
    *tabptr = bk.table;
    return (OIok);
}


// Read cell and its flattened hierarchy into a Zlist database.
//
// Special readFlat to read objects as trapezoids into a database.
// This is a symbol table keyed by layer pointers, with the payload
// being a linked list of trapezoids.
//
//  Return values:
//    OIerror       unspecified error
//    OIok          ok
//    OIaborted     user aborted
//
// WARNING: *tabptr must be 0, or a new Symtab(false, false)
//
OItype
cCHD::readFlat_zl(SymTab **tabptr, const char *cellname,
    const FIOcvtPrms *prms)
{
    cv_backend_zl bk;
    OItype oiret = readFlat(cellname, prms, &bk);
    if (oiret != OIok) {
        SymTabGen gen(bk.table, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            Zlist::destroy(((Zlist*)h->stData));
            delete h;
        }
        delete bk.table;
        return (oiret);
    }
    *tabptr = bk.table;
    return (OIok);
}


// Read cell and its flattened hierarchy into a zbins_t database.
//
// Special readFlat to read objects as trapezoids into a database.
// This is a symbol table keyed by layer pointers, with the payload
// being an array of clipped trapezoid list heads. 
//
//  Return values:
//    OIerror       unspecified error
//    OIok          ok
//    OIaborted     user aborted
//
// WARNING: *tabptr must be 0, or a new Symtab(false, false)
//
OItype
cCHD::readFlat_zbdb(SymTab **tabptr, const char *cellname,
    const FIOcvtPrms *prms, unsigned int dx, unsigned int dy,
    unsigned int bx, unsigned int by)
{
    if (!prms || !prms->use_window())
        return (OIerror);

    unsigned int nx = (prms->window()->width() + dx - 1)/dx;
    unsigned int ny = (prms->window()->height() + dy - 1)/dy;
    cv_backend_zbdb bk(*tabptr,
        prms->window()->left, prms->window()->bottom,
        nx, ny, dx, dy, bx, by);

    Tdbg()->start_timing("read_flat_zbdb");

    // The passed window needs to be bloated by the boundary area.
    BBox BB = *prms->window();
    BB.left -= bx;
    BB.bottom -= by;
    BB.right += bx;
    BB.top += by;
    FIOcvtPrms fprms(*prms);
    fprms.set_window(&BB);

    OItype oiret = readFlat(cellname, &fprms, &bk);
    if (oiret != OIok) {
        SymTabGen gen(bk.output_table(), true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            delete (zbins_t*)h->stData;
            delete h;
        }
        delete bk.output_table();
        *tabptr = 0;
        return (oiret);
    }
    Tdbg()->stop_timing("read_flat_zbdb");

    Tdbg()->start_timing("read_flat_zbdb_merge");
    // Make sure that the accumulated lists are conditioned for use.
    SymTabGen gen(bk.output_table(), false);
    SymTabEnt *h;
    while ((h = gen.next()) != 0)
        ((zbins_t*)h->stData)->merge();
    *tabptr = bk.output_table();
    Tdbg()->stop_timing("read_flat_zbdb_merge");
    return (OIok);
}


namespace {
    // A struct to handle user feedback while flattening.
    //
    struct flat_read_fb
    {
        flat_read_fb(int total)
            {
                frfb_total = 0;
                frfb_delta = 0;
                frfb_count = 0;
                frfb_dcount = 0;
                if (FIO()->IsFlatReadFeedback()) {
                    frfb_total = total;
                    frfb_delta = frfb_total/20;
                    if (!frfb_delta)
                        frfb_delta++;
                }
            }

        void check()
            {
                if (frfb_total) {
                    frfb_count++;
                    if (frfb_count > frfb_dcount) {
                        FIO()->ifInfoMessage(IFMSG_INFO,
                            "Cells read: %d/%d\n", frfb_count, frfb_total);
                        frfb_dcount += frfb_delta;
                    }
                }
            }

        void final()
            {
                if (frfb_total) {
                    FIO()->ifInfoMessage(IFMSG_INFO,
                        "Cells read: %d/%d\n", frfb_total, frfb_total);
                }
            }

    private:
        int frfb_total;
        int frfb_delta;
        int frfb_count;
        int frfb_dcount;
    };
}


// Core function for flattening.
//
// In this flattening scheme, each symref is associated with a list of
// instance placement transforms.  An object from the cell is written
// according to each transform in the list (if it passes windowing
// tests).
//
// When flattening, the TT_U1 transform register is applied to the top
// of the transform stack, and so will be applied to the output
// objects.  This enables setting up a pseudo "instance"
// transformation, which might be useful.
//
// Note that the windowing area must be in post-transform coordinates.
//
OItype
cCHD::flatten(symref_t *p, cv_in *in, int maxdepth, const FIOcvtPrms *prms)
{
    // Local struct for cleanup and timing info.
    struct flatten_cleanup : public TimeDbg
    {
        flatten_cleanup() : TimeDbg("chd_flatten")
            {
                ctab = 0;
                itab = 0;
            }

        ~flatten_cleanup()
            {
                delete ctab;
                delete itab;
            }
            
        cCVtab      *ctab;
        chd_intab   *itab;
    };
    flatten_cleanup fc;

    if (!p || !in || !prms) {
        Errs()->add_error("cCHD::flatten: unexpected null argument.");
        return (OIerror);
    }
    if (p->get_elec()) {
        Errs()->add_error(
            "cCHD::flatten: can't flatten electrical data.");
        return (OIerror);
    }

    // Set up windowing/clipping.
    //
    if (prms->use_window()) {
        if (!setBoundaries(p)) {
            Errs()->add_error(
                "cCHD::flatten: CHD cell boundary setup failed.");
            return (OIerror);
        }
#ifdef FLATTEN_DBG
        printf("AOI %g %g %g %g\n", MICRONS(prms->window()->left),
            MICRONS(prms->window()->bottom), MICRONS(prms->window()->right),
            MICRONS(prms->window()->top));
#endif
        in->set_area_filt(true, prms->window());
        in->set_clip(prms->clip());
    }
    else {
        in->set_area_filt(false, 0);
        in->set_clip(false);
    }

    // If layer filtering, do the empty-cell pre-filtering, which sets
    // the CVemty flags in the symrefs in the hierarchy.  These are
    // cleared from the ecf destructor.
    //
    CVecFilt ecf;
    ecf.setup(this, p);

    // Set up flattening.
    //
    if (maxdepth < 0 || maxdepth > CDMAXCALLDEPTH)
        maxdepth = CDMAXCALLDEPTH;
    in->set_flatten(maxdepth, true);
    in->TPush();
    in->TLoad(CDtfRegI2);

    // Build table of cells to write.
    //
    bool ok = true;
    fc.ctab = new cCVtab(false, 1);;
    if (prms->use_window()) {
        BBox tBB(*prms->window());
        tBB.scale(1.0/prms->scale());
        if (!fc.ctab->build_BB_table(this, p, 0, &tBB)) {
            Errs()->add_error("cCHD::flatten: failed to build cell table.");
            return (OIerror);
        }
        ok = fc.ctab->build_TS_table(p, 0, maxdepth);
    }
    else {
        if (!fc.ctab->build_BB_table(this, p, 0, 0)) {
            Errs()->add_error("cCHD::flatten: failed to build cell table.");
            return (OIerror);
        }
        ok = fc.ctab->build_TS_table(p, 0, maxdepth);
    }
    if (!ok) {
        Errs()->add_error(
            "cCHD::flatten: failed to build flat cell map table.");
        return (OIerror);
    }

    // Set up additional CHD input channels, for library references.
    //
    fc.itab = new chd_intab;
    for (int i = 0; ; i++) {
        cCHD *tchd = fc.ctab->get_chd(i);
        if (!tchd)
            break;
        if (tchd == this) {
            fc.itab->insert(tchd, in);
            fc.itab->set_no_free(in);
            continue;
        }
        cv_in *new_in = tchd->newInput(prms->allow_layer_mapping());
        if (!new_in) {
            Errs()->add_error(
                "cCHD::write: reference channel setup failed.");
            return (OIerror);
        }
        if (prms->use_window()) {
            new_in->set_area_filt(true, prms->window());
            new_in->set_clip(prms->clip());
        }
        new_in->set_flatten(maxdepth, true);
        new_in->TPush();
        new_in->TLoad(CDtfRegI2);
        new_in->setup_backend(in->peek_backend());
        fc.itab->insert(tchd, new_in);
    }

    // Write out the table contents.
    //
    TimeDbg tdbg("read_content");
    cv_out *out = in->peek_backend();  // don't free!
    CVtabGen ctgen(fc.ctab, 0, p);
    cCHD *tchd;
    symref_t *tp;
    cv_in *cin = in;
    cCHD *cchd = this;
    if (!cin->chd_setup(cchd, fc.ctab, 0, Physical, prms->scale())) {
        Errs()->add_error("cCHD::flatten: main channel setup failed.");
        ok = false;
    }
    if (ok && out) {
        if (!in->no_open_lib() && !out->open_library(Physical, 1.0)) {
            Errs()->add_error(
                "cCHD::flatten: main channel header write failed.");
            ok = false;
        }
    }
    if (ok) {
        bool first = true;
        cin->set_ignore_instances(true);
        unsigned char *tstream;
        flat_read_fb frfb(fc.ctab->num_cells(0));
        cvtab_item_t *item;
        while ((item = ctgen.next()) != 0) {
            tchd = fc.ctab->get_chd(item->get_chd_tkt());
            tp = item->symref();
            tstream = fc.ctab->get_tstream(item);
            if (!tstream)
                continue;
            if (tchd != cchd) {
                cin->set_ignore_instances(false);
                cin->set_transform_level(0);
                cin->chd_finalize();
                FIOaliasTab *at = cin->extract_alias();
                cin = fc.itab->find(tchd);
                cin->assign_alias(at);
                cchd = tchd;

                if (!cin->chd_setup(cchd, fc.ctab, 0, Physical,
                        prms->scale())) {
                    Errs()->add_error(
                        "cCHD::flatten: reference channel setup failed.");
                    ok = false;
                    break;
                }
                cin->set_ignore_instances(true);
            }
            if (first) {
                cin->set_transform_level(0);
                first = false;
            }
            else
                cin->set_transform_level(1);  // suppress cell headers
            frfb.check();
            cin->set_tf_list(tstream);
            ok = cin->chd_read_cell(tp, false);
            cin->set_tf_list(0);
            if (!ok) {
                Errs()->add_error("cCHD::flatten: cell read failed.");
                break;
            }
        }
        frfb.final();
    }
    cin->set_ignore_instances(false);
    cin->set_transform_level(0);
    cin->set_tf_list(0);
    cin->chd_finalize();
    in->assign_alias(cin->extract_alias());

    if (ok && out) {
        if (!out->write_end_struct()) {
            Errs()->add_error("cCHD::flatten: write end struct failed.");
            ok = false;
        }
        if (ok) {
            if (!in->no_end_lib() &&
                    !out->write_endlib(Tstring(p->get_name()))) {
                Errs()->add_error("cCHD::flatten: write end lib failed.");
                ok = false;
            }
        }
    }
    OItype oiret =
        ok ? OIok : cin->was_interrupted() ? OIaborted : OIerror;

    return (oiret);
}


// Make sure all layers exist, otherwise layer exppressions may fail
// if applied before a given layer is seen.  This makes sure that all
// layers map to existing layers, but DOES NOT ensure that the mapped
// layer is accessible with the given or encoded hex name!
//
bool
cCHD::createLayers()
{
    stringlist *sl = layers(Physical);
    for (stringlist *s = sl; s; s = s->next) {
        if (c_filetype == Fgds || c_filetype == Foas) {
            int layer, dtype;
            if (strmdata::hextrn(s->string, &layer, &dtype)) {
                CDll *ll = FIO()->GetGdsInputLayers(layer, dtype, Physical);
                if (ll)
                    CDll::destroy(ll);
                else {
                    bool err;
                    FIO()->MapGdsLayer(layer, dtype, Physical, s->string,
                        true, &err);
                }
                continue;
            }
        }
        CDl *ld = CDldb()->findLayer(s->string, Physical);
        if (!ld) {
            if (FIO()->IsNoCreateLayer()) {
                Errs()->add_error(
                    "cCHD::readFlat: not allowed to create layer %s.",
                    s->string);
                return (false);
            }
            ld = CDldb()->newLayer(s->string, Physical);
            if (!ld) {
                Errs()->add_error(
                    "cCHD::readFlat: failed to create layer %s.",
                    s->string);
                return (false);
            }
        }
    }
    stringlist::destroy(sl);
    return (true);
}
// End of cCHD functions.


//-----------------------------------------------------------------------------
// rf_out functions
// Private back-end for readFlat.

// This is called when using override cells from the database.  We have to
// apply the current transformation to the object data.
//
bool
rf_out::write_object(const CDo *odesc, cvLchk *lchk)
{
    if (!odesc)
        return (true);

    if (lchk && *lchk == cvLneedCheck) {
        bool brk = false;
        if (!check_set_layer(odesc->ldesc(), &brk))
            return (false);
        if (brk) {
            *lchk = cvLnogo;
            return (true);
        }
        *lchk = cvLok;
    }

    bool ret = true;
    if (odesc->type() == CDBOX) {
        if (out_stk) {
            BBox BB = odesc->oBB();
            Point *p;
            out_stk->TBB(&BB, &p);
            if (p) {
                Poly po(5, p);
                ret = write_poly(&po);
                delete [] p;
            }
            else
                ret = write_box(&BB);
        }
        else
            ret = write_box(&odesc->oBB());
    }
    else if (odesc->type() == CDPOLYGON) {
        if (out_stk) {
            int num = ((const CDpo*)odesc)->numpts();
            Poly po(num, Point::dup(((const CDpo*)odesc)->points(), num));
            out_stk->TPath(po.numpts, po.points);
            ret = write_poly(&po);
            delete [] po.points;
        }
        else {
            const Poly po(((const CDpo*)odesc)->po_poly());
            ret = write_poly(&po);
        }
    }
    else if (odesc->type() == CDWIRE) {
        if (out_stk) {
            int num = ((const CDw*)odesc)->numpts();
            Wire w(num, Point::dup(((const CDw*)odesc)->points(), num),
                ((const CDw*)odesc)->attributes());
            w.set_wire_width(mmRnd(w.wire_width() * out_stk->TGetMagn()));
            out_stk->TPath(w.numpts, w.points);
            ret =  write_wire(&w);
            delete [] w.points;
        }
        else {
            const Wire w(((const CDw*)odesc)->w_wire());
            ret = write_wire(&w);
        }
    }
    else if (odesc->type() == CDLABEL) {
        Label la(((CDla*)odesc)->la_label());
        Text text;
        text.set(&la, Physical, Fnone);
        if (out_stk)
            text.transform(out_stk);
        ret =  write_text(&text);
    }
    return (ret);
}


// Back end function to process boxes.
//
bool
rf_out::write_box(const BBox *BB)
{
    bool ret = true;
    BBox nBB = *BB;
    if (rf_noAOI || nBB.intersect(&rf_AOI, false)) {
        if (rf_clip) {
            if (nBB.left < rf_AOI.left)
                nBB.left = rf_AOI.left;
            if (nBB.bottom < rf_AOI.bottom)
                nBB.bottom = rf_AOI.bottom;
            if (nBB.right > rf_AOI.right)
                nBB.right = rf_AOI.right;
            if (nBB.top > rf_AOI.top)
                nBB.top = rf_AOI.top;
        }
        if (rf_backend) {
            ret = rf_backend->write_box(&nBB);
            if (!ret)
                out_interrupted = rf_backend->aborted();
        }
        else {
            CDo *newo;
            CDerrType err = rf_targcell->makeBox(rf_targld, &nBB, &newo);
            if (err != CDok)
                ret = (err == CDbadBox);
            if (newo)
                newo->set_flag(rf_targld->getStrmDatatypeFlags(rf_targdt));
        }
    }
    return (ret);
}


// Back end function to process polygons.
//
bool
rf_out::write_poly(const Poly *po)
{
    bool ret = true;
    if (rf_noAOI || po->intersect(&rf_AOI, false)) {
        bool need_out = true;
        if (rf_clip) {
            need_out = false;
            PolyList *pl = po->clip(&rf_AOI, &need_out);
            for (PolyList *px = pl; px; px = px->next) {
                if (rf_backend) {
                    ret = rf_backend->write_poly(&px->po);
                    if (!ret) {
                        out_interrupted = rf_backend->aborted();
                        break;
                    }
                }
                else {
                    CDpo *newo;
                    CDerrType err =
                        rf_targcell->makePolygon(rf_targld, &px->po, &newo);
                    if (err != CDok) {
                        ret = (err == CDbadPolygon);
                        break;
                    }
                    if (newo)
                        newo->set_flag(
                            rf_targld->getStrmDatatypeFlags(rf_targdt));
                }
            }
            PolyList::destroy(pl);
        }
        if (need_out) {
            if (rf_backend) {
                ret = rf_backend->write_poly(po);
                if (!ret)
                    out_interrupted = rf_backend->aborted();
            }
            else {
                CDpo *newo;
                Poly poly(po->numpts, Point::dup(po->points, po->numpts));
                CDerrType err =
                    rf_targcell->makePolygon(rf_targld, &poly, &newo);
                if (err != CDok)
                    ret = (err == CDbadPolygon);
                if (newo)
                    newo->set_flag(rf_targld->getStrmDatatypeFlags(rf_targdt));
            }
        }
    }
    return (ret);
}


// Back end function to process wires.
//
bool
rf_out::write_wire(const Wire *w)
{
    Poly wp;
    bool ret = true;
    if (rf_backend && rf_backend->wire_to_poly()) {
        if (w->toPoly(&wp.points, &wp.numpts) && (rf_noAOI ||
                wp.intersect(&rf_AOI, false))) {
            bool need_out = true;
            if (rf_clip) {
                need_out = false;
                PolyList *pl = wp.clip(&rf_AOI, &need_out);
                for (PolyList *px = pl; px; px = px->next) {
                    ret = rf_backend->write_poly(&px->po);
                    if (!ret) {
                        out_interrupted = rf_backend->aborted();
                        break;
                    }
                }
                PolyList::destroy(pl);
            }
            if (need_out) {
                ret = rf_backend->write_poly(&wp);
                if (!ret)
                    out_interrupted = rf_backend->aborted();
            }
        }
    }
    else if (rf_noAOI || (w->toPoly(&wp.points, &wp.numpts) &&
            wp.intersect(&rf_AOI, false))) {
        bool need_out = true;
        if (rf_clip) {
            need_out = false;
            PolyList *pl = wp.clip(&rf_AOI, &need_out);
            for (PolyList *px = pl; px; px = px->next) {
                if (rf_backend) {
                    ret = rf_backend->write_poly(&px->po);
                    if (!ret) {
                        out_interrupted = rf_backend->aborted();
                        break;
                    }
                }
                else {
                    CDpo *newo;
                    CDerrType err = rf_targcell->makePolygon(rf_targld,
                        &px->po, &newo);
                    if (err != CDok) {
                        ret = (err == CDbadPolygon);
                        break;
                    }
                    if (newo)
                        newo->set_flag(
                            rf_targld->getStrmDatatypeFlags(rf_targdt));
                }
            }
            PolyList::destroy(pl);
        }
        if (need_out) {
            if (rf_backend) {
                ret = rf_backend->write_wire(w);
                if (!ret)
                    out_interrupted = rf_backend->aborted();
            }
            else {
                Wire wire(w->numpts, Point::dup(w->points, w->numpts),
                    w->attributes);

                Point *pres = 0;
                int nres = 0;
                for (;;) {
                    wire.checkWireVerts(&pres, &nres);

                    CDw *newo;
                    CDerrType err = rf_targcell->makeWire(rf_targld, &wire,
                        &newo);
                    if (err != CDok)
                        ret = (err == CDbadWire);
                    else if (newo)
                        newo->set_flag(
                            rf_targld->getStrmDatatypeFlags(rf_targdt));
                    if (!pres)
                        break;
                    wire.points = pres;
                    wire.numpts = nres;
                }
            }
        }
    }
    delete [] wp.points;
    return (ret);
}


bool
rf_out::write_text(const Text *text)
{
    if (!rf_noAOI) {
        BBox tBB(text->x, text->y, text->x + text->width,
            text->y + text->height);
        Label::TransformLabelBB(text->xform, &tBB, 0);
        if (!tBB.intersect(&rf_AOI, false))
            return (true);
        if (rf_clip &&
                (tBB.left < rf_AOI.left || tBB.bottom < rf_AOI.bottom ||
                tBB.right > rf_AOI.right || tBB.top > rf_AOI.top)) {
            return (true);
        }
    }

    bool ret = true;
    if (rf_backend) {
        ret = rf_backend->write_text(text);
        if (!ret)
            out_interrupted = rf_backend->aborted();
    }
    else {
        Label la(0, text->x, text->y, text->width, text->height, text->xform);
        la.label = new hyList(rf_targcell, text->text, HYcvAscii);
        CDla *newo;
        CDerrType err = rf_targcell->makeLabel(rf_targld, &la, &newo);
        if (err != CDok)
            ret = (err == CDbadLabel);
        if (newo)
            newo->set_flag(rf_targld->getStrmDatatypeFlags(rf_targdt));
    }
    return (ret);
}


// This can be used to keep standard vias, pcells, etc.  as instances
// when flattening.  Not presently supported by backend interface, so
// only applies when writing to memory.
//
bool
rf_out::write_sref(const Instance *inst)
{
    bool ret = true;
    if (!rf_backend) {
        CDtx tx(inst->reflection, inst->ax, inst->ay,
            inst->origin.x, inst->origin.y, inst->magn);
        CDap ap(inst->nx, inst->ny, inst->dx, inst->dy);
        CDcellName cname = CD()->CellNameTableAdd(inst->name);
        CallDesc calldesc(cname, 0);

        CDc *newo;
        if (rf_targcell->makeCall(&calldesc, &tx, &ap, CDcallDb, &newo)
                != OIok)
            ret = false;
        if (ret && newo && out_prpty) {
            newo->set_prpty_list(out_prpty);
            out_prpty = 0;
        }
    }
    return (ret);
}
// End of rf_out functions

