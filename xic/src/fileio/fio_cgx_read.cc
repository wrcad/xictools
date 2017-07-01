
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
 $Id: fio_cgx_read.cc,v 1.155 2017/04/12 05:02:46 stevew Exp $
 *========================================================================*/

#include "fio.h"
#include "fio_cgx.h"
#include "fio_chd.h"
#include "fio_layermap.h"
#include "cd_hypertext.h"
#include "cd_strmdata.h"
#include "cd_propnum.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "timedbg.h"
#include "texttf.h"


namespace {
    inline int
    cgxx2xicx(int cgxx)
    {
        return ((cgxx & 0xf) | ((cgxx & 0xf0) << 1));
    }
}


// Return true if fp points to a CGX file (may be gzipped).
//
bool
cFIO::IsCGX(FILE *fp)
{
    FilePtr file = sFilePtr::newFilePtr(fp);
    char tb[4];
    tb[0] = 0;  // valgrind wants these initialized
    tb[1] = 0;
    tb[2] = 0;
    tb[3] = 0;
    file->z_read(tb, 1, 4);
    delete file;
    if (tb[0] == 'c' && tb[1] == 'g' && tb[2] == 'x' && tb[3] == 0)
        return (true);
    return (false);
}


// Read the CGX file cgx_fname, the new cells will be be added to
// the database.  All conversions will be scaled by the value of
// scale.
//  Return values:
//    OIerror       unspecified error
//    OIok          file was read and new cells opened ok
//    OIaborted     user aborted
//
OItype
cFIO::DbFromCGX(const char *cgx_fname, const FIOreadPrms *prms,
    stringlist **tlp, stringlist **tle)
{
    if (!prms)
        return (OIerror);
    Tdbg()->start_timing("cgx_read");
    cgx_in *cgx = new cgx_in(prms->allow_layer_mapping());
    cgx->set_show_progress(true);
    cgx->set_no_test_empties(IsNoCheckEmpties());

    CD()->SetReading(true);
    cgx->assign_alias(NewReadingAlias(prms->alias_mask()));
    cgx->read_alias(cgx_fname);
    bool ret = cgx->setup_source(cgx_fname);
    if (ret) {
        cgx->set_to_database();
        Tdbg()->start_timing("cgx_read_phys");
        cgx->begin_log(Physical);
        CD()->SetDeferInst(true);
        ret = cgx->parse(Physical, false, prms->scale());
        CD()->SetDeferInst(false);
        Tdbg()->stop_timing("cgx_read_phys");
        if (ret)
            ret = cgx->mark_references(tlp);
        cgx->end_log();
    }
    if (ret && !CD()->IsNoElectrical() && cgx->has_electrical()) {
        Tdbg()->start_timing("cgx_read_elec");
        cgx->begin_log(Electrical);
        CD()->SetDeferInst(true);
        bool lpc = CD()->EnableLabelPatchCache(true);
        ret = cgx->parse(Electrical, false, prms->scale());
        CD()->EnableLabelPatchCache(lpc);
        CD()->SetDeferInst(false);
        Tdbg()->stop_timing("cgx_read_elec");
        if (ret)
            ret = cgx->mark_references(tle);
        cgx->end_log();
    }
    if (ret)
        cgx->mark_top(tlp, tle);
    CD()->SetReading(false);

    if (ret)
        cgx->dump_alias(cgx_fname);

    OItype oiret = ret ? OIok : cgx->was_interrupted() ? OIaborted : OIerror;
    delete cgx;
    Tdbg()->stop_timing("cgx_read");
    return (oiret);
}


// Read the CGX file cgx_fname, performing translation.
//  Return values:
//    OIerror       unspecified error
//    OIok          file was read and new cells opened ok
//    OIaborted     user aborted
//
OItype
cFIO::FromCGX(const char *cgx_fname, const FIOcvtPrms *prms,
    const char *chdcell)
{
    if (!prms) {
        Errs()->add_error("FromCGX: null destination pointer.");
        return (OIerror);
    }
    if (!prms->destination()) {
        Errs()->add_error("FromCGX: no destination given!");
        return (OIerror);
    }

    cCHD *chd = CDchd()->chdRecall(cgx_fname, false);
    if (chd) {
        if (chd->filetype() != Fcgx) {
            Errs()->add_error("FromCGX:: CHD file type not CGX!");
            return (OIerror);
        }

        // We were given a CHD name, use it.
        cgx_fname = chd->filename();
    }

    SetAllowPrptyStrip(true);
    if (chd || prms->use_window() || prms->flatten() ||
            (prms->ecf_level() != ECFnone)) {

        // Using windowing or other features requiring a cCHD
        // description.  This is restricted to physical-mode data.

        bool free_chd = false;
        if (!chd) {
            unsigned int mask = prms->alias_mask();
            if (prms->filetype() == Fgds)
                mask |= CVAL_GDS;
            FIOaliasTab *tab = NewTranslatingAlias(mask);
            if (tab)
                tab->read_alias(cgx_fname);
            cvINFO info = cvINFOtotals;
            if (prms->ecf_level() == ECFall || prms->ecf_level() == ECFpre)
                info = cvINFOplpc;
            chd = NewCHD(cgx_fname, Fcgx, Physical, tab, info);
            delete tab;
            free_chd = true;
        }
        OItype oiret = OIerror;
        if (chd) {
            oiret = chd->translate_write(prms, chdcell);
            if (free_chd)
                delete chd;
        }
        SetAllowPrptyStrip(false);
        return (oiret);
    }

    cgx_in *cgx = new cgx_in(prms->allow_layer_mapping());
    cgx->set_show_progress(true);

    // Translating, directly streaming.  Skip electrical data if
    // StripForExport is set.

    const cv_alias_info *aif = prms->alias_info();
    if (aif)
        cgx->assign_alias(new FIOaliasTab(true, false, aif));
    else {
        unsigned int mask = prms->alias_mask();
        if (prms->filetype() == Fgds)
            mask |= CVAL_GDS;
        cgx->assign_alias(NewTranslatingAlias(mask));
    }
    cgx->read_alias(cgx_fname);

    bool ret = cgx->setup_source(cgx_fname);
    if (ret)
        ret = cgx->setup_destination(prms->destination(), prms->filetype(),
            prms->to_cgd());

    if (ret) {
        cgx->begin_log(Physical);
        ret = cgx->parse(Physical, false, prms->scale());
        cgx->end_log();
    }
    if (ret && !fioStripForExport && !prms->to_cgd() &&
            !CD()->IsNoElectrical() && cgx->has_electrical()) {
        cgx->begin_log(Electrical);
        ret = cgx->parse(Electrical, false, prms->scale());
        cgx->end_log();
    }

    if (ret)
        cgx->dump_alias(cgx_fname);

    OItype oiret = ret ? OIok : cgx->was_interrupted() ? OIaborted : OIerror;
    delete cgx;
    SetAllowPrptyStrip(false);
    return (oiret);
}
// End of cFIO functions


namespace {
    const char *cgx_record_names[] =
    {
        "LIBRARY",      // 0
        "STRUCT",       // 1
        "CPRPTY",       // 2
        "PROPERTY",     // 3
        "LAYER",        // 4
        "BOX",          // 5
        "POLY",         // 6
        "WIRE",         // 7
        "TEXT",         // 8
        "SREF",         // 9
        "ENDLIB",       // 10
        "SKINFO"        // 11
    };
}


void
cgx_info::add_record(int rtype)
{
    rec_counts[rtype]++;
    cv_info::add_record(rtype);
}


char *
cgx_info::pr_records(FILE *fp)
{
    sLstr lstr;
    char buf[256];
    for (int i = 0; i < CGX_NUM_REC_TYPES; i++) {
        if (rec_counts[i]) {
            sprintf(buf, "%-16s %d\n", cgx_record_names[i], rec_counts[i]);
            if (fp)
                fputs(buf, fp);
            else
                lstr.add(buf);
        }
    }
    return (lstr.string_trim());
}
// End of cgx_info functions


cgx_in::cgx_in(bool allow_layer_mapping) : cv_in(allow_layer_mapping)
{
    in_filetype = Fcgx;

    in_skip = false;
    in_no_create_layer = false;
    in_defsym = false;
    in_munit = 0.0;
    in_has_cprops = false;
    in_fp = 0;
    in_curlayer = 0;
    in_cellname = 0;
    in_layername = 0;

    ftab[R_LIBRARY]     = 0;
    ftab[R_STRUCT]      = &cgx_in::a_struct;
    ftab[R_CPRPTY]      = &cgx_in::a_cprpty;
    ftab[R_PROPERTY]    = &cgx_in::a_property;
    ftab[R_LAYER]       = &cgx_in::a_layer;
    ftab[R_BOX]         = &cgx_in::a_box;
    ftab[R_POLY]        = &cgx_in::a_poly;
    ftab[R_WIRE]        = &cgx_in::a_wire;
    ftab[R_TEXT]        = &cgx_in::a_text;
    ftab[R_SREF]        = &cgx_in::a_sref;
    ftab[R_ENDLIB]      = &cgx_in::a_endlib;
}


cgx_in::~cgx_in()
{
    delete in_fp;
    delete [] in_cellname;
    delete [] in_layername;
}


//
// Setup functions
//

// Open the source file.
//
bool
cgx_in::setup_source(const char *cgx_fname, const cCHD *chd)
{
    in_fp = sFilePtr::newFilePtr(cgx_fname, "r");
    if (!in_fp) {
        Errs()->sys_error("open");
        Errs()->add_error("Can't open CGX file %s.", cgx_fname);
        return (false);
    }
    in_filename = lstring::copy(cgx_fname);

    // The chd argument allows passing of the checksum for gzipped files
    // enabling use of random access table.
    if (chd && chd->crc())
        in_fp->z_set_crc(chd->crc());

    return (true);
}


// Set up for reading into main database.
//
bool
cgx_in::setup_to_database()
{
    in_action = cvOpenModeDb;
    return (true);
}


// Set up the destination channel.
//
bool
cgx_in::setup_destination(const char *destination, FileType ftype,
    bool to_cgd)
{
    if (destination) {
        in_out = FIO()->NewOutput(in_filename, destination, ftype, to_cgd);
        if (!in_out)
            return (false);
        in_own_in_out = true;
    }

    in_action = cvOpenModeTrans;

    ftab[R_LIBRARY]     = 0;
    ftab[R_STRUCT]      = &cgx_in::ac_struct;
    ftab[R_CPRPTY]      = &cgx_in::ac_cprpty;
    ftab[R_PROPERTY]    = &cgx_in::ac_property;
    ftab[R_LAYER]       = &cgx_in::ac_layer;
    ftab[R_BOX]         = &cgx_in::ac_box;
    ftab[R_POLY]        = &cgx_in::ac_poly;
    ftab[R_WIRE]        = &cgx_in::ac_wire;
    ftab[R_TEXT]        = &cgx_in::ac_text;
    ftab[R_SREF]        = &cgx_in::ac_sref;
    ftab[R_ENDLIB]      = &cgx_in::ac_endlib;

    return (true);
}


// Set to text output mode.
//
bool
cgx_in::setup_ascii_out(const char *txtfile, uint64_t start_ofs,
    uint64_t end_ofs, int numrecs, int numcells)
{
    in_print_fp = 0;
    if (txtfile) {
        if (!*txtfile || !strcmp(txtfile, "stdout"))
            in_print_fp = stdout;
        else if (!strcmp(txtfile, "stderr"))
            in_print_fp = stderr;
        else {
            in_print_fp = fopen(txtfile, "w");
            if (!in_print_fp) {
                Errs()->add_error("can't open %s, permission denied.",
                    txtfile);
                return (false);
            }
        }
        fprintf(in_print_fp, ">> %s\n", CD()->ifIdString());
        if (in_filename) {
            if (numcells > 0 || numrecs > 0 || start_ofs > 0 ||
                    end_ofs > start_ofs)
                fprintf(in_print_fp,
                    ">> PARTIAL text rendition of CGX file %s\n",
                    in_filename);
            else
                fprintf(in_print_fp, ">> Text rendition of CGX file %s.\n",
                    in_filename);
        }
    }
    in_action = cvOpenModePrint;

    in_print_start = start_ofs;
    in_print_end = end_ofs;
    in_print_reccnt = numrecs;
    in_print_symcnt = numcells;

    ftab[R_LIBRARY]     = 0;
    ftab[R_STRUCT]      = &cgx_in::ap_struct;
    ftab[R_CPRPTY]      = &cgx_in::ap_cprpty;
    ftab[R_PROPERTY]    = &cgx_in::ap_property;
    ftab[R_LAYER]       = &cgx_in::ap_layer;
    ftab[R_BOX]         = &cgx_in::ap_box;
    ftab[R_POLY]        = &cgx_in::ap_poly;
    ftab[R_WIRE]        = &cgx_in::ap_wire;
    ftab[R_TEXT]        = &cgx_in::ap_text;
    ftab[R_SREF]        = &cgx_in::ap_sref;
    ftab[R_ENDLIB]      = &cgx_in::ap_endlib;

    return (true);
}


// Explicitly set the back-end processor, and reset a few things.
//
bool
cgx_in::setup_backend(cv_out *out)
{
    if (in_fp)
        in_fp->z_rewind();
    in_bytes_read = 0;
    in_fb_incr = UFB_INCR;
    in_offset = 0;
    in_out = out;
    in_own_in_out = false;

    in_action = cvOpenModeTrans;

    ftab[R_LIBRARY]     = 0;
    ftab[R_STRUCT]      = &cgx_in::ac_struct;
    ftab[R_CPRPTY]      = &cgx_in::ac_cprpty;
    ftab[R_PROPERTY]    = &cgx_in::ac_property;
    ftab[R_LAYER]       = &cgx_in::ac_layer;
    ftab[R_BOX]         = &cgx_in::ac_box;
    ftab[R_POLY]        = &cgx_in::ac_poly;
    ftab[R_WIRE]        = &cgx_in::ac_wire;
    ftab[R_TEXT]        = &cgx_in::ac_text;
    ftab[R_SREF]        = &cgx_in::ac_sref;
    ftab[R_ENDLIB]      = &cgx_in::ac_endlib;

    return (true);
}


// Main entry for reading.  If sc is not 1.0, geometry will be scaled.
// If listonly is true, the file offsets will be saved in the name
// table, but there is no conversion.
//
bool
cgx_in::parse(DisplayMode mode, bool listonly, double sc, bool save_bb,
    cvINFO infoflags)
{
    if (listonly && in_action != cvOpenModeDb)
        return (false);
    in_mode = mode;
    if (mode == Physical) {
        in_ext_phys_scale = dfix(sc);
        in_scale = in_ext_phys_scale;
        in_needs_mult = (in_scale != 1.0);
        if (infoflags != cvINFOnone) {
            bool pl = (infoflags == cvINFOpl || infoflags == cvINFOplpc);
            bool pc = (infoflags == cvINFOpc || infoflags == cvINFOplpc);
            in_phys_info = new cgx_info(pl, pc);
            in_phys_info->initialize();
        }
    }
    else {
        in_scale = 1.0;
        in_needs_mult = false;
        if (infoflags != cvINFOnone) {
            bool pl = (infoflags == cvINFOpl || infoflags == cvINFOplpc);
            bool pc = (infoflags == cvINFOpc || infoflags == cvINFOplpc);
            in_elec_info = new cgx_info(pl, pc);
            in_elec_info->initialize();
        }
    }
    in_listonly = listonly;
    in_curlayer = 0;
    in_sdesc = 0;
    in_cellname = 0;
    in_cell_offset = 0;
    in_prpty_list = 0;
    in_has_cprops = false;
    in_header_read = false;

    in_savebb = save_bb;
    in_ignore_text = in_mode == Physical && FIO()->IsNoReadLabels();

    if (in_action == cvOpenModeDb && !in_listonly) {
        if (!read_header(false))
            return (false);
    }
    else {
        if (!read_header(true))
            return (false);
        if (in_action == cvOpenModeTrans && !in_cv_no_openlib) {
            if (!in_out->open_library(in_mode, 1.0))
                return (false);
        }
    }

    char buf[256];
    if (in_listonly)
        FIO()->ifPrintCvLog(IFLOG_INFO, "Building symbol table (%s).",
            DisplayModeNameLC(in_mode));

    int type, flags, size;
    while (get_record(&type, &flags, &size)) {
        if (type >= R_ENDLIB) {
            if (type == R_ENDLIB) {
                if (in_action == cvOpenModePrint && in_printing_done)
                    return (true);
                if (!(this->*ftab[R_ENDLIB])(size, flags))
                    return (false);
                if (in_mode == Physical)
                    FIO()->ifPrintCvLog(IFLOG_INFO, "End physical records");
                else
                    FIO()->ifPrintCvLog(IFLOG_INFO, "End electrical records");
                in_savebb = false;
                return (true);
            }
            sprintf(buf, "strange record type %d (ignored)", type);
            warning(buf);
            continue;
        }
        if (!(this->*ftab[type])(size, flags))
            return (false);
    }
    FIO()->ifPrintCvLog(IFLOG_FATAL, "Premature end of file");
    return (false);
}


//
// Entries for reading through CHD.
//

// Setup scaling and read the file header.
//
bool
cgx_in::chd_read_header(double phys_scale)
{
    bool ret = true;
    if (in_mode == Physical) {
        in_ext_phys_scale = dfix(phys_scale);
        in_scale = in_ext_phys_scale;
        in_phys_scale = in_scale;
        in_needs_mult = (in_scale != 1.0);

        ret = read_header(false);
    }
    else {
        in_scale = 1.0;
        in_needs_mult = false;

        // not supported in electrical mode
        in_areafilt = false;
        in_flatmax = -1;
        in_clip = false;
    }
    return (ret);
}


// Access a cell in the file through the symbol reference pointer. 
// This can be used for reading into memory or translation, and is not
// recursive.  The second arg is a return for the new cell opened in
// the database, if any.
//
bool
cgx_in::chd_read_cell(symref_t *p, bool use_inst_list, CDs **sdret)
{
    if (sdret)
        *sdret = 0;
    in_curlayer = 0;
    in_sdesc = 0;
    in_cellname = 0;
    in_cell_offset = 0;
    in_prpty_list = 0;
    in_has_cprops = false;
    in_uselist = true;

    // The AuxCellTab may contain cells to stream out from the main
    // database, overriding the cell data in the CHD.
    //
    if (in_mode == Physical && in_action == cvOpenModeTrans &&
            FIO()->IsUseCellTab()) {
        OItype oiret = chd_process_override_cell(p);
        if (oiret == OIerror)
            return (false);
        if (oiret == OInew)
            return (true);
    }

    if (!p->get_defseen()) {
        // No cell definition in file, just ignore this.
        return (true);
    }
    in_offset = p->get_offset();
    in_fp->z_seek(in_offset, SEEK_SET);

    CD()->InitCoincCheck();
    int type, flags, size;
    if (!get_record(&type, &flags, &size))
        return (false);
    if (type != R_STRUCT) {
        Errs()->add_error("no STRUCT record at offset.");
        return (false);
    }

    cv_chd_state stbak;
    in_chd_state.push_state(p, use_inst_list ? get_sym_tab(in_mode) : 0,
        &stbak);
    bool ret = (this->*ftab[R_STRUCT])(size, flags);
    if (ret) {
        if (in_sdesc || in_action == cvOpenModeTrans) {
            char buf[64];
            bool ok;
            while ((ok = get_record(&type, &flags, &size))) {
                if (type == R_STRUCT)
                    break;
                if (type >= R_ENDLIB) {
                    if (type == R_ENDLIB)
                        break;
                    sprintf(buf, "strange record type %d (ignored)", type);
                    warning(buf);
                    continue;
                }
                if (!(this->*ftab[type])(size, flags)) {
                    ret = false;
                    break;
                }
            }
            if (!ok)
                ret = false;
        }
    }

    if (sdret)
        *sdret = in_sdesc;
    if (ret && !end_struct())
        ret = false;
    in_chd_state.pop_state(&stbak);

    if (!ret && in_out && in_out->was_interrupted())
        in_interrupted = true;

    return (ret);
}


cv_header_info *
cgx_in::chd_get_header_info()
{
    if (!in_header_read)
        return (0);
    cgx_header_info *cgx = new cgx_header_info(in_munit);
    in_header_read = false;
    return (cgx);
}


void
cgx_in::chd_set_header_info(cv_header_info *hinfo)
{
    if (!hinfo || in_header_read)
        return;
    cgx_header_info *cgxh = static_cast<cgx_header_info*>(hinfo);
    in_munit = cgxh->unit();
    in_header_read = true;
}


//
// Misc. entries.
//

// Return true if electrical records present.
//
bool
cgx_in::has_electrical()
{
    int64_t posn = in_fp->z_tell();
    bool ok = true;
    if (in_fp->z_getc() != 'c' || in_fp->z_getc() != 'g' ||
            in_fp->z_getc() != 'x' || in_fp->z_getc() != 0)
        ok = false;
    in_fp->z_seek(posn, SEEK_SET);
    return (ok);
}


// Check if the cell contains any geometry that would make it through
// any layer filtering, overlapping AOI if given.  Subcells are
// ignored here.
// Returns:
//   OIaborted
//   OIerror
//   OIok          has geometry
//   OIambiguous   no geometry
//
OItype
cgx_in::has_geom(symref_t *p, const BBox *AOI)
{
    in_cellname = 0;
    in_cell_offset = 0;
    in_prpty_list = 0;
    in_has_cprops = false;
    in_uselist = true;

    if (!p->get_defseen()) {
        // No cell definition in file, just ignore this.
        return (OIambiguous);
    }
    in_offset = p->get_offset();
    in_fp->z_seek(in_offset, SEEK_SET);

    int type, flags, size;
    if (!get_record(&type, &flags, &size))
        return (OIerror);
    if (type != R_STRUCT)
        return (OIerror);

    in_sdesc = 0;
    in_skip = false;
    in_curlayer = 0;
    in_no_create_layer = true;

    bool ok;
    while ((ok = get_record(&type, &flags, &size))) {
        if (type == R_STRUCT)
            break;
        if (type >= R_ENDLIB) {
            if (type == R_ENDLIB)
                break;
            // unknown record, ignore
            continue;
        }
        if (type == R_LAYER) {
            if (!a_layer(size, flags))
                return (OIerror);
        }
        else if (type == R_BOX) {
            if (AOI && !in_skip) {
                in_skip = true;
                int nboxes = size/16;
                char *ptr = data_buf;
                for (int i = 0; i < nboxes; i++) {
                    int l = scale(long_value(&ptr));
                    int b = scale(long_value(&ptr));
                    int r = scale(long_value(&ptr));
                    int t = scale(long_value(&ptr));
                    BBox BB(l, b, r, t);
                    BB.fix();
                    if (BB.intersect(AOI, false)) {
                        in_skip = false;
                        break;
                    }
                }
            }
            if (!in_skip)
                return (OIok);
        }
        else if (type == R_POLY) {
            if (AOI && !in_skip) {
                char *ptr = data_buf;
                Poly po;
                po.numpts = size/8;
                po.points = new Point[po.numpts];
                for (int i = 0; i < po.numpts; i++) {
                    po.points[i].x = scale(long_value(&ptr));
                    po.points[i].y = scale(long_value(&ptr));
                }
                if (!po.intersect(AOI, false))
                    in_skip = true;
                delete [] po.points;
            }
            if (!in_skip)
                return (OIok);
        }
        else if (type == R_WIRE) {
            if (AOI && !in_skip) {
                char *ptr = data_buf;
                Wire w;
                w.numpts = (size - 4)/8;
                w.points = new Point[w.numpts];
                w.set_wire_width(scale(long_value(&ptr)));
                for (int i = 0; i < w.numpts; i++) {
                    w.points[i].x = scale(long_value(&ptr));
                    w.points[i].y = scale(long_value(&ptr));
                }
                if (flags != 0 && flags != 1 && flags != 2)
                    flags = 0;
                w.set_wire_style((WireStyle)flags);
                if (!w.intersect(AOI, false))
                    in_skip = true;
                delete [] w.points;
            }
            if (!in_skip)
                return (OIok);
        }
    }
    in_no_create_layer = false;
    if (!ok)
        return (OIerror);
    return (OIambiguous);
}
// End of virtual overrides


//
//---- Private Functions -----------
//


namespace {
    void check_factor(double fct, double munit)
    {
        if (fct < .999) {
            FIO()->ifPrintCvLog(IFLOG_WARN,
        "File resolution of %d is larger than the database\n"
        "**  resolution %d, loss by rounding can occur.  Suggest using the\n"
        "**  DatabaseResolution variable to set matching resolution when\n"
        "**  working with this file.\n**",
                mmRnd(1e-6/munit), CDphysResolution);
        }
    }
}

// Read the file header and set the scale, used before random read.
//
bool
cgx_in::read_header(bool quick)
{
    if (in_header_read) {
        if (in_mode == Physical) {
            double fct = 1e6*CDphysResolution*in_munit;
            in_scale = dfix(in_ext_phys_scale*fct);
            check_factor(fct, in_munit);
        }
        else
            in_scale = dfix(1e6*CDelecResolution*in_munit);
        in_needs_mult = (in_scale != 1.0);
        if (in_mode == Physical)
            in_phys_scale = in_scale;
        return (true);
    }

    const char *msg =  "file has incorrect format for CGX.";
    // test and skip past the "cgx\0" header
    if (in_fp->z_getc() != 'c' || in_fp->z_getc() != 'g' ||
            in_fp->z_getc() != 'x' || in_fp->z_getc() != 0) {
        Errs()->add_error(msg);
        return (false);
    }

    int type, flags, size;
    if (!get_record(&type, &flags, &size))
        return (false);
    if (type != R_LIBRARY) {
        Errs()->add_error(msg);
        return (false);
    }

    char *ptr = data_buf;
    in_munit = double_value(&ptr);
    if (in_mode == Physical) {
        double fct = 1e6*CDphysResolution*in_munit;
        in_scale = dfix(in_ext_phys_scale*fct);
        check_factor(fct, in_munit);
    }
    else
        in_scale = dfix(1e6*CDelecResolution*in_munit);
    in_needs_mult = (in_scale != 1.0);
    if (in_mode == Physical)
        in_phys_scale = in_scale;

    if (!quick) {
        if (in_mode == Physical) {
            double uunit = double_value(&ptr);
            tm cdate;
            date_value(&cdate, &ptr);
            tm mdate;
            date_value(&mdate, &ptr);

            char buf[256];
            char *cd = lstring::copy(asctime(&cdate));
            char *md = lstring::copy(asctime(&mdate));
            char *s = strchr(cd, '\n');
            if (s)
                *s = 0;
            s = strchr(md, '\n');
            if (s)
                *s = 0;
            sprintf(buf,
            "Opening CGX library: %s\nCreated %24s GMT\nModified %24s GMT\n",
                ptr, cd, md);
            delete [] cd;
            delete [] md;
            if (in_mode == Physical && in_munit != 1e-9) {
                FIO()->ifPrintCvLog(IFLOG_INFO,
                    "Units: m_unit=%.5e  u_unit=%.5e",
                    in_munit, uunit);
            }
            if (in_action == cvOpenModePrint) {
                fprintf(in_print_fp, "%s\n", buf);
                pr_record("LIBRARY", size, flags);
                fprintf(in_print_fp, "%12c munits=%g uunits=%g\n", ' ',
                    in_munit, uunit);
            }
            else
                FIO()->ifPrintCvLog(IFLOG_INFO, buf);
        }
        else {
            if (in_action == cvOpenModePrint)
                pr_record("LIBRARY", size, flags);
            else
                FIO()->ifPrintCvLog(IFLOG_INFO, "Begin electrical records");
        }
    }
    if (in_mode == Physical)
        in_header_read = true;
    return (true);
}


// Finalize cell reading, database and translation modes.
//
bool
cgx_in::end_struct()
{
    if (in_listonly) {
        if (in_savebb && in_symref)
            in_symref->set_bb(&in_cBB);
        if (in_cref_end) {
            nametab_t *ntab = get_sym_tab(in_mode);
            cref_t *c = ntab->find_cref(in_cref_end);
            c->set_last_cref(true);
            in_cref_end = 0;

            if (!in_symref) {
                Errs()->add_error("Internal error: null symbol reference.");
                return (false);
            }

            if (!ntab->cref_cmp_test(in_symref, in_cref_end, CRCMP_end))
                return (false);
        }
        in_symref = 0;
    }
    else if (in_action == cvOpenModeTrans) {
        if (in_transform <= 0) {
            if (in_defsym) {
                if (in_flatten_mode != cvTfFlatten) {
                    if (!in_out->write_end_struct())
                        return (false);
                }
                in_defsym = false;
            }
            delete [] in_layername;
            in_layername = 0;
        }
    }
    else if (in_sdesc) {
        if (in_mode == Physical) {
            if (!in_savebb && !in_no_test_empties && in_sdesc->isEmpty()) {
                FIO()->ifPrintCvLog(IFLOG_INFO,
                    "Cell %s physical definition is empty at offset %llu.",
                    in_cellname, (unsigned long long)in_cell_offset);
            }
        }
        else {
            // This sets up the pointers for bound labels.
            in_sdesc->prptyPatchAll();
        }
    }
    if (in_sdesc && in_has_cprops) {
        FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0, in_mode);
        stringlist *s0 = in_sdesc->prptyApplyList(0, &in_prpty_list);
        for (stringlist *s = s0; s; s = s->next)
            warning(s->string);
        s0->free();
        in_has_cprops = 0;

        in_sdesc->setPCellFlags();
        if (in_sdesc->isPCellSubMaster())
            in_sdesc->setPCellReadFromFile(true);
    }
    if (in_sdesc && FIO()->IsReadingLibrary()) {
        in_sdesc->setImmutable(true);
        in_sdesc->setLibrary(true);
    }
    in_sdesc = 0;
    in_curlayer = 0;
    clear_properties();
    return (true);
}


//
//===========================================================================
// Actions for creating cells in database
//===========================================================================
//

bool
cgx_in::a_struct(int, int)
{
    // end previous struct
    if (!end_struct())
        return (false);

    CD()->InitCoincCheck();

    char *ptr = data_buf;
    tm cdate;
    date_value(&cdate, &ptr);
    tm mdate;
    date_value(&mdate, &ptr);
    in_cell_offset = in_offset;

    delete [] in_cellname;
    if (in_chd_state.symref())
        in_cellname =
            lstring::copy(in_chd_state.symref()->get_name()->string());
    else
        in_cellname = lstring::copy(alias(ptr));
    char *cellname = ptr;

    if (in_savebb)
        in_cBB = CDnullBB;

    bool dup_sym = false;
    symref_t *srf = 0;
    if (in_uselist) {
        if (in_chd_state.symref())
            srf = in_chd_state.symref();
        else {
            // CHD name might be aliased or not.
            srf = get_symref(in_cellname, in_mode);
            if (!srf)
                srf = get_symref(cellname, in_mode);
            if (!srf) {
                Errs()->add_error("Can't find %s in CHD.", in_cellname);
                return (false);
            }
        }
    }
    else {
        nametab_t *ntab = get_sym_tab(in_mode);
        srf = get_symref(in_cellname, in_mode);
        if (srf && srf->get_defseen()) {
            FIO()->ifPrintCvLog(IFLOG_WARN,
                "Duplicate cell definition for %s at offset %llu.",
                in_cellname, (unsigned long long)in_offset);
            dup_sym = true;
        }

        if (!srf) {
            ntab->new_symref(in_cellname, in_mode, &srf);
            add_symref(srf, in_mode);
        }
        if (in_listonly) {
            // Flush instance list, this will be rebuilt.
            srf->set_cmpr(0);
            srf->set_crefs(0);
        }
        in_symref = srf;
        in_cref_end = 0;  // used as end pointer for cref_t list
        srf->set_defseen(true);
        srf->set_offset(in_offset);
    }
    if (info())
        info()->add_cell(srf);

    if (!in_listonly) {
        CDs *sd = CDcdb()->findCell(in_cellname, in_mode);
        if (!sd) {
            // doesn't already exist, open it
            sd = CDcdb()->insertCell(in_cellname, in_mode);
            sd->setFileType(Fcgx);
            sd->setFileName(in_filename);
            if (in_lcheck || in_layer_alias || in_phys_scale != 1.0)
                sd->setAltered(true);
            in_sdesc = sd;
        }
        else if (sd->isLibrary() && (FIO()->IsReadingLibrary() ||
                FIO()->IsNoOverwriteLibCells())) {
            // Skip reading into library cells.
            if (!FIO()->IsReadingLibrary()) {
                FIO()->ifPrintCvLog(IFLOG_WARN,
                    "can't overwrite library cell %s already in memory.",
                    in_cellname);
            }
        }
        else if (sd->isLibrary() && sd->isDevice()) {
            const char *nname = handle_lib_clash(in_cellname);
            if (nname) {
                delete [] in_cellname;
                in_cellname = lstring::copy(nname);
            }
        }
        else if (in_mode == Physical) {
            bool reuse = false;
            if (sd->isUnread()) {
                sd->setUnread(false);
                reuse = true;
            }
            else {
                mitem_t mi(in_cellname);
                if (sd->isViaSubMaster()) {
                    mi.overwrite_phys = false;
                    mi.overwrite_elec = false;
                }
                else if (dup_sym) {
                    mi.overwrite_phys = true;
                    mi.overwrite_elec = true;
                }
                else {
                    if (!FIO()->IsNoOverwritePhys())
                        mi.overwrite_phys = true;
                    if (!FIO()->IsNoOverwriteElec())
                        mi.overwrite_elec = true;
                    FIO()->ifMergeControl(&mi);
                }
                if (mi.overwrite_phys) {
                    sd->setImmutable(false);
                    sd->setLibrary(false);
                    sd->clear(true);
                    reuse = true;
                }
                // Save the electrical overwrite flag, presence in the
                // table means that we have prompted for this cell.
                if (!in_over_tab)
                    in_over_tab = new SymTab(false, false);
                in_over_tab->add((unsigned long)sd->cellname(),
                    (void*)(long)mi.overwrite_elec, false);
                if (mi.overwrite_elec) {
                    // If overwriting electrical, clear the existing
                    // electrical cell (if any) here.  There may not
                    // be a matching electrical cell from the file, in
                    // which case it won't get cleared in the code
                    // below.
                    CDs *tsd = CDcdb()->findCell(sd->cellname(), Electrical);
                    if (tsd) {
                        tsd->setImmutable(false);
                        tsd->setLibrary(false);
                        tsd->clear(true);
                    }
                }
            }
            if (reuse) {
                sd->setFileType(Fcgx);
                sd->setFileName(in_filename);
                if (in_lcheck || in_layer_alias || in_phys_scale != 1.0)
                    sd->setAltered(true);
                in_sdesc = sd;
            }
        }
        else {
            bool reuse = false;
            if (sd->isUnread()) {
                sd->setUnread(false);
                reuse = true;
            }
            else {
                void *xx;
                if (in_over_tab &&
                        (xx = in_over_tab->get((unsigned long)sd->cellname()))
                        != ST_NIL) {
                    // We already asked about overwriting.
                    if (xx) {
                        // User chose to overwrite the electrical
                        // part, the cell has already been cleared.
                        reuse = true;
                    }
                }
                else {
                    // Physical part of this cell (if any) has already
                    // been read, and there was no conflict.

                    mitem_t mi(in_cellname);
                    if (dup_sym) {
                        mi.overwrite_phys = true;
                        mi.overwrite_elec = true;
                    }
                    else {
                        if (!FIO()->IsNoOverwritePhys())
                            mi.overwrite_phys = true;
                        if (!FIO()->IsNoOverwriteElec())
                            mi.overwrite_elec = true;
                        FIO()->ifMergeControl(&mi);
                    }
                    if (mi.overwrite_phys) {
                        if (!get_symref(sd->cellname()->string(),
                                Physical)) {
                            // The phys cell was not read from the
                            // current file.  If overwriting, clear
                            // existing phys cell.
                            CDs *tsd = CDcdb()->findCell(sd->cellname(),
                                Physical);
                            if (tsd) {
                                tsd->setImmutable(false);
                                tsd->setLibrary(false);
                                tsd->clear(true);
                            }
                        }
                    }
                    if (mi.overwrite_elec) {
                        sd->setImmutable(false);
                        sd->setLibrary(false);
                        sd->clear(true);
                        reuse = true;
                    }
                }
            }
            if (reuse) {
                sd->setFileType(Fcgx);
                sd->setFileName(in_filename);
                if (in_lcheck || in_layer_alias || in_phys_scale != 1.0)
                    sd->setAltered(true);
                in_sdesc = sd;
            }
        }
    }
    return (true);
}


// The cell properties appear ahead of object property records, so we
// can use the same list head.

bool
cgx_in::a_cprpty(int, int)
{
    if (in_sdesc) {
        char *ptr = data_buf;
        int value = long_value(&ptr);
        CDp *p = new CDp(ptr, value);
        p->set_next_prp(in_prpty_list);
        in_prpty_list = p;
        in_has_cprops = true;
    }
    return (true);
}


bool
cgx_in::a_property(int, int)
{
    if (in_sdesc) {
        if (in_has_cprops) {
            FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0,
                in_mode);
            stringlist *s0 = in_sdesc->prptyApplyList(0, &in_prpty_list);
            for (stringlist *s = s0; s; s = s->next)
                warning(s->string);
            s0->free();
            clear_properties();
            in_has_cprops = 0;
        }
        char *ptr = data_buf;
        int value = long_value(&ptr);
        CDp *p = new CDp(ptr, value);
        p->set_next_prp(in_prpty_list);
        in_prpty_list = p;
    }
    return (true);
}


bool
cgx_in::a_layer(int, int)
{
    if (in_sdesc && in_has_cprops) {
        FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0,
            in_mode);
        stringlist *s0 = in_sdesc->prptyApplyList(0, &in_prpty_list);
        for (stringlist *s = s0; s; s = s->next)
            warning(s->string);
        s0->free();
        clear_properties();
        in_has_cprops = 0;
    }
    char *ptr = data_buf;
    int layer_num = short_value(&ptr);
    int data_type = short_value(&ptr);
    const char *layer_name = ptr;
    char buf[32];
    if (!*layer_name) {
        strmdata::hexpr(buf, layer_num, data_type);
        layer_name = buf;
    }
    in_skip = false;
    if (in_mode == Physical) {
        if (in_lcheck &&
                !in_lcheck->layer_ok(layer_name, layer_num, data_type)) {
            in_skip = true;
            return (true);
        }
        if (in_layer_alias) {
            const char *new_lname = in_layer_alias->alias(layer_name);
            if (new_lname)
                layer_name = new_lname;
        }
    }
    if (in_listonly) {
        if (info())
            info()->add_layer(layer_name);
        return (true);
    }
    in_curlayer = 0;
    if (layer_num >= 0) {
        CDll *lyrs = FIO()->GetGdsInputLayers(layer_num, data_type, in_mode);
        if (lyrs) {
            in_curlayer = lyrs->ldesc;
            lyrs->free();
        }
    }
    if (!in_curlayer)
        in_curlayer = CDldb()->findLayer(layer_name, in_mode);

    if (!in_no_create_layer) {
        if (!in_curlayer) {
            if (FIO()->IsNoCreateLayer()) {
                in_curlayer = CDldb()->findLayer(layer_name, in_mode);
                if (!in_curlayer) {
                    Errs()->add_error(
                    "unresolved layer, not allowed to create new layer %s.",
                        layer_name);
                    return (false);
                }
            }
            else {
                in_curlayer = CDldb()->newLayer(layer_name, in_mode);
                if (!in_curlayer) {
                    Errs()->add_error("could not create new layer %s.",
                        layer_name);
                    return (false);
                }
            }
        }
    }
    return (true);
}


bool
cgx_in::a_box(int size, int)
{
    if (in_sdesc && in_has_cprops) {
        FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0,
            in_mode);
        stringlist *s0 = in_sdesc->prptyApplyList(0, &in_prpty_list);
        for (stringlist *s = s0; s; s = s->next)
            warning(s->string);
        s0->free();
        clear_properties();
        in_has_cprops = 0;
    }
    if ((in_savebb || in_sdesc) && !in_skip) {
        int nboxes = size/16;
        if (info())
            info()->add_box(nboxes);
        char *ptr = data_buf;
        for (int i = 0; i < nboxes; i++) {
            int l = scale(long_value(&ptr));
            int b = scale(long_value(&ptr));
            int r = scale(long_value(&ptr));
            int t = scale(long_value(&ptr));
            BBox BB(l, b, r, t);
            BB.fix();

            if (in_savebb)
                in_cBB.add(&BB);
            else if (!in_areafilt || BB.intersect(&in_cBB, false)) {
                CDo *newo;
                CDerrType err = in_sdesc->makeBox(in_curlayer, &BB, &newo);
                if (err != CDok) {
                    if (err == CDbadBox) {
                        warning("bad box (ignored)", BB.left, BB.bottom);
                        continue;
                    }
                    clear_properties();
                    return (false);
                }
                if (newo) {
                    add_properties_db(newo);
                    if (FIO()->IsMergeInput() && in_mode == Physical) {
                        if (!in_sdesc->mergeBox(newo, false)) {
                            clear_properties();
                            return (false);
                        }
                    }
                }
            }
        }
    }
    clear_properties();
    return (true);
}


bool
cgx_in::a_poly(int size, int)
{
    if (in_sdesc && in_has_cprops) {
        FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0,
            in_mode);
        stringlist *s0 = in_sdesc->prptyApplyList(0, &in_prpty_list);
        for (stringlist *s = s0; s; s = s->next)
            warning(s->string);
        s0->free();
        clear_properties();
        in_has_cprops = 0;
    }
    if ((in_savebb || in_sdesc) && !in_skip) {
        if (info())
            info()->add_poly(size);
        char *ptr = data_buf;
        int nv = size/8;
        Poly po(nv, new Point[nv]);
        for (int i = 0; i < po.numpts; i++) {
            po.points[i].x = scale(long_value(&ptr));
            po.points[i].y = scale(long_value(&ptr));
        }
        if (po.is_rect()) {
            BBox BB(po.points);
            delete [] po.points;

            if (in_savebb)
                in_cBB.add(&BB);
            else if (!in_areafilt || BB.intersect(&in_cBB, false)) {
                CDo *newo;
                CDerrType err = in_sdesc->makeBox(in_curlayer, &BB, &newo);
                if (err != CDok) {
                    clear_properties();
                    if (err == CDbadBox) {
                        warning("bad rectangular polygon (ignored)",
                            BB.left, BB.bottom);
                        return (true);
                    }
                    return (false);
                }
                if (newo) {
                    add_properties_db(newo);
                    if (FIO()->IsMergeInput() && in_mode == Physical) {
                        if (!in_sdesc->mergeBox(newo, false)) {
                            clear_properties();
                            return (false);
                        }
                    }
                }
            }
        }
        else if (in_savebb) {
            BBox BB;
            po.computeBB(&BB);
            in_cBB.add(&BB);
            delete [] po.points;
        }
        else if (!in_areafilt || po.intersect(&in_cBB, false)) {
            CDpo *newo;
            int pchk_flags;
            int x = po.points->x;
            int y = po.points->y;
            CDerrType err =
                in_sdesc->makePolygon(in_curlayer, &po, &newo, &pchk_flags);
            if (err != CDok) {
                clear_properties();
                if (err == CDbadPolygon) {
                    warning("bad polygon (ignored)", x, y);
                    return (true);
                }
                return (false);
            }
            if (newo) {
                add_properties_db(newo);

                if (pchk_flags & PCHK_REENT)
                    warning("badly formed polygon", x, y);
                if (pchk_flags & PCHK_ZERANG) {
                    if (pchk_flags & PCHK_FIXED)
                        warning("repaired polygon 0/180 angle", x, y);
                    else
                        warning("polygon with 0/180 angle", x, y);
                }
            }
        }
    }
    clear_properties();
    return (true);
}


bool
cgx_in::a_wire(int size, int flags)
{
    if (in_sdesc && in_has_cprops) {
        FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0,
            in_mode);
        stringlist *s0 = in_sdesc->prptyApplyList(0, &in_prpty_list);
        for (stringlist *s = s0; s; s = s->next)
            warning(s->string);
        s0->free();
        clear_properties();
        in_has_cprops = 0;
    }
    if ((in_savebb || in_sdesc) && !in_skip) {
        if (info())
            info()->add_wire(size);
        char *ptr = data_buf;
        Wire w;
        w.numpts = (size - 4)/8;
        w.points = new Point[w.numpts];
        w.set_wire_width(scale(long_value(&ptr)));
        for (int i = 0; i < w.numpts; i++) {
            w.points[i].x = scale(long_value(&ptr));
            w.points[i].y = scale(long_value(&ptr));
        }
        if (flags != 0 && flags != 1 && flags != 2) {
            char buf[64];
            sprintf(buf, "unknown pathtype %d set to 0", flags);
            warning(buf, w.points->x, w.points->y);
            flags = 0;
        }
        w.set_wire_style((WireStyle)flags);

        if (in_savebb) {
            BBox BB;
            // warning: if dup verts, computeBB may fail
            Point::removeDups(w.points, &w.numpts);
            w.computeBB(&BB);
            in_cBB.add(&BB);
        }
        else if (!in_areafilt || w.intersect(&in_cBB, false)) {
            int x = w.points->x;
            int y = w.points->y;

            Point *pres = 0;
            int nres = 0;
            for (;;) {
                if (in_mode == Physical)
                    w.checkWireVerts(&pres, &nres);

                CDw *newo;
                int wchk_flags;
                CDerrType err = in_sdesc->makeWire(in_curlayer, &w, &newo,
                    &wchk_flags);

                if (err != CDok) {
                    if (err == CDbadWire)
                        warning("bad wire (ignored)", x, y);
                    else {
                        delete [] pres;
                        clear_properties();
                        return (false);
                    }
                }
                else if (newo) {
                    add_properties_db(newo);

                    if (wchk_flags) {
                        char buf[256];
                        Wire::flagWarnings(buf, wchk_flags,
                            "questionable wire, warnings: ");
                        warning(buf, x, y);
                    }
                }
                if (!pres)
                    break;
                warning("breaking colinear reentrant wire", x, y);
                w.points = pres;
                w.numpts = nres;
            }
        }
        delete [] w.points;
    }
    clear_properties();
    return (true);
}


bool
cgx_in::a_text(int size, int flags)
{
    if (in_sdesc && in_has_cprops) {
        FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0,
            in_mode);
        stringlist *s0 = in_sdesc->prptyApplyList(0, &in_prpty_list);
        for (stringlist *s = s0; s; s = s->next)
            warning(s->string);
        s0->free();
        clear_properties();
        in_has_cprops = 0;
    }
    if (!in_ignore_text && ((in_savebb || in_sdesc) && !in_skip)) {
        if (info())
            info()->add_text();
        char *ptr = data_buf;
        Label la;
        la.xform = cgxx2xicx(flags);
        la.x = scale(long_value(&ptr));
        la.y = scale(long_value(&ptr));
        la.width = scale(long_value(&ptr));

        // The label flags are stashed in a byte that follows the
        // label text, starting with 3.2.2.  This should be compatible
        // with earlier files, foreward and backward.  The string is
        // followed by a null byte, then the flags byte, then an
        // optional null pad byte.
        //
        int len = strlen(ptr);
        if (size - len - 12 > 1) {
            unsigned char flag_byte = ptr[len+1];
            if (flag_byte & CGX_LA_SHOW)
                la.xform |= TXTF_SHOW;
            else if (flag_byte & CGX_LA_HIDE)
                la.xform |= TXTF_HIDE;
            if (flag_byte & CGX_LA_TLEV)
                la.xform |= TXTF_TLEV;
            if (flag_byte & CGX_LA_LIML)
                la.xform |= TXTF_LIML;
        }

        hyList *hpl = new hyList(in_sdesc, ptr, HYcvAscii);
        char *string = hyList::string(hpl, HYcvPlain, false);
        // This is the displayed string, not necessarily the same as the
        // label text.
        double tw, th;
        CD()->DefaultLabelSize(string, in_mode, &tw, &th);
        la.height = mmRnd(la.width*th/tw);
        delete [] string;

        if (in_savebb) {
            BBox BB;
            la.computeBB(&BB);
            in_cBB.add(&BB);
        }
        else if (!in_areafilt || la.intersect(&in_cBB, false)) {
            la.label = hyList::dup(hpl);
            CDla *newo;
            CDerrType err = in_sdesc->makeLabel(in_curlayer, &la, &newo);
            if (err != CDok) {
                clear_properties();
                hyList::destroy(hpl);
                if (err == CDbadLabel) {
                    warning("bad label (ignored)", la.x, la.y);
                    return (true);
                }
                return (false);
            }
            if (newo)
                add_properties_db(newo);
        }
        hyList::destroy(hpl);
    }
    clear_properties();
    return (true);
}


bool
cgx_in::a_sref(int, int flags)
{
    if (in_sdesc && in_has_cprops) {
        FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0,
            in_mode);
        stringlist *s0 = in_sdesc->prptyApplyList(0, &in_prpty_list);
        for (stringlist *s = s0; s; s = s->next)
            warning(s->string);
        s0->free();
        clear_properties();
        in_has_cprops = 0;
    }
    if (in_chd_state.gen()) {
        if (!in_sdesc || !in_chd_state.symref()) {
            clear_properties();
            return (true);
        }
        nametab_t *ntab = get_sym_tab(in_mode);
        const cref_o_t *c = in_chd_state.gen()->next();
        if (!c) {
            Errs()->add_error(
                "Context instance list ended prematurely in %s.",
                in_chd_state.symref()->get_name());
            return (false);
        }
        symref_t *cp = ntab->find_symref(c->srfptr);
        if (!cp) {
            Errs()->add_error(
                "Unresolved symref back-pointer from %s.",
                in_chd_state.symref()->get_name());
            return (false);
        }
        if (cp->should_skip()) {
            clear_properties();
            return (true);
        }
        tristate_t fst = in_chd_state.get_inst_use_flag();
        if (fst == ts_unset) {
            clear_properties();
            return (true);
        }

        CDcellName cname = cp->get_name();
        CDattr at;
        if (!CD()->FindAttr(c->attr, &at)) {
            Errs()->add_error("Unresolved transform ticket %d.", c->attr);
            return (false);
        }
        int x, y, dx, dy;
        if (in_mode == Physical) {
            double tscale = in_ext_phys_scale;
            x = mmRnd(c->tx*tscale);
            y = mmRnd(c->ty*tscale);
            dx = mmRnd(at.dx*tscale);
            dy = mmRnd(at.dy*tscale);
        }
        else {
            x = c->tx;
            y = c->ty;
            dx = at.dx;
            dy = at.dy;
        }
        CDtx tx(at.refly, at.ax, at.ay, x, y, at.magn);
        CDap ap(at.nx, at.ny, dx, dy);

        cname = check_sub_master(cname);
        CallDesc calldesc(cname, 0);

        CDc *newo;
        if (OIfailed(in_sdesc->makeCall(&calldesc, &tx, &ap, CDcallDb, &newo)))
            return (false);
        FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0, in_mode);
        add_properties_db(newo);
        clear_properties();
        return (true);
    }

    if (info())
        info()->add_inst(flags & RF_ARRAY);

    if (!in_ignore_inst && (in_sdesc || (in_listonly && in_symref))) {
        char *ptr = data_buf;
        int x = scale(long_value(&ptr));
        int y = scale(long_value(&ptr));
        double angle = 0.0;
        if (flags & RF_ANGLE)
            angle = double_value(&ptr);
        double magn = 1.0;
        if (flags & RF_MAGN)
            magn = double_value(&ptr);
        int cols = 1, rows = 1;
        Point_c pt1, pt2;
        if (flags & RF_ARRAY) {
            cols = long_value(&ptr);
            rows = long_value(&ptr);
            pt1.x = scale(long_value(&ptr));
            pt1.y = scale(long_value(&ptr));
            pt2.x = scale(long_value(&ptr));
            pt2.y = scale(long_value(&ptr));
        }
        char *name = ptr;
        const char *cellname = alias(name);

        int ax, ay;
        char *emsg = FIO()->GdsParamSet(&angle, &magn, &ax, &ay);
        if (emsg) {
            warning(emsg, name, x, y);
            delete [] emsg;
        }

        int dx = 0, dy = 0;
        if (flags & RF_ARRAY) {
            // The pts are three coordinates of transformed points of the
            // box:
            //   g_points[0] (PREF)  0, 0
            //   g_points[1] (PCOL)  dx, 0
            //   g_points[2] (PROW)  0, dy
            // Apply the inverse transform to get dx, dy.
            //
            Point tp[3];
            tp[0].set(x, y);
            tp[1] = pt1;
            tp[2] = pt2;
            TPush();
            // defer de-magnification until after difference
            TApply(tp[0].x, tp[0].y, ax, ay, 1.0, (flags & RF_REFLECT));
            TInverse();
            TLoadInverse();
            TPoint(&tp[0].x, &tp[0].y);
            TPoint(&tp[1].x, &tp[1].y);
            TPoint(&tp[2].x, &tp[2].y);
            TPop();
            dx = mmRnd((tp[1].x - tp[0].x)/(cols*magn));
            dy = mmRnd((tp[2].y - tp[0].y)/(rows*magn));
        }
        if (in_listonly && in_symref) {
            nametab_t *ntab = get_sym_tab(in_mode);
            cref_t *sr;
            ticket_t ctk = ntab->new_cref(&sr);
            symref_t *srf = get_symref(cellname, in_mode);
            if (!srf) {
                sr->set_refptr(ntab->new_symref(cellname, in_mode, &srf));
                add_symref(srf, in_mode);
            }
            else
                sr->set_refptr(srf->get_ticket());

            CDtx tx(flags & RF_REFLECT, ax, ay, x, y, magn);
            sr->set_pos_x(tx.tx);
            sr->set_pos_y(tx.ty);
            CDattr at(&tx, 0);
            if (flags & RF_ARRAY) {
                at.nx = cols;
                at.ny = rows;
                at.dx = dx;
                at.dy = dy;
            }
            sr->set_tkt(CD()->RecordAttr(&at));

            // keep the list in order of appearance!
            if (!in_cref_end) {
                in_symref->set_crefs(ctk);
                if (!ntab->cref_cmp_test(in_symref, ctk, CRCMP_start))
                    return (false);
            }
            else {
                if (!ntab->cref_cmp_test(in_symref, ctk, CRCMP_check))
                    return (false);
            }
            in_cref_end = ctk;
        }
        else if (in_sdesc) {
            CDcellName cname = CD()->CellNameTableAdd(cellname);
            cname = check_sub_master(cname);
            CallDesc calldesc(cname, 0);

            // Spec says reflection about the X axis before rotation
            CDtx tx(flags & RF_REFLECT, ax, ay, x, y, magn);
            CDap ap(cols, rows, dx, dy);

            CDc *newo;
            if (OIfailed(in_sdesc->makeCall(&calldesc, &tx, &ap, CDcallDb,
                    &newo))) {
                clear_properties();
                return (false);
            }
            FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0,
                in_mode);
            add_properties_db(newo);

            if (in_symref) {
                // Add a symref for this instance if necessary.
                nametab_t *ntab = get_sym_tab(in_mode);
                symref_t *srf = get_symref(cname->string(), in_mode);
                if (!srf) {
                    ntab->new_symref(cname->string(), in_mode, &srf);
                    add_symref(srf, in_mode);
                }
            }
        }
    }
    clear_properties();
    return (true);
}


bool
cgx_in::a_endlib(int, int)
{
    return (end_struct());
}


//
//===========================================================================
// Actions for format translation
//===========================================================================
//

bool
cgx_in::ac_struct(int, int)
{
    if (in_transform <= 0) {
        // end previous struct
        if (!end_struct())
            return (false);

        char *ptr = data_buf;
        date_value(&in_cdate, &ptr);
        date_value(&in_mdate, &ptr);
        in_cell_offset = in_offset;
        delete [] in_cellname;
        if (info())
            info()->add_cell(in_chd_state.symref());
        if (in_chd_state.symref())
            in_cellname =
                lstring::copy(in_chd_state.symref()->get_name()->string());
        else
            in_cellname = lstring::copy(alias(ptr));

        // We now have to wait for the cell properties.  For this
        // conversion to work, the CPRPTY records must immediately
        // follow the STRUCT record.  When we see a LAYER, SREF or
        // PROPERTY record, we can actually dump the cell header.
    }

    return (true);
}


bool
cgx_in::ac_cprpty(int, int)
{
    char *ptr = data_buf;
    int value = long_value(&ptr);
    CDp *px = new CDp(ptr, value);

    px->scale(1.0, in_phys_scale, in_mode);
    bool ret = true;
    if (in_defsym)
        FIO()->ifPrintCvLog(IFLOG_WARN, "out of order cell property");
    else if (!in_out->queue_property(px->value(), px->string()))
        ret = false;
    delete px;
    return (ret);
}


bool
cgx_in::ac_property(int, int)
{
    bool ret = true;
    if (!in_defsym) {
        if (in_cellname)
            ret = in_out->write_struct(in_cellname, &in_cdate, &in_mdate);
        in_out->clear_property_queue();
        if (!ret)
            return (false);
        in_defsym = true;
    }
    char *ptr = data_buf;
    int value = long_value(&ptr);
    CDp *px = new CDp(ptr, value);

    px->scale(1.0, in_phys_scale, in_mode);
    ret = in_out->queue_property(px->value(), px->string());
    delete px;
    return (ret);
}


bool
cgx_in::ac_layer(int, int)
{
    if (!in_defsym) {
        bool ret = true;
        if (in_cellname)
            ret = in_out->write_struct(in_cellname, &in_cdate, &in_mdate);
        in_out->clear_property_queue();
        if (!ret)
            return (false);
        in_defsym = true;
    }
    char *ptr = data_buf;
    int layer_num = short_value(&ptr);
    int data_type = short_value(&ptr);
    const char *layer_name = ptr;
    char buf[32];
    if (!*layer_name) {
        strmdata::hexpr(buf, layer_num, data_type);
        layer_name = buf;
    }
    in_skip = false;
    if (in_mode == Physical) {
        if (in_lcheck &&
                !in_lcheck->layer_ok(layer_name, layer_num, data_type)) {
            in_skip = true;
            return (true);
        }
        if (in_layer_alias) {
            const char *new_lname = in_layer_alias->alias(layer_name);
            if (new_lname)
                layer_name = new_lname;
        }
    }
    if (!in_layername || strcmp(in_layername, layer_name)) {
        delete [] in_layername;
        in_layername = lstring::copy(layer_name);
        Layer layer(in_layername, layer_num, data_type);
        if (!in_out->queue_layer(&layer))
            return (false);
    }
    return (true);
}


bool
cgx_in::ac_box(int size, int)
{
    if (in_skip) {
        in_out->clear_property_queue();
        return (true);
    }

    int nboxes = size/16;
    if (info())
        info()->add_box(nboxes);

    bool ret = true;
    char *ptr = data_buf;
    for (int i = 0; i < nboxes; i++) {
        int l = scale(long_value(&ptr));
        int b = scale(long_value(&ptr));
        int r = scale(long_value(&ptr));
        int t = scale(long_value(&ptr));
        BBox BB(l, b, r, t);
        BB.fix();

        if (in_tf_list) {
            ts_reader tsr(in_tf_list);
            CDtx ttx;
            CDap ap;
            BBox BBbak = BB;

            bool wrote_once = false;
            while (tsr.read_record(&ttx, &ap)) {
                ttx.scale(in_ext_phys_scale);
                ap.scale(in_ext_phys_scale);
                TPush();
                TApply(ttx.tx, ttx.ty, ttx.ax, ttx.ay, ttx.magn, ttx.refly);

                if (in_out->size_test(in_chd_state.symref(), this)) {
                    TPop();
                    continue;
                }
                in_transform++;

                if (ap.nx > 1 || ap.ny > 1) {
                    int tx, ty;
                    TGetTrans(&tx, &ty);
                    xyg_t xyg(0, ap.nx-1, 0, ap.ny-1);
                    do {
                        TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                        if (wrote_once)
                            BB = BBbak;
                        ret = ac_box_prv(BB);
                        wrote_once = true;
                        if (!ret)
                            break;
                        TSetTrans(tx, ty);
                    } while (xyg.advance());
                }
                else {
                    if (wrote_once)
                        BB = BBbak;
                    ret = ac_box_prv(BB);
                    wrote_once = true;
                }

                in_transform--;
                TPop();
                if (!ret)
                    break;
            }
        }
        else
            ret = ac_box_prv(BB);

        if (!ret)
            break;
    }
    in_out->clear_property_queue();
    return (ret);

}


bool
cgx_in::ac_box_prv(BBox &BB)
{
    bool ret = true;
    if (in_transform) {
        Point *pp;
        TBB(&BB, &pp);
        if (pp) {
            Poly po(5, pp);
            if (!in_areafilt || po.intersect(&in_cBB, false)) {
                TPush();
                bool need_out = true;
                if (in_areafilt && in_clip) {
                    need_out = false;
                    PolyList *pl = po.clip(&in_cBB, &need_out);
                    for (PolyList *p = pl; p; p = p->next) {
                        ret = in_out->write_poly(&p->po);
                        if (!ret)
                            break;
                    }
                    PolyList::destroy(pl);
                }
                if (need_out)
                    ret = in_out->write_poly(&po);
                TPop();
            }
            delete [] pp;
            return (ret);
        }
    }
    if (!in_areafilt || BB.intersect(&in_cBB, false)) {
        if (in_areafilt && in_clip) {
            if (BB.left < in_cBB.left)
                BB.left = in_cBB.left;
            if (BB.bottom < in_cBB.bottom)
                BB.bottom = in_cBB.bottom;
            if (BB.right > in_cBB.right)
                BB.right = in_cBB.right;
            if (BB.top > in_cBB.top)
                BB.top = in_cBB.top;
        }
        TPush();
        ret = in_out->write_box(&BB);
        TPop();
    }
    return (ret);
}


bool
cgx_in::ac_poly(int size, int)
{
    bool ret = true;
    if (in_skip) {
        in_out->clear_property_queue();
        return (ret);
    }

    if (info())
        info()->add_poly(size);
    char *ptr = data_buf;
    Poly po;
    po.numpts = size/8;
    po.points = new Point[po.numpts];
    for (int i = 0; i < po.numpts; i++) {
        po.points[i].x = scale(long_value(&ptr));
        po.points[i].y = scale(long_value(&ptr));
    }
    if (in_tf_list) {
        ts_reader tsr(in_tf_list);
        CDtx ttx;
        CDap ap;
        Point *scratch = new Point[po.numpts];
        memcpy(scratch, po.points, po.numpts*sizeof(Point));

        bool wrote_once = false;
        while (tsr.read_record(&ttx, &ap)) {
            ttx.scale(in_ext_phys_scale);
            ap.scale(in_ext_phys_scale);
            TPush();
            TApply(ttx.tx, ttx.ty, ttx.ax, ttx.ay, ttx.magn, ttx.refly);

            if (in_out->size_test(in_chd_state.symref(), this)) {
                TPop();
                continue;
            }
            in_transform++;

            if (ap.nx > 1 || ap.ny > 1) {
                int tx, ty;
                TGetTrans(&tx, &ty);
                xyg_t xyg(0, ap.nx-1, 0, ap.ny-1);
                do {
                    TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                    if (wrote_once)
                        memcpy(po.points, scratch, po.numpts*sizeof(Point));
                    ret = ac_poly_prv(po);
                    wrote_once = true;
                    if (!ret)
                        break;
                    TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            else {
                if (wrote_once)
                    memcpy(po.points, scratch, po.numpts*sizeof(Point));
                ret = ac_poly_prv(po);
                wrote_once = true;
            }

            in_transform--;
            TPop();
            if (!ret)
                break;
        }
        delete [] scratch;
    }
    else
        ret = ac_poly_prv(po);

    delete [] po.points;
    in_out->clear_property_queue();
    return (ret);
}


bool
cgx_in::ac_poly_prv(Poly &po)
{
    bool ret = true;
    if (in_transform)
        TPath(po.numpts, po.points);
    if (po.is_rect()) {
        BBox BB(po.points);

        if (!in_areafilt || BB.intersect(&in_cBB, false)) {
            if (in_areafilt && in_clip) {
                if (BB.left < in_cBB.left)
                    BB.left = in_cBB.left;
                if (BB.bottom < in_cBB.bottom)
                    BB.bottom = in_cBB.bottom;
                if (BB.right > in_cBB.right)
                    BB.right = in_cBB.right;
                if (BB.top > in_cBB.top)
                    BB.top = in_cBB.top;
            }
            TPush();
            ret = in_out->write_box(&BB);
            TPop();
        }
    }
    else {
        if (!in_areafilt || po.intersect(&in_cBB, false)) {
            TPush();
            bool need_out = true;
            if (in_areafilt && in_clip) {
                need_out = false;
                PolyList *pl = po.clip(&in_cBB, &need_out);
                for (PolyList *p = pl; p; p = p->next) {
                    ret = in_out->write_poly(&p->po);
                    if (!ret)
                        break;
                }
                PolyList::destroy(pl);
            }
            if (need_out)
                ret = in_out->write_poly(&po);
            TPop();
        }
    }
    return (ret);
}


bool
cgx_in::ac_wire(int size, int flags)
{
    if (in_skip) {
        in_out->clear_property_queue();
        return (true);
    }
    if (info())
        info()->add_wire(size);
    char *ptr = data_buf;

    Wire w;
    w.numpts = (size - 4)/8;
    w.points = new Point[w.numpts];
    int pwidth = scale(long_value(&ptr));
    w.set_wire_width(pwidth);
    for (int i = 0; i < w.numpts; i++) {
        w.points[i].x = scale(long_value(&ptr));
        w.points[i].y = scale(long_value(&ptr));
    }
    if (flags != 0 && flags != 1 && flags != 2) {
        char buf[64];
        sprintf(buf, "unknown pathtype %d, set to 0", flags);
        warning(buf, w.points->x, w.points->y);
        flags = 0;
    }
    w.set_wire_style((WireStyle)flags);

    bool ret = true;
    if (in_tf_list) {
        ts_reader tsr(in_tf_list);
        CDtx ttx;
        CDap ap;
        Point *scratch = new Point[w.numpts];
        memcpy(scratch, w.points, w.numpts*sizeof(Point));

        bool wrote_once = false;
        while (tsr.read_record(&ttx, &ap)) {
            ttx.scale(in_ext_phys_scale);
            ap.scale(in_ext_phys_scale);
            TPush();
            TApply(ttx.tx, ttx.ty, ttx.ax, ttx.ay, ttx.magn, ttx.refly);

            if (in_out->size_test(in_chd_state.symref(), this)) {
                TPop();
                continue;
            }
            in_transform++;

            if (ap.nx > 1 || ap.ny > 1) {
                int tx, ty;
                TGetTrans(&tx, &ty);
                xyg_t xyg(0, ap.nx-1, 0, ap.ny-1);
                do {
                    TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                    if (wrote_once) {
                        memcpy(w.points, scratch, w.numpts*sizeof(Point));
                        w.set_wire_width(pwidth);
                    }
                    ret = ac_wire_prv(w);
                    wrote_once = true;
                    if (!ret)
                        break;
                    TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            else {
                if (wrote_once) {
                    memcpy(w.points, scratch, w.numpts*sizeof(Point));
                    w.set_wire_width(pwidth);
                }
                ret = ac_wire_prv(w);
                wrote_once = true;
            }

            in_transform--;
            TPop();
            if (!ret)
                break;
        }
        delete [] scratch;
    }
    else
        ret = ac_wire_prv(w);

    delete [] w.points;
    in_out->clear_property_queue();
    return (ret);
}


bool
cgx_in::ac_wire_prv(Wire &w)
{
    bool ret = true;
    if (in_transform) {
        TPath(w.numpts, w.points);
        w.set_wire_width(mmRnd(w.wire_width() * TGetMagn()));
    }

    Poly wp;
    if (!in_areafilt || (w.toPoly(&wp.points, &wp.numpts) &&
            wp.intersect(&in_cBB, false))) {
        TPush();
        bool need_out = true;
        if (in_areafilt && in_clip) {
            need_out = false;
            PolyList *pl = wp.clip(&in_cBB, &need_out);
            for (PolyList *p = pl; p; p = p->next) {
                ret = in_out->write_poly(&p->po);
                if (!ret)
                    break;
            }
            PolyList::destroy(pl);
        }
        if (need_out)
            ret = in_out->write_wire(&w);
        TPop();
    }
    delete [] wp.points;
    return (ret);
}


bool
cgx_in::ac_text(int size, int flags)
{
    if (in_ignore_text || in_skip) {
        in_out->clear_property_queue();
        return (true);
    }
    if (info())
        info()->add_text();
    char *ptr = data_buf;

    Text text;
    text.xform = cgxx2xicx(flags);
    text.x = scale(long_value(&ptr));
    text.y = scale(long_value(&ptr));
    text.width = scale(long_value(&ptr));

    // The label flags are stashed in a byte that follows the label
    // text, starting with 3.2.2.  This should be compatible with
    // earlier files, foreward and backward.  The string is followed
    // by a null byte, then the flags byte, then an optional null pad
    // byte.
    //
    int len = strlen(ptr);
    if (size - len - 12 > 1) {
        unsigned char flag_byte = ptr[len+1];
        if (flag_byte & CGX_LA_SHOW)
            text.xform |= TXTF_SHOW;
        else if (flag_byte & CGX_LA_HIDE)
            text.xform |= TXTF_HIDE;
        if (flag_byte & CGX_LA_TLEV)
            text.xform |= TXTF_TLEV;
        if (flag_byte & CGX_LA_LIML)
            text.xform |= TXTF_LIML;
    }

    hyList *hpl = new hyList(in_sdesc, ptr, HYcvAscii);
    char *string = hyList::string(hpl, HYcvPlain, false);
    // This is the displayed string, not necessarily the same as the
    // label text.
    double tw, th;
    CD()->DefaultLabelSize(string, in_mode, &tw, &th);
    text.height = mmRnd(text.width*th/tw);
    delete [] string;
    hyList::destroy(hpl);
    text.text = ptr;

    bool ret = true;
    if (in_tf_list) {
        ts_reader tsr(in_tf_list);
        CDtx ttx;
        CDap ap;
        while (tsr.read_record(&ttx, &ap)) {
            ttx.scale(in_ext_phys_scale);
            ap.scale(in_ext_phys_scale);
            TPush();
            TApply(ttx.tx, ttx.ty, ttx.ax, ttx.ay, ttx.magn, ttx.refly);

            if (in_out->size_test(in_chd_state.symref(), this)) {
                TPop();
                continue;
            }
            in_transform++;

            if (ap.nx > 1 || ap.ny > 1) {
                int tx, ty;
                TGetTrans(&tx, &ty);
                xyg_t xyg(0, ap.nx-1, 0, ap.ny-1);
                do {
                    TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                    ret = ac_text_prv(text);
                    if (!ret)
                        break;
                    TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            else
                ret = ac_text_prv(text);

            in_transform--;
            TPop();
            if (!ret)
                break;
        }
    }
    else
        ret = ac_text_prv(text);

    in_out->clear_property_queue();
    return (ret);
}


bool
cgx_in::ac_text_prv(const Text &ctext)
{
    Text text(ctext);
    if (in_transform)
        text.transform(this);

    if (in_areafilt) {
        BBox lBB(0, 0, text.width, text.height);
        TSetTransformFromXform(text.xform, text.width, text.height);
        TTranslate(text.x, text.y);
        Poly po(5, 0);
        TBB(&lBB, &po.points);
        TPop();

        bool ret = bound_isect(&in_cBB, &lBB, &po);
        delete [] po.points;
        if (!ret)
            return (true);
        if (in_clip && !in_keep_clip_text && !(in_cBB >= lBB))
            return (true);
    }
    return (in_out->write_text(&text));
}


bool
cgx_in::ac_sref(int, int flags)
{
    if (!in_defsym) {
        bool ret = true;
        if (in_cellname)
            ret = in_out->write_struct(in_cellname, &in_cdate, &in_mdate);
        in_out->clear_property_queue();
        if (!ret)
            return (false);
        in_defsym = true;
    }

    if (in_chd_state.gen()) {
        if (!in_chd_state.symref()) {
            in_out->clear_property_queue();
            return (true);
        }
        nametab_t *ntab = get_sym_tab(in_mode);
        const cref_o_t *c = in_chd_state.gen()->next();
        if (!c) {
            Errs()->add_error(
                "Context instance list ended prematurely in %s.",
                in_chd_state.symref()->get_name());
            return (false);
        }
        symref_t *cp = ntab->find_symref(c->srfptr);
        if (!cp) {
            Errs()->add_error(
                "Unresolved symref back-pointer from %s.",
                in_chd_state.symref()->get_name());
            return (false);
        }
        if (cp->should_skip()) {
            in_out->clear_property_queue();
            return (true);
        }
        tristate_t fst = in_chd_state.get_inst_use_flag();
        if (fst == ts_unset) {
            in_out->clear_property_queue();
            return (true);
        }

        CDcellName cellname = cp->get_name();
        CDattr at;
        if (!CD()->FindAttr(c->attr, &at)) {
            Errs()->add_error("Unresolved transform ticket %d.", c->attr);
            return (false);
        }
        int x, y, dx, dy;
        if (in_mode == Physical) {
            double tscale = in_ext_phys_scale;
            x = mmRnd(c->tx*tscale);
            y = mmRnd(c->ty*tscale);
            dx = mmRnd(at.dx*tscale);
            dy = mmRnd(at.dy*tscale);
        }
        else {
            x = c->tx;
            y = c->ty;
            dx = at.dx;
            dy = at.dy;
        }

        Instance inst;
        inst.magn = at.magn;
        inst.name = cellname->string();
        inst.nx = at.nx;
        inst.ny = at.ny;
        inst.dx = dx;
        inst.dy = dy;
        inst.origin.set(x, y);
        inst.reflection = at.refly;
        inst.set_angle(at.ax, at.ay);

        if (!ac_sref_backend(&inst, cp, (fst == ts_set)))
            return (false);
        in_out->clear_property_queue();
        return (true);
    }

    if (info())
        info()->add_inst(flags & RF_ARRAY);

    bool ret = true;
    if (!in_ignore_inst) {
        char *ptr = data_buf;

        int x = long_value(&ptr);
        int y = long_value(&ptr);
        double angle = 0.0;
        if (flags & RF_ANGLE)
            angle = double_value(&ptr);
        double magn = 1.0;
        if (flags & RF_MAGN)
            magn = double_value(&ptr);
        int cols = 1, rows = 1;
        Point_c pt1, pt2;
        if (flags & RF_ARRAY) {
            cols = long_value(&ptr);
            rows = long_value(&ptr);
            pt1.x = long_value(&ptr);
            pt1.y = long_value(&ptr);
            pt2.x = long_value(&ptr);
            pt2.y = long_value(&ptr);
        }
        char *name = ptr;
        const char *cname = alias(name);

        int ax, ay;
        char *emsg = FIO()->GdsParamSet(&angle, &magn, &ax, &ay);
        if (emsg) {
            char buf[256];
            sprintf(buf, "%s for instance of %s", emsg, name);
            warning(emsg, name, x, y);
            delete [] emsg;
        }

        int dx = 0, dy = 0;
        if (flags & RF_ARRAY) {
            // The pts are three coordinates of transformed points of the
            // box:
            //   g_points[0] (PREF)  0, 0
            //   g_points[1] (PCOL)  dx, 0
            //   g_points[2] (PROW)  0, dy
            // Apply the inverse transform to get dx, dy.
            //
            Point tp[3];
            tp[0].set(x, y);
            tp[1] = pt1;
            tp[2] = pt2;
            TPush();
            // defer de-magnification until after difference
            TApply(tp[0].x, tp[0].y, ax, ay, 1.0, (flags & RF_REFLECT));
            TInverse();
            TLoadInverse();
            TPoint(&tp[0].x, &tp[0].y);
            TPoint(&tp[1].x, &tp[1].y);
            TPoint(&tp[2].x, &tp[2].y);
            TPop();
            dx = mmRnd((tp[1].x - tp[0].x)/(cols*magn));
            dy = mmRnd((tp[2].y - tp[0].y)/(rows*magn));
        }

        Instance inst;
        inst.magn = magn;
        inst.angle = angle;
        inst.name = cname;
        inst.nx = cols;
        inst.ny = rows;
        inst.dx = scale(dx);
        inst.dy = scale(dy);
        inst.origin.set(scale(x), scale(y));
        inst.reflection = flags & RF_REFLECT;
        inst.ax = ax;
        inst.ay = ay;

        ret = ac_sref_backend(&inst, 0, false);
    }
    in_out->clear_property_queue();
    return (ret);
}


bool
cgx_in::ac_sref_backend(Instance *inst, symref_t *p, bool no_area_test)
{
    bool ret = true;
    if (in_flatmax >= 0 && in_transform < in_flatmax) {
        // Flattening (not used)
        return (false);
    }
    else if (in_areafilt) {
        //
        // Area filtering
        //
        if (p) {
            //
            // Using CHD to reference cells.  If a vec_ctab was given,
            // use it to resolve the bounding box.
            //
            if (!no_area_test) {
                const BBox *tsBB;
                if (in_chd_state.ctab()) {
                    tsBB = in_chd_state.ctab()->resolve_BB(
                        &p, in_chd_state.ct_index());
                    if (!tsBB)
                        return (true);
                }
                else
                    tsBB = p->get_bb();

                if (!tsBB) {
                    Errs()->add_error("Bounding box for master %s not found.",
                        p->get_name()->string());
                    return (false);
                }
                BBox sBB(*tsBB);
                if (in_mode == Physical)
                    sBB.scale(in_ext_phys_scale);

                no_area_test = inst->check_overlap(this, &sBB, &in_cBB);
            }
            if (no_area_test)
                ret = in_out->write_sref(inst);
        }
    }
    else
        ret = in_out->write_sref(inst);
    return (ret);
}


bool
cgx_in::ac_endlib(int, int)
{
    if (!end_struct())
        return (false);
    if (!in_cv_no_endlib) {
        if (!in_out->write_endlib(0))
            return (false);
    }
    return (true);
}


//
//===========================================================================
// Actions for printing records in ascii text format
//===========================================================================
//

bool
cgx_in::ap_struct(int size, int flags)
{
    clear_properties();
    if (in_print_symcnt > 0)
        in_print_symcnt--;
    if (in_print_symcnt == 0)
        return (true);

    char *ptr = data_buf;
    tm cdate;
    date_value(&cdate, &ptr);
    tm mdate;
    date_value(&mdate, &ptr);
    delete [] in_cellname;
    in_cell_offset = in_offset;
    in_cellname = lstring::copy(ptr);

    pr_record("STRUCT", size, flags);
    if (in_printing)
        fprintf(in_print_fp, "%12c name=%s\n", ' ', ptr);
    return (true);
}


bool
cgx_in::ap_cprpty(int size, int flags)
{
    char *ptr = data_buf;
    int value = long_value(&ptr);
    pr_record("CPRPTY", size, flags);
    if (in_printing)
        fprintf(in_print_fp, "%12c num=%d string=%s\n", ' ', value, ptr);
    return (true);
}


bool
cgx_in::ap_property(int size, int flags)
{
    char *ptr = data_buf;
    int value = long_value(&ptr);
    pr_record("PROPERTY", size, flags);
    if (in_printing)
        fprintf(in_print_fp, "%12c num=%d string=%s\n", ' ', value, ptr);
    return (true);
}


bool
cgx_in::ap_layer(int size, int flags)
{
    char *ptr = data_buf;
    int layer_num = short_value(&ptr);
    int data_type = short_value(&ptr);
    char *layer_name = ptr;
    pr_record("LAYER", size, flags);
    if (in_printing)
        fprintf(in_print_fp, "%12c layer=%d dtype=%d name=%s\n", ' ',
            layer_num, data_type, layer_name);
    return (true);
}


bool
cgx_in::ap_box(int size, int flags)
{
    int nboxes = size/16;
    if (info())
        info()->add_box(nboxes);
    pr_record("BOX", size, flags);
    if (in_printing)
        fprintf(in_print_fp, "%12c count=%d\n", ' ', nboxes);
    return (true);
}


bool
cgx_in::ap_poly(int size, int flags)
{
    if (info())
        info()->add_poly(size);
    pr_record("POLY", size, flags);
    return (true);
}


bool
cgx_in::ap_wire(int size, int flags)
{
    if (info())
        info()->add_wire(size);
    pr_record("WIRE", size, flags);
    return (true);
}


bool
cgx_in::ap_text(int size, int flags)
{
    if (info())
        info()->add_text();
    char *ptr = data_buf;
    Label la;
    la.xform = flags;
    la.x = long_value(&ptr);
    la.y = long_value(&ptr);
    la.width = long_value(&ptr);
    pr_record("TEXT", size, flags);
    if (in_printing)
        fprintf(in_print_fp, "%12c xform=%x x=%d y=%d width=%d label=%s\n",
            ' ', la.xform, la.x, la.y, la.width, ptr);
    return (true);
}


bool
cgx_in::ap_sref(int size, int flags)
{
    char *ptr = data_buf;

    int x = scale(long_value(&ptr));
    int y = scale(long_value(&ptr));
    double angle = 0.0;
    if (flags & RF_ANGLE)
        angle = double_value(&ptr);
    double magn = 1.0;
    if (flags & RF_MAGN)
        magn = double_value(&ptr);
    int cols = 1, rows = 1;
    Point_c pt1, pt2;
    if (info())
        info()->add_inst(flags & RF_ARRAY);
    if (flags & RF_ARRAY) {
        cols = long_value(&ptr);
        rows = long_value(&ptr);
        pt1.x = scale(long_value(&ptr));
        pt1.y = scale(long_value(&ptr));
        pt2.x = scale(long_value(&ptr));
        pt2.y = scale(long_value(&ptr));
    }
    char *name = ptr;

    pr_record("SREF", size, flags);
    if (in_printing)
        fprintf(in_print_fp,
            "%12c x=%d y=%d name=%s row=%d col=%d ang=%g mag=%g refl=%d\n",
            ' ', x, y, name, rows, cols, angle, magn,
            (flags & RF_REFLECT) ? 1:0);
    return (true);
}


bool
cgx_in::ap_endlib(int, int)
{
    clear_properties();
    return (true);
}
// End of actions


void
cgx_in::add_properties_db(CDo *odesc)
{
    if (in_sdesc && odesc) {
        stringlist *s0 = in_sdesc->prptyApplyList(odesc, &in_prpty_list);
        for (stringlist *s = s0; s; s = s->next)
            warning(s->string);
        delete s0;
    }
}


void
cgx_in::clear_properties()
{
    in_prpty_list->free_list();
    in_prpty_list = 0;
}


// Generate a warning log message, format is
//  string [cellname offset]
//
void
cgx_in::warning(const char *str)
{
    FIO()->ifPrintCvLog(IFLOG_WARN, "%s [%s %llu]",
        str, in_cellname, (unsigned long long)in_offset);
}


// Generate a warning log message, format is
//  string [cellname x,y layername offset]
//
void
cgx_in::warning(const char *str, int x, int y)
{
    const char *lname = "";
    if (in_curlayer)
        lname = in_curlayer->name();
    else
        lname = in_layername;

    FIO()->ifPrintCvLog(IFLOG_WARN, "%s [%s %d,%d %s %llu]", str,
        in_cellname, x, y, lname, (unsigned long long)in_offset);
}


// Generate a warning log message, format is
//  string for instance of mstr [cellname x,y offset]
//
void
cgx_in::warning(const char *str, const char *mstr, int x, int y)
{
    FIO()->ifPrintCvLog(IFLOG_WARN,
        "%s for instance of %s [%s %d,%d %llu]",
        str, mstr, in_cellname, x, y, (unsigned long long)in_offset);
}


bool
cgx_in::get_record(int *rectype, int *flags, int *size)
{
    in_offset = in_fp->z_tell();

    if (in_bytes_read > in_fb_incr) {
        show_feedback();
        if (in_interrupted) {
            Errs()->add_error("user interrupt");
            return (false);
        }
    }

    unsigned char bytes[4];
    if (in_fp->z_read(bytes, 1, 4) != 4) {
        if (in_fp->z_eof())
            Errs()->add_error("Premature end of file.");
        else
            Errs()->add_error("Read error on input.");
        return (false);
    }
    unsigned rtype = bytes[2];
    *flags = bytes[3];

    if (in_action == cvOpenModePrint) {
        in_printing = (in_offset >= in_print_start);
        if (in_print_reccnt > 0)
            in_print_reccnt--;
        if (in_print_reccnt == 0 || in_print_symcnt == 0 ||
                (in_print_end > in_print_start && in_offset > in_print_end)) {
            // Stop printing.
            in_printing_done = true;
            *rectype = R_ENDLIB;
            *flags = 0;
            *size = 0;
            return (true);
        }
    }

    int sz = bytes[0]*256 + bytes[1] - 4;
    if (sz < 0)
        sz = 0;

    if (sz & 1) {
        // "CGX records are always an even number of bytes"
        Errs()->add_error("Odd-size record found, size %d.", sz);
        return (false);
    }
    *size = sz;
    sz >>= 1;
    if (sz && in_fp->z_read(data_buf, sz, 2) != 2) {
        if (in_fp->z_eof())
            Errs()->add_error("Premature end of file.");
        else
            Errs()->add_error("Read error on input.");
        return (false);
    }
    sz <<= 1;
    data_buf[sz] = '\0';  // Null terminate strings
    *rectype = rtype;
    if (info())
        info()->add_record(rtype);
    in_bytes_read += sz + 4;
    return (true);
}


int
cgx_in::short_value(char **p)
{
    unsigned char *b = (unsigned char *)*p;
    (*p) += 2;
    // CGX format is big-endian
    unsigned i = b[1] | ((b[0] & 0x7f) << 8);
    if (b[0] & 0x80)
        i |= (0xffffffffU << 15);
    return ((int)i);
}


int
cgx_in::long_value(char **p)
{
    unsigned char *b = (unsigned char *)*p;
    (*p) += 4;
    // CGX format is big-endian
    unsigned i = b[3] | (b[2] << 8) | (b[1] << 16) | ((b[0] & 0x7f) << 24);
    if (b[0] & 0x80)
        i |= (0xffffffffU << 31);
    return ((int)i);
}


//
//    IEEE double precision field:
//
//    Mantissa is base 2 (1/2 <= mantissa < 1).  Exponent is excess-1023.
//
//              111111 1111222222222233 3333333344444444 4455555555556666
//    0123456789012345 6789012345678901 2345678901234567 8901234567890123
//    ---------------- ---------------- ---------------- ----------------
//    FFFFEEEEEEEEEEES FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF
//    L  M             L              M L              M L              M
//
//
//    CALMA's double precision field:
//
//    Mantissa is base 16 (1/16 <= mantissa < 1).  Exponent is excess-64.
//
//              111111 1111222222222233 3333333344444444 4455555555556666
//    0123456789012345 6789012345678901 2345678901234567 8901234567890123
//    ---------------- ---------------- ---------------- ----------------
//    FFFFFFFFEEEEEEES FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF
//    L      M         L              M L              M L              M
//
//
//    where  E  =  exponent field
//           S  =  sign bit
//           F  =  fraction field
//           FL =  least sig. bit of word or byte
//           FM =  most sig. bit of word or byte

//
// This function is really inefficient, but there are typically few
// conversions required, and protability is an issue.
//
double
cgx_in::double_value(char **p)
{
    unsigned char *b = (unsigned char*)*p;
    (*p) += 8;
    int sign = 0;
    int exp = b[0];
    // test the sign bit
    if (exp & 0x80) {
        sign = 1;
        exp &= 0x7f;
    }
    exp -= 64;

    // Construct the mantissa
    double mantissa = 0.0;
    for (int i = 7; i > 0; i--) {
        mantissa += b[i];
        mantissa /= 256.0;
    }

    // Now raise the mantissa to the exponent
    double d = mantissa;
    if (exp > 0) {
        while (exp-- > 0)
            d *= 16.0;
    }
    else if (exp < 0) {
        while (exp++ < 0)
            d /= 16.0;
    }

    // Make it negative if necessary
    if (sign)
        d = -d;

    return (d);
}


void
cgx_in::date_value(tm *tm, char **p)
{
    memset(tm, 0, sizeof(struct tm));
    tm->tm_year = short_value(p);
    tm->tm_mon = *(*p)++ - 1;
    tm->tm_mday = *(*p)++;
    tm->tm_hour = *(*p)++;
    tm->tm_min = *(*p)++;
    tm->tm_sec = *(*p)++;
    (*p)++;
}


void
cgx_in::pr_record(const char *rec, int size, int flags)
{
    if (in_printing) {
#ifdef WIN32
        fprintf(in_print_fp, "%-12I64x ", (long long unsigned)in_offset);
#else
        fprintf(in_print_fp, "%-12llx ", (long long unsigned)in_offset);
#endif
        fprintf(in_print_fp, "%-9s size=%-9d flags=%d\n", rec, size, flags);
    }
}
// End of cgx_in functions

