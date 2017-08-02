
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
#include "fio_cif.h"
#include "fio_cvt_base.h"
#include "fio_gencif.h"
#include "cd_hypertext.h"
#include "cd_propnum.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "cd_chkintr.h"
#include "pathlist.h"
#include "filestat.h"
#include <errno.h>


// Write the entire heirarchy under cellname to native files in
// destination, with possible scaling.  If destination is null, use
// the CWD.
// Return values:
//  OIok        normal return
//  OIerror     unspecified error (message in Errs)
//  OIabort     user interrupt
//
OItype
cFIO::ToNative(const char *cellname, const FIOcvtPrms *prms)
{
    if (!cellname)
        return (OIerror);
    bool allcells = !strcmp(cellname, FIO_CUR_SYMTAB);

    CDcbin cbin;
    if (!allcells && !CDcdb()->findSymbol(cellname, &cbin))
        return (OIerror);

    xic_out *xic = new xic_out(0);
    const char *destination = prms->destination();
    if (destination && *destination) {
        if (!allcells && prms->flatten())
            xic->set_destination_flat(destination);
        else if (!xic->set_destination(destination)) {
            delete xic;
            return (OIerror);
        }
    }
    xic->assign_alias(NewWritingAlias(prms->alias_mask(), false));

    bool ok = true;
    if (allcells) {
        // Dump all applicable cells found in the current symbol table.

        CDgenTab_cbin gen;
        while (gen.next(&cbin)) {
            if (cbin.isLibrary() && cbin.isDevice())
                continue;

            CDs *sdesc = cbin.phys();
            if (sdesc)
                ok = xic->write_this_cell(sdesc,
                    Tstring(sdesc->cellname()), prms->scale());
            sdesc = cbin.elec();
            if (ok && sdesc && !sdesc->isEmpty()) {
                FIO()->ifUpdateNodes(sdesc);
                ok = xic->write_this_cell(sdesc,
                    Tstring(sdesc->cellname()), prms->scale());
            }
            if (!ok)
                break;
        }
    }
    else {
        const BBox *AOI = prms->use_window() ? prms->window() : 0;
        if (prms->flatten())
            ok = xic->write_flat(cellname, prms->scale(), AOI, prms->clip());
        else {
            CDgenHierDn_cbin gen(&cbin);
            CDcbin ncbin;
            bool err;
            while (gen.next(&ncbin, &err)) {
                if (ncbin.isLibrary() && ncbin.isDevice())
                    continue;

                CDs *sdesc = ncbin.phys();
                if (sdesc)
                    ok = xic->write_this_cell(sdesc,
                        Tstring(sdesc->cellname()), prms->scale());
                sdesc = ncbin.elec();
                if (ok && sdesc && !sdesc->isEmpty()) {
                    FIO()->ifUpdateNodes(sdesc);
                    ok = xic->write_this_cell(sdesc,
                        Tstring(sdesc->cellname()), prms->scale());
                }
                if (!ok)
                    break;
            }
            if (err)
                ok = false;
        }
    }
    OItype oiret = ok ? OIok : xic->was_interrupted() ? OIaborted : OIerror;
    if (oiret == OIok && !prms->flatten()) {
        // Create a library file for the files.
        xic->make_native_lib(cellname, prms->destination());
    }
    delete xic;
    return (oiret);
}


// Update a symbol to a file, as sname if sname is given and the cell is
// not immutable.  Both physical and electrical data are updated from
// memory.  A backup file is produced, if an overwrite occurs.
//
bool
cFIO::WriteNative(CDcbin *cbin, const char *sname)
{
    if (!cbin)
        return (true);
    char *tmpstr = 0;
    if (!sname || !*sname)
        sname = Tstring(cbin->cellname());
    else if (cbin->isImmutable()) {
        // We cant't change the name of an immutable cell, but we want to
        // use the given directory path, if any.
        const char *cngiven = lstring::strip_path(sname);
        const char *cname = Tstring(cbin->cellname());
        if (cngiven == sname)
            // No path given.
            sname = cname;
        else if (strcmp(cngiven, cname)) {
            // There was a path, and the cell name component differs.
            int n = cngiven - sname;
            tmpstr = new char[n + strlen(cname) + 1];
            memcpy(tmpstr, sname, n);
            strcpy(tmpstr + n, cname);
            sname = tmpstr;
        }
    }
    GCarray<char*> gc_tmpstr(tmpstr);

    if (!filestat::create_bak(sname)) {
        Errs()->add_error(filestat::error_msg());
        return (false);
    }
    FILE *fp = fopen(sname, "w");
    if (!fp) {
        Errs()->add_error("Unable to open %s.", sname);
        return (false);
    }

    // strip path
    sname = lstring::strip_path(sname);

    xic_out *xic = new xic_out(0);
    xic->set_file_ptr(fp);

    // Native cells can have a path to an archive file as the instance
    // name, i.e.
    // 9 /path/to/some/file.gds;
    // C 0 ...;
    // in which case, when reading into the database, the archive will
    // be opened and the instance will reference the top-level cell in
    // the archive.  We save the cellname -> path association in a
    // table, and check it here when writing a single cell only (not
    // when writing a full hierarchy, which would write the cell files
    // from the archive.
    //
    if (fioNativeImportTab) {
        // Create an alias table containing the associations, this
        // will perform the name change in write_sref.

        FIOaliasTab *alias = new FIOaliasTab(true, true);
        SymTabGen gen(fioNativeImportTab);
        SymTabEnt *h, *h0 = SymTab::get_ent(fioNativeImportTab, sname);
        while ((h = gen.next()) != 0) {
            if (h != h0)
                // Never alias the current cell name!
                alias->set_alias(h->stTag, (char*)h->stData);
        }
        xic->assign_alias(alias);
    }

    bool ret = true;
    CDs *sdesc = cbin->phys();
    if (sdesc)
        ret = xic->write_this_cell(sdesc, sname, 1.0);
    else
        xic->write_dummy(sname, Physical);

    if (ret) {
        sdesc = cbin->elec();
        if (sdesc) {
            if (!sdesc->isEmpty()) {
                FIO()->ifUpdateNodes(sdesc);
                ret = xic->write_this_cell(sdesc, sname, 1.0);
            }
            else {
                BBox BB(*sdesc->BB());

                // Expand BB to enclose all terminals, needed only for
                // virtual and bus terminals.
                CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
                for ( ; pn; pn = pn->next()) {
                    int x, y;
                    pn->get_schem_pos(&x, &y);
                    BB.add(x, y);
                }
                CDp_bsnode *pb = (CDp_bsnode*)sdesc->prpty(P_BNODE);
                for ( ; pb; pb = pb->next()) {
                    int x, y;
                    pb->get_schem_pos(&x, &y);
                    BB.add(x, y);
                }
                sdesc->setBB(&BB);
                sdesc->setBBvalid(true);
                sdesc->setSaventv(false);
                sdesc->clearModified();
            }
        }
    }
    xic->set_file_ptr(0);
    delete xic;
    return (ret);
}


// Update sdesc to a symbol file.
// If sfile == 0, update to file sdesc->cellname()
// Returns true if success, else returns false.
//
bool
cFIO::WriteFile(CDs *sdesc, const char *sfile, FILE *fp)
{
    if (!sdesc)
        return (true);

    CDs *tsd = sdesc->owner();
    if (tsd)
        sdesc = tsd;

    if (!sfile || !*sfile) {
        sfile = Tstring(sdesc->cellname());
        if (!sfile || !*sfile) {
            Errs()->add_error("Null or empty cell name encountered.");
            return (false);
        }
    }
    else
        sfile = lstring::strip_path(sfile);
    FIO()->ifUpdateNodes(sdesc);
    xic_out *xic = new xic_out(0);
    xic->set_file_ptr(fp);
    xic->set_lib_device(sdesc->isLibrary() && sdesc->isDevice());

    bool ret = xic->write_this_cell(sdesc, sfile, 1.0);
    delete xic;
    return (ret);
}


// Write the contents of the object list to a native cell file, physical
// only.
//
bool
cFIO::ListToNative(CDol *olist, const char *xic_fname)
{
    unsigned tmp = fioStripForExport;
    fioStripForExport = false;
    double scale = WriteScale();
    SetWriteScale(1.0);

    bool nogo = false;
    cif_out *cif = new cif_out(0);
    if (!filestat::create_bak(xic_fname)) {
        Errs()->add_error(filestat::error_msg());
        nogo = true;
    }
    if (!nogo && !cif->set_destination(xic_fname)) {
        Errs()->add_error("set_distination failed");
        nogo = true;
    }
    if (!nogo && !cif->begin_native(lstring::strip_path(xic_fname))) {
        Errs()->add_error("begin_native failed");
        nogo = true;
    }
    if (!nogo) {
        int lindex = 0;
        CDlgen lgen(FIO()->ifCurrentDisplayMode(), CDlgen::BotToTopWithCells);
        CDl *ld;
        while ((ld = lgen.next()) != 0 && !nogo) {
            ld->setTmpSkip(false);
            lindex++;

            cvLchk lchk = cvLneedCheck;
            for (CDol *o = olist; o; o = o->next) {
                if (o->odesc->ldesc() != ld)
                    continue;
                if (o->odesc->type() == CDINSTANCE) {
                    CDc *cdesc = (CDc*)o->odesc;
                    if (!cif->queue_properties(cdesc)) {
                        Errs()->add_error("queue properties failed");
                        nogo = true;
                        break;
                    }

                    Instance inst;
                    inst.name = Tstring(cdesc->cellname());
                    if (!inst.name) {
                        Errs()->add_error("null cell name encountered");
                        nogo = true;
                        break;
                    }
                    if (!inst.set(cdesc)) {
                        Errs()->add_error("instance setup failed");
                        nogo = true;
                        break;
                    }
                    if (!cif->write_sref(&inst)) {
                        Errs()->add_error("write_sref failed");
                        nogo = true;
                        break;
                    }
                    cif->clear_property_queue();
                }
                else {
                    if (!cif->queue_properties(o->odesc)) {
                        Errs()->add_error("queue_properties failed");
                        nogo = true;
                        break;
                    }
                    if (!cif->write_object(o->odesc, &lchk)) {
                        Errs()->add_error("write_object failed");
                        nogo = true;
                        break;
                    }
                    cif->clear_property_queue();
                    if (lchk == cvLnogo)
                        break;
                }
            }
        }
    }
    if (!nogo && !cif->end_native()) {
        Errs()->add_error("end_native failed");
        nogo = true;
    }

    delete cif;
    fioStripForExport = tmp;
    SetWriteScale(scale);
    return (!nogo);
}


// Write the contents of the table to a native cell file.  The table
// contains CDol lists of objects, keyed by the layer desc as an
// unsigned long.  Output is physical mode, objects only, no subcells.
//
bool
cFIO::TabToNative(SymTab *tab, const char *xic_fname)
{
    unsigned tmp = fioStripForExport;
    fioStripForExport = false;
    double scale = WriteScale();
    SetWriteScale(1.0);

    bool nogo = false;
    cif_out *cif = new cif_out(0);
    if (!filestat::create_bak(xic_fname)) {
        Errs()->add_error(filestat::error_msg());
        nogo = true;
    }
    if (!nogo && !cif->set_destination(xic_fname)) {
        Errs()->add_error("set_distination failed");
        nogo = true;
    }
    if (!nogo && !cif->begin_native(lstring::strip_path(xic_fname))) {
        Errs()->add_error("begin_native failed");
        nogo = true;
    }
    if (!nogo) {
        int lindex = 0;
        CDlgen lgen(Physical);
        CDl *ld;
        while ((ld = lgen.next()) != 0 && !nogo) {
            ld->setTmpSkip(false);
            lindex++;

            CDol *ol0 = (CDol*)SymTab::get(tab, (unsigned long)ld);
            if (ol0 == (CDol*)ST_NIL)
                continue;

            cvLchk lchk = cvLneedCheck;
            for (CDol *ol = ol0; ol; ol = ol->next) {
                CDo *odesc = ol->odesc;
                if (!cif->queue_properties(odesc)) {
                    Errs()->add_error("queue_properties failed");
                    nogo = true;
                    break;
                }
                if (!cif->write_object(odesc, &lchk)) {
                    Errs()->add_error("write_object failed");
                    nogo = true;
                    break;
                }
                cif->clear_property_queue();
                if (lchk == cvLnogo)
                    break;
            }
        }
    }
    if (!nogo && !cif->end_native()) {
        Errs()->add_error("end_native failed");
        nogo = true;
    }

    delete cif;
    fioStripForExport = tmp;
    SetWriteScale(scale);
    return (!nogo);
}
// End of cFIO functions


xic_out::xic_out(const char *infilename)
{
    out_filetype = Fnative;
    out_symfp = 0;
    out_rootfp = 0;
    out_infilename = lstring::copy(infilename);
    out_destdir = 0;
    out_destpath = 0;
    out_written = 0;
    out_last_offset = 0;
    out_lname_temp = 0;
    out_fp_set = false;
    out_lib_device = false;
    out_layer_written = true;
}


xic_out::~xic_out()
{
    clear_property_queue();
    if (out_symfp && !out_fp_set)
        fclose(out_symfp);
    if (out_rootfp)
        fclose(out_rootfp);
    delete [] out_infilename;
    delete [] out_destdir;
    delete [] out_destpath;
    delete out_written;
    delete [] out_lname_temp;
}


void
xic_out::set_file_ptr(FILE *fp)
{
    if (out_symfp)
        fclose(out_symfp);
    out_symfp = fp;
    out_fp_set = out_symfp;
}


bool
xic_out::write_this_cell(CDs *sdesc, const char *sfile, double tscale)
{
    if (sdesc->isPCellSubMaster() && !sdesc->isPCellReadFromFile() &&
            !FIO()->IsKeepPCellSubMasters())
        return (true);
    if (sdesc->isViaSubMaster() && !FIO()->IsKeepViaSubMasters())
        return (true);

    out_mode = sdesc->displayMode();
    if (out_mode == Physical) {
        out_scale = dfix(tscale);
        out_needs_mult = (out_scale != 1.0);
        out_phys_scale = out_scale;
    }
    else {
        out_scale = 1.0;
        out_needs_mult = false;
    }

    time_t tloc = time(0);
    tm *date = gmtime(&tloc);
    bool ret = queue_properties(sdesc);
    if (ret)
        ret = write_struct(sfile, date, date);
    clear_property_queue();

    // Note:  it is important for electrical mode that instances be
    // written before geometry (really just labels).  See
    // CDs::prptyLabelPatch().

    if (ret)
        ret = write_instances(sdesc);
    if (ret)
        ret = write_geometry(sdesc);
    if (ret)
        ret = write_end_struct();

    if (ret) {
        // Update the cell bounding box, which was recomputed during
        // the write.
        BBox BB(*cellBB());

        if (sdesc->isElectrical()) {
            // Expand BB to enclose all terminals, needed only for
            // virtual and bus terminals.
            CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                int x, y;
                pn->get_schem_pos(&x, &y);
                BB.add(x, y);
            }
            CDp_bsnode *pb = (CDp_bsnode*)sdesc->prpty(P_BNODE);
            for ( ; pb; pb = pb->next()) {
                int x, y;
                pb->get_schem_pos(&x, &y);
                BB.add(x, y);
            }
        }
        sdesc->setBB(&BB);
        sdesc->setBBvalid(true);
    }
    sdesc->setSaventv(false);
    sdesc->clearModified();
    return (ret);
}


bool
xic_out::check_for_interrupt()
{
    out_rec_count++;
    if (!(out_rec_count % 16)) {
        long sx = ftell(out_symfp);
        out_byte_count += sx - out_last_offset;
        out_last_offset = sx;
        if (out_byte_count > out_fb_thresh) {
            if (out_fb_thresh >= 1000000) {
                if (!(out_fb_thresh % 1000000))
                    FIO()->ifInfoMessage(IFMSG_WR_PGRS,
                        "Wrote %d Mb", (int)(out_fb_thresh/1000000));
            }
            else
                FIO()->ifInfoMessage(IFMSG_WR_PGRS,
                    "Wrote %d Kb", (int)(out_fb_thresh/1000));
            out_fb_thresh += UFB_INCR;
            if (checkInterrupt("Interrupt received, abort translation? ")) {
                Errs()->add_error("user interrupt");
                out_interrupted = true;
                return (false);
            }
        }
    }
    return (true);
}


bool
xic_out::write_header(const CDs*)
{
    return (true);
}


// Write an object, properties are already queued.
//
bool
xic_out::write_object(const CDo *odesc, cvLchk *lchk)
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
            BBox BB(odesc->oBB());
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
        Text text;
        // use long text for unbound labels
        CDp_lref *prf = (CDp_lref*)odesc->prpty(P_LABRF);
        text.text = hyList::string(((CDla*)odesc)->label(), HYcvAscii,
            !(prf && prf->devref()));
        const Label la(((const CDla*)odesc)->la_label());
        ret = text.set(&la, out_mode, Fnative);
        if (ret) {
            if (out_stk)
                text.transform(out_stk);
            ret = write_text(&text);
        }
        delete [] text.text;
    }
    return (ret);
}


bool
xic_out::set_destination(const char *destination)
{
    GFTtype rt = filestat::get_file_type(destination);
    if (rt == GFT_NONE) {
        Errs()->add_error("%s: %s.", destination, strerror(errno));
        return (false);
    }
    if (rt != GFT_DIR) {
        Errs()->add_error("Destination %s is not a directory.",
            destination);
        return (false);
    }
    if (access(destination, W_OK)) {
        Errs()->add_error("No write permission for directory %s.",
            destination);
        return (false);
    }
    out_destdir = lstring::copy(destination);
    return (true);
}


bool
xic_out::queue_property(int val, const char *string)
{
    if (prpty_reserved(val) || prpty_pseudo(val))
        // these are never exported
        return (true);

    CDp *p = new CDp(string, val);
    if (!out_prpty)
        out_prpty = p;
    else {
        CDp *px = out_prpty;
        while (px->next_prp())
            px = px->next_prp();
        px->set_next_prp(p);
    }
    return (true);
}


bool
xic_out::open_library(DisplayMode mode, double sc)
{
    out_mode = mode;
    out_scale = dfix(sc);
    out_needs_mult = (out_scale != 1.0);
    if (mode == Physical)
        out_phys_scale = sc;

    clear_property_queue();
    out_in_struct = false;
    out_symnum = 0;
    if (out_visited)
        out_visited->clear();
    out_layer.set_null();
    out_layer_written = true;

    if (out_symfp && !out_fp_set) {
        fclose(out_symfp);
        out_symfp = 0;
    }
    return (true);
}


bool
xic_out::write_library(int, double, double, tm*, tm*, const char*)
{
    return (true);
}


bool
xic_out::write_struct(const char *name, tm *cdate, tm *mdate)
{
    if (out_in_struct) {
        if (!write_end_struct())
            return (false);
    }
    out_in_struct = true;
    out_cellBB = CDnullBB;
    out_layer.set_null();
    out_layer_written = true;

    if (out_destpath)
        name = lstring::strip_path(out_destpath);
    else
        name = alias(name);

    if (out_mode == Physical) {
        if (!out_fp_set) {
            if (!fopen_tof(name, "w")) {
                Errs()->add_error("Unable to open %s.", name);
                return (false);
            }
        }
        fprintf(out_symfp, "(Symbol %s);\n", name);
        if (out_infilename) {
            char *gm = lstring::copy(asctime(cdate));
            char *s = strchr(gm, '\n');
            if (s)
                *s = 0;
            fprintf(out_symfp, "(Translated from %s, %.24s GMT);\n",
                out_infilename, gm);
            delete [] gm;
        }
        fprintf(out_symfp, "(%cId:%c);\n", '$', '$');
        fprintf(out_symfp, "(%s);\n", CD()->ifIdString());
        fprintf(out_symfp, "(PHYSICAL);\n");
    }
    else if (out_mode == Electrical) {
        if (!out_fp_set) {
            if (!fopen_tof(name, "a")) {
                Errs()->add_error("Unable to open %s.", name);
                return (false);
            }
        }
        if (out_lib_device)
            fprintf(out_symfp, "(Symbol %s);\n", name);
        else
            fprintf(out_symfp, "(ELECTRICAL);\n");
    }
    else
        return (false);

    if (out_mode == Physical) {
        if (CDphysResolution != 100)
            fprintf(out_symfp, "(RESOLUTION %d);\n", CDphysResolution);
    }
    else {
        if (CDelecResolution != 100)
            fprintf(out_symfp, "(RESOLUTION %d);\n", CDelecResolution);
    }

    if (out_mode == Physical)
        fprintf(out_symfp,
            "( CREATED %d/%d/%d %d:%d:%d, MODIFIED %d/%d/%d %d:%d:%d );\n",
            cdate->tm_mon + 1, cdate->tm_mday, cdate->tm_year + 1900,
            cdate->tm_hour, cdate->tm_min, cdate->tm_sec,
            mdate->tm_mon + 1, mdate->tm_mday, mdate->tm_year + 1900,
            mdate->tm_hour, mdate->tm_min, mdate->tm_sec);

    for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
        if (pd->value() == XICP_CHD_REF) {
            sChdPrp prp(pd->string());
            prp.scale_bb(out_phys_scale);
            char *newstr = prp.compose();
            Gen.Property(out_symfp, pd->value(), newstr);
            delete [] newstr;
            continue;
        }
        CDp *px = pd->dup();
        if (px) {
            px->scale(out_scale, out_phys_scale, out_mode);
            Gen.Property(out_symfp, px->value(), px->string());
            delete px;
        }
    }

    fprintf(out_symfp, "9 %s;\n", name);
    Gen.BeginSymbol(out_symfp, 0, 1, 1);
    return (ferror(out_symfp) == 0);
}


bool
xic_out::write_end_struct(bool)
{
    bool ret = true;
    if (out_in_struct) {
        Gen.EndSymbol(out_symfp);
        Gen.End(out_symfp);
        ret = (ferror(out_symfp) == 0);
        if (!out_fp_set) {
            fclose(out_symfp);
            out_symfp = 0;
        }
        out_in_struct = false;
    }
    return (ret);
}


bool
xic_out::queue_layer(const Layer *layer, bool*)
{
    if (layer->name &&
            (!out_layer.name || strcmp(out_layer.name, layer->name))) {

        delete [] out_lname_temp;
        out_lname_temp = lstring::copy(layer->name);
        out_layer.name = out_lname_temp;
        out_layer_written = false;
    }
    return (true);
}


bool
xic_out::write_box(const BBox *BB)
{
    if (!check_for_interrupt())
        return (false);
    if (!write_layer_rec())
        return (false);

    FILE *fp = out_symfp;
    if (!out_in_struct) {
        if (!open_root())
            return (false);
        fp = out_rootfp;
    }

    for (CDp *pd = out_prpty; pd; pd = pd->next_prp())
        Gen.Property(fp, pd->value(), pd->string());

    if (out_needs_mult) {
        BBox tBB(scale(BB->left), scale(BB->bottom),
            scale(BB->right), scale(BB->top));
        Gen.Box(fp, tBB);
    }
    else
        Gen.Box(fp, *BB);
    return (ferror(fp) == 0);
}


bool
xic_out::write_poly(const Poly *po)
{
    if (!check_for_interrupt())
        return (false);
    if (!write_layer_rec())
        return (false);

    FILE *fp = out_symfp;
    if (!out_in_struct) {
        if (!open_root())
            return (false);
        fp = out_rootfp;
    }

    for (CDp *pd = out_prpty; pd; pd = pd->next_prp())
        Gen.Property(fp, pd->value(), pd->string());

    if (out_needs_mult) {
        Point *p = po->points;
        Point *points = new Point[po->numpts];
        for (int i = 0; i < po->numpts; i++)
            points[i].set(scale(p[i].x), scale(p[i].y));
        Gen.Polygon(fp, points, po->numpts);
        delete [] points;
    }
    else
        Gen.Polygon(fp, po->points, po->numpts);
    return (ferror(fp) == 0);
}


bool
xic_out::write_wire(const Wire *wire)
{
    if (!check_for_interrupt())
        return (false);
    if (!write_layer_rec())
        return (false);

    FILE *fp = out_symfp;
    if (!out_in_struct) {
        if (!open_root())
            return (false);
        fp = out_rootfp;
    }

    for (CDp *pd = out_prpty; pd; pd = pd->next_prp())
        Gen.Property(fp, pd->value(), pd->string());

    if (out_needs_mult) {
        Point *p = wire->points;
        Point *points = new Point[wire->numpts];
        for (int i = 0; i < wire->numpts; i++)
            points[i].set(scale(p[i].x), scale(p[i].y));
        int width = scale(wire->wire_width());
        Gen.Wire(fp, width, wire->wire_style(), points, wire->numpts);
        delete [] points;
    }
    else {
        Gen.Wire(fp, wire->wire_width(), wire->wire_style(), wire->points,
            wire->numpts);
    }
    return (ferror(fp) == 0);
}


bool
xic_out::write_text(const Text *text)
{
    if (!check_for_interrupt())
        return (false);

    if (!text->text || !*text->text)
        return (true);
    if (!write_layer_rec())
        return (false);

    FILE *fp = out_symfp;
    if (!out_in_struct) {
        if (!open_root())
            return (false);
        fp = out_rootfp;
    }

    for (CDp *pd = out_prpty; pd; pd = pd->next_prp())
        Gen.Property(fp, pd->value(), pd->string());
    fputs("94 <<", fp);
    if (out_mode == Electrical && out_needs_mult) {
        char *string = lstring::copy(text->text);
        string = hyList::hy_scale(string, out_scale);
        fputs(string, fp);
        delete [] string;
    }
    else
        fputs(text->text, fp);
    if (text->width <= 0)
        fprintf(fp, ">> %d %d %d;\n", scale(text->x), scale(text->y),
            text->xform);
    else
        fprintf(fp, ">> %d %d %d %d %d;\n", scale(text->x), scale(text->y),
            text->xform, scale(text->width), scale(text->height));
    return (ferror(fp) == 0);
}


bool
xic_out::write_sref(const Instance *inst)
{
    if (!check_for_interrupt())
        return (false);

    FILE *fp = out_symfp;
    if (!out_in_struct) {
        if (!open_root())
            return (false);
        fp = out_rootfp;
    }
    const char *cellname = alias(inst->name);

    // add a comment, start of instance
    fprintf(fp, "(SymbolCall %s);\n", cellname);

    // add property list info
    for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
        CDp *px = pd->dup();
        if (px) {
            px->scale(out_scale, out_phys_scale, out_mode);
            Gen.Property(fp, px->value(), px->string());
            delete px;
        }
    }
    // add bounding box extension, if possible
    if (inst->cdesc)
        fprintf(fp, "1 Bound %d %d %d %d;\n",
            inst->cdesc->oBB().left, inst->cdesc->oBB().bottom,
            inst->cdesc->oBB().right, inst->cdesc->oBB().top);

    // add Magnification extension
    if (inst->magn != 1.0)
        fprintf(fp, "1 Magnify %.16f;\n", inst->magn);

    // add Array extension
    if (inst->nx > 1 || inst->ny > 1)
        fprintf(fp, "1 Array %d %d %d %d;\n", inst->nx, scale(inst->dx),
            inst->ny, scale(inst->dy));

    if (inst->angle != 0.0)
        fprintf(fp, "( Rotate %g );\n", inst->angle);

    fprintf(fp, "9 %s;\n", cellname);
    Gen.BeginCall(fp, 0);
    Gen.MirrorY(fp, inst->reflection);
    Gen.Rotation(fp, inst->ax, inst->ay);
    Gen.Translation(fp, scale(inst->origin.x), scale(inst->origin.y));
    Gen.EndCall(fp);

    return (ferror(fp) == 0);
}


bool
xic_out::write_endlib(const char*)
{
    if (out_in_struct) {
        if (!write_end_struct())
            return (false);
    }
    return (true);
}


bool
xic_out::write_info(Attribute *at, const char *propfile)
{
    //
    // Write the header stuff as CIF properties to a special file.
    //
    char buf[256];
    if (out_destdir)
        sprintf(buf, "%s/%s", out_destdir, propfile);
    else
        strcpy(buf, propfile);
    if (!filestat::create_bak(buf)) {
        Errs()->add_error(filestat::error_msg());
        Errs()->add_error("Unable to open %s.", buf);
        return (false);
    }
    FILE *fp = fopen(buf, "w");
    if (!fp) {
        Errs()->add_error("Unable to open %s.", buf);
        return (false);
    }

    fprintf(fp, "(Root symbol properties from GDSII header);\n");
    time_t now = time(0);
    struct tm *tgmt = gmtime(&now);
    char *gm = lstring::copy(asctime(tgmt));
    char *s = strchr(gm, '\n');
    if (s)
        *s = 0;
    fprintf(fp, "(Translated from %s, %.24s GMT);\n", out_infilename, gm);
    delete [] gm;

    // add the symbol property list
    for (Attribute *sp = at; sp; sp = sp->next()) {
        if (sp->pComment)
            fprintf(fp, "%s;\n", sp->pComment);
        Gen.Property(fp, sp->value(), sp->string());
    }
    fclose(fp);
    return (true);
}


// A special set_destination for use when writing to a single cell file,
// as when flattening.  The cell name will be the last component if a
// path is given.
//
bool
xic_out::set_destination_flat(const char *fname)
{
    // If not set, will use top cell name, in current directory.
    if (fname && *fname)
        out_destpath = lstring::copy(fname);
    return (true);
}


// Write an empty cell
//
void
xic_out::write_dummy(const char *cellname, DisplayMode mode)
{
    char tbuf[256];
    sprintf(tbuf, "Symbol %s", cellname);
    Gen.Comment(out_symfp, tbuf);
    if (out_infilename) {
        time_t now = time(0);
        struct tm *tgmt = gmtime(&now);
        char *gm = lstring::copy(asctime(tgmt));
        char *s = strchr(gm, '\n');
        if (s)
            *s = 0;
        sprintf(tbuf, "Translated from %s, %.24s GMT", out_infilename, gm);
        delete [] gm;
        Gen.Comment(out_symfp, tbuf);
    }
    if (mode == Physical) {
        sprintf(tbuf, "%cId:%c", '$', '$');
        Gen.Comment(out_symfp, tbuf);
        Gen.Comment(out_symfp, "PHYSICAL");
    }
    else
        Gen.Comment(out_symfp, "ELECTRICAL");
    if (out_mode == Physical) {
        if (CDphysResolution != 100)
            fprintf(out_symfp, "(RESOLUTION %d);\n", CDphysResolution);
    }
    else {
        if (CDelecResolution != 100)
            fprintf(out_symfp, "(RESOLUTION %d);\n", CDelecResolution);
    }
    Gen.UserExtension(out_symfp, '9', cellname);
    Gen.BeginSymbol(out_symfp, 0, 1, 1);
    Gen.EndSymbol(out_symfp);
    Gen.End(out_symfp);
}


// Create a library file listing each symbol in the hierarchy as a
// reference to a native symbol file.
//
void
xic_out::make_native_lib(const char *cellname, const char *defpath)
{
    CDcbin cbin;
    if (!CDcdb()->findSymbol(cellname, &cbin))
        return;
    stringlist *s0 = 0;
    CDgenHierDn_cbin sgen(&cbin);
    CDcbin ncbin;
    bool err;
    while (sgen.next(&ncbin, &err)) {
        if (!FIO()->IsKeepPCellSubMasters() && ncbin.phys() &&
                ncbin.phys()->isPCellSubMaster())
            continue;
        if (!FIO()->IsKeepViaSubMasters() && ncbin.phys() &&
                ncbin.phys()->isViaSubMaster())
            continue;
        s0 = new stringlist(
            lstring::copy(alias(Tstring(ncbin.cellname()))), s0);
    }
    stringlist::sort(s0);
    if (!defpath || !*defpath)
        defpath = ".";
    char *path = pathlist::expand_path(defpath, true, true);
    if (!lstring::is_rooted(path)) {
        char *t = new char[strlen(path) + 3];
        t[0] = '.';
        t[1] = '/';
        strcpy(t+2, path);
        delete [] path;
        path = pathlist::expand_path(t, true, true);
        delete [] t;
    }
    char *e = path + strlen(path) - 1;
    if (lstring::is_dirsep(*e))
        *e = 0;
    for (e = path; *e; e++) {
        if (isspace(*e) || *e == PATH_SEP) {
            char *t = new char[strlen(path) + 3];
            t[0] = '"';
            strcpy(t+1, path);
            strcat(t, "\"");
            delete [] path;
            path = t;
            break;
        }
    }

    const char *cellname_alias = alias(cellname);
    char *libfilename = new char[strlen(cellname_alias) + 5];
    strcpy(libfilename, cellname_alias);
    char *tt = strrchr(libfilename, '.');
    if (tt && tt != libfilename)
        *tt = 0;
    strcat(libfilename, ".lib");
    GCarray<char*> gc_libfilename(libfilename);

    if (!filestat::create_bak(libfilename)) {
        stringlist::destroy(s0);
        delete [] path;
        FIO()->ifInfoMessage(IFMSG_POP_ERR,
            "Unable to create library file.\n%s", filestat::error_msg());
        return;
    }
    FILE *fp = fopen(libfilename, "w");
    if (!fp) {
        stringlist::destroy(s0);
        delete [] path;
        filestat::save_perror(libfilename);
        FIO()->ifInfoMessage(IFMSG_POP_ERR,
            "Unable to create library file.\n%s", filestat::error_msg());
        return;
    }
    fprintf(fp, "(Library %s);\n", libfilename);

    // list the top-level cell first, followed by the others in
    // alphabetical order
    stringlist *sp = 0, *sx = 0;
    for (stringlist *s = s0; s; s = s->next) {
        if (!strcmp(s->string, cellname_alias)) {
            if (!sp)
                s0 = s->next;
            else
                sp->next = s->next;
            sx = s;
            break;
        }
        sp = s;
    }
    if (sx) {
        sx->next = s0;
        s0 = sx;
    }

    for (stringlist *s = s0; s; s = s->next)
        fprintf(fp, "Reference %-20s %s/%s\n", s->string, path, s->string);
    delete [] path;
    stringlist::destroy(s0);
    fclose(fp);
}


// An fopen() for native symbol files taking into account the
// destination directory given.
//
// The out_writen table keeps track of files written, and unlike
// out_visited this is not cleared after writing physical before
// writing electrical.
//
bool
xic_out::fopen_tof(const char *name, const char *mode)
{
    out_last_offset = 0;
    if (out_symfp) {
        fclose(out_symfp);
        out_symfp = 0;
    }
    bool need_dummy = false;
    if (!out_written)
        out_written = new vtab_t(true);
    if (*mode == 'w') {
        if (out_written->find(name) >= 0)
            FIO()->ifPrintCvLog(IFLOG_WARN,
                "reopening and overwriting file %s.", name);
        else
            out_written->add(name, 0);
    }
    else if (*mode == 'a') {
        if (out_written->find(name) < 0) {
            out_written->add(name, 0);
            need_dummy = true;
        }
    }

    char buf[512];
    if (out_destdir)
        sprintf(buf, "%s/%s", out_destdir, name);
    else if (out_destpath)
        strcpy(buf, out_destpath);
    else
        strcpy(buf, name);
    if ((*mode == 'w' || need_dummy) && !filestat::create_bak(buf)) {
        Errs()->add_error(filestat::error_msg());
        return (false);
    }
    out_symfp = fopen(buf, mode);
    if (!out_symfp)
        return (false);
    if (need_dummy)
        write_dummy(name, Physical);
    return (true);
}


// If output is requested outside of the context of a symbol, this
// will go to a "root" cell.
//
bool
xic_out::open_root()
{
    if (out_rootfp)
        return (true);
    const char *root = "Root";
    char root_fname[256];
    strcpy(root_fname, out_infilename);
    char *fn = lstring::strrdirsep(root_fname);
    if (!fn)
        fn = root_fname;
    char *t = strrchr(fn, '.');
    if (t) {
       t++;
       strcpy(t, root);
    }
    else {
        strcat(root_fname, ".");
        strcat(root_fname, root);
    }
    if (out_destdir) {
        char *path = pathlist::mk_path(out_destdir, fn);
        out_rootfp = fopen(path, "w");
        delete [] path;
    }
    else
        out_rootfp = fopen(fn, "w");
    if (!out_rootfp) {
        Errs()->add_error("Unable to open %s.", root_fname);
        return (false);
    }
    return (true);
}


bool
xic_out::write_layer_rec()
{
    if (!out_layer_written) {
        FILE *fp = out_symfp;
        if (!out_in_struct) {
            if (!open_root())
                return (false);
            fp = out_rootfp;
        }
        out_layer_written = true;
        Gen.Layer(fp, out_layer.name);
        return (ferror(fp) == 0);
    }
    return (true);
}

