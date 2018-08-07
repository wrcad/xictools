
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

#include "config.h"  // for computed goto
#include "fio.h"
#include "fio_oasis.h"
#include "fio_chd.h"
#include "fio_layermap.h"
#include "fio_cgd.h"
#include "cd_hypertext.h"
#include "cd_propnum.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "geo_zlist.h"
#include "miscutil/timedbg.h"


// Return true if fp points to an OASIS file.
//
bool
cFIO::IsOASIS(FILE *fp)
{
    FilePtr file = sFilePtr::newFilePtr(fp);
    int len = strlen(OAS_MAGIC);
    char buf[64];
    memset(buf, 0, 64);  // for valgrind
    file->z_read(buf, 1, len);
    delete file;
    if (!memcmp(OAS_MAGIC, buf, len))
        return (true);
    return (false);
}


// Read the OASIS file oas_fname, the new cells will be be added to
// the database.  All conversions will be scaled by the value of
// scale.
//  Return values:
//    OIerror       unspecified error
//    OIok          file was read and new cells opened ok
//    OIaborted     user aborted
//
OItype
cFIO::DbFromOASIS(const char *oas_fname, const FIOreadPrms *prms,
    stringlist **tlp, stringlist **tle)
{
    if (!prms)
        return (OIerror);
    Tdbg()->start_timing("oas_read");
    oas_in *oas = new oas_in(prms->allow_layer_mapping());
    oas->set_show_progress(true);
    oas->set_no_test_empties(IsNoCheckEmpties());

    CD()->SetReading(true);
    oas->assign_alias(NewReadingAlias(prms->alias_mask()));
    oas->read_alias(oas_fname);
    bool ret = oas->setup_source(oas_fname);
    if (ret) {
        oas->set_to_database();
        Tdbg()->start_timing("oas_read_phys");
        oas->begin_log(Physical);
        CD()->SetDeferInst(true);
        ret = oas->parse(Physical, false, prms->scale());
        CD()->SetDeferInst(false);
        Tdbg()->stop_timing("oas_read_phys");
        if (ret)
            ret = oas->mark_references(tlp);
        oas->end_log();
    }
    if (ret && !CD()->IsNoElectrical() && oas->has_electrical()) {
        Tdbg()->start_timing("oas_read_elec");
        oas->begin_log(Electrical);
        CD()->SetDeferInst(true);
        bool lpc = CD()->EnableLabelPatchCache(true);
        ret = oas->parse(Electrical, false, prms->scale());
        CD()->EnableLabelPatchCache(lpc);
        CD()->SetDeferInst(false);
        Tdbg()->stop_timing("oas_read_elec");
        if (ret)
            ret = oas->mark_references(tle);
        oas->end_log();
    }
    if (ret)
        oas->mark_top(tlp, tle);
    CD()->SetReading(false);

    if (ret)
        oas->dump_alias(oas_fname);

    OItype oiret = ret ? OIok : oas->was_interrupted() ? OIaborted : OIerror;
    delete oas;
    Tdbg()->stop_timing("oas_read");
    return (oiret);
}


// Read the OASIS file oas_fname, performing translation.
//  Return values:
//    OIerror       unspecified error
//    OIok          file was read and new cells opened ok
//    OIaborted     user aborted
//
OItype
cFIO::FromOASIS(const char *oas_fname, const FIOcvtPrms *prms,
    const char *chdcell)
{
    if (!prms) {
        Errs()->add_error("FromOASIS: null destination pointer.");
        return (OIerror);
    }
    if (!prms->destination()) {
        Errs()->add_error("FromOASIS: no destination given!");
        return (OIerror);
    }

    cCHD *chd = CDchd()->chdRecall(oas_fname, false);
    if (chd) {
        if (chd->filetype() != Foas) {
            Errs()->add_error("FromOASIS:: CHD file type not OASIS!");
            return (OIerror);
        }

        // We were given a CHD name, use it.
        oas_fname = chd->filename();
    }

    SetAllowPrptyStrip(true);
    if (chd || prms->use_window() || prms->flatten() ||
            dfix(prms->scale()) != 1.0 || (prms->ecf_level() != ECFnone)) {

        // Using windowing or other features requiring a cCHD.
        // description.  This is restricted to physical-mode data.

        bool free_chd = false;
        if (!chd) {
            unsigned int mask = prms->alias_mask();
            if (prms->filetype() == Fgds)
                mask |= CVAL_GDS;
            FIOaliasTab *tab = NewTranslatingAlias(mask);
            if (tab)
                tab->read_alias(oas_fname);
            cvINFO info = cvINFOtotals;
            if (prms->ecf_level() == ECFall || prms->ecf_level() == ECFpre)
                info = cvINFOplpc;
            chd = NewCHD(oas_fname, Foas, Physical, tab, info);
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

    oas_in *oas = new oas_in(prms->allow_layer_mapping());
    oas->set_show_progress(true);

    // Translating, directly streaming.  Skip electrical data if
    // StripForExport is set.

    const cv_alias_info *aif = prms->alias_info();
    if (aif)
        oas->assign_alias(new FIOaliasTab(true, false, aif));
    else {
        unsigned int mask = prms->alias_mask();
        if (prms->filetype() == Fgds)
            mask |= CVAL_GDS;
        oas->assign_alias(NewTranslatingAlias(mask));
    }
    oas->read_alias(oas_fname);

    bool ret = oas->setup_source(oas_fname);
    if (ret)
        ret = oas->setup_destination(prms->destination(), prms->filetype(),
            prms->to_cgd());

    if (ret) {
        oas->begin_log(Physical);
        ret = oas->parse(Physical, false, prms->scale());
        oas->end_log();
    }
    if (ret && !fioStripForExport && !prms->to_cgd() &&
            !CD()->IsNoElectrical() && oas->has_electrical()) {
        oas->begin_log(Electrical);
        ret = oas->parse(Electrical, false, prms->scale());
        oas->end_log();
    }

    if (ret)
        oas->dump_alias(oas_fname);

    OItype oiret = ret ? OIok : oas->was_interrupted() ? OIaborted : OIerror;
    delete oas;
    SetAllowPrptyStrip(false);
    return (oiret);
}


// Export to read a zoidlist file.
//
Zlist *
cFIO::ZlistFromFile(const char *fname, XIrt *xrt)
{
    if (xrt)
        *xrt = XIok;
    oas_in *in = new oas_in(false);
    bool ret = in->setup_source(fname);
    if (ret) {
        in->set_to_database();
        in->set_save_zoids(true);
        ret = in->parse(Physical, true, 1.0);
    }
    Zlist *z = 0;
    if (ret)
        z = in->get_zoids();

    if (!ret && xrt)
        *xrt = in->was_interrupted() ? XIintr : XIbad;
    delete in;
    return (z);
}
// End of cFIO functions


// Reinitialize the modal variables, call when CELL or <name> record is
// encountered.
//
void
oas_modal::reset(bool del)
{
    repetition.reset(-1);
    repetition_set = false;

    placement_x = 0;
    placement_x_set = true;
    placement_y = 0;
    placement_y_set = true;
    if (del)
        delete [] placement_cell;
    placement_cell = 0;
    placement_cell_set = false;

    layer = 0;
    layer_set = false;
    datatype = 0;
    datatype_set = false;

    textlayer = 0;
    textlayer_set = false;
    texttype = 0;
    texttype_set = false;
    text_x = 0;
    text_x_set = true;
    text_y = 0;
    text_y_set = true;
    if (del)
        delete [] text_string;
    text_string = 0;
    text_string_set = false;

    geometry_x = 0;
    geometry_x_set = true;
    geometry_y = 0;
    geometry_y_set = true;

    xy_mode = XYabsolute;
    xy_mode_set = true;

    geometry_w = 0;
    geometry_w_set = false;
    geometry_h = 0;
    geometry_h_set = false;

    if (del)
        delete [] polygon_point_list;
    polygon_point_list = 0;
    polygon_point_list_size = 0;
    polygon_point_list_set = false;

    path_half_width = 0;
    path_half_width_set = false;
    if (del)
        delete [] path_point_list;
    path_point_list = 0;
    path_point_list_size = 0;
    path_point_list_set = false;
    path_start_extension = 0;
    path_start_extension_set = false;
    path_end_extension = 0;
    path_end_extension_set = false;

    ctrapezoid_type = 0;
    ctrapezoid_type_set = false;

    circle_radius = 0;
    circle_radius_set = false;

    if (del)
        delete [] last_property_name;
    last_property_name = 0;
    last_property_name_set = false;
    if (del)
        delete [] last_value_list;
    last_value_list = 0;
    last_value_list_size = 0;
    last_value_list_set = false;
    last_value_standard = false;
}
// End of oas_modal functions


namespace {
    const char *oas_record_names[] =
    {
        "PAD",              // 0
        "START",            // 1
        "END",              // 2
        "CELLNAME_3",       // 3
        "CELLNAME_4",       // 4
        "TEXTSTRING_5",     // 5
        "TEXTSTRING_6",     // 6
        "PROPNAME_7",       // 7
        "PROPNAME_8",       // 8
        "PROPSTRING_9",     // 9
        "PROPSTRING_10",    // 10
        "LAYERNAME_11",     // 11
        "LAYERNAME_12",     // 12
        "CELL_13",          // 13
        "CELL_14",          // 14
        "XYABSOLUTE",       // 15
        "XYRELATIVE",       // 16
        "PLACEMENT_17",     // 17
        "PLACEMENT_18",     // 18
        "TEXT",             // 19
        "RECTANGLE",        // 20
        "POLYGON",          // 21
        "PATH",             // 22
        "TRAPEZOID_23",     // 23
        "TRAPEZOID_24",     // 24
        "TRAPEZOID_25",     // 25
        "CTRAPEZOID",       // 26
        "CIRCLR",           // 27
        "PROPERTY_28",      // 28
        "PROPERTY_29",      // 29
        "XNAME_30",         // 30
        "XNAME_31",         // 31
        "XELEMENT",         // 32
        "XGEOMETRY",        // 33
        "CBLOCK"            // 34
    };
}


void
oas_info::add_record(int rtype)
{
    rec_counts[rtype]++;
    cv_info::add_record(rtype);
}


char *
oas_info::pr_records(FILE *fp)
{
    sLstr lstr;
    char buf[256];
    for (int i = 0; i < OAS_NUM_REC_TYPES; i++) {
        if (rec_counts[i]) {
            sprintf(buf, "%-16s %d\n", oas_record_names[i], rec_counts[i]);
            if (fp)
                fputs(buf, fp);
            else
                lstr.add(buf);
        }
    }
    return (lstr.string_trim());
}
// End of oas_info functions


oas_in::oas_in(bool allow_layer_mapping) : cv_in(allow_layer_mapping)
{
    in_filetype = Foas;

    in_fp = 0;
    in_cellname = 0;
    in_curlayer = -1;
    in_curdtype = -1;
    in_layers = 0;
    in_undef_layers = 0;
    in_layer_name[0] = 0;
    in_unit = 0.0;
    in_zfile = 0;
    in_byte_offset = 0;
    in_compression_end = 0;
    in_next_offset = 0;
    in_start_offset = 0;
    in_scan_start_offset = 0;
    in_phys_layer_tab = 0;
    in_elec_layer_tab = 0;

    in_cgd = 0;
    in_byte_stream = 0;
    in_bakif = 0;

    in_state = oasNeedInit;

    in_xic_source = false;
    in_nogo = false;
    in_incremental = false;
    in_skip_all = false;
    in_skip_action = false;
    in_skip_elem = false;
    in_skip_cell = false;
    in_peek_record = 0;
    in_offset_flag = 0;
    in_peek_byte = 0;
    in_peeked = false;
    in_tables_read = false;
    in_print_break_lines = true;
    in_print_offset = false;
    in_looked_ahead = false;
    in_scanning = false;
    in_reading_table = false;

    in_print_cur_col = 0;
    in_print_start_col = 0;

    in_max_signed_integer_width = 0;
    in_max_unsigned_integer_width = 0;
    in_max_string_length = 0;
    in_polygon_max_vertices = 0;
    in_path_max_vertices = 0;
    in_top_cell = 0;
    in_bounding_boxes_available = 0;

    // dispatch table
    ftab[0] = 0;
    ftab[1] = &oas_in::read_start;
    ftab[2] = &oas_in::read_end;
    ftab[3] = &oas_in::read_cellname;
    ftab[4] = &oas_in::read_cellname;
    ftab[5] = &oas_in::read_textstring;
    ftab[6] = &oas_in::read_textstring;
    ftab[7] = &oas_in::read_propname;
    ftab[8] = &oas_in::read_propname;
    ftab[9] = &oas_in::read_propstring;
    ftab[10] = &oas_in::read_propstring;
    ftab[11] = &oas_in::read_layername;
    ftab[12] = &oas_in::read_layername;
    ftab[13] = &oas_in::read_cell;
    ftab[14] = &oas_in::read_cell;
    ftab[15] = &oas_in::xy_absolute;
    ftab[16] = &oas_in::xy_relative;
    ftab[17] = &oas_in::read_placement;
    ftab[18] = &oas_in::read_placement;
    ftab[19] = &oas_in::read_text;
    ftab[20] = &oas_in::read_rectangle;
    ftab[21] = &oas_in::read_polygon;
    ftab[22] = &oas_in::read_path;
    ftab[23] = &oas_in::read_trapezoid;
    ftab[24] = &oas_in::read_trapezoid;
    ftab[25] = &oas_in::read_trapezoid;
    ftab[26] = &oas_in::read_ctrapezoid;
    ftab[27] = &oas_in::read_circle;
    ftab[28] = &oas_in::read_property;
    ftab[29] = &oas_in::read_property;
    ftab[30] = &oas_in::read_xname;
    ftab[31] = &oas_in::read_xname;
    ftab[32] = &oas_in::read_xelement;
    ftab[33] = &oas_in::read_xgeometry;
    ftab[34] = &oas_in::read_cblock;

    // ctrapezoid stuff
    ctrtab[0] = &oas_in::ctrap0;
    ctrtab[1] = &oas_in::ctrap1;
    ctrtab[2] = &oas_in::ctrap2;
    ctrtab[3] = &oas_in::ctrap3;
    ctrtab[4] = &oas_in::ctrap4;
    ctrtab[5] = &oas_in::ctrap5;
    ctrtab[6] = &oas_in::ctrap6;
    ctrtab[7] = &oas_in::ctrap7;
    ctrtab[8] = &oas_in::ctrap8;
    ctrtab[9] = &oas_in::ctrap9;
    ctrtab[10] = &oas_in::ctrap10;
    ctrtab[11] = &oas_in::ctrap11;
    ctrtab[12] = &oas_in::ctrap12;
    ctrtab[13] = &oas_in::ctrap13;
    ctrtab[14] = &oas_in::ctrap14;
    ctrtab[15] = &oas_in::ctrap15;
    ctrtab[16] = &oas_in::ctrap16;
    ctrtab[17] = &oas_in::ctrap17;
    ctrtab[18] = &oas_in::ctrap18;
    ctrtab[19] = &oas_in::ctrap19;
    ctrtab[20] = &oas_in::ctrap20;
    ctrtab[21] = &oas_in::ctrap21;
    ctrtab[22] = &oas_in::ctrap22;
    ctrtab[23] = &oas_in::ctrap23;
    ctrtab[24] = &oas_in::ctrap24;
    ctrtab[25] = &oas_in::ctrap25;

    // actions
    cell_action = &oas_in::a_cell;
    placement_action = &oas_in::a_placement;
    text_action = &oas_in::a_text;
    rectangle_action = &oas_in::a_rectangle;
    polygon_action = &oas_in::a_polygon;
    path_action = &oas_in::a_path;
    trapezoid_action = &oas_in::a_trapezoid;
    ctrapezoid_action = &oas_in::a_ctrapezoid;
    circle_action = &oas_in::a_circle;
    property_action = &oas_in::a_property;
    standard_property_action = &oas_in::a_standard_property;
    end_action = &oas_in::a_endlib;
    clear_properties_action = &oas_in::a_clear_properties;

    in_cellname_table = 0;
    in_textstring_table = 0;
    in_propname_table = 0;
    in_propstring_table = 0;
    in_layername_table = 0;
    in_layermap_list = 0;
    in_xname_table = 0;

    in_cellname_index = 0;
    in_textstring_index = 0;
    in_propname_index = 0;
    in_propstring_index = 0;
    in_layername_index = 0;
    in_xname_index = 0;

    in_cellname_type = 0;
    in_textstring_type = 0;
    in_propname_type = 0;
    in_propstring_type = 0;
    in_layername_type = 0;
    in_xname_type = 0;

    in_cellname_off = 0;
    in_textstring_off = 0;
    in_propname_off = 0;
    in_propstring_off = 0;
    in_layername_off = 0;
    in_xname_off = 0;

    in_cellname_flag = false;
    in_textstring_flag = false;
    in_propname_flag = false;
    in_propstring_flag = false;
    in_layername_flag = false;
    in_xname_flag = false;

    in_save_zoids = false;
    in_zoidlist = 0;
}


oas_in::~oas_in()
{
    delete in_zfile;
    delete in_fp;
    delete [] in_cellname;
    delete in_undef_layers;
    if (in_phys_layer_tab) {
        SymTabGen gen(in_phys_layer_tab, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            delete (gds_lspec*)h->stData;
            delete h;
        }
        delete in_phys_layer_tab;
    }
    if (in_elec_layer_tab) {
        SymTabGen gen(in_elec_layer_tab, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            delete (gds_lspec*)h->stData;
            delete h;
        }
        delete in_elec_layer_tab;
    }
    if (in_cgd)
        in_cgd->dec_refcnt();

    delete in_cellname_table;
    delete in_textstring_table;
    delete in_propname_table;
    delete in_propstring_table;
    delete in_layername_table;
    oas_layer_map_elem::destroy(in_layermap_list);
    delete in_xname_table;

    Zlist::destroy(in_zoidlist);

    delete in_bakif;
}


//
// Setup functions
//

// Open the source file.
//
bool
oas_in::setup_source(const char *oas_fname, const cCHD*)
{
    in_fp = sFilePtr::newFilePtr(oas_fname, "rb");
    if (!in_fp) {
        Errs()->sys_error("open");
        Errs()->add_error("Can't open OASIS file %s.", oas_fname);
        return (false);
    }
    if (in_fp->file)
        in_gzipped = true;
    in_filename = lstring::copy(oas_fname);
    return (true);
}


// Set up the destination channel.
//
bool
oas_in::setup_destination(const char *destination, FileType ftype,
    bool to_cgd)
{
    if (destination) {
        const char *fn = in_filename;
        if (!fn && in_bakif)
            fn = ((oas_in*)in_bakif)->in_filename;
        in_out = FIO()->NewOutput(fn, destination, ftype, to_cgd);
        if (!in_out)
            return (false);
        in_own_in_out = true;
    }

    in_action = cvOpenModeTrans;

    cell_action = &oas_in::ac_cell;
    placement_action = &oas_in::ac_placement;
    text_action = &oas_in::ac_text;
    rectangle_action = &oas_in::ac_rectangle;
    polygon_action = &oas_in::ac_polygon;
    path_action = &oas_in::ac_path;
    trapezoid_action = &oas_in::ac_trapezoid;
    ctrapezoid_action = &oas_in::ac_ctrapezoid;
    circle_action = &oas_in::ac_circle;
    property_action = &oas_in::ac_property;
    standard_property_action = &oas_in::ac_standard_property;
    end_action = &oas_in::ac_endlib;
    clear_properties_action = &oas_in::ac_clear_properties;

    if (in_bakif)
        // Switch the non-cache channel to translation mode.
        in_bakif->setup_backend(0);

    return (true);
}


// Explicitly set the back-end processor, and reset a few things.
//
bool
oas_in::setup_backend(cv_out *out)
{
    if (in_fp)
        in_fp->z_rewind();
    in_zfile = 0;
    in_bytes_read = 0;
    in_byte_offset = 0;
    in_fb_incr = UFB_INCR;
    in_offset = 0;
    in_out = out;
    in_own_in_out = false;

    in_action = cvOpenModeTrans;

    cell_action = &oas_in::ac_cell;
    placement_action = &oas_in::ac_placement;
    text_action = &oas_in::ac_text;
    rectangle_action = &oas_in::ac_rectangle;
    polygon_action = &oas_in::ac_polygon;
    path_action = &oas_in::ac_path;
    trapezoid_action = &oas_in::ac_trapezoid;
    ctrapezoid_action = &oas_in::ac_ctrapezoid;
    circle_action = &oas_in::ac_circle;
    property_action = &oas_in::ac_property;
    standard_property_action = &oas_in::ac_standard_property;
    end_action = &oas_in::ac_endlib;
    clear_properties_action = &oas_in::ac_clear_properties;

    if (in_bakif)
        // Switch the non-cache channel to translation mode.
        in_bakif->setup_backend(0);

    return (true);
}


// Set up an input channel for an OASIS stream.  We also set up a
// "normal" channel as a backup.  This is used to obtain cell data if
// the cell is not in the OASIS stream source (used as a cache, for
// example).  It is not an error if the backup channel setup fails.
//
// The CHD arg will propagate the crc for gzip random access mapping,
// used in the backup channel.
//
bool
oas_in::setup_cgd_if(cCGD *cgd, bool lmap_ok, const char *fname,
    FileType ft, const cCHD *chd)
{
    if (!cgd)
        return (false);
    in_cgd = cgd;
    cgd->inc_refcnt();
    in_bakif = FIO()->NewInput(ft, lmap_ok);
    if (in_bakif && !in_bakif->setup_source(fname, chd)) {
        Errs()->get_error();
        delete in_bakif;
        in_bakif = 0;
    }
    return (true);
}


// Set this to create standard via masters when listing a hierarchy,
// such as when creating a CHD.  Creating these at this point might
// avoid name uncertainty later.
//
#define VIAS_IN_LISTONLY

// Main entry for reading.  If sc is not 1.0, geometry will be scaled.
// If listonly is true, the file offsets will be saved in the name
// table, but there is no conversion.
//
bool
oas_in::parse(DisplayMode mode, bool listonly, double sc, bool save_bb,
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
            in_phys_info = new oas_info(pl, pc);
            in_phys_info->initialize();
        }
    }
    else {
        in_scale = 1.0;
        in_needs_mult = false;
        if (infoflags != cvINFOnone) {
            bool pl = (infoflags == cvINFOpl || infoflags == cvINFOplpc);
            bool pc = (infoflags == cvINFOpc || infoflags == cvINFOplpc);
            in_elec_info = new oas_info(pl, pc);
            in_elec_info->initialize();
        }
    }
    in_listonly = listonly;
    in_sdesc = 0;
    in_cellname = 0;
    in_cell_offset = 0;
    in_prpty_list = 0;

    in_savebb = save_bb;
    in_ignore_text = in_mode == Physical && FIO()->IsNoReadLabels();

    delete in_cellname_table;
    in_cellname_table = 0;
    delete in_textstring_table;
    in_textstring_table = 0;
    delete in_propname_table;
    in_propname_table = 0;
    delete in_propstring_table;
    in_propstring_table = 0;
    delete in_layername_table;
    in_layername_table = 0;
    oas_layer_map_elem::destroy(in_layermap_list);
    in_layermap_list = 0;
    delete in_xname_table;
    in_xname_table = 0;

    in_cellname_index = 0;
    in_textstring_index = 0;
    in_propname_index = 0;
    in_propstring_index = 0;
    in_layername_index = 0;
    in_xname_index = 0;
    in_header_read = false;

    if (!in_byte_stream) {
        // Skip reading header when reading from block.

        if (in_fp)
            in_start_offset = in_fp->z_tell();

        if (in_action != cvOpenModePrint) {
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
        }
        else {
            // Check the magic string
            const char *magic = OAS_MAGIC;
            int len = strlen(magic);
            char buf[64];
            for (int i = 0; i < len; i++)
                buf[i] = read_byte();
            if (memcmp(magic, buf, len)) {
                Errs()->add_error("read_header: bad version string.");
                in_nogo = true;
                return (false);
            }
        }
    }

    if (in_listonly)
        FIO()->ifPrintCvLog(IFLOG_INFO, "Building symbol table (%s).",
            DisplayModeNameLC(in_mode));
    if (in_action == cvOpenModePrint) {
        if (in_mode == Electrical) {
            if (in_print_end > in_print_start && in_offset > in_print_end)
                return (true);
            if (in_print_reccnt == 0 || in_print_symcnt == 0)
                return (true);
            if (in_offset >= in_print_start)
                fprintf(in_print_fp, "\n\n>> Begin ELECTRICAL records\n");
        }
    }

    // Add count for START record.
    if (info())
        info()->add_record(1);

    for (;;) {
        unsigned int ix = read_record_index(34);
        if (in_nogo) {
            if (in_byte_stream && in_byte_stream->done() &&
                    !in_byte_stream->error())
                // Done reading stream.
                break;
            return (false);
        }
        if (ix == 0) {
            // pad
            if (in_action == cvOpenModePrint)
                dispatch(ix);
            continue;
        }
        if (in_action == cvOpenModePrint) {
            in_printing = (in_offset >= in_print_start);
            if (in_print_reccnt > 0)
                in_print_reccnt--;
            if (in_print_reccnt == 0 || in_print_symcnt == 0 ||
                    (in_print_end > in_print_start &&
                    in_offset > in_print_end)) {
                // Stop printing.
                in_printing_done = true;
                break;
            }
        }
        if (info())
            info()->add_record(ix);
        if (!dispatch(ix)) {
            Errs()->add_error("parse: failed in record type %d.", ix);
            return (false);
        }
        if (ix == 2) {
            // end
            if (in_mode == Physical)
                FIO()->ifPrintCvLog(IFLOG_INFO, "End physical records");
            else
                FIO()->ifPrintCvLog(IFLOG_INFO, "End electrical records");
            break;
        }
    }
    in_savebb = false;
    return (true);
}


// Incremental parser for reading from a byte stream.
//
bool
oas_in::parse_incremental(double sc)
{
    if (in_state == oasNeedInit) {
        in_mode = Physical;
        in_ext_phys_scale = dfix(sc);
        in_scale = in_ext_phys_scale;
        in_needs_mult = (in_scale != 1.0);
        in_listonly = false;
        in_sdesc = 0;
        in_cellname = 0;
        in_cell_offset = 0;
        in_prpty_list = 0;

        in_savebb = false;
        in_ignore_text = in_mode == Physical && FIO()->IsNoReadLabels();

        delete in_cellname_table;
        in_cellname_table = 0;
        delete in_textstring_table;
        in_textstring_table = 0;
        delete in_propname_table;
        in_propname_table = 0;
        delete in_propstring_table;
        in_propstring_table = 0;
        delete in_layername_table;
        in_layername_table = 0;
        oas_layer_map_elem::destroy(in_layermap_list);
        in_layermap_list = 0;
        delete in_xname_table;
        in_xname_table = 0;

        in_cellname_index = 0;
        in_textstring_index = 0;
        in_propname_index = 0;
        in_propstring_index = 0;
        in_layername_index = 0;
        in_xname_index = 0;
        in_header_read = false;

        in_state = oasNeedRecord;
        in_incremental = true;
    }

    if (!in_byte_stream) {
        Errs()->add_error("parse_incremental: no byte stream!");
        in_state = oasError;
        in_nogo = true;
        return (false);
    }

    for (;;) {
        unsigned int ix = read_record_index(34);
        if (in_nogo) {
            if (in_byte_stream && in_byte_stream->done() &&
                    !in_byte_stream->error()) {
                // Done reading stream.
                in_state = oasDone;
                in_nogo = false;
                break;
            }
            in_state = oasError;
            break;
        }
        if (ix == 0) {
            // pad
            if (in_action == cvOpenModePrint)
                dispatch(ix);
            continue;
        }
        if (in_action == cvOpenModePrint) {
            in_printing = (in_offset >= in_print_start);
            if (in_print_reccnt > 0)
                in_print_reccnt--;
            if (in_print_reccnt == 0 || in_print_symcnt == 0 ||
                    (in_print_end > in_print_start &&
                    in_offset > in_print_end)) {
                // Stop printing.
                in_printing_done = true;
                in_state = oasDone;
                break;
            }
        }
        if (!dispatch(ix)) {
            Errs()->add_error(
                "parse_incremental: failed in record type %d.", ix);
            in_state = oasError;
            break;
        }
        if (ix == 2) {
            in_state = oasDone;
            break;
        }
        if (in_state != oasNeedRecord)
            break;
    }
    return (!in_nogo);
}


//
// Object processing functions for incremental parser.
//

bool
oas_in::run_placement()
{
    if (in_state == oasHasPlacement) {
        int dx, dy;
        unsigned int nx, ny;
        if (in_rgen.get_array(&nx, &ny, &dx, &dy)) {
            if (!(this->*placement_action)(dx, dy, nx, ny)) {
                Errs()->add_error("read_placement: action failed.");
                in_nogo = true;
                in_state = oasError;
                return (false);
            }
            in_state = oasNeedRecord;
            return (true);
        }

        int xoff, yoff;
        if (!in_rgen.next(&xoff, &yoff)) {
            in_state = oasNeedRecord;
            return (true);
        }
        uobj.placement.x = modal.placement_x + xoff;
        uobj.placement.y = modal.placement_y + yoff;
        if (!(this->*placement_action)(0, 0, 1, 1)) {
            Errs()->add_error("read_placement: action failed.");
            in_nogo = true;
            in_state = oasError;
            return (false);
        }
        return (true);
    }
    return (false);
}


bool
oas_in::run_text()
{
    if (in_state == oasHasText) {
        int xoff, yoff;
        if (!in_rgen.next(&xoff, &yoff)) {
            in_state = oasNeedRecord;
            return (true);
        }
        uobj.text.x = modal.text_x + xoff;
        uobj.text.y = modal.text_y + yoff;
        if (!(this->*text_action)()) {
            Errs()->add_error("read_text: action failed.");
            in_nogo = true;
            in_state = oasError;
            return (false);
        }
        return (true);
    }
    return (false);
}


bool
oas_in::run_rectangle()
{
    if (in_state == oasHasRectangle) {
        int xoff, yoff;
        if (!in_rgen.next(&xoff, &yoff)) {
            in_state = oasNeedRecord;
            return (true);
        }
        uobj.rectangle.x = modal.geometry_x + xoff;
        uobj.rectangle.y = modal.geometry_y + yoff;
        if (!(this->*rectangle_action)()) {
            Errs()->add_error("read_rectangle: action failed.");
            in_nogo = true;
            in_state = oasError;
            return (false);
        }
        return (true);
    }
    return (false);
}


bool
oas_in::run_polygon()
{
    if (in_state == oasHasPolygon) {
        int xoff, yoff;
        if (!in_rgen.next(&xoff, &yoff)) {
            in_state = oasNeedRecord;
            return (true);
        }
        uobj.polygon.x = modal.geometry_x + xoff;
        uobj.polygon.y = modal.geometry_y + yoff;
        if (!(this->*polygon_action)()) {
            Errs()->add_error("read_polygon: action failed.");
            in_nogo = true;
            in_state = oasError;
            return (false);
        }
        return (true);
    }
    return (false);
}


bool
oas_in::run_path()
{
    if (in_state == oasHasPath) {
        int xoff, yoff;
        if (!in_rgen.next(&xoff, &yoff)) {
            in_state = oasNeedRecord;
            return (true);
        }
        uobj.path.x = modal.geometry_x + xoff;
        uobj.path.y = modal.geometry_y + yoff;
        if (!(this->*path_action)()) {
            Errs()->add_error("read_path: action failed.");
            in_nogo = true;
            in_state = oasError;
            return (false);
        }
        return (true);
    }
    return (false);
}


bool
oas_in::run_trapezoid()
{
    if (in_state == oasHasTrapezoid) {
        int xoff, yoff;
        if (!in_rgen.next(&xoff, &yoff)) {
            in_state = oasNeedRecord;
            return (true);
        }
        uobj.trapezoid.x = modal.geometry_x + xoff;
        uobj.trapezoid.y = modal.geometry_y + yoff;
        if (!(this->*trapezoid_action)()) {
            Errs()->add_error("read_trapezoid: action failed.");
            in_nogo = true;
            in_state = oasError;
            return (false);
        }
        return (true);
    }
    return (false);
}


bool
oas_in::run_ctrapezoid()
{
    if (in_state == oasHasCtrapezoid) {
        int xoff, yoff;
        if (!in_rgen.next(&xoff, &yoff)) {
            in_state = oasNeedRecord;
            return (true);
        }
        uobj.ctrapezoid.x = modal.geometry_x + xoff;
        uobj.ctrapezoid.y = modal.geometry_y + yoff;
        if (!(this->*ctrapezoid_action)()) {
            Errs()->add_error("read_ctrapezoid: action failed.");
            in_nogo = true;
            in_state = oasError;
            return (false);
        }
        return (true);
    }
    return (false);
}


bool
oas_in::run_circle()
{
    if (in_state == oasHasCircle) {
        int xoff, yoff;
        if (!in_rgen.next(&xoff, &yoff)) {
            in_state = oasNeedRecord;
            return (true);
        }
        uobj.circle.x = modal.geometry_x + xoff;
        uobj.circle.y = modal.geometry_y + yoff;
        if (!(this->*circle_action)()) {
            Errs()->add_error("read_circle: action failed.");
            in_nogo = true;
            in_state = oasError;
            return (false);
        }
        return (true);
    }
    return (false);
}


bool
oas_in::run_xgeometry()
{
    if (in_state == oasHasXgeometry) {
        int xoff, yoff;
        if (!in_rgen.next(&xoff, &yoff)) {
            in_state = oasNeedRecord;
            return (true);
        }
        uobj.xgeometry.x = modal.geometry_x + xoff;
        uobj.xgeometry.y = modal.geometry_y + yoff;
        /*
        if (!(this->*xgeometry_action)()) {
            Errs()->add_error("read_rectangle: action failed.");
            in_nogo = true;
            in_state = oasError;
            return (false);
        }
        */
        return (true);
    }
    return (false);
}


//
// Entries for reading through CHD.
//

// Setup scaling and read the file header.
//
bool
oas_in::chd_read_header(double phys_scale)
{
    bool ret = true;
    in_compression_end = 0;
    in_next_offset = 0;
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
oas_in::chd_read_cell(symref_t *p, bool use_inst_list, CDs **sdret)
{
    if (in_cgd && in_bakif && (in_mode == Electrical ||
            in_cgd->unlisted_test(Tstring(p->get_name())))) {

        // In electrical mode, always obtain data from the file. 
        // Likewse, in physical mode if the cell has been removed from
        // the cgd "cache", we need to get data from the file.  Only
        // physical cells that have geometry will be "unlisted",
        // physical cells that have subcells only (or are empty) will
        // be handled in the main part of this function.

        in_bakif->init_to(this);
        chd_set_header_info(in_chd_state.chd()->headerInfo());
        bool ret = in_bakif->chd_read_cell(p, use_inst_list, sdret);
        in_chd_state.chd()->setHeaderInfo(chd_get_header_info());
        in_bakif->init_bak();
        return (ret);
    }

    if (sdret)
        *sdret = 0;
    in_sdesc = 0;
    in_cellname = 0;
    in_cell_offset = 0;
    in_prpty_list = 0;
    in_uselist = true;
    in_curlayer = -1;
    in_curdtype = -1;
    in_nogo = false;

    if (FIO()->IsUseCellTab()) {
        if (in_mode == Physical && 
                ((in_action == cvOpenModeTrans) ||
                ((in_action == cvOpenModeDb) && in_flatten))) {

            // The AuxCellTab may contain cells to stream out from the
            // main database, overriding the cell data in the CHD.

            OItype oiret = chd_process_override_cell(p);
            if (oiret == OIerror)
                return (false);
            if (oiret == OInew)
                return (true);
        }
    }
    if (!p->get_defseen()) {
        CDs *sd = CDcdb()->findCell(p->get_name(), in_mode);
        if (sd && in_mode == Physical && 
                ((in_action == cvOpenModeTrans) ||
                ((in_action == cvOpenModeDb) && in_flatten))) {

            // The cell definition was not in the file.  This could mean
            // that the cell is a standard via or parameterized cell
            // sub-master, or is a native library cell.  In any case, it
            // should be in memory.

            if (sd && (sd->isViaSubMaster() || sd->isPCellSubMaster()))
                return (chd_output_cell(sd) == OIok);
        }
        if (sd && (in_action == cvOpenModeDb))
            return (true);
        FIO()->ifPrintCvLog(IFLOG_WARN, "Unresolved cell %s (%s).",
            DisplayModeNameLC(in_mode), Tstring(p->get_name()),
            sd ? "in memory" : "not found");
        return (true);
    }

    if (in_mode == Physical && in_cgd) {
        // This takes care of accessing the data through an attached
        // geometry database.

        if (in_action == cvOpenModeDb) {
            if (!a_cell(Tstring(p->get_name())))
                return (false);
        }
        else if (in_action == cvOpenModeTrans) {
            // Need this for error messages.
            in_cellname = lstring::copy(Tstring(p->get_name()));
            if (in_transform <= 0) {
                in_out->write_struct(Tstring(p->get_name()),
                    &in_cdate, &in_mdate);
            }
        }
        else {
            Errs()->add_error("chd_read_cell: internal, bad action.");
            return (false);
        }
        if (!in_flatten) {

            // If not flattening, have to write the instances.
            nametab_t *ntab = get_sym_tab(in_mode);
            crgen_t cgen(ntab, p);

            // Make use of the instance skip flags, if available.
            cv_bbif *ctab = in_chd_state.ctab();
            const cv_fgif *item =
                ctab ? ctab->resolve_fgif(&p, in_chd_state.ct_index()) : 0;

            const cref_o_t *c;
            while ((c = cgen.next()) != 0) {
                symref_t *cp = ntab->find_symref(c->srfptr);
                if (!cp) {
                    Errs()->add_error(
                        "Unresolved symref back-pointer from %s.",
                        in_chd_state.symref()->get_name());
                    return (false);
                }
                if (cp->should_skip())
                    continue;
                if (item && item->get_flag(cgen.call_count()) == ts_unset)
                     continue;

                CDcellName cellname = cp->get_name();
                CDattr at;
                if (!CD()->FindAttr(c->attr, &at)) {
                    Errs()->add_error(
                        "Unresolved transform ticket %d.", c->attr);
                    return (false);
                }
                int x, y, tdx, tdy;
                if (in_mode == Physical) {
                    double tscale = in_ext_phys_scale;
                    x = mmRnd(c->tx*tscale);
                    y = mmRnd(c->ty*tscale);
                    tdx = mmRnd(at.dx*tscale);
                    tdy = mmRnd(at.dy*tscale);
                }
                else {
                    x = c->tx;
                    y = c->ty;
                    tdx = at.dx;
                    tdy = at.dy;
                }

                if (in_action == cvOpenModeDb) {
                    CDtx tx(at.refly, at.ax, at.ay, x, y, at.magn);
                    CDap ap(at.nx, at.ny, tdx, tdy);

                    CDcellName cname = check_sub_master(cp->get_name());
                    CallDesc calldesc(cname, 0);
                    CDc *newo;
                    if (OIfailed(in_sdesc->makeCall(&calldesc, &tx, &ap,
                            CDcallDb, &newo)))
                        return (false);
                    if (newo)
                        a_add_properties(in_sdesc, newo);
                }
                else {
                    Instance inst;
                    inst.magn = at.magn;
                    inst.name = Tstring(cellname);
                    inst.nx = at.nx;
                    inst.ny = at.ny;
                    inst.dx = tdx;
                    inst.dy = tdy;
                    inst.origin.set(x, y);
                    inst.reflection = at.refly;
                    inst.set_angle(at.ax, at.ay);

                    if (!ac_placement_backend(&inst, cp, false))
                        return (false);
                }
            }
        }

        bool ret = true;
        stringlist *layers = in_cgd->layer_list(Tstring(p->get_name()));
        if (layers) {
            GCdestroy<stringlist> gc_layers(layers);
            for (stringlist *s = layers; s; s = s->next) {
                if (FIO()->IsCgdSkipInvisibleLayers()) {
                    CDl *ld = CDldb()->findLayer(s->string, Physical);
                    if (ld && ld->isInvisible())
                        continue;
                }

                oas_byte_stream *bs;
                if (!in_cgd->get_byte_stream(Tstring(p->get_name()),
                        s->string, &bs))
                    return (false);
                if (!bs)
                    continue;

                set_byte_stream(bs);
                modal.reset();
                for (;;) {
                    int ix = read_record_index(34);
                    if (in_nogo) {
                        if (in_byte_stream->done() &&
                                !in_byte_stream->error()) {
                            // Done reading stream.
                            in_nogo = false;
                            break;
                        }
                        ret = false;
                        break;
                    }
                    if (ix == 0)
                        continue;
                    if (ix < 15 || ix == 30 || ix == 31)
                        // END, <name>
                        break;
                    if (!dispatch(ix)) {
                        ret = false;
                        break;
                    }
                }
                delete bs;
                if (!ret)
                    return (false);
            }
        }
        if (ret) {
            if (in_action == cvOpenModeDb) {
                if (sdret)
                    *sdret = in_sdesc;
                ret = end_cell();
            }
            else if (in_action == cvOpenModeTrans) {
                if (!in_flatten && in_transform <= 0)
                    ret = in_out->write_end_struct();
                delete [] in_cellname;
                in_cellname = 0;
            }
        }
        return (ret);
    }

    in_byte_offset = p->get_offset();
    in_offset = 0;
    in_peeked = false;
    in_zfile = 0;
    if (!in_fp) {
        Errs()->add_error("read_cell: null file pointer.");
        return (false);
    }
    if (in_fp->z_seek(p->get_offset(), SEEK_SET) < 0) {
        Errs()->add_error("read_cell: seek failed.");
        return (false);
    }

    CD()->InitCoincCheck();
    unsigned int ix = read_record_index(34);
    if (in_nogo)
        return (false);
    if (ix != 13 && ix != 14) {
        Errs()->add_error("read_cell: no CELL record at offset.");
        return (false);
    }

    cv_chd_state stbak;
    in_chd_state.push_state(p, use_inst_list ? get_sym_tab(in_mode) : 0,
        &stbak);
    bool ret = read_cell(ix);
    if (ret) {
        // Skip labels when flattening subcells when flag set.
        bool bktxt = in_ignore_text;
        if (in_mode == Physical && in_flatten && in_transform > 0 &&
                FIO()->IsNoFlattenLabels())
            in_ignore_text = true;

        const char *msg = "read_cell: failed in record type %d.";
        if (in_sdesc || in_action == cvOpenModeTrans) {
            for (;;) {
                ix = read_record_index(34);
                if (in_nogo) {
                    ret = false;
                    break;
                }
                if (ix == 0)
                    continue;
                if (ix < 15 || ix == 30 || ix == 31)
                    // END, <name>
                    break;
                if (!dispatch(ix)) {
                    Errs()->add_error(msg, ix);
                    ret = false;
                    break;
                }
            }
        }
        in_ignore_text = bktxt;
    }
    if (sdret)
        *sdret = in_sdesc;
    if (ret)
        ret = end_cell();
    in_chd_state.pop_state(&stbak);

    if (!ret && in_out && in_out->was_interrupted())
        in_interrupted = true;

    return (ret);
}


cv_header_info *
oas_in::chd_get_header_info()
{
    if (!in_header_read)
        return (0);
    oas_header_info *oas = new oas_header_info(in_unit);
    oas->cellname_tab = in_cellname_table;
    oas->textstring_tab = in_textstring_table;
    oas->propname_tab = in_propname_table;
    oas->propstring_tab = in_propstring_table;
    oas->layername_tab = in_layername_table;
    oas->xname_tab = in_xname_table;
    in_cellname_table = 0;
    in_textstring_table = 0;
    in_propname_table = 0;
    in_propstring_table = 0;
    in_layername_table = 0;
    in_xname_table = 0;
    in_header_read = false;
    return (oas);
}


void
oas_in::chd_set_header_info(cv_header_info *hinfo)
{
    if (!hinfo || in_header_read)
        return;
    oas_header_info *oash = static_cast<oas_header_info*>(hinfo);
    in_unit = oash->unit();
    in_cellname_table = oash->cellname_tab;
    in_textstring_table = oash->textstring_tab;
    in_propname_table = oash->propname_tab;
    in_propstring_table = oash->propstring_tab;
    in_layername_table = oash->layername_tab;
    in_xname_table = oash->xname_tab;
    oash->cellname_tab = 0;
    oash->textstring_tab = 0;
    oash->propname_tab = 0;
    oash->propstring_tab = 0;
    oash->layername_tab = 0;
    oash->xname_tab = 0;
    in_header_read = true;
}


//
// Misc. entries.
//

// Return true if electrical records present.
//
bool
oas_in::has_electrical()
{
    if (!in_fp)
        return (false);

    if (in_action == cvOpenModePrint) {
        if (in_print_end > in_print_start && in_offset > in_print_end)
            return (false);
        if (in_print_reccnt == 0 || in_print_symcnt == 0)
            return (false);
    }

    int64_t pos = in_fp->z_tell();
    const char *magic = OAS_MAGIC;
    int len = strlen(magic);
    for (int i = 0; i < len; i++) {
        int c = in_fp->z_getc();
        if (c != magic[i]) {
            in_fp->z_seek(pos, SEEK_SET);
            in_fp->z_clearerr();
            return (false);
        }
    }
    in_fp->z_seek(pos, SEEK_SET);
    return (true);
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
oas_in::has_geom(symref_t *p, const BBox *AOI)
{
    if (in_cgd && in_bakif && (in_mode == Electrical ||
            in_cgd->unlisted_test(Tstring(p->get_name())))) {

        // In electrical mode, always obtain data from the file. 
        // Likewse, in physical mode if the cell has been removed from
        // the cgd "cache", we need to get data from the file.  Only
        // physical cells that have geometry will be "unlisted",
        // physical cells that have subcells only (or are empty) will
        // be handled in the main part of this function.

        in_bakif->init_to(this);
        chd_set_header_info(in_chd_state.chd()->headerInfo());
        OItype oiret = in_bakif->has_geom(p, AOI);
        in_chd_state.chd()->setHeaderInfo(chd_get_header_info());
        in_bakif->init_bak();
        return (oiret);
    }

    in_sdesc = 0;
    in_cellname = 0;
    in_cell_offset = 0;
    in_prpty_list = 0;
    in_uselist = true;
    in_curlayer = -1;
    in_curdtype = -1;

    // The AuxCellTab may contain cells to stream out from the main
    // database, overriding the cell data in the CHD.  Take these as
    // having geometry.
    //
    if (in_mode == Physical && in_action == cvOpenModeTrans &&
            FIO()->IsUseCellTab()) {
        CDcellTab *ct = CDcdb()->auxCellTab();
        if (ct) {
            if (ct->find(p->get_name()))
                return (OIok);
        }
    }

    if (in_mode == Physical && in_cgd) {

        // This takes care of accessing the data through an attached
        // geometry database.

        stringlist *layers = in_cgd->layer_list(Tstring(p->get_name()));
        if (!layers)
            return (OIambiguous);

        GCdestroy<stringlist> gc_layers(layers);
        for (stringlist *s = layers; s; s = s->next) {
            if (FIO()->IsCgdSkipInvisibleLayers()) {
                CDl *ld = CDldb()->findLayer(s->string, Physical);
                if (ld && ld->isInvisible())
                    continue;
            }
            if (in_lcheck) {
                int layer, datatype;
                if (strmdata::hextrn(s->string, &layer, &datatype) &&
                        !in_lcheck->layer_ok(0, layer, datatype))
                    continue;
            }

            oas_byte_stream *bs;
            if (!in_cgd->get_byte_stream(Tstring(p->get_name()),
                    s->string, &bs)) {
                return (OIerror);
            }
            if (!bs)
                continue;

            set_byte_stream(bs);
            modal.reset();
            OItype oiret = has_geom_core(AOI);
            delete bs;
            if (oiret != OIambiguous)
                return (oiret);
        }
        return (OIambiguous);
    }

    if (!p->get_defseen()) {
        // No cell definition in file, just ignore this.
        return (OIambiguous);
    }

    in_byte_offset = p->get_offset();
    in_offset = 0;
    in_peeked = false;
    in_zfile = 0;
    if (!in_fp) {
        Errs()->add_error("has_geom: null file pointer.");
        return (OIerror);
    }
    if (in_fp->z_seek(p->get_offset(), SEEK_SET) < 0) {
        Errs()->add_error("has_geom: seek failed.");
        return (OIerror);
    }

    unsigned int ix = read_record_index(34);
    if (in_nogo)
        return (OIerror);
    if (ix != 13 && ix != 14) {
        Errs()->add_error("has_geom: no CELL record at offset.");
        return (OIerror);
    }

    in_skip_action = true;
    if (!read_cell(ix)) {
        in_skip_action = false;
        return (OIerror);
    }
    return (has_geom_core(AOI));
}
// End of virtual overrides.


//
//---- Private Functions -----------
//

OItype
oas_in::has_geom_core(const BBox *AOI)
{
    const char *msg = "has_geom_core: failed in record type %d.";
    in_skip_action = true;
    for (;;) {
        unsigned int ix = read_record_index(34);
        if (in_nogo) {
            in_skip_action = false;
            return (OIerror);
        }
        if (ix == 0)
            continue;
        if (ix < 15 || ix == 30 || ix == 31)
            // END, <name>
            break;
        if (!dispatch(ix)) {
            Errs()->add_error(msg, ix);
            in_skip_action = false;
            return (OIerror);
        }
        if (ix == 20) {  // rect
            if (!a_layer(uobj.rectangle.layer, uobj.rectangle.datatype, 0,
                    false)) {
                in_skip_action = false;
                return (OIerror);
            }
            if (AOI && !in_skip_elem) {
                in_skip_elem = true;
                int w = scale(uobj.rectangle.width);
                int h = scale(uobj.rectangle.height);

                in_rgen.init(uobj.rectangle.repetition);
                int xoff, yoff;
                while (in_rgen.next(&xoff, &yoff)) {
                    int l = scale(uobj.rectangle.x + xoff);
                    int b = scale(uobj.rectangle.y + yoff);
                    BBox BB(l, b, l + w, b + h);
                    BB.fix();
                    if (BB.intersect(AOI, false)) {
                        in_skip_elem = false;
                        break;
                    }
                }
            }
        }
        else if (ix == 21) {  // poly
            if (!a_layer(uobj.polygon.layer, uobj.polygon.datatype, 0,
                    false)) {
                in_skip_action = false;
                return (OIerror);
            }
            if (AOI && !in_skip_elem) {
                in_skip_elem = true;
                Poly po;
                po.numpts = uobj.polygon.point_list_size + 2;
                po.points = new Point[po.numpts];

                in_rgen.init(uobj.polygon.repetition);
                int xoff, yoff;
                while (in_rgen.next(&xoff, &yoff)) {
                    po.points[0].set(
                        scale(uobj.polygon.x + xoff),
                        scale(uobj.polygon.y + yoff));
                    po.points[po.numpts-1] = po.points[0];
                    for (unsigned int i = 0; i < uobj.polygon.point_list_size;
                            i++) {
                        po.points[i+1].set(
                            po.points[i].x +
                                scale(uobj.polygon.point_list[i].x),
                            po.points[i].y +
                                scale(uobj.polygon.point_list[i].y));
                    }
                    if (po.intersect(AOI, false)) {
                        in_skip_elem = false;
                        break;
                    }
                }
                delete [] po.points;
            }
        }
        else if (ix == 22) {  // path
            if (!a_layer(uobj.path.layer, uobj.path.datatype, 0, false)) {
                in_skip_action = false;
                return (OIerror);
            }
            if (AOI && !in_skip_elem) {
                in_skip_elem = true;
                int numpts = uobj.path.point_list_size + 1;
                Wire w(scale(2*uobj.path.half_width), CDWIRE_FLUSH, numpts,
                    new Point[numpts]);
                if (uobj.path.start_extension == (int)uobj.path.half_width &&
                        uobj.path.end_extension == (int)uobj.path.half_width)
                    w.set_wire_style(uobj.path.rounded_end ?
                        CDWIRE_ROUND:CDWIRE_EXTEND);
                else if (uobj.path.start_extension != 0 ||
                        uobj.path.end_extension != 0)
                    w.set_wire_style(CDWIRE_EXTEND);

                in_rgen.init(uobj.path.repetition);
                int xoff, yoff;
                while (in_rgen.next(&xoff, &yoff)) {
                    w.points[0].set(
                        scale(uobj.path.x + xoff), scale(uobj.path.y + yoff));
                    for (unsigned int i = 0; i < uobj.path.point_list_size;
                            i++) {
                        w.points[i+1].set(
                            w.points[0].x + scale(uobj.path.point_list[i].x),
                            w.points[0].y + scale(uobj.path.point_list[i].y));
                    }
                    if (w.intersect(AOI, false)) {
                        in_skip_elem = false;
                        break;
                    }
                }
                delete [] w.points;
            }
        }
        else if (ix >= 23 && ix <= 25) {  // trap
            if (!a_layer(uobj.trapezoid.layer, uobj.trapezoid.datatype, 0,
                    false)) {
                in_skip_action = false;
                return (OIerror);
            }
            if (AOI && !in_skip_elem) {
                in_skip_elem = true;
                Poly po;
                if (trap2poly(&po)) {
                    in_rgen.init(uobj.trapezoid.repetition);
                    int xoff0 = 0, yoff0 = 0;
                    int xoff, yoff;
                    while (in_rgen.next(&xoff, &yoff)) {
                        for (int i = 0; i < po.numpts; i++) {
                            po.points[i].x += xoff - xoff0;
                            po.points[i].y += yoff - yoff0;
                        }
                        if (po.intersect(AOI, false)) {
                            in_skip_elem = false;
                            break;
                        }
                        xoff0 = xoff;
                        yoff0 = yoff;
                    }
                    delete [] po.points;
                }
            }
        }
        else if (ix == 26) {  // ctrap
            if (!a_layer(uobj.ctrapezoid.layer, uobj.ctrapezoid.datatype, 0,
                    false)) {
                in_skip_action = false;
                return (OIerror);
            }
            if (AOI && !in_skip_elem) {
                in_skip_elem = true;
                unsigned int w = scale(uobj.ctrapezoid.width);
                unsigned int h = scale(uobj.ctrapezoid.height);

                in_rgen.init(uobj.ctrapezoid.repetition);
                int xoff, yoff;
                while (in_rgen.next(&xoff, &yoff)) {
                    int x = scale(uobj.ctrapezoid.x + xoff);
                    int y = scale(uobj.ctrapezoid.y + yoff);
                    Poly po;
                    if (!(this->*ctrtab[uobj.ctrapezoid.type])(&po, x, y,
                            w, h))
                        continue;
                    if (po.intersect(AOI, false)) {
                        delete [] po.points;
                        in_skip_elem = false;
                        break;
                    }
                    delete [] po.points;
                }
            }
        }
        else if (ix == 27) {  // circle
            if (!a_layer(uobj.circle.layer, uobj.circle.datatype, 0, false)) {
                in_skip_action = false;
                return (OIerror);
            }
            if (AOI && !in_skip_elem) {
                in_skip_elem = true;
                unsigned int r = scale(uobj.circle.radius);

                in_rgen.init(uobj.circle.repetition);
                int xoff, yoff;
                while (in_rgen.next(&xoff, &yoff)) {
                    Poly po;
                    int x = scale(uobj.circle.x + xoff);
                    int y = scale(uobj.circle.y + yoff);
                    int tsp = GEO()->spotSize();
                    GEO()->setSpotSize(0);
                    po.points = GEO()->makeArcPath(
                        &po.numpts, (in_mode == Electrical), x, y, r, r,
                        0, 0, 0.0, 0.0);
                    GEO()->setSpotSize(tsp);
                    if (po.intersect(AOI, false)) {
                        in_skip_elem = false;
                        delete [] po.points;
                        break;
                    }
                    delete [] po.points;
                }
            }
        }
        else
            continue;
        if (!in_skip_elem) {
            in_skip_action = false;
            return (OIok);
        }
    }
    in_skip_action = false;
    return (OIambiguous);
}


// Read the file header and set the scale, used before random read.
//
bool
oas_in::read_header(bool)
{
    if (in_mode == Physical && in_cgd) {
        in_header_read = true;
        return (true);
    }
    if (!in_header_read) {
        const char *magic = OAS_MAGIC;
        int len = strlen(magic);
        char buf[64];
        for (int i = 0; i < len; i++)
            buf[i] = read_byte();
        if (memcmp(magic, buf, len)) {
            Errs()->add_error("read_header: bad version string.");
            in_nogo = true;
            return (false);
        }
        unsigned int rec = read_record_index(1);
        if (rec != 1) {
            Errs()->add_error("read_header: initial record not START.");
            in_nogo = true;
            return (false);
        }
        read_start(rec);
        if (in_mode == Physical)
            in_header_read = true;
    }

    if (in_mode == Physical) {
        double fct = CDphysResolution/in_unit;
        in_scale = dfix(in_ext_phys_scale*fct);
        if (fct < .999) {
            FIO()->ifPrintCvLog(IFLOG_WARN,
        "File resolution of %d is larger than the database\n"
        "**  resolution %d, loss by rounding can occur.  Suggest using the\n"
        "**  DatabaseResolution variable to set matching resolution when\n"
        "**  working with this file.\n**",
                mmRnd(1e-6/in_unit), CDphysResolution);
        }
    }
    else
        in_scale = dfix(CDelecResolution/in_unit);
    in_needs_mult = (in_scale != 1.0);
    if (in_mode == Physical)
        in_phys_scale = in_scale;

    return (true);
}


// Finalize cell reading, database and translation modes.
//
bool
oas_in::end_cell()
{
    if (in_listonly) {
        if (in_savebb && in_symref)
            in_symref->set_bb(&in_cBB);
        if (in_cref_end) {
            nametab_t *ntab = get_sym_tab(in_mode);
            cref_t *c = ntab->find_cref(in_cref_end);
            c->set_last_cref(true);
            in_cref_end = 0;

            if (!ntab->cref_cmp_test(in_symref, in_cref_end, CRCMP_end))
                return (false);
        }
        in_symref = 0;
    }
    else if (in_action == cvOpenModeTrans) {
        if (in_transform <= 0) {
            if (!in_flatten) {
                if (!in_out->write_end_struct())
                    return (false);
            }
        }
    }
    else if (in_sdesc) {
        if (in_mode == Physical) {
            if (in_cell_offset && !in_savebb && !in_no_test_empties &&
                    in_sdesc->isEmpty()) {
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
    if (in_sdesc && FIO()->IsReadingLibrary()) {
        in_sdesc->setImmutable(true);
        in_sdesc->setLibrary(true);
    }
    return (true);
}


//
//===========================================================================
// Actions for creating cells in database
//===========================================================================
//

namespace {
    // Convert a pathtype 4 endpoint to pathtype 0.
    // int *xe, *ye;     coordinate of endpoint
    // int xb, yb;       coordinate of previous or next point in path
    // int extn;         specified extension
    //
    void
    convert_4to0(int *xe, int *ye, int xb, int yb, int extn)
    {
        if (!extn)
            return;
        if (*xe == xb) {
            if (*ye > yb)
                *ye += extn;
            else
                *ye -= extn;
        }
        else if (*ye == yb) {
            if (*xe > xb)
                *xe += extn;
            else
                *xe -= extn;
        }
        else {
            double dx = (double)(*xe - xb);
            double dy = (double)(*ye - yb);
            double d = sqrt(dx*dx + dy*dy);
            d = extn/d;
            *ye += mmRnd(dy*d);
            *xe += mmRnd(dx*d);
        }
    }
}


bool
oas_in::a_cell(const char *name)
{
    if (in_skip_cell) {
        delete [] in_cellname;
        in_cellname = lstring::copy(name);
        return (true);
    }

    // end previous struct
    if (!end_cell())
        return (false);

    CD()->InitCoincCheck();
    in_cell_offset = in_offset;
    in_curlayer = -1;
    in_curdtype = -1;

    delete [] in_cellname;
    in_cellname = lstring::copy(alias(name));

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
                srf = get_symref(name, in_mode);
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
            sd->setFileType(Foas);
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
                if (sd->isViaSubMaster() || sd->isPCellSubMaster()) {
                    mi.overwrite_phys = false;
                    mi.overwrite_elec = false;
                    mi.skip_elec = true;
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
                sd->setFileType(Foas);
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
                if (in_over_tab && (xx = SymTab::get(in_over_tab,
                        (unsigned long)sd->cellname())) != ST_NIL) {
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
                    else if (!mi.skip_elec) {
                        if (!FIO()->IsNoOverwritePhys())
                            mi.overwrite_phys = true;
                        if (!FIO()->IsNoOverwriteElec())
                            mi.overwrite_elec = true;
                        FIO()->ifMergeControl(&mi);
                    }
                    if (mi.overwrite_phys) {
                        if (!get_symref(Tstring(sd->cellname()),
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
                sd->setFileType(Foas);
                sd->setFileName(in_filename);
                if (in_lcheck || in_layer_alias || in_phys_scale != 1.0)
                    sd->setAltered(true);
                in_sdesc = sd;
            }
        }
        a_add_properties(in_sdesc, 0);
    }
    return (true);
}


bool
oas_in::a_placement(int dx, int dy, unsigned int nx, unsigned int ny)
{
    if (in_chd_state.gen()) {
        if (!in_sdesc || !in_chd_state.symref())
            return (true);
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
        if (cp->should_skip())
            return (true);
        tristate_t fst = in_chd_state.get_inst_use_flag();
        if (fst == ts_unset)
            return (true);

        CDcellName cname = cp->get_name();
        cname = check_sub_master(cname);
        CDattr at;
        if (!CD()->FindAttr(c->attr, &at)) {
            Errs()->add_error(
                "Unresolved transform ticket %d.", c->attr);
            return (false);
        }
        int x, y, tdx, tdy;
        if (in_mode == Physical) {
            double tscale = in_ext_phys_scale;
            x = mmRnd(c->tx*tscale);
            y = mmRnd(c->ty*tscale);
            tdx = mmRnd(at.dx*tscale);
            tdy = mmRnd(at.dy*tscale);
        }
        else {
            x = c->tx;
            y = c->ty;
            tdx = at.dx;
            tdy = at.dy;
        }

        // If we're reading into the database and scaling, we
        // don't want to scale library, standard cell, or pcell
        // sub-masters.  Instead, these are always read in
        // unscaled, and instance placements are scaled.  We
        // check/set this here.

        double tmag = at.magn;
        if (in_mode == Physical && in_needs_mult &&
                in_action == cvOpenModeDb) {
            CDs *sd = CDcdb()->findCell(cname, in_mode);
            if (sd && (sd->isViaSubMaster() || sd->isPCellSubMaster() ||
                    sd->isLibrary()))
                tmag *= in_phys_scale;
            else if (in_chd_state.chd()) {
                symref_t *sr = in_chd_state.chd()->findSymref(cname,
                    in_mode);
                if (!sr || !sr->get_defseen() || !sr->get_offset())
                    tmag *= in_phys_scale;
            }
        }
        CDtx tx(at.refly, at.ax, at.ay, x, y, tmag);
        CDap ap(at.nx, at.ny, tdx, tdy);

        CDc *newo;
        CallDesc calldesc(cname, 0);
        if (OIfailed(in_sdesc->makeCall(&calldesc, &tx, &ap, CDcallDb, &newo)))
            return (false);
        if (newo)
            a_add_properties(in_sdesc, newo);
        return (true);
    }

    if (info())
        info()->add_inst(nx > 1 || ny > 1);

    if (in_ignore_inst)
        return (true);
    if (in_sdesc || (in_listonly && in_symref)) {
        int x = scale(uobj.placement.x);
        int y = scale(uobj.placement.y);
        double angle = uobj.placement.angle;
        double magn = uobj.placement.magnification;
        if (nx > 1)
            dx = scale(dx);
        if (ny > 1)
            dy = scale(dy);
        char *name = uobj.placement.name;
        const char *cellname = alias(name);

        int ax, ay;
        char *emsg = FIO()->GdsParamSet(&angle, &magn, &ax, &ay);
        if (emsg) {
            warning(emsg, name, x, y);
            delete [] emsg;
        }

        if (in_listonly && in_symref) {
            nametab_t *ntab = get_sym_tab(in_mode);
            cref_t *sr;
            ticket_t ctk = ntab->new_cref(&sr);
#ifdef VIAS_IN_LISTONLY
            if (in_mode == Physical) {
                CDcellName cname = CD()->CellNameTableAdd(cellname);
                cname = check_sub_master(cname);
                cellname = Tstring(cname);
            }
#endif
            symref_t *ptr = get_symref(cellname, in_mode);
            if (!ptr) {
                sr->set_refptr(ntab->new_symref(cellname, in_mode, &ptr));
                add_symref(ptr, in_mode);
            }
            else
                sr->set_refptr(ptr->get_ticket());

            CDtx tx(uobj.placement.flip_y, ax, ay, x, y, magn);
            sr->set_pos_x(tx.tx);
            sr->set_pos_y(tx.ty);
            CDattr at(&tx, 0);
            if (nx > 1 || ny > 1) {
                at.nx = nx;
                at.ny = ny;
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

            // If we're reading into the database and scaling, we
            // don't want to scale library, standard cell, or pcell
            // sub-masters.  Instead, these are always read in
            // unscaled, and instance placements are scaled.  We
            // check/set this here.

            double tmag = magn;
            if (in_mode == Physical && in_needs_mult &&
                    in_action == cvOpenModeDb) {
                CDs *sd = CDcdb()->findCell(cname, in_mode);
                if (sd && (sd->isViaSubMaster() || sd->isPCellSubMaster() ||
                        sd->isLibrary()))
                    tmag *= in_phys_scale;
                else if (in_chd_state.chd()) {
                    symref_t *sr = in_chd_state.chd()->findSymref(cname,
                        in_mode);
                    if (!sr || !sr->get_defseen() || !sr->get_offset())
                        tmag *= in_phys_scale;
                }
            }
            CDtx tx(uobj.placement.flip_y, ax, ay, x, y, tmag);
            CDap ap(nx, ny, dx, dy);

            CDc *newo;
            CallDesc calldesc(cname, 0);
            if (OIfailed(in_sdesc->makeCall(&calldesc, &tx, &ap, CDcallDb,
                    &newo)))
                return (false);
            if (newo)
                a_add_properties(in_sdesc, newo);

            if (in_symref) {
                // Add a symref for this instance if necessary.
                nametab_t *ntab = get_sym_tab(in_mode);
                symref_t *ptr = get_symref(Tstring(cname), in_mode);
                if (!ptr) {
                    ntab->new_symref(Tstring(cname), in_mode, &ptr);
                    add_symref(ptr, in_mode);
                }
            }
        }
    }
    return (true);
}


bool
oas_in::a_text()
{
    if (in_ignore_text)
        return (true);
    if (!a_layer(uobj.text.textlayer, uobj.text.texttype))
        return (false);
    if ((in_savebb || in_sdesc) && !in_skip_elem) {
        if (info())
            info()->add_text();
        char *str = uobj.text.string;
        Label la;
        la.x = scale(uobj.text.x);
        la.y = scale(uobj.text.y);
        la.width = scale(uobj.text.width);
        la.xform = uobj.text.xform;

        // in_sdesc can be 0
        hyList *hpl = new hyList(in_sdesc, str, HYcvAscii);
        char *string = hyList::string(hpl, HYcvPlain, false);
        // This is the displayed string, not necessarily the same as the
        // label text.

        double tw, th;
        CD()->DefaultLabelSize(string, in_mode, &tw, &th);
        delete [] string;

        if (la.width)
            la.height = mmRnd(la.width*th/tw);
        else {
            if (in_mode == Physical) {
                la.width = INTERNAL_UNITS(tw*in_ext_phys_scale);
                la.height = INTERNAL_UNITS(th*in_ext_phys_scale);
            }
            else {
                la.width = ELEC_INTERNAL_UNITS(tw);
                la.height = ELEC_INTERNAL_UNITS(th);
            }
        }

        if (in_savebb) {
            BBox BB;
            la.computeBB(&BB);
            in_cBB.add(&BB);
        }
        else if (!in_areafilt || la.intersect(&in_cBB, false)) {
            la.label = hyList::dup(hpl);
            CDla *newo;
            CDerrType err = in_sdesc->makeLabel(in_layers->ldesc, &la, &newo);
            if (err != CDok) {
                hyList::destroy(hpl);
                if (err == CDbadLabel) {
                    warning("bad label (ignored)", la.x, la.y,
                        uobj.text.textlayer, uobj.text.texttype);
                    return (true);
                }
                return (false);
            }
            if (newo)
                a_add_properties(in_sdesc, newo);
            else
                hyList::destroy(la.label);
        }
        hyList::destroy(hpl);
    }
    return (true);
}


bool
oas_in::a_rectangle()
{
    if (!a_layer(uobj.rectangle.layer, uobj.rectangle.datatype))
        return (false);
    if ((in_savebb || in_sdesc) && !in_skip_elem) {
        if (info())
            info()->add_box(1);
        int l = scale(uobj.rectangle.x);
        int b = scale(uobj.rectangle.y);
        int r = l + scale(uobj.rectangle.width);
        int t = b + scale(uobj.rectangle.height);
        BBox BB(l, b, r, t);
        BB.fix();

        if (in_savebb)
            in_cBB.add(&BB);
        else if (!in_areafilt || BB.intersect(&in_cBB, false)) {
            CDo *newo;
            CDerrType err = in_sdesc->makeBox(in_layers->ldesc, &BB, &newo);
            if (err != CDok) {
                if (err == CDbadBox) {
                    warning("bad box (ignored)", BB.left, BB.bottom,
                        uobj.rectangle.layer, uobj.rectangle.datatype);
                    return (true);
                }
                return (false);
            }
            if (newo) {
                a_add_properties(in_sdesc, newo);
                if (FIO()->IsMergeInput() && in_mode == Physical) {
                    if (!in_sdesc->mergeBox(newo, false))
                        return (false);
                }
            }
        }
    }
    return (true);
}


bool
oas_in::a_polygon()
{
    if (!a_layer(uobj.polygon.layer, uobj.polygon.datatype))
        return (false);
    bool ret = true;
    if ((in_savebb || in_sdesc) && !in_skip_elem) {
        if (info())
            info()->add_poly(uobj.polygon.point_list_size + 2);
        Poly po;
        po.numpts = uobj.polygon.point_list_size + 2;
        po.points = new Point[po.numpts];
        po.points[0].set(scale(uobj.polygon.x), scale(uobj.polygon.y));
        po.points[po.numpts-1] = po.points[0];
        for (unsigned int i = 0; i < uobj.polygon.point_list_size; i++)
            po.points[i+1].set(
                po.points[i].x + scale(uobj.polygon.point_list[i].x),
                po.points[i].y + scale(uobj.polygon.point_list[i].y));
        ret = a_polygon_save(&po);
        delete [] po.points;
    }
    return (ret);
}


bool
oas_in::a_path()
{
    if (!a_layer(uobj.path.layer, uobj.path.datatype))
        return (false);

    if ((in_savebb || in_sdesc) && !in_skip_elem) {
        if (info())
            info()->add_wire(uobj.path.point_list_size + 1);
        int numpts = uobj.path.point_list_size + 1;
        Wire w(scale(2*uobj.path.half_width), CDWIRE_FLUSH, numpts,
            new Point[numpts]);
        w.points[0].set(scale(uobj.path.x), scale(uobj.path.y));
        for (unsigned int i = 0; i < uobj.path.point_list_size; i++)
            w.points[i+1].set(
                w.points[i].x + scale(uobj.path.point_list[i].x),
                w.points[i].y + scale(uobj.path.point_list[i].y));
        if (uobj.path.start_extension == (int)uobj.path.half_width &&
                uobj.path.end_extension == (int)uobj.path.half_width)
            w.set_wire_style(uobj.path.rounded_end ?
                CDWIRE_ROUND : CDWIRE_EXTEND);
        else if (uobj.path.start_extension != 0 ||
                uobj.path.end_extension != 0) {
            warning("unsupported pathtype converted to 0",
                w.points->x, w.points->y, uobj.path.layer, uobj.path.datatype);
            convert_4to0(&w.points[0].x, &w.points[0].y, w.points[1].x,
                w.points[1].y, scale(uobj.path.start_extension));
            convert_4to0(&w.points[w.numpts-1].x, &w.points[w.numpts-1].y,
                w.points[w.numpts-2].x, w.points[w.numpts-2].y,
                scale(uobj.path.end_extension));
        }

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
                CDerrType err = in_sdesc->makeWire(in_layers->ldesc, &w, &newo,
                    &wchk_flags);
                if (err != CDok) {
                    if (err == CDbadWire) {
                        warning("bad wire (ignored)", x, y,
                            uobj.path.layer, uobj.path.datatype);
                    }
                    else {
                        delete [] pres;
                        return (false);
                    }
                }
                else if (newo) {
                    a_add_properties(in_sdesc, newo);

                    if (wchk_flags) {
                        char buf[256];
                        Wire::flagWarnings(buf, wchk_flags,
                            "questionable wire, warnings: ");
                        warning(buf, x, y, uobj.path.layer, uobj.path.datatype);
                    }
                }
                if (!pres)
                    break;
                warning("breaking colinear reentrant wire", x, y,
                    uobj.path.layer, uobj.path.datatype);
                w.points = pres;
                w.numpts = nres;
            }
        }
        delete [] w.points;
    }
    return (true);
}


bool
oas_in::a_trapezoid()
{
    if (!a_layer(uobj.trapezoid.layer, uobj.trapezoid.datatype))
        return (false);

    bool ret = true;
    if ((in_savebb || in_sdesc) && !in_skip_elem) {
        Poly po;
        ret = trap2poly(&po);
        if (!ret)
            Errs()->add_error("a_trapezoid: bad geometry.");
        else
            ret = a_polygon_save(&po);
        delete [] po.points;
    }
    return (ret);
}


bool
oas_in::a_ctrapezoid()
{
    if (!a_layer(uobj.ctrapezoid.layer, uobj.ctrapezoid.datatype))
        return (false);

    bool ret = true;
    if ((in_savebb || in_sdesc) && !in_skip_elem) {
        int x = scale(uobj.ctrapezoid.x);
        int y = scale(uobj.ctrapezoid.y);
        unsigned int w = scale(uobj.ctrapezoid.width);
        unsigned int h = scale(uobj.ctrapezoid.height);
        Poly po;
        ret = (this->*ctrtab[uobj.ctrapezoid.type])(&po, x, y, w, h);
        if (!ret)
            Errs()->add_error("a_ctrapezoid: bad geometry.");
        else
            ret = a_polygon_save(&po);
        delete [] po.points;
    }
    return (ret);
}


bool
oas_in::a_circle()
{
    if (!a_layer(uobj.circle.layer, uobj.circle.datatype))
        return (false);

    bool ret = true;
    if ((in_savebb || in_sdesc) && !in_skip_elem) {
        Poly po;
        int x = scale(uobj.circle.x);
        int y = scale(uobj.circle.y);
        unsigned int r = scale(uobj.circle.radius);
        int tsp = GEO()->spotSize();
        GEO()->setSpotSize(0);
        po.points = GEO()->makeArcPath(
            &po.numpts, (in_mode == Electrical), x, y, r, r, 0, 0, 0.0, 0.0);
        GEO()->setSpotSize(tsp);
        ret = a_polygon_save(&po);
        delete [] po.points;
    }
    return (ret);
}


bool
oas_in::a_property()
{
    oas_property_value *pv = modal.last_value_list;
    if (!strcmp(modal.last_property_name, "XIC_PROPERTIES")) {
        for (unsigned int i = 0; i < modal.last_value_list_size; i++) {
            int value;
            if (pv[i].type == 8)
                value = pv[i].u.uval;
            else if (pv[i].type == 9)
                value = pv[i].u.ival;
            else {
                Errs()->add_error(
                    "a_property: bad XIC_PROPERTIES value type.");
                return (false);
            }
            i++;
            char *string;
            if (pv[i].type == 10 || pv[i].type == 12 ||
                    pv[i].type == 13 || pv[i].type == 15)
                string = pv[i].u.sval;
            else {
                Errs()->add_error(
                    "a_property: bad XIC_PROPERTIES string type.");
                return (false);
            }

            CDp *p = new CDp(string, value);
            p->set_next_prp(in_prpty_list);
            in_prpty_list = p;
        }
    }
    else if (!strcmp(modal.last_property_name, "XIC_LABEL")) {
        if (in_peek_record != 19) {
            // TEXT only
            Errs()->add_error("a_property: XIC_LABEL on non-text.");
            return (false);
        }
        const char *msg = "a_property: bad XIC_LABEL format.";
        if (modal.last_value_list_size < 2) {
            Errs()->add_error(msg);
            return (false);
        }
        if (pv[0].type != 8 ||pv[1].type != 8) {
            Errs()->add_error(msg);
            return (false);
        }
        uobj.text.width = pv[0].u.uval;
        uobj.text.xform = pv[1].u.uval;
    }
    else if (!strcmp(modal.last_property_name, "XIC_ROUNDED_END")) {
        if (in_peek_record != 22) {
            // PATH only
            Errs()->add_error(
                "a_property: XIC_ROUNDED_END on non-path.");
            return (false);
        }
        uobj.path.rounded_end = true;
    }
    else if (!strcmp(modal.last_property_name, "XIC_SOURCE")) {
        // ID marker, file was generated by Xic.
        in_xic_source = true;
    }
    return (true);
}


bool
oas_in::a_standard_property()
{
    char *name = modal.last_property_name;
    bool bad_data = false;
    if (in_peek_record == 1) {
        // START properties
        bad_data = true;
        if (!strcmp(name, "S_MAX_SIGNED_INTEGER_WIDTH")) {
            if (modal.last_value_list && modal.last_value_list->type == 8) {
                in_max_signed_integer_width = modal.last_value_list->u.uval;
                return (true);
            }
        }
        else if (!strcmp(name, "S_MAX_UNSIGNED_INTEGER_WIDTH")) {
            if (modal.last_value_list && modal.last_value_list->type == 8) {
                in_max_unsigned_integer_width = modal.last_value_list->u.uval;
                return (true);
            }
        }
        else if (!strcmp(name, "S_MAX_STRING_LENGTH")) {
            if (modal.last_value_list && modal.last_value_list->type == 8) {
                in_max_string_length = modal.last_value_list->u.uval;
                return (true);
            }
        }
        else if (!strcmp(name, "S_POLYGON_MAX_VERTICES")) {
            if (modal.last_value_list && modal.last_value_list->type == 8) {
                in_polygon_max_vertices = modal.last_value_list->u.uval;
                return (true);
            }
        }
        else if (!strcmp(name, "S_PATH_MAX_VERTICES")) {
            if (modal.last_value_list && modal.last_value_list->type == 8) {
                in_path_max_vertices = modal.last_value_list->u.uval;
                return (true);
            }
        }
        else if (!strcmp(name, "S_TOP_CELL")) {
            if (modal.last_value_list && modal.last_value_list->type == 12) {
                in_top_cell = modal.last_value_list->u.sval;
                return (true);
            }
        }
        else if (!strcmp(name, "S_BOUNDING_BOXES_AVAILABLE")) {
            if (modal.last_value_list && modal.last_value_list->type == 8) {
                in_bounding_boxes_available = modal.last_value_list->u.uval;
                return (true);
            }
        }
        else
            bad_data = false;
    }
    else if (in_peek_record == 3 || in_peek_record == 4) {
        // CELLNAME properties
        bad_data = true;
        if (!strcmp(name, "S_BOUNDING_BOX")) {
            return (true);
        }
        else if (!strcmp(name, "S_CELL_OFFSET")) {
            return (true);
        }
        else
            bad_data = false;
    }
    else if ((in_peek_record >= 17 && in_peek_record <= 27) ||
            in_peek_record == 32 || in_peek_record == 33) {
        // <element> properties
        bad_data = true;
        if (!strcmp(name, "S_GDS_PROPERTY")) {
            return (true);
        }
        else
            bad_data = false;
    }
    if (bad_data)
        FIO()->ifPrintCvLog(IFLOG_WARN,
            "ignored bad data for %s at offset %llu.",
            name, (unsigned long long)in_offset);
    else
        FIO()->ifPrintCvLog(IFLOG_WARN,
            "ignored unknown standard property %s at offset %llu.",
            name, (unsigned long long)in_offset);
    return (true);
}


bool
oas_in::a_endlib()
{
    return (end_cell());
}


void
oas_in::a_clear_properties()
{
    CDp::destroy(in_prpty_list);
    in_prpty_list = 0;
}


// Look at the layer/datatype mapping to internal layers, The list of
// internal layers to output is returned in in_layers.  If this is
// changed from previous, *changed is set true.  If the layer is in
// the skip list, in_skip_elem is set.  If no mapping exists, a new
// name is defined in in_layer_name.  If not nocreate, the new layer
// is created and added to the database.  Otherwise, in_layers is set
// to 0, and the caller must handle this case.
//
bool
oas_in::a_layer(unsigned int layer, unsigned int datatype,
    bool *changed, bool nocreate)
{
    const char *msg = "Mapping all datatypes for layer %d to layer %s.";
    const char *msg1 = "Mapping layer %d datatype %d to layer %s.";

    if (changed)
        *changed = false;
    if (layer >= GDS_MAX_LAYERS) {
        Errs()->add_error("Illegal layer number %d.", layer);
        return (false);
    }
    if (datatype >= GDS_MAX_DTYPES) {
        Errs()->add_error("Illegal datatype number %d.", datatype);
        return (false);
    }

    in_skip_elem = false;

    SymTab *layer_tab =
        in_mode == Physical ? in_phys_layer_tab : in_elec_layer_tab;
    if (!layer_tab) {
        if (in_mode == Physical)
            // Optimize layer hash table for 128 entries.
            layer_tab = in_phys_layer_tab = new SymTab(false, false, 7);
        else
            // Optimize layer hash table for 16 entries.
            layer_tab = in_elec_layer_tab = new SymTab(false, false, 4);
    }

    unsigned long ll = (layer << 16) | datatype;

    gds_lspec *lspec = (gds_lspec*)SymTab::get(layer_tab, ll);
    if (lspec != (gds_lspec*)ST_NIL) {
        layer = lspec->layer;
        datatype = lspec->dtype;
        strncpy(in_layer_name, lspec->hexname, 12);
        in_layers = lspec->list;

        if (layer != (unsigned)in_curlayer ||
                datatype != (unsigned)in_curdtype) {
            in_curlayer = layer;
            in_curdtype = datatype;
            if (changed)
                *changed = true;
            if (in_listonly && info())
                info()->add_layer(in_layer_name);
        }
        else if (changed && in_layers && in_layers->next)
            // writing multiple layers
            *changed = true;

        if (!in_layers && !in_layer_name[0])
            in_skip_elem = true;
        return (true);
    }

    char buf[128];
    if (in_mode == Physical) {
        // See if layer is in skip list, before aliasing.
        if (in_lcheck && !in_lcheck->layer_ok(0, layer, datatype)) {
            in_skip_elem = true;
            layer_tab->add(ll, new gds_lspec(layer, datatype, 0, 0), false);
            return (true);
        }
        if (in_layer_alias) {
            strmdata::hexpr(buf, layer, datatype);
            const char *new_lname = in_layer_alias->alias(buf);
            if (new_lname) {
                int lyr, dt;
                if (!strmdata::hextrn(new_lname, &lyr, &dt) || lyr < 0 ||
                        dt < 0) {
                    FIO()->ifPrintCvLog(IFLOG_WARN,
                        "layer alias %s not 4 or 8 byte hex, "
                        "mapping from %s removed.",
                        new_lname, buf);
                    in_layer_alias->remove(buf);
                }
                else {
                    layer = lyr;
                    datatype = dt;
                }
            }
        }
    }

    if (in_listonly) {
        strmdata::hexpr(buf, layer, datatype);
        if (info())
            info()->add_layer(buf);
        in_curlayer = layer;
        in_curdtype = datatype;
        return (true);
    }

    in_layers = FIO()->GetGdsInputLayers(layer, datatype, in_mode);
    if (in_layers) {
        in_curlayer = layer;
        in_curdtype = datatype;
        if (changed)
            *changed = true;
        layer_tab->add(ll, new gds_lspec(layer, datatype, 0, in_layers),
            false);
        return (true);
    }

    bool error;
    CDl *ld = FIO()->MapGdsLayer(layer, datatype, in_mode,
        in_layer_name, !nocreate, &error);
    if (ld) {
        in_layers = new CDll(ld, 0);
        in_curlayer = layer;
        in_curdtype = datatype;
        if (FIO()->IsNoMapDatatypes())
            FIO()->ifPrintCvLog(IFLOG_INFO, msg, layer, ld->name());
        else
            FIO()->ifPrintCvLog(IFLOG_INFO, msg1, layer, datatype, ld->name());
        if (changed)
            *changed = true;
        layer_tab->add(ll, new gds_lspec(layer, datatype, 0, in_layers),
            false);
        return (true);
    }
    else if (error)
        return (false);
    in_curlayer = layer;
    in_curdtype = datatype;
    if (changed)
        *changed = true;
    layer_tab->add(ll, new gds_lspec(layer, datatype, in_layer_name, 0),
        false);
    return (true);
}


void
oas_in::a_add_properties(CDs *sdesc, CDo *odesc)
{
    if (sdesc) {
        stringlist *s0 = sdesc->prptyApplyList(odesc, &in_prpty_list);
        for (stringlist *s = s0; s; s = s->next)
            warning(s->string);
        delete s0;

        if (!odesc) {
            sdesc->setPCellFlags();
            if (sdesc->isPCellSubMaster())
                sdesc->setPCellReadFromFile(true);
        }
    }
}


bool
oas_in::a_polygon_save(Poly *po)
{
    if (in_savebb) {
        BBox BB;
        po->computeBB(&BB);
        in_cBB.add(&BB);
    }
    else if (!in_areafilt || po->intersect(&in_cBB, false)) {
        CDpo *newo;
        int pchk_flags;
        int x = po->points->x;
        int y = po->points->y;
        // This will zero po.points!
        CDerrType err =
            in_sdesc->makePolygon(in_layers->ldesc, po, &newo, &pchk_flags);
        if (err != CDok) {
            if (err == CDbadPolygon) {
                warning("bad polygon (ignored)", x, y,
                    uobj.polygon.layer, uobj.polygon.datatype);
                return (true);
            }
            return (false);
        }
        if (newo) {
            a_add_properties(in_sdesc, newo);

            if (pchk_flags & PCHK_REENT)
                warning("badly formed polygon", x, y,
                    uobj.polygon.layer, uobj.polygon.datatype);
            if (pchk_flags & PCHK_ZERANG) {
                if (pchk_flags & PCHK_FIXED)
                    warning("repaired polygon 0/180 angle", x, y,
                        uobj.polygon.layer, uobj.polygon.datatype);
                else
                    warning("polygon with 0/180 angle", x, y,
                        uobj.polygon.layer, uobj.polygon.datatype);
            }
        }
    }
    return (true);
}


//
//===========================================================================
// Actions for format translation
//===========================================================================
//

bool
oas_in::ac_cell(const char *name)
{
    bool ret = true;
    if (in_transform <= 0) {
        if (in_skip_cell) {
            delete [] in_cellname;
            in_cellname = lstring::copy(name);
            return (true);
        }

        // end previous struct
        if (!end_cell())
            return (false);

        if (info())
            info()->add_cell(in_chd_state.symref());

        in_cell_offset = in_offset;
        in_curlayer = -1;
        in_curdtype = -1;
        delete [] in_cellname;
        in_cellname = lstring::copy(alias(name));

        ret = in_out->write_struct(in_cellname, &in_cdate, &in_mdate);
    }
    return (ret);
}


bool
oas_in::ac_placement(int dx, int dy, unsigned int nx, unsigned int ny)
{
    if (in_chd_state.gen()) {
        if (!in_chd_state.symref())
            return (true);
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
        if (cp->should_skip())
            return (true);
        tristate_t fst = in_chd_state.get_inst_use_flag();
        if (fst == ts_unset)
            return (true);

        CDcellName cellname = cp->get_name();
        CDattr at;
        if (!CD()->FindAttr(c->attr, &at)) {
            Errs()->add_error("Unresolved transform ticket %d.", c->attr);
            return (false);
        }
        int x, y, tdx, tdy;
        if (in_mode == Physical) {
            double tscale = in_ext_phys_scale;
            x = mmRnd(c->tx*tscale);
            y = mmRnd(c->ty*tscale);
            tdx = mmRnd(at.dx*tscale);
            tdy = mmRnd(at.dy*tscale);
        }
        else {
            x = c->tx;
            y = c->ty;
            tdx = at.dx;
            tdy = at.dy;
        }

        // If we're scaling, we don't want to scale library, standard
        // cell, or pcell sub-masters.  Instead, these are always
        // unscaled, and instance placements are scaled.

        double tmag = at.magn;
        if (in_mode == Physical && in_needs_mult &&
                in_action == cvOpenModeTrans) {
            CDs *sd = CDcdb()->findCell(cellname, in_mode);
            if (sd && (sd->isViaSubMaster() || sd->isPCellSubMaster() ||
                    sd->isLibrary()))
                tmag *= in_phys_scale;
            else if (in_chd_state.chd()) {
                symref_t *sr = in_chd_state.chd()->findSymref(cellname,
                    in_mode);
                if (!sr || !sr->get_defseen() || !sr->get_offset())
                    tmag *= in_phys_scale;
            }
        }

        Instance inst;
        inst.magn = tmag;
        inst.name = Tstring(cellname);
        inst.nx = at.nx;
        inst.ny = at.ny;
        inst.dx = tdx;
        inst.dy = tdy;
        inst.origin.set(x, y);
        inst.reflection = at.refly;
        inst.set_angle(at.ax, at.ay);

        if (!ac_placement_backend(&inst, cp, (fst == ts_set)))
            return (false);
        return (true);
    }

    if (info())
        info()->add_inst(nx > 1 || ny > 1);

    bool ret = true;
    if (!in_ignore_inst) {
        int x = uobj.placement.x;
        int y = uobj.placement.y;
        double angle = uobj.placement.angle;
        double magn = uobj.placement.magnification;
        char *name = uobj.placement.name;
        const char *cellname = alias(name);

        int ax, ay;
        char *emsg = FIO()->GdsParamSet(&angle, &magn, &ax, &ay);
        if (emsg) {
            warning(emsg, name, scale(x), scale(y));
            delete [] emsg;
        }

        // If we're scaling, we don't want to scale library, standard
        // cell, or pcell sub-masters.  Instead, these are always
        // unscaled, and instance placements are scaled.

        double tmag = magn;
        if (in_mode == Physical && in_needs_mult &&
                in_action == cvOpenModeTrans) {
            CDs *sd = CDcdb()->findCell(cellname, in_mode);
            if (sd && (sd->isViaSubMaster() || sd->isPCellSubMaster() ||
                    sd->isLibrary()))
                tmag *= in_phys_scale;
            else if (in_chd_state.chd()) {
                symref_t *sr = in_chd_state.chd()->findSymref(cellname,
                    in_mode);
                if (!sr || !sr->get_defseen() || !sr->get_offset())
                    tmag *= in_phys_scale;
            }
        }

        Instance inst;
        inst.magn = tmag;
        inst.angle = angle;
        inst.name = cellname;
        inst.nx = nx;
        inst.ny = ny;
        inst.dx = scale(dx);
        inst.dy = scale(dy);
        inst.origin.set(scale(x), scale(y));
        inst.reflection = uobj.placement.flip_y;
        inst.ax = ax;
        inst.ay = ay;

        ret = ac_placement_backend(&inst, 0, false);
    }
    return (ret);
}


bool
oas_in::ac_placement_backend(Instance *inst, symref_t *p, bool no_area_test)
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
                        Tstring(p->get_name()));
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
oas_in::ac_text()
{
    if (in_ignore_text)
        return (true);
    if (!ac_layer(uobj.text.textlayer, uobj.text.texttype))
        return (false);
    bool ret = true;
    if (in_skip_elem)
        return (true);

    if (info())
        info()->add_text();

    Text text;
    text.x = uobj.text.x;
    text.y = uobj.text.y;
    text.width = scale(uobj.text.width);
    text.xform = uobj.text.xform;

    hyList *hpl = new hyList(in_sdesc, uobj.text.string, HYcvAscii);
    char *string = hyList::string(hpl, HYcvPlain, false);

    double tw, th;
    CD()->DefaultLabelSize(string, in_mode, &tw, &th);
    delete [] string;
    hyList::destroy(hpl);

    if (text.width)
        text.height = mmRnd(text.width*th/tw);
    else {
        if (in_mode == Physical) {
            text.width = INTERNAL_UNITS(tw*in_ext_phys_scale);
            text.height = INTERNAL_UNITS(th*in_ext_phys_scale);
        }
        else {
            text.width = ELEC_INTERNAL_UNITS(tw);
            text.height = ELEC_INTERNAL_UNITS(th);
        }
    }
    text.text = uobj.text.string;

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
                    ret = ac_text_prv(&text);
                    if (!ret)
                        break;
                    TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            else
                ret = ac_text_prv(&text);

            in_transform--;
            TPop();
            if (!ret)
                break;
        }
    }
    else
        ret = ac_text_prv(&text);

    return (ret);
}


bool
oas_in::ac_text_prv(const Text *ctext)
{
    Text text(*ctext);
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
oas_in::ac_rectangle()
{
    if (!ac_layer(uobj.rectangle.layer, uobj.rectangle.datatype))
        return (false);
    if (in_skip_elem)
        return (true);

    if (info())
        info()->add_box(1);
    int l = scale(uobj.rectangle.x);
    int b = scale(uobj.rectangle.y);
    int r = l + scale(uobj.rectangle.width);
    int t = b + scale(uobj.rectangle.height);
    BBox BB(l, b, r, t);
    BB.fix();

    bool ret = true;
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
                    ret = ac_rectangle_prv(BB);
                    wrote_once = true;
                    if (!ret)
                        break;
                    TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            else {
                if (wrote_once)
                    BB = BBbak;
                ret = ac_rectangle_prv(BB);
                wrote_once = true;
            }

            in_transform--;
            TPop();
            if (!ret)
                break;
        }
    }
    else
        ret = ac_rectangle_prv(BB);

    return (ret);
}


bool
oas_in::ac_rectangle_prv(BBox &BB)
{
    bool ret = true;
    if (in_transform) {
        Point *p;
        TBB(&BB, &p);
        if (p) {
            Poly po(5, p);
            if (!in_areafilt || po.intersect(&in_cBB, false)) {
                TPush();
                ret = in_out->write_poly(&po);
                delete [] p;
                TPop();
            }
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
oas_in::ac_polygon()
{
    if (!ac_layer(uobj.polygon.layer, uobj.polygon.datatype))
        return (false);
    bool ret = true;
    if (!in_skip_elem) {
        if (info())
            info()->add_poly(uobj.polygon.point_list_size + 2);

        Poly po;
        po.numpts = uobj.polygon.point_list_size + 2;
        po.points = new Point[po.numpts];
        po.points[0].set(scale(uobj.polygon.x), scale(uobj.polygon.y));
        po.points[po.numpts-1] = po.points[0];
        for (unsigned int i = 0; i < uobj.polygon.point_list_size; i++)
            po.points[i+1].set(
                po.points[i].x + scale(uobj.polygon.point_list[i].x),
                po.points[i].y + scale(uobj.polygon.point_list[i].y));
        ret = ac_polygon_save(&po);
        delete [] po.points;
    }
    return (ret);
}


bool
oas_in::ac_path()
{
    if (!ac_layer(uobj.path.layer, uobj.path.datatype))
        return (false);
    if (in_skip_elem)
        return (true);

    if (info())
        info()->add_wire(uobj.path.point_list_size + 1);
    int numpts = uobj.path.point_list_size + 1;
    int pwidth = scale(2*uobj.path.half_width);
    Wire w(pwidth, CDWIRE_FLUSH, numpts, new Point[numpts]);
    w.points[0].set(scale(uobj.path.x), scale(uobj.path.y));
    for (unsigned int i = 0; i < uobj.path.point_list_size; i++)
        w.points[i+1].set(
            w.points[i].x + scale(uobj.path.point_list[i].x),
            w.points[i].y + scale(uobj.path.point_list[i].y));
    if (uobj.path.start_extension == (int)uobj.path.half_width &&
            uobj.path.end_extension == (int)uobj.path.half_width)
        w.set_wire_style(uobj.path.rounded_end ?
                CDWIRE_ROUND : CDWIRE_EXTEND);
    else if (uobj.path.start_extension != 0 ||
            uobj.path.end_extension != 0) {
        warning("unsupported pathtype converted to 0",
            w.points->x, w.points->y, uobj.path.layer, uobj.path.datatype);
        convert_4to0(&w.points[0].x, &w.points[0].y, w.points[1].x,
            w.points[1].y, scale(uobj.path.start_extension));
        convert_4to0(&w.points[w.numpts-1].x, &w.points[w.numpts-1].y,
            w.points[w.numpts-2].x, w.points[w.numpts-2].y,
            scale(uobj.path.end_extension));
    }
    Point::removeDups(w.points, &w.numpts);

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
                    ret = ac_path_prv(w);
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
                ret = ac_path_prv(w);
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
        ret = ac_path_prv(w);

    delete [] w.points;
    return (ret);
}


bool
oas_in::ac_path_prv(Wire &w)
{
    if (in_transform) {
        TPath(w.numpts, w.points);
        w.set_wire_width(mmRnd(w.wire_width() * TGetMagn()));
    }
    bool ret = true;
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
oas_in::ac_trapezoid()
{
    if (!ac_layer(uobj.path.layer, uobj.path.datatype))
        return (false);
    bool ret = true;
    if (!in_skip_elem) {
        Poly po;
        ret = trap2poly(&po);
        if (!ret)
            Errs()->add_error("a_trapezoid: bad geometry.");
        else
            ret = ac_polygon_save(&po);
        delete [] po.points;
    }
    return (ret);
}


bool
oas_in::ac_ctrapezoid()
{
    if (!ac_layer(uobj.path.layer, uobj.path.datatype))
        return (false);
    bool ret = true;
    if (!in_skip_elem) {
        int x = scale(uobj.ctrapezoid.x);
        int y = scale(uobj.ctrapezoid.y);
        unsigned int w = scale(uobj.ctrapezoid.width);
        unsigned int h = scale(uobj.ctrapezoid.height);
        Poly po;
        ret = (this->*ctrtab[uobj.ctrapezoid.type])(&po, x, y, w, h);
        if (!ret)
            Errs()->add_error("a_ctrapezoid: bad geometry.");
        else
            ret = ac_polygon_save(&po);
        delete [] po.points;
    }
    return (ret);
}


bool
oas_in::ac_circle()
{
    if (!ac_layer(uobj.path.layer, uobj.path.datatype))
        return (false);
    bool ret = true;
    if (!in_skip_elem) {
        Poly po;
        int x = scale(uobj.circle.x);
        int y = scale(uobj.circle.y);
        unsigned int r = scale(uobj.circle.radius);
        int tsp = GEO()->spotSize();
        GEO()->setSpotSize(0);
        po.points = GEO()->makeArcPath(
            &po.numpts, (in_mode == Electrical), x, y, r, r, 0, 0, 0.0, 0.0);
        GEO()->setSpotSize(tsp);
        ret = ac_polygon_save(&po);
        delete [] po.points;
    }
    return (ret);
}


bool
oas_in::ac_property()
{
    if (!a_property())
        return (false);

    // Reverse the list, so order is consistent with other translators.
    CDp *p0 = 0;
    while (in_prpty_list) {
        CDp *px = in_prpty_list;
        in_prpty_list = in_prpty_list->next_prp();
        px->set_next_prp(p0);
        p0 = px;
    }
    in_prpty_list = p0;

    bool ret = true;
    for (CDp *p = in_prpty_list; p; p = p->next_prp()) {
        if (!in_out->queue_property(p->value(), p->string())) {
            Errs()->add_error("ac_property: queue property failed.");
            ret = false;
            break;
        }
    }
    a_clear_properties();
    return (ret);
}


bool
oas_in::ac_standard_property()
{
    return (true);
}


bool
oas_in::ac_endlib()
{
    if (!end_cell())
        return (false);
    if (!in_cv_no_endlib) {
        if (!in_out->write_endlib(0))
            return (false);
    }
    return (true);
}


void
oas_in::ac_clear_properties()
{
    a_clear_properties();
    if (in_out)
        in_out->clear_property_queue();
}


bool
oas_in::ac_layer(unsigned int layer, unsigned int datatype)
{
    bool lchg;
    if (!a_layer(layer, datatype, &lchg, true))
        return (false);
    if (in_skip_elem)
        // skipping this layer
        return (true);
    if (lchg) {
        if (in_layers) {
            Layer lyr(in_layers->ldesc->name(), in_curlayer, in_curdtype);
            if (!in_out->queue_layer(&lyr))
                return (false);
        }
        else {
            Layer lyr(in_layer_name, in_curlayer, in_curdtype);
            if (!in_out->queue_layer(&lyr))
                return (false);
            char buf[256];
            if (!in_undef_layers)
                in_undef_layers = new SymTab(true, false);
            if (SymTab::get(in_undef_layers, in_layer_name) == ST_NIL) {
                sprintf(buf,
                    "No mapping for layer %d datatype %d (naming layer %s)",
                    in_curlayer, in_curdtype, in_layer_name);
                in_undef_layers->add(lstring::copy(in_layer_name), 0, false);
                warning(buf);
            }
        }
    }
    return (true);
}


bool
oas_in::ac_polygon_save(Poly *po)
{
    bool ret = true;
    if (in_tf_list) {
        ts_reader tsr(in_tf_list);
        CDtx ttx;
        CDap ap;
        Point *scratch = new Point[po->numpts];
        memcpy(scratch, po->points, po->numpts*sizeof(Point));

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
                        memcpy(po->points, scratch, po->numpts*sizeof(Point));
                    ret = ac_polygon_save_prv(po);
                    wrote_once = true;
                    if (!ret)
                        break;
                    TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            else {
                if (wrote_once)
                    memcpy(po->points, scratch, po->numpts*sizeof(Point));
                ret = ac_polygon_save_prv(po);
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
        ret = ac_polygon_save_prv(po);

    return (ret);
}


bool
oas_in::ac_polygon_save_prv(Poly *po)
{
    bool ret = true;
    if (in_transform)
        TPath(po->numpts, po->points);
    if (po->is_rect()) {
        BBox BB(po->points);
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
        if (!in_areafilt || po->intersect(&in_cBB, false)) {
            TPush();
            bool need_out = true;
            if (in_areafilt && in_clip) {
                need_out = false;
                PolyList *pl = po->clip(&in_cBB, &need_out);
                for (PolyList *p = pl; p; p = p->next) {
                    ret = in_out->write_poly(&p->po);
                    if (!ret)
                        break;
                }
                PolyList::destroy(pl);
            }
            if (need_out)
                ret = in_out->write_poly(po);
            TPop();
        }
    }
    return (ret);
}
// End of actions


// Put the vertices of the current trapezoid into po.
//
bool
oas_in::trap2poly(Poly *po)
{
    if (!uobj.trapezoid.vertical) {
        int yl = uobj.trapezoid.y;
        int yu = uobj.trapezoid.y + uobj.trapezoid.height;
        int xul = uobj.trapezoid.x;
        int xll = xul;
        int xur = uobj.trapezoid.x + uobj.trapezoid.width;
        int xlr = xur;
        if (uobj.trapezoid.delta_a < 0)
            xll -= uobj.trapezoid.delta_a;
        else
            xul += uobj.trapezoid.delta_a;
        if (uobj.trapezoid.delta_b < 0)
            xur += uobj.trapezoid.delta_b;
        else
            xlr -= uobj.trapezoid.delta_b;
        po->numpts = 5 - (xul == xur) - (xll == xlr);
        if (po->numpts < 4)
            return (false);
        po->points = new Point[po->numpts];
        int i = 0;
        po->points[i].set(scale(xll), scale(yl));
        i++;
        po->points[i].set(scale(xul), scale(yu));
        if (xul != xur) {
            i++;
            po->points[i].set(scale(xur), scale(yu));
        }
        i++;
        po->points[i].set(scale(xlr), scale(yl));
        if (xll != xlr) {
            i++;
            po->points[i].set(scale(xll), scale(yl));
        }
    }
    else {
        int xl = uobj.trapezoid.x;
        int xu = xl + uobj.trapezoid.width;
        int yub = uobj.trapezoid.y;
        int ylb = yub;
        int yut = uobj.trapezoid.y + uobj.trapezoid.height;
        int ylt = yut;
        if (uobj.trapezoid.delta_a < 0)
            yub -= uobj.trapezoid.delta_a;
        else
            ylb += uobj.trapezoid.delta_a;
        if (uobj.trapezoid.delta_b < 0)
            ylt += uobj.trapezoid.delta_b;
        else
            yut -= uobj.trapezoid.delta_b;
        po->numpts = 5 - (yub == yut) - (ylb == ylt);
        if (po->numpts < 4)
            return (false);
        po->points = new Point[po->numpts];
        int i = 0;
        po->points[i].set(scale(xl), scale(ylb));
        if (ylb != ylt) {
            i++;
            po->points[i].set(scale(xl), scale(ylt));
        }
        i++;
        po->points[i].set(scale(xu), scale(yut));
        if (yub != yut) {
            i++;
            po->points[i].set(scale(xu), scale(yub));
        }
        i++;
        po->points[i].set(scale(xl), scale(ylb));
    }
    return (true);
}


// Functions for the ctrapezoid interpreter, each puts the appropriate
// points into the Poly.

bool
oas_in::ctrap0(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    if (w < h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x, y);
    po->points[1].set(x, y + h);
    po->points[2].set(x + w - h, y + h);
    po->points[3].set(x + w, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap1(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    if (w < h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x, y);
    po->points[1].set(x, y + h);
    po->points[2].set(x + w, y + h);
    po->points[3].set(x + w - h, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap2(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    if (w < h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x, y);
    po->points[1].set(x + h, y + h);
    po->points[2].set(x + w, y + h);
    po->points[3].set(x + w, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap3(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    if (w < h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x + h, y);
    po->points[1].set(x, y + h);
    po->points[2].set(x + w, y + h);
    po->points[3].set(x + w, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap4(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    if (w < 2*h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x, y);
    po->points[1].set(x + h, y + h);
    po->points[2].set(x + w - h, y + h);
    po->points[3].set(x + w, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap5(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    if (w < 2*h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x + h, y);
    po->points[1].set(x, y + h);
    po->points[2].set(x + w, y + h);
    po->points[3].set(x + w - h, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap6(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    // spec says w < 2*h should be fatal error - seems wrong
    if (w < h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x, y);
    po->points[1].set(x + h, y + h);
    po->points[2].set(x + w, y + h);
    po->points[3].set(x + w - h, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap7(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    // spec says w < 2*h should be fatal error - seems wrong
    if (w < h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x + h, y);
    po->points[1].set(x, y + h);
    po->points[2].set(x + w - h, y + h);
    po->points[3].set(x + w, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap8(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    if (w > h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x, y);
    po->points[1].set(x, y + h);
    po->points[2].set(x + w, y + h - w);
    po->points[3].set(x + w, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap9(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    if (w > h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x, y);
    po->points[1].set(x, y + h - w);
    po->points[2].set(x + w, y + h);
    po->points[3].set(x + w, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap10(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    if (w > h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x, y);
    po->points[1].set(x, y + h);
    po->points[2].set(x + w, y + h);
    po->points[3].set(x + w, y + w);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap11(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    if (w > h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x, y + w);
    po->points[1].set(x, y + h);
    po->points[2].set(x + w, y + h);
    po->points[3].set(x + w, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap12(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    if (2*w > h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x, y);
    po->points[1].set(x, y + h);
    po->points[2].set(x + w, y + h - w);
    po->points[3].set(x + w, y + w);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap13(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    if (2*w > h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x, y + w);
    po->points[1].set(x, y + h - w);
    po->points[2].set(x + w, y + h);
    po->points[3].set(x + w, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap14(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    // spec says 2*w > h should be fatal error - seems wrong
    if (w > h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x, y);
    po->points[1].set(x, y + h - w);
    po->points[2].set(x + w, y + h);
    po->points[3].set(x + w, y + w);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap15(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    // spec says 2*w > h should be fatal error - seems wrong
    if (w > h)
        return (false);
    po->points = new Point[5];
    po->points[0].set(x, y + w);
    po->points[1].set(x, y + h);
    po->points[2].set(x + w, y + h - w);
    po->points[3].set(x + w, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap16(Poly *po, int x, int y, unsigned int w, unsigned int)
{
    po->points = new Point[4];
    po->points[0].set(x, y);
    po->points[1].set(x, y + w);
    po->points[2].set(x + w, y);
    po->points[3] = po->points[0];
    po->numpts = 4;
    return (true);
}


bool
oas_in::ctrap17(Poly *po, int x, int y, unsigned int w, unsigned int)
{
    po->points = new Point[4];
    po->points[0].set(x, y);
    po->points[1].set(x, y + w);
    po->points[2].set(x + w, y + w);
    po->points[3] = po->points[0];
    po->numpts = 4;
    return (true);
}


bool
oas_in::ctrap18(Poly *po, int x, int y, unsigned int w, unsigned int)
{
    po->points = new Point[4];
    po->points[0].set(x, y);
    po->points[1].set(x + w, y + w);
    po->points[2].set(x + w, y);
    po->points[3] = po->points[0];
    po->numpts = 4;
    return (true);
}


bool
oas_in::ctrap19(Poly *po, int x, int y, unsigned int w, unsigned int)
{
    po->points = new Point[4];
    po->points[0].set(x, y + w);
    po->points[1].set(x + w, y + w);
    po->points[2].set(x + w, y);
    po->points[3] = po->points[0];
    po->numpts = 4;
    return (true);
}


bool
oas_in::ctrap20(Poly *po, int x, int y, unsigned int, unsigned int h)
{
    po->points = new Point[4];
    po->points[0].set(x, y);
    po->points[1].set(x + h, y + h);
    po->points[2].set(x + 2*h, y);
    po->points[3] = po->points[0];
    po->numpts = 4;
    return (true);
}


bool
oas_in::ctrap21(Poly *po, int x, int y, unsigned int, unsigned int h)
{
    po->points = new Point[4];
    po->points[0].set(x, y + h);
    po->points[1].set(x + 2*h, y + h);
    po->points[2].set(x + h, y);
    po->points[3] = po->points[0];
    po->numpts = 4;
    return (true);
}


bool
oas_in::ctrap22(Poly *po, int x, int y, unsigned int w, unsigned int)
{
    po->points = new Point[4];
    po->points[0].set(x, y);
    po->points[1].set(x, y + 2*w);
    po->points[2].set(x + w, y + w);
    po->points[3] = po->points[0];
    po->numpts = 4;
    return (true);
}


bool
oas_in::ctrap23(Poly *po, int x, int y, unsigned int w, unsigned int)
{
    po->points = new Point[4];
    po->points[0].set(x, y + w);
    po->points[1].set(x + w, y + 2*w);
    po->points[2].set(x + w, y);
    po->points[3] = po->points[0];
    po->numpts = 4;
    return (true);
}


bool
oas_in::ctrap24(Poly *po, int x, int y, unsigned int w, unsigned int h)
{
    po->points = new Point[5];
    po->points[0].set(x, y);
    po->points[1].set(x, y + h);
    po->points[2].set(x + w, y + h);
    po->points[3].set(x + w, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


bool
oas_in::ctrap25(Poly *po, int x, int y, unsigned int w, unsigned int)
{
    po->points = new Point[5];
    po->points[0].set(x, y);
    po->points[1].set(x, y + w);
    po->points[2].set(x + w, y + w);
    po->points[3].set(x + w, y);
    po->points[4] = po->points[0];
    po->numpts = 5;
    return (true);
}


// Generate a warning log message, format is
//  string [cellname offset]
//
void
oas_in::warning(const char *str)
{
    FIO()->ifPrintCvLog(IFLOG_WARN, "%s [%s %llu]",
        str, in_cellname, (unsigned long long)in_offset);
}


// Generate a warning log message, format is
//  string [cellname x,y layername offset]
//
void
oas_in::warning(const char *str, int x, int y, unsigned int l, unsigned int d)
{
    char tbf[64];
    if (l < 256 && d < 256)
        sprintf(tbf, "%02X%02X", l, d);
    else
        sprintf(tbf, "%04X%04X", l, d);

    FIO()->ifPrintCvLog(IFLOG_WARN, "%s [%s %d,%d %s %llu]", str,
        in_cellname, x, y, tbf, (unsigned long long)in_offset);
}


// Generate a warning log message, format is
//  string for instance of mstr [cellname x,y offset]
//
void
oas_in::warning(const char *str, const char *mstr, int x, int y)
{
    FIO()->ifPrintCvLog(IFLOG_WARN,
        "%s for instance of %s [%s %d,%d %llu]",
        str, mstr, in_cellname, x, y, (unsigned long long)in_offset);
}


#define MODAL_QUERY(x) (modal.x##_set ? modal.x : (in_nogo = true, modal.x))
#define MODAL_ASSIGN(x, y) modal.x = y, modal.x##_set = true

//---------------------------------------------------------------------------
// Initialization and Control Functions
//---------------------------------------------------------------------------

void
oas_in::push_state(oas_state *st)
{
    unsigned int sz = sizeof(oas_modal);
    st->modal_store = new char[sz];
    memcpy(st->modal_store, &modal, sz);
    modal.reset(false);

    st->zfile = in_zfile;

    st->last_pos = in_byte_offset;
    if (in_peeked)
        st->last_pos--;
    st->last_offset = in_offset;
    st->next_offset = in_next_offset;
    st->comp_end = in_compression_end;
    in_zfile = 0;
    in_next_offset = 0;
    in_compression_end = 0;
}


void
oas_in::pop_state(oas_state *st)
{
    modal.reset();
    unsigned int sz = sizeof(oas_modal);
    memcpy(&modal, st->modal_store, sz);
    delete [] (char*)st->modal_store;

    in_zfile = st->zfile;

    if (in_fp)
        in_fp->z_seek(st->last_pos, SEEK_SET);
    in_byte_offset = st->last_pos;
    in_peeked = false;
    in_offset = st->last_offset;
    in_next_offset = st->next_offset;
    in_compression_end = st->comp_end;
}


//---------------------------------------------------------------------------
// Functions to Read Basic Data Types
//---------------------------------------------------------------------------

// Read and return one byte from the oasis input source.
//
unsigned char
oas_in::read_byte()
{
    if (in_peeked) {
        in_peeked = false;
        return (in_peek_byte);
    }

    if (in_byte_stream) {
        if (in_byte_stream->done()) {
            in_nogo = true;
            return (0);
        }
        unsigned char c = in_byte_stream->get_byte();
        if (in_byte_stream->error()) {
            in_nogo = true;
            return (0);
        }
        in_byte_offset++;
        in_bytes_read++;
        return (c);
    }

    if (!in_fp) {
        in_nogo = true;
        Errs()->add_error("read_byte: null file pointer.");
        return (0);
    }
    int c;
    if (in_zfile) {
        if (in_byte_offset >= in_compression_end) {
            delete in_zfile;
            in_zfile = 0;
            in_fp->z_seek(in_next_offset, SEEK_SET);
            in_byte_offset = in_next_offset;
            c = in_fp->z_getc();
            if (in_action == cvOpenModePrint) {
                print_recname(35);  // "END_CBLOCK"
                put_char('\n');
            }
        }
        else
            c = in_zfile->zio_getc();
    }
    else
        c = in_fp->z_getc();
    if (c == EOF) {
        in_nogo = true;
        Errs()->add_error("Premature EOF.");
        return (0);
    }
    in_byte_offset++;
    in_bytes_read++;

    if (in_bytes_read > in_fb_incr) {
        show_feedback();
        if (in_interrupted) {
            Errs()->add_error("user interrupt");
            in_nogo = true;
            return (0);
        }
    }
    return (c);
}


// Read and return an unsigned integer from the oasis input file.
//
unsigned int
oas_in::read_unsigned()
{
    unsigned int b = read_byte();
    if (in_nogo)
        return (0);
    unsigned int i = (b & 0x7f);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << 7);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << 14);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << 21);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0xf) << 28); // 4 bits left

    if (b & 0xf0) {
        // Handle the case where there are 0x80 bytes followed by 0. 
        // This is cruft that can be ignored.
        if (b & 0x80) {
            do {
                b = read_byte();
                if (in_nogo)
                    return (0);
            } while (b == 0x80);
        }
        if (b) {
            Errs()->add_error("read_unsigned: int32 overflow.");
            in_nogo = true;
            return (0);
        }
    }
    return (i);
}

uint64_t
oas_in::read_unsigned64()
{
    uint64_t b = read_byte();
    if (in_nogo)
        return (0);
    uint64_t i = (b & 0x7f);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << 7);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << 14);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << 21);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << 28);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << 35);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << 42);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << 49);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << 56);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << 63); // 1 bit left

    if (b & 0xfe) {
        // Handle the case where there are 0x80 bytes followed by 0. 
        // This is cruft that can be ignored.

        if (b & 0x80) {
            do {
                b = read_byte();
                if (in_nogo)
                    return (0);
            } while (b == 0x80);
        }
        if (b) {
            Errs()->add_error("read_unsigned64: uint64 overflow.");
            in_nogo = true;
            return (0);
        }
    }
    return (i);
}


// Read and return a signed integer from the oasis input file.
//
int
oas_in::read_signed()
{
    unsigned int b = read_byte();
    if (in_nogo)
        return (0);
    bool neg = b & 1;
    unsigned int i = (b & 0x7f) >> 1;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= (b & 0x7f) << 6;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= (b & 0x7f) << 13;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= (b & 0x7f) << 20;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= (b & 0xf) << 27; // 4 bits left, excluding sign

    if (b & 0xf0) {
        // Handle the case where there are 0x80 bytes followed by 0. 
        // This is cruft that can be ignored.
        if (b & 0x80) {
            do {
                b = read_byte();
                if (in_nogo)
                    return (0);
            } while (b == 0x80);
        }
        if (b) {
            Errs()->add_error("read_signed: int32 overflow.");
            in_nogo = true;
            return (0);
        }
    }
    return (neg ? -i : i);
}


// Read and return a signed 64-bit integer from the oasis input file.
//
int64_t
oas_in::read_signed64()
{
    uint64_t b = read_byte();
    if (in_nogo)
        return (0);
    bool neg = b & 1;
    uint64_t i = (b & 0x7f) >> 1;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= (b & 0x7f) << 6;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= (b & 0x7f) << 13;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= (b & 0x7f) << 20;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= (b & 0xf) << 27;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= (b & 0xf) << 34;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= (b & 0xf) << 41;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= (b & 0xf) << 48;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= (b & 0xf) << 55;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= (b & 0x1) << 62; // 1 bit left, excluding sign
    if (b & 0xfe) {
        // Handle the case where there are 0x80 bytes followed by 0. 
        // This is cruft that can be ignored.
        if (b & 0x80) {
            do {
                b = read_byte();
                if (in_nogo)
                    return (0);
            } while (b == 0x80);
        }
        if (b) {
            Errs()->add_error("read_signed: int64 overflow");
            in_nogo = true;
            return (0);
        }
    }
    return (neg ? -i : i);
}


// Read and return a real number from the oasis input file.
//
double
oas_in::read_real()
{
    unsigned type = read_record_index(7);
    if (in_nogo) {
        Errs()->add_error("read_real: bad index.");
        return (0.0);
    }
    double d = read_real_data(type);
    if (in_nogo) {
        Errs()->add_error("read_real: bad data.");
        return (0.0);
    }
    return (d);
}


// Read and return a real number from the oasis input file.  The data
// type index has already been read, and is passed as the argument.
//
double
oas_in::read_real_data(unsigned int type)
{
    if (type == 0) {
        // positive whole number
        double ret = read_unsigned();
        return (ret);
    }
    if (type == 1) {
        // negative whole number
        double ret = read_unsigned();
        return (-ret);
    }
    if (type == 2) {
        // positive reciprocal
        double ret = read_unsigned();
        if (ret == 0.0) {
            if (in_nogo)
                Errs()->add_error(
                    "read_real_data: reciprocal data 0.0");
            in_nogo = true;
            return (0.0);
        }
        return (1.0/ret);
    }
    if (type == 3) {
        // negative reciprocal
        double ret = read_unsigned();
        if (ret == 0.0) {
            if (in_nogo)
                Errs()->add_error(
                    "read_real_data: reciprocal data 0.0");
            in_nogo = true;
            return (0.0);
        }
        return (-1.0/ret);
    }
    if (type == 4) {
        // positive ratio
        double a = read_unsigned();
        if (in_nogo)
            return (0.0);
        double b = read_unsigned();
        if (b == 0.0) {
            if (in_nogo)
                Errs()->add_error(
                    "read_real_data: reciprocal data 0.0");
            in_nogo = true;
            return (0.0);
        }
        return (a/b);
    }
    if (type == 5) {
        // negative ratio
        double a = read_unsigned();
        if (in_nogo)
            return (0.0);
        double b = read_unsigned();
        if (b == 0.0) {
            if (in_nogo)
                Errs()->add_error(
                    "read_real_data: reciprocal data 0.0");
            in_nogo = true;
            return (0.0);
        }
        return (-a/b);
    }
    if (type == 6) {
        // ieee float
        // first byte is lsb of mantissa
        union { float n; unsigned char b[sizeof(float)]; } u;
        u.n = 1.0;
        if (u.b[0] == 0) {
            // machine is little-endian, read bytes in order
            for (int i = 0; i < (int)sizeof(float); i++) {
                u.b[i] = read_byte();
                if (in_nogo)
                    return (0.0);
            }
        }
        else {
            // machine is big-endian, read bytes in reverse order
            for (int i = sizeof(float) - 1; i >= 0; i--) {
                u.b[i] = read_byte();
                if (in_nogo)
                    return (0.0);
            }
        }
#ifndef WIN32
        if (isnan(u.n)) {
            Errs()->add_error("read_real_data: bad value (nan).");
            in_nogo = true;
            return (0.0);
        }
#endif
        return (u.n);

    }
    if (type == 7) {
        // ieee double
        // first byte is lsb of mantissa
        union { double n; unsigned char b[sizeof(double)]; } u;
        u.n = 1.0;
        if (u.b[0] == 0) {
            // machine is little-endian, read bytes in order
            for (int i = 0; i < (int)sizeof(double); i++) {
                u.b[i] = read_byte();
                if (in_nogo)
                    return (0.0);
            }
        }
        else {
            // machine is big-endian, read bytes in reverse order
            for (int i = sizeof(double) - 1; i >= 0; i--) {
                u.b[i] = read_byte();
                if (in_nogo)
                    return (0.0);
            }
        }
#ifndef WIN32
        if (isnan(u.n)) {
            Errs()->add_error("read_real_data: bad value (nan).");
            in_nogo = true;
            return (0.0);
        }
#endif
        return (u.n);
    }
    Errs()->add_error("read_real_data: datatype error.");
    in_nogo = true;
    return (0.0);
}


// Read and return an N-delta from the oasis input file.  The first
// argument is the N value.  The residual bits are returned in the
// second argument.
//
unsigned int
oas_in::read_delta(int bits, int *bits_ret)
{
    unsigned char b = read_byte();
    if (in_nogo) {
        *bits_ret = 0;
        return (0);
    }
    unsigned int ret = read_delta_bytes(b, bits, bits_ret);
    if (in_nogo) {
        *bits_ret = 0;
        return (0);
    }
    return (ret);
}


// Read and return an N-delta from the oasis input file.  The first
// byte has already been read, and is passed in the first arguemnt.
// The second argument is the N value.  The residual bits are returned
// in the third argument.
//
unsigned int
oas_in::read_delta_bytes(unsigned char bc, int bits, int *bits_ret)
{
    unsigned int b = bc;
    unsigned char mask = 0xff;
    mask >>= (8 - bits);
    *bits_ret = b & mask;
    unsigned int i = (b & 0x7f) >> bits;
    int s = 7 - bits;
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << s);
    s += 7;
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << s);
    s += 7;
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << s);
    s += 7;
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (in_nogo)
        return (0);
    i |= ((b & 0x7f) << s);

    unsigned int x = (b & 0x7f) >> (32 - s);
    if (x) {
        Errs()->add_error("read_delta_bytes: integer overflow.");
        in_nogo = true;
        return (0);
    }

    if (b & 0x80) {
        // Handle the case where there are 0x80 bytes followed by 0. 
        // This is cruft that can be ignored.
        do {
            b = read_byte();
            if (in_nogo)
                return (0);
        } while (b == 0x80);
        if (b) {
            Errs()->add_error("read_delta_bytes: int32 overflow.");
            in_nogo = true;
            return (0);
        }
    }
    return (i);
}


// Read and return a g-delta from the oasis input file.  The x/y
// values are returned in the arguments.
//
bool
oas_in::read_g_delta(int *mx, int *my)
{
    unsigned char b = read_byte();
    if (in_nogo)
        return (false);
    if (b & 1) {
        // type 2
        int xbits;
        unsigned int xmag = read_delta_bytes(b, 2, &xbits);
        if (in_nogo)
            return (false);
        *mx = (xbits & 2) ? -xmag : xmag;
        *my = read_signed();
        if (in_nogo)
            return (false);
    }
    else {
        // type 1
        int bits;
        unsigned int mag = read_delta_bytes(b, 4, &bits);
        if (in_nogo)
            return (false);
        if (bits & 8) {
            if (bits & 4) {
                if (bits & 2) {
                    // southeast
                    *mx = mag;
                    *my = -mag;
                }
                else {
                    // southwest
                    *mx = -mag;
                    *my = -mag;
                }
            }
            else {
                if (bits & 2) {
                    // northwest
                    *mx = -mag;
                    *my = mag;
                }
                else {
                    // northeast
                    *mx = mag;
                    *my = mag;
                }
            }
        }
        else {
            if (bits & 4) {
                if (bits & 2) {
                    // south
                    *mx = 0;
                    *my = -mag;
                }
                else {
                    // west
                    *mx = -mag;
                    *my = 0;
                }
            }
            else {
                if (bits & 2) {
                    // north
                    *mx = 0;
                    *my = mag;
                }
                else {
                    // east
                    *mx = mag;
                    *my = 0;
                }
            }
        }
    }
    return (true);
}


// Read a repetition record from the oasis input file.  This updates
// the current (modal) repetition specification.
//
bool
oas_in::read_repetition()
{
    unsigned int type = read_record_index(11);
    if (in_nogo)
        return (false);

    if (in_skip_all) {
        if (type == 0)
            return (true);
        if (type == 1) {
            read_unsigned();
            read_unsigned();
            read_unsigned();
            read_unsigned();
            if (in_nogo)
                return (false);
            return (true);
        }
        if (type == 2 || type == 3) {
            read_unsigned();
            read_unsigned();
            if (in_nogo)
                return (false);
            return (true);
        }
        if (type == 4 || type == 6) {
            unsigned int n = read_unsigned();
            if (in_nogo)
                return (false);
            for (unsigned int i = 0; i <= n; i++) {
                read_unsigned();
                if (in_nogo)
                    return (false);
            }
            return (true);
        }
        if (type == 5 || type == 7) {
            unsigned int n = read_unsigned();
            read_unsigned();
            if (in_nogo)
                return (false);
            for (unsigned int i = 0; i <= n; i++) {
                read_unsigned();
                if (in_nogo)
                    return (false);
            }
            return (true);
        }
        if (type == 8) {
            read_unsigned();
            if (in_nogo)
                return (false);
            read_unsigned();
            if (in_nogo)
                return (false);
            int mx, my;
            if (!read_g_delta(&mx, &my))
                return (false);
            if (!read_g_delta(&mx, &my))
                return (false);
            return (true);
        }
        if (type == 9) {
            read_unsigned();
            if (in_nogo)
                return (false);
            int mx, my;
            if (!read_g_delta(&mx, &my))
                return (false);
            return (true);
        }
        if (type == 10) {
            unsigned int n = read_unsigned();
            if (in_nogo)
                return (false);
            int mx, my;
            for (unsigned int i = 0; i <= n; i++) {
                if (!read_g_delta(&mx, &my))
                    return (false);
            }
            return (true);
        }
        if (type == 11) {
            unsigned int n = read_unsigned();
            if (in_nogo)
                return (false);
            read_unsigned();
            if (in_nogo)
                return (false);
            int mx, my;
            for (unsigned int i = 0; i <= n; i++) {
                if (!read_g_delta(&mx, &my))
                    return (false);
            }
            return (true);
        }
        return (false);
    }

    if (type == 0) {
        // reuse the previous repetition definition
        if (modal.repetition.type < 0) {
            Errs()->add_error(
                "read_repetition: request to reuse undefined state.");
            in_nogo = true;
            return (false);
        }
        return (true);
    }

    modal.repetition.reset(type);
    if (type == 1) {
        // x-dimension y-dimension x-space y-space
        modal.repetition.xdim = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.ydim = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.xspace = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.yspace = read_unsigned();
        if (in_nogo)
            return (false);
        return (true);
    }
    if (type == 2) {
        // x-dimension x-space
        modal.repetition.xdim = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.xspace = read_unsigned();
        if (in_nogo)
            return (false);
        return (true);
    }
    if (type == 3) {
        // y-dimension y-space
        modal.repetition.ydim = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.yspace = read_unsigned();
        if (in_nogo)
            return (false);
        return (true);
    }
    if (type == 4) {
        // x-dimension x-space ... x-space
        unsigned int n = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.xdim = n;
        modal.repetition.array = new int[n+1];
        for (unsigned int i = 0; i <= n; i++) {
            modal.repetition.array[i] = read_unsigned();
            if (in_nogo)
                return (false);
        }
        return (true);
    }
    if (type == 5) {
        // x-dimension grid x-space ... x-space
        unsigned int n = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.xdim = n;
        unsigned int grid = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.array = new int[n+1];
        for (unsigned int i = 0; i <= n; i++) {
            modal.repetition.array[i] = grid*read_unsigned();
            if (in_nogo)
                return (false);
        }
        return (true);
    }
    if (type == 6) {
        // y-dimension y-space ... y-space
        unsigned int n = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.ydim = n;
        modal.repetition.array = new int[n+1];
        for (unsigned int i = 0; i <= n; i++) {
            modal.repetition.array[i] = read_unsigned();
            if (in_nogo)
                return (false);
        }
        return (true);
    }
    if (type == 7) {
        // y-dimension grid y-space ... y-space
        unsigned int n = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.ydim = n;
        unsigned int grid = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.array = new int[n+1];
        for (unsigned int i = 0; i <= n; i++) {
            modal.repetition.array[i] = grid*read_unsigned();
            if (in_nogo)
                return (false);
        }
        return (true);
    }
    if (type == 8) {
        // n-dimension m-dimension n-displacement m-displacement
        modal.repetition.xdim = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.ydim = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.array = new int[4];
        if (!read_g_delta(&modal.repetition.array[0],
                &modal.repetition.array[1]))
            return (false);
        if (!read_g_delta(&modal.repetition.array[2],
                &modal.repetition.array[3]))
            return (false);
        return (true);
    }
    if (type == 9) {
        // dimension displacement
        modal.repetition.xdim = read_unsigned();
        if (in_nogo)
            return (false);
        if (!read_g_delta(&modal.repetition.xspace, &modal.repetition.yspace))
            return (false);
        return (true);
    }
    if (type == 10) {
        // dimension displacement ... displacement
        unsigned int n = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.xdim = n;
        modal.repetition.array = new int[2*n + 2];
        int *u = modal.repetition.array;
        for (unsigned int i = 0; i <= n; i++, u += 2) {
            if (!read_g_delta(u, u+1))
                return (false);
        }
        return (true);
    }
    if (type == 11) {
        // dimension grid displacement ... displacement
        unsigned int n = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.xdim = n;
        unsigned int grid = read_unsigned();
        if (in_nogo)
            return (false);
        modal.repetition.array = new int[2*n + 2];
        int *u = modal.repetition.array;
        for (unsigned int i = 0; i <= n; i++, u += 2) {
            if (!read_g_delta(u, u+1))
                return (false);
            *u *= grid;
            *(u+1) *= grid;
        }
        return (true);
    }
    Errs()->add_error("read_repetition: datatype error.");
    in_nogo = true;
    return (false);
}


// Read a list of coordinates from the oasis input file.  The new
// array is returned, and its size is returned in the argument.
//
Point *
oas_in::read_point_list(unsigned int *len, bool is_polygon)
{
    *len = 0;
    unsigned type = read_record_index(5);
    if (in_nogo)
        return (0);
    if (type > 5) {
        Errs()->add_error("read_point_list: datatype error.");
        in_nogo = true;
        return (0);
    }
    unsigned int vcount = read_unsigned();
    if (in_nogo)
        return (0);
    if (!vcount)
        return (0);
    Point *pts = new Point[type <= 1 && is_polygon ? vcount+1 : vcount];

    if (type == 0) {
        int vtot = 0, htot = 0;
        if (is_polygon && (vcount < 2 || (vcount & 1))) {
            Errs()->add_error(
                "read_point_list: bad poly point list type=%d np=%d.",
                0, vcount);
            delete [] pts;
            in_nogo = true;
            return (0);
        }
        for (unsigned int i = 0; i < vcount; i++) {
            if (i & 1) {
                pts[i].set(0, read_signed());
                vtot += pts[i].y;
            }
            else {
                pts[i].set(read_signed(), 0);
                htot += pts[i].x;
            }
            if (in_nogo) {
                delete [] pts;
                return (0);
            }
        }
        if (is_polygon) {
            pts[vcount].set(-htot, 0);
            vcount++;
        }
    }
    else if (type == 1) {
        int vtot = 0, htot = 0;
        if (is_polygon && (vcount < 2 || (vcount & 1))) {
            Errs()->add_error(
                "read_point_list: bad poly point list type=%d np=%d.",
                0, vcount);
            delete [] pts;
            in_nogo = true;
            return (0);
        }
        for (unsigned int i = 0; i < vcount; i++) {
            if (i & 1) {
                pts[i].set(read_signed(), 0);
                htot += pts[i].x;
            }
            else {
                pts[i].set(0, read_signed());
                vtot += pts[i].y;
            }
            if (in_nogo) {
                delete [] pts;
                return (0);
            }
        }
        if (is_polygon) {
            pts[vcount].set(0, -vtot);
            vcount++;
        }
    }
    else if (type == 2) {
        for (unsigned int i = 0; i < vcount; i++) {
            int bits;
            int mag = read_delta(2, &bits);
            if (in_nogo) {
                delete [] pts;
                return (0);
            }
            if (bits == 0)
                pts[i].set(mag, 0); // east
            else if (bits == 1)
                pts[i].set(0, mag); // north
            else if (bits == 2)
                pts[i].set(-mag, 0); // west
            else
                pts[i].set(0, -mag); // south
        }
    }
    else if (type == 3) {
        for (unsigned int i = 0; i < vcount; i++) {
            int bits;
            int mag = read_delta(3, &bits);
            if (in_nogo) {
                delete [] pts;
                return (0);
            }
            if (bits == 0)
                pts[i].set(mag, 0); // east
            else if (bits == 1)
                pts[i].set(0, mag); // north
            else if (bits == 2)
                pts[i].set(-mag, 0); // west
            else if (bits == 3)
                pts[i].set(0, -mag); // south
            else if (bits == 4)
                pts[i].set(mag, mag); // northeast
            else if (bits == 5)
                pts[i].set(-mag, mag); // northwest
            else if (bits == 6)
                pts[i].set(-mag, -mag); // southwest
            else
                pts[i].set(mag, -mag); // southeast
        }
    }
    else if (type == 4) {
        for (unsigned int i = 0; i < vcount; i++) {
            int mx, my;
            if (!read_g_delta(&mx, &my)) {
                delete [] pts;
                return (0);
            }
            pts[i].set(mx, my);
        }
    }
    else {
        int sx = 0, sy = 0;
        for (unsigned int i = 0; i < vcount; i++) {
            int mx, my;
            if (!read_g_delta(&mx, &my)) {
                delete [] pts;
                return (0);
            }
            sx += mx;
            sy += my;
            pts[i].set(sx, sy);
        }
    }
    *len = vcount;
    return (pts);
}


// Read and return an 'a' string from the oasis input file.
//
// If the file was generted by Xic or derivative, this will replace
// the sequences "\n", "\r", and "\t" with the actual non-printing
// character, which is encoded thusly in the OASIS a-string writer. 
// This is needed to support multi-line text labels.
//
char *
oas_in::read_a_string()
{
    int len = read_unsigned();
    if (in_nogo)
        return (0);
    char *str = new char[len+1];
    char *s = str;
    char lastc = 0;
    while (len--) {
        int c = read_byte();
        if (in_nogo) {
            delete [] str;
            return (0);
        }
        if (c < 0x20 || c > 0x7e) {
            Errs()->add_error(
                "read_a_string: bad character 0x%x in a-string.", c);
            delete [] str;
            in_nogo = true;
            return (0);
        }
        if (lastc == '\\' && in_xic_source) {
            if (c == 'n') {
                s--;
                *s++ = '\n';
                lastc = 0;
                continue;
            }
            if (c == 'r') {
                s--;
                *s++ = '\r';
                lastc = 0;
                continue;
            }
            if (c == 't') {
                s--;
                *s++ = '\t';
                lastc = 0;
                continue;
            }
        }
        *s++ = c;
        lastc = c;
    }
    *s = 0;
    return (str);
}


// Read an 'a' string from the oasis input file, discarding chars.
//
void
oas_in::read_a_string_nr()
{
    int len = read_unsigned();
    if (in_nogo)
        return;
    while (len--) {
        int c = read_byte();
        if (in_nogo)
            return;
        if (c < 0x20 || c > 0x7e) {
            Errs()->add_error(
                "read_a_string_nr: bad character 0x%x in a-string.", c);
            in_nogo = true;
            return;
        }
    }
}


// Read and return a 'b' string from the oasis input file.  The length
// of the string is returned in the argument.
//
char *
oas_in::read_b_string(unsigned int *len_ret)
{
    unsigned int len = read_unsigned();
    if (in_nogo)
        return (0);
    *len_ret = len;
    char *str = new char[len+1];
    char *s = str;
    while (len--) {
        *s++ = read_byte();
        if (in_nogo) {
            delete [] str;
            return (0);
        }
    }
    *s = 0;
    return (str);
}


// Read a 'b' string from the oasis input file, discarding chars.
//
void
oas_in::read_b_string_nr()
{
    unsigned int len = read_unsigned();
    if (in_nogo)
        return;
    while (len--) {
        read_byte();
        if (in_nogo)
            return;
    }
}


// Read and return an 'n' string from the oasis input file.
//
char *
oas_in::read_n_string()
{
    int len = read_unsigned();
    if (len == 0) {
        Errs()->add_error("read_n_string: zero length n-string.");
        in_nogo = true;
        return (0);
    }
    char *str = new char[len+1];
    char *s = str;
    while (len--) {
        int c = read_byte();
        if (in_nogo) {
            delete [] str;
            return (0);
        }
        if (c < 0x21 || c > 0x7e) {
            Errs()->add_error(
                "read_n_string: bad character 0x%x in n-string.", c);
            delete [] str;
            in_nogo = true;
            return (0);
        }
        *s++ = c;
    }
    *s = 0;
    return (str);
}


// Read an 'n' string from the oasis input file, discarding chars.
//
void
oas_in::read_n_string_nr()
{
    int len = read_unsigned();
    if (len == 0) {
        Errs()->add_error("read_n_string_nr: zero length n-string.");
        in_nogo = true;
        return;
    }
    while (len--) {
        int c = read_byte();
        if (in_nogo)
            return;
        if (c < 0x21 || c > 0x7e) {
            Errs()->add_error(
                "read_n_string_nr: bad character 0x%x in n-string.", c);
            in_nogo = true;
            return;
        }
    }
}


// Read a single property value record from the oasis input file,
// filling the passed structure.
//
bool
oas_in::read_property_value(oas_property_value *pv)
{
    if (!pv) {
        unsigned int type = read_unsigned();
        if (in_nogo)
            return (false);
        if (type <= 7)
            read_real_data(type);
        else if (type == 8)
            read_unsigned64();
        else if (type == 9)
            read_signed64();
        else if (type == 10)
            read_a_string_nr();
        else if (type == 11)
            read_b_string_nr();
        else if (type == 12)
            read_n_string_nr();
        else if (type == 13 || type == 14 || type == 15)
            read_unsigned();
        if (in_nogo)
            return (false);
        return (true);
    }

    pv->type = read_record_index(15);
    if (in_nogo)
        return (false);
    if (pv->type <= 7)
        pv->u.rval = read_real_data(pv->type);
    else if (pv->type == 8)
        pv->u.uval = read_unsigned64();
    else if (pv->type == 9)
        pv->u.ival = read_signed64();
    else if (pv->type == 10)
        pv->u.sval = read_a_string();
    else if (pv->type == 11)
        pv->u.sval = read_b_string(&pv->b_length);
    else if (pv->type == 12)
        pv->u.sval = read_n_string();
    else if (pv->type == 13 || pv->type == 14 || pv->type == 15) {
        unsigned int refnum = read_unsigned();
        if (in_nogo)
            return (false);
        oas_elt *te = in_propstring_table->get(refnum);
        if (!te) {
            // Scan ahead if not strict-mode.
            if (!in_propstring_flag) {
                if (!scan_for_names())
                    return (false);
                te = in_propstring_table->get(refnum);
            }
        }
        if (!te) {
            if (in_nogo)
                Errs()->add_error(
                    "read_property_value: propstring reference %d not found.",
                    refnum);
            in_nogo = true;
            return (false);
        }
        pv->u.sval = lstring::copy(te->string());
        if (pv->type == 14)
            pv->b_length = te->length();
    }
    else {
        Errs()->add_error("read_property_value: datatype error.");
        in_nogo = true;
        return (false);
    }
    if (in_nogo)
        return (false);
    return (true);
}


// Read an interval specification as used in a LAYERNAME record from
// the oasis input file.  The passed structure is filled in.
//
bool
oas_in::read_interval(oas_interval *iv)
{
    unsigned int type = read_record_index(4);
    if (in_nogo)
        return (false);
    if (type == 0) {
        iv->minval = 0;
        iv->maxval = (unsigned int)-1;
    }
    else if (type == 1) {
        iv->minval = 0;
        iv->maxval = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else if (type == 2) {
        iv->minval = read_unsigned();
        if (in_nogo)
            return (false);
        iv->maxval = (unsigned int)-1;
    }
    else if (type == 3) {
        iv->minval = read_unsigned();
        if (in_nogo)
            return (false);
        iv->maxval = iv->minval;
    }
    else if (type == 4) {
        iv->minval = read_unsigned();
        if (in_nogo)
            return (false);
        iv->maxval = read_unsigned();
        if (in_nogo)
            return (false);
    }
    return (true);
}


// Read the offset table, which is found in the START or END record of
// the oasis input file.
//
bool
oas_in::read_offset_table()
{
    // If an offset is zero, the corresponding strict-mode flag is set
    // false.  I've seen files with the strict-mode flags set with
    // zero offset, and corresponding properties present.  Any such
    // properties will trigger a fatal error.  I don't know if these
    // files came from a buggy source, or exactly what the story is,
    // but here we will allow this.

    in_cellname_flag = read_unsigned();
    if (in_nogo)
        return (false);
    in_cellname_off = read_unsigned64();
    if (in_nogo)
        return (false);
    if (in_cellname_off == 0)
        in_cellname_flag = false;

    in_textstring_flag = read_unsigned();
    if (in_nogo)
        return (false);
    in_textstring_off = read_unsigned64();
    if (in_nogo)
        return (false);
    if (in_textstring_off == 0)
        in_textstring_flag = false;

    in_propname_flag = read_unsigned();
    if (in_nogo)
        return (false);
    in_propname_off = read_unsigned64();
    if (in_nogo)
        return (false);
    if (in_propname_off == 0)
        in_propname_flag = false;

    in_propstring_flag = read_unsigned();
    if (in_nogo)
        return (false);
    in_propstring_off = read_unsigned64();
    if (in_nogo)
        return (false);
    if (in_propstring_off == 0)
        in_propstring_flag = false;

    in_layername_flag = read_unsigned();
    if (in_nogo)
        return (false);
    in_layername_off = read_unsigned64();
    if (in_nogo)
        return (false);
    if (in_layername_off == 0)
        in_layername_flag = 0;

    in_xname_flag = read_unsigned();
    if (in_nogo)
        return (false);
    in_xname_off = read_unsigned64();
    if (in_nogo)
        return (false);
    if (in_xname_off == 0)
        in_xname_flag = false;

    return (true);
}


// Read the tables and construct the hash tables.
//
bool
oas_in::read_tables()
{
    if (!in_tables_read) {
        // Need to read the property strings/names first, since other tables
        // may have properties that reference these tables.
        if (in_propstring_off) {
            unsigned int t = in_propstring_flag;
            in_propstring_flag = 0;
            bool ret = read_table(in_propstring_off, 9, 10);
            in_propstring_flag = t;
            if (!ret)
                return (false);
            in_propstring_index = 0;
        }
        if (in_propname_off) {
            unsigned int t = in_propname_flag;
            in_propname_flag = 0;
            bool ret = read_table(in_propname_off, 7, 8);
            in_propname_flag = t;
            if (!ret)
                return (false);
            in_propname_index = 0;
        }
        if (in_cellname_off) {
            unsigned int t = in_cellname_flag;
            in_cellname_flag = 0;
            bool ret = read_table(in_cellname_off, 3, 4);
            in_cellname_flag = t;
            if (!ret)
                return (false);
            in_cellname_index = 0;
        }
        if (in_textstring_off) {
            unsigned int t = in_textstring_flag;
            in_textstring_flag = 0;
            bool ret = read_table(in_textstring_off, 5, 6);
            in_textstring_flag = t;
            if (!ret)
                return (false);
            in_textstring_index = 0;
        }
        if (in_layername_off) {
            unsigned int t = in_layername_flag;
            in_layername_flag = 0;
            bool ret = read_table(in_layername_off, 11, 12);
            in_layername_flag = t;
            if (!ret)
                return (false);
            in_layername_index = 0;
        }
        if (in_xname_off) {
            unsigned int t = in_xname_flag;
            in_xname_flag = 0;
            bool ret = read_table(in_xname_off, 30, 31);
            in_xname_flag = t;
            if (!ret)
                return (false);
            in_xname_index = 0;
        }
        in_tables_read = true;
    }
    return (true);
}


// Read the records pointed to by the offset table.
//
bool
oas_in::read_table(uint64_t offset, unsigned int ixmin, unsigned int ixmax)
{
    in_reading_table = true;
    if (!in_fp) {
        Errs()->add_error("read_table: null file pointer.");
        in_nogo = true;
        return (false);
    }
    int64_t posn = in_fp->z_tell();
    if (in_peeked) {
        posn--;
        in_peeked = false;
    }
    // We're called from START or END record, so we are not in a
    // compressed block.
    if (in_fp->z_seek(offset, SEEK_SET) < 0) {
        Errs()->add_error("read_table: seek failed.");
        in_nogo = true;
        return (false);
    }
    in_byte_offset = offset;
    for (;;) {
        unsigned int ix = read_record_index(34);
        if (in_nogo)
            return (false);
        if (ix == 0) {
            // pad;
            continue;
        }
        if ((ix < ixmin || ix > ixmax) && ix != 34)
            // CBLOCK is 34
            break;
        if (!dispatch(ix)) {
            Errs()->add_error(
                "read_table: failed in record type %d.", ix);
            return (false);
        }
    }
    if (in_zfile) {
        // Still in compressed block, have to exit before returning.
        delete in_zfile;
        in_zfile = 0;
    }

    if (in_fp->z_seek(posn, SEEK_SET) < 0) {
        Errs()->add_error("read_table: seek failed.");
        in_nogo = true;
        return (false);
    }
    in_byte_offset = posn;
    in_peeked = false;
    in_reading_table = false;
    return (true);
}


// Read an unsigned integer from the oasis input file, fail if the
// integer is larger than the argument.
//
unsigned int
oas_in::read_record_index(unsigned int max_index)
{
    unsigned int b = read_byte();
    if (in_nogo)
        return (0);
    unsigned int i = (b & 0x7f);

    if (i > max_index) {
        Errs()->add_error("read_record_index: index out of range.");
        in_nogo = true;
        return (0);
    }
    if (b & 0x80) {
        // Handle the case where there are 0x80 bytes followed by 0. 
        // This is cruft that can be ignored.
        do {
            b = read_byte();
            if (in_nogo)
                return (0);
        } while (b == 0x80);
        if (b) {
            Errs()->add_error("read_record_index: index out of range.");
            in_nogo = true;
            return (0);
        }
    }
    return (i);
}


// Within the OASIS spec, it is possible that <name> records that
// aren't in tables appear after references, requiring that the file
// be scanned ahead to satisfy references.  This sucks big-time, but
// this function will do the look-ahead.
//
bool
oas_in::scan_for_names()
{
    if (in_looked_ahead)
        return (true);

    in_scanning = true;
    FIO()->ifPrintCvLog(IFLOG_INFO,
        "Note: unsatisfied string reference at offset %llu, "
        "scanning file for <name> records.", (unsigned long long)in_offset);
    oas_state bak;
    push_state(&bak);

    uint64_t tmp_offset = in_offset;

    if (!in_fp) {
        Errs()->add_error("scan_for_names: null file pointer.");
        in_nogo = true;
        return (false);
    }
    int64_t posn = in_fp->z_tell();

    uint64_t start_offset;
    if (in_reading_table) {
        // When reading a table, we may have jumped over records, so
        // start scanning from the record that follows the START
        // record.
        //
        // Something similar may be needed for random cell access,
        // but is not implemented.

        in_cellname_index = 0;
        in_textstring_index = 0;
        in_propname_index = 0;
        in_propstring_index = 0;
        in_layername_index = 0;
        in_xname_index = 0;

        start_offset = in_scan_start_offset;
    }
    else
        // Otherwise, we have presumably read all of the file to the
        // present point, se we search forward.
        start_offset = in_offset;

    if (in_fp->z_seek(start_offset, SEEK_SET) < 0) {
        Errs()->add_error("scan_for_names: seek failed.");
        in_nogo = true;
        return (false);
    }
    for (;;) {
        unsigned int ix = read_record_index(34);
        if (in_nogo)
            return (false);
        if (ix == 0)
            continue;
        if (ix == 2)
            break;

        in_skip_all = true;
        bool ret = dispatch(ix);
        in_skip_all = false;
        if (!ret) {
            Errs()->add_error(
                "scan_for_names: failed in record type %d.", ix);
            return (false);
        }
    }
    if (in_fp->z_seek(posn, SEEK_SET) < 0) {
        Errs()->add_error("scan_for_names: seek failed.");
        in_nogo = true;
        return (false);
    }
    in_offset = tmp_offset;
    pop_state(&bak);
    in_looked_ahead = true;
    in_scanning = false;
    return (true);
}


//---------------------------------------------------------------------------
// Functions to Read OASIS Records
//---------------------------------------------------------------------------

// Read the START record.  This should immediately follow the magic
// string, as the first record in the oasis file.
//
bool
oas_in::read_start(unsigned int ix)
{
    // '1' version-string unit offset-flag [offset-table]
    if (!in_fp) {
        Errs()->add_error("read_start: null file pointer.");
        in_nogo = true;
        return (false);
    }
    if (ix != 1) {
        Errs()->add_error("read_start: bad index.");
        in_nogo = true;
        return (false);
    }

    char *vers = read_a_string();
    if (in_nogo)
        return (false);
    GCarray<char*> gc_vers(vers);

    if (strcmp(vers, "1.0")) {
        Errs()->add_error("read_start: unknown version.");
        in_nogo = true;
        return (false);
    }
    in_unit = read_real();
    if (in_nogo)
        return (false);
    in_offset_flag = read_record_index(1);
    if (in_nogo)
        return (false);
    if (in_offset_flag == 0) {
        if (!read_offset_table())
            return (false);
        if (in_fp->file) {
            // The input file was gzipped, seeks are expensive.
            // If there are tables, scan the whole file for names.
                
            if (in_propstring_off || in_propname_off || in_cellname_off ||
                    in_textstring_off || in_layername_off || in_xname_off) {
                
                // Don't re-read the start record.  
                in_offset = in_fp->z_tell();
                
                // Turn off strict mode and offsets.
                in_propstring_flag = false;
                in_propstring_off = 0;
                in_propname_flag = false;
                in_propname_off = 0;
                in_cellname_flag = false;
                in_cellname_off = 0;
                in_textstring_flag = false;
                in_textstring_off = 0;
                in_layername_flag = false;
                in_layername_off = 0;
                in_xname_flag = false;
                in_xname_off = 0;

                if (!scan_for_names())
                    return (false);
            }
        }
    }
    else {
        if (in_fp->file) {
            // Input file is gzipped, file seeks are very expensive,
            // so we just scan the whole file for name records.
    
            // Don't re-read start record.
            in_offset = in_fp->z_tell();
  
            // Note that we haven't read the table offsets, so all
            // offsets are zero and strict-mode flags are false.
            if (!scan_for_names())
                return (false);
        }
        else {
            // the offset table is in the END record;
            int64_t posn = in_fp->z_tell();
            if (in_fp->z_seek(-256, SEEK_END) < 0) {
                Errs()->add_error("read_start: seek end failed.");
                in_nogo = true;
                return (false);
            }
            in_byte_offset = in_fp->z_tell();
            unsigned int erec = read_unsigned();
            if (erec != 2) {
                Errs()->add_error("read_start: can't find END.");
                in_nogo = true;
                return (false);
            }
            if (!read_offset_table())
                return (false);
            if (in_fp->z_seek(posn, SEEK_SET) < 0) {
                Errs()->add_error("read_start: seek failed.");
                in_nogo = true;
                return (false);
            }
            in_byte_offset = posn;
            in_peeked = false;
        }
    }

    // When scanning for names, avoid re-reading the start record.
    in_scan_start_offset = in_fp->z_tell();

    // Read and process the table data.
    if (!read_tables())
        return (false);

    // read file properties
    while (peek_property(ix)) ;
    if (in_nogo)
        return (false);

    return (true);
}


// Read the END record, which is the last record in the oasis file.
//
bool
oas_in::read_end(unsigned int ix)
{
    // '2' [table-offsets] padding-string validation-scheme
    //      [validation-signature]
    if (ix != 2) {
        Errs()->add_error("read_end: bad index.");
        in_nogo = true;
        return (false);
    }

    if (in_offset_flag) {
        if (!read_offset_table())
            return (false);
    }

    read_b_string_nr();
    if (in_nogo)
        return (false);

    int valtype = read_record_index(2);
    if (in_nogo)
        return (false);

    if (!in_fp) {
        Errs()->add_error("read_end: null file pointer.");
        in_nogo = true;
        return (false);
    }
    int64_t posn = in_fp->z_tell();  // start of checksum data

    unsigned int file_csum = 0;
    if (valtype) {
        file_csum = read_byte();
        file_csum |= (read_byte() << 8);
        file_csum |= (read_byte() << 16);
        file_csum |= (read_byte() << 24);
        if (in_nogo)
            return (false);
    }
    if (in_skip_all || in_skip_action)
        return (true);

    if (valtype && !FIO()->IsOasReadNoChecksum()) {

        unsigned int crc = 0;
        if (in_fp->z_seek(in_start_offset, SEEK_SET) < 0) {
            Errs()->add_error("read_end: seek for validation failed.");
            return (false);
        }
        int64_t bytes = posn - in_start_offset;
        const int BFSZ = 4096;
        unsigned char buf[BFSZ];
        if (valtype == 1)
            crc = crc32(crc, 0, 0);
        while (bytes) {
            int sz = bytes < BFSZ ? bytes : BFSZ;
            if (in_fp->z_read(buf, 1, sz) != sz) {
                Errs()->add_error(
                    "read_end: read for validation failed.");
                return (false);
            }
            bytes -= sz;
            if (valtype == 1)
                crc = crc32(crc, buf, sz);
            else if (valtype == 2) {
                for (int i = 0; i < sz; i++)
                    crc += buf[i];
            }
        }

        if (file_csum != crc) {
            Errs()->add_error(
                "read_end: checksums differ, in file %x != computed %x.",
                file_csum, crc);
            in_nogo = true;
            return (false);
        }
        if (in_fp->z_seek(posn + 4, SEEK_SET) < 0) {
            Errs()->add_error("read_end: fseek for validation failed.");
            return (false);
        }
    }
    if (!(this->*end_action)()) {
        Errs()->add_error("read_end: action failed.");
        in_nogo = true;
        return (false);
    }
    return (true);
}


// Read a CELLNAME record, which is in the <name> class.
//
bool
oas_in::read_cellname(unsigned int ix)
{
    // '3' cellname-string
    // '4' cellname-string reference-number
    if (ix != 3 && ix != 4) {
        Errs()->add_error("read_cellname: bad index.");
        in_nogo = true;
        return (false);
    }

    modal.reset();
    if (ix != in_cellname_type) {
        if (!in_cellname_type)
            in_cellname_type = ix;
        else {
            Errs()->add_error("read_cellname: types 3 and 4 found.");
            in_nogo = true;
            return (false);
        }
    }

    char *name = read_n_string();
    if (in_nogo)
        return (false);
    GCarray<char*> gc_name(name);

    unsigned int refnum;
    if (ix == 3)                    // cellname-string
        refnum = in_cellname_index++;
    else {                          // cellname-string reference-number
        refnum = read_unsigned();
        if (in_nogo)
            return (false);
    }
    // We may have already read this record as part of a table.
    if (!in_cellname_table)
        in_cellname_table = new oas_table;
    oas_elt *te = in_cellname_table->get(refnum);
    if (te) {
        if (strcmp(name, te->string())) {
            Errs()->add_error(
                "read_cellname: duplicate refnum %d.", refnum);
            in_nogo = true;
            return (false);
        }
    }
    else {
        if (in_cellname_flag) {
            // Table may not have been read yet!
            if (in_scanning) {
                if (ix == 3)
                    in_cellname_index--;
                return (true);
            }

            // in strict mode, shouldn't be here
            Errs()->add_error(
                "read_cellname: strict mode, record not in table.");
            in_nogo = true;
            return (false);
        }
        te = in_cellname_table->add(refnum);
        in_cellname_table = in_cellname_table->check_rehash();
        te->set_string(name);
        gc_name.clear();
    }

    if (!in_skip_action && !in_skip_all) {
        // read properties
        while (peek_property(ix)) ;
        if (in_nogo)
            return (false);
        if (!te->prpty_list())
            te->set_prpty_list(in_prpty_list);
        else
            CDp::destroy(in_prpty_list);
        in_prpty_list = 0;
    }

    return (true);
}


// Read a TEXTSTRING record, which is in the <name> class.
//
bool
oas_in::read_textstring(unsigned int ix)
{
    // '5' text-string
    // '6' text-string reference-number
    if (ix != 5 && ix != 6) {
        Errs()->add_error("read_textstring: bad index.");
        in_nogo = true;
        return (false);
    }

    modal.reset();
    if (ix != in_textstring_type) {
        if (!in_textstring_type)
            in_textstring_type = ix;
        else {
            Errs()->add_error("read_textstring: types 5 and 6 found.");
            in_nogo = true;
            return (false);
        }
    }

    char *text = read_a_string();
    if (in_nogo)
        return (false);
    GCarray<char*> gc_text(text);

    unsigned int refnum;
    if (ix == 5)                    // text-string
        refnum = in_textstring_index++;
    else {                          // text-string reference-number
        refnum = read_unsigned();
        if (in_nogo)
            return (false);
    }
    // We may have already read this record as part of a table.
    if (!in_textstring_table)
        in_textstring_table = new oas_table;
    oas_elt *te = in_textstring_table->get(refnum);
    if (te) {
        if (strcmp(text, te->string())) {
            bool ok = false;
            if (in_xic_source) {
                // If in_xic_src is true, and characters were mapped
                // in read_a_string, the strcmp will fail here.  The
                // table will have the pre-mapped string.

                char *str = new char[strlen(te->string()) + 1];
                char *s = str;
                const char *t = te->string();
                while (*t) {
                    if (*t == '\\') {
                        t++;
                        if (*t == 'n') {
                            t++;
                            *s++ = '\n';
                        }
                        else if (*t == 'r') {
                            t++;
                            *s++ = '\r';
                        }
                        else if (*t == 't') {
                            t++;
                            *s++ = '\t';
                        }
                        else
                            *s++ = '\\';
                    }
                    else
                        *s++ = *t++;
                }
                *s = 0;
                if (!strcmp(text, str)) {
                    delete [] te->string();
                    te->set_string(str, strlen(str));
                    ok = true;
                }
            }
            if (!ok) {
                Errs()->add_error(
                    "read_textstring: duplicate refnum %d.", refnum);
                in_nogo = true;
                return (false);
            }
        }
    }
    else {
        if (in_textstring_flag) {
            // Table may not have been read yet!
            if (in_scanning) {
                if (ix == 5)
                    in_textstring_index--;
                return (true);
            }

            // in strict mode, shouldn't be here
            Errs()->add_error(
                "read_textstring: strict mode, record not in table.");
            in_nogo = true;
            return (false);
        }
        te = in_textstring_table->add(refnum);
        in_textstring_table = in_textstring_table->check_rehash();
        te->set_string(text);
        gc_text.clear();
    }

    if (!in_skip_action && !in_skip_all) {
        // read properties
        while (peek_property(ix)) ;
        if (in_nogo)
            return (false);
        if (!te->prpty_list())
            te->set_prpty_list(in_prpty_list);
        else
            CDp::destroy(in_prpty_list);
        in_prpty_list = 0;
    }

    return (true);
}


// Read a PROPNAME record, which is in the <name> class.
//
bool
oas_in::read_propname(unsigned int ix)
{
    // '7' propname-string
    // '8' propname-string reference-number
    if (ix != 7 && ix != 8) {
        Errs()->add_error("read_propname: bad index.");
        in_nogo = true;
        return (false);
    }

    modal.reset();
    if (ix != in_propname_type) {
        if (!in_propname_type)
            in_propname_type = ix;
        else {
            Errs()->add_error("read_propname: types 7 and 8 found.");
            in_nogo = true;
            return (false);
        }
    }

    char *name = read_n_string();
    if (in_nogo)
        return (false);
    GCarray<char*> gc_name(name);

    unsigned int refnum;
    if (ix == 7)                    // propname-string
        refnum = in_propname_index++;
    else {                          // propname-string reference-number
        refnum = read_unsigned();
        if (in_nogo)
            return (false);
    }
    // We may have already read this record as part of a table.
    if (!in_propname_table)
        in_propname_table = new oas_table;
    oas_elt *te = in_propname_table->get(refnum);
    if (te) {
        if (strcmp(name, te->string())) {
            Errs()->add_error(
                "read_propname: duplicate refnum %d.", refnum);
            in_nogo = true;
            return (false);
        }
    }
    else {
        if (in_propname_flag) {
            // Table may not have been read yet!
            if (in_scanning) {
                if (ix == 7)
                    in_propname_index--;
                return (true);
            }

            // in strict mode, shouldn't be here
            Errs()->add_error(
                "read_propname: strict mode, record not in table.");
            in_nogo = true;
            return (false);
        }
        te = in_propname_table->add(refnum);
        in_propname_table = in_propname_table->check_rehash();
        te->set_string(name);
        gc_name.clear();
    }

    if (!in_skip_action && !in_skip_all) {
        // read properties
        while (peek_property(ix)) ;
        if (in_nogo)
            return (false);
        if (!te->prpty_list())
            te->set_prpty_list(in_prpty_list);
        else
            CDp::destroy(in_prpty_list);
        in_prpty_list = 0;
    }

    return (true);
}


// Read a PROPSTRING record, which is in the <name> class.
//
bool
oas_in::read_propstring(unsigned int ix)
{
    // '9' prop-string
    // '10' prop-string reference-number
    if (ix != 9 && ix != 10) {
        Errs()->add_error("read_propstring: bad index.");
        in_nogo = true;
        return (false);
    }

    modal.reset();
    if (ix != in_propstring_type) {
        if (!in_propstring_type)
            in_propstring_type = ix;
        else {
            Errs()->add_error("read_propstring: types 9 and 10 found.");
            in_nogo = true;
            return (false);
        }
    }

    unsigned int len = 0;
    char *prop = read_b_string(&len);
    if (in_nogo)
        return (false);
    GCarray<char*> gc_prop(prop);

    unsigned int refnum;
    if (ix == 9)                    // prop-string
        refnum = in_propstring_index++;
    else {                          // prop-string reference-number
        refnum = read_unsigned();
        if (in_nogo)
            return (false);
    }
    // We may have already read this record as part of a table.
    if (!in_propstring_table)
        in_propstring_table = new oas_table;
    oas_elt *te = in_propstring_table->get(refnum);
    if (te) {
        if (memcmp(prop, te->string(), len)) {
            Errs()->add_error(
                "read_propstring: duplicate refnum %d.", refnum);
            in_nogo = true;
            return (false);
        }
    }
    else {
        if (in_propstring_flag) {
            // Table may not have been read yet!
            if (in_scanning) {
                if (ix == 9)
                    in_propstring_index--;
                return (true);
            }

            // in strict mode, shouldn't be here
            Errs()->add_error(
                "read_propstring: strict mode, record not in table.");
            in_nogo = true;
            return (false);
        }
        te = in_propstring_table->add(refnum);
        in_propstring_table = in_propstring_table->check_rehash();
        te->set_string(prop, len);
        gc_prop.clear();
    }

    if (!in_skip_action && !in_skip_all) {
        // read properties
        while (peek_property(ix)) ;
        if (in_nogo)
            return (false);
        if (!te->prpty_list())
            te->set_prpty_list(in_prpty_list);
        else
            CDp::destroy(in_prpty_list);
        in_prpty_list = 0;
    }

    return (true);
}


// Read a LAYERNAME record, which is in the <name> class.
//
bool
oas_in::read_layername(unsigned int ix)
{
    // '11' layername-string layer-interval datatype-interval
    // '12' layername-string textlsyer-interval texttype-interval

    // Note: this is different from the other <name>s, there is no
    // integer referencing of the strings.  Both record types can
    // appear in a file.

    if (ix != 11 && ix != 12) {
        Errs()->add_error("read_layername: bad index.");
        in_nogo = true;
        return (false);
    }

    modal.reset();
    oas_layer_map_elem *lm = new oas_layer_map_elem;
    lm->name = read_n_string();
    if (in_nogo) {
        delete lm;
        return (false);
    }
    if (!read_interval(&lm->number_range)) {
        delete lm;
        return (false);
    }
    if (!read_interval(&lm->type_range)) {
        delete lm;
        return (false);
    }
    if (ix == 11) // layername-string layer-interval datatype-interval
        ;
    else          // layername-string textlayer-interval texttype-interval
        lm->is_text = true;

    // We may have already read this record as part of a table.
    unsigned int refnum = in_layername_index++;
    if (!in_layername_table)
        in_layername_table = new oas_table;
    oas_elt *te = in_layername_table->get(refnum);
    if (te) {
        // already read this
        delete lm;
    }
    else {
        if (in_layername_flag) {
            // Table may not have been read yet!
            if (in_scanning) {
                in_layername_index--;
                return (true);
            }

            // in strict mode, shouldn't be here
            Errs()->add_error(
                "read_layername: strict mode, record not in table.");
            delete lm;
            in_nogo = true;
            return (false);
        }
        te = in_layername_table->add(refnum);
        in_layername_table = in_layername_table->check_rehash();
        te->set_string(lm->name);
        te->set_data(lm);
        lm->next = in_layermap_list;
        in_layermap_list = lm;
    }

    if (!in_skip_action && !in_skip_all) {
        // read properties
        while (peek_property(ix)) ;
        if (in_nogo)
            return (false);
        if (!te->prpty_list())
            te->set_prpty_list(in_prpty_list);
        else
            CDp::destroy(in_prpty_list);
        in_prpty_list = 0;
    }

    return (true);
}


// Read a CELL record.  The cell applies to all following records up
// to the next CELL or <name>.
//
bool
oas_in::read_cell(unsigned int ix)
{
    // '13' reference-number
    // '14' cellname-string
    modal.reset();

    char *name = 0;
    if (ix == 13) {
        // reference-number
        unsigned int refnum = read_unsigned();
        if (in_nogo)
            return (false);
        if (in_skip_all)
            return (true);

        // If we have a cell name from in_chd_state, we can get by
        // without the cell table name, except that it may provide
        // properties.
        if (in_chd_state.symref()) {
            name = lstring::copy(
                Tstring(in_chd_state.symref()->get_name()));
        }
        oas_elt *te = in_cellname_table->get(refnum);
        if (!te) {
            // Scan ahead if not strict-mode.
            if (!in_cellname_flag) {
                if (!scan_for_names())
                    return (false);
                te = in_cellname_table->get(refnum);
            }
        }
        if (te) {
            if (!name)
                name = lstring::copy(te->string());
            in_prpty_list = te->prpty_list();
            te->set_prpty_list(0);
        }
        else if (!name) {
            Errs()->add_error(
                "read_cell: reference %d to undefined cellname.", refnum);
            in_nogo = true;
            return (false);
        }
    }
    else if (ix == 14) {
        // cellname-string
        name = read_n_string();
        if (in_nogo)
            return (false);
        if (in_skip_all) {
            delete [] name;
            return (true);
        }
        if (in_chd_state.symref()) {
            delete [] name;
            name = lstring::copy(Tstring(in_chd_state.symref()->get_name()));
        }
    }
    else {
        Errs()->add_error("read_cell: bad index.");
        in_nogo = true;
        return (false);
    }

    if (!in_skip_action) {
        // read properties
        while (peek_property(ix)) ;
        if (!in_nogo) {
            FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0,
                in_mode);
            if (!(this->*cell_action)(name)) {
                Errs()->add_error("read_cell: action failed.");
                in_nogo = true;
            }
        }
        (this->*clear_properties_action)();
    }
    delete [] name;
    return (!in_nogo);
}


// Respond to an XYABSOLUTE record.
//
bool
oas_in::xy_absolute(unsigned int ix)
{
    // '15'
    if (ix != 15) {
        Errs()->add_error("read_absolute: bad index.");
        in_nogo = true;
        return (false);
    }
    if (!in_skip_all)
        modal.xy_mode = XYabsolute;
    return (true);
}


// Respond to an XYRELATIVE record.
//
bool
oas_in::xy_relative(unsigned int ix)
{
    // '16'
    if (ix != 16) {
        Errs()->add_error("read_relative: bad index.");
        in_nogo = true;
        return (false);
    }
    if (!in_skip_all)
        modal.xy_mode = XYrelative;
    return (true);
}


// Read a PLACEMENT record.
//
bool
oas_in::read_placement(unsigned int ix)
{
    // '17' placement-info-byte [reference-number | cellname-string]
    //      [x][y][repetition]
    // '18' placement-info-byte [reference-number | cellname-string]
    //      [magnification] [angle] [x] [y] [repetition]
    if (ix != 17 && ix != 18) {
        Errs()->add_error("read_placement: bad index.");
        in_nogo = true;
        return (false);
    }

    unsigned int info_byte = read_byte();  // CNXYRAAF or CNXYRMAF
    if (in_nogo)
        return (false);

    if (in_skip_all) {
        if (info_byte & 0x80) {
            if (info_byte & 0x40) {
                read_unsigned();
                if (in_nogo)
                    return (false);
            }
            else {
                read_n_string_nr();
                if (in_nogo)
                    return (false);
            }
        }
        if (ix == 18) {
            if (info_byte & 0x4)
                read_real();
            if (info_byte & 0x2)
                read_real();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x20) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x10) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x8) {
            if (!read_repetition())
                return (false);
        }
        return (true);
    }

    char *name = 0;
    if (info_byte & 0x80) {
        if (info_byte & 0x40) {
            unsigned int refnum = read_unsigned();
            if (in_nogo)
                return (false);
            oas_elt *te = in_cellname_table->get(refnum);
            if (!te) {
                // Scan ahead if not strict-mode.
                if (!in_cellname_flag) {
                    if (!scan_for_names())
                        return (false);
                    te = in_cellname_table->get(refnum);
                }
            }
            if (!te) {
                if (in_nogo)
                    Errs()->add_error(
                        "read_placement: name reference %d not found.",
                        refnum);
                in_nogo = true;
                return (false);
            }
            name = uobj.placement.name = lstring::copy(te->string());
            for (CDp *p = te->prpty_list(); p; p = p->next_prp()) {
                CDp *px = new CDp(*p);
                px->set_next_prp(in_prpty_list);
                in_prpty_list = px;
            }
        }
        else {
            name = uobj.placement.name = read_n_string();
            if (in_nogo)
                return (false);
        }
    }
    else {
        uobj.placement.name = MODAL_QUERY(placement_cell);
        if (in_nogo) {
            Errs()->add_error(
                "read_placement: modal placement_cell unset.");
            return (false);
        }
    }
    GCarray<char*> gc_name(name);

    if (ix == 17) {
        uobj.placement.magnification = 1.0;
        int aa = (info_byte & 0x6) >> 1;
        uobj.placement.angle = aa*90.0;
    }
    else {
        uobj.placement.magnification = (info_byte & 0x4) ? read_real() : 1.0;
        uobj.placement.angle = (info_byte & 0x2) ? read_real() : 0.0;
    }

    // [x]
    if (info_byte & 0x20) {
        uobj.placement.x = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.placement.x += modal.placement_x;
    }
    else
        uobj.placement.x = modal.placement_x;

    // [y]
    if (info_byte & 0x10) {
        uobj.placement.y = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.placement.y += modal.placement_y;
    }
    else
        uobj.placement.y = modal.placement_y;

    if (info_byte & 0x8) {
        if (!read_repetition())
            return (false);
        uobj.placement.repetition = &modal.repetition;
    }
    else
        uobj.placement.repetition = 0;
    uobj.placement.flip_y = (info_byte & 0x1);

    if (modal.placement_cell != uobj.placement.name) {
        delete [] modal.placement_cell;
        modal.placement_cell = uobj.placement.name;
        modal.placement_cell_set = true;
    }
    modal.placement_x = uobj.placement.x;
    modal.placement_y = uobj.placement.y;
    gc_name.clear();

    if (!in_skip_action) {
        while (peek_property(ix)) ;
        if (!in_nogo) {
            if (in_incremental) {
                int dx, dy;
                unsigned int nx, ny;
                if (placement_array_params(&dx, &dy, &nx, &ny))
                    in_rgen.set_array(nx, ny, dx, dy);
                else
                    in_rgen.init(uobj.placement.repetition);
            }
            else {
                FIO()->ScalePrptyStrings(in_prpty_list, in_phys_scale, 1.0,
                    in_mode);
                int dx, dy;
                unsigned int nx, ny;
                if (placement_array_params(&dx, &dy, &nx, &ny)) {
                    if (!(this->*placement_action)(dx, dy, nx, ny)) {
                        Errs()->add_error("read_placement: action failed.");
                        in_nogo = true;
                    }
                }
                else {
                    in_rgen.init(uobj.placement.repetition);
                    int xoff, yoff;
                    while (in_rgen.next(&xoff, &yoff)) {
                        uobj.placement.x = modal.placement_x + xoff;
                        uobj.placement.y = modal.placement_y + yoff;
                        if (!(this->*placement_action)(0, 0, 1, 1)) {
                            Errs()->add_error(
                                "read_placement: action failed.");
                            in_nogo = true;
                            break;
                        }
                    }
                }
            }
        }
        (this->*clear_properties_action)();
    }

    in_state = in_nogo ? oasError : oasHasPlacement;
    return (!in_nogo);
}


// Read a TEXT record.
//
bool
oas_in::read_text(unsigned int ix)
{
    // '19' text-info-byte [reference-number | text-string]
    //      [textlayer-number] [texttype-number] [x] [y] [repetition]
    if (ix != 19) {
        Errs()->add_error("read_text: bad index.");
        in_nogo = true;
        return (false);
    }

    unsigned char info_byte = read_byte();  // 0CNXYRTL
    if (in_nogo)
        return (false);

    if (in_skip_all) {
        if (info_byte & 0x40) {
            if (info_byte & 0x20) {
                read_unsigned();
                if (in_nogo)
                    return (false);
            }
            else {
                read_a_string_nr();
                if (in_nogo)
                    return (false);
            }
        }
        if (info_byte & 0x1) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x2) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x10) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x8) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x4) {
            if (!read_repetition())
                return (false);
        }
        return (true);
    }

    // [reference-number | text-string]
    char *text = 0;
    if (info_byte & 0x40) {
        if (info_byte & 0x20) {
            unsigned int refnum = read_unsigned();
            if (in_nogo)
                return (false);
            oas_elt *te = in_textstring_table->get(refnum);
            if (!te) {
                // Scan ahead if not strict-mode.
                if (!in_textstring_flag) {
                    if (!scan_for_names())
                        return (false);
                    te = in_textstring_table->get(refnum);
                }
            }
            if (!te) {
                if (in_nogo)
                    Errs()->add_error(
                        "read_text: string reference %d not found.",
                        refnum);
                in_nogo = true;
                return (false);
            }
            text = uobj.text.string = lstring::copy(te->string());
            for (CDp *p = te->prpty_list(); p; p = p->next_prp()) {
                CDp *px = new CDp(*p);
                px->set_next_prp(in_prpty_list);
                in_prpty_list = px;
            }
        }
        else {
            text = uobj.text.string = read_a_string();
            if (in_nogo)
                return (false);
        }
    }
    else {
        uobj.text.string = MODAL_QUERY(text_string);
        if (in_nogo) {
            Errs()->add_error("read_text: modal text_string unset.");
            return (false);
        }
    }
    GCarray<char*> gc_text(text);

    // [textlayer]
    uobj.text.textlayer =
        (info_byte & 0x1) ? read_unsigned() : MODAL_QUERY(textlayer);
    if (in_nogo) {
        if (!(info_byte & 0x1))
            Errs()->add_error("read_text: modal textlayer unset.");
        return (false);
    }

    // [texttype]
    uobj.text.texttype =
        (info_byte & 0x2) ? read_unsigned() : MODAL_QUERY(texttype);
    if (in_nogo) {
        if (!(info_byte & 0x2))
            Errs()->add_error("read_text: modal texttype unset.");
        return (false);
    }

    // [x]
    if (info_byte & 0x10) {
        uobj.text.x = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.text.x += modal.text_x;
    }
    else
        uobj.text.x = modal.text_x;

    // [y]
    if (info_byte & 0x8) {
        uobj.text.y = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.text.y += modal.text_y;
    }
    else
        uobj.text.y = modal.text_y;

    // [repetition]
    if (info_byte & 0x4) {
        if (!read_repetition())
            return (false);
        uobj.text.repetition = &modal.repetition;
    }
    else
        uobj.text.repetition = 0;

    // set modal variables
    MODAL_ASSIGN(textlayer, uobj.text.textlayer);
    MODAL_ASSIGN(texttype, uobj.text.texttype);
    modal.text_x = uobj.text.x;
    modal.text_y = uobj.text.y;
    if (modal.text_string != uobj.text.string) {
        delete [] modal.text_string;
        modal.text_string = uobj.text.string;
        modal.text_string_set = true;
    }
    gc_text.clear();

    uobj.text.width = 0;
    uobj.text.xform = 0;

    if (!in_skip_action) {
        while (peek_property(ix)) ;
        if (!in_nogo && (!in_listonly || in_savebb)) {
            in_rgen.init(uobj.text.repetition);
            if (!in_incremental) {
                int xoff, yoff;
                while (in_rgen.next(&xoff, &yoff)) {
                    uobj.text.x = modal.text_x + xoff;
                    uobj.text.y = modal.text_y + yoff;
                    if (!(this->*text_action)()) {
                        Errs()->add_error("read_text: action failed.");
                        in_nogo = true;
                        break;
                    }
                }
            }
        }
        (this->*clear_properties_action)();
    }

    in_state = in_nogo ? oasError : oasHasText;
    return (!in_nogo);
}


// Read a RECTANGLE record, which is in the <geometry> class.
//
bool
oas_in::read_rectangle(unsigned int ix)
{
    // '20' rectangle-info-byte [layer-number] [datatype-number]
    //      [width] [height] [x] [y] [repetition]
    if (ix != 20) {
        Errs()->add_error("read_rectangle: bad index.");
        in_nogo = true;
        return (false);
    }

    unsigned char info_byte = read_byte();  // SWHXYRDL
    if (in_nogo)
        return (false);

    if (in_skip_all) {
        if (info_byte & 0x1) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x2) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x40) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x20) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x10) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x8) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x4) {
            if (!read_repetition())
                return (false);
        }
        return (true);
    }

    // [layer-number]
    if (info_byte & 0x1) {
        uobj.rectangle.layer = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.rectangle.layer = MODAL_QUERY(layer);
        if (in_nogo) {
            Errs()->add_error("read_rectangle: modal layer unset.");
            return (false);
        }
    }

    // [datatype-number]
    if (info_byte & 0x2) {
        uobj.rectangle.datatype = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.rectangle.datatype = MODAL_QUERY(datatype);
        if (in_nogo) {
            Errs()->add_error("read_rectangle: modal datatype unset.");
            return (false);
        }
    }

    // [width]
    if (info_byte & 0x40) {
        uobj.rectangle.width = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.rectangle.width = MODAL_QUERY(geometry_w);
        if (in_nogo) {
            Errs()->add_error("read_rectangle: modal geometry_w unset.");
            return (false);
        }
    }

    // [height]
    if (info_byte & 0x80) {
        if (info_byte & 0x20) {
            Errs()->add_error("read_rectangle: S and H both set.");
            in_nogo = true;
            return (false);
        }
        uobj.rectangle.height = uobj.rectangle.width;
    }
    else {
        if (info_byte & 0x20) {
            uobj.rectangle.height = read_unsigned();
            if (in_nogo)
                return (false);
        }
        else {
            uobj.rectangle.height = MODAL_QUERY(geometry_h);
            if (in_nogo) {
                Errs()->add_error(
                    "read_rectangle: modal geometry_h unset.");
                return (false);
            }
        }
    }

    // [x]
    if (info_byte & 0x10) {
        uobj.rectangle.x = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.rectangle.x += modal.geometry_x;
    }
    else
        uobj.rectangle.x = modal.geometry_x;

    // [y]
    if (info_byte & 0x8) {
        uobj.rectangle.y = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.rectangle.y += modal.geometry_y;
    }
    else
        uobj.rectangle.y = modal.geometry_y;

    // [repetition]
    if (info_byte & 0x4) {
        if (!read_repetition())
            return (false);
        uobj.rectangle.repetition = &modal.repetition;
    }
    else
        uobj.rectangle.repetition = 0;

    // set modal variables
    MODAL_ASSIGN(layer, uobj.rectangle.layer);
    MODAL_ASSIGN(datatype, uobj.rectangle.datatype);
    modal.geometry_x = uobj.rectangle.x;
    modal.geometry_y = uobj.rectangle.y;
    MODAL_ASSIGN(geometry_w, uobj.rectangle.width);
    if (info_byte & 0x80)
        MODAL_ASSIGN(geometry_h, uobj.rectangle.width);
    else
        MODAL_ASSIGN(geometry_h, uobj.rectangle.height);

    if (!in_skip_action) {
        while (peek_property(ix)) ;
        if (!in_nogo && (!in_listonly || in_savebb)) {
            in_rgen.init(uobj.rectangle.repetition);
            if (!in_incremental) {
                int xoff, yoff;
                while (in_rgen.next(&xoff, &yoff)) {
                    uobj.rectangle.x = modal.geometry_x + xoff;
                    uobj.rectangle.y = modal.geometry_y + yoff;
                    if (!(this->*rectangle_action)()) {
                        Errs()->add_error("read_rectangle: action failed.");
                        in_nogo = true;
                        break;
                    }
                }
            }
        }
        (this->*clear_properties_action)();
    }

    in_state = in_nogo ? oasError : oasHasRectangle;
    return (!in_nogo);
}


// Read a POLYGON record, which is in the <geometry> class.
//
bool
oas_in::read_polygon(unsigned int ix)
{
    // '21' polygon-info-byte [layer-number] [datatype-number] [point-list]
    //      [x] [y] [repetition]
    if (ix != 21) {
        Errs()->add_error("read_polygon: bad index.");
        in_nogo = true;
        return (false);
    }

    unsigned char info_byte = read_byte();  // 00PXYRDL
    if (in_nogo)
        return (false);

    if (in_skip_all) {
        if (info_byte & 0x1) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x2) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x20) {
            ptlist_t *pl = read_pt_list();
            delete pl;
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x10) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x8) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x4) {
            if (!read_repetition())
                return (false);
        }
        return (true);
    }

    // [layer-number]
    if (info_byte & 0x1) {
        uobj.polygon.layer = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.polygon.layer = MODAL_QUERY(layer);
        if (in_nogo) {
            Errs()->add_error("read_polygon: modal layer unset.");
            return (false);
        }
    }

    // [datatype-number]
    if (info_byte & 0x2) {
        uobj.polygon.datatype = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.polygon.datatype = MODAL_QUERY(datatype);
        if (in_nogo) {
            Errs()->add_error("read_polygon: modal datatype unset.");
            return (false);
        }
    }

    // [point-list]
    if (info_byte & 0x20) {
        uobj.polygon.point_list =
            read_point_list(&uobj.polygon.point_list_size, true);
        if (in_nogo)
            return (false);
    }
    else {
        uobj.polygon.point_list = MODAL_QUERY(polygon_point_list);
        if (in_nogo) {
            Errs()->add_error(
                "read_polygon: modal polygon_point_list unset.");
            return (false);
        }
        uobj.polygon.point_list_size = modal.polygon_point_list_size;
    }

    // [x]
    if (info_byte & 0x10) {
        uobj.polygon.x = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.polygon.x += modal.geometry_x;
    }
    else
        uobj.polygon.x = modal.geometry_x;

    // [y]
    if (info_byte & 0x8) {
        uobj.polygon.y = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.polygon.y += modal.geometry_y;
    }
    else
        uobj.polygon.y = modal.geometry_y;

    // [repetition]
    if (info_byte & 0x4) {
        if (!read_repetition())
            return (false);
        uobj.polygon.repetition = &modal.repetition;
    }
    else
        uobj.polygon.repetition = 0;

    // set modal variables
    MODAL_ASSIGN(layer, uobj.polygon.layer);
    MODAL_ASSIGN(datatype, uobj.polygon.datatype);
    if (modal.polygon_point_list != uobj.polygon.point_list) {
        delete [] modal.polygon_point_list;
        modal.polygon_point_list = uobj.polygon.point_list;
        modal.polygon_point_list_size = uobj.polygon.point_list_size;
        modal.polygon_point_list_set = true;
    }
    modal.geometry_x = uobj.polygon.x;
    modal.geometry_y = uobj.polygon.y;

    if (!in_skip_action) {
        while (peek_property(ix)) ;
        if (!in_nogo && (!in_listonly || in_savebb)) {
            in_rgen.init(uobj.polygon.repetition);
            if (!in_incremental) {
                int xoff, yoff;
                while (in_rgen.next(&xoff, &yoff)) {
                    uobj.polygon.x = modal.geometry_x + xoff;
                    uobj.polygon.y = modal.geometry_y + yoff;
                    if (!(this->*polygon_action)()) {
                        Errs()->add_error("read_polygon: action failed.");
                        in_nogo = true;
                        break;
                    }
                }
            }
        }
        (this->*clear_properties_action)();
    }

    in_state = in_nogo ? oasError : oasHasPolygon;
    return (!in_nogo);
}


// Read a PATH record, which is in the <geometry> class.
//
bool
oas_in::read_path(unsigned int ix)
{
    // '22' path-info-byte [layer-number] [datatype-number] [half-width]
    //      [extension-scheme [start-exntension] [end-extension]]
    //      [point-list] [x] [y] [repetition]
    if (ix != 22) {
        Errs()->add_error("read_path: bad index.");
        in_nogo = true;
        return (false);
    }

    unsigned char info_byte = read_byte();  // EWPXYRDL
    if (in_nogo)
        return (false);

    if (in_skip_all) {
        if (info_byte & 0x1) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x2) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x40) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x80) {
            unsigned int extn = read_record_index(15);  // 0000SSEE
            if (in_nogo)
                return (false);
            unsigned int s_extn = (extn >> 2) & 0x3;
            if (s_extn == 3) {
                read_signed();
                if (in_nogo)
                    return (false);
            }
            unsigned int e_extn = extn & 0x3;
            if (e_extn == 3) {
                read_signed();
                if (in_nogo)
                    return (false);
            }
        }
        if (info_byte & 0x20) {
            ptlist_t *pl = read_pt_list();
            delete pl;
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x10) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x8) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x4) {
            if (!read_repetition())
                return (false);
        }
        return (true);
    }

    // [layer-number]
    if (info_byte & 0x1) {
        uobj.path.layer = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.path.layer = MODAL_QUERY(layer);
        if (in_nogo) {
            Errs()->add_error("read_path: modal layer unset.");
            return (false);
        }
    }

    // [datatype-number]
    if (info_byte & 0x2) {
        uobj.path.datatype = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.path.datatype = MODAL_QUERY(datatype);
        if (in_nogo) {
            Errs()->add_error("read_path: modal datatype unset.");
            return (false);
        }
    }

    // [half-width]
    if (info_byte & 0x40) {
        uobj.path.half_width = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.path.half_width = MODAL_QUERY(path_half_width);
        if (in_nogo) {
            Errs()->add_error("read_path: modal path_half_width unset.");
            return (false);
        }
    }

    // [extension-scheme [start-exntension] [end-extension]]
    if (info_byte & 0x80) {
        unsigned int extn = read_record_index(15);  // 0000SSEE
        if (in_nogo)
            return (false);
        unsigned int s_extn = (extn >> 2) & 0x3;
        if (s_extn == 0) {
            uobj.path.start_extension = MODAL_QUERY(path_start_extension);
            if (in_nogo) {
                Errs()->add_error(
                    "read_path: modal path_start_extension unset.");
                return (false);
            }
        }
        else if (s_extn == 1)
            uobj.path.start_extension = 0;
        else if (s_extn == 2)
            uobj.path.start_extension = uobj.path.half_width;
        else {
            uobj.path.start_extension = read_signed();
            if (in_nogo)
                return (false);
        }
        unsigned int e_extn = extn & 0x3;
        if (e_extn == 0) {
            uobj.path.end_extension = MODAL_QUERY(path_end_extension);
            if (in_nogo) {
                Errs()->add_error(
                    "read_path: modal path_end_extension unset.");
                return (false);
            }
        }
        else if (e_extn == 1)
            uobj.path.end_extension = 0;
        else if (e_extn == 2)
            uobj.path.end_extension = uobj.path.half_width;
        else {
            uobj.path.end_extension = read_signed();
            if (in_nogo)
                return (false);
        }
    }
    else {
        uobj.path.start_extension = MODAL_QUERY(path_start_extension);
        if (in_nogo) {
            Errs()->add_error(
                "read_path: modal path_start_extension unset.");
            return (false);
        }
        uobj.path.end_extension = MODAL_QUERY(path_end_extension);
        if (in_nogo) {
            Errs()->add_error(
                "read_path: modal path_end_extension unset.");
            return (false);
        }
    }

    // [point-list]
    if (info_byte & 0x20) {
        uobj.path.point_list =
            read_point_list(&uobj.path.point_list_size, false);
        if (in_nogo)
            return (false);
    }
    else {
        uobj.path.point_list = MODAL_QUERY(path_point_list);
        if (in_nogo) {
            Errs()->add_error(
                "read_path: modal path_point_list unset.");
            return (false);
        }
        uobj.path.point_list_size = modal.path_point_list_size;
    }

    // [x]
    if (info_byte & 0x10) {
        uobj.path.x = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.path.x += modal.geometry_x;
    }
    else
        uobj.path.x = modal.geometry_x;

    // [y]
    if (info_byte & 0x8) {
        uobj.path.y = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.path.y += modal.geometry_y;
    }
    else
        uobj.path.y = modal.geometry_y;

    // [repetition]
    if (info_byte & 0x4) {
        if (!read_repetition())
            return (false);
        uobj.path.repetition = &modal.repetition;
    }
    else
        uobj.path.repetition = 0;

    // set modal variables
    MODAL_ASSIGN(layer, uobj.path.layer);
    MODAL_ASSIGN(datatype, uobj.path.datatype);
    MODAL_ASSIGN(path_half_width, uobj.path.half_width);
    MODAL_ASSIGN(path_start_extension, uobj.path.start_extension);
    MODAL_ASSIGN(path_end_extension, uobj.path.end_extension);
    if (modal.path_point_list != uobj.path.point_list) {
        delete [] modal.path_point_list;
        modal.path_point_list = uobj.path.point_list;
        modal.path_point_list_size = uobj.path.point_list_size;
        modal.path_point_list_set = true;
    }
    modal.geometry_x = uobj.path.x;
    modal.geometry_y = uobj.path.y;

    uobj.path.rounded_end = false;

    if (!in_skip_action) {
        while (peek_property(ix)) ;
        if (!in_nogo && (!in_listonly || in_savebb)) {
            in_rgen.init(uobj.path.repetition);
            if (!in_incremental) {
                int xoff, yoff;
                while (in_rgen.next(&xoff, &yoff)) {
                    uobj.path.x = modal.geometry_x + xoff;
                    uobj.path.y = modal.geometry_y + yoff;
                    if (!(this->*path_action)()) {
                        Errs()->add_error("read_path: action failed.");
                        in_nogo = true;
                        break;
                    }
                }
            }
        }
        (this->*clear_properties_action)();
    }

    in_state = in_nogo ? oasError : oasHasPath;
    return (!in_nogo);
}


// Read a TRAPEZOID record, which is in the <geometry> class.
//
bool
oas_in::read_trapezoid(unsigned int ix)
{
    // '23' trap-info-byte [layer-number] [datatype-number]
    //      [width] [height] delta-a delta-b [x] [y] [repetition]
    // '24' trap-info-byte [layer-number] [datatype-number]
    //      [width] [height] delta-a [x] [y] [repetition]
    // '25' trap-info-byte [layer-number] [datatype-number]
    //      [width] [height] delta-b [x] [y] [repetition]
    if (ix < 23 || ix > 25) {
        Errs()->add_error("read_trapezoid: bad index.");
        in_nogo = true;
        return (false);
    }

    unsigned char info_byte = read_byte();  // 0WHXYRDL
    if (in_nogo)
        return (false);

    if (in_skip_all) {
        if (info_byte & 0x1) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x2) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x40) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x20) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (ix == 23) {
            read_signed();
            read_signed();
            if (in_nogo)
                return (false);
        }
        else {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x10) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x8) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x4) {
            if (!read_repetition())
                return (false);
        }
        return (true);
    }

    // [layer-number]
    if (info_byte & 0x1) {
        uobj.trapezoid.layer = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.trapezoid.layer = MODAL_QUERY(layer);
        if (in_nogo) {
            Errs()->add_error("read_trapezoid: modal layer unset.");
            return (false);
        }
    }

    // [datatype-number]
    if (info_byte & 0x2) {
        uobj.trapezoid.datatype = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.trapezoid.datatype = MODAL_QUERY(datatype);
        if (in_nogo) {
            Errs()->add_error("read_trapezoid: modal datatype unset.");
            return (false);
        }
    }

    // [width]
    if (info_byte & 0x40) {
        uobj.trapezoid.width = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.trapezoid.width = MODAL_QUERY(geometry_w);
        if (in_nogo) {
            Errs()->add_error("read_trapezoid: modal geometry_w unset.");
            return (false);
        }
    }

    // [height]
    if (info_byte & 0x20) {
        uobj.trapezoid.height = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.trapezoid.height = MODAL_QUERY(geometry_h);
        if (in_nogo) {
            Errs()->add_error("read_trapezoid: modal geometry_h unset.");
            return (false);
        }
    }

    uobj.trapezoid.delta_a = 0;
    uobj.trapezoid.delta_b = 0;
    if (ix == 23) {
        uobj.trapezoid.delta_a = read_signed();
        if (in_nogo)
            return (false);
        uobj.trapezoid.delta_b = read_signed();
        if (in_nogo)
            return (false);
    }
    else if (ix == 24) {
        uobj.trapezoid.delta_a = read_signed();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.trapezoid.delta_b = read_signed();
        if (in_nogo)
            return (false);
    }
    uobj.trapezoid.vertical = (info_byte & 0x80);

    // [x]
    if (info_byte & 0x10) {
        uobj.trapezoid.x = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.trapezoid.x += modal.geometry_x;
    }
    else
        uobj.trapezoid.x = modal.geometry_x;

    // [y]
    if (info_byte & 0x8) {
        uobj.trapezoid.y = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.trapezoid.y += modal.geometry_y;
    }
    else
        uobj.trapezoid.y = modal.geometry_y;

    // [repetition]
    if (info_byte & 0x4) {
        if (!read_repetition())
            return (false);
        uobj.trapezoid.repetition = &modal.repetition;
    }
    else
        uobj.trapezoid.repetition = 0;

    // set modal variables
    MODAL_ASSIGN(layer, uobj.trapezoid.layer);
    MODAL_ASSIGN(datatype, uobj.trapezoid.datatype);
    MODAL_ASSIGN(geometry_w, uobj.trapezoid.width);
    MODAL_ASSIGN(geometry_h, uobj.trapezoid.height);
    modal.geometry_x = uobj.trapezoid.x;
    modal.geometry_y = uobj.trapezoid.y;

    if (in_save_zoids) {
        Zoid Z;
        if (!uobj.trapezoid.vertical) {
            Z.yl = uobj.trapezoid.y;
            Z.yu = uobj.trapezoid.y + uobj.trapezoid.height;
            if (uobj.trapezoid.delta_a < 0) {
                Z.xul = uobj.trapezoid.x;
                Z.xll = Z.xul - uobj.trapezoid.delta_a;
            }
            else {
                Z.xll = uobj.trapezoid.x;
                Z.xul = Z.xll + uobj.trapezoid.delta_a;
            }
            if (uobj.trapezoid.delta_b < 0) {
                Z.xlr = uobj.trapezoid.x + uobj.trapezoid.width;
                Z.xur = Z.xlr + uobj.trapezoid.delta_b;
            }
            else {
                Z.xur = uobj.trapezoid.x + uobj.trapezoid.width;
                Z.xlr = Z.xur - uobj.trapezoid.delta_b;
            }
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
    }

    if (!in_skip_action) {
        while (peek_property(ix)) ;
        if (!in_nogo && (!in_listonly || in_savebb)) {
            in_rgen.init(uobj.trapezoid.repetition);
            if (!in_incremental) {
                int xoff, yoff;
                while (in_rgen.next(&xoff, &yoff)) {
                    uobj.trapezoid.x = modal.geometry_x + xoff;
                    uobj.trapezoid.y = modal.geometry_y + yoff;
                    if (!(this->*trapezoid_action)()) {
                        Errs()->add_error("read_trapezoid: action failed.");
                        in_nogo = true;
                        break;
                    }
                }
            }
        }
        (this->*clear_properties_action)();
    }

    in_state = in_nogo ? oasError : oasHasTrapezoid;
    return (!in_nogo);
}


// Read a CTRAPEZOID record, which is in the <geometry> class.
//
bool
oas_in::read_ctrapezoid(unsigned int ix)
{
    // '26' ctrapezoid-info-byte [layer-number] [datatype-number]
    //      [ctrapezoid-type] [width] [height] [x] [y] [repetition]
    if (ix != 26) {
        Errs()->add_error("read_ctrapezoid: bad index.");
        in_nogo = true;
        return (false);
    }

    unsigned char info_byte = read_byte();  // TWHXYRDL
    if (in_nogo)
        return (false);

    if (in_skip_all) {
        if (info_byte & 0x1) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x2) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x80) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x40) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x20) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x10) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x8) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x4) {
            if (!read_repetition())
                return (false);
        }
        return (true);
    }

    // [layer-number]
    if (info_byte & 0x1) {
        uobj.ctrapezoid.layer = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.ctrapezoid.layer = MODAL_QUERY(layer);
        if (in_nogo) {
            Errs()->add_error("read_ctrapezoid: modal layer unset.");
            return (false);
        }
    }

    // [datatype-number]
    if (info_byte & 0x2) {
        uobj.ctrapezoid.datatype = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.ctrapezoid.datatype = MODAL_QUERY(datatype);
        if (in_nogo) {
            Errs()->add_error("read_ctrapezoid: modal datatype unset.");
            return (false);
        }
    }

    // [ctrapezoid-type]
    if (info_byte & 0x80) {
        uobj.ctrapezoid.type = read_unsigned();
        if (in_nogo)
            return (false);
        if (uobj.ctrapezoid.type > 25) {
            Errs()->add_error("read_ctrapezoid: type out of range.");
            in_nogo = true;
            return (false);
        }
    }
    else {
        uobj.ctrapezoid.type = MODAL_QUERY(ctrapezoid_type);
        if (in_nogo) {
            Errs()->add_error(
                "read_ctrapezoid: modal ctrapezoid_type unset.");
            return (false);
        }
    }

    // [width]
    if (info_byte & 0x40) {
        unsigned int t = uobj.ctrapezoid.type;
        if (t == 20 || t == 21) {
            Errs()->add_error(
                "read_ctrapezoid: width given for type %d.", t);
            return (false);
        }
        uobj.ctrapezoid.width = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        unsigned int t = uobj.ctrapezoid.type;
        if (!(t == 20 || t == 21)) {
            uobj.ctrapezoid.width = MODAL_QUERY(geometry_w);
            if (in_nogo) {
                Errs()->add_error(
                    "read_ctrapezoid: modal geometry_w unset.");
                return (false);
            }
        }
        else
            uobj.ctrapezoid.width = 0;
    }

    // [height]
    if (info_byte & 0x20) {
        unsigned int t = uobj.ctrapezoid.type;
        if ((t >= 16 && t <= 19) || t == 22 || t == 23 || t == 25) {
            Errs()->add_error(
                "read_ctrapezoid: height given for type %d.", t);
            return (false);
        }
        uobj.ctrapezoid.height = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        unsigned int t = uobj.ctrapezoid.type;
        if (!((t >= 16 && t <= 19) || t == 22 || t == 23 || t == 25)) {
            uobj.ctrapezoid.height = MODAL_QUERY(geometry_h);
            if (in_nogo) {
                Errs()->add_error(
                    "read_ctrapezoid: modal geometry_h unset.");
                return (false);
            }
        }
        else
            uobj.ctrapezoid.height = 0;
    }

    // [x]
    if (info_byte & 0x10) {
        uobj.ctrapezoid.x = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.ctrapezoid.x += modal.geometry_x;
    }
    else
        uobj.ctrapezoid.x = modal.geometry_x;

    // [y]
    if (info_byte & 0x8) {
        uobj.ctrapezoid.y = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.ctrapezoid.y += modal.geometry_y;
    }
    else
        uobj.ctrapezoid.y = modal.geometry_y;

    // [repetition]
    if (info_byte & 0x4) {
        if (!read_repetition())
            return (false);
        uobj.ctrapezoid.repetition = &modal.repetition;
    }
    else
        uobj.ctrapezoid.repetition = 0;

    // set modal variables
    MODAL_ASSIGN(layer, uobj.ctrapezoid.layer);
    MODAL_ASSIGN(datatype, uobj.ctrapezoid.datatype);
    MODAL_ASSIGN(ctrapezoid_type, uobj.ctrapezoid.type);

    unsigned int t = uobj.ctrapezoid.type;
    if ((t >= 16 && t <= 19) || t == 25) {
        MODAL_ASSIGN(geometry_w, uobj.ctrapezoid.width);
        MODAL_ASSIGN(geometry_h, uobj.ctrapezoid.width);
    }
    else if (t == 22 || t == 23) {
        MODAL_ASSIGN(geometry_w, uobj.ctrapezoid.width);
        MODAL_ASSIGN(geometry_h, 2*uobj.ctrapezoid.width);
    }
    else if (t == 20 || t == 21) {
        MODAL_ASSIGN(geometry_w, 2*uobj.ctrapezoid.height);
        MODAL_ASSIGN(geometry_h, uobj.ctrapezoid.height);
    }
    else {
        MODAL_ASSIGN(geometry_w, uobj.ctrapezoid.width);
        MODAL_ASSIGN(geometry_h, uobj.ctrapezoid.height);
    }
    modal.geometry_x = uobj.ctrapezoid.x;
    modal.geometry_y = uobj.ctrapezoid.y;

    if (in_save_zoids) {
        Zoid Z;
        Z.yl = uobj.ctrapezoid.y;
        if (uobj.ctrapezoid.type == 0) {
            Z.xll = Z.xul = uobj.ctrapezoid.x;
            Z.xlr = Z.xll + uobj.ctrapezoid.width;
            Z.xur = Z.xlr - uobj.ctrapezoid.height;
            Z.yu = Z.yl + uobj.ctrapezoid.height;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 1) {
            Z.xll = Z.xul = uobj.ctrapezoid.x;
            Z.xur = Z.xul + uobj.ctrapezoid.width;
            Z.xlr = Z.xur - uobj.ctrapezoid.height;
            Z.yu = Z.yl + uobj.ctrapezoid.height;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 2) {
            Z.xll = uobj.ctrapezoid.x;
            Z.xul = Z.xll + uobj.ctrapezoid.height;
            Z.xlr = Z.xur = Z.xll + uobj.ctrapezoid.width;
            Z.yu = Z.yl + uobj.ctrapezoid.height;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 3) {
            Z.xul = uobj.ctrapezoid.x;
            Z.xll = Z.xul + uobj.ctrapezoid.height;
            Z.xlr = Z.xur = Z.xul + uobj.ctrapezoid.width;
            Z.yu = Z.yl + uobj.ctrapezoid.height;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 4) {
            Z.xll = uobj.ctrapezoid.x;
            Z.xul = Z.xll + uobj.ctrapezoid.height;
            Z.xlr = Z.xll + uobj.ctrapezoid.width;
            Z.xur = Z.xlr - uobj.ctrapezoid.height;
            Z.yu = Z.yl + uobj.ctrapezoid.height;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 5) {
            Z.xul = uobj.ctrapezoid.x;
            Z.xll = Z.xul + uobj.ctrapezoid.height;
            Z.xur = Z.xul + uobj.ctrapezoid.width;
            Z.xlr = Z.xur - uobj.ctrapezoid.height;
            Z.yu = Z.yl + uobj.ctrapezoid.height;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 6) {
            Z.xll = uobj.ctrapezoid.x;
            Z.xul = Z.xll + uobj.ctrapezoid.height;
            Z.xur = Z.xll + uobj.ctrapezoid.width;
            Z.xlr = Z.xur - uobj.ctrapezoid.height;
            Z.yu = Z.yl + uobj.ctrapezoid.height;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 7) {
            Z.xul = uobj.ctrapezoid.x;
            Z.xll = Z.xul + uobj.ctrapezoid.height;
            Z.xlr = Z.xul + uobj.ctrapezoid.width;
            Z.xur = Z.xlr - uobj.ctrapezoid.height;
            Z.yu = Z.yl + uobj.ctrapezoid.height;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 16) {
            Z.xul = Z.xll = Z.xur = uobj.ctrapezoid.x;
            Z.xlr = Z.xll + uobj.ctrapezoid.width;
            Z.yu = Z.yl + uobj.ctrapezoid.width;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 17) {
            Z.xul = Z.xll = Z.xlr = uobj.ctrapezoid.x;
            Z.xur = Z.xul + uobj.ctrapezoid.width;
            Z.yu = Z.yl + uobj.ctrapezoid.width;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 18) {
            Z.xll = uobj.ctrapezoid.x;
            Z.xul = Z.xur = Z.xlr = Z.xll + uobj.ctrapezoid.width;
            Z.yu = Z.yl + uobj.ctrapezoid.width;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 19) {
            Z.xul = uobj.ctrapezoid.x;
            Z.xll = Z.xur = Z.xlr = Z.xul + uobj.ctrapezoid.width;
            Z.yu = Z.yl + uobj.ctrapezoid.width;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 20) {
            Z.xll = uobj.ctrapezoid.x;
            Z.xul = Z.xur = Z.xll + uobj.ctrapezoid.height;
            Z.xlr = Z.xll + 2*uobj.ctrapezoid.height;
            Z.yu = Z.yl + uobj.ctrapezoid.height;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 21) {
            Z.xul = uobj.ctrapezoid.x;
            Z.xll = Z.xlr = Z.xul + uobj.ctrapezoid.height;
            Z.xur = Z.xul + 2*uobj.ctrapezoid.height;
            Z.yu = Z.yl + uobj.ctrapezoid.height;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 24) {
            Z.xll = Z.xul = uobj.ctrapezoid.x;
            Z.xlr = Z.xur = Z.xll + uobj.ctrapezoid.width;
            Z.yu = Z.yl + uobj.ctrapezoid.height;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
        else if (uobj.ctrapezoid.type == 25) {
            Z.xll = Z.xul = uobj.ctrapezoid.x;
            Z.xlr = Z.xur = Z.xll + uobj.ctrapezoid.width;
            Z.yu = Z.yl + uobj.ctrapezoid.width;
            in_zoidlist = new Zlist(&Z, in_zoidlist);
        }
    }
    if (!in_skip_action) {
        while (peek_property(ix)) ;
        if (!in_nogo && (!in_listonly || in_savebb)) {
            in_rgen.init(uobj.ctrapezoid.repetition);
            if (!in_incremental) {
                int xoff, yoff;
                while (in_rgen.next(&xoff, &yoff)) {
                    uobj.ctrapezoid.x = modal.geometry_x + xoff;
                    uobj.ctrapezoid.y = modal.geometry_y + yoff;
                    if (!(this->*ctrapezoid_action)()) {
                        Errs()->add_error("read_ctrapezoid: action failed.");
                        in_nogo = true;
                        break;
                    }
                }
            }
        }
        (this->*clear_properties_action)();
    }

    in_state = in_nogo ? oasError : oasHasCtrapezoid;
    return (!in_nogo);
}


// Read a CIRCLE record, which is in the <geometry> class.
//
bool
oas_in::read_circle(unsigned int ix)
{
    // '27' circle-info-byte [layer-number] [datatype-number]
    //      [radius] [x] [y] [repetition]
    if (ix != 27) {
        Errs()->add_error("read_circle: bad index.");
        in_nogo = true;
        return (false);
    }

    unsigned char info_byte = read_byte();  // 00rXYRDL
    if (in_nogo)
        return (false);

    if (in_skip_all) {
        if (info_byte & 0x1) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x2) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x20) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x10) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x8) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x4) {
            if (!read_repetition())
                return (false);
        }
        return (true);
    }

    // [layer-number]
    if (info_byte & 0x1) {
        uobj.circle.layer = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.circle.layer = MODAL_QUERY(layer);
        if (in_nogo) {
            Errs()->add_error("read_circle: modal layer unset.");
            return (false);
        }
    }

    // [datatype-number]
    if (info_byte & 0x2) {
        uobj.circle.datatype = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.circle.datatype = MODAL_QUERY(datatype);
        if (in_nogo) {
            Errs()->add_error("read_circle: modal datatype unset.");
            return (false);
        }
    }

    // [circle-radius]
    if (info_byte & 0x20) {
        uobj.circle.radius = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.circle.radius = MODAL_QUERY(circle_radius);
        if (in_nogo) {
            Errs()->add_error("read_circle: modal circle_radius unset.");
            return (false);
        }
    }

    // [x]
    if (info_byte & 0x10) {
        uobj.circle.x = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.circle.x += modal.geometry_x;
    }
    else
        uobj.circle.x = modal.geometry_x;

    // [y]
    if (info_byte & 0x8) {
        uobj.circle.y = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.circle.y += modal.geometry_y;
    }
    else
        uobj.circle.y = modal.geometry_y;

    // [repetition]
    if (info_byte & 0x4) {
        if (!read_repetition())
            return (false);
        uobj.circle.repetition = &modal.repetition;
    }
    else
        uobj.circle.repetition = 0;

    // set modal variables
    MODAL_ASSIGN(layer, uobj.circle.layer);
    MODAL_ASSIGN(datatype, uobj.circle.datatype);
    MODAL_ASSIGN(circle_radius, uobj.circle.radius);
    modal.geometry_x = uobj.circle.x;
    modal.geometry_y = uobj.circle.y;

    if (!in_skip_action) {
        while (peek_property(ix)) ;
        if (!in_nogo && (!in_listonly || in_savebb)) {
            in_rgen.init(uobj.circle.repetition);
            if (!in_incremental) {
                int xoff, yoff;
                while (in_rgen.next(&xoff, &yoff)) {
                    uobj.circle.x = modal.geometry_x + xoff;
                    uobj.circle.y = modal.geometry_y + yoff;
                    if (!(this->*circle_action)()) {
                        Errs()->add_error("read_circle: action failed.");
                        in_nogo = true;
                        break;
                    }
                }
            }
        }
        (this->*clear_properties_action)();
    }

    in_state = in_nogo ? oasError : oasHasCircle;
    return (!in_nogo);
}


// Read a PROPERTY record.  These records immediately follow the
// associated object.
//
bool
oas_in::read_property(unsigned int ix)
{
    // '28' prop-info-byte [reference-number | propname-string]
    //      [prop-value-count] [<property-value>]
    // '29'
    if (ix != 28 && ix != 29) {
        Errs()->add_error("read_property: bad index.");
        in_nogo = true;
        return (false);
    }

    if (in_skip_all) {
        if (ix == 29)
            return (true);

        unsigned char info_byte = read_byte();  // UUUUVCNS
        if (in_nogo)
            return (false);

        if (info_byte & 0x4) {
            if (info_byte & 0x2) {
                read_unsigned();
                if (in_nogo)
                    return (false);
            }
            else {
                read_n_string_nr();
                if (in_nogo)
                    return (false);
            }
        }
        if (!(info_byte & 0x8)) {
            int n = info_byte >> 4;
            unsigned int sz;
            if (n == 15) {
                sz = read_unsigned();
                if (in_nogo)
                    return (false);
            }
            else
                sz = n;
            for (unsigned int i = 0; i < sz; i++) {
                if (!read_property_value(0))
                    return (false);
            }
        }
        return (true);
    }

    char *property_name;
    oas_property_value *property_list;
    unsigned int property_list_size;
    bool standard_property = false;
    if (ix == 28) {
        unsigned char info_byte = read_byte();  // UUUUVCNS
        if (in_nogo)
            return (false);

        // [reference-number | propname-string]
        char *prpname = 0;
        if (info_byte & 0x4) {
            if (info_byte & 0x2) {
                unsigned int refnum = read_unsigned();
                if (in_nogo)
                    return (false);
                oas_elt *te = in_propname_table->get(refnum);
                if (!te) {
                    // Scan ahead if not strict-mode.
                    if (!in_propname_flag) {
                        if (!scan_for_names())
                            return (false);
                        te = in_propname_table->get(refnum);
                    }
                }
                if (!te) {
                    Errs()->add_error(
                        "read_property: propname reference %d not found.",
                        refnum);
                    in_nogo = true;
                    return (false);
                }
                prpname = property_name = lstring::copy(te->string());
                // te->properties?
            }
            else {
                prpname = property_name = read_n_string();
                if (in_nogo)
                    return (false);
            }
        }
        else {
            property_name = MODAL_QUERY(last_property_name);
            if (in_nogo) {
                Errs()->add_error(
                    "read_property: modal last_property_name unset.");
                return (false);
            }
        }
        GCarray<char*> gc_prpname(prpname);

        if (info_byte & 0x8) {
            // use last-property-list
            if (info_byte & 0xf0) {
                Errs()->add_error("read_property: both U and V set.");
                in_nogo = true;
                return (false);
            }
            property_list = MODAL_QUERY(last_value_list);
            if (in_nogo) {
                Errs()->add_error(
                    "read_property: modal last_value_list unset.");
                return (false);
            }
            property_list_size = modal.last_value_list_size;
        }
        else {
            int n = info_byte >> 4;
            if (n == 15) {
                property_list_size = read_unsigned();
                if (in_nogo)
                    return (false);
            }
            else
                property_list_size = n;
            property_list = new oas_property_value[property_list_size];
            for (unsigned int i = 0; i < property_list_size; i++) {
                if (!read_property_value(property_list + i)) {
                    delete [] property_list;
                    return (false);
                }
            }
        }
        standard_property = (info_byte & 1);
        gc_prpname.clear();
    }
    else {
        property_name = MODAL_QUERY(last_property_name);
        if (in_nogo) {
            Errs()->add_error(
                "read_property 29: modal last_property_name unset.");
            return (false);
        }
        property_list = MODAL_QUERY(last_value_list);
        if (in_nogo) {
            Errs()->add_error(
                "read_property 29: modal last_value_list unset.");
            return (false);
        }
        property_list_size = modal.last_value_list_size;
        standard_property = modal.last_value_standard;
    }

    if (modal.last_property_name != property_name) {
        delete [] modal.last_property_name;
        modal.last_property_name = property_name;
        modal.last_property_name_set = true;
    }
    if (modal.last_value_list != property_list) {
        delete [] modal.last_value_list;
        modal.last_value_list = property_list;
        modal.last_value_list_size = property_list_size;
        modal.last_value_list_set = true;
    }
    modal.last_value_standard = standard_property;

    if (!in_skip_action) {
        if (standard_property) {
            if (!(this->*standard_property_action)()) {
                Errs()->add_error(
                    "read_property: standard property action failed.");
                in_nogo = true;
                return (false);
            }
        }
        else {
            if (!(this->*property_action)()) {
                Errs()->add_error("read_property: property action failed.");
                in_nogo = true;
                return (false);
            }
        }
    }
    return (true);
}


// Read an XNAME record, which is in the <name> class.
//
bool
oas_in::read_xname(unsigned int ix)
{
    // '30' xname-attribute xname-string
    // '31' xname-attribute xname-string reference-number
    if (ix != 30 && ix != 31) {
        Errs()->add_error("read_xname: bad index.");
        in_nogo = true;
        return (false);
    }

    modal.reset();
    if (ix != in_xname_type) {
        if (!in_xname_type)
            in_xname_type = ix;
        else {
            Errs()->add_error("read_xname: types 30 and 31 found.");
            in_nogo = true;
            return (false);
        }
    }

    unsigned int len = 0;
    char *str = read_b_string(&len);
    if (in_nogo)
        return (false);
    GCarray<char*> gc_str(str);

    unsigned int attr = read_unsigned();
    if (in_nogo)
        return (false);
    unsigned int num;
    if (ix == 30)       // xname-attribute xname-string
        num = in_xname_index++;
    else {              // xname-attribute xname-string reference-number
        num = read_unsigned();
        if (in_nogo)
            return (false);
    }
    // We may have already read this record as part of a table.
    if (!in_xname_table)
        in_xname_table = new oas_table;
    oas_elt *te = in_xname_table->get(num);
    if (te) {
        if (memcmp(str, te->string(), len)) {
            Errs()->add_error("read_xname: duplicate refnum %d.", num);
            in_nogo = true;
            return (false);
        }
    }
    else {
        if (in_xname_flag) {
            // Table may not have been read yet!
            if (in_scanning) {
                if (ix == 30)
                    in_xname_index--;
                return (true);
            }

            // in strict mode, shouldn't be here
            Errs()->add_error(
                "read_xname: strict mode, record not in table.");
            in_nogo = true;
            return (false);
        }
        te = in_xname_table->add(num);
        in_xname_table = in_xname_table->check_rehash();
        te->set_string(str, len);
        gc_str.clear();
        te->set_data((void*)(long)attr);
    }

    if (!in_skip_action && !in_skip_all) {
        // read properties
        while (peek_property(ix)) ;
        if (in_nogo)
            return (false);
        if (!te->prpty_list())
            te->set_prpty_list(in_prpty_list);
        else
            CDp::destroy(in_prpty_list);
        in_prpty_list = 0;
    }

    return (true);
}


// Read an XELEMENT record.
//
bool
oas_in::read_xelement(unsigned int ix)
{
    // '32' xelement-attribute xelement-string
    if (ix != 32) {
        Errs()->add_error("read_xelement: bad index.");
        in_nogo = true;
        return (false);
    }

    unsigned int xattr = read_unsigned();
    if (in_nogo)
        return (false);
    read_b_string_nr();
    if (in_nogo)
        return (false);

    // do something here eventually
    (void)xattr;
    return (true);
}


// Read an XGEOMETRY record, which is in the <geometry> class.
//
bool
oas_in::read_xgeometry(unsigned int ix)
{
    // '33' xgeometry-info-byte xgeometry-attribute
    //      [layer-number] [datatype-number] xgeometry-string
    //      [x] [y] [repetition]
    if (ix != 33) {
        Errs()->add_error("read_xgeometry: bad index.");
        in_nogo = true;
        return (false);
    }

    unsigned char info_byte = read_byte();  // 000XYRDL
    if (in_nogo)
        return (false);

    if (in_skip_all) {
        read_unsigned();
        if (in_nogo)
            return (false);
        if (info_byte & 0x1) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x2) {
            read_unsigned();
            if (in_nogo)
                return (false);
        }
        read_b_string_nr();
        if (in_nogo)
            return (false);
        if (info_byte & 0x10) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x8) {
            read_signed();
            if (in_nogo)
                return (false);
        }
        if (info_byte & 0x4) {
            if (!read_repetition())
                return (false);
        }
        return (true);
    }

    // xgeometry-attribute
    uobj.xgeometry.attribute = read_unsigned();
    if (in_nogo)
        return (false);

    // [layer-number]
    if (info_byte & 0x1) {
        uobj.xgeometry.layer = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.xgeometry.layer = MODAL_QUERY(layer);
        if (in_nogo) {
            Errs()->add_error("read_xgeometry: modal layer unset.");
            return (false);
        }
    }

    // [datatype-number]
    if (info_byte & 0x2) {
        uobj.xgeometry.datatype = read_unsigned();
        if (in_nogo)
            return (false);
    }
    else {
        uobj.xgeometry.datatype = MODAL_QUERY(datatype);
        if (in_nogo) {
            Errs()->add_error("read_xgeometry: modal datatype unset.");
            return (false);
        }
    }

    // xgeometry-string
    unsigned int len;
    char *str = read_b_string(&len);
    if (in_nogo)
        return (false);
    GCarray<char*> gc_str(str);

    // [x]
    if (info_byte & 0x10) {
        uobj.xgeometry.x = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.xgeometry.x += modal.geometry_x;
    }
    else
        uobj.xgeometry.x = modal.geometry_x;

    // [y]
    if (info_byte & 0x8) {
        uobj.xgeometry.y = read_signed();
        if (in_nogo)
            return (false);
        if (modal.xy_mode == XYrelative)
            uobj.xgeometry.y += modal.geometry_y;
    }
    else
        uobj.xgeometry.y = modal.geometry_y;

    // [repetition]
    if (info_byte & 0x4) {
        if (!read_repetition())
            return (false);
        uobj.xgeometry.repetition = &modal.repetition;
    }
    else
        uobj.xgeometry.repetition = 0;

    // set modal variables
    MODAL_ASSIGN(layer, uobj.xgeometry.layer);
    MODAL_ASSIGN(datatype, uobj.xgeometry.datatype);
    modal.geometry_x = uobj.xgeometry.x;
    modal.geometry_y = uobj.xgeometry.y;

    if (!in_skip_action) {
        while (peek_property(ix)) ;
        if (!in_nogo && (!in_listonly || in_savebb)) {
            in_rgen.init(uobj.xgeometry.repetition);
            if (!in_incremental) {
                int xoff, yoff;
                while (in_rgen.next(&xoff, &yoff)) {
                    uobj.xgeometry.x = modal.geometry_x + xoff;
                    uobj.xgeometry.y = modal.geometry_y + yoff;
                    /*
                    if (!(this->*xgeometry_action)()) {
                        Errs()->add_error("read_rectangle: action failed.");
                        in_nogo = true;
                        break;
                    }
                    */
                }
            }
        }
        (this->*clear_properties_action)();
    }

    in_state = in_nogo ? oasError : oasHasXgeometry;
    return (!in_nogo);
}


// Read a CBLOCK record.
//
bool
oas_in::read_cblock(unsigned int ix)
{
    // '34' comp-type uncomp-byte-count comp-byte-count comp-bytes
    if (in_zfile) {
        Errs()->add_error(
            "read_cblock: already decompressing, nested CBLOCK?");
        in_nogo = true;
        return (false);
    }
    if (!in_fp) {
        Errs()->add_error("read_cblock: null file pointer.");
        in_nogo = true;
        return (false);
    }
    in_offset = in_fp->z_tell() - 1;
    if (ix != 34) {
        Errs()->add_error("read_cblock: bad index.");
        in_nogo = true;
        return (false);
    }

    unsigned int comp_type = read_unsigned();
    if (in_nogo)
        return (false);
    if (comp_type != 0) {
        Errs()->add_error("read_cblock: unknown compression-type %d.",
            comp_type);
        in_nogo = true;
        return (false);
    }
    uint64_t uncomp_cnt = read_unsigned64();
    if (in_nogo)
        return (false);
    uint64_t comp_cnt = read_unsigned64();
    if (in_nogo)
        return (false);

    in_byte_offset = in_fp->z_tell();  // should already be equal
    in_zfile = zio_stream::zio_open(in_fp, "r", comp_cnt);
    if (!in_zfile) {
        Errs()->add_error("read_cblock: open failed.");
        in_nogo = true;
        return (false);
    }
    in_compression_end = in_byte_offset + uncomp_cnt;
    in_next_offset = in_byte_offset + comp_cnt;
    return (true);
}


bool
oas_in::peek_property(unsigned int ix)
{
    int c = read_byte();
    if (in_nogo) {
        if (in_byte_stream)
            // End of stream is not a fatal error.
            in_nogo = false;
        return (false);
    }
    if (c == 0)
        // PAD
        return (true);
    if (c == 28 || c == 29 || c == 34) {
        // PROPERTY or CBLOCK
        in_peek_record = ix;
        // in_offset should point to the start of the record when the
        // ftab dispatch functions are called, so have to back it up
        // here since read_property will reset it.  If read_property
        // fails, keep its offset for error message.
        //
        uint64_t tmpoff = in_offset;
        if (info())
            info()->add_record(c);
        bool ret = dispatch(c);
        in_peek_record = 0;
        if (!in_nogo)
            in_offset = tmpoff;
        return (ret);
    }
    in_peek_byte = c;
    in_peeked = true;
    return (false);
}


// Return true with the parameters set if the placement repetition can be
// converted to a database instance array.
//
bool
oas_in::placement_array_params(int *dx, int *dy, unsigned int *nx,
    unsigned int *ny)
{
    *dx = 0;
    *dy = 0;
    *nx = 1;
    *ny = 1;
    if (!uobj.placement.repetition || !uobj.placement.repetition->periodic())
        return (false);
    double angle = uobj.placement.angle;
    double magn = 1.0;

    int ax, ay;
    char *emsg = FIO()->GdsParamSet(&angle, &magn, &ax, &ay);
    delete [] emsg;

    TPush();
    if (uobj.placement.flip_y)
        TMY();
    TRotate(ax, ay);
    TInverse();
    TPop();

    int dx1, dy1, dx2, dy2;
    unsigned int n1, n2;
    if (uobj.placement.repetition->grid_params(&dx1, &dy1, &n1,
            &dx2, &dy2, &n2)) {
        if (n1 > 1)
            TInversePoint(&dx1, &dy1);
        if (n2 > 1)
            TInversePoint(&dx2, &dy2);

        if (dx2 == 0 && dy1 == 0) {
            bool badprm = false;
            *dx = dx1;
            *nx = n1;
            if (dx1 == 0 && n1 > 1) {
                badprm = true;
                *nx = 1;
            }
            *dy = dy2;
            *ny = n2;
            if (dy2 == 0 && n2 > 1) {
                badprm = true;
                *ny = 1;
            }
            if (badprm)
                warning("bad array parameters (scalarized)",
                    uobj.placement.name, uobj.placement.x, uobj.placement.y);
            return (true);
        }
        if (dx1 == 0 && dy2 == 0) {
            bool badprm = false;
            *dx = dx2;
            *nx = n2;
            if (dx2 == 0 && n2 > 1) {
                badprm = true;
                *nx = 1;
            }
            *dy = dy1;
            *ny = n1;
            if (dy1 == 0 && n1 > 1) {
                badprm = true;
                *ny = 1;
            }
            if (badprm)
                warning("bad array parameters (scalarized)",
                    uobj.placement.name, uobj.placement.x, uobj.placement.y);
            return (true);
        }
    }
    return (false);
}
// End of oas_in functions


oas_table::~oas_table()
{
    for (unsigned int i = 0; i <= hashmask; i++) {
        oas_elt *e = tab[i];
        tab[i] = 0;
        while (e) {
            oas_elt *ex = e;
            e = e->e_next;
            delete ex;
        }
    }
}


oas_elt *
oas_table::get(unsigned int x)
{
    unsigned int k = hash(x);
    for (oas_elt *e = tab[k]; e; e = e->e_next) {
        if (e->e_index == x)
            return (e);
    }
    return (0);
}


oas_elt *
oas_table::add(unsigned int x)
{
    unsigned int k = hash(x);
    tab[k] = new oas_elt(x, tab[k]);
    count++;
    return (tab[k]);
}


// If the density exceeds the limit, rebuild the table, returning the
// new one, or the old one if not rebuilt.  Should be called
// periodically when adding elements.
//
oas_table *
oas_table::check_rehash()
{
    if (count/(hashmask + 1) <= ST_MAX_DENS)
        return (this);

    unsigned int newmask = (hashmask << 1) | 1;
    oas_table *st =
        (oas_table*)new char[sizeof(oas_table) +
            newmask*sizeof(oas_elt*)];
    st->count = count;
    st->hashmask = newmask;
    for (unsigned int i = 0; i <= newmask; i++)
        st->tab[i] = 0;
    for (unsigned int i = 0;  i <= hashmask; i++) {
        oas_elt *en;
        for (oas_elt *e = tab[i]; e; e = en) {
            en = e->e_next;
            int j = st->hash(e->e_index);
            e->e_next = st->tab[j];
            st->tab[j] = e;
        }
        tab[i] = 0;
    }
    delete this;
    return (st);
}
// End of oas_table functions


// Return periodic array parameters, or false if repetition not a
// periodic type.
//
bool
oas_repetition::grid_params(int *dx1, int *dy1, unsigned int *n1,
    int *dx2, int *dy2, unsigned int *n2)
{
    if (type == 1) {
        *n1 = xdim + 2;
        *dx1 = xspace;
        *dy1 = 0;
        *n2 = ydim + 2;
        *dx2 = 0;
        *dy2 = yspace;
    }
    else if (type == 2) {
        *n1 = xdim + 2;
        *dx1 = xspace;
        *dy1 = 0;
        *n2 = 1;
        *dx2 = 0;
        *dy2 = 0;
    }
    else if (type == 3) {
        *n1 = 1;
        *dx1 = 0;
        *dy1 = 0;
        *n2 = ydim + 2;
        *dx2 = 0;
        *dy2 = yspace;
    }
    else if (type == 8) {
        *n1 = xdim + 2;
        *dx1 = array[0];
        *dy1 = array[1];
        *n2 = ydim + 2;
        *dx2 = array[2];
        *dy2 = array[3];
    }
    else if (type == 9) {
        *n1 = xdim + 2;
        *dx1 = xspace;
        *dy1 = yspace;
        *n2 = 1;
        *dx2 = 0;
        *dy2 = 0;
    }
    else
        return (false);
    return (true);
}
// End of oas_repetition functions


// Iterator, return the offsets for each replication.  Return false
// when complete.
//
bool
oas_rgen::next(int *xoff, int *yoff)
{
    if (done)
        return (false);
    if (!r) {
        done = true;
        *xoff = 0;
        *yoff = 0;
        return (true);
    }
    unsigned int x = xnext;
    unsigned int y = ynext;

#ifdef HAVE_COMPUTED_GOTO
    static void *array[] = { &&lbl1, &&lbl1, &&lbl2, &&lbl3, &&lbl4, &&lbl5,
        &&lbl6, &&lbl7, &&lbl8, &&lbl9, &&lbl10, &&lbl11 };
    if (r->type < 1 || r->type > 11)
        return (false);
    goto *array[r->type];
#endif

#ifdef HAVE_COMPUTED_GOTO
lbl1:
#else
    if (r->type == 1) {
#endif
        if (xnext++ > r->xdim) {
            xnext = 0;
            if (ynext++ > r->ydim)
                done = true;
        }
        *xoff = x*r->xspace;
        *yoff = y*r->yspace;
#ifdef HAVE_COMPUTED_GOTO
        return (true);
lbl2:
#else
    }
    else if (r->type == 2) {
#endif
        if (xnext++ > r->xdim)
            done = true;
        *xoff = x*r->xspace;
        *yoff = 0;
#ifdef HAVE_COMPUTED_GOTO
        return (true);
lbl3:
#else
    }
    else if (r->type == 3) {
#endif
        if (ynext++ > r->ydim)
            done = true;
        *xoff = 0;
        *yoff = y*r->yspace;
#ifdef HAVE_COMPUTED_GOTO
        return (true);
lbl4:
lbl5:
#else
    }
    else if (r->type == 4 || r->type == 5) {
#endif
        if (xnext++ > r->xdim)
            done = true;
        *xoff = xacc;
        *yoff = 0;
        if (x <= r->xdim)
            xacc += r->array[x];
#ifdef HAVE_COMPUTED_GOTO
        return (true);
lbl6:
lbl7:
#else
    }
    else if (r->type == 6 || r->type == 7) {
#endif
        if (ynext++ > r->ydim)
            done = true;
        *xoff = 0;
        *yoff = yacc;
        if (y <= r->ydim)
            yacc += r->array[y];
#ifdef HAVE_COMPUTED_GOTO
        return (true);
lbl8:
#else
    }
    else if (r->type == 8) {
#endif
        if (xnext++ > r->xdim) {
            xnext = 0;
            if (ynext++ > r->ydim)
                done = true;
        }
        *xoff = x*r->array[0] + y*r->array[2];
        *yoff = x*r->array[1] + y*r->array[3];
#ifdef HAVE_COMPUTED_GOTO
        return (true);
lbl9:
#else
    }
    else if (r->type == 9) {
#endif
        if (xnext++ > r->xdim)
            done = true;
        *xoff = x*r->xspace;
        *yoff = x*r->yspace;
#ifdef HAVE_COMPUTED_GOTO
        return (true);
lbl10:
lbl11:
#else
    }
    else if (r->type == 10 || r->type == 11) {
#endif
        if (xnext++ > r->xdim)
            done = true;
        *xoff = xacc;
        *yoff = yacc;
        if (x <= r->xdim) {
            xacc += r->array[2*x];
            yacc += r->array[2*x + 1];
        }
#ifdef HAVE_COMPUTED_GOTO
#else
    }
    else
        return (false);
#endif

    return (true);
}
// End of oas_rgen functions

