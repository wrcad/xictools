
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
#include "fio_gdsii.h"
#include "fio_cgx.h"
#include "fio_cif.h"
#include "fio_oasis.h"
#include "fio_chd.h"
#include "fio_cgd.h"
#include "fio_layermap.h"
#include "fio_library.h"
#include "fio_chd_srctab.h"
#include "cd_propnum.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "cd_chkintr.h"
#include "texttf.h"
#include "timedbg.h"


// Factory for cv_out.
//
cv_out *
cFIO::NewOutput(const char *infile, const char *destination, FileType ftype,
    bool to_cgd)
{
    cv_out *out = 0;
    if (ftype == Fnative)
        out = new xic_out(infile);
    else if (ftype == Fgds)
        out = new gds_out;
    else if (ftype == Fcgx)
        out = new cgx_out;
    else if (ftype == Foas)
        out = new oas_out(to_cgd ? new cCGD(infile) : 0);
    else if (ftype == Fcif)
        out = new cif_out(infile);
    else {
        Errs()->add_error("NewOutput: unsupported file type.");
        return (0);
    }
    if (destination && !out->set_destination(destination)) {
        delete out;
        return (0);
    }
    return (out);
}


// Factory for cv_in.
//
cv_in *
cFIO::NewInput(FileType ftype, bool allow_layer_mapping)
{
    switch (ftype) {
    case Fgds:
        return (new gds_in(allow_layer_mapping));
    case Fcgx:
        return (new cgx_in(allow_layer_mapping));
    case Foas:
        return (new oas_in(allow_layer_mapping));
    case Fcif:
        return (new cif_in(allow_layer_mapping));
    default:
        Errs()->add_error("new_input: unsupported file type.");
        break;
    }
    return (0);
}


#define _ISCALE(x) (1000000*x)

namespace {
    // Check if the angle and magnification are close to supported values.
    // If so, with respect to angle, set i and j.  Return false if the
    // angle value is unsupported (not 45 degree multiple).
    //
    bool
    check_set_ang_mag(double *rotn, double *magn, int *ax, int *ay)
    {
        int imag = (int)(1e6*(*magn));
        if (imag >= _ISCALE(1)-1 && imag <= _ISCALE(1)+1)
            *magn = 1.0;

        while (*rotn < 0)
            *rotn += 360.0;
        while (*rotn >= 360.0)
            *rotn -= 360.0;
        int iang = (int)(1e6*(*rotn));
        if (iang <= 1 || iang == _ISCALE(360)-1) {
            *rotn = 0.0;
            *ax = 1;
            *ay = 0;
        }
        else if (iang >= _ISCALE(45)-1 && iang <= _ISCALE(45)+1) {
            *rotn = 45.0;
            *ax = 1;
            *ay = 1;
        }
        else if (iang >= _ISCALE(90)-1 && iang <= _ISCALE(90)+1) {
            *rotn = 90.0;
            *ax = 0;
            *ay = 1;
        }
        else if (iang >= _ISCALE(135)-1 && iang <= _ISCALE(135)+1) {
            *rotn = 135.0;
            *ax = -1;
            *ay = 1;
        }
        else if (iang >= _ISCALE(180)-1 && iang <= _ISCALE(180)+1) {
            *rotn = 180.0;
            *ax = -1;
            *ay = 0;
        }
        else if (iang >= _ISCALE(225)-1 && iang <= _ISCALE(225)+1) {
            *rotn = 225.0;
            *ax = -1;
            *ay = -1;
        }
        else if (iang >= _ISCALE(270)-1 && iang <= _ISCALE(270)+1) {
            *rotn = 270.0;
            *ax = 0;
            *ay = -1;
        }
        else if (iang >= _ISCALE(315)-1 && iang <= _ISCALE(315)+1) {
            *rotn = 315.0;
            *ax = 1;
            *ay = -1;
        }
        else
            // non-45
            return (false);
        return (true);
    }
}


// Fix up the angle and magnification parameters and compute i,j.  If
// the angle given is not a multiple of 45 degrees, set it to the
// nearest multiple and return a warning message.
//
char *
cFIO::GdsParamSet(double *angle, double *magn, int *ax, int *ay)
{
    double oldang = *angle;
    if (!check_set_ang_mag(angle, magn, ax, ay)) {
        int ai = (int)((*angle + 22.5)/45.0);
        *angle = ai * 45.0;
        if (!check_set_ang_mag(angle, magn, ax, ay)) {
            // shouldn't happen
            *angle = 0.0;
            *ax = 1;
            *ay = 0;
        }
        char *buf = new char[256];
        sprintf(buf, "angle %g unacceptable, set to %g", oldang, *angle);
        return (buf);
    }
    return (0);
}
// End of cFIO functions.


//-----------------------------------------------------------------------------
// cv_in   Common functions for input processing.

// Set new state, maybe saving state in bak.
//
void
cv_chd_state::push_state(symref_t *pnew, nametab_t *nt, cv_chd_state *bak)
{
    if (bak)
        *bak = *this;
    s_symref = pnew;
    if (nt)
        s_gen = new crgen_t(nt, s_symref); 
    else
        s_gen = 0;
    if (s_ctab) {
        symref_t *tp = s_symref;
        s_ct_item = s_ctab->resolve_fgif(&tp, s_ct_index);
    }
    else
        s_ct_item = 0;
}


// Clean up and maybe restore prior state.
//
void
cv_chd_state::pop_state(cv_chd_state *bak)
{
    delete s_gen;
    s_gen = 0;
    if (bak)
        *this = *bak;
}


// Return the status of the instance use flag.
//
tristate_t
cv_chd_state::get_inst_use_flag()
{
    if (!s_ct_item)
        return (ts_ambiguous);
    return (s_ct_item->get_flag(s_gen->call_count()));
}
// End of cv_chd_state functions.


cv_in::cv_in(bool allow_layer_mapping)
{
    in_ext_phys_scale = 1.0;
    in_scale = 1.0;
    in_phys_scale = 1.0;
    in_bytes_read = 0;
    in_fb_incr = UFB_INCR;
    in_offset = 0;
    in_cell_offset = 0;
    in_action = cvOpenModeNone;
    in_mode = Physical;
    in_filetype = Fnone;
    in_filename = 0;
    in_symref = 0;
    in_cref_end = 0;
    in_prpty_list = 0;
    in_phys_sym_tab = new nametab_t(Physical);
    in_elec_sym_tab = new nametab_t(Electrical);
    in_submaster_tab = 0;
    in_tf_list = 0;
    in_flatten = false;

    in_gzipped = false;
    in_needs_mult = false;
    in_listonly = false;
    in_uselist = false;
    in_areafilt = false;
    in_clip = false;
    in_no_test_empties = false;
    in_cv_no_openlib = false;
    in_cv_no_endlib = false;
    in_savebb = false;
    in_header_read = false;
    in_ignore_text = false;
    in_ignore_inst = false;
    in_ignore_prop = false;
    in_interrupted = false;
    in_show_progress = false;
    in_own_in_out = false;
    in_keep_clip_text = false;
    in_flatmax = -1;
    in_transform = 0;
    in_sdesc = 0;
    in_out = 0;
    in_lcheck = 0;
    in_alias = 0;
    in_layer_alias = 0;
    in_print_fp = 0;
    in_print_start = 0;
    in_print_end = 0;
    in_print_reccnt = -1;
    in_print_symcnt = -1;
    in_printing = true;
    in_printing_done = false;

    if (allow_layer_mapping) {
        const char *l = FIO()->LayerList();
        ULLtype ull = FIO()->UseLayerList();
        if (l && *l && ull != ULLnoList)
            in_lcheck = new Lcheck((ull == ULLonlyList), l);
        if (FIO()->IsUseLayerAlias()) {
            const char *la = FIO()->LayerAlias();
            if (la && *la) {
                in_layer_alias = new FIOlayerAliasTab;
                in_layer_alias->parse(la);
            }
        }
    }

    in_phys_info = 0;
    in_elec_info = 0;
    in_over_tab = 0;

    // Clear the written flags in the CellTab, if it might be used.
    if (FIO()->IsUseCellTab() && CDcdb()->auxCellTab())
        CDcdb()->auxCellTab()->clear_written();

    CD()->RegisterCreate("cv_in");
}


cv_in::~cv_in()
{
    delete [] in_filename;
    CDp::destroy(in_prpty_list);
    delete in_phys_sym_tab;
    delete in_elec_sym_tab;
    if (in_own_in_out)
        delete in_out;
    if (in_print_fp && in_print_fp != stdout && in_print_fp != stderr)
        fclose(in_print_fp);
    delete in_alias;
    delete in_phys_info;
    delete in_elec_info;
    delete in_submaster_tab;
    delete in_lcheck;
    delete in_layer_alias;
    delete in_over_tab;

    CD()->RegisterDestroy("cv_in");
}


// Return a new CHD struct, with tables transferred to the new
// struct.  Used by cFIO::NewCHD, scaling is 1.0.
//
cCHD *
cv_in::new_chd()
{
    if (!in_filename)
        return (0);
    cCHD *chd = new cCHD(in_filename, file_type());
    chd->setNameTab(in_phys_sym_tab, Physical);
    in_phys_sym_tab = 0;
    chd->setNameTab(in_elec_sym_tab, Electrical);
    in_elec_sym_tab = 0;
    chd->setPcInfo(in_phys_info, Physical);
    in_phys_info = 0;
    chd->setPcInfo(in_elec_info, Electrical);
    in_elec_info = 0;
    cv_alias_info *ai = new cv_alias_info;
    ai->setup(in_alias);
    chd->setAliasInfo(ai);
    return (chd);
}


// Warning: must call chd_finalize whether or not this function returns
// an error, and should only be called once.  See handling of name
// tables.
//
bool
cv_in::chd_setup(cCHD *chd, cv_bbif *ctab, int ix, DisplayMode mode,
    double phys_scale)
{
    if (!chd)
        return (false);

    in_chd_state.setup(chd, ctab, ix, in_phys_sym_tab, in_elec_sym_tab);

    in_phys_sym_tab = chd->nameTab(Physical);
    in_elec_sym_tab = chd->nameTab(Electrical);
    in_mode = mode;

    if (!chd->hasCgd()) {
        // Don't want to do this when using a byte stream.  The type of
        // 'this' is OASIS, which may not match the original file type
        // saved in the CHD.

        chd_set_header_info(chd->headerInfo());
    }

    bool ret = chd_read_header(phys_scale);
    if (ret) {
        in_savebb = false;
        in_ignore_text = in_mode == Physical && FIO()->IsNoReadLabels();
    }
    return (ret);
}


void
cv_in::chd_finalize()
{
    in_savebb = false;
    in_ignore_text = false;

    cCHD *chd = in_chd_state.chd();
    if (chd) {
        if (!chd->hasCgd())
            chd->setHeaderInfo(chd_get_header_info());
        chd->setNameTab(in_phys_sym_tab, Physical);
        chd->setNameTab(in_elec_sym_tab, Electrical);
        in_phys_sym_tab = in_chd_state.phys_stab_bak();
        in_elec_sym_tab = in_chd_state.elec_stab_bak();
    }
    in_chd_state.setup(0, 0, 0, 0, 0);
}


// The AuxCellTab may contain cells to stream out from the main
// database, overriding the cell data in the CHD.  This will write the
// cell(s), and return OInew if p is in the override list.
//
OItype
cv_in::chd_process_override_cell(symref_t *p)
{
    CDcellTab *ct = CDcdb()->auxCellTab();
    if (!ct)
        return (OIok);
    ct_elt *elt = ct->find(p->get_name());
    if (!elt)
        return (OIok);
    if (elt->is_written())
        return (OInew);

    if (!FIO()->IsSkipOverrideCells()) {
        const char *cname = Tstring(p->get_name());
        CDs *sd = CDcdb()->findCell(cname, in_mode);
        if (!sd) {
            Errs()->add_error("Override cell %s not found.", cname);
            return (OIerror);
        }
        if (chd_output_cell(sd, ct) != OIok)
            return (OIerror);
    }
    elt->set_written(true);
    return (OInew);
}


// Output the memory cell, as if read from an input channel.  This is
// intended for override cells, via, and parameterized sub-masters.
// Returns OIok on success, OIerror otherwise.
//
OItype
cv_in::chd_output_cell(CDs *sd, CDcellTab *ct)
{
    if (!sd) {
        Errs()->add_error("chd_output_cell:  unexpected null argument.");
        return (OIerror);
    }
    if (!in_flatten) {
        // Not flattening, write out sd, and the hierarchy under sd if
        // expanding.

        if (!in_out->write_symbol(sd, true))
            return (OIerror);

        // If ct is not null, we are processing override cells.  Via and
        // PCells don't have hierarchy.
        if (ct) {
            ct_elt *elt = ct->find(sd->cellname());
            if (elt)
                elt->set_written(true);
            CDgenHierDn_s gen(sd);
            while ((sd = gen.next()) != 0) {
                elt = ct->find(sd->cellname());
                if (!elt)
                    continue;
                if (!elt->is_written()) {
                    if (!in_out->write_symbol(sd, true))
                        return (OIerror);
                    elt->set_written(true);
                }
            }
        }
        return (OIok);
    }

    // We're flattening, write out sd only, using the transform list.
    if (in_tf_list) {
        ts_reader tsr(in_tf_list);
        CDtx ttx;
        CDap ap;

        while (tsr.read_record(&ttx, &ap)) {
            ttx.scale(in_ext_phys_scale);
            ap.scale(in_ext_phys_scale);
            TPush();
            TApply(ttx.tx, ttx.ty, ttx.ax, ttx.ay, ttx.magn, ttx.refly);
            in_transform++;

            if (ap.nx > 1 || ap.ny > 1) {
                int tx, ty;
                TGetTrans(&tx, &ty);
                xyg_t xyg(0, ap.nx-1, 0, ap.ny-1);
                do {
                    TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                    if (!in_out->write_geometry(sd))
                        return (OIerror);
                    TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            else {
                if (!in_out->write_geometry(sd))
                    return (OIerror);
            }

            in_transform--;
            TPop();
        }
    }
    else {
        if (!in_out->write_geometry(sd))
            return (OIerror);
    }
    return (OIok);
}


//
// Translation accessories
//

void
cv_in::begin_log(DisplayMode mode)
{
    if (in_filename)
        FIO()->ifPrintCvLog(IFLOG_INFO, "Reading %s records in file %s.",
            DisplayModeName(mode), in_filename);
    else
        FIO()->ifPrintCvLog(IFLOG_INFO, "Reading %s records.",
            DisplayModeName(mode));
}


void
cv_in::end_log()
{
    if (in_mode == Physical) {
        FIO()->ifPrintCvLog(IFLOG_INFO, "Done with physical records.");
        if (in_phys_info)
            FIO()->ifPrintCvLog(IFLOG_INFO,
                "Total Phys: symb=%llu sref=%llu aref=%llu box=%llu "
                "poly=%llu wire=%llu text=%llu",
                in_phys_info->total_cells(), in_phys_info->total_srefs(),
                in_phys_info->total_arefs(), in_phys_info->total_boxes(),
                in_phys_info->total_polys(), in_phys_info->total_wires(),
                in_phys_info->total_labels());
    }
    else {
        FIO()->ifPrintCvLog(IFLOG_INFO, "Done with electrical records.");
        if (in_elec_info)
            FIO()->ifPrintCvLog(IFLOG_INFO,
                "Total Elec: symb=%llu sref=%llu aref=%llu box=%llu "
                "poly=%llu wire=%llu text=%llu",
                in_elec_info->total_cells(), in_elec_info->total_srefs(),
                in_elec_info->total_arefs(), in_elec_info->total_boxes(),
                in_elec_info->total_polys(), in_elec_info->total_wires(),
                in_elec_info->total_labels());
    }
}


void
cv_in::show_feedback()
{
    if (in_show_progress) {
        if (in_fb_incr >= 1000000) {
            if (!(in_fb_incr % 1000000))
                FIO()->ifInfoMessage(IFMSG_RD_PGRS,
                    "Processed %d Mb", (int)(in_fb_incr/1000000));
        }
        else
            FIO()->ifInfoMessage(IFMSG_RD_PGRS,
                "Processed %d Kb", (int)(in_fb_incr/1000));
    }
    in_fb_incr += UFB_INCR;
    if (checkInterrupt("Interrupt received, abort translation? "))
        in_interrupted = true;
}


//
// Hierarchy checking
//

// Function to handle cell names encountered while reading that clash
// with the device library.  In electrical mode, we simply make sure
// that in_sdesc is zeroed, so that data from the file are not read
// into the cell.  The library cell will override.  In physical mode,
// the following steps are taken:
//  1. Find a free new name and open a new cell.
//  2. Remove the old name from the name table, and add the new name.
//  3. Look for instances of the old cell that may exist, and fix the
//     master desc to point to the new cell.
// In either case, a log warning is emitted.
//
const char *
cv_in::handle_lib_clash(const char *oldname)
{
    if (in_mode == Physical) {
        if (!in_alias)
            // This should already be set.
            in_alias = new FIOaliasTab(false, false);
        const char *nname = in_alias->new_name(oldname, true);
        FIO()->ifPrintCvLog(IFLOG_WARN,
            "Library device cell name clash detected in physical mode:\n"
            "%s changed to %s.", oldname, nname);

        // Create new cell, we know the new name does not conflict.
        in_sdesc = CDcdb()->insertCell(nname, Physical);

        nametab_t *tab = in_phys_sym_tab;
        if (tab) {
            // Remove old name and add new name in name table.
            symref_t *p = tab->get(CD()->CellNameTableFind(oldname));
            if (p) {
                tab->unlink(p);
                p->set_name(CD()->CellNameTableAdd(nname));
                tab->add(p);
            }

            namegen_t gen(tab);
            while ((p = gen.next()) != 0) {
                CDs *sdesc =  CDcdb()->findCell(p->get_name(), Physical);
                if (!sdesc)
                    continue;
                CDm *md = sdesc->findMaster(CD()->CellNameTableFind(oldname));
                if (md) {
                    md->unlink();
                    md->setCellname(CD()->CellNameTableAdd(nname));
                    md->setCelldesc(in_sdesc);
                    sdesc->linkMaster(md);
                    CDs *oldsd = CDcdb()->findCell(oldname, Physical);
                    if (oldsd)
                        oldsd->unlinkMasterRef(md);
                    in_sdesc->linkMasterRef(md);
                }
            }
        }
        return (nname);
    }
    FIO()->ifPrintCvLog(IFLOG_WARN,
        "Library device cell name clash detected in electrical mode:\n"
        "%s ignored in input.", oldname);
    in_sdesc = 0;
    return (0);
}


namespace {
    // Return true if sdesc has no master refs in tab.
    //
    bool is_toplevel_in_tab(CDs *sdesc, nametab_t *tab)
    {
        CDm_gen mgen(sdesc, GEN_MASTER_REFS);
        for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
            if (m->parent()) {
                if (tab->get(m->parent()->cellname()))
                    return (false);
            }
        }
        return (true);
    }


    // Return true if the first instance has the standard via property.
    //
    bool is_std_via(CDs *sdesc)
    {
        if (!sdesc)
            return (false);
        if (sdesc->prpty(XICP_STDVIA))
            return (true);
        CDm_gen mgen(sdesc, GEN_MASTER_REFS);
        CDm *m = mgen.m_first();
        CDc_gen cgen(m);
        CDc *cd = cgen.c_first();
        return (cd && cd->prpty(XICP_STDVIA));
    }
}


// This function will
//  1) Generate a list of top-level cells, and return it in slp if
//     this is not nil.  The top-level cells don't have a master
//     references table.
// 2)  Take care of resolving library cells, and warn if a cell is
//     referenced and not defined.  All referenced or defined cells are
//     in the database, and pointer linkage is set (in CDs::makeCall).  If
//     a cell still has the unread flag set, it was never actually
//     defined.
//  3) Compute the bounding boxes of all cells.  This will cause
//     insertion/reinsertion of instances as their BB's become valid,
//     in CDs::fixBBs.  This will also check for coincident instances.
//
bool
cv_in::mark_references(stringlist **slp)
{
    bool compressed = false;
    if (in_filename) {
        char *s = strrchr(in_filename, '.');
        if (s && lstring::cieq(s, ".gz"))
            compressed = true;
    }

    FIO()->ifInfoMessage(IFMSG_INFO, "Computing bounding rectangles ...");
    stringlist *s0 = 0;
    nametab_t *tab = in_mode == Physical ? in_phys_sym_tab : in_elec_sym_tab;
    if (tab) {
        Tdbg()->start_timing("check_links");
        stringlist *unkns = 0;
        namegen_t gen(tab);
        symref_t *p;
        while ((p = gen.next()) != 0) {
            CDs *sdesc = CDcdb()->findCell(p->get_name(), in_mode);
            if (!sdesc) {
                // See if the submaster tab resolves this.  In CIF,
                // the first pass can put unmapped names into the
                // phys_sym_tab.

                CDcellName cn = (CDcellName)SymTab::get(in_submaster_tab,
                    (unsigned long)p->get_name());
                if (cn != (CDcellName)ST_NIL && cn != p->get_name())
                    sdesc = CDcdb()->findCell(cn, in_mode);
            }
            if (!sdesc) {
                unkns = new stringlist(lstring::copy(Tstring(p->get_name())),
                    unkns);
                continue;
            }
            sdesc->setArchiveTopLevel(false);
            FIO()->SetSkipFixBB(true);
            bool ret = FIO()->ifReopenSubMaster(sdesc);
            FIO()->SetSkipFixBB(false);
            if (!ret) {
                FIO()->ifPrintCvLog(IFLOG_WARN,
                    "PCell evaluation failed for %s (%s):\n%s",
                    p->get_name(), DisplayModeNameLC(in_mode),
                    Errs()->get_error());
                sdesc->setUnread(false);
            }
            if (sdesc->isUnread()) {

                // The cell was not defined in the archive file.  If
                // the name matches a library cell, open it. 
                // Otherwise, add a warning message.

                CDcbin cbin;
                FIO()->SetSkipFixBB(true);
                OItype oiret = FIO()->OpenLibCell(0,
                    Tstring(sdesc->cellname()), LIBdevice | LIBuser, &cbin);
                FIO()->SetSkipFixBB(false);
                if (oiret != OIok) {
                    // An error or user abort.  OIok is returned whether
                    // or not the cell was actually found.
                    Errs()->add_error("mark_references: %s.",
                        oiret == OIaborted ? "user abort" : "internal error");
                    Tdbg()->stop_timing("check_links");
                    return (false);
                }
                if (cbin.celldesc(in_mode) != sdesc) {
                    // The cell was not resolved.
                    if (in_mode == Physical && is_std_via(sdesc)) {
                        FIO()->ifPrintCvLog(IFLOG_WARN,
                            "Unsatisfied standard via reference to %s.",
                            p->get_name());
                    }
                    else {
                        FIO()->ifPrintCvLog(IFLOG_WARN,
                            "Unsatisfied symbol reference (%s) to %s.",
                            DisplayModeNameLC(in_mode), p->get_name());
                    }
                }
                sdesc->setUnread(false);
                // Unsatisfied references will be ordinary empty cells.
            }
            else if (is_toplevel_in_tab(sdesc, tab)) {
                s0 = new stringlist(
                    lstring::copy(Tstring(p->get_name())), s0);
                if (compressed)
                    sdesc->setCompressed(true);
            }
        }
        if (unkns) {
            // These should have been created as the file was read.
            char *str = stringlist::col_format(unkns, 72, 0);
            FIO()->ifPrintCvLog(IFLOG_WARN,
                "Internal error, the following cell names were found in "
                "input but not resolved after conversion.\n%s\n", str);
            delete [] str;
            stringlist::destroy(unkns);
        }
        Tdbg()->stop_timing("check_links");
    }

    if (!FIO()->IsSkipFixBB()) {
        // set the bounding boxes
        Tdbg()->start_timing("fix_bbs");
        ptrtab_t *stab = new ptrtab_t;
        for (stringlist *sl = s0; sl; sl = sl->next) {
            CDs *sdesc = CDcdb()->findCell(sl->string, in_mode);
            if (!sdesc)
                // can't happen
                continue;
            if (!sdesc->fixBBs(stab)) {
                delete stab;
                Tdbg()->stop_timing("fix_bbs");
                return (false);
            }
        }
        delete stab;
        Tdbg()->stop_timing("fix_bbs");
    }
    if (in_mode == Physical && has_header_props()) {
        // GDSII only
        for (stringlist *sl = s0; sl; sl = sl->next) {
            CDs *sdesc = CDcdb()->findCell(sl->string, in_mode);
            if (sdesc)
                add_header_props(sdesc);
        }
    }

    if (slp)
        *slp = s0;
    else
        stringlist::destroy(s0);

    return (true);
}


// Identify the top cell in the file, it there is a unique cell.
// If a top cell is found, set its file name field and perhaps other
// info, and alter the lists to simplify search.
//
void
cv_in::mark_top(stringlist **psl, stringlist **esl)
{
    // Find the top cell, if any.
    if (psl && *psl && !(*psl)->next) {
        // There is exactly one top cell in the physical part.  If
        // all electrical cells have a counterpart in the physical
        // part, we've got the top cell.

        bool found_top = true;
        if (esl) {
            for (stringlist *sl = *esl; sl; sl = sl->next) {
                if (!get_symref(sl->string, Physical)) {
                    found_top = false;
                    break;
                }
            }
        }
        if (found_top) {
            CDs *top = CDcdb()->findCell((*psl)->string, Physical);
            if (top)
                top->setArchiveTopLevel(true);
            if (esl) {
                stringlist::destroy(*esl);
                *esl = 0;
            }
            return;
        }
    }
    if (esl && *esl && !(*esl)->next) {
        // There is exactly one top cell in the electrical part.  If
        // all physical cells have a counterpart in the electrical
        // part, we've got the top cell.

        bool found_top = true;
        if (psl) {
            for (stringlist *sl = *psl; sl; sl = sl->next) {
                if (!get_symref(sl->string, Electrical)) {
                    found_top = false;
                    break;
                }
            }
        }
        if (found_top) {
            // Set the flag in the physical cell, unless there isn't one.
            CDs *top = CDcdb()->findCell((*esl)->string, Physical);
            if (!top)
                top = CDcdb()->findCell((*esl)->string, Electrical);
            if (top)
                top->setArchiveTopLevel(true);
            if (psl) {
                stringlist::destroy(*psl);
                *psl = 0;
            }
        }
    }
}


//
// via/pcell sub-master checking
//

// If the instance we are about to create is a PCell or StdVia, create
// or find the sub-master.  It may already exist under a different
// name, so we keep a table of name mappings.  Calling this will make
// sure that the (correct) sub-master is available in CDs::makeCall. 
// Vias will always be recognized here if they exist in the current
// technology.  PCells will be recognized if the super-master is
// available.  In either case, if the sub-master can't be created, we
// silently continue.  The sub-masters might be saved as cells in the
// archive being read, in which case they will be resolved in
// mark_references.
//
CDcellName
cv_in::check_sub_master(CDcellName cname)
{
    if (in_mode == Physical) {
        CDcellName cn = (CDcellName)SymTab::get(in_submaster_tab,
            (unsigned long)cname);
        if (cn != (CDcellName)ST_NIL)
            cname = cn;
        else {
            CDs *sdnew;
            cn = cname;
            if (FIO()->ifOpenSubMaster(in_prpty_list, &sdnew) && sdnew) {
                cname = sdnew->cellname();
                if (!in_submaster_tab)
                    in_submaster_tab = new SymTab(false, false);
                in_submaster_tab->add((unsigned long)cn, cname, false);
            }
        }
    }
    return (cname);
}


//
// name list accessories
//

symref_t *
cv_in::get_symref(const char *name, DisplayMode m)
{
    CDcellName nm = CD()->CellNameTableFind(name);
    if (m == Physical)
        return (in_phys_sym_tab ? in_phys_sym_tab->get(nm) : 0);
    else
        return (in_elec_sym_tab ? in_elec_sym_tab->get(nm) : 0);
}


void
cv_in::add_symref(symref_t *p, DisplayMode m)
{
    if (m == Physical) {
        if (in_phys_sym_tab)
            in_phys_sym_tab->add(p);
    }
    else {
        if (in_elec_sym_tab)
            in_elec_sym_tab->add(p);
    }
}
// End of cv_in functions


//-----------------------------------------------------------------------------
// Lcheck  layer access testing

// Constructor, list is a space-separated list of layer names or
// hex-encoded "LLDD" layer number/data type tokens (these must be
// four chars, 0-padded fields, each field can be "xx" for don't care
// condition).  If use_only is true, only the layers listed will be
// converted.  If use_only is false, none of the layers listed will be
// converted.  This is used only for input layer filtering.
//
Lcheck::Lcheck(bool use_only, const char *list)
{
    useonly = use_only;
    nlayers = 0;

    stringlist *s0 = 0;
    char *tok;
    const char *s = list;
    while ((tok = lstring::gettok(&s)) != 0) {
        s0 = new stringlist(tok, s0);
        nlayers++;
    }
    layers = new lnam_t[nlayers + 1];
    for (int i = 0; i < nlayers; i++) {
        layers[i].set(s0->string);
        s0->string = 0;
        stringlist *st = s0;
        s0 = s0->next;
        delete st;
    }
}


// Fill in the fields of a lnam_t.  The string is used or freed.
// If the string is a 4 or 8 digit hex number in LLDD or LLLLDDDD
// format, with possible Xs in either field as a wildcard, set the
// layer/dtype numbers.
//
// If name is not in this format, layer/datatype remain at -1,-1.
//
void
Lcheck::lnam_t::set(char *string)
{
    lname = strmdata::dec_to_hex(string);  // handle "layer,datatype"
    if (lname)
        delete [] string;
    else
        lname = string;
    int l, d;
    if (strmdata::hextrn(lname, &l, &d)) {
        layer = l;
        dtype = d;
        int n = strlen(lname);
        for (int i = 0; i < n; i++) {
            // Make hex name upper case
            if (islower(lname[i]))
                lname[i] = toupper(lname[i]);
        }
    }
}
// End of Lcheck functions and related


//-----------------------------------------------------------------------------
// cv_out   Common functions for output generation.

cv_out::cv_out()
{
    out_scale = 1.0;
    out_phys_scale = 1.0;
    out_byte_count = 0;
    out_fb_thresh = UFB_INCR;
    out_filename = 0;
    out_visited = 0;
    out_prpty = 0;
    out_alias = 0;
    out_chd_refs = 0;
    out_stk = 0;
    out_size_thr = 0;
    out_symnum = 0;
    out_rec_count = 0;
    out_struct_count = 0;
    out_mode = Physical;
    out_filetype = Fnone;
    out_needs_mult = false;
    out_in_struct = false;
    out_interrupted = false;
    out_no_struct = false;

    CD()->RegisterCreate("cv_out");
}


cv_out::~cv_out()
{
    dump_alias(out_filename);
    delete [] out_filename;
    delete out_visited;
    CDp::destroy(out_prpty);
    delete out_alias;
    stringlist::destroy(out_chd_refs);

    CD()->RegisterDestroy("cv_out");
}


// Write all cells in the current symbol table.
//
bool
cv_out::write_all(DisplayMode mode, double sc)
{
    out_rec_count = 0;
    out_byte_count = 0;
    out_symnum = 0;
    if (mode == Physical) {
        out_scale = dfix(sc);
        out_needs_mult = (out_scale != 1.0);
    }
    else {
        out_phys_scale = dfix(sc);
        out_scale = 1.0;
        out_needs_mult = false;
    }
    if (!write_header(0))
        return (false);

    CDgenTab_s gen(mode);
    CDs *sd;
    while ((sd = gen.next()) != 0) {
        if (!write_symbol(sd, true))
            return (false);
    }
    if (!write_chd_refs())
        return (false);
    return (write_endlib(0));
}


bool
cv_out::write(const stringlist *cnlist, DisplayMode mode, double sc)
{
    if (!cnlist)
        return (false);
    out_mode = mode;
    if (!out_visited)
        out_visited = new vtab_t(false);
    else
        out_visited->clear();
    out_rec_count = 0;
    out_byte_count = 0;
    out_symnum = 0;
    if (mode == Physical) {
        out_scale = dfix(sc);
        out_needs_mult = (out_scale != 1.0);
    }
    else {
        out_phys_scale = dfix(sc);
        out_scale = 1.0;
        out_needs_mult = false;
    }

    // On this pass, we don't write empty cells.  Empty cells that are
    // in the sub-hierarchy will be output in write_symbol().
    //
    bool wrote_header = false;
    CDs *top_sdesc = CDcdb()->findCell(cnlist->string, out_mode);
    // top_sdesc can be null!

    for (const stringlist *sl = cnlist; sl; sl = sl->next) {
        CDcbin cbin;
        CDcdb()->findSymbol(sl->string, &cbin);
        CDgenHierDn_cbin gen(&cbin);
        CDcbin ncbin;
        bool err;
        while ((gen.next(&ncbin, &err)) != 0) {
            CDs *sdesc = ncbin.celldesc(out_mode);
            if (!sdesc || sdesc->isEmpty())
                continue;
            if (out_visited->find(Tstring(sdesc->cellname())) < 0) {
                if (!wrote_header) {
                    if (!write_header(top_sdesc))
                        return (false);
                    wrote_header = true;
                }
                if (!write_symbol(sdesc))
                    return (false);
            }
        }
        if (err)
            return (false);
    }

    if (!wrote_header) {
        // Hmmm, nothing written.  For physical mode, dump the empty
        // top level cell, if it exists.
        if (out_mode == Physical) {
            if (!write_header(top_sdesc))
                return (false);
            wrote_header = true;
            if (top_sdesc && !write_symbol(top_sdesc))
                return (false);
        }
    }
    if (wrote_header) {
        if (!write_chd_refs())
            return (false);
        return (write_endlib(
            top_sdesc ? Tstring(top_sdesc->cellname()) : 0));
    }
    return (true);
}


bool
cv_out::write_flat(const char *cname, double sc, const BBox *AOI, bool clip)
{
    if (!cname)
        return (false);
    out_mode = Physical;
    if (!out_visited)
        out_visited = new vtab_t(false);
    else
        out_visited->clear();
    out_rec_count = 0;
    out_byte_count = 0;
    out_symnum = 0;
    if (out_mode == Physical) {
        out_scale = dfix(sc);
        out_needs_mult = (out_scale != 1.0);
    }
    else {
        out_phys_scale = dfix(sc);
        out_scale = 1.0;
        out_needs_mult = false;
    }
    CDs *sdesc = CDcdb()->findCell(cname, Physical);
    if (!sdesc)
        return (false);
    if (!write_header(sdesc))
        return (false);

// XXX need this anymore?
    // The flattening can't handle CHD reference cells which might be in
    // the hierarchy.  The hierarchy traversal generator will issue a
    // warning if a CHD reference is passed.  Turn on warnings and warn
    // the user if one shows up.
    Errs()->arm_warnings(true);
    bool ok = write_symbol_flat(sdesc, AOI, clip);
    if (ok && Errs()->get_warnings()) {
        for (const stringlist *s = Errs()->get_warnings(); s; s = s->next)
            FIO()->ifPrintCvLog(IFLOG_WARN, s->string);
    }
    Errs()->arm_warnings(false);

    return (ok && write_endlib(Tstring(sdesc->cellname())));
}


// File end write operation for multi-archive merge, add the dummy top
// cell and end-of-library marker.
//
bool
cv_out::write_multi_final(Topcell *topcell)
{
    if (topcell) {
        time_t now = time(0);
        struct tm *tgmt = gmtime(&now);
        if (!write_struct(topcell->cellname, tgmt, tgmt))
            return (false);
        clear_property_queue();
        for (int i = 0; i < topcell->numinst; i++) {
            if (!write_sref(topcell->instances + i))
                return (false);
        }
        if (!write_end_struct())
            return (false);
    }
    clear_property_queue();
    return (write_endlib(topcell ? topcell->cellname : 0));
}


bool
cv_out::write_begin(double sc)
{
    out_mode = Physical;
    if (!out_visited)
        out_visited = new vtab_t(false);
    else
        out_visited->clear();
    out_rec_count = 0;
    out_byte_count = 0;
    out_symnum = 0;
    if (out_mode == Physical) {
        out_scale = dfix(sc);
        out_needs_mult = (out_scale != 1.0);
    }
    else {
        out_phys_scale = dfix(sc);
        out_scale = 1.0;
        out_needs_mult = false;
    }
    return (write_header(0));
}


bool
cv_out::write_begin_struct(const char *name)
{
    if (!name || !*name)
        return (false);
    name = Tstring(CD()->CellNameTableAdd(name));
    add_visited(name);

    time_t tloc = time(0);
    tm *date = gmtime(&tloc);
    bool ret = write_struct(name, date, date);
    clear_property_queue();
    if (!ret)
        return (false);
    return (true);
}


bool
cv_out::write_symbol(const CDs *sdesc, bool thisonly)
{
    if (!sdesc)
        return (true);

    // Mark cell as visited, store symbol number.
    add_visited(Tstring(sdesc->cellname()));

    // Never write library cell definitions.
    if (sdesc->isDevice() && sdesc->isLibrary())
        return (true);

    // Sub-masters are included in output when exporting layout
    // (StripFoExport set) or when KeepPCellSubMasters is set,
    // or if the sub-master was read from a file (not created).
    // Otherwise, these cells are stripped and will be recreated when
    // needed.
    if (!FIO()->IsStripForExport()) {
        if (sdesc->isPCellSubMaster() && !sdesc->isPCellReadFromFile() &&
                !FIO()->IsKeepPCellSubMasters())
            return (true);
        if (sdesc->isViaSubMaster() && !FIO()->IsKeepViaSubMasters())
            return (true);
    }

    // Unless WriteAllCells or StripForExport is true, user library
    // cells do not appear in output.
    if (sdesc->isLibrary() &&
            !FIO()->IsWriteAllCells() && !FIO()->IsStripForExport())
        return (true);

    if (sdesc->isChdRef() && out_filetype != Fnative) {
        out_chd_refs = new stringlist(
            lstring::copy(Tstring(sdesc->cellname())), out_chd_refs);
        return (true);
    }

    if (!thisonly) {
        // First write to the output file any cell definitions below
        // sdesc.
        //
        CDm_gen mgen(sdesc, GEN_MASTERS);
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            if (mdesc->hasInstances()) {
                CDs *msdesc = mdesc->celldesc();
                if (msdesc && out_visited->find(
                        Tstring(msdesc->cellname())) < 0) {
                    // Write master's definition to output file
                    if (!write_symbol(msdesc))
                        return (false);
                }
            }
        }
    }

    FIO()->ifUpdateNodes(sdesc);
    if (!queue_properties(sdesc))
        return (false);

    time_t tloc = time(0);
    tm *date = gmtime(&tloc);
    bool ret = write_struct(Tstring(sdesc->cellname()), date, date);
    clear_property_queue();
    if (!ret)
        return (false);

    // Note:  it is important for electrical mode that instances be
    // written before geometry (really just labels).  See
    // CDs::prptyLabelPatch().

    if (!write_instances(sdesc))
        return (false);
    if (!write_geometry(sdesc))
        return (false);
    if (!write_end_struct())
        return (false);
    return (true);
}


// Write out the hierarchies of any chd reference cells that were 
// encountered.
//
bool
cv_out::write_chd_refs()
{
    if (!out_chd_refs)
        return (true);

    // If auto-aliasing, cell names seen that match a name in the
    // in-core hierarchy or a previous CHD are automatically changed,
    // and the cell written with the new name.  Duplicate cells from
    // the same CHD are skipped.
    //
    // If not auto-aliasing, cells with names that have been
    // previously output are always skipped.

    bool auto_alias = FIO()->IsRefCellAutoRename();
    if (auto_alias && out_alias) {
        // Remove all of the reference cells from the alias seen table.
        // This is needed for auto-aliasing.

        for (stringlist *sl = out_chd_refs; sl; sl = sl->next)
            out_alias->unseen(sl->string);
    }

    // First create a table for all of the reference cells.  The cells
    // will be grouped by CHD.

    chd_src_tab tab;
    for (stringlist *sl = out_chd_refs; sl; sl = sl->next) {
        CDs *sdesc = CDcdb()->findCell(sl->string, out_mode);
        if (!sdesc) {
            Errs()->add_error(
                "write_chd_refs: no %s cell found in current symbol table.",
                Tstring(sdesc->cellname()));
            return (false);
        }
        if (!sdesc->isChdRef()) {
            Errs()->add_error(
                "write_chd_refs: cell %s not a CHD reference.",
                Tstring(sdesc->cellname()));
            return (false);
        }
        CDp *p = sdesc->prpty(XICP_CHD_REF);
        if (!p) {
            Errs()->add_error(
                "write_chd_refs: no ChdRef (7150) property in cell %s.",
                Tstring(sdesc->cellname()));
            return (false);
        }
        tab.add(sl->string, new sChdPrp(p->string()));
    }

    // Loop through the table elements, each has a unique CHD.

    chd_src_tab_gen gen(&tab);
    chd_src_t *src;
    while ((src = gen.next()) != 0) {
        if (!src->cs_cellnames)
            continue;

        if (auto_alias && out_alias) {
            // This sets up the auto-aliasing to automatically change
            // cell names found in the file that clash with existing
            // names (from a different CHD).

            out_alias->reinit();
        }

        // Create the CHD if necessary.

        cCHD *chd =  0;
        bool free_chd = false;
        const char *dbname = CDchd()->chdFind(src->cs_chd_prp);
        if (dbname)
            chd = CDchd()->chdRecall(dbname, false);
        if (!chd) {
            if (!src->cs_chd_prp->get_path()) {
                Errs()->add_error(
                    "write_chd_refs: no filename in ChdRef (7150) property "
                    "for cell %s.", src->cs_cellnames->string);
                return (false);
            }
            FIO()->ifPrintCvLog(IFLOG_INFO,
                "Opening file for reference cells, name:\n%s.",
                src->cs_chd_prp->get_path());
            FILE *fp = FIO()->POpen(src->cs_chd_prp->get_path(), "r");
            if (!fp) {
                Errs()->add_error(
                    "write_chd_refs: can't open referenced file\n%s.",
                    src->cs_chd_prp->get_path());
                return (false);
            }
            FileType ft = FIO()->GetFileType(fp);
            fclose(fp);
            if (!FIO()->IsSupportedArchiveFormat(ft)) {
                Errs()->add_error(
                    "write_chd_refs: referenced file type unsupported\n%s.",
                    src->cs_chd_prp->get_path());
                return (false);
            }

            FIOaliasTab *atab = new FIOaliasTab(false, false);
            atab->setup(src->cs_chd_prp->get_alias_flags(),
                src->cs_chd_prp->get_alias_prefix(),
                src->cs_chd_prp->get_alias_suffix());
            chd = FIO()->NewCHD(src->cs_chd_prp->get_path(), ft, Electrical,
                atab);
            delete atab;
            if (!chd) {
                Errs()->add_error(
                    "write_chd_refs: can't create digest for\n%s.",
                    src->cs_chd_prp->get_path());
                return (false);
            }
            free_chd = true;
        }

        // Keep track of the cells written from this chd, so that we
        // don't write duplicates.
        //
        SymTab *vtab = auto_alias ? new SymTab(false, false) : CHD_USE_VISITED;

        // Loop through the cells from this CHD.
        //
        bool ok = true;
        for (stringlist *sl = src->cs_cellnames; sl; sl = sl->next) {

            cv_in *in = FIO()->NewInput(chd->filetype(), false);

            if (!in->setup_source(src->cs_chd_prp->get_path())) {
                Errs()->add_error(
                    "write_chd_refs: failed to open source\n%s.",
                    src->cs_chd_prp->get_path());
                ok = false;
                delete in;
                break;
            }
            if (!in->setup_backend(this)) {
                Errs()->add_error(
                    "write_chd_refs: failed to set up back end.");
                ok = false;
                delete in;
                break;
            }
            in->set_no_open_lib(true);
            in->set_no_end_lib(true);

            symref_t *p = chd->findSymref(sl->string, Physical);
            if (!p) {
                Errs()->add_error(
                    "write_chd_refs: failed find %s in CHD.", sl->string);
                ok = false;
                delete in;
                break;
            }

            FIOcvtPrms prms;
            OItype oiret = chd->write(p, in, &prms, true, vtab);
            delete in;
            if (oiret != OIok) {
                ok = false;
                break;
            }
        }
        if (free_chd)
            delete chd;
        if (auto_alias)
            delete vtab;
        if (!ok)
            return (false);
    }
    return (true);
}


bool
cv_out::write_symbol_flat(const CDs *sdesc, const BBox *AOI, bool clip)
{
    time_t tloc = time(0);
    tm *date = gmtime(&tloc);
    if (!write_struct(Tstring(sdesc->cellname()), date, date))
        return (false);

    clear_property_queue();
    CDlgen gen(out_mode);
    CDl *ldesc;
    while ((ldesc = gen.next()) != 0) {
        // marked layers are skipped
        if (ldesc->isTmpSkip() || ldesc->layerType() != CDLnormal)
            continue;

        cvLchk lchk = cvLneedCheck;
        sPF pf(sdesc, AOI, ldesc, CDMAXCALLDEPTH);
        CDo *odesc;
        while ((odesc = pf.next(false, false)) != 0) {
            if (clip && !odesc->oBB().intersect(AOI, false))
                continue;

            bool ret;
            if (clip)
                ret = write_object_clipped(odesc, AOI, &lchk);
            else
                ret = write_object(odesc, &lchk);
            delete odesc;
            if (!ret)
                return (false);
            if (lchk == cvLnogo)
                break;
        }
        if (!flush_cache())
            return (false);
    }
    if (!write_end_struct())
        return (false);
    return (true);
}


bool
cv_out::write_instances(const CDs *sdesc)
{
    if (!sdesc)
        return (true);

#define SORT_INSTS
#ifdef SORT_INSTS
    // Sort the instances alphabetically by master, then by database
    // order.

    CDcl *cl0 = 0;
    CDg gdesc;
    gdesc.init_gen(sdesc, CellLayer());
    const CDc *cdesc;
    while ((cdesc = (const CDc*)gdesc.next()) != 0) {
        if (cdesc->cellname())
            cl0 = new CDcl(cdesc, cl0);
    }
    CDcl::sort_instances(cl0);
    bool ret = true;
    for (CDcl *cl = cl0; cl; cl = cl->next) {
        cdesc = cl->cdesc;

        out_cellBB.add(&cdesc->oBB());
        ret = queue_properties(cdesc);
        if (!ret)
            break;

        Instance inst;
        inst.name = Tstring(cdesc->cellname());
        if (!inst.name)
            continue;
        ret = inst.set(cdesc);
        if (ret)
            ret = write_sref(&inst);
        clear_property_queue();
        if (!ret)
            break;
    }
    CDcl::destroy(cl0);
    return (ret);

#else
    // This outputs instances in database order.
    CDg gdesc;
    gdesc.init_gen(sdesc, CellLayer());
    const CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {

        out_cellBB.add(&cdesc->oBB());
        if (!queue_properties(cdesc))
            return (false);

        Instance inst;
        inst.name = cdesc->cellname()->string();
        if (!inst.name)
            continue;
        bool ret = inst.set(cdesc);
        if (ret)
            ret = write_sref(&inst);
        clear_property_queue();
        if (!ret)
            return (false);
    }
    return (true);

    /* This groups by master, but ordering is otherwise random.
    CDm_gen mgen(sdesc, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {

            out_cellBB.add(&cdesc->oBB());
            if (!queue_properties(cdesc))
                return (false);

            Instance inst;
            inst.name = mdesc->cellname()->string();
            if (!inst.name)
                return (false);
            bool ret = inst.set(cdesc);
            if (ret)
                ret = write_sref(&inst);
            clear_property_queue();
            if (!ret)
                return (false);
        }
    }
    return (true);
    */
#endif
}


// Process the geometry records.  If a transform is given, we
// transform each object before output.
//
bool
cv_out::write_geometry(const CDs *sdesc)
{
    if (!sdesc)
        return (true);
    CDsLgen gen(sdesc);
    gen.sort();  // layer table order
    CDl *ldesc;
    while ((ldesc = gen.next()) != 0) {
        // marked layers are skipped
        if (ldesc->isTmpSkip() || ldesc->layerType() != CDLnormal)
            continue;

        CDg gdesc;
        gdesc.init_gen(sdesc, ldesc);

        cvLchk lchk = cvLneedCheck;
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (!odesc->is_normal())
                continue;
            if (!queue_properties(odesc))
                return (false);
            bool ret = write_object(odesc, &lchk);

            clear_property_queue();
            if (!ret)
                return (false);
            if (lchk == cvLnogo)
                break;
        }
        if (!flush_cache())
            return (false);
    }
    return (true);
}


bool
cv_out::write_object_clipped(const CDo *odesc, const BBox *AOI, cvLchk *lchk)
{
    if (!odesc || !AOI)
        return (false);

    if (odesc->type() == CDBOX) {
        BBox BB(odesc->oBB());
        if (BB.left < AOI->left)
            BB.left = AOI->left;
        if (BB.bottom < AOI->bottom)
            BB.bottom = AOI->bottom;
        if (BB.right > AOI->right)
            BB.right = AOI->right;
        if (BB.top > AOI->top)
            BB.top = AOI->top;
        if (BB.width() <= 0 || BB.height() <= 0)
            return (true);

        CDo cdo(odesc->ldesc());
        cdo.set_state(odesc->state());
        cdo.set_flags(odesc->flags());
        cdo.set_oBB(BB);
        return (write_object(&cdo, lchk));
    }

    if (odesc->type() == CDPOLYGON) {
        bool enclosing;
        PolyList *pl = ((const CDpo*)odesc)->po_clip(AOI, &enclosing);
        if (enclosing)
            return (write_object(odesc, lchk));
            // pl is 0

        CDpo cdpo(odesc->ldesc());
        cdpo.set_state(odesc->state());
        cdpo.set_flags(odesc->flags());
        for (PolyList *px = pl; px; px = px->next) {
            cdpo.set_points(px->po.points);
            cdpo.set_numpts(px->po.numpts);
            BBox tBB;
            px->po.computeBB(&tBB);
            cdpo.set_oBB(tBB);
            if (!write_object(&cdpo, lchk)) {
                cdpo.set_points(0);
                PolyList::destroy(pl);
                return (false);
            }
            if (lchk && *lchk == cvLnogo)
                break;
        }
        cdpo.set_points(0);
        PolyList::destroy(pl);
        return (true);
    }

    if (odesc->type() == CDWIRE) {
        Poly wp;
        bool ok = ((const CDw*)odesc)->w_toPoly(&wp.points, &wp.numpts);
        if (!ok)
            return (false);

        bool enclosing;
        PolyList *pl = wp.clip(AOI, &enclosing);
        delete [] wp.points;
        if (enclosing)
            return (write_object(odesc, lchk));
            // pl is 0

        CDpo cdpo(odesc->ldesc());
        cdpo.set_state(odesc->state());
        cdpo.set_flags(odesc->flags());
        for (PolyList *px = pl; px; px = px->next) {
            cdpo.set_points(px->po.points);
            cdpo.set_numpts(px->po.numpts);
            BBox tBB;
            px->po.computeBB(&tBB);
            cdpo.set_oBB(tBB);
            if (!write_object(&cdpo, lchk)) {
                cdpo.set_points(0);
                cdpo.set_numpts(0);
                PolyList::destroy(pl);
                return (false);
            }
            if (lchk && *lchk == cvLnogo)
                break;
        }
        cdpo.set_points(0);
        cdpo.set_numpts(0);
        PolyList::destroy(pl);
        return (true);
    }
    return (true);
}


bool
cv_out::check_set_layer(const CDl *ld, bool *check_mapping)
{
    if (!ld)
        return (false);
    strm_odata *so = ld->strmOut();
    Layer layer(ld->name(), so ? so->layer() : -1, so ? so->dtype() : -1,
        ld->index(out_mode));
    return (queue_layer(&layer, check_mapping));
}


bool
cv_out::queue_properties(const CDs *sdesc)
{
    for (CDp *pd = sdesc->prptyList(); pd; pd = pd->next_prp()) {
        char *s;
        if (pd->string(&s)) {
            bool ret = queue_property(pd->value(), s);
            delete [] s;
            if (!ret)
                return (false);
        }
    }
    return (true);
}


bool
cv_out::queue_properties(const CDo *odesc)
{
    // Add the property list
    for (CDp *pd = odesc->prpty_list(); pd; pd = pd->next_prp()) {
        char *s;
        if (pd->string(&s)) {
            bool ret = queue_property(pd->value(), s);
            delete [] s;
            if (!ret) {
                clear_property_queue();
                return (false);
            }
        }
    }

    // Add the NODRC property if necessary
    if (out_mode == Physical && odesc->type() != CDINSTANCE &&
            odesc->type() != CDLABEL && (odesc->flags() & CDnoDRC)) {
        if (!queue_property(XICP_NODRC, "NODRC")) {
            clear_property_queue();
            return (false);
        }
    }
    return (true);
}


void
cv_out::clear_property_queue()
{
    CDp::destroy(out_prpty);
    out_prpty = 0;
}


// Private non-inline part of the size test function.
//
bool
cv_out::size_test_prv(const symref_t *p, const cTfmStack *stk)
{
    if (p) {
        BBox BB(*p->get_bb());
        stk->TBB(&BB, 0);
        if (BB.width() < out_size_thr || BB.height() < out_size_thr)
            return (true);
    }
    return (false);
}
// End of cv_out functions


bool
Instance::set(const CDc *cdarg)
{
    if (!cdarg)
        return (false);
    CDap ap(cdarg);
    nx = ap.nx;
    ny = ap.ny;
    dx = ap.dx;
    dy = ap.dy;
    CDtx tx(cdarg);
    magn = tx.magn;
    set_angle(tx.ax, tx.ay);
    origin.set(cdarg->posX(), cdarg->posY());
    cdesc = cdarg;
    reflection = tx.refly;

    return (true);
}


// Set the ax,ay and angle fields.
//
void
Instance::set_angle(int tax, int tay)
{
    ax = tax;
    ay = tay;
    angle = 0.0;

    if (ay == 1) {
        if (ax == 1)
            angle = 45.0;
        else if (ax == 0)
            angle = 90.0;
        else if (ax == -1)
            angle = 135.0;
    }
    else if (ay == 0) {
        if (ax == -1)
            angle = 180.0;
    }
    else if (ay == -1) {
        if (ax == -1)
            angle = 225.0;
        else if (ax == 0)
            angle = 270.0;
        else if (ax == 1)
            angle = 315.0;
    }
}


// This returns the two coordinates which along with the origin defines
// the array orientation in GDSII.
//
bool
Instance::get_array_pts(Point *p) const
{
    if (nx <= 1 && ny <= 1)
        return (false);
    if (gds_text) {
        p[0] = apts[0];
        p[1] = apts[1];
        return (true);
    }
    cTfmStack stk;
    stk.TPush();
    stk.TApply(origin.x, origin.y, ax, ay, magn, reflection);

    // Create the transformed array coordinates
    // PCOL
    p[0].set(dx*nx, 0);
    stk.TPoint(&p[0].x, &p[0].y);
    // PROW
    p[1].set(0, dy*ny);
    stk.TPoint(&p[1].x, &p[1].y);

    stk.TPop();
    return (true);
}


void
Instance::scale(double sc)
{
    origin.set(mmRnd(sc*origin.x), mmRnd(sc*origin.y));
    dx = mmRnd(sc*dx);
    dy = mmRnd(sc*dy);
}


// Return true if the instance overlaps BBaoi.  The sBB is the master
// cell BB.
//
bool
Instance::check_overlap(cTfmStack *stk, const BBox *sBB, const BBox *BBaoi)
const
{
    bool ret = false;
    stk->TPush();
    applyTransform(stk);
    stk->TPremultiply();
    if (nx <= 1 && ny <= 1) {
        BBox iBB(*sBB);
        ret = stk->TBBcheck(&iBB, BBaoi);
    }
    else if ((abs(dx) - sBB->width() < BBaoi->width()) &&
            (abs(dy) - sBB->height() < BBaoi->height())) {
        // It is sufficient to see overlap of arrayed BB.

        BBox iBB(*sBB);
        if (dx > 0)
            iBB.right += (nx - 1)*dx;
        else
            iBB.left += (nx - 1)*dx;
        if (dy > 0)
            iBB.top += (ny - 1)*dy;
        else
            iBB.bottom += (ny - 1)*dy;

        ret = stk->TBBcheck(&iBB, BBaoi);
    }
    else {
        // Need to check individual elements, since BBaoi could
        // fit in the spacing between elements.

        unsigned int x1, x2, y1, y2;
        if (stk->TOverlap(sBB, BBaoi, nx, dx, ny, dy, &x1, &x2, &y1, &y2)) {
            int tx, ty;
            stk->TGetTrans(&tx, &ty);

            xyg_t xyg(x1, x2, y1, y2);
            do {
                stk->TTransMult(xyg.x*dx, xyg.y*dy);

                BBox iBB(*sBB);
                ret = stk->TBBcheck(&iBB, BBaoi);
                stk->TSetTrans(tx, ty);
                if (ret)
                    break;

            } while (xyg.advance());
        }
    }
    stk->TPop();
    return (ret);
}
// End of Instance functions.


bool
Text::set(const Label *la, DisplayMode mode, FileType ft)
{
    x = la->x;
    y = la->y;
    xform = la->xform;
    width = la->width;
    height = la->height;

    if (ft == Fgds)
        setup_gds(mode);
    return (true);
}


// Fill in the GDSII stuff, if not already valid.
//
void
Text::setup_gds(DisplayMode mode)
{
    if (gds_valid)
        return;
    int lines = 1;
    if (text) {
        for (const char *s = text; *s; s++) {
            if (*s == '\n' && *(s+1))
                lines++;
        }
    }
    if (mode == Physical)
        magn = (double)height/INTERNAL_UNITS(lines*CDphysDefTextHeight);
    else
        magn = (double)height/ELEC_INTERNAL_UNITS(lines*CDelecDefTextHeight);
    cTfmStack stk;
    stk.TSetTransformFromXform(xform, 0, 0);
    CDtf tf;
    stk.TCurrent(&tf);
    stk.TPop();

    // Set angle and reflection according to xform.  GDSII specifies
    // reflection (y -> -y) followed by rotation.

    int a, b, c, d;
    tf.abcd(&a, &b, &c, &d);

    reflection = false;
    if ((a && (a == -d)) || (b && (b == c))) {
        // tm is reflected, un-reflect and get T
        c = -c;
        reflection = true;
    }
    if (!a && c)
        angle = (c > 0) ? 90.0 : 270.0;
    else if (a && !c)
        angle = (a > 0) ? 0.0 : 180.0;
    else if (a > 0)
        angle = (c > 0) ? 45.0 : 315.0;
    else
        angle = (c > 0) ? 135.0 : 225.0;

    hj = 0;
    if (xform & TXTF_HJC)
        hj = 1;
    else if (xform & TXTF_HJR)
        hj = 2;
    vj = 2;
    if (xform & TXTF_VJC)
        vj = 1;
    else if (xform & TXTF_VJT)
        vj = 0;
    font = TXTF_FONT_INDEX(xform);
    gds_valid = true;
}


void
Text::transform(cTfmStack *tstk)
{
    tstk->TPoint(&x, &y);
    tstk->TSetTransformFromXform(xform, 0, 0);
    tstk->TPremultiply();
    CDtf tf;
    tstk->TCurrent(&tf);
    xform &= ~(TXTF_ROT | TXTF_MY | TXTF_MX);
    xform |= (tf.get_xform() & (TXTF_ROT | TXTF_MY | TXTF_MX));
    tstk->TPop();

    if (gds_valid) {
        int a, b, c, d;
        tf.abcd(&a, &b, &c, &d);

        reflection = false;
        if ((a && (a == -d)) || (b && (b == c))) {
            // tm is reflected, un-reflect and get T
            c = -c;
            reflection = true;
        }
        if (!a && c)
            angle = (c > 0) ? 90.0 : 270.0;
        else if (a && !c)
            angle = (a > 0) ? 0.0 : 180.0;
        else if (a > 0)
            angle = (c > 0) ? 45.0 : 315.0;
        else
            angle = (c > 0) ? 135.0 : 225.0;
    }
}


void
Text::scale(double sc)
{
    x = mmRnd(sc*x);
    y = mmRnd(sc*y);
    width = mmRnd(sc*width);
    height = mmRnd(sc*height);

    if (gds_valid) {
        magn *= sc;
        pwidth = mmRnd(sc*pwidth);
    }
}
// End of Text functions

